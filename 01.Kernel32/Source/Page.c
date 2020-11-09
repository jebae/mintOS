#include "Page.h"

void initPageTables(void)
{
	PML4T_ENTRY* PML4Entry;
	PDPT_ENTRY* PDPTEntry;
	PD_ENTRY* PDEntry;
	DWORD address;
	int i;

	PML4Entry = (PML4T_ENTRY*)0x100000;
	setPageEntryData(&PML4Entry[0], 0x00, 0x101000, PAGE_FLAGS_DEFAULT, 0);
	for (i=1; i < PAGE_MAX_ENTRY_COUNT; i++)
	{
		setPageEntryData(&PML4Entry[i], 0, 0, 0, 0);
	}

	PDPTEntry = (PDPT_ENTRY*)0x101000;
	for (i=0; i < 64; i++)
	{
		setPageEntryData(&PDPTEntry[i], 0, 0x102000 + (i * PAGE_TABLE_SIZE),
			PAGE_FLAGS_DEFAULT, 0);
	}
	for (; i < PAGE_MAX_ENTRY_COUNT; i++)
	{
		setPageEntryData(&PDPTEntry[i], 0, 0, 0, 0);
	}

	PDEntry = (PD_ENTRY*)0x102000;
	address = 0;
	for (i=0; i < PAGE_MAX_ENTRY_COUNT * 64; i++)
	{
		setPageEntryData(&PDEntry[i], (i * (0x200000 >> 20)) >> 12, address,
			PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS, 0);
		address += PAGE_DEFAULT_SIZE;
	}
}

void setPageEntryData(PT_ENTRY* entry, DWORD upperBaseAddress,
	DWORD lowerBaseAddress, DWORD lowerFlags, DWORD upperFlags)
{
	entry->attributeAndLowerBaseAddress = lowerBaseAddress | lowerFlags;
	entry->upperBaseAddressAndEXB = (upperBaseAddress & 0xFF) | upperFlags;
}

