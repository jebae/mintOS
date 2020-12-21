#ifndef __RAMDISK_H__
#define __RAMDISK_H__

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"

#define RDD_TOTAL_SECTOR_COUNT				(8 * 1024 * 1024 / 512)

#pragma pack(push, 1)

typedef struct RDDManagerStruct
{
	BYTE* buf;
	DWORD totalSectorCount;
	MUTEX mutex;
} RDD_MANAGER;

#pragma pack(pop)

BOOL initRDD(DWORD totalSectorCount);
BOOL readRDDInformation(BOOL isPrimary, BOOL isMaster, HDDINFORMATION* information);
int readRDDSector(BOOL isPrimary, BOOL isMaster, DWORD LBA, int sectorCount, char* buf);
int writeRDDSector(BOOL isPrimary, BOOL isMaster, DWORD LBA, int sectorCount, char* buf);

#endif
