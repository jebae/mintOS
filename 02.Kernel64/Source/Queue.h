#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "Types.h"

#pragma pack(push, 1)

typedef struct queueManagerStruct
{
	int dataSize;
	int maxDataCount;
	void* arr;
	int putIdx;
	int getIdx;
	BOOL lastOperationPut;
} QUEUE;

#pragma pack(pop)

void initQueue(QUEUE* q, void* queueBuffer, int maxDataCount, int dataSize);
BOOL isQueueFull(const QUEUE* q);
BOOL isQueueEmpty(const QUEUE* q);
BOOL putQueue(QUEUE* q, const void* data);
BOOL getQueue(QUEUE* q, void* data);

#endif
