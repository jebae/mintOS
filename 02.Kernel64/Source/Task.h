#ifndef __TASK_H__
#define __TASK_H__

#include "Types.h"

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

#pragma pack(push, 1)

typedef struct ContextStruct
{
	QWORD registers[TASK_REGISTER_COUNT];
} CONTEXT;

typedef struct TaskControlBlockStruct
{
	CONTEXT context;
	QWORD id;
	QWORD flags;
	void* stackAddress;
	QWORD stackSize;
} TCB;

#pragma pack(pop)

void setupTask(TCB* tcb, QWORD id, QWORD flags, QWORD entryPointAddress,\
	void* stackAddress, QWORD stackSize);

#endif
