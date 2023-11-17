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
#include "engine/audio/sound.hpp"
#include "net.hpp"
#include "items.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "ui/MainMenu.hpp"
#include "prng.hpp"
#include "scores.hpp"

void initSpider(Entity* my, Stat* myStats)
{
	int c;

	my->flags[BURNABLE] = true;
	my->flags[UPDATENEEDED] = true;
	my->flags[INVISIBLE] = false;
	my->z = 4.5;

    my->initMonster(arachnophobia_filter ? 997 : 267);

	if ( multiplayer != CLIENT )
	{
	    if (arachnophobia_filter)
	    {
		    MONSTER_SPOTSND = 502;
		    MONSTER_SPOTVAR = 3;
		    MONSTER_IDLESND = 506;
		    MONSTER_IDLEVAR = 4;
	    }
	    else
	    {
		    MONSTER_SPOTSND = 229;
		    MONSTER_SPOTVAR = 3;
		    MONSTER_IDLESND = 232;
		    MONSTER_IDLEVAR = 4;
	    }
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
			    if (!arachnophobia_filter) {
					myStats->setAttribute("special_npc", "shelob");
					strcpy(myStats->name, MonsterData_t::getSpecialNPCName(*myStats).c_str());
					my->sprite = MonsterData_t::getSpecialNPCBaseModel(*myStats);
			    } else {
					myStats->setAttribute("special_npc", "bubbles");
					strcpy(myStats->name, MonsterData_t::getSpecialNPCName(*myStats).c_str());
					my->sprite = MonsterData_t::getSpecialNPCBaseModel(*myStats);
			    }
				myStats->sex = FEMALE;
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
				newItem(RING_INVISIBILITY, EXCELLENT, -5, 1, rng.rand(), false, &myStats->inventory);
				int status = DECREPIT + (currentlevel > 5) + (currentlevel > 15) + (currentlevel > 20);
				newItem(ARTIFACT_SWORD, static_cast<Status>(status), 1, 1, rng.rand(), false, &myStats->inventory);
				customItemsToGenerate -= 2;
				int c;
				for ( c = 0; c < 3; c++ )
				{
					Entity* entity = summonMonster(SPIDER, my->x, my->y);
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
			int customItems = countCustomItems(myStats);
			//max limit of 6 custom items per entity.

			// count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			my->setHardcoreStats(*myStats);

			newItem(SPELLBOOK_SPRAY_WEB, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, &myStats->inventory);

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

	int model;

	// right pedipalp
	model = arachnophobia_filter ? 998 : (my->sprite == 1118 ? 1119 : 268);
	Entity* entity = newEntity(model, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	if (arachnophobia_filter)
	{
	    entity->focalx = limbs[CRAB][1][0];
	    entity->focaly = limbs[CRAB][1][1];
	    entity->focalz = limbs[CRAB][1][2];
	}
	else
	{
	    entity->focalx = limbs[SPIDER][1][0]; // 1
	    entity->focaly = limbs[SPIDER][1][1]; // 0
	    entity->focalz = limbs[SPIDER][1][2]; // 1
	}
	entity->behavior = &actSpiderLimb;
	entity->parent = my->getUID();
	node_t* node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left pedipalp
	model = arachnophobia_filter ? 998 : (my->sprite == 1118 ? 1119 : 268);
	entity = newEntity(model, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	if (arachnophobia_filter)
	{
	    entity->focalx = limbs[CRAB][2][0];
	    entity->focaly = limbs[CRAB][2][1];
	    entity->focalz = limbs[CRAB][2][2];
	}
	else
	{
	    entity->focalx = limbs[SPIDER][2][0]; // 1
	    entity->focaly = limbs[SPIDER][2][1]; // 0
	    entity->focalz = limbs[SPIDER][2][2]; // 1
	}
	entity->behavior = &actSpiderLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// eight legs :)
	for ( c = 0; c < 8; c++ )
	{
		// "thigh"
	    model = arachnophobia_filter ? 999 : (my->sprite == 1118 ? 1121 : 269);
		entity = newEntity(model, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = my->getUID();
		entity->fskill[2] = (c / 8.f);
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	    if (arachnophobia_filter)
	    {
	        entity->focalx = limbs[CRAB][3][0];
	        entity->focaly = limbs[CRAB][3][1];
	        entity->focalz = limbs[CRAB][3][2];
	    }
	    else
	    {
	        entity->focalx = limbs[SPIDER][3][0]; // 1
	        entity->focaly = limbs[SPIDER][3][1]; // 0
	        entity->focalz = limbs[SPIDER][3][2]; // -1
	    }
		entity->behavior = &actSpiderLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
	    my->bodyparts.push_back(entity);

		// "shin"
	    model = arachnophobia_filter ? 1000 : (my->sprite == 1118 ? 1120 : 270);
		entity = newEntity(model, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = my->getUID();
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	    if (arachnophobia_filter)
	    {
	        entity->focalx = limbs[CRAB][4][0];
	        entity->focaly = limbs[CRAB][4][1];
	        entity->focalz = limbs[CRAB][4][2];
	    }
	    else
	    {
	        entity->focalx = limbs[SPIDER][4][0]; // 3
	        entity->focaly = limbs[SPIDER][4][1]; // 0
	        entity->focalz = limbs[SPIDER][4][2]; // 0
	    }
		entity->behavior = &actSpiderLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
	    my->bodyparts.push_back(entity);
	}
}

void spiderDie(Entity* my)
{
	Entity* gib = spawnGib(my);
	gib->sprite = my->sprite;
	gib->skill[5] = 1; // poof
	serverSpawnGibForClient(gib);
	for ( int c = 0; c < 4; c++ )
	{
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}
	for ( int c = 0; c < 8; c++ )
	{
		Entity* gib = spawnGib(my);
		if ( my->sprite == 997 || my->sprite == 1189 ) // crabs
		{
			gib->sprite = 999;
		}
		else
		{
			gib->sprite = my->sprite == 1118 ? 1121 : 269;
		}
		gib->skill[5] = 1; // poof
		serverSpawnGibForClient(gib);
	}

	my->spawnBlood(212);

	my->removeMonsterDeathNodes();

    if (arachnophobia_filter)
    {
	    playSoundEntity(my, 509 + local_rng.rand() % 2, 128);
    }
    else
    {
	    playSoundEntity(my, 236 + local_rng.rand() % 2, 128);
    }
	list_RemoveNode(my->mynode);
	return;
}

void actSpiderLimb(Entity* my)
{
	my->actMonsterLimb();
}

void spiderMoveBodyparts(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity;
	int bodypart;

	// set invisibility //TODO: isInvisible()?
	if ( multiplayer != CLIENT )
	{
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

	// animate limbs
	Entity* leftArm = nullptr;
	for (bodypart = 0, node = my->children.first; node != NULL; node = node->next, bodypart++)
	{
		if ( bodypart < 2 )
		{
			continue;
		}
		entity = (Entity*)node->element;
		Entity* previous = NULL; // previous part
		if ( bodypart > 2 )
		{
			previous = (Entity*)node->prev->element;
		}
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		entity->yaw = my->yaw;

		if ( bodypart == 2 || bodypart == 3 )
		{
			bool left = (bodypart == 2);
			if ( left )
			{
				leftArm = entity;
			}

			if ( !left && leftArm )
			{
				entity->roll = -leftArm->roll;
				entity->pitch = leftArm->pitch;
			}
			
			if ( left )
			{
				real_t rollRate = 0.0;
				real_t pitchMult = arachnophobia_filter ? 0.5 : 1.0; // crab limbs are more expressive so tone it down
				if ( MONSTER_ATTACK != 0 )
				{
					if ( MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 )
					{
						if ( MONSTER_ATTACKTIME == 0 )
						{
							entity->pitch = 0.0;
							entity->roll = 0.0;
							entity->skill[1] = 0;
							entity->skill[0] = 0;
							createParticleDot(my);
						}
						else
						{
							rollRate = 0.1;
							limbAnimateToLimit(entity, ANIMATE_PITCH, -0.1 * pitchMult, -(20.0 / 180.0) * PI * pitchMult, false, 0.0);
							if ( entity->skill[0] == 0 )
							{
								if ( limbAnimateToLimit(entity, ANIMATE_ROLL, rollRate, PI / 4, false, 0.0) )
								{
									entity->skill[0] = 1;
								}
							}
							else
							{
								if ( limbAnimateToLimit(entity, ANIMATE_ROLL, -rollRate, 0.0, false, 0.0) )
								{
									entity->skill[0] = 0;
								}
							}
							if ( MONSTER_ATTACKTIME >= 3 * ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
							{
								if ( multiplayer != CLIENT )
								{
									// swing the arm after we prepped the spell
									my->attack(1, 0, nullptr);
								}
							}
						}
					}
					else if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP1 )
					{
						if ( MONSTER_ATTACKTIME == 0 )
						{
							entity->pitch = 0.0;
							entity->roll = 0.0;
							entity->skill[1] = 0;
						}
						else
						{
							limbAnimateToLimit(entity, ANIMATE_PITCH, -0.25 * pitchMult, -(65.0 / 180.0) * PI * pitchMult, false, 0.0);
							if ( MONSTER_ATTACKTIME >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
							{
								if ( multiplayer != CLIENT )
								{
									my->attack(1, 0, nullptr);
								}
							}
						}
					}
					else if ( MONSTER_ATTACK == 1 )
					{
						if ( entity->skill[1] == 0 )
						{
							if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.4 * pitchMult, pitchMult * PI / 2, false, 0.0) )
							{
								entity->skill[1] = 1;
							}
						}
						else if ( entity->skill[1] >= 1 )
						{
							if ( limbAnimateToLimit(entity, ANIMATE_PITCH, -0.25 * pitchMult, 0.0, false, 0.0) )
							{
								entity->skill[1] = 0;
								entity->roll = 0;
								MONSTER_ATTACK = 0;
							}
						}
					}
				}
				else
				{
					if ( dist > 0.1 )
					{
						rollRate = 0.1; // walking/chasing click click click
					}
					else if ( my->monsterState == MONSTER_STATE_WAIT )
					{
						// idle, waiting
						rollRate = 0.01;
					}
					
					if ( rollRate > 0.0001 )
					{
						if ( entity->skill[0] == 0 )
						{
							if ( limbAnimateToLimit(entity, ANIMATE_ROLL, rollRate, PI / 4, false, 0.0) )
							{
								entity->skill[0] = 1;
							}
						}
						else
						{
							if ( limbAnimateToLimit(entity, ANIMATE_ROLL, -rollRate, 0.0, false, 0.0) )
							{
								entity->skill[0] = 0;
							}
						}
					}
				}

				if ( my->monsterState == MONSTER_STATE_WAIT )
				{
					entity->fskill[0] = std::max(-PI * 10 / 180.0, entity->fskill[0] - 0.05); // lower the butt
				}
				else if ( MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 )
				{
					entity->fskill[0] = std::max(-PI * 20 / 180.0, entity->fskill[0] - 0.05); // lower the butt more
				}
				else if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP1 )
				{
					entity->fskill[0] = std::max(-PI * 20 / 180.0, entity->fskill[0] - 0.2); // lower the butt more and fast
				}
				else
				{
					entity->fskill[0] = std::min(PI * 20 / 180.0, entity->fskill[0] + 0.1); // raise the butt if walking not attacking
				}

				my->pitch = entity->fskill[0];
			}
		}

		if ( bodypart >= 4 )
		{
			entity->roll = my->roll;
			entity->pitch = 0.0;// -my->pitch;
			entity->pitch = std::max(-PI / 32, std::min(PI / 32, entity->pitch));
		}

		switch ( bodypart )
		{
			// right pedipalp
			case 2:
				entity->x += cos(my->yaw) * 2 + cos(my->yaw + PI / 2) * 2;
				entity->y += sin(my->yaw) * 2 + sin(my->yaw + PI / 2) * 2;
				entity->yaw += PI / 10;
				//entity->pitch -= PI / 8;
				break;
			// left pedipalp
			case 3:
				entity->x += cos(my->yaw) * 2 - cos(my->yaw + PI / 2) * 2;
				entity->y += sin(my->yaw) * 2 - sin(my->yaw + PI / 2) * 2;
				entity->yaw -= PI / 10;
				//entity->pitch -= PI / 8;
				break;

			// 1st/5th leg:
			// thigh
			case 4:
			case 12:
				entity->x += cos(my->yaw) * 1 + cos(my->yaw + PI / 2) * 2.5 * (1 - 2 * (bodypart > 11));
				entity->y += sin(my->yaw) * 1 + sin(my->yaw + PI / 2) * 2.5 * (1 - 2 * (bodypart > 11));
				if ( dist > 0.1 )
				{
					if ( !entity->skill[4] )
					{
						entity->fskill[2] += .1;
						if ( entity->fskill[2] >= 1 )
						{
							entity->fskill[2] = 1;
							entity->skill[4] = 1;
						}
					}
					else
					{
						entity->fskill[2] -= .1;
						if ( entity->fskill[2] <= 0 )
						{
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
				if ( dist > 0.1 )
				{
					if ( !entity->skill[4] )
					{
						entity->fskill[2] += .1;
						if ( entity->fskill[2] >= 1 )
						{
							entity->fskill[2] = 1;
							entity->skill[4] = 1;
						}
					}
					else
					{
						entity->fskill[2] -= .1;
						if ( entity->fskill[2] <= 0 )
						{
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
				if ( dist > 0.1 )
				{
					if ( !entity->skill[4] )
					{
						entity->fskill[2] += .1;
						if ( entity->fskill[2] >= 1 )
						{
							entity->fskill[2] = 1;
							entity->skill[4] = 1;
						}
					}
					else
					{
						entity->fskill[2] -= .1;
						if ( entity->fskill[2] <= 0 )
						{
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
				if ( dist > 0.1 )
				{
					if ( !entity->skill[4] )
					{
						entity->fskill[2] += .1;
						if ( entity->fskill[2] >= 1 )
						{
							entity->fskill[2] = 1;
							entity->skill[4] = 1;
						}
					}
					else
					{
						entity->fskill[2] -= .1;
						if ( entity->fskill[2] <= 0 )
						{
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
				entity->flags[INVISIBLE] = true; // for debugging
				break;
		}
	}

	if ( MONSTER_ATTACK > 0 && MONSTER_ATTACK <= MONSTER_POSE_MAGIC_CAST3 )
	{
		MONSTER_ATTACKTIME++;
	}
	else if ( MONSTER_ATTACK == 0 )
	{
		MONSTER_ATTACKTIME = 0;
	}
	else
	{
		// do nothing, don't reset attacktime or increment it.
	}
}
