#ifndef __SYNCHRONIZATION_H__
#define __SYNCHRONIZATION_H__

#include "Types.h"

#pragma pack(push, 1)

typedef struct MutexStruct
{
	volatile QWORD taskId;
	volatile DWORD lockCount;
	volatile BOOL isLocked;
	BYTE padding[3];
} MUTEX;

BOOL lockForSystemData(void);
void unlockForSystemData(BOOL interruptFlag);
void initMutex(MUTEX* mutex);
void lock(MUTEX* mutex);
void unlock(MUTEX* mutex);

#pragma pack(pop)

#endif
