/*-------------------------------------------------------------------------------

	BARONY
	File: actsink.cpp
	Desc: behavior function for sinks

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
#include "player.hpp"
#include "magic/magic.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define SINK_AMBIENCE my->skill[7]

void actSink(Entity* my)
{
	SINK_AMBIENCE--;
	if ( SINK_AMBIENCE <= 0 )
	{
		SINK_AMBIENCE = TICKS_PER_SECOND * 30;
		playSoundEntityLocal( my, 149, 128 );
	}

	if ( my->skill[2] > 0 )
	{
		Entity* entity = spawnGib(my);
		entity->flags[INVISIBLE] = false;
		entity->x += .5;
		entity->z -= 3;
		entity->flags[SPRITE] = false;
		entity->flags[NOUPDATE] = true;
		entity->flags[UPDATENEEDED] = false;
		entity->skill[4] = 6;
		entity->sprite = 4;
		entity->yaw = (rand() % 360) * PI / 180.0;
		entity->pitch = (rand() % 360) * PI / 180.0;
		entity->roll = (rand() % 360) * PI / 180.0;
		entity->vel_x = 0;
		entity->vel_y = 0;
		entity->vel_z = .25;
		entity->fskill[3] = 0.03;

		if ( multiplayer != CLIENT )
		{
			my->skill[2]--;
		}
	}

	if ( multiplayer == CLIENT )
	{
		return;
	}

	//Using the sink. //TODO: Monsters using it?
	int i;
	for (i = 0; i < MAXPLAYERS; ++i)
	{
		if ( (i == 0 && selectedEntity == my) || (client_selected[i] == my) )
		{
			if (inrange[i])
			{
				//First check that it's not depleted.
				if (my->skill[0] == 0)
				{
					messagePlayer(i, language[580]);
					playSoundEntity(my, 140 + rand() % 2, 64);
				}
				else
				{
					if ( players[i]->entity->flags[BURNING] )
					{
						messagePlayer(i, language[468]);
						players[i]->entity->flags[BURNING] = false;
						serverUpdateEntityFlag(players[i]->entity, BURNING);
						steamAchievementClient(i, "BARONY_ACH_HOT_SHOWER");
					}
					if ( stats[i] && stats[i]->EFFECTS[EFF_POLYMORPH] )
					{
						players[i]->entity->setEffect(EFF_POLYMORPH, false, 0, true);
						players[i]->entity->effectPolymorph = 0;
						serverUpdateEntitySkill(players[i]->entity, 50);

						messagePlayer(i, language[3192]);
						messagePlayer(i, language[3185]);

						playSoundEntity(players[i]->entity, 400, 92);
						createParticleDropRising(players[i]->entity, 593, 1.f);
						serverSpawnMiscParticles(players[i]->entity, PARTICLE_EFFECT_RISING_DROP, 593);
					}
					switch (my->skill[3])
					{
						case 0:
						{
							//playSoundEntity(players[i]->entity, 52, 64);
							messagePlayer(i, language[581]);

							//Randomly choose a ring.
							//88-99 are rings.
							//So 12 rings total.
							int ring = rand() % 12 + (int)(RING_ADORNMENT); //Generate random number between 0 & 11, then add 88 to it so that it's at the location of the rings.

							//Generate a random status.
							Status status = SERVICABLE;
							int status_rand = rand() % 4;
							switch (status_rand)
							{
								case 0:
									status = DECREPIT;
									break;
								case 1:
									status = WORN;
									break;
								case 2:
									status = SERVICABLE;
									break;
								case 3:
									status = EXCELLENT;
									break;
								default:
									status = SERVICABLE;
									break;
							}
							//Random beatitude (third parameter).
							int beatitude = rand() % 5 - 2; //No item will be able to generate with less than -2 or more than +2 beatitude

							//Actually create the item, put it in the player's inventory, and then free the memory of the temp item.
							Item* item = newItem(static_cast<ItemType>(ring), static_cast<Status>(status), beatitude, 1, rand(), false, NULL);
							if (item)
							{
								itemPickup(i, item);
								messagePlayer(i, language[504], item->description());
								free(item);
							}
							break;
						}
						case 1:
						{
							//playSoundEntity(players[i]->entity, 52, 64);

							// spawn slime
							Entity* monster = summonMonster(SLIME, my->x, my->y);
							if ( monster )
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 255, 128, 0);
								messagePlayerColor(i, color, language[582]);
								Stat* monsterStats = monster->getStats();
								monsterStats->LVL = 4;
								monster->sprite = 210;
								monster->flags[INVISIBLE] = false;
							}
							break;
						}
						case 2:
						{
							if ( stats[i]->type != VAMPIRE )
							{
								messagePlayer(i, language[583]);
								playSoundEntity(players[i]->entity, 52, 64);
								stats[i]->HUNGER += 50; //Less nutrition than the refreshing fountain.
								players[i]->entity->modHP(1);
							}
							else
							{
								players[i]->entity->modHP(-2);
								playSoundEntity(players[i]->entity, 28, 64);
								playSoundEntity(players[i]->entity, 249, 128);
								players[i]->entity->setObituary(language[1533]);

								Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
								messagePlayerColor(i, color, language[3183]);
								if ( i == 0 )
								{
									camera_shakex += .1;
									camera_shakey += 10;
								}
								else if ( multiplayer == SERVER && i > 0 )
								{
									strcpy((char*)net_packet->data, "SHAK");
									net_packet->data[4] = 10; // turns into .1
									net_packet->data[5] = 10;
									net_packet->address.host = net_clients[i - 1].host;
									net_packet->address.port = net_clients[i - 1].port;
									net_packet->len = 6;
									sendPacketSafe(net_sock, -1, net_packet, i - 1);
								}
							}
							break;
						}
						case 3:
						{
							if ( stats[i]->type != VAMPIRE )
							{
								players[i]->entity->modHP(-2);
							}
							else
							{
								players[i]->entity->modHP(-2);
								playSoundEntity(players[i]->entity, 249, 128);
							}
							playSoundEntity(players[i]->entity, 28, 64);
							players[i]->entity->setObituary(language[1533]);

							Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
							messagePlayerColor(i, color, language[584]);

							if ( i == 0 )
							{
								camera_shakex += .1;
								camera_shakey += 10;
							}
							else if ( multiplayer == SERVER && i > 0 )
							{
								strcpy((char*)net_packet->data, "SHAK");
								net_packet->data[4] = 10; // turns into .1
								net_packet->data[5] = 10;
								net_packet->address.host = net_clients[i - 1].host;
								net_packet->address.port = net_clients[i - 1].port;
								net_packet->len = 6;
								sendPacketSafe(net_sock, -1, net_packet, i - 1);
							}
							break;
						}
						default:
							break;
					}

					// run the water particles
					my->skill[2] = TICKS_PER_SECOND / 2;

					//Deduct one usage from it.
					if (my->skill[0] > 1)   //First usage. Will create second stats now.
					{
						my->skill[0]--; //Deduct one usage.

						//Randomly choose second usage stats.
						int effect = rand() % 10; //4 possible effects.
						switch (effect)
						{
							case 0:
								//10% chance.
								my->skill[3] = 0; //Player will find a ring.
							case 1:
								//10% chance.
								my->skill[3] = 1; //Will spawn a slime.
								break;
							case 2:
							case 3:
							case 4:
							case 5:
							case 6:
							case 7:
								//60% chance.
								my->skill[3] = 2; //Will raise nutrition.
								break;
							case 8:
							case 9:
								//20% chance.
								my->skill[3] = 3; //Player will lose 1 HP.
								break;
							default:
								break; //Should never happen.
						}
					}
					else     //Second usage.
					{
						my->skill[0]--; //Sink is depleted!
						messagePlayer(i, language[585]);
						playSoundEntity(my, 132, 64);
					}
					serverUpdateEntitySkill(my, 0);
				}
			}
		}
	}
}
