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
#include "stat.hpp"

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

// entity class
class Entity {
	Sint32& char_gonnavomit;
	Sint32& char_heal;
	Sint32& char_energize;
	Sint32& char_torchtime;
	Sint32& char_poison;
	Sint32& monster_attack;
	Sint32& monster_attacktime;
	Sint32& monster_state;
	Sint32& monster_target;
	Sint32& circuit_status; //Use CIRCUIT_OFF and CIRCUIT_ON.
	Sint32& switch_power; //Switch/mechanism power status.

	Sint32& chest_status; //0 = closed. 1 = open.
	Sint32& chest_opener; //Index of the player the chest was opened by.

	//--- Mechanism defines ---
	static const int CIRCUIT_OFF = 1;
	static const int CIRCUIT_ON = 2;

	static const int SWITCH_UNPOWERED = 0;
	static const int SWITCH_POWERED = 1;
public:
	Entity(Sint32 in_sprite, Uint32 pos, list_t *entlist);
	~Entity();


	Uint32 uid;                    // entity uid
	Uint32 ticks;                  // duration of the entity's existence
	double x, y, z;                // world coordinates
	double yaw, pitch, roll;       // rotation
	double focalx, focaly, focalz; // focal point for rotation, movement, etc.
	double scalex, scaley, scalez; // stretches/squashes the entity visually
	Sint32 sizex, sizey;           // entity bounding box size
	Sint32 sprite;                 // the entity's sprite index

	int editorChestType;
	
	// network stuff
	Uint32 lastupdate;                   // last time since the entity was updated
	Uint32 lastupdateserver;             // used to sort out old packets
	double vel_x, vel_y, vel_z;          // entity velocity vector
	double new_x, new_y, new_z;          // world coordinates
	double new_yaw, new_pitch, new_roll; // rotation
	
	// entity attributes
	double fskill[30]; // floating point general purpose variables
	Sint32 skill[30];  // general purpose variables
	bool flags[16];    // engine flags
	char *string;      // general purpose string
	light_t *light;    // every entity has a specialized light pointer
	list_t children;   // every entity has a list of child objects
	Uint32 parent;     // id of the entity's "parent" entity
	
	// a pointer to the entity's location in a list (ie the map list of entities)
	node_t *mynode;

	list_t *path; // pathfinding stuff. Most of the code currently stuffs that into children, but the magic code makes use of this variable instead.

	// behavior function pointer
	void (*behavior)(class Entity *my);
	bool ranbehavior;

	void setObituary(char *obituary);

	void killedByMonsterObituary(Entity *victim);

	Sint32 getSTR();
	Sint32 getDEX();
	Sint32 getCON();
	Sint32 getINT();
	Sint32 getPER();
	Sint32 getCHR();

	int entityLight(); //NOTE: Name change conflicted with light_t *light

	void handleEffects(Stat *myStats);

	void effectTimes();
	void increaseSkill(int skill);

	Stat *getStats();

	void setHP(int amount);
	void modHP(int amount); //Adds amount to HP.

	void setMP(int amount);
	void modMP(int amount); //Adds amount to MP.

	void drainMP(int amount); //Removes this much from MP. Anything over the entity's MP is subtracted from their health. Can be very dangerous.
	bool safeConsumeMP(int amount); //A function for the magic code. Attempts to remove mana without overdrawing the player. Returns true if success, returns false if didn't have enough mana.

	Sint32 getAttack();
	bool isBlind();

	bool isInvisible();

	bool isMobile();

	void attack(int pose, int charge);

	void teleport(int x, int y);
	void teleportRandom();

	//void entityAwardXP(Entity *dest, Entity *src, bool share, bool root);
	void awardXP(Entity *src, bool share, bool root);

	//--*CheckBetterEquipment functions--
	void checkBetterEquipment(Stat *myStats);

	//--- Mechanism functions ---
	void circuitPowerOn(); //Called when a nearby circuit or switch powers on.
	void circuitPowerOff(); //Called when a nearby circuit or switch powers off.
	void updateCircuitNeighbors(); //Called when a circuit's powered state changes.
	void mechanismPowerOn(); //Called when a circuit or switch next to a mechanism powers on.
	void mechanismPowerOff(); //Called when a circuit or switch next to a mechanism powers on.
	void toggleSwitch(); //Called when a player flips a switch (lever).
	void switchUpdateNeighbors(); //Run each time actSwitch() is called to make sure the network is online if any one switch connected to it is still set to the on position.
	list_t *getPowerableNeighbors(); //Returns a list of all circuits and mechanisms this entity can influence.

	//Chest/container functions.
	void closeChest();
	void closeChestServer(); //Close the chest serverside, silently. Called when the chest is closed somewhere else for that client, but the server end stuff needs to be tied up.
	void addItemToChest(Item *item); //Adds an item to the chest. If server, notifies the client. If client, notifies the server.
	Item *getItemFromChest(Item *item, bool all); //Removes an item from the chest and returns a pointer to it.
	void addItemToChestFromInventory(int player, Item *item, bool all);
	void addItemToChestServer(Item *item); //Adds an item to the chest. Called when the server receives a notification from the client that an item was added to the chest.
	void removeItemFromChestServer(Item *item, int count); //Called when the server learns that a client removed an item from the chest.

	bool checkEnemy(Entity *your);
	bool checkFriend(Entity *your);
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

Sint32 statGetSTR(Stat *entitystats);
Sint32 statGetDEX(Stat *entitystats);
Sint32 statGetCON(Stat *entitystats);
Sint32 statGetINT(Stat *entitystats);
Sint32 statGetPER(Stat *entitystats);
Sint32 statGetCHR(Stat *entitystats);
int AC(Stat *stat);

Entity *uidToEntity(Uint32 uidnum);
list_t *checkTileForEntity(int x, int y); //Don't forget to free the list returned when you're done with it. Also, provide x and y in map, not entity, units.
/*
 * Don't forget to free the list returned when you're done with it.
 * Provide x and y in map, not entity, units.
 * The list parameter is a pointer to the list all the items found will be appended to.
 */
void getItemsOnTile(int x, int y, list_t **list);

//--- Entity act* functions ---


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
void actFountain(Entity *my);
void actSink(Entity *my);

//--- Mechanism functions ---
void actCircuit(Entity *my);
void actSwitch(Entity *my); //Needs to be called periodically to ensure network's powered state is correct.
void getPowerablesOnTile(int x, int y, list_t **list); //Stores a list of all circuits and mechanisms, on the tile (in map coordinates), in list.
void actGate(Entity *my);
void actArrowTrap(Entity *my);
void actTrap(Entity *my);
void actTrapPermanent(Entity *my);

/*
 * Note: Circuits and mechanisms use skill[28] to signify powered state.
 * * If skill[28] == 0, it's not a mechanism (or circuit).
 * * If skill[28] == 1, it's powered off.
 * * If skill[28] == 2, it's powered on.
 * * Mechanism only: If skill[28] == 3, it's powered on and the entity already processed it. Sort of combining a mechanism->powered and mechanism->powered_last_frame variable into one. Not sure if it's necessary, but I thought it did when I came up with this, so there you have it.
 */

//---Chest/container functions---
void actChest(Entity *my);
void actChestLid(Entity *my);
void closeChestClientside(); //Called by the client to manage all clientside stuff relating to closing a chest.
void addItemToChestClientside(Item *item); //Called by the client to manage all clientside stuff relating to adding an item to a chest.

//---Magic entity functions---
void actMagiclightBall(Entity *my);
