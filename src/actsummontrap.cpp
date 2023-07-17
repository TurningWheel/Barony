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
#include "engine/audio/sound.hpp"
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
#define SUMMONTRAP_TICKS_TO_FIRE my->skill[8]

void actSummonTrap(Entity* my)
{
	if ( my->skill[28] == 0 || (SUMMONTRAP_INITIALIZED && SUMMONTRAP_FIRED) )
	{
		return;
	}

	// received on signal
	if ( (my->skill[28] == 2 && !SUMMONTRAP_POWERTODISABLE) || my->skill[28] == 1 && SUMMONTRAP_POWERTODISABLE )
	{
		if ( SUMMONTRAP_TICKS_TO_FIRE > 0 )
		{
			--SUMMONTRAP_TICKS_TO_FIRE;
		}

		// if the time interval between between spawns is reached, or if the mechanism is switched on for the first time.
		if ( SUMMONTRAP_TICKS_TO_FIRE == 0 || !SUMMONTRAP_INITIALIZED )
		{
			SUMMONTRAP_TICKS_TO_FIRE = (TICKS_PER_SECOND * SUMMONTRAP_INTERVAL);

			if ( !SUMMONTRAP_FIRED && SUMMONTRAP_SPAWNCYCLES > 0 )
			{
				bool useCustomMonsters = monsterCurveCustomManager.curveExistsForCurrentMapName(map.name);
				bool fixedCustomMonster = true;
				Monster customMonsterType = NOTHING;
				bool pickedRandomMonsters = (SUMMONTRAP_MONSTER == -1);

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
					// pick a completely random monster (barring some exceptions)
					const std::set<Monster> typesToSkip
					{
	                    LICH, SHOPKEEPER, DEVIL, MIMIC, CRAB, OCTOPUS,
	                    MINOTAUR, LICH_FIRE, LICH_ICE, NOTHING,
	                    SENTRYBOT, SPELLBOT, GYROBOT, DUMMYBOT
	                };
	                
					std::vector<Monster> possibleTypes;
					for ( int i = 0; i < NUMMONSTERS; ++i )
					{
						const Monster mon = static_cast<Monster>(i);
						if ( typesToSkip.find(mon) == typesToSkip.end() )
						{
							possibleTypes.push_back(mon);
						}
					}
	                SUMMONTRAP_MONSTER = possibleTypes.at(local_rng.rand() % possibleTypes.size());
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
					if ( monster && monster->getStats() )
					{
						if ( useCustomMonsters )
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
						else
						{
							monster->getStats()->MISC_FLAGS[STAT_FLAG_DISABLE_MINIBOSS] = 1; // disable champion normally.
						}
					}
				}

				playSoundEntity(my, 153, 128);

				if ( !SUMMONTRAP_INITIALIZED )
				{
					if ( SUMMONTRAP_MONSTER < KOBOLD )
					{
						//messagePlayer(clientnum, Language::get(2352), Language::get(90 + SUMMONTRAP_MONSTER));
					}
					else if ( SUMMONTRAP_MONSTER >= KOBOLD )
					{
						//messagePlayer(clientnum, Language::get(2352), Language::get(2000 + (SUMMONTRAP_MONSTER - 21)));
					}
					SUMMONTRAP_INITIALIZED = 1; // trap is starting up for the first time.
				}

				if ( pickedRandomMonsters )
				{
					SUMMONTRAP_MONSTER = -1;
				}

				if ( (SUMMONTRAP_FAILURERATE != 0) && (local_rng.rand() % 100 < SUMMONTRAP_FAILURERATE) )
				{
					// trap breaks!
					SUMMONTRAP_FIRED = 1;
					//playSoundEntity(my, 76, 128);
					return;
				}

				if ( SUMMONTRAP_SPAWNCYCLES > 0 )
				{
					--SUMMONTRAP_SPAWNCYCLES; // decrement the amount of spawn chances left.
					if ( SUMMONTRAP_SPAWNCYCLES == 0 )
					{
						// trap is finished running
						SUMMONTRAP_FIRED = 1;
						//playSoundEntity(my, 76, 128);
					}	
				}
				else
				{
					// trap keeps running.
				}
			}
		}
	}
	else
	{
		SUMMONTRAP_TICKS_TO_FIRE = 0;
	}
	return;
}
