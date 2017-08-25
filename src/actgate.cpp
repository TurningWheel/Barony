/*-------------------------------------------------------------------------------

	BARONY
	File: gate.cpp
	Desc: implements all gate related code

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

void actGate(Entity* my)
{
	int i;

	if ( multiplayer != CLIENT )
	{
		if (!my->skill[28])
		{
			return;    //Gate needs the mechanism powered state variable to be set.
		}

		if (my->skill[28] == 2)
		{
			//Raise gate if it's closed.
			if (!my->gateStatus)
			{
				my->gateStatus = 1;
				playSoundEntity(my, 81, 64);
				serverUpdateEntitySkill(my, 3);
			}
		}
		else
		{
			//Close gate if it's open.
			if (my->gateStatus)
			{
				my->gateStatus = 0;
				playSoundEntity(my, 82, 64);
				serverUpdateEntitySkill(my, 3);
			}
		}
	}
	else
	{
		my->flags[NOUPDATE] = true;
	}
	if (!my->gateInit )
	{
		my->gateInit = 1;
		my->gateStartHeight = my->z;
		my->scalex = 1.01;
		my->scaley = 1.01;
		my->scalez = 1.01;
	}

	// rightclick message
	if ( multiplayer != CLIENT )
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if ( (i == 0 && selectedEntity == my) || (client_selected[i] == my) )
			{
				if (inrange[i])
				{
					messagePlayer(i, language[475]);
				}
			}
		}
	}

	if ( !my->gateStatus )
	{
		//Closing gate.
		if ( my->z < my->gateStartHeight )
		{
			my->gateVelZ += .25;
			my->z = std::min(my->gateStartHeight, my->z + my->gateVelZ);
		}
		else
		{
			my->gateVelZ = 0;
		}
	}
	else
	{
		//Opening gate.
		if ( my->z > my->gateStartHeight - 12 )
		{
			my->z = std::max(my->gateStartHeight - 12, my->z - 0.25);

			// rattle the gate
			my->gateRattle = (my->gateRattle == 0);
			if ( my->gateRattle )
			{
				my->x += .05;
				my->y += .05;
			}
			else
			{
				my->x -= .05;
				my->y -= .05;
			}
		}
		else
		{
			// reset the gate's position
			if ( my->gateRattle )
			{
				my->gateRattle = 0;
				my->x -= .05;
				my->y -= .05;
			}
		}
	}

	//Setting collision
	node_t* node;
	bool somebodyinside = false;
	if ( my->z > my->gateStartHeight - 6 && my->flags[PASSABLE] )
	{
		for ( node = map.entities->first; node != NULL; node = node->next )
		{
			Entity* entity = (Entity*)node->element;
			if ( entity == my || entity->flags[PASSABLE] || entity->sprite == 1 )
			{
				continue;
			}
			if ( entityInsideEntity(my, entity) )
			{
				somebodyinside = true;
				break;
			}
		}
		if ( !somebodyinside )
		{
			my->flags[PASSABLE] = false;
		}
	}
	else if ( my->z < my->gateStartHeight - 9 && !my->flags[PASSABLE] )
	{
		my->flags[PASSABLE] = true;
	}
}
