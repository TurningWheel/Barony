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

float limbs[NUMMONSTERS][20][3];

// determines which monsters fight which
bool swornenemies[NUMMONSTERS][NUMMONSTERS] =
{
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // NOTHING
	{ 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // HUMAN
	{ 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // RAT
	{ 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // GOBLIN
	{ 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // SLIME
	{ 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // TROLL
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // OCTOPUS
	{ 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // SPIDER
	{ 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // GHOUL
	{ 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // SKELETON
	{ 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // SCORPION
	{ 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // IMP
	{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // BUGBEAR
	{ 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // GNOME
	{ 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // DEMON
	{ 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // SUCCUBUS
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // MIMIC
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // LICH
	{ 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // MINOTAUR
	{ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // DEVIL
	{ 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // SHOPKEEPER
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // KOBOLD
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // SCARAB
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // CRYSTALGOLEM
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // INCUBUS
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // VAMPIRE
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // SHADOW
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // COCKATRICE
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // INSECTOID
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // GOATMAN
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // AUTOMATON
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // LICH_ICE
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }  // LICH_FIRE
};

// determines which monsters come to the aid of other monsters
bool monsterally[NUMMONSTERS][NUMMONSTERS] =
{
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // NOTHING
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // HUMAN
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // RAT
	{ 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // GOBLIN
	{ 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // SLIME
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // TROLL
	{ 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // OCTOPUS
	{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // SPIDER
	{ 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // GHOUL
	{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // SKELETON
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // SCORPION
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // IMP
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // BUGBEAR
	{ 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // GNOME
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // DEMON
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // SUCCUBUS
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // MIMIC
	{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // LICH
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // MINOTAUR
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // DEVIL
	{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // SHOPKEEPER
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // KOBOLD
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // SCARAB
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0 }, // CRYSTALGOLEM
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 }, // INCUBUS
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1 }, // VAMPIRE
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1 }, // SHADOW
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 }, // COCKATRICE
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 }, // INSECTOID
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0 }, // GOATMAN
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0 }, // AUTOMATON
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
	128,  // KOBOLD
	512,  // SCARAB
	32,   // CRYSTALGOLEM
	256,  // INCUBUS
	192,  // VAMPIRE
	256,  // SHADOW
	256,  // COCKATRICE
	256,  // INSECTOID
	256,  // GOATMAN
	192,  // AUTOMATON
	512,  // LICH_ICE
	512,  // LICH_FIRE
};

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
	Entity* entity = newEntity(-1, 1, map.entities);
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

	Stat* myStats = NULL;
	if ( multiplayer != CLIENT )
	{
		// Need to give the entity its list stuff.
		// create an empty first node for traversal purposes
		node_t* node = NULL;
		node = list_AddNodeFirst(&entity->children);
		node->element = NULL;
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
				entity = NULL;
				break;
			}
			while (1);
		}
	}

	if ( entity )
	{
		switch (creature)
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
				entity->z = -2;
				entity->yaw = PI;
				entity->sprite = 274;
				entity->skill[29] = 120;
				break;
			case LICH_FIRE:
				entity->focalx = limbs[LICH_FIRE][0][0]; // -0.75
				entity->focaly = limbs[LICH_FIRE][0][1]; // 0
				entity->focalz = limbs[LICH_FIRE][0][2]; // 0
				entity->z = -2;
				entity->yaw = PI;
				entity->sprite = 274;
				entity->skill[29] = 120;
				break;
			default:
				//Spawn a potato.
				list_RemoveNode(entity->mynode);
				return NULL;
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
	return NULL;
}

/*-------------------------------------------------------------------------------

	monsterMoveAside

	Causes the monster given in *entity to move aside from the entity given
	in *my

-------------------------------------------------------------------------------*/

bool monsterMoveAside(Entity* my, Entity* entity)
{
	if ( MONSTER_STATE != 0 )
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
		MONSTER_STATE = 2;
		MONSTER_TARGET = 0;
		MONSTER_TARGETX = my->x + x;
		MONSTER_TARGETY = my->y + y;

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

			// move aside
			monsterMoveAside(my, players[monsterclicked]->entity);
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
		if ( race == HUMAN || race == GOBLIN )
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
		if ( myStats->type < KOBOLD ) //Original monster count
		{
			messagePlayer(monsterclicked, language[529], language[90 + myStats->type]);
		}
		else if ( myStats->type >= KOBOLD ) //New monsters
		{
			messagePlayer(monsterclicked, language[529], language[2000 + (myStats->type - KOBOLD)]);
		}
	}

	monsterMoveAside(my, players[monsterclicked]->entity);
	players[monsterclicked]->entity->increaseSkill(PRO_LEADERSHIP);
	MONSTER_STATE = 0; // be ready to follow
	myStats->leader_uid = players[monsterclicked]->entity->getUID();

	if ( monsterclicked > 0 && multiplayer == SERVER )
	{
		//Tell the client he suckered somebody into his cult.
		strcpy((char*) (net_packet->data), "LEAD");
		SDLNet_Write32((Uint32 )my->getUID(), &net_packet->data[4]);
		net_packet->address.host = net_clients[monsterclicked - 1].host;
		net_packet->address.port = net_clients[monsterclicked - 1].port;
		net_packet->len = 8;
		sendPacketSafe(net_sock, -1, net_packet, monsterclicked - 1);
	}
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
			node->element = NULL;
			node->deconstructor = &emptyDeconstructor;
			node->size = 0;
			node = list_AddNodeLast(&my->children);
			node->element = NULL;
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
				initHuman(my, NULL);
			}
			else if ( my->sprite == 131 || my->sprite == 265 )     // rat
			{
				initRat(my, NULL);
			}
			else if ( my->sprite == 180 )     // goblin head
			{
				initGoblin(my, NULL);
			}
			else if ( my->sprite == 196 || my->sprite == 266 )     // scorpion body
			{
				initScorpion(my, NULL);
			}
			else if ( my->sprite == 190 )     // succubus head
			{
				initSuccubus(my, NULL);
			}
			else if ( my->sprite == 204 )     // troll head
			{
				initTroll(my, NULL);
			}
			else if ( my->sprite == 217 )     // shopkeeper head
			{
				initShopkeeper(my, NULL);
			}
			else if ( my->sprite == 229 )     // skeleton head
			{
				initSkeleton(my, NULL);
			}
			else if ( my->sprite == 239 )     // minotaur waist
			{
				initMinotaur(my, NULL);
			}
			else if ( my->sprite == 246 )     // ghoul head
			{
				initGhoul(my, NULL);
			}
			else if ( my->sprite == 258 )     // demon head
			{
				initDemon(my, NULL);
			}
			else if ( my->sprite == 267 )     // spider body
			{
				initSpider(my, NULL);
			}
			else if ( my->sprite == 274 )     // lich body
			{
				initLich(my, NULL);
			}
			else if ( my->sprite == 289 )     // imp head
			{
				initImp(my, NULL);
			}
			else if ( my->sprite == 295 )     // gnome head
			{
				initGnome(my, NULL);
			}
			else if ( my->sprite == 304 )     // devil torso
			{
				initDevil(my, NULL);
			}
			else if ( my->sprite == 475 )     // crystal golem head
			{
				initCrystalgolem(my, NULL);
			}
			else if ( my->sprite == 413 )     // cockatrice head
			{
				initCockatrice(my, NULL);
			}
			else if ( my->sprite == 467 )     // automaton torso
			{
				initAutomaton(my, NULL);
			}
			else if ( my->sprite == 429 || my->sprite == 430 )     // scarab
			{
				initScarab(my, NULL);
			}
			else if ( my->sprite == 421 )     // kobold head
			{
				initKobold(my, NULL);
			}
			else if ( my->sprite == 481 )     // shadow head
			{
				initShadow(my, NULL);
			}
			else if ( my->sprite == 437 )     // vampire head
			{
				initVampire(my, NULL);
			}
			else if ( my->sprite == 445 )     // incubus head
			{
				initIncubus(my, NULL);
			}
			else if ( my->sprite == 455 )     // insectoid head
			{
				initInsectoid(my, NULL);
			}
			else if ( my->sprite == 463 )     // goatman head
			{
				initGoatman(my, NULL);
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
			else
			{
				my->flags[BURNABLE] = false;
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
					initKobold ( my, myStats );
					break;
				case SCARAB:
					initScarab ( my, myStats );
					break;
				case CRYSTALGOLEM:
					initCrystalgolem ( my, myStats );
					break;
				case INCUBUS:
					initIncubus ( my, myStats );
					break;
				case VAMPIRE:
					initVampire ( my, myStats );
					break;
				case SHADOW:
					initShadow ( my, myStats );
					break;
				case COCKATRICE:
					initCockatrice (my, myStats);
					break;
				case INSECTOID:
					initInsectoid ( my, myStats );
					break;
				case GOATMAN:
					initGoatman ( my, myStats );
					break;
				case AUTOMATON:
					initAutomaton (my, myStats);
					break;
				case LICH_ICE:
					//initLichIce ( my, myStats );
					break;
				case LICH_FIRE:
					//initLichFire ( my, myStats );
					break;
				default:
					break; //This should never be reached.
			}
		}

		MONSTER_INIT = 2;
		if ( myStats->type != LICH && myStats->type != DEVIL )
		{
			MONSTER_LOOKDIR = (rand() % 360) * PI / 180;
		}
		else
		{
			MONSTER_LOOKDIR = PI;
		}
		MONSTER_LOOKTIME = rand() % 120;
		MONSTER_MOVETIME = rand() % 10;
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
		MONSTER_TARGET = 0;

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

	// levitation
	bool levitating = isLevitating(myStats);

	if ( myStats->type == MINOTAUR )
	{
		int c;
		for ( c = 0; c < MAXPLAYERS; c++ )
		{
			assailant[c] = true; // as long as this is active, combat music doesn't turn off
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
			for ( node = map.entities->first; node != NULL; node = nextnode )
			{
				nextnode = node->next;
				Entity* tempEntity = (Entity*)node->element;

				if ( tempEntity->behavior == &actTorch || tempEntity->behavior == &actCampfire )
				{
					foundlights = true;
					if ( tempEntity->light )
					{
						list_RemoveNode(tempEntity->light->node);
						tempEntity->light = NULL;
					}
					list_RemoveNode(tempEntity->mynode);
				}
			}
			if ( foundlights )
			{
#ifdef HAVE_FMOD
				if ( MONSTER_SOUND )
				{
					FMOD_Channel_Stop(MONSTER_SOUND);
				}
#elif defined HAVE_OPENAL
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
		if ( ( ( rand() % 4 == 0 && MONSTER_STATE != 6 ) || ( rand() % 10 == 0 && MONSTER_STATE == 6 ) ) && myStats->OLDHP != myStats->HP )
		{
			playSoundEntity(my, 180, 128);
			MONSTER_STATE = 5; // dodge state
			double dir = my->yaw - (PI / 2) + PI * (rand() % 2);
			MONSTER_VELX = cos(dir) * 5;
			MONSTER_VELY = sin(dir) * 5;
			MONSTER_SPECIAL = 0;
		}
	}

	// hunger, regaining hp/mp, poison, etc.
	if ( !intro )
	{
		my->handleEffects(myStats);
	}
	if ( myStats->HP <= 0 && MONSTER_STATE != 7 && MONSTER_STATE != 8 )
	{
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
					break;
				}
			}
		}
		if ( playerFollower < MAXPLAYERS )
		{
			for ( c = 0; c < MAXPLAYERS; c++ )
			{
				if ( client_disconnected[c] )
				{
					continue;
				}
				char whatever[256];
				if ( strcmp(myStats->name, "") )
				{
					snprintf(whatever, 255, "%s %s", myStats->name, myStats->obituary);
				}
				else
				{
					if ( myStats->type < KOBOLD ) //Original monster count
					{
						snprintf(whatever, 255, language[1499], stats[c]->name, language[90 + myStats->type], myStats->obituary);
					}
					else if ( myStats->type >= KOBOLD ) //New monsters
					{
						snprintf(whatever, 255, language[1499], stats[c]->name, language[2000 + (myStats->type - KOBOLD)], myStats->obituary);
					}
				}
				messagePlayer(c, whatever);
			}
		}

		// drop gold
		if ( myStats->GOLD > 0 )
		{
			int x = std::min<int>(std::max(0, (int)(my->x / 16)), map.width - 1);
			int y = std::min<int>(std::max(0, (int)(my->y / 16)), map.height - 1);

			// check for floor to drop gold...
			if ( map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height] )
			{
				entity = newEntity(130, 0, map.entities); // 130 = goldbag model
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
#ifdef HAVE_FMOD
		if ( MONSTER_SOUND )
		{
			FMOD_Channel_Stop(MONSTER_SOUND);
		}
#elif defined HAVE_OPENAL
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
				MONSTER_STATE = 7; // lich death state
				MONSTER_SPECIAL = 0;
				MONSTER_ATTACK = 0;
				MONSTER_ATTACKTIME = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				break;
			case CREATURE_IMP:
				impDie(my);
				break;
			case GNOME:
				gnomeDie(my);
				break;
			case DEVIL:
				my->flags[PASSABLE] = true; // so I can't take any more hits
				MONSTER_STATE = 8; // devil death state
				MONSTER_SPECIAL = 0;
				MONSTER_ATTACK = 0;
				MONSTER_ATTACKTIME = 0;
				MONSTER_ARMBENDED = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				serverUpdateEntitySkill(my, 10);
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
			default:
				break; //This should never be reached.
		}
		return;
	}

	if ( multiplayer != CLIENT )
	{
		my->effectTimes();
	}

	//Calls the function for a monster to pick up an item, if it's a monster that picks up items.
	if ( !strcmp(myStats->name, "") )
	{
		// only for monsters that have no name
		switch ( myStats->type )
		{
			case GOBLIN:
				//messagePlayer(0, "BLARG.");
				my->checkBetterEquipment(myStats);
				break;
			case HUMAN:
				my->checkBetterEquipment(myStats);
				break;
			default:
				break;
		}
	}

	// check to see if monster can scream again
	if ( MONSTER_SOUND != NULL )
	{
#ifdef HAVE_FMOD
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
#elif defined HAVE_OPENAL
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
	if ( myStats->weapon != NULL )
	{
		if ( myStats->weapon->type == SLING )
		{
			hasrangedweapon = true;
		}
		else if ( myStats->weapon->type == SHORTBOW )
		{
			hasrangedweapon = true;
		}
		else if ( myStats->weapon->type == CROSSBOW )
		{
			hasrangedweapon = true;
		}
		else if ( myStats->weapon->type == ARTIFACT_BOW )
		{
			hasrangedweapon = true;
		}
		else if ( itemCategory(myStats->weapon) == MAGICSTAFF )
		{
			hasrangedweapon = true;
		}
		else if ( itemCategory(myStats->weapon) == SPELLBOOK )
		{
			hasrangedweapon = true;
		}
	}

	// effect of a ring of conflict
	bool ringconflict = false;
	for ( node = map.entities->first; node != NULL; node = node->next )
	{
		Entity* tempentity = (Entity*)node->element;
		if ( tempentity != NULL && tempentity != my )
		{
			Stat* tempstats = tempentity->getStats();
			if ( tempstats != NULL )
			{
				if ( tempstats->ring != NULL )
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

	// invisibility
	bool handleinvisible = true;
	switch ( myStats->type )
	{
		case HUMAN:
		case GOBLIN:
		case SKELETON:
		case GNOME:
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
			if ( !strcmp(myStats->name, "") )
			{
				if ( myStats->type < KOBOLD ) //Original monster count
				{
					messagePlayer(monsterclicked, language[514], language[90 + myStats->type]);
				}
				else if ( myStats->type >= KOBOLD ) //New monsters
				{
					messagePlayer(monsterclicked, language[514], language[2000 + (myStats->type - KOBOLD)]);
				}
			}
			else
			{
				messagePlayer(monsterclicked, language[515], myStats->name);
			}
		}
		else
		{
			if (MONSTER_TARGET == players[monsterclicked]->entity->getUID() && MONSTER_STATE != 4)
			{
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
			else if (MONSTER_STATE == 4)
			{
				if (MONSTER_TARGET != players[monsterclicked]->entity->getUID())
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
				if (myStats->type != SHOPKEEPER)
				{
					makeFollower(monsterclicked, ringconflict, namesays, my, myStats);
				}
				else
				{
					if (!swornenemies[SHOPKEEPER][HUMAN])
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
		for ( node2 = map.entities->first; node2 != NULL; node2 = node2->next )
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
				}
				entity->flags[PASSABLE] = true;
				clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
				entity->flags[PASSABLE] = false;
			}
		}

		// state machine
		if ( MONSTER_STATE == 0 )   // wait state
		{
			MONSTER_TARGET = -1;
			MONSTER_VELX = 0;
			MONSTER_VELY = 0;
			if ( myReflex )
			{
				for ( node2 = map.entities->first; node2 != NULL; node2 = node2->next )
				{
					entity = (Entity*)node2->element;
					if ( entity == my || entity->flags[PASSABLE] )
					{
						continue;
					}
					hitstats = entity->getStats();
					if ( hitstats != NULL )
					{
						if ( (my->checkEnemy(entity) || MONSTER_TARGET == entity->getUID() || ringconflict) )
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
							int light = entity->entityLight();
							if ( !entity->isInvisible() )
							{
								if ( entity->behavior == &actPlayer && entity->skill[2] == 0 )
								{
									if ( stats[0]->shield )
									{
										if ( itemCategory(stats[0]->shield) == ARMOR )
										{
											light -= 95;
										}
									}
									else
									{
										light -= 95;
									}
								}
								light -= hitstats->PROFICIENCIES[PRO_STEALTH] * 2 - my->getPER() * 5;
							}
							else
							{
								light = TOUCHRANGE;
							}
							if ( (myStats->type >= LICH && myStats->type < KOBOLD) || myStats->type == LICH_FIRE || myStats->type == LICH_ICE )
							{
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
								if ( (myStats->type >= LICH && myStats->type < KOBOLD) || myStats->type == LICH_FIRE || myStats->type == LICH_ICE )
								{
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
									MONSTER_STATE = 1; // charge state
									MONSTER_TARGET = hit.entity->getUID();
									MONSTER_TARGETX = hit.entity->x;
									MONSTER_TARGETY = hit.entity->y;
									if ( MONSTER_SOUND == NULL )
									{
										if ( myStats->type != MINOTAUR )
										{
											MONSTER_SOUND = playSoundEntity(my, MONSTER_SPOTSND + rand() % MONSTER_SPOTVAR, 128);
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

									if ( entity != NULL )
										if ( entity->behavior == &actPlayer )
										{
											assailant[entity->skill[2]] = true;  // as long as this is active, combat music doesn't turn off
										}

									// alert other monsters of this enemy's presence
									for ( node = map.entities->first; node != NULL; node = node->next )
									{
										entity = (Entity*)node->element;
										if ( entity->behavior == &actMonster )
										{
											hitstats = entity->getStats();
											if ( hitstats != NULL )
											{
												if ( hitstats->type == myStats->type )
												{
													if ( entity->skill[0] == 0 )   // monster is waiting
													{
														tangent = atan2( entity->y - my->y, entity->x - my->x );
														lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, false);
														if ( hit.entity == entity )
														{
															entity->skill[0] = 2; // path state
															entity->skill[1] = MONSTER_TARGET;
															entity->fskill[2] = MONSTER_TARGETX;
															entity->fskill[3] = MONSTER_TARGETY;
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
				if (myStats->type == MINOTAUR || myStats->type == LICH || myStats->type == LICH_FIRE || myStats->type == LICH_ICE || (myStats->type == CREATURE_IMP && strstr(map.name, "Boss")))
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
						MONSTER_STATE = 2; // path state
						MONSTER_TARGET = players[playerToChase]->entity->getUID();
						MONSTER_TARGETX = players[playerToChase]->entity->x;
						MONSTER_TARGETY = players[playerToChase]->entity->y;
					}
					return;
				}
			}

			// follow the leader :)
			if ( myStats->leader_uid != 0 && my->getUID() % TICKS_PER_SECOND == ticks % TICKS_PER_SECOND )
			{
				Entity* leader = uidToEntity(myStats->leader_uid);
				if ( leader )
				{
					double dist = sqrt(pow(my->x - leader->x, 2) + pow(my->y - leader->y, 2));
					if ( dist > WAIT_FOLLOWDIST )
					{
						MONSTER_TARGET = 0;
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
						MONSTER_STATE = 3; // hunt state
						return;
					}
					else
					{
						tangent = atan2( leader->y - my->y, leader->x - my->x );
						lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, true);
						if ( hit.entity != leader )
						{
							MONSTER_TARGET = 0;
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
							MONSTER_STATE = 3; // hunt state
							return;
						}
					}
				}
			}

			// look
			MONSTER_LOOKTIME++;
			if ( MONSTER_LOOKTIME >= 120 && myStats->type != LICH && myStats->type != DEVIL )
			{
				MONSTER_LOOKTIME = 0;
				MONSTER_MOVETIME--;
				if ( myStats->type != GHOUL && myStats->type != SPIDER )
				{
					MONSTER_LOOKDIR = (rand() % 360) * PI / 180;
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
			if ( MONSTER_MOVETIME == 0 && uidToEntity(myStats->leader_uid) == NULL )
			{
				MONSTER_MOVETIME = rand() % 30;
				int goodspots = 0;
				if ( myStats->type != SHOPKEEPER )
				{
					for ( x = 0; x < map.width; x++ )
					{
						for ( y = 0; y < map.height; y++ )
						{
							if ( !checkObstacle(x << 4, y << 4, my, NULL) )
							{
								goodspots++;
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
							if ( x << 4 >= MONSTER_SHOPXS && x << 4 <= MONSTER_SHOPXE && y << 4 >= MONSTER_SHOPYS && y << 4 <= MONSTER_SHOPYE )
								if ( !checkObstacle(x << 4, y << 4, my, NULL) )
								{
									goodspots++;
								}
						}
					}
				}
				if ( goodspots )
				{
					int chosenspot = rand() % goodspots;
					int currentspot = 0;
					bool foundit = false;
					x = 0;
					y = 0;
					if ( myStats->type != SHOPKEEPER )
					{
						for ( x = 0; x < map.width; x++ )
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
						}
					}
					else
					{
						for ( x = 0; x < map.width; x++ )
						{
							for ( y = 0; y < map.height; y++ )
							{
								if ( x << 4 >= MONSTER_SHOPXS && x << 4 <= MONSTER_SHOPXE && y << 4 >= MONSTER_SHOPYS && y << 4 <= MONSTER_SHOPYE )
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
							}
							if ( foundit )
							{
								break;
							}
						}
					}
					path = generatePath( (int)floor(my->x / 16), (int)floor(my->y / 16), x, y, my, NULL );
					if ( my->children.first != NULL )
					{
						list_RemoveNode(my->children.first);
					}
					node = list_AddNodeFirst(&my->children);
					node->element = path;
					node->deconstructor = &listDeconstructor;
					MONSTER_STATE = 3; // hunt state
				}
			}

			// rotate monster
			dir = my->yaw - MONSTER_LOOKDIR;
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
		else if ( MONSTER_STATE == 1 )     // charge state
		{
			if ( uidToEntity(MONSTER_TARGET) == NULL )
			{
				MONSTER_STATE = 0;
				return;
			}
			entity = uidToEntity(MONSTER_TARGET);
			if ( entity != NULL )
				if ( entity->behavior == &actPlayer )
				{
					assailant[entity->skill[2]] = true;  // as long as this is active, combat music doesn't turn off
				}
			MONSTER_TARGETX = entity->x;
			MONSTER_TARGETY = entity->y;
			hitstats = entity->getStats();

			if (myStats->type == SHOPKEEPER)
			{
				// shopkeepers hold a grudge against players
				for (c = 0; c < MAXPLAYERS; c++)
				{
					if (players[c] && players[c]->entity)
					{
						if (MONSTER_TARGET == players[c]->entity->getUID())
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
				int light = entity->entityLight();
				if ( !entity->isInvisible() )
				{
					if ( entity->behavior == &actPlayer && entity->skill[2] == 0 )
					{
						if ( stats[0]->shield )
						{
							if ( itemCategory(stats[0]->shield) == ARMOR )
							{
								light -= 95;
							}
						}
						else
						{
							light -= 95;
						}
					}
					light -= hitstats->PROFICIENCIES[PRO_STEALTH] * 2 - my->getPER() * 5;
				}
				else
				{
					light = TOUCHRANGE;
				}
				if ( (myStats->type >= LICH && myStats->type < KOBOLD) || myStats->type == LICH_FIRE || myStats->type == LICH_ICE )
				{
					light = 1000;
				}
				double targetdist = sqrt( pow(my->x - entity->x, 2) + pow(my->y - entity->y, 2) );
				if ( targetdist > sightranges[myStats->type] )
				{
					if ( myStats->HP <= myStats->MAXHP / 3 && my->getCHR() >= -2 )
					{
						MONSTER_MOVETIME = 0;
						MONSTER_STATE = 0; // wait state
					}
					else
					{
						MONSTER_STATE = 2; // path state
					}
				}
				else
				{
					if ( targetdist > TOUCHRANGE && targetdist > light && myReflex )
					{
						tangent = atan2( MONSTER_TARGETY - my->y, MONSTER_TARGETX - my->x );
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
						if ( myStats->HP <= myStats->MAXHP / 3 && my->getCHR() >= -2 )
						{
							MONSTER_MOVETIME = 0;
							MONSTER_STATE = 0; // wait state
						}
						else
						{
							MONSTER_STATE = 2; // path state
						}
					}
					else
					{
						if ( myReflex )
						{
							tangent = atan2( MONSTER_TARGETY - my->y, MONSTER_TARGETX - my->x );
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
							if ( myStats->HP <= myStats->MAXHP / 3 && my->getCHR() >= -2 )
							{
								MONSTER_MOVETIME = 0;
								MONSTER_STATE = 0; // wait state
							}
							else
							{
								MONSTER_STATE = 2; // path state
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
							if ( targetdist > TOUCHRANGE * 1.5 && !hasrangedweapon && (myStats->HP > myStats->MAXHP / 3 || my->getCHR() < -1) && my->getINT() > -2 )
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
							if ( (dist > 16 && !hasrangedweapon && (myStats->HP > myStats->MAXHP / 3 || my->getCHR() < -1)) || (dist > 160 && hasrangedweapon) )
							{
								if ( myStats->HP > myStats->MAXHP / 3 || my->getCHR() < -1 )
								{
									dist2 = clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
								}
								else
								{
									MONSTER_VELX *= -.5;
									MONSTER_VELY *= -.5;
									dist2 = clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
								}
								if ( hit.entity != NULL )
								{
									if ( hit.entity->behavior == &actDoor )
									{
										// opens the door if unlocked and monster can do it
										if ( !hit.entity->skill[5] && my->getINT() > -2 )
										{
											if ( !hit.entity->skill[0] && !hit.entity->skill[3] )
											{
												hit.entity->skill[3] = 1 + (my->x > hit.entity->x);
												playSoundEntity(hit.entity, 21, 96);
											}
											else if ( hit.entity->skill[0] && !hit.entity->skill[3] )
											{
												hit.entity->skill[3] = 1 + (my->y < hit.entity->y);
												playSoundEntity(hit.entity, 21, 96);
											}
										}
										else
										{
											// can't open door, so break it down
											MONSTER_HITTIME++;
											if ( MONSTER_HITTIME >= HITRATE )
											{
												MONSTER_ATTACK = (rand() % 3) + 1; // random attack motion
												MONSTER_HITTIME = 0;
												hit.entity->skill[4]--; // decrease door health
												if ( myStats->type == MINOTAUR )
												{
													hit.entity->skill[4] = 0;    // minotaurs smash doors instantly
												}
												playSoundEntity(hit.entity, 28, 64);
												if ( hit.entity->skill[4] <= 0 )
												{
													// set direction of splinters
													if ( !hit.entity->skill[0] )
													{
														hit.entity->skill[6] = (my->x > hit.entity->x);
													}
													else
													{
														hit.entity->skill[6] = (my->y < hit.entity->y);
													}
												}
											}
										}
									}
									else
									{
										if ( myStats->HP <= myStats->MAXHP / 3 && my->getCHR() >= -2 )
										{
											MONSTER_MOVETIME = 0;
											MONSTER_STATE = 0; // wait state
										}
										else
										{
											MONSTER_STATE = 2; // path state
										}
									}
								}
								else
								{
									if ( myStats->HP <= myStats->MAXHP / 3 && my->getCHR() >= -2 )
									{
										MONSTER_MOVETIME = 0;
										MONSTER_STATE = 0; // wait state
									}
									else if ( dist2 <= 0.1 && myStats->HP > myStats->MAXHP / 3 )
									{
										MONSTER_STATE = 2; // path state
									}
								}
							}
							else
							{
								if ( (hasrangedweapon && dist < 100) || (myStats->HP <= myStats->MAXHP / 3 && my->getCHR() >= -2) )
								{
									// injured monsters or monsters with ranged weapons back up
									MONSTER_VELX = cos(tangent2) * .045 * (my->getDEX() + 10) * weightratio * -.5;
									MONSTER_VELY = sin(tangent2) * .045 * (my->getDEX() + 10) * weightratio * -.5;
									dist2 = clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
								}
								else
								{
									// this is just so that the monster rotates. it doesn't actually move
									MONSTER_VELX = cos(tangent) * .02 * .045 * (my->getDEX() + 10) * weightratio;
									MONSTER_VELY = sin(tangent) * .02 * .045 * (my->getDEX() + 10) * weightratio;
								}
							}

							handleMonsterAttack(my, myStats, entity, dist, hasrangedweapon);

							// bust ceilings
							/*if( myStats->type == MINOTAUR ) {
								if( my->x>=0 && my->y>=0 && my->x<map.width<<4 && my->y<map.height<<4 ) {
									if( map.tiles[MAPLAYERS+(int)(my->y/16)*MAPLAYERS+(int)(my->x/16)*MAPLAYERS*map.height] )
										map.tiles[MAPLAYERS+(int)(my->y/16)*MAPLAYERS+(int)(my->x/16)*MAPLAYERS*map.height] = 0;
								}
							}*/

							// rotate monster
							if ( (hasrangedweapon && dist < 100) || (myStats->HP <= myStats->MAXHP / 3 && my->getCHR() >= -2) )
							{
								dir = my->yaw - atan2( -MONSTER_VELY, -MONSTER_VELX );
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
					MONSTER_SPECIAL++;
					if ( MONSTER_SPECIAL > 60 )
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
								MONSTER_STATE = 9; // devil teleport state
							}
						}
						else
						{
							if ( !devilacted )
							{
								switch ( devilstate )
								{
									case 72:
										MONSTER_STATE = 11; // devil summoning state
										break;
									case 73:
										MONSTER_ATTACK = 5 + rand() % 2; // fireballs
										break;
									case 74:
										MONSTER_STATE = 12; // devil boulder drop
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
									MONSTER_STATE = 9; // devil teleport state
								}
							}
						}
						MONSTER_SPECIAL = 0;
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
		}
		else if ( MONSTER_STATE == 2 )     // path state
		{
			if ( myStats->type == DEVIL )
			{
				MONSTER_STATE = 1;
				return;
			}
			if ( uidToEntity(MONSTER_TARGET) == NULL && MONSTER_TARGET != 0 )
			{
				MONSTER_TARGET = 0;
				MONSTER_STATE = 0; // wait state
				return;
			}
			entity = uidToEntity(MONSTER_TARGET);
			if ( entity != NULL )
				if ( entity->behavior == &actPlayer )
				{
					assailant[entity->skill[2]] = true;  // as long as this is active, combat music doesn't turn off
				}
			x = ((int)floor(MONSTER_TARGETX)) >> 4;
			y = ((int)floor(MONSTER_TARGETY)) >> 4;
			path = generatePath( (int)floor(my->x / 16), (int)floor(my->y / 16), x, y, my, uidToEntity(MONSTER_TARGET) );
			if ( my->children.first != NULL )
			{
				list_RemoveNode(my->children.first);
			}
			node = list_AddNodeFirst(&my->children);
			node->element = path;
			node->deconstructor = &listDeconstructor;
			MONSTER_STATE = 3; // hunt state
		}
		else if ( MONSTER_STATE == 3 )     // hunt state
		{
			if ( myReflex && (myStats->type != LICH || MONSTER_SPECIAL <= 0) )
			{
				for ( node2 = map.entities->first; node2 != NULL; node2 = node2->next )
				{
					entity = (Entity*)node2->element;
					if ( entity == my || entity->flags[PASSABLE] )
					{
						continue;
					}
					hitstats = entity->getStats();
					if ( hitstats != NULL )
					{
						if ( (my->checkEnemy(entity) || MONSTER_TARGET == entity->getUID() || ringconflict) )
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
							int light = entity->entityLight();
							if ( !entity->isInvisible() )
							{
								if ( entity->behavior == &actPlayer && entity->skill[2] == 0 )
								{
									if ( stats[0]->shield )
									{
										if ( itemCategory(stats[0]->shield) == ARMOR )
										{
											light -= 95;
										}
									}
									else
									{
										light -= 95;
									}
								}
								light -= hitstats->PROFICIENCIES[PRO_STEALTH] * 2 - my->getPER() * 5;
							}
							else
							{
								light = TOUCHRANGE;
							}
							if ( (myStats->type >= LICH && myStats->type < KOBOLD) || myStats->type == LICH_FIRE || myStats->type == LICH_ICE )
							{
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
												MONSTER_TARGET = hit.entity->getUID();
												MONSTER_STATE = 1; // charge state
												MONSTER_TARGETX = hit.entity->x;
												MONSTER_TARGETY = hit.entity->y;
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

												if ( entity != NULL )
													if ( entity->behavior == &actPlayer )
													{
														assailant[entity->skill[2]] = true;  // as long as this is active, combat music doesn't turn off
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
			if (myStats->type == MINOTAUR || (myStats->type == LICH && MONSTER_SPECIAL <= 0) || (myStats->type == CREATURE_IMP && strstr(map.name, "Boss")))
			{
				bool shouldHuntPlayer = false;
				Entity* playerOrNot = uidToEntity(MONSTER_TARGET);
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
						MONSTER_STATE = 2; // path state
						MONSTER_TARGET = players[playerToChase]->entity->getUID();
						MONSTER_TARGETX = players[playerToChase]->entity->x;
						MONSTER_TARGETY = players[playerToChase]->entity->y;
					}
					return;
				}
			}

			// lich cooldown
			if ( myStats->type == LICH )
			{
				if ( MONSTER_SPECIAL > 0 )
				{
					MONSTER_SPECIAL--;
				}
			}

			// follow the leader :)
			if ( myStats->leader_uid != 0 && my->getUID() % TICKS_PER_SECOND == ticks % TICKS_PER_SECOND )
			{
				Entity* leader = uidToEntity(myStats->leader_uid);
				if ( leader )
				{
					double dist = sqrt(pow(my->x - leader->x, 2) + pow(my->y - leader->y, 2));
					if ( dist > HUNT_FOLLOWDIST && !MONSTER_TARGET )
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
						MONSTER_STATE = 3; // hunt state
						return;
					}
					else
					{
						double tangent = atan2( leader->y - my->y, leader->x - my->x );
						Entity* ohitentity = hit.entity;
						lineTrace(my, my->x, my->y, tangent, sightranges[myStats->type], 0, true);
						if ( hit.entity != leader )
						{
							MONSTER_TARGET = 0;
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
							MONSTER_STATE = 3; // hunt state
							return;
						}
						hit.entity = ohitentity;
					}
				}
			}

			entity = uidToEntity(MONSTER_TARGET);
			if ( entity != NULL )
				if ( entity->behavior == &actPlayer )
				{
					assailant[entity->skill[2]] = true;  // as long as this is active, combat music doesn't turn off
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
									if ( !hit.entity->skill[5] && my->getINT() > -2 )
									{
										if ( !hit.entity->skill[0] && !hit.entity->skill[3] )
										{
											hit.entity->skill[3] = 1 + (my->x > hit.entity->x);
											playSoundEntity(hit.entity, 21, 96);
										}
										else if ( hit.entity->skill[0] && !hit.entity->skill[3] )
										{
											hit.entity->skill[3] = 1 + (my->y < hit.entity->y);
											playSoundEntity(hit.entity, 21, 96);
										}
									}
									else
									{
										// can't open door, so break it down
										MONSTER_HITTIME++;
										if ( MONSTER_HITTIME >= HITRATE )
										{
											MONSTER_ATTACK = (rand() % 3) + 1; // random attack motion
											MONSTER_HITTIME = 0;
											hit.entity->skill[4]--; // decrease door health
											if ( myStats->type == MINOTAUR )
											{
												hit.entity->skill[4] = 0;    // minotaurs smash doors instantly
											}
											playSoundEntity(hit.entity, 28, 64);
											if ( hit.entity->skill[4] <= 0 )
											{
												// set direction of splinters
												if ( !hit.entity->skill[0] )
												{
													hit.entity->skill[6] = (my->x > hit.entity->x);
												}
												else
												{
													hit.entity->skill[6] = (my->y < hit.entity->y);
												}
											}
										}
									}
								}
								else if ( hit.entity->behavior == &actMonster )
								{
									Stat* yourStats = hit.entity->getStats();
									if ( hit.entity->getUID() == MONSTER_TARGET )
									{
										MONSTER_STATE = 1; // charge state
									}
									else if ( yourStats )
									{
										if ( my->checkFriend(hit.entity) )
										{
											// would you kindly move out of the way, sir?
											if ( !monsterMoveAside(hit.entity, my) )
											{
												MONSTER_STATE = 2;    // try something else and remake path
											}
										}
										else if ( my->checkEnemy(hit.entity) )
										{
											MONSTER_TARGET = hit.entity->getUID();
											MONSTER_TARGETX = hit.entity->x;
											MONSTER_TARGETY = hit.entity->y;
											MONSTER_STATE = 1; // charge state
										}
									}
								}
								else if ( hit.entity->behavior == &actPlayer )
								{
									if ( my->checkEnemy(hit.entity) )
									{
										MONSTER_TARGET = hit.entity->getUID();
										MONSTER_TARGETX = hit.entity->x;
										MONSTER_TARGETY = hit.entity->y;
										MONSTER_STATE = 1; // charge state
									}
									else
									{
										MONSTER_STATE = 2; // try something else and remake path
									}
								}
								else
								{
									MONSTER_STATE = 2; // remake path
								}
							}
							else
							{
								if ( dist2 <= 0.1 )
								{
									MONSTER_STATE = 2;    // remake path
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
						Entity* target = uidToEntity(MONSTER_TARGET);
						if ( target )
						{
							double tangent = atan2( target->y - my->y, target->x - my->x );
							MONSTER_LOOKTIME = 1;
							MONSTER_MOVETIME = rand() % 10 + 1;
							MONSTER_LOOKDIR = tangent;
						}
						MONSTER_STATE = 0; // no path, return to wait state
					}
				}
				else
				{
					Entity* target = uidToEntity(MONSTER_TARGET);
					if ( target )
					{
						double tangent = atan2( target->y - my->y, target->x - my->x );
						MONSTER_LOOKTIME = 1;
						MONSTER_MOVETIME = rand() % 10 + 1;
						MONSTER_LOOKDIR = tangent;
					}
					MONSTER_STATE = 0; // no path, return to wait state
				}
			}
			else
			{
				Entity* target = uidToEntity(MONSTER_TARGET);
				if ( target )
				{
					double tangent = atan2( target->y - my->y, target->x - my->x );
					MONSTER_LOOKTIME = 1;
					MONSTER_MOVETIME = rand() % 10 + 1;
					MONSTER_LOOKDIR = tangent;
				}
				MONSTER_STATE = 0; // no path, return to wait state
			}
		}
		else if ( MONSTER_STATE == 4 )     // talk state
		{
			MONSTER_VELX = 0;
			MONSTER_VELY = 0;

			// turn towards target
			Entity* target = uidToEntity(MONSTER_TARGET);
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
					MONSTER_STATE = 0;
					MONSTER_TARGET = 0;
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
				MONSTER_STATE = 0;
				MONSTER_TARGET = 0;
			}
		}
		else if ( MONSTER_STATE == 5 )     // dodge state (herx)
		{
			double dist = 0;
			dist = clipMove(&my->x, &my->y, MONSTER_VELX, MONSTER_VELY, my);
			if ( dist != sqrt(MONSTER_VELX * MONSTER_VELX + MONSTER_VELY * MONSTER_VELY) )   // hit obstacle
			{
				MONSTER_SPECIAL = 60;
				if ( rand() % 2 )
				{
					MONSTER_STATE = 0; // wait state
				}
				else
				{
					MONSTER_STATE = 6; // summoning state
				}
			}
			else
			{
				MONSTER_SPECIAL++;
				if ( MONSTER_SPECIAL > 20 )
				{
					MONSTER_SPECIAL = 60;
					if ( rand() % 2 )
					{
						MONSTER_STATE = 0; // wait state
					}
					else
					{
						MONSTER_STATE = 6; // summoning state
					}
				}
			}
		}
		else if ( MONSTER_STATE == 6 )     // summoning state (herx)
		{
			MONSTER_ATTACK = 1;
			MONSTER_ATTACKTIME = 0;
			if ( MONSTER_SPECIAL )
			{
				MONSTER_SPECIAL--;
			}
			else
			{
				MONSTER_SPECIAL = 60;
				MONSTER_STATE = 0; // wait state
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
		else if ( MONSTER_STATE == 7 )     // lich death state
		{
			my->yaw += .5; // rotate
			if ( my->yaw >= PI * 2 )
			{
				my->yaw -= PI * 2;
			}
			MONSTER_ATTACK = 1;
			MONSTER_ATTACKTIME = 0;
			if ( MONSTER_SPECIAL == 0 )
			{
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				int c;
				for ( c = 0; c < MAXPLAYERS; c++ )
				{
					playSoundPlayer(c, 186, 128);
				}
			}
			if ( MONSTER_SPECIAL % 10 == 0 )
			{
				spawnExplosion(my->x - 8 + rand() % 16, my->y - 8 + rand() % 16, -4 + rand() % 8);
			}
			MONSTER_SPECIAL++;
			if ( MONSTER_SPECIAL > 180 )
			{
				lichDie(my);
			}
		}
		else if ( MONSTER_STATE == 8 )     // devil death state
		{
			my->z += .5; // descend slowly
			MONSTER_ATTACK = 4;
			MONSTER_ATTACKTIME = 0;
			/*if( MONSTER_SPECIAL==0 ) {
				int c;
				for( c=0; c<MAXPLAYERS; c++ )
					playSoundPlayer(c,186,128);
			}*/
			if ( MONSTER_SPECIAL == 0 )
			{
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				my->x += cos(my->yaw + PI / 2) * 2;
				my->y += sin(my->yaw + PI / 2) * 2;
			}
			else if ( MONSTER_SPECIAL % 2 == 0 )
			{
				my->x += cos(my->yaw + PI / 2) * 4;
				my->y += sin(my->yaw + PI / 2) * 4;
			}
			else
			{
				my->x -= cos(my->yaw + PI / 2) * 4;
				my->y -= sin(my->yaw + PI / 2) * 4;
			}
			if ( MONSTER_SPECIAL % 10 == 0 )
			{
				spawnExplosion(my->x - 24 + rand() % 48, my->y - 24 + rand() % 48, -16 + rand() % 32);
			}
			MONSTER_SPECIAL++;
			if ( my->z > 96 )
			{
				devilDie(my);
			}
		}
		else if ( MONSTER_STATE == 9 )     // devil teleport state
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
			if ( MONSTER_SPECIAL == 0 )
			{
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				serverUpdateEntitySkill(my, 10);
			}
			MONSTER_SPECIAL++;
			if ( my->z >= 64 )
			{
				node_t* node;
				int c = 0;
				for ( node = map.entities->first; node != NULL; node = node->next )
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
					for ( node = map.entities->first; node != NULL; node = node->next )
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
				MONSTER_SPECIAL = 30;
				MONSTER_STATE = 10;
			}
		}
		else if ( MONSTER_STATE == 10 )     // devil rising state (post-teleport)
		{
			if ( MONSTER_SPECIAL <= 0 )
			{
				my->z = std::max<int>(my->z - 1, -4); // ascend
			}
			else
			{
				MONSTER_SPECIAL--;
				if ( MONSTER_SPECIAL <= 0 )
				{
					if ( myStats->HP > 0 )
					{
						my->flags[PASSABLE] = false;
					}
					node_t* node;
					for ( node = map.entities->first; node != NULL; node = node->next )
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
								if ( stats->HP > 0 )
								{
									stats->HP = 0;
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
				Entity* playertotrack = NULL;
				for ( tempNode = map.entities->first; tempNode != NULL; tempNode = tempNode->next )
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
					MONSTER_TARGET = playertotrack->getUID();
					MONSTER_TARGETX = playertotrack->x;
					MONSTER_TARGETY = playertotrack->y;
					MONSTER_VELX = MONSTER_TARGETX - my->x;
					MONSTER_VELY = MONSTER_TARGETY - my->y;
				}
				else
				{
					MONSTER_VELX = 0;
					MONSTER_VELY = 0;
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

				if ( MONSTER_ATTACKTIME > 60 )
				{
					MONSTER_STATE = 1;
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
		else if ( MONSTER_STATE == 11 )     // devil summoning state
		{
			MONSTER_ATTACK = 4;
			MONSTER_ATTACKTIME = 0;
			if ( MONSTER_SPECIAL == 0 )
			{
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
			}
			MONSTER_SPECIAL++;
			if ( MONSTER_SPECIAL > 120 )
			{
				MONSTER_ATTACK = 0;
				MONSTER_ATTACKTIME = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				MONSTER_SPECIAL = 0;
				MONSTER_STATE = 1;
				node_t* tempNode;
				Entity* playertotrack = NULL;
				for ( tempNode = map.entities->first; tempNode != NULL; tempNode = tempNode->next )
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
					MONSTER_TARGET = playertotrack->getUID();
					MONSTER_TARGETX = playertotrack->x;
					MONSTER_TARGETY = playertotrack->y;
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
		else if ( MONSTER_STATE == 12 )     // devil boulder spawn state
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
			MONSTER_SPECIAL++;
			if ( MONSTER_SPECIAL == 30 )
			{
				MONSTER_ATTACK = 1;
				MONSTER_ATTACKTIME = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
			}
			if ( MONSTER_SPECIAL == 60 )
			{
				int c;
				double oyaw = my->yaw;
				for ( c = 0; c < 12; c++ )
				{
					my->yaw = ((double)c + ((rand() % 100) / 100.f)) * (PI * 2) / 12.f;
					castSpell(my->getUID(), &spell_fireball, true, false);
				}
				my->yaw = oyaw;
				for ( c = 0; c < 7; c++ )
				{
					Entity* entity = newEntity(245, 1, map.entities); // boulder
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
			if ( MONSTER_SPECIAL == 150 )
			{
				MONSTER_ATTACK = 2;
				MONSTER_ATTACKTIME = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
			}
			if ( MONSTER_SPECIAL == 180 )
			{
				int c;
				double oyaw = my->yaw;
				for ( c = 0; c < 12; c++ )
				{
					my->yaw = ((double)c + ((rand() % 100) / 100.f)) * (PI * 2) / 12.f;
					castSpell(my->getUID(), &spell_fireball, true, false);
				}
				my->yaw = oyaw;
				for ( c = 0; c < 7; c++ )
				{
					Entity* entity = newEntity(245, 1, map.entities); // boulder
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
			if ( MONSTER_SPECIAL == 270 )
			{
				MONSTER_ATTACK = 3;
				MONSTER_ATTACKTIME = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
			}
			if ( MONSTER_SPECIAL == 300 )
			{
				int c;
				double oyaw = my->yaw;
				for ( c = 0; c < 12; c++ )
				{
					my->yaw = ((double)c + ((rand() % 100) / 100.f)) * (PI * 2) / 12.f;
					castSpell(my->getUID(), &spell_fireball, true, false);
				}
				my->yaw = oyaw;
				for ( c = 0; c < 12; c++ )
				{
					Entity* entity = newEntity(245, 1, map.entities); // boulder
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
			if ( MONSTER_SPECIAL == 420 )   // 420 blaze it faggot
			{
				MONSTER_ATTACK = 0;
				MONSTER_ATTACKTIME = 0;
				serverUpdateEntitySkill(my, 8);
				serverUpdateEntitySkill(my, 9);
				MONSTER_SPECIAL = 0;
				MONSTER_STATE = 1;
				node_t* tempNode;
				Entity* playertotrack = NULL;
				for ( tempNode = map.entities->first; tempNode != NULL; tempNode = tempNode->next )
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
					MONSTER_TARGET = playertotrack->getUID();
					MONSTER_TARGETX = playertotrack->x;
					MONSTER_TARGETY = playertotrack->y;
				}
			}
		}
	}
	else
	{
		MONSTER_VELX = 0;
		MONSTER_VELY = 0;
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
	}
}

void handleMonsterAttack(Entity* my, Stat* myStats, Entity* target, double dist, int hasrangedweapon)
{
	node_t* node = nullptr;
	Entity* entity = nullptr;
	Stat* hitstats = nullptr;

	// check the range to the target, depending on ranged weapon or melee.
	if ( (dist < STRIKERANGE && !hasrangedweapon) || (dist < 160 && hasrangedweapon) )
	{
		// increment the hit time, don't attack until this reaches the hitrate of the weapon
		MONSTER_HITTIME++;
		int bow = 1;
		if ( hasrangedweapon )
		{
			if ( myStats->weapon->type == SLING || myStats->weapon->type == SHORTBOW || myStats->weapon->type == ARTIFACT_BOW )
			{
				bow = 2;
			}
		}
		if ( myStats->type == CRYSTALGOLEM )
		{
			bow = 1;
		}

		// check if ready to attack
		if ( (MONSTER_HITTIME >= HITRATE * bow && myStats->type != LICH) || (MONSTER_HITTIME >= 5 && myStats->type == LICH) )
		{
			if ( myStats->type == LICH )
			{
				MONSTER_SPECIAL++;
				if ( MONSTER_SPECIAL >= 5 )
				{
					MONSTER_SPECIAL = 90;
					MONSTER_TARGET = 0;
					MONSTER_TARGETX = my->x - 50 + rand() % 100;
					MONSTER_TARGETY = my->y - 50 + rand() % 100;
					MONSTER_STATE = 2; // path state
				}
			}

			// reset the hit timer
			MONSTER_HITTIME = 0;
			int tracedist;
			if ( hasrangedweapon )
			{
				tracedist = 160;
			}
			else
			{
				tracedist = STRIKERANGE;
			}

			// check again for the target in attack range. return the result into hit.entity.
			double newTangent = atan2(target->y - my->y, target->x - my->x);
			lineTrace(my, my->x, my->y, newTangent, tracedist, 0, false);
			if ( hit.entity != nullptr )
			{
				// found the target in range
				hitstats = hit.entity->getStats();
				if ( hit.entity->behavior == &actMonster && !hasrangedweapon )
				{
					// alert the monster!
					if ( hit.entity->skill[0] != 1 )
					{
						//hit.entity->skill[0]=0;
						//hit.entity->skill[4]=0;
						//hit.entity->fskill[4]=atan2(players[player]->y-hit.entity->y,players[player]->x-hit.entity->x);
						hit.entity->skill[0] = 2;
						hit.entity->skill[1] = my->getUID();
						hit.entity->fskill[2] = my->x;
						hit.entity->fskill[3] = my->y;
					}
				}
				if ( hit.entity->getStats() != NULL )
				{
					// prepare attack, set the animation of the attack based on the current weapon.
					int pose = 0;
					if ( myStats->weapon )
					{
						if ( itemCategory(myStats->weapon) == MAGICSTAFF )
						{
							pose = 3;  // jab
						}
						else if ( itemCategory(myStats->weapon) == SPELLBOOK )
						{
							pose = 1;  // vertical swing
						}
						else if ( hasrangedweapon )
						{
							pose = 0;
						}
						else
						{
							pose = rand() % 3 + 1;
						}
					}

					if ( myStats->type == CRYSTALGOLEM )
					{
						pose = 5;
					}
					else if ( myStats->type == CRYSTALGOLEM && rand() % 2 )
					{
						pose = GOLEM_SMASH;
					}

					// turn to the target, then reset my yaw.
					double oYaw = my->yaw;
					my->yaw = newTangent;
					my->attack(pose, 1, nullptr); // attacku! D:<
					my->yaw = oYaw;
				}
			}
		}
	}

	//if ( myStats->type == CRYSTALGOLEM )
	//{
	//	Entity* tmpEntity = hit.entity;

	//	// aoe
	//	for ( node = map.entities->first; node != NULL; node = node->next )
	//	{
	//		entity = (Entity*)node->element;
	//		if ( (entity->behavior == &actMonster || entity->behavior == &actPlayer) && entity != tmpEntity && entity != my )
	//		{
	//			double aoeTangent = atan2(entity->y - my->y, entity->x - my->x);
	//			int dir = my->yaw - aoeTangent;
	//			while ( dir >= PI )
	//			{
	//				dir -= PI * 2;
	//			}
	//			while ( dir < -PI )
	//			{
	//				dir += PI * 2;
	//			}
	//			if ( dir >= -1 * PI / 2 && dir <= 1 * PI / 2 )
	//			{
	//				// 60 degree arc to check.
	//				//lineTrace(my, my->x, my->y, aoeTangent, 64, 0, true);
	//				double dist = sqrt(pow(my->x - entity->x, 2) + pow(my->y - entity->y, 2));
	//				//messagePlayer(0, "dist %f", dist);
	//				if ( dist < 32 )
	//				{
	//					//messagePlayer(0, "hit enemy type %d", hit.entity->getStats()->type);
	//					my->attack(pose, 1, entity); // attacku! D:<

	//												 //Entity* entity2 = newEntity(-1, 1, map.entities);
	//												 //entity2->flags[INVISIBLE] = false;
	//												 //entity2->flags[UPDATENEEDED] = true;
	//												 //entity2->x = entity->x - 4 + rand() % 8;
	//												 //entity2->y = entity->y - 4 + rand() % 8;
	//												 //entity2->z = 4;
	//												 //entity2->sizex = 4;
	//												 //entity2->sizey = 4;
	//												 //entity2->yaw = rand() % 360 * PI / 180;
	//												 //entity2->vel_x = (rand() % 20 - 10) / 10.0;
	//												 //entity2->vel_y = (rand() % 20 - 10) / 10.0;
	//												 //entity2->vel_z = -.75 - (rand() % 5) / 10.0;
	//												 //entity2->flags[PASSABLE] = true;
	//												 //entity2->behavior = &actItem;
	//												 //entity2->flags[USERFLAG1] = true; // no collision: helps performance
	//												 //entity2->skill[10] = GEM_ROCK;    // type
	//												 //entity2->skill[11] = WORN;        // status
	//												 //entity2->skill[12] = 0;           // beatitude
	//												 //entity2->skill[13] = 1;           // count
	//												 //entity2->skill[14] = 0;           // appearance
	//												 //entity2->skill[15] = false;       // identified
	//				}
	//			}
	//		}
	//	}
	//}







	return;
}

int limbAnimateWithOvershoot(Entity* limb, int axis, double setpointRate, double setpoint, double endpointRate, double endpoint, int dir)
{
	if ( axis == 0 || MONSTER_LIMB_OVERSHOOT == ANIMATE_OVERSHOOT_NONE || dir == ANIMATE_DIR_NONE )
	{
		// no animation required.
		return -1;
	}

	if ( axis == ANIMATE_PITCH )
	{
		if ( MONSTER_LIMB_OVERSHOOT == ANIMATE_OVERSHOOT_TO_SETPOINT )
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
				MONSTER_LIMB_OVERSHOOT = ANIMATE_OVERSHOOT_TO_ENDPOINT;
				return ANIMATE_OVERSHOOT_TO_SETPOINT; //reached setpoint
			}
		}
		else if ( MONSTER_LIMB_OVERSHOOT == ANIMATE_OVERSHOOT_TO_ENDPOINT )
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
				MONSTER_LIMB_OVERSHOOT = ANIMATE_OVERSHOOT_NONE;
				return ANIMATE_OVERSHOOT_TO_ENDPOINT; //reached endpoint.
			}
		}
	}
	else if ( axis == ANIMATE_ROLL )
	{
		if ( MONSTER_LIMB_OVERSHOOT == ANIMATE_OVERSHOOT_TO_SETPOINT )
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
				MONSTER_LIMB_OVERSHOOT = ANIMATE_OVERSHOOT_TO_ENDPOINT;
				return ANIMATE_OVERSHOOT_TO_SETPOINT; //reached setpoint
			}
		}
		else if ( MONSTER_LIMB_OVERSHOOT == ANIMATE_OVERSHOOT_TO_ENDPOINT )
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
				MONSTER_LIMB_OVERSHOOT = ANIMATE_OVERSHOOT_NONE;
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
		return -1;
	}

	if ( axis == ANIMATE_YAW )
	{
		limb->yaw += rate;
		if ( limb->yaw >= setpoint )
		{
			limb->yaw = setpoint;
			return 1; //reached setpoint
		}
	} else if ( axis == ANIMATE_PITCH )
	{
		limb->pitch += rate;
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
				if ( MONSTER_LIMB_DIR == ANIMATE_DIR_NONE )
				{
					// no direction for shake is set.
					MONSTER_LIMB_DIR = ANIMATE_DIR_POSITIVE;
				}
				if ( MONSTER_LIMB_DIR == ANIMATE_DIR_POSITIVE )
				{
					limb->pitch += shakerate;
					MONSTER_LIMB_DIR = ANIMATE_DIR_NEGATIVE;
				}
				else if ( MONSTER_LIMB_DIR == ANIMATE_DIR_NEGATIVE )
				{
					limb->pitch -= shakerate;
					MONSTER_LIMB_DIR = ANIMATE_DIR_POSITIVE;
				}
			}
			return 1; //reached setpoint
		}
	} else if ( axis == ANIMATE_ROLL )
	{
		limb->roll += rate;
		while ( limb->roll < 0 )
		{
			limb->roll += 2 * PI;
		}
		while ( limb->roll >= 2 * PI )
		{
			limb->roll -= 2 * PI;
		}

		if ( limb->roll >= setpoint )
		{
			limb->roll = setpoint;
			if ( shake )
			{
				if ( MONSTER_LIMB_DIR == ANIMATE_DIR_NONE )
				{
					// no direction for shake is set.
					MONSTER_LIMB_DIR = ANIMATE_DIR_POSITIVE;
				}
				if ( MONSTER_LIMB_DIR == ANIMATE_DIR_POSITIVE )
				{
					limb->roll += shakerate;
					MONSTER_LIMB_DIR = ANIMATE_DIR_NEGATIVE;
				}
				else if ( MONSTER_LIMB_DIR == ANIMATE_DIR_NEGATIVE )
				{
					limb->roll -= shakerate;
					MONSTER_LIMB_DIR = ANIMATE_DIR_POSITIVE;
				}
			}
			return 1; //reached setpoint
		}
	}

	return 0;
}

int limbAngleWithinRange(real_t angle, double rate, double setpoint)
{
	if ( (angle <= (setpoint + rate)) && (angle >= (setpoint - rate)) )
	{
		return 1;
	}

	return 0;
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
				list_RemoveNodeWithElement<Uint32>(oldLeaderStats->FOLLOWERS, *myuid);
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
	}

	return true;
}
