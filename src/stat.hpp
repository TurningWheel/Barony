/*-------------------------------------------------------------------------------

	BARONY
	File: stat.hpp
	Desc: header for stat.cpp (contains Stat class definition)

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include "fmod.h"

class Item;
//enum Item;
//enum Status;
//enum Category;

typedef enum {
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
	SHOPKEEPER
} Monster;
#define NUMMONSTERS 21
extern int kills[NUMMONSTERS];

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
#define PRO_LOCKPICKING 0   // base attribute: dex
#define PRO_STEALTH 1       // base attribute: dex
#define PRO_TRADING 2       // base attribute: chr
#define PRO_APPRAISAL 3     // base attribute: per
#define PRO_SWIMMING 4      // base attribute: con
#define PRO_LEADERSHIP 5    // base attribute: chr
#define PRO_SPELLCASTING 6  // base attribute: int
#define PRO_MAGIC 7         // base attribute: int
#define PRO_RANGED 8        // base attribute: dex
#define PRO_SWORD 9         // base attribute: str
#define PRO_MACE 10         // base attribute: str
#define PRO_AXE 11          // base attribute: str
#define PRO_POLEARM 12      // base attribute: str
#define PRO_SHIELD 13       // base attribute: con
const int NUMPROFICIENCIES = 14;

#define NUMCATEGORIES 13

typedef enum {
	MALE,
	FEMALE
} sex_t;

class Stat {
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
		Item *helmet;
		Item *breastplate;
		Item *gloves;
		Item *shoes;
		Item *shield;
		Item *weapon;
		Item *cloak;
		Item *amulet;
		Item *ring;
		Item *mask;

		// misc
		FMOD_CHANNEL *monster_sound; //TODO: Do?
		int monster_idlevar;

		list_t magic_effects; //Makes things like the invisibility spell work.
		Stat();
		~Stat();
		void clearStats();
		void freePlayerEquipment();
		Stat *copyStats();
		void printStats();
};
extern Stat *stats[MAXPLAYERS];