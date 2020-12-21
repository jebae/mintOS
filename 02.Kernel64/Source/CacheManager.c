#include "CacheManager.h"
#include "FileSystem.h"
#include "DynamicMemory.h"
#include "Console.h"

static CACHE_MANAGER gCacheManager;

BOOL initCacheManager(void)
{
	int i;

	memset(&gCacheManager, 0, sizeof(gCacheManager));
	gCacheManager.accessTime[CACHE_CLUSTER_LINKTABLE_AREA] = 0;
	gCacheManager.accessTime[CACHE_DATA_AREA] = 0;

	gCacheManager.maxCount[CACHE_CLUSTER_LINKTABLE_AREA] = CACHE_MAX_CLUSTER_LINKTABLE_AREA_COUNT;
	gCacheManager.maxCount[CACHE_DATA_AREA] = CACHE_MAX_DATA_AREA_COUNT;

	// one link table is 512 (one sector)
	gCacheManager.buf[CACHE_CLUSTER_LINKTABLE_AREA] =
		(BYTE*)allocateMemory(CACHE_MAX_CLUSTER_LINKTABLE_AREA_COUNT * 512);
	if (gCacheManager.buf[CACHE_CLUSTER_LINKTABLE_AREA] == NULL)
		return FALSE;
	for (i=0; i < CACHE_MAX_CLUSTER_LINKTABLE_AREA_COUNT; i++)
	{
		gCacheManager.cacheBuf[CACHE_CLUSTER_LINKTABLE_AREA][i].buf =
			&gCacheManager.buf[CACHE_CLUSTER_LINKTABLE_AREA][i * 512];
		gCacheManager.cacheBuf[CACHE_CLUSTER_LINKTABLE_AREA][i].tag = CACHE_INVALID_TAG;
	}

	gCacheManager.buf[CACHE_DATA_AREA] =
		(BYTE*)allocateMemory(CACHE_MAX_DATA_AREA_COUNT * FILESYSTEM_CLUSTER_SIZE);
	if (gCacheManager.buf[CACHE_DATA_AREA] == NULL)
	{
		freeMemory(gCacheManager.buf[CACHE_CLUSTER_LINKTABLE_AREA]);
		return FALSE;
	}
	for (i=0; i < CACHE_MAX_DATA_AREA_COUNT; i++)
	{
		gCacheManager.cacheBuf[CACHE_DATA_AREA][i].buf =
			&gCacheManager.buf[CACHE_DATA_AREA][i * FILESYSTEM_CLUSTER_SIZE];
		gCacheManager.cacheBuf[CACHE_DATA_AREA][i].tag = CACHE_INVALID_TAG;
	}
	return TRUE;
}

CACHE_BUFFER* allocateCacheBuffer(int cacheTableIdx)
{
	CACHE_BUFFER* cacheBuf;
	int i;

	if (cacheTableIdx > CACHE_MAX_CACHE_TABLE_INDEX)
		return FALSE;

	cutDownAccessTime(cacheTableIdx);

	cacheBuf = gCacheManager.cacheBuf[cacheTableIdx];
	for (i=0; i < gCacheManager.maxCount[cacheTableIdx]; i++)
	{
		if (cacheBuf[i].tag == CACHE_INVALID_TAG)
		{
			cacheBuf[i].tag = CACHE_INVALID_TAG - 1;
			cacheBuf[i].accessTime = gCacheManager.accessTime[cacheTableIdx]++;
			return &cacheBuf[i];
		}
	}
	return NULL;
}

CACHE_BUFFER* findCacheBuffer(int cacheTableIdx, DWORD tag)
{
	CACHE_BUFFER* cacheBuf;
	int i;

	if (cacheTableIdx > CACHE_MAX_CACHE_TABLE_INDEX)
		return FALSE;

	cutDownAccessTime(cacheTableIdx);

	cacheBuf = gCacheManager.cacheBuf[cacheTableIdx];
	for (i=0; i < gCacheManager.maxCount[cacheTableIdx]; i++)
	{
		if (cacheBuf[i].tag == tag)
		{
			cacheBuf[i].accessTime = gCacheManager.accessTime[cacheTableIdx]++;
			return &cacheBuf[i];
		}
	}
	return NULL;
}

static void cutDownAccessTime(int cacheTableIdx)
{
	CACHE_BUFFER temp;
	CACHE_BUFFER* cacheBuf;
	int i, j;
	BOOL isSorted;

	if (cacheTableIdx > CACHE_MAX_CACHE_TABLE_INDEX)
		return;

	if (gCacheManager.accessTime[cacheTableIdx] < 0xFFFFFFFE)
		return;

	cacheBuf = gCacheManager.cacheBuf[cacheTableIdx];
	for (i=0; i < gCacheManager.maxCount[cacheTableIdx] - 1; i++)
	{
		isSorted = TRUE;
		for (j=0; i < gCacheManager.maxCount[cacheTableIdx] - 1 - i; j++)
		{
			if (cacheBuf[j].accessTime > cacheBuf[j + 1].accessTime)
			{
				temp = cacheBuf[j];
				cacheBuf[j] = cacheBuf[j + 1];
				cacheBuf[j + 1] = temp;
				isSorted = FALSE;
			}
		}
		if (isSorted)
			break;
	}

	for (i=0; i < gCacheManager.maxCount[cacheTableIdx]; i++)
	{
		gCacheManager.cacheBuf[cacheTableIdx][i].accessTime = i;
	}
	gCacheManager.accessTime[cacheTableIdx] = i;
}

CACHE_BUFFER* getVictimInCacheBuffer(int cacheTableIdx)
{
	CACHE_BUFFER* cacheBuf;
	DWORD oldTime;
	int oldIdx;
	int i;

	if (cacheTableIdx > CACHE_MAX_CACHE_TABLE_INDEX)
		return NULL;

	cacheBuf = gCacheManager.cacheBuf[cacheTableIdx];
	oldIdx = -1;
	oldTime = 0xFFFFFFFF;
	for (i=0; i < gCacheManager.maxCount[cacheTableIdx]; i++)
	{
		if (cacheBuf[i].tag == CACHE_INVALID_TAG)
		{
			oldIdx = i;
			break;
		}
		if (cacheBuf[i].accessTime < oldTime)
		{
			oldTime = cacheBuf[i].accessTime;
			oldIdx = i;
		}
	}

	if (oldIdx == -1)
	{
		printf("getVictimInCacheBuffer: Cache buffer error\n");
		return NULL;
	}

	cacheBuf[oldIdx].accessTime = gCacheManager.accessTime[cacheTableIdx]++;
	return &cacheBuf[oldIdx];
}

void discardAllCacheBuffer(int cacheTableIdx)
{
	CACHE_BUFFER* cacheBuf;
	int i;

	if (cacheTableIdx > CACHE_MAX_CACHE_TABLE_INDEX)
		return;

	cacheBuf = gCacheManager.cacheBuf[cacheTableIdx];
	for (i=0; i < gCacheManager.maxCount[cacheTableIdx]; i++)
	{
		cacheBuf[i].tag = CACHE_INVALID_TAG;
	}
	gCacheManager.accessTime[cacheTableIdx] = 0;
}

BOOL getCacheBufferAndCount(int cacheTableIdx,
		CACHE_BUFFER** cacheBuf, int* maxCount)
{
	if (cacheTableIdx > CACHE_MAX_CACHE_TABLE_INDEX)
		return FALSE;

	*cacheBuf = gCacheManager.cacheBuf[cacheTableIdx];
	*maxCount = gCacheManager.maxCount[cacheTableIdx];
	return TRUE;
}
