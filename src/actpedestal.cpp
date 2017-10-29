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

void actPedestalBase(Entity* my)
{
	if ( !my )
	{
		return;
	}

	my->actPedestalBase();
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
					crystalTurning = 1;
					crystalTurnStartDir = static_cast<Sint32>(this->yaw / (PI / 2));
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
				/*else if ( !crystalInitialised )
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
