#ifndef __PIC_H__
#define __PIC_H__

#include "Types.h"

#define PIC_MASTER_PORT1			0x20
#define PIC_MASTER_PORT2			0x21
#define PIC_SLAVE_PORT1				0xA0
#define PIC_SLAVE_PORT2				0xA1
#define PIC_IRQ_START_VECTOR	0x20 // 0~31 IRQ is reserved by processor

void initPIC(void);
void maskPICInterrupt(WORD IRQBitMask);
void sendEOIToPIC(int IRQNum);

#endif
