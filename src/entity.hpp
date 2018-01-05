/*-------------------------------------------------------------------------------

	BARONY
	File: entity.hpp
	Desc: contains entity related declarations

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include "main.hpp"
#include "game.hpp"
#include "light.hpp"
#include "stat.hpp"
#include "monster.hpp"

// entity flags
#define BRIGHT 1
#define INVISIBLE 2
#define NOUPDATE 3
#define UPDATENEEDED 4
#define GENIUS 5
#define OVERDRAW 6
#define SPRITE 7
#define BLOCKSIGHT 8
#define BURNING 9
#define BURNABLE 10
#define UNCLICKABLE 11
#define PASSABLE 12
#define USERFLAG1 14
#define USERFLAG2 15

// number of entity skills and fskills
static const int NUMENTITYSKILLS = 60;
static const int NUMENTITYFSKILLS = 30;

struct spell_t;

// entity class
class Entity
{
	Sint32& char_gonnavomit;
	Sint32& char_heal;
	Sint32& char_energize;
	Sint32& char_torchtime;
	Sint32& char_poison;
	Sint32& char_fire;		// skill[36] - Counter for how many ticks Entity will be on fire
	Sint32& circuit_status;	// Use CIRCUIT_OFF and CIRCUIT_ON.
	Sint32& switch_power;	// Switch/mechanism power status.

	Sint32& chanceToPutOutFire; // skill[37] - Value between 5 and 10, with 10 being the default starting chance, and 5 being absolute minimum

	//Chest skills.
	//skill[0]
	Sint32& chestInit;
	//skill[1]
	//0 = closed. 1 = open.
	//0 = closed. 1 = open.
	Sint32& chestStatus;
	//skill[2] is reserved for all entities.
	//skill[3]
	Sint32& chestHealth;
	//skill[5]
	//Index of the player the chest was opened by.
	Sint32& chestOpener;
	//skill[6]
	Sint32& chestLidClicked;
	//skill[7]
	Sint32& chestAmbience;
	//skill[8]
	Sint32& chestMaxHealth;
	//skill[9]
	//field to be set if the chest sprite is 75-81 in the editor, otherwise should stay at value 0
	Sint32& chestType;

	// Power crystal skills
	Sint32& crystalInitialised; // 1 if init, else 0
	Sint32& crystalTurning; // 1 if currently rotating, else 0
	Sint32& crystalTurnStartDir; // when rotating, the previous facing direction stored here 0-3

	Sint32& crystalGeneratedElectricityNodes; // 1 if electricity nodes generated previously, else 0
	Sint32& crystalHoverDirection; // animation, waiting/up/down floating state
	Sint32& crystalHoverWaitTimer; // animation, if waiting state, then wait this many ticks before moving to next state

	// Pedestal Orb skills
	Sint32& orbInitialised; // 1 if init, else 0
	Sint32& orbHoverDirection; // animation, waiting/up/down floating state
	Sint32& orbHoverWaitTimer; // animation, if waiting state, then wait this many ticks before moving to next state


	//### Begin - Private Entity Constants for BURNING Status Effect
	static const Sint32 MIN_TICKS_ON_FIRE		= TICKS_TO_PROCESS_FIRE *  4; // Minimum time an Entity can be on fire is  4 cycles (120 ticks)
	static const Sint32 MAX_TICKS_ON_FIRE		= TICKS_TO_PROCESS_FIRE * 20; // Maximum time an Entity can be on fire is 20 cycles (600 ticks)
	static const Sint32 MIN_CHANCE_STOP_FIRE	= 5;	// Minimum chance an Entity has to stop being on fire is 1 in  5
	static const Sint32 MAX_CHANCE_STOP_FIRE	= 10;	// Maximum chance an Entity has to stop being on fire is 1 in 10

	// Maximum level of CON needed to get MIN_CHANCE_STOP_FIRE. Every 5 points = 1 increase in chance up to MIN_CHANCE_STOP_FIRE
	static const Sint32 MAX_CON_FOR_STOP_FIRE = 5 * MIN_CHANCE_STOP_FIRE;	
	// Maximum level of CON needed to get MIN_TICKS_ON_FIRE. Every 2 points = 1 second decrease in time up to MIN_TICKS_ON_FIRE
	static const Sint32 MAX_CON_FOR_FIRE_TIME = (2 * (MAX_TICKS_ON_FIRE - MIN_TICKS_ON_FIRE)) / TICKS_TO_PROCESS_FIRE;
	//### End   - Private Entity Constants for BURNING Status Effect

	static const int CRYSTAL_HOVER_UP = 0;
	static const int CRYSTAL_HOVER_UP_WAIT = 1;
	static const int CRYSTAL_HOVER_DOWN = 2;
	static const int CRYSTAL_HOVER_DOWN_WAIT = 3;

	//--- Mechanism defines ---
	static const int CIRCUIT_OFF = 1;
	static const int CIRCUIT_ON = 2;

	static const int SWITCH_UNPOWERED = 0;
	static const int SWITCH_POWERED = 1;
	Uint32 uid;                    // entity uid
public:
	Entity(Sint32 in_sprite, Uint32 pos, list_t* entlist, list_t* creaturelist);
	~Entity();


	Uint32 getUID() const {return uid;}
	void setUID(Uint32 new_uid);
	Uint32 ticks;                  // duration of the entity's existence
	real_t x, y, z;                // world coordinates
	real_t yaw, pitch, roll;       // rotation
	real_t focalx, focaly, focalz; // focal point for rotation, movement, etc.
	real_t scalex, scaley, scalez; // stretches/squashes the entity visually
	Sint32 sizex, sizey;           // entity bounding box size
	Sint32 sprite;                 // the entity's sprite index

	// network stuff
	Uint32 lastupdate;                   // last time since the entity was updated
	Uint32 lastupdateserver;             // used to sort out old packets
	real_t vel_x, vel_y, vel_z;          // entity velocity vector
	real_t new_x, new_y, new_z;          // world coordinates
	real_t new_yaw, new_pitch, new_roll; // rotation

	// entity attributes
	real_t fskill[NUMENTITYFSKILLS]; // floating point general purpose variables
	Sint32 skill[NUMENTITYSKILLS];  // general purpose variables
	bool flags[16];    // engine flags
	char* string;      // general purpose string
	light_t* light;    // every entity has a specialized light pointer
	list_t children;   // every entity has a list of child objects
	Uint32 parent;     // id of the entity's "parent" entity

	//--PUBLIC CHEST SKILLS--

	//skill[4]
	//0 = unlocked. 1 = locked.
	Sint32& chestLocked;
	/*
	 * skill[10]
	 * 1 = chest already has been unlocked, or spawned in unlocked (prevent spell exploit)
	 * 0 = chest spawned in locked and is still ripe for harvest.
	 * Purpose: To prevent exploits with repeatedly locking and unlocking a chest.
	 * Also doesn't spawn gold for chests that didn't spawn locked
	 * (e.g. you locked a chest with a spell...sorry, no gold for you)
	 */
	Sint32& chestPreventLockpickCapstoneExploit;

	//--PUBLIC MONSTER SKILLS--
	Sint32& monsterState; //skill[0]
	Sint32& monsterTarget; //skill[1]
	real_t& monsterTargetX; //fskill[2]
	real_t& monsterTargetY; //fskill[3]
	Sint32& monsterSpecialTimer; //skill[29]
	//Only used by goatman.
	Sint32& monsterSpecialState; //skill[33]
	Sint32& monsterSpellAnimation;
	Sint32& monsterFootstepType;
	Sint32& monsterLookTime;
	Sint32& monsterAttack;
	Sint32& monsterAttackTime;
	Sint32& monsterArmbended;
	real_t& monsterWeaponYaw;
	Sint32& monsterMoveTime;
	Sint32& monsterHitTime;
	Sint32& monsterPathBoundaryXStart;
	Sint32& monsterPathBoundaryYStart;
	Sint32& monsterPathBoundaryXEnd;
	Sint32& monsterPathBoundaryYEnd;
	Sint32& monsterStoreType;

	real_t& monsterLookDir;

	//--PUBLIC MONSTER ANIMATION SKILLS--
	Sint32& monsterAnimationLimbDirection;
	Sint32& monsterAnimationLimbOvershoot;

	//--PUBLIC MONSTER SHADOW SKILLS--
	Sint32& monsterShadowInitialMimic; //skill[34]. 0 = false, 1 = true.
	Sint32& monsterShadowDontChangeName; //skill[35]. 0 = false, 1 = true. Doesn't change name in its mimic if = 1.

	//--PUBLIC POWER CRYSTAL SKILLS--
	Sint32& crystalTurnReverse; // 0 Clockwise, 1 Anti-Clockwise
	Sint32& crystalNumElectricityNodes; // how many nodes to spawn in the facing dir
	Sint32& crystalSpellToActivate; // If 1, must be hit by unlocking spell to start generating electricity.

	real_t& crystalStartZ; // mid point of animation, starting height.
	real_t& crystalMaxZVelocity;
	real_t& crystalMinZVelocity;
	real_t& crystalTurnVelocity; // how fast to turn on click.

	//--PUBLIC GATE SKILLS--
	Sint32& gateInit;
	Sint32& gateStatus;
	Sint32& gateRattle;
	real_t& gateStartHeight;
	real_t& gateVelZ;
	Sint32& gateInverted;

	//--PUBLIC LEVER SKILLS--
	Sint32& leverTimerTicks;
	Sint32& leverStatus;

	//--PUBLIC BOULDER TRAP SKILLS--
	Sint32& boulderTrapRefireAmount;
	Sint32& boulderTrapRefireDelay;
	Sint32& boulderTrapAmbience;
	Sint32& boulderTrapFired;
	Sint32& boulderTrapRefireCounter;
	Sint32& boulderTrapPreDelay;

	//--PUBLIC AMBIENT PARTICLE EFFECT SKILLS--
	Sint32& particleDuration;
	Sint32& particleShrink;

	//--PUBLIC PARTICLE TIMER EFFECT SKILLS--
	Sint32& particleTimerDuration;
	Sint32& particleTimerEndAction;
	Sint32& particleTimerEndSprite;
	Sint32& particleTimerCountdownAction;
	Sint32& particleTimerCountdownSprite;
	Sint32& particleTimerTarget;
	Sint32& particleTimerPreDelay;
	Sint32& particleTimerVariable1;

	//--PUBLIC DOOR SKILLS--
	Sint32& doorDir;
	Sint32& doorInit;
	Sint32& doorStatus;
	Sint32& doorHealth;
	Sint32& doorLocked;
	Sint32& doorSmacked;
	Sint32& doorTimer;
	Sint32& doorOldStatus;
	Sint32& doorMaxHealth;
	real_t& doorStartAng;

	//--PUBLIC PEDESTAL SKILLS--
	Sint32& pedestalHasOrb;
	Sint32& pedestalOrbType;
	Sint32& pedestalInvertedPower;
	Sint32& pedestalInGround;
	Sint32& pedestalInit;
	Sint32& pedestalAmbience;
	Sint32& pedestalLockOrb;

	real_t& orbStartZ; // mid point of animation, starting height.
	real_t& orbMaxZVelocity;
	real_t& orbMinZVelocity;
	real_t& orbTurnVelocity; // how fast to turn.

	//--PUBLIC PORTAL SKILLS--
	Sint32& portalAmbience;;
	Sint32& portalInit;
	Sint32& portalNotSecret;
	Sint32& portalVictoryType;
	Sint32& portalFireAnimation;

	//--PUBLIC TELEPORTER SKILLS--
	Sint32& teleporterX;
	Sint32& teleporterY;
	Sint32& teleporterType;
	Sint32& teleporterAmbience;

	//--PUBLIC CEILING TILE SKILLS--
	Sint32& ceilingTileModel;

	//--PUBLIC SPELL TRAP SKILLS--
	Sint32& spellTrapType;
	Sint32& spellTrapRefire;
	Sint32& spellTrapLatchPower;
	Sint32& spellTrapFloorTile;
	Sint32& spellTrapRefireRate;
	Sint32& spellTrapAmbience;
	Sint32& spellTrapInit;
	Sint32& spellTrapCounter;
	Sint32& spellTrapReset;
	
	//--PUBLIC FURNITURE SKILLS--
	Sint32& furnitureType;
	Sint32& furnitureInit;
	Sint32& furnitureDir;
	Sint32& furnitureHealth;
	Sint32& furnitureMaxHealth;

	//--PUBLIC PISTON SKILLS--
	Sint32& pistonCamDir;
	Sint32& pistonCamTimer;
	real_t& pistonCamRotateSpeed;

	//--PUBLIC ARROW/PROJECTILE SKILLS--
	Sint32& arrowPower;
	Sint32& arrowPoisonTime;
	Sint32& arrowArmorPierce;

	//--PUBLIC ITEM SKILLS--
	Sint32& itemNotMoving; // skill[18]
	Sint32& itemNotMovingClient; // skill[19]

	void pedestalOrbInit(); // init orb properties

	// a pointer to the entity's location in a list (ie the map list of entities)
	node_t* mynode;
	node_t* myCreatureListNode;

	list_t* path; // pathfinding stuff. Most of the code currently stuffs that into children, but the magic code makes use of this variable instead.

	//Dummy stats to make certain visual features work on clients (such as ambient particles for magic reflection).
	Stat* clientStats;
	bool clientsHaveItsStats;
	void giveClientStats();

	// behavior function pointer
	void (*behavior)(class Entity* my);
	bool ranbehavior;

	void setObituary(char* obituary);

	char* getMonsterLangEntry();

	void killedByMonsterObituary(Entity* victim);

	Sint32 getSTR();
	Sint32 getDEX();
	Sint32 getCON();
	Sint32 getINT();
	Sint32 getPER();
	Sint32 getCHR();

	int entityLight(); //NOTE: Name change conflicted with light_t *light
	int entityLightAfterReductions(Stat& myStats, Entity& observer);

	void handleEffects(Stat* myStats);
	void handleEffectsClient();

	void effectTimes();
	void increaseSkill(int skill);

	Stat* getStats() const;

	void setHP(int amount);
	void modHP(int amount); //Adds amount to HP.

	void setMP(int amount);
	void modMP(int amount); //Adds amount to MP.
	int getMP();

	void drainMP(int amount); //Removes this much from MP. Anything over the entity's MP is subtracted from their health. Can be very dangerous.
	bool safeConsumeMP(int amount); //A function for the magic code. Attempts to remove mana without overdrawing the player. Returns true if success, returns false if didn't have enough mana.

	Sint32 getAttack();
	Sint32 getBonusAttackOnTarget(Stat& hitstats);
	Sint32 getRangedAttack();
	Sint32 getThrownAttack();
	bool isBlind();
	bool isSpellcasterBeginner();

	bool isInvisible() const;

	bool isMobile();

	void attack(int pose, int charge, Entity* target);

	bool teleport(int x, int y);
	bool teleportRandom();
	// teleport entity to a target, within a radius dist (range in whole tile lengths)
	bool teleportAroundEntity(const Entity* target, int dist);
	// teleport entity to fixed position with appropriate sounds, for actTeleporter.
	bool teleporterMove(int x, int y, int type);

	//void entityAwardXP(Entity *dest, Entity *src, bool share, bool root);
	void awardXP(Entity* src, bool share, bool root);

	//--*CheckBetterEquipment functions--
	void checkBetterEquipment(Stat* myStats);
	void checkGroundForItems();
	bool canWieldItem(const Item& item) const;
	bool goblinCanWieldItem(const Item& item) const;
	bool humanCanWieldItem(const Item& item) const;
	bool goatmanCanWieldItem(const Item& item) const;
	bool automatonCanWieldItem(const Item& item) const;
	bool shadowCanWieldItem(const Item& item) const;
	bool insectoidCanWieldItem(const Item& item) const;

	bool monsterWantsItem(const Item& item, Item**& shouldEquip, node_t*& replaceInventoryItem) const;

	void createPathBoundariesNPC();
	void humanSetLimbsClient(int bodypart);

	/*
	 * Check if the goatman can wield the item, and if so, is it something it wants? E.g. does it really want to carry 2 sets of armor?
	 */
	//bool goatmanWantsItem(const Item& item, Item*& shouldWield, node_t*& replaceInventoryItem) const;

	bool shouldMonsterEquipThisWeapon(const Item& itemToEquip) const;//TODO: Look @ proficiencies.
	Item** shouldMonsterEquipThisArmor(const Item& item) const;

	void removeLightField(); // Removes light field from entity, sets this->light to nullptr.

	//--- Mechanism functions ---
	void circuitPowerOn(); //Called when a nearby circuit or switch powers on.
	void circuitPowerOff(); //Called when a nearby circuit or switch powers off.
	void updateCircuitNeighbors(); //Called when a circuit's powered state changes.
	void mechanismPowerOn(); //Called when a circuit or switch next to a mechanism powers on.
	void mechanismPowerOff(); //Called when a circuit or switch next to a mechanism powers on.
	void toggleSwitch(); //Called when a player flips a switch (lever).
	void switchUpdateNeighbors(); //Run each time actSwitch() is called to make sure the network is online if any one switch connected to it is still set to the on position.
	list_t* getPowerableNeighbors(); //Returns a list of all circuits and mechanisms this entity can influence.

	//Chest/container functions.
	void closeChest();
	void closeChestServer(); //Close the chest serverside, silently. Called when the chest is closed somewhere else for that client, but the server end stuff needs to be tied up.
	void addItemToChest(Item* item); //Adds an item to the chest. If server, notifies the client. If client, notifies the server.
	Item* getItemFromChest(Item* item, bool all, bool getInfoOnly = false); //Removes an item from the chest and returns a pointer to it.
	void addItemToChestFromInventory(int player, Item* item, bool all);
	void addItemToChestServer(Item* item); //Adds an item to the chest. Called when the server receives a notification from the client that an item was added to the chest.
	void removeItemFromChestServer(Item* item, int count); //Called when the server learns that a client removed an item from the chest.
	void unlockChest();
	void lockChest();
	void chestHandleDamageMagic(int damage, Entity &magicProjectile, Entity *caster);

	//Power Crystal functions.
	void powerCrystalCreateElectricityNodes();

	//Door functions.
	void doorHandleDamageMagic(int damage, Entity &magicProjectile, Entity *caster);

	bool checkEnemy(Entity* your);
	bool checkFriend(Entity* your);

	//Act functions.
	void actChest();
	void actPowerCrystal();
	void actGate();
	void actPedestalBase();
	void actPedestalOrb();
	void actMidGamePortal();
	void actTeleporter();
	void actMagicTrapCeiling();
	bool magicFallingCollision();
	void actFurniture();
	void actPistonCam();
	void actStalagCeiling();
	void actStalagFloor();
	void actStalagColumn();
	void actColumn();

	Monster getRace() const
	{
		Stat* myStats = getStats();

		if ( !myStats )
		{
			return NOTHING;
		}

		return myStats->type;
	}

	bool inline skillCapstoneUnlockedEntity(int proficiency) const
	{
		if ( !getStats() )
		{
			return false;
		}

		return (getStats()->PROFICIENCIES[proficiency] >= CAPSTONE_UNLOCK_LEVEL[proficiency]);
	}

	/*
	 * Returns -1 if not a player.
	 */
	int isEntityPlayer() const;

	void initMonster(int mySprite);

	//--monster type from sprite
	int getMonsterTypeFromSprite();
	//--monster helmet limb offsets
	void setHelmetLimbOffset(Entity* helm);

	void actMonsterLimb(bool processLight = false);

	void removeMonsterDeathNodes();

	void spawnBlood(int bloodsprite = 160);

	// reflection is set 1, 2 or 3 depending on the item slot. reflection of 3 does not degrade.
	int getReflection() const;
	// monster attack pose, return the animation to use based on weapon.
	int getAttackPose() const;
	// if monster holding ranged weapon.
	bool hasRangedWeapon() const;
	// weapon arm animation attacks
	void handleWeaponArmAttack(Entity* weaponarm);
	// handle walking movement for arms and legs
	void humanoidAnimateWalk(Entity* limb, node_t* bodypartNode, int bodypart, double walkSpeed, double dist, double distForFootstepSound);
	// monster footsteps, needs to be client friendly
	Uint32 getMonsterFootstepSound(int footstepType, int bootSprite);
	// handle humanoid weapon arm animation/sprite offsets
	void handleHumanoidWeaponLimb(Entity* weaponLimb, Entity* weaponArmLimb);
	// server only function to set boot sprites on monsters.
	bool setBootSprite(Entity* leg, int spriteOffset);
	// monster special attack handler, returns true if monster should attack after calling this function.
	bool handleMonsterSpecialAttack(Stat* myStats, Entity* target, double dist);
	// monster attack handler
	void handleMonsterAttack(Stat* myStats, Entity* target, double dist);
	void lookAtEntity(Entity& target);
	// automaton specific function
	void automatonRecycleItem();
	// incubus teleport spells
	void incubusTeleportToTarget(const Entity* target);
	void incubusTeleportRandom();
	//Shadow teleport spells.
	void shadowTeleportToTarget(const Entity* target, int range);
	// check for nearby items to add to monster's inventory
	void monsterAddNearbyItemToInventory(Stat* myStats, int rangeToFind, int maxInventoryItems);
	// degrade chosen armor piece by 1 on entity, update clients.
	void degradeArmor(Stat& hitstats, Item& armor, int armornum);
	// check stats if monster should "retreat" in actMonster
	bool shouldRetreat(Stat& myStats);
	// check if monster should retreat or stand still when less than given distance
	bool backupWithRangedWeapon(Stat& myStats, int dist, int hasrangedweapon);
	// calc time required for a mana regen tick.
	int getManaRegenInterval(Stat& myStats); 
	// calc damage/effects for ranged weapons.
	void setRangedProjectileAttack(Entity& marksman, Stat& myStats);
	real_t yawDifferenceFromPlayer(int player); // calc targets yaw compared to a player, returns 0 - 2 * PI, where > PI is facing towards player.
	spell_t* getActiveMagicEffect(int spellID);

	/*
	 * 1 in @chance chance in spawning a particle with the given sprite and duration.
	 */
	void spawnAmbientParticles(int chance, int particleSprite, int duration, double particleScale, bool shrink);

	//Updates the EFFECTS variable for all clients for this entity.
	void serverUpdateEffectsForEntity(bool guarantee);

	/*
	 * If set on a player, will call serverUpdateEffects() on the player.
	 * @param guarantee: Causes serverUpdateEffectsForEntity() to use sendPacketSafe() rather than just sendPacket().
	 */
	void setEffect(int effect, bool value, int duration, bool updateClients, bool guarantee = true);

	/*
	 * @param state: required to let the entity know if it should enter MONSTER_STATE_PATH, MONSTER_STATE_ATTACK, etc.
	 */
	void monsterAcquireAttackTarget(const Entity& target, Sint32 state);

	/*
	 * Attempts to set the target to 0.
	 * May refuses to do so and consequently return false in cases such as the shadow, which cannot lose its target until it's dead.
	 * Returns true otherwise, if successfully zero-d out target.
	 */
	bool monsterReleaseAttackTarget(bool force = false);

	//Lets monsters swap out weapons.
	void inline chooseWeapon(const Entity* target, double dist)
	{
		Stat* myStats = getStats();
		if ( !myStats )
		{
			return;
		}

		switch ( myStats->type )
		{
			case GOATMAN:
				goatmanChooseWeapon(target, dist);
				break;
			case INSECTOID:
				insectoidChooseWeapon(target, dist);
				break;
			case INCUBUS:
				incubusChooseWeapon(target, dist);
				break;
			case VAMPIRE:
				vampireChooseWeapon(target, dist);
				break;
			case SHADOW:
				shadowChooseWeapon(target, dist);
				break;
			default:
				break;
		}
	}
	void goatmanChooseWeapon(const Entity* target, double dist);
	void insectoidChooseWeapon(const Entity* target, double dist);
	void incubusChooseWeapon(const Entity* target, double dist);
	void vampireChooseWeapon(const Entity* target, double dist);
	void shadowChooseWeapon(const Entity* target, double dist);

	bool monsterInMeleeRange(const Entity* target, double dist) const
	{
		return (dist < STRIKERANGE);
	}

	node_t* addItemToMonsterInventory(Item* item);

	//void returnWeaponarmToNeutral(Entity* weaponarm, Entity* rightbody); //TODO: Need a proper refactor?

	void shadowSpecialAbility(bool initialMimic);

	bool shadowCanMimickSpell(int spellID);

	double monsterRotate();

	//TODO: These two won't work with multiplayer because clients are stubborn little tater tots that refuse to surrender their inventories on demand.
	//Here's the TODO: Fix it.
	Item* getBestMeleeWeaponIHave() const;
	Item* getBestShieldIHave() const;

	void monsterEquipItem(Item& item, Item** slot);

	bool monsterHasSpellbook(int spellbookType);
	//bool monsterKnowsSpell(int spellID); //TODO: Should monsters use the spell item instead of spellbooks?
	node_t* chooseAttackSpellbookFromInventory();

	/* entity.cpp
	 * Attempts to set the Entity on fire. Entities that are not Burnable or are already on fire will return before any processing
	 * Entities that do not have Stats (such as furniture) will return after setting the fire time and chance to stop at max
	 * Entities with Stats will have their fire time (char_fire) and chance to stop being on fire (chanceToPutOutFire) reduced by their CON
	 * Calculations for reductions is outlined in this function
	 */
	void SetEntityOnFire();

	void addToCreatureList(list_t* list);
	std::vector<Entity*> bodyparts;
};

extern list_t entitiesToDelete[MAXPLAYERS];
extern Uint32 entity_uids, lastEntityUIDs;
//extern Entity *players[4];
extern Uint32 nummonsters;

#define CHAR_POISON my->skill[21] //TODO: Being replaced with Entity char_poison
#define CHAR_HEAL my->skill[22] //TODO: Being replaced with Entity::char_heal
#define CHAR_ENERGIZE my->skill[23] //TODO: Being replaced with Entity::char_energize
#define CHAR_DRUNK my->skill[24]
#define CHAR_TORCHTIME my->skill[25] //TODO: Being replaced with Entity::char_torchtime
#define CHAR_GONNAVOMIT my->skill[26] //TODO: Being replaced with Entity::char_gonnavomit

class Item;

extern bool swornenemies[NUMMONSTERS][NUMMONSTERS];
extern bool monsterally[NUMMONSTERS][NUMMONSTERS];

Sint32 statGetSTR(Stat* entitystats);
Sint32 statGetDEX(Stat* entitystats);
Sint32 statGetCON(Stat* entitystats);
Sint32 statGetINT(Stat* entitystats);
Sint32 statGetPER(Stat* entitystats);
Sint32 statGetCHR(Stat* entitystats);
int AC(Stat* stat);

Entity* uidToEntity(Sint32 uidnum);
list_t* checkTileForEntity(int x, int y); //Don't forget to free the list returned when you're done with it. Also, provide x and y in map, not entity, units.
/*
 * Don't forget to free the list returned when you're done with it.
 * Provide x and y in map, not entity, units.
 * The list parameter is a pointer to the list all the items found will be appended to.
 */
void getItemsOnTile(int x, int y, list_t** list);

//--- Entity act* functions ---
void actMonster(Entity* my);
void actPlayer(Entity* my);

/*
 * NOTE: Potion effects
 * value 0 = POTION_WATER
 * value 1 = POTION_BOOZE
 * value 2 = POTION_JUICE
 * value 3 = POTION_SICKNESS
 * value 4 = POTION_CONFUSION
 * value 5 = POTION_EXTRAHEALING
 * value 6 = POTION_HEALING
 * value 7 = POTION_RESTORABILITY
 * value 8 = POTION_BLINDNESS
 * value 9 = POTION_RESTOREMAGIC
 * value 10 = POTION_INVISIBILITY
 * value 11 = POTION_LEVITATION
 * value 12 = POTION_SPEED
 * value 13 = POTION_ACID
 * value 14 = POTION_PARALYSIS
 */
//TODO: Allow for cursed fountains. Any fountain that has a negative effect has, say, skill[4] set to 1 to indicate cursed. Used for monster behavior and for effects of things like healing potions.
void actFountain(Entity* my);
void actSink(Entity* my);

//--- Mechanism functions ---
void actCircuit(Entity* my);
void actSwitch(Entity* my); //Needs to be called periodically to ensure network's powered state is correct.
void getPowerablesOnTile(int x, int y, list_t** list); //Stores a list of all circuits and mechanisms, on the tile (in map coordinates), in list.
void actGate(Entity* my);
void actArrowTrap(Entity* my);
void actTrap(Entity* my);
void actTrapPermanent(Entity* my);
void actSwitchWithTimer(Entity* my);

/*
 * Note: Circuits and mechanisms use skill[28] to signify powered state.
 * * If skill[28] == 0, it's not a mechanism (or circuit).
 * * If skill[28] == 1, it's powered off.
 * * If skill[28] == 2, it's powered on.
 * * Mechanism only: If skill[28] == 3, it's powered on and the entity already processed it. Sort of combining a mechanism->powered and mechanism->powered_last_frame variable into one. Not sure if it's necessary, but I thought it did when I came up with this, so there you have it.
 */

//---Chest/container functions---
void actChest(Entity* my);
void actChestLid(Entity* my);
void closeChestClientside(); //Called by the client to manage all clientside stuff relating to closing a chest.
void addItemToChestClientside(Item* item); //Called by the client to manage all clientside stuff relating to adding an item to a chest.

//---Stalag functions---
void actStalagFloor(Entity* my);
void actStalagCeiling(Entity* my);
void actStalagColumn(Entity* my);

//---Ceiling Tile functions---
void actCeilingTile(Entity* my);

//--Piston functions--
void actPistonBase(Entity* my);
void actPistonCam(Entity* my);

void actColumn(Entity* my);

//---Magic entity functions---
void actMagiclightBall(Entity* my);

//---Misc act functions---
void actAmbientParticleEffectIdle(Entity* my);

//checks if a sprite falls in certain sprite ranges

static const int NUM_ITEM_STRINGS = 219;
static const int NUM_ITEM_STRINGS_BY_TYPE = 90;
static const int NUM_EDITOR_SPRITES = 127;
static const int NUM_EDITOR_TILES = 234;

// furniture types.
static const int FURNITURE_TABLE = 0;
static const int FURNITURE_CHAIR = 1;
static const int FURNITURE_BED = 2;
static const int FURNITURE_BUNKBED = 3;
static const int FURNITURE_PODIUM = 4;

int checkSpriteType(Sint32 sprite);
extern char spriteEditorNameStrings[NUM_EDITOR_SPRITES][64];
extern char tileEditorNameStrings[NUM_EDITOR_TILES][44];
extern char monsterEditorNameStrings[NUMMONSTERS][13];
extern char itemStringsByType[10][NUM_ITEM_STRINGS_BY_TYPE][32];
extern char itemNameStrings[NUM_ITEM_STRINGS][32];
int canWearEquip(Entity* entity, int category);
void createMonsterEquipment(Stat* stats);
int countCustomItems(Stat* stats);
int countDefaultItems(Stat* stats);
void copyMonsterStatToPropertyStrings(Stat* tmpSpriteStats);
void setRandomMonsterStats(Stat* stats);

int checkEquipType(const Item *ITEM);

static const int SPRITE_GLOVE_RIGHT_OFFSET = 0;
static const int SPRITE_GLOVE_LEFT_OFFSET = 4;
static const int SPRITE_BOOT_RIGHT_OFFSET = 0;
static const int SPRITE_BOOT_LEFT_OFFSET = 2;

int setGloveSprite(Stat * myStats, Entity* ent, int spriteOffset);
bool isLevitating(Stat * myStats);
int getWeaponSkill(Item* weapon);
int getStatForProficiency(int skill);
void setSpriteAttributes(Entity* entityToSet, Entity* entityToCopy, Entity* entityStatToCopy);
void playerStatIncrease(int playerClass, int chosenStats[3]);

static const int MSG_DESCRIPTION = 0;
static const int MSG_COMBAT = 1;
static const int MSG_OBITUARY = 2;
static const int MSG_GENERIC = 3;
static const int MSG_ATTACKS = 4;
void messagePlayerMonsterEvent(int player, Uint32 color, Stat& monsterStats, char* msgGeneric, char* msgNamed, int detailType);
char* playerClassLangEntry(int classnum);
char* playerClassDescription(int classnum);

//Some testing functions/commands.
Entity* summonChest(long x, long y);

