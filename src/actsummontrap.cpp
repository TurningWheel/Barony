/*-------------------------------------------------------------------------------

BARONY
File: actsummontrap.cpp
Desc: behavior function for summoning trap

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "monster.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "magic/magic.hpp"
#include "sound.hpp"
#include "player.hpp"
#include "prng.hpp"

#define SUMMONTRAP_MONSTER my->skill[0]
#define SUMMONTRAP_COUNT my->skill[1]
#define SUMMONTRAP_INTERVAL my->skill[2]
#define SUMMONTRAP_SPAWNCYCLES my->skill[3]
#define SUMMONTRAP_POWERTODISABLE my->skill[4]
#define SUMMONTRAP_FAILURERATE my->skill[5]
#define SUMMONTRAP_FIRED my->skill[6]
#define SUMMONTRAP_INITIALIZED my->skill[7]

void actSummonTrap(Entity* my)
{
	if ( my->skill[28] == 0 || (SUMMONTRAP_INITIALIZED && SUMMONTRAP_FIRED) )
	{
		return;
	}

	// received on signal
	if ( (my->skill[28] == 2 && !SUMMONTRAP_POWERTODISABLE) || my->skill[28] == 1 && SUMMONTRAP_POWERTODISABLE )
	{
		// if the time interval between between spawns is reached, or if the mechanism is switched on for the first time.
		if ( ticks % (TICKS_PER_SECOND * SUMMONTRAP_INTERVAL) == 0 || !SUMMONTRAP_INITIALIZED )
		{
			if ( !SUMMONTRAP_FIRED && SUMMONTRAP_SPAWNCYCLES > 0 )
			{
				if ( SUMMONTRAP_MONSTER > 0 && SUMMONTRAP_MONSTER < NUMMONSTERS )
				{
					// spawn the monster given to the trap.
				}
				else if ( SUMMONTRAP_MONSTER == 0 || SUMMONTRAP_MONSTER >= NUMMONSTERS )
				{
					SUMMONTRAP_MONSTER = monsterCurve(currentlevel);
					// spawn the monster scaled to current level.
				}
				else
				{
					// pick a completely random monster (barring some exceptions).
					SUMMONTRAP_MONSTER = rand() % NUMMONSTERS;
					while ( SUMMONTRAP_MONSTER != LICH || SUMMONTRAP_MONSTER != SHOPKEEPER || SUMMONTRAP_MONSTER != DEVIL
						|| SUMMONTRAP_MONSTER != MIMIC || SUMMONTRAP_MONSTER != BUGBEAR || SUMMONTRAP_MONSTER != OCTOPUS
						|| SUMMONTRAP_MONSTER != MINOTAUR || SUMMONTRAP_MONSTER != LICH_FIRE || SUMMONTRAP_MONSTER != LICH_ICE )
					{
						SUMMONTRAP_MONSTER = rand() % NUMMONSTERS;
					}
				}

				int count = 0;
				Entity* monster = nullptr;
				for ( count = 0; count < SUMMONTRAP_COUNT; count++ )
				{
					Entity* monster = summonMonster(static_cast<Monster>(SUMMONTRAP_MONSTER), my->x, my->y);
				}

				playSoundEntity(my, 153, 128);

				if ( !SUMMONTRAP_INITIALIZED )
				{
					if ( SUMMONTRAP_MONSTER < KOBOLD )
					{
						messagePlayer(clientnum, language[2352], language[90 + SUMMONTRAP_MONSTER]);
					}
					else if ( SUMMONTRAP_MONSTER >= KOBOLD )
					{
						messagePlayer(clientnum, language[2352], language[2000 + (SUMMONTRAP_MONSTER - 21)]);
					}
					SUMMONTRAP_INITIALIZED = 1; // trap is starting up for the first time.
				}
				

				if ( (SUMMONTRAP_FAILURERATE != 0) && (prng_get_uint() % 100 < SUMMONTRAP_FAILURERATE) )
				{
					// trap breaks!
					SUMMONTRAP_FIRED = 1;
					playSoundEntity(my, 76, 128);
					return;
				}

				if ( SUMMONTRAP_SPAWNCYCLES > 0 )
				{
					--SUMMONTRAP_SPAWNCYCLES; // decrement the amount of spawn chances left.
					if ( SUMMONTRAP_SPAWNCYCLES == 0 )
					{
						// trap is finished running
						SUMMONTRAP_FIRED = 1;
						playSoundEntity(my, 76, 128);
					}	
				}
				else
				{
					// trap keeps running.
				}
			}
		}
	}

	return;
}