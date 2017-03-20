/*-------------------------------------------------------------------------------

	BARONY
	File: stat.cpp
	Desc: functions for the Stat struct

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "entity.hpp"


// Constructor
Stat::Stat()
{
	this->type = NOTHING;
	this->sex = static_cast<sex_t>(rand() % 10);
	this->appearance = 0;
	strcpy(this->name, "Set Via Map");
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
Stat::~Stat() {}

