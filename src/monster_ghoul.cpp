/*-------------------------------------------------------------------------------

	BARONY
	File: monster_ghoul.cpp
	Desc: implements all of the ghoul monster's code

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

void initGhoul(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	my->initMonster(246);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 142;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = 146;
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

			bool lesserMonster = false;
			if ( !strncmp(myStats->name, "enslaved ghoul", strlen("enslaved ghoul")) )
			{
				if ( !strncmp(map.name, "Bram's Castle", 13) )
				{
				}
				else
				{
					myStats->HP = 110;
					myStats->MAXHP = myStats->HP;
					myStats->OLDHP = myStats->HP;
					myStats->STR = 13;
					myStats->DEX = 5;
					if ( !strncmp(map.name, "The Haunted Castle", 18) )
					{
						myStats->LVL = 10;
					}
					else
					{
						myStats->LVL = 15;
					}
					myStats->PER = 10;
					if ( rand() % 2 == 0 )
					{
						myStats->EFFECTS[EFF_VAMPIRICAURA] = true;
						myStats->EFFECTS_TIMERS[EFF_VAMPIRICAURA] = -1;
					}
				}
			}


			// apply random stat increases if set in stat_shared.cpp or editor
			setRandomMonsterStats(myStats);

			// generate 6 items max, less if there are any forced items from boss variants
			int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;

			// boss variants
			if ( rand() % 50 || my->flags[USERFLAG2] )
			{
				if ( !strncmp(map.name, "Bram's Castle", 13) )
				{
					myStats->EFFECTS[EFF_VAMPIRICAURA] = true;
					myStats->EFFECTS_TIMERS[EFF_VAMPIRICAURA] = -1;
				}
			}
			else if ( !lesserMonster )
			{
				strcpy(myStats->name, "Coral Grimes");
				for ( c = 0; c < 3; c++ )
				{
					Entity* entity = summonMonster(GHOUL, my->x, my->y);
					if ( entity )
					{
						entity->parent = my->getUID();
					}
				}
				myStats->HP *= 3;
				myStats->MAXHP *= 3;
				myStats->OLDHP = myStats->HP;
				myStats->LVL = 15;
				myStats->DEX = 2;
				myStats->STR = 13;
				newItem(GEM_GARNET, EXCELLENT, 0, 1, rand(), false, &myStats->inventory);
				customItemsToGenerate -= 1;
			}

			// random effects

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate);

			// count if any custom inventory items from editor
			// max limit of 6 custom items per entity.
			int customItems = countCustomItems(myStats);

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
					if ( rand() % 20 == 0 )
					{
						newItem(POTION_WATER, SERVICABLE, 2, 1, rand(), false, &myStats->inventory);
					}
				case 2:
					if ( rand() % 10 == 0 )
					{
						newItem(itemLevelCurve(TOOL, 0, currentlevel), DECREPIT, 1, 1, rand(), false, &myStats->inventory);
					}
				case 1:
					if ( rand() % 4 == 0 )
					{
						newItem(FOOD_MEAT, DECREPIT, -1, 1, rand(), false, &myStats->inventory);
					}
					break;
				default:
					break;
			}
		}
	}

	// torso
	Entity* entity = newEntity(247, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[GHOUL][1][0]; // 0
	entity->focaly = limbs[GHOUL][1][1]; // 0
	entity->focalz = limbs[GHOUL][1][2]; // 0
	entity->behavior = &actGhoulLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right leg
	entity = newEntity(251, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[GHOUL][2][0]; // 1
	entity->focaly = limbs[GHOUL][2][1]; // 0
	entity->focalz = limbs[GHOUL][2][2]; // 2
	entity->behavior = &actGhoulLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left leg
	entity = newEntity(250, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[GHOUL][3][0]; // 1
	entity->focaly = limbs[GHOUL][3][1]; // 0
	entity->focalz = limbs[GHOUL][3][2]; // 2
	entity->behavior = &actGhoulLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right arm
	entity = newEntity(249, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[GHOUL][4][0]; // -.25
	entity->focaly = limbs[GHOUL][4][1]; // 0
	entity->focalz = limbs[GHOUL][4][2]; // 3
	entity->behavior = &actGhoulLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left arm
	entity = newEntity(248, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[GHOUL][5][0]; // -.25
	entity->focaly = limbs[GHOUL][5][1]; // 0
	entity->focalz = limbs[GHOUL][5][2]; // 3
	entity->behavior = &actGhoulLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);
}

void actGhoulLimb(Entity* my)
{
	my->actMonsterLimb();
}

void ghoulDie(Entity* my)
{
	int c;
	for ( c = 0; c < 10; c++ )
	{
		Entity* entity = spawnGib(my);
		if ( entity )
		{
			if ( c < 6 )
			{
				entity->sprite = 246 + c;
			}
			serverSpawnGibForClient(entity);
		}
	}

	my->spawnBlood(212);

	my->removeMonsterDeathNodes();

	playSoundEntity(my, 145, 128);
	list_RemoveNode(my->mynode);
	return;
}

#define GHOULWALKSPEED .125

void ghoulMoveBodyparts(Entity* my, Stat* myStats, double dist)
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
	my->x -= cos(my->yaw);
	my->y -= sin(my->yaw);
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
			if ( bodypart == LIMB_HUMANOID_LEFTARM )
			{
				if ( my->monsterAttack > 0 )
				{
					// vertical chop windup
					if ( my->monsterAttack == MONSTER_POSE_MELEE_WINDUP1 )
					{
						if ( my->monsterAttackTime == 0 )
						{
							// init rotations
							entity->pitch = 0;
							my->monsterArmbended = 0;
							//my->monsterWeaponYaw = 0; // keep the arms outstretched.
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
					// vertical chop attack
					else if ( my->monsterAttack == 1 )
					{
						my->monsterWeaponYaw = 0;
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
							my->monsterWeaponYaw = -PI / 16.0;
							// return to neutral
							if ( limbAnimateToLimit(entity, ANIMATE_PITCH, -0.25, 25 * PI / 16, false, 0.0) )
							{
								entity->skill[0] = rightbody->skill[0];
								entity->pitch = rightbody->pitch;
								entity->roll = 0;
								my->monsterArmbended = 0;
								my->monsterAttack = 0;
							}
						}
					}
				}
				else
				{
					my->monsterWeaponYaw = -PI / 16.0;
					entity->pitch = -7 * PI / 16;
					entity->roll = 0;
				}
			}
			else
			{
				if ( dist > 0.1 )
				{
					if ( !rightbody->skill[0] )
					{
						entity->pitch -= dist * GHOULWALKSPEED;
						if ( entity->pitch < -PI / 4.0 )
						{
							entity->pitch = -PI / 4.0;
						}
					}
					else
					{
						entity->pitch += dist * GHOULWALKSPEED;
						if ( entity->pitch > PI / 4.0 )
						{
							entity->pitch = PI / 4.0;
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
		else if ( bodypart == LIMB_HUMANOID_LEFTLEG || bodypart == LIMB_HUMANOID_RIGHTARM )
		{
			if ( bodypart == LIMB_HUMANOID_RIGHTARM )
			{
				if ( my->monsterAttack > 0 )
				{
					 //vertical chop
					 //get leftarm from bodypart 6 element if ready to attack
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
				else
				{
					entity->pitch = -7 * PI / 16;
					entity->roll = 0;
				}
			}
			else
			{
				if ( dist > 0.1 )
				{
					if ( entity->skill[0] )
					{
						entity->pitch -= dist * GHOULWALKSPEED * .5;
						if ( entity->pitch < -PI / 8.0 )
						{
							entity->skill[0] = 0;
							entity->pitch = -PI / 8.0;
						}
					}
					else
					{
						entity->pitch += dist * GHOULWALKSPEED * .5;
						if ( entity->pitch > PI / 8.0 )
						{
							entity->skill[0] = 1;
							entity->pitch = PI / 8.0;
						}
					}
				}
				else
				{
					if ( entity->pitch < 0 )
					{
						entity->pitch += (1 / fmax(dist * .1, 10.0)) * .5;
						if ( entity->pitch > 0 )
						{
							entity->pitch = 0;
						}
					}
					else if ( entity->pitch > 0 )
					{
						entity->pitch -= (1 / fmax(dist * .1, 10.0)) * .5;
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
			case LIMB_HUMANOID_TORSO:
				entity->x += .5 * cos(my->yaw);
				entity->y += .5 * sin(my->yaw);
				entity->z += 1.5;
				entity->pitch = PI / 16;
				break;
			// right leg
			case LIMB_HUMANOID_RIGHTLEG:
				entity->x -= .5 * cos(my->yaw) - 1 * cos(my->yaw + PI / 2);
				entity->y -= .5 * sin(my->yaw) - 1 * sin(my->yaw + PI / 2);
				entity->z += 4;
				entity->yaw += PI / 16;
				break;
			// left leg
			case LIMB_HUMANOID_LEFTLEG:
				entity->x -= .5 * cos(my->yaw) + 1 * cos(my->yaw + PI / 2);
				entity->y -= .5 * sin(my->yaw) + 1 * sin(my->yaw + PI / 2);
				entity->z += 4;
				entity->yaw -= PI / 4;
				entity->roll = PI / 8;
				break;
			// right arm
			case LIMB_HUMANOID_RIGHTARM:
				entity->x += 1 * cos(my->yaw) + 2 * cos(my->yaw + PI / 2);
				entity->y += 1 * sin(my->yaw) + 2 * sin(my->yaw + PI / 2);
				entity->z -= 1;
				entity->yaw -= my->monsterWeaponYaw;
				break;
			// left arm
			case LIMB_HUMANOID_LEFTARM:
				entity->x += 1 * cos(my->yaw) - 2 * cos(my->yaw + PI / 2);
				entity->y += 1 * sin(my->yaw) - 2 * sin(my->yaw + PI / 2);
				entity->z -= 1;
				entity->yaw += my->monsterWeaponYaw;
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
	my->x += cos(my->yaw);
	my->y += sin(my->yaw);
}
