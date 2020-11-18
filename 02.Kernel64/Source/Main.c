#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "PIC.h"

void printString(int iX, int iY, const char *pcString);

void Main(void)
{
	char temp[2] = {0, };
	int i = 0;
	KEY_DATA keyData;

	printString(0, 10, "Switch To IA-32e Mode Success");
	printString(0, 11, "IA-32e C Language Kernel Start.....[PASS]");

	printString(0, 12, "init GDT And Switch For IA-32e Mode.....[    ]");
	initGDTTableAndTSS();
	loadGDTR(GDTR_START_ADDRESS);
	printString(41, 12, "PASS");

	printString(0, 13, "TSS Segment Load.....[    ]");
	loadTR(GDT_TSS_SEGMENT);
	printString(22, 13, "PASS");

	printString(0, 14, "init IDT.....[    ]");
	initIDTTables();
	loadIDTR(IDTR_START_ADDRESS);
	printString(14, 14, "PASS");

	printString(0, 15, "Keyboard Activate And Queue Initialize.....[    ]");
	if (initKeyboard())
	{
		printString(44, 15, "PASS");
		changeKeyboardLED(FALSE, FALSE, FALSE);
	}
	else
	{
		printString(44, 15, "FAIL");
		while (1);
	}

	printString(0, 16, "PIC Controller And Interrupt Initialize.....[    ]");
	initPIC();
	maskPICInterrupt(0);
	enableInterrupt();
	printString(45, 16, "PASS");
	while (1)
	{
		if (!getKeyFromKeyQueue(&keyData))
			continue;
		if (keyData.flags & KEY_FLAGS_DOWN)
		{
			temp[0] = keyData.ASCIICode;
			printString(i++, 17, temp);
		}
		if (temp[0] == '0')
			temp[0] /= 0;
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
