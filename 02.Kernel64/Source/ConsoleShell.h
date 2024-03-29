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
static void executeCommand(const char* command);
static void initParameter(PARAMETER_LIST* list, const char* parameter);
static void help(const char* params);
static void cls(const char* params);
static void showTotalRAMSize(const char* params);
static void stringToDecimalHexTest(const char* params);
static void shutdown(const char* params);
static void setTimer(const char* params);
static void waitUsingPIT(const char* params);
static void readTimestampCounter(const char* params);
static void measureProcessorSpeed(const char* params);
static void showDateAndTime(const char* params);
static void createTestTask(const char* params);
static void changeTaskPriority(const char* params);
static void showTaskList(const char* params);
static void killTask(const char* params);
static void cpuload(const char* params);
static void testMutex(const char* params);
static void testDeadlock(const char* params);
static void createThreadTask(void);
static void testThread(const char* params);
static void showMatrix(const char* params);
static void testPie(const char* params);
static void showDynamicMemoryInformation(const char* params);
static void testSequentialAllocation(const char* params);
static void randomAllocationTask(void);
static void testRandomAllocation(const char* params);
static void showHDDInformation(const char* params);
static void readSector(const char* params);
static void writeSector(const char* params);
static void mountHDD(const char* params);
static void formatHDD(const char* params);
static void showFileSystemInformation(const char* params);
static void createFileInRootDir(const char* params);
static void deleteFileInRootDir(const char* params);
static void showRootDir(const char* params);
static void writeDataToFile(const char* params);
static void readDataToFile(const char* params);
static void testFileIO(const char* params);
static void flushCache(const char* params);
static void testPerformance(const char* params);
static void downloadFile(const char* params);

#endif
