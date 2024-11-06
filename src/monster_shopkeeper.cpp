/*-------------------------------------------------------------------------------

	BARONY
	File: monster_shopkeeper.cpp
	Desc: implements all of the shopkeeper's code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "monster.hpp"
#include "engine/audio/sound.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "magic/magic.hpp"
#include "shops.hpp"
#include "mod_tools.hpp"
#include "prng.hpp"

std::vector<Item*> generateShopkeeperConsumables(Entity& my, Stat& myStats, int storetype)
{
	auto& rng = my.entity_rng ? *my.entity_rng : local_rng;

	std::vector<Item*> itemsGenerated;
	if ( ShopkeeperConsumables_t::entries.find(storetype) == ShopkeeperConsumables_t::entries.end() )
	{
		return itemsGenerated;
	}

	for ( auto& slots : ShopkeeperConsumables_t::entries[storetype] )
	{
		const int tradingReq = slots.slotTradingReq;

		std::vector<unsigned int> chances;
		for ( auto& slot : slots.itemEntries )
		{
			chances.push_back(slot.weightedChance);
		}
		int result = rng.discrete(chances.data(), chances.size());
		auto& slot = slots.itemEntries.at(result);

		if ( rng.rand() % 100 >= slot.percentChance )
		{
			continue; // no spawn
		}
		if ( slot.emptyItemEntry )
		{
			continue;
		}

		ItemType type = slot.type[rng.uniform(0, slot.type.size() - 1)];
		Status status = slot.status[rng.uniform(0, slot.status.size() - 1)];
		Sint16 beatitude = slot.beatitude[rng.uniform(0, slot.beatitude.size() - 1)];
		Sint16 count = slot.count[rng.uniform(0, slot.count.size() - 1)];
		Uint32 appearance = 0;
		if ( slot.appearance.empty() )
		{
			appearance = rand();
		}
		else
		{
			appearance = slot.appearance[rng.uniform(0, slot.appearance.size() - 1)];
		}
		bool identified = slot.identified[rng.uniform(0, slot.identified.size() - 1)];
		
		if ( Item* item = newItem(type, status, beatitude, count, appearance, identified, &myStats.inventory) )
		{
			item->itemRequireTradingSkillInShop = tradingReq;
			item->itemSpecialShopConsumable = true;
			if ( rng.rand() % 100 >= slot.dropChance )
			{
				// no drop
				item->isDroppable = false;
			}
			else
			{
				item->isDroppable = true;
			}
			itemsGenerated.push_back(item);
		}
	}
	std::vector<Item*> shuffled;
	while ( !itemsGenerated.empty() )
	{
		size_t index = rng.rand() % itemsGenerated.size();
		shuffled.push_back(itemsGenerated[index]);
		itemsGenerated.erase(itemsGenerated.begin() + index);
	}
	std::sort(shuffled.begin(), shuffled.end(), [](const Item* lhs, const Item* rhs) {
		return lhs->itemRequireTradingSkillInShop < rhs->itemRequireTradingSkillInShop;
	});
	int previousReq = -1;
	for ( auto it = shuffled.begin(); it != shuffled.end(); ++it )
	{
		if ( (*it)->itemRequireTradingSkillInShop <= previousReq )
		{
			(*it)->itemRequireTradingSkillInShop = std::min(previousReq + 1, 5);
		}
		previousReq = (*it)->itemRequireTradingSkillInShop;
		(*it)->itemRequireTradingSkillInShop = std::min((*it)->itemRequireTradingSkillInShop, (Uint8)5);
	}
	std::sort(shuffled.begin(), shuffled.end(), [](const Item* lhs, const Item* rhs) {
		return lhs->itemRequireTradingSkillInShop < rhs->itemRequireTradingSkillInShop;
	});
	return shuffled;
}

void initShopkeeper(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	my->flags[BURNABLE] = true;
	my->initMonster(217);
	my->z = -1;

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = -1;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 1;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		auto& rng = my->entity_rng ? *my->entity_rng : local_rng;

		my->createPathBoundariesNPC();

		for ( int x = my->monsterPathBoundaryXStart - 16; x <= my->monsterPathBoundaryXEnd + 16; x += 16 )
		{
			for ( int y = my->monsterPathBoundaryYStart - 16; y <= my->monsterPathBoundaryYEnd + 16; y += 16 )
			{
				if ( x / 16 >= 0 && x / 16 < map.width && y / 16 >= 0 && y / 16 < map.height )
				{
					shoparea[y / 16 + (x / 16)*map.height] = true;
				}
			}
		}

		if ( myStats != NULL )
		{
			if ( !myStats->leader_uid )
			{
				myStats->leader_uid = 0;
			}

			if ( !strcmp(myStats->name, "") )
			{
				strcpy(myStats->name, Language::get(158 + rng.rand() % 26));
			}

			// apply random stat increases if set in stat_shared.cpp or editor
			setRandomMonsterStats(myStats, rng);

			if ( currentlevel >= 25 && myStats->HP == 300 && myStats->MAXHP == 300 )
			{
				myStats->HP *= 2;
				myStats->MAXHP *= 2;
				myStats->OLDHP = myStats->HP;
			}

			// generate 6 items max, less if there are any forced items from boss variants
			int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;

			// boss variants

			// random effects
			if ( rng.rand() % 20 == 0 )
			{
				myStats->EFFECTS[EFF_ASLEEP] = true;
				myStats->EFFECTS_TIMERS[EFF_ASLEEP] = 1800 + rng.rand() % 3600;
			}

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats, rng);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate, rng);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

														 // count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			my->setHardcoreStats(*myStats);

			// generate the default inventory items for the monster, provided the editor sprite allowed enough default slots
			switch ( defaultItems )
			{
				case 6:
				case 5:
				case 4:
				case 3:
				case 2:
				case 1:
					break;
				default:
					break;
			}

			//give weapon
			if ( myStats->weapon == NULL && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
			{
				if ( currentlevel < 25 )
				{
					myStats->weapon = newItem(SPELLBOOK_MAGICMISSILE, EXCELLENT, 0, 1, 0, false, NULL);
				}
				else
				{
					if ( rng.rand() % 2 == 0 )
					{
						myStats->weapon = newItem(SPELLBOOK_DRAIN_SOUL, EXCELLENT, 0, 1, 0, false, NULL);
					}
					else
					{
						myStats->weapon = newItem(SPELLBOOK_BLEED, EXCELLENT, 0, 1, 0, false, NULL);
					}
				}
			}

			// give shopkeeper items
			if ( myStats->MISC_FLAGS[STAT_FLAG_NPC] == 14 )
			{
				myStats->MISC_FLAGS[STAT_FLAG_MYSTERIOUS_SHOPKEEP] = 1;
				my->monsterStoreType = 10;
			}
			else if ( myStats->MISC_FLAGS[STAT_FLAG_NPC] > 0 )
			{
				my->monsterStoreType = myStats->MISC_FLAGS[STAT_FLAG_NPC] - 1;
				if ( my->monsterStoreType > 9 )
				{
					my->monsterStoreType = rng.rand() % 10;
				}
			}
			else
			{
				my->monsterStoreType = rng.rand() % 10;
			}
			int numitems = 10 + rng.rand() % 5;
			int blessedShopkeeper = 1; // bless important pieces of gear like armor, jewelry, weapons..

			int shoplevel = currentlevel;
			if ( gameModeManager.currentSession.challengeRun.isActive(GameModeManager_t::CurrentSession_t::ChallengeRun_t::CHEVENT_SHOPPING_SPREE) )
			{
				shoplevel = std::max(15, shoplevel);
			}

			if ( shoplevel >= 30 )
			{
				if ( rng.rand() % 3 == 0 )
				{
					blessedShopkeeper = 3;
				}
				else
				{
					blessedShopkeeper = 2;
				}
			}
			else if ( shoplevel >= 25 )
			{
				if ( rng.rand() % 4 == 0 )
				{
					blessedShopkeeper = 3;
				}
				else
				{
					blessedShopkeeper = 2;
				}
			}
			else if ( shoplevel >= 18 )
			{
				if ( rng.rand() % 3 == 0 )
				{
					blessedShopkeeper = 2;
				}
			}

			int customShopkeeperInUse = ((myStats->MISC_FLAGS[STAT_FLAG_SHOPKEEPER_CUSTOM_PROPERTIES] >> 12) & 0xF);
			int oldMonsterStoreType = my->monsterStoreType;
			if ( customShopkeeperInUse == MonsterStatCustomManager::StatEntry::ShopkeeperCustomFlags::ENABLE_GEN_ITEMS )
			{
				if ( (myStats->MISC_FLAGS[STAT_FLAG_SHOPKEEPER_CUSTOM_PROPERTIES] & 0xFF) > 1 )
				{
					numitems = (myStats->MISC_FLAGS[STAT_FLAG_SHOPKEEPER_CUSTOM_PROPERTIES] & 0xFF) - 1;
				}
				if ( ((myStats->MISC_FLAGS[STAT_FLAG_SHOPKEEPER_CUSTOM_PROPERTIES] >> 8) & 0xF) > 0 )
				{
					blessedShopkeeper = std::max(1, (myStats->MISC_FLAGS[STAT_FLAG_SHOPKEEPER_CUSTOM_PROPERTIES] >> 8) & 0xF);
				}
			}
			else if ( customShopkeeperInUse == MonsterStatCustomManager::StatEntry::ShopkeeperCustomFlags::DISABLE_GEN_ITEMS )
			{
				my->monsterStoreType = -1;
			}

			bool sellVampireBlood = false;
			for ( c = 0; c < MAXPLAYERS; ++c )
			{
				if ( playerRequiresBloodToSustain(c) )
				{
					sellVampireBlood = true;
					break;
				}
			}

			Item* tmpItem = nullptr;
			bool doneAlembic = false;
			bool doneLockpick = false;
			bool doneBackpack = false;
			bool doneTinkeringKit = false;
			bool doneFeather = false;
			int doneHardwareHat = 0;
			switch ( my->monsterStoreType )
			{
				case -1:
					my->monsterStoreType = oldMonsterStoreType; // don't generate any items.
					break;
				case 0:
					// arms & armor store
					if ( blessedShopkeeper > 0 )
					{
						numitems += rng.rand() % 5; // offset some of the quantity reduction.
					}
					for ( c = 0; c < numitems; c++ )
					{
						if ( shoplevel >= 18 )
						{
							if ( rng.rand() % 2 )
							{
								if ( rng.rand() % 10 == 0 )
								{
									tmpItem = newItem(itemLevelCurveEntity(*my, THROWN, 8, shoplevel, rng), static_cast<Status>(SERVICABLE + rng.rand() % 2), 0, 3 + rng.rand() % 3, rng.rand(), false, &myStats->inventory);
								}
								else
								{
									tmpItem = newItem(itemLevelCurveEntity(*my, ARMOR, 5, shoplevel, rng), static_cast<Status>(WORN + rng.rand() % 3), rng.rand() % blessedShopkeeper, 1 + rng.rand() % 4, rng.rand(), false, &myStats->inventory);
								}
							}
							else
							{
								tmpItem = newItem(itemLevelCurveEntity(*my, WEAPON, 10, shoplevel, rng), static_cast<Status>(WORN + rng.rand() % 3), rng.rand() % blessedShopkeeper, 1 + rng.rand() % 4, rng.rand(), false, &myStats->inventory);
							}
						}
						else
						{
							if ( rng.rand() % 2 )
							{
								if ( rng.rand() % 8 == 0 )
								{
									tmpItem = newItem(itemLevelCurveEntity(*my, THROWN, 0, shoplevel + 20, rng), static_cast<Status>(WORN + rng.rand() % 3), 0, 3 + rng.rand() % 3, rng.rand(), false, &myStats->inventory);
								}
								else
								{
									tmpItem = newItem(static_cast<ItemType>(rng.rand() % 20), static_cast<Status>(WORN + rng.rand() % 3), 0, 1 + rng.rand() % 4, rng.rand(), false, &myStats->inventory);
								}
							}
							else
							{
								int i = rng.rand() % 23;
								if ( i < 18 )
								{
									tmpItem = newItem(static_cast<ItemType>(GLOVES + i), static_cast<Status>(WORN + rng.rand() % 3), 0, 1 + rng.rand() % 4, rng.rand(), false, &myStats->inventory);
								}
								else if ( i < 21 )
								{
									tmpItem = newItem(static_cast<ItemType>(GLOVES + i + 4), static_cast<Status>(WORN + rng.rand() % 3), 0, 1 + rng.rand() % 6, rng.rand(), false, &myStats->inventory);
								}
								else
								{
									// punching armaments
									tmpItem = newItem(static_cast<ItemType>(BRASS_KNUCKLES + rng.rand() % 3), static_cast<Status>(WORN + rng.rand() % 3), 0, 1 + rng.rand() % 2, rng.rand(), false, &myStats->inventory);
								}
							}
						}
						// post-processing
						if ( tmpItem )
						{
							if ( tmpItem->beatitude > 0 )
							{
								tmpItem->count = 1;
								tmpItem->status = static_cast<Status>(SERVICABLE + rng.rand() % 2);
							}
							if ( tmpItem->type >= BRONZE_TOMAHAWK && tmpItem->type <= CRYSTAL_SHURIKEN )
							{
								// thrown weapons always fixed status. (tomahawk = decrepit, shuriken = excellent)
								tmpItem->status = std::min(static_cast<Status>(DECREPIT + (tmpItem->type - BRONZE_TOMAHAWK)), EXCELLENT);
							}
						}
					}
					break;
				case 1:
					// hat store
					for ( c = 0; c < numitems; c++ )
					{
						//if ( blessedShopkeeper > 0 )
						//{
						//	numitems += rng.rand() % 5; // offset some of the quantity reduction.
						//}
						tmpItem = newItem(itemLevelCurveEntity(*my, ARMOR, 0, shoplevel + 5, rng), 
							static_cast<Status>(WORN + rng.rand() % 3), rng.rand() % blessedShopkeeper, 
							1 + ((rng.rand() % 4 == 0 ? 1 : 0)) /* 1 hat, random % to be +qty */, rng.rand(), false, &myStats->inventory);
						// post-processing
						if ( tmpItem && tmpItem->beatitude > 0 )
						{
							tmpItem->count = 1;
							tmpItem->status = static_cast<Status>(SERVICABLE + rng.rand() % 2);
						}
					}
					break;
				case 2:
					// jewelry store
					for ( c = 0; c < numitems; c++ )
					{
						switch ( rng.rand() % 3 )
						{
							case 0:
								tmpItem = newItem(itemLevelCurveEntity(*my, AMULET, 0, shoplevel + 5, rng), static_cast<Status>(WORN + rng.rand() % 3), rng.rand() % blessedShopkeeper, 1 + rng.rand() % 2, rng.rand(), false, &myStats->inventory);
								break;
							case 1:
								tmpItem = newItem(itemLevelCurveEntity(*my, RING, 0, shoplevel + 5, rng), static_cast<Status>(WORN + rng.rand() % 3), rng.rand() % blessedShopkeeper, 1 + rng.rand() % 2, rng.rand(), false, &myStats->inventory);
								break;
							case 2:
								tmpItem = newItem(static_cast<ItemType>(GEM_GARNET + rng.rand() % 16), static_cast<Status>(WORN + rng.rand() % 3), 0, 1 + rng.rand() % 2, rng.rand(), false, &myStats->inventory);
								break;
						}
						// post-processing
						if ( tmpItem && tmpItem->beatitude > 0 )
						{
							tmpItem->count = 1;
							tmpItem->status = static_cast<Status>(SERVICABLE + rng.rand() % 2);
						}
					}
					break;
				case 3:
					// bookstore
					for ( c = 0; c < numitems; c++ )
					{
						switch ( rng.rand() % 3 )
						{
							case 0:
								if ( shoplevel >= 18 )
								{
									tmpItem = newItem(itemLevelCurveEntity(*my, SPELLBOOK, 0, shoplevel, rng), static_cast<Status>(WORN + rng.rand() % 3), rng.rand() % blessedShopkeeper, 1 + rng.rand() % 2, rng.rand(), true, &myStats->inventory);
								}
								else
								{
									tmpItem = newItem(static_cast<ItemType>(SPELLBOOK_FORCEBOLT + rng.rand() % 21), static_cast<Status>(WORN + rng.rand() % 3), 0, 1 + rng.rand() % 2, rng.rand(), true, &myStats->inventory);
								}
								break;
							case 1:
								tmpItem = newItem(itemLevelCurveEntity(*my, SCROLL, 0, 35, rng), static_cast<Status>(WORN + rng.rand() % 3), 0, 1 + rng.rand() % 2, rng.rand(), true, &myStats->inventory);
								break;
							case 2:
								if ( rng.rand() % 3 == 0 )
								{
									tmpItem = newItem(itemLevelCurveEntity(*my, SCROLL, 0, 35, rng), static_cast<Status>(WORN + rng.rand() % 3), 0, 1 + rng.rand() % 2, rng.rand(), true, &myStats->inventory);
								}
								else
								{
									tmpItem = newItem(READABLE_BOOK, static_cast<Status>(WORN + rng.rand() % 3), 0, 1 + rng.rand() % 3, rng.rand(), false, &myStats->inventory);
								}
								break;
						}
						// post-processing
						if ( rng.rand() % blessedShopkeeper > 0 )
						{
							tmpItem->status = static_cast<Status>(SERVICABLE + rng.rand() % 2);
						}
					}
					if ( !doneFeather && rng.rand() % 20 == 0 )
					{
						if ( rng.rand() % 5 == 0 )
						{
							newItem(ENCHANTED_FEATHER, EXCELLENT, 0, 1, ENCHANTED_FEATHER_MAX_DURABILITY - 1, true, &myStats->inventory);
						}
						else
						{
							newItem(ENCHANTED_FEATHER, SERVICABLE, 0, 1, (3 * (ENCHANTED_FEATHER_MAX_DURABILITY - 1)) / 4, true, &myStats->inventory);
						}
						tmpItem = newItem(SCROLL_BLANK, static_cast<Status>(WORN + rng.rand() % 3), 0, 1 + rng.rand() % 3, rng.rand(), true, &myStats->inventory);
						doneFeather = true;
					}
					break;
				case 4:
					// apothecary
					for ( c = 0; c < numitems; c++ )
					{
						if ( !doneAlembic && rng.rand() % 2 == 0 )
						{
							if ( rng.rand() % 2 == 0 )
							{
								tmpItem = newItem(TOOL_ALEMBIC, static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), true, &myStats->inventory);
								if ( rng.rand() % blessedShopkeeper > 0 )
								{
									tmpItem->status = static_cast<Status>(SERVICABLE + rng.rand() % 2);
								}
							}
							if ( rng.rand() % 2 == 0 )
							{
								tmpItem = newItem(TOOL_ALEMBIC, static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), true, &myStats->inventory);
								if ( rng.rand() % blessedShopkeeper > 0 )
								{
									tmpItem->status = static_cast<Status>(SERVICABLE + rng.rand() % 2);
								}
							}
							tmpItem = newItem(TOOL_ALEMBIC, static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), true, &myStats->inventory);
							doneAlembic = true;
						}
						else
						{
							tmpItem = newItem(static_cast<ItemType>(POTION_WATER + rng.rand() % 15), static_cast<Status>(WORN + rng.rand() % 3), 0, 1 + rng.rand() % 5, rng.rand(), true, &myStats->inventory);
						}
						// post-processing
						if ( rng.rand() % blessedShopkeeper > 0 )
						{
							tmpItem->status = static_cast<Status>(SERVICABLE + rng.rand() % 2);
						}
					}
					newItem(POTION_EMPTY, SERVICABLE, 0, 2 + rng.rand() % 5, 0, true, &myStats->inventory);
					if ( sellVampireBlood )
					{
						tmpItem = newItem(FOOD_BLOOD, EXCELLENT, 0, 2 + rng.rand() % 3, rng.rand(), false, &myStats->inventory);
					}
					break;
				case 5:
					// staff shop
					for ( c = 0; c < numitems; c++ )
					{
						if ( shoplevel >= 18 )
						{
							tmpItem = newItem(itemLevelCurveEntity(*my, MAGICSTAFF, 0, shoplevel, rng), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), true, &myStats->inventory);
						}
						else
						{
							tmpItem = newItem(itemLevelCurveEntity(*my, MAGICSTAFF, 0, 15, rng), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), true, &myStats->inventory);
						}
						// post-processing
						if ( rng.rand() % blessedShopkeeper > 0 )
						{
							tmpItem->status = static_cast<Status>(SERVICABLE + rng.rand() % 2);
						}
					}
					break;
				case 6:
					// food store
					for ( c = 0; c < numitems; c++ )
					{
						tmpItem = newItem(static_cast<ItemType>(FOOD_BREAD + rng.rand() % 7), static_cast<Status>(SERVICABLE + rng.rand() % 2), 0, 1 + rng.rand() % 3, rng.rand(), false, &myStats->inventory);
						// post-processing
						if ( rng.rand() % blessedShopkeeper > 0 )
						{
							tmpItem->status = static_cast<Status>(SERVICABLE + rng.rand() % 2);
						}
					}
					break;
				case 7:
					// hardware store
					for ( c = 0; c < numitems; c++ )
					{
						if ( rng.rand() % 20 == 0 )
						{
							tmpItem = newItem(itemLevelCurveEntity(*my, THROWN, 0, shoplevel + 20, rng), static_cast<Status>(SERVICABLE + rng.rand() % 2), 0, 3 + rng.rand() % 3, rng.rand(), false, &myStats->inventory);
						}
						else
						{
							tmpItem = newItem(static_cast<ItemType>(TOOL_PICKAXE + rng.rand() % 11), static_cast<Status>(WORN + rng.rand() % 3), 0, 1 + rng.rand() % 3, rng.rand(), false, &myStats->inventory);
						}
						// post-processing
						if ( rng.rand() % blessedShopkeeper > 0 )
						{
							tmpItem->status = static_cast<Status>(SERVICABLE + rng.rand() % 2);
						}
						if ( tmpItem->type >= BRONZE_TOMAHAWK && tmpItem->type <= CRYSTAL_SHURIKEN )
						{
							// thrown weapons always fixed status. (tomahawk = decrepit, shuriken = excellent)
							tmpItem->status = std::min(static_cast<Status>(DECREPIT + (tmpItem->type - BRONZE_TOMAHAWK)), EXCELLENT);
						}

						if ( !doneLockpick && rng.rand() % 2 == 0 )
						{
							tmpItem = newItem(TOOL_LOCKPICK, static_cast<Status>(WORN + rng.rand() % 3), 0, 1 + rng.rand() % 3, rng.rand(), true, &myStats->inventory);
							if ( rng.rand() % blessedShopkeeper > 0 )
							{
								tmpItem->status = static_cast<Status>(SERVICABLE + rng.rand() % 2);
							}
							doneLockpick = true;
						}

						if ( !doneTinkeringKit && rng.rand() % 5 == 0 )
						{
							newItem(TOOL_TINKERING_KIT, DECREPIT, 0, 1, rng.rand(), true, &myStats->inventory);
							doneTinkeringKit = true;
						}

						if ( !doneAlembic && rng.rand() % 2 == 0 )
						{
							tmpItem = newItem(TOOL_ALEMBIC, static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), true, &myStats->inventory);
							if ( rng.rand() % blessedShopkeeper > 0 )
							{
								tmpItem->status = static_cast<Status>(SERVICABLE + rng.rand() % 2);
							}
							if ( rng.rand() % 2 == 0 )
							{
								tmpItem = newItem(TOOL_ALEMBIC, static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), true, &myStats->inventory);
								if ( rng.rand() % blessedShopkeeper > 0 )
								{
									tmpItem->status = static_cast<Status>(SERVICABLE + rng.rand() % 2);
								}
							}
							if ( rng.rand() % 2 == 0 )
							{
								tmpItem = newItem(TOOL_ALEMBIC, static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), true, &myStats->inventory);
								if ( rng.rand() % blessedShopkeeper > 0 )
								{
									tmpItem->status = static_cast<Status>(SERVICABLE + rng.rand() % 2);
								}
							}
							doneAlembic = true;
						}

					}
					if ( !doneBackpack && rng.rand() % 10 == 0 )
					{
						newItem(CLOAK_BACKPACK, static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), true, &myStats->inventory);
						doneBackpack = true;
					}
					if ( (doneHardwareHat == 0 && rng.rand() % 5 == 0) || (doneHardwareHat == 1 && rng.rand() % 20 == 0) )
					{
						doneHardwareHat++;
						int numHats = 1 + ((rng.rand() % 4 == 0) ? 1 : 0);
						while ( numHats > 0 )
						{
							--numHats;
							int roll = rng.rand() % 15;
							ItemType hat = WOODEN_SHIELD;
							switch ( roll )
							{
								case 0:
								case 1:
								case 2:
								case 3:
								case 4:
								case 5:
								case 6:
								case 7:
								case 8:
								case 9:
									hat = HELM_MINING;
									break;
								case 10:
								case 11:
								case 12:
									hat = MASK_HAZARD_GOGGLES;
									break;
								case 13:
									hat = MASK_PIPE;
									break;
								case 14:
									hat = MASK_MOUTHKNIFE;
									break;
								default:
									break;
							}
							Item* tmpItem = newItem(hat, static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), true, &myStats->inventory);
							if ( rng.rand() % blessedShopkeeper > 0 )
							{
								tmpItem->status = static_cast<Status>(SERVICABLE + rng.rand() % 2);
								tmpItem->beatitude += rng.rand() % blessedShopkeeper;
							}
						}
					}
					break;
				case 8:
					// weapon/hunting store
					if ( shoplevel < 10 && customShopkeeperInUse == 0 )
					{
						numitems = 7 + rng.rand() % 4;
					}
					for ( c = 0; c < numitems; c++ )
					{
						switch ( rng.rand() % 20 )
						{
							case 0:
							case 1:
							case 2:
							case 3:
							{
								// ranged weapons
								std::vector<ItemType> rangedWeapons;
								rangedWeapons.push_back(SHORTBOW);
								if ( shoplevel < 5 )
								{
									rangedWeapons.push_back(SLING);
								}
								if ( shoplevel >= 8 )
								{
									rangedWeapons.push_back(CROSSBOW);
								}
								if ( shoplevel >= 13 )
								{
									rangedWeapons.push_back(LONGBOW);
								}
								if ( shoplevel >= 15 )
								{
									rangedWeapons.push_back(HEAVY_CROSSBOW);
									rangedWeapons.push_back(COMPOUND_BOW);
								}
								ItemType chosenType = rangedWeapons[rng.rand() % rangedWeapons.size()];
								tmpItem = newItem(chosenType, static_cast<Status>(WORN + rng.rand() % 3), rng.rand() % blessedShopkeeper, 1, rng.rand(), false, &myStats->inventory);
								break;
							}
							case 4:
							case 5:
							case 6:
							case 7:
							case 8:
							case 9:
							case 10:
							case 11:
								// standard weapons
								if ( shoplevel >= 18 )
								{
									tmpItem = newItem(itemLevelCurveEntity(*my, WEAPON, 10, shoplevel + 5, rng), static_cast<Status>(WORN + rng.rand() % 3), rng.rand() % blessedShopkeeper, 1, rng.rand(), false, &myStats->inventory);
								}
								else
								{
									tmpItem = newItem(itemLevelCurveEntity(*my, WEAPON, 0, shoplevel + 5, rng), static_cast<Status>(WORN + rng.rand() % 3), rng.rand() % blessedShopkeeper, 1, rng.rand(), false, &myStats->inventory);
								}
								break;
							case 12:
							case 13:
								// thrown weapons (10%), sometime punching things
								if ( rng.rand() % 10 == 0 )
								{
									// punching stuff (5%)
									std::vector<ItemType> gloveWeapons;
									gloveWeapons.push_back(BRASS_KNUCKLES);
									if ( shoplevel >= items[SPIKED_GAUNTLETS].level )
									{
										gloveWeapons.push_back(SPIKED_GAUNTLETS);
									}
									if ( shoplevel >= items[IRON_KNUCKLES].level )
									{
										gloveWeapons.push_back(IRON_KNUCKLES);
									}
									ItemType chosenType = gloveWeapons[rng.rand() % gloveWeapons.size()];
									tmpItem = newItem(chosenType, static_cast<Status>(WORN + rng.rand() % 3), rng.rand() % blessedShopkeeper, 1, rng.rand(), false, &myStats->inventory);
								}
								else
								{
									if ( shoplevel >= 18 )
									{
										tmpItem = newItem(itemLevelCurveEntity(*my, THROWN, 0, shoplevel + 20, rng), static_cast<Status>(WORN + rng.rand() % 3), 0, 3 + rng.rand() % 3, rng.rand(), false, &myStats->inventory);
									}
									else
									{
										tmpItem = newItem(itemLevelCurveEntity(*my, THROWN, 0, 8, rng), static_cast<Status>(SERVICABLE + rng.rand() % 2), 0, 3 + rng.rand() % 3, rng.rand(), false, &myStats->inventory);
									}
								}
								break;
							case 14:
							case 15:
							case 16:
							case 17:
							case 18:
							case 19:
							{
								// quivers (30%)
								std::vector<ItemType> quivers;
								quivers.push_back(QUIVER_SILVER);
								quivers.push_back(QUIVER_LIGHTWEIGHT);
								if ( shoplevel >= 5 )
								{
									quivers.push_back(QUIVER_KNOCKBACK);
								}
								if ( shoplevel >= 10 )
								{
									quivers.push_back(QUIVER_FIRE);
									quivers.push_back(QUIVER_HUNTING);
								}
								if ( shoplevel >= 18 )
								{
									quivers.push_back(QUIVER_PIERCE);
									quivers.push_back(QUIVER_CRYSTAL);
								}
								ItemType chosenType = quivers[rng.rand() % quivers.size()];
								tmpItem = newItem(chosenType, EXCELLENT, 0, 10 + rng.rand() % 6, 0, true, &myStats->inventory); // 10-15 arrows.
								break;
							}
							default:
								break;
						}
						// post-processing
						if ( tmpItem )
						{
							if ( tmpItem->beatitude > 0 )
							{
								tmpItem->count = 1;
								tmpItem->status = static_cast<Status>(SERVICABLE + rng.rand() % 2);
							}
							if ( tmpItem->type >= BRONZE_TOMAHAWK && tmpItem->type <= CRYSTAL_SHURIKEN )
							{
								// thrown weapons always fixed status. (tomahawk = decrepit, shuriken = excellent)
								tmpItem->status = std::min(static_cast<Status>(DECREPIT + (tmpItem->type - BRONZE_TOMAHAWK)), EXCELLENT);
							}
						}
					}
					break;
				case 9:
					// general store
					for ( c = 0; c < numitems; c++ )
					{
						Category cat = static_cast<Category>(rng.rand() % (NUMCATEGORIES - 1));
						tmpItem = newItem(itemLevelCurveEntity(*my, cat, 0, shoplevel + 5, rng), static_cast<Status>(WORN + rng.rand() % 3), 0, 1 + rng.rand() % 3, rng.rand(), false, &myStats->inventory);
						if ( tmpItem && (itemCategory(tmpItem) == WEAPON || itemCategory(tmpItem) == ARMOR || itemCategory(tmpItem) == RING || itemCategory(tmpItem) == AMULET) )
						{
							tmpItem->beatitude += rng.rand() % blessedShopkeeper;
							// post-processing
							if ( tmpItem->beatitude > 0 )
							{
								tmpItem->count = 1;
							}
						}
						if ( tmpItem && tmpItem->type >= BRONZE_TOMAHAWK && tmpItem->type <= CRYSTAL_SHURIKEN )
						{
							// thrown weapons always fixed status. (tomahawk = decrepit, shuriken = excellent)
							tmpItem->status = std::min(static_cast<Status>(DECREPIT + (tmpItem->type - BRONZE_TOMAHAWK)), EXCELLENT);
						}
					}
					if ( !doneTinkeringKit && rng.rand() % 20 == 0 )
					{
						if ( rng.rand() % 5 == 0 )
						{
							newItem(TOOL_TINKERING_KIT, WORN, 0, 1, rng.rand(), true, &myStats->inventory);
						}
						else
						{
							newItem(TOOL_TINKERING_KIT, DECREPIT, 0, 1, rng.rand(), true, &myStats->inventory);
						}
						doneTinkeringKit = true;
					}
					if ( sellVampireBlood )
					{
						tmpItem = newItem(FOOD_BLOOD, EXCELLENT, 0, 1 + rng.rand() % 4, rng.rand(), false, &myStats->inventory);
					}
					break;
				case 10:
				{
					// mysterious merchant.
					// general store
					numitems = 15;
					int itemx = 0;
					int itemy = 0;
					while ( numitems > 0 )
					{
						for ( auto orbCategories : shopkeeperMysteriousItems )
						{
							for ( auto itemInCategory : orbCategories.second )
							{
								if ( itemInCategory == ENCHANTED_FEATHER )
								{
									Item* item = newItem(static_cast<ItemType>(itemInCategory), EXCELLENT, 0, 1, ENCHANTED_FEATHER_MAX_DURABILITY - 1, true, &myStats->inventory);
									item->x = itemx;
									item->y = itemy;
								}
								else if ( itemTypeIsQuiver(static_cast<ItemType>(itemInCategory)) )
								{
									Item* item = newItem(static_cast<ItemType>(itemInCategory), SERVICABLE, 0, 50, ITEM_GENERATED_QUIVER_APPEARANCE, true, &myStats->inventory);
									item->x = itemx;
									item->y = itemy;
								}
								else
								{
									int bless = 0;
									Status status = SERVICABLE;
									if ( itemInCategory >= CRYSTAL_SWORD && itemInCategory <= CRYSTAL_MACE )
									{
										bless = 3;
										status = EXCELLENT;
									}
									else if ( itemInCategory >= ARTIFACT_SWORD && itemInCategory <= ARTIFACT_BOW )
									{
										status = SERVICABLE;
									}
									else if ( itemInCategory == MASK_ARTIFACT_VISOR )
									{
										status = EXCELLENT;
									}
									Item* item = newItem(static_cast<ItemType>(itemInCategory), status, bless, 1, rng.rand(), true, &myStats->inventory);
									item->x = itemx;
									item->y = itemy;
								}
								--numitems;
								++itemx;
								if ( itemx >= Player::ShopGUI_t::MAX_SHOP_X )
								{
									itemx = 0;
									++itemy;
									itemy = std::min(itemy, Player::ShopGUI_t::MAX_SHOP_Y);
								}
							}
							++itemy;
							itemy = std::min(itemy, Player::ShopGUI_t::MAX_SHOP_Y);
							itemx = 0;
						}
						break;
					}
					break;
				}
				default:
					break;
			}
		}

		node_t* nextnode;
		// sort items into slots
		if ( my->monsterStoreType != 10 )
		{
			std::vector<std::pair<int, Item*>> priceAndItems;
			for ( node_t* node = myStats->inventory.first; node != nullptr; node = nextnode )
			{
				nextnode = node->next;
				Item* item = (Item*)node->element;
				if ( !item ) { continue; }

				priceAndItems.push_back(std::make_pair(item->buyValue(clientnum), item));
			}

			std::sort(priceAndItems.begin(), priceAndItems.end(), [](std::pair<int, Item*> lhs, std::pair<int, Item*> rhs) {
				return lhs.first > rhs.first;
			});

			int slotx = 0;
			int sloty = 0;
			std::set<int> takenSlots;
			for ( auto& v : priceAndItems )
			{
				Item* item = v.second;
				item->x = slotx;
				item->y = sloty;
				takenSlots.insert(slotx + sloty * 100);
				++slotx;
				if ( slotx >= Player::ShopGUI_t::MAX_SHOP_X )
				{
					slotx = 0;
					++sloty;
				}
				if ( sloty >= Player::ShopGUI_t::MAX_SHOP_Y )
				{
					break;
				}
			}

			slotx = Player::ShopGUI_t::MAX_SHOP_X - 1;
			sloty = Player::ShopGUI_t::MAX_SHOP_Y - 1;
			auto generatedItems = generateShopkeeperConsumables(*my, *myStats, my->monsterStoreType);
			for ( auto it = generatedItems.rbegin(); it != generatedItems.rend(); ++it )
			{
				if ( takenSlots.find(slotx + sloty * 100) != takenSlots.end() )
				{
					break; // too many items
				}
				(*it)->x = slotx;
				(*it)->y = sloty;
				--slotx;
				if ( slotx < 0 )
				{
					slotx = Player::ShopGUI_t::MAX_SHOP_X - 1;
					--sloty;
				}
				if ( sloty < 0 )
				{
					break;
				}
			}
		}

		for ( node_t* node = myStats->inventory.first; node != nullptr; node = nextnode )
		{
			nextnode = node->next;
			Item* item = (Item*)node->element;
			if ( !item ) { continue; }

			if ( itemCategory(item) == POTION )
			{
				// convert potion appearances into standard types
				for ( size_t p = 0; p < potionStandardAppearanceMap.size(); ++p )
				{
					if ( potionStandardAppearanceMap[p].first == item->type )
					{
						item->appearance = potionStandardAppearanceMap[p].second;
					}
				}
			}
		}
	}

	// torso
	Entity* entity = newEntity(218, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SHOPKEEPER][1][0]; // 0
	entity->focaly = limbs[SHOPKEEPER][1][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][1][2]; // 0
	entity->behavior = &actShopkeeperLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right leg
	entity = newEntity(222, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SHOPKEEPER][2][0]; // 0
	entity->focaly = limbs[SHOPKEEPER][2][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][2][2]; // 2
	entity->behavior = &actShopkeeperLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left leg
	entity = newEntity(221, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SHOPKEEPER][3][0]; // 0
	entity->focaly = limbs[SHOPKEEPER][3][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][3][2]; // 2
	entity->behavior = &actShopkeeperLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right arm
	entity = newEntity(220, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SHOPKEEPER][4][0]; // 0
	entity->focaly = limbs[SHOPKEEPER][4][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][4][2]; // 1.5
	entity->behavior = &actShopkeeperLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left arm
	entity = newEntity(219, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SHOPKEEPER][5][0]; // 0
	entity->focaly = limbs[SHOPKEEPER][5][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][5][2]; // 1.5
	entity->behavior = &actShopkeeperLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// world weapon
	entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->noColorChangeAllyLimb = 1.0;
	entity->focalx = limbs[SHOPKEEPER][6][0]; // 1.5
	entity->focaly = limbs[SHOPKEEPER][6][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][6][2]; // -.5
	entity->behavior = &actShopkeeperLimb;
	entity->parent = my->getUID();
	entity->pitch = .25;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// shield
	entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->noColorChangeAllyLimb = 1.0;
	entity->focalx = limbs[SHOPKEEPER][7][0]; // 2
	entity->focaly = limbs[SHOPKEEPER][7][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][7][2]; // 0
	entity->behavior = &actShopkeeperLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// cloak
	entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->scalex = 1.01;
	entity->scaley = 1.01;
	entity->scalez = 1.01;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->noColorChangeAllyLimb = 1.0;
	entity->focalx = limbs[SHOPKEEPER][8][0]; // 0
	entity->focaly = limbs[SHOPKEEPER][8][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][8][2]; // 4
	entity->behavior = &actShopkeeperLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// helmet
	entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->scalex = 1.01;
	entity->scaley = 1.01;
	entity->scalez = 1.01;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->noColorChangeAllyLimb = 1.0;
	entity->focalx = limbs[SHOPKEEPER][9][0]; // 0
	entity->focaly = limbs[SHOPKEEPER][9][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][9][2]; // -1.75
	entity->behavior = &actShopkeeperLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// mask
	entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->scalex = .99;
	entity->scaley = .99;
	entity->scalez = .99;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->noColorChangeAllyLimb = 1.0;
	entity->focalx = limbs[SHOPKEEPER][10][0]; // 0
	entity->focaly = limbs[SHOPKEEPER][10][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][10][2]; // .5
	entity->behavior = &actShopkeeperLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);
}

void actShopkeeperLimb(Entity* my)
{
	my->actMonsterLimb();
}

void shopkeeperDie(Entity* my)
{
	int c;
	for ( c = 0; c < 10; c++ )
	{
		Entity* gib = spawnGib(my);
		if (c < 6) {
		    gib->sprite = 217 + c;
		    gib->skill[5] = 1; // poof
		}
		serverSpawnGibForClient(gib);
	}

	my->spawnBlood();

	my->removeMonsterDeathNodes();

	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( players[i]->isLocalPlayer() && shopkeeper[i] == my->getUID() )
		{
			players[i]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_CLOSE_ALL);
		}
		else if ( i > 0 && !client_disconnected[i] && multiplayer == SERVER && !players[i]->isLocalPlayer() )
		{
			// inform client of abandonment
			strcpy((char*)net_packet->data, "SHPC");
			SDLNet_Write32(my->getUID(), &net_packet->data[4]);
			net_packet->address.host = net_clients[i - 1].host;
			net_packet->address.port = net_clients[i - 1].port;
			net_packet->len = 8;
			sendPacketSafe(net_sock, -1, net_packet, i - 1);
		}
	}

	list_RemoveNode(my->mynode);
	return;
}

#define SHOPKEEPERWALKSPEED .15

void shopkeeperMoveBodyparts(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = nullptr, *entity2 = nullptr;
	Entity* rightbody = nullptr;
	Entity* weaponarm = nullptr;
	int bodypart;
	bool wearingring = false;

	if ( multiplayer != CLIENT && myStats->MISC_FLAGS[STAT_FLAG_MYSTERIOUS_SHOPKEEP] > 0 )
	{
		if ( my->ticks == 5 * TICKS_PER_SECOND )
		{
			serverUpdateEntitySkill(my, 18); // update the store type for clients.
		}
		if ( myStats->HP < ((3 * myStats->MAXHP) / 4) )
		{
			playSoundEntity(my, 77, 64);
			createParticleErupt(my, 593);
			serverSpawnMiscParticles(my, PARTICLE_EFFECT_ERUPT, 593);
			my->removeMonsterDeathNodes();
			list_RemoveNode(my->mynode);
			return;
		}
	}

	// set invisibility //TODO: isInvisible()?
	if ( multiplayer != CLIENT )
	{
		if ( myStats->ring != nullptr )
			if ( myStats->ring->type == RING_INVISIBILITY )
			{
				wearingring = true;
			}
		if ( myStats->cloak != nullptr )
			if ( myStats->cloak->type == CLOAK_INVISIBILITY )
			{
				wearingring = true;
			}
		if ( myStats->EFFECTS[EFF_INVISIBLE] == true || wearingring == true )
		{
			my->flags[INVISIBLE] = true;
			my->flags[BLOCKSIGHT] = false;
			bodypart = 0;
			for ( node = my->children.first; node != nullptr; node = node->next )
			{
				if ( bodypart < 2 )
				{
					bodypart++;
					continue;
				}
				if ( bodypart >= 7 )
				{
					break;
				}
				entity = (Entity*)node->element;
				if ( !entity->flags[INVISIBLE] )
				{
					entity->flags[INVISIBLE] = true;
					serverUpdateEntityBodypart(my, bodypart);
				}
				bodypart++;
			}
		}
		else
		{
			my->flags[INVISIBLE] = false;
			my->flags[BLOCKSIGHT] = true;
			bodypart = 0;
			for ( node = my->children.first; node != nullptr; node = node->next )
			{
				if ( bodypart < 2 )
				{
					bodypart++;
					continue;
				}
				if ( bodypart >= 7 )
				{
					break;
				}
				entity = (Entity*)node->element;
				if ( entity->flags[INVISIBLE] )
				{
					entity->flags[INVISIBLE] = false;
					serverUpdateEntityBodypart(my, bodypart);
					serverUpdateEntityFlag(my, INVISIBLE);
				}
				bodypart++;
			}
		}

		// sleeping
		if ( myStats->EFFECTS[EFF_ASLEEP] )
		{
			my->z = 1.5;
			my->pitch = PI / 4;
		}
		else
		{
			my->z = -1;
			my->pitch = 0;
		}
	}

	Entity* helmet = nullptr;

	//Move bodyparts
	for (bodypart = 0, node = my->children.first; node != NULL; node = node->next, bodypart++)
	{
		if ( bodypart < LIMB_HUMANOID_TORSO )
		{
			continue;
		}
		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		if ( MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 && bodypart == LIMB_HUMANOID_RIGHTARM )
		{
			// don't let the creatures's yaw move the casting arm
		}
		else
		{
			entity->yaw = my->yaw;
		}
		if ( bodypart == LIMB_HUMANOID_RIGHTLEG || bodypart == LIMB_HUMANOID_LEFTARM )
		{
			my->humanoidAnimateWalk(entity, node, bodypart, SHOPKEEPERWALKSPEED, dist, 0.4);
		}
		else if ( bodypart == LIMB_HUMANOID_LEFTLEG || bodypart == LIMB_HUMANOID_RIGHTARM || bodypart == LIMB_HUMANOID_CLOAK )
		{
			// left leg, right arm, cloak.
			if ( bodypart == LIMB_HUMANOID_RIGHTARM )
			{
				weaponarm = entity;
				if ( my->monsterAttack > 0 )
				{
					my->handleWeaponArmAttack(weaponarm);
				}
			}
			my->humanoidAnimateWalk(entity, node, bodypart, SHOPKEEPERWALKSPEED, dist, 0.4);

			if ( bodypart == LIMB_HUMANOID_CLOAK )
			{
				entity->fskill[0] = entity->pitch;
				entity->roll = my->roll - fabs(entity->pitch) / 2;
				entity->pitch = 0;
			}
		}
		switch ( bodypart )
		{
			// torso
			case 2:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->breastplate == nullptr )
					{
						entity->sprite = 218;
					}
					else
					{
						entity->sprite = itemModel(myStats->breastplate);
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
				}
				entity->x -= .25 * cos(my->yaw);
				entity->y -= .25 * sin(my->yaw);
				entity->z += 2.5;
				break;
			// right leg
			case LIMB_HUMANOID_RIGHTLEG:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shoes == nullptr )
					{
						entity->sprite = 222;
					}
					else
					{
						my->setBootSprite(entity, SPRITE_BOOT_RIGHT_OFFSET);
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
				}
				entity->x += 1 * cos(my->yaw + PI / 2) + .25 * cos(my->yaw);
				entity->y += 1 * sin(my->yaw + PI / 2) + .25 * sin(my->yaw);
				entity->z += 5;
				if ( my->z >= 1.4 && my->z <= 1.6 )
				{
					entity->yaw += PI / 8;
					entity->pitch = -PI / 2;
				}
				break;
			// left leg
			case LIMB_HUMANOID_LEFTLEG:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shoes == nullptr )
					{
						entity->sprite = 221;
					}
					else
					{
						my->setBootSprite(entity, SPRITE_BOOT_LEFT_OFFSET);
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
				}
				entity->x -= 1 * cos(my->yaw + PI / 2) - .25 * cos(my->yaw);
				entity->y -= 1 * sin(my->yaw + PI / 2) - .25 * sin(my->yaw);
				entity->z += 5;
				if ( my->z >= 1.4 && my->z <= 1.6 )
				{
					entity->yaw -= PI / 8;
					entity->pitch = -PI / 2;
				}
				break;
			// right arm
			case LIMB_HUMANOID_RIGHTARM:
			{
				node_t* weaponNode = list_Node(&my->children, LIMB_HUMANOID_WEAPON);
				if ( weaponNode )
				{
					Entity* weapon = (Entity*)weaponNode->element;
					if ( MONSTER_ARMBENDED || (weapon->flags[INVISIBLE] && my->monsterState != MONSTER_STATE_ATTACK) )
					{
						// if weapon invisible and I'm not attacking, relax arm.
						entity->focalx = limbs[SHOPKEEPER][4][0]; // 0
						entity->focaly = limbs[SHOPKEEPER][4][1]; // 0
						entity->focalz = limbs[SHOPKEEPER][4][2]; // 1.5
						entity->sprite = 220;
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[SHOPKEEPER][4][0] + 0.75;
						entity->focaly = limbs[SHOPKEEPER][4][1];
						entity->focalz = limbs[SHOPKEEPER][4][2] - 0.75;
						entity->sprite = 111;
					}
				}
				entity->x += 2.25 * cos(my->yaw + PI / 2) - .20 * cos(my->yaw);
				entity->y += 2.25 * sin(my->yaw + PI / 2) - .20 * sin(my->yaw);
				entity->z += 1.5;
				entity->yaw += MONSTER_WEAPONYAW;
				if ( my->z >= 1.4 && my->z <= 1.6 )
				{
					entity->pitch = 0;
				}
				break;
			}
			// left arm
			case LIMB_HUMANOID_LEFTARM:
			{
				node_t* shieldNode = list_Node(&my->children, 8);
				if ( shieldNode )
				{
					Entity* shield = (Entity*)shieldNode->element;
					if ( shield->flags[INVISIBLE] && (my->monsterState != MONSTER_STATE_ATTACK) )
					{
						// if shield invisible and I'm not attacking, relax arm.
						entity->focalx = limbs[SHOPKEEPER][5][0]; // 0
						entity->focaly = limbs[SHOPKEEPER][5][1]; // 0
						entity->focalz = limbs[SHOPKEEPER][5][2]; // 1.5
						entity->sprite = 219;
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[SHOPKEEPER][5][0] + 0.75;
						entity->focaly = limbs[SHOPKEEPER][5][1];
						entity->focalz = limbs[SHOPKEEPER][5][2] - 0.75;
						entity->sprite = 112;
					}
				}
				entity->x -= 2.25 * cos(my->yaw + PI / 2) + .20 * cos(my->yaw);
				entity->y -= 2.25 * sin(my->yaw + PI / 2) + .20 * sin(my->yaw);
				entity->z += 1.5;
				if ( my->z >= 1.4 && my->z <= 1.6 )
				{
					entity->pitch = 0;
				}
				break;
			}
				// weapon
			case LIMB_HUMANOID_WEAPON:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->weapon == nullptr || myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) //TODO: isInvisible()?
					{
						entity->flags[INVISIBLE] = true;
					}
					else
					{
						entity->sprite = itemModel(myStats->weapon);
						if ( itemCategory(myStats->weapon) == SPELLBOOK )
						{
							entity->flags[INVISIBLE] = true;
						}
						else
						{
							entity->flags[INVISIBLE] = false;
						}
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->skill[11] != entity->flags[INVISIBLE] )
							{
								entity->skill[11] = entity->flags[INVISIBLE];
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
				}
				else
				{
					if ( entity->sprite <= 0 )
					{
						entity->flags[INVISIBLE] = true;
					}
				}
				if ( weaponarm != nullptr )
				{
					my->handleHumanoidWeaponLimb(entity, weaponarm);
				}
				break;
				// shield
			case LIMB_HUMANOID_SHIELD:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shield == nullptr )
					{
						entity->flags[INVISIBLE] = true;
						entity->sprite = 0;
					}
					else
					{
						entity->flags[INVISIBLE] = false;
						entity->sprite = itemModel(myStats->shield);
						if ( itemTypeIsQuiver(myStats->shield->type) )
						{
							entity->handleQuiverThirdPersonModel(*myStats);
						}
					}
					if ( myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) //TODO: isInvisible()?
					{
						entity->flags[INVISIBLE] = true;
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->skill[11] != entity->flags[INVISIBLE] )
							{
								entity->skill[11] = entity->flags[INVISIBLE];
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
				}
				else
				{
					if ( entity->sprite <= 0 )
					{
						entity->flags[INVISIBLE] = true;
					}
				}
				entity->x -= 2.5 * cos(my->yaw + PI / 2) + .20 * cos(my->yaw);
				entity->y -= 2.5 * sin(my->yaw + PI / 2) + .20 * sin(my->yaw);
				entity->z += 2.5;
				if ( entity->sprite == items[TOOL_LANTERN].index )
				{
				    entity->z += 2;
				}
	            if ( flickerLights || my->ticks % TICKS_PER_SECOND == 1 )
	            {
				    if ( entity->sprite == items[TOOL_TORCH].index )
				    {
						if ( entity2 = spawnFlame(entity, SPRITE_FLAME) )
						{
							entity2->x += 2 * cos(my->yaw);
							entity2->y += 2 * sin(my->yaw);
							entity2->z -= 2;
						}
				    }
				    else if ( entity->sprite == items[TOOL_CRYSTALSHARD].index )
				    {
					    /*entity2 = spawnFlame(entity, SPRITE_CRYSTALFLAME);
					    entity2->x += 2 * cos(my->yaw);
					    entity2->y += 2 * sin(my->yaw);
					    entity2->z -= 2;*/
				    }
				    else if ( entity->sprite == items[TOOL_LANTERN].index )
				    {
						if ( entity2 = spawnFlame(entity, SPRITE_FLAME) )
						{
							entity2->x += 2 * cos(my->yaw);
							entity2->y += 2 * sin(my->yaw);
							entity2->z += 1;
						}
				    }
				}
				break;
				// cloak
			case LIMB_HUMANOID_CLOAK:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->cloak == nullptr || myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) //TODO: isInvisible()?
					{
						entity->flags[INVISIBLE] = true;
					}
					else
					{
						entity->flags[INVISIBLE] = false;
						entity->sprite = itemModel(myStats->cloak);
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->skill[11] != entity->flags[INVISIBLE] )
							{
								entity->skill[11] = entity->flags[INVISIBLE];
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
				}
				else
				{
					if ( entity->sprite <= 0 )
					{
						entity->flags[INVISIBLE] = true;
					}
				}
				entity->x -= cos(my->yaw);
				entity->y -= sin(my->yaw);
				entity->yaw += PI / 2;
				break;
			// helm
			case LIMB_HUMANOID_HELMET:
				helmet = entity;
				entity->focalx = limbs[SHOPKEEPER][9][0]; // 0
				entity->focaly = limbs[SHOPKEEPER][9][1]; // 0
				entity->focalz = limbs[SHOPKEEPER][9][2]; // -1.75
				entity->pitch = my->pitch;
				entity->roll = 0;
				if ( multiplayer != CLIENT )
				{
					entity->sprite = itemModel(myStats->helmet);
					if ( myStats->helmet == nullptr || myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) //TODO: isInvisible()?
					{
						entity->flags[INVISIBLE] = true;
					}
					else
					{
						entity->flags[INVISIBLE] = false;
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->skill[11] != entity->flags[INVISIBLE] )
							{
								entity->skill[11] = entity->flags[INVISIBLE];
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
				}
				else
				{
					if ( entity->sprite <= 0 )
					{
						entity->flags[INVISIBLE] = true;
					}
				}
				my->setHelmetLimbOffset(entity);
				break;
			// mask
			case LIMB_HUMANOID_MASK:
				entity->focalx = limbs[SHOPKEEPER][10][0]; // 0
				entity->focaly = limbs[SHOPKEEPER][10][1]; // 0
				entity->focalz = limbs[SHOPKEEPER][10][2]; // .5
				entity->pitch = my->pitch;
				entity->roll = PI / 2;
				if ( multiplayer != CLIENT )
				{
					bool hasSteelHelm = false;
					/*if ( myStats->helmet )
					{
						if ( myStats->helmet->type == STEEL_HELM
							|| myStats->helmet->type == CRYSTAL_HELM
							|| myStats->helmet->type == ARTIFACT_HELM )
						{
							hasSteelHelm = true;
						}
					}*/
					if ( myStats->mask == nullptr || myStats->EFFECTS[EFF_INVISIBLE] || wearingring || hasSteelHelm ) //TODO: isInvisible()?
					{
						entity->flags[INVISIBLE] = true;
					}
					else
					{
						entity->flags[INVISIBLE] = false;
					}
					if ( myStats->mask != nullptr )
					{
						if ( myStats->mask->type == TOOL_GLASSES )
						{
							entity->sprite = 165; // GlassesWorn.vox
						}
						else if ( myStats->mask->type == MONOCLE )
						{
							entity->sprite = 1196; // monocleWorn.vox
						}
						else
						{
							entity->sprite = itemModel(myStats->mask);
						}
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
						{
							bool updateBodypart = false;
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								updateBodypart = true;
							}
							if ( entity->skill[11] != entity->flags[INVISIBLE] )
							{
								entity->skill[11] = entity->flags[INVISIBLE];
								updateBodypart = true;
							}
							if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
							{
								updateBodypart = true;
							}
							if ( updateBodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
				}
				else
				{
					if ( entity->sprite <= 0 )
					{
						entity->flags[INVISIBLE] = true;
					}
				}

				if ( entity->sprite != 165 && entity->sprite != 1196 )
				{
					if ( entity->sprite == items[MASK_SHAMAN].index )
					{
						entity->roll = 0;
						my->setHelmetLimbOffset(entity);
						my->setHelmetLimbOffsetWithMask(helmet, entity);
					}
					else if ( EquipmentModelOffsets.modelOffsetExists(SHOPKEEPER, entity->sprite) )
					{
						my->setHelmetLimbOffset(entity);
						my->setHelmetLimbOffsetWithMask(helmet, entity);
					}
					else
					{
						entity->focalx = limbs[SHOPKEEPER][10][0] + .35; // .35
						entity->focaly = limbs[SHOPKEEPER][10][1] - 2; // -2
						entity->focalz = limbs[SHOPKEEPER][10][2]; // .5
					}
				}
				else
				{
					entity->focalx = limbs[SHOPKEEPER][10][0] + .25; // .25
					entity->focaly = limbs[SHOPKEEPER][10][1] - 2.25; // -2.25
					entity->focalz = limbs[SHOPKEEPER][10][2]; // .5
				}
				break;
		}
	}
	// rotate shield a bit
	node_t* shieldNode = list_Node(&my->children, LIMB_HUMANOID_SHIELD);
	if ( shieldNode )
	{
		Entity* shieldEntity = (Entity*)shieldNode->element;
		if ( shieldEntity->sprite != items[TOOL_TORCH].index && shieldEntity->sprite != items[TOOL_LANTERN].index && shieldEntity->sprite != items[TOOL_CRYSTALSHARD].index )
		{
			shieldEntity->yaw -= PI / 6;
		}
	}
	if ( MONSTER_ATTACK > 0 && MONSTER_ATTACK <= MONSTER_POSE_MAGIC_CAST3 )
	{
		MONSTER_ATTACKTIME++;
	}
	else if ( MONSTER_ATTACK == 0 )
	{
		MONSTER_ATTACKTIME = 0;
	}
	else
	{
		// do nothing, don't reset attacktime or increment it.
	}
}

void shopkeeperMysteriousGenerateInventory(ItemType orb)
{
	/*std::vector<int> items;
	if ( orb == ARTIFACT_ORB_GREEN )
	{
		items.push_back(QUIVER_CRYSTAL);
		items.push_back(QUIVER_LIGHTWEIGHT);
		items.push_back(QUIVER_HUNTING);
		items.push_back(HEAVY_CROSSBOW);
		items.push_back(BOOMERANG);
		items.push_back(ARTIFACT_BOW);
	}
	else if ( orb == ARTIFACT_ORB_BLUE )
	{
		items.push_back(ARTIFACT_MACE);
		items.push_back(ENCHANTED_FEATHER);
	}
	else if ( orb == ARTIFACT_ORB_RED )
	{
		items.push_back(ARTIFACT_AXE);
		items.push_back(ARTIFACT_SWORD);
		items.push_back(ARTIFACT_SPEAR);
	}

	if ( !items.empty() )
	{
		if ( shopkeeperMysteriousItems.find(orb) == shopkeeperMysteriousItems.end() )
		{
			shopkeeperMysteriousItems.insert(std::make_pair(orb, items));
		}
		else
		{
			shopkeeperMysteriousItems[orb] = items;
		}
	}*/
}
