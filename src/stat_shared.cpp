/*-------------------------------------------------------------------------------

BARONY
File: stat.cpp
Desc: shared functions for the Stat struct within editor.exe and barony.exe

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/


#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "magic/magic.hpp"

// Constructor
Stat::Stat(Sint32 sprite) :
	sneaking(MISC_FLAGS[1])
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

	//random variables to add to base
	this->RANDOM_LVL = 0;
	this->RANDOM_GOLD = 0;
	this->RANDOM_STR = 0;
	this->RANDOM_DEX = 0;
	this->RANDOM_CON = 0;
	this->RANDOM_INT = 0;
	this->RANDOM_PER = 0;
	this->RANDOM_CHR = 0;
	this->RANDOM_MAXHP = 0;
	this->RANDOM_HP = 0;
	this->RANDOM_MAXMP = 0;
	this->RANDOM_MP = 0;
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

	for ( c = 0; c < ITEM_SLOT_NUM; c = c + ITEM_SLOT_NUMPROPERTIES )
	{
		this->EDITOR_ITEMS[c] = 0;
		this->EDITOR_ITEMS[c + 1] = 0;
		this->EDITOR_ITEMS[c + 2] = 10;
		this->EDITOR_ITEMS[c + 3] = 1;
		this->EDITOR_ITEMS[c + 4] = 1;
		this->EDITOR_ITEMS[c + 5] = 100;
		this->EDITOR_ITEMS[c + 6] = 0;
	}
	for ( c = 0; c < 32; c++ )
	{
		this->MISC_FLAGS[c] = 0;
	}

	for ( c = 0; c < NUMSTATS; c++ )
	{
		this->PLAYER_LVL_STAT_BONUS[c] = -1;
	}

	for ( c = 0; c < NUMSTATS * 2; c++ )
	{
		this->PLAYER_LVL_STAT_TIMER[c] = 0;
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

	if ( multiplayer != CLIENT )
	{
		setDefaultMonsterStats(this, (int)sprite);
	}
}

void setDefaultMonsterStats(Stat* stats, int sprite)
{
	switch ( sprite )
	{
		case 70:
		case (1000 + GNOME):
			stats->type = GNOME;
			stats->sex = static_cast<sex_t>(rand() % 2);
			stats->appearance = 0;
			stats->HP = 50;
			stats->MAXHP = 50;
			stats->MP = 50;
			stats->MAXMP = 50;
			stats->OLDHP = stats->HP;
			stats->STR = 2;
			stats->DEX = 0;
			stats->CON = 4;
			stats->INT = 0;
			stats->PER = 2;
			stats->CHR = -1;
			stats->EXP = 0;
			stats->LVL = 5;
			stats->RANDOM_GOLD = 20;
			stats->GOLD = 40;
			stats->HUNGER = 900;

			stats->PROFICIENCIES[PRO_SWORD] = 35;
			stats->PROFICIENCIES[PRO_MACE] = 50;
			stats->PROFICIENCIES[PRO_AXE] = 45;
			stats->PROFICIENCIES[PRO_POLEARM] = 25;
			stats->PROFICIENCIES[PRO_RANGED] = 35;
			stats->PROFICIENCIES[PRO_SHIELD] = 35;


			stats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_CLOAK] = 1;

			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1 + ITEM_CHANCE] = 33; //Fish
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_2] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_2 + ITEM_CHANCE] = 10; //Random Gems
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_3] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_3 + ITEM_CHANCE] = 2; //Winny's report

			break;
		case 71:
		case (1000 + DEVIL):
			stats->type = DEVIL;
			stats->sex = static_cast<sex_t>(rand() % 2);
			stats->appearance = rand();
			strcpy(stats->name, "Baphomet");
			stats->inventory.first = nullptr;
			stats->inventory.last = nullptr;
			stats->HP = 1250 + 250 * numplayers;
			stats->MAXHP = stats->HP;
			stats->MP = 2000;
			stats->MAXMP = 2000;
			stats->OLDHP = stats->HP;
			stats->STR = -50;
			stats->DEX = -20;
			stats->CON = 10;
			stats->INT = 50;
			stats->PER = 500;
			stats->CHR = 50;
			stats->EXP = 0;
			stats->LVL = 30;
			stats->HUNGER = 900;

			stats->EFFECTS[EFF_LEVITATING] = true;
			stats->EFFECTS_TIMERS[EFF_LEVITATING] = 0;

			stats->PROFICIENCIES[PRO_MAGIC] = 100;
			stats->PROFICIENCIES[PRO_SPELLCASTING] = 100;

			break;
		case 62:
		case (1000 + LICH):
			stats->type = LICH;
			stats->sex = MALE;
			stats->appearance = rand();
			strcpy(stats->name, "Baron Herx");
			stats->inventory.first = NULL;
			stats->inventory.last = NULL;
			stats->HP = 1000 + 250 * numplayers;
			stats->MAXHP = stats->HP;
			stats->MP = 1000;
			stats->MAXMP = 1000;
			stats->OLDHP = stats->HP;
			stats->STR = 20;
			stats->DEX = 8;
			stats->CON = 8;
			stats->INT = 20;
			stats->PER = 80;
			stats->CHR = 50;
			stats->EXP = 0;
			stats->LVL = 25;
			stats->GOLD = 100;
			stats->HUNGER = 900;

			stats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] = 1;
			break;
		case 48:
		case (1000 + SPIDER):
			stats->type = SPIDER;
			stats->sex = static_cast<sex_t>(rand() % 2);
			stats->appearance = rand();
			stats->inventory.first = NULL;
			stats->inventory.last = NULL;
			stats->HP = 50;
			stats->MAXHP = 50;
			stats->MP = 10;
			stats->MAXMP = 10;
			stats->OLDHP = stats->HP;
			stats->STR = 3;
			stats->DEX = 8;
			stats->CON = 4;
			stats->INT = -3;
			stats->PER = -3;
			stats->CHR = -1;
			stats->EXP = 0;
			stats->LVL = 5;
			stats->GOLD = 0;
			stats->HUNGER = 900;

			break;
		case 36:
		case (1000 + GOBLIN):
			stats->type = GOBLIN;
			stats->sex = static_cast<sex_t>(rand() % 2);
			stats->appearance = rand();
			stats->inventory.first = NULL;
			stats->inventory.last = NULL;
			stats->HP = 60;
			stats->MAXHP = 60;
			stats->MP = 20;
			stats->MAXMP = 20;
			stats->OLDHP = stats->HP;
			stats->STR = 5;
			stats->DEX = 0;
			stats->CON = 3;
			stats->INT = -1;
			stats->PER = 2;
			stats->CHR = -1;
			stats->EXP = 0;
			stats->LVL = 6;
			if ( rand() % 3 == 0 )
			{
				stats->GOLD = 10;
				stats->RANDOM_GOLD = 20;
			}
			else
			{
				stats->GOLD = 0;
				stats->RANDOM_GOLD = 0;
			}
			stats->HUNGER = 900;

			stats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_ARMOR] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_HELM] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_CLOAK] = 1;

			stats->PROFICIENCIES[PRO_SWORD] = 35;
			stats->PROFICIENCIES[PRO_MACE] = 50;
			stats->PROFICIENCIES[PRO_AXE] = 45;
			stats->PROFICIENCIES[PRO_POLEARM] = 25;
			stats->PROFICIENCIES[PRO_RANGED] = 100;
			stats->PROFICIENCIES[PRO_SHIELD] = 35;

			break;
		case 35:
		case (1000 + SHOPKEEPER):
			stats->type = SHOPKEEPER;
			stats->sex = MALE;
			stats->appearance = rand();
			stats->inventory.first = NULL;
			stats->inventory.last = NULL;
			stats->HP = 300;
			stats->MAXHP = 300;
			stats->MP = 200;
			stats->MAXMP = 200;
			stats->OLDHP = stats->HP;
			stats->STR = 10;
			stats->DEX = 4;
			stats->CON = 10;
			stats->INT = 7;
			stats->PER = 7;
			stats->CHR = 3;
			stats->RANDOM_CHR = 3;
			stats->EXP = 0;
			stats->LVL = 10;
			stats->GOLD = 300;
			stats->RANDOM_GOLD = 200;
			stats->HUNGER = 900;

			stats->FOLLOWERS.first = NULL;
			stats->FOLLOWERS.last = NULL;
			stats->PROFICIENCIES[PRO_MAGIC] = 50;
			stats->PROFICIENCIES[PRO_SPELLCASTING] = 50;
			stats->PROFICIENCIES[PRO_TRADING] = 75;
			stats->PROFICIENCIES[PRO_APPRAISAL] = 75;

			stats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] = 1;

			break;
		case 30:
		case (1000 + TROLL):
			stats->type = TROLL;
			stats->sex = static_cast<sex_t>(rand() % 2);
			stats->appearance = rand();
			stats->inventory.first = NULL;
			stats->inventory.last = NULL;
			stats->HP = 100;
			stats->RANDOM_HP = 20;
			stats->MAXHP = stats->HP;
			stats->RANDOM_MAXHP = stats->RANDOM_HP;
			stats->MP = 30;
			stats->MAXMP = 30;
			stats->OLDHP = stats->HP;
			stats->STR = 15;
			stats->DEX = -2;
			stats->CON = 5;
			stats->INT = -4;
			stats->PER = -2;
			stats->CHR = -1;
			stats->EXP = 0;
			stats->LVL = 12;
			stats->GOLD = 0;
			stats->HUNGER = 900;

			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1 + ITEM_CHANCE] = 33; //Random Items


			break;
		case 27:
		case (1000 + HUMAN):
			stats->type = HUMAN;
			stats->sex = static_cast<sex_t>(rand() % 2);
			stats->appearance = rand() % 18; //NUMAPPEARANCES = 18
			stats->inventory.first = NULL;
			stats->inventory.last = NULL;
			stats->HP = 30;
			stats->RANDOM_HP = 20;
			stats->MAXHP = stats->HP;
			stats->RANDOM_MAXHP = stats->RANDOM_HP;
			stats->MP = 20;
			stats->RANDOM_MP = 20;
			stats->RANDOM_MAXMP = stats->RANDOM_MP;
			stats->MAXMP = stats->MP;
			stats->OLDHP = stats->HP;
			stats->STR = -1;
			stats->RANDOM_STR = 3;
			stats->DEX = 4;
			stats->RANDOM_DEX = 3;
			stats->CON = -2;
			stats->RANDOM_CON = 3;
			stats->INT = -1;
			stats->RANDOM_INT = 3;
			stats->PER = -3;
			stats->RANDOM_PER = 4;
			stats->CHR = -3;
			stats->RANDOM_CHR = 3;
			stats->EXP = 0;
			stats->LVL = 3;
			if ( rand() % 2 == 0 )
			{
				stats->GOLD = 20;
				stats->RANDOM_GOLD = 20;
			}
			else
			{
				stats->GOLD = 0;
			}
			stats->HUNGER = 900;

			stats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_ARMOR] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_HELM] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_CLOAK] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_BOOTS] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_GLOVES] = 1;

			stats->PROFICIENCIES[PRO_SWORD] = 45;
			stats->PROFICIENCIES[PRO_MACE] = 35;
			stats->PROFICIENCIES[PRO_AXE] = 35;
			stats->PROFICIENCIES[PRO_POLEARM] = 45;
			//stats->PROFICIENCIES[PRO_RANGED] = 40;
			stats->PROFICIENCIES[PRO_SHIELD] = 35;

			break;
		case 84:
		case (1000 + KOBOLD):
			stats->type = KOBOLD;
			stats->sex = static_cast<sex_t>(rand() % 2);
			stats->appearance = 0;

			stats->HP = 100;
			stats->MAXHP = stats->HP;
			stats->RANDOM_HP = 20;
			stats->RANDOM_MAXHP = stats->RANDOM_HP;
			stats->MP = 60;
			stats->MAXMP = 60;
			stats->OLDHP = stats->HP;
			stats->STR = 20;
			stats->RANDOM_STR = 5;
			stats->DEX = 5;
			stats->RANDOM_DEX = 5;
			stats->CON = 3;
			stats->RANDOM_CON = 2;
			stats->INT = -2;
			stats->RANDOM_INT = 4;
			stats->PER = 14;
			stats->RANDOM_PER = 2;
			stats->CHR = 3;
			stats->RANDOM_CHR = 2;

			stats->EXP = 0;
			stats->LVL = 15;
			stats->GOLD = 80;
			stats->RANDOM_GOLD = 40;
			stats->HUNGER = 900;

			stats->PROFICIENCIES[PRO_SWORD] = 75;
			stats->PROFICIENCIES[PRO_AXE] = 50;
			stats->PROFICIENCIES[PRO_POLEARM] = 50;
			stats->PROFICIENCIES[PRO_RANGED] = 75;
			stats->PROFICIENCIES[PRO_SHIELD] = 35;


			stats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_CLOAK] = 1;

			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1 + ITEM_CHANCE] = 80; //Ranged spellbook
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_2] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_2 + ITEM_CHANCE] = 20; //Misc tools
			break;
		case 85:
		case (1000 + SCARAB):
			stats->type = SCARAB;
			stats->sex = static_cast<sex_t>(rand() % 2);
			stats->appearance = rand();
			stats->inventory.first = NULL;
			stats->inventory.last = NULL;
			stats->HP = 60;
			stats->MAXHP = 60;
			stats->MP = 20;
			stats->MAXMP = 20;
			stats->OLDHP = stats->HP;
			stats->STR = 10;
			stats->DEX = 12;
			stats->CON = 2;
			stats->INT = -1;
			stats->PER = 8;
			stats->CHR = 0;
			stats->EXP = 0;
			stats->LVL = 15;
			stats->GOLD = 0;
			stats->HUNGER = 900;

			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1 + ITEM_CHANCE] = 50;

			stats->PROFICIENCIES[PRO_MAGIC] = 50;
			stats->PROFICIENCIES[PRO_SPELLCASTING] = 50;

			break;
		case 86:
		case (1000 + CRYSTALGOLEM):
			stats->type = CRYSTALGOLEM;
			stats->sex = static_cast<sex_t>(rand() % 2);
			stats->appearance = 0;

			stats->HP = 200;
			stats->MAXHP = stats->HP;
			stats->RANDOM_HP = 50;
			stats->RANDOM_MAXHP = stats->RANDOM_HP;
			stats->MP = 50;
			stats->MAXMP = 50;
			stats->OLDHP = stats->HP;
			stats->STR = 50;
			stats->RANDOM_STR = 5;
			stats->DEX = 2;
			stats->RANDOM_DEX = 2;
			stats->CON = 25;
			stats->RANDOM_CON = 0;
			stats->INT = -2;
			stats->RANDOM_INT = 0;
			stats->PER = 5;
			stats->RANDOM_PER = 5;
			stats->CHR = -3;
			stats->RANDOM_CHR = 0;

			stats->EXP = 0;
			stats->LVL = 30;
			stats->GOLD = 0;
			stats->RANDOM_GOLD = 0;
			stats->HUNGER = 900;

			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1 + ITEM_CHANCE] = 50; //Random crystal item
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_2] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_2 + ITEM_CHANCE] = 5; //Random second crystal item
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_3] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_3 + ITEM_CHANCE] = 10; //Gem
			break;
		case 87:
		case (1000 + INCUBUS):
			stats->type = INCUBUS;
			stats->sex = sex_t::MALE;
			stats->appearance = rand();
			stats->inventory.first = nullptr;
			stats->inventory.last = nullptr;
			stats->MAXHP = 280;
			stats->HP = stats->MAXHP;
			stats->MAXMP = 50;
			stats->MP = stats->MAXMP;
			stats->OLDHP = stats->HP;
			stats->RANDOM_MAXHP = 25;
			stats->RANDOM_HP = stats->RANDOM_MAXHP;
			stats->RANDOM_MAXMP = 0;
			stats->RANDOM_MP = stats->RANDOM_MAXMP;
			stats->STR = 20;
			stats->RANDOM_STR = 5;
			stats->DEX = 8;
			stats->CON = 3;
			stats->RANDOM_CON = 2;
			stats->INT = -2;
			stats->RANDOM_INT = 2;
			stats->PER = 10;
			stats->RANDOM_PER = 5;
			stats->CHR = -3;
			stats->EXP = 0;
			stats->LVL = 25;

			stats->GOLD = 50;
			stats->RANDOM_GOLD = 50;

			stats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] = 1;
			//stats->EDITOR_ITEMS[ITEM_SLOT_CLOAK] = 1;
			//stats->EDITOR_ITEMS[ITEM_SLOT_BOOTS] = 1;
			//stats->EDITOR_ITEMS[ITEM_SLOT_GLOVES] = 1;

			stats->PROFICIENCIES[PRO_MACE] = 75;
			stats->PROFICIENCIES[PRO_POLEARM] = 60;
			stats->PROFICIENCIES[PRO_RANGED] = 75;
			stats->PROFICIENCIES[PRO_MAGIC] = 100;

			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1 + ITEM_CHANCE] = 33; // booze potion
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_2] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_2 + ITEM_CHANCE] = 20; // confusion potion
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_3] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_3 + ITEM_CHANCE] = 50; // magicstaff

			break;
		case 88:
		case (1000 + VAMPIRE):
			stats->type = VAMPIRE;
			stats->sex = MALE;
			stats->appearance = rand();
			stats->inventory.first = nullptr;
			stats->inventory.last = nullptr;
			stats->HP = 300;
			stats->RANDOM_HP = 0;
			stats->MAXHP = stats->HP;
			stats->RANDOM_MAXHP = stats->RANDOM_HP;
			stats->MP = 50;
			stats->RANDOM_MP = 50;
			stats->RANDOM_MAXMP = stats->RANDOM_MP;
			stats->MAXMP = stats->MP;
			stats->OLDHP = stats->HP;
			stats->STR = 20;
			stats->RANDOM_STR = 10;
			stats->DEX = 4;
			stats->RANDOM_DEX = 3;
			stats->CON = 0;
			stats->RANDOM_CON = 0;
			stats->INT = 15;
			stats->RANDOM_INT = 5;
			stats->PER = 5;
			stats->RANDOM_PER = 5;
			stats->CHR = -3;
			stats->RANDOM_CHR = -3;
			stats->EXP = 0;
			stats->LVL = 30;
			stats->GOLD = 130;
			stats->RANDOM_GOLD = 70;
			stats->HUNGER = 900;

			/*stats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_ARMOR] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_HELM] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_CLOAK] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_BOOTS] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_GLOVES] = 1;*/

			//stats->PROFICIENCIES[PRO_SWORD] = 45;
			//stats->PROFICIENCIES[PRO_MACE] = 35;
			stats->PROFICIENCIES[PRO_AXE] = 25;
			//stats->PROFICIENCIES[PRO_POLEARM] = 45;
			stats->PROFICIENCIES[PRO_RANGED] = 25;
			stats->PROFICIENCIES[PRO_SHIELD] = 25;
			stats->PROFICIENCIES[PRO_MAGIC] = 80;
			stats->PROFICIENCIES[PRO_SPELLCASTING] = 80;

			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1 + ITEM_CHANCE] = 10; // doublet
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_2] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_2 + ITEM_CHANCE] = 25; // magicstaff
			break;
		case 89:
		case (1000 + SHADOW):
			stats->type = SHADOW;
			stats->RANDOM_MAXHP = stats->RANDOM_HP;
			stats->RANDOM_MAXMP = stats->RANDOM_MP;
			stats->appearance = rand();
			stats->inventory.first = nullptr;
			stats->inventory.last = nullptr;
			stats->sex = static_cast<sex_t>(rand() % 2);
			stats->MAXHP = 170;
			stats->HP = stats->MAXHP;
			stats->MAXMP = 500;
			stats->MP = stats->MAXMP;
			stats->OLDHP = stats->HP;
			stats->STR = 20;
			stats->RANDOM_STR = 5;
			stats->DEX = 10;
			stats->RANDOM_DEX = 5;
			stats->CON = 2;
			stats->RANDOM_CON = 2;
			stats->INT = 5;
			stats->RANDOM_INT = 4;
			stats->PER = 20;
			stats->RANDOM_PER = 5;
			stats->CHR = -1;
			stats->RANDOM_CHR = 2;
			stats->EXP = 0;
			stats->LVL = 25;
			stats->HUNGER = 900;
			stats->GOLD = 100;
			stats->RANDOM_GOLD = 50;
			break;
		case 90:
		case (1000 + COCKATRICE):
			stats->type = COCKATRICE;
			stats->sex = static_cast<sex_t>(rand() % 2);
			stats->appearance = 0;

			stats->HP = 500;
			stats->MAXHP = stats->HP;
			stats->RANDOM_HP = 100;
			stats->RANDOM_MAXHP = stats->RANDOM_HP;
			stats->MP = 100;
			stats->MAXMP = 100;
			stats->OLDHP = stats->HP;
			stats->STR = 65;
			stats->RANDOM_STR = 10;
			stats->DEX = 8;
			stats->RANDOM_DEX = 0;
			stats->CON = 20;
			stats->RANDOM_CON = 0;
			stats->INT = -2;
			stats->RANDOM_INT = 0;
			stats->PER = 25;
			stats->RANDOM_PER = 0;
			stats->CHR = -3;
			stats->RANDOM_CHR = 0;

			stats->EXP = 0;
			stats->LVL = 35;
			stats->GOLD = 50;
			stats->RANDOM_GOLD = 100;
			stats->HUNGER = 900;

			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1 + ITEM_CHANCE] = 100; // random potion, qty 1-3
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_2] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_2 + ITEM_CHANCE] = 30; // magicstaff
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_3] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_3 + ITEM_CHANCE] = 20; // gemstones, qty 1-2
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_4] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_4 + ITEM_CHANCE] = 5; // spellbook
			break;
		case 91:
		case (1000 + INSECTOID):
			stats->type = INSECTOID;
			stats->sex = static_cast<sex_t>(rand() % 2);
			stats->appearance = rand();
			stats->inventory.first = nullptr;
			stats->inventory.last = nullptr;
			stats->MAXHP = 130;
			stats->HP = stats->MAXHP;
			stats->MAXMP = 50;
			stats->MP = stats->MAXMP;
			stats->OLDHP = stats->HP;
			stats->RANDOM_MAXHP = 25;
			stats->RANDOM_HP = stats->RANDOM_MAXHP;
			stats->RANDOM_MAXMP = 0;
			stats->RANDOM_MP = stats->RANDOM_MAXMP;
			stats->STR = 18;
			stats->RANDOM_STR = 4;
			stats->DEX = 9;
			stats->CON = 15;
			stats->RANDOM_CON = 0;
			stats->INT = -2;
			stats->RANDOM_INT = 2;
			stats->PER = 15;
			stats->RANDOM_PER = 0;
			stats->CHR = -3;
			stats->EXP = 0;
			stats->LVL = 25;

			stats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_ARMOR] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_HELM] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_CLOAK] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_BOOTS] = 1;
			//stats->EDITOR_ITEMS[ITEM_SLOT_GLOVES] = 1;

			stats->PROFICIENCIES[PRO_SWORD] = 50;
			//stats->PROFICIENCIES[PRO_MACE] = 35;
			stats->PROFICIENCIES[PRO_AXE] = 35;
			stats->PROFICIENCIES[PRO_POLEARM] = 60;
			stats->PROFICIENCIES[PRO_RANGED] = 50;
			stats->PROFICIENCIES[PRO_SHIELD] = 35;

			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1 + ITEM_CHANCE] = 100; // iron daggers, qty 2-8
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_2] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_2 + ITEM_CHANCE] = 50; // shortbow
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_3] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_3 + ITEM_CHANCE] = 5; // spellbook
			//stats->EDITOR_ITEMS[ITEM_SLOT_INV_3] = 1;
			//stats->EDITOR_ITEMS[ITEM_SLOT_INV_3 + ITEM_CHANCE] = 20; // gemstones, qty 1-2
			//stats->EDITOR_ITEMS[ITEM_SLOT_INV_4] = 1;
			//stats->EDITOR_ITEMS[ITEM_SLOT_INV_4 + ITEM_CHANCE] = 5; // spellbook
			break;
		case 92:
		case (1000 + GOATMAN):
			stats->type = GOATMAN;
			stats->sex = static_cast<sex_t>(rand() % 2);
			stats->appearance = rand();
			stats->inventory.first = nullptr;
			stats->inventory.last = nullptr;
			stats->MAXHP = 150;
			stats->HP = stats->MAXHP;
			stats->MAXMP = 20;
			stats->MP = stats->MAXMP;
			stats->OLDHP = stats->HP;
			stats->RANDOM_MAXHP = 20;
			stats->RANDOM_HP = stats->RANDOM_MAXHP;
			//stats->RANDOM_MAXMP = 20;
			//stats->RANDOM_MP = stats->RANDOM_MAXMP;
			stats->STR = 15;
			stats->DEX = 5;
			stats->CON = 5;
			stats->INT = -1;
			stats->PER = 0;
			stats->RANDOM_PER = 5;
			stats->CHR = -1;
			stats->EXP = 0;
			stats->LVL = 20;
			if ( rand() % 3 > 0 )
			{
				stats->GOLD = 100;
				stats->RANDOM_GOLD = 50;
			}
			else
			{
				stats->GOLD = 0;
				stats->RANDOM_GOLD = 0;
			}
			stats->HUNGER = 900;

			stats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_ARMOR] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_HELM] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_CLOAK] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_BOOTS] = 1;

			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1 + ITEM_CHANCE] = 5; // spellbook

			//stats->PROFICIENCIES[PRO_SWORD] = 35;
			stats->PROFICIENCIES[PRO_MACE] = 80;
			stats->PROFICIENCIES[PRO_AXE] = 60;
			//stats->PROFICIENCIES[PRO_POLEARM] = 25;
			stats->PROFICIENCIES[PRO_RANGED] = 60; //Chuck booze at you.
			//stats->PROFICIENCIES[PRO_SHIELD] = 35;
			break;
		case 93:
		case (1000 + AUTOMATON):
			stats->type = AUTOMATON;
			stats->sex = static_cast<sex_t>(rand() % 2);
			stats->appearance = rand();
			stats->inventory.first = nullptr;
			stats->inventory.last = nullptr;
			stats->MAXHP = 115;
			stats->HP = stats->MAXHP;
			stats->MAXMP = 20;
			stats->MP = stats->MAXMP;
			stats->OLDHP = stats->HP;
			stats->RANDOM_MAXHP = 20;
			stats->RANDOM_HP = stats->RANDOM_MAXHP;
			//stats->RANDOM_MAXMP = 20;
			//stats->RANDOM_MP = stats->RANDOM_MAXMP;
			stats->STR = 20;
			stats->DEX = 5;
			stats->CON = 8;
			stats->INT = -1;
			stats->PER = 10;
			stats->CHR = -3;
			stats->EXP = 0;
			stats->LVL = 20;
			stats->HUNGER = 900;

			stats->PROFICIENCIES[PRO_SWORD] = 60;
			stats->PROFICIENCIES[PRO_MACE] = 60;
			stats->PROFICIENCIES[PRO_AXE] = 60;
			stats->PROFICIENCIES[PRO_RANGED] = 60;
			//stats->PROFICIENCIES[PRO_POLEARM] = 25;
			stats->PROFICIENCIES[PRO_SHIELD] = 60;
			break;
		case 94:
		case (1000 + LICH_ICE):
			stats->type = LICH_ICE;
			stats->RANDOM_MAXHP = stats->RANDOM_HP;
			stats->RANDOM_MAXMP = stats->RANDOM_MP;
			break;
		case 95:
		case (1000 + LICH_FIRE):
			stats->type = LICH_FIRE;
			stats->RANDOM_MAXHP = stats->RANDOM_HP;
			stats->RANDOM_MAXMP = stats->RANDOM_MP;
			break;
		case 83:
		case (1000 + SKELETON):
			stats->type = SKELETON;
			stats->sex = static_cast<sex_t>(rand() % 2);
			stats->appearance = rand();
			stats->HP = 40;
			stats->MAXHP = 40;
			stats->MP = 30;
			stats->MAXMP = 30;
			stats->OLDHP = stats->HP;
			stats->STR = 0;
			stats->DEX = -1;
			stats->CON = 1;
			stats->INT = -1;
			stats->PER = 2;
			stats->CHR = -3;
			stats->EXP = 0;
			stats->LVL = 2;
			stats->GOLD = 0;
			stats->HUNGER = 900;

			stats->PROFICIENCIES[PRO_SWORD] = 35;
			stats->PROFICIENCIES[PRO_MACE] = 50;
			stats->PROFICIENCIES[PRO_AXE] = 45;
			stats->PROFICIENCIES[PRO_POLEARM] = 25;
			stats->PROFICIENCIES[PRO_RANGED] = 35;
			stats->PROFICIENCIES[PRO_SHIELD] = 35;

			stats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_HELM] = 1;

			break;

		case 75:
		case (1000 + DEMON):
			stats->type = DEMON;
			stats->sex = static_cast<sex_t>(rand() % 2);
			stats->appearance = rand();
			stats->inventory.first = NULL;
			stats->inventory.last = NULL;
			stats->HP = 120;
			stats->MAXHP = stats->HP;
			stats->MP = 200;
			stats->MAXMP = 200;
			stats->OLDHP = stats->HP;
			stats->STR = 30;
			stats->DEX = 10;
			stats->CON = 10;
			stats->INT = 5;
			stats->PER = 50;
			stats->CHR = -4;
			stats->EXP = 0;
			stats->LVL = 20;
			stats->GOLD = 0;
			stats->HUNGER = 900;

			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1 + ITEM_CHANCE] = 50; //Random Items
			break;
		case 76:
		case (1000 + CREATURE_IMP):
			stats->type = CREATURE_IMP;
			stats->sex = static_cast<sex_t>(rand() % 2);
			stats->appearance = rand();
			stats->inventory.first = NULL;
			stats->inventory.last = NULL;
			stats->HP = 80;
			stats->MAXHP = stats->HP;
			stats->MP = 80;
			stats->MAXMP = 80;
			stats->OLDHP = stats->HP;
			stats->STR = 20;
			stats->DEX = 7;
			stats->CON = 9;
			stats->INT = -2;
			stats->PER = 50;
			stats->CHR = -3;
			stats->EXP = 0;
			stats->LVL = 14;
			if ( rand() % 10 )
			{
				stats->GOLD = 0;
				stats->RANDOM_GOLD = 0;
			}
			else
			{
				stats->GOLD = 20;
				stats->RANDOM_GOLD = 20;
			}
			stats->HUNGER = 900;

			stats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] = 1;

			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1 + ITEM_CHANCE] = 100; //Random Items
			break;
		case 77:
		//case 37:
		case (1000 + MINOTAUR):
			stats->type = MINOTAUR;
			stats->sex = MALE;
			stats->appearance = rand();
			stats->inventory.first = NULL;
			stats->inventory.last = NULL;
			stats->HP = 400;
			stats->MAXHP = 400;
			stats->MP = 100;
			stats->MAXMP = 100;
			stats->OLDHP = stats->HP;
			stats->STR = 35;
			stats->DEX = 15;
			stats->CON = 15;
			stats->INT = 5;
			stats->PER = 5;
			stats->CHR = -5;
			stats->EXP = 0;
			stats->LVL = 20;
			stats->GOLD = 0;
			stats->HUNGER = 900;

			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1 + ITEM_CHANCE] = 100; //Random Items
			break;
		case 78:
		case (1000 + SCORPION):
			stats->type = SCORPION;
			stats->sex = static_cast<sex_t>(rand() % 2);
			stats->appearance = rand();
			stats->inventory.first = NULL;
			stats->inventory.last = NULL;
			stats->HP = 70;
			stats->MAXHP = 70;
			stats->MP = 10;
			stats->MAXMP = 10;
			stats->OLDHP = stats->HP;
			stats->STR = 13;
			stats->DEX = 3;
			stats->CON = 4;
			stats->INT = -3;
			stats->PER = -3;
			stats->CHR = -4;
			stats->EXP = 0;
			stats->LVL = 7;
			stats->GOLD = 0;
			stats->HUNGER = 900;
			break;
		case 79:
		case (1000 + SLIME):
			stats->type = SLIME;
			stats->sex = static_cast<sex_t>(rand() % 2);
			stats->appearance = rand();
			stats->inventory.first = NULL;
			stats->inventory.last = NULL;
			if ( stats->LVL >= 7 )   // blue slime
			{
				stats->HP = 70;
				stats->MAXHP = 70;
				stats->MP = 70;
				stats->MAXMP = 70;
				stats->STR = 10;
			}
			else     // green slime
			{
				stats->STR = 3;
				stats->HP = 60;
				stats->MAXHP = 60;
				stats->MP = 60;
				stats->MAXMP = 60;
			}
			stats->OLDHP = stats->HP;
			stats->DEX = -4;
			stats->CON = 3;
			stats->INT = -4;
			stats->PER = -2;
			stats->CHR = -4;
			stats->EXP = 0;
			stats->GOLD = 0;
			stats->HUNGER = 900;
			break;
		case 80:
		case (1000 + SUCCUBUS):
			stats->type = SUCCUBUS;
			stats->sex = FEMALE;
			stats->appearance = rand();
			stats->HP = 60;
			stats->MAXHP = 60;
			stats->MP = 40;
			stats->MAXMP = 40;
			stats->OLDHP = stats->HP;
			stats->STR = 7;
			stats->DEX = 3;
			stats->CON = 3;
			stats->INT = 2;
			stats->PER = 2;
			stats->CHR = 5;
			stats->EXP = 0;
			stats->LVL = 10;
			stats->GOLD = 0;
			stats->HUNGER = 900;

			break;
		case 81:
		case (1000 + RAT):
			stats->type = RAT;
			stats->sex = static_cast<sex_t>(rand() % 2);
			stats->appearance = rand();
			stats->inventory.first = NULL;
			stats->inventory.last = NULL;
			stats->HP = 30;
			stats->MAXHP = 30;
			stats->MP = 10;
			stats->MAXMP = 10;
			stats->OLDHP = stats->HP;
			stats->STR = 0;
			stats->DEX = 2;
			stats->CON = 1;
			stats->INT = -2;
			stats->PER = 0;
			stats->CHR = -1;
			stats->EXP = 0;
			stats->LVL = 1;
			stats->GOLD = 0;
			stats->HUNGER = 900;

			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1 + ITEM_CHANCE] = 25; //Random Items
			break;
		case 82:
		case (1000 + GHOUL):
			stats->type = GHOUL;
			stats->sex = static_cast<sex_t>(rand() % 2);
			stats->appearance = rand();
			stats->inventory.first = NULL;
			stats->inventory.last = NULL;
			stats->HP = 90;
			stats->MAXHP = 90;
			stats->MP = 10;
			stats->MAXMP = 10;
			stats->OLDHP = stats->HP;
			stats->STR = 8;
			stats->DEX = -3;
			stats->CON = -1;
			stats->INT = -2;
			stats->PER = -1;
			stats->CHR = -5;
			stats->EXP = 0;
			stats->LVL = 7;
			stats->GOLD = 0;
			stats->HUNGER = 900;

			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_1 + ITEM_CHANCE] = 5; //Random Items
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_2] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_2 + ITEM_CHANCE] = 10; //Random Items
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_3] = 1;
			stats->EDITOR_ITEMS[ITEM_SLOT_INV_3 + ITEM_CHANCE] = 25; //Random Items
			break;
		case 10:
		default:
			break;
	}
}
