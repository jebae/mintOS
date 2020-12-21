#include "RAMDisk.h"
#include "Utility.h"
#include "DynamicMemory.h"

static RDD_MANAGER gRDDManager;

BOOL initRDD(DWORD totalSectorCount)
{
	memset(&gRDDManager, 0, sizeof(gRDDManager));
	gRDDManager.buf = (BYTE*)allocateMemory(totalSectorCount * 512);
	if (gRDDManager.buf == NULL)
		return FALSE;
	gRDDManager.totalSectorCount = totalSectorCount;
	initMutex(&gRDDManager.mutex);
	return TRUE;
}

BOOL readRDDInformation(BOOL isPrimary, BOOL isMaster, HDDINFORMATION* information)
{
	memset(information, 0, sizeof(HDDINFORMATION));
	information->totalSectors = gRDDManager.totalSectorCount;
	memcpy(information->serialNumber, "0000-0000", 9);
	memcpy(information->modelNumber, "mintOS RAM Disk v1.0", 20);
	return TRUE;
}

int readRDDSector(BOOL isPrimary, BOOL isMaster, DWORD LBA, int sectorCount, char* buf)
{
	int realReadCount;

	realReadCount = MIN(gRDDManager.totalSectorCount - LBA, sectorCount);
	memcpy(buf, gRDDManager.buf + LBA * 512, realReadCount * 512);
	return realReadCount;
}

int writeRDDSector(BOOL isPrimary, BOOL isMaster, DWORD LBA, int sectorCount, char* buf)
{
	int realWriteCount;

	realWriteCount = MIN(gRDDManager.totalSectorCount - LBA, sectorCount);
	memcpy(gRDDManager.buf + LBA * 512, buf, realWriteCount * 512);
	return realWriteCount;
}
