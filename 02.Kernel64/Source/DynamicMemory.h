#ifndef __DYNAMICMEMORY_H__
#define __DYNAMICMEMORY_H__

#include "Types.h"
#include "Task.h"

#define DYNAMIC_MEMORY_START_ADDRESS ((TASK_STACK_POOL_ADDRESS +\
		(TASK_STACK_SIZE * TASK_MAX_COUNT) + 0xfffff) + 0xfffffffffff00000)
#define DYNAMIC_MEMORY_MIN_SIZE			1024
#define DYNAMIC_MEMORY_EXIST				0x01
#define DYNAMIC_MEMORY_EMPTY				0x00

typedef struct BitmapStruct
{
	BYTE* bitmap;
	QWORD existBitCount;
} BITMAP;

typedef struct DynamicMemoryManagerStruct
{
	int maxLevelCount;
	int blockCountOfSmallestBlock;
	QWORD usedSize;
	QWORD startAddress;
	QWORD endAddress;
	BYTE* allocatedBlockListIdx;
	BITMAP* bitmapOfLevel;
} DYNAMICMEMORY;

void initDynamicMemory(void);
void* allocateMemory(QWORD size);
BOOL freeMemory(void* address);
void getDynamicMemoryInfo(QWORD* dynamicMemoryStartAddress,\
		QWORD* dynamicMemoryTotalSize, QWORD* metaDataSize, QWORD* usedMemorySize);
DYNAMICMEMORY* getDynamicMemoryManager(void);
static QWORD calculateDynamicMemorySize(void);
static int calculateMetaBlockCount(QWORD dynamicRAMSize);
static int allocateBuddyBlock(QWORD alignedSize);
static QWORD getBuddyBlockSize(QWORD size);
static int getBlockListIdxOfMatchSize(QWORD alignedSize);
static int findFreeBlockInBitmap(int blockListIdx);
static void setFlagInBitmap(int blockListIdx, int offset, BYTE flag);
static BOOL freeBuddyBlock(int blockListIdx, int offset);
static BYTE getFlagInBitmap(int blockListIdx, int offset);

#endif
