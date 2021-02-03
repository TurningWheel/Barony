/*-------------------------------------------------------------------------------

BARONY
File: mod_tools.hpp
Desc: misc modding tools

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once
#include "main.hpp"
#include "stat.hpp"
#include "json.hpp"
#include "files.hpp"
#include "prng.hpp"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"
#include "net.hpp"
#include "scores.hpp"

class CustomHelpers
{
public:
	static void addMemberToSubkey(rapidjson::Document& d, std::string subkey, std::string name, const rapidjson::Value& value)
	{
		rapidjson::Value key(name.c_str(), d.GetAllocator()); // copy string name
		rapidjson::Value val(value, d.GetAllocator());
		d[subkey.c_str()].AddMember(key, val, d.GetAllocator());
	}
	static void addMemberToRoot(rapidjson::Document& d, std::string name, const rapidjson::Value& value)
	{
		rapidjson::Value key(name.c_str(), d.GetAllocator()); // copy string name
		rapidjson::Value val(value, d.GetAllocator());
		d.AddMember(key, val, d.GetAllocator());
	}
	static void addArrayMemberToSubkey(rapidjson::Document& d, std::string subkey, const rapidjson::Value& value)
	{
		rapidjson::Value val(value, d.GetAllocator());        // some value
		d[subkey.c_str()].PushBack(val, d.GetAllocator());
	}
	static bool isLevelPartOfSet(int level, bool secret, std::pair<std::unordered_set<int>, std::unordered_set<int>>& pairOfSets)
	{
		if ( !secret )
		{
			if ( pairOfSets.first.find(level) == pairOfSets.first.end() )
			{
				return false;
			}
		}
		else
		{
			if ( pairOfSets.second.find(level) == pairOfSets.second.end() )
			{
				return false;
			}
		}
		return true;
	}
};

class MonsterStatCustomManager
{
public:
	std::mt19937 monsterStatSeed;
	static const std::vector<std::string> itemStatusStrings;
	static const std::vector<std::string> shopkeeperTypeStrings;
	MonsterStatCustomManager() :
		monsterStatSeed(rand())
	{
	};

	int getSlotFromKeyName(std::string keyName)
	{
		if ( keyName.compare("weapon") == 0 )
		{
			return ITEM_SLOT_WEAPON;
		}
		else if ( keyName.compare("shield") == 0 )
		{
			return ITEM_SLOT_SHIELD;
		}
		else if ( keyName.compare("helmet") == 0 )
		{
			return ITEM_SLOT_HELM;
		}
		else if ( keyName.compare("breastplate") == 0 )
		{
			return ITEM_SLOT_ARMOR;
		}
		else if ( keyName.compare("gloves") == 0 )
		{
			return ITEM_SLOT_GLOVES;
		}
		else if ( keyName.compare("shoes") == 0 )
		{
			return ITEM_SLOT_BOOTS;
		}
		else if ( keyName.compare("cloak") == 0 )
		{
			return ITEM_SLOT_CLOAK;
		}
		else if ( keyName.compare("ring") == 0 )
		{
			return ITEM_SLOT_RING;
		}
		else if ( keyName.compare("amulet") == 0 )
		{
			return ITEM_SLOT_AMULET;
		}
		else if ( keyName.compare("mask") == 0 )
		{
			return ITEM_SLOT_MASK;
		}
		return 0;
	}

	class ItemEntry
	{
	public:
		ItemType type = WOODEN_SHIELD;
		Status status = DECREPIT;
		Sint16 beatitude = 0;
		Sint16 count = 1;
		Uint32 appearance = 0;
		bool identified = 0;
		int percentChance = 100;
		int weightedChance = 1;
		int dropChance = 100;
		bool emptyItemEntry = false;
		bool dropItemOnDeath = true;
		ItemEntry() {};
		ItemEntry(const Item& itemToRead)
		{
			readFromItem(itemToRead);
		}
		void readFromItem(const Item& itemToRead)
		{
			type = itemToRead.type;
			status = itemToRead.status;
			beatitude = itemToRead.beatitude;
			count = itemToRead.count;
			appearance = itemToRead.appearance;
			identified = itemToRead.identified;
			if ( itemToRead.appearance == MONSTER_ITEM_UNDROPPABLE_APPEARANCE )
			{
				dropItemOnDeath = false;
			}
		}
		void setValueFromAttributes(rapidjson::Document& d, rapidjson::Value& outObject)
		{
			rapidjson::Value key1("type", d.GetAllocator());
			rapidjson::Value val1(itemNameStrings[type + 2], d.GetAllocator());
			outObject.AddMember(key1, val1, d.GetAllocator());

			rapidjson::Value key2("status", d.GetAllocator());
			rapidjson::Value val2(itemStatusStrings.at(status).c_str(), d.GetAllocator());
			outObject.AddMember(key2, val2, d.GetAllocator());

			outObject.AddMember("beatitude", rapidjson::Value(beatitude), d.GetAllocator());
			outObject.AddMember("count", rapidjson::Value(count), d.GetAllocator());
			outObject.AddMember("appearance", rapidjson::Value(appearance), d.GetAllocator());
			outObject.AddMember("identified", rapidjson::Value(identified), d.GetAllocator());
			outObject.AddMember("spawn_percent_chance", rapidjson::Value(100), d.GetAllocator());
			outObject.AddMember("drop_percent_chance", rapidjson::Value(dropItemOnDeath ? 100 : 0), d.GetAllocator());
			outObject.AddMember("slot_weighted_chance", rapidjson::Value(1), d.GetAllocator());
		}

		const char* getRandomArrayStr(const rapidjson::GenericArray<true, rapidjson::GenericValue<rapidjson::UTF8<>>>& arr, const char* invalidEntry)
		{
			if ( arr.Size() == 0 )
			{
				return invalidEntry;
			}
			return (arr[rapidjson::SizeType(rand() % arr.Size())].GetString());
		}
		int getRandomArrayInt(const rapidjson::GenericArray<true, rapidjson::GenericValue<rapidjson::UTF8<>>>& arr, int invalidEntry)
		{
			if ( arr.Size() == 0 )
			{
				return invalidEntry;
			}
			return (arr[rapidjson::SizeType(rand() % arr.Size())].GetInt());
		}

		bool readKeyToItemEntry(rapidjson::Value::ConstMemberIterator& itr)
		{
			std::string name = itr->name.GetString();
			if ( name.compare("type") == 0 )
			{
				std::string itemName = "empty";
				if ( itr->value.IsArray() )
				{
					itemName = getRandomArrayStr(itr->value.GetArray(), "empty");
				}
				else if ( itr->value.IsString() )
				{
					itemName = itr->value.GetString();
				}

				if ( itemName.compare("empty") == 0 )
				{
					emptyItemEntry = true;
					return true;
				}
				for ( int i = 0; i < NUMITEMS; ++i )
				{
					if ( itemName.compare(itemNameStrings[i + 2]) == 0 )
					{
						this->type = static_cast<ItemType>(i);
						return true;
					}
				}
			}
			else if ( name.compare("status") == 0 )
			{
				std::string status = "broken";
				if ( itr->value.IsArray() )
				{
					status = getRandomArrayStr(itr->value.GetArray(), "broken");
				}
				else if ( itr->value.IsString() )
				{
					status = itr->value.GetString();
				}
				for ( Uint32 i = 0; i < itemStatusStrings.size(); ++i )
				{
					if ( status.compare(itemStatusStrings.at(i)) == 0 )
					{
						this->status = static_cast<Status>(i);
						return true;
					}
				}
			}
			else if ( name.compare("beatitude") == 0 )
			{
				if ( itr->value.IsArray() )
				{
					this->beatitude = static_cast<Sint16>(getRandomArrayInt(itr->value.GetArray(), 0));
				}
				else if ( itr->value.IsInt() )
				{
					this->beatitude = static_cast<Sint16>(itr->value.GetInt());
				}
				return true;
			}
			else if ( name.compare("count") == 0 )
			{
				if ( itr->value.IsArray() )
				{
					this->count = static_cast<Sint16>(getRandomArrayInt(itr->value.GetArray(), 1));
				}
				else if ( itr->value.IsInt() )
				{
					this->count = static_cast<Sint16>(itr->value.GetInt());
				}
				return true;
			}
			else if ( name.compare("appearance") == 0 )
			{
				if ( itr->value.IsArray() )
				{
					this->appearance = static_cast<Uint32>(getRandomArrayInt(itr->value.GetArray(), rand()));
				}
				else if ( itr->value.IsInt() )
				{
					this->appearance = static_cast<Uint32>(itr->value.GetInt());
				}
				else if ( itr->value.IsString() )
				{
					std::string str = itr->value.GetString();
					if ( str.compare("random") == 0 )
					{
						this->appearance = rand();
					}
				}
				return true;
			}
			else if ( name.compare("identified") == 0 )
			{
				this->identified = itr->value.GetBool();
				return true;
			}
			else if ( name.compare("spawn_percent_chance") == 0 )
			{
				this->percentChance = itr->value.GetInt();
				return true;
			}
			else if ( name.compare("drop_percent_chance") == 0 )
			{
				this->dropChance = itr->value.GetInt();
				if ( rand() % 100 >= this->dropChance )
				{
					this->dropItemOnDeath = false;
				}
				else
				{
					this->dropItemOnDeath = true;
				}
			}
			else if ( name.compare("slot_weighted_chance") == 0 )
			{
				this->weightedChance = std::max(1, itr->value.GetInt());
				return true;
			}
			return false;
		}
	};

	class StatEntry
	{
		std::mt19937 StatEntrySeed;
	public:
		char name[128] = "";
		int type = NOTHING;
		sex_t sex = sex_t::MALE;
		Uint32 appearance = 0;
		Sint32 HP = 10;
		Sint32 MAXHP = 10;
		Sint32 OLDHP = 10;
		Sint32 MP = 10;
		Sint32 MAXMP = 10;
		Sint32 STR = 0;
		Sint32 DEX = 0;
		Sint32 CON = 0;
		Sint32 INT = 0;
		Sint32 PER = 0;
		Sint32 CHR = 0;
		Sint32 EXP = 0;
		Sint32 LVL = 0;
		Sint32 GOLD = 0;
		Sint32 HUNGER = 0;
		Sint32 RANDOM_STR = 0;
		Sint32 RANDOM_DEX = 0;
		Sint32 RANDOM_CON = 0;
		Sint32 RANDOM_INT = 0;
		Sint32 RANDOM_PER = 0;
		Sint32 RANDOM_CHR = 0;
		Sint32 RANDOM_MAXHP = 0;
		Sint32 RANDOM_HP = 0;
		Sint32 RANDOM_MAXMP = 0;
		Sint32 RANDOM_MP = 0;
		Sint32 RANDOM_LVL = 0;
		Sint32 RANDOM_GOLD = 0;

		Sint32 PROFICIENCIES[NUMPROFICIENCIES];

		std::vector<std::pair<ItemEntry, int>> equipped_items;
		std::vector<ItemEntry> inventory_items;
		std::vector<std::pair<std::string, int>> followerVariants;
		std::vector<std::pair<std::string, int>> shopkeeperStoreTypes;
		int chosenShopkeeperStore = -1;
		int shopkeeperMinItems = -1;
		int shopkeeperMaxItems = -1;
		int shopkeeperMaxGeneratedBlessing = -1;
		bool shopkeeperGenDefaultItems = true;
		enum ShopkeeperCustomFlags : int
		{
			ENABLE_GEN_ITEMS = 1,
			DISABLE_GEN_ITEMS
		};

		int numFollowers = 0;
		bool isMonsterNameGeneric = false;
		bool useDefaultEquipment = true;
		bool useDefaultInventoryItems = true;
		bool disableMiniboss = true;
		bool forceFriendlyToPlayer = false;
		bool forceEnemyToPlayer = false;
		bool forceRecruitableToPlayer = false;
		bool disableItemDrops = false;
		int xpAwardPercent = 100;
		bool castSpellbooksFromInventory = false;
		int spellbookCastCooldown = 250;

		StatEntry(const Stat* myStats) :
			StatEntrySeed(rand())
		{
			readFromStats(myStats);
		}
		StatEntry() :
			StatEntrySeed(rand())
		{
			for ( int i = 0; i < NUMPROFICIENCIES; ++i )
			{
				PROFICIENCIES[i] = 0;
			}
		};

		std::string getFollowerVariant()
		{
			if ( followerVariants.size() > 0 )
			{
				std::vector<int> variantChances(followerVariants.size(), 0);
				int index = 0;
				for ( auto& pair : followerVariants )
				{
					variantChances.at(index) = pair.second;
					++index;
				}

				std::discrete_distribution<> variantWeightedDistribution(variantChances.begin(), variantChances.end());
				int result = variantWeightedDistribution(StatEntrySeed);
				return followerVariants.at(result).first;
			}
			return "none";
		}

		void readFromStats(const Stat* myStats)
		{
			strcpy(name, myStats->name);
			type = myStats->type;
			sex = myStats->sex;
			appearance = myStats->appearance;
			HP = myStats->HP;
			MAXHP = myStats->MAXHP;
			OLDHP = HP;
			MP = myStats->MP;
			MAXMP = myStats->MAXMP;
			STR = myStats->STR;
			DEX = myStats->DEX;
			CON = myStats->CON;
			INT = myStats->INT;
			PER = myStats->PER;
			CHR = myStats->CHR;
			EXP = myStats->EXP;
			LVL = myStats->LVL;
			GOLD = myStats->GOLD;

			RANDOM_STR = myStats->RANDOM_STR;
			RANDOM_DEX = myStats->RANDOM_DEX;
			RANDOM_CON = myStats->RANDOM_CON;
			RANDOM_INT = myStats->RANDOM_INT;
			RANDOM_PER = myStats->RANDOM_PER;
			RANDOM_CHR = myStats->RANDOM_CHR;
			RANDOM_MAXHP = myStats->RANDOM_MAXHP;
			RANDOM_HP = myStats->RANDOM_HP;
			RANDOM_MAXMP = myStats->RANDOM_MAXMP;
			RANDOM_MP = myStats->RANDOM_MP;
			RANDOM_LVL = myStats->RANDOM_LVL;
			RANDOM_GOLD = myStats->RANDOM_GOLD;

			for ( int i = 0; i < NUMPROFICIENCIES; ++i )
			{
				PROFICIENCIES[i] = 0;
			}
			for ( int i = 0; i < NUMPROFICIENCIES; ++i )
			{
				PROFICIENCIES[i] = myStats->PROFICIENCIES[i];
			}
		}

		void setStats(Stat* myStats)
		{
			strcpy(myStats->name, name);
			myStats->type = static_cast<Monster>(type);
			myStats->sex = static_cast<sex_t>(sex);
			myStats->appearance = appearance;
			myStats->HP = HP;
			myStats->MAXHP = MAXHP;
			myStats->OLDHP = myStats->HP;
			myStats->MP = MP;
			myStats->MAXMP = MAXMP;
			myStats->STR = STR;
			myStats->DEX = DEX;
			myStats->CON = CON;
			myStats->INT = INT;
			myStats->PER = PER;
			myStats->CHR = CHR;
			myStats->EXP = EXP;
			myStats->LVL = LVL;
			myStats->GOLD = GOLD;

			myStats->RANDOM_STR = RANDOM_STR;
			myStats->RANDOM_DEX = RANDOM_DEX;
			myStats->RANDOM_CON = RANDOM_CON;
			myStats->RANDOM_INT = RANDOM_INT;
			myStats->RANDOM_PER = RANDOM_PER;
			myStats->RANDOM_CHR = RANDOM_CHR;
			myStats->RANDOM_MAXHP = RANDOM_MAXHP;
			myStats->RANDOM_HP = RANDOM_HP;
			myStats->RANDOM_MAXMP = RANDOM_MAXMP;
			myStats->RANDOM_MP = RANDOM_MP;
			myStats->RANDOM_LVL = RANDOM_LVL;
			myStats->RANDOM_GOLD = RANDOM_GOLD;

			for ( int i = 0; i < NUMPROFICIENCIES; ++i )
			{
				myStats->PROFICIENCIES[i] = PROFICIENCIES[i];
			}
		}

		void setItems(Stat* myStats)
		{
			std::unordered_set<int> equippedSlots;
			for ( auto& it : equipped_items )
			{
				equippedSlots.insert(it.second);
				if ( it.first.percentChance < 100 )
				{
					if ( rand() % 100 >= it.first.percentChance )
					{
						continue;
					}
				}
				if ( it.first.emptyItemEntry )
				{
					continue;
				}
				switch ( it.second )
				{
					case ITEM_SLOT_WEAPON:
						myStats->weapon = newItem(it.first.type, it.first.status, it.first.beatitude, it.first.count, it.first.appearance, it.first.identified, nullptr);
						if ( myStats->weapon )
						{
							myStats->weapon->isDroppable = it.first.dropItemOnDeath;
						}
						break;
					case ITEM_SLOT_SHIELD:
						myStats->shield = newItem(it.first.type, it.first.status, it.first.beatitude, it.first.count, it.first.appearance, it.first.identified, nullptr);
						if ( myStats->shield )
						{
							myStats->shield->isDroppable = it.first.dropItemOnDeath;
						}
						break;
					case ITEM_SLOT_HELM:
						myStats->helmet = newItem(it.first.type, it.first.status, it.first.beatitude, it.first.count, it.first.appearance, it.first.identified, nullptr);
						if ( myStats->helmet )
						{
							myStats->helmet->isDroppable = it.first.dropItemOnDeath;
						}
						break;
					case ITEM_SLOT_ARMOR:
						myStats->breastplate = newItem(it.first.type, it.first.status, it.first.beatitude, it.first.count, it.first.appearance, it.first.identified, nullptr);
						if ( myStats->breastplate )
						{
							myStats->breastplate->isDroppable = it.first.dropItemOnDeath;
						}
						break;
					case ITEM_SLOT_GLOVES:
						myStats->gloves = newItem(it.first.type, it.first.status, it.first.beatitude, it.first.count, it.first.appearance, it.first.identified, nullptr);
						if ( myStats->gloves )
						{
							myStats->gloves->isDroppable = it.first.dropItemOnDeath;
						}
						break;
					case ITEM_SLOT_BOOTS:
						myStats->shoes = newItem(it.first.type, it.first.status, it.first.beatitude, it.first.count, it.first.appearance, it.first.identified, nullptr);
						if ( myStats->shoes )
						{
							myStats->shoes->isDroppable = it.first.dropItemOnDeath;
						}
						break;
					case ITEM_SLOT_CLOAK:
						myStats->cloak = newItem(it.first.type, it.first.status, it.first.beatitude, it.first.count, it.first.appearance, it.first.identified, nullptr);
						if ( myStats->cloak )
						{
							myStats->cloak->isDroppable = it.first.dropItemOnDeath;
						}
						break;
					case ITEM_SLOT_RING:
						myStats->ring = newItem(it.first.type, it.first.status, it.first.beatitude, it.first.count, it.first.appearance, it.first.identified, nullptr);
						if ( myStats->ring )
						{
							myStats->ring->isDroppable = it.first.dropItemOnDeath;
						}
						break;
					case ITEM_SLOT_AMULET:
						myStats->amulet = newItem(it.first.type, it.first.status, it.first.beatitude, it.first.count, it.first.appearance, it.first.identified, nullptr);
						if ( myStats->amulet )
						{
							myStats->amulet->isDroppable = it.first.dropItemOnDeath;
						}
						break;
					case ITEM_SLOT_MASK:
						myStats->mask = newItem(it.first.type, it.first.status, it.first.beatitude, it.first.count, it.first.appearance, it.first.identified, nullptr);
						if ( myStats->mask )
						{
							myStats->mask->isDroppable = it.first.dropItemOnDeath;
						}
						break;
					default:
						break;
				}
			}
			for ( int equipSlots = 0; equipSlots < 10; ++equipSlots )
			{
				if ( !useDefaultEquipment )
				{
					// disable any default item slot spawning.
					myStats->EDITOR_ITEMS[equipSlots * ITEM_SLOT_NUMPROPERTIES] = 0;
				}
				else
				{
					if ( equippedSlots.find(equipSlots * ITEM_SLOT_NUMPROPERTIES) != equippedSlots.end() )
					{
						// disable item slots we (attempted) to fill in.
						myStats->EDITOR_ITEMS[equipSlots * ITEM_SLOT_NUMPROPERTIES] = 0;
					}
				}
			}
			for ( auto& it : inventory_items )
			{
				if ( it.emptyItemEntry )
				{
					continue;
				}
				if ( it.percentChance < 100 )
				{
					if ( rand() % 100 >= it.percentChance )
					{
						continue;
					}
				}
				Item* item = newItem(it.type, it.status, it.beatitude, it.count, it.appearance, it.identified, &myStats->inventory);
				if ( item )
				{
					item->isDroppable = it.dropItemOnDeath;
				}
			}
			if ( !useDefaultInventoryItems )
			{
				for ( int invSlots = ITEM_SLOT_INV_1; invSlots <= ITEM_SLOT_INV_6; invSlots = invSlots + ITEM_SLOT_NUMPROPERTIES )
				{
					myStats->EDITOR_ITEMS[invSlots] = 0;
				}
			}
		}

		void setStatsAndEquipmentToMonster(Stat* myStats)
		{
			//myStats->clearStats();
			setStats(myStats);
			setItems(myStats);

			if ( isMonsterNameGeneric )
			{
				myStats->MISC_FLAGS[STAT_FLAG_MONSTER_NAME_GENERIC] = 1;
			}
			if ( disableMiniboss )
			{
				myStats->MISC_FLAGS[STAT_FLAG_DISABLE_MINIBOSS] = 1;
			}
			if ( forceFriendlyToPlayer )
			{
				myStats->MISC_FLAGS[STAT_FLAG_FORCE_ALLEGIANCE_TO_PLAYER] = 
					Stat::MonsterForceAllegiance::MONSTER_FORCE_PLAYER_ALLY;
			}
			if ( forceEnemyToPlayer )
			{
				myStats->MISC_FLAGS[STAT_FLAG_FORCE_ALLEGIANCE_TO_PLAYER] =
					Stat::MonsterForceAllegiance::MONSTER_FORCE_PLAYER_ENEMY;
			}
			if ( forceRecruitableToPlayer )
			{
				myStats->MISC_FLAGS[STAT_FLAG_FORCE_ALLEGIANCE_TO_PLAYER] =
					Stat::MonsterForceAllegiance::MONSTER_FORCE_PLAYER_RECRUITABLE;
			}
			if ( disableItemDrops )
			{
				myStats->MISC_FLAGS[STAT_FLAG_NO_DROP_ITEMS] = 1;
			}
			if ( xpAwardPercent != 100 )
			{
				myStats->MISC_FLAGS[STAT_FLAG_XP_PERCENT_AWARD] = 1 + std::min(std::max(0, xpAwardPercent), 100);
			}
			if ( castSpellbooksFromInventory )
			{
				myStats->MISC_FLAGS[STAT_FLAG_MONSTER_CAST_INVENTORY_SPELLBOOKS] = 1;
				myStats->MISC_FLAGS[STAT_FLAG_MONSTER_CAST_INVENTORY_SPELLBOOKS] |= (spellbookCastCooldown << 4);
			}
			if ( myStats->type == SHOPKEEPER )
			{
				if ( chosenShopkeeperStore >= 0 )
				{
					myStats->MISC_FLAGS[STAT_FLAG_NPC] = chosenShopkeeperStore + 1;
				}
				Uint8 numItems = 0;
				myStats->MISC_FLAGS[STAT_FLAG_SHOPKEEPER_CUSTOM_PROPERTIES] = 0;
				if ( shopkeeperGenDefaultItems )
				{
					if ( shopkeeperMinItems >= 0 && shopkeeperMaxItems >= 0 )
					{
						numItems = shopkeeperMinItems + rand() % std::max(1, (shopkeeperMaxItems - shopkeeperMinItems + 1));
						myStats->MISC_FLAGS[STAT_FLAG_SHOPKEEPER_CUSTOM_PROPERTIES] |= numItems + 1;
					}
					if ( shopkeeperMaxGeneratedBlessing >= 0 )
					{
						myStats->MISC_FLAGS[STAT_FLAG_SHOPKEEPER_CUSTOM_PROPERTIES] |= (static_cast<Uint8>(shopkeeperMaxGeneratedBlessing + 1) << 8);
					}
					myStats->MISC_FLAGS[STAT_FLAG_SHOPKEEPER_CUSTOM_PROPERTIES] |= (ShopkeeperCustomFlags::ENABLE_GEN_ITEMS << 12); // indicate to use this property.
				}
				else
				{
					myStats->MISC_FLAGS[STAT_FLAG_SHOPKEEPER_CUSTOM_PROPERTIES] |= (ShopkeeperCustomFlags::DISABLE_GEN_ITEMS << 12); // indicate to disable gen items.
				}
			}
		}

		void setStatsAndEquipmentToPlayer(Stat* myStats, int player)
		{
			//if ( player == 0 )
			//{
			//	TextSourceScript tmpScript;
			//	tmpScript.playerClearInventory(true);
			//}
			//else
			//{
			//	// other players
			//	myStats->freePlayerEquipment();
			//	myStats->clearStats();
			//	TextSourceScript tmpScript;
			//	tmpScript.updateClientInformation(player, true, true, TextSourceScript::CLIENT_UPDATE_ALL);
			//}
		}
	};

	void writeAllFromStats(Stat* myStats)
	{
		rapidjson::Document d;
		d.SetObject();
		rapidjson::Value version;
		version.SetInt(1);
		CustomHelpers::addMemberToRoot(d, "version", version);
		readAttributesFromStats(myStats, d);
		readItemsFromStats(myStats, d);
		
		// misc properties
		rapidjson::Value propsObject;
		propsObject.SetObject();
		CustomHelpers::addMemberToRoot(d, "properties", propsObject);
		CustomHelpers::addMemberToSubkey(d, "properties", "monster_name_always_display_as_generic_species", rapidjson::Value(false));
		CustomHelpers::addMemberToSubkey(d, "properties", "populate_empty_equipped_items_with_default", rapidjson::Value(true));
		CustomHelpers::addMemberToSubkey(d, "properties", "populate_default_inventory", rapidjson::Value(true));
		CustomHelpers::addMemberToSubkey(d, "properties", "disable_miniboss_chance", rapidjson::Value(false));
		CustomHelpers::addMemberToSubkey(d, "properties", "force_player_recruitable", rapidjson::Value(false));
		CustomHelpers::addMemberToSubkey(d, "properties", "force_player_friendly", rapidjson::Value(false));
		CustomHelpers::addMemberToSubkey(d, "properties", "force_player_enemy", rapidjson::Value(false));
		CustomHelpers::addMemberToSubkey(d, "properties", "disable_item_drops", rapidjson::Value(false));
		CustomHelpers::addMemberToSubkey(d, "properties", "xp_award_percent", rapidjson::Value(100));
		CustomHelpers::addMemberToSubkey(d, "properties", "enable_casting_inventory_spellbooks", rapidjson::Value(false));
		CustomHelpers::addMemberToSubkey(d, "properties", "spellbook_cast_cooldown", rapidjson::Value(250));

		if ( myStats->type == SHOPKEEPER )
		{
			// shop properties
			CustomHelpers::addMemberToRoot(d, "shopkeeper_properties", propsObject);

			rapidjson::Value shopObject(rapidjson::kObjectType);
			shopObject.SetObject();

			rapidjson::Value storeTypesObject(rapidjson::kObjectType);
			storeTypesObject.AddMember("equipment", rapidjson::Value(1), d.GetAllocator());
			storeTypesObject.AddMember("hats", rapidjson::Value(1), d.GetAllocator());
			storeTypesObject.AddMember("jewelry", rapidjson::Value(1), d.GetAllocator());
			storeTypesObject.AddMember("books", rapidjson::Value(1), d.GetAllocator());
			storeTypesObject.AddMember("apothecary", rapidjson::Value(1), d.GetAllocator());
			storeTypesObject.AddMember("staffs", rapidjson::Value(1), d.GetAllocator());
			storeTypesObject.AddMember("food", rapidjson::Value(1), d.GetAllocator());
			storeTypesObject.AddMember("hardware", rapidjson::Value(1), d.GetAllocator());
			storeTypesObject.AddMember("hunting", rapidjson::Value(1), d.GetAllocator());
			storeTypesObject.AddMember("general", rapidjson::Value(1), d.GetAllocator());

			CustomHelpers::addMemberToSubkey(d, "shopkeeper_properties", "store_type_chances", storeTypesObject);
			CustomHelpers::addMemberToSubkey(d, "shopkeeper_properties", "generate_default_shop_items", rapidjson::Value(true));
			CustomHelpers::addMemberToSubkey(d, "shopkeeper_properties", "num_generated_items_min", rapidjson::Value(10));
			CustomHelpers::addMemberToSubkey(d, "shopkeeper_properties", "num_generated_items_max", rapidjson::Value(15));
			CustomHelpers::addMemberToSubkey(d, "shopkeeper_properties", "generated_item_blessing_max", rapidjson::Value(0));
		}

		// follower details
		rapidjson::Value followersObject;
		followersObject.SetObject();
		CustomHelpers::addMemberToRoot(d, "followers", followersObject);
		CustomHelpers::addMemberToSubkey(d, "followers", "num_followers", rapidjson::Value(0));
		rapidjson::Value followerVariantsObject;
		followerVariantsObject.SetObject();
		CustomHelpers::addMemberToSubkey(d, "followers", "follower_variants", followerVariantsObject);

		writeToFile(d, monstertypename[myStats->type]);
	}

	void readItemsFromStats(Stat* myStats, rapidjson::Document& d)
	{
		rapidjson::Value equippedItemsObject;
		equippedItemsObject.SetObject();
		CustomHelpers::addMemberToRoot(d, "equipped_items", equippedItemsObject);
		addMemberFromItem(d, "equipped_items", "weapon", myStats->weapon);
		addMemberFromItem(d, "equipped_items", "shield", myStats->shield);
		addMemberFromItem(d, "equipped_items", "helmet", myStats->helmet);
		addMemberFromItem(d, "equipped_items", "breastplate", myStats->breastplate);
		addMemberFromItem(d, "equipped_items", "gloves", myStats->gloves);
		addMemberFromItem(d, "equipped_items", "shoes", myStats->shoes);
		addMemberFromItem(d, "equipped_items", "cloak", myStats->cloak);
		addMemberFromItem(d, "equipped_items", "ring", myStats->ring);
		addMemberFromItem(d, "equipped_items", "amulet", myStats->amulet);
		addMemberFromItem(d, "equipped_items", "mask", myStats->mask);

		rapidjson::Value invItemsArray;
		invItemsArray.SetArray();
		CustomHelpers::addMemberToRoot(d, "inventory_items", invItemsArray);
		for ( node_t* node = myStats->inventory.first; node; node = node->next )
		{
			Item* item = (Item*)node->element;
			if ( item )
			{
				addArrayMemberFromItem(d, "inventory_items", item);
			}
		}
	}

	void readAttributesFromStats(Stat* myStats, rapidjson::Document& d)
	{
		rapidjson::Value statsObject;
		statsObject.SetObject();
		CustomHelpers::addMemberToRoot(d, "stats", statsObject);

		StatEntry statEntry(myStats);
		CustomHelpers::addMemberToSubkey(d, "stats", "name", rapidjson::Value(statEntry.name, d.GetAllocator()));
		CustomHelpers::addMemberToSubkey(d, "stats", "type", rapidjson::Value(monstertypename[statEntry.type], d.GetAllocator()));
		CustomHelpers::addMemberToSubkey(d, "stats", "sex", rapidjson::Value(statEntry.sex));
		CustomHelpers::addMemberToSubkey(d, "stats", "appearance", rapidjson::Value(statEntry.appearance));
		CustomHelpers::addMemberToSubkey(d, "stats", "HP", rapidjson::Value(statEntry.HP));
		CustomHelpers::addMemberToSubkey(d, "stats", "MAXHP", rapidjson::Value(statEntry.MAXHP));
		CustomHelpers::addMemberToSubkey(d, "stats", "MP", rapidjson::Value(statEntry.MP));
		CustomHelpers::addMemberToSubkey(d, "stats", "MAXMP", rapidjson::Value(statEntry.MAXMP));
		CustomHelpers::addMemberToSubkey(d, "stats", "STR", rapidjson::Value(statEntry.STR));
		CustomHelpers::addMemberToSubkey(d, "stats", "DEX", rapidjson::Value(statEntry.DEX));
		CustomHelpers::addMemberToSubkey(d, "stats", "CON", rapidjson::Value(statEntry.CON));
		CustomHelpers::addMemberToSubkey(d, "stats", "INT", rapidjson::Value(statEntry.INT));
		CustomHelpers::addMemberToSubkey(d, "stats", "PER", rapidjson::Value(statEntry.PER));
		CustomHelpers::addMemberToSubkey(d, "stats", "CHR", rapidjson::Value(statEntry.CHR));
		CustomHelpers::addMemberToSubkey(d, "stats", "EXP", rapidjson::Value(statEntry.EXP));
		CustomHelpers::addMemberToSubkey(d, "stats", "LVL", rapidjson::Value(statEntry.LVL));
		CustomHelpers::addMemberToSubkey(d, "stats", "GOLD", rapidjson::Value(statEntry.GOLD));

		rapidjson::Value miscStatsObject;
		miscStatsObject.SetObject();
		CustomHelpers::addMemberToRoot(d, "misc_stats", miscStatsObject);

		CustomHelpers::addMemberToSubkey(d, "misc_stats", "RANDOM_STR", rapidjson::Value(statEntry.RANDOM_STR));
		CustomHelpers::addMemberToSubkey(d, "misc_stats", "RANDOM_DEX", rapidjson::Value(statEntry.RANDOM_DEX));
		CustomHelpers::addMemberToSubkey(d, "misc_stats", "RANDOM_CON", rapidjson::Value(statEntry.RANDOM_CON));
		CustomHelpers::addMemberToSubkey(d, "misc_stats", "RANDOM_INT", rapidjson::Value(statEntry.RANDOM_INT));
		CustomHelpers::addMemberToSubkey(d, "misc_stats", "RANDOM_PER", rapidjson::Value(statEntry.RANDOM_PER));
		CustomHelpers::addMemberToSubkey(d, "misc_stats", "RANDOM_CHR", rapidjson::Value(statEntry.RANDOM_CHR));
		CustomHelpers::addMemberToSubkey(d, "misc_stats", "RANDOM_MAXHP", rapidjson::Value(statEntry.RANDOM_MAXHP));
		CustomHelpers::addMemberToSubkey(d, "misc_stats", "RANDOM_HP", rapidjson::Value(statEntry.RANDOM_HP));
		CustomHelpers::addMemberToSubkey(d, "misc_stats", "RANDOM_MAXMP", rapidjson::Value(statEntry.RANDOM_MAXMP));
		CustomHelpers::addMemberToSubkey(d, "misc_stats", "RANDOM_MP", rapidjson::Value(statEntry.RANDOM_MP));
		CustomHelpers::addMemberToSubkey(d, "misc_stats", "RANDOM_LVL", rapidjson::Value(statEntry.RANDOM_LVL));
		CustomHelpers::addMemberToSubkey(d, "misc_stats", "RANDOM_GOLD", rapidjson::Value(statEntry.RANDOM_GOLD));

		rapidjson::Value profObject;
		profObject.SetObject();
		CustomHelpers::addMemberToRoot(d, "proficiencies", profObject);

		for ( int i = 0; i < NUMPROFICIENCIES; ++i )
		{
			CustomHelpers::addMemberToSubkey(d, "proficiencies", getSkillLangEntry(i), rapidjson::Value(statEntry.PROFICIENCIES[i]));
		}
	}

	bool readKeyToStatEntry(StatEntry& statEntry, rapidjson::Value::ConstMemberIterator& itr)
	{
		std::string name = itr->name.GetString();
		if ( name.compare("name") == 0 )
		{
			strcpy(statEntry.name, itr->value.GetString());
			return true;
		}
		else if ( name.compare("type") == 0 )
		{
			std::string val = itr->value.GetString();
			for ( int i = 0; i < NUMMONSTERS; ++i )
			{
				if ( val.compare(monstertypename[i]) == 0 )
				{
					statEntry.type = i;
					break;
				}
			}
			return true;
		}
		else if ( name.compare("sex") == 0 )
		{
			statEntry.sex = static_cast<sex_t>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("appearance") == 0 )
		{
			statEntry.appearance = static_cast<Uint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("HP") == 0 )
		{
			statEntry.HP = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("MAXHP") == 0 )
		{
			statEntry.MAXHP = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("MP") == 0 )
		{
			statEntry.MP = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("MAXMP") == 0 )
		{
			statEntry.MAXMP = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("STR") == 0 )
		{
			statEntry.STR = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("DEX") == 0 )
		{
			statEntry.DEX = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("CON") == 0 )
		{
			statEntry.CON = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("INT") == 0 )
		{
			statEntry.INT = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("PER") == 0 )
		{
			statEntry.PER = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("CHR") == 0 )
		{
			statEntry.CHR = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("EXP") == 0 )
		{
			statEntry.EXP = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("LVL") == 0 )
		{
			statEntry.LVL = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("GOLD") == 0 )
		{
			statEntry.GOLD = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("RANDOM_STR") == 0 )
		{
			statEntry.RANDOM_STR = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("RANDOM_DEX") == 0 )
		{
			statEntry.RANDOM_DEX = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("RANDOM_CON") == 0 )
		{
			statEntry.RANDOM_CON = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("RANDOM_INT") == 0 )
		{
			statEntry.RANDOM_INT = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("RANDOM_PER") == 0 )
		{
			statEntry.RANDOM_PER = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("RANDOM_CHR") == 0 )
		{
			statEntry.RANDOM_CHR = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("RANDOM_MAXHP") == 0 )
		{
			statEntry.RANDOM_MAXHP = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("RANDOM_HP") == 0 )
		{
			statEntry.RANDOM_HP = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("RANDOM_MAXMP") == 0 )
		{
			statEntry.RANDOM_MAXMP = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("RANDOM_MP") == 0 )
		{
			statEntry.RANDOM_MP = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("RANDOM_LVL") == 0 )
		{
			statEntry.RANDOM_LVL = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else if ( name.compare("RANDOM_GOLD") == 0 )
		{
			statEntry.RANDOM_GOLD = static_cast<Sint32>(itr->value.GetInt());
			return true;
		}
		else
		{
			for ( int i = 0; i < NUMPROFICIENCIES; ++i )
			{
				if ( name.compare(getSkillLangEntry(i)) == 0 )
				{
					statEntry.PROFICIENCIES[i] = static_cast<Sint32>(itr->value.GetInt());
					return true;
				}
			}
		}
		return false;
	}

	void addArrayMemberFromItem(rapidjson::Document& d, std::string rootKey, Item* item)
	{
		if ( item )
		{
			rapidjson::Value itemObject(rapidjson::kObjectType);
			ItemEntry itemEntry(*item);
			itemEntry.setValueFromAttributes(d, itemObject);
			CustomHelpers::addArrayMemberToSubkey(d, rootKey, itemObject);
		}
	}
	void addMemberFromItem(rapidjson::Document& d, std::string rootKey, std::string key, Item* item)
	{
		if ( item )
		{
			rapidjson::Value itemObject(rapidjson::kObjectType);
			ItemEntry itemEntry(*item);
			itemEntry.setValueFromAttributes(d, itemObject);
			CustomHelpers::addMemberToSubkey(d, rootKey, key.c_str(), itemObject);
		}
	}

	void writeToFile(rapidjson::Document& d, std::string monsterFileName)
	{
		int filenum = 0;
		std::string testPath = "/data/custom-monsters/monster_" + monsterFileName + "_export" + std::to_string(filenum) + ".json";
		while ( PHYSFS_getRealDir(testPath.c_str()) != nullptr && filenum < 1000 )
		{
			++filenum;
			testPath = "/data/custom-monsters/monster_" + monsterFileName + "_export" + std::to_string(filenum) + ".json";
		}
		std::string outputPath = PHYSFS_getRealDir("/data/custom-monsters/");
		outputPath.append(PHYSFS_getDirSeparator());
		std::string fileName = "data/custom-monsters/monster_" + monsterFileName + "_export" + std::to_string(filenum) + ".json";
		outputPath.append(fileName.c_str());


		File* fp = FileIO::open(outputPath.c_str(), "wb");
		if ( !fp )
		{
			return;
		}
		rapidjson::StringBuffer os;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
		d.Accept(writer);
		fp->write(os.GetString(), sizeof(char), os.GetSize());
		FileIO::close(fp);
	}

	StatEntry* readFromFile(std::string monsterFileName)
	{
		std::string filePath = "/data/custom-monsters/";
		filePath.append(monsterFileName);
		if ( filePath.find(".json") == std::string::npos )
		{
			filePath.append(".json");
		}
		if ( PHYSFS_getRealDir(filePath.c_str()) )
		{
			std::string inputPath = PHYSFS_getRealDir(filePath.c_str());
			inputPath.append(filePath);

			File* fp = FileIO::open(inputPath.c_str(), "rb");
			if ( !fp )
			{
				printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
				return nullptr;
			}
			char buf[65536];
			int count = fp->read(buf, sizeof(buf[0]), sizeof(buf));
			buf[count] = '\0';
			rapidjson::StringStream is(buf);
			FileIO::close(fp);

			rapidjson::Document d;
			d.ParseStream(is);


			if ( !d.HasMember("version") )
			{
				printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
				return nullptr;
			}
			StatEntry* statEntry = new StatEntry();
			int version = d["version"].GetInt();
			const rapidjson::Value& stats = d["stats"];
			for ( rapidjson::Value::ConstMemberIterator stat_itr = stats.MemberBegin(); stat_itr != stats.MemberEnd(); ++stat_itr )
			{
				readKeyToStatEntry(*statEntry, stat_itr);
			}
			const rapidjson::Value& miscStats = d["misc_stats"];
			for ( rapidjson::Value::ConstMemberIterator stat_itr = miscStats.MemberBegin(); stat_itr != miscStats.MemberEnd(); ++stat_itr )
			{
				readKeyToStatEntry(*statEntry, stat_itr);
			}
			const rapidjson::Value& proficiencies = d["proficiencies"];
			for ( rapidjson::Value::ConstMemberIterator stat_itr = proficiencies.MemberBegin(); stat_itr != proficiencies.MemberEnd(); ++stat_itr )
			{
				readKeyToStatEntry(*statEntry, stat_itr);
			}
			const rapidjson::Value& equipped_items = d["equipped_items"];
			for ( rapidjson::Value::ConstMemberIterator itemSlot_itr = equipped_items.MemberBegin(); itemSlot_itr != equipped_items.MemberEnd(); ++itemSlot_itr )
			{
				std::string slotName = itemSlot_itr->name.GetString();
				if ( itemSlot_itr->value.MemberCount() > 0 )
				{
					if ( itemSlot_itr->value.IsArray() )
					{
						std::vector<std::pair<ItemEntry, int>> itemsToChoose;
						// a selection of items in the slot. need to choose 1.
						for ( rapidjson::Value::ConstValueIterator itemArray_itr = itemSlot_itr->value.Begin(); itemArray_itr != itemSlot_itr->value.End(); ++itemArray_itr )
						{
							ItemEntry item;
							for ( rapidjson::Value::ConstMemberIterator item_itr = itemArray_itr->MemberBegin(); item_itr != itemArray_itr->MemberEnd(); ++item_itr )
							{
								item.readKeyToItemEntry(item_itr);
							}
							itemsToChoose.push_back(std::make_pair(item, getSlotFromKeyName(slotName)));
						}
						if ( itemsToChoose.size() > 0 )
						{
							std::vector<int> itemChances(itemsToChoose.size(), 0);
							int index = 0;
							for ( auto& pair : itemsToChoose )
							{
								itemChances.at(index) = pair.first.weightedChance;
								++index;
							}

							std::discrete_distribution<> itemWeightedDistribution(itemChances.begin(), itemChances.end());
							int result = itemWeightedDistribution(monsterStatSeed);
							statEntry->equipped_items.push_back(std::make_pair(itemsToChoose.at(result).first, itemsToChoose.at(result).second));
						}
					}
					else
					{
						ItemEntry item;
						for ( rapidjson::Value::ConstMemberIterator item_itr = itemSlot_itr->value.MemberBegin(); item_itr != itemSlot_itr->value.MemberEnd(); ++item_itr )
						{
							item.readKeyToItemEntry(item_itr);
						}
						statEntry->equipped_items.push_back(std::make_pair(item, getSlotFromKeyName(slotName)));
					}
				}
			}
			const rapidjson::Value& inventory_items = d["inventory_items"];
			for ( rapidjson::Value::ConstValueIterator itemSlot_itr = inventory_items.Begin(); itemSlot_itr != inventory_items.End(); ++itemSlot_itr )
			{
				if ( itemSlot_itr->IsArray() )
				{
					std::vector<ItemEntry> itemsToChoose;
					// a selection of items in the slot. need to choose 1.
					for ( rapidjson::Value::ConstValueIterator itemArray_itr = itemSlot_itr->Begin(); itemArray_itr != itemSlot_itr->End(); ++itemArray_itr )
					{
						ItemEntry item;
						for ( rapidjson::Value::ConstMemberIterator item_itr = itemArray_itr->MemberBegin(); item_itr != itemArray_itr->MemberEnd(); ++item_itr )
						{
							item.readKeyToItemEntry(item_itr);
						}
						itemsToChoose.push_back(item);
					}
					if ( itemsToChoose.size() > 0 )
					{
						std::vector<int> itemChances(itemsToChoose.size(), 0);
						int index = 0;
						for ( auto& i : itemsToChoose )
						{
							itemChances.at(index) = i.weightedChance;
							++index;
						}

						std::discrete_distribution<> itemWeightedDistribution(itemChances.begin(), itemChances.end());
						int result = itemWeightedDistribution(monsterStatSeed);
						statEntry->inventory_items.push_back(itemsToChoose.at(result));
					}
				}
				else
				{
					ItemEntry item;
					for ( rapidjson::Value::ConstMemberIterator item_itr = itemSlot_itr->MemberBegin(); item_itr != itemSlot_itr->MemberEnd(); ++item_itr )
					{
						item.readKeyToItemEntry(item_itr);
					}
					statEntry->inventory_items.push_back(item);
				}
			}
			if ( d.HasMember("followers") )
			{
				const rapidjson::Value& numFollowersVal = d["followers"]["num_followers"];
				statEntry->numFollowers = numFollowersVal.GetInt();
				const rapidjson::Value& followers = d["followers"]["follower_variants"];

				statEntry->followerVariants.clear();
				for ( rapidjson::Value::ConstMemberIterator follower_itr = followers.MemberBegin(); follower_itr != followers.MemberEnd(); ++follower_itr )
				{
					statEntry->followerVariants.push_back(std::make_pair(follower_itr->name.GetString(), follower_itr->value.GetInt()));
				}
			}
			if ( d.HasMember("properties") )
			{
				if ( d["properties"].HasMember("monster_name_always_display_as_generic_species") )
				{
					statEntry->isMonsterNameGeneric = d["properties"]["monster_name_always_display_as_generic_species"].GetBool();
				}
				if ( d["properties"].HasMember("populate_empty_equipped_items_with_default") )
				{
					statEntry->useDefaultEquipment = d["properties"]["populate_empty_equipped_items_with_default"].GetBool();
				}
				if ( d["properties"].HasMember("populate_default_inventory") )
				{
					statEntry->useDefaultInventoryItems = d["properties"]["populate_default_inventory"].GetBool();
				}
				if ( d["properties"].HasMember("disable_miniboss_chance") )
				{
					statEntry->disableMiniboss = d["properties"]["disable_miniboss_chance"].GetBool();
				}
				if ( d["properties"].HasMember("force_player_recruitable") )
				{
					statEntry->forceRecruitableToPlayer = d["properties"]["force_player_recruitable"].GetBool();
				}
				if ( d["properties"].HasMember("force_player_friendly") )
				{
					statEntry->forceFriendlyToPlayer = d["properties"]["force_player_friendly"].GetBool();
				}
				if ( d["properties"].HasMember("force_player_enemy") )
				{
					statEntry->forceEnemyToPlayer = d["properties"]["force_player_enemy"].GetBool();
				}
				if ( d["properties"].HasMember("disable_item_drops") )
				{
					statEntry->disableItemDrops = d["properties"]["disable_item_drops"].GetBool();
				}
				if ( d["properties"].HasMember("xp_award_percent") )
				{
					statEntry->xpAwardPercent = d["properties"]["xp_award_percent"].GetInt();
				}
				if ( d["properties"].HasMember("enable_casting_inventory_spellbooks") )
				{
					statEntry->castSpellbooksFromInventory = d["properties"]["enable_casting_inventory_spellbooks"].GetBool();
				}
				if ( d["properties"].HasMember("spellbook_cast_cooldown") )
				{
					statEntry->spellbookCastCooldown = d["properties"]["spellbook_cast_cooldown"].GetInt();
				}
			}
			if ( d.HasMember("shopkeeper_properties") )
			{
				if ( d["shopkeeper_properties"].HasMember("store_type_chances") )
				{
					for ( rapidjson::Value::ConstMemberIterator types_itr = d["shopkeeper_properties"]["store_type_chances"].MemberBegin(); 
						types_itr != d["shopkeeper_properties"]["store_type_chances"].MemberEnd(); ++types_itr )
					{
						statEntry->shopkeeperStoreTypes.push_back(std::make_pair(types_itr->name.GetString(), types_itr->value.GetInt()));
					}
					if ( !statEntry->shopkeeperStoreTypes.empty() )
					{
						std::vector<int> storeChances(statEntry->shopkeeperStoreTypes.size(), 0);
						int index = 0;
						for ( auto& chance : storeChances )
						{
							chance = statEntry->shopkeeperStoreTypes.at(index).second;
							++index;
						}

						std::discrete_distribution<> storeTypeWeightedDistribution(storeChances.begin(), storeChances.end());
						std::string result = statEntry->shopkeeperStoreTypes.at(storeTypeWeightedDistribution(monsterStatSeed)).first;
						index = 0;
						for ( auto& lookup : shopkeeperTypeStrings )
						{
							if ( lookup.compare(result) == 0 )
							{
								statEntry->chosenShopkeeperStore = index;
								break;
							}
							++index;
						}
					}
					if ( d["shopkeeper_properties"].HasMember("generate_default_shop_items") )
					{
						statEntry->shopkeeperGenDefaultItems = d["shopkeeper_properties"]["generate_default_shop_items"].GetBool();
					}
					if ( d["shopkeeper_properties"].HasMember("num_generated_items_min") )
					{
						statEntry->shopkeeperMinItems = d["shopkeeper_properties"]["num_generated_items_min"].GetInt();
					}
					if ( d["shopkeeper_properties"].HasMember("num_generated_items_max") )
					{
						statEntry->shopkeeperMaxItems = d["shopkeeper_properties"]["num_generated_items_max"].GetInt();
					}
					if ( d["shopkeeper_properties"].HasMember("generated_item_blessing_max") )
					{
						statEntry->shopkeeperMaxGeneratedBlessing = d["shopkeeper_properties"]["generated_item_blessing_max"].GetInt();
					}
				}
			}
			printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
			return statEntry;
		}
		else
		{
			printlog("[JSON]: Error: Could not locate json file %s", filePath.c_str());
		}
		return nullptr;
	}
};
extern MonsterStatCustomManager monsterStatCustomManager;

class MonsterCurveCustomManager
{
	bool usingCustomManager = false;
public:
	std::mt19937 curveSeed;
	MonsterCurveCustomManager() :
		curveSeed(rand())
	{};

	class MonsterCurveEntry
	{
	public:
		int monsterType = NOTHING;
		int levelmin = 0;
		int levelmax = 99;
		int chance = 1;
		int fallbackMonsterType = NOTHING;
		std::vector<std::pair<std::string, int>> variants;
		MonsterCurveEntry(std::string monsterStr, int levelNumMin, int levelNumMax, int chanceNum, std::string fallbackMonsterStr)
		{
			monsterType = getMonsterTypeFromString(monsterStr);
			fallbackMonsterType = getMonsterTypeFromString(fallbackMonsterStr);
			levelmin = levelNumMin;
			levelmax = levelNumMax;
			chance = chanceNum;
		};
		void addVariant(std::string variantName, int chance)
		{
			variants.push_back(std::make_pair(variantName, chance));
		}
	};

	class LevelCurve
	{
	public:
		std::string mapName = "";
		std::vector<MonsterCurveEntry> monsterCurve;
		std::vector<MonsterCurveEntry> fixedSpawns;
	};

	std::vector<LevelCurve> allLevelCurves;
	inline bool inUse() { return usingCustomManager; };

	void readFromFile()
	{
		allLevelCurves.clear();
		usingCustomManager = false;
		if ( PHYSFS_getRealDir("/data/monstercurve.json") )
		{
			std::string inputPath = PHYSFS_getRealDir("/data/monstercurve.json");
			inputPath.append("/data/monstercurve.json");

			File* fp = FileIO::open(inputPath.c_str(), "rb");
			if ( !fp )
			{
				printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
				return;
			}
			char buf[65536];
			int count = fp->read(buf, sizeof(buf[0]), sizeof(buf));
			buf[count] = '\0';
			rapidjson::StringStream is(buf);
			FileIO::close(fp);

			rapidjson::Document d;
			d.ParseStream(is);
			if ( !d.HasMember("version") )
			{
				printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
				return;
			}
			int version = d["version"].GetInt();

			if ( d.HasMember("levels") )
			{
				usingCustomManager = true;
				const rapidjson::Value& levels = d["levels"];
				for ( rapidjson::Value::ConstMemberIterator map_itr = levels.MemberBegin(); map_itr != levels.MemberEnd(); ++map_itr )
				{
					LevelCurve newCurve;
					newCurve.mapName = map_itr->name.GetString();
					const rapidjson::Value& randomGeneration = map_itr->value["random_generation_monsters"];
					for ( rapidjson::Value::ConstValueIterator monsters_itr = randomGeneration.Begin(); monsters_itr != randomGeneration.End(); ++monsters_itr )
					{
						const rapidjson::Value& monster = *monsters_itr;
						MonsterCurveEntry newMonster(monster["name"].GetString(),
							monster["dungeon_depth_minimum"].GetInt(),
							monster["dungeon_depth_maximum"].GetInt(),
							monster["weighted_chance"].GetInt(),
							"");

						if ( monster.HasMember("variants") )
						{
							for ( rapidjson::Value::ConstMemberIterator var_itr = monster["variants"].MemberBegin();
								var_itr != monster["variants"].MemberEnd(); ++var_itr )
							{
								newMonster.addVariant(var_itr->name.GetString(), var_itr->value.GetInt());
							}
						}
						newCurve.monsterCurve.push_back(newMonster);
					}

					if ( map_itr->value.HasMember("fixed_monsters") )
					{
						const rapidjson::Value& fixedGeneration = map_itr->value["fixed_monsters"];
						for ( rapidjson::Value::ConstValueIterator monsters_itr = fixedGeneration.Begin(); monsters_itr != fixedGeneration.End(); ++monsters_itr )
						{
							const rapidjson::Value& monster = *monsters_itr;
							MonsterCurveEntry newMonster(monster["name"].GetString(), 0, 255, 1, "");

							if ( monster.HasMember("variants") )
							{
								for ( rapidjson::Value::ConstMemberIterator var_itr = monster["variants"].MemberBegin();
									var_itr != monster["variants"].MemberEnd(); ++var_itr )
								{
									newMonster.addVariant(var_itr->name.GetString(), var_itr->value.GetInt());
								}
							}
							newCurve.fixedSpawns.push_back(newMonster);
						}
					}
					allLevelCurves.push_back(newCurve);
				}
			}
			printCurve(allLevelCurves);
			printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
		}
	}

	static int getMonsterTypeFromString(std::string monsterStr)
	{
		if ( monsterStr.compare("") == 0 )
		{
			return NOTHING;
		}
		for ( int i = NOTHING; i < NUMMONSTERS; ++i )
		{
			if ( monsterStr.compare(monstertypename[i]) == 0 )
			{
				return i;
			}
		}
		return NOTHING;
	}
	void printCurve(std::vector<LevelCurve> toPrint)
	{
		return;
		for ( LevelCurve curve : toPrint )
		{
			printlog("Map Name: %s", curve.mapName.c_str());
			for ( MonsterCurveEntry monsters : curve.monsterCurve )
			{
				printlog("[MonsterCurveCustomManager]: Monster: %s | lvl: %d-%d | chance: %d | fallback type: %s", monstertypename[monsters.monsterType],
					monsters.levelmin, monsters.levelmax, monsters.chance, monstertypename[monsters.fallbackMonsterType]);
			}
		}
	}
	bool curveExistsForCurrentMapName(std::string currentMap)
	{
		if ( !inUse() )
		{
			return false;
		}
		if ( currentMap.compare("") == 0 )
		{
			return false;
		}
		for ( LevelCurve curve : allLevelCurves )
		{
			if ( curve.mapName.compare(currentMap) == 0 )
			{
				//printlog("[MonsterCurveCustomManager]: curveExistsForCurrentMapName: true");
				return true;
			}
		}
		return false;
	}
	int rollMonsterFromCurve(std::string currentMap)
	{
		std::vector<int> monsterCurveChances(NUMMONSTERS, 0);

		for ( LevelCurve curve : allLevelCurves )
		{
			if ( curve.mapName.compare(currentMap) == 0 )
			{
				for ( MonsterCurveEntry& monster : curve.monsterCurve )
				{
					if ( currentlevel >= monster.levelmin && currentlevel <= monster.levelmax )
					{
						if ( monster.monsterType != NOTHING )
						{
							monsterCurveChances[monster.monsterType] += monster.chance;
						}
					}
					else
					{
						if ( monster.fallbackMonsterType != NOTHING )
						{
							monsterCurveChances[monster.fallbackMonsterType] += monster.chance;
						}
					}
				}
				std::discrete_distribution<> monsterWeightedDistribution(monsterCurveChances.begin(), monsterCurveChances.end());
				int result = monsterWeightedDistribution(curveSeed);
				//printlog("[MonsterCurveCustomManager]: Rolled: %d", result);
				return result;
			}
		}
		printlog("[MonsterCurveCustomManager]: Error: default to nothing.");
		return NOTHING;
	}
	std::string rollMonsterVariant(std::string currentMap, int monsterType)
	{
		for ( LevelCurve& curve : allLevelCurves )
		{
			if ( curve.mapName.compare(currentMap) == 0 )
			{
				std::vector<std::string> variantResults;
				std::vector<int> variantChances;
				for ( MonsterCurveEntry& monster : curve.monsterCurve )
				{
					if ( currentlevel >= monster.levelmin && currentlevel <= monster.levelmax )
					{
						if ( monster.monsterType == monsterType && monster.variants.size() > 0 )
						{
							for ( auto& pair : monster.variants )
							{
								auto find = std::find(variantResults.begin(), variantResults.end(), pair.first);
								if ( find == variantResults.end() )
								{
									variantResults.push_back(pair.first);
									variantChances.push_back(pair.second);
								}
								else
								{
									size_t dist = static_cast<size_t>(std::distance(variantResults.begin(), find));
									variantChances.at(dist) += pair.second;
								}
							}

						}
					}
				}
				if ( !variantResults.empty() )
				{
					std::discrete_distribution<> variantWeightedDistribution(variantChances.begin(), variantChances.end());
					int result = variantWeightedDistribution(curveSeed);
					return variantResults[result];
				}
			}
		}
		return "default";
	}
	std::string rollFixedMonsterVariant(std::string currentMap, int monsterType)
	{
		for ( LevelCurve& curve : allLevelCurves )
		{
			if ( curve.mapName.compare(currentMap) == 0 )
			{
				for ( MonsterCurveEntry& monster : curve.fixedSpawns )
				{
					if ( monster.monsterType == monsterType && monster.variants.size() > 0 )
					{
						std::vector<int> variantChances(monster.variants.size(), 0);
						int index = 0;
						for ( auto& pair : monster.variants )
						{
							variantChances.at(index) = pair.second;
							++index;
						}

						std::discrete_distribution<> variantWeightedDistribution(variantChances.begin(), variantChances.end());
						int result = variantWeightedDistribution(curveSeed);
						return monster.variants.at(result).first;
					}
				}
			}
		}
		return "default";
	}

	void createMonsterFromFile(Entity* entity, Stat* myStats, const std::string& filename, Monster& outMonsterType)
	{
		MonsterStatCustomManager::StatEntry* statEntry = monsterStatCustomManager.readFromFile(filename.c_str());
		if ( statEntry )
		{
			statEntry->setStatsAndEquipmentToMonster(myStats);
			outMonsterType = myStats->type;
			while ( statEntry->numFollowers > 0 )
			{
				std::string followerName = statEntry->getFollowerVariant();
				if ( followerName.compare("") && followerName.compare("none") )
				{
					MonsterStatCustomManager::StatEntry* followerEntry = monsterStatCustomManager.readFromFile(followerName.c_str());
					if ( followerEntry )
					{
						Entity* summonedFollower = summonMonster(static_cast<Monster>(followerEntry->type), entity->x, entity->y);
						if ( summonedFollower )
						{
							if ( summonedFollower->getStats() )
							{
								followerEntry->setStatsAndEquipmentToMonster(summonedFollower->getStats());
								summonedFollower->getStats()->leader_uid = entity->getUID();
							}
						}
						delete followerEntry;
					}
					else
					{
						Entity* summonedFollower = summonMonster(myStats->type, entity->x, entity->y);
						if ( summonedFollower )
						{
							if ( summonedFollower->getStats() )
							{
								summonedFollower->getStats()->leader_uid = entity->getUID();
							}
						}
					}
				}
				--statEntry->numFollowers;
			}
			delete statEntry;
		}
	}

	void writeSampleToDocument()
	{
		rapidjson::Document d;
		d.SetObject();

		CustomHelpers::addMemberToRoot(d, "version", rapidjson::Value(1));
		rapidjson::Value levelObj(rapidjson::kObjectType);
		levelObj.AddMember("The Mines", rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		levelObj["The Mines"].AddMember("fixed_monsters", rapidjson::Value(rapidjson::kArrayType), d.GetAllocator());

		auto& fm = levelObj["The Mines"]["fixed_monsters"];
		fm.PushBack(rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		fm[rapidjson::SizeType(0)].AddMember("name", "rat", d.GetAllocator());
		fm[rapidjson::SizeType(0)].AddMember("variants", rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		fm[rapidjson::SizeType(0)]["variants"].AddMember("default", rapidjson::Value(1), d.GetAllocator());

		fm.PushBack(rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		fm[rapidjson::SizeType(1)].AddMember("name", "skeleton", d.GetAllocator());
		fm[rapidjson::SizeType(1)].AddMember("variants", rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		fm[rapidjson::SizeType(1)]["variants"].AddMember("default", rapidjson::Value(1), d.GetAllocator());
		
		fm.PushBack(rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		fm[rapidjson::SizeType(2)].AddMember("name", "spider", d.GetAllocator());
		fm[rapidjson::SizeType(2)].AddMember("variants", rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		fm[rapidjson::SizeType(2)]["variants"].AddMember("default", rapidjson::Value(1), d.GetAllocator());

		fm.PushBack(rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		fm[rapidjson::SizeType(3)].AddMember("name", "troll", d.GetAllocator());
		fm[rapidjson::SizeType(3)].AddMember("variants", rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		fm[rapidjson::SizeType(3)]["variants"].AddMember("default", rapidjson::Value(1), d.GetAllocator());

		levelObj["The Mines"].AddMember("random_generation_monsters", rapidjson::Value(rapidjson::kArrayType), d.GetAllocator());

		auto& mines = levelObj["The Mines"]["random_generation_monsters"];
		mines.PushBack(rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		mines[rapidjson::SizeType(0)].AddMember("name", "rat", d.GetAllocator());
		mines[rapidjson::SizeType(0)].AddMember("weighted_chance", rapidjson::Value(4), d.GetAllocator());
		mines[rapidjson::SizeType(0)].AddMember("dungeon_depth_minimum", rapidjson::Value(0), d.GetAllocator());
		mines[rapidjson::SizeType(0)].AddMember("dungeon_depth_maximum", rapidjson::Value(99), d.GetAllocator());
		mines[rapidjson::SizeType(0)].AddMember("variants", rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		mines[rapidjson::SizeType(0)]["variants"].AddMember("default", rapidjson::Value(1), d.GetAllocator());

		mines.PushBack(rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		mines[rapidjson::SizeType(1)].AddMember("name", "skeleton", d.GetAllocator());
		mines[rapidjson::SizeType(1)].AddMember("weighted_chance", rapidjson::Value(4), d.GetAllocator());
		mines[rapidjson::SizeType(1)].AddMember("dungeon_depth_minimum", rapidjson::Value(0), d.GetAllocator());
		mines[rapidjson::SizeType(1)].AddMember("dungeon_depth_maximum", rapidjson::Value(99), d.GetAllocator());
		mines[rapidjson::SizeType(1)].AddMember("variants", rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		mines[rapidjson::SizeType(1)]["variants"].AddMember("default", rapidjson::Value(1), d.GetAllocator());

		mines.PushBack(rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		mines[rapidjson::SizeType(2)].AddMember("name", "spider", d.GetAllocator());
		mines[rapidjson::SizeType(2)].AddMember("weighted_chance", rapidjson::Value(1), d.GetAllocator());
		mines[rapidjson::SizeType(2)].AddMember("dungeon_depth_minimum", rapidjson::Value(2), d.GetAllocator());
		mines[rapidjson::SizeType(2)].AddMember("dungeon_depth_maximum", rapidjson::Value(99), d.GetAllocator());
		mines[rapidjson::SizeType(2)].AddMember("variants", rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		mines[rapidjson::SizeType(2)]["variants"].AddMember("default", rapidjson::Value(1), d.GetAllocator());

		mines.PushBack(rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		mines[rapidjson::SizeType(3)].AddMember("name", "troll", d.GetAllocator());
		mines[rapidjson::SizeType(3)].AddMember("weighted_chance", rapidjson::Value(1), d.GetAllocator());
		mines[rapidjson::SizeType(3)].AddMember("dungeon_depth_minimum", rapidjson::Value(2), d.GetAllocator());
		mines[rapidjson::SizeType(3)].AddMember("dungeon_depth_maximum", rapidjson::Value(99), d.GetAllocator());
		mines[rapidjson::SizeType(3)].AddMember("variants", rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		mines[rapidjson::SizeType(3)]["variants"].AddMember("default", rapidjson::Value(1), d.GetAllocator());

		levelObj.AddMember("The Swamp", rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		levelObj["The Swamp"].AddMember("random_generation_monsters", rapidjson::Value(rapidjson::kArrayType), d.GetAllocator());
		levelObj["The Swamp"]["random_generation_monsters"].PushBack(rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());

		auto& swamp = levelObj["The Swamp"]["random_generation_monsters"];
		swamp[rapidjson::SizeType(0)].AddMember("name", "spider", d.GetAllocator());
		swamp[rapidjson::SizeType(0)].AddMember("weighted_chance", rapidjson::Value(2), d.GetAllocator());
		swamp[rapidjson::SizeType(0)].AddMember("dungeon_depth_minimum", rapidjson::Value(0), d.GetAllocator());
		swamp[rapidjson::SizeType(0)].AddMember("dungeon_depth_maximum", rapidjson::Value(99), d.GetAllocator());
		swamp[rapidjson::SizeType(0)].AddMember("variants", rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		swamp[rapidjson::SizeType(0)]["variants"].AddMember("default", rapidjson::Value(1), d.GetAllocator());

		swamp.PushBack(rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		swamp[rapidjson::SizeType(1)].AddMember("name", "goblin", d.GetAllocator());
		swamp[rapidjson::SizeType(1)].AddMember("weighted_chance", rapidjson::Value(3), d.GetAllocator());
		swamp[rapidjson::SizeType(1)].AddMember("dungeon_depth_minimum", rapidjson::Value(0), d.GetAllocator());
		swamp[rapidjson::SizeType(1)].AddMember("dungeon_depth_maximum", rapidjson::Value(99), d.GetAllocator());
		swamp[rapidjson::SizeType(1)].AddMember("variants", rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		swamp[rapidjson::SizeType(1)]["variants"].AddMember("default", rapidjson::Value(1), d.GetAllocator());

		swamp.PushBack(rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		swamp[rapidjson::SizeType(2)].AddMember("name", "slime", d.GetAllocator());
		swamp[rapidjson::SizeType(2)].AddMember("weighted_chance", rapidjson::Value(3), d.GetAllocator());
		swamp[rapidjson::SizeType(2)].AddMember("dungeon_depth_minimum", rapidjson::Value(0), d.GetAllocator());
		swamp[rapidjson::SizeType(2)].AddMember("dungeon_depth_maximum", rapidjson::Value(99), d.GetAllocator());
		swamp[rapidjson::SizeType(2)].AddMember("variants", rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		swamp[rapidjson::SizeType(2)]["variants"].AddMember("default", rapidjson::Value(1), d.GetAllocator());

		swamp.PushBack(rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		swamp[rapidjson::SizeType(3)].AddMember("name", "ghoul", d.GetAllocator());
		swamp[rapidjson::SizeType(3)].AddMember("weighted_chance", rapidjson::Value(2), d.GetAllocator());
		swamp[rapidjson::SizeType(3)].AddMember("dungeon_depth_minimum", rapidjson::Value(0), d.GetAllocator());
		swamp[rapidjson::SizeType(3)].AddMember("dungeon_depth_maximum", rapidjson::Value(99), d.GetAllocator());
		swamp[rapidjson::SizeType(3)].AddMember("variants", rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		swamp[rapidjson::SizeType(3)]["variants"].AddMember("default", rapidjson::Value(1), d.GetAllocator());

		levelObj.AddMember("My level", rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());

		levelObj["My level"].AddMember("random_generation_monsters", rapidjson::Value(rapidjson::kArrayType), d.GetAllocator());
		levelObj["My level"]["random_generation_monsters"].PushBack(rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		auto& customLevel = levelObj["My level"]["random_generation_monsters"];
		customLevel[rapidjson::SizeType(0)].AddMember("name", "demon", d.GetAllocator());
		customLevel[rapidjson::SizeType(0)].AddMember("weighted_chance", rapidjson::Value(1), d.GetAllocator());
		customLevel[rapidjson::SizeType(0)].AddMember("dungeon_depth_minimum", rapidjson::Value(0), d.GetAllocator());
		customLevel[rapidjson::SizeType(0)].AddMember("dungeon_depth_maximum", rapidjson::Value(99), d.GetAllocator());
		customLevel[rapidjson::SizeType(0)].AddMember("variants", rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
		customLevel[rapidjson::SizeType(0)]["variants"].AddMember("default", rapidjson::Value(1), d.GetAllocator());

		CustomHelpers::addMemberToRoot(d, "levels", levelObj);

		writeToFile(d);
	}

	void writeToFile(rapidjson::Document& d)
	{
		int filenum = 0;
		std::string testPath = "/data/monstercurve_export" + std::to_string(filenum) + ".json";
		while ( PHYSFS_getRealDir(testPath.c_str()) != nullptr && filenum < 1000 )
		{
			++filenum;
			testPath = "/data/monstercurve_export" + std::to_string(filenum) + ".json";
		}
		std::string outputPath = PHYSFS_getRealDir("/data/");
		outputPath.append(PHYSFS_getDirSeparator());
		std::string fileName = "data/monstercurve_export" + std::to_string(filenum) + ".json";
		outputPath.append(fileName.c_str());

		File* fp = FileIO::open(outputPath.c_str(), "wb");
		if ( !fp )
		{
			return;
		}
		rapidjson::StringBuffer os;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
		d.Accept(writer);
		fp->write(os.GetString(), sizeof(char), os.GetSize());

		FileIO::close(fp);
	}
};
extern MonsterCurveCustomManager monsterCurveCustomManager;

class GameplayCustomManager
{
public:
	bool usingCustomManager = false;
	int xpShareRange = XPSHARERANGE;
	std::pair<std::unordered_set<int>, std::unordered_set<int>> minotaurForceEnableFloors;
	std::pair<std::unordered_set<int>, std::unordered_set<int>> minotaurForceDisableFloors;
	std::pair<std::unordered_set<int>, std::unordered_set<int>> hungerDisableFloors;
	std::pair<std::unordered_set<int>, std::unordered_set<int>> herxChatterDisableFloors;
	std::pair<std::unordered_set<int>, std::unordered_set<int>> minimapDisableFloors;
	int globalXPPercent = 100;
	int globalGoldPercent = 100;
	bool minimapShareProgress = false;
	int playerWeightPercent = 100;
	double playerSpeedMax = 18.0;
	inline bool inUse() { return usingCustomManager; };
	void resetValues()
	{
		usingCustomManager = false;
		xpShareRange = XPSHARERANGE;
		globalXPPercent = 100;
		globalGoldPercent = 100;
		minimapShareProgress = false;
		playerWeightPercent = 100;
		playerSpeedMax = 18.0;

		minotaurForceEnableFloors.first.clear();
		minotaurForceEnableFloors.second.clear();
		minotaurForceDisableFloors.first.clear();
		minotaurForceDisableFloors.second.clear();
		hungerDisableFloors.first.clear();
		hungerDisableFloors.second.clear();
		herxChatterDisableFloors.first.clear();
		herxChatterDisableFloors.second.clear();
		minimapDisableFloors.first.clear();
		minimapDisableFloors.second.clear();
		allMapGenerations.clear();
	}

	class MapGeneration
	{
	public:
		MapGeneration(std::string name) { mapName = name; };
		std::string mapName = "";
		std::vector<std::string> trapTypes;
		std::unordered_set<int> minoFloors;
		std::unordered_set<int> darkFloors;
		std::unordered_set<int> shopFloors;
		std::unordered_set<int> npcSpawnFloors;
		bool usingTrapTypes = false;
		int minoPercent = -1;
		int shopPercent = -1;
		int darkPercent = -1;
		int npcSpawnPercent = -1;
	};

	std::vector<MapGeneration> allMapGenerations;
	bool mapGenerationExistsForMapName(std::string name)
	{
		for ( auto& it : allMapGenerations )
		{
			if ( it.mapName.compare(name) == 0 )
			{
				return true;
			}
		}
		return false;
	}
	MapGeneration* getMapGenerationForMapName(std::string name)
	{
		for ( auto& it : allMapGenerations )
		{
			if ( it.mapName.compare(name) == 0 )
			{
				return &it;
			}
		}
		return nullptr;
	}

	void writeAllToDocument()
	{
		rapidjson::Document d;
		d.SetObject();

		CustomHelpers::addMemberToRoot(d, "version", rapidjson::Value(1));
		CustomHelpers::addMemberToRoot(d, "xp_share_range", rapidjson::Value(xpShareRange));
		CustomHelpers::addMemberToRoot(d, "global_xp_award_percent", rapidjson::Value(globalXPPercent));
		CustomHelpers::addMemberToRoot(d, "global_gold_drop_scale_percent", rapidjson::Value(globalGoldPercent));
		CustomHelpers::addMemberToRoot(d, "player_share_minimap_progress", rapidjson::Value(minimapShareProgress));
		CustomHelpers::addMemberToRoot(d, "player_speed_weight_impact_percent", rapidjson::Value(playerWeightPercent));
		CustomHelpers::addMemberToRoot(d, "player_speed_max", rapidjson::Value(playerSpeedMax));

		rapidjson::Value obj(rapidjson::kObjectType);
		rapidjson::Value arr(rapidjson::kArrayType);
		CustomHelpers::addMemberToRoot(d, "minotaur_force_disable_on_floors", obj);
		CustomHelpers::addMemberToSubkey(d, "minotaur_force_disable_on_floors", "normal_floors", arr);
		CustomHelpers::addMemberToSubkey(d, "minotaur_force_disable_on_floors", "secret_floors", arr);
		CustomHelpers::addMemberToRoot(d, "minotaur_force_enable_on_floors", obj);
		CustomHelpers::addMemberToSubkey(d, "minotaur_force_enable_on_floors", "normal_floors", arr);
		CustomHelpers::addMemberToSubkey(d, "minotaur_force_enable_on_floors", "secret_floors", arr);
		CustomHelpers::addMemberToRoot(d, "disable_herx_messages_on_floors", obj);
		CustomHelpers::addMemberToSubkey(d, "disable_herx_messages_on_floors", "normal_floors", arr);
		CustomHelpers::addMemberToSubkey(d, "disable_herx_messages_on_floors", "secret_floors", arr);
		CustomHelpers::addMemberToRoot(d, "disable_minimap_on_floors", obj);
		CustomHelpers::addMemberToSubkey(d, "disable_minimap_on_floors", "normal_floors", arr);
		CustomHelpers::addMemberToSubkey(d, "disable_minimap_on_floors", "secret_floors", arr);

		rapidjson::Value mapGenObj;
		mapGenObj.SetObject();
		CustomHelpers::addMemberToRoot(d, "map_generation", mapGenObj);
		rapidjson::Value key1("The Mines", d.GetAllocator());
		rapidjson::Value minesObj(rapidjson::kObjectType);

		rapidjson::Value trapArray1(rapidjson::kArrayType);
		trapArray1.PushBack("boulders", d.GetAllocator());
		minesObj.AddMember("trap_generation_types", trapArray1, d.GetAllocator());
		minesObj.AddMember("minotaur_floors", rapidjson::Value(rapidjson::kArrayType), d.GetAllocator());
		minesObj["minotaur_floors"].PushBack(2, d.GetAllocator());
		minesObj["minotaur_floors"].PushBack(3, d.GetAllocator());
		minesObj.AddMember("minotaur_floor_percent", rapidjson::Value(50), d.GetAllocator());

		minesObj.AddMember("dark_floors", rapidjson::Value(rapidjson::kArrayType), d.GetAllocator());
		minesObj["dark_floors"].PushBack(1, d.GetAllocator());
		minesObj["dark_floors"].PushBack(2, d.GetAllocator());
		minesObj["dark_floors"].PushBack(3, d.GetAllocator());
		minesObj["dark_floors"].PushBack(4, d.GetAllocator());
		minesObj.AddMember("dark_floor_percent", rapidjson::Value(25), d.GetAllocator());

		minesObj.AddMember("shop_floors", rapidjson::Value(rapidjson::kArrayType), d.GetAllocator());
		minesObj["shop_floors"].PushBack(2, d.GetAllocator());
		minesObj["shop_floors"].PushBack(3, d.GetAllocator());
		minesObj["shop_floors"].PushBack(4, d.GetAllocator());
		minesObj.AddMember("shop_floor_percent", rapidjson::Value(50), d.GetAllocator());

		minesObj.AddMember("npc_floors", rapidjson::Value(rapidjson::kArrayType), d.GetAllocator());
		minesObj["npc_floors"].PushBack(2, d.GetAllocator());
		minesObj["npc_floors"].PushBack(3, d.GetAllocator());
		minesObj["npc_floors"].PushBack(4, d.GetAllocator());
		minesObj.AddMember("npc_spawn_chance", rapidjson::Value(10), d.GetAllocator());

		d["map_generation"].AddMember(key1, minesObj, d.GetAllocator());
		
		rapidjson::Value key2("The Swamp", d.GetAllocator());
		rapidjson::Value swampObj(rapidjson::kObjectType);

		rapidjson::Value trapArray2(rapidjson::kArrayType);
		trapArray2.PushBack("boulders", d.GetAllocator());
		trapArray2.PushBack("arrows", d.GetAllocator());
		swampObj.AddMember("trap_generation_types", trapArray2, d.GetAllocator());
		swampObj.AddMember("minotaur_floors", rapidjson::Value(rapidjson::kArrayType), d.GetAllocator());
		swampObj["minotaur_floors"].PushBack(7, d.GetAllocator());
		swampObj["minotaur_floors"].PushBack(8, d.GetAllocator());
		swampObj.AddMember("minotaur_floor_percent", rapidjson::Value(50), d.GetAllocator());

		swampObj.AddMember("dark_floors", rapidjson::Value(rapidjson::kArrayType), d.GetAllocator());
		swampObj["dark_floors"].PushBack(6, d.GetAllocator());
		swampObj["dark_floors"].PushBack(7, d.GetAllocator());
		swampObj["dark_floors"].PushBack(8, d.GetAllocator());
		swampObj["dark_floors"].PushBack(9, d.GetAllocator());
		swampObj.AddMember("dark_floor_percent", rapidjson::Value(25), d.GetAllocator());

		swampObj.AddMember("shop_floors", rapidjson::Value(rapidjson::kArrayType), d.GetAllocator());
		swampObj["shop_floors"].PushBack(6, d.GetAllocator());
		swampObj["shop_floors"].PushBack(7, d.GetAllocator());
		swampObj["shop_floors"].PushBack(8, d.GetAllocator());
		swampObj["shop_floors"].PushBack(9, d.GetAllocator());
		swampObj.AddMember("shop_floor_percent", rapidjson::Value(50), d.GetAllocator());

		swampObj.AddMember("npc_floors", rapidjson::Value(rapidjson::kArrayType), d.GetAllocator());
		swampObj["npc_floors"].PushBack(6, d.GetAllocator());
		swampObj["npc_floors"].PushBack(7, d.GetAllocator());
		swampObj["npc_floors"].PushBack(8, d.GetAllocator());
		swampObj["npc_floors"].PushBack(9, d.GetAllocator());
		swampObj.AddMember("npc_spawn_chance", rapidjson::Value(10), d.GetAllocator());

		d["map_generation"].AddMember(key2, swampObj, d.GetAllocator());

		writeToFile(d);
	}

	void writeToFile(rapidjson::Document& d)
	{
		int filenum = 0;
		std::string testPath = "/data/gameplaymodifiers_export" + std::to_string(filenum) + ".json";
		while ( PHYSFS_getRealDir(testPath.c_str()) != nullptr && filenum < 1000 )
		{
			++filenum;
			testPath = "/data/gameplaymodifiers_export" + std::to_string(filenum) + ".json";
		}
		std::string outputPath = PHYSFS_getRealDir("/data/");
		outputPath.append(PHYSFS_getDirSeparator());
		std::string fileName = "data/gameplaymodifiers_export" + std::to_string(filenum) + ".json";
		outputPath.append(fileName.c_str());

		File* fp = FileIO::open(outputPath.c_str(), "wb");
		if ( !fp )
		{
			return;
		}
		rapidjson::StringBuffer os;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
		d.Accept(writer);
		fp->write(os.GetString(), sizeof(char), os.GetSize());

		FileIO::close(fp);
	}

	void readFromFile()
	{
		resetValues();
		if ( PHYSFS_getRealDir("/data/gameplaymodifiers.json") )
		{
			std::string inputPath = PHYSFS_getRealDir("/data/gameplaymodifiers.json");
			inputPath.append("/data/gameplaymodifiers.json");

			File* fp = FileIO::open(inputPath.c_str(), "rb");
			if ( !fp )
			{
				printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
				return;
			}
			char buf[65536];
			int count = fp->read(buf, sizeof(buf[0]), sizeof(buf));
			buf[count] = '\0';
			rapidjson::StringStream is(buf);
			FileIO::close(fp);

			rapidjson::Document d;
			d.ParseStream(is);
			if ( !d.HasMember("version") )
			{
				printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
				return;
			}
			int version = d["version"].GetInt();

			for ( rapidjson::Value::ConstMemberIterator prop_itr = d.MemberBegin(); prop_itr != d.MemberEnd(); ++prop_itr )
			{
				if ( readKeyToGameplayProperty(prop_itr) )
				{
					usingCustomManager = true;
				}
			}
			
			printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
		}
	}

	bool readKeyToGameplayProperty(rapidjson::Value::ConstMemberIterator& itr)
	{
		std::string name = itr->name.GetString();
		if ( name.compare("version") == 0 )
		{
			return true;
		}
		else if ( name.compare("xp_share_range") == 0 )
		{
			xpShareRange = itr->value.GetInt();
			return true;
		}
		else if ( name.compare("global_xp_award_percent") == 0 )
		{
			globalXPPercent = itr->value.GetInt();
			return true;
		}
		else if ( name.compare("global_gold_drop_scale_percent") == 0 )
		{
			globalGoldPercent = itr->value.GetInt();
			return true;
		}
		else if ( name.compare("player_share_minimap_progress") == 0 )
		{
			minimapShareProgress = itr->value.GetBool();
			return true;
		}
		else if ( name.compare("player_speed_weight_impact_percent") == 0 )
		{
			playerWeightPercent = itr->value.GetInt();
			return true;
		}
		else if ( name.compare("player_speed_max") == 0 )
		{
			playerSpeedMax = itr->value.GetDouble();
			return true;
		}
		else if ( name.compare("minotaur_force_disable_on_floors") == 0 )
		{
			for ( rapidjson::Value::ConstValueIterator arr_itr = itr->value["normal_floors"].Begin(); arr_itr != itr->value["normal_floors"].End(); ++arr_itr )
			{
				minotaurForceDisableFloors.first.insert(arr_itr->GetInt());
			}
			for ( rapidjson::Value::ConstValueIterator arr_itr = itr->value["secret_floors"].Begin(); arr_itr != itr->value["secret_floors"].End(); ++arr_itr )
			{
				minotaurForceDisableFloors.second.insert(arr_itr->GetInt());
			}
			return true;
		}
		else if ( name.compare("minotaur_force_enable_on_floors") == 0 )
		{
			for ( rapidjson::Value::ConstValueIterator arr_itr = itr->value["normal_floors"].Begin(); arr_itr != itr->value["normal_floors"].End(); ++arr_itr )
			{
				minotaurForceEnableFloors.first.insert(arr_itr->GetInt());
			}
			for ( rapidjson::Value::ConstValueIterator arr_itr = itr->value["secret_floors"].Begin(); arr_itr != itr->value["secret_floors"].End(); ++arr_itr )
			{
				minotaurForceEnableFloors.second.insert(arr_itr->GetInt());
			}
			return true;
		}
		else if ( name.compare("disable_hunger_on_floors") == 0 )
		{
			for ( rapidjson::Value::ConstValueIterator arr_itr = itr->value["normal_floors"].Begin(); arr_itr != itr->value["normal_floors"].End(); ++arr_itr )
			{
				hungerDisableFloors.first.insert(arr_itr->GetInt());
			}
			for ( rapidjson::Value::ConstValueIterator arr_itr = itr->value["secret_floors"].Begin(); arr_itr != itr->value["secret_floors"].End(); ++arr_itr )
			{
				hungerDisableFloors.second.insert(arr_itr->GetInt());
			}
			return true;
		}
		else if ( name.compare("disable_herx_messages_on_floors") == 0 )
		{
			for ( rapidjson::Value::ConstValueIterator arr_itr = itr->value["normal_floors"].Begin(); arr_itr != itr->value["normal_floors"].End(); ++arr_itr )
			{
				herxChatterDisableFloors.first.insert(arr_itr->GetInt());
			}
			for ( rapidjson::Value::ConstValueIterator arr_itr = itr->value["secret_floors"].Begin(); arr_itr != itr->value["secret_floors"].End(); ++arr_itr )
			{
				herxChatterDisableFloors.second.insert(arr_itr->GetInt());
			}
			return true;
		}
		else if ( name.compare("disable_minimap_on_floors") == 0 )
		{
			for ( rapidjson::Value::ConstValueIterator arr_itr = itr->value["normal_floors"].Begin(); arr_itr != itr->value["normal_floors"].End(); ++arr_itr )
			{
				minimapDisableFloors.first.insert(arr_itr->GetInt());
			}
			for ( rapidjson::Value::ConstValueIterator arr_itr = itr->value["secret_floors"].Begin(); arr_itr != itr->value["secret_floors"].End(); ++arr_itr )
			{
				minimapDisableFloors.second.insert(arr_itr->GetInt());
			}
			return true;
		}
		else if ( name.compare("map_generation") == 0 )
		{
			for ( rapidjson::Value::ConstMemberIterator map_itr = itr->value.MemberBegin(); map_itr != itr->value.MemberEnd(); ++map_itr )
			{
				std::string mapName = map_itr->name.GetString();
				MapGeneration m(mapName);
				for ( rapidjson::Value::ConstMemberIterator obj_itr = map_itr->value.MemberBegin(); obj_itr != map_itr->value.MemberEnd(); ++obj_itr )
				{
					readKeyToMapGenerationProperty(m, obj_itr);
				}
				allMapGenerations.push_back(m);
			}
			return true;
		}
		printlog("[JSON]: Unknown property '%s'", name.c_str());
		return false;
	}

	bool readKeyToMapGenerationProperty(MapGeneration& m, rapidjson::Value::ConstMemberIterator& itr)
	{
		std::string name = itr->name.GetString();
		if ( name.compare("trap_generation_types") == 0 )
		{
			m.usingTrapTypes = true;
			for ( rapidjson::Value::ConstValueIterator arr_itr = itr->value.Begin(); arr_itr != itr->value.End(); ++arr_itr )
			{
				m.trapTypes.push_back(arr_itr->GetString());
			}
			return true;
		}
		else if ( name.compare("minotaur_floors") == 0 )
		{
			for ( rapidjson::Value::ConstValueIterator arr_itr = itr->value.Begin(); arr_itr != itr->value.End(); ++arr_itr )
			{
				m.minoFloors.insert(arr_itr->GetInt());
			}
			return true;
		}
		else if ( name.compare("dark_floors") == 0 )
		{
			for ( rapidjson::Value::ConstValueIterator arr_itr = itr->value.Begin(); arr_itr != itr->value.End(); ++arr_itr )
			{
				m.darkFloors.insert(arr_itr->GetInt());
			}
			return true;
		}
		else if ( name.compare("shop_floors") == 0 )
		{
			for ( rapidjson::Value::ConstValueIterator arr_itr = itr->value.Begin(); arr_itr != itr->value.End(); ++arr_itr )
			{
				m.shopFloors.insert(arr_itr->GetInt());
			}
			return true;
		}
		else if ( name.compare("npc_floors") == 0 )
		{
			for ( rapidjson::Value::ConstValueIterator arr_itr = itr->value.Begin(); arr_itr != itr->value.End(); ++arr_itr )
			{
				m.npcSpawnFloors.insert(arr_itr->GetInt());
			}
			return true;
		}
		else if ( name.compare("dark_floor_percent") == 0 )
		{
			m.darkPercent = itr->value.GetInt();
			return true;
		}
		else if ( name.compare("minotaur_floor_percent") == 0 )
		{
			m.minoPercent = itr->value.GetInt();
			return true;
		}
		else if ( name.compare("shop_floor_percent") == 0 )
		{
			m.shopPercent = itr->value.GetInt();
			return true;
		}
		else if ( name.compare("npc_spawn_chance") == 0 )
		{
			m.npcSpawnPercent = itr->value.GetInt();
			return true;
		}
		printlog("[JSON]: Unknown property '%s'", name.c_str());
		return false;
	}

	bool processedMinotaurSpawn(int level, bool secret, std::string mapName)
	{
		if ( !inUse() )
		{
			return false;
		}

		if ( CustomHelpers::isLevelPartOfSet(level, secret, minotaurForceEnableFloors) )
		{
			minotaurlevel = 1;
			return true;
		}
		if ( CustomHelpers::isLevelPartOfSet(level, secret, minotaurForceDisableFloors) )
		{
			minotaurlevel = 0;
			return true;
		}

		auto m = getMapGenerationForMapName(mapName);
		if ( m )
		{
			if ( m->minoPercent == -1 )
			{
				// no key value read in.
				return false;
			}

			if ( m->minoFloors.find(level) == m->minoFloors.end() )
			{
				// not found
				minotaurlevel = 0;
				return true;
			}
			// found, roll prng
			if ( prng_get_uint() % 100 < m->minoPercent )
			{
				minotaurlevel = 1;
			}
			else
			{
				minotaurlevel = 0;
			}
			return true;
		}
		return false;
	}

	bool processedDarkFloor(int level, bool secret, std::string mapName)
	{
		if ( !inUse() )
		{
			return false;
		}

		auto m = getMapGenerationForMapName(mapName);
		if ( m )
		{
			if ( m->darkPercent == -1 )
			{
				// no key value read in.
				return false;
			}

			if ( m->darkFloors.find(level) == m->darkFloors.end() )
			{
				// not found
				darkmap = false;
				return true;
			}
			// found, roll prng
			if ( prng_get_uint() % 100 < m->darkPercent )
			{
				darkmap = true;
			}
			else
			{
				darkmap = false;
			}
			return true;
		}
		return false;
	}

	bool processedShopFloor(int level, bool secret, std::string mapName, bool& shoplevel)
	{
		if ( !inUse() )
		{
			return false;
		}

		auto m = getMapGenerationForMapName(mapName);
		if ( m )
		{
			if ( m->shopPercent == -1 )
			{
				// no key value read in.
				return false;
			}

			if ( m->shopFloors.find(level) == m->shopFloors.end() )
			{
				// not found
				shoplevel = false;
				return true;
			}
			// found, roll prng
			if ( prng_get_uint() % 100 < m->shopPercent )
			{
				shoplevel = true;
			}
			else
			{
				shoplevel = false;
			}
			return true;
		}
		return false;
	}

	enum PropertyTypes : int
	{
		PROPERTY_NPC
	};

	bool processedPropertyForFloor(int level, bool secret, std::string mapName, PropertyTypes propertyType, bool& bOut)
	{
		if ( !inUse() )
		{
			return false;
		}

		auto m = getMapGenerationForMapName(mapName);
		if ( m )
		{
			int percentValue = -1;
			switch ( propertyType )
			{
				case PROPERTY_NPC:
					if ( m->npcSpawnFloors.find(level) == m->npcSpawnFloors.end() )
					{
						// not found
						bOut = false;
						return true;
					}
					percentValue = m->npcSpawnPercent;
					break;
				default:
					break;
			}

			if ( percentValue == -1 )
			{
				// no key value read in.
				return false;
			}

			// found, roll prng
			if ( prng_get_uint() % 100 < percentValue )
			{
				bOut = true;
			}
			else
			{
				bOut = false;
			}
			return true;
		}
		return false;
	}
};
extern GameplayCustomManager gameplayCustomManager;

class GameModeManager_t
{
public:
	enum GameModes : int
	{
		GAME_MODE_DEFAULT,
		GAME_MODE_TUTORIAL_INIT,
		GAME_MODE_TUTORIAL
	};
	GameModes currentMode = GAME_MODE_DEFAULT;
	GameModes getMode() const { return currentMode; };
	void setMode(const GameModes mode) { currentMode = mode; };
	class CurrentSession_t
	{
	public:
		Uint32 serverFlags = 0;
		bool bHasSavedServerFlags = false;
		void restoreSavedServerFlags()
		{ 
			if ( bHasSavedServerFlags )
			{
				bHasSavedServerFlags = false;
				svFlags = serverFlags;
				printlog("[SESSION]: Restoring server flags at stage: %d", introstage);
			}
		}
		void saveServerFlags()
		{
			serverFlags = svFlags;
			bHasSavedServerFlags = true;
			printlog("[SESSION]: Saving server flags at stage: %d", introstage);
		}
	} currentSession;
	bool isServerflagDisabledForCurrentMode(int i)
	{
		if ( getMode() == GAME_MODE_DEFAULT )
		{
			return false;
		}
		else if ( getMode() == GAME_MODE_TUTORIAL )
		{
			int flag = power(2, i);
			switch ( flag )
			{
				case SV_FLAG_HARDCORE:
				case SV_FLAG_HUNGER:
				case SV_FLAG_FRIENDLYFIRE:
				case SV_FLAG_LIFESAVING:
				case SV_FLAG_TRAPS:
				case SV_FLAG_CLASSIC:
				case SV_FLAG_MINOTAURS:
				case SV_FLAG_KEEPINVENTORY:
					return true;
					break;
				default:
					break;
			}
			return false;
		}
		return false;
	}
	class Tutorial_t
	{
		std::string currentMap = "";
		const Uint32 kNumTutorialLevels = 10;
	public:
		void init()
		{
			readFromFile();
		}
		int dungeonLevel = -1;
		void setTutorialMap(std::string& mapname)
		{
			loadCustomNextMap = mapname;
			currentMap = loadCustomNextMap;
		}
		void launchHub()
		{
			loadCustomNextMap = "tutorial_hub.lmp";
			currentMap = loadCustomNextMap;
		}
		void startTutorial(std::string mapToSet);
		static void buttonReturnToTutorialHub(button_t* my);
		static void buttonRestartTrial(button_t* my);
		const Uint32 getNumTutorialLevels() { return kNumTutorialLevels; }
		void openGameoverWindow();
		void onMapRestart(int levelNum)
		{
			achievementObserver.updateGlobalStat(
				std::min(STEAM_GSTAT_TUTORIAL1_ATTEMPTS - 1 + levelNum, static_cast<int>(STEAM_GSTAT_TUTORIAL10_ATTEMPTS)));
		}

		class Menu_t
		{
			bool bWindowOpen = false;
		public:
			bool isOpen() { return bWindowOpen; }
			void open();
			void close() { bWindowOpen = false; }
			void onClickEntry();
			int windowScroll = 0;
			int selectedMenuItem = -1;
			std::string windowTitle = "";
			std::string defaultHoverText = "";
		} Menu;

		class FirstTimePrompt_t
		{
			bool bWindowOpen = false;
		public:
			void createPrompt();
			void drawDialogue();
			bool isOpen() { return bWindowOpen; }
			void close() { bWindowOpen = false; }
			bool doButtonSkipPrompt = false;
			bool showFirstTimePrompt = false;
			static void buttonSkipPrompt(button_t* my);
			static void buttonPromptEnterTutorialHub(button_t* my);
		} FirstTimePrompt;

		class Level_t
		{
		public:
			Level_t()
			{
				filename = "";
				title = "";
				description = "";
				completionTime = 0;
			};
			std::string filename;
			std::string title;
			std::string description;
			Uint32 completionTime;
		};
		std::vector<Level_t> levels;

		void readFromFile();
		void writeToDocument();
		void writeToFile(rapidjson::Document& d)
		{
			std::string outputPath = outputdir;
			outputPath.append(PHYSFS_getDirSeparator());
			std::string fileName = "data/tutorial_scores.json";
			outputPath.append(fileName.c_str());

			File* fp = FileIO::open(outputPath.c_str(), "wb");
			if ( !fp )
			{
				return;
			}
			rapidjson::StringBuffer os;
			rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
			d.Accept(writer);
			fp->write(os.GetString(), sizeof(char), os.GetSize());

			FileIO::close(fp);
		}
	} Tutorial;
};
extern GameModeManager_t gameModeManager;

class IRCHandler_t
{
	IPaddress ip;
	TCPsocket net_ircsocket = nullptr;
	TCPsocket net_ircsocket_from_server = nullptr;
	SDLNet_SocketSet net_ircsocketset = nullptr;
	bool bSocketConnected = false;
	const unsigned int MAX_BUFFER_LEN = 1024;
	std::vector<char> recvBuffer;
	struct Auth_t
	{
		std::string oauth = "";
		std::string chatroom = "";
		std::string username = "";
	} auth;
public:
	IRCHandler_t()
	{
		ip.host = 0;
		ip.port = 0;
		recvBuffer.resize(MAX_BUFFER_LEN);
		std::fill(recvBuffer.begin(), recvBuffer.end(), '\0');
	}
	int packetSend(std::string data);
	int packetReceive();
	void handleMessage(std::string& msg);
	void run();
	bool connect();
	void disconnect();
	bool readFromFile();
};
extern IRCHandler_t IRCHandler;