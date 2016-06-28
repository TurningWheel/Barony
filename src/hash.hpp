/*-------------------------------------------------------------------------------

	BARONY
	File: hash.hpp
	Desc: header for hash.cpp

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#define HASH_SIZE 256

typedef struct ttfTextHash_t {
	char *str;
	SDL_Surface *surf;
	TTF_Font *font;
	bool outline;
} ttfTextHash_t;

unsigned long djb2Hash(char *str);
SDL_Surface *ttfTextHashRetrieve(list_t *buckets, char *str, TTF_Font *font, bool outline);
SDL_Surface *ttfTextHashStore(list_t *buckets, char *str, TTF_Font *font, bool outline, SDL_Surface *surf);
void ttfTextHash_deconstructor(void *data);
