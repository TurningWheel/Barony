/*-------------------------------------------------------------------------------

	BARONY
	File: stat.hpp
	Desc: header for stat.cpp (contains Stat class definition)

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#ifdef HAVE_FMOD
#include "fmod.h"
#endif

class Item;
enum Monster : int;
//enum Item;
//enum Status;
//enum Category;


// effects
#define EFF_ASLEEP 0
#define EFF_POISONED 1
#define EFF_STUNNED 2
#define EFF_CONFUSED 3
#define EFF_DRUNK 4
#define EFF_INVISIBLE 5
#define EFF_BLIND 6
#define EFF_GREASY 7
#define EFF_MESSY 8
#define EFF_FAST 9
#define EFF_PARALYZED 10
#define EFF_LEVITATING 11
#define EFF_TELEPATH 12
#define EFF_VOMITING 13
#define EFF_BLEEDING 14
#define EFF_SLOW 15
#define NUMEFFECTS 16

// proficiencies
static const int PRO_LOCKPICKING = 0;   // base attribute: dex
static const int PRO_STEALTH = 1;       // base attribute: dex
static const int PRO_TRADING = 2;       // base attribute: chr
static const int PRO_APPRAISAL = 3;     // base attribute: per
static const int PRO_SWIMMING = 4;      // base attribute: con
static const int PRO_LEADERSHIP = 5;    // base attribute: chr
static const int PRO_SPELLCASTING = 6;  // base attribute: int
static const int PRO_MAGIC = 7;         // base attribute: int
static const int PRO_RANGED = 8;        // base attribute: dex
static const int PRO_SWORD = 9;         // base attribute: str
static const int PRO_MACE = 10;         // base attribute: str
static const int PRO_AXE = 11;          // base attribute: str
static const int PRO_POLEARM = 12;      // base attribute: str
static const int PRO_SHIELD = 13;       // base attribute: con
static const int NUMPROFICIENCIES = 14;

//Start levels for the various proficiency ranges.
//0 = "none"
static const int SKILL_LEVEL_NOVICE = 1;
static const int SKILL_LEVEL_BASIC = 20;
static const int SKILL_LEVEL_SKILLED = 40;
static const int SKILL_LEVEL_EXPERT = 60;
static const int SKILL_LEVEL_MASTER = 80;
static const int SKILL_LEVEL_LEGENDARY = 100;

static const int CAPSTONE_LOCKPICKING_UNLOCK = SKILL_LEVEL_LEGENDARY;
static const int CAPSTONE_UNLOCK_LEVEL[NUMPROFICIENCIES] =
{
		100,		//Lockpicking
		101,		//Stealth
		100,		//Trading
		100,		//Appraisal
		101,		//Swimming
		100,		//Leadership
		101,
		101,
		101,
		101,
		101,
		101,
		101,
		101
};

static const int CAPSTONE_LOCKPICKING_CHEST_GOLD_AMOUNT = 25;

static const int NUMCATEGORIES = 14;

#define ITEM_SLOT_NUMPROPERTIES 7
#define ITEM_SLOT_HELM 0
#define ITEM_SLOT_WEAPON ITEM_SLOT_HELM + ITEM_SLOT_NUMPROPERTIES
#define ITEM_SLOT_SHIELD ITEM_SLOT_WEAPON + ITEM_SLOT_NUMPROPERTIES
#define ITEM_SLOT_ARMOR ITEM_SLOT_SHIELD + ITEM_SLOT_NUMPROPERTIES
#define ITEM_SLOT_BOOTS ITEM_SLOT_ARMOR + ITEM_SLOT_NUMPROPERTIES
#define ITEM_SLOT_RING ITEM_SLOT_BOOTS + ITEM_SLOT_NUMPROPERTIES
#define ITEM_SLOT_AMULET ITEM_SLOT_RING + ITEM_SLOT_NUMPROPERTIES
#define ITEM_SLOT_CLOAK ITEM_SLOT_AMULET + ITEM_SLOT_NUMPROPERTIES
#define ITEM_SLOT_MASK ITEM_SLOT_CLOAK + ITEM_SLOT_NUMPROPERTIES
#define ITEM_SLOT_GLOVES ITEM_SLOT_MASK + ITEM_SLOT_NUMPROPERTIES
#define ITEM_SLOT_INV_1 ITEM_SLOT_GLOVES + ITEM_SLOT_NUMPROPERTIES
#define ITEM_SLOT_INV_2 ITEM_SLOT_INV_1 + ITEM_SLOT_NUMPROPERTIES
#define ITEM_SLOT_INV_3 ITEM_SLOT_INV_2 + ITEM_SLOT_NUMPROPERTIES
#define ITEM_SLOT_INV_4 ITEM_SLOT_INV_3 + ITEM_SLOT_NUMPROPERTIES
#define ITEM_SLOT_INV_5 ITEM_SLOT_INV_4 + ITEM_SLOT_NUMPROPERTIES
#define ITEM_SLOT_INV_6 ITEM_SLOT_INV_5 + ITEM_SLOT_NUMPROPERTIES
#define ITEM_CHANCE 5
#define ITEM_SLOT_CATEGORY 6
#define ITEM_CUSTOM_SLOT_LIMIT 6
#define ITEM_SLOT_NUM ITEM_SLOT_INV_6 + ITEM_SLOT_NUMPROPERTIES

typedef enum
{
	MALE,
	FEMALE
} sex_t;

class Stat
{
public:
	Monster type;
	sex_t sex;
	Uint32 appearance;
	char name[128];
	char obituary[128];
	Uint32 poisonKiller; // uid of the entity which killed me via burning/poison

	// attributes
	Sint32 HP, MAXHP, OLDHP;
	Sint32 MP, MAXMP;
	Sint32 STR, DEX, CON, INT, PER, CHR;
	Sint32 EXP, LVL;
	Sint32 GOLD, HUNGER;
	// randomised additional values to add to attributes
	Sint32 RANDOM_STR, RANDOM_DEX, RANDOM_CON, RANDOM_INT, RANDOM_PER, RANDOM_CHR;
	Sint32 RANDOM_MAXHP, RANDOM_HP, RANDOM_MAXMP, RANDOM_MP;
	Sint32 RANDOM_LVL;
	Sint32 RANDOM_GOLD;
	// flags to set for future entity behaviour
	Sint32 EDITOR_FLAGS[32];

	// skills and effects
	Sint32 PROFICIENCIES[NUMPROFICIENCIES];
	bool EFFECTS[NUMEFFECTS];
	Sint32 EFFECTS_TIMERS[NUMEFFECTS];
	bool defending;

	// group think
	Uint32 leader_uid;
	list_t FOLLOWERS;
	int stache_x1, stache_x2;
	int stache_y1, stache_y2;

	// equipment
	list_t inventory;
	Item* helmet;
	Item* breastplate;
	Item* gloves;
	Item* shoes;
	Item* shield;
	Item* weapon;
	Item* cloak;
	Item* amulet;
	Item* ring;
	Item* mask;

	// misc
#ifdef HAVE_FMOD
	FMOD_CHANNEL* monster_sound; //TODO: Do?
#else
	void* monster_sound;
#endif
	int monster_idlevar;

	list_t magic_effects; //Makes things like the invisibility spell work.
	Stat(Sint32 sprite);
	~Stat();
	void clearStats();
	void freePlayerEquipment();
	Stat* copyStats();
	void printStats();
	Sint32 EDITOR_ITEMS[ITEM_SLOT_NUM];
};
extern Stat* stats[MAXPLAYERS];

inline bool skillCapstoneUnlocked(int player, int proficiency)
{
	return (stats[player]->PROFICIENCIES[proficiency] >= CAPSTONE_UNLOCK_LEVEL[proficiency]);
}

void setDefaultMonsterStats(Stat* stats, int sprite);
