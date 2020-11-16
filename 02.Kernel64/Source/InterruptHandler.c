#include "InterruptHandler.h"
#include "PIC.h"

void commonExceptionHandler(int vecNum, QWORD errCode)
{
	char buf[3] = {0,};

	buf[0] = '0' + vecNum / 10;
	buf[1] = '0' + vecNum % 10;
	printString(0, 0, "========================================");
	printString(0, 1, "Exception occur");
	printString(0, 2, "Vector: ");
	printString(8, 2, buf);
	printString(0, 3, "========================================");
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
	printString(70, 0, buf);

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

	buf[5] = '0' + vecNum / 10;
	buf[6] = '0' + vecNum % 10;
	buf[8] = '0' + gKeyboardInterruptCount;
	gKeyboardInterruptCount = (gKeyboardInterruptCount + 1) % 10;
	printString(0, 0, buf);
	sendEOIToPIC(vecNum - PIC_IRQ_START_VECTOR);
}
