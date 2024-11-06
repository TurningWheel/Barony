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
#include "engine/audio/sound.hpp"
#include "items.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "magic/magic.hpp"
#include "prng.hpp"
#include "mod_tools.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define SINK_AMBIENCE my->skill[7]
#define SINK_DISABLE_POLYMORPH_WASHING my->skill[8]

void actSink(Entity* my)
{
#ifdef USE_FMOD
	if ( SINK_AMBIENCE == 0 )
	{
		SINK_AMBIENCE--;
		my->stopEntitySound();
		my->entity_sound = playSoundEntityLocal(my, 149, 32);
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
	SINK_AMBIENCE--;
	if ( SINK_AMBIENCE <= 0 )
	{
		SINK_AMBIENCE = TICKS_PER_SECOND * 30;
		playSoundEntityLocal( my, 149, 32 );
	}
#endif

	if ( my->ticks == 1 )
	{
		my->createWorldUITooltip();
	}

	if ( my->skill[2] > 0 )
	{
		Entity* entity = spawnGib(my);
		entity->flags[INVISIBLE] = false;
		entity->x += .5;
		entity->z = my->z - 3;
		entity->flags[SPRITE] = false;
		entity->flags[NOUPDATE] = true;
		entity->flags[UPDATENEEDED] = false;
		entity->skill[4] = 6;
		entity->sprite = 4;
		entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
		entity->pitch = (local_rng.rand() % 360) * PI / 180.0;
		entity->roll = (local_rng.rand() % 360) * PI / 180.0;
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
		if ( selectedEntity[i] == my || client_selected[i] == my )
		{
			if (inrange[i])
			{
				auto& rng = my->entity_rng ? *my->entity_rng : local_rng;

				//First check that it's not depleted.
				if (my->skill[0] == 0)
				{
					messagePlayer(i, MESSAGE_INTERACTION, Language::get(580));
					playSoundEntity(my, 140 + local_rng.rand() % 2, 64);
				}
				else
				{
					if ( players[i]->entity->flags[BURNING] )
					{
						messagePlayer(i, MESSAGE_INTERACTION, Language::get(468));
						players[i]->entity->flags[BURNING] = false;
						serverUpdateEntityFlag(players[i]->entity, BURNING);
						steamAchievementClient(i, "BARONY_ACH_HOT_SHOWER");
					}
					if ( stats[i] && stats[i]->EFFECTS[EFF_POLYMORPH] && (SINK_DISABLE_POLYMORPH_WASHING == 0) )
					{
						if ( stats[i]->EFFECTS[EFF_POLYMORPH] )
						{
							players[i]->entity->setEffect(EFF_POLYMORPH, false, 0, true);
							players[i]->entity->effectPolymorph = 0;
							serverUpdateEntitySkill(players[i]->entity, 50);
							messagePlayer(i, MESSAGE_INTERACTION, Language::get(3192));
							if ( !stats[i]->EFFECTS[EFF_SHAPESHIFT] )
							{
								messagePlayer(i, MESSAGE_INTERACTION, Language::get(3185));
							}
							else
							{
								messagePlayer(i, MESSAGE_INTERACTION, Language::get(4303));  // wears out, no mention of 'normal' form
							}
						}

						playSoundEntity(players[i]->entity, 400, 92);
						createParticleDropRising(players[i]->entity, 593, 1.f);
						serverSpawnMiscParticles(players[i]->entity, PARTICLE_EFFECT_RISING_DROP, 593);
					}
					switch (my->skill[3])
					{
						case 0:
						{
							//playSoundEntity(players[i]->entity, 52, 64);
							messagePlayer(i, MESSAGE_INTERACTION, Language::get(581));
							Compendium_t::Events_t::eventUpdateWorld(i, Compendium_t::CPDM_SINKS_USED, "sink", 1);
							//Randomly choose a ring.
							//88-99 are rings.
							//So 12 rings total.
							int ring = rng.rand() % 12 + (int)(RING_ADORNMENT); //Generate random number between 0 & 11, then add 88 to it so that it's at the location of the rings.

							//Generate a random status.
							Status status = SERVICABLE;
							int status_rand = rng.rand() % 4;
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
							int beatitude = rng.rand() % 5 - 2; //No item will be able to generate with less than -2 or more than +2 beatitude

							//Actually create the item, put it in the player's inventory, and then free the memory of the temp item.
							Item* item = newItem(static_cast<ItemType>(ring), static_cast<Status>(status), beatitude, 1, rng.rand(), false, NULL);
							if (item)
							{
								itemPickup(i, item);
								messagePlayer(i, MESSAGE_INTERACTION | MESSAGE_INVENTORY, Language::get(504), item->description());
								free(item);
								Compendium_t::Events_t::eventUpdateWorld(i, Compendium_t::CPDM_SINKS_RINGS, "sink", 1);
							}
							break;
						}
						case 1:
						{
							//playSoundEntity(players[i]->entity, 52, 64);
							Compendium_t::Events_t::eventUpdateWorld(i, Compendium_t::CPDM_SINKS_USED, "sink", 1);
							// spawn slime
							Entity* monster = summonMonster(SLIME, my->x, my->y);
							if ( monster )
							{
								monster->seedEntityRNG(rng.getU32());
								slimeSetType(monster, monster->getStats(), true, &rng);
								Uint32 color = makeColorRGB(255, 128, 0);
								messagePlayerColor(i, MESSAGE_HINT, color, Language::get(582));
								Compendium_t::Events_t::eventUpdateWorld(i, Compendium_t::CPDM_SINKS_SLIMES, "sink", 1);
							}
							break;
						}
						case 2:
						{
							if ( !stats[i] )
							{
								break;
							}
							Compendium_t::Events_t::eventUpdateWorld(i, Compendium_t::CPDM_SINKS_USED, "sink", 1);
							if ( stats[i]->type == AUTOMATON )
							{
								Uint32 color = makeColorRGB(255, 128, 0);
								messagePlayerColor(i, MESSAGE_STATUS, color, Language::get(3700));
								playSoundEntity(players[i]->entity, 52, 64);
								stats[i]->HUNGER -= 200; //Lose boiler
								players[i]->entity->modMP(5 + local_rng.rand() % 6); //Raise temperature because steam.
								serverUpdateHunger(i);
							}
							else if ( stats[i]->type != VAMPIRE )
							{
								messagePlayer(i, MESSAGE_INTERACTION, Language::get(583));
								playSoundEntity(players[i]->entity, 52, 64);
								if ( stats[i]->type != SKELETON )
								{
									stats[i]->HUNGER += 50; //Less nutrition than the refreshing fountain.
									serverUpdateHunger(i);
								}
								players[i]->entity->modHP(1);
								Compendium_t::Events_t::eventUpdateWorld(i, Compendium_t::CPDM_SINKS_HEALTH_RESTORED, "sink", 1);
							}
							else
							{
								players[i]->entity->modHP(-2);
								Compendium_t::Events_t::eventUpdateWorld(i, Compendium_t::CPDM_SINKS_HEALTH_RESTORED, "sink", -2);
								playSoundEntity(players[i]->entity, 28, 64);
								playSoundEntity(players[i]->entity, 249, 128);
								players[i]->entity->setObituary(Language::get(1533));
						        stats[i]->killer = KilledBy::SINK;

								Uint32 color = makeColorRGB(255, 0, 0);
								messagePlayerColor(i, MESSAGE_STATUS, color, Language::get(3183));
								if ( i >= 0 && players[i]->isLocalPlayer() )
								{
									cameravars[i].shakex += .1;
									cameravars[i].shakey += 10;
								}
								else if ( multiplayer == SERVER && i > 0 && !players[i]->isLocalPlayer() )
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
							if ( !stats[i] )
							{
								break;
							}
							Compendium_t::Events_t::eventUpdateWorld(i, Compendium_t::CPDM_SINKS_USED, "sink", 1);
							if ( stats[i]->type == AUTOMATON )
							{
								Uint32 color = makeColorRGB(255, 128, 0);
								messagePlayerColor(i, MESSAGE_STATUS, color, Language::get(3701));
								playSoundEntity(players[i]->entity, 52, 64);
								stats[i]->HUNGER += 200; //Gain boiler
								players[i]->entity->modMP(2);
								serverUpdateHunger(i);
								break;
							}
							else
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
								Compendium_t::Events_t::eventUpdateWorld(i, Compendium_t::CPDM_SINKS_HEALTH_RESTORED, "sink", -2);
								playSoundEntity(players[i]->entity, 28, 64);
								players[i]->entity->setObituary(Language::get(1533));
						        stats[i]->killer = KilledBy::SINK;

								Uint32 color = makeColorRGB(255, 0, 0);
								messagePlayerColor(i, MESSAGE_STATUS, color, Language::get(584));

								if ( i >= 0 && players[i]->isLocalPlayer() )
								{
									cameravars[i].shakex += .1;
									cameravars[i].shakey += 10;
								}
								else if ( multiplayer == SERVER && i > 0 && !players[i]->isLocalPlayer() )
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
						int effect = rng.rand() % 10; //4 possible effects.
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
						messagePlayer(i, MESSAGE_INTERACTION, Language::get(585));
						playSoundEntity(my, 132, 64);
					}
					serverUpdateEntitySkill(my, 0);
				}
			}
		}
	}
}
