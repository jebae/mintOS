#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "Utility.h"
#include "PIC.h"
#include "PIT.h"
#include "Task.h"
#include "Console.h"
#include "ConsoleShell.h"

void Main(void)
{
	int x, y;

	initConsole(0, 10);
	printf("Switch To IA-32e Mode Success\n");
	printf("IA-32e C Language Kernel Start.....[PASS]\n");

	getCursor(&x, &y);
	printf("init GDT And Switch For IA-32e Mode.....[    ]");
	initGDTTableAndTSS();
	loadGDTR(GDTR_START_ADDRESS);
	setCursor(41, y++);
	printf("PASS\n");

	printf("TSS Segment Load.....[    ]");
	loadTR(GDT_TSS_SEGMENT);
	setCursor(22, y++);
	printf("PASS\n");

	printf("init IDT.....[    ]");
	initIDTTables();
	loadIDTR(IDTR_START_ADDRESS);
	setCursor(14, y++);
	printf("PASS\n");

	printf("Total RAM Size Check.....[    ]");
	checkTotalRAMSize();
	setCursor(26, y++);
	printf("PASS], Size = %dMB\n", getTotalRAMSize());

	printf("TCB Pool And Scheduler initialize.....[PASS]\n");
	y++;
	initScheduler();
	initPIT(MS_TO_COUNT(1), 1);

	printf("Keyboard Activate And Queue Initialize.....[    ]");
	if (initKeyboard())
	{
		setCursor(44, y++);
		printf("PASS\n");
		changeKeyboardLED(FALSE, FALSE, FALSE);
	}
	else
	{
		setCursor(44, y++);
		printf("FAIL\n");
		while (1);
	}

	printf("PIC Controller And Interrupt Initialize.....[    ]");
	initPIC();
	maskPICInterrupt(0);
	enableInterrupt();
	setCursor(45, y++);
	printf("PASS\n");
	createTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE,\
		0, 0, (QWORD)idleTask);
	startConsoleShell();
}
