/*-------------------------------------------------------------------------------

	BARONY
	File: hash.cpp
	Desc: module for generating hash values and working with tables

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "hash.hpp"

unsigned long djb2Hash(char* str) {
	unsigned long hash = 5381;
	int c;

	while (c = *str++) {
		hash = ((hash << 5) + hash) + c;    /* hash * 33 + c */
	}

	return hash;
}

void ttfTextHash_deconstructor(void* data) {
	ttfTextHash_t* hashedVal = static_cast<ttfTextHash_t*>(data);
	SDL_FreeSurface(hashedVal->surf);
	free(hashedVal->str);
	free(data);
}

SDL_Surface* ttfTextHashRetrieve(list_t* buckets, char* str, TTF_Font* font, bool outline) {
	node_t* node;

	// retrieve bucket
	list_t* list = &buckets[djb2Hash(str) % HASH_SIZE];

	// find data in bucket (linear search)
	for ( node = list->first; node != NULL; node = node->next ) {
		ttfTextHash_t* hashedVal = (ttfTextHash_t*)node->element;
		if ( !strcmp(hashedVal->str, str) && hashedVal->font == font && hashedVal->outline == outline ) {
			return hashedVal->surf;
		}
	}

	return NULL;
}

SDL_Surface* ttfTextHashStore(list_t* buckets, char* str, TTF_Font* font, bool outline, SDL_Surface* surf) {
	ttfTextHash_t* hashedVal;
	node_t* node;

	// retrieve bucket
	list_t* list = &buckets[djb2Hash(str) % HASH_SIZE];

	// add surface to bucket
	if ( (node = list_AddNodeFirst(list)) == NULL ) {
		return NULL;
	} else {
		hashedVal = (ttfTextHash_t*) malloc(sizeof(ttfTextHash_t));
		hashedVal->str = (char*) calloc(strlen(str) + 1, sizeof(char));
		strcpy(hashedVal->str, str);
		hashedVal->surf = surf;
		hashedVal->font = font;
		hashedVal->outline = outline;

		node->deconstructor = &ttfTextHash_deconstructor;
		node->size = sizeof(ttfTextHash_t);
		node->element = hashedVal;

		return surf;
	}
}