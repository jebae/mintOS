#include "Types.h"

void printString(int iX, int iY, const char *pcString);

void Main(void)
{
	printString(0, 10, "Switch To IA-32e Mode Success");
	printString(0, 11, "IA-32e C Language Kernel Start.....[PASS]");
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
