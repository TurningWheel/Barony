/*-------------------------------------------------------------------------------

	BARONY
	File: monster_slime.cpp
	Desc: implements all of the slime monster's code

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
#include "collision.hpp"

void initSlime(Entity *my, Stat *myStats) {
	int c;

	my->flags[UPDATENEEDED]=TRUE;
	my->flags[INVISIBLE]=FALSE;

	if( multiplayer!=CLIENT ) {
		if( myStats->LVL == 7 ) {
			my->sprite = 189;    // blue slime model
		} else {
			my->sprite = 210;    // green slime model
		}
		MONSTER_SPOTSND = 68;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 1;
	}
	if( multiplayer!=CLIENT && !MONSTER_INIT ) {
		myStats->sex = static_cast<sex_t>(rand()%2);
		myStats->appearance = rand();
		strcpy(myStats->name,"");
		myStats->inventory.first = NULL;
		myStats->inventory.last = NULL;
		if( myStats->LVL == 7 ) { // blue slime
			myStats->HP = 70;
			myStats->MAXHP = 70;
			myStats->MP = 70;
			myStats->MAXMP = 70;
			myStats->STR = 10;
		} else { // green slime
			myStats->STR = 3;
			myStats->HP = 60;
			myStats->MAXHP = 60;
			myStats->MP = 60;
			myStats->MAXMP = 60;
		}
		myStats->OLDHP = myStats->HP;
		myStats->DEX = -4;
		myStats->CON = 3;
		myStats->INT = -4;
		myStats->PER = -2;
		myStats->CHR = -4;
		myStats->EXP = 0;
		myStats->GOLD = 0;
		myStats->HUNGER = 900;
		if( !myStats->leader_uid ) {
			myStats->leader_uid = 0;
		}
		myStats->FOLLOWERS.first=NULL;
		myStats->FOLLOWERS.last=NULL;
		for( c=0; c<std::max(NUMPROFICIENCIES,NUMEFFECTS); c++ ) {
			if( c<NUMPROFICIENCIES ) {
				myStats->PROFICIENCIES[c]=0;
			}
			if( c<NUMEFFECTS ) {
				myStats->EFFECTS[c]=FALSE;
			}
			if( c<NUMEFFECTS ) {
				myStats->EFFECTS_TIMERS[c]=0;
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
	}
}

void slimeAnimate(Entity *my, double dist) {
	if(my->skill[24]) {
		my->scalez += .05*dist;
		my->scalex -= .05*dist;
		my->scaley -= .05*dist;
		if( my->scalez>=1.25 ) {
			my->scalez = 1.25;
			my->scalex = .75;
			my->scaley = .75;
			my->skill[24] = 0;
		}
	} else {
		my->scalez -= .05*dist;
		my->scalex += .05*dist;
		my->scaley += .05*dist;
		if( my->scalez<=.75 ) {
			my->scalez = .75;
			my->scalex = 1.25;
			my->scaley = 1.25;
			my->skill[24] = 1;
		}
	}
}

void slimeDie(Entity *my) {
	Entity *entity;
	int c = 0;
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
				if( my->sprite == 210 ) {
					entity = newEntity(212,1,map.entities);
				} else {
					entity = newEntity(214,1,map.entities);
				}
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
	playSoundEntity(my, 69, 64);
	list_RemoveNode(my->mynode);
	return;
}
