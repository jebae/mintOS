#include "Task.h"
#include "Descriptor.h"
#include "Utility.h"
#include "AssemblyUtility.h"

static SCHEDULER gScheduler;
static TCB_POOL_MANAGER gTCBPoolManager;

void initTCBPool(void)
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

TCB* allocateTCB(void)
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

void freeTCB(QWORD id)
{
	int i;

	i = id & 0xFFFFFFFF; // lower 32bit role as index of TCB in pool
	memset(&gTCBPoolManager.TCBPool[i], 0, sizeof(TCB));
	gTCBPoolManager.TCBPool[i].link.id = i;
	gTCBPoolManager.useCount--;
}

TCB* createTask(QWORD flags, QWORD entryPointAddress)
{
	TCB* task;
	void* stackAddress;

	task = allocateTCB();
	if (task == NULL)
		return NULL;
	stackAddress = (void*)(TASK_STACK_POOL_ADDRESS +\
		TASK_STACK_SIZE * (task->link.id & 0xFFFFFFFF));
	setupTask(task, flags, entryPointAddress, stackAddress, TASK_STACK_SIZE);
	addTaskToReadyList(task);
	return task;
}

void setupTask(TCB* tcb, QWORD flags, QWORD entryPointAddress,\
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
	initTCBPool();
	initList(&gScheduler.readyList);
	gScheduler.runningTask = allocateTCB();
}

void setRunningTask(TCB* task)
{
	gScheduler.runningTask = task;
}

TCB* getRunningTask(void)
{
	return gScheduler.runningTask;
}

TCB* getNextTaskToRun(void)
{
	if (getListCount(&gScheduler.readyList) == 0)
		return NULL;
	return (TCB*)removeListFromHeader(&gScheduler.readyList);
}

void addTaskToReadyList(TCB* task)
{
	addListToTail(&gScheduler.readyList, task);
}

void schedule(void)
{
	TCB* runningTask;
	TCB* nextTask;
	BOOL prevFlag;

	if (getListCount(&gScheduler.readyList) == 0)
		return;
	prevFlag = setInterruptFlag(FALSE);
	nextTask = getNextTaskToRun();
	if (nextTask == NULL)
	{
		setInterruptFlag(prevFlag);
		return;
	}
	runningTask = getRunningTask();
	addTaskToReadyList(runningTask);
	gScheduler.processorTime = TASK_PROCESSOR_TIME;
	gScheduler.runningTask = nextTask;
	switchContext(&runningTask->context, &nextTask->context);
	setInterruptFlag(prevFlag);
}

BOOL scheduleInInterrupt(void)
{
	TCB* runningTask;
	TCB* nextTask;
	char* context;

	if (getListCount(&gScheduler.readyList) == 0)
		return FALSE;
	nextTask = getNextTaskToRun();
	if (nextTask == NULL)
		return FALSE;

	/*
	 * copy context from IST to runningTask context
	 * cause ISR already did SAVE_CONTEXT to IST
	 */
	runningTask = getRunningTask();
	context = (char*)IST_START_ADDRESS + IST_SIZE - sizeof(CONTEXT);
	memcpy(&runningTask->context, context, sizeof(CONTEXT));
	addTaskToReadyList(runningTask);

	/*
	 * ISR will LOAD_CONTEXT, so before do that, save context to IST
	 */
	gScheduler.runningTask = nextTask;
	memcpy(context, &nextTask->context, sizeof(CONTEXT));
	gScheduler.processorTime = TASK_PROCESSOR_TIME;
	return TRUE;
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
