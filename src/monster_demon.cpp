/*-------------------------------------------------------------------------------

	BARONY
	File: monster_demon.cpp
	Desc: implements all of the demon monster's code

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

void initDemon(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	my->initMonster(258);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 210;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = 214;
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
			if ( rand() % 50 || my->flags[USERFLAG2] )
			{
			}
			else
			{
				strcpy(myStats->name, "Deu De'Breau");
				myStats->LVL = 30;
				for ( c = 0; c < 3; c++ )
				{
					Entity* entity = summonMonster(DEMON, my->x, my->y);
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
			int customItems = countCustomItems(myStats);
			//max limit of 6 custom items per entity.

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
					if ( rand() % 2 == 0 )
					{
						myStats->weapon = newItem(SPELLBOOK_FIREBALL, EXCELLENT, 0, 1, 0, false, nullptr);
					}
					break;
				default:
					break;
			}
		}
	}

	// torso
	Entity* entity = newEntity(264, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[DEMON][1][0]; // 0
	entity->focaly = limbs[DEMON][1][1]; // 0
	entity->focalz = limbs[DEMON][1][2]; // 0
	entity->behavior = &actDemonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right leg
	entity = newEntity(263, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[DEMON][2][0]; // 1
	entity->focaly = limbs[DEMON][2][1]; // 0
	entity->focalz = limbs[DEMON][2][2]; // 5
	entity->behavior = &actDemonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left leg
	entity = newEntity(262, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[DEMON][3][0]; // 1
	entity->focaly = limbs[DEMON][3][1]; // 0
	entity->focalz = limbs[DEMON][3][2]; // 5
	entity->behavior = &actDemonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right arm
	entity = newEntity(261, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[DEMON][4][0]; // -.25
	entity->focaly = limbs[DEMON][4][1]; // 0
	entity->focalz = limbs[DEMON][4][2]; // 4
	entity->behavior = &actDemonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left arm
	entity = newEntity(260, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[DEMON][5][0]; // -.25
	entity->focaly = limbs[DEMON][5][1]; // 0
	entity->focalz = limbs[DEMON][5][2]; // 4
	entity->behavior = &actDemonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// jaw
	entity = newEntity(259, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[DEMON][6][0]; // 1.5
	entity->focaly = limbs[DEMON][6][1]; // 0
	entity->focalz = limbs[DEMON][6][2]; // 1
	entity->behavior = &actDemonLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);
}

void actDemonLimb(Entity* my)
{
	my->actMonsterLimb();
}

void demonDie(Entity* my)
{
	int c;
	for ( c = 0; c < 5; c++ )
	{
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}

	my->spawnBlood();

	playSoundEntity(my, 213, 128);

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define DEMONWALKSPEED .125

void demonMoveBodyparts(Entity* my, Stat* myStats, double dist)
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
			if ( bodypart == LIMB_HUMANOID_RIGHTLEG || my->monsterAttack == 0 )
			{
				// swing right leg, left arm in sync.
				if ( dist > 0.1 )
				{
					if ( !rightbody->skill[0] )
					{
						entity->pitch -= dist * DEMONWALKSPEED;
						if ( entity->pitch < -PI / 4.0 )
						{
							entity->pitch = -PI / 4.0;
							if (bodypart == 3)
							{
								entity->skill[0] = 1;
							}
						}
					}
					else
					{
						entity->pitch += dist * DEMONWALKSPEED;
						if ( entity->pitch > PI / 4.0 )
						{
							entity->pitch = PI / 4.0;
							if (bodypart == 3)
							{
								entity->skill[0] = 0;
							}
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
			else
			{
				// vertical chop windup
				if ( my->monsterAttack == MONSTER_POSE_MELEE_WINDUP1 )
				{
					if ( my->monsterAttackTime == 0 )
					{
						// init rotations
						entity->pitch = 0;
						my->monsterArmbended = 0;
						my->monsterWeaponYaw = 0;
						entity->roll = 0;
						entity->skill[1] = 0;
					}

					limbAnimateToLimit(entity, ANIMATE_PITCH, -0.25, 5 * PI / 4, false, 0.0);

					if ( my->monsterAttackTime >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
					{
						if ( multiplayer != CLIENT )
						{
							my->attack(1, 0, nullptr);
						}
					}
				}
				// ceiling buster chop windup
				if ( my->monsterAttack == MONSTER_POSE_MELEE_WINDUP2 )
				{
					if ( my->monsterAttackTime == 0 )
					{
						// init rotations
						entity->pitch = 0;
						my->monsterArmbended = 0;
						my->monsterWeaponYaw = 0;
						entity->roll = 0;
						entity->skill[1] = 0;
					}

					limbAnimateToLimit(entity, ANIMATE_PITCH, -0.25, 5 * PI / 4, false, 0.0);

					if ( my->monsterAttackTime >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
					{
						my->monsterAttack = 1;
					}
				}
				// vertical chop attack
				else if ( my->monsterAttack == 1 )
				{
					if ( entity->pitch >= 3 * PI / 2 )
					{
						my->monsterArmbended = 1;
					}

					if ( entity->skill[1] == 0 )
					{
						// chop forwards
						if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.4, PI / 3, false, 0.0) )
						{
							entity->skill[1] = 1;
						}
					}
					else if ( entity->skill[1] == 1 )
					{
						// return to neutral
						if ( limbAnimateToLimit(entity, ANIMATE_PITCH, -0.25, 0, false, 0.0) )
						{
							entity->skill[0] = rightbody->skill[0];
							my->monsterWeaponYaw = 0;
							entity->pitch = rightbody->pitch;
							entity->roll = 0;
							my->monsterArmbended = 0;
							my->monsterAttack = 0;
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

					if ( my->monsterAttack == 1 || my->monsterAttack == MONSTER_POSE_MELEE_WINDUP1
						|| my->monsterAttack == MONSTER_POSE_MELEE_WINDUP2 )
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

			if ( bodypart != LIMB_HUMANOID_RIGHTARM || (my->monsterAttack == 0) )
			{
				// swing right arm/ left leg in sync
				if ( dist > 0.1 )
				{
					if ( entity->skill[0] )
					{
						entity->pitch -= dist * DEMONWALKSPEED;
						if ( entity->pitch < -PI / 4.0 )
						{
							entity->skill[0] = 0;
							entity->pitch = -PI / 4.0;
						}
					}
					else
					{
						entity->pitch += dist * DEMONWALKSPEED;
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
		else if ( bodypart == 7 )
		{
			// jaw
			if ( MONSTER_ATTACK == 1 )
			{
				limbAnimateToLimit(entity, ANIMATE_PITCH, 0.04, 0.16, false, 0.0);
			}
			else
			{
				entity->pitch = 0;
			}
		}
		switch ( bodypart )
		{
			// torso
			case 2:
				entity->x -= .5 * cos(my->yaw);
				entity->y -= .5 * sin(my->yaw);
				entity->z += 5;
				break;
			// right leg
			case 3:
				entity->x += 2.25 * cos(my->yaw + PI / 2) - 1.25 * cos(my->yaw);
				entity->y += 2.25 * sin(my->yaw + PI / 2) - 1.25 * sin(my->yaw);
				entity->z += 7.5;
				break;
			// left leg
			case 4:
				entity->x -= 2.25 * cos(my->yaw + PI / 2) + 1.25 * cos(my->yaw);
				entity->y -= 2.25 * sin(my->yaw + PI / 2) + 1.25 * sin(my->yaw);
				entity->z += 7.5;
				break;
			// right arm
			case 5:
				entity->x += 5 * cos(my->yaw + PI / 2) - 1 * cos(my->yaw);
				entity->y += 5 * sin(my->yaw + PI / 2) - 1 * sin(my->yaw);
				entity->z += 2.75;
				entity->yaw += MONSTER_WEAPONYAW;
				break;
			// left arm
			case 6:
				entity->x -= 5 * cos(my->yaw + PI / 2) + 1 * cos(my->yaw);
				entity->y -= 5 * sin(my->yaw + PI / 2) + 1 * sin(my->yaw);
				entity->z += 2.75;
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

void actDemonCeilingBuster(Entity* my)
{
	double x, y;

	// bust ceilings
	for ( x = my->x - my->sizex - 1; x <= my->x + my->sizex + 1; x += 1 )
	{
		for ( y = my->y - my->sizey - 1; y <= my->y + my->sizey + 1; y += 1 )
		{
			if ( x >= 0 && y >= 0 && x < map.width << 4 && y < map.height << 4 )
			{
				int index = (MAPLAYERS - 1) + ((int)floor(y / 16)) * MAPLAYERS + ((int)floor(x / 16)) * MAPLAYERS * map.height;
				if ( map.tiles[index] )
				{
					if ( my->monsterAttack == 0 )
					{
						if ( multiplayer != CLIENT )
						{
							my->attack(MONSTER_POSE_MELEE_WINDUP2, 0, nullptr);
						}
						return;
					}
					else if ( my->monsterAttack == MONSTER_POSE_MELEE_WINDUP2 )
					{
						return;
					}
					map.tiles[index] = 0;
					if ( multiplayer != CLIENT )
					{
						playSoundEntity(my, 67, 128);
						//my->monsterAttack = MONSTER_POSE_MELEE_WINDUP1;
						//my->monsterAttackTime = 0;
						Stat* myStats = my->getStats();
						if ( myStats )
						{
							// easy hack to stop the demon while he breaks stuff
							myStats->EFFECTS[EFF_PARALYZED] = true;
							myStats->EFFECTS_TIMERS[EFF_PARALYZED] = TICKS_PER_SECOND / 2;
						}
					}

					// spawn several rock particles (NOT items)
					int c, i = 6 + rand() % 4;
					for ( c = 0; c < i; c++ )
					{
						Entity* entity = nullptr;
						if ( multiplayer == SERVER )
						{
							entity = spawnGib(my);
						}
						else
						{
							entity = spawnGibClient(my->x, my->y, my->z, 5);
						}
						if ( entity )
						{
							entity->x = ((int)(my->x / 16)) * 16 + rand() % 16;
							entity->y = ((int)(my->y / 16)) * 16 + rand() % 16;
							entity->z = -8;
							entity->flags[PASSABLE] = true;
							entity->flags[INVISIBLE] = false;
							entity->flags[NOUPDATE] = true;
							entity->flags[UPDATENEEDED] = false;
							entity->sprite = items[GEM_ROCK].index;
							entity->yaw = rand() % 360 * PI / 180;
							entity->pitch = rand() % 360 * PI / 180;
							entity->roll = rand() % 360 * PI / 180;
							entity->vel_x = (rand() % 20 - 10) / 10.0;
							entity->vel_y = (rand() % 20 - 10) / 10.0;
							entity->vel_z = -.25;
							entity->fskill[3] = 0.03;
						}
					}
				}
			}
		}
	}
}
