/*-------------------------------------------------------------------------------

	BARONY
	File: monster_rat.cpp
	Desc: implements all of the rat monster's code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "monster.hpp"
#include "sound.hpp"
#include "items.hpp"
#include "net.hpp"
#include "collision.hpp"

void initRat(Entity* my, Stat* myStats)
{
	int c;

	my->sprite = 131; // rat model

	my->flags[UPDATENEEDED] = true;
	my->flags[INVISIBLE] = false;

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 29;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = 29;
		MONSTER_IDLEVAR = 1;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		myStats->sex = static_cast<sex_t>(rand() % 2);
		myStats->appearance = rand();
		strcpy(myStats->name, "");
		myStats->inventory.first = NULL;
		myStats->inventory.last = NULL;
		myStats->HP = 30;
		myStats->MAXHP = 30;
		myStats->MP = 10;
		myStats->MAXMP = 10;
		myStats->OLDHP = myStats->HP;
		myStats->STR = 0;
		myStats->DEX = 2;
		myStats->CON = 1;
		myStats->INT = -2;
		myStats->PER = 0;
		myStats->CHR = -1;
		myStats->EXP = 0;
		myStats->LVL = 1;
		myStats->GOLD = 0;
		myStats->HUNGER = 900;
		if ( !myStats->leader_uid )
		{
			myStats->leader_uid = 0;
		}
		myStats->FOLLOWERS.first = NULL;
		myStats->FOLLOWERS.last = NULL;
		for ( c = 0; c < std::max(NUMPROFICIENCIES, NUMEFFECTS); c++ )
		{
			if ( c < NUMPROFICIENCIES )
			{
				myStats->PROFICIENCIES[c] = 0;
			}
			if ( c < NUMEFFECTS )
			{
				myStats->EFFECTS[c] = false;
			}
			if ( c < NUMEFFECTS )
			{
				myStats->EFFECTS_TIMERS[c] = 0;
			}
		}
		myStats->helmet = NULL;
		myStats->breastplate = NULL;
		myStats->gloves = NULL;
		myStats->shoes = NULL;
		myStats->shield = NULL;
		myStats->weapon = NULL;
		myStats->cloak = NULL;
		myStats->amulet = NULL;
		myStats->ring = NULL;
		myStats->mask = NULL;

		if ( rand() % 4 )
		{
			if ( rand() % 2 )
			{
				newItem( FOOD_MEAT, EXCELLENT, 0, 1, rand(), false, &myStats->inventory );
			}
			else
			{
				newItem( FOOD_CHEESE, DECREPIT, 0, 1, rand(), false, &myStats->inventory );
			}
		}

		if ( rand() % 50 == 0 && !my->flags[USERFLAG2] )
		{
			strcpy(myStats->name, "Algernon");
			myStats->HP = 60;
			myStats->MAXHP = 60;
			myStats->OLDHP = myStats->HP;
			myStats->STR = -1;
			myStats->DEX = 20;
			myStats->CON = 2;
			myStats->INT = 20;
			myStats->PER = -2;
			myStats->CHR = 5;
			myStats->LVL = 10;
			newItem(GEM_EMERALD, static_cast<ItemStatus>(1 + rand() % 4), 0, 1, rand(), true, &myStats->inventory );

			int c;
			for ( c = 0; c < 6; c++ )
			{
				Entity* entity = summonMonster(RAT, my->x, my->y);
				if ( entity )
				{
					entity->parent = my->getUID();
				}
			}
		}
	}
}

void ratAnimate(Entity* my, double dist)
{
	// move legs
	if ( (ticks % 10 == 0 && dist > 0.1) || (MONSTER_ATTACKTIME != MONSTER_ATTACK) )
	{
		MONSTER_ATTACKTIME = MONSTER_ATTACK;
		if ( my->sprite == 131 )
		{
			my->sprite = 265;
		}
		else
		{
			my->sprite = 131;
		}
	}
}

void ratDie(Entity* my)
{
	int c = 0;
	for ( c = 0; c < 5; c++ )
	{
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}
	if (spawn_blood)
	{
		int x, y;
		x = std::min<unsigned int>(std::max<int>(0, my->x / 16), map.width - 1);
		y = std::min<unsigned int>(std::max<int>(0, my->y / 16), map.height - 1);
		if ( map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height] )
		{
			if ( !checkObstacle(my->x, my->y, my, NULL) )
			{
				Entity* entity = newEntity(160, 1, map.entities);
				entity->x = my->x;
				entity->y = my->y;
				entity->z = 7.4 + (rand() % 20) / 100.f;
				entity->parent = my->getUID();
				entity->sizex = 2;
				entity->sizey = 2;
				entity->yaw = (rand() % 360) * PI / 180.0;
				entity->flags[UPDATENEEDED] = true;
				entity->flags[PASSABLE] = true;
			}
		}
	}
	playSoundEntity(my, 30, 64);
	list_RemoveNode(my->mynode);
	return;
}
