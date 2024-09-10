/*-------------------------------------------------------------------------------

	BARONY
	File: actarrowtrap.cpp
	Desc: implements arrow trap code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "engine/audio/sound.hpp"
#include "collision.hpp"
#include "items.hpp"
#include "net.hpp"
#include "prng.hpp"
#include "mod_tools.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define ARROWTRAP_FIRED my->skill[0]
#define ARROWTRAP_TYPE my->skill[1]
#define ARROWTRAP_REFIRE my->skill[3]
#define ARROWTRAP_AMBIENCE my->skill[6]
#define ARROWTRAP_DISABLED my->skill[4]

void actArrowTrap(Entity* my)
{
	int x, y;
	int c;

	// eliminate arrow traps that have been destroyed.
	// check wall inside me.
	int checkx = static_cast<int>(my->x) >> 4;
	int checky = static_cast<int>(my->y) >> 4;
	if ( !map.tiles[OBSTACLELAYER + checky * MAPLAYERS + checkx * MAPLAYERS * map.height] )   // wall
	{
		list_RemoveNode(my->mynode);
		return;
	}
	if ( ARROWTRAP_FIRED >= 10 ) // shot my piece, time to die.
	{
		list_RemoveNode(my->mynode);
		return;
	}
	if ( ARROWTRAP_DISABLED > 0 )
	{
		if ( multiplayer != CLIENT )
		{
			ItemType quiver = static_cast<ItemType>(ARROWTRAP_TYPE);
			int qty = 2 + (5 - ARROWTRAP_FIRED / 2); // 2 to 7
			Compendium_t::Events_t::eventUpdateWorld(ARROWTRAP_DISABLED - 1, Compendium_t::CPDM_ARROWS_PILFERED, "arrow trap", qty);
			Entity* dropped = dropItemMonster(newItem(quiver, SERVICABLE, 0, qty, ITEM_GENERATED_QUIVER_APPEARANCE, false, nullptr), my, nullptr, qty);
			std::vector<std::pair<int, int>> freeTiles;
			int x = my->x / 16;
			int y = my->y / 16;
			if ( (x + 1) >= 0 && (x + 1) < map.width && y >= 0 && y < map.height )
			{
				if ( !map.tiles[OBSTACLELAYER + y * MAPLAYERS + (x + 1) * MAPLAYERS * map.height] )
				{
					freeTiles.push_back(std::make_pair(x + 1, y));
				}
			}
			if ( (x - 1) >= 0 && (x - 1) < map.width && y >= 0 && y < map.height )
			{
				if ( !map.tiles[OBSTACLELAYER + y * MAPLAYERS + (x - 1) * MAPLAYERS * map.height] )
				{
					freeTiles.push_back(std::make_pair(x - 1, y));
				}
			}
			if ( x >= 0 && x < map.width && (y + 1) >= 0 && (y + 1) < map.height )
			{
				if ( !map.tiles[OBSTACLELAYER + (y + 1) * MAPLAYERS + x * MAPLAYERS * map.height] )
				{
					freeTiles.push_back(std::make_pair(x, y + 1));
				}
			}
			if ( x >= 0 && x < map.width && (y - 1) >= 0 && (y - 1) < map.height )
			{
				if ( !map.tiles[OBSTACLELAYER + (y - 1) * MAPLAYERS + x * MAPLAYERS * map.height] )
				{
					freeTiles.push_back(std::make_pair(x, y - 1));
				}
			}
			if ( !freeTiles.empty() )
			{
				std::pair<int, int> chosenTile = freeTiles[local_rng.rand() % freeTiles.size()];
				dropped->x += (chosenTile.first - x) * 8;
				dropped->y += (chosenTile.second - y) * 8;
				dropped->vel_x = (chosenTile.first - x);
				if ( abs(dropped->vel_x) > 0.01 )
				{
					dropped->vel_x *= (5 + local_rng.rand() % 11) / 10.0; //50% to 150%
				}
				dropped->vel_y = (chosenTile.second - y);
				if ( abs(dropped->vel_y) > 0.01 )
				{
					dropped->vel_y *= (5 + local_rng.rand() % 11) / 10.0; //50% to 150%
				}
			}
		}
		list_RemoveNode(my->mynode);
		return;
	}

#ifdef USE_FMOD
	if ( ARROWTRAP_AMBIENCE == 0 )
	{
		ARROWTRAP_AMBIENCE--;
		my->entity_sound = playSoundEntityLocal(my, 149, 64);
	}
	if ( my->entity_sound )
	{
		bool playing = false;
		my->entity_sound->isPlaying(&playing);
		if ( !playing )
		{
			my->entity_sound = nullptr;
		}
	}
#else
	ARROWTRAP_AMBIENCE--;
	if ( ARROWTRAP_AMBIENCE <= 0 )
	{
		ARROWTRAP_AMBIENCE = TICKS_PER_SECOND * 30;
		playSoundEntityLocal( my, 149, 64 );
	}
#endif

	if ( !my->skill[28] )
	{
		return;
	}

	Entity* targetToAutoHit = nullptr;

	// received on signal
	if ( my->skill[28] == 2 || ARROWTRAP_DISABLED == -1 )
	{
		if ( ARROWTRAP_FIRED % 2 == 1 ) // not ready to fire.
		{
			if ( ARROWTRAP_TYPE == QUIVER_LIGHTWEIGHT || (gameModeManager.currentSession.challengeRun.isActive(GameModeManager_t::CurrentSession_t::ChallengeRun_t::CHEVENT_STRONG_TRAPS)) )
			{
				if ( ARROWTRAP_REFIRE > 0 )
				{
					--ARROWTRAP_REFIRE;
					if ( ARROWTRAP_REFIRE == 0 )
					{
						++ARROWTRAP_FIRED;
					}
				}
			}
		}
		if ( ARROWTRAP_DISABLED == -1 )
		{
			ARROWTRAP_DISABLED = 0;
			if ( ARROWTRAP_FIRED % 2 == 1 )
			{
				ARROWTRAP_FIRED++;
			}
			ARROWTRAP_REFIRE = 0;
			// misfire from a lockpick, try to find a nearby target.
			for ( node_t* node = map.creatures->first; node != nullptr; node = node->next )
			{
				Entity* entity = (Entity*)node->element;
				if ( entity && entity->behavior == &actPlayer && entityDist(my, entity) < TOUCHRANGE )
				{
					targetToAutoHit = entity;
					break;
				}
			}
		}

		if ( ARROWTRAP_FIRED % 2 == 0 && ARROWTRAP_REFIRE <= 0 )
		{
			ARROWTRAP_FIRED++;
			ARROWTRAP_REFIRE = 5;
			for ( c = 0; c < 4; c++ )
			{
				switch ( c )
				{
					case 0:
						x = 12;
						y = 0;
						break;
					case 1:
						x = 0;
						y = 12;
						break;
					case 2:
						x = -12;
						y = 0;
						break;
					case 3:
						x = 0;
						y = -12;
						break;
				}
				int checkx = (my->x + x) / 16;
				int checky = (my->y + y) / 16;
				if ( !(checkx >= 0 && checkx < map.width && checky >= 0 && checky < map.height) )
				{
					// out of bounds.
					continue;
				}
				int index = checky * MAPLAYERS + checkx * MAPLAYERS * map.height;
				if ( !map.tiles[OBSTACLELAYER + index] )
				{
					Entity* entity = newEntity(166, 1, map.entities, nullptr); // arrow
					playSoundEntity(my, 239 + local_rng.rand() % 3, 96);
					entity->parent = my->getUID();
					entity->x = my->x + x;
					entity->y = my->y + y;
					entity->z = my->z;
					entity->yaw = c * (PI / 2.f);
					entity->sizex = 1;
					entity->sizey = 1;
					entity->behavior = &actArrow;
					entity->flags[UPDATENEEDED] = true;
					entity->flags[PASSABLE] = true;

					// arrow power
					entity->arrowPower = 17;
					if ( currentlevel >= 10 )
					{
						entity->arrowPower += currentlevel - 10;
					}
					bool stronger = false;
					if ( gameModeManager.currentSession.challengeRun.isActive(GameModeManager_t::CurrentSession_t::ChallengeRun_t::CHEVENT_STRONG_TRAPS) )
					{
						stronger = true;
					}
					switch ( ARROWTRAP_TYPE )
					{
						case QUIVER_SILVER:
							entity->sprite = 924;
							if ( stronger ) { ARROWTRAP_REFIRE = 50; }
							break;
						case QUIVER_PIERCE:
							entity->arrowArmorPierce = 2;
							entity->sprite = 925;
							if ( stronger ) { ARROWTRAP_REFIRE = 50; }
							break;
						case QUIVER_LIGHTWEIGHT:
							entity->sprite = 926;
							ARROWTRAP_REFIRE = 50;
							if ( stronger ) { ARROWTRAP_REFIRE = 25; }
							break;
						case QUIVER_FIRE:
							entity->sprite = 927;
							if ( stronger ) { ARROWTRAP_REFIRE = 25; }
							break;
						case QUIVER_KNOCKBACK:
							entity->sprite = 928;
							if ( stronger ) { ARROWTRAP_REFIRE = 25; }
							break;
						case QUIVER_CRYSTAL:
							entity->sprite = 929;
							if ( stronger ) { ARROWTRAP_REFIRE = 25; }
							break;
						case QUIVER_HUNTING:
							entity->sprite = 930;
							// causes poison for six seconds
							entity->arrowPoisonTime = 360;
							if ( stronger ) { ARROWTRAP_REFIRE = 25; }
							break;
						default:
							break;
					}
					entity->arrowQuiverType = ARROWTRAP_TYPE;
					entity->arrowSpeed = 7;
					entity->vel_x = cos(entity->yaw) * entity->arrowSpeed;
					entity->vel_y = sin(entity->yaw) * entity->arrowSpeed;
					if ( multiplayer == SERVER )
					{
						Sint32 val = (1 << 31);
						val |= (Uint8)(17);
						val |= (((Uint16)(TOOL_SENTRYBOT) & 0xFFF) << 8);
						val |= (8) << 20;
						entity->skill[2] = val;//-(1000 + TOOL_SENTRYBOT); // invokes actArrow for clients.
						entity->arrowShotByWeapon = TOOL_SENTRYBOT;
					}
					if ( targetToAutoHit )
					{
						/*entity->x = targetToAutoHit->x;
						entity->y = targetToAutoHit->y;*/
						if ( local_rng.rand() % 2 == 0 )
						{
							double tangent = atan2(entity->y - targetToAutoHit->y, entity->x - targetToAutoHit->x);
							entity->yaw = tangent + PI;
							entity->vel_x = cos(entity->yaw) * entity->arrowSpeed;
							entity->vel_y = sin(entity->yaw) * entity->arrowSpeed;
							targetToAutoHit = nullptr;
						}
						else if ( local_rng.rand() % 2 == 0 )
						{
							entity->yaw = entity->yaw - PI / 12 + (0.1 * (local_rng.rand() % 11) * (PI / 6)); // -/+ PI/12 range
							entity->vel_x = cos(entity->yaw) * entity->arrowSpeed;
							entity->vel_y = sin(entity->yaw) * entity->arrowSpeed;
							targetToAutoHit = nullptr;
						}
					}
				}
			}
		}
	}
	else
	{
		if ( ARROWTRAP_REFIRE > 0 )
		{
			--ARROWTRAP_REFIRE;
		}
		else if ( ARROWTRAP_FIRED % 2 == 1 )
		{
			// fired, time to reload.
			ARROWTRAP_FIRED++;
		}
	}
}
