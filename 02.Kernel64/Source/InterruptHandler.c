#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"
#include "Console.h"
#include "AssemblyUtility.h"
#include "HardDisk.h"

void commonExceptionHandler(int vecNum, QWORD errCode)
{
	char buf[3] = {0,};

	buf[0] = '0' + vecNum / 10;
	buf[1] = '0' + vecNum % 10;
	printStringXY(0, 0, "========================================");
	printStringXY(0, 1, "Exception occur");
	printStringXY(0, 2, "Vector: ");
	printStringXY(8, 2, buf);
	printStringXY(0, 3, "========================================");
	while (1);
}

void commonInterruptHandler(int vecNum)
{
	char buf[] = "[INT:  , ]";
	static int gCommonInterruptCount = 0;

	buf[5] = '0' + vecNum / 10;
	buf[6] = '0' + vecNum % 10;
	buf[8] = '0' + gCommonInterruptCount;
	gCommonInterruptCount = (gCommonInterruptCount + 1) % 10;
	printStringXY(70, 0, buf);

	/*
	 * interrupt vector start with 32,
	 * so IRQ = vecNum - 32
	 */
	sendEOIToPIC(vecNum - PIC_IRQ_START_VECTOR);
}

void keyboardHandler(int vecNum)
{
	char buf[] = "[INT:  , ]";
	static int gKeyboardInterruptCount = 0;
	BYTE scanCode;

	buf[5] = '0' + vecNum / 10;
	buf[6] = '0' + vecNum % 10;
	buf[8] = '0' + gKeyboardInterruptCount;
	gKeyboardInterruptCount = (gKeyboardInterruptCount + 1) % 10;
	printStringXY(0, 0, buf);

	if (isOutputBufferFull())
	{
		scanCode = getKeyboardScanCode();
		convertScanCodeAndPutQueue(scanCode);
	}
	sendEOIToPIC(vecNum - PIC_IRQ_START_VECTOR);
}

void timerHandler(int vecNum)
{
	char buf[] = "[INT:  , ]";
	static int gTimerInterruptCount = 0;

	buf[5] = '0' + vecNum / 10;
	buf[6] = '0' + vecNum % 10;
	buf[8] = '0' + gTimerInterruptCount;
	gTimerInterruptCount = (gTimerInterruptCount + 1) % 10;
	printStringXY(70, 0, buf);

	sendEOIToPIC(vecNum - PIC_IRQ_START_VECTOR);
	gTickCount++;
	decreaseProcessorTime();
	if (isProcessorTimeExpired() == TRUE)
		scheduleInInterrupt();
}

void deviceNotAvailableHandler(int vecNum)
{
	TCB* lastFPUUsedTask, * currentTask;
	QWORD lastFPUUsedTaskId;
	char buf[] = "[EXC:  , ]";
	static int gFPUInterruptCount = 0;

	buf[5] = '0' + vecNum / 10;
	buf[6] = '0' + vecNum % 10;
	buf[8] = '0' + gFPUInterruptCount;
	gFPUInterruptCount = (gFPUInterruptCount + 1) % 10;
	printStringXY(0, 0, buf);

	clearTS();
	lastFPUUsedTaskId = getLastFPUUsedTaskId();
	currentTask = getRunningTask();

	// save last FPU used tasks's FPU context if exist
	if (lastFPUUsedTaskId != TASK_INVALID_ID)
	{
		lastFPUUsedTask = getTCBInTCBPool(GET_TCB_OFFSET(lastFPUUsedTaskId));
		if (lastFPUUsedTask != NULL && lastFPUUsedTask->link.id == lastFPUUsedTaskId)
			saveFPUContext(lastFPUUsedTask->FPUContext);
	}

	// load or init currentTask FPU context
	if (currentTask->FPUUsed)
	{
		loadFPUContext(currentTask->FPUContext);
	}
	else
	{
		initFPU();
		currentTask->FPUUsed = TRUE;
	}
	setLastFPUUsedTaskId(currentTask->link.id);
}

void HDDhandler(int vecNum)
{
	char buf[] = "[INT:  , ]";
	static int gHDDInterruptCount = 0;

	buf[5] = '0' + vecNum / 10;
	buf[6] = '0' + vecNum % 10;
	buf[8] = '0' + gHDDInterruptCount;
	gHDDInterruptCount = (gHDDInterruptCount + 1) % 10;
	printStringXY(10, 0, buf);

	if (vecNum - PIC_IRQ_START_VECTOR == 14)
		setHDDInterruptFlag(TRUE, TRUE);
	else
		setHDDInterruptFlag(FALSE, TRUE);
	sendEOIToPIC(vecNum - PIC_IRQ_START_VECTOR);
}
