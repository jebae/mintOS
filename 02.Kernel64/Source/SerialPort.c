#include "SerialPort.h"
#include "Utility.h"

static SERIAL_MANAGER gSerialManager;

void initSerialPort(void)
{
	WORD portBaseAddr;

	initMutex(&gSerialManager.mutex);
	portBaseAddr = SERIAL_PORT_COM1;

	outPortByte(portBaseAddr + SERIAL_PORT_INDEX_INTERRUPT_ENABLE, 0);

	outPortByte(portBaseAddr + SERIAL_PORT_INDEX_LINE_CONTROL,
			SERIAL_LINE_CONTROL_DLAB);
	outPortByte(portBaseAddr + SERIAL_PORT_INDEX_DIVISOR_LATCH_LSB,
			SERIAL_DIVISOR_LATCH_115200);
	outPortByte(portBaseAddr + SERIAL_PORT_INDEX_DIVISOR_LATCH_MSB,
			SERIAL_DIVISOR_LATCH_115200 >> 8);

	outPortByte(portBaseAddr + SERIAL_PORT_INDEX_LINE_CONTROL,
			SERIAL_LINE_CONTROL_8BIT | SERIAL_LINE_CONTROL_NO_PARITY |
			SERIAL_LINE_CONTROL_1BIT_STOP);
	outPortByte(portBaseAddr + SERIAL_PORT_INDEX_FIFO_CONTROL,
			SERIAL_FIFO_CONTROL_FIFO_ENABLE | SERIAL_FIFO_CONTROL_14BYTE_FIFO);
}

static BOOL isSerialTransmitBufferEmpty(void)
{
	BYTE data;

	data = inPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_LINE_STATUS);
	return (data & SERIAL_LINE_STATUS_TRANSMIT_BUFFER_EMPTY) == SERIAL_LINE_STATUS_TRANSMIT_BUFFER_EMPTY;
}

void sendSerialData(BYTE* buf, int size)
{
	int sentSize, tempSize;
	int i;

	lock(&gSerialManager.mutex);
	sentSize = 0;
	while (sentSize < size)
	{
		while (!isSerialTransmitBufferEmpty())
			sleep(0);
		tempSize = MIN(SERIAL_FIFO_MAX_SIZE, size - sentSize);
		for (i=0; i < tempSize; i++)
		{
			outPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_TRANSMIT_BUFFER,
					buf[sentSize + i]);
		}
		sentSize += tempSize;
	}
	unlock(&gSerialManager.mutex);
}

static BOOL isSerialReceiveBufferFull(void)
{
	BYTE data;

	data = inPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_LINE_STATUS);
	return (data & SERIAL_LINE_STATUS_RECEIVED_DATA_READY) == SERIAL_LINE_STATUS_RECEIVED_DATA_READY;
}

int receiveSerialData(BYTE* buf, int size)
{
	int i;

	lock(&gSerialManager.mutex);
	for (i=0; i < size; i++)
	{
		if (!isSerialReceiveBufferFull())
			break;
		buf[i] = inPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_RECEIVE_BUFFER);
	}
	unlock(&gSerialManager.mutex);
	return i;
}

void clearSerialFIFO(void)
{
	lock(&gSerialManager.mutex);
	outPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_FIFO_CONTROL,
			SERIAL_FIFO_CONTROL_FIFO_ENABLE | SERIAL_FIFO_CONTROL_14BYTE_FIFO |
			SERIAL_FIFO_CONTROL_CLEAR_RECEIVE_FIFO | SERIAL_FIFO_CONTROL_CLEAR_TRANSMIT_FIFO);
	unlock(&gSerialManager.mutex);
}
