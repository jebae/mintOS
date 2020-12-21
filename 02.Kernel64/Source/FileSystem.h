#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"
#include "CacheManager.h"

#define FILESYSTEM_SIGNATURE						0x7E38CF10
#define FILESYSTEM_SECTOR_PER_CLUSTER		8
#define FILESYSTEM_LAST_CLUSTER					0xFFFFFFFF
#define FILESYSTEM_FREE_CLUSTER					0x00

#define FILESYSTEM_MAX_DIRECTORY_ENTRY_COUNT ((FILESYSTEM_SECTOR_PER_CLUSTER * 512)\
		/ sizeof(DIRECTORY_ENTRY))
#define FILESYSTEM_CLUSTER_SIZE (FILESYSTEM_SECTOR_PER_CLUSTER * 512)
#define FILESYSTEM_HANDLE_MAX_COUNT			(TASK_MAX_COUNT * 3)
#define FILESYSTEM_MAX_FILENAME_LENGTH	24

#define FILESYSTEM_TYPE_FREE						0
#define FILESYSTEM_TYPE_FILE						1
#define FILESYSTEM_TYPE_DIRECTORY				2

#define FILESYSTEM_SEEK_SET							0
#define FILESYSTEM_SEEK_CUR							1
#define FILESYSTEM_SEEK_END							2

typedef BOOL (*fReadHDDInformation)(BOOL isPrimary,\
		BOOL isMaster, HDDINFORMATION* information);
typedef int (*fReadHDDSector)(BOOL isPrimary, BOOL isMaster, DWORD LBA, int sectorCount, char* buf);
typedef int (*fWriteHDDSector)(BOOL isPrimary, BOOL isMaster, DWORD LBA, int sectorCount, char* buf);

#define fopen														openFile
#define fread														readFile
#define fwrite													writeFile
#define fseek														seekFile
#define fclose													closeFile
#define remove													removeFile
#define opendir													openDirectory
#define readdif													readDirectory
#define rewinddir												rewindDirectory
#define closedir												closeDirectory

#define SEEK_SET												FILESYSTEM_SEEK_SET
#define SEEK_CUR												FILESYSTEM_SEEK_CUR
#define SEEK_END												FILESYSTEM_SEEK_END

#define size_t													DWORD
#define dirent													DirectoryEntryStruct
#define d_name													fileName

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

typedef struct DirectoryEntryStruct
{
	char fileName[FILESYSTEM_MAX_FILENAME_LENGTH];
	DWORD fileSize;
	DWORD startClusterIdx;
} DIRECTORY_ENTRY;

typedef struct FileHandleStruct
{
	int directoryEntryOffset;
	DWORD fileSize;
	DWORD startClusterIdx;
	DWORD currentClusterIdx;
	DWORD prevClusterIdx;
	DWORD currentOffset;
} FILE_HANDLE;

typedef struct DirectoryHandleStruct
{
	DIRECTORY_ENTRY* directoryBuf;
	int currentOffset;
} DIRECTORY_HANDLE;

typedef struct FileDirectoryHandleStruct
{
	BYTE type;
	union
	{
		FILE_HANDLE fileHandle;
		DIRECTORY_HANDLE dirHandle;
	};
} FILE, DIR;

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
	FILE* handlePool;
	BOOL cacheEnabled;
} FILESYSTEM_MANAGER;

BOOL initFileSystem(void);
BOOL format(void);
BOOL mount(void);
BOOL getHDDInformation(HDDINFORMATION* information);
static BOOL readClusterLinkTable(DWORD offset, BYTE* buf);
static BOOL writeClusterLinkTable(DWORD offset, BYTE* buf);
static BOOL readCluster(DWORD offset, BYTE* buf);
static BOOL writeCluster(DWORD offset, BYTE* buf);
static DWORD findFreeCluster(void);
static BOOL setClusterLinkData(DWORD clusterIdx, DWORD data);
static BOOL getClusterLinkData(DWORD clusterIdx, DWORD* data);
static int findFreeDirectoryEntry(void);
static BOOL setDirectoryEntryData(int idx, DIRECTORY_ENTRY* entry);
static BOOL getDirectoryEntryData(int idx, DIRECTORY_ENTRY* entry);
static int findDirectoryEntry(const char* fileName, DIRECTORY_ENTRY* entry);
void getFileSystemInformation(FILESYSTEM_MANAGER* manager);

FILE* openFile(const char* fileName, const char* mode);
DWORD readFile(void* buf, DWORD size, DWORD count, FILE* file);
DWORD writeFile(const void* buf, DWORD size, DWORD count, FILE* file);
int seekFile(FILE* file, int offset, int origin);
int closeFile(FILE* file);
int removeFile(const char* fileName);
DIR* openDirectory(const char* dirName);
DIRECTORY_ENTRY* readDirectory(DIR* dir);
void rewindDirectory(DIR* dir);
int closeDirectory(DIR* dir);
BOOL writeZero(FILE* file, DWORD count);
BOOL isFileOpened(const DIRECTORY_ENTRY* entry);
static void* allocateFileDirectoryHandle(void);
static void freeFileDirectoryHandle(FILE* file);
static BOOL createFile(const char* fileName, DIRECTORY_ENTRY* entry,\
		int* directoryEntryIdx);
static BOOL freeClusterUntilEnd(DWORD clusterIdx);
static BOOL updateDirectoryEntry(FILE_HANDLE* fileHandle);

static BOOL readClusterLinkTableWithoutCache(DWORD offset, BYTE* buf);
static BOOL readClusterLinkTableWithCache(DWORD offset, BYTE* buf);
static BOOL writeClusterLinkTableWithoutCache(DWORD offset, BYTE* buf);
static BOOL writeClusterLinkTableWithCache(DWORD offset, BYTE* buf);

static BOOL readClusterWithoutCache(DWORD offset, BYTE* buf);
static BOOL readClusterWithCache(DWORD offset, BYTE* buf);
static BOOL writeClusterWithoutCache(DWORD offset, BYTE* buf);
static BOOL writeClusterWithCache(DWORD offset, BYTE* buf);

static CACHE_BUFFER* allocateCacheBufferWithFlush(int cacheTableIdx);
BOOL flushFileSystemCache(void);

#endif
