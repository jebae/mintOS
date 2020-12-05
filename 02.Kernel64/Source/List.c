#include "List.h"

void initList(LIST* list)
{
	list->itemCount = 0;
	list->header = NULL;
	list->tail = NULL;
}

int getListCount(const LIST* list)
{
	return list->itemCount;
}

void addListToTail(LIST* list, void* item)
{
	LISTLINK* link = (LISTLINK*)item;

	link->next = NULL;
	if (list->header == NULL)
	{
		list->header = item;
		list->tail = item;
		list->itemCount = 1;
		return;
	}
	link = (LISTLINK*)list->tail;
	link->next = item;
	list->tail = item;
	list->itemCount++;
}

void addListToHeader(LIST* list, void* item)
{
	LISTLINK* link = (LISTLINK*)item;

	link->next = list->header;
	if (list->header == NULL)
	{
		list->header = item;
		list->tail = item;
		list->itemCount = 1;
		return;
	}
	list->header = item;
	list->itemCount++;
}

void* removeList(LIST* list, QWORD id)
{
	LISTLINK* prev = NULL;
	LISTLINK* cur = list->header;

	while (cur != NULL && cur->id != id)
	{
		prev = cur;
		cur = cur->next;
	}
	if (cur == NULL)
		return NULL;
	if (cur == list->header)
		list->header = cur->next;
	if (cur == list->tail)
		list->tail = prev;
	if (prev)
		prev->next = cur->next;
	list->itemCount--;
	return cur;
} 
void* removeListFromHeader(LIST* list)
{
	LISTLINK* link;

	if (list->itemCount == 0)
		return NULL;
	link = (LISTLINK*)list->header;
	return removeList(list, link->id);
}

void* removeListFromTail(LIST* list)
{
	LISTLINK* link;

	if (list->itemCount == 0)
		return NULL;
	link = (LISTLINK*)list->tail;
	return removeList(list, link->id);
}

void* findList(const LIST* list, QWORD id)
{
	LISTLINK* cur;

	for (cur=(LISTLINK*)list->header; cur != NULL; cur=cur->next)
	{
		if (cur->id == id)
			return cur;
	}
	return NULL;
}

void* getHeaderFromList(const LIST* list)
{
	return list->header;
}

void* getTailFromList(const LIST* list)
{
	return list->tail;
}
void* getNextFromList(const LIST* list, void* current)
{
	LISTLINK* link = (LISTLINK*)current;

	return link->next;
}
