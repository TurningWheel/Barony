/*-------------------------------------------------------------------------------

BARONY
File: items.cpp
Desc: contains helper functions for item stuff

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "items.hpp"
#include "messages.hpp"
#include "interface/interface.hpp"
#include "magic/magic.hpp"
#include "sound.hpp"
#include "book.hpp"
#include "scrolls.hpp"
#include "shops.hpp"
#include "prng.hpp"
#include "scores.hpp"
#include "net.hpp"
#include "player.hpp"

Uint32 itemuids = 1;
ItemGeneric items[NUMITEMS];

/*-------------------------------------------------------------------------------

newItem

Creates a new item and places it in an inventory

-------------------------------------------------------------------------------*/

Item* newItem(ItemType type, Status status, Sint16 beatitude, Sint16 count, Uint32 appearance, bool identified, list_t* inventory)
{
	Item* item;

	// allocate memory for the item
	if ( (item = (Item*)malloc(sizeof(Item))) == NULL )
	{
		printlog("failed to allocate memory for new item!\n");
		exit(1);
	}

	//item->captured_monster = nullptr;

	// add the item to the inventory
	if ( inventory != NULL )
	{
		item->node = list_AddNodeLast(inventory);
		item->node->element = item;
		item->node->deconstructor = &defaultDeconstructor;
		item->node->size = sizeof(Item);
	}
	else
	{
		item->node = NULL;
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
		int x, y;
		bool notfree = false, foundaspot = false;

		bool is_spell = false;
		if ( itemCategory(item) == SPELL_CAT )
		{
			is_spell = true;
		}

		x = 0;
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

Category itemCategory(const Item* item)
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

Sint32 itemModel(Item* item)
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

Sint32 itemModelFirstperson(Item* item)
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

SDL_Surface* itemSprite(Item* item)
{
	if ( !item )
	{
		return NULL;
	}
		node_t* node = list_Node(&items[item->type].surfaces, item->appearance % items[item->type].variations);
		if ( !node )
		{
			return NULL;
		}
		SDL_Surface** surface = (SDL_Surface**)node->element;
		return *surface;
	return NULL;
}

/*-------------------------------------------------------------------------------

newItemFromEntity

returns a pointer to an item struct from the given entity if it's an
"item" entity, and returns NULL if the entity is anything else

-------------------------------------------------------------------------------*/

Item* newItemFromEntity(Entity* entity)
{
	if ( entity == NULL )
	{
		return NULL;
	}
	return newItem(static_cast<ItemType>(entity->skill[10]), static_cast<Status>(entity->skill[11]), entity->skill[12], entity->skill[13], entity->skill[14], entity->skill[15], NULL);
}

/*-------------------------------------------------------------------------------

Item::weaponGetAttack

returns the attack power of the given item

-------------------------------------------------------------------------------*/

Sint32 Item::weaponGetAttack()
{
	Sint32 attack = beatitude;
	if ( itemCategory(this) == MAGICSTAFF )
	{
		attack += 6;
	}
	else if ( type == SLING )
	{
		attack += 4;
	}
	else if ( type == QUARTERSTAFF )
	{
		attack += 4;
	}
	else if ( type == BRONZE_SWORD )
	{
		attack += 4;
	}
	else if ( type == BRONZE_MACE )
	{
		attack += 4;
	}
	else if ( type == BRONZE_AXE )
	{
		attack += 4;
	}
	else if ( type == IRON_SPEAR )
	{
		attack += 5;
	}
	else if ( type == IRON_SWORD )
	{
		attack += 5;
	}
	else if ( type == IRON_MACE )
	{
		attack += 5;
	}
	else if ( type == IRON_AXE )
	{
		attack += 5;
	}
	else if ( type == SHORTBOW )
	{
		attack += 8;
	}
	else if ( type == STEEL_HALBERD )
	{
		attack += 6;
	}
	else if ( type == STEEL_SWORD )
	{
		attack += 6;
	}
	else if ( type == STEEL_MACE )
	{
		attack += 6;
	}
	else if ( type == STEEL_AXE )
	{
		attack += 6;
	}
	else if ( type == CROSSBOW )
	{
		attack += 8;
	}
	else if ( type == ARTIFACT_SWORD )
	{
		attack += 10;
	}
	else if ( type == ARTIFACT_MACE )
	{
		attack += 10;
	}
	else if ( type == ARTIFACT_SPEAR )
	{
		attack += 10;
	}
	else if ( type == ARTIFACT_AXE )
	{
		attack += 10;
	}
	else if ( type == ARTIFACT_BOW )
	{
		attack += 15;
	}
	attack *= (double)(status / 5.0);

	return attack;
}

/*-------------------------------------------------------------------------------

Item::armorGetAC

returns the armor value of the given item

-------------------------------------------------------------------------------*/

Sint32 Item::armorGetAC()
{
	Sint32 armor = beatitude;
	if ( type == LEATHER_HELM )
	{
		armor += 1;
	}
	else if ( type == IRON_HELM )
	{
		armor += 2;
	}
	else if ( type == STEEL_HELM )
	{
		armor += 3;
	}
	else if ( type == LEATHER_BREASTPIECE )
	{
		armor += 2;
	}
	else if ( type == IRON_BREASTPIECE )
	{
		armor += 3;
	}
	else if ( type == STEEL_BREASTPIECE )
	{
		armor += 4;
	}
	else if ( type == GLOVES || type == GLOVES_DEXTERITY )
	{
		armor += 1;
	}
	else if ( type == BRACERS || type == BRACERS_CONSTITUTION )
	{
		armor += 2;
	}
	else if ( type == GAUNTLETS || type == GAUNTLETS_STRENGTH )
	{
		armor += 3;
	}
	else if ( type == LEATHER_BOOTS || type == LEATHER_BOOTS_SPEED )
	{
		armor += 1;
	}
	else if ( type == IRON_BOOTS || type == IRON_BOOTS_WATERWALKING )
	{
		armor += 2;
	}
	else if ( type == STEEL_BOOTS || type == STEEL_BOOTS_LEVITATION || type == STEEL_BOOTS_FEATHER )
	{
		armor += 3;
	}
	else if ( type == WOODEN_SHIELD )
	{
		armor += 1;
	}
	else if ( type == BRONZE_SHIELD )
	{
		armor += 2;
	}
	else if ( type == IRON_SHIELD )
	{
		armor += 3;
	}
	else if ( type == STEEL_SHIELD || type == STEEL_SHIELD_RESISTANCE )
	{
		armor += 4;
	}
	else if ( type == CLOAK_PROTECTION )
	{
		armor += 1;
	}
	else if ( type == RING_PROTECTION )
	{
		armor += 1;
	}
	//armor *= (double)(item->status/5.0);

	return armor;
}

int loadItems()
{
	int c, x;
	char name[32];
	FILE* fp;
	// load item types
	printlog("loading items...\n");
	fp = fopen("items/items.txt", "r");
	for ( c = 0; !feof(fp); c++ )
	{
		items[c].name_identified = language[1545 + c * 2];
		items[c].name_unidentified = language[1546 + c * 2];
		fscanf(fp, "%d", &items[c].index);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
		{
			break;
		}
		fscanf(fp, "%d", &items[c].fpindex);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
		{
			break;
		}
		fscanf(fp, "%d", &items[c].variations);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
		{
			break;
		}
		fscanf(fp, "%s", name);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
		{
			break;
		}
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
		fscanf(fp, "%d", &items[c].weight);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
		{
			break;
		}
		fscanf(fp, "%d", &items[c].value);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
		{
			break;
		}
		items[c].images.first = NULL;
		items[c].images.last = NULL;
		while ( 1 )
		{
			string_t* string = (string_t*)malloc(sizeof(string_t));
			string->data = (char*)malloc(sizeof(char) * 64);
			string->lines = 1;

			node_t* node = list_AddNodeLast(&items[c].images);
			node->element = string;
			node->deconstructor = &stringDeconstructor;
			node->size = sizeof(string_t);
			string->node = node;

			x = 0;
			bool fileend = false;
			while ( (string->data[x] = fgetc(fp)) != '\n' )
			{
				if ( feof(fp) )
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
		items[c].surfaces.first = NULL;
		items[c].surfaces.last = NULL;
		for ( x = 0; x < list_Size(&items[c].images); x++ )
		{
			SDL_Surface** surface = (SDL_Surface**)malloc(sizeof(SDL_Surface*));
			node_t* node = list_AddNodeLast(&items[c].surfaces);
			node->element = surface;
			node->deconstructor = &defaultDeconstructor;
			node->size = sizeof(SDL_Surface*);

			node_t* node2 = list_Node(&items[c].images, x);
			string_t* string = (string_t*)node2->element;
			*surface = loadImage(string->data);
		}
	}
	fclose(fp);
}