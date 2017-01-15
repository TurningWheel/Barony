/*-------------------------------------------------------------------------------

	BARONY
	File: actdoor.cpp
	Desc: behavior function for doors

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "sound.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define DOOR_DIR my->skill[0]
#define DOOR_INIT my->skill[1]
#define DOOR_STATUS my->skill[3]
#define DOOR_HEALTH my->skill[4]
#define DOOR_LOCKED my->skill[5]
#define DOOR_SMACKED my->skill[6]
#define DOOR_TIMER my->skill[7]
#define DOOR_OLDSTATUS my->skill[8]
#define DOOR_MAXHEALTH my->skill[9]
#define DOOR_STARTANG my->fskill[0]

void actDoor(Entity* my)
{
	if (!my)
	{
		return;
	}

	Entity* entity;
	int i, c;

	if ( !DOOR_INIT )
	{
		DOOR_INIT = 1;
		DOOR_STARTANG = my->yaw;
		DOOR_HEALTH = 15 + rand() % 5;
		DOOR_MAXHEALTH = DOOR_HEALTH;
		if ( rand() % 20 == 0 )   // 5% chance
		{
			DOOR_LOCKED = 1;
		}
		DOOR_OLDSTATUS = DOOR_STATUS;
		my->scalex = 1.01;
		my->scaley = 1.01;
		my->scalez = 1.01;
		my->flags[BURNABLE] = TRUE;
	}
	else
	{
		if ( multiplayer != CLIENT )
		{
			// burning
			if ( my->flags[BURNING] )
			{
				if ( ticks % 30 == 0 )
				{
					DOOR_HEALTH--;
				}
			}

			// door mortality :p
			if ( DOOR_HEALTH <= 0 )
			{
				for ( c = 0; c < 5; c++ )
				{
					entity = spawnGib(my);
					entity->flags[INVISIBLE] = FALSE;
					entity->sprite = 187; // Splinter.vox
					entity->x = floor(my->x / 16) * 16 + 8;
					entity->y = floor(my->y / 16) * 16 + 8;
					entity->z = 0;
					entity->z += -7 + rand() % 14;
					if ( !DOOR_DIR )
					{
						// horizontal door
						entity->y += -4 + rand() % 8;
						if ( DOOR_SMACKED )
						{
							entity->yaw = PI;
						}
						else
						{
							entity->yaw = 0;
						}
					}
					else
					{
						// vertical door
						entity->x += -4 + rand() % 8;
						if ( DOOR_SMACKED )
						{
							entity->yaw = PI / 2;
						}
						else
						{
							entity->yaw = 3 * PI / 2;
						}
					}
					entity->pitch = (rand() % 360) * PI / 180.0;
					entity->roll = (rand() % 360) * PI / 180.0;
					entity->vel_x = cos(entity->yaw) * (1.2 + (rand() % 10) / 50.0);
					entity->vel_y = sin(entity->yaw) * (1.2 + (rand() % 10) / 50.0);
					entity->vel_z = -.25;
					entity->fskill[3] = 0.04;
					serverSpawnGibForClient(entity);
				}
				playSoundEntity(my, 177, 64);
				list_RemoveNode(my->mynode);
				return;
			}

			// using door
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if ( (i == 0 && selectedEntity == my) || (client_selected[i] == my) )
				{
					if (inrange[i])
					{
						if ( !DOOR_LOCKED )   // door unlocked
						{
							if ( !DOOR_DIR && !DOOR_STATUS )
							{
								// open door
								DOOR_STATUS = 1 + (players[i]->entity->x > my->x);
								playSoundEntity(my, 21, 96);
								messagePlayer(i, language[464]);
							}
							else if ( DOOR_DIR && !DOOR_STATUS )
							{
								// open door
								DOOR_STATUS = 1 + (players[i]->entity->y < my->y);
								playSoundEntity(my, 21, 96);
								messagePlayer(i, language[464]);
							}
							else
							{
								// close door
								DOOR_STATUS = 0;
								playSoundEntity(my, 22, 96);
								messagePlayer(i, language[465]);
							}
						}
						else
						{
							// door locked
							messagePlayer(i, language[466]);
							playSoundEntity(my, 152, 64);
						}
					}
				}
			}
		}

		// door swinging
		if ( !DOOR_STATUS )
		{
			// closing door
			if ( my->yaw > DOOR_STARTANG )
			{
				my->yaw = std::max(DOOR_STARTANG, my->yaw - 0.15);
			}
			else if ( my->yaw < DOOR_STARTANG )
			{
				my->yaw = std::min(DOOR_STARTANG, my->yaw + 0.15);
			}
		}
		else
		{
			// opening door
			if ( DOOR_STATUS == 1 )
			{
				if ( my->yaw > DOOR_STARTANG + PI / 2 )
				{
					my->yaw = std::max(DOOR_STARTANG + PI / 2, my->yaw - 0.15);
				}
				else if ( my->yaw < DOOR_STARTANG + PI / 2 )
				{
					my->yaw = std::min(DOOR_STARTANG + PI / 2, my->yaw + 0.15);
				}
			}
			else if ( DOOR_STATUS == 2 )
			{
				if ( my->yaw > DOOR_STARTANG - PI / 2 )
				{
					my->yaw = std::max(DOOR_STARTANG - PI / 2, my->yaw - 0.15);
				}
				else if ( my->yaw < DOOR_STARTANG - PI / 2 )
				{
					my->yaw = std::min(DOOR_STARTANG - PI / 2, my->yaw + 0.15);
				}
			}
		}

		// setting collision
		if ( my->yaw == DOOR_STARTANG && my->flags[PASSABLE] )
		{
			// don't set impassable if someone's inside, otherwise do
			node_t* node;
			bool somebodyinside = FALSE;
			for ( node = map.entities->first; node != NULL; node = node->next )
			{
				Entity* entity = (Entity*)node->element;
				if ( entity == my || entity->flags[PASSABLE] || entity->sprite == 1  )
				{
					continue;
				}
				if ( entityInsideEntity(my, entity) )
				{
					somebodyinside = TRUE;
					break;
				}
			}
			if ( !somebodyinside )
			{
				my->focaly = 0;
				if ( DOOR_STARTANG == 0 )
				{
					my->y -= 5;
				}
				else
				{
					my->x -= 5;
				}
				my->flags[PASSABLE] = FALSE;
			}
		}
		else if ( my->yaw != DOOR_STARTANG && !my->flags[PASSABLE] )
		{
			my->focaly = -5;
			if ( DOOR_STARTANG == 0 )
			{
				my->y += 5;
			}
			else
			{
				my->x += 5;
			}
			my->flags[PASSABLE] = TRUE;
		}

		// update for clients
		if ( multiplayer == SERVER )
		{
			if ( DOOR_OLDSTATUS != DOOR_STATUS )
			{
				DOOR_OLDSTATUS = DOOR_STATUS;
				serverUpdateEntitySkill(my, 3);
			}
		}
	}
}

void actDoorFrame(Entity* my)
{
	// dummy function
	// intended to make it easier
	// to determine whether an entity
	// is part of a door frame
	if ( my->sprite == 1 && my->flags[INVISIBLE] == FALSE )
	{
		my->flags[PASSABLE] = TRUE; // the actual frame should ALWAYS be passable
	}
}
