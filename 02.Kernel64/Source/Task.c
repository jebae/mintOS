#include "Task.h"
#include "Descriptor.h"
#include "Utility.h"

void setupTask(TCB* tcb, QWORD id, QWORD flags, QWORD entryPointAddress,\
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

	tcb->id = id;
	tcb->stackAddress = stackAddress;
	tcb->stackSize = stackSize;
	tcb->flags = flags;
}
