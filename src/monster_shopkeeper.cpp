/*-------------------------------------------------------------------------------

	BARONY
	File: monster_shopkeeper.cpp
	Desc: implements all of the shopkeeper's code

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

void initShopkeeper(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	my->sprite = 217;
	//my->flags[GENIUS]=true;
	my->flags[UPDATENEEDED] = true;
	my->flags[BLOCKSIGHT] = true;
	my->flags[INVISIBLE] = false;

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = -1;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 1;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		if ( myStats )
		{
			for ( c = 0; c < std::max(NUMPROFICIENCIES, NUMEFFECTS); c++ )
			{
				if ( c < NUMPROFICIENCIES )
				{
					myStats->PROFICIENCIES[c] = 0;
				}
				if ( c < NUMEFFECTS )
				{
					myStats->EFFECTS[c] = false;
					myStats->EFFECTS_TIMERS[c] = 0;
				}
			}
		}

		int x, y;
		MONSTER_SHOPXS = my->x / 16;
		MONSTER_SHOPXE = my->x / 16;
		MONSTER_SHOPYS = my->y / 16;
		MONSTER_SHOPYE = my->y / 16;
		for ( x = my->x; x >= 0; x -= 16 )
		{
			if ( !checkObstacle(x, my->y, my, NULL) )
			{
				MONSTER_SHOPXS = x;
			}
			else
			{
				break;
			}
		}
		for ( x = my->x; x < map.width << 4; x += 16 )
		{
			if ( !checkObstacle(x, my->y, my, NULL) )
			{
				MONSTER_SHOPXE = x;
			}
			else
			{
				break;
			}
		}
		for ( y = my->y; y >= 0; y -= 16 )
		{
			if ( !checkObstacle(my->x, y, my, NULL) )
			{
				MONSTER_SHOPYS = y;
			}
			else
			{
				break;
			}
		}
		for ( y = my->y; y < map.height << 4; y += 16 )
		{
			if ( !checkObstacle(my->x, y, my, NULL) )
			{
				MONSTER_SHOPYE = y;
			}
			else
			{
				break;
			}
		}
		for ( x = MONSTER_SHOPXS - 16; x <= MONSTER_SHOPXE + 16; x += 16 )
		{
			for ( y = MONSTER_SHOPYS - 16; y <= MONSTER_SHOPYE + 16; y += 16 )
			{
				if ( x / 16 >= 0 && x / 16 < map.width && y / 16 >= 0 && y / 16 < map.height )
				{
					shoparea[y / 16 + (x / 16)*map.height] = true;
				}
			}
		}

		myStats->sex = MALE;
		myStats->appearance = rand();
		strcpy(myStats->name, language[158 + rand() % 26]);
		myStats->inventory.first = NULL;
		myStats->inventory.last = NULL;
		myStats->HP = 300;
		myStats->MAXHP = 300;
		myStats->MP = 200;
		myStats->MAXMP = 200;
		myStats->OLDHP = myStats->HP;
		myStats->STR = 10;
		myStats->DEX = 4;
		myStats->CON = 10;
		myStats->INT = 7;
		myStats->PER = 7;
		myStats->CHR = 3 + rand() % 4;
		myStats->EXP = 0;
		myStats->LVL = 10;
		myStats->GOLD = 300 + rand() % 200;
		myStats->HUNGER = 900;
		if ( !myStats->leader_uid )
		{
			myStats->leader_uid = 0;
		}
		myStats->FOLLOWERS.first = NULL;
		myStats->FOLLOWERS.last = NULL;
		myStats->PROFICIENCIES[PRO_MAGIC] = 50;
		myStats->PROFICIENCIES[PRO_SPELLCASTING] = 50;
		myStats->PROFICIENCIES[PRO_TRADING] = 75;
		myStats->PROFICIENCIES[PRO_APPRAISAL] = 75;
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
		myStats->weapon = newItem(SPELLBOOK_MAGICMISSILE, EXCELLENT, 0, 1, 0, false, NULL);

		if ( rand() % 20 == 0 )
		{
			myStats->EFFECTS[EFF_ASLEEP] = true;
			myStats->EFFECTS_TIMERS[EFF_ASLEEP] = 1800 + rand() % 3600;
		}

		// give shopkeeper items
		MONSTER_STORETYPE = rand() % 9;
		if ( MONSTER_STORETYPE == 8 )
		{
			MONSTER_STORETYPE++;
		}
		int numitems = 10 + rand() % 5;
		switch ( MONSTER_STORETYPE )
		{
			case 0:
				// arms & armor store
				for ( c = 0; c < numitems; c++ )
				{
					if ( rand() % 2 )
					{
						newItem( static_cast<ItemType>(rand() % 20), static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 4, rand(), false, &myStats->inventory );
					}
					else
					{
						int i = rand() % 21;
						if ( i < 18 )
						{
							newItem( static_cast<ItemType>(GLOVES + i), static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 4, rand(), false, &myStats->inventory );
						}
						else
						{
							newItem( static_cast<ItemType>(GLOVES + i + 4), static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 6, rand(), false, &myStats->inventory );
						}
					}
				}
				break;
			case 1:
				// hat store
				for ( c = 0; c < numitems; c++ )
				{
					newItem( static_cast<ItemType>(HAT_PHRYGIAN + rand() % 7), static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 6, rand(), false, &myStats->inventory );
				}
				break;
			case 2:
				// jewelry store
				for ( c = 0; c < numitems; c++ )
				{
					switch ( rand() % 3 )
					{
						case 0:
							newItem( static_cast<ItemType>(AMULET_SEXCHANGE + rand() % 6), static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 2, rand(), false, &myStats->inventory );
							break;
						case 1:
							newItem( static_cast<ItemType>(RING_ADORNMENT + rand() % 12), static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 2, rand(), false, &myStats->inventory );
							break;
						case 2:
							newItem( static_cast<ItemType>(GEM_GARNET + rand() % 16), static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 2, rand(), false, &myStats->inventory );
							break;
					}
				}
				break;
			case 3:
				// bookstore
				for ( c = 0; c < numitems; c++ )
				{
					switch ( rand() % 3 )
					{
						case 0:
							newItem( static_cast<ItemType>(SPELLBOOK_FORCEBOLT + rand() % 22), static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 2, rand(), true, &myStats->inventory );
							break;
						case 1:
							newItem( static_cast<ItemType>(SCROLL_MAIL + rand() % 14), static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 2, rand(), true, &myStats->inventory );
							break;
						case 2:
							newItem( READABLE_BOOK, static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 3, rand(), false, &myStats->inventory );
							break;
					}
				}
				break;
			case 4:
				// apothecary
				for ( c = 0; c < numitems; c++ )
				{
					newItem( static_cast<ItemType>(POTION_WATER + rand() % 15), static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 5, rand(), true, &myStats->inventory );
				}
				break;
			case 5:
				// staff shop
				for ( c = 0; c < numitems; c++ )
				{
					newItem( static_cast<ItemType>(MAGICSTAFF_LIGHT + rand() % 10), static_cast<Status>(WORN + rand() % 3), 0, 1, 1, true, &myStats->inventory );
				}
				break;
			case 6:
				// food store
				for ( c = 0; c < numitems; c++ )
				{
					newItem( static_cast<ItemType>(FOOD_BREAD + rand() % 7), static_cast<Status>(SERVICABLE + rand() % 2), 0, 1 + rand() % 3, rand(), false, &myStats->inventory );
				}
				break;
			case 7:
				// hardware store
				for ( c = 0; c < numitems; c++ )
				{
					newItem( static_cast<ItemType>(TOOL_PICKAXE + rand() % 11), static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 3, rand(), false, &myStats->inventory );
				}
				break;
			case 8:
				// lighting store
				for ( c = 0; c < numitems; c++ )
				{
					newItem( static_cast<ItemType>(TOOL_TORCH + rand() % 2), EXCELLENT, 0, 1, 7, false, &myStats->inventory );
				}
				break;
			case 9:
				// general store
				for ( c = 0; c < numitems; c++ )
				{
					newItem( static_cast<ItemType>(rand() % (NUMITEMS - (NUMITEMS - SPELL_ITEM))), static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 3, rand(), false, &myStats->inventory );
				}
				break;
		}
	}

	// torso
	Entity* entity = newEntity(218, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SHOPKEEPER][1][0]; // 0
	entity->focaly = limbs[SHOPKEEPER][1][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][1][2]; // 0
	entity->behavior = &actShopkeeperLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// right leg
	entity = newEntity(222, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SHOPKEEPER][2][0]; // 0
	entity->focaly = limbs[SHOPKEEPER][2][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][2][2]; // 2
	entity->behavior = &actShopkeeperLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// left leg
	entity = newEntity(221, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SHOPKEEPER][3][0]; // 0
	entity->focaly = limbs[SHOPKEEPER][3][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][3][2]; // 2
	entity->behavior = &actShopkeeperLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// right arm
	entity = newEntity(220, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SHOPKEEPER][4][0]; // 0
	entity->focaly = limbs[SHOPKEEPER][4][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][4][2]; // 1.5
	entity->behavior = &actShopkeeperLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// left arm
	entity = newEntity(219, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SHOPKEEPER][5][0]; // 0
	entity->focaly = limbs[SHOPKEEPER][5][1]; // 0
	entity->focalz = limbs[SHOPKEEPER][5][2]; // 1.5
	entity->behavior = &actShopkeeperLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
}

void actShopkeeperLimb(Entity* my)
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

void shopkeeperDie(Entity* my)
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
				entity->z = 8.0 + (rand() % 20) / 100.0;
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

#define SHOPKEEPERWALKSPEED .15

void shopkeeperMoveBodyparts(Entity* my, Stat* myStats, double dist)
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

		// sleeping
		if ( myStats->EFFECTS[EFF_ASLEEP] )
		{
			my->z = 1.5;
			my->pitch = PI / 4;
		}
		else
		{
			my->z = -1;
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
			if ( dist > 0.1 && bodypart != 6 )
			{
				if ( !rightbody->skill[0] )
				{
					entity->pitch -= dist * SHOPKEEPERWALKSPEED;
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
					entity->pitch += dist * SHOPKEEPERWALKSPEED;
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
		else if ( bodypart == 4 || bodypart == 5 )
		{
			if ( bodypart == 5 )
			{
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

			if ( bodypart != 5 || (MONSTER_ATTACK == 0 && MONSTER_ATTACKTIME == 0) )
			{
				if ( dist > 0.1 )
				{
					if ( entity->skill[0] )
					{
						entity->pitch -= dist * SHOPKEEPERWALKSPEED;
						if ( entity->pitch < -PI / 4.0 )
						{
							entity->skill[0] = 0;
							entity->pitch = -PI / 4.0;
						}
					}
					else
					{
						entity->pitch += dist * SHOPKEEPERWALKSPEED;
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
		switch ( bodypart )
		{
			// torso
			case 2:
				entity->x -= .25 * cos(my->yaw);
				entity->y -= .25 * sin(my->yaw);
				entity->z += 2.5;
				break;
			// right leg
			case 3:
				entity->x += 1 * cos(my->yaw + PI / 2) + .25 * cos(my->yaw);
				entity->y += 1 * sin(my->yaw + PI / 2) + .25 * sin(my->yaw);
				entity->z += 5;
				if ( my->z >= 1.4 && my->z <= 1.6 )
				{
					entity->yaw += PI / 8;
					entity->pitch = -PI / 2;
				}
				break;
			// left leg
			case 4:
				entity->x -= 1 * cos(my->yaw + PI / 2) - .25 * cos(my->yaw);
				entity->y -= 1 * sin(my->yaw + PI / 2) - .25 * sin(my->yaw);
				entity->z += 5;
				if ( my->z >= 1.4 && my->z <= 1.6 )
				{
					entity->yaw -= PI / 8;
					entity->pitch = -PI / 2;
				}
				break;
			// right arm
			case 5:
				entity->x += 2.25 * cos(my->yaw + PI / 2) - .20 * cos(my->yaw);
				entity->y += 2.25 * sin(my->yaw + PI / 2) - .20 * sin(my->yaw);
				entity->z += 1.5;
				entity->focalx = limbs[SHOPKEEPER][4][0]; // 0
				entity->focaly = limbs[SHOPKEEPER][4][1]; // 0
				entity->focalz = limbs[SHOPKEEPER][4][2]; // 1.5
				entity->yaw += MONSTER_WEAPONYAW;
				if ( my->z >= 1.4 && my->z <= 1.6 )
				{
					entity->pitch = 0;
				}
				break;
			// left arm
			case 6:
				entity->x -= 2.25 * cos(my->yaw + PI / 2) + .20 * cos(my->yaw);
				entity->y -= 2.25 * sin(my->yaw + PI / 2) + .20 * sin(my->yaw);
				entity->z += 1.5;
				entity->focalx = limbs[SHOPKEEPER][5][0]; // 0
				entity->focaly = limbs[SHOPKEEPER][5][1]; // 0
				entity->focalz = limbs[SHOPKEEPER][5][2]; // 1.5
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
