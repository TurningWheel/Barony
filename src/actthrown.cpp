/*-------------------------------------------------------------------------------

	BARONY
	File: actthrown.cpp
	Desc: behavior function for a thrown item

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "items.hpp"
#include "sound.hpp"
#include "monster.hpp"
#include "interface/interface.hpp"
#include "net.hpp"
#include "collision.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define THROWN_VELX my->vel_x
#define THROWN_VELY my->vel_y
#define THROWN_VELZ my->vel_z
#define THROWN_TYPE (Item)my->skill[10]
#define THROWN_STATUS (Status)my->skill[11]
#define THROWN_BEATITUDE my->skill[12]
#define THROWN_COUNT my->skill[13]
#define THROWN_APPEARANCE my->skill[14]
#define THROWN_IDENTIFIED my->skill[15]
#define THROWN_LIFE my->skill[16]

void actThrown(Entity *my) {
	Item *item;
	Category cat=GEM;
	char *itemname=NULL;
	node_t *node;
	
	if( multiplayer==CLIENT ) {
		if( THROWN_LIFE==0 ) {
			Entity *tempEntity = uidToEntity(clientplayer);
			if( tempEntity ) {
				if( entityInsideEntity(my,tempEntity) ) {
					my->parent = tempEntity->uid;
				} else {
					node_t *node;
					for( node=map.entities->first; node!=NULL; node=node->next ) {
						Entity *entity = (Entity *)node->element;
						if( entity->behavior==&actPlayer || entity->behavior==&actMonster ) {
							if( entityInsideEntity(my,entity) ) {
								my->parent = entity->uid;
								break;
							}
						}
					}
				}
			} else {
				node_t *node;
				for( node=map.entities->first; node!=NULL; node=node->next ) {
					Entity *entity = (Entity *)node->element;
					if( entity->behavior==&actPlayer || entity->behavior==&actMonster ) {
						if( entityInsideEntity(my,entity) ) {
							my->parent = entity->uid;
							break;
						}
					}
				}
			}
		}
	} else {
		// select appropriate model
		my->skill[2] = -5;
		my->flags[INVISIBLE]=FALSE;
		item = newItemFromEntity(my);
		my->sprite=itemModel(item);
		free(item);
	}

	if( multiplayer==CLIENT )
		return;

	// gravity
	if( my->z < 7.5-models[my->sprite]->sizey*.25 ) {
		// fall
		THROWN_VELZ += 0.04;
		my->z += THROWN_VELZ;
		my->roll += 0.04;
	} else {
		if( my->x>=0 && my->y>=0 && my->x<map.width<<4 && my->y<map.height<<4 ) {
			if( map.tiles[(int)(my->y/16)*MAPLAYERS+(int)(my->x/16)*MAPLAYERS*map.height] ) {
				item = newItemFromEntity(my);
				if( itemCategory(item)==POTION ) {
					playSoundEntity(my,162,64);
					free(item);
					list_RemoveNode(my->mynode);
					return;
				} else {
					Entity *entity = newEntity(-1,1,map.entities);
					entity->flags[INVISIBLE]=TRUE;
					entity->flags[UPDATENEEDED]=TRUE;
					entity->flags[PASSABLE] = TRUE;
					entity->x = my->x;
					entity->y = my->y;
					entity->z = my->z;
					entity->sizex = my->sizex;
					entity->sizey = my->sizey;
					entity->yaw = my->yaw;
					entity->pitch = my->pitch;
					entity->roll = my->roll;
					entity->vel_x = THROWN_VELX;
					entity->vel_y = THROWN_VELY;
					entity->vel_z = my->vel_z;
					entity->behavior = &actItem;
					entity->skill[10] = item->type;
					entity->skill[11] = item->status;
					entity->skill[12] = item->beatitude;
					entity->skill[13] = item->count;
					entity->skill[14] = item->appearance;
					entity->skill[15] = item->identified;
					free(item);
					list_RemoveNode(my->mynode);
					return;
				}
			} else {
				// fall
				THROWN_VELZ += 0.04;
				my->z += THROWN_VELZ;
				my->roll += 0.04;
			}
		} else {
			// fall
			THROWN_VELZ += 0.04;
			my->z += THROWN_VELZ;
			my->roll += 0.04;
		}
	}
	
	// falling out of the map
	if( my->z > 128 ) {
		list_RemoveNode(my->mynode);
		return;
	}
	
	// horizontal motion
	double ox = my->x;
	double oy = my->y;
	double oz = my->z;
	double result = clipMove(&my->x,&my->y,THROWN_VELX,THROWN_VELY,my);

	bool usedpotion=FALSE;
	if( result != sqrt( THROWN_VELX*THROWN_VELX + THROWN_VELY*THROWN_VELY ) ) {
		item = newItemFromEntity(my);
		cat = itemCategory(item);
		itemname = item->getName();
		item->count = 1;
		if( hit.entity != NULL ) {
			if( !(svFlags&SV_FLAG_FRIENDLYFIRE) ) {
				// test for friendly fire
				Entity *parent = uidToEntity(my->parent);
				if ( parent && parent->checkFriend(hit.entity) ) {
					list_RemoveNode(my->mynode);
					return;
				}
			}
			if( hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer ) {
				int damage = std::max(0,10-AC(hit.entity->getStats())+item->beatitude);
				hit.entity->modHP(-damage);

				// set the obituary
				char whatever[256];
				snprintf(whatever,255,language[1508],itemname);
				hit.entity->setObituary(whatever);

				Entity *parent = uidToEntity(my->parent);
				Stat *hitstats = hit.entity->getStats();
				if( hitstats ) {
					if( hitstats->type < LICH || hitstats->type >= SHOPKEEPER ) { // this makes it impossible to bork the end boss :)
						switch( item->type ) {
							case POTION_WATER:
								item_PotionWater(item, hit.entity);
								usedpotion=TRUE;
								break;
							case POTION_BOOZE:
								item_PotionBooze(item, hit.entity);
								usedpotion=TRUE;
								break;
							case POTION_JUICE:
								item_PotionJuice(item, hit.entity);
								usedpotion=TRUE;
								break;
							case POTION_SICKNESS:
								item_PotionSickness(item, hit.entity);
								usedpotion=TRUE;
								break;
							case POTION_CONFUSION:
								item_PotionConfusion(item, hit.entity);
								usedpotion=TRUE;
								break;
							case POTION_EXTRAHEALING:
								item_PotionExtraHealing(item, hit.entity);
								usedpotion=TRUE;
								break;
							case POTION_HEALING:
								item_PotionHealing(item, hit.entity);
								usedpotion=TRUE;
								break;
							case POTION_CUREAILMENT:
								item_PotionCureAilment(item, hit.entity);
								usedpotion=TRUE;
								break;
							case POTION_BLINDNESS:
								item_PotionBlindness(item, hit.entity);
								usedpotion=TRUE;
								break;
							case POTION_RESTOREMAGIC:
								item_PotionRestoreMagic(item, hit.entity);
								usedpotion=TRUE;
								break;
							case POTION_INVISIBILITY:
								item_PotionInvisibility(item, hit.entity);
								usedpotion=TRUE;
								break;
							case POTION_LEVITATION:
								item_PotionLevitation(item, hit.entity);
								usedpotion=TRUE;
								break;
							case POTION_SPEED:
								item_PotionSpeed(item, hit.entity);
								usedpotion=TRUE;
								break;
							case POTION_ACID:
								item_PotionAcid(item, hit.entity);
								usedpotion=TRUE;
								break;
							case POTION_PARALYSIS:
								item_PotionParalysis(item, hit.entity);
								usedpotion=TRUE;
								break;
							default:
								break;
						}
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
					if( rand()%10==0 && parent != NULL )
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
					node_t *node;
					for( node=map.entities->first; node!=NULL; node=node->next ) {
						Entity *entity = (Entity *)node->element;
						if( entity && entity->behavior == &actMonster && entity != ohitentity ) {
							if( entity->checkFriend(hit.entity) ) {
								if( entity->skill[0] == 0 ) { // monster is waiting
									double tangent = atan2( entity->y-ohitentity->y, entity->x-ohitentity->x );
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
					hit.entity = ohitentity;
					Uint32 color = SDL_MapRGB(mainsurface->format,0,255,0);
					if( parent->behavior == &actPlayer ) {
						if( !strcmp(hitstats->name,"") ) {
							messagePlayerColor(parent->skill[2],color,language[690],language[90+hitstats->type]);
							if( damage==0 )
								messagePlayer(parent->skill[2],language[447]);
						} else {
							messagePlayerColor(parent->skill[2],color,language[694],hitstats->name);
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
					messagePlayerColor(hit.entity->skill[2],color,language[588],itemname);
					if( damage==0 )
						messagePlayer(hit.entity->skill[2],language[452]);
				}
			}
		}
		if( cat==POTION ) {
			// potions shatter on impact
			playSoundEntity(my,162,64);
			if( !usedpotion )
				free(item);
			list_RemoveNode(my->mynode);
			return;
		} else {
			Entity *entity = newEntity(-1,1,map.entities);
			entity->flags[INVISIBLE]=TRUE;
			entity->flags[UPDATENEEDED]=TRUE;
			entity->flags[PASSABLE] = TRUE;
			entity->x = ox;
			entity->y = oy;
			entity->z = oz;
			entity->sizex = my->sizex;
			entity->sizey = my->sizey;
			entity->yaw = my->yaw;
			entity->pitch = my->pitch;
			entity->roll = my->roll;
			entity->vel_x = THROWN_VELX/2;
			entity->vel_y = THROWN_VELY/2;
			entity->vel_z = my->vel_z;
			entity->behavior = &actItem;
			entity->skill[10] = item->type;
			entity->skill[11] = item->status;
			entity->skill[12] = item->beatitude;
			entity->skill[13] = item->count;
			entity->skill[14] = item->appearance;
			entity->skill[15] = item->identified;
			free(item);
			list_RemoveNode(my->mynode);
			return;
		}
	}
	THROWN_VELX = THROWN_VELX*.99;
	THROWN_VELY = THROWN_VELY*.99;
	my->pitch += result*.01;
}
