/*-------------------------------------------------------------------------------

BARONY
File: stat.cpp
Desc: shared functions for the Stat struct within editor.exe and barony.exe

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include "entity.hpp"
#include "stat.hpp"

// Constructor
Stat::Stat(Sint32 sprite)
{
	this->type = NOTHING;
	strcpy(this->name, "");
	strcpy(this->obituary, language[1500]);
	this->defending = false;
	this->poisonKiller = 0;
	this->sex = static_cast<sex_t>(rand() % 2);
	this->appearance = 0;
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
	int c;
	for ( c = 0; c < std::max<real_t>(NUMPROFICIENCIES, NUMEFFECTS); c++ )
	{
		if ( c < NUMPROFICIENCIES )
		{
			this->PROFICIENCIES[c] = 0;
		}
		if ( c < NUMEFFECTS )
		{
			this->EFFECTS[c] = false;
		}
		if ( c < NUMEFFECTS )
		{
			this->EFFECTS_TIMERS[c] = 0;
		}
	}

	/*for ( c = 0; c < 96; c = c + 6 )
	{
		this->EDITOR_ITEMS[c] = 0;
		this->EDITOR_ITEMS[c + 1] = 0;
		this->EDITOR_ITEMS[c + 2] = 10;
		this->EDITOR_ITEMS[c + 3] = 1;
		this->EDITOR_ITEMS[c + 4] = 1;
		this->EDITOR_ITEMS[c + 5] = 100;
	}*/

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

	if ( multiplayer != CLIENT )
	{
		switch ( (int)sprite )
		{
		case 70:
		case (1000 + GNOME):
			//this->type = GNOME;
			this->sex = static_cast<sex_t>(rand() % 2);
			this->appearance = 0;
			this->HP = 50;
			this->MAXHP = 50;
			this->MP = 50;
			this->MAXMP = 50;
			this->OLDHP = this->HP;
			this->STR = 2;
			this->DEX = 0;
			this->CON = 4;
			this->INT = 0;
			this->PER = 2;
			this->CHR = -1;
			this->EXP = 0;
			this->LVL = 5;
			//this->RANDOMGOLD = 20;
			//this->GOLD = 40 + rand() % this->RANDOMGOLD;
			this->HUNGER = 900;

			this->PROFICIENCIES[PRO_SWORD] = 35;
			this->PROFICIENCIES[PRO_MACE] = 50;
			this->PROFICIENCIES[PRO_AXE] = 45;
			this->PROFICIENCIES[PRO_POLEARM] = 25;
			this->PROFICIENCIES[PRO_RANGED] = 35;
			this->PROFICIENCIES[PRO_SHIELD] = 35;

			break;
		case 71:
		case (1000 + DEVIL):
			this->type = DEVIL;
			this->sex = static_cast<sex_t>(rand() % 2);
			this->appearance = rand();
			strcpy(this->name, "Baphomet");
			this->inventory.first = NULL;
			this->inventory.last = NULL;
			this->HP = 1250 + 250 * numplayers;
			this->MAXHP = this->HP;
			this->MP = 2000;
			this->MAXMP = 2000;
			this->OLDHP = this->HP;
			this->STR = -50;
			this->DEX = -20;
			this->CON = 10;
			this->INT = 50;
			this->PER = 500;
			this->CHR = 50;
			this->EXP = 0;
			this->LVL = 30;
			this->HUNGER = 900;

			this->EFFECTS[EFF_LEVITATING] = true;
			this->EFFECTS_TIMERS[EFF_LEVITATING] = 0;

			this->PROFICIENCIES[PRO_MAGIC] = 100;
			this->PROFICIENCIES[PRO_SPELLCASTING] = 100;

			break;
		case 62:
		case (1000 + LICH):
			this->type = LICH;
			this->sex = MALE;
			this->appearance = rand();
			strcpy(this->name, "Baron Herx");
			this->inventory.first = NULL;
			this->inventory.last = NULL;
			this->HP = 1000 + 250 * numplayers;
			this->MAXHP = this->HP;
			this->MP = 1000;
			this->MAXMP = 1000;
			this->OLDHP = this->HP;
			this->STR = 20;
			this->DEX = 8;
			this->CON = 8;
			this->INT = 20;
			this->PER = 80;
			this->CHR = 50;
			this->EXP = 0;
			this->LVL = 25;
			this->GOLD = 100;
			this->HUNGER = 900;

			this->EFFECTS[EFF_LEVITATING] = true;
			this->EFFECTS_TIMERS[EFF_LEVITATING] = 0;

			break;
		case 48:
		case (1000 + SPIDER):
			this->type = SPIDER;
			this->sex = static_cast<sex_t>(rand() % 2);
			this->appearance = rand();
			strcpy(this->name, "");
			this->inventory.first = NULL;
			this->inventory.last = NULL;
			this->HP = 50;
			this->MAXHP = 50;
			this->MP = 10;
			this->MAXMP = 10;
			this->OLDHP = this->HP;
			this->STR = 3;
			this->DEX = 8;
			this->CON = 4;
			this->INT = -3;
			this->PER = -3;
			this->CHR = -1;
			this->EXP = 0;
			this->LVL = 5;
			this->GOLD = 0;
			this->HUNGER = 900;

			break;
		case 36:
		case (1000 + GOBLIN):
			this->type = GOBLIN;
			this->sex = static_cast<sex_t>(rand() % 2);
			this->appearance = rand();
			strcpy(this->name, "");
			this->inventory.first = NULL;
			this->inventory.last = NULL;
			this->HP = 60;
			this->MAXHP = 60;
			this->MP = 20;
			this->MAXMP = 20;
			this->OLDHP = this->HP;
			this->STR = 6;
			this->DEX = 0;
			this->CON = 2;
			this->INT = -1;
			this->PER = 0;
			this->CHR = -1;
			this->EXP = 0;
			this->LVL = 6;
			if ( rand() % 3 == 0 )
			{
				this->GOLD = 10 + rand() % 20;
			}
			else
			{
				this->GOLD = 0;
			}
			this->HUNGER = 900;

			this->PROFICIENCIES[PRO_SWORD] = 35;
			this->PROFICIENCIES[PRO_MACE] = 50;
			this->PROFICIENCIES[PRO_AXE] = 45;
			this->PROFICIENCIES[PRO_POLEARM] = 25;
			this->PROFICIENCIES[PRO_RANGED] = 35;
			this->PROFICIENCIES[PRO_SHIELD] = 35;

			break;
		case 35:
		case (1000 + SHOPKEEPER):
			this->type = SHOPKEEPER;
			this->sex = MALE;
			this->appearance = rand();
			strcpy(this->name, language[158 + rand() % 26]);
			this->inventory.first = NULL;
			this->inventory.last = NULL;
			this->HP = 300;
			this->MAXHP = 300;
			this->MP = 200;
			this->MAXMP = 200;
			this->OLDHP = this->HP;
			this->STR = 10;
			this->DEX = 4;
			this->CON = 10;
			this->INT = 7;
			this->PER = 7;
			this->CHR = 3 + rand() % 4;
			this->EXP = 0;
			this->LVL = 10;
			this->GOLD = 300 + rand() % 200;
			this->HUNGER = 900;

			this->FOLLOWERS.first = NULL;
			this->FOLLOWERS.last = NULL;
			this->PROFICIENCIES[PRO_MAGIC] = 50;
			this->PROFICIENCIES[PRO_SPELLCASTING] = 50;
			this->PROFICIENCIES[PRO_TRADING] = 75;
			this->PROFICIENCIES[PRO_APPRAISAL] = 75;

			break;
		case 30:
		case (1000 + TROLL):
			this->type = TROLL;
			this->sex = static_cast<sex_t>(rand() % 2);
			this->appearance = rand();
			this->inventory.first = NULL;
			this->inventory.last = NULL;
			this->HP = 100 + rand() % 20;
			this->MAXHP = this->HP;
			this->MP = 30;
			this->MAXMP = 30;
			this->OLDHP = this->HP;
			this->STR = 15;
			this->DEX = -2;
			this->CON = 5;
			this->INT = -4;
			this->PER = -2;
			this->CHR = -1;
			this->EXP = 0;
			this->LVL = 12;
			this->GOLD = 0;
			this->HUNGER = 900;

			break;
		case 27:
		case (1000 + HUMAN):
			this->type = HUMAN;
			this->sex = static_cast<sex_t>(rand() % 2);
			this->appearance = rand() % 18; //NUMAPPEARANCES = 18
			strcpy(this->name, "");
			this->inventory.first = NULL;
			this->inventory.last = NULL;
			this->HP = 30 + rand() % 20;
			this->MAXHP = this->HP;
			this->MP = 20 + rand() % 20;
			this->MAXMP = this->MP;
			this->OLDHP = this->HP;
			this->STR = -1 + rand() % 4;
			this->DEX = 4 + rand() % 4;
			this->CON = -2 + rand() % 4;
			this->INT = -1 + rand() % 4;
			this->PER = -2 + rand() % 4;
			this->CHR = -3 + rand() % 4;
			this->EXP = 0;
			this->LVL = 3;
			if ( rand() % 2 == 0 )
			{
				this->GOLD = 20 + rand() % 20;
			}
			else
			{
				this->GOLD = 0;
			}
			this->HUNGER = 900;

			this->PROFICIENCIES[PRO_SWORD] = 45;
			this->PROFICIENCIES[PRO_MACE] = 35;
			this->PROFICIENCIES[PRO_AXE] = 35;
			this->PROFICIENCIES[PRO_POLEARM] = 45;
			this->PROFICIENCIES[PRO_RANGED] = 40;
			this->PROFICIENCIES[PRO_SHIELD] = 35;

			break;
		case 83:
		case (1000 + KOBOLD):
			this->type = KOBOLD;
		case 84:
		case (1000 + SCARAB):
			this->type = SCARAB;
		case 85:
		case (1000 + CRYSTALGOLEM):
			this->type = CRYSTALGOLEM;
		case 86:
		case (1000 + INCUBUS):
			this->type = INCUBUS;
		case 87:
		case (1000 + VAMPIRE):
			this->type = VAMPIRE;
		case 88:
		case (1000 + SHADOW):
			this->type = SHADOW;
		case 89:
		case (1000 + COCKATRICE):
			this->type = COCKATRICE;
		case 90:
		case (1000 + INSECTOID):
			this->type = INSECTOID;
		case 91:
		case (1000 + GOATMAN):
			this->type = GOATMAN;
		case 92:
		case (1000 + AUTOMATON):
			this->type = AUTOMATON;
		case 93:
		case (1000 + LICH_ICE):
			this->type = LICH_ICE;
		case 94:
		case (1000 + LICH_FIRE):
			this->type = LICH_FIRE;
			break;
		case 95:
		case (1000 + SKELETON):
			this->type = SKELETON;
			this->sex = static_cast<sex_t>(rand() % 2);
			this->appearance = rand();
			this->HP = 40;
			this->MAXHP = 40;
			this->MP = 30;
			this->MAXMP = 30;
			this->OLDHP = this->HP;
			this->STR = 1;
			this->DEX = -1;
			this->CON = 0;
			this->INT = -1;
			this->PER = 1;
			this->CHR = -3;
			this->EXP = 0;
			this->LVL = 2;
			this->GOLD = 0;
			this->HUNGER = 900;

			this->PROFICIENCIES[PRO_SWORD] = 35;
			this->PROFICIENCIES[PRO_MACE] = 50;
			this->PROFICIENCIES[PRO_AXE] = 45;
			this->PROFICIENCIES[PRO_POLEARM] = 25;
			this->PROFICIENCIES[PRO_RANGED] = 35;
			this->PROFICIENCIES[PRO_SHIELD] = 35;
			break;
		case 96:
		case (1000 + RAT):
			this->type = RAT;
			this->sex = static_cast<sex_t>(rand() % 2);
			this->appearance = rand();
			strcpy(this->name, "");
			this->inventory.first = NULL;
			this->inventory.last = NULL;
			this->HP = 30;
			this->MAXHP = 30;
			this->MP = 10;
			this->MAXMP = 10;
			this->OLDHP = this->HP;
			this->STR = 0;
			this->DEX = 2;
			this->CON = 1;
			this->INT = -2;
			this->PER = 0;
			this->CHR = -1;
			this->EXP = 0;
			this->LVL = 1;
			this->GOLD = 0;
			this->HUNGER = 900;
			break;
		case 10:
		default:
			break;
		}
	}
}
