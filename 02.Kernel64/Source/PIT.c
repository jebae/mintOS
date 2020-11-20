#include "PIT.h"
#include "AssemblyUtility.h"
#include "Console.h"

void initPIT(WORD count, BOOL isPeriodic)
{
	// init PIT control register (0x43) to stop counter
	outPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_ONCE);
	if (isPeriodic)
		outPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_PERIODIC);
	outPortByte(PIT_PORT_COUNTER0, count); // LSB (least significant)
	outPortByte(PIT_PORT_COUNTER0, count >> 8); // MSB (most significant)
}

WORD readCounter0(void)
{
	BYTE highByte, lowByte;
	WORD res;

	outPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_LATCH);
	lowByte = inPortByte(PIT_PORT_COUNTER0);
	highByte = inPortByte(PIT_PORT_COUNTER0);
	res = highByte;
	res = (res << 8) | lowByte;
	return res;
}

// maximum 50ms (count available
void waitUsingDirectPIT(WORD count)
{
	WORD lastCounter0;
	WORD currentCounter0;

	initPIT(0, TRUE);
	lastCounter0 = readCounter0();
	while (1)
	{
		currentCounter0 = readCounter0();
		if (((lastCounter0 - currentCounter0) & 0xFFFF) >= count)
			break;
	}
}
