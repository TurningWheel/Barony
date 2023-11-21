/*-------------------------------------------------------------------------------

	BARONY
	File: monster_sentrybot.cpp
	Desc: implements all of the kobold monster's code

	Copyright 2013-2019 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "monster.hpp"
#include "engine/audio/sound.hpp"
#include "book.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "magic/magic.hpp"
#include "interface/interface.hpp"
#include "prng.hpp"

void initMimic(Entity* my, Stat* myStats)
{
	node_t* node;

	my->z = 0;
	my->initMonster(1247);
	my->flags[INVISIBLE] = true; // hide the "AI" bodypart
	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = -1;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 1;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		auto& rng = my->entity_rng ? *my->entity_rng : local_rng;

		if ( myStats != nullptr )
		{
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

			//my->setHardcoreStats(*myStats);
		}
	}

	// trunk
	Entity* entity = newEntity(1247, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->z = 6;
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[MIMIC][1][0];
	entity->focaly = limbs[MIMIC][1][1];
	entity->focalz = limbs[MIMIC][1][2];
	entity->behavior = &actMimicLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// lid
	entity = newEntity(1248, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[MIMIC][2][0];
	entity->focaly = limbs[MIMIC][2][1];
	entity->focalz = limbs[MIMIC][2][2];
	entity->behavior = &actMimicLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	if ( multiplayer == CLIENT || MONSTER_INIT )
	{
		return;
	}
}

void actMimicLimb(Entity* my)
{
	my->actMonsterLimb(false);
}

void mimicDie(Entity* my)
{
	bool gibs = true;

	my->removeMonsterDeathNodes();
	//if ( gibs )
	//{
	//	playSoundEntity(my, 451 + local_rng.rand() % 2, 128);
	//	int c;
	//	for ( c = 0; c < 5; c++ )
	//	{
	//		Entity* entity = spawnGib(my);
	//		if ( entity )
	//		{
	//		    entity->skill[5] = 1; // poof
	//			switch ( c )
	//			{
	//				case 0:
	//					entity->sprite = 889;
	//					break;
	//				case 1:
	//					entity->sprite = 890;
	//					break;
	//				case 2:
	//					entity->sprite = 891;
	//					break;
	//				case 3:
	//					entity->sprite = 892;
	//					break;
	//				case 4:
	//					entity->sprite = 893;
	//					break;
	//				default:
	//					break;
	//			}
	//			serverSpawnGibForClient(entity);
	//		}
	//	}
	//}

	list_RemoveNode(my->mynode);
	return;
}

#define MIMIC_TRUNK 2
#define MIMIC_LID 3

void mimicAnimate(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = nullptr;
	Entity* head = nullptr;
	int bodypart;

	my->flags[INVISIBLE] = true; // hide the "AI" bodypart

	my->focalx = limbs[MIMIC][0][0];
	my->focaly = limbs[MIMIC][0][1];
	my->focalz = limbs[MIMIC][0][2];
	if ( multiplayer != CLIENT )
	{
		my->z = limbs[MIMIC][5][2];
	}
	if ( keystatus[SDLK_j] )
	{
		keystatus[SDLK_j] = 0;
		my->monsterAttack = MONSTER_POSE_MELEE_WINDUP1;
		my->monsterAttackTime = 0;
	}
	//if ( keystatus[SDLK_c] )
	//{
	//	keystatus[SDLK_c] = 0;
	//	if ( keystatus[SDLK_LCTRL] )
	//	{
	//		my->fskill[6] -= 0.1;
	//	}
	//	else
	//	{
	//		my->fskill[6] += 0.1;
	//	}
	//}

	my->yaw += my->fskill[6];

	if ( keystatus[SDLK_n] )
	{
		keystatus[SDLK_n] = 0;
		myStats->EFFECTS[EFF_PARALYZED] = !myStats->EFFECTS[EFF_PARALYZED];
		myStats->EFFECTS_TIMERS[EFF_PARALYZED] = -1;
	}

	bool bWalkCycle = false;

	//Move bodyparts
	for ( bodypart = 0, node = my->children.first; node != nullptr; node = node->next, ++bodypart )
	{
		if ( bodypart < MIMIC_TRUNK )
		{
			continue;
		}

		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		entity->yaw = my->yaw;

		if ( bodypart == MIMIC_TRUNK )
		{
			head = entity;

			if ( keystatus[SDLK_LCTRL] && keystatus[SDLK_h] )
			{
				entity->monsterWeaponYaw += 0.01;
				if ( entity->monsterWeaponYaw >= 1.0 )
				{
					entity->monsterWeaponYaw = 1.0;
				}
			}
			else if ( !keystatus[SDLK_h] )
			{
				/*entity->monsterWeaponYaw -= 0.01;
				if ( entity->monsterWeaponYaw <= -1.0 )
				{
					entity->monsterWeaponYaw = -1.0;
				}*/
				// walk cycle
				bWalkCycle = true;
				auto& walkCycle = entity->skill[3];
				auto& walkEndDelay = entity->skill[4];
				if ( walkCycle % 2 == 0 )
				{
					entity->monsterWeaponYaw -= limbs[MIMIC][12][1];
					if ( entity->monsterWeaponYaw <= limbs[MIMIC][12][2] )
					{
						entity->monsterWeaponYaw = limbs[MIMIC][12][2];
						walkCycle++;
					}
				}
				else if ( walkCycle % 2 == 1 )
				{
					entity->monsterWeaponYaw += limbs[MIMIC][12][0];
					if ( entity->monsterWeaponYaw >= 0.0 )
					{
						entity->monsterWeaponYaw = 0.0;
						++walkEndDelay;
						if ( walkEndDelay >= TICKS_PER_SECOND / 25 )
						{
							walkEndDelay = 0;
							walkCycle++;
							if ( walkCycle >= 4 )
							{
								walkCycle = 0;
							}
						}
					}
				}

				real_t rate = limbs[MIMIC][13][0];
				if ( walkCycle % 4 < 2 )
				{
					entity->fskill[3] += rate;
					entity->fskill[3] = std::min(entity->fskill[3], 1.0);
				}
				else
				{
					entity->fskill[3] -= rate;
					entity->fskill[3] = std::max(entity->fskill[3], -1.0);
				}

				real_t rotate = -PI / 32 * (-1.0 + cos((PI / 2) * entity->fskill[3]));
				real_t roll = PI / 16 * sin((PI / 2) + (PI / 2) * abs(entity->fskill[3]));
				if ( entity->fskill[3] >= 0.0 )
				{
					entity->yaw += -rotate;
				}
				else
				{
					entity->yaw += rotate;
				}
				if ( keystatus[SDLK_LSHIFT] )
				{
					entity->roll = walkCycle % 4 < 2 ? -roll : roll;
				}
			}
			else
			{
				if ( my->monsterAttack == MONSTER_POSE_MELEE_WINDUP1 )
				{
					if ( my->monsterAttackTime == 0 )
					{
						entity->skill[0] = 0;
						entity->monsterWeaponYaw = 0.0;
						entity->skill[1] = 0;
						entity->roll = 0.0;
						entity->fskill[3] = 0;
						entity->fskill[2] = -1.0 + 0.4 * (local_rng.rand() % 6); // random roll adjustment for attack
					}

					const real_t rollRate = entity->fskill[2] * limbs[MIMIC][11][0];
					const real_t rollSetpoint = entity->fskill[2] * limbs[MIMIC][11][1];

					if ( my->monsterAttackTime >= limbs[MIMIC][9][2] )
					{
						if ( limbAngleWithinRange(entity->roll, rollRate, rollSetpoint) )
						{
							entity->roll = rollSetpoint;
						}
						else
						{
							entity->roll += rollRate;
						}

						if ( limbAngleWithinRange(entity->monsterWeaponYaw, limbs[MIMIC][9][0], limbs[MIMIC][9][1]) )
						{
							entity->monsterWeaponYaw = limbs[MIMIC][9][1];
							if ( my->monsterAttackTime >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
							{
								if ( multiplayer != CLIENT )
								{
									my->attack(1, 0, nullptr);
								}
							}
						}
						else
						{
							entity->monsterWeaponYaw += limbs[MIMIC][9][0];
						}
					}
				}
				else if ( my->monsterAttack == 1 )
				{
					const real_t rollRate = entity->fskill[2] * limbs[MIMIC][11][0] * 2;
					const real_t rollSetpoint = entity->fskill[2] * limbs[MIMIC][11][1] * 2;

					if ( entity->skill[1] == 0 )
					{
						if ( limbAngleWithinRange(entity->roll, -rollRate, -rollSetpoint) )
						{
							entity->roll = -rollSetpoint;
							entity->skill[1] = 1;
						}
						else
						{
							entity->roll += -rollRate;
						}
					}
					else if ( entity->skill[1] == 1 )
					{
						if ( limbAngleWithinRange(entity->roll, rollRate, 0.0) )
						{
							entity->roll = 0.0;
							entity->skill[1] = 2;
						}
						else
						{
							entity->roll += rollRate;
						}
					}

					if ( entity->skill[0] == 0 )
					{
						if ( limbAngleWithinRange(entity->monsterWeaponYaw, limbs[MIMIC][10][0], limbs[MIMIC][10][1]) )
						{
							entity->skill[0] = 1;
							entity->monsterWeaponYaw = limbs[MIMIC][10][1];
						}
						else
						{
							entity->monsterWeaponYaw += limbs[MIMIC][10][0];
						}
					}
					else if ( entity->skill[0] == 1 )
					{
						if ( limbAngleWithinRange(entity->monsterWeaponYaw, 0.025, 0.0) )
						{
							entity->skill[0] = 0;
							entity->monsterWeaponYaw = 0.0;
							my->monsterAttack = 0;
						}
						else
						{
							entity->monsterWeaponYaw += 0.025;
						}
					}
				}


				/*if ( entity->skill[0] == 0 )
				{
					entity->monsterWeaponYaw += 0.02;
					if ( entity->monsterWeaponYaw >= 1.0 )
					{
						entity->monsterWeaponYaw = 1.0;
						entity->skill[0] = 1;
					}
				}
				else if ( entity->skill[0] == 1 )
				{
					entity->monsterWeaponYaw -= 0.02;
					if ( entity->monsterWeaponYaw <= -0.25 )
					{
						entity->monsterWeaponYaw = -0.25;
						entity->skill[0] = 0;
					}
				}*/
			}

			if ( keystatus[SDLK_v] )
			{
				keystatus[SDLK_v] = 0;
				if ( keystatus[SDLK_LCTRL] )
				{
					entity->fskill[1] += 0.05;
				}
				else
				{
					entity->fskill[1] -= 0.05;
				}
			}
			entity->roll += entity->fskill[1];

			entity->pitch = sin(PI * entity->monsterWeaponYaw / 3);
		}
		else if ( bodypart == MIMIC_LID )
		{
			entity->pitch = head->pitch;
			if ( keystatus[SDLK_g] )
			{
				if ( keystatus[SDLK_LCTRL] )
				{
					keystatus[SDLK_g] = 0;

					entity->skill[1] = 1;
					entity->fskill[1] = 0.0;
				}
				else
				{
					entity->monsterWeaponYaw -= 0.05;
				}
			}

			if ( my->monsterAttack == MONSTER_POSE_MELEE_WINDUP1 )
			{
				if ( my->monsterAttackTime == 0 )
				{
					entity->skill[0] = 0;
					entity->skill[1] = 0;
					entity->monsterWeaponYaw = 0.0;
				}

				if ( limbAngleWithinRange(entity->monsterWeaponYaw, limbs[MIMIC][8][0], limbs[MIMIC][8][1]) )
				{
					entity->skill[0] = 1;
				}
				else
				{
					entity->monsterWeaponYaw += limbs[MIMIC][8][0];
				}
			}
			else if ( my->monsterAttack == 1 )
			{
				if ( limbAngleWithinRange(entity->monsterWeaponYaw, limbs[MIMIC][8][2], 0) )
				{
					entity->skill[0] = 1;
					entity->monsterWeaponYaw = 0.0;
					entity->skill[1] = 1;
					entity->fskill[1] = 0.0;
				}
				else
				{
					entity->monsterWeaponYaw += limbs[MIMIC][8][2];
				}
			}

			if ( entity->skill[1] > 0 )
			{
				entity->fskill[1] += PI / 16;

				if ( entity->fskill[1] >= PI )
				{
					entity->fskill[1] = 0.0;
					++entity->skill[1];

					if ( entity->skill[1] > 1 )
					{
						entity->skill[1] = 0;
						entity->monsterWeaponYaw = 0.0;
						entity->fskill[1] = 0.0;
					}
				}
			}
			entity->monsterWeaponYaw = std::min(0.0, entity->monsterWeaponYaw);
			while ( entity->monsterWeaponYaw > 0.0 )
			{
				entity->monsterWeaponYaw -= 2 * PI;
			}
			while ( entity->monsterWeaponYaw < -2 * PI )
			{
				entity->monsterWeaponYaw += 2 * PI;
			}

			entity->pitch += entity->monsterWeaponYaw * PI / 8 - 0.5 * sin(entity->fskill[1]) / (entity->skill[1] + 1.0);

			entity->roll = head->roll;

			// walk cycle
			if ( head->fskill[3] >= 0.0 )
			{
				entity->yaw += PI / 32 * (-1.0 + cos((PI / 2) * head->fskill[3]));
			}
			else
			{
				entity->yaw += -PI / 32 * (-1.0 + cos((PI / 2) * head->fskill[3]));
			}

			auto& walkCycle = entity->skill[3];
			if ( head->skill[3] != walkCycle )
			{
				walkCycle = head->skill[3];
				if ( walkCycle % 2 == 1 )
				{
					entity->skill[1] = 1;
					entity->fskill[1] = 0.0;
				}
			}

			/*if ( my->monsterAttack == 0 )
			{
				entity->monsterWeaponYaw = -0.3 * cos((PI / 2) * head->fskill[3]);
				if ( entity->skill[1] == 0 )
				{

				}
			}*/
		}

		switch ( bodypart )
		{
			case MIMIC_TRUNK:
				entity->x += limbs[MIMIC][3][0] * cos(my->yaw);
				entity->y += limbs[MIMIC][3][1] * sin(my->yaw);
				entity->z += limbs[MIMIC][3][2];
				entity->focalx = limbs[MIMIC][1][0];
				entity->focaly = limbs[MIMIC][1][1];
				entity->focalz = limbs[MIMIC][1][2];

				if ( entity->pitch < 0.0 )
				{
					entity->z += -0.5 * sin(entity->pitch) * limbs[MIMIC][7][2];
				}
				else
				{
					entity->z += -0.5 * sin(entity->pitch) * limbs[MIMIC][7][1];
				}

				//entity->z += sin(PI * entity->fskill[0] / 3) * limbs[MIMIC][7][2];
				break;
			case MIMIC_LID:
				entity->x += limbs[MIMIC][4][0] * cos(my->yaw);
				entity->y += limbs[MIMIC][4][1] * sin(my->yaw);
				entity->z += limbs[MIMIC][4][2];
				entity->focalx = limbs[MIMIC][2][0];
				entity->focaly = limbs[MIMIC][2][1];
				entity->focalz = limbs[MIMIC][2][2];

				//entity->x += limbs[MIMIC][6][0] * cos(my->yaw) * sin(entity->fskill[0] * PI / 8);
				//entity->y += limbs[MIMIC][6][1] * sin(my->yaw) * sin(entity->fskill[0] * PI / 8);
				//entity->z += limbs[MIMIC][6][2] * sin(entity->fskill[0] * PI / 8);
				if ( head->pitch < 0.0 )
				{
					entity->z += -0.5 * sin(head->pitch) * limbs[MIMIC][6][2];
				}
				else
				{
					entity->z += -0.5 * sin(head->pitch) * limbs[MIMIC][6][1];
				}
				break;
			default:
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

