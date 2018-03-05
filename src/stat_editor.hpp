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
//enum Item;
//enum Status;
//enum Category;

typedef enum
{
	NOTHING,
	HUMAN,
	RAT,
	GOBLIN,
	SLIME,
	TROLL,
	OCTOPUS,
	SPIDER,
	GHOUL,
	SKELETON,
	SCORPION,
	CREATURE_IMP, //Because Apple so unkindly is already using the IMP keyword.
	BUGBEAR,
	GNOME,
	DEMON,
	SUCCUBUS,
	MIMIC,
	LICH,
	MINOTAUR,
	DEVIL,
	SHOPKEEPER,
	KOBOLD,
	SCARAB,
	CRYSTALGOLEM,
	INCUBUS,
	VAMPIRE,
	SHADOW,
	COCKATRICE,
	INSECTOID,
	GOATMAN,
	AUTOMATON,
	LICH_ICE,
	LICH_FIRE
} Monster;
#define NUMMONSTERS 33
extern int kills[NUMMONSTERS];

// effects
static const int EFF_ASLEEP = 0;
static const int EFF_POISONED = 1;
static const int EFF_STUNNED = 2;
static const int EFF_CONFUSED = 3;
static const int EFF_DRUNK = 4;
static const int EFF_INVISIBLE = 5;
static const int EFF_BLIND = 6;
static const int EFF_GREASY = 7;
static const int EFF_MESSY = 8;
static const int EFF_FAST = 9;
static const int EFF_PARALYZED = 10;
static const int EFF_LEVITATING = 11;
static const int EFF_TELEPATH = 12;
static const int EFF_VOMITING = 13;
static const int EFF_BLEEDING = 14;
static const int EFF_SLOW = 15;
static const int EFF_MAGICRESIST = 16;
static const int EFF_MAGICREFLECT = 17;
static const int NUMEFFECTS = 32;

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

#define NUMCATEGORIES 13

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
	Stat();
	~Stat();
	void clearStats();
	void freePlayerEquipment();
	Stat* copyStats();
	void printStats();
};
//extern Stat* stats[MAXPLAYERS];