/*-------------------------------------------------------------------------------

	BARONY
	File: monster_cockatrice.cpp
	Desc: implements all of the cockatrice monster's code

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
#include "magic/magic.hpp"

void initCockatrice(Entity* my, Stat* myStats)
{
	node_t* node;

	my->initMonster(413);

	if ( multiplayer != CLIENT )
	{
		/*MONSTER_SPOTSND = 79;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 1;*/
		MONSTER_SPOTSND = 385;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = 382;
		MONSTER_IDLEVAR = 2;
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

			// cockatrices don't sleep!
			/*if ( rand() % 4 == 0 )
			{
				myStats->EFFECTS[EFF_ASLEEP] = true;
				myStats->EFFECTS_TIMERS[EFF_ASLEEP] = 1800 + rand() % 3600;
			}*/

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

														 // count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			// always give special spell to cockatrice, undroppable.
			newItem(SPELLBOOK_STONEBLOOD, DECREPIT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, &myStats->inventory);
			// variables for potion drops below.
			int minValue = 70;
			int maxValue = 80;
			int numRolls = 1; // 0-2 extra rolls
			if ( rand() % 2 == 0 ) // 50% chance
			{
				++numRolls;
				if ( rand() % 2 == 0 ) // 25% chance, including the previous roll
				{
					++numRolls;
				}
			}

			// generate the default inventory items for the monster, provided the editor sprite allowed enough default slots
			switch ( defaultItems )
			{
				case 6:
				case 5:
					// TODO: cockatrice head.
				case 4:
					if ( rand() % 20 == 0 ) // 5% drop stoneblood spellbook
					{
						newItem(static_cast<ItemType>(SPELLBOOK_STONEBLOOD), static_cast<Status>(1 + rand() % 4), -1 + rand() % 3, 1, rand(), false, &myStats->inventory);
					}
				case 3:
					if ( rand() % 5 == 0 ) // 20% for gemstone, luckstone to obsidian. qty 1-2.
					{
						newItem(static_cast<ItemType>(GEM_LUCK + rand() % 16), static_cast<Status>(EXCELLENT), 0, 1 + rand() % 2, rand(), false, &myStats->inventory);
					}
				case 2:
					if ( rand() % 10 < 3 ) // 30% drop stoneblood magicstaff
					{
						newItem(static_cast<ItemType>(MAGICSTAFF_STONEBLOOD), static_cast<Status>(1 + rand() % 4), -1 + rand() % 3, 1, rand(), false, &myStats->inventory);
					}
				case 1:
					for ( int i = 0; i < numRolls; ++i )
					{
						if ( rand() % 3 == 0 ) // 33% chance to choose high value item
						{
							minValue = 100;
							maxValue = 100;
						}
						ItemType itemType = itemTypeWithinGoldValue(Category::POTION, minValue, maxValue);
						newItem(itemType, static_cast<Status>(1 + rand() % 4), -1 + rand() % 3, 1, rand(), false, &myStats->inventory);
						// reset values for next loop.
						minValue = 70;
						maxValue = 80;
					}
					break;
				default:
					break;
			}

			//give weapon
			if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
			{
				//TODO: normal spell?
			}
		}
	}

	// torso
	Entity* entity = newEntity(414, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->focaly = 1;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[COCKATRICE][1][0]; // 0
	entity->focaly = limbs[COCKATRICE][1][1]; // 1
	entity->focalz = limbs[COCKATRICE][1][2]; // 0
	entity->behavior = &actCockatriceLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right leg
	entity = newEntity(416, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[COCKATRICE][2][0]; // 0
	entity->focaly = limbs[COCKATRICE][2][1]; // 0
	entity->focalz = limbs[COCKATRICE][2][2]; // 2
	entity->behavior = &actCockatriceLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left leg
	entity = newEntity(415, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[COCKATRICE][3][0]; // 0
	entity->focaly = limbs[COCKATRICE][3][1]; // 0
	entity->focalz = limbs[COCKATRICE][3][2]; // 2
	entity->behavior = &actCockatriceLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right arm
	entity = newEntity(418, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[COCKATRICE][4][0]; // 0
	entity->focaly = limbs[COCKATRICE][4][1]; // 0
	entity->focalz = limbs[COCKATRICE][4][2]; // 3
	entity->behavior = &actCockatriceLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left arm
	entity = newEntity(417, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[COCKATRICE][5][0]; // 0
	entity->focaly = limbs[COCKATRICE][5][1]; // 0
	entity->focalz = limbs[COCKATRICE][5][2]; // 3
	entity->behavior = &actCockatriceLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right wing
	entity = newEntity(420, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[COCKATRICE][6][0]; // 0
	entity->focaly = limbs[COCKATRICE][6][1]; // 4
	entity->focalz = limbs[COCKATRICE][6][2]; // 0
	entity->behavior = &actCockatriceLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left wing
	entity = newEntity(419, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[COCKATRICE][7][0]; // 0
	entity->focaly = limbs[COCKATRICE][7][1]; // -4
	entity->focalz = limbs[COCKATRICE][7][2]; // 0
	entity->behavior = &actCockatriceLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);
}

void actCockatriceLimb(Entity* my)
{
	my->actMonsterLimb();
}

void cockatriceDie(Entity* my)
{
	int c;
	for ( c = 0; c < 5; c++ )
	{
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}

	my->spawnBlood();

	//playSoundEntity(my, 28, 128);
	playSoundEntity(my, 388 + rand() % 2, 128);

	my->removeMonsterDeathNodes();

	node_t* node;
	Entity* entity = nullptr;
	if ( multiplayer != CLIENT && !strncmp(map.name, "Cockatrice Lair", 15) )
	{
		for ( node = map.entities->first; node != nullptr; )
		{
			entity = (Entity*)node->element;
			node = node->next;
			if ( entity )
			{
				if ( entity->behavior == &actMagicTrap )
				{
					list_RemoveNode(entity->mynode);
				}
				else if ( entity->behavior == &actPortal )
				{
					entity->flags[INVISIBLE] = false;
					serverUpdateEntityFlag(entity, INVISIBLE);
				}
			}
		}
	}

	list_RemoveNode(my->mynode);
	return;
}

#define COCKATRICEWALKSPEED .01

void cockatriceMoveBodyparts(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = nullptr;
	Entity* leftbody = nullptr;
	Entity* leftarm = nullptr;
	Entity* rightarm = nullptr;
	int bodypart = 0;
	int limbSpeedMultiplier = 1;

	// set invisibility //TODO: use isInvisible()?
	if ( multiplayer != CLIENT )
	{
		if ( myStats->EFFECTS[EFF_INVISIBLE] == true )
		{
			my->flags[INVISIBLE] = true;
			my->flags[BLOCKSIGHT] = false;
			bodypart = 0;
			for (node = my->children.first; node != nullptr; node = node->next)
			{
				if ( bodypart < 2 )
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
				if ( bodypart < 2 )
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
			if ( MONSTER_ATTACK != MONSTER_POSE_MAGIC_WINDUP2 && MONSTER_ATTACK != MONSTER_POSE_MELEE_WINDUP3 && my->monsterAnimationLimbOvershoot == ANIMATE_OVERSHOOT_NONE )
			{
				my->pitch = 0; // dont adjust head when attacking
			}
		}

		// cockatrices are always flying
		myStats->EFFECTS[EFF_LEVITATING] = true;
		myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 0;
	}

	//Move bodyparts
	for (bodypart = 0, node = my->children.first; node != nullptr; node = node->next, bodypart++)
	{
		if ( bodypart < 2 )
		{
			if ( bodypart == 1 && multiplayer != CLIENT ) // only trigger once per loop, skip bodypart 0
			{
				if ( MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP2 )
				{
					if ( my->monsterAnimationLimbOvershoot >= ANIMATE_OVERSHOOT_TO_SETPOINT )
					{
						// handle z movement on windup
						limbAnimateWithOvershoot(my, ANIMATE_Z, 0.2, -3.5, 0.05, -5.5, ANIMATE_DIR_POSITIVE); // default z is -4.5 in actmonster.cpp
					}
				}
				else if(MONSTER_ATTACK != MONSTER_POSE_MELEE_WINDUP3 )
				{
					// post-swing head animation. client doesn't need to adjust the entity pitch, server will handle.
					limbAnimateWithOvershoot(my, ANIMATE_PITCH, 0.2, PI / 4, 0.1, 0, ANIMATE_DIR_POSITIVE);
					limbAnimateToLimit(my, ANIMATE_Z, 0.2, -4.5, false, 0);
				}
			}
			
			continue;
		}
		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		entity->yaw = my->yaw;
		if ( bodypart == 3 || bodypart == 6 )
		{
			// right leg, left arm.
			if ( bodypart == 3 )
			{
				// set leftbody to the left leg.
				leftbody = (Entity*)node->next->element;
			}
			if ( bodypart == 3 || !MONSTER_ATTACK )
			{
				if ( (MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP2 || MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP3) && bodypart == 3 )
				{
					limbSpeedMultiplier = 4;
				}
				else
				{
					limbSpeedMultiplier = 1;
				}

				if ( !leftbody->skill[0] )
				{
					entity->pitch -= COCKATRICEWALKSPEED * limbSpeedMultiplier;
					if ( entity->pitch < -PI / 8.0 )
					{
						entity->pitch = -PI / 8.0;
						if ( bodypart == 3 )
						{
							entity->skill[0] = 1;
						}
					}
				}
				else
				{
					entity->pitch += COCKATRICEWALKSPEED * limbSpeedMultiplier;
					if ( entity->pitch > PI / 8.0 )
					{
						entity->pitch = PI / 8.0;
						if ( bodypart == 3 )
						{
							entity->skill[0] = 0;
						}
					}
				}
			}
			else
			{
				rightarm = (Entity*)node->prev->element;
				// ATTACK!
				// move left arm

				// vertical chop windup
				if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP1 )
				{
					if ( MONSTER_ATTACKTIME == 0 )
					{
						// init rotations
						entity->pitch = 0;
						entity->roll = 0;
					}

					limbAnimateToLimit(entity, ANIMATE_PITCH, -0.25, 5 * PI / 4, false, 0);
					entity->skill[0] = 0;

					if ( MONSTER_ATTACKTIME >= ANIMATE_DURATION_WINDUP )
					{
						if ( multiplayer != CLIENT )
						{
							my->attack(1, 0, nullptr);
						}
					}
				}
				// vertical chop attack
				else if ( MONSTER_ATTACK == 1 )
				{
					if ( MONSTER_ATTACKTIME > 0 )
					{
						if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.3, PI / 3, false, 0.0) == 1 )
						{
							entity->skill[0] = leftbody->skill[0];
							entity->pitch = leftbody->pitch;
							MONSTER_ATTACK = 0;
						}
					}
				}
				// horizontal chop windup
				if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP2 )
				{
					if ( MONSTER_ATTACKTIME == 0 )
					{
						// init rotations
						entity->pitch = 0;
						entity->roll = 0;
					}

					// get rightarm from bodypart 5 element if ready to attack
					// vertical chop
					if ( rightarm->pitch > PI && rightarm->pitch < 7 * PI / 4 )
					{
						limbAnimateToLimit(entity, ANIMATE_PITCH, -0.25, 6 * PI / 4, false, 0);
						entity->skill[0] = 0;
					}
				}
				// horizontal chop attack
				else if ( MONSTER_ATTACK == 2 )
				{
					//limbAnimateToLimit(entity, ANIMATE_PITCH, 0.3, 0, false, 0.0);
					if ( MONSTER_ATTACKTIME > 0 )
					{
						if ( entity->skill[0] == 0 )
						{
							if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.25, PI / 3, false, 0) )
							{
								entity->skill[0] = 1;
							}
						}
						else if ( entity->skill[0] == 1 )
						{
							entity->pitch = rightarm->pitch;
						}
					}
				}
				// double attack windup
				else if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP3 )
				{
					if ( MONSTER_ATTACKTIME == 0 )
					{
						// init rotations
						entity->pitch = 0;
						entity->roll = 0;
						playSoundEntityLocal(my, 383, 128);
						createParticleDot(my);
						if ( multiplayer != CLIENT )
						{
							// cockatrice can't be paralyzed, use EFF_STUNNED instead.
							myStats->EFFECTS[EFF_STUNNED] = true;
							myStats->EFFECTS_TIMERS[EFF_STUNNED] = 20;
						}
						entity->skill[0] = 0;
					}
					else if ( MONSTER_ATTACKTIME > 20 )
					{
						entity->skill[0] = 2;
					}

					switch ( entity->skill[0] )
					{
						case 0:
							// swing forwards.
							if ( limbAnimateToLimit(entity, ANIMATE_PITCH, -0.25, 7 * PI / 4, false, 0) == 1 )
							{
								entity->skill[0] = 1;
							}
							break;
						case 1:
							// move backwards, shake at endpoint
							// move the head.
							limbAnimateToLimit(my, ANIMATE_PITCH, -0.1, 11 * PI / 6, true, 0.05);
							limbAnimateToLimit(entity, ANIMATE_PITCH, 0.1, PI / 8, true, 0.05);
							break;
						case 2:
							// start attack swing
							if ( limbAnimateToLimit(entity, ANIMATE_PITCH, -0.4, 5 * PI / 4, false, 0) == 1 )
							{
								//playSoundEntityLocal(my, 79, 128);
								if ( multiplayer != CLIENT )
								{
									my->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_SETPOINT;
									my->attack(MONSTER_POSE_COCKATRICE_DOUBLEATTACK, 0, nullptr);
								}
							}
							break;
						default:
							break;
					}
				}
				// double attack
				else if ( MONSTER_ATTACK == MONSTER_POSE_COCKATRICE_DOUBLEATTACK )
				{
					switch ( entity->skill[0] )
					{
						case 2:
							// swing down
							entity->roll = 31 * PI / 16;
							if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.5, PI / 3, false, 0) == 1 )
							{
								entity->skill[0] = 3;
							}
							break;
						case 3:
							// raise arms up again
							entity->roll = 0;
							if ( limbAnimateToLimit(entity, ANIMATE_PITCH, -0.4, 5 * PI / 4, false, 0) == 1 )
							{
								entity->roll = 31 * PI / 16;
								if ( multiplayer != CLIENT )
								{
									my->attack(3, 0, nullptr);
								}
							}
							break;
						default:
							break;
					}
					MONSTER_ATTACKTIME++; // manually increment counter
				}
				else if ( MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP2 )
				{
					if ( MONSTER_ATTACKTIME == 0 )
					{
						// init rotations
						entity->pitch = 0;
						entity->roll = 0;
						// set overshoot for z axis animation
						playSoundEntityLocal(my, 383, 128);
						createParticleDot(my);
						if ( multiplayer != CLIENT )
						{
							my->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_SETPOINT;
							// cockatrice can't be paralyzed, use EFF_STUNNED instead.
							myStats->EFFECTS[EFF_STUNNED] = true;
							myStats->EFFECTS_TIMERS[EFF_STUNNED] = 50;
						}
					}

					// only do the following during 2nd + end stage of overshoot animation.
					if ( my->monsterAnimationLimbOvershoot != ANIMATE_OVERSHOOT_TO_SETPOINT )
					{
						// move the head.
						limbAnimateToLimit(my, ANIMATE_PITCH, -0.1, 11 * PI / 6, true, 0.05);

						// raise left arm and tilt.
						limbAnimateToLimit(entity, ANIMATE_ROLL, -0.1, 31 * PI / 16, false, 0);
						limbAnimateToLimit(entity, ANIMATE_PITCH, -0.1, 5 * PI / 4, false, 0);
					}

					if ( MONSTER_ATTACKTIME > 50 )
					{
						// reset roll
						entity->roll = 0;

						if ( multiplayer != CLIENT )
						{
							// set overshoot for head animation
							my->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_SETPOINT;
							my->attack(3, 0, nullptr);
						}
					}
				}
				// default swing
				else if ( MONSTER_ATTACK == 3 )
				{
					if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.3, PI / 3, false, 0.0) == 1 )
					{
						entity->skill[0] = leftbody->skill[0];
						entity->pitch = leftbody->pitch;
						MONSTER_ATTACK = 0;
					}
				}
			}
		}
		else if ( bodypart == 4 || bodypart == 5 )
		{
			// right arm
			if ( bodypart == 5 )
			{
				if ( MONSTER_ATTACK > 0 )
				{
					// get leftarm from bodypart 6 element if ready to attack
					leftarm = (Entity*)node->next->element;
					// vertical chop
					if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP1 )
					{
						if ( MONSTER_ATTACKTIME == 0 )
						{
							// init rotations.
							entity->pitch = 0;
							entity->roll = 0;
						}
						else if ( leftarm->pitch > PI && leftarm->pitch < 7 * PI / 4 )
						{
							limbAnimateToLimit(entity, ANIMATE_PITCH, -0.25, 5 * PI / 4, false, 0);
							entity->skill[0] = 0;
						}
					}
					// vertical chop attack
					else if ( MONSTER_ATTACK == 1 )
					{
						if ( MONSTER_ATTACKTIME > 0 )
						{
							if ( entity->skill[0] == 0 )
							{
								if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.3, PI / 3, false, 0) )
								{
									entity->skill[0] = 1;
								}
							}
							else if ( entity->skill[0] == 1 )
							{
								entity->pitch = leftarm->pitch;
							}
						}
					}

					else if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP2 )
					{
						if ( MONSTER_ATTACKTIME == 0 )
						{
							// init rotations
							entity->pitch = 0;
							entity->roll = 0;
						}
						limbAnimateToLimit(entity, ANIMATE_PITCH, -0.25, 6 * PI / 4, false, 0);
						limbAnimateToLimit(entity, ANIMATE_ROLL, -0.1, 31 * PI / 16, false, 0);
						entity->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_SETPOINT;

						if ( MONSTER_ATTACKTIME >= ANIMATE_DURATION_WINDUP )
						{
							if ( multiplayer != CLIENT )
							{
								my->attack(2, 0, nullptr);
							}
						}
					}
					else if ( MONSTER_ATTACK == 2 )
					{
						if ( MONSTER_ATTACKTIME > 0 )
						{
							limbAnimateToLimit(entity, ANIMATE_PITCH, 0.3, 0, false, 0.0);
							if ( limbAnimateWithOvershoot(entity, ANIMATE_ROLL, 0.1, PI / 8, 0.05, 0, ANIMATE_DIR_POSITIVE) == ANIMATE_OVERSHOOT_TO_ENDPOINT )
							{
								entity->skill[0] = leftbody->skill[0];
								entity->pitch = leftbody->pitch;
								entity->roll = 0;
								MONSTER_ATTACK = 0;
							}
						}
					}
					else
					{
						if ( leftarm != nullptr )
						{
							// follow the left arm animation.
							entity->pitch = leftarm->pitch;
							entity->roll = -leftarm->roll;
						}
					}
				}
				else
				{
					entity->skill[0] = leftbody->skill[0];
					entity->pitch = leftbody->pitch;
					entity->roll = 0;
				}
			}
			// swing right arm/ left leg in sync
			if ( bodypart != 5 || (MONSTER_ATTACK == 0 && MONSTER_ATTACKTIME == 0) )
			{
				if ( entity->skill[0] )
				{
					entity->pitch -= COCKATRICEWALKSPEED  * limbSpeedMultiplier;
					if ( entity->pitch < -PI / 8.0 )
					{
						entity->skill[0] = 0;
						entity->pitch = -PI / 8.0;
					}
				}
				else
				{
					entity->pitch += COCKATRICEWALKSPEED  * limbSpeedMultiplier;
					if ( entity->pitch > PI / 8.0 )
					{
						entity->skill[0] = 1;
						entity->pitch = PI / 8.0;
					}
				}
			}
		}
		// wings
		else if ( bodypart == 7 || bodypart == 8 )
		{
			if ( MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP2 || MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP3 )
			{
				// flap wings faster during attack.
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
			case 2:
				entity->x -= 2 * cos(my->yaw);
				entity->y -= 2 * sin(my->yaw);
				entity->z += 2.75;
				break;
			// right leg
			case 3:
				entity->x += 1 * cos(my->yaw + PI / 2);
				entity->y += 1 * sin(my->yaw + PI / 2);
				entity->z += 6;
				break;
			// left leg
			case 4:
				entity->x -= 1 * cos(my->yaw + PI / 2);
				entity->y -= 1 * sin(my->yaw + PI / 2);
				entity->z += 6;
				break;
			// right arm
			case 5:
				entity->x += 3 * cos(my->yaw + PI / 2) - 1 * cos(my->yaw);
				entity->y += 3 * sin(my->yaw + PI / 2) - 1 * sin(my->yaw);
				entity->z += 1;
				entity->yaw += MONSTER_WEAPONYAW;
				break;
			// left arm
			case 6:
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
