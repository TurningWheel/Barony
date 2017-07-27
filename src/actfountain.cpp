/*-------------------------------------------------------------------------------

	BARONY
	File: actfountain.cpp
	Desc: behavior function for fountains

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

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

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
	if ( my->skill[0] > 0 || ( !my->skill[2] && multiplayer == CLIENT ) )
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

	// makes the fountain stop spraying water on clients
	if ( my->skill[0] <= 0 )
	{
		my->skill[2] = 1;
	}
	else
	{
		my->skill[2] = 0;
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
						if (i > 0)
						{
							serverUpdateEntityFlag(players[i]->entity, BURNING);
						}
					}
					switch (my->skill[1])
					{
						case 0:
						{
							playSoundEntity(players[i]->entity, 52, 64);

							//Spawn succubus.
							Uint32 color = SDL_MapRGB(mainsurface->format, 255, 128, 0);
							messagePlayerColor(i, color, language[469]);
							summonMonster(SUCCUBUS, my->x, my->y);
							break;
						}
						case 1:
							messagePlayer(i, language[470]);
							messagePlayer(i, language[471]);
							playSoundEntity(players[i]->entity, 52, 64);
							stats[i]->HUNGER += 50;
							break;
						case 2:
						{
							//Potion effect. Potion effect is stored in my->skill[3], randomly chosen when the fountain is created.
							messagePlayer(i, language[470]);
							Item* item = newItem(static_cast<ItemType>(POTION_WATER + my->skill[3]), static_cast<Status>(4), 0, 1, 0, false, NULL);
							useItem(item, i);
							// Long live the mystical fountain of TODO.
							break;
						}
						case 3:
						{
							// bless equipment
							playSoundEntity(players[i]->entity, 52, 64);
							Uint32 textcolor = SDL_MapRGB(mainsurface->format, 0, 255, 255);
							messagePlayerColor(i, textcolor, language[471]);
							messagePlayer(i, language[473]);
							if ( stats[i]->helmet )
							{
								stats[i]->helmet->beatitude++;
							}
							if ( stats[i]->breastplate )
							{
								stats[i]->breastplate->beatitude++;
							}
							if ( stats[i]->gloves )
							{
								stats[i]->gloves->beatitude++;
							}
							if ( stats[i]->shoes )
							{
								stats[i]->shoes->beatitude++;
							}
							if ( stats[i]->shield )
							{
								stats[i]->shield->beatitude++;
							}
							if ( stats[i]->weapon )
							{
								stats[i]->weapon->beatitude++;
							}
							if ( stats[i]->cloak )
							{
								stats[i]->cloak->beatitude++;
							}
							if ( stats[i]->amulet )
							{
								stats[i]->amulet->beatitude++;
							}
							if ( stats[i]->ring )
							{
								stats[i]->ring->beatitude++;
							}
							if ( stats[i]->mask )
							{
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
							break;
						}
						default:
							break;
					}
					messagePlayer(i, language[474]);
					my->skill[0] = 0; //Dry up fountain.
					//TODO: messagePlayersInSight() instead.
				}
				//Then perform the effect randomly determined when the fountain was created.
				return;
			}
		}
	}
}
