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
#include "mod_tools.hpp"

void initMimic(Entity* my, Stat* myStats)
{
	node_t* node;

	my->z = 0;
	my->initMonster(1247);
	my->flags[INVISIBLE] = true; // hide the "AI" bodypart
	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 619;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 1;
	}

	my->monsterSpecialState = MIMIC_INERT;

	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		auto& rng = my->entity_rng ? *my->entity_rng : local_rng;

		if ( myStats != nullptr )
		{
			if ( !myStats->leader_uid )
			{
				myStats->leader_uid = 0;
			}

			if ( isMonsterStatsDefault(*myStats) )
			{
				my->mimicSetStats(myStats);
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

			if ( rng.rand() % 10 == 0 )
			{
				my->setEffect(EFF_MIMIC_LOCKED, true, -1, false);
			}
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
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
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
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
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
	int c;
	for ( c = 0; c < 5; c++ )
	{
		Entity* entity = spawnGib(my);
		if ( entity )
		{
			serverSpawnGibForClient(entity);
		}
	}

	my->spawnBlood();

	// wood chunk particles
	for ( c = 0; c < 10; c++ )
	{
		Entity* entity = spawnGib(my);
		entity->skill[5] = 1; // poof
		entity->flags[INVISIBLE] = false;
		entity->sprite = 187; // Splinter.vox
		entity->x = my->x;
		entity->y = my->y;
		entity->z = -7 + local_rng.rand() % 14;
		entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
		entity->pitch = (local_rng.rand() % 360) * PI / 180.0;
		entity->roll = (local_rng.rand() % 360) * PI / 180.0;
		entity->vel_x = cos(entity->yaw) * (0.5 + (local_rng.rand() % 100) / 100.f);
		entity->vel_y = sin(entity->yaw) * (0.5 + (local_rng.rand() % 100) / 100.f);
		entity->vel_z = -.25;
		entity->fskill[3] = 0.04;
		serverSpawnGibForClient(entity);
	}
	playSoundEntity(my, 625 + local_rng.rand() % 3, 64);
	playSoundEntity(my, 177, 64);

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define MIMIC_TRUNK 2
#define MIMIC_LID 3

void mimicSpecialEat(Entity* my, Stat* myStats); // unused

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

	if ( multiplayer != CLIENT )
	{
		if ( my->monsterAttack == MONSTER_POSE_MIMIC_DISTURBED )
		{
			my->monsterRotate();
		}
		else if ( my->isInertMimic() )
		{
			my->monsterRotate();
		}
	}

	bool bWalkCycle = false;
	bool aggressiveMove = my->monsterState == MONSTER_STATE_ATTACK 
		|| my->monsterState == MONSTER_STATE_HUNT
		|| my->monsterState == MONSTER_STATE_PATH;
		//(my->monsterState == MONSTER_STATE_ATTACK || my->monsterState == MONSTER_STATE_HUNT) && dist > 0.1;

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

			auto& attackStageSwing = entity->skill[0];
			auto& attackStageRoll = entity->skill[1];

			auto& walkCycle = entity->skill[3];
			auto& walkEndDelay = entity->skill[4];
			auto& walkShuffleRollDelay = entity->skill[5];
			auto& walkShuffleYaw = entity->fskill[3];
			auto& bounceFromRoll = entity->fskill[4];
			auto& resetToInert = entity->fskill[6];


			if ( my->monsterAttack == MONSTER_POSE_MELEE_WINDUP1
				|| my->monsterAttack == MONSTER_POSE_MIMIC_DISTURBED
				|| my->monsterAttack == MONSTER_POSE_MIMIC_LOCKED )
			{
				if ( my->monsterAttackTime == 0 )
				{
					if ( my->monsterAttack == MONSTER_POSE_MELEE_WINDUP1 )
					{
						playSoundEntityLocal(my, 622 + local_rng.rand() % 3, 64);
					}
					attackStageSwing = 0;
					attackStageRoll = 0;
					entity->monsterWeaponYaw = 0.0;
					entity->roll = 0.0;
					walkShuffleYaw = 0;
					walkCycle = 0;
					walkShuffleRollDelay = 1;
					entity->fskill[2] = -1.0 + 0.4 * (local_rng.rand() % 6); // random roll adjustment for attack
					if ( my->monsterAttack == MONSTER_POSE_MIMIC_DISTURBED )
					{
						entity->fskill[2] = -0.5 + (local_rng.rand() % 2) * 1.0;
					}
					else if ( my->monsterAttack == MONSTER_POSE_MIMIC_LOCKED )
					{
						entity->fskill[2] = -1.0 + (local_rng.rand() % 2) * 2.0; // strong shake
					}
				}

				const real_t rollRate = entity->fskill[2] * -0.05;
				const real_t rollSetpoint = entity->fskill[2] * -0.15;

				if ( my->monsterAttackTime >= 0 )
				{
					if ( limbAngleWithinRange(entity->roll, rollRate, rollSetpoint) )
					{
						entity->roll = rollSetpoint;
					}
					else
					{
						entity->roll += rollRate;
					}

					real_t rate = 0.1 + (my->monsterAttack == MONSTER_POSE_MIMIC_DISTURBED ? 0.1 : 0.0);
					real_t setpoint = 2.0;
					if ( limbAngleWithinRange(entity->monsterWeaponYaw, rate, setpoint) )
					{
						entity->monsterWeaponYaw = setpoint;
						if ( my->monsterAttackTime >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
						{
							if ( multiplayer != CLIENT )
							{
								int pose = 1;
								if ( my->monsterAttack == MONSTER_POSE_MIMIC_DISTURBED )
								{
									pose = MONSTER_POSE_MIMIC_DISTURBED2;
								}
								else if ( my->monsterAttack == MONSTER_POSE_MIMIC_LOCKED )
								{
									pose = MONSTER_POSE_MIMIC_LOCKED2;
								}
								my->attack(pose, 0, nullptr);
							}
						}
					}
					else
					{
						entity->monsterWeaponYaw += rate;
					}
				}
			}
			else if ( my->monsterAttack == MONSTER_POSE_MIMIC_MAGIC1 )
			{
				if ( my->monsterAttackTime == 0 )
				{
					attackStageSwing = 0;
					attackStageRoll = 0;
					entity->monsterWeaponYaw = 0.0;
					entity->roll = 0.0;
					walkShuffleYaw = 0;
					walkCycle = 0;
					walkShuffleRollDelay = 1;
					entity->fskill[2] = 0.0;
				}

				if ( my->monsterAttackTime >= 0 )
				{
					real_t rate = limbs[MIMIC][16][0];
					real_t setpoint = limbs[MIMIC][16][1];
					if ( limbAngleWithinRange(entity->monsterWeaponYaw, rate, setpoint) )
					{
						entity->monsterWeaponYaw = setpoint;
						//if ( my->monsterAttackTime >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
						{
							if ( multiplayer != CLIENT )
							{
								int pose = MONSTER_POSE_MIMIC_MAGIC2;
								my->attack(pose, 0, nullptr);
							}
						}
					}
					else
					{
						entity->monsterWeaponYaw += rate;
					}

					entity->z -= -4 * sin(entity->monsterWeaponYaw * PI);
				}
			}
			else if ( my->monsterAttack == 1 || my->monsterAttack == MONSTER_POSE_MIMIC_DISTURBED2
				|| my->monsterAttack == MONSTER_POSE_MIMIC_LOCKED2
				|| my->monsterAttack == MONSTER_POSE_MIMIC_MAGIC2 )
			{
				real_t rollRate = entity->fskill[2] * limbs[MIMIC][11][0] * 2;
				const real_t rollSetpoint = entity->fskill[2] * limbs[MIMIC][11][1] * 2;
				if ( my->monsterAttack == MONSTER_POSE_MIMIC_DISTURBED2 )
				{
					rollRate *= 1.5;
				}

				if ( attackStageRoll == 0 )
				{
					if ( limbAngleWithinRange(entity->roll, -rollRate, -rollSetpoint) )
					{
						entity->roll = -rollSetpoint;
						++attackStageRoll;
					}
					else
					{
						entity->roll += -rollRate;
					}
				}
				else if ( attackStageRoll == 1 )
				{
					if ( limbAngleWithinRange(entity->roll, rollRate, 0.0) )
					{
						entity->roll = 0.0;
						++attackStageRoll;
						if ( my->monsterAttack == MONSTER_POSE_MIMIC_DISTURBED2 )
						{
							attackStageRoll = 0;
						}
					}
					else
					{
						entity->roll += rollRate;
					}
				}

				if ( my->monsterAttack == MONSTER_POSE_MIMIC_DISTURBED2 )
				{
					if ( my->monsterAttackTime >= TICKS_PER_SECOND / 5 )
					{
						if ( limbAngleWithinRange(entity->monsterWeaponYaw, -0.025 * 4, 0.0) )
						{
							attackStageSwing = 0;
							entity->monsterWeaponYaw = 0.0;
							my->monsterAttack = 0;
						}
						else
						{
							entity->monsterWeaponYaw -= 0.025 * 4;
						}
					}
				}
				else if ( my->monsterAttack == MONSTER_POSE_MIMIC_MAGIC2 )
				{
					if ( my->monsterAttackTime >= TICKS_PER_SECOND / 20 )
					{
						if ( limbAngleWithinRange(entity->monsterWeaponYaw, limbs[MIMIC][14][0], limbs[MIMIC][14][1]) )
						{
							attackStageSwing = 0;
							entity->monsterWeaponYaw = limbs[MIMIC][14][1];
							my->monsterAttack = 0;
						}
						else
						{
							entity->monsterWeaponYaw += limbs[MIMIC][14][0];
						}
					}
				}
				else if ( attackStageSwing == 0 )
				{
					if ( limbAngleWithinRange(entity->monsterWeaponYaw, limbs[MIMIC][10][0], limbs[MIMIC][10][1]) )
					{
						attackStageSwing = 1;
						entity->monsterWeaponYaw = limbs[MIMIC][10][1];
					}
					else
					{
						entity->monsterWeaponYaw += limbs[MIMIC][10][0];
					}
				}
				else if ( attackStageSwing == 1 )
				{
					if ( limbAngleWithinRange(entity->monsterWeaponYaw, 0.025 * 4, 0.0) )
					{
						attackStageSwing = 0;
						entity->monsterWeaponYaw = 0.0;
						my->monsterAttack = 0;
					}
					else
					{
						entity->monsterWeaponYaw += 0.025;
					}
				}
			}
			else if ( my->monsterAttack == 0 && !my->isInertMimic() )
			{
				// walk cycle
				bWalkCycle = true;
			}

			bounceFromRoll = 0.0;

			bool resetAnimation = (my->isInertMimic() && my->monsterAttack == 0) 
				|| (my->monsterSpecialState == MIMIC_STATUS_IMMOBILE);
			if ( !resetAnimation )
			{
				resetToInert = 1.0;
			}

			if ( bWalkCycle || resetAnimation )
			{
				if ( resetAnimation )
				{
					resetToInert -= 0.1;
					resetToInert = std::max(resetToInert, 0.0);
				}
				else if ( walkCycle % 2 == 0 )
				{
					entity->monsterWeaponYaw -= limbs[MIMIC][12][1];
					if ( walkShuffleRollDelay > 0 )
					{
						entity->monsterWeaponYaw -= limbs[MIMIC][12][1] * 4;
					}
					if ( entity->monsterWeaponYaw <= limbs[MIMIC][12][2] )
					{
						entity->monsterWeaponYaw = limbs[MIMIC][12][2];
						walkCycle++;

						if ( !aggressiveMove )
						{
							playSoundEntityLocal(my, 628 + local_rng.rand() % 4, 24);
						}
						else if ( dist > 0.001 )
						{
							playSoundEntityLocal(my, 615 + local_rng.rand() % 4, 48);
						}
						else
						{
							playSoundEntityLocal(my, 615 + local_rng.rand() % 4, 32);
						}
					}
				}
				else if ( walkCycle % 2 == 1 )
				{
					entity->monsterWeaponYaw += limbs[MIMIC][12][0];
					if ( entity->monsterWeaponYaw >= 0.0 )
					{
						entity->monsterWeaponYaw = 0.0;
						++walkEndDelay;

						bool longStep = !aggressiveMove;
						Sint32 stepTime = TICKS_PER_SECOND / 25;
						if ( longStep )
						{
							stepTime = TICKS_PER_SECOND / 5;
						}
						if ( walkEndDelay >= stepTime )
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
					walkShuffleYaw += rate;
					if ( walkShuffleRollDelay > 0 )
					{
						walkShuffleYaw += rate;
					}
					walkShuffleYaw = std::min(walkShuffleYaw, 1.0);
					if ( walkShuffleYaw >= 1.0 )
					{
						if ( walkShuffleRollDelay > 0 )
						{
							walkShuffleRollDelay = 0;
						}
					}
				}
				else
				{
					walkShuffleYaw -= rate;
					walkShuffleYaw = std::max(walkShuffleYaw, -1.0);
				}

				real_t rotate = -PI / 16 * (-1.0 + cos((PI / 2) * walkShuffleYaw));
				if ( resetAnimation )
				{
					rotate *= resetToInert;
					walkShuffleYaw *= resetToInert;
					entity->monsterWeaponYaw *= resetToInert;
				}

				if ( walkShuffleYaw >= 0.0 )
				{
					entity->yaw += -rotate;
				}
				else
				{
					entity->yaw += rotate;
				}

				real_t roll = sin((PI / 2) + (PI / 2) * abs(walkShuffleYaw));
				if ( resetAnimation )
				{
					roll *= resetToInert;
				}
				if ( walkShuffleRollDelay > 0 )
				{
					roll = 0.0;
				}
				entity->roll = (PI / 16) * (walkCycle % 4 < 2 ? -roll : roll);

				real_t bounceAmount = (aggressiveMove ? 2.0 : 1.0) + limbs[MIMIC][13][1];
				bounceFromRoll = roll * bounceAmount;
				entity->z -= bounceFromRoll;
			}

			entity->pitch = sin(PI * entity->monsterWeaponYaw / 3);
		}
		else if ( bodypart == MIMIC_LID )
		{
			auto& attackStageSwing = entity->skill[0];
			auto& lidBounceStage = entity->skill[1];
			auto& lidBounceAngle = entity->fskill[1];

			entity->pitch = head->pitch;

			if ( my->monsterAttack == MONSTER_POSE_MELEE_WINDUP1
				|| my->monsterAttack == MONSTER_POSE_MIMIC_DISTURBED
				|| my->monsterAttack == MONSTER_POSE_MIMIC_LOCKED )
			{
				if ( my->monsterAttackTime == 0 )
				{
					lidBounceStage = 0;
					attackStageSwing = 0;
					entity->monsterWeaponYaw = 0.0;
				}

				real_t rate = -0.25;
				real_t setpoint = -4.0;
				if ( my->monsterAttack == MONSTER_POSE_MIMIC_DISTURBED )
				{
					rate *= 1.75;
				}
				if ( limbAngleWithinRange(entity->monsterWeaponYaw, rate, setpoint) )
				{
					entity->monsterWeaponYaw = setpoint;
					attackStageSwing = 1;
				}
				else
				{
					entity->monsterWeaponYaw += rate;
				}
			}
			else if ( my->monsterAttack == MONSTER_POSE_MIMIC_DISTURBED2 )
			{
				real_t rate = 0.25;
				if ( my->monsterAttackTime >= TICKS_PER_SECOND / 5 )
				{
					if ( limbAngleWithinRange(entity->monsterWeaponYaw, rate, 0) )
					{
						entity->monsterWeaponYaw = 0.0;
						if ( attackStageSwing == 1 )
						{
							lidBounceStage = 1;
							lidBounceAngle = 0.0;
						}
						attackStageSwing = 2;
					}
					else
					{
						entity->monsterWeaponYaw += rate;
					}
				}
			}
			else if ( my->monsterAttack == 1
				|| my->monsterAttack == MONSTER_POSE_MIMIC_LOCKED2 )
			{
				real_t rate = 0.5;
				if ( limbAngleWithinRange(entity->monsterWeaponYaw, rate, 0) )
				{
					entity->monsterWeaponYaw = 0.0;
					if ( attackStageSwing == 1 )
					{
						lidBounceStage = 1;
						lidBounceAngle = 0.0;
					}
					attackStageSwing = 2;
				}
				else
				{
					entity->monsterWeaponYaw += rate;
				}
			}
			else if ( my->monsterAttack == MONSTER_POSE_MIMIC_MAGIC1 
				|| my->monsterAttack == MONSTER_POSE_MIMIC_MAGIC2 )
			{
				if ( my->monsterAttackTime == 0 && my->monsterAttack == MONSTER_POSE_MIMIC_MAGIC1 )
				{
					lidBounceStage = 0;
					lidBounceAngle = 0.0;
					attackStageSwing = 0;
					//entity->monsterWeaponYaw = 0.0;
				}

				if ( attackStageSwing == 0 )
				{
					real_t rate = limbs[MIMIC][15][1];
					real_t setpoint = limbs[MIMIC][15][2];
					if ( limbAngleWithinRange(entity->monsterWeaponYaw, rate, setpoint) )
					{
						entity->monsterWeaponYaw = setpoint;
						attackStageSwing = 1;
					}
					else
					{
						entity->monsterWeaponYaw += rate;
					}
				}
				else
				{
					real_t rate = limbs[MIMIC][15][0];
					if ( limbAngleWithinRange(entity->monsterWeaponYaw, rate, 0.0) )
					{
						entity->monsterWeaponYaw = 0.0;
						/*if ( attackStageSwing == 1 )
						{
							lidBounceStage = 1;
							lidBounceAngle = 0.0;
						}*/
						attackStageSwing = 2;
					}
					else
					{
						entity->monsterWeaponYaw += rate;
					}
				}

				if ( my->monsterAttack == MONSTER_POSE_MIMIC_MAGIC1 )
				{
					entity->z -= -4 * sin(head->monsterWeaponYaw * PI);
				}
			}

			if ( (my->monsterAttack == 0 && my->monsterSpecialState == MIMIC_MAGIC) )
			{
				real_t rate = -0.15;
				if ( limbAngleWithinRange(entity->monsterWeaponYaw, rate, -2.0) )
				{
					entity->monsterWeaponYaw = -2.0;
				}
				else
				{
					entity->monsterWeaponYaw += rate;
				}
			}

			if ( lidBounceStage > 0 )
			{
				lidBounceAngle += PI / 16;

				if ( lidBounceAngle >= PI )
				{
					lidBounceAngle = 0.0;
					++lidBounceStage;

					int numBounces = bWalkCycle ? 1 : 2;
					if ( lidBounceStage > numBounces )
					{
						lidBounceStage = 0;
						if ( !(my->monsterAttack == 0 && my->monsterSpecialState == MIMIC_MAGIC) )
						{
							entity->monsterWeaponYaw = 0.0;
						}
						lidBounceAngle = 0.0;
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

			real_t bounceAmount = aggressiveMove ? 1.0 : 0.5;
			real_t pitchAmount = entity->monsterWeaponYaw * PI / 8 - bounceAmount * sin(lidBounceAngle) / (lidBounceStage + 1.0);
			if ( my->monsterAttack == MONSTER_POSE_MIMIC_LOCKED || my->monsterAttack == MONSTER_POSE_MIMIC_LOCKED2 )
			{
				pitchAmount *= .1;
			}
			entity->pitch += pitchAmount;

			entity->roll = head->roll;

			// walk shuffle yaw
			if ( head->fskill[3] >= 0.0 )
			{
				entity->yaw += PI / 16 * (-1.0 + cos((PI / 2) * head->fskill[3]));
			}
			else
			{
				entity->yaw += -PI / 16 * (-1.0 + cos((PI / 2) * head->fskill[3]));
			}
			// walk cycle z offset
			auto& headBounceFromRoll = head->fskill[4];
			entity->z -= headBounceFromRoll;


			if ( bWalkCycle )
			{
				auto& walkCycle = entity->skill[3];
				if ( head->skill[3] != walkCycle )
				{
					walkCycle = head->skill[3];
					if ( walkCycle % 2 == 1 )
					{
						lidBounceStage = 1;
						lidBounceAngle = 0.0;
					}
				}
			}
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

	if ( MONSTER_ATTACK > 0 && (MONSTER_ATTACK <= MONSTER_POSE_MAGIC_CAST3 
		|| MONSTER_ATTACK == MONSTER_POSE_MIMIC_DISTURBED
		|| MONSTER_ATTACK == MONSTER_POSE_MIMIC_LOCKED
		|| MONSTER_ATTACK == MONSTER_POSE_MIMIC_LOCKED2
		|| MONSTER_ATTACK == MONSTER_POSE_MIMIC_DISTURBED2
		|| MONSTER_ATTACK == MONSTER_POSE_MIMIC_MAGIC1
		|| MONSTER_ATTACK == MONSTER_POSE_MIMIC_MAGIC2) )
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

bool Entity::disturbMimic(Entity* touched, bool takenDamage, bool doMessage)
{
	if ( monsterSpecialState == MIMIC_MAGIC )
	{
		monsterSpecialState = MIMIC_ACTIVE;
		serverUpdateEntitySkill(this, 33);
		return false;
	}
	if ( monsterSpecialState != MIMIC_INERT && monsterSpecialState != MIMIC_INERT_SECOND )
	{
		return false;
	}

	Stat* myStats = getStats();
	if ( myStats && takenDamage )
	{
		const int damageThreshold = 5;
		if ( (myStats->MAXHP - myStats->HP < damageThreshold) )
		{
			// no disturbing
			return false;
		}
	}

	if ( myStats && myStats->EFFECTS[EFF_MIMIC_LOCKED] )
	{
		if ( myStats->EFFECTS_TIMERS[EFF_MIMIC_LOCKED] == -1 )
		{
			// started locked, force open
			setEffect(EFF_MIMIC_LOCKED, false, 0, false);
		}
		else
		{
			monsterHitTime = HITRATE;
		}
	}

	if ( myStats && !myStats->EFFECTS[EFF_MIMIC_LOCKED] )
	{
		if ( monsterSpecialState == MIMIC_INERT )
		{
			// longer stun
			int duration = TICKS_PER_SECOND;
			int mimicLvl = currentlevel / 5;
			duration -= mimicLvl * 10;
			duration = std::max(duration, 20);
			setEffect(EFF_STUNNED, true, duration, false);
		}
		else
		{
			setEffect(EFF_STUNNED, true, 20, false);
		}
		monsterHitTime = HITRATE / 2;
		if ( touched && touched->behavior == &actPlayer )
		{
			if ( monsterSpecialState == MIMIC_INERT )
			{
				Compendium_t::Events_t::eventUpdateWorld(touched->skill[2], Compendium_t::CPDM_CHESTS_MIMICS_AWAKENED1ST, "chest", 1);
			}
			Compendium_t::Events_t::eventUpdateWorld(touched->skill[2], Compendium_t::CPDM_CHESTS_MIMICS_AWAKENED, "chest", 1);
		}
	}

	monsterSpecialState = MIMIC_ACTIVE;
	serverUpdateEntitySkill(this, 33);


	if ( touched )
	{
		lookAtEntity(*touched);
		if ( !uidToEntity(monsterTarget) )
		{
			monsterAcquireAttackTarget(*touched, MONSTER_STATE_PATH, true);
			if ( touched->behavior == &actPlayer )
			{
				messagePlayerColor(touched->skill[2], MESSAGE_INTERACTION, 
					makeColorRGB(255, 0 ,0), Language::get(6082));
			}
		}
	}

	if ( !myStats->EFFECTS[EFF_MIMIC_LOCKED] )
	{
		attack(MONSTER_POSE_MIMIC_DISTURBED, 0, nullptr);
		playSoundEntity(this, 21, 64);
		playSoundEntity(this, 619 + local_rng.rand() % 3, 64);
	}
	return true;
}

void Entity::mimicSetStats(Stat* myStats)
{
	if ( !myStats )
	{
		return;
	}

	myStats->HP = 90;
	myStats->STR = 15;
	myStats->CON = 3;
	myStats->LVL = 10;
	myStats->DEX = 0;
	myStats->PER = 5;
	
	int level = std::max(currentlevel, 0) / LENGTH_OF_LEVEL_REGION;
	myStats->LVL += 5 * level;
	myStats->STR += 3 * level;
	myStats->CON += 4 * level;
	myStats->DEX += 1 * level;
	myStats->HP += 50 * level;
	myStats->PER += 1 * level;
	myStats->RANDOM_GOLD = 50 + 50 * level;


	myStats->MAXHP = myStats->HP;
	myStats->OLDHP = myStats->HP;
}

MimicGenerator mimic_generator;

static ConsoleVariable<bool> cvar_mimic_test2("/mimic_test2", false);

void MimicGenerator::init()
{
	mimic_floors.clear();
	mimic_secret_floors.clear();
	mimic_rng.seedBytes(&uniqueGameKey, sizeof(uniqueGameKey));

	for ( int i = 0; i <= 35; i += 5 )
	{
		for ( int j = 0; j < 2; ++j )
		{
			auto& floors = (j == 0) ? mimic_floors : mimic_secret_floors;
			std::vector<unsigned int> chances = { 0, 10, 7, 7, 10 };
			if ( i == 0 && j == 0 )
			{
				chances[0] = 0;
				chances[1] = 0;
			}

			unsigned int res1 = mimic_rng.discrete(chances.data(), chances.size());
			chances[res1] = 0;
			unsigned int res2 = mimic_rng.discrete(chances.data(), chances.size());

			if ( (svFlags & SV_FLAG_CHEATS) && *cvar_mimic_test2 )
			{
				floors.insert(i + res1);
				floors.insert(i + res2);
			}
			else
			{
				auto chosen = mimic_rng.rand() % 2 == 0 ? res1 : res2;
				floors.insert(i + chosen);
			}
		}
	}
}

bool MimicGenerator::bForceSpawnForCurrentFloor()
{
	if ( secretlevel )
	{
		return mimic_secret_floors.find(currentlevel) != mimic_secret_floors.end();
	}
	else
	{
		return mimic_floors.find(currentlevel) != mimic_floors.end();
	}
}

void mimicSpecialEat(Entity* my, Stat* myStats)
{
	//my->monsterSpecialState = MIMIC_MAGIC;
	//serverUpdateEntitySkill(my, 33);

	//my->setEffect(EFF_MIMIC_LOCKED, false, 0, false);

	//my->attack(MONSTER_POSE_MIMIC_MAGIC1, 0, nullptr);

	//list_t* itemsList = nullptr;

	//int tx = my->x / 16;
	//int ty = my->y / 16;
	//getItemsOnTile(tx, ty, &itemsList); //Check the tile the monster is on for items.
	//getItemsOnTile(tx - 1, ty, &itemsList); //Check tile to the left.
	//getItemsOnTile(tx + 1, ty, &itemsList); //Check tile to the right.
	//getItemsOnTile(tx, ty - 1, &itemsList); //Check tile up.
	//getItemsOnTile(tx, ty + 1, &itemsList); //Check tile down.
	//getItemsOnTile(tx - 1, ty - 1, &itemsList); //Check tile diagonal up left.
	//getItemsOnTile(tx + 1, ty - 1, &itemsList); //Check tile diagonal up right.
	//getItemsOnTile(tx - 1, ty + 1, &itemsList); //Check tile diagonal down left.
	//getItemsOnTile(tx + 1, ty + 1, &itemsList); //Check tile diagonal down right.

	//if ( itemsList )
	//{
	//	for ( node_t* node = itemsList->first; node != nullptr; node = node->next )
	//	{
	//		if ( node->element )
	//		{
	//			/*if ( list_Size(&myStats->inventory) >= maxInventoryItems + 1 )
	//			{
	//				break;
	//			}*/

	//			Entity* entity = (Entity*)node->element;
	//			if ( entity->flags[INVISIBLE] )
	//			{
	//				continue; // ignore invisible items like Sokoban gloves or other scripted events.
	//			}

	//			if ( entity->behavior != &actItem )
	//			{
	//				continue;
	//			}

	//			double dist = sqrt(pow(my->x - entity->x, 2) + pow(my->y - entity->y, 2));
	//			if ( std::floor(dist) > 16.0 )
	//			{
	//				// item was too far away, continue.
	//				continue;
	//			}

	//			Item* item = newItemFromEntity(entity);
	//			if ( !item ) { continue; }

	//			Entity* spellEntity = createParticleSapCenter(my, entity, SPELL_STEAL_WEAPON, 175, 175);
	//			//playSoundEntity(my, 174, 128); // succeeded spell sound
	//			spellEntity->skill[7] = 1; // found weapon

	//			// store weapon data
	//			spellEntity->skill[10] = item->type;
	//			spellEntity->skill[11] = item->status;
	//			spellEntity->skill[12] = item->beatitude;
	//			spellEntity->skill[13] = item->count;
	//			spellEntity->skill[14] = item->appearance;
	//			spellEntity->skill[15] = item->identified;

	//			if ( item != nullptr )
	//			{
	//				free(item);
	//			}

	//			item = nullptr;
	//			list_RemoveNode(entity->mynode);
	//			break;
	//		}
	//	}
	//}
}