#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "PIT.h"
#include "RTC.h"
#include "Task.h"

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
	{"killtask", "end task, ex) killtask 1(id)", killTask},
	{"cpuload", "show processor load", cpuload},
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
		//schedule();
	}
	exitTask();
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
		//schedule();
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
			if (createTask(TASK_FLAGS_LOW, (QWORD)testTask1) == NULL)
				break;
		}
		printf("Task1 %d created\n", i);
		break;
	case 2:
	default:
		for (i=0; i < atoi(count, 10); i++)
		{
			if (createTask(TASK_FLAGS_LOW, (QWORD)testTask2) == NULL)
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
		printf("[%d] task id[0x%q], priority[%d], flags[0x%q]\n",
			1 + count++, task->link.id, GET_PRIORITY(task->flags), task->flags);
	}
}

static void killTask(const char* params)
{
	PARAMETER_LIST paramList;
	char idBuf[30];
	QWORD id;

	initParameter(&paramList, params);
	getNextParameter(&paramList, idBuf);
	if (memcmp("0x", idBuf, 2) == 0)
		id = atoi(idBuf + 2, 16);
	else
		id = atoi(idBuf, 10);
	printf("kill task [0x%q] ", id);
	if (endTask(id))
		printf("success\n");
	else
		printf("fail\n");
}

static void cpuload(const char* params)
{
	printf("processor load: %d%%\n", getProcessorLoad());
}
