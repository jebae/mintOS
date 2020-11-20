#ifndef __CONSOLE_SHELL_H__
#define __CONSOLE_SHELL_H__

#include "Types.h"

#define CONSOLE_SHELL_MAX_COMMAND_BUFFER_COUNT		300
#define CONSOLE_SHELL_PROMPT_MESSAGE							"mintOS>"

typedef void (*CommandFunction)(const char* params);

#pragma pack(push, 1)

typedef struct ShellCommandEntryStruct
{
	char* command;
	char* help;
	CommandFunction func;
} SHELL_COMMAND_ENTRY;

typedef struct ParameterListStruct
{
	const char* buf;
	int len;
	int pos;
} PARAMETER_LIST;

#pragma pack(pop)

void startConsoleShell(void);
void executeCommand(const char* command);
void initParameter(PARAMETER_LIST* list, const char* parameter);
void help(const char* params);
void cls(const char* params);
void showTotalRAMSize(const char* params);
void stringToDecimalHexTest(const char* params);
void shutdown(const char* params);
void setTimer(const char* params);
void waitUsingPIT(const char* params);
void readTimestampCounter(const char* params);
void measureProcessorSpeed(const char* params);
void showDateAndTime(const char* params);

#endif
