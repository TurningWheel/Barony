/*-------------------------------------------------------------------------------

	BARONY
	File: monster_spider.cpp
	Desc: implements all of the spider monster's code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "monster.hpp"
#include "sound.hpp"
#include "net.hpp"
#include "items.hpp"
#include "collision.hpp"
#include "player.hpp"

void initSpider(Entity* my, Stat* myStats) {
	int c;

	my->flags[UPDATENEEDED] = TRUE;
	my->flags[INVISIBLE] = FALSE;

	my->sprite = 267;
	if ( multiplayer != CLIENT ) {
		MONSTER_SPOTSND = 229;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = 232;
		MONSTER_IDLEVAR = 4;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT ) {
		myStats->sex = static_cast<sex_t>(rand() % 2);
		myStats->appearance = rand();
		strcpy(myStats->name, "");
		myStats->inventory.first = NULL;
		myStats->inventory.last = NULL;
		myStats->HP = 50;
		myStats->MAXHP = 50;
		myStats->MP = 10;
		myStats->MAXMP = 10;
		myStats->OLDHP = myStats->HP;
		myStats->STR = 3;
		myStats->DEX = 8;
		myStats->CON = 4;
		myStats->INT = -3;
		myStats->PER = -3;
		myStats->CHR = -1;
		myStats->EXP = 0;
		myStats->LVL = 5;
		myStats->GOLD = 0;
		myStats->HUNGER = 900;
		if ( !myStats->leader_uid ) {
			myStats->leader_uid = 0;
		}
		myStats->FOLLOWERS.first = NULL;
		myStats->FOLLOWERS.last = NULL;
		for ( c = 0; c < std::max(NUMPROFICIENCIES, NUMEFFECTS); c++ ) {
			if ( c < NUMPROFICIENCIES ) {
				myStats->PROFICIENCIES[c] = 0;
			}
			if ( c < NUMEFFECTS ) {
				myStats->EFFECTS[c] = FALSE;
			}
			if ( c < NUMEFFECTS ) {
				myStats->EFFECTS_TIMERS[c] = 0;
			}
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

		if ( rand() % 50 == 0 && !my->flags[USERFLAG2] ) {
			strcpy(myStats->name, "Shelob");
			myStats->HP = 150;
			myStats->MAXHP = 150;
			myStats->OLDHP = myStats->HP;
			myStats->STR = 10;
			myStats->DEX = 10;
			myStats->CON = 8;
			myStats->INT = 5;
			myStats->PER = 10;
			myStats->CHR = 10;
			myStats->LVL = 15;
			newItem( RING_INVISIBILITY, EXCELLENT, -5, 1, rand(), FALSE, &myStats->inventory );
			newItem( ARTIFACT_SWORD, EXCELLENT, 1, 1, rand(), FALSE, &myStats->inventory );

			int c;
			for ( c = 0; c < 3; c++ ) {
				Entity* entity = summonMonster(SPIDER, my->x, my->y);
				if ( entity ) {
					entity->parent = my->uid;
				}
			}
		}
	}

	// right pedipalp
	Entity* entity = newEntity(268, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->uid;
	entity->flags[PASSABLE] = TRUE;
	entity->flags[NOUPDATE] = TRUE;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SPIDER][1][0]; // 1
	entity->focaly = limbs[SPIDER][1][1]; // 0
	entity->focalz = limbs[SPIDER][1][2]; // 1
	entity->behavior = &actSpiderLimb;
	entity->parent = my->uid;
	node_t* node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// left pedipalp
	entity = newEntity(268, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->uid;
	entity->flags[PASSABLE] = TRUE;
	entity->flags[NOUPDATE] = TRUE;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SPIDER][2][0]; // 1
	entity->focaly = limbs[SPIDER][2][1]; // 0
	entity->focalz = limbs[SPIDER][2][2]; // 1
	entity->behavior = &actSpiderLimb;
	entity->parent = my->uid;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// eight legs :)
	for ( c = 0; c < 8; c++ ) {
		// "thigh"
		entity = newEntity(269, 0, map.entities);
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = my->uid;
		entity->fskill[2] = (c / 8.f);
		entity->flags[PASSABLE] = TRUE;
		entity->flags[NOUPDATE] = TRUE;
		entity->flags[USERFLAG2] = my->flags[USERFLAG2];
		entity->focalx = limbs[SPIDER][3][0]; // 1
		entity->focaly = limbs[SPIDER][3][1]; // 0
		entity->focalz = limbs[SPIDER][3][2]; // -1
		entity->behavior = &actSpiderLimb;
		entity->parent = my->uid;
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);

		// "shin"
		entity = newEntity(270, 0, map.entities);
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = my->uid;
		entity->flags[PASSABLE] = TRUE;
		entity->flags[NOUPDATE] = TRUE;
		entity->flags[USERFLAG2] = my->flags[USERFLAG2];
		entity->focalx = limbs[SPIDER][4][0]; // 3
		entity->focaly = limbs[SPIDER][4][1]; // 0
		entity->focalz = limbs[SPIDER][4][2]; // 0
		entity->behavior = &actSpiderLimb;
		entity->parent = my->uid;
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
	}
}

void spiderDie(Entity* my) {
	node_t* node, *nextnode;

	int c = 0;
	for ( c = 0; c < 5; c++ ) {
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}
	if (spawn_blood) {
		int x, y;
		x = std::min<unsigned int>(std::max<int>(0, my->x / 16), map.width - 1);
		y = std::min<unsigned int>(std::max<int>(0, my->y / 16), map.height - 1);
		if ( map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height] ) {
			if ( !checkObstacle(my->x, my->y, my, NULL) ) {
				Entity* entity = newEntity(212, 1, map.entities);
				entity->x = my->x;
				entity->y = my->y;
				entity->z = 7.4 + (rand() % 20) / 100.f;
				entity->parent = my->uid;
				entity->sizex = 2;
				entity->sizey = 2;
				entity->yaw = (rand() % 360) * PI / 180.0;
				entity->flags[UPDATENEEDED] = TRUE;
				entity->flags[PASSABLE] = TRUE;
			}
		}
	}
	int i = 0;
	for (node = my->children.first; node != NULL; node = nextnode) {
		nextnode = node->next;
		if (node->element != NULL && i >= 2) {
			Entity* entity = (Entity*)node->element;
			entity->flags[UPDATENEEDED] = FALSE;
			list_RemoveNode(entity->mynode);
		}
		list_RemoveNode(node);
		++i;
	}
	playSoundEntity(my, 236 + rand() % 2, 128);
	list_RemoveNode(my->mynode);
	return;
}

void actSpiderLimb(Entity* my) {
	int i;

	Entity* parent = NULL;
	if ( (parent = uidToEntity(my->skill[2])) == NULL ) {
		list_RemoveNode(my->mynode);
		return;
	}

	if ( multiplayer != CLIENT ) {
		for ( i = 0; i < MAXPLAYERS; i++ ) {
			if ( inrange[i] ) {
				if ( i == 0 && selectedEntity == my ) {
					parent->skill[13] = i + 1;
				} else if ( client_selected[i] == my ) {
					parent->skill[13] = i + 1;
				}
			}
		}
	}
	return;
}

void spiderMoveBodyparts(Entity* my, Stat* myStats, double dist) {
	node_t* node;
	Entity* entity;
	int bodypart;

	// set invisibility
	if ( multiplayer != CLIENT ) {
		if ( myStats->EFFECTS[EFF_INVISIBLE] == TRUE ) {
			my->flags[INVISIBLE] = TRUE;
			my->flags[BLOCKSIGHT] = FALSE;
			bodypart = 0;
			for (node = my->children.first; node != NULL; node = node->next) {
				if ( bodypart < 2 ) {
					bodypart++;
					continue;
				}
				entity = (Entity*)node->element;
				if ( !entity->flags[INVISIBLE] ) {
					entity->flags[INVISIBLE] = TRUE;
					serverUpdateEntityBodypart(my, bodypart);
				}
				bodypart++;
			}
		} else {
			my->flags[INVISIBLE] = FALSE;
			my->flags[BLOCKSIGHT] = TRUE;
			bodypart = 0;
			for (node = my->children.first; node != NULL; node = node->next) {
				if ( bodypart < 2 ) {
					bodypart++;
					continue;
				}
				entity = (Entity*)node->element;
				if ( entity->flags[INVISIBLE] ) {
					entity->flags[INVISIBLE] = FALSE;
					serverUpdateEntityBodypart(my, bodypart);
				}
				bodypart++;
			}
		}
	}

	// animate limbs
	for (bodypart = 0, node = my->children.first; node != NULL; node = node->next, bodypart++) {
		if ( bodypart < 2 ) {
			continue;
		}
		entity = (Entity*)node->element;
		Entity* previous = NULL; // previous part
		if ( bodypart > 2 ) {
			previous = (Entity*)node->prev->element;
		}
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		entity->yaw = my->yaw;
		entity->pitch = my->pitch;
		entity->roll = my->roll;

		switch ( bodypart ) {
			// right pedipalp
			case 2:
				entity->x += cos(my->yaw) * 2 + cos(my->yaw + PI / 2) * 2;
				entity->y += sin(my->yaw) * 2 + sin(my->yaw + PI / 2) * 2;
				entity->yaw += PI / 10;
				entity->pitch -= PI / 8;
				break;
			// left pedipalp
			case 3:
				entity->x += cos(my->yaw) * 2 - cos(my->yaw + PI / 2) * 2;
				entity->y += sin(my->yaw) * 2 - sin(my->yaw + PI / 2) * 2;
				entity->yaw -= PI / 10;
				entity->pitch -= PI / 8;
				break;

			// 1st/5th leg:
			// thigh
			case 4:
			case 12:
				entity->x += cos(my->yaw) * 1 + cos(my->yaw + PI / 2) * 2.5 * (1 - 2 * (bodypart > 11));
				entity->y += sin(my->yaw) * 1 + sin(my->yaw + PI / 2) * 2.5 * (1 - 2 * (bodypart > 11));
				if ( dist > 0.1 ) {
					if ( !entity->skill[4] ) {
						entity->fskill[2] += .1;
						if ( entity->fskill[2] >= 1 ) {
							entity->fskill[2] = 1;
							entity->skill[4] = 1;
						}
					} else {
						entity->fskill[2] -= .1;
						if ( entity->fskill[2] <= 0 ) {
							entity->fskill[2] = 0;
							entity->skill[4] = 0;
						}
					}
				}
				entity->z += entity->fskill[2];
				entity->yaw += PI / 6 * (1 - 2 * (bodypart > 11));
				entity->pitch += PI / 4;
				break;
			// shin
			case 5:
			case 13:
				entity->x = previous->x;
				entity->y = previous->y;
				entity->z = previous->z;
				entity->yaw = previous->yaw;
				entity->pitch = previous->pitch;
				entity->x += cos(my->yaw) * 3 + cos(my->yaw + PI / 2) * 2 * (1 - 2 * (bodypart > 11));
				entity->y += sin(my->yaw) * 3 + sin(my->yaw + PI / 2) * 2 * (1 - 2 * (bodypart > 11));
				entity->z += .5;
				entity->pitch += PI / 6 - PI / 4;
				entity->pitch -= (PI / 10) * (previous->z - my->z);
				break;

			// 2nd/6th leg:
			// thigh
			case 6:
			case 14:
				entity->x += cos(my->yaw + PI / 2) * 3 * (1 - 2 * (bodypart > 11));
				entity->y += sin(my->yaw + PI / 2) * 3 * (1 - 2 * (bodypart > 11));
				if ( dist > 0.1 ) {
					if ( !entity->skill[4] ) {
						entity->fskill[2] += .1;
						if ( entity->fskill[2] >= 1 ) {
							entity->fskill[2] = 1;
							entity->skill[4] = 1;
						}
					} else {
						entity->fskill[2] -= .1;
						if ( entity->fskill[2] <= 0 ) {
							entity->fskill[2] = 0;
							entity->skill[4] = 0;
						}
					}
				}
				entity->z += entity->fskill[2];
				entity->yaw += PI / 3 * (1 - 2 * (bodypart > 11));
				entity->pitch += PI / 4;
				break;
			// shin
			case 7:
			case 15:
				entity->x = previous->x;
				entity->y = previous->y;
				entity->z = previous->z;
				entity->yaw = previous->yaw;
				entity->pitch = previous->pitch;
				entity->x += cos(my->yaw) * 1.75 + cos(my->yaw + PI / 2) * 3 * (1 - 2 * (bodypart > 11));
				entity->y += sin(my->yaw) * 1.75 + sin(my->yaw + PI / 2) * 3 * (1 - 2 * (bodypart > 11));
				entity->z += .5;
				entity->pitch += PI / 6 - PI / 4;
				entity->pitch -= (PI / 10) * (previous->z - my->z);
				break;

			// 3rd/7th leg:
			// thigh
			case 8:
			case 16:
				entity->x += cos(my->yaw) * -.5 + cos(my->yaw + PI / 2) * 2 * (1 - 2 * (bodypart > 11));
				entity->y += sin(my->yaw) * -.5 + sin(my->yaw + PI / 2) * 2 * (1 - 2 * (bodypart > 11));
				if ( dist > 0.1 ) {
					if ( !entity->skill[4] ) {
						entity->fskill[2] += .1;
						if ( entity->fskill[2] >= 1 ) {
							entity->fskill[2] = 1;
							entity->skill[4] = 1;
						}
					} else {
						entity->fskill[2] -= .1;
						if ( entity->fskill[2] <= 0 ) {
							entity->fskill[2] = 0;
							entity->skill[4] = 0;
						}
					}
				}
				entity->z += entity->fskill[2];
				entity->yaw += (PI / 2 + PI / 8) * (1 - 2 * (bodypart > 11));
				entity->pitch += PI / 4;
				break;
			// shin
			case 9:
			case 17:
				entity->x = previous->x;
				entity->y = previous->y;
				entity->z = previous->z;
				entity->yaw = previous->yaw;
				entity->pitch = previous->pitch;
				entity->x += cos(my->yaw) * -1.25 + cos(my->yaw + PI / 2) * 3.25 * (1 - 2 * (bodypart > 11));
				entity->y += sin(my->yaw) * -1.25 + sin(my->yaw + PI / 2) * 3.25 * (1 - 2 * (bodypart > 11));
				entity->z += .5;
				entity->pitch += PI / 6 - PI / 4;
				entity->pitch -= (PI / 10) * (previous->z - my->z);
				break;

			// 4th/8th leg:
			// thigh
			case 10:
			case 18:
				entity->x += cos(my->yaw) * -.5 + cos(my->yaw + PI / 2) * 2 * (1 - 2 * (bodypart > 11));
				entity->y += sin(my->yaw) * -.5 + sin(my->yaw + PI / 2) * 2 * (1 - 2 * (bodypart > 11));
				if ( dist > 0.1 ) {
					if ( !entity->skill[4] ) {
						entity->fskill[2] += .1;
						if ( entity->fskill[2] >= 1 ) {
							entity->fskill[2] = 1;
							entity->skill[4] = 1;
						}
					} else {
						entity->fskill[2] -= .1;
						if ( entity->fskill[2] <= 0 ) {
							entity->fskill[2] = 0;
							entity->skill[4] = 0;
						}
					}
				}
				entity->z += entity->fskill[2];
				entity->yaw += (PI / 2 + PI / 3) * (1 - 2 * (bodypart > 11));
				entity->pitch += PI / 4;
				break;
			// shin
			case 11:
			case 19:
				entity->x = previous->x;
				entity->y = previous->y;
				entity->z = previous->z;
				entity->yaw = previous->yaw;
				entity->pitch = previous->pitch;
				entity->x += cos(my->yaw) * -3 + cos(my->yaw + PI / 2) * 1.75 * (1 - 2 * (bodypart > 11));
				entity->y += sin(my->yaw) * -3 + sin(my->yaw + PI / 2) * 1.75 * (1 - 2 * (bodypart > 11));
				entity->z += .5;
				entity->pitch += PI / 6 - PI / 4;
				entity->pitch += (PI / 10) * (previous->z - my->z);
				break;
			default:
				entity->flags[INVISIBLE] = TRUE; // for debugging
				break;
		}
	}
}