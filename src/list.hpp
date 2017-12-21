#pragma once

typedef struct node_t
{
	struct node_t* next;
	struct node_t* prev;
	struct list_t* list;
	void* element;
	void (*deconstructor)(void* data);
	Uint32 size;
} node_t;

typedef struct list_t
{
	node_t* first;
	node_t* last;
} list_t;

void list_FreeAll(list_t* list);
void list_RemoveNode(node_t* node);
node_t* list_AddNodeFirst(list_t* list);
node_t* list_AddNodeLast(list_t* list);
node_t* list_AddNode(list_t* list, int index);
Uint32 list_Size(list_t* list);
list_t* list_Copy(list_t* destlist, list_t* srclist);
list_t* list_CopyNew(list_t* srclist);
Uint32 list_Index(node_t* node);
node_t* list_Node(list_t* list, int index);
