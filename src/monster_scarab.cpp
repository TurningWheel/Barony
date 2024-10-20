/*-------------------------------------------------------------------------------

	BARONY
	File: monster_scarab.cpp
	Desc: implements all of the scarab monster's code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "monster.hpp"
#include "engine/audio/sound.hpp"
#include "items.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "prng.hpp"
#include "scores.hpp"

void initScarab(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	my->flags[BURNABLE] = true;
	my->flags[UPDATENEEDED] = true;
	my->flags[INVISIBLE] = false;

	my->initMonster(429);

    auto& scarabFly = my->fskill[24];
    scarabFly = 0.0;

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 310;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = 306;
		MONSTER_IDLEVAR = 2;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		auto& rng = my->entity_rng ? *my->entity_rng : local_rng;

		if ( myStats != NULL )
		{
		    if ( !strncmp(map.name, "The Labyrinth", 13) )
		    {
				myStats->DEX -= 4;
			    myStats->LVL = 10;
		    }
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
				myStats->setAttribute("special_npc", "xyggi");
				strcpy(myStats->name, MonsterData_t::getSpecialNPCName(*myStats).c_str());
				my->sprite = MonsterData_t::getSpecialNPCBaseModel(*myStats);
				myStats->sex = FEMALE;
				myStats->HP = 70;
				myStats->MAXHP = 70;
				myStats->OLDHP = myStats->HP;
				myStats->MP = 40;
				myStats->MAXMP = 40;
				myStats->STR = -1;
				myStats->DEX = 20;
				myStats->CON = 2;
				myStats->INT = 20;
				myStats->PER = -2;
				myStats->CHR = 5;
				myStats->LVL = 10;
				my->setEffect(EFF_MAGICREFLECT, true, -1, true); //-1 duration, never expires.
				newItem(ENCHANTED_FEATHER, EXCELLENT, 0, 1, (ENCHANTED_FEATHER_MAX_DURABILITY - 1), false, &myStats->inventory);
				myStats->weapon = newItem(SPELLBOOK_COLD, EXCELLENT, 0, 1, 0, false, NULL);
				customItemsToGenerate = customItemsToGenerate - 1;
				int c;
				for ( c = 0; c < 4; ++c )
				{
					Entity* entity = summonMonster(SCARAB, my->x, my->y);
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

			int playerCount = 0;
			for ( c = 0; c < MAXPLAYERS; ++c )
			{
				if ( !client_disconnected[c] )
				{
					++playerCount;
				}
			}

			// generate the default inventory items for the monster, provided the editor sprite allowed enough default slots
			switch ( defaultItems )
			{
				case 6:
				case 5:
				case 4:
				case 3:
				case 2:
				case 1:
					if ( rng.rand() % 2 || playerCount > 1 )
					{
						if ( rng.rand() % 3 > 0 )
						{
							newItem(FOOD_TOMALLEY, static_cast<Status>(DECREPIT + rng.rand() % 4), 0, 1, rng.rand(), false, &myStats->inventory);
						}
						else
						{
							ItemType gem = GEM_GLASS;
							switch( rng.rand() % 7 )
							{
								case 0:
									gem = GEM_OPAL;
									break;
								case 1:
									gem = GEM_JADE;
									break;
								case 2:
									gem = GEM_AMETHYST;
									break;
								case 3:
									gem = GEM_FLUORITE;
									break;
								case 4:
									gem = GEM_JETSTONE;
									break;
								case 5:
									gem = GEM_OBSIDIAN;
									break;
								default:
									gem = GEM_GLASS;
									break;
							}
							newItem(gem, static_cast<Status>(DECREPIT + rng.rand()%2), (rng.rand()%4 == 0), 1, rng.rand(), false, &myStats->inventory);
						}
						if ( playerCount > 2 )
						{
							newItem(FOOD_TOMALLEY, static_cast<Status>(DECREPIT + rng.rand() % 4), 0, 1 + rng.rand() % 2, rng.rand(), false, &myStats->inventory);
						}
					}
					break;
				default:
					break;
			}
		}
	}

	if ( multiplayer != CLIENT && myStats )
	{
		if ( myStats->getAttribute("special_npc") == "xyggi" )
		{
			my->z = 3.25;
		}
	}

	// right wing
	Entity* entity = newEntity(my->sprite == 1078 ? 1077 : 483, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 5;
	entity->sizey = 11;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SCARAB][1][0]; // 0
	entity->focaly = limbs[SCARAB][1][1] + 2; // 0
	entity->focalz = limbs[SCARAB][1][2]; // 0
	entity->behavior = &actScarabLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left wing
	entity = newEntity(my->sprite == 1078 ? 1076 : 484, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 5;
	entity->sizey = 11;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SCARAB][2][0]; // 0
	entity->focaly = limbs[SCARAB][2][1] - 2; // 0
	entity->focalz = limbs[SCARAB][2][2]; // 0
	entity->behavior = &actScarabLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);
}

void scarabAnimate(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	int bodypart;
	Entity* entity = nullptr;

	// set invisibility //TODO: isInvisible()?
	if ( multiplayer != CLIENT )
	{
		if ( myStats->EFFECTS[EFF_INVISIBLE] == true )
		{
			my->flags[INVISIBLE] = true;
			my->flags[BLOCKSIGHT] = false;
			bodypart = 0;
			for ( node = my->children.first; node != nullptr; node = node->next )
			{
				if ( bodypart >= 3 )
				{
					break;
				}
				entity = (Entity*)node->element;
				if ( !entity->flags[INVISIBLE] )
				{
					entity->flags[INVISIBLE] = true;
					serverUpdateEntityBodypart(my, bodypart);
				}
				++bodypart;
			}
		}
		else
		{
			my->flags[INVISIBLE] = false;
			//my->flags[BLOCKSIGHT] = true; //No. It never blocks sight.
			bodypart = 0;
			for ( node = my->children.first; node != NULL; node = node->next )
			{
				if ( bodypart < 2 )
				{
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

	const bool xyggi = (my->sprite == 1078 || my->sprite == 1079);

	// move wings
	for ( bodypart = 0, node = my->children.first; node != nullptr; node = node->next, ++bodypart )
	{
		//messagePlayer(0, "bodypart - %d", bodypart);
		if ( bodypart < 2 )
		{

		}
		else
		{
			//if ( bodypart == 2 || bodypart == 3 )
			//{
			//messagePlayer(0, "bodypart - %d", bodypart);
			entity = (Entity*)node->element;
			entity->x = my->x - 1.1 * cos(my->yaw);
			entity->y = my->y - 1.1 * sin(my->yaw);
			entity->z = my->z - 3.4;
			entity->yaw = my->yaw;

			if ( bodypart == 2 )
			{
				if ( MONSTER_ATTACK == 1 )
				{
					if ( MONSTER_ATTACKTIME == 0 )
					{
						entity->pitch = 0;
						entity->roll = 0;
						entity->skill[0] = 0;
					}
					else
					{
						if ( entity->skill[0] == 0 )
						{
							if ( MONSTER_ATTACKTIME <= 5 )
							{
								entity->pitch = 1;
								entity->roll = -1;
							}
							else
							{
								entity->skill[0] = 1;
							}
						}
						else if ( entity->skill[0] == 1 )
						{
							entity->pitch -= 0.1;
							if ( entity->roll < 0)
							{
								entity->roll += 0.1;
							}
							if ( entity->pitch <= 0 && entity->roll >= 0)
							{
								entity->skill[0] = 0;
								entity->pitch = 0;
								entity->roll = 0;
								MONSTER_ATTACK = 0;
							}
						}
					}
				}
				else if ( my->monsterState == 1 )
				{
					if ( entity->pitch < 0.5 )
					{
						entity->pitch += 0.1;
					}
					else
					{
						entity->pitch = 0.5;
					}

					if ( entity->roll > -0.2 )
					{
						entity->roll -= 0.1;
					}
					else
					{
						entity->roll = -0.2;
					}
				}
				else if ( my->monsterState == 0 )
				{
					if ( entity->pitch > 0 )
					{
						entity->pitch -= 0.1;
					}
					else
					{
						entity->pitch = 0;
					}

					if ( entity->roll < 0 )
					{
						entity->roll += 0.1;
					}
					else
					{
						entity->roll = 0;
					}
				}

				if ( multiplayer == SERVER )
				{
					// update sprites for clients
					if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
					{
						bool updateBodypart = false;
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							updateBodypart = true;
						}
						if ( entity->skill[11] != entity->flags[INVISIBLE] )
						{
							entity->skill[11] = entity->flags[INVISIBLE];
							updateBodypart = true;
						}
						if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
						{
							updateBodypart = true;
						}
						if ( updateBodypart )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
			}
			else if ( bodypart == 3 )
			{
				if ( MONSTER_ATTACK == 1 )
				{
					if ( MONSTER_ATTACKTIME == 0 )
					{
						entity->pitch = 0;
						entity->roll = 0;
						entity->skill[0] = 0;
					}
					else
					{
						if ( entity->skill[0] == 0 )
						{
							if ( MONSTER_ATTACKTIME <= 5 )
							{
								entity->pitch = 1;
								entity->roll = 1;
							}
							else
							{
								entity->skill[0] = 1;
							}
						}
						else if ( entity->skill[0] == 1 )
						{
							entity->pitch -= 0.1;
							if ( entity->roll > 0 )
							{
								entity->roll -= 0.1;
							}
							if ( entity->pitch <= 0 && entity->roll <= 0 )
							{
								entity->skill[0] = 0;
								entity->pitch = 0;
								entity->roll = 0;
								MONSTER_ATTACK = 0;
							}
						}
					}
				}
				else if ( my->monsterState == 1 )
				{
					if ( entity->pitch < 0.5 )
					{
						entity->pitch += 0.1;
					}
					else
					{
						entity->pitch = 0.5;
					}

					if ( entity->roll < 0.2 )
					{
						entity->roll += 0.1;
					}
					else
					{
						entity->roll = 0.2;
					}
				}
				else if ( my->monsterState == 0 )
				{
					if ( entity->pitch > 0 )
					{
						entity->pitch -= 0.1;
					}
					else
					{
						entity->pitch = 0;
					}

					if ( entity->roll > 0 )
					{
						entity->roll -= 0.1;
					}
					else
					{
						entity->roll = 0;
					}
				}

				if ( multiplayer == SERVER )
				{
					// update sprites for clients
					if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
					{
						bool updateBodypart = false;
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							updateBodypart = true;
						}
						if ( entity->skill[11] != entity->flags[INVISIBLE] )
						{
							entity->skill[11] = entity->flags[INVISIBLE];
							updateBodypart = true;
						}
						if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
						{
							updateBodypart = true;
						}
						if ( updateBodypart )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
			}
		}
	}

    auto& scarabFly = my->fskill[24];

	// move body
	if (xyggi)
	{
	    if ( (ticks % 10 == 0 && dist > 0.1) )
	    {
		    my->sprite = my->sprite == 1078 ? 1079 : 1078;
		}
	} else {
        if (MONSTER_ATTACK)
        {
	        MONSTER_ATTACKTIME++;
        }
	    if (MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP1)
	    {
	        if (scarabFly < PI / 2.0) {
	            scarabFly += (PI / TICKS_PER_SECOND) * 2.0;
	            if (scarabFly >= PI / 2.0) {
	                scarabFly = PI / 2.0;
	                my->attack(1, 0, nullptr); // munch
	            }
	        }
	        my->sprite = 1075;
		    my->focalz = -1;
	    }
	    else
	    {
            if (scarabFly > 0.0) {
                scarabFly -= PI / TICKS_PER_SECOND;
                if (scarabFly <= 0.0) {
                    scarabFly = 0.0;
	                my->sprite = 429;
	                my->focalz = limbs[SCARAB][0][2];
	                MONSTER_ATTACK = 0;
	                MONSTER_ATTACKTIME = 0;
                }
            }
            if (my->sprite == 429 || my->sprite == 430) {
                if (ticks % 10 == 0 && dist > 0.1)
                {
	                my->sprite = my->sprite == 429 ? 430 : 429;
	            }
	        }
	    }
        my->new_z = my->z = 6.0 - sin(scarabFly) * 6.0;
	}
}

void actScarabLimb(Entity* my)
{
	my->actMonsterLimb(true); //Can create light, but can't hold a lightsource.
}

void scarabDie(Entity* my)
{
	Entity* gib = spawnGib(my);
	gib->sprite = my->sprite;
	gib->skill[5] = 1; // poof
	serverSpawnGibForClient(gib);
	for ( int c = 0; c < 2; c++ )
	{
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}

	my->spawnBlood(212);

	playSoundEntity(my, 308 + local_rng.rand() % 2, 64); //TODO: Scarab death sound effect.

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}
