#include "PIC.h"
#include "AssemblyUtility.h"

void initPIC(void)
{
	// master
	outPortByte(PIC_MASTER_PORT1, 0x11); // ICW1: IC4 bit(0)=1
	outPortByte(PIC_MASTER_PORT2, PIC_IRQ_START_VECTOR); // ICW2: offset IRQ vector
	outPortByte(PIC_MASTER_PORT2, 0x04); // ICW3: slave pin number=2 (by bit)
	outPortByte(PIC_MASTER_PORT2, 0x01); // ICW4: uPM bit(0)=1

	// slave
	outPortByte(PIC_SLAVE_PORT1, 0x11); // ICW1: IC4 bit(0)=1
	outPortByte(PIC_SLAVE_PORT2, PIC_IRQ_START_VECTOR + 8); // ICW2: offset IRQ vector after master
	outPortByte(PIC_SLAVE_PORT2, 0x02); // ICW3: slave pin number=2 (by num)
	outPortByte(PIC_SLAVE_PORT2, 0x01); // ICW4: uPM bit(0)=1
}

// ignore interrupt by bit mask
void maskPICInterrupt(WORD IRQBitMask)
{
	// OCW1, IRQ 0~7
	outPortByte(PIC_MASTER_PORT2, (BYTE)IRQBitMask);

	// OCW1, IRQ 8~15
	outPortByte(PIC_SLAVE_PORT2, (BYTE)(IRQBitMask >> 8));
}

// send interrupt handling completed
void sendEOIToPIC(int IRQNum)
{
	// OCW2, EOI bit(5)=1
	outPortByte(PIC_MASTER_PORT1, 0x20);
	if (IRQNum >= 8)
	{
		outPortByte(PIC_SLAVE_PORT1, 0x20);
	}
}
