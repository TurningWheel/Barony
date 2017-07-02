/*-------------------------------------------------------------------------------

	BARONY
	File: monster_crystalgolem.cpp
	Desc: implements all of the crystal golem monster's code

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

void initCrystalgolem(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	my->sprite = 475;

	my->flags[UPDATENEEDED] = true;
	my->flags[BLOCKSIGHT] = true;
	my->flags[INVISIBLE] = false;

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 79;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 1;
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
				strcpy(myStats->name, "Thumpus the Troll");
				for ( c = 0; c < 3; c++ )
				{
					Entity* entity = summonMonster(GNOME, my->x, my->y);
					if ( entity )
					{
						entity->parent = my->getUID();
					}
				}
				myStats->HP *= 2;
				myStats->MAXHP *= 2;
				myStats->OLDHP = myStats->HP;
				myStats->GOLD += 300;
				myStats->LVL += 10;
			}

			// random effects
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
					if ( rand() % 3 == 0 )
					{
						int i = 1 + rand() % 3;
						for ( c = 0; c < i; c++ )
						{
							newItem(static_cast<ItemType>(rand() % (NUMITEMS - 6)), static_cast<Status>(1 + rand() % 4), -1 + rand() % 3, 1, rand(), false, &myStats->inventory);
						}
					}
					break;
				default:
					break;
			}
		}
	}

	// torso
	Entity* entity = newEntity(476, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[CRYSTALGOLEM][1][0]; // 0
	entity->focaly = limbs[CRYSTALGOLEM][1][1]; // 0
	entity->focalz = limbs[CRYSTALGOLEM][1][2]; // 0
	entity->behavior = &actCrystalgolemLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// right leg
	entity = newEntity(480, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[CRYSTALGOLEM][2][0]; // 1
	entity->focaly = limbs[CRYSTALGOLEM][2][1]; // 0
	entity->focalz = limbs[CRYSTALGOLEM][2][2]; // 2
	entity->behavior = &actCrystalgolemLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// left leg
	entity = newEntity(479, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[CRYSTALGOLEM][3][0]; // 1
	entity->focaly = limbs[CRYSTALGOLEM][3][1]; // 0
	entity->focalz = limbs[CRYSTALGOLEM][3][2]; // 2
	entity->behavior = &actCrystalgolemLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// right arm
	entity = newEntity(478, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[CRYSTALGOLEM][4][0]; // -.25
	entity->focaly = limbs[CRYSTALGOLEM][4][1]; // 0
	entity->focalz = limbs[CRYSTALGOLEM][4][2]; // 3
	entity->behavior = &actCrystalgolemLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// left arm
	entity = newEntity(477, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[CRYSTALGOLEM][5][0]; // -.25
	entity->focaly = limbs[CRYSTALGOLEM][5][1]; // 0
	entity->focalz = limbs[CRYSTALGOLEM][5][2]; // 3
	entity->behavior = &actCrystalgolemLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
}

void actCrystalgolemLimb(Entity* my)
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

void crystalgolemDie(Entity* my)
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
	playSoundEntity(my, 80, 128);
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

#define CRYSTALGOLEMWALKSPEED .12

void crystalgolemMoveBodyparts(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = NULL;
	Entity* rightbody = NULL;
	Entity* rightarm = NULL;
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

		// sleeping
		if ( myStats->EFFECTS[EFF_ASLEEP] )
		{
			my->z = 1.5;
		}
		else
		{
			my->z = -1.5;
		}
	}

	//Move bodyparts
	for (bodypart = 0, node = my->children.first; node != NULL; node = node->next, bodypart++)
	{
		if ( bodypart < 2 )
		{
			// post-swing head animation.
			limbAnimateWithOvershoot(my, ANIMATE_PITCH, 0.2, PI / 4, 0.1, 0, ANIMATE_DIR_POSITIVE);
			continue;
		}
		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		entity->yaw = my->yaw;

		if ( bodypart == 3 || bodypart == 6 )
		{

			// left leg, right arm.
			if ( bodypart == 3 )
			{
				// set rightbody to the right leg.
				rightbody = (Entity*)node->next->element;
			}
			if ( bodypart == 3 || MONSTER_ATTACK == 0 )
			{
				// swing left leg/right arm in sync.
				if ( dist > 0.1 )
				{
					if ( !rightbody->skill[0] )
					{
						entity->pitch -= dist * CRYSTALGOLEMWALKSPEED;
						if ( entity->pitch < -PI / 4.0 )
						{
							entity->pitch = -PI / 4.0;
							if (bodypart == 3)
							{
								playSoundEntityLocal(my, 115, 64);
								entity->skill[0] = 1;
							}
						}
					}
					else
					{
						entity->pitch += dist * CRYSTALGOLEMWALKSPEED;
						if ( entity->pitch > PI / 4.0 )
						{
							entity->pitch = PI / 4.0;
							if (bodypart == 3)
							{
								playSoundEntityLocal(my, 115, 64);
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
				// ATTACK!
				// move right arm
				// vertical chop
				if ( MONSTER_ATTACKTIME == 0 )
				{
					// prepare the arm animation
					if ( MONSTER_ATTACK == GOLEM_SMASH )
					{
						entity->pitch = 0;
						entity->roll = 0;
					}
					else
					{
						entity->pitch = -3 * PI / 4;
						entity->roll = 0;
					}
				}
				else
				{
					if ( MONSTER_ATTACK == GOLEM_SMASH )
					{
						myStats->EFFECTS[EFF_PARALYZED] = true;
						myStats->EFFECTS_TIMERS[EFF_PARALYZED] = 60;
						if ( MONSTER_ATTACKTIME < 40 )
						{
							// move the head.
							limbAnimateToLimit(my, ANIMATE_PITCH, -0.1, -PI / 6, true, 0.1);

							// adjust pitch/roll of arms
							//entity->pitch -= 0.1;
							//entity->roll -= 0.2;

							limbAnimateToLimit(entity, ANIMATE_PITCH, -0.1, -7 * PI / 8, true, 0.1);
							limbAnimateToLimit(entity, ANIMATE_ROLL, -0.2, PI / 16, false, 0);
							/*if ( entity->roll < PI / 16 )
							{
								entity->roll = PI / 16;
							}
							if ( entity->pitch < -7 * PI / 8 )
							{
								entity->pitch = -7 * PI / 8;
							}*/
						}
						else if ( MONSTER_ATTACKTIME == 40 )
						{
							playSoundEntityLocal(my, 79, 128);
						}
						else if ( MONSTER_ATTACKTIME > 50 )
						{
							// reset the head.
							my->fskill[21] = ANIMATE_OVERSHOOT_TO_SETPOINT;
							//limbAnimateToLimit(my, ANIMATE_PITCH, 0, 0, false, 0);
							my->attack(10, 0, nullptr);
						}
					}
					else
					{
						if ( entity->pitch >= PI / 4 )
						{
							// reset limbs
							entity->skill[0] = rightbody->skill[0];
							entity->pitch = rightbody->pitch;
							entity->roll = 0;
							if ( MONSTER_ATTACK == 10 )
							{
								myStats->EFFECTS[EFF_PARALYZED] = false;
							}
							MONSTER_ATTACK = 0;

							// set overshoot animation for head.
						}
						else
						{
							entity->pitch += .25;
						}
					}
				}
			}
		}
		else if ( bodypart == 4 || bodypart == 5 )
		{
			// left arm
			if ( bodypart == 5 )
			{
				if ( MONSTER_ATTACK > 0 )
				{
					// get rightarm from bodypart 6 element if ready to attack
					rightarm = (Entity*)node->next->element;

					if ( MONSTER_ATTACK == GOLEM_SMASH )
					{
						if ( rightarm != NULL )
						{
							// follow the right arm animation.
							entity->pitch = rightarm->pitch;
							entity->roll = -rightarm->roll;
						}
					}
					else
					{
						// vertical chop
						if ( MONSTER_ATTACKTIME == 0 )
						{
							entity->pitch = -3 * PI / 4;
							entity->roll = 0;
						}
						else
						{
							if ( entity->pitch >= PI / 4 )
							{
								entity->skill[0] = rightbody->skill[0];
								entity->pitch = rightbody->pitch;
								entity->roll = 0;
							}
							else
							{
								entity->pitch += .25;
							}
						}
					}
				}
			}

			if ( bodypart != 5 || (MONSTER_ATTACK == 0) )
			{
				// swing right leg/ left arm in sync
				if ( dist > 0.1 )
				{
					if ( entity->skill[0] )
					{
						entity->pitch -= dist * CRYSTALGOLEMWALKSPEED;
						if ( entity->pitch < -PI / 4.0 )
						{
							entity->skill[0] = 0;
							entity->pitch = -PI / 4.0;
						}
					}
					else
					{
						entity->pitch += dist * CRYSTALGOLEMWALKSPEED;
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
				entity->x -= .5 * cos(my->yaw);
				entity->y -= .5 * sin(my->yaw);
				entity->z += 2.25;
				break;
			// left leg
			case 3:
				entity->x += 2 * cos(my->yaw + PI / 2) - 1.25 * cos(my->yaw);
				entity->y += 2 * sin(my->yaw + PI / 2) - 1.25 * sin(my->yaw);
				entity->z += 5;
				if ( my->z >= 1.4 && my->z <= 1.6 )
				{
					entity->yaw += PI / 8;
					entity->pitch = -PI / 2;
				}
				break;
			// right leg
			case 4:
				entity->x -= 2 * cos(my->yaw + PI / 2) + 1.25 * cos(my->yaw);
				entity->y -= 2 * sin(my->yaw + PI / 2) + 1.25 * sin(my->yaw);
				entity->z += 5;
				if ( my->z >= 1.4 && my->z <= 1.6 )
				{
					entity->yaw -= PI / 8;
					entity->pitch = -PI / 2;
				}
				break;
			// left arm
			case 5:
				entity->x += 3.5 * cos(my->yaw + PI / 2) - 1 * cos(my->yaw);
				entity->y += 3.5 * sin(my->yaw + PI / 2) - 1 * sin(my->yaw);
				entity->z += .1;
				entity->yaw += MONSTER_WEAPONYAW;
				if ( my->z >= 1.4 && my->z <= 1.6 )
				{
					entity->pitch = 0;
				}
				break;
			// right arm
			case 6:
				entity->x -= 3.5 * cos(my->yaw + PI / 2) + 1 * cos(my->yaw);
				entity->y -= 3.5 * sin(my->yaw + PI / 2) + 1 * sin(my->yaw);
				entity->z += .1;
				if ( my->z >= 1.4 && my->z <= 1.6 )
				{
					entity->pitch = 0;
				}
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
