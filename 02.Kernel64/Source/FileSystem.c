#include "FileSystem.h"
#include "HardDisk.h"
#include "DynamicMemory.h"

static FILESYSTEM_MANAGER gFileSystemManager;
static BYTE gTempBuf[FILESYSTEM_SECTOR_PER_CLUSTER * 512]; // one cluster

fReadHDDInformation gReadHDDInformation = NULL;
fReadHDDSector gReadHDDSector = NULL;
fWriteHDDSector gWriteHDDSector = NULL;

BOOL initFileSystem(void)
{
	memset(&gFileSystemManager, 0, sizeof(gFileSystemManager));
	initMutex(&gFileSystemManager.mutex);

	if (initHDD())
	{
		gReadHDDInformation = readHDDInformation;
		gReadHDDSector = readHDDSector;
		gWriteHDDSector = writeHDDSector;
	}
	else
	{
		return FALSE;
	}
	return mount();
}

BOOL mount(void)
{
	MBR* pMBR;

	lock(&gFileSystemManager.mutex);
	if (!gReadHDDSector(TRUE, TRUE, 0, 1, gTempBuf))
	{
		unlock(&gFileSystemManager.mutex);
		return FALSE;
	}

	pMBR = (MBR*)gTempBuf;
	if (pMBR->signature != FILESYSTEM_SIGNATURE)
	{
		unlock(&gFileSystemManager.mutex);
		return FALSE;
	}

	gFileSystemManager.isMounted = TRUE;
	gFileSystemManager.reservedSectorCount = pMBR->reservedSectorCount;
	gFileSystemManager.clusterLinkAreaStartAddress =\
		pMBR->reservedSectorCount + 1;
	gFileSystemManager.clusterLinkAreaSize = pMBR->clusterLinkSectorCount;
	gFileSystemManager.dataAreaStartAddress =\
		pMBR->reservedSectorCount + 1 + pMBR->clusterLinkSectorCount;
	gFileSystemManager.totalClusterCount = pMBR->totalClusterCount;

	unlock(&gFileSystemManager.mutex);
	return TRUE;
}

BOOL format(void)
{
	HDDINFORMATION* HDD;
	MBR* pMBR;
	DWORD totalSectorCount, remainSectorCount;
	DWORD maxClusterCount, clusterCount, clusterLinkSectorCount;
	DWORD i;

	HDD = (HDDINFORMATION*)gTempBuf;

	if (!gReadHDDInformation(TRUE, TRUE, HDD))
	{
		unlock(&gFileSystemManager.mutex);
		return FALSE;
	}
	totalSectorCount = HDD->totalSectors;

	maxClusterCount = totalSectorCount / FILESYSTEM_SECTOR_PER_CLUSTER;
	clusterLinkSectorCount = (maxClusterCount + 127) / 128;

	// remain = total - link table sector - MBR
	remainSectorCount = totalSectorCount - clusterLinkSectorCount - 1;
	clusterCount = remainSectorCount / FILESYSTEM_SECTOR_PER_CLUSTER;
	clusterLinkSectorCount = (clusterCount + 127) / 128;

	pMBR = (MBR*)gTempBuf;
	memset(pMBR->partition, 0, sizeof(pMBR->partition));
	pMBR->signature = FILESYSTEM_SIGNATURE;
	pMBR->reservedSectorCount = 0;
	pMBR->clusterLinkSectorCount = clusterLinkSectorCount;
	pMBR->totalClusterCount = clusterCount;

	if (!gWriteHDDSector(TRUE, TRUE, 0, 1, gTempBuf))
	{
		unlock(&gFileSystemManager.mutex);
		return FALSE;
	}

	memset(gTempBuf, 0, 512);

	// init link table + root directory cluster
	for (i=0; i < pMBR->clusterLinkSectorCount + FILESYSTEM_SECTOR_PER_CLUSTER; i++)
	{
		if (i == 0)
		{
			// set first link which pointing root directory
			*(DWORD*)&gTempBuf[0] = FILESYSTEM_LAST_CLUSTER;
		}
		else
			*(DWORD*)&gTempBuf[0] = FILESYSTEM_FREE_CLUSTER;

		// skip MBR
		if (!gWriteHDDSector(TRUE, TRUE, i + 1, 1, gTempBuf))
		{
			unlock(&gFileSystemManager.mutex);
			return FALSE;
		}
	}
	unlock(&gFileSystemManager.mutex);
	return TRUE;
}

BOOL getHDDInformation(HDDINFORMATION* information)
{
	BOOL res;

	lock(&gFileSystemManager.mutex);
	res = gReadHDDInformation(TRUE, TRUE, information);
	unlock(&gFileSystemManager.mutex);
	return res;
}

BOOL readClusterLinkTable(DWORD offset, BYTE* buf)
{
	return gReadHDDSector(TRUE, TRUE, offset +\
			gFileSystemManager.clusterLinkAreaStartAddress, 1, buf);
}

BOOL writeClusterLinkTable(DWORD offset, BYTE* buf)
{
	return gWriteHDDSector(TRUE, TRUE, offset +\
			gFileSystemManager.clusterLinkAreaStartAddress, 1, buf);
}

BOOL readCluster(DWORD offset, BYTE* buf)
{
	return gReadHDDSector(TRUE, TRUE,
			(offset * FILESYSTEM_SECTOR_PER_CLUSTER) + gFileSystemManager.dataAreaStartAddress,
			FILESYSTEM_SECTOR_PER_CLUSTER,
			buf);
}

BOOL writeCluster(DWORD offset, BYTE* buf)
{
	return gWriteHDDSector(TRUE, TRUE,
			(offset * FILESYSTEM_SECTOR_PER_CLUSTER) + gFileSystemManager.dataAreaStartAddress,
			FILESYSTEM_SECTOR_PER_CLUSTER,
			buf);
}

DWORD findFreeCluster(void)
{
	DWORD lastSectorOffset;
	DWORD linkCountInSector, currentSectorOffset;
	DWORD i, j;

	if (!gFileSystemManager.isMounted)
		return FILESYSTEM_LAST_CLUSTER;
	lastSectorOffset = gFileSystemManager.lastAllocatedClusterLinkSectorOffset;
	for (i=0; i < gFileSystemManager.clusterLinkAreaSize; i++)
	{
		if (lastSectorOffset + i == gFileSystemManager.clusterLinkAreaSize)
			linkCountInSector = gFileSystemManager.totalClusterCount % 128;
		else
			linkCountInSector = 128;

		currentSectorOffset = (lastSectorOffset + i) % gFileSystemManager.clusterLinkAreaSize;

		if (!readClusterLinkTable(currentSectorOffset, gTempBuf))
			return FILESYSTEM_LAST_CLUSTER;
		for (j=0; j < linkCountInSector; j++)
		{
			if (((DWORD*)gTempBuf)[j] == FILESYSTEM_FREE_CLUSTER)
				break;
		}
		if (j != linkCountInSector)
		{
			gFileSystemManager.lastAllocatedClusterLinkSectorOffset = currentSectorOffset;
			return currentSectorOffset * 128 + j;
		}
	}
	return FILESYSTEM_LAST_CLUSTER;
}

BOOL setClusterLinkData(DWORD clusterIdx, DWORD data)
{
	DWORD sectorOffset;

	if (!gFileSystemManager.isMounted)
		return FALSE;
	sectorOffset = clusterIdx / 128;
	if (sectorOffset >= gFileSystemManager.clusterLinkAreaSize)
		return FALSE;
	if (!readClusterLinkTable(sectorOffset, gTempBuf))
		return FALSE;
	((DWORD*)gTempBuf)[clusterIdx % 128] = data;
	if (!writeClusterLinkTable(sectorOffset, gTempBuf))
		return FALSE;
	return TRUE;
}

BOOL getClusterLinkData(DWORD clusterIdx, DWORD* data)
{
	DWORD sectorOffset;

	if (!gFileSystemManager.isMounted)
		return FALSE;
	sectorOffset = clusterIdx / 128;
	if (sectorOffset >= gFileSystemManager.clusterLinkAreaSize)
		return FALSE;
	if (!readClusterLinkTable(sectorOffset, gTempBuf))
		return FALSE;
	*data = ((DWORD*)gTempBuf)[clusterIdx % 128];
	return TRUE;
}

int findFreeDirectoryEntry(void)
{
	DIRECTORY_ENTRY* entry;
	DWORD i;

	if (!gFileSystemManager.isMounted)
		return FALSE;
	if (!readCluster(0, gTempBuf))
		return -1;
	entry = (DIRECTORY_ENTRY*)gTempBuf;
	for (i=0; i < FILESYSTEM_MAX_DIRECTORY_ENTRY_COUNT; i++)
	{
		if (entry[i].startClusterIdx == 0)
			return i;
	}
	return -1;
}

BOOL setDirectoryEntryData(int idx, DIRECTORY_ENTRY* entry)
{
	DIRECTORY_ENTRY* destEntry;

	if (!gFileSystemManager.isMounted ||
		idx < 0 ||
		idx >= FILESYSTEM_MAX_DIRECTORY_ENTRY_COUNT)
		return FALSE;

	if (!readCluster(0, gTempBuf))
		return FALSE;
	destEntry = (DIRECTORY_ENTRY*)gTempBuf;
	memcpy(destEntry + idx, entry, sizeof(DIRECTORY_ENTRY));
	if (!writeCluster(0, gTempBuf))
		return FALSE;
	return TRUE;
}

BOOL getDirectoryEntryData(int idx, DIRECTORY_ENTRY* entry)
{
	DIRECTORY_ENTRY* srcEntry;

	if (!gFileSystemManager.isMounted ||
		idx < 0 ||
		idx >= FILESYSTEM_MAX_DIRECTORY_ENTRY_COUNT)
		return FALSE;

	if (!readCluster(0, gTempBuf))
		return FALSE;
	srcEntry = (DIRECTORY_ENTRY*)gTempBuf;
	memcpy(entry, srcEntry + idx, sizeof(DIRECTORY_ENTRY));
	return TRUE;
}

int findDirectoryEntry(char* fileName, DIRECTORY_ENTRY* entry)
{
	DIRECTORY_ENTRY* srcEntry;
	DWORD i;
	int len;

	if (!gFileSystemManager.isMounted)
		return -1;
	if (!readCluster(0, gTempBuf))
		return -1;

	len = strlen(fileName);
	srcEntry = (DIRECTORY_ENTRY*)gTempBuf;
	for (i=0; i < FILESYSTEM_MAX_DIRECTORY_ENTRY_COUNT; i++)
	{
		if (memcmp(srcEntry[i].fileName, fileName, len) == 0)
		{
			memcpy(entry, srcEntry + i, sizeof(DIRECTORY_ENTRY));
			return i;
		}
	}
	return -1;
}

void getFileSystemInformation(FILESYSTEM_MANAGER* manager)
{
	memcpy(manager, &gFileSystemManager, sizeof(FILESYSTEM_MANAGER));
}
