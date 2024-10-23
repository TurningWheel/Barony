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
#include "engine/audio/sound.hpp"
#include "net.hpp"
#include "player.hpp"
#include "prng.hpp"

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
	int i;

	// init
	if ( !CAMPFIRE_INIT )
	{
		CAMPFIRE_INIT = 1;
		CAMPFIRE_HEALTH = MAXPLAYERS;
		my->createWorldUITooltip();
	}

	// crackling sounds
	if ( CAMPFIRE_HEALTH > 0 )
	{
#ifdef USE_FMOD
		if ( CAMPFIRE_SOUNDTIME == 0 )
		{
			CAMPFIRE_SOUNDTIME--;
			my->stopEntitySound();
			my->entity_sound = playSoundEntityLocal(my, 133, 32);
		}
		if ( my->entity_sound )
		{
			bool playing = false;
			my->entity_sound->isPlaying(&playing);
			if ( !playing )
			{
				my->entity_sound = nullptr;
			}
		}
#else
		CAMPFIRE_SOUNDTIME--;
		if ( CAMPFIRE_SOUNDTIME <= 0 )
		{
			CAMPFIRE_SOUNDTIME = 480;
			playSoundEntityLocal( my, 133, 128 );
		}
#endif

		// spew flame particles
		if ( flickerLights )
		{
		    for ( i = 0; i < 3; i++ )
		    {
				if ( Entity* entity = spawnFlame(my, SPRITE_FLAME) )
				{
					entity->x += ((local_rng.rand() % 30) - 10) / 10.f;
					entity->y += ((local_rng.rand() % 30) - 10) / 10.f;
					entity->z -= 1;
				}
		    }
			if ( Entity* entity = spawnFlame(my, SPRITE_FLAME) )
			{
				entity->z -= 2;
			}
		}
		else
		{
		    if ( ticks % TICKS_PER_SECOND == 0 )
		    {
				if ( Entity* entity = spawnFlame(my, SPRITE_FLAME) )
				{
					entity->z -= 2;
				}
		    }
		}

		// light environment
		if ( !CAMPFIRE_LIGHTING )
		{
			my->light = addLight(my->x / 16, my->y / 16, "campfire");
			CAMPFIRE_LIGHTING = 1;
		}
		if ( flickerLights )
		{
			//Campfires will never flicker if this setting is disabled.
			CAMPFIRE_FLICKER--;
		}
		if (CAMPFIRE_FLICKER <= 0)
		{
			CAMPFIRE_LIGHTING = (CAMPFIRE_LIGHTING == 1) + 1;

			if (CAMPFIRE_LIGHTING == 1)
			{
				my->removeLightField();
				my->light = addLight(my->x / 16, my->y / 16, "campfire");
			}
			else
			{
				my->removeLightField();
				my->light = addLight(my->x / 16, my->y / 16, "campfire_flicker");
			}
			CAMPFIRE_FLICKER = 2 + local_rng.rand() % 7;
		}
	}
	else
	{
		my->removeLightField();
		my->light = NULL;

		my->stopEntitySound();
	}

	if ( multiplayer != CLIENT )
	{
		// using campfire
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if ( selectedEntity[i] == my || client_selected[i] == my )
			{
				if (inrange[i])
				{
					if ( CAMPFIRE_HEALTH > 0 )
					{
						messagePlayer(i, MESSAGE_INTERACTION, Language::get(457));
						CAMPFIRE_HEALTH--;
						if ( CAMPFIRE_HEALTH <= 0 )
						{
							serverUpdateEntitySkill(my, 3); // extinguish for all clients
							messagePlayer(i, MESSAGE_INTERACTION, Language::get(458));
							my->removeLightField();
							my->light = NULL;
						}
						Item* item = newItem(TOOL_TORCH, WORN, 0, 1, 0, true, NULL);
						itemPickup(i, item);
						free(item);
					}
					else
					{
						messagePlayer(i, MESSAGE_INTERACTION, Language::get(458));
					}
				}
			}
		}
	}
}
