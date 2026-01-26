/*-------------------------------------------------------------------------------

	BARONY
	File: stat.hpp
	Desc: header for stat.cpp (contains Stat class definition)

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#ifdef USE_FMOD
#include <fmod.hpp>
#endif

#include "items.hpp"

enum Monster : int;


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
static const int EFF_VAMPIRICAURA = 18;
static const int EFF_SHRINE_RED_BUFF = 19;
static const int EFF_SHRINE_GREEN_BUFF = 20;
static const int EFF_SHRINE_BLUE_BUFF = 21;
static const int EFF_HP_REGEN = 22;
static const int EFF_MP_REGEN = 23;
static const int EFF_PACIFY = 24;
static const int EFF_POLYMORPH = 25;
static const int EFF_KNOCKBACK = 26;
static const int EFF_WITHDRAWAL = 27;
static const int EFF_POTION_STR = 28;
static const int EFF_SHAPESHIFT = 29;
static const int EFF_WEBBED = 30;
static const int EFF_FEAR = 31;
static const int EFF_MAGICAMPLIFY = 32;
static const int EFF_DISORIENTED = 33;
static const int EFF_SHADOW_TAGGED = 34;
static const int EFF_TROLLS_BLOOD = 35;
static const int EFF_FLUTTER = 36;
static const int EFF_DASH = 37;
static const int EFF_DISTRACTED_COOLDOWN = 38;
static const int EFF_MIMIC_LOCKED = 39;
static const int EFF_ROOTED = 40;
static const int EFF_NAUSEA_PROTECTION = 41;
static const int EFF_CON_BONUS = 42;
static const int EFF_PWR = 43;
static const int EFF_AGILITY = 44;
static const int EFF_RALLY = 45;
static const int EFF_MARIGOLD = 46;
static const int NUMEFFECTS = 64;

// stats
static const int STAT_STR = 0;
static const int STAT_DEX = 1;
static const int STAT_CON = 2;
static const int STAT_INT = 3;
static const int STAT_PER = 4;
static const int STAT_CHR = 5;

static const int NUMSTATS = 6;

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
static const int PRO_UNARMED = 14;       // base attribute: str
static const int PRO_ALCHEMY = 15;       // base attribute: int
static const int NUMPROFICIENCIES = 16;

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
		100,		//Stealth
		100,		//Trading
		100,		//Appraisal
		100,		//Swimming
		100,		//Leadership
		100,		//Spellcasting
		100,		//Magic
		100,		//Ranged
		100,		//Sword
		100,		//Mace
		100,		//Axe
		100,		//Polearm
		100,		//Shield
		100,		//Unarmed
		100			//Alchemy
};

static const int CAPSTONE_LOCKPICKING_CHEST_GOLD_AMOUNT = 100;

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

//--Stat Flag constants--
static const int STAT_FLAG_NPC = 0;
static const int STAT_FLAG_SNEAK = 1;
static const int STAT_FLAG_ALLY_PICKUP = 2;
static const int STAT_FLAG_ALLY_CLASS = 3;
static const int STAT_FLAG_PLAYER_RACE = 4;
static const int STAT_FLAG_POLYMORPH_STORAGE = 5;
static const int STAT_FLAG_ALLY_SUMMON_LVLHP = 6;
static const int STAT_FLAG_ALLY_SUMMON_STRDEXCONINT = 7;
static const int STAT_FLAG_ALLY_SUMMON_PERCHR = 8;
static const int STAT_FLAG_ALLY_SUMMON2_LVLHP = 9;
static const int STAT_FLAG_ALLY_SUMMON2_STRDEXCONINT = 10;
static const int STAT_FLAG_ALLY_SUMMON2_PERCHR = 11;
static const int STAT_FLAG_MYSTERIOUS_SHOPKEEP = 16;
static const int STAT_FLAG_NO_DROP_ITEMS = 19;
static const int STAT_FLAG_FORCE_ALLEGIANCE_TO_PLAYER = 20;
static const int STAT_FLAG_DISABLE_MINIBOSS = 21;
static const int STAT_FLAG_XP_PERCENT_AWARD = 22;
static const int STAT_FLAG_MONSTER_CAST_INVENTORY_SPELLBOOKS = 23;
static const int STAT_FLAG_SHOPKEEPER_CUSTOM_PROPERTIES = 24;
static const int STAT_FLAG_MONSTER_NAME_GENERIC = 25;
static const int STAT_FLAG_MONSTER_DISABLE_HC_SCALING = 26;
static const int STAT_FLAG_HP_BONUS = 27;
static const int STAT_FLAG_MP_BONUS = 28;
static const int STAT_FLAG_ASSISTANCE_PLAYER_PTS = 29;

typedef enum
{
	MALE = 0,
	FEMALE = 1
} sex_t;

enum KilledBy {
    UNKNOWN,
    MONSTER,
    ITEM,
    ALLY_BETRAYAL,
    ATTEMPTED_ROBBERY,
    TRESPASSING,
    TRAP_ARROW,
    TRAP_BEAR,
    TRAP_SPIKE,
    TRAP_MAGIC,
    TRAP_BOMB,
    BOULDER,
    LAVA,
    WATER,
    FAILED_INVOCATION,
    STARVATION,
    POISON,
    BLEEDING,
    BURNING_TO_CRISP,
    STRANGULATION,
    FUNNY_POTION,
    BOTTOMLESS_PIT,
    NO_FUEL,
    FOUNTAIN,
    SINK,
    FAILED_ALCHEMY,
	FAILED_CHALLENGE,
	BELL
};

class Stat
{
	Sint32 PROFICIENCIES[NUMPROFICIENCIES];
public:
	Monster type;
	sex_t sex;
	Uint32 stat_appearance = 0;
	char name[128];

	// uid of the entity which killed me via burning/poison (for rewarding XP to them)
	Uint32 poisonKiller;

	// Obituary stuff
	char obituary[128];
	KilledBy killer = KilledBy::UNKNOWN;
	Uint32 killer_uid = 0;
	Monster killer_monster;
	ItemType killer_item;
	std::string killer_name = "";

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
	Sint32 MISC_FLAGS[32];

	// flags for player stats only
	Sint32 PLAYER_LVL_STAT_BONUS[NUMSTATS];
	Sint32 PLAYER_LVL_STAT_TIMER[NUMSTATS * 2];

	// skills and effects
	Sint32 getProficiency(int skill) const
	{
		if ( skill >= 0 && skill < NUMPROFICIENCIES )
		{
			return PROFICIENCIES[skill];
		}
		return 0;
	}
	Sint32 getModifiedProficiency(int skill) const;
	void setProficiency(int skill, int value)
	{
		if ( skill >= 0 && skill < NUMPROFICIENCIES )
		{
			PROFICIENCIES[skill] = std::min(std::max(0, value), 100);
		}
	}
	void setProficiencyUnsafe(int skill, int value)
	{
		PROFICIENCIES[skill] = value;
	}
	int getGoldWeight() const;
	bool EFFECTS[NUMEFFECTS];
	Sint32 EFFECTS_TIMERS[NUMEFFECTS];
	bool defending;
	Sint32& sneaking; // MISC_FLAGS[1]
	Sint32& allyItemPickup; // MISC_FLAGS[2]
	Sint32& allyClass; // MISC_FLAGS[3]
	Sint32& playerRace; // MISC_FLAGS[4]
	Sint32& playerPolymorphStorage; // MISC_FLAGS[5]
	Sint32& playerSummonLVLHP; // MISC_FLAGS[6]
	Sint32& playerSummonSTRDEXCONINT; // MISC_FLAGS[7]
	Sint32& playerSummonPERCHR; // MISC_FLAGS[8]
	Sint32& playerSummon2LVLHP; // MISC_FLAGS[9]
	Sint32& playerSummon2STRDEXCONINT; // MISC_FLAGS[10]
	Sint32& playerSummon2PERCHR; // MISC_FLAGS[11]
	Sint32& monsterIsCharmed; // MISC_FLAGS[12]
	Sint32& playerShapeshiftStorage; // MISC_FLAGS[13]
	Sint32& monsterTinkeringStatus; // MISC_FLAGS[14]
	Sint32& monsterMimicLockedBy; // MISC_FLAGS[14]
	Sint32& monsterDemonHasBeenExorcised; // MISC_FLAGS[15]
	Sint32& bleedInflictedBy; // MISC_FLAGS[17]
	Sint32& burningInflictedBy; // MISC_FLAGS[18]
	Sint32& monsterNoDropItems; // MISC_FLAGS[19]
	Sint32& monsterForceAllegiance; // MISC_FLAGS[20]

	// group think
	Uint32 leader_uid;
	list_t FOLLOWERS;

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
#ifdef USE_FMOD
	FMOD::Channel* monster_sound;
#else
	void* monster_sound;
#endif
	int monster_idlevar;
	std::map<std::string, std::string> attributes;
	struct Lootbag_t
	{
		int spawn_x = 0;
		int spawn_y = 0;
		bool spawnedOnGround = false;
		bool looted = false;
		std::vector<Item> items;
	};
	std::map<Uint32, Lootbag_t> player_lootbags;
	list_t magic_effects; //Makes things like the invisibility spell work.
	Stat(Sint32 sprite);
	~Stat();
	void clearStats();
	void freePlayerEquipment();
	Stat* copyStats();
	void copyNPCStatsAndInventoryFrom(Stat& src);
	void printStats();
	Sint32 EDITOR_ITEMS[ITEM_SLOT_NUM];
	int pickRandomEquippedItemToDegradeOnHit(Item** returnItem, bool excludeWeapon, bool excludeShield, bool excludeArmor, bool excludeJewelry);
	int pickRandomEquippedItem(Item** returnItem, bool excludeWeapon, bool excludeShield, bool excludeArmor, bool excludeJewelry);
	enum MonsterForceAllegiance : int
	{
		MONSTER_FORCE_ALLEGIANCE_NONE = 0,
		MONSTER_FORCE_PLAYER_ALLY,
		MONSTER_FORCE_PLAYER_ENEMY,
		MONSTER_FORCE_PLAYER_RECRUITABLE
	};
	int getPassiveShieldBonus(bool checkShield, bool excludeSkill) const;
	int getActiveShieldBonus(bool checkShield, bool excludeSkill, Item* shieldItem = nullptr, bool checkNonShieldBonus = false) const;
	std::string getAttribute(std::string key) const
	{ 
		if ( attributes.find(key) != attributes.end() )
		{
			return attributes.at(key);
		}
		else
		{
			return "";
		}
	}
	void setAttribute(std::string key, std::string value);
	bool statusEffectRemovedByCureAilment(const int effect, Entity* my);
	void addItemToLootingBag(const int player, const real_t x, const real_t y, Item& item);
	Uint32 getLootingBagKey(const int player);
	static bool emptyLootingBag(const int player, Uint32 key);
	static int maxEquipmentBonusToSkill;
};
extern Stat* stats[MAXPLAYERS];

inline bool skillCapstoneUnlocked(int player, int proficiency)
{
	return (stats[player]->getModifiedProficiency(proficiency) >= CAPSTONE_UNLOCK_LEVEL[proficiency]);
}
static const int MAX_PLAYER_STAT_VALUE = 248;
void setDefaultMonsterStats(Stat* stats, int sprite);
bool isMonsterStatsDefault(Stat& myStats);
const char* getSkillLangEntry(int skill);
