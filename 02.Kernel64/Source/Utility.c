#include "Utility.h"

void memSet(void* dest, BYTE data, int size)
{
	for (int i=0; i < size; i++)
	{
		((char*)dest)[i] = data;
	}
}

int memCpy(void* dest, const void* src, int size)
{
	for (int i=0; i < size; i++)
	{
		((char*)dest)[i] = ((char*)src)[i];
	}
	return size;
}

int memCmp(const void* a, const void* b, int size)
{
	for (int i=0; i < size; i++)
	{
		if (((char*)a)[i] != ((char*)b)[i])
			return (int)(((char*)a)[i] - ((char*)b)[i]);
	}
	return 0;
}
