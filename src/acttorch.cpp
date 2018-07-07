/*-------------------------------------------------------------------------------

	BARONY
	File: acttorch.cpp
	Desc: behavior function for torch

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "items.hpp"
#include "sound.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define TORCH_LIGHTING my->skill[0]
#define TORCH_FLICKER my->skill[1]
#define TORCH_FIRE my->skill[3]

bool flickerLights = true;

void actTorch(Entity* my)
{
	int i;

	// ambient noises (yeah, torches can make these...)
	TORCH_FIRE--;
	if ( TORCH_FIRE <= 0 )
	{
		TORCH_FIRE = 480;
		playSoundEntityLocal( my, 133, 32 );
	}
	Entity* entity = spawnFlame(my, SPRITE_FLAME);
	entity->x += .25 * cos(my->yaw);
	entity->y += .25 * sin(my->yaw);
	entity->z -= 2.5;
	entity->flags[GENIUS] = false;
	entity->setUID(-3);

	// check wall behind me. (e.g mined or destroyed then remove torch)
	int checkx = my->x - cos(my->yaw) * 8;
	checkx = checkx >> 4;
	int checky = my->y - sin(my->yaw) * 8;
	checky = checky >> 4;
	if ( !map.tiles[OBSTACLELAYER + checky * MAPLAYERS + checkx * MAPLAYERS * map.height] )   // wall
	{
		my->removeLightField();
		list_RemoveNode(my->mynode);
		return;
	}

	// lighting
	if ( !TORCH_LIGHTING )
	{
		my->light = lightSphereShadow(my->x / 16, my->y / 16, 7, 192);
		TORCH_LIGHTING = 1;
	}
	if ( flickerLights )
	{
		//Torches will never flicker if this setting is disabled.
		TORCH_FLICKER--;
	}
	if (TORCH_FLICKER <= 0)
	{
		TORCH_LIGHTING = (TORCH_LIGHTING == 1) + 1;

		if (TORCH_LIGHTING == 1)
		{
			my->removeLightField();
			my->light = lightSphereShadow(my->x / 16, my->y / 16, 7, 192);
		}
		else
		{
			my->removeLightField();
			my->light = lightSphereShadow(my->x / 16, my->y / 16, 7, 174);
		}
		TORCH_FLICKER = 2 + rand() % 7;
	}

	// using
	if ( multiplayer != CLIENT )
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if ( (i == 0 && selectedEntity == my) || (client_selected[i] == my) )
			{
				if (inrange[i])
				{
					messagePlayer(i, language[589]);
					list_RemoveNode(my->light->node);
					list_RemoveNode(my->mynode);
					Item* item = newItem(TOOL_TORCH, WORN, 0, 1, 0, true, NULL);
					itemPickup(i, item);
					free(item);
					return;
				}
			}
		}
		if ( my->isInteractWithMonster() )
		{
			list_RemoveNode(my->light->node);
			list_RemoveNode(my->mynode);
			Entity* monster = uidToEntity(my->interactedByMonster);
			if ( monster )
			{
				Item* item = newItem(TOOL_TORCH, WORN, 0, 1, 0, true, NULL);
				monster->addItemToMonsterInventory(item);
			}
			my->clearMonsterInteract();
		}
	}
}

#define TORCH_LIGHTING my->skill[0]
#define TORCH_FLICKER my->skill[1]
#define TORCH_FIRE my->skill[3]

void actCrystalShard(Entity* my)
{
	int i;

	// ambient noises (yeah, torches can make these...)
	TORCH_FIRE--;
	if ( TORCH_FIRE <= 0 )
	{
		TORCH_FIRE = 480;
		playSoundEntityLocal(my, 133, 32);
	}
	Entity* entity = spawnFlame(my, SPRITE_CRYSTALFLAME);
	entity->x += .25 * cos(my->yaw);
	entity->y += .25 * sin(my->yaw);
	entity->z -= 2.5;
	entity->flags[GENIUS] = false;
	entity->setUID(-3);

	// check wall behind me. (e.g mined or destroyed then remove torch)
	int checkx = my->x - cos(my->yaw) * 8;
	checkx = checkx >> 4;
	int checky = my->y - sin(my->yaw) * 8;
	checky = checky >> 4;
	if ( !map.tiles[OBSTACLELAYER + checky * MAPLAYERS + checkx * MAPLAYERS * map.height] )   // wall
	{
		my->removeLightField();
		list_RemoveNode(my->mynode);
		return;
	}

	// lighting
	if ( !TORCH_LIGHTING )
	{
		my->light = lightSphereShadow(my->x / 16, my->y / 16, 5, 128);
		TORCH_LIGHTING = 1;
	}

	if ( flickerLights )
	{
		//Crystal shards will never flicker if this setting is disabled.
		TORCH_FLICKER--;
	}
	if ( TORCH_FLICKER <= 0 )
	{
		TORCH_LIGHTING = (TORCH_LIGHTING == 1) + 1;

		if ( TORCH_LIGHTING == 1 )
		{
			my->removeLightField();
			my->light = lightSphereShadow(my->x / 16, my->y / 16, 5, 128);
		}
		else
		{
			my->removeLightField();
			my->light = lightSphereShadow(my->x / 16, my->y / 16, 5, 112);
		}
		TORCH_FLICKER = 2 + rand() % 7;
	}

	// using
	if ( multiplayer != CLIENT )
	{
		for ( i = 0; i < MAXPLAYERS; i++ )
		{
			if ( (i == 0 && selectedEntity == my) || (client_selected[i] == my) )
			{
				if ( inrange[i] )
				{
					messagePlayer(i, language[589]);
					list_RemoveNode(my->light->node);
					list_RemoveNode(my->mynode);
					Item* item = newItem(TOOL_CRYSTALSHARD, WORN, 0, 1, 0, true, NULL);
					itemPickup(i, item);
					free(item);
					return;
				}
			}
		}
	}
}
