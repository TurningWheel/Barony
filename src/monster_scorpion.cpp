/*-------------------------------------------------------------------------------

	BARONY
	File: monster_scorpion.cpp
	Desc: implements all of the scorpion monster's code

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

void initScorpion(Entity *my, Stat *myStats) {
	int c;

	my->flags[UPDATENEEDED] = TRUE;
	my->flags[INVISIBLE] = FALSE;

	my->sprite = 196;
	if ( multiplayer != CLIENT ) {
		MONSTER_SPOTSND = 101;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 1;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT ) {
		myStats->sex = static_cast<sex_t>(rand() % 2);
		myStats->appearance = rand();
		strcpy(myStats->name, "");
		myStats->inventory.first = NULL;
		myStats->inventory.last = NULL;
		myStats->HP = 70;
		myStats->MAXHP = 70;
		myStats->MP = 10;
		myStats->MAXMP = 10;
		myStats->OLDHP = myStats->HP;
		myStats->STR = 13;
		myStats->DEX = 3;
		myStats->CON = 4;
		myStats->INT = -3;
		myStats->PER = -3;
		myStats->CHR = -4;
		myStats->EXP = 0;
		myStats->LVL = 7;
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
			strcpy(myStats->name, "Skrabblag");
			myStats->HP = 100;
			myStats->MAXHP = 100;
			myStats->OLDHP = myStats->HP;
			myStats->STR = 15;
			myStats->DEX = 5;
			myStats->CON = 6;
			myStats->INT = 10;
			myStats->PER = 10;
			myStats->CHR = 10;
			myStats->LVL = 15;
			newItem( GEM_RUBY, static_cast<Status>(1 + rand() % 4), 0, 1, rand(), TRUE, &myStats->inventory );

			int c;
			for ( c = 0; c < 3; c++ ) {
				Entity *entity = summonMonster(SCORPION, my->x, my->y);
				if ( entity ) {
					entity->parent = my->uid;
				}
			}
		}
	}

	// tail
	Entity *entity = newEntity(197, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->uid;
	entity->focalz = -models[entity->sprite]->sizez / 4 + .5;
	entity->focalx = .75;
	entity->flags[PASSABLE] = TRUE;
	entity->flags[NOUPDATE] = TRUE;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->behavior = &actScorpionTail;
	entity->parent = my->uid;
	node_t *node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity *);
}

void scorpionDie(Entity *my) {
	node_t *node, *nextnode;

	int c = 0;
	for ( c = 0; c < 5; c++ ) {
		Entity *gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}
	if (spawn_blood) {
		int x, y;
		x = std::min<unsigned int>(std::max<int>(0, my->x / 16), map.width - 1);
		y = std::min<unsigned int>(std::max<int>(0, my->y / 16), map.height - 1);
		if ( map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height] ) {
			if ( !checkObstacle(my->x, my->y, my, NULL) ) {
				Entity *entity = newEntity(212, 1, map.entities);
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
			Entity *entity = (Entity *)node->element;
			entity->flags[UPDATENEEDED] = FALSE;
			list_RemoveNode(entity->mynode);
		}
		list_RemoveNode(node);
		++i;
	}
	playSoundEntity(my, 104 + rand() % 3, 128);
	list_RemoveNode(my->mynode);
	return;
}
void actScorpionTail(Entity *my) {
	int i;

	Entity *parent = NULL;
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

void scorpionAnimate(Entity *my, double dist) {
	if (!my) {
		return;
	}

	node_t *node;
	Entity *entity;
	int bodypart;

	// set invisibility
	if ( multiplayer != CLIENT ) {
		Stat *myStats = my->getStats();
		if ( myStats->EFFECTS[EFF_INVISIBLE] == TRUE ) {
			my->flags[INVISIBLE] = TRUE;
			my->flags[BLOCKSIGHT] = FALSE;
			bodypart = 0;
			for (node = my->children.first; node != NULL; node = node->next) {
				if ( bodypart < 2 ) {
					bodypart++;
					continue;
				}
				entity = (Entity *)node->element;
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
				entity = (Entity *)node->element;
				if ( entity->flags[INVISIBLE] ) {
					entity->flags[INVISIBLE] = FALSE;
					serverUpdateEntityBodypart(my, bodypart);
				}
				bodypart++;
			}
		}
	}

	// move legs
	if ( ticks % 10 == 0 && dist > 0.1 ) {
		if ( my->sprite == 196 ) {
			my->sprite = 266;
		} else {
			my->sprite = 196;
		}
	}

	// move tail
	for (bodypart = 0, node = my->children.first; node != NULL; node = node->next, bodypart++) {
		if ( bodypart < 2 ) {
			continue;
		}
		entity = (Entity *)node->element;
		entity->x = my->x - 4 * cos(my->yaw);
		entity->y = my->y - 4 * sin(my->yaw);
		entity->z = my->z;
		entity->yaw = my->yaw;
		if ( !MONSTER_ATTACK ) {
			entity->pitch = 0;
		} else {
			if ( !MONSTER_ATTACKTIME ) {
				entity->pitch += .2;
				if ( entity->pitch > PI / 3 ) {
					entity->pitch = PI / 3;
					MONSTER_ATTACKTIME = 1;
				}
			} else {
				entity->pitch -= .1;
				if ( entity->pitch < 0 ) {
					entity->pitch = 0;
					MONSTER_ATTACKTIME = 0;
					MONSTER_ATTACK = 0;
				}
			}
		}
	}
}
