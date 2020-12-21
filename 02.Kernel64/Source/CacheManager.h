#ifndef __CACHEMANAGER_H__
#define __CACHEMANAGER_H__

#include "Types.h"

#define CACHE_MAX_CLUSTER_LINKTABLE_AREA_COUNT		16
#define CACHE_MAX_DATA_AREA_COUNT									32
#define CACHE_INVALID_TAG													0xFFFFFFFF
#define CACHE_MAX_CACHE_TABLE_INDEX								2
#define CACHE_CLUSTER_LINKTABLE_AREA							0
#define CACHE_DATA_AREA														1

typedef struct CacheBufferStruct
{
	DWORD tag;
	DWORD accessTime;
	BOOL isChanged;
	BYTE* buf;
} CACHE_BUFFER;

typedef struct CacheManagerStruct
{
	DWORD accessTime[CACHE_MAX_CACHE_TABLE_INDEX];
	BYTE* buf[CACHE_MAX_CACHE_TABLE_INDEX];
	CACHE_BUFFER cacheBuf[CACHE_MAX_CACHE_TABLE_INDEX][CACHE_MAX_DATA_AREA_COUNT];
	DWORD maxCount[CACHE_MAX_CACHE_TABLE_INDEX];
} CACHE_MANAGER;

BOOL initCacheManager(void);
CACHE_BUFFER* allocateCacheBuffer(int cacheTableIdx);
CACHE_BUFFER* findCacheBuffer(int cacheTableIdx, DWORD tag);
CACHE_BUFFER* getVictimInCacheBuffer(int cacheTableIdx);
void discardAllCacheBuffer(int cacheTableIdx);
BOOL getCacheBufferAndCount(int cacheTableIdx,
		CACHE_BUFFER** cacheBuf, int* maxCount);
static void cutDownAccessTime(int cacheTableIdx);

#endif
