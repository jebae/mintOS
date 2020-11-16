#ifndef __DESCRIPTOR_H__
#define __DESCRIPTOR_H__

#include "Types.h"

// GDT
#define GDT_TYPE_CODE						0x0A // type=Execute/Read
#define GDT_TYPE_DATA						0x02 // type=Read/Write
#define GDT_TYPE_TSS						0x09 // type-Execute only,Accessed
#define GDT_FLAGS_LOWER_S				0x10
#define GDT_FLAGS_LOWER_DPL0		0x00
#define GDT_FLAGS_LOWER_DPL1		0x20
#define GDT_FLAGS_LOWER_DPL2		0x40
#define GDT_FLAGS_LOWER_DPL3		0x60
#define GDT_FLAGS_LOWER_P				0x80
#define GDT_FLAGS_UPPER_L				0x20
#define GDT_FLAGS_UPPER_DB			0x40
#define GDT_FLAGS_UPPER_G				0x80

#define GDT_FLAGS_LOWER_KERNEL_CODE (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S |\
		GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)

#define GDT_FLAGS_LOWER_KERNEL_DATA (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S |\
		GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)

#define GDT_FLAGS_LOWER_TSS (GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)

#define GDT_FLAGS_LOWER_USER_CODE (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S |\
		GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)

#define GDT_FLAGS_LOWER_USER_DATA (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S |\
		GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)

#define GDT_FLAGS_UPPER_CODE (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_DATA (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_TSS (GDT_FLAGS_UPPER_G)

#define GDT_KERNEL_CODE_SEGMENT	0x08
#define GDT_KERNEL_DATA_SEGMENT	0x10
#define GDT_TSS_SEGMENT					0x18

#define GDTR_START_ADDRESS			0x142000 // After 1MB + 264KB (page table)
#define GDT_MAX_ENTRY8_COUNT		3 // count of 8byte entry (null, kernel code, kernel data)
#define GDT_MAX_ENTRY16_COUNT		1 // count of 16byte entry (TSS)
#define GDT_TABLE_SIZE ((sizeof(GDT_ENTRY8) * GDT_MAX_ENTRY8_COUNT) +\
		(sizeof(GDT_ENTRY16) * GDT_MAX_ENTRY16_COUNT))

// IDT
#define IDT_TYPE_INTERRUPT			0x0E
#define IDT_TYPE_TRAP						0x0F
#define IDT_FLAGS_DPL0					0x00
#define IDT_FLAGS_DPL1					0x20
#define IDT_FLAGS_DPL2					0x40
#define IDT_FLAGS_DPL3					0x60
#define IDT_FLAGS_P							0x80
#define IDT_FLAGS_IST0					0
#define IDT_FLAGS_IST1					1

#define IDT_FLAGS_KERNEL (IDT_FLAGS_DPL0 | IDT_FLAGS_P)
#define IDT_FLAGS_USER (IDT_FLAGS_DPL3 | IDT_FLAGS_P)

#define IDT_MAX_ENTRY_COUNT			100
#define IDTR_START_ADDRESS (GDTR_START_ADDRESS + sizeof(GDTR) +\
		GDT_TABLE_SIZE + sizeof(TSS_SEGMENT))

#define IDT_START_ADDRESS (IDTR_START_ADDRESS + sizeof(IDTR))
#define IDT_TABLE_SIZE (IDT_MAX_ENTRY_COUNT * sizeof(IDT_ENTRY))
#define IST_START_ADDRESS				0x700000
#define IST_SIZE								0x100000

#pragma pack(push, 1)

typedef struct GDTRStruct
{
	WORD limit;
	QWORD baseAddress;
	WORD wPadding;
	DWORD dwPadding;
} GDTR, IDTR;

typedef struct GDTEntry8Struct
{
	WORD lowerLimit;
	WORD lowerBaseAddress;
	BYTE upperBaseAddress1;
	BYTE typeAndLowerFlag; // 4bit Type, 1bit S, 2bit DPL, 1bit P
	BYTE upperLimitAndUpperFlag; // 4bit Segment limit, 1bit AVL, L, D/B, G
	BYTE upperBaseAddress2;
} GDT_ENTRY8;

typedef struct GDTEntry16Struct
{
	WORD lowerLimit;
	WORD lowerBaseAddress;
	BYTE middleBaseAddress1;
	BYTE typeAndLowerFlag; // 4bit Type, 1bit 0, 2bit DPL, 1bit P
	BYTE upperLimitAndUpperFlag; // 4bit Segment limit, 1bit AVL, 0, 0, G
	BYTE middleBaseAddress2;
	DWORD upperBaseAddress;
	DWORD reserved;
} GDT_ENTRY16;

typedef struct TSSDataStruct
{
	DWORD reserved1;
	QWORD rsp[3];
	QWORD reserved2;
	QWORD IST[7];
	QWORD reserved3;
	WORD reserved4;
	WORD IOMapBaseAddress;
} TSS_SEGMENT;

typedef struct IDTEntryStruct
{
	WORD lowerBaseAddress;
	WORD segmentSelector;
	BYTE IST; // 3bit IST, 5bit 0
	BYTE typeAndFlags; // 4bit Type, 1bit 0, 2bit DPL, 1bit P
	WORD middleBaseAddress;
	DWORD upperBaseAddress;
	DWORD reserved;
} IDT_ENTRY;

#pragma pack (pop)

void initGDTTableAndTSS(void);

void setGDTEntry8(GDT_ENTRY8* entry, DWORD baseAddress, DWORD limit,
	BYTE upperFlags, BYTE lowerFlags, BYTE type);

void setGDTEntry16(GDT_ENTRY16* entry, QWORD baseAddress, DWORD limit,
	BYTE upperFlags, BYTE lowerFlags, BYTE type);

void initTSSSegment(TSS_SEGMENT* TSS);
void initIDTTables(void);
void setIDTEntry(IDT_ENTRY* entry, void* handler, WORD selector,
	BYTE IST, BYTE flags, BYTE type);

#endif
