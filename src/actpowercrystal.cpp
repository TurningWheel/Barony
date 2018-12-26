/*-------------------------------------------------------------------------------

BARONY
File: actpowercrystal.cpp
Desc: behavior function for power crystals

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "monster.hpp"
#include "sound.hpp"
#include "items.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"

/*-------------------------------------------------------------------------------

act*

The following function describes an entity behavior. The function
takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

bool actPowerCrystalBase(Entity* my)
{
	if ( my->flags[PASSABLE] ) // stop the compiler optimising into a different entity.
	{
		my->flags[PASSABLE] = false;
	}

	return true;
}

bool actPowerCrystal(Entity* my)
{
	if ( !my )
	{
		return false;
	}

	return my->actPowerCrystal();
}

bool Entity::actPowerCrystal()
{
	//Entity* entity;
	real_t upper_z = this->crystalStartZ - 0.4;
	real_t lower_z = crystalStartZ + 0.4;
	int i = 0;

	real_t acceleration = 0.95;

	//this->light = lightSphereShadow(this->x / 16, this->y / 16, 3, 64);
	//messagePlayer(0, "vel z: %f", this->vel_z);

	if ( !crystalInitialised && !crystalSpellToActivate )
	{
		if ( this->z > crystalStartZ )
		{
			this->z -= this->vel_z * (1 / acceleration); // start levitating upwards.
		}
		else
		{
			this->z = crystalStartZ;
			this->powerCrystalCreateElectricityNodes();
			crystalInitialised = 1;
		}
	}

	if ( crystalInitialised )
	{
		if ( crystalHoverDirection == CRYSTAL_HOVER_UP ) //rise state
		{
			this->z -= this->vel_z;

			if ( this->z < upper_z )
			{
				this->z = upper_z;
				crystalHoverDirection = CRYSTAL_HOVER_UP_WAIT;
			}

			if ( this->z < crystalStartZ ) //higher than mid point
			{
				this->vel_z = std::max(this->vel_z * acceleration, crystalMinZVelocity);
			}
			else if ( this->z > crystalStartZ ) //lower than midpoint
			{
				this->vel_z = std::min(this->vel_z * (1 / acceleration), crystalMaxZVelocity);
			}
		}
		else if ( crystalHoverDirection == CRYSTAL_HOVER_UP_WAIT ) // wait state
		{
			crystalHoverWaitTimer++;
			if ( crystalHoverWaitTimer >= 1 )
			{
				crystalHoverDirection = CRYSTAL_HOVER_DOWN; // advance state
				crystalHoverWaitTimer = 0; // reset timer
			}
		}
		else if ( crystalHoverDirection == CRYSTAL_HOVER_DOWN ) //fall state
		{
			this->z += this->vel_z;

			if ( this->z > lower_z )
			{
				this->z = lower_z;
				crystalHoverDirection = CRYSTAL_HOVER_DOWN_WAIT;
			}

			if ( this->z < crystalStartZ ) //higher than mid point, start accelerating
			{
				this->vel_z = std::min(this->vel_z * (1 / acceleration), crystalMaxZVelocity);
			}
			else if ( this->z > crystalStartZ ) //lower than midpoint, start decelerating
			{
				this->vel_z = std::max(this->vel_z * acceleration, crystalMinZVelocity);
			}
		}
		else if ( crystalHoverDirection == CRYSTAL_HOVER_DOWN_WAIT ) // wait state
		{
			crystalHoverWaitTimer++;
			if ( crystalHoverWaitTimer >= 1 )
			{
				crystalHoverDirection = CRYSTAL_HOVER_UP; // advance state
				crystalHoverWaitTimer = 0; // reset timer
			}
		}


		if ( this->z <= crystalStartZ + crystalMaxZVelocity && this->z >= crystalStartZ - crystalMaxZVelocity )
		{
			this->vel_z = this->fskill[1]; // reset velocity at the mid point of animation
		}

		spawnAmbientParticles(80, 579, 10 + rand() % 40, 1.0, false);

		if ( crystalTurning == 1 )
		{
			if ( !crystalTurnReverse )
			{
				this->yaw += crystalTurnVelocity; // reverse velocity if turnReverse is 1

				if ( (this->yaw >= (crystalTurnStartDir * (PI / 2)) + (PI / 2)) )
				{
					this->yaw = crystalTurnStartDir * (PI / 2) + (PI / 2);
					crystalTurning = 0;

					if ( this->yaw >= 2 * PI )
					{
						this->yaw = 0;
					}
					this->powerCrystalCreateElectricityNodes();
				}
			}
			else
			{
				this->yaw -= crystalTurnVelocity;// reverse velocity if turnReverse is 1

				if ( (this->yaw <= (crystalTurnStartDir * (PI / 2)) - (PI / 2)) )
				{
					this->yaw = crystalTurnStartDir * (PI / 2) - (PI / 2);
					crystalTurning = 0;

					if ( this->yaw < 0 )
					{
						this->yaw += 2 * PI;
					}
					this->powerCrystalCreateElectricityNodes();
				}
			}
		}
	}

	if ( multiplayer == CLIENT )
	{
		return true;
	}

	// handle player turning the crystal

	for ( i = 0; i < MAXPLAYERS; i++ )
	{
		if ( ((i == 0 && selectedEntity == this) || (client_selected[i] == this)) && crystalTurning == 0 )
		{
			if ( inrange[i] )
			{
				if ( players[i] && players[i]->entity && crystalInitialised )
				{
					playSoundEntity(this, 151, 128);
					crystalTurning = 1;
					crystalTurnStartDir = static_cast<Sint32>(this->yaw / (PI / 2));
					serverUpdateEntitySkill(this, 3);
					serverUpdateEntitySkill(this, 4);
					messagePlayer(i, language[2356]);
				}
				else if ( !crystalInitialised )
				{
					messagePlayer(i, language[2357]);
				}
			}	
		}
	}

	return true;
}

// ambient particle effects.
bool actPowerCrystalParticleIdle(Entity* my)
{
	if ( !my )
	{
		return false;
	}

	if ( my->skill[0] < 0 )
	{
		list_RemoveNode(my->mynode);
		return false;
	}
	else
	{
		--my->skill[0];
		my->z += my->vel_z;
		//my->z -= 0.01;
	}
	return true;
}

void Entity::powerCrystalCreateElectricityNodes()
{
	Entity* entity = nullptr;
	node_t* node = nullptr;
	node_t* nextnode = nullptr;
	real_t xtest = 0;
	real_t ytest = 0;
	
	int i = 0;

	if ( crystalGeneratedElectricityNodes )
	{
		this->mechanismPowerOff(); // turn off my signal
		this->updateCircuitNeighbors(); // update the old wires to depower

		if ( multiplayer != CLIENT )
		{
			for ( node = this->children.first; node != nullptr; node = nextnode )
			{
				nextnode = node->next;
				if ( node->element != nullptr )
				{
					entity = (Entity*)node->element;
					if ( entity->light != nullptr )
					{
						list_RemoveNode(entity->light->node);
					}
					entity->light = nullptr;
					list_RemoveNode(entity->mynode);
				}
				list_RemoveNode(node); // delete all previously generated electricity nodes.
			}
		}
	}

	for ( i = 1; i <= crystalNumElectricityNodes; i++ )
	{
		entity = newEntity(-1, 0, map.entities, nullptr); // electricity node
		xtest = this->x + i * 16 * ((this->yaw == 0) - (this->yaw == PI)); // add/subtract x depending on direction.
		ytest = this->y + i * 16 * ((this->yaw == PI / 2) - (this->yaw == 3 * PI / 2)); // add/subtract y depending on direction.
		
		if ( (static_cast<int>(xtest) >> 4) < 0 || (static_cast<int>(xtest) >> 4) >= map.width || 
			(static_cast<int>(ytest) >> 4) < 0 || (static_cast<int>(ytest) >> 4) >= map.height )
		{
			//messagePlayer(0, "stopped at index %d, x: %d, y: %d", i, (static_cast<int>(xtest) >> 4), (static_cast<int>(ytest) >> 4));
			break; // stop generating more nodes as we are out of bounds
		}
		
		//messagePlayer(0, "gen at index %d", i);
		entity->x = xtest;
		entity->y = ytest;
		entity->z = 5;
		entity->behavior = &actCircuit;
		entity->flags[PASSABLE] = true;
		entity->flags[INVISIBLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->circuit_status = CIRCUIT_OFF; //It's a depowered powerable.

		node = list_AddNodeLast(&this->children);
		node->element = entity; // add the node to the children list.
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);

		TileEntityList.addEntity(*entity); // make sure new nodes are added to the tile list to properly update neighbors.

		this->crystalGeneratedElectricityNodes = 1;
	}
	
	this->mechanismPowerOn();
	this->updateCircuitNeighbors();

	return;
}
