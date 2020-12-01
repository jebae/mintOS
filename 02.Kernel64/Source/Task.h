#ifndef __TASK_H__
#define __TASK_H__

#include "Types.h"
#include "List.h"
#include "Utility.h"

/*
 * SS, RSP, RFALGS, CS, RIP (5 registers which CPU save)
 * 19 registers saved in ISR
 */
#define TASK_REGISTER_COUNT					(5 + 19)
#define TASK_REGISTER_SIZE					8

#define TASK_GS_OFFSET							0
#define TASK_FS_OFFSET							1
#define TASK_ES_OFFSET							2
#define TASK_DS_OFFSET							3
#define TASK_R15_OFFSET							4
#define TASK_R14_OFFSET							5
#define TASK_R13_OFFSET							6
#define TASK_R12_OFFSET							7
#define TASK_R11_OFFSET							8
#define TASK_R10_OFFSET							9
#define TASK_R9_OFFSET							10
#define TASK_R8_OFFSET							11
#define TASK_RSI_OFFSET							12
#define TASK_RDI_OFFSET							13
#define TASK_RDX_OFFSET							14
#define TASK_RCX_OFFSET							15
#define TASK_RBX_OFFSET							16
#define TASK_RAX_OFFSET							17
#define TASK_RBP_OFFSET							18
#define TASK_RIP_OFFSET							19
#define TASK_CS_OFFSET							20
#define TASK_RFLAGS_OFFSET					21
#define TASK_RSP_OFFSET							22
#define TASK_SS_OFFSET							23

#define TASK_TCB_POOL_ADDRESS				0x800000 // after IST (0x700000 ~ 1MB)
#define TASK_MAX_COUNT							1024
#define TASK_STACK_POOL_ADDRESS			(TASK_TCB_POOL_ADDRESS + sizeof(TCB) * TASK_MAX_COUNT)
#define TASK_STACK_SIZE							8192
#define TASK_INVALID_ID							0xFFFFFFFFFFFFFFFF
#define TASK_PROCESSOR_TIME					5 // 5ms

#define TASK_MAX_READY_LIST_COUNT		5
#define TASK_FLAGS_HIGHEST					0
#define TASK_FLAGS_HIGH							1
#define TASK_FLAGS_MEDIUM						2
#define TASK_FLAGS_LOW							3
#define TASK_FLAGS_LOWEST						4
#define TASK_FLAGS_WAIT							0xFF

// task flag
#define TASK_FLAGS_ENDTASK					0x8000000000000000
#define TASK_FLAGS_IDLE							0x0800000000000000

#define GET_PRIORITY(x) ((x) & 0xFF)
#define SET_PRIORITY(x, priority) ((x) = ((x) & 0xFFFFFFFFFFFFFF00) | (priority))
#define GET_TCB_OFFSET(x) ((x) & 0xFFFFFFFF)

#pragma pack(push, 1)

typedef struct ContextStruct
{
	QWORD registers[TASK_REGISTER_COUNT];
} CONTEXT;

typedef struct TaskControlBlockStruct
{
	LISTLINK link;
	QWORD flags;
	CONTEXT context;
	void* stackAddress;
	QWORD stackSize;
} TCB;

typedef struct TCBPoolManagerStruct
{
	TCB* TCBPool;
	int maxCount;
	int useCount;
	int allocatedCount; // use to make task id
} TCB_POOL_MANAGER;

typedef struct SchedulerStruct
{
	TCB* runningTask;
	int processorTime;
	LIST readyList[TASK_MAX_READY_LIST_COUNT];
	LIST waitList;
	int executeCount[TASK_MAX_READY_LIST_COUNT];
	QWORD processorLoad;
	QWORD processorTimeSpentByIdleTask;
} SCHEDULER;

#pragma pack(pop)

static void initTCBPool(void);
static TCB* allocateTCB(void);
static void freeTCB(QWORD id);
TCB* createTask(QWORD flags, QWORD entryPointAddress);
static void setupTask(TCB* tcb, QWORD flags, QWORD entryPointAddress,\
	void* stackAddress, QWORD stackSize);

void initScheduler(void);
void setRunningTask(TCB* task);
TCB* getRunningTask(void);
static TCB* getNextTaskToRun(void);
static BOOL addTaskToReadyList(TCB* task);
void schedule(void);
BOOL scheduleInInterrupt(void);
void decreaseProcessorTime(void);
BOOL isProcessorTimeExpired(void);
static TCB* removeTaskFromReadyList(QWORD id);
BOOL changePriority(QWORD id, BYTE priority);
BOOL endTask(QWORD id);
void exitTask(void);
int getReadyTaskCount(void);
int getTaskCount(void);
TCB* getTCBInTCBPool(int offset);
BOOL isTaskExist(QWORD id);
QWORD getProcessorLoad(void);

void idleTask(void);
void haltProcessorByLoad(void);

#endif
