/*-------------------------------------------------------------------------------

	BARONY
	File: monster_troll.cpp
	Desc: implements all of the troll monster's code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "monster.hpp"
#include "sound.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"

void initTroll(Entity *my, Stat *myStats) {
	int c;
	node_t *node;

	my->sprite = 204;

	my->flags[UPDATENEEDED]=TRUE;
	my->flags[BLOCKSIGHT]=TRUE;
	my->flags[INVISIBLE]=FALSE;

	if( multiplayer!=CLIENT ) {
		MONSTER_SPOTSND = 79;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 1;
	}
	if( multiplayer!=CLIENT && !MONSTER_INIT ) {
		myStats->sex = static_cast<sex_t>(rand()%2);
		myStats->appearance = rand();
		myStats->inventory.first = NULL;
		myStats->inventory.last = NULL;
		myStats->HP = 100+rand()%20;
		myStats->MAXHP = myStats->HP;
		myStats->MP = 30;
		myStats->MAXMP = 30;
		myStats->OLDHP = myStats->HP;
		myStats->STR = 15;
		myStats->DEX = -2;
		myStats->CON = 5;
		myStats->INT = -4;
		myStats->PER = -2;
		myStats->CHR = -1;
		myStats->EXP = 0;
		myStats->LVL = 12;
		myStats->GOLD = 0;
		myStats->HUNGER = 900;
		if( !myStats->leader_uid )
			myStats->leader_uid = 0;
		myStats->FOLLOWERS.first=NULL;
		myStats->FOLLOWERS.last=NULL;
		for( c=0; c<std::max(NUMPROFICIENCIES,NUMEFFECTS); c++ ) {
			if( c<NUMPROFICIENCIES )
				myStats->PROFICIENCIES[c]=0;
			if( c<NUMEFFECTS )
				myStats->EFFECTS[c]=FALSE;
			if( c<NUMEFFECTS )
				myStats->EFFECTS_TIMERS[c]=0;
		}
		myStats->helmet = NULL;
		myStats->breastplate = NULL;
		myStats->gloves = NULL;
		myStats->shoes = NULL;
		myStats->shield = NULL;
		myStats->weapon = NULL;
		myStats->cloak = NULL;
		myStats->amulet = NULL;
		myStats->ring = NULL;
		myStats->mask = NULL;

		if( rand()%4==0 ) {
			myStats->EFFECTS[EFF_ASLEEP] = TRUE;
			myStats->EFFECTS_TIMERS[EFF_ASLEEP] = 1800+rand()%3600;
		}

		if( rand()%3==0 ) {
			int i = 1+rand()%3;
			for( c=0; c<i; c++ )
				newItem( static_cast<ItemType>(rand()%(NUMITEMS-6)), static_cast<Status>(1+rand()%4), -1+rand()%3, 1, rand(), FALSE, &myStats->inventory );
		}

		if( rand()%50 || my->flags[USERFLAG2] ) {
			strcpy(myStats->name,"");
		} else {
			strcpy(myStats->name,"Thumpus the Troll");
			for( c=0; c<3; c++ ) {
				Entity *entity = summonMonster(GNOME,my->x,my->y);
				if( entity )
					entity->parent = my->uid;
			}
			myStats->HP *= 2;
			myStats->MAXHP *= 2;
			myStats->OLDHP = myStats->HP;
			myStats->GOLD += 300;
			myStats->LVL += 10;
		}
	}

	// torso
	Entity *entity = newEntity(205, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->uid;
	entity->flags[PASSABLE]=TRUE;
	entity->flags[NOUPDATE]=TRUE;
	entity->flags[USERFLAG2]=my->flags[USERFLAG2];
	entity->focalx=limbs[TROLL][1][0]; // 0
	entity->focaly=limbs[TROLL][1][1]; // 0
	entity->focalz=limbs[TROLL][1][2]; // 0
	entity->behavior=&actTrollLimb;
	entity->parent=my->uid;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity *);

	// right leg
	entity = newEntity(209, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->uid;
	entity->flags[PASSABLE]=TRUE;
	entity->flags[NOUPDATE]=TRUE;
	entity->flags[USERFLAG2]=my->flags[USERFLAG2];
	entity->focalx=limbs[TROLL][2][0]; // 1
	entity->focaly=limbs[TROLL][2][1]; // 0
	entity->focalz=limbs[TROLL][2][2]; // 2
	entity->behavior=&actTrollLimb;
	entity->parent=my->uid;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity *);

	// left leg
	entity = newEntity(208, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->uid;
	entity->flags[PASSABLE]=TRUE;
	entity->flags[NOUPDATE]=TRUE;
	entity->flags[USERFLAG2]=my->flags[USERFLAG2];
	entity->focalx=limbs[TROLL][3][0]; // 1
	entity->focaly=limbs[TROLL][3][1]; // 0
	entity->focalz=limbs[TROLL][3][2]; // 2
	entity->behavior=&actTrollLimb;
	entity->parent=my->uid;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity *);

	// right arm
	entity = newEntity(207, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->uid;
	entity->flags[PASSABLE]=TRUE;
	entity->flags[NOUPDATE]=TRUE;
	entity->flags[USERFLAG2]=my->flags[USERFLAG2];
	entity->focalx=limbs[TROLL][4][0]; // -.25
	entity->focaly=limbs[TROLL][4][1]; // 0
	entity->focalz=limbs[TROLL][4][2]; // 3
	entity->behavior=&actTrollLimb;
	entity->parent=my->uid;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity *);

	// left arm
	entity = newEntity(206, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->uid;
	entity->flags[PASSABLE]=TRUE;
	entity->flags[NOUPDATE]=TRUE;
	entity->flags[USERFLAG2]=my->flags[USERFLAG2];
	entity->focalx=limbs[TROLL][5][0]; // -.25
	entity->focaly=limbs[TROLL][5][1]; // 0
	entity->focalz=limbs[TROLL][5][2]; // 3
	entity->behavior=&actTrollLimb;
	entity->parent=my->uid;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity *);
}

void actTrollLimb(Entity *my) {
	int i;

	Entity *parent = NULL;
	if( (parent=uidToEntity(my->skill[2]))==NULL ) {
		list_RemoveNode(my->mynode);
		return;
	}

	if( multiplayer!=CLIENT ) {
		for( i=0; i<MAXPLAYERS; i++ ) {
			if( inrange[i] ) {
				if( i==0 && selectedEntity==my ) {
					parent->skill[13] = i+1;
				} else if( client_selected[i]==my ) {
					parent->skill[13] = i+1;
				}
			}
		}
	}
	return;
}

void trollDie(Entity *my) {
	node_t *node, *nextnode;

	int c;
	for( c=0; c<5; c++ ) {
		Entity *gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}
	if (spawn_blood) {
		int x, y;
		x = std::min<unsigned int>(std::max<int>(0,my->x/16),map.width-1);
		y = std::min<unsigned int>(std::max<int>(0,my->y/16),map.height-1);
		if( map.tiles[y*MAPLAYERS+x*MAPLAYERS*map.height] ) {
			if( !checkObstacle(my->x,my->y,my,NULL) ) {
				Entity *entity = newEntity(160,1,map.entities);
				entity->x = my->x;
				entity->y = my->y;
				entity->z = 7.4+(rand()%20)/100.f;
				entity->parent = my->uid;
				entity->sizex = 2;
				entity->sizey = 2;
				entity->yaw = (rand()%360)*PI/180.0;
				entity->flags[UPDATENEEDED] = TRUE;
				entity->flags[PASSABLE] = TRUE;
			}
		}
	}
	playSoundEntity(my, 80, 128);
	int i = 0;
	for (node=my->children.first; node!=NULL; node=nextnode) {
		nextnode=node->next;
		if (node->element != NULL && i >= 2) {
			Entity *entity=(Entity *)node->element;
			entity->flags[UPDATENEEDED]=FALSE;
			list_RemoveNode(entity->mynode);
		}
		list_RemoveNode(node);
		++i;
	}
	list_RemoveNode(my->mynode);
	return;
}

#define TROLLWALKSPEED .12

void trollMoveBodyparts(Entity *my, Stat *myStats, double dist) {
	node_t *node;
	Entity *entity = NULL;
	Entity *rightbody = NULL;
	int bodypart;

	// set invisibility
	if( multiplayer != CLIENT ) {
		if( myStats->EFFECTS[EFF_INVISIBLE] == TRUE ) {
			my->flags[INVISIBLE] = TRUE;
			my->flags[BLOCKSIGHT] = FALSE;
			bodypart=0;
			for(node = my->children.first; node!=NULL; node=node->next) {
				if( bodypart<2 ) {
					bodypart++;
					continue;
				}
				if( bodypart>=7 ) {
					break;
				}
				entity = (Entity *)node->element;
				if( !entity->flags[INVISIBLE] ) {
					entity->flags[INVISIBLE] = TRUE;
					serverUpdateEntityBodypart(my,bodypart);
				}
				bodypart++;
			}
		} else {
			my->flags[INVISIBLE] = FALSE;
			my->flags[BLOCKSIGHT] = TRUE;
			bodypart=0;
			for(node = my->children.first; node!=NULL; node=node->next) {
				if( bodypart<2 ) {
					bodypart++;
					continue;
				}
				if( bodypart>=7 ) {
					break;
				}
				entity = (Entity *)node->element;
				if( entity->flags[INVISIBLE] ) {
					entity->flags[INVISIBLE] = FALSE;
					serverUpdateEntityBodypart(my,bodypart);
				}
				bodypart++;
			}
		}

		// sleeping
		if( myStats->EFFECTS[EFF_ASLEEP] ) {
			my->z = 1.5;
		} else {
			my->z = -1.5;
		}
	}

	//Move bodyparts
	for(bodypart=0, node = my->children.first; node!=NULL; node=node->next, bodypart++) {
		if( bodypart<2 ) {
			continue;
		}
		entity = (Entity *)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		entity->yaw = my->yaw;
		if( bodypart==3||bodypart==6 ) {
			if( bodypart==3 )
				rightbody = (Entity *)node->next->element;
			if( bodypart==3 || !MONSTER_ATTACK ) {
				if( dist>0.1 ) {
					if( !rightbody->skill[0] ) {
						entity->pitch -= dist*TROLLWALKSPEED;
						if( entity->pitch < -PI/4.0 ) {
							entity->pitch = -PI/4.0;
							if(bodypart==3) {
								playSoundEntityLocal(my, 115, 64);
								entity->skill[0]=1;
							}
						}
					} else {
						entity->pitch += dist*TROLLWALKSPEED;
						if( entity->pitch > PI/4.0 ) {
							entity->pitch = PI/4.0;
							if(bodypart==3) {
								playSoundEntityLocal(my, 115, 64);
								entity->skill[0]=0;
							}
						}
					}
				} else {
					if( entity->pitch < 0 ) {
						entity->pitch += 1/fmax(dist*.1,10.0);
						if( entity->pitch > 0 )
							entity->pitch=0;
					} else if( entity->pitch > 0 ) {
						entity->pitch -= 1/fmax(dist*.1,10.0);
						if( entity->pitch < 0 )
							entity->pitch=0;
					}
				}
			} else {
				// vertical chop
				if( MONSTER_ATTACKTIME == 0 ) {
					MONSTER_ARMBENDED = 0;
					MONSTER_WEAPONYAW = 0;
					entity->pitch = -3*PI/4;
					entity->roll = 0;
				} else {
					if( entity->pitch >= -PI/2 )
						MONSTER_ARMBENDED = 1;
					if( entity->pitch >= PI/4 ) {
						entity->skill[0] = rightbody->skill[0];
						MONSTER_WEAPONYAW = 0;
						entity->pitch = rightbody->pitch;
						entity->roll = 0;
						MONSTER_ARMBENDED = 0;
						MONSTER_ATTACK = 0;
					} else {
						entity->pitch += .25;
					}
				}
			}
		} else if( bodypart==4||bodypart==5 ) {
			if( bodypart==5 ) {
				if( MONSTER_ATTACK ) {
					// vertical chop
					if( MONSTER_ATTACKTIME == 0 ) {
						MONSTER_ARMBENDED = 0;
						MONSTER_WEAPONYAW = 0;
						entity->pitch = -3*PI/4;
						entity->roll = 0;
					} else {
						if( entity->pitch >= -PI/2 )
							MONSTER_ARMBENDED = 1;
						if( entity->pitch >= PI/4 ) {
							entity->skill[0] = rightbody->skill[0];
							MONSTER_WEAPONYAW = 0;
							entity->pitch = rightbody->pitch;
							entity->roll = 0;
							MONSTER_ARMBENDED = 0;
						} else {
							entity->pitch += .25;
						}
					}
				}
			}

			if( bodypart!=5 || (MONSTER_ATTACK==0 && MONSTER_ATTACKTIME==0) ) {
				if( dist>0.1 ) {
					if( entity->skill[0] ) {
						entity->pitch -= dist*TROLLWALKSPEED;
						if( entity->pitch < -PI/4.0 ) {
							entity->skill[0]=0;
							entity->pitch = -PI/4.0;
						}
					} else {
						entity->pitch += dist*TROLLWALKSPEED;
						if( entity->pitch > PI/4.0 ) {
							entity->skill[0]=1;
							entity->pitch = PI/4.0;
						}
					}
				} else {
					if( entity->pitch < 0 ) {
						entity->pitch += 1/fmax(dist*.1,10.0);
						if( entity->pitch > 0 )
							entity->pitch=0;
					} else if( entity->pitch > 0 ) {
						entity->pitch -= 1/fmax(dist*.1,10.0);
						if( entity->pitch < 0 )
							entity->pitch=0;
					}
				}
			}
		}
		switch( bodypart ) {
		// torso
		case 2:
			entity->x-=.5*cos(my->yaw);
			entity->y-=.5*sin(my->yaw);
			entity->z+=2.25;
			break;
		// right leg
		case 3:
			entity->x+=2*cos(my->yaw+PI/2)-1.25*cos(my->yaw);
			entity->y+=2*sin(my->yaw+PI/2)-1.25*sin(my->yaw);
			entity->z+=5;
			if( my->z >= 1.4 && my->z <= 1.6 ) {
				entity->yaw += PI/8;
				entity->pitch = -PI/2;
			}
			break;
		// left leg
		case 4:
			entity->x-=2*cos(my->yaw+PI/2)+1.25*cos(my->yaw);
			entity->y-=2*sin(my->yaw+PI/2)+1.25*sin(my->yaw);
			entity->z+=5;
			if( my->z >= 1.4 && my->z <= 1.6 ) {
				entity->yaw -= PI/8;
				entity->pitch = -PI/2;
			}
			break;
		// right arm
		case 5:
			entity->x+=3.5*cos(my->yaw+PI/2)-1*cos(my->yaw);
			entity->y+=3.5*sin(my->yaw+PI/2)-1*sin(my->yaw);
			entity->z+=.1;
			entity->yaw += MONSTER_WEAPONYAW;
			if( my->z >= 1.4 && my->z <= 1.6 ) {
				entity->pitch = 0;
			}
			break;
		// left arm
		case 6:
			entity->x-=3.5*cos(my->yaw+PI/2)+1*cos(my->yaw);
			entity->y-=3.5*sin(my->yaw+PI/2)+1*sin(my->yaw);
			entity->z+=.1;
			if( my->z >= 1.4 && my->z <= 1.6 ) {
				entity->pitch = 0;
			}
			break;
		}
	}
	if( MONSTER_ATTACK != 0 )
		MONSTER_ATTACKTIME++;
	else
		MONSTER_ATTACKTIME=0;
}
