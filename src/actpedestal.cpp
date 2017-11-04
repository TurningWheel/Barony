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
	node_t* node = children.first;
	Entity* orbEntity = (Entity*)(node->element);

	if ( pedestalInGround != 0 )
	{
		if ( z > 4.5 )
		{
			vel_z = -0.05;
			z += vel_z;
			orbEntity->vel_z = vel_z;
			orbEntity->z += orbEntity->vel_z;
		}
		else
		{
			z = 4.5;
			orbEntity->z = -2;
			vel_z = 0;
			orbEntity->vel_z = 0;
			pedestalInGround = 0;
			if ( multiplayer != CLIENT )
			{
				serverUpdateEntitySkill(this, 4);
			}
		}
	}

	if ( multiplayer == CLIENT )
	{
		return;
	}

	if ( circuit_status < CIRCUIT_OFF )
	{
		// set the entity to be a circuit if not already set.
		if ( !pedestalInvertedPower )
		{
			circuit_status = CIRCUIT_OFF; 
		}
		else
		{
			circuit_status = CIRCUIT_ON;
		}
	}

	if ( pedestalHasOrb == pedestalOrbType )
	{
		// power on/off the circuit if it hasn't updated
		if ( circuit_status == CIRCUIT_OFF && !pedestalInvertedPower )
		{
			mechanismPowerOn();
			updateCircuitNeighbors();
		}
		else if ( circuit_status == CIRCUIT_ON && pedestalInvertedPower )
		{
			mechanismPowerOff();
			updateCircuitNeighbors();
		}
	}

	if ( flags[PASSABLE] && pedestalInGround == 0 )
	{
		// see if any entity is currently inside, otherwise set PASSABLE to false
		bool somebodyInside = false;
		node_t* node2 = nullptr;
		for ( node2 = map.entities->first; node2 != nullptr; node2 = node2->next )
		{
			Entity* entity = (Entity*)node2->element;
			if ( entity == this || entity->flags[PASSABLE]
				|| entity->sprite == 1 || entity == orbEntity )
			{
				continue;
			}
			if ( entityInsideEntity(this, entity) )
			{
				somebodyInside = true;
				break;
			}
		}
		if ( !somebodyInside )
		{
			flags[PASSABLE] = false;
			serverUpdateEntityFlag(this, PASSABLE);
		}
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
					if ( orbEntity && pedestalHasOrb > 0 )
					{
						Item* itemOrb = newItem(static_cast<ItemType>(ARTIFACT_ORB_BLUE + pedestalHasOrb - 1), EXCELLENT, 0, 1, rand(), true, nullptr);
						itemPickup(i, itemOrb);
						if ( pedestalHasOrb == pedestalOrbType )
						{
							// only update power when right orb is in place.
							if ( !pedestalInvertedPower )
							{
								mechanismPowerOff();
							}
							else
							{
								mechanismPowerOn();
							}
							updateCircuitNeighbors();
							removeLightField();
						}
						pedestalHasOrb = 0;
						serverUpdateEntitySkill(this, 0); // update orb status.
						messagePlayer(i, language[2374], itemOrb->getName());
					}
					else
					{
						messagePlayer(i, language[2364]);
					}
				}
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

	Entity* parent = uidToEntity(this->parent);
	if ( !parent )
	{
		return;
	}

	if ( !parent->pedestalInGround )
	{
		pedestalOrbInit();
	}

	if ( parent->pedestalHasOrb == 0 )
	{
		flags[INVISIBLE] = true;
		flags[UNCLICKABLE] = true;
		flags[PASSABLE] = true;
		orbTurnVelocity = 0.5; // reset the speed of the orb.
		return;
	}
	else if ( orbInitialised )
	{
		sprite = parent->pedestalHasOrb + 602 - 1;

		// handle player interaction
		if ( multiplayer != CLIENT )
		{
			for ( int i = 0; i < MAXPLAYERS; i++ )
			{
				if ( ((i == 0 && selectedEntity == this) || (client_selected[i] == this)) )
				{
					if ( inrange[i] )
					{
						if ( players[i] && players[i]->entity )
						{
							if ( parent->pedestalHasOrb > 0 )
							{
								Item* itemOrb = newItem(static_cast<ItemType>(ARTIFACT_ORB_BLUE + parent->pedestalHasOrb - 1), EXCELLENT, 0, 1, rand(), true, nullptr);
								itemPickup(i, itemOrb);
								if ( parent->pedestalHasOrb == parent->pedestalOrbType )
								{
									// only update power when right orb is in place.
									if ( !pedestalInvertedPower )
									{
										parent->mechanismPowerOff();
									}
									else
									{
										parent->mechanismPowerOn();
									}
									updateCircuitNeighbors();
									removeLightField();
								}
								parent->pedestalHasOrb = 0;
								serverUpdateEntitySkill(parent, 0); // update orb status 
								messagePlayer(i, language[2374], itemOrb->getName());
							}
						}
					}
				}
			}
		}

		if ( parent->pedestalHasOrb != parent->pedestalOrbType )
		{
			// not properly activated - return early, no animate.
			flags[INVISIBLE] = false;
			flags[UNCLICKABLE] = false;
			flags[PASSABLE] = false;
			return;
		}
		else if ( parent->pedestalHasOrb == parent->pedestalOrbType )
		{
			flags[INVISIBLE] = false;
			flags[UNCLICKABLE] = false;
			flags[PASSABLE] = false;
		}
	}
	else
	{
		return;
	}

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

	yaw += (orbTurnVelocity * abs((vel_z / orbMaxZVelocity)) + orbTurnVelocity);
	if ( orbTurnVelocity > 0.04 )
	{
		orbTurnVelocity -= 0.01;
	}

	int particleSprite = 606;

	switch ( sprite )
	{
		case 602:
		case 603:
		case 604:
		case 605:
			particleSprite = sprite + 4;
			break;
		default:
			particleSprite = -1;
			break;
	}
	spawnAmbientParticles(40, particleSprite, 10 + rand() % 40, 1.0, false);
}

void Entity::pedestalOrbInit()
{
	Entity* parent = uidToEntity(this->parent);

	if ( !orbInitialised && !parent->pedestalInGround )
	{
		x = parent->x;
		y = parent->y;
		z = -2;
		sizex = 2;
		sizey = 2;
		if ( parent->pedestalHasOrb == parent->pedestalOrbType )
		{
			flags[UNCLICKABLE] = false;
			flags[INVISIBLE] = false;
		}
		else
		{
			flags[UNCLICKABLE] = true;
			flags[INVISIBLE] = true;
		}
		flags[PASSABLE] = true;
		if ( orbStartZ != z )
		{
			orbStartZ = z;
			z = orbStartZ - 0.4 + ((rand() % 8) * 0.1); // start the height randomly
		}
		orbMaxZVelocity = 0.02; //max velocity
		orbMinZVelocity = 0.001; //min velocity
		vel_z = crystalMaxZVelocity * ((rand() % 100) * 0.01); // start the velocity randomly
		orbTurnVelocity = 0.5;
		orbInitialised = 1;
		if ( multiplayer != CLIENT )
		{
			serverUpdateEntitySkill(parent, 0);
			serverUpdateEntitySkill(parent, 1);
		}
	}
}

