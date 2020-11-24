#ifndef __LIST_H__
#define __LIST_H__

#include "Types.h"

#pragma pack(push, 1)

typedef struct ListLinkStruct
{
	void* next;
	QWORD id;
} LISTLINK;

typedef struct ListManagerStruct
{
	int itemCount;
	void* header;
	void* tail;
} LIST;

#pragma pack(pop)

void initList(LIST* list);
int getListCount(const LIST* list);
void addListToTail(LIST* list, void* item);
void addListToHeader(LIST* list, void* item);
void* removeList(LIST* list, QWORD id);
void* removeListFromHeader(LIST* list);
void* removeListFromTail(LIST* list);
void* findList(const LIST* list, QWORD id);
void* getHeaderFromList(const LIST* list);
void* getTailFromList(const LIST* list);
void* getNextFromList(const LIST* list, void* current);

#endif
