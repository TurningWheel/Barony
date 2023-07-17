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
#include "engine/audio/sound.hpp"
#include "book.hpp"
#include "scrolls.hpp"
#include "shops.hpp"
#include "prng.hpp"
#include "scores.hpp"
#include "net.hpp"
#include "player.hpp"
#include "mod_tools.hpp"

#include <assert.h>

Uint32 itemuids = 1;
ItemGeneric items[NUMITEMS];
const real_t potionDamageSkillMultipliers[6] = { 1.f, 1.1, 1.25, 1.5, 2.5, 4.f };
const real_t thrownDamageSkillMultipliers[6] = { 1.f, 1.1, 1.25, 1.5, 2.f, 3.f };
Uint32 enchantedFeatherScrollSeed;
std::vector<int> enchantedFeatherScrollsShuffled;
bool overrideTinkeringLimit = false;
int decoyBoxRange = 15;

bool autoHotbarSoftReserveItem(Item& item)
{
	Category cat = itemCategory(&item);
	if ( cat == THROWN || item.type == GEM_ROCK || itemIsThrowableTinkerTool(&item) || item.type == TOOL_BEARTRAP )
	{
		return true;
	}
	return false;
}

void autoHotbarTryAdd(const int player, Item& item)
{
	if ( players[player] && players[player]->entity && players[player]->entity->effectShapeshift != NOTHING )
	{
		if ( !item.usableWhileShapeshifted(stats[player]) )
		{
			return;
		}
	}
	if ( !autoAddHotbarFilter(item) )
	{
		return;
	}
	if ( item.type == SPELL_ITEM && players[player]->magic.spellbookUidFromHotbarSlot != 0 )
	{
		return; // we're going to replace our spellbook slot
	}
	Category cat = itemCategory(&item);
	if ( !item.identified && cat != SPELL_CAT ) { return; }

	std::vector<std::tuple<int, int, hotbar_slot_t*>> slots(NUM_HOTBAR_SLOTS); // index, priority, then stored item info
	size_t index = 0;
	const int DEFAULT_PRIORITY = 5;
	const int RESERVED_PRIORITY = 4;
	for ( auto& slot : players[player]->hotbar.slots() )
	{
		std::get<0>(slots[index]) = index;
		if ( uidToItem(slot.item) )
		{
			std::get<1>(slots[index]) = -1; // taken
			std::get<2>(slots[index]) = nullptr;
			++index;
			continue;
		}

		if ( slot.lastItem.type == item.type )
		{
			std::get<1>(slots[index]) = DEFAULT_PRIORITY + 1;
		}
		else if ( autoHotbarSoftReserveItem(slot.lastItem) )
		{
			std::get<1>(slots[index]) = RESERVED_PRIORITY;
		}
		else
		{
			std::get<1>(slots[index]) = DEFAULT_PRIORITY;
		}
		std::get<2>(slots[index]) = &slot;
		++index;
	}


	bool softReserveSlots = autoHotbarSoftReserveItem(item);
	if ( softReserveSlots )
	{
		for ( auto& slot : slots )
		{
			auto& priority = std::get<1>(slot);
			if ( priority < 0 ) { continue; }

			auto hotbar_slot = std::get<2>(slot);
			if ( !hotbar_slot ) { continue; }
			if ( !hotbar_slot->lastItem.identified ) { continue; } // no good

			if ( hotbar_slot->lastItem.type == item.type )
			{
				priority = std::max(priority, 10); // match item type, good
				if ( hotbar_slot->lastItem.status == item.status )
				{
					priority = std::max(priority, 15); // match item status, good+
					if ( hotbar_slot->lastItem.beatitude == item.beatitude )
					{
						priority = std::max(priority, 20); // match item beatitude, good++
					}
				}
			}
			else if ( autoHotbarSoftReserveItem(hotbar_slot->lastItem) ) // these items are interchangeable, non matching types
			{
				priority = std::max(priority, RESERVED_PRIORITY - 1); // slightly less than normal reserved slot
			}
		}
	}

	std::pair<int, int> indexAndPriorityToPick = { -1, 0 };
	for ( auto& slot : slots )
	{
		auto& priority = std::get<1>(slot);
		if ( priority > indexAndPriorityToPick.second )
		{
			indexAndPriorityToPick.second = priority;
			indexAndPriorityToPick.first = std::get<0>(slot);
		}
	}

	if ( indexAndPriorityToPick.first >= 0 )
	{
		size_t index = indexAndPriorityToPick.first;
		players[player]->hotbar.slots()[index].item = item.uid;
		players[player]->hotbar.slots()[index].storeLastItem(&item);
		if ( item.type == BOOMERANG )
		{
			players[player]->hotbar.magicBoomerangHotbarSlot = index;
		}
		return;
	}

	for ( auto& hotbarSlot : players[player]->hotbar.slots() )
	{
		if ( !uidToItem(hotbarSlot.item) )
		{
			if ( autoAddHotbarFilter(item) )
			{
				hotbarSlot.item = item.uid;
				break;
			}
		}
	}
}

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
	// try to sanitize these a bit so that corrupt data doesn't crash the whole game
	item->type = (type >= 0 && type < NUMITEMS) ? type : ItemType::GEM_ROCK;
	item->status = (int)status < Status::BROKEN ?
		Status::BROKEN : ((int)status > EXCELLENT ? EXCELLENT : status);
	item->beatitude = std::min(std::max((Sint16)-100, beatitude), (Sint16)100);
	item->count = std::max(count, (Sint16)1);
	item->appearance = appearance;
	item->identified = identified;
	item->uid = itemuids;
	item->ownerUid = 0;
	item->isDroppable = true;
	item->playerSoldItemToShop = false;
	item->itemHiddenFromShop = false;
	item->itemRequireTradingSkillInShop = 0;
	item->itemSpecialShopConsumable = false;
	item->interactNPCUid = 0;
	item->notifyIcon = false;
	if ( inventory )
	{
		Player::Inventory_t* playerInventoryUI = nullptr;
		Player::Magic_t* playerMagic = nullptr;
		int player = -1;
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( stats[i] && inventory == &stats[i]->inventory )
			{
				playerInventoryUI = &players[i]->inventoryUI;
				playerMagic = &players[i]->magic;
				player = i;
				break;
			}
		}

		if ( !playerInventoryUI )
		{
			//printlog("warning: newItem inventory was not a local player?");
			itemuids++;
			return item;
		}

		if ( player >= 0 )
		{
			playerInventoryUI->moveItemToFreeInventorySlot(item);
		}

		// add the item to the hotbar automatically
		if ( !intro && auto_hotbar_new_items )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( !players[i]->isLocalPlayer() )
				{
					continue;
				}
				if ( stats[i] && inventory == &stats[i]->inventory )
				{
					autoHotbarTryAdd(i, *item);
					break;
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
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( !players[i]->isLocalPlayer() )
		{
			continue;
		}
		for ( node_t* node = stats[i]->inventory.first; node != nullptr; node = node->next )
		{
			Item* item = static_cast<Item*>(node->element);
			if ( item->uid == uid )
			{
				return item;
			}
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
						if ( map_rng.rand() % 2 )   // 50% chance
						{
							chances[c] = true;
						}
						break;
					case TOOL_LANTERN:
						if ( map_rng.rand() % 4 )   // 75% chance
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
		if ( map_rng.rand() % 10 )
		{
			return GEM_GLASS;
		}
	}

	// pick the item
	Uint32 pick = map_rng.rand() % numleft;
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
							if ( map_rng.rand() % 2 )   // 50% chance
							{
								chances[c] = false;
							}
							break;
						case TOOL_LANTERN:
							if ( map_rng.rand() % 4 == 0 )   // 25% chance
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
							if ( map_rng.rand() % 4 )   // 25% chance
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
		if ( map_rng.rand() % 10 )
		{
			return GEM_GLASS;
		}
	}

	// pick the item
	Uint32 pick = map_rng.rand() % numleft;
	for ( c = 0; c < numitems; c++ )
	{
		if ( items[c].category == cat )
		{
			if ( chances[c] == true )
			{
				if ( pick == 0 )
				{
					//messagePlayer(0, "Chose item: %s of %d items.", items[c].getIdentifiedName() ,numleft);
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
			if ( type >= ARTIFACT_ORB_BLUE && type <= ARTIFACT_ORB_GREEN )
			{
				snprintf(tempstr, 1024, Language::get(987 + status), beatitude);
			}
			else if ( itemCategory(this) == WEAPON || itemCategory(this) == ARMOR || itemCategory(this) == MAGICSTAFF || itemCategory(this) == TOOL || itemCategory(this) == THROWN )
			{
				if ( this->type == TOOL_GYROBOT || this->type == TOOL_DUMMYBOT || this->type == TOOL_SENTRYBOT || this->type == TOOL_SPELLBOT )
				{
					snprintf(tempstr, 1024, "%s", Language::get(3653 + status));
				}
				else if ( itemTypeIsQuiver(this->type) )
				{
					snprintf(tempstr, 1024, Language::get(3738), beatitude);
				}
				else
				{
					snprintf(tempstr, 1024, Language::get(982 + status), beatitude);
				}
			}
			else if ( itemCategory(this) == AMULET || itemCategory(this) == RING || itemCategory(this) == GEM )
			{
				snprintf(tempstr, 1024, Language::get(987 + status), beatitude);
			}
			else if ( itemCategory(this) == POTION )
			{
				if ( type == POTION_EMPTY )
				{
					//No fancy descriptives for empty potions.
					snprintf(tempstr, 1024, Language::get(982 + status), beatitude);
				}
				else
				{
					snprintf(tempstr, 1024, Language::get(992 + status), Language::get(974 + items[type].index + appearance % items[type].variations - 50), beatitude);
				}
			}
			else if ( itemCategory(this) == SCROLL || itemCategory(this) == SPELLBOOK || itemCategory(this) == BOOK )
			{
				snprintf(tempstr, 1024, Language::get(997 + status), beatitude);
			}
			else if ( itemCategory(this) == FOOD )
			{
				snprintf(tempstr, 1024, Language::get(1002 + status), beatitude);
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
					snprintf(&tempstr[c], 1024 - c, Language::get(1007), getBookNameFromIndex(appearance % numbooks).c_str());
				}
				else
				{
					snprintf(&tempstr[c], 1024 - c, "%s", items[type].getIdentifiedName());
				}
			}
			else
			{
				snprintf(&tempstr[c], 1024 - c, "ITEM%03d", type);
			}
		}
		else
		{
			if ( type >= ARTIFACT_ORB_BLUE && type <= ARTIFACT_ORB_GREEN )
			{
				snprintf(tempstr, 1024, Language::get(1023 + status), count, beatitude);
			}
			else if ( itemCategory(this) == WEAPON || itemCategory(this) == ARMOR || itemCategory(this) == MAGICSTAFF || itemCategory(this) == TOOL || itemCategory(this) == THROWN )
			{
				if ( this->type == TOOL_GYROBOT || this->type == TOOL_DUMMYBOT || this->type == TOOL_SENTRYBOT || this->type == TOOL_SPELLBOT )
				{
					snprintf(tempstr, 1024, Language::get(3658 + status), count);
				}
				else if ( itemTypeIsQuiver(this->type) )
				{
					snprintf(tempstr, 1024, Language::get(3738), beatitude);
				}
				else
				{
					snprintf(tempstr, 1024, Language::get(1008 + status), count, beatitude);
				}
			}
			else if ( itemCategory(this) == AMULET || itemCategory(this) == RING || itemCategory(this) == GEM )
			{
				snprintf(tempstr, 1024, Language::get(1013 + status), count, beatitude);
			}
			else if ( itemCategory(this) == POTION )
			{
				if ( type == POTION_EMPTY )
				{
					//No fancy descriptives for empty potions.
					snprintf(tempstr, 1024, Language::get(1008 + status), count, beatitude);
				}
				else
				{
					snprintf(tempstr, 1024, Language::get(1018 + status), count, Language::get(974 + items[type].index + appearance % items[type].variations - 50), beatitude);
				}
			}
			else if ( itemCategory(this) == SCROLL || itemCategory(this) == SPELLBOOK || itemCategory(this) == BOOK )
			{
				snprintf(tempstr, 1024, Language::get(1023 + status), count, beatitude);
			}
			else if ( itemCategory(this) == FOOD )
			{
				snprintf(tempstr, 1024, Language::get(1028 + status), count, beatitude);
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
					snprintf(&tempstr[c], 1024 - c, Language::get(1033), count, getBookNameFromIndex(appearance % numbooks).c_str());
				}
				else
				{
					snprintf(&tempstr[c], 1024 - c, "%s", items[type].getIdentifiedName());
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
			if ( type >= ARTIFACT_ORB_BLUE && type <= ARTIFACT_ORB_GREEN )
			{
				strncpy(tempstr, Language::get(1049 + status), 1024);
			}
			else if ( itemCategory(this) == WEAPON || itemCategory(this) == ARMOR || itemCategory(this) == MAGICSTAFF || itemCategory(this) == TOOL || itemCategory(this) == THROWN )
			{
				if ( this->type == TOOL_GYROBOT || this->type == TOOL_DUMMYBOT || this->type == TOOL_SENTRYBOT || this->type == TOOL_SPELLBOT )
				{
					strncpy(tempstr, Language::get(3653 + status), 1024);
				}
				else if ( itemTypeIsQuiver(this->type) )
				{
					snprintf(tempstr, 1024, "%s", Language::get(3763));
				}
				else
				{
					strncpy(tempstr, Language::get(1034 + status), 1024);
				}
			}
			else if ( itemCategory(this) == AMULET || itemCategory(this) == RING || itemCategory(this) == GEM )
			{
				strncpy(tempstr, Language::get(1039 + status), 1024);
			}
			else if ( itemCategory(this) == POTION )
			{
				if ( type == POTION_EMPTY )
				{
					//No fancy descriptives for empty potions.
					snprintf(tempstr, 1024, Language::get(1034 + status), beatitude);
				}
				else
				{
					snprintf(tempstr, 1024, Language::get(1044 + status), Language::get(974 + items[type].index + appearance % items[type].variations - 50));
				}
			}
			else if ( itemCategory(this) == SCROLL || itemCategory(this) == SPELLBOOK || itemCategory(this) == BOOK )
			{
				strncpy(tempstr, Language::get(1049 + status), 1024);
			}
			else if ( itemCategory(this) == FOOD )
			{
				strncpy(tempstr, Language::get(1054 + status), 1024);
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
					snprintf(&tempstr[c], 1024 - c, Language::get(1059), items[type].getUnidentifiedName(), this->getScrollLabel());
				}
				else
				{
					if ( itemCategory(this) == BOOK )
					{
						snprintf(&tempstr[c], 1024 - c, Language::get(1007), getBookNameFromIndex(appearance % numbooks).c_str());
					}
					else
					{
						snprintf(&tempstr[c], 1024 - c, "%s", items[type].getUnidentifiedName());
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
			if ( type >= ARTIFACT_ORB_BLUE && type <= ARTIFACT_ORB_GREEN )
			{
				snprintf(tempstr, 1024, Language::get(1065 + status), count);
			}
			else if ( itemCategory(this) == WEAPON || itemCategory(this) == ARMOR || itemCategory(this) == MAGICSTAFF || itemCategory(this) == TOOL || itemCategory(this) == THROWN )
			{
				if ( this->type == TOOL_GYROBOT || this->type == TOOL_DUMMYBOT || this->type == TOOL_SENTRYBOT || this->type == TOOL_SPELLBOT )
				{
					snprintf(tempstr, 1024, Language::get(3658 + status), count);
				}
				else if ( itemTypeIsQuiver(this->type) )
				{
					snprintf(tempstr, 1024, "%s", Language::get(3763));
				}
				else
				{
					snprintf(tempstr, 1024, Language::get(1060 + status), count);
				}
			}
			else if ( itemCategory(this) == AMULET || itemCategory(this) == RING || itemCategory(this) == GEM )
			{
				snprintf(tempstr, 1024, Language::get(1065 + status), count);
			}
			else if ( itemCategory(this) == POTION )
			{
				if ( type == POTION_EMPTY )
				{
					//No fancy descriptives for empty potions.
					snprintf(tempstr, 1024, Language::get(1060 + status), count);
				}
				else
				{
					snprintf(tempstr, 1024, Language::get(1070 + status), count, Language::get(974 + items[type].index + appearance % items[type].variations - 50));
				}
			}
			else if ( itemCategory(this) == SCROLL || itemCategory(this) == SPELLBOOK || itemCategory(this) == BOOK )
			{
				snprintf(tempstr, 1024, Language::get(1075 + status), count);
			}
			else if ( itemCategory(this) == FOOD )
			{
				snprintf(tempstr, 1024, Language::get(1080 + status), count);
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
					snprintf(&tempstr[c], 1024 - c, Language::get(1085), items[type].getUnidentifiedName(), this->getScrollLabel());
				}
				else
				{
					if ( itemCategory(this) == BOOK )
					{
						snprintf(&tempstr[c], 1024 - c, Language::get(1086), count, getBookNameFromIndex(appearance % numbooks).c_str());
					}
					else
					{
						snprintf(&tempstr[c], 1024 - c, "%s", items[type].getUnidentifiedName());
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
	if ( !item || item->type < 0 || item->type >= NUMITEMS )
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
				snprintf(tempstr, sizeof(tempstr), Language::get(1007), getBookNameFromIndex(appearance % numbooks).c_str());
			}
			else
			{
				strcpy(tempstr, items[type].getIdentifiedName());
			}
		}
		else
		{
			if ( itemCategory(this) == SCROLL )
			{
				snprintf(tempstr, sizeof(tempstr), Language::get(1059), items[type].getUnidentifiedName(), this->getScrollLabel());
			}
			else if ( itemCategory(this) == BOOK )
			{
				snprintf(tempstr, sizeof(tempstr), Language::get(1007), getBookNameFromIndex(appearance % numbooks).c_str());
			}
			else
			{
				strcpy(tempstr, items[type].getUnidentifiedName());
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
	if ( !item || item->type < 0 || item->type >= NUMITEMS )
	{
		return 0;
	}
	if ( item->type == TOOL_PLAYER_LOOT_BAG )
	{
		if ( colorblind_lobby )
		{
			int playerOwner = (item->appearance) % MAXPLAYERS;
			Uint32 index = 4;
			switch ( playerOwner )
			{
			case 0:
				index = 2;
				break;
			case 1:
				index = 3;
				break;
			case 2:
				index = 1;
				break;
			case 3:
				index = 4;
				break;
			default:
				break;
			}
			return items[item->type].index + index;
		}
		else
		{
			return items[item->type].index + item->appearance % MAXPLAYERS;
		}
	}
	return items[item->type].index + item->appearance % items[item->type].variations;
}

/*-------------------------------------------------------------------------------

	itemModelFirstperson

	returns the first person model of the given item

-------------------------------------------------------------------------------*/

Sint32 itemModelFirstperson(const Item* const item)
{
	if ( !item || item->type < 0 || item->type >= NUMITEMS )
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
	if ( !item || item->type < 0 || item->type >= NUMITEMS )
	{
		return nullptr;
	}
	if (itemCategory(item) == SPELL_CAT)
	{
		spell_t* spell = nullptr;
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			spell = getSpellFromItem(i, item);
			if ( spell )
			{
				break;
			}
		}
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
		node_t* node = nullptr;
		if ( item->type == TOOL_PLAYER_LOOT_BAG )
		{
			if ( colorblind_lobby )
			{
				int playerOwner = (item->appearance) % MAXPLAYERS;
				Uint32 index = 4;
				switch ( playerOwner )
				{
					case 0:
						index = 2;
						break;
					case 1:
						index = 3;
						break;
					case 2:
						index = 1;
						break;
					case 3:
						index = 4;
						break;
					default:
						break;
				}
				node = list_Node(&items[item->type].surfaces, index);
			}
			else
			{
				node = list_Node(&items[item->type].surfaces, (item->appearance) % MAXPLAYERS);
			}
		}
		else
		{
			node = list_Node(&items[item->type].surfaces, item->appearance % items[item->type].variations);
		}
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

int itemCompare(const Item* const item1, const Item* const item2, bool checkAppearance, bool comparisonUsedForStacking)
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
	else if ( item1->type == SCROLL_MAIL || item1->type == READABLE_BOOK || items[item1->type].category == SPELL_CAT
		|| item1->type == TOOL_PLAYER_LOOT_BAG )
	{
		if ( comparisonUsedForStacking )
		{
			return 1; // these items do not stack
		}
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

Uint32 dropItemSfxTicks[MAXPLAYERS] = { 0 };
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
				messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(3218));
			}
			else
			{
				messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(1087));
			}
			playSoundPlayer(player, 90, 64);
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
		if ( item == players[player]->bookGUI.openBookItem )
		{
			players[player]->bookGUI.closeBookGUI();
		}

		oldcount = item->count;
		if ( item->count >= 10 && (item->type == TOOL_METAL_SCRAP || item->type == TOOL_MAGIC_SCRAP) )
		{
			item->count = 10;
			messagePlayer(player, MESSAGE_SPAM_MISC, Language::get(1088), item->description());
			item->count = oldcount - 10;
		}
		else if ( itemTypeIsQuiver(item->type) )
		{
			item->count = 1;
			if ( notifyMessage )
			{
				messagePlayer(player, MESSAGE_SPAM_MISC, Language::get(1088), item->description());
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
				messagePlayer(player, MESSAGE_SPAM_MISC, Language::get(1088), item->description());
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

		players[player]->paperDoll.updateSlots();

		if ( item->count <= 0 )
		{
			list_RemoveNode(item->node);
			return true;
		}
		return false;
	}
	else
	{
		if ( item == players[player]->bookGUI.openBookItem )
		{
			players[player]->bookGUI.closeBookGUI();
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
		entity->vel_x = (1.5 + .025 * (local_rng.rand() % 11)) * cos(players[player]->entity->yaw);
		entity->vel_y = (1.5 + .025 * (local_rng.rand() % 11)) * sin(players[player]->entity->yaw);
		entity->vel_z = (-10 - local_rng.rand() % 20) * .01;
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

		// play sound - not in the same tick
		if ( ticks - dropItemSfxTicks[player] > 1 )
		{
			playSoundEntity( players[player]->entity, 47 + local_rng.rand() % 3, 64 );
		}
		dropItemSfxTicks[player] = ticks;

		// unequip the item
		Item** slot = itemSlot(stats[player], item);
		if ( slot != nullptr )
		{
			*slot = nullptr;
		}

		players[player]->paperDoll.updateSlots();

		if ( item->node != nullptr )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( !players[i]->isLocalPlayer() )
				{
					continue;
				}
				if ( item->node->list == &stats[i]->inventory )
				{
					oldcount = item->count;
					item->count = qtyToDrop;
					if ( notifyMessage )
					{
						messagePlayer(player, MESSAGE_SPAM_MISC, Language::get(1088), item->description());
					}
					item->count = oldcount - qtyToDrop;
					if ( item->count <= 0 )
					{
						list_RemoveNode(item->node);
						return true;
					}
					break;
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

	if ( monster->behavior == &actPlayer && players[monster->skill[2]]->isLocalPlayer() )
	{
		if ( item == inputs.getUIInteraction(monster->skill[2])->selectedItem )
		{
			inputs.getUIInteraction(monster->skill[2])->selectedItem = nullptr;
			inputs.getUIInteraction(monster->skill[2])->selectedItemFromChest = 0;
		}
	}
	else if ( monster->behavior == &actChest )
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( item == inputs.getUIInteraction(i)->selectedItem )
			{
				inputs.getUIInteraction(i)->selectedItem = nullptr;
				inputs.getUIInteraction(i)->selectedItemFromChest = 0;
			}
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
		entity->vel_x = (local_rng.rand() % 20 - 10) / 10.0;
		entity->vel_y = (local_rng.rand() % 20 - 10) / 10.0;
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
			else if ( monsterStats->type == GYROBOT )
			{
				entity->vel_x = 0.0;
				entity->vel_y = 0.0;
				entity->vel_z = -.5;
			}
			else if ( monsterStats->type == SENTRYBOT || monsterStats->type == SPELLBOT )
			{
				entity->vel_x *= 0.1;
				entity->vel_y *= 0.1;
				entity->vel_z = -.5;
			}
			else if ( item->type == ARTIFACT_ORB_PURPLE && monsterStats->type == LICH )
			{
				entity->vel_x = 0.0;
				entity->vel_y = 0.0;
				int ix = static_cast<int>(std::floor(monster->x)) >> 4;
				int iy = static_cast<int>(std::floor(monster->y)) >> 4;
				if ( map.tiles[OBSTACLELAYER + iy * MAPLAYERS + ix * MAPLAYERS * map.height]
					|| !map.tiles[iy * MAPLAYERS + ix * MAPLAYERS * map.height] )
				{
					// failsafe area in the center of the boss room
					entity->x = 36 * 16.0 + 8.0;
					entity->y = 17 * 16.0 + 8.0;
				}
				else
				{
					// drop in center of tile
					entity->x = ix * 16.0 + 8.0;
					entity->y = iy * 16.0 + 8.0;
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

		if ( monster->behavior == &actPlayer )
		{
			players[monster->skill[2]]->paperDoll.updateSlots();
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

	if ( player >= 0 && players[player]->isLocalPlayer() && players[player]->inventoryUI.appraisal.current_item == item->uid && item->count == 1 )
	{
		players[player]->inventoryUI.appraisal.current_item = 0;
		players[player]->inventoryUI.appraisal.timer = 0;
	}

	bool clientConsumedEquippedItem = false;
	if ( player >= 0 && !players[player]->isLocalPlayer() && multiplayer == SERVER )
	{
		Item** slot = nullptr;
		if ( (slot = itemSlot(stats[player], item)) != nullptr )
		{
			(*slot)->count--; // if client had consumed item equipped, this'll update the count.
			if ( item == (*slot) )
			{
				clientConsumedEquippedItem = true;
			}
		}
	}

	if ( !clientConsumedEquippedItem )
	{
		item->count--;
	}
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

	if ( player >= 0 )
	{
		players[player]->paperDoll.updateSlots();
	}
}

/*-------------------------------------------------------------------------------

	equipItem

	Handles the client impulse to equip an item

-------------------------------------------------------------------------------*/

EquipItemResult equipItem(Item* const item, Item** const slot, const int player, bool checkInventorySpaceForPaperDoll)
{
	int oldcount;

	if ( players[player]->isLocalPlayer() && players[player]->hud.pickaxeGimpTimer > 0 && !intro )
	{
		return EQUIP_ITEM_FAIL_CANT_UNEQUIP;
	}

	if (!item)   // needs "|| !slot " ?
	{
		return EQUIP_ITEM_FAIL_CANT_UNEQUIP;
	}

	if ( players[player]->isLocalPlayer() && multiplayer != SINGLE
		&& item->unableToEquipDueToSwapWeaponTimer(player) )
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
					if ( players[player]->isLocalPlayer() )
					{
						if ( shouldInvertEquipmentBeatitude(stats[player]) && (*slot)->beatitude > 0 )
						{
							messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(3217), (*slot)->getName());
						}
						else
						{
							messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(1089), (*slot)->getName());
						}
						playSoundPlayer(player, 90, 64);
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
						playSoundEntity(players[player]->entity, 33 + local_rng.rand() % 2, 64);
					}
					else if ( item->type == BOOMERANG )
					{
					}
					else if ( itemCategory(item) == WEAPON || itemCategory(item) == THROWN )
					{
						playSoundEntity(players[player]->entity, 40 + local_rng.rand() % 4, 64);
					}
					else if ( itemCategory(item) == ARMOR 
						|| item->type == TOOL_TINKERING_KIT 
						|| itemTypeIsQuiver(item->type) )
					{
						playSoundEntity(players[player]->entity, 44 + local_rng.rand() % 3, 64);
					}
					else if ( item->type == TOOL_TORCH || item->type == TOOL_LANTERN || item->type == TOOL_CRYSTALSHARD )
					{
						playSoundEntity(players[player]->entity, 134, 64);
					}
				}
			}
		}
		if ( multiplayer == SERVER && !players[player]->isLocalPlayer() )
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
				messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(1090), item->description());
			}
			item->count = oldcount;
		}

		if ( players[player]->isLocalPlayer() && players[player]->paperDoll.enabled && item )
		{
			// item is going into paperdoll.
			item->x = Player::PaperDoll_t::ITEM_PAPERDOLL_COORDINATE;
			item->y = Player::PaperDoll_t::ITEM_PAPERDOLL_COORDINATE;
		}

		*slot = item;

		if ( players[player]->isLocalPlayer() )
		{
			if ( slot == &stats[player]->weapon )
			{
				players[player]->hud.weaponSwitch = true;
			}
			else if ( slot == &stats[player]->shield )
			{
				players[player]->hud.shieldSwitch = true;
			}
		}

		if ( players[player]->isLocalPlayer() )
		{
			players[player]->paperDoll.updateSlots();
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
					if ( players[player]->isLocalPlayer() )
					{
						if ( shouldInvertEquipmentBeatitude(stats[player]) && (*slot)->beatitude > 0 )
						{
							messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(3217), (*slot)->getName());
						}
						else
						{
							messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(1089), (*slot)->getName());
						}
						playSoundPlayer(player, 90, 64);
					}
					(*slot)->identified = true;
					return EQUIP_ITEM_FAIL_CANT_UNEQUIP;
				}

				if ( players[player]->isLocalPlayer()
					&& players[player]->paperDoll.enabled
					&& players[player]->paperDoll.isItemOnDoll(**slot) )
				{
					if ( checkInventorySpaceForPaperDoll && !players[player]->inventoryUI.bItemInventoryHasFreeSlot() )
					{
						// no backpack space
						messagePlayer(player, MESSAGE_INVENTORY, Language::get(3997), item->getName());
						if ( players[player]->isLocalPlayer() )
						{
							playSoundPlayer(player, 90, 64);
						}
						return EQUIP_ITEM_FAIL_CANT_UNEQUIP;
					}
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
						playSoundEntity(players[player]->entity, 44 + local_rng.rand() % 3, 64);
					}
				}
			}
		}
		if ( !players[player]->isLocalPlayer() && multiplayer == SERVER )
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
				messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(1091), item->description());
			}
			item->count = oldcount;
		}

		*slot = nullptr;

		players[player]->paperDoll.updateSlots();
		return EQUIP_ITEM_SUCCESS_UNEQUIP;
	}
}

/*-------------------------------------------------------------------------------

	useItem

	Handles the client impulse to use an item

-------------------------------------------------------------------------------*/

void useItem(Item* item, const int player, Entity* usedBy, bool unequipForDropping)
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


	if ( item->status == BROKEN && player >= 0 && players[player]->isLocalPlayer() )
	{
		messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(1092), item->getName());
		playSoundPlayer(player, 90, 64);
		return;
	}
	if ( item->type == FOOD_CREAMPIE && player >= 0 && players[player]->isLocalPlayer() && itemIsEquipped(item, player) )
	{
		messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(3874)); // can't eat while equipped.
		playSoundPlayer(player, 90, 64);
		return;
	}

	// tins need a tin opener to open...
	if ( player >= 0 && players[player]->isLocalPlayer() && !(stats[player]->type == GOATMAN || stats[player]->type == AUTOMATON) )
	{
		if ( item->type == FOOD_TIN )
		{
			bool havetinopener = false;
			for ( node_t* node = stats[player]->inventory.first; node != nullptr; node = node->next )
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
				messagePlayer(player, MESSAGE_HINT, Language::get(1093));
				playSoundPlayer(player, 90, 64);
				return;
			}
		}
	}

	EquipItemResult equipItemResult = EquipItemResult::EQUIP_ITEM_SUCCESS_UNEQUIP;

	bool checkInventorySpaceForPaperDoll = players[player]->paperDoll.isItemOnDoll(*item);
	if ( unequipForDropping )
	{
		checkInventorySpaceForPaperDoll = false;
	}
	struct ItemDetailsForServer
	{
		ItemType type = WOODEN_SHIELD;
		Status status = EXCELLENT;
		Sint16 beatitude = 0;
		Sint16 count = 1;
		Uint32 appearance = 0;
		bool identified = false;
		bool sendToServer = false;
		void setItem(Item& item)
		{
			type = item.type;
			status = item.status;
			beatitude = item.beatitude;
			count = item.count;
			appearance = item.appearance;
			identified = item.identified;
			sendToServer = true;
		}
		void send()
		{
			if ( multiplayer != CLIENT ) { return; }
			strcpy((char*)net_packet->data, "USEI");
			SDLNet_Write32(static_cast<Uint32>(type), &net_packet->data[4]);
			SDLNet_Write32(static_cast<Uint32>(status), &net_packet->data[8]);
			SDLNet_Write32(static_cast<Uint32>(beatitude), &net_packet->data[12]);
			SDLNet_Write32(static_cast<Uint32>(count), &net_packet->data[16]);
			SDLNet_Write32(static_cast<Uint32>(appearance), &net_packet->data[20]);
			net_packet->data[24] = identified;
			net_packet->data[25] = clientnum;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 26;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
	};
	ItemDetailsForServer itemDetailsForServer;

	if ( multiplayer == CLIENT && !intro )
	{
		if ( item->unableToEquipDueToSwapWeaponTimer(player) && itemCategory(item) != POTION )
		{
			// don't send to host as we're not allowed to "use" or equip these items. 
			// will return false in equipItem.
			// potions allowed here because we're drinking em.
		}
		else
		{
			itemDetailsForServer.setItem(*item);
		}
	}

	bool drankPotion = false;
	bool tryLearnPotionRecipe = false;
	bool tryLevelAppraiseFromPotion = false;
	const bool tryEmptyBottle = (item->status >= SERVICABLE);
	const ItemType potionType = item->type;
	if ( player >= 0 && players[player]->isLocalPlayer() )
	{
		if ( itemCategory(item) == POTION && item->type != POTION_EMPTY && usedBy
			&& (players[player] && players[player]->entity)
			&& players[player]->entity == usedBy )
		{
			if ( item->identified )
			{
				tryLearnPotionRecipe = true;
			}
			else
			{
				tryLevelAppraiseFromPotion = true;
			}
		}
	}

	switch ( item->type )
	{
		case WOODEN_SHIELD:
			equipItemResult = equipItem(item, &stats[player]->shield, player, checkInventorySpaceForPaperDoll);
			break;
		case QUARTERSTAFF:
		case BRONZE_SWORD:
		case BRONZE_MACE:
		case BRONZE_AXE:
			equipItemResult = equipItem(item, &stats[player]->weapon, player, checkInventorySpaceForPaperDoll);
			break;
		case BRONZE_SHIELD:
			equipItemResult = equipItem(item, &stats[player]->shield, player, checkInventorySpaceForPaperDoll);
			break;
		case SLING:
		case IRON_SPEAR:
		case IRON_SWORD:
		case IRON_MACE:
		case IRON_AXE:
			equipItemResult = equipItem(item, &stats[player]->weapon, player, checkInventorySpaceForPaperDoll);
			break;
		case IRON_SHIELD:
			equipItemResult = equipItem(item, &stats[player]->shield, player, checkInventorySpaceForPaperDoll);
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
			equipItemResult = equipItem(item, &stats[player]->weapon, player, checkInventorySpaceForPaperDoll);
			break;
		case STEEL_SHIELD:
		case STEEL_SHIELD_RESISTANCE:
		case MIRROR_SHIELD:
		case CRYSTAL_SHIELD:
			equipItemResult = equipItem(item, &stats[player]->shield, player, checkInventorySpaceForPaperDoll);
			break;
		case CROSSBOW:
		case LONGBOW:
		case COMPOUND_BOW:
		case HEAVY_CROSSBOW:
			equipItemResult = equipItem(item, &stats[player]->weapon, player, checkInventorySpaceForPaperDoll);
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
			equipItemResult = equipItem(item, &stats[player]->gloves, player, checkInventorySpaceForPaperDoll);
			break;
		case CLOAK:
		case CLOAK_BLACK:
		case CLOAK_MAGICREFLECTION:
		case CLOAK_INVISIBILITY:
		case CLOAK_PROTECTION:
		case ARTIFACT_CLOAK:
		case CLOAK_BACKPACK:
		case CLOAK_SILVER:
			equipItemResult = equipItem(item, &stats[player]->cloak, player, checkInventorySpaceForPaperDoll);
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
			equipItemResult = equipItem(item, &stats[player]->shoes, player, checkInventorySpaceForPaperDoll);
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
			equipItemResult = equipItem(item, &stats[player]->breastplate, player, checkInventorySpaceForPaperDoll);
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
			equipItemResult = equipItem(item, &stats[player]->helmet, player, checkInventorySpaceForPaperDoll);
			break;
		case AMULET_SEXCHANGE:
			item_AmuletSexChange(item, player);
			break;
		case AMULET_LIFESAVING:
		case AMULET_WATERBREATHING:
		case AMULET_MAGICREFLECTION:
			equipItemResult = equipItem(item, &stats[player]->amulet, player, checkInventorySpaceForPaperDoll);
			break;
		case AMULET_STRANGULATION:
		{
			bool oldStrangulation = stats[player]->amulet && stats[player]->amulet->type == AMULET_STRANGULATION;
			equipItemResult = equipItem(item, &stats[player]->amulet, player, checkInventorySpaceForPaperDoll);
			if ( stats[player]->amulet && stats[player]->amulet->type == AMULET_STRANGULATION
				&& !oldStrangulation )
			{
				if ( players[player]->isLocalPlayer() )
				{
					messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(1095));
				}
			}
			if ( item->beatitude >= 0 )
			{
				item->beatitude = -1;
			}
		}
			break;
		case AMULET_POISONRESISTANCE:
			equipItemResult = equipItem(item, &stats[player]->amulet, player, checkInventorySpaceForPaperDoll);
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
			messagePlayer(player, MESSAGE_HINT, Language::get(2359));
			if ( players[player]->isLocalPlayer() )
			{
				playSoundPlayer(player, 90, 64);
			}
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
				//consumeItem(item, player);
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
					const Uint32 color = makeColorRGB(255, 128, 0);
					messagePlayerColor(player, MESSAGE_STATUS, color, Language::get(3699)); // superheats
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
			equipItemResult = equipItem(item, &stats[player]->weapon, player, checkInventorySpaceForPaperDoll);
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
			equipItemResult = equipItem(item, &stats[player]->ring, player, checkInventorySpaceForPaperDoll);
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
			equipItemResult = equipItem(item, &stats[player]->weapon, player, checkInventorySpaceForPaperDoll);
			break;
		case TOOL_PICKAXE:
		case TOOL_WHIP:
			equipItemResult = equipItem(item, &stats[player]->weapon, player, checkInventorySpaceForPaperDoll);
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
			equipItemResult = equipItem(item, &stats[player]->weapon, player, checkInventorySpaceForPaperDoll);
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
			equipItemResult = equipItem(item, &stats[player]->shield, player, checkInventorySpaceForPaperDoll);
			break;
		case TOOL_BLINDFOLD:
		case TOOL_BLINDFOLD_FOCUS:
		case TOOL_BLINDFOLD_TELEPATHY:
			equipItemResult = equipItem(item, &stats[player]->mask, player, checkInventorySpaceForPaperDoll);
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
		case MONOCLE:
		case MASK_SHAMAN:
			equipItemResult = equipItem(item, &stats[player]->mask, player, checkInventorySpaceForPaperDoll);
			break;
		case TOOL_BEARTRAP:
			equipItemResult = equipItem(item, &stats[player]->weapon, player, checkInventorySpaceForPaperDoll);
			break;
		case TOOL_ALEMBIC:
			if ( !players[player]->isLocalPlayer() )
			{
				consumeItem(item, player);
			}
			else
			{
				if ( GenericGUI[player].alchemyGUI.bOpen && GenericGUI[player].alembicItem == item )
				{
					GenericGUI[player].closeGUI();
				}
				else
				{
					GenericGUI[player].openGUI(GUI_TYPE_ALCHEMY, true, item);
				}
			}
			break;
		case ENCHANTED_FEATHER:
			if ( !players[player]->isLocalPlayer() )
			{
				consumeItem(item, player);
			}
			else
			{
				if ( GenericGUI[player].featherGUI.bOpen && GenericGUI[player].scribingToolItem == item )
				{
					GenericGUI[player].closeGUI();
				}
				else
				{
					GenericGUI[player].openGUI(GUI_TYPE_SCRIBING, item);
				}
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
			if ( players[player]->isLocalPlayer() )
			{
				if ( item->type == TOOL_METAL_SCRAP )
				{
					messagePlayer(player, MESSAGE_HINT, Language::get(3705));
				}
				else
				{
					messagePlayer(player, MESSAGE_HINT, Language::get(3706));
				}
			}
			break;
		case READABLE_BOOK:
			if (numbooks && players[player]->isLocalPlayer() )
			{
				if (players[player] && players[player]->entity)
				{
					if (!players[player]->entity->isBlind())
					{
						players[player]->bookGUI.openBook(item->appearance % numbooks, item);
						conductIlliterate = false;
					}
					else
					{
						messagePlayer(player, MESSAGE_HINT | MESSAGE_STATUS, Language::get(970));
						playSoundPlayer(player, 90, 64);
					}
				}
			}
			break;
		case SPELL_ITEM:
		{
			spell_t* spell = getSpellFromItem(player, item);
			if (spell)
			{
				equipSpell(spell, player, item);
			}
			break;
		}
		case ARTIFACT_SWORD:
			equipItemResult = equipItem(item, &stats[player]->weapon, player, checkInventorySpaceForPaperDoll);
			break;
		case ARTIFACT_MACE:
			if ( players[player]->isLocalPlayer() )
			{
				messagePlayer(player, MESSAGE_WORLD, Language::get(1096));
			}
			equipItemResult = equipItem(item, &stats[player]->weapon, player, checkInventorySpaceForPaperDoll);
			break;
		case ARTIFACT_SPEAR:
		case ARTIFACT_AXE:
		case ARTIFACT_BOW:
			equipItemResult = equipItem(item, &stats[player]->weapon, player, checkInventorySpaceForPaperDoll);
			break;
		case ARTIFACT_ORB_BLUE:
		case ARTIFACT_ORB_RED:
		case ARTIFACT_ORB_PURPLE:
		case ARTIFACT_ORB_GREEN:
			equipItemResult = equipItem(item, &stats[player]->weapon, player, checkInventorySpaceForPaperDoll);
			break;
		case TOOL_PLAYER_LOOT_BAG:
			if ( multiplayer != CLIENT )
			{
				int lootbagPlayer = item->getLootBagPlayer();
				
				if ( lootbagPlayer >= 0 && lootbagPlayer < MAXPLAYERS
					&& stats[lootbagPlayer] )
				{
					std::string name = stats[lootbagPlayer]->name;
					if ( lootbagPlayer == player )
					{
						messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_HINT | MESSAGE_EQUIPMENT,
							Language::get(4331), item->getLootBagNumItems());
					}
					else if ( name == "" || client_disconnected[lootbagPlayer] )
					{
						messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_HINT | MESSAGE_EQUIPMENT,
							Language::get(4330), item->getLootBagNumItems());
					}
					else
					{
						messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_HINT | MESSAGE_EQUIPMENT,
							Language::get(4329), item->getLootBagNumItems(), 
							name.c_str());
					}
				}
				else
				{
					messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_HINT | MESSAGE_EQUIPMENT,
						Language::get(4330), item->getLootBagNumItems());
				}

				if ( !players[player]->isLocalPlayer() )
				{
					consumeItem(item, player);
				}
			}
			break;
		default:
			printlog("error: item %d used, but it has no use case!\n", static_cast<int>(item->type));
			break;
	}

	if ( players[player]->isLocalPlayer() )
	{
		if ( checkInventorySpaceForPaperDoll && equipItemResult == EquipItemResult::EQUIP_ITEM_FAIL_CANT_UNEQUIP )
		{
			itemDetailsForServer.sendToServer = false;
		}

		if ( itemDetailsForServer.sendToServer )
		{
			itemDetailsForServer.send();
		}
		if ( drankPotion && usedBy
			&& (players[player] && players[player]->entity)
			&& players[player]->entity == usedBy )
		{
			if ( tryLearnPotionRecipe )
			{
				GenericGUI[player].alchemyLearnRecipe(potionType, true);
			}
			if ( tryLevelAppraiseFromPotion )
			{
				if ( stats[player]->PROFICIENCIES[PRO_APPRAISAL] < SKILL_LEVEL_BASIC )
				{
					if ( stats[player] && players[player]->entity )
					{
						if ( local_rng.rand() % 4 == 0 )
						{
							if ( multiplayer == CLIENT )
							{
								// request level up
								strcpy((char*)net_packet->data, "CSKL");
								net_packet->data[4] = player;
								net_packet->data[5] = PRO_APPRAISAL;
								net_packet->address.host = net_server.host;
								net_packet->address.port = net_server.port;
								net_packet->len = 6;
								sendPacketSafe(net_sock, -1, net_packet, 0);
							}
							else
							{
								players[player]->entity->increaseSkill(PRO_APPRAISAL);
							}
						}
					}
				}
			}
			const int skillLVL = stats[player]->PROFICIENCIES[PRO_ALCHEMY] / 20;
			if ( tryEmptyBottle && local_rng.rand() % 100 < std::min(80, (60 + skillLVL * 10)) ) // 60 - 80% chance
			{
				Item* emptyBottle = newItem(POTION_EMPTY, SERVICABLE, 0, 1, 0, true, nullptr);
				itemPickup(player, emptyBottle);
				messagePlayer(player, MESSAGE_INTERACTION, Language::get(3351), items[POTION_EMPTY].getIdentifiedName());
				free(emptyBottle);
			}
		}
	}

	if ( !item )
	{
		return;
	}

	// on-equip messages.
	if ( multiplayer != CLIENT && equipItemResult == EquipItemResult::EQUIP_ITEM_SUCCESS_NEWITEM && itemIsEquipped(item, player) )
	{
		switch ( item->type )
		{
			case ARTIFACT_BREASTPIECE:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2972));
				break;
			case ARTIFACT_HELM:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2973));
				break;
			case ARTIFACT_BOOTS:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2974));
				break;
			case ARTIFACT_CLOAK:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2975));
				break;
			case ARTIFACT_GLOVES:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2976));
				break;
			case AMULET_LIFESAVING:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2478));
				break;
			case AMULET_WATERBREATHING:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2479));
				break;
			case AMULET_MAGICREFLECTION:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2480));
				break;
			case HAT_WIZARD:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2481));
				break;
			case SPIKED_GAUNTLETS:
			case BRASS_KNUCKLES:
			case IRON_KNUCKLES:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2482));
				break;
			case HAT_JESTER:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2483));
				break;
			case IRON_BOOTS_WATERWALKING:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2484));
				break;
			case LEATHER_BOOTS_SPEED:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2485));
				break;
			case CLOAK_INVISIBILITY:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2486));
				break;
			case CLOAK_PROTECTION:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2487));
				break;
			case CLOAK_MAGICREFLECTION:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2488));
				break;
			case GLOVES_DEXTERITY:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2489));
				break;
			case BRACERS_CONSTITUTION:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2490));
				break;
			case GAUNTLETS_STRENGTH:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2491));
				break;
			case AMULET_POISONRESISTANCE:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2492));
				break;
			case RING_ADORNMENT:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2384));
				break;
			case RING_SLOWDIGESTION:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2385));
				break;
			case RING_PROTECTION:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2386));
				break;
			case RING_WARNING:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2387));
				break;
			case RING_STRENGTH:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2388));
				break;
			case RING_CONSTITUTION:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2389));
				break;
			case RING_INVISIBILITY:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2412));
				break;
			case RING_MAGICRESISTANCE:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2413));
				break;
			case RING_CONFLICT:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2414));
				break;
			case RING_LEVITATION:
				if ( !MFLAG_DISABLELEVITATION )
				{
					// can levitate
					messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2415));
				}
				else
				{
					messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2381));
				}
				break;
			case RING_REGENERATION:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2416));
				break;
			case RING_TELEPORTATION:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2417));
				break;
			case STEEL_BOOTS_FEATHER:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2418));
				break;
			case STEEL_BOOTS_LEVITATION:
				if ( !MFLAG_DISABLELEVITATION )
				{
					// can levitate
					messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2419));
				}
				else
				{
					messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2381));
				}
				break;
			case VAMPIRE_DOUBLET:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2597));
				break;
			case TOOL_BLINDFOLD:
				break;
			case TOOL_BLINDFOLD_TELEPATHY:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(2908));
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

Item* itemPickup(const int player, Item* const item, Item* addToSpecificInventoryItem, bool forceNewStack)
{
	if (!item)
	{
		return nullptr;
	}
	Item* item2;

	if ( stats[player]->PROFICIENCIES[PRO_APPRAISAL] >= CAPSTONE_UNLOCK_LEVEL[PRO_APPRAISAL] )
	{
		if ( !(player != 0 && multiplayer == SERVER && !players[player]->isLocalPlayer()) )
		{
			if ( !item->identified )
			{
				item->identified = true;
				item->notifyIcon = true;
				if ( item->type == GEM_GLASS )
				{
					steamStatisticUpdate(STEAM_STAT_RHINESTONE_COWBOY, STEAM_STAT_INT, 1);
				}
			}
		}
	}

	if ( item->identified && !intro )
	{
		item->notifyIcon = true;
	}

	if ( multiplayer != CLIENT && player >= 0 && players[player] && players[player]->entity )
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

	if ( player != 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
	{
		// send the client info on the item it just picked up
		strcpy((char*)net_packet->data, "ITEM");
		SDLNet_Write32(static_cast<Uint32>(item->type), &net_packet->data[4]);
		SDLNet_Write32(static_cast<Uint32>(item->status), &net_packet->data[8]);
		SDLNet_Write32(static_cast<Uint32>(item->beatitude), &net_packet->data[12]);
		SDLNet_Write32(static_cast<Uint32>(item->count), &net_packet->data[16]);
		SDLNet_Write32(static_cast<Uint32>(item->appearance), &net_packet->data[20]);
		SDLNet_Write32(static_cast<Uint32>(item->ownerUid), &net_packet->data[24]);
		net_packet->data[28] = item->identified ? 1 : 0;
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 29;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
	}
	else
	{
		std::unordered_set<Uint32> appearancesOfSimilarItems;
		bool doSpecificItemCheck = (addToSpecificInventoryItem != nullptr);
		bool hasRunSpecificItemCheck = false;
		for ( node_t* node = stats[player]->inventory.first; node != nullptr; node = node->next )
		{
			if ( doSpecificItemCheck )
			{
				if ( !hasRunSpecificItemCheck )
				{
					item2 = addToSpecificInventoryItem;
					hasRunSpecificItemCheck = true;
				}
				else
				{
					node = stats[player]->inventory.first;
					item2 = static_cast<Item*>(node->element);
					doSpecificItemCheck = false;
				}
			}
			else
			{
				item2 = static_cast<Item*>(node->element);
			}

			if ( forceNewStack )
			{
				if ( !itemCompare(item, item2, true) )
				{
					// items are the same (incl. appearance!)
					// if they shouldn't stack, we need to change appearance of the new item.
					appearancesOfSimilarItems.insert(item2->appearance);
				}
				continue;
			}

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

					if ( item2->count >= maxStack - 1 )
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

					if ( multiplayer == CLIENT && player >= 0 && players[player]->isLocalPlayer() && itemIsEquipped(item2, player) )
					{
						// if incrementing qty and holding item, then send "equip" for server to update their count of your held item.
						strcpy((char*)net_packet->data, "EQUS");
						SDLNet_Write32(static_cast<Uint32>(item2->type), &net_packet->data[4]);
						SDLNet_Write32(static_cast<Uint32>(item2->status), &net_packet->data[8]);
						SDLNet_Write32(static_cast<Uint32>(item2->beatitude), &net_packet->data[12]);
						SDLNet_Write32(static_cast<Uint32>(item2->count), &net_packet->data[16]);
						SDLNet_Write32(static_cast<Uint32>(item2->appearance), &net_packet->data[20]);
						net_packet->data[24] = item2->identified;
						net_packet->data[25] = player;
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
					if ( multiplayer == CLIENT && player >= 0 && players[player]->isLocalPlayer() && itemIsEquipped(item2, player) )
					{
						// if incrementing qty and holding item, then send "equip" for server to update their count of your held item.
						Item** slot = itemSlot(stats[player], item2);
						if ( slot )
						{
							if ( slot == &stats[player]->weapon )
							{
								clientSendEquipUpdateToServer(EQUIP_ITEM_SLOT_WEAPON, EQUIP_ITEM_SUCCESS_UPDATE_QTY, player,
									item2->type, item2->status, item2->beatitude, item2->count, item2->appearance, item2->identified);
							}
							else if ( slot == &stats[player]->shield )
							{
								clientSendEquipUpdateToServer(EQUIP_ITEM_SLOT_SHIELD, EQUIP_ITEM_SUCCESS_UPDATE_QTY, player,
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
		if ( !appearancesOfSimilarItems.empty() && item && item->type >= 0 && item->type < NUMITEMS )
		{
			Uint32 originalAppearance = item->appearance;
			int originalVariation = originalAppearance % items[item->type].variations;

			int tries = 100;
			bool robot = false;
			// we need to find a unique appearance within the list.
			if ( item->type == TOOL_SENTRYBOT || item->type == TOOL_SPELLBOT || item->type == TOOL_GYROBOT
				|| item->type == TOOL_DUMMYBOT )
			{
				robot = true;
				item->appearance += (local_rng.rand() % 100000) * 10;
			}
			else
			{
				item->appearance = local_rng.rand();
				if ( item->appearance % items[item->type].variations != originalVariation )
				{
					// we need to match the variation for the new appearance, take the difference so new varation matches
					int change = (item->appearance % items[item->type].variations - originalVariation);
					if ( item->appearance < change ) // underflow protection
					{
						item->appearance += items[item->type].variations;
					}
					item->appearance -= change;
					int newVariation = item->appearance % items[item->type].variations;
					assert(newVariation == originalVariation);
				}
			}
			auto it = appearancesOfSimilarItems.find(item->appearance);
			while ( it != appearancesOfSimilarItems.end() && tries > 0 )
			{
				if ( robot )
				{
					item->appearance += (local_rng.rand() % 100000) * 10;
				}
				else
				{
					item->appearance = local_rng.rand();
					if ( item->appearance % items[item->type].variations != originalVariation )
					{
						// we need to match the variation for the new appearance, take the difference so new varation matches
						int change = (item->appearance % items[item->type].variations - originalVariation);
						if ( item->appearance < change ) // underflow protection
						{
							item->appearance += items[item->type].variations;
						}
						item->appearance -= change;
						int newVariation = item->appearance % items[item->type].variations;
						assert(newVariation == originalVariation);
					}
				}
				it = appearancesOfSimilarItems.find(item->appearance);
				--tries;
			}
		}

		item2 = newItem(item->type, item->status, item->beatitude, item->count, item->appearance, item->identified, &stats[player]->inventory);
		item2->ownerUid = item->ownerUid;
		item2->notifyIcon = item->notifyIcon;
		return item2;
	}

	return item;
}

int Item::getMaxStackLimit(int player) const
{
	if ( !shouldItemStack(player, true) )
	{
		return 1;
	}

	int maxStack = 100;
	if ( itemCategory(this) == THROWN || itemCategory(this) == GEM )
	{
		maxStack = THROWN_GEM_MAX_STACK_QTY;
	}
	else if ( itemTypeIsQuiver(this->type) )
	{
		maxStack = QUIVER_MAX_AMMO_QTY - 1;
	}
	else if ( type == TOOL_METAL_SCRAP || type == TOOL_MAGIC_SCRAP )
	{
		maxStack = SCRAP_MAX_STACK_QTY - 1;
	}

	return maxStack;
}

ItemStackResult getItemStackingBehaviorIndividualItemCheck(const int player, Item* itemToCheck, Item* itemDestinationStack, int& newQtyForCheckedItem, int& newQtyForDestItem)
{
	ItemStackResult itemStackResult;
	itemStackResult.itemToStackInto = nullptr;
	if ( !itemToCheck || !itemDestinationStack )
	{
		itemStackResult.resultType = ITEM_STACKING_ERROR;
		return itemStackResult;
	}

	if ( !itemCompare(itemToCheck, itemDestinationStack, false) )
	{
		if ( (itemTypeIsQuiver(itemDestinationStack->type) && (itemToCheck->count + itemDestinationStack->count) >= QUIVER_MAX_AMMO_QTY)
			|| ((itemDestinationStack->type == TOOL_MAGIC_SCRAP || itemDestinationStack->type == TOOL_METAL_SCRAP)
				&& (itemToCheck->count + itemDestinationStack->count) >= SCRAP_MAX_STACK_QTY) )
		{
			int maxStack = QUIVER_MAX_AMMO_QTY;
			if ( itemDestinationStack->type == TOOL_MAGIC_SCRAP || itemDestinationStack->type == TOOL_METAL_SCRAP )
			{
				maxStack = SCRAP_MAX_STACK_QTY;
			}

			if ( itemDestinationStack->count >= maxStack - 1 )
			{
				// can't add anymore to this stack, let's skip over this.
				newQtyForDestItem = itemDestinationStack->count;
				newQtyForCheckedItem = itemToCheck->count;
				itemStackResult.resultType = ITEM_DESTINATION_STACK_IS_FULL;
				itemStackResult.itemToStackInto = itemDestinationStack;
				return itemStackResult;
			}

			// too many arrows, split off into a new stack with reduced qty.
			const int total = itemToCheck->count + itemDestinationStack->count;
			const int destinationStackQty = maxStack - 1;
			newQtyForDestItem = destinationStackQty;
			newQtyForCheckedItem = total - newQtyForDestItem;

			if ( newQtyForCheckedItem <= 0 )
			{
				itemStackResult.resultType = ITEM_ADDED_ENTIRELY_TO_DESTINATION_STACK;
				itemStackResult.itemToStackInto = itemDestinationStack;
				return itemStackResult;
			}
			else
			{
				itemStackResult.resultType = ITEM_ADDED_PARTIALLY_TO_DESTINATION_STACK;
				itemStackResult.itemToStackInto = itemDestinationStack;
				return itemStackResult;
			}
		}
		// if items are the same, check to see if they should stack
		else if ( itemDestinationStack->shouldItemStack(player) )
		{
			int maxStack = itemDestinationStack->getMaxStackLimit(player);

			const int total = itemToCheck->count + itemDestinationStack->count;
			if ( total > maxStack )
			{
				newQtyForDestItem = maxStack;
				newQtyForCheckedItem = total - newQtyForDestItem;
				if ( newQtyForCheckedItem <= 0 )
				{
					itemStackResult.resultType = ITEM_ADDED_ENTIRELY_TO_DESTINATION_STACK;
					itemStackResult.itemToStackInto = itemDestinationStack;
					return itemStackResult;
				}
				else
				{
					itemStackResult.resultType = ITEM_ADDED_PARTIALLY_TO_DESTINATION_STACK;
					itemStackResult.itemToStackInto = itemDestinationStack;
					return itemStackResult;
				}
			}
			else
			{
				newQtyForCheckedItem = 0;
				newQtyForDestItem = total;
				itemStackResult.resultType = ITEM_ADDED_ENTIRELY_TO_DESTINATION_STACK;
				itemStackResult.itemToStackInto = itemDestinationStack;
				return itemStackResult;
			}
		}
		else if ( !itemCompare(itemToCheck, itemDestinationStack, true) )
		{
			newQtyForCheckedItem = itemToCheck->count;
			newQtyForDestItem = itemDestinationStack->count;
			itemStackResult.resultType = ITEM_DESTINATION_STACK_IS_FULL;
			return itemStackResult;
		}
	}
	newQtyForCheckedItem = itemToCheck->count;
	newQtyForDestItem = itemDestinationStack->count;
	itemStackResult.resultType = ITEM_DESTINATION_NOT_SAME_ITEM;
	return itemStackResult;
}

void getItemEmptySlotStackingBehavior(const int player, Item& itemToCheck, int& newQtyForCheckedItem, int& newQtyForDestItem)
{
	int maxStack = itemToCheck.getMaxStackLimit(player);
	if ( itemToCheck.count > maxStack )
	{
		newQtyForCheckedItem = itemToCheck.count - maxStack;
		newQtyForDestItem = maxStack;
	}
	else
	{
		newQtyForCheckedItem = 0;
		newQtyForDestItem = itemToCheck.count;
	}
}

ItemStackResult getItemStackingBehaviorIntoChest(const int player, Item* itemToCheck, Item* itemDestinationStack, int& newQtyForCheckedItem, int& newQtyForDestItem)
{
	ItemStackResult itemStackResult;
	itemStackResult.itemToStackInto = nullptr;
	if ( !itemToCheck )
	{
		itemStackResult.resultType = ITEM_STACKING_ERROR;
		return itemStackResult;
	}

	list_t* chest_inventory = nullptr;
	if ( multiplayer == CLIENT )
	{
		chest_inventory = &chestInv[player];
	}
	else if ( openedChest[player]->children.first && openedChest[player]->children.first->element )
	{
		chest_inventory = (list_t*)openedChest[player]->children.first->element;
	}
	if ( !chest_inventory )
	{
		// no chest inventory available
		itemStackResult.resultType = ITEM_STACKING_ERROR;
		return itemStackResult;
	}

	if ( itemDestinationStack )
	{
		return getItemStackingBehaviorIndividualItemCheck(player, itemToCheck, itemDestinationStack, newQtyForCheckedItem, newQtyForDestItem);
	}

	itemStackResult.resultType = ITEM_ADDED_WITHOUT_NEEDING_STACK;
	newQtyForCheckedItem = itemToCheck->count;
	newQtyForDestItem = 0;

	for ( node_t* node = chest_inventory->first; node != nullptr; node = node->next )
	{
		Item* item2 = static_cast<Item*>(node->element);
		if ( item2 )
		{
			int tmpQtyCheckedItem = newQtyForCheckedItem;
			int tmpQtyDestItem = newQtyForDestItem;
			auto res = getItemStackingBehaviorIndividualItemCheck(player, itemToCheck, item2, tmpQtyCheckedItem, tmpQtyDestItem);
			bool skipResult = false;
			switch ( res.resultType )
			{
				case ITEM_DESTINATION_NOT_SAME_ITEM:
				case ITEM_STACKING_ERROR:
				case ITEM_DESTINATION_STACK_IS_FULL:
					skipResult = true;
					break;
				default:
					break;
			}
			if ( skipResult )
			{
				continue;
			}

			// found a stack to add this item to
			newQtyForCheckedItem = tmpQtyCheckedItem;
			newQtyForDestItem = tmpQtyDestItem;
			res.itemToStackInto = item2;
			return res;
		}
	}

	//std::vector<std::pair<int, Item*>> chestSlotOrder;
	//for ( node_t* node = chest_inventory->first; node != nullptr; node = node->next )
	//{
	//	Item* item2 = static_cast<Item*>(node->element);
	//	if ( item2 )
	//	{
	//		int key = item2->x + item2->y * 100;
	//		chestSlotOrder.push_back(std::make_pair(key, item2));
	//	}
	//}
	//std::sort(chestSlotOrder.begin(), chestSlotOrder.end()); // sort ascending by position, left to right, then down
	//for ( auto& keyValue : chestSlotOrder )
	//{
	//	Item* item2 = keyValue.second;
	//	if ( item2 )
	//	{
	//		int tmpQtyCheckedItem = newQtyForCheckedItem;
	//		int tmpQtyDestItem = newQtyForDestItem;
	//		auto res = getItemStackingBehaviorIndividualItemCheck(player, itemToCheck, item2, tmpQtyCheckedItem, tmpQtyDestItem);
	//		bool skipResult = false;
	//		switch ( res )
	//		{
	//			case ITEM_DESTINATION_NOT_SAME_ITEM:
	//			case ITEM_STACKING_ERROR:
	//			case ITEM_DESTINATION_STACK_IS_FULL:
	//				skipResult = true;
	//				break;
	//			default:
	//				break;
	//		}
	//		if ( skipResult )
	//		{
	//			continue;
	//		}

	//		// found a stack to add this item to
	//		result = res;
	//		newQtyForCheckedItem = tmpQtyCheckedItem;
	//		newQtyForDestItem = tmpQtyDestItem;
	//		return result;
	//	}
	//}

	itemStackResult.resultType = ITEM_ADDED_WITHOUT_NEEDING_STACK;
	int maxStack = itemToCheck->getMaxStackLimit(player);
	if ( itemToCheck->count > maxStack )
	{
		newQtyForCheckedItem = itemToCheck->count - maxStack;
		newQtyForDestItem = maxStack;
	}
	else
	{
		newQtyForCheckedItem = 0;
		newQtyForDestItem = itemToCheck->count;
	}
	return itemStackResult;
}

ItemStackResult getItemStackingBehavior(const int player, Item* itemToCheck, Item* itemDestinationStack, int& newQtyForCheckedItem, int& newQtyForDestItem)
{
	ItemStackResult itemStackResult;
	itemStackResult.itemToStackInto = nullptr;
	if ( !itemToCheck )
	{
		itemStackResult.resultType = ITEM_STACKING_ERROR;
		return itemStackResult;
	}

	if ( itemDestinationStack )
	{
		return getItemStackingBehaviorIndividualItemCheck(player, itemToCheck, itemDestinationStack, newQtyForCheckedItem, newQtyForDestItem);
	}

	itemStackResult.resultType = ITEM_ADDED_WITHOUT_NEEDING_STACK;
	newQtyForCheckedItem = itemToCheck->count;
	newQtyForDestItem = 0;

	for ( node_t* node = stats[player]->inventory.first; node != nullptr; node = node->next )
	{
		Item* item2 = static_cast<Item*>(node->element);
		if ( item2 )
		{
			int tmpQtyCheckedItem = newQtyForCheckedItem;
			int tmpQtyDestItem = newQtyForDestItem;
			auto res = getItemStackingBehaviorIndividualItemCheck(player, itemToCheck, item2, tmpQtyCheckedItem, tmpQtyDestItem);
			bool skipResult = false;
			switch ( res.resultType )
			{
				case ITEM_DESTINATION_NOT_SAME_ITEM:
				case ITEM_STACKING_ERROR:
				case ITEM_DESTINATION_STACK_IS_FULL:
					skipResult = true;
					break;
				default:
					break;
			}
			if ( skipResult )
			{
				continue;
			}
			
			// found a stack to add this item to
			newQtyForCheckedItem = tmpQtyCheckedItem;
			newQtyForDestItem = tmpQtyDestItem;
			res.itemToStackInto = item2;
			return res;
		}
	}

	itemStackResult.resultType = ITEM_ADDED_WITHOUT_NEEDING_STACK;
	int maxStack = itemToCheck->getMaxStackLimit(player);
	if ( itemToCheck->count > maxStack )
	{
		newQtyForCheckedItem = itemToCheck->count - maxStack;
		newQtyForDestItem = maxStack;
	}
	else
	{
		newQtyForCheckedItem = 0;
		newQtyForDestItem = itemToCheck->count;
	}
	return itemStackResult;
}

/*-------------------------------------------------------------------------------

	newItemFromEntity

	returns a pointer to an item struct from the given entity if it's an
	"item" entity, and returns NULL if the entity is anything else

-------------------------------------------------------------------------------*/

Item* newItemFromEntity(const Entity* const entity, bool discardUid)
{
	if ( entity == nullptr )
	{
		return nullptr;
	}
	Uint32 oldUids = itemuids;
	Item* item = newItem(static_cast<ItemType>(entity->skill[10]), static_cast<Status>(entity->skill[11]), entity->skill[12], entity->skill[13], entity->skill[14], entity->skill[15], nullptr);
	if ( !item )
	{
		return nullptr;
	}
	if ( discardUid && itemuids == oldUids + 1 )
	{
		--itemuids;
		item->uid = 0;
	}
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
	if ( player < 0 || !stats[player] )
	{
		return false;
	}
	if ( !item->node || item->node->list != &stats[player]->inventory )
	{
		return false;
	}
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

bool Item::doesItemProvideBeatitudeAC() const
{
	if ( itemTypeIsQuiver(type) || itemCategory(this) == SPELLBOOK || itemCategory(this) == AMULET )
	{
		return false;
	}
	return true;
}

bool Item::doesItemProvidePassiveShieldBonus() const
{
	if ( itemTypeIsQuiver(type) || itemCategory(this) == SPELLBOOK )
	{
		return false;
	}
	return true;
}

bool Item::doesPotionHarmAlliesOnThrown() const
{
	switch ( type )
	{
		case POTION_HEALING:
		case POTION_EXTRAHEALING:
		case POTION_RESTOREMAGIC:
		case POTION_CUREAILMENT:
		case POTION_WATER:
		case POTION_BOOZE:
		case POTION_JUICE:
		case POTION_STRENGTH:
		case POTION_SPEED:
		case POTION_INVISIBILITY:
		case POTION_LEVITATION:
		case POTION_POLYMORPH:
			return false;
		default:
			break;
	}
	return true;
}

Sint32 Item::potionGetEffectHealth(Entity* my, Stat* myStats) const
{
	if ( itemCategory(this) != POTION )
	{
		return 0;
	}

	int heal = 0;

	switch ( type )
	{
		case POTION_WATER:
			heal += (beatitude <= 0 ? 1 : (5 * beatitude));
			break;
		case POTION_BOOZE:
			heal += (5 * (1 + beatitude));
			break;
		case POTION_JUICE:
			heal += (5 * (1 + std::max((Sint16)0, beatitude))); // always 5 at cursed.
			break;
		case POTION_HEALING:
		{
			int amount = std::max(7 + status, 0);
			int multiplier = std::max(5, beatitude + 5);
			amount *= multiplier / 5.f;
			heal += amount;
			break;
		}
		case POTION_EXTRAHEALING:
		{
			int amount = std::max(15 + status, 0);
			int multiplier = std::max(5, beatitude + 5);
			amount *= multiplier;
			heal += amount;
			break;
		}
		case POTION_RESTOREMAGIC:
		{
			int amount = std::max(7 + status, 0);
			int multiplier = std::max(5, beatitude + 5);
			amount *= multiplier;
			heal += amount;
			break;
		}
		default:
			break;
	}

	return heal;
}
Sint32 Item::potionGetEffectDamage(Entity* my, Stat* myStats) const
{
	if ( itemCategory(this) != POTION )
	{
		return 0;
	}

	int damage = 0;
	switch ( type )
	{
		case POTION_SICKNESS:
			damage += (5 + 5 * abs(beatitude));
			break;
		case POTION_ACID:
			damage += (10 + 5 * abs(beatitude));
			break;
		case POTION_THUNDERSTORM:
		case POTION_FIRESTORM:
		case POTION_ICESTORM:
			damage += (10 + 5 * abs(beatitude));
			break;
		default:
			break;
	}

	return damage;
}

Sint32 Item::potionGetEffectDurationMinimum(Entity* my, Stat* myStats) const
{
	if ( itemCategory(this) != POTION )
	{
		return 1;
	}

	int duration = 1;

	switch ( type )
	{
		case POTION_WATER:
			break;
		case POTION_BOOZE:
			if ( myStats && myStats->type == GOATMAN )
			{
				duration = 7500; // 2.5 mins
			}
			else
			{
				duration = 2000;
			}
			break;
		case POTION_JUICE:
			break;
		case POTION_SICKNESS:
			break;
		case POTION_CONFUSION:
			duration = 750;
			break;
		case POTION_EXTRAHEALING:
			break;
		case POTION_HEALING:
			break;
		case POTION_CUREAILMENT:
			duration = 4 * beatitude * TICKS_PER_SECOND;
			break;
		case POTION_BLINDNESS:
			duration = 500;
			break;
		case POTION_RESTOREMAGIC:
			break;
		case POTION_INVISIBILITY:
			duration = 1500 + (beatitude > 0 ? beatitude * 1500 : 0);
			break;
		case POTION_LEVITATION:
			duration = 1500 + (beatitude > 0 ? beatitude * 1500 : 0);
			break;
		case POTION_SPEED:
			duration = 3000 + (beatitude > 0 ? beatitude * 3000 : 0);
			break;
		case POTION_ACID:
			break;
		case POTION_PARALYSIS:
			duration = 350;
			break;
		case POTION_POLYMORPH:
			duration = 60 * TICKS_PER_SECOND * 4; // 4 mins
			break;
		case POTION_FIRESTORM:
		case POTION_ICESTORM:
		case POTION_THUNDERSTORM:
			break;
		case POTION_STRENGTH:
			duration = 3000 + (beatitude > 0 ? beatitude * 3000 : 0);
			break;
		default:
			break;
	}

	return duration;
}

Sint32 Item::potionGetEffectDurationMaximum(Entity* my, Stat* myStats) const
{
	if ( itemCategory(this) != POTION )
	{
		return 1;
	}

	int duration = 1;

	switch ( type )
	{
		case POTION_WATER:
			break;
		case POTION_BOOZE:
			if ( myStats && myStats->type == GOATMAN )
			{
				duration = 10500; // 3.5 mins
			}
			else
			{
				duration = 3000;
			}
			break;
		case POTION_JUICE:
			break;
		case POTION_SICKNESS:
			break;
		case POTION_CONFUSION:
			duration = 1500;
			break;
		case POTION_EXTRAHEALING:
			break;
		case POTION_HEALING:
			break;
		case POTION_CUREAILMENT:
			duration = 4 * beatitude * TICKS_PER_SECOND;
			break;
		case POTION_BLINDNESS:
			duration = 750;
			break;
		case POTION_RESTOREMAGIC:
			break;
		case POTION_INVISIBILITY:
			duration = 3000 + (beatitude > 0 ? beatitude * 1500 : 0);
			break;
		case POTION_LEVITATION:
			duration = 3000 + (beatitude > 0 ? beatitude * 1500 : 0);
			break;
		case POTION_SPEED:
			duration = 3000 + (beatitude > 0 ? beatitude * 3000 : 0);
			break;
		case POTION_ACID:
			break;
		case POTION_PARALYSIS:
			duration = 500;
			break;
		case POTION_POLYMORPH:
			duration = 60 * TICKS_PER_SECOND * 6; // 6 mins
			break;
		case POTION_FIRESTORM:
		case POTION_ICESTORM:
		case POTION_THUNDERSTORM:
			break;
		case POTION_STRENGTH:
			duration = 3000 + (beatitude > 0 ? beatitude * 3000 : 0);
			break;
		default:
			break;
	}

	return duration;
}

Sint32 Item::potionGetEffectDurationRandom(Entity* my, Stat* myStats) const
{
	Sint32 range = std::max(1, potionGetEffectDurationMaximum(my, myStats) - potionGetEffectDurationMinimum(my, myStats));
	return potionGetEffectDurationMinimum(my, myStats) + (local_rng.rand() % (range));
}

Sint32 Item::potionGetCursedEffectDurationMinimum(Entity* my, Stat* myStats) const
{
	if ( itemCategory(this) != POTION )
	{
		return 1;
	}

	int duration = 1;

	switch ( type )
	{
		case POTION_WATER:
			break;
		case POTION_BOOZE:
			break;
		case POTION_JUICE:
			duration = 1000;
			break;
		case POTION_SICKNESS:
			break;
		case POTION_CONFUSION:
			break;
		case POTION_EXTRAHEALING:
			duration = 750;
			break;
		case POTION_HEALING:
			duration = 750;
			break;
		case POTION_CUREAILMENT:
			duration = 750;
			break;
		case POTION_BLINDNESS:
			break;
		case POTION_RESTOREMAGIC:
			duration = 1000;
			break;
		case POTION_INVISIBILITY:
			break;
		case POTION_LEVITATION:
			duration = 1000;
			break;
		case POTION_SPEED:
			duration = 2000;
			break;
		case POTION_ACID:
			break;
		case POTION_PARALYSIS:
			break;
		case POTION_POLYMORPH:
			break;
		case POTION_FIRESTORM:
		case POTION_ICESTORM:
		case POTION_THUNDERSTORM:
			break;
		case POTION_STRENGTH:
			duration = 1000;
			break;
		default:
			break;
	}

	return duration;
}

Sint32 Item::potionGetCursedEffectDurationMaximum(Entity* my, Stat* myStats) const
{
	if ( itemCategory(this) != POTION )
	{
		return 1;
	}

	int duration = 1;

	switch ( type )
	{
		case POTION_WATER:
			break;
		case POTION_BOOZE:
			break;
		case POTION_JUICE:
			duration = 1500;
			break;
		case POTION_SICKNESS:
			break;
		case POTION_CONFUSION:
			break;
		case POTION_EXTRAHEALING:
			duration = 750;
			break;
		case POTION_HEALING:
			duration = 750;
			break;
		case POTION_CUREAILMENT:
			duration = 750;
			break;
		case POTION_BLINDNESS:
			break;
		case POTION_RESTOREMAGIC:
			duration = 1500;
			break;
		case POTION_INVISIBILITY:
			break;
		case POTION_LEVITATION:
			duration = 1500;
			break;
		case POTION_SPEED:
			duration = 3000;
			break;
		case POTION_ACID:
			break;
		case POTION_PARALYSIS:
			break;
		case POTION_POLYMORPH:
			break;
		case POTION_FIRESTORM:
		case POTION_ICESTORM:
		case POTION_THUNDERSTORM:
			break;
		case POTION_STRENGTH:
			duration = 1500;
			break;
		default:
			break;
	}

	return duration;
}

Sint32 Item::potionGetCursedEffectDurationRandom(Entity* my, Stat* myStats) const
{
	Sint32 range = std::max(1, potionGetCursedEffectDurationMaximum(my, myStats) - potionGetCursedEffectDurationMinimum(my, myStats));
	return potionGetCursedEffectDurationMinimum(my, myStats) + (local_rng.rand() % (range));
}

Sint32 Item::getWeight() const
{
	if ( type >= 0 && type < NUMITEMS )
	{
		if ( itemTypeIsQuiver(type) )
		{
			return std::max(1, items[type].weight * count / 5);
		}
		else
		{
			return items[type].weight * count;
		}
	}
	return 0;
}

void Item::foodTinGetDescriptionIndices(int* a, int* b, int* c) const
{
	Uint32 scaledAppearance = appearance % 4096;
	if ( a )
	{
		*a = ((scaledAppearance >> 8) & 0xF); // 0-15
	}
	if ( b )
	{
		*b = ((scaledAppearance >> 4) & 0xF); // 0-15
	}
	if ( c )
	{
		*c = (scaledAppearance & 0xF); // 0-15
	}
}

void Item::foodTinGetDescription(std::string& cookingMethod, std::string& protein, std::string& sides) const
{
	int a, b, c;
	foodTinGetDescriptionIndices(&a, &b, &c);
	cookingMethod = Language::get(918 + a);
	protein = Language::get(934 + b);
	sides = Language::get(950 + c);
}

int Item::foodGetPukeChance(Stat* eater) const
{
	int pukeChance = 100;
	switch ( status )
	{
		case EXCELLENT:
			pukeChance = 100; // 0%
			break;
		case SERVICABLE:
			pukeChance = 25; // 1 in 25, 4%
			break;
		case WORN:
			pukeChance = 10; // 1 in 10, 10%
			break;
		case DECREPIT:
			pukeChance = 4; // 1 in 4, 25%
			break;
		default:
			pukeChance = 100;
			break;
	}

	if ( eater )
	{
		if ( eater->type == VAMPIRE )
		{
			pukeChance = 1;
		}
		else if ( eater->type == INSECTOID )
		{
			pukeChance = 100; // insectoids can eat anything.
		}
	}

	return pukeChance;
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

	if ( !doesItemProvideBeatitudeAC() )
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
	/*value /= 1.f + statGetCHR(stats[player], players[player]->entity) / 20.f;*/

	// result
	value = std::max(1, value);

	/*if ( shopIsMysteriousShopkeeper(uidToEntity(shopkeeper[player])) )
	{
		value *= 2;
	}*/
	if ( itemSpecialShopConsumable )
	{
		real_t valueMult = std::max(1.0, ShopkeeperConsumables_t::consumableBuyValueMult / 100.0);
		valueMult *= value;
		value = valueMult;
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
	value *= 1.f + statGetCHR(stats[player], players[player]->entity) / 20.f;

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
				if ( player >= 0 && players[player] && players[player]->entity
					&& stats[player] && stats[player]->weapon
					&& (stats[player]->weapon->type == TOOL_LOCKPICK || stats[player]->weapon->type == TOOL_SKELETONKEY) )
				{
					const int skill = std::max(1, stats[player]->PROFICIENCIES[PRO_LOCKPICKING] / 10);
					bool failed = false;
					if ( skill < 2 || local_rng.rand() % skill == 0 ) // 20 skill requirement.
					{
						// failed.
						const Uint32 color = makeColorRGB(255, 0, 0);
						messagePlayerColor(player, MESSAGE_INTERACTION, color, Language::get(3871)); // trap fires.
						if ( skill < 2 )
						{
							messagePlayer(player, MESSAGE_INTERACTION, Language::get(3887)); // not skilled enough.
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
						const Uint32 color = makeColorRGB(0, 255, 0);
						messagePlayerColor(player, MESSAGE_INTERACTION, color, Language::get(3872));
						playSoundEntity(entity, 176, 128);
						entity->skill[4] = player + 1; // disabled flag and spit out items.
						serverUpdateEntitySkill(entity, 4); // update clients.
					}

					// degrade lockpick.
					if ( !(stats[player]->weapon->type == TOOL_SKELETONKEY) && (local_rng.rand() % 10 == 0 || (failed && local_rng.rand() % 4 == 0)) )
					{
						if ( players[player]->isLocalPlayer() )
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
							messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(1104));
							playSoundEntity(players[player]->entity, 76, 64);
						}
						else
						{
							messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(1103));
						}
						if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
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
					if ( !failed && local_rng.rand() % 5 == 0 )
					{
						players[player]->entity->increaseSkill(PRO_LOCKPICKING);
					}
					return;
				}
				else if ( entity->skill[4] != 0 )
				{
					messagePlayer(player, MESSAGE_HINT, Language::get(3870));
					return;
				}
			}
			break;
		}
	}

	if ( map.tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map.height] == 53 )
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(3873));
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

	if (potion.identified && 
		(potion.type == POTION_SICKNESS 
		|| potion.type == POTION_CONFUSION 
		|| potion.type == POTION_BLINDNESS 
		|| potion.type == POTION_ACID 
		|| potion.type == POTION_PARALYSIS
		|| potion.type == POTION_FIRESTORM 
		|| potion.type == POTION_ICESTORM 
		|| potion.type == POTION_THUNDERSTORM) )
	{
		return true;
	}

	if ( potion.type == POTION_EMPTY ) //So that you wield empty potions by default.
	{
		return true;
	}

	return false;
}

void createCustomInventory(Stat* const stats, const int itemLimit)
{
	int itemSlots[6] = { ITEM_SLOT_INV_1, ITEM_SLOT_INV_2, ITEM_SLOT_INV_3, ITEM_SLOT_INV_4, ITEM_SLOT_INV_5, ITEM_SLOT_INV_6 };
	int i = 0;
	Sint32 itemId = -1;
	int itemAppearance = local_rng.rand();
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
						randType = local_rng.rand() % 2;
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
						randType = local_rng.rand() % 2;
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
						randType = local_rng.rand() % 3;
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
					itemStatus = static_cast<Status>(DECREPIT + local_rng.rand() % 4);
				}
				else if ( itemStatus > BROKEN )
				{
					itemStatus = static_cast<Status>(itemStatus - 1); // reserved '0' for random, so '1' is decrepit... etc to '5' being excellent.
				}
				int itemBless = stats->EDITOR_ITEMS[itemSlots[i] + 2];
				if ( itemBless == 10 )
				{
					itemBless = -1 + local_rng.rand() % 3;
				}
				const int itemCount = stats->EDITOR_ITEMS[itemSlots[i] + 3];
				if ( stats->EDITOR_ITEMS[itemSlots[i] + 4] == 1 )
				{
					itemIdentified = true;
				}
				else if ( stats->EDITOR_ITEMS[itemSlots[i] + 4] == 2 )
				{
					itemIdentified = local_rng.rand() % 2;
				}
				else
				{
					itemIdentified = false;
				}
				itemAppearance = local_rng.rand();
				chance = stats->EDITOR_ITEMS[itemSlots[i] + 5];
				if ( local_rng.rand() % 100 < chance )
				{
					newItem(static_cast<ItemType>(itemId), itemStatus, itemBless, itemCount, itemAppearance, itemIdentified, &stats->inventory);
				}
				itemsGenerated++;
			}
		}
	}
}

node_t* itemNodeInInventory(const Stat* const myStats, Sint32 itemToFind, const Category cat)
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
			else if ( itemToFind >= 0 && item->type == static_cast<ItemType>(itemToFind) )
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
				playSoundEntity(my, 40 + local_rng.rand() % 4, 64);
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
				playSoundEntity(my, 40 + local_rng.rand() % 4, 64);
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
	int pick = local_rng.rand() % numoftype;// map_rng.rand() % numoftype;
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

bool Item::isThisABetterArmor(const Item& newArmor, const Item* const armorAlreadyHave )
{
	if ( !armorAlreadyHave )
	{
		//Some thing is better than no thing!
		return true;
	}

	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( FollowerMenu[i].entityToInteractWith )
		{
			if ( newArmor.interactNPCUid == FollowerMenu[i].entityToInteractWith->interactedByMonster )
			{
				return true;
			}
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

bool Item::shouldItemStack(const int player, bool ignoreStackLimit) const
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
				&& this->type != ENCHANTED_FEATHER
				&& this->type != TOOL_LANTERN
				&& this->type != TOOL_GLASSES
				&& this->type != TOOL_MIRROR
				&& this->type != TOOL_BLINDFOLD
				&& this->type != TOOL_BLINDFOLD_FOCUS
				&& this->type != TOOL_BLINDFOLD_TELEPATHY)
			|| itemCategory(this) == THROWN
			|| itemCategory(this) == GEM
			|| itemCategory(this) == POTION
			|| (itemCategory(this) == TOOL 
				&& this->type != TOOL_PICKAXE 
				&& this->type != TOOL_ALEMBIC 
				&& this->type != TOOL_TINKERING_KIT
				&& this->type != ENCHANTED_FEATHER
				&& this->type != TOOL_LANTERN
				&& this->type != TOOL_GLASSES
				&& this->type != TOOL_MIRROR
				&& this->type != TOOL_BLINDFOLD
				&& this->type != TOOL_BLINDFOLD_FOCUS
				&& this->type != TOOL_BLINDFOLD_TELEPATHY)
			)
		{
			// THROWN, GEM, TOOLS, POTIONS should stack when equipped.
			// otherwise most equippables should not stack.
			if ( itemCategory(this) == THROWN || itemCategory(this) == GEM )
			{
				if ( !ignoreStackLimit && count >= THROWN_GEM_MAX_STACK_QTY )
				{
					return false;
				}
			}
			else if ( itemTypeIsQuiver(this->type) )
			{
				if ( !ignoreStackLimit && count >= QUIVER_MAX_AMMO_QTY - 1 )
				{
					return false;
				}
				return true;
			}
			else if ( type == TOOL_METAL_SCRAP || type == TOOL_MAGIC_SCRAP )
			{
				if ( !ignoreStackLimit && count >= SCRAP_MAX_STACK_QTY - 1 )
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

bool Item::shouldItemStackInShop(bool ignoreStackLimit)
{
	node_t* itemNode = node;
	node = nullptr; // to make isEquipped return false in shouldItemStack
	bool result = shouldItemStack(clientnum, ignoreStackLimit);
	node = itemNode;
	return result;
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
	else if ( myStats->weapon->type == ARTIFACT_BOW )
	{
		bowModifier = 0.9;
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

bool Item::unableToEquipDueToSwapWeaponTimer(const int player) const
{
	if ( player >= 0 && players[player]->hud.pickaxeGimpTimer > 0 && !intro )
	{
		return true;
	}
	if ( player >= 0 && players[player]->hud.swapWeaponGimpTimer > 0 && !intro )
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
		Entity* follower = nullptr;
		if ( static_cast<Uint32*>(node->element) )
		{
			follower = uidToEntity(*static_cast<Uint32*>(node->element));
		}
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

void playerTryEquipItemAndUpdateServer(const int player, Item* const item, bool checkInventorySpaceForPaperDoll)
{
	if ( !item )
	{
		return;
	}

	if ( multiplayer == CLIENT && !players[player]->isLocalPlayer() )
	{
		return;
	}

	if ( checkInventorySpaceForPaperDoll )
	{
		if ( players[player]->isLocalPlayer() 
			&& players[player]->paperDoll.enabled
			&& players[player]->paperDoll.isItemOnDoll(*item) )
		{
			if ( !players[player]->inventoryUI.bItemInventoryHasFreeSlot() )
			{
				messagePlayer(player, MESSAGE_INVENTORY, Language::get(3997), item->getName());
				playSoundPlayer(player, 90, 64);
				return;
			}
		}
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
			if ( !cast_animation[player].active_spellbook )
			{
				equipResult = equipItem(item, &stats[player]->shield, player, checkInventorySpaceForPaperDoll);
			}
		}
		else
		{
			equipResult = equipItem(item, &stats[player]->weapon, player, checkInventorySpaceForPaperDoll);
		}
		if ( equipResult != EQUIP_ITEM_FAIL_CANT_UNEQUIP )
		{
			if ( cat == SPELLBOOK )
			{
				if ( !cast_animation[player].active_spellbook )
				{
					clientSendEquipUpdateToServer(EQUIP_ITEM_SLOT_SHIELD, equipResult, player,
						type, status, beatitude, count, appearance, identified);
				}
			}
			else
			{
				clientSendEquipUpdateToServer(EQUIP_ITEM_SLOT_WEAPON, equipResult, player,
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
			if ( !cast_animation[player].active_spellbook )
			{
				equipResult = equipItem(item, &stats[player]->shield, player, checkInventorySpaceForPaperDoll);
			}
		}
		else
		{
			equipResult = equipItem(item, &stats[player]->weapon, player, checkInventorySpaceForPaperDoll);
		}
	}
}

void clientSendAppearanceUpdateToServer(const int player, Item* item, const bool onIdentify)
{
	if ( multiplayer != CLIENT ) { return; }
	if ( !item || !itemIsEquipped(item, player) || items[item->type].item_slot == NO_EQUIP )
	{
		return;
	}
	strcpy((char*)net_packet->data, "EQUA");
	SDLNet_Write32(static_cast<Uint32>(item->type), &net_packet->data[4]);
	SDLNet_Write32(static_cast<Uint32>(item->status), &net_packet->data[8]);
	SDLNet_Write32(static_cast<Uint32>(item->beatitude), &net_packet->data[12]);
	SDLNet_Write32(static_cast<Uint32>(item->count), &net_packet->data[16]);
	SDLNet_Write32(static_cast<Uint32>(item->appearance), &net_packet->data[20]);
	net_packet->data[24] = item->identified;
	net_packet->data[25] = player;
	net_packet->data[26] = items[item->type].item_slot;
	net_packet->data[27] = onIdentify;
	net_packet->address.host = net_server.host;
	net_packet->address.port = net_server.port;
	net_packet->len = 28;
	sendPacketSafe(net_sock, -1, net_packet, 0);
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

void clientUnequipSlotAndUpdateServer(const int player, const EquipItemSendToServerSlot slot, Item* const item)
{
	if ( !item )
	{
		return;
	}
	EquipItemResult equipType = EQUIP_ITEM_FAIL_CANT_UNEQUIP;

	if ( slot == EQUIP_ITEM_SLOT_HELM )
	{
		equipType = equipItem(item, &stats[player]->helmet, player, false);
	}
	else if ( slot == EQUIP_ITEM_SLOT_BREASTPLATE )
	{
		equipType = equipItem(item, &stats[player]->breastplate, player, false);
	}
	else if ( slot == EQUIP_ITEM_SLOT_GLOVES )
	{
		equipType = equipItem(item, &stats[player]->gloves, player, false);
	}
	else if ( slot == EQUIP_ITEM_SLOT_BOOTS )
	{
		equipType = equipItem(item, &stats[player]->shoes, player, false);
	}
	else if ( slot == EQUIP_ITEM_SLOT_SHIELD )
	{
		equipType = equipItem(item, &stats[player]->shield, player, false);
	}
	else if ( slot == EQUIP_ITEM_SLOT_CLOAK )
	{
		equipType = equipItem(item, &stats[player]->cloak, player, false);
	}
	else if ( slot == EQUIP_ITEM_SLOT_AMULET )
	{
		equipType = equipItem(item, &stats[player]->amulet, player, false);
	}
	else if ( slot == EQUIP_ITEM_SLOT_RING )
	{
		equipType = equipItem(item, &stats[player]->ring, player, false);
	}
	else if ( slot == EQUIP_ITEM_SLOT_MASK )
	{
		equipType = equipItem(item, &stats[player]->mask, player, false);
	}

	clientSendEquipUpdateToServer(slot, equipType, player,
		item->type, item->status, item->beatitude, item->count, item->appearance, item->identified);
}

int Item::getLootBagPlayer() const
{
	return (int)(appearance & 0x000000FF) % MAXPLAYERS;
}
int Item::getLootBagNumItems() const
{
	if ( multiplayer == CLIENT )
	{
		return 0;
	}
	if ( stats[0]->player_lootbags.find(appearance)
		!= stats[0]->player_lootbags.end() )
	{
		auto& lootbag = stats[0]->player_lootbags[appearance];
		if ( !lootbag.looted )
		{
			return lootbag.items.size();
		}
		return 0;
	}
	return 0;
}