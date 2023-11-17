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
#include "engine/audio/sound.hpp"
#include "net.hpp"
#include "items.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "prng.hpp"
#include "scores.hpp"

void initScorpion(Entity* my, Stat* myStats)
{
	my->flags[BURNABLE] = true;
	my->flags[UPDATENEEDED] = true;
	my->flags[INVISIBLE] = false;

	my->initMonster(196);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 101;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 1;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		auto& rng = my->entity_rng ? *my->entity_rng : local_rng;

		if ( myStats != NULL )
		{
			if ( !myStats->leader_uid )
			{
				myStats->leader_uid = 0;
			}

			// apply random stat increases if set in stat_shared.cpp or editor
			setRandomMonsterStats(myStats, rng);

			// generate 6 items max, less if there are any forced items from boss variants
			int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;

			// boss variants
			const bool boss =
			    rng.rand() % 50 == 0 &&
			    !my->flags[USERFLAG2] &&
			    !myStats->MISC_FLAGS[STAT_FLAG_DISABLE_MINIBOSS];
			if ( (boss || (*cvar_summonBosses && conductGameChallenges[CONDUCT_CHEATS_ENABLED])) && myStats->leader_uid == 0 )
			{
				myStats->setAttribute("special_npc", "skrabblag");
				strcpy(myStats->name, MonsterData_t::getSpecialNPCName(*myStats).c_str());
				my->sprite = MonsterData_t::getSpecialNPCBaseModel(*myStats);
				myStats->sex = FEMALE;
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
				newItem(GEM_RUBY, static_cast<Status>(1 + rng.rand() % 4), 0, 1, rng.rand(), true, &myStats->inventory);
				customItemsToGenerate = customItemsToGenerate - 1;
				int c;
				for ( c = 0; c < 3; c++ )
				{
					Entity* entity = summonMonster(SCORPION, my->x, my->y);
					if ( entity )
					{
						entity->parent = my->getUID();
						if ( Stat* followerStats = entity->getStats() )
						{
							followerStats->leader_uid = entity->parent;
						}
						entity->seedEntityRNG(rng.getU32());
					}
				}
			}
			// random effects

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats, rng);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate, rng);

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
				default:
					break;
			}
		}
	}

	// tail
	Entity* entity = newEntity(my->sprite == 1080 ? 1082 : 197, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->focalz = -models[entity->sprite]->sizez / 4 + .5;
	entity->focalx = .75;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->behavior = &actScorpionTail;
	entity->parent = my->getUID();
	node_t* node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);
}

void scorpionDie(Entity* my)
{
	Entity* gib = spawnGib(my);
	gib->sprite = my->sprite;
	gib->skill[5] = 1; // poof
	serverSpawnGibForClient(gib);
	for ( int c = 0; c < 8; c++ )
	{
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}

	my->spawnBlood(212);

	my->removeMonsterDeathNodes();

	playSoundEntity(my, 104 + local_rng.rand() % 3, 128);
	list_RemoveNode(my->mynode);
	return;
}
void actScorpionTail(Entity* my)
{
	int i;

	Entity* parent = NULL;
	if ( (parent = uidToEntity(my->skill[2])) == NULL )
	{
		list_RemoveNode(my->mynode);
		return;
	}

	if ( multiplayer != CLIENT )
	{
		for ( i = 0; i < MAXPLAYERS; i++ )
		{
			if ( inrange[i] )
			{
				if ( selectedEntity[i] == my || client_selected[i] == my )
				{
					parent->skill[13] = i + 1;
				}
			}
		}
	}
	return;
}

void scorpionAnimate(Entity* my, double dist)
{
	if (!my)
	{
		return;
	}

	node_t* node;
	Entity* entity;
	int bodypart;

	// set invisibility //TODO: isInvisible()?
	if ( multiplayer != CLIENT )
	{
		Stat* myStats = my->getStats();
		if ( myStats->EFFECTS[EFF_INVISIBLE] == true )
		{
			my->flags[INVISIBLE] = true;
			my->flags[BLOCKSIGHT] = false;
			bodypart = 0;
			for (node = my->children.first; node != NULL; node = node->next)
			{
				if ( bodypart < 2 )
				{
					bodypart++;
					continue;
				}
				entity = (Entity*)node->element;
				if ( !entity->flags[INVISIBLE] )
				{
					entity->flags[INVISIBLE] = true;
					serverUpdateEntityBodypart(my, bodypart);
				}
				bodypart++;
			}
		}
		else
		{
			my->flags[INVISIBLE] = false;
			my->flags[BLOCKSIGHT] = true;
			bodypart = 0;
			for (node = my->children.first; node != NULL; node = node->next)
			{
				if ( bodypart < 2 )
				{
					bodypart++;
					continue;
				}
				entity = (Entity*)node->element;
				if ( entity->flags[INVISIBLE] )
				{
					entity->flags[INVISIBLE] = false;
					serverUpdateEntityBodypart(my, bodypart);
					serverUpdateEntityFlag(my, INVISIBLE);
				}
				bodypart++;
			}
		}
	}

	bool skrabblag = false;

	// move tail
	for (bodypart = 0, node = my->children.first; node != NULL; node = node->next, bodypart++)
	{
		if ( bodypart < 2 )
		{
			continue;
		}
		entity = (Entity*)node->element;
		entity->x = my->x - 4 * cos(my->yaw);
		entity->y = my->y - 4 * sin(my->yaw);
		entity->z = my->z;
		entity->yaw = my->yaw;
		if (entity->sprite == 1082) {
		    skrabblag = true;
		}
		if ( !MONSTER_ATTACK )
		{
			entity->pitch = 0;
		}
		else
		{
			if ( !MONSTER_ATTACKTIME )
			{
				entity->pitch += .2;
				if ( entity->pitch > PI / 3 )
				{
					entity->pitch = PI / 3;
					MONSTER_ATTACKTIME = 1;
				}
			}
			else
			{
				entity->pitch -= .1;
				if ( entity->pitch < 0 )
				{
					entity->pitch = 0;
					MONSTER_ATTACKTIME = 0;
					MONSTER_ATTACK = 0;
				}
			}
		}
	}

	// move legs
	if ( ticks % 10 == 0 && dist > 0.1 )
	{
		if ( skrabblag )
		{
			my->sprite = my->sprite == 1080 ? 1081 : 1080;
		}
		else
		{
			my->sprite = my->sprite == 196 ? 266 : 196;
		}
	}
}
