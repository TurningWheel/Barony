/*-------------------------------------------------------------------------------

BARONY
File: items.cpp
Desc: contains helper functions for item stuff

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "items.hpp"
#include "files.hpp"

Uint32 itemuids = 1;
ItemGeneric items[NUMITEMS];

/*-------------------------------------------------------------------------------

newItem

Creates a new item and places it in an inventory

-------------------------------------------------------------------------------*/

Item* newItem(const ItemType type, const Status status, const Sint16 beatitude, const Sint16 count, const Uint32 appearance, const bool identified, list_t* const inventory)
{
	Item* item;

	// allocate memory for the item
	if ( (item = static_cast<Item*>(malloc(sizeof(Item)))) == nullptr )
	{
		printlog("failed to allocate memory for new item!\n");
		exit(1);
	}

	//item->captured_monster = nullptr;

	// add the item to the inventory
	if ( inventory != nullptr )
	{
		item->node = list_AddNodeLast(inventory);
		item->node->element = item;
		item->node->deconstructor = &defaultDeconstructor;
		item->node->size = sizeof(Item);
	}
	else
	{
		item->node = nullptr;
	}

	// now set all of my data elements
	item->type = type;
	item->status = status;
	item->beatitude = beatitude;
	item->count = count;
	item->appearance = appearance;
	item->identified = identified;
	item->uid = itemuids;
	if ( inventory )
	{
		/*int x, y;
		bool notfree = false, foundaspot = false;

		bool is_spell = false;
		if ( itemCategory(item) == SPELL_CAT )
		{
			is_spell = true;
		}

		x = 0;*/
	}
	else
	{
		item->x = 0;
		item->y = 0;
	}

	itemuids++;
	return item;
}

/*-------------------------------------------------------------------------------

itemCategory

Returns the category that a specified item belongs to

-------------------------------------------------------------------------------*/

Category itemCategory(const Item* const item)
{
	if ( !item )
	{
		return GEM;
	}
	return items[item->type].category;
}

/*-------------------------------------------------------------------------------

itemModel

returns a model index number based on the properties of the given item

-------------------------------------------------------------------------------*/

Sint32 itemModel(const Item* const item)
{
	if ( !item )
	{
		return 0;
	}
	return items[item->type].index + item->appearance % items[item->type].variations;
}

/*-------------------------------------------------------------------------------

itemModelFirstperson

returns the first person model of the given item

-------------------------------------------------------------------------------*/

Sint32 itemModelFirstperson(const Item* const item)
{
	if ( !item )
	{
		return 0;
	}
	return items[item->type].fpindex + item->appearance % items[item->type].variations;
}

/*-------------------------------------------------------------------------------

itemSprite

returns a pointer to the SDL_Surface used to represent the item

-------------------------------------------------------------------------------*/

SDL_Surface* itemSprite(Item* const item)
{
	if ( !item )
	{
		return nullptr;
	}
	node_t* node = list_Node(&items[item->type].surfaces, item->appearance % items[item->type].variations);
	if ( !node )
	{
		return nullptr;
	}
	
	auto** surface = static_cast<SDL_Surface**>(node->element);
	return *surface;
}

/*-------------------------------------------------------------------------------

newItemFromEntity

returns a pointer to an item struct from the given entity if it's an
"item" entity, and returns NULL if the entity is anything else

-------------------------------------------------------------------------------*/

Item* newItemFromEntity(const Entity* const entity)
{
	if ( entity == nullptr )
	{
		return nullptr;
	}
	return newItem(static_cast<ItemType>(entity->skill[10]), static_cast<Status>(entity->skill[11]), entity->skill[12], entity->skill[13], entity->skill[14], entity->skill[15], nullptr);
}

char* Item::description() const
{
	return tempstr;
}

int loadItems()
{
	int c, x;
	char name[32];
	// load item types
	printlog("loading items...\n");
	File* const fp = openDataFile("items/items.txt", "r");
	for ( c = 0; !fp->eof(); c++ )
	{
		items[c].name_identified = language[1545 + c * 2];
		items[c].name_unidentified = language[1546 + c * 2];
		items[c].index = fp->geti();
		items[c].fpindex = fp->geti();
		items[c].variations = fp->geti();
		fp->gets(name, sizeof(name));
		if ( !strcmp(name, "WEAPON") )
		{
			items[c].category = WEAPON;
		}
		else if ( !strcmp(name, "ARMOR") )
		{
			items[c].category = ARMOR;
		}
		else if ( !strcmp(name, "AMULET") )
		{
			items[c].category = AMULET;
		}
		else if ( !strcmp(name, "POTION") )
		{
			items[c].category = POTION;
		}
		else if ( !strcmp(name, "SCROLL") )
		{
			items[c].category = SCROLL;
		}
		else if ( !strcmp(name, "MAGICSTAFF") )
		{
			items[c].category = MAGICSTAFF;
		}
		else if ( !strcmp(name, "RING") )
		{
			items[c].category = RING;
		}
		else if ( !strcmp(name, "SPELLBOOK") )
		{
			items[c].category = SPELLBOOK;
		}
		else if ( !strcmp(name, "TOOL") )
		{
			items[c].category = TOOL;
		}
		else if ( !strcmp(name, "FOOD") )
		{
			items[c].category = FOOD;
		}
		else if ( !strcmp(name, "BOOK") )
		{
			items[c].category = BOOK;
		}
		else if ( !strcmp(name, "SPELL_CAT") )
		{
			items[c].category = SPELL_CAT;
		}
		else
		{
			items[c].category = GEM;
		}
		items[c].weight = fp->geti();
		items[c].value = fp->geti();
		items[c].images.first = nullptr;
		items[c].images.last = nullptr;
		while ( true )
		{
			auto* string = static_cast<string_t*>(malloc(sizeof(string_t)));
			string->data = static_cast<char*>(malloc(sizeof(char) * 64));
			string->lines = 1;

			node_t* node = list_AddNodeLast(&items[c].images);
			node->element = string;
			node->deconstructor = &stringDeconstructor;
			node->size = sizeof(string_t);
			string->node = node;

			x = 0;
			bool fileend = false;
			while ( (string->data[x] = fp->getc()) != '\n' )
			{
				if ( fp->eof() )
				{
					fileend = true;
					break;
				}
				x++;
			}
			if ( x == 0 || fileend )
			{
				list_RemoveNode(node);
				break;
			}
			string->data[x] = 0;
		}
	}
	for ( c = 0; c < NUMITEMS; c++ )
	{
		items[c].surfaces.first = nullptr;
		items[c].surfaces.last = nullptr;
		for ( x = 0; x < list_Size(&items[c].images); x++ )
		{
			auto** surface = static_cast<SDL_Surface**>(malloc(sizeof(SDL_Surface*)));
			node_t* node = list_AddNodeLast(&items[c].surfaces);
			node->element = surface;
			node->deconstructor = &defaultDeconstructor;
			node->size = sizeof(SDL_Surface*);

			node_t* node2 = list_Node(&items[c].images, x);
			auto* string = static_cast<string_t*>(node2->element);
			*surface = loadImage(string->data);
		}
	}
	FileIO::close(fp);
	return 1;
}

int Item::sellValue(const int player) const
{
	return 0;
}