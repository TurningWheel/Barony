/*-------------------------------------------------------------------------------

	BARONY
	File: monster_gnome.cpp
	Desc: implements all of the gnome monster's code

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
#include "book.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"

void initGnome(Entity *my, Stat *myStats) {
	int c;
	node_t *node;

	my->sprite = 295; //Gnome head model

	my->flags[UPDATENEEDED]=TRUE;
	my->flags[BLOCKSIGHT]=TRUE;
	my->flags[INVISIBLE]=FALSE;

	if( multiplayer!=CLIENT ) {
		MONSTER_SPOTSND = 220;
		MONSTER_SPOTVAR = 5;
		MONSTER_IDLESND = 217;
		MONSTER_IDLEVAR = 3;
	}
	if( multiplayer!=CLIENT && !MONSTER_INIT ) {
		myStats->sex = static_cast<sex_t>(rand()%2);
		myStats->appearance = rand();
		strcpy(myStats->name,"");
		myStats->inventory.first = NULL;
		myStats->inventory.last = NULL;
		myStats->HP = 50; myStats->MAXHP = 50;
		myStats->MP = 50; myStats->MAXMP = 50;
		myStats->OLDHP = myStats->HP;
		myStats->STR = 2;
		myStats->DEX = 0;
		myStats->CON = 4;
		myStats->INT = 0;
		myStats->PER = 2;
		myStats->CHR = -1;
		myStats->EXP = 0;
		myStats->LVL = 5;
		myStats->GOLD = 40+rand()%20;
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
		myStats->PROFICIENCIES[PRO_SWORD]=35;
		myStats->PROFICIENCIES[PRO_MACE]=50;
		myStats->PROFICIENCIES[PRO_AXE]=45;
		myStats->PROFICIENCIES[PRO_POLEARM]=25;
		myStats->PROFICIENCIES[PRO_RANGED]=35;
		myStats->PROFICIENCIES[PRO_SHIELD]=35;
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

		if( rand()%8==0 ) {
			myStats->EFFECTS[EFF_ASLEEP] = TRUE;
			myStats->EFFECTS_TIMERS[EFF_ASLEEP] = 1800+rand()%1800;
		}

		if( rand()%3==0 )
			newItem( FOOD_FISH, EXCELLENT, 0, 1, rand(), FALSE, &myStats->inventory );

		if( rand()%50==0 )
			newItem( READABLE_BOOK, EXCELLENT, 0, 1, getBook("Winny's Report"), FALSE, &myStats->inventory );

		if( rand()%10==0 ) {
			int i = 1+rand()%4;
			for( c=0; c<i; c++ )
				newItem( static_cast<ItemType>(GEM_GARNET+rand()%15), static_cast<Status>(1+rand()%4), 0, 1, rand(), FALSE, &myStats->inventory );
		}

		if( rand()%50==0 && !my->flags[USERFLAG2] ) {
			strcpy(myStats->name,"Rumplewort");
			myStats->LVL += 10;
			newItem( GEM_DIAMOND, static_cast<Status>(1+rand()%4), 0, 1, rand(), TRUE, &myStats->inventory );

			int c;
			for( c=0; c<3; c++ ) {
				Entity *entity = summonMonster(GNOME,my->x,my->y);
				if( entity )
					entity->parent = my->uid;
			}
		}
	}

	// torso
	Entity *entity = newEntity(296, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->uid;
	entity->flags[PASSABLE]=TRUE;
	entity->flags[NOUPDATE]=TRUE;
	entity->flags[USERFLAG2]=my->flags[USERFLAG2];
	entity->focalx = limbs[GNOME][1][0]; // 0
	entity->focaly = limbs[GNOME][1][1]; // 0
	entity->focalz = limbs[GNOME][1][2]; // 0
	entity->behavior=&actGnomeLimb;
	entity->parent=my->uid;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity *);

	// right leg
	entity = newEntity(297, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->uid;
	entity->flags[PASSABLE]=TRUE;
	entity->flags[NOUPDATE]=TRUE;
	entity->flags[USERFLAG2]=my->flags[USERFLAG2];
	entity->focalx = limbs[GNOME][2][0]; // .25
	entity->focaly = limbs[GNOME][2][1]; // 0
	entity->focalz = limbs[GNOME][2][2]; // 1.5
	entity->behavior=&actGnomeLimb;
	entity->parent=my->uid;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity *);

	// left leg
	entity = newEntity(298, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->uid;
	entity->flags[PASSABLE]=TRUE;
	entity->flags[NOUPDATE]=TRUE;
	entity->flags[USERFLAG2]=my->flags[USERFLAG2];
	entity->focalx = limbs[GNOME][3][0]; // .25
	entity->focaly = limbs[GNOME][3][1]; // 0
	entity->focalz = limbs[GNOME][3][2]; // 1.5
	entity->behavior=&actGnomeLimb;
	entity->parent=my->uid;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity *);

	// right arm
	entity = newEntity(299, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->uid;
	entity->flags[PASSABLE]=TRUE;
	entity->flags[NOUPDATE]=TRUE;
	entity->flags[USERFLAG2]=my->flags[USERFLAG2];
	entity->focalx = limbs[GNOME][4][0]; // 0
	entity->focaly = limbs[GNOME][4][1]; // 0
	entity->focalz = limbs[GNOME][4][2]; // 2
	entity->behavior=&actGnomeLimb;
	entity->parent=my->uid;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity *);

	// left arm
	entity = newEntity(301, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->uid;
	entity->flags[PASSABLE]=TRUE;
	entity->flags[NOUPDATE]=TRUE;
	entity->flags[USERFLAG2]=my->flags[USERFLAG2];
	entity->focalx = limbs[GNOME][5][0]; // 0
	entity->focaly = limbs[GNOME][5][1]; // 0
	entity->focalz = limbs[GNOME][5][2]; // 2
	entity->behavior=&actGnomeLimb;
	entity->parent=my->uid;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity *);

	// world weapon
	entity = newEntity(-1, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->uid;
	entity->flags[PASSABLE]=TRUE;
	entity->flags[NOUPDATE]=TRUE;
	entity->flags[INVISIBLE]=TRUE;
	entity->flags[USERFLAG2]=my->flags[USERFLAG2];
	entity->focalx = limbs[GNOME][6][0]; // 2
	entity->focaly = limbs[GNOME][6][1]; // 0
	entity->focalz = limbs[GNOME][6][2]; // -.5
	entity->behavior=&actGnomeLimb;
	entity->parent=my->uid;
	entity->pitch=.25;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity *);

	// shield
	entity = newEntity(-1, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->uid;
	entity->flags[PASSABLE]=TRUE;
	entity->flags[NOUPDATE]=TRUE;
	entity->flags[INVISIBLE]=TRUE;
	entity->flags[USERFLAG2]=my->flags[USERFLAG2];
	entity->focalx = limbs[GNOME][7][0]; // 0
	entity->focaly = limbs[GNOME][7][1]; // 0
	entity->focalz = limbs[GNOME][7][2]; // 1.5
	entity->behavior=&actGnomeLimb;
	entity->parent=my->uid;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity *);

	// cloak
	entity = newEntity(-1, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->uid;
	entity->scalex = 1.01;
	entity->scaley = 1.01;
	entity->scalez = 1.01;
	entity->flags[PASSABLE]=TRUE;
	entity->flags[NOUPDATE]=TRUE;
	entity->flags[INVISIBLE]=TRUE;
	entity->flags[USERFLAG2]=my->flags[USERFLAG2];
	entity->focalx = limbs[GNOME][8][0]; // 0
	entity->focaly = limbs[GNOME][8][1]; // 0
	entity->focalz = limbs[GNOME][8][2]; // 4
	entity->behavior=&actGnomeLimb;
	entity->parent=my->uid;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity *);
	
	if( multiplayer==CLIENT || MONSTER_INIT )
		return;
	
	// give shield
	switch( rand()%10 ) {
		case 0:
		case 1:
			myStats->shield = newItem(TOOL_LANTERN,EXCELLENT,-1+rand()%3,1,rand(),FALSE,NULL);
			break;
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			break;
		case 7:
		case 8:
		case 9:
			myStats->shield = newItem(WOODEN_SHIELD,static_cast<Status>(WORN+rand()%2),-1+rand()%3,1,rand(),FALSE,NULL);
			break;
	}
	
	// give weapon
	switch( rand()%10 ) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
			myStats->weapon = newItem(TOOL_PICKAXE,EXCELLENT,-1+rand()%3,1,rand(),FALSE,NULL);
			break;
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
			myStats->GOLD += 100;
			myStats->weapon = newItem(MAGICSTAFF_LIGHTNING,EXCELLENT,-1+rand()%3,1,rand(),FALSE,NULL);
			break;
	}
	
	// give cloak
	switch( rand()%10 ) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			break;
		case 6:
		case 7:
		case 8:
		case 9:
			myStats->cloak = newItem(CLOAK,SERVICABLE,-1+rand()%3,1,rand(),FALSE,NULL);
			break;
	}
}

void actGnomeLimb(Entity *my) {
	int i;
	
	Entity *parent = NULL;
	if( (parent=uidToEntity(my->skill[2]))==NULL ) {
		list_RemoveNode(my->mynode);
		return;
	}
	
	if( my->light != NULL ) {
		list_RemoveNode(my->light->node);
		my->light = NULL;
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

	int torch = 0;
	if( my->flags[INVISIBLE] == FALSE ) {
		if( my->sprite == 93 ) { // torch
			torch = 6;
		} else if( my->sprite == 94 ) { // lantern
			torch = 9;
		}
	}
	if( torch != 0 ) {
		my->light = lightSphereShadow(my->x/16,my->y/16,torch,50+15*torch);
	}
}

void gnomeDie(Entity *my) {
	node_t *node, *nextnode;
	int c;
	for( c=0; c<6; c++ ) {
		Entity *entity = spawnGib(my);
		if( entity )
			serverSpawnGibForClient(entity);
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
	int i = 0;
	for( node=my->children.first; node!=NULL; node=nextnode ) {
		nextnode = node->next;
		if( node->element != NULL && i >= 2 ) {
			Entity *entity=(Entity *)node->element;
			if( entity->light != NULL )
				list_RemoveNode(entity->light->node);
			entity->light = NULL;
			list_RemoveNode(entity->mynode);
		}
		list_RemoveNode(node);
		i++;
	}
	playSoundEntity(my,225+rand()%4,128);
	list_RemoveNode(my->mynode);
	return;
}

#define GNOMEWALKSPEED .13

void gnomeMoveBodyparts(Entity *my, Stat *myStats, double dist) {
	node_t *node;
	Entity *entity = NULL, *entity2 = NULL;
	Entity *rightbody = NULL;
	Entity *weaponarm=NULL;
	int bodypart;
	bool wearingring=FALSE;
	
	// set invisibility
	if( multiplayer != CLIENT ) {
		if( myStats->ring != NULL )
			if( myStats->ring->type == RING_INVISIBILITY )
				wearingring = TRUE;
		if( myStats->cloak != NULL )
			if( myStats->cloak->type == CLOAK_INVISIBILITY )
				wearingring = TRUE;
		if( myStats->EFFECTS[EFF_INVISIBLE] == TRUE || wearingring==TRUE ) {
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
			my->z = 4;
			my->pitch = PI/4;
		} else {
			my->z = 2.25;
			my->pitch = 0;
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
			node_t *shieldNode = list_Node(&my->children,7);
			if( shieldNode ) {
				Entity *shield = (Entity *)shieldNode->element;
				if( dist>0.1 && (bodypart!=6||shield->flags[INVISIBLE]) ) {
					if( !rightbody->skill[0] ) {
						entity->pitch -= dist*GNOMEWALKSPEED;
						if( entity->pitch < -PI/4.0 ) {
							entity->pitch = -PI/4.0;
							if(bodypart==3) {
								entity->skill[0]=1;
								if( dist>.4 )
									playSoundEntityLocal(my,rand()%7,32);
							}
						}
					} else {
						entity->pitch += dist*GNOMEWALKSPEED;
						if( entity->pitch > PI/4.0 ) {
							entity->pitch = PI/4.0;
							if(bodypart==3) {
								entity->skill[0]=0;
								if( dist>.4 )
									playSoundEntityLocal(my,rand()%7,32);
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
			}
		} else if( bodypart==4||bodypart==5||bodypart==9 ) {
			if( bodypart==5 ) {
				weaponarm = entity;
				if( MONSTER_ATTACK == 1 ) {
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
				} else if( MONSTER_ATTACK == 2 ) {
					// horizontal chop
					if( MONSTER_ATTACKTIME == 0 ) {
						MONSTER_ARMBENDED = 1;
						MONSTER_WEAPONYAW = -3*PI/4;
						entity->pitch = 0;
						entity->roll = -PI/2;
					} else {
						if( MONSTER_WEAPONYAW >= PI/8 ) {
							entity->skill[0] = rightbody->skill[0];
							MONSTER_WEAPONYAW = 0;
							entity->pitch = rightbody->pitch;
							entity->roll = 0;
							MONSTER_ARMBENDED = 0;
							MONSTER_ATTACK = 0;
						} else {
							MONSTER_WEAPONYAW += .25;
						}
					}
				} else if( MONSTER_ATTACK == 3 ) {
					// stab
					if( MONSTER_ATTACKTIME == 0 ) {
						MONSTER_ARMBENDED = 0;
						MONSTER_WEAPONYAW = 0;
						entity->pitch = 2*PI/3;
						entity->roll = 0;
					} else {
						if( MONSTER_ATTACKTIME >= 5 ) {
							MONSTER_ARMBENDED = 1;
							entity->pitch = -PI/6;
						}
						if( MONSTER_ATTACKTIME >= 10 ) {
							entity->skill[0] = rightbody->skill[0];
							MONSTER_WEAPONYAW = 0;
							entity->pitch = rightbody->pitch;
							entity->roll = 0;
							MONSTER_ARMBENDED = 0;
							MONSTER_ATTACK = 0;
						}
					}
				}
			} else if( bodypart==9 ) {
				entity->pitch=entity->fskill[0];
			}
			
			if( bodypart!=5 || (MONSTER_ATTACK==0 && MONSTER_ATTACKTIME==0) ) {
				if( dist>0.1 ) {
					if( entity->skill[0] ) {
						entity->pitch -= dist*GNOMEWALKSPEED;
						if( entity->pitch < -PI/4.0 ) {
							entity->skill[0]=0;
							entity->pitch = -PI/4.0;
						}
					} else {
						entity->pitch += dist*GNOMEWALKSPEED;
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
			if( bodypart==9 ) {
				entity->fskill[0]=entity->pitch;
				entity->roll=my->roll-fabs(entity->pitch)/2;
				entity->pitch=0;
			}
		}
		switch( bodypart ) {
			// torso
			case 2:
				entity->x-=.25*cos(my->yaw);
				entity->y-=.25*sin(my->yaw);
				entity->z+=1.25;
				break;
			// right leg
			case 3:
				entity->x+=1.25*cos(my->yaw+PI/2);
				entity->y+=1.25*sin(my->yaw+PI/2);
				entity->z+=2.75;
				if( my->z >= 3.9 && my->z <= 4.1 ) {
					entity->yaw += PI/8;
					entity->pitch = -PI/2;
				}
				break;
			// left leg
			case 4:
				entity->x-=1.25*cos(my->yaw+PI/2);
				entity->y-=1.25*sin(my->yaw+PI/2);
				entity->z+=2.75;
				if( my->z >= 3.9 && my->z <= 4.1 ) {
					entity->yaw -= PI/8;
					entity->pitch = -PI/2;
				}
				break;
			// right arm
			case 5: {
				;
				node_t *weaponNode = list_Node(&my->children,7);
				if( weaponNode ) {
					Entity *weapon = (Entity *)weaponNode->element;
					if( weapon->flags[INVISIBLE] || MONSTER_ARMBENDED ) {
						entity->focalx = limbs[GNOME][4][0]; // 0
						entity->focaly = limbs[GNOME][4][1]; // 0
						entity->focalz = limbs[GNOME][4][2]; // 2
						entity->sprite = 299;
					} else {
						entity->focalx = limbs[GNOME][4][0] + 1; // 1
						entity->focaly = limbs[GNOME][4][1]; // 0
						entity->focalz = limbs[GNOME][4][2] - 1; // 1
						entity->sprite = 300;
					}
				}
				entity->x+=2.5*cos(my->yaw+PI/2)-.75*cos(my->yaw);
				entity->y+=2.5*sin(my->yaw+PI/2)-.75*sin(my->yaw);
				entity->z-=.25;
				entity->yaw += MONSTER_WEAPONYAW;
				if( my->z >= 3.9 && my->z <= 4.1 ) {
					entity->pitch = 0;
				}
				break;
			// left arm
			}
			case 6: {
				;
				node_t *shieldNode = list_Node(&my->children,8);
				if( shieldNode ) {
					Entity *shield = (Entity *)shieldNode->element;
					if( shield->flags[INVISIBLE] ) {
						entity->focalx = limbs[GNOME][5][0]; // 0
						entity->focaly = limbs[GNOME][5][1]; // 0
						entity->focalz = limbs[GNOME][5][2]; // 2
						entity->sprite=301;
					} else {
						entity->focalx = limbs[GNOME][5][0] + 1; // 1
						entity->focaly = limbs[GNOME][5][1]; // 0
						entity->focalz = limbs[GNOME][5][2] - 1; // 1
						entity->sprite=302;
					}
				}
				entity->x-=2.5*cos(my->yaw+PI/2)+.75*cos(my->yaw);
				entity->y-=2.5*sin(my->yaw+PI/2)+.75*sin(my->yaw);
				entity->z-=.25;
				if( my->z >= 3.9 && my->z <= 4.1 ) {
					entity->pitch = 0;
				}
				break;
			}
			// weapon
			case 7:
				if( multiplayer!=CLIENT ) {
					if( myStats->weapon == NULL || myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) {
						entity->flags[INVISIBLE]=TRUE;
					} else {
						entity->sprite = itemModel(myStats->weapon);
						if( itemCategory(myStats->weapon) == SPELLBOOK )
							entity->flags[INVISIBLE]=TRUE;
						else
							entity->flags[INVISIBLE]=FALSE;
					}
					if( multiplayer==SERVER ) {
						// update sprites for clients
						if( entity->skill[10]!=entity->sprite ) {
							entity->skill[10]=entity->sprite;
							serverUpdateEntityBodypart(my,bodypart);
						}
						if( entity->skill[11] != entity->flags[INVISIBLE] ) {
							entity->skill[11] = entity->flags[INVISIBLE];
							serverUpdateEntityBodypart(my,bodypart);
						}
						if( entity->uid%(TICKS_PER_SECOND*10) == ticks%(TICKS_PER_SECOND*10) ) {
							serverUpdateEntityBodypart(my,bodypart);
						}
					}
				}
				if( weaponarm != NULL ) {
					if( entity->flags[INVISIBLE] != TRUE ) {
						if( entity->sprite == items[SHORTBOW].index ) {
							entity->x=weaponarm->x-.5*cos(weaponarm->yaw);
							entity->y=weaponarm->y-.5*sin(weaponarm->yaw);
							entity->z=weaponarm->z+1;
							entity->pitch=weaponarm->pitch+.25;
						} else if( entity->sprite == items[ARTIFACT_BOW].index ) {
							entity->x=weaponarm->x-1.5*cos(weaponarm->yaw);
							entity->y=weaponarm->y-1.5*sin(weaponarm->yaw);
							entity->z=weaponarm->z+2;
							entity->pitch=weaponarm->pitch+.25;
						} else if( entity->sprite == items[CROSSBOW].index ) {
							entity->x=weaponarm->x;
							entity->y=weaponarm->y;
							entity->z=weaponarm->z+1;
							entity->pitch=weaponarm->pitch;
						} else {
							entity->x=weaponarm->x+.5*cos(weaponarm->yaw)*(MONSTER_ATTACK==0);
							entity->y=weaponarm->y+.5*sin(weaponarm->yaw)*(MONSTER_ATTACK==0);
							entity->z=weaponarm->z-.5*(MONSTER_ATTACK==0);
							entity->pitch=weaponarm->pitch+.25*(MONSTER_ATTACK==0);
						}
					}
					entity->yaw=weaponarm->yaw;
					entity->roll=weaponarm->roll;
					if( !MONSTER_ARMBENDED ) {
						entity->focalx=limbs[GNOME][6][0];
						if( entity->sprite == items[CROSSBOW].index )
							entity->focalx+=2;
						entity->focaly=limbs[GNOME][6][1];
						entity->focalz=limbs[GNOME][6][2];
					} else {
						entity->focalx=limbs[GNOME][6][0]+1;
						entity->focaly=limbs[GNOME][6][1];
						entity->focalz=limbs[GNOME][6][2]-2;
						entity->yaw -= sin(weaponarm->roll)*PI/2;
						entity->pitch += cos(weaponarm->roll)*PI/2;
					}
				}
				break;
			// shield
			case 8:
				if( multiplayer!=CLIENT ) {
					if( myStats->shield == NULL ) {
						entity->flags[INVISIBLE]=TRUE;
						entity->sprite = 0;
					} else {
						entity->flags[INVISIBLE]=FALSE;
						entity->sprite = itemModel(myStats->shield);
					}
					if( myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) {
						entity->flags[INVISIBLE]=TRUE;
					}
					if( multiplayer==SERVER ) {
						// update sprites for clients
						if( entity->skill[10]!=entity->sprite ) {
							entity->skill[10]=entity->sprite;
							serverUpdateEntityBodypart(my,bodypart);
						}
						if( entity->skill[11] != entity->flags[INVISIBLE] ) {
							entity->skill[11] = entity->flags[INVISIBLE];
							serverUpdateEntityBodypart(my,bodypart);
						}
						if( entity->uid%(TICKS_PER_SECOND*10) == ticks%(TICKS_PER_SECOND*10) ) {
							serverUpdateEntityBodypart(my,bodypart);
						}
					}
				}
				entity->x-=2.5*cos(my->yaw+PI/2)+.20*cos(my->yaw);
				entity->y-=2.5*sin(my->yaw+PI/2)+.20*sin(my->yaw);
				entity->z+=1;
				if( entity->sprite == items[TOOL_TORCH].index ) {
					entity2 = spawnFlame(entity);
					entity2->x += 2*cos(my->yaw);
					entity2->y += 2*sin(my->yaw);
					entity2->z -= 2;
				} else if( entity->sprite == items[TOOL_LANTERN].index ) {
					entity->z += 2;
					entity2 = spawnFlame(entity);
					entity2->x += 2*cos(my->yaw);
					entity2->y += 2*sin(my->yaw);
					entity2->z += 1;
				}
				break;
			// cloak
			case 9:
				if( multiplayer!=CLIENT ) {
					if( myStats->cloak == NULL || myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) {
						entity->flags[INVISIBLE]=TRUE;
					} else {
						entity->flags[INVISIBLE]=FALSE;
						entity->sprite = itemModel(myStats->cloak);
					}
					if( multiplayer==SERVER ) {
						// update sprites for clients
						if( entity->skill[10]!=entity->sprite ) {
							entity->skill[10]=entity->sprite;
							serverUpdateEntityBodypart(my,bodypart);
						}
						if( entity->skill[11] != entity->flags[INVISIBLE] ) {
							entity->skill[11] = entity->flags[INVISIBLE];
							serverUpdateEntityBodypart(my,bodypart);
						}
						if( entity->uid%(TICKS_PER_SECOND*10) == ticks%(TICKS_PER_SECOND*10) ) {
							serverUpdateEntityBodypart(my,bodypart);
						}
					}
				}
				entity->x-=cos(my->yaw)*1.5;
				entity->y-=sin(my->yaw)*1.5;
				entity->yaw+=PI/2;
				break;
		}
	}
	if( MONSTER_ATTACK != 0 )
		MONSTER_ATTACKTIME++;
	else
		MONSTER_ATTACKTIME=0;
}
