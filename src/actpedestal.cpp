/*-------------------------------------------------------------------------------

BARONY
File: actpowerorb.cpp
Desc: behavior function for power orbs

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

void actPedestalBase(Entity* my)
{
	if ( !my )
	{
		return;
	}

	my->actPedestalBase();
}

void actPedestalOrb(Entity* my)
{
	if ( !my )
	{
		return;
	}

	my->actPedestalOrb();
}

void Entity::actPedestalBase()
{
	//Entity* entity;

	if ( multiplayer == CLIENT )
	{
		return;
	}

	// handle player interaction
	for ( int i = 0; i < MAXPLAYERS; i++ )
	{
		if ( ((i == 0 && selectedEntity == this) || (client_selected[i] == this)))
		{
			if ( inrange[i] )
			{
				if ( players[i] && players[i]->entity )
				{
					/*playSoundEntity(this, 151, 128);
					orbTurning = 1;
					orbTurnStartDir = static_cast<Sint32>(this->yaw / (PI / 2));
					serverUpdateEntitySkill(this, 3);
					serverUpdateEntitySkill(this, 4);*/
					messagePlayer(i, language[2364]);
					if ( circuit_status == CIRCUIT_OFF )
					{
						mechanismPowerOn();
						updateCircuitNeighbors();
					}
					else
					{
						mechanismPowerOff();
						updateCircuitNeighbors();
					}
				}
				/*else if ( !orbInitialised )
				{
					messagePlayer(i, language[2357]);
				}*/
			}	
		}
	}

	return;
}

//// ambient particle effects.
//void actPowerCrystalParticleIdle(Entity* my)
//{
//	if ( !my )
//	{
//		return;
//	}
//
//	if ( my->skill[0] < 0 )
//	{
//		list_RemoveNode(my->mynode);
//		return;
//	}
//	else
//	{
//		--my->skill[0];
//		my->z += my->vel_z;
//		//my->z -= 0.01;
//	}
//	return;
//}
//
//void Entity::pedestalCreateElectricityNodes()
//{
//	this->mechanismPowerOn();
//	this->updateCircuitNeighbors();
//
//	return;
//}

void Entity::actPedestalOrb()
{
	real_t upper_z = orbStartZ - 0.4;
	real_t lower_z = orbStartZ + 0.4;
	int i = 0;

	real_t acceleration = 0.95;

	pedestalOrbInit();

	if ( orbHoverDirection == CRYSTAL_HOVER_UP ) //rise state
	{
		z -= vel_z;

		if ( z < upper_z )
		{
			z = upper_z;
			orbHoverDirection = CRYSTAL_HOVER_UP_WAIT;
		}

		if ( z < orbStartZ ) //higher than mid point
		{
			vel_z = std::max(vel_z * acceleration, orbMinZVelocity);
		}
		else if ( z > orbStartZ ) //lower than midpoint
		{
			vel_z = std::min(vel_z * (1 / acceleration), orbMaxZVelocity);
		}
	}
	else if ( orbHoverDirection == CRYSTAL_HOVER_UP_WAIT ) // wait state
	{
		orbHoverWaitTimer++;
		if ( orbHoverWaitTimer >= 1 )
		{
			orbHoverDirection = CRYSTAL_HOVER_DOWN; // advance state
			orbHoverWaitTimer = 0; // reset timer
		}
	}
	else if ( orbHoverDirection == CRYSTAL_HOVER_DOWN ) //fall state
	{
		z += vel_z;

		if ( z > lower_z )
		{
			z = lower_z;
			orbHoverDirection = CRYSTAL_HOVER_DOWN_WAIT;
		}

		if ( z < orbStartZ ) //higher than mid point, start accelerating
		{
			vel_z = std::min(vel_z * (1 / acceleration), orbMaxZVelocity);
		}
		else if ( z > orbStartZ ) //lower than midpoint, start decelerating
		{
			vel_z = std::max(vel_z * acceleration, orbMinZVelocity);
		}
	}
	else if ( orbHoverDirection == CRYSTAL_HOVER_DOWN_WAIT ) // wait state
	{
		orbHoverWaitTimer++;
		if ( orbHoverWaitTimer >= 1 )
		{
			orbHoverDirection = CRYSTAL_HOVER_UP; // advance state
			orbHoverWaitTimer = 0; // reset timer
		}
	}


	if ( z <= orbStartZ + orbMaxZVelocity && z >= orbStartZ - orbMaxZVelocity )
	{
		vel_z = orbMaxZVelocity; // reset velocity at the mid point of animation
	}

	yaw += orbTurnVelocity;
	int particleSprite = 606;

	switch ( sprite )
	{
		case 602:
			particleSprite = 606;
			break;
		case 603:
			particleSprite = 607;
			break;
		case 604:
			particleSprite = 608;
			break;
		case 605:
			particleSprite = 609;
			break;
		default:
			break;
	}
	spawnAmbientParticles(80, particleSprite, 10 + rand() % 40, 1.0, false);
}

void Entity::pedestalOrbInit()
{
	if ( !orbInitialised )
	{
		Entity* parent = uidToEntity(this->parent);
		x = parent->x;
		y = parent->y;
		z = -2;
		sizex = 2;
		sizey = 2;
		flags[PASSABLE] = false;
		orbStartZ = z;
		z = orbStartZ - 0.4 + ((rand() % 8) * 0.1); // start the height randomly
		orbMaxZVelocity = 0.02; //max velocity
		orbMinZVelocity = 0.001; //min velocity
		vel_z = crystalMaxZVelocity * ((rand() % 100) * 0.01); // start the velocity randomly
		orbTurnVelocity = 0.02;
		orbInitialised = 1;
	}
}

