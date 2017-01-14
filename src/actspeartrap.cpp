/*-------------------------------------------------------------------------------

	BARONY
	File: actspeartrap.cpp
	Desc: implements spear trap code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "sound.hpp"
#include "net.hpp"
#include "collision.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define SPEARTRAP_INIT my->skill[0]
#define SPEARTRAP_AMBIENCE my->skill[1]
#define SPEARTRAP_STATUS my->skill[3]
#define SPEARTRAP_OUTTIME my->skill[4]
#define SPEARTRAP_STARTHEIGHT my->fskill[0]
#define SPEARTRAP_VELZ my->vel_z

void actSpearTrap(Entity *my) {
	SPEARTRAP_AMBIENCE--;
	if( SPEARTRAP_AMBIENCE<=0 ) {
		SPEARTRAP_AMBIENCE = TICKS_PER_SECOND*30;
		playSoundEntityLocal( my, 149, 128 );
	}

	if( multiplayer!=CLIENT ) {
		if (!my->skill[28]) {
			return;
		}

		if (my->skill[28] == 2) {
			// shoot out the spears
			if (!SPEARTRAP_STATUS ) {
				SPEARTRAP_STATUS = 1;
				SPEARTRAP_OUTTIME = 0;
				playSoundEntity(my, 82, 64);
				serverUpdateEntitySkill(my,3);
				serverUpdateEntitySkill(my,4);
			}
		} else {
			// retract the spears
			if( SPEARTRAP_STATUS ) {
				SPEARTRAP_STATUS = 0;
				if( SPEARTRAP_OUTTIME<=60 ) {
					playSoundEntity(my, 82, 64);
				}
				SPEARTRAP_OUTTIME = 0;
				serverUpdateEntitySkill(my,3);
				serverUpdateEntitySkill(my,4);
			}
		}
	} else {
		my->flags[NOUPDATE] = TRUE;
	}
	if( !SPEARTRAP_INIT ) {
		SPEARTRAP_INIT = 1;
		SPEARTRAP_STARTHEIGHT = my->z;
	}

	if( !SPEARTRAP_STATUS || SPEARTRAP_OUTTIME > 60 ) {
		// retract spears
		if( my->z < SPEARTRAP_STARTHEIGHT ) {
			SPEARTRAP_VELZ += .25;
			my->z = std::min(SPEARTRAP_STARTHEIGHT, my->z + SPEARTRAP_VELZ);
		} else {
			SPEARTRAP_VELZ = 0;
		}
	} else {
		// shoot out spears
		my->z = fmax(SPEARTRAP_STARTHEIGHT-20, my->z - 4);
		if( multiplayer != CLIENT ) {
			SPEARTRAP_OUTTIME++;
			if( SPEARTRAP_OUTTIME > 60 ) {
				playSoundEntity(my, 82, 64);
				serverUpdateEntitySkill(my,4);
			} else if( SPEARTRAP_OUTTIME==1 ) {
				node_t *node;
				for( node=map.entities->first; node!=NULL; node=node->next ) {
					Entity *entity = (Entity *)node->element;
					if( entity->behavior==&actPlayer || entity->behavior==&actMonster ) {
						Stat *stats = entity->getStats();
						if( stats ) {
							if( entityInsideEntity(my,entity) ) {
								// do damage!
								if( entity->behavior==&actPlayer ) {
									Uint32 color = SDL_MapRGB(mainsurface->format,255,0,0);
									messagePlayerColor(entity->skill[2],color,language[586]);
									if( entity->skill[2] == clientnum ) {
										camera_shakex += .1;
										camera_shakey += 10;
									} else {
										strcpy((char *)net_packet->data,"SHAK");
										net_packet->data[4]=10; // turns into .1
										net_packet->data[5]=10;
										net_packet->address.host = net_clients[entity->skill[2]-1].host;
										net_packet->address.port = net_clients[entity->skill[2]-1].port;
										net_packet->len = 6;
										sendPacketSafe(net_sock, -1, net_packet, entity->skill[2]-1);
									}
								}
								playSoundEntity(entity,28,64);
								spawnGib(entity);
								entity->modHP(-50);

								// set obituary
								entity->setObituary(language[1507]);
							}
						}
					}
				}
			}
		}
	}
}