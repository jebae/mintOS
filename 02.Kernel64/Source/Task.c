#include "Task.h"
#include "Descriptor.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "Console.h"
#include "Synchronization.h"

static SCHEDULER gScheduler;
static TCB_POOL_MANAGER gTCBPoolManager;

static void initTCBPool(void)
{
	int i;

	memset(&gTCBPoolManager, 0, sizeof(gTCBPoolManager));
	gTCBPoolManager.TCBPool = (TCB*)TASK_TCB_POOL_ADDRESS;
	memset((void*)TASK_TCB_POOL_ADDRESS, 0, sizeof(TCB) * TASK_MAX_COUNT);

	for (i=0; i < TASK_MAX_COUNT; i++)
	{
		gTCBPoolManager.TCBPool[i].link.id = i;
	}
	gTCBPoolManager.maxCount = TASK_MAX_COUNT;
	gTCBPoolManager.allocatedCount = 1;
}

static TCB* allocateTCB(void)
{
	int i;
	TCB* emptyTCB;

	if (gTCBPoolManager.useCount == gTCBPoolManager.maxCount)
		return NULL;
	for (i=0; i < gTCBPoolManager.maxCount; i++)
	{
		if ((gTCBPoolManager.TCBPool[i].link.id >> 32) == 0)
		{
			emptyTCB = &gTCBPoolManager.TCBPool[i];
			break;
		}
	}
	emptyTCB->link.id = ((QWORD)gTCBPoolManager.allocatedCount << 32) | i;
	gTCBPoolManager.useCount++;
	gTCBPoolManager.allocatedCount++;
	if (gTCBPoolManager.allocatedCount == 0)
		gTCBPoolManager.allocatedCount = 1;
	return emptyTCB;
}

static void freeTCB(QWORD id)
{
	int i = GET_TCB_OFFSET(id);

	memset(&gTCBPoolManager.TCBPool[i], 0, sizeof(TCB));
	gTCBPoolManager.TCBPool[i].link.id = i;
	gTCBPoolManager.useCount--;
}

TCB* createTask(QWORD flags, QWORD entryPointAddress)
{
	TCB* task;
	void* stackAddress;
	BOOL prevInterruptFlag;

	prevInterruptFlag = lockForSystemData();
	task = allocateTCB();
	if (task == NULL)
	{
		unlockForSystemData(prevInterruptFlag);
		return NULL;
	}
	unlockForSystemData(prevInterruptFlag);

	stackAddress = (void*)(TASK_STACK_POOL_ADDRESS +\
		TASK_STACK_SIZE * GET_TCB_OFFSET(task->link.id));
	setupTask(task, flags, entryPointAddress, stackAddress, TASK_STACK_SIZE);

	prevInterruptFlag = lockForSystemData();
	addTaskToReadyList(task);
	unlockForSystemData(prevInterruptFlag);
	return task;
}

static void setupTask(TCB* tcb, QWORD flags, QWORD entryPointAddress,\
	void* stackAddress, QWORD stackSize)
{
	memset(tcb->context.registers, 0, sizeof(tcb->context.registers));

	tcb->context.registers[TASK_RSP_OFFSET] = (QWORD)stackAddress + stackSize;
	tcb->context.registers[TASK_RBP_OFFSET] = (QWORD)stackAddress + stackSize;

	tcb->context.registers[TASK_CS_OFFSET] = GDT_KERNEL_CODE_SEGMENT;
	tcb->context.registers[TASK_DS_OFFSET] = GDT_KERNEL_DATA_SEGMENT;
	tcb->context.registers[TASK_ES_OFFSET] = GDT_KERNEL_DATA_SEGMENT;
	tcb->context.registers[TASK_FS_OFFSET] = GDT_KERNEL_DATA_SEGMENT;
	tcb->context.registers[TASK_GS_OFFSET] = GDT_KERNEL_DATA_SEGMENT;
	tcb->context.registers[TASK_SS_OFFSET] = GDT_KERNEL_DATA_SEGMENT;

	tcb->context.registers[TASK_RIP_OFFSET] = entryPointAddress;

	// activate interrupt for task, setting IF bit(9) 1
	tcb->context.registers[TASK_RFLAGS_OFFSET] = 0x0200;

	tcb->stackAddress = stackAddress;
	tcb->stackSize = stackSize;
	tcb->flags = flags;
}

void initScheduler(void)
{
	int i;

	initTCBPool();
	for (i=0; i < TASK_MAX_READY_LIST_COUNT; i++)
	{
		initList(&gScheduler.readyList[i]);
		gScheduler.executeCount[i] = 0;
	}
	initList(&gScheduler.waitList);
	gScheduler.runningTask = allocateTCB();
	gScheduler.runningTask->flags = TASK_FLAGS_HIGHEST;

	gScheduler.processorLoad = 0;
	gScheduler.processorTimeSpentByIdleTask = 0;
}

void setRunningTask(TCB* task)
{
	BOOL prevInterruptFlag;

	prevInterruptFlag = lockForSystemData();
	gScheduler.runningTask = task;
	unlockForSystemData(prevInterruptFlag);
}

TCB* getRunningTask(void)
{
	BOOL prevInterruptFlag;
	TCB* runningTask;

	prevInterruptFlag = lockForSystemData();
	runningTask = gScheduler.runningTask;
	unlockForSystemData(prevInterruptFlag);
	return runningTask;
}

static TCB* getNextTaskToRun(void)
{
	int i, j;
	TCB* nextTask = NULL;

	for (j=0; j < 2; j++)
	{
		for (i=0; i < TASK_MAX_READY_LIST_COUNT; i++)
		{
			if (gScheduler.executeCount[i] < getListCount(&gScheduler.readyList[i]))
			{
				nextTask = (TCB*)removeListFromHeader(&gScheduler.readyList[i]);
				gScheduler.executeCount[i]++;
				break;
			}
			else
			{
				gScheduler.executeCount[i] = 0;
			}
		}
		if (nextTask != NULL)
			break;
	}
	return nextTask;
}

static BOOL addTaskToReadyList(TCB* task)
{
	BYTE priority = GET_PRIORITY(task->flags);

	if (priority >= TASK_MAX_READY_LIST_COUNT)
		return FALSE;
	addListToTail(&gScheduler.readyList[priority], task);
	return TRUE;
}

static TCB* removeTaskFromReadyList(QWORD id)
{
	TCB* target;
	BYTE priority;

	if (GET_TCB_OFFSET(id) >= TASK_MAX_COUNT)
		return NULL;
	target = getTCBInTCBPool(GET_TCB_OFFSET(id));
	if (target->link.id != id)
		return NULL;
	priority = GET_PRIORITY(target->flags);
	removeList(&gScheduler.readyList[priority], id);
	return target;
}

BOOL changePriority(QWORD id, BYTE priority)
{
	TCB* target;
	BOOL prevInterruptFlag;

	if (priority >= TASK_MAX_READY_LIST_COUNT)
		return FALSE;

	prevInterruptFlag = lockForSystemData();
	target = gScheduler.runningTask;
	if (target->link.id == id)
	{
		SET_PRIORITY(target->flags, priority);
	}
	else
	{
		target = removeTaskFromReadyList(id);
		if (target == NULL)
		{
			target = getTCBInTCBPool(GET_TCB_OFFSET(id));
			if (target != NULL)
				SET_PRIORITY(target->flags, priority);
		}
		else
		{
			SET_PRIORITY(target->flags, priority);
			addTaskToReadyList(target);
		}
	}
	unlockForSystemData(prevInterruptFlag);
	return TRUE;
}

void schedule(void)
{
	TCB* runningTask;
	TCB* nextTask;
	BOOL prevInterruptFlag;

	if (getReadyTaskCount() < 1)
		return;
	prevInterruptFlag = lockForSystemData();
	nextTask = getNextTaskToRun();
	if (nextTask == NULL)
	{
		unlockForSystemData(prevInterruptFlag);
		return;
	}
	runningTask = getRunningTask();
	gScheduler.runningTask = nextTask;
	if ((runningTask->flags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE)
	{
		gScheduler.processorTimeSpentByIdleTask +=\
			TASK_PROCESSOR_TIME - gScheduler.processorTime;
	}
	if ((runningTask->flags & TASK_FLAGS_ENDTASK) == TASK_FLAGS_ENDTASK)
	{
		addListToTail(&gScheduler.waitList, runningTask);
		switchContext(NULL, &nextTask->context);
	}
	else
	{
		addTaskToReadyList(runningTask);
		switchContext(&runningTask->context, &nextTask->context);
	}
	gScheduler.processorTime = TASK_PROCESSOR_TIME;
	unlockForSystemData(prevInterruptFlag);
}

BOOL scheduleInInterrupt(void)
{
	TCB* runningTask;
	TCB* nextTask;
	char* context = (char*)IST_START_ADDRESS + IST_SIZE - sizeof(CONTEXT);
	BOOL prevInterruptFlag;

	if (getReadyTaskCount() < 1)
		return FALSE;
	prevInterruptFlag = lockForSystemData();
	nextTask = getNextTaskToRun();
	if (nextTask == NULL)
	{
		unlockForSystemData(prevInterruptFlag);
		return FALSE;
	}
	runningTask = getRunningTask();
	gScheduler.runningTask = nextTask;

	if ((runningTask->flags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE)
		gScheduler.processorTimeSpentByIdleTask += TASK_PROCESSOR_TIME;
	gScheduler.processorTime = TASK_PROCESSOR_TIME;

	if ((runningTask->flags & TASK_FLAGS_ENDTASK) == TASK_FLAGS_ENDTASK)
	{
		addListToTail(&gScheduler.waitList, runningTask);
	}
	else
	{
		/*
		 * copy context from IST to runningTask context
		 * cause ISR already did SAVE_CONTEXT to IST
		 */
		memcpy(&runningTask->context, context, sizeof(CONTEXT));
		addTaskToReadyList(runningTask);
	}

	unlockForSystemData(prevInterruptFlag);
	/*
	 * ISR will LOAD_CONTEXT, so before do that, save context to IST
	 */
	memcpy(context, &nextTask->context, sizeof(CONTEXT));
	return TRUE;
}

BOOL endTask(QWORD id)
{
	TCB* target;
	BOOL prevInterruptFlag;

	if (GET_TCB_OFFSET(id) >= TASK_MAX_COUNT)
		return FALSE;
	prevInterruptFlag = lockForSystemData();
	target = gScheduler.runningTask;
	if (target->link.id == id)
	{
		target->flags |= TASK_FLAGS_ENDTASK;
		SET_PRIORITY(target->flags, TASK_FLAGS_WAIT);
		unlockForSystemData(prevInterruptFlag);
		schedule();

		// never able to to run below code
		while (1);
	}
	else
	{
		target = removeTaskFromReadyList(id);
		if (target == NULL)
		{
			target = getTCBInTCBPool(GET_TCB_OFFSET(target->link.id));
			if (target != NULL)
			{
				target->flags |= TASK_FLAGS_ENDTASK;
				SET_PRIORITY(target->flags, TASK_FLAGS_WAIT);
			}
			unlockForSystemData(prevInterruptFlag);
			return TRUE;
		}
		target->flags |= TASK_FLAGS_ENDTASK;
		SET_PRIORITY(target->flags, TASK_FLAGS_WAIT);
		addListToTail(&gScheduler.waitList, target);
	}
	unlockForSystemData(prevInterruptFlag);
	return TRUE;
}

void exitTask(void)
{
	endTask(gScheduler.runningTask->link.id);
}

int getReadyTaskCount(void)
{
	int count = 0;
	int i;
	BOOL prevInterruptFlag;

	prevInterruptFlag = lockForSystemData();
	for (i=0; i < TASK_MAX_READY_LIST_COUNT; i++)
		count += getListCount(&gScheduler.readyList[i]);
	unlockForSystemData(prevInterruptFlag);
	return count;
}

int getTaskCount(void)
{
	int count = getReadyTaskCount();
	BOOL prevInterruptFlag;

	prevInterruptFlag = lockForSystemData();
	count += getListCount(&gScheduler.waitList);
	if (gScheduler.runningTask != NULL)
		count++;
	unlockForSystemData(prevInterruptFlag);
	return count;
}

TCB* getTCBInTCBPool(int offset)
{
	if (offset < 0 || offset >= TASK_MAX_COUNT)
		return NULL;
	return &gTCBPoolManager.TCBPool[offset];
}

BOOL isTaskExist(QWORD id)
{
	TCB* task = getTCBInTCBPool(GET_TCB_OFFSET(id));

	if (task == NULL || task->link.id != id)
		return FALSE;
	return TRUE;
}

QWORD getProcessorLoad(void)
{
	return gScheduler.processorLoad;
}

void idleTask(void)
{
	TCB* task;
	QWORD lastTotalTick, lastIdleTick;
	QWORD currentTotalTick, currentIdleTick;
	BOOL prevInterruptFlag;

	lastTotalTick = getTickCount();
	lastIdleTick = gScheduler.processorTimeSpentByIdleTask;
	while (1)
	{
		currentTotalTick = getTickCount();
		currentIdleTick = gScheduler.processorTimeSpentByIdleTask;
		if (currentTotalTick == lastTotalTick)
		{
			gScheduler.processorLoad = 0;
		}
		else
		{
			gScheduler.processorLoad = 100 -\
				((currentIdleTick - lastIdleTick) * 100 / (currentTotalTick - lastTotalTick));
		}
		lastTotalTick = currentTotalTick;
		lastIdleTick = currentIdleTick;
		haltProcessorByLoad();
		while (getListCount(&gScheduler.waitList) > 0)
		{
			prevInterruptFlag = lockForSystemData();
			task = (TCB*)removeListFromHeader(&gScheduler.waitList);
			if (task == NULL)
			{
				unlockForSystemData(prevInterruptFlag);
				break;
			}
			freeTCB(task->link.id);
			unlockForSystemData(prevInterruptFlag);
			printf("IDLE: TCB ID[0x%q] end\n", task->link.id);
		}
		schedule();
	}
}

void haltProcessorByLoad(void)
{
	if (gScheduler.processorLoad < 40)
		hlt();
	if (gScheduler.processorLoad < 80)
		hlt();
	if (gScheduler.processorLoad < 95)
		hlt();
}

void decreaseProcessorTime(void)
{
	if (gScheduler.processorTime > 0)
		gScheduler.processorTime--;
}

BOOL isProcessorTimeExpired(void)
{
	return gScheduler.processorTime <= 0;
}

