#ifndef __MODESWITCH_H__
#define __MODESWITCH_H__

#include "Types.h"

void readCPUID(DWORD eax, DWORD* pEAX, DWORD* pEBX, DWORD* pECX, DWORD* pEDX);
void switchAndExecute64bitKernel(void);

#endif
