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
std::mt19937 enchantedFeatherScrollSeed(0);
std::vector<int> enchantedFeatherScrollsShuffled;
bool overrideTinkeringLimit = false;
int decoyBoxRange = 15;

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
		printlog( "failed to allocate memory for new item!\n" );
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
	item->ownerUid = 0;
	item->isDroppable = true;
	if ( inventory )
	{
		int y;
		bool notfree = false, foundaspot = false;

		bool is_spell = false;
		if (itemCategory(item) == SPELL_CAT)
		{
			is_spell = true;
		}

		int x = 0;
		int inventory_y = INVENTORY_SIZEY;
		if ( is_spell )
		{
			if ( list_Size(&spellList) >= INVENTORY_SIZEX * 3 )
			{
				inventory_y = INVENTORY_SIZEY = 4 + ((list_Size(&spellList) - (INVENTORY_SIZEX * 3)) / INVENTORY_SIZEX);
			}
			else
			{
				inventory_y = 3;
			}
		}
		else if ( multiplayer != CLIENT )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( stats[i] && &stats[i]->inventory == inventory )
				{
					if ( stats[i]->cloak && stats[i]->cloak->type == CLOAK_BACKPACK 
						&& (shouldInvertEquipmentBeatitude(stats[i]) ? stats[i]->cloak->beatitude <= 0 : stats[i]->cloak->beatitude >= 0) )
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
				if ( stats[clientnum]->cloak && stats[clientnum]->cloak->type == CLOAK_BACKPACK 
					&& (shouldInvertEquipmentBeatitude(stats[clientnum]) ? stats[clientnum]->cloak->beatitude <= 0 : stats[clientnum]->cloak->beatitude >= 0) )
				{
					inventory_y = 4;
				}
			}
		}
		const int sort_y = std::min(std::max(inventory_y, 2), 3); // only sort y values of 2-3, if extra row don't auto sort into it.

		while ( true )
		{
			for ( y = 0; y < sort_y; y++ )
			{
				for ( node_t* node = inventory->first; node != nullptr; node = node->next )
				{
					Item* tempItem = static_cast<Item*>(node->element);
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
			while ( true )
			{
				for ( y = 3; y < inventory_y; y++ )
				{
					for ( node_t* node = inventory->first; node != nullptr; node = node->next )
					{
						Item* tempItem = static_cast<Item*>(node->element);
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
				for ( int c = 0; c < NUM_HOTBAR_SLOTS; c++ )
				{
					if ( !uidToItem(hotbar[c].item) )
					{
						if ( autoAddHotbarFilter(*item) )
						{
							if ( players[clientnum] && players[clientnum]->entity && players[clientnum]->entity->effectShapeshift != NOTHING )
							{
								if ( item->usableWhileShapeshifted(stats[clientnum]) )
								{
									hotbar[c].item = item->uid;
								}
							}
							else
							{
								hotbar[c].item = item->uid;
							}
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

	bool notfree = false, foundaspot = false;

	bool is_spell = false;
	if (itemCategory(&item) == SPELL_CAT)
	{
		is_spell = true;
	}

	int x = 0;
	while ( true )
	{
		for ( int y = 0; y < INVENTORY_SIZEY; ++y )
		{
			for ( node_t* node = inventory.first; node != nullptr; node = node->next )
			{
				Item* tempItem = static_cast<Item*>(node->element);
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
			for ( int c = 0; c < NUM_HOTBAR_SLOTS; c++ )
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

Item* uidToItem(const Uint32 uid)
{
	if ( uid == 0 )
	{
		return nullptr;
	}
	for ( node_t* node = stats[clientnum]->inventory.first; node != nullptr; node = node->next )
	{
		Item* item = static_cast<Item*>(node->element);
		if ( item->uid == uid )
		{
			return item;
		}
	}
	return nullptr;
}

/*-------------------------------------------------------------------------------

	itemCurve

	Selects an item type from the given category of items by factoring in
	dungeon level, value of the item, etc.

-------------------------------------------------------------------------------*/

ItemType itemCurve(const Category cat)
{
	const int numitems = NUMITEMS - ( NUMITEMS - static_cast<int>(ARTIFACT_SWORD) );
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
				switch ( static_cast<ItemType>(c) )
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
		const int acceptablehigh = std::max<Uint32>(highestvalue * fmin(1.0, (currentlevel + 10) / 25.0), lowestvalue); //TODO: Why are double and Uint32 being compared?
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
	Uint32 pick = prng_get_uint() % numleft;
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

ItemType itemLevelCurve(const Category cat, const int minLevel, const int maxLevel)
{
	const int numitems = NUMITEMS;
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
					switch ( static_cast<ItemType>(c) )
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
					switch ( static_cast<ItemType>(c) )
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
	Uint32 pick = prng_get_uint() % numleft;
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

char* Item::description() const
{
	int c = 0;

	if ( identified == true )
	{
		if ( count < 2 )
		{
			if ( itemCategory(this) == WEAPON || itemCategory(this) == ARMOR || itemCategory(this) == MAGICSTAFF || itemCategory(this) == TOOL || itemCategory(this) == THROWN )
			{
				if ( this->type == TOOL_GYROBOT || this->type == TOOL_DUMMYBOT || this->type == TOOL_SENTRYBOT || this->type == TOOL_SPELLBOT )
				{
					snprintf(tempstr, 1024, language[3653 + status]);
				}
				else if ( itemTypeIsQuiver(this->type) )
				{
					snprintf(tempstr, 1024, language[3738], beatitude);
				}
				else
				{
					snprintf(tempstr, 1024, language[982 + status], beatitude);
				}
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
				if ( this->type == TOOL_GYROBOT || this->type == TOOL_DUMMYBOT || this->type == TOOL_SENTRYBOT || this->type == TOOL_SPELLBOT )
				{
					snprintf(tempstr, 1024, language[3658 + status], count);
				}
				else if ( itemTypeIsQuiver(this->type) )
				{
					snprintf(tempstr, 1024, language[3738], beatitude);
				}
				else
				{
					snprintf(tempstr, 1024, language[1008 + status], count, beatitude);
				}
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
				if ( this->type == TOOL_GYROBOT || this->type == TOOL_DUMMYBOT || this->type == TOOL_SENTRYBOT || this->type == TOOL_SPELLBOT )
				{
					strncpy(tempstr, language[3653 + status], 1024);
				}
				else if ( itemTypeIsQuiver(this->type) )
				{
					snprintf(tempstr, 1024, language[3763]);
				}
				else
				{
					strncpy(tempstr, language[1034 + status], 1024);
				}
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
					snprintf(&tempstr[c], 1024 - c, language[1059], items[type].name_unidentified, this->getScrollLabel());
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
				if ( this->type == TOOL_GYROBOT || this->type == TOOL_DUMMYBOT || this->type == TOOL_SENTRYBOT || this->type == TOOL_SPELLBOT )
				{
					snprintf(tempstr, 1024, language[3658 + status], count);
				}
				else if ( itemTypeIsQuiver(this->type) )
				{
					snprintf(tempstr, 1024, language[3763]);
				}
				else
				{
					snprintf(tempstr, 1024, language[1060 + status], count);
				}
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
					snprintf(&tempstr[c], 1024 - c, language[1085], count, items[type].name_unidentified, this->getScrollLabel());
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

Category itemCategory(const Item* const item)
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

char* Item::getName() const
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
				snprintf(tempstr, sizeof(tempstr), language[1059], items[type].name_unidentified, this->getScrollLabel());
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
	if (itemCategory(item) == SPELL_CAT)
	{
		spell_t* spell = getSpellFromItem(item);
		if (spell)
		{
			node_t* node = list_Node(&items[item->type].surfaces, spell->ID);
			if ( !node )
			{
				return nullptr;
			}
			SDL_Surface** surface = static_cast<SDL_Surface**>(node->element);
			return *surface;
		}
	}
	else
	{
		node_t* node = list_Node(&items[item->type].surfaces, item->appearance % items[item->type].variations);
		if ( !node )
		{
			return nullptr;
		}
		SDL_Surface** surface = static_cast<SDL_Surface**>(node->element);
		return *surface;
	}
	return nullptr;
}

/*-------------------------------------------------------------------------------

	itemCompare

	Compares two items and returns 0 if they are identical or 1 if they are
	not identical. Item count is excluded during comparison testing.

-------------------------------------------------------------------------------*/

int itemCompare(const Item* const item1, const Item* const item2, bool checkAppearance)
{
	Sint32 model1 = 0;
	Sint32 model2 = 0;

	// null cases
	if ( item1 == nullptr )
	{
		if ( item2 == nullptr )
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
		if ( item2 == nullptr )
		{
			return 1;
		}
	}

	// check attributes
	if (item1->type != item2->type)
	{
		return 1;
	}
	if ( itemCategory(item1) != THROWN && !itemTypeIsQuiver(item1->type) )
	{
		if (item1->status != item2->status)
		{
			return 1;
		}
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

	if ( !item1->identified && itemCategory(item1) == SCROLL && itemCategory(item2) == SCROLL )
	{
		if ( item1->getScrollLabel() != item2->getScrollLabel() )
		{
			return 1;
		}
	}

	if ( item1->type == TOOL_GYROBOT || item1->type == TOOL_SENTRYBOT || item1->type == TOOL_SPELLBOT || item1->type == TOOL_DUMMYBOT )
	{
		checkAppearance = true; // these items store their HP inside appearance.
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

	Handles the client impulse to drop an item, returns true on free'd item.

-------------------------------------------------------------------------------*/

bool dropItem(Item* const item, const int player, const bool notifyMessage)
{
	if (!item)
	{
		return false;
	}

	Sint16 oldcount;

	if (item == nullptr || players[player] == nullptr || players[player]->entity == nullptr || itemCategory(item) == SPELL_CAT)
	{
		return false;
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
			return false;
		}
	}

	if ( multiplayer == CLIENT )
	{
		strcpy((char*)net_packet->data, "DROP");
		SDLNet_Write32(static_cast<Uint32>(item->type), &net_packet->data[4]);
		SDLNet_Write32(static_cast<Uint32>(item->status), &net_packet->data[8]);
		SDLNet_Write32(static_cast<Uint32>(item->beatitude), &net_packet->data[12]);
		SDLNet_Write32(static_cast<Uint32>(item->count), &net_packet->data[16]);
		SDLNet_Write32(static_cast<Uint32>(item->appearance), &net_packet->data[20]);
		net_packet->data[24] = item->identified;
		net_packet->data[25] = clientnum;
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 26;
		sendPacketSafe(net_sock, -1, net_packet, 0);
		if ( item == open_book_item )
		{
			closeBookGUI();
		}

		oldcount = item->count;
		if ( item->count >= 10 && (item->type == TOOL_METAL_SCRAP || item->type == TOOL_MAGIC_SCRAP) )
		{
			item->count = 10;
			messagePlayer(player, language[1088], item->description());
			item->count = oldcount - 10;
		}
		else if ( itemTypeIsQuiver(item->type) )
		{
			item->count = 1;
			if ( notifyMessage )
			{
				messagePlayer(player, language[1088], item->description());
			}
			item->count = 0;
			/*if ( oldcount >= 10 )
			{
				item->count = oldcount - 10;
			}
			else
			{
				item->count = 0;
			}*/
		}
		else
		{
			item->count = 1;
			if ( notifyMessage )
			{
				messagePlayer(player, language[1088], item->description());
			}
			item->count = oldcount - 1;
		}

		// unequip the item
		/*if ( item->count <= 1 )
		{
		}*/
		Item** slot = itemSlot(stats[player], item);
		if ( slot != nullptr )
		{
			*slot = nullptr;
		}

		if ( item->count <= 0 )
		{
			list_RemoveNode(item->node);
			return true;
		}
		return false;
	}
	else
	{
		if (item == open_book_item)
		{
			closeBookGUI();
		}
		int qtyToDrop = 1;
		if ( item->count >= 10 && (item->type == TOOL_METAL_SCRAP || item->type == TOOL_MAGIC_SCRAP) )
		{
			qtyToDrop = 10;
		}
		else if ( itemTypeIsQuiver(item->type) )
		{
			qtyToDrop = item->count;
			/*if ( item->count >= 10 )
				{
					qtyToDrop = 10;
				}
				else
				{
					qtyToDrop = item->count;
				}*/
		}

		Entity* entity = newEntity(-1, 1, map.entities, nullptr); //Item entity.
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
		entity->skill[13] = qtyToDrop;
		entity->skill[14] = item->appearance;
		entity->skill[15] = item->identified;
		entity->parent = players[player]->entity->getUID();
		entity->itemOriginalOwner = entity->parent;

		// play sound
		playSoundEntity( players[player]->entity, 47 + rand() % 3, 64 );

		// unequip the item
		Item** slot = itemSlot(stats[player], item);
		if ( slot != nullptr )
		{
			*slot = nullptr;
		}

		if ( item->node != nullptr )
		{
			if ( item->node->list == &stats[0]->inventory )
			{
				oldcount = item->count;
				item->count = qtyToDrop;
				if ( notifyMessage )
				{
					messagePlayer(player, language[1088], item->description());
				}
				item->count = oldcount - qtyToDrop;
				if ( item->count <= 0 )
				{
					list_RemoveNode(item->node);
					return true;
				}
			}
		}
		else
		{
			item->count = item->count - qtyToDrop;
			if ( item->count <= 0 )
			{
				free(item);
				return true;
			}
		}

		return false;
	}
}

Entity* dropItemMonster(Item* const item, Entity* const monster, Stat* const monsterStats, Sint16 count)
{
	// WARNING - dropItemMonster is used on playerDeaths, modifying this here neet to edit in actPlayer.cpp and net.cpp
	Entity* entity = nullptr;
	bool itemDroppable = true;

	if ( !item || !monster )
	{
		return nullptr;
	}

	if ( monster->behavior == &actPlayer && monster->skill[2] == clientnum )
	{
		if ( item == selectedItem )
		{
			selectedItem = nullptr;
		}
	}

	/*if ( monsterStats->type == SHADOW && itemCategory(item) == SPELLBOOK )
	{
		//Shadows don't drop spellbooks.
		itemDroppable = false;
	}*/
	if ( monsterStats )
	{
		if ( monsterStats->monsterNoDropItems == 1 )
		{
			itemDroppable = false;
		}
		if ( monsterStats->type == SKELETON && monster->behavior == &actMonster && monster->monsterAllySummonRank != 0 )
		{
			itemDroppable = false;
		}
		if ( monsterStats->type == INCUBUS )
		{
			if ( !strncmp(monsterStats->name, "inner demon", strlen("inner demon")) )
			{
				itemDroppable = false;
			}
		}
		if ( !item->isDroppable )
		{
			itemDroppable = false;
		}

		if ( item->appearance == MONSTER_ITEM_UNDROPPABLE_APPEARANCE )
		{
			if ( monster->behavior == &actMonster
				&& (item->type < ARTIFACT_ORB_BLUE || item->type > ARTIFACT_ORB_GREEN) )
			{
				// default no monster drops these if appearance is set
				itemDroppable = false;
			}
			else
			{
				if ( monsterStats->type == SHADOW || monsterStats->type == AUTOMATON )
				{
					itemDroppable = false;
				}
				if ( monster->monsterIsTinkeringCreation() )
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
	if ( itemTypeIsQuiver(item->type) )
	{
		count = item->count;
	}

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

		if ( monsterStats )
		{
			if (monsterStats->type == INCUBUS || monsterStats->type == SUCCUBUS )
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
			else if ( monsterStats->type == DUMMYBOT )
			{
				entity->z = 4;
			}
			else if ( monsterStats->type == SENTRYBOT || monsterStats->type == SPELLBOT )
			{
				entity->vel_x *= 0.1;
				entity->vel_y *= 0.1;
				entity->vel_z = -.5;
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

void consumeItem(Item*& item, const int player)
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
			for ( int i = 0; i < MAXPLAYERS; i++ )
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

EquipItemResult equipItem(Item* const item, Item** const slot, const int player)
{
	int oldcount;

	if ( player == clientnum && pickaxeGimpTimer > 0 && !intro )
	{
		return EQUIP_ITEM_FAIL_CANT_UNEQUIP;
	}

	if (!item)   // needs "|| !slot " ?
	{
		return EQUIP_ITEM_FAIL_CANT_UNEQUIP;
	}

	if ( player == clientnum && multiplayer != SINGLE 
		&& item->unableToEquipDueToSwapWeaponTimer() )
	{
		return EQUIP_ITEM_FAIL_CANT_UNEQUIP;
	}

	if ( itemCompare(*slot, item, true) )
	{
		// if items are different... (excluding the quantity of both item nodes)
		if ( *slot != nullptr )
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
						if ( shouldInvertEquipmentBeatitude(stats[player]) && (*slot)->beatitude > 0 )
						{
							messagePlayer(player, language[3217], (*slot)->getName());
						}
						else
						{
							messagePlayer(player, language[1089], (*slot)->getName());
						}
					}
					(*slot)->identified = true;
					return EQUIP_ITEM_FAIL_CANT_UNEQUIP;
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
					else if ( item->type == BOOMERANG )
					{
					}
					else if ( itemCategory(item) == WEAPON || itemCategory(item) == THROWN )
					{
						playSoundEntity(players[player]->entity, 40 + rand() % 4, 64);
					}
					else if ( itemCategory(item) == ARMOR 
						|| item->type == TOOL_TINKERING_KIT 
						|| itemTypeIsQuiver(item->type) )
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
			if ( *slot != nullptr )
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
		return EQUIP_ITEM_SUCCESS_NEWITEM;
	}
	else
	{
		// if items are the same... (excluding the quantity of both item nodes)
		if ( *slot != nullptr )
		{
			if ( (*slot)->count == item->count ) // if quantity is the same then it's the same item, can unequip
			{
				if (!(*slot)->canUnequip(stats[player]))
				{
					if ( player == clientnum )
					{
						if ( shouldInvertEquipmentBeatitude(stats[player]) && (*slot)->beatitude > 0 )
						{
							messagePlayer(player, language[3217], (*slot)->getName());
						}
						else
						{
							messagePlayer(player, language[1089], (*slot)->getName());
						}
					}
					(*slot)->identified = true;
					return EQUIP_ITEM_FAIL_CANT_UNEQUIP;
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
				return EQUIP_ITEM_SUCCESS_UPDATE_QTY;
			}
		}
		if (multiplayer != CLIENT && !intro && !fadeout)
		{
			if (players[player] != nullptr && players[player]->entity != nullptr)
			{
				if (players[player]->entity->ticks > 60)
				{
					if (itemCategory(item) == ARMOR )
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
			if ( *slot != nullptr )
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
		*slot = nullptr;
		return EQUIP_ITEM_SUCCESS_UNEQUIP;
	}
}
/*-------------------------------------------------------------------------------

	useItem

	Handles the client impulse to use an item

-------------------------------------------------------------------------------*/

void useItem(Item* item, const int player, Entity* usedBy)
{
	if ( item == nullptr )
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
	if ( item->type == FOOD_CREAMPIE && player == clientnum && itemIsEquipped(item, player) )
	{
		messagePlayer(player, language[3874]); // can't eat while equipped.
		return;
	}

	// tins need a tin opener to open...
	if ( player == clientnum && !(stats[player]->type == GOATMAN || stats[player]->type == AUTOMATON) )
	{
		if ( item->type == FOOD_TIN )
		{
			bool havetinopener = false;
			for ( node_t* node = stats[clientnum]->inventory.first; node != nullptr; node = node->next )
			{
				Item* tempitem = static_cast<Item*>(node->element);
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
		if ( item->unableToEquipDueToSwapWeaponTimer() && itemCategory(item) != POTION )
		{
			// don't send to host as we're not allowed to "use" or equip these items. 
			// will return false in equipItem.
			// potions allowed here because we're drinking em.
		}
		else
		{
			strcpy((char*)net_packet->data, "USEI");
			SDLNet_Write32(static_cast<Uint32>(item->type), &net_packet->data[4]);
			SDLNet_Write32(static_cast<Uint32>(item->status), &net_packet->data[8]);
			SDLNet_Write32(static_cast<Uint32>(item->beatitude), &net_packet->data[12]);
			SDLNet_Write32(static_cast<Uint32>(item->count), &net_packet->data[16]);
			SDLNet_Write32(static_cast<Uint32>(item->appearance), &net_packet->data[20]);
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
	const bool tryEmptyBottle = (item->status >= SERVICABLE);
	const ItemType potionType = item->type;
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
		case BOOMERANG:
			equipItem(item, &stats[player]->weapon, player);
			break;
		case STEEL_SHIELD:
		case STEEL_SHIELD_RESISTANCE:
		case MIRROR_SHIELD:
		case CRYSTAL_SHIELD:
			equipItem(item, &stats[player]->shield, player);
			break;
		case CROSSBOW:
		case LONGBOW:
		case COMPOUND_BOW:
		case HEAVY_CROSSBOW:
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
		case MACHINIST_APRON:
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
		case PUNISHER_HOOD:
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
			const int oldcount = item->count;
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
		{
			const bool exploded = item_ScrollFire(item, player);
			if ( exploded && stats[player] && stats[player]->type == AUTOMATON )
			{
				if ( multiplayer != CLIENT )
				{
					stats[player]->HUNGER = std::min(stats[player]->HUNGER + 1500, 1500);
					players[player]->entity->modMP(stats[player]->MAXMP);
					// results of eating
					const Uint32 color = SDL_MapRGB(mainsurface->format, 255, 128, 0);
					messagePlayerColor(player, color, language[3699]); // superheats
					serverUpdateHunger(player);
					if ( stats[player]->playerRace == RACE_AUTOMATON && stats[player]->appearance == 0 )
					{
						steamStatisticUpdateClient(player, STEAM_STAT_SPICY, STEAM_STAT_INT, 1);
						steamStatisticUpdateClient(player, STEAM_STAT_FASCIST, STEAM_STAT_INT, 1);
					}
				}
			}
			if ( !players[player]->entity->isBlind() )
			{
				consumeItem(item, player);
			}
			break;
		}
		case SCROLL_FOOD:
			item_ScrollFood(item, player);
			if ( !players[player]->entity->isBlind() )
			{
				consumeItem(item, player);
			}
			break;
		case SCROLL_CONJUREARROW:
			item_ScrollConjureArrow(item, player);
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
		case SCROLL_CHARGING:
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
		case MAGICSTAFF_POISON:
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
		case SPELLBOOK_REVERT_FORM:
		case SPELLBOOK_RAT_FORM:
		case SPELLBOOK_SPIDER_FORM:
		case SPELLBOOK_TROLL_FORM:
		case SPELLBOOK_IMP_FORM:
		case SPELLBOOK_SPRAY_WEB:
		case SPELLBOOK_POISON:
		case SPELLBOOK_SPEED:
		case SPELLBOOK_FEAR:
		case SPELLBOOK_STRIKE:
		case SPELLBOOK_DETECT_FOOD:
		case SPELLBOOK_WEAKNESS:
		case SPELLBOOK_AMPLIFY_MAGIC:
		case SPELLBOOK_SHADOW_TAG:
		case SPELLBOOK_TELEPULL:
		case SPELLBOOK_DEMON_ILLU:
		case SPELLBOOK_TROLLS_BLOOD:
		case SPELLBOOK_SALVAGE:
		case SPELLBOOK_FLUTTER:
		case SPELLBOOK_DASH:
		case SPELLBOOK_SELF_POLYMORPH:
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
		case TOOL_WHIP:
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
		case TOOL_BOMB:
		case TOOL_SLEEP_BOMB:
		case TOOL_FREEZE_BOMB:
		case TOOL_TELEPORT_BOMB:
		case TOOL_DECOY:
		case TOOL_DUMMYBOT:
		case TOOL_GYROBOT:
		case TOOL_SENTRYBOT:
		case TOOL_SPELLBOT:
			equipItem(item, &stats[player]->weapon, player);
			break;
		case TOOL_TORCH:
		case TOOL_LANTERN:
		case TOOL_CRYSTALSHARD:
		case TOOL_TINKERING_KIT:
		case QUIVER_SILVER:
		case QUIVER_PIERCE:
		case QUIVER_LIGHTWEIGHT:
		case QUIVER_FIRE:
		case QUIVER_KNOCKBACK:
		case QUIVER_CRYSTAL:
		case QUIVER_HUNTING:
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
		case MASK_SHAMAN:
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
		case ENCHANTED_FEATHER:
			if ( player != clientnum )
			{
				consumeItem(item, player);
			}
			else
			{
				GenericGUI.openGUI(GUI_TYPE_SCRIBING, item);
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
		case TOOL_MAGIC_SCRAP:
		case TOOL_METAL_SCRAP:
			if ( player == clientnum )
			{
				if ( item->type == TOOL_METAL_SCRAP )
				{
					messagePlayer(player, language[3705]);
				}
				else
				{
					messagePlayer(player, language[3706]);
				}
			}
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
			spell_t* spell = getSpellFromItem(item);
			if (spell)
			{
				equipSpell(spell, player, item);
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
			printlog("error: item %d used, but it has no use case!\n", static_cast<int>(item->type));
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
			const int skillLVL = stats[clientnum]->PROFICIENCIES[PRO_ALCHEMY] / 20;
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

Item* itemPickup(const int player, Item* const item)
{
	if (!item)
	{
		return nullptr;
	}
	Item* item2;

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
		bool assignNewOwner = true;
		if ( item->ownerUid != 0 && !achievementObserver.playerAchievements[player].ironicPunishmentTargets.empty() )
		{
			const auto it = achievementObserver.playerAchievements[player].ironicPunishmentTargets.find(item->ownerUid);
			if ( it != achievementObserver.playerAchievements[player].ironicPunishmentTargets.end() )
			{
				assignNewOwner = false;
			}
		}

		if ( assignNewOwner )
		{
			item->ownerUid = players[player]->entity->getUID();
		}
	}

	//messagePlayer(0, "id: %d", item->ownerUid);

	if ( player != 0 && multiplayer == SERVER )
	{
		// send the client info on the item it just picked up
		strcpy((char*)net_packet->data, "ITEM");
		SDLNet_Write32(static_cast<Uint32>(item->type), &net_packet->data[4]);
		SDLNet_Write32(static_cast<Uint32>(item->status), &net_packet->data[8]);
		SDLNet_Write32(static_cast<Uint32>(item->beatitude), &net_packet->data[12]);
		SDLNet_Write32(static_cast<Uint32>(item->count), &net_packet->data[16]);
		SDLNet_Write32(static_cast<Uint32>(item->appearance), &net_packet->data[20]);
		SDLNet_Write32(static_cast<Uint32>(item->ownerUid), &net_packet->data[24]);
		net_packet->data[28] = item->identified;
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 29;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
	}
	else
	{
		std::unordered_set<Uint32> appearancesOfSimilarItems;
		for ( node_t* node = stats[player]->inventory.first; node != nullptr; node = node->next )
		{
			item2 = static_cast<Item*>(node->element);
			if (!itemCompare(item, item2, false))
			{
				if ( (itemTypeIsQuiver(item2->type) && (item->count + item2->count) >= QUIVER_MAX_AMMO_QTY)
					|| ((item2->type == TOOL_MAGIC_SCRAP || item2->type == TOOL_METAL_SCRAP)
						&& (item->count + item2->count) >= SCRAP_MAX_STACK_QTY) )
				{
					int maxStack = QUIVER_MAX_AMMO_QTY;
					if ( item2->type == TOOL_MAGIC_SCRAP || item2->type == TOOL_METAL_SCRAP )
					{
						maxStack = SCRAP_MAX_STACK_QTY;
					}

					if ( item2->count == maxStack - 1 )
					{
						// can't add anymore to this stack, let's skip over this.

						if ( item->appearance == item2->appearance )
						{
							// items are the same (incl. appearance!)
							// if they shouldn't stack, we need to change appearance of the new item.
							appearancesOfSimilarItems.insert(item2->appearance);
						}
						continue;
					}

					// too many arrows, split off into a new stack with reduced qty.
					const int total = item->count + item2->count;
					item2->count = maxStack - 1;
					item->count = total - item2->count;

					if ( multiplayer == CLIENT && player == clientnum && itemIsEquipped(item2, clientnum) )
					{
						// if incrementing qty and holding item, then send "equip" for server to update their count of your held item.
						strcpy((char*)net_packet->data, "EQUS");
						SDLNet_Write32(static_cast<Uint32>(item2->type), &net_packet->data[4]);
						SDLNet_Write32(static_cast<Uint32>(item2->status), &net_packet->data[8]);
						SDLNet_Write32(static_cast<Uint32>(item2->beatitude), &net_packet->data[12]);
						SDLNet_Write32(static_cast<Uint32>(item2->count), &net_packet->data[16]);
						SDLNet_Write32(static_cast<Uint32>(item2->appearance), &net_packet->data[20]);
						net_packet->data[24] = item2->identified;
						net_packet->data[25] = clientnum;
						net_packet->address.host = net_server.host;
						net_packet->address.port = net_server.port;
						net_packet->len = 27;
						sendPacketSafe(net_sock, -1, net_packet, 0);
					}
					item2->ownerUid = item->ownerUid;
					if ( item->count <= 0 )
					{
						return item2;
					}
					else
					{
						// we have to search other items to stack with, otherwise this search ends after 1 full stack.
						if ( item->appearance == item2->appearance )
						{
							// items are the same (incl. appearance!)
							// if they shouldn't stack, we need to change appearance of the new item.
							appearancesOfSimilarItems.insert(item2->appearance);
						}
						continue;
					}
				}
				// if items are the same, check to see if they should stack
				else if ( item2->shouldItemStack(player) )
				{
					item2->count += item->count;
					if ( multiplayer == CLIENT && player == clientnum && itemIsEquipped(item2, clientnum) )
					{
						// if incrementing qty and holding item, then send "equip" for server to update their count of your held item.
						Item** slot = itemSlot(stats[clientnum], item2);
						if ( slot )
						{
							if ( slot == &stats[clientnum]->weapon )
							{
								clientSendEquipUpdateToServer(EQUIP_ITEM_SLOT_WEAPON, EQUIP_ITEM_SUCCESS_UPDATE_QTY, clientnum,
									item2->type, item2->status, item2->beatitude, item2->count, item2->appearance, item2->identified);
							}
							else if ( slot == &stats[clientnum]->shield )
							{
								clientSendEquipUpdateToServer(EQUIP_ITEM_SLOT_SHIELD, EQUIP_ITEM_SUCCESS_UPDATE_QTY, clientnum,
									item2->type, item2->status, item2->beatitude, item2->count, item2->appearance, item2->identified);
							}
						}
					}
					item2->ownerUid = item->ownerUid;
					return item2;
				}
				else if ( !itemCompare(item, item2, true) )
				{
					// items are the same (incl. appearance!)
					// if they shouldn't stack, we need to change appearance of the new item.
					appearancesOfSimilarItems.insert(item2->appearance);
				}
			}
		}
		if ( !appearancesOfSimilarItems.empty() )
		{
			int tries = 100;
			bool robot = false;
			// we need to find a unique appearance within the list.
			if ( item->type == TOOL_SENTRYBOT || item->type == TOOL_SPELLBOT || item->type == TOOL_GYROBOT
				|| item->type == TOOL_DUMMYBOT )
			{
				robot = true;
				item->appearance += (rand() % 100000) * 10;
			}
			else
			{
				item->appearance = rand();
			}
			auto it = appearancesOfSimilarItems.find(item->appearance);
			while ( it != appearancesOfSimilarItems.end() && tries > 0 )
			{
				if ( robot )
				{
					item->appearance += (rand() % 100000) * 10;
				}
				else
				{
					item->appearance = rand();
				}
				it = appearancesOfSimilarItems.find(item->appearance);
				--tries;
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

Item* newItemFromEntity(const Entity* const entity)
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

Item** itemSlot(Stat* const myStats, Item* const item)
{
	if ( !myStats || !item )
	{
		return nullptr;
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
	return nullptr;
}

/*-------------------------------------------------------------------------------

	itemIsEquipped

	returns 1 if the passed item is equipped on the passed player number, otherwise returns 0

-------------------------------------------------------------------------------*/

bool itemIsEquipped(const Item* const item, const int player)
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

Sint32 Item::weaponGetAttack(const Stat* const wielder) const
{
	Sint32 attack = beatitude;
	if ( wielder )
	{
		if ( wielder->type == TROLL || wielder->type == RAT || wielder->type == SPIDER || wielder->type == CREATURE_IMP )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( wielder == stats[i] ) // is a player stat pointer.
				{
					return 0; // players that are these monsters do not benefit from weapons
				}
			}
		}
		if ( wielder->type == INCUBUS && wielder->playerRace == 0 && !strncmp(wielder->name, "inner demon", strlen("inner demon")) )
		{
			return -9999;
		}
		if ( shouldInvertEquipmentBeatitude(wielder) )
		{
			attack = abs(beatitude);
		}
	}
	if ( itemCategory(this) == MAGICSTAFF )
	{
		attack += 6;
	}
	else if ( itemCategory(this) == GEM )
	{
		attack += 4;
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
	else if ( type == ARTIFACT_SWORD )
	{
		return (attack + 2 + status * 2);
	}
	else if ( type == ARTIFACT_MACE )
	{
		return (attack + 2 + status * 2);
	}
	else if ( type == ARTIFACT_SPEAR )
	{
		return (attack + 2 + status * 2);
	}
	else if ( type == ARTIFACT_AXE )
	{
		return (attack + 2 + status * 2);
	}
	else if ( type == ARTIFACT_BOW )
	{
		return (attack + 2 + status * 2);
	}
	else if ( type == SHORTBOW )
	{
		attack += 6;
	}
	else if ( type == CROSSBOW )
	{
		attack += 7;
	}
	else if ( type == LONGBOW )
	{
		attack += 10;
	}
	else if ( type == HEAVY_CROSSBOW )
	{
		attack += 16;
	}
	else if ( type == COMPOUND_BOW )
	{
		attack += 9;
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
		attack += 6;
	}
	else if ( type == IRON_DAGGER )
	{
		attack += 8;
	}
	else if ( type == BOOMERANG )
	{
		return (attack + std::max(0, (status - 1) * 2));
	}
	else if ( type == STEEL_CHAKRAM )
	{
		attack += 10;
	}
	else if ( type == CRYSTAL_SHURIKEN )
	{
		attack += 12;
	}
	else if ( type == TOOL_WHIP )
	{
		attack += 2;
	}
	else if ( type == QUIVER_SILVER )
	{
		return attack + 2;
	}
	else if ( type == QUIVER_PIERCE )
	{
		return attack + 4;
	}
	else if ( type == QUIVER_LIGHTWEIGHT )
	{
		return attack - 2;
	}
	else if ( type == QUIVER_FIRE )
	{
		return attack + 2;
	}
	else if ( type == QUIVER_KNOCKBACK )
	{
		return attack + 4;
	}
	else if ( type == QUIVER_CRYSTAL )
	{
		return attack + 6;
	}
	else if ( type == QUIVER_HUNTING )
	{
		return attack + 4;
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

Sint32 Item::armorGetAC(const Stat* const wielder) const
{
	Sint32 armor = beatitude;
	if ( wielder )
	{
		if ( shouldInvertEquipmentBeatitude(wielder) )
		{
			armor = abs(beatitude);
		}
	}

	if ( itemTypeIsQuiver(type) )
	{
		armor = 0;
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
	else if ( type == MACHINIST_APRON )
	{
		armor += 1;
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
		armor += std::max(3, 3 + (status - 1)); // 2-6
	}
	else if ( type == ARTIFACT_HELM)
	{
		armor += std::max(1, 1 + (status - 1)); // 1-4
	}
	else if ( type == ARTIFACT_BOOTS )
	{
		armor += std::max(1, 1 + (status - 1)); // 1-4
	}
	else if ( type == ARTIFACT_GLOVES )
	{
		armor += std::max(1, 1 + (status - 1)); // 1-4
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

	if ( wielder )
	{
		if ( wielder->type == TROLL || wielder->type == RAT || wielder->type == SPIDER || wielder->type == CREATURE_IMP )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( wielder == stats[i] ) // is a player stat pointer.
				{
					if ( itemCategory(this) == RING || itemCategory(this) == AMULET )
					{
						return armor;
					}
					return 0; // players that are these monsters do not benefit from non rings/amulets
				}
			}
		}
	}

	return armor;
}

/*-------------------------------------------------------------------------------

	Item::canUnequip

	returns true if the item may be unequipped (ie it isn't cursed)

-------------------------------------------------------------------------------*/

bool Item::canUnequip(const Stat* const wielder)
{
	/*
	//Spellbooks are no longer equipable.
	if (type >= 100 && type <= 121) { //Spellbooks always unequipable regardless of cursed.
		return true;
	}*/

	if ( wielder )
	{
		if ( wielder->type == AUTOMATON )
		{
			return true;
		}
		else if ( shouldInvertEquipmentBeatitude(wielder) )
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

int Item::buyValue(const int player) const
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
	value *= (static_cast<int>(status) + 5) / 10.f;

	// trading bonus
	value /= (50 + stats[player]->PROFICIENCIES[PRO_TRADING]) / 150.f;

	// charisma bonus
	value /= 1.f + stats[player]->CHR / 20.f;

	// result
	value = std::max(1, value);

	if ( shopIsMysteriousShopkeeper(uidToEntity(shopkeeper)) )
	{
		value *= 2;
	}

	if ( itemTypeIsQuiver(type) )
	{
		return std::max(value, items[type].value) * count;
	}

	return std::max(value, items[type].value);
}

/*-------------------------------------------------------------------------------

	Item::sellValue

	returns value of an item to be sold by the given player

-------------------------------------------------------------------------------*/

int Item::sellValue(const int player) const
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
	value *= (static_cast<int>(status) + 5) / 10.f;

	// trading bonus
	value *= (50 + stats[player]->PROFICIENCIES[PRO_TRADING]) / 150.f;

	// charisma bonus
	value *= 1.f + stats[player]->CHR / 20.f;

	// result
	value = std::max(1, value);

	if ( itemTypeIsQuiver(type) )
	{
		return std::min(value, items[type].value) * count;
	}

	return std::min(value, items[type].value);
}

/*-------------------------------------------------------------------------------

	Item::apply

	Applies the given item from the given player to the given entity
	(ie for key unlocking door)

-------------------------------------------------------------------------------*/

void Item::apply(const int player, Entity* const entity)
{
	if ( !entity )
	{
		return;
	}


	// for clients:
	if ( multiplayer == CLIENT )
	{
		strcpy((char*)net_packet->data, "APIT");
		SDLNet_Write32(static_cast<Uint32>(type), &net_packet->data[4]);
		SDLNet_Write32(static_cast<Uint32>(status), &net_packet->data[8]);
		SDLNet_Write32(static_cast<Uint32>(beatitude), &net_packet->data[12]);
		SDLNet_Write32(static_cast<Uint32>(count), &net_packet->data[16]);
		SDLNet_Write32(static_cast<Uint32>(appearance), &net_packet->data[20]);
		net_packet->data[24] = identified;
		net_packet->data[25] = player;
		SDLNet_Write32(static_cast<Uint32>(entity->getUID()), &net_packet->data[26]);
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 30;
		sendPacketSafe(net_sock, -1, net_packet, 0);
		if ( type >= ARTIFACT_ORB_BLUE && type <= ARTIFACT_ORB_GREEN )
		{
			applyOrb(player, type, *entity);
		}
		else if ( type == POTION_EMPTY )
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

void Item::applyLockpickToWall(const int player, const int x, const int y) const
{
	// for clients:
	if ( multiplayer == CLIENT )
	{
		strcpy((char*)net_packet->data, "APIW");
		SDLNet_Write32(static_cast<Uint32>(type), &net_packet->data[4]);
		SDLNet_Write32(static_cast<Uint32>(status), &net_packet->data[8]);
		SDLNet_Write32(static_cast<Uint32>(beatitude), &net_packet->data[12]);
		SDLNet_Write32(static_cast<Uint32>(count), &net_packet->data[16]);
		SDLNet_Write32(static_cast<Uint32>(appearance), &net_packet->data[20]);
		net_packet->data[24] = identified;
		net_packet->data[25] = player;
		SDLNet_Write16(x, &net_packet->data[26]);
		SDLNet_Write16(y, &net_packet->data[28]);
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 30;
		sendPacketSafe(net_sock, -1, net_packet, 0);
		return;
	}

	for ( node_t* node = map.entities->first; node != nullptr; node = node->next )
	{
		Entity* entity = static_cast<Entity*>(node->element);
		if ( entity && entity->behavior == &actArrowTrap
			&& static_cast<int>(entity->x / 16) == x
			&& static_cast<int>(entity->y / 16) == y )
		{
			// found a trap.
			if ( entity->skill[4] == 0 )
			{
				if ( players[player] && players[player]->entity
					&& stats[player] && stats[player]->weapon
					&& (stats[player]->weapon->type == TOOL_LOCKPICK || stats[player]->weapon->type == TOOL_SKELETONKEY) )
				{
					const int skill = std::max(1, stats[player]->PROFICIENCIES[PRO_LOCKPICKING] / 10);
					bool failed = false;
					if ( skill < 2 || rand() % skill == 0 ) // 20 skill requirement.
					{
						// failed.
						const Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
						messagePlayerColor(player, color, language[3871]); // trap fires.
						if ( skill < 2 )
						{
							messagePlayer(player, language[3887]); // not skilled enough.
						}
						failed = true;
					}

					if ( failed )
					{
						entity->skill[4] = -1; // make the trap shoot.
						playSoundEntity(entity, 92, 64);
					}
					else
					{
						const Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
						messagePlayerColor(player, color, language[3872]);
						playSoundEntity(entity, 176, 128);
						entity->skill[4] = player + 1; // disabled flag and spit out items.
						serverUpdateEntitySkill(entity, 4); // update clients.
					}

					// degrade lockpick.
					if ( !(stats[player]->weapon->type == TOOL_SKELETONKEY) && (rand() % 10 == 0 || (failed && rand() % 4 == 0)) )
					{
						if ( player == clientnum )
						{
							if ( count > 1 )
							{
								newItem(type, status, beatitude, count - 1, appearance, identified, &stats[player]->inventory);
							}
						}
						stats[player]->weapon->count = 1;
						stats[player]->weapon->status = static_cast<Status>(stats[player]->weapon->status - 1);
						if ( stats[player]->weapon->status == BROKEN )
						{
							messagePlayer(player, language[1104]);
							playSoundEntity(players[player]->entity, 76, 64);
						}
						else
						{
							messagePlayer(player, language[1103]);
						}
						if ( player > 0 && multiplayer == SERVER )
						{
							strcpy((char*)net_packet->data, "ARMR");
							net_packet->data[4] = 5;
							net_packet->data[5] = stats[player]->weapon->status;
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}
					}
					if ( !failed && rand() % 5 == 0 )
					{
						players[player]->entity->increaseSkill(PRO_LOCKPICKING);
					}
					return;
				}
				else if ( entity->skill[4] != 0 )
				{
					messagePlayer(player, language[3870]);
					return;
				}
			}
			break;
		}
	}

	if ( map.tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map.height] == 53 )
	{
		messagePlayer(player, language[3873]);
	}
}

SummonProperties::SummonProperties() = default;

SummonProperties::~SummonProperties() noexcept = default;

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

void createCustomInventory(Stat* const stats, const int itemLimit)
{
	int itemSlots[6] = { ITEM_SLOT_INV_1, ITEM_SLOT_INV_2, ITEM_SLOT_INV_3, ITEM_SLOT_INV_4, ITEM_SLOT_INV_5, ITEM_SLOT_INV_6 };
	int i = 0;
	ItemType itemId { static_cast<ItemType>(-1) };
	int itemAppearance = rand();
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
				Status itemStatus = static_cast<Status>(stats->EDITOR_ITEMS[itemSlots[i] + 1]);
				if ( itemStatus == 0 )
				{
					itemStatus = static_cast<Status>(DECREPIT + rand() % 4);
				}
				else if ( itemStatus > BROKEN )
				{
					itemStatus = static_cast<Status>(itemStatus - 1); // reserved '0' for random, so '1' is decrepit... etc to '5' being excellent.
				}
				int itemBless = stats->EDITOR_ITEMS[itemSlots[i] + 2];
				if ( itemBless == 10 )
				{
					itemBless = -1 + rand() % 3;
				}
				const int itemCount = stats->EDITOR_ITEMS[itemSlots[i] + 3];
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

node_t* itemNodeInInventory(const Stat* const myStats, const int32_t itemToFind, const Category cat)
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
		Item* item = static_cast<Item*>(node->element);
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

node_t* spellbookNodeInInventory(const Stat* const myStats, const int spellIDToFind)
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
		Item* item = static_cast<Item*>(node->element);
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

node_t* getRangedWeaponItemNodeInInventory(const Stat* const myStats, const bool includeMagicstaff)
{
	if ( myStats == nullptr )
	{
		return nullptr;
	}

	for ( node_t* node = myStats->inventory.first; node != nullptr; node = node->next )
	{
		Item* item = static_cast<Item*>(node->element);
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

node_t* getMeleeWeaponItemNodeInInventory(const Stat* const myStats)
{
	if ( myStats == nullptr )
	{
		return nullptr;
	}

	for ( node_t* node = myStats->inventory.first; node != nullptr; node = node->next )
	{
		Item* item = static_cast<Item*>(node->element);
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
		case LONGBOW:
		case COMPOUND_BOW:
		case HEAVY_CROSSBOW:
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

bool swapMonsterWeaponWithInventoryItem(Entity* const my, Stat* const myStats, node_t* const inventoryNode, const bool moveStack,  const bool overrideCursed)
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

	item = static_cast<Item*>(inventoryNode->element);
	
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
}

bool monsterUnequipSlot(Stat* const myStats, Item** const slot, Item* const itemToUnequip)
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

bool monsterUnequipSlotFromCategory(Stat* const myStats, Item** const slot, const Category cat)
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

void copyItem(Item* const itemToSet, const Item* const itemToCopy) //This should probably use references instead...
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
	itemToSet->isDroppable = itemToCopy->isDroppable;
}

ItemType itemTypeWithinGoldValue(const int cat, const int minValue, const int maxValue)
{
	const int numitems = NUMITEMS;
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
	int pick = rand() % numoftype;// prng_get_uint() % numoftype;
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

bool Item::isThisABetterWeapon(const Item& newWeapon, const Item* const weaponAlreadyHave)
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

bool Item::isThisABetterArmor(const Item& newArmor, const Item* const armorAlreadyHave)
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

	if ( itemTypeIsQuiver(armorAlreadyHave->type) )
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

bool Item::shouldItemStack(const int player) const
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
				&& this->type != TOOL_ALEMBIC
				&& this->type != TOOL_TINKERING_KIT
				&& this->type != ENCHANTED_FEATHER )
			|| itemCategory(this) == THROWN
			|| itemCategory(this) == GEM
			|| itemCategory(this) == POTION
			|| (itemCategory(this) == TOOL 
				&& this->type != TOOL_PICKAXE 
				&& this->type != TOOL_ALEMBIC 
				&& this->type != TOOL_TINKERING_KIT
				&& this->type != ENCHANTED_FEATHER)
			)
		{
			// THROWN, GEM, TOOLS, POTIONS should stack when equipped.
			// otherwise most equippables should not stack.
			if ( itemCategory(this) == THROWN || itemCategory(this) == GEM )
			{
				if ( count >= 9 )
				{
					return false;
				}
			}
			else if ( itemTypeIsQuiver(this->type) )
			{
				if ( count >= QUIVER_MAX_AMMO_QTY - 1 )
				{
					return false;
				}
				return true;
			}
			else if ( type == TOOL_METAL_SCRAP || type == TOOL_MAGIC_SCRAP )
			{
				if ( count >= SCRAP_MAX_STACK_QTY - 1 )
				{
					return false;
				}
				return true;
			}
			else if ( type == TOOL_SPELLBOT || type == TOOL_DUMMYBOT || type == TOOL_SENTRYBOT || type == TOOL_GYROBOT )
			{
				return false;
			}
			return true;
		}
	}
	return false;
}


bool shouldInvertEquipmentBeatitude(const Stat* const wielder)
{
	if ( wielder->type == SUCCUBUS || wielder->type == INCUBUS )
	{
		return true;
	}
	return false;
}

bool isItemEquippableInShieldSlot(const Item* const item)
{
	if ( !item )
	{
		return false;
	}

	if ( itemTypeIsQuiver(item->type) )
	{
		return true;
	}

	switch ( item->type )
	{
		case WOODEN_SHIELD:
		case BRONZE_SHIELD:
		case IRON_SHIELD:
		case STEEL_SHIELD:
		case STEEL_SHIELD_RESISTANCE:
		case MIRROR_SHIELD:
		case CRYSTAL_SHIELD:
		case TOOL_TORCH:
		case TOOL_LANTERN:
		case TOOL_CRYSTALSHARD:
			return true;
		default:
			break;
	}
	return false;
}

bool Item::usableWhileShapeshifted(const Stat* const wielder) const
{
	if ( !wielder )
	{
		return true;
	}
	switch ( itemCategory(this) )
	{
		case WEAPON:
		case ARMOR:
		case GEM:
		case THROWN:
		case TOOL:
		case BOOK:
		case SCROLL:
			return false;
		case MAGICSTAFF:
		case SPELLBOOK:
		{
			if (wielder->type == CREATURE_IMP)
			{
				return true;
			}

			return false;
		}
		case POTION:
		{
			if (type == POTION_EMPTY)
			{
				return false;
			}

			return true;
		}
		case AMULET:
		case RING:
		case FOOD:
		case SPELL_CAT:
			return true;
		default:
			break;
	}
	return false;
}

char* Item::getScrollLabel() const
{
	if ( enchantedFeatherScrollsShuffled.empty() )
	{
		strcpy(tempstr, "");
		return tempstr;
	}
	std::vector<int> indices;
	for ( int i = 0; i < NUMLABELS && i < enchantedFeatherScrollsShuffled.size(); ++i )
	{
		if ( enchantedFeatherScrollsShuffled.at(i) == this->type )
		{
			indices.push_back(i);
		}
	}

	if ( indices.empty() )
	{
		strcpy(tempstr, "");
		return tempstr;
	}
	int chosenLabel = 0;
	if ( this->appearance >= indices.size() )
	{
		chosenLabel = indices[this->appearance % indices.size()];
	}
	else
	{
		chosenLabel = indices[this->appearance];
	}
	return scroll_label[chosenLabel];
}

bool itemSpriteIsQuiverThirdPersonModel(const int sprite)
{
	for ( int i = QUIVER_SILVER; i <= QUIVER_HUNTING; ++i )
	{
		if ( sprite == items[i].index
			|| sprite == items[i].index + 1
			|| sprite == items[i].index + 2
			|| sprite == items[i].index + 3 )
		{
			return true;
		}
	}
	return false;
}

bool itemSpriteIsQuiverBaseThirdPersonModel(const int sprite)
{
	for ( int i = QUIVER_SILVER; i <= QUIVER_HUNTING; ++i )
	{
		if ( sprite == items[i].index + 1 )
		{
			return true;
		}
	}
	return false;
}

bool itemTypeIsQuiver(const ItemType type)
{
	return (type >= QUIVER_SILVER && type <= QUIVER_HUNTING);
}

real_t rangedAttackGetSpeedModifier(const Stat* const myStats)
{
	if ( !myStats || !myStats->weapon )
	{
		return 1.0;
	}

	real_t bowModifier = 1.00;
	real_t arrowModifier = 0.0;
	if ( myStats->shield )
	{
		if ( myStats->shield->type == QUIVER_LIGHTWEIGHT )
		{
			arrowModifier = -.5;
		}
	}

	if ( myStats->weapon->type == LONGBOW )
	{
		bowModifier = 1.25;
	}
	else if ( myStats->weapon->type == COMPOUND_BOW )
	{
		bowModifier = 0.75;
		arrowModifier /= 2;
	}
	else if ( myStats->weapon->type == SLING )
	{
		bowModifier = 0.75;
		arrowModifier = 0.0; // no impact on slings.
	}
	else if ( myStats->weapon->type == HEAVY_CROSSBOW )
	{
		bowModifier = 0.4;
		return std::max(0.1, bowModifier + arrowModifier);
	}
	else
	{
		bowModifier = 1.00;
	}

	return std::max(0.25, bowModifier + arrowModifier);
}

bool rangedWeaponUseQuiverOnAttack(const Stat* const myStats)
{
	if ( !myStats || !myStats->weapon || !myStats->shield )
	{
		return false;
	}
	if ( !isRangedWeapon(*myStats->weapon) )
	{
		return false;
	}

	if ( myStats->shield && itemTypeIsQuiver(myStats->shield->type) && !(myStats->weapon && myStats->weapon->type == SLING) )
	{
		return true;
	}
	return false;
}

bool itemSpriteIsBreastpiece(const int sprite)
{
	if ( sprite < 0 || sprite > NUMITEMS )
	{
		return false;
	}
	if ( sprite == items[LEATHER_BREASTPIECE].index
		|| sprite == items[IRON_BREASTPIECE].index
		|| sprite == items[STEEL_BREASTPIECE].index
		|| sprite == items[CRYSTAL_BREASTPIECE].index
		|| sprite == items[VAMPIRE_DOUBLET].index
		|| sprite == items[WIZARD_DOUBLET].index
		|| sprite == items[HEALER_DOUBLET].index
		|| sprite == items[SILVER_DOUBLET].index
		|| sprite == items[ARTIFACT_BREASTPIECE].index
		|| sprite == items[TUNIC].index
		|| sprite == items[MACHINIST_APRON].index )
	{
		return true;
	}
	return false;
}

real_t getArtifactWeaponEffectChance(const ItemType type, Stat& wielder, real_t* const effectAmount)
{
	if ( type == ARTIFACT_AXE )
	{
		const real_t percent = 25 * (wielder.PROFICIENCIES[PRO_AXE]) / 100.f; //0-25%
		if ( effectAmount )
		{
			*effectAmount = 1.5; //1.5x damage.
		}

		return percent;
	}
	else if ( type == ARTIFACT_SWORD )
	{
		const real_t percent = (wielder.PROFICIENCIES[PRO_SWORD]); //0-100%
		if ( effectAmount )
		{
			*effectAmount = (wielder.PROFICIENCIES[PRO_SWORD]) / 200.f + 0.5; //0.5x-1.0x add to weapon multiplier
		}

		return percent;
	}
	else if ( type == ARTIFACT_SPEAR )
	{
		const real_t percent = 25 * (wielder.PROFICIENCIES[PRO_POLEARM]) / 100.f; //0-25%
		if ( effectAmount )
		{
			*effectAmount = .5; // bypasses 50% enemies' armor.
		}

		return percent;
	}
	else if ( type == ARTIFACT_MACE )
	{
		const real_t percent = 1.f; //100%
		if ( effectAmount )
		{
			*effectAmount = wielder.PROFICIENCIES[PRO_MACE]; // 0-2 second bonus mana regen
		}

		return percent;
	}
	else if ( type == ARTIFACT_BOW )
	{
		const real_t percent = wielder.PROFICIENCIES[PRO_RANGED] / 2.f; //0-50%
		if ( effectAmount )
		{
			*effectAmount = 0.f; // no use here.
		}

		return percent;
	}

	return 0.0;
}

bool Item::unableToEquipDueToSwapWeaponTimer() const
{
	if ( pickaxeGimpTimer > 0 && !intro )
	{
		return true;
	}
	if ( swapWeaponGimpTimer > 0 && !intro )
	{
		return true;
	}
	return false;

	// not needed, block all items?
	/*if ( itemCategory(this) == POTION || itemCategory(this) == GEM || itemCategory(this) == THROWN
		|| itemTypeIsQuiver(this->type) || this->type == FOOD_CREAMPIE )
	{
		return true;
	}
	return false;*/
}

bool Item::tinkeringBotIsMaxHealth() const
{
	if ( type == TOOL_GYROBOT || type == TOOL_DUMMYBOT || type == TOOL_SENTRYBOT || type == TOOL_SPELLBOT )
	{
		if ( appearance == ITEM_TINKERING_APPEARANCE || (appearance > 0 && appearance % 10 == 0) )
		{
			return true;
		}
	}
	return false;
}

bool Item::isTinkeringItemWithThrownLimit() const
{
	if ( type == TOOL_SENTRYBOT || type == TOOL_SPELLBOT || type == TOOL_DUMMYBOT || type == TOOL_GYROBOT )
	{
		return true;
	}
	return false;
}

int maximumTinkeringBotsCanBeDeployed(const Stat* const myStats)
{
	if ( !myStats )
	{
		return 0;
	}
	int maxFollowers = 2;
	const int skillLVL = myStats->PROFICIENCIES[PRO_LOCKPICKING] / 20; // 0-5.
	switch ( skillLVL )
	{
		case 0:
		case 1:
			maxFollowers = 2;
			break;
		case 2:
			maxFollowers = 4;
			break;
		case 3:
			maxFollowers = 6;
			break;
		case 4:
			maxFollowers = 8;
			break;
		case 5:
			maxFollowers = 10;
			break;
		default:
			break;
	}
	return maxFollowers;
}

bool playerCanSpawnMoreTinkeringBots(const Stat* const myStats)
{
	if ( !myStats )
	{
		return false;
	}
	if ( overrideTinkeringLimit )
	{
		return true;
	}
	int numBots = 0;
	for ( node_t* node = myStats->FOLLOWERS.first; node != nullptr; node = node->next )
	{
		Entity* follower = uidToEntity(*static_cast<Uint32*>(node->element));
		if ( follower )
		{
			Stat* followerStats = follower->getStats();
			if ( followerStats )
			{
				if ( followerStats->type == SENTRYBOT || followerStats->type == GYROBOT
					|| followerStats->type == SPELLBOT || followerStats->type == DUMMYBOT )
				{
					++numBots;
				}
			}
		}
	}
	if ( numBots < maximumTinkeringBotsCanBeDeployed(myStats) )
	{
		return true;
	}
	return false;
}

void playerTryEquipItemAndUpdateServer(Item* const item)
{
	if ( !item )
	{
		return;
	}
	if ( multiplayer == CLIENT )
	{
		// store these to send to server.
		const ItemType type = item->type;
		const Status status = item->status;
		const Sint16 beatitude = item->beatitude;
		const int count = item->count;
		const Uint32 appearance = item->appearance;
		const bool identified = item->identified;

		const Category cat = itemCategory(item);

		EquipItemResult equipResult = EQUIP_ITEM_FAIL_CANT_UNEQUIP;
		if ( cat == SPELLBOOK )
		{
			if ( !cast_animation.active_spellbook )
			{
				equipResult = equipItem(item, &stats[clientnum]->shield, clientnum);
			}
		}
		else
		{
			equipResult = equipItem(item, &stats[clientnum]->weapon, clientnum);
		}
		if ( equipResult != EQUIP_ITEM_FAIL_CANT_UNEQUIP )
		{
			if ( cat == SPELLBOOK )
			{
				if ( !cast_animation.active_spellbook )
				{
					clientSendEquipUpdateToServer(EQUIP_ITEM_SLOT_SHIELD, equipResult, clientnum, 
						type, status, beatitude, count, appearance, identified);
				}
			}
			else
			{
				clientSendEquipUpdateToServer(EQUIP_ITEM_SLOT_WEAPON, equipResult, clientnum,
					type, status, beatitude, count, appearance, identified);
			}
		}
	}
	else
	{
		// server/singleplayer
		EquipItemResult equipResult = EQUIP_ITEM_FAIL_CANT_UNEQUIP;
		if ( itemCategory(item) == SPELLBOOK )
		{
			if ( !cast_animation.active_spellbook )
			{
				equipResult = equipItem(item, &stats[clientnum]->shield, clientnum);
			}
		}
		else
		{
			equipResult = equipItem(item, &stats[clientnum]->weapon, clientnum);
		}
	}
}

void clientSendEquipUpdateToServer(const EquipItemSendToServerSlot slot, const EquipItemResult equipType, const int player,
	const ItemType type, const Status status, const Sint16 beatitude, const int count, const Uint32 appearance, const bool identified)
{
	if ( slot == EQUIP_ITEM_SLOT_SHIELD )
	{
		strcpy((char*)net_packet->data, "EQUS");
	}
	else if ( slot == EQUIP_ITEM_SLOT_WEAPON )
	{
		strcpy((char*)net_packet->data, "EQUI");
	}
	else
	{
		strcpy((char*)net_packet->data, "EQUM");
	}
	SDLNet_Write32(static_cast<Uint32>(type), &net_packet->data[4]);
	SDLNet_Write32(static_cast<Uint32>(status), &net_packet->data[8]);
	SDLNet_Write32(static_cast<Uint32>(beatitude), &net_packet->data[12]);
	SDLNet_Write32(static_cast<Uint32>(count), &net_packet->data[16]);
	SDLNet_Write32(static_cast<Uint32>(appearance), &net_packet->data[20]);
	net_packet->data[24] = identified;
	net_packet->data[25] = player;
	net_packet->data[26] = equipType;
	net_packet->data[27] = slot;
	net_packet->address.host = net_server.host;
	net_packet->address.port = net_server.port;
	net_packet->len = 28;
	sendPacketSafe(net_sock, -1, net_packet, 0);
}

void clientUnequipSlotAndUpdateServer(const EquipItemSendToServerSlot slot, Item* const item)
{
	if ( !item )
	{
		return;
	}
	EquipItemResult equipType = EQUIP_ITEM_FAIL_CANT_UNEQUIP;

	if ( slot == EQUIP_ITEM_SLOT_HELM )
	{
		equipType = equipItem(item, &stats[clientnum]->helmet, clientnum);
	}
	else if ( slot == EQUIP_ITEM_SLOT_BREASTPLATE )
	{
		equipType = equipItem(item, &stats[clientnum]->breastplate, clientnum);
	}
	else if ( slot == EQUIP_ITEM_SLOT_GLOVES )
	{
		equipType = equipItem(item, &stats[clientnum]->gloves, clientnum);
	}
	else if ( slot == EQUIP_ITEM_SLOT_BOOTS )
	{
		equipType = equipItem(item, &stats[clientnum]->shoes, clientnum);
	}
	else if ( slot == EQUIP_ITEM_SLOT_SHIELD )
	{
		equipType = equipItem(item, &stats[clientnum]->shield, clientnum);
	}
	else if ( slot == EQUIP_ITEM_SLOT_CLOAK )
	{
		equipType = equipItem(item, &stats[clientnum]->cloak, clientnum);
	}
	else if ( slot == EQUIP_ITEM_SLOT_AMULET )
	{
		equipType = equipItem(item, &stats[clientnum]->amulet, clientnum);
	}
	else if ( slot == EQUIP_ITEM_SLOT_RING )
	{
		equipType = equipItem(item, &stats[clientnum]->ring, clientnum);
	}
	else if ( slot == EQUIP_ITEM_SLOT_MASK )
	{
		equipType = equipItem(item, &stats[clientnum]->mask, clientnum);
	}

	clientSendEquipUpdateToServer(slot, equipType, clientnum,
		item->type, item->status, item->beatitude, item->count, item->appearance, item->identified);
}