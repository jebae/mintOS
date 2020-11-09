#include "Types.h"
#include "Keyboard.h"

void printString(int iX, int iY, const char *pcString);

void Main(void)
{
	char temp[2] = {0, };
	BYTE flags;
	BYTE scanCode;
	int i = 0;

	printString(0, 10, "Switch To IA-32e Mode Success");
	printString(0, 11, "IA-32e C Language Kernel Start.....[PASS]");
	printString(0, 12, "Keyboard Activate.....[    ]");

	if (activateKeyboard())
	{
		printString(23, 12, "PASS");
		changeKeyboardLED(FALSE, FALSE, FALSE);
	}
	else
	{
		printString(23, 12, "FAIL");
		while (1);
	}
	while (1)
	{
		if (!isOutputBufferFull())
			continue;
		scanCode = getKeybardScanCode();
		if (!convertScanCodeToASCIICode(scanCode, &temp[0], &flags))
			continue;
		if (flags & KEY_FLAGS_DOWN)
			printString(i++, 13, temp);
	}
}

void printString(int iX, int iY, const char *pcString)
{
	CHARACTER* pstScreen = (CHARACTER*)0xB8000;
	int i;

	pstScreen += (iY * 80) + iX;
	for (i=0; pcString[i] != 0; i++)
	{
		pstScreen[i].bCharacter = pcString[i];
	}
}
