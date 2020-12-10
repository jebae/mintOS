#ifndef __INTERRUPTHANDLER_H__
#define __INTERRUPTHANDLER_H__

#include "Types.h"
#include "Utility.h"
#include "Task.h"
#include "Descriptor.h"

void commonExceptionHandler(int vecNum, QWORD errCode);
void commonInterruptHandler(int vecNum);
void keyboardHandler(int vecNum);
void timerHandler(int vecNum);
void deviceNotAvailableHandler(int vecNum);
void HDDhandler(int vecNum);

#endif
