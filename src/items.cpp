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
int INVENTORY_SIZEY = 3;
const real_t potionDamageSkillMultipliers[6] = { 1.f, 1.1, 1.25, 1.5, 2.5, 4.f };
const real_t thrownDamageSkillMultipliers[6] = { 1.f, 1.1, 1.25, 1.5, 2.f, 3.f };

/*-------------------------------------------------------------------------------

	newItem

	Creates a new item and places it in an inventory

-------------------------------------------------------------------------------*/

Item* newItem(ItemType type, Status status, Sint16 beatitude, Sint16 count, Uint32 appearance, bool identified, list_t* inventory)
{
	Item* item;

	// allocate memory for the item
	if ( (item = (Item*) malloc(sizeof(Item))) == NULL )
	{
		printlog( "failed to allocate memory for new item!\n" );
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
	item->ownerUid = 0;
	if ( inventory )
	{
		int x, y;
		bool notfree = false, foundaspot = false;

		bool is_spell = false;
		if (itemCategory(item) == SPELL_CAT)
		{
			is_spell = true;
		}

		x = 0;
		int inventory_y = INVENTORY_SIZEY;
		if ( is_spell )
		{
			inventory_y = 3;
		}
		else if ( multiplayer != CLIENT )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( stats[i] && &stats[i]->inventory == inventory )
				{
					if ( stats[i]->cloak && stats[i]->cloak->type == CLOAK_BACKPACK && stats[i]->cloak->beatitude >= 0 )
					{
						inventory_y = 4;
						break;
					}
					break;
				}
			}
		}
		else if ( multiplayer == CLIENT )
		{
			if ( stats[clientnum] && &stats[clientnum]->inventory == inventory )
			{
				if ( stats[clientnum]->cloak && stats[clientnum]->cloak->type == CLOAK_BACKPACK && stats[clientnum]->cloak->beatitude >= 0 )
				{
					inventory_y = 4;
				}
			}
		}
		int sort_y = std::min(std::max(inventory_y, 2), 3); // only sort y values of 2-3, if extra row don't auto sort into it.

		while ( 1 )
		{
			for ( y = 0; y < sort_y; y++ )
			{
				node_t* node;
				for ( node = inventory->first; node != NULL; node = node->next )
				{
					Item* tempItem = (Item*)node->element;
					if ( tempItem == item )
					{
						continue;
					}
					if ( tempItem )
					{
						if ( tempItem->x == x && tempItem->y == y )
						{
							if (is_spell && itemCategory(tempItem) == SPELL_CAT)
							{
								notfree = true;  //Both spells. Can't fit in the same slot.
							}
							else if (!is_spell && itemCategory(tempItem) != SPELL_CAT)
							{
								notfree = true;  //Both not spells. Can't fit in the same slot.
							}
						}
					}
				}
				if ( notfree )
				{
					notfree = false;
					continue;
				}
				item->x = x;
				item->y = y;
				foundaspot = true;
				break;
			}
			if ( foundaspot )
			{
				break;
			}
			x++;
		}


		// backpack sorting, sort into here as last priority.
		if ( x > INVENTORY_SIZEX - 1 && inventory_y > 3 )
		{
			x = 0;
			foundaspot = false;
			notfree = false;
			while ( 1 )
			{
				for ( y = 3; y < inventory_y; y++ )
				{
					node_t* node;
					for ( node = inventory->first; node != NULL; node = node->next )
					{
						Item* tempItem = (Item*)node->element;
						if ( tempItem == item )
						{
							continue;
						}
						if ( tempItem )
						{
							if ( tempItem->x == x && tempItem->y == y )
							{
								if ( is_spell && itemCategory(tempItem) == SPELL_CAT )
								{
									notfree = true;  //Both spells. Can't fit in the same slot.
								}
								else if ( !is_spell && itemCategory(tempItem) != SPELL_CAT )
								{
									notfree = true;  //Both not spells. Can't fit in the same slot.
								}
							}
						}
					}
					if ( notfree )
					{
						notfree = false;
						continue;
					}
					item->x = x;
					item->y = y;
					foundaspot = true;
					break;
				}
				if ( foundaspot )
				{
					break;
				}
				x++;
			}
		}

		// add the item to the hotbar automatically
		if ( !intro && auto_hotbar_new_items)
		{
			if ( inventory == &stats[clientnum]->inventory )
			{
				int c;
				for ( c = 0; c < NUM_HOTBAR_SLOTS; c++ )
				{
					if ( !uidToItem(hotbar[c].item) )
					{
						if ( autoAddHotbarFilter(*item) )
						{
							hotbar[c].item = item->uid;
							break;
						}
					}
				}
			}
		}
	}
	else
	{
		item->x = 0;
		item->y = 0;
	}

	itemuids++;
	return item;
}

void addItemToMonsterInventory(Item &item, list_t& inventory)
{
	// add the item to the inventory

	item.node = list_AddNodeLast(&inventory);
	item.node->element = &item;
	item.node->deconstructor = &defaultDeconstructor;
	item.node->size = sizeof(Item);

	int x, y;
	bool notfree = false, foundaspot = false;

	bool is_spell = false;
	if (itemCategory(&item) == SPELL_CAT)
	{
		is_spell = true;
	}

	x = 0;
	while ( 1 )
	{
		for ( y = 0; y < INVENTORY_SIZEY; ++y )
		{
			node_t* node;
			for ( node = inventory.first; node != nullptr; node = node->next )
			{
				Item* tempItem = (Item*)node->element;
				if ( tempItem == &item )
				{
					continue;
				}
				if ( tempItem )
				{
					if ( tempItem->x == x && tempItem->y == y )
					{
						if (is_spell && itemCategory(tempItem) == SPELL_CAT)
						{
							notfree = true;  //Both spells. Can't fit in the same slot.
						}
						else if (!is_spell && itemCategory(tempItem) != SPELL_CAT)
						{
							notfree = true;  //Both not spells. Can't fit in the same slot.
						}
					}
				}
			}
			if ( notfree )
			{
				notfree = false;
				continue;
			}
			item.x = x;
			item.y = y;
			foundaspot = true;
			break;
		}
		if ( foundaspot )
		{
			break;
		}
		++x;
	}

	// add the item to the hotbar automatically
	if ( !intro && auto_hotbar_new_items )
	{
		if ( &inventory == &stats[clientnum]->inventory )
		{
			int c;
			for ( c = 0; c < NUM_HOTBAR_SLOTS; c++ )
			{
				if ( !uidToItem(hotbar[c].item) )
				{
					hotbar[c].item = item.uid;
					break;
				}
			}
		}
	}
}

/*-------------------------------------------------------------------------------

	uidToItem

	returns an item from the player's inventory from the given uid

-------------------------------------------------------------------------------*/

Item* uidToItem(Uint32 uid)
{
	node_t* node;
	for ( node = stats[clientnum]->inventory.first; node != NULL; node = node->next )
	{
		Item* item = (Item*)node->element;
		if ( item->uid == uid )
		{
			return item;
		}
	}
	return NULL;
}

/*-------------------------------------------------------------------------------

	itemCurve

	Selects an item type from the given category of items by factoring in
	dungeon level, value of the item, etc.

-------------------------------------------------------------------------------*/

ItemType itemCurve(Category cat)
{
	int numitems = NUMITEMS - ( NUMITEMS - ((int)ARTIFACT_SWORD) );
	bool chances[NUMITEMS];
	int c;

	if ( cat < 0 || cat >= NUMCATEGORIES )
	{
		printlog("warning: itemCurve() called with bad category value!\n");
		return GEM_ROCK;
	}

	// find highest value of items in category
	Uint32 highestvalue = 0;
	Uint32 lowestvalue = 0;
	Uint32 numoftype = 0;
	for ( c = 0; c < numitems; c++ )
	{
		if ( items[c].category == cat )
		{
			highestvalue = std::max<Uint32>(highestvalue, items[c].value); //TODO: Why are Uint32 and int being compared?
			lowestvalue = std::min<Uint32>(lowestvalue, items[c].value); //TODO: Why are Uint32 and int being compared?
			numoftype++;
		}
	}
	if ( numoftype == 0 )
	{
		printlog("warning: category passed to itemCurve has no items!\n");
		return GEM_ROCK;
	}

	if ( cat == SCROLL || cat == POTION || cat == BOOK )
	{
		// these item categories will spawn anything of their type
		for ( c = 0; c < numitems; c++ )
		{
			chances[c] = false;
			if ( items[c].category == cat )
			{
				chances[c] = true;
			}
		}
	}
	else if ( cat == TOOL )
	{
		// this category will spawn specific items more frequently regardless of level
		for ( c = 0; c < numitems; c++ )
		{
			chances[c] = false;
			if ( items[c].category == cat )
			{
				switch ( (ItemType)c )
				{
					case TOOL_TINOPENER:
						if ( prng_get_uint() % 2 )   // 50% chance
						{
							chances[c] = true;
						}
						break;
					case TOOL_LANTERN:
						if ( prng_get_uint() % 4 )   // 75% chance
						{
							chances[c] = true;
						}
						break;
					case TOOL_SKELETONKEY:
						chances[c] = false; // 0% chance
						break;
					default:
						chances[c] = true;
						break;
				}
			}
		}
	}
	else
	{
		// other categories get a special chance algorithm based on item value and dungeon level
		int acceptablehigh = std::max<Uint32>(highestvalue * fmin(1.0, (currentlevel + 10) / 25.0), lowestvalue); //TODO: Why are double and Uint32 being compared?
		for ( c = 0; c < numitems; c++ )
		{
			chances[c] = false;
			if ( items[c].category == cat && items[c].value <= acceptablehigh )
			{
				chances[c] = true;
			}
		}
	}

	// calculate number of items left
	Uint32 numleft = 0;
	for ( c = 0; c < numitems; c++ )
	{
		if ( chances[c] == true )
		{
			numleft++;
		}
	}
	if ( numleft == 0 )
	{
		return GEM_ROCK;
	}

	// most gems are worthless pieces of glass
	if ( cat == GEM )
	{
		if ( prng_get_uint() % 10 )
		{
			return GEM_GLASS;
		}
	}

	// pick the item
	int pick = prng_get_uint() % numleft;
	for ( c = 0; c < numitems; c++ )
	{
		if ( items[c].category == cat )
		{
			if ( chances[c] == true )
			{
				if ( pick == 0 )
				{
					return static_cast<ItemType>(c);
				}
				else
				{
					pick--;
				}
			}
		}
	}

	return GEM_ROCK;
}

/*-------------------------------------------------------------------------------

itemLevelCurve

Selects an item type from the given category of items by factoring in
dungeon level and defined level of the item

-------------------------------------------------------------------------------*/

ItemType itemLevelCurve(Category cat, int minLevel, int maxLevel)
{
	int numitems = NUMITEMS;
	bool chances[NUMITEMS];
	int c;

	if ( cat < 0 || cat >= NUMCATEGORIES )
	{
		printlog("warning: itemLevelCurve() called with bad category value!\n");
		return GEM_ROCK;
	}

	Uint32 numoftype = 0;
	for ( c = 0; c < numitems; ++c )
	{
		chances[c] = false;
		if ( items[c].category == cat )
		{
			if ( items[c].level != -1 && (items[c].level >= minLevel && items[c].level <= maxLevel) )
			{
				chances[c] = true;
				numoftype++;
				if ( cat == TOOL )
				{
					switch ( (ItemType)c )
					{
						case TOOL_TINOPENER:
							if ( prng_get_uint() % 2 )   // 50% chance
							{
								chances[c] = false;
							}
							break;
						case TOOL_LANTERN:
							if ( prng_get_uint() % 4 == 0 )   // 25% chance
							{
								chances[c] = false;
							}
							break;
						default:
							break;
					}
				}
				else if ( cat == ARMOR )
				{
					switch ( (ItemType)c )
					{
						case CLOAK_BACKPACK:
							if ( prng_get_uint() % 4 )   // 25% chance
							{
								chances[c] = false;
							}
							break;
						default:
							break;
					}
				}
			}
		}
	}
	if ( numoftype == 0 )
	{
		printlog("warning: category passed to itemLevelCurve has no items!\n");
		return GEM_ROCK;
	}

	// calculate number of items left
	Uint32 numleft = 0;
	for ( c = 0; c < numitems; c++ )
	{
		if ( chances[c] == true )
		{
			numleft++;
		}
	}
	if ( numleft == 0 )
	{
		return GEM_ROCK;
	}

	// most gems are worthless pieces of glass
	if ( cat == GEM )
	{
		if ( prng_get_uint() % 10 )
		{
			return GEM_GLASS;
		}
	}

	// pick the item
	int pick = prng_get_uint() % numleft;
	for ( c = 0; c < numitems; c++ )
	{
		if ( items[c].category == cat )
		{
			if ( chances[c] == true )
			{
				if ( pick == 0 )
				{
					//messagePlayer(0, "Chose item: %s of %d items.", items[c].name_identified ,numleft);
					return static_cast<ItemType>(c);
				}
				else
				{
					pick--;
				}
			}
		}
	}

	return GEM_ROCK;
}

/*-------------------------------------------------------------------------------

	Item::description

	Returns a string that describes the given item's properties

-------------------------------------------------------------------------------*/

char* Item::description()
{
	int c = 0;

	if ( identified == true )
	{
		if ( count < 2 )
		{
			if ( itemCategory(this) == WEAPON || itemCategory(this) == ARMOR || itemCategory(this) == MAGICSTAFF || itemCategory(this) == TOOL || itemCategory(this) == THROWN )
			{
				snprintf(tempstr, 1024, language[982 + status], beatitude);
			}
			else if ( itemCategory(this) == AMULET || itemCategory(this) == RING || itemCategory(this) == GEM )
			{
				snprintf(tempstr, 1024, language[987 + status], beatitude);
			}
			else if ( itemCategory(this) == POTION )
			{
				if ( type == POTION_EMPTY )
				{
					//No fancy descriptives for empty potions.
					snprintf(tempstr, 1024, language[982 + status], beatitude);
				}
				else
				{
					snprintf(tempstr, 1024, language[992 + status], language[974 + items[type].index + appearance % items[type].variations - 50], beatitude);
				}
			}
			else if ( itemCategory(this) == SCROLL || itemCategory(this) == SPELLBOOK || itemCategory(this) == BOOK )
			{
				snprintf(tempstr, 1024, language[997 + status], beatitude);
			}
			else if ( itemCategory(this) == FOOD )
			{
				snprintf(tempstr, 1024, language[1002 + status], beatitude);
			}

			for ( c = 0; c < 1024; ++c )
			{
				if ( tempstr[c] == 0 )
				{
					break;
				}
			}

			if ( type >= 0 && type < NUMITEMS )
			{
				if ( itemCategory(this) == BOOK )
				{
					snprintf(&tempstr[c], 1024 - c, language[1007], books[appearance % numbooks]->name);
				}
				else
				{
					snprintf(&tempstr[c], 1024 - c, "%s", items[type].name_identified);
				}
			}
			else
			{
				snprintf(&tempstr[c], 1024 - c, "ITEM%03d", type);
			}
		}
		else
		{
			if ( itemCategory(this) == WEAPON || itemCategory(this) == ARMOR || itemCategory(this) == MAGICSTAFF || itemCategory(this) == TOOL || itemCategory(this) == THROWN )
			{
				snprintf(tempstr, 1024, language[1008 + status], count, beatitude);
			}
			else if ( itemCategory(this) == AMULET || itemCategory(this) == RING || itemCategory(this) == GEM )
			{
				snprintf(tempstr, 1024, language[1013 + status], count, beatitude);
			}
			else if ( itemCategory(this) == POTION )
			{
				if ( type == POTION_EMPTY )
				{
					//No fancy descriptives for empty potions.
					snprintf(tempstr, 1024, language[1008 + status], count, beatitude);
				}
				else
				{
					snprintf(tempstr, 1024, language[1018 + status], count, language[974 + items[type].index + appearance % items[type].variations - 50], beatitude);
				}
			}
			else if ( itemCategory(this) == SCROLL || itemCategory(this) == SPELLBOOK || itemCategory(this) == BOOK )
			{
				snprintf(tempstr, 1024, language[1023 + status], count, beatitude);
			}
			else if ( itemCategory(this) == FOOD )
			{
				snprintf(tempstr, 1024, language[1028 + status], count, beatitude);
			}

			for ( c = 0; c < 1024; ++c )
			{
				if ( tempstr[c] == 0 )
				{
					break;
				}
			}

			if ( type >= 0 && type < NUMITEMS )
			{
				if ( itemCategory(this) == BOOK )
				{
					snprintf(&tempstr[c], 1024 - c, language[1033], count, books[appearance % numbooks]->name);
				}
				else
				{
					snprintf(&tempstr[c], 1024 - c, "%s", items[type].name_identified);
				}
			}
			else
			{
				snprintf(&tempstr[c], 1024 - c, "ITEM%03d", type);
			}
		}
	}
	else
	{
		if ( count < 2 )
		{
			if ( itemCategory(this) == WEAPON || itemCategory(this) == ARMOR || itemCategory(this) == MAGICSTAFF || itemCategory(this) == TOOL || itemCategory(this) == THROWN )
			{
				strncpy(tempstr, language[1034 + status], 1024);
			}
			else if ( itemCategory(this) == AMULET || itemCategory(this) == RING || itemCategory(this) == GEM )
			{
				strncpy(tempstr, language[1039 + status], 1024);
			}
			else if ( itemCategory(this) == POTION )
			{
				if ( type == POTION_EMPTY )
				{
					//No fancy descriptives for empty potions.
					snprintf(tempstr, 1024, language[1034 + status], beatitude);
				}
				else
				{
					snprintf(tempstr, 1024, language[1044 + status], language[974 + items[type].index + appearance % items[type].variations - 50]);
				}
			}
			else if ( itemCategory(this) == SCROLL || itemCategory(this) == SPELLBOOK || itemCategory(this) == BOOK )
			{
				strncpy(tempstr, language[1049 + status], 1024);
			}
			else if ( itemCategory(this) == FOOD )
			{
				strncpy(tempstr, language[1054 + status], 1024);
			}

			for ( c = 0; c < 1024; ++c )
			{
				if ( tempstr[c] == 0 )
				{
					break;
				}
			}

			if ( type >= 0 && type < NUMITEMS )
			{
				if ( itemCategory(this) == SCROLL )
				{
					snprintf(&tempstr[c], 1024 - c, language[1059], items[type].name_unidentified, scroll_label[appearance % NUMLABELS]);
				}
				else
				{
					if ( itemCategory(this) == BOOK )
					{
						snprintf(&tempstr[c], 1024 - c, language[1007], books[appearance % numbooks]->name);
					}
					else
					{
						snprintf(&tempstr[c], 1024 - c, "%s", items[type].name_unidentified);
					}
				}
			}
			else
			{
				snprintf(&tempstr[c], 1024 - c, "ITEM%03d", type);
			}
		}
		else
		{
			if ( itemCategory(this) == WEAPON || itemCategory(this) == ARMOR || itemCategory(this) == MAGICSTAFF || itemCategory(this) == TOOL || itemCategory(this) == THROWN )
			{
				snprintf(tempstr, 1024, language[1060 + status], count);
			}
			else if ( itemCategory(this) == AMULET || itemCategory(this) == RING || itemCategory(this) == GEM )
			{
				snprintf(tempstr, 1024, language[1065 + status], count);
			}
			else if ( itemCategory(this) == POTION )
			{
				if ( type == POTION_EMPTY )
				{
					//No fancy descriptives for empty potions.
					snprintf(tempstr, 1024, language[1060 + status], count);
				}
				else
				{
					snprintf(tempstr, 1024, language[1070 + status], count, language[974 + items[type].index + appearance % items[type].variations - 50]);
				}
			}
			else if ( itemCategory(this) == SCROLL || itemCategory(this) == SPELLBOOK || itemCategory(this) == BOOK )
			{
				snprintf(tempstr, 1024, language[1075 + status], count);
			}
			else if ( itemCategory(this) == FOOD )
			{
				snprintf(tempstr, 1024, language[1080 + status], count);
			}

			for ( c = 0; c < 1024; ++c )
			{
				if ( tempstr[c] == 0 )
				{
					break;
				}
			}

			if ( type >= 0 && type < NUMITEMS )
			{
				if ( itemCategory(this) == SCROLL )
				{
					snprintf(&tempstr[c], 1024 - c, language[1085], count, items[type].name_unidentified, scroll_label[appearance % NUMLABELS]);
				}
				else
				{
					if ( itemCategory(this) == BOOK )
					{
						snprintf(&tempstr[c], 1024 - c, language[1086], count, books[appearance % numbooks]->name);
					}
					else
					{
						snprintf(&tempstr[c], 1024 - c, "%s", items[type].name_unidentified);
					}
				}
			}
			else
			{
				snprintf(&tempstr[c], 1024 - c, "ITEM%03d", type);
			}
		}
	}
	return tempstr;
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

	Item::getName

	Returns the name of an item type as a character string

-------------------------------------------------------------------------------*/

char* Item::getName()
{
	if ( type >= 0 && type < NUMITEMS )
	{
		if ( identified )
		{
			if ( itemCategory(this) == BOOK )
			{
				snprintf(tempstr, sizeof(tempstr), language[1007], books[appearance % numbooks]->name);
			}
			else
			{
				strcpy(tempstr, items[type].name_identified);
			}
		}
		else
		{
			if ( itemCategory(this) == SCROLL )
			{
				snprintf(tempstr, sizeof(tempstr), language[1059], items[type].name_unidentified, scroll_label[appearance % NUMLABELS]);
			}
			else if ( itemCategory(this) == BOOK )
			{
				snprintf(tempstr, sizeof(tempstr), language[1007], books[appearance % numbooks]->name);
			}
			else
			{
				strcpy(tempstr, items[type].name_unidentified);
			}
		}
	}
	else
	{
		snprintf(tempstr, sizeof(tempstr), "ITEM%03d", type);
	}
	return tempstr;
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
	if (itemCategory(item) == SPELL_CAT)
	{
		spell_t* spell = getSpellFromItem(item);
		if (spell)
		{
			node_t* node = list_Node(&items[item->type].surfaces, spell->ID);
			if ( !node )
			{
				return NULL;
			}
			SDL_Surface** surface = (SDL_Surface**)node->element;
			return *surface;
		}
	}
	else
	{
		node_t* node = list_Node(&items[item->type].surfaces, item->appearance % items[item->type].variations);
		if ( !node )
		{
			return NULL;
		}
		SDL_Surface** surface = (SDL_Surface**)node->element;
		return *surface;
	}
	return NULL;
}

/*-------------------------------------------------------------------------------

	itemCompare

	Compares two items and returns 0 if they are identical or 1 if they are
	not identical. Item count is excluded during comparison testing.

-------------------------------------------------------------------------------*/

int itemCompare(const Item* item1, const Item* item2, bool checkAppearance)
{
	Sint32 model1 = 0;
	Sint32 model2 = 0;

	// null cases
	if ( item1 == NULL )
	{
		if ( item2 == NULL )
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		if ( item2 == NULL )
		{
			return 1;
		}
	}

	// check attributes
	if (item1->type != item2->type)
	{
		return 1;
	}
	if (item1->status != item2->status)
	{
		return 1;
	}
	if (item1->beatitude != item2->beatitude)
	{
		return 1;
	}
	model1 = items[item1->type].index + item1->appearance % items[item1->type].variations;
	model2 = items[item2->type].index + item2->appearance % items[item2->type].variations;
	//messagePlayer(0, "item1- %d, item2 - %d", model1, model2);
	if ( model1 != model2 )
	{
		return 1;
	}
	else if ( item1->type == SCROLL_MAIL || item1->type == READABLE_BOOK || items[item1->type].category == SPELL_CAT )
	{
		return 1; // these items do not stack
	}
	if (item1->identified != item2->identified)
	{
		return 1;
	}
	if ( checkAppearance && (item1->appearance != item2->appearance) )
	{
		return 1;
	}

	// items are identical
	return 0;
}

/*-------------------------------------------------------------------------------

	dropItem

	Handles the client impulse to drop an item

-------------------------------------------------------------------------------*/

void dropItem(Item* item, int player)
{
	if (!item)
	{
		return;
	}

	Entity* entity;
	Sint16 oldcount;

	if (item == nullptr || players[player] == nullptr || players[player]->entity == nullptr || itemCategory(item) == SPELL_CAT)
	{
		return;
	}
	if ( itemIsEquipped(item, player) )
	{
		if (!item->canUnequip(stats[player]))
		{
			if ( shouldInvertEquipmentBeatitude(stats[player]) && item->beatitude > 0 )
			{
				messagePlayer(player, language[3218]);
			}
			else
			{
				messagePlayer(player, language[1087]);
			}
			return;
		}
	}

	if ( multiplayer == CLIENT )
	{
		strcpy((char*)net_packet->data, "DROP");
		SDLNet_Write32((Uint32)item->type, &net_packet->data[4]);
		SDLNet_Write32((Uint32)item->status, &net_packet->data[8]);
		SDLNet_Write32((Uint32)item->beatitude, &net_packet->data[12]);
		SDLNet_Write32((Uint32)item->count, &net_packet->data[16]);
		SDLNet_Write32((Uint32)item->appearance, &net_packet->data[20]);
		net_packet->data[24] = item->identified;
		net_packet->data[25] = clientnum;
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 26;
		sendPacketSafe(net_sock, -1, net_packet, 0);
		if (item == open_book_item)
		{
			closeBookGUI();
		}

		oldcount = item->count;
		item->count = 1;
		messagePlayer(player, language[1088], item->description());
		item->count = oldcount - 1;

		// unequip the item
		if ( item->count <= 1 )
		{
			Item** slot = itemSlot(stats[player], item);
			if ( slot != NULL )
			{
				*slot = NULL;
			}
		}

		if ( item->count <= 0 )
		{
			list_RemoveNode(item->node);
		}
	}
	else
	{
		if (item == open_book_item)
		{
			closeBookGUI();
		}
		entity = newEntity(-1, 1, map.entities, nullptr); //Item entity.
		entity->flags[INVISIBLE] = true;
		entity->flags[UPDATENEEDED] = true;
		entity->x = players[player]->entity->x;
		entity->y = players[player]->entity->y;
		entity->sizex = 4;
		entity->sizey = 4;
		entity->yaw = players[player]->entity->yaw;
		entity->vel_x = (1.5 + .025 * (rand() % 11)) * cos(players[player]->entity->yaw);
		entity->vel_y = (1.5 + .025 * (rand() % 11)) * sin(players[player]->entity->yaw);
		entity->vel_z = (-10 - rand() % 20) * .01;
		entity->flags[PASSABLE] = true;
		entity->behavior = &actItem;
		entity->skill[10] = item->type;
		entity->skill[11] = item->status;
		entity->skill[12] = item->beatitude;
		entity->skill[13] = 1;
		entity->skill[14] = item->appearance;
		entity->skill[15] = item->identified;
		entity->parent = players[player]->entity->getUID();
		entity->itemOriginalOwner = entity->parent;

		// play sound
		playSoundEntity( players[player]->entity, 47 + rand() % 3, 64 );

		// unequip the item
		Item** slot = itemSlot(stats[player], item);
		if ( slot != NULL )
		{
			*slot = NULL;
		}
		if ( item->node != NULL )
		{
			if ( item->node->list == &stats[0]->inventory )
			{
				oldcount = item->count;
				item->count = 1;
				messagePlayer(player, language[1088], item->description());
				item->count = oldcount - 1;
				if ( item->count <= 0 )
				{
					list_RemoveNode(item->node);
				}
			}
		}
		else
		{
			item->count--;
			if ( item->count <= 0 )
			{
				free(item);
			}
		}
	}
}

Entity* dropItemMonster(Item* item, Entity* monster, Stat* monsterStats, Sint16 count)
{
	Entity* entity = nullptr;
	bool itemDroppable = true;

	if ( !item || !monster )
	{
		return nullptr;
	}

	/*if ( monsterStats->type == SHADOW && itemCategory(item) == SPELLBOOK )
	{
		//Shadows don't drop spellbooks.
		itemDroppable = false;
	}*/
	if ( monsterStats )
	{
		if ( monsterStats->type == SKELETON && monster->behavior == &actMonster && monster->monsterAllySummonRank != 0 )
		{
			itemDroppable = false;
		}

		if ( item->appearance == MONSTER_ITEM_UNDROPPABLE_APPEARANCE )
		{
			if ( monsterStats->type == SHADOW || monsterStats->type == AUTOMATON )
			{
				itemDroppable = false;
			}

			if ( (monsterStats->type == KOBOLD
				|| monsterStats->type == COCKATRICE
				|| monsterStats->type == INSECTOID
				|| monsterStats->type == INCUBUS
				|| monsterStats->type == VAMPIRE
				|| monsterStats->type == SUCCUBUS)
				&& (itemCategory(item) == SPELLBOOK || itemCategory(item) == MAGICSTAFF) )
			{
				// monsters with special spell attacks won't drop their book.
				itemDroppable = false;
			}
			if ( monsterStats->type == INSECTOID && itemCategory(item) == THROWN )
			{
				// insectoids won't drop their un-thrown daggers.
				itemDroppable = false;
			}
			if ( monsterStats->type == INCUBUS && itemCategory(item) == POTION )
			{
				// incubus won't drop excess potions.
				itemDroppable = false;
			}
			if ( monsterStats->type == GOATMAN && (itemCategory(item) == POTION || itemCategory(item) == SPELLBOOK) )
			{
				// goatman sometimes won't drop excess potions.
				itemDroppable = false;
			}
		}
		else if ( monsterStats->HP <= 0 )
		{
			// we're dropping the item on death.
			switch ( itemCategory(item) )
			{
				case WEAPON:
				case ARMOR:
				case THROWN:
					if ( item->status == BROKEN )
					{
						itemDroppable = false;
					}
					break;
				default:
					break;
			}
			if ( monster->behavior == &actPlayer )
			{
				if ( item->type >= ARTIFACT_SWORD && item->type <= ARTIFACT_GLOVES )
				{
					for ( int c = 1; c < MAXPLAYERS; ++c )
					{
						if ( players[c] && players[c]->entity && players[c]->entity == monster )
						{
							if ( itemIsEquipped(item, c) )
							{
								steamAchievementClient(c, "BARONY_ACH_CHOSEN_ONE");
							}
							break;
						}
					}
				}
			}
		}
	}

	count = std::min(count, item->count);

	if ( itemDroppable )
	{
		//TODO: Spawn multiple entities for count...
		entity = newEntity(-1, 1, map.entities, nullptr); //Item entity.
		entity->flags[INVISIBLE] = true;
		entity->flags[UPDATENEEDED] = true;
		entity->x = monster->x;
		entity->y = monster->y;
		entity->sizex = 4;
		entity->sizey = 4;
		entity->yaw = monster->yaw;
		entity->vel_x = (rand() % 20 - 10) / 10.0;
		entity->vel_y = (rand() % 20 - 10) / 10.0;
		entity->vel_z = -.5;
		entity->flags[PASSABLE] = true;
		entity->flags[USERFLAG1] = true; // speeds up game when many items are dropped
		entity->behavior = &actItem;
		entity->skill[10] = item->type;
		entity->skill[11] = item->status;
		entity->skill[12] = item->beatitude;
		entity->skill[13] = count;
		entity->skill[14] = item->appearance;
		entity->skill[15] = item->identified;
		entity->itemOriginalOwner = item->ownerUid;
		entity->parent = monster->getUID();

		if ( monsterStats && (monsterStats->type == INCUBUS || monsterStats->type == SUCCUBUS) )
		{
			// check if item was stolen.
			for ( int c = 0; c < MAXPLAYERS; ++c )
			{
				if ( players[c] && players[c]->entity )
				{
					if ( entity->itemOriginalOwner == players[c]->entity->getUID() )
					{
						entity->itemStolen = 1;
						break;
					}
				}
			}
		}
	}

	item->count -= count;
	if ( item->count <= 0 )
	{
		Item** slot;
		if ( (slot = itemSlot(monsterStats, item)) != nullptr )
		{
			*slot = nullptr; // clear the item slot
		}

		if ( item->node )
		{
			list_RemoveNode(item->node);
		}
		else
		{
			free(item);
		}
	}

	return entity;
}

/*-------------------------------------------------------------------------------

	consumeItem

	consumes an item

-------------------------------------------------------------------------------*/

void consumeItem(Item*& item, int player)
{
	if ( item == nullptr )
	{
		return;
	}
	if ( appraisal_item == item->uid && item->count == 1 )
	{
		appraisal_item = 0;
		appraisal_timer = 0;
	}

	if ( player > 0 && multiplayer == SERVER )
	{
		Item** slot = nullptr;
		if ( (slot = itemSlot(stats[player], item)) != nullptr )
		{
			(*slot)->count--; // if client had consumed item equipped, this'll update the count.
		}
	}

	item->count--;
	if ( item->count <= 0 )
	{
		if ( item->node != nullptr )
		{
			int i;
			for ( i = 0; i < MAXPLAYERS; i++ )
			{
				if ( item->node->list == &stats[i]->inventory )
				{
					Item** slot;
					if ( (slot = itemSlot(stats[i], item)) != nullptr )
					{
						*slot = nullptr;
					}
				}
			}
			list_RemoveNode(item->node);
		}
		else
		{
			free(item);
		}
		item = nullptr;
	}
}

/*-------------------------------------------------------------------------------

	equipItem

	Handles the client impulse to equip an item

-------------------------------------------------------------------------------*/

void equipItem(Item* item, Item** slot, int player)
{
	int oldcount;

	if ( pickaxeGimpTimer > 0 )
	{
		return;
	}

	if (!item)   // needs "|| !slot " ?
	{
		return;
	}

	if ( player == clientnum && multiplayer != SINGLE && swapWeaponGimpTimer > 0
		&& (itemCategory(item) == POTION || itemCategory(item) == GEM || itemCategory(item) == THROWN) )
	{
		return;
	}

	if ( itemCompare(*slot, item, true) )
	{
		// if items are different... (excluding the quantity of both item nodes)
		if ( *slot != NULL )
		{
			if (!(*slot)->canUnequip(stats[player]))
			{
				if ( item->type == ARTIFACT_ORB_PURPLE && !strncmp(map.name, "Boss", 4) )
				{
					// can unequip anything when trying to equip the dang orb.
				}
				else
				{
					if ( player == clientnum )
					{
						if ( shouldInvertEquipmentBeatitude(stats[player]) && item->beatitude > 0 )
						{
							messagePlayer(player, language[3217], (*slot)->getName());
						}
						else
						{
							messagePlayer(player, language[1089], (*slot)->getName());
						}
					}
					(*slot)->identified = true;
					return;
				}
			}
		}
		if ( multiplayer != CLIENT && !intro && !fadeout )
		{
			if ( players[player] != nullptr && players[player]->entity != nullptr)
			{
				if (players[player]->entity->ticks > 60)
				{
					if ( itemCategory(item) == AMULET || itemCategory(item) == RING )
					{
						playSoundEntity(players[player]->entity, 33 + rand() % 2, 64);
					}
					else if ( itemCategory(item) == WEAPON || itemCategory(item) == THROWN )
					{
						playSoundEntity(players[player]->entity, 40 + rand() % 4, 64);
					}
					else if ( itemCategory(item) == ARMOR )
					{
						playSoundEntity(players[player]->entity, 44 + rand() % 3, 64);
					}
					else if ( item->type == TOOL_TORCH || item->type == TOOL_LANTERN || item->type == TOOL_CRYSTALSHARD )
					{
						playSoundEntity(players[player]->entity, 134, 64);
					}
				}
			}
		}
		if ( multiplayer == SERVER && player > 0 )
		{
			if ( *slot != NULL )
			{
				if ( (*slot)->node )
				{
					list_RemoveNode((*slot)->node);
				}
				else
				{
					free(*slot);
				}
			}
		}
		else
		{
			oldcount = item->count;
			item->count = 1;
			if ( intro == false )
			{
				messagePlayer(player, language[1090], item->description());
			}
			item->count = oldcount;
		}
		*slot = item;
		if ( player == clientnum )
		{
			if ( slot == &stats[player]->weapon )
			{
				weaponSwitch = true;
			}
			else if ( slot == &stats[player]->shield )
			{
				shieldSwitch = true;
			}
		}
	}
	else
	{
		// if items are the same... (excluding the quantity of both item nodes)
		if ( *slot != NULL )
		{
			if ( (*slot)->count == item->count ) // if quantity is the same then it's the same item, can unequip
			{
				if (!(*slot)->canUnequip(stats[player]))
				{
					if ( player == clientnum )
					{
						if ( shouldInvertEquipmentBeatitude(stats[player]) && item->beatitude > 0 )
						{
							messagePlayer(player, language[3217], (*slot)->getName());
						}
						else
						{
							messagePlayer(player, language[1089], (*slot)->getName());
						}
					}
					(*slot)->identified = true;
					return;
				}
			}
			else
			{
				// This lets the server know when a client "equipped" a new item in their slot but actually just updated the count.
				// Otherwise if this count check were not here, server would think that equipping 2 rocks after only holding 1 rock is
				// the same as unequipping the slot since they are the same item, barring the quantity. So the client would appear to
				// the server as empty handed, while the client holds 2 rocks, and when thrown on client end, the server never sees the item
				// and the client "throws" nothing, but actually loses their thrown items into nothingness. This fixes that issue.
				(*slot)->count = item->count; // update quantity. 
				return;
			}
		}
		if (multiplayer != CLIENT && !intro && !fadeout)
		{
			if (players[player] != nullptr && players[player]->entity != nullptr)
			{
				if (players[player]->entity->ticks > 60)
				{
					if (itemCategory(item) == ARMOR)
					{
						playSoundEntity(players[player]->entity, 44 + rand() % 3, 64);
					}
				}
			}
		}
		if ( player != 0 && multiplayer == SERVER )
		{
			if ( item->node )
			{
				list_RemoveNode(item->node);
			}
			else
			{
				free(item);
			}
			if ( *slot != NULL )
			{
				if ( (*slot)->node )
				{
					list_RemoveNode((*slot)->node);
				}
				else
				{
					free(*slot);
				}
			}
		}
		else
		{
			oldcount = item->count;
			item->count = 1;
			if ( intro == false && !fadeout )
			{
				messagePlayer(player, language[1091], item->description());
			}
			item->count = oldcount;
		}
		*slot = NULL;
	}
}

/*-------------------------------------------------------------------------------

	useItem

	Handles the client impulse to use an item

-------------------------------------------------------------------------------*/

void useItem(Item* item, int player, Entity* usedBy)
{
	if ( item == NULL )
	{
		return;
	}

	if ( !usedBy && player >= 0 && player < MAXPLAYERS && players[player] && players[player]->entity )
	{
		// assume used by the player unless otherwise (a fountain potion effect e.g)
		usedBy = players[player]->entity;
	}

	if ( openedChest[player] && itemCategory(item) != SPELL_CAT ) //TODO: What if fountain called this function for its potion effect?
	{
		//If a chest is open, put the item in the chest.
		openedChest[player]->addItemToChestFromInventory(player, item, false);
		return;
	}
	else if ( gui_mode == GUI_MODE_SHOP && player == clientnum && itemCategory(item) != SPELL_CAT) //TODO: What if fountain called this function for its potion effect?
	{
		bool deal = true;
		switch ( shopkeepertype )
		{
			case 0: // arms & armor
				if ( itemCategory(item) != WEAPON && itemCategory(item) != ARMOR && itemCategory(item) != THROWN )
				{
					deal = false;
				}
				break;
			case 1: // hats
				if ( itemCategory(item) != ARMOR )
				{
					deal = false;
				}
				break;
			case 2: // jewelry
				if ( itemCategory(item) != RING && itemCategory(item) != AMULET && itemCategory(item) != GEM )
				{
					deal = false;
				}
				break;
			case 3: // bookstore
				if ( itemCategory(item) != SPELLBOOK && itemCategory(item) != SCROLL && itemCategory(item) != BOOK )
				{
					deal = false;
				}
				break;
			case 4: // potion shop
				if ( itemCategory(item) != POTION )
				{
					deal = false;
				}
				break;
			case 5: // magicstaffs
				if ( itemCategory(item) != MAGICSTAFF )
				{
					deal = false;
				}
				break;
			case 6: // food
				if ( itemCategory(item) != FOOD )
				{
					deal = false;
				}
				break;
			case 7: // tools
			case 8: // lights
				if ( itemCategory(item) != TOOL )
				{
					deal = false;
				}
				break;
			default:
				break;
		}
		if ( deal )
		{
			sellitem = item;
			shopspeech = language[215];
			shoptimer = ticks - 1;
		}
		else
		{
			shopspeech = language[212 + rand() % 3];
			shoptimer = ticks - 1;
		}
		return;
	}

	if ( item->status == BROKEN && player == clientnum )
	{
		messagePlayer(player, language[1092], item->getName());
		return;
	}

	// tins need a tin opener to open...
	if ( player == clientnum && !(stats[player]->type == GOATMAN) )
	{
		if ( item->type == FOOD_TIN )
		{
			bool havetinopener = false;
			node_t* node;
			for ( node = stats[clientnum]->inventory.first; node != NULL; node = node->next )
			{
				Item* tempitem = (Item*)node->element;
				if ( tempitem->type == TOOL_TINOPENER )
				{
					if ( tempitem->status != BROKEN )
					{
						havetinopener = true;
						break;
					}
				}
			}
			if ( !havetinopener )
			{
				messagePlayer(clientnum, language[1093]);
				return;
			}
		}
	}

	if ( multiplayer == CLIENT && !intro )
	{
		if ( swapWeaponGimpTimer > 0
			&& ( itemCategory(item) == GEM || itemCategory(item) == THROWN) )
		{
			// don't send to host as we're not allowed to "use" or equip these items. 
			// will return false in equipItem.
			// potions allowed here because we're drinking em.
		}
		else
		{
			strcpy((char*)net_packet->data, "USEI");
			SDLNet_Write32((Uint32)item->type, &net_packet->data[4]);
			SDLNet_Write32((Uint32)item->status, &net_packet->data[8]);
			SDLNet_Write32((Uint32)item->beatitude, &net_packet->data[12]);
			SDLNet_Write32((Uint32)item->count, &net_packet->data[16]);
			SDLNet_Write32((Uint32)item->appearance, &net_packet->data[20]);
			net_packet->data[24] = item->identified;
			net_packet->data[25] = clientnum;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 26;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
	}

	bool drankPotion = false;
	bool tryLearnPotionRecipe = false;
	bool tryEmptyBottle = (item->status >= SERVICABLE);
	ItemType potionType = item->type;
	if ( player == clientnum )
	{
		if ( itemCategory(item) == POTION && item->type != POTION_EMPTY && usedBy
			&& (players[clientnum] && players[clientnum]->entity)
			&& players[clientnum]->entity == usedBy )
		{
			if ( item->identified )
			{
				tryLearnPotionRecipe = true;
			}
		}
	}

	switch ( item->type )
	{
		case WOODEN_SHIELD:
			equipItem(item, &stats[player]->shield, player);
			break;
		case QUARTERSTAFF:
		case BRONZE_SWORD:
		case BRONZE_MACE:
		case BRONZE_AXE:
			equipItem(item, &stats[player]->weapon, player);
			break;
		case BRONZE_SHIELD:
			equipItem(item, &stats[player]->shield, player);
			break;
		case SLING:
		case IRON_SPEAR:
		case IRON_SWORD:
		case IRON_MACE:
		case IRON_AXE:
			equipItem(item, &stats[player]->weapon, player);
			break;
		case IRON_SHIELD:
			equipItem(item, &stats[player]->shield, player);
			break;
		case SHORTBOW:
		case STEEL_HALBERD:
		case STEEL_SWORD:
		case STEEL_MACE:
		case STEEL_AXE:
		case CRYSTAL_SWORD:
		case CRYSTAL_SPEAR:
		case CRYSTAL_BATTLEAXE:
		case CRYSTAL_MACE:
		case BRONZE_TOMAHAWK:
		case IRON_DAGGER:
		case STEEL_CHAKRAM:
		case CRYSTAL_SHURIKEN:
			equipItem(item, &stats[player]->weapon, player);
			break;
		case STEEL_SHIELD:
		case STEEL_SHIELD_RESISTANCE:
		case MIRROR_SHIELD:
		case CRYSTAL_SHIELD:
			equipItem(item, &stats[player]->shield, player);
			break;
		case CROSSBOW:
			equipItem(item, &stats[player]->weapon, player);
			break;
		case GLOVES:
		case GLOVES_DEXTERITY:
		case BRACERS:
		case BRACERS_CONSTITUTION:
		case GAUNTLETS:
		case GAUNTLETS_STRENGTH:
		case ARTIFACT_GLOVES:
		case CRYSTAL_GLOVES:
		case BRASS_KNUCKLES:
		case IRON_KNUCKLES:
		case SPIKED_GAUNTLETS:
		case SUEDE_GLOVES:
			equipItem(item, &stats[player]->gloves, player);
			break;
		case CLOAK:
		case CLOAK_BLACK:
		case CLOAK_MAGICREFLECTION:
		case CLOAK_INVISIBILITY:
		case CLOAK_PROTECTION:
		case ARTIFACT_CLOAK:
		case CLOAK_BACKPACK:
		case CLOAK_SILVER:
			equipItem(item, &stats[player]->cloak, player);
			break;
		case LEATHER_BOOTS:
		case LEATHER_BOOTS_SPEED:
		case IRON_BOOTS:
		case IRON_BOOTS_WATERWALKING:
		case STEEL_BOOTS:
		case STEEL_BOOTS_LEVITATION:
		case STEEL_BOOTS_FEATHER:
		case ARTIFACT_BOOTS:
		case CRYSTAL_BOOTS:
		case SUEDE_BOOTS:
			equipItem(item, &stats[player]->shoes, player);
			break;
		case LEATHER_BREASTPIECE:
		case IRON_BREASTPIECE:
		case STEEL_BREASTPIECE:
		case CRYSTAL_BREASTPIECE:
		case VAMPIRE_DOUBLET:
		case WIZARD_DOUBLET:
		case HEALER_DOUBLET:
		case SILVER_DOUBLET:
		case ARTIFACT_BREASTPIECE:
		case TUNIC:
			equipItem(item, &stats[player]->breastplate, player);
			break;
		case HAT_PHRYGIAN:
		case HAT_HOOD:
		case HAT_WIZARD:
		case HAT_JESTER:
		case LEATHER_HELM:
		case IRON_HELM:
		case STEEL_HELM:
		case CRYSTAL_HELM:
		case ARTIFACT_HELM:
		case HAT_FEZ:
		case HAT_HOOD_RED:
		case HAT_HOOD_SILVER:
			equipItem(item, &stats[player]->helmet, player);
			break;
		case AMULET_SEXCHANGE:
			messagePlayer(player, language[1094]);
			item_AmuletSexChange(item, player);
			consumeItem(item, player);
			break;
		case AMULET_LIFESAVING:
		case AMULET_WATERBREATHING:
		case AMULET_MAGICREFLECTION:
			equipItem(item, &stats[player]->amulet, player);
			break;
		case AMULET_STRANGULATION:
			equipItem(item, &stats[player]->amulet, player);
			if ( stats[player]->amulet )
			{
				messagePlayer(player, language[1095]);
			}
			if ( item->beatitude >= 0 )
			{
				item->beatitude = -1;
			}
			break;
		case AMULET_POISONRESISTANCE:
			equipItem(item, &stats[player]->amulet, player);
			break;
		case POTION_WATER:
			drankPotion = item_PotionWater(item, players[player]->entity, usedBy);
			break;
		case POTION_BOOZE:
			drankPotion = item_PotionBooze(item, players[player]->entity, usedBy);
			break;
		case POTION_JUICE:
			drankPotion = item_PotionJuice(item, players[player]->entity, usedBy);
			break;
		case POTION_SICKNESS:
			drankPotion = item_PotionSickness(item, players[player]->entity, usedBy);
			break;
		case POTION_CONFUSION:
			drankPotion = item_PotionConfusion(item, players[player]->entity, usedBy);
			break;
		case POTION_EXTRAHEALING:
			drankPotion = item_PotionExtraHealing(item, players[player]->entity, usedBy);
			break;
		case POTION_HEALING:
			drankPotion = item_PotionHealing(item, players[player]->entity, usedBy);
			break;
		case POTION_CUREAILMENT:
			drankPotion = item_PotionCureAilment(item, players[player]->entity, usedBy);
			break;
		case POTION_BLINDNESS:
			drankPotion = item_PotionBlindness(item, players[player]->entity, usedBy);
			break;
		case POTION_RESTOREMAGIC:
			drankPotion = item_PotionRestoreMagic(item, players[player]->entity, usedBy);
			break;
		case POTION_INVISIBILITY:
			drankPotion = item_PotionInvisibility(item, players[player]->entity, usedBy);
			break;
		case POTION_LEVITATION:
			drankPotion = item_PotionLevitation(item, players[player]->entity, usedBy);
			break;
		case POTION_SPEED:
			drankPotion = item_PotionSpeed(item, players[player]->entity, usedBy);
			break;
		case POTION_ACID:
			drankPotion = item_PotionAcid(item, players[player]->entity, usedBy);
			break;
		case POTION_PARALYSIS:
			drankPotion = item_PotionParalysis(item, players[player]->entity, usedBy);
			break;
		case POTION_EMPTY:
			messagePlayer(player, language[2359]);
			break;
		case POTION_POLYMORPH:
		{
			int oldcount = item->count;
			item_PotionPolymorph(item, players[player]->entity, nullptr);
			if ( !item || (item && item->count < oldcount) )
			{
				drankPotion = true;
			}
			break;
		}
		case POTION_FIRESTORM:
		case POTION_ICESTORM:
		case POTION_THUNDERSTORM:
			drankPotion = item_PotionUnstableStorm(item, players[player]->entity, usedBy, nullptr);
			break;
		case POTION_STRENGTH:
			drankPotion = item_PotionStrength(item, players[player]->entity, usedBy);
			break;
		case SCROLL_MAIL:
			item_ScrollMail(item, player);
			break;
		case SCROLL_IDENTIFY:
			item_ScrollIdentify(item, player);
			if ( !players[player]->entity->isBlind() )
			{
				consumeItem(item, player);
			}
			break;
		case SCROLL_LIGHT:
			item_ScrollLight(item, player);
			if ( !players[player]->entity->isBlind() )
			{
				consumeItem(item, player);
			}
			break;
		case SCROLL_BLANK:
			item_ScrollBlank(item, player);
			break;
		case SCROLL_ENCHANTWEAPON:
			item_ScrollEnchantWeapon(item, player);
			if ( !players[player]->entity->isBlind() )
			{
				//consumeItem(item, player);
			}
			break;
		case SCROLL_ENCHANTARMOR:
			item_ScrollEnchantArmor(item, player);
			if ( !players[player]->entity->isBlind() )
			{
				//consumeItem(item, player);
			}
			break;
		case SCROLL_REMOVECURSE:
			item_ScrollRemoveCurse(item, player);
			if ( !players[player]->entity->isBlind() )
			{
				//consumeItem(item, player);
			}
			break;
		case SCROLL_FIRE:
			item_ScrollFire(item, player);
			if ( !players[player]->entity->isBlind() )
			{
				consumeItem(item, player);
			}
			break;
		case SCROLL_FOOD:
			item_ScrollFood(item, player);
			if ( !players[player]->entity->isBlind() )
			{
				consumeItem(item, player);
			}
			break;
		case SCROLL_MAGICMAPPING:
			item_ScrollMagicMapping(item, player);
			if ( !players[player]->entity->isBlind() )
			{
				consumeItem(item, player);
			}
			break;
		case SCROLL_REPAIR:
			item_ScrollRepair(item, player);
			break;
		case SCROLL_DESTROYARMOR:
			item_ScrollDestroyArmor(item, player);
			if ( !players[player]->entity->isBlind() )
			{
				//consumeItem(item, player);
			}
			break;
		case SCROLL_TELEPORTATION:
			item_ScrollTeleportation(item, player);
			if ( !players[player]->entity->isBlind() )
			{
				consumeItem(item, player);
			}
			break;
		case SCROLL_SUMMON:
			item_ScrollSummon(item, player);
			if ( !players[player]->entity->isBlind() )
			{
				consumeItem(item, player);
			}
			break;
		case MAGICSTAFF_LIGHT:
		case MAGICSTAFF_DIGGING:
		case MAGICSTAFF_LOCKING:
		case MAGICSTAFF_MAGICMISSILE:
		case MAGICSTAFF_OPENING:
		case MAGICSTAFF_SLOW:
		case MAGICSTAFF_COLD:
		case MAGICSTAFF_FIRE:
		case MAGICSTAFF_LIGHTNING:
		case MAGICSTAFF_SLEEP:
		case MAGICSTAFF_STONEBLOOD:
		case MAGICSTAFF_BLEED:
		case MAGICSTAFF_SUMMON:
		case MAGICSTAFF_CHARM:
			equipItem(item, &stats[player]->weapon, player);
			break;
		case RING_ADORNMENT:
		case RING_SLOWDIGESTION:
		case RING_PROTECTION:
		case RING_WARNING:
		case RING_STRENGTH:
		case RING_CONSTITUTION:
		case RING_INVISIBILITY:
		case RING_MAGICRESISTANCE:
		case RING_CONFLICT:
		case RING_LEVITATION:
		case RING_REGENERATION:
		case RING_TELEPORTATION:
			equipItem(item, &stats[player]->ring, player);
			break;
		case SPELLBOOK_FORCEBOLT:
		case SPELLBOOK_MAGICMISSILE:
		case SPELLBOOK_COLD:
		case SPELLBOOK_FIREBALL:
		case SPELLBOOK_LIGHTNING:
		case SPELLBOOK_REMOVECURSE:
		case SPELLBOOK_LIGHT:
		case SPELLBOOK_IDENTIFY:
		case SPELLBOOK_MAGICMAPPING:
		case SPELLBOOK_SLEEP:
		case SPELLBOOK_CONFUSE:
		case SPELLBOOK_SLOW:
		case SPELLBOOK_OPENING:
		case SPELLBOOK_LOCKING:
		case SPELLBOOK_LEVITATION:
		case SPELLBOOK_INVISIBILITY:
		case SPELLBOOK_TELEPORTATION:
		case SPELLBOOK_HEALING:
		case SPELLBOOK_EXTRAHEALING:
		case SPELLBOOK_CUREAILMENT:
		case SPELLBOOK_DIG:
		case SPELLBOOK_SUMMON:
		case SPELLBOOK_STONEBLOOD:
		case SPELLBOOK_BLEED:
		case SPELLBOOK_REFLECT_MAGIC:
		case SPELLBOOK_ACID_SPRAY:
		case SPELLBOOK_STEAL_WEAPON:
		case SPELLBOOK_DRAIN_SOUL:
		case SPELLBOOK_VAMPIRIC_AURA:
		case SPELLBOOK_CHARM_MONSTER:
			item_Spellbook(item, player);
			break;
		case GEM_ROCK:
		case GEM_LUCK:
		case GEM_GARNET:
		case GEM_RUBY:
		case GEM_JACINTH:
		case GEM_AMBER:
		case GEM_CITRINE:
		case GEM_JADE:
		case GEM_EMERALD:
		case GEM_SAPPHIRE:
		case GEM_AQUAMARINE:
		case GEM_AMETHYST:
		case GEM_FLUORITE:
		case GEM_OPAL:
		case GEM_DIAMOND:
		case GEM_JETSTONE:
		case GEM_OBSIDIAN:
		case GEM_GLASS:
			equipItem(item, &stats[player]->weapon, player);
			break;
		case TOOL_PICKAXE:
			equipItem(item, &stats[player]->weapon, player);
			break;
		case TOOL_TINOPENER:
			item_ToolTinOpener(item, player);
			break;
		case TOOL_MIRROR:
			item_ToolMirror(item, player);
			break;
		case TOOL_LOCKPICK:
		case TOOL_SKELETONKEY:
			equipItem(item, &stats[player]->weapon, player);
			break;
		case TOOL_TORCH:
		case TOOL_LANTERN:
		case TOOL_CRYSTALSHARD:
			equipItem(item, &stats[player]->shield, player);
			break;
		case TOOL_BLINDFOLD:
		case TOOL_BLINDFOLD_FOCUS:
		case TOOL_BLINDFOLD_TELEPATHY:
			equipItem(item, &stats[player]->mask, player);
			break;
		case TOOL_TOWEL:
			item_ToolTowel(item, player);
			if ( multiplayer == CLIENT )
				if ( stats[player]->EFFECTS[EFF_BLEEDING] )
				{
					consumeItem(item, player);
				}
			break;
		case TOOL_GLASSES:
			equipItem(item, &stats[player]->mask, player);
			break;
		case TOOL_BEARTRAP:
			item_ToolBeartrap(item, player);
			break;
		case TOOL_ALEMBIC:
			if ( player != clientnum )
			{
				consumeItem(item, player);
			}
			else
			{
				GenericGUI.openGUI(GUI_TYPE_ALCHEMY, false, item);
			}
			break;
		case FOOD_BREAD:
		case FOOD_CREAMPIE:
		case FOOD_CHEESE:
		case FOOD_APPLE:
		case FOOD_MEAT:
		case FOOD_FISH:
		case FOOD_TOMALLEY:
		case FOOD_BLOOD:
			item_Food(item, player);
			break;
		case FOOD_TIN:
			item_FoodTin(item, player);
			break;
		case READABLE_BOOK:
			if (numbooks && player == clientnum)
			{
				if (players[player] && players[player]->entity)
				{
					if (!players[player]->entity->isBlind())
					{
						openBook(books[item->appearance % numbooks], item);
						conductIlliterate = false;
					}
					else
					{
						messagePlayer(player, language[970]);
					}
				}
			}
			break;
		case SPELL_ITEM:
		{
			;
			spell_t* spell = getSpellFromItem(item);
			if (spell)
			{
				equipSpell(spell, player);
			}
			break;
		}
		case ARTIFACT_SWORD:
			equipItem(item, &stats[player]->weapon, player);
			break;
		case ARTIFACT_MACE:
			if ( player == clientnum )
			{
				messagePlayer(clientnum, language[1096]);
			}
			equipItem(item, &stats[player]->weapon, player);
			break;
		case ARTIFACT_SPEAR:
		case ARTIFACT_AXE:
		case ARTIFACT_BOW:
			equipItem(item, &stats[player]->weapon, player);
			break;
		case ARTIFACT_ORB_BLUE:
		case ARTIFACT_ORB_RED:
		case ARTIFACT_ORB_PURPLE:
		case ARTIFACT_ORB_GREEN:
			equipItem(item, &stats[player]->weapon, player);
			break;
		default:
			printlog("error: item %d used, but it has no use case!\n", (int)item->type);
			break;
	}

	if ( player == clientnum )
	{
		if ( drankPotion && usedBy
			&& (players[clientnum] && players[clientnum]->entity)
			&& players[clientnum]->entity == usedBy )
		{
			if ( tryLearnPotionRecipe )
			{
				GenericGUI.alchemyLearnRecipe(potionType, true);
			}
			int skillLVL = stats[clientnum]->PROFICIENCIES[PRO_ALCHEMY] / 20;
			if ( tryEmptyBottle && rand() % 100 < std::min(80, (60 + skillLVL * 10)) ) // 60 - 80% chance
			{
				Item* emptyBottle = newItem(POTION_EMPTY, SERVICABLE, 0, 1, 0, true, nullptr);
				itemPickup(clientnum, emptyBottle);
				messagePlayer(clientnum, language[3351], items[POTION_EMPTY].name_identified);
				free(emptyBottle);
			}
		}
	}

	if ( !item )
	{
		return;
	}

	// on-equip messages.
	if ( multiplayer != CLIENT && itemIsEquipped(item, player) )
	{
		switch ( item->type )
		{
			case ARTIFACT_BREASTPIECE:
				messagePlayer(player, language[2972]);
				break;
			case ARTIFACT_HELM:
				messagePlayer(player, language[2973]);
				break;
			case ARTIFACT_BOOTS:
				messagePlayer(player, language[2974]);
				break;
			case ARTIFACT_CLOAK:
				messagePlayer(player, language[2975]);
				break;
			case ARTIFACT_GLOVES:
				messagePlayer(player, language[2976]);
				break;
			case AMULET_LIFESAVING:
				messagePlayer(player, language[2478]);
				break;
			case AMULET_WATERBREATHING:
				messagePlayer(player, language[2479]);
				break;
			case AMULET_MAGICREFLECTION:
				messagePlayer(player, language[2480]);
				break;
			case HAT_WIZARD:
				messagePlayer(player, language[2481]);
				break;
			case SPIKED_GAUNTLETS:
			case BRASS_KNUCKLES:
			case IRON_KNUCKLES:
				messagePlayer(player, language[2482]);
				break;
			case HAT_JESTER:
				messagePlayer(player, language[2483]);
				break;
			case IRON_BOOTS_WATERWALKING:
				messagePlayer(player, language[2484]);
				break;
			case LEATHER_BOOTS_SPEED:
				messagePlayer(player, language[2485]);
				break;
			case CLOAK_INVISIBILITY:
				messagePlayer(player, language[2486]);
				break;
			case CLOAK_PROTECTION:
				messagePlayer(player, language[2487]);
				break;
			case CLOAK_MAGICREFLECTION:
				messagePlayer(player, language[2488]);
				break;
			case GLOVES_DEXTERITY:
				messagePlayer(player, language[2489]);
				break;
			case BRACERS_CONSTITUTION:
				messagePlayer(player, language[2490]);
				break;
			case GAUNTLETS_STRENGTH:
				messagePlayer(player, language[2491]);
				break;
			case AMULET_POISONRESISTANCE:
				messagePlayer(player, language[2492]);
				break;
			case RING_ADORNMENT:
				messagePlayer(player, language[2384]);
				break;
			case RING_SLOWDIGESTION:
				messagePlayer(player, language[2385]);
				break;
			case RING_PROTECTION:
				messagePlayer(player, language[2386]);
				break;
			case RING_WARNING:
				messagePlayer(player, language[2387]);
				break;
			case RING_STRENGTH:
				messagePlayer(player, language[2388]);
				break;
			case RING_CONSTITUTION:
				messagePlayer(player, language[2389]);
				break;
			case RING_INVISIBILITY:
				messagePlayer(player, language[2412]);
				break;
			case RING_MAGICRESISTANCE:
				messagePlayer(player, language[2413]);
				break;
			case RING_CONFLICT:
				messagePlayer(player, language[2414]);
				break;
			case RING_LEVITATION:
				if ( !MFLAG_DISABLELEVITATION )
				{
					// can levitate
					messagePlayer(player, language[2415]);
				}
				else
				{
					messagePlayer(player, language[2381]);
				}
				break;
			case RING_REGENERATION:
				messagePlayer(player, language[2416]);
				break;
			case RING_TELEPORTATION:
				messagePlayer(player, language[2417]);
				break;
			case STEEL_BOOTS_FEATHER:
				messagePlayer(player, language[2418]);
				break;
			case STEEL_BOOTS_LEVITATION:
				if ( !MFLAG_DISABLELEVITATION )
				{
					// can levitate
					messagePlayer(player, language[2419]);
				}
				else
				{
					messagePlayer(player, language[2381]);
				}
				break;
			case VAMPIRE_DOUBLET:
				messagePlayer(player, language[2597]);
				break;
			case TOOL_BLINDFOLD:
				break;
			case TOOL_BLINDFOLD_FOCUS:
				messagePlayer(player, language[2907]);
				break;
			case TOOL_BLINDFOLD_TELEPATHY:
				messagePlayer(player, language[2908]);
				break;
			default:
				break;
		}
	}
}

/*-------------------------------------------------------------------------------

	itemPickup

	gives the supplied item to the specified player. Returns the item

-------------------------------------------------------------------------------*/

Item* itemPickup(int player, Item* item)
{
	if (!item)
	{
		return NULL;
	}
	Item* item2;
	node_t* node;

	if ( stats[player]->PROFICIENCIES[PRO_APPRAISAL] >= CAPSTONE_UNLOCK_LEVEL[PRO_APPRAISAL] )
	{
		item->identified = true;
		if ( item->type == GEM_GLASS )
		{
			steamStatisticUpdate(STEAM_STAT_RHINESTONE_COWBOY, STEAM_STAT_INT, 1);
		}
	}

	if ( multiplayer != CLIENT && players[player] && players[player]->entity )
	{
		item->ownerUid = players[player]->entity->getUID();
	}

	//messagePlayer(0, "id: %d", item->ownerUid);

	if ( player != 0 && multiplayer == SERVER )
	{
		// send the client info on the item it just picked up
		strcpy((char*)net_packet->data, "ITEM");
		SDLNet_Write32((Uint32)item->type, &net_packet->data[4]);
		SDLNet_Write32((Uint32)item->status, &net_packet->data[8]);
		SDLNet_Write32((Uint32)item->beatitude, &net_packet->data[12]);
		SDLNet_Write32((Uint32)item->count, &net_packet->data[16]);
		SDLNet_Write32((Uint32)item->appearance, &net_packet->data[20]);
		SDLNet_Write32((Uint32)item->ownerUid, &net_packet->data[24]);
		net_packet->data[28] = item->identified;
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 29;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
	}
	else
	{
		for ( node = stats[player]->inventory.first; node != NULL; node = node->next )
		{
			item2 = (Item*) node->element;
			if (!itemCompare(item, item2, false))
			{
				// if items are the same, check to see if they should stack
				if ( item2->shouldItemStack(player) )
				{
					item2->count += item->count;
					if ( multiplayer == CLIENT && player == clientnum && itemIsEquipped(item2, clientnum) )
					{
						// if incrementing qty and holding item, then send "equip" for server to update their count of your held item.
						strcpy((char*)net_packet->data, "EQUI"); 
						SDLNet_Write32((Uint32)item2->type, &net_packet->data[4]);
						SDLNet_Write32((Uint32)item2->status, &net_packet->data[8]);
						SDLNet_Write32((Uint32)item2->beatitude, &net_packet->data[12]);
						SDLNet_Write32((Uint32)item2->count, &net_packet->data[16]);
						SDLNet_Write32((Uint32)item2->appearance, &net_packet->data[20]);
						net_packet->data[24] = item2->identified;
						net_packet->data[25] = clientnum;
						net_packet->address.host = net_server.host;
						net_packet->address.port = net_server.port;
						net_packet->len = 26;
						sendPacketSafe(net_sock, -1, net_packet, 0);
					}
					item2->ownerUid = item->ownerUid;
					return item2;
				}
				else if ( !itemCompare(item, item2, true) )
				{
					// items are the same (incl. appearance!)
					// if they shouldn't stack, we need to change appearance of the new item.
					item->appearance = rand();
				}
			}
		}
		item2 = newItem(item->type, item->status, item->beatitude, item->count, item->appearance, item->identified, &stats[player]->inventory);
		item2->ownerUid = item->ownerUid;
		return item2;
	}

	return item;
}

/*-------------------------------------------------------------------------------

	newItemFromEntity

	returns a pointer to an item struct from the given entity if it's an
	"item" entity, and returns NULL if the entity is anything else

-------------------------------------------------------------------------------*/

Item* newItemFromEntity(Entity* entity)
{
	if ( entity == nullptr )
	{
		return nullptr;
	}
	Item* item = newItem(static_cast<ItemType>(entity->skill[10]), static_cast<Status>(entity->skill[11]), entity->skill[12], entity->skill[13], entity->skill[14], entity->skill[15], nullptr);
	item->ownerUid = static_cast<Uint32>(entity->itemOriginalOwner);
	item->interactNPCUid = static_cast<Uint32>(entity->interactedByMonster);
	return item;
}

/*-------------------------------------------------------------------------------

	itemSlot

	returns a pointer to the equipment slot in which the item is residing,
	or NULL if the item isn't stored in an equipment slot

-------------------------------------------------------------------------------*/

Item** itemSlot(Stat* myStats, Item* item)
{
	if ( !myStats || !item )
	{
		return NULL;
	}
	if (!itemCompare(item, myStats->helmet, true))
	{
		return &myStats->helmet;
	}
	if (!itemCompare(item, myStats->breastplate, true))
	{
		return &myStats->breastplate;
	}
	if (!itemCompare(item, myStats->gloves, true))
	{
		return &myStats->gloves;
	}
	if (!itemCompare(item, myStats->shoes, true))
	{
		return &myStats->shoes;
	}
	if (!itemCompare(item, myStats->shield, true))
	{
		return &myStats->shield;
	}
	if (!itemCompare(item, myStats->weapon, true))
	{
		return &myStats->weapon;
	}
	if (!itemCompare(item, myStats->cloak, true))
	{
		return &myStats->cloak;
	}
	if (!itemCompare(item, myStats->amulet, true))
	{
		return &myStats->amulet;
	}
	if (!itemCompare(item, myStats->ring, true))
	{
		return &myStats->ring;
	}
	if (!itemCompare(item, myStats->mask, true))
	{
		return &myStats->mask;
	}
	return NULL;
}

/*-------------------------------------------------------------------------------

	itemIsEquipped

	returns 1 if the passed item is equipped on the passed player number, otherwise returns 0

-------------------------------------------------------------------------------*/

bool itemIsEquipped(const Item* item, int player)
{
	if ( !itemCompare(item, stats[player]->helmet, true) )
	{
		return true;
	}
	if ( !itemCompare(item, stats[player]->breastplate, true) )
	{
		return true;
	}
	if ( !itemCompare(item, stats[player]->gloves, true) )
	{
		return true;
	}
	if ( !itemCompare(item, stats[player]->shoes, true) )
	{
		return true;
	}
	if ( !itemCompare(item, stats[player]->shield, true) )
	{
		return true;
	}
	if ( !itemCompare(item, stats[player]->weapon, true) )
	{
		return true;
	}
	if ( !itemCompare(item, stats[player]->cloak, true) )
	{
		return true;
	}
	if ( !itemCompare(item, stats[player]->amulet, true) )
	{
		return true;
	}
	if ( !itemCompare(item, stats[player]->ring, true) )
	{
		return true;
	}
	if ( !itemCompare(item, stats[player]->mask, true) )
	{
		return true;
	}

	return false;
}

/*-------------------------------------------------------------------------------

	Item::weaponGetAttack

	returns the attack power of the given item

-------------------------------------------------------------------------------*/

Sint32 Item::weaponGetAttack(Stat* wielder) const
{
	Sint32 attack = beatitude;
	if ( wielder )
	{
		if ( shouldInvertEquipmentBeatitude(wielder) )
		{
			attack = abs(beatitude);
		}
	}
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
	else if ( type == CRYSTAL_SWORD )
	{
		attack += 10;
	}
	else if ( type == CRYSTAL_SPEAR )
	{
		attack += 10;
	}
	else if ( type == CRYSTAL_BATTLEAXE )
	{
		attack += 10;
	}
	else if ( type == CRYSTAL_MACE )
	{
		attack += 10;
	}
	else if ( type == BRONZE_TOMAHAWK )
	{
		attack += 2;
	}
	else if ( type == IRON_DAGGER )
	{
		attack += 4;
	}
	else if ( type == STEEL_CHAKRAM )
	{
		attack += 6;
	}
	else if ( type == CRYSTAL_SHURIKEN )
	{
		attack += 8;
	}
	// old formula
	//attack *= (double)(status / 5.0);
	//
	if ( itemCategory(this) != TOOL && itemCategory(this) != THROWN && itemCategory(this) != GEM && itemCategory(this) != POTION )
	{
		// new formula
		attack += status - 3;
	}

	return attack;
}

/*-------------------------------------------------------------------------------

	Item::armorGetAC

	returns the armor value of the given item

-------------------------------------------------------------------------------*/

Sint32 Item::armorGetAC(Stat* wielder) const
{
	Sint32 armor = beatitude;
	if ( wielder )
	{
		if ( shouldInvertEquipmentBeatitude(wielder) )
		{
			armor = abs(beatitude);
		}
	}

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
	else if ( type == WIZARD_DOUBLET || type == HEALER_DOUBLET )
	{
		armor += 0;
	}
	else if ( type == VAMPIRE_DOUBLET )
	{
		armor += 1;
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
	else if ( type == CRYSTAL_BREASTPIECE )
	{
		armor += 5;
	}
	else if ( type == CRYSTAL_HELM )
	{
		armor += 4;
	}
	else if ( type == CRYSTAL_BOOTS )
	{
		armor += 4;
	}
	else if ( type == CRYSTAL_SHIELD )
	{
		armor += 5;
	}
	else if ( type == CRYSTAL_GLOVES )
	{
		armor += 4;
	}
	else if ( type == ARTIFACT_BREASTPIECE )
	{
		armor += 6;
	}
	else if ( type == ARTIFACT_HELM)
	{
		armor += 6;
	}
	else if ( type == ARTIFACT_BOOTS )
	{
		armor += 6;
	}
	else if ( type == ARTIFACT_GLOVES )
	{
		armor += 6;
	}
	else if ( type == ARTIFACT_CLOAK )
	{
		armor += 0;
	}
	else if ( type == MIRROR_SHIELD )
	{
		armor += 0;
	}
	else if ( type == BRASS_KNUCKLES )
	{
		armor += 1;
	}
	else if ( type == IRON_KNUCKLES )
	{
		armor += 2;
	}
	else if ( type == SPIKED_GAUNTLETS )
	{
		armor += 3;
	}
	//armor *= (double)(item->status/5.0);

	return armor;
}

/*-------------------------------------------------------------------------------

	Item::canUnequip

	returns true if the item may be unequipped (ie it isn't cursed)

-------------------------------------------------------------------------------*/

bool Item::canUnequip(Stat* wielder)
{
	/*
	//Spellbooks are no longer equipable.
	if (type >= 100 && type <= 121) { //Spellbooks always unequipable regardless of cursed.
		return true;
	}*/

	if ( wielder )
	{
		if ( shouldInvertEquipmentBeatitude(wielder) )
		{
			if ( beatitude > 0 )
			{
				identified = true;
				return false;
			}
			else
			{
				return true;
			}
		}
	}

	if (beatitude < 0)
	{
		identified = true;
		return false;
	}

	return true;
}

/*-------------------------------------------------------------------------------

	Item::buyValue

	returns value of an item to be bought by the given player

-------------------------------------------------------------------------------*/

int Item::buyValue(int player)
{
	int value = items[type].value; // base value

	// identified bonus
	if ( identified )
	{
		value *= .8;
	}
	else
	{
		if ( type == GEM_GLASS )
		{
			value = 1400;
		}
		else
		{
			value *= 1.25;
		}
	}

	// cursed and status bonuses
	if ( beatitude > 0 )
	{
		value *= 1.f * beatitude * 3; // 3x multiplier for blessed gear.
	}
	else
	{
		value *= 1.f + beatitude / 2.f;
	}
	value *= ((int)status + 5) / 10.f;

	// trading bonus
	value /= (50 + stats[player]->PROFICIENCIES[PRO_TRADING]) / 150.f;

	// charisma bonus
	value /= 1.f + stats[player]->CHR / 20.f;

	// result
	value = std::max(1, value);
	return std::max(value, items[type].value);
}

/*-------------------------------------------------------------------------------

	Item::sellValue

	returns value of an item to be sold by the given player

-------------------------------------------------------------------------------*/

int Item::sellValue(int player)
{
	int value = items[type].value; // base value

	// identified bonus
	if ( identified )
	{
		value *= 1.20;
	}
	else
	{
		if ( itemCategory(this) == GEM )
		{
			value = items[GEM_GLASS].value;
		}
		else
		{
			value *= .75;
		}
	}

	// cursed and status bonuses
	value *= 1.f + beatitude / 20.f;
	value *= ((int)status + 5) / 10.f;

	// trading bonus
	value *= (50 + stats[player]->PROFICIENCIES[PRO_TRADING]) / 150.f;

	// charisma bonus
	value *= 1.f + stats[player]->CHR / 20.f;

	// result
	value = std::max(1, value);
	return std::min(value, items[type].value);
}

/*-------------------------------------------------------------------------------

	Item::apply

	Applies the given item from the given player to the given entity
	(ie for key unlocking door)

-------------------------------------------------------------------------------*/

void Item::apply(int player, Entity* entity)
{
	if ( !entity )
	{
		return;
	}


	// for clients:
	if ( multiplayer == CLIENT )
	{
		strcpy((char*)net_packet->data, "APIT");
		SDLNet_Write32((Uint32)type, &net_packet->data[4]);
		SDLNet_Write32((Uint32)status, &net_packet->data[8]);
		SDLNet_Write32((Uint32)beatitude, &net_packet->data[12]);
		SDLNet_Write32((Uint32)count, &net_packet->data[16]);
		SDLNet_Write32((Uint32)appearance, &net_packet->data[20]);
		net_packet->data[24] = identified;
		net_packet->data[25] = player;
		SDLNet_Write32((Uint32)entity->getUID(), &net_packet->data[26]);
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 30;
		sendPacketSafe(net_sock, -1, net_packet, 0);
		if ( type >= ARTIFACT_ORB_BLUE && type <= ARTIFACT_ORB_GREEN )
		{
			applyOrb(player, type, *entity);
		}
		if ( type == POTION_EMPTY )
		{
			applyEmptyPotion(player, *entity);
		}
		return;
	}

	if ( type >= ARTIFACT_ORB_BLUE && type <= ARTIFACT_ORB_GREEN )
	{
		applyOrb(player, type, *entity);
	}
	// effects
	if ( type == TOOL_SKELETONKEY )
	{
		applySkeletonKey(player, *entity);
	}
	else if ( type == TOOL_LOCKPICK )
	{
		applyLockpick(player, *entity);
	}
	else if ( type == POTION_EMPTY )
	{
		applyEmptyPotion(player, *entity);
	}
}

SummonProperties::SummonProperties()
{
	//TODO:
}

SummonProperties::~SummonProperties()
{
	//TODO:
}

bool isPotionBad(const Item& potion)
{
	if (itemCategory(&potion) != POTION)
	{
		return false;
	}

	if (potion.type == POTION_SICKNESS 
		|| potion.type == POTION_CONFUSION 
		|| potion.type == POTION_BLINDNESS 
		|| potion.type == POTION_ACID 
		|| potion.type == POTION_PARALYSIS
		|| potion.type == POTION_FIRESTORM 
		|| potion.type == POTION_ICESTORM 
		|| potion.type == POTION_THUNDERSTORM )
	{
		return true;
	}

	return false;
}

void createCustomInventory(Stat* stats, int itemLimit)
{
	int itemSlots[6] = { ITEM_SLOT_INV_1, ITEM_SLOT_INV_2, ITEM_SLOT_INV_3, ITEM_SLOT_INV_4, ITEM_SLOT_INV_5, ITEM_SLOT_INV_6 };
	int i = 0;
	ItemType itemId;
	Status itemStatus;
	int itemBless;
	int itemAppearance = rand();
	int itemCount;
	int category = 0;
	bool itemIdentified;
	int itemsGenerated = 0;
	int chance = 1;

	if ( stats != nullptr )
	{
		for ( i = 0; i < 6 && itemsGenerated <= itemLimit; ++i )
		{
			category = stats->EDITOR_ITEMS[itemSlots[i] + ITEM_SLOT_CATEGORY];
			if ( category > 0 && stats->EDITOR_ITEMS[itemSlots[i]] == 1 )
			{
				if ( category > 0 && category <= 13 )
				{
					itemId = itemLevelCurve(static_cast<Category>(category - 1), 0, currentlevel);
				}
				else
				{
					int randType = 0;
					if ( category == 14 )
					{
						// equipment
						randType = rand() % 2;
						if ( randType == 0 )
						{
							itemId = itemLevelCurve(static_cast<Category>(WEAPON), 0, currentlevel);
						}
						else if ( randType == 1 )
						{
							itemId = itemLevelCurve(static_cast<Category>(ARMOR), 0, currentlevel);
						}
					}
					else if ( category == 15 )
					{
						// jewelry
						randType = rand() % 2;
						if ( randType == 0 )
						{
							itemId = itemLevelCurve(static_cast<Category>(AMULET), 0, currentlevel);
						}
						else
						{
							itemId = itemLevelCurve(static_cast<Category>(RING), 0, currentlevel);
						}
					}
					else if ( category == 16 )
					{
						// magical
						randType = rand() % 3;
						if ( randType == 0 )
						{
							itemId = itemLevelCurve(static_cast<Category>(SCROLL), 0, currentlevel);
						}
						else if ( randType == 1 )
						{
							itemId = itemLevelCurve(static_cast<Category>(MAGICSTAFF), 0, currentlevel);
						}
						else
						{
							itemId = itemLevelCurve(static_cast<Category>(SPELLBOOK), 0, currentlevel);
						}
					}
				}
			}
			else
			{
				itemId = static_cast<ItemType>(stats->EDITOR_ITEMS[itemSlots[i]] - 2);
			}

			if ( itemId >= 0 )
			{
				itemStatus = static_cast<Status>(stats->EDITOR_ITEMS[itemSlots[i] + 1]);
				if ( itemStatus == 0 )
				{
					itemStatus = static_cast<Status>(DECREPIT + rand() % 4);
				}
				else if ( itemStatus > BROKEN )
				{
					itemStatus = static_cast<Status>(itemStatus - 1); // reserved '0' for random, so '1' is decrepit... etc to '5' being excellent.
				}
				itemBless = stats->EDITOR_ITEMS[itemSlots[i] + 2];
				if ( itemBless == 10 )
				{
					itemBless = -1 + rand() % 3;
				}
				itemCount = stats->EDITOR_ITEMS[itemSlots[i] + 3];
				if ( stats->EDITOR_ITEMS[itemSlots[i] + 4] == 1 )
				{
					itemIdentified = false;
				}
				else if ( stats->EDITOR_ITEMS[itemSlots[i] + 4] == 2 )
				{
					itemIdentified = true;
				}
				else
				{
					itemIdentified = rand() % 2;
				}
				itemAppearance = rand();
				chance = stats->EDITOR_ITEMS[itemSlots[i] + 5];
				if ( rand() % 100 < chance )
				{
					newItem(itemId, itemStatus, itemBless, itemCount, itemAppearance, itemIdentified, &stats->inventory);
				}
				itemsGenerated++;
			}
		}
	}
}

node_t* itemNodeInInventory(Stat* myStats, ItemType itemToFind, Category cat)
{
	if ( myStats == nullptr )
	{
		return nullptr;
	}

	node_t* node = nullptr;
	node_t* nextnode = nullptr;

	for ( node = myStats->inventory.first; node != nullptr; node = nextnode )
	{
		nextnode = node->next;
		Item* item = (Item*)node->element;
		if ( item != nullptr )
		{
			if ( cat >= WEAPON && itemCategory(item) == cat )
			{
				return node;
			}
			else if ( itemToFind != -1 && item->type == itemToFind )
			{
				return node;
			}
		}
	}

	return nullptr;
}

node_t* spellbookNodeInInventory(Stat* myStats, int spellIDToFind)
{
	if ( spellIDToFind == SPELL_NONE )
	{
		return nullptr;
	}

	if ( myStats == nullptr )
	{
		return nullptr;
	}
	//messagePlayer(clientnum, "Got into spellbookNodeInInventory().");

	for ( node_t* node = myStats->inventory.first; node != nullptr; node = node->next )
	{
		Item* item = (Item*)node->element;
		if ( item != nullptr && itemCategory(item) == SPELLBOOK && getSpellIDFromSpellbook(item->type) == spellIDToFind )
		{
			return node;
		}
		else
		{
			if ( itemCategory(item) == SPELLBOOK )
			{
				//messagePlayer(clientnum, "Well...I found a spellbook? Type: %d. Looking for: %d.", getSpellIDFromSpellbook(item->type), spellIDToFind);
			}
		}
	}

	//messagePlayer(clientnum, "Spellbook %d not found.", spellIDToFind);

	return nullptr;
}

node_t* getRangedWeaponItemNodeInInventory(Stat* myStats, bool includeMagicstaff)
{
	if ( myStats == nullptr )
	{
		return nullptr;
	}

	for ( node_t* node = myStats->inventory.first; node != nullptr; node = node->next )
	{
		Item* item = (Item*)node->element;
		if ( item != nullptr )
		{
			if ( isRangedWeapon(*item) )
			{
				return node;
			}
			if ( includeMagicstaff && itemCategory(item) == MAGICSTAFF )
			{
				return node;
			}
		}
	}

	return nullptr;
}

node_t* getMeleeWeaponItemNodeInInventory(Stat* myStats)
{
	if ( myStats == nullptr )
	{
		return nullptr;
	}

	for ( node_t* node = myStats->inventory.first; node != nullptr; node = node->next )
	{
		Item* item = (Item*)node->element;
		if ( item != nullptr )
		{
			if ( isMeleeWeapon(*item) )
			{
				return node;
			}
		}
	}

	return nullptr;
}

bool isRangedWeapon(const Item& item)
{
	switch ( item.type )
	{
		case SLING:
		case SHORTBOW:
		case CROSSBOW:
		case ARTIFACT_BOW:
			return true;
		default:
			return false;
	}
}

bool isMeleeWeapon(const Item& item)
{
	if ( itemCategory(&item) != WEAPON )
	{
		return false;
	}

	return ( !isRangedWeapon(item) );
}

bool Item::isShield() const
{
	if ( itemCategory(this) != ARMOR || checkEquipType(this) != TYPE_SHIELD )
	{
		return false;
	}

	return true;
}

bool swapMonsterWeaponWithInventoryItem(Entity* my, Stat* myStats, node_t* inventoryNode, bool moveStack, bool overrideCursed)
{
	//TODO: Does this work with multiplayer?
	Item* item = nullptr;
	Item* tmpItem = nullptr;

	if ( myStats == nullptr || inventoryNode == nullptr )
	{
		return false;
	}

	if ( (myStats->weapon && myStats->weapon->beatitude < 0) && !overrideCursed )
	{
		return false; //Can't unequip cursed items!
	}

	item = (Item*)inventoryNode->element;
	
	if ( item->count == 1 || moveStack )
	{
		// TODO: handle stacks.
		tmpItem = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, false, nullptr);
		if ( !tmpItem )
		{
			return false;
		}
		copyItem(tmpItem, item);
		if ( myStats->weapon != nullptr )
		{
			copyItem(item, myStats->weapon);
			copyItem(myStats->weapon, tmpItem);
			if ( multiplayer != CLIENT && (itemCategory(myStats->weapon) == WEAPON || itemCategory(myStats->weapon) == THROWN) )
			{
				playSoundEntity(my, 40 + rand() % 4, 64);
			}
			free(tmpItem);
		}
		else
		{
			myStats->weapon = tmpItem;
			// remove the new item we created.
			list_RemoveNode(inventoryNode);
		}
		return true;
	}
	else
	{
		//Move exactly 1 item into hand.
		if ( my == nullptr )
		{
			return false;
		}

		tmpItem = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, false, nullptr);
		if ( !tmpItem )
		{
			return false;
		}

		copyItem(tmpItem, item);
		tmpItem->count = 1;
		item->count--;

		if ( myStats->weapon != nullptr )
		{
			my->addItemToMonsterInventory(myStats->weapon);
			myStats->weapon = tmpItem;
			if ( multiplayer != CLIENT && (itemCategory(myStats->weapon) == WEAPON || itemCategory(myStats->weapon) == THROWN) )
			{
				playSoundEntity(my, 40 + rand() % 4, 64);
			}
		}
		else
		{
			myStats->weapon = tmpItem;
		}
		return true;
	}

	return false;
}

bool monsterUnequipSlot(Stat* myStats, Item** slot, Item* itemToUnequip)
{
	Item* tmpItem = nullptr;

	if ( myStats == nullptr || *slot == nullptr )
	{
		return false;
	}

	if ( itemCompare(*slot, itemToUnequip, false) )
	{
		tmpItem = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, false, &myStats->inventory);
		copyItem(tmpItem, itemToUnequip);

		if ( (*slot)->node )
		{
			list_RemoveNode((*slot)->node);
		}
		else
		{
			free(*slot);
		}

		*slot = nullptr;
	}

	return true;
}

bool monsterUnequipSlotFromCategory(Stat* myStats, Item** slot, Category cat)
{
	Item* tmpItem = nullptr;

	if ( myStats == nullptr || *slot == nullptr )
	{
		return false;
	}

	if ( itemCategory(*slot) == cat)
	{
		tmpItem = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, false, &myStats->inventory);
		copyItem(tmpItem, *slot);

		if ( (*slot)->node )
		{
			list_RemoveNode((*slot)->node);
		}
		else
		{
			free(*slot);
		}

		*slot = nullptr;
		//messagePlayer(0, "un-equip!");
		return true;
	}

	return false;
}

void copyItem(Item* itemToSet, Item* itemToCopy) //This should probably use references instead...
{
	if ( !itemToSet || !itemToCopy )
	{
		return;
	}

	itemToSet->type = itemToCopy->type;
	itemToSet->status = itemToCopy->status;
	itemToSet->beatitude = itemToCopy->beatitude;
	itemToSet->count = itemToCopy->count;
	itemToSet->appearance = itemToCopy->appearance;
	itemToSet->identified = itemToCopy->identified;
	itemToSet->uid = itemToCopy->uid;
	itemToSet->ownerUid = itemToCopy->ownerUid;
	return;
}

ItemType itemTypeWithinGoldValue(Category cat, int minValue, int maxValue)
{
	int numitems = NUMITEMS;
	int numoftype = 0;
	bool chances[NUMITEMS] = { false };
	bool pickAnyCategory = false;
	int c;

	if ( cat < -1 || cat >= NUMCATEGORIES )
	{
		printlog("warning: pickItemWithinGoldValue() called with bad category value!\n");
		return GEM_ROCK;
	}

	if ( cat == -1 )
	{
		pickAnyCategory = true;
	}

	// find highest value of items in category
	for ( c = 0; c < NUMITEMS; ++c )
	{
		if ( items[c].category == cat || (pickAnyCategory && items[c].category != SPELL_CAT) )
		{
			if ( items[c].value >= minValue && items[c].value <= maxValue && items[c].level != -1 )
			{
				// chance true for an item if it's not forbidden from the global item list.
				chances[c] = true;
				numoftype++;
			}
		}
	}
	
	if ( numoftype == 0 )
	{
		printlog("warning: category passed has no items within gold values!\n");
		return GEM_ROCK;
	}

	// pick the item
	int pick = prng_get_uint() % numoftype;
	for ( c = 0; c < numitems; c++ )
	{
		if ( chances[c] == true )
		{
			if ( pick == 0 )
			{
				return static_cast<ItemType>(c);
			}
			else
			{
				pick--;
			}
		}
	}

	return GEM_ROCK;
}

bool Item::isThisABetterWeapon(const Item& newWeapon, const Item* weaponAlreadyHave)
{
	if ( !weaponAlreadyHave )
	{
		//Any thing is better than no thing!
		return true;
	}

	if ( newWeapon.weaponGetAttack() > weaponAlreadyHave->weaponGetAttack() )
	{
		return true; //If the new weapon does more damage than the current weapon, it's better. Even if it's cursed, eh?
	}

	return false;
}

bool Item::isThisABetterArmor(const Item& newArmor, const Item* armorAlreadyHave)
{
	if ( !armorAlreadyHave )
	{
		//Some thing is better than no thing!
		return true;
	}

	if ( FollowerMenu.entityToInteractWith )
	{
		if ( newArmor.interactNPCUid == FollowerMenu.entityToInteractWith->interactedByMonster )
		{
			return true;
		}
	}

	if ( armorAlreadyHave->forcedPickupByPlayer == true )
	{
		return false;
	}

	//If the new weapon defends better than the current armor, it's better. Even if it's cursed, eh?
	//TODO: Special effects/abilities, like magic resistance or reflection...
	if ( newArmor.armorGetAC() > armorAlreadyHave->armorGetAC() )
	{
		return true;
	}

	return false;
}

bool Item::shouldItemStack(int player)
{
	if ( player >= 0 )
	{
		if ( (!itemIsEquipped(this, player)
				&& itemCategory(this) != ARMOR
				&& itemCategory(this) != WEAPON
				&& itemCategory(this) != MAGICSTAFF
				&& itemCategory(this) != RING
				&& itemCategory(this) != AMULET
				&& itemCategory(this) != SPELLBOOK
				&& this->type != TOOL_PICKAXE
				&& this->type != TOOL_ALEMBIC)
			|| itemCategory(this) == THROWN
			|| itemCategory(this) == GEM
			|| itemCategory(this) == POTION
			|| (itemCategory(this) == TOOL && this->type != TOOL_PICKAXE && this->type != TOOL_ALEMBIC)
			)
		{
			// THROWN, GEM, TOOLS, POTIONS should stack when equipped.
			// otherwise most equippables should not stack.
			return true;
		}
	}
	return false;
}


bool shouldInvertEquipmentBeatitude(Stat* wielder)
{
	if ( wielder->type == SUCCUBUS || wielder->type == INCUBUS )
	{
		return true;
	}
	return false;
}