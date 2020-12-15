#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"

#define FILESYSTEM_SIGNATURE						0x7E38CF10
#define FILESYSTEM_SECTOR_PER_CLUSTER		8
#define FILESYSTEM_LAST_CLUSTER					0xFFFFFFFF
#define FILESYSTEM_FREE_CLUSTER					0x00

#define FILESYSTEM_MAX_DIRECTORY_ENTRY_COUNT ((FILESYSTEM_SECTOR_PER_CLUSTER * 512)\
		/ sizeof(DIRECTORY_ENTRY))
#define FILESYSTEM_CLUSTER_SIZE (FILESYSTEM_SECTOR_PER_CLUSTER * 512)
#define FILESYSTEM_MAX_FILENAME_LENGTH	24

typedef BOOL (*fReadHDDInformation)(BOOL isPrimary,\
		BOOL isMaster, HDDINFORMATION* information);
typedef int (*fReadHDDSector)(BOOL isPrimary, BOOL isMaster, DWORD LBA, int sectorCount, char* buf);
typedef int (*fWriteHDDSector)(BOOL isPrimary, BOOL isMaster, DWORD LBA, int sectorCount, char* buf);

#pragma pack(push, 1)

typedef struct PartitionStruct
{
	BYTE bootableFlag;
	BYTE startingCHSAddress[3];
	BYTE partiionType;
	BYTE endingCHSAddress[3];
	DWORD startingLBAAddress;
	DWORD sectorCount; // sector count in partition
} PARTITION;

typedef struct MBRStruct
{
	BYTE bootCode[430];
	DWORD signature;
	DWORD reservedSectorCount;
	DWORD clusterLinkSectorCount;
	DWORD totalClusterCount;
	PARTITION partition[4];
	BYTE bootLoaderSignature[2]; // 0x55, 0xAA of boot loader signature
} MBR;

typedef struct DirectoryEntryStruct{
	char fileName[FILESYSTEM_MAX_FILENAME_LENGTH];
	DWORD fileSize;
	DWORD startClusterIdx;
} DIRECTORY_ENTRY;

#pragma pack(pop)

typedef struct FileSystemManagerStruct
{
	BOOL isMounted;
	DWORD reservedSectorCount;
	DWORD clusterLinkAreaStartAddress;
	DWORD clusterLinkAreaSize;
	DWORD dataAreaStartAddress;
	DWORD totalClusterCount; // only data area
	DWORD lastAllocatedClusterLinkSectorOffset;
	MUTEX mutex;
} FILESYSTEM_MANAGER;

BOOL initFileSystem(void);
BOOL format(void);
BOOL mount(void);
BOOL getHDDInformation(HDDINFORMATION* information);
BOOL readClusterLinkTable(DWORD offset, BYTE* buf);
BOOL writeClusterLinkTable(DWORD offset, BYTE* buf);
BOOL readCluster(DWORD offset, BYTE* buf);
BOOL writeCluster(DWORD offset, BYTE* buf);
DWORD findFreeCluster(void);
BOOL setClusterLinkData(DWORD clusterIdx, DWORD data);
BOOL getClusterLinkData(DWORD clusterIdx, DWORD* data);
int findFreeDirectoryEntry(void);
BOOL setDirectoryEntryData(int idx, DIRECTORY_ENTRY* entry);
BOOL getDirectoryEntryData(int idx, DIRECTORY_ENTRY* entry);
int findDirectoryEntry(char* fileName, DIRECTORY_ENTRY* entry);
void getFileSystemInformation(FILESYSTEM_MANAGER* manager);

#endif
