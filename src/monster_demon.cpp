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

	my->sprite = 258;

	//my->flags[GENIUS]=true;
	my->flags[UPDATENEEDED] = true;
	my->flags[BLOCKSIGHT] = true;
	my->flags[INVISIBLE] = false;

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 210;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = 214;
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
						myStats->weapon = newItem(SPELLBOOK_FIREBALL, EXCELLENT, 0, 1, 0, false, NULL);
					}
					break;
				default:
					break;
			}
		}
	}

	// torso
	Entity* entity = newEntity(264, 0, map.entities);
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

	// right leg
	entity = newEntity(263, 0, map.entities);
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

	// left leg
	entity = newEntity(262, 0, map.entities);
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

	// right arm
	entity = newEntity(261, 0, map.entities);
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

	// left arm
	entity = newEntity(260, 0, map.entities);
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

	// jaw
	entity = newEntity(259, 0, map.entities);
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
}

void actDemonLimb(Entity* my)
{
	int i;

	Entity* parent = NULL;
	if ( (parent = uidToEntity(my->skill[2])) == NULL )
	{
		list_RemoveNode(my->mynode);
		return;
	}

	if ( multiplayer != CLIENT )
	{
		for ( i = 0; i < MAXPLAYERS; i++ )
		{
			if ( inrange[i] )
			{
				if ( i == 0 && selectedEntity == my )
				{
					parent->skill[13] = i + 1;
				}
				else if ( client_selected[i] == my )
				{
					parent->skill[13] = i + 1;
				}
			}
		}
	}
	return;
}

void demonDie(Entity* my)
{
	node_t* node, *nextnode;

	int c;
	for ( c = 0; c < 5; c++ )
	{
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
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
				Entity* entity = newEntity(160, 1, map.entities);
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
	playSoundEntity(my, 213, 128);
	int i = 0;
	for (node = my->children.first; node != NULL; node = nextnode)
	{
		nextnode = node->next;
		if (node->element != NULL && i >= 2)
		{
			Entity* entity = (Entity*)node->element;
			entity->flags[UPDATENEEDED] = false;
			list_RemoveNode(entity->mynode);
		}
		list_RemoveNode(node);
		++i;
	}
	list_RemoveNode(my->mynode);
	return;
}

#define DEMONWALKSPEED .125

void demonMoveBodyparts(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = NULL;
	Entity* rightbody = NULL;
	int bodypart;

	// set invisibility
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
			if ( MONSTER_ATTACK )
			{
				entity->pitch += 0.04;
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
	if ( MONSTER_ATTACK != 0 )
	{
		MONSTER_ATTACKTIME++;
	}
	else
	{
		MONSTER_ATTACKTIME = 0;
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
					map.tiles[index] = 0;
					if ( multiplayer != CLIENT )
					{
						playSoundEntity(my, 67, 128);
						MONSTER_ATTACK = 1;
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
						Entity* entity = spawnGib(my);
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