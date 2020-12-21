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
#include "interface/interface.hpp"

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

	if ( my->ticks == 1 )
	{
		my->createWorldUITooltip();
	}

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
			if ( (i == 0 && selectedEntity[0] == my) || (client_selected[i] == my) || (splitscreen && selectedEntity[i] == my) )
			{
				if (inrange[i])
				{
					bool trySalvage = false;
					if ( static_cast<Uint32>(my->itemAutoSalvageByPlayer) == players[i]->entity->getUID() )
					{
						trySalvage = true;
						my->itemAutoSalvageByPlayer = 0; // clear interact flag.
					}

					Item* item = newItem(TOOL_TORCH, WORN, 0, 1, 0, true, NULL);

					if ( trySalvage )
					{
						// auto salvage this item, don't pick it up.
						messagePlayer(i, language[589]);
						bool salvaged = false;
						if ( GenericGUI[i].isItemSalvageable(item, i) )
						{
							if ( GenericGUI[i].tinkeringSalvageItem(item, true, i) )
							{
								salvaged = true;
							}
						}

						if ( salvaged )
						{
							if ( players[i] != nullptr && players[i]->entity != nullptr )
							{
								playSoundEntity(players[i]->entity, 35 + rand() % 3, 64);
							}
							free(item);
							/*if ( GenericGUI.tinkeringKitRollIfShouldBreak() )
							{
								GenericGUI.tinkeringKitDegradeOnUse(i);
							}*/
							list_RemoveNode(my->light->node);
							list_RemoveNode(my->mynode);
							return;
						}
						else
						{
							// unable to salvage.
							free(item);
							return;
						}
					}
					else
					{
						messagePlayer(i, language[589]);
						list_RemoveNode(my->light->node);
						list_RemoveNode(my->mynode);
						itemPickup(i, item);
						free(item);
					}
					return;
				}
			}
		}
		if ( my->isInteractWithMonster() )
		{
			list_RemoveNode(my->light->node);
			Entity* monster = uidToEntity(my->interactedByMonster);
			my->clearMonsterInteract();
			if ( monster )
			{
				Item* item = newItem(TOOL_TORCH, WORN, 0, 1, 0, true, NULL);
				dropItemMonster(item, monster, monster->getStats());
				//monster->addItemToMonsterInventory(item);
			}
			list_RemoveNode(my->mynode);
		}
	}
}

#define TORCH_LIGHTING my->skill[0]
#define TORCH_FLICKER my->skill[1]
#define TORCH_FIRE my->skill[3]

void actCrystalShard(Entity* my)
{
	int i;

	if ( my->ticks == 1 )
	{
		my->createWorldUITooltip();
	}

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
			if ( (i == 0 && selectedEntity[0] == my) || (client_selected[i] == my) || (splitscreen && selectedEntity[i] == my) )
			{
				if ( inrange[i] )
				{
					bool trySalvage = false;
					if ( static_cast<Uint32>(my->itemAutoSalvageByPlayer) == players[i]->entity->getUID() )
					{
						trySalvage = true;
						my->itemAutoSalvageByPlayer = 0; // clear interact flag.
					}

					Item* item = newItem(TOOL_CRYSTALSHARD, WORN, 0, 1, 0, true, NULL);

					if ( trySalvage )
					{
						// auto salvage this item, don't pick it up.
						messagePlayer(i, language[589]);
						bool salvaged = false;
						if ( GenericGUI[i].isItemSalvageable(item, i) )
						{
							if ( GenericGUI[i].tinkeringSalvageItem(item, true, i) )
							{
								salvaged = true;
							}
						}

						if ( salvaged )
						{
							if ( players[i] != nullptr && players[i]->entity != nullptr )
							{
								playSoundEntity(players[i]->entity, 35 + rand() % 3, 64);
							}
							free(item);
							/*if ( GenericGUI.tinkeringKitRollIfShouldBreak() )
							{
							GenericGUI.tinkeringKitDegradeOnUse(i);
							}*/
							list_RemoveNode(my->light->node);
							list_RemoveNode(my->mynode);
							return;
						}
						else
						{
							// unable to salvage.
							free(item);
							return;
						}
					}
					else
					{
						messagePlayer(i, language[589]);
						list_RemoveNode(my->light->node);
						list_RemoveNode(my->mynode);
						itemPickup(i, item);
						free(item);
					}
					return;
				}
			}
		}
	}
}

void actLightSource(Entity* my)
{
	if ( !my )
	{
		return;
	}

	my->actLightSource();
}

#define LIGHTSOURCE_LIGHT skill[8]
#define LIGHTSOURCE_FLICKER skill[9]
#define LIGHTSOURCE_ENABLED skill[10]

void Entity::actLightSource()
{
	if ( multiplayer != CLIENT )
	{
		if ( lightSourceDelay > 0 && lightSourceDelayCounter == 0 )
		{
			lightSourceDelayCounter = lightSourceDelay;
		}
	}

	if ( LIGHTSOURCE_ENABLED )
	{
		// lighting
		if ( !LIGHTSOURCE_LIGHT )
		{
			light = lightSphereShadow(x / 16, y / 16, lightSourceRadius, lightSourceBrightness);
			LIGHTSOURCE_LIGHT = 1;
		}
		if ( lightSourceFlicker && flickerLights )
		{
			--LIGHTSOURCE_FLICKER;
		}
		else
		{
			LIGHTSOURCE_LIGHT = 1;
			if ( !light )
			{
				light = lightSphereShadow(x / 16, y / 16, lightSourceRadius, lightSourceBrightness);
			}
		}

		if ( LIGHTSOURCE_FLICKER <= 0 )
		{
			LIGHTSOURCE_LIGHT = (LIGHTSOURCE_LIGHT == 1) + 1;

			if ( LIGHTSOURCE_LIGHT == 1 )
			{
				removeLightField();
				light = lightSphereShadow(x / 16, y / 16, lightSourceRadius, lightSourceBrightness);
			}
			else
			{
				removeLightField();
				light = lightSphereShadow(x / 16, y / 16, lightSourceRadius, std::max(lightSourceBrightness - 16, 0));
			}
			LIGHTSOURCE_FLICKER = 2 + rand() % 7;
		}

		if ( multiplayer != CLIENT )
		{
			if ( !lightSourceAlwaysOn && (circuit_status == CIRCUIT_OFF && !lightSourceInvertPower)
				|| (circuit_status == CIRCUIT_ON && lightSourceInvertPower == 1) )
			{
				if ( LIGHTSOURCE_ENABLED == 1 && lightSourceLatchOn < 2 + lightSourceInvertPower )
				{
					if ( lightSourceInvertPower == 1 && lightSourceDelayCounter > 0 )
					{
						--lightSourceDelayCounter;
						if ( lightSourceDelayCounter != 0 )
						{
							return;
						}
					}
					else if ( lightSourceInvertPower == 0 && lightSourceDelay > 0 )
					{
						lightSourceDelayCounter = lightSourceDelay;
					}
					LIGHTSOURCE_ENABLED = 0;
					serverUpdateEntitySkill(this, 10);
					if ( lightSourceLatchOn > 0 )
					{
						++lightSourceLatchOn;
					}
				}
			}
		}
	}
	else
	{
		removeLightField();
		if ( multiplayer == CLIENT )
		{
			return;
		}

		if ( lightSourceAlwaysOn == 1 || (circuit_status == CIRCUIT_ON && !lightSourceInvertPower)
			|| (circuit_status == CIRCUIT_OFF && lightSourceInvertPower == 1) )
		{
			if ( LIGHTSOURCE_ENABLED == 0 && lightSourceLatchOn < 2 + lightSourceInvertPower )
			{
				if ( lightSourceInvertPower == 0 && lightSourceDelayCounter > 0 )
				{
					--lightSourceDelayCounter;
					if ( lightSourceDelayCounter != 0 )
					{
						return;
					}
				}
				else if ( lightSourceInvertPower == 1 && lightSourceDelay > 0 )
				{
					lightSourceDelayCounter = lightSourceDelay;
				}
				LIGHTSOURCE_ENABLED = 1;
				serverUpdateEntitySkill(this, 10);
				if ( lightSourceLatchOn > 0 )
				{
					++lightSourceLatchOn;
				}
			}
		}
	}
}
