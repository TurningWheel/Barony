/*-------------------------------------------------------------------------------

	BARONY
	File: actbeartrap.cpp
	Desc: behavior function for set beartraps

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "messages.hpp"
#include "sound.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define BEARTRAP_CAUGHT my->skill[0]
#define BEARTRAP_STATUS my->skill[11]
#define BEARTRAP_BEATITUDE my->skill[12]
#define BEARTRAP_APPEARANCE my->skill[14]
#define BEARTRAP_IDENTIFIED my->skill[15]

void actBeartrap(Entity* my) {
	int i;

	// undo beartrap
	for (i = 0; i < MAXPLAYERS; i++) {
		if ( (i == 0 && selectedEntity == my) || (client_selected[i] == my) ) {
			if (inrange[i]) {
				Entity* entity = newEntity(-1, 1, map.entities);
				entity->flags[INVISIBLE] = TRUE;
				entity->flags[UPDATENEEDED] = TRUE;
				entity->flags[PASSABLE] = TRUE;
				entity->x = my->x;
				entity->y = my->y;
				entity->z = my->z;
				entity->sizex = my->sizex;
				entity->sizey = my->sizey;
				entity->yaw = my->yaw;
				entity->pitch = my->pitch;
				entity->roll = my->roll;
				entity->behavior = &actItem;
				entity->skill[10] = TOOL_BEARTRAP;
				entity->skill[11] = BEARTRAP_STATUS;
				entity->skill[12] = BEARTRAP_BEATITUDE;
				entity->skill[13] = 1;
				entity->skill[14] = BEARTRAP_APPEARANCE;
				entity->skill[15] = BEARTRAP_IDENTIFIED;
				messagePlayer(i, language[1300]);
				list_RemoveNode(my->mynode);
				return;
			}
		}
	}

	// launch beartrap
	node_t* node;
	for ( node = map.entities->first; node != NULL; node = node->next ) {
		Entity* entity = (Entity*)node->element;
		if ( my->parent == entity->uid ) {
			continue;
		}
		if ( entity->behavior == &actMonster || entity->behavior == &actPlayer ) {
			Stat* stat = entity->getStats();
			if ( stat ) {
				if ( entityDist(my, entity) < 6.5 ) {
					stat->EFFECTS[EFF_PARALYZED] = TRUE;
					stat->EFFECTS_TIMERS[EFF_PARALYZED] = 200;
					stat->EFFECTS[EFF_BLEEDING] = TRUE;
					stat->EFFECTS_TIMERS[EFF_BLEEDING] = 300;
					entity->modHP(-15 - rand() % 10);

					// set obituary
					entity->setObituary(language[1504]);

					if ( stat->HP <= 0 ) {
						Entity* parent = uidToEntity(my->parent);
						if ( parent ) {
							parent->awardXP( entity, TRUE, TRUE );
						}
					}
					if ( entity->behavior == &actPlayer ) {
						int player = entity->skill[2];
						Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
						messagePlayerColor(player, color, language[454]);
						if ( player > 0 ) {
							serverUpdateEffects(player);
						}
						if ( player == clientnum ) {
							camera_shakex += .1;
							camera_shakey += 10;
						} else if ( player > 0 && multiplayer == SERVER ) {
							strcpy((char*)net_packet->data, "SHAK");
							net_packet->data[4] = 10; // turns into .1
							net_packet->data[5] = 10;
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}
					}
					playSoundEntity(my, 76, 64);
					playSoundEntity(entity, 28, 64);
					Entity* gib = spawnGib(entity);
					serverSpawnGibForClient(gib);

					// make first arm
					entity = newEntity(98, 1, map.entities);
					entity->behavior = &actBeartrapLaunched;
					entity->flags[PASSABLE] = TRUE;
					entity->flags[UPDATENEEDED] = TRUE;
					entity->x = my->x;
					entity->y = my->y;
					entity->z = my->z + 1;
					entity->yaw = my->yaw;
					entity->pitch = my->pitch;
					entity->roll = 0;

					// and then the second
					entity = newEntity(98, 1, map.entities);
					entity->behavior = &actBeartrapLaunched;
					entity->flags[PASSABLE] = TRUE;
					entity->flags[UPDATENEEDED] = TRUE;
					entity->x = my->x;
					entity->y = my->y;
					entity->z = my->z + 1;
					entity->yaw = my->yaw;
					entity->pitch = my->pitch;
					entity->roll = PI;

					// remove me
					list_RemoveNode(my->mynode);
					return;
				}
			}
		}
	}
}

void actBeartrapLaunched(Entity* my) {
	if ( my->ticks >= 200 ) {
		list_RemoveNode(my->mynode);
		return;
	}
}