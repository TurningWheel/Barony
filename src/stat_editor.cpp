/*-------------------------------------------------------------------------------

	BARONY
	File: stat_editor.cpp
	Desc: functions for the Stat struct available to editor only

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "magic/magic.hpp"

//Destructor
Stat::~Stat() {}


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

	//random variables to add to base
	newStat->RANDOM_LVL = this->RANDOM_LVL = 0;
	newStat->RANDOM_GOLD = this->RANDOM_GOLD = 0;
	newStat->RANDOM_STR = this->RANDOM_STR = 0;
	newStat->RANDOM_DEX = this->RANDOM_DEX = 0;
	newStat->RANDOM_CON = this->RANDOM_CON = 0;
	newStat->RANDOM_INT = this->RANDOM_INT = 0;
	newStat->RANDOM_PER = this->RANDOM_PER = 0;
	newStat->RANDOM_CHR = this->RANDOM_CHR = 0;
	newStat->RANDOM_MAXHP = this->RANDOM_MAXHP = 0;
	newStat->RANDOM_HP = this->RANDOM_HP = 0;
	newStat->RANDOM_MAXMP = this->RANDOM_MAXMP = 0;
	newStat->RANDOM_MP = this->RANDOM_MP = 0;

	for ( c = 0; c < NUMPROFICIENCIES; c++ )
	{
		newStat->PROFICIENCIES[c] = this->PROFICIENCIES[c];
	}
	for ( c = 0; c < NUMEFFECTS; c++ )
	{
		newStat->EFFECTS[c] = this->EFFECTS[c];
		newStat->EFFECTS_TIMERS[c] = this->EFFECTS_TIMERS[c];
	}

	for ( c = 0; c < 96; c++ )
	{
		newStat->EDITOR_ITEMS[c] = this->EDITOR_ITEMS[c];
	}

	for ( c = 0; c < 32; c++ )
	{
		newStat->EDITOR_FLAGS[c] = this->EDITOR_FLAGS[c] = 0;
	}

	return newStat;
}

