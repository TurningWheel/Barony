/*-------------------------------------------------------------------------------

	BARONY
	File: objects.cpp
	Desc: contains object constructors and deconstructors

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include <new>
#include "main.hpp"
#include "entity.hpp"

/*-------------------------------------------------------------------------------

	defaultDeconstructor

	Frees the memory occupied by a typical node's data. Do not use for more
	complex nodes that malloc extraneous data to themselves!

-------------------------------------------------------------------------------*/

void defaultDeconstructor(void *data) {
	if (data != NULL) {
		free(data);
	}
}

/*-------------------------------------------------------------------------------

	stringDeconstructor

	Frees the memory occupied by a string.

-------------------------------------------------------------------------------*/

void stringDeconstructor(void *data) {
	string_t *string;
	if (data != NULL) {
		string = (string_t *)data;
		if ( string->data != NULL ) {
			free(string->data);
			string->data = NULL;
		}
		free(data);
	}
}

/*-------------------------------------------------------------------------------

	emptyDeconstructor

	Useful to remove a node without deallocating its data.

-------------------------------------------------------------------------------*/

void emptyDeconstructor(void *data) {
	return;
}

/*-------------------------------------------------------------------------------

	entityDeconstructor

	Frees the memory occupied by a node pointing to an entity

-------------------------------------------------------------------------------*/

void entityDeconstructor(void *data) {
	Entity *entity;

	if (data != NULL) {
		entity = (Entity *)data;

		//free(data);
		delete entity;
	}
}

/*-------------------------------------------------------------------------------

	lightDeconstructor

	Frees the memory occupied by a node pointing to a light

-------------------------------------------------------------------------------*/

void lightDeconstructor(void *data) {
	Sint32 x, y;
	light_t *light;

	if ( data != NULL) {
		light = (light_t *)data;
		if ( light->tiles != NULL ) {
			for (y = 0; y < light->radius * 2; y++) {
				for (x = 0; x < light->radius * 2; x++) {
					if ( x + light->x - light->radius >= 0 && x + light->x - light->radius < map.width )
						if ( y + light->y - light->radius >= 0 && y + light->y - light->radius < map.height ) {
							lightmap[(y + light->y - light->radius) + (x + light->x - light->radius)*map.height] -= light->tiles[y + x * (light->radius * 2 + 1)];
						}
				}
			}
			free(light->tiles);
		}
		free(data);
	}
}

/*-------------------------------------------------------------------------------

	mapDeconstructor

	Frees the memory occupied by a node pointing to a map

-------------------------------------------------------------------------------*/

void mapDeconstructor(void *data) {
	map_t *map;

	if (data != NULL) {
		map = (map_t *)data;
		if (map->tiles != NULL) {
			free(map->tiles);
		}
		if (map->entities != NULL) {
			list_FreeAll(map->entities);
			free(map->entities);
		}
		free(data);
	}
}

/*-------------------------------------------------------------------------------

	listDeconstructor

	Frees the memory occupied by a node pointing to a list

-------------------------------------------------------------------------------*/

void listDeconstructor(void *data) {
	list_t *list;

	if (data != NULL) {
		list = (list_t *)data;
		list_FreeAll(list);
		free(data);
	}
}

/*-------------------------------------------------------------------------------

	newEntity

	Creates a new entity with empty settings and places it in the entity list

-------------------------------------------------------------------------------*/

Entity *newEntity(Sint32 sprite, Uint32 pos, list_t *entlist) {
	Entity *entity;

	// allocate memory for entity
	/*if( (entity = (Entity *) malloc(sizeof(Entity)))==NULL ) {
		printlog( "failed to allocate memory for new entity!\n" );
		exit(1);
	}*/
	try {
		entity = new Entity(sprite, pos, entlist);
	} catch (std::bad_alloc& ba) {
		printlog( "failed to allocate memory for new entity!\n" );
		exit(1);
	}

	return entity;
}

/*-------------------------------------------------------------------------------

	newButton

	Creates a new button and places it in the button list

-------------------------------------------------------------------------------*/

button_t *newButton(void) {
	button_t *button;

	// allocate memory for button
	if ( (button = (button_t *) malloc(sizeof(button_t))) == NULL ) {
		printlog( "failed to allocate memory for new button!\n" );
		exit(1);
	}

	// add the button to the button list
	button->node = list_AddNodeLast(&button_l);
	button->node->element = button;
	button->node->deconstructor = &defaultDeconstructor;
	button->node->size = sizeof(button_t);

	// now set all of my data elements to ZERO or NULL
	button->x = 0;
	button->y = 0;
	button->sizex = 0;
	button->sizey = 0;
	button->visible = 1;
	button->focused = 0;
	button->key = 0;
	button->joykey = -1;
	button->pressed = FALSE;
	button->needclick = TRUE;
	button->action = NULL;
	strcpy(button->label, "nodef");

	button->outline = false;

	return button;
}

/*-------------------------------------------------------------------------------

	newLight

	Creates a new light and places it in the light list

-------------------------------------------------------------------------------*/

light_t *newLight(Sint32 x, Sint32 y, Sint32 radius, Sint32 intensity) {
	light_t *light;

	// allocate memory for light
	if ( (light = (light_t *) malloc(sizeof(light_t))) == NULL ) {
		printlog( "failed to allocate memory for new light!\n" );
		exit(1);
	}

	// add the light to the light list
	light->node = list_AddNodeLast(&light_l);
	light->node->element = light;
	light->node->deconstructor = &lightDeconstructor;
	light->node->size = sizeof(light_t);

	// now set all of my data elements to ZERO or NULL
	light->x = x;
	light->y = y;
	light->radius = radius;
	light->intensity = intensity;
	if ( light->radius > 0 ) {
		light->tiles = (Sint32 *) malloc(sizeof(Sint32) * (radius * 2 + 1) * (radius * 2 + 1));
		memset(light->tiles, 0, sizeof(Sint32) * (radius * 2 + 1) * (radius * 2 + 1));
	} else {
		light->tiles = NULL;
	}
	return light;
}

/*-------------------------------------------------------------------------------

	newString

	Creates a new string and places it in a list

-------------------------------------------------------------------------------*/

string_t *newString(list_t *list, Uint32 color, char *content, ...) {
	string_t *string;
	char str[1024] = { 0 };
	va_list argptr;
	int c, i;

	// allocate memory for string
	if ( (string = (string_t *) malloc(sizeof(string_t))) == NULL ) {
		printlog( "failed to allocate memory for new string!\n" );
		exit(1);
	}

	if ( content ) {
		if ( strlen(content) > 2048 ) {
			printlog( "error creating new string: buffer overflow.\n" );
			exit(1);
		}
	}

	string->color = color;
	string->lines = 1;
	if ( content != NULL ) {
		// format the content
		va_start( argptr, content );
		i = vsnprintf(str, 1023, content, argptr);
		va_end( argptr );
		string->data = (char *) malloc(sizeof(char) * (i + 1));
		if ( !string->data ) {
			printlog( "error creating new string: couldn't allocate string data.\n" );
			exit(1);
		}
		memset(string->data, 0, sizeof(char) * (i + 1));
		for ( c = 0; c < i; c++ ) {
			if ( str[c] == 10 ) { // line feed
				string->lines++;
			}
		}
		strncpy(string->data, str, i);
	} else {
		string->data = NULL;
	}

	// add the string to the list
	if ( list != NULL ) {
		string->node = list_AddNodeLast(list);
		string->node->element = string;
		string->node->deconstructor = &stringDeconstructor;
		string->node->size = sizeof(string_t);
	} else {
		string->node = NULL;
	}

	return string;
}

/*-------------------------------------------------------------------------------

	newPathnode

	Creates a new pathnode and places it in a list

-------------------------------------------------------------------------------*/

pathnode_t *newPathnode(list_t *list, Sint32 x, Sint32 y, pathnode_t *parent, Sint8 pos) {
	pathnode_t *pathnode;

	// allocate memory for pathnode
	if ( (pathnode = (pathnode_t *) malloc(sizeof(pathnode_t))) == NULL ) {
		printlog( "failed to allocate memory for new pathnode!\n" );
		exit(1);
	}

	// assign values
	pathnode->x = x;
	pathnode->y = y;
	pathnode->parent = parent;
	pathnode->g = 0;
	pathnode->h = 0;

	// add the pathnode to the list
	if ( !pos ) {
		pathnode->node = list_AddNodeFirst(list);
	} else {
		pathnode->node = list_AddNodeLast(list);
	}
	pathnode->node->element = pathnode;
	pathnode->node->deconstructor = &defaultDeconstructor;
	pathnode->node->size = sizeof(pathnode_t);

	return pathnode;
}
