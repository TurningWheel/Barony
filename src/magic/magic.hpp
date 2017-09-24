/*-------------------------------------------------------------------------------

	BARONY
	File: magic.hpp
	Desc: Defines magic related stuff.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

//TODO: Spell icons: http://game-icons.net/

#pragma once

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


#define SPELLELEMENT_CONFUSE_BASE_DURATION 2//In seconds.
#define SPELLELEMENT_BLEED_BASE_DURATION 10//In seconds.
#define SPELLELEMENT_STONEBLOOD_BASE_DURATION 5//In seconds.
static const int SPELLELEMENT_ACIDSPRAY_BASE_DURATION = 10;

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

void addSpell(int spell, int player, bool ignoreSkill = false); //Adds a spell to the client's spell list. Note: Do not use this to add custom spells.

//TODO: Create a spell class which has the basic spell(s) involved, the mana to use etc. All of those important details. This should support vanilla spells and custom spells with just one data type. The addSpell function gives the player a vanilla spell if they don't already have it.

typedef struct spellElement_t spellElement_t;

//TODO: Give spellElements/spells a property that makes it so players don't know they exist/can't use them. Sorta like "You have no idea on the inner workings of this spell. It is beyond your comprehension."
//TODO: Channeling spells. Or spells that otherwise impose a constant drain until you cancel them. NEED THIS ASAP I SAY.
//TODO: Don't re-invent the wheel with lists here.
typedef struct spellElement_t
{
	int mana, base_mana;
	int overload_multiplier; // what does this do?
	int damage;
	int duration; // travel time if it's a missile element, duration for a light spell, duration for curses/enchants/traps/beams/rays/effects/what have you.
	char name[64];
	bool can_be_learned; // if a spellElement can't be learned, a player won't be able to build spells with it.
	bool channeled; // false by default. Specific spells can set this to true. Channeling it sustains the effect in some fashion. It reconsumes the casting mana after every duration has expired.
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
	char name[64];
	//spellElement_t *elements;
	int difficulty; //The proficiency you need in the magic skill to learn this spell. //TODO: Should this instead be determined by the spell elements?
	//int skill_caster; //The spellcasting skill it was cast with. Lower skill can introduce inefficiencies and other !!FUN!!
	bool sustain; //If a spell is channeled, should it be sustained? (NOTE: True by default. Set to false when the player decides to cancel/abandon a spell)
	bool magicstaff; // if true the spell was cast from a magicstaff and thus it may have slightly different behavior
	node_t* sustain_node; //Node in the sustained/channeled spells list.
	node_t* magic_effects_node;
	Uint32 caster;
	int channel_duration; //This is the value to reset the timer to when a spell is channeled.
	list_t elements; //NOTE: This could technically allow a spell to have multiple roots. So you could make a flurry of fireballs, for example.
	//TODO: Some way to make spells work with "need to cast more to get better at casting the spell." A sort of spell learning curve. The first time you cast it, prone to failure. Less the more you cast it.
};

extern list_t spellList; //All of the player's spells are stored here.
extern spell_t* selected_spell; //The spell the player's currently selected.
extern list_t channeledSpells[4]; //Spells the player is currently channeling. //TODO: Universalize it for all entities that can cast spells? //TODO: Cleanup and stuff.

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
//TODO: Armor/protection/warding spells.
//TODO: Targeting method?

void setupSpells();

void equipSpell(spell_t* spell, int playernum);
Entity* castSpell(Uint32 caster_uid, spell_t* spell, bool using_magicstaff, bool trap);
void castSpellInit(Uint32 caster_uid, spell_t* spell); //Initiates the spell animation, then hands off the torch to it, which, when finished, calls castSpell.

void actMagicTrap(Entity* my);
void actMagicStatusEffect(Entity* my);
void actMagicMissile(Entity* my);
void actMagicClient(Entity* my);
void actMagicClientNoLight(Entity* my);
void actMagicParticle(Entity* my);
Entity* spawnMagicParticle(Entity* parentent);
void spawnMagicEffectParticles(Sint16 x, Sint16 y, Sint16 z, Uint32 sprite);
void createParticle1(Entity* caster, int player);
void createParticle2(Entity* parent);
void actParticleCircle(Entity* my);
void actParticleDot(Entity* my);
void actParticleRock(Entity* my);
void actParticleTest(Entity* my);
void actParticleErupt(Entity* my);
void actParticleTimer(Entity* my);
void actParticleSap(Entity* my);
void actParticleSapCenter(Entity* my);

void createParticleDropRising(Entity* parent, int sprite);
void createParticleDot(Entity* parent);
void createParticleRock(Entity* parent);
void createParticleErupt(Entity* parent, int sprite);
void createParticleSapCenter(Entity* parent, real_t startx, real_t starty, int sprite, int endSprite);
void createParticleSap(Entity* parent);

spell_t* newSpell();
spell_t* copySpell(spell_t* spell);
void spellConstructor(spell_t* spell);
void spellDeconstructor(void* data);
spellElement_t* newSpellElement();
spellElement_t* copySpellElement(spellElement_t* spellElement);
void spellElementConstructor(spellElement_t* element);
void spellElementDeconstructor(void* data);

int getCostOfSpell(spell_t* spell);
int getCostOfSpellElement(spellElement_t* spellElement);
bool spell_isChanneled(spell_t* spell);
bool spellElement_isChanneled(spellElement_t* spellElement);

spell_t* getSpellFromID(int ID);

bool spellInList(list_t* list, spell_t* spell);

//-----Implementations of spell effects-----
void spell_magicMap(int player); //Magics the map. I mean maps the magic. I mean magically maps the level.
void spell_summonFamiliar(int player); // summons some familiars.
void spell_changeHealth(Entity* entity, int amount); //This function changes an entity's health.

//-----Spell Casting Animation-----
//The two hand animation functions.
void actLeftHandMagic(Entity* my);
void actRightHandMagic(Entity* my);

typedef struct spellcastingAnimationManager
{
	//The data to pass on to the castSpell function.
	spell_t* spell;
	Uint32 caster;

	bool active;
	int stage; //The current stage of the animation.
	int circle_count; //How many times it's circled around in the circle stage.
	int times_to_circle; //How many times to circle around in the circle stage.


	int consume_interval; //Every consume_interval ticks, eat a mana.
	int consume_timer; //How many ticks left till next mana consume.
	int mana_left; //How much mana is left to consume.
	bool consumeMana; //If false, goes through the motions, even casts the spell -- just doesn't consume any mana.

	float lefthand_movex;
	float lefthand_movey;
	float lefthand_angle;
} spellcasting_animation_manager_t;
extern spellcasting_animation_manager_t cast_animation;

void fireOffSpellAnimation(spellcasting_animation_manager_t* animation_manager, Uint32 caster_uid, spell_t* spell);
extern Entity* magicLeftHand;
extern Entity* magicRightHand;
void spellcastingAnimationManager_deactivate(spellcasting_animation_manager_t* animation_manager);
void spellcastingAnimationManager_completeSpell(spellcasting_animation_manager_t* animation_manager);

class Item;

spell_t* getSpellFromItem(Item* item);

//Spell implementation stuff.
bool spellEffectDominate(Entity& my, spellElement_t& element, Entity& caster, Entity* parent);
void spellEffectAcid(Entity& my, spellElement_t& element, Entity* parent, int resistance);

void freeSpells();
