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

void initCockatrice(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	my->initMonster(413);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 198;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = 201;
		MONSTER_IDLEVAR = 3;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		if ( myStats != NULL )
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
			myStats->EFFECTS[EFF_LEVITATING] = false;
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
			if ( myStats->weapon == NULL && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
			{
				myStats->weapon = newItem(SPELLBOOK_FIREBALL, EXCELLENT, 0, 1, 0, false, NULL);
			}
		}
	}

	// torso
	Entity* entity = newEntity(414, 0, map.entities);
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

	// right leg
	entity = newEntity(416, 0, map.entities);
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

	// left leg
	entity = newEntity(415, 0, map.entities);
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

	// right arm
	entity = newEntity(418, 0, map.entities);
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

	// left arm
	entity = newEntity(417, 0, map.entities);
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

	// right wing
	entity = newEntity(420, 0, map.entities);
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

	// left wing
	entity = newEntity(419, 0, map.entities);
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

	playSoundEntity(my, 28, 128);

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define COCKATRICEWALKSPEED .01

void cockatriceMoveBodyparts(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = NULL;
	Entity* rightbody = NULL;
	int bodypart;

	// set invisibility //TODO: use isInvisible()?
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
			for (node = my->children.first; node != NULL; node = node->next)
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

		// cockatrices are always flying
		//myStats->EFFECTS[EFF_LEVITATING] = true;
		//myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 0;
	}

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
		if ( bodypart == 3 || bodypart == 6 )
		{
			if ( bodypart == 3 )
			{
				rightbody = (Entity*)node->next->element;
			}
			if ( bodypart == 3 || !MONSTER_ATTACK )
			{
				if ( !rightbody->skill[0] )
				{
					entity->pitch -= COCKATRICEWALKSPEED;
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
					entity->pitch += COCKATRICEWALKSPEED;
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
				// vertical chop
				if ( MONSTER_ATTACKTIME == 0 )
				{
					MONSTER_ARMBENDED = 0;
					MONSTER_WEAPONYAW = 0;
					entity->pitch = -3 * PI / 4;
					entity->roll = 0;
				}
				else
				{
					if ( entity->pitch >= -PI / 2 )
					{
						MONSTER_ARMBENDED = 1;
					}
					if ( entity->pitch >= PI / 4 )
					{
						entity->skill[0] = rightbody->skill[0];
						MONSTER_WEAPONYAW = 0;
						entity->pitch = rightbody->pitch;
						entity->roll = 0;
						MONSTER_ARMBENDED = 0;
						MONSTER_ATTACK = 0;
					}
					else
					{
						entity->pitch += .25;
					}
				}
			}
		}
		else if ( bodypart == 4 || bodypart == 5 )
		{
			if ( bodypart == 5 )
			{
				if ( MONSTER_ATTACK )
				{
					// vertical chop
					if ( MONSTER_ATTACKTIME == 0 )
					{
						MONSTER_ARMBENDED = 0;
						MONSTER_WEAPONYAW = 0;
						entity->pitch = -3 * PI / 4;
						entity->roll = 0;
					}
					else
					{
						if ( entity->pitch >= -PI / 2 )
						{
							MONSTER_ARMBENDED = 1;
						}
						if ( entity->pitch >= PI / 4 )
						{
							entity->skill[0] = rightbody->skill[0];
							MONSTER_WEAPONYAW = 0;
							entity->pitch = rightbody->pitch;
							entity->roll = 0;
							MONSTER_ARMBENDED = 0;
						}
						else
						{
							entity->pitch += .25;
						}
					}
				}
			}

			if ( bodypart != 5 || (MONSTER_ATTACK == 0 && MONSTER_ATTACKTIME == 0) )
			{
				if ( entity->skill[0] )
				{
					entity->pitch -= COCKATRICEWALKSPEED;
					if ( entity->pitch < -PI / 8.0 )
					{
						entity->skill[0] = 0;
						entity->pitch = -PI / 8.0;
					}
				}
				else
				{
					entity->pitch += COCKATRICEWALKSPEED;
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
			entity->fskill[1] += .1;
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
	if ( MONSTER_ATTACK != 0 )
	{
		MONSTER_ATTACKTIME++;
	}
	else
	{
		MONSTER_ATTACKTIME = 0;
	}
}
