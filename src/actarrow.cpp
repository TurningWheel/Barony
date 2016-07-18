/*-------------------------------------------------------------------------------

	BARONY
	File: actarrow.cpp
	Desc: behavior function for arrows/bolts

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "monster.hpp"
#include "sound.hpp"
#include "interface/interface.hpp"
#include "net.hpp"
#include "collision.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define ARROW_STUCK my->skill[0]
#define ARROW_LIFE my->skill[1]
#define ARROW_POWER my->skill[3]
#define ARROW_POISON my->skill[4]
#define ARROW_VELX my->vel_x
#define ARROW_VELY my->vel_y
#define ARROW_VELZ my->vel_z
#define ARROW_OLDX my->fskill[2]
#define ARROW_OLDY my->fskill[3]
#define ARROW_MAXLIFE 600

void actArrow(Entity *my) {
	double dist;
	int damage;
	Entity *entity;
	node_t *node;
	double tangent;
	
	my->skill[2]=-7; // invokes actEmpty() on clients
	
	if( !ARROW_POWER ) {
		ARROW_POWER = 10+(my->sprite==167);
	}

	// lifespan
	ARROW_LIFE++;
	if( ARROW_LIFE>=ARROW_MAXLIFE ) {
		list_RemoveNode(my->mynode);
		return;
	}

	if( !ARROW_STUCK ) {
		// horizontal motion
		ARROW_VELX = cos(my->yaw)*7;
		ARROW_VELY = sin(my->yaw)*7;
		
		ARROW_OLDX = my->x;
		ARROW_OLDY = my->y;
		dist = clipMove(&my->x,&my->y,ARROW_VELX,ARROW_VELY,my);
		
		// damage monsters
		if( dist != sqrt(ARROW_VELX*ARROW_VELX+ARROW_VELY*ARROW_VELY) ) {
			ARROW_STUCK = 1;
			my->x = ARROW_OLDX;
			my->y = ARROW_OLDY;
			Entity *oentity = hit.entity;
			lineTrace(my,my->x,my->y,my->yaw,sqrt(ARROW_VELX*ARROW_VELX+ARROW_VELY*ARROW_VELY),0,FALSE);
			hit.entity = oentity;
			my->x = hit.x-cos(my->yaw);
			my->y = hit.y-sin(my->yaw);
			ARROW_VELX = 0;
			ARROW_VELY = 0;
			ARROW_VELZ = 0;
			if( hit.entity != NULL ) {
				Entity *parent = uidToEntity(my->parent);
				Stat *hitstats = hit.entity->getStats();
				playSoundEntity(my,72+rand()%3,64);
				if( hitstats != NULL && hit.entity != parent ) {
					if( !(svFlags&SV_FLAG_FRIENDLYFIRE) ) {
						// test for friendly fire
						if ( parent && parent->checkFriend(hit.entity) ) {
							list_RemoveNode(my->mynode);
							return;
						}
					}

					// do damage
					damage = std::max(ARROW_POWER-AC(hitstats),0)*damagetables[hitstats->type][4];
					hit.entity->modHP(-damage);

					// write obituary
					if( parent ) {
						if( parent->behavior==&actArrowTrap ) {
							hit.entity->setObituary(language[1503]);
						} else {
							parent->killedByMonsterObituary(hit.entity);
						}
					}

					// update enemy bar for attacker
					if( !strcmp(hitstats->name,"") )
						updateEnemyBar(parent,hit.entity,language[90+hitstats->type],hitstats->HP,hitstats->MAXHP);
					else
						updateEnemyBar(parent,hit.entity,hitstats->name,hitstats->HP,hitstats->MAXHP);

					if( damage>0 ) {
						Entity *gib = spawnGib(hit.entity);
						serverSpawnGibForClient(gib);
						playSoundEntity(hit.entity,28,64);
						if( hit.entity->behavior == &actPlayer ) {
							if( hit.entity->skill[2] == clientnum ) {
								camera_shakex += .1;
								camera_shakey += 10;
							} else {
								strcpy((char *)net_packet->data,"SHAK");
								net_packet->data[4]=10; // turns into .1
								net_packet->data[5]=10;
								net_packet->address.host = net_clients[hit.entity->skill[2]-1].host;
								net_packet->address.port = net_clients[hit.entity->skill[2]-1].port;
								net_packet->len = 6;
								sendPacketSafe(net_sock, -1, net_packet, hit.entity->skill[2]-1);
							}
						}
						if( rand()%10==0 && parent )
							parent->increaseSkill(PRO_RANGED);
					}
					if( hitstats->HP <= 0 && parent) {
						parent->awardXP( hit.entity, TRUE, TRUE );
					}
					
					// alert the monster
					if( hit.entity->behavior == &actMonster && parent != NULL ) {
						if( hit.entity->skill[0]!=1 && (hitstats->type<LICH || hitstats->type>=SHOPKEEPER) ) {
							hit.entity->skill[0]=2;
							hit.entity->skill[1]=parent->uid;
							hit.entity->fskill[2]=parent->x;
							hit.entity->fskill[3]=parent->y;
						}
							
						// alert other monsters too
						Entity *ohitentity = hit.entity;
						for( node=map.entities->first; node!=NULL; node=node->next ) {
							entity = (Entity *)node->element;
							if ( entity && entity->behavior == &actMonster && entity != ohitentity ) {
								Stat *buddystats = entity->getStats();
								if( buddystats != NULL ) {
									if ( entity->checkFriend(hit.entity) ) {
										if( entity->skill[0] == 0 ) { // monster is waiting
											tangent = atan2( entity->y-ohitentity->y, entity->x-ohitentity->x );
											lineTrace(ohitentity,ohitentity->x,ohitentity->y,tangent,1024,0,FALSE);
											if( hit.entity == entity ) {
												entity->skill[0] = 2; // path state
												entity->skill[1] = parent->uid;
												entity->fskill[2] = parent->x;
												entity->fskill[3] = parent->y;
											}
										}
									}
								}
							}
						}
						hit.entity = ohitentity;
						if( parent->behavior == &actPlayer ) {
							if( !strcmp(hitstats->name,"") ) {
								Uint32 color = SDL_MapRGB(mainsurface->format,0,255,0);
								messagePlayerColor(parent->skill[2],color,language[446],language[90+hitstats->type]);
								if( damage==0 )
									messagePlayer(parent->skill[2],language[447]);
							} else {
								Uint32 color = SDL_MapRGB(mainsurface->format,0,255,0);
								messagePlayerColor(parent->skill[2],color,language[448],hitstats->name);
								if( damage==0 ) {
									if( hitstats->sex )
										messagePlayer(parent->skill[2],language[449]);
									else
										messagePlayer(parent->skill[2],language[450]);
								}
							}
						}
					} else if( hit.entity->behavior == &actPlayer ) {
						Uint32 color = SDL_MapRGB(mainsurface->format,255,0,0);
						messagePlayerColor(hit.entity->skill[2],color,language[451]);
						if( damage==0 )
							messagePlayer(hit.entity->skill[2],language[452]);
					}
					if( ARROW_POISON && damage>0 ) {
						hitstats->poisonKiller = my->parent;
						hitstats->EFFECTS[EFF_POISONED] = TRUE;
						hitstats->EFFECTS_TIMERS[EFF_POISONED] = ARROW_POISON;
						if( hit.entity->behavior==&actPlayer ) {
							Uint32 color = SDL_MapRGB(mainsurface->format,255,0,0);
							messagePlayerColor(hit.entity->skill[2],color,language[453]);
							serverUpdateEffects(hit.entity->skill[2]);
						}
					}
				}
				list_RemoveNode(my->mynode);
			} else if( my->sprite==78 ) {
				list_RemoveNode(my->mynode); // rocks don't stick to walls...
			} else {
				playSoundEntity(my,72+rand()%3,64);
			}
		}
	}
}
