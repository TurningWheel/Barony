/*-------------------------------------------------------------------------------

	BARONY
	File: actboulder.cpp
	Desc: implements boulder and boulder trap code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "sound.hpp"
#include "items.hpp"
#include "net.hpp"
#include "monster.hpp"
#include "collision.hpp"

#define BOULDER_STOPPED my->skill[0]
#define BOULDER_AMBIENCE my->skill[1]
#define BOULDER_NOGROUND my->skill[3]
#define BOULDER_ROLLING my->skill[4]
#define BOULDER_ROLLDIR my->skill[5]
#define BOULDER_DESTX my->skill[6]
#define BOULDER_DESTY my->skill[7]

/*-------------------------------------------------------------------------------

	boulderCheckAgainstEntity

	causes the boulder given in my to crush the object given in entity
	or vice versa

-------------------------------------------------------------------------------*/

int boulderCheckAgainstEntity(Entity *my, Entity *entity) {
	if (!my || !entity)
		return 0;

	if( entity->behavior == &actPlayer || entity->behavior == &actMonster ) {
		if( entityInsideEntity( my, entity ) ) {
			stat_t *stats = entity->getStats();
			if( stats ) {
				if( entity->behavior==&actPlayer ) {
					Uint32 color = SDL_MapRGB(mainsurface->format,255,0,0);
					messagePlayerColor(entity->skill[2],color,language[455]);
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
				playSoundEntity(my,181,128);
				playSoundEntity(entity,28,64);
				spawnGib(entity);
				entity->modHP(-80);
				entity->setObituary(language[1505]);
				if( entity->behavior==&actPlayer )
					if( stats->HP<=0 )
						steamAchievementClient(entity->skill[2],"BARONY_ACH_THROW_ME_THE_WHIP");
				if( stats->HP > 0 ) {
					// spawn several rock items
					int i = 8+rand()%4;
					
					int c;
					for( c=0; c<i; c++ ) {
						Entity *entity = newEntity(-1,1,map.entities);
						entity->flags[INVISIBLE]=TRUE;
						entity->flags[UPDATENEEDED]=TRUE;
						entity->x = my->x - 4 + rand()%8;
						entity->y = my->y - 4 + rand()%8;
						entity->z = -6+rand()%12;
						entity->sizex = 4;
						entity->sizey = 4;
						entity->yaw = rand()%360 * PI/180;
						entity->vel_x = (rand()%20-10)/10.0;
						entity->vel_y = (rand()%20-10)/10.0;
						entity->vel_z = -.25 - (rand()%5)/10.0;
						entity->flags[PASSABLE] = TRUE;
						entity->behavior = &actItem;
						entity->flags[USERFLAG1] = TRUE; // no collision: helps performance
						entity->skill[10] = GEM_ROCK;    // type
						entity->skill[11] = WORN;        // status
						entity->skill[12] = 0;           // beatitude
						entity->skill[13] = 1;           // count
						entity->skill[14] = 0;           // appearance
						entity->skill[15] = FALSE;       // identified
					}

					double ox = my->x;
					double oy = my->y;
					
					// destroy the boulder
					playSoundEntity(my,67,128);
					list_RemoveNode(my->mynode);

					// on sokoban, destroying boulders spawns scorpions
					if( !strcmp(map.name,"Sokoban") ) {
						Entity *monster = summonMonster(SCORPION,ox,oy);
						if( monster ) {
							int c;
							for( c=0; c<MAXPLAYERS; c++ ) {
								Uint32 color = SDL_MapRGB(mainsurface->format,255,128,0);
								messagePlayerColor(c,color,language[406]);
							}
						}
					}

					return 1;
				}
			}
		}
	} else if( entity->behavior == &actGate || entity->behavior == &actBoulder || entity->behavior==&actChest || entity->behavior==&actHeadstone || entity->behavior==&actFountain || entity->behavior==&actSink ) {
		if( !entity->flags[PASSABLE] ) {
			if( entityInsideEntity( my, entity ) ) {
				// stop the boulder
				BOULDER_STOPPED=1;
				BOULDER_ROLLING=0;
				playSoundEntity(my,181,128);
				if( my->flags[PASSABLE] ) {
					my->flags[PASSABLE] = FALSE;
					if( multiplayer==SERVER )
						serverUpdateEntityFlag(my,PASSABLE);
				}
			}
		}
	} else if( entity->behavior == &actDoor ) {
		if( entityInsideEntity( my, entity ) ) {
			playSoundEntity(entity,28,64);
			entity->skill[4] = 0;
			if( !entity->skill[0] )
				entity->skill[6] = (my->x > entity->x);
			else
				entity->skill[6] = (my->y < entity->y);
			playSoundEntity(my,181,128);
		}
	}
	return 0;
}

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

void actBoulder(Entity *my) {
	int i;
	
	my->skill[2]=-7; // invokes actEmpty() on clients
	my->flags[UPDATENEEDED] = TRUE;

	bool noground=FALSE;
	int x = std::min<int>(std::max(0,(int)(my->x/16)),map.width);
	int y = std::min<int>(std::max(0,(int)(my->y/16)),map.height);
	Uint32 index = y*MAPLAYERS+x*MAPLAYERS*map.height;
	if( !map.tiles[index] || animatedtiles[map.tiles[index]] )
		noground = TRUE;

	// gravity
	bool nobounce=TRUE;
	if( !BOULDER_NOGROUND )
		if( noground )
			BOULDER_NOGROUND = TRUE;
	if( my->z < 0 || BOULDER_NOGROUND ) {
		my->vel_z = std::min(my->vel_z+.1,3.0);
		nobounce = TRUE;
		if( my->z >= 128 ) {
			list_RemoveNode(my->mynode);
			return;
		}
		if( !BOULDER_NOGROUND ) {
			if( my->z >= -8 && fabs(my->vel_z) > 2 ) {
				node_t *node;
				for( node=map.entities->first; node!=NULL; node=node->next ) {
					Entity *entity = (Entity *)node->element;
					if( entity == my )
						continue;
					if( boulderCheckAgainstEntity(my,entity) )
						return;
				}
			}
		}
	} else {
		if( fabs(my->vel_z) > 1 ) {
			playSoundEntity(my,182,128);
			my->vel_z = -(my->vel_z/2);
			nobounce = TRUE;
		} else {
			if( my->vel_z )
				playSoundEntity(my,182,128);
			my->vel_z = 0;
			nobounce = FALSE;
		}
		my->z = 0;
	}
	my->z += my->vel_z;
	if( nobounce ) {
		if( !my->flags[PASSABLE] ) {
			my->flags[PASSABLE] = TRUE;
			if( multiplayer==SERVER )
				serverUpdateEntityFlag(my,PASSABLE);
		}
		if( !BOULDER_STOPPED ) {
			my->x += my->vel_x;
			my->y += my->vel_y;
			double dist = sqrt(pow(my->vel_x,2)+pow(my->vel_y,2));
			my->pitch += dist*.06;
			my->roll = PI/2;
		}
	} else if( !BOULDER_STOPPED ) {
		if( my->flags[PASSABLE] ) {
			my->flags[PASSABLE] = FALSE;
			if( multiplayer==SERVER )
				serverUpdateEntityFlag(my,PASSABLE);
		}
		
		// horizontal velocity
		my->vel_x += cos(my->yaw)*.1;
		my->vel_y += sin(my->yaw)*.1;
		if( my->vel_x > 1.5 )
			my->vel_x = 1.5;
		if( my->vel_x < -1.5 )
			my->vel_x = -1.5;
		if( my->vel_y > 1.5 )
			my->vel_y = 1.5;
		if( my->vel_y < -1.5 )
			my->vel_y = -1.5;
		int x = std::min<int>(std::max(0.0,(my->x+cos(my->yaw)*8)/16),map.width-1);
		int y = std::min<int>(std::max(0.0,(my->y+sin(my->yaw)*8)/16),map.height-1);
		if( map.tiles[OBSTACLELAYER+y*MAPLAYERS+x*MAPLAYERS*map.height] ) {
			playSoundEntity(my,181,128);
			BOULDER_STOPPED = 1;
		} else {
			my->x += my->vel_x;
			my->y += my->vel_y;
			double dist = sqrt(pow(my->vel_x,2)+pow(my->vel_y,2));
			my->pitch += dist*.06;
			my->roll = PI/2;
		
			// crush objects
			if( dist && !BOULDER_NOGROUND ) {
				node_t *node;
				for( node=map.entities->first; node!=NULL; node=node->next ) {
					Entity *entity = (Entity *)node->element;
					if( entity == my )
						continue;
					if( boulderCheckAgainstEntity(my,entity) )
						return;
				}
			}
		}
	}
	
	// pushing boulders
	if( BOULDER_STOPPED ) {
		if( !BOULDER_ROLLING ) {
			for(i=0;i<MAXPLAYERS;i++) {
				if( (i==0 && selectedEntity==my) || (client_selected[i]==my) ) {
					if(inrange[i]) {
						if( statGetSTR(&stats[i])<5 ) {
							messagePlayer(i,language[456]);
						} else {
							if( players[i] ) {
								playSoundEntity(my, 151, 128);
								BOULDER_ROLLING=1;
								my->x = floor(my->x/16)*16+8;
								my->y = floor(my->y/16)*16+8;
								BOULDER_DESTX=(int)(my->x/16)*16+8;
								BOULDER_DESTY=(int)(my->y/16)*16+8;
								if( (int)(players[i]->x/16) < (int)(my->x/16) ) {
									BOULDER_ROLLDIR=0; // east
								} else if( (int)(players[i]->y/16) < (int)(my->y/16) ) {
									BOULDER_ROLLDIR=1; // south
								} else if( (int)(players[i]->x/16) > (int)(my->x/16) ) {
									BOULDER_ROLLDIR=2; // west
								} else if( (int)(players[i]->y/16) > (int)(my->y/16) ) {
									BOULDER_ROLLDIR=3; // north
								}
								switch( BOULDER_ROLLDIR ) {
									case 0:
										BOULDER_DESTX += 16;
										break;
									case 1:
										BOULDER_DESTY += 16;
										break;
									case 2:
										BOULDER_DESTX -= 16;
										break;
									case 3:
										BOULDER_DESTY -= 16;
										break;
								}
							}
						}
					}
				}
			}
		} else {
			switch( BOULDER_ROLLDIR ) {
				case 0:
					my->vel_x=1;
					my->vel_y=0;
					break;
				case 1:
					my->vel_x=0;
					my->vel_y=1;
					break;
				case 2:
					my->vel_x=-1;
					my->vel_y=0;
					break;
				case 3:
					my->vel_x=0;
					my->vel_y=-1;
					break;
			}
			int x = (my->x+my->vel_x*8)/16;
			int y = (my->y+my->vel_y*8)/16;
			x = std::min<unsigned int>(std::max(0,x),map.width-1);
			y = std::min<unsigned int>(std::max(0,y),map.height-1);
			if( map.tiles[OBSTACLELAYER+y*MAPLAYERS+x*MAPLAYERS*map.height] ) {
				BOULDER_ROLLING = 0;
			} else {
				my->x += my->vel_x;
				my->y += my->vel_y;
				double dist = sqrt(pow(my->vel_x,2)+pow(my->vel_y,2));
				my->pitch += dist*.06;

				if( BOULDER_ROLLDIR==0 ) {
					if( my->x>=BOULDER_DESTX ) {
						my->x=BOULDER_DESTX;
						BOULDER_ROLLING=0;
					}
				} else if( BOULDER_ROLLDIR==1 ) {
					if( my->y>=BOULDER_DESTY ) {
						my->y=BOULDER_DESTY;
						BOULDER_ROLLING=0;
					}
				} else if( BOULDER_ROLLDIR==2 ) {
					if( my->x<=BOULDER_DESTX ) {
						my->x=BOULDER_DESTX;
						BOULDER_ROLLING=0;
					}
				} else if( BOULDER_ROLLDIR==3 ) {
					if( my->y<=BOULDER_DESTY ) {
						my->y=BOULDER_DESTY;
						BOULDER_ROLLING=0;
					}
				}
				double dir = my->yaw - BOULDER_ROLLDIR*PI/2;
				while( dir >= PI )
					dir -= PI*2;
				while( dir < -PI )
					dir += PI*2;
				my->yaw -= dir/16;
				while( my->yaw < 0 )
					my->yaw += 2*PI;
				while( my->yaw >= 2*PI )
					my->yaw -= 2*PI;
		
				// crush objects
				if( dist && !BOULDER_NOGROUND ) {
					node_t *node;
					for( node=map.entities->first; node!=NULL; node=node->next ) {
						Entity *entity = (Entity *)node->element;
						if( entity == my )
							continue;
						if( boulderCheckAgainstEntity(my,entity) )
							return;
					}
				}
			}
		}
	}

	// wrap around angles
	while( my->pitch >= PI*2 )
		my->pitch -= PI*2;
	while( my->pitch < 0 )
		my->pitch += PI*2;
	while( my->roll >= PI*2 )
		my->roll -= PI*2;
	while( my->roll < 0 )
		my->roll += PI*2;
	
	// rolling sound
	if( !BOULDER_STOPPED && (fabs(my->vel_x)>0 || fabs(my->vel_y)>0) ) {
		BOULDER_AMBIENCE++;
		if( BOULDER_AMBIENCE>=TICKS_PER_SECOND/3 ) {
			BOULDER_AMBIENCE=0;
			playSoundEntity(my, 151, 128);
		}
	}
}

#define BOULDERTRAP_FIRED my->skill[0]
#define BOULDERTRAP_AMBIENCE my->skill[6]

void actBoulderTrap(Entity *my) {
	int x, y;
	int c;

	BOULDERTRAP_AMBIENCE--;
	if( BOULDERTRAP_AMBIENCE<=0 ) {
		BOULDERTRAP_AMBIENCE = TICKS_PER_SECOND*30;
		playSoundEntity( my, 149, 64 );
	}
	
	if( !my->skill[28] )
		return;

	// received on signal
	if( my->skill[28] == 2) {
		if( !BOULDERTRAP_FIRED ) {
			playSoundEntity(my, 150, 128);
			for( c=0; c<MAXPLAYERS; c++ )
				playSoundPlayer(c, 150, 64);
			BOULDERTRAP_FIRED = 1;
			for( c=0; c<4; c++ ) {
				switch( c ) {
					case 0:
						x = 16;
						y = 0;
						break;
					case 1:
						x = 0;
						y = 16;
						break;
					case 2:
						x = -16;
						y = 0;
						break;
					case 3:
						x = 0;
						y = -16;
						break;
				}
				x = ((int)(x+my->x))>>4;
				y = ((int)(y+my->y))>>4;
				if( x>=0 && y>=0 && x<map.width && y<map.height ) {
					if( !map.tiles[OBSTACLELAYER+y*MAPLAYERS+x*MAPLAYERS*map.height] ) {
						Entity *entity = newEntity(245, 1, map.entities); // boulder
						entity->parent = my->uid;
						entity->x = (x<<4)+8;
						entity->y = (y<<4)+8;
						entity->z = -64;
						entity->yaw = c*(PI/2.f);
						entity->sizex = 7;
						entity->sizey = 7;
						if( checkObstacle( entity->x+cos(entity->yaw)*16, entity->y+sin(entity->yaw)*16, entity, NULL ) ) {
							entity->yaw += PI*(rand()%2)-PI/2;
							if( entity->yaw >= PI*2 )
								entity->yaw -= PI*2;
							else if( entity->yaw < 0 )
								entity->yaw += PI*2;
						}
						entity->behavior = &actBoulder;
						entity->flags[UPDATENEEDED] = TRUE;
						entity->flags[PASSABLE] = TRUE;
					}
				}
			}
		}
	}
}
