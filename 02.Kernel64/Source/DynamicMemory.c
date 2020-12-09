#include "DynamicMemory.h"
#include "Utility.h"
#include "Synchronization.h"

static DYNAMICMEMORY gDynamicMemory;

void initDynamicMemory(void)
{
	QWORD dynamicMemorySize;
	BYTE* currentBitmapPos;
	int metaBlockCount, blockCountOfLevel;
	int i, j;

	dynamicMemorySize = calculateDynamicMemorySize();
	metaBlockCount = calculateMetaBlockCount(dynamicMemorySize);
	gDynamicMemory.blockCountOfSmallestBlock = dynamicMemorySize / DYNAMIC_MEMORY_MIN_SIZE\
		- metaBlockCount;

	i = 0;
	while ((gDynamicMemory.blockCountOfSmallestBlock >> i) > 0)
		i++;
	gDynamicMemory.maxLevelCount = i;

	gDynamicMemory.allocatedBlockListIdx = (BYTE*)DYNAMIC_MEMORY_START_ADDRESS;
	for (i=0; i < gDynamicMemory.blockCountOfSmallestBlock; i++)
	{
		gDynamicMemory.allocatedBlockListIdx[i] = 0xFF;
	}

	gDynamicMemory.bitmapOfLevel = (BITMAP*)(DYNAMIC_MEMORY_START_ADDRESS +\
		sizeof(BYTE) * gDynamicMemory.blockCountOfSmallestBlock);
	currentBitmapPos = ((BYTE*)gDynamicMemory.bitmapOfLevel) +\
		sizeof(BITMAP) * gDynamicMemory.maxLevelCount;
	for (i=0; i < gDynamicMemory.maxLevelCount; i++)
	{
		gDynamicMemory.bitmapOfLevel[i].bitmap = currentBitmapPos;
		gDynamicMemory.bitmapOfLevel[i].existBitCount = 0;
		blockCountOfLevel = gDynamicMemory.blockCountOfSmallestBlock >> i;
		for (j=0; j < blockCountOfLevel / 8; j++)
		{
			*currentBitmapPos = 0x00;
			currentBitmapPos++;
		}
		j = blockCountOfLevel % 8;
		if (j != 0)
		{
			*currentBitmapPos = 0x00;
			if (j % 2 == 1)
			{
				*currentBitmapPos |= (DYNAMIC_MEMORY_EXIST << (j - 1));
				gDynamicMemory.bitmapOfLevel[i].existBitCount = 1;
			}
			currentBitmapPos++;
		}
	}

	gDynamicMemory.startAddress = DYNAMIC_MEMORY_START_ADDRESS +\
		(metaBlockCount * DYNAMIC_MEMORY_MIN_SIZE);
	gDynamicMemory.endAddress = DYNAMIC_MEMORY_START_ADDRESS + dynamicMemorySize;
	gDynamicMemory.usedSize = 0;
}

static QWORD calculateDynamicMemorySize(void)
{
	QWORD RAMSize = getTotalRAMSize(); // unit: MB

	RAMSize *= 1 << 20;
	if (RAMSize > (QWORD)3 * (1 << 30))
		RAMSize = (QWORD)3 * (1 << 30);
	return RAMSize - DYNAMIC_MEMORY_START_ADDRESS;
}

static int calculateMetaBlockCount(QWORD dynamicRAMSize)
{
	long blockCountOfSmallestBlock;
	QWORD sizeOfBlockListIdx;
	QWORD sizeOfBitmap;

	blockCountOfSmallestBlock = dynamicRAMSize / DYNAMIC_MEMORY_MIN_SIZE;
	sizeOfBlockListIdx = blockCountOfSmallestBlock * sizeof(BYTE);
	sizeOfBitmap = 0;
	while (blockCountOfSmallestBlock > 0)
	{
		sizeOfBitmap += sizeof(BITMAP);

		// with +7, assign one more byte if rest exist
		sizeOfBitmap += (blockCountOfSmallestBlock + 7) / 8;
		blockCountOfSmallestBlock >>= 1;
	}

	// with + (DYNAMIC_MEMORY_MIN_SIZE - 1), get enough block count
	return (sizeOfBlockListIdx + sizeOfBitmap + DYNAMIC_MEMORY_MIN_SIZE - 1) /
		DYNAMIC_MEMORY_MIN_SIZE;
}

void* allocateMemory(QWORD size)
{
	QWORD alignedSize, relativeAddress;
	int blockListIdx, offset;

	alignedSize = getBuddyBlockSize(size);
	if (alignedSize == 0)
		return NULL;
	if (gDynamicMemory.startAddress + gDynamicMemory.usedSize + alignedSize
			> gDynamicMemory.endAddress)
		return NULL;
	offset = allocateBuddyBlock(alignedSize);
	if (offset == -1)
		return NULL;
	relativeAddress = alignedSize * offset;
	blockListIdx = getBlockListIdxOfMatchSize(alignedSize);
	gDynamicMemory.allocatedBlockListIdx[relativeAddress / DYNAMIC_MEMORY_MIN_SIZE]\
		= (BYTE)blockListIdx;
	gDynamicMemory.usedSize += alignedSize;
	return (void*)(gDynamicMemory.startAddress + relativeAddress);
}

static QWORD getBuddyBlockSize(QWORD size)
{
	long i;

	for (i=0; i < gDynamicMemory.maxLevelCount; i++)
	{
		if (size <= (DYNAMIC_MEMORY_MIN_SIZE << i))
			return DYNAMIC_MEMORY_MIN_SIZE << i;
	}
	return 0;
}

static int allocateBuddyBlock(QWORD alignedSize)
{
	int blockListIdx, offset, i;
	BOOL prevInterruptFlag;

	blockListIdx = getBlockListIdxOfMatchSize(alignedSize);
	prevInterruptFlag = lockForSystemData();
	for (i=blockListIdx; i < gDynamicMemory.maxLevelCount; i++)
	{
		offset = findFreeBlockInBitmap(i);
		if (offset != -1)
			break;
	}
	if (offset == -1)
	{
		unlockForSystemData(prevInterruptFlag);
		return -1;
	}
	setFlagInBitmap(i, offset, DYNAMIC_MEMORY_EMPTY);
	i--;
	while (i >= blockListIdx)
	{
		setFlagInBitmap(i, offset * 2, DYNAMIC_MEMORY_EMPTY);
		setFlagInBitmap(i, offset * 2 + 1, DYNAMIC_MEMORY_EXIST);
		offset *= 2;
		i--;
	}
	unlockForSystemData(prevInterruptFlag);
	return offset;
}

static int getBlockListIdxOfMatchSize(QWORD alignedSize)
{
	int i;

	for (i=0; i < gDynamicMemory.maxLevelCount; i++)
	{
		if (alignedSize <= (DYNAMIC_MEMORY_MIN_SIZE << i))
			return i;
	}
	return -1;
}

static int findFreeBlockInBitmap(int blockListIdx)
{
	BYTE* bitmap;
	int i, maxCount;

	if (gDynamicMemory.bitmapOfLevel[blockListIdx].existBitCount == 0)
		return -1;
	bitmap = gDynamicMemory.bitmapOfLevel[blockListIdx].bitmap;
	maxCount = gDynamicMemory.blockCountOfSmallestBlock >> blockListIdx;
	for (i=0; i < maxCount;)
	{
		if (maxCount - i >= 64)
		{
			if (*(QWORD*)&bitmap[i / 8] == 0)
			{
				i += 64;
				continue;
			}
		}
		if ((bitmap[i / 8] & (DYNAMIC_MEMORY_EXIST << (i % 8))) != 0)
			return i;
		i++;
	}
	return -1;
}

static void setFlagInBitmap(int blockListIdx, int offset, BYTE flag)
{
	BYTE* bitmap;

	bitmap = gDynamicMemory.bitmapOfLevel[blockListIdx].bitmap;
	if (flag == DYNAMIC_MEMORY_EXIST)
	{
		if ((bitmap[offset / 8] & (0x01 << (offset % 8))) == 0)
			gDynamicMemory.bitmapOfLevel[blockListIdx].existBitCount++;
		bitmap[offset / 8] |= (0x01 << (offset % 8));
	}
	else
	{
		if ((bitmap[offset / 8] & (0x01 << (offset% 8))) != 0)
			gDynamicMemory.bitmapOfLevel[blockListIdx].existBitCount--;
		bitmap[offset / 8] &= ~(0x01 << (offset % 8));
	}
}

BOOL freeMemory(void* address)
{
	QWORD relativeAddress, size;
	int blockListIdx, offset;

	if (address == NULL)
		return FALSE;
	relativeAddress = (QWORD)address - gDynamicMemory.startAddress;
	if (gDynamicMemory.allocatedBlockListIdx[relativeAddress / DYNAMIC_MEMORY_MIN_SIZE] == 0xFF)
		return FALSE;
	blockListIdx = (int)gDynamicMemory.allocatedBlockListIdx[relativeAddress / DYNAMIC_MEMORY_MIN_SIZE];
	size = (DYNAMIC_MEMORY_MIN_SIZE << blockListIdx);
	offset = relativeAddress / size;
	if (freeBuddyBlock(blockListIdx, offset))
	{
		gDynamicMemory.usedSize -= size;
		return TRUE;
	}
	return FALSE;
}

static BOOL freeBuddyBlock(int blockListIdx, int offset)
{
	int i, buddyBlock;
	BOOL prevInterruptFlag;

	prevInterruptFlag = lockForSystemData();
	for (i=blockListIdx; i < gDynamicMemory.maxLevelCount; i++)
	{
		setFlagInBitmap(i, offset, DYNAMIC_MEMORY_EXIST);
		buddyBlock = (offset % 2) == 0
			? offset + 1
			: offset - 1;
		if (getFlagInBitmap(i, buddyBlock) == DYNAMIC_MEMORY_EMPTY)
			break;
		setFlagInBitmap(i, offset, DYNAMIC_MEMORY_EMPTY);
		setFlagInBitmap(i, buddyBlock, DYNAMIC_MEMORY_EMPTY);
		offset /= 2;
	}
	unlockForSystemData(prevInterruptFlag);
	return TRUE;
}

static BYTE getFlagInBitmap(int blockListIdx, int offset)
{
	BYTE* bitmap = gDynamicMemory.bitmapOfLevel[blockListIdx].bitmap;

	if ((bitmap[offset / 8] & (0x01 << (offset % 8))) == 0)
		return DYNAMIC_MEMORY_EMPTY;
	return DYNAMIC_MEMORY_EXIST;
}

void getDynamicMemoryInfo(QWORD* dynamicMemoryStartAddress,\
	QWORD* dynamicMemoryTotalSize, QWORD* metaDataSize, QWORD* usedMemorySize)
{
	*dynamicMemoryStartAddress = gDynamicMemory.startAddress;
	*dynamicMemoryTotalSize = calculateDynamicMemorySize();
	*metaDataSize = calculateMetaBlockCount(*dynamicMemoryTotalSize) * DYNAMIC_MEMORY_MIN_SIZE;
	*usedMemorySize = gDynamicMemory.usedSize;
}

DYNAMICMEMORY* getDynamicMemoryManager(void)
{
	return &gDynamicMemory;
}
