/*-------------------------------------------------------------------------------

	BARONY
	File: actarrowtrap.cpp
	Desc: implements arrow trap code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "sound.hpp"
#include "collision.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define ARROWTRAP_FIRED my->skill[0]
#define ARROWTRAP_AMBIENCE my->skill[6]

void actArrowTrap(Entity* my)
{
	int x, y;
	int c;

	// eliminate arrow traps that have been destroyed.
	if ( !checkObstacle(my->x, my->y, my, NULL) )
	{
		list_RemoveNode(my->mynode);
		return;
	}

	ARROWTRAP_AMBIENCE--;
	if ( ARROWTRAP_AMBIENCE <= 0 )
	{
		ARROWTRAP_AMBIENCE = TICKS_PER_SECOND * 30;
		playSoundEntity( my, 149, 128 );
	}

	if ( !my->skill[28] )
	{
		return;
	}

	// received on signal
	if ( my->skill[28] == 2)
	{
		if ( !ARROWTRAP_FIRED )
		{
			ARROWTRAP_FIRED = 1;
			for ( c = 0; c < 4; c++ )
			{
				switch ( c )
				{
					case 0:
						x = 12;
						y = 0;
						break;
					case 1:
						x = 0;
						y = 12;
						break;
					case 2:
						x = -12;
						y = 0;
						break;
					case 3:
						x = 0;
						y = -12;
						break;
				}
				if ( !checkObstacle(my->x + x, my->y + y, my, NULL) )
				{
					Entity* entity = newEntity(166, 1, map.entities); // arrow
					playSoundEntity(my, 239 + rand() % 3, 96);
					entity->parent = my->getUID();
					entity->x = my->x + x;
					entity->y = my->y + y;
					entity->z = my->z;
					entity->yaw = c * (PI / 2.f);
					entity->sizex = 1;
					entity->sizey = 1;
					entity->behavior = &actArrow;
					entity->flags[UPDATENEEDED] = TRUE;
					entity->flags[PASSABLE] = TRUE;

					// arrow power
					entity->skill[3] = 17;

					// causes poison for six seconds
					entity->skill[4] = 360;
				}
			}
		}
	}
}
