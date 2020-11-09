#include "Types.h"
#include "Page.h"
#include "ModeSwitch.h"

void printString(int iX, int iY, const char *pcString);
BOOL initKernel64Area(void);
BOOL isMemoryEnough(void);
void copyKernel64ImageTo2MB(void);

void Main(void)
{
	DWORD i;
	DWORD eax, ebx, ecx, edx;
	char vendor[13] = {0,};

	printString(0, 3, "Protected Mode C Language Kernel Start...........[PASS]");

	printString(0, 4, "Minimum Memory Size Check.....[    ]");
	if (isMemoryEnough())
	{
		printString(31, 4, "PASS");
	}
	else
	{
		printString(31, 4, "FAIL");
		printString(0, 5, "Not Enough Memory. mintOS Requires Over 64MB Memory");
		while (1);
	}

	printString(0, 5, "IA-32e Kernel Area Initialize.....[    ]");
	if (initKernel64Area())
	{
		printString(35, 5, "PASS");
	}
	else
	{
		printString(35, 5, "FAIL");
		printString(0, 6, "Kernel Area Initialization Fail");
		while (1);
	}

	printString(0, 6, "IA-32e Page Tables Initialize.....[    ]");
	initPageTables();
	printString(35, 6, "PASS");

	readCPUID(0x00, &eax, &ebx, &ecx, &edx);
	*(DWORD*)vendor = ebx;
	*((DWORD*)vendor + 1) = edx;
	*((DWORD*)vendor + 2) = ecx;
	printString(0, 7, "Processor Vendor String.....[            ]");
	printString(29, 7, vendor);

	readCPUID(0x80000001, &eax, &ebx, &ecx, &edx);
	printString(0, 8, "64bit Mode Support Check.....[    ]");
	if (edx & (1 << 29))
	{
		printString(30, 8, "PASS");
	}
	else
	{
		printString(30, 8, "FAIL");
		printString(0, 9, "This processor does not support 64bit mode");
		while (1);
	}

	printString(0, 9, "Copy IA-32e Kernel To 2MB Address.....[    ]");
	copyKernel64ImageTo2MB();
	printString(39, 9, "PASS");

	printString(0, 10, "Switch To IA-32e Mode");
	switchAndExecute64bitKernel();
	while (1);
}

void printString(int iX, int iY, const char *pcString)
{
	CHARACTER* pstScreen = (CHARACTER*) 0xB8000;
	int i;

	pstScreen += (iY * 80) + iX;
	for (i=0; pcString[i] != 0; i++)
	{
		pstScreen[i].bCharacter = pcString[i];
	}
}

BOOL initKernel64Area()
{
	DWORD* dwCurrentAddress = (DWORD*)0x100000;

	while ((DWORD)dwCurrentAddress < 0x600000)
	{
		*dwCurrentAddress = 0x00000000;
		if (*dwCurrentAddress != 0)
			return FALSE;
		dwCurrentAddress++;
	}
	return TRUE;
}

BOOL isMemoryEnough()
{
	DWORD* dwCurrentAddress = (DWORD*)0x100000;

	while ((DWORD)dwCurrentAddress < 0x4000000)
	{
		*dwCurrentAddress = 0x12345678;
		if (*dwCurrentAddress != 0x12345678)
			return FALSE;
		dwCurrentAddress += 0x100000;
	}
	return TRUE;
}

void copyKernel64ImageTo2MB(void)
{
	WORD kernel32SectorCount, totalKernelSectorCount;
	DWORD* srcAddress;
	DWORD* destAddress;
	int i;

	// 0x7C05 means bootLoader address (0x7C00) + TOTAL_SECTOR_COUNT address (0x05)
	totalKernelSectorCount = *((WORD*)0x7C05);

	// 0x7C07 means bootLoader address (0x7C00) + KERNEL32_SECTOR_COUNT address (0x07)
	kernel32SectorCount = *((WORD*)0x7C07);

	srcAddress = (DWORD*)(0x10000 + (kernel32SectorCount * 512));
	destAddress = (DWORD*)0x200000;

	for (i=0; i < 512 * (totalKernelSectorCount - kernel32SectorCount) / 4; i++)
	{
		*destAddress = *srcAddress;
		srcAddress++;
		destAddress++;
	}
}
