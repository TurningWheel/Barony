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
			if ( rand() % 50 || my->flags[USERFLAG2] )
			{
			}
			else
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
						newItem(itemCurve(TOOL), DECREPIT, 1, 1, rand(), false, &myStats->inventory);
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
	Entity* entity = newEntity(247, 0, map.entities);
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

	// right leg
	entity = newEntity(251, 0, map.entities);
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

	// left leg
	entity = newEntity(250, 0, map.entities);
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

	// right arm
	entity = newEntity(249, 0, map.entities);
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

	// left arm
	entity = newEntity(248, 0, map.entities);
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
	if (spawn_blood)
	{
		int x, y;
		x = std::min<unsigned int>(std::max<int>(0, my->x / 16), map.width - 1);
		y = std::min<unsigned int>(std::max<int>(0, my->y / 16), map.height - 1);
		if ( map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height] )
		{
			if ( !checkObstacle(my->x, my->y, my, NULL) )
			{
				Entity* entity = newEntity(212, 1, map.entities);
				entity->x = my->x;
				entity->y = my->y;
				entity->z = 7.4 + (rand() % 20) / 100.f;
				entity->parent = my->getUID();
				entity->sizex = 2;
				entity->sizey = 2;
				entity->yaw = (rand() % 360) * PI / 180.0;
				entity->flags[UPDATENEEDED] = true;
				entity->flags[PASSABLE] = true;
			}
		}
	}

	my->removeMonsterDeathNodes();

	playSoundEntity(my, 145, 128);
	list_RemoveNode(my->mynode);
	return;
}

#define GHOULWALKSPEED .125

void ghoulMoveBodyparts(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = NULL;
	Entity* rightbody = NULL;
	int bodypart;

	// set invisibility //TODO: isInvisible()?
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
	}

	//Move bodyparts
	my->x -= cos(my->yaw);
	my->y -= sin(my->yaw);
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
			if ( bodypart == 6 )
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
							entity->skill[0] = 0;
							MONSTER_ARMBENDED = 0;
							MONSTER_ATTACK = 0;
						}
						else
						{
							entity->pitch += .25;
						}
					}
				}
				else
				{
					MONSTER_WEAPONYAW = -PI / 16.0;
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
							entity->skill[0] = 0;
						}
						else
						{
							entity->pitch += .25;
						}
					}
				}
				else
				{
					MONSTER_WEAPONYAW = -PI / 16.0;
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
			case 2:
				entity->x += .5 * cos(my->yaw);
				entity->y += .5 * sin(my->yaw);
				entity->z += 1.5;
				entity->pitch = PI / 16;
				break;
			// right leg
			case 3:
				entity->x -= .5 * cos(my->yaw) - 1 * cos(my->yaw + PI / 2);
				entity->y -= .5 * sin(my->yaw) - 1 * sin(my->yaw + PI / 2);
				entity->z += 4;
				entity->yaw += PI / 16;
				break;
			// left leg
			case 4:
				entity->x -= .5 * cos(my->yaw) + 1 * cos(my->yaw + PI / 2);
				entity->y -= .5 * sin(my->yaw) + 1 * sin(my->yaw + PI / 2);
				entity->z += 4;
				entity->yaw -= PI / 4;
				entity->roll = PI / 8;
				break;
			// right arm
			case 5:
				entity->x += 1 * cos(my->yaw) + 2 * cos(my->yaw + PI / 2);
				entity->y += 1 * sin(my->yaw) + 2 * sin(my->yaw + PI / 2);
				entity->z -= 1;
				entity->yaw -= MONSTER_WEAPONYAW;
				break;
			// left arm
			case 6:
				entity->x += 1 * cos(my->yaw) - 2 * cos(my->yaw + PI / 2);
				entity->y += 1 * sin(my->yaw) - 2 * sin(my->yaw + PI / 2);
				entity->z -= 1;
				entity->yaw += MONSTER_WEAPONYAW;
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
	my->x += cos(my->yaw);
	my->y += sin(my->yaw);
}
