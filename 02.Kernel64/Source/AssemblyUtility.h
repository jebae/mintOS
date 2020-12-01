#ifndef __ASSEMBLY_UTILITY_H__
#define __ASSEMBLY_UTILITY_H__

#include "Types.h"
#include "Task.h"

BYTE inPortByte(WORD port);
void outPortByte(WORD port, BYTE data);
void loadGDTR(QWORD GDTRAddress);
void loadTR(WORD TSSSegmentOffset);
void loadIDTR(QWORD IDTRAddress);
void enableInterrupt(void);
void disableInterrupt(void);
QWORD readRFLAGS(void);
QWORD readTSC(void);
void switchContext(CONTEXT* current, CONTEXT* next);
void hlt(void);
BOOL checkLockAndSet(volatile BYTE* dest, BYTE compare, BYTE src);

#endif
