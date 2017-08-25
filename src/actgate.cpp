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
	if ( !my )
	{
		return;
	}

	my->actGate();
}

void Entity::actGate()
{
	int i;

	if ( multiplayer != CLIENT )
	{
		if ( this->circuit_status == 0 )
		{
			return;    //Gate needs the mechanism powered state variable to be set.
		}

		if ( this->circuit_status == CIRCUIT_ON )
		{
			//Raise gate if it's closed.
			if ( !this->gateStatus )
			{
				this->gateStatus = 1;
				playSoundEntity(this, 81, 64);
				serverUpdateEntitySkill(this, 3);
			}
		}
		else
		{
			//Close gate if it's open.
			if ( this->gateStatus )
			{
				this->gateStatus = 0;
				playSoundEntity(this, 82, 64);
				serverUpdateEntitySkill(this, 3);
			}
		}
	}
	else
	{
		this->flags[NOUPDATE] = true;
	}
	if ( !this->gateInit )
	{
		this->gateInit = 1;
		this->gateStartHeight = this->z;
		this->scalex = 1.01;
		this->scaley = 1.01;
		this->scalez = 1.01;
	}

	// rightclick message
	if ( multiplayer != CLIENT )
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if ( (i == 0 && selectedEntity == this) || (client_selected[i] == this) )
			{
				if (inrange[i])
				{
					messagePlayer(i, language[475]);
				}
			}
		}
	}

	if ( !this->gateStatus )
	{
		//Closing gate.
		if ( this->z < this->gateStartHeight )
		{
			this->gateVelZ += .25;
			this->z = std::min(this->gateStartHeight, this->z + this->gateVelZ);
		}
		else
		{
			this->gateVelZ = 0;
		}
	}
	else
	{
		//Opening gate.
		if ( this->z > this->gateStartHeight - 12 )
		{
			this->z = std::max(this->gateStartHeight - 12, this->z - 0.25);

			// rattle the gate
			this->gateRattle = (this->gateRattle == 0);
			if ( this->gateRattle )
			{
				this->x += .05;
				this->y += .05;
			}
			else
			{
				this->x -= .05;
				this->y -= .05;
			}
		}
		else
		{
			// reset the gate's position
			if ( this->gateRattle )
			{
				this->gateRattle = 0;
				this->x -= .05;
				this->y -= .05;
			}
		}
	}

	//Setting collision
	node_t* node;
	bool somebodyinside = false;
	if ( this->z > this->gateStartHeight - 6 && this->flags[PASSABLE] )
	{
		for ( node = map.entities->first; node != NULL; node = node->next )
		{
			Entity* entity = (Entity*)node->element;
			if ( entity == this || entity->flags[PASSABLE] || entity->sprite == 1 )
			{
				continue;
			}
			if ( entityInsideEntity(this, entity) )
			{
				somebodyinside = true;
				break;
			}
		}
		if ( !somebodyinside )
		{
			this->flags[PASSABLE] = false;
		}
	}
	else if ( this->z < this->gateStartHeight - 9 && !this->flags[PASSABLE] )
	{
		this->flags[PASSABLE] = true;
	}
}
