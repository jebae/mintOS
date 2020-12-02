#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <stdarg.h>
#include "Types.h"

extern volatile QWORD gTickCount;

void memset(void* dest, BYTE data, int size);
int memcpy(void* dest, const void* src, int size);
int memcmp(const void* a, const void* b, int size);
int strlen(const char* buf);
BOOL setInterruptFlag(BOOL enableInterrupt);
void checkTotalRAMSize(void);
QWORD getTotalRAMSize(void);
void reverseString(char* buf);
long atoi(const char* buf, int radix);
QWORD hexStringToQword(const char* buf);
long decimalStringToLong(const char* buf);
int itoa(long value, char* buf, int radix);
int hexToString(QWORD value, char* buf);
int decimalToString(long value, char* buf);
int sprintf(char* buf, const char* formatString, ...);
int vsprintf(char* buf, const char* formatString, va_list ap);
QWORD getTickCount(void);
void sleep(QWORD millisecond);

#endif
