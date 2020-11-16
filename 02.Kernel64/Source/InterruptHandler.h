#ifndef __INTERRUPTHANDLER_H__
#define __INTERRUPTHANDLER_H__

#include "Types.h"

void commonExceptionHandler(int vecNum, QWORD errCode);
void commonInterruptHandler(int vecNum);
void keyboardHandler(int vecNum);

#endif
