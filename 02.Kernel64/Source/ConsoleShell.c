#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"

SHELL_COMMAND_ENTRY gCommandTable[] = {
	{"help", "show help", help},
	{"cls", "clear screen", cls},
	{"totalram", "show total RAM size", showTotalRAMSize},
	{"strtod", "string to decimal/hex convert", stringToDecimalHexTest},
	{"shutdown", "shutdown and reboot os", shutdown},
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

void help(const char* params)
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

void cls(const char* params)
{
	clearScreen();
	setCursor(0, 1);
}

void showTotalRAMSize(const char* params)
{
	printf("total RAM size = %dMB\n", getTotalRAMSize());
}

void stringToDecimalHexTest(const char* params)
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

void shutdown(const char* params)
{
	printf("System shutdown start...\n");
	printf("Press any key to reboot...");
	getch();
	reboot();
}
