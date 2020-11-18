#ifndef __UTILITY_H__
#define __UTILITY_H__

#include "Types.h"

void memSet(void* dest, BYTE data, int size);
int memCpy(void* dest, const void* src, int size);
int memCmp(const void* a, const void* b, int size);
BOOL setInterruptFlag(BOOL enableInterrupt);

#endif
