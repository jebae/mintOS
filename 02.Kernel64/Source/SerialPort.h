#ifndef __SERIALPORT_H__
#define __SERIALPORT_H__

#include "Types.h"
#include "Queue.h"
#include "Synchronization.h"
#include "AssemblyUtility.h"

#define SERIAL_PORT_COM1																0x3F8
#define SERIAL_PORT_COM2																0x2F8
#define SERIAL_PORT_COM3																0x3E8
#define SERIAL_PORT_COM4																0x2E8

#define SERIAL_PORT_INDEX_RECEIVE_BUFFER								0x00
#define SERIAL_PORT_INDEX_TRANSMIT_BUFFER								0x00
#define SERIAL_PORT_INDEX_INTERRUPT_ENABLE							0x01
#define SERIAL_PORT_INDEX_DIVISOR_LATCH_LSB							0x00
#define SERIAL_PORT_INDEX_DIVISOR_LATCH_MSB							0x01
#define SERIAL_PORT_INDEX_INTERRUPT_IDENTIFICATION			0x02
#define SERIAL_PORT_INDEX_FIFO_CONTROL									0x02
#define SERIAL_PORT_INDEX_LINE_CONTROL									0x03
#define SERIAL_PORT_INDEX_MODEM_CONTROL									0x04
#define SERIAL_PORT_INDEX_LINE_STATUS										0x05
#define SERIAL_PORT_INDEX_MODEM_STATUS									0x06

#define SERIAL_INTERRUPT_ENABLE_RECEIVE_BUFFER_FULL			0x01
#define SERIAL_INTERRUPT_ENABLE_TRANSMIT_BUFFER_EMPTY		0x02
#define SERIAL_INTERRUPT_ENABLE_LINE_STATUS							0x04
#define SERIAL_INTERRUPT_ENABLE_DELTA_STATUS						0x08

#define SERIAL_FIFO_CONTROL_FIFO_ENABLE									0x01
#define SERIAL_FIFO_CONTROL_CLEAR_RECEIVE_FIFO					0x02
#define SERIAL_FIFO_CONTROL_CLEAR_TRANSMIT_FIFO					0x04
#define SERIAL_FIFO_CONTROL_ENABLE_DMA									0x08
#define SERIAL_FIFO_CONTROL_1BYTE_FIFO									0x00
#define SERIAL_FIFO_CONTROL_4BYTE_FIFO									0x40
#define SERIAL_FIFO_CONTROL_8BYTE_FIFO									0x80
#define SERIAL_FIFO_CONTROL_14BYTE_FIFO									0xC0

#define SERIAL_LINE_CONTROL_8BIT												0x03
#define SERIAL_LINE_CONTROL_1BIT_STOP										0x00
#define SERIAL_LINE_CONTROL_NO_PARITY										0x00
#define SERIAL_LINE_CONTROL_ODD_PARITY									0x08
#define SERIAL_LINE_CONTROL_EVEN_PARITY									0x18
#define SERIAL_LINE_CONTROL_MARK_PARITY									0x28
#define SERIAL_LINE_CONTROL_SPACE_PARITY								0x38
#define SERIAL_LINE_CONTROL_DLAB												0x80

#define SERIAL_LINE_STATUS_RECEIVED_DATA_READY					0x01
#define SERIAL_LINE_STATUS_OVERRUN_ERROR								0x02
#define SERIAL_LINE_STATUS_PARITY_ERROR									0x04
#define SERIAL_LINE_STATUS_FRAMING_ERROR								0x08
#define SERIAL_LINE_STATUS_BREAK_INDICATOR							0x10
#define SERIAL_LINE_STATUS_TRANSMIT_BUFFER_EMPTY				0x20
#define SERIAL_LINE_STATUS_TRANSMIT_EMPTY								0x40
#define SERIAL_LINE_STATUS_RECEIVED_CHARACTOR_ERROR			0x80

#define SERIAL_DIVISOR_LATCH_115200											1
#define SERIAL_DIVISOR_LATCH_57600											2
#define SERIAL_DIVISOR_LATCH_38400											3
#define SERIAL_DIVISOR_LATCH_19200											6
#define SERIAL_DIVISOR_LATCH_9600												12
#define SERIAL_DIVISOR_LATCH_4800												24
#define SERIAL_DIVISOR_LATCH_2400												48

#define SERIAL_FIFO_MAX_SIZE														16

typedef struct SerialPortManager
{
	MUTEX mutex;
} SERIAL_MANAGER;

void initSerialPort(void);
void sendSerialData(BYTE* buf, int size);
int receiveSerialData(BYTE* buf, int size);
void clearSerialFIFO(void);
static BOOL isSerialTransmitBufferEmpty(void);
static BOOL isSerialReceiveBufferFull(void);

#endif