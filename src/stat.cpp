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

Stat* stats[MAXPLAYERS];

// Constructor
Stat::Stat()
{
	this->type = NOTHING;
	this->sex = static_cast<sex_t>(rand() % 2);
	this->appearance = 0;
	strcpy(this->name, "Nobody");
	strcpy(this->obituary, language[1500]);
	this->poisonKiller = 0;
	this->HP = 10;
	this->MAXHP = 10;
	this->OLDHP = this->HP;
	this->MP = 10;
	this->MAXMP = 10;
	this->STR = 0;
	this->DEX = 0;
	this->CON = 0;
	this->INT = 0;
	this->PER = 0;
	this->CHR = 0;
	this->EXP = 0;
	this->LVL = 1;
	this->GOLD = 0;
	this->HUNGER = 800;
	this->defending = false;

	int c;
	for (c = 0; c < NUMPROFICIENCIES; c++)
	{
		this->PROFICIENCIES[c] = 0;
	}
	for (c = 0; c < NUMEFFECTS; c++)
	{
		this->EFFECTS[c] = 0;
		this->EFFECTS_TIMERS[c] = 0;
	}
	this->leader_uid = 0;
	this->FOLLOWERS.first = NULL;
	this->FOLLOWERS.last = NULL;
	this->stache_x1 = 0;
	this->stache_x2 = 0;
	this->stache_y1 = 0;
	this->stache_y2 = 0;
	this->inventory.first = NULL;
	this->inventory.last = NULL;
	this->helmet = NULL;
	this->breastplate = NULL;
	this->gloves = NULL;
	this->shoes = NULL;
	this->shield = NULL;
	this->weapon = NULL;
	this->cloak = NULL;
	this->amulet = NULL;
	this->ring = NULL;
	this->mask = NULL;
#if defined(HAVE_FMOD) || defined(HAVE_OPENAL)
	this->monster_sound = NULL;
#endif
	this->monster_idlevar = 1;
	this->magic_effects.first = NULL;
	this->magic_effects.last = NULL;
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

	strcpy(this->obituary, language[1500]);
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
	list_FreeAll(&this->inventory);
	this->helmet = NULL;
	this->breastplate = NULL;
	this->gloves = NULL;
	this->shoes = NULL;
	this->shield = NULL;
	this->weapon = NULL;
	this->cloak = NULL;
	this->amulet = NULL;
	this->ring = NULL;
	this->mask = NULL;
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

	Stat* newStat = new Stat();

	newStat->type = this->type;
	newStat->sex = this->sex;
	newStat->appearance = this->appearance;
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
	newStat->INT = this->PER;
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

	newStat->defending = this->defending;
	newStat->leader_uid = this->leader_uid;
	newStat->FOLLOWERS.first = NULL;
	newStat->FOLLOWERS.last = NULL;
	list_Copy(&newStat->FOLLOWERS, &this->FOLLOWERS);
	newStat->stache_x1 = this->stache_x1;
	newStat->stache_x2 = this->stache_x2;
	newStat->stache_y1 = this->stache_y1;
	newStat->stache_y2 = this->stache_y2;

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

#if defined(HAVE_FMOD) || defined(HAVE_OPENAL)
	newStat->monster_sound = NULL;
#endif
	newStat->monster_idlevar = this->monster_idlevar;
	newStat->magic_effects.first = NULL;
	newStat->magic_effects.last = NULL;

	return newStat;
}

void Stat::printStats()
{
	printlog("type = %d\n", this->type);
	printlog("sex = %d\n", this->sex);
	printlog("appearance = %d\n", this->appearance);
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

