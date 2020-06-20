/*-------------------------------------------------------------------------------

BARONY
File: actsummontrap.cpp
Desc: behavior function for summoning trap

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

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
#include "mod_tools.hpp"

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
				bool useCustomMonsters = monsterCurveCustomManager.curveExistsForCurrentMapName(map.name);
				bool fixedCustomMonster = true;
				Monster customMonsterType = NOTHING;
				if ( SUMMONTRAP_MONSTER > 0 && SUMMONTRAP_MONSTER < NUMMONSTERS )
				{
					// spawn the monster given to the trap.
				}
				else if ( SUMMONTRAP_MONSTER == 0 || SUMMONTRAP_MONSTER >= NUMMONSTERS )
				{
					// spawn the monster scaled to current level.
					if ( useCustomMonsters )
					{
						fixedCustomMonster = false;
						customMonsterType = static_cast<Monster>(monsterCurveCustomManager.rollMonsterFromCurve(map.name));
					}
					else
					{
						SUMMONTRAP_MONSTER = monsterCurve(currentlevel);
					}
				}
				else
				{
					// pick a completely random monster (barring some exceptions).
					SUMMONTRAP_MONSTER = rand() % NUMMONSTERS;
					while ( SUMMONTRAP_MONSTER == LICH || SUMMONTRAP_MONSTER == SHOPKEEPER || SUMMONTRAP_MONSTER == DEVIL
						|| SUMMONTRAP_MONSTER == MIMIC || SUMMONTRAP_MONSTER == BUGBEAR || SUMMONTRAP_MONSTER == OCTOPUS
						|| SUMMONTRAP_MONSTER == MINOTAUR || SUMMONTRAP_MONSTER == LICH_FIRE || SUMMONTRAP_MONSTER == LICH_ICE
						|| SUMMONTRAP_MONSTER == NOTHING || SUMMONTRAP_MONSTER == SENTRYBOT || SUMMONTRAP_MONSTER == SPELLBOT
						|| SUMMONTRAP_MONSTER == GYROBOT || SUMMONTRAP_MONSTER == DUMMYBOT )
					{
						SUMMONTRAP_MONSTER = rand() % NUMMONSTERS;
					}
				}

				int count = 0;
				for ( count = 0; count < SUMMONTRAP_COUNT; count++ )
				{
					Entity* monster = nullptr;
					int typeToSpawn = SUMMONTRAP_MONSTER;
					if ( useCustomMonsters && customMonsterType != NOTHING )
					{
						typeToSpawn = customMonsterType;
					}
					monster = summonMonster(static_cast<Monster>(typeToSpawn), my->x, my->y);
					if ( monster && useCustomMonsters )
					{
						std::string variantName = "default";
						if ( fixedCustomMonster )
						{
							variantName = monsterCurveCustomManager.rollFixedMonsterVariant(map.name, typeToSpawn);
						}
						else
						{
							variantName = monsterCurveCustomManager.rollMonsterVariant(map.name, typeToSpawn);
						}
						if ( variantName.compare("default") != 0 )
						{
							Monster tmp = NOTHING;
							monsterCurveCustomManager.createMonsterFromFile(monster, monster->getStats(), variantName, tmp);
						}
					}
				}

				playSoundEntity(my, 153, 128);

				if ( !SUMMONTRAP_INITIALIZED )
				{
					if ( SUMMONTRAP_MONSTER < KOBOLD )
					{
						//messagePlayer(clientnum, language[2352], language[90 + SUMMONTRAP_MONSTER]);
					}
					else if ( SUMMONTRAP_MONSTER >= KOBOLD )
					{
						//messagePlayer(clientnum, language[2352], language[2000 + (SUMMONTRAP_MONSTER - 21)]);
					}
					SUMMONTRAP_INITIALIZED = 1; // trap is starting up for the first time.
				}
				

				if ( (SUMMONTRAP_FAILURERATE != 0) && (rand() % 100 < SUMMONTRAP_FAILURERATE) )
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
