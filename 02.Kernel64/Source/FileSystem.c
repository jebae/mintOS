#include "FileSystem.h"
#include "HardDisk.h"
#include "DynamicMemory.h"
#include "Task.h"
#include "Utility.h"
#include "CacheManager.h"
#include "RAMDisk.h"
#include "Console.h"

static FILESYSTEM_MANAGER gFileSystemManager;
static BYTE gTempBuf[FILESYSTEM_SECTOR_PER_CLUSTER * 512]; // one cluster

fReadHDDInformation gReadHDDInformation = NULL;
fReadHDDSector gReadHDDSector = NULL;
fWriteHDDSector gWriteHDDSector = NULL;

BOOL initFileSystem(void)
{
	BOOL cacheEnabled = FALSE;

	memset(&gFileSystemManager, 0, sizeof(gFileSystemManager));
	initMutex(&gFileSystemManager.mutex);

	if (initHDD())
	{
		gReadHDDInformation = readHDDInformation;
		gReadHDDSector = readHDDSector;
		gWriteHDDSector = writeHDDSector;
		cacheEnabled = TRUE;
	}
	else if (initRDD(RDD_TOTAL_SECTOR_COUNT))
	{
		gReadHDDInformation = readRDDInformation;
		gReadHDDSector = readRDDSector;
		gWriteHDDSector = writeRDDSector;
		if (!format())
			return FALSE;
	}
	else
	{
		return FALSE;
	}
	if (!mount())
		return FALSE;

	gFileSystemManager.handlePool = (FILE*)allocateMemory(sizeof(FILE) * \
			FILESYSTEM_HANDLE_MAX_COUNT);
	if (gFileSystemManager.handlePool == NULL)
	{
		gFileSystemManager.isMounted = FALSE;
		return FALSE;
	}
	memset(gFileSystemManager.handlePool, 0, sizeof(FILE) * \
			FILESYSTEM_HANDLE_MAX_COUNT);
	if (cacheEnabled)
		gFileSystemManager.cacheEnabled = initCacheManager();
	return TRUE;
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

	lock(&gFileSystemManager.mutex);
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
			((DWORD*)gTempBuf)[0] = FILESYSTEM_LAST_CLUSTER;
		}
		else
			((DWORD*)gTempBuf)[0] = FILESYSTEM_FREE_CLUSTER;

		// skip MBR
		if (!gWriteHDDSector(TRUE, TRUE, i + 1, 1, gTempBuf))
		{
			unlock(&gFileSystemManager.mutex);
			return FALSE;
		}
	}

	if (gFileSystemManager.cacheEnabled)
	{
		discardAllCacheBuffer(CACHE_CLUSTER_LINKTABLE_AREA);
		discardAllCacheBuffer(CACHE_DATA_AREA);
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

static BOOL readClusterLinkTable(DWORD offset, BYTE* buf)
{
	if (gFileSystemManager.cacheEnabled)
		return readClusterLinkTableWithCache(offset, buf);
	else
		return readClusterLinkTableWithoutCache(offset, buf);
}

static BOOL readClusterLinkTableWithoutCache(DWORD offset, BYTE* buf)
{
	return gReadHDDSector(TRUE, TRUE,
			offset + gFileSystemManager.clusterLinkAreaStartAddress, 1, buf);
}

static BOOL readClusterLinkTableWithCache(DWORD offset, BYTE* buf)
{
	CACHE_BUFFER* cacheBuf;

	cacheBuf = findCacheBuffer(CACHE_CLUSTER_LINKTABLE_AREA, offset);
	if (cacheBuf != NULL)
	{
		memcpy(buf, cacheBuf->buf, 512);
		return TRUE;
	}

	if (!readClusterLinkTableWithoutCache(offset, buf))
		return FALSE;

	cacheBuf = allocateCacheBufferWithFlush(CACHE_CLUSTER_LINKTABLE_AREA);
	if (cacheBuf == NULL)
		return FALSE;

	memcpy(cacheBuf->buf, buf, 512);
	cacheBuf->tag = offset;
	cacheBuf->isChanged = FALSE;
	return TRUE;
}

static CACHE_BUFFER* allocateCacheBufferWithFlush(int cacheTableIdx)
{
	CACHE_BUFFER* cacheBuf;

	cacheBuf = allocateCacheBuffer(cacheTableIdx);
	if (cacheBuf != NULL)
		return cacheBuf;

	cacheBuf = getVictimInCacheBuffer(cacheTableIdx);
	if (cacheBuf == NULL)
	{
		printf("allocateCacheBufferWithFlush: Cache allocate fail\n");
		return NULL;
	}

	if (cacheBuf->isChanged)
	{
		switch (cacheTableIdx)
		{
			case CACHE_CLUSTER_LINKTABLE_AREA:
				if (!writeClusterLinkTableWithoutCache(cacheBuf->tag, cacheBuf->buf))
				{
					printf("allocateCacheBufferWithFlush: Cache buffer write fail\n");
					return NULL;
				}
				break;
			case CACHE_DATA_AREA:
				if (!writeClusterWithoutCache(cacheBuf->tag, cacheBuf->buf))
				{
					printf("allocateCacheBufferWithFlush: Cache buffer write fail\n");
					return NULL;
				}
				break;
			default:
				printf("allocateCacheBufferWithFlush: Cache buffer write fail\n");
				return NULL;
		}
	}
	return cacheBuf;
}

static BOOL writeClusterLinkTable(DWORD offset, BYTE* buf)
{
	if (gFileSystemManager.cacheEnabled)
		return writeClusterLinkTableWithCache(offset, buf);
	else
		return writeClusterLinkTableWithoutCache(offset, buf);
}

static BOOL writeClusterLinkTableWithoutCache(DWORD offset, BYTE* buf)
{
	return gWriteHDDSector(TRUE, TRUE,
			offset + gFileSystemManager.clusterLinkAreaStartAddress, 1, buf);
}

static BOOL writeClusterLinkTableWithCache(DWORD offset, BYTE* buf)
{
	CACHE_BUFFER* cacheBuf;

	cacheBuf = findCacheBuffer(CACHE_CLUSTER_LINKTABLE_AREA, offset);
	if (cacheBuf != NULL)
	{
		memcpy(cacheBuf->buf, buf, 512);
		cacheBuf->isChanged = TRUE;
		return TRUE;
	}

	cacheBuf = allocateCacheBufferWithFlush(CACHE_CLUSTER_LINKTABLE_AREA);
	if (cacheBuf == NULL)
		return FALSE;

	memcpy(cacheBuf->buf, buf, 512);
	cacheBuf->tag = offset;
	cacheBuf->isChanged = TRUE;
	return TRUE;
}

static BOOL readCluster(DWORD offset, BYTE* buf)
{
	if (gFileSystemManager.cacheEnabled)
		return readClusterWithCache(offset, buf);
	else
		return readClusterWithoutCache(offset, buf);
}

static BOOL readClusterWithoutCache(DWORD offset, BYTE* buf)
{
	return gReadHDDSector(TRUE, TRUE,
			(offset * FILESYSTEM_SECTOR_PER_CLUSTER) + gFileSystemManager.dataAreaStartAddress,
			FILESYSTEM_SECTOR_PER_CLUSTER,
			buf);
}

static BOOL readClusterWithCache(DWORD offset, BYTE* buf)
{
	CACHE_BUFFER* cacheBuf;

	cacheBuf = findCacheBuffer(CACHE_DATA_AREA, offset);
	if (cacheBuf != NULL)
	{
		memcpy(buf, cacheBuf->buf, 512);
		return TRUE;
	}

	if (!readClusterWithoutCache(offset, buf))
		return FALSE;

	cacheBuf = allocateCacheBufferWithFlush(CACHE_DATA_AREA);
	if (cacheBuf == NULL)
		return FALSE;

	memcpy(cacheBuf->buf, buf, FILESYSTEM_CLUSTER_SIZE);
	cacheBuf->tag = offset;
	cacheBuf->isChanged = FALSE;
	return TRUE;
}

static BOOL writeCluster(DWORD offset, BYTE* buf)
{
	if (gFileSystemManager.cacheEnabled)
		return writeClusterWithCache(offset, buf);
	else
		return writeClusterWithoutCache(offset, buf);
}

static BOOL writeClusterWithoutCache(DWORD offset, BYTE* buf)
{
	return gWriteHDDSector(TRUE, TRUE,
			(offset * FILESYSTEM_SECTOR_PER_CLUSTER) + gFileSystemManager.dataAreaStartAddress,
			FILESYSTEM_SECTOR_PER_CLUSTER,
			buf);
}

static BOOL writeClusterWithCache(DWORD offset, BYTE* buf)
{
	CACHE_BUFFER* cacheBuf;

	cacheBuf = findCacheBuffer(CACHE_DATA_AREA, offset);
	if (cacheBuf != NULL)
	{
		memcpy(cacheBuf->buf, buf, FILESYSTEM_CLUSTER_SIZE);
		cacheBuf->isChanged = TRUE;
		return TRUE;
	}
	cacheBuf = allocateCacheBufferWithFlush(CACHE_DATA_AREA);
	if (cacheBuf == NULL)
		return FALSE;
	memcpy(cacheBuf->buf, buf, FILESYSTEM_CLUSTER_SIZE);
	cacheBuf->tag = offset;
	cacheBuf->isChanged = TRUE;
	return TRUE;
}

BOOL flushFileSystemCache(void)
{
	CACHE_BUFFER* cacheBuf;
	int cacheCount;
	int i;

	if (!gFileSystemManager.cacheEnabled)
		return TRUE;

	lock(&gFileSystemManager.mutex);

	getCacheBufferAndCount(CACHE_CLUSTER_LINKTABLE_AREA, &cacheBuf, &cacheCount);
	for (i=0; i < cacheCount; i++)
	{
		if (cacheBuf[i].isChanged)
		{
			if (!writeClusterLinkTableWithoutCache(cacheBuf[i].tag, cacheBuf[i].buf))
			{
				unlock(&gFileSystemManager.mutex);
				return FALSE;
			}
			cacheBuf[i].isChanged = FALSE;
		}
	}

	getCacheBufferAndCount(CACHE_DATA_AREA, &cacheBuf, &cacheCount);
	for (i=0; i < cacheCount; i++)
	{
		if (cacheBuf[i].isChanged)
		{
			if (!writeClusterWithoutCache(cacheBuf[i].tag, cacheBuf[i].buf))
			{
				unlock(&gFileSystemManager.mutex);
				return FALSE;
			}
			cacheBuf[i].isChanged = FALSE;
		}
	}
	unlock(&gFileSystemManager.mutex);
	return TRUE;
}

static DWORD findFreeCluster(void)
{
	DWORD lastSectorOffset, currentSectorOffset;
	DWORD linkCountInSector;
	DWORD i, j;

	if (!gFileSystemManager.isMounted)
		return FILESYSTEM_LAST_CLUSTER;
	lastSectorOffset = gFileSystemManager.lastAllocatedClusterLinkSectorOffset;
	for (i=0; i < gFileSystemManager.clusterLinkAreaSize; i++)
	{
		if (lastSectorOffset + i == gFileSystemManager.clusterLinkAreaSize - 1)
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

static BOOL setClusterLinkData(DWORD clusterIdx, DWORD data)
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

static BOOL getClusterLinkData(DWORD clusterIdx, DWORD* data)
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

static int findFreeDirectoryEntry(void)
{
	DIRECTORY_ENTRY* entry;
	DWORD i;

	if (!gFileSystemManager.isMounted)
		return -1;
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

static BOOL setDirectoryEntryData(int idx, DIRECTORY_ENTRY* entry)
{
	DIRECTORY_ENTRY* destEntry;

	if (!gFileSystemManager.isMounted ||
		(idx < 0) ||
		(idx >= FILESYSTEM_MAX_DIRECTORY_ENTRY_COUNT))
		return FALSE;

	if (!readCluster(0, gTempBuf))
		return FALSE;
	destEntry = (DIRECTORY_ENTRY*)gTempBuf;
	memcpy(destEntry + idx, entry, sizeof(DIRECTORY_ENTRY));
	if (!writeCluster(0, gTempBuf))
		return FALSE;
	return TRUE;
}

static BOOL getDirectoryEntryData(int idx, DIRECTORY_ENTRY* entry)
{
	DIRECTORY_ENTRY* srcEntry;

	if (!gFileSystemManager.isMounted ||
		(idx < 0) ||
		(idx >= FILESYSTEM_MAX_DIRECTORY_ENTRY_COUNT))
		return FALSE;

	if (!readCluster(0, gTempBuf))
		return FALSE;
	srcEntry = (DIRECTORY_ENTRY*)gTempBuf;
	memcpy(entry, srcEntry + idx, sizeof(DIRECTORY_ENTRY));
	return TRUE;
}

static int findDirectoryEntry(const char* fileName, DIRECTORY_ENTRY* entry)
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

static void* allocateFileDirectoryHandle(void)
{
	int i;
	FILE* file = gFileSystemManager.handlePool;

	for (i=0; i < FILESYSTEM_HANDLE_MAX_COUNT; i++)
	{
		if (file->type == FILESYSTEM_TYPE_FREE)
		{
			file->type = FILESYSTEM_TYPE_FILE;
			return file;
		}
		file++;
	}
	return NULL;
}

static void freeFileDirectoryHandle(FILE* file)
{
	memset(file, 0, sizeof(FILE));
	file->type = FILESYSTEM_TYPE_FREE;
}

static BOOL createFile(const char* fileName, DIRECTORY_ENTRY* entry,\
		int* directoryEntryOffset)
{
	DWORD cluster;

	cluster = findFreeCluster();
	if (cluster == FILESYSTEM_LAST_CLUSTER ||
			!setClusterLinkData(cluster, FILESYSTEM_LAST_CLUSTER))
		return FALSE;

	*directoryEntryOffset = findFreeDirectoryEntry();
	if (*directoryEntryOffset == -1)
	{
		setClusterLinkData(cluster, FILESYSTEM_FREE_CLUSTER);
		return FALSE;
	}

	memcpy(entry->fileName, fileName, strlen(fileName) + 1);
	entry->startClusterIdx = cluster;
	entry->fileSize = 0;

	if (!setDirectoryEntryData(*directoryEntryOffset, entry))
	{
		setClusterLinkData(cluster, FILESYSTEM_FREE_CLUSTER);
		return FALSE;
	}
	return TRUE;
}

static BOOL freeClusterUntilEnd(DWORD clusterIdx)
{
	DWORD nextClusterIdx;

	while (clusterIdx != FILESYSTEM_LAST_CLUSTER)
	{
		if (!getClusterLinkData(clusterIdx, &nextClusterIdx))
			return FALSE;
		if (!setClusterLinkData(clusterIdx, FILESYSTEM_FREE_CLUSTER))
			return FALSE;
		clusterIdx = nextClusterIdx;
	}
	return TRUE;
}

FILE* openFile(const char* fileName, const char* mode)
{
	DIRECTORY_ENTRY entry;
	int directoryEntryOffset;
	int fileNameLength;
	DWORD secondCluster;
	FILE* file;

	fileNameLength = strlen(fileName);
	if (fileNameLength == 0 || (fileNameLength > sizeof(entry.fileName) - 1))
		return NULL;

	lock(&gFileSystemManager.mutex);

	directoryEntryOffset = findDirectoryEntry(fileName, &entry);
	if (directoryEntryOffset == -1)
	{
		if (mode[0] == 'r')
		{
			unlock(&gFileSystemManager.mutex);
			return NULL;
		}
		if (!createFile(fileName, &entry, &directoryEntryOffset))
		{
			unlock(&gFileSystemManager.mutex);
			return NULL;
		}
	}
	else if (mode[0] == 'w')
	{
		if (!getClusterLinkData(entry.startClusterIdx, &secondCluster))
		{
			unlock(&gFileSystemManager.mutex);
			return NULL;
		}
		if (!setClusterLinkData(entry.startClusterIdx, FILESYSTEM_LAST_CLUSTER))
		{
			unlock(&gFileSystemManager.mutex);
			return NULL;
		}
		if (!freeClusterUntilEnd(secondCluster))
		{
			unlock(&gFileSystemManager.mutex);
			return NULL;
		}
		entry.fileSize = 0;
		if (!setDirectoryEntryData(directoryEntryOffset, &entry))
		{
			unlock(&gFileSystemManager.mutex);
			return NULL;
		}
	}

	file = allocateFileDirectoryHandle();
	if (file == NULL)
	{
		unlock(&gFileSystemManager.mutex);
		return NULL;
	}
	file->type = FILESYSTEM_TYPE_FILE;
	file->fileHandle.directoryEntryOffset = directoryEntryOffset;
	file->fileHandle.fileSize = entry.fileSize;
	file->fileHandle.startClusterIdx = entry.startClusterIdx;
	file->fileHandle.currentClusterIdx = entry.startClusterIdx;
	file->fileHandle.prevClusterIdx = entry.startClusterIdx;
	file->fileHandle.currentOffset = 0;

	if (mode[0] == 'a')
		seekFile(file, 0, FILESYSTEM_SEEK_END);

	unlock(&gFileSystemManager.mutex);
	return file;
}

DWORD readFile(void* buf, DWORD size, DWORD count, FILE* file)
{
	FILE_HANDLE* fileHandle;
	DWORD totalSize, readSize, copySize;
	DWORD offsetInCluster;
	DWORD nextClusterIdx;

	if (file == NULL || file->type != FILESYSTEM_TYPE_FILE)
		return 0;

	fileHandle = &file->fileHandle;

	if (fileHandle->currentOffset == fileHandle->fileSize ||
			fileHandle->currentClusterIdx == FILESYSTEM_LAST_CLUSTER)
		return 0;

	totalSize = MIN(size * count, fileHandle->fileSize - fileHandle->currentOffset);
	readSize = 0;
	lock(&gFileSystemManager.mutex);
	while (readSize != totalSize)
	{
		if (!readCluster(fileHandle->currentClusterIdx, gTempBuf))
			break;

		offsetInCluster = fileHandle->currentOffset % FILESYSTEM_CLUSTER_SIZE;
		copySize = MIN(totalSize - readSize,
				FILESYSTEM_CLUSTER_SIZE - offsetInCluster);
		memcpy((char*)buf + readSize, gTempBuf + offsetInCluster, copySize);

		readSize += copySize;
		fileHandle->currentOffset += copySize;

		if (fileHandle->currentOffset % FILESYSTEM_CLUSTER_SIZE == 0)
		{
			if (!getClusterLinkData(fileHandle->currentClusterIdx, &nextClusterIdx))
				break;
			fileHandle->prevClusterIdx = fileHandle->currentClusterIdx;
			fileHandle->currentClusterIdx = nextClusterIdx;
		}
	}
	unlock(&gFileSystemManager.mutex);
	return readSize / size;
}

static BOOL updateDirectoryEntry(FILE_HANDLE* fileHandle)
{
	DIRECTORY_ENTRY entry;

	if (fileHandle == NULL ||\
			!getDirectoryEntryData(fileHandle->directoryEntryOffset, &entry))
		return FALSE;

	entry.fileSize = fileHandle->fileSize;
	entry.startClusterIdx = fileHandle->startClusterIdx;
	if (!setDirectoryEntryData(fileHandle->directoryEntryOffset, &entry))
		return FALSE;
	return TRUE;
}

DWORD writeFile(const void* buf, DWORD size, DWORD count, FILE* file)
{
	FILE_HANDLE* fileHandle;
	DWORD totalSize, writeSize, copySize;
	DWORD allocatedClusterIdx;
	DWORD nextClusterIdx;
	DWORD offsetInCluster;

	if (file == NULL || file->type != FILESYSTEM_TYPE_FILE)
		return 0;

	fileHandle = &file->fileHandle;
	totalSize = size * count;
	lock(&gFileSystemManager.mutex);
	writeSize = 0;
	while (writeSize != totalSize)
	{
		if (fileHandle->currentClusterIdx == FILESYSTEM_LAST_CLUSTER)
		{
			allocatedClusterIdx = findFreeCluster();
			if (allocatedClusterIdx == FILESYSTEM_LAST_CLUSTER)
				break;
			if (!setClusterLinkData(allocatedClusterIdx, FILESYSTEM_LAST_CLUSTER))
				break;
			if (!setClusterLinkData(fileHandle->prevClusterIdx, allocatedClusterIdx))
			{
				setClusterLinkData(allocatedClusterIdx, FILESYSTEM_FREE_CLUSTER);
				break;
			}
			fileHandle->currentClusterIdx = allocatedClusterIdx;

			memset(gTempBuf, 0, FILESYSTEM_CLUSTER_SIZE);
		}
		else if (fileHandle->currentOffset % FILESYSTEM_CLUSTER_SIZE != 0 ||
				totalSize - writeSize < FILESYSTEM_CLUSTER_SIZE)
		{
			if (!readCluster(fileHandle->currentClusterIdx, gTempBuf))
				break;
		}

		offsetInCluster = fileHandle->currentOffset % FILESYSTEM_CLUSTER_SIZE;
		copySize = MIN(totalSize - writeSize, FILESYSTEM_CLUSTER_SIZE - offsetInCluster);
		memcpy(gTempBuf + offsetInCluster, (char*)buf + writeSize, copySize);

		if (!writeCluster(fileHandle->currentClusterIdx, gTempBuf))
			break;

		writeSize += copySize;
		fileHandle->currentOffset += copySize;

		if (fileHandle->currentOffset % FILESYSTEM_CLUSTER_SIZE == 0)
		{
			if (!getClusterLinkData(fileHandle->currentClusterIdx, &nextClusterIdx))
				break;
			fileHandle->prevClusterIdx = fileHandle->currentClusterIdx;
			fileHandle->currentClusterIdx = nextClusterIdx;
		}
	}

	if (fileHandle->fileSize < fileHandle->currentOffset)
	{
		fileHandle->fileSize = fileHandle->currentOffset;
		updateDirectoryEntry(fileHandle);
	}
	unlock(&gFileSystemManager.mutex);
	return writeSize / size;
}

BOOL writeZero(FILE* file, DWORD count)
{
	BYTE* buf;
	DWORD writeSize;

	if (file == NULL)
		return FALSE;
	buf = (BYTE*)allocateMemory(FILESYSTEM_CLUSTER_SIZE);
	if (buf == NULL)
		return FALSE;
	memset(buf, 0, FILESYSTEM_CLUSTER_SIZE);
	while (count != 0)
	{
		writeSize = MIN(count, FILESYSTEM_CLUSTER_SIZE);
		if (writeFile(buf, 1, writeSize, file) != writeSize)
		{
			freeMemory(buf);
			return FALSE;
		}
		count -= writeSize;
	}
	freeMemory(buf);
	return TRUE;
}

int seekFile(FILE* file, int offset, int origin)
{
	FILE_HANDLE* fileHandle;
	DWORD realOffset;
	DWORD lastClusterOffset;
	DWORD currentClusterOffset;
	DWORD clusterOffsetToMove;
	DWORD moveCount;
	DWORD i;
	DWORD prevClusterIdx;
	DWORD currentClusterIdx;

	if (file == NULL || file->type != FILESYSTEM_TYPE_FILE)
		return 0;

	fileHandle = &file->fileHandle;
	switch (origin)
	{
		case FILESYSTEM_SEEK_SET:
			realOffset = (offset < 0) ? 0 : offset;
			break;
		case FILESYSTEM_SEEK_CUR:
			if (offset < 0 && fileHandle->currentOffset <= (DWORD)-offset)
				realOffset = 0;
			else
				realOffset = fileHandle->currentOffset + offset;
			break;
		case FILESYSTEM_SEEK_END:
			if (offset < 0 && fileHandle->fileSize <= (DWORD)-offset)
				realOffset = 0;
			else
				realOffset = fileHandle->fileSize + offset;
			break;
	}

	lastClusterOffset = fileHandle->fileSize / FILESYSTEM_CLUSTER_SIZE;
	currentClusterOffset = fileHandle->currentOffset / FILESYSTEM_CLUSTER_SIZE;
	clusterOffsetToMove = realOffset / FILESYSTEM_CLUSTER_SIZE;

	if (lastClusterOffset < clusterOffsetToMove)
	{
		moveCount = lastClusterOffset - currentClusterOffset;
		currentClusterIdx = fileHandle->currentClusterIdx;
	}
	else if (currentClusterOffset <= clusterOffsetToMove)
	{
		moveCount = clusterOffsetToMove - currentClusterOffset;
		currentClusterIdx = fileHandle->currentClusterIdx;
	}
	else
	{
		moveCount = clusterOffsetToMove;
		currentClusterIdx = fileHandle->startClusterIdx;
	}

	lock(&gFileSystemManager.mutex);

	for (i=0; i < moveCount; i++)
	{
		prevClusterIdx = currentClusterIdx;
		if (!getClusterLinkData(prevClusterIdx, &currentClusterIdx))
		{
			unlock(&gFileSystemManager.mutex);
			return -1;
		}
	}

	if (moveCount > 0)
	{
		fileHandle->currentClusterIdx = currentClusterIdx;
		fileHandle->prevClusterIdx = prevClusterIdx;
	}
	// CODE CHANGED
	else if (currentClusterIdx == fileHandle->startClusterIdx)
	{
		fileHandle->currentClusterIdx = fileHandle->startClusterIdx;
		fileHandle->prevClusterIdx = fileHandle->startClusterIdx;
	}

	if (lastClusterOffset < clusterOffsetToMove)
	{
		fileHandle->currentOffset = fileHandle->fileSize;
		if (!writeZero(file, realOffset - fileHandle->fileSize))
		{
			unlock(&gFileSystemManager.mutex);
			return 0;
		}
	}
	fileHandle->currentOffset = realOffset;
	unlock(&gFileSystemManager.mutex);
	return 0;
}

int closeFile(FILE* file)
{
	if (file == NULL || file->type != FILESYSTEM_TYPE_FILE)
		return -1;
	freeFileDirectoryHandle(file);
	return 0;
}

BOOL isFileOpened(const DIRECTORY_ENTRY* entry)
{
	int i;
	FILE* file;

	file = gFileSystemManager.handlePool;
	for (i=0; i < FILESYSTEM_HANDLE_MAX_COUNT; i++)
	{
		if (file[i].type == FILESYSTEM_TYPE_FILE &&
			entry->startClusterIdx == file[i].fileHandle.startClusterIdx)
			return TRUE;
	}
	return FALSE;
}

int removeFile(const char* fileName)
{
	DIRECTORY_ENTRY entry;
	int directoryEntryOffset;
	int fileNameLength;

	fileNameLength = strlen(fileName);
	if (fileNameLength > sizeof(entry.fileName) - 1 || fileNameLength == 0)
		return -1;

	lock(&gFileSystemManager.mutex);
	directoryEntryOffset = findDirectoryEntry(fileName, &entry);
	if (directoryEntryOffset == -1)
	{
		unlock(&gFileSystemManager.mutex);
		return -1;
	}
	if (isFileOpened(&entry))
	{
		unlock(&gFileSystemManager.mutex);
		return -1;
	}
	if (!freeClusterUntilEnd(entry.startClusterIdx))
	{
		unlock(&gFileSystemManager.mutex);
		return -1;
	}
	memset(&entry, 0, sizeof(entry));
	if (!setDirectoryEntryData(directoryEntryOffset, &entry))
	{
		unlock(&gFileSystemManager.mutex);
		return -1;
	}
	unlock(&gFileSystemManager.mutex);
	return 0;
}

DIR* openDirectory(const char* dirName)
{
	DIR* directory;
	DIRECTORY_ENTRY* directoryBuf;

	lock(&gFileSystemManager.mutex);

	directory = allocateFileDirectoryHandle();
	if (directory == NULL)
	{
		unlock(&gFileSystemManager.mutex);
		return NULL;
	}

	directoryBuf = (DIRECTORY_ENTRY*)allocateMemory(FILESYSTEM_CLUSTER_SIZE);
	if (directoryBuf == NULL)
	{
		freeFileDirectoryHandle(directory);
		unlock(&gFileSystemManager.mutex);
		return NULL;
	}

	if (!readCluster(0, (BYTE*)directoryBuf))
	{
		freeFileDirectoryHandle(directory);
		freeMemory(directoryBuf);
		unlock(&gFileSystemManager.mutex);
		return NULL;
	}

	directory->type = FILESYSTEM_TYPE_DIRECTORY;
	directory->dirHandle.currentOffset = 0;
	directory->dirHandle.directoryBuf = directoryBuf;
	unlock(&gFileSystemManager.mutex);
	return directory;
}

DIRECTORY_ENTRY* readDirectory(DIR* dir)
{
	DIRECTORY_ENTRY* entry;
	DIRECTORY_HANDLE* dirHandle;

	if (dir == NULL || dir->type != FILESYSTEM_TYPE_DIRECTORY)
		return NULL;

	dirHandle = &dir->dirHandle;
	if (dirHandle->currentOffset < 0 ||\
			dirHandle->currentOffset >= FILESYSTEM_MAX_DIRECTORY_ENTRY_COUNT)
		return NULL;

	lock(&gFileSystemManager.mutex);
	entry = dirHandle->directoryBuf;
	while (dirHandle->currentOffset < FILESYSTEM_MAX_DIRECTORY_ENTRY_COUNT)
	{
		if (entry[dirHandle->currentOffset].startClusterIdx != 0)
		{
			unlock(&gFileSystemManager.mutex);
			return &entry[dirHandle->currentOffset++];
		}
		dirHandle->currentOffset++;
	}
	unlock(&gFileSystemManager.mutex);
	return NULL;
}

void rewindDirectory(DIR* dir)
{
	if (dir == NULL || dir->type != FILESYSTEM_TYPE_DIRECTORY)
		return;
	lock(&gFileSystemManager.mutex);
	dir->dirHandle.currentOffset = 0;
	unlock(&gFileSystemManager.mutex);
}

int closeDirectory(DIR* dir)
{
	DIRECTORY_HANDLE* dirHandle;

	if (dir == NULL || dir->type != FILESYSTEM_TYPE_DIRECTORY)
		return -1;

	dirHandle = &dir->dirHandle;
	lock(&gFileSystemManager.mutex);
	freeMemory(dirHandle->directoryBuf);
	freeFileDirectoryHandle(dir);
	unlock(&gFileSystemManager.mutex);
	return 0;
}
