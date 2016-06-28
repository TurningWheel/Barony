/*-------------------------------------------------------------------------------

	BARONY
	File: stat.cpp
	Desc: functions for the stat_t struct

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "magic/magic.hpp"

stat_t stats[MAXPLAYERS];

/*-------------------------------------------------------------------------------

	statConstructor

	initializes important stat elements to NULL

-------------------------------------------------------------------------------*/

void statConstructor(stat_t *stat) {
	stat->type = NOTHING;
	stat->sex = static_cast<sex_t>(rand()%2);
	stat->appearance = 0;
	strcpy(stat->name,"Nobody");
	strcpy(stat->obituary,language[1500]);
	stat->poisonKiller = 0;
	stat->HP = 10;
	stat->MAXHP = 10;
	stat->OLDHP=stat->HP;
	stat->MP = 10;
	stat->MAXMP = 10;
	stat->STR=0;
	stat->DEX=0;
	stat->CON=0;
	stat->INT=0;
	stat->PER=0;
	stat->CHR=0;
	stat->EXP=0;
	stat->LVL=1;
	stat->GOLD=0;
	stat->HUNGER=800;
	stat->defending=FALSE;
	
	int c;
	for( c=0; c<NUMPROFICIENCIES; c++ ) {
		stat->PROFICIENCIES[c] = 0;
	}
	for( c=0; c<NUMEFFECTS; c++ ) {
		stat->EFFECTS[c] = 0;
		stat->EFFECTS_TIMERS[c] = 0;
	}
	stat->leader_uid=0;
	stat->FOLLOWERS.first=NULL;
	stat->FOLLOWERS.last=NULL;
	stat->stache_x1=0;
	stat->stache_x2=0;
	stat->stache_y1=0;
	stat->stache_y2=0;
	stat->inventory.first = NULL;
	stat->inventory.last = NULL;
	stat->helmet = NULL;
	stat->breastplate = NULL;
	stat->gloves = NULL;
	stat->shoes = NULL;
	stat->shield = NULL;
	stat->weapon = NULL;
	stat->cloak = NULL;
	stat->amulet = NULL;
	stat->ring = NULL;
	stat->mask = NULL;
	stat->monster_sound = NULL;
	stat->monster_idlevar = 1;
	stat->magic_effects.first = NULL;
	stat->magic_effects.last = NULL;
}

/*-------------------------------------------------------------------------------

	statDeconstructor

	deconstructs a stat_t object

-------------------------------------------------------------------------------*/

void statDeconstructor(void *data) {
	stat_t *stat;
	if(data != NULL) {
		stat = (stat_t *)data;
		if( stat->helmet != NULL ) {
			if( stat->helmet->node == NULL )
				free(stat->helmet);
			else
				list_RemoveNode(stat->helmet->node);
			stat->helmet = NULL;
		}
		if( stat->breastplate != NULL ) {
			if( stat->breastplate->node == NULL )
				free(stat->breastplate);
			else
				list_RemoveNode(stat->breastplate->node);
			stat->breastplate = NULL;
		}
		if( stat->gloves != NULL ) {
			if( stat->gloves->node == NULL )
				free(stat->gloves);
			else
				list_RemoveNode(stat->gloves->node);
			stat->gloves = NULL;
		}
		if( stat->shoes != NULL ) {
			if( stat->shoes->node == NULL )
				free(stat->shoes);
			else
				list_RemoveNode(stat->shoes->node);
			stat->shoes = NULL;
		}
		if( stat->shield != NULL ) {
			if( stat->shield->node == NULL )
				free(stat->shield);
			else
				list_RemoveNode(stat->shield->node);
			stat->shield = NULL;
		}
		if( stat->weapon != NULL ) {
			if( stat->weapon->node == NULL )
				free(stat->weapon);
			else
				list_RemoveNode(stat->weapon->node);
			stat->weapon = NULL;
		}
		if( stat->cloak != NULL ) {
			if( stat->cloak->node == NULL )
				free(stat->cloak);
			else
				list_RemoveNode(stat->cloak->node);
			stat->cloak = NULL;
		}
		if( stat->amulet != NULL ) {
			if( stat->amulet->node == NULL )
				free(stat->amulet);
			else
				list_RemoveNode(stat->amulet->node);
			stat->amulet = NULL;
		}
		if( stat->ring != NULL ) {
			if( stat->ring->node == NULL )
				free(stat->ring);
			else
				list_RemoveNode(stat->ring->node);
			stat->ring = NULL;
		}
		if( stat->mask != NULL ) {
			if( stat->mask->node == NULL )
				free(stat->mask);
			else
				list_RemoveNode(stat->mask->node);
			stat->mask = NULL;
		}
		//Free memory for magic effects.
		node_t *spellnode;
		spellnode = stat->magic_effects.first;
		while (spellnode) {
			node_t *oldnode = spellnode;
			spellnode = spellnode->next;
			spell_t *spell = (spell_t*)oldnode->element;
			spell->magic_effects_node = NULL;
		}
		list_FreeAll(&stat->magic_effects);
		list_FreeAll(&stat->inventory);
		free(data);
	}
}

/*-------------------------------------------------------------------------------

	clearStats
	
	resets the values of the given stat_t structure

-------------------------------------------------------------------------------*/

void clearStats(stat_t *stats) {
	int x;

	strcpy(stats->obituary,language[1500]);
	stats->poisonKiller = 0;
	stats->HP=DEFAULT_HP; stats->MAXHP=DEFAULT_HP;
	stats->OLDHP=stats->HP;
	stats->MP=DEFAULT_MP; stats->MAXMP=DEFAULT_MP;
	stats->STR=0;
	stats->DEX=0;
	stats->CON=0;
	stats->INT=0;
	stats->PER=0;
	stats->CHR=0;
	stats->GOLD=0;
	stats->HUNGER=1000;
	stats->LVL=1; stats->EXP=0;
	list_FreeAll(&stats->FOLLOWERS);
	for( x=0; x<std::max(NUMPROFICIENCIES,NUMEFFECTS); x++ ) {
		if( x<NUMPROFICIENCIES )
			stats->PROFICIENCIES[x]=0;
		if( x<NUMEFFECTS ) {
			stats->EFFECTS[x]=FALSE;
			stats->EFFECTS_TIMERS[x]=0;
		}
	}
	list_FreeAll(&stats->inventory);
	stats->helmet=NULL;
	stats->breastplate=NULL;
	stats->gloves=NULL;
	stats->shoes=NULL;
	stats->shield=NULL;
	stats->weapon=NULL;
	stats->cloak=NULL;
	stats->amulet=NULL;
	stats->ring=NULL;
	stats->mask=NULL;
}

/*-------------------------------------------------------------------------------

	freePlayerEquipment

	frees all the malloc'd data for the given player's equipment

-------------------------------------------------------------------------------*/

void freePlayerEquipment(int x) {
	if( stats[x].helmet != NULL ) {
		if( stats[x].helmet->node )
			list_RemoveNode(stats[x].helmet->node);
		else
			free(stats[x].helmet);
		stats[x].helmet = NULL;
	}
	if( stats[x].breastplate != NULL ) {
		if( stats[x].breastplate->node )
			list_RemoveNode(stats[x].breastplate->node);
		else
			free(stats[x].breastplate);
		stats[x].breastplate = NULL;
	}
	if( stats[x].gloves != NULL ) {
		if( stats[x].gloves->node )
			list_RemoveNode(stats[x].gloves->node);
		else
			free(stats[x].gloves);
		stats[x].gloves = NULL;
	}
	if( stats[x].shoes != NULL ) {
		if( stats[x].shoes->node )
			list_RemoveNode(stats[x].shoes->node);
		else
			free(stats[x].shoes);
		stats[x].shoes = NULL;
	}
	if( stats[x].shield != NULL ) {
		if( stats[x].shield->node )
			list_RemoveNode(stats[x].shield->node);
		else
			free(stats[x].shield);
		stats[x].shield = NULL;
	}
	if( stats[x].weapon != NULL ) {
		if( stats[x].weapon->node )
			list_RemoveNode(stats[x].weapon->node);
		else
			free(stats[x].weapon);
		stats[x].weapon = NULL;
	}
	if( stats[x].cloak != NULL ) {
		if( stats[x].cloak->node )
			list_RemoveNode(stats[x].cloak->node);
		else
			free(stats[x].cloak);
		stats[x].cloak = NULL;
	}
	if( stats[x].amulet != NULL ) {
		if( stats[x].amulet->node )
			list_RemoveNode(stats[x].amulet->node);
		else
			free(stats[x].amulet);
		stats[x].amulet = NULL;
	}
	if( stats[x].ring != NULL ) {
		if( stats[x].ring->node )
			list_RemoveNode(stats[x].ring->node);
		else
			free(stats[x].ring);
		stats[x].ring = NULL;
	}
	if( stats[x].mask != NULL ) {
		if( stats[x].mask->node )
			list_RemoveNode(stats[x].mask->node);
		else
			free(stats[x].mask);
		stats[x].mask = NULL;
	}
}

/*-------------------------------------------------------------------------------

	copyStats

	Returns a pointer to a copy of the stat_t struct given in stat*

-------------------------------------------------------------------------------*/

stat_t *copyStats(stat_t *stat) {
	node_t *node;
	int c;

	stat_t *newStat = (stat_t *) malloc(sizeof(stat_t));

	newStat->type = stat->type;
	newStat->sex = stat->sex;
	newStat->appearance = stat->appearance;
	strcpy(newStat->name,stat->name);
	strcpy(newStat->obituary,stat->obituary);

	newStat->HP = stat->HP;
	newStat->MAXHP = stat->MAXHP;
	newStat->OLDHP = stat->OLDHP;

	newStat->MP = stat->MP;
	newStat->MAXMP = stat->MAXMP;
	newStat->STR = stat->STR;
	newStat->DEX = stat->DEX;
	newStat->CON = stat->CON;
	newStat->INT = stat->PER;
	newStat->CHR = stat->CHR;
	newStat->EXP = stat->EXP;
	newStat->LVL = stat->LVL;
	newStat->GOLD = stat->GOLD;
	newStat->HUNGER = stat->HUNGER;

	for( c=0; c<NUMPROFICIENCIES; c++ ) {
		newStat->PROFICIENCIES[c] = stat->PROFICIENCIES[c];
	}
	for( c=0; c<NUMEFFECTS; c++ ) {
		newStat->EFFECTS[c] = stat->EFFECTS[c];
		newStat->EFFECTS_TIMERS[c] = stat->EFFECTS_TIMERS[c];
	}

	newStat->defending = stat->defending;
	newStat->leader_uid = stat->leader_uid;
	newStat->FOLLOWERS.first = NULL; newStat->FOLLOWERS.last = NULL;
	list_Copy(&newStat->FOLLOWERS,&stat->FOLLOWERS);
	newStat->stache_x1 = stat->stache_x1;
	newStat->stache_x2 = stat->stache_x2;
	newStat->stache_y1 = stat->stache_y1;
	newStat->stache_y2 = stat->stache_y2;

	newStat->inventory.first = NULL; newStat->inventory.last = NULL;
	list_Copy(&newStat->inventory,&stat->inventory);
	for( node=newStat->inventory.first; node!=NULL; node=node->next ) {
		Item *item = (Item *)node->element;
		item->node = node;
	}

	if( stat->helmet ) {
		if( stat->helmet->node ) {
			node_t *node = list_Node(&newStat->inventory,list_Index(stat->helmet->node));
			newStat->helmet = (Item *) node->element;
		} else {
			newStat->helmet = (Item *) malloc(sizeof(Item));
			memcpy(newStat->helmet,stat->helmet,sizeof(Item));
		}
	} else {
		newStat->helmet = NULL;
	}
	if( stat->breastplate ) {
		if( stat->breastplate->node ) {
			node_t *node = list_Node(&newStat->inventory,list_Index(stat->breastplate->node));
			newStat->breastplate = (Item *) node->element;
		} else {
			newStat->breastplate = (Item *) malloc(sizeof(Item));
			memcpy(newStat->breastplate,stat->breastplate,sizeof(Item));
		}
	} else {
		newStat->breastplate = NULL;
	}
	if( stat->gloves ) {
		if( stat->gloves->node ) {
			node_t *node = list_Node(&newStat->inventory,list_Index(stat->gloves->node));
			newStat->gloves = (Item *) node->element;
		} else {
			newStat->gloves = (Item *) malloc(sizeof(Item));
			memcpy(newStat->gloves,stat->gloves,sizeof(Item));
		}
	} else {
		newStat->gloves = NULL;
	}
	if( stat->shoes ) {
		if( stat->shoes->node ) {
			node_t *node = list_Node(&newStat->inventory,list_Index(stat->shoes->node));
			newStat->shoes = (Item *) node->element;
		} else {
			newStat->shoes = (Item *) malloc(sizeof(Item));
			memcpy(newStat->shoes,stat->shoes,sizeof(Item));
		}
	} else {
		newStat->shoes = NULL;
	}
	if( stat->shield ) {
		if( stat->shield->node ) {
			node_t *node = list_Node(&newStat->inventory,list_Index(stat->shield->node));
			newStat->shield = (Item *) node->element;
		} else {
			newStat->shield = (Item *) malloc(sizeof(Item));
			memcpy(newStat->shield,stat->shield,sizeof(Item));
		}
	} else {
		newStat->shield = NULL;
	}
	if( stat->weapon ) {
		if( stat->weapon->node ) {
			node_t *node = list_Node(&newStat->inventory,list_Index(stat->weapon->node));
			newStat->weapon = (Item *) node->element;
		} else {
			newStat->weapon = (Item *) malloc(sizeof(Item));
			memcpy(newStat->weapon,stat->weapon,sizeof(Item));
		}
	} else {
		newStat->weapon = NULL;
	}
	if( stat->cloak ) {
		if( stat->cloak->node ) {
			node_t *node = list_Node(&newStat->inventory,list_Index(stat->cloak->node));
			newStat->cloak = (Item *) node->element;
		} else {
			newStat->cloak = (Item *) malloc(sizeof(Item));
			memcpy(newStat->cloak,stat->cloak,sizeof(Item));
		}
	} else {
		newStat->cloak = NULL;
	}
	if( stat->amulet ) {
		if( stat->amulet->node ) {
			node_t *node = list_Node(&newStat->inventory,list_Index(stat->amulet->node));
			newStat->amulet = (Item *) node->element;
		} else {
			newStat->amulet = (Item *) malloc(sizeof(Item));
			memcpy(newStat->amulet,stat->amulet,sizeof(Item));
		}
	} else {
		newStat->amulet = NULL;
	}
	if( stat->ring ) {
		if( stat->ring->node ) {
			node_t *node = list_Node(&newStat->inventory,list_Index(stat->ring->node));
			newStat->ring = (Item *) node->element;
		} else {
			newStat->ring = (Item *) malloc(sizeof(Item));
			memcpy(newStat->ring,stat->ring,sizeof(Item));
		}
	} else {
		newStat->ring = NULL;
	}
	if( stat->mask ) {
		if( stat->mask->node ) {
			node_t *node = list_Node(&newStat->inventory,list_Index(stat->mask->node));
			newStat->mask = (Item *) node->element;
		} else {
			newStat->mask = (Item *) malloc(sizeof(Item));
			memcpy(newStat->mask,stat->mask,sizeof(Item));
		}
	} else {
		newStat->mask = NULL;
	}

	newStat->monster_sound = NULL;
	newStat->monster_idlevar = stat->monster_idlevar;
	newStat->magic_effects.first = NULL; newStat->magic_effects.last = NULL;

	return newStat;
}

void stat_t::printStats()
{
	printlog("type = %d\n", type);
	printlog("sex = %d\n", sex);
	printlog("appearance = %d\n", appearance);
	printlog("name = \"%s\"\n", name);
	printlog("HP = %d\n", HP);
	printlog("MAXHP = %d\n", MAXHP);
	printlog("MP = %d\n", MP);
	printlog("MAXMP = %d\n", MAXMP);
	printlog("STR = %d\n", STR);
	printlog("DEX = %d\n", DEX);
	printlog("CON = %d\n", CON);
	printlog("INT = %d\n", INT);
	printlog("PER = %d\n", PER);
	printlog("CHR = %d\n", CHR);
	printlog("EXP = %d\n", EXP);
	printlog("LVL = %d\n", LVL);
	printlog("GOLD = %d\n", GOLD);
	printlog("HUNGER = %d\n", HUNGER);

	printlog("Proficiencies:");
	for (int i = 0; i < NUMPROFICIENCIES; ++i)
	{
		printlog("[%d] = %d%s", i, PROFICIENCIES[i], ((i == NUMPROFICIENCIES - 1) ? "\n" : ", "));
	}

	printlog("Effects & timers: ");
	for (int i = 0; i < NUMEFFECTS; ++i)
	{
		printlog("[%d] = %s. timer[%d] = %d", i, (EFFECTS[i]) ? "true" : "false", i, EFFECTS_TIMERS[i]);
	}
}