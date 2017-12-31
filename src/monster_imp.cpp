/*-------------------------------------------------------------------------------

	BARONY
	File: monster_imp.cpp
	Desc: implements all of the imp monster's code

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
#include "player.hpp"

void initImp(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	my->initMonster(289);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 198;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = 201;
		MONSTER_IDLEVAR = 3;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		if ( myStats != nullptr )
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
			myStats->EFFECTS[EFF_LEVITATING] = true;
			myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 0;

			if ( rand() % 4 == 0 )
			{
				myStats->EFFECTS[EFF_ASLEEP] = true;
				myStats->EFFECTS_TIMERS[EFF_ASLEEP] = 1800 + rand() % 3600;
			}

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
					if ( rand() % 4 == 0 )
					{
						newItem(static_cast<ItemType>(SPELLBOOK_FORCEBOLT + rand() % 21), static_cast<Status>(1 + rand() % 4), -1 + rand() % 3, 1, rand(), false, &myStats->inventory);
					}
					break;
				default:
					break;
			}

			//give weapon
			if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
			{
				myStats->weapon = newItem(SPELLBOOK_FIREBALL, EXCELLENT, 0, 1, 0, false, nullptr);
			}
		}
	}

	// torso
	Entity* entity = newEntity(290, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->focaly = 1;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[CREATURE_IMP][1][0]; // 0
	entity->focaly = limbs[CREATURE_IMP][1][1]; // 1
	entity->focalz = limbs[CREATURE_IMP][1][2]; // 0
	entity->behavior = &actImpLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right leg
	entity = newEntity(292, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[CREATURE_IMP][2][0]; // 0
	entity->focaly = limbs[CREATURE_IMP][2][1]; // 0
	entity->focalz = limbs[CREATURE_IMP][2][2]; // 2
	entity->behavior = &actImpLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left leg
	entity = newEntity(291, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[CREATURE_IMP][3][0]; // 0
	entity->focaly = limbs[CREATURE_IMP][3][1]; // 0
	entity->focalz = limbs[CREATURE_IMP][3][2]; // 2
	entity->behavior = &actImpLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right arm
	entity = newEntity(294, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[CREATURE_IMP][4][0]; // 0
	entity->focaly = limbs[CREATURE_IMP][4][1]; // 0
	entity->focalz = limbs[CREATURE_IMP][4][2]; // 3
	entity->behavior = &actImpLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left arm
	entity = newEntity(293, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[CREATURE_IMP][5][0]; // 0
	entity->focaly = limbs[CREATURE_IMP][5][1]; // 0
	entity->focalz = limbs[CREATURE_IMP][5][2]; // 3
	entity->behavior = &actImpLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right wing
	entity = newEntity(310, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[CREATURE_IMP][6][0]; // 0
	entity->focaly = limbs[CREATURE_IMP][6][1]; // 4
	entity->focalz = limbs[CREATURE_IMP][6][2]; // 0
	entity->behavior = &actImpLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left wing
	entity = newEntity(309, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[CREATURE_IMP][7][0]; // 0
	entity->focaly = limbs[CREATURE_IMP][7][1]; // -4
	entity->focalz = limbs[CREATURE_IMP][7][2]; // 0
	entity->behavior = &actImpLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);
}

void actImpLimb(Entity* my)
{
	my->actMonsterLimb();
}

void impDie(Entity* my)
{
	int c;
	for ( c = 0; c < 5; c++ )
	{
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}

	my->spawnBlood();

	playSoundEntity(my, 28, 128);

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define IMPWALKSPEED .01

void impMoveBodyparts(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = nullptr;
	Entity* rightbody = nullptr;
	int bodypart;

	// set invisibility //TODO: isInvisible()?
	if ( multiplayer != CLIENT )
	{
		if ( myStats->EFFECTS[EFF_INVISIBLE] == true )
		{
			my->flags[INVISIBLE] = true;
			my->flags[BLOCKSIGHT] = false;
			bodypart = 0;
			for (node = my->children.first; node != nullptr; node = node->next)
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
			for (node = my->children.first; node != nullptr; node = node->next)
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
			my->pitch = PI / 4;
		}
		else
		{
			my->pitch = 0;
		}

		// imps are always flying
		myStats->EFFECTS[EFF_LEVITATING] = true;
		myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 0;
	}

	//Move bodyparts
	for (bodypart = 0, node = my->children.first; node != nullptr; node = node->next, bodypart++)
	{
		if ( bodypart < LIMB_HUMANOID_TORSO )
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
			if ( bodypart == LIMB_HUMANOID_RIGHTLEG || !my->monsterAttack )
			{
				if ( !rightbody->skill[0] )
				{
					entity->pitch -= IMPWALKSPEED;
					if ( entity->pitch < -PI / 8.0 )
					{
						entity->pitch = -PI / 8.0;
						if (bodypart == 3)
						{
							entity->skill[0] = 1;
						}
					}
				}
				else
				{
					entity->pitch += IMPWALKSPEED;
					if ( entity->pitch > PI / 8.0 )
					{
						entity->pitch = PI / 8.0;
						if (bodypart == 3)
						{
							entity->skill[0] = 0;
						}
					}
				}
			}
			else
			{
				// vertical chop windup
				if ( my->monsterAttack == MONSTER_POSE_MELEE_WINDUP1 )
				{
					if ( my->monsterAttackTime == 0 )
					{
						// init rotations
						entity->pitch = 0;
						entity->roll = 0;
					}

					limbAnimateToLimit(entity, ANIMATE_PITCH, -0.25, 6 * PI / 4, false, 0);
					entity->skill[0] = 0;

					if ( my->monsterAttackTime >= ANIMATE_DURATION_WINDUP )
					{
						if ( multiplayer != CLIENT )
						{
							my->attack(1, 0, nullptr);
						}
					}
				}
				// vertical chop attack
				else if ( my->monsterAttack == 1 )
				{
					if ( my->monsterAttackTime > 0 )
					{
						if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.3, PI / 3, false, 0.0) == 1 )
						{
							entity->skill[0] = rightbody->skill[0];
							entity->pitch = rightbody->pitch;
							MONSTER_ATTACK = 0;
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
					// vertical chop
					// get leftarm from bodypart 6 element if ready to attack
					Entity* leftarm = (Entity*)node->next->element;

					if ( my->monsterAttack == 1 || my->monsterAttack == MONSTER_POSE_MELEE_WINDUP1 )
					{
						if ( leftarm != nullptr )
						{
							// follow the right arm animation.
							entity->pitch = leftarm->pitch;
							entity->roll = -leftarm->roll;
						}
					}
				}
			}

			if ( bodypart != LIMB_HUMANOID_RIGHTARM || (my->monsterAttack == 0 && my->monsterAttackTime == 0) )
			{
				if ( entity->skill[0] )
				{
					entity->pitch -= IMPWALKSPEED;
					if ( entity->pitch < -PI / 8.0 )
					{
						entity->skill[0] = 0;
						entity->pitch = -PI / 8.0;
					}
				}
				else
				{
					entity->pitch += IMPWALKSPEED;
					if ( entity->pitch > PI / 8.0 )
					{
						entity->skill[0] = 1;
						entity->pitch = PI / 8.0;
					}
				}
			}
		}
		else if ( bodypart == 7 || bodypart == 8 )
		{
			if ( my->monsterAttack == MONSTER_POSE_MELEE_WINDUP1 )
			{
				// flap wings faster during windup
				entity->fskill[1] += .4;
			}
			else
			{
				entity->fskill[1] += .1;
			}
			if ( entity->fskill[1] >= PI * 2 )
			{
				entity->fskill[1] -= PI * 2;
			}
		}
		switch ( bodypart )
		{
			// torso
			case LIMB_HUMANOID_TORSO:
				entity->x -= 2 * cos(my->yaw);
				entity->y -= 2 * sin(my->yaw);
				entity->z += 2.75;
				break;
			// right leg
			case LIMB_HUMANOID_RIGHTLEG:
				entity->x += 1 * cos(my->yaw + PI / 2);
				entity->y += 1 * sin(my->yaw + PI / 2);
				entity->z += 6;
				break;
			// left leg
			case LIMB_HUMANOID_LEFTLEG:
				entity->x -= 1 * cos(my->yaw + PI / 2);
				entity->y -= 1 * sin(my->yaw + PI / 2);
				entity->z += 6;
				break;
			// right arm
			case LIMB_HUMANOID_RIGHTARM:
				entity->x += 3 * cos(my->yaw + PI / 2) - 1 * cos(my->yaw);
				entity->y += 3 * sin(my->yaw + PI / 2) - 1 * sin(my->yaw);
				entity->z += 1;
				entity->yaw += MONSTER_WEAPONYAW;
				break;
			// left arm
			case LIMB_HUMANOID_LEFTARM:
				entity->x -= 3 * cos(my->yaw + PI / 2) + 1 * cos(my->yaw);
				entity->y -= 3 * sin(my->yaw + PI / 2) + 1 * sin(my->yaw);
				entity->z += 1;
				break;
			// right wing
			case 7:
				entity->x += 1 * cos(my->yaw + PI / 2) - 2.5 * cos(my->yaw);
				entity->y += 1 * sin(my->yaw + PI / 2) - 2.5 * sin(my->yaw);
				entity->z += 1;
				entity->yaw += cos(entity->fskill[1]) * PI / 6 + PI / 6;
				break;
			// left wing
			case 8:
				entity->x -= 1 * cos(my->yaw + PI / 2) + 2.5 * cos(my->yaw);
				entity->y -= 1 * sin(my->yaw + PI / 2) + 2.5 * sin(my->yaw);
				entity->z += 1;
				entity->yaw -= cos(entity->fskill[1]) * PI / 6 + PI / 6;
				break;
		}
	}
	if ( my->monsterAttack > 0 && my->monsterAttack <= MONSTER_POSE_MAGIC_CAST3 )
	{
		my->monsterAttackTime++;
	}
	else if ( my->monsterAttack == 0 )
	{
		my->monsterAttackTime = 0;
	}
	else
	{
		// do nothing, don't reset attacktime or increment it.
	}
}
