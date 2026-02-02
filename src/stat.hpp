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
#include <cassert>
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
static const int EFF_ENSEMBLE_FLUTE = 47;
static const int EFF_ENSEMBLE_LYRE = 48;
static const int EFF_ENSEMBLE_DRUM = 49;
static const int EFF_ENSEMBLE_LUTE = 50;
static const int EFF_ENSEMBLE_HORN = 51;
static const int EFF_LIFT = 52;
static const int EFF_GUARD_SPIRIT = 53;
static const int EFF_GUARD_BODY = 54;
static const int EFF_DIVINE_GUARD = 55;
static const int EFF_NIMBLENESS = 56;
static const int EFF_GREATER_MIGHT = 57;
static const int EFF_COUNSEL = 58;
static const int EFF_STURDINESS = 59;
static const int EFF_BLESS_FOOD = 60;
static const int EFF_PINPOINT = 61;
static const int EFF_PENANCE = 62;
static const int EFF_SACRED_PATH = 63;
static const int EFF_DETECT_ENEMY = 64;
static const int EFF_BLOOD_WARD = 65;
static const int EFF_TRUE_BLOOD = 66;
static const int EFF_DIVINE_ZEAL = 67;
static const int EFF_MAXIMISE = 68;
static const int EFF_MINIMISE = 69;
static const int EFF_WEAKNESS = 70;
static const int EFF_INCOHERENCE = 71;
static const int EFF_OVERCHARGE = 72;
static const int EFF_ENVENOM_WEAPON = 73;
static const int EFF_MAGIC_GREASE = 74;
static const int EFF_COMMAND = 75;
static const int EFF_MIMIC_VOID = 76;
static const int EFF_CURSE_FLESH = 77;
static const int EFF_NUMBING_BOLT = 78;
static const int EFF_DELAY_PAIN = 79;
static const int EFF_SEEK_CREATURE = 80;
static const int EFF_TABOO = 81;
static const int EFF_COURAGE = 82;
static const int EFF_COWARDICE = 83;
static const int EFF_SPORES = 84;
static const int EFF_ABUNDANCE = 85;
static const int EFF_GREATER_ABUNDANCE = 86;
static const int EFF_PRESERVE = 87;
static const int EFF_MIST_FORM = 88;
static const int EFF_FORCE_SHIELD = 89;
static const int EFF_LIGHTEN_LOAD = 90;
static const int EFF_ATTRACT_ITEMS = 91;
static const int EFF_RETURN_ITEM = 92;
static const int EFF_DEMESNE_DOOR = 93;
static const int EFF_REFLECTOR_SHIELD = 94;
static const int EFF_DIZZY = 95;
static const int EFF_SPIN = 96;
static const int EFF_CRITICAL_SPELL = 97;
static const int EFF_MAGIC_WELL = 98;
static const int EFF_STATIC = 99;
static const int EFF_ABSORB_MAGIC = 100;
static const int EFF_FLAME_CLOAK = 101;
static const int EFF_DUSTED = 102;
static const int EFF_NOISE_VISIBILITY = 103;
static const int EFF_RATION_SPICY = 104;
static const int EFF_RATION_SOUR = 105;
static const int EFF_RATION_BITTER = 106;
static const int EFF_RATION_HEARTY = 107;
static const int EFF_RATION_HERBAL = 108;
static const int EFF_RATION_SWEET = 109;
static const int EFF_GROWTH = 110;
static const int EFF_THORNS = 111;
static const int EFF_BLADEVINES = 112;
static const int EFF_BASTION_MUSHROOM = 113;
static const int EFF_BASTION_ROOTS = 114;
static const int EFF_FOCI_LIGHT_PEACE = 115;
static const int EFF_FOCI_LIGHT_JUSTICE = 116;
static const int EFF_FOCI_LIGHT_PROVIDENCE = 117;
static const int EFF_FOCI_LIGHT_PURITY = 118;
static const int EFF_FOCI_LIGHT_SANCTUARY = 119;
static const int EFF_STASIS = 120;
static const int EFF_HP_MP_REGEN = 121;
static const int EFF_DISRUPTED = 122;
static const int EFF_FROST = 123;
static const int EFF_MAGICIANS_ARMOR = 124;
static const int EFF_PROJECT_SPIRIT = 125;
static const int EFF_DEFY_FLESH = 126;
static const int EFF_PINPOINT_DAMAGE = 127;
static const int EFF_SALAMANDER_HEART = 128;
static const int EFF_DIVINE_FIRE = 129;
static const int EFF_HEALING_WORD = 130;
static const int EFF_HOLY_FIRE = 131;
static const int EFF_SIGIL = 132;
static const int EFF_SANCTUARY = 133;
static const int EFF_DUCKED = 134;
static const int NUMEFFECTS = 160;

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
static const int PRO_THAUMATURGY = 4;      // base attribute: con
static const int PRO_LEADERSHIP = 5;    // base attribute: chr
static const int PRO_MYSTICISM = 6;  // base attribute: int
static const int PRO_SORCERY = 7;         // base attribute: int
static const int PRO_RANGED = 8;        // base attribute: dex
static const int PRO_SWORD = 9;         // base attribute: str
static const int PRO_MACE = 10;         // base attribute: str
static const int PRO_AXE = 11;          // base attribute: str
static const int PRO_POLEARM = 12;      // base attribute: str
static const int PRO_SHIELD = 13;       // base attribute: con
static const int PRO_UNARMED = 14;       // base attribute: str
static const int PRO_ALCHEMY = 15;       // base attribute: int
static const int NUMPROFICIENCIES = 16;
static const int PRO_LEGACY_SWIMMING = 32; // for image lookups
static const int PRO_LEGACY_MAGIC = 33; // for image lookups
static const int PRO_LEGACY_SPELLCASTING = 34; // for image lookups

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
	BELL,
	MUSHROOM,
	LEAVES,
	DEATH_KNOCKBACK
};

class Stat
{
	Sint32 PROFICIENCIES[NUMPROFICIENCIES];
	Uint8 EFFECTS[NUMEFFECTS];
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
	static constexpr Uint8 nullEffectValue = 0;
	const Uint8& getEffectActive(int effect) const
	{
		if ( effect >= 0 && effect < NUMEFFECTS )
		{
			return EFFECTS[effect];
		}
		return Stat::nullEffectValue;
	}
	void clearEffect(int effect)
	{
		if ( effect >= 0 && effect < NUMEFFECTS )
		{
			EFFECTS[effect] = 0;
		}
	}
	void setEffectActive(int effect, Uint8 effectStrength)
	{
#ifndef EDITOR
		assert(effectStrength > 0);
#endif
		if ( effect >= 0 && effect < NUMEFFECTS )
		{
			EFFECTS[effect] = std::max(EFFECTS[effect], effectStrength); // strongest value remains
		}
	}
	void setEffectValueUnsafe(int effect, Uint8 effectStrength)
	{
		if ( effect >= 0 && effect < NUMEFFECTS )
		{
			EFFECTS[effect] = effectStrength;
		}
	}
	Uint32 EFFECTS_ACCRETION_TIME[NUMEFFECTS];
	Sint32 EFFECTS_TIMERS[NUMEFFECTS];
	bool defending;
	Uint32 parrying = 0;
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
	list_t void_chest_inventory;
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
	static int getParryingACBonus(Stat* myStats, Item* myWeapon, bool checkWeapon, bool excludeSkill, int weaponSkill);
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
	int numShillelaghDebuffsActive(Entity* my);
	void addItemToLootingBag(const int player, const real_t x, const real_t y, Item& item);
	Uint32 getLootingBagKey(const int player);
	static bool emptyLootingBag(const int player, Uint32 key);
	static int maxEquipmentBonusToSkill;
	enum EnsembleEffectsBonusType
	{
		ENSEMBLE_FLUTE_EFF_1,
		ENSEMBLE_FLUTE_EFF_2,
		ENSEMBLE_FLUTE_TIER,
		ENSEMBLE_LUTE_EFF_1,
		ENSEMBLE_LUTE_EFF_2,
		ENSEMBLE_LUTE_TIER,
		ENSEMBLE_DRUM_EFF_1,
		ENSEMBLE_DRUM_EFF_2,
		ENSEMBLE_DRUM_TIER,
		ENSEMBLE_HORN_EFF_1,
		ENSEMBLE_HORN_EFF_2,
		ENSEMBLE_HORN_TIER,
		ENSEMBLE_LYRE_EFF_1,
		ENSEMBLE_LYRE_EFF_2,
		ENSEMBLE_LYRE_TIER,
		ENSEMBLE_LYRE_TIER_2
	};
	static const Sint32 kEnsembleBreakPointTier4 = 40;
	static const Sint32 kEnsembleBreakPointTier3 = 20;
	static const Sint32 kEnsembleBreakPointTier2 = 5;
	static const Sint32 kEnsembleBreakPointTier1 = 0;
	real_t getEnsembleEffectBonus(EnsembleEffectsBonusType bonusType, int checkEffectStrength = -1);
	Sint32 getThaumProficiencySpellStatBonus(int whichStat, Sint32 currentBonus);
	static int getMaxAttackCharge(Stat* myStats);
	struct MonsterRangedAccuracy
	{
		Uint32 lastTarget = 0;
		real_t accuracy = 0.0;
		Uint32 lastTick = 0;
		real_t getAccuracy(Uint32 target);
		void incrementAccuracy();
		void modifyProjectile(Entity& my, Entity& projectile);
	};
	MonsterRangedAccuracy monsterRangedAccuracy;
	std::map<ItemType, Uint32> itemLastDegradeTick;
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
