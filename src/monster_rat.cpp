/*-------------------------------------------------------------------------------

	BARONY
	File: monster_rat.cpp
	Desc: implements all of the rat monster's code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "monster.hpp"
#include "sound.hpp"
#include "items.hpp"
#include "net.hpp"
#include "collision.hpp"

void initRat(Entity* my, Stat* myStats)
{
	int c;

	my->sprite = 131; // rat model

	my->flags[UPDATENEEDED] = true;
	my->flags[INVISIBLE] = false;

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 29;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = 29;
		MONSTER_IDLEVAR = 1;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		if ( myStats != NULL )
		{
			if ( !myStats->leader_uid )
			{
				myStats->leader_uid = 0;
			}

			// apply random stat increases if set in stat_shared.cpp or editor
			setRandomMonsterStats(myStats);

			// generate 6 items max, less if there are any forced items from boss variants
			int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;

			// boss variants
			if ( rand() % 50 == 0 && !my->flags[USERFLAG2] )
			{
				strcpy(myStats->name, "Algernon");
				myStats->HP = 60;
				myStats->MAXHP = 60;
				myStats->OLDHP = myStats->HP;
				myStats->STR = -1;
				myStats->DEX = 20;
				myStats->CON = 2;
				myStats->INT = 20;
				myStats->PER = -2;
				myStats->CHR = 5;
				myStats->LVL = 10;
				newItem(GEM_EMERALD, static_cast<Status>(1 + rand() % 4), 0, 1, rand(), true, &myStats->inventory);
				customItemsToGenerate = customItemsToGenerate - 1;
				int c;
				for ( c = 0; c < 6; c++ )
				{
					Entity* entity = summonMonster(RAT, my->x, my->y);
					if ( entity )
					{
						entity->parent = my->getUID();
					}
				}
			}
			// random effects

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

														 // count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			// generate the default inventory items for the monster, provided the editor sprite allowed enough default slots
			switch ( defaultItems )
			{
				case 6:
				case 5:
				case 4:
				case 3:
				case 2:
				case 1:
					if ( rand() % 4 )
					{
						if ( rand() % 2 )
						{
							newItem(FOOD_MEAT, EXCELLENT, 0, 1, rand(), false, &myStats->inventory);
						}
						else
						{
							newItem(FOOD_CHEESE, DECREPIT, 0, 1, rand(), false, &myStats->inventory);
						}
					}
					break;
				default:
					break;
			}
		}
	}
}

void ratAnimate(Entity* my, double dist)
{
	// move legs
	if ( (ticks % 10 == 0 && dist > 0.1) || (MONSTER_ATTACKTIME != MONSTER_ATTACK) )
	{
		MONSTER_ATTACKTIME = MONSTER_ATTACK;
		if ( my->sprite == 131 )
		{
			my->sprite = 265;
		}
		else
		{
			my->sprite = 131;
		}
	}
}

void ratDie(Entity* my)
{
	int c = 0;
	for ( c = 0; c < 5; c++ )
	{
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}

	my->spawnBlood();

	playSoundEntity(my, 30, 64);
	list_RemoveNode(my->mynode);
	return;
}
