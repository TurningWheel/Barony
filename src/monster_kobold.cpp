/*-------------------------------------------------------------------------------

	BARONY
	File: monster_kobold.cpp
	Desc: implements all of the kobold monster's code

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
#include "book.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"

void initKobold(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	my->sprite = 421; //Kobold head model

	my->flags[UPDATENEEDED] = true;
	my->flags[BLOCKSIGHT] = true;
	my->flags[INVISIBLE] = false;

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 220;
		MONSTER_SPOTVAR = 5;
		MONSTER_IDLESND = 217;
		MONSTER_IDLEVAR = 3;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		myStats->sex = static_cast<sex_t>(rand() % 2);
		myStats->appearance = rand();
		myStats->inventory.first = NULL;
		myStats->inventory.last = NULL;
		myStats->HP = 50;
		myStats->MAXHP = 50;
		myStats->MP = 50;
		myStats->MAXMP = 50;
		myStats->OLDHP = myStats->HP;
		myStats->STR = 2;
		myStats->DEX = 0;
		myStats->CON = 4;
		myStats->INT = 0;
		myStats->PER = 2;
		myStats->CHR = -1;
		myStats->EXP = 0;
		myStats->LVL = 5;
		myStats->GOLD = 40 + rand() % 20;
		myStats->HUNGER = 900;
		if ( !myStats->leader_uid )
		{
			myStats->leader_uid = 0;
		}
		myStats->FOLLOWERS.first = NULL;
		myStats->FOLLOWERS.last = NULL;
		for ( c = 0; c < std::max(NUMPROFICIENCIES, NUMEFFECTS); c++ )
		{
			if ( c < NUMPROFICIENCIES )
			{
				myStats->PROFICIENCIES[c] = 0;
			}
			if ( c < NUMEFFECTS )
			{
				myStats->EFFECTS[c] = false;
			}
			if ( c < NUMEFFECTS )
			{
				myStats->EFFECTS_TIMERS[c] = 0;
			}
		}
		myStats->PROFICIENCIES[PRO_SWORD] = 35;
		myStats->PROFICIENCIES[PRO_MACE] = 50;
		myStats->PROFICIENCIES[PRO_AXE] = 45;
		myStats->PROFICIENCIES[PRO_POLEARM] = 25;
		myStats->PROFICIENCIES[PRO_RANGED] = 35;
		myStats->PROFICIENCIES[PRO_SHIELD] = 35;
		myStats->helmet = NULL;
		myStats->breastplate = NULL;
		myStats->gloves = NULL;
		myStats->shoes = NULL;
		myStats->shield = NULL;
		myStats->weapon = NULL;
		myStats->cloak = NULL;
		myStats->amulet = NULL;
		myStats->ring = NULL;
		myStats->mask = NULL;

		if ( rand() % 8 == 0 )
		{
			myStats->EFFECTS[EFF_ASLEEP] = true;
			myStats->EFFECTS_TIMERS[EFF_ASLEEP] = 1800 + rand() % 1800;
		}

		if ( rand() % 3 == 0 )
		{
			newItem( FOOD_FISH, EXCELLENT, 0, 1, rand(), false, &myStats->inventory );
		}

		if ( rand() % 50 == 0 )
		{
			newItem( READABLE_BOOK, EXCELLENT, 0, 1, getBook("Winny's Report"), false, &myStats->inventory );
		}

		if ( rand() % 10 == 0 )
		{
			int i = 1 + rand() % 4;
			for ( c = 0; c < i; c++ )
			{
				newItem( static_cast<ItemType>(GEM_GARNET + rand() % 15), static_cast<Status>(1 + rand() % 4), 0, 1, rand(), false, &myStats->inventory );
			}
		}

		if ( rand() % 50 == 0 && !my->flags[USERFLAG2] )
		{
			strcpy(myStats->name, "Rumplewort");
			myStats->LVL += 10;
			newItem( GEM_DIAMOND, static_cast<Status>(1 + rand() % 4), 0, 1, rand(), true, &myStats->inventory );

			int c;
			for ( c = 0; c < 3; c++ )
			{
				Entity* entity = summonMonster(KOBOLD, my->x, my->y);
				if ( entity )
				{
					entity->parent = my->getUID();
				}
			}
		}
	}

	// torso
	Entity* entity = newEntity(422, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[KOBOLD][1][0]; // 0
	entity->focaly = limbs[KOBOLD][1][1]; // 0
	entity->focalz = limbs[KOBOLD][1][2]; // 0
	entity->behavior = &actKoboldLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// right leg
	entity = newEntity(423, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[KOBOLD][2][0]; // .25
	entity->focaly = limbs[KOBOLD][2][1]; // 0
	entity->focalz = limbs[KOBOLD][2][2]; // 1.5
	entity->behavior = &actKoboldLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// left leg
	entity = newEntity(424, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[KOBOLD][3][0]; // .25
	entity->focaly = limbs[KOBOLD][3][1]; // 0
	entity->focalz = limbs[KOBOLD][3][2]; // 1.5
	entity->behavior = &actKoboldLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// right arm
	entity = newEntity(425, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[KOBOLD][4][0]; // 0
	entity->focaly = limbs[KOBOLD][4][1]; // 0
	entity->focalz = limbs[KOBOLD][4][2]; // 2
	entity->behavior = &actKoboldLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// left arm
	entity = newEntity(427, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[KOBOLD][5][0]; // 0
	entity->focaly = limbs[KOBOLD][5][1]; // 0
	entity->focalz = limbs[KOBOLD][5][2]; // 2
	entity->behavior = &actKoboldLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// world weapon
	entity = newEntity(-1, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[INVISIBLE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[KOBOLD][6][0]; // 2
	entity->focaly = limbs[KOBOLD][6][1]; // 0
	entity->focalz = limbs[KOBOLD][6][2]; // -.5
	entity->behavior = &actKoboldLimb;
	entity->parent = my->getUID();
	entity->pitch = .25;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// shield
	entity = newEntity(-1, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[INVISIBLE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[KOBOLD][7][0]; // 0
	entity->focaly = limbs[KOBOLD][7][1]; // 0
	entity->focalz = limbs[KOBOLD][7][2]; // 1.5
	entity->behavior = &actKoboldLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// cloak
	entity = newEntity(-1, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->scalex = 1.01;
	entity->scaley = 1.01;
	entity->scalez = 1.01;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[INVISIBLE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[KOBOLD][8][0]; // 0
	entity->focaly = limbs[KOBOLD][8][1]; // 0
	entity->focalz = limbs[KOBOLD][8][2]; // 4
	entity->behavior = &actKoboldLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	if ( multiplayer == CLIENT || MONSTER_INIT )
	{
		return;
	}

	// give shield
	switch ( rand() % 10 )
	{
		case 0:
		case 1:
			myStats->shield = newItem(TOOL_LANTERN, EXCELLENT, -1 + rand() % 3, 1, rand(), false, NULL);
			break;
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			break;
		case 7:
		case 8:
		case 9:
			myStats->shield = newItem(WOODEN_SHIELD, static_cast<Status>(WORN + rand() % 2), -1 + rand() % 3, 1, rand(), false, NULL);
			break;
	}

	// give weapon
	switch ( rand() % 10 )
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
			myStats->weapon = newItem(TOOL_PICKAXE, EXCELLENT, -1 + rand() % 3, 1, rand(), false, NULL);
			break;
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
			myStats->GOLD += 100;
			//myStats->weapon = newItem(MAGICSTAFF_LIGHTNING, EXCELLENT, -1 + rand() % 3, 1, rand(), false, NULL);
			break;
	}

	// give cloak
	switch ( rand() % 10 )
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			break;
		case 6:
		case 7:
		case 8:
		case 9:
			myStats->cloak = newItem(CLOAK, SERVICABLE, -1 + rand() % 3, 1, rand(), false, NULL);
			break;
	}
}

void actKoboldLimb(Entity* my)
{
	int i;

	Entity* parent = NULL;
	if ( (parent = uidToEntity(my->skill[2])) == NULL )
	{
		list_RemoveNode(my->mynode);
		return;
	}

	if ( my->light != NULL )
	{
		list_RemoveNode(my->light->node);
		my->light = NULL;
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

	int torch = 0;
	if ( my->flags[INVISIBLE] == false )
	{
		if ( my->sprite == 93 )   // torch
		{
			torch = 6;
		}
		else if ( my->sprite == 94 )     // lantern
		{
			torch = 9;
		}
	}
	if ( torch != 0 )
	{
		my->light = lightSphereShadow(my->x / 16, my->y / 16, torch, 50 + 15 * torch);
	}
}

void koboldDie(Entity* my)
{
	node_t* node, *nextnode;
	int c;
	for ( c = 0; c < 6; c++ )
	{
		Entity* entity = spawnGib(my);
		if ( entity )
		{
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
	int i = 0;
	for ( node = my->children.first; node != NULL; node = nextnode )
	{
		nextnode = node->next;
		if ( node->element != NULL && i >= 2 )
		{
			Entity* entity = (Entity*)node->element;
			if ( entity->light != NULL )
			{
				list_RemoveNode(entity->light->node);
			}
			entity->light = NULL;
			list_RemoveNode(entity->mynode);
		}
		list_RemoveNode(node);
		i++;
	}
	playSoundEntity(my, 225 + rand() % 4, 128);
	list_RemoveNode(my->mynode);
	return;
}

#define KOBOLDWALKSPEED .13

void koboldMoveBodyparts(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = NULL, *entity2 = NULL;
	Entity* rightbody = NULL;
	Entity* weaponarm = NULL;
	int bodypart;
	bool wearingring = false;

	// set invisibility
	if ( multiplayer != CLIENT )
	{
		if ( myStats->ring != NULL )
			if ( myStats->ring->type == RING_INVISIBILITY )
			{
				wearingring = true;
			}
		if ( myStats->cloak != NULL )
			if ( myStats->cloak->type == CLOAK_INVISIBILITY )
			{
				wearingring = true;
			}
		if ( myStats->EFFECTS[EFF_INVISIBLE] == true || wearingring == true )
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
			my->z = 4;
			my->pitch = PI / 4;
		}
		else
		{
			my->z = 2.25;
			my->pitch = 0;
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
			node_t* shieldNode = list_Node(&my->children, 7);
			if ( shieldNode )
			{
				Entity* shield = (Entity*)shieldNode->element;
				if ( dist > 0.1 && (bodypart != 6 || shield->flags[INVISIBLE]) )
				{
					if ( !rightbody->skill[0] )
					{
						entity->pitch -= dist * KOBOLDWALKSPEED;
						if ( entity->pitch < -PI / 4.0 )
						{
							entity->pitch = -PI / 4.0;
							if (bodypart == 3)
							{
								entity->skill[0] = 1;
								if ( dist > .4 )
								{
									playSoundEntityLocal(my, rand() % 7, 32);
								}
							}
						}
					}
					else
					{
						entity->pitch += dist * KOBOLDWALKSPEED;
						if ( entity->pitch > PI / 4.0 )
						{
							entity->pitch = PI / 4.0;
							if (bodypart == 3)
							{
								entity->skill[0] = 0;
								if ( dist > .4 )
								{
									playSoundEntityLocal(my, rand() % 7, 32);
								}
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
		}
		else if ( bodypart == 4 || bodypart == 5 || bodypart == 9 )
		{
			if ( bodypart == 5 )
			{
				weaponarm = entity;
				if ( MONSTER_ATTACK == 1 )
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
				else if ( MONSTER_ATTACK == 2 )
				{
					// horizontal chop
					if ( MONSTER_ATTACKTIME == 0 )
					{
						MONSTER_ARMBENDED = 1;
						MONSTER_WEAPONYAW = -3 * PI / 4;
						entity->pitch = 0;
						entity->roll = -PI / 2;
					}
					else
					{
						if ( MONSTER_WEAPONYAW >= PI / 8 )
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
							MONSTER_WEAPONYAW += .25;
						}
					}
				}
				else if ( MONSTER_ATTACK == 3 )
				{
					// stab
					if ( MONSTER_ATTACKTIME == 0 )
					{
						MONSTER_ARMBENDED = 0;
						MONSTER_WEAPONYAW = 0;
						entity->pitch = 2 * PI / 3;
						entity->roll = 0;
					}
					else
					{
						if ( MONSTER_ATTACKTIME >= 5 )
						{
							MONSTER_ARMBENDED = 1;
							entity->pitch = -PI / 6;
						}
						if ( MONSTER_ATTACKTIME >= 10 )
						{
							entity->skill[0] = rightbody->skill[0];
							MONSTER_WEAPONYAW = 0;
							entity->pitch = rightbody->pitch;
							entity->roll = 0;
							MONSTER_ARMBENDED = 0;
							MONSTER_ATTACK = 0;
						}
					}
				}
			}
			else if ( bodypart == 9 )
			{
				entity->pitch = entity->fskill[0];
			}

			if ( bodypart != 5 || (MONSTER_ATTACK == 0 && MONSTER_ATTACKTIME == 0) )
			{
				if ( dist > 0.1 )
				{
					if ( entity->skill[0] )
					{
						entity->pitch -= dist * KOBOLDWALKSPEED;
						if ( entity->pitch < -PI / 4.0 )
						{
							entity->skill[0] = 0;
							entity->pitch = -PI / 4.0;
						}
					}
					else
					{
						entity->pitch += dist * KOBOLDWALKSPEED;
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
			if ( bodypart == 9 )
			{
				entity->fskill[0] = entity->pitch;
				entity->roll = my->roll - fabs(entity->pitch) / 2;
				entity->pitch = 0;
			}
		}
		switch ( bodypart )
		{
			// torso
			case 2:
				entity->x -= .25 * cos(my->yaw);
				entity->y -= .25 * sin(my->yaw);
				entity->z += 1.25;
				break;
			// right leg
			case 3:
				entity->x += 1.25 * cos(my->yaw + PI / 2);
				entity->y += 1.25 * sin(my->yaw + PI / 2);
				entity->z += 2.75;
				if ( my->z >= 3.9 && my->z <= 4.1 )
				{
					entity->yaw += PI / 8;
					entity->pitch = -PI / 2;
				}
				break;
			// left leg
			case 4:
				entity->x -= 1.25 * cos(my->yaw + PI / 2);
				entity->y -= 1.25 * sin(my->yaw + PI / 2);
				entity->z += 2.75;
				if ( my->z >= 3.9 && my->z <= 4.1 )
				{
					entity->yaw -= PI / 8;
					entity->pitch = -PI / 2;
				}
				break;
			// right arm
			case 5:
			{
				;
				node_t* weaponNode = list_Node(&my->children, 7);
				if ( weaponNode )
				{
					Entity* weapon = (Entity*)weaponNode->element;
					if ( weapon->flags[INVISIBLE] || MONSTER_ARMBENDED )
					{
						entity->focalx = limbs[KOBOLD][4][0]; // 0
						entity->focaly = limbs[KOBOLD][4][1]; // 0
						entity->focalz = limbs[KOBOLD][4][2]; // 2
						entity->sprite = 425;
					}
					else
					{
						entity->focalx = limbs[KOBOLD][4][0] + 1; // 1
						entity->focaly = limbs[KOBOLD][4][1]; // 0
						entity->focalz = limbs[KOBOLD][4][2] - 1; // 1
						entity->sprite = 426;
					}
				}
				entity->x += 2.5 * cos(my->yaw + PI / 2) - .75 * cos(my->yaw);
				entity->y += 2.5 * sin(my->yaw + PI / 2) - .75 * sin(my->yaw);
				entity->z -= .25;
				entity->yaw += MONSTER_WEAPONYAW;
				if ( my->z >= 3.9 && my->z <= 4.1 )
				{
					entity->pitch = 0;
				}
				break;
				// left arm
			}
			case 6:
			{
				;
				node_t* shieldNode = list_Node(&my->children, 8);
				if ( shieldNode )
				{
					Entity* shield = (Entity*)shieldNode->element;
					if ( shield->flags[INVISIBLE] )
					{
						entity->focalx = limbs[KOBOLD][5][0]; // 0
						entity->focaly = limbs[KOBOLD][5][1]; // 0
						entity->focalz = limbs[KOBOLD][5][2]; // 2
						entity->sprite = 427;
					}
					else
					{
						entity->focalx = limbs[KOBOLD][5][0] + 1; // 1
						entity->focaly = limbs[KOBOLD][5][1]; // 0
						entity->focalz = limbs[KOBOLD][5][2] - 1; // 1
						entity->sprite = 428;
					}
				}
				entity->x -= 2.5 * cos(my->yaw + PI / 2) + .75 * cos(my->yaw);
				entity->y -= 2.5 * sin(my->yaw + PI / 2) + .75 * sin(my->yaw);
				entity->z -= .25;
				if ( my->z >= 3.9 && my->z <= 4.1 )
				{
					entity->pitch = 0;
				}
				break;
			}
			// weapon
			case 7:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->weapon == NULL || myStats->EFFECTS[EFF_INVISIBLE] || wearingring )
					{
						entity->flags[INVISIBLE] = true;
					}
					else
					{
						entity->sprite = itemModel(myStats->weapon);
						if ( itemCategory(myStats->weapon) == SPELLBOOK )
						{
							entity->flags[INVISIBLE] = true;
						}
						else
						{
							entity->flags[INVISIBLE] = false;
						}
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->skill[11] != entity->flags[INVISIBLE] )
						{
							entity->skill[11] = entity->flags[INVISIBLE];
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				if ( weaponarm != NULL )
				{
					if ( entity->flags[INVISIBLE] != true )
					{
						if ( entity->sprite == items[SHORTBOW].index )
						{
							entity->x = weaponarm->x - .5 * cos(weaponarm->yaw);
							entity->y = weaponarm->y - .5 * sin(weaponarm->yaw);
							entity->z = weaponarm->z + 1;
							entity->pitch = weaponarm->pitch + .25;
						}
						else if ( entity->sprite == items[ARTIFACT_BOW].index )
						{
							entity->x = weaponarm->x - 1.5 * cos(weaponarm->yaw);
							entity->y = weaponarm->y - 1.5 * sin(weaponarm->yaw);
							entity->z = weaponarm->z + 2;
							entity->pitch = weaponarm->pitch + .25;
						}
						else if ( entity->sprite == items[CROSSBOW].index )
						{
							entity->x = weaponarm->x;
							entity->y = weaponarm->y;
							entity->z = weaponarm->z + 1;
							entity->pitch = weaponarm->pitch;
						}
						else
						{
							entity->x = weaponarm->x + .5 * cos(weaponarm->yaw) * (MONSTER_ATTACK == 0);
							entity->y = weaponarm->y + .5 * sin(weaponarm->yaw) * (MONSTER_ATTACK == 0);
							entity->z = weaponarm->z - .5 * (MONSTER_ATTACK == 0);
							entity->pitch = weaponarm->pitch + .25 * (MONSTER_ATTACK == 0);
						}
					}
					entity->yaw = weaponarm->yaw;
					entity->roll = weaponarm->roll;
					if ( !MONSTER_ARMBENDED )
					{
						entity->focalx = limbs[KOBOLD][6][0];
						if ( entity->sprite == items[CROSSBOW].index )
						{
							entity->focalx += 2;
						}
						entity->focaly = limbs[KOBOLD][6][1];
						entity->focalz = limbs[KOBOLD][6][2];
					}
					else
					{
						entity->focalx = limbs[KOBOLD][6][0] + 1;
						entity->focaly = limbs[KOBOLD][6][1];
						entity->focalz = limbs[KOBOLD][6][2] - 2;
						entity->yaw -= sin(weaponarm->roll) * PI / 2;
						entity->pitch += cos(weaponarm->roll) * PI / 2;
					}
				}
				break;
			// shield
			case 8:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->shield == NULL )
					{
						entity->flags[INVISIBLE] = true;
						entity->sprite = 0;
					}
					else
					{
						entity->flags[INVISIBLE] = false;
						entity->sprite = itemModel(myStats->shield);
					}
					if ( myStats->EFFECTS[EFF_INVISIBLE] || wearingring )
					{
						entity->flags[INVISIBLE] = true;
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->skill[11] != entity->flags[INVISIBLE] )
						{
							entity->skill[11] = entity->flags[INVISIBLE];
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				entity->x -= 2.5 * cos(my->yaw + PI / 2) + .20 * cos(my->yaw);
				entity->y -= 2.5 * sin(my->yaw + PI / 2) + .20 * sin(my->yaw);
				entity->z += 1;
				if ( entity->sprite == items[TOOL_TORCH].index )
				{
					entity2 = spawnFlame(entity, SPRITE_FLAME);
					entity2->x += 2 * cos(my->yaw);
					entity2->y += 2 * sin(my->yaw);
					entity2->z -= 2;
				}
				else if ( entity->sprite == items[TOOL_CRYSTALSHARD].index )
				{
					entity2 = spawnFlame(entity, SPRITE_CRYSTALFLAME);
					entity2->x += 2 * cos(my->yaw);
					entity2->y += 2 * sin(my->yaw);
					entity2->z -= 2;
				}
				else if ( entity->sprite == items[TOOL_LANTERN].index )
				{
					entity->z += 2;
					entity2 = spawnFlame(entity, SPRITE_FLAME);
					entity2->x += 2 * cos(my->yaw);
					entity2->y += 2 * sin(my->yaw);
					entity2->z += 1;
				}
				break;
			// cloak
			case 9:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->cloak == NULL || myStats->EFFECTS[EFF_INVISIBLE] || wearingring )
					{
						entity->flags[INVISIBLE] = true;
					}
					else
					{
						entity->flags[INVISIBLE] = false;
						entity->sprite = itemModel(myStats->cloak);
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->skill[11] != entity->flags[INVISIBLE] )
						{
							entity->skill[11] = entity->flags[INVISIBLE];
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				entity->x -= cos(my->yaw) * 1.5;
				entity->y -= sin(my->yaw) * 1.5;
				entity->yaw += PI / 2;
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
