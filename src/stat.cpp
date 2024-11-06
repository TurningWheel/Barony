/*-------------------------------------------------------------------------------

	BARONY
	File: stat.cpp
	Desc: functions for the Stat struct

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/


#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "magic/magic.hpp"
#include "net.hpp"
#include "player.hpp"
#include "prng.hpp"
#include "mod_tools.hpp"

Stat* stats[MAXPLAYERS];

int Stat::getGoldWeight() const
{
	bool cursedItemIsBuff = false;
	bool isPlayer = false;
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( this == stats[i] )
		{
			isPlayer = true;
			break;
		}
	}
	if ( isPlayer )
	{
		cursedItemIsBuff = shouldInvertEquipmentBeatitude(this);
	}

	int weight = GOLD / 100;
	if ( mask && mask->type == MASK_GOLDEN )
	{
		real_t equipmentBonus = 100.0;
		if ( mask->beatitude >= 0 || cursedItemIsBuff )
		{
			equipmentBonus -= 50.0 * (1 + abs(mask->beatitude));
			equipmentBonus = std::max(-50.0, equipmentBonus);
		}
		else
		{
			equipmentBonus -= 50.0 * (abs(mask->beatitude));
			equipmentBonus = std::max(0.0, equipmentBonus);
		}

		weight *= (equipmentBonus / 100.0);
	}

	return weight;
}

int Stat::maxEquipmentBonusToSkill = 25;
Sint32 Stat::getModifiedProficiency(int skill) const
{
	if ( !(skill >= 0 && skill < NUMPROFICIENCIES) )
	{
		return 0;
	}

	Sint32 base = std::min(100, std::max(0, PROFICIENCIES[skill]));
	Sint32 equipmentBonus = 0;

	bool cursedItemIsBuff = false;
	bool isPlayer = false;
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( this == stats[i] )
		{
			isPlayer = true;
			break;
		}
	}
	if ( isPlayer )
	{
		cursedItemIsBuff = shouldInvertEquipmentBeatitude(this);
	}

	if ( mask )
	{
		if ( mask->type == MASK_HAZARD_GOGGLES && skill == PRO_ALCHEMY )
		{
			if ( mask->beatitude >= 0 || cursedItemIsBuff )
			{
				equipmentBonus += std::min(maxEquipmentBonusToSkill, (1 + abs(mask->beatitude)) * 10);
			}
			else
			{
				equipmentBonus -= abs(mask->beatitude) * 10;
			}
		}
		else if ( mask->type == MASK_GOLDEN && skill == PRO_TRADING )
		{
			if ( mask->beatitude >= 0 || cursedItemIsBuff )
			{
				equipmentBonus += std::min(maxEquipmentBonusToSkill, (1 + abs(mask->beatitude)) * 10);
			}
			else
			{
				equipmentBonus -= 999;
			}
		}
		else if ( (mask->type == MASK_STEEL_VISOR || mask->type == MASK_CRYSTAL_VISOR)
			&& (skill == PRO_SWORD || skill == PRO_AXE || skill == PRO_POLEARM || skill == PRO_MACE) )
		{
			if ( mask->beatitude >= 0 || cursedItemIsBuff )
			{
				equipmentBonus += std::min(maxEquipmentBonusToSkill, (1 + abs(mask->beatitude)) * 5);
			}
			else
			{
				equipmentBonus -= abs(mask->beatitude) * 5;
			}
		}
		else if ( (mask->type == MASK_ARTIFACT_VISOR)
			&& (skill == PRO_SWORD || skill == PRO_AXE || skill == PRO_POLEARM || skill == PRO_MACE) )
		{
			if ( mask->beatitude >= 0 || cursedItemIsBuff )
			{
				equipmentBonus += std::min(maxEquipmentBonusToSkill, (1 + abs(mask->beatitude)) * 10);
			}
			else
			{
				equipmentBonus -= abs(mask->beatitude) * 10;
			}
		}
	}
	if ( helmet )
	{
		if ( helmet->type == HAT_PLUMED_CAP && skill == PRO_LEADERSHIP )
		{
			if ( helmet->beatitude >= 0 || cursedItemIsBuff )
			{
				equipmentBonus += std::min(maxEquipmentBonusToSkill, (1 + abs(helmet->beatitude)) * 10);
			}
			else
			{
				equipmentBonus -= abs(helmet->beatitude) * 10;
			}
		}
		else if ( helmet->type == HAT_BOUNTYHUNTER && skill == PRO_RANGED )
		{
			if ( helmet->beatitude >= 0 || cursedItemIsBuff )
			{
				equipmentBonus += std::min(maxEquipmentBonusToSkill, (1 + abs(helmet->beatitude)) * 10);
			}
			else
			{
				equipmentBonus -= abs(helmet->beatitude) * 10;
			}
		}
		else if ( helmet->type == HAT_HOOD_WHISPERS && skill == PRO_STEALTH )
		{
			if ( helmet->beatitude >= 0 || cursedItemIsBuff )
			{
				equipmentBonus += std::min(maxEquipmentBonusToSkill, (1 + abs(helmet->beatitude)) * 10);
			}
			else
			{
				equipmentBonus -= abs(helmet->beatitude) * 10;
			}
		}
		else if ( (helmet->type == HAT_CIRCLET || helmet->type == HAT_CIRCLET_WISDOM) && skill == PRO_SPELLCASTING )
		{
			if ( helmet->beatitude >= 0 || cursedItemIsBuff )
			{
				equipmentBonus += std::min(maxEquipmentBonusToSkill, (1 + abs(helmet->beatitude)) * 10);
			}
			else
			{
				equipmentBonus -= abs(helmet->beatitude) * 10;
			}
		}
	}

	return std::min(100, std::max(0, base + equipmentBonus));
}

//Destructor
Stat::~Stat()
{
	if (this->helmet != NULL)
	{
		if (this->helmet->node == NULL)
		{
			free(this->helmet);
		}
		else
		{
			list_RemoveNode(this->helmet->node);
		}
		this->helmet = NULL;
	}
	if (this->breastplate != NULL)
	{
		if (this->breastplate->node == NULL)
		{
			free(this->breastplate);
		}
		else
		{
			list_RemoveNode(this->breastplate->node);
		}
		this->breastplate = NULL;
	}
	if (this->gloves != NULL)
	{
		if (this->gloves->node == NULL)
		{
			free(this->gloves);
		}
		else
		{
			list_RemoveNode(this->gloves->node);
		}
		this->gloves = NULL;
	}
	if (this->shoes != NULL)
	{
		if (this->shoes->node == NULL)
		{
			free(this->shoes);
		}
		else
		{
			list_RemoveNode(this->shoes->node);
		}
		this->shoes = NULL;
	}
	if (this->shield != NULL)
	{
		if (this->shield->node == NULL)
		{
			free(this->shield);
		}
		else
		{
			list_RemoveNode(this->shield->node);
		}
		this->shield = NULL;
	}
	if (this->weapon != NULL)
	{
		if (this->weapon->node == NULL)
		{
			free(this->weapon);
		}
		else
		{
			list_RemoveNode(this->weapon->node);
		}
		this->weapon = NULL;
	}
	if (this->cloak != NULL)
	{
		if (this->cloak->node == NULL)
		{
			free(this->cloak);
		}
		else
		{
			list_RemoveNode(this->cloak->node);
		}
		this->cloak = NULL;
	}
	if (this->amulet != NULL)
	{
		if (this->amulet->node == NULL)
		{
			free(this->amulet);
		}
		else
		{
			list_RemoveNode(this->amulet->node);
		}
		this->amulet = NULL;
	}
	if (this->ring != NULL)
	{
		if (this->ring->node == NULL)
		{
			free(this->ring);
		}
		else
		{
			list_RemoveNode(this->ring->node);
		}
		this->ring = NULL;
	}
	if (this->mask != NULL)
	{
		if (this->mask->node == NULL)
		{
			free(this->mask);
		}
		else
		{
			list_RemoveNode(this->mask->node);
		}
		this->mask = NULL;
	}
	//Free memory for magic effects.
	node_t* spellnode;
	spellnode = this->magic_effects.first;
	while (spellnode)
	{
		node_t* oldnode = spellnode;
		spellnode = spellnode->next;
		spell_t* spell = (spell_t*)oldnode->element;
		spell->magic_effects_node = NULL;
	}
	list_FreeAll(&this->magic_effects);
	list_FreeAll(&this->inventory);
}

void Stat::clearStats()
{
	int x;

	strcpy(this->obituary, Language::get(1500));
	this->killer = KilledBy::UNKNOWN;
	this->killer_uid = 0;
	this->killer_monster = NOTHING;
	this->killer_item = WOODEN_SHIELD;
	this->killer_name = "";
	this->poisonKiller = 0;
	this->HP = DEFAULT_HP;
	this->MAXHP = DEFAULT_HP;
	this->OLDHP = this->HP;
	this->MP = DEFAULT_MP;
	this->MAXMP = DEFAULT_MP;
	this->STR = 0;
	this->DEX = 0;
	this->CON = 0;
	this->INT = 0;
	this->PER = 0;
	this->CHR = 0;
	this->GOLD = 0;
	this->HUNGER = 1000;
	this->LVL = 1;
	this->EXP = 0;
	list_FreeAll(&this->FOLLOWERS);
	for (x = 0; x < std::max(NUMPROFICIENCIES, NUMEFFECTS); x++)
	{
		if (x < NUMPROFICIENCIES)
		{
			this->PROFICIENCIES[x] = 0;
		}
		if (x < NUMEFFECTS)
		{
			this->EFFECTS[x] = false;
			this->EFFECTS_TIMERS[x] = 0;
		}
	}

	for ( x = 0; x < ITEM_SLOT_NUM; x = x + ITEM_SLOT_NUMPROPERTIES )
	{
		this->EDITOR_ITEMS[x] = 0;
		this->EDITOR_ITEMS[x + 1] = 0;
		this->EDITOR_ITEMS[x + 2] = 10;
		this->EDITOR_ITEMS[x + 3] = 1;
		this->EDITOR_ITEMS[x + 4] = 1;
		this->EDITOR_ITEMS[x + 5] = 1;
		this->EDITOR_ITEMS[x + 6] = 0;
	}

	for ( x = 0; x < 32; x++ )
	{
		if ( x != 4 ) // MISC_FLAGS[4] is playerRace, don't reset, same as ->sex
		{
			this->MISC_FLAGS[x] = 0;
		}
	}

	for ( x = 0; x < NUMSTATS; x++ )
	{
		this->PLAYER_LVL_STAT_BONUS[x] = -1;
	}

	for ( x = 0; x < NUMSTATS * 2; x++ )
	{
		this->PLAYER_LVL_STAT_TIMER[x] = -1;
	}

	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( stats[i] == this )
		{
			players[i]->hud.resetBars();
			players[i]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_CLOSE_ALL);
			break;
		}
	}
	this->attributes.clear();
	this->player_lootbags.clear();

	freePlayerEquipment();
	list_FreeAll(&this->inventory);
}

/*-------------------------------------------------------------------------------

freePlayerEquipment

frees all the malloc'd data for the given player's equipment

-------------------------------------------------------------------------------*/

void Stat::freePlayerEquipment()
{
	if (this->helmet != NULL)
	{
		if (this->helmet->node)
		{
			list_RemoveNode(this->helmet->node);
		}
		else
		{
			free(this->helmet);
		}
		this->helmet = NULL;
	}
	if (this->breastplate != NULL)
	{
		if (this->breastplate->node)
		{
			list_RemoveNode(this->breastplate->node);
		}
		else
		{
			free(this->breastplate);
		}
		this->breastplate = NULL;
	}
	if (this->gloves != NULL)
	{
		if (this->gloves->node)
		{
			list_RemoveNode(this->gloves->node);
		}
		else
		{
			free(this->gloves);
		}
		this->gloves = NULL;
	}
	if (this->shoes != NULL)
	{
		if (this->shoes->node)
		{
			list_RemoveNode(this->shoes->node);
		}
		else
		{
			free(this->shoes);
		}
		this->shoes = NULL;
	}
	if (this->shield != NULL)
	{
		if (this->shield->node)
		{
			list_RemoveNode(this->shield->node);
		}
		else
		{
			free(this->shield);
		}
		this->shield = NULL;
	}
	if (this->weapon != NULL)
	{
		if (this->weapon->node)
		{
			list_RemoveNode(this->weapon->node);
		}
		else
		{
			free(this->weapon);
		}
		this->weapon = NULL;
	}
	if (this->cloak != NULL)
	{
		if (this->cloak->node)
		{
			list_RemoveNode(this->cloak->node);
		}
		else
		{
			free(this->cloak);
		}
		this->cloak = NULL;
	}
	if (this->amulet != NULL)
	{
		if (this->amulet->node)
		{
			list_RemoveNode(this->amulet->node);
		}
		else
		{
			free(this->amulet);
		}
		this->amulet = NULL;
	}
	if (this->ring != NULL)
	{
		if (this->ring->node)
		{
			list_RemoveNode(this->ring->node);
		}
		else
		{
			free(this->ring);
		}
		this->ring = NULL;
	}
	if (this->mask != NULL)
	{
		if (this->mask->node)
		{
			list_RemoveNode(this->mask->node);
		}
		else
		{
			free(this->mask);
		}
		this->mask = NULL;
	}
}


/*-------------------------------------------------------------------------------

copyStats

Returns a pointer to a new instance of the Stats class

-------------------------------------------------------------------------------*/

Stat* Stat::copyStats()
{
	node_t* node;
	int c;

	// create new stat, using the type (HUMAN, SKELETON) as a reference.
	// this is handled in stat_shared.cpp by adding 1000 to the type.
	Stat* newStat = new Stat(this->type + 1000);

	newStat->type = this->type;
	newStat->sex = this->sex;
	newStat->stat_appearance = this->stat_appearance;
	strcpy(newStat->name, this->name);
	strcpy(newStat->obituary, this->obituary);

	newStat->HP = this->HP;
	newStat->MAXHP = this->MAXHP;
	newStat->OLDHP = this->OLDHP;

	newStat->MP = this->MP;
	newStat->MAXMP = this->MAXMP;
	newStat->STR = this->STR;
	newStat->DEX = this->DEX;
	newStat->CON = this->CON;
	newStat->INT = this->INT;
	newStat->PER = this->PER;
	newStat->CHR = this->CHR;
	newStat->EXP = this->EXP;
	newStat->LVL = this->LVL;
	newStat->GOLD = this->GOLD;
	newStat->HUNGER = this->HUNGER;

	for (c = 0; c < NUMPROFICIENCIES; c++)
	{
		newStat->PROFICIENCIES[c] = this->PROFICIENCIES[c];
	}
	for (c = 0; c < NUMEFFECTS; c++)
	{
		newStat->EFFECTS[c] = this->EFFECTS[c];
		newStat->EFFECTS_TIMERS[c] = this->EFFECTS_TIMERS[c];
	}

	for ( c = 0; c < ITEM_SLOT_NUM; c++ )
	{
		newStat->EDITOR_ITEMS[c] = this->EDITOR_ITEMS[c];
	}

	for ( c = 0; c < 32; c++ )
	{
		newStat->MISC_FLAGS[c] = this->MISC_FLAGS[c];
	}

	for ( c = 0; c < NUMSTATS; c++ )
	{
		newStat->PLAYER_LVL_STAT_BONUS[c] = this->PLAYER_LVL_STAT_BONUS[c];
	}

	for ( c = 0; c < NUMSTATS * 2; c++ )
	{
		newStat->PLAYER_LVL_STAT_TIMER[c] = this->PLAYER_LVL_STAT_TIMER[c];
	}

	newStat->defending = this->defending;
	newStat->leader_uid = this->leader_uid;
	newStat->FOLLOWERS.first = NULL;
	newStat->FOLLOWERS.last = NULL;
	list_Copy(&newStat->FOLLOWERS, &this->FOLLOWERS);

	newStat->inventory.first = NULL;
	newStat->inventory.last = NULL;
	list_Copy(&newStat->inventory, &this->inventory);
	for (node = newStat->inventory.first; node != NULL; node = node->next)
	{
		Item* item = (Item*)node->element;
		item->node = node;
	}

	if (this->helmet)
	{
		if (this->helmet->node)
		{
			node_t* node = list_Node(&newStat->inventory, list_Index(this->helmet->node));
			newStat->helmet = (Item*)node->element;
		}
		else
		{
			newStat->helmet = (Item*)malloc(sizeof(Item));
			memcpy(newStat->helmet, this->helmet, sizeof(Item));
		}
	}
	else
	{
		newStat->helmet = NULL;
	}
	if (this->breastplate)
	{
		if (this->breastplate->node)
		{
			node_t* node = list_Node(&newStat->inventory, list_Index(this->breastplate->node));
			newStat->breastplate = (Item*)node->element;
		}
		else
		{
			newStat->breastplate = (Item*)malloc(sizeof(Item));
			memcpy(newStat->breastplate, this->breastplate, sizeof(Item));
		}
	}
	else
	{
		newStat->breastplate = NULL;
	}
	if (this->gloves)
	{
		if (this->gloves->node)
		{
			node_t* node = list_Node(&newStat->inventory, list_Index(this->gloves->node));
			newStat->gloves = (Item*)node->element;
		}
		else
		{
			newStat->gloves = (Item*)malloc(sizeof(Item));
			memcpy(newStat->gloves, this->gloves, sizeof(Item));
		}
	}
	else
	{
		newStat->gloves = NULL;
	}
	if (this->shoes)
	{
		if (this->shoes->node)
		{
			node_t* node = list_Node(&newStat->inventory, list_Index(this->shoes->node));
			newStat->shoes = (Item*)node->element;
		}
		else
		{
			newStat->shoes = (Item*)malloc(sizeof(Item));
			memcpy(newStat->shoes, this->shoes, sizeof(Item));
		}
	}
	else
	{
		newStat->shoes = NULL;
	}
	if (this->shield)
	{
		if (this->shield->node)
		{
			node_t* node = list_Node(&newStat->inventory, list_Index(this->shield->node));
			newStat->shield = (Item*)node->element;
		}
		else
		{
			newStat->shield = (Item*)malloc(sizeof(Item));
			memcpy(newStat->shield, this->shield, sizeof(Item));
		}
	}
	else
	{
		newStat->shield = NULL;
	}
	if (this->weapon)
	{
		if (this->weapon->node)
		{
			node_t* node = list_Node(&newStat->inventory, list_Index(this->weapon->node));
			newStat->weapon = (Item*)node->element;
		}
		else
		{
			newStat->weapon = (Item*)malloc(sizeof(Item));
			memcpy(newStat->weapon, this->weapon, sizeof(Item));
		}
	}
	else
	{
		newStat->weapon = NULL;
	}
	if (this->cloak)
	{
		if (this->cloak->node)
		{
			node_t* node = list_Node(&newStat->inventory, list_Index(this->cloak->node));
			newStat->cloak = (Item*)node->element;
		}
		else
		{
			newStat->cloak = (Item*)malloc(sizeof(Item));
			memcpy(newStat->cloak, this->cloak, sizeof(Item));
		}
	}
	else
	{
		newStat->cloak = NULL;
	}
	if (this->amulet)
	{
		if (this->amulet->node)
		{
			node_t* node = list_Node(&newStat->inventory, list_Index(this->amulet->node));
			newStat->amulet = (Item*)node->element;
		}
		else
		{
			newStat->amulet = (Item*)malloc(sizeof(Item));
			memcpy(newStat->amulet, this->amulet, sizeof(Item));
		}
	}
	else
	{
		newStat->amulet = NULL;
	}
	if (this->ring)
	{
		if (this->ring->node)
		{
			node_t* node = list_Node(&newStat->inventory, list_Index(this->ring->node));
			newStat->ring = (Item*)node->element;
		}
		else
		{
			newStat->ring = (Item*)malloc(sizeof(Item));
			memcpy(newStat->ring, this->ring, sizeof(Item));
		}
	}
	else
	{
		newStat->ring = NULL;
	}
	if (this->mask)
	{
		if (this->mask->node)
		{
			node_t* node = list_Node(&newStat->inventory, list_Index(this->mask->node));
			newStat->mask = (Item*)node->element;
		}
		else
		{
			newStat->mask = (Item*)malloc(sizeof(Item));
			memcpy(newStat->mask, this->mask, sizeof(Item));
		}
	}
	else
	{
		newStat->mask = NULL;
	}

#if defined(USE_FMOD) || defined(USE_OPENAL)
	newStat->monster_sound = nullptr;
#endif
	newStat->monster_idlevar = this->monster_idlevar;
	newStat->magic_effects.first = NULL;
	newStat->magic_effects.last = NULL;
	newStat->attributes = this->attributes;
	newStat->player_lootbags = this->player_lootbags;
	return newStat;
}

void Stat::printStats()
{
	printlog("type = %d\n", this->type);
	printlog("sex = %d\n", this->sex);
	printlog("appearance = %d\n", this->stat_appearance);
	printlog("name = \"%s\"\n", this->name);
	printlog("HP = %d\n", this->HP);
	printlog("MAXHP = %d\n", this->MAXHP);
	printlog("MP = %d\n", this->MP);
	printlog("MAXMP = %d\n", this->MAXMP);
	printlog("STR = %d\n", this->STR);
	printlog("DEX = %d\n", this->DEX);
	printlog("CON = %d\n", this->CON);
	printlog("INT = %d\n", this->INT);
	printlog("PER = %d\n", this->PER);
	printlog("CHR = %d\n", this->CHR);
	printlog("EXP = %d\n", this->EXP);
	printlog("LVL = %d\n", this->LVL);
	printlog("GOLD = %d\n", this->GOLD);
	printlog("HUNGER = %d\n", this->HUNGER);

	printlog("Proficiencies:");
	for (int i = 0; i < NUMPROFICIENCIES; ++i)
	{
		printlog("[%d] = %d%s", i, this->PROFICIENCIES[i], ((i == NUMPROFICIENCIES - 1) ? "\n" : ", "));
	}

	printlog("Effects & timers: ");
	for (int i = 0; i < NUMEFFECTS; ++i)
	{
		printlog("[%d] = %s. timer[%d] = %d", i, (this->EFFECTS[i]) ? "true" : "false", i, this->EFFECTS_TIMERS[i]);
	}
}

int Stat::pickRandomEquippedItemToDegradeOnHit(Item** returnItem, bool excludeWeapon, bool excludeShield, bool excludeArmor, bool excludeJewelry)
{
	if ( EFFECTS[EFF_SHAPESHIFT] )
	{
		returnItem = nullptr;
		return -1;
	}
	if ( shield && (itemTypeIsQuiver(shield->type)
		|| itemCategory(shield) == SPELLBOOK
		|| shield->type == TOOL_TINKERING_KIT) )
	{
		excludeShield = true;
	}
	Item* maskItem = mask; 
	if ( mask && !Item::doesItemProvideBeatitudeAC(mask->type) )
	{
		// exclude mask
		mask = nullptr;
	}
	int result = pickRandomEquippedItem(returnItem, excludeWeapon, excludeShield, excludeArmor, excludeJewelry);
	mask = maskItem;
	return result;
}

int Stat::pickRandomEquippedItem(Item** returnItem, bool excludeWeapon, bool excludeShield, bool excludeArmor, bool excludeJewelry)
{
	int numEquippedItems = 0;
	int equipNum[10] = { 0 };// index of equipment piece to update the client, defined in net.cpp "ARMR"
	for ( int i = 0; i < 10; ++i )
	{
		equipNum[i] = -1;
	}

	if ( !excludeArmor )
	{
		if ( this->helmet != nullptr && this->helmet->status > BROKEN )
		{
			equipNum[numEquippedItems] = 0;
			++numEquippedItems;
		}
		if ( this->breastplate != nullptr && this->breastplate->status > BROKEN )
		{
			equipNum[numEquippedItems] = 1;
			++numEquippedItems;
		}
		if ( this->gloves != nullptr && this->gloves->status > BROKEN )
		{
			equipNum[numEquippedItems] = 2;
			++numEquippedItems;
		}
		if ( this->shoes != nullptr && this->shoes->status > BROKEN )
		{
			equipNum[numEquippedItems] = 3;
			++numEquippedItems;
		}
		if ( this->cloak != nullptr && this->cloak->status > BROKEN )
		{
			equipNum[numEquippedItems] = 6;
			++numEquippedItems;
		}
		if ( this->mask != nullptr && this->mask->status > BROKEN )
		{
			equipNum[numEquippedItems] = 9;
			++numEquippedItems;
		}
	}

	if ( !excludeWeapon )
	{
		if ( this->weapon != nullptr && this->weapon->status > BROKEN )
		{
			equipNum[numEquippedItems] = 5;
			++numEquippedItems;
		}
	}

	if ( !excludeShield )
	{
		if ( this->shield != nullptr && this->shield->status > BROKEN )
		{
			equipNum[numEquippedItems] = 4;
			++numEquippedItems;
		}
	}

	if ( !excludeJewelry )
	{
		if ( this->amulet != nullptr  && this->amulet->status > BROKEN )
		{
			equipNum[numEquippedItems] = 7;
			++numEquippedItems;
		}
		if ( this->ring != nullptr && this->ring->status > BROKEN )
		{
			equipNum[numEquippedItems] = 8;
			++numEquippedItems;
		}
	}

	if ( numEquippedItems == 0 )
	{
		*returnItem = nullptr;
		return -1;
	}

	int roll = local_rng.rand() % numEquippedItems;

	switch ( equipNum[roll] )
	{
		case 0:
			*returnItem = this->helmet;
			break;
		case 1:
			*returnItem = this->breastplate;
			break;
		case 2:
			*returnItem = this->gloves;
			break;
		case 3:
			*returnItem = this->shoes;
			break;
		case 4:
			*returnItem = this->shield;
			break;
		case 5:
			*returnItem = this->weapon;
			break;
		case 6:
			*returnItem = this->cloak;
			break;
		case 7:
			*returnItem = this->amulet;
			break;
		case 8:
			*returnItem = this->ring;
			break;
		case 9:
			*returnItem = this->mask;
			break;
		default:
			*returnItem = nullptr;
			break;
	}

	return equipNum[roll];
}

const char* getSkillLangEntry(int skill)
{
	int langEntry = 236 + skill;
	if ( skill == PRO_UNARMED )
	{
		langEntry = 3204;
	}
	else if ( skill == PRO_ALCHEMY )
	{
		langEntry = 3340;
	}
	return Language::get(langEntry);
}

void Stat::copyNPCStatsAndInventoryFrom(Stat& src)
{
	int player = -1;
	if ( multiplayer == CLIENT )
	{
		return;
	}
	for ( int c = 0; c < MAXPLAYERS; ++c )
	{
		if ( stats[c] == this )
		{
			player = c;
			break;
		}
	}

	//this->type = src.type;

	this->HP = src.HP;
	this->MAXHP = src.MAXHP;
	this->OLDHP = src.HP;
	this->MP = src.MP;
	this->MAXMP = src.MAXMP;

	this->STR = src.STR;
	this->DEX = src.DEX;
	this->CON = src.CON;
	this->INT = src.INT;
	this->PER = src.PER;
	this->CHR = src.CHR;
	this->EXP = src.EXP;
	this->LVL = src.LVL;

	this->GOLD = src.GOLD;
	bool oldIntro = intro;
	if ( player >= 0 && players[player]->isLocalPlayer() )
	{
		intro = true;
	}
	if ( src.helmet )
	{
		if ( player >= 0 )
		{
			Item* item = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, true, nullptr);
			copyItem(item, src.helmet);
			item->identified = true;
			if ( players[player]->isLocalPlayer() )
			{
				Item* pickedUp = itemPickup(player, item);
				useItem(pickedUp, player);
				free(item);
			}
			else
			{
				serverSendItemToPickupAndEquip(player, item);
				useItem(item, player);
			}
		}
		else
		{
			if ( !this->helmet )
			{
				this->helmet = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, true, nullptr);
			}
			copyItem(this->helmet, src.helmet);
		}
	}
	else
	{
		this->helmet = NULL;
	}
	if ( src.breastplate )
	{
		if ( player >= 0 )
		{
			Item* item = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, true, nullptr);
			copyItem(item, src.breastplate);
			item->identified = true;
			if ( players[player]->isLocalPlayer() )
			{
				Item* pickedUp = itemPickup(player, item);
				useItem(pickedUp, player);
				free(item);
			}
			else
			{
				serverSendItemToPickupAndEquip(player, item);
				useItem(item, player);
			}
		}
		else
		{
			if ( !this->breastplate )
			{
				this->breastplate = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, true, nullptr);
			}
			copyItem(this->breastplate, src.breastplate);
		}
	}
	else
	{
		this->breastplate = NULL;
	}
	if ( src.gloves )
	{
		if ( player >= 0 )
		{
			Item* item = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, true, nullptr);
			copyItem(item, src.gloves);
			item->identified = true;
			if ( players[player]->isLocalPlayer() )
			{
				Item* pickedUp = itemPickup(player, item);
				useItem(pickedUp, player);
				free(item);
			}
			else
			{
				serverSendItemToPickupAndEquip(player, item);
				useItem(item, player);
			}
		}
		else
		{
			if ( !this->gloves )
			{
				this->gloves = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, true, nullptr);
			}
			copyItem(this->gloves, src.gloves);
		}
	}
	else
	{
		this->gloves = NULL;
	}
	if ( src.shoes )
	{
		if ( player >= 0 )
		{
			Item* item = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, true, nullptr);
			copyItem(item, src.shoes);
			item->identified = true;
			if ( players[player]->isLocalPlayer() )
			{
				Item* pickedUp = itemPickup(player, item);
				useItem(pickedUp, player);
				free(item);
			}
			else
			{
				serverSendItemToPickupAndEquip(player, item);
				useItem(item, player);
			}
		}
		else
		{
			if ( !this->shoes )
			{
				this->shoes = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, true, nullptr);
			}
			copyItem(this->shoes, src.shoes);
		}
	}
	else
	{
		this->shoes = NULL;
	}
	if ( src.shield )
	{
		if ( player >= 0 )
		{
			Item* item = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, true, nullptr);
			copyItem(item, src.shield);
			item->identified = true;
			if ( players[player]->isLocalPlayer() )
			{
				Item* pickedUp = itemPickup(player, item);
				useItem(pickedUp, player);
				free(item);
			}
			else
			{
				serverSendItemToPickupAndEquip(player, item);
				useItem(item, player);
			}
		}
		else
		{
			if ( !this->shield )
			{
				this->shield = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, true, nullptr);
			}
			copyItem(this->shield, src.shield);
		}
	}
	else
	{
		this->shield = NULL;
	}
	if ( src.weapon )
	{
		if ( player >= 0 )
		{
			Item* item = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, true, nullptr);
			copyItem(item, src.weapon);
			item->identified = true;
			if ( players[player]->isLocalPlayer() )
			{
				Item* pickedUp = itemPickup(player, item);
				useItem(pickedUp, player);
				free(item);
			}
			else
			{
				serverSendItemToPickupAndEquip(player, item);
				useItem(item, player);
			}
		}
		else
		{
			if ( !this->weapon )
			{
				this->weapon = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, true, nullptr);
			}
			copyItem(this->weapon, src.weapon);
		}
	}
	else
	{
		this->weapon = NULL;
	}
	if ( src.cloak )
	{
		if ( player >= 0 )
		{
			Item* item = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, true, nullptr);
			copyItem(item, src.cloak);
			item->identified = true;
			if ( players[player]->isLocalPlayer() )
			{
				Item* pickedUp = itemPickup(player, item);
				useItem(pickedUp, player);
				free(item);
			}
			else
			{
				serverSendItemToPickupAndEquip(player, item);
				useItem(item, player);
			}
		}
		else
		{
			if ( !this->cloak )
			{
				this->cloak = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, true, nullptr);
			}
			copyItem(this->cloak, src.cloak);
		}
	}
	else
	{
		this->cloak = NULL;
	}
	if ( src.amulet )
	{
		if ( player >= 0 )
		{
			Item* item = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, true, nullptr);
			copyItem(item, src.amulet);
			item->identified = true;
			if ( players[player]->isLocalPlayer() )
			{
				Item* pickedUp = itemPickup(player, item);
				useItem(pickedUp, player);
				free(item);
			}
			else
			{
				serverSendItemToPickupAndEquip(player, item);
				useItem(item, player);
			}
		}
		else
		{
			if ( !this->amulet )
			{
				this->amulet = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, true, nullptr);
			}
			copyItem(this->amulet, src.amulet);
		}
	}
	else
	{
		this->amulet = NULL;
	}
	if ( src.ring )
	{
		if ( player >= 0 )
		{
			Item* item = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, true, nullptr);
			copyItem(item, src.ring);
			item->identified = true;
			if ( players[player]->isLocalPlayer() )
			{
				Item* pickedUp = itemPickup(player, item);
				useItem(pickedUp, player);
				free(item);
			}
			else
			{
				serverSendItemToPickupAndEquip(player, item);
				useItem(item, player);
			}
		}
		else
		{
			if ( !this->ring )
			{
				this->ring = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, true, nullptr);
			}
			copyItem(this->ring, src.ring);
		}
	}
	else
	{
		this->ring = NULL;
	}
	if ( src.mask )
	{
		if ( player >= 0 )
		{
			Item* item = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, true, nullptr);
			copyItem(item, src.mask);
			item->identified = true;
			if ( players[player]->isLocalPlayer() )
			{
				Item* pickedUp = itemPickup(player, item);
				useItem(pickedUp, player);
				free(item);
			}
			else
			{
				serverSendItemToPickupAndEquip(player, item);
				useItem(item, player);
			}
		}
		else
		{
			if ( !this->mask )
			{
				this->mask = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, true, nullptr);
			}
			copyItem(this->mask, src.mask);
		}
	}
	else
	{
		this->mask = NULL;
	}

	for ( node_t* node = src.inventory.first; node; node = node->next )
	{
		Item* invItem = (Item*)node->element;
		if ( invItem )
		{
			if ( player >= 0 )
			{
				Item* item = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, true, nullptr);
				copyItem(item, invItem);
				item->identified = true;
				Item* pickedUp = itemPickup(player, item);
				if ( pickedUp )
				{
					if ( players[player]->isLocalPlayer() )
					{
						free(item);
					}
					else
					{
						free(pickedUp);
					}
				}
			}
			else
			{
				Item* item = newItem(GEM_ROCK, EXCELLENT, 0, 1, 0, true, &inventory);
				copyItem(item, invItem);
			}
		}
	}
	intro = oldIntro;
}

int Stat::getActiveShieldBonus(bool checkShield, bool excludeSkill, Item* shieldItem, bool checkNonShieldBonus) const
{
	Item* item = shieldItem;
	if ( !item )
	{
		if ( !checkShield )
		{
			if ( checkNonShieldBonus )
			{
				return (5 + (excludeSkill ? 0 : std::min(SKILL_LEVEL_SKILLED, getModifiedProficiency(PRO_SHIELD)) / 5));
			}
			else
			{
				return (5 + (excludeSkill ? 0 : (getModifiedProficiency(PRO_SHIELD) / 5)));
			}
		}
		item = shield;
	}
	if ( item )
	{
		if ( itemCategory(item) == SPELLBOOK || itemTypeIsQuiver(item->type) )
		{
			return 0;
		}
		if ( itemCategory(item) != ARMOR )
		{
			// non-armor caps out at 40 blocking
			return (5 + (excludeSkill ? 0 : std::min(SKILL_LEVEL_SKILLED, getModifiedProficiency(PRO_SHIELD)) / 5));
		}
		return (5 + (excludeSkill ? 0 : (getModifiedProficiency(PRO_SHIELD) / 5)));
	}
	else
	{
		return 0;
	}
}

int Stat::getPassiveShieldBonus(bool checkShield, bool excludeSkill) const
{
	if ( !checkShield )
	{
		if ( excludeSkill )
		{
			return 0;
		}
		return (getModifiedProficiency(PRO_SHIELD) / 25);
	}

	if ( shield )
	{
		if ( itemCategory(shield) == SPELLBOOK || itemTypeIsQuiver(shield->type) 
			|| itemCategory(shield) == TOOL )
		{
			return 0;
		}
		if ( excludeSkill )
		{
			return 0;
		}
		return (getModifiedProficiency(PRO_SHIELD) / 25);
	}
	else
	{
		return 0;
	}
}

bool Stat::statusEffectRemovedByCureAilment(const int effect, Entity* my)
{
	switch ( effect )
	{
		case EFF_ASLEEP:
		case EFF_POISONED:
		case EFF_CONFUSED:
		case EFF_BLIND:
		case EFF_GREASY:
		case EFF_MESSY:
		case EFF_PARALYZED:
		case EFF_BLEEDING:
		case EFF_SLOW:
		case EFF_PACIFY:
		case EFF_WEBBED:
		case EFF_FEAR:
		case EFF_DISORIENTED:
			return true;
			break;
		case EFF_DRUNK:
			if ( this->type == GOATMAN
				|| (my && my->behavior == &actPlayer 
					&& playerRace == RACE_GOATMAN && stat_appearance == 0) )
			{
				return false;
			}
			return true;
			break;
		default:
			break;
	}
	return false;
}

Uint32 Stat::getLootingBagKey(const int player)
{
	Uint32 lootingBagKey = player & 0xF;
	Uint16 levelKey = currentlevel & 0xFFF;
	levelKey |= ((secretlevel ? 1 : 0) << 11);
	lootingBagKey |= (levelKey << 4);

	return lootingBagKey;
}

void Stat::addItemToLootingBag(const int player, const real_t x, const real_t y, Item& item)
{
	Uint32 lootingBagKey = getLootingBagKey(player);
	if ( player_lootbags.find(lootingBagKey) == player_lootbags.end() )
	{
		player_lootbags[lootingBagKey].spawn_x = x;
		player_lootbags[lootingBagKey].spawn_y = y;

		if ( !player_lootbags[lootingBagKey].spawnedOnGround )
		{
			Entity* entity = newEntity(-1, 1, map.entities, nullptr); //Item entity.
			entity->flags[INVISIBLE] = true;
			entity->flags[UPDATENEEDED] = true;
			entity->x = x;
			entity->y = y;
			entity->sizex = 4;
			entity->sizey = 4;
			entity->yaw = (local_rng.rand() % 360) * (PI / 180.f);
			entity->vel_x = 0.0;
			entity->vel_y = 0.0;
			entity->vel_z = -.5;
			entity->flags[PASSABLE] = true;
			entity->flags[USERFLAG1] = true;
			entity->behavior = &actItem;
			entity->skill[10] = TOOL_PLAYER_LOOT_BAG;
			entity->skill[11] = WORN;
			entity->skill[12] = 0;
			entity->skill[13] = 1;
			entity->skill[14] = lootingBagKey;
			entity->skill[15] = true;
		}
		
		player_lootbags[lootingBagKey].spawnedOnGround = true;
	}
	auto& loot = player_lootbags[lootingBagKey];
	loot.items.push_back(Item());
	auto& i = loot.items.back();
	copyItem(&i, &item);

	if ( item.type >= ARTIFACT_SWORD && item.type <= ARTIFACT_GLOVES )
	{
		if ( itemIsEquipped(&item, player) )
		{
			steamAchievementClient(player, "BARONY_ACH_CHOSEN_ONE");
		}
	}
}

bool Stat::emptyLootingBag(const int player, Uint32 key)
{
	if ( multiplayer == CLIENT )
	{
		return false;
	}
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( stats[i] && stats[i]->player_lootbags.find(key) != stats[i]->player_lootbags.end() )
		{
			messagePlayer(player, MESSAGE_INTERACTION | MESSAGE_INVENTORY, Language::get(4332));
			if ( !stats[i]->player_lootbags[key].looted )
			{
				for ( auto& item_loot : stats[i]->player_lootbags[key].items )
				{
					//dropItemMonster(&item, players[i]->entity, stats[i], item.count);
					Item* item2 = newItem(item_loot.type, item_loot.status, 
						item_loot.beatitude, item_loot.count, item_loot.appearance, item_loot.identified, nullptr);
					if ( item2 )
					{
						int pickedUpCount = item2->count;
						Item* item = itemPickup(player, item2);
						if ( item )
						{
							if ( players[player]->isLocalPlayer() )
							{
								// item is the new inventory stack for server, free the picked up items
								free(item2);
								int oldcount = item->count;
								item->count = pickedUpCount;
								//messagePlayer(i, MESSAGE_INTERACTION | MESSAGE_INVENTORY, Language::get(504), item->description());
								item->count = oldcount;
							}
							else
							{
								//messagePlayer(i, MESSAGE_INTERACTION | MESSAGE_INVENTORY, Language::get(504), item->description());
								free(item); // item is the picked up items (item == item2)
							}
						}
					}
					if ( !stats[i]->player_lootbags[key].looted )
					{
						playSoundEntity(players[player]->entity, 558, 64);
						playSoundEntity(players[player]->entity, 35 + local_rng.rand() % 3, 64);
					}
					stats[i]->player_lootbags[key].looted = true;
				}
				int owner = (key & 0xF);
				if ( owner == player )
				{
					Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_DEATHBOX_OPEN_OWN, TOOL_PLAYER_LOOT_BAG, 1);
				}
				else if ( owner != player )
				{
					Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_DEATHBOX_OPEN_OTHERS, TOOL_PLAYER_LOOT_BAG, 1);
				}
			}
			stats[i]->player_lootbags.erase(key);
			return true;
		}
	}
	return false;
}