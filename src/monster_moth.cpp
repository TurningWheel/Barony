/*-------------------------------------------------------------------------------

	BARONY
	File: monster_skeleton.cpp
	Desc: implements all of the skeleton monster's code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "monster.hpp"
#include "engine/audio/sound.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "magic/magic.hpp"
#include "prng.hpp"
#include "scores.hpp"
#include "mod_tools.hpp"

void initMoth(Entity* my, Stat* myStats)
{
	node_t* node;

	my->z = 0;
	my->initMonster(1819);
	my->flags[INVISIBLE] = true; // hide the "AI" bodypart
	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = -1;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 3;
	}

	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		auto& rng = my->entity_rng ? *my->entity_rng : local_rng;

		if ( myStats != nullptr )
		{
			myStats->setAttribute("moth_state", "0");
			if ( !myStats->leader_uid )
			{
				myStats->leader_uid = 0;
			}

			// apply random stat increases if set in stat_shared.cpp or editor
			setRandomMonsterStats(myStats, rng);

			// generate 6 items max, less if there are any forced items from boss variants
			int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats, rng);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate, rng);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

			// count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			my->setHardcoreStats(*myStats);
		}
	}

	for ( int i = 0; i < 6; ++i )
	{
		// body
		Entity* entity = newEntity(1819, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 2;
		entity->sizey = 2;
		entity->skill[2] = my->getUID();
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[INVISIBLE] = true;
		entity->yaw = my->yaw;
		entity->z = 6;
		entity->flags[USERFLAG2] = my->flags[USERFLAG2];
		entity->focalx = limbs[MOTH_SMALL][1][0];
		entity->focaly = limbs[MOTH_SMALL][1][1];
		entity->focalz = limbs[MOTH_SMALL][1][2];
		entity->behavior = &actMothLimb;

		// animation offsets
		entity->fskill[1] = (2 * PI) * i / 6.0; // flapping

		if ( i == 1 || i == 3 )
		{
			entity->fskill[6] = i == 1 ? 1.0 : -1.0; // left/right
		}
		else if ( i == 0 || i == 2 )
		{
			entity->fskill[7] = i == 2 ? 1.0 : -1.0; // up/down
		}
		else if ( i == 4 )
		{
			entity->fskill[8] = -3.0; // circling
			entity->fskill[7] = 2.0; // up/down
		}
		else if ( i == 5 )
		{
			entity->fskill[8] = 3.0; // circling
			entity->fskill[7] = -2.0; // up/down
			entity->fskill[9] = PI; // current circling
		}

		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);

		// wingleft
		entity = newEntity(1820, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 2;
		entity->sizey = 2;
		entity->skill[2] = my->getUID();
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[INVISIBLE] = true;
		entity->yaw = my->yaw;
		entity->z = 6;
		entity->flags[USERFLAG2] = my->flags[USERFLAG2];
		entity->focalx = limbs[MOTH_SMALL][3][0];
		entity->focaly = limbs[MOTH_SMALL][3][1];
		entity->focalz = limbs[MOTH_SMALL][3][2];
		entity->behavior = &actMothLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);

		// wingright
		entity = newEntity(1821, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 2;
		entity->sizey = 2;
		entity->skill[2] = my->getUID();
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[INVISIBLE] = true;
		entity->yaw = my->yaw;
		entity->z = 6;
		entity->flags[USERFLAG2] = my->flags[USERFLAG2];
		entity->focalx = limbs[MOTH_SMALL][4][0];
		entity->focaly = limbs[MOTH_SMALL][4][1];
		entity->focalz = limbs[MOTH_SMALL][4][2];
		entity->behavior = &actMothLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);
	}
}

void actMothLimb(Entity* my)
{
	my->actMonsterLimb(false);
}

void mothDie(Entity* my)
{
	int c;
	for ( c = 0; c < 3; c++ )
	{
		Entity* entity = spawnGib(my);
		if ( entity )
		{
			entity->skill[5] = 1; // poof

			switch ( c )
			{
			case 0:
				entity->sprite = my->sprite;
				break;
			case 1:
				entity->sprite = my->sprite + 1;
				break;
			case 2:
				entity->sprite = my->sprite + 2;
				break;
			default:
				break;
			}

			serverSpawnGibForClient(entity);
		}
	}

	my->spawnBlood();

	playSoundEntity(my, 670 + local_rng.rand() % 2, 128);

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define MOTH_BODY 2
#define MOTH_LEFTWING 3
#define MOTH_RIGHTWING 4

#define BODY_ATTACK body->skill[3]
#define BODY_ATTACKTIME body->skill[4]
#define BODY_INIT body->skill[5]
#define BODY_FLOAT_X body->fskill[2]
#define BODY_FLOAT_Y body->fskill[3]
#define BODY_FLOAT_Z body->fskill[4]
#define BODY_FLOAT_ATK body->fskill[5]
#define BODY_LEFTRIGHT_OFFSET body->fskill[6]
#define BODY_HEIGHT_OFFSET body->fskill[7]
#define BODY_CIRCLING_AMOUNT body->fskill[8]
#define BODY_CURRENT_CIRCLING body->fskill[9]
#define BODY_OFFSET_REDUCE body->fskill[10]
#define BODY_CIRCLING_ATTACK body->fskill[11]
#define BODY_CIRCLING_ATTACK_SETPOINT body->fskill[12]

int mothGetAttackPose(Entity* my, int basePose)
{
	if ( !my )
	{
		return 0;
	}
	if ( basePose == MONSTER_POSE_MELEE_WINDUP1 )
	{
		// find a body available to attack
		std::vector<int> available;
		for ( int i = 0; i < my->bodyparts.size(); i += 3 )
		{
			if ( (i / 3) < 4 )
			{
				Entity* body = my->bodyparts.at(i);
				if ( BODY_ATTACK == 0 && !body->flags[INVISIBLE] )
				{
					available.push_back(i);
				}
			}
		}

		if ( available.size() > 0 )
		{
			int pick = available[local_rng.rand() % available.size()];
			switch ( pick / 3 )
			{
			case 0:
				return MONSTER_POSE_MELEE_WINDUP1;
			case 1:
				return MONSTER_POSE_MELEE_WINDUP2;
			case 2:
				return MONSTER_POSE_MELEE_WINDUP3;
			case 3:
				return MONSTER_POSE_RANGED_WINDUP1;
			default:
				return 0;
			}
		}
		else
		{
			return 0;
		}
	}
	else if ( basePose == MONSTER_POSE_MAGIC_WINDUP1 )
	{
		// find a body available to attack
		std::vector<int> available;
		for ( int i = 0; i < my->bodyparts.size(); i += 3 )
		{
			if ( (i / 3) >= 4 )
			{
				Entity* body = my->bodyparts.at(i);
				if ( BODY_ATTACK == 0 && !body->flags[INVISIBLE] )
				{
					available.push_back(i);
				}
			}
		}

		if ( available.size() > 0 )
		{
			int pick = available[local_rng.rand() % available.size()];
			switch ( pick / 3 )
			{
			case 4:
				return MONSTER_POSE_MAGIC_WINDUP1;
			case 5:
				return MONSTER_POSE_MAGIC_WINDUP2;
			default:
				return 0;
			}
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return basePose;
	}
}

void mothAnimate(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = nullptr;
	Entity* head = nullptr;
	int bodypart;

	my->flags[INVISIBLE] = true; // hide the "AI" bodypart
	//my->flags[PASSABLE] = true;

	my->sizex = 2;
	my->sizey = 2;

	my->focalx = limbs[MOTH_SMALL][0][0];
	my->focaly = limbs[MOTH_SMALL][0][1];
	my->focalz = limbs[MOTH_SMALL][0][2];
	if ( multiplayer != CLIENT )
	{
		my->z = limbs[MOTH_SMALL][5][2];
		if ( !myStats->getEffectActive(EFF_LEVITATING) )
		{
			myStats->setEffectActive(EFF_LEVITATING, 1);
			myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 0;
		}
	}

	//my->setEffect(EFF_STUNNED, true, -1, false);
	//my->monsterLookDir = 0.0;
	//my->yaw = 0.0;
	if ( enableDebugKeys && (svFlags & SV_FLAG_CHEATS) )
	{
		if ( keystatus[SDLK_KP_5] )
		{
			my->yaw += 0.05;
			my->monsterLookDir = my->yaw;
		}
		if ( keystatus[SDLK_g] )
		{
			keystatus[SDLK_g] = 0;
			//MONSTER_ATTACK = mothGetAttackPose(my, MONSTER_POSE_MELEE_WINDUP1);
			MONSTER_ATTACK = mothGetAttackPose(my, MONSTER_POSE_MAGIC_WINDUP1);
			MONSTER_ATTACKTIME = 0;
		}
		if ( keystatus[SDLK_h] )
		{
			keystatus[SDLK_h] = 0;
			myStats->setEffectValueUnsafe(EFF_STUNNED, myStats->getEffectActive(EFF_STUNNED) ? 0 : 1);
			myStats->EFFECTS_TIMERS[EFF_STUNNED] = myStats->getEffectActive(EFF_STUNNED) ? -1 : 0;
		}
	}

	if ( multiplayer != CLIENT && myStats )
	{
		real_t percentHP = myStats->HP / (real_t)std::max(1, myStats->MAXHP);
		if ( percentHP < 0.1 )
		{
			myStats->setAttribute("moth_state", "5");
		}
		else if ( percentHP < 0.2 )
		{
			myStats->setAttribute("moth_state", "4");
		}
		else if ( percentHP < 0.4 )
		{
			myStats->setAttribute("moth_state", "3");
		}
		else if ( percentHP < 0.6 )
		{
			myStats->setAttribute("moth_state", "2");
		}
		else if ( percentHP < 0.8 )
		{
			myStats->setAttribute("moth_state", "1");
		}
		else
		{
			myStats->setAttribute("moth_state", "0");
		}

		int state = myStats->getAttribute("moth_state") != "" ? std::stoi(myStats->getAttribute("moth_state")) : 0;
		std::set<int> toDisappear;
		if ( state >= 1 )
		{
			toDisappear.insert(5);
		}
		if ( state >= 2 )
		{
			toDisappear.insert(3);
		}
		if ( state >= 3 )
		{
			toDisappear.insert(2);
		}
		if ( state >= 4 )
		{
			toDisappear.insert(1);
		}
		if ( state >= 5 )
		{
			toDisappear.insert(4);
		}
		
		for ( int i = 0; i < my->bodyparts.size(); i += 3 )
		{
			int index = i / 3;
			Entity* body = my->bodyparts.at(i);
			if ( toDisappear.find(index) != toDisappear.end() )
			{
				if ( !my->bodyparts.at(i)->flags[INVISIBLE] )
				{
					my->bodyparts.at(i)->flags[INVISIBLE] = true;
					serverUpdateEntityBodypart(my, i + MOTH_BODY);
					for ( int c = 0; c < 3; c++ )
					{
						Entity* entity = spawnGib(my);
						if ( entity )
						{
							entity->x = my->bodyparts.at(i)->x;
							entity->y = my->bodyparts.at(i)->y;
							entity->z = my->bodyparts.at(i)->z;
							entity->skill[5] = 1; // poof

							switch ( c )
							{
							case 0:
								entity->sprite = my->bodyparts.at(i)->sprite;
								break;
							case 1:
								entity->sprite = my->bodyparts.at(i)->sprite + 1;
								break;
							case 2:
								entity->sprite = my->bodyparts.at(i)->sprite + 2;
								break;
							default:
								break;
							}

							serverSpawnGibForClient(entity);
						}
					}
				}
			}
			else
			{
				if ( !BODY_INIT )
				{
					BODY_INIT = 1;
					body->flags[INVISIBLE] = false;
				}
			}
		}
	}

	int numBodies = 0;
	for ( int i = 0; i < my->bodyparts.size(); i += 3 )
	{
		Entity* body = my->bodyparts.at(i);
		if ( !body->flags[INVISIBLE] )
		{
			++numBodies;
		}
	}

	if ( MONSTER_ATTACK >= MONSTER_POSE_MELEE_WINDUP1 && MONSTER_ATTACK <= MONSTER_POSE_RANGED_WINDUP1
		&& MONSTER_ATTACKTIME == 0 )
	{
		int bodypart = (MONSTER_ATTACK - MONSTER_POSE_MELEE_WINDUP1) * 3;
		if ( bodypart < my->bodyparts.size() )
		{
			Entity* body = my->bodyparts.at(bodypart);
			BODY_ATTACK = MONSTER_POSE_MELEE_WINDUP1;
			BODY_ATTACKTIME = 0;
		}
	}
	else if ( MONSTER_ATTACK >= MONSTER_POSE_MAGIC_WINDUP1 && MONSTER_ATTACK <= MONSTER_POSE_MAGIC_WINDUP2
		&& MONSTER_ATTACKTIME == 0 )
	{
		/*int bodypart = 3 * 4 + ((MONSTER_ATTACK - MONSTER_POSE_MAGIC_WINDUP1) * 3);
		if ( bodypart < my->bodyparts.size() )
		{
			Entity* body = my->bodyparts.at(bodypart);
			BODY_ATTACK = MONSTER_POSE_MAGIC_WINDUP1;
			BODY_ATTACKTIME = 0;
		}*/
		int bodypart = (local_rng.rand() % 6) * 3;
		if ( bodypart < my->bodyparts.size() )
		{
			Entity* body = my->bodyparts.at(bodypart);
			BODY_ATTACK = MONSTER_POSE_MAGIC_WINDUP1;
			BODY_ATTACKTIME = 0;
		}
	}

	//Move bodyparts
	Entity* leftWing = nullptr;
	Entity* body = nullptr;
	for ( bodypart = 0, node = my->children.first; node != nullptr; node = node->next, ++bodypart )
	{
		if ( bodypart < MOTH_BODY )
		{
			continue;
		}

		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		if ( (bodypart - MOTH_BODY) % 3 == 0 ) // bodies
		{
			body = entity;
		}

		if ( (bodypart - MOTH_BODY) / 3 >= 4 ) // bodies circling
		{
			entity->yaw = 0.0;
		}
		else
		{
			entity->yaw = my->yaw;
		}

		if ( (bodypart - MOTH_BODY) % 3 == 0 ) // bodies
		{
			body = entity;

			if ( multiplayer == SERVER )
			{
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

			entity->fskill[0] = fmod(entity->fskill[0], 2 * PI);
			while ( entity->fskill[0] >= PI )
			{
				entity->fskill[0] -= 2 * PI;
			}
			while ( entity->fskill[0] < -PI )
			{
				entity->fskill[0] += 2 * PI;
			}
			if ( MONSTER_ATTACK == 0 )
			{
				BODY_FLOAT_ATK = 0.0;
			}

			if ( BODY_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 )
			{
				if ( BODY_ATTACKTIME == 0 )
				{
					entity->fskill[0] = 0.0;
					entity->skill[1] = 0;
					BODY_FLOAT_ATK = 0.0;
					BODY_CIRCLING_ATTACK = 0.0;
					BODY_CIRCLING_ATTACK_SETPOINT = 2.0;
				}
				else
				{
					if ( BODY_ATTACKTIME >= 2 * TICKS_PER_SECOND )
					{
						BODY_ATTACK = 0;
						BODY_CIRCLING_ATTACK_SETPOINT = 0.0;
					}
				}
			}

			if ( BODY_ATTACK == MONSTER_POSE_MELEE_WINDUP1 || BODY_ATTACK == 1 ) // main body
			{
				{
					if ( BODY_ATTACKTIME == 0 )
					{
						entity->fskill[0] = 0.0;
						entity->skill[1] = 0;
						BODY_FLOAT_ATK = 0.0;
					}
					else
					{
						if ( BODY_ATTACKTIME >= (int)limbs[MOTH_SMALL][15][0] )
						{
							if ( BODY_ATTACKTIME == (int)limbs[MOTH_SMALL][15][0] )
							{
								if ( multiplayer != CLIENT )
								{
									const Sint32 temp = MONSTER_ATTACKTIME;
									if ( !body->flags[INVISIBLE] )
									{
										my->attack(1, 0, nullptr); // slop
									}
									BODY_ATTACK = 1;
									MONSTER_ATTACKTIME = temp;
								}
							}

							if ( entity->skill[1] == 0 )
							{
								real_t speed = limbs[MOTH_SMALL][13][2];
								real_t setpoint = PI / 16;
								if ( limbAngleWithinRange(entity->fskill[0], -speed, setpoint) )
								{
									entity->fskill[0] = setpoint;
									entity->skill[1] = 1;
								}
								else
								{
									entity->fskill[0] -= speed;
									entity->fskill[0] = std::max(entity->fskill[0], setpoint);
								}
							}
							else
							{
								real_t speed = limbs[MOTH_SMALL][13][1];
								entity->fskill[0] += speed;
								entity->fskill[0] = std::min(entity->fskill[0], 0.0);
							}
						}
						else
						{
							real_t speed = limbs[MOTH_SMALL][13][0];
							entity->fskill[0] -= speed;
							entity->fskill[0] = std::max(entity->fskill[0], -((PI / 2) + PI / 32));
						}

						if ( BODY_ATTACKTIME >= (int)limbs[MOTH_SMALL][18][0] )
						{
							BODY_FLOAT_ATK -= limbs[MOTH_SMALL][18][1];
							BODY_FLOAT_ATK = std::max(BODY_FLOAT_ATK, (real_t)limbs[MOTH_SMALL][18][2]);
						}
						else if ( BODY_ATTACKTIME >= (int)limbs[MOTH_SMALL][17][0] )
						{
							BODY_FLOAT_ATK += limbs[MOTH_SMALL][17][1];
							BODY_FLOAT_ATK = std::min(BODY_FLOAT_ATK, (real_t)limbs[MOTH_SMALL][17][2]);
						}
						else if ( BODY_ATTACKTIME >= (int)limbs[MOTH_SMALL][16][0] )
						{
							BODY_FLOAT_ATK -= limbs[MOTH_SMALL][16][1];
							BODY_FLOAT_ATK = std::max(BODY_FLOAT_ATK, (real_t)limbs[MOTH_SMALL][16][2]);
						}
					}

					if ( BODY_ATTACKTIME >= (int)limbs[MOTH_SMALL][15][1] )
					{
						BODY_ATTACK = 0;
					}
				}
			}
		}
		if ( (bodypart - MOTH_BODY) % 3 == 1 ) // leftwings
		{
			entity->fskill[1] = fmod(entity->fskill[1], 2 * PI);
			while ( entity->fskill[1] >= PI )
			{
				entity->fskill[1] -= 2 * PI;
			}
			while ( entity->fskill[1] < -PI )
			{
				entity->fskill[1] += 2 * PI;
			}
			entity->fskill[0] = fmod(entity->fskill[0], 2 * PI);
			while ( entity->fskill[0] >= PI )
			{
				entity->fskill[0] -= 2 * PI;
			}
			while ( entity->fskill[0] < -PI )
			{
				entity->fskill[0] += 2 * PI;
			}

			if ( BODY_ATTACK == MONSTER_POSE_MELEE_WINDUP1 || BODY_ATTACK == 1 )
			{
				if ( BODY_ATTACKTIME == 0 )
				{
					entity->fskill[1] = -PI / 2;
					entity->skill[1] = 0;
				}


				if ( entity->skill[1] == 0 )
				{
					real_t speed = limbs[MOTH_SMALL][14][0];
					if ( limbAngleWithinRange(entity->fskill[1], speed, 0.0) )
					{
						entity->fskill[1] = 0.0;
						entity->skill[1] = 1;
					}
					else
					{
						if ( entity->fskill[1] > 0.01 )
						{
							entity->fskill[1] -= speed;
						}
						else if ( entity->fskill[1] < -0.01 )
						{
							entity->fskill[1] += speed;
						}
					}
					entity->fskill[0] = body->fskill[0];
				}
				else if ( entity->skill[1] == 1 )
				{
					real_t speed = limbs[MOTH_SMALL][14][0];
					if ( limbAngleWithinRange(entity->fskill[1], speed, PI / 4) )
					{
						entity->fskill[1] = PI / 4;
						entity->skill[1] = 2;
					}
					else
					{
						entity->fskill[1] += speed;
					}
					entity->fskill[0] = body->fskill[0];
				}
				else if ( entity->skill[1] == 2 )
				{
					real_t speed = limbs[MOTH_SMALL][14][1];
					real_t setpoint = -PI / 2 - PI / 8;
					if ( limbAngleWithinRange(entity->fskill[1], -speed, setpoint) )
					{
						entity->fskill[1] = setpoint;
						entity->skill[1] = 3;
					}
					else
					{
						entity->fskill[1] -= speed;
					}
					entity->fskill[0] = body->fskill[0];
				}
				else if ( entity->skill[1] == 3 )
				{
					real_t speed = limbs[MOTH_SMALL][14][2];
					if ( limbAngleWithinRange(entity->fskill[1], speed, 0.0) )
					{
						entity->fskill[1] = 0.0;
						entity->skill[1] = 4;
					}
					else
					{
						entity->fskill[1] += speed;
					}
					if ( limbAngleWithinRange(entity->fskill[0], speed * 2, 0.0) )
					{
						entity->fskill[0] = 0.0;
					}
					else
					{
						entity->fskill[0] += speed * 2;
					}
				}
			}
		}

		switch ( bodypart )
		{
		case MOTH_BODY + 3 * 0:
		case MOTH_BODY + 3 * 1:
		case MOTH_BODY + 3 * 2:
		case MOTH_BODY + 3 * 3:
		case MOTH_BODY + 3 * 4:
		case MOTH_BODY + 3 * 5:
		{
			entity->x += limbs[MOTH_SMALL][6][0] * cos(entity->yaw);
			entity->y += limbs[MOTH_SMALL][6][1] * sin(entity->yaw);
			entity->z += limbs[MOTH_SMALL][6][2];
			entity->focalx = limbs[MOTH_SMALL][1][0];
			entity->focaly = limbs[MOTH_SMALL][1][1];
			entity->focalz = limbs[MOTH_SMALL][1][2];

			if ( enableDebugKeys && (svFlags & SV_FLAG_CHEATS) )
			{
				if ( keystatus[SDLK_KP_6] )
				{
					entity->fskill[0] -= 0.05;
				}
				else if ( keystatus[SDLK_KP_4] )
				{
					entity->fskill[0] += 0.05;
				}
			}
			entity->pitch = entity->fskill[0];

			if ( enableDebugKeys && (svFlags & SV_FLAG_CHEATS) )
			{
				if ( keystatus[SDLK_KP_PLUS] )
				{
					keystatus[SDLK_KP_PLUS] = 0;
					entity->skill[0] = entity->skill[0] == 0 ? 1 : 0;
				}
			}
			if ( entity->skill[0] == 0 )
			{
				entity->fskill[1] += 0.1;
			}

			{
				BODY_FLOAT_X = limbs[MOTH_SMALL][10][0] * sin(body->fskill[1] * limbs[MOTH_SMALL][11][0]) * cos(entity->yaw + PI / 2);
				BODY_FLOAT_Y = limbs[MOTH_SMALL][10][1] * sin(body->fskill[1] * limbs[MOTH_SMALL][11][1]) * sin(entity->yaw + PI / 2);
				BODY_FLOAT_Z = limbs[MOTH_SMALL][10][2] * sin(body->fskill[1] * limbs[MOTH_SMALL][11][2]);
				real_t floatAtkZ = BODY_FLOAT_ATK < 0 ? 2 * sin(BODY_FLOAT_ATK * PI / 8) : 0.5 * sin(BODY_FLOAT_ATK * PI / 8);
				BODY_FLOAT_Z += floatAtkZ;
			}

			BODY_FLOAT_X += BODY_FLOAT_ATK * cos(entity->yaw);
			BODY_FLOAT_Y += BODY_FLOAT_ATK * sin(entity->yaw);

			real_t reduce = 1.0 - BODY_OFFSET_REDUCE;
			real_t setpoint = std::max(0.0, (numBodies - 1) / (real_t)5);
			if ( BODY_OFFSET_REDUCE > setpoint + 0.01 )
			{
				BODY_OFFSET_REDUCE -= 0.05;
				BODY_OFFSET_REDUCE = std::max(setpoint, BODY_OFFSET_REDUCE);
			}
			if ( BODY_OFFSET_REDUCE < setpoint - 0.01 )
			{
				BODY_OFFSET_REDUCE += 0.05;
				BODY_OFFSET_REDUCE = std::min(setpoint, BODY_OFFSET_REDUCE);
			}

			BODY_FLOAT_X += BODY_OFFSET_REDUCE * BODY_LEFTRIGHT_OFFSET * cos(entity->yaw + PI / 2) * limbs[MOTH_SMALL][2][0];
			BODY_FLOAT_Y += BODY_OFFSET_REDUCE * BODY_LEFTRIGHT_OFFSET * sin(entity->yaw + PI / 2) * limbs[MOTH_SMALL][2][0];
			BODY_FLOAT_X += BODY_OFFSET_REDUCE * BODY_LEFTRIGHT_OFFSET * cos(entity->yaw) * limbs[MOTH_SMALL][2][1];
			BODY_FLOAT_Y += BODY_OFFSET_REDUCE * BODY_LEFTRIGHT_OFFSET * sin(entity->yaw) * limbs[MOTH_SMALL][2][1];
			BODY_FLOAT_Z += BODY_OFFSET_REDUCE * BODY_HEIGHT_OFFSET * limbs[MOTH_SMALL][2][2];

			{
				real_t setpoint = BODY_CIRCLING_ATTACK_SETPOINT;
				if ( BODY_CIRCLING_ATTACK > setpoint + 0.01 )
				{
					real_t diff = std::max(0.025, (BODY_CIRCLING_ATTACK - setpoint) / 10.0);
					BODY_CIRCLING_ATTACK -= diff;
					BODY_CIRCLING_ATTACK = std::max(setpoint, BODY_CIRCLING_ATTACK);
				}
				if ( BODY_CIRCLING_ATTACK < setpoint - 0.01 )
				{
					real_t diff = std::max(0.025, (setpoint - BODY_CIRCLING_ATTACK) / 10.0);
					BODY_CIRCLING_ATTACK += diff;
					BODY_CIRCLING_ATTACK = std::min(setpoint, BODY_CIRCLING_ATTACK);
				}
			}

			if ( abs(BODY_CIRCLING_AMOUNT) > 0.01 || abs(BODY_CIRCLING_ATTACK) > 0.01 )
			{
				static ConsoleVariable<float> cvar_moth_circle_yaw("/moth_circle_yaw", PI / 2);
				if ( (bodypart - MOTH_BODY) / 3 < 4 ) // static bodies
				{
					// no yaw?
				}
				else
				{
					entity->yaw += BODY_CURRENT_CIRCLING;
					entity->yaw += *cvar_moth_circle_yaw;
				}

				real_t speed = 1.0;
				if ( BODY_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 )
				{
					speed = 2.0;
				}

				real_t& amount = abs(BODY_CIRCLING_AMOUNT) > abs(BODY_CIRCLING_ATTACK)
					? BODY_CIRCLING_AMOUNT : BODY_CIRCLING_ATTACK;

				if ( BODY_CIRCLING_AMOUNT < 0.0 )
				{
					BODY_CURRENT_CIRCLING -= speed * 0.075;
				}
				else
				{
					BODY_CURRENT_CIRCLING += speed * 0.075;
				}
				BODY_CURRENT_CIRCLING = fmod(BODY_CURRENT_CIRCLING, 2 * PI);
				while ( BODY_CURRENT_CIRCLING >= PI )
				{
					BODY_CURRENT_CIRCLING -= 2 * PI;
				}
				while ( BODY_CURRENT_CIRCLING < -PI )
				{
					BODY_CURRENT_CIRCLING += 2 * PI;
				}
				BODY_FLOAT_X += amount * cos(BODY_CURRENT_CIRCLING);
				BODY_FLOAT_Y += amount * sin(BODY_CURRENT_CIRCLING);
			}

			entity->x += BODY_FLOAT_X;
			entity->y += BODY_FLOAT_Y;
			entity->z += BODY_FLOAT_Z;
			if ( BODY_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 )
			{
				Entity* fx = spawnMagicParticleCustom(entity, 576, 1.0, 10.0);
				fx->z += 0.5;
				fx->vel_z = 0.04;
			}
			break;
		}
		case MOTH_LEFTWING + 3 * 0:
		case MOTH_LEFTWING + 3 * 1:
		case MOTH_LEFTWING + 3 * 2:
		case MOTH_LEFTWING + 3 * 3:
		case MOTH_LEFTWING + 3 * 4:
		case MOTH_LEFTWING + 3 * 5:
			if ( body )
			{
				entity->yaw = body->yaw;
				entity->flags[INVISIBLE] = body->flags[INVISIBLE];
			}
			entity->x += limbs[MOTH_SMALL][8][0] * cos(entity->yaw + PI / 2);
			entity->y += limbs[MOTH_SMALL][8][1] * sin(entity->yaw + PI / 2);
			entity->z += limbs[MOTH_SMALL][8][2];
			entity->focalx = limbs[MOTH_SMALL][3][0];
			entity->focaly = limbs[MOTH_SMALL][3][1];
			entity->focalz = limbs[MOTH_SMALL][3][2];

			// wings flap sync with body
			{
				real_t wingMin = -1.3;
				real_t wingMax = 0.8;
				real_t wingMid = wingMin + (wingMax - wingMin) / 2;
				real_t speed = 1.0;
				if ( BODY_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 )
				{
					speed = 2.0;
				}
				entity->fskill[1] = wingMid + ((wingMax - wingMin) / 2) * sin(speed * body->fskill[1] * limbs[MOTH_SMALL][11][2]);
			}

			entity->pitch = entity->fskill[0];
			entity->roll = entity->fskill[1];
			leftWing = entity;

			if ( body )
			{
				entity->x += BODY_FLOAT_X;
				entity->y += BODY_FLOAT_Y;
				entity->z += BODY_FLOAT_Z;

				entity->x += limbs[MOTH_SMALL][12][0] * sin(body->pitch) * cos(entity->yaw);
				entity->y += limbs[MOTH_SMALL][12][1] * sin(body->pitch) * sin(entity->yaw);
				entity->z += limbs[MOTH_SMALL][12][2] * abs(sin(body->pitch / 2));
			}
			break;
		case MOTH_RIGHTWING + 3 * 0:
		case MOTH_RIGHTWING + 3 * 1:
		case MOTH_RIGHTWING + 3 * 2:
		case MOTH_RIGHTWING + 3 * 3:
		case MOTH_RIGHTWING + 3 * 4:
		case MOTH_RIGHTWING + 3 * 5:
			if ( body )
			{
				entity->yaw = body->yaw;
				entity->flags[INVISIBLE] = body->flags[INVISIBLE];
			}
			entity->x += limbs[MOTH_SMALL][9][0] * cos(entity->yaw + PI / 2);
			entity->y += limbs[MOTH_SMALL][9][1] * sin(entity->yaw + PI / 2);
			entity->z += limbs[MOTH_SMALL][9][2];
			entity->focalx = limbs[MOTH_SMALL][4][0];
			entity->focaly = limbs[MOTH_SMALL][4][1];
			entity->focalz = limbs[MOTH_SMALL][4][2];
			if ( leftWing )
			{
				entity->fskill[0] = leftWing->fskill[0];
				entity->fskill[1] = -leftWing->fskill[1];
			}
			entity->pitch = entity->fskill[0];
			entity->roll = entity->fskill[1];

			if ( body )
			{
				entity->x += BODY_FLOAT_X;
				entity->y += BODY_FLOAT_Y;
				entity->z += BODY_FLOAT_Z;

				entity->x += limbs[MOTH_SMALL][12][0] * sin(body->pitch) * cos(entity->yaw);
				entity->y += limbs[MOTH_SMALL][12][1] * sin(body->pitch) * sin(entity->yaw);
				entity->z += limbs[MOTH_SMALL][12][2] * abs(sin(body->pitch / 2));

			}
			break;
		default:
			break;
		}
	}

	if ( MONSTER_ATTACK > 0 )
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

	for (int i = 0; i < my->bodyparts.size(); i += 3)
	{
		Entity* body = my->bodyparts.at(i);
		if ( BODY_ATTACK > 0 )
		{
			BODY_ATTACKTIME++;
		}
		else if ( BODY_ATTACK == 0 )
		{
			BODY_ATTACKTIME = 0;
		}
		else
		{
			// do nothing, don't reset attacktime or increment it.
		}
	}
}