/*-------------------------------------------------------------------------------

	BARONY
	File: monster_shared.cpp
	Desc: contains shared monster implementation and helper functions

	Copyright 2013-2017 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "entity.hpp"

void Entity::initMonster(int mySprite)
{
	sprite = mySprite;

	//Common flags.
	flags[UPDATENEEDED] = true;
	flags[BLOCKSIGHT] = true;
	flags[INVISIBLE] = false;
}

void Entity::actMonsterLimb(bool processLight)
{
	//If no longer part of a monster, delete the limb.
	Entity *parentEnt = nullptr;
	if ( (parentEnt = uidToEntity(skill[2])) == nullptr )
	{
		list_RemoveNode(mynode);
		return;
	}

	//Do something magical beyond my comprehension.
	if ( multiplayer != CLIENT )
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( inrange[i] )
			{
				if ( i == 0 && selectedEntity == this )
				{
					parentEnt->skill[13] = i + 1;
				}
				else if ( client_selected[i] == this )
				{
					parentEnt->skill[13] = i + 1;
				}
			}
		}
	}

	if ( processLight )
	{
		//Only run by monsters who can carry stuff (like torches). Sorry, rats.
		if ( light != nullptr )
		{
			list_RemoveNode(light->node);
			light = nullptr;
		}

		int carryingLightSource = 0;
		if ( flags[INVISIBLE] == false )
		{
			if ( sprite == 93 )   // torch
			{
				carryingLightSource = 6;
			}
			else if ( sprite == 94 )     // lantern
			{
				carryingLightSource = 9;
			}
			else if ( sprite == 529 )	// crystal shard
			{
				carryingLightSource = 4;
			}
		}

		if ( carryingLightSource != 0 )
		{
			light = lightSphereShadow(x / 16, y / 16, carryingLightSource, 50 + 15 * carryingLightSource);
		}
	}
}

void Entity::removeMonsterDeathNodes()
{
	int i = 0;
	node_t *nextnode = nullptr;
	for ( node_t *node = children.first; node != nullptr; node = nextnode )
	{
		nextnode = node->next;
		if ( node->element != nullptr && i >= 2 )
		{
			Entity* entity = (Entity*)node->element;
			if ( entity->light != nullptr )
			{
				list_RemoveNode(entity->light->node);
			}
			entity->light = nullptr;
			entity->flags[UPDATENEEDED] = false; //TODO: Do only demon & baphy need this?
			list_RemoveNode(entity->mynode);
		}
		list_RemoveNode(node);
		++i;
	}
}

void Entity::spawnBlood(int bloodSprite)
{
	if ( spawn_blood )
	{
		int tileX = std::min<unsigned int>(std::max<int>(0, this->x / 16), map.width - 1);
		int tileY = std::min<unsigned int>(std::max<int>(0, this->y / 16), map.height - 1);
		if ( map.tiles[tileY * MAPLAYERS + tileX * MAPLAYERS * map.height] )
		{
			if ( !checkObstacle(this->x, this->y, this, nullptr) )
			{
				Entity* entity = newEntity(bloodSprite, 1, map.entities);
				entity->x = this->x;
				entity->y = this->y;
				entity->z = 7.4 + (rand() % 20) / 100.f;
				entity->parent = getUID();
				entity->sizex = 2;
				entity->sizey = 2;
				entity->yaw = (rand() % 360) * PI / 180.0;
				entity->flags[UPDATENEEDED] = true;
				entity->flags[PASSABLE] = true;
			}
		}
	}
}
