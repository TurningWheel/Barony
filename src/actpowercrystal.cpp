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

void actPowerCrystalBase(Entity* my)
{
	Entity* entity;

	// the rest of the function is server-side.
	if ( multiplayer == CLIENT )
	{
		return;
	}

	return;
}

void actPowerCrystal(Entity* my)
{
	if ( !my )
	{
		return;
	}

	my->actPowerCrystal();
}

void Entity::actPowerCrystal()
{
	//Entity* entity;
	real_t upper_z = this->crystalStartZ - 0.4;
	real_t lower_z = crystalStartZ + 0.4;
	int i = 0;

	real_t acceleration = 0.95;

	//this->light = lightSphereShadow(this->x / 16, this->y / 16, 3, 64);
	//messagePlayer(0, "vel z: %f", this->vel_z);

	if ( !crystalInitialised )
	{
		this->powerCrystalCreateElectricityNodes();
		crystalInitialised = 1;
	}

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

	if ( rand() % 80 == 0 )
	{
		Entity* spawnParticle = newEntity(579, 1, map.entities);
		spawnParticle->sizex = 1;
		spawnParticle->sizey = 1;
		spawnParticle->x = this->x + (-2 + rand() % 5);
		spawnParticle->y = this->y + (-2 + rand() % 5);
		spawnParticle->z = 7.5;
		spawnParticle->vel_z = -1;
		spawnParticle->skill[0] = 10 + rand() % 40;
		spawnParticle->behavior = &actPowerCrystalParticleIdle;
		spawnParticle->flags[PASSABLE] = true;
		spawnParticle->setUID(-3);
	}


	if ( crystalTurning == 1 )
	{
		this->yaw += crystalTurnVelocity;

		if ( this->yaw >= (crystalTurnStartDir * (PI / 2)) + (PI / 2) )
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

	if ( multiplayer == CLIENT )
	{
		return;
	}

	// handle player turning the crystal

	for ( i = 0; i < MAXPLAYERS; i++ )
	{
		if ( ((i == 0 && selectedEntity == this) || (client_selected[i] == this)) && crystalTurning == 0 )
		{
			if ( inrange[i] )
			{
				if ( players[i] && players[i]->entity )
				{
					playSoundEntity(this, 151, 128);
					crystalTurning = 1;
					crystalTurnStartDir = static_cast<Sint32>(this->yaw / (PI / 2));
					serverUpdateEntitySkill(this, 3);
					serverUpdateEntitySkill(this, 4);
				}
			}
		}
	}

	return;
}

// ambient particle effects.
void actPowerCrystalParticleIdle(Entity* my)
{
	if ( my->skill[0] < 0 )
	{
		list_RemoveNode(my->mynode);
		return;
	}
	else
	{
		--my->skill[0];
		my->z += my->vel_z;
		//my->z -= 0.01;
	}
	return;
}

void Entity::powerCrystalCreateElectricityNodes()
{
	Entity* entity = nullptr;
	node_t* node = nullptr;
	node_t* nextnode = nullptr;
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
				//messagePlayer(0, "Deleted node");
			}
		}
	}

	for ( i = 1; i <= crystalNumElectricityNodes; i++ )
	{
		entity = newEntity(-1, 1, map.entities); // electricity node
		entity->x = this->x + i * 16 * ((this->yaw == 0) - (this->yaw == PI)); // add/subtract x depending on direction.
		entity->y = this->y + i * 16 * ((this->yaw == PI / 2) - (this->yaw == 3 * PI / 2)); // add/subtract y depending on direction.
		entity->z = 5;
		entity->behavior = &actCircuit;
		entity->flags[PASSABLE] = true;
		entity->flags[INVISIBLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->circuit_status = CIRCUIT_OFF; //It's a depowered powerable.

		node = list_AddNodeLast(&this->children);
		node->element = entity; // add the node to the children list.
		node->deconstructor = &entityDeconstructor;
		node->size = sizeof(Entity*);

		crystalGeneratedElectricityNodes = 1;
	}
	
	this->mechanismPowerOn();
	this->updateCircuitNeighbors();

	return;
}