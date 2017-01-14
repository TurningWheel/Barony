/*-------------------------------------------------------------------------------

	BARONY
	File: actladder.cpp
	Desc: behavior function for ladders

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "sound.hpp"
#include "scores.hpp"
#include "entity.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define LADDER_AMBIENCE my->skill[1]
#define LADDER_SECRET my->skill[3]

void actLadder(Entity *my) {
	int playercount = 0;
	double dist;
	int i, c;

	LADDER_AMBIENCE--;
	if (LADDER_AMBIENCE <= 0) {
		LADDER_AMBIENCE = TICKS_PER_SECOND * 30;
		playSoundEntityLocal(my, 149, 64);
	}

	// use ladder (climb)
	if (multiplayer != CLIENT) {
		for (i = 0; i < MAXPLAYERS; i++) {
			if ((i == 0 && selectedEntity == my) || (client_selected[i] == my)) {
				if (inrange[i]) {
					for (c = 0; c < MAXPLAYERS; c++) {
						if (client_disconnected[c] || players[c] == nullptr || players[c]->entity == nullptr) {
							continue;
						} else {
							playercount++;
						}
						dist = sqrt(pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
						if (dist > TOUCHRANGE) {
							messagePlayer(i, language[505]);
							return;
						}
					}
					if (playercount == 1) {
						messagePlayer(i, language[506]);
					} else {
						messagePlayer(i, language[507]);
					}
					loadnextlevel = TRUE;
					if (secretlevel) {
						switch (currentlevel) {
							case 3:
								for (c = 0; c < MAXPLAYERS; c++) {
									steamAchievementClient(c, "BARONY_ACH_THUNDERGNOME");
								}
								break;
						}
					}
					if (LADDER_SECRET) {
						secretlevel = (secretlevel == FALSE);    // toggle level lists
					}
					return;
				}
			}
		}
	}
}

void actLadderUp(Entity *my) {
	int i;

	LADDER_AMBIENCE--;
	if ( LADDER_AMBIENCE <= 0 ) {
		LADDER_AMBIENCE = TICKS_PER_SECOND * 30;
		playSoundEntityLocal( my, 149, 64 );
	}

	// use ladder
	if ( multiplayer != CLIENT ) {
		for (i = 0; i < MAXPLAYERS; i++) {
			if ( (i == 0 && selectedEntity == my) || (client_selected[i] == my) ) {
				if (inrange[i]) {
					messagePlayer(i, language[508]);
					return;
				}
			}
		}
	}
}

#define PORTAL_AMBIENCE my->skill[0]
#define PORTAL_INIT my->skill[1]
#define PORTAL_NOTSECRET my->skill[3]

void actPortal(Entity *my) {
	int playercount = 0;
	double dist;
	int i, c;

	if ( !PORTAL_INIT ) {
		PORTAL_INIT = 1;
		my->light = lightSphereShadow(my->x / 16, my->y / 16, 3, 255);
	}

	PORTAL_AMBIENCE--;
	if ( PORTAL_AMBIENCE <= 0 ) {
		PORTAL_AMBIENCE = TICKS_PER_SECOND * 2;
		playSoundEntityLocal( my, 154, 128 );
	}

	my->yaw += 0.01; // rotate slowly on my axis
	my->sprite = 254 + (my->ticks / 20) % 4; // animate

	if ( multiplayer == CLIENT ) {
		return;
	}

	// step through portal
	for (i = 0; i < MAXPLAYERS; i++) {
		if ((i == 0 && selectedEntity == my) || (client_selected[i] == my)) {
			if (inrange[i]) {
				for (c = 0; c < MAXPLAYERS; c++) {
					if (client_disconnected[c] || players[c] == nullptr || players[c]->entity == nullptr) {
						continue;
					} else {
						playercount++;
					}
					dist = sqrt(pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
					if (dist > TOUCHRANGE) {
						messagePlayer(i, language[505]);
						return;
					}
				}
				if (playercount == 1) {
					messagePlayer(i, language[510]);
				} else {
					messagePlayer(i, language[511]);
				}
				loadnextlevel = TRUE;
				if ( secretlevel ) {
					switch ( currentlevel ) {
						case 9: {
							;
							bool visiblegrave = FALSE;
							node_t *node;
							for ( node = map.entities->first; node != NULL; node = node->next ) {
								Entity *entity = (Entity *)node->element;
								if ( entity->sprite == 224 && !entity->flags[INVISIBLE] ) {
									visiblegrave = TRUE;
									break;
								}
							}
							if ( visiblegrave )
								for ( c = 0; c < MAXPLAYERS; c++ ) {
									steamAchievementClient(c, "BARONY_ACH_ROBBING_THE_CRADLE");
								}
							break;
						}
						case 14:
							for ( c = 0; c < MAXPLAYERS; c++ ) {
								steamAchievementClient(c, "BARONY_ACH_THESEUS_LEGACY");
							}
							break;
					}
				}
				if ( !PORTAL_NOTSECRET ) {
					secretlevel = (secretlevel == FALSE);  // toggle level lists
				}
				return;
			}
		}
	}
}

#define PORTAL_VICTORYTYPE my->skill[4]

void actWinningPortal(Entity *my) {
	int playercount = 0;
	double dist;
	int i, c;

	if ( multiplayer != CLIENT ) {
		if ( my->flags[INVISIBLE] ) {
			node_t *node;
			for ( node = map.entities->first; node != NULL; node = node->next ) {
				Entity *entity = (Entity *)node->element;
				if ( entity->behavior == &actMonster ) {
					Stat *stats = entity->getStats();
					if ( stats ) {
						if ( stats->type == LICH || stats->type == DEVIL ) {
							return;
						}
					}
				}
			}
			my->flags[INVISIBLE] = FALSE;
		}
	} else {
		if ( my->flags[INVISIBLE] ) {
			return;
		}
	}

	if ( !PORTAL_INIT ) {
		PORTAL_INIT = 1;
		my->light = lightSphereShadow(my->x / 16, my->y / 16, 3, 255);
	}

	PORTAL_AMBIENCE--;
	if ( PORTAL_AMBIENCE <= 0 ) {
		PORTAL_AMBIENCE = TICKS_PER_SECOND * 2;
		playSoundEntityLocal( my, 154, 128 );
	}

	my->yaw += 0.01; // rotate slowly on my axis
	my->sprite = 278 + (my->ticks / 20) % 4; // animate

	if ( multiplayer == CLIENT ) {
		return;
	}

	// step through portal
	for (i = 0; i < MAXPLAYERS; i++) {
		if ((i == 0 && selectedEntity == my) || (client_selected[i] == my)) {
			if (inrange[i]) {
				for (c = 0; c < MAXPLAYERS; c++) {
					if (client_disconnected[c] || players[c] == nullptr || players[c]->entity == nullptr) {
						continue;
					} else {
						playercount++;
					}
					dist = sqrt( pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
					if (dist > TOUCHRANGE) {
						messagePlayer(i, language[509]);
						return;
					}
				}
				victory = PORTAL_VICTORYTYPE;
				if ( multiplayer == SERVER ) {
					for ( c = 0; c < MAXPLAYERS; c++ ) {
						if ( client_disconnected[c] == TRUE ) {
							continue;
						}
						strcpy((char *)net_packet->data, "WING");
						net_packet->data[4] = victory;
						net_packet->address.host = net_clients[c - 1].host;
						net_packet->address.port = net_clients[c - 1].port;
						net_packet->len = 8;
						sendPacketSafe(net_sock, -1, net_packet, c - 1);
					}
				}
				subwindow = 0;
				introstage = 5; // prepares win game sequence
				fadeout = TRUE;
				if ( !intro ) {
					pauseGame(2, FALSE);
				}
				return;
			}
		}
	}
}