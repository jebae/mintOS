#include "Synchronization.h"
#include "Utility.h"
#include "Task.h"
#include "AssemblyUtility.h"

BOOL lockForSystemData(void)
{
	return setInterruptFlag(FALSE);
}

void unlockForSystemData(BOOL interruptFlag)
{
	setInterruptFlag(interruptFlag);
}

void initMutex(MUTEX* mutex)
{
	mutex->isLocked = FALSE;
	mutex->lockCount = 0;
	mutex->taskId = TASK_INVALID_ID;
}

void lock(MUTEX* mutex)
{
	/*
	 * check if mutex is locked
	 * checkLockAndSet compares first and second parameter,
	 * if same, set first parameter as third parameter
	 */
	if (checkLockAndSet(&mutex->isLocked, 0, 1) == FALSE)
	{
		if (mutex->taskId == getRunningTask()->link.id)
		{
			mutex->lockCount++;
			return;
		}
		while (checkLockAndSet(&mutex->isLocked, 0, 1) == FALSE)
			schedule();
	}
	mutex->taskId = getRunningTask()->link.id;
	mutex->lockCount = 1;
}

void unlock(MUTEX* mutex)
{
	if (!mutex->isLocked || getRunningTask()->link.id != mutex->taskId)
		return;
	mutex->lockCount--;
	if (mutex->lockCount == 0)
	{
		mutex->isLocked = FALSE;
		mutex->taskId = TASK_INVALID_ID;
	}
}
