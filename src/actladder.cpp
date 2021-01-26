/*-------------------------------------------------------------------------------

	BARONY
	File: actladder.cpp
	Desc: behavior function for ladders

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "sound.hpp"
#include "entity.hpp"
#include "scores.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "magic/magic.hpp"
#include "menu.hpp"
#include "files.hpp"
#include "items.hpp"
#include "mod_tools.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define LADDER_AMBIENCE my->skill[1]
#define LADDER_SECRET_ENTRANCE my->skill[3]

void actLadder(Entity* my)
{
	int playercount = 0;
	double dist;
	int i, c;

	if ( my->ticks == 1 )
	{
		my->createWorldUITooltip();
	}

	LADDER_AMBIENCE--;
	if (LADDER_AMBIENCE <= 0)
	{
		LADDER_AMBIENCE = TICKS_PER_SECOND * 30;
		playSoundEntityLocal(my, 149, 64);
	}

	// use ladder (climb)
	if (multiplayer != CLIENT)
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if ( (i == 0 && selectedEntity[0] == my) || (client_selected[i] == my) || (splitscreen && selectedEntity[i] == my) )
			{
				if (inrange[i])
				{
					for (c = 0; c < MAXPLAYERS; c++)
					{
						if (client_disconnected[c] || players[c] == nullptr || players[c]->entity == nullptr)
						{
							continue;
						}
						else
						{
							playercount++;
						}
						dist = sqrt(pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
						if (dist > TOUCHRANGE)
						{
							messagePlayer(i, language[505]);
							return;
						}
					}
					if (playercount == 1)
					{
						messagePlayer(i, language[506]);
					}
					else
					{
						messagePlayer(i, language[507]);
					}
					loadnextlevel = true;
					if (secretlevel)
					{
						switch (currentlevel)
						{
							case 3:
								for (c = 0; c < MAXPLAYERS; c++)
								{
									steamAchievementClient(c, "BARONY_ACH_THUNDERGNOME");
								}
								break;
						}
						if ( LADDER_SECRET_ENTRANCE )
						{
							skipLevelsOnLoad = -1; // don't skip a regular level anymore. still skip if in underworld.
						}
					}
					if ( LADDER_SECRET_ENTRANCE )
					{
						secretlevel = (secretlevel == false);    // toggle level lists
					}
					return;
				}
			}
		}
	}
}

void actLadderUp(Entity* my)
{
	/*LADDER_AMBIENCE--;
	if ( LADDER_AMBIENCE <= 0 )
	{
		LADDER_AMBIENCE = TICKS_PER_SECOND * 30;
		playSoundEntityLocal( my, 149, 64 );
	}*/
	if ( my->ticks == 1 )
	{
		my->createWorldUITooltip();
	}

	// use ladder
	if ( multiplayer != CLIENT )
	{
		for (int i = 0; i < MAXPLAYERS; i++)
		{
			if ( (i == 0 && selectedEntity[0] == my) || (client_selected[i] == my) || (splitscreen && selectedEntity[i] == my) )
			{
				if (inrange[i])
				{
					messagePlayer(i, language[508]);
					return;
				}
			}
		}
	}
}

void actPortal(Entity* my)
{
	int playercount = 0;
	double dist;
	int i, c;

	if ( !my->portalInit )
	{
		my->createWorldUITooltip();
		my->portalInit = 1;
		my->light = lightSphereShadow(my->x / 16, my->y / 16, 3, 255);
		if ( !strncmp(map.name, "Cockatrice Lair", 15) )
		{
			my->flags[INVISIBLE] = true;
		}
		else if ( !strncmp(map.name, "Bram's Castle", 13) )
		{
			my->flags[INVISIBLE] = true;
		}
	}

	my->portalAmbience--;
	if ( my->portalAmbience <= 0 )
	{
		my->portalAmbience = TICKS_PER_SECOND * 2;
		if ( !my->flags[INVISIBLE] )
		{
			playSoundEntityLocal( my, 154, 128 );
		}
	}

	my->yaw += 0.01; // rotate slowly on my axis
	my->sprite = 254 + (my->ticks / 20) % 4; // animate

	if ( multiplayer == CLIENT )
	{
		return;
	}

	if ( my->flags[INVISIBLE] && ticks % 50 == 0 && !strncmp(map.name, "Cockatrice Lair", 15) )
	{
		node_t* node = nullptr;
		Entity* entity = nullptr;
		bool bossAlive = false;
		for ( node = map.entities->first; node != nullptr; )
		{
			entity = (Entity*)node->element;
			node = node->next;
			if ( entity && entity->behavior == &actMonster 
				&& entity->getMonsterTypeFromSprite() == COCKATRICE
				&& !entity->monsterAllyGetPlayerLeader() )
			{
				bossAlive = true;
			}
		}
		if ( !bossAlive )
		{
			for ( node = map.entities->first; node != nullptr; )
			{
				entity = (Entity*)node->element;
				node = node->next;
				if ( entity->behavior == &actMagicTrap )
				{
					list_RemoveNode(entity->mynode);
				}
			}
			my->flags[INVISIBLE] = false;
			serverUpdateEntityFlag(my, INVISIBLE);
		}
	}
	else if ( my->flags[INVISIBLE] && ticks % 50 == 0 && !strncmp(map.name, "Bram's Castle", 13) )
	{
		node_t* node = nullptr;
		Entity* entity = nullptr;
		bool bossAlive = false;
		for ( node = map.entities->first; node != nullptr; )
		{
			entity = (Entity*)node->element;
			node = node->next;
			if ( entity && entity->behavior == &actMonster
				&& entity->getMonsterTypeFromSprite() == VAMPIRE
				&& !entity->monsterAllyGetPlayerLeader() )
			{
				Stat* stats = entity->getStats();
				if ( stats && !strncmp(stats->name, "Bram Kindly", 11) )
				{
					bossAlive = true;
				}
			}
		}
		if ( !bossAlive )
		{
			for ( node = map.entities->first; node != nullptr; )
			{
				entity = (Entity*)node->element;
				node = node->next;
				if ( entity && entity->behavior == &actMagicTrap )
				{
					list_RemoveNode(entity->mynode);
				}
			}
			my->flags[INVISIBLE] = false;
			serverUpdateEntityFlag(my, INVISIBLE);
		}
	}

	// step through portal
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if ( (i == 0 && selectedEntity[0] == my) || (client_selected[i] == my) || (splitscreen && selectedEntity[i] == my) )
		{
			if (inrange[i])
			{
				for (c = 0; c < MAXPLAYERS; c++)
				{
					if (client_disconnected[c] || players[c] == nullptr || players[c]->entity == nullptr)
					{
						continue;
					}
					else
					{
						playercount++;
					}
					dist = sqrt(pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
					if (dist > TOUCHRANGE)
					{
						messagePlayer(i, language[505]);
						return;
					}
				}
				if (playercount == 1)
				{
					messagePlayer(i, language[510]);
				}
				else
				{
					messagePlayer(i, language[511]);
				}
				loadnextlevel = true;
				if ( secretlevel )
				{
					switch ( currentlevel )
					{
						case 9:
						{
							; //lol
							bool visiblegrave = false;
							node_t* node;
							for ( node = map.entities->first; node != nullptr; node = node->next )
							{
								Entity* entity = (Entity*)node->element;
								if ( entity->sprite == 224 && !entity->flags[INVISIBLE] )
								{
									visiblegrave = true;
									break;
								}
							}
							if ( visiblegrave )
							{
								for ( c = 0; c < MAXPLAYERS; ++c )
								{
									steamAchievementClient(c, "BARONY_ACH_ROBBING_THE_CRADLE");
								}
							}
							break;
						}
						case 14:
							for ( c = 0; c < MAXPLAYERS; ++c )
							{
								steamAchievementClient(c, "BARONY_ACH_THESEUS_LEGACY");
							}
							break;
						case 29:
							for ( c = 0; c < MAXPLAYERS; c++ )
							{
								steamAchievementClient(c, "BARONY_ACH_CULT_FOLLOWING");
							}
							break;
						case 34:
							for ( c = 0; c < MAXPLAYERS; c++ )
							{
								steamAchievementClient(c, "BARONY_ACH_DESPAIR_CALMS");
							}
							break;
						default:
							break;
					}
					if ( strncmp(map.name, "Underworld", 10) )
					{
						skipLevelsOnLoad = -1; // don't skip a regular level anymore. still skip if in underworld.
					}
					else
					{
						// underworld - don't skip on the early sections.
						if ( currentlevel == 6 || currentlevel == 7 )
						{
							skipLevelsOnLoad = -1;
						}
					}
				}
				if ( !my->portalNotSecret )
				{
					secretlevel = (secretlevel == false);  // toggle level lists
				}
				return;
			}
		}
	}
}

void actWinningPortal(Entity* my)
{
	int playercount = 0;
	double dist;
	int i, c;

	if ( my->ticks == 1 )
	{
		my->createWorldUITooltip();
	}

	if ( multiplayer != CLIENT )
	{
		if ( my->flags[INVISIBLE] )
		{
			if ( !strncmp(map.name, "Boss", 4) || !strncmp(map.name, "Hell Boss", 9) )
			{
				if ( !(svFlags & SV_FLAG_CLASSIC) )
				{
					return; // classic mode disabled.
				}
			}
			node_t* node;
			for ( node = map.creatures->first; node != nullptr; node = node->next )
			{
				Entity* entity = (Entity*)node->element;
				if ( entity->behavior == &actMonster )
				{
					Stat* stats = entity->getStats();
					if ( stats )
					{
						if ( stats->type == LICH || stats->type == DEVIL )
						{
							return;
						}
					}
				}
			}
			if ( my->skill[28] != 0 )
			{
				if ( my->skill[28] == 2 )
				{
					// powered on.
					if ( !my->portalFireAnimation )
					{
						Entity* timer = createParticleTimer(my, 100, 174);
						timer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SPAWN_PORTAL;
						timer->particleTimerCountdownSprite = 174;
						timer->particleTimerEndAction = PARTICLE_EFFECT_PORTAL_SPAWN;
						serverSpawnMiscParticles(my, PARTICLE_EFFECT_PORTAL_SPAWN, 174);
						my->portalFireAnimation = 1;
					}
				}
			}
		}
		else
		{
			if ( !strncmp(map.name, "Boss", 4) || !strncmp(map.name, "Hell Boss", 9) )
			{
				if ( !(svFlags & SV_FLAG_CLASSIC) )
				{
					my->flags[INVISIBLE] = true; // classic mode disabled, hide.
					serverUpdateEntityFlag(my, INVISIBLE);
					my->portalFireAnimation = 0;
				}
			}
		}
	}
	else
	{
		if ( my->flags[INVISIBLE] )
		{
			return;
		}
	}

	if ( !my->portalInit )
	{
		my->portalInit = 1;
		my->light = lightSphereShadow(my->x / 16, my->y / 16, 3, 255);
	}

	my->portalAmbience--;
	if ( my->portalAmbience <= 0 )
	{
		my->portalAmbience = TICKS_PER_SECOND * 2;
		playSoundEntityLocal( my, 154, 128 );
	}

	my->yaw += 0.01; // rotate slowly on my axis
	my->sprite = 278 + (my->ticks / 20) % 4; // animate

	if ( multiplayer == CLIENT )
	{
		return;
	}

	// step through portal
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if ( (i == 0 && selectedEntity[0] == my) || (client_selected[i] == my) || (splitscreen && selectedEntity[i] == my) )
		{
			if (inrange[i])
			{
				for (c = 0; c < MAXPLAYERS; c++)
				{
					if (client_disconnected[c] || players[c] == nullptr || players[c]->entity == nullptr)
					{
						continue;
					}
					else
					{
						playercount++;
					}
					dist = sqrt( pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
					if (dist > TOUCHRANGE)
					{
						messagePlayer(i, language[509]);
						return;
					}
				}
				victory = my->portalVictoryType;
				if ( multiplayer == SERVER )
				{
					for ( c = 1; c < MAXPLAYERS; c++ )
					{
						if ( client_disconnected[c] == true )
						{
							continue;
						}
						strcpy((char*)net_packet->data, "WING");
						net_packet->data[4] = victory;
						net_packet->address.host = net_clients[c - 1].host;
						net_packet->address.port = net_clients[c - 1].port;
						net_packet->len = 8;
						sendPacketSafe(net_sock, -1, net_packet, c - 1);
					}
				}
				subwindow = 0;
				introstage = 5; // prepares win game sequence
				fadeout = true;
				if ( !intro )
				{
					pauseGame(2, false);
				}
				return;
			}
		}
	}
}

void actExpansionEndGamePortal(Entity* my)
{
	if ( !my )
	{
		return;
	}

	my->actExpansionEndGamePortal();
}

void Entity::actExpansionEndGamePortal()
{
	int playercount = 0;
	double dist;
	int i, c;

	if ( this->ticks == 1 )
	{
		this->createWorldUITooltip();
	}

	if ( multiplayer != CLIENT )
	{
		if ( flags[INVISIBLE] )
		{
			node_t* node;
			for ( node = map.creatures->first; node != nullptr; node = node->next )
			{
				Entity* entity = (Entity*)node->element;
				if ( entity )
				{
					if ( entity->behavior == &actMonster )
					{
						Stat* stats = entity->getStats();
						if ( stats )
						{
							if ( stats->type == LICH_FIRE || stats->type == LICH_ICE )
							{
								return;
							}
						}
					}
				}
			}
			if ( circuit_status != 0 )
			{
				if ( circuit_status == CIRCUIT_ON )
				{
					// powered on.
					if ( !portalFireAnimation )
					{
						Entity* timer = createParticleTimer(this, 100, 174);
						timer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SPAWN_PORTAL;
						timer->particleTimerCountdownSprite = 174;
						timer->particleTimerEndAction = PARTICLE_EFFECT_PORTAL_SPAWN;
						serverSpawnMiscParticles(this, PARTICLE_EFFECT_PORTAL_SPAWN, 174);
						portalFireAnimation = 1;
					}
				}
			}
		}
	}
	else
	{
		if ( flags[INVISIBLE] )
		{
			return;
		}
	}

	if ( !portalInit )
	{
		portalInit = 1;
		light = lightSphereShadow(x / 16, y / 16, 3, 255);
	}

	portalAmbience--;
	if ( portalAmbience <= 0 )
	{
		portalAmbience = TICKS_PER_SECOND * 2;
		playSoundEntityLocal(this, 154, 128);
	}

	yaw += 0.01; // rotate slowly on my axis
	sprite = 614 + (this->ticks / 20) % 4; // animate

	if ( multiplayer == CLIENT )
	{
		return;
	}

	// step through portal
	for ( i = 0; i < MAXPLAYERS; i++ )
	{
		if ( (i == 0 && selectedEntity[0] == this) || (client_selected[i] == this) || (splitscreen && selectedEntity[i] == this) )
		{
			if ( inrange[i] )
			{
				for ( c = 0; c < MAXPLAYERS; c++ )
				{
					if ( client_disconnected[c] || players[c] == nullptr || players[c]->entity == nullptr )
					{
						continue;
					}
					else
					{
						playercount++;
					}
					dist = sqrt(pow(x - players[c]->entity->x, 2) + pow(y - players[c]->entity->y, 2));
					if ( dist > TOUCHRANGE )
					{
						messagePlayer(i, language[509]);
						return;
					}
				}
				victory = portalVictoryType;
				if ( multiplayer == SERVER )
				{
					for ( c = 1; c < MAXPLAYERS; c++ )
					{
						if ( client_disconnected[c] == true )
						{
							continue;
						}
						strcpy((char*)net_packet->data, "WING");
						net_packet->data[4] = victory;
						net_packet->address.host = net_clients[c - 1].host;
						net_packet->address.port = net_clients[c - 1].port;
						net_packet->len = 8;
						sendPacketSafe(net_sock, -1, net_packet, c - 1);
					}
				}
				subwindow = 0;
				introstage = 5; // prepares win game sequence
				fadeout = true;
				if ( !intro )
				{
					pauseGame(2, false);
				}
				return;
			}
		}
	}
}

void actMidGamePortal(Entity* my)
{
	if ( !my )
	{
		return;
	}

	my->actMidGamePortal();
}

void Entity::actMidGamePortal()
{
	int playercount = 0;
	double dist;
	int i, c;

	if ( this->ticks == 1 )
	{
		this->createWorldUITooltip();
	}

	if ( multiplayer != CLIENT )
	{
		if ( flags[INVISIBLE] )
		{
			if ( !strncmp(map.name, "Boss", 4) || !strncmp(map.name, "Hell Boss", 9) )
			{
				if ( (svFlags & SV_FLAG_CLASSIC) )
				{
					return; // classic mode enabled, don't process.
				}
			}
			node_t* node;
			for ( node = map.creatures->first; node != nullptr; node = node->next )
			{
				Entity* entity = (Entity*)node->element;
				if ( entity )
				{
					if ( entity->behavior == &actMonster )
					{
						Stat* stats = entity->getStats();
						if ( stats )
						{
							if ( stats->type == LICH || stats->type == DEVIL )
							{
								return;
							}
						}
					}
				}
			}
			if ( circuit_status != 0 )
			{
				if ( circuit_status == CIRCUIT_ON )
				{
					// powered on.
					if ( !portalFireAnimation )
					{
						Entity* timer = createParticleTimer(this, 100, 174);
						timer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SPAWN_PORTAL;
						timer->particleTimerCountdownSprite = 174;
						timer->particleTimerEndAction = PARTICLE_EFFECT_PORTAL_SPAWN;
						serverSpawnMiscParticles(this, PARTICLE_EFFECT_PORTAL_SPAWN, 174);
						portalFireAnimation = 1;
					}
				}
			}
		}
		else
		{
			if ( !strncmp(map.name, "Boss", 4) || !strncmp(map.name, "Hell Boss", 9) )
			{
				if ( (svFlags & SV_FLAG_CLASSIC) )
				{
					flags[INVISIBLE] = true; // classic mode enabled, hide.
					serverUpdateEntityFlag(this, INVISIBLE);
					portalFireAnimation = 0;
				}
			}
		}
	}
	else
	{
		if ( flags[INVISIBLE] )
		{
			return;
		}
	}

	if ( !portalInit )
	{
		portalInit = 1;
		light = lightSphereShadow(x / 16, y / 16, 3, 255);
	}

	portalAmbience--;
	if ( portalAmbience <= 0 )
	{
		portalAmbience = TICKS_PER_SECOND * 2;
		playSoundEntityLocal(this, 154, 128);
	}

	yaw += 0.01; // rotate slowly on my axis
	sprite = 614 + (this->ticks / 20) % 4; // animate

	if ( multiplayer == CLIENT )
	{
		return;
	}

	// step through portal
	for ( i = 0; i < MAXPLAYERS; i++ )
	{
		if ( (i == 0 && selectedEntity[0] == this) || (client_selected[i] == this) || (splitscreen && selectedEntity[i] == this) )
		{
			if ( inrange[i] )
			{
				for ( c = 0; c < MAXPLAYERS; c++ )
				{
					if ( client_disconnected[c] || players[c] == nullptr || players[c]->entity == nullptr )
					{
						continue;
					}
					else
					{
						playercount++;
					}
					dist = sqrt(pow(x - players[c]->entity->x, 2) + pow(y - players[c]->entity->y, 2));
					if ( dist > TOUCHRANGE )
					{
						messagePlayer(i, language[509]);
						return;
					}
				}
				//victory = portalVictoryType;
				int movieCrawlType = -1;
				if ( !strncmp(map.name, "Hell Boss", 9) )
				{
					movieCrawlType = MOVIE_MIDGAME_BAPHOMET_HUMAN_AUTOMATON;
					if ( stats[0] && stats[0]->playerRace > 0 && stats[0]->playerRace != RACE_AUTOMATON )
					{
						movieCrawlType = MOVIE_MIDGAME_BAPHOMET_MONSTERS;
					}
				}
				else if ( !strncmp(map.name, "Boss", 4) )
				{
					if ( stats[0] && stats[0]->playerRace > 0 && stats[0]->playerRace != RACE_AUTOMATON )
					{
						movieCrawlType = MOVIE_MIDGAME_HERX_MONSTERS;
					}
				}
				int introstageToChangeTo = 9;
				if ( movieCrawlType >= 0 )
				{
					introstageToChangeTo = 11 + movieCrawlType;
				}

				if ( multiplayer == SERVER )
				{
					for ( c = 1; c < MAXPLAYERS; c++ )
					{
						if ( client_disconnected[c] == true )
						{
							continue;
						}
						strcpy((char*)net_packet->data, "MIDG");
						net_packet->data[4] = introstageToChangeTo;
						net_packet->address.host = net_clients[c - 1].host;
						net_packet->address.port = net_clients[c - 1].port;
						net_packet->len = 8;
						sendPacketSafe(net_sock, -1, net_packet, c - 1);
					}
				}
				subwindow = 0;
				fadeout = true;
				if ( !intro )
				{
					pauseGame(2, false);
				}
				introstage = introstageToChangeTo; // prepares mid game sequence
				return;
			}
		}
	}
}

int customPortalLookForMapWithName(char* mapToSearch, bool isSecretLevel, int levelOffset)
{
	if ( !mapToSearch )
	{
		return -1000;
	}
	std::string mapsDirectory; // store the full file path here.
	if ( !isSecretLevel )
	{
		mapsDirectory = PHYSFS_getRealDir(LEVELSFILE);
		mapsDirectory.append(PHYSFS_getDirSeparator()).append(LEVELSFILE);
	}
	else
	{
		mapsDirectory = PHYSFS_getRealDir(SECRETLEVELSFILE);
		mapsDirectory.append(PHYSFS_getDirSeparator()).append(SECRETLEVELSFILE);
	}
	printlog("Maps directory: %s", mapsDirectory.c_str());
	std::vector<std::string> levelsList = getLinesFromDataFile(mapsDirectory);
	std::string line = levelsList.front();
	int levelsCounted = 0;

	std::vector<int> eligibleLevels;

	for ( auto it = levelsList.begin(); it != levelsList.end(); ++it )
	{
		line = *it;
		if ( line[0] == '\n' )
		{
			continue;
		}

		// find the actual map name, ignoring gen: or map: in the line.
		std::size_t found = line.find(' ');
		std::string mapName;
		if ( found != std::string::npos )
		{
			std::string mapType = line.substr(0, found);
			mapName = line.substr(found + 1, line.find('\n'));
			std::size_t carriageReturn = mapName.find('\r');
			if ( carriageReturn != std::string::npos )
			{
				mapName.erase(carriageReturn);
			}
			if ( mapType.compare("map:") == 0 )
			{
				/*strncpy(tempstr, mapName.c_str(), mapName.length());
				tempstr[mapName.length()] = '\0';*/
			}
			else if ( mapType.compare("gen:") == 0 )
			{
				mapName = mapName.substr(0, mapName.find_first_of(" \0"));
				/*strncpy(tempstr, mapName.c_str(), mapName.length());
				tempstr[mapName.length()] = '\0';*/
			}
		}
		if ( !strcmp(mapToSearch, mapName.c_str()) )
		{
			// found an entry matching our map name.
			eligibleLevels.push_back(levelsCounted);
		}
		++levelsCounted;
	}
	
	if ( eligibleLevels.empty() )
	{
		std::string mapPath = "maps/";
		mapPath.append(mapToSearch);
		if ( mapPath.find(".lmp") == std::string::npos )
		{
			mapPath.append(".lmp");
		}
		if ( !PHYSFS_getRealDir(mapPath.c_str()) )
		{
			// could not find the map.
			return -998;
		}
		else
		{
			loadCustomNextMap = mapToSearch;
			return -999;
		}
	}

	int min = eligibleLevels.front();
	int max = eligibleLevels.back();

	if ( eligibleLevels.size() == 1 )
	{
		return eligibleLevels.front();
	}
	else if ( currentlevel > max )
	{
		// all the maps are behind us.
		if ( levelOffset >= 0 )
		{
			return max; // going backwards to the nearest element.
		}
		else
		{
			auto it = eligibleLevels.rbegin();
			int lastGoodMapIndex = *it;
			for ( ; it != eligibleLevels.rend() && levelOffset != 0; ++it )
			{
				++levelOffset;
				lastGoodMapIndex = *it;
			}
			return lastGoodMapIndex;
		}
	}
	else if ( currentlevel < min )
	{
		// all the maps are ahead of us.
		if ( levelOffset <= 0 )
		{
			return min; // going forwards to the nearest element.
		}
		else
		{
			auto it = eligibleLevels.begin();
			int lastGoodMapIndex = *it;
			for ( ; it != eligibleLevels.end() && levelOffset != 0; ++it )
			{
				--levelOffset;
				lastGoodMapIndex = *it;
			}
			return lastGoodMapIndex;
		}
	}
	else
	{
		// we're in the middle of the range.
		// find our position, then move the offset amount.
		if ( levelOffset >= 0 )
		{
			auto find = eligibleLevels.begin();
			for ( auto it = eligibleLevels.begin(); it != eligibleLevels.end(); ++it )
			{
				if ( *find > currentlevel )
				{
					break;
				}
				find = it;
			}

			if ( levelOffset == 0 )
			{
				return *find;
			}

			int lastGoodMapIndex = *find;
			for ( auto it = find; it != eligibleLevels.end() && levelOffset != 0; ++it )
			{
				lastGoodMapIndex = *it;
				--levelOffset;
			}
			return lastGoodMapIndex;
		}
		else if ( levelOffset < 0 )
		{
			auto find = eligibleLevels.rbegin();
			for ( auto it = eligibleLevels.rbegin(); it != eligibleLevels.rend(); ++it )
			{
				if ( *find < currentlevel )
				{
					break;
				}
				find = it;
			}

			int lastGoodMapIndex = *find;
			for ( auto it = find; it != eligibleLevels.rend() && levelOffset != 0; ++it )
			{
				lastGoodMapIndex = *it;
				++levelOffset;
			}
			return lastGoodMapIndex;
		}
	}

	return -1000;
}

void actCustomPortal(Entity* my)
{
	int playercount = 0;
	double dist;
	int i, c;

	if ( my->ticks == 1 )
	{
		my->createWorldUITooltip();
	}

	if ( multiplayer != CLIENT )
	{
		if ( my->flags[INVISIBLE] )
		{
			if ( my->skill[28] != 0 )
			{
				if ( my->skill[28] == 2 )
				{
					// powered on.
					if ( !my->portalFireAnimation && my->portalCustomSpriteAnimationFrames > 0 )
					{
						Entity* timer = createParticleTimer(my, 100, 174);
						timer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SPAWN_PORTAL;
						timer->particleTimerCountdownSprite = 174;
						timer->particleTimerEndAction = PARTICLE_EFFECT_PORTAL_SPAWN;
						serverSpawnMiscParticles(my, PARTICLE_EFFECT_PORTAL_SPAWN, 174);
						my->portalFireAnimation = 1;
					}
				}
			}
		}
	}
	else
	{
		if ( my->flags[INVISIBLE] )
		{
			return;
		}
	}

	if ( !my->portalInit )
	{
		my->portalInit = 1;
		if ( my->portalCustomSpriteAnimationFrames > 0 )
		{
			my->light = lightSphereShadow(my->x / 16, my->y / 16, 3, 255);
		}
	}

	my->portalAmbience--;
	if ( my->portalAmbience <= 0 )
	{
		if ( my->portalCustomSpriteAnimationFrames > 0 )
		{
			my->portalAmbience = TICKS_PER_SECOND * 2; // portal whirr
			playSoundEntityLocal(my, 154, 128);
		}
		else
		{
			my->portalAmbience = TICKS_PER_SECOND * 30; // trap hum
			playSoundEntityLocal(my, 149, 64);
		}
	}

	if ( my->portalCustomSpriteAnimationFrames > 0 )
	{
		my->yaw += 0.01; // rotate slowly on my axis
		my->sprite = my->portalCustomSprite + (my->ticks / 20) % my->portalCustomSpriteAnimationFrames; // animate
	}
	else
	{
		my->sprite = my->portalCustomSprite;
	}

	if ( multiplayer == CLIENT )
	{
		return;
	}

	// step through portal
	for ( i = 0; i < MAXPLAYERS; i++ )
	{
		if ( (i == 0 && selectedEntity[0] == my) || (client_selected[i] == my) || (splitscreen && selectedEntity[i] == my) )
		{
			if ( inrange[i] )
			{
				for ( c = 0; c < MAXPLAYERS; c++ )
				{
					if ( client_disconnected[c] || players[c] == nullptr || players[c]->entity == nullptr )
					{
						continue;
					}
					else
					{
						playercount++;
					}
					dist = sqrt(pow(my->x - players[c]->entity->x, 2) + pow(my->y - players[c]->entity->y, 2));
					if ( dist > TOUCHRANGE )
					{
						messagePlayer(i, language[509]);
						return;
					}
				}
				if ( playercount == 1 )
				{
					messagePlayer(i, language[506]);
				}
				else
				{
					messagePlayer(i, language[507]);
				}
				loadnextlevel = true;
				skipLevelsOnLoad = 0;

				if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
				{
					std::string mapname = map.name;
					if ( mapname.find("Tutorial Hub") == std::string::npos
						&& mapname.find("Tutorial ") != std::string::npos )
					{
						achievementObserver.updatePlayerAchievement(clientnum, AchievementObserver::BARONY_ACH_DIPLOMA, AchievementObserver::DIPLOMA_LEVEL_COMPLETE);
						achievementObserver.updatePlayerAchievement(clientnum, AchievementObserver::BARONY_ACH_BACK_TO_BASICS, AchievementObserver::BACK_TO_BASICS_LEVEL_COMPLETE);
						int number = stoi(mapname.substr(mapname.find("Tutorial ") + strlen("Tutorial "), 2));
						auto& tutorialLevels = gameModeManager.Tutorial.levels;
						if ( number >= 1 && number < tutorialLevels.size() )
						{
							if ( tutorialLevels.at(number).completionTime == 0 )
							{
								tutorialLevels.at(number).completionTime = completionTime; // first time score.
							}
							else
							{
								tutorialLevels.at(number).completionTime = std::min(tutorialLevels.at(number).completionTime, completionTime);
							}
							achievementObserver.updateGlobalStat(
								std::min(STEAM_GSTAT_TUTORIAL1_COMPLETED - 1 + number, static_cast<int>(STEAM_GSTAT_TUTORIAL10_COMPLETED)));
							achievementObserver.updateGlobalStat(
								std::min(STEAM_GSTAT_TUTORIAL1_ATTEMPTS - 1 + number, static_cast<int>(STEAM_GSTAT_TUTORIAL10_ATTEMPTS)));
						}
						completionTime = 0;
						gameModeManager.Tutorial.writeToDocument();
						achievementObserver.updatePlayerAchievement(clientnum, AchievementObserver::BARONY_ACH_FAST_LEARNER, AchievementObserver::FAST_LEARNER_TIME_UPDATE);
					}
					else if ( mapname.find("Tutorial Hub") != std::string::npos )
					{
						completionTime = 0;
					}
				}

				if ( my->portalCustomLevelText1 != 0 )
				{
					// we're looking for a specific map name.
					char mapName[64] = "";
					int totalChars = 0;
					for ( int i = 11; i <= 18; ++i ) // starting from my->portalCustomMapText1 to my->portalCustomMapText8, 32 chars
					{
						if ( my->skill[i] != 0 && i != 28 ) // skill[28] is circuit status.
						{
							for ( int c = 0; c < 4; ++c )
							{
								if ( static_cast<char>((my->skill[i] >> (c * 8)) & 0xFF) == '\0'
									&& i != 18 && my->skill[i + 1] != 0 )
								{
									// don't add '\0' termination unless the next skill slot is empty as we have more data to read.
								}
								else
								{
									mapName[totalChars] = static_cast<char>((my->skill[i] >> (c * 8)) & 0xFF);
									++totalChars;
								}
							}
						}
					}
					if ( mapName[totalChars] != '\0' )
					{
						mapName[totalChars] = '\0';
					}
					int levelToJumpTo = customPortalLookForMapWithName(mapName, my->portalNotSecret ? false : true, my->portalCustomLevelsToJump);
					if ( levelToJumpTo == -1000 )
					{
						// error.
						printlog("Warning: Error in map teleport!");
						return;
					}
					else if ( levelToJumpTo == -999 )
					{
						// custom level not in the levels list, but was found in the maps folder.
						// we've set the next map to warp to.
						if ( my->portalCustomLevelsToJump - currentlevel > 0 )
						{
							skipLevelsOnLoad = my->portalCustomLevelsToJump - currentlevel;
						}
						else
						{
							skipLevelsOnLoad = my->portalCustomLevelsToJump - currentlevel - 1;
						}
						if ( skipLevelsOnLoad == -1 )
						{
							loadingSameLevelAsCurrent = true;
						}
						if ( my->portalNotSecret )
						{
							secretlevel = false;
						}
						else
						{
							secretlevel = true;
						}
						return;
					}
					else if ( levelToJumpTo == -998 )
					{
						// could not find the map name anywhere.
						loadnextlevel = false;
						skipLevelsOnLoad = 0;
						messagePlayer(i, "Error: Map %s was not found in the maps folder!", mapName);
						return;
					}
					int levelDifference = currentlevel - levelToJumpTo;
					if ( levelDifference == 0 && ((my->portalNotSecret && !secretlevel) || (!my->portalNotSecret && secretlevel)) )
					{
						//// error, we're reloading the same position, will glitch out clients.
						//loadnextlevel = false;
						//skipLevelsOnLoad = 0;
						//messagePlayer(i, "Error: Map to teleport to (%s) is the same position as current!", mapName);
						//return;
						loadingSameLevelAsCurrent = true; // update - can handle this now.
					}

					if ( levelToJumpTo - currentlevel > 0 )
					{
						skipLevelsOnLoad = levelToJumpTo - currentlevel;
					}
					else
					{
						skipLevelsOnLoad = levelToJumpTo - currentlevel - 1;
					}
					if ( my->portalNotSecret )
					{
						secretlevel = false;
					}
					else
					{
						secretlevel = true;
					}
				}
				else
				{
					if ( !my->portalNotSecret )
					{
						secretlevel = (secretlevel == false);    // toggle level lists
						skipLevelsOnLoad = -1; // don't skip levels when toggling.
					}
					skipLevelsOnLoad += my->portalCustomLevelsToJump;
				}
				return;
			}
		}
	}
}