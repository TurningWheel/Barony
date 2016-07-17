/*-------------------------------------------------------------------------------

	BARONY
	File: monster_imp.cpp
	Desc: implements all of the imp monster's code

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

void initImp(Entity *my, Stat *myStats) {
	int c;
	node_t *node;

	my->sprite = 289;

	my->flags[UPDATENEEDED]=TRUE;
	my->flags[BLOCKSIGHT]=TRUE;
	my->flags[INVISIBLE]=FALSE;

	if( multiplayer!=CLIENT ) {
		MONSTER_SPOTSND = 198;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = 201;
		MONSTER_IDLEVAR = 3;
	}
	if( multiplayer!=CLIENT && !MONSTER_INIT ) {
		myStats->sex = static_cast<sex_t>(rand()%2);
		myStats->appearance = rand();
		strcpy(myStats->name,"");
		myStats->inventory.first = NULL;
		myStats->inventory.last = NULL;
		myStats->HP = 80; myStats->MAXHP = myStats->HP;
		myStats->MP = 80; myStats->MAXMP = 80;
		myStats->OLDHP = myStats->HP;
		myStats->STR = 20;
		myStats->DEX = 7;
		myStats->CON = 9;
		myStats->INT = -2;
		myStats->PER = 50;
		myStats->CHR = -3;
		myStats->EXP = 0;
		myStats->LVL = 14;
		if( rand()%10 )
			myStats->GOLD = 0;
		else
			myStats->GOLD = 20+rand()%20;
		myStats->HUNGER = 900;
		if( !myStats->leader_uid )
			myStats->leader_uid = 0;
		myStats->FOLLOWERS.first=NULL; myStats->FOLLOWERS.last=NULL;
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
		myStats->weapon = newItem(SPELLBOOK_FIREBALL,EXCELLENT,0,1,0,FALSE,NULL);
		myStats->EFFECTS[EFF_LEVITATING] = TRUE;
		myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 0;

		if( rand()%4==0 ) {
			myStats->EFFECTS[EFF_ASLEEP] = TRUE;
			myStats->EFFECTS_TIMERS[EFF_ASLEEP] = 1800+rand()%3600;
		}

		if( rand()%4==0 ) {
			newItem( static_cast<ItemType>(SPELLBOOK_FORCEBOLT+rand()%21), static_cast<Status>(1+rand()%4), -1+rand()%3, 1, rand(), FALSE, &myStats->inventory );
		}
	}

	// torso
	Entity *entity = newEntity(290, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->focaly = 1;
	entity->skill[2] = my->uid;
	entity->flags[PASSABLE]=TRUE;
	entity->flags[NOUPDATE]=TRUE;
	entity->flags[USERFLAG2]=my->flags[USERFLAG2];
	entity->focalx = limbs[CREATURE_IMP][1][0]; // 0
	entity->focaly = limbs[CREATURE_IMP][1][1]; // 1
	entity->focalz = limbs[CREATURE_IMP][1][2]; // 0
	entity->behavior=&actImpLimb;
	entity->parent=my->uid;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity *);

	// right leg
	entity = newEntity(292, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->uid;
	entity->flags[PASSABLE]=TRUE;
	entity->flags[NOUPDATE]=TRUE;
	entity->flags[USERFLAG2]=my->flags[USERFLAG2];
	entity->focalx = limbs[CREATURE_IMP][2][0]; // 0
	entity->focaly = limbs[CREATURE_IMP][2][1]; // 0
	entity->focalz = limbs[CREATURE_IMP][2][2]; // 2
	entity->behavior=&actImpLimb;
	entity->parent=my->uid;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity *);

	// left leg
	entity = newEntity(291, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->uid;
	entity->flags[PASSABLE]=TRUE;
	entity->flags[NOUPDATE]=TRUE;
	entity->flags[USERFLAG2]=my->flags[USERFLAG2];
	entity->focalx = limbs[CREATURE_IMP][3][0]; // 0
	entity->focaly = limbs[CREATURE_IMP][3][1]; // 0
	entity->focalz = limbs[CREATURE_IMP][3][2]; // 2
	entity->behavior=&actImpLimb;
	entity->parent=my->uid;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity *);

	// right arm
	entity = newEntity(294, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->uid;
	entity->flags[PASSABLE]=TRUE;
	entity->flags[NOUPDATE]=TRUE;
	entity->flags[USERFLAG2]=my->flags[USERFLAG2];
	entity->focalx = limbs[CREATURE_IMP][4][0]; // 0
	entity->focaly = limbs[CREATURE_IMP][4][1]; // 0
	entity->focalz = limbs[CREATURE_IMP][4][2]; // 3
	entity->behavior=&actImpLimb;
	entity->parent=my->uid;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity *);

	// left arm
	entity = newEntity(293, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->uid;
	entity->flags[PASSABLE]=TRUE;
	entity->flags[NOUPDATE]=TRUE;
	entity->flags[USERFLAG2]=my->flags[USERFLAG2];
	entity->focalx = limbs[CREATURE_IMP][5][0]; // 0
	entity->focaly = limbs[CREATURE_IMP][5][1]; // 0
	entity->focalz = limbs[CREATURE_IMP][5][2]; // 3
	entity->behavior=&actImpLimb;
	entity->parent=my->uid;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity *);

	// right wing
	entity = newEntity(310, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->uid;
	entity->flags[PASSABLE]=TRUE;
	entity->flags[NOUPDATE]=TRUE;
	entity->flags[USERFLAG2]=my->flags[USERFLAG2];
	entity->focalx = limbs[CREATURE_IMP][6][0]; // 0
	entity->focaly = limbs[CREATURE_IMP][6][1]; // 4
	entity->focalz = limbs[CREATURE_IMP][6][2]; // 0
	entity->behavior=&actImpLimb;
	entity->parent=my->uid;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity *);

	// left wing
	entity = newEntity(309, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->uid;
	entity->flags[PASSABLE]=TRUE;
	entity->flags[NOUPDATE]=TRUE;
	entity->flags[USERFLAG2]=my->flags[USERFLAG2];
	entity->focalx = limbs[CREATURE_IMP][7][0]; // 0
	entity->focaly = limbs[CREATURE_IMP][7][1]; // -4
	entity->focalz = limbs[CREATURE_IMP][7][2]; // 0
	entity->behavior=&actImpLimb;
	entity->parent=my->uid;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity *);
}

void actImpLimb(Entity *my) {
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
				}
				else if( client_selected[i]==my ) {
					parent->skill[13] = i+1;
				}
			}
		}
	}
	return;
}

void impDie(Entity *my) {
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
	playSoundEntity(my, 28, 128);
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

#define IMPWALKSPEED .01

void impMoveBodyparts(Entity *my, Stat *myStats, double dist) {
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
			my->pitch = PI/4;
		} else {
			my->pitch = 0;
		}

		// imps are always flying
		myStats->EFFECTS[EFF_LEVITATING] = TRUE;
		myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 0;
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
				if( !rightbody->skill[0] ) {
					entity->pitch -= IMPWALKSPEED;
					if( entity->pitch < -PI/8.0 ) {
						entity->pitch = -PI/8.0;
						if(bodypart==3)
							entity->skill[0]=1;
					}
				} else {
					entity->pitch += IMPWALKSPEED;
					if( entity->pitch > PI/8.0 ) {
						entity->pitch = PI/8.0;
						if(bodypart==3)
							entity->skill[0]=0;
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
				if( entity->skill[0] ) {
					entity->pitch -= IMPWALKSPEED;
					if( entity->pitch < -PI/8.0 ) {
						entity->skill[0]=0;
						entity->pitch = -PI/8.0;
					}
				} else {
					entity->pitch += IMPWALKSPEED;
					if( entity->pitch > PI/8.0 ) {
						entity->skill[0]=1;
						entity->pitch = PI/8.0;
					}
				}
			}
		} else if( bodypart==7 || bodypart==8 ) {
			entity->fskill[1] += .1;
			if( entity->fskill[1] >= PI*2 )
				entity->fskill[1] -= PI*2;
		}
		switch( bodypart ) {
			// torso
			case 2:
				entity->x-=2*cos(my->yaw);
				entity->y-=2*sin(my->yaw);
				entity->z+=2.75;
				break;
			// right leg
			case 3:
				entity->x+=1*cos(my->yaw+PI/2);
				entity->y+=1*sin(my->yaw+PI/2);
				entity->z+=6;
				break;
			// left leg
			case 4:
				entity->x-=1*cos(my->yaw+PI/2);
				entity->y-=1*sin(my->yaw+PI/2);
				entity->z+=6;
				break;
			// right arm
			case 5:
				entity->x+=3*cos(my->yaw+PI/2)-1*cos(my->yaw);
				entity->y+=3*sin(my->yaw+PI/2)-1*sin(my->yaw);
				entity->z+=1;
				entity->yaw += MONSTER_WEAPONYAW;
				break;
			// left arm
			case 6:
				entity->x-=3*cos(my->yaw+PI/2)+1*cos(my->yaw);
				entity->y-=3*sin(my->yaw+PI/2)+1*sin(my->yaw);
				entity->z+=1;
				break;
			// right wing
			case 7:
				entity->x+=1*cos(my->yaw+PI/2)-2.5*cos(my->yaw);
				entity->y+=1*sin(my->yaw+PI/2)-2.5*sin(my->yaw);
				entity->z+=1;
				entity->yaw += cos(entity->fskill[1])*PI/6+PI/6;
				break;
			// left wing
			case 8:
				entity->x-=1*cos(my->yaw+PI/2)+2.5*cos(my->yaw);
				entity->y-=1*sin(my->yaw+PI/2)+2.5*sin(my->yaw);
				entity->z+=1;
				entity->yaw -= cos(entity->fskill[1])*PI/6+PI/6;
				break;
		}
	}
	if( MONSTER_ATTACK != 0 )
		MONSTER_ATTACKTIME++;
	else
		MONSTER_ATTACKTIME=0;
}
