/*-------------------------------------------------------------------------------

	BARONY
	File: list.cpp
	Desc: contains list handling functions.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"

/*-------------------------------------------------------------------------------

	list_FreeAll

	frees an entire list and all of its contents

-------------------------------------------------------------------------------*/

void list_FreeAll(list_t *list) {
	node_t *node, *nextnode;
	for( node=list->first; node!=NULL; node=nextnode ) {
		nextnode = node->next;
		list_RemoveNode(node);
	}
	list->first = NULL;
	list->last = NULL;
}

/*-------------------------------------------------------------------------------

	list_RemoveNode

	removes a specific node from a list

-------------------------------------------------------------------------------*/

void list_RemoveNode(node_t *node) {
	if( node->list && node->list->first ) {
		// if this is the first node...
		if( node == node->list->first ) {
			// is it also the last node?
			if( node->list->last == node ) {
				node->list->first = NULL;
				node->list->last = NULL;
			}

			// otherwise, the "first" pointer needs to point to the next node
			else {
				node->next->prev = NULL;
				node->list->first = node->next;
			}
		}

		// if this is the last node, but not the first...
		else if( node == node->list->last ) {
			node->prev->next = NULL;
			node->list->last = node->prev; // the "last" pointer needs to point to the previous node
		}

		// if the node is neither first nor last, it is in the middle
		else {
			// bridge the previous node and the first node together
			node->prev->next = node->next;
			node->next->prev = node->prev;
		}
	}

	// once the node is removed from the list, delete it
	// If a node has a deconstructor, then deconstruct it.  Otherwise it's a class and we'll delete it (which calls the destructor)
	if (*node->deconstructor) {
		(*node->deconstructor)(node->element);
	} else {
		delete(node->element);
	}
	free(node);
}

/*-------------------------------------------------------------------------------

	list_AddNodeFirst

	inserts a new node at the beginning of a given list

	warning: "element" and "deconstructor" elements are NULL when created!

-------------------------------------------------------------------------------*/

node_t *list_AddNodeFirst(list_t *list) {
	node_t *node;

	// allocate memory for node
	if( (node = (node_t *) malloc(sizeof(node_t))) == NULL ) {
		printlog( "failed to allocate memory for new node!\n" );
		exit(1);
	}

	// initialize data pointers to NULL
	node->element = NULL;
	node->deconstructor = NULL;
	node->size = 0;
	node->prev = NULL;

	// integrate it into the list
	node->list = list;
	if( list->first != NULL ) {
		// there are prior nodes in the list
		node->next = list->first;
		list->first->prev = node;
	} else {
		// inserting into an empty list
		node->next = NULL;
		list->last = node;
	}
	list->first = node;

	return node;
}

/*-------------------------------------------------------------------------------

	list_AddNodeLast

	inserts a new node at the end of a given list

	warning: "element" and "deconstructor" elements are NULL when created!

-------------------------------------------------------------------------------*/

node_t *list_AddNodeLast(list_t *list) {
	node_t *node;

	// allocate memory for node
	if( (node = (node_t *) malloc(sizeof(node_t))) == NULL ) {
		printlog( "failed to allocate memory for new node!\n" );
		exit(1);
	}

	// initialize data pointers to NULL
	node->element = NULL;
	node->deconstructor = NULL;
	node->size = 0;
	node->next = NULL;

	// integrate it into the list
	node->list = list;
	if( list->last != NULL ) {
		// there are prior nodes in the list
		node->prev = list->last;
		list->last->next = node;
	} else {
		// inserting into an empty list
		node->prev = NULL;
		list->first = node;
	}
	list->last = node;

	return node;
}

/*-------------------------------------------------------------------------------

	list_AddNode

	inserts a new node at the specified index of the given list, pushing
	any existing nodes back to make room for it.

	warning: "element" and "deconstructor" elements are NULL when created!

-------------------------------------------------------------------------------*/

node_t *list_AddNode(list_t *list, int index) {
	node_t *node;
	if( index<0 || index>list_Size(list)) {
		return NULL;
	}

	// allocate memory for node
	if( (node = (node_t *) malloc(sizeof(node_t))) == NULL ) {
		printlog( "failed to allocate memory for new node!\n" );
		exit(1);
	}

	// initialize data pointers to NULL
	node->element = NULL;
	node->deconstructor = NULL;
	node->size = 0;
	node->prev = NULL;
	node->next = NULL;

	// integrate it into the list
	node->list = list;
	node_t *oldnode = list_Node(list,index);
	if( oldnode ) {
		// inserting at the beginning or middle of a list
		node->prev = oldnode->prev;
		node->next = oldnode;
		if( list->first == oldnode ) {
			// inserting at the beginning
			list->first = node;
		} else {
			// inserting in the middle
			oldnode->prev->next = node;
		}
		oldnode->prev = node;
	} else {
		if( list_Size(list) ) {
			// inserting at the end of a list
			node->prev = list->last;
			node->next = NULL;
			list->last->next = node;
			list->last = node;
		} else {
			// inserting into an empty list
			node->prev = NULL;
			node->next = NULL;
			list->first = node;
			list->last = node;
		}
	}

	return node;
}

/*-------------------------------------------------------------------------------

	list_Size

	returns the number of nodes in the given list

-------------------------------------------------------------------------------*/

Uint32 list_Size(list_t *list) {
	node_t *node;
	int c;

	for( c=0, node=list->first; node!=NULL; node=node->next, c++ );
	return c;
}

/*-------------------------------------------------------------------------------

	list_Copy

	copies the contents of one list into another (appending any new nodes at
	the end of destlist)

-------------------------------------------------------------------------------*/

list_t *list_Copy(list_t *destlist, list_t *srclist) {
	node_t *node;
	for( node=srclist->first; node!=NULL; node=node->next ) {
		if( node->size == 0 ) {
			printlog("error: attempted copy of node with size 0! Node not copied\n");
			continue;
		}
		node_t *newnode = list_AddNodeLast(destlist);
		newnode->deconstructor = node->deconstructor;
		newnode->element = malloc(node->size);
		newnode->size = node->size;
		memcpy(newnode->element,node->element,node->size);
	}

	return destlist;
}

/*-------------------------------------------------------------------------------

	list_CopyNew

	copies the contents of one list into a new one which is subsequently
	returned

-------------------------------------------------------------------------------*/

list_t *list_CopyNew(list_t *srclist) {
	if( !srclist ) {
		return NULL;
	}
	list_t *destlist = (list_t *) malloc(sizeof(list_t));
	if( !destlist ) {
		printlog("critical error: list_CopyNew() failed to allocate memory for new list!\n");
		return NULL;
	}
	destlist->first = NULL;
	destlist->last = NULL;

	node_t *node;
	for( node=srclist->first; node!=NULL; node=node->next ) {
		if( node->size == 0 ) {
			printlog("error: attempted copy of node with size 0! Node not copied\n");
			continue;
		}
		node_t *newnode = list_AddNodeLast(destlist);
		newnode->deconstructor = node->deconstructor;
		newnode->element = malloc(node->size);
		newnode->size = node->size;
		memcpy(newnode->element,node->element,node->size);
	}

	return destlist;
}

/*-------------------------------------------------------------------------------

	list_Index

	returns the index number of a given node in its list.

-------------------------------------------------------------------------------*/

Uint32 list_Index(node_t *node) {
	node_t *tempnode;
	int i;

	for( i=0, tempnode=node->list->first; tempnode!=NULL; tempnode=tempnode->next, i++ ) {
		if( tempnode==node ) {
			break;
		}
	}

	return i;
}

/*-------------------------------------------------------------------------------

	list_Node

	returns the node with the given index number in the given list

-------------------------------------------------------------------------------*/

node_t *list_Node(list_t *list, int index) {
	if( index<0 || index>=list_Size(list) ) {
		return NULL;
	}

	int i;
	node_t *node=list->first;

	for( i=0; i!=index; node=node->next, i++ );
	return node;
}
