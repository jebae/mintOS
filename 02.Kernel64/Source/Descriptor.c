#include "Descriptor.h"
#include "Utility.h"
#include "ISR.h"

void initGDTTableAndTSS(void)
{
	GDTR* gdtr;
	GDT_ENTRY8* entry;
	TSS_SEGMENT* TSS;
	int i;

	// set GDTR
	gdtr = (GDTR*)GDTR_START_ADDRESS;
	entry = (GDT_ENTRY8*)(GDTR_START_ADDRESS + sizeof(GDTR));
	gdtr->limit = GDT_TABLE_SIZE - 1;
	gdtr->baseAddress = (QWORD)entry;

	// set TSS
	TSS = (TSS_SEGMENT*)((QWORD)entry + GDT_TABLE_SIZE);

	// null descriptor
	setGDTEntry8(&entry[0], 0, 0, 0, 0, 0);

	// code segment descriptor
	setGDTEntry8(&entry[1], 0, 0xFFFFF,
		GDT_FLAGS_UPPER_CODE, GDT_FLAGS_LOWER_KERNEL_CODE, GDT_TYPE_CODE);

	// data segment descriptor
	setGDTEntry8(&entry[2], 0, 0xFFFFF,
		GDT_FLAGS_UPPER_DATA, GDT_FLAGS_LOWER_KERNEL_DATA, GDT_TYPE_DATA);

	// TSS segment descriptor
	setGDTEntry16((GDT_ENTRY16*)&entry[3], (QWORD)TSS, sizeof(TSS_SEGMENT) - 1,
		GDT_FLAGS_UPPER_TSS, GDT_FLAGS_LOWER_TSS, GDT_TYPE_TSS);

	initTSSSegment(TSS);
}

void setGDTEntry8(GDT_ENTRY8* entry, DWORD baseAddress, DWORD limit,
	BYTE upperFlags, BYTE lowerFlags, BYTE type)
{
	entry->lowerLimit = limit & 0xFFFF;
	entry->lowerBaseAddress = baseAddress & 0xFFFF;
	entry->upperBaseAddress1 = (baseAddress >> 16) & 0xFF;
	entry->typeAndLowerFlag = type | lowerFlags;
	entry->upperLimitAndUpperFlag = ((limit >> 16) & 0x0F) | upperFlags;
	entry->upperBaseAddress2 = (baseAddress >> 24) & 0xFF;
}

void setGDTEntry16(GDT_ENTRY16* entry, QWORD baseAddress, DWORD limit,
	BYTE upperFlags, BYTE lowerFlags, BYTE type)
{
	entry->lowerLimit = limit & 0xFFFF;
	entry->lowerBaseAddress = baseAddress & 0xFFFF;
	entry->middleBaseAddress1 = (baseAddress >> 16) & 0xFF;
	entry->typeAndLowerFlag = type | lowerFlags;
	entry->upperLimitAndUpperFlag = ((limit >> 16) & 0x0F) | upperFlags;
	entry->middleBaseAddress2 = (baseAddress >> 24) & 0xFF;
	entry->upperBaseAddress = baseAddress >> 32;
	entry->reserved = 0;
}

void initTSSSegment(TSS_SEGMENT* TSS)
{
	memset(TSS, 0, sizeof(TSS_SEGMENT));
	TSS->IST[0] = IST_START_ADDRESS + IST_SIZE;
	TSS->IOMapBaseAddress = 0xFFFF; // disable blocking IO access from application
}

void initIDTTables(void)
{
	IDTR* idtr;
	IDT_ENTRY* entry;
	int i;

	idtr = (IDTR*)IDTR_START_ADDRESS;
	entry = (IDT_ENTRY*)(IDTR_START_ADDRESS + sizeof(IDTR));
	idtr->baseAddress = (QWORD)entry;
	idtr->limit = IDT_TABLE_SIZE - 1;

	setIDTEntry(&entry[0], ISRDivideError, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[1], ISRDebug, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[2], ISRNMI, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[3], ISRBreakPoint, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[4], ISROverflow, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[5], ISRBoundRangeExceeded, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[6], ISRInvalidOpcode, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[7], ISRDeviceNotAvailable, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[8], ISRDoubleFault, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[9], ISRCoprocessorSegmentOverrun, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[10], ISRInvalidTSS, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[11], ISRSegmentNotPresent, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[12], ISRStackSegmentFault, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[13], ISRGeneralProtection, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[14], ISRPageFault, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[15], ISR15, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[16], ISRFPUError, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[17], ISRAlignmentCheck, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[18], ISRMachineCheck, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[19], ISRSIMDError, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[20], ISRETCException, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);

	for (i=21; i < 32; i++)
	{
		setIDTEntry(&entry[i], ISRETCException, 0x08, IDT_FLAGS_IST1,
			IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	}

	setIDTEntry(&entry[32], ISRTimer, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[33], ISRKeyboard, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[34], ISRSlavePIC, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[35], ISRSerial2, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[36], ISRSerial1, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[37], ISRParallel2, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[38], ISRFloppy, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[39], ISRParallel1, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[40], ISRRTC, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[41], ISRReserved, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[42], ISRNotUsed1, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[43], ISRNotUsed2, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[44], ISRMouse, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[45], ISRCoprocessor, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[46], ISRHDD1, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	setIDTEntry(&entry[47], ISRHDD2, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);

	for (i=48; i < IDT_MAX_ENTRY_COUNT; i++)
	{
		setIDTEntry(&entry[i], ISRETCInterrupt, 0x08, IDT_FLAGS_IST1,
			IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	}
}

void setIDTEntry(IDT_ENTRY* entry, void* handler, WORD selector,
	BYTE IST, BYTE flags, BYTE type)
{
	entry->lowerBaseAddress = (QWORD)handler & 0xFFFF;
	entry->segmentSelector = selector;
	entry->IST = IST & 0x3;
	entry->typeAndFlags = type | flags;
	entry->middleBaseAddress = ((QWORD)handler >> 16) & 0xFFFF;
	entry->upperBaseAddress = (QWORD)handler >> 32;
	entry->reserved = 0;
}
