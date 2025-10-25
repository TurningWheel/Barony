/*-------------------------------------------------------------------------------

	BARONY
	File: magic.hpp
	Desc: Defines magic related stuff.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

//TODO: Spell icons: http://game-icons.net/

#pragma once
#include "../interface/interface.hpp"

class Stat;

static const int SPELLCASTING_BEGINNER = 40; //If the player's spellcasting skill is below this, they're a newbie and will suffer various penalties to their spellcasting.

static const int SPELL_NONE = 0; //This define is not meant to be used. Rather, it is to signify that a spell type of 0 means no spell, which is of particular use in the Spell struct.
static const int SPELL_FORCEBOLT = 1;
static const int SPELL_MAGICMISSILE = 2;
static const int SPELL_COLD = 3;
static const int SPELL_FIREBALL = 4;
static const int SPELL_LIGHTNING = 5;
static const int SPELL_REMOVECURSE = 6;
static const int SPELL_LIGHT = 7;
static const int SPELL_IDENTIFY = 8;
static const int SPELL_MAGICMAPPING = 9;
static const int SPELL_SLEEP = 10;
static const int SPELL_CONFUSE = 11;
static const int SPELL_SLOW = 12;
static const int SPELL_OPENING = 13;
static const int SPELL_LOCKING = 14;
static const int SPELL_LEVITATION = 15;
static const int SPELL_INVISIBILITY = 16;
static const int SPELL_TELEPORTATION = 17;
static const int SPELL_HEALING = 18;
static const int SPELL_EXTRAHEALING = 19;
//#define SPELL_RESTOREABILITY 20
static const int SPELL_CUREAILMENT = 20;
static const int SPELL_DIG = 21;
static const int SPELL_SUMMON = 22;
static const int SPELL_STONEBLOOD = 23;
static const int SPELL_BLEED = 24;
static const int SPELL_DOMINATE = 25;
static const int SPELL_REFLECT_MAGIC = 26;
static const int SPELL_ACID_SPRAY = 27;
static const int SPELL_STEAL_WEAPON = 28;
static const int SPELL_DRAIN_SOUL = 29;
static const int SPELL_VAMPIRIC_AURA = 30;
static const int SPELL_CHARM_MONSTER = 31;
static const int SPELL_REVERT_FORM = 32;
static const int SPELL_RAT_FORM = 33;
static const int SPELL_SPIDER_FORM = 34;
static const int SPELL_TROLL_FORM = 35;
static const int SPELL_IMP_FORM = 36;
static const int SPELL_SPRAY_WEB = 37;
static const int SPELL_POISON = 38;
static const int SPELL_SPEED = 39;
static const int SPELL_FEAR = 40;
static const int SPELL_STRIKE = 41;
static const int SPELL_DETECT_FOOD = 42;
static const int SPELL_WEAKNESS = 43;
static const int SPELL_AMPLIFY_MAGIC = 44;
static const int SPELL_SHADOW_TAG = 45;
static const int SPELL_TELEPULL = 46;
static const int SPELL_DEMON_ILLUSION = 47;
static const int SPELL_TROLLS_BLOOD = 48;
static const int SPELL_SALVAGE = 49;
static const int SPELL_FLUTTER = 50;
static const int SPELL_DASH = 51;
static const int SPELL_SELF_POLYMORPH = 52;
static const int SPELL_CRAB_FORM = 53;
static const int SPELL_CRAB_WEB = 54;
static const int SPELL_GHOST_BOLT = 55;
static const int SPELL_SLIME_ACID = 56;
static const int SPELL_SLIME_WATER = 57;
static const int SPELL_SLIME_FIRE = 58;
static const int SPELL_SLIME_TAR = 59;
static const int SPELL_SLIME_METAL = 60;
static const int SPELL_FOCI_FIRE = 61;
static const int SPELL_FOCI_SNOW = 62;
static const int SPELL_FOCI_NEEDLES = 63;
static const int SPELL_FOCI_ARCS = 64;
static const int SPELL_FOCI_SANDBLAST = 65;
static const int SPELL_METEOR = 66;
static const int SPELL_FLAMES = 67;
static const int SPELL_ICE_WAVE = 68;
static const int SPELL_CONJURE_FOOD = 69;
static const int SPELL_NULL_MELEE = 70;
static const int SPELL_NULL_MAGIC = 71;
static const int SPELL_NULL_RANGED = 72;
static const int SPELL_PROF_NIMBLENESS = 73;
static const int SPELL_PROF_GREATER_MIGHT = 74;
static const int SPELL_PROF_COUNSEL = 75;
static const int SPELL_PROF_STURDINESS = 76;
static const int SPELL_BLESS_FOOD = 77;
static const int SPELL_PINPOINT = 78;
static const int SPELL_DONATION = 79;
static const int SPELL_SCRY_ALLIES = 80;
static const int SPELL_SCRY_SHRINES = 81;
static const int SPELL_SCRY_TRAPS = 82;
static const int SPELL_SCRY_TREASURES = 83;
static const int SPELL_PENANCE = 84;
static const int SPELL_CALL_ALLIES = 85;
static const int SPELL_SACRED_PATH = 86;
static const int SPELL_MANIFEST_DESTINY = 87;
static const int SPELL_DETECT_ENEMY = 88;
static const int SPELL_DETECT_ENEMIES = 89;
static const int SPELL_TURN_UNDEAD = 90;
static const int SPELL_HEAL_OTHER = 91;
static const int SPELL_BLOOD_WARD = 92;
static const int SPELL_TRUE_BLOOD = 93;
static const int SPELL_DIVINE_ZEAL = 94;
static const int SPELL_ALTER_INSTRUMENT = 95;
static const int SPELL_MAXIMISE = 96;
static const int SPELL_MINIMISE = 97;
static const int SPELL_JUMP = 98;
static const int SPELL_INCOHERENCE = 99;
static const int SPELL_OVERCHARGE = 100;
static const int SPELL_ENVENOM_WEAPON = 101;
static const int SPELL_HUMILIATE = 102;
static const int SPELL_LVL_DEATH = 103;
static const int SPELL_GREASE_SPRAY = 104;
static const int SPELL_MANA_BURST = 105;
static const int SPELL_BOOBY_TRAP = 106;
static const int SPELL_COMMAND = 107;
static const int SPELL_METALLURGY = 108;
static const int SPELL_GEOMANCY = 109;
static const int SPELL_FORGE_KEY = 110;
static const int SPELL_FORGE_JEWEL = 111;
static const int SPELL_ENHANCE_WEAPON = 112;
static const int SPELL_RESHAPE_WEAPON = 113;
static const int SPELL_ALTER_ARROW = 114;
static const int SPELL_VOID_CHEST = 115;
static const int SPELL_PUNCTURE_VOID = 116;
static const int SPELL_LEAD_BOLT = 117;
static const int SPELL_MERCURY_BOLT = 118;
static const int SPELL_NUMBING_BOLT = 119;
static const int SPELL_DELAY_PAIN = 120;
static const int SPELL_CURSE_FLESH = 121;
static const int SPELL_REVENANT_CURSE = 122;
static const int SPELL_COWARDICE = 123;
static const int SPELL_COURAGE = 124;
static const int SPELL_SEEK_ALLY = 125;
static const int SPELL_SEEK_FOE = 126;
static const int SPELL_DEEP_SHADE = 127;
static const int SPELL_SHADE_BOLT = 128;
static const int SPELL_SPIRIT_WEAPON = 129;
static const int SPELL_ADORCISM = 130;
static const int SPELL_TABOO = 131;
static const int SPELL_WONDERLIGHT = 132;
static const int SPELL_SPORES = 133;
static const int SPELL_SPORE_BOMB = 134;
static const int SPELL_WINDGATE = 135;
static const int SPELL_VORTEX = 136;
static const int SPELL_TELEKINESIS = 137;
static const int SPELL_KINETIC_PUSH = 138;
static const int SPELL_DISARM = 139;
static const int SPELL_STRIP = 140;
static const int SPELL_ABUNDANCE = 141;
static const int SPELL_GREATER_ABUNDANCE = 142;
static const int SPELL_PRESERVE = 143;
static const int SPELL_RESTORE = 144;
static const int SPELL_SABOTAGE = 145;
static const int SPELL_HARVEST_TRAP = 146;
static const int SPELL_MIST_FORM = 147;
static const int SPELL_HOLOGRAM = 148;
static const int SPELL_FORCE_SHIELD = 149;
static const int SPELL_REFLECTOR = 150;
static const int SPELL_SPLINTER_GEAR = 151;
static const int SPELL_LIGHTEN_LOAD = 152;
static const int SPELL_ATTRACT_ITEMS = 153;
static const int SPELL_RETURN_ITEMS = 154;
static const int SPELL_ABSORB_MAGIC = 155;
static const int SPELL_SEIZE_MAGIC = 156;
static const int SPELL_DEFACE = 157;
static const int SPELL_SUNDER_MONUMENT = 158;
static const int SPELL_DEMESNE_DOOR = 159;
static const int SPELL_TUNNEL = 160;
static const int SPELL_NULL_AREA = 161;
static const int SPELL_SPHERE_SILENCE = 162;
static const int SPELL_FORGE_METAL_SCRAP = 163;
static const int SPELL_FORGE_MAGIC_SCRAP = 164;
static const int SPELL_FIRE_SPRITE = 165;
static const int SPELL_FLAME_ELEMENTAL = 166;
static const int SPELL_SPIN = 167;
static const int SPELL_DIZZY = 168;
static const int SPELL_VANDALISE = 169;
static const int SPELL_DESECRATE = 170;
static const int SPELL_SANCTIFY = 171;
static const int SPELL_SANCTIFY_WATER = 172;
static const int SPELL_CLEANSE_FOOD = 173;
static const int SPELL_ADORCISE_INSTRUMENT = 174;
static const int SPELL_FLAME_CLOAK = 175;
static const int SPELL_CRITICAL_SPELL = 176;
static const int SPELL_MAGIC_WELL = 177;
static const int SPELL_FLAME_SHIELD = 178;
static const int SPELL_LIGHTNING_BOLT = 179;
static const int SPELL_DISRUPT_EARTH = 180;
static const int SPELL_EARTH_SPINES = 181;
static const int SPELL_LIGHTNING_NEXUS = 182;
static const int SPELL_FIRE_WALL = 183;
static const int SPELL_LIFT = 184;
static const int SPELL_SLAM = 185;
static const int SPELL_IGNITE = 186;
static const int SPELL_SHATTER_OBJECTS = 187;
static const int SPELL_KINETIC_FIELD = 188;
static const int SPELL_ICE_BLOCK = 189;
static const int SPELL_METEOR_SHOWER = 190;
static const int SPELL_CHRONOMIC_FIELD = 191;
static const int SPELL_ETERNALS_GAZE = 192;
static const int SPELL_SHATTER_EARTH = 193;
static const int SPELL_EARTH_ELEMENTAL = 194;
static const int SPELL_ROOTS = 195;
static const int SPELL_MUSHROOM = 196;
static const int SPELL_MYCELIUM_BOMB = 197;
static const int SPELL_MYCELIUM_SPORES = 198;
static const int SPELL_HEAL_PULSE = 199;
static const int SPELL_SHRUB = 200;
static const int SPELL_THORNS = 201;
static const int SPELL_BLADEVINES = 202;
static const int SPELL_BASTION_MUSHROOM = 203;
static const int SPELL_BASTION_ROOTS = 204;
static const int SPELL_FOCI_DARK_LIFE = 205;
static const int SPELL_FOCI_DARK_RIFT = 206;
static const int SPELL_FOCI_DARK_SILENCE = 207;
static const int SPELL_FOCI_DARK_VENGEANCE = 208;
static const int SPELL_FOCI_DARK_SUPPRESS = 209;
static const int SPELL_FOCI_LIGHT_PEACE = 210;
static const int SPELL_FOCI_LIGHT_JUSTICE = 211;
static const int SPELL_FOCI_LIGHT_PROVIDENCE = 212;
static const int SPELL_FOCI_LIGHT_PURITY = 213;
static const int SPELL_FOCI_LIGHT_SANCTUARY = 214;
static const int SPELL_SCEPTER_BLAST = 215;
static const int NUM_SPELLS = 225;

#define SPELLELEMENT_CONFUSE_BASE_DURATION 2//In seconds.
#define SPELLELEMENT_BLEED_BASE_DURATION 10//In seconds.
#define SPELLELEMENT_STONEBLOOD_BASE_DURATION 5//In seconds.
static const int SPELLELEMENT_ACIDSPRAY_BASE_DURATION = 5;

//Definitions for actMagic(note that other functions may use this)
#define MAGIC_TYPE (Item)my->skill[10] //TODO: OLD.
#define MAGIC_VELX my->fskill[0]
#define MAGIC_VELY my->fskill[1]
#define MAGIC_USED my->skill[3]
#define MAGIC_LIFE my->skill[4]
#define MAGIC_MAXLIFE my->skill[5]
#define MAGIC_MAXLIFE_DEF 200

#define CASTING_EXTRA_TIMES_CAP 3 //Max numbers of time to circle hands in the animation.

//Definitions for actMagiclightBall() (note that other functions may use this)
#define magic_init my->skill[0]
#define lightball_orbit_angle my->skill[1]
#define lightball_orbit_length my->skill[3]
#define lightball_player_startx my->skill[4] //The x the player started moving from. If !init, this is used for the x it started moving out from the player at. //TODO: Change this so that it doesn't conflict with the spell duration variable.
#define lightball_player_starty my->skill[5] //The y the player started moving from. If !init, this is used for the y it started moving out from the player at.
#define lightball_movement_timer my->skill[6] //Don't remember what this is for.
#define lightball_hover_basez my->skill[7] //Used for the hover bobbing up and down.
#define lightball_hoverangle my->skill[8]
#define lightball_player_lastmove_timer my->skill[9] //The number of MS it's been since the player last moved. Used for when it starts orbiting the player.
#define lightball_flicker my->skill[10]
#define lightball_lighting  my->skill[11]
#define lightball_timer my->skill[12] //How long it has left to live.
#define LIGHTBALL_MOVE_DELAY 15 //How long the lightball is to delay before starting to follow the player.
#define LIGHTBALL_HOVER_HIGHPEAK (-1.0f)
#define LIGHTBALL_HOVER_LOWPEAK (1.0f)
#define MAGICLIGHT_BALL_FOLLOW_DISTANCE 12
#define LIGHTBALL_CIRCLE_TIME 180
#define MAGIC_LIGHTBALL_SPEEDLIMIT 10.0f
#define MAGIC_LIGHTBALL_TURNSPEED 5
#define MAGICLIGHTBALL_DIVIDE_CONSTANT 25

#define MAGICSTAFF_LIGHT_DURATION 6000

#define HEAL_RADIUS 128

/*** misc effect particles ***/
static const int PARTICLE_EFFECT_ABILITY_ROCK = 1;
static const int PARTICLE_EFFECT_ABILITY_PURPLE = 2;
static const int PARTICLE_EFFECT_SAP = 3;
static const int PARTICLE_EFFECT_SHADOW_INVIS = 4;
static const int PARTICLE_EFFECT_INCUBUS_TELEPORT_STEAL = 5;
static const int PARTICLE_EFFECT_INCUBUS_TELEPORT_TARGET = 6;
static const int PARTICLE_EFFECT_ERUPT = 7;
static const int PARTICLE_EFFECT_VAMPIRIC_AURA = 8;
static const int PARTICLE_EFFECT_RISING_DROP = 9;
static const int PARTICLE_EFFECT_PORTAL_SPAWN = 10;
static const int PARTICLE_EFFECT_SHADOW_TELEPORT = 11;
static const int PARTICLE_EFFECT_LICHFIRE_TELEPORT_STATIONARY = 12;
static const int PARTICLE_EFFECT_LICH_TELEPORT_ROAMING = 13;
static const int PARTICLE_EFFECT_LICHICE_TELEPORT_STATIONARY = 14;
static const int PARTICLE_EFFECT_SUMMON_MONSTER = 15;
static const int PARTICLE_EFFECT_CHARM_MONSTER = 16;
static const int PARTICLE_EFFECT_SPELL_SUMMON = 17;
static const int PARTICLE_EFFECT_SPELL_WEB_ORBIT = 18;
static const int PARTICLE_EFFECT_TELEPORT_PULL = 19;
static const int PARTICLE_EFFECT_TELEPORT_PULL_TARGET_LOCATION = 20;
static const int PARTICLE_EFFECT_SHADOW_TAG = 21;
static const int PARTICLE_EFFECT_SPELLBOT_ORBIT = 22;
static const int PARTICLE_EFFECT_PLAYER_AUTOMATON_DEATH = 23;
static const int PARTICLE_EFFECT_DEVIL_SUMMON_MONSTER = 24;
static const int PARTICLE_EFFECT_SHATTERED_GEM = 25;
static const int PARTICLE_EFFECT_SHRINE_TELEPORT = 26;
static const int PARTICLE_EFFECT_GHOST_TELEPORT = 27;
static const int PARTICLE_EFFECT_SLIME_SPRAY = 28;
static const int PARTICLE_EFFECT_FOCI_SPRAY = 29;
static const int PARTICLE_EFFECT_ENSEMBLE_SELF_CAST = 30;
static const int PARTICLE_EFFECT_ENSEMBLE_OTHER_CAST = 31;
static const int PARTICLE_EFFECT_STATIC_ORBIT = 32;
static const int PARTICLE_EFFECT_LIGHTNING_SEQ = 33;
static const int PARTICLE_EFFECT_PINPOINT = 34;
static const int PARTICLE_EFFECT_DESTINY_TELEPORT = 35;
static const int PARTICLE_EFFECT_BOOBY_TRAP = 36;
static const int PARTICLE_EFFECT_SPORE_BOMB = 37;
static const int PARTICLE_EFFECT_WINDGATE = 38;
static const int PARTICLE_EFFECT_DEMESNE_DOOR = 39;
static const int PARTICLE_EFFECT_NULL_PARTICLE = 40;
static const int PARTICLE_EFFECT_VORTEX_ORBIT = 41;
static const int PARTICLE_EFFECT_SPIN = 42;
static const int PARTICLE_EFFECT_AREA_EFFECT = 43;
static const int PARTICLE_EFFECT_ETERNALS_GAZE1 = 44;
static const int PARTICLE_EFFECT_ETERNALS_GAZE2 = 45;
static const int PARTICLE_EFFECT_ETERNALS_GAZE_STATIC = 46;
static const int PARTICLE_EFFECT_SHATTER_EARTH_ORBIT = 47;
static const int PARTICLE_EFFECT_EARTH_ELEMENTAL_DIE = 48;
static const int PARTICLE_EFFECT_MUSHROOM_SPELL = 49;
static const int PARTICLE_EFFECT_MISC_PUDDLE = 50;
static const int PARTICLE_EFFECT_BOLAS = 51;
static const int PARTICLE_EFFECT_BASTION_MUSHROOM = 52;
static const int PARTICLE_EFFECT_FOCI_ORBIT = 53;
static const int PARTICLE_EFFECT_SCEPTER_BLAST_ORBIT1 = 54;
static const int PARTICLE_EFFECT_STASIS_CAGE_ORBIT = 55;
static const int PARTICLE_EFFECT_STASIS_RIFT_ORBIT = 56;
static const int PARTICLE_EFFECT_EARTH_ELEMENTAL_SUMMON_AOE = 57;
static const int PARTICLE_EFFECT_THORNS_ORBIT = 58;

// actmagicIsVertical constants
static const int MAGIC_ISVERTICAL_NONE = 0;
static const int MAGIC_ISVERTICAL_Z = 1;
static const int MAGIC_ISVERTICAL_XYZ = 2;

// misc particle timer actions
static const int PARTICLE_TIMER_ACTION_SHOOT_PARTICLES = 1;
static const int PARTICLE_TIMER_ACTION_SPAWN_PORTAL = 2;
static const int PARTICLE_TIMER_ACTION_SUMMON_MONSTER = 3;
static const int PARTICLE_TIMER_ACTION_SPELL_SUMMON = 4;
static const int PARTICLE_TIMER_ACTION_DEVIL_SUMMON_MONSTER = 5;
static const int PARTICLE_TIMER_ACTION_MAGIC_SPRAY = 6;
static const int PARTICLE_TIMER_ACTION_FOCI_SPRAY = 7;
static const int PARTICLE_TIMER_ACTION_MAGIC_WAVE = 8;
static const int PARTICLE_TIMER_ACTION_TEST_1 = 9;
static const int PARTICLE_TIMER_ACTION_TEST_2 = 10;
static const int PARTICLE_TIMER_ACTION_IGNITE = 11;
static const int PARTICLE_TIMER_ACTION_SHATTER = 12;
static const int PARTICLE_TIMER_ACTION_VORTEX = 13;
static const int PARTICLE_TIMER_ACTION_LIGHTNING = 14;
static const int PARTICLE_TIMER_ACTION_BOOBY_TRAP = 15;
static const int PARTICLE_TIMER_ACTION_SPORES = 16;
static const int PARTICLE_TIMER_ACTION_DAMAGE_LOS_AREA = 17;
static const int PARTICLE_TIMER_ACTION_DISRUPT_EARTH = 18;
static const int PARTICLE_TIMER_ACTION_ETERNALS_GAZE = 19;
static const int PARTICLE_TIMER_TELEPORT_PULL_TARGET_LOCATION = 20;
static const int PARTICLE_TIMER_ACTION_ETERNALS_GAZE2 = 21;
static const int PARTICLE_TIMER_ACTION_SHATTER_EARTH = 22;
static const int PARTICLE_TIMER_ACTION_EARTH_ELEMENTAL = 23;
static const int PARTICLE_TIMER_ACTION_EARTH_ELEMENTAL_ROLL = 24;
static const int PARTICLE_TIMER_ACTION_EARTH_ELEMENTAL_DIE = 25;
static const int PARTICLE_TIMER_ACTION_ROOTS1 = 26;
static const int PARTICLE_TIMER_ACTION_ROOTS_SINGLE_TILE = 27;
static const int PARTICLE_TIMER_ACTION_ROOTS_PATH = 28;
static const int PARTICLE_TIMER_ACTION_SPORES_TRAIL = 29;
static const int PARTICLE_TIMER_ACTION_ROOTS_SUSTAIN = 30;
static const int PARTICLE_TIMER_ACTION_BASTION_MUSHROOM = 31;
static const int PARTICLE_TIMER_ACTION_ROOTS_SINGLE_TILE_VOID = 32;

struct ParticleEmitterHit_t
{
	Uint32 tick = 0;
	int hits = 0;
};
struct ParticleTimerEffect_t
{
	enum EffectType
	{
		EFFECT_NONE,
		EFFECT_ICE_WAVE,
		EFFECT_TEST_1,
		EFFECT_TEST_2,
		EFFECT_TEST_3,
		EFFECT_DISRUPT_EARTH,
		EFFECT_LIGHTNING_BOLT,
		EFFECT_ROOTS_SELF,
		EFFECT_TEST_7,
		EFFECT_FIRE_WAVE,
		EFFECT_KINETIC_FIELD,
		EFFECT_PULSE,
		EFFECT_SPORES,
		EFFECT_TUNNEL,
		EFFECT_CHRONOMIC_FIELD,
		EFFECT_ROOTS_TILE,
		EFFECT_ROOTS_PATH,
		EFFECT_MYCELIUM,
		EFFECT_ROOTS_SELF_SUSTAIN,
		EFFECT_ROOTS_TILE_VOID
	};
	struct Effect_t
	{
		real_t x = 0.0;
		real_t y = 0.0;
		EffectType effectType = EFFECT_NONE;
		real_t yaw = 0.0;
		int sfx = 0;
		bool firstEffect = false;
	};

	struct EffectLocations_t
	{
		real_t yawOffset = 0.0;
		real_t xOffset = 0.0;
		real_t seconds = 0.0;
		real_t dist = 0.0;
		int sfx = 0;
	};
	std::map<Uint32, Effect_t> effectMap;
};
extern std::map<Uint32, std::map<Uint32, ParticleEmitterHit_t>> particleTimerEmitterHitEntities;
extern std::map<Uint32, ParticleTimerEffect_t> particleTimerEffects;
ParticleEmitterHit_t* getParticleEmitterHitProps(Uint32 emitterUid, Entity* hitentity);

bool addSpell(int spell, int player, bool ignoreSkill = false); //Adds a spell to the client's spell list. Note: Do not use this to add custom spells.

//TODO: Create a spell class which has the basic spell(s) involved, the mana to use etc. All of those important details. This should support vanilla spells and custom spells with just one data type. The addSpell function gives the player a vanilla spell if they don't already have it.

typedef struct spellElement_t spellElement_t;

//TODO: Give spellElements/spells a property that makes it so players don't know they exist/can't use them. Sorta like "You have no idea on the inner workings of this spell. It is beyond your comprehension."
//TODO: Channeling spells. Or spells that otherwise impose a constant drain until you cancel them. NEED THIS ASAP I SAY.
//TODO: Don't re-invent the wheel with lists here.
typedef struct spellElement_t
{
private:
	int damage = 0;
	int damage2 = 0;
	int duration2 = 0;
	real_t damage_mult = 1.0;
	real_t damage2_mult = 1.0;
	real_t channeledMana_mult = 1.0;
	int channeledMana_duration = TICKS_PER_SECOND;
public:
	void setDamage(int _damage) { damage = _damage; }
	int getDamage() { return damage; }
	void setDamageSecondary(int _damage) { damage2 = _damage; }
	int getDamageSecondary() { return damage2; }
	void setDurationSecondary(int _duration) { duration2 = _duration; }
	int getDurationSecondary() { return duration2; }
	real_t getDamageMult() { return damage_mult; }
	real_t getDamage2Mult() { return damage2_mult; }
	real_t getChanneledManaMult() { return channeledMana_mult; }
	int getChanneledManaDuration() { return channeledMana_duration; }
	void setChanneledManaDuration(int _duration) { channeledMana_duration = _duration; }
	void setChanneledManaMult(real_t _mult) { channeledMana_mult = _mult; }
	int duration; // travel time if it's a missile element, duration for a light spell, duration for curses/enchants/traps/beams/rays/effects/what have you.
	char element_internal_name[64];
	int elementID = 0;
	bool can_be_learned; // if a spellElement can't be learned, a player won't be able to build spells with it.
	int channeledMana = 0; // sustained spell if channeled cost > 0
	bool fociSpell = false;
	/*
	I've been thinking about mana consumption. I think it should drain mana 1 by 1 regardless of how much the spell initially cost to cast.
	So:
	//----------
	timer = duration / mana; //TODO: UH OH. I just realized it drains for the whole spell's cost. Umm. It should only drain the channeled element's cost.
	...
	if (timer <= 0)
		consumeMana(1);
	//----------
	So, what this does is divide the spell's duration up into a number of equal parts based on how much mana the element is being pumped with.
	So, for example, if a fire element costs 3 mana to sustain and it's duration is one minute, the timer will be set to 20 seconds and will consume one mana every 20 seconds.
	*/

	list_t elements; // all spell elements attached to this one.
	node_t* node; // points to its location in whatever list it's in.
} spellElement_t;

/*
 * To the player, this means that they have no idea what this node is.
 * To the game, this indicates a stock spell and does whatever automagic it needs to do for the spell to work as stock. Because player spells != stock spells because this is a small dungeon dive and we can't have archmages reprogramming the world, now, can we? In a sequel perhaps, because there will be more for other classes too.
 * So, it basically marks a spell as a "complex spell," which just means that this spell does complicated stuff that can't be easily achieved, if at all, with the basic spell system implemented in this game. Like I said, this game doesn't even need anything more complex. It's just a dungeon dive, and it's not like the player will become an archmage out of his first dungeon dive. No, that takes decades of adventuring and study.
 */
extern spellElement_t spellElement_unintelligible;

/*
 * The missile element gives propulsion to a spell; it makes a spell a projectile.
 * Base cost: 1 mana.
 * Overload: Every additional mana put into this spell increases speed & lifetime. //TODO: Separately control speed & lifetime? E.g. put mana into each separately, must have at least one in each
 */
extern spellElement_t spellElement_missile;

/*
 * The force element gives a spell properties related with force. For example, when it is attached onto a missile element, it creates a projectile of pure force.
 * Base cost: 2 mana.
 * Base damage: 1.
 * Overload: Every 2 additional points of mana put into this spell increaes its damage by 1.
 * Treshold effects (in concjunction with missile only? Maybe apply it to all spells that use it in general to give those spells a piercing quality): Every 10 points of mana, it pierces an additional enemy (or gains one more bounce?).
 */
extern spellElement_t spellElement_force;

/*
 * The fire element gives a spell fire properties. Duh.
 * For example, when it is attached onto a missile element, it turns it into a fire projectile.
 * Base cost: 2 mana.
 * Base damage: 4.
 * Overload: Every additional point increases its damage by 1.
 * Treshold effects: ?
 * //TODO: Cause burning effect?
 */
extern spellElement_t spellElement_fire;

/*
 * The lightning element gives a spell lightning properties. Duh.
 * For example, when it is attached onto a missile element, it turns it into a lightning projectile.
 * Base cost: 5 mana.
 * Base damage: 20.
 * Overload: Every additional 1 points increases its damage by 4.
 * Treshold effects: AoE.
 */
extern spellElement_t spellElement_lightning;

/*
 * The light element gives a spell light properties. Duh.
 * For example, by itself, it will create a magical orb that follows you around.
 * Base cost: 1 mana.
 * Duration: 100
 * Overload: Every additional 1 point increases its duration by 100.  //What about intensity? Blind at certain intensity? Burn damage at higher intensity? Set things on fire/melt things at super intensity?
 */
extern spellElement_t spellElement_light;

/*
 * The dig element gives a spell digging properties.
 * If attached to a missile element, creates a bolt you can shoot at walls that destroys them.
 * Base cost: 20 mana.
 * Overload: More walls?
 */
extern spellElement_t spellElement_dig;

/*
 * The invisible element gives a spell invisible properties.
 * If used by itself, turns the caster invisible. //TODO: Make a personal spell element or something to target the caster or something along those lines?
 * Base cost: 1 mana.
 * Duration: 100
 * Overload: Every 1 extra point of mana increases duration by 100.
 */
extern spellElement_t spellElement_invisible;

/*
 * The identify element gives a spell identifying properties.
 * If used by itself, it lets the caster select an item in their inventory and identify it.
 * Base cost: 10 mana.
 * TODO: Make it a complex spell? That is, it can't be used in other spells, only as the base spell it comes with.
 * ...actually, it might be cool shooting a missile of identify to identify traps and/or monsters.
 */
extern spellElement_t spellElement_identify;

/*
 * The magicmapping element maps out the level for the player's minimap.
 */
extern spellElement_t spellElement_magicmapping;

/*
 * The heal element makes a healing spell.
 */
extern spellElement_t spellElement_heal;

/*
 * The confuse element makes a confuse spell.
 * Monsters run around and do stuff and stuff.
 * Can cause an array of different effects: The monster attacks the wrong people, the monster runs around screaming mindlessly, the monster fights invisible foes...(Perhaps the effects should be a random roll on the monster's behalf)
 * Duration is a function of mana.
 */
extern spellElement_t spellElement_confuse;

/*
 * The cure ailments elements makes a cure ailments spell.
 * It...removes all status effects on the player. (Not just any old entity. Might wanna rectify that eventually)
 * Overloading it does nothing.
 */
extern spellElement_t spellElement_cure_ailment;

/*
 * Locks doors.
 * Overloading it does nothing.
 */
extern spellElement_t spellElement_locking;

/*
 * Opens doors & gates.
 * Overloading it does nothing.
 */
extern spellElement_t spellElement_opening;

/*
 * Makes the entity sleep.
 * Overloading it does nothing,
 * You see, every creature has a sleep saturation thing. This spell triggers the sleep state. The creature will sleep until it's slept too much. After that, this spell will have no effect -- the creature's simply slept too long!
 * So this spell simply sets the creature in a sleep state. It doesn't make it sleep for x time. It just makes it asleep!
 */
extern spellElement_t spellElement_sleep;

/*
 * Deals damage and slows for duration.
 */
extern spellElement_t spellElement_cold;

/*
 * Slows for duration.
 */
extern spellElement_t spellElement_slow;

/*
 * Channeled spell.
 */
extern spellElement_t spellElement_levitation;

/*
 * Easiest spell to implement ever.
 */
extern spellElement_t spellElement_teleportation;

/*
 * Easiest spell to implement ever.
 */
extern spellElement_t spellElement_magicmissile;

/*
 * Pops up a GUI (similar to the identify GUI) which uncurses items.
 */
extern spellElement_t spellElement_removecurse;

/*
* Summons familiars.
*/
extern spellElement_t spellElement_summon;
/*
* Paralysis effect.
*/
extern spellElement_t spellElement_stoneblood;
/*
* Damage and bleed effect.
*/
extern spellElement_t spellElement_bleed;

/*Dmg/Poison and degrade armor*/
extern spellElement_t spellElement_acidSpray;

/*
* The missile element gives propulsion to a spell; it makes a spell a projectile.
* Base cost: 1 mana.
* Shoots 3 projectiles.
* Overload: Every additional mana put into this spell increases speed & lifetime. //TODO: Separately control speed & lifetime? E.g. put mana into each separately, must have at least one in each
*/
extern spellElement_t spellElement_missile_trio;

/*
 * Turns a non-boss non-player creature into one of your followers.
 */
extern spellElement_t spellElement_dominate;
extern spellElement_t spellElement_reflectMagic;
extern spellElement_t spellElement_stealWeapon;
extern spellElement_t spellElement_drainSoul;
extern spellElement_t spellElement_vampiricAura;
extern spellElement_t spellElement_charmMonster;
extern spellElement_t spellElement_shapeshift;
extern spellElement_t spellElement_sprayWeb;
extern spellElement_t spellElement_poison;
extern spellElement_t spellElement_speed;
extern spellElement_t spellElement_fear;
extern spellElement_t spellElement_strike;
extern spellElement_t spellElement_detectFood;
extern spellElement_t spellElement_weakness;
extern spellElement_t spellElement_amplifyMagic;
extern spellElement_t spellElement_shadowTag;
extern spellElement_t spellElement_telePull;
extern spellElement_t spellElement_demonIllusion;
extern spellElement_t spellElement_trollsBlood;
extern spellElement_t spellElement_salvageItem;
extern spellElement_t spellElement_flutter;
extern spellElement_t spellElement_dash;
extern spellElement_t spellElement_selfPolymorph;
extern spellElement_t spellElement_ghostBolt;
extern spellElement_t spellElement_slimeAcid;
extern spellElement_t spellElement_slimeWater;
extern spellElement_t spellElement_slimeFire;
extern spellElement_t spellElement_slimeTar;
extern spellElement_t spellElement_slimeMetal;
extern spellElement_t spellElement_slime_spray;
extern std::map<int, spellElement_t> spellElementMap;

enum SpellElementIDs_t
{
	SPELL_ELEMENT_NONE = 10000,
	SPELL_ELEMENT_PROPULSION_FOCI_SPRAY,
	SPELL_ELEMENT_PROPULSION_MISSILE,
	SPELL_ELEMENT_METEOR_FLAMES,
	SPELL_ELEMENT_PROPULSION_FLOOR_TILE,
	SPELL_ELEMENT_PROPULSION_MAGIC_SPRAY,
	SPELL_ELEMENT_PROPULSION_MISSILE_NOCOST,
	SPELL_ELEMENT_SPRITE_FLAMES,
	SPELL_ELEMENT_METEOR_EXPLODE,
	SPELL_ELEMENT_MAX
};

enum SpellRangefinderType
{
	RANGEFINDER_NONE,
	RANGEFINDER_TARGET,
	RANGEFINDER_TOUCH,
	RANGEFINDER_TOUCH_FLOOR_TILE,
	RANGEFINDER_TOUCH_WALL_TILE,
	RANGEFINDER_TOUCH_INTERACT,
	RANGEFINDER_TOUCH_INTERACT_TEST
};

/*
 */
//TODO: Differentiate between touch spells, enchantment spells, personal spells, ranged spells, area of effect spells, close blast/burst spells, and enemy/ally target spells.
//TODO: Support setting how a spell resolves? Eg teleportation: Click, shoot ball, end up where ball hits wall or end of teleport range, or, bring up map, click where you want to teleport to, etc. Or maybe just make different spells for each one. Eg teleporting step could be click and appear at end of path, while teleportation itself could bring up a map and you click on where you want to teleport to.
/*
 * TODO: Consider this idea:
 * Spells give you discounts. So it's more expensive to cast 5 fireball spells than a single spell which creates 5 fireballs.
 */
//TODO: Here's a good question: How do we determine spell casting times? By the total mana you need to amass & magic skills?
typedef struct spell_t
{
	int ID;
	char spell_internal_name[64];
	//spellElement_t *elements;
	int difficulty; //The proficiency you need in the magic skill to learn this spell. //TODO: Should this instead be determined by the spell elements?
	//int skill_caster; //The spellcasting skill it was cast with. Lower skill can introduce inefficiencies and other !!FUN!!
	bool sustain; //If a spell is channeled, should it be sustained? (NOTE: True by default. Set to false when the player decides to cancel/abandon a spell)
	bool magicstaff; // if true the spell was cast from a magicstaff and thus it may have slightly different behavior
	node_t* sustain_node = nullptr; //Node in the sustained/channeled spells list.
	node_t* magic_effects_node = nullptr;
	bool hide_from_ui = false; // hide from skillsheet/other UI places
	SpellRangefinderType rangefinder = SpellRangefinderType::RANGEFINDER_NONE;

	Uint32 caster = 0;
	real_t distance = 0.0;
	real_t distance_mult = 1.0;
	int skillID = PRO_MAGIC;
	real_t cast_time = 1.0;
	real_t cast_time_mult = 1.0;
	int mana = 1;

	int radius = 0;
	real_t radius_mult = 0.0;
	int life_time = 0; // for floor based effects
	real_t life_time_mult = 1.0;
	int sustainEffectDissipate = -1; // when the spell is unsustained, clear this effect from the player (unique spell effects)
	int channel_duration = 0; //This is the value to reset the timer to when a spell is channeled.
	int channel_effectStrength = 1; // how strong to reapply the effect each duration tick
	list_t elements; //NOTE: This could technically allow a spell to have multiple roots. So you could make a flurry of fireballs, for example.
	//TODO: Some way to make spells work with "need to cast more to get better at casting the spell." A sort of spell learning curve. The first time you cast it, prone to failure. Less the more you cast it.
	
	enum SpellBasePropertiesFloat
	{
		SPELLPROP_DISTANCE,
		SPELLPROP_DISTANCE_MULT,
		SPELLPROP_CAST_TIME,
		SPELLPROP_CAST_TIME_MULT,
		SPELLPROP_MODIFIED_CAST_TIME,
		SPELLPROP_BASE_PROPERTY_FLOAT_ENUM_END
	};

	enum SpellBasePropertiesInt
	{
		SPELLPROP_FOCI_REFIRE_TICKS,
		SPELLPROP_FOCI_SECONDARY_MANA_COST,
		SPELLPROP_MODIFIED_RADIUS,
		SPELLPROP_BASE_PROPERTY_INT_ENUM_END
	};

	enum SpellOnCastEventTypes
	{
		SPELL_LEVEL_EVENT_DEFAULT = 1,
		SPELL_LEVEL_EVENT_DMG = 2,
		SPELL_LEVEL_EVENT_EFFECT = 4,
		SPELL_LEVEL_EVENT_SUMMON = 8,
		SPELL_LEVEL_EVENT_SHAPESHIFT = 16,
		SPELL_LEVEL_EVENT_SUSTAIN = 32,
		SPELL_LEVEL_EVENT_MAGICSTAFF = 64,
		SPELL_LEVEL_EVENT_SPELLBOOK = 128
	};

	// get localized spell name
	const char* getSpellName();
} spell_t;

extern list_t channeledSpells[MAXPLAYERS]; //Spells the player is currently channeling. //TODO: Universalize it for all entities that can cast spells? //TODO: Cleanup and stuff.
extern std::map<int, spell_t*> allGameSpells; // to iterate over for quickly finding attributes of all spells.

//TODO: Add stock spells.

//Magic missile is a more powerful version of forcebolt.
//Cold deals damage & slows.
//Remove curse pops open a GUI and lets you uncurse cursed items.
//Teleportation is random.
//Sleep just sets an effect.
//TODO: Major spells. AoE spells. Zone spells. Etc.
extern spell_t spell_forcebolt; //Done.
extern spell_t spell_magicmissile; //Done. //TODO: Better effects?
extern spell_t spell_cold; //Done.
extern spell_t spell_fireball; //Done.
extern spell_t spell_lightning; //Done.
extern spell_t spell_removecurse;
extern spell_t spell_light; //Done.
extern spell_t spell_identify; //Done.
extern spell_t spell_magicmapping; //Done.
extern spell_t spell_sleep; //Done.
extern spell_t spell_confuse; //Done.
extern spell_t spell_slow; //Done.
extern spell_t spell_opening; //Done.
extern spell_t spell_locking; //Done.
extern spell_t spell_levitation; //Done.
extern spell_t spell_invisibility; //Done.
extern spell_t spell_teleportation; //Done.
extern spell_t spell_healing; //Done. //TODO: Target modes? (Self or projectile?) /TODO: Make it work for NPCs.
extern spell_t spell_extrahealing; //Done. //TODO: AoE heal? Or target modes (self, projectile, aoe, etc)?
//extern spell_t spell_restoreability; //--CUT--
extern spell_t spell_cureailment; //Done. //TODO: Generalize for NPCs?
extern spell_t spell_dig; //Done.
extern spell_t spell_summon;
extern spell_t spell_stoneblood;
extern spell_t spell_bleed;
extern spell_t spell_dominate;
extern spell_t spell_reflectMagic;
extern spell_t spell_acidSpray;
extern spell_t spell_stealWeapon;
extern spell_t spell_drainSoul;
extern spell_t spell_vampiricAura;
extern spell_t spell_charmMonster;
extern spell_t spell_revertForm;
extern spell_t spell_ratForm;
extern spell_t spell_spiderForm;
extern spell_t spell_trollForm;
extern spell_t spell_impForm;
extern spell_t spell_sprayWeb;
extern spell_t spell_poison;
extern spell_t spell_speed;
extern spell_t spell_fear;
extern spell_t spell_strike;
extern spell_t spell_detectFood;
extern spell_t spell_weakness;
extern spell_t spell_amplifyMagic;
extern spell_t spell_shadowTag;
extern spell_t spell_telePull;
extern spell_t spell_demonIllusion;
extern spell_t spell_trollsBlood;
extern spell_t spell_salvageItem;
extern spell_t spell_flutter;
extern spell_t spell_dash;
extern spell_t spell_polymorph;
extern spell_t spell_ghost_bolt;
extern spell_t spell_slime_acid;
extern spell_t spell_slime_water;
extern spell_t spell_slime_fire;
extern spell_t spell_slime_tar;
extern spell_t spell_slime_metal;
//TODO: Armor/protection/warding spells.
//TODO: Targeting method?

struct CastSpellProps_t
{
	real_t caster_x = 0.0;
	real_t caster_y = 0.0;
	real_t target_x = 0.0;
	real_t target_y = 0.0;
	Uint32 targetUID = 0;
	int elementIndex = 0;
	real_t distanceOffset = 0.0;
	int wallDir = 0;
	Uint8 optionalData = 0;
	bool setToMonsterCast(Entity* monster, int spellID);
};

void setupSpells();
void equipSpell(spell_t* spell, int playernum, Item* spellItem);
Entity* castSpell(Uint32 caster_uid, spell_t* spell, bool using_magicstaff, bool trap, bool usingSpellbook = false, CastSpellProps_t* castSpellProps = nullptr, bool usingFoci = false);
void castSpellInit(Uint32 caster_uid, spell_t* spell, bool usingSpellbook); //Initiates the spell animation, then hands off the torch to it, which, when finished, calls castSpell.
int spellGetCastSound(spell_t* spell);
#ifndef EDITOR // editor doesn't know about stat*
int getSpellcastingAbilityFromUsingSpellbook(spell_t* spell, Entity* caster, Stat* casterStats);
bool isSpellcasterBeginnerFromSpellbook(int player, Entity* caster, Stat* stat, spell_t* spell, Item* spellbookItem);
int getSpellbookBonusPercent(Entity* caster, Stat* stat, Item* spellbookItem);
real_t getBonusFromCasterOfSpellElement(Entity* caster, Stat* casterStats, spellElement_t* spellElement, int spellID, int proficiencyWhenNoSpell);
real_t getSpellBonusFromCasterINT(Entity* caster, Stat* casterStats, int skillID);
int getSpellbookBaseINTBonus(Entity* caster, Stat* casterStats, int skillID);
void magicOnEntityHit(Entity* parent, Entity* particle, Entity* hitentity, Stat* hitstats, Sint32 preResistanceDamage, Sint32 damage, Sint32 oldHP, int spellID, int selfCastUsingItem = 0);
void magicTrapOnHit(Entity* parent, Entity* hitentity, Stat* hitstats, Sint32 oldHP, int spellID);
bool applyGenericMagicDamage(Entity* caster, Entity* hitentity, Entity& damageSourceProjectile, int spellID, int damage, bool alertMonsters,
	bool monsterCollisionOnly = false);
#endif
bool isSpellcasterBeginner(int player, Entity* caster, int skillID);
void actMagicTrap(Entity* my);
void actMagicStatusEffect(Entity* my);
void actMagicMissile(Entity* my);
void actMagicClient(Entity* my);
void actMagicClientNoLight(Entity* my);
void actMagicParticle(Entity* my);
void actHUDMagicParticle(Entity* my);
void actTouchCastThirdPersonParticle(Entity* my);
void actHUDMagicParticleCircling(Entity* my);
void actMagicParticleCircling2(Entity* my);
void actMagicParticleEnsembleCircling(Entity* my);
void createEnsembleHUDParticleCircling(Entity* parent);
void createEnsembleTargetParticleCircling(Entity* parent);
Entity* spawnMagicParticle(Entity* parentent);
Entity* spawnMagicParticleCustom(Entity* parentent, int sprite, real_t scale, real_t spreadReduce);
void spawnMagicEffectParticles(Sint16 x, Sint16 y, Sint16 z, Uint32 sprite);
void spawnMagicEffectParticlesBell(Entity* bell, Uint32 sprite);
void createParticleCircling(Entity* parent, int duration, int sprite);
void actParticleCircle(Entity* my);
void actParticleDot(Entity* my);
void actParticleRock(Entity* my);
void actParticleTest(Entity* my);
void actParticleErupt(Entity* my);
void actParticleTimer(Entity* my);
void actParticleSap(Entity* my);
void actParticleSapCenter(Entity* my);
void actParticleExplosionCharge(Entity* my);
void actParticleFollowerCommand(Entity* my);
void actParticleCharmMonster(Entity* my);
void actParticleAestheticOrbit(Entity* my);
void actParticleBolas(Entity* my);
void actParticleShadowTag(Entity* my);
void actParticlePinpointTarget(Entity* my);
void actParticleFloorMagic(Entity* my);
void actParticleVortex(Entity* my);
void actParticleWave(Entity* my);
void actParticleRoot(Entity* my);
void actParticleDemesneDoor(Entity* my);
void actRadiusMagic(Entity* my);
void actRadiusMagicBadge(Entity* my);
void actParticleShatterEarth(Entity* my);
void actParticleShatterEarthRock(Entity* my);

void createParticleDropRising(Entity* parent, int sprite, double scale);
void createParticleDot(Entity* parent);
Entity* createParticleBolas(Entity* parent, int sprite, int duration, Item* item);
Entity* createParticleAestheticOrbit(Entity* parent, int sprite, int duration, int particleType);
void createParticleRock(Entity* parent, int sprite = -1, bool light = false);
void createParticleShatteredGem(real_t x, real_t y, real_t z, int sprite, Entity* parent);
void createParticleErupt(Entity* parent, int sprite);
void createParticleErupt(real_t x, real_t y, int sprite);
Entity* createParticleBoobyTrapExplode(Entity* caster, real_t x, real_t y);
Entity* createParticleSapCenter(Entity* parent, Entity* target, int spell, int sprite, int endSprite);
Entity* createParticleTimer(Entity* parent, int duration, int sprite);
void createParticleSap(Entity* parent);
void createParticleExplosionCharge(Entity* parent, int sprite, int particleCount, double scale);
void createParticleFollowerCommand(real_t x, real_t y, real_t z, int sprite, Uint32 uid);
Entity* createParticleCastingIndicator(Entity* parent, real_t x, real_t y, real_t z, Uint32 lifetime, Uint32 followUid);
Entity* createParticleAOEIndicator(Entity* parent, real_t x, real_t y, real_t z, Uint32 lifetime, int size);
static const int FOLLOWER_SELECTED_PARTICLE = 1229;
static const int FOLLOWER_TARGET_PARTICLE = 1230;
void createParticleCharmMonster(Entity* parent);
void createParticleShadowTag(Entity* parent, Uint32 casterUid, int duration);
static const int PINPOINT_PARTICLE_START = 1767;
static const int PINPOINT_PARTICLE_END = 1775;
void createParticleSpellPinpointTarget(Entity* parent, Uint32 casterUid, int sprite, int duration, int spellID);
Entity* createFloorMagic(ParticleTimerEffect_t::EffectType particleType, int sprite, real_t x, real_t y, real_t z, real_t dir, Uint32 lifetime);
Entity* createRadiusMagic(int spellID, Entity* caster, real_t x, real_t y, real_t radius, Uint32 lifetime, Entity* follow);
void floorMagicClientReceive(Entity* my);
void particleWaveClientReceive(Entity* my);
void radiusMagicClientReceive(Entity* entity);
Entity* floorMagicSetLightningParticle(Entity* my);
void floorMagicCreateLightningSequence(Entity* spellTimer, int startTickOffset);
void floorMagicCreateSpores(Entity* spawnOnEntity, real_t x, real_t y, Entity* caster, int damage, int spellID);
Entity* floorMagicCreateRoots(real_t x, real_t y, Entity* caster, int damage, int spellID, int duration, int particleTimerAction);
Entity* createVortexMagic(int sprite, real_t x, real_t y, real_t z, real_t dir, Uint32 lifetime);
Entity* createParticleWave(ParticleTimerEffect_t::EffectType particleType, int sprite, real_t x, real_t y, real_t z, real_t dir, Uint32 lifetime, bool light);
Entity* createParticleRoot(int sprite, real_t x, real_t y, real_t z, real_t dir, Uint32 lifetime);
void createMushroomSpellEffect(Entity* caster, real_t x, real_t y);
Entity* createWindMagic(Uint32 casterUID, int x, int y, int duration, int dir, int length);
void createParticleDemesneDoor(real_t x, real_t y, real_t dir);
Entity* createTunnelPortal(real_t x, real_t y, int duration, int dir);
void tunnelPortalSetAttributes(Entity* portal, int duration, int dir);
Entity* createSpellExplosionArea(int spellID, Entity* caster, real_t x, real_t y, real_t z, real_t radius, int damage, Entity* ohitentity);
void doSpellExplosionArea(int spellID, Entity* my, Entity* caster, real_t x, real_t y, real_t z, real_t radius);
void createParticleSpin(Entity* entity);
void createParticleShatterEarth(Entity* my, Entity* caster, real_t _x, real_t _y, int spellID);
void actEarthElementalDeathGib(Entity* my);
void actLeafParticle(Entity* my);
void actLeafPile(Entity* my);
Entity* spawnLeafPile(real_t x, real_t y, bool trap);

void spawnMagicTower(Entity* parent, real_t x, real_t y, int spellID, Entity* autoHitTarget, bool castedSpell = false); // autoHitTarget is to immediate damage an entity, as all 3 tower magics hitting is unreliable
bool magicDig(Entity* parent, Entity* projectile, int numRocks, int randRocks);

spell_t* copySpell(spell_t* spell, int subElementToCopy = -1);
void spellConstructor(spell_t* spell, int ID);
spell_t* spellConstructor(int ID, int difficulty, const char* internal_name, std::vector<int> elements);
void spellDeconstructor(void* data);
spellElement_t* copySpellElement(spellElement_t* spellElement);
void spellElementConstructor(spellElement_t* element);
void spellElementConstructor(int elementID, int mana, int base_mana, int overload_mult, int damage, int duration, const char* internal_name);
void spellElementDeconstructor(void* data);

int getCostOfSpell(spell_t* spell, Entity* caster = nullptr);
int getGoldCostOfSpell(spell_t* spell, int player);
int getSustainCostOfSpell(spell_t* spell, Entity* caster);
bool spell_isChanneled(spell_t* spell);
bool spellElement_isChanneled(spellElement_t* spellElement);

spell_t* getSpellFromID(int ID);
int getSpellbookFromSpellID(int spellID);

bool spellInList(list_t* list, spell_t* spell);

//-----Implementations of spell effects-----
void spell_magicMap(int player, int radius, int x, int y); //Magics the map. I mean maps the magic. I mean magically maps the level.
void spell_detectFoodEffectOnMap(int player);
void spell_summonFamiliar(int player); // summons some familiars.
void spell_changeHealth(Entity* entity, int amount, bool overdrewFromHP = false); //This function changes an entity's health.

//-----Spell Casting Animation-----
//The two hand animation functions.
void actLeftHandMagic(Entity* my);
void actRightHandMagic(Entity* my);
void actMagicRangefinder(Entity* my);

typedef struct spellcastingAnimationManager
{
	//The data to pass on to the castSpell function.
	spell_t* spell;
	Uint32 caster;
	int player;

	bool active;
	bool active_spellbook;
	int stage; //The current stage of the animation.
	int circle_count; //How many times it's circled around in the circle stage.
	int times_to_circle; //How many times to circle around in the circle stage.
	int throw_count = 0;
	int active_count = 0;

	int consume_interval; //Every consume_interval ticks, eat a mana.
	int consume_timer; //How many ticks left till next mana consume.
	int mana_left; //How much mana is left to consume.
	int mana_cost; //Tracking cost of spell
	bool consumeMana; //If false, goes through the motions, even casts the spell -- just doesn't consume any mana.

	float lefthand_movex;
	float lefthand_movey;
	float lefthand_angle;
	float vibrate_x = 0.f;
	float vibrate_y = 0.f;

	real_t target_x = 0.0;
	real_t target_y = 0.0;
	real_t caster_x = 0.0;
	real_t caster_y = 0.0;
	Uint32 targetUid = 0;
	int wallDir = 0;
	SpellRangefinderType rangefinder = RANGEFINDER_NONE;
	void setRangeFinderLocation();
	void resetRangefinder();
	bool hideShieldFromBasicCast();
	void executeAttackSpell(bool swingweapon);
	bool spellWaitingAttackInput();
	bool spellIgnoreAttack();
} spellcasting_animation_manager_t;
extern spellcasting_animation_manager_t cast_animation[MAXPLAYERS];

void fireOffSpellAnimation(spellcasting_animation_manager_t* animation_manager, Uint32 caster_uid, spell_t* spell, bool usingSpellbook);
void spellcastingAnimationManager_deactivate(spellcasting_animation_manager_t* animation_manager);
void spellcastAnimationUpdateReceive(int player, int attackPose, int castTime);
void spellcastAnimationUpdate(int player, int attackPose, int castTime);

class Item;

spell_t* getSpellFromItem(const int player, Item* item, bool usePlayerInventory);
int getSpellIDFromSpellbook(int spellbookType);
int getSpellIDFromFoci(int fociType);
int canUseShapeshiftSpellInCurrentForm(const int player, Item& item);

//Spell implementation stuff.
bool spellEffectDominate(Entity& my, spellElement_t& element, Entity& caster, Entity* parent);
void spellEffectAcid(Entity& my, spellElement_t& element, Entity* parent, int damage, int resistance);
void spellEffectStealWeapon(Entity& my, spellElement_t& element, Entity* parent, int resistance);
void spellEffectDrainSoul(Entity& my, spellElement_t& element, Entity* parent, int damage, int resistance);
spell_t* spellEffectVampiricAura(Entity* caster, spell_t* spell);
int getCharmMonsterDifficulty(Entity& my, Stat& myStats);
void spellEffectCharmMonster(Entity& my, spellElement_t& element, Entity* parent, int resistance, bool magicstaff);
Entity* spellEffectPolymorph(Entity* target, Entity* parent, bool fromMagicSpell, int customDuration = 0, Monster customMonster = NOTHING); // returns nullptr if target was monster, otherwise returns pointer to new creature
void spellEffectPoison(Entity& my, spellElement_t& element, Entity* parent, int damage, int resistance);
void spellEffectSprayWeb(Entity& my, spellElement_t& element, Entity* parent, int resistance);
bool spellEffectFear(Entity* my, spellElement_t& element, Entity* forceParent, Entity* target, int resistance);
bool spellEffectTeleportPull(Entity* my, spellElement_t& element, Entity* parent, Entity* target, int resistance);
void spellEffectShadowTag(Entity& my, spellElement_t& element, Entity* parent, int resistance);
bool spellEffectDemonIllusion(Entity& my, spellElement_t& element, Entity* parent, Entity* target, int resistance);
Entity* spellEffectAdorcise(Entity& caster, spellElement_t& element, real_t x, real_t y, Item* itemToAdorcise);
Entity* spellEffectFlameSprite(Entity& caster, spellElement_t& element, real_t x, real_t y);
Entity* spellEffectHologram(Entity& caster, spellElement_t& element, real_t x, real_t y);
Entity* spellEffectDemesneDoor(Entity& caster, Entity& doorFrame);
void magicSetResistance(Entity* entity, Entity* parent, int& resistance, real_t& damageMultiplier, DamageGib& dmgGib, int& trapResist);
int getSpellDamageFromID(int spellID, Entity* parent, Stat* parentStats, Entity* magicSourceParticle, real_t addSpellBonus = 0.0, bool applyingDamageOnCast = true);
int getSpellDamageSecondaryFromID(int spellID, Entity* parent, Stat* parentStats, Entity* magicSourceParticle, real_t addSpellBonus = 0.0, bool applyingDamageOnCast = true);
int getSpellEffectDurationFromID(int spellID, Entity* parent, Stat* parentStats, Entity* magicSourceParticle, real_t addSpellBonus = 0.0);
int getSpellEffectDurationSecondaryFromID(int spellID, Entity* parent, Stat* parentStats, Entity* magicSourceParticle, real_t addSpellBonus = 0.0);
real_t getSpellPropertyFromID(spell_t::SpellBasePropertiesFloat prop, int spellID, Entity* parent, Stat* parentStats, Entity* magicSourceParticle, real_t addSpellBonus = 0.0);
int getSpellPropertyFromID(spell_t::SpellBasePropertiesInt prop, int spellID, Entity* parent, Stat* parentStats, Entity* magicSourceParticle, real_t addSpellBonus = 0.0);
void thrownItemUpdateSpellTrail(Entity& my, real_t _x, real_t _y);
const char* magicLightColorForSprite(Entity* my, int sprite, bool darker);
void doParticleEffectForTouchSpell(Entity& my, Entity* focalLimb, Monster monsterType);
bool magicOnSpellCastEvent(Entity* parent, Entity* projectile, int spellID, Uint32 eventType, int eventValue, bool allowedLevelup = true); // return true on level up
void freeSpells();

struct AOEIndicators_t
{
	static Uint32 uids;
	enum SurfaceCacheTypes : int
	{
		CACHE_NONE,
		CACHE_VORTEX,
		CACHE_CASTING,
		CACHE_BOOBY_TRAP,
		CACHE_BOOBY_TRAP2
	};
	struct Indicator_t
	{
		TempTexture* texture = nullptr;
		SDL_Surface* surfaceOld = nullptr;
		int radiusMax = 128;
		int radiusMin = 32;
		int radius = 32;
		int size = 132;
		int lifetime = 1;
		int gradient = 8;
		Uint32 uid = 0;
		bool loop = true;
		bool castingTarget = false;
		Uint32 framesPerTick = 1;
		Uint32 ticksPerUpdate = 1;
		Uint32 delayTicks = 0;
		Uint32 indicatorColor = 0xFFFFFFFF;
		real_t arc = 0.0;
		bool expired = false;
		int loopType = 0;
		Uint32 loopTicks = 0;
		Uint32 loopTimer = 0;
		real_t expireAlphaRate = 0.9;
		SurfaceCacheTypes cacheType = CACHE_NONE;

		struct PrevData_t
		{
			Uint8 r = 0;
			Uint8 g = 0;
			Uint8 b = 0;
			Uint8 a = 0;
			real_t radMin = 0.0;
			real_t radMax = 0.0;
			int size = 0;
		};
		PrevData_t prevData;

		void updateIndicator();
		Indicator_t(int _radiusMin, int _radiusMax, int _size, int _lifetime, Uint32 _uid)
		{
			radiusMax = _radiusMax;
			radiusMin = _radiusMin;
			radius = radiusMin;
			size = _size;
			lifetime = _lifetime;
			uid = _uid;
		}
		~Indicator_t()
		{
			if ( texture )
			{
				delete texture;
				texture = nullptr;
			}
			if ( surfaceOld )
			{
				if ( cacheType == CACHE_NONE )
				{
					SDL_FreeSurface(surfaceOld);
				}
				surfaceOld = nullptr;
			}
		}
	};
	static std::map<int, std::map<std::tuple<Uint8, Uint8, Uint8, Uint8, real_t, real_t, int>, SDL_Surface*>> surfaceCache;
	static void cleanup();
	static std::map<Uint32, Indicator_t> indicators;

	static TempTexture* getTexture(Uint32 uid)
	{
		auto find = indicators.find(uid);
		if ( find != indicators.end() )
		{
			return find->second.texture;
		}
		return nullptr;
	}
	static SDL_Surface* getSurface(Uint32 uid)
	{
		auto find = indicators.find(uid);
		if ( find != indicators.end() )
		{
			return find->second.surfaceOld;
		}
		return nullptr;
	}
	static Indicator_t* getIndicator(Uint32 uid)
	{
		auto find = indicators.find(uid);
		if ( find != indicators.end() )
		{
			return &find->second;
		}
		return nullptr;
	}
	static void update();
	static Uint32 createIndicator(int _radiusMin, int _radiusMax, int _size, int _lifetime);
};