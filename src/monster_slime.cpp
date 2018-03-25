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
#include "items.hpp"
#include "monster.hpp"
#include "sound.hpp"
#include "net.hpp"
#include "collision.hpp"

void initSlime(Entity* my, Stat* myStats)
{
	int c;

	my->flags[UPDATENEEDED] = true;
	my->flags[INVISIBLE] = false;

	if ( multiplayer != CLIENT )
	{
		if ( myStats->LVL == 7 )
		{
			my->sprite = 189;    // blue slime model
		}
		else
		{
			my->sprite = 210;    // green slime model
		}
		MONSTER_SPOTSND = 68;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = -1;
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

			// random effects

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

														 // count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			my->setHardcoreStats(*myStats);

			// generate the default inventory items for the monster, provided the editor sprite allowed enough default slots
			switch ( defaultItems )
			{
				case 6:
				case 5:
				case 4:
				case 3:
				case 2:
				case 1:
					break;
				default:
					break;
			}
		}
	}
}

void slimeAnimate(Entity* my, double dist)
{
	if (my->skill[24])
	{
		my->scalez += .05 * dist;
		my->scalex -= .05 * dist;
		my->scaley -= .05 * dist;
		if ( my->scalez >= 1.25 )
		{
			my->scalez = 1.25;
			my->scalex = .75;
			my->scaley = .75;
			my->skill[24] = 0;
		}
	}
	else
	{
		my->scalez -= .05 * dist;
		my->scalex += .05 * dist;
		my->scaley += .05 * dist;
		if ( my->scalez <= .75 )
		{
			my->scalez = .75;
			my->scalex = 1.25;
			my->scaley = 1.25;
			my->skill[24] = 1;
		}
	}
}

void slimeDie(Entity* my)
{
	Entity* entity;
	int c = 0;
	for ( c = 0; c < 5; c++ )
	{
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}

	if ( my->sprite == 210 )
	{
		my->spawnBlood(212);
	}
	else
	{
		my->spawnBlood(214);
	}

	playSoundEntity(my, 69, 64);
	list_RemoveNode(my->mynode);
	return;
}
