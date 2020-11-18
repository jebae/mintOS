#include <stdarg.h>
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "AssemblyUtility.h"

CONSOLE_MANAGER gConsoleManager = {0, };

void initConsole(int x, int y)
{
	memset(&gConsoleManager, 0, sizeof(gConsoleManager));
	setCursor(x, y);
}

void setCursor(int x, int y)
{
	int linearValue = y * CONSOLE_WIDTH + x;

	outPortByte(VGA_PORT_INDEX, VGA_INDEX_UPPER_CURSOR);
	outPortByte(VGA_PORT_DATA, linearValue >> 8);
	outPortByte(VGA_PORT_INDEX, VGA_INDEX_LOWER_CURSOR);
	outPortByte(VGA_PORT_DATA, linearValue & 0xFF);
	gConsoleManager.currentPrintOffset = linearValue;
}

void getCursor(int *x, int *y)
{
	*x = gConsoleManager.currentPrintOffset % CONSOLE_WIDTH;
	*y = gConsoleManager.currentPrintOffset / CONSOLE_WIDTH;
}

void printf(const char* formatString, ...)
{
	va_list ap;
	char buf[1024];
	int nextPrintOffset;

	va_start(ap, formatString);
	vsprintf(buf, formatString, ap);
	va_end(ap);

	nextPrintOffset = consolePrintString(buf);
	setCursor(nextPrintOffset % CONSOLE_WIDTH, nextPrintOffset / CONSOLE_WIDTH);
}

int consolePrintString(const char* buf)
{
	CHARACTER* screen = (CHARACTER*)CONSOLE_VIDEO_MEMORY_ADDRESS;
	int i, j;
	int len;
	int printOffset = gConsoleManager.currentPrintOffset;

	len = strlen(buf);
	for (i=0; i < len; i++)
	{
		if (buf[i] == '\n')
			printOffset += CONSOLE_WIDTH - (printOffset % CONSOLE_WIDTH);
		else if (buf[i] == '\t')
			printOffset += 8 - (printOffset % 8);
		else
		{
			screen[printOffset].character = buf[i];
			screen[printOffset].attribute = CONSOLE_DEFAULT_TEXT_COLOR;
			printOffset++;
		}
		if (printOffset >= CONSOLE_WIDTH * CONSOLE_HEIGHT)
		{
			memcpy((void*)CONSOLE_VIDEO_MEMORY_ADDRESS,
				(void*)CONSOLE_VIDEO_MEMORY_ADDRESS + sizeof(CHARACTER) * CONSOLE_WIDTH,
				sizeof(CHARACTER) * CONSOLE_WIDTH * (CONSOLE_HEIGHT - 1));
			for (j=CONSOLE_WIDTH * (CONSOLE_HEIGHT - 1); j < CONSOLE_HEIGHT * CONSOLE_WIDTH; j++)
			{
				screen[j].character = ' ';
				screen[j].attribute = CONSOLE_DEFAULT_TEXT_COLOR;
			}
			printOffset = CONSOLE_WIDTH * (CONSOLE_HEIGHT - 1);
		}
	}
	return printOffset;
}

void clearScreen(void)
{
	CHARACTER* screen = (CHARACTER*)CONSOLE_VIDEO_MEMORY_ADDRESS;

	for (int i=0; i < CONSOLE_HEIGHT * CONSOLE_WIDTH; i++)
	{
		screen[i].character = ' ';
		screen[i].attribute = CONSOLE_DEFAULT_TEXT_COLOR;
	}
	setCursor(0, 0);
}

BYTE getch(void)
{
	KEY_DATA data;

	while (1)
	{
		if (!getKeyFromKeyQueue(&data))
			continue;
		if (data.flags & KEY_FLAGS_DOWN)
			return data.ASCIICode;
	}
}

void printStringXY(int x, int y, const char* str)
{
	CHARACTER* screen = (CHARACTER*)CONSOLE_VIDEO_MEMORY_ADDRESS + y * CONSOLE_WIDTH + x;
	int i;

	for (i=0; str[i] != '\0'; i++)
	{
		screen[i].character = str[i];
		screen[i].attribute = CONSOLE_DEFAULT_TEXT_COLOR;
	}
}
