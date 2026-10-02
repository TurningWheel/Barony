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
#include "monster.hpp"
#include "collision.hpp"

#include <assert.h>
#include <limits>

Uint32 itemuids = 1;
ItemGeneric items[NUMITEMS];
const real_t potionDamageSkillMultipliers[6] = { 1.f, 1.1, 1.25, 1.5, 2.5, 4.f };
const real_t thrownDamageSkillMultipliers[6] = { 1.f, 1.1, 1.25, 1.5, 2.f, 3.f };
Uint32 enchantedFeatherScrollSeed;
std::vector<int> enchantedFeatherScrollsShuffled;
bool overrideTinkeringLimit = false;
int decoyBoxRange = 15;

namespace
{
	Uint32 nextMagicRuneInstanceId = 1;
	Uint32 nextRuneCreatorIdentity = 1;
	Uint32 nextFamiliarInstanceId = 1;

	Uint32 allocateNonzeroId(Uint32& nextId)
	{
		if ( nextId == 0 ) { nextId = 1; }
		const Uint32 result = nextId++;
		if ( nextId == 0 ) { nextId = 1; }
		return result;
	}
}

void reserveFamiliarInstanceId(Uint32 familiarInstanceId)
{
	if ( familiarInstanceId != 0 && familiarInstanceId >= nextFamiliarInstanceId )
	{
		nextFamiliarInstanceId = familiarInstanceId + 1;
		if ( nextFamiliarInstanceId == 0 ) { nextFamiliarInstanceId = 1; }
	}
}

void reserveMagicRuneInstanceId(Uint32 runeInstanceId)
{
	if ( runeInstanceId != 0 && runeInstanceId >= nextMagicRuneInstanceId )
	{
		nextMagicRuneInstanceId = runeInstanceId + 1;
		if ( nextMagicRuneInstanceId == 0 ) { nextMagicRuneInstanceId = 1; }
	}
}

void reserveRuneCreatorIdentity(Uint32 creatorIdentity)
{
	if ( creatorIdentity != 0 && creatorIdentity >= nextRuneCreatorIdentity )
	{
		nextRuneCreatorIdentity = creatorIdentity + 1;
		if ( nextRuneCreatorIdentity == 0 ) { nextRuneCreatorIdentity = 1; }
	}
}

Uint32 ensureRuneCreatorIdentity(int player)
{
	if ( player < 0 || player >= MAXPLAYERS || !stats[player] ) { return 0; }
	if ( stats[player]->runeCreatorIdentity != 0 )
	{
		reserveRuneCreatorIdentity(stats[player]->runeCreatorIdentity);
		return stats[player]->runeCreatorIdentity;
	}
	if ( multiplayer == CLIENT ) { return 0; }
	stats[player]->runeCreatorIdentity = allocateNonzeroId(nextRuneCreatorIdentity);
	return stats[player]->runeCreatorIdentity;
}

static bool itemQualifiesForGolemPolarityReversal(const Item& item)
{
	if ( item.type == AMULET_SEXCHANGE || items[item.type].item_slot == NO_EQUIP )
	{
		return false;
	}

	switch ( itemCategory(&item) )
	{
		case POTION:
		case FOOD:
		case SCROLL:
		case SPELL_CAT:
			return false;
		default:
			return true;
	}
}

bool updateGolemPolarityFromComposition(int player)
{
	if ( multiplayer == CLIENT || player < 0 || player >= MAXPLAYERS || !stats[player] )
	{
		return false;
	}

	Stat& playerStats = *stats[player];
	// This identifies the persistent natural-Golem identity without consulting
	// its temporary polymorph/shapeshift body. Only sex is changed below.
	if ( playerStats.playerRace != RACE_GOLEM || playerStats.stat_appearance != 0 )
	{
		return false;
	}

	sex_t newPolarity = playerStats.sex;
	if ( playerStats.sex == MALE
		&& playerStats.golemBlessedComposition < GOLEM_POLARITY_SWITCH_POINT )
	{
		newPolarity = FEMALE;
	}
	else if ( playerStats.sex == FEMALE
		&& playerStats.golemBlessedComposition > GOLEM_POLARITY_SWITCH_POINT )
	{
		newPolarity = MALE;
	}

	if ( newPolarity == playerStats.sex )
	{
		return false;
	}
	playerStats.sex = newPolarity;
	serverUpdateSexChange(player);
	return true;
}

void reverseGolemEquipmentBeatitudes(Stat& playerStats, const Item* excludedItem)
{
	// Player equipment slots point at these same inventory objects, so this single
	// authoritative pass also updates equipped gear without double-flipping it.
	for ( node_t* node = playerStats.inventory.first; node; node = node->next )
	{
		Item* inventoryItem = static_cast<Item*>(node->element);
		if ( inventoryItem && inventoryItem != excludedItem
			&& itemQualifiesForGolemPolarityReversal(*inventoryItem) )
		{
			inventoryItem->beatitude = -inventoryItem->beatitude;
		}
	}
}

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
		if ( item.type == TOOL_DUCK )
		{
			players[player]->hotbar.magicDuckHotbarSlot = index;
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
	if ( itemTypeIsIngredientBag(item->type) ) { item->count = 1; }
	item->appearance = appearance;
	item->identified = identified;
	item->uid = itemuids;
	item->ownerUid = 0;
	item->isDroppable = true;
	item->playerSoldItemToShop = false;
	item->itemHiddenFromShop = false;
	item->itemRequireTradingSkillInShop = 0;
	item->itemSpecialShopConsumable = false;
	item->runeStoredPWR = Item::RUNE_STORED_PWR_INVALID;
	item->runeCreatorPlayer = Item::RUNE_CREATOR_INVALID;
	item->runeInstanceId = Item::RUNE_INSTANCE_ID_INVALID;
	item->runeCreatorIdentity = Item::RUNE_CREATOR_IDENTITY_INVALID;
	item->appliedOil = APPLIED_OIL_NONE;
	item->ingredientCounts[0] = 0;
	item->ingredientCounts[1] = 0;
	item->ingredientCounts[2] = 0;
	item->ingredientCounts[3] = 0;
	item->familiarInitialized = false;
	item->familiarInstanceId = 0;
	item->familiarHP = 0;
	item->familiarMAXHP = 0;
	item->familiarMP = 0;
	item->familiarMAXMP = 0;
	item->familiarLVL = 0;
	item->familiarEXP = 0;
	item->familiarSTR = 0;
	item->familiarDEX = 0;
	item->familiarCON = 0;
	item->familiarINT = 0;
	item->familiarPER = 0;
	item->familiarCHR = 0;
	item->interactNPCUid = 0;
	item->notifyIcon = false;
	item->spellNotifyIcon = false;
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

void copyItemPersistentCustomState(Item& destination, const Item& source)
{
	destination.runeSetStoredPWRRaw(source.runeGetStoredPWRRaw());
	destination.runeSetCreatorPlayer(source.runeGetCreatorPlayer());
	destination.runeSetInstanceId(source.runeGetInstanceId());
	destination.runeSetCreatorIdentity(source.runeGetCreatorIdentity());
	destination.appliedOil = source.appliedOil;
	for ( int i = 0; i < 4; ++i )
	{
		destination.ingredientCounts[i] = source.ingredientCounts[i];
	}
	destination.familiarInitialized = source.familiarInitialized;
	destination.familiarInstanceId = source.familiarInstanceId;
	destination.familiarHP = source.familiarHP;
	destination.familiarMAXHP = source.familiarMAXHP;
	destination.familiarMP = source.familiarMP;
	destination.familiarMAXMP = source.familiarMAXMP;
	destination.familiarLVL = source.familiarLVL;
	destination.familiarEXP = source.familiarEXP;
	destination.familiarSTR = source.familiarSTR;
	destination.familiarDEX = source.familiarDEX;
	destination.familiarCON = source.familiarCON;
	destination.familiarINT = source.familiarINT;
	destination.familiarPER = source.familiarPER;
	destination.familiarCHR = source.familiarCHR;
	reserveFamiliarInstanceId(destination.familiarInstanceId);
}

void copyItemPersistentCustomStateToEntity(Entity& destination, const Item& source)
{
	destination.itemRuneStoredPWR = source.runeGetStoredPWRRaw();
	destination.itemRuneStoredPWRValid = source.runeHasStoredPWR() ? 1 : 0;
	destination.itemRuneCreatorPlayer = source.runeGetCreatorPlayer();
	destination.itemRuneCreatorPlayerValid = source.runeHasCreator() ? 1 : 0;
	destination.itemRuneInstanceId = static_cast<Sint32>(source.runeGetInstanceId());
	destination.itemRuneCreatorIdentity = static_cast<Sint32>(source.runeGetCreatorIdentity());
	destination.itemAppliedOil = static_cast<Sint32>(source.appliedOil);
	destination.itemIngredientCount0 = 0;
	destination.itemIngredientCount1 = 0;
	destination.itemIngredientCount2 = 0;
	destination.itemIngredientCount3 = 0;
	destination.itemIngredientCount0 = static_cast<Sint32>(source.ingredientCounts[0]);
	destination.itemIngredientCount1 = static_cast<Sint32>(source.ingredientCounts[1]);
	destination.itemIngredientCount2 = static_cast<Sint32>(source.ingredientCounts[2]);
	destination.itemIngredientCount3 = static_cast<Sint32>(source.ingredientCounts[3]);
	destination.itemFamiliarInitialized = source.familiarInitialized ? 1 : 0;
	destination.itemFamiliarInstanceId = static_cast<Sint32>(source.familiarInstanceId);
	destination.itemFamiliarHP = source.familiarHP;
	destination.itemFamiliarMAXHP = source.familiarMAXHP;
	destination.itemFamiliarMP = source.familiarMP;
	destination.itemFamiliarMAXMP = source.familiarMAXMP;
	destination.itemFamiliarLVL = source.familiarLVL;
	destination.itemFamiliarEXP = source.familiarEXP;
	destination.itemFamiliarSTR = source.familiarSTR;
	destination.itemFamiliarDEX = source.familiarDEX;
	destination.itemFamiliarCON = source.familiarCON;
	destination.itemFamiliarINT = source.familiarINT;
	destination.itemFamiliarPER = source.familiarPER;
	destination.itemFamiliarCHR = source.familiarCHR;
}

void copyItemPersistentCustomStateFromEntity(Item& destination, const Entity& source)
{
	destination.runeSetStoredPWRRaw(source.itemRuneStoredPWRValid
		? source.itemRuneStoredPWR : Item::RUNE_STORED_PWR_INVALID);
	destination.runeSetCreatorPlayer(source.itemRuneCreatorPlayerValid
		? source.itemRuneCreatorPlayer : Item::RUNE_CREATOR_INVALID);
	destination.runeSetInstanceId(static_cast<Uint32>(source.itemRuneInstanceId));
	destination.runeSetCreatorIdentity(static_cast<Uint32>(source.itemRuneCreatorIdentity));
	destination.appliedOil = static_cast<AppliedOil>(std::max(0, std::min(source.itemAppliedOil,
		static_cast<Sint32>(APPLIED_OIL_MAX) - 1)));
	destination.ingredientCounts[0] = static_cast<Uint32>(source.itemIngredientCount0);
	destination.ingredientCounts[1] = static_cast<Uint32>(source.itemIngredientCount1);
	destination.ingredientCounts[2] = static_cast<Uint32>(source.itemIngredientCount2);
	destination.ingredientCounts[3] = static_cast<Uint32>(source.itemIngredientCount3);
	destination.familiarInitialized = source.itemFamiliarInitialized != 0;
	destination.familiarInstanceId = static_cast<Uint32>(source.itemFamiliarInstanceId);
	destination.familiarHP = source.itemFamiliarHP;
	destination.familiarMAXHP = source.itemFamiliarMAXHP;
	destination.familiarMP = source.itemFamiliarMP;
	destination.familiarMAXMP = source.itemFamiliarMAXMP;
	destination.familiarLVL = source.itemFamiliarLVL;
	destination.familiarEXP = source.itemFamiliarEXP;
	destination.familiarSTR = source.itemFamiliarSTR;
	destination.familiarDEX = source.itemFamiliarDEX;
	destination.familiarCON = source.itemFamiliarCON;
	destination.familiarINT = source.itemFamiliarINT;
	destination.familiarPER = source.itemFamiliarPER;
	destination.familiarCHR = source.itemFamiliarCHR;
	reserveFamiliarInstanceId(destination.familiarInstanceId);
}

bool itemTypeIsTincture(ItemType type)
{
	return type >= TINCTURE_STR && type <= TINCTURE_CHA;
}

bool itemTypeIsOil(ItemType type)
{
	return type >= OIL_AETHER && type <= OIL_BRINE;
}

bool itemTypeIsInk(ItemType type)
{
	return type >= INK_MYSTICISM && type <= INK_THAUMATURGY;
}

bool itemTypeIsWitchbrewConsumable(ItemType type)
{
	return itemTypeIsTincture(type) || itemTypeIsOil(type) || itemTypeIsInk(type);
}

bool itemTypeIsIngredientBag(ItemType type)
{
	return type >= INGREDIENT_BAG_SLIME && type <= INGREDIENT_BAG_ANIMATIC;
}

bool itemTypeIsWitchIngredient(ItemType type)
{
	return type >= INGREDIENT_GELATIN && type <= INGREDIENT_FRAGMENTS;
}

bool itemTypeIsWitchbrewGeneratedItem(ItemType type)
{
	return itemTypeIsIngredientBag(type) || itemTypeIsWitchIngredient(type);
}

WitchIngredientCategory ingredientCategoryFromBag(ItemType type)
{
	if ( !itemTypeIsIngredientBag(type) ) { return INGREDIENT_CATEGORY_NONE; }
	return static_cast<WitchIngredientCategory>(INGREDIENT_CATEGORY_SLIME
		+ static_cast<int>(type - INGREDIENT_BAG_SLIME));
}

WitchIngredientCategory ingredientCategoryFromItem(ItemType type)
{
	if ( !itemTypeIsWitchIngredient(type) ) { return INGREDIENT_CATEGORY_NONE; }
	return static_cast<WitchIngredientCategory>(INGREDIENT_CATEGORY_SLIME
		+ static_cast<int>(type - INGREDIENT_GELATIN) / 4);
}

ItemType ingredientBagForCategory(WitchIngredientCategory category)
{
	if ( category < INGREDIENT_CATEGORY_SLIME || category > INGREDIENT_CATEGORY_ANIMATIC )
	{
		return ITEM_ENUM_MAX;
	}
	return static_cast<ItemType>(INGREDIENT_BAG_SLIME + category - INGREDIENT_CATEGORY_SLIME);
}

ItemType ingredientForCategorySlot(WitchIngredientCategory category, int slot)
{
	if ( category < INGREDIENT_CATEGORY_SLIME || category > INGREDIENT_CATEGORY_ANIMATIC
		|| slot < 0 || slot >= 4 )
	{
		return ITEM_ENUM_MAX;
	}
	return static_cast<ItemType>(INGREDIENT_GELATIN
		+ (category - INGREDIENT_CATEGORY_SLIME) * 4 + slot);
}

int ingredientSlotForItem(ItemType type)
{
	return itemTypeIsWitchIngredient(type) ? static_cast<int>(type - INGREDIENT_GELATIN) % 4 : -1;
}

WitchIngredientCategory getIngredientCategoryForMonster(Entity* monster)
{
	Stat* monsterStats = monster ? monster->getStats() : nullptr;
	if ( !monsterStats ) { return INGREDIENT_CATEGORY_NONE; }
	if ( monsterStats->type == SLIME ) { return INGREDIENT_CATEGORY_SLIME; }
	auto found = CompendiumEntries.monsters.find(monstertypename[monsterStats->type]);
	if ( found == CompendiumEntries.monsters.end() ) { return INGREDIENT_CATEGORY_NONE; }
	switch ( found->second.species )
	{
		case Compendium_t::CompendiumMonsters_t::SPECIES_HUMANOID:
		case Compendium_t::CompendiumMonsters_t::SPECIES_DEMONOID:
			return INGREDIENT_CATEGORY_HUMORIN;
		case Compendium_t::CompendiumMonsters_t::SPECIES_BEAST:
		case Compendium_t::CompendiumMonsters_t::SPECIES_BEASTFOLK:
			return INGREDIENT_CATEGORY_WILDBLOOD;
		case Compendium_t::CompendiumMonsters_t::SPECIES_UNDEAD:
		case Compendium_t::CompendiumMonsters_t::SPECIES_CONSTRUCT:
			return INGREDIENT_CATEGORY_ANIMATIC;
		default:
			return INGREDIENT_CATEGORY_NONE;
	}
}

bool ingredientBagIsEmpty(const Item& bag)
{
	if ( !itemTypeIsIngredientBag(bag.type) ) { return true; }
	return bag.ingredientCounts[0] == 0 && bag.ingredientCounts[1] == 0
		&& bag.ingredientCounts[2] == 0 && bag.ingredientCounts[3] == 0;
}

bool mergeIngredientBags(Item& destination, const Item& source)
{
	if ( !itemTypeIsIngredientBag(destination.type) || !itemTypeIsIngredientBag(source.type)
		|| ingredientCategoryFromBag(destination.type) != ingredientCategoryFromBag(source.type) )
	{
		return false;
	}
	for ( int i = 0; i < 4; ++i )
	{
		const Uint32 room = std::numeric_limits<Uint32>::max() - destination.ingredientCounts[i];
		destination.ingredientCounts[i] += std::min(room, source.ingredientCounts[i]);
	}
	destination.count = 1;
	return true;
}

Item* findIngredientBagInInventory(list_t& inventory, WitchIngredientCategory category)
{
	for ( node_t* node = inventory.first; node; node = node->next )
	{
		Item* item = static_cast<Item*>(node->element);
		if ( item && ingredientCategoryFromBag(item->type) == category ) { return item; }
	}
	return nullptr;
}

Item* mergeIngredientBagIntoInventory(list_t& inventory, const Item& source)
{
	const WitchIngredientCategory category = ingredientCategoryFromBag(source.type);
	if ( category == INGREDIENT_CATEGORY_NONE || ingredientBagIsEmpty(source) ) { return nullptr; }
	if ( Item* existing = findIngredientBagInInventory(inventory, category) )
	{
		mergeIngredientBags(*existing, source);
		return existing;
	}
	Item* created = newItem(ingredientBagForCategory(category), EXCELLENT, 0, 1, 0, true, &inventory);
	copyItemPersistentCustomState(*created, source);
	created->count = 1;
	return created;
}

namespace
{
	std::array<std::array<Uint32, STEWING_RESOURCE_COUNT>, MAXPLAYERS> serverStewingInventoryMirrors{};
	std::array<bool, MAXPLAYERS> serverStewingInventoryMirrorValid{};
	std::array<Uint32, MAXPLAYERS> learnedStewingRecipeMasks{};
	Uint32 activeStewingGeneratorVersion = STEWING_GENERATOR_VERSION;

	using SymbolicStewingRecipe = std::array<std::array<Uint8, 4>, 4>;
	using StewingCategoryMappings = std::array<std::array<ItemType, 4>, 4>;

	struct GeneratedStewingRecipeBook
	{
		std::array<StewingRecipe, STEWING_RECIPE_COUNT> recipes{};
		std::array<SymbolicStewingRecipe, STEWING_RECIPE_COUNT> symbolicRecipes{};
		StewingCategoryMappings categoryMappings{};
		std::array<bool, 4> highTinctureCategories{};
	};

	template <typename T>
	void stewingShuffle(std::vector<T>& values, BaronyRNG& rng)
	{
		for ( int i = static_cast<int>(values.size()) - 1; i > 0; --i )
		{
			const int j = rng.uniform(0, i);
			std::swap(values[i], values[j]);
		}
	}

	template <typename T, size_t Size>
	void stewingShuffle(std::array<T, Size>& values, BaronyRNG& rng)
	{
		for ( int i = static_cast<int>(Size) - 1; i > 0; --i )
		{
			const int j = rng.uniform(0, i);
			std::swap(values[i], values[j]);
		}
	}

	StewingCategoryMappings generateStewingCategoryMappings(BaronyRNG& rng)
	{
		StewingCategoryMappings mappings{};
		for ( int category = 0; category < 4; ++category )
		{
			const auto ingredientCategory = static_cast<WitchIngredientCategory>(
				static_cast<int>(INGREDIENT_CATEGORY_SLIME) + category);
			for ( int slot = 0; slot < 4; ++slot )
			{
				mappings[category][slot] = ingredientForCategorySlot(ingredientCategory, slot);
			}
			stewingShuffle(mappings[category], rng);
		}
		return mappings;
	}

	bool symbolicRecipeAlreadyUsed(const SymbolicStewingRecipe& candidate,
		const std::array<SymbolicStewingRecipe, STEWING_RECIPE_COUNT>& recipes,
		int first, int count)
	{
		for ( int i = 0; i < count; ++i )
		{
			if ( recipes[first + i] == candidate ) { return true; }
		}
		return false;
	}

	bool generateTinctureRecipes(GeneratedStewingRecipeBook& book, BaronyRNG& rng)
	{
		std::array<int, 4> categoryOrder{ { 0, 1, 2, 3 } };
		stewingShuffle(categoryOrder, rng);
		std::array<int, 4> doubledTargets{};
		std::array<std::array<int, 2>, 4> resourceTargets{};
		for ( int i = 0; i < 4; ++i )
		{
			const int category = categoryOrder[i];
			const bool high = i < 2;
			book.highTinctureCategories[category] = high;
			doubledTargets[category] = high ? 2 : 1;
			if ( high )
			{
				resourceTargets[category] = { { 4, 4 } };
			}
			else if ( rng.uniform(0, 1) == 0 )
			{
				resourceTargets[category] = { { 4, 3 } };
			}
			else
			{
				resourceTargets[category] = { { 3, 4 } };
			}
		}

		struct Candidate
		{
			SymbolicStewingRecipe recipe{};
			int doubledCategory = 0;
		};
		std::array<int, 4> doubledUsed{};
		std::array<std::array<int, 2>, 4> resourceUsed{};
		std::function<bool(int)> solve = [&](int row)
		{
			if ( row == 6 )
			{
				return doubledUsed == doubledTargets && resourceUsed == resourceTargets;
			}
			std::vector<Candidate> candidates;
			for ( int doubled = 0; doubled < 4; ++doubled )
			{
				if ( doubledUsed[doubled] >= doubledTargets[doubled] ) { continue; }
				for ( int choices = 0; choices < 8; ++choices )
				{
					Candidate candidate;
					candidate.doubledCategory = doubled;
					candidate.recipe[doubled][0] = 1;
					candidate.recipe[doubled][1] = 1;
					int choiceBit = 0;
					for ( int category = 0; category < 4; ++category )
					{
						if ( category == doubled ) { continue; }
						const int symbol = (choices >> choiceBit) & 1;
						candidate.recipe[category][symbol] = 1;
						++choiceBit;
					}
					bool valid = !symbolicRecipeAlreadyUsed(candidate.recipe,
						book.symbolicRecipes, 4, row);
					for ( int category = 0; category < 4 && valid; ++category )
					{
						for ( int symbol = 0; symbol < 2; ++symbol )
						{
							if ( resourceUsed[category][symbol]
								+ candidate.recipe[category][symbol] > resourceTargets[category][symbol] )
							{
								valid = false;
							}
						}
					}
					if ( valid ) { candidates.push_back(candidate); }
				}
			}
			stewingShuffle(candidates, rng);
			for ( const Candidate& candidate : candidates )
			{
				book.symbolicRecipes[4 + row] = candidate.recipe;
				++doubledUsed[candidate.doubledCategory];
				for ( int category = 0; category < 4; ++category )
				{
					for ( int symbol = 0; symbol < 2; ++symbol )
					{
						resourceUsed[category][symbol] += candidate.recipe[category][symbol];
					}
				}
				if ( solve(row + 1) ) { return true; }
				--doubledUsed[candidate.doubledCategory];
				for ( int category = 0; category < 4; ++category )
				{
					for ( int symbol = 0; symbol < 2; ++symbol )
					{
						resourceUsed[category][symbol] -= candidate.recipe[category][symbol];
					}
				}
			}
			book.symbolicRecipes[4 + row] = SymbolicStewingRecipe{};
			return false;
		};
		return solve(0);
	}

	bool generateOilRecipes(GeneratedStewingRecipeBook& book, BaronyRNG& rng)
	{
		static const std::array<std::array<Uint8, 4>, 12> categoryPatterns{ {
			{ { 3, 2, 2, 1 } }, { { 3, 2, 1, 2 } }, { { 3, 1, 2, 2 } },
			{ { 2, 3, 2, 1 } }, { { 2, 3, 1, 2 } },
			{ { 2, 2, 3, 1 } }, { { 2, 2, 1, 3 } },
			{ { 2, 1, 3, 2 } }, { { 2, 1, 2, 3 } },
			{ { 1, 3, 2, 2 } }, { { 1, 2, 3, 2 } }, { { 1, 2, 2, 3 } }
		} };
		std::array<std::array<int, 2>, 4> resourceUsed{};
		std::function<bool(int)> solve = [&](int row)
		{
			if ( row == 7 )
			{
				for ( const auto& category : resourceUsed )
				{
					if ( category[0] != 7 || category[1] != 7 ) { return false; }
				}
				return true;
			}
			std::vector<SymbolicStewingRecipe> candidates;
			for ( const auto& pattern : categoryPatterns )
			{
				for ( int tripleOrientation = 0; tripleOrientation < 2; ++tripleOrientation )
				{
					for ( int singleOrientation = 0; singleOrientation < 2; ++singleOrientation )
					{
						SymbolicStewingRecipe candidate{};
						for ( int category = 0; category < 4; ++category )
						{
							if ( pattern[category] == 3 )
							{
								candidate[category][2] = tripleOrientation == 0 ? 2 : 1;
								candidate[category][3] = tripleOrientation == 0 ? 1 : 2;
							}
							else if ( pattern[category] == 2 )
							{
								candidate[category][2] = 1;
								candidate[category][3] = 1;
							}
							else
							{
								candidate[category][2 + singleOrientation] = 1;
							}
						}
						bool valid = !symbolicRecipeAlreadyUsed(candidate,
							book.symbolicRecipes, 10, row);
						for ( int category = 0; category < 4 && valid; ++category )
						{
							for ( int symbol = 0; symbol < 2; ++symbol )
							{
								if ( resourceUsed[category][symbol]
									+ candidate[category][2 + symbol] > 7 )
								{
									valid = false;
								}
							}
						}
						if ( valid ) { candidates.push_back(candidate); }
					}
				}
			}
			stewingShuffle(candidates, rng);
			for ( const SymbolicStewingRecipe& candidate : candidates )
			{
				book.symbolicRecipes[10 + row] = candidate;
				for ( int category = 0; category < 4; ++category )
				{
					for ( int symbol = 0; symbol < 2; ++symbol )
					{
						resourceUsed[category][symbol] += candidate[category][2 + symbol];
					}
				}
				if ( solve(row + 1) ) { return true; }
				for ( int category = 0; category < 4; ++category )
				{
					for ( int symbol = 0; symbol < 2; ++symbol )
					{
						resourceUsed[category][symbol] -= candidate[category][2 + symbol];
					}
				}
			}
			book.symbolicRecipes[10 + row] = SymbolicStewingRecipe{};
			return false;
		};
		return solve(0);
	}

	void generateInkRecipes(GeneratedStewingRecipeBook& book, BaronyRNG& rng)
	{
		std::array<std::array<bool, 2>, 4> omitExtra{};
		for ( int category = 0; category < 4; ++category )
		{
			if ( book.highTinctureCategories[category] )
			{
				omitExtra[category][rng.uniform(0, 1)] = true;
			}
		}
		std::vector<std::array<int, 2>> extras;
		for ( int category = 0; category < 4; ++category )
		{
			for ( int symbol = 0; symbol < 2; ++symbol )
			{
				if ( !omitExtra[category][symbol] )
				{
					extras.push_back({ { category, symbol + 2 } });
				}
			}
		}
		stewingShuffle(extras, rng);
		for ( int row = 0; row < 3; ++row )
		{
			SymbolicStewingRecipe& recipe = book.symbolicRecipes[17 + row];
			for ( int category = 0; category < 4; ++category )
			{
				recipe[category][2] = 2;
				recipe[category][3] = 2;
			}
			for ( int extra = 0; extra < 2; ++extra )
			{
				const auto& resource = extras[row * 2 + extra];
				++recipe[resource[0]][resource[1]];
			}
		}
	}

	StewingRecipe symbolicRecipeToConcrete(const SymbolicStewingRecipe& symbolic,
		const StewingCategoryMappings& mappings, ItemType result, bool addWater)
	{
		StewingRecipe recipe;
		recipe.result = result;
		recipe.alwaysUnlocked = addWater;
		recipe.educts.fill(StewingEduct{});
		recipe.eductCount = 0;
		for ( int category = 0; category < 4; ++category )
		{
			for ( int symbol = 0; symbol < 4; ++symbol )
			{
				const Uint8 quantity = symbolic[category][symbol];
				if ( quantity == 0 ) { continue; }
				assert(recipe.eductCount < STEWING_MAX_EDUCTS);
				recipe.educts[recipe.eductCount++] = { mappings[category][symbol], quantity };
			}
		}
		if ( addWater )
		{
			assert(recipe.eductCount < STEWING_MAX_EDUCTS);
			recipe.educts[recipe.eductCount++] = { POTION_WATER, 1 };
		}
		return recipe;
	}

	bool stewingValidationFailure(const char* invariant)
	{
		printlog("[Stewing] recipe generator validation failed: %s", invariant);
		return false;
	}

	bool validateStewingRecipeBook(const GeneratedStewingRecipeBook& book)
	{
		static const std::array<ItemType, STEWING_RECIPE_COUNT> expectedResults{ {
			POTION_ACID, POTION_BOOZE, POTION_JUICE, POTION_INVISIBILITY,
			TINCTURE_STR, TINCTURE_DEX, TINCTURE_CON, TINCTURE_INT, TINCTURE_PER, TINCTURE_CHA,
			OIL_AETHER, OIL_SILVER, OIL_TOXINS, OIL_BASES, OIL_LEAD, OIL_MINERAL, OIL_BRINE,
			INK_MYSTICISM, INK_SORCERY, INK_THAUMATURGY
		} };
		std::array<std::array<Uint32, STEWING_RESOURCE_COUNT>, STEWING_RECIPE_COUNT> concrete{};
		for ( int row = 0; row < STEWING_RECIPE_COUNT; ++row )
		{
			const StewingRecipe& recipe = book.recipes[row];
			if ( recipe.result != expectedResults[row] )
			{
				return stewingValidationFailure("result order/uniqueness");
			}
			if ( recipe.eductCount > STEWING_MAX_EDUCTS )
			{
				return stewingValidationFailure("eductCount exceeds STEWING_MAX_EDUCTS");
			}
			for ( Uint32 i = 0; i < recipe.eductCount; ++i )
			{
				const int resource = getStewingResourceIndex(recipe.educts[i].type);
				if ( recipe.educts[i].quantity == 0 )
				{
					return stewingValidationFailure("zero-quantity stored educt");
				}
				if ( resource < 0 || concrete[row][resource] != 0 )
				{
					return stewingValidationFailure("invalid or duplicate concrete educt");
				}
				concrete[row][resource] = recipe.educts[i].quantity;
			}
			for ( int previous = 0; previous < row; ++previous )
			{
				if ( concrete[previous] == concrete[row] )
				{
					return stewingValidationFailure("duplicate full concrete recipe");
				}
			}
		}

		std::array<Uint32, 4> wholeCategoryTotals{};
		for ( int row = 0; row < STEWING_RECIPE_COUNT; ++row )
		{
			for ( int category = 0; category < 4; ++category )
			{
				for ( int symbol = 0; symbol < 4; ++symbol )
				{
					wholeCategoryTotals[category] += book.symbolicRecipes[row][category][symbol];
				}
			}
		}

		for ( int row = 0; row < 4; ++row )
		{
			if ( !book.recipes[row].alwaysUnlocked || concrete[row][16] != 1 )
			{
				return stewingValidationFailure("starting Potion unlock/Water invariant");
			}
			Uint32 witchTotal = 0;
			for ( int category = 0; category < 4; ++category )
			{
				for ( int symbol = 0; symbol < 4; ++symbol )
				{
					const Uint8 expected = category == row ? 1 : 0;
					if ( book.symbolicRecipes[row][category][symbol] != expected )
					{
						return stewingValidationFailure("starting Potion category A+B+C+D invariant");
					}
					witchTotal += book.symbolicRecipes[row][category][symbol];
				}
			}
			if ( witchTotal != 4 ) { return stewingValidationFailure("starting Potion cost"); }
		}
		for ( int row = 4; row < STEWING_RECIPE_COUNT; ++row )
		{
			if ( book.recipes[row].alwaysUnlocked || concrete[row][16] != 0 )
			{
				return stewingValidationFailure("non-Potion lock/Water invariant");
			}
		}

		std::array<Uint32, 4> tinctureCategoryTotals{};
		std::array<std::array<Uint32, 2>, 4> tinctureResourceTotals{};
		for ( int row = 4; row < 10; ++row )
		{
			std::array<int, 4> categoryTotals{};
			int total = 0;
			for ( int category = 0; category < 4; ++category )
			{
				if ( book.symbolicRecipes[row][category][2] != 0
					|| book.symbolicRecipes[row][category][3] != 0 )
				{
					return stewingValidationFailure("Tincture uses C/D");
				}
				for ( int symbol = 0; symbol < 2; ++symbol )
				{
					const Uint32 quantity = book.symbolicRecipes[row][category][symbol];
					categoryTotals[category] += quantity;
					tinctureResourceTotals[category][symbol] += quantity;
					tinctureCategoryTotals[category] += quantity;
					total += quantity;
				}
				if ( categoryTotals[category] == 2
					&& (book.symbolicRecipes[row][category][0] != 1
						|| book.symbolicRecipes[row][category][1] != 1) )
				{
					return stewingValidationFailure("Tincture doubled category is not A+B");
				}
			}
			std::sort(categoryTotals.begin(), categoryTotals.end());
			if ( total != 5 || categoryTotals != std::array<int, 4>{ { 1, 1, 1, 2 } } )
			{
				return stewingValidationFailure("Tincture row cost/category multiplicity");
			}
		}
		auto sortedTinctureCategories = tinctureCategoryTotals;
		std::sort(sortedTinctureCategories.begin(), sortedTinctureCategories.end());
		if ( sortedTinctureCategories != std::array<Uint32, 4>{ { 7, 7, 8, 8 } } )
		{
			return stewingValidationFailure("Tincture global category totals");
		}
		int tinctureThrees = 0;
		std::array<bool, 4> categoryHasThree{};
		for ( int category = 0; category < 4; ++category )
		{
			if ( (tinctureCategoryTotals[category] == 8) != book.highTinctureCategories[category] )
			{
				return stewingValidationFailure("Tincture high-category identity");
			}
			for ( int symbol = 0; symbol < 2; ++symbol )
			{
				const Uint32 total = tinctureResourceTotals[category][symbol];
				if ( total == 3 ) { ++tinctureThrees; categoryHasThree[category] = true; }
				else if ( total != 4 ) { return stewingValidationFailure("Tincture A/B total is not 3 or 4"); }
			}
		}
		int categoriesWithThree = 0;
		for ( bool hasThree : categoryHasThree ) { categoriesWithThree += hasThree ? 1 : 0; }
		if ( tinctureThrees != 2 || categoriesWithThree != 2 )
		{
			return stewingValidationFailure("Tincture two 3-use resources in distinct categories");
		}

		std::array<std::array<Uint32, 2>, 4> oilResourceTotals{};
		for ( int row = 10; row < 17; ++row )
		{
			std::array<int, 4> categoryTotals{};
			int total = 0;
			for ( int category = 0; category < 4; ++category )
			{
				if ( book.symbolicRecipes[row][category][0] != 0
					|| book.symbolicRecipes[row][category][1] != 0 )
				{
					return stewingValidationFailure("Oil uses A/B");
				}
				for ( int symbol = 0; symbol < 2; ++symbol )
				{
					const Uint32 quantity = book.symbolicRecipes[row][category][2 + symbol];
					categoryTotals[category] += quantity;
					oilResourceTotals[category][symbol] += quantity;
					total += quantity;
				}
				if ( categoryTotals[category] >= 2
					&& (book.symbolicRecipes[row][category][2] == 0
						|| book.symbolicRecipes[row][category][3] == 0) )
				{
					return stewingValidationFailure("Oil category >=2 lacks C or D");
				}
			}
			std::sort(categoryTotals.begin(), categoryTotals.end());
			if ( total != 8 || categoryTotals != std::array<int, 4>{ { 1, 2, 2, 3 } } )
			{
				return stewingValidationFailure("Oil row cost/category multiplicity");
			}
		}
		for ( const auto& category : oilResourceTotals )
		{
			if ( category[0] != 7 || category[1] != 7 )
			{
				return stewingValidationFailure("Oil global C/D totals");
			}
		}

		std::array<std::array<Uint32, 2>, 4> inkResourceTotals{};
		for ( int row = 17; row < 20; ++row )
		{
			int total = 0;
			for ( int category = 0; category < 4; ++category )
			{
				if ( book.symbolicRecipes[row][category][0] != 0
					|| book.symbolicRecipes[row][category][1] != 0
					|| book.symbolicRecipes[row][category][2] < 2
					|| book.symbolicRecipes[row][category][3] < 2 )
				{
					return stewingValidationFailure("Ink row baseline/C-D-only invariant");
				}
				for ( int symbol = 0; symbol < 2; ++symbol )
				{
					const Uint32 quantity = book.symbolicRecipes[row][category][2 + symbol];
					inkResourceTotals[category][symbol] += quantity;
					total += quantity;
				}
			}
			if ( total != 18 ) { return stewingValidationFailure("Ink row cost"); }
		}
		int inkSixes = 0;
		std::array<bool, 4> inkSixCategories{};
		for ( int category = 0; category < 4; ++category )
		{
			for ( int symbol = 0; symbol < 2; ++symbol )
			{
				const Uint32 total = inkResourceTotals[category][symbol];
				if ( total == 6 ) { ++inkSixes; inkSixCategories[category] = true; }
				else if ( total != 7 ) { return stewingValidationFailure("Ink global C/D total is not 6 or 7"); }
			}
		}
		int inkSixCategoryCount = 0;
		for ( int category = 0; category < 4; ++category )
		{
			inkSixCategoryCount += inkSixCategories[category] ? 1 : 0;
			if ( inkSixCategories[category] != book.highTinctureCategories[category] )
			{
				return stewingValidationFailure("Ink 6-use category does not match high Tincture category");
			}
		}
		if ( inkSixes != 2 || inkSixCategoryCount != 2 )
		{
			return stewingValidationFailure("Ink two 6-use resources in distinct categories");
		}

		if ( wholeCategoryTotals != std::array<Uint32, 4>{ { 39, 39, 39, 39 } } )
		{
			return stewingValidationFailure("whole-book category totals are not 39/39/39/39");
		}
		return true;
	}

	bool buildStewingRecipeBook(GeneratedStewingRecipeBook& book, Uint32 gameKey, Uint32 generatorVersion)
	{
		book = GeneratedStewingRecipeBook{};
		BaronyRNG stewingRng;
		const Uint32 seedWords[] = { gameKey, 0x53544557u, generatorVersion };
		stewingRng.seedBytes(seedWords, sizeof(seedWords));
		book.categoryMappings = generateStewingCategoryMappings(stewingRng);

		for ( int potion = 0; potion < 4; ++potion )
		{
			for ( int symbol = 0; symbol < 4; ++symbol )
			{
				book.symbolicRecipes[potion][potion][symbol] = 1;
			}
		}
		if ( !generateTinctureRecipes(book, stewingRng) )
		{
			printlog("[Stewing] Tincture exhaustive solver failed");
			return false;
		}
		if ( !generateOilRecipes(book, stewingRng) )
		{
			printlog("[Stewing] Oil exhaustive solver failed");
			return false;
		}
		generateInkRecipes(book, stewingRng);

		static const std::array<ItemType, STEWING_RECIPE_COUNT> results{ {
			POTION_ACID, POTION_BOOZE, POTION_JUICE, POTION_INVISIBILITY,
			TINCTURE_STR, TINCTURE_DEX, TINCTURE_CON, TINCTURE_INT, TINCTURE_PER, TINCTURE_CHA,
			OIL_AETHER, OIL_SILVER, OIL_TOXINS, OIL_BASES, OIL_LEAD, OIL_MINERAL, OIL_BRINE,
			INK_MYSTICISM, INK_SORCERY, INK_THAUMATURGY
		} };
		for ( int row = 0; row < STEWING_RECIPE_COUNT; ++row )
		{
			book.recipes[row] = symbolicRecipeToConcrete(book.symbolicRecipes[row],
				book.categoryMappings, results[row], row < 4);
		}
		return validateStewingRecipeBook(book);
	}

	std::array<Uint64, STEWING_RESOURCE_COUNT> stewingSelectionAmounts(const StewingSelection& selection,
		bool* invalidResource = nullptr)
	{
		std::array<Uint64, STEWING_RESOURCE_COUNT> amounts{};
		bool invalid = false;
		for ( const StewingEduct& educt : selection )
		{
			if ( educt.quantity == 0 ) { continue; }
			const int index = getStewingResourceIndex(educt.type);
			if ( index < 0 || educt.type == POTION_EMPTY )
			{
				invalid = true;
				continue;
			}
			amounts[index] += educt.quantity;
			if ( amounts[index] > std::numeric_limits<Uint32>::max() )
			{
				invalid = true;
			}
		}
		if ( invalidResource ) { *invalidResource = invalid; }
		return amounts;
	}

	StewingSelection stewingSelectionFromAmounts(
		const std::array<Uint64, STEWING_RESOURCE_COUNT>& amounts)
	{
		StewingSelection selection{};
		int slot = 0;
		for ( int i = 0; i < STEWING_RESOURCE_COUNT && slot < STEWING_MAX_EDUCTS; ++i )
		{
			if ( amounts[i] == 0 || getStewingResourceType(i) == POTION_EMPTY ) { continue; }
			selection[slot].type = getStewingResourceType(i);
			selection[slot].quantity = static_cast<Uint32>(amounts[i]);
			++slot;
		}
		return selection;
	}

	Uint64 countStewingResourceInList(const list_t& inventory, ItemType resource)
	{
		Uint64 total = 0;
		if ( itemTypeIsWitchIngredient(resource) )
		{
			const WitchIngredientCategory category = ingredientCategoryFromItem(resource);
			const int ingredientSlot = ingredientSlotForItem(resource);
			for ( node_t* node = inventory.first; node; node = node->next )
			{
				const Item* item = static_cast<const Item*>(node->element);
				if ( item && ingredientCategoryFromBag(item->type) == category )
				{
					total += item->ingredientCounts[ingredientSlot];
				}
			}
		}
		else
		{
			for ( node_t* node = inventory.first; node; node = node->next )
			{
				const Item* item = static_cast<const Item*>(node->element);
				if ( item && item->type == resource && item->count > 0 )
				{
					total += static_cast<Uint32>(item->count);
				}
			}
		}
		return total;
	}

	Uint32 consumeStewingIngredientFromList(list_t& inventory, ItemType ingredient, Uint32 amount)
	{
		const WitchIngredientCategory category = ingredientCategoryFromItem(ingredient);
		const int ingredientSlot = ingredientSlotForItem(ingredient);
		Uint32 consumed = 0;
		for ( node_t* node = inventory.first; node && consumed < amount; )
		{
			node_t* next = node->next;
			Item* bag = static_cast<Item*>(node->element);
			if ( bag && ingredientCategoryFromBag(bag->type) == category )
			{
				const Uint32 take = std::min(amount - consumed, bag->ingredientCounts[ingredientSlot]);
				bag->ingredientCounts[ingredientSlot] -= take;
				consumed += take;
				if ( ingredientBagIsEmpty(*bag) )
				{
					list_RemoveNode(node);
				}
			}
			node = next;
		}
		return consumed;
	}

	Uint32 consumeStewingOrdinaryItemFromList(list_t& inventory, ItemType type, Uint32 amount)
	{
		Uint32 consumed = 0;
		for ( node_t* node = inventory.first; node && consumed < amount; )
		{
			node_t* next = node->next;
			Item* item = static_cast<Item*>(node->element);
			if ( item && item->type == type && item->count > 0 )
			{
				const Uint32 take = std::min<Uint32>(amount - consumed,
					static_cast<Uint32>(item->count));
				item->count -= static_cast<Sint16>(take);
				consumed += take;
				if ( item->count <= 0 )
				{
					list_RemoveNode(node);
				}
			}
			node = next;
		}
		return consumed;
	}

	void giveStewingItem(int player, ItemType type, Uint64 amount)
	{
		const int resourceIndex = getStewingResourceIndex(type);
		const Uint64 originalAmount = amount;
		while ( amount > 0 )
		{
			const Sint16 count = static_cast<Sint16>(std::min<Uint64>(amount,
				static_cast<Uint64>(std::numeric_limits<Sint16>::max())));
			Item* item = newItem(type, SERVICABLE, 0, count, 0, true, nullptr);
			itemPickup(player, item);
			free(item);
			amount -= count;
		}
		if ( multiplayer == SERVER && player > 0 && player < MAXPLAYERS
			&& players[player] && !players[player]->isLocalPlayer()
			&& serverStewingInventoryMirrorValid[player] && resourceIndex >= 0 )
		{
			const Uint64 updated = static_cast<Uint64>(serverStewingInventoryMirrors[player][resourceIndex])
				+ originalAmount;
			serverStewingInventoryMirrors[player][resourceIndex] = static_cast<Uint32>(
				std::min<Uint64>(updated, std::numeric_limits<Uint32>::max()));
		}
	}

	bool playerCanApplyStewingResult(int player)
	{
		if ( player < 0 || player >= MAXPLAYERS || !players[player] || !players[player]->entity
			|| !stats[player] || playerIsPanicking(player) )
		{
			return false;
		}
		if ( stats[player]->amulet && stats[player]->amulet->type == AMULET_STRANGULATION
			&& stats[player]->type != SKELETON )
		{
			return false;
		}
		return !stats[player]->getEffectActive(EFF_VOMITING);
	}

	bool applyStewingResult(int player, ItemType result)
	{
		if ( !playerCanApplyStewingResult(player) ) { return false; }
		Item* item = newItem(result, SERVICABLE, 0, 1, 0, true, nullptr);
		Entity* entity = players[player]->entity;
		bool applied = false;
		switch ( result )
		{
			case POTION_ACID: applied = item_PotionAcid(item, entity, entity); break;
			case POTION_BOOZE: applied = item_PotionBooze(item, entity, entity); break;
			case POTION_JUICE: applied = item_PotionJuice(item, entity, entity); break;
			case POTION_INVISIBILITY: applied = item_PotionInvisibility(item, entity, entity); break;
			case POTION_SICKNESS: applied = item_PotionSickness(item, entity, entity); break;
			default: break;
		}
		if ( item ) { free(item); }
		return applied;
	}
}

const std::array<StewingRecipe, STEWING_RECIPE_COUNT>& getStewingRecipes()
{
	static bool initialized = false;
	static Uint32 cachedGameKey = 0;
	static Uint32 cachedGeneratorVersion = 0;
	static GeneratedStewingRecipeBook book;
	const Uint32 generatorVersion = getActiveStewingGeneratorVersion();
	if ( !initialized || cachedGameKey != uniqueGameKey
		|| cachedGeneratorVersion != generatorVersion )
	{
		const bool success = buildStewingRecipeBook(book, uniqueGameKey, generatorVersion);
		if ( !success )
		{
			printlog("[Stewing] failed to build recipe book for key %u, version %u",
				uniqueGameKey, generatorVersion);
			assert(success);
		}
		cachedGameKey = uniqueGameKey;
		cachedGeneratorVersion = generatorVersion;
		initialized = true;
	}
	return book.recipes;
}

Uint32 getActiveStewingGeneratorVersion()
{
	return activeStewingGeneratorVersion;
}

void setActiveStewingGeneratorVersion(Uint32 version)
{
	activeStewingGeneratorVersion = version == 0 ? STEWING_GENERATOR_VERSION : version;
}

Uint32 getStewingRecipeUnlockMask(int player)
{
	return player >= 0 && player < MAXPLAYERS ? learnedStewingRecipeMasks[player] : 0;
}

void setStewingRecipeUnlockMask(int player, Uint32 mask)
{
	if ( player < 0 || player >= MAXPLAYERS ) { return; }
	learnedStewingRecipeMasks[player] = mask & 0xffffu;
}

bool playerKnowsStewingRecipe(int player, ItemType result)
{
	if ( player < 0 || player >= MAXPLAYERS ) { return false; }
	for ( const StewingRecipe& recipe : getStewingRecipes() )
	{
		if ( recipe.result == result && recipe.alwaysUnlocked ) { return true; }
	}
	const int bit = static_cast<int>(result) - static_cast<int>(TINCTURE_STR);
	return bit >= 0 && bit < 16
		&& (learnedStewingRecipeMasks[player] & (1u << bit)) != 0;
}

bool unlockStewingRecipe(int player, ItemType result)
{
	if ( player < 0 || player >= MAXPLAYERS ) { return false; }
	const int bit = static_cast<int>(result) - static_cast<int>(TINCTURE_STR);
	if ( bit < 0 || bit >= 16 ) { return false; }
	const Uint32 flag = 1u << bit;
	if ( (learnedStewingRecipeMasks[player] & flag) != 0 ) { return false; }
	learnedStewingRecipeMasks[player] |= flag;
	return true;
}

void unlockAllStewingRecipes(int player)
{
	setStewingRecipeUnlockMask(player, 0xffffu);
}

int getNumKnownStewingRecipes(int player)
{
	if ( player < 0 || player >= MAXPLAYERS ) { return 0; }
	Uint32 mask = learnedStewingRecipeMasks[player];
	int count = 4;
	while ( mask != 0 )
	{
		count += mask & 1u;
		mask >>= 1;
	}
	return count;
}

ItemType tryDiscoverStewingRecipe(int player,
	const std::array<Uint64, STEWING_RESOURCE_COUNT>& consumedResources)
{
	if ( multiplayer == CLIENT || player < 0 || player >= MAXPLAYERS || !stats[player] )
	{
		return ITEM_ENUM_MAX;
	}

	std::vector<ItemType> eligibleTinctures;
	std::vector<ItemType> eligibleAdvanced;
	for ( const StewingRecipe& recipe : getStewingRecipes() )
	{
		if ( recipe.alwaysUnlocked || playerKnowsStewingRecipe(player, recipe.result) )
		{
			continue;
		}
		bool sharesUsedIngredient = false;
		for ( Uint32 i = 0; i < recipe.eductCount; ++i )
		{
			const int resourceIndex = getStewingResourceIndex(recipe.educts[i].type);
			if ( resourceIndex >= 0 && resourceIndex < 16
				&& consumedResources[resourceIndex] > 0 )
			{
				sharesUsedIngredient = true;
				break;
			}
		}
		if ( !sharesUsedIngredient )
		{
			continue;
		}
		if ( itemTypeIsTincture(recipe.result) )
		{
			eligibleTinctures.push_back(recipe.result);
		}
		else if ( itemTypeIsOil(recipe.result) || itemTypeIsInk(recipe.result) )
		{
			eligibleAdvanced.push_back(recipe.result);
		}
	}

	if ( eligibleTinctures.empty() && eligibleAdvanced.empty() )
	{
		return ITEM_ENUM_MAX;
	}
	const int chance = std::clamp(stats[player]->getProficiency(PRO_ALCHEMY), 0, 100);
	if ( local_rng.rand() % 100 >= chance )
	{
		return ITEM_ENUM_MAX;
	}

	const std::vector<ItemType>* selectedGroup = nullptr;
	if ( eligibleTinctures.empty() )
	{
		selectedGroup = &eligibleAdvanced;
	}
	else if ( eligibleAdvanced.empty() )
	{
		selectedGroup = &eligibleTinctures;
	}
	else
	{
		selectedGroup = local_rng.rand() % 100 < 80
			? &eligibleTinctures : &eligibleAdvanced;
	}
	const ItemType candidate = (*selectedGroup)[local_rng.rand() % selectedGroup->size()];
	if ( unlockStewingRecipe(player, candidate) )
	{
		if ( players[player] && players[player]->entity )
		{
			players[player]->entity->increaseSkill(PRO_ALCHEMY);
		}
		return candidate;
	}
	return ITEM_ENUM_MAX;
}

void tryIncreaseAlchemyFromWitchbrewUse(int player)
{
	if ( multiplayer == CLIENT || player < 0 || player >= MAXPLAYERS || !players[player]
		|| !players[player]->entity || !stats[player] )
	{
		return;
	}
	if ( local_rng.rand() % 6 == 0 )
	{
		players[player]->entity->increaseSkill(PRO_ALCHEMY);
	}
}

bool stewingResultCanApplyImmediately(ItemType result)
{
	switch ( result )
	{
		case POTION_ACID:
		case POTION_BOOZE:
		case POTION_JUICE:
		case POTION_INVISIBILITY:
		case POTION_SICKNESS:
			return true;
		default:
			return false;
	}
}

bool itemTypeIsStewingSelectable(ItemType type)
{
	return itemTypeIsWitchIngredient(type) || type == POTION_WATER;
}

int getStewingResourceIndex(ItemType type)
{
	if ( itemTypeIsWitchIngredient(type) )
	{
		return static_cast<int>(type) - static_cast<int>(INGREDIENT_GELATIN);
	}
	if ( type == POTION_WATER ) { return 16; }
	if ( type == POTION_EMPTY ) { return 17; }
	return -1;
}

ItemType getStewingResourceType(int index)
{
	if ( index >= 0 && index < 16 )
	{
		return static_cast<ItemType>(static_cast<int>(INGREDIENT_GELATIN) + index);
	}
	if ( index == 16 ) { return POTION_WATER; }
	if ( index == 17 ) { return POTION_EMPTY; }
	return ITEM_ENUM_MAX;
}

int getBottleContribution(ItemType educt)
{
	return educt == POTION_WATER ? 1 : 0;
}

Uint32 countSelectedStewingResource(const StewingSelection& selection, ItemType type)
{
	Uint64 total = 0;
	for ( const StewingEduct& educt : selection )
	{
		if ( educt.type == type ) { total += educt.quantity; }
	}
	return static_cast<Uint32>(std::min<Uint64>(total, std::numeric_limits<Uint32>::max()));
}

Uint32 countSelectedWitchIngredients(const StewingSelection& selection)
{
	Uint64 total = 0;
	for ( const StewingEduct& educt : selection )
	{
		if ( itemTypeIsWitchIngredient(educt.type) ) { total += educt.quantity; }
	}
	return static_cast<Uint32>(std::min<Uint64>(total, std::numeric_limits<Uint32>::max()));
}

StewingMatch evaluateStewingMixture(const StewingSelection& selection)
{
	StewingMatch match;
	bool invalidResource = false;
	const auto selected = stewingSelectionAmounts(selection, &invalidResource);
	Uint64 total = 0;
	Uint64 witchIngredients = 0;
	for ( int i = 0; i < STEWING_RESOURCE_COUNT; ++i )
	{
		total += selected[i];
		if ( i < 16 ) { witchIngredients += selected[i]; }
	}
	if ( invalidResource )
	{
		match.type = STEWING_MATCH_INVALID_RESOURCE;
		return match;
	}
	if ( total == 0 )
	{
		match.type = STEWING_MATCH_EMPTY;
		return match;
	}
	if ( witchIngredients < 4 )
	{
		match.type = STEWING_MATCH_TOO_SMALL;
		return match;
	}

	const auto& recipes = getStewingRecipes();
	for ( int recipeIndex = 0; recipeIndex < STEWING_RECIPE_COUNT; ++recipeIndex )
	{
		const StewingRecipe& recipe = recipes[recipeIndex];
		std::array<Uint64, STEWING_RESOURCE_COUNT> required{};
		for ( Uint32 i = 0; i < recipe.eductCount; ++i )
		{
			const int index = getStewingResourceIndex(recipe.educts[i].type);
			if ( index >= 0 ) { required[index] = recipe.educts[i].quantity; }
		}
		Uint64 multiplier = 0;
		bool valid = true;
		for ( int i = 0; i < STEWING_RESOURCE_COUNT; ++i )
		{
			if ( required[i] == 0 )
			{
				if ( selected[i] != 0 ) { valid = false; break; }
				continue;
			}
			if ( selected[i] == 0 || selected[i] % required[i] != 0 )
			{
				valid = false;
				break;
			}
			const Uint64 ratio = selected[i] / required[i];
			if ( ratio == 0 || (multiplier != 0 && ratio != multiplier) )
			{
				valid = false;
				break;
			}
			multiplier = ratio;
		}
		if ( valid && multiplier >= 1 && multiplier <= std::numeric_limits<Uint32>::max() )
		{
			match.type = STEWING_MATCH_VALID_RECIPE;
			match.recipeIndex = recipeIndex;
			match.multiplier = static_cast<Uint32>(multiplier);
			match.result = recipe.result;
			return match;
		}
	}
	match.type = STEWING_MATCH_INVALID_POISON;
	match.multiplier = 1;
	match.result = POTION_SICKNESS;
	return match;
}

std::array<Uint32, STEWING_RESOURCE_COUNT> getStewingInventoryResourceSnapshot(int player)
{
	std::array<Uint32, STEWING_RESOURCE_COUNT> resources{};
	if ( player < 0 || player >= MAXPLAYERS || !stats[player] ) { return resources; }
	for ( int i = 0; i < STEWING_RESOURCE_COUNT; ++i )
	{
		resources[i] = static_cast<Uint32>(std::min<Uint64>(
			countStewingResourceInList(stats[player]->inventory, getStewingResourceType(i)),
			std::numeric_limits<Uint32>::max()));
	}
	return resources;
}

void setServerStewingInventoryMirror(int player,
	const std::array<Uint32, STEWING_RESOURCE_COUNT>& resources)
{
	if ( player <= 0 || player >= MAXPLAYERS ) { return; }
	serverStewingInventoryMirrors[player] = resources;
	serverStewingInventoryMirrorValid[player] = true;
}

void clearServerStewingInventoryMirror(int player)
{
	if ( player < 0 || player >= MAXPLAYERS ) { return; }
	serverStewingInventoryMirrors[player].fill(0);
	serverStewingInventoryMirrorValid[player] = false;
}

bool serverStewingInventoryMirrorIsValid(int player)
{
	return player >= 0 && player < MAXPLAYERS && serverStewingInventoryMirrorValid[player];
}

Uint32 getTotalStewingResource(int player, ItemType resource)
{
	const int index = getStewingResourceIndex(resource);
	if ( index < 0 || player < 0 || player >= MAXPLAYERS || !stats[player] || !stats[0] ) { return 0; }
	Uint64 total = 0;
	if ( multiplayer == SERVER && player > 0 && !players[player]->isLocalPlayer()
		&& serverStewingInventoryMirrorValid[player] )
	{
		total = serverStewingInventoryMirrors[player][index];
	}
	else
	{
		total = countStewingResourceInList(stats[player]->inventory, resource);
	}
	total += countStewingResourceInList(stats[0]->void_chest_inventory, resource);
	return static_cast<Uint32>(std::min<Uint64>(total, std::numeric_limits<Uint32>::max()));
}

Uint32 getAvailableStewingIngredient(int player, ItemType ingredient,
	const StewingSelection& reserved)
{
	const Uint32 total = getTotalStewingResource(player, ingredient);
	const Uint32 selected = countSelectedStewingResource(reserved, ingredient);
	return selected >= total ? 0 : total - selected;
}

std::array<Uint32, STEWING_RESOURCE_COUNT> getStewingResourceSnapshot(int player)
{
	std::array<Uint32, STEWING_RESOURCE_COUNT> resources{};
	for ( int i = 0; i < STEWING_RESOURCE_COUNT; ++i )
	{
		resources[i] = getTotalStewingResource(player, getStewingResourceType(i));
	}
	return resources;
}

void consumeClientStewingInventoryResources(int player,
	const std::array<Uint32, STEWING_RESOURCE_COUNT>& resources)
{
	if ( player < 0 || player >= MAXPLAYERS || !stats[player] ) { return; }
	for ( int i = 0; i < STEWING_RESOURCE_COUNT; ++i )
	{
		if ( resources[i] == 0 ) { continue; }
		const ItemType type = getStewingResourceType(i);
		if ( itemTypeIsWitchIngredient(type) )
		{
			consumeStewingIngredientFromList(stats[player]->inventory, type, resources[i]);
		}
		else
		{
			consumeStewingOrdinaryItemFromList(stats[player]->inventory, type, resources[i]);
		}
	}
}

bool performStewingTransaction(int player, Entity* familiar, StewingOperation operation,
	const StewingSelection& selection, StewingTransactionResult& result)
{
	result = StewingTransactionResult{};
	result.remainingSelection = selection;
	if ( multiplayer == CLIENT || player < 0 || player >= MAXPLAYERS || !players[player]
		|| !players[player]->entity || !stats[player] || !stats[0] || !familiar
		|| !isAmuletFamiliar(familiar, familiar->getStats())
		|| getAmuletFamiliarForm(familiar) != FAMILIAR_FORM_CAULDRON
		|| entityDist(players[player]->entity, familiar) > TOUCHRANGE )
	{
		return false;
	}
	Stat* familiarStats = familiar->getStats();
	if ( !familiarStats || familiarStats->leader_uid != players[player]->entity->getUID()
		|| !stats[player]->amulet || stats[player]->amulet->type != AMULET_FAMILIAR )
	{
		return false;
	}
	const bool remotePlayer = multiplayer == SERVER && player > 0
		&& !players[player]->isLocalPlayer();
	if ( remotePlayer && !serverStewingInventoryMirrorValid[player] ) { return false; }

	const StewingMatch match = evaluateStewingMixture(selection);
	if ( match.type != STEWING_MATCH_VALID_RECIPE && match.type != STEWING_MATCH_INVALID_POISON )
	{
		return false;
	}
	if ( operation != STEWING_OPERATION_BREW_INTO && operation != STEWING_OPERATION_APPLY )
	{
		return false;
	}
	if ( operation == STEWING_OPERATION_APPLY
		&& !stewingResultCanApplyImmediately(match.result) )
	{
		return false;
	}
	if ( operation == STEWING_OPERATION_APPLY && !playerCanApplyStewingResult(player) )
	{
		return false;
	}

	std::array<Uint64, STEWING_RESOURCE_COUNT> consumeAmounts{};
	if ( match.type == STEWING_MATCH_VALID_RECIPE )
	{
		const StewingRecipe& recipe = getStewingRecipes()[match.recipeIndex];
		for ( Uint32 i = 0; i < recipe.eductCount; ++i )
		{
			const int index = getStewingResourceIndex(recipe.educts[i].type);
			consumeAmounts[index] += recipe.educts[i].quantity;
		}
	}
	else
	{
		consumeAmounts = stewingSelectionAmounts(selection);
	}

	Uint64 contributedBottles = 0;
	for ( int i = 0; i < STEWING_RESOURCE_COUNT; ++i )
	{
		if ( consumeAmounts[i] > getTotalStewingResource(player, getStewingResourceType(i)) )
		{
			return false;
		}
		contributedBottles += consumeAmounts[i]
			* static_cast<Uint64>(getBottleContribution(getStewingResourceType(i)));
	}
	const Uint32 requiredEmptyBottles = operation == STEWING_OPERATION_BREW_INTO
		&& contributedBottles == 0 ? 1 : 0;
	if ( requiredEmptyBottles > getTotalStewingResource(player, POTION_EMPTY) )
	{
		return false;
	}
	consumeAmounts[getStewingResourceIndex(POTION_EMPTY)] += requiredEmptyBottles;

	for ( int i = 0; i < STEWING_RESOURCE_COUNT; ++i )
	{
		Uint32 amount = static_cast<Uint32>(consumeAmounts[i]);
		if ( amount == 0 ) { continue; }
		const ItemType type = getStewingResourceType(i);
		Uint32 consumedFromInventory = 0;
		if ( remotePlayer )
		{
			consumedFromInventory = std::min(amount, serverStewingInventoryMirrors[player][i]);
			serverStewingInventoryMirrors[player][i] -= consumedFromInventory;
		}
		else if ( itemTypeIsWitchIngredient(type) )
		{
			consumedFromInventory = consumeStewingIngredientFromList(
				stats[player]->inventory, type, amount);
		}
		else
		{
			consumedFromInventory = consumeStewingOrdinaryItemFromList(
				stats[player]->inventory, type, amount);
		}
		result.inventoryConsumed[i] = consumedFromInventory;
		const Uint32 remaining = amount - consumedFromInventory;
		if ( remaining > 0 )
		{
			const Uint32 consumedFromVoid = itemTypeIsWitchIngredient(type)
				? consumeStewingIngredientFromList(stats[0]->void_chest_inventory, type, remaining)
				: consumeStewingOrdinaryItemFromList(stats[0]->void_chest_inventory, type, remaining);
			if ( consumedFromVoid != remaining )
			{
				return false; // Prevalidation makes this unreachable in the authoritative tick.
			}
		}
	}

	const Uint64 emptyBottleReturns = operation == STEWING_OPERATION_APPLY
		? contributedBottles : (contributedBottles > 0 ? contributedBottles - 1 : 0);
	if ( emptyBottleReturns > 0 )
	{
		giveStewingItem(player, POTION_EMPTY, emptyBottleReturns);
	}
	if ( operation == STEWING_OPERATION_BREW_INTO )
	{
		giveStewingItem(player, match.result, 1);
	}
	else if ( !applyStewingResult(player, match.result) )
	{
		return false;
	}
	if ( match.type == STEWING_MATCH_VALID_RECIPE )
	{
		auto remaining = stewingSelectionAmounts(selection);
		const StewingRecipe& recipe = getStewingRecipes()[match.recipeIndex];
		for ( Uint32 i = 0; i < recipe.eductCount; ++i )
		{
			remaining[getStewingResourceIndex(recipe.educts[i].type)] -= recipe.educts[i].quantity;
		}
		result.remainingSelection = stewingSelectionFromAmounts(remaining);
	}
	else
	{
		result.remainingSelection = StewingSelection{};
	}
	result.success = true;
	playSoundEntity(familiar, SFX_STEW_BUBBLES, 96);
	spawnAmuletFamiliarTransformParticles(familiar);
	result.newlyUnlockedRecipe = tryDiscoverStewingRecipe(player, consumeAmounts);
	if ( match.type == STEWING_MATCH_VALID_RECIPE )
	{
		if ( local_rng.rand() % 2 == 0 )
		{
			players[player]->entity->increaseSkill(PRO_ALCHEMY);
		}
	}
	else
	{
		const bool raiseSkill = local_rng.rand() % 10 == 0;
		if ( raiseSkill && local_rng.rand() % 2 == 0 )
		{
			players[player]->entity->increaseSkill(PRO_ALCHEMY);
		}
	}
	result.resources = getStewingResourceSnapshot(player);
	return true;
}

void writeIngredientCountsToPacket(const Item& item, Uint8* data, int offset)
{
	if ( !data ) { return; }
	for ( int i = 0; i < 4; ++i )
	{
		SDLNet_Write32(item.ingredientCounts[i], &data[offset + i * 4]);
	}
}

void readIngredientCountsFromPacket(Item& item, const Uint8* data, int packetLength, int offset)
{
	item.ingredientCounts[0] = 0;
	item.ingredientCounts[1] = 0;
	item.ingredientCounts[2] = 0;
	item.ingredientCounts[3] = 0;
	if ( !data || offset < 0 ) { return; }
	for ( int i = 0; i < 4; ++i )
	{
		const int fieldEnd = offset + (i + 1) * static_cast<int>(sizeof(Uint32));
		if ( packetLength >= fieldEnd )
		{
			item.ingredientCounts[i] = SDLNet_Read32(&data[offset + i * sizeof(Uint32)]);
		}
	}
}

void writeFamiliarPersistentStateToPacket(const Item& item, Uint8* data, int offset)
{
	if ( !data ) { return; }
	const Uint32 fields[14] = {
		item.familiarInitialized ? 1u : 0u, item.familiarInstanceId,
		static_cast<Uint32>(item.familiarHP), static_cast<Uint32>(item.familiarMAXHP),
		static_cast<Uint32>(item.familiarMP), static_cast<Uint32>(item.familiarMAXMP),
		static_cast<Uint32>(item.familiarLVL), static_cast<Uint32>(item.familiarEXP),
		static_cast<Uint32>(item.familiarSTR), static_cast<Uint32>(item.familiarDEX),
		static_cast<Uint32>(item.familiarCON), static_cast<Uint32>(item.familiarINT),
		static_cast<Uint32>(item.familiarPER), static_cast<Uint32>(item.familiarCHR)
	};
	for ( int i = 0; i < 14; ++i )
	{
		SDLNet_Write32(fields[i], &data[offset + i * sizeof(Uint32)]);
	}
}

void readFamiliarPersistentStateFromPacket(Item& item, const Uint8* data, int packetLength, int offset)
{
	item.familiarInitialized = false;
	item.familiarInstanceId = 0;
	item.familiarHP = item.familiarMAXHP = item.familiarMP = item.familiarMAXMP = 0;
	item.familiarLVL = item.familiarEXP = 0;
	item.familiarSTR = item.familiarDEX = item.familiarCON = 0;
	item.familiarINT = item.familiarPER = item.familiarCHR = 0;
	if ( !data || offset < 0 || packetLength < offset + FAMILIAR_PERSISTENT_PACKET_BYTES ) { return; }
	Uint32 fields[14];
	for ( int i = 0; i < 14; ++i )
	{
		fields[i] = SDLNet_Read32(&data[offset + i * sizeof(Uint32)]);
	}
	item.familiarInitialized = fields[0] != 0;
	item.familiarInstanceId = fields[1];
	item.familiarHP = static_cast<Sint32>(fields[2]);
	item.familiarMAXHP = static_cast<Sint32>(fields[3]);
	item.familiarMP = static_cast<Sint32>(fields[4]);
	item.familiarMAXMP = static_cast<Sint32>(fields[5]);
	item.familiarLVL = static_cast<Sint32>(fields[6]);
	item.familiarEXP = static_cast<Sint32>(fields[7]);
	item.familiarSTR = static_cast<Sint32>(fields[8]);
	item.familiarDEX = static_cast<Sint32>(fields[9]);
	item.familiarCON = static_cast<Sint32>(fields[10]);
	item.familiarINT = static_cast<Sint32>(fields[11]);
	item.familiarPER = static_cast<Sint32>(fields[12]);
	item.familiarCHR = static_cast<Sint32>(fields[13]);
	reserveFamiliarInstanceId(item.familiarInstanceId);
}

AppliedOil appliedOilFromItemType(ItemType type)
{
	switch ( type )
	{
		case OIL_AETHER: return APPLIED_OIL_AETHER;
		case OIL_SILVER: return APPLIED_OIL_SILVER;
		case OIL_TOXINS: return APPLIED_OIL_TOXINS;
		case OIL_BASES: return APPLIED_OIL_BASES;
		case OIL_LEAD: return APPLIED_OIL_LEAD;
		case OIL_MINERAL: return APPLIED_OIL_MINERAL;
		case OIL_BRINE: return APPLIED_OIL_BRINE;
		default: return APPLIED_OIL_NONE;
	}
}

const char* getOilIconPath(AppliedOil oil)
{
	static const char* paths[] = {
		"", "images/ui/Inventory/oils/aether.png", "images/ui/Inventory/oils/silver.png",
		"images/ui/Inventory/oils/toxins.png", "images/ui/Inventory/oils/bases.png",
		"images/ui/Inventory/oils/lead.png", "images/ui/Inventory/oils/mineral.png",
		"images/ui/Inventory/oils/brine.png"
	};
	return oil > APPLIED_OIL_NONE && oil < APPLIED_OIL_MAX ? paths[oil] : "";
}

const char* getTinctureIconPath(ItemType type)
{
	static const char* paths[] = {
		"images/ui/Inventory/tinctures/tincture_STR.png",
		"images/ui/Inventory/tinctures/tincture_DEX.png",
		"images/ui/Inventory/tinctures/tincture_CON.png",
		"images/ui/Inventory/tinctures/tincture_INT.png",
		"images/ui/Inventory/tinctures/tincture_PER.png",
		"images/ui/Inventory/tinctures/tincture_CHA.png"
	};
	return itemTypeIsTincture(type) ? paths[type - TINCTURE_STR] : "";
}

bool itemTypeCanReceiveOil(ItemType type)
{
	if ( type < 0 || type >= NUMITEMS ) { return false; }
	if ( itemTypeIsQuiver(type) || items[type].category == THROWN ) { return true; }
	if ( items[type].category != WEAPON ) { return false; }
	Item item{};
	item.type = type;
	return getWeaponSkill(&item) != PRO_RANGED;
}

bool itemCanReceiveOil(const Item* item)
{
	return item && itemTypeCanReceiveOil(item->type);
}

bool oilMatchesTarget(AppliedOil oil, Entity* target)
{
	if ( oil <= APPLIED_OIL_NONE || oil >= APPLIED_OIL_MAX || !target ) { return false; }
	Stat* targetStats = target->getStats();
	if ( !targetStats ) { return false; }
	if ( oil == APPLIED_OIL_BASES )
	{
		return targetStats->type == SLIME;
	}
	auto found = CompendiumEntries.monsters.find(monstertypename[targetStats->type]);
	if ( found == CompendiumEntries.monsters.end() ) { return false; }
	const auto species = found->second.species;
	switch ( oil )
	{
		case APPLIED_OIL_AETHER: return species == Compendium_t::CompendiumMonsters_t::SPECIES_UNDEAD;
		case APPLIED_OIL_SILVER: return species == Compendium_t::CompendiumMonsters_t::SPECIES_DEMONOID;
		case APPLIED_OIL_TOXINS: return species == Compendium_t::CompendiumMonsters_t::SPECIES_HUMANOID;
		case APPLIED_OIL_LEAD: return species == Compendium_t::CompendiumMonsters_t::SPECIES_BEASTFOLK;
		case APPLIED_OIL_MINERAL: return species == Compendium_t::CompendiumMonsters_t::SPECIES_BEAST;
		case APPLIED_OIL_BRINE: return species == Compendium_t::CompendiumMonsters_t::SPECIES_CONSTRUCT;
		default: return false;
	}
}

void serverSyncTinctureStacks(int player)
{
	if ( multiplayer != SERVER || player <= 0 || player >= MAXPLAYERS || !stats[player] ) { return; }
	strcpy(reinterpret_cast<char*>(net_packet->data), "TINC");
	net_packet->data[4] = static_cast<Uint8>(player);
	for ( int stat = 0; stat < NUMSTATS; ++stat )
	{
		SDLNet_Write32(stats[player]->tinctureStacks[stat], &net_packet->data[5 + stat * 4]);
	}
	net_packet->address = net_clients[player - 1];
	net_packet->len = 5 + NUMSTATS * 4;
	sendPacketSafe(net_sock, -1, net_packet, player - 1);
}

void serverSyncAppliedOil(int player, bool shieldSlot)
{
	if ( multiplayer != SERVER || player <= 0 || player >= MAXPLAYERS || !stats[player] ) { return; }
	Item* target = shieldSlot ? stats[player]->shield : stats[player]->weapon;
	strcpy(reinterpret_cast<char*>(net_packet->data), "OILU");
	net_packet->data[4] = shieldSlot ? 1 : 0;
	net_packet->data[5] = target ? static_cast<Uint8>(target->appliedOil) : APPLIED_OIL_NONE;
	net_packet->address = net_clients[player - 1];
	net_packet->len = 6;
	sendPacketSafe(net_sock, -1, net_packet, player - 1);
}

void rollOilWearOff(Item* source, int ownerPlayer, bool shieldSlot)
{
	if ( multiplayer == CLIENT || !source || source->appliedOil == APPLIED_OIL_NONE ) { return; }
	if ( local_rng.rand() % 40 != 0 ) { return; }
	source->appliedOil = APPLIED_OIL_NONE;
	serverSyncAppliedOil(ownerPlayer, shieldSlot);
	messagePlayer(ownerPlayer, MESSAGE_EQUIPMENT, "The oil coating on %s wears off.", source->getName());
}

void rollProjectileOilWearOff(Entity* projectile)
{
	if ( multiplayer == CLIENT || !projectile ) { return; }
	const AppliedOil oil = static_cast<AppliedOil>(projectile->projectileAppliedOil);
	if ( oil == APPLIED_OIL_NONE || local_rng.rand() % 40 != 0 ) { return; }
	for ( int player = 0; player < MAXPLAYERS; ++player )
	{
		if ( !stats[player] ) { continue; }
		Item* candidates[2] = { stats[player]->weapon, stats[player]->shield };
		for ( int slot = 0; slot < 2; ++slot )
		{
			Item* candidate = candidates[slot];
			if ( candidate && candidate->uid == static_cast<Uint32>(projectile->projectileOilSourceItemUID)
				&& candidate->type == static_cast<ItemType>(projectile->projectileOilSourceItemType)
				&& candidate->appliedOil == oil )
			{
				candidate->appliedOil = APPLIED_OIL_NONE;
				serverSyncAppliedOil(player, slot == 1);
				return;
			}
		}
		for ( node_t* node = stats[player]->inventory.first; node; node = node->next )
		{
			Item* candidate = static_cast<Item*>(node->element);
			if ( candidate && candidate->uid == static_cast<Uint32>(projectile->projectileOilSourceItemUID)
				&& candidate->type == static_cast<ItemType>(projectile->projectileOilSourceItemType)
				&& candidate->appliedOil == oil )
			{
				candidate->appliedOil = APPLIED_OIL_NONE;
				return;
			}
		}
	}
}

bool applyTinctureStackToPlayer(ItemType tincture, int player)
{
	if ( multiplayer == CLIENT || player < 0 || player >= MAXPLAYERS || !stats[player]
		|| !itemTypeIsTincture(tincture) ) { return false; }
	const int stat = static_cast<int>(tincture) - static_cast<int>(TINCTURE_STR);
	if ( stats[player]->tinctureStacks[stat] != UINT32_MAX )
	{
		++stats[player]->tinctureStacks[stat];
	}
	serverSyncTinctureStacks(player);
	return true;
}

bool spellbookIsEligibleForInk(ItemType book)
{
	if ( book < 0 || book >= NUMITEMS || items[book].category != SPELLBOOK )
	{
		return false;
	}
	switch ( book )
	{
		case SPELLBOOK_9:
		case SPELLBOOK_10:
		case SPELLBOOK_ENTRENCH:
		case SPELLBOOK_VAMPIRIC_AURA:
			return false;
		default:
			break;
	}
	const int spellID = getSpellIDFromSpellbook(book);
	if ( spellID == SPELL_NONE )
	{
		return false;
	}
	const spell_t* spell = getSpellFromID(spellID);
	if ( !spell || spell->ID != spellID || spell->hide_from_ui || spell->drop_table < 0 )
	{
		return false;
	}
	return getSpellbookFromSpellID(spell->ID) == static_cast<int>(book);
}

ItemType getInkSpellbookResult(ItemType ink, ItemType sourceSpellbook)
{
	if ( !itemTypeIsInk(ink) || sourceSpellbook < 0 || sourceSpellbook >= NUMITEMS
		|| items[sourceSpellbook].category != SPELLBOOK ) { return ITEM_ENUM_MAX; }
	const int sourceSpellID = getSpellIDFromSpellbook(sourceSpellbook);
	spell_t* source = sourceSpellID != SPELL_NONE ? getSpellFromID(sourceSpellID) : nullptr;
	if ( !source || source->ID != sourceSpellID ) { return ITEM_ENUM_MAX; }
	const int targetSkill = ink == INK_MYSTICISM ? PRO_MYSTICISM
		: (ink == INK_SORCERY ? PRO_SORCERY : PRO_THAUMATURGY);
	auto tierOf = [](const spell_t* spell)
	{
		return spell ? std::min(5, std::max(0, spell->difficulty / 20)) : -1;
	};
	const int sourceTier = tierOf(source);
	std::vector<ItemType> sameTier;
	std::vector<ItemType> nextTier;
	for ( int type = 0; type < NUMITEMS; ++type )
	{
		if ( !spellbookIsEligibleForInk(static_cast<ItemType>(type)) ) { continue; }
		spell_t* candidate = getSpellFromID(getSpellIDFromSpellbook(static_cast<ItemType>(type)));
		if ( !candidate || candidate->skillID != targetSkill ) { continue; }
		if ( tierOf(candidate) == sourceTier ) { sameTier.push_back(static_cast<ItemType>(type)); }
		else if ( tierOf(candidate) == sourceTier + 1 ) { nextTier.push_back(static_cast<ItemType>(type)); }
	}
	if ( sameTier.empty() ) { return ITEM_ENUM_MAX; }
	auto* pool = &sameTier;
	if ( source->skillID == targetSkill && !nextTier.empty() && local_rng.rand() % 4 == 0 )
	{
		pool = &nextTier;
	}
	return (*pool)[local_rng.rand() % pool->size()];
}

Item* newItemFromExistingItem(const Item& source, const Sint16 count, list_t* const inventory)
{
	Item* item = newItem(source.type, source.status, source.beatitude, count,
		source.appearance, source.identified, inventory);
	copyItemPersistentCustomState(*item, source);
	// This reconstructs the same physical item: preserve its Rune identity while
	// retaining the fresh allocation-local Item::uid assigned by newItem().
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

itemLevelCurve

Selects an item type from the given category of items by factoring in
dungeon level and defined level of the item

-------------------------------------------------------------------------------*/
enum ItemLevelCurveType
{
	ITEM_LEVEL_CURVE_TYPE_DEFAULT,
	ITEM_LEVEL_CURVE_TYPE_CHEST,
	ITEM_LEVEL_CURVE_TYPE_SHOP
};

ItemLevelCurveType itemLevelCurveType = ITEM_LEVEL_CURVE_TYPE_DEFAULT;
int itemLevelCurveShop = -1;
ItemType itemLevelCurveEntity(Entity& my, Category cat, int minLevel, int maxLevel, BaronyRNG& rng)
{
	itemLevelCurveType = ITEM_LEVEL_CURVE_TYPE_DEFAULT;
	itemLevelCurveShop = -1;
	if ( my.behavior == &actMonster && (my.getMonsterTypeFromSprite() == SHOPKEEPER || my.monsterCanTradeWith(-1)) )
	{
		itemLevelCurveType = ITEM_LEVEL_CURVE_TYPE_SHOP;
		itemLevelCurveShop = my.monsterStoreType;
	}
	else if ( my.behavior == &actChest )
	{
		itemLevelCurveType = ITEM_LEVEL_CURVE_TYPE_CHEST;
	}

	auto result = itemLevelCurve(cat, minLevel, maxLevel, rng);
	itemLevelCurveType = ITEM_LEVEL_CURVE_TYPE_DEFAULT;
	itemLevelCurveShop = -1;
	return result;
}

bool itemLevelCurvePostProcess(Entity* my, Item* item, BaronyRNG& rng, int itemLevel, int* lastItemType, int* lastItemSpellType)
{
	if ( !((my && my->behavior == &actItem) || item) )
	{
		return false;
	}

	bool modified = false;
	itemLevelCurveType = ITEM_LEVEL_CURVE_TYPE_DEFAULT;
	if ( my )
	{
		if ( my->behavior == &actMonster && (my->getMonsterTypeFromSprite() == SHOPKEEPER || my->monsterCanTradeWith(-1)) )
		{
			itemLevelCurveType = ITEM_LEVEL_CURVE_TYPE_SHOP;
			itemLevelCurveShop = my->monsterStoreType;
		}
		else if ( my->behavior == &actChest )
		{
			itemLevelCurveType = ITEM_LEVEL_CURVE_TYPE_CHEST;
		}
	}

	int itemType = (my && my->behavior == &actItem) ? my->skill[10] : item->type;
	int itemStatus = (my && my->behavior == &actItem) ? my->skill[11] : item->type;
	if ( itemType >= BRONZE_TOMAHAWK && itemType <= CRYSTAL_SHURIKEN )
	{
		// thrown weapons always fixed status. (tomahawk = decrepit, shuriken = excellent)
		itemStatus = std::min(static_cast<Status>(DECREPIT + (itemType - BRONZE_TOMAHAWK)), EXCELLENT);
		if ( my && my->behavior == &actItem )
		{
			my->skill[11] = itemStatus;
		}
		else
		{
			item->status = static_cast<Status>(itemStatus);
		}
	}
	if ( itemType == BOLAS )
	{
		itemStatus = SERVICABLE;
		if ( my && my->behavior == &actItem )
		{
			my->skill[11] = itemStatus;
		}
		else
		{
			item->status = static_cast<Status>(itemStatus);
		}
	}
	if ( itemType >= 0 && itemType < NUMITEMS )
	{
		if ( items[itemType].category == SPELLBOOK )
		{
			//if ( itemLevelCurveType == ITEM_LEVEL_CURVE_TYPE_DEFAULT )
			{
				std::vector<std::pair<int, int>> chances;
				chances.reserve(NUM_SPELLS);
				std::vector<unsigned int> chanceWeights;
				chanceWeights.reserve(NUM_SPELLS);
				int minDifficulty = std::min(60, (itemLevel / 5) * 20);
#ifndef NDEBUG
				std::map<int, int> debugChances;
				std::map<int, int> debugChancesNum;
#endif

				int lastSpellSkill = -1;
				if ( lastItemSpellType && *lastItemSpellType >= 0 && *lastItemSpellType < NUMITEMS )
				{
					int spellID = getSpellIDFromSpellbook(*lastItemSpellType);
					if ( spellID != SPELL_NONE )
					{
						if ( auto spell = getSpellFromID(spellID) )
						{
							lastSpellSkill = spell->skillID;
						}
					}
				}

				for ( int i = 0; i < NUM_SPELLS; ++i )
				{
					auto find = allGameSpells.find(i);
					if ( find != allGameSpells.end() )
					{
						if ( auto spell = find->second )
						{
							if ( spell->ID != SPELL_NONE
								&& spell->ID != SPELL_ENTRENCH //mod add: Entrench has explicit acquisition sources only
								&& !spell->hide_from_ui && itemLevel >= spell->drop_table )
							{
								if ( (spell->difficulty / 20) <= (1 + (itemLevel / 5))
									/*&& (spell->difficulty >= minDifficulty)*/ )
								{

									if ( lastSpellSkill >= 0 )
									{
										if ( lastSpellSkill == PRO_SORCERY )
										{
											if ( spell->skillID != PRO_MYSTICISM ) // mysticism after sorc
											{
												continue;
											}
										}
										else if ( lastSpellSkill == PRO_MYSTICISM )
										{
											if ( spell->skillID != PRO_THAUMATURGY ) // thaum after mysticism
											{
												continue;
											}
										}
										else if ( lastSpellSkill == PRO_THAUMATURGY )
										{
											if ( spell->skillID != PRO_SORCERY ) // sorc after thaum
											{
												continue;
											}
										}
									}

									chances.push_back(std::make_pair(spell->skillID, spell->ID));
									if ( spell->difficulty == minDifficulty )
									{
										chanceWeights.push_back(40);
									}
									else if ( spell->difficulty > minDifficulty )
									{
										chanceWeights.push_back(30);
									}
									else if ( spell->difficulty == 40 )
									{
										chanceWeights.push_back(10);
									}
									else if ( spell->difficulty == 20 )
									{
										chanceWeights.push_back(10);
									}
									else if ( spell->difficulty == 0 )
									{
										chanceWeights.push_back(10);
									}

									if ( spell->difficulty == 0 )
									{
										if ( minDifficulty == 0 )
										{
											chanceWeights.back() += 20;
										}
										else if ( minDifficulty == 20 )
										{
											chanceWeights.back() += 20;
										}
										chanceWeights.back() *= 2;
									}
									else if ( spell->difficulty >= 80 )
									{
										chanceWeights.back() *= 2;
									}
#ifndef NDEBUG
									debugChances[spell->difficulty] += chanceWeights.back();
									debugChancesNum[spell->difficulty] += 1;
#endif
								}
							}
						}
					}
				}
				assert(chances.size() == chanceWeights.size());
				if ( chances.size() )
				{
					Uint32 appearance = (my && my->behavior == &actItem) ? my->skill[14] : item->appearance;
					int pick = rng.discrete(chanceWeights.data(), chanceWeights.size());
					int spellbookType = getSpellbookFromSpellID(chances[pick].second);
					if ( items[spellbookType].category == SPELLBOOK )
					{
						itemType = spellbookType;
					}
					else
					{
						itemType = TOME_SORCERY;
						appearance = spellTomeIDToAppearance[chances[pick].second];
						if ( chances[pick].first == PRO_MYSTICISM )
						{
							itemType = TOME_MYSTICISM;
						}
						else if ( chances[pick].first == PRO_THAUMATURGY )
						{
							itemType = TOME_THAUMATURGY;
						}
					}
					if ( my && my->behavior == &actItem )
					{
						my->skill[10] = itemType;
						my->skill[14] = appearance;
					}
					else
					{
						item->type = static_cast<ItemType>(itemType);
						item->appearance = appearance;
					}
				}
			}
		}
		if ( lastItemType )
		{
			*lastItemType = itemType;
		}
		if ( lastItemSpellType )
		{
			if ( items[itemType].category == SPELLBOOK )
			{
				*lastItemSpellType = itemType;
			}
		}
	}

	itemLevelCurveType = ITEM_LEVEL_CURVE_TYPE_DEFAULT;
	itemLevelCurveShop = -1;

	return modified;
}

bool isHatShopItem(ItemType hat)
{
	switch ( hat )
	{
	case HAT_PHRYGIAN:
	case HAT_HOOD:
	case HAT_WIZARD:
	case HAT_JESTER:
	case LEATHER_HELM:
	case IRON_HELM:
	case STEEL_HELM:
	case CRYSTAL_HELM:
	case HAT_FEZ:
	case HAT_HOOD_SILVER:
	case HAT_HOOD_RED:

	case MASK_EYEPATCH:
	case MASK_MASQUERADE:
	case MASK_GOLDEN:
	case MASK_SPOOKY:
	case MASK_HAZARD_GOGGLES:
	case MASK_PHANTOM:
	case MASK_PLAGUE:
	case HAT_SILKEN_BOW:
	case HAT_PLUMED_CAP:
	case HAT_BYCOCKET:
	case HAT_TOPHAT:
	case HAT_BANDANA:
	case HAT_CIRCLET:
	case HAT_CROWN:
	case HAT_LAURELS:
	case HAT_TURBAN:
	case HAT_CROWNED_HELM:
	case HAT_WARM:
	case HAT_WOLF_HOOD:
	case HAT_BEAR_HOOD:
	case HAT_STAG_HOOD:
	case HAT_BUNNY_HOOD:
	case HAT_MITER:
	case HAT_HEADDRESS:
	case HAT_CHEF:
	case HELM_MINING:
	case MASK_STEEL_VISOR:
	case MASK_CRYSTAL_VISOR:
	case HAT_CIRCLET_WISDOM:
	case HAT_HOOD_APPRENTICE:
	case HAT_HOOD_ASSASSIN:
	case HAT_HOOD_WHISPERS:
	case HAT_FELT:
	case HOOD_TEAL:
	case HAT_CIRCLET_SORCERY:
	case HAT_CIRCLET_THAUMATURGY:
		return true;
		default:
			break;
	}
	return false;
}

ItemType itemLevelCurve(const Category cat, const int minLevel, const int maxLevel, BaronyRNG& rng)
{
	const int numitems = NUMITEMS;
	bool chances[NUMITEMS];
	int c;

	if ( cat < 0 || cat >= Category::CATEGORY_MAX )
	{
		printlog("warning: itemLevelCurve() called with bad category value!\n");
		return GEM_ROCK;
	}

	Uint32 numoftype = 0;
	for ( c = 0; c < numitems; ++c )
	{
		chances[c] = false;
		if ( itemTypeIsWitchbrewConsumable(static_cast<ItemType>(c))
			|| itemTypeIsWitchbrewGeneratedItem(static_cast<ItemType>(c)) )
		{
			continue;
		}
		if ( items[c].category == cat )
		{
			if ( items[c].level != -1 && (items[c].level >= minLevel && items[c].level <= maxLevel) )
			{
				chances[c] = true;
				numoftype++;

				if ( itemLevelCurveType == ITEM_LEVEL_CURVE_TYPE_SHOP )
				{
					if ( c == READABLE_BOOK )
					{
						continue;
					}

					if ( itemLevelCurveShop == 1 ) // hat store
					{
						if ( !isHatShopItem(static_cast<ItemType>(c)) )
						{
							chances[c] = false;
							continue;
						}
					}
					if ( items[c].hasAttribute("SHOP_EXCLUDE_FROM_CATEGORY_1") )
					{
						if ( items[c].attributes["SHOP_EXCLUDE_FROM_CATEGORY_1"] == itemLevelCurveShop )
						{
							chances[c] = false;
							continue;
						}
					}
					if ( items[c].hasAttribute("SHOP_EXCLUDE_FROM_CATEGORY_2") )
					{
						if ( items[c].attributes["SHOP_EXCLUDE_FROM_CATEGORY_2"] == itemLevelCurveShop )
						{
							chances[c] = false;
							continue;
						}
					}
				}

				if ( cat == TOOL )
				{
					switch ( static_cast<ItemType>(c) )
					{
						case TOOL_TINOPENER:
							if ( rng.rand() % 2 )   // 50% chance
							{
								chances[c] = false;
							}
							break;
						case TOOL_LANTERN:
							if ( rng.rand() % 4 == 0 )   // 25% chance
							{
								chances[c] = false;
							}
							break;
						case TOOL_FRYING_PAN:
							if ( rng.rand() % 4 )   // 75% chance
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
							if ( rng.rand() % 4 )   // 25% chance
							{
								chances[c] = false;
							}
							break;
						case MASK_GRASS_SPRIG: // swamp only
							if ( !(!strncmp(map.name, "The Swamp", 9)
								&& itemLevelCurveType == ITEM_LEVEL_CURVE_TYPE_DEFAULT) )
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
		if ( rng.rand() % 10 )
		{
			return GEM_GLASS;
		}
	}

	// pick the item
	Uint32 pick = rng.rand() % numleft;
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
				if ( type == POTION_EMPTY || itemTypeIsWitchbrewConsumable(type) )
				{
					//No fancy descriptives for empty potions.
					snprintf(tempstr, 1024, Language::get(982 + status), beatitude);
				}
				/*else if ( type == POTION_GREASE )
				{
					snprintf(tempstr, 1024, Language::get(992 + status), Language::get(975), beatitude);
				}*/
				else
				{
					snprintf(tempstr, 1024, Language::get(992 + status), Language::get(974 + items[type].index + appearance % items[type].variations - 50), beatitude);
				}
			}
			else if ( itemCategory(this) == SCROLL || itemCategory(this) == SPELLBOOK || itemCategory(this) == BOOK || itemCategory(this) == TOME_SPELL )
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
					snprintf(&tempstr[c], 1024 - c, Language::get(1007), getBookLocalizedNameFromIndex(appearance % numbooks).c_str());
				}
				else if ( itemCategory(this) == TOME_SPELL )
				{
					snprintf(&tempstr[c], 1024 - c, Language::get(6850), getTomeLabel());
				}
				else if ( isMagicRune() )
				{
					const std::string name = runeGetDisplayName();
					snprintf(&tempstr[c], 1024 - c, "%s", name.c_str());
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
				if ( type == POTION_EMPTY || itemTypeIsWitchbrewConsumable(type) )
				{
					//No fancy descriptives for empty potions.
					snprintf(tempstr, 1024, Language::get(1008 + status), count, beatitude);
				}
				/*else if ( type == POTION_GREASE )
				{
					snprintf(tempstr, 1024, Language::get(1018 + status), count, Language::get(975), beatitude);
				}*/
				else
				{
					snprintf(tempstr, 1024, Language::get(1018 + status), count, Language::get(974 + items[type].index + appearance % items[type].variations - 50), beatitude);
				}
			}
			else if ( itemCategory(this) == SCROLL || itemCategory(this) == SPELLBOOK || itemCategory(this) == BOOK || itemCategory(this) == TOME_SPELL )
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
					snprintf(&tempstr[c], 1024 - c, Language::get(1033), count, getBookLocalizedNameFromIndex(appearance % numbooks).c_str());
				}
				else if ( itemCategory(this) == TOME_SPELL )
				{
					snprintf(&tempstr[c], 1024 - c, Language::get(6851), getTomeLabel());
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
				if ( type == POTION_EMPTY || itemTypeIsWitchbrewConsumable(type) )
				{
					//No fancy descriptives for empty potions.
					snprintf(tempstr, 1024, Language::get(1034 + status), beatitude);
				}
				/*else if ( type == POTION_GREASE )
				{
					snprintf(tempstr, 1024, Language::get(1044 + status), Language::get(975), beatitude);
				}*/
				else
				{
					snprintf(tempstr, 1024, Language::get(1044 + status), Language::get(974 + items[type].index + appearance % items[type].variations - 50));
				}
			}
			else if ( itemCategory(this) == SCROLL || itemCategory(this) == SPELLBOOK || itemCategory(this) == BOOK || itemCategory(this) == TOME_SPELL )
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
						snprintf(&tempstr[c], 1024 - c, Language::get(1007), getBookLocalizedNameFromIndex(appearance % numbooks).c_str());
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
				if ( type == POTION_EMPTY || itemTypeIsWitchbrewConsumable(type) )
				{
					//No fancy descriptives for empty potions.
					snprintf(tempstr, 1024, Language::get(1060 + status), count);
				}
				/*else if ( type == POTION_GREASE )
				{
					snprintf(tempstr, 1024, Language::get(1070 + status), count, Language::get(975), beatitude);
				}*/
				else
				{
					snprintf(tempstr, 1024, Language::get(1070 + status), count, Language::get(974 + items[type].index + appearance % items[type].variations - 50));
				}
			}
			else if ( itemCategory(this) == SCROLL || itemCategory(this) == SPELLBOOK || itemCategory(this) == BOOK || itemCategory(this) == TOME_SPELL )
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
						snprintf(&tempstr[c], 1024 - c, Language::get(1086), count, getBookLocalizedNameFromIndex(appearance % numbooks).c_str());
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
			if ( isMagicRune() )
			{
				const std::string name = runeGetDisplayName();
				stringCopy(tempstr, name.c_str(), sizeof(tempstr) - 1, name.size());
			}
			else if ( itemCategory(this) == BOOK )
			{
				snprintf(tempstr, sizeof(tempstr), Language::get(1007), getBookLocalizedNameFromIndex(appearance % numbooks).c_str());
			}
			else if ( itemCategory(this) == TOME_SPELL )
			{
				snprintf(tempstr, sizeof(tempstr), Language::get(6850), getTomeLabel());
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
				snprintf(tempstr, sizeof(tempstr), Language::get(1007), getBookLocalizedNameFromIndex(appearance % numbooks).c_str());
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

namespace
{
	// New Rune appearance format. The gem nibble is stored as gem + 1, so its
	// nonzero value is a collision-free format tag: legacy spell IDs use only
	// bits 0..7 and therefore always leave bits 8..11 clear.
	// Common fields: spell[0..7], biased gem[8..11], payload[12..31]. Dominate
	// divides payload into factor[12..19] and exact capacity[20..31].
	constexpr Uint32 RUNE_SPELL_BITS = 8;
	constexpr Uint32 RUNE_SPELL_MASK = (1u << RUNE_SPELL_BITS) - 1u;
	constexpr Uint32 RUNE_GEM_SHIFT = RUNE_SPELL_BITS;
	constexpr Uint32 RUNE_GEM_BITS = 4;
	constexpr Uint32 RUNE_GEM_MASK = ((1u << RUNE_GEM_BITS) - 1u) << RUNE_GEM_SHIFT;
	constexpr Uint32 RUNE_GEM_FORMAT_BIAS = 1;
	constexpr Uint32 RUNE_PAYLOAD_SHIFT = RUNE_GEM_SHIFT + RUNE_GEM_BITS;
	constexpr Uint32 RUNE_PAYLOAD_BITS = 20;
	constexpr Uint32 RUNE_PAYLOAD_MAX = (1u << RUNE_PAYLOAD_BITS) - 1u;
	constexpr Uint32 RUNE_PAYLOAD_MASK = RUNE_PAYLOAD_MAX << RUNE_PAYLOAD_SHIFT;
	constexpr Uint32 RUNE_DOMINATE_FACTOR_BITS = 8;
	constexpr Uint32 RUNE_DOMINATE_FACTOR_MAX = (1u << RUNE_DOMINATE_FACTOR_BITS) - 1u;
	constexpr Uint32 RUNE_DOMINATE_FACTOR_MASK = RUNE_DOMINATE_FACTOR_MAX << RUNE_PAYLOAD_SHIFT;
	constexpr real_t RUNE_DOMINATE_FACTOR_MIN = 1.0 / 1024.0;
	constexpr Uint32 RUNE_DOMINATE_CAPACITY_SHIFT = RUNE_PAYLOAD_SHIFT + RUNE_DOMINATE_FACTOR_BITS;
	constexpr Uint32 RUNE_DOMINATE_CAPACITY_BITS = 12;
	constexpr Uint32 RUNE_DOMINATE_CAPACITY_MASK =
		((1u << RUNE_DOMINATE_CAPACITY_BITS) - 1u) << RUNE_DOMINATE_CAPACITY_SHIFT;
	constexpr int RUNE_DOMINATE_CAPACITY_MAX = (1 << RUNE_DOMINATE_CAPACITY_BITS) - 1;
	// Legacy format is read only so pre-rework Runes retain spell/gem/capacity.
	constexpr Uint32 RUNE_LEGACY_SPELL_MASK = (1u << 12) - 1u;
	constexpr Uint32 RUNE_LEGACY_GEM_SHIFT = 12;
	constexpr Uint32 RUNE_LEGACY_GEM_MASK = ((1u << 5) - 1u) << RUNE_LEGACY_GEM_SHIFT;
	constexpr Uint32 RUNE_LEGACY_DOMINATE_CAPACITY_SHIFT = 20;
	constexpr Uint32 RUNE_LEGACY_DOMINATE_CAPACITY_MASK = ((1u << 12) - 1u) << 20;
	static_assert(NUM_SPELLS <= (1 << RUNE_SPELL_BITS), "Rune spell field is too small");
	static_assert(RUNE_GEM_COUNT < (1 << RUNE_GEM_BITS), "Rune gem field is too small");

	constexpr real_t RUNE_VALUE_TRADING_FACTOR = 0.70;
	constexpr real_t RUNE_MP_COIN_RATIO = 0.75;
	constexpr real_t RUNE_AVERAGE_QUALITY = 2.50;
	constexpr real_t RUNE_GEM_VALUE_MIN = 200.0;
	constexpr real_t RUNE_GEM_VALUE_MAX = 3000.0;
	constexpr real_t RUNE_INSCRIPTION_DECREPIT_MULT = 1.20;
	constexpr real_t RUNE_INSCRIPTION_WORN_MULT = 2.20;
	constexpr real_t RUNE_INSCRIPTION_SERVICABLE_MULT = 3.00;
	constexpr real_t RUNE_INSCRIPTION_EXCELLENT_MULT = 3.60;

	real_t runeInscriptionConditionMultiplier(Status status)
	{
		switch ( status )
		{
			case DECREPIT: return RUNE_INSCRIPTION_DECREPIT_MULT;
			case WORN: return RUNE_INSCRIPTION_WORN_MULT;
			case SERVICABLE: return RUNE_INSCRIPTION_SERVICABLE_MULT;
			case EXCELLENT: return RUNE_INSCRIPTION_EXCELLENT_MULT;
			default: return 0.0;
		}
	}

	bool runeUsesVariableMPFactor(const spell_t* spell)
	{
		if ( !spell ) { return false; }
		return runeSpellIsSustained(const_cast<spell_t*>(spell))
			|| spell->ID == SPELL_DOMINATE || spell->ID == SPELL_SUMMON
			|| spell->ID == SPELL_BREATHE_FIRE || spell->ID == SPELL_TELEPORTATION
			|| spell->ID == SPELL_TELEPULL || spell->ID == SPELL_SANCTUARY
			// Item-targeting spells have a distinct, target-dependent MP event
			// after their initial cast. One general factor represents both events.
			|| spell->ID == SPELL_ALTER_INSTRUMENT || spell->ID == SPELL_METALLURGY
			|| spell->ID == SPELL_GEOMANCY || spell->ID == SPELL_FORGE_KEY
			|| spell->ID == SPELL_FORGE_JEWEL || spell->ID == SPELL_ENHANCE_WEAPON
			|| spell->ID == SPELL_RESHAPE_WEAPON || spell->ID == SPELL_ALTER_ARROW
			|| spell->ID == SPELL_VANDALISE || spell->ID == SPELL_RESTORE
			|| spell->ID == SPELL_SANCTIFY_WATER;
	}

	real_t runeEncodeClamp(real_t value)
	{
		return std::max<real_t>(0.0, std::min<real_t>(1.0, value));
	}

	Uint32 encodeRuneDominateFactor(real_t factor)
	{
		factor = runeEncodeClamp(factor);
		if ( factor <= 0.0 ) { return 0; }
		const real_t normalized = (log2(std::max(RUNE_DOMINATE_FACTOR_MIN, factor)) + 10.0) / 10.0;
		return 1u + static_cast<Uint32>(std::max<real_t>(0.0,
			std::min<real_t>(1.0, normalized)) * (RUNE_DOMINATE_FACTOR_MAX - 1) + 0.5);
	}

	real_t decodeRuneDominateFactor(Uint32 encoded)
	{
		if ( encoded == 0 ) { return 0.0; }
		const real_t normalized = (encoded - 1) / static_cast<real_t>(RUNE_DOMINATE_FACTOR_MAX - 1);
		return pow(2.0, -10.0 + normalized * 10.0);
	}

}

bool runeSpellIsSustained(spell_t* spell)
{
	if ( !spell ) { return false; }
	if ( spell_isChanneled(spell) ) { return true; }
	const auto definition = ItemTooltips.spellItems.find(spell->ID);
	return definition != ItemTooltips.spellItems.end()
		&& definition->second.spellType == ItemTooltips_t::SpellItemTypes::SPELL_TYPE_SELF_SUSTAIN;
}

bool Item::isMagicRune() const { return type == MAGIC_RUNE; }

int Item::runeGetSpellID() const
{
	if ( !isMagicRune() ) { return SPELL_NONE; }
	return static_cast<int>(appearance & ((appearance & RUNE_GEM_MASK)
		? RUNE_SPELL_MASK : RUNE_LEGACY_SPELL_MASK));
}

void Item::runeSetSpellID(int spellID)
{
	if ( !isMagicRune() ) { return; }
	appearance = (appearance & ~RUNE_SPELL_MASK)
		| (static_cast<Uint32>(spellID) & RUNE_SPELL_MASK);
}

RuneGemType Item::runeGetGemType() const
{
	if ( !isMagicRune() ) { return RUNE_GEM_INVALID; }
	const bool current = (appearance & RUNE_GEM_MASK) != 0;
	const Uint32 gem = current
		? (((appearance & RUNE_GEM_MASK) >> RUNE_GEM_SHIFT) - RUNE_GEM_FORMAT_BIAS)
		: ((appearance & RUNE_LEGACY_GEM_MASK) >> RUNE_LEGACY_GEM_SHIFT);
	return gem < RUNE_GEM_COUNT ? static_cast<RuneGemType>(gem) : RUNE_GEM_INVALID;
}

void Item::runeSetGemType(RuneGemType gem)
{
	if ( !isMagicRune() || gem >= RUNE_GEM_COUNT ) { return; }
	if ( !(appearance & RUNE_GEM_MASK) )
	{
		// Converting a newly-created/legacy layout must not retain old payload bits.
		appearance &= RUNE_SPELL_MASK;
	}
	appearance = (appearance & ~RUNE_GEM_MASK) | (((static_cast<Uint32>(gem)
		+ RUNE_GEM_FORMAT_BIAS) << RUNE_GEM_SHIFT) & RUNE_GEM_MASK);
}

ItemType Item::runeGetVisualGemItemType() const
{
	return itemTypeFromRuneGemType(runeGetGemType());
}

ItemType itemTypeFromRuneGemType(RuneGemType gem)
{
	switch ( gem )
	{
		case RUNE_GEM_OBSIDIAN: return GEM_OBSIDIAN;
		case RUNE_GEM_JADE: return GEM_JADE;
		case RUNE_GEM_FLUORITE: return GEM_FLUORITE;
		case RUNE_GEM_AMETHYST: return GEM_AMETHYST;
		case RUNE_GEM_GARNET: return GEM_GARNET;
		case RUNE_GEM_CITRINE: return GEM_CITRINE;
		case RUNE_GEM_OPAL: return GEM_OPAL;
		case RUNE_GEM_JETSTONE: return GEM_JETSTONE;
		case RUNE_GEM_AMBER: return GEM_AMBER;
		case RUNE_GEM_AQUAMARINE: return GEM_AQUAMARINE;
		case RUNE_GEM_JACINTH: return GEM_JACINTH;
		case RUNE_GEM_EMERALD: return GEM_EMERALD;
		case RUNE_GEM_SAPPHIRE: return GEM_SAPPHIRE;
		case RUNE_GEM_RUBY: return GEM_RUBY;
		case RUNE_GEM_DIAMOND: return GEM_DIAMOND;
		default: return GEM_OBSIDIAN;
	}
}

RuneGemType runeGemTypeFromItemType(ItemType type)
{
	switch ( type )
	{
		case GEM_OBSIDIAN: return RUNE_GEM_OBSIDIAN;
		case GEM_JADE: return RUNE_GEM_JADE;
		case GEM_FLUORITE: return RUNE_GEM_FLUORITE;
		case GEM_AMETHYST: return RUNE_GEM_AMETHYST;
		case GEM_GARNET: return RUNE_GEM_GARNET;
		case GEM_CITRINE: return RUNE_GEM_CITRINE;
		case GEM_OPAL: return RUNE_GEM_OPAL;
		case GEM_JETSTONE: return RUNE_GEM_JETSTONE;
		case GEM_AMBER: return RUNE_GEM_AMBER;
		case GEM_AQUAMARINE: return RUNE_GEM_AQUAMARINE;
		case GEM_JACINTH: return RUNE_GEM_JACINTH;
		case GEM_EMERALD: return RUNE_GEM_EMERALD;
		case GEM_SAPPHIRE: return RUNE_GEM_SAPPHIRE;
		case GEM_RUBY: return RUNE_GEM_RUBY;
		case GEM_DIAMOND: return RUNE_GEM_DIAMOND;
		default: return RUNE_GEM_INVALID;
	}
}

int Item::runeGetMagicSchool() const
{
	if ( auto spell = getSpellFromID(runeGetSpellID()) ) { return spell->skillID; }
	return -1;
}

bool Item::runeIsValid() const
{
	return isMagicRune() && runeGetGemType() != RUNE_GEM_INVALID
		&& runeGetSpellID() > SPELL_NONE && getSpellFromID(runeGetSpellID()) != nullptr;
}

bool Item::runeCanCast() const
{
	if ( status == BROKEN || !runeIsValid() || !runeHasStoredPWR() ) { return false; }
	spell_t* spell = getSpellFromID(runeGetSpellID());
	return spell && spell->elements.first;
}

real_t Item::runeGetStoredBreakChance() const
{
	if ( !isMagicRune() || !(appearance & RUNE_GEM_MASK) ) { return 1.0; }
	return static_cast<real_t>((appearance & RUNE_PAYLOAD_MASK) >> RUNE_PAYLOAD_SHIFT)
		/ static_cast<real_t>(RUNE_PAYLOAD_MAX);
}

bool Item::runeSetStoredBreakChance(real_t chance)
{
	if ( !isMagicRune() || runeGetSpellID() == SPELL_DOMINATE ) { return false; }
	const Uint32 encoded = static_cast<Uint32>(runeEncodeClamp(chance) * RUNE_PAYLOAD_MAX + 0.5);
	appearance = (appearance & ~RUNE_PAYLOAD_MASK) | (encoded << RUNE_PAYLOAD_SHIFT);
	return true;
}

real_t Item::runeGetStoredResourceFactor() const
{
	if ( !isMagicRune() || !(appearance & RUNE_GEM_MASK) ) { return 1.0; }
	if ( runeGetSpellID() == SPELL_DOMINATE )
	{
		return decodeRuneDominateFactor((appearance & RUNE_DOMINATE_FACTOR_MASK) >> RUNE_PAYLOAD_SHIFT);
	}
	return runeGetStoredBreakChance();
}

bool Item::runeSetStoredResourceFactor(real_t factor)
{
	if ( !isMagicRune() ) { return false; }
	if ( runeGetSpellID() == SPELL_DOMINATE )
	{
		const Uint32 encoded = encodeRuneDominateFactor(factor);
		appearance = (appearance & ~RUNE_DOMINATE_FACTOR_MASK) | (encoded << RUNE_PAYLOAD_SHIFT);
	}
	else
	{
		return runeSetStoredBreakChance(factor);
	}
	return true;
}

int Item::runeGetDominateCapacity() const
{
	if ( !isMagicRune() ) { return 0; }
	return (appearance & RUNE_GEM_MASK)
		? static_cast<int>((appearance & RUNE_DOMINATE_CAPACITY_MASK) >> RUNE_DOMINATE_CAPACITY_SHIFT)
		: static_cast<int>((appearance & RUNE_LEGACY_DOMINATE_CAPACITY_MASK) >> RUNE_LEGACY_DOMINATE_CAPACITY_SHIFT);
}

bool Item::runeSetDominateCapacity(int capacity)
{
	if ( !isMagicRune() || capacity < 0 || capacity > RUNE_DOMINATE_CAPACITY_MAX ) { return false; }
	appearance = (appearance & ~RUNE_DOMINATE_CAPACITY_MASK)
		| ((static_cast<Uint32>(capacity) << RUNE_DOMINATE_CAPACITY_SHIFT) & RUNE_DOMINATE_CAPACITY_MASK);
	return true;
}

bool Item::runeHasStoredPWR() const
{
	return isMagicRune() && runeStoredPWR != RUNE_STORED_PWR_INVALID;
}

real_t Item::runeGetStoredPWR() const
{
	return runeHasStoredPWR() ? runeStoredPWR / RUNE_STORED_PWR_SCALE : 0.0;
}

real_t Item::runeGetStoredPWRDisplayPercent() const
{
	return 100.0 + runeGetStoredPWR() * 100.0;
}

Sint32 Item::runeGetStoredPWRRaw() const
{
	return runeStoredPWR;
}

bool Item::runeSetStoredPWR(real_t pwr)
{
	if ( !isMagicRune() || !std::isfinite(pwr) ) { return false; }
	const real_t scaled = pwr * RUNE_STORED_PWR_SCALE;
	// INT32_MIN remains reserved as the invalid/pre-change sentinel.
	if ( scaled < static_cast<real_t>(RUNE_STORED_PWR_INVALID + 1)
		|| scaled > static_cast<real_t>(INT32_MAX) )
	{
		return false;
	}
	runeStoredPWR = static_cast<Sint32>(llround(scaled));
	return true;
}

void Item::runeSetStoredPWRRaw(Sint32 raw)
{
	runeStoredPWR = isMagicRune() ? raw : RUNE_STORED_PWR_INVALID;
}

bool Item::runeHasCreator() const
{
	return isMagicRune() && runeCreatorPlayer >= 0 && runeCreatorPlayer < MAXPLAYERS;
}

int Item::runeGetCreatorPlayer() const
{
	return runeHasCreator() ? runeCreatorPlayer : RUNE_CREATOR_INVALID;
}

void Item::runeSetCreatorPlayer(int player)
{
	runeCreatorPlayer = isMagicRune() && player >= 0 && player < MAXPLAYERS
		? static_cast<Sint8>(player) : RUNE_CREATOR_INVALID;
}

bool Item::runeHasInstanceId() const
{
	return isMagicRune() && runeInstanceId != RUNE_INSTANCE_ID_INVALID;
}

Uint32 Item::runeGetInstanceId() const
{
	return runeHasInstanceId() ? runeInstanceId : RUNE_INSTANCE_ID_INVALID;
}

void Item::runeSetInstanceId(Uint32 instanceId)
{
	runeInstanceId = isMagicRune() ? instanceId : RUNE_INSTANCE_ID_INVALID;
	reserveMagicRuneInstanceId(runeInstanceId);
}

Uint32 Item::runeEnsureInstanceId()
{
	if ( !isMagicRune() ) { return RUNE_INSTANCE_ID_INVALID; }
	if ( !runeHasInstanceId() && multiplayer != CLIENT )
	{
		runeInstanceId = allocateNonzeroId(nextMagicRuneInstanceId);
	}
	return runeInstanceId;
}

Uint32 Item::familiarEnsureInstanceId()
{
	if ( type != AMULET_FAMILIAR ) { return 0; }
	if ( familiarInstanceId == 0 && multiplayer != CLIENT )
	{
		familiarInstanceId = allocateNonzeroId(nextFamiliarInstanceId);
	}
	reserveFamiliarInstanceId(familiarInstanceId);
	return familiarInstanceId;
}

Uint32 Item::runeGetCreatorIdentity() const
{
	return isMagicRune() ? runeCreatorIdentity : RUNE_CREATOR_IDENTITY_INVALID;
}

void Item::runeSetCreatorIdentity(Uint32 identity)
{
	runeCreatorIdentity = isMagicRune() ? identity : RUNE_CREATOR_IDENTITY_INVALID;
	reserveRuneCreatorIdentity(runeCreatorIdentity);
}

bool Item::runeCreatorMatchesPlayer(int player) const
{
	return runeHasCreator() && runeCreatorIdentity != RUNE_CREATOR_IDENTITY_INVALID
		&& player == runeGetCreatorPlayer() && player >= 0 && player < MAXPLAYERS
		&& stats[player] && stats[player]->runeCreatorIdentity == runeCreatorIdentity;
}

real_t getRuneDominateStorageEfficiency(const Item& rune)
{
	// Retain 60% at the least valuable supported gem, scaling linearly to 100%
	// at the most valuable.
	const real_t value = std::min(RUNE_GEM_VALUE_MAX,
		std::max(RUNE_GEM_VALUE_MIN, static_cast<real_t>(items[rune.runeGetVisualGemItemType()].gold_value)));
	return 0.60 + 0.40 * ((value - RUNE_GEM_VALUE_MIN) / (RUNE_GEM_VALUE_MAX - RUNE_GEM_VALUE_MIN));
}

real_t getRuneCrafterCompetence(const spell_t* spell, Entity* creator)
{
	if ( !spell || !creator || !creator->getStats() ) { return 0.0; }
	Stat* creatorStats = creator->getStats();
	if ( spell->skillID != PRO_SORCERY && spell->skillID != PRO_MYSTICISM
		&& spell->skillID != PRO_THAUMATURGY )
	{
		printlog("[Rune]: Spell %d uses unsupported crafting proficiency %d", spell->ID, spell->skillID);
		return 0.0;
	}
	const int school = creatorStats->getModifiedProficiency(spell->skillID);
	const int ability = std::max(0, std::min(100, school + statGetINT(creatorStats, creator)));
	const real_t cFizzle = ability < 20 ? 1.0 - 0.36 * (1.0 - ability / 100.0) : 1.0;
	const real_t cExtraMP = ability < 40 ? ability / 100.0 : 1.0;
	real_t castBonus = getSpellBonusFromCasterINT(creator, creatorStats, spell->skillID);
	if ( school >= SKILL_LEVEL_LEGENDARY ) { castBonus += 1.0; }
	const real_t traditionalTime = std::max<real_t>(1.0,
		2.0 - statGetDEX(creatorStats, creator) * (1.0 + std::max<real_t>(0.0, castBonus)) / 100.0);
	const real_t cCast = std::min<real_t>(1.0, 1.0 / traditionalTime);
	return runeEncodeClamp(cFizzle * cExtraMP * cCast);
}

namespace
{
	real_t getFavorableItemBeatitudePWRBonus(const Item& item, const Stat* wielder)
	{
		const int effectiveBeatitude = shouldInvertEquipmentBeatitude(wielder)
			? -static_cast<int>(item.beatitude) : static_cast<int>(item.beatitude);
		return std::max(0, effectiveBeatitude) * 0.10;
	}
}

bool initializeMagicRuneCraftingProfile(Item& rune, spell_t* spell, Entity* creator,
	const Item& sourceSpellbook)
{
	if ( !rune.isMagicRune() || !spell || !creator || !creator->getStats() ) { return false; }
	spellElement_t* primaryElement = nullptr;
	if ( spell->elements.first )
	{
		primaryElement = static_cast<spellElement_t*>(spell->elements.first->element);
		if ( primaryElement && primaryElement->elements.first )
		{
			primaryElement = static_cast<spellElement_t*>(primaryElement->elements.first->element);
		}
	}
	const real_t storedPWR = getBonusFromCasterOfSpellElement(creator,
		creator->getStats(), primaryElement, spell->ID, spell->skillID)
		+ getFavorableItemBeatitudePWRBonus(sourceSpellbook, creator->getStats());
	if ( !rune.runeSetStoredPWR(storedPWR) ) { return false; }
	const ItemType gemType = rune.runeGetVisualGemItemType();
	const real_t gemValue = std::max(0, items[gemType].gold_value);
	const real_t competence = getRuneCrafterCompetence(spell, creator);
	real_t mpFactor = 0.0;
	real_t goldFactor = 0.0;
	if ( competence <= 0.0 || gemValue <= 0.0 )
	{
		mpFactor = goldFactor = 1.0;
	}
	else
	{
		mpFactor = RUNE_AVERAGE_QUALITY
			/ (gemValue * RUNE_VALUE_TRADING_FACTOR * RUNE_MP_COIN_RATIO * competence);
		goldFactor = RUNE_AVERAGE_QUALITY
			/ (gemValue * RUNE_VALUE_TRADING_FACTOR * competence);
	}
	if ( spell->ID == SPELL_LEAD_BOLT )
	{
		return rune.runeSetStoredResourceFactor(goldFactor);
	}
	if ( runeUsesVariableMPFactor(spell) )
	{
		return rune.runeSetStoredResourceFactor(mpFactor);
	}
	const int fixedCost = std::max(0, getCostOfSpell(spell, creator));
	return rune.runeSetStoredBreakChance(fixedCost > 0 ? fixedCost * mpFactor : 0.0);
}

real_t getMagicRuneEffectivePWR(const Item& rune, const Entity* caster)
{
	if ( !rune.runeHasStoredPWR() ) { return 0.0; }
	return rune.runeGetStoredPWR() + (caster
		? getFavorableItemBeatitudePWRBonus(rune, caster->getStats()) : 0.0);
}

bool applyMagicRuneEffectivePWR(spell_t& spell, const Item& rune, const Entity* caster)
{
	if ( !rune.runeCanCast() || rune.runeGetSpellID() != spell.ID )
	{
		return false;
	}
	spell.hasSpellPowerOverride = true;
	spell.spellPowerOverride = getMagicRuneEffectivePWR(rune, caster);
	spell.runeCreatorPlayer = static_cast<Sint8>(rune.runeGetCreatorPlayer());
	spell.runeCreatorIdentity = rune.runeGetCreatorIdentity();
	spell.runeInstanceId = rune.runeGetInstanceId();
	return true;
}

int getRuneInscriptionManaCost(Status gemstoneStatus, const spell_t* spell, Entity* caster)
{
	if ( !spell || !caster || gemstoneStatus <= BROKEN || gemstoneStatus > EXCELLENT ) { return -1; }
	const real_t multiplier = runeInscriptionConditionMultiplier(gemstoneStatus);
	const int normalCost = getCostOfSpell(const_cast<spell_t*>(spell), caster);
	return normalCost < 0 ? -1 : static_cast<int>(ceil(normalCost * multiplier));
}

int getRuneInscriptionGoldCost(Status gemstoneStatus, spell_t* spell, int player)
{
	if ( !spell || gemstoneStatus <= BROKEN || gemstoneStatus > EXCELLENT ) { return -1; }
	const int normalCost = getGoldCostOfSpell(spell, player);
	return normalCost < 0 ? -1
		: static_cast<int>(ceil(normalCost * runeInscriptionConditionMultiplier(gemstoneStatus)));
}

bool activateMagicRune(int player)
{
	if ( player < 0 || player >= MAXPLAYERS || !players[player] || !players[player]->entity
		|| !stats[player] || !stats[player]->shield || !stats[player]->shield->isMagicRune() )
	{
		return false;
	}
	Item* rune = stats[player]->shield;
	if ( rune->runeGetSpellID() == SPELL_ENTRENCH
		&& (players[player]->mechanics.entrenchCarriedUid != 0
			|| players[player]->mechanics.entrenchStash.valid())
		&& players[player]->isLocalPlayer() )
	{
		return resumeEntrenchPlacementAnimation(player);
	}
	if ( rune->status == BROKEN )
	{
		messagePlayer(player, MESSAGE_EQUIPMENT, "The broken rune can no longer release its magic.");
		return false;
	}
	if ( !rune->runeIsValid() )
	{
		messagePlayer(player, MESSAGE_STATUS, "The rune's stored magic is invalid.");
		return false;
	}
	if ( !rune->runeHasStoredPWR() )
	{
		messagePlayer(player, MESSAGE_STATUS,
			"This legacy rune has no stored PWR snapshot and cannot be cast.");
		return false;
	}
	spell_t* spell = getSpellFromID(rune->runeGetSpellID());
	if ( !rune->runeCanCast() )
	{
		messagePlayer(player, MESSAGE_STATUS, "The rune cannot release its stored magic.");
		return false;
	}
	// Every Rune uses the explicit zero-resource Rune animation path. The cast
	// packet is sent only when that animation reaches its completion/target stage.
	if ( players[player]->isLocalPlayer() )
	{
		return castMagicRuneInit(player, spell, rune->runeGetInstanceId());
	}
	return false;
}

Item* findMagicRuneByInstanceId(Uint32 runeInstanceId, int* ownerOut, int requiredOwner)
{
	const int firstPlayer = requiredOwner >= 0 ? requiredOwner : 0;
	const int lastPlayer = requiredOwner >= 0 ? requiredOwner + 1 : MAXPLAYERS;
	for ( int player = firstPlayer; player < lastPlayer; ++player )
	{
		if ( player < 0 || player >= MAXPLAYERS || !stats[player] ) { continue; }
		for ( node_t* node = stats[player]->inventory.first; node; node = node->next )
		{
			Item* candidate = static_cast<Item*>(node->element);
			if ( candidate && candidate->isMagicRune()
				&& candidate->runeGetInstanceId() == runeInstanceId )
			{
				if ( ownerOut ) { *ownerOut = player; }
				return candidate;
			}
		}
	}
	return nullptr;
}

static void terminateSustainedSpellsForRune(Uint32 runeInstanceId)
{
	for ( int player = 0; player < MAXPLAYERS; ++player )
	{
		if ( !stats[player] ) { continue; }
		for ( node_t* node = stats[player]->magic_effects.first; node; node = node->next )
		{
			spell_t* sustained = static_cast<spell_t*>(node->element);
			if ( sustained && sustained->runeInstanceId == runeInstanceId )
			{
				sustained->sustain = false;
			}
		}
		for ( node_t* node = channeledSpells[player].first; node; node = node->next )
		{
			spell_t* sustained = static_cast<spell_t*>(node->element);
			if ( sustained && sustained->runeInstanceId == runeInstanceId )
			{
				sustained->sustain = false;
			}
		}
	}
}

bool breakMagicRune(Uint32 runeInstanceId, const char* message)
{
	if ( multiplayer == CLIENT ) { return false; }
	int owner = -1;
	Item* rune = findMagicRuneByInstanceId(runeInstanceId, &owner);
	if ( !rune ) { return false; }
	rune->status = BROKEN;
	terminateSustainedSpellsForRune(runeInstanceId);
	if ( message ) { messagePlayer(owner, MESSAGE_EQUIPMENT, "%s", message); }
	if ( multiplayer == SERVER && owner > 0 && !players[owner]->isLocalPlayer() )
	{
		strcpy((char*)net_packet->data, "RUSD");
		SDLNet_Write32(rune->runeGetInstanceId(), &net_packet->data[4]);
		net_packet->data[8] = rune->status;
		SDLNet_Write32(rune->appearance, &net_packet->data[9]);
		SDLNet_Write32(static_cast<Uint32>(rune->runeGetStoredPWRRaw()), &net_packet->data[13]);
		net_packet->data[17] = static_cast<Uint8>(rune->runeGetCreatorPlayer());
		SDLNet_Write32(rune->runeGetCreatorIdentity(), &net_packet->data[18]);
		net_packet->address.host = net_clients[owner - 1].host;
		net_packet->address.port = net_clients[owner - 1].port;
		net_packet->len = 22;
		sendPacketSafe(net_sock, -1, net_packet, owner - 1);
	}
	return true;
}

bool applyMagicRuneCastStress(Uint32 runeInstanceId, int resourceEquivalent, int requiredOwner)
{
	if ( multiplayer == CLIENT ) { return true; }
	int owner = -1;
	Item* rune = findMagicRuneByInstanceId(runeInstanceId, &owner, requiredOwner);
	if ( !rune || rune->status == BROKEN ) { return false; }
	spell_t* spell = getSpellFromID(rune->runeGetSpellID());
	real_t breakChance = 0.0;
	if ( spell && spell->ID == SPELL_LEAD_BOLT )
	{
		breakChance = std::max(0, resourceEquivalent) * rune->runeGetStoredResourceFactor();
	}
	else if ( runeUsesVariableMPFactor(spell) )
	{
		breakChance = std::max(0, resourceEquivalent) * rune->runeGetStoredResourceFactor();
	}
	else
	{
		breakChance = rune->runeGetStoredBreakChance();
	}
	breakChance = runeEncodeClamp(breakChance);
	if ( breakChance >= 1.0 || (breakChance > 0.0
		&& local_rng.getU32() < static_cast<Uint32>(breakChance * UINT32_MAX)) )
	{
		Entity* ownerEntity = owner >= 0 && owner < MAXPLAYERS && players[owner]
			? players[owner]->entity : nullptr;
		if ( ownerEntity && ownerEntity->spellEffectPreserveItem(rune) )
		{
			return true;
		}
		rune->status = static_cast<Status>(rune->status - 1);
		messagePlayer(owner, MESSAGE_EQUIPMENT, rune->status == BROKEN
			? "The rune cracks and its magic goes dark."
			: "The rune cracks slightly as it releases its magic.");
		if ( rune->status == BROKEN )
		{
			// Teardown follows the original caster, regardless of current Rune owner.
			terminateSustainedSpellsForRune(runeInstanceId);
		}
	}
	if ( multiplayer == SERVER && owner > 0 && !players[owner]->isLocalPlayer() )
	{
		strcpy((char*)net_packet->data, "RUSD");
		SDLNet_Write32(rune->runeGetInstanceId(), &net_packet->data[4]);
		net_packet->data[8] = rune->status;
		SDLNet_Write32(rune->appearance, &net_packet->data[9]);
		SDLNet_Write32(static_cast<Uint32>(rune->runeGetStoredPWRRaw()), &net_packet->data[13]);
		net_packet->data[17] = static_cast<Uint8>(rune->runeGetCreatorPlayer());
		SDLNet_Write32(rune->runeGetCreatorIdentity(), &net_packet->data[18]);
		net_packet->address.host = net_clients[owner - 1].host;
		net_packet->address.port = net_clients[owner - 1].port;
		net_packet->len = 22;
		sendPacketSafe(net_sock, -1, net_packet, owner - 1);
	}
	return rune->status != BROKEN;
}

bool consumeSustainedSpellResource(Entity* caster, spell_t* spell, int manaCost)
{
	if ( !spell || spell->runeInstanceId == 0 )
	{
		return caster && caster->safeConsumeMP(manaCost);
	}
	const int originalPlayer = spell->runeCasterPlayer;
	if ( originalPlayer < 0 || originalPlayer >= MAXPLAYERS || !caster
		|| !players[originalPlayer] || players[originalPlayer]->entity != caster
		|| caster->getUID() != spell->runeCasterEntityUid
		|| !findMagicRuneByInstanceId(spell->runeInstanceId, nullptr, originalPlayer)
		|| !applyMagicRuneCastStress(spell->runeInstanceId, manaCost, originalPlayer) )
	{
		spell->sustain = false;
		return false;
	}
	return true;
}

std::string Item::runeGetDisplayName() const
{
	if ( !runeIsValid() ) { return items[MAGIC_RUNE].getIdentifiedName(); }
	std::string gemName = items[runeGetVisualGemItemType()].getIdentifiedName();
	if ( !gemName.empty() ) { gemName[0] = static_cast<char>(toupper(gemName[0])); }
	return gemName + " Rune of " + getSpellFromID(runeGetSpellID())->getSpellName();
}

Item* createMagicRune(RuneGemType gem, int spellID, Status status, Sint16 beatitude,
	bool identified, list_t* inventory)
{
	if ( gem >= RUNE_GEM_COUNT || spellID <= SPELL_NONE
		|| spellID > static_cast<int>(RUNE_SPELL_MASK) || !getSpellFromID(spellID) )
	{
		return nullptr;
	}
	Item* rune = newItem(MAGIC_RUNE, status, beatitude, 1, 0, identified, inventory);
	if ( rune )
	{
		rune->runeEnsureInstanceId();
		rune->runeSetGemType(gem);
		rune->runeSetSpellID(spellID);
	}
	return rune;
}

int getItemVariationFromSpellbookOrTome(const Item& item)
{
	int spellID = SPELL_NONE;
	if ( itemCategory(&item) == SPELLBOOK )
	{
		spellID = getSpellIDFromSpellbook(item.type);
	}
	else if ( itemCategory(&item) == TOME_SPELL )
	{
		spellID = item.getTomeSpellID();
	}
	if ( spellID <= SPELL_NONE )
	{
		return -1;
	}
	if ( auto spell = getSpellFromID(spellID) )
	{
		int index = -1;
		switch ( spell->skillID )
		{
		case PRO_SORCERY:
			if ( spell->difficulty <= 20 )
			{
				index = SPELLBOOK_COLOR_SORCERY_1;
			}
			else if ( spell->difficulty <= 60 )
			{
				index = SPELLBOOK_COLOR_SORCERY_2;
			}
			else
			{
				index = SPELLBOOK_COLOR_SORCERY_3;
			}
			break;
		case PRO_MYSTICISM:
			if ( spell->difficulty <= 20 )
			{
				index = SPELLBOOK_COLOR_MYSTICISM_1;
			}
			else if ( spell->difficulty <= 60 )
			{
				index = SPELLBOOK_COLOR_MYSTICISM_2;
			}
			else
			{
				index = SPELLBOOK_COLOR_MYSTICISM_3;
			}
			break;
		case PRO_THAUMATURGY:
			if ( spell->difficulty <= 20 )
			{
				index = SPELLBOOK_COLOR_THAUM_1;
			}
			else if ( spell->difficulty <= 60 )
			{
				index = SPELLBOOK_COLOR_THAUM_2;
			}
			else
			{
				index = SPELLBOOK_COLOR_THAUM_3;
			}
			break;
		default:
			break;
		}
		return index;
	}
	return -1;
}

/*-------------------------------------------------------------------------------

	itemModel

	returns a model index number based on the properties of the given item

-------------------------------------------------------------------------------*/

Sint32 itemModel(const Item* const item, bool shortModel, Entity* creature)
{
	if ( !item || item->type < 0 || item->type >= NUMITEMS )
	{
		return 0;
	}
	if ( item->isMagicRune() )
	{
		const ItemType gem = item->runeGetVisualGemItemType();
		return shortModel ? items[gem].indexShort : items[gem].index;
	}
	if ( item->type == RUNE_HAMMER )
	{
		return shortModel ? items[RUNE_HAMMER].indexShort : items[RUNE_HAMMER].index;
	}

	if ( creature && creature->behavior == &actMonster )
	{
		if ( item->type == IRON_PAULDRONS )
		{
			if ( creature->sprite == 1569 )
			{
				return 2142;
			}
			else if ( creature->sprite == 1570 )
			{
				return 2143;
			}
			return 0;
		}
		else if ( item->type == SHAWL )
		{
			if ( creature->sprite == 1569 )
			{
				if ( item->appearance % items[item->type].variations < 2 )
				{
					return 2144 + item->appearance % items[item->type].variations;
				}
			}
			else if ( creature->sprite == 1570 )
			{
				if ( item->appearance % items[item->type].variations < 2 )
				{
					return 2146 + item->appearance % items[item->type].variations;
				}
			}
			return 0;
		}
		else if ( !shortModel &&
			(item->type == ROBE_WIZARD
				|| item->type == ROBE_MONK
				|| item->type == ROBE_CULTIST
				|| item->type == ROBE_HEALER) )
		{
			if ( item->type == ROBE_WIZARD )
			{
				return 2148;
			}
			else if ( item->type == ROBE_MONK )
			{
				return 2151;
			}
			else if ( item->type == ROBE_CULTIST )
			{
				return 2149;
			}
			else if ( item->type == ROBE_HEALER )
			{
				return 2150;
			}
		}
	}

	int index = shortModel ? items[item->type].indexShort : items[item->type].index;

	if ( item->type == TOOL_PLAYER_LOOT_BAG )
	{
		if ( colorblind_lobby )
		{
			int playerOwner = item->getLootBagPlayer();
			Uint32 playerIndex = 4;
			switch ( playerOwner )
			{
			case 0:
				playerIndex = 2;
				break;
			case 1:
				playerIndex = 3;
				break;
			case 2:
				playerIndex = 1;
				break;
			case 3:
				playerIndex = 4;
				break;
			default:
				break;
			}
			return index + playerIndex;
		}
		else
		{
			return index + item->getLootBagPlayer();
		}
	}
	else if ( item->type == MAGICSTAFF_SCEPTER )
	{
		if ( item->appearance % MAGICSTAFF_SCEPTER_CHARGE_MAX == 0 )
		{
			return index + 2;
		}
		else
		{
			return index;
		}
	}
	else if ( item->type == TOOL_DUCK )
	{
		return items[TOOL_DUCK].index + (item->appearance % items[item->type].variations) / MAXPLAYERS;
	}
	else if ( itemCategory(item) == SPELLBOOK || itemCategory(item) == TOME_SPELL )
	{
		int variation = getItemVariationFromSpellbookOrTome(*item);
		if ( variation >= 0 && variation < items[item->type].variations )
		{
			return index + variation;
		}
	}
	return index + item->appearance % items[item->type].variations;
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
	if ( item->isMagicRune() )
	{
		return items[item->runeGetVisualGemItemType()].fpindex;
	}
	if ( item->type == RUNE_HAMMER )
	{
		return items[RUNE_HAMMER].fpindex;
	}

	if ( item->type == MAGICSTAFF_SCEPTER )
	{
		if ( item->appearance % MAGICSTAFF_SCEPTER_CHARGE_MAX == 0 )
		{
			return items[item->type].fpindex + 2;
		}
		else
		{
			return items[item->type].fpindex;
		}
	}
	else if ( itemCategory(item) == SPELLBOOK || itemCategory(item) == TOME_SPELL )
	{
		int variation = getItemVariationFromSpellbookOrTome(*item);
		if ( variation >= 0 && variation < items[item->type].variations )
		{
			return items[item->type].fpindex + variation;
		}
	}
	else if ( item->type == TOOL_DUCK )
	{
		return items[TOOL_DUCK].fpindex + (item->appearance % items[item->type].variations) / MAXPLAYERS;
	}
	return items[item->type].fpindex + item->appearance % items[item->type].variations;
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
	if ( item1->isMagicRune() && item1->runeGetStoredPWRRaw() != item2->runeGetStoredPWRRaw() )
	{
		return 1;
	}
	if ( item1->isMagicRune() && item1->runeGetCreatorPlayer() != item2->runeGetCreatorPlayer() )
	{
		return 1;
	}
	if ( item1->isMagicRune() && (item1->runeGetInstanceId() != item2->runeGetInstanceId()
		|| item1->runeGetCreatorIdentity() != item2->runeGetCreatorIdentity()) )
	{
		return 1;
	}
	if ( item1->appliedOil != item2->appliedOil )
	{
		return 1;
	}
	if ( itemTypeIsIngredientBag(item1->type) )
	{
		for ( int i = 0; i < 4; ++i )
		{
			if ( item1->ingredientCounts[i] != item2->ingredientCounts[i] ) { return 1; }
		}
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
	//mod add: malformed/outdated mod item data can leave newly added item
	// slots without image variations. Never modulo by zero while comparing
	// those items; treating them as non-matching is the safest fallback.
	if ( item1->type < 0 || item1->type >= NUMITEMS
		|| items[item1->type].variations <= 0 )
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
		|| items[item1->type].category == TOME_SPELL
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

bool playerThrowDuck(const int player, Item* const item, int charge)
{
	if ( !item || item->count == 0 ) { return false; }
	if ( player < 0 || player >= MAXPLAYERS ) { return false; }
	if ( !players[player]->entity ) { return false; }

	if ( players[player]->isLocalPlayer() )
	{
		if ( itemIsEquipped(item, player) )
		{
			if ( multiplayer == CLIENT )
			{
				strcpy((char*)net_packet->data, "DCKA");
				SDLNet_Write32(static_cast<Uint32>(item->type), &net_packet->data[4]);
				SDLNet_Write32(static_cast<Uint32>(item->status), &net_packet->data[8]);
				SDLNet_Write32(static_cast<Uint32>(item->beatitude), &net_packet->data[12]);
				SDLNet_Write32(static_cast<Uint32>(item->count), &net_packet->data[16]);
				SDLNet_Write32(static_cast<Uint32>(item->appearance), &net_packet->data[20]);
				net_packet->data[24] = item->identified;
				net_packet->data[25] = clientnum;
				net_packet->data[26] = charge;
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 27;
				sendPacketSafe(net_sock, -1, net_packet, 0);

				Item** slot = itemSlot(stats[player], item);
				if ( slot != nullptr )
				{
					*slot = nullptr;
				}

				players[player]->paperDoll.updateSlots();

				if ( item->node != nullptr )
				{
					list_RemoveNode(item->node);
				}
				else
				{
					free(item);
				}

				return true;
			}
		}
	}

	if ( multiplayer != CLIENT && players[player]->entity )
	{
		playSoundEntity(players[player]->entity, 75, 64);

		Entity* entity = newEntity(itemModel(item), 1, map.entities, nullptr); // thrown item
		entity->parent = players[player]->entity->getUID();
		entity->x = players[player]->entity->x;
		entity->y = players[player]->entity->y;
		entity->z = players[player]->entity->z;
		entity->yaw = players[player]->entity->yaw;
		entity->sizex = 2;
		entity->sizey = 2;
		entity->behavior = &actThrown;
		entity->flags[UPDATENEEDED] = true;
		entity->flags[PASSABLE] = true;
		entity->skill[10] = item->type;
		entity->skill[11] = item->status;
		entity->skill[12] = item->beatitude;
		entity->skill[13] = 1;
		entity->skill[14] = item->appearance;
		entity->skill[15] = item->identified;
		entity->itemRuneStoredPWR = item->runeGetStoredPWRRaw();
		entity->itemRuneStoredPWRValid = item->runeHasStoredPWR() ? 1 : 0;
		entity->itemRuneCreatorPlayer = item->runeGetCreatorPlayer();
		entity->itemRuneCreatorPlayerValid = item->runeHasCreator() ? 1 : 0;
		entity->itemRuneInstanceId = static_cast<Sint32>(item->runeEnsureInstanceId());
		entity->itemRuneCreatorIdentity = static_cast<Sint32>(item->runeGetCreatorIdentity());
		entity->itemAppliedOil = static_cast<Sint32>(item->appliedOil);
		copyItemPersistentCustomStateToEntity(*entity, *item);

		real_t speed = 1.f + 4.f * (-30 + std::min(50, std::max(30, charge))) / (real_t)(20);
		entity->vel_x = speed * cos(players[player]->entity->yaw);
		entity->vel_y = speed * sin(players[player]->entity->yaw);
		entity->vel_z = -.5;

		Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_THROWN,
			item->type, 1);

		Item** slot = itemSlot(stats[player], item);

		if ( slot != nullptr )
		{
			*slot = nullptr;
		}
		players[player]->paperDoll.updateSlots();

		if ( item->node != nullptr )
		{
			list_RemoveNode(item->node);
		}
		else
		{
			free(item);
		}
		return true;
	}

	return false;
}

bool playerGreasyDropItem(const int player, Item* const item)
{
	if ( !item || item->count == 0 ) { return false; }
	if ( player < 0 || player >= MAXPLAYERS ) { return false; }
	if ( players[player]->isLocalPlayer() )
	{
		if ( !stats[player]->getEffectActive(EFF_GREASY) ) { return false; }
		if ( itemIsEquipped(item, player) )
		{
			Item** slot = itemSlot(stats[player], item);
			if ( !(slot == &stats[player]->weapon || slot == &stats[player]->shield) )
			{
				return false;
			}
			if ( !players[player]->entity ) { return false; }
			bool canDrop = false;
			bool shapeshifted = false;
			if ( players[player]->entity->effectShapeshift != NOTHING )
			{
				shapeshifted = true;
			}
			if ( !shapeshifted ||
				(shapeshifted && slot == &stats[player]->weapon
					&& stats[player]->type == CREATURE_IMP && itemCategory(item) == MAGICSTAFF)
				|| (shapeshifted && slot == &stats[player]->shield
					&& stats[player]->type == CREATURE_IMP && itemCategory(item) == SPELLBOOK) )
			{
				if ( item->beatitude == 0 || stats[player]->type == AUTOMATON
					|| !shouldInvertEquipmentBeatitude(stats[player]) && item->beatitude > 0
					|| shouldInvertEquipmentBeatitude(stats[player]) && item->beatitude < 0 )
				{
					canDrop = true;
				}
			}

			if ( !canDrop )
			{
				return false;
			}
			if ( multiplayer == CLIENT )
			{
				strcpy((char*)net_packet->data, "GRES");
				SDLNet_Write32(static_cast<Uint32>(item->type), &net_packet->data[4]);
				SDLNet_Write32(static_cast<Uint32>(item->status), &net_packet->data[8]);
				SDLNet_Write32(static_cast<Uint32>(item->beatitude), &net_packet->data[12]);
				SDLNet_Write32(static_cast<Uint32>(item->count), &net_packet->data[16]);
				SDLNet_Write32(static_cast<Uint32>(item->appearance), &net_packet->data[20]);
				net_packet->data[24] = item->identified;
				net_packet->data[25] = clientnum;
				net_packet->data[26] = (slot == &stats[player]->weapon) ? 0 : 1;
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 27;
				sendPacketSafe(net_sock, -1, net_packet, 0);

				if ( slot == &stats[player]->weapon )
				{
					messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(636));
				}
				else if ( slot == &stats[player]->shield )
				{
					messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(6246));
				}
				messagePlayer(player, MESSAGE_SPAM_MISC, Language::get(1088), item->description());

				if ( slot != nullptr )
				{
					*slot = nullptr;
				}
				players[player]->paperDoll.updateSlots();

				list_RemoveNode(item->node);

				return true;
			}
		}
	}

	if ( multiplayer != CLIENT && players[player]->entity )
	{
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
		entity->skill[13] = item->count;
		entity->skill[14] = item->appearance;
		entity->skill[15] = item->identified;
		entity->itemRuneStoredPWR = item->runeGetStoredPWRRaw();
		entity->itemRuneStoredPWRValid = item->runeHasStoredPWR() ? 1 : 0;
		entity->itemRuneCreatorPlayer = item->runeGetCreatorPlayer();
		entity->itemRuneCreatorPlayerValid = item->runeHasCreator() ? 1 : 0;
		entity->itemRuneInstanceId = static_cast<Sint32>(item->runeEnsureInstanceId());
		entity->itemRuneCreatorIdentity = static_cast<Sint32>(item->runeGetCreatorIdentity());
		entity->itemAppliedOil = static_cast<Sint32>(item->appliedOil);
		copyItemPersistentCustomStateToEntity(*entity, *item);
		entity->parent = players[player]->entity->getUID();
		entity->itemOriginalOwner = entity->parent;

		// play sound - not in the same tick
		if ( ticks - dropItemSfxTicks[player] > 1 )
		{
			playSoundEntity(players[player]->entity, 47 + local_rng.rand() % 3, 64);
		}
		dropItemSfxTicks[player] = ticks;

		Item** slot = itemSlot(stats[player], item);
		if ( players[player]->isLocalPlayer() )
		{
			if ( slot == &stats[player]->weapon )
			{
				messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(636));
			}
			else if ( slot == &stats[player]->shield )
			{
				messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(6246));
			}
			messagePlayer(player, MESSAGE_SPAM_MISC, Language::get(1088), item->description());
		}

		if ( slot != nullptr )
		{
			*slot = nullptr;
		}
		players[player]->paperDoll.updateSlots();

		if ( item->node != nullptr )
		{
			list_RemoveNode(item->node);
		}
		else
		{
			free(item);
		}
		return true;
	}

	return false;
}

bool dropItem(Item* const item, const int player, const bool notifyMessage, const bool dropAll)
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
		SDLNet_Write32(static_cast<Uint32>(item->runeGetStoredPWRRaw()), &net_packet->data[26]);
		net_packet->data[30] = static_cast<Uint8>(item->runeGetCreatorPlayer());
		SDLNet_Write32(item->runeGetInstanceId(), &net_packet->data[31]);
		SDLNet_Write32(item->runeGetCreatorIdentity(), &net_packet->data[35]);
		net_packet->data[39] = static_cast<Uint8>(item->appliedOil);
		writeIngredientCountsToPacket(*item, net_packet->data, 40);
		writeFamiliarPersistentStateToPacket(*item, net_packet->data, 56);
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 112;
		sendPacketSafe(net_sock, -1, net_packet, 0);
		if ( item == players[player]->bookGUI.openBookItem )
		{
			players[player]->bookGUI.closeBookGUI();
		}

		oldcount = item->count;
		if ( item->count >= 10 && (item->type == TOOL_METAL_SCRAP || item->type == TOOL_MAGIC_SCRAP) )
		{
			int qty = dropAll ? item->count : 10;
			item->count = qty;
			messagePlayer(player, MESSAGE_SPAM_MISC, Language::get(1088), item->description());
			item->count = oldcount - qty;
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
			item->count = dropAll ? 0 : (oldcount - 1);
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
		}

		if ( dropAll )
		{
			qtyToDrop = item->count;
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
		entity->itemRuneStoredPWR = item->runeGetStoredPWRRaw();
		entity->itemRuneStoredPWRValid = item->runeHasStoredPWR() ? 1 : 0;
		entity->itemRuneCreatorPlayer = item->runeGetCreatorPlayer();
		entity->itemRuneCreatorPlayerValid = item->runeHasCreator() ? 1 : 0;
		entity->itemRuneInstanceId = static_cast<Sint32>(item->runeEnsureInstanceId());
		entity->itemRuneCreatorIdentity = static_cast<Sint32>(item->runeGetCreatorIdentity());
		entity->itemAppliedOil = static_cast<Sint32>(item->appliedOil);
		copyItemPersistentCustomStateToEntity(*entity, *item);
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
		entity->itemRuneStoredPWR = item->runeGetStoredPWRRaw();
		entity->itemRuneStoredPWRValid = item->runeHasStoredPWR() ? 1 : 0;
		entity->itemRuneCreatorPlayer = item->runeGetCreatorPlayer();
		entity->itemRuneCreatorPlayerValid = item->runeHasCreator() ? 1 : 0;
		entity->itemRuneInstanceId = static_cast<Sint32>(item->runeEnsureInstanceId());
		entity->itemRuneCreatorIdentity = static_cast<Sint32>(item->runeGetCreatorIdentity());
		entity->itemAppliedOil = static_cast<Sint32>(item->appliedOil);
		copyItemPersistentCustomStateToEntity(*entity, *item);
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
			else if ( monsterStats->type == DUCK_SMALL )
			{
				// drop in center of tile
				int ix = static_cast<int>(std::floor(monster->x)) >> 4;
				int iy = static_cast<int>(std::floor(monster->y)) >> 4;
				entity->x = ix * 16.0 + 8.0;
				entity->y = iy * 16.0 + 8.0;
				entity->z = 4;
				entity->vel_x *= 0.1;
				entity->vel_y *= 0.1;
				entity->vel_z = -.5;
				entity->pitch = -PI / 8;
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
						if ( !((*slot)->identified) )
						{
							Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_APPRAISED, (*slot)->type, 1);
						}
					}
					bool prevIdentified = (*slot)->identified;
					(*slot)->identified = true;
					if ( !prevIdentified )
					{
						Item::onItemIdentified(player, item);
					}
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
					else if ( item->type == BOOMERANG || itemTypeIsThrownBall(item->type) )
					{
					}
					else if ( itemCategory(item) == WEAPON || itemCategory(item) == THROWN )
					{
						playSoundEntity(players[player]->entity, 40 + local_rng.rand() % 4, 64);
					}
					else if ( itemCategory(item) == ARMOR 
						|| item->type == TOOL_TINKERING_KIT 
						|| item->type == TOOL_FRYING_PAN
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
						if ( !((*slot)->identified) )
						{
							Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_APPRAISED, (*slot)->type, 1);
						}
					}
					bool prevIdentified = (*slot)->identified;
					(*slot)->identified = true;
					if ( !prevIdentified )
					{
						Item::onItemIdentified(player, item);
					}
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
				int newCount = item->count;
				if ( players[player]->isLocalPlayer() )
				{
					item->count = (*slot)->count;
				}
				(*slot)->count = newCount; // update quantity. 
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

static void witchbrewReturnEmptyBottle(int player)
{
	if ( multiplayer == CLIENT ) { return; }
	Item* bottle = newItem(POTION_EMPTY, SERVICABLE, 0, 1, 0, true, nullptr);
	itemPickup(player, bottle);
	messagePlayer(player, MESSAGE_INTERACTION, "You recover an empty bottle.");
	free(bottle);
}

static bool useTincture(Item*& item, int player)
{
	if ( !item || !stats[player] || !itemTypeIsTincture(item->type) ) { return false; }
	if ( multiplayer != CLIENT )
	{
		const int stat = static_cast<int>(item->type) - static_cast<int>(TINCTURE_STR);
		if ( !applyTinctureStackToPlayer(item->type, player) ) { return false; }
		tryIncreaseAlchemyFromWitchbrewUse(player);
		static const char* statNames[NUMSTATS] = { "Strength", "Dexterity", "Constitution", "Intelligence", "Perception", "Charisma" };
		messagePlayer(player, MESSAGE_STATUS, "%s affinity tincture stored (%u).",
			statNames[stat], stats[player]->tinctureStacks[stat]);
		witchbrewReturnEmptyBottle(player);
	}
	consumeItem(item, player);
	return true;
}

static Item* chooseOilTarget(int player, bool reversePriority, bool* shieldSlot)
{
	Item* weapon = itemCanReceiveOil(stats[player]->weapon) ? stats[player]->weapon : nullptr;
	Item* quiver = stats[player]->shield && itemTypeIsQuiver(stats[player]->shield->type)
		? stats[player]->shield : nullptr;
	Item* first = reversePriority ? quiver : weapon;
	Item* second = reversePriority ? weapon : quiver;
	Item* result = (first && first->appliedOil == APPLIED_OIL_NONE) ? first
		: ((second && second->appliedOil == APPLIED_OIL_NONE) ? second : (first ? first : second));
	if ( shieldSlot ) { *shieldSlot = result && result == quiver; }
	return result;
}

static bool useOil(Item*& item, int player, bool reversePriority = false)
{
	if ( !item || !stats[player] || !itemTypeIsOil(item->type) ) { return false; }
	bool shieldSlot = false;
	Item* target = chooseOilTarget(player, reversePriority, &shieldSlot);
	if ( !target )
	{
		if ( players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT, "Equip a melee weapon, throwing weapon, or quiver first.");
			playSoundPlayer(player, 90, 64);
		}
		return false;
	}
	target->appliedOil = appliedOilFromItemType(item->type);
	if ( multiplayer != CLIENT )
	{
		tryIncreaseAlchemyFromWitchbrewUse(player);
		serverSyncAppliedOil(player, shieldSlot);
		witchbrewReturnEmptyBottle(player);
	}
	messagePlayer(player, MESSAGE_EQUIPMENT, "You coat %s with %s.", target->getName(), item->getName());
	consumeItem(item, player);
	return true;
}

bool useOilOnItem(Item*& oil, Item* target, int player)
{
	if ( !oil || !target || !itemTypeIsOil(oil->type) || !itemCanReceiveOil(target)
		|| player < 0 || player >= MAXPLAYERS || !stats[player] )
	{
		return false;
	}
	const AppliedOil coating = appliedOilFromItemType(oil->type);
	if ( multiplayer == CLIENT )
	{
		strcpy(reinterpret_cast<char*>(net_packet->data), "WOIL");
		SDLNet_Write32(static_cast<Uint32>(oil->type), &net_packet->data[4]);
		SDLNet_Write32(target->uid, &net_packet->data[8]);
		SDLNet_Write32(static_cast<Uint32>(target->type), &net_packet->data[12]);
		net_packet->data[16] = static_cast<Uint8>(player);
		SDLNet_Write32(oil->uid, &net_packet->data[17]);
		net_packet->data[21] = target == stats[player]->weapon ? 1
			: (target == stats[player]->shield ? 2 : 0);
		net_packet->address = net_server;
		net_packet->len = 22;
		sendPacketSafe(net_sock, -1, net_packet, 0);
		return true;
	}
	target->appliedOil = coating;
	tryIncreaseAlchemyFromWitchbrewUse(player);
	if ( target == stats[player]->weapon )
	{
		serverSyncAppliedOil(player, false);
	}
	else if ( target == stats[player]->shield )
	{
		serverSyncAppliedOil(player, true);
	}
	messagePlayer(player, MESSAGE_EQUIPMENT, "You coat %s with %s.", target->getName(), oil->getName());
	witchbrewReturnEmptyBottle(player);
	consumeItem(oil, player);
	return true;
}

bool useInkOnSpellbook(Item*& ink, Item* book, int player)
{
	if ( !ink || !book || !itemTypeIsInk(ink->type) || !stats[player]
		|| itemCategory(book) != SPELLBOOK ) { return false; }
	if ( multiplayer == CLIENT )
	{
		strcpy(reinterpret_cast<char*>(net_packet->data), "WINK");
		SDLNet_Write32(static_cast<Uint32>(ink->type), &net_packet->data[4]);
		SDLNet_Write32(book->uid, &net_packet->data[8]);
		SDLNet_Write32(static_cast<Uint32>(book->type), &net_packet->data[12]);
		net_packet->data[16] = static_cast<Uint8>(player);
		SDLNet_Write32(ink->uid, &net_packet->data[17]);
		net_packet->data[21] = itemIsEquipped(book, player) ? 1 : 0;
		net_packet->address = net_server;
		net_packet->len = 22;
		sendPacketSafe(net_sock, -1, net_packet, 0);
		return true;
	}
	const ItemType result = getInkSpellbookResult(ink->type, book->type);
	if ( result == ITEM_ENUM_MAX ) { return false; }
	book->type = result;
	tryIncreaseAlchemyFromWitchbrewUse(player);
	messagePlayer(player, MESSAGE_INVENTORY, "The spellbook becomes %s.", book->getName());
	witchbrewReturnEmptyBottle(player);
	consumeItem(ink, player);
	return true;
}

void useItem(Item* item, const int player, Entity* usedBy, bool unequipForDropping, bool serverCheckUse, bool alternateUse)
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
	// mod add: Inventory browsing remains available while Panicking, but using
	// an item is a deliberate character action. Environmental item effects that
	// supply a different source entity are left untouched.
	if ( player >= 0 && player < MAXPLAYERS && players[player]
		&& usedBy == players[player]->entity && playerIsPanicking(player)
		&& !unequipForDropping )
	{
		return;
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

	int equipItemResult = -1;

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
		Sint32 runeStoredPWR = Item::RUNE_STORED_PWR_INVALID;
		Sint8 runeCreatorPlayer = Item::RUNE_CREATOR_INVALID;
		Uint32 runeInstanceId = 0;
		Uint32 runeCreatorIdentity = 0;
		AppliedOil appliedOil = APPLIED_OIL_NONE;
		Item familiarState{};
		bool alternateUse = false;
		bool sendToServer = false;
		void setItem(Item& item)
		{
			type = item.type;
			status = item.status;
			beatitude = item.beatitude;
			count = item.count;
			appearance = item.appearance;
			identified = item.identified;
			runeStoredPWR = item.runeGetStoredPWRRaw();
			runeCreatorPlayer = static_cast<Sint8>(item.runeGetCreatorPlayer());
			runeInstanceId = item.runeGetInstanceId();
			runeCreatorIdentity = item.runeGetCreatorIdentity();
			appliedOil = item.appliedOil;
			copyItemPersistentCustomState(familiarState, item);
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
			SDLNet_Write32(static_cast<Uint32>(runeStoredPWR), &net_packet->data[26]);
			net_packet->data[30] = static_cast<Uint8>(runeCreatorPlayer);
			SDLNet_Write32(runeInstanceId, &net_packet->data[31]);
			SDLNet_Write32(runeCreatorIdentity, &net_packet->data[35]);
			net_packet->data[39] = static_cast<Uint8>(appliedOil);
			net_packet->data[40] = alternateUse ? 1 : 0;
			writeFamiliarPersistentStateToPacket(familiarState, net_packet->data, 41);
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 41 + FAMILIAR_PERSISTENT_PACKET_BYTES;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
	};
	ItemDetailsForServer itemDetailsForServer;
	itemDetailsForServer.alternateUse = alternateUse;

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
		case TINCTURE_STR:
		case TINCTURE_DEX:
		case TINCTURE_CON:
		case TINCTURE_INT:
		case TINCTURE_PER:
		case TINCTURE_CHA:
			useTincture(item, player);
			break;
		case OIL_AETHER:
		case OIL_SILVER:
		case OIL_TOXINS:
		case OIL_BASES:
		case OIL_LEAD:
		case OIL_MINERAL:
		case OIL_BRINE:
			if ( !useOil(item, player, alternateUse) )
			{
				itemDetailsForServer.sendToServer = false;
			}
			break;
		case INK_MYSTICISM:
		case INK_SORCERY:
		case INK_THAUMATURGY:
			itemDetailsForServer.sendToServer = false;
			if ( players[player]->isLocalPlayer() )
			{
				bool hasSpellbook = false;
				for ( node_t* node = stats[player]->inventory.first; node; node = node->next )
				{
					Item* candidate = static_cast<Item*>(node->element);
					if ( candidate && itemCategory(candidate) == SPELLBOOK )
					{
						hasSpellbook = true;
						break;
					}
				}
				if ( hasSpellbook )
				{
					GenericGUI[player].openGUI(GUI_TYPE_ITEMFX, item, item->beatitude, item->type, SPELL_NONE);
				}
				else
				{
					messagePlayer(player, MESSAGE_HINT, "You need a spellbook to use this ink.");
					playSoundPlayer(player, 90, 64);
				}
			}
			break;
		case WOODEN_SHIELD:
			equipItemResult = equipItem(item, &stats[player]->shield, player, checkInventorySpaceForPaperDoll);
			break;
		case QUARTERSTAFF:
		case RUNE_HAMMER:
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
		case SCUTUM:
		case BONE_SHIELD:
		case BLACKIRON_SHIELD:
		case SILVER_SHIELD:
			equipItemResult = equipItem(item, &stats[player]->shield, player, checkInventorySpaceForPaperDoll);
			break;
		case SHORTBOW:
		// mod add: firearms equip in the main hand.
		case FLINTLOCK_PISTOL:
		case MUSKET:
		// mod add end
		case STEEL_HALBERD:
		case STEEL_SWORD:
		case STEEL_MACE:
		case STEEL_AXE:
		case CRYSTAL_SWORD:
		case CRYSTAL_SPEAR:
		case CRYSTAL_BATTLEAXE:
		case CRYSTAL_MACE:
		case HARPOON:     //Mod addon
		case BRONZE_TOMAHAWK:
		case IRON_DAGGER:
		case STEEL_CHAKRAM:
		case CRYSTAL_SHURIKEN:
		case BOOMERANG:
		case RAPIER:
		case GREASE_BALL:
		case DUST_BALL:
		case BOLAS:
		case STEEL_FLAIL:
		case SLOP_BALL:
		case SHILLELAGH_MACE:
		case CLAYMORE_SWORD:
		case ANELACE_SWORD:
		case LANCE_SPEAR:
		case STEEL_FALSHION:
		case STEEL_GREATAXE:
		case BLACKIRON_AXE:
		case BLACKIRON_CROSSBOW:
		case BLACKIRON_DART:
		case BLACKIRON_MACE:
		case BLACKIRON_SWORD:
		case BLACKIRON_TRIDENT:
		case BONE_AXE:
		case BONE_MACE:
		case BONE_SHORTBOW:
		case BONE_SPEAR:
		case BONE_SWORD:
		case BONE_THROWING:
		case SILVER_AXE:
		case SILVER_GLAIVE:
		case SILVER_MACE:
		case SILVER_PLUMBATA:
		case SILVER_SWORD:
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
		case BRANCH_BOW:
		case BRANCH_BOW_INFECTED:
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
		case BONE_BRACERS:
		case BLACKIRON_GAUNTLETS:
		case SILVER_GAUNTLETS:
		case QUILTED_GLOVES:
		case CHAIN_GLOVES:
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
		case CLOAK_GUARDIAN:
		case CLOAK_DENDRITE:
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
		case CLEAT_BOOTS:
		case BONE_BOOTS:
		case BLACKIRON_BOOTS:
		case SILVER_BOOTS:
		case QUILTED_BOOTS:
		case LOAFERS:
		case CHAIN_BOOTS:
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
		case BANDIT_BREASTPIECE:
		case TUNIC_BLOUSE:
		case BONE_BREASTPIECE:
		case BLACKIRON_BREASTPIECE:
		case SILVER_BREASTPIECE:
		case IRON_PAULDRONS:
		case QUILTED_GAMBESON:
		case ROBE_CULTIST:
		case ROBE_HEALER:
		case ROBE_MONK:
		case ROBE_WIZARD:
		case SHAWL:
		case CHAIN_HAUBERK:
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
		case HAT_SILKEN_BOW:
		case HAT_PLUMED_CAP:
		case HAT_BYCOCKET:
		case HAT_TOPHAT:
		case HAT_BANDANA:
		case HAT_CIRCLET:
		case HAT_CROWN:
		case HAT_LAURELS:
		case HAT_TURBAN:
		case HAT_CROWNED_HELM:
		case HAT_WARM:
		case HAT_WOLF_HOOD:
		case HAT_BEAR_HOOD:
		case HAT_STAG_HOOD:
		case HAT_BUNNY_HOOD:
		case HAT_BOUNTYHUNTER:
		case HAT_MITER:
		case HAT_HEADDRESS:
		case HAT_CHEF:
		case HELM_MINING:
		case HAT_CIRCLET_WISDOM:
		case HAT_HOOD_APPRENTICE:
		case HAT_HOOD_ASSASSIN:
		case HAT_HOOD_WHISPERS:
		case BONE_HELM:
		case BLACKIRON_HELM:
		case SILVER_HELM:
		case QUILTED_CAP:
		case CHAIN_COIF:
		case HAT_FELT:
		case HOOD_TEAL:
		case HAT_CIRCLET_SORCERY:
		case HAT_CIRCLET_THAUMATURGY:
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
		case AMULET_BURNINGRESIST:
		case AMULET_FAMILIAR:
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
					if ( stats[player]->playerRace == RACE_AUTOMATON && stats[player]->stat_appearance == 0 )
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
		case BRANCH_STAFF:
		case MAGICSTAFF_SCEPTER:
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
		case RING_RESOLVE:
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
		case SPELLBOOK_METEOR:
		case SPELLBOOK_ICE_WAVE:
		case SPELLBOOK_GUARD_BODY:
		case SPELLBOOK_GUARD_SPIRIT:
		case SPELLBOOK_DIVINE_GUARD:
		case SPELLBOOK_PROF_NIMBLENESS:
		case SPELLBOOK_PROF_GREATER_MIGHT:
		case SPELLBOOK_PROF_COUNSEL:
		case SPELLBOOK_PROF_STURDINESS:
		case SPELLBOOK_BLESS_FOOD:
		case SPELLBOOK_PINPOINT:
		case SPELLBOOK_DONATION:
		case SPELLBOOK_SCRY_ALLIES:
		case SPELLBOOK_SCRY_TRAPS:
		case SPELLBOOK_SCRY_TREASURES:
		case SPELLBOOK_DETECT_ENEMY:
		case SPELLBOOK_TURN_UNDEAD:
		case SPELLBOOK_HEAL_OTHER:
		case SPELLBOOK_BLOOD_WARD:
		case SPELLBOOK_DIVINE_ZEAL:
		case SPELLBOOK_MAXIMISE:
		case SPELLBOOK_MINIMISE:
		case SPELLBOOK_INCOHERENCE:
		case SPELLBOOK_OVERCHARGE:
		case SPELLBOOK_ENVENOM_WEAPON:
		case SPELLBOOK_PSYCHIC_SPEAR:
		case SPELLBOOK_DEFY_FLESH:
		case SPELLBOOK_GREASE_SPRAY:
		case SPELLBOOK_BLOOD_WAVES:
		case SPELLBOOK_COMMAND:
		case SPELLBOOK_METALLURGY:
		case SPELLBOOK_FORGE_KEY:
		case SPELLBOOK_RESHAPE_WEAPON:
		case SPELLBOOK_ALTER_ARROW:
		case SPELLBOOK_VOID_CHEST:
		case SPELLBOOK_LEAD_BOLT:
		case SPELLBOOK_NUMBING_BOLT:
		case SPELLBOOK_CURSE_FLESH:
		case SPELLBOOK_COWARDICE:
		case SPELLBOOK_SEEK_ALLY:
		case SPELLBOOK_DEEP_SHADE:
		case SPELLBOOK_SPIRIT_WEAPON:
		case SPELLBOOK_SPORES:
		case SPELLBOOK_WINDGATE:
		case SPELLBOOK_TELEKINESIS:
		case SPELLBOOK_DISARM:
		case SPELLBOOK_ABUNDANCE:
		case SPELLBOOK_PRESERVE:
		case SPELLBOOK_SABOTAGE:
		case SPELLBOOK_MIST_FORM:
		case SPELLBOOK_FORCE_SHIELD:
		case SPELLBOOK_SPLINTER_GEAR:
		case SPELLBOOK_ATTRACT_ITEMS:
		case SPELLBOOK_ABSORB_MAGIC:
		case SPELLBOOK_TUNNEL:
		case SPELLBOOK_NULL_AREA:
		case SPELLBOOK_FIRE_SPRITE:
		case SPELLBOOK_SPIN:
		case SPELLBOOK_CLEANSE_FOOD:
		case SPELLBOOK_FLAME_CLOAK:
		case SPELLBOOK_LIGHTNING_BOLT:
		case SPELLBOOK_DISRUPT_EARTH:
		case SPELLBOOK_FIRE_WALL:
		case SPELLBOOK_SLAM:
		case SPELLBOOK_IGNITE:
		case SPELLBOOK_SHATTER_OBJECTS:
		case SPELLBOOK_KINETIC_FIELD:
		case SPELLBOOK_THORNS:
		case SPELLBOOK_MAGICIANS_ARMOR:
		case SPELLBOOK_HEAL_MINOR:
		case SPELLBOOK_SIGIL:
		case SPELLBOOK_SANCTUARY:
		case SPELLBOOK_HOLY_BEAM:
		case SPELLBOOK_DOMINATE:
		case SPELLBOOK_ENTRENCH: // mod add: Entrench spellbook
			item_Spellbook(item, player);
			break;
		case TOME_SORCERY:
		case TOME_MYSTICISM:
		case TOME_THAUMATURGY:
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
		case GEM_JEWEL:
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
		case SPYGLASS: // mod add: normal offhand equip/use dispatch
		case MAGIC_RUNE:
		case TOOL_TINKERING_KIT:
		case QUIVER_SILVER:
		case QUIVER_PIERCE:
		case QUIVER_LIGHTWEIGHT:
		case QUIVER_FIRE:
		case QUIVER_KNOCKBACK:
		case QUIVER_CRYSTAL:
		case QUIVER_HUNTING:
		case QUIVER_BONE:
		case QUIVER_BLACKIRON:
		case TOOL_FOCI_FIRE:
		case TOOL_FOCI_SNOW:
		case TOOL_FOCI_NEEDLES:
		case TOOL_FOCI_ARCS:
		case TOOL_FOCI_SAND:
		case TOOL_FOCI_DARK_LIFE:
		case TOOL_FOCI_DARK_RIFT:
		case TOOL_FOCI_DARK_SILENCE:
		case TOOL_FOCI_DARK_VENGEANCE:
		case TOOL_FOCI_DARK_SUPPRESS:
		case TOOL_FOCI_LIGHT_PEACE:
		case TOOL_FOCI_LIGHT_JUSTICE:
		case TOOL_FOCI_LIGHT_PROVIDENCE:
		case TOOL_FOCI_LIGHT_PURITY:
		case TOOL_FOCI_LIGHT_SANCTUARY:
		case INSTRUMENT_FLUTE:
		case INSTRUMENT_LYRE:
		case INSTRUMENT_DRUM:
		case INSTRUMENT_LUTE:
		case INSTRUMENT_HORN:
		case TOOL_FRYING_PAN:
		case TOOL_DUCK:
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
			{
				if ( stats[player]->getEffectActive(EFF_BLEEDING) || (players[player] && players[player]->entity && players[player]->entity->isMerrowPlayer())  ) //mod add Merrow
				{
					consumeItem(item, player);
				}
			}
			break;
		case TOOL_GLASSES:
		case MONOCLE:
		case MASK_SHAMAN:
		case MASK_BANDIT:
		case MASK_EYEPATCH:
		case MASK_MASQUERADE:
		case MASK_MOUTH_ROSE:
		case MASK_MARIGOLD:
		case MASK_GOLDEN:
		case MASK_SPOOKY:
		case MASK_TECH_GOGGLES:
		case MASK_HAZARD_GOGGLES:
		case MASK_PHANTOM:
		case MASK_PIPE:
		case MASK_GRASS_SPRIG:
		case MASK_PLAGUE:
		case MASK_MOUTHKNIFE:
		case MASK_STEEL_VISOR:
		case MASK_CRYSTAL_VISOR:
		case MASK_ARTIFACT_VISOR:
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
		case FOOD_RATION:
		case FOOD_RATION_SPICY:
		case FOOD_RATION_SOUR:
		case FOOD_RATION_BITTER:
		case FOOD_RATION_HEARTY:
		case FOOD_RATION_HERBAL:
		case FOOD_RATION_SWEET:
		case FOOD_SHROOM:
		case FOOD_NUT:
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
			if ( !players[player]->isLocalPlayer() )
			{
				consumeItem(item, player);
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
			if ( !players[player]->isLocalPlayer() )
			{
				consumeItem(item, player);
			}
			break;
		case SPELL_ITEM:
		{
			spell_t* spell = getSpellFromItem(player, item, true);
			if (spell)
			{
				equipSpell(spell, player, item);
			}
			if ( !players[player]->isLocalPlayer() )
			{
				consumeItem(item, player);
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
			Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_CONSUMED, potionType, 1);
			if ( tryLearnPotionRecipe )
			{
				GenericGUI[player].alchemyLearnRecipe(potionType, true);
			}
			if ( tryLevelAppraiseFromPotion )
			{
				if ( stats[player]->getProficiency(PRO_APPRAISAL) < SKILL_LEVEL_BASIC )
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
			const int skillLVL = stats[player]->getModifiedProficiency(PRO_ALCHEMY) / 20;
			if ( tryEmptyBottle && local_rng.rand() % 100 < std::min(80, (60 + skillLVL * 10)) ) // 60 - 80% chance
			{
				Item* emptyBottle = newItem(POTION_EMPTY, SERVICABLE, 0, 1, 0, true, nullptr);
				itemPickup(player, emptyBottle);
				messagePlayer(player, MESSAGE_INTERACTION, Language::get(3351), items[POTION_EMPTY].getIdentifiedName());
				Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_BOTTLES_FROM_CONSUME, POTION_EMPTY, 1);
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
			case AMULET_BURNINGRESIST:
				messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(6535));
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

	if ( serverCheckUse )
	{
		if ( equipItemResult == EQUIP_ITEM_SUCCESS_UPDATE_QTY
			|| equipItemResult == EQUIP_ITEM_FAIL_CANT_UNEQUIP )
		{
			if ( item->node )
			{
				list_RemoveNode(item->node);
			}
			else
			{
				free(item);
			}
		}
		else if ( equipItemResult == -1 )
		{
			if ( item->node )
			{
				list_RemoveNode(item->node);
			}
			else
			{
				free(item);
			}
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
	if ( itemTypeIsWitchIngredient(item->type) )
	{
		const WitchIngredientCategory category = ingredientCategoryFromItem(item->type);
		const int slot = ingredientSlotForItem(item->type);
		const Uint32 amount = static_cast<Uint32>(std::max<Sint16>(1, item->count));
		item->type = ingredientBagForCategory(category);
		item->count = 1;
		if ( slot >= 0 )
		{
			const Uint32 room = std::numeric_limits<Uint32>::max() - item->ingredientCounts[slot];
			item->ingredientCounts[slot] += std::min(room, amount);
		}
	}
	if ( itemTypeIsIngredientBag(item->type) )
	{
		item->count = 1;
	}
	Item* item2;

	/*if ( stats[player]->getProficiency(PRO_APPRAISAL) >= CAPSTONE_UNLOCK_LEVEL[PRO_APPRAISAL] )
	{
		if ( !(player != 0 && multiplayer == SERVER && !players[player]->isLocalPlayer()) )
		{
			if ( !item->identified )
			{
				Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_APPRAISED, item->type, 1);
				item->identified = true;
				item->notifyIcon = true;
				if ( item->type == GEM_GLASS )
				{
					steamStatisticUpdate(STEAM_STAT_RHINESTONE_COWBOY, STEAM_STAT_INT, 1);
				}
			}
		}
	}*/

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
		SDLNet_Write32(static_cast<Uint32>(item->runeGetStoredPWRRaw()), &net_packet->data[29]);
		net_packet->data[33] = static_cast<Uint8>(item->runeGetCreatorPlayer());
		SDLNet_Write32(item->runeGetInstanceId(), &net_packet->data[34]);
		SDLNet_Write32(item->runeGetCreatorIdentity(), &net_packet->data[38]);
		net_packet->data[42] = static_cast<Uint8>(item->appliedOil);
		writeIngredientCountsToPacket(*item, net_packet->data, 43);
		writeFamiliarPersistentStateToPacket(*item, net_packet->data, 59);
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 115;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
	}
	else
	{
		if ( itemTypeIsIngredientBag(item->type) )
		{
			if ( ingredientBagIsEmpty(*item) ) { return nullptr; }
			Item* existing = nullptr;
			if ( addToSpecificInventoryItem
				&& ingredientCategoryFromBag(addToSpecificInventoryItem->type)
					== ingredientCategoryFromBag(item->type) )
			{
				existing = addToSpecificInventoryItem;
			}
			else
			{
				existing = findIngredientBagInInventory(stats[player]->inventory,
					ingredientCategoryFromBag(item->type));
			}
			if ( existing )
			{
				mergeIngredientBags(*existing, *item);
				return existing;
			}
		}
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
				if ( ((itemTypeIsQuiver(item2->type) || itemTypeIsThrownBall(item2->type)) && (item->count + item2->count) >= QUIVER_MAX_AMMO_QTY)
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
						Item** slot = itemSlot(stats[player], item2);
						if ( slot )
						{
							if ( slot == &stats[player]->weapon )
							{
								clientSendEquipUpdateToServer(EQUIP_ITEM_SLOT_WEAPON, EQUIP_ITEM_SUCCESS_UPDATE_QTY, player,
									item2->type, item2->status, item2->beatitude, item2->count, item2->appearance, item2->identified,
									item2->runeGetStoredPWRRaw(), static_cast<Sint8>(item2->runeGetCreatorPlayer()), item2->runeGetInstanceId(), item2->runeGetCreatorIdentity(), item2->appliedOil);
							}
							else if ( slot == &stats[player]->shield )
							{
								clientSendEquipUpdateToServer(EQUIP_ITEM_SLOT_SHIELD, EQUIP_ITEM_SUCCESS_UPDATE_QTY, player,
									item2->type, item2->status, item2->beatitude, item2->count, item2->appearance, item2->identified,
									item2->runeGetStoredPWRRaw(), static_cast<Sint8>(item2->runeGetCreatorPlayer()), item2->runeGetInstanceId(), item2->runeGetCreatorIdentity(), item2->appliedOil);
							}
						}
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
									item2->type, item2->status, item2->beatitude, item2->count, item2->appearance, item2->identified,
									item2->runeGetStoredPWRRaw(), static_cast<Sint8>(item2->runeGetCreatorPlayer()), item2->runeGetInstanceId(), item2->runeGetCreatorIdentity(), item2->appliedOil);
							}
							else if ( slot == &stats[player]->shield )
							{
								clientSendEquipUpdateToServer(EQUIP_ITEM_SLOT_SHIELD, EQUIP_ITEM_SUCCESS_UPDATE_QTY, player,
									item2->type, item2->status, item2->beatitude, item2->count, item2->appearance, item2->identified,
									item2->runeGetStoredPWRRaw(), static_cast<Sint8>(item2->runeGetCreatorPlayer()), item2->runeGetInstanceId(), item2->runeGetCreatorIdentity(), item2->appliedOil);
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
		if ( !appearancesOfSimilarItems.empty() && item && !item->isMagicRune()
			&& item->type >= 0 && item->type < NUMITEMS )
		{
			Item::itemFindUniqueAppearance(item, appearancesOfSimilarItems);
		}

		item2 = newItem(item->type, item->status, item->beatitude, item->count, item->appearance, item->identified, &stats[player]->inventory);
		copyItemPersistentCustomState(*item2, *item);
		item2->ownerUid = item->ownerUid;
		item2->notifyIcon = item->notifyIcon;

		if ( item2->type == TOOL_DUCK && !stats[player]->shield )
		{
			bool shapeshifted = false;
			if ( players[player] && players[player]->entity && players[player]->entity->effectShapeshift != NOTHING )
			{
				shapeshifted = true;
			}

			if ( !shapeshifted && !intro )
			{
				useItem(item2, player);
				auto& hotbar_t = players[player]->hotbar;
				auto& hotbar = hotbar_t.slots();
				if ( hotbar_t.magicDuckHotbarSlot >= 0 )
				{
					hotbar[hotbar_t.magicDuckHotbarSlot].item = item2->uid;
					for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
					{
						if ( i != hotbar_t.magicDuckHotbarSlot && hotbar[i].item == item2->uid )
						{
							hotbar[i].item = 0;
							hotbar[i].resetLastItem();
						}
					}
				}
			}
		}

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
	if ( itemTypeIsThrownBall(this->type) )
	{
		maxStack = QUIVER_MAX_AMMO_QTY - 1;
	}
	else if ( itemCategory(this) == THROWN || itemCategory(this) == GEM )
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
	if ( itemTypeIsIngredientBag(itemToCheck->type) || itemTypeIsIngredientBag(itemDestinationStack->type) )
	{
		newQtyForCheckedItem = itemToCheck->count;
		newQtyForDestItem = itemDestinationStack->count;
		if ( itemToCheck != itemDestinationStack
			&& ingredientCategoryFromBag(itemToCheck->type) != INGREDIENT_CATEGORY_NONE
			&& ingredientCategoryFromBag(itemToCheck->type)
				== ingredientCategoryFromBag(itemDestinationStack->type) )
		{
			newQtyForCheckedItem = 0;
			newQtyForDestItem = 1;
			itemStackResult.resultType = ITEM_ADDED_ENTIRELY_TO_DESTINATION_STACK;
			itemStackResult.itemToStackInto = itemDestinationStack;
		}
		else
		{
			itemStackResult.resultType = ITEM_DESTINATION_NOT_SAME_ITEM;
		}
		return itemStackResult;
	}

	if ( !itemCompare(itemToCheck, itemDestinationStack, false) )
	{
		if ( ((itemTypeIsQuiver(itemDestinationStack->type) || itemTypeIsThrownBall(itemDestinationStack->type)) && (itemToCheck->count + itemDestinationStack->count) >= QUIVER_MAX_AMMO_QTY)
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
			if ( itemDestinationStack->count >= maxStack )
			{
				// can't add anymore to this stack, let's skip over this.
				newQtyForDestItem = itemDestinationStack->count;
				newQtyForCheckedItem = itemToCheck->count;
				itemStackResult.resultType = ITEM_DESTINATION_STACK_IS_FULL;
				itemStackResult.itemToStackInto = itemDestinationStack;
				return itemStackResult;
			}

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
	else if ( openedChest[player] )
	{
		chest_inventory = openedChest[player]->getChestInventoryList();
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
	if ( itemTypeIsIngredientBag(itemToCheck->type) && itemToCheck->node
		&& itemToCheck->node->list == chest_inventory )
	{
		return itemStackResult;
	}

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
	if ( itemTypeIsIngredientBag(itemToCheck->type) && itemToCheck->node
		&& itemToCheck->node->list == &stats[player]->inventory )
	{
		return itemStackResult;
	}

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
	copyItemPersistentCustomStateFromEntity(*item, *entity);
	if ( multiplayer != CLIENT ) { item->runeEnsureInstanceId(); }
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
		attack += 1;
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
	else if ( type == RUNE_HAMMER )
	{
		// Conservative first pass: +2 over Quarterstaff.
		attack += 6;
	}
	else if ( type == ANELACE_SWORD )
	{
		attack += 1;
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
	else if ( type == RAPIER )
	{
		attack += 7;
	}
	else if ( type == IRON_MACE )
	{
		attack += 5;
	}
	else if ( type == IRON_AXE )
	{
		attack += 5;
	}
	else if ( type == SHILLELAGH_MACE )
	{
		attack += 5;
	}
	else if ( type == HARPOON ) //mod addon
	{							//mod addon
		attack += 5;			//mod addon
	}							//mod addon
	else if ( type == BONE_AXE )
	{
		attack += 5;
	}
	else if ( type == BONE_SWORD )
	{
		attack += 5;
	}
	else if ( type == BONE_MACE )
	{
		attack += 5;
	}
	else if ( type == BONE_SPEAR )
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
	else if ( type == STEEL_FLAIL )
	{
		attack += 6;
	}
	else if ( type == CLAYMORE_SWORD )
	{
		attack += 6;
	}
	else if ( type == STEEL_FALSHION )
	{
		attack += 6;
	}
	else if ( type == STEEL_GREATAXE )
	{
		attack += 6;
	}
	else if ( type == LANCE_SPEAR )
	{
		attack += 6;
	}
	else if ( type == BLACKIRON_AXE )
	{
		attack += 6;
	}
	else if ( type == BLACKIRON_MACE )
	{
		attack += 6;
	}
	else if ( type == BLACKIRON_SWORD )
	{
		attack += 6;
	}
	else if ( type == BLACKIRON_TRIDENT )
	{
		attack += 6;
	}
	else if ( type == SILVER_AXE )
	{
		attack += 6;
	}
	else if ( type == SILVER_MACE )
	{
		attack += 6;
	}
	else if ( type == SILVER_SWORD )
	{
		attack += 6;
	}
	else if ( type == SILVER_GLAIVE )
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
	else if ( type == BONE_SHORTBOW )
	{
		attack += 6;
	}
	else if ( type == CROSSBOW )
	{
		attack += 7;
	}
	else if ( type == BLACKIRON_CROSSBOW )
	{
		attack += 7;
	}
	else if ( type == LONGBOW )
	{
		attack += 10;
	}
	else if ( type == BRANCH_BOW )
	{
		attack += 6;
	}
	else if ( type == BRANCH_BOW_INFECTED )
	{
		attack += 8;
	}
	else if ( type == HEAVY_CROSSBOW )
	{
		attack += 14;
	}
	// mod add: firearm base attack
	else if ( type == FLINTLOCK_PISTOL)
	{
		attack += 24;
	}
	else if ( type == MUSKET )
	{
		attack += 30;
	}
	// mod add end
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
	else if ( type == BONE_THROWING )
	{
		attack += 6;
	}
	else if ( type == BOLAS )
	{
		attack += 3;
	}
	else if ( type == GREASE_BALL || type == DUST_BALL )
	{
		attack += 3;
	}
	else if ( type == SLOP_BALL )
	{
		attack += 3;
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
	else if ( type == SILVER_PLUMBATA )
	{
		attack += 10;
	}
	else if ( type == BLACKIRON_DART )
	{
		attack += 10;
	}
	else if ( type == CRYSTAL_SHURIKEN )
	{
		attack += 12;
	}
	else if ( type == TOOL_WHIP )
	{
		attack += 6;
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
	if ( itemCategory(this) != TOOL && itemCategory(this) != THROWN && itemCategory(this) != GEM && itemCategory(this) != POTION
		&& itemCategory(this) != MAGICSTAFF )
	{
		// new formula
		attack += status - 3;
	}

	return attack;
}

bool Item::doesItemProvideBeatitudeAC(ItemType type)
{
	if ( itemTypeIsQuiver(type) || items[type].category == SPELLBOOK
		|| itemTypeIsFoci(type)
		|| itemTypeIsInstrument(type)
		|| type == TOOL_DUCK
		|| items[type].category == AMULET )
	{
		return false;
	}
	if ( items[type].item_slot == EQUIPPABLE_IN_SLOT_HELM )
	{
		if ( type == HAT_SILKEN_BOW
			|| type == HAT_PLUMED_CAP
			|| type == HAT_BYCOCKET
			|| type == HAT_CIRCLET
			|| type == HAT_CIRCLET_SORCERY
			|| type == HAT_CIRCLET_THAUMATURGY
			|| type == HAT_CIRCLET_WISDOM
			|| type == HAT_CROWN 
			|| type == HAT_LAURELS 
			|| type == HAT_TURBAN
			)
		{
			return false;
		}
	}
	else if ( items[type].item_slot == EQUIPPABLE_IN_SLOT_MASK )
	{
		if ( type == MASK_SPOOKY
			|| type == MASK_GOLDEN
			|| type == MASK_STEEL_VISOR
			|| type == MASK_CRYSTAL_VISOR 
			|| type == MASK_ARTIFACT_VISOR
			|| type == MASK_PLAGUE )
		{
			return true;
		}
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
	if ( type == GREASE_BALL )
	{
		return 500;
	}

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
	if ( type == GREASE_BALL )
	{
		return 750;
	}

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

Sint32 Item::getGoldValue() const
{
	if ( type >= 0 && type < NUMITEMS )
	{
		if ( items[type].category == TOME_SPELL )
		{
			int spellID = getTomeSpellID();
			if ( spellID > SPELL_NONE )
			{
				if ( auto spell = getSpellFromID(spellID) )
				{
					if ( spell->difficulty >= 100 )
					{
						return items[type].gold_value + 950;
					}
					else if ( spell->difficulty >= 80 )
					{
						return items[type].gold_value + 550;
					}
					else if ( spell->difficulty >= 60 )
					{
						return items[type].gold_value + 350;
					}
					else if ( spell->difficulty >= 40 )
					{
						return items[type].gold_value + 150;
					}
					else if ( spell->difficulty >= 20 )
					{
						return items[type].gold_value + 50;
					}
				}
			}
		}
		if ( type == GEM_JEWEL )
		{
			int value = items[type].gold_value;
			if ( status == BROKEN )
			{
				value = 0;
			}
			else if ( status == DECREPIT )
			{
				value = 250;
			}
			else if ( status == WORN )
			{
				value = 500;
			}
			else if ( status == SERVICABLE )
			{
				value = 1000;
			}
			else if ( status == EXCELLENT )
			{
				value = 2000;
			}
			return value;
		}
		return items[type].gold_value;
	}
	return 0;
}

Sint32 Item::getWeight() const
{
	if ( type >= 0 && type < NUMITEMS )
	{
		if ( itemTypeIsIngredientBag(type) )
		{
			Uint64 total = 0;
			const WitchIngredientCategory category = ingredientCategoryFromBag(type);
			for ( int i = 0; i < 4; ++i )
			{
				const ItemType ingredient = ingredientForCategorySlot(category, i);
				if ( ingredient >= 0 && ingredient < NUMITEMS )
				{
					total += static_cast<Uint64>(ingredientCounts[i])
						* static_cast<Uint64>(std::max(0, items[ingredient].weight));
				}
			}
			return static_cast<Sint32>(std::min<Uint64>(total, std::numeric_limits<Sint32>::max()));
		}
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

	if ( !Item::doesItemProvideBeatitudeAC(type) )
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
	else if ( type == BONE_HELM )
	{
		armor += 1;
	}
	else if ( type == BLACKIRON_HELM )
	{
		armor += 3;
	}
	else if ( type == SILVER_HELM )
	{
		armor += 3;
	}
	else if ( type == QUILTED_CAP )
	{
		armor += 1;
	}
	else if ( type == CHAIN_COIF )
	{
		armor += 1;
	}
	else if ( type == HAT_FELT )
	{
		armor += 0;
	}
	else if ( type == HOOD_TEAL )
	{
		armor += 0;
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
	else if ( type == WIZARD_DOUBLET 
		|| type == HEALER_DOUBLET
		|| type == MASK_GOLDEN
		|| type == MASK_SPOOKY
		|| type == MASK_PLAGUE
		|| type == HAT_TOPHAT
		|| type == HAT_BANDANA
		|| type == HAT_WARM
		|| type == HAT_MITER
		|| type == HAT_HEADDRESS
		|| type == HAT_CHEF )
	{
		armor += 0;
	}
	else if ( type == VAMPIRE_DOUBLET
		|| type == HELM_MINING
		|| type == HAT_BOUNTYHUNTER
		|| type == MASK_STEEL_VISOR )
	{
		armor += 1;
	}
	else if ( type == HAT_WOLF_HOOD
		|| type == HAT_BEAR_HOOD
		|| type == HAT_STAG_HOOD
		|| type == HAT_BUNNY_HOOD )
	{
		armor += 2;
	}
	else if ( type == MASK_CRYSTAL_VISOR
		|| type == MASK_ARTIFACT_VISOR )
	{
		armor += 2;
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
	else if ( type == IRON_BOOTS || type == IRON_BOOTS_WATERWALKING || type == CLEAT_BOOTS || type == TOOL_FRYING_PAN )
	{
		armor += 2;
	}
	else if ( type == STEEL_BOOTS || type == STEEL_BOOTS_LEVITATION || type == STEEL_BOOTS_FEATHER )
	{
		armor += 3;
	}
	else if ( type == BONE_BRACERS )
	{
		armor += 1;
	}
	else if ( type == BLACKIRON_GAUNTLETS )
	{
		armor += 3;
	}
	else if ( type == SILVER_GAUNTLETS )
	{
		armor += 3;
	}
	else if ( type == QUILTED_GLOVES )
	{
		armor += 1;
	}
	else if ( type == CHAIN_GLOVES )
	{
		armor += 1;
	}
	else if ( type == BONE_BOOTS )
	{
		armor += 1;
	}
	else if ( type == BLACKIRON_BOOTS )
	{
		armor += 3;
	}
	else if ( type == SILVER_BOOTS )
	{
		armor += 3;
	}
	else if ( type == QUILTED_BOOTS )
	{
		armor += 1;
	}
	else if ( type == LOAFERS )
	{
		armor += 0;
	}
	else if ( type == CHAIN_BOOTS )
	{
		armor += 1;
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
	else if ( type == SCUTUM )
	{
		armor += 6;
	}
	else if ( type == BONE_SHIELD )
	{
		armor += 2;
	}
	else if ( type == BLACKIRON_SHIELD )
	{
		armor += 4;
	}
	else if ( type == SILVER_SHIELD )
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
	else if ( type == CLOAK_DENDRITE )
	{
		armor += 1;
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
	else if ( type == BANDIT_BREASTPIECE )
	{
		armor += 2;
	}
	else if ( type == TUNIC_BLOUSE)
	{
		armor += 0;
	}
	else if ( type == BONE_BREASTPIECE)
	{
		armor += 2;
	}
	else if ( type == BLACKIRON_BREASTPIECE)
	{
		armor += 4;
	}
	else if ( type == SILVER_BREASTPIECE)
	{
		armor += 4;
	}
	else if ( type == IRON_PAULDRONS)
	{
		armor += 2;
	}
	else if ( type == QUILTED_GAMBESON)
	{
		armor += 2;
	}
	else if ( type == ROBE_CULTIST)
	{
		armor += 1;
	}
	else if ( type == ROBE_HEALER)
	{
		armor += 1;
	}
	else if ( type == ROBE_MONK)
	{
		armor += 1;
	}
	else if ( type == ROBE_WIZARD)
	{
		armor += 1;
	}
	else if ( type == SHAWL)
	{
		armor += 1;
	}
	else if ( type == CHAIN_HAUBERK)
	{
		armor += 2;
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

	int player = -1;
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( stats[i] == wielder )
		{
			player = i;
			break;
		}
	}

	if ( type == TOOL_DUCK )
	{
		return true;
	}

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
				if ( !identified )
				{
					if ( player >= 0 && players[player]->isLocalPlayer() )
					{
						Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_APPRAISED, type, 1);
					}
				}
				bool prevIdentified = identified;
				identified = true;
				if ( !prevIdentified )
				{
					Item::onItemIdentified(player, this);
				}
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
		if ( !identified )
		{
			if ( player >= 0 && players[player]->isLocalPlayer() )
			{
				Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_APPRAISED, type, 1);
			}
		}
		bool prevIdentified = identified;
		identified = true;
		if ( !prevIdentified )
		{
			Item::onItemIdentified(player, this);
		}
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
	int value = this->getGoldValue(); // base value

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
	value /= (50 + stats[player]->getModifiedProficiency(PRO_TRADING)) / 150.f;

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
		return std::max(value, this->getGoldValue()) * count;
	}

	return std::max(value, this->getGoldValue());
}

/*-------------------------------------------------------------------------------

	Item::sellValue

	returns value of an item to be sold by the given player

-------------------------------------------------------------------------------*/

int Item::sellValue(const int player) const
{
	int value = this->getGoldValue(); // base value

	// identified bonus
	if ( identified )
	{
		value *= 1.20;
	}
	else
	{
		if ( itemCategory(this) == GEM )
		{
			value = items[GEM_GLASS].gold_value;
		}
		else
		{
			value *= .75;
		}
	}

	// cursed and status bonuses
	value *= 1.f + beatitude / 20.f;
	value *= (static_cast<int>(status) + 5) / 10.f;

	if ( player >= 0 )
	{
		// trading bonus
		value *= (50 + stats[player]->getModifiedProficiency(PRO_TRADING)) / 150.f;

		// charisma bonus
		value *= 1.f + statGetCHR(stats[player], players[player]->entity) / 20.f;
	}

	// result
	value = std::max(1, value);

	if ( itemTypeIsQuiver(type) )
	{
		return std::min(value, this->getGoldValue()) * count;
	}

	return std::min(value, this->getGoldValue());
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
					const int skill = std::max(1, stats[player]->getModifiedProficiency(PRO_LOCKPICKING) / 10);
					bool failed = false;
					if ( entity->actTrapSabotaged == 0 )
					{
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
						Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_LOCKPICK_ARROWTRAPS, stats[player]->weapon->type, 1);
					}

					// degrade lockpick.
					if ( !(stats[player]->weapon->type == TOOL_SKELETONKEY) && (local_rng.rand() % 10 == 0 || (failed && local_rng.rand() % 4 == 0))
						&& !(players[player]->entity && players[player]->entity->spellEffectPreserveItem(stats[player]->weapon)) )
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
							SDLNet_Write16((int)stats[player]->weapon->type, &net_packet->data[6]);
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 8;
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

void createCustomInventory(Stat* const stats, const int itemLimit, BaronyRNG& rng)
{
	int itemSlots[6] = { ITEM_SLOT_INV_1, ITEM_SLOT_INV_2, ITEM_SLOT_INV_3, ITEM_SLOT_INV_4, ITEM_SLOT_INV_5, ITEM_SLOT_INV_6 };
	int i = 0;
	Sint32 itemId = -1;
	int itemAppearance = rng.rand();
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
					itemId = itemLevelCurve(static_cast<Category>(category - 1), 0, currentlevel, rng);
				}
				else
				{
					int randType = 0;
					if ( category == 14 )
					{
						// equipment
						randType = rng.rand() % 2;
						if ( randType == 0 )
						{
							itemId = itemLevelCurve(static_cast<Category>(WEAPON), 0, currentlevel, rng);
						}
						else if ( randType == 1 )
						{
							itemId = itemLevelCurve(static_cast<Category>(ARMOR), 0, currentlevel, rng);
						}
					}
					else if ( category == 15 )
					{
						// jewelry
						randType = rng.rand() % 2;
						if ( randType == 0 )
						{
							itemId = itemLevelCurve(static_cast<Category>(AMULET), 0, currentlevel, rng);
						}
						else
						{
							itemId = itemLevelCurve(static_cast<Category>(RING), 0, currentlevel, rng);
						}
					}
					else if ( category == 16 )
					{
						// magical
						randType = rng.rand() % 3;
						if ( randType == 0 )
						{
							itemId = itemLevelCurve(static_cast<Category>(SCROLL), 0, currentlevel, rng);
						}
						else if ( randType == 1 )
						{
							itemId = itemLevelCurve(static_cast<Category>(MAGICSTAFF), 0, currentlevel, rng);
						}
						else
						{
							itemId = itemLevelCurve(static_cast<Category>(SPELLBOOK), 0, currentlevel, rng);
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
					itemStatus = static_cast<Status>(DECREPIT + rng.rand() % 4);
				}
				else if ( itemStatus > BROKEN )
				{
					itemStatus = static_cast<Status>(itemStatus - 1); // reserved '0' for random, so '1' is decrepit... etc to '5' being excellent.
				}
				int itemBless = stats->EDITOR_ITEMS[itemSlots[i] + 2];
				if ( itemBless == 10 )
				{
					itemBless = -1 + rng.rand() % 3;
				}
				const int itemCount = stats->EDITOR_ITEMS[itemSlots[i] + 3];
				if ( stats->EDITOR_ITEMS[itemSlots[i] + 4] == 1 )
				{
					itemIdentified = true;
				}
				else if ( stats->EDITOR_ITEMS[itemSlots[i] + 4] == 2 )
				{
					itemIdentified = rng.rand() % 2;
				}
				else
				{
					itemIdentified = false;
				}
				itemAppearance = rng.rand();
				chance = stats->EDITOR_ITEMS[itemSlots[i] + 5];
				if ( rng.rand() % 100 < chance )
				{
					newItem(static_cast<ItemType>(itemId), itemStatus, itemBless, itemCount, itemAppearance, itemIdentified, &stats->inventory);
				}
				itemsGenerated++;
			}
		}
	}
}

node_t* itemNodeInInventory(const Stat* const myStats, Sint32 itemToFind, const Category cat, bool randomSlot)
{
	if ( myStats == nullptr )
	{
		return nullptr;
	}

	node_t* node = nullptr;
	node_t* nextnode = nullptr;
	std::vector<node_t*> allNodes;
	for ( node = myStats->inventory.first; node != nullptr; node = nextnode )
	{
		nextnode = node->next;
		Item* item = static_cast<Item*>(node->element);
		if ( item != nullptr )
		{
			if ( cat >= WEAPON && itemCategory(item) == cat )
			{
				if ( randomSlot )
				{
					allNodes.push_back(node);
				}
				else
				{
					return node;
				}
			}
			else if ( itemToFind >= 0 && item->type == static_cast<ItemType>(itemToFind) )
			{
				if ( randomSlot )
				{
					allNodes.push_back(node);
				}
				else
				{
					return node;
				}
			}
		}
	}

	if ( randomSlot )
	{
		if ( allNodes.size() > 0 )
		{
			return allNodes.at(local_rng.rand() % allNodes.size());
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
	return isRangedWeapon(item.type);
}

bool isRangedWeapon(const ItemType type)
{
	switch ( type )
	{
	case SLING:
	case SHORTBOW:
	case CROSSBOW:
	case ARTIFACT_BOW:
	case LONGBOW:
	case COMPOUND_BOW:
	case HEAVY_CROSSBOW:
	case BRANCH_BOW:
	case BRANCH_BOW_INFECTED:
	case BLACKIRON_CROSSBOW:
	case BONE_SHORTBOW:
	// mod add: firearms use normal ranged proficiency/attack paths.
	case FLINTLOCK_PISTOL:
	case MUSKET:
	// mod add end
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
	// mod add: Non-player holders may carry a Musket but can never move one
	// into the firing slot through native AI weapon-selection helpers.
	if ( my && my->behavior != &actPlayer && item && item->type == MUSKET )
	{
		return false;
	}
	
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
	copyItemPersistentCustomState(*itemToSet, *itemToCopy);
	itemToSet->identified = itemToCopy->identified;
	itemToSet->uid = itemToCopy->uid;
	itemToSet->ownerUid = itemToCopy->ownerUid;
	itemToSet->isDroppable = itemToCopy->isDroppable;
}

ItemType itemTypeWithinGoldValue(const int cat, const int minValue, const int maxValue, BaronyRNG& rng)
{
	const int numitems = NUMITEMS;
	int numoftype = 0;
	bool chances[NUMITEMS] = { false };
	bool pickAnyCategory = false;
	int c;

	if ( cat < -1 || cat >= Category::CATEGORY_MAX )
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
		if ( items[c].category == cat || (pickAnyCategory && items[c].category < Category::CATEGORY_MAX - 2) )
		{
			if ( items[c].gold_value >= minValue && items[c].gold_value <= maxValue && items[c].level != -1 )
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
	int pick = rng.rand() % numoftype;// rng.rand() % numoftype;
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
	if ( type == AMULET_FAMILIAR )
	{
		return false;
	}
	if ( itemTypeIsIngredientBag(type) )
	{
		return false;
	}
	if ( isMagicRune() )
	{
		return false;
	}
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
				&& this->type != TOOL_FRYING_PAN
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
				&& this->type != TOOL_FRYING_PAN
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
			if ( itemTypeIsThrownBall(this->type) )
			{
				if ( !ignoreStackLimit && count >= QUIVER_MAX_AMMO_QTY - 1 )
				{
					return false;
				}
				return true;
			}
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
			else if ( items[type].hasAttribute("no_stack") )
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
	if ( !wielder ) { return false; }
	if ( wielder->type == SUCCUBUS || wielder->type == INCUBUS )
	{
		return true;
	}
	for ( int player = 0; player < MAXPLAYERS; ++player )
	{
		if ( stats[player] == wielder && players[player] && players[player]->entity
			&& players[player]->entity->isCursedGolemPlayer() )
		{
			return true;
		}
	}
	return false;
}

bool isItemEquippableInShieldSlot(const Item* const item)
{
	if ( !item )
	{
		return false;
	}

	if ( item->type < 0 || item->type >= NUMITEMS )
	{
		return false;
	}
	return (items[item->type].item_slot == EQUIPPABLE_IN_SLOT_SHIELD);
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
			if ( wielder->type == CREATURE_IMP && itemTypeIsFoci(type) )
			{
				return true;
			}
			return false;
		case MAGICSTAFF:
		case SPELLBOOK:
		case TOME_SPELL:
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

int Item::getTomeSpellID() const
{
	if ( type == TOME_SORCERY )
	{
		auto find = spellTomeAppearanceToID[PRO_SORCERY].find(appearance % TOME_APPEARANCE_MAX);
		if ( find == spellTomeAppearanceToID[PRO_SORCERY].end() )
		{
			return SPELL_FORCEBOLT;
		}
		return find->second;
	}
	else if ( type == TOME_MYSTICISM )
	{
		auto find = spellTomeAppearanceToID[PRO_MYSTICISM].find(appearance % TOME_APPEARANCE_MAX);
		if ( find == spellTomeAppearanceToID[PRO_MYSTICISM].end() )
		{
			return SPELL_SLOW;
		}
		return find->second;
	}
	else if ( type == TOME_THAUMATURGY )
	{
		auto find = spellTomeAppearanceToID[PRO_THAUMATURGY].find(appearance % TOME_APPEARANCE_MAX);
		if ( find == spellTomeAppearanceToID[PRO_THAUMATURGY].end() )
		{
			return SPELL_LIGHT;
		}
		return find->second;
	}
	return SPELL_NONE;
}

const char* Item::getTomeLabel() const
{
	if ( itemCategory(this) == TOME_SPELL )
	{
		int spellID = getTomeSpellID();
		if ( auto spell = getSpellFromID(spellID) )
		{
			return spell->getSpellName(true);
		}
	}
	return "";
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

bool itemSpriteIsFociThirdPersonModel(const int sprite)
{
	static std::set<int> fociModels;
	if ( fociModels.size() == 0 )
	{
		for ( int i = 0; i < NUMITEMS; ++i )
		{
			if ( itemTypeIsFoci((ItemType)i) )
			{
				fociModels.insert(items[i].index);
				if ( items[i].indexShort >= 0 )
				{
					fociModels.insert(items[i].indexShort);
				}
			}
		}
	}

	return fociModels.find(sprite) != fociModels.end();
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
	for ( int i = QUIVER_BONE; i <= QUIVER_BLACKIRON; ++i )
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
	for ( int i = QUIVER_BONE; i <= QUIVER_BLACKIRON; ++i )
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
	return (type >= QUIVER_SILVER && type <= QUIVER_HUNTING) || (type >= QUIVER_BONE && type <= QUIVER_BLACKIRON);
}

bool itemTypeIsFoci(const ItemType type)
{
	switch ( type )
	{
	case TOOL_FOCI_FIRE:
	case TOOL_FOCI_SNOW:
	case TOOL_FOCI_NEEDLES:
	case TOOL_FOCI_ARCS:
	case TOOL_FOCI_SAND:
	case TOOL_FOCI_DARK_LIFE:
	case TOOL_FOCI_DARK_RIFT:
	case TOOL_FOCI_DARK_SILENCE:
	case TOOL_FOCI_DARK_VENGEANCE:
	case TOOL_FOCI_DARK_SUPPRESS:
	case TOOL_FOCI_LIGHT_PEACE:
	case TOOL_FOCI_LIGHT_JUSTICE:
	case TOOL_FOCI_LIGHT_PROVIDENCE:
	case TOOL_FOCI_LIGHT_PURITY:
	case TOOL_FOCI_LIGHT_SANCTUARY:
		return true;
		break;
	default:
		break;
	}
	return false;
}

bool itemTypeIsInstrument(const ItemType type)
{
	return (type >= INSTRUMENT_FLUTE && type <= INSTRUMENT_HORN);
}

bool itemTypeIsThrownBall(const ItemType type)
{
	return type == DUST_BALL || type == GREASE_BALL || type == SLOP_BALL;
}

real_t rangedAttackGetSpeedModifier(const Stat* const myStats)
{
	if ( !myStats || !myStats->weapon )
	{
		return 1.0;
	}

	real_t bowModifier = 1.00;
	real_t arrowModifier = 0.0;
	real_t equipmentModifier = 0.0;

	if ( myStats->helmet && myStats->helmet->type == HAT_BYCOCKET )
	{
		if ( myStats->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(myStats) )
		{
			equipmentModifier -= std::min(0.3, 0.1 + 0.1 * abs(myStats->helmet->beatitude));
		}
		else
		{
			equipmentModifier += std::min(0.6, 0.3 * abs(myStats->helmet->beatitude));
		}
	}

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
	else if ( myStats->weapon->type == BRANCH_BOW 
		|| myStats->weapon->type == BRANCH_BOW_INFECTED
		|| myStats->weapon->type == BONE_SHORTBOW )
	{
		bowModifier = 1.0;
	}
	else if ( myStats->weapon->type == ARTIFACT_BOW )
	{
		bowModifier = 0.75;
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
	else if ( myStats->weapon->type == CROSSBOW
		|| myStats->weapon->type == BLACKIRON_CROSSBOW )
	{
		arrowModifier /= 2;
	}
	else if ( myStats->weapon->type == HEAVY_CROSSBOW )
	{
		bowModifier = 0.4;
		return std::max(0.1, bowModifier + arrowModifier + equipmentModifier);
	}
	else
	{
		bowModifier = 1.00;
	}

	return std::max(0.25, bowModifier + arrowModifier + equipmentModifier);
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
	// mod add: Firearms reload from scrap and never use quivers.
	if ( myStats->weapon->isFirearm() )
	{
		return false;
	}
	// mod add end

	if ( myStats->shield && itemTypeIsQuiver(myStats->shield->type) && !(myStats->weapon && myStats->weapon->type == SLING) )
	{
		return true;
	}
	return false;
}

// mod add: firearm state and reload behavior.
static const Uint32 FIREARM_LOADED_APPEARANCE_BIT = 0x80000000u;
static const Uint32 MUSKET_QUEST_JAMMED_APPEARANCE_BIT = 0x40000000u;

bool Item::isFirearm() const
{
	return type == FLINTLOCK_PISTOL || type == MUSKET;
}

bool Item::isUnbreakableFromUse() const
{
	//mod add: Firearms protected from use wear may still be dropped, traded, or deliberately
	// salvaged, and repaired when acquired damaged; only ordinary use/combat
	// durability loss is suppressed.
	return type == FLINTLOCK_PISTOL || type == MUSKET || type == RUNE_HAMMER;
}

bool Item::firearmAppearanceIsLoaded(ItemType type, Uint32 appearance)
{
	return (type == FLINTLOCK_PISTOL || type == MUSKET)
		&& (appearance & FIREARM_LOADED_APPEARANCE_BIT) != 0;
}

Uint32 Item::setFirearmAppearanceLoaded(ItemType type, Uint32 appearance, bool loaded)
{
	if ( type != FLINTLOCK_PISTOL && type != MUSKET )
	{
		return appearance;
	}
	if ( loaded )
	{
		return appearance | FIREARM_LOADED_APPEARANCE_BIT;
	}
	return appearance & ~FIREARM_LOADED_APPEARANCE_BIT;
}

bool Item::firearmIsLoaded() const
{
	return firearmAppearanceIsLoaded(type, appearance);
}

void Item::setFirearmLoaded(bool loaded)
{
	appearance = setFirearmAppearanceLoaded(type, appearance, loaded);
}

bool Item::musketAppearanceIsQuestJammed(ItemType type, Uint32 appearance)
{
	return type == MUSKET
		&& (appearance & MUSKET_QUEST_JAMMED_APPEARANCE_BIT) != 0;
}

Uint32 Item::setMusketAppearanceQuestJammed(ItemType type, Uint32 appearance, bool jammed)
{
	if ( type != MUSKET )
	{
		return appearance;
	}
	if ( jammed )
	{
		return appearance | MUSKET_QUEST_JAMMED_APPEARANCE_BIT;
	}
	return appearance & ~MUSKET_QUEST_JAMMED_APPEARANCE_BIT;
}

bool Item::musketQuestJammed() const
{
	return musketAppearanceIsQuestJammed(type, appearance);
}

void Item::setMusketQuestJammed(bool jammed)
{
	appearance = setMusketAppearanceQuestJammed(type, appearance, jammed);
}

static bool clearQuestJamFromItem(Item* item)
{
	if ( !item || !item->musketQuestJammed() )
	{
		return false;
	}
	item->setMusketQuestJammed(false);
	return true;
}

static bool clearQuestJamFromInventory(Stat* carrierStats)
{
	if ( !carrierStats )
	{
		return false;
	}

	bool found = false;
	for ( node_t* node = carrierStats->inventory.first; node; node = node->next )
	{
		found |= clearQuestJamFromItem(static_cast<Item*>(node->element));
	}

	// Equipment normally points into inventory, but checking the slots explicitly
	// also covers inventory-owning entities whose equipped item is stored separately.
	Item* equipment[] = {
		carrierStats->helmet, carrierStats->breastplate, carrierStats->gloves,
		carrierStats->shoes, carrierStats->shield, carrierStats->weapon,
		carrierStats->cloak, carrierStats->amulet, carrierStats->ring,
		carrierStats->mask
	};
	for ( Item* item : equipment )
	{
		found |= clearQuestJamFromItem(item);
	}
	return found;
}

void receiveLibraryMusketUnlock()
{
	// mod add: Only the local inventory needs mirroring. Remote inventories are
	// authoritative on the server and their equipment presentation has no jam state.
	gameStatistics[STATISTICS_MUSKET_QUEST_UNLOCKED] = 1;
	if ( clientnum >= 0 && clientnum < MAXPLAYERS )
	{
		clearQuestJamFromInventory(stats[clientnum]);
	}
}

bool unlockLibraryMusketFromGnomeDeath()
{
	if ( multiplayer == CLIENT
		|| gameStatistics[STATISTICS_MUSKET_QUEST_UNLOCKED] != 0 )
	{
		return false;
	}

	bool found = false;
	real_t originX = 0.0;
	real_t originY = 0.0;
	real_t originZ = 0.0;
	auto rememberOrigin = [&](Entity* carrier)
	{
		if ( !found && carrier )
		{
			originX = carrier->x;
			originY = carrier->y;
			originZ = carrier->z - 1.0;
		}
		found = true;
	};

	// Prefer player ownership when resolving the audiovisual origin.
	for ( int c = 0; c < MAXPLAYERS; ++c )
	{
		if ( !players[c] || !players[c]->entity || !stats[c] )
		{
			continue;
		}
		if ( clearQuestJamFromInventory(stats[c]) )
		{
			rememberOrigin(players[c]->entity);
		}
	}

	// Then inspect every creature inventory, including equipped-only items.
	if ( map.creatures )
	{
		for ( node_t* node = map.creatures->first; node; node = node->next )
		{
			Entity* carrier = static_cast<Entity*>(node->element);
			if ( carrier && clearQuestJamFromInventory(carrier->getStats()) )
			{
				rememberOrigin(carrier);
			}
		}
	}

	// Finally find the ordinary dropped/map Item entity at its exact world position.
	if ( map.entities )
	{
		for ( node_t* node = map.entities->first; node; node = node->next )
		{
			Entity* entity = static_cast<Entity*>(node->element);
			if ( !entity || entity->behavior != &actItem
				|| entity->skill[10] != MUSKET
				|| !Item::musketAppearanceIsQuestJammed(MUSKET,
					static_cast<Uint32>(entity->skill[14])) )
			{
				continue;
			}
			entity->skill[14] = static_cast<Sint32>(Item::setMusketAppearanceQuestJammed(
				MUSKET, static_cast<Uint32>(entity->skill[14]), false));
			serverUpdateEntitySkill(entity, 14);
			if ( !found )
			{
				originX = entity->x;
				originY = entity->y;
				originZ = entity->z;
			}
			found = true;
		}
	}

	// A valid quest placement always supplies the marked Musket. Do not fabricate
	// an awakening at the Gnome if a malformed map omitted it.
	if ( !found )
	{
		return false;
	}

	gameStatistics[STATISTICS_MUSKET_QUEST_UNLOCKED] = 1;
	if ( multiplayer == SERVER )
	{
		for ( int c = 1; c < MAXPLAYERS; ++c )
		{
			if ( client_disconnected[c] || !players[c] || players[c]->isLocalPlayer() )
			{
				continue;
			}
			strcpy((char*)net_packet->data, "MUQU");
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 4;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}

	playSoundPos(originX, originY, 863, 128);
	spawnFirearmMuzzleFlash(static_cast<Sint16>(originX),
		static_cast<Sint16>(originY), static_cast<Sint16>(originZ), 0.0, true);
	for ( int c = 0; c < MAXPLAYERS; ++c )
	{
		if ( players[c] && (c == 0 || !client_disconnected[c]) )
		{
			messagePlayer(c, MESSAGE_WORLD, "Ak-kelte finally speaks again");
		}
	}
	return true;
}

Sint32 spoilLoadedFirearmsInInventory(int player, bool giveFeedback)
{
	if ( player < 0 || player >= MAXPLAYERS || !stats[player] )
	{
		return 0;
	}

	Sint32 spoiled = 0;
	Item* singleFirearm = nullptr;
	for ( node_t* node = stats[player]->inventory.first; node; node = node->next )
	{
		Item* item = static_cast<Item*>(node->element);
		if ( item && item->firearmIsLoaded() )
		{
			item->setFirearmLoaded(false);
			singleFirearm = item;
			++spoiled;
		}
	}

	if ( giveFeedback && spoiled > 0 && players[player] && players[player]->isLocalPlayer() )
	{
		if ( spoiled == 1 )
		{
			messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(6999), singleFirearm->getName());
		}
		else
		{
			messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(7000), spoiled);
		}
	}
	return spoiled;
}

ItemType Item::firearmReloadMaterialType() const
{
	// mod add: centralized weapon-specific scrap material mapping.
	switch ( type )
	{
		case FLINTLOCK_PISTOL:
			return TOOL_METAL_SCRAP;
		case MUSKET:
			return TOOL_MAGIC_SCRAP;
		default:
			return WOODEN_SHIELD;
	}
}

Sint32 Item::firearmReloadMaterialCost(Sint32 rawTinkering) const
{
	// Reload efficiency uses raw Tinkering only, with no further discount at 100.
	const Sint32 discount = rawTinkering < 50 ? 0
		: std::min<Sint32>(5, 1 + (rawTinkering - 50) / 10);
	switch ( type )
	{
		case FLINTLOCK_PISTOL:
			return std::max<Sint32>(1, 10 - discount);
		case MUSKET:
			return std::max<Sint32>(1, 15 - discount);
		default:
			return 0;
	}
}

Sint32 Item::firearmReloadDuration(Sint32 rawTinkering) const
{
	const bool legendary = rawTinkering >= SKILL_LEVEL_LEGENDARY;
	switch ( type )
	{
		case FLINTLOCK_PISTOL:
			return legendary ? 3 * TICKS_PER_SECOND / 2 : 2 * TICKS_PER_SECOND;
		case MUSKET:
			return legendary ? 2 * TICKS_PER_SECOND : 3 * TICKS_PER_SECOND;
		default:
			return 0;
	}
}

Sint32 Item::firearmJamChancePercent(Sint32 rawTinkering) const
{
	// mod add: Jams deliberately use raw Tinkering (PRO_LOCKPICKING), without
	// equipment, race, or other modified-proficiency bonuses.
	switch ( type )
	{
		case FLINTLOCK_PISTOL:
			return std::max<Sint32>(0, 25 - rawTinkering / 4);
		case MUSKET:
			return std::max<Sint32>(0, 50 - rawTinkering / 2);
		default:
			return 0;
	}
}

Sint32 Item::firearmJamRecoveryScrapCost(Sint32 rawTinkering) const
{
	// mod add: Only Musket jams have a recovery cost. Clamp the lower end so
	// raw proficiency 0-5 is the first five-point band and 51+ is always free.
	if ( type != MUSKET || rawTinkering >= 51 )
	{
		return 0;
	}
	return 10 - ((std::max<Sint32>(rawTinkering, 1) - 1) / 5);
}

Sint32 Item::firearmReloadTrainingCap() const
{
	switch ( type )
	{
		case FLINTLOCK_PISTOL:
			return 60;
		case MUSKET:
			return 100;
		default:
			return 0;
	}
}

static Sint32 playerCountInventoryItemType(int player, ItemType type)
{
	if ( player < 0 || player >= MAXPLAYERS || !stats[player] )
	{
		return 0;
	}
	Sint32 total = 0;
	for ( node_t* node = stats[player]->inventory.first; node; node = node->next )
	{
		Item* item = static_cast<Item*>(node->element);
		if ( item && item->type == type )
		{
			total += item->count;
		}
	}
	return total;
}

static bool playerConsumeInventoryItemType(int player, ItemType type, Sint32 amount)
{
	if ( amount <= 0 )
	{
		return true;
	}
	// Preflight the complete cost. Never partially consume a recovery/reload fee.
	if ( playerCountInventoryItemType(player, type) < amount )
	{
		return false;
	}

	Sint32 remaining = amount;
	for ( node_t* node = stats[player]->inventory.first; node && remaining > 0; )
	{
		node_t* next = node->next;
		Item* item = static_cast<Item*>(node->element);
		if ( item && item->type == type )
		{
			while ( item && remaining > 0 )
			{
				consumeItem(item, player);
				--remaining;
			}
		}
		node = next;
	}
	return remaining == 0;
}

bool firearmUnjamIsActive(int player)
{
	return player >= 0 && player < MAXPLAYERS && players[player]
		&& (players[player]->mechanics.firearmUnjamTicks > 0
			|| players[player]->mechanics.firearmJamAwaitingPayment);
}

void playFirearmJamSound(Entity* wielder)
{
	if ( !wielder )
	{
		return;
	}
	// mod add: Native common Tinkering salvage/disassembly sounds.
	playSoundEntity(wielder, 462 + local_rng.rand() % 2, 64);
}

static void beginFirearmUnjam(Item& firearm, int player, Sint32 duration,
	bool retainsLoadedShot, bool questJammed = false)
{
	if ( player < 0 || player >= MAXPLAYERS || !players[player]
		|| !firearm.isFirearm() || duration <= 0 )
	{
		return;
	}

	Player::PlayerMechanics_t& mechanics = players[player]->mechanics;
	if ( mechanics.firearmUnjamTicks > 0 )
	{
		return;
	}
	mechanics.firearmUnjamTicks = duration;
	mechanics.firearmUnjamItemUid = firearm.uid;
	mechanics.firearmUnjamItemType = firearm.type;
	mechanics.firearmUnjamRetainsLoadedShot = retainsLoadedShot;
	mechanics.firearmUnjamWasQuestJammed = questJammed;
	if ( multiplayer != CLIENT )
	{
		playFirearmJamSound(players[player]->entity);
		// The authoritative message path also delivers this to remote owners.
		messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT,
			Language::get(7001), firearm.getName());
		if ( !retainsLoadedShot && !questJammed )
		{
			messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT,
				Language::get(7017));
		}
	}
}

// Paid jam recovery is independent of reload: payment is immediate and never refunded.
static Uint32 firearmJamSequence = 0;

static void sendFirearmJamResult(int player, ItemType type, Sint32 duration,
	bool retained, Uint32 token)
{
	if ( multiplayer != SERVER || player <= 0 || player >= MAXPLAYERS
		|| !players[player] || players[player]->isLocalPlayer() || client_disconnected[player]
		|| !net_packet || !net_clients ) { return; }
	strcpy((char*)net_packet->data, "FJAM");
	SDLNet_Write32(static_cast<Uint32>(type), &net_packet->data[4]);
	SDLNet_Write16(static_cast<Uint16>(duration), &net_packet->data[8]);
	net_packet->data[10] = retained ? 1 : 0;
	SDLNet_Write32(token, &net_packet->data[11]);
	net_packet->address = net_clients[player - 1];
	net_packet->len = 15;
	sendPacketSafe(net_sock, -1, net_packet, player - 1);
}

static void clearFirearmJamPayment(int player)
{
	auto& m = players[player]->mechanics;
	m.firearmJamToken = 0;
	m.firearmJamAwaitingPayment = false;
	m.firearmJamPaymentCommitted = false;
	m.firearmJamRecoveryCost = 0;
}

void cancelFirearmJamPayment(int player, bool endSession)
{
	if ( player < 0 || player >= MAXPLAYERS || !players[player] ) { return; }
	auto& m = players[player]->mechanics;
	if ( m.firearmJamAwaitingPayment )
	{
		// Duration zero closes the transaction without touching a replacement gun.
		sendFirearmJamResult(player, m.firearmUnjamItemType, 0, false, m.firearmJamToken);
		m.firearmUnjamTicks = 0;
		m.firearmUnjamItemUid = 0;
		m.firearmUnjamItemType = WOODEN_SHIELD;
	}
	clearFirearmJamPayment(player);
	m.firearmJamAttackItemUid = 0;
	if ( endSession ) { m.firearmJamLastToken = 0; }
}

static void sendFirearmJamPayment(int player, ItemType type, Uint32 token, bool paid)
{
	strcpy((char*)net_packet->data, "FJCP");
	net_packet->data[4] = player;
	SDLNet_Write32(static_cast<Uint32>(type), &net_packet->data[5]);
	SDLNet_Write32(token, &net_packet->data[9]);
	net_packet->data[13] = paid ? 1 : 0;
	net_packet->address = net_server;
	net_packet->len = 14;
	sendPacketSafe(net_sock, -1, net_packet, 0);
}

void receiveFirearmJamReady(int player, ItemType type, Sint32 duration,
	Sint32 cost, Uint32 token)
{
	if ( multiplayer != CLIENT || player != clientnum || player < 0 || player >= MAXPLAYERS
		|| !players[player] || !stats[player] || !token || type != MUSKET
		|| duration != TICKS_PER_SECOND / 2 || cost <= 0 ) { return; }
	auto& m = players[player]->mechanics;
	if ( token == m.firearmJamToken && m.firearmJamAwaitingPayment )
	{
		// Cache failures too: newly acquired Scrap cannot change this jam's decision.
		sendFirearmJamPayment(player, m.firearmUnjamItemType, token, m.firearmJamPaymentCommitted);
		return;
	}
	if ( token <= m.firearmJamLastToken ) { return; }
	if ( m.firearmJamAwaitingPayment )
	{
		sendFirearmJamPayment(player, type, token, false);
		return;
	}
	Item* firearm = stats[player]->weapon;
	// Use the maximum from the unchanged cost helper, not a second cost formula.
	Item costProbe = {};
	costProbe.type = MUSKET;
	if ( cost > costProbe.firearmJamRecoveryScrapCost(0) ) { return; }
	const bool valid = players[player]->entity && stats[player]->HP > 0
		&& !client_disconnected[player] && firearm && firearm->type == type
		&& firearm->uid == m.firearmJamAttackItemUid && !firearm->musketQuestJammed()
		&& !firearm->firearmIsLoaded() && m.firearmUnjamTicks == 0;
	m.firearmJamLastToken = token;
	m.firearmJamToken = token;
	m.firearmJamAwaitingPayment = true;
	m.firearmJamRecoveryCost = cost;
	m.firearmUnjamItemUid = valid ? firearm->uid : 0;
	m.firearmUnjamItemType = type;
	m.firearmUnjamTicks = duration; // held until final FJAM, never advanced while pending
	m.firearmJamAttackItemUid = 0;
	m.firearmJamPaymentCommitted = valid
		&& playerConsumeInventoryItemType(player, TOOL_MAGIC_SCRAP, cost);
	sendFirearmJamPayment(player, type, token, m.firearmJamPaymentCommitted);
}

void receiveFirearmJamPayment(int player, ItemType type, Uint32 token, bool paid)
{
	if ( multiplayer != SERVER || player <= 0 || player >= MAXPLAYERS
		|| !players[player] || players[player]->isLocalPlayer() || client_disconnected[player] ) { return; }
	auto& m = players[player]->mechanics;
	if ( !token || token != m.firearmJamToken || !m.firearmJamAwaitingPayment
		|| type != m.firearmUnjamItemType ) { return; }
	Item* firearm = stats[player] ? stats[player]->weapon : nullptr;
	const bool valid = players[player]->entity && stats[player] && stats[player]->HP > 0
		&& firearm && firearm->uid == m.firearmUnjamItemUid && firearm->type == type
		&& firearm->firearmIsLoaded() && !firearm->musketQuestJammed();
	const Sint32 duration = valid ? m.firearmUnjamTicks : 0;
	// Disarm before callbacks; a duplicate FJCP cannot start a second clearing action.
	m.firearmUnjamTicks = 0;
	m.firearmUnjamItemUid = 0;
	m.firearmUnjamItemType = WOODEN_SHIELD;
	clearFirearmJamPayment(player);
	if ( valid )
	{
		firearm->setFirearmLoaded(paid);
		beginFirearmUnjam(*firearm, player, duration, paid);
	}
	sendFirearmJamResult(player, type, duration, paid, token);
}

bool tryQuestJamMusket(Item& firearm, int player)
{
	if ( multiplayer == CLIENT || player < 0 || player >= MAXPLAYERS
		|| !players[player] || !stats[player] || !players[player]->entity
		|| playerIsPanicking(player)
		|| !firearm.musketQuestJammed() )
	{
		return false;
	}

	Player::PlayerMechanics_t& mechanics = players[player]->mechanics;
	if ( mechanics.firearmReloadTicks > 0 || firearmUnjamIsActive(player) )
	{
		return true;
	}

	const Sint32 duration = TICKS_PER_SECOND / 2;
	const bool retainsLoadedShot = firearm.firearmIsLoaded();
	beginFirearmUnjam(firearm, player, duration, retainsLoadedShot, true);
	if ( multiplayer == SERVER && player > 0 && !players[player]->isLocalPlayer() )
	{
		sendFirearmJamResult(player, firearm.type, duration, retainsLoadedShot, 0);
	}
	return true;
}

bool tryJamFirearm(Item& firearm, int player)
{
	if ( multiplayer == CLIENT || player < 0 || player >= MAXPLAYERS
		|| !players[player] || !stats[player] || !players[player]->entity
		|| playerIsPanicking(player)
		|| !firearm.isFirearm() || firearm.musketQuestJammed()
		|| !firearm.firearmIsLoaded() )
	{
		return false;
	}

	Player::PlayerMechanics_t& mechanics = players[player]->mechanics;
	if ( mechanics.firearmReloadTicks > 0 || firearmUnjamIsActive(player) )
	{
		return false;
	}

	const Sint32 rawTinkering = stats[player]->getProficiency(PRO_LOCKPICKING);
	const Sint32 jamChance = firearm.firearmJamChancePercent(rawTinkering);
	if ( jamChance <= 0 || local_rng.rand() % 100 >= jamChance )
	{
		return false;
	}

	bool retainsLoadedShot = true;
	const Sint32 recoveryCost = firearm.firearmJamRecoveryScrapCost(rawTinkering);
	if ( multiplayer == SERVER && player > 0 && !players[player]->isLocalPlayer()
		&& firearm.type == MUSKET && recoveryCost > 0 )
	{
		if ( ++firearmJamSequence == 0 ) { ++firearmJamSequence; }
		mechanics.firearmJamToken = firearmJamSequence;
		mechanics.firearmJamAwaitingPayment = true;
		mechanics.firearmJamRecoveryCost = recoveryCost;
		mechanics.firearmUnjamItemUid = firearm.uid;
		mechanics.firearmUnjamItemType = firearm.type;
		mechanics.firearmUnjamTicks = TICKS_PER_SECOND / 2;
		mechanics.firearmUnjamRetainsLoadedShot = true;
		mechanics.firearmUnjamWasQuestJammed = false;
		strcpy((char*)net_packet->data, "FJRD");
		SDLNet_Write32(static_cast<Uint32>(firearm.type), &net_packet->data[4]);
		SDLNet_Write16(static_cast<Uint16>(mechanics.firearmUnjamTicks), &net_packet->data[8]);
		net_packet->data[10] = static_cast<Uint8>(recoveryCost);
		SDLNet_Write32(mechanics.firearmJamToken, &net_packet->data[11]);
		net_packet->address = net_clients[player - 1];
		net_packet->len = 15;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
		return true; // this trigger pull jammed; never resume it as a shot
	}
	if ( recoveryCost > 0 )
	{
		if ( !playerConsumeInventoryItemType(player, TOOL_MAGIC_SCRAP, recoveryCost) )
		{
			// The full recovery price cannot be paid: consume nothing and spoil
			// the chambered shot while retaining the short jam-clear action.
			retainsLoadedShot = false;
			firearm.setFirearmLoaded(false);
		}
	}

	const Sint32 duration = TICKS_PER_SECOND / 2;
	beginFirearmUnjam(firearm, player, duration, retainsLoadedShot);
	if ( multiplayer == SERVER && player > 0 && !players[player]->isLocalPlayer() )
	{
		sendFirearmJamResult(player, firearm.type, duration, retainsLoadedShot, 0);
	}
	return true;
}

void receiveFirearmJam(int player, ItemType type, Sint32 duration,
	bool retainsLoadedShot, Uint32 token)
{
	if ( multiplayer != CLIENT || player != clientnum || player < 0 || player >= MAXPLAYERS
		|| !stats[player] || !players[player] ) { return; }
	auto& m = players[player]->mechanics;
	Item* firearm = stats[player]->weapon;
	bool matches = firearm && firearm->isFirearm() && firearm->type == type;
	if ( token )
	{
		if ( token != m.firearmJamToken || !m.firearmJamAwaitingPayment
			|| type != m.firearmUnjamItemType )
		{
			// SAFE is reliable but not ordered. A cancellation can precede FJRD;
			// remember it so the delayed request cannot charge a later interaction.
			if ( duration == 0 && token > m.firearmJamLastToken )
			{
				m.firearmJamLastToken = token;
			}
			return;
		}
		matches = matches && firearm->uid == m.firearmUnjamItemUid;
		m.firearmUnjamTicks = 0;
		m.firearmUnjamItemUid = 0;
		m.firearmUnjamItemType = WOODEN_SHIELD;
		clearFirearmJamPayment(player);
	}
	else if ( m.firearmJamAwaitingPayment ) { return; }
	m.firearmJamAttackItemUid = 0;
	// Payment was already committed at FJRD. A zero-duration result only closes
	// an invalidated transaction and must not alter a replacement firearm.
	if ( !matches || duration <= 0 ) { return; }
	firearm->setFirearmLoaded(retainsLoadedShot);
	beginFirearmUnjam(*firearm, player, duration, retainsLoadedShot,
		firearm->musketQuestJammed());
}

void updateFirearmUnjam(int player)
{
	if ( player < 0 || player >= MAXPLAYERS || !players[player] )
	{
		return;
	}

	Player::PlayerMechanics_t& mechanics = players[player]->mechanics;
	if ( mechanics.firearmJamAwaitingPayment )
	{
		if ( !players[player]->entity || !stats[player] || stats[player]->HP <= 0
			|| client_disconnected[player] ) { cancelFirearmJamPayment(player); }
		return; // the normal half-second clear starts only after the payment decision
	}
	if ( mechanics.firearmUnjamTicks <= 0 )
	{
		return;
	}
	// mod add: A jam-clear cannot finish while Panicking. Pause the existing
	// action instead of restarting it or charging its already-decided recovery
	// cost again; it resumes normally when the effect ends.
	if ( playerIsPanicking(player) )
	{
		return;
	}

	Item* firearm = stats[player] ? stats[player]->weapon : nullptr;
	const bool actionStillValid = players[player]->entity && stats[player] && stats[player]->HP > 0
		&& firearm && firearm->uid == mechanics.firearmUnjamItemUid
		&& firearm->type == mechanics.firearmUnjamItemType
		&& firearm->isFirearm()
		&& firearm->firearmIsLoaded() == mechanics.firearmUnjamRetainsLoadedShot;
	if ( !actionStillValid )
	{
		mechanics.firearmUnjamTicks = 0;
		mechanics.firearmUnjamItemUid = 0;
		mechanics.firearmUnjamItemType = WOODEN_SHIELD;
		mechanics.firearmUnjamRetainsLoadedShot = true;
		mechanics.firearmUnjamWasQuestJammed = false;
		return;
	}

	--mechanics.firearmUnjamTicks;
	if ( mechanics.firearmUnjamTicks > 0 )
	{
		return;
	}

	// Clear first so completion cannot be observed twice, including during the
	// callbacks and network updates performed by increaseSkill().
	const ItemType completedFirearmType = mechanics.firearmUnjamItemType;
	const bool completedQuestJam = mechanics.firearmUnjamWasQuestJammed;
	mechanics.firearmUnjamItemUid = 0;
	mechanics.firearmUnjamItemType = WOODEN_SHIELD;
	mechanics.firearmUnjamRetainsLoadedShot = true;
	mechanics.firearmUnjamWasQuestJammed = false;
	if ( !completedQuestJam && players[player]->isLocalPlayer() )
	{
		messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT,
			Language::get(7002), firearm->getName());
	}
	if ( multiplayer != CLIENT )
	{
		const Sint32 rawTinkering = stats[player]->getProficiency(PRO_LOCKPICKING);
		// Flintlock retains its existing guaranteed call. Musket progression is
		// reserved for established tinkerers and stops at raw proficiency 100.
		if ( !completedQuestJam && (completedFirearmType == FLINTLOCK_PISTOL
			|| (completedFirearmType == MUSKET
				&& rawTinkering >= 60 && rawTinkering < 100)) )
		{
			players[player]->entity->increaseSkill(PRO_LOCKPICKING);
		}
	}
}

static Sint32 playerCountFirearmReloadMaterials(const Item& firearm, int player)
{
	if ( player < 0 || player >= MAXPLAYERS || !stats[player] || !firearm.isFirearm() )
	{
		return 0;
	}
	return playerCountInventoryItemType(player, firearm.firearmReloadMaterialType());
}

static bool playerConsumeFirearmReloadMaterials(const Item& firearm, int player)
{
	// Match the preflight cost even if proficiency changes during this reload.
	const Sint32 materialCost = players[player]->mechanics.firearmReloadMaterialCost;
	return playerConsumeInventoryItemType(player,
		firearm.firearmReloadMaterialType(), materialCost);
}

// Tokens are action identities, never Item UIDs. Keep the sequence across player init.
static Uint32 firearmReloadSequence = 0;

static bool remoteFirearmOwner(int player)
{
	return multiplayer == SERVER && player > 0 && players[player]
		&& !players[player]->isLocalPlayer();
}

static void sendFirearmReloadResult(int player, ItemType type,
	Uint32 appearance, bool success, Uint32 token)
{
	if ( !remoteFirearmOwner(player) || client_disconnected[player] || !net_packet || !net_clients ) { return; }
	strcpy((char*)net_packet->data, "FRLD");
	SDLNet_Write32(static_cast<Uint32>(type), &net_packet->data[4]);
	SDLNet_Write32(appearance, &net_packet->data[8]);
	net_packet->data[12] = success ? 1 : 0;
	SDLNet_Write32(token, &net_packet->data[13]);
	net_packet->address = net_clients[player - 1];
	net_packet->len = 17;
	sendPacketSafe(net_sock, -1, net_packet, player - 1);
}

static void clearFirearmReload(int player)
{
	auto& m = players[player]->mechanics;
	m.firearmReloadTicks = 0;
	m.firearmReloadTotalTicks = 0;
	m.firearmReloadMaterialCost = 0;
	m.firearmReloadItemUid = 0;
	m.firearmReloadItemType = WOODEN_SHIELD;
	m.firearmReloadToken = 0;
	m.firearmReloadAwaitingPayment = false;
	m.firearmReloadMaterialsCommitted = false;
	m.firearmReloadCommittedMaterialType = WOODEN_SHIELD;
	m.firearmReloadCommittedMaterialCost = 0;
	m.firearmReloadRefundOnGround = false;
	m.firearmReloadDeathX = 0;
	m.firearmReloadDeathY = 0;
}

static bool firearmReloadValid(int player)
{
	auto& m = players[player]->mechanics;
	Item* firearm = stats[player] ? stats[player]->weapon : nullptr;
	return players[player]->entity && stats[player] && stats[player]->HP > 0
		&& !client_disconnected[player] && !playerIsPanicking(player)
		&& m.firearmReloadTicks > 0 && m.firearmUnjamTicks == 0
		&& firearm && firearm->uid == m.firearmReloadItemUid
		&& firearm->type == m.firearmReloadItemType && firearm->isFirearm()
		&& !firearm->firearmIsLoaded() && !firearm->musketQuestJammed();
}

static void refundFirearmReload(int player)
{
	auto& m = players[player]->mechanics;
	if ( !m.firearmReloadMaterialsCommitted || !stats[player] ) { return; }
	// Clear before inventory callbacks: a repeated result must never refund twice.
	m.firearmReloadMaterialsCommitted = false;
	Item* material = newItem(m.firearmReloadCommittedMaterialType, DECREPIT,
		0, m.firearmReloadCommittedMaterialCost, 0, true, nullptr);
	if ( m.firearmReloadRefundOnGround && net_packet )
	{
		// Death already sent ordinary inventory through DIEI. Add the late refund
		// to that same death drop, instead of an inventory that respawn will erase.
		strcpy((char*)net_packet->data, "DIEI");
		SDLNet_Write32(static_cast<Uint32>(material->type), &net_packet->data[4]);
		SDLNet_Write32(static_cast<Uint32>(material->status), &net_packet->data[8]);
		SDLNet_Write32(0, &net_packet->data[12]);
		SDLNet_Write32(material->count, &net_packet->data[16]);
		SDLNet_Write32(0, &net_packet->data[20]);
		net_packet->data[24] = 1;
		net_packet->data[25] = player;
		net_packet->data[26] = m.firearmReloadDeathX;
		net_packet->data[27] = m.firearmReloadDeathY;
		net_packet->address = net_server;
		net_packet->len = 28;
		sendPacketSafe(net_sock, -1, net_packet, 0);
	}
	else { itemPickup(player, material); }
	free(material);
}

void cancelFirearmReload(int player, bool endSession)
{
	if ( player < 0 || player >= MAXPLAYERS || !players[player] ) { return; }
	auto& m = players[player]->mechanics;
	if ( m.firearmReloadToken )
	{
		sendFirearmReloadResult(player, m.firearmReloadItemType, 0, false, m.firearmReloadToken);
	}
	// A paid transaction survives entity removal until the server result arrives.
	// Refunding speculatively could refund a reload the server already completed.
	if ( multiplayer == CLIENT && m.firearmReloadMaterialsCommitted && !endSession )
	{
		m.firearmReloadTicks = 0;
		return;
	}
	if ( multiplayer == CLIENT ) { refundFirearmReload(player); }
	clearFirearmReload(player);
}

void receiveFirearmReloadResult(int player, ItemType type, Uint32 appearance,
	bool success, Uint32 token)
{
	if ( multiplayer != CLIENT || player < 0 || player >= MAXPLAYERS
		|| !players[player] || !stats[player] ) { return; }
	auto& m = players[player]->mechanics;
	if ( !token || token != m.firearmReloadToken || type != m.firearmReloadItemType ) { return; }
	Item* firearm = stats[player]->weapon;
	if ( success && m.firearmReloadMaterialsCommitted && firearm
		&& firearm->uid == m.firearmReloadItemUid && firearm->type == type && firearm->isFirearm() )
	{
		firearm->appearance = appearance;
		messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT,
			Language::get(7005), firearm->getName());
	}
	if ( !success ) { refundFirearmReload(player); }
	clearFirearmReload(player);
}

static void sendFirearmReloadPayment(int player, ItemType type, Uint32 token, bool success)
{
	strcpy((char*)net_packet->data, "FRCP");
	net_packet->data[4] = player;
	SDLNet_Write32(static_cast<Uint32>(type), &net_packet->data[5]);
	SDLNet_Write32(token, &net_packet->data[9]);
	net_packet->data[13] = success ? 1 : 0;
	net_packet->address = net_server;
	net_packet->len = 14;
	sendPacketSafe(net_sock, -1, net_packet, 0);
}

void receiveFirearmReloadReady(int player, ItemType type, Uint32 token)
{
	if ( multiplayer != CLIENT || player < 0 || player >= MAXPLAYERS || !players[player] ) { return; }
	auto& m = players[player]->mechanics;
	if ( !token || m.firearmReloadToken != token || m.firearmReloadItemType != type )
	{
		sendFirearmReloadPayment(player, type, token, false);
		return;
	}
	if ( m.firearmReloadMaterialsCommitted ) { return; } // reliable payment already queued
	bool paid = false;
	if ( firearmReloadValid(player) )
	{
		Item& firearm = *stats[player]->weapon;
		// Reload materials are committed only at completion, from the owning inventory.
		paid = playerConsumeFirearmReloadMaterials(firearm, player);
		if ( paid )
		{
			m.firearmReloadMaterialsCommitted = true;
			m.firearmReloadCommittedMaterialType = firearm.firearmReloadMaterialType();
			m.firearmReloadCommittedMaterialCost = m.firearmReloadMaterialCost;
		}
		else
		{
			messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(7006),
				items[firearm.firearmReloadMaterialType()].getIdentifiedName(), firearm.getName());
			playSoundPlayer(player, 90, 64);
		}
	}
	sendFirearmReloadPayment(player, type, token, paid);
	if ( !paid ) { clearFirearmReload(player); }
}

static void completeFirearmReload(int player)
{
	auto& mechanics = players[player]->mechanics;
	Item* firearm = stats[player]->weapon;
	firearm->setFirearmLoaded(true);
	sendFirearmReloadResult(player, firearm->type, firearm->appearance, true, mechanics.firearmReloadToken);
	clearFirearmReload(player); // disarm before sound/training callbacks
	if ( multiplayer != CLIENT )
	{
		// mod add: Dedicated loaded/ready report at the successful transition.
		playSoundEntity(players[player]->entity, 861, 255);
	}
	// mod add: Only a completed, paid reload may train raw Tinkering.
	// The authoritative side owns both the 20% roll and skill award.
	const Sint32 rawTinkering = stats[player]->getProficiency(PRO_LOCKPICKING);
	if ( multiplayer != CLIENT && rawTinkering < firearm->firearmReloadTrainingCap()
		&& local_rng.rand() % 100 < 20 && players[player]->entity )
	{
		players[player]->entity->increaseSkill(PRO_LOCKPICKING);
	}
	if ( players[player]->isLocalPlayer() )
	{
		messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT,
			Language::get(7005), firearm->getName());
	}
}

void receiveFirearmReloadPayment(int player, ItemType type, Uint32 token, bool success)
{
	if ( player <= 0 || player >= MAXPLAYERS || !remoteFirearmOwner(player) ) { return; }
	auto& m = players[player]->mechanics;
	if ( !token || token != m.firearmReloadToken || type != m.firearmReloadItemType
		|| !m.firearmReloadAwaitingPayment ) { return; }
	if ( success && firearmReloadValid(player) )
	{
		// Disarm before sound/training callbacks; payment cannot commit twice.
		m.firearmReloadAwaitingPayment = false;
		completeFirearmReload(player);
	}
	else { cancelFirearmReload(player); }
}

bool tryReloadFirearm(Item& firearm, int player)
{
	if ( player < 0 || player >= MAXPLAYERS || !players[player] || !stats[player]
		|| !players[player]->entity || stats[player]->HP <= 0 || stats[player]->weapon != &firearm
		|| (playerIsUsingSpyglass(players[player]->entity) && firearm.type != MUSKET)
		|| playerIsPanicking(player) || !firearm.isFirearm() || firearm.musketQuestJammed()
		|| firearm.firearmIsLoaded() ) { return false; }
	auto& m = players[player]->mechanics;
	if ( m.firearmReloadTicks > 0 || m.firearmUnjamTicks > 0
		|| m.firearmReloadMaterialsCommitted ) { return false; }
	const ItemType materialType = firearm.firearmReloadMaterialType();
	const Sint32 rawTinkering = stats[player]->getProficiency(PRO_LOCKPICKING);
	const Sint32 materialCost = firearm.firearmReloadMaterialCost(rawTinkering);
	if ( !remoteFirearmOwner(player) && playerCountFirearmReloadMaterials(firearm, player) < materialCost )
	{
		if ( players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(7003),
				materialCost, items[materialType].getIdentifiedName(), firearm.getName());
			playSoundPlayer(player, 90, 64);
		}
		return false;
	}
	// Sample cost and timing once; payment, refund and HUD use this transaction.
	m.firearmReloadMaterialCost = materialCost;
	m.firearmReloadTotalTicks = firearm.firearmReloadDuration(rawTinkering);
	m.firearmReloadTicks = m.firearmReloadTotalTicks;
	m.firearmReloadItemUid = firearm.uid;
	m.firearmReloadItemType = firearm.type;
	if ( multiplayer == CLIENT )
	{
		if ( ++firearmReloadSequence == 0 ) { ++firearmReloadSequence; }
		m.firearmReloadToken = firearmReloadSequence;
		strcpy((char*)net_packet->data, "FRST");
		net_packet->data[4] = player;
		SDLNet_Write32(static_cast<Uint32>(firearm.type), &net_packet->data[5]);
		SDLNet_Write32(m.firearmReloadToken, &net_packet->data[9]);
		net_packet->address = net_server;
		net_packet->len = 13;
		sendPacketSafe(net_sock, -1, net_packet, 0);
	}
	else { playFirearmJamSound(players[player]->entity); }
	if ( players[player]->isLocalPlayer() )
	{
		messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(7004), firearm.getName());
	}
	return true;
}

void receiveFirearmReloadStart(int player, ItemType type, Uint32 token)
{
	if ( player <= 0 || player >= MAXPLAYERS || !remoteFirearmOwner(player)
		|| client_disconnected[player] || !token ) { return; }
	auto& m = players[player]->mechanics;
	if ( token <= m.firearmReloadLastToken ) { return; }
	m.firearmReloadLastToken = token;
	Item* firearm = stats[player] ? stats[player]->weapon : nullptr;
	if ( firearm && firearm->type == type && tryReloadFirearm(*firearm, player) )
	{
		m.firearmReloadToken = token;
	}
	else { sendFirearmReloadResult(player, type, 0, false, token); }
}

void updateFirearmReload(int player)
{
	if ( player < 0 || player >= MAXPLAYERS || !players[player] ) { return; }
	auto& m = players[player]->mechanics;
	if ( m.firearmReloadTicks <= 0 ) { return; }
	if ( !firearmReloadValid(player) ) { cancelFirearmReload(player); return; }
	if ( m.firearmReloadAwaitingPayment || (multiplayer == CLIENT && m.firearmReloadTicks == 1) ) { return; }
	if ( --m.firearmReloadTicks > 0 ) { return; }
	Item* firearm = stats[player]->weapon;
	if ( remoteFirearmOwner(player) )
	{
		m.firearmReloadTicks = 1; // retain action/weapon lock until payment or cancellation
		m.firearmReloadAwaitingPayment = true;
		strcpy((char*)net_packet->data, "FRDY");
		SDLNet_Write32(static_cast<Uint32>(firearm->type), &net_packet->data[4]);
		SDLNet_Write32(m.firearmReloadToken, &net_packet->data[8]);
		net_packet->address = net_clients[player - 1];
		net_packet->len = 12;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
		return;
	}
	// Host/singleplayer inventory is authoritative; preserve completion-time payment.
	if ( playerConsumeFirearmReloadMaterials(*firearm, player) ) { completeFirearmReload(player); }
	else
	{
		if ( players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT | MESSAGE_EQUIPMENT, Language::get(7006),
				items[firearm->firearmReloadMaterialType()].getIdentifiedName(), firearm->getName());
			playSoundPlayer(player, 90, 64);
		}
		clearFirearmReload(player);
	}
}
// mod add end

real_t getArtifactWeaponEffectChance(const ItemType type, Stat& wielder, real_t* const effectAmount)
{
	if ( type == ARTIFACT_AXE )
	{
		const real_t percent = 25 * (wielder.getModifiedProficiency(PRO_AXE)) / 100.f; //0-25%
		if ( effectAmount )
		{
			*effectAmount = 1.5; //1.5x damage.
		}

		return percent;
	}
	else if ( type == ARTIFACT_SWORD )
	{
		const real_t percent = (wielder.getModifiedProficiency(PRO_SWORD)); //0-100%
		if ( effectAmount )
		{
			*effectAmount = (wielder.getModifiedProficiency(PRO_SWORD)) / 400.f + 0.25; //0.25x-0.5x add to weapon multiplier
		}

		return percent;
	}
	else if ( type == ARTIFACT_SPEAR )
	{
		const real_t percent = 25 * (wielder.getModifiedProficiency(PRO_POLEARM)) / 100.f; //0-25%
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
			*effectAmount = wielder.getModifiedProficiency(PRO_MACE); // 0-2 second bonus mana regen
		}

		return percent;
	}
	else if ( type == ARTIFACT_BOW )
	{
		const real_t percent = wielder.getModifiedProficiency(PRO_RANGED) / 2.f; //0-50%
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
	const int skillLVL = myStats->getModifiedProficiency(PRO_LOCKPICKING) / 20; // 0-5.
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
		const Sint32 runeStoredPWR = item->runeGetStoredPWRRaw();
		const Sint8 runeCreatorPlayer = static_cast<Sint8>(item->runeGetCreatorPlayer());
		const Uint32 runeInstanceId = item->runeGetInstanceId();
		const Uint32 runeCreatorIdentity = item->runeGetCreatorIdentity();
		const AppliedOil appliedOil = item->appliedOil;

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
						type, status, beatitude, count, appearance, identified, runeStoredPWR, runeCreatorPlayer,
						runeInstanceId, runeCreatorIdentity, appliedOil);
				}
			}
			else
			{
				clientSendEquipUpdateToServer(EQUIP_ITEM_SLOT_WEAPON, equipResult, player,
				type, status, beatitude, count, appearance, identified, runeStoredPWR, runeCreatorPlayer,
				runeInstanceId, runeCreatorIdentity, appliedOil);
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

void clientSendItemTypeUpdateToServer(const int player, Item* item, ItemType prevItemType)
{
	if ( multiplayer != CLIENT ) { return; }
	if ( !item || !itemIsEquipped(item, player) || items[item->type].item_slot == NO_EQUIP )
	{
		return;
	}
	strcpy((char*)net_packet->data, "EQUT");
	SDLNet_Write32(static_cast<Uint32>(prevItemType), &net_packet->data[4]);
	SDLNet_Write32(static_cast<Uint32>(item->status), &net_packet->data[8]);
	SDLNet_Write32(static_cast<Uint32>(item->beatitude), &net_packet->data[12]);
	SDLNet_Write32(static_cast<Uint32>(item->count), &net_packet->data[16]);
	SDLNet_Write32(static_cast<Uint32>(item->appearance), &net_packet->data[20]);
	net_packet->data[24] = item->identified;
	net_packet->data[25] = player;
	net_packet->data[26] = items[item->type].item_slot;
	SDLNet_Write32(static_cast<Uint32>(item->type), &net_packet->data[27]);
	net_packet->address.host = net_server.host;
	net_packet->address.port = net_server.port;
	net_packet->len = 31;
	sendPacketSafe(net_sock, -1, net_packet, 0);
}

void clientSendEquipUpdateToServer(const EquipItemSendToServerSlot slot, const EquipItemResult equipType, const int player,
	const ItemType type, const Status status, const Sint16 beatitude, const int count, const Uint32 appearance, const bool identified,
	const Sint32 runeStoredPWR, const Sint8 runeCreatorPlayer, const Uint32 runeInstanceId,
	const Uint32 runeCreatorIdentity, const AppliedOil appliedOil)
{
	AppliedOil oilToSend = appliedOil;
	// Older quantity-only call sites do not have the item metadata at hand. Preserve
	// the equipped instance's oil rather than silently clearing it on the server.
	if ( oilToSend == APPLIED_OIL_NONE && player >= 0 && player < MAXPLAYERS && stats[player] )
	{
		Item* equipped = slot == EQUIP_ITEM_SLOT_WEAPON ? stats[player]->weapon
			: (slot == EQUIP_ITEM_SLOT_SHIELD ? stats[player]->shield : nullptr);
		if ( equipped && equipped->type == type && equipped->appearance == appearance )
		{
			oilToSend = equipped->appliedOil;
		}
	}
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
	SDLNet_Write32(static_cast<Uint32>(runeStoredPWR), &net_packet->data[28]);
	net_packet->data[32] = static_cast<Uint8>(runeCreatorPlayer);
	SDLNet_Write32(runeInstanceId, &net_packet->data[33]);
	SDLNet_Write32(runeCreatorIdentity, &net_packet->data[37]);
	net_packet->data[41] = static_cast<Uint8>(oilToSend);
	Item* familiarItem = nullptr;
	if ( type == AMULET_FAMILIAR && player >= 0 && player < MAXPLAYERS && stats[player] )
	{
		familiarItem = stats[player]->amulet;
	}
	Item emptyFamiliarState{};
	writeFamiliarPersistentStateToPacket(familiarItem ? *familiarItem : emptyFamiliarState,
		net_packet->data, 42);
	net_packet->address.host = net_server.host;
	net_packet->address.port = net_server.port;
	net_packet->len = 42 + FAMILIAR_PERSISTENT_PACKET_BYTES;
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
		item->type, item->status, item->beatitude, item->count, item->appearance, item->identified,
		item->runeGetStoredPWRRaw(), static_cast<Sint8>(item->runeGetCreatorPlayer()),
		item->runeGetInstanceId(), item->runeGetCreatorIdentity(), item->appliedOil);
}

int Item::getDuckPlayer() const
{
	return (int)(appearance % items[type].variations) % MAXPLAYERS;
}

int Item::getLootBagPlayer() const
{
	return (int)(appearance & 0xF) % MAXPLAYERS;
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

void Item::itemFindUniqueAppearance(Item* tempItem, std::unordered_set<Uint32>& appearancesOfSimilarItems)
{
	// MAGIC_RUNE appearance is structured persistent metadata, not disposable
	// cosmetic uniqueness data. Separate Rune instances are distinguished by UID.
	if ( !tempItem || tempItem->isMagicRune() )
	{
		return;
	}
	if ( !appearancesOfSimilarItems.empty() && tempItem )
	{
		Uint32 originalAppearance = tempItem->appearance;
		int originalVariation = originalAppearance % items[tempItem->type].variations;

		int tries = 100;
		bool robot = false;
		// we need to find a unique appearance within the list.
		if ( tempItem->type == TOOL_SENTRYBOT || tempItem->type == TOOL_SPELLBOT || tempItem->type == TOOL_GYROBOT
			|| tempItem->type == TOOL_DUMMYBOT )
		{
			robot = true;
			tempItem->appearance += (local_rng.rand() % 100000) * 10;
		}
		else if ( tempItem->type == MAGICSTAFF_SCEPTER )
		{
			tempItem->appearance = ((local_rng.rand() % 10000) * (MAGICSTAFF_SCEPTER_CHARGE_MAX)) + (originalAppearance % MAGICSTAFF_SCEPTER_CHARGE_MAX);
		}
		else if ( itemCategory(tempItem) == TOME_SPELL )
		{
			tempItem->appearance = ((local_rng.rand() % 10000) * (TOME_APPEARANCE_MAX)) + (originalAppearance % TOME_APPEARANCE_MAX);
		}
		else
		{
			tempItem->appearance = local_rng.rand();
			if ( tempItem->appearance % items[tempItem->type].variations != originalVariation )
			{
				// we need to match the variation for the new appearance, take the difference so new varation matches
				int change = (tempItem->appearance % items[tempItem->type].variations - originalVariation);
				if ( tempItem->appearance < change ) // underflow protection
				{
					tempItem->appearance += items[tempItem->type].variations;
				}
				tempItem->appearance -= change;
				int newVariation = tempItem->appearance % items[tempItem->type].variations;
				assert(newVariation == originalVariation);
			}
		}
		auto it = appearancesOfSimilarItems.find(tempItem->appearance);
		while ( it != appearancesOfSimilarItems.end() && tries > 0 )
		{
			if ( robot )
			{
				tempItem->appearance += (local_rng.rand() % 100000) * 10;
			}
			else if ( tempItem->type == MAGICSTAFF_SCEPTER )
			{
				tempItem->appearance = ((local_rng.rand() % 10000) * (MAGICSTAFF_SCEPTER_CHARGE_MAX)) + (originalAppearance % MAGICSTAFF_SCEPTER_CHARGE_MAX);
			}
			else if ( itemCategory(tempItem) == TOME_SPELL )
			{
				tempItem->appearance = ((local_rng.rand() % 10000) * (TOME_APPEARANCE_MAX)) + (originalAppearance % TOME_APPEARANCE_MAX);
			}
			else
			{
				tempItem->appearance = local_rng.rand();
				if ( tempItem->appearance % items[tempItem->type].variations != originalVariation )
				{
					// we need to match the variation for the new appearance, take the difference so new varation matches
					int change = (tempItem->appearance % items[tempItem->type].variations - originalVariation);
					if ( tempItem->appearance < change ) // underflow protection
					{
						tempItem->appearance += items[tempItem->type].variations;
					}
					tempItem->appearance -= change;
					int newVariation = tempItem->appearance % items[tempItem->type].variations;
					assert(newVariation == originalVariation);
				}
			}
			it = appearancesOfSimilarItems.find(tempItem->appearance);
			--tries;
		}
	}
}

void Item::onItemIdentified(int player, Item* tempItem)
{
	if ( player >= 0 && player < MAXPLAYERS && players[player]->isLocalPlayer() && stats[player] )
	{
		std::unordered_set<Uint32> appearancesOfSimilarItems;
		for ( node_t* node = stats[player]->inventory.first; node != NULL; node = node->next )
		{
			Item* item2 = static_cast<Item*>(node->element);
			if ( item2 && item2 != tempItem && !itemCompare(tempItem, item2, true) )
			{
				// items are the same (incl. appearance!)
				// if they shouldn't stack, we need to change appearance of the new item.
				appearancesOfSimilarItems.insert(item2->appearance);
			}
		}

		Item::itemFindUniqueAppearance(tempItem, appearancesOfSimilarItems);

		if ( multiplayer == CLIENT && itemIsEquipped(tempItem, player) && players[player]->paperDoll.isItemOnDoll(*tempItem) )
		{
			clientSendAppearanceUpdateToServer(player, tempItem, true);
		}
	}
}
