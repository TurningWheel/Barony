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

#define GATE_INIT my->skill[1]
#define GATE_STATUS my->skill[3]
#define GATE_RATTLE my->skill[4]
#define GATE_STARTHEIGHT my->fskill[0]
#define GATE_VELZ my->vel_z

void actGate(Entity* my)
{
	int i;

	if ( localPlayerNetworkType != NetworkType::CLIENT )
	{
		if (!my->skill[28])
		{
			return;    //Gate needs the mechanism powered state variable to be set.
		}

		if (my->skill[28] == 2)
		{
			//Raise gate if it's closed.
			if (!GATE_STATUS)
			{
				GATE_STATUS = 1;
				playSoundEntity(my, 81, 64);
				serverUpdateEntitySkill(my, 3);
			}
		}
		else
		{
			//Close gate if it's open.
			if (GATE_STATUS)
			{
				GATE_STATUS = 0;
				playSoundEntity(my, 82, 64);
				serverUpdateEntitySkill(my, 3);
			}
		}
	}
	else
	{
		my->flags[NOUPDATE] = true;
	}
	if (!GATE_INIT )
	{
		GATE_INIT = 1;
		GATE_STARTHEIGHT = my->z;
		my->scalex = 1.01;
		my->scaley = 1.01;
		my->scalez = 1.01;
	}

	// rightclick message
	if ( localPlayerNetworkType != NetworkType::CLIENT )
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

	if ( !GATE_STATUS )
	{
		//Closing gate.
		if ( my->z < GATE_STARTHEIGHT )
		{
			GATE_VELZ += .25;
			my->z = std::min(GATE_STARTHEIGHT, my->z + GATE_VELZ);
		}
		else
		{
			GATE_VELZ = 0;
		}
	}
	else
	{
		//Opening gate.
		if ( my->z > GATE_STARTHEIGHT - 12 )
		{
			my->z = std::max(GATE_STARTHEIGHT - 12, my->z - 0.25);

			// rattle the gate
			GATE_RATTLE = (GATE_RATTLE == 0);
			if ( GATE_RATTLE )
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
			if ( GATE_RATTLE )
			{
				GATE_RATTLE = 0;
				my->x -= .05;
				my->y -= .05;
			}
		}
	}

	//Setting collision
	node_t* node;
	bool somebodyinside = false;
	if ( my->z > GATE_STARTHEIGHT - 6 && my->flags[PASSABLE] )
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
	else if ( my->z < GATE_STARTHEIGHT - 9 && !my->flags[PASSABLE] )
	{
		my->flags[PASSABLE] = true;
	}
}
