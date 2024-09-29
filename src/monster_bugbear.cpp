/*-------------------------------------------------------------------------------

	BARONY
	File: monster_bugbear.cpp
	Desc: implements all of the bugbear monster's code

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
#include "prng.hpp"
#include "scores.hpp"

void initBugbear(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	my->flags[BURNABLE] = true;
	my->initMonster(1412);
	my->z = limbs[BUGBEAR][11][2];

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 679;
		MONSTER_SPOTVAR = 2;
		MONSTER_IDLESND = 673;
		MONSTER_IDLEVAR = 3;
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
					break;
				default:
					break;
			}

			bool hasAlly = false;
			if ( myStats->leader_uid == 0 && !my->flags[USERFLAG2] && rng.rand() % 5 == 0 )
			{
				Entity* entity = summonMonster(BUGBEAR, my->x, my->y);
				if ( entity )
				{
					hasAlly = true;
					entity->parent = my->getUID();
					if ( Stat* followerStats = entity->getStats() )
					{
						followerStats->leader_uid = entity->parent;
					}
					my->parent = entity->getUID(); // so I know my ally
					entity->seedEntityRNG(rng.getU32());
				}
			}

			if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
			{
				if ( myStats->leader_uid != 0 ) // minion
				{
					if ( Entity* leader = uidToEntity(myStats->leader_uid) )
					{
						if ( leader->hasRangedWeapon() || rng.rand() % 2 == 0 )
						{
							if ( rng.rand() % 2 == 0 )
							{
								myStats->weapon = newItem(STEEL_SWORD, static_cast<Status>(rng.rand() % 2 + SERVICABLE), -1 + rng.rand() % 3, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
							}
							else
							{
								myStats->weapon = newItem(STEEL_AXE, static_cast<Status>(rng.rand() % 2 + SERVICABLE), -1 + rng.rand() % 3, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
							}
						}
						else
						{
							myStats->weapon = newItem(HEAVY_CROSSBOW, static_cast<Status>(rng.rand() % 2 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
						}
					}
				}
				else
				{
					if ( rng.rand() % 4 == 0 && !hasAlly )
					{
						myStats->weapon = newItem(HEAVY_CROSSBOW, static_cast<Status>(rng.rand() % 2 + WORN), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
					}
					else
					{
						if ( rng.rand() % 2 == 0 )
						{
							myStats->weapon = newItem(STEEL_SWORD, static_cast<Status>(rng.rand() % 2 + SERVICABLE), -1 + rng.rand() % 3, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
						}
						else
						{
							myStats->weapon = newItem(STEEL_AXE, static_cast<Status>(rng.rand() % 2 + SERVICABLE), -1 + rng.rand() % 3, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
						}
					}
				}
			}

			if ( myStats->shield == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] == 1 )
			{
				if ( myStats->leader_uid != 0 ) // minion
				{
					if ( Entity* leader = uidToEntity(myStats->leader_uid) )
					{
						if ( !leader->hasRangedWeapon() && rng.rand() % 4 == 0 )
						{
							myStats->shield = newItem(STEEL_SHIELD, static_cast<Status>(rng.rand() % 2 + SERVICABLE), -1 + rng.rand() % 3, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
						}
					}
				}
				else
				{
					if ( (hasAlly && rng.rand() % 4 == 0) || (!hasAlly && rng.rand() % 3 > 0) )
					{
						myStats->shield = newItem(STEEL_SHIELD, static_cast<Status>(rng.rand() % 2 + SERVICABLE), -1 + rng.rand() % 3, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
					}
				}
			}
		}
	}

	// torso
	Entity* entity = newEntity(1413, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[BUGBEAR][1][0]; // 0
	entity->focaly = limbs[BUGBEAR][1][1]; // 0
	entity->focalz = limbs[BUGBEAR][1][2]; // 0
	entity->behavior = &actBugbearLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right leg
	entity = newEntity(1419, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[BUGBEAR][2][0]; // 1
	entity->focaly = limbs[BUGBEAR][2][1]; // 0
	entity->focalz = limbs[BUGBEAR][2][2]; // 2
	entity->behavior = &actBugbearLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left leg
	entity = newEntity(1418, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[BUGBEAR][3][0]; // 1
	entity->focaly = limbs[BUGBEAR][3][1]; // 0
	entity->focalz = limbs[BUGBEAR][3][2]; // 2
	entity->behavior = &actBugbearLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right arm
	entity = newEntity(1415, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[BUGBEAR][4][0]; // -.25
	entity->focaly = limbs[BUGBEAR][4][1]; // 0
	entity->focalz = limbs[BUGBEAR][4][2]; // 3
	entity->behavior = &actBugbearLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left arm
	entity = newEntity(1414, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[BUGBEAR][4][0]; // -.25
	entity->focaly = limbs[BUGBEAR][4][1]; // 0
	entity->focalz = limbs[BUGBEAR][4][2]; // 3
	entity->behavior = &actBugbearLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// world weapon
	entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->noColorChangeAllyLimb = 1.0;
	entity->focalx = limbs[BUGBEAR][6][0];
	entity->focaly = limbs[BUGBEAR][6][1];
	entity->focalz = limbs[BUGBEAR][6][2];
	entity->behavior = &actBugbearLimb;
	entity->parent = my->getUID();
	entity->pitch = .25;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// shield
	entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->noColorChangeAllyLimb = 1.0;
	entity->focalx = limbs[BUGBEAR][7][0];
	entity->focaly = limbs[BUGBEAR][7][1];
	entity->focalz = limbs[BUGBEAR][7][2];
	entity->behavior = &actBugbearLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);
}

void actBugbearLimb(Entity* my)
{
	my->actMonsterLimb();
}

void bugbearDie(Entity* my)
{
	for ( int c = 0; c < 12; c++ )
	{
		Entity* gib = spawnGib(my);
		if (c < 6) {
		    gib->sprite = 1412 + c;
		    gib->skill[5] = 1; // poof
		}
		serverSpawnGibForClient(gib);
	}

	my->spawnBlood();

	playSoundEntity(my, 676 + local_rng.rand() % 3, 128);

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define BUGBEARWALKSPEED .12
real_t getWalkSpeed(Entity& my)
{
	real_t val = BUGBEARWALKSPEED;
	if ( my.monsterState == MONSTER_STATE_GENERIC_CHARGE )
	{
		val /= 2;
	}
	return val;
}

void bugbearMoveBodyparts(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = nullptr;
	Entity* rightbody = nullptr;
	int bodypart;

	my->focalx = limbs[BUGBEAR][0][0];
	my->focaly = limbs[BUGBEAR][0][1];
	my->focalz = limbs[BUGBEAR][0][2];

	/*if ( keystatus[SDLK_LSHIFT] )
	{
		if ( keystatus[SDLK_h] )
		{
			keystatus[SDLK_h] = 0;
			myStats->EFFECTS[EFF_STUNNED] = !myStats->EFFECTS[EFF_STUNNED];
			myStats->EFFECTS_TIMERS[EFF_STUNNED] = myStats->EFFECTS[EFF_STUNNED] ? -1 : 0;
		}
		if ( keystatus[SDLK_g] )
		{
			keystatus[SDLK_g] = 0;
			int pose = MONSTER_POSE_MELEE_WINDUP1;
			my->attack(pose, 0, nullptr);
		}
		if ( keystatus[SDLK_1] )
		{
			keystatus[SDLK_1] = 0;
			int pose = MONSTER_POSE_MELEE_WINDUP1;
			pose = MONSTER_POSE_MELEE_WINDUP2;
			my->attack(pose, 0, nullptr);
		}
		if ( keystatus[SDLK_2] )
		{
			keystatus[SDLK_2] = 0;
			int pose = MONSTER_POSE_MELEE_WINDUP1;
			pose = MONSTER_POSE_MELEE_WINDUP3;
			my->attack(pose, 0, nullptr);
		}
		if ( keystatus[SDLK_3] )
		{
			keystatus[SDLK_3] = 0;
			my->attack(MONSTER_POSE_SPECIAL_WINDUP1, 0, nullptr);
		}
	}*/
	bool wearingring = false;

	// set invisibility //TODO: isInvisible()?
	if ( multiplayer != CLIENT )
	{
		if ( myStats->ring != nullptr )
			if ( myStats->ring->type == RING_INVISIBILITY )
			{
				wearingring = true;
			}
		if ( myStats->cloak != nullptr )
			if ( myStats->cloak->type == CLOAK_INVISIBILITY )
			{
				wearingring = true;
			}
		if ( myStats->EFFECTS[EFF_INVISIBLE] == true || wearingring == true )
		{
			my->flags[INVISIBLE] = true;
			my->flags[BLOCKSIGHT] = false;
			bodypart = 0;
			for ( node = my->children.first; node != nullptr; node = node->next )
			{
				if ( bodypart < LIMB_HUMANOID_TORSO )
				{
					bodypart++;
					continue;
				}
				if ( bodypart >= 7 )
				{
					break;
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
			for ( node = my->children.first; node != nullptr; node = node->next )
			{
				if ( bodypart < LIMB_HUMANOID_TORSO )
				{
					bodypart++;
					continue;
				}
				if ( bodypart >= 7 )
				{
					break;
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

		// sleeping
		if ( myStats->EFFECTS[EFF_ASLEEP] )
		{
			my->z = limbs[BUGBEAR][11][0];
		}
		else
		{
			my->z = limbs[BUGBEAR][11][2];
		}
	}

	Entity* weaponarm = nullptr;
	Entity* shieldarm = nullptr;

	//Move bodyparts
	for (bodypart = 0, node = my->children.first; node != NULL; node = node->next, bodypart++)
	{
		if ( bodypart < 2 )
		{
			continue;
		}
		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		entity->yaw = my->yaw;
		if ( bodypart == LIMB_HUMANOID_RIGHTLEG || bodypart == LIMB_HUMANOID_LEFTARM )
		{
			if ( bodypart == LIMB_HUMANOID_RIGHTLEG )
			{
				rightbody = (Entity*)node->next->element;
			}

			node_t* shieldNode = list_Node(&my->children, 8);
			Entity* shield = nullptr;
			if ( shieldNode )
			{
				shield = (Entity*)shieldNode->element;
			}

			if ( bodypart == LIMB_HUMANOID_LEFTARM
				&& (MONSTER_ATTACK == MONSTER_POSE_SPECIAL_WINDUP1
					|| MONSTER_ATTACK == MONSTER_POSE_BUGBEAR_SHIELD) )
			{
				my->monsterHitTime = 0;
				if ( MONSTER_ATTACK == MONSTER_POSE_SPECIAL_WINDUP1 )
				{
					if ( MONSTER_ATTACKTIME == 0 )
					{
						entity->pitch = 0;
						entity->skill[1] = 0;
					}

					real_t shieldSetpoint = 2 * PI / 4;
					if ( MONSTER_SHIELDYAW < shieldSetpoint )
					{
						MONSTER_SHIELDYAW += 0.1;
					}
					if ( MONSTER_SHIELDYAW >= shieldSetpoint )
					{
						MONSTER_SHIELDYAW = shieldSetpoint;
					}

					if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.1, PI / 16, true, 0.01) )
					{
						if ( MONSTER_ATTACKTIME >= 2 * ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
						{
							if ( multiplayer != CLIENT )
							{
								my->attack(MONSTER_POSE_BUGBEAR_SHIELD, 0, nullptr);
							}
						}
					}
				}
				else if ( MONSTER_ATTACK == MONSTER_POSE_BUGBEAR_SHIELD )
				{
					if ( MONSTER_ATTACKTIME == 0 )
					{
						entity->pitch = PI / 16;
						MONSTER_SHIELDYAW = PI / 4;
						entity->skill[1] = 0;
					}

					if ( MONSTER_SHIELDYAW > -PI / 8 )
					{
						MONSTER_SHIELDYAW -= 0.1;
					}
					if ( MONSTER_SHIELDYAW <= -PI / 8 )
					{
						MONSTER_SHIELDYAW = -PI / 8;
					}

					if ( entity->skill[1] == 0 )
					{
						if ( limbAnimateToLimit(entity, ANIMATE_PITCH, -0.25, 6 * PI / 4, false, 0.0) )
						{
							entity->skill[1] = 1;
						}
					}
					else if ( entity->skill[1] == 1 )
					{
						if ( MONSTER_ATTACKTIME >= 0 )
						{
							entity->skill[1] = 2;
						}
					}
					else if ( entity->skill[1] == 2 )
					{
						if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.1, 0.0, false, 0.0) )
						{
							entity->skill[1] = 0;
							MONSTER_ATTACK = 0;
						}
					}
				}
			}
			else if ( dist > 0.1 && (bodypart == LIMB_HUMANOID_RIGHTLEG || (shield && shield->sprite <= 0)) )
			{
				// swing right leg, left arm in sync.
				if ( !rightbody->skill[0] )
				{
					entity->pitch -= dist * getWalkSpeed(*my);
					if ( entity->pitch < -PI / 4.0 )
					{
						entity->pitch = -PI / 4.0;
						if ( bodypart == 3 && entity->skill[0] == 0 )
						{
							playSoundEntityLocal(my, 115, 128);
							entity->skill[0] = 1;
						}
					}
				}
				else
				{
					entity->pitch += dist * getWalkSpeed(*my);
					if ( entity->pitch > PI / 4.0 )
					{
						entity->pitch = PI / 4.0;
						if ( bodypart == 3 && entity->skill[0] == 1 )
						{
							playSoundEntityLocal(my, 115, 128);
							entity->skill[0] = 0;
						}
					}
				}
			}
			else
			{
				// if not moving, reset position of the leg/arm.
				if ( bodypart == LIMB_HUMANOID_LEFTARM )
				{
					while ( entity->pitch < -PI )
					{
						entity->pitch += 2 * PI;
					}
					while ( entity->pitch >= PI )
					{
						entity->pitch -= 2 * PI;
					}

					if ( entity->pitch < 0.0 )
					{
						entity->pitch += 1 / fmax(dist * .1, 10.0);
						if ( entity->pitch > 0.0 )
						{
							entity->pitch = 0.0;
						}
					}
					else if ( entity->pitch > 0.0 )
					{
						entity->pitch -= 1 / fmax(dist * .1, 10.0);
						if ( entity->pitch < 0.0 )
						{
							entity->pitch = 0.0;
						}
					}
				}
				else
				{
					if ( entity->pitch < 0 )
					{
						entity->pitch += 1 / fmax(dist * .1, 10.0);
						if ( entity->pitch > 0 )
						{
							entity->pitch = entity->fskill[0];
						}
					}
					else if ( entity->pitch > 0 )
					{
						entity->pitch -= 1 / fmax(dist * .1, 10.0);
						if ( entity->pitch < 0 )
						{
							entity->pitch = entity->fskill[0];
						}
					}
				}
			}
		}
		else if ( bodypart == LIMB_HUMANOID_LEFTLEG || bodypart == LIMB_HUMANOID_RIGHTARM )
		{
			if ( bodypart == LIMB_HUMANOID_RIGHTARM )
			{
				if ( my->monsterAttack > 0 )
				{
					my->handleWeaponArmAttack(entity);
				}
			}

			if ( bodypart != LIMB_HUMANOID_RIGHTARM || (my->monsterAttack == 0 ) )
			{
				// swing right arm/ left leg in sync
				if ( dist > 0.1 )
				{
					if ( entity->skill[0] )
					{
						entity->pitch -= dist * getWalkSpeed(*my);
						if ( entity->pitch < -PI / 4.0 )
						{
							entity->skill[0] = 0;
							entity->pitch = -PI / 4.0;
						}
					}
					else
					{
						entity->pitch += dist * getWalkSpeed(*my);
						if ( entity->pitch > PI / 4.0 )
						{
							entity->skill[0] = 1;
							entity->pitch = PI / 4.0;
						}
					}
				}
				else
				{
					// if not moving, reset position of the leg/arm.
					if ( entity->pitch < 0 )
					{
						entity->pitch += 1 / fmax(dist * .1, 10.0);
						if ( entity->pitch > 0 )
						{
							entity->pitch = 0;
						}
					}
					else if ( entity->pitch > 0 )
					{
						entity->pitch -= 1 / fmax(dist * .1, 10.0);
						if ( entity->pitch < 0 )
						{
							entity->pitch = 0;
						}
					}
				}
			}
		}
		switch ( bodypart )
		{
			// torso
			case 2:
				entity->focalx = limbs[BUGBEAR][1][0];
				entity->focaly = limbs[BUGBEAR][1][1];
				entity->focalz = limbs[BUGBEAR][1][2];
				entity->x -= .5 * cos(my->yaw);
				entity->y -= .5 * sin(my->yaw);
				entity->z += 2.25;
				break;
			// right leg
			case 3:
				entity->focalx = limbs[BUGBEAR][2][0];
				entity->focaly = limbs[BUGBEAR][2][1];
				entity->focalz = limbs[BUGBEAR][2][2];
				entity->x += 2 * cos(my->yaw + PI / 2) - 1.25 * cos(my->yaw);
				entity->y += 2 * sin(my->yaw + PI / 2) - 1.25 * sin(my->yaw);
				entity->z += 5;
				entity->x += limbs[BUGBEAR][9][0] * cos(my->yaw + PI / 2) + limbs[BUGBEAR][9][1] * cos(my->yaw);
				entity->y += limbs[BUGBEAR][9][0] * sin(my->yaw + PI / 2) + limbs[BUGBEAR][9][1] * sin(my->yaw);
				entity->z += limbs[BUGBEAR][9][2];
				if ( my->z >= (limbs[BUGBEAR][11][0] - 0.1) && my->z <= (limbs[BUGBEAR][11][0] + 0.1) )
				{
					entity->yaw += PI / 8;
					entity->pitch = -PI / 2;
				}
				else if ( entity->pitch <= -PI / 3 )
				{
					entity->pitch = 0;
				}
				break;
			// left leg
			case 4:
				entity->focalx = limbs[BUGBEAR][3][0];
				entity->focaly = limbs[BUGBEAR][3][1];
				entity->focalz = limbs[BUGBEAR][3][2];
				entity->x -= 2 * cos(my->yaw + PI / 2) + 1.25 * cos(my->yaw);
				entity->y -= 2 * sin(my->yaw + PI / 2) + 1.25 * sin(my->yaw);
				entity->z += 5;
				entity->x += limbs[BUGBEAR][10][0] * cos(my->yaw + PI / 2) + limbs[BUGBEAR][10][1] * cos(my->yaw);
				entity->y += limbs[BUGBEAR][10][0] * sin(my->yaw + PI / 2) + limbs[BUGBEAR][10][1] * sin(my->yaw);
				entity->z += limbs[BUGBEAR][10][2];
				if ( my->z >= (limbs[BUGBEAR][11][0] - 0.1) && my->z <= (limbs[BUGBEAR][11][0] + 0.1) )
				{
					entity->yaw -= PI / 8;
					entity->pitch = -PI / 2;
				}
				else if ( entity->pitch <= -PI / 3 )
				{
					entity->pitch = 0;
				}
				break;
			// right arm
			case 5:
			{
				weaponarm = entity;
				entity->x += 3.5 * cos(my->yaw + PI / 2) - 1 * cos(my->yaw);
				entity->y += 3.5 * sin(my->yaw + PI / 2) - 1 * sin(my->yaw);
				entity->z += .1;

				node_t* weaponNode = list_Node(&my->children, 7);
				if ( weaponNode )
				{
					Entity* weapon = (Entity*)weaponNode->element;
					if ( my->monsterState != MONSTER_STATE_ATTACK && my->monsterAttack == 0 )
					{
						if ( weapon )
						{
							if ( weapon->sprite != items[HEAVY_CROSSBOW].index )
							{
								my->monsterArmbended = 1;
							}
							else
							{
								my->monsterArmbended = 0;
							}
						}
					}
					if ( MONSTER_ARMBENDED || (weapon->flags[INVISIBLE] && my->monsterState == MONSTER_STATE_WAIT) )
					{
						// if weapon invisible and I'm not attacking, relax arm.
						entity->focalx = limbs[BUGBEAR][4][0];
						entity->focaly = limbs[BUGBEAR][4][1];
						entity->focalz = limbs[BUGBEAR][4][2];
						entity->sprite = 1415;

						entity->x += limbs[BUGBEAR][16][0] * cos(my->yaw + PI / 2) + limbs[BUGBEAR][16][1] * cos(my->yaw);
						entity->y += limbs[BUGBEAR][16][0] * sin(my->yaw + PI / 2) + limbs[BUGBEAR][16][1] * sin(my->yaw);
						entity->z += limbs[BUGBEAR][16][2];

						if ( my->monsterAttack == MONSTER_POSE_MELEE_WINDUP2 || my->monsterAttack == 2 )
						{
							entity->focaly += 0.5;
							entity->x += -1 * cos(my->yaw + PI / 2);
							entity->y += -1 * sin(my->yaw + PI / 2);
						}
					}
					else
					{
						// else flex arm.
						entity->focalx = limbs[BUGBEAR][15][0];
						entity->focaly = limbs[BUGBEAR][15][1];
						entity->focalz = limbs[BUGBEAR][15][2];
						entity->sprite = 1417;

						entity->x += limbs[BUGBEAR][12][0] * cos(my->yaw + PI / 2) + limbs[BUGBEAR][12][1] * cos(my->yaw);
						entity->y += limbs[BUGBEAR][12][0] * sin(my->yaw + PI / 2) + limbs[BUGBEAR][12][1] * sin(my->yaw);
						entity->z += limbs[BUGBEAR][12][2];
					}
				}

				entity->yaw += MONSTER_WEAPONYAW;
				if ( my->z >= (limbs[BUGBEAR][11][0] - 0.1) && my->z <= (limbs[BUGBEAR][11][0] + 0.1) )
				{
					entity->pitch = 0;
				}
				break;
			}
			// left arm
			case 6:
			{
				shieldarm = entity;
				entity->x -= 3.5 * cos(my->yaw + PI / 2) + 1 * cos(my->yaw);
				entity->y -= 3.5 * sin(my->yaw + PI / 2) + 1 * sin(my->yaw);
				entity->z += .1;
				node_t* shieldNode = list_Node(&my->children, 8);
				if ( shieldNode )
				{
					Entity* shield = (Entity*)shieldNode->element;
					if ( shield->flags[INVISIBLE] && my->monsterState == MONSTER_STATE_WAIT )
					{
						// relax arm
						entity->focalx = limbs[BUGBEAR][4][0];
						entity->focaly = limbs[BUGBEAR][4][1];
						entity->focalz = limbs[BUGBEAR][4][2];
						entity->sprite = 1414;

						entity->x += -limbs[BUGBEAR][16][0] * cos(my->yaw + PI / 2) + limbs[BUGBEAR][16][1] * cos(my->yaw);
						entity->y += -limbs[BUGBEAR][16][0] * sin(my->yaw + PI / 2) + limbs[BUGBEAR][16][1] * sin(my->yaw);
						entity->z += limbs[BUGBEAR][16][2];
					}
					else
					{
						// else flex arm
						entity->focalx = limbs[BUGBEAR][5][0];
						entity->focaly = limbs[BUGBEAR][5][1];
						entity->focalz = limbs[BUGBEAR][5][2];
						entity->sprite = 1416;
						entity->x += limbs[BUGBEAR][8][0] * cos(my->yaw + PI / 2) + limbs[BUGBEAR][8][1] * cos(my->yaw);
						entity->y += limbs[BUGBEAR][8][0] * sin(my->yaw + PI / 2) + limbs[BUGBEAR][8][1] * sin(my->yaw);
						entity->z += limbs[BUGBEAR][8][2];
					}
				}
				if ( my->z >= (limbs[BUGBEAR][11][0] - 0.1) && my->z <= (limbs[BUGBEAR][11][0] + 0.1) )
				{
					entity->pitch = 0;
				}

				if ( MONSTER_ATTACK != MONSTER_POSE_BUGBEAR_SHIELD && MONSTER_ATTACK != MONSTER_POSE_SPECIAL_WINDUP1 )
				{
					if ( my->monsterDefend && my->monsterAttack == 0 )
					{
						if ( MONSTER_SHIELDYAW < 2 * PI / 5 )
						{
							MONSTER_SHIELDYAW += 0.3;
							MONSTER_SHIELDYAW = std::min(MONSTER_SHIELDYAW, 2 * PI / 5);
						}
						else if ( MONSTER_SHIELDYAW > 2 * PI / 5 )
						{
							MONSTER_SHIELDYAW -= 0.3;
							MONSTER_SHIELDYAW = std::max(MONSTER_SHIELDYAW, 2 * PI / 5);
						}
					}
					else
					{
						if ( MONSTER_SHIELDYAW > 0.0 )
						{
							MONSTER_SHIELDYAW -= 0.3;
							MONSTER_SHIELDYAW = std::max(MONSTER_SHIELDYAW, 0.0);
						}
						else if ( MONSTER_SHIELDYAW < 0.0 )
						{
							MONSTER_SHIELDYAW += 0.3;
							MONSTER_SHIELDYAW = std::min(MONSTER_SHIELDYAW, 0.0);
						}
					}
				}
				entity->yaw += MONSTER_SHIELDYAW;
				break;
			}
			case LIMB_HUMANOID_WEAPON:
				if ( multiplayer != CLIENT )
				{
					/*if ( keystatus[SDLK_LSHIFT] )
					{
						if ( keystatus[SDLK_KP_7] )
						{
							keystatus[SDLK_KP_7] = 0;
							entity->sprite = -1;
							if ( myStats->weapon )
							{
								list_RemoveNode(myStats->weapon->node);
								myStats->weapon = nullptr;
							}
						}
						if ( keystatus[SDLK_KP_8] )
						{
							keystatus[SDLK_KP_8] = 0;
							entity->sprite = 1422;
							if ( !myStats->weapon )
							{
								myStats->weapon = newItem(STEEL_SWORD, EXCELLENT, 0, 1, 0, false, nullptr);
							}
							else
							{
								myStats->weapon->type = STEEL_SWORD;
							}
						}
						if ( keystatus[SDLK_KP_9] )
						{
							keystatus[SDLK_KP_9] = 0;
							entity->sprite = 1424;
							if ( !myStats->weapon )
							{
								myStats->weapon = newItem(STEEL_AXE, EXCELLENT, 0, 1, 0, false, nullptr);
							}
							else
							{
								myStats->weapon->type = STEEL_AXE;
							}
						}
						if ( keystatus[SDLK_KP_6] )
						{
							keystatus[SDLK_KP_6] = 0;
							entity->sprite = 984;
							if ( !myStats->weapon )
							{
								myStats->weapon = newItem(HEAVY_CROSSBOW, EXCELLENT, 0, 1, 0, false, nullptr);
							}
							else
							{
								myStats->weapon->type = HEAVY_CROSSBOW;
							}
						}
					}*/

					if ( myStats->weapon == nullptr || myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) //TODO: isInvisible()?
					{
						entity->flags[INVISIBLE] = true;
					}
					else
					{
						entity->flags[INVISIBLE] = false;
						if ( myStats->weapon->type == STEEL_AXE )
						{
							entity->sprite = 1424;
						}
						else if ( myStats->weapon->type == STEEL_SWORD )
						{
							entity->sprite = 1422;
						}
						else if ( myStats->weapon->type == HEAVY_CROSSBOW )
						{
							entity->sprite = itemModel(myStats->weapon);
						}
						else
						{
							entity->flags[INVISIBLE] = true;
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
				else
				{
					if ( entity->sprite <= 0 )
					{
						entity->flags[INVISIBLE] = true;
					}
				}
				if ( weaponarm != nullptr )
				{
					my->handleHumanoidWeaponLimb(entity, weaponarm);
				}
				entity->x += limbs[BUGBEAR][13][0] * cos(my->yaw + PI / 2) + limbs[BUGBEAR][13][1] * cos(my->yaw);
				entity->y += limbs[BUGBEAR][13][0] * sin(my->yaw + PI / 2) + limbs[BUGBEAR][13][1] * sin(my->yaw);
				entity->z += limbs[BUGBEAR][13][2];
				if ( MONSTER_ARMBENDED )
				{
					entity->focalx = limbs[BUGBEAR][17][0];
					entity->focaly = limbs[BUGBEAR][17][1];
					entity->focalz = limbs[BUGBEAR][17][2];
					if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP2 || MONSTER_ATTACK == 2 )
					{
						entity->focaly += 0.75;
					}
				}
				else
				{
					entity->focalx = limbs[BUGBEAR][6][0];
					entity->focaly = limbs[BUGBEAR][6][1];
					entity->focalz = limbs[BUGBEAR][6][2];
					if ( entity->sprite == items[HEAVY_CROSSBOW].index )
					{
						entity->focalx += 4.0;
						entity->focalz += 2;
					}
					else if ( entity->sprite == 1424 )
					{
						entity->focalz += 1;
					}
				}
				break;
			case LIMB_HUMANOID_SHIELD:
				//if ( keystatus[SDLK_LSHIFT] )
				//{
				//	if ( keystatus[SDLK_KP_1] )
				//	{
				//		keystatus[SDLK_KP_1] = 0;
				//		entity->sprite = -1;
				//		if ( myStats->shield )
				//		{
				//			list_RemoveNode(myStats->shield->node);
				//			myStats->shield = nullptr;
				//		}
				//	}
				//	if ( keystatus[SDLK_KP_3] )
				//	{
				//		keystatus[SDLK_KP_3] = 0;
				//		entity->sprite = 1420;
				//		if ( !myStats->shield )
				//		{
				//			myStats->shield = newItem(STEEL_SHIELD, EXCELLENT, 0, 1, 0, false, nullptr);
				//		}
				//		else
				//		{
				//			myStats->shield->type = STEEL_SHIELD;
				//		}
				//	}
				//}
				///*if ( keystatus[SDLK_SPACE] )
				//{
				//	keystatus[SDLK_SPACE] = 0;
				//	my->monsterDefend = my->monsterDefend == 0 ? 1 : 0;
				//}*/
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shield == nullptr )
					{
						entity->flags[INVISIBLE] = true;
						entity->sprite = 0;
					}
					else
					{
						entity->flags[INVISIBLE] = false;
						if ( myStats->shield->type == STEEL_SHIELD )
						{
							entity->sprite = 1420;
						}
						else
						{
							entity->flags[INVISIBLE] = true;
						}
					}
					if ( myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) //TODO: isInvisible()?
					{
						entity->flags[INVISIBLE] = true;
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
				else
				{
					if ( entity->sprite <= 0 )
					{
						entity->flags[INVISIBLE] = true;
					}
				}
				if ( shieldarm != nullptr )
				{
					my->handleHumanoidShieldLimb(entity, shieldarm);
				}
				break;
			default:
				break;
		}
	}
	// rotate shield a bit
	node_t* shieldNode = list_Node(&my->children, 8);
	if ( shieldNode )
	{
		Entity* shieldEntity = (Entity*)shieldNode->element;
		if ( shieldEntity->sprite != items[TOOL_TORCH].index && shieldEntity->sprite != items[TOOL_LANTERN].index && shieldEntity->sprite != items[TOOL_CRYSTALSHARD].index )
		{
			shieldEntity->yaw -= 2 * PI / 6;
		}
	}
	if ( MONSTER_ATTACK > 0 && (MONSTER_ATTACK <= MONSTER_POSE_MAGIC_CAST3 
		|| MONSTER_ATTACK == MONSTER_POSE_SPECIAL_WINDUP1
		|| MONSTER_ATTACK == MONSTER_POSE_BUGBEAR_SHIELD) )
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

void Entity::bugbearChooseWeapon(const Entity* target, double dist)
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return;
	}

	if ( monsterSpecialState == BUGBEAR_DEFENSE )
	{
		if ( monsterStrafeDirection == 0 && local_rng.rand() % 10 == 0 && ticks % 10 == 0 )
		{
			setBugbearStrafeDir(true);
			//monsterStrafeDirection = -1 + ((local_rng.rand() % 2 == 0) ? 2 : 0);
		}
	}

	if ( monsterSpecialState != 0 || monsterSpecialTimer != 0 )
	{
		if ( monsterSpecialTimer < MONSTER_SPECIAL_COOLDOWN_BUGBEAR / 2 )
		{
			monsterSpecialState = 0;
			monsterStrafeDirection = 0;
		}
		return;
	}

	if ( monsterSpecialTimer == 0
		&& (ticks % 10 == 0)
		&& (dist < STRIKERANGE * 2 || hasRangedWeapon()) )
	{
		Stat* targetStats = target->getStats();
		if ( !targetStats )
		{
			return;
		}

		int specialRoll = -1;
		int bonusFromHP = 0;
		specialRoll = local_rng.rand() % 40;
		if ( myStats->HP <= myStats->MAXHP * 0.8 )
		{
			bonusFromHP += 2; // +% chance if on low health
		}
		if ( myStats->HP <= myStats->MAXHP * 0.4 )
		{
			bonusFromHP += 3; // +extra % chance if on lower health
		}

		int requiredRoll = (2 + bonusFromHP);

		if ( dist < STRIKERANGE )
		{
			requiredRoll += 5;
		}

		if ( specialRoll < requiredRoll )
		{
			Entity* leader = nullptr;
			if ( myStats->leader_uid != 0 )
			{
				leader = uidToEntity(myStats->leader_uid);
			}
			else if ( parent != 0 )
			{
				leader = uidToEntity(parent);
			}

			if ( leader && leader->monsterSpecialState == BUGBEAR_DEFENSE )
			{
				if ( local_rng.rand() % 2 != 0 || leader->monsterSpecialTimer >= MONSTER_SPECIAL_COOLDOWN_BUGBEAR - TICKS_PER_SECOND )
				{
					return;
				}
			}

			monsterSpecialState = BUGBEAR_DEFENSE;
			monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_BUGBEAR;

			if ( leader )
			{
				if ( leader->monsterStrafeDirection != 0 )
				{
					if ( local_rng.rand() % 2 == 0 )
					{
						monsterStrafeDirection = 0;
						return;
					}
				}
			}

			setBugbearStrafeDir(true);
			//monsterStrafeDirection = -1 + ((local_rng.rand() % 2 == 0) ? 2 : 0);
		}
	}
}

void Entity::setBugbearStrafeDir(bool forceDirection)
{
	Stat* myStats = getStats();
	if ( !myStats ) { return; }
	if ( myStats->type != BUGBEAR )
	{
		return;
	}
	if ( monsterSpecialState != BUGBEAR_DEFENSE )
	{
		return;
	}
	Entity* leader = nullptr;
	if ( myStats->leader_uid != 0 )
	{
		leader = uidToEntity(myStats->leader_uid);
	}
	else if ( parent != 0 )
	{
		leader = uidToEntity(parent);
	}

	std::vector<int> dirs = { -1, 1 };
	if ( !forceDirection )
	{
		dirs.push_back(0);
	}
	std::set<int> gooddirs;
	real_t ox = x;
	real_t oy = y;

	Entity* target = monsterTarget != 0 ? uidToEntity(monsterTarget) : nullptr;

	for ( auto dir : dirs )
	{
		x = ox;
		y = oy;
		real_t tangent = yaw;
		tangent += (PI / 2) * dir;
		if ( dir != 0 )
		{
			x = this->x + 4 * cos(tangent);
			y = this->y + 4 * sin(tangent);

			bool bFlag = false;
			if ( target )
			{
				bFlag = target->flags[PASSABLE];
				target->flags[PASSABLE] = true;
			}
			Entity* ohitentity = hit.entity;
			bool clear = barony_clear(x, y, this);
			hit.entity = ohitentity;
			if ( target )
			{
				target->flags[PASSABLE] = bFlag;
			}

			if ( !clear )
			{
				// no good, blocked by things
				continue;
			}
		}

		if ( leader )
		{
			if ( entityInsideEntity(leader, this) )
			{
				// no good, blocked by ally
				continue;
			}

			if ( Stat* leaderStats = leader->getStats() )
			{
				if ( monsterTarget != 0 && leader->monsterTarget == monsterTarget )
				{
					// check LOS of leader to their target
					if ( target )
					{
						real_t tangent2 = atan2(target->y - leader->y, target->x - leader->x);
						// trace the tangent see if we would intersect it
						Entity* ohitentity = hit.entity;
						real_t dist = lineTraceTarget(leader, leader->x, leader->y, tangent2, 128.0, 0, false, this);
						bool inTheWay = hit.entity == this;
						hit.entity = ohitentity;
						if ( inTheWay )
						{
							continue;
						}
					}
				}
			}
		}
		gooddirs.insert(dir);
	}

	x = ox;
	y = oy;

	if ( gooddirs.size() > 0 )
	{
		if ( monsterStrafeDirection != 0 )
		{
			if ( gooddirs.size() >= 2 && gooddirs.find(monsterStrafeDirection) != gooddirs.end() )
			{
				// remove current direction
				gooddirs.erase(monsterStrafeDirection);
			}
		}
		auto it = gooddirs.begin();
		int pick = local_rng.rand() % gooddirs.size();
		for ( int i = 0; i < pick; ++i )
		{
			++it;
		}
		monsterStrafeDirection = *it;
	}
	else
	{
		if ( leader && leader->monsterStrafeDirection != 0 )
		{
			monsterStrafeDirection = -1 * leader->monsterStrafeDirection;
		}
		else
		{
			if ( monsterStrafeDirection != 0 )
			{
				monsterStrafeDirection = -1 * monsterStrafeDirection;
			}
			else
			{
				monsterStrafeDirection = local_rng.rand() % 2 == 0 ? -1 : 1;
			}
		}
	}

	//messagePlayer(0, MESSAGE_DEBUG, "pickdir: %d", monsterStrafeDirection);
}