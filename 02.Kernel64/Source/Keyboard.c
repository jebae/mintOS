#include "AssemblyUtility.h"
#include "Utility.h"
#include "Keyboard.h"
#include "Queue.h"
#include "Synchronization.h"

// check data sent from keyboard
BOOL isOutputBufferFull(void)
{
	// 0x64 means keyboard control register port
	if (inPortByte(0x64) & 0x01)
		return TRUE;
	return FALSE;
}

// check data sent from processor
BOOL isInputBufferFull(void)
{
	if (inPortByte(0x64) & 0x02)
		return TRUE;
	return FALSE;
}

BOOL waitForACKAndPutOtherScanCode(void)
{
	int i, j;
	BYTE data;
	BOOL ACKReceived = FALSE;

	for (j=0; j < 100; j++)
	{
		for (i=0; i < 0xFFFF; i++)
		{
			if (isOutputBufferFull())
				break;
		}
		data = inPortByte(0x60);
		if (data == 0xFA) // ACK
		{
			ACKReceived = TRUE;
			break;
		}
		else
		{
			convertScanCodeAndPutQueue(data);
		}
	}
	return ACKReceived;
}

BOOL activateKeyboard(void)
{
	int i, j;
	BOOL prevInterruptFlag;
	BOOL res;

	prevInterruptFlag = setInterruptFlag(FALSE);

	// 0xAE means activation command for keyboard controller
	outPortByte(0x64, 0xAE);

	// wait until input buffer empty to send activation command to keyboard
	for (i=0; i < 0xFFFF; i++)
	{
		if (isInputBufferFull() == FALSE)
			break;
	}

	/*
	 * 0x60 means IO register which save command or data
	 * between device(keyboard, mouse) and processor.
	 * 0xF4 means activation command for keyboard
	 */
	outPortByte(0x60, 0xF4);

	res = waitForACKAndPutOtherScanCode();
	setInterruptFlag(prevInterruptFlag);
	return res;
}

BYTE getKeyboardScanCode(void)
{
	while (isOutputBufferFull() == FALSE);
	return inPortByte(0x60);
}

BOOL changeKeyboardLED(BOOL capsLockOn, BOOL numLockOn, BOOL scrollLockOn)
{
	int i, j;
	BOOL prevInterruptFlag;
	BOOL res;
	BYTE data;

	prevInterruptFlag = setInterruptFlag(FALSE);

	for (i=0; i < 0xFFFF; i++)
	{
		if (isInputBufferFull() == FALSE)
			break;
	}

	// 0xED means LED status change command
	outPortByte(0x60, 0xED);

	for (i=0; i < 0xFFFF; i++)
	{
		if (isInputBufferFull() == FALSE)
			break;
	}

	res = waitForACKAndPutOtherScanCode();
	if (!res)
	{
		setInterruptFlag(prevInterruptFlag);
		return FALSE;
	}

	// change LED status
	outPortByte(0x60, (capsLockOn << 2) | (numLockOn << 1) | scrollLockOn);
	for (i=0; i < 0xFFFF; i++)
	{
		if (isInputBufferFull() == FALSE)
			break;
	}
	res = waitForACKAndPutOtherScanCode();
	setInterruptFlag(prevInterruptFlag);
	return res;
}

void enableA20Gate(void)
{
	BYTE outputPortData;
	int i;

	// 0xD0 command means copy keyboard controller output port to buffer (0x60)
	outPortByte(0x64, 0xD0);
	for (i=0; i < 0xFFFF; i++)
	{
		if (isOutputBufferFull() == TRUE)
			break;
	}
	outputPortData = inPortByte(0x60);
	outputPortData |= 0x01;

	for (i=0; i < 0xFFFF; i++)
	{
		if (isInputBufferFull() == FALSE)
			break;
	}

	// 0xD1 command means copy buffer data to keyboard controller output port
	outPortByte(0x64, 0xD1);
	outPortByte(0x60, outputPortData);
}

void reboot(void)
{
	int i;

	for (i=0; i < 0xFFFF; i++)
	{
		if (isInputBufferFull() == FALSE)
			break;
	}
	outPortByte(0x64, 0xD1);

	// reset processor
	outPortByte(0x60, 0x00);
	while (1);
}

static QUEUE gKeyQueue;
static KEY_DATA gKeyQueueBuffer[KEY_MAX_QUEUE_COUNT];

static KEYBOARD_MANAGER gKeyboardManager = {0, };

// below mapping table always use key down code as index
static KEY_MAPPING_ENTRY gKeyMappingTable[KEY_MAPPING_TABLE_MAX_COUNT] = {
	/*  0   */  {   KEY_NONE        ,   KEY_NONE        },
	/*  1   */  {   KEY_ESC         ,   KEY_ESC         },
	/*  2   */  {   '1'             ,   '!'             },
	/*  3   */  {   '2'             ,   '@'             },
	/*  4   */  {   '3'             ,   '#'             },
	/*  5   */  {   '4'             ,   '$'             },
	/*  6   */  {   '5'             ,   '%'             },
	/*  7   */  {   '6'             ,   '^'             },
	/*  8   */  {   '7'             ,   '&'             },
	/*  9   */  {   '8'             ,   '*'             },
	/*  10  */  {   '9'             ,   '('             },
	/*  11  */  {   '0'             ,   ')'             },
	/*  12  */  {   '-'             ,   '_'             },
	/*  13  */  {   '='             ,   '+'             },
	/*  14  */  {   KEY_BACKSPACE   ,   KEY_BACKSPACE   },
	/*  15  */  {   KEY_TAB         ,   KEY_TAB         },
	/*  16  */  {   'q'             ,   'Q'             },
	/*  17  */  {   'w'             ,   'W'             },
	/*  18  */  {   'e'             ,   'E'             },
	/*  19  */  {   'r'             ,   'R'             },
	/*  20  */  {   't'             ,   'T'             },
	/*  21  */  {   'y'             ,   'Y'             },
	/*  22  */  {   'u'             ,   'U'             },
	/*  23  */  {   'i'             ,   'I'             },
	/*  24  */  {   'o'             ,   'O'             },
	/*  25  */  {   'p'             ,   'P'             },
	/*  26  */  {   '['             ,   '{'             },
	/*  27  */  {   ']'             ,   '}'             },
	/*  28  */  {   '\n'            ,   '\n'            },
	/*  29  */  {   KEY_CTRL        ,   KEY_CTRL        },
	/*  30  */  {   'a'             ,   'A'             },
	/*  31  */  {   's'             ,   'S'             },
	/*  32  */  {   'd'             ,   'D'             },
	/*  33  */  {   'f'             ,   'F'             },
	/*  34  */  {   'g'             ,   'G'             },
	/*  35  */  {   'h'             ,   'H'             },
	/*  36  */  {   'j'             ,   'J'             },
	/*  37  */  {   'k'             ,   'K'             },
	/*  38  */  {   'l'             ,   'L'             },
	/*  39  */  {   ';'             ,   ':'             },
	/*  40  */  {   '\''            ,   '\"'            },
	/*  41  */  {   '`'             ,   '~'             },
	/*  42  */  {   KEY_LSHIFT      ,   KEY_LSHIFT      },
	/*  43  */  {   '\\'            ,   '|'             },
	/*  44  */  {   'z'             ,   'Z'             },
	/*  45  */  {   'x'             ,   'X'             },
	/*  46  */  {   'c'             ,   'C'             },
	/*  47  */  {   'v'             ,   'V'             },
	/*  48  */  {   'b'             ,   'B'             },
	/*  49  */  {   'n'             ,   'N'             },
	/*  50  */  {   'm'             ,   'M'             },
	/*  51  */  {   ','             ,   '<'             },
	/*  52  */  {   '.'             ,   '>'             },
	/*  53  */  {   '/'             ,   '?'             },
	/*  54  */  {   KEY_RSHIFT      ,   KEY_RSHIFT      },
	/*  55  */  {   '*'             ,   '*'             },
	/*  56  */  {   KEY_LALT        ,   KEY_LALT        },
	/*  57  */  {   ' '             ,   ' '             },
	/*  58  */  {   KEY_CAPSLOCK    ,   KEY_CAPSLOCK    },
	/*  59  */  {   KEY_F1          ,   KEY_F1          },
	/*  60  */  {   KEY_F2          ,   KEY_F2          },
	/*  61  */  {   KEY_F3          ,   KEY_F3          },
	/*  62  */  {   KEY_F4          ,   KEY_F4          },
	/*  63  */  {   KEY_F5          ,   KEY_F5          },
	/*  64  */  {   KEY_F6          ,   KEY_F6          },
	/*  65  */  {   KEY_F7          ,   KEY_F7          },
	/*  66  */  {   KEY_F8          ,   KEY_F8          },
	/*  67  */  {   KEY_F9          ,   KEY_F9          },
	/*  68  */  {   KEY_F10         ,   KEY_F10         },
	/*  69  */  {   KEY_NUMLOCK     ,   KEY_NUMLOCK     },
	/*  70  */  {   KEY_SCROLLLOCK  ,   KEY_SCROLLLOCK  },

	/*  71  */  {   KEY_HOME        ,   '7'             },
	/*  72  */  {   KEY_UP          ,   '8'             },
	/*  73  */  {   KEY_PAGEUP      ,   '9'             },
	/*  74  */  {   '-'             ,   '-'             },
	/*  75  */  {   KEY_LEFT        ,   '4'             },
	/*  76  */  {   KEY_CENTER      ,   '5'             },
	/*  77  */  {   KEY_RIGHT       ,   '6'             },
	/*  78  */  {   '+'             ,   '+'             },
	/*  79  */  {   KEY_END         ,   '1'             },
	/*  80  */  {   KEY_DOWN        ,   '2'             },
	/*  81  */  {   KEY_PAGEDOWN    ,   '3'             },
	/*  82  */  {   KEY_INS         ,   '0'             },
	/*  83  */  {   KEY_DEL         ,   '.'             },
	/*  84  */  {   KEY_NONE        ,   KEY_NONE        },
	/*  85  */  {   KEY_NONE        ,   KEY_NONE        },
	/*  86  */  {   KEY_NONE        ,   KEY_NONE        },
	/*  87  */  {   KEY_F11         ,   KEY_F11         },
	/*  88  */  {   KEY_F12         ,   KEY_F12         }
};

BOOL initKeyboard(void)
{
	initQueue(&gKeyQueue, gKeyQueueBuffer, KEY_MAX_QUEUE_COUNT, sizeof(KEY_DATA));
	return activateKeyboard();
}

BOOL convertScanCodeAndPutQueue(BYTE scanCode)
{
	KEY_DATA data;
	BOOL res = FALSE;
	BOOL prevInterruptFlag;

	data.scanCode = scanCode;
	if (convertScanCodeToASCIICode(scanCode, &data.ASCIICode, &data.flags))
	{
		prevInterruptFlag = lockForSystemData();
		res = putQueue(&gKeyQueue, &data);
		unlockForSystemData(prevInterruptFlag);
	}
	return res;
}

BOOL getKeyFromKeyQueue(KEY_DATA* data)
{
	BOOL res;
	BOOL prevInterruptFlag;

	if (isQueueEmpty(&gKeyQueue))
		return FALSE;
	prevInterruptFlag = lockForSystemData();
	res = getQueue(&gKeyQueue, data);
	unlockForSystemData(prevInterruptFlag);
	return res;
}

BOOL isAlphabetScanCode(BYTE scanCode)
{
	if ('a' <= gKeyMappingTable[scanCode].normalCode &&
		gKeyMappingTable[scanCode].normalCode <= 'z')
		return TRUE;
	return FALSE;
}

BOOL isNumberOrSymbolScanCode(BYTE scanCode)
{
	if (2 <= scanCode && scanCode <= 53 && isAlphabetScanCode(scanCode) == FALSE)
		return TRUE;
	return FALSE;
}

BOOL isNumberPadScanCode(BYTE scanCode)
{
	if (71 <= scanCode && scanCode <= 83)
		return TRUE;
	return FALSE;
}

BOOL isUseCombinedCode(BYTE scanCode)
{
	BYTE downScanCode;
	BOOL useCombinedKey = FALSE;

	downScanCode = scanCode & 0x7F;

	if (isAlphabetScanCode(downScanCode))
	{
		if (gKeyboardManager.shiftDown ^ gKeyboardManager.capsLockOn)
		{
			useCombinedKey = TRUE;
		}
	}
	else if (isNumberOrSymbolScanCode(downScanCode))
	{
		if (gKeyboardManager.shiftDown)
		{
			useCombinedKey = TRUE;
		}
	}
	else if (isNumberPadScanCode(downScanCode))
	{
		if (gKeyboardManager.numLockOn)
		{
			useCombinedKey = TRUE;
		}
	}
	return useCombinedKey;
}

void updateCombinationKeyStatusAndLED(BYTE scanCode)
{
	BOOL down;
	BYTE downScanCode;
	BOOL LEDStatusChanged = FALSE;

	// keydown | 0x80 = keyup
	down = scanCode & 0x80 ? FALSE : TRUE;
	downScanCode = scanCode & 0x7F;

	// case shift
	if (downScanCode == 42 || downScanCode == 54)
	{
		gKeyboardManager.shiftDown = down;
	}
	// case caps lock
	else if (downScanCode == 58 && down)
	{
		gKeyboardManager.capsLockOn ^= TRUE;
		LEDStatusChanged = TRUE;
	}
	// case num lock
	else if (downScanCode == 69 && down)
	{
		gKeyboardManager.numLockOn ^= TRUE;
		LEDStatusChanged = TRUE;
	}
	// case scroll lock
	else if (downScanCode == 70 && down)
	{
		gKeyboardManager.scrollLockOn ^= TRUE;
		LEDStatusChanged = TRUE;
	}

	LEDStatusChanged && changeKeyboardLED(gKeyboardManager.capsLockOn,
		gKeyboardManager.numLockOn, gKeyboardManager.scrollLockOn);
}

BOOL convertScanCodeToASCIICode(BYTE scanCode, BYTE* ASCIICode, BOOL* flags)
{
	BOOL useCombinedKey;

	// if pause, ignore 2 trailing scan codes
	if (gKeyboardManager.skipCountForPause > 0)
	{
		gKeyboardManager.skipCountForPause--;
		return FALSE;
	}

	// case pause
	if (scanCode == 0xE1)
	{
		*ASCIICode = KEY_PAUSE;
		*flags = KEY_FLAGS_DOWN;
		gKeyboardManager.skipCountForPause = KEY_SKIP_COUNT_FOR_PAUSE;
		return TRUE;
	}
	// case extended key
	else if (scanCode == 0xE0)
	{
		gKeyboardManager.extendedCodeIn = TRUE;
		return FALSE;
	}

	useCombinedKey = isUseCombinedCode(scanCode);

	*ASCIICode = useCombinedKey
		? gKeyMappingTable[scanCode & 0x7F].combinedCode
		: gKeyMappingTable[scanCode & 0x7F].normalCode;

	if (gKeyboardManager.extendedCodeIn == TRUE)
	{
		*flags = KEY_FLAGS_EXTENDED_KEY;
		gKeyboardManager.extendedCodeIn = FALSE;
	}
	else
	{
		*flags = 0;
	}

	if ((scanCode & 0x80) == 0)
	{
		*flags |= KEY_FLAGS_DOWN;
	}

	updateCombinationKeyStatusAndLED(scanCode);
	return TRUE;
}
