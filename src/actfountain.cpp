/*-------------------------------------------------------------------------------

	BARONY
	File: actfountain.cpp
	Desc: behavior function for fountains

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include <utility>
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
#include "colors.hpp"
#include "scores.hpp"

//Fountain functions.
const std::vector<int> fountainPotionDropChances =
{
	5,	//POTION_WATER,
	20,	//POTION_BOOZE,
	10,	//POTION_JUICE,
	10,	//POTION_SICKNESS,
	5,	//POTION_CONFUSION,
	2,	//POTION_EXTRAHEALING,
	5,	//POTION_HEALING,
	5,	//POTION_CUREAILMENT,
	10,	//POTION_BLINDNESS,
	5,	//POTION_RESTOREMAGIC,
	2,	//POTION_INVISIBILITY,
	2,	//POTION_LEVITATION,
	5,	//POTION_SPEED,
	10,	//POTION_ACID,
	2,	//POTION_PARALYSIS,
	2	//POTION_POLYMORPH
};

const std::vector<std::pair<int, int>> potionStandardAppearanceMap =
{
	// second element is appearance.
	{ POTION_WATER, 0 },
	{ POTION_BOOZE, 2 },
	{ POTION_JUICE, 3 },
	{ POTION_SICKNESS, 1 },
	{ POTION_CONFUSION, 0 },
	{ POTION_EXTRAHEALING, 0 },
	{ POTION_HEALING, 0 },
	{ POTION_CUREAILMENT, 0 },
	{ POTION_BLINDNESS, 0 },
	{ POTION_RESTOREMAGIC, 1 },
	{ POTION_INVISIBILITY, 0 },
	{ POTION_LEVITATION, 0 },
	{ POTION_SPEED, 0 },
	{ POTION_ACID, 0 },
	{ POTION_PARALYSIS, 1 },
	{ POTION_POLYMORPH, 0 },
	{ POTION_FIRESTORM, 0 },
	{ POTION_ICESTORM, 0 },
	{ POTION_THUNDERSTORM, 0 },
	{ POTION_STRENGTH, 0 }
};

std::mt19937 fountainSeed(rand());
std::discrete_distribution<> fountainDistribution(fountainPotionDropChances.begin(), fountainPotionDropChances.end());

std::pair<int, int> fountainGeneratePotionDrop()
{
	auto keyPair = potionStandardAppearanceMap.at(fountainDistribution(fountainSeed));
	return std::make_pair(keyPair.first, keyPair.second);
}

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

	my->skill[0] is either 0 or 1. If it is 0, the fountain is dry and cannot be used
	my->skill[1] is either 0, 1, 2, or 3. It is set at the creation of the fountain.
		Those values correspond to what the fountain does:
		0 = spawn succubus, 1 = raise hunger, 2 = random potion effect, 3 = bless equipment
	my->skill[3] is a random potion effect. It is set at the creation of the fountain

-------------------------------------------------------------------------------*/

void actFountain(Entity* my)
{
	Entity* entity;

	//messagePlayer(0, "actFountain()");
	//TODO: Temporary mechanism testing code.
	/*
	if( multiplayer != CLIENT ) {
		if (my->skill[28]) {
			//All it does is change its sprite to sink if it's powered.
			if (my->skill[28] == 1) {
				my->sprite = 163;
			} else {
				my->sprite = 164;
			}
		}
	}*/
	//****************END TEST CODE***************

	//TODO: Sounds.

	// spray water
	if ( my->skill[0] > 0 )
	{
#define FOUNTAIN_AMBIENCE my->skill[7]
		FOUNTAIN_AMBIENCE--;
		if ( FOUNTAIN_AMBIENCE <= 0 )
		{
			FOUNTAIN_AMBIENCE = TICKS_PER_SECOND * 6;
			playSoundEntityLocal(my, 135, 32 );
		}
		entity = spawnGib(my);
		entity->flags[INVISIBLE] = false;
		entity->y -= 2;
		entity->z -= 8;
		entity->flags[SPRITE] = false;
		entity->flags[NOUPDATE] = true;
		entity->flags[UPDATENEEDED] = false;
		entity->skill[4] = 7;
		entity->sprite = 4;
		entity->yaw = (rand() % 360) * PI / 180.0;
		entity->pitch = (rand() % 360) * PI / 180.0;
		entity->roll = (rand() % 360) * PI / 180.0;
		entity->vel_x = 0;
		entity->vel_y = 0;
		entity->vel_z = .25;
		entity->fskill[3] = 0.03;
	}

	// the rest of the function is server-side.
	if ( multiplayer == CLIENT )
	{
		return;
	}

	//Using the fountain (TODO: Monsters using it?).
	int i;
	for (i = 0; i < MAXPLAYERS; ++i)
	{
		if ( (i == 0 && selectedEntity == my) || (client_selected[i] == my) )
		{
			if (inrange[i])   //Act on it only if the player (or monster, if/when this is changed to support monster interaction?) is in range.
			{
				//First check that it's not depleted.
				if (my->skill[0] == 0)
				{
					//Depleted
					messagePlayer(i, language[467]);
				}
				else
				{
					if (players[i]->entity->flags[BURNING])
					{
						messagePlayer(i, language[468]);
						players[i]->entity->flags[BURNING] = false;
						serverUpdateEntityFlag(players[i]->entity, BURNING);
						steamAchievementClient(i, "BARONY_ACH_HOT_SHOWER");
					}
					int potionDropQuantity = 0;
					if ( stats[i] && (stats[i]->type == GOATMAN || stats[i]->playerRace == RACE_GOATMAN) && stats[i]->appearance == 0 )
					{
						// drop some random potions.
						switch ( rand() % 10 )
						{
							case 0:
							case 1:
							case 2:
							case 3:
								potionDropQuantity = 1;
								break;
							case 4:
							case 5:
								potionDropQuantity = 2;
								break;
							case 6:
								potionDropQuantity = 3;
								break;
							case 7:
							case 8:
							case 9:
								// nothing
								potionDropQuantity = 0;
								break;
							default:
								break;
						}

						if ( potionDropQuantity > 0 )
						{
							steamStatisticUpdateClient(i, STEAM_STAT_BOTTLE_NOSED, STEAM_STAT_INT, 1);
						}

						for ( int j = 0; j < potionDropQuantity; ++j )
						{
							std::pair<int, int> generatedPotion = fountainGeneratePotionDrop();
							ItemType type = static_cast<ItemType>(generatedPotion.first);
							int appearance = generatedPotion.second;
							Item* item = newItem(type, EXCELLENT, 0, 1, appearance, false, NULL);
							Entity* dropped = dropItemMonster(item, my, NULL);
							dropped->yaw = ((0 + rand() % 360) / 180.f) * PI;
							dropped->vel_x = (0.75 + .025 * (rand() % 11)) * cos(dropped->yaw);
							dropped->vel_y = (0.75 + .025 * (rand() % 11)) * sin(dropped->yaw);
							dropped->vel_z = (-10 - rand() % 20) * .01;
							dropped->flags[USERFLAG1] = false;
						}
					}
					switch (my->skill[1])
					{
						case 0:
						{
							playSoundEntity(players[i]->entity, 52, 64);
							//Spawn succubus.
							Uint32 color = SDL_MapRGB(mainsurface->format, 255, 128, 0);
							Entity* spawnedMonster = nullptr;

							if ( !strncmp(map.name, "Underworld", 10) )
							{
								Monster creature = SUCCUBUS;
								if ( rand() % 2 )
								{
									creature = INCUBUS;
								}
								for ( int c = 0; spawnedMonster == nullptr && c < 5; ++c )
								{
									switch ( c )
									{
										case 0:
											spawnedMonster = summonMonster(creature, my->x, my->y);
											break;
										case 1:
											spawnedMonster = summonMonster(creature, my->x + 16, my->y);
											break;
										case 2:
											spawnedMonster = summonMonster(creature, my->x - 16, my->y);
											break;
										case 3:
											spawnedMonster = summonMonster(creature, my->x, my->y + 16);
											break;
										case 4:
											spawnedMonster = summonMonster(creature, my->x, my->y - 16);
											break;
									}
								}
								if ( spawnedMonster )
								{
									if ( creature == INCUBUS )
									{
										messagePlayerColor(i, color, language[2519]);
										Stat* tmpStats = spawnedMonster->getStats();
										if ( tmpStats )
										{
											strcpy(tmpStats->name, "lesser incubus");
										}
									}
									else
									{
										messagePlayerColor(i, color, language[469]);
									}
								}
							}
							else if ( currentlevel < 10 )
							{
								messagePlayerColor(i, color, language[469]);
								spawnedMonster = summonMonster(SUCCUBUS, my->x, my->y);
							}
							else if ( currentlevel < 20 )
							{
								if ( rand() % 2 )
								{
									spawnedMonster = summonMonster(INCUBUS, my->x, my->y);
									Stat* tmpStats = spawnedMonster->getStats();
									if ( tmpStats )
									{
										strcpy(tmpStats->name, "lesser incubus");
									}
									messagePlayerColor(i, color, language[2519]);
								}
								else
								{
									messagePlayerColor(i, color, language[469]);
									spawnedMonster = summonMonster(SUCCUBUS, my->x, my->y);
								}
							}
							else
							{
								messagePlayerColor(i, color, language[2519]);
								spawnedMonster = summonMonster(INCUBUS, my->x, my->y);
							}
							break;
						}
						case 1:
							if ( stats[i]->type != VAMPIRE )
							{
								messagePlayer(i, language[470]);
								messagePlayer(i, language[471]);
								playSoundEntity(players[i]->entity, 52, 64);
								stats[i]->HUNGER += 100;
								players[i]->entity->modHP(5);
							}
							else
							{
								players[i]->entity->modHP(-3);
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
						case 2:
						{
							//Potion effect. Potion effect is stored in my->skill[3], randomly chosen when the fountain is created.
							messagePlayer(i, language[470]);
							Item* item = newItem(static_cast<ItemType>(POTION_WATER + my->skill[3]), static_cast<Status>(4), 0, 1, 0, false, NULL);
							useItem(item, i, my);
							// Long live the mystical fountain of TODO.
							break;
						}
						case 3:
						{
							// bless all equipment
							playSoundEntity(players[i]->entity, 52, 64);
							//playSoundEntity(players[i]->entity, 167, 64);
							Uint32 textcolor = SDL_MapRGB(mainsurface->format, 0, 255, 255);
							messagePlayerColor(i, textcolor, language[471]);
							messagePlayerColor(i, textcolor, language[473]);
							bool stuckOnYouSuccess = false;
							if ( stats[i]->helmet )
							{
								if ( stats[i]->type == SUCCUBUS && stats[i]->helmet->beatitude == 0 )
								{
									stuckOnYouSuccess = true;
								}
								stats[i]->helmet->beatitude++;
							}
							if ( stats[i]->breastplate )
							{
								if ( stats[i]->type == SUCCUBUS && stats[i]->breastplate->beatitude == 0 )
								{
									stuckOnYouSuccess = true;
								}
								stats[i]->breastplate->beatitude++;
							}
							if ( stats[i]->gloves )
							{
								if ( stats[i]->type == SUCCUBUS && stats[i]->gloves->beatitude == 0 )
								{
									stuckOnYouSuccess = true;
								}
								stats[i]->gloves->beatitude++;
							}
							if ( stats[i]->shoes )
							{
								if ( stats[i]->type == SUCCUBUS && stats[i]->shoes->beatitude == 0 )
								{
									stuckOnYouSuccess = true;
								}
								stats[i]->shoes->beatitude++;
							}
							if ( stats[i]->shield )
							{
								if ( stats[i]->type == SUCCUBUS && stats[i]->shield->beatitude == 0 )
								{
									stuckOnYouSuccess = true;
								}
								stats[i]->shield->beatitude++;
							}
							if ( stats[i]->weapon )
							{
								if ( stats[i]->type == SUCCUBUS && stats[i]->weapon->beatitude == 0 )
								{
									stuckOnYouSuccess = true;
								}
								stats[i]->weapon->beatitude++;
							}
							if ( stats[i]->cloak )
							{
								if ( stats[i]->type == SUCCUBUS && stats[i]->cloak->beatitude == 0 )
								{
									stuckOnYouSuccess = true;
								}
								stats[i]->cloak->beatitude++;
							}
							if ( stats[i]->amulet )
							{
								if ( stats[i]->type == SUCCUBUS && stats[i]->amulet->beatitude == 0 )
								{
									stuckOnYouSuccess = true;
								}
								stats[i]->amulet->beatitude++;
							}
							if ( stats[i]->ring )
							{
								if ( stats[i]->type == SUCCUBUS && stats[i]->ring->beatitude == 0 )
								{
									stuckOnYouSuccess = true;
								}
								stats[i]->ring->beatitude++;
							}
							if ( stats[i]->mask )
							{
								if ( stats[i]->type == SUCCUBUS && stats[i]->mask->beatitude == 0 )
								{
									stuckOnYouSuccess = true;
								}
								stats[i]->mask->beatitude++;
							}
							if ( multiplayer == SERVER && i > 0 )
							{
								strcpy((char*)net_packet->data, "BLES");
								net_packet->address.host = net_clients[i - 1].host;
								net_packet->address.port = net_clients[i - 1].port;
								net_packet->len = 4;
								sendPacketSafe(net_sock, -1, net_packet, i - 1);
							}
							if ( stuckOnYouSuccess )
							{
								steamAchievementClient(i, "BARONY_ACH_STUCK_ON_YOU");
							}
							break;
						}
						case 4:
						{
							// bless one piece of equipment
							playSoundEntity(players[i]->entity, 52, 64);
							//playSoundEntity(players[i]->entity, 167, 64);
							Uint32 textcolor = SDL_MapRGB(mainsurface->format, 0, 255, 255);
							messagePlayerColor(i, textcolor, language[471]);
							//Choose only one piece of equipment to bless.

							//First, Figure out what equipment is available.
							std::vector<std::pair<Item*, Uint32>> items;
							if ( stats[i]->helmet )
							{
								items.push_back(std::pair<Item*,int>(stats[i]->helmet, 0));
							}
							if ( stats[i]->breastplate )
							{
								items.push_back(std::pair<Item*,int>(stats[i]->breastplate, 1));
							}
							if ( stats[i]->gloves )
							{
								items.push_back(std::pair<Item*,int>(stats[i]->gloves, 2));
							}
							if ( stats[i]->shoes )
							{
								items.push_back(std::pair<Item*,int>(stats[i]->shoes, 3));
							}
							if ( stats[i]->shield )
							{
								items.push_back(std::pair<Item*,int>(stats[i]->shield, 4));
							}
							if ( stats[i]->weapon && stats[i]->weapon->type != POTION_EMPTY )
							{
								items.push_back(std::pair<Item*,int>(stats[i]->weapon, 5));
							}
							if ( stats[i]->cloak )
							{
								items.push_back(std::pair<Item*,int>(stats[i]->cloak, 6));
							}
							if ( stats[i]->amulet )
							{
								items.push_back(std::pair<Item*,int>(stats[i]->amulet, 7));
							}
							if ( stats[i]->ring )
							{
								items.push_back(std::pair<Item*,int>(stats[i]->ring, 8));
							}
							if ( stats[i]->mask )
							{
								items.push_back(std::pair<Item*,int>(stats[i]->mask, 9));
							}

							if ( items.size() )
							{
								messagePlayerColor(i, textcolor, language[2592]); //"The fountain blesses a piece of equipment"
								//Randomly choose a piece of equipment.
								std::pair<Item*, Uint32> chosen = items[rand()%items.size()];
								if ( chosen.first->beatitude == 0 )
								{
									if ( stats[i]->type == SUCCUBUS )
									{
										steamAchievementClient(i, "BARONY_ACH_STUCK_ON_YOU");
									}
								}
								chosen.first->beatitude++;

								if ( multiplayer == SERVER && i > 0 )
								{
									strcpy((char*)net_packet->data, "BLE1");
									SDLNet_Write32(chosen.second, &net_packet->data[4]);
									net_packet->address.host = net_clients[i - 1].host;
									net_packet->address.port = net_clients[i - 1].port;
									net_packet->len = 8;
									sendPacketSafe(net_sock, -1, net_packet, i - 1);
								}
							}
							//Does nothing if no valid items.
							break;
						}
						default:
							break;
					}
					if ( potionDropQuantity > 0 )
					{
						playSoundEntity(my, 47 + rand() % 3, 64);
					}
					if ( potionDropQuantity > 1 )
					{
						messagePlayerColor(i, uint32ColorGreen(*mainsurface), language[3245], potionDropQuantity);
					}
					else if ( potionDropQuantity == 1 )
					{
						messagePlayerColor(i, uint32ColorGreen(*mainsurface), language[3246]);
					}
					messagePlayer(i, language[474]);
					my->skill[0] = 0; //Dry up fountain.
					serverUpdateEntitySkill(my, 0);
					//TODO: messagePlayersInSight() instead.
				}
				//Then perform the effect randomly determined when the fountain was created.
				return;
			}
		}
	}
}
