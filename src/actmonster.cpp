/*-------------------------------------------------------------------------------

	BARONY
	File: actmonster.cpp
	Desc: behavior function for monsters

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "messages.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "monster.hpp"
#include "sound.hpp"
#include "shops.hpp"
#include "interface/interface.hpp"
#include "magic/magic.hpp"
#include "net.hpp"
#include "paths.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "colors.hpp"

float limbs[NUMMONSTERS][20][3];

// determines which monsters fight which
bool swornenemies[NUMMONSTERS][NUMMONSTERS] =
{
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // NOTHING
	{ 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1 }, // HUMAN
	{ 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0 }, // RAT
	{ 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0 }, // GOBLIN
	{ 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0 }, // SLIME
	{ 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0 }, // TROLL
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // OCTOPUS
	{ 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0 }, // SPIDER
	{ 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // GHOUL
	{ 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // SKELETON
	{ 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0 }, // SCORPION
	{ 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // IMP
	{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // BUGBEAR
	{ 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0 }, // GNOME
	{ 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0 }, // DEMON
	{ 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // SUCCUBUS
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // MIMIC
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 }, // LICH
	{ 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0 }, // MINOTAUR
	{ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // DEVIL
	{ 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // SHOPKEEPER
	{ 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, // KOBOLD
	{ 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0 }, // SCARAB
	{ 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0 }, // CRYSTALGOLEM
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // INCUBUS
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // VAMPIRE
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // SHADOW
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 }, // COCKATRICE
	{ 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0 }, // INSECTOID
	{ 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 }, // GOATMAN
	{ 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1 }, // AUTOMATON
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // LICH_ICE
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }  // LICH_FIRE
};

// determines which monsters come to the aid of other monsters
bool monsterally[NUMMONSTERS][NUMMONSTERS] =
{
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // NOTHING
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 }, // HUMAN
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // RAT
	{ 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 }, // GOBLIN
	{ 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 }, // SLIME
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // TROLL
	{ 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // OCTOPUS
	{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // SPIDER
	{ 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1 }, // GHOUL
	{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1 }, // SKELETON
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // SCORPION
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1 }, // IMP
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // BUGBEAR
	{ 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // GNOME
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1 }, // DEMON
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1 }, // SUCCUBUS
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // MIMIC
	{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1 }, // LICH
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // MINOTAUR
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1 }, // DEVIL
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // SHOPKEEPER
	{ 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // KOBOLD
	{ 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // SCARAB
	{ 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // CRYSTALGOLEM
	{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1 }, // INCUBUS
	{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1 }, // VAMPIRE
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1 }, // SHADOW
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 }, // COCKATRICE
	{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // INSECTOID
	{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1 }, // GOATMAN
	{ 0, 1, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 }, // AUTOMATON
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 }, // LICH_ICE
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 }  // LICH_FIRE
};

// monster sight ranges
double sightranges[NUMMONSTERS] =
{
	256,  // NOTHING
	256,  // HUMAN
	128,  // RAT
	256,  // GOBLIN
	80,   // SLIME
	32,   // TROLL
	0,    // OCTOPUS
	96,   // SPIDER
	128,  // GHOUL
	192,  // SKELETON
	96,   // SCORPION
	256,  // IMP
	0,    // BUGBEAR
	128,  // GNOME
	256,  // DEMON
	256,  // SUCCUBUS
	0,    // MIMIC
	512,  // LICH
	512,  // MINOTAUR
	1024, // DEVIL
	256,  // SHOPKEEPER
	192,  // KOBOLD
	512,  // SCARAB
	192,   // CRYSTALGOLEM
	256,  // INCUBUS
	192,  // VAMPIRE
	768,  // SHADOW
	256,  // COCKATRICE
	256,  // INSECTOID
	256,  // GOATMAN
	192,  // AUTOMATON
	512,  // LICH_ICE
	512,  // LICH_FIRE
};

int monsterGlobalAnimationMultiplier = 10;
int monsterGlobalAttackTimeMultiplier = 1;

/*-------------------------------------------------------------------------------

	summonMonster

	summons a monster near (but not at) the given location

-------------------------------------------------------------------------------*/

void summonMonsterClient(Monster creature, long x, long y, Uint32 uid)
{
	Entity* entity = summonMonster(creature, x, y);
	entity->flags[INVISIBLE] = false;
	entity->setUID(uid);
}

Entity* summonMonster(Monster creature, long x, long y)
{
	Entity* entity = newEntity(-1, 1, map.entities, map.creatures); //Monster entity.
	//Set the monster's variables.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->x = x;
	entity->y = y;
	entity->z = 6;
	entity->yaw = (rand() % 360) * PI / 180.0;
	entity->behavior = &actMonster;
	entity->flags[UPDATENEEDED] = true;
	entity->flags[INVISIBLE] = true;
	entity->ranbehavior = true;
	entity->skill[5] = nummonsters;

	Stat* myStats = nullptr;
	if ( multiplayer != CLIENT )
	{
		// Need to give the entity its list stuff.
		// create an empty first node for traversal purposes
		node_t* node = nullptr;
		node = list_AddNodeFirst(&entity->children);
		node->element = nullptr;
		node->deconstructor = &emptyDeconstructor;

		myStats = new Stat(creature + 1000);
		node = list_AddNodeLast(&entity->children); //ASSUMING THIS ALREADY EXISTS WHEN THIS FUNCTION IS CALLED.
		node->element = myStats;
		node->size = sizeof(myStats);
		//node->deconstructor = myStats->~Stat;
		if ( entity->parent )
		{
			myStats->leader_uid = entity->parent;
			entity->parent = 0;
		}
		myStats->type = creature;
	}
	else
	{
		//Give dummy stats.
		entity->clientStats = new Stat(creature + 1000);
	}

	// Find a free tile next to the source and then spawn it there.
	if ( multiplayer != CLIENT )
	{
		if ( entityInsideSomething(entity) )
		{
			do
			{
				entity->x = x;
				entity->y = y - 16;
				if (!entityInsideSomething(entity))
				{
					break;    // north
				}
				entity->x = x;
				entity->y = y + 16;
				if (!entityInsideSomething(entity))
				{
					break;    // south
				}
				entity->x = x - 16;
				entity->y = y;
				if (!entityInsideSomething(entity))
				{
					break;    // west
				}
				entity->x = x + 16;
				entity->y = y;
				if (!entityInsideSomething(entity))
				{
					break;    // east
				}
				entity->x = x + 16;
				entity->y = y - 16;
				if (!entityInsideSomething(entity))
				{
					break;    // northeast
				}
				entity->x = x + 16;
				entity->y = y + 16;
				if (!entityInsideSomething(entity))
				{
					break;    // southeast
				}
				entity->x = x - 16;
				entity->y = y - 16;
				if (!entityInsideSomething(entity))
				{
					break;    // northwest
				}
				entity->x = x - 16;
				entity->y = y + 16;
				if (!entityInsideSomething(entity))
				{
					break;    // southwest
				}

				// we can't have monsters in walls...
				list_RemoveNode(entity->mynode);
				entity = nullptr;
				break;
			}
			while ( 1 );
		}
	}

	if ( entity )
	{
		switch ( creature )
		{
			case RAT:
				entity->focalx = limbs[RAT][0][0]; // 0
				entity->focaly = limbs[RAT][0][1]; // 0
				entity->focalz = limbs[RAT][0][2]; // 0
				break;
			case SCORPION:
				entity->focalx = limbs[SCORPION][0][0]; // 0
				entity->focaly = limbs[SCORPION][0][1]; // 0
				entity->focalz = limbs[SCORPION][0][2]; // 0
				break;
			case HUMAN:
				entity->z = -1;
				entity->focalx = limbs[HUMAN][0][0]; // 0
				entity->focaly = limbs[HUMAN][0][1]; // 0
				entity->focalz = limbs[HUMAN][0][2]; // -1.5
				break;
			case GOBLIN:
				entity->z = 0;
				entity->focalx = limbs[GOBLIN][0][0]; // 0
				entity->focaly = limbs[GOBLIN][0][1]; // 0
				entity->focalz = limbs[GOBLIN][0][2]; // -1.75
				break;
			case SLIME:
				if ( multiplayer != CLIENT )
				{
					myStats->LVL = 7;
				}
				break;
			case SUCCUBUS:
				entity->z = -1;
				entity->focalx = limbs[SUCCUBUS][0][0]; // 0
				entity->focaly = limbs[SUCCUBUS][0][1]; // 0
				entity->focalz = limbs[SUCCUBUS][0][2]; // -1.5
				break;
			case TROLL:
				entity->z = -1.5;
				entity->focalx = limbs[TROLL][0][0]; // 1
				entity->focaly = limbs[TROLL][0][1]; // 0
				entity->focalz = limbs[TROLL][0][2]; // -2
				break;
			case SHOPKEEPER:
				entity->z = -1;
				entity->focalx = limbs[SHOPKEEPER][0][0]; // 0
				entity->focaly = limbs[SHOPKEEPER][0][1]; // 0
				entity->focalz = limbs[SHOPKEEPER][0][2]; // -1.5
				break;
			case SKELETON:
				entity->z = -.5;
				entity->focalx = limbs[SKELETON][0][0]; // 0
				entity->focaly = limbs[SKELETON][0][1]; // 0
				entity->focalz = limbs[SKELETON][0][2]; // -1.5
				break;
			case MINOTAUR:
				entity->z = -6;
				entity->focalx = limbs[MINOTAUR][0][0]; // 0
				entity->focaly = limbs[MINOTAUR][0][1]; // 0
				entity->focalz = limbs[MINOTAUR][0][2]; // 0
				break;
			case GHOUL:
				entity->z = -.25;
				entity->focalx = limbs[GHOUL][0][0]; // 0
				entity->focaly = limbs[GHOUL][0][1]; // 0
				entity->focalz = limbs[GHOUL][0][2]; // -1.5
				break;
			case DEMON:
				entity->z = -8.5;
				entity->focalx = limbs[DEMON][0][0]; // -1
				entity->focaly = limbs[DEMON][0][1]; // 0
				entity->focalz = limbs[DEMON][0][2]; // -1.25
				break;
			case SPIDER:
				entity->z = 4.5;
				entity->focalx = limbs[SPIDER][0][0]; // -3
				entity->focaly = limbs[SPIDER][0][1]; // 0
				entity->focalz = limbs[SPIDER][0][2]; // -1
				break;
			case LICH:
				entity->focalx = limbs[LICH][0][0]; // -0.75
				entity->focaly = limbs[LICH][0][1]; // 0
				entity->focalz = limbs[LICH][0][2]; // 0
				entity->z = -2;
				entity->yaw = PI;
				entity->sprite = 274;
				entity->skill[29] = 120;
				break;
			case CREATURE_IMP:
				entity->z = -4.5;
				entity->focalx = limbs[CREATURE_IMP][0][0]; // 0
				entity->focaly = limbs[CREATURE_IMP][0][1]; // 0
				entity->focalz = limbs[CREATURE_IMP][0][2]; // -1.75
				break;
			case GNOME:
				entity->z = 2.25;
				entity->focalx = limbs[GNOME][0][0]; // 0
				entity->focaly = limbs[GNOME][0][1]; // 0
				entity->focalz = limbs[GNOME][0][2]; // -2
				break;
			case DEVIL:
				entity->focalx = limbs[DEVIL][0][0]; // 0
				entity->focaly = limbs[DEVIL][0][1]; // 0
				entity->focalz = limbs[DEVIL][0][2]; // 0
				entity->z = -4;
				entity->sizex = 20;
				entity->sizey = 20;
				entity->yaw = PI;
				break;
			case KOBOLD:
				entity->z = 2.25;
				entity->focalx = limbs[KOBOLD][0][0]; // 0
				entity->focaly = limbs[KOBOLD][0][1]; // 0
				entity->focalz = limbs[KOBOLD][0][2]; // -2
				break;
			case SCARAB:
				entity->focalx = limbs[SCARAB][0][0]; // 0
				entity->focaly = limbs[SCARAB][0][1]; // 0
				entity->focalz = limbs[SCARAB][0][2]; // 0
				break;
			case CRYSTALGOLEM:
				entity->z = -1.5;
				entity->focalx = limbs[CRYSTALGOLEM][0][0]; // 1
				entity->focaly = limbs[CRYSTALGOLEM][0][1]; // 0
				entity->focalz = limbs[CRYSTALGOLEM][0][2]; // -2
				break;
			case INCUBUS:
				entity->z = -1;
				entity->focalx = limbs[INCUBUS][0][0]; // 0
				entity->focaly = limbs[INCUBUS][0][1]; // 0
				entity->focalz = limbs[INCUBUS][0][2]; // -1.5
				break;
			case VAMPIRE:
				entity->z = -1;
				entity->focalx = limbs[HUMAN][0][0]; // 0
				entity->focaly = limbs[HUMAN][0][1]; // 0
				entity->focalz = limbs[HUMAN][0][2]; // -1.5
				break;
			case SHADOW:
				entity->z = -1;
				entity->focalx = limbs[SHADOW][0][0]; // 0
				entity->focaly = limbs[SHADOW][0][1]; // 0
				entity->focalz = limbs[SHADOW][0][2]; // -1.75
				break;
			case COCKATRICE:
				entity->z = -4.5;
				entity->focalx = limbs[COCKATRICE][0][0]; // 0
				entity->focaly = limbs[COCKATRICE][0][1]; // 0
				entity->focalz = limbs[COCKATRICE][0][2]; // -1.75
				break;
			case INSECTOID:
				entity->z = 0;
				entity->focalx = limbs[INSECTOID][0][0]; // 0
				entity->focaly = limbs[INSECTOID][0][1]; // 0
				entity->focalz = limbs[INSECTOID][0][2]; // -1.75
				if ( multiplayer != CLIENT )
				{
				if ( !strncmp(map.name, "Sokoban", 7) || !strncmp(map.name, "The Labyrinth", 13) )
				{
					strcpy(myStats->name, "lesser insectoid");
				}
				}
				break;
			case GOATMAN:
				entity->z = 0;
				entity->focalx = limbs[GOATMAN][0][0]; // 0
				entity->focaly = limbs[GOATMAN][0][1]; // 0
				entity->focalz = limbs[GOATMAN][0][2]; // -1.75
				break;
			case AUTOMATON:
				entity->z = -.5;
				entity->focalx = limbs[AUTOMATON][0][0]; // 0
				entity->focaly = limbs[AUTOMATON][0][1]; // 0
				entity->focalz = limbs[AUTOMATON][0][2]; // -1.5
				break;
			case LICH_ICE:
				entity->focalx = limbs[LICH_ICE][0][0]; // -0.75
				entity->focaly = limbs[LICH_ICE][0][1]; // 0
				entity->focalz = limbs[LICH_ICE][0][2]; // 0
				entity->z = -1.2;
				entity->yaw = PI;
				entity->sprite = 650;
				break;
			case LICH_FIRE:
				entity->focalx = limbs[LICH_FIRE][0][0]; // -0.75
				entity->focaly = limbs[LICH_FIRE][0][1]; // 0
				entity->focalz = limbs[LICH_FIRE][0][2]; // 0
				entity->z = -1.2;
				entity->yaw = PI;
				entity->sprite = 646;
				break;
			default:
				//Spawn a potato.
				list_RemoveNode(entity->mynode);
				return nullptr;
				break;
		}
		if ( entity )
		{
			nummonsters++;
		}
		if ( multiplayer == SERVER )
		{
			strcpy((char*)net_packet->data, "SUMM");
			SDLNet_Write32((Uint32)creature, &net_packet->data[4]);
			SDLNet_Write32((Uint32)entity->x, &net_packet->data[8]);
			SDLNet_Write32((Uint32)entity->y, &net_packet->data[12]);
			SDLNet_Write32(entity->getUID(), &net_packet->data[16]);
			net_packet->len = 20;

			int c;
			for ( c = 0; c < MAXPLAYERS; c++ )
			{
				if ( client_disconnected[c] )
				{
					continue;
				}
				net_packet->address.host = net_clients[c - 1].host;
				net_packet->address.port = net_clients[c - 1].port;
				sendPacketSafe(net_sock, -1, net_packet, c - 1);
			}
		}
		return entity;
	}
	return nullptr;
}

void summonManyMonster(Monster creature)
{
	for ( long x = 1; x < map.width - 1; ++x )
	{
		for ( long y = 1; y < map.height - 1; ++y )
		{
			summonMonster(creature, (x * 16) + 8, (y * 16) + 8);
		}
	}
}

/*-------------------------------------------------------------------------------

	monsterMoveAside

	Causes the monster given in *entity to move aside from the entity given
	in *my

-------------------------------------------------------------------------------*/

bool monsterMoveAside(Entity* my, Entity* entity)
{
	if ( !my || !entity )
	{
		return false;
	}

	if ( my->monsterState != 0 )
	{
		return false;
	}

	int x = 0, y = 0;
	if ( cos(entity->yaw) > .4 )
	{
		y += 16;
		if ( checkObstacle(my->x, my->y + y, my, NULL) )
		{
			y -= 32;
			if ( checkObstacle(my->x, my->y + y, my, NULL) )
			{
				y = 0;
				x += 16;
			}
		}
	}
	else if ( cos(entity->yaw) < -.4 )
	{
		y -= 16;
		if ( checkObstacle(my->x, my->y + y, my, NULL) )
		{
			y += 32;
			if ( checkObstacle(my->x, my->y + y, my, NULL) )
			{
				y = 0;
				x -= 16;
			}
		}
	}
	if ( sin(entity->yaw) > .4 )
	{
		x -= 16;
		if ( checkObstacle(my->x + x, my->y, my, NULL) )
		{
			x += 32;
			if ( checkObstacle(my->x + x, my->y, my, NULL) )
			{
				x = 0;
				y += 16;
			}
		}
	}
	else if ( sin(entity->yaw) < -.4 )
	{
		x += 16;
		if ( checkObstacle(my->x + x, my->y, my, NULL) )
		{
			x -= 32;
			if ( checkObstacle(my->x + x, my->y, my, NULL) )
			{
				x = 0;
				y -= 16;
			}
		}
	}

	// move away
	if ( x != 0 || y != 0 )
	{
		my->monsterState = MONSTER_STATE_PATH;
		my->monsterReleaseAttackTarget();
		my->monsterTargetX = my->x + x;
		my->monsterTargetY = my->y + y;
		serverUpdateEntitySkill(my, 0);
		return true;
	}
	return false;
}

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

int devilstate = 0;
int devilacted = 0;
int devilroar = 0;
int devilintro = 0;

bool makeFollower(int monsterclicked, bool ringconflict, char namesays[32], Entity* my, Stat* myStats)
{
	if ( ringconflict )
	{
		//Instant fail if ring of conflict is in effect. You have no allies!
		return false;
	}

	Monster race = my->getRace();

	if ( myStats->leader_uid != 0 )
	{
		//Handle the "I have a leader!" situation.
		if ( myStats->leader_uid == players[monsterclicked]->entity->getUID() )
		{
			//Follows this player already!
			if ( my->getINT() > -2 && race == HUMAN )
			{
				messagePlayer(monsterclicked, language[535], namesays, stats[monsterclicked]->name);
			}
			else
			{
				messagePlayer(monsterclicked, language[534], namesays);
			}
		}
		else
		{
			//Follows somebody else.
			if ( my->getINT() > -2 && race == HUMAN )
			{
				messagePlayer(monsterclicked, language[536], namesays, stats[monsterclicked]->name);
			}
			else
			{
				messagePlayer(monsterclicked, language[534], namesays);
			}

			if ( my->checkFriend(players[monsterclicked]->entity) )
			{
				//If friendly, move aside.
				monsterMoveAside(my, players[monsterclicked]->entity);
			}
		}

		return false;
	}

	bool canAlly = false;
	if ( skillCapstoneUnlocked(monsterclicked, PRO_LEADERSHIP) )
	{
		//No cap on # of followers.
		//Can control humans & goblins both.
		//TODO: Control humanoids in general? Or otherwise something from each tileset.
		if ( race == HUMAN || race == GOBLIN || race == AUTOMATON )
		{
			canAlly = true;
		}

		//TODO: If enemies (e.g. goblin or an angry human), require the player to be unseen by this creature to gain control of it.
	}
	else
	{
		if ( my->checkFriend(players[monsterclicked]->entity) )
		{
			if ( myStats->leader_uid == 0 )
			{
				if ( (stats[monsterclicked]->PROFICIENCIES[PRO_LEADERSHIP] / 4 >= list_Size(&stats[monsterclicked]->FOLLOWERS) ) )
				{
					canAlly = true;
				}
			}
		}
	}

	if ( !canAlly )
	{
		//This one does not want to join your ever-enlarging cult.
		if ( my->getINT() > -2 && race == HUMAN )
		{
			//Human tells off the player.
			messagePlayer(monsterclicked, language[530 + rand() % 4], namesays);
			// move aside
			monsterMoveAside(my, players[monsterclicked]->entity);
		}
		else
		{
			messagePlayer(monsterclicked, language[534], namesays);
		}

		return false;
	}

	node_t* newNode = list_AddNodeLast(&stats[monsterclicked]->FOLLOWERS);
	newNode->deconstructor = &defaultDeconstructor;
	Uint32* myuid = (Uint32*) (malloc(sizeof(Uint32)));
	newNode->element = myuid;
	*myuid = my->getUID();

	if ( my->getINT() > -2 && race == HUMAN )
	{
		messagePlayer(monsterclicked, language[525 + rand() % 4], namesays, stats[monsterclicked]->name);
	}
	else
	{
		//This one can't speak, so generic "The %s decides to follow you!" message.
		messagePlayerMonsterEvent(monsterclicked, 0xFFFFFFFF, *myStats, language[529], language[529], MSG_COMBAT);
	}

	monsterMoveAside(my, players[monsterclicked]->entity);
	players[monsterclicked]->entity->increaseSkill(PRO_LEADERSHIP);
	my->monsterState = MONSTER_STATE_WAIT; // be ready to follow
	myStats->leader_uid = players[monsterclicked]->entity->getUID();
	my->monsterAllyIndex = monsterclicked;
	if ( multiplayer == SERVER )
	{
		serverUpdateEntitySkill(my, 42); // update monsterAllyIndex for clients.
	}
	if ( monsterclicked > 0 && multiplayer == SERVER )
	{
		//Tell the client he suckered somebody into his cult.
		strcpy((char*) (net_packet->data), "LEAD");
		SDLNet_Write32((Uint32 )my->getUID(), &net_packet->data[4]);
		net_packet->address.host = net_clients[monsterclicked - 1].host;
		net_packet->address.port = net_clients[monsterclicked - 1].port;
		net_packet->len = 8;
		sendPacketSafe(net_sock, -1, net_packet, monsterclicked - 1);

		serverUpdateAllyStat(monsterclicked, my->getUID(), myStats->LVL, myStats->HP, myStats->MAXHP, myStats->type);
	}

	if ( !followerMenuEntityRecent && monsterclicked == clientnum )
	{
		followerMenuEntityRecent = my;
	}
	return true;
}

//int devilintro=0;

void actMonster(Entity* my)
{
	if (!my)
	{
		return;
	}

	int x, y, c, i;
	double dist, dist2;
	list_t* path;
	node_t* node, *node2;
	pathnode_t* pathnode;
	double dir;
	double tangent;
	Stat* myStats;
	Entity* entity;
	Stat* hitstats = NULL;
	bool hasrangedweapon = false;
	bool myReflex;
	Sint32 previousMonsterState = my->monsterState;

	// deactivate in menu
	if ( intro )
	{
		return;
	}

	// this is mostly a SERVER function.
	// however, there is a small part for clients:
	if ( multiplayer == CLIENT )
	{
		if ( !MONSTER_INIT && my->sprite >= 100 )
		{
			MONSTER_INIT = 1;

			// make two empty nodes
			node = list_AddNodeLast(&my->children);
			node->element = nullptr;
			node->deconstructor = &emptyDeconstructor;
			node->size = 0;
			node = list_AddNodeLast(&my->children);
			node->element = nullptr;
			node->deconstructor = &emptyDeconstructor;
			node->size = 0;
			if ( (my->sprite >= 113 && my->sprite < 118) ||
			        (my->sprite >= 125 && my->sprite < 130) ||
			        (my->sprite >= 332 && my->sprite < 334) ||
			        (my->sprite >= 341 && my->sprite < 347) ||
			        (my->sprite >= 354 && my->sprite < 360) ||
			        (my->sprite >= 367 && my->sprite < 373) ||
			        (my->sprite >= 380 && my->sprite < 386) )   // human heads
			{
				initHuman(my, nullptr);
			}
			else if ( my->sprite == 131 || my->sprite == 265 )     // rat
			{
				initRat(my, nullptr);
			}
			else if ( my->sprite == 180 )     // goblin head
			{
				initGoblin(my, nullptr);
			}
			else if ( my->sprite == 196 || my->sprite == 266 )     // scorpion body
			{
				initScorpion(my, nullptr);
			}
			else if ( my->sprite == 190 )     // succubus head
			{
				initSuccubus(my, nullptr);
			}
			else if ( my->sprite == 204 )     // troll head
			{
				initTroll(my, nullptr);
			}
			else if ( my->sprite == 217 )     // shopkeeper head
			{
				initShopkeeper(my, nullptr);
			}
			else if ( my->sprite == 229 )     // skeleton head
			{
				initSkeleton(my, nullptr);
			}
			else if ( my->sprite == 239 )     // minotaur waist
			{
				initMinotaur(my, nullptr);
			}
			else if ( my->sprite == 246 )     // ghoul head
			{
				initGhoul(my, nullptr);
			}
			else if ( my->sprite == 258 )     // demon head
			{
				initDemon(my, nullptr);
			}
			else if ( my->sprite == 267 )     // spider body
			{
				initSpider(my, nullptr);
			}
			else if ( my->sprite == 274 )     // lich body
			{
				initLich(my, nullptr);
			}
			else if ( my->sprite == 289 )     // imp head
			{
				initImp(my, nullptr);
			}
			else if ( my->sprite == 295 )     // gnome head
			{
				initGnome(my, nullptr);
			}
			else if ( my->sprite == 304 )     // devil torso
			{
				initDevil(my, nullptr);
			}
			else if ( my->sprite == 475 )     // crystal golem head
			{
				initCrystalgolem(my, nullptr);
			}
			else if ( my->sprite == 413 )     // cockatrice head
			{
				initCockatrice(my, nullptr);
			}
			else if ( my->sprite == 467 )     // automaton torso
			{
				initAutomaton(my, NULL);
			}
			else if ( my->sprite == 429 || my->sprite == 430 )     // scarab
			{
				initScarab(my, nullptr);
			}
			else if ( my->sprite == 421 )     // kobold head
			{
				initKobold(my, nullptr);
			}
			else if ( my->sprite == 481 )     // shadow head
			{
				initShadow(my, nullptr);
			}
			else if ( my->sprite == 437 )     // vampire head
			{
				initVampire(my, nullptr);
			}
			else if ( my->sprite == 445 )     // incubus head
			{
				initIncubus(my, nullptr);
			}
			else if ( my->sprite == 455 )     // insectoid head
			{
				initInsectoid(my, nullptr);
			}
			else if ( my->sprite == 463 )     // goatman head
			{
				initGoatman(my, nullptr);
			}
			else if ( my->sprite == 646 )     // lich body
			{
				initLichFire(my, nullptr);
			}
			else if ( my->sprite == 650 )     // lich body
			{
				initLichIce(my, nullptr);
			}
		}
		else
		{
			my->flags[BURNABLE] = true;
			if ( (my->sprite >= 113 && my->sprite < 118) ||
			        (my->sprite >= 125 && my->sprite < 130) ||
			        (my->sprite >= 332 && my->sprite < 334) ||
			        (my->sprite >= 341 && my->sprite < 347) ||
			        (my->sprite >= 354 && my->sprite < 360) ||
			        (my->sprite >= 367 && my->sprite < 373) ||
			        (my->sprite >= 380 && my->sprite < 386) )   // human heads
			{
				humanMoveBodyparts(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 131 || my->sprite == 265 )     // rat
			{
				ratAnimate(my, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 180 )     // goblin head
			{
				goblinMoveBodyparts(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 196 || my->sprite == 266 )     // scorpion body
			{
				scorpionAnimate(my, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 190 )     // succubus head
			{
				succubusMoveBodyparts(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 204 )     // troll head
			{
				trollMoveBodyparts(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 217 )     // shopkeeper head
			{
				shopkeeperMoveBodyparts(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 229 )     // skeleton head
			{
				my->flags[BURNABLE] = false;
				skeletonMoveBodyparts(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 239 )     // minotaur waist
			{
				minotaurMoveBodyparts(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
				actMinotaurCeilingBuster(my);
			}
			else if ( my->sprite == 246 )     // ghoul head
			{
				ghoulMoveBodyparts(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 258 )     // demon head
			{
				my->flags[BURNABLE] = false;
				demonMoveBodyparts(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
				actDemonCeilingBuster(my);
			}
			else if ( my->sprite == 267 )     // spider body
			{
				spiderMoveBodyparts(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 274 )     // lich body
			{
				my->flags[BURNABLE] = false;
				lichAnimate(my, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 289 )     // imp head
			{
				my->flags[BURNABLE] = false;
				impMoveBodyparts(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 295 )     // gnome head
			{
				gnomeMoveBodyparts(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 304 )     // devil torso
			{
				my->flags[BURNABLE] = false;
				devilMoveBodyparts(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 421 )     // kobold head
			{
				koboldMoveBodyparts(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 429 || my->sprite == 430 )     // scarab
			{
				scarabAnimate(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 475 )     // crystal golem head
			{
				crystalgolemMoveBodyparts(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 445 )     // incubus head
			{
				incubusMoveBodyparts(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 437 )     // vampire head
			{
				vampireMoveBodyparts(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 481 )     // shadow head
			{
				shadowMoveBodyparts(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 413 )     // cockatrice head
			{
				cockatriceMoveBodyparts(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 455 )     // insectoid head
			{
				insectoidMoveBodyparts(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 463 )     // goatman head
			{
				goatmanMoveBodyparts(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 467 )     // automaton head
			{
				automatonMoveBodyparts(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 646 )     // lich body
			{
				my->flags[BURNABLE] = false;
				lichFireAnimate(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else if ( my->sprite == 650 )     // lich body
			{
				my->flags[BURNABLE] = false;
				lichIceAnimate(my, NULL, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			}
			else
			{
				my->flags[BURNABLE] = false;
			}

			if ( !intro )
			{
				my->handleEffectsClient();
			}

			// request entity update (check if I've been deleted)
			if ( ticks % (TICKS_PER_SECOND * 5) == my->getUID() % (TICKS_PER_SECOND * 5) )
			{
				strcpy((char*)net_packet->data, "ENTE");
				net_packet->data[4] = clientnum;
				SDLNet_Write32(my->getUID(), &net_packet->data[5]);
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 9;
				sendPacketSafe(net_sock, -1, net_packet, 0);
			}
		}
		return;
	}

	if ( ticks % (TICKS_PER_SECOND) == my->getUID() % (TICKS_PER_SECOND / 2) )
	{
		myReflex = true;
	}
	else
	{
		myReflex = false;
	}

	// init
	if ( MONSTER_INIT < 2 )   // 0 means no initialization, 1 means stats are initialized
	{
		my->skill[2] = -4; // tells clients to set this entity behavior to actMonster
		myStats = my->getStats();
		if (myStats)
		{
			myStats->monster_sound = NULL;
			my->flags[BURNABLE] = true;
			switch ( myStats->type )
			{
				case HUMAN:
					initHuman(my, myStats);
					break;
				case RAT:
					initRat(my, myStats);
					break;
				case GOBLIN:
					initGoblin(my, myStats);
					break;
				case SLIME:
					my->flags[BURNABLE] = false;
					initSlime(my, myStats);
					break;
				case SCORPION:
					initScorpion(my, myStats);
					break;
				case SUCCUBUS:
					initSuccubus(my, myStats);
					break;
				case TROLL:
					initTroll(my, myStats);
					break;
				case SHOPKEEPER:
					initShopkeeper(my, myStats);
					break;
				case SKELETON:
					my->flags[BURNABLE] = false;
					initSkeleton(my, myStats);
					break;
				case MINOTAUR:
					initMinotaur(my, myStats);
					break;
				case GHOUL:
					initGhoul(my, myStats);
					break;
				case DEMON:
					my->flags[BURNABLE] = false;
					initDemon(my, myStats);
					break;
				case SPIDER:
					initSpider(my, myStats);
					break;
				case LICH:
					my->flags[BURNABLE] = false;
					initLich(my, myStats);
					break;
				case CREATURE_IMP:
					my->flags[BURNABLE] = false;
					initImp(my, myStats);
					break;
				case GNOME:
					initGnome(my, myStats);
					break;
				case DEVIL:
					my->flags[BURNABLE] = false;
					devilstate = 0;
					devilacted = 0;
					initDevil(my, myStats);
					break;
				case KOBOLD:
					initKobold (my, myStats);
					break;
				case SCARAB:
					initScarab (my, myStats);
					break;
				case CRYSTALGOLEM:
					initCrystalgolem (my, myStats);
					break;
				case INCUBUS:
					initIncubus (my, myStats);
					break;
				case VAMPIRE:
					initVampire (my, myStats);
					break;
				case SHADOW:
					initShadow (my, myStats);
					break;
				case COCKATRICE:
					initCockatrice (my, myStats);
					break;
				case INSECTOID:
					initInsectoid (my, myStats);
					break;
				case GOATMAN:
					initGoatman (my, myStats);
					break;
				case AUTOMATON:
					my->flags[BURNABLE] = false;
					initAutomaton (my, myStats);
					break;
				case LICH_ICE:
					my->flags[BURNABLE] = false;
					initLichIce (my, myStats);
					my->monsterLichBattleState = LICH_BATTLE_IMMOBILE;
					break;
				case LICH_FIRE:
					my->flags[BURNABLE] = false;
					initLichFire (my, myStats);
					my->monsterLichBattleState = LICH_BATTLE_IMMOBILE;
					break;
				default:
					break; //This should never be reached.
			}
		}

		MONSTER_INIT = 2;
		if ( myStats->type != LICH && myStats->type != DEVIL )
		{
			my->monsterLookDir = (rand() % 360) * PI / 180;
		}
		else
		{
			my->monsterLookDir = PI;
		}
		my->monsterLookTime = rand() % 120;
		my->monsterMoveTime = rand() % 10;
		MONSTER_SOUND = NULL;
		if ( MONSTER_NUMBER == -1 )
		{
			MONSTER_NUMBER = nummonsters;
			nummonsters++;
		}
		/*if( rand()%20==0 ) { // 20% chance
			MONSTER_STATE = 2; // start hunting the player immediately
			MONSTER_TARGET = rand()%numplayers;
			MONSTER_TARGETX = players[MONSTER_TARGET]->x;
			MONSTER_TARGETY = players[MONSTER_TARGET]->y;
		} else {
			MONSTER_TARGET = -1;
		}*/
		my->monsterTarget = 0;

		/*// create an empty first node for traversal purposes //GOING TO ASSUME THIS ALREADY EXISTS WHEN THIS FUNCTION IS CALLED.
		node = list_AddNodeFirst(my->children);
		node->element = NULL;
		node->deconstructor = &emptyDeconstructor;*/

		// assign stats to the monster
		//myStats = (Stat *) malloc(sizeof(Stat)); //GOING TO ASSUME THIS ALREADY EXISTS WHEN THIS FUNCTION IS CALLED.
		//myStats->type = RAT; //GOING TO ASSUME THIS IS ALREADY PROPERLY SET WHEN THE FUNCTION IS CALLED.
		//TODO: Move the rest of this into the monster specific init functions.
		/*node = list_AddNodeLast(my->children); //ASSUMING THIS ALREADY EXISTS WHEN THIS FUNCTION IS CALLED.
		node->element = myStats;
		node->deconstructor = &defaultDeconstructor;*/

		return;
	}

	myStats = my->getStats();
	if ( myStats == NULL )
	{
		printlog("ERROR: monster entity at %p has no stats struct!", my);
		return;
	}
	myStats->defending = false;
	myStats->sneaking = 0;

	// levitation
	bool levitating = isLevitating(myStats);

	if ( myStats->type == MINOTAUR )
	{
		int c;
		for ( c = 0; c < MAXPLAYERS; c++ )
		{
			assailant[c] = true; // as long as this is active, combat music doesn't turn off
			assailantTimer[c] = COMBAT_MUSIC_COOLDOWN;
		}
	}

	if ( myStats->type == SHADOW && my->monsterTarget != 0 )
	{
		for ( int c = 0; c < MAXPLAYERS; ++c )
		{
			if ( players[c] && players[c]->entity && players[c]->entity->getUID() == my->monsterTarget )
			{
				assailant[c] = true; //Keeps combat music on as long as a shadow is hunting you down down down!
				assailantTimer[c] = COMBAT_MUSIC_COOLDOWN;
				break;
			}
		}
	}

	if ( my->ticks == 120 + MONSTER_NUMBER )
	{
		serverUpdateBodypartIDs(my);
	}

	// some special herx behavior
	if ( myStats->type == LICH )
	{
		// destroying room lights
		if ( myStats->HP <= myStats->MAXHP / 2 )
		{
			node_t* node, *nextnode;
			bool foundlights = false;
			for ( node = map.entities->first; node != nullptr; node = nextnode )
			{
				nextnode = node->next;
				Entity* tempEntity = (Entity*)node->element;

				if ( tempEntity->behavior == &actTorch || tempEntity->behavior == &actCampfire )
				{
					foundlights = true;
					if ( tempEntity->light )
					{
						list_RemoveNode(tempEntity->light->node);
						tempEntity->light = nullptr;
					}
					list_RemoveNode(tempEntity->mynode);
				}
			}
			if ( foundlights )
			{
#ifdef USE_FMOD
				if ( MONSTER_SOUND )
				{
					FMOD_Channel_Stop(MONSTER_SOUND);
				}
#elif defined USE_OPENAL
				if ( MONSTER_SOUND )
				{
					OPENAL_Channel_Stop(MONSTER_SOUND);
				}
#endif
				int c;
				for ( c = 0; c < MAXPLAYERS; c++ )
				{
					MONSTER_SOUND = playSoundPlayer(c, 179, 128);
					playSoundPlayer(c, 166, 128);
					Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 255);
					messagePlayerColor(c, color, language[512]);
				}
			}
		}
		// dodging away
		if ( ( ( rand() % 4 == 0 && my->monsterState != 6 ) || ( rand() % 10 == 0 && my->monsterState == MONSTER_STATE_LICH_SUMMON) ) && myStats->OLDHP != myStats->HP )
		{
			playSoundEntity(my, 180, 128);
			my->monsterState = MONSTER_STATE_LICH_DODGE; // dodge state
			double dir = my->yaw - (PI / 2) + PI * (rand() % 2);
			MONSTER_VELX = cos(dir) * 5;
			MONSTER_VELY = sin(dir) * 5;
			my->monsterSpecialTimer = 0;
		}
	}

	if ( (myStats->type == LICH_FIRE && my->monsterState != MONSTER_STATE_LICHFIRE_DIE) 
		|| (myStats->type == LICH_ICE && my->monsterState != MONSTER_STATE_LICHICE_DIE )
		&& myStats->HP > 0 )
	{
		//messagePlayer(0, "state: %d", my->monsterState);
		if ( my->monsterLichBattleState >= LICH_BATTLE_READY )
		{
			for ( int c = 0; c < MAXPLAYERS; c++ )
			{
				assailant[c] = true; // as long as this is active, combat music doesn't turn off
				assailantTimer[c] = COMBAT_MUSIC_COOLDOWN;
			}
		}
		if ( my->monsterSpecialTimer > 0 )
		{
			--my->monsterSpecialTimer;
		}
		else
		{
			my->monsterSpecialTimer = 0;
			if ( my->monsterState == MONSTER_STATE_LICH_CASTSPELLS )
			{
				my->monsterState = MONSTER_STATE_LICH_TELEPORT_ROAMING;
				my->monsterSpecialTimer = 60;
				if ( myStats->type == LICH_FIRE )
				{
					my->lichFireTeleport();
				}
				else
				{
					my->lichIceTeleport();
				}
			}
		}

		if ( my->monsterState != MONSTER_STATE_ATTACK && my->monsterState <= MONSTER_STATE_HUNT )
		{
			my->monsterHitTime = HITRATE * 2;
		}
		//messagePlayer(0, "Ally state: %d", my->monsterLichAllyStatus);
		Entity* lichAlly = nullptr;
		if ( my->ticks > (TICKS_PER_SECOND) 
			&& my->monsterLichAllyStatus == LICH_ALLY_ALIVE 
			&& ticks % (TICKS_PER_SECOND * 2) == 0 )
		{
			if ( myStats->type == LICH_ICE )
			{
				numMonsterTypeAliveOnMap(LICH_FIRE, lichAlly);
				if ( lichAlly == nullptr )
				{
					//messagePlayer(0, "DEAD");
					my->monsterLichAllyStatus = LICH_ALLY_DEAD;
					my->monsterLichAllyUID = 0;
					for ( int c = 0; c < MAXPLAYERS; c++ )
					{
						playSoundPlayer(c, 392, 128);
						messagePlayerColor(c, uint32ColorBaronyBlue(*mainsurface), language[2647]);
					}
				}
				else if ( lichAlly && my->monsterLichAllyUID == 0 )
				{
					my->monsterLichAllyUID = lichAlly->getUID();
				}
			}
			else
			{
				numMonsterTypeAliveOnMap(LICH_ICE, lichAlly);
				if ( lichAlly == nullptr )
				{
					//messagePlayer(0, "DEAD");
					my->monsterLichAllyStatus = LICH_ALLY_DEAD;
					my->monsterLichAllyUID = 0;
					for ( int c = 0; c < MAXPLAYERS; c++ )
					{
						playSoundPlayer(c, 391, 128);
						messagePlayerColor(c, uint32ColorOrange(*mainsurface), language[2649]);
					}
				}
				else if ( lichAlly && my->monsterLichAllyUID == 0 )
				{
					my->monsterLichAllyUID = lichAlly->getUID();
				}
			}
		}
		real_t lichDist = 0.f;
		Entity* target = uidToEntity(my->monsterTarget);

		if ( myStats->OLDHP != myStats->HP && myStats->HP > 0 )
		{
			if ( my->monsterState == MONSTER_STATE_LICH_CASTSPELLS
				&& my->monsterSpecialTimer < 250 )
			{
				if ( rand() % 8 == 0 )
				{
					my->monsterState = MONSTER_STATE_LICH_TELEPORT_ROAMING;
					my->lichFireTeleport();
					my->monsterSpecialTimer = 60;
				}
			}
			if ( my->monsterState <= MONSTER_STATE_HUNT )
			{
				switch ( my->monsterLichBattleState )
				{
					// track when a teleport can happen, battleState needs to be odd numbered to allow stationary teleport
					case 0:
						if ( myStats->HP <= myStats->MAXHP * 0.9 )
						{
							my->monsterLichBattleState = 1;
						}
						break;
					case 2:
						if ( myStats->HP <= myStats->MAXHP * 0.7 )
						{
							my->monsterLichBattleState = 3;
						}
						break;
					case 4:
						if ( myStats->HP <= myStats->MAXHP * 0.5 )
						{
							my->monsterLichBattleState = 5;
						}
						break;
					case 6:
						if ( myStats->HP <= myStats->MAXHP * 0.3 )
						{
							my->monsterLichBattleState = 7;
						}
						break;
					case 8:
						if ( myStats->HP <= myStats->MAXHP * 0.1 )
						{
							my->monsterLichBattleState = 9;
						}
						break;
					default:
						break;
				}
				if ( my->monsterLichBattleState % 2 == 1
					&& (rand() % 5 == 0 
						|| (rand() % 4 == 0 && my->monsterLichTeleportTimer > 0)
						|| (rand() % 2 == 0 && my->monsterLichAllyStatus == LICH_ALLY_DEAD))
					)
				{
					// chance to change state to teleport after being hit.
					if ( my->monsterLichAllyUID != 0 )
					{
						lichAlly = uidToEntity(my->monsterLichAllyUID);
					}
					if ( myStats->type == LICH_FIRE )
					{
						if ( !myStats->EFFECTS[EFF_VAMPIRICAURA] )
						{
							if ( (lichAlly && lichAlly->monsterState != MONSTER_STATE_LICH_CASTSPELLS)
								|| my->monsterLichAllyStatus == LICH_ALLY_DEAD
								|| multiplayer != SINGLE )
							{
								// don't teleport if ally is casting spells. unless multiplayer, then go nuts!
								my->monsterState = MONSTER_STATE_LICHFIRE_TELEPORT_STATIONARY;
								my->lichFireTeleport();
								my->monsterSpecialTimer = 80;
								++my->monsterLichBattleState;
							}
						}
					}
					else if ( myStats->type == LICH_ICE )
					{
						if ( (lichAlly && lichAlly->monsterState != MONSTER_STATE_LICH_CASTSPELLS)
							|| my->monsterLichAllyStatus == LICH_ALLY_DEAD
							|| multiplayer != SINGLE )
						{
							// don't teleport if ally is casting spells. unless multiplayer, then go nuts!
							my->monsterState = MONSTER_STATE_LICHICE_TELEPORT_STATIONARY;
							my->lichIceTeleport();
							my->monsterSpecialTimer = 80;
							++my->monsterLichBattleState;
						}
					}
				}
			}
		}
		if ( my->monsterSpecialTimer == 0 && my->monsterAttack == 0 )
		{
			if ( my->monsterState <= MONSTER_STATE_HUNT && my->monsterTarget )
			{
				if ( target )
				{
					lichDist = sqrt(pow(my->x - target->x, 2) + pow(my->y - target->y, 2));
				}
				if ( ticks % 30 == 0 )
				{
					// check tiles around the monster.
					int sides = 0;
					int my_x = static_cast<int>(my->x) >> 4;
					int my_y = static_cast<int>(my->y) >> 4;
					int mapIndex = (my_y) * MAPLAYERS + (my_x + 1) * MAPLAYERS * map.height;
					if ( map.tiles[OBSTACLELAYER + mapIndex] )   // wall
					{
						++sides;
					}
					mapIndex = (my_y) * MAPLAYERS + (my_x - 1) * MAPLAYERS * map.height;
					if ( map.tiles[OBSTACLELAYER + mapIndex] )   // wall
					{
						++sides;
					}
					mapIndex = (my_y + 1) * MAPLAYERS + (my_x) * MAPLAYERS * map.height;
					if ( map.tiles[OBSTACLELAYER + mapIndex] )   // wall
					{
						++sides;
					}
					mapIndex = (my_y - 1) * MAPLAYERS + (my_x) * MAPLAYERS * map.height;
					if ( map.tiles[OBSTACLELAYER + mapIndex] )   // wall
					{
						++sides;
					}
					//messagePlayer(0, "sides: %d, timer %d", sides, my->monsterLichTeleportTimer);
					if ( sides == 0 )
					{
						my->monsterLichTeleportTimer = 0;
					}
					else
					{
						if ( sides >= 2 )
						{
							my->monsterLichTeleportTimer++;
						}
						else
						{
							if ( rand() % 3 == 0 )
							{
								my->monsterLichTeleportTimer++;
							}
						}
						if ( my->monsterLichTeleportTimer >= 3 )
						{
							// let's teleport, reset the counter inside the teleport functions.
							if ( myStats->type == LICH_FIRE )
							{
								my->lichFireTeleport();
							}
							else
							{
								my->lichIceTeleport();
							}
							my->monsterSpecialTimer = 40;
						}
					}
				}
				if ( myStats->type == LICH_FIRE )
				{
					if ( (	my->monsterLichFireMeleePrev == LICH_ATK_RISING_SINGLE
							|| my->monsterLichFireMeleePrev == LICH_ATK_HORIZONTAL_RETURN)
							&& rand() % 4 == 0 
							&& ticks % 10 == 0
						)
					{
						// chance to dodge immediately after the above 2 attacks
						playSoundEntity(my, 180, 128);
						dir = my->yaw - (PI / 2) + PI * (rand() % 2);
						MONSTER_VELX = cos(dir) * 3;
						MONSTER_VELY = sin(dir) * 3;
						my->monsterState = MONSTER_STATE_LICHFIRE_DODGE;
						my->monsterSpecialTimer = 20;
						my->monsterLichFireMeleePrev = 0;
						my->monsterLichFireMeleeSeq = LICH_ATK_BASICSPELL_SINGLE;
					}
					else if ( myStats->OLDHP != myStats->HP )
					{
						if ( rand() % 4 == 0 )
						{
							// chance to dodge on hp loss
							playSoundEntity(my, 180, 128);
							dir = my->yaw - (PI / 2) + PI * (rand() % 2);
							MONSTER_VELX = cos(dir) * 3;
							MONSTER_VELY = sin(dir) * 3;
							my->monsterState = MONSTER_STATE_LICHFIRE_DODGE;
							my->monsterSpecialTimer = 20;
						}
					}
					else if ( lichDist > 64 )
					{
						if ( target && rand() % 100 == 0 )
						{
							// chance to dodge towards the target if distance is great enough.
							playSoundEntity(my, 180, 128);
							tangent = atan2(target->y - my->y, target->x - my->x);
							dir = tangent;
							while ( dir < 0 )
							{
								dir += 2 * PI;
							}
							while ( dir > 2 * PI )
							{
								dir -= 2 * PI;
							}
							MONSTER_VELX = cos(dir) * 3;
							MONSTER_VELY = sin(dir) * 3;
							my->monsterState = MONSTER_STATE_LICHFIRE_DODGE;
							my->monsterSpecialTimer = 50;
						}
					}
				}
				else if ( myStats->type == LICH_ICE )
				{
					int enemiesInMelee = 0;
					if ( ticks % 50 == 0 )
					{
						// grab enemies around the lich once per second to determine threat level.
						enemiesInMelee = numTargetsAroundEntity(my, 32.0, PI, MONSTER_TARGET_ENEMY);
					}
					if ( myStats->OLDHP != myStats->HP )
					{
						if ( rand() % 3 == 0 )
						{
							// chance to dodge on hp loss
							playSoundEntity(my, 180, 128);
							dir = my->yaw - (PI / 2) + PI * (rand() % 2);
							MONSTER_VELX = cos(dir) * 3;
							MONSTER_VELY = sin(dir) * 3;
							my->monsterState = MONSTER_STATE_LICHICE_DODGE;
							my->monsterSpecialTimer = 30;
							if ( rand() % 2 == 0 )
							{
								// prepare off-hand spell after dodging
								my->monsterLichIceCastPrev = 0;
								my->monsterLichIceCastSeq = LICH_ATK_BASICSPELL_SINGLE;
							}
						}
					}
					else if ( (lichDist < 32) || (enemiesInMelee > 1) )
					{
						if ( target && ticks % 10 == 0 && rand() % 100 == 0 || (enemiesInMelee > 1 && rand() % 8 == 0) )
						{
							// chance to dodge away from target if distance is low enough.
							playSoundEntity(my, 180, 128);
							tangent = atan2(target->y - my->y, target->x - my->x);
							dir = tangent + PI;
							while ( dir < 0 )
							{
								dir += 2 * PI;
							}
							while ( dir > 2 * PI )
							{
								dir -= 2 * PI;
							}
							MONSTER_VELX = cos(dir) * 3;
							MONSTER_VELY = sin(dir) * 3;
							my->monsterState = MONSTER_STATE_LICHICE_DODGE;
							my->monsterSpecialTimer = 20;
							if ( rand() % 2 == 0 )
							{
								// prepare off-hand spell after dodging
								my->monsterLichIceCastPrev = 0;
								my->monsterLichIceCastSeq = LICH_ATK_BASICSPELL_SINGLE;
							}
						}
						else if ( (ticks % 50 == 0 && rand() % 10 == 0) || (enemiesInMelee > 1 && rand() % 4 == 0) )
						{
							my->monsterSpecialTimer = 100;
							my->monsterLichIceCastPrev = 0;
							my->monsterLichIceCastSeq = LICH_ATK_CHARGE_AOE;
						}
					}
					else if ( lichDist > 64 )
					{
						// chance to dodge towards the target if distance is great enough.
						if ( rand() % 100 == 0 )
						{
							if ( my->monsterLichAllyUID != 0 )
							{
								lichAlly = uidToEntity(my->monsterLichAllyUID);
							}
							if ( lichAlly && target )
							{
								// if ally is close to your target, then don't dodge towards.
								if ( entityDist(lichAlly, target) > 48.0 )
								{
									playSoundEntity(my, 180, 128);
									tangent = atan2(target->y - my->y, target->x - my->x);
									dir = tangent;
									while ( dir < 0 )
									{
										dir += 2 * PI;
									}
									while ( dir > 2 * PI )
									{
										dir -= 2 * PI;
									}
									MONSTER_VELX = cos(dir) * 3;
									MONSTER_VELY = sin(dir) * 3;
									my->monsterState = MONSTER_STATE_LICHICE_DODGE;
									my->monsterSpecialTimer = 50;
								}
							}
							if ( my->monsterState != MONSTER_STATE_LICHICE_DODGE )
							{
								// chance to dodge sideways if not set above
								playSoundEntity(my, 180, 128);
								dir = my->yaw - (PI / 2) + PI * (rand() % 2);
								MONSTER_VELX = cos(dir) * 3;
								MONSTER_VELY = sin(dir) * 3;
								my->monsterState = MONSTER_STATE_LICHICE_DODGE;
								my->monsterSpecialTimer = 30;
								if ( rand() % 10 == 0 )
								{
									// prepare off-hand spell after dodging
									my->monsterLichIceCastPrev = 0;
									my->monsterLichIceCastSeq = LICH_ATK_BASICSPELL_SINGLE;
								}
							}
						}
					}
					else if ( my->monsterLichMeleeSwingCount > 3 )
					{
						// reached x successive normal attacks, either move/teleport/dodge around the map
						my->monsterLichMeleeSwingCount = 0;
						if ( rand() % 10 > 0 )
						{
							if ( rand() % 2 == 0 )
							{
								my->monsterTarget = 0;
								my->monsterTargetX = my->x - 50 + rand() % 100;
								my->monsterTargetY = my->y - 50 + rand() % 100;
								my->monsterState = MONSTER_STATE_PATH; // path state
							}
							else
							{
								// chance to dodge
								playSoundEntity(my, 180, 128);
								dir = my->yaw - (PI / 2) + PI * (rand() % 2);
								MONSTER_VELX = cos(dir) * 3;
								MONSTER_VELY = sin(dir) * 3;
								my->monsterState = MONSTER_STATE_LICHICE_DODGE;
								my->monsterSpecialTimer = 30;
								if ( rand() % 2 == 0 )
								{
									// prepare off-hand spell after dodging
									my->monsterLichIceCastPrev = 0;
									my->monsterLichIceCastSeq = LICH_ATK_BASICSPELL_SINGLE;
								}
								else
								{
									my->monsterLichIceCastPrev = 0;
									my->monsterLichIceCastSeq = LICH_ATK_FALLING_DIAGONAL;
								}
							}
						}
					}
				}
			}
			else if ( my->monsterState == MONSTER_STATE_LICHFIRE_TELEPORT_STATIONARY
				|| my->monsterState == MONSTER_STATE_LICHICE_TELEPORT_STATIONARY )
			{
				my->monsterState = MONSTER_STATE_LICH_CASTSPELLS;
				my->monsterSpecialTimer = 500; // cast spells for 10 seconds.
				my->monsterHitTime = 0;
				my->monsterLichMagicCastCount = 0;
				my->monsterLichFireMeleeSeq = 0;
				// acquire a new target.
				lichDist = 1024;
				for ( node = map.creatures->first; node != nullptr; node = node->next ) //Only creatures need to be targetted.
				{
					Entity* tempEntity = (Entity*)node->element;
					if ( tempEntity->behavior == &actPlayer 
						&& (sqrt(pow(my->x - tempEntity->x, 2) + pow(my->y - tempEntity->y, 2)) < lichDist)
						)
					{
						lichDist = sqrt(pow(my->x - tempEntity->x, 2) + pow(my->y - tempEntity->y, 2));
						target = tempEntity;
					}
				}
				if ( target )
				{
					my->monsterAcquireAttackTarget(*target, MONSTER_STATE_LICH_CASTSPELLS);
				}
				my->castOrbitingMagicMissile(SPELL_BLEED, 16.0, 0.0, 500);
				my->castOrbitingMagicMissile(SPELL_BLEED, 16.0, 2 * PI / 5, 500);
				my->castOrbitingMagicMissile(SPELL_BLEED, 16.0, 4 * PI / 5, 500);
				my->castOrbitingMagicMissile(SPELL_BLEED, 16.0, 6 * PI / 5, 500);
				my->castOrbitingMagicMissile(SPELL_BLEED, 16.0, 8 * PI / 5, 500);
			}
			else if ( my->monsterState == MONSTER_STATE_LICH_TELEPORT_ROAMING )
			{
				my->monsterHitTime = 0;
				my->monsterLichMagicCastCount = 0;
				my->monsterLichFireMeleeSeq = 0;
				// acquire a new target.
				lichDist = 1024;
				for ( node = map.creatures->first; node != nullptr; node = node->next ) //Only creatures need to be targetted.
				{
					Entity* tempEntity = (Entity*)node->element;
					if ( tempEntity && tempEntity->behavior == &actPlayer
						&& (sqrt(pow(my->x - tempEntity->x, 2) + pow(my->y - tempEntity->y, 2)) < lichDist)
						)
					{
						lichDist = sqrt(pow(my->x - tempEntity->x, 2) + pow(my->y - tempEntity->y, 2));
						target = tempEntity;
					}
				}
				if ( target )
				{
					my->monsterAcquireAttackTarget(*target, MONSTER_STATE_PATH);
					my->monsterState = MONSTER_STATE_PATH;
				}
				else
				{
					my->monsterState = MONSTER_STATE_WAIT;
				}
			}
		}
	}

	// hunger, regaining hp/mp, poison, etc.
	if ( !intro )
	{
		my->handleEffects(myStats);
	}
	if ( myStats->HP <= 0
		&& my->monsterState != MONSTER_STATE_LICH_DEATH
		&& my->monsterState != MONSTER_STATE_DEVIL_DEATH
		&& my->monsterState != MONSTER_STATE_LICHFIRE_DIE 
		&& my->monsterState != MONSTER_STATE_LICHICE_DIE )
	{
		//TODO: Refactor die function.
		// drop all equipment
		entity = dropItemMonster(myStats->helmet, my, myStats);
		if ( entity )
		{
			entity->flags[USERFLAG1] = true;
		}
		myStats->helmet = NULL;
		entity = dropItemMonster(myStats->breastplate, my, myStats);
		if ( entity )
		{
			entity->flags[USERFLAG1] = true;
		}
		myStats->breastplate = NULL;
		entity = dropItemMonster(myStats->gloves, my, myStats);
		if ( entity )
		{
			entity->flags[USERFLAG1] = true;
		}
		myStats->gloves = NULL;
		entity = dropItemMonster(myStats->shoes, my, myStats);
		if ( entity )
		{
			entity->flags[USERFLAG1] = true;
		}
		myStats->shoes = NULL;
		entity = dropItemMonster(myStats->shield, my, myStats);
		if ( entity )
		{
			entity->flags[USERFLAG1] = true;
		}
		myStats->shield = NULL;
		if ( myStats->weapon )
		{
			if ( itemCategory(myStats->weapon) != SPELLBOOK )
			{
				entity = dropItemMonster(myStats->weapon, my, myStats);
				if ( entity )
				{
					entity->flags[USERFLAG1] = true;
				}
			}
			else
			{
				// spellbooks are not dropped
				if ( myStats->weapon->node )
				{
					list_RemoveNode(myStats->weapon->node);
				}
				else
				{
					free(myStats->weapon);
				}
			}
			myStats->weapon = NULL;
		}
		entity = dropItemMonster(myStats->cloak, my, myStats);
		if ( entity )
		{
			entity->flags[USERFLAG1] = true;
		}
		myStats->cloak = NULL;
		entity = dropItemMonster(myStats->amulet, my, myStats);
		if ( entity )
		{
			entity->flags[USERFLAG1] = true;
		}
		myStats->amulet = NULL;
		entity = dropItemMonster(myStats->ring, my, myStats);
		if ( entity )
		{
			entity->flags[USERFLAG1] = true;
		}
		myStats->ring = NULL;
		entity = dropItemMonster(myStats->mask, my, myStats);
		if ( entity )
		{
			entity->flags[USERFLAG1] = true;
		}
		myStats->mask = NULL;
		node_t* nextnode = NULL;
		for ( node = myStats->inventory.first; node != NULL; node = nextnode )
		{
			nextnode = node->next;
			Item* item = (Item*)node->element;
			for ( c = item->count; c > 0; c-- )
			{
				entity = dropItemMonster(item, my, myStats);
				if ( entity )
				{
					entity->flags[USERFLAG1] = true;    // makes items passable, improves performance
				}
			}
		}

		// broadcast my player allies about my death
		int playerFollower = MAXPLAYERS;
		for (c = 0; c < MAXPLAYERS; c++)
		{
			if (players[c] && players[c]->entity)
			{
				if (myStats->leader_uid == players[c]->entity->getUID())
				{
					playerFollower = c;
					if ( stats[c] )
					{
						for ( node_t* allyNode = stats[c]->FOLLOWERS.first; allyNode != nullptr; allyNode = allyNode->next )
						{
							if ( *((Uint32*)allyNode->element) == my->getUID() )
							{
								list_RemoveNode(allyNode);
								if ( c != clientnum )
								{
									serverRemoveClientFollower(c, my->getUID());
								}
								break;
							}
						}
					}
					break;
				}
			}
		}
		if ( playerFollower < MAXPLAYERS )
		{
			messagePlayerMonsterEvent(c, 0xFFFFFFFF, *myStats, language[1499], language[2589], MSG_OBITUARY);
		}

		// drop gold
		if ( myStats->GOLD > 0 )
		{
			int x = std::min<int>(std::max(0, (int)(my->x / 16)), map.width - 1);
			int y = std::min<int>(std::max(0, (int)(my->y / 16)), map.height - 1);

			// check for floor to drop gold...
			if ( map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height] )
			{
				entity = newEntity(130, 0, map.entities, nullptr); // 130 = goldbag model
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x = my->x;
				entity->y = my->y;
				entity->z = 6;
				entity->yaw = (rand() % 360) * PI / 180.0;
				entity->flags[PASSABLE] = true;
				entity->flags[UPDATENEEDED] = true;
				entity->behavior = &actGoldBag;
				entity->skill[0] = myStats->GOLD; // amount
			}
		}

		// die
#ifdef USE_FMOD
		if ( MONSTER_SOUND )
		{
			FMOD_Channel_Stop(MONSTER_SOUND);
		}
#elif defined USE_OPENAL
		if ( MONSTER_SOUND )
		{
			OPENAL_Channel_Stop(MONSTER_SOUND);
		}
#endif
		myStats = my->getStats();
		switch ( myStats->type )
		{
			case HUMAN:
				humanDie(my);
				break;
			case RAT:
				ratDie(my);
				break;
			case GOBLIN:
				goblinDie(my);
				break;
			case SLIME:
				slimeDie(my);
				break;
			case SCORPION:
				scorpionDie(my);
				break;
			case SUCCUBUS:
				succubusDie(my);
				break;
			case TROLL:
				trollDie(my);
				break;
			case SHOPKEEPER:
				shopkeeperDie(my);
				break;
			case SKELETON:
				skeletonDie(my);
				break;
			case MINOTAUR:
				minotaurDie(my);
				break;
			case GHOUL:
				ghoulDie(my);
				break;
			case DEMON:
				demonDie(my);
				break;
			case SPIDER:
				spiderDie(my);
				break;
			case LICH:
				my->flags[PASSABLE] = true; // so I can't take any more hits
				my->monsterState = MONSTER_STATE_LICH_DEATH; // lich death state
				my->monsterSpecialTimer = 0;
				MONSTER_ATTACK = 0;
				MONSTER_ATTACKTIME = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				for ( c = 0; c < NUMEFFECTS; ++c )
				{
					myStats->EFFECTS[c] = false;
					myStats->EFFECTS_TIMERS[c] = 0;
				}
				break;
			case CREATURE_IMP:
				impDie(my);
				break;
			case GNOME:
				gnomeDie(my);
				break;
			case DEVIL:
				my->flags[PASSABLE] = true; // so I can't take any more hits
				my->monsterState = MONSTER_STATE_DEVIL_DEATH; // devil death state
				my->monsterSpecialTimer = 0;
				MONSTER_ATTACK = 0;
				MONSTER_ATTACKTIME = 0;
				MONSTER_ARMBENDED = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				serverUpdateEntitySkill(my, 10);
				for ( c = 0; c < NUMEFFECTS; ++c )
				{
					myStats->EFFECTS[c] = false;
					myStats->EFFECTS_TIMERS[c] = 0;
				}
				break;
			case AUTOMATON:
				automatonDie(my);
				break;
			case COCKATRICE:
				cockatriceDie(my);
				break;
			case CRYSTALGOLEM:
				crystalgolemDie(my);
				break;
			case SCARAB:
				scarabDie(my);
				break;
			case KOBOLD:
				koboldDie(my);
				break;
			case SHADOW:
				shadowDie(my);
				break;
			case VAMPIRE:
				vampireDie(my);
				break;
			case INCUBUS:
				incubusDie(my);
				break;
			case INSECTOID:
				insectoidDie(my);
				break;
			case GOATMAN:
				goatmanDie(my);
				break;
			case LICH_FIRE:
				my->flags[PASSABLE] = true; // so I can't take any more hits
				my->monsterState = MONSTER_STATE_LICHFIRE_DIE; // lich death state
				my->monsterSpecialTimer = 180;
				my->monsterAttack = 0;
				my->monsterAttackTime = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				serverUpdateEntitySkill(my, 0);
				for ( c = 0; c < NUMEFFECTS; ++c )
				{
					myStats->EFFECTS[c] = false;
					myStats->EFFECTS_TIMERS[c] = 0;
				}
				break;
			case LICH_ICE:
				my->flags[PASSABLE] = true; // so I can't take any more hits
				my->monsterState = MONSTER_STATE_LICHICE_DIE; // lich death state
				my->monsterSpecialTimer = 180;
				my->monsterAttack = 0;
				my->monsterAttackTime = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				serverUpdateEntitySkill(my, 0);
				for ( c = 0; c < NUMEFFECTS; ++c )
				{
					myStats->EFFECTS[c] = false;
					myStats->EFFECTS_TIMERS[c] = 0;
				}
				break;
			default:
				break; //This should never be reached.
		}
		return;
	}

	if ( multiplayer != CLIENT )
	{
		my->effectTimes();
	}

	if ( ticks % TICKS_PER_SECOND == 0 )
	{
		my->checkGroundForItems();
	}

	// check to see if monster can scream again
	if ( MONSTER_SOUND != NULL )
	{
#ifdef USE_FMOD
		FMOD_BOOL playing;
		FMOD_Channel_IsPlaying(MONSTER_SOUND, &playing);
		if (!playing)
		{
			MONSTER_SOUND = NULL;
		}
		else
		{
			for ( c = 0; c < numsounds; c++ )
			{
				/*if( sounds[c] == Mix_GetChunk(MONSTER_SOUND) && ( c<MONSTER_SPOTSND || c>=MONSTER_SPOTSND+MONSTER_SPOTVAR ) ) { //TODO: Is this necessary? If so, port it to FMOD or find a workaround.
					MONSTER_SOUND = -1;
					break;
				}*/
				FMOD_BOOL playing = true;
				FMOD_Channel_IsPlaying(MONSTER_SOUND, &playing);
				if (!playing)
				{
					MONSTER_SOUND = NULL;
					break;
				}
			}
		}
#elif defined USE_OPENAL
		ALboolean playing;
		OPENAL_Channel_IsPlaying(MONSTER_SOUND, &playing);
		if (!playing)
		{
			MONSTER_SOUND = NULL;
		}
		else
		{
			for ( c = 0; c < numsounds; c++ )
			{
				ALboolean playing = true;
				OPENAL_Channel_IsPlaying(MONSTER_SOUND, &playing);
				if (!playing)
				{
					MONSTER_SOUND = NULL;
					break;
				}
			}
		}
#endif
	}

	// remove broken equipment
	if ( myStats->helmet != NULL )
	{
		if ( myStats->helmet->status == BROKEN )
		{
			free(myStats->helmet);
			myStats->helmet = NULL;
		}
	}
	if ( myStats->breastplate != NULL )
	{
		if ( myStats->breastplate->status == BROKEN )
		{
			free(myStats->breastplate);
			myStats->breastplate = NULL;
		}
	}
	if ( myStats->gloves != NULL )
	{
		if ( myStats->gloves->status == BROKEN )
		{
			free(myStats->gloves);
			myStats->gloves = NULL;
		}
	}
	if ( myStats->shoes != NULL )
	{
		if ( myStats->shoes->status == BROKEN )
		{
			free(myStats->shoes);
			myStats->shoes = NULL;
		}
	}
	if ( myStats->shield != NULL )
	{
		if ( myStats->shield->status == BROKEN )
		{
			free(myStats->shield);
			myStats->shield = NULL;
		}
	}
	if ( myStats->weapon != NULL )
	{
		if ( myStats->weapon->status == BROKEN )
		{
			free(myStats->weapon);
			myStats->weapon = NULL;
		}
	}
	if ( myStats->cloak != NULL )
	{
		if ( myStats->cloak->status == BROKEN )
		{
			free(myStats->cloak);
			myStats->cloak = NULL;
		}
	}
	if ( myStats->amulet != NULL )
	{
		if ( myStats->amulet->status == BROKEN )
		{
			free(myStats->amulet);
			myStats->amulet = NULL;
		}
	}
	if ( myStats->ring != NULL )
	{
		if ( myStats->ring->status == BROKEN )
		{
			free(myStats->ring);
			myStats->ring = NULL;
		}
	}
	if ( myStats->mask != NULL )
	{
		if ( myStats->mask->status == BROKEN )
		{
			free(myStats->mask);
			myStats->mask = NULL;
		}
	}

	// calculate weight
	Sint32 weight = 0;
	if ( myStats->helmet != NULL )
	{
		weight += items[myStats->helmet->type].weight * myStats->helmet->count;
	}
	if ( myStats->breastplate != NULL )
	{
		weight += items[myStats->breastplate->type].weight * myStats->breastplate->count;
	}
	if ( myStats->gloves != NULL )
	{
		weight += items[myStats->gloves->type].weight * myStats->gloves->count;
	}
	if ( myStats->shoes != NULL )
	{
		weight += items[myStats->shoes->type].weight * myStats->shoes->count;
	}
	if ( myStats->shield != NULL )
	{
		weight += items[myStats->shield->type].weight * myStats->shield->count;
	}
	if ( myStats->weapon != NULL )
	{
		weight += items[myStats->weapon->type].weight * myStats->weapon->count;
	}
	if ( myStats->cloak != NULL )
	{
		weight += items[myStats->cloak->type].weight * myStats->cloak->count;
	}
	if ( myStats->amulet != NULL )
	{
		weight += items[myStats->amulet->type].weight * myStats->amulet->count;
	}
	if ( myStats->ring != NULL )
	{
		weight += items[myStats->ring->type].weight * myStats->ring->count;
	}
	if ( myStats->mask != NULL )
	{
		weight += items[myStats->mask->type].weight * myStats->mask->count;
	}
	weight += myStats->GOLD / 100;
	weight /= 2; // on monsters weight shouldn't matter so much
	double weightratio = (1000 + my->getSTR() * 100 - weight) / (double)(1000 + my->getSTR() * 100);
	weightratio = fmin(fmax(0, weightratio), 1);
	// determine if I have a ranged weapon or not
	hasrangedweapon = my->hasRangedWeapon();

	// effect of a ring of conflict
	bool ringconflict = false;
	if ( myStats->type != LICH_ICE && myStats->type != LICH_FIRE )
	{
		for ( node = map.creatures->first; node != nullptr; node = node->next ) //Only creatures can wear rings, so don't search map.entities.
		{
			Entity* tempentity = (Entity*)node->element;
			if ( tempentity != nullptr && tempentity != my )
			{
				Stat* tempstats = tempentity->getStats();
				if ( tempstats != nullptr )
				{
					if ( tempstats->ring != nullptr )
					{
						if ( tempstats->ring->type == RING_CONFLICT )
						{
							if ( sqrt(pow(my->x - tempentity->x, 2) + pow(my->y - tempentity->y, 2)) < 200 )
							{
								ringconflict = true;
							}
						}
					}
				}
			}
		}
	}

	// invisibility
	bool handleinvisible = true;
	switch ( myStats->type )
	{
		case HUMAN:
		case GOBLIN:
		case SKELETON:
		case GNOME:
		case KOBOLD:
		case AUTOMATON:
		case INSECTOID:
		case GOATMAN:
		case INCUBUS:
		case SHADOW:
		case VAMPIRE:
		case SUCCUBUS:
		case SHOPKEEPER:
		case LICH_FIRE:
		case LICH_ICE:
			handleinvisible = false;
			break;
		default:
			break;
	}
	if ( handleinvisible )
	{
		//TODO: Should this use isInvisible()?
		if ( myStats->EFFECTS[EFF_INVISIBLE] )
		{
			my->flags[INVISIBLE] = true;
			for ( node = list_Node(&my->children, 2); node != NULL; node = node->next )
			{
				Entity* entity = (Entity*)node->element;
				entity->flags[INVISIBLE] = true;
			}
		}
		else
		{
			my->flags[INVISIBLE] = false;
			for ( node = list_Node(&my->children, 2); node != NULL; node = node->next )
			{
				Entity* entity = (Entity*)node->element;
				entity->flags[INVISIBLE] = false;
			}
		}
	}

	// chatting
	char namesays[32];
	if ( !strcmp(myStats->name, "") )
	{
		if ( myStats->type < KOBOLD ) //Original monster count
		{
			snprintf(namesays, 31, language[513], language[90 + myStats->type]);
		}
		else if ( myStats->type >= KOBOLD ) //New monsters
		{
			snprintf(namesays, 31, language[513], language[2000 + myStats->type - KOBOLD]);
		}
	}
	else
	{
		snprintf(namesays, 31, language[1302], myStats->name);
	}
	int monsterclicked = -1;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if ( (i == 0 && selectedEntity == my) || (client_selected[i] == my) )
		{
			if (inrange[i])
			{
				monsterclicked = i;
			}
		}
	}
	if ( MONSTER_CLICKED )
	{
		monsterclicked = MONSTER_CLICKED - 1;
		MONSTER_CLICKED = 0;
	}
	if ( monsterclicked >= 0 )
	{
		if ( !my->isMobile() )
		{
			// message the player, "the %s doesn't respond"
			messagePlayerMonsterEvent(monsterclicked, 0xFFFFFFFF, *myStats, language[514], language[515], MSG_COMBAT);
		}
		else
		{
			if (my->monsterTarget == players[monsterclicked]->entity->getUID() && my->monsterState != 4)
			{
				// angry at the player, "En Guarde!"
				switch (myStats->type)
				{
					case SHOPKEEPER:
					case HUMAN:
						messagePlayer(monsterclicked, language[516 + rand() % 4], namesays);
						break;
					default:
						break;
				}
			}
			else if (my->monsterState == MONSTER_STATE_TALK)
			{
				// for shopkeepers trading with a player, "I am somewhat busy now."
				if (my->monsterTarget != players[monsterclicked]->entity->getUID())
				{
					switch (myStats->type)
					{
						case SHOPKEEPER:
						case HUMAN:
							messagePlayer(monsterclicked, language[520 + rand() % 4], namesays);
							break;
						default:
							messagePlayer(monsterclicked, language[524], namesays);
							break;
					}
				}
			}
			else
			{
				// handle followers/trading
				if ( myStats->type != SHOPKEEPER )
				{
					if ( myStats->MISC_FLAGS[STAT_FLAG_NPC] == 0 )
					{
						makeFollower(monsterclicked, ringconflict, namesays, my, myStats);
					}
					else
					{
						handleMonsterChatter(monsterclicked, ringconflict, namesays, my, myStats);
					}
					my->lookAtEntity(*players[monsterclicked]->entity);
				}
				else
				{
					if ( !swornenemies[SHOPKEEPER][HUMAN] )
					{
						// shopkeepers start trading
						startTradingServer(my, monsterclicked);
					}
				}
			}
		}
	}

	if ( my->isMobile() )
	{
		// ghouls rise out of the dirt :O
		if ( myStats->type == GHOUL )
		{
			if ( my->z > -.25 )
			{
				my->z -= .25;
				if ( my->z < -.25 )
				{
					my->z = -.25;
				}
				ghoulMoveBodyparts(my, myStats, 0);
				return;
			}
		}

		// being bumped by someone friendly
		for ( node2 = map.entities->first; node2 != nullptr; node2 = node2->next ) //Can't convert to map.creatures because of doorframes.
		{
			entity = (Entity*)node2->element;
			if ( entity == my )
			{
				continue;
			}
			if ( entity->behavior != &actMonster && entity->behavior != &actPlayer && entity->behavior != &actDoorFrame )
			{
				continue;
			}
			if ( entityInsideEntity(my, entity) )
			{
				if ( entity->behavior != &actDoorFrame )
				{
					double tangent = atan2(my->y - entity->y, my->x - entity->x);
					MONSTER_VELX = cos(tangent) * .1;
					MONSTER_VELY = sin(tangent) * .1;
				}
				else
				{
					if ( entity->yaw >= -0.1 && entity->yaw <= 0.1 )
					{
						// east/west doorway
						if ( my->y < floor(my->y / 16) * 16 + 8 )
						{
							// slide south
							MONSTER_VELX = 0;
							MONSTER_VELY = .25;
						}
						else
						{
							// slide north
							MONSTER_VELX = 0;
							MONSTER_VELY = -.25;
						}
					}
					else
					{
						// north/south doorway
						if ( my->x < floor(my->x / 16) * 16 + 8 )
						{
							// slide east
							MONSTER_VELX = .25;
							MONSTER_VELY = 0;
						}
						else
						{
							// slide west
							MONSTER_VELX = -.25;
							MONSTER_VELY = 0;
						}
					}
					//messagePlayer(0, "path: %d", my->monsterPathCount);
					++my->monsterPathCount;
					if ( my->monsterPathCount > 50 )
					{
						my->monsterPathCount = 0;
						monsterMoveAside(my, my);
					}
				}


				if ( (entity->sprite == 274 || entity->sprite == 646 
					|| entity->sprite == 650 || entity->sprite == 304) 
					&& entity->flags[PASSABLE] == true )
				{
					// LICH/LICH_FIRE/LICH_ICE/DEVIL
					// If these guys are PASSABLE then they're either dying or some other animation
					// Move the monster inside the boss, but don't set PASSABLE to false again.
					clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
				}
				else
				{
					entity->flags[PASSABLE] = true;
					clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
					entity->flags[PASSABLE] = false;
				}
			}
		}

		if ( myStats->type != LICH 
			&& myStats->type != DEVIL 
			&& myStats->type != LICH_ICE
			&& myStats->type != LICH_FIRE
			&& my->monsterSpecialTimer > 0 )
		{
			--my->monsterSpecialTimer;
		}


		if ( myStats->type == AUTOMATON )
		{
			my->automatonRecycleItem();
		}

		//if ( myStats->type == SHADOW && uidToEntity(my->monsterTarget)
		//	|| myStats->type == LICH_ICE )
		//{
		//	std::string state_string;

		//	switch(my->monsterState)
		//	{
		//	case MONSTER_STATE_WAIT:
		//		state_string = "WAIT";
		//		break;
		//	case MONSTER_STATE_ATTACK:
		//		state_string = "CHARGE";
		//		break;
		//	case MONSTER_STATE_PATH:
		//		state_string = "PATH";
		//		break;
		//	case MONSTER_STATE_HUNT:
		//		state_string = "HUNT";
		//		break;
		//	case MONSTER_STATE_TALK:
		//		state_string = "TALK";
		//		break;
		//	default:
		//		state_string = "Unknown state";
		//		break;
		//	}

		//	messagePlayer(0, "My state is %s", state_string.c_str()); //Debug message.
		//}

		//Begin state machine
		if ( my->monsterState == MONSTER_STATE_WAIT ) //Begin wait state
		{
			//my->monsterTarget = -1; //TODO: Setting it to -1 = Bug? -1 may not work properly for cases such as: if ( !my->monsterTarget )
			my->monsterReleaseAttackTarget();
			MONSTER_VELX = 0;
			MONSTER_VELY = 0;
			if ( myReflex )
			{
				for ( node2 = map.creatures->first; node2 != nullptr; node2 = node2->next ) //So my concern is that this never explicitly checks for actMonster or actPlayer, instead it relies on there being stats. Now, only monsters and players have stats, so that's not a problem, except...actPlayerLimb can still return a stat from getStat()! D: Meh, if you can find the player's hand, you can find the actual player too, so it shouldn't be an issue.
				{
					entity = (Entity*)node2->element;
					if ( entity == my || entity->flags[PASSABLE] )
					{
						continue;
					}
					hitstats = entity->getStats();
					if ( hitstats != nullptr )
					{
						if ( (my->checkEnemy(entity) || my->monsterTarget == entity->getUID() || ringconflict) )
						{
							tangent = atan2( entity->y - my->y, entity->x - my->x );
							dir = my->yaw - tangent;
							while ( dir >= PI )
							{
								dir -= PI * 2;
							}
							while ( dir < -PI )
							{
								dir += PI * 2;
							}

							// skip if light level is too low and distance is too high
							int light = entity->entityLightAfterReductions(*hitstats, *my);
							if ( (myStats->type >= LICH && myStats->type < KOBOLD) || myStats->type == LICH_FIRE || myStats->type == LICH_ICE || myStats->type == SHADOW )
							{
								//See invisible.
								light = 1000;
							}
							double targetdist = sqrt( pow(my->x - entity->x, 2) + pow(my->y - entity->y, 2) );
							if ( targetdist > sightranges[myStats->type] )
							{
								continue;
							}
							if ( targetdist > TOUCHRANGE && targetdist > light )
							{
								if ( !levitating )
								{
									lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, true);
								}
								else
								{
									lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, false);
								}
								if ( hit.entity == entity )
									if ( rand() % 100 == 0 )
									{
										entity->increaseSkill(PRO_STEALTH);
									}
								continue;
							}
							bool visiontest = false;
							if ( myStats->type != SPIDER )
							{
								if ( dir >= -7 * PI / 16 && dir <= 7 * PI / 16 )
								{
									visiontest = true;
								}
							}
							else
							{
								if ( dir >= -13 * PI / 16 && dir <= 13 * PI / 16 )
								{
									visiontest = true;
								}
							}
							if ( visiontest )   // vision cone
							{
								if ( (myStats->type >= LICH && myStats->type < KOBOLD) || myStats->type == LICH_FIRE || myStats->type == LICH_ICE || myStats->type == SHADOW )
								{
									//See invisible
									lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, false);
								}
								else
								{
									lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], IGNORE_ENTITIES, false);
								}
								if ( !hit.entity )
								{
									lineTrace(my, my->x, my->y, tangent, TOUCHRANGE, 0, false);
								}
								if ( hit.entity == entity )
								{
									/*my->monsterState = MONSTER_STATE_ATTACK; // charge state
									my->monsterTarget = hit.entity->getUID();
									my->monsterTargetX = hit.entity->x;
									my->monsterTargetY = hit.entity->y;*/
									Entity& attackTarget = *hit.entity;
									my->monsterAcquireAttackTarget(attackTarget, MONSTER_STATE_ATTACK);

									if ( MONSTER_SOUND == nullptr )
									{
										if ( myStats->type != MINOTAUR )
										{
											MONSTER_SOUND = playSoundEntity(my, MONSTER_SPOTSND + rand() % MONSTER_SPOTVAR, 128);
										}
										else
										{
											int c;
											for ( c = 0; c < MAXPLAYERS; ++c )
											{
												if ( c == 0 )
												{
													MONSTER_SOUND = playSoundPlayer( c, MONSTER_SPOTSND + rand() % MONSTER_SPOTVAR, 128 );
												}
												else
												{
													playSoundPlayer( c, MONSTER_SPOTSND + rand() % MONSTER_SPOTVAR, 128 );
												}
											}
										}
									}

									if ( entity != nullptr )
									{
										if ( entity->behavior == &actPlayer )
										{
											assailant[entity->skill[2]] = true;  // as long as this is active, combat music doesn't turn off
											assailantTimer[entity->skill[2]] = COMBAT_MUSIC_COOLDOWN;
										}
									}

									// alert other monsters of this enemy's presence //TODO: Refactor into its own function.
									for ( node = map.creatures->first; node != nullptr; node = node->next )
									{
										entity = (Entity*)node->element;
										if ( entity->behavior == &actMonster )
										{
											hitstats = entity->getStats();
											if ( hitstats != nullptr )
											{
												if ( hitstats->type == myStats->type )
												{
													if ( entity->skill[0] == 0 )   // monster is waiting
													{
														tangent = atan2( entity->y - my->y, entity->x - my->x );
														lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, false);
														if ( hit.entity == entity )
														{
															/*entity->monsterState = MONSTER_STATE_PATH;
															entity->monsterTarget = my->monsterTarget;
															entity->fskill[2] = my->monsterTargetX;
															entity->fskill[3] = my->monsterTargetY;*/
															entity->monsterAcquireAttackTarget(attackTarget, MONSTER_STATE_PATH);
														}
													}
												}
											}
										}
									}
									break;
								}
							}
						}
					}
				}
			}

			// minotaurs and liches chase players relentlessly.
			if (myReflex)
			{
				if (myStats->type == MINOTAUR 
					|| myStats->type == LICH 
					|| myStats->type == LICH_FIRE 
					|| myStats->type == LICH_ICE 
					|| (myStats->type == CREATURE_IMP && strstr(map.name, "Boss"))
					|| (myStats->type == AUTOMATON && strstr(myStats->name, "corrupted automaton")) )
				{
					double distToPlayer = 0;
					int c, playerToChase = -1;
					for (c = 0; c < MAXPLAYERS; c++)
					{
						if (players[c] && players[c]->entity)
						{
							if (!distToPlayer)
							{
								distToPlayer = sqrt(pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
								playerToChase = c;
							}
							else
							{
								double newDistToPlayer = sqrt(pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
								if (newDistToPlayer < distToPlayer)
								{
									distToPlayer = newDistToPlayer;
									playerToChase = c;
								}
							}
						}
					}
					if ( playerToChase >= 0 && players[playerToChase] && players[playerToChase]->entity )
					{
						/*my->monsterState = MONSTER_STATE_PATH; // path state
						my->monsterTarget = players[playerToChase]->entity->getUID();
						my->monsterTargetX = players[playerToChase]->entity->x;
						my->monsterTargetY = players[playerToChase]->entity->y;*/
						my->monsterAcquireAttackTarget(*players[playerToChase]->entity, MONSTER_STATE_PATH);
					}
					if ( previousMonsterState != my->monsterState )
					{
						serverUpdateEntitySkill(my, 0);
					}
					return;
				}
				else if ( myStats->type == SHADOW && my->monsterTarget && my->monsterState != MONSTER_STATE_ATTACK )
				{
					//Fix shadow state.
					my->monsterState = MONSTER_STATE_PATH;
					//my->monsterTargetX = my->monsterTarget.x;
					//my->monsterTargetY = my->monsterTarget.y;
					serverUpdateEntitySkill(my, 0); //Update monster state because it changed.
					return;
				}
			}

			// follow the leader :)
			if ( myStats->leader_uid != 0 && my->monsterAllyState == ALLY_STATE_DEFAULT && my->getUID() % TICKS_PER_SECOND == ticks % TICKS_PER_SECOND )
			{
				Entity* leader = uidToEntity(myStats->leader_uid);
				if ( leader )
				{
					double dist = sqrt(pow(my->x - leader->x, 2) + pow(my->y - leader->y, 2));
					if ( dist > WAIT_FOLLOWDIST )
					{
						my->monsterReleaseAttackTarget();
						x = ((int)floor(leader->x)) >> 4;
						y = ((int)floor(leader->y)) >> 4;
						int u, v;
						bool foundplace = false;
						for ( u = x - 1; u <= x + 1; u++ )
						{
							for ( v = y - 1; v <= y + 1; v++ )
							{
								if ( !checkObstacle((u << 4) + 8, (v << 4) + 8, my, leader) )
								{
									x = u;
									y = v;
									foundplace = true;
									break;
								}
							}
							if ( foundplace )
							{
								break;
							}
						}
						path = generatePath( (int)floor(my->x / 16), (int)floor(my->y / 16), x, y, my, leader );
						if ( my->children.first != NULL )
						{
							list_RemoveNode(my->children.first);
						}
						node = list_AddNodeFirst(&my->children);
						node->element = path;
						node->deconstructor = &listDeconstructor;
						my->monsterState = MONSTER_STATE_HUNT; // hunt state
						if ( previousMonsterState != my->monsterState )
						{
							serverUpdateEntitySkill(my, 0);
						}
						return;
					}
					else
					{
						tangent = atan2( leader->y - my->y, leader->x - my->x );
						lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, true);
						if ( hit.entity != leader )
						{
							my->monsterReleaseAttackTarget();
							x = ((int)floor(leader->x)) >> 4;
							y = ((int)floor(leader->y)) >> 4;
							int u, v;
							bool foundplace = false;
							for ( u = x - 1; u <= x + 1; u++ )
							{
								for ( v = y - 1; v <= y + 1; v++ )
								{
									if ( !checkObstacle((u << 4) + 8, (v << 4) + 8, my, leader) )
									{
										x = u;
										y = v;
										foundplace = true;
										break;
									}
								}
								if ( foundplace )
								{
									break;
								}
							}
							path = generatePath( (int)floor(my->x / 16), (int)floor(my->y / 16), x, y, my, leader );
							if ( my->children.first != NULL )
							{
								list_RemoveNode(my->children.first);
							}
							node = list_AddNodeFirst(&my->children);
							node->element = path;
							node->deconstructor = &listDeconstructor;
							my->monsterState = MONSTER_STATE_HUNT; // hunt state
							if ( previousMonsterState != my->monsterState )
							{
								serverUpdateEntitySkill(my, 0);
							}
							return;
						}
					}
				}
			}

			// look
			my->monsterLookTime++;
			if ( my->monsterLookTime >= 120 
				&& myStats->type != LICH 
				&& myStats->type != DEVIL
				&& myStats->type != LICH_FIRE
				&& myStats->type != LICH_ICE )
			{
				my->monsterLookTime = 0;
				my->monsterMoveTime--;
				if ( myStats->type != GHOUL && myStats->type != SPIDER )
				{
					my->monsterLookDir = (rand() % 360) * PI / 180;
				}
				if ( rand() % 3 == 0 )
				{
					if ( !MONSTER_SOUND )
					{
						if ( myStats->type != MINOTAUR )
						{
							MONSTER_SOUND = playSoundEntity(my, MONSTER_IDLESND + (rand() % MONSTER_IDLEVAR), 128);
						}
						else
						{
							int c;
							for ( c = 0; c < MAXPLAYERS; c++ )
							{
								if ( c == 0 )
								{
									MONSTER_SOUND = playSoundPlayer( c, MONSTER_SPOTSND + rand() % MONSTER_SPOTVAR, 128 );
								}
								else
								{
									playSoundPlayer( c, MONSTER_SPOTSND + rand() % MONSTER_SPOTVAR, 128 );
								}
							}
						}
					}
				}
			}
			if ( my->monsterMoveTime == 0 && (uidToEntity(myStats->leader_uid) == NULL || my->monsterAllyState == ALLY_STATE_DEFEND) )
			{
				std::vector<std::pair<int, int>> possibleCoordinates;
				my->monsterMoveTime = rand() % 30;
				int goodspots = 0;
				int centerX = static_cast<int>(my->x / 16); // grab the coordinates in small form.
				int centerY = static_cast<int>(my->y / 16); // grab the coordinates in small form.
				int lowerX = std::max<int>(0, centerX - (map.width / 2)); // assigned upper/lower x coords from entity start position.
				int upperX = std::min<int>(centerX + (map.width / 2), map.width);

				int lowerY = std::max<int>(0, centerY - (map.height / 2)); // assigned upper/lower y coords from entity start position.
				int upperY = std::min<int>(centerY + (map.height / 2), map.height);
				//messagePlayer(0, "my x: %d, my y: %d, rangex: (%d-%d), rangey: (%d-%d)", centerX, centerY, lowerX, upperX, lowerY, upperY);

				if ( myStats->type != SHOPKEEPER && (myStats->MISC_FLAGS[STAT_FLAG_NPC] == 0 && my->monsterAllyState == ALLY_STATE_DEFAULT) )
				{
					for ( x = lowerX; x < upperX; x++ )
					{
						for ( y = lowerY; y < upperY; y++ )
						{
							if ( !checkObstacle(x << 4, y << 4, my, NULL) )
							{
								goodspots++;
								possibleCoordinates.push_back(std::make_pair(x, y));
							}
						}
					}
				}
				else
				{
					for ( x = 0; x < map.width; x++ )
					{
						for ( y = 0; y < map.height; y++ )
						{
							if ( x << 4 >= my->monsterPathBoundaryXStart && x << 4 <= my->monsterPathBoundaryXEnd
								&& y << 4 >= my->monsterPathBoundaryYStart && y << 4 <= my->monsterPathBoundaryYEnd )
								if ( !checkObstacle(x << 4, y << 4, my, NULL) )
								{
									goodspots++;
									possibleCoordinates.push_back(std::make_pair(x, y));
								}
						}
					}
				}
				if ( goodspots )
				{
					int chosenspot = rand() % goodspots;
					int currentspot = 0;
					bool foundit = false;
					x = possibleCoordinates.at(chosenspot).first;
					y = possibleCoordinates.at(chosenspot).second;
					//messagePlayer(0, "Chose distance: %.1fpercent", 100 * (sqrt(pow(my->x / 16 - (x), 2) + pow(my->y / 16 - (y), 2))) / sqrt(pow(map.height, 2) + pow(map.width, 2)));
					/*for ( x = 0; x < map.width; x++ )
					{
						for ( y = 0; y < map.height; y++ )
						{
							if ( !checkObstacle(x << 4, y << 4, my, NULL) )
							{
								if ( currentspot == chosenspot )
								{
									foundit = true;
									break;
								}
								else
								{
									currentspot++;
								}
							}
						}
						if ( foundit )
						{
							break;
						}
					}*/
					path = generatePath( (int)floor(my->x / 16), (int)floor(my->y / 16), x, y, my, NULL );
					if ( my->children.first != NULL )
					{
						list_RemoveNode(my->children.first);
					}
					node = list_AddNodeFirst(&my->children);
					node->element = path;
					node->deconstructor = &listDeconstructor;
					my->monsterState = MONSTER_STATE_HUNT; // hunt state
				}
			}

			// rotate monster
			dir = my->monsterRotate();

			if ( myStats->type == SHADOW && !uidToEntity(my->monsterTarget) && my->monsterSpecialTimer == 0 && my->monsterSpecialState == 0 && ticks%500 == 0 && rand()%5 == 0 )
			{
				//Random chance for a shadow to teleport around the map if it has nothing better to do.
				//messagePlayer(0, "Shadow idle telepotty.");
				my->monsterSpecialState = SHADOW_TELEPORT_ONLY;
				my->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_SHADOW_PASIVE_TELEPORT;
				my->shadowTeleportToTarget(nullptr, 3); // teleport in closer range
				my->monsterState = MONSTER_STATE_WAIT;
			}
		} //End wait state
		else if ( my->monsterState == MONSTER_STATE_ATTACK ) //Begin charge state
		{
			entity = uidToEntity(my->monsterTarget);
			if ( entity == nullptr )
			{
				my->monsterState = MONSTER_STATE_WAIT;
				if ( previousMonsterState != my->monsterState )
				{
					serverUpdateEntitySkill(my, 0);
				}
				if ( myStats->type == SHADOW )
				{
					//messagePlayer(0, "DEBUG: Shadow lost entity.");
					my->monsterReleaseAttackTarget(true);
					my->monsterState = MONSTER_STATE_WAIT;
					serverUpdateEntitySkill(my, 0); //Update state.
				}
				return;
			}
			if ( entity != nullptr )
			{
				if ( entity->behavior == &actPlayer )
				{
					assailant[entity->skill[2]] = true;  // as long as this is active, combat music doesn't turn off
					assailantTimer[entity->skill[2]] = COMBAT_MUSIC_COOLDOWN;
				}
			}
			my->monsterTargetX = entity->x;
			my->monsterTargetY = entity->y;
			hitstats = entity->getStats();

			if ( myStats->type == SHOPKEEPER && strncmp(map.name, "Mages Guild", 11) )
			{
				// shopkeepers hold a grudge against players
				for ( c = 0; c < MAXPLAYERS; ++c )
				{
					if ( players[c] && players[c]->entity )
					{
						if ( my->monsterTarget == players[c]->entity->getUID() )
						{
							swornenemies[SHOPKEEPER][HUMAN] = true;
							monsterally[SHOPKEEPER][HUMAN] = false;
							break;
						}
					}
				}
			}

			if ( myStats->type != DEVIL )
			{
				// skip if light level is too low and distance is too high
				int light = entity->entityLightAfterReductions(*hitstats, *my);
				if ( (myStats->type >= LICH && myStats->type < KOBOLD) || myStats->type == LICH_FIRE || myStats->type == LICH_ICE || myStats->type == SHADOW )
				{
					//See invisible.
					light = 1000;
				}
				double targetdist = sqrt( pow(my->x - entity->x, 2) + pow(my->y - entity->y, 2) );
				if ( targetdist > sightranges[myStats->type] )
				{
					// if target has left my sight, decide whether or not to path or retreat (stay put).
					if ( my->shouldRetreat(*myStats) )
					{
						my->monsterMoveTime = 0;
						my->monsterState = MONSTER_STATE_WAIT; // wait state
					}
					else
					{
						my->monsterState = MONSTER_STATE_PATH; // path state
					}
				}
				else
				{
					if ( targetdist > TOUCHRANGE && targetdist > light && myReflex )
					{
						tangent = atan2( my->monsterTargetY - my->y, my->monsterTargetX - my->x );
						if ( !levitating )
						{
							lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, true);
						}
						else
						{
							lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, false);
						}
						if ( hit.entity == entity )
						{	
							if ( rand() % 100 == 0 )
							{
								entity->increaseSkill(PRO_STEALTH);
							}
						}
						// if target is within sight range but light level is too low and out of melee range.
						// decide whether or not to path or retreat (stay put).
						if ( my->shouldRetreat(*myStats) )
						{
							my->monsterMoveTime = 0;
							my->monsterState = MONSTER_STATE_WAIT; // wait state
						}
						else
						{
							my->monsterState = MONSTER_STATE_PATH; // path state
						}
					}
					else
					{
						if ( myReflex )
						{
							tangent = atan2( my->monsterTargetY - my->y, my->monsterTargetX - my->x );
							if ( !levitating )
							{
								dist = lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, true);
							}
							else
							{
								dist = lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, false);
							}
						}
						else
						{
							dist = sqrt( pow(my->x - entity->x, 2) + pow(my->y - entity->y, 2) );
						}

						if ( hit.entity != entity && myReflex )
						{
							// if I currently lost sight of my target in a straight line in front of me
							// decide whether or not to path or retreat (stay put).
							if ( my->shouldRetreat(*myStats) )
							{
								my->monsterMoveTime = 0;
								my->monsterState = MONSTER_STATE_WAIT; // wait state
							}
							else
							{
								my->monsterState = MONSTER_STATE_PATH; // path state
							}
						}
						else
						{
							// chaaaarge
							tangent = atan2( entity->y - my->y, entity->x - my->x );
							double tangent2 = tangent;

							// get movement dir
							int goAgain = 0;
timeToGoAgain:
							if ( targetdist > TOUCHRANGE * 1.5 && !hasrangedweapon && !my->shouldRetreat(*myStats) && my->getINT() > -2 )
							{
								if ( MONSTER_FLIPPEDANGLE < 5 )
								{
									if ( (my->ticks + my->getUID()) % (TICKS_PER_SECOND * 4) > TICKS_PER_SECOND * 2 )
									{
										tangent2 += PI / 6;
									}
									else
									{
										tangent2 -= PI / 6;
									}
								}
								else
								{
									if ( (my->ticks + my->getUID()) % (TICKS_PER_SECOND * 4) > TICKS_PER_SECOND * 2 )
									{
										tangent2 += PI / 6;
									}
									else
									{
										tangent2 -= PI / 6;
									}
								}

								Entity* tempHitEntity = hit.entity;
								if ( lineTrace(my, my->x, my->x, tangent2, TOUCHRANGE, 1, false) < TOUCHRANGE )
								{
									MONSTER_FLIPPEDANGLE = (MONSTER_FLIPPEDANGLE < 5) * 10;
									goAgain++;
									if ( goAgain < 2 )
									{
										hit.entity = tempHitEntity;
										goto timeToGoAgain;
									}
									else
									{
										tangent2 = tangent;
									}
								}
								hit.entity = tempHitEntity;
							}
							else
							{
								tangent2 = tangent;
							}


							MONSTER_VELX = cos(tangent2) * .045 * (my->getDEX() + 10) * weightratio;
							MONSTER_VELY = sin(tangent2) * .045 * (my->getDEX() + 10) * weightratio;
							if ( (dist > 16 && !hasrangedweapon && !my->shouldRetreat(*myStats)) || (dist > 160 && hasrangedweapon) )
							{
								if ( my->shouldRetreat(*myStats) )
								{
									MONSTER_VELX *= -.5;
									MONSTER_VELY *= -.5;
									dist2 = clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
								}
								else
								{
									dist2 = clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
								}
								if ( hit.entity != NULL )
								{
									if ( hit.entity->behavior == &actDoor )
									{
										// opens the door if unlocked and monster can do it
										if ( !hit.entity->doorLocked && my->getINT() > -2 )
										{
											if ( !hit.entity->doorDir && !hit.entity->doorStatus )
											{
												hit.entity->doorStatus = 1 + (my->x > hit.entity->x);
												playSoundEntity(hit.entity, 21, 96);
											}
											else if ( hit.entity->doorDir && !hit.entity->doorStatus )
											{
												hit.entity->doorStatus = 1 + (my->y < hit.entity->y);
												playSoundEntity(hit.entity, 21, 96);
											}
										}
										else
										{
											// can't open door, so break it down
											my->monsterHitTime++;
											if ( my->monsterHitTime >= HITRATE )
											{
												my->monsterAttack = my->getAttackPose(); // random attack motion
												my->monsterHitTime = 0;
												hit.entity->doorHealth--; // decrease door health
												if ( myStats->STR > 20 )
												{
													hit.entity->doorHealth -= static_cast<int>(std::max((myStats->STR - 20), 0) / 3); // decrease door health
													hit.entity->doorHealth = std::max(hit.entity->doorHealth, 0);
												}
												if ( myStats->type == MINOTAUR )
												{
													hit.entity->doorHealth = 0;    // minotaurs smash doors instantly
												}
												playSoundEntity(hit.entity, 28, 64);
												if ( hit.entity->doorHealth <= 0 )
												{
													// set direction of splinters
													if ( !hit.entity->doorDir )
													{
														hit.entity->doorSmacked = (my->x > hit.entity->x);
													}
													else
													{
														hit.entity->doorSmacked = (my->y < hit.entity->y);
													}
												}
											}
										}
									}
									else if ( hit.entity->behavior == &actFurniture )
									{
										// break it down!
										my->monsterHitTime++;
										if ( my->monsterHitTime >= HITRATE )
										{
											my->monsterAttack = my->getAttackPose(); // random attack motion
											my->monsterHitTime = HITRATE / 4;
											hit.entity->furnitureHealth--; // decrease door health
											if ( myStats->STR > 20 )
											{
												hit.entity->furnitureHealth -= static_cast<int>(std::max((myStats->STR - 20), 0) / 3); // decrease door health
												hit.entity->furnitureHealth = std::max(hit.entity->furnitureHealth, 0);
											}
											if ( myStats->type == MINOTAUR )
											{
												hit.entity->furnitureHealth = 0;    // minotaurs smash furniture instantly
											}
											playSoundEntity(hit.entity, 28, 64);
										}
									}
									else
									{
										if ( my->shouldRetreat(*myStats) )
										{
											my->monsterMoveTime = 0;
											my->monsterState = MONSTER_STATE_WAIT; // wait state
										}
										else
										{
											my->monsterState = MONSTER_STATE_PATH; // path state
										}
									}
								}
								else
								{
									if ( my->shouldRetreat(*myStats) )
									{
										my->monsterMoveTime = 0;
										my->monsterState = MONSTER_STATE_WAIT; // wait state
									}
									else if ( dist2 <= 0.1 && myStats->HP > myStats->MAXHP / 3 )
									{
										my->monsterState = MONSTER_STATE_PATH; // path state
									}
								}
							}
							else
							{
								if ( my->backupWithRangedWeapon(*myStats, dist, hasrangedweapon) || my->shouldRetreat(*myStats) )
								{
									// injured monsters or monsters with ranged weapons back up
									if ( myStats->type == LICH_ICE )
									{
										double strafeTangent = tangent2;
										//messagePlayer(0, "strafe: %d", my->monsterStrafeDirection);
										if ( ticks % 10 == 0 && my->monsterStrafeDirection != 0 && rand() % 10 == 0 )
										{
											Entity* lichAlly = nullptr;
											if ( my->monsterLichAllyUID != 0 )
											{
												lichAlly = uidToEntity(my->monsterLichAllyUID);
											}
											Entity* tmpEntity = hit.entity;
											lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, false);
											if ( hit.entity != tmpEntity )
											{
												// sight is blocked, keep strafing.
											}
											else
											{
												my->monsterStrafeDirection = 0;
											}
											hit.entity = tmpEntity;
										}
										if ( dist < 64 )
										{
											// move diagonally
											strafeTangent -= ((PI / 4) * my->monsterStrafeDirection);
										}
										else
										{
											// move sideways (dist between 64 and 100 from backupWithRangedWeapon)
											strafeTangent -= ((PI / 2) * my->monsterStrafeDirection);
										}
										MONSTER_VELX = cos(strafeTangent) * .045 * (my->getDEX() + 10) * weightratio * -.5;
										MONSTER_VELY = sin(strafeTangent) * .045 * (my->getDEX() + 10) * weightratio * -.5;
									}
									else
									{
										MONSTER_VELX = cos(tangent2) * .045 * (my->getDEX() + 10) * weightratio * -.5;
										MONSTER_VELY = sin(tangent2) * .045 * (my->getDEX() + 10) * weightratio * -.5;
									}
									dist2 = clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
								}
								else
								{
									// this is just so that the monster rotates. it doesn't actually move
									MONSTER_VELX = cos(tangent) * .02 * .045 * (my->getDEX() + 10) * weightratio;
									MONSTER_VELY = sin(tangent) * .02 * .045 * (my->getDEX() + 10) * weightratio;
								}
							}

							my->handleMonsterAttack(myStats, entity, dist);

							// bust ceilings
							/*if( myStats->type == MINOTAUR ) {
								if( my->x>=0 && my->y>=0 && my->x<map.width<<4 && my->y<map.height<<4 ) {
									if( map.tiles[MAPLAYERS+(int)(my->y/16)*MAPLAYERS+(int)(my->x/16)*MAPLAYERS*map.height] )
										map.tiles[MAPLAYERS+(int)(my->y/16)*MAPLAYERS+(int)(my->x/16)*MAPLAYERS*map.height] = 0;
								}
							}*/

							// rotate monster
							if ( my->backupWithRangedWeapon(*myStats, dist, hasrangedweapon) || my->shouldRetreat(*myStats) )
							{
								real_t tempVelX = cos(tangent2) * .045 * (my->getDEX() + 10) * weightratio * -.5;
								real_t tempVelY = sin(tangent2) * .045 * (my->getDEX() + 10) * weightratio * -.5;
								if ( myStats->type == LICH_ICE )
								{
									// override if we're strafing, keep facing the target
									dir = my->yaw - atan2(-tempVelY, -tempVelX);
								}
								else
								{
									dir = my->yaw - atan2( -MONSTER_VELY, -MONSTER_VELX );
								}
							}
							else
							{
								dir = my->yaw - atan2( MONSTER_VELY, MONSTER_VELX );
							}
							while ( dir >= PI )
							{
								dir -= PI * 2;
							}
							while ( dir < -PI )
							{
								dir += PI * 2;
							}
							my->yaw -= dir / 2;
							while ( my->yaw < 0 )
							{
								my->yaw += 2 * PI;
							}
							while ( my->yaw >= 2 * PI )
							{
								my->yaw -= 2 * PI;
							}
						}
					}
				}
			}
			else
			{
				// devil specific code
				if ( !MONSTER_ATTACK || MONSTER_ATTACK == 4 )
				{
					my->monsterSpecialTimer++;
					if ( my->monsterSpecialTimer > 60 )
					{
						if ( !devilstate )
						{
							if ( !MONSTER_ATTACK )
							{
								int c;
								for ( c = 0; c < MAXPLAYERS; c++ )
								{
									playSoundPlayer(c, 204, 64);
								}
								playSoundEntity(my, 204, 128);
								MONSTER_ATTACK = 4;
								MONSTER_ATTACKTIME = 0;
								MONSTER_ARMBENDED = 1;
								serverUpdateEntitySkill(my, 8);
								serverUpdateEntitySkill(my, 9);
								serverUpdateEntitySkill(my, 10);
							}
							else if ( MONSTER_ATTACKTIME > 90 )
							{
								my->monsterState = MONSTER_STATE_DEVIL_TELEPORT; // devil teleport state
							}
						}
						else
						{
							if ( !devilacted )
							{
								switch ( devilstate )
								{
									case 72:
										my->monsterState = MONSTER_STATE_DEVIL_SUMMON; // devil summoning state
										break;
									case 73:
										MONSTER_ATTACK = 5 + rand() % 2; // fireballs
										break;
									case 74:
										my->monsterState = MONSTER_STATE_DEVIL_BOULDER; // devil boulder drop
										break;
								}
								devilacted = 1;
							}
							else
							{
								if ( rand() % 2 && devilstate == 73 )
								{
									MONSTER_ATTACK = 5 + rand() % 2; // more fireballs
								}
								else
								{
									my->monsterState = MONSTER_STATE_DEVIL_TELEPORT; // devil teleport state
								}
							}
						}
						my->monsterSpecialTimer = 0;
					}
				}
				else if ( MONSTER_ATTACK == 5 || MONSTER_ATTACK == 6 )
				{
					// throw fireballs
					my->yaw = my->yaw + MONSTER_WEAPONYAW;
					castSpell(my->getUID(), &spell_fireball, true, false);
					my->yaw = my->yaw - MONSTER_WEAPONYAW;
				}

				// rotate monster
				tangent = atan2( entity->y - my->y, entity->x - my->x );
				MONSTER_VELX = cos(tangent);
				MONSTER_VELY = sin(tangent);
				dir = my->yaw - atan2( MONSTER_VELY, MONSTER_VELX );
				while ( dir >= PI )
				{
					dir -= PI * 2;
				}
				while ( dir < -PI )
				{
					dir += PI * 2;
				}
				my->yaw -= dir / 2;
				while ( my->yaw < 0 )
				{
					my->yaw += 2 * PI;
				}
				while ( my->yaw >= 2 * PI )
				{
					my->yaw -= 2 * PI;
				}
			}
		} //End charge state
		else if ( my->monsterState == MONSTER_STATE_PATH )     //Begin path state
		{
			if ( myStats->type == DEVIL )
			{
				my->monsterState = MONSTER_STATE_ATTACK;
				if ( previousMonsterState != my->monsterState )
				{
					serverUpdateEntitySkill(my, 0);
				}
				return;
			}

			//Don't path if your target dieded!
			if ( uidToEntity(my->monsterTarget) == nullptr && my->monsterTarget != 0 )
			{
				my->monsterReleaseAttackTarget(true);
				my->monsterState = MONSTER_STATE_WAIT; // wait state
				if ( previousMonsterState != my->monsterState )
				{
					serverUpdateEntitySkill(my, 0);
				}
				return;
			}

			entity = uidToEntity(my->monsterTarget);
			if ( entity != nullptr )
			{
				if ( entity->behavior == &actPlayer )
				{
					assailant[entity->skill[2]] = true;  // as long as this is active, combat music doesn't turn off
					assailantTimer[entity->skill[2]] = COMBAT_MUSIC_COOLDOWN;
				}
				my->monsterTargetX = entity->x;
				my->monsterTargetY = entity->y;
			}
			x = ((int)floor(my->monsterTargetX)) >> 4;
			y = ((int)floor(my->monsterTargetY)) >> 4;
			path = generatePath( (int)floor(my->x / 16), (int)floor(my->y / 16), x, y, my, uidToEntity(my->monsterTarget) );
			if ( my->children.first != nullptr )
			{
				list_RemoveNode(my->children.first);
			}
			node = list_AddNodeFirst(&my->children);
			node->element = path;
			node->deconstructor = &listDeconstructor;
			my->monsterState = MONSTER_STATE_HUNT; // hunt state
			/*if ( myStats->type == SHADOW && entity )
			{
				if ( path == nullptr )
				{
					messagePlayer(0, "Warning: Shadow failed to generate a path to its target.");
				}
			}*/
		} //End path state.
		else if ( my->monsterState == MONSTER_STATE_HUNT ) //Begin hunt state
		{
			if ( myStats->type == SHADOW && my->monsterSpecialState == SHADOW_TELEPORT_ONLY )
			{
				//messagePlayer(0, "Shadow in special state teleport only! Aborting hunt state.");
				my->monsterState = MONSTER_STATE_WAIT;
				return; //Don't do anything, yer casting a spell!
			}
			//Do the shadow's passive teleport to catch up to their target..
			if ( myStats->type == SHADOW && my->monsterSpecialTimer == 0 && my->monsterTarget )
			{
				Entity* target = uidToEntity(my->monsterTarget);
				if ( !target )
				{
					my->monsterReleaseAttackTarget(true);
					my->monsterState = MONSTER_STATE_WAIT;
					serverUpdateEntitySkill(my, 0); //Update state.
					return;
				}

				//If shadow has no path to target, then should do the passive teleport.
				bool passiveTeleport = false;
				if ( my->children.first ) //First child is the path.
				{
					if ( !my->children.first->element )
					{
						//messagePlayer(0, "No path for shadow!");
						passiveTeleport = true;
					}
				}
				else
				{
					//messagePlayer(0, "No path for shadow!");
					passiveTeleport = true; //Path is saved as first child. If no first child, no path!
				}

				//Shadow has path to target, but still passive teleport if far enough away.
				if ( !passiveTeleport )
				{
					int specialRoll = rand() % 50;
					//messagePlayer(0, "roll %d", specialRoll);
					double targetdist = sqrt(pow(my->x - entity->x, 2) + pow(my->y - entity->y, 2));
					if ( specialRoll <= (1 + (targetdist > 80 ? 4 : 0)) )
					{
						passiveTeleport = true;
					}
				}

				if ( passiveTeleport )
				{
					//messagePlayer(0, "Shadow is doing a passive tele.");
					my->monsterSpecialState = SHADOW_TELEPORT_ONLY;
					my->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_SHADOW_PASIVE_TELEPORT;
					my->shadowTeleportToTarget(target, 3); // teleport in closer range
					my->monsterState = MONSTER_STATE_WAIT;
					if ( target && target->behavior == actPlayer )
					{
						messagePlayer(target->skill[2], language[2518]);
					}
					return;
				}
			}

			if ( myReflex && (myStats->type != LICH || my->monsterSpecialTimer <= 0) )
			{
				for ( node2 = map.creatures->first; node2 != nullptr; node2 = node2->next ) //Stats only exist on a creature, so don't iterate all map.entities.
				{
					entity = (Entity*)node2->element;
					if ( entity == my || entity->flags[PASSABLE] )
					{
						continue;
					}
					hitstats = entity->getStats();
					if ( hitstats != nullptr )
					{
						if ( (my->checkEnemy(entity) || my->monsterTarget == entity->getUID() || ringconflict) )
						{
							tangent = atan2( entity->y - my->y, entity->x - my->x );
							dir = my->yaw - tangent;
							while ( dir >= PI )
							{
								dir -= PI * 2;
							}
							while ( dir < -PI )
							{
								dir += PI * 2;
							}

							// skip if light level is too low and distance is too high
							int light = entity->entityLightAfterReductions(*hitstats, *my);
							if ( (myStats->type >= LICH && myStats->type < KOBOLD) || myStats->type == LICH_FIRE || myStats->type == LICH_ICE || myStats->type == SHADOW )
							{
								//See invisible.
								light = 1000;
							}
							double targetdist = sqrt( pow(my->x - entity->x, 2) + pow(my->y - entity->y, 2) );
							if ( targetdist > sightranges[myStats->type] )
							{
								continue;
							}
							if ( targetdist > TOUCHRANGE && targetdist > light )
							{
								if ( !levitating )
								{
									lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, true);
								}
								else
								{
									lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, false);
								}
								if ( hit.entity == entity )
								{
									if ( rand() % 100 == 0 )
									{
										entity->increaseSkill(PRO_STEALTH);
									}
								}
								continue;
							}
							bool visiontest = false;
							if ( myStats->type != SPIDER )
							{
								if ( dir >= -7 * PI / 16 && dir <= 7 * PI / 16 )
								{
									visiontest = true;
								}
							}
							else
							{
								if ( dir >= -13 * PI / 16 && dir <= 13 * PI / 16 )
								{
									visiontest = true;
								}
							}
							if ( visiontest )   // vision cone
							{
								lineTrace(my, my->x + 1, my->y, tangent, sightranges[myStats->type], 0, (levitating == false));
								if ( hit.entity == entity )
								{
									lineTrace(my, my->x - 1, my->y, tangent, sightranges[myStats->type], 0, (levitating == false));
									if ( hit.entity == entity )
									{
										lineTrace(my, my->x, my->y + 1, tangent, sightranges[myStats->type], 0, (levitating == false));
										if ( hit.entity == entity )
										{
											lineTrace(my, my->x, my->y - 1, tangent, sightranges[myStats->type], 0, (levitating == false));
											if ( hit.entity == entity )
											{
												Entity& attackTarget = *hit.entity;
												/*my->monsterTarget = hit.entity->getUID();
												my->monsterState = MONSTER_STATE_ATTACK; // charge state
												my->monsterTargetX = hit.entity->x;
												my->monsterTargetY = hit.entity->y;*/
												my->monsterAcquireAttackTarget(attackTarget, MONSTER_STATE_ATTACK);

												if ( MONSTER_SOUND == NULL )
												{
													if ( myStats->type != MINOTAUR )
													{
														if ( myStats->type != LICH || rand() % 3 == 0 )
														{
															MONSTER_SOUND = playSoundEntity(my, MONSTER_SPOTSND + rand() % MONSTER_SPOTVAR, 128);
														}
													}
													else
													{
														int c;
														for ( c = 0; c < MAXPLAYERS; c++ )
														{
															if ( c == 0 )
															{
																MONSTER_SOUND = playSoundPlayer( c, MONSTER_SPOTSND + rand() % MONSTER_SPOTVAR, 128 );
															}
															else
															{
																playSoundPlayer( c, MONSTER_SPOTSND + rand() % MONSTER_SPOTVAR, 128 );
															}
														}
													}
												}

												if ( entity != nullptr )
												{
													if ( entity->behavior == &actPlayer )
													{
														assailant[entity->skill[2]] = true;  // as long as this is active, combat music doesn't turn off
														assailantTimer[entity->skill[2]] = COMBAT_MUSIC_COOLDOWN;
													}
												}
												break;
											}
										}
									}
								}
							}
						}
					}
				}
			}

			// minotaurs and liches chase players relentlessly.
			if ( myStats->type == MINOTAUR 
				|| (myStats->type == LICH && my->monsterSpecialTimer <= 0)
				|| ((myStats->type == LICH_FIRE || myStats->type == LICH_ICE) && my->monsterSpecialTimer <= 0 )
				|| (myStats->type == CREATURE_IMP && strstr(map.name, "Boss"))
				|| (myStats->type == AUTOMATON && strstr(myStats->name, "corrupted automaton")) )
			{
				bool shouldHuntPlayer = false;
				Entity* playerOrNot = uidToEntity(my->monsterTarget);
				if (playerOrNot)
				{
					if (ticks % 180 == 0 && playerOrNot->behavior == &actPlayer)
					{
						shouldHuntPlayer = true;
					}
				}
				else if (ticks % 180 == 0)
				{
					shouldHuntPlayer = true;
				}
				if (shouldHuntPlayer)
				{
					double distToPlayer = 0;
					int c, playerToChase = -1;
					for (c = 0; c < MAXPLAYERS; c++)
					{
						if (players[c] && players[c]->entity)
						{
							if (!distToPlayer)
							{
								distToPlayer = sqrt(pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
								playerToChase = c;
							}
							else
							{
								double newDistToPlayer = sqrt(pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
								if (newDistToPlayer < distToPlayer)
								{
									distToPlayer = newDistToPlayer;
									playerToChase = c;
								}
							}
						}
					}
					if (playerToChase >= 0)
					{
						/*my->monsterState = MONSTER_STATE_PATH; // path state
						my->monsterTarget = players[playerToChase]->entity->getUID();
						my->monsterTargetX = players[playerToChase]->entity->x;
						my->monsterTargetY = players[playerToChase]->entity->y;*/

						if ( players[playerToChase] && players[playerToChase]->entity )
						{
							my->monsterAcquireAttackTarget(*players[playerToChase]->entity, MONSTER_STATE_PATH);
						}
					}

					if ( previousMonsterState != my->monsterState )
					{
						serverUpdateEntitySkill(my, 0);
					}
					return;
				}
			}
			else if ( myStats->type == SHADOW && my->monsterTarget && (ticks % 180 == 0) )
			{
				if ( !uidToEntity(my->monsterTarget) )
				{
					my->monsterReleaseAttackTarget(true);
					my->monsterState = MONSTER_STATE_WAIT;
					serverUpdateEntitySkill(my, 0); //Update state.
					return;
				}
				my->monsterState = MONSTER_STATE_PATH;
				serverUpdateEntitySkill(my, 0); //Update state.
				return;
			}

			// lich cooldown
			if ( myStats->type == LICH )
			{
				if ( my->monsterSpecialTimer > 0 )
				{
					my->monsterSpecialTimer--;
				}
			}

			// follow the leader :)
			if ( myStats->leader_uid != 0 && my->monsterAllyState == ALLY_STATE_DEFAULT && my->getUID() % TICKS_PER_SECOND == ticks % TICKS_PER_SECOND )
			{
				Entity* leader = uidToEntity(myStats->leader_uid);
				if ( leader )
				{
					double dist = sqrt(pow(my->x - leader->x, 2) + pow(my->y - leader->y, 2));
					if ( dist > HUNT_FOLLOWDIST && !my->monsterTarget )
					{
						x = ((int)floor(leader->x)) >> 4;
						y = ((int)floor(leader->y)) >> 4;
						int u, v;
						bool foundplace = false;
						for ( u = x - 1; u <= x + 1; u++ )
						{
							for ( v = y - 1; v <= y + 1; v++ )
							{
								if ( !checkObstacle((u << 4) + 8, (v << 4) + 8, my, leader) )
								{
									x = u;
									y = v;
									foundplace = true;
									break;
								}
							}
							if ( foundplace )
							{
								break;
							}
						}
						path = generatePath( (int)floor(my->x / 16), (int)floor(my->y / 16), x, y, my, leader );
						if ( my->children.first != NULL )
						{
							list_RemoveNode(my->children.first);
						}
						node = list_AddNodeFirst(&my->children);
						node->element = path;
						node->deconstructor = &listDeconstructor;
						my->monsterState = MONSTER_STATE_HUNT; // hunt state
						if ( previousMonsterState != my->monsterState )
						{
							serverUpdateEntitySkill(my, 0);
						}
						return;
					}
					else
					{
						double tangent = atan2( leader->y - my->y, leader->x - my->x );
						Entity* ohitentity = hit.entity;
						lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, true);
						if ( hit.entity != leader )
						{
							my->monsterReleaseAttackTarget();
							int x = ((int)floor(leader->x)) >> 4;
							int y = ((int)floor(leader->y)) >> 4;
							int u, v;
							bool foundplace = false;
							for ( u = x - 1; u <= x + 1; u++ )
							{
								for ( v = y - 1; v <= y + 1; v++ )
								{
									if ( !checkObstacle((u << 4) + 8, (v << 4) + 8, my, leader) )
									{
										x = u;
										y = v;
										foundplace = true;
										break;
									}
								}
								if ( foundplace )
								{
									break;
								}
							}
							path = generatePath( (int)floor(my->x / 16), (int)floor(my->y / 16), x, y, my, leader );
							if ( my->children.first != NULL )
							{
								list_RemoveNode(my->children.first);
							}
							node = list_AddNodeFirst(&my->children);
							node->element = path;
							node->deconstructor = &listDeconstructor;
							my->monsterState = MONSTER_STATE_HUNT; // hunt state
							if ( previousMonsterState != my->monsterState )
							{
								serverUpdateEntitySkill(my, 0);
							}
							return;
						}
						hit.entity = ohitentity;
					}
				}
			}

			entity = uidToEntity(my->monsterTarget);
			if ( entity != NULL )
			{
				if ( entity->behavior == &actPlayer )
				{
					assailant[entity->skill[2]] = true; // as long as this is active, combat music doesn't turn off
					assailantTimer[entity->skill[2]] = COMBAT_MUSIC_COOLDOWN;
				}
			}
			if ( my->children.first != NULL )
			{
				if ( my->children.first->element != NULL )
				{
					path = (list_t*)my->children.first->element;
					if ( path->first != NULL )
					{
						pathnode = (pathnode_t*)path->first->element;
						dist = sqrt( pow(pathnode->y * 16 + 8 - my->y, 2) + pow(pathnode->x * 16 + 8 - my->x, 2) );
						if ( dist <= 2 )
						{
							list_RemoveNode(pathnode->node);
							if ( rand() % 8 == 0 )
							{
								if ( !MONSTER_SOUND )
								{
									if ( myStats->type != MINOTAUR )
									{
										MONSTER_SOUND = playSoundEntity(my, MONSTER_IDLESND + (rand() % MONSTER_IDLEVAR), 128);
									}
									else
									{
										int c;
										for ( c = 0; c < MAXPLAYERS; c++ )
										{
											if ( c == 0 )
											{
												MONSTER_SOUND = playSoundPlayer( c, MONSTER_IDLESND + (rand() % MONSTER_IDLEVAR), 128 );
											}
											else
											{
												playSoundPlayer( c, MONSTER_IDLESND + (rand() % MONSTER_IDLEVAR), 128 );
											}
										}
									}
								}
							}
						}
						else
						{
							// move monster
							tangent = atan2( pathnode->y * 16 + 8 - my->y, pathnode->x * 16 + 8 - my->x );
							MONSTER_VELX = cos(tangent) * .045 * (my->getDEX() + 10) * weightratio;
							MONSTER_VELY = sin(tangent) * .045 * (my->getDEX() + 10) * weightratio;
							dist2 = clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
							if ( hit.entity != NULL )
							{
								if ( hit.entity->behavior == &actDoor )
								{
									// opens the door if unlocked and monster can do it
									if ( !hit.entity->doorLocked && my->getINT() > -2 )
									{
										if ( !hit.entity->doorDir && !hit.entity->doorStatus )
										{
											hit.entity->doorStatus = 1 + (my->x > hit.entity->x);
											playSoundEntity(hit.entity, 21, 96);
										}
										else if ( hit.entity->doorDir && !hit.entity->doorStatus )
										{
											hit.entity->doorStatus = 1 + (my->y < hit.entity->y);
											playSoundEntity(hit.entity, 21, 96);
										}
									}
									else
									{
										// can't open door, so break it down
										my->monsterHitTime++;
										if ( my->monsterHitTime >= HITRATE )
										{
											my->monsterAttack = my->getAttackPose(); // random attack motion
											my->monsterHitTime = 0;
											hit.entity->doorHealth--; // decrease door health
											if ( myStats->STR > 20 )
											{
												hit.entity->doorHealth -= static_cast<int>(std::max((myStats->STR - 20), 0) / 3); // decrease door health
												hit.entity->doorHealth = std::max(hit.entity->doorHealth, 0);
											}
											if ( myStats->type == MINOTAUR )
											{
												hit.entity->doorHealth = 0;    // minotaurs smash doors instantly
											}
											playSoundEntity(hit.entity, 28, 64);
											if ( hit.entity->doorHealth <= 0 )
											{
												// set direction of splinters
												if ( !hit.entity->doorDir )
												{
													hit.entity->doorSmacked = (my->x > hit.entity->x);
												}
												else
												{
													hit.entity->doorSmacked = (my->y < hit.entity->y);
												}
											}
										}
									}
								}
								else if ( hit.entity->behavior == &actFurniture )
								{
									// break it down!
									my->monsterHitTime++;
									if ( my->monsterHitTime >= HITRATE )
									{
										my->monsterAttack = my->getAttackPose(); // random attack motion
										my->monsterHitTime = HITRATE / 4;
										hit.entity->furnitureHealth--; // decrease door health
										if ( myStats->STR > 20 )
										{
											hit.entity->furnitureHealth -= static_cast<int>(std::max((myStats->STR - 20), 0) / 3); // decrease door health
											hit.entity->furnitureHealth = std::max(hit.entity->furnitureHealth, 0);
										}
										if ( myStats->type == MINOTAUR )
										{
											hit.entity->furnitureHealth = 0;    // minotaurs smash furniture instantly
										}
										playSoundEntity(hit.entity, 28, 64);
									}
								}
								else if ( hit.entity->behavior == &actMonster )
								{
									Stat* yourStats = hit.entity->getStats();
									if ( hit.entity->getUID() == my->monsterTarget )
									{
										//TODO: Refactor with setMonsterStateAttack().
										my->monsterState = MONSTER_STATE_ATTACK; // charge state
									}
									else if ( yourStats )
									{
										if ( !my->checkEnemy(hit.entity) )
										{
											// would you kindly move out of the way, sir?
											if ( !monsterMoveAside(hit.entity, my) )
											{
												my->monsterState = MONSTER_STATE_PATH;    // try something else and remake path
											}
											++my->monsterPathCount;
											if ( my->monsterPathCount > 100 )
											{
												my->monsterPathCount = 0;
												//messagePlayer(0, "running into monster like a fool!");
												my->monsterMoveBackwardsAndPath();
											}
										}
										else if ( my->checkEnemy(hit.entity) )
										{
											/*my->monsterTarget = hit.entity->getUID();
											my->monsterTargetX = hit.entity->x;
											my->monsterTargetY = hit.entity->y;
											my->monsterState = MONSTER_STATE_ATTACK; // charge state*/

											Entity& attackTarget = *hit.entity;
											my->monsterAcquireAttackTarget(attackTarget, MONSTER_STATE_ATTACK);
										}
									}
								}
								else if ( hit.entity->behavior == &actPlayer )
								{
									if ( my->checkEnemy(hit.entity) )
									{
										/*my->monsterTarget = hit.entity->getUID();
										my->monsterTargetX = hit.entity->x;
										my->monsterTargetY = hit.entity->y;
										my->monsterState = MONSTER_STATE_ATTACK; // charge state*/

										Entity& attackTarget = *hit.entity;
										my->monsterAcquireAttackTarget(attackTarget, MONSTER_STATE_ATTACK);
									}
									else
									{
										my->monsterState = MONSTER_STATE_PATH; // try something else and remake path
									}
								}
								else
								{
									my->monsterState = MONSTER_STATE_PATH; // remake path
									if ( myStats->type != LICH_FIRE && myStats->type != LICH_ICE )
									{
										if ( hit.entity->behavior == &actGate || hit.entity->behavior == &actBoulder
											 )
										{
											++my->monsterPathCount;
											if ( hit.entity->behavior == &actBoulder )
											{
												my->monsterPathCount += 5;
											}
											if ( my->monsterPathCount > 100 )
											{
												my->monsterPathCount = 0;
												//messagePlayer(0, "remaking path!");
												my->monsterMoveBackwardsAndPath();
											}
										}
										else
										{
											my->monsterPathCount = 0;
										}
									}
								}
							}
							else
							{
								if ( dist2 <= 0.1 )
								{
									my->monsterState = MONSTER_STATE_PATH;    // remake path
								}
							}

							// rotate monster
							dir = my->yaw - atan2( MONSTER_VELY, MONSTER_VELX );
							while ( dir >= PI )
							{
								dir -= PI * 2;
							}
							while ( dir < -PI )
							{
								dir += PI * 2;
							}
							my->yaw -= dir / 2;
							while ( my->yaw < 0 )
							{
								my->yaw += 2 * PI;
							}
							while ( my->yaw >= 2 * PI )
							{
								my->yaw -= 2 * PI;
							}
						}
					}
					else
					{
						Entity* target = uidToEntity(my->monsterTarget);
						if ( target )
						{
							my->lookAtEntity(*target);
							/*if ( myStats->type == SHADOW )
							{
								messagePlayer(0, "[SHADOW] No path #1: Resetting to wait state.");
							}*/
						}
						my->monsterState = MONSTER_STATE_WAIT; // no path, return to wait state
						if ( my->monsterAllyState == ALLY_STATE_MOVETO )
						{
							if ( my->monsterAllyInteractTarget != 0 )
							{
								//messagePlayer(0, "Interacting with a target!");
								if ( my->monsterAllySetInteract() )
								{
									if ( followerInteractedEntity && followerInteractedEntity->behavior == &actItem )
									{
										my->handleNPCInteractDialogue(*myStats, ALLY_EVENT_INTERACT_ITEM);
									}
									else
									{
										my->handleNPCInteractDialogue(*myStats, ALLY_EVENT_INTERACT_OTHER);
									}
								}
								my->monsterAllyInteractTarget = 0;
								my->monsterAllyState = ALLY_STATE_DEFAULT;
							}
							else
							{
								//messagePlayer(0, "Sent a move to command, defending here!");
								// scan for enemies after reaching move point.
								for ( node = map.creatures->first; node != nullptr; node = node->next )
								{
									Entity* target = (Entity*)node->element;
									if ( target->behavior == &actMonster && my->checkEnemy(target) )
									{
										dist = sqrt(pow(my->x - target->x, 2) + pow(my->y - target->y, 2));
										if ( dist < sightranges[myStats->type] )
										{
											double tangent = atan2(target->y - my->y, target->x - my->x);
											my->monsterLookTime = 1;
											my->monsterMoveTime = rand() % 10 + 1;
											my->monsterLookDir = tangent;
										}
									}
								}
								my->monsterAllyState = ALLY_STATE_DEFEND;
								my->createPathBoundariesNPC();
							}
						}
					}
				}
				else
				{
					Entity* target = uidToEntity(my->monsterTarget);
					if ( target )
					{
						double tangent = atan2( target->y - my->y, target->x - my->x );
						my->monsterLookTime = 1;
						my->monsterMoveTime = rand() % 10 + 1;
						my->monsterLookDir = tangent;
						/*if ( myStats->type == SHADOW )
						{
							messagePlayer(0, "[SHADOW] No path #2: Resetting to wait state.");
						}*/
					}
					my->monsterState = MONSTER_STATE_WAIT; // no path, return to wait state
					if ( my->monsterAllyState == ALLY_STATE_MOVETO )
					{
						messagePlayer(0, "Couldn't reach, retrying.");
						if ( target )
						{
							if ( my->monsterAllySetInteract() )
							{
								// we found our interactable within distance.
								//messagePlayer(0, "Found my interactable.");
								if ( followerInteractedEntity && followerInteractedEntity->behavior == &actItem )
								{
									my->handleNPCInteractDialogue(*myStats, ALLY_EVENT_INTERACT_ITEM);
								}
								else
								{
									my->handleNPCInteractDialogue(*myStats, ALLY_EVENT_INTERACT_OTHER);
								}
								my->monsterAllyInteractTarget = 0;
								my->monsterAllyState = ALLY_STATE_DEFAULT;
							}
							else if ( my->monsterSetPathToLocation(static_cast<int>(target->x / 16), static_cast<int>(target->y / 16), 1) )
							{
								my->monsterState = MONSTER_STATE_HUNT;
								my->monsterAllyState = ALLY_STATE_MOVETO;
								//messagePlayer(0, "Moving to my interactable!.");
								if ( rand() % 10 == 0 )
								{
									my->handleNPCInteractDialogue(*myStats, ALLY_EVENT_MOVETO_REPATH);
								}
							}
							else
							{
								// no path possible, give up.
								//messagePlayer(0, "I can't get to my target.");
								my->handleNPCInteractDialogue(*myStats, ALLY_EVENT_MOVETO_FAIL);
								my->monsterAllyInteractTarget = 0;
								my->monsterAllyState = ALLY_STATE_DEFAULT;
							}
						}
						else
						{
							//messagePlayer(0, "Issued move command, defending here.");
							// scan for enemies after reaching move point.
							for ( node = map.creatures->first; node != nullptr; node = node->next )
							{
								Entity* target = (Entity*)node->element;
								if ( target->behavior == &actMonster && my->checkEnemy(target) )
								{
									dist = sqrt(pow(my->x - target->x, 2) + pow(my->y - target->y, 2));
									if ( dist < sightranges[myStats->type] )
									{
										double tangent = atan2(target->y - my->y, target->x - my->x);
										my->monsterLookTime = 1;
										my->monsterMoveTime = rand() % 10 + 1;
										my->monsterLookDir = tangent;
									}
								}
							}
							my->monsterAllyState = ALLY_STATE_DEFEND;
							my->createPathBoundariesNPC();
						}
					}
				}
			}
			else
			{
				Entity* target = uidToEntity(my->monsterTarget);
				if ( target )
				{
					double tangent = atan2( target->y - my->y, target->x - my->x );
					my->monsterLookTime = 1;
					my->monsterMoveTime = rand() % 10 + 1;
					my->monsterLookDir = tangent;
					/*if ( myStats->type == SHADOW )
					{
						messagePlayer(0, "[SHADOW] No path #3: Resetting to wait state.");
					}*/
				}
				my->monsterState = MONSTER_STATE_WAIT; // no path, return to wait state
				//TODO: Replace with lookAtEntity();
			}
		}
		else if ( my->monsterState == MONSTER_STATE_TALK )     //Begin talk state
		{
			MONSTER_VELX = 0;
			MONSTER_VELY = 0;

			// turn towards target
			Entity* target = uidToEntity(my->monsterTarget);
			if ( target != NULL )
			{
				dir = my->yaw - atan2( target->y - my->y, target->x - my->x );
				while ( dir >= PI )
				{
					dir -= PI * 2;
				}
				while ( dir < -PI )
				{
					dir += PI * 2;
				}
				my->yaw -= dir / 2;
				while ( my->yaw < 0 )
				{
					my->yaw += 2 * PI;
				}
				while ( my->yaw >= 2 * PI )
				{
					my->yaw -= 2 * PI;
				}

				// abandon conversation if distance is too great
				if ( sqrt( pow(my->x - target->x, 2) + pow(my->y - target->y, 2) ) > TOUCHRANGE )
				{
					my->monsterState = MONSTER_STATE_WAIT;
					my->monsterTarget = 0;
					int player = -1;
					if ( target->behavior == &actPlayer )
					{
						player = target->skill[2];
					}
					if ( player == 0 )
					{
						shootmode = false;
						gui_mode = GUI_MODE_INVENTORY;
					}
					else
					{
						// inform client of abandonment
						strcpy((char*)net_packet->data, "SHPC");
						net_packet->address.host = net_clients[player - 1].host;
						net_packet->address.port = net_clients[player - 1].port;
						net_packet->len = 4;
						sendPacketSafe(net_sock, -1, net_packet, player - 1);
					}
					monsterMoveAside(my, target);
				}
			}
			else
			{
				// abandon conversation
				my->monsterState = MONSTER_STATE_WAIT;
				my->monsterTarget = 0;
			}
		} //End talk state
		else if ( my->monsterState == MONSTER_STATE_LICH_DODGE )     // dodge state (herx)
		{
			double dist = 0;
			dist = clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
			if ( dist != sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY) )   // hit obstacle
			{
				my->monsterSpecialTimer = 60;
				if ( rand() % 2 )
				{
					my->monsterState = MONSTER_STATE_WAIT; // wait state
				}
				else
				{
					my->monsterState = MONSTER_STATE_LICH_SUMMON; // summoning state
				}
			}
			else
			{
				my->monsterSpecialTimer++;
				if ( my->monsterSpecialTimer > 20 )
				{
					my->monsterSpecialTimer = 60;
					if ( rand() % 2 )
					{
						my->monsterState = MONSTER_STATE_WAIT; // wait state
					}
					else
					{
						my->monsterState = MONSTER_STATE_LICH_SUMMON; // summoning state
					}
				}
			}
		}
		else if ( my->monsterState == MONSTER_STATE_LICH_SUMMON )     // summoning state (herx)
		{
			MONSTER_ATTACK = 1;
			MONSTER_ATTACKTIME = 0;
			if ( my->monsterSpecialTimer )
			{
				my->monsterSpecialTimer--;
			}
			else
			{
				my->monsterSpecialTimer = 60;
				my->monsterState = MONSTER_STATE_WAIT; // wait state
				playSoundEntity(my, 166, 128);

				Monster creature = NOTHING;
				switch ( rand() % 5 )
				{
					case 0:
					case 1:
						creature = CREATURE_IMP;
						break;
					case 2:
					case 3:
					case 4:
						creature = DEMON;
						break;
				}
				if ( creature != DEMON )
				{
					summonMonster(creature, ((int)(my->x / 16)) * 16 + 8, ((int)(my->y / 16)) * 16 + 8);
				}
				summonMonster(creature, ((int)(my->x / 16)) * 16 + 8, ((int)(my->y / 16)) * 16 + 8);
			}
		}
		else if ( my->monsterState == MONSTER_STATE_LICH_DEATH )     // lich death state
		{
			my->yaw += .5; // rotate
			if ( my->yaw >= PI * 2 )
			{
				my->yaw -= PI * 2;
			}
			MONSTER_ATTACK = 1;
			MONSTER_ATTACKTIME = 0;
			if ( my->monsterSpecialTimer == 0 )
			{
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				int c;
				for ( c = 0; c < MAXPLAYERS; c++ )
				{
					playSoundPlayer(c, 186, 128);
				}
			}
			if ( my->monsterSpecialTimer % 10 == 0 )
			{
				spawnExplosion(my->x - 8 + rand() % 16, my->y - 8 + rand() % 16, -4 + rand() % 8);
			}
			my->monsterSpecialTimer++;
			if ( my->monsterSpecialTimer > 180 )
			{
				lichDie(my);
			}
		}
		else if ( my->monsterState == MONSTER_STATE_LICHFIRE_DIE 
			|| my->monsterState == MONSTER_STATE_LICHICE_DIE )     // lich death state
		{
			if ( my->monsterSpecialTimer < 100 )
			{
				my->yaw += .5; // rotate
				if ( my->yaw >= PI * 2 )
				{
					my->yaw -= PI * 2;
				}
			}
			//messagePlayer(0, "timer: %d", my->monsterSpecialTimer);
			if ( my->monsterSpecialTimer == 180 )
			{
				if ( myStats->type == LICH_FIRE )
				{
					my->monsterAttack = MONSTER_POSE_SPECIAL_WINDUP1;
				}
				else if ( myStats->type == LICH_ICE )
				{
					my->monsterAttack = MONSTER_POSE_SPECIAL_WINDUP2;
				}
				my->monsterAttackTime = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				for ( int c = 0; c < MAXPLAYERS; c++ )
				{
					if ( myStats->type == LICH_FIRE )
					{
						playSoundPlayer(c, 376, 128);
						messagePlayerColor(c, uint32ColorOrange(*mainsurface), language[2646]);
					}
					else if ( myStats->type == LICH_ICE )
					{
						playSoundPlayer(c, 381, 128);
						messagePlayerColor(c, uint32ColorBaronyBlue(*mainsurface), language[2648]);
					}
				}
			}
			if ( my->monsterSpecialTimer % 15 == 0 )
			{
				spawnExplosion(my->x - 8 + rand() % 16, my->y - 8 + rand() % 16, my->z -4 + rand() % 8);
			}
			--my->monsterSpecialTimer;
			if ( my->monsterSpecialTimer <= 0 )
			{
				if ( myStats->type == LICH_FIRE )
				{
					lichFireDie(my);
				}
				else if ( myStats->type == LICH_ICE )
				{
					lichIceDie(my);
				}
			}
		}
		else if ( my->monsterState == MONSTER_STATE_DEVIL_DEATH )     // devil death state
		{
			my->z += .5; // descend slowly
			MONSTER_ATTACK = 4;
			MONSTER_ATTACKTIME = 0;
			/*if( MONSTER_SPECIAL==0 ) {
				int c;
				for( c=0; c<MAXPLAYERS; c++ )
					playSoundPlayer(c,186,128);
			}*/
			if ( my->monsterSpecialTimer == 0 )
			{
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				my->x += cos(my->yaw + PI / 2) * 2;
				my->y += sin(my->yaw + PI / 2) * 2;
			}
			else if ( my->monsterSpecialTimer % 2 == 0 )
			{
				my->x += cos(my->yaw + PI / 2) * 4;
				my->y += sin(my->yaw + PI / 2) * 4;
			}
			else
			{
				my->x -= cos(my->yaw + PI / 2) * 4;
				my->y -= sin(my->yaw + PI / 2) * 4;
			}
			if ( my->monsterSpecialTimer % 10 == 0 )
			{
				spawnExplosion(my->x - 24 + rand() % 48, my->y - 24 + rand() % 48, -16 + rand() % 32);
			}
			my->monsterSpecialTimer++;
			if ( my->z > 96 )
			{
				devilDie(my);
			}
		}
		else if ( my->monsterState == MONSTER_STATE_DEVIL_TELEPORT )     // devil teleport state
		{
			my->flags[PASSABLE] = true;
			my->yaw += .1; // rotate
			if ( my->yaw >= PI * 2 )
			{
				my->yaw -= PI * 2;
			}
			my->z = std::min<int>(my->z + 1, 64); // descend
			MONSTER_ATTACK = 4;
			MONSTER_ATTACKTIME = 0;
			MONSTER_ARMBENDED = 1;
			if ( my->monsterSpecialTimer == 0 )
			{
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				serverUpdateEntitySkill(my, 10);
			}
			++my->monsterSpecialTimer;
			if ( my->z >= 64 )
			{
				node_t* node;
				int c = 0;
				for ( node = map.entities->first; node != nullptr; node = node->next )
				{
					Entity* entity = (Entity*)node->element;
					if ( entity->behavior == &actDevilTeleport )
					{
						if ( entity->x == my->x && entity->y == my->y )
						{
							continue;
						}
						switch ( entity->sprite )
						{
							case 72:
								if ( devilstate == 74 )
								{
									c++;
								}
								continue;
							case 73:
								if ( devilstate == 0 || devilstate == 72 )
								{
									c++;
								}
								continue;
							case 74:
								if ( devilstate == 73 )
								{
									c++;
								}
								continue;
							default:
								continue;
						}
					}
				}
				if ( c )
				{
					int i = rand() % c;
					c = 0;
					for ( node = map.entities->first; node != nullptr; node = node->next )
					{
						Entity* entity = (Entity*)node->element;
						if ( entity->behavior == &actDevilTeleport )
						{
							if ( entity->x == my->x && entity->y == my->y )
							{
								continue;
							}
							switch ( entity->sprite )
							{
								case 72:
									if ( devilstate == 74 )
									{
										if ( c == i )
										{
											break;
										}
										else
										{
											c++;
											continue;
										}
									}
									continue;
								case 73:
									if ( devilstate == 0 || devilstate == 72 )
									{
										if ( c == i )
										{
											break;
										}
										else
										{
											c++;
											continue;
										}
									}
									continue;
								case 74:
									if ( devilstate == 73 )
									{
										if ( c == i )
										{
											break;
										}
										else
										{
											c++;
											continue;
										}
									}
									continue;
								default:
									continue;
							}
							my->x = entity->x;
							my->y = entity->y;
							devilstate = entity->sprite;
							devilacted = 0;
							break;
						}
					}
				}
				my->monsterSpecialTimer = 30;
				my->monsterState = MONSTER_STATE_DEVIL_RISING;
			}
		}
		else if ( my->monsterState == MONSTER_STATE_DEVIL_RISING )     // devil rising state (post-teleport)
		{
			if ( my->monsterSpecialTimer <= 0 )
			{
				my->z = std::max<int>(my->z - 1, -4); // ascend
			}
			else
			{
				--my->monsterSpecialTimer;
				if ( my->monsterSpecialTimer <= 0 )
				{
					if ( myStats->HP > 0 )
					{
						my->flags[PASSABLE] = false;
					}
					node_t* node;
					for ( node = map.creatures->first; node != nullptr; node = node->next ) //Since it only looks at entities that have stats, only creatures can have stats; don't iterate map.entities.
					{
						Entity* entity = (Entity*)node->element;
						if ( entity == my )
						{
							continue;
						}
						if ( entityInsideEntity(my, entity) )
						{
							Stat* stats = entity->getStats();
							if ( stats )
							{
								if ( stats->HP > 0 )
								{
									stats->HP = 0;
								}
							}
						}
					}
				}
			}
			if ( !devilroar )
			{
				if ( my->z <= -4 )
				{
					int j = rand() % 5;
					int c;
					for ( c = 0; c < MAXPLAYERS; c++ )
					{
						playSoundPlayer(c, 204 + j, 64);
					}
					playSoundEntity(my, 204 + j, 128);
					devilroar = 1;
					MONSTER_ATTACK = 4;
					MONSTER_ATTACKTIME = 0;
					MONSTER_ARMBENDED = 1;
					serverUpdateEntitySkill(my, 8);
					serverUpdateEntitySkill(my, 9);
					serverUpdateEntitySkill(my, 10);
				}
				else
				{
					my->yaw += .1; // rotate
					if ( my->yaw >= PI * 2 )
					{
						my->yaw -= PI * 2;
					}
				}
			}
			else
			{
				node_t* tempNode;
				Entity* playertotrack = nullptr;
				for ( tempNode = map.creatures->first; tempNode != nullptr; tempNode = tempNode->next ) //Only inspects players, so don't iterate map.entities.
				{
					Entity* tempEntity = (Entity*)tempNode->element;
					double lowestdist = 5000;
					if ( tempEntity->behavior == &actPlayer )
					{
						double disttoplayer = entityDist(my, tempEntity);
						if ( disttoplayer < lowestdist )
						{
							playertotrack = tempEntity;
						}
					}
				}
				if ( playertotrack )
				{
					my->monsterTarget = playertotrack->getUID();
					my->monsterTargetX = playertotrack->x;
					my->monsterTargetY = playertotrack->y;
					MONSTER_VELX = my->monsterTargetX - my->x;
					MONSTER_VELY = my->monsterTargetY - my->y;
				}
				else
				{
					MONSTER_VELX = 0;
					MONSTER_VELY = 0;
				}

				// rotate monster
				dir = my->yaw - atan2( MONSTER_VELY, MONSTER_VELX );

				// To prevent the Entity's position from being updated by dead reckoning on the CLient, set the velocity to 0 after usage
				MONSTER_VELX = 0.0;
				MONSTER_VELY = 0.0;

				while ( dir >= PI )
				{
					dir -= PI * 2;
				}
				while ( dir < -PI )
				{
					dir += PI * 2;
				}
				my->yaw -= dir / 2;
				while ( my->yaw < 0 )
				{
					my->yaw += 2 * PI;
				}
				while ( my->yaw >= 2 * PI )
				{
					my->yaw -= 2 * PI;
				}

				if ( MONSTER_ATTACKTIME > 60 )
				{
					my->monsterState = MONSTER_STATE_ATTACK;
					MONSTER_ATTACK = 0;
					MONSTER_ATTACKTIME = 0;
					MONSTER_ARMBENDED = 0;
					serverUpdateEntitySkill(my, 8);
					serverUpdateEntitySkill(my, 9);
					serverUpdateEntitySkill(my, 10);
					devilroar = 0;
					MONSTER_VELX = 0;
					MONSTER_VELY = 0;
				}
			}
		}
		else if ( my->monsterState == MONSTER_STATE_DEVIL_SUMMON )     // devil summoning state
		{
			MONSTER_ATTACK = 4;
			MONSTER_ATTACKTIME = 0;
			if ( my->monsterSpecialTimer == 0 )
			{
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
			}
			++my->monsterSpecialTimer;
			if ( my->monsterSpecialTimer > 120 )
			{
				MONSTER_ATTACK = 0;
				MONSTER_ATTACKTIME = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				my->monsterSpecialTimer = 0;
				my->monsterState = MONSTER_STATE_ATTACK;
				node_t* tempNode;
				Entity* playertotrack = nullptr;
				for ( tempNode = map.creatures->first; tempNode != nullptr; tempNode = tempNode->next ) //Only inspects players, so don't iterate map.entities. Technically, only needs to iterate through the players[] array, eh?
				{
					Entity* tempEntity = (Entity*)tempNode->element;
					double lowestdist = 5000;
					if ( tempEntity->behavior == &actPlayer )
					{
						double disttoplayer = entityDist(my, tempEntity);
						if ( disttoplayer < lowestdist )
						{
							playertotrack = tempEntity;
						}
					}
				}
				if ( playertotrack )
				{
					my->monsterTarget = playertotrack->getUID();
					my->monsterTargetX = playertotrack->x;
					my->monsterTargetY = playertotrack->y;
				}

				int c;
				double ox = my->x;
				double oy = my->y;
				for ( c = 0; c < 3; c++ )
				{
					my->x = 21 * 16 + (rand() % (43 - 21)) * 16;
					my->y = 21 * 16 + (rand() % (43 - 21)) * 16;

					playSoundEntity(my, 166, 128);

					Monster creature = NOTHING;
					switch ( rand() % 5 )
					{
						case 0:
						case 1:
							creature = CREATURE_IMP;
							break;
						case 2:
						case 3:
						case 4:
							creature = DEMON;
							break;
					}
					summonMonster(creature, ((int)(my->x / 16)) * 16 + 8, ((int)(my->y / 16)) * 16 + 8);
				}
				my->x = ox;
				my->y = oy;
			}
		}
		else if ( my->monsterState == MONSTER_STATE_DEVIL_BOULDER )     // devil boulder spawn state
		{
			int angle = -1;
			if ( (int)(my->x / 16) == 14 && (int)(my->y / 16) == 32 )
			{
				angle = 0;
			}
			else if ( (int)(my->x / 16) == 32 && (int)(my->y / 16) == 14 )
			{
				angle = 1;
			}
			else if ( (int)(my->x / 16) == 50 && (int)(my->y / 16) == 32 )
			{
				angle = 2;
			}
			else if ( (int)(my->x / 16) == 32 && (int)(my->y / 16) == 50 )
			{
				angle = 3;
			}
			my->yaw = angle * PI / 2;
			my->monsterSpecialTimer++;
			if ( my->monsterSpecialTimer == 30 )
			{
				MONSTER_ATTACK = 1;
				MONSTER_ATTACKTIME = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
			}
			if ( my->monsterSpecialTimer == 60 )
			{
				int c;
				double oyaw = my->yaw;
				for ( c = 0; c < 12; c++ )
				{
					my->yaw = ((double)c + ((rand() % 100) / 100.f)) * (PI * 2) / 12.f;
					castSpell(my->getUID(), &spell_fireball, true, false);
				}
				my->yaw = oyaw;
				for ( c = 0; c < 7; ++c )
				{
					Entity* entity = newEntity(245, 1, map.entities, nullptr); // boulder
					entity->parent = my->getUID();
					if ( angle == 0 )
					{
						entity->x = (20 << 4) + 8;
						entity->y = (32 << 4) + 8 + 32 * c;
					}
					else if ( angle == 1 )
					{
						entity->x = (20 << 4) + 8 + 32 * c;
						entity->y = (20 << 4) + 8;
					}
					else if ( angle == 2 )
					{
						entity->x = (44 << 4) + 8;
						entity->y = (20 << 4) + 8 + 32 * c;
					}
					else if ( angle == 3 )
					{
						entity->x = (32 << 4) + 8 + 32 * c;
						entity->y = (44 << 4) + 8;
					}
					entity->z = -64;
					entity->yaw = angle * (PI / 2.f);
					entity->sizex = 7;
					entity->sizey = 7;
					entity->behavior = &actBoulder;
					entity->flags[UPDATENEEDED] = true;
					entity->flags[PASSABLE] = true;
				}
			}
			if ( my->monsterSpecialTimer == 150 )
			{
				MONSTER_ATTACK = 2;
				MONSTER_ATTACKTIME = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
			}
			if ( my->monsterSpecialTimer == 180 )
			{
				int c;
				double oyaw = my->yaw;
				for ( c = 0; c < 12; ++c )
				{
					my->yaw = ((double)c + ((rand() % 100) / 100.f)) * (PI * 2) / 12.f;
					castSpell(my->getUID(), &spell_fireball, true, false);
				}
				my->yaw = oyaw;
				for ( c = 0; c < 7; ++c )
				{
					Entity* entity = newEntity(245, 1, map.entities, nullptr); // boulder
					entity->parent = my->getUID();
					if ( angle == 0 )
					{
						entity->x = (20 << 4) + 8;
						entity->y = (20 << 4) + 8 + 32 * c;
					}
					else if ( angle == 1 )
					{
						entity->x = (32 << 4) + 8 + 32 * c;
						entity->y = (20 << 4) + 8;
					}
					else if ( angle == 2 )
					{
						entity->x = (44 << 4) + 8;
						entity->y = (32 << 4) + 8 + 32 * c;
					}
					else if ( angle == 3 )
					{
						entity->x = (20 << 4) + 8 + 32 * c;
						entity->y = (44 << 4) + 8;
					}
					entity->z = -64;
					entity->yaw = angle * (PI / 2.f);
					entity->sizex = 7;
					entity->sizey = 7;
					entity->behavior = &actBoulder;
					entity->flags[UPDATENEEDED] = true;
					entity->flags[PASSABLE] = true;
				}
			}
			if ( my->monsterSpecialTimer == 270 )
			{
				MONSTER_ATTACK = 3;
				MONSTER_ATTACKTIME = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
			}
			if ( my->monsterSpecialTimer == 300 )
			{
				int c;
				double oyaw = my->yaw;
				for ( c = 0; c < 12; ++c )
				{
					my->yaw = ((double)c + ((rand() % 100) / 100.f)) * (PI * 2) / 12.f;
					castSpell(my->getUID(), &spell_fireball, true, false);
				}
				my->yaw = oyaw;
				for ( c = 0; c < 12; ++c )
				{
					Entity* entity = newEntity(245, 1, map.entities, nullptr); // boulder
					entity->parent = my->getUID();
					if ( angle == 0 )
					{
						entity->x = (20 << 4) + 8;
						entity->y = (21 << 4) + 8 + 32 * c;
					}
					else if ( angle == 1 )
					{
						entity->x = (21 << 4) + 8 + 32 * c;
						entity->y = (20 << 4) + 8;
					}
					else if ( angle == 2 )
					{
						entity->x = (44 << 4) + 8;
						entity->y = (21 << 4) + 8 + 32 * c;
					}
					else if ( angle == 3 )
					{
						entity->x = (21 << 4) + 8 + 32 * c;
						entity->y = (44 << 4) + 8;
					}
					entity->z = -64;
					entity->yaw = angle * (PI / 2.f);
					entity->sizex = 7;
					entity->sizey = 7;
					entity->behavior = &actBoulder;
					entity->flags[UPDATENEEDED] = true;
					entity->flags[PASSABLE] = true;
				}
			}
			if ( my->monsterSpecialTimer == 420 )   // 420 blaze it faggot
			{
				MONSTER_ATTACK = 0;
				MONSTER_ATTACKTIME = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				my->monsterSpecialTimer = 0;
				my->monsterState = MONSTER_STATE_ATTACK;
				node_t* tempNode;
				Entity* playertotrack = nullptr;
				for ( tempNode = map.creatures->first; tempNode != nullptr; tempNode = tempNode->next ) //Iterate map.creatures, since only inspecting players, not all entities. Technically should just iterate over players[]?
				{
					Entity* tempEntity = (Entity*)tempNode->element;
					double lowestdist = 5000;
					if ( tempEntity->behavior == &actPlayer )
					{
						double disttoplayer = entityDist(my, tempEntity);
						if ( disttoplayer < lowestdist )
						{
							playertotrack = tempEntity;
						}
					}
				}
				if ( playertotrack )
				{
					my->monsterTarget = playertotrack->getUID();
					my->monsterTargetX = playertotrack->x;
					my->monsterTargetY = playertotrack->y;
				}
			}
		}
		else if ( my->monsterState == MONSTER_STATE_LICHFIRE_DODGE
			|| my->monsterState == MONSTER_STATE_LICHICE_DODGE )
		{
			dist = clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
			Entity* target = uidToEntity(my->monsterTarget);
			if ( dist != sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY) )
			{
				my->monsterSpecialTimer = 0; // hit obstacle
			}
			if ( target && my->monsterSpecialTimer != 0 && myStats->type == LICH_FIRE )
			{
				dist = sqrt(pow(my->x - target->x, 2) + pow(my->y - target->y, 2));
				if ( dist < STRIKERANGE && rand () % 20 == 0 )
				{
					my->monsterSpecialTimer = 0; // close enough to target, chance to stop early
				}
			}
			if ( my->monsterSpecialTimer == 0 )
			{
				my->monsterState = MONSTER_STATE_WAIT;
				MONSTER_VELX = 0;
				MONSTER_VELY = 0;
				if ( target )
				{
					my->monsterAcquireAttackTarget(*target, MONSTER_STATE_PATH);
					if ( myStats->type == LICH_FIRE )
					{
						my->monsterHitTime = HITRATE * 2;
						if ( sqrt(pow(my->x - target->x, 2) + pow(my->y - target->y, 2)) < STRIKERANGE )
						{
							if ( rand() % 2 == 0 )
							{
								my->monsterLichFireMeleeSeq = LICH_ATK_RISING_RAIN;
								my->handleMonsterAttack(myStats, target, 0.f);
							}
							else
							{
								my->monsterHitTime = 25;
							}
						}
					}
					else if ( myStats->type == LICH_ICE )
					{
						my->monsterHitTime = HITRATE * 2;
						if ( sqrt(pow(my->x - target->x, 2) + pow(my->y - target->y, 2)) < STRIKERANGE * 2 )
						{
							my->monsterLichIceCastSeq = LICH_ATK_CHARGE_AOE;
							my->handleMonsterAttack(myStats, target, 0.f);
						}
					}
				}
			}
		}
		else if ( my->monsterState == MONSTER_STATE_LICH_CASTSPELLS )
		{
			++my->monsterHitTime;
			if ( myStats->type == LICH_FIRE )
			{
				if ( my->monsterLichFireMeleeSeq == 0 )
				{
					if ( my->monsterHitTime >= 60 || my->monsterLichAllyStatus == LICH_ALLY_DEAD && my->monsterHitTime >= 45 )
					{
						Entity* target = uidToEntity(my->monsterTarget);
						if ( target )
						{
							tangent = atan2(target->y - my->y, target->x - my->x);
							lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, false);
							/*if ( hit.entity )
							{
								messagePlayer(0, "sprite: %d", hit.entity->sprite);
							}*/
							if ( hit.entity == target && rand() % 5 > 0 )
							{
								switch ( rand() % 3 )
								{
									case 0:
										my->monsterLichFireMeleeSeq = LICH_ATK_BASICSPELL_SINGLE;
										my->handleMonsterAttack(myStats, target, entityDist(my, target));
										my->monsterLichFireMeleeSeq = 0;
										break;
									case 1:
										my->monsterLichFireMeleeSeq = LICH_ATK_RISING_SINGLE;
										break;
									case 2:
										my->monsterLichFireMeleeSeq = LICH_ATK_HORIZONTAL_SINGLE;
										break;
									default:
										break;
								}
								my->monsterLichMagicCastCount = 0;
								//my->handleMonsterAttack(myStats, target, entityDist(my, target));
							}
							else
							{
								my->monsterLichFireMeleeSeq = LICH_ATK_RISING_RAIN;
								my->handleMonsterAttack(myStats, target, entityDist(my, target));
								my->monsterLichFireMeleeSeq = 0;
							}
							my->monsterHitTime = 0;
						}
						else
						{
							real_t distToPlayer = 0.f;
							int playerToChase = -1;
							for ( c = 0; c < MAXPLAYERS; c++ )
							{
								if ( players[c] && players[c]->entity )
								{
									if ( !distToPlayer )
									{
										distToPlayer = sqrt(pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
										playerToChase = c;
									}
									else
									{
										double newDistToPlayer = sqrt(pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
										if ( newDistToPlayer < distToPlayer )
										{
											distToPlayer = newDistToPlayer;
											playerToChase = c;
										}
									}
								}
							}
							if ( playerToChase >= 0 && players[playerToChase] && players[playerToChase]->entity )
							{
								my->monsterAcquireAttackTarget(*players[playerToChase]->entity, MONSTER_STATE_PATH);
							}
						}
					}
				}
			
				if ( my->monsterLichFireMeleeSeq != 0
					&& my->monsterHitTime >= 0
					&& my->monsterHitTime % 10 == 0 )
				{
					if ( my->monsterLichFireMeleeSeq == LICH_ATK_RISING_SINGLE )
					{
						if ( my->monsterLichMagicCastCount < 3 + rand() % 2 )
						{
							if ( my->monsterLichMagicCastCount == 0 )
							{
								my->attack(MONSTER_POSE_MELEE_WINDUP3, 0, nullptr);
							}
							else
							{
								Entity* spell = castSpell(my->getUID(), getSpellFromID(SPELL_FIREBALL), true, false);
								spell->yaw += (PI / 64) * (-1 + rand() % 3);
								spell->vel_x = cos(spell->yaw) * 4;
								spell->vel_y = sin(spell->yaw) * 4;
							}
							++my->monsterLichMagicCastCount;
						}
						else
						{
							my->monsterLichFireMeleeSeq = 0;
							my->monsterHitTime = 0;
						}
					}
					else if ( my->monsterLichFireMeleeSeq == LICH_ATK_HORIZONTAL_SINGLE )
					{
						if ( my->monsterLichMagicCastCount < 2 )
						{
							if ( my->monsterLichMagicCastCount == 0 )
							{
								my->attack(MONSTER_POSE_MELEE_WINDUP2, 0, nullptr);
							}
							else
							{
								Entity* spell = castSpell(my->getUID(), getSpellFromID(SPELL_FIREBALL), true, false);
								spell->yaw += PI / 16;
								spell->vel_x = cos(spell->yaw) * 4;
								spell->vel_y = sin(spell->yaw) * 4;
								spell = castSpell(my->getUID(), getSpellFromID(SPELL_FIREBALL), true, false);
								spell->yaw -= PI / 16;
								spell->vel_x = cos(spell->yaw) * 4;
								spell->vel_y = sin(spell->yaw) * 4;
								spell = castSpell(my->getUID(), getSpellFromID(SPELL_FIREBALL), true, false);
							}
							++my->monsterLichMagicCastCount;
						}
						else
						{
							my->monsterLichFireMeleeSeq = 0;
							my->monsterHitTime = 0;
						}
					}
				}
			}
			else if ( myStats->type == LICH_ICE )
			{
				if ( my->monsterLichIceCastSeq == 0 )
				{
					if ( my->monsterHitTime >= 60 || (my->monsterLichAllyStatus == LICH_ALLY_DEAD && my->monsterHitTime >= 45) )
					{
						Entity* target = uidToEntity(my->monsterTarget);
						if ( target )
						{
							tangent = atan2(target->y - my->y, target->x - my->x);
							lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, false);
							switch ( rand() % 4 )
							{
								case 0:
								case 1:
									my->monsterLichIceCastSeq = LICH_ATK_HORIZONTAL_SINGLE;
									break;
								case 2:
								case 3:
									if ( my->monsterLichAllyStatus == LICH_ALLY_DEAD )
									{
										Entity* dummyEntity = nullptr;
										if ( numMonsterTypeAliveOnMap(AUTOMATON, dummyEntity) <= 1 )
										{
											my->monsterLichIceCastSeq = LICH_ATK_SUMMON;
										}
										else
										{
											my->monsterLichIceCastSeq = LICH_ATK_RISING_SINGLE;
										}
									}
									else
									{
										my->monsterLichIceCastSeq = LICH_ATK_RISING_SINGLE;
									}
									break;
								default:
									break;
							}
							my->monsterHitTime = 0;
							my->monsterLichMagicCastCount = 0;
						}
						else
						{
							real_t distToPlayer = 0.f;
							int playerToChase = -1;
							for ( c = 0; c < MAXPLAYERS; c++ )
							{
								if ( players[c] && players[c]->entity )
								{
									if ( !distToPlayer )
									{
										distToPlayer = sqrt(pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
										playerToChase = c;
									}
									else
									{
										double newDistToPlayer = sqrt(pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
										if ( newDistToPlayer < distToPlayer )
										{
											distToPlayer = newDistToPlayer;
											playerToChase = c;
										}
									}
								}
							}
							if ( playerToChase >= 0 && players[playerToChase] && players[playerToChase]->entity )
							{
								my->monsterAcquireAttackTarget(*players[playerToChase]->entity, MONSTER_STATE_PATH);
							}
						}
					}
				}

				if ( my->monsterLichIceCastSeq != 0
					&& my->monsterHitTime >= 0
					&& my->monsterHitTime % 10 == 0 )
				{
					if ( my->monsterLichIceCastSeq == LICH_ATK_RISING_SINGLE
						|| my->monsterLichIceCastSeq == LICH_ATK_HORIZONTAL_SINGLE
						|| my->monsterLichIceCastSeq == LICH_ATK_SUMMON )
					{
						int castLimit = 6;
						if ( my->monsterLichIceCastSeq == LICH_ATK_SUMMON )
						{
							castLimit = 2 + rand() % 2;
						}
						if ( my->monsterLichMagicCastCount < castLimit )
						{
							if ( my->monsterLichMagicCastCount == 0 )
							{
								my->attack(my->getAttackPose(), 0, nullptr);
							}
							else
							{
								if ( my->monsterLichIceCastSeq == LICH_ATK_SUMMON )
								{
									my->lichIceSummonMonster(AUTOMATON);
								}
								else
								{
									Entity* spell = castSpell(my->getUID(), getSpellFromID(SPELL_MAGICMISSILE), true, false);
									real_t horizontalSpeed = 4.0;
									Entity* target = uidToEntity(my->monsterTarget);
									if ( target )
									{
										real_t spellDistance = sqrt(pow(spell->x - target->x, 2) + pow(spell->y - target->y, 2));
										spell->vel_z = 22.0 / (spellDistance / horizontalSpeed);
										if ( rand() % 3 >= 1 )
										{
											// spells will track the velocity of the target.
											real_t ticksToHit = (spellDistance / horizontalSpeed);
											real_t predictx = target->x + (target->vel_x * ticksToHit);
											real_t predicty = target->y + (target->vel_y * ticksToHit);
											tangent = atan2(predicty - spell->y, predictx - spell->x); // assume target will be here when spell lands.
											//messagePlayer(0, "x: %f->%f, y: %f->%f, angle offset: %f", target->x, predictx, target->y, predicty, spell->yaw - tangent);
											spell->yaw = tangent;
										}
										else
										{
											// do some minor variations in spell angle
											spell->yaw += ((PI * (-4 + rand() % 9)) / 100);
										}
									}
									else
									{
										spell->vel_z = 1.6;
										// do some minor variations in spell angle
										spell->yaw += ((PI * (-4 + rand() % 9)) / 100);
									}
									spell->vel_x = horizontalSpeed * cos(spell->yaw);
									spell->vel_y = horizontalSpeed * sin(spell->yaw);
									spell->actmagicIsVertical = MAGIC_ISVERTICAL_XYZ;
									spell->z = -22.0;
									spell->pitch = atan2(spell->vel_z, horizontalSpeed);
								}
							}
							++my->monsterLichMagicCastCount;
						}
						else
						{
							if ( my->monsterLichIceCastSeq == LICH_ATK_SUMMON )
							{
								my->monsterHitTime = 45;
							}
							else
							{
								my->monsterHitTime = 0;
							}
							my->monsterLichIceCastSeq = 0;
						}
					}
				}
			}
		}
		else if ( my->monsterState == MONSTER_STATE_LICHFIRE_TELEPORT_STATIONARY
			|| my->monsterState == MONSTER_STATE_LICHICE_TELEPORT_STATIONARY )
		{
			MONSTER_VELX = 0;
			MONSTER_VELY = 0;
		}
		//End state machine.

	}
	else
	{
		MONSTER_VELX = 0;
		MONSTER_VELY = 0;
	}

	if ( previousMonsterState != my->monsterState )
	{
		serverUpdateEntitySkill(my, 0);
	}

	// move body parts
	myStats = my->getStats();
	if ( myStats != NULL )
	{
		if ( myStats->type == HUMAN )
		{
			humanMoveBodyparts(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == RAT )
		{
			ratAnimate(my, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == GOBLIN )
		{
			goblinMoveBodyparts(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == SLIME )
		{
			slimeAnimate(my, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == SCORPION )
		{
			scorpionAnimate(my, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == SUCCUBUS )
		{
			succubusMoveBodyparts(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == TROLL )
		{
			trollMoveBodyparts(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == SHOPKEEPER )
		{
			shopkeeperMoveBodyparts(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == SKELETON )
		{
			skeletonMoveBodyparts(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == MINOTAUR )
		{
			minotaurMoveBodyparts(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			actMinotaurCeilingBuster(my);
		}
		else if ( myStats->type == GHOUL )
		{
			ghoulMoveBodyparts(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == DEMON )
		{
			demonMoveBodyparts(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
			actDemonCeilingBuster(my);
		}
		else if ( myStats->type == SPIDER )
		{
			spiderMoveBodyparts(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == LICH )
		{
			lichAnimate(my, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == CREATURE_IMP )
		{
			impMoveBodyparts(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == GNOME )
		{
			gnomeMoveBodyparts(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == DEVIL )
		{
			devilMoveBodyparts(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == COCKATRICE )
		{
			cockatriceMoveBodyparts(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == AUTOMATON )
		{
			automatonMoveBodyparts(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == CRYSTALGOLEM )
		{
			crystalgolemMoveBodyparts(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == SCARAB )
		{
			scarabAnimate(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == KOBOLD )
		{
			koboldMoveBodyparts(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == SHADOW )
		{
			shadowMoveBodyparts(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == GOATMAN )
		{
			goatmanMoveBodyparts(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == INSECTOID )
		{
			insectoidMoveBodyparts(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == INCUBUS )
		{
			incubusMoveBodyparts(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == VAMPIRE )
		{
			vampireMoveBodyparts(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == LICH_FIRE )
		{
			lichFireAnimate(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
		else if ( myStats->type == LICH_ICE )
		{
			lichIceAnimate(my, myStats, sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY));
		}
	}
}

void Entity::handleMonsterAttack(Stat* myStats, Entity* target, double dist)
{
	node_t* node = nullptr;
	Entity* entity = nullptr;
	Stat* hitstats = nullptr;
	int charge = 1;

	//TODO: I don't like this function getting called every frame. Find a better place to put it.
	chooseWeapon(target, dist);
	bool hasrangedweapon = this->hasRangedWeapon();
	bool lichRangeCheckOverride = false;
	if ( myStats->type == LICH_FIRE)
	{
		if ( monsterLichFireMeleeSeq == LICH_ATK_BASICSPELL_SINGLE )
		{
			hasrangedweapon = true;
		}
		if ( monsterState == MONSTER_STATE_LICH_CASTSPELLS )
		{
			lichRangeCheckOverride = true;
		}
	}
	else if ( myStats->type == LICH_ICE )
	{
		// lichice todo
		if ( monsterLichIceCastSeq == LICH_ATK_BASICSPELL_SINGLE )
		{
			hasrangedweapon = true;
		}
		if ( monsterStrafeDirection == 0 && rand() % 10 == 0 && ticks % 10 == 0 )
		{
			monsterStrafeDirection = -1 + ((rand() % 2 == 0) ? 2 : 0);
		}
	}

	// check the range to the target, depending on ranged weapon or melee.
	if ( (dist < STRIKERANGE && !hasrangedweapon) || (dist < 160 && hasrangedweapon) || lichRangeCheckOverride )
	{
		// increment the hit time, don't attack until this reaches the hitrate of the weapon
		this->monsterHitTime++;
		int bow = 1;
		if ( hasrangedweapon )
		{
			if ( myStats->weapon && (myStats->weapon->type == SLING || myStats->weapon->type == SHORTBOW || myStats->weapon->type == ARTIFACT_BOW) )
			{
				bow = 2;
			}
		}
		// check if ready to attack
		if ( (this->monsterHitTime >= HITRATE * monsterGlobalAttackTimeMultiplier * bow 
				&& (myStats->type != LICH && myStats->type != LICH_ICE))
			|| (this->monsterHitTime >= 5 && myStats->type == LICH)
			|| (this->monsterHitTime >= HITRATE * 2 && myStats->type == LICH_ICE)
			)
		{
			bool shouldAttack = this->handleMonsterSpecialAttack(myStats, nullptr, dist);
			if ( !shouldAttack )
			{
				// handleMonsterSpecialAttack processed an action where the monster should not try to attack this frame.
				// e.g unequipping/swapping from special weapon, stops punching the air after casting a spell.
				return;
			}

			if ( myStats->type == LICH )
			{
				this->monsterSpecialTimer++;
				if ( this->monsterSpecialTimer >= 5 )
				{
					this->monsterSpecialTimer = 90;
					this->monsterTarget = 0;
					this->monsterTargetX = this->x - 50 + rand() % 100;
					this->monsterTargetY = this->y - 50 + rand() % 100;
					this->monsterState = MONSTER_STATE_PATH; // path state
				}
			}

			// reset the hit timer
			this->monsterHitTime = 0;
			int tracedist = 0;
			if ( lichRangeCheckOverride )
			{
				tracedist = 1024;
			}
			else if ( hasrangedweapon )
			{
				tracedist = 160;
			}
			else
			{
				tracedist = STRIKERANGE;
			}

			// check again for the target in attack range. return the result into hit.entity.
			double newTangent = atan2(target->y - this->y, target->x - this->x);
			if ( lichRangeCheckOverride )
			{
				hit.entity = uidToEntity(monsterTarget);
			}
			else
			{
				lineTrace(this, this->x, this->y, newTangent, tracedist, 0, false);
			}
			if ( hit.entity != nullptr )
			{
				// found the target in range
				hitstats = hit.entity->getStats();
				if ( hit.entity->behavior == &actMonster && !hasrangedweapon )
				{
					// alert the monster!
					if ( hit.entity->skill[0] != MONSTER_STATE_ATTACK )
					{
						//hit.entity->skill[0]=0;
						//hit.entity->skill[4]=0;
						//hit.entity->fskill[4]=atan2(players[player]->y-hit.entity->y,players[player]->x-hit.entity->x);

						/*hit.entity->monsterState = MONSTER_STATE_PATH;
						hit.entity->monsterTarget = this->getUID();
						hit.entity->monsterTargetX = this->x;
						hit.entity->monsterTargetY = this->y;*/

						hit.entity->monsterAcquireAttackTarget(*this, MONSTER_STATE_PATH);
					}
				}
				if ( hitstats != nullptr )
				{
					// prepare attack, set the animation of the attack based on the current weapon.
					int pose = this->getAttackPose();

					// turn to the target, then reset my yaw.
					double oYaw = this->yaw;
					this->yaw = newTangent;
					if ( myStats->type == LICH_FIRE )
					{
						if ( monsterState != MONSTER_STATE_LICH_CASTSPELLS )
						{
							lichFireSetNextAttack(*myStats);
							//messagePlayer(0, "previous %d, next is %d", monsterLichFireMeleePrev, monsterLichFireMeleeSeq);
						}
					}
					else if ( myStats->type == LICH_ICE )
					{
						lichIceSetNextAttack(*myStats);
						//messagePlayer(0, "previous %d, next is %d", monsterLichIceCastPrev, monsterLichIceCastSeq);
						if ( monsterLichIceCastPrev == LICH_ATK_BASICSPELL_SINGLE )
						{
							monsterHitTime = HITRATE;
						}
						if ( monsterSpecialState == LICH_ICE_ATTACK_COMBO )
						{
							monsterHitTime = HITRATE * 2 - 25;
							if ( monsterLichMeleeSwingCount > 1 )
							{
								monsterSpecialState = 0;
								monsterSpecialTimer = 100;
							}
						}
					}
					this->attack(pose, charge, nullptr); // attacku! D:<
					this->yaw = oYaw;
				}
			}
		}
	}

	return;
}

int limbAnimateWithOvershoot(Entity* limb, int axis, double setpointRate, double setpoint, double endpointRate, double endpoint, int dir)
{
	double speedMultiplier = 1.0;

	if ( monsterGlobalAnimationMultiplier != 10 )
	{
		speedMultiplier = monsterGlobalAnimationMultiplier / 10.0;
		setpointRate = setpointRate * speedMultiplier;
		endpointRate = endpointRate * speedMultiplier;
	}

	if ( axis == 0 || limb->monsterAnimationLimbOvershoot == ANIMATE_OVERSHOOT_NONE || dir == ANIMATE_DIR_NONE )
	{
		if ( axis == ANIMATE_PITCH )
		{
			limb->pitch = endpoint;
		}
		else if ( axis == ANIMATE_ROLL )
		{
			limb->roll = endpoint;
		}
		else if ( axis == ANIMATE_YAW )
		{
			limb->yaw = endpoint;
		}
		// no animation required.
		return -1;
	}

	if ( axis == ANIMATE_PITCH )
	{
		if ( limb->monsterAnimationLimbOvershoot == ANIMATE_OVERSHOOT_TO_SETPOINT )
		{
			limb->pitch += setpointRate * dir;
			while ( limb->pitch < 0 )
			{
				limb->pitch += 2 * PI;
			}
			while ( limb->pitch >= 2 * PI )
			{
				limb->pitch -= 2 * PI;
			}

			if ( limbAngleWithinRange(limb->pitch, setpointRate, setpoint) )
			{
				limb->pitch = setpoint;
				limb->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_ENDPOINT;
				return ANIMATE_OVERSHOOT_TO_SETPOINT; //reached setpoint
			}
		}
		else if ( limb->monsterAnimationLimbOvershoot == ANIMATE_OVERSHOOT_TO_ENDPOINT )
		{
			limb->pitch -= endpointRate * dir;
			while ( limb->pitch < 0 )
			{
				limb->pitch += 2 * PI;
			}
			while ( limb->pitch >= 2 * PI )
			{
				limb->pitch -= 2 * PI;
			}

			if ( limbAngleWithinRange(limb->pitch, endpointRate, endpoint) )
			{
				limb->pitch = endpoint;
				limb->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_NONE;
				return ANIMATE_OVERSHOOT_TO_ENDPOINT; //reached endpoint.
			}
		}
	}
	else if ( axis == ANIMATE_ROLL )
	{
		if ( limb->monsterAnimationLimbOvershoot == ANIMATE_OVERSHOOT_TO_SETPOINT )
		{
			limb->roll += setpointRate * dir;
			while ( limb->roll < 0 )
			{
				limb->roll += 2 * PI;
			}
			while ( limb->roll >= 2 * PI )
			{
				limb->roll -= 2 * PI;
			}

			if ( limbAngleWithinRange(limb->roll, setpointRate, setpoint) )
			{
				limb->roll = setpoint;
				limb->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_ENDPOINT;
				return ANIMATE_OVERSHOOT_TO_SETPOINT; //reached setpoint
			}
		}
		else if ( limb->monsterAnimationLimbOvershoot == ANIMATE_OVERSHOOT_TO_ENDPOINT )
		{
			limb->roll -= endpointRate * dir;
			while ( limb->roll < 0 )
			{
				limb->roll += 2 * PI;
			}
			while ( limb->roll >= 2 * PI )
			{
				limb->roll -= 2 * PI;
			}

			if ( limbAngleWithinRange(limb->roll, endpointRate, endpoint) )
			{
				limb->roll = endpoint;
				limb->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_NONE;
				return ANIMATE_OVERSHOOT_TO_ENDPOINT; //reached endpoint.
			}
		}
	}
	else if ( axis == ANIMATE_YAW )
	{
		if ( limb->monsterAnimationLimbOvershoot == ANIMATE_OVERSHOOT_TO_SETPOINT )
		{
			limb->yaw += setpointRate * dir;
			while ( limb->yaw < 0 )
			{
				limb->yaw += 2 * PI;
			}
			while ( limb->yaw >= 2 * PI )
			{
				limb->yaw -= 2 * PI;
			}

			if ( limbAngleWithinRange(limb->yaw, setpointRate, setpoint) )
			{
				limb->yaw = setpoint;
				limb->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_ENDPOINT;
				return ANIMATE_OVERSHOOT_TO_SETPOINT; //reached setpoint
			}
		}
		else if ( limb->monsterAnimationLimbOvershoot == ANIMATE_OVERSHOOT_TO_ENDPOINT )
		{
			limb->yaw -= endpointRate * dir;
			while ( limb->yaw < 0 )
			{
				limb->yaw += 2 * PI;
			}
			while ( limb->yaw >= 2 * PI )
			{
				limb->yaw -= 2 * PI;
			}

			if ( limbAngleWithinRange(limb->yaw, endpointRate, endpoint) )
			{
				limb->yaw = endpoint;
				limb->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_NONE;
				return ANIMATE_OVERSHOOT_TO_ENDPOINT; //reached endpoint.
			}
		}
	}
	else if ( axis == ANIMATE_Z )
	{
		if ( limb->monsterAnimationLimbOvershoot == ANIMATE_OVERSHOOT_TO_SETPOINT )
		{
			limb->z += setpointRate * dir;

			if ( limbAngleWithinRange(limb->z, setpointRate, setpoint) )
			{
				limb->z = setpoint;
				limb->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_ENDPOINT;
				return ANIMATE_OVERSHOOT_TO_SETPOINT; //reached setpoint
			}
		}
		else if ( limb->monsterAnimationLimbOvershoot == ANIMATE_OVERSHOOT_TO_ENDPOINT )
		{
			limb->z -= endpointRate * dir;

			if ( limbAngleWithinRange(limb->z, endpointRate, endpoint) )
			{
				limb->z = endpoint;
				limb->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_NONE;
				return ANIMATE_OVERSHOOT_TO_ENDPOINT; //reached endpoint.
			}
		}
	}

	return -1;
}

int limbAnimateToLimit(Entity* limb, int axis, double rate, double setpoint, bool shake, double shakerate)
{
	if ( axis == 0 )
	{
		return 0;
	}

	double speedMultiplier = 1.0;

	if ( monsterGlobalAnimationMultiplier != 10 )
	{
		speedMultiplier = monsterGlobalAnimationMultiplier / 10.0;
		rate = rate * speedMultiplier;
		shakerate = shakerate * speedMultiplier;
	}

	if ( axis == ANIMATE_YAW )
	{
		while ( limb->yaw < 0 )
		{
			limb->yaw += 2 * PI;
		}
		while ( limb->yaw >= 2 * PI )
		{
			limb->yaw -= 2 * PI;
		}

		if ( limbAngleWithinRange(limb->yaw, rate, setpoint) )
		{
			limb->yaw = setpoint;
			if ( shake )
			{
				if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_NONE )
				{
					// no direction for shake is set.
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_POSITIVE;
				}
				if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_POSITIVE )
				{
					limb->yaw += shakerate;
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_NEGATIVE;
				}
				else if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_NEGATIVE )
				{
					limb->yaw -= shakerate;
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_POSITIVE;
				}
			}
			return 1; //reached setpoint
		}
		limb->yaw += rate;
	} else if ( axis == ANIMATE_PITCH )
	{
		while ( limb->pitch < 0 )
		{
			limb->pitch += 2 * PI;
		}
		while ( limb->pitch >= 2 * PI )
		{
			limb->pitch -= 2 * PI;
		}

		if ( limbAngleWithinRange(limb->pitch, rate, setpoint) )
		{
			limb->pitch = setpoint;
			if ( shake )
			{
				if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_NONE )
				{
					// no direction for shake is set.
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_POSITIVE;
				}
				if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_POSITIVE )
				{
					limb->pitch += shakerate;
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_NEGATIVE;
				}
				else if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_NEGATIVE )
				{
					limb->pitch -= shakerate;
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_POSITIVE;
				}
			}
			return 1; //reached setpoint
		}
		limb->pitch += rate;
	} else if ( axis == ANIMATE_ROLL )
	{
		while ( limb->roll < 0 )
		{
			limb->roll += 2 * PI;
		}
		while ( limb->roll >= 2 * PI )
		{
			limb->roll -= 2 * PI;
		}

		if ( limbAngleWithinRange(limb->roll, rate, setpoint) )
		{
			limb->roll = setpoint;
			if ( shake )
			{
				if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_NONE )
				{
					// no direction for shake is set.
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_POSITIVE;
				}
				if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_POSITIVE )
				{
					limb->roll += shakerate;
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_NEGATIVE;
				}
				else if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_NEGATIVE )
				{
					limb->roll -= shakerate;
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_POSITIVE;
				}
			}
			return 1; //reached setpoint
		}
		limb->roll += rate;
	}
	else if ( axis == ANIMATE_Z )
	{
		if ( limbAngleWithinRange(limb->z, rate, setpoint) )
		{
			limb->z = setpoint;
			if ( shake )
			{
				if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_NONE )
				{
					// no direction for shake is set.
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_POSITIVE;
				}
				if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_POSITIVE )
				{
					limb->z += shakerate;
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_NEGATIVE;
				}
				else if ( limb->monsterAnimationLimbDirection == ANIMATE_DIR_NEGATIVE )
				{
					limb->z -= shakerate;
					limb->monsterAnimationLimbDirection = ANIMATE_DIR_POSITIVE;
				}
			}
			return 1; //reached setpoint
		}
		limb->z += rate;
	}
	else if ( axis == ANIMATE_WEAPON_YAW )
	{
		while ( limb->fskill[5] < 0 )
		{
			limb->fskill[5] += 2 * PI;
		}
		while ( limb->fskill[5] >= 2 * PI )
		{
			limb->fskill[5] -= 2 * PI;
		}

		if ( limbAngleWithinRange(limb->fskill[5], rate, setpoint) )
		{
			limb->fskill[5] = setpoint;
			return 1; //reached setpoint
		}
		limb->fskill[5] += rate;
	}

	return 0;
}

int limbAngleWithinRange(real_t angle, double rate, double setpoint)
{
	if ( rate > 0 )
	{
		if ( (angle <= (setpoint + rate)) && (angle >= (setpoint - rate)) )
		{
			return 1;
		}
	}
	else if ( rate < 0 )
	{
		if ( (angle >= (setpoint + rate)) && (angle <= (setpoint - rate)) )
		{
			return 1;
		}
	}

	return 0;
}

real_t normaliseAngle2PI(real_t angle)
{
	while ( angle >= 2 * PI )
	{
		angle -= 2 * PI;
	}
	while ( angle < 0 )
	{
		angle += 2 * PI;
	}

	return angle;
}

bool forceFollower(Entity& leader, Entity& follower)
{
	Stat* leaderStats = leader.getStats();
	Stat* followerStats = follower.getStats();
	if ( !leaderStats || !followerStats )
	{
		printlog("[forceFollower] Error: Either leader or follower did not have stats.");
		return false;
	}

	Uint32* myuid = (Uint32*) (malloc(sizeof(Uint32)));
	*myuid = follower.getUID();

	//Deal with the old leader.
	if ( followerStats->leader_uid != 0 )
	{
		Entity* oldLeader = uidToEntity(followerStats->leader_uid);
		if ( oldLeader )
		{
			Stat* oldLeaderStats = oldLeader->getStats();
			if ( oldLeaderStats )
			{
				if ( leader.behavior == &actPlayer 
					&& oldLeader == &leader
					&& follower.flags[USERFLAG2] == false )
				{
					steamAchievementClient(leader.skill[2], "BARONY_ACH_CONFESSOR");
				}
				list_RemoveNodeWithElement<Uint32>(oldLeaderStats->FOLLOWERS, *myuid);
				if ( oldLeader->behavior == &actPlayer )
				{
					serverRemoveClientFollower(oldLeader->skill[2], *myuid);
				}
			}
		}
	}

	node_t* newNode = list_AddNodeLast(&leaderStats->FOLLOWERS);
	newNode->deconstructor = &defaultDeconstructor;
	newNode->element = myuid;

	follower.monsterState = 0;
	follower.monsterTarget = 0;
	followerStats->leader_uid = leader.getUID();

	int player = leader.isEntityPlayer();
	if ( player > 0 && multiplayer == SERVER )
	{
		//Tell the client he suckered somebody into his cult.
		strcpy((char*) (net_packet->data), "LEAD");
		SDLNet_Write32((Uint32 )follower.getUID(), &net_packet->data[4]);
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 8;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);

		serverUpdateAllyStat(player, follower.getUID(), followerStats->LVL, followerStats->HP, followerStats->MAXHP, followerStats->type);
	}

	if ( !followerMenuEntityRecent && player == clientnum )
	{
		followerMenuEntityRecent = &follower;
	}
	return true;
}

bool Entity::handleMonsterSpecialAttack(Stat* myStats, Entity* target, double dist)
{
	int specialRoll = 0;
	node_t* node = nullptr;
	int enemiesNearby = 0;
	int bonusFromHP = 0;
	bool hasrangedweapon = this->hasRangedWeapon();

	if ( myStats != nullptr )
	{
		if ( myStats->type == LICH 
			|| myStats->type == DEVIL 
			|| myStats->type == SHOPKEEPER
			|| myStats->type == LICH_FIRE
			|| myStats->type == LICH_ICE )
		{
			// monster should attack after this function is called.
			return true;
		}

		if ( this->monsterSpecialTimer == 0 )
		{
			switch ( myStats->type )
			{
				case KOBOLD:
					if ( hasrangedweapon || myStats->weapon == nullptr )
					{
						specialRoll = rand() % 20;
						//messagePlayer(0, "Rolled: %d", specialRoll);
						if ( myStats->HP < myStats->MAXHP / 2 )
						{
							if ( (dist < 40 && specialRoll < 10) || (dist < 100 && specialRoll < 5) ) // 50%/25% chance
							{
								node = itemNodeInInventory(myStats, static_cast<ItemType>(-1), SPELLBOOK);
								if ( node != nullptr )
								{
									swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
									this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_KOBOLD;
								}
							}
						}
						else if ( myStats->HP < (0.8 * myStats->MAXHP) )
						{
							if ( (dist < 40 && specialRoll < 5) || (dist < 100 && specialRoll < 2) ) // 25%/10% chance
							{
								node = itemNodeInInventory(myStats, static_cast<ItemType>(-1), SPELLBOOK);
								if ( node != nullptr )
								{
									swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
									this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_KOBOLD;
								}
							}
						}
					}
					break;
				case CRYSTALGOLEM:
					specialRoll = rand() % 20;
					enemiesNearby = numTargetsAroundEntity(this, STRIKERANGE, PI, MONSTER_TARGET_ENEMY);
					if ( enemiesNearby > 1 )
					{
						enemiesNearby = std::min(enemiesNearby, 4);
						if ( specialRoll < enemiesNearby * 2 ) // 10% for each enemy > 1, capped at 40%
						{
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_GOLEM;
							break;
						}
					}		
					
					specialRoll = rand() % 20;
					if ( myStats->HP > myStats->MAXHP * 0.8 )
					{
						if ( specialRoll < 2 ) // 10%
						{
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_GOLEM;
						}
					}
					else if ( myStats->HP > myStats->MAXHP * 0.6 )
					{
						if ( specialRoll < 3 ) // 15%
						{
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_GOLEM;
						}
					}
					else if ( myStats->HP > myStats->MAXHP * 0.4 )
					{
						if ( specialRoll < 4 ) // 20%
						{
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_GOLEM;
						}
					}
					else if ( myStats->HP > myStats->MAXHP * 0.2 )
					{
						if ( specialRoll < 5 ) // 25%
						{
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_GOLEM;
						}
					}
					else if ( myStats->HP > myStats->MAXHP * 0.2 )
					{
						if ( specialRoll < 5 ) // 25%
						{
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_GOLEM;
						}
					}
					break;
				case COCKATRICE:
					specialRoll = rand() % 20;
					//specialRoll = 0;
					// check for paralyze first
					enemiesNearby = std::min(numTargetsAroundEntity(this, STRIKERANGE * 2, PI, MONSTER_TARGET_ENEMY), 4);
					
					if ( myStats->HP <= myStats->MAXHP * 0.5 )
					{
						bonusFromHP = 4; // +20% chance if on low health
					}
					if ( specialRoll < (enemiesNearby * 2 + bonusFromHP) ) // +10% for each enemy, capped at 40%
					{
						node = itemNodeInInventory(myStats, static_cast<ItemType>(-1), SPELLBOOK);
						if ( node != nullptr )
						{
							swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_COCKATRICE_STONE;
						}
						break;
					}

					// nothing selected, look for double attack.
					specialRoll = rand() % 20;
					if ( myStats->HP > myStats->MAXHP * 0.8 )
					{
						if ( specialRoll < 2 ) // 10%
						{
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_COCKATRICE_ATK;
						}
					}
					else if ( myStats->HP > myStats->MAXHP * 0.6 )
					{
						if ( specialRoll < 2 ) // 10%
						{
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_COCKATRICE_ATK;
						}
					}
					else if ( myStats->HP > myStats->MAXHP * 0.4 )
					{
						if ( specialRoll < 3 ) // 15%
						{
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_COCKATRICE_ATK;
						}
					}
					else if ( myStats->HP > myStats->MAXHP * 0.2 )
					{
						if ( specialRoll < 4 ) // 20%
						{
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_COCKATRICE_ATK;
						}
					}
					else if ( myStats->HP <= myStats->MAXHP * 0.2 )
					{
						if ( specialRoll < 5 ) // 25%
						{
							this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_COCKATRICE_ATK;
						}
					}
					break;
				case INSECTOID:
					if ( monsterSpecialState == INSECTOID_DOUBLETHROW_FIRST || monsterSpecialState == INSECTOID_DOUBLETHROW_SECOND )
					{
						this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_INSECTOID_THROW;
						break;
					}

					// spray acid
					if ( dist < STRIKERANGE * 2 )
					{
						specialRoll = rand() % 20;
						enemiesNearby = std::min(numTargetsAroundEntity(this, STRIKERANGE * 2, PI, MONSTER_TARGET_ENEMY), 4);
						//messagePlayer(0, "insectoid roll %d", specialRoll);
						if ( myStats->HP <= myStats->MAXHP * 0.8 )
						{
							bonusFromHP = 4; // +20% chance if on low health
						}
						if ( specialRoll < (enemiesNearby * 2 + bonusFromHP) ) // +10% for each enemy, capped at 40%
						{
							node = itemNodeInInventory(myStats, static_cast<ItemType>(-1), SPELLBOOK);
							if ( node != nullptr )
							{
								monsterSpecialState = INSECTOID_ACID;
								swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
								this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_INSECTOID_ACID;
								serverUpdateEntitySkill(this, 33); // for clients to handle animation
							}
							break;
						}
					}
					// throwing weapon special handled in insectoidChooseWeapon()
					break;
				case INCUBUS:
					if ( monsterSpecialState == INCUBUS_CONFUSION )
					{
						// throwing weapon special handled in incubusChooseWeapon()
						this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_INCUBUS_CONFUSION;
						break;
					}
					else if ( monsterSpecialState == INCUBUS_STEAL )
					{
						// special handled in incubusChooseWeapon()
						this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_INCUBUS_STEAL;
						break;
					}
					else if ( monsterSpecialState == INCUBUS_TELEPORT )
					{
						// special handled in incubusChooseWeapon()
						this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_INCUBUS_TELEPORT_TARGET;
						break;
					}
					break;
				case VAMPIRE:
					if ( monsterSpecialState == VAMPIRE_CAST_AURA )
					{
						// special handled in vampireChooseWeapon()
						this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_VAMPIRE_AURA;
					}
					else if ( monsterSpecialState == VAMPIRE_CAST_DRAIN )
					{
						// special handled in vampireChooseWeapon()
						this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_VAMPIRE_DRAIN;
					}
					break;
				case SHADOW:
					if ( monsterSpecialState == SHADOW_SPELLCAST )
					{
						monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_SHADOW_SPELLCAST;
					}
					else if ( monsterSpecialState == SHADOW_TELEPORT_ONLY )
					{
						// special handled in shadowChooseWeapon(), teleport code in path state.
						this->monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_SHADOW_TELEPORT;
						break;
					}
				case GOATMAN:
					if ( monsterSpecialState == GOATMAN_POTION )
					{
						monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_GOATMAN_DRINK;
					}
					else if ( monsterSpecialState == GOATMAN_THROW )
					{
						monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_GOATMAN_THROW;
					}
					break;
				default:
					break;
			}
		}
		else if ( this->monsterSpecialTimer > 0 )
		{
			bool shouldAttack = true;
			switch ( myStats->type )
			{
				case KOBOLD:
					node = itemNodeInInventory(myStats, static_cast<ItemType>(-1), WEAPON); // find weapon to re-equip
					if ( node != nullptr )
					{
						swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
					}
					else
					{
						monsterUnequipSlotFromCategory(myStats, &myStats->weapon, SPELLBOOK);	
					}
					break;
				case INSECTOID:
					if ( monsterSpecialState == INSECTOID_ACID )
					{
						monsterSpecialState = 0;
						serverUpdateEntitySkill(this, 33); // for clients to handle animation
						node = itemNodeInInventory(myStats, static_cast<ItemType>(-1), WEAPON); // find weapon to re-equip
						if ( node != nullptr )
						{
							swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						}
						else
						{
							monsterUnequipSlotFromCategory(myStats, &myStats->weapon, SPELLBOOK);
						}
						shouldAttack = false;
					}
					else if ( monsterSpecialState == INSECTOID_DOUBLETHROW_SECOND )
					{
						monsterSpecialState = 0;
						node = itemNodeInInventory(myStats, static_cast<ItemType>(-1), WEAPON); // find weapon to re-equip
						if ( node != nullptr )
						{
							swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						}
						else
						{
							monsterUnequipSlotFromCategory(myStats, &myStats->weapon, THROWN);
						}
						shouldAttack = false;
					}
					break;
				case COCKATRICE:
					monsterUnequipSlotFromCategory(myStats, &myStats->weapon, SPELLBOOK);
					break;
				case INCUBUS:
					if ( monsterSpecialState == INCUBUS_CONFUSION )
					{
						node = itemNodeInInventory(myStats, static_cast<ItemType>(-1), WEAPON); // find weapon to re-equip
						if ( node != nullptr )
						{
							swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						}
						else
						{
							monsterUnequipSlotFromCategory(myStats, &myStats->weapon, POTION);
						}
						shouldAttack = false;
						monsterSpecialState = 0;
					}
					else if ( monsterSpecialState == INCUBUS_STEAL )
					{
						node = itemNodeInInventory(myStats, static_cast<ItemType>(-1), WEAPON); // find weapon to re-equip
						if ( node != nullptr )
						{
							swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						}
						else
						{
							monsterUnequipSlotFromCategory(myStats, &myStats->weapon, SPELLBOOK);
						}
						shouldAttack = false;
						monsterSpecialState = 0;
					}
					else if ( monsterSpecialState == INCUBUS_TELEPORT_STEAL )
					{
						// this flag will be cleared in incubusChooseWeapon
					}
					else if ( monsterSpecialState == INCUBUS_TELEPORT )
					{
						// this flag will be cleared in incubusChooseWeapon
					}
					serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
					break;
				case VAMPIRE:
					if ( monsterSpecialState == VAMPIRE_CAST_AURA )
					{
						node = itemNodeInInventory(myStats, static_cast<ItemType>(-1), WEAPON); // find weapon to re-equip
						if ( node != nullptr )
						{
							swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						}
						else
						{
							monsterUnequipSlotFromCategory(myStats, &myStats->weapon, SPELLBOOK);
						}
						shouldAttack = false;
						monsterSpecialState = 0;
					}
					else if ( monsterSpecialState == VAMPIRE_CAST_DRAIN )
					{
						node = itemNodeInInventory(myStats, static_cast<ItemType>(-1), WEAPON); // find weapon to re-equip
						if ( node != nullptr )
						{
							swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						}
						else
						{
							monsterUnequipSlotFromCategory(myStats, &myStats->weapon, SPELLBOOK);
						}
						shouldAttack = false;
						monsterSpecialState = 0;
					}
					serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
					break;
				case SHADOW:
					if ( monsterSpecialState == SHADOW_SPELLCAST ) //TODO: This code is destroying spells?
					{
						//TODO: Nope, this code isn't destroying spells. Something *before* this code is.
						//messagePlayer(clientnum, "[DEBUG: handleMonsterSpecialAttack()] Resolving shadow's spellcast.");
						node = itemNodeInInventory(myStats, static_cast<ItemType>(-1), WEAPON); // find weapon to re-equip
						if ( node != nullptr )
						{
							swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						}
						else
						{
							monsterUnequipSlotFromCategory(myStats, &myStats->weapon, SPELLBOOK);
						}
						/*Item *spellbook = newItem(static_cast<ItemType>(0), static_cast<Status>(0), 0, 1, rand(), 0, &myStats->inventory);
						copyItem(spellbook, myStats->weapon);
						dropItemMonster(myStats->weapon, this, myStats, 1);*/
						shouldAttack = false;
						monsterSpecialState = 0;
						monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_SHADOW_SPELLCAST;
					}
					serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
					break;
				case GOATMAN:
					if ( monsterSpecialState == GOATMAN_POTION )
					{
						node = itemNodeInInventory(myStats, static_cast<ItemType>(-1), WEAPON); // find weapon to re-equip
						if ( node != nullptr )
						{
							swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						}
						else
						{
							monsterUnequipSlotFromCategory(myStats, &myStats->weapon, POTION);
						}
						shouldAttack = false;
						monsterSpecialState = 0;
					}
					else if ( monsterSpecialState == GOATMAN_THROW )
					{
						node = itemNodeInInventory(myStats, static_cast<ItemType>(-1), WEAPON); // find weapon to re-equip
						if ( node != nullptr )
						{
							swapMonsterWeaponWithInventoryItem(this, myStats, node, false, true);
						}
						else
						{
							monsterUnequipSlotFromCategory(myStats, &myStats->weapon, THROWN);
						}
						shouldAttack = false;
						monsterSpecialState = 0;
					}
					serverUpdateEntitySkill(this, 33); // for clients to keep track of animation
					break;
				default:
					break;
			}
			// Whether monster should attack following the unequip action.
			return shouldAttack;
		}
	}
	// monster should attack after this function is called.
	return true;
}

void getTargetsAroundEntity(Entity* my, Entity* originalTarget, double distToFind, real_t angleToSearch, int searchType, list_t** list)
{
	Entity* entity = nullptr;
	node_t* node = nullptr;
	node_t* node2 = nullptr;

	// aoe
	for ( node = map.creatures->first; node != nullptr; node = node->next ) //Only looks at monsters and players, don't iterate all entities (map.entities).
	{
		entity = (Entity*)node->element;
		if ( (entity->behavior == &actMonster || entity->behavior == &actPlayer) && entity != originalTarget && entity != my )
		{
			if ( searchType == MONSTER_TARGET_ENEMY )
			{
				if ( !my->checkEnemy(entity) )
				{
					continue;
				}
			}
			else if ( searchType == MONSTER_TARGET_FRIEND )
			{
				if ( !my->checkFriend(entity) )
				{
					continue;
				}
			}
			else if ( searchType == MONSTER_TARGET_PLAYER )
			{
				if ( !(entity->behavior == &actPlayer) )
				{
					continue;
				}
			}
			else if ( searchType == MONSTER_TARGET_ALL )
			{
			}

			double aoeTangent = atan2(entity->y - my->y, entity->x - my->x);
			real_t angle = my->yaw - aoeTangent;
			while ( angle >= PI )
			{
				angle -= PI * 2;
			}
			while ( angle < -PI )
			{
				angle += PI * 2;
			}
			if ( abs(angle) <= angleToSearch ) // searches in 2x the given angle, +/- from yaw.
			{
				double dist = sqrt(pow(my->x - entity->x, 2) + pow(my->y - entity->y, 2));
				if ( dist < distToFind )
				{
					//If this is the first entity found, the list needs to be created.
					if ( !(*list) )
					{
						*list = (list_t*)malloc(sizeof(list_t));
						(*list)->first = nullptr;
						(*list)->last = nullptr;
					}
					node2 = list_AddNodeLast(*list);
					node2->element = entity;
					node2->deconstructor = &emptyDeconstructor;
					node2->size = sizeof(Entity*);
				}
			}
		}
	}
	return;
}

int numTargetsAroundEntity(Entity* my, double distToFind, real_t angleToSearch, int searchType)
{
	list_t* aoeTargets = nullptr;
	int count = 0;
	getTargetsAroundEntity(my, nullptr, distToFind, angleToSearch, searchType, &aoeTargets);
	if ( aoeTargets )
	{
		count = list_Size(aoeTargets);
		//messagePlayer(0, "found %d targets", count);
		//Free the list.
		list_FreeAll(aoeTargets);
		free(aoeTargets);
	}
	return count;
}

bool handleMonsterChatter(int monsterclicked, bool ringconflict, char namesays[32], Entity* my, Stat* myStats)
{
	if ( ringconflict || myStats->MISC_FLAGS[STAT_FLAG_NPC] == 0 )
	{
		//Instant fail if ring of conflict is in effect/not NPC
		return false;
	}

	int NPCtype = myStats->MISC_FLAGS[STAT_FLAG_NPC] & 0xFF; // get NPC type, lowest 8 bits.
	int NPClastLine = (myStats->MISC_FLAGS[STAT_FLAG_NPC] & 0xFF00) >> 8; // get last line said, next 8 bits.

	int numLines = 0;
	int startLine = 2700 + (NPCtype - 1) * MONSTER_NPC_DIALOGUE_LINES; // lang line to start from.
	int currentLine = startLine + 1;

	bool isSequential = false;

	if ( !strcmp(language[startLine], "type:seq") )
	{
		isSequential = true;
	}

	for ( int i = 1; i < MONSTER_NPC_DIALOGUE_LINES; ++i )
	{
		// find the next 9 lines if available.
		if ( !strcmp(language[currentLine], "") )
		{
			break;
		}
		++currentLine;
		++numLines;
	}

	// choose a dialogue line.
	if ( numLines > 0 )
	{
		if ( isSequential )
		{
			// say the next line in series.
			if ( NPClastLine != 0 )
			{
				++NPClastLine;
				if ( (NPClastLine) > numLines )
				{
					// reset to beginning
					NPClastLine = 1;
				}
			}
			else
			{
				// first line being said, choose the first.
				NPClastLine = 1;
			}
		}
		else if ( !isSequential )
		{
			// choose randomly
			NPClastLine = 1 + rand() % numLines;
		}
		messagePlayer(monsterclicked, language[startLine + NPClastLine], namesays, stats[monsterclicked]->name);
		myStats->MISC_FLAGS[STAT_FLAG_NPC] = NPCtype + (NPClastLine << 8);
	}
	return true;
}

int numMonsterTypeAliveOnMap(Monster creature, Entity*& lastMonster)
{
	node_t* node = nullptr;
	Entity* entity = nullptr;
	int monsterCount = 0;
	for ( node = map.creatures->first; node != nullptr; node = node->next )
	{
		entity = (Entity*)node->element;
		if ( entity )
		{
			if ( entity->getRace() == creature )
			{
				lastMonster = entity;
				++monsterCount;
			}
		}
	}
	return monsterCount;
}

void Entity::monsterMoveBackwardsAndPath()
{
	while ( yaw < 0 )
	{
		yaw += 2 * PI;
	}
	while ( yaw >= 2 * PI )
	{
		yaw -= 2 * PI;
	}
	int x1 = ((int)floor(x)) >> 4;
	int y1 = ((int)floor(y)) >> 4;
	int u, v;
	std::vector<std::pair<int, int>> areaToTry;
	if ( yaw <= PI / 4 || yaw > 7 * PI / 4 )
	{
		areaToTry.push_back(std::pair<int, int>(x1 - 1, y1));
		areaToTry.push_back(std::pair<int, int>(x1 - 2, y1));
		areaToTry.push_back(std::pair<int, int>(x1 - 2, y1 + 1));
		areaToTry.push_back(std::pair<int, int>(x1 - 2, y1 - 1));
		areaToTry.push_back(std::pair<int, int>(x1 - 3, y1));
	}
	else if ( yaw > PI / 4 && yaw <= 3 * PI / 4 )
	{
		areaToTry.push_back(std::pair<int, int>(x1, y1 - 1));
		areaToTry.push_back(std::pair<int, int>(x1, y1 - 2));
		areaToTry.push_back(std::pair<int, int>(x1 - 1, y1 - 2));
		areaToTry.push_back(std::pair<int, int>(x1 + 1, y1 - 2));
		areaToTry.push_back(std::pair<int, int>(x1, y1 - 3));
	}
	else if ( yaw > 3 * PI / 4 && yaw <= 5 * PI / 4 )
	{
		areaToTry.push_back(std::pair<int, int>(x1 + 1, y1));
		areaToTry.push_back(std::pair<int, int>(x1 + 2, y1));
		areaToTry.push_back(std::pair<int, int>(x1 + 2, y1 + 1));
		areaToTry.push_back(std::pair<int, int>(x1 + 2, y1 - 1));
		areaToTry.push_back(std::pair<int, int>(x1 + 3, y1));
	}
	else if ( yaw > 5 * PI / 4 && yaw <= 7 * PI / 4 )
	{
		areaToTry.push_back(std::pair<int, int>(x1, y1 + 1));
		areaToTry.push_back(std::pair<int, int>(x1, y1 + 2));
		areaToTry.push_back(std::pair<int, int>(x1 - 1, y1 + 2));
		areaToTry.push_back(std::pair<int, int>(x1 + 1, y1 + 2));
		areaToTry.push_back(std::pair<int, int>(x1, y1 + 3));
	}
	bool foundplace = false;
	for ( int tries = 0; tries < areaToTry.size() && !foundplace; ++tries )
	{
		std::pair<int, int> tmpPair = areaToTry[rand() % areaToTry.size()];
		u = tmpPair.first;
		v = tmpPair.second;
		if ( !checkObstacle((u << 4) + 8, (v << 4) + 8, this, nullptr) )
		{
			x1 = u;
			y1 = v;
			foundplace = true;
		}
	}
	path = generatePath((int)floor(x / 16), (int)floor(y / 16), x1, y1, this, this);
	if ( children.first != NULL )
	{
		list_RemoveNode(children.first);
	}
	node_t* node = list_AddNodeFirst(&this->children);
	node->element = path;
	node->deconstructor = &listDeconstructor;
	monsterState = MONSTER_STATE_HUNT; // hunt state
}

bool Entity::monsterHasLeader()
{
	Stat* myStats = this->getStats();
	if ( myStats )
	{
		if ( myStats->leader_uid != 0 )
		{
			return true;
		}
	}
	return false;
}

void Entity::monsterAllySendCommand(int command, int destX, int destY, Uint32 uid)
{
	if ( multiplayer == CLIENT )
	{
		return;
	}
	if ( command == -1 || monsterAllyIndex == -1 || monsterAllyIndex > MAXPLAYERS )
	{
		return;
	}
	if ( !players[monsterAllyIndex] )
	{
		return;
	}
	if ( !players[monsterAllyIndex]->entity )
	{
		return;
	}
	if ( !isMobile() )
	{
		return;
	}

	Stat* myStats = getStats();
	if ( !myStats )
	{
		return;
	}

	switch ( command )
	{
		case ALLY_CMD_ATTACK_CONFIRM:
			if ( uid != 0 && uid != getUID() )
			{
				Entity* target = uidToEntity(uid);
				if ( target )
				{
					if ( target->behavior == &actMonster || target->behavior == &actPlayer )
					{
						if ( stats[monsterAllyIndex] ) // check owner's proficiency.
						{
							int skillLVL = stats[clientnum]->PROFICIENCIES[PRO_LEADERSHIP] + statGetCHR(stats[clientnum]);
							if ( skillLVL >= SKILL_LEVEL_MASTER )
							{
								// attack anything except if FF is off + friend.
								if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
								{
									if ( !checkFriend(target) )
									{
										monsterAcquireAttackTarget(*target, MONSTER_STATE_ATTACK);
										handleNPCInteractDialogue(*myStats, ALLY_EVENT_ATTACK);
									}
									else
									{
										// messagePlayer(0, "Friendly fire is on!");
										handleNPCInteractDialogue(*myStats, ALLY_EVENT_ATTACK_FRIENDLY_FIRE);
									}
								}
								else
								{
									monsterAcquireAttackTarget(*target, MONSTER_STATE_ATTACK);
									handleNPCInteractDialogue(*myStats, ALLY_EVENT_ATTACK);
								}
							}
						}
						monsterAllyInteractTarget = 0;
					}
					else
					{
						monsterAcquireAttackTarget(*target, MONSTER_STATE_PATH);
						monsterAllyState = ALLY_STATE_MOVETO;
						handleNPCInteractDialogue(*myStats, ALLY_EVENT_MOVETO_BEGIN);
					}
				}
			}
			break;
		case ALLY_CMD_MOVEASIDE:
			monsterMoveAside(this, players[monsterAllyIndex]->entity);
			handleNPCInteractDialogue(*myStats, ALLY_EVENT_MOVEASIDE);
			break;
		case ALLY_CMD_DEFEND:
			monsterAllyState = ALLY_STATE_DEFEND;
			createPathBoundariesNPC();
			// stop in your tracks!
			handleNPCInteractDialogue(*myStats, ALLY_EVENT_WAIT);
			monsterState = MONSTER_STATE_WAIT; // wait state
			serverUpdateEntitySkill(this, 0);
			break;
		case ALLY_CMD_MOVETO_SELECT:
			break;
		case ALLY_CMD_FOLLOW:
			monsterAllyState = ALLY_STATE_DEFAULT;
			handleNPCInteractDialogue(*myStats, ALLY_EVENT_FOLLOW);
			break;
		case ALLY_CMD_CLASS_TOGGLE:
			++monsterAllyClass;
			if ( monsterAllyClass > ALLY_CLASS_RANGED )
			{
				monsterAllyClass = ALLY_CLASS_MIXED;
			}
			myStats->allyClass = monsterAllyClass;
			serverUpdateEntitySkill(this, 46);
			break;
		case ALLY_CMD_PICKUP_TOGGLE:
			++monsterAllyPickupItems;
			if ( monsterAllyPickupItems > ALLY_PICKUP_ALL )
			{
				monsterAllyPickupItems = ALLY_PICKUP_NONPLAYER;
			}
			myStats->allyItemPickup = monsterAllyPickupItems;
			serverUpdateEntitySkill(this, 44);
			break;
		case ALLY_CMD_DROP_EQUIP:
			if ( stats[monsterAllyIndex] )
			{
				if ( stats[monsterAllyIndex] )
				{
					Entity* dropped;
					Uint32 owner = players[monsterAllyIndex]->entity->getUID();
					if ( (stats[monsterAllyIndex]->PROFICIENCIES[PRO_LEADERSHIP] + statGetCHR(stats[monsterAllyIndex])) >= SKILL_LEVEL_MASTER )
					{
						dropped = dropItemMonster(myStats->helmet, this, myStats);
						if ( dropped )
						{
							dropped->itemOriginalOwner = owner;
						}
						dropped = dropItemMonster(myStats->breastplate, this, myStats);
						if ( dropped )
						{
							dropped->itemOriginalOwner = owner;
						}
						dropped = dropItemMonster(myStats->shoes, this, myStats);
						if ( dropped )
						{
							dropped->itemOriginalOwner = owner;
						}
						dropped = dropItemMonster(myStats->shield, this, myStats);
						if ( dropped )
						{
							dropped->itemOriginalOwner = owner;
						}

						if ( (stats[monsterAllyIndex]->PROFICIENCIES[PRO_LEADERSHIP] + statGetCHR(stats[monsterAllyIndex])) >= SKILL_LEVEL_LEGENDARY )
						{
							dropped = dropItemMonster(myStats->ring, this, myStats);
							if ( dropped )
							{
								dropped->itemOriginalOwner = owner;
							}
							dropped = dropItemMonster(myStats->amulet, this, myStats);
							if ( dropped )
							{
								dropped->itemOriginalOwner = owner;
							}
							dropped = dropItemMonster(myStats->cloak, this, myStats);
							if ( dropped )
							{
								dropped->itemOriginalOwner = owner;
							}
							handleNPCInteractDialogue(*myStats, ALLY_EVENT_DROP_ALL);
						}
						else
						{
							handleNPCInteractDialogue(*myStats, ALLY_EVENT_DROP_EQUIP);
						}
					}
					else
					{
						if ( myStats->weapon )
						{
							handleNPCInteractDialogue(*myStats, ALLY_EVENT_DROP_WEAPON);
						}
					}
					dropped = dropItemMonster(myStats->weapon, this, myStats);
					if ( dropped )
					{
						dropped->itemOriginalOwner = owner;
					}
				}
			}
			break;
		case ALLY_CMD_MOVETO_CONFIRM:
		{
			if ( monsterSetPathToLocation(destX, destY, 1) )
			{
				monsterState = MONSTER_STATE_HUNT; // hunt state
				monsterAllyState = ALLY_STATE_MOVETO;
				serverUpdateEntitySkill(this, 0);
			}
			else
			{
				//messagePlayer(0, "no path to destination");
				handleNPCInteractDialogue(*myStats, ALLY_EVENT_MOVETO_FAIL);
			}
			break;
		}
		default:
			break;
	}
	serverUpdateEntitySkill(this, 43);
	//messagePlayer(0, "received: %d", command);
}

bool Entity::monsterAllySetInteract()
{
	if ( multiplayer == CLIENT )
	{
		return false;
	}
	if ( monsterAllyInteractTarget == 0 )
	{
		return false;
	}
	Entity* target = uidToEntity(monsterAllyInteractTarget);
	if ( !target )
	{
		monsterAllyState = ALLY_STATE_DEFAULT;
		monsterAllyInteractTarget = 0;
		return false;
	}
	// check distance to interactable.
	double range = pow(y - target->y, 2) + pow(x - target->x, 2);
	if ( range < 256 ) // 16 squared
	{
		followerInteractedEntity = target; // set followerInteractedEntity to the mechanism/item/gold etc.
		followerInteractedEntity->interactedByMonster = getUID(); // set the remote entity to this monster's uid to lookup later.
	}
	else
	{
		return false;
	}

	return true;
}

bool Entity::isInteractWithMonster()
{
	if ( followerInteractedEntity == nullptr )
	{
		return false; // entity is not set to interact with any monster.
	}
	if ( followerInteractedEntity->interactedByMonster == 0 )
	{
		return false; // recent monster is not set to interact.
	}
	if ( followerInteractedEntity == this )
	{
		return true; // a monster is set to interact with myself.
	}
	return false;
}

void Entity::clearMonsterInteract()
{
	followerInteractedEntity = nullptr;
	interactedByMonster = 0;
}

bool Entity::monsterSetPathToLocation(int destX, int destY, int adjacentTilesToCheck)
{
	int u, v;
	bool foundplace = false;
	int pathToX = destX;
	int pathToY = destY;

	if ( static_cast<int>(x / 16) == destX && static_cast<int>(y / 16) == destY )
	{
		foundplace = true; // we're trying to move to the spot we're already at!
	}
	else if ( !checkObstacle((destX << 4) + 8, (destY << 4) + 8, this, nullptr) )
	{
		foundplace = true; // we can path directly to the destination specified.
	}

	std::vector<std::pair<int, std::pair<int, int>>> possibleDestinations; // store distance and the x, y coordinates in each element.

	if ( !foundplace )
	{
		for ( u = destX - adjacentTilesToCheck; u <= destX + adjacentTilesToCheck; u++ )
		{
			for ( v = destY - adjacentTilesToCheck; v <= destY + adjacentTilesToCheck; v++ )
			{
				if ( static_cast<int>(x / 16) == u && static_cast<int>(y / 16) == v )
				{
					// we're trying to move to the spot we're already at!
				}
				else if ( !checkObstacle((u << 4) + 8, (v << 4) + 8, this, nullptr) )
				{
					int distance = pow(x / 16 - u, 2) + pow(y / 16 - v, 2);
					possibleDestinations.push_back(std::make_pair(distance, std::make_pair(u, v)));
				}
			}
		}
	}

	if ( !possibleDestinations.empty() )
	{
		// sort by distance from monster, first result is shortest path.
		std::sort(possibleDestinations.begin(), possibleDestinations.end());
		pathToX = possibleDestinations.at(0).second.first;
		pathToY = possibleDestinations.at(0).second.second;
		foundplace = true;
	}

	path = generatePath(static_cast<int>(floor(x / 16)), static_cast<int>(floor(y / 16)), pathToX, pathToY, this, nullptr);
	if ( children.first != NULL )
	{
		list_RemoveNode(children.first);
	}
	node_t* node = list_AddNodeFirst(&children);
	node->element = path;
	node->deconstructor = &listDeconstructor;

	if ( path == nullptr || !foundplace )
	{
		return false;
	}
	return true;
}

void Entity::handleNPCInteractDialogue(Stat& myStats, AllyNPCChatter event)
{
	if ( multiplayer == CLIENT )
	{
		return;
	}
	if ( !isMobile() )
	{
		return;
	}
	if ( monsterAllyIndex < 0 || monsterAllyIndex >= MAXPLAYERS )
	{
		return;
	}
	if ( !stats[monsterAllyIndex] )
	{
		return;
	}

	char namesays[32];
	if ( !strcmp(myStats.name, "") )
	{
		if ( myStats.type < KOBOLD ) //Original monster count
		{
			snprintf(namesays, 31, language[513], language[90 + myStats.type]); // The %s says
		}
		else if ( myStats.type >= KOBOLD ) //New monsters
		{
			snprintf(namesays, 31, language[513], language[2000 + myStats.type - KOBOLD]); // The %s says
		}
	}
	else
	{
		snprintf(namesays, 31, language[1302], myStats.name); // %s says
	}

	std::string message;

	if ( myStats.type == HUMAN )
	{
		switch ( event )
		{
			case ALLY_EVENT_MOVEASIDE:
				message = language[535];
				break;
			case ALLY_EVENT_MOVETO_BEGIN:
				if ( rand() % 3 == 0 && monsterAllyInteractTarget == 0 )
				{
					message = language[3079 + rand() % 2];
				}
				break;
			case ALLY_EVENT_MOVETO_END:
				message = language[3074 + rand() % 3];
				break;
			case ALLY_EVENT_MOVETO_FAIL:
				message = language[3077 + rand() % 2];
				break;
			case ALLY_EVENT_INTERACT_ITEM:
				break;
			case ALLY_EVENT_INTERACT_OTHER:
				break;
			case ALLY_EVENT_ATTACK:
			case ALLY_EVENT_SPOT_ENEMY:
				message = language[516 + rand() % 3];
				break;
			case ALLY_EVENT_ATTACK_FRIENDLY_FIRE:
				message = language[3084 + rand() % 2];
				break;
			case ALLY_EVENT_DROP_WEAPON:
			case ALLY_EVENT_DROP_EQUIP:
			case ALLY_EVENT_DROP_ALL:
				if ( rand() % 2 )
				{
					if ( rand() % 2 && event == ALLY_EVENT_DROP_ALL )
					{
						message = language[3083];
					}
					else
					{
						message = language[3072 + rand() % 2];
					}
				}
				else
				{
					message = language[3081 + rand() % 2];
				}
				break;
			case ALLY_EVENT_WAIT:
				message = language[3069 + rand() % 3];
				break;
			case ALLY_EVENT_FOLLOW:
				message = language[526 + rand() % 3];
				break;
			case ALLY_EVENT_MOVETO_REPATH:
				message = language[3086 + rand() % 2];
			default:
				break;
		}
	}
	char fullmsg[256] = "";
	strcpy(fullmsg, message.c_str());
	messagePlayer(monsterAllyIndex, fullmsg, namesays, stats[monsterAllyIndex]->name);
}