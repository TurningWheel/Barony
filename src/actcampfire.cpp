/*-------------------------------------------------------------------------------

	BARONY
	File: actcampfire.cpp
	Desc: behavior function for campfires

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "items.hpp"
#include "sound.hpp"
#include "net.hpp"
#include "player.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define CAMPFIRE_LIGHTING my->skill[0]
#define CAMPFIRE_FLICKER my->skill[1]
#define CAMPFIRE_HEALTH my->skill[3]
#define CAMPFIRE_INIT my->skill[4]
#define CAMPFIRE_SOUNDTIME my->skill[5]

void actCampfire(Entity* my)
{
	Entity* entity;
	int i;

	// init
	if ( !CAMPFIRE_INIT )
	{
		CAMPFIRE_INIT = 1;
		CAMPFIRE_HEALTH = MAXPLAYERS;
	}

	// crackling sounds
	if ( CAMPFIRE_HEALTH > 0 )
	{
		CAMPFIRE_SOUNDTIME--;
		if ( CAMPFIRE_SOUNDTIME <= 0 )
		{
			CAMPFIRE_SOUNDTIME = 480;
			playSoundEntityLocal( my, 133, 128 );
		}

		// spew flame particles
		for ( i = 0; i < 3; i++ )
		{
			entity = spawnFlame(my);
			entity->x += ((rand() % 30) - 10) / 10.f;
			entity->y += ((rand() % 30) - 10) / 10.f;
			entity->z -= 1;
		}
		entity = spawnFlame(my);
		entity->z -= 2;

		// light environment
		if ( !CAMPFIRE_LIGHTING )
		{
			my->light = lightSphereShadow(my->x / 16, my->y / 16, 6, 160);
			CAMPFIRE_LIGHTING = 1;
		}
		CAMPFIRE_FLICKER--;
		if (CAMPFIRE_FLICKER <= 0)
		{
			CAMPFIRE_LIGHTING = (CAMPFIRE_LIGHTING == 1) + 1;

			if (CAMPFIRE_LIGHTING == 1)
			{
				if ( my->light != NULL )
				{
					list_RemoveNode(my->light->node);
				}
				my->light = lightSphereShadow(my->x / 16, my->y / 16, 6, 160);
			}
			else
			{
				if ( my->light != NULL )
				{
					list_RemoveNode(my->light->node);
				}
				my->light = lightSphereShadow(my->x / 16, my->y / 16, 6, 152);
			}
			CAMPFIRE_FLICKER = 2 + rand() % 7;
		}
	}
	else
	{
		if ( my->light )
			if ( my->light->node )
			{
				list_RemoveNode(my->light->node);
			}
		my->light = NULL;
		my->flags[BRIGHT] = false;
	}

	if ( multiplayer != CLIENT )
	{
		// using campfire
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if ( (i == 0 && selectedEntity == my) || (client_selected[i] == my) )
			{
				if (inrange[i])
				{
					if ( CAMPFIRE_HEALTH > 0 )
					{
						messagePlayer(i, language[457]);
						CAMPFIRE_HEALTH--;
						if ( CAMPFIRE_HEALTH <= 0 )
						{
							serverUpdateEntitySkill(my, 3); // extinguish for all clients
							messagePlayer(i, language[458]);
							if ( my->light )
								if ( my->light->node )
								{
									list_RemoveNode(my->light->node);
								}
							my->light = NULL;
						}
						Item* item = newItem(TOOL_TORCH, WORN, 0, 1, 0, true, NULL);
						itemPickup(i, item);
						free(item);
					}
					else
					{
						messagePlayer(i, language[458]);
					}
				}
			}
		}
	}
}
