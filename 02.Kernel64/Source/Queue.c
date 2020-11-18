#include "Queue.h"
#include "Utility.h"

void initQueue(QUEUE* q, void* queueBuffer, int maxDataCount, int dataSize)
{
	q->maxDataCount = maxDataCount;
	q->dataSize = dataSize;
	q->arr = queueBuffer;

	q->putIdx = 0;
	q->getIdx = 0;
	q->lastOperationPut = FALSE;
}

BOOL isQueueFull(const QUEUE* q)
{
	return q->getIdx == q->putIdx && q->lastOperationPut;
}

BOOL isQueueEmpty(const QUEUE* q)
{
	return q->getIdx == q->putIdx && !(q->lastOperationPut);
}

BOOL putQueue(QUEUE* q, const void* data)
{
	if (isQueueFull(q))
		return FALSE;
	memCpy((char*)(q->arr) + q->putIdx * q->dataSize, data, q->dataSize);
	q->putIdx = (q->putIdx + 1) % q->maxDataCount;
	q->lastOperationPut = TRUE;
	return TRUE;
}

BOOL getQueue(QUEUE* q, void* data)
{
	if (isQueueEmpty(q))
		return FALSE;
	memCpy(data, (char*)(q->arr) + q->getIdx * q->dataSize, q->dataSize);
	q->getIdx = (q->getIdx + 1) % q->maxDataCount;
	q->lastOperationPut = FALSE;
	return TRUE;
}
