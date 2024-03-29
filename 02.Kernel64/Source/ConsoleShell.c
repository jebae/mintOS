#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "PIT.h"
#include "RTC.h"
#include "Task.h"
#include "Synchronization.h"
#include "DynamicMemory.h"
#include "HardDisk.h"
#include "FileSystem.h"
#include "SerialPort.h"

SHELL_COMMAND_ENTRY gCommandTable[] = {
	{"help", "show help", help},
	{"cls", "clear screen", cls},
	{"totalram", "show total RAM size", showTotalRAMSize},
	{"strtod", "string to decimal/hex convert", stringToDecimalHexTest},
	{"shutdown", "shutdown and reboot os", shutdown},
	{"settimer", "set PIT controller counter0, ex) settimer 10(ms) 1(periodic)", setTimer},
	{"wait", "wait ms using PIT", waitUsingPIT},
	{"rdtsc", "read timestamp counter", readTimestampCounter},
	{"cpuspeed", "measure processor speed", measureProcessorSpeed},
	{"date", "show date and time", showDateAndTime},
	{"createtask", "create task, ex) createtask 1(type) 10(count)", createTestTask},
	{"changepriority", "change task priority, ex) changepriority 1(id) 2(priority)", changeTaskPriority},
	{"tasklist", "show task list", showTaskList},
	{"killtask", "end task, ex) killtask 1(id) or all", killTask},
	{"cpuload", "show processor load", cpuload},
	{"testmutex", "test mutex", testMutex},
	{"deadlock", "test deadlock", testDeadlock},
	{"testthread", "test thread and process", testThread},
	{"matrix", "show matrix", showMatrix},
	{"testpie", "test pie calculation", testPie},
	{"dynamicraminfo", "show dynamic memory information", showDynamicMemoryInformation},
	{"testseqalloc", "test sequential allocation & free", testSequentialAllocation},
	{"testranalloc", "test random allocation & free", testRandomAllocation},
	{"hddinfo", "show HDD information", showHDDInformation},
	{"readsector", "read HDD sector, ex) readsector 0(LBA) 10(count)", readSector},
	{"writesector", "write HDD sector, ex) writesector 0(LBA) 10(count)", writeSector},
	{"mounthdd", "mount HDD", mountHDD},
	{"formathdd", "format HDD", formatHDD},
	{"filesysteminfo", "show file system information", showFileSystemInformation},
	{"createfile", "create file, ex) createfile a.txt", createFileInRootDir},
	{"deletefile", "delete file, ex) deletefile a.txt", deleteFileInRootDir},
	{"ls", "show directory", showRootDir},
	{"writefile", "write data to file ex) writefile a.txt", writeDataToFile},
	{"readfile", "read data from file ex) readfile a.txt", readDataToFile},
	{"testfileio", "test file IO function", testFileIO},
	{"testperformance", "test file read/write performance", testPerformance},
	{"flush", "flush file system cache", flushCache},
	{"download", "download data from serial, ex) download a.txt", downloadFile},
};

void startConsoleShell(void)
{
	char buf[CONSOLE_SHELL_MAX_COMMAND_BUFFER_COUNT];
	int idx = 0;
	BYTE key;
	int x, y;

	printf(CONSOLE_SHELL_PROMPT_MESSAGE);
	while (1)
	{
		key = getch();
		if (key == KEY_BACKSPACE)
		{
			if (idx > 0)
			{
				getCursor(&x, &y);
				printStringXY(x - 1, y, " ");
				setCursor(x - 1, y);
				idx--;
			}
		}
		else if (key == KEY_ENTER)
		{
			printf("\n");
			if (idx > 0)
			{
				buf[idx] = '\0';
				executeCommand(buf);
			}
			printf(CONSOLE_SHELL_PROMPT_MESSAGE);
			memset(buf, '\0', idx);
			idx = 0;
		}
		else if (key == KEY_LSHIFT || key == KEY_RSHIFT || key == KEY_CAPSLOCK ||\
			key == KEY_NUMLOCK || key == KEY_SCROLLLOCK)
		{
			continue;
		}
		else
		{
			if (key == KEY_TAB)
				key = ' ';
			if (idx < CONSOLE_SHELL_MAX_COMMAND_BUFFER_COUNT)
			{
				buf[idx++] = key;
				printf("%c", key);
			}
		}
	}
}

void executeCommand(const char* command)
{
	int i;
	int commandIdx = 0;
	int commandLen;
	int commandCount;

	while (command[commandIdx] != ' ' && command[commandIdx] != '\0')
		commandIdx++;
	commandCount = sizeof(gCommandTable) / sizeof(SHELL_COMMAND_ENTRY);
	for (i=0; i < commandCount; i++)
	{
		commandLen = strlen(gCommandTable[i].command);
		if (commandLen == commandIdx &&
			memcmp(gCommandTable[i].command, command, commandLen) == 0)
		{
			gCommandTable[i].func(command + commandIdx + 1);
			break;
		}
	}
	if (i >= commandCount)
		printf("command not found: %s\n", command);
}

void initParameter(PARAMETER_LIST* list, const char* parameter)
{
	list->buf = parameter;
	list->len = strlen(parameter);
	list->pos = 0;
}

int getNextParameter(PARAMETER_LIST* list, char* dest)
{
	int i;
	int len;

	if (list->len <= list->pos)
		return 0;
	for (i=list->pos; i < list->len; i++)
	{
		if (list->buf[i] == ' ')
			break;
	}
	len = i - list->pos;
	memcpy(dest, list->buf + list->pos, len);
	dest[len] = '\0';
	list->pos += len + 1;
	return len;
}

static void help(const char* params)
{
	int i;
	int commandCount;
	int len, maxLen = 0;
	int x, y;

	commandCount = sizeof(gCommandTable) / sizeof(SHELL_COMMAND_ENTRY);
	for (i=0; i < commandCount; i++)
	{
		len = strlen(gCommandTable[i].command);
		if (len > maxLen)
			maxLen = len;
	}

	for (i=0; i < commandCount; i++)
	{
		if (i != 0 && i % 20 == 0)
		{
			printf("press any key to continue...('q' is exit) : ");
			if (getch() == 'q')
			{
				printf("\n");
				break;
			}
			printf("\n");
		}
		printf("%s", gCommandTable[i].command);
		getCursor(&x, &y);
		setCursor(maxLen, y);
		printf(" - %s\n", gCommandTable[i].help);
	}
}

static void cls(const char* params)
{
	clearScreen();
	setCursor(0, 1);
}

static void showTotalRAMSize(const char* params)
{
	printf("total RAM size = %dMB\n", getTotalRAMSize());
}

static void stringToDecimalHexTest(const char* params)
{
	PARAMETER_LIST paramList;
	char param[100];
	int len;
	int count = 0;
	long value;

	initParameter(&paramList, params);
	while (1)
	{
		if ((len = getNextParameter(&paramList, param)) == 0)
			break;
		printf("Param %d = %s, Length = %d, ", count, param, len);
		if (memcmp(param, "0x", 2) == 0)
		{
			value = atoi(param + 2, 16);
			printf("Hex value = %q\n", value);
		}
		else
		{
			value = atoi(param, 10);
			printf("Decimal value = %d\n", value);
		}
		count++;
	}
}

static void shutdown(const char* params)
{
	printf("System shutdown start...\n");
	printf("Cache Flush...");
	if (flushFileSystemCache())
		printf("[PASS]\n");
	else
		printf("[FAIL]\n");
	printf("Press any key to reboot...");
	getch();
	reboot();
}

static void setTimer(const char* params)
{
	PARAMETER_LIST paramList;
	char param[100];
	long ms;
	BOOL isPeriodic;

	initParameter(&paramList, params);
	if (getNextParameter(&paramList, param) == 0)
	{
		printf("ex) settimer 10(ms) 1(periodic)\n");
		return;
	}
	ms = atoi(param, 10);
	if (getNextParameter(&paramList, param) == 0)
	{
		printf("ex) settimer 10(ms) 1(periodic)\n");
		return;
	}
	isPeriodic = atoi(param, 10);
	initPIT(MS_TO_COUNT(ms), isPeriodic);
	printf("Time = %d ms, Periodic = %d, Change complete\n", ms, isPeriodic);
}

static void waitUsingPIT(const char* params)
{
	PARAMETER_LIST paramList;
	char param[100];
	long ms;
	int until;
	int i;

	initParameter(&paramList, params);
	if (getNextParameter(&paramList, param) == 0)
	{
		printf("ex) wait 100(ms)\n");
		return;
	}
	ms = atoi(param, 10);
	printf("%d ms sleep\n", ms);
	disableInterrupt();
	until = ms / 30;
	for (i=0; i < until; i++)
		waitUsingDirectPIT(MS_TO_COUNT(30));
	waitUsingDirectPIT(MS_TO_COUNT(ms % 30));
	enableInterrupt();

	// restore timer
	initPIT(MS_TO_COUNT(1), TRUE);
}

static void readTimestampCounter(const char* params)
{
	QWORD tsc;

	tsc = readTSC();
	printf("timestamp counter: 0x%q\n", tsc);
}

static void measureProcessorSpeed(const char* params)
{
	int i;
	QWORD lastTSC, totalTSC = 0;

	// calculate speed for 10s
	disableInterrupt();
	for (i=0; i < 200; i++)
	{
		lastTSC = readTSC();
		waitUsingDirectPIT(MS_TO_COUNT(50));
		totalTSC += readTSC() - lastTSC;
	}
	initPIT(MS_TO_COUNT(1), TRUE);
	enableInterrupt();
	printf("CPU speed = %d MHz\n", totalTSC / 10 / 1000 / 1000);
}

static void showDateAndTime(const char* params)
{
	BYTE hour, minute, second, month, dayOfMonth, dayOfWeek;
	WORD year;

	readRTCTime(&hour, &minute, &second);
	readRTCDate(&year, &month, &dayOfMonth, &dayOfWeek);
	printf("%s %d %d %d:%d:%d %d\n",
		convertDayOfWeekToString(dayOfWeek),
		month, dayOfMonth, hour, minute, second, year);
}

static TCB task[2] = {0,};
static QWORD stack[1024] = {0,};

static void testTask1(void)
{
	BYTE data;
	int i = 0, j;
	int x = 0, y = 0, margin;
	CHARACTER* screen = (CHARACTER*)CONSOLE_VIDEO_MEMORY_ADDRESS;
	TCB* runningTask;

	runningTask = getRunningTask();
	margin = (runningTask->link.id & 0xFFFFFFFF) % 10;
	for (j=0; j < 20000; j++)
	{
		switch (i)
		{
		case 0:
			x++;
			if (x >= CONSOLE_WIDTH - margin)
				i = 1;
			break;
		case 1:
			y++;
			if (y >= CONSOLE_HEIGHT - margin)
				i = 2;
			break;
		case 2:
			x--;
			if (x < margin)
				i = 3;
			break;
		case 3:
			y--;
			if (y < margin)
				i = 0;
			break;
		}
		screen[y * CONSOLE_WIDTH + x].character = data;
		screen[y * CONSOLE_WIDTH + x].attribute = data & 0x0F;
		data++;
	}
}

static void testTask2(void)
{
	int i = 0, offset;
	CHARACTER* screen = (CHARACTER*)CONSOLE_VIDEO_MEMORY_ADDRESS;
	TCB* runningTask;
	char data[4] = {'-', '\\', '|', '/'};

	runningTask = getRunningTask();
	offset = (runningTask->link.id & 0xFFFFFFFF) * 2;
	offset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (offset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));
	while (1)
	{
		screen[offset].character = data[i % 4];
		screen[offset].attribute = offset % 15 + 1;
		i++;
	}
}

static void createTestTask(const char* params)
{
	PARAMETER_LIST paramList;
	char type[30];
	char count[30];
	int i;

	initParameter(&paramList, params);
	getNextParameter(&paramList, type);
	getNextParameter(&paramList, count);
	switch (atoi(type, 10))
	{
	case 1:
		for (i=0; i < atoi(count, 10); i++)
		{
			if (createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)testTask1) == NULL)
				break;
		}
		printf("Task1 %d created\n", i);
		break;
	case 2:
	default:
		for (i=0; i < atoi(count, 10); i++)
		{
			if (createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)testTask2) == NULL)
				break;
		}
		printf("Task2 %d created\n", i);
		break;
	}
}

static void changeTaskPriority(const char* params)
{
	PARAMETER_LIST paramList;
	char idBuf[30];
	char priorityBuf[30];
	QWORD id;
	BYTE priority;

	initParameter(&paramList, params);
	getNextParameter(&paramList, idBuf);
	getNextParameter(&paramList, priorityBuf);
	if (memcmp("0x", idBuf, 2) == 0)
		id = atoi(idBuf + 2, 16);
	else
		id = atoi(idBuf, 10);
	priority = atoi(priorityBuf, 10);
	printf("change task priority id [0x%q] priority [%d] ", id, priority);
	if (changePriority(id, priority) == TRUE)
		printf("success\n");
	else
		printf("fail\n");
}

static void showTaskList(const char* params)
{
	int i;
	int count = 0;
	TCB* task;

	printf("total task [%d]\n", getTaskCount());
	for (i=0; i < TASK_MAX_COUNT; i++)
	{
		task = getTCBInTCBPool(i);
		if ((task->link.id >> 32) == 0)
			continue;
		if (count != 0 && count % 10 == 0)
		{
			printf("Press any key to continue...('q' is exit): ");
			if (getch() == 'q')
			{
				printf("\n");
				break;
			}
			printf("\n");
		}
		printf("[%d] task id[0x%q], priority[%d], flags[0x%q], thread[%d]\n",
			1 + count++, task->link.id, GET_PRIORITY(task->flags),
			task->flags, getListCount(&task->childThreadList));
		printf("	parent PID[0x%Q], memory address[0x%Q], size[0x%Q]\n",
			task->parentProcessId, task->memoryAddress, task->memorySize);
	}
}

static void killTask(const char* params)
{
	PARAMETER_LIST paramList;
	char idBuf[30];
	QWORD id;
	int i;
	TCB* task;

	initParameter(&paramList, params);
	getNextParameter(&paramList, idBuf);
	if (memcmp("all", idBuf, 3) == 0)
	{
		for (i=0; i < TASK_MAX_COUNT; i++)
		{
			task = getTCBInTCBPool(i);
			id = task->link.id;
			if ((id >> 32) != 0 && (task->flags & TASK_FLAGS_SYSTEM) == 0)
			{
				printf("kill task [0x%q] ", id);
				if (endTask(id))
					printf("success\n");
				else
					printf("fail\n");
			}
		}
		return;
	}
	else if (memcmp("0x", idBuf, 2) == 0)
		id = atoi(idBuf + 2, 16);
	else
		id = atoi(idBuf, 10);
	task = getTCBInTCBPool(GET_TCB_OFFSET(id));
	if ((id >> 32) != 0 && (task->flags & TASK_FLAGS_SYSTEM) == 0)
	{
		printf("kill task [0x%q] ", id);
		if (endTask(id))
			printf("success\n");
		else
			printf("fail\n");
	}
	else
	{
		printf("task does not exist or is system task\n");
	}
}

static void cpuload(const char* params)
{
	printf("processor load: %d%%\n", getProcessorLoad());
}

static MUTEX gMutex;
static volatile QWORD gAdder;

static void printNumberTask(void)
{
	int i, j;
	QWORD tickCount;

	tickCount = getTickCount();
	while (getTickCount() - tickCount < 50)
		schedule();
	for (i=0; i < 5; i++)
	{
		lock(&gMutex);
		printf("ID[0x%Q] VALUE[%d]\n", getRunningTask()->link.id, gAdder);
		gAdder++;
		unlock(&gMutex);
		for (j=0; j < 300000; j++);
	}
	tickCount = getTickCount();
	while (getTickCount() - tickCount < 1000)
		schedule();
	exitTask();
}

static void testMutex(const char* params)
{
	int i;

	gAdder = 1;
	initMutex(&gMutex);
	for (i=0; i < 3; i++)
	{
		createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)printNumberTask);
	}
	printf("wait until %d task end...\n", i);
	getch();
}

static MUTEX gMutexA;
static MUTEX gMutexB;

static void deadLock1()
{
	printf("1 start\n");

	printf("1 lock A\n");
	lock(&gMutexA);

	for (int i=0; i < 9999999; i++);

	printf("1 lock B\n");
	lock(&gMutexB);

	printf("1 unlock B\n");
	unlock(&gMutexB);

	printf("1 unlock A\n");
	unlock(&gMutexA);

	printf("1 end\n");
}

static void deadLock2()
{
	printf("2 start\n");

	printf("2 lock B\n");
	lock(&gMutexB);

	printf("2 lock A\n");
	lock(&gMutexA);

	printf("2 unlock A\n");
	unlock(&gMutexA);

	printf("2 unlock B\n");
	unlock(&gMutexB);

	printf("2 end\n");
}

static void testDeadlock(const char* params)
{
	TCB* tasks[2];

	initMutex(&gMutexA);
	initMutex(&gMutexB);

	tasks[0] = createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)deadLock1);
	tasks[1] = createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)deadLock2);
}

static void createThreadTask(void)
{
	int i;

	for (i=0; i < 3; i++)
	{
		createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)testTask2);
	}
	while (1)
		sleep(1);
}

static void testThread(const char* params)
{
	TCB* process;

	process = createTask(TASK_FLAGS_LOW | TASK_FLAGS_PROCESS, 0, 0, (QWORD)createThreadTask);
	if (process == NULL)
		printf("process create fail\n");
	else
		printf("process [0x%Q] create success\n", process->link.id);

}

static volatile QWORD gRandomValue = 0;

QWORD random(void)
{
	gRandomValue = (gRandomValue * 412153 + 5571031) >> 16;
	return gRandomValue;
}

static void dropCharactorThread(void)
{
	int x;
	int i;
	char text[2] = {0,};

	x = random() % CONSOLE_WIDTH;
	while (1)
	{
		sleep(random() % 20);
		if (random() % 20 < 15)
		{
			text[0] = ' ';
			for (i=0; i < CONSOLE_HEIGHT - 1; i++)
			{
				printStringXY(x, i, text);
				sleep(50);
			}
		}
		else
		{
			for (i=0; i < CONSOLE_HEIGHT - 1; i++)
			{
				text[0] = i + random();
				printStringXY(x, i, text);
				sleep(50);
			}
		}
	}
}

static void matrixProcess(void)
{
	int i;

	for (i=0; i < 300; i++)
	{
		if (createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)dropCharactorThread) == NULL)
			break;
		sleep(random() % 5 + 5);
	}
	printf("%d thread created\n", i);
	getch();
}

static void showMatrix(const char* params)
{
	TCB* process;

	process = createTask(TASK_FLAGS_LOW | TASK_FLAGS_PROCESS, 0, 0, (QWORD)matrixProcess);
	if (process != NULL)
	{
		printf("matrix process [0x%Q] create success\n", process->link.id);
		while ((process->link.id >> 32) != 0)
			sleep(100);
	}
	else
	{
		printf("matrix process create fail\n");
	}
}

static void testFPUTask(void)
{
	double v1, v2;
	TCB* task;
	QWORD count, randomValue;
	int i, offset;
	char data[4] = {'-', '\\', '|', '/'};
	CHARACTER* screen = (CHARACTER*)CONSOLE_VIDEO_MEMORY_ADDRESS;

	task = getRunningTask();
	offset = (task->link.id & 0xFFFFFFFF) * 2;
	offset = (CONSOLE_WIDTH * CONSOLE_HEIGHT) - (offset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));
	while (1)
	{
		v1 = 1.0;
		v2 = 1.0;
		for (i=0; i < 10; i++)
		{
			randomValue = random();
			v1 *= (double)randomValue;
			v2 *= (double)randomValue;
			sleep(1);
			randomValue = random();
			v1 /= (double)randomValue;
			v2 /= (double)randomValue;
		}
		if (v1 != v2)
		{
			printf("value not same [%f] != [%f]\n", v1, v2);
			break;
		}
		count++;
		screen[offset].character = data[count % 4];
		screen[offset].attribute = offset % 15 + 1;
	}
}

static void testPie(const char* params)
{
	double res;
	int i;

	printf("pie calculation test\n");
	printf("355 / 113 = ");
	res = (double)355 / 113;
	printf("%d.%d%d\n", (QWORD)res, (QWORD)(res * 10) % 10, (QWORD)(res * 100) % 10);
	for (i=0; i < 100; i++)
	{
		createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)testFPUTask);
	}
}

static void showDynamicMemoryInformation(const char* params)
{
	QWORD startAddress, totalSize, metaSize, usedSize;

	getDynamicMemoryInfo(&startAddress, &totalSize, &metaSize, &usedSize);
	printf("=========== Dynamic Memory Information ===========\n");
	printf("start address: [0x%Q]\n", startAddress);
	printf("total size: [0x%Q]byte, [%d]MB\n", totalSize, totalSize / (1 << 20));
	printf("meta size: [0x%Q]byte, [%d]KB\n", metaSize, metaSize / (1 << 10));
	printf("used size: [0x%Q]byte, [%d]KB\n", usedSize, usedSize / (1 << 10));
}

static void testSequentialAllocation(const char* params)
{
	DYNAMICMEMORY* dynamicMemory = getDynamicMemoryManager();
	QWORD* buf;
	int i, j, k;

	printf("=========== Dynamic Memory Test ===========\n");
	for (i=0; i < dynamicMemory->maxLevelCount; i++)
	{
		printf("block list [%d] test start\n", i);
		printf("allocation and compare: ");
		for (j=0; j < (dynamicMemory->blockCountOfSmallestBlock >> i); j++)
		{
			buf = allocateMemory(DYNAMIC_MEMORY_MIN_SIZE << i);
			if (buf == NULL)
			{
				printf("\nallocation fail\n");
				return;
			}
			for (k=0; k < (DYNAMIC_MEMORY_MIN_SIZE << i) / 8; k++)
			{
				buf[k] = k;
			}
			for (k=0; k < (DYNAMIC_MEMORY_MIN_SIZE << i) / 8; k++)
			{
				if (buf[k] != k)
				{
					printf("\ncompare fail\n");
					return;
				}
			}
			printf(".");
		}
		printf("\nfree: ");
		for (j=0; j < (dynamicMemory->blockCountOfSmallestBlock >> i); j++)
		{
			if (freeMemory((void*)(dynamicMemory->startAddress +\
				(DYNAMIC_MEMORY_MIN_SIZE << i) * j)) == FALSE)
			{
				printf("\nfree fail\n");
				return;
			}
			printf(".");
		}
		printf("\n");
	}
	printf("test complete\n");
}

static void randomAllocationTask(void)
{
	TCB* task;
	QWORD memorySize;
	char strBuffer[200];
	BYTE* buf;
	int i, j, y;

	task = getRunningTask();
	y = task->link.id % 15 + 9;
	for (i=0; i < 10; i++)
	{
		do {
			memorySize = ((random() % (32 * 1024)) + 1) * 1024;
			buf = allocateMemory(memorySize);
			if (buf == NULL)
				sleep(1);
		} while (buf == NULL);
		sprintf(strBuffer, "|address: [0x%Q] size: [0x%Q] allocation success", buf, memorySize);
		printStringXY(20, y, strBuffer);
		sleep(200);

		sprintf(strBuffer, "|address: [0x%Q] size: [0x%Q] data write...", buf, memorySize);
		printStringXY(20, y, strBuffer);
		for (j=0; j < memorySize / 2; j++)
		{
			buf[j] = random() & 0xFF;
			buf[j + memorySize / 2] = buf[j];
		}
		sleep(200);

		sprintf(strBuffer, "|address: [0x%Q] size: [0x%Q] data verify...", buf, memorySize);
		printStringXY(20, y, strBuffer);
		for (j=0; j < memorySize / 2; j++)
		{
			if (buf[j] != buf[j + memorySize / 2])
			{
				printf("task id [0x%Q] verify fail\n", task->link.id);
				exitTask();
			}
		}
		freeMemory(buf);
		sleep(200);
	}
	exitTask();
}

static void testRandomAllocation(const char* params)
{
	int i;

	for (i=0; i < 1000; i++)
	{
		createTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD, 0, 0, (QWORD)randomAllocationTask);
	}
}

static void showHDDInformation(const char* params)
{
	HDDINFORMATION HDD;
	char buf[100];

	if (getHDDInformation(&HDD) == FALSE)
	{
		printf("HDD information read fail\n");
		return;
	}
	printf("========= Primary Master HDD Information =========\n");
	memcpy(buf, HDD.modelNumber, sizeof(HDD.modelNumber));
	buf[sizeof(HDD.modelNumber)] = '\0';
	printf("Model number: %s\n", buf);

	memcpy(buf, HDD.serialNumber, sizeof(HDD.serialNumber));
	buf[sizeof(HDD.serialNumber)] = '\0';
	printf("Serial number: %s\n", buf);

	printf("Head count: %d\n", HDD.numberOfHead);
	printf("Cylinder count: %d\n", HDD.numberOfCylinder);
	printf("Sector count: %d\n", HDD.numberOfSectorPerCylinder);
	printf("Total sector: %d, %dMB\n", HDD.totalSectors, HDD.totalSectors / 2 / 1024);
}

static void readSector(const char* params)
{
	PARAMETER_LIST paramList;
	char LBABuf[50], sectorCountBuf[50];
	DWORD LBA;
	int sectorCount, i, j;
	char* buf;
	BOOL exit;
	BYTE data;

	initParameter(&paramList, params);
	if (getNextParameter(&paramList, LBABuf) == 0 ||
		getNextParameter(&paramList, sectorCountBuf) == 0)
	{
		printf("ex) readsector 0(LBA) 10(count)\n");
		return;
	}
	LBA = atoi(LBABuf, 10);
	sectorCount = atoi(sectorCountBuf, 10);
	buf = allocateMemory(512 * sectorCount);
	if (buf == NULL)
	{
		printf("allocate memory fail\n");
		return;
	}
	if (readHDDSector(TRUE, TRUE, LBA, sectorCount, buf) != sectorCount)
	{
		printf("read fail\n");
		freeMemory(buf);
		return;
	}
	printf("LBA [%d], [%d] sector read sucess\n", LBA, sectorCount);
	for (i=0; i < sectorCount; i++)
	{
		for (j=0; j < 512; j++)
		{
			if (j % 256 == 0 && !(j == 0 && i == 0))
			{
				printf("\nPress any key to continue...('q' is exit): ");
				if (getch() == 'q')
				{
					exit = TRUE;
					break;
				}
			}
			if (j % 16 == 0)
				printf("\n[LBA:%d, Offset:%d]\t| ", LBA + i, j);
			data = buf[i * 512 + j] & 0xFF;
			if (data < 0x10)
				printf("0");
			printf("%X ", data);
		}
		if (exit)
			break;
		printf("\n");
	}
	freeMemory(buf);
}

static void writeSector(const char* params)
{
	PARAMETER_LIST paramList;
	char LBABuf[50], sectorCountBuf[50];
	DWORD LBA;
	int sectorCount, i, j;
	char* buf;
	BOOL exit;
	BYTE data;
	DWORD writeCount = 1;

	initParameter(&paramList, params);
	if (getNextParameter(&paramList, LBABuf) == 0 ||
		getNextParameter(&paramList, sectorCountBuf) == 0)
	{
		printf("ex) writesector 0(LBA) 10(count)\n");
		return;
	}
	LBA = atoi(LBABuf, 10);
	sectorCount = atoi(sectorCountBuf, 10);
	buf = allocateMemory(512 * sectorCount);
	if (buf == NULL)
	{
		printf("allocate memory fail\n");
		return;
	}

	for (i=0; i < sectorCount; i++)
	{
		for (j=0; j < 512; j+=8)
		{
			*(DWORD*)&buf[512 * i + j] = LBA + i;
			*(DWORD*)&buf[512 * i + j + 4] = writeCount++;
		}
	}
	if (writeHDDSector(TRUE, TRUE, LBA, sectorCount, buf) != sectorCount)
		printf("write fail\n");
	freeMemory(buf);
}

static void mountHDD(const char* params)
{
	if (!mount())
	{
		printf("HDD mount fail\n");
		return;
	}
	printf("HDD mount success\n");
}

static void formatHDD(const char* params)
{
	if (!format())
	{
		printf("HDD format fail\n");
		return;
	}
	printf("HDD format success\n");
}

static void showFileSystemInformation(const char* params)
{
	FILESYSTEM_MANAGER manager;

	getFileSystemInformation(&manager);
	printf("========== File System Information ==========\n");
	printf("Mounted: %d\n", manager.isMounted);
	printf("Reserved sector count: %d\n", manager.reservedSectorCount);
	printf("Cluster link table start address: %d\n", manager.clusterLinkAreaStartAddress);
	printf("Cluster link table size: %d\n", manager.clusterLinkAreaSize);
	printf("Data area start address: %d\n", manager.dataAreaStartAddress);
	printf("Total cluster count: %d\n", manager.totalClusterCount);
}

static void createFileInRootDir(const char* params)
{
	PARAMETER_LIST paramList;
	char fileName[50];
	int len;
	FILE* file;

	initParameter(&paramList, params);
	getNextParameter(&paramList, fileName);
	len = strlen(fileName);
	if (len == 0 || len > FILESYSTEM_MAX_FILENAME_LENGTH - 1)
	{
		printf("invalid file name\n");
		return;
	}

	file = fopen(fileName, "w");
	if (file == NULL)
	{
		printf("file create fail\n");
		return;
	}
	fclose(file);
	printf("%s created\n", fileName);
}

static void deleteFileInRootDir(const char* params)
{
	PARAMETER_LIST paramList;
	char fileName[50];
	int len;

	initParameter(&paramList, params);
	getNextParameter(&paramList, fileName);
	len = strlen(fileName);
	if (len == 0 || len > FILESYSTEM_MAX_FILENAME_LENGTH - 1)
	{
		printf("invalid file name\n");
		return;
	}

	if (remove(fileName) != 0)
	{
		printf("file not found or file opened\n", fileName);
		return;
	}
	printf("%s removed\n", fileName);
}

static void showRootDir(const char* params)
{
	BYTE* clusterBuf;
	int i, count, totalCount;
	DIRECTORY_ENTRY* entry;
	char buf[400];
	char temp[50];
	DWORD totalByte, usedClusterCount;
	DIR* dir;
	FILESYSTEM_MANAGER manager;

	getFileSystemInformation(&manager);

	dir = openDirectory("/");
	if (dir == NULL)
	{
		printf("root directory open fail\n");
		return;
	}

	totalCount = 0;
	totalByte = 0;
	usedClusterCount = 0;
	entry = readDirectory(dir);
	while (entry != NULL)
	{
		totalByte += entry->fileSize;
		totalCount++;
		if (entry->fileSize == 0)
			usedClusterCount++;
		else
			usedClusterCount += (entry->fileSize + (FILESYSTEM_CLUSTER_SIZE - 1)) /
				FILESYSTEM_CLUSTER_SIZE;
		entry = readDirectory(dir);
	}

	rewindDirectory(dir);
	entry = readDirectory(dir);
	count = 0;
	while (entry != NULL)
	{
		memset(buf, ' ', sizeof(buf) - 1);
		buf[sizeof(buf) - 1] = '\0';

		// file name
		memcpy(buf, entry->d_name, strlen(entry->d_name));

		// file size
		sprintf(temp, "%d byte", entry->fileSize);
		memcpy(buf + 30, temp, strlen(temp));

		// start cluster idx
		sprintf(temp, "0x%X cluster", entry->startClusterIdx);
		memcpy(buf + 55, temp, strlen(temp) + 1);

		printf("   %s\n", buf);
		if (count != 0 && count % 20 == 0)
		{
			printf("Press any key to continue...('q' is exit): ");
			if (getch() == 'q')
			{
				printf("\n");
				break;
			}
			printf("\n");
		}
		entry = readDirectory(dir);
	}
	printf("Total file count: %d\n", totalCount);
	printf("Total file size: %dKB (%d cluster)\n",
			totalByte / 1024, usedClusterCount);
	printf("Free space: %dKB (%d cluster)\n",
			(manager.totalClusterCount - usedClusterCount) * FILESYSTEM_CLUSTER_SIZE / 1024,
			manager.totalClusterCount - usedClusterCount);
	closedir(dir);
}

static void writeDataToFile(const char* params)
{
	PARAMETER_LIST paramList;
	char fileName[50];
	int len;
	FILE* fp;
	int enterCount;
	BYTE key;

	initParameter(&paramList, params);
	len = getNextParameter(&paramList, fileName);
	fileName[len] = '\0';

	if (len == 0 || len > FILESYSTEM_MAX_FILENAME_LENGTH - 1)
	{
		printf("invalid file name\n");
		return;
	}

	fp = fopen(fileName, "w");
	if (fp == NULL)
	{
		printf("%s file open fail\n", fileName);
		return;
	}
	enterCount = 0;
	while (1)
	{
		key = getch();
		if (key == KEY_ENTER)
		{
			enterCount++;
			if (enterCount >= 3)
				break;
		}
		else
		{
			enterCount = 0;
		}
		printf("%c", key);
		if (fwrite(&key, 1, 1, fp) != 1)
		{
			printf("file write fail\n");
			break;
		}
	}
	printf("file create success\n");
	fclose(fp);
}

static void readDataToFile(const char* params)
{
	PARAMETER_LIST paramList;
	char fileName[50];
	int len, enterCount;
	FILE* fp;
	BYTE key;

	initParameter(&paramList, params);
	len = getNextParameter(&paramList, fileName);
	fileName[len] = '\0';
	if (len == 0 || len > FILESYSTEM_MAX_FILENAME_LENGTH - 1)
	{
		printf("invalid file name\n");
		return;
	}

	fp = fopen(fileName, "r");
	if (fp == NULL)
	{
		printf("%s file open fail\n", fileName);
		return;
	}
	enterCount = 0;
	while (1)
	{
		if (fread(&key, 1, 1, fp) != 1)
			break;
		printf("%c", key);
		if (key == KEY_ENTER)
		{
			enterCount++;
			if (enterCount % 20 == 0)
			{
				printf("Press any key to continue...('q' is exit): ");
				if (getch() == 'q')
				{
					printf("\n");
					break;
				}
				printf("\n");
				enterCount = 0;
			}
		}
	}
	fclose(fp);
}

static void testFileIO(const char* params)
{
	FILE* file;
	BYTE* buf;
	int i, j;
	DWORD randomOffset;
	DWORD byteCount;
	BYTE tempBuffer[1024];
	DWORD maxFileSize;

	printf("============= File I/O Function Test =============\n");
	maxFileSize = 4 * 1024 * 1024; // 4MB
	buf = (BYTE*)allocateMemory(maxFileSize);
	if (buf == NULL)
	{
		printf("memory allocation fail\n");
		return;
	}
	remove("testfileio.bin");

	// ====================================
	// open file
	// ====================================
	printf("1. File open fail test...");
	file = fopen("testfileio.bin", "r");
	if (file == NULL)
		printf("[PASS]\n");
	else
	{
		printf("[FAIL]\n");
		fclose(file);
	}

	// ====================================
	// file create
	// ====================================
	printf("2. File create test...");
	file = fopen("testfileio.bin", "w");
	if (file != NULL)
	{
		printf("[PASS]\n");
		printf("\t File handle [0x%Q]\n", file);
	}
	else
		printf("[FAIL]\n");

	// ====================================
	// sequential file write
	// ====================================
	printf("3. Sequential write test(cluster size)...");
	for (i=0; i < 100; i++)
	{
		memset(buf, (BYTE)i, FILESYSTEM_CLUSTER_SIZE);
		if (fwrite(buf, 1, FILESYSTEM_CLUSTER_SIZE, file) != FILESYSTEM_CLUSTER_SIZE)
		{
			printf("[FAIL]\n");
			printf("\t %d cluster error\n", i);
			break;
		}
	}
	if (i >= 100)
		printf("[PASS]\n");

	// ====================================
	// sequential file read
	// ====================================
	printf("4. Sequential read and verify test(cluster size)...");
	fseek(file, -100 * FILESYSTEM_CLUSTER_SIZE, SEEK_END);
	for (i=0; i < 100; i++)
	{
		if (fread(buf, 1, FILESYSTEM_CLUSTER_SIZE, file) != FILESYSTEM_CLUSTER_SIZE)
		{
			printf("[FAIL]\n");
			return;
		}
		for (j=0; j < FILESYSTEM_CLUSTER_SIZE; j++)
		{
			if (buf[j] != (BYTE)i)
			{
				printf("[FAIL]\n");
				printf("\t %d [%X] != [%X]\n", i, buf[j], (BYTE)i);
				break;
			}
		}
	}
	if (i >= 100)
		printf("[PASS]\n");

	// ====================================
	// random file write
	// ====================================
	printf("5. Random write test...\n");
	memset(buf, 0, maxFileSize);
	fseek(file, -100 * FILESYSTEM_CLUSTER_SIZE, SEEK_CUR);
	fread(buf, 1, maxFileSize, file);
	
	for (i=0; i < 100; i++)
	{
		// from randomOffset to randomOffset + byteCount
		byteCount = random() % (sizeof(tempBuffer) - 1) + 1;
		randomOffset = random() % (maxFileSize - byteCount);
		printf("\t [%d] offset [%d] byte [%d]...", i, randomOffset, byteCount);

		fseek(file, randomOffset, SEEK_SET);
		memset(tempBuffer, (BYTE)i, byteCount);
		if (fwrite(tempBuffer, 1, byteCount, file) != byteCount)
		{
			printf("[FAIL]\n");
			break;
		}
		else
		{
			printf("[PASS]\n");
		}
		memset(buf + randomOffset, (BYTE)i, byteCount);
	}
	fseek(file, maxFileSize - 1, SEEK_SET);
	fwrite(&i, 1, 1, file);
	buf[maxFileSize - 1] = (BYTE)i;

	// ====================================
	// random file read
	// ====================================
	printf("6. Random read and verify test...\n");
	for (i=0; i < 100; i++)
	{
		// from randomOffset to randomOffset + byteCount
		byteCount = random() % (sizeof(tempBuffer) - 1) + 1;
		randomOffset = random() % (maxFileSize - byteCount);
		printf("\t [%d] offset [%d] byte [%d]...", i, randomOffset, byteCount);

		fseek(file, randomOffset, SEEK_SET);
		if (fread(tempBuffer, 1, byteCount, file) != byteCount)
		{
			printf("[FAIL]\n");
			printf("\t %d read fail\n", randomOffset);
			break;
		}
		if (memcmp(buf + randomOffset, tempBuffer, byteCount) != 0)
		{
			printf("[FAIL]\n");
			printf("\t %d compare fail\n", randomOffset);
			break;
		}
		printf("[PASS]\n");
	}

	// ====================================
	// sequential file read
	// ====================================
	printf("7. Sequential write, read and verify test(1024 byte)...\n");
	fseek(file, -maxFileSize, SEEK_CUR);
	for (i=0; i < (2 * 1024 * 1024 / 1024); i++)
	{
		printf("\t [%d] offset [%d] byte [%d] write...", i, i * 1024, 1024);
		if (fwrite(buf + i * 1024, 1, 1024, file) != 1024)
		{
			printf("[FAIL]\n");
			return;
		}
		else
		{
			printf("[PASS]\n");
		}
	}
	fseek(file, -maxFileSize, SEEK_SET);
	
	for (i=0; i < maxFileSize / 1024; i++)
	{
		printf("\t [%d] offset [%d] byte [%d] read and verify...",
				i, i * 1024, 1024);
		if (fread(tempBuffer, 1, 1024, file) != 1024)
		{
			printf("[FAIL]\n");
			return;
		}
		if (memcmp(buf + i * 1024, tempBuffer, 1024) != 0)
		{
			printf("[FAIL]\n");
			break;
		}
		else
		{
			printf("[PASS]\n");
		}
	}

	// ====================================
	// file remove fail
	// ====================================
	printf("8. File delete fail test...");
	if (remove("testfileio.bin") != 0)
		printf("[PASS]\n");
	else
		printf("[FAIL]\n");

	// ====================================
	// file close
	// ====================================
	printf("9. File close test...");
	if (fclose(file) == 0)
		printf("[PASS]\n");
	else
		printf("[FAIL]\n");

	// ====================================
	// file remove
	// ====================================
	printf("10. File delete test...");
	if (remove("testfileio.bin") == 0)
		printf("[PASS]\n");
	else
		printf("[FAIL]\n");

	freeMemory(buf);
}

static void testPerformance(const char* params)
{
	FILE* file;
	DWORD clusterTestFileSize;
	DWORD oneByteTestFileSize;
	QWORD lastTickCount;
	DWORD i;
	BYTE* buf;

	clusterTestFileSize = 1024 * 1024; // 1MB
	oneByteTestFileSize = 16 * 1024; // 16KB

	buf = (BYTE*)allocateMemory(clusterTestFileSize);
	if (buf == NULL)
	{
		printf("Memory allocate fail\n");
		return;
	}
	memset(buf, 0, FILESYSTEM_CLUSTER_SIZE);

	// ====================================
	// file write by cluster
	// ====================================
	printf("1. Sequential Read/Write test(cluster size)\n");
	remove("performance.txt");
	file = fopen("performance.txt", "w");
	if (file == NULL)
	{
		printf("file open fail\n");
		freeMemory(buf);
		return;
	}

	lastTickCount = getTickCount();
	for (i=0; i < clusterTestFileSize / FILESYSTEM_CLUSTER_SIZE; i++)
	{
		if (fwrite(buf, 1, FILESYSTEM_CLUSTER_SIZE, file) != FILESYSTEM_CLUSTER_SIZE)
		{
			printf("write fail\n");
			fclose(file);
			freeMemory(buf);
			return;
		}
	}
	printf("\t sequential write(cluster size): %dms\n", getTickCount() - lastTickCount);

	// ====================================
	// file read by cluster
	// ====================================
	fseek(file, 0, SEEK_SET);
	lastTickCount = getTickCount();
	for (i=0; i < clusterTestFileSize / FILESYSTEM_CLUSTER_SIZE; i++)
	{
		if (fread(buf, 1, FILESYSTEM_CLUSTER_SIZE, file) != FILESYSTEM_CLUSTER_SIZE)
		{
			printf("read fail\n");
			fclose(file);
			freeMemory(buf);
			return;
		}
	}
	printf("\t sequential read(cluster size): %dms\n", getTickCount() - lastTickCount);
	fclose(file);

	// ====================================
	// file write by 1 byte
	// ====================================
	printf("2. Sequential Read/Write test(1 byte)\n");
	remove("performance.txt");
	file = fopen("performance.txt", "w");
	if (file == NULL)
	{
		printf("file open fail\n");
		freeMemory(buf);
		return;
	}
	lastTickCount = getTickCount();
	for (i=0; i < oneByteTestFileSize; i++)
	{
		if (fwrite(buf, 1, 1, file) != 1)
		{
			printf("write fail\n");
			fclose(file);
			freeMemory(buf);
			return;
		}
	}
	printf("\t sequential write(1 byte): %dms\n", getTickCount() - lastTickCount);

	// ====================================
	// file read by 1 byte
	// ====================================
	fseek(file, 0, SEEK_SET);
	lastTickCount = getTickCount();
	for (i=0; i < oneByteTestFileSize; i++)
	{
		if (fread(buf, 1, 1, file) != 1)
		{
			printf("read fail\n");
			fclose(file);
			freeMemory(buf);
			return;
		}
	}
	printf("\t sequential read(1 byte): %dms\n", getTickCount() - lastTickCount);
	freeMemory(buf);
	fclose(file);
}

static void flushCache(const char* params)
{
	QWORD lastTickCount;

	lastTickCount = getTickCount();
	printf("Cache flush...");
	if (flushFileSystemCache())
		printf("[PASS]\n");
	else
		printf("[FAIL]\n");
	printf("Total time: %dms\n", getTickCount() - lastTickCount);
}

static void downloadFile(const char* params)
{
	PARAMETER_LIST paramList;
	char fileName[50];
	int fileNameLength;
	DWORD dataLength;
	FILE* fp;
	DWORD receivedSize;
	DWORD tempSize;
	BYTE buf[SERIAL_FIFO_MAX_SIZE];
	QWORD lastReceivedTickCount;

	initParameter(&paramList, params);
	fileNameLength = getNextParameter(&paramList, fileName);
	fileName[fileNameLength] = '\0';
	if (fileNameLength > FILESYSTEM_MAX_FILENAME_LENGTH - 1 ||
			fileNameLength == 0)
	{
		printf("invalid file name\n");
		return;
	}

	clearSerialFIFO();

	printf("wait for data length...");
	receivedSize = 0;
	lastReceivedTickCount = getTickCount();
	while (receivedSize < 4)
	{
		tempSize = receiveSerialData(((BYTE*)&dataLength) + receivedSize, 4 - receivedSize);
		receivedSize += tempSize;
		if (tempSize == 0)
		{
			sleep(0);
			if (getTickCount() - lastReceivedTickCount > 30000)
			{
				printf("time out\n");
				return;
			}
		}
		else
		{
			lastReceivedTickCount = getTickCount();
		}
	}
	printf("%d\n", dataLength);

	sendSerialData("A", 1);

	fp = fopen(fileName, "w");
	if (fp == NULL)
	{
		printf("file open fail\n");
		return;
	}

	printf("data receive start: ");
	receivedSize = 0;
	lastReceivedTickCount = getTickCount();
	while (receivedSize < dataLength)
	{
		tempSize = receiveSerialData(buf, SERIAL_FIFO_MAX_SIZE);
		receivedSize += tempSize;
		if (tempSize == 0)
		{
			sleep(0);
			if (getTickCount() - lastReceivedTickCount > 10000)
			{
				printf("time out\n");
				return;
			}
		}
		else
		{
			if (receivedSize % SERIAL_FIFO_MAX_SIZE == 0 ||
					receivedSize == dataLength)
			{
				sendSerialData("A", 1);
				printf("#");
			}
			if (fwrite(buf, 1, tempSize, fp) != tempSize)
			{
				printf("file write error occur\n");
				break;
			}
			lastReceivedTickCount = getTickCount();
		}
	}

	if (dataLength != receivedSize)
		printf("error occur. %d/%d\n", dataLength, receivedSize);
	else
		printf("download complete %d\n", dataLength);

	fclose(fp);
	flushFileSystemCache();
}
