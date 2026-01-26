/*-------------------------------------------------------------------------------

	BARONY
	File: maps.cpp
	Desc: level generator code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "files.hpp"
#include "items.hpp"
#include "prng.hpp"
#include "monster.hpp"
#include "magic/magic.hpp"
#include "interface/interface.hpp"
#include "book.hpp"
#include "net.hpp"
#include "paths.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "scores.hpp"
#include "mod_tools.hpp"
#include "menu.hpp"
#include "ui/MainMenu.hpp"

int startfloor = 0;
BaronyRNG map_rng;
BaronyRNG map_server_rng;
int numChests = 0;
int numMimics = 0;

Sint32 doorFrameSprite() {
    if (stringStr(map.name, "Caves", sizeof(map_t::name), 5)) {
        return 1163;
    }
    if (stringStr(map.name, "Citadel", sizeof(map_t::name), 7)) {
        return 1164;
    }
    if (stringStr(map.name, "Sanctum", sizeof(map_t::name), 7)) {
        return 1164;
    }
    if (stringStr(map.name, "Hell", sizeof(map_t::name), 4)) {
        return 1165;
    }
    if (stringStr(map.name, "Minotaur", sizeof(map_t::name), 8)) {
        return 1166;
    }
    if (stringStr(map.name, "Labyrinth", sizeof(map_t::name), 9)) {
        return 1166;
    }
    if (stringStr(map.name, "Mystic Library", sizeof(map_t::name), 14)) {
        return 1167;
    }
    if (stringStr(map.name, "Ruins", sizeof(map_t::name), 5)) {
        return 1167;
    }
    if (stringStr(map.name, "Swamp", sizeof(map_t::name), 5)) {
        return 1168;
    }
    if (stringStr(map.name, "Bram", sizeof(map_t::name), 4)) {
        return 1169;
    }
    if (stringStr(map.name, "Underworld", sizeof(map_t::name), 10)) {
        return 1169;
    }
    return 1; // default door frame
}

/*-------------------------------------------------------------------------------

	monsterCurve

	selects a monster randomly, taking into account the region the monster
	is being spawned in

-------------------------------------------------------------------------------*/

static ConsoleVariable<std::string> cvar_monster_curve("/monster_curve", "nothing");
int monsterCurve(int level)
{
	if ( svFlags & SV_FLAG_CHEATS )
	{
		for ( int i = 0; i < NUMMONSTERS; ++i )
		{
			if ( *cvar_monster_curve == monstertypename[i] )
			{
				if ( i != NOTHING )
				{
					return i;
				}
			}
		}
	}
	if ( !strncmp(map.name, "The Mines", 9) )   // the mines
	{
		switch ( map_rng.rand() % 10 )
		{
			case 0:
			case 1:
			case 2:
			case 3:
				return RAT;
			case 4:
			case 5:
			case 6:
			case 7:
				return SKELETON;
			case 8:
				if ( level >= 2 )
				{
					return SPIDER;
				}
				else
				{
					return SKELETON;
				}
			case 9:
				if ( level >= 2 )
				{
					return TROLL;
				}
				else
				{
					return SKELETON;
				}
				break;
		}
	}
	else if ( !strncmp(map.name, "The Swamp", 9) )     // the swamp
	{
		switch ( map_rng.rand() % 10 )
		{
			case 0:
			case 1:
				return SPIDER;
			case 2:
			case 3:
			case 4:
				return GOBLIN;
			case 5:
			case 6:
			case 7:
				return SLIME;
			case 8:
			case 9:
				return GHOUL;
		}
	}
	else if ( !strncmp(map.name, "The Labyrinth", 13) )     // sand labyrinth
	{
		switch ( map_rng.rand() % 20 )
		{
			case 0:
			case 1:
			case 2:
			case 3:
				return GOBLIN;
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
				return SCORPION;
			case 9:
			case 10:
			case 11:
			case 12:
				return TROLL;
			case 13:
			case 14:
			case 15:
			case 16:
				return SCARAB;
			case 17:
			case 18:
			case 19:
				return INSECTOID;
		}
	}
	else if ( !strncmp(map.name, "The Ruins", 9) )     // blue ruins
	{
		switch ( map_rng.rand() % 10 )
		{
			case 0:
				return GOBLIN;
			case 1:
			case 2:
			case 3:
			case 4:
				return GNOME;
			case 5:
			case 6:
			case 7:
				return BUGBEAR;
			case 8:
				if ( map_rng.rand() % 10 > 0 )
				{
					return BUGBEAR;
				}
				else
				{
					return VAMPIRE;
				}
			case 9:
				return DEMON;
		}
	}
	else if ( !strncmp(map.name, "Underworld", 10) )     // underworld
	{
		switch ( map_rng.rand() % 10 )
		{
			case 0:
				return SLIME;
			case 1:
				return SHADOW;
			case 2:
			case 3:
			case 4:
				return CREATURE_IMP;
			case 5:
			case 6:
			case 7:
				return GHOUL;
			case 8:
			case 9:
				return SKELETON;
		}
	}
	else if ( !strncmp(map.name, "Hell", 4) )     // hell
	{
		switch ( map_rng.rand() % 20 )
		{
			case 0:
			case 1:
				return SUCCUBUS;
			case 2:
			case 3:
				return INCUBUS;
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
				if ( strstr(map.name, "Boss") )
				{
					return DEMON;    // we would otherwise lag bomb on the boss level
				}
				else
				{
					return CREATURE_IMP;
				}
			case 9:
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
				return DEMON;
			case 15:
			case 16:
			case 17:
			case 18:
				return GOATMAN;
			case 19:
				return SHADOW;
		}
	}
	else if ( !strncmp(map.name, "Caves", 5) )
	{
		if ( currentlevel <= 26 )
		{
			switch ( map_rng.rand() % 15 )
			{
				case 0:
				case 1:
				case 2:
				case 3:
				case 4:
					return KOBOLD;
				case 5:
				case 6:
					return SCARAB;
				case 7:
					return AUTOMATON;
				case 8:
				case 9:
				case 10:
				case 11:
					return INSECTOID;
				case 12:
				case 13:
					if ( map_rng.rand() % 2 == 0 )
					{
						return INCUBUS;
					}
					else
					{
						return INSECTOID;
					}
				case 14:
					if ( map_rng.rand() % 2 == 0 )
					{
						return CRYSTALGOLEM;
					}
					else
					{
						return COCKATRICE;
					}
			}
		}
		else
		{
			switch ( map_rng.rand() % 15 )
			{
				case 0:
				case 1:
				case 2:
				case 3:
					return KOBOLD;
				case 4:
					return SCARAB;
				case 5:
					return AUTOMATON;
				case 6:
				case 7:
				case 8:
				case 9:
					return INSECTOID;
				case 10:
				case 11:
				case 12:
					return CRYSTALGOLEM;
				case 13:
					return INCUBUS;
				case 14:
					return COCKATRICE;
			}
		}
	}
	else if ( !strncmp(map.name, "Citadel", 7) )
	{
		switch ( map_rng.rand() % 15 )
		{
			case 0:
			case 1:
				return INCUBUS;
			case 2:
			case 3:
			case 4:
			case 5:
				return GOATMAN;
			case 6:
			case 7:
				return CRYSTALGOLEM;
			case 8:
			case 9:
				return VAMPIRE;
			case 10:
				return SHADOW;
			case 11:
				return INCUBUS;
			case 12:
				return AUTOMATON;
			case 13:
			case 14:
				return COCKATRICE;
		}
	}
	return SKELETON; // basic monster
}

struct StartRoomInfo_t
{
	int x1 = -1;
	int x2 = -1;
	int y1 = -1;
	int y2 = -1;
	bool isWall(int x, int y)
	{
		if ( x <= 0 || x >= map.width - 1 || y <= 0 || y >= map.height - 1 )
		{
			return true;
		}
		return map.tiles[OBSTACLELAYER + (y)* MAPLAYERS + (x)* MAPLAYERS * map.height];
	}
	bool isWalkable(int x, int y)
	{
		if ( x <= 0 || x >= map.width - 1 || y <= 0 || y >= map.height - 1 )
		{
			return false;
		}
		return map.tiles[(y)* MAPLAYERS + (x)* MAPLAYERS * map.height];
	}
	void addCoord(int x, int y)
	{
		if ( x1 == -1 )
		{
			x1 = x;
		}
		else
		{
			x1 = std::min(x, x1);
		}
		if ( x2 == -1 )
		{
			x2 = x;
		}
		else
		{
			x2 = std::max(x2, x);
		}

		if ( y1 == -1 )
		{
			y1 = y;
		}
		else
		{
			y1 = std::min(y, y1);
		}
		if ( y2 == -1 )
		{
			y2 = y;
		}
		else
		{
			y2 = std::max(y2, y);
		}
	}
	void checkBorderAccessibility()
	{
		enum Direction : int
		{
			NORTH,
			EAST,
			SOUTH,
			WEST
		};
		std::vector<std::pair<std::pair<int, int>, Direction>> potentialExitPoints;
		std::vector<std::pair<std::pair<int, int>, Direction>> goodTunnelPoints;
		std::vector<std::pair<std::pair<int, int>, Direction>> badTunnelPoints;
		std::vector<std::pair<std::pair<int, int>, Direction>> worstTunnelPoints;
		std::vector<std::pair<std::pair<int, int>, Direction>> exitPoints;
		if ( x1 - 1 > 0 )
		{
			for ( int y = y1; y <= y2; ++y )
			{
				if ( !isWall(x1, y) && isWalkable(x1, y) )
				{
					potentialExitPoints.push_back(std::make_pair(std::make_pair(x1, y), Direction::WEST));
					if ( !isWall(x1 - 1, y) && isWalkable(x1 - 1, y) )
					{
						// exit point found heading west
						exitPoints.push_back(std::make_pair(std::make_pair(x1, y), Direction::WEST));
					}
					else if ( isWall(x1 - 1, y) && isWalkable(x1 - 1, y) )
					{
						if ( isWalkable(x1 - 2, y) )
						{
							if ( isWall(x1 - 2, y) )
							{
								badTunnelPoints.push_back(std::make_pair(std::make_pair(x1, y), Direction::WEST));
							}
							else
							{
								if ( pathCheckObstacle(x1 - 2, y, nullptr, nullptr) == 1 ) // check interfering entities
								{
									badTunnelPoints.push_back(std::make_pair(std::make_pair(x1, y), Direction::WEST));
								}
								else
								{
									goodTunnelPoints.push_back(std::make_pair(std::make_pair(x1, y), Direction::WEST));
								}
							}
						}
						else
						{
							worstTunnelPoints.push_back(std::make_pair(std::make_pair(x1, y), Direction::WEST));
						}
					}
				}
			}
		}
		if ( x2 + 1 < (map.width) )
		{
			for ( int y = y1; y <= y2; ++y )
			{
				if ( !isWall(x2, y) && isWalkable(x2, y) )
				{
					potentialExitPoints.push_back(std::make_pair(std::make_pair(x2, y), Direction::EAST));
					if ( !isWall(x2 + 1, y) && isWalkable(x2 + 1, y) )
					{
						// exit point found heading east
						exitPoints.push_back(std::make_pair(std::make_pair(x2, y), Direction::EAST));
					}
					else if ( isWall(x2 + 1, y) && isWalkable(x2 + 1, y) )
					{
						if ( isWalkable(x2 + 2, y) )
						{
							if ( isWall(x2 + 2, y) )
							{
								badTunnelPoints.push_back(std::make_pair(std::make_pair(x2, y), Direction::EAST));
							}
							else
							{
								if ( pathCheckObstacle(x2 + 2, y, nullptr, nullptr) == 1 ) // check interfering entities
								{
									badTunnelPoints.push_back(std::make_pair(std::make_pair(x2, y), Direction::EAST));
								}
								else
								{
									goodTunnelPoints.push_back(std::make_pair(std::make_pair(x2, y), Direction::EAST));
								}
							}
						}
						else
						{
							worstTunnelPoints.push_back(std::make_pair(std::make_pair(x2, y), Direction::EAST));
						}
					}
				}
			}
		}
		if ( y1 - 1 > 0 )
		{
			for ( int x = x1; x <= x2; ++x )
			{
				if ( !isWall(x, y1) && isWalkable(x, y1) )
				{
					potentialExitPoints.push_back(std::make_pair(std::make_pair(x, y1), Direction::NORTH));
					if ( !isWall(x, y1 - 1) && isWalkable(x, y1 - 1) )
					{
						// exit point found heading north
						exitPoints.push_back(std::make_pair(std::make_pair(x, y1), Direction::NORTH));
					}
					else if ( isWall(x, y1 - 1) && isWalkable(x, y1 - 1) )
					{
						if ( isWalkable(x, y1 - 2) )
						{
							if ( isWall(x, y1 - 2) )
							{
								badTunnelPoints.push_back(std::make_pair(std::make_pair(x, y1), Direction::NORTH));
							}
							else
							{
								if ( pathCheckObstacle(x, y1 - 2, nullptr, nullptr) == 1 ) // check interfering entities
								{
									badTunnelPoints.push_back(std::make_pair(std::make_pair(x, y1), Direction::NORTH));
								}
								else
								{
									goodTunnelPoints.push_back(std::make_pair(std::make_pair(x, y1), Direction::NORTH));
								}
							}
						}
						else
						{
							worstTunnelPoints.push_back(std::make_pair(std::make_pair(x, y1), Direction::NORTH));
						}
					}
				}
			}
		}
		if ( y2 + 1 < (map.height) )
		{
			for ( int x = x1; x <= x2; ++x )
			{
				if ( !isWall(x, y2) && isWalkable(x, y2) )
				{
					potentialExitPoints.push_back(std::make_pair(std::make_pair(x, y2), Direction::SOUTH));
					if ( !isWall(x, y2 + 1) && isWalkable(x, y2 + 1) )
					{
						// exit point found heading north
						exitPoints.push_back(std::make_pair(std::make_pair(x, y2), Direction::SOUTH));
					}
					else if ( isWall(x, y2 + 1) && isWalkable(x, y2 + 1) )
					{
						if ( isWalkable(x, y2 + 2) )
						{
							if ( isWall(x, y2 + 2) )
							{
								badTunnelPoints.push_back(std::make_pair(std::make_pair(x, y2), Direction::SOUTH));
							}
							else
							{
								if ( pathCheckObstacle(x, y2 + 2, nullptr, nullptr) == 1 ) // check interfering entities
								{
									badTunnelPoints.push_back(std::make_pair(std::make_pair(x, y2), Direction::SOUTH));
								}
								else
								{
									goodTunnelPoints.push_back(std::make_pair(std::make_pair(x, y2), Direction::SOUTH));
								}
							}
						}
						else
						{
							worstTunnelPoints.push_back(std::make_pair(std::make_pair(x, y2), Direction::SOUTH));
						}
					}
				}
			}
		}

		//for ( auto point : exitPoints )
		//{
		//	std::string dir = "";
		//	switch ( point.second )
		//	{
		//		case WEST:
		//			dir = "West";
		//			break;
		//		case EAST:
		//			dir = "East";
		//			break;
		//		case NORTH:
		//			dir = "North";
		//			break;
		//		case SOUTH:
		//			dir = "South";
		//			break;
		//		default:
		//			break;
		//	}
		//	printlog("exitPoints %s: (x: %d y: %d)", dir.c_str(), point.first.first, point.first.second);
		//}
		if ( exitPoints.empty() )
		{
			printlog("[MAP GENERATOR]: Start map does not have accessibility to any other areas!");
			for ( auto point : goodTunnelPoints )
			{
				std::string dir = "";
				switch ( point.second )
				{
					case WEST:
						dir = "West";
						break;
					case EAST:
						dir = "East";
						break;
					case NORTH:
						dir = "North";
						break;
					case SOUTH:
						dir = "South";
						break;
					default:
						break;
				}
				printlog("[MAP GENERATOR]: TunnelPoints1 %s: (x: %d y: %d)", dir.c_str(), point.first.first, point.first.second);
			}
			for ( auto point : badTunnelPoints )
			{
				std::string dir = "";
				switch ( point.second )
				{
					case WEST:
						dir = "West";
						break;
					case EAST:
						dir = "East";
						break;
					case NORTH:
						dir = "North";
						break;
					case SOUTH:
						dir = "South";
						break;
					default:
						break;
				}
				printlog("[MAP GENERATOR]: TunnelPoints2 %s: (x: %d y: %d)", dir.c_str(), point.first.first, point.first.second);
			}
			for ( auto point : worstTunnelPoints )
			{
				std::string dir = "";
				switch ( point.second )
				{
					case WEST:
						dir = "West";
						break;
					case EAST:
						dir = "East";
						break;
					case NORTH:
						dir = "North";
						break;
					case SOUTH:
						dir = "South";
						break;
					default:
						break;
				}
				printlog("[MAP GENERATOR]: TunnelPoints3 %s: (x: %d y: %d)", dir.c_str(), point.first.first, point.first.second);
			}
			if ( !goodTunnelPoints.empty() )
			{
				auto picked = goodTunnelPoints.at(map_rng.rand() % goodTunnelPoints.size());
				switch ( picked.second )
				{
					case WEST:
						picked.first.first--;
						break;
					case EAST:
						picked.first.first++;
						break;
					case NORTH:
						picked.first.second--;
						break;
					case SOUTH:
						picked.first.second++;
						break;
					default:
						break;
				}
				printlog("[MAP GENERATOR]: Dug hole using TunnelPoints1 at x: %d y: %d", picked.first.first, picked.first.second);
				map.tiles[OBSTACLELAYER + (picked.first.second)* MAPLAYERS + (picked.first.first)* MAPLAYERS * map.height] = 0;
			}
			else if ( !badTunnelPoints.empty() )
			{
				auto picked = badTunnelPoints.at(map_rng.rand() % badTunnelPoints.size());
				switch ( picked.second )
				{
					case WEST:
						picked.first.first--;
						break;
					case EAST:
						picked.first.first++;
						break;
					case NORTH:
						picked.first.second--;
						break;
					case SOUTH:
						picked.first.second++;
						break;
					default:
						break;
				}
				printlog("[MAP GENERATOR]: Dug hole using TunnelPoints2 at x: %d y: %d", picked.first.first, picked.first.second);
				map.tiles[OBSTACLELAYER + (picked.first.second)* MAPLAYERS + (picked.first.first)* MAPLAYERS * map.height] = 0;
			}
			else if ( !worstTunnelPoints.empty() )
			{
				auto picked = worstTunnelPoints.at(map_rng.rand() % worstTunnelPoints.size());
				switch ( picked.second )
				{
					case WEST:
						picked.first.first--;
						break;
					case EAST:
						picked.first.first++;
						break;
					case NORTH:
						picked.first.second--;
						break;
					case SOUTH:
						picked.first.second++;
						break;
					default:
						break;
				}
				printlog("[MAP GENERATOR]: Dug hole using TunnelPoints3 at x: %d y: %d", picked.first.first, picked.first.second);
				map.tiles[OBSTACLELAYER + (picked.first.second)* MAPLAYERS + (picked.first.first)* MAPLAYERS * map.height] = 0;
			}
		}
	}
};

bool mapSpriteIsDoorway(int sprite)
{
	switch ( sprite )
	{
		case 2:
		case 3:
			return true;
			break;
		case 19:
		case 20:
			return true;
			break;
		case 113:
		case 114:
			return true;
			break;
		default:
			break;
	}
	return false;
}

int getMapPossibleLocationX1()
{
	const int perimeter = MFLAG_PERIMETER_GAP;
	return perimeter;
}

int getMapPossibleLocationY1()
{
	const int perimeter = MFLAG_PERIMETER_GAP;
	return perimeter;
}

int getMapPossibleLocationX2()
{
	const int perimeter = MFLAG_PERIMETER_GAP;
	return map.width - perimeter;
}

int getMapPossibleLocationY2()
{
	const int perimeter = MFLAG_PERIMETER_GAP;
	return map.height - perimeter;
}

bool mapTileDiggable(const int x, const int y)
{
	if ( swimmingtiles[map.tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map.height]]
		|| lavatiles[map.tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map.height]] )
	{
		return false;
	}
	if ( !strncmp(map.name, "Hell", 4) )
	{
		if ( x < getMapPossibleLocationX1() || x >= getMapPossibleLocationX2()
			|| y < getMapPossibleLocationY1() || y >= getMapPossibleLocationY2() )
		{
			return false;
		}
	}
	return true;
}

/*-------------------------------------------------------------------------------

	generateDungeon

	generates a level by drawing data from numerous files and connecting
	their rooms together with tunnels.

-------------------------------------------------------------------------------*/

int generateDungeon(char* levelset, Uint32 seed, std::tuple<int, int, int, int> mapParameters)
{
	char* sublevelname, *subRoomName;
	char sublevelnum[3];
	map_t* tempMap = nullptr;
	map_t* subRoomMap = nullptr;
	list_t mapList, *newList, *subRoomList, subRoomMapList;
	node_t* node, *node2, *node3, *nextnode, *subRoomNode;
	Sint32 c, i, j;
	Sint32 numlevels, levelnum, levelnum2;
	Sint32 x, y, z;
	Sint32 x0, y0, x1, y1;
	door_t* door, *newDoor;
	bool* possiblelocations, *possiblelocations2, *possiblerooms;
	bool* firstroomtile;
	Sint32 numpossiblelocations, pickedlocation, subroomPickRoom;
	Entity* entity, *entity2, *childEntity;
	Uint32 levellimit;
	list_t doorList;
	node_t* doorNode = nullptr; 
	node_t* subRoomDoorNode = nullptr;
	bool shoplevel = false;
	map_t shopmap;
	map_t secretlevelmap;
	int secretlevelexit = 0;

	if ( map.trapexcludelocations )
	{
		free(map.trapexcludelocations);
		map.trapexcludelocations = nullptr;
	}
	if ( map.monsterexcludelocations )
	{
		free(map.monsterexcludelocations);
		map.monsterexcludelocations = nullptr;
	}
	if ( map.lootexcludelocations )
	{
		free(map.lootexcludelocations);
		map.lootexcludelocations = nullptr;
	}

	if ( std::get<LEVELPARAM_CHANCE_SECRET>(mapParameters) == -1
		&& std::get<LEVELPARAM_CHANCE_DARKNESS>(mapParameters) == -1
		&& std::get<LEVELPARAM_CHANCE_MINOTAUR>(mapParameters) == -1
		&& std::get<LEVELPARAM_DISABLE_NORMAL_EXIT>(mapParameters) == 0 )
	{
		printlog("generating a dungeon from level set '%s' (seed %lu)...\n", levelset, seed);
	}
	else
	{
		char generationLog[256] = "generating a dungeon from level set '%s'";
		char tmpBuffer[32];
		if ( std::get<LEVELPARAM_CHANCE_SECRET>(mapParameters) != -1 )
		{
			snprintf(tmpBuffer, 31, ", secret chance %d%%%%", std::get<LEVELPARAM_CHANCE_SECRET>(mapParameters));
			strcat(generationLog, tmpBuffer);
		}
		if ( std::get<LEVELPARAM_CHANCE_DARKNESS>(mapParameters) != -1 )
		{
			snprintf(tmpBuffer, 31, ", darkmap chance %d%%%%", std::get<LEVELPARAM_CHANCE_DARKNESS>(mapParameters));
			strcat(generationLog, tmpBuffer);
		}
		if ( std::get<LEVELPARAM_CHANCE_MINOTAUR>(mapParameters) != -1 )
		{
			snprintf(tmpBuffer, 31, ", minotaur chance %d%%%%", std::get<LEVELPARAM_CHANCE_MINOTAUR>(mapParameters));
			strcat(generationLog, tmpBuffer);
		}
		if ( std::get<LEVELPARAM_DISABLE_NORMAL_EXIT>(mapParameters) != 0 )
		{
			snprintf(tmpBuffer, 31, ", disabled normal exit %d%%%%", std::get<LEVELPARAM_DISABLE_NORMAL_EXIT>(mapParameters));
			strcat(generationLog, tmpBuffer);
		}
		strcat(generationLog, ", (seed %lu)...\n");
		printlog(generationLog, levelset, seed);

		conductGameChallenges[CONDUCT_MODDED] = 1;
		Mods::disableSteamAchievements = true;
	}

	std::string fullMapPath;
	fullMapPath = physfsFormatMapName(levelset);

	int checkMapHash = -1;
	if ( fullMapPath.empty() || loadMap(fullMapPath.c_str(), &map, map.entities, map.creatures, &checkMapHash) == -1 )
	{
		printlog("error: no level of set '%s' could be found.\n", levelset);
		return -1;
	}
	if ( !verifyMapHash(fullMapPath.c_str(), checkMapHash) )
	{
		conductGameChallenges[CONDUCT_MODDED] = 1;
		Mods::disableSteamAchievements = true;
	}

	// store this map's seed
	mapseed = seed;
	map_rng.seedBytes(&mapseed, sizeof(mapseed));
	map_server_rng.seedBytes(&mapseed, sizeof(mapseed));

	// generate a custom monster curve if file exists
	monsterCurveCustomManager.readFromFile(mapseed);

	// determine whether shop level or not
	if ( gameplayCustomManager.processedShopFloor(currentlevel, secretlevel, map.name, shoplevel) )
	{
		// function sets shop level for us.
	}
	else if ( map_rng.rand() % 2 && currentlevel > 1 && strncmp(map.name, "Underworld", 10) && strncmp(map.name, "Hell", 4) )
	{
		shoplevel = true;
	}

	// determine whether minotaur level or not
	if ( (svFlags & SV_FLAG_MINOTAURS) && gameplayCustomManager.processedMinotaurSpawn(currentlevel, secretlevel, map.name) )
	{
		// function sets mino level for us.
	}
	else if ( std::get<LEVELPARAM_CHANCE_MINOTAUR>(mapParameters) != -1 )
	{
		if ( map_rng.rand() % 100 < std::get<LEVELPARAM_CHANCE_MINOTAUR>(mapParameters) && (svFlags & SV_FLAG_MINOTAURS) )
		{
			minotaurlevel = 1;
		}
	}
	else if ( (currentlevel < 25 && (currentlevel % LENGTH_OF_LEVEL_REGION == 2 || currentlevel % LENGTH_OF_LEVEL_REGION == 3))
		|| (currentlevel > 25 && (currentlevel % LENGTH_OF_LEVEL_REGION == 2 || currentlevel % LENGTH_OF_LEVEL_REGION == 4)) )
	{
		if ( map_rng.rand() % 2 && (svFlags & SV_FLAG_MINOTAURS) )
		{
			minotaurlevel = 1;
		}
	}

	// dark level
	if ( gameplayCustomManager.processedDarkFloor(currentlevel, secretlevel, map.name) )
	{
		// function sets dark level for us.
		if ( darkmap )
		{
			messageLocalPlayers(MESSAGE_HINT, Language::get(1108));
		}
	}
	else if ( !secretlevel )
	{
		if ( std::get<LEVELPARAM_CHANCE_DARKNESS>(mapParameters) != -1 )
		{
			if ( map_rng.rand() % 100 < std::get<LEVELPARAM_CHANCE_DARKNESS>(mapParameters) )
			{
				darkmap = true;
				messageLocalPlayers(MESSAGE_HINT, Language::get(1108));
			}
			else
			{
				darkmap = false;
			}
		}
		else if ( currentlevel % LENGTH_OF_LEVEL_REGION >= 2 )
		{
			if ( map_rng.rand() % 4 == 0 )
			{
				darkmap = true;
				messageLocalPlayers(MESSAGE_HINT, Language::get(1108));
			}
		}
	}

	// secret stuff
	if ( !secretlevel )
	{
		if ( std::get<LEVELPARAM_CHANCE_SECRET>(mapParameters) != -1 )
		{
			if ( map_rng.rand() % 100 < std::get<LEVELPARAM_CHANCE_SECRET>(mapParameters) )
			{
				secretlevelexit = 7;
			}
			else
			{
				secretlevelexit = 0;
			}
		}
		else if ( (currentlevel == 3 && map_rng.rand() % 2) || currentlevel == 2 )
		{
			secretlevelexit = 1;
		}
		else if ( currentlevel == 7 || currentlevel == 8 )
		{
			secretlevelexit = 2;
		}
		else if ( currentlevel == 11 || currentlevel == 13 )
		{
			secretlevelexit = 3;
		}
		else if ( currentlevel == 16 || currentlevel == 18 )
		{
			secretlevelexit = 4;
		}
		else if ( currentlevel == 28 )
		{
			secretlevelexit = 5;
		}
		else if ( currentlevel == 33 )
		{
			secretlevelexit = 6;
		}
		else if ( currentlevel == 23 )
		{
			secretlevelexit = 8;
			minotaurlevel = false;
		}
	}

	mapList.first = nullptr;
	mapList.last = nullptr;
	doorList.first = nullptr;
	doorList.last = nullptr;

	struct ShopSubRooms_t
	{
		std::string shopFileName = "";
		int count = 0;
		list_t list;

		ShopSubRooms_t()
		{
			list.first = nullptr;
			list.last = nullptr;
		}
	};
	ShopSubRooms_t shopSubRooms;

	// load shop room
	if ( shoplevel )
	{
		sublevelname = (char*) malloc(sizeof(char) * 128);
		std::string shopMapTitle = "shop";
		if ( MFLAG_GENADJACENTROOMS )
		{
			shopMapTitle = "shop-roomgen";
		}
		for ( numlevels = 0; numlevels < 100; numlevels++ )
		{
			strcpy(sublevelname, shopMapTitle.c_str());
			snprintf(sublevelnum, 3, "%02d", numlevels);
			strcat(sublevelname, sublevelnum);

			fullMapPath = physfsFormatMapName(sublevelname);

			if ( fullMapPath.empty() )
			{
				break;    // no more levels to load
			}
		}
		if ( numlevels )
		{
			int shopleveltouse = map_rng.rand() % numlevels;
			strcpy(sublevelname, shopMapTitle.c_str());
			snprintf(sublevelnum, 3, "%02d", shopleveltouse);
			strcat(sublevelname, sublevelnum);
			shopSubRooms.shopFileName = sublevelname;
			fullMapPath = physfsFormatMapName(sublevelname);

			shopmap.tiles = nullptr;
			shopmap.entities = (list_t*) malloc(sizeof(list_t));
			shopmap.entities->first = nullptr;
			shopmap.entities->last = nullptr;
			shopmap.creatures = new list_t;
			shopmap.creatures->first = nullptr;
			shopmap.creatures->last = nullptr;
			shopmap.worldUI = nullptr;
			if ( fullMapPath.empty() || loadMap(fullMapPath.c_str(), &shopmap, shopmap.entities, shopmap.creatures, &checkMapHash) == -1 )
			{
				list_FreeAll(shopmap.entities);
				free(shopmap.entities);
				list_FreeAll(shopmap.creatures);
				delete shopmap.creatures;
				if ( shopmap.tiles )
				{
					free(shopmap.tiles);
				}
			}
			if (!verifyMapHash(fullMapPath.c_str(), checkMapHash))
			{
				conductGameChallenges[CONDUCT_MODDED] = 1;
				Mods::disableSteamAchievements = true;
			}
		}
		else
		{
			shoplevel = false;
		}
		free( sublevelname );
	}

	sublevelname = (char*)malloc(sizeof(char) * 128);

	// a maximum of 100 (0-99 inclusive) sublevels can be added to the pool
	for ( numlevels = 0; numlevels < 100; ++numlevels )
	{
		strcpy(sublevelname, levelset);
		snprintf(sublevelnum, 3, "%02d", numlevels);
		strcat(sublevelname, sublevelnum);

		fullMapPath = physfsFormatMapName(sublevelname);
		if ( fullMapPath.empty() )
		{
			break;    // no more levels to load
		}

		// allocate memory for the next sublevel and attempt to load it
		tempMap = (map_t*) malloc(sizeof(map_t));
		tempMap->tiles = nullptr;
		tempMap->entities = (list_t*) malloc(sizeof(list_t));
		tempMap->entities->first = nullptr;
		tempMap->entities->last = nullptr;
		tempMap->creatures = new list_t;
		tempMap->creatures->first = nullptr;
		tempMap->creatures->last = nullptr;
		tempMap->worldUI = nullptr;
		tempMap->trapexcludelocations = nullptr;
		tempMap->monsterexcludelocations = nullptr;
		tempMap->lootexcludelocations = nullptr;
		if ( fullMapPath.empty() || loadMap(fullMapPath.c_str(), tempMap, tempMap->entities, tempMap->creatures, &checkMapHash) == -1 )
		{
			mapDeconstructor((void*)tempMap);
			continue; // failed to load level
		}
		if (!verifyMapHash(fullMapPath.c_str(), checkMapHash))
		{
			conductGameChallenges[CONDUCT_MODDED] = 1;
			Mods::disableSteamAchievements = true;
		}

		// level is successfully loaded, add it to the pool
		newList = (list_t*) malloc(sizeof(list_t));
		newList->first = nullptr;
		newList->last = nullptr;
		node = list_AddNodeLast(&mapList);
		node->element = newList;
		node->deconstructor = &listDeconstructor;

		node = list_AddNodeLast(newList);
		node->element = tempMap;
		node->deconstructor = &mapDeconstructor;

		// more nodes are created to record the exit points on the sublevel
		for ( y = 0; y < tempMap->height; y++ )
		{
			for ( x = 0; x < tempMap->width; x++ )
			{
				if ( x == 0 || y == 0 || x == tempMap->width - 1 || y == tempMap->height - 1 )
				{
					if ( !tempMap->tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * tempMap->height] )
					{
						door = (door_t*) malloc(sizeof(door_t));
						door->x = x;
						door->y = y;
						if ( x == tempMap->width - 1 )
						{
							door->dir = door_t::DIR_EAST;
							if ( y == tempMap->height - 1 )
							{
								door->edge = door_t::EDGE_SOUTHEAST;
							}
							else if ( y == 0 )
							{
								door->edge = door_t::EDGE_NORTHEAST;
							}
							else
							{
								door->edge = door_t::EDGE_EAST;
							}
						}
						else if ( y == tempMap->height - 1 )
						{
							door->dir = door_t::DIR_SOUTH;
							if ( x == tempMap->width - 1 )
							{
								door->edge = door_t::EDGE_SOUTHEAST;
							}
							else if ( x == 0 )
							{
								door->edge = door_t::EDGE_SOUTHWEST;
							}
							else
							{
								door->edge = door_t::EDGE_SOUTH;
							}
						}
						else if ( x == 0 )
						{
							door->dir = door_t::DIR_WEST;
							if ( y == tempMap->height - 1 )
							{
								door->edge = door_t::EDGE_SOUTHWEST;
							}
							else if ( y == 0 )
							{
								door->edge = door_t::EDGE_NORTHWEST;
							}
							else
							{
								door->edge = door_t::EDGE_WEST;
							}
						}
						else if ( y == 0 )
						{
							door->dir = door_t::DIR_NORTH;
							if ( x == tempMap->width - 1 )
							{
								door->edge = door_t::EDGE_NORTHEAST;
							}
							else if ( x == 0 )
							{
								door->edge = door_t::EDGE_NORTHWEST;
							}
							else
							{
								door->edge = door_t::EDGE_NORTH;
							}
						}
						node2 = list_AddNodeLast(newList);
						node2->element = door;
						node2->deconstructor = &defaultDeconstructor;
					}
				}
			}
		}
	}

	subRoomName = (char*)malloc(sizeof(char) * 128);
	subRoomMapList.first = nullptr;
	subRoomMapList.last = nullptr;
	char letterString[2];
	letterString[1] = '\0';
	int subroomCount[100] = {0};

	// a maximum of 100 (0-99 inclusive) sublevels can be added to the pool
	for ( int subRoomNumLevels = 0; subRoomNumLevels <= numlevels; subRoomNumLevels++ )
	{
		for ( char letter = 'a'; letter <= 'z'; letter++ )
		{
			// look for mapnames ending in a letter a to z
			strcpy(subRoomName, levelset);
			snprintf(sublevelnum, 3, "%02d", subRoomNumLevels);
			letterString[0] = letter;
			strcat(subRoomName, sublevelnum);
			strcat(subRoomName, letterString);

			fullMapPath = physfsFormatMapName(subRoomName);

			if ( fullMapPath.empty() )
			{
				break;    // no more levels to load
			}

			// check if there is another subroom to load
			//if ( !dataPathExists(fullMapPath.c_str()) )
			//{
			//	break;    // no more levels to load
			//}

			printlog("[SUBMAP GENERATOR] Found map lv %s, count: %d", subRoomName, subroomCount[subRoomNumLevels]);
			++subroomCount[subRoomNumLevels];

			// allocate memory for the next subroom and attempt to load it
			subRoomMap = (map_t*)malloc(sizeof(map_t));
			subRoomMap->tiles = nullptr;
			subRoomMap->entities = (list_t*)malloc(sizeof(list_t));
			subRoomMap->entities->first = nullptr;
			subRoomMap->entities->last = nullptr;
			subRoomMap->creatures = new list_t;
			subRoomMap->creatures->first = nullptr;
			subRoomMap->creatures->last = nullptr;
			subRoomMap->worldUI = nullptr;
			subRoomMap->trapexcludelocations = nullptr;
			subRoomMap->monsterexcludelocations = nullptr;
			subRoomMap->lootexcludelocations = nullptr;
			if ( fullMapPath.empty() || loadMap(fullMapPath.c_str(), subRoomMap, subRoomMap->entities, subRoomMap->creatures, &checkMapHash) == -1 )
			{
				mapDeconstructor((void*)subRoomMap);
				continue; // failed to load level
			}
			if (!verifyMapHash(fullMapPath.c_str(), checkMapHash))
			{
				conductGameChallenges[CONDUCT_MODDED] = 1;
				Mods::disableSteamAchievements = true;
			}

			// level is successfully loaded, add it to the pool
			subRoomList = (list_t*)malloc(sizeof(list_t));
			subRoomList->first = nullptr;
			subRoomList->last = nullptr;
			node = list_AddNodeLast(&subRoomMapList);
			node->element = subRoomList;
			node->deconstructor = &listDeconstructor;

			node = list_AddNodeLast(subRoomList);
			node->element = subRoomMap;
			node->deconstructor = &mapDeconstructor;

			/*if ( subRoomMap->flags[MAP_FLAG_DISABLETRAPS] == 1 )
			{
				printlog("%s: no traps", subRoomMap->filename);
			}
			if ( subRoomMap->flags[MAP_FLAG_DISABLEMONSTERS] == 1 )
			{
				printlog("%s: no monsters", subRoomMap->filename);
			}
			if ( subRoomMap->flags[MAP_FLAG_DISABLELOOT] == 1 )
			{
				printlog("%s: no loot", subRoomMap->filename);
			}*/

			// more nodes are created to record the exit points on the sublevel
			for ( int y = 0; y < subRoomMap->height; y++ )
			{
				for ( int x = 0; x < subRoomMap->width; x++ )
				{
					if ( x == 0 || y == 0 || x == subRoomMap->width - 1 || y == subRoomMap->height - 1 )
					{
						if ( !subRoomMap->tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * subRoomMap->height] )
						{
							door = (door_t*)malloc(sizeof(door_t));
							door->x = x;
							door->y = y;
							if ( x == subRoomMap->width - 1 )
							{
								door->dir = door_t::DIR_EAST;
								if ( y == subRoomMap->height - 1 )
								{
									door->edge = door_t::EDGE_SOUTHEAST;
								}
								else if ( y == 0 )
								{
									door->edge = door_t::EDGE_NORTHEAST;
								}
								else
								{
									door->edge = door_t::EDGE_EAST;
								}
							}
							else if ( y == subRoomMap->height - 1 )
							{
								door->dir = door_t::DIR_SOUTH;
								if ( x == subRoomMap->width - 1 )
								{
									door->edge = door_t::EDGE_SOUTHEAST;
								}
								else if ( x == 0 )
								{
									door->edge = door_t::EDGE_SOUTHWEST;
								}
								else
								{
									door->edge = door_t::EDGE_SOUTH;
								}
							}
							else if ( x == 0 )
							{
								door->dir = door_t::DIR_WEST;
								if ( y == subRoomMap->height - 1 )
								{
									door->edge = door_t::EDGE_SOUTHWEST;
								}
								else if ( y == 0 )
								{
									door->edge = door_t::EDGE_NORTHWEST;
								}
								else
								{
									door->edge = door_t::EDGE_WEST;
								}
							}
							else if ( y == 0 )
							{
								door->dir = door_t::DIR_NORTH;
								if ( x == subRoomMap->width - 1 )
								{
									door->edge = door_t::EDGE_NORTHEAST;
								}
								else if ( x == 0 )
								{
									door->edge = door_t::EDGE_NORTHWEST;
								}
								else
								{
									door->edge = door_t::EDGE_NORTH;
								}
							}
							node2 = list_AddNodeLast(subRoomList);
							node2->element = door;
							node2->deconstructor = &defaultDeconstructor;
						}
					}
				}
			}
		}
	}

	for ( char letter = 'a'; letter <= 'z' && shoplevel && shopSubRooms.shopFileName.size() > 0; letter++ )
	{
		// look for mapnames ending in a letter a to z
		char shopSubRoomName[64];
		snprintf(shopSubRoomName, sizeof(shopSubRoomName), "%s%c", shopSubRooms.shopFileName.c_str(), letter);
		fullMapPath = physfsFormatMapName(shopSubRoomName);

		if ( fullMapPath.empty() )
		{
			break;    // no more levels to load
		}

		// check if there is another subroom to load
		//if ( !dataPathExists(fullMapPath.c_str()) )
		//{
		//	break;    // no more levels to load
		//}

		printlog("[SUBMAP GENERATOR] Found map lv %s, count: %d", shopSubRoomName, shopSubRooms.count);
		++shopSubRooms.count;

		// allocate memory for the next subroom and attempt to load it
		map_t* subRoomMap = (map_t*)malloc(sizeof(map_t));
		subRoomMap->tiles = nullptr;
		subRoomMap->entities = (list_t*)malloc(sizeof(list_t));
		subRoomMap->entities->first = nullptr;
		subRoomMap->entities->last = nullptr;
		subRoomMap->creatures = new list_t;
		subRoomMap->creatures->first = nullptr;
		subRoomMap->creatures->last = nullptr;
		subRoomMap->worldUI = nullptr;
		subRoomMap->trapexcludelocations = nullptr;
		subRoomMap->monsterexcludelocations = nullptr;
		subRoomMap->lootexcludelocations = nullptr;
		if ( fullMapPath.empty() || loadMap(fullMapPath.c_str(), subRoomMap, subRoomMap->entities, subRoomMap->creatures, &checkMapHash) == -1 )
		{
			mapDeconstructor((void*)subRoomMap);
			continue; // failed to load level
		}
		if (!verifyMapHash(fullMapPath.c_str(), checkMapHash))
		{
			conductGameChallenges[CONDUCT_MODDED] = 1;
			Mods::disableSteamAchievements = true;
		}

		// level is successfully loaded, add it to the pool
		subRoomList = (list_t*)malloc(sizeof(list_t));
		subRoomList->first = nullptr;
		subRoomList->last = nullptr;

		node = list_AddNodeLast(&shopSubRooms.list);
		node->element = subRoomList;
		node->deconstructor = &listDeconstructor;

		node = list_AddNodeLast(subRoomList);
		node->element = subRoomMap;
		node->deconstructor = &mapDeconstructor;

		// more nodes are created to record the exit points on the sublevel
		for ( y = 0; y < subRoomMap->height; y++ )
		{
			for ( x = 0; x < subRoomMap->width; x++ )
			{
				if ( x == 0 || y == 0 || x == subRoomMap->width - 1 || y == subRoomMap->height - 1 )
				{
					if ( !subRoomMap->tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * subRoomMap->height] )
					{
						door = (door_t*)malloc(sizeof(door_t));
						door->x = x;
						door->y = y;
						if ( x == subRoomMap->width - 1 )
						{
							door->dir = door_t::DIR_EAST;
							if ( y == subRoomMap->height - 1 )
							{
								door->edge = door_t::EDGE_SOUTHEAST;
							}
							else if ( y == 0 )
							{
								door->edge = door_t::EDGE_NORTHEAST;
							}
							else
							{
								door->edge = door_t::EDGE_EAST;
							}
						}
						else if ( y == subRoomMap->height - 1 )
						{
							door->dir = door_t::DIR_SOUTH;
							if ( x == subRoomMap->width - 1 )
							{
								door->edge = door_t::EDGE_SOUTHEAST;
							}
							else if ( x == 0 )
							{
								door->edge = door_t::EDGE_SOUTHWEST;
							}
							else
							{
								door->edge = door_t::EDGE_SOUTH;
							}
						}
						else if ( x == 0 )
						{
							door->dir = door_t::DIR_WEST;
							if ( y == subRoomMap->height - 1 )
							{
								door->edge = door_t::EDGE_SOUTHWEST;
							}
							else if ( y == 0 )
							{
								door->edge = door_t::EDGE_NORTHWEST;
							}
							else
							{
								door->edge = door_t::EDGE_WEST;
							}
						}
						else if ( y == 0 )
						{
							door->dir = door_t::DIR_NORTH;
							if ( x == subRoomMap->width - 1 )
							{
								door->edge = door_t::EDGE_NORTHEAST;
							}
							else if ( x == 0 )
							{
								door->edge = door_t::EDGE_NORTHWEST;
							}
							else
							{
								door->edge = door_t::EDGE_NORTH;
							}
						}
						node2 = list_AddNodeLast(subRoomList);
						node2->element = door;
						node2->deconstructor = &defaultDeconstructor;
					}
				}
			}
		}
	}

	StartRoomInfo_t startRoomInfo;

	// generate dungeon level...
	int roomcount = 0;
	if ( numlevels > 1 )
	{
		possiblelocations = (bool*) malloc(sizeof(bool) * map.width * map.height);
		map.trapexcludelocations = (bool*)malloc(sizeof(bool) * map.width * map.height);
		map.monsterexcludelocations = (bool*)malloc(sizeof(bool) * map.width * map.height);
		map.lootexcludelocations = (bool*)malloc(sizeof(bool) * map.width * map.height);
		for ( y = 0; y < map.height; y++ )
		{
			for ( x = 0; x < map.width; x++ )
			{
				if ( x < (std::max(2, getMapPossibleLocationX1()))
					|| y < (std::max(2, getMapPossibleLocationY1())) 
					|| x > (std::min(getMapPossibleLocationX2(), (int)map.width - 3))
					|| y > (std::min(getMapPossibleLocationY2(), (int)map.height - 3)) )
				{
					possiblelocations[x + y * map.width] = false;
				}
				else
				{
					possiblelocations[x + y * map.width] = true;
				}
				map.trapexcludelocations[x + y * map.width] = false;
				if ( map.flags[MAP_FLAG_DISABLEMONSTERS] == 1 )
				{
					// the base map excludes all monsters
					map.monsterexcludelocations[x + y * map.width] = true;
				}
				else
				{
					map.monsterexcludelocations[x + y * map.width] = false;
				}
				if ( map.flags[MAP_FLAG_DISABLELOOT] == 1 )
				{
					// the base map excludes all monsters
					map.lootexcludelocations[x + y * map.width] = true;
				}
				else
				{
					map.lootexcludelocations[x + y * map.width] = false;
				}
			}
		}
		possiblelocations2 = (bool*) malloc(sizeof(bool) * map.width * map.height);
		firstroomtile = (bool*) malloc(sizeof(bool) * map.width * map.height);
		possiblerooms = (bool*) malloc(sizeof(bool) * numlevels);
		for ( c = 0; c < numlevels; c++ )
		{
			possiblerooms[c] = true;
		}
		levellimit = (map.width * map.height);
		for ( c = 0; c < levellimit; c++ )
		{
			// reset array of possible locations for the current room
			for ( y = 0; y < map.height; y++ )
			{
				for ( x = 0; x < map.width; x++ )
				{
					possiblelocations2[x + y * map.width] = true;
				}
			}
			doorNode = nullptr;

			// pick the room to be used
			if ( c == 0 )
			{
				levelnum = 0; // the first room *must* be an entrance hall
				levelnum2 = 0;
				numlevels--;
				possiblerooms[0] = false;
				node = mapList.first;
				node = ((list_t*)node->element)->first;
				doorNode = node->next;
				tempMap = (map_t*)node->element;
			}
			else if ( c == 1 && secretlevelexit )
			{
				secretlevelmap.tiles = nullptr;
				secretlevelmap.entities = (list_t*) malloc(sizeof(list_t));
				secretlevelmap.entities->first = nullptr;
				secretlevelmap.entities->last = nullptr;
				secretlevelmap.creatures = new list_t;
				secretlevelmap.creatures->first = nullptr;
				secretlevelmap.creatures->last = nullptr;
				secretlevelmap.worldUI = nullptr;
				char secretmapname[128];
				switch ( secretlevelexit )
				{
					case 1:
						strcpy(secretmapname, "minesecret");
						break;
					case 2:
						strcpy(secretmapname, "swampsecret");
						break;
					case 3:
						strcpy(secretmapname, "labyrinthsecret");
						break;
					case 4:
						strcpy(secretmapname, "ruinssecret");
						break;
					case 5:
						strcpy(secretmapname, "cavessecret");
						break;
					case 6:
						strcpy(secretmapname, "citadelsecret");
						break;
					case 7:
						strcpy(secretmapname, levelset);
						strcat(secretmapname, "secret");
						break;
					case 8:
						strcpy(secretmapname, "baphoexit");
						break;
					default:
						break;
				}
				fullMapPath = physfsFormatMapName(secretmapname);
				if ( fullMapPath.empty() || loadMap(fullMapPath.c_str(), &secretlevelmap, secretlevelmap.entities, secretlevelmap.creatures, &checkMapHash) == -1 )
				{
					list_FreeAll(secretlevelmap.entities);
					free(secretlevelmap.entities);
					list_FreeAll(secretlevelmap.creatures);
					delete secretlevelmap.creatures;
					if ( secretlevelmap.tiles )
					{
						free(secretlevelmap.tiles);
					}
				}
				if (!verifyMapHash(fullMapPath.c_str(), checkMapHash))
				{
					conductGameChallenges[CONDUCT_MODDED] = 1;
					Mods::disableSteamAchievements = true;
				}

				levelnum = 0;
				levelnum2 = -1;
				tempMap = &secretlevelmap;
			}
			else if ( c == 2 && shoplevel )
			{
				// generate a shop
				levelnum = 0;
				levelnum2 = -1;
				tempMap = &shopmap;
			}
			else
			{
				if ( !numlevels )
				{
					break;
				}
				levelnum = map_rng.rand() % (numlevels); // draw randomly from the pool

				// traverse the map list to the picked level
				node = mapList.first;
				i = 0;
				j = -1;
				while (1)
				{
					if (possiblerooms[i])
					{
						++j;
						if (j == levelnum)
						{
							break;
						}
					}
					node = node->next;
					++i;
				}
				levelnum2 = i;
				node = ((list_t*)node->element)->first;
				doorNode = node->next;
				tempMap = (map_t*)node->element;
			}

			// find locations where the selected room can be added to the level
			numpossiblelocations = map.width * map.height;

			bool hellGenerationFix = !strncmp(map.name, "Hell", 4) && !MFLAG_GENADJACENTROOMS;

			for ( y0 = 0; y0 < map.height; y0++ )
			{
				for ( x0 = 0; x0 < map.width; x0++ )
				{
					for ( y1 = y0; y1 < std::min(y0 + tempMap->height, map.height); y1++ )
					{
						// don't generate start room in hell along the rightmost wall, causes pathing to fail. Check 2 tiles to the right extra
						// to try fit start room.
						for ( x1 = x0; x1 < std::min(x0 + tempMap->width + ((hellGenerationFix && c == 0) ? 2 : 0), map.width); x1++ )
						{
							if ( possiblelocations[x1 + y1 * map.width] == false && possiblelocations2[x0 + y0 * map.width] == true )
							{
								possiblelocations2[x0 + y0 * map.width] = false;
								numpossiblelocations--;
							}
						}
					}
				}
			}

			// in case no locations are available, remove this room from the selection
			if ( numpossiblelocations <= 0 )
			{
				if ( levelnum2 >= 0 && levelnum2 < numlevels )
				{
					possiblerooms[levelnum2] = false;
				}
				--numlevels;
				if ( levelnum2 == 0 )
				{
					// if we couldn't even fit the entrance hall into the dungeon, we have a problem
					free(possiblerooms);
					free(possiblelocations);
					free(possiblelocations2);
					if ( map.trapexcludelocations )
					{
						free(map.trapexcludelocations);
						map.trapexcludelocations = nullptr;
					}
					if ( map.monsterexcludelocations )
					{
						free(map.monsterexcludelocations);
						map.monsterexcludelocations = nullptr;
					}
					if ( map.lootexcludelocations )
					{
						free(map.lootexcludelocations);
						map.lootexcludelocations = nullptr;
					}
					free(firstroomtile);
					free(sublevelname);
					free(subRoomName);
					list_FreeAll(&subRoomMapList);
					list_FreeAll(&mapList);
					if ( shoplevel && c == 2 )
					{
						list_FreeAll(shopmap.entities);
						free(shopmap.entities);
						list_FreeAll(shopmap.creatures);
						delete shopmap.creatures;
						if ( shopmap.tiles )
						{
							free(shopmap.tiles);
						}
					}
					if ( secretlevelexit && c == 1 )
					{
						list_FreeAll(secretlevelmap.entities);
						free(secretlevelmap.entities);
						list_FreeAll(secretlevelmap.creatures);
						delete secretlevelmap.creatures;
						if ( secretlevelmap.tiles )
						{
							free(secretlevelmap.tiles);
						}
					}
					printlog("error: entrance room must fit into dungeon!\n");
					return -1;
				}
				else if ( numlevels < 1 )
				{
					// if we've run out of rooms to build the dungeon with, skip to the next step of generation
					break;
				}
				continue;
			}

			// otherwise, choose a location from those available (to be stored in x/y)
			if ( MFLAG_GENADJACENTROOMS )
			{
				pickedlocation = 0;
				i = -1;
				x = 0;
				y = 0;

				if ( !strncmp(map.name, "Citadel", 7) )
				{
					if ( c == 0 )
					{
						// 7x7, pick random location across all map.
						x = 2 + (map_rng.rand() % 7) * 7;
						y = 2 + (map_rng.rand() % 7) * 7;
					}
					else if ( secretlevelexit && c == 1 )
					{
						// 14x14, pick random location minus 1 from both edges.
						x = 2 + (map_rng.rand() % 6) * 7;
						y = 2 + (map_rng.rand() % 6) * 7;
					}
					else if ( c == 2 && shoplevel )
					{
						// 7x7, pick random location across all map.
						x = 2 + (map_rng.rand() % 7) * 7;
						y = 2 + (map_rng.rand() % 7) * 7;
					}
				}
				else if ( !strncmp(map.name, "Hell", 4) )
				{
					if ( c == 0 )
					{
						if ( secretlevelexit == 8 )
						{
							x = getMapPossibleLocationX1() + 7;
							y = getMapPossibleLocationY1() + 7;
						}
						else
						{
							// 7x7, pick random location across all map.
							x = getMapPossibleLocationX1() + (1 + map_rng.rand() % 4) * 7;
							y = getMapPossibleLocationY1() + (1 + map_rng.rand() % 4) * 7;
						}
					}
					else if ( secretlevelexit == 8 && c == 1 )
					{
						x = getMapPossibleLocationX1() + (3) * 7;
						y = getMapPossibleLocationY1() + (3) * 7;
					}
					else if ( c == 2 && shoplevel )
					{
						// 7x7, pick random location across all map.
						x = 2 + (map_rng.rand() % 6) * 7;
						y = 2 + (map_rng.rand() % 6) * 7;
					}
				}
				else
				{
					if ( c == 0 )
					{
						// pick random location across all map.
						x = 2 + (map_rng.rand() % tempMap->width) * tempMap->width;
						y = 2 + (map_rng.rand() % tempMap->height) * tempMap->height;
					}
					else if ( secretlevelexit && c == 1 )
					{
						x = 2 + (map_rng.rand() % tempMap->width) * tempMap->width;
						y = 2 + (map_rng.rand() % tempMap->height) * tempMap->height;
						while ( x + tempMap->width >= map.width )
						{
							x = 2 + (map_rng.rand() % tempMap->width) * tempMap->width;
						}
						while ( y + tempMap->height >= map.height )
						{
							y = 2 + (map_rng.rand() % tempMap->height) * tempMap->height;
						}
					}
					else if ( c == 2 && shoplevel )
					{
						// pick random location across all map.
						x = 2 + (map_rng.rand() % tempMap->width) * tempMap->width;
						y = 2 + (map_rng.rand() % tempMap->height) * tempMap->height;
					}
				}

				while ( 1 )
				{
					if ( possiblelocations2[x + y * map.width] == true )
					{
						++i;
						if ( i == pickedlocation )
						{
							break;
						}
					}
					++x;
					if ( x >= map.width )
					{
						x = 0;
						++y;
						if ( y >= map.height )
						{
							y = 0;
							++pickedlocation;
						}
					}
				}
			}
			else
			{
				pickedlocation = map_rng.rand() % numpossiblelocations;
				i = -1;
				x = 0;
				y = 0;
				while ( 1 )
				{
					if ( possiblelocations2[x + y * map.width] == true )
					{
						++i;
						if ( i == pickedlocation )
						{
							break;
						}
					}
					++x;
					if ( x >= map.width )
					{
						x = 0;
						++y;
						if ( y >= map.height )
						{
							y = 0;
						}
					}
				}
			}

			// now copy all the geometry from the sublevel to the chosen location
			if ( c == 0 )
			{
				for ( z = 0; z < map.width * map.height; ++z )
				{
					firstroomtile[z] = false;
				}
			}
			x1 = x + tempMap->width;
			y1 = y + tempMap->height;


			//**********pick subroom if available
			int pickSubRoom = 0;
			int subRoom_tilex = 0;
			int subRoom_tiley = 0;
			int subRoom_tileStartx = -1;
			int subRoom_tileStarty = -1;
			bool foundSubRoom = false;
			if ( c == 2 && shoplevel && tempMap == &shopmap && shopSubRooms.count > 0 )
			{
				pickSubRoom = map_rng.rand() % shopSubRooms.count;
				subRoomNode = shopSubRooms.list.first;
				int k = 0;
				while ( 1 )
				{
					if ( k == pickSubRoom )
					{
						break;
					}
					subRoomNode = subRoomNode->next;
					k++;
				}
				subRoomNode = ((list_t*)subRoomNode->element)->first;
				subRoomMap = (map_t*)subRoomNode->element;
				subRoomDoorNode = subRoomNode->next;
			}
			else
			{
				if ( ((levelnum2 - levelnum) > 1) && (c > 0) && (subroomCount[levelnum2] > 0) )
				{
					// levelnum is the start of map search, levelnum2 is jumps required to get to a suitable map.
					// normal operation is levelnum2 - levelnum == 1. if a levelnum map is unavailable, 
					// then levelnum2 will advance search by 1 (higher than normal).
					// levelnum2 will keep incrementing until a suitable map is found.
					printlog("[SUBMAP GENERATOR] Skipped map when searching for levelnum %d, setting to %d", levelnum, levelnum2 - 1);
					levelnum = levelnum2 - 1;
				}
				//printlog("(%d | %d), possible: (%d, %d) x: %d y: %d", levelnum, levelnum2, possiblerooms[1], possiblerooms[2], x, y);
				if ( subroomCount[levelnum + 1] > 0 )
				{
					int jumps = 0;
					pickSubRoom = map_rng.rand() % subroomCount[levelnum + 1];
					// traverse the map list to the picked level
					subRoomNode = subRoomMapList.first;
					for ( int cycleRooms = 0; (cycleRooms < levelnum + 1) && (subRoomNode != nullptr); ++cycleRooms )
					{
						for ( int cycleRoomSubMaps = subroomCount[cycleRooms]; cycleRoomSubMaps > 0; --cycleRoomSubMaps )
						{
							// advance the subroom map list by the previous entries.
							// e.g 2 subrooms, 3 maps each should advance pointer 3 maps when loading second room.
							subRoomNode = subRoomNode->next;
							jumps++; // just to keep track of how many jumps we made.
						}
					}
					int k = 0;
					while ( 1 )
					{
						if ( k == pickSubRoom )
						{
							break;
						}
						subRoomNode = subRoomNode->next;
						k++;
					}
					//messagePlayer(0, "%d + %d jumps!", jumps, k + 1);
					subRoomNode = ((list_t*)subRoomNode->element)->first;
					subRoomMap = (map_t*)subRoomNode->element;
					subRoomDoorNode = subRoomNode->next;
				}
			}

			int subroomLogCount = 0;
			if ( shopSubRooms.count > 0 )
			{
				subroomLogCount = shopSubRooms.count;
			}
			else
			{
				subroomLogCount = subroomCount[levelnum + 1];
			}
			for ( z = 0; z < MAPLAYERS; z++ )
			{
				for ( y0 = y; y0 < y1; y0++ )
				{
					for ( x0 = x; x0 < x1; x0++ )
					{
						if ( (subroomLogCount > 0) && tempMap->tiles[z + (y0 - y) * MAPLAYERS + (x0 - x) * MAPLAYERS * tempMap->height] == 201 )
						{
							if ( !foundSubRoom )
							{
								subRoom_tileStartx = x0;
								subRoom_tileStarty = y0;
								foundSubRoom = true;
								if ( shoplevel && c == 2 )
								{
									printlog("Picked level: %d from %d possible rooms in submap %s at x:%d y:%d", pickSubRoom + 1, subroomLogCount, shopSubRooms.shopFileName.c_str(), x, y);
								}
								else
								{
									printlog("Picked level: %d from %d possible rooms in submap %d at x:%d y:%d", pickSubRoom + 1, subroomLogCount, levelnum + 1, x, y);
								}
							}

							map.tiles[z + y0 * MAPLAYERS + x0 * MAPLAYERS * map.height] = subRoomMap->tiles[z + (subRoom_tiley)* MAPLAYERS + (subRoom_tilex)* MAPLAYERS * subRoomMap->height];

							if ( z == 0 )
							{
								// apply submap disable flags
								if ( subRoomMap->flags[MAP_FLAG_DISABLETRAPS] == 1 )
								{
									map.trapexcludelocations[x0 + y0 * map.width] = true;
									//map.tiles[z + y0 * MAPLAYERS + x0 * MAPLAYERS * map.height] = 83;
								}
								if ( subRoomMap->flags[MAP_FLAG_DISABLEMONSTERS] == 1 )
								{
									map.monsterexcludelocations[x0 + y0 * map.width] = true;
								}
								if ( subRoomMap->flags[MAP_FLAG_DISABLELOOT] == 1 )
								{
									map.lootexcludelocations[x0 + y0 * map.width] = true;
								}
							}

							++subRoom_tilex;
							if ( subRoom_tilex >= subRoomMap->width )
							{
								subRoom_tilex = 0;
								++subRoom_tiley;
								if ( subRoom_tiley >= subRoomMap->height )
								{
									subRoom_tiley = 0;
								}
							}
						}
						else
						{
							map.tiles[z + y0 * MAPLAYERS + x0 * MAPLAYERS * map.height] = tempMap->tiles[z + (y0 - y) * MAPLAYERS + (x0 - x) * MAPLAYERS * tempMap->height];
						}

						if ( z == 0 )
						{
							possiblelocations[x0 + y0 * map.width] = false;
							if ( tempMap->flags[MAP_FLAG_DISABLETRAPS] == 1 )
							{
								map.trapexcludelocations[x0 + y0 * map.width] = true;
								//map.tiles[z + y0 * MAPLAYERS + x0 * MAPLAYERS * map.height] = 83;
							}
							if ( tempMap->flags[MAP_FLAG_DISABLEMONSTERS] == 1 )
							{
								map.monsterexcludelocations[x0 + y0 * map.width] = true;
							}
							if ( tempMap->flags[MAP_FLAG_DISABLELOOT] == 1 )
							{
								map.lootexcludelocations[x0 + y0 * map.width] = true;
							}
							if ( c == 0 )
							{
								firstroomtile[y0 + x0 * map.height] = true;
								startRoomInfo.addCoord(x0, y0);
							}
							else if ( c == 1 && secretlevelexit == 8 && !strncmp(map.name, "Hell", 4) )
							{
								firstroomtile[y0 + x0 * map.height] = true;
							}
							else if ( c == 2 && shoplevel )
							{
								firstroomtile[y0 + x0 * map.height] = true;
								if ( x0 - x > 0 && y0 - y > 0 && x0 - x < tempMap->width - 1 && y0 - y < tempMap->height - 1 )
								{
									shoparea[y0 + x0 * map.height] = true;
								}
							}
						}

						// remove any existing entities in this region too
						for ( node = map.entities->first; node != nullptr; node = nextnode )
						{
							nextnode = node->next;
							Entity* entity = (Entity*)node->element;
							if ( (int)entity->x == x0 << 4 && (int)entity->y == y0 << 4 )
							{
								list_RemoveNode(entity->mynode);
							}
						}
					}
				}
			}

			// copy the entities as well from the tempMap.
			for ( node = tempMap->entities->first; node != nullptr; node = node->next )
			{
				entity = (Entity*)node->element;
				childEntity = newEntity(entity->sprite, 1, map.entities, nullptr);

				// entity will return nullptr on getStats called in setSpriteAttributes as behaviour &actmonster is not set.
				// check if the monster sprite is correct and set the behaviour manually for getStats.
				if ( checkSpriteType(entity->sprite) == 1 && multiplayer != CLIENT )
				{
					entity->behavior = &actMonster;
				}

				setSpriteAttributes(childEntity, entity, entity);
				childEntity->x = entity->x + x * 16;
				childEntity->y = entity->y + y * 16;
				childEntity->mapGenerationRoomX = x;
				childEntity->mapGenerationRoomY = y;
				//printlog("1 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);

				if ( entity->behavior == &actMonster || entity->behavior == &actPlayer )
				{
					entity->addToCreatureList(map.creatures);
				}
			}

			if ( foundSubRoom )
			{
				// copy the entities from subroom
				for ( subRoomNode = subRoomMap->entities->first; subRoomNode != nullptr; subRoomNode = subRoomNode->next )
				{
					entity = (Entity*)subRoomNode->element;
					childEntity = newEntity(entity->sprite, 1, map.entities, nullptr);

					// entity will return nullptr on getStats called in setSpriteAttributes as behaviour &actmonster is not set.
					// check if the monster sprite is correct and set the behaviour manually for getStats.
					if ( checkSpriteType(entity->sprite) == 1 && multiplayer != CLIENT )
					{
						entity->behavior = &actMonster;
					}

					setSpriteAttributes(childEntity, entity, entity);
					childEntity->x = entity->x + subRoom_tileStartx * 16;
					childEntity->y = entity->y + subRoom_tileStarty * 16;
					childEntity->mapGenerationRoomX = subRoom_tileStartx;
					childEntity->mapGenerationRoomY = subRoom_tileStarty;
					if ( entity->behavior == &actMonster || entity->behavior == &actPlayer )
					{
						entity->addToCreatureList(map.creatures);
					}

					//messagePlayer(0, "1 Generated entity. Sprite: %d X: %.2f Y: %.2f", childEntity->sprite, childEntity->x / 16, childEntity->y / 16);
				}
			}

			// finally, copy the doors into a single doors list
			while ( doorNode != nullptr )
			{
				door = (door_t*)doorNode->element;
				newDoor = (door_t*) malloc(sizeof(door_t));
				newDoor->x = door->x + x;
				newDoor->y = door->y + y;
				newDoor->dir = door->dir;
				newDoor->edge = door->edge;
				node = list_AddNodeLast(&doorList);
				node->element = newDoor;
				node->deconstructor = &defaultDeconstructor;
				doorNode = doorNode->next;
			}

			if ( foundSubRoom )
			{
				// copy subroom doors
				while ( subRoomDoorNode != nullptr )
				{
					door = (door_t*)subRoomDoorNode->element;
					newDoor = (door_t*)malloc(sizeof(door_t));
					newDoor->x = door->x + subRoom_tileStartx;
					newDoor->y = door->y + subRoom_tileStarty;
					newDoor->dir = door->dir;
					newDoor->edge = door->edge;
					node = list_AddNodeLast(&doorList);
					node->element = newDoor;
					node->deconstructor = &defaultDeconstructor;
					subRoomDoorNode = subRoomDoorNode->next;
				}
			}

			// free shop map if used
			if ( shoplevel && c == 2 )
			{
				list_FreeAll(shopmap.entities);
				free(shopmap.entities);
				list_FreeAll(shopmap.creatures);
				delete shopmap.creatures;
				if ( shopmap.tiles )
				{
					free(shopmap.tiles);
				}
			}
			if ( secretlevelexit && c == 1 )
			{
				list_FreeAll(secretlevelmap.entities);
				free(secretlevelmap.entities);
				list_FreeAll(secretlevelmap.creatures);
				delete secretlevelmap.creatures;
				if ( secretlevelmap.tiles )
				{
					free(secretlevelmap.tiles);
				}
			}
			++roomcount;
		}
		list_FreeAll(&shopSubRooms.list);
		free(possiblerooms);
		free(possiblelocations2);
	}
	else
	{
		list_FreeAll(&shopSubRooms.list);
		free(subRoomName);
		free(sublevelname);
		list_FreeAll(&subRoomMapList);
		list_FreeAll(&mapList);
		list_FreeAll(&doorList);
		printlog("error: not enough levels to begin generating dungeon.\n");
		return -1;
	}

	// post-processing:

	// gates
	for ( node = doorList.first; node != nullptr; node = node->next )  // loop through gates first to delete conflicting gates/doors
	{
		door = (door_t*)node->element;
		for (node2 = map.entities->first; node2 != nullptr; node2 = node2->next)
		{
			entity = (Entity*)node2->element;
			if ( entity->x / 16 == door->x && entity->y / 16 == door->y 
				&& (/*entity->sprite == 2 || entity->sprite == 3 ||*/ entity->sprite == 19 || entity->sprite == 20
					|| entity->sprite == 113 || entity->sprite == 114) )
			{
				int doordir = door->dir;

				// if door is on a corner, then determine the proper facing based on the entity dir
				if ( doordir == door_t::DIR_EAST || doordir == door_t::DIR_WEST )
				{
					if ( (entity->sprite == 3 || entity->sprite == 19 || entity->sprite == 113) ) // north/south sprites
					{
						switch ( door->edge )
						{
							case door_t::EDGE_SOUTHEAST:
							case door_t::EDGE_SOUTHWEST:
								doordir = door_t::DIR_SOUTH;
								break;
							case door_t::EDGE_NORTHEAST:
							case door_t::EDGE_NORTHWEST:
								doordir = door_t::DIR_NORTH;
								break;
							case door_t::EDGE_EAST:
							case door_t::EDGE_WEST:
								continue; // no need to process this door as it is facing internal map contents
							default:
								break;
						}
					}
				}
				else if ( doordir == door_t::DIR_SOUTH || doordir == door_t::DIR_NORTH )
				{
					if ( (entity->sprite == 2 || entity->sprite == 20 || entity->sprite == 114) ) // east/west sprites
					{
						switch ( door->edge )
						{
							case door_t::EDGE_SOUTHEAST:
							case door_t::EDGE_NORTHEAST:
								doordir = door_t::DIR_EAST;
								break;
							case door_t::EDGE_NORTHWEST:
							case door_t::EDGE_SOUTHWEST:
								doordir = door_t::DIR_WEST;
								break;
							case door_t::EDGE_SOUTH:
							case door_t::EDGE_NORTH:
								continue; // no need to process this door as it is facing internal map contents
							default:
								break;
						}
					}
				}

				switch ( doordir )
				{
					case door_t::DIR_EAST: // east
						map.tiles[OBSTACLELAYER + door->y * MAPLAYERS + (door->x + 1)*MAPLAYERS * map.height] = 0;
						for ( node3 = map.entities->first; node3 != nullptr; node3 = nextnode )
						{
							entity = (Entity*)node3->element;
							nextnode = node3->next;
							if ( mapSpriteIsDoorway(entity->sprite) )
							{
								if ( (int)(entity->x / 16) == door->x + 2 && (int)(entity->y / 16) == door->y 
									&& (entity->sprite == 3 || entity->sprite == 19 || entity->sprite == 113) ) // north/south doors 2 tiles away
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x + 1 && (int)(entity->y / 16) == door->y )
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x + 1 && (int)(entity->y / 16) == door->y + 1 )
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x + 1 && (int)(entity->y / 16) == door->y - 1 )
								{
									list_RemoveNode(entity->mynode);
								}
							}
						}
						break;
					case door_t::DIR_SOUTH: // south
						map.tiles[OBSTACLELAYER + (door->y + 1)*MAPLAYERS + door->x * MAPLAYERS * map.height] = 0;
						for ( node3 = map.entities->first; node3 != nullptr; node3 = nextnode )
						{
							entity = (Entity*)node3->element;
							nextnode = node3->next;
							if ( mapSpriteIsDoorway(entity->sprite) )
							{
								if ( (int)(entity->x / 16) == door->x && (int)(entity->y / 16) == door->y + 2
									&& (entity->sprite == 2 || entity->sprite == 20 || entity->sprite == 114) ) // east/west doors 2 tiles away
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x && (int)(entity->y / 16) == door->y + 1 )
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x + 1 && (int)(entity->y / 16) == door->y + 1 )
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x - 1 && (int)(entity->y / 16) == door->y + 1 )
								{
									list_RemoveNode(entity->mynode);
								}
							}
						}
						break;
					case door_t::DIR_WEST: // west
						map.tiles[OBSTACLELAYER + door->y * MAPLAYERS + (door->x - 1)*MAPLAYERS * map.height] = 0;
						for ( node3 = map.entities->first; node3 != nullptr; node3 = nextnode )
						{
							entity = (Entity*)node3->element;
							nextnode = node3->next;
							if ( mapSpriteIsDoorway(entity->sprite) )
							{
								if ( (int)(entity->x / 16) == door->x - 2 && (int)(entity->y / 16) == door->y
									&& (entity->sprite == 3 || entity->sprite == 19 || entity->sprite == 113) ) // north/south doors 2 tiles away
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x - 1 && (int)(entity->y / 16) == door->y )
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x - 1 && (int)(entity->y / 16) == door->y + 1 )
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x - 1 && (int)(entity->y / 16) == door->y - 1 )
								{
									list_RemoveNode(entity->mynode);
								}
							}
						}
						break;
					case door_t::DIR_NORTH: // north
						map.tiles[OBSTACLELAYER + (door->y - 1)*MAPLAYERS + door->x * MAPLAYERS * map.height] = 0;
						for ( node3 = map.entities->first; node3 != nullptr; node3 = nextnode )
						{
							entity = (Entity*)node3->element;
							nextnode = node3->next;
							if ( mapSpriteIsDoorway(entity->sprite) )
							{
								if ( (int)(entity->x / 16) == door->x && (int)(entity->y / 16) == door->y - 2
									&& (entity->sprite == 2 || entity->sprite == 20 || entity->sprite == 114) ) // east/west doors 2 tiles away
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x && (int)(entity->y / 16) == door->y - 1 )
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x + 1 && (int)(entity->y / 16) == door->y - 1 )
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x - 1 && (int)(entity->y / 16) == door->y - 1 )
								{
									list_RemoveNode(entity->mynode);
								}
							}
						}
						break;
				}
			}
		}
	}

	// doors
	for ( node = doorList.first; node != nullptr; node = node->next ) // now loop through doors to delete conflicting gates/doors
	{
		door = (door_t*)node->element;
		for ( node2 = map.entities->first; node2 != nullptr; node2 = node2->next )
		{
			entity = (Entity*)node2->element;
			if ( entity->x / 16 == door->x && entity->y / 16 == door->y
				&& (entity->sprite == 2 || entity->sprite == 3/* || entity->sprite == 19 || entity->sprite == 20
															  || entity->sprite == 113 || entity->sprite == 114*/) )
			{
				int doordir = door->dir;

				// if door is on a corner, then determine the proper facing based on the entity dir
				if ( doordir == door_t::DIR_EAST || doordir == door_t::DIR_WEST )
				{
					if ( (entity->sprite == 3 || entity->sprite == 19 || entity->sprite == 113) ) // north/south sprites
					{
						switch ( door->edge )
						{
							case door_t::EDGE_SOUTHEAST:
							case door_t::EDGE_SOUTHWEST:
								doordir = door_t::DIR_SOUTH;
								break;
							case door_t::EDGE_NORTHEAST:
							case door_t::EDGE_NORTHWEST:
								doordir = door_t::DIR_NORTH;
								break;
							case door_t::EDGE_EAST:
							case door_t::EDGE_WEST:
								continue; // no need to process this door as it is facing internal map contents
								break;
							default:
								break;
						}
					}
				}
				else if ( doordir == door_t::DIR_SOUTH || doordir == door_t::DIR_NORTH )
				{
					if ( (entity->sprite == 2 || entity->sprite == 20 || entity->sprite == 114) ) // east/west sprites
					{
						switch ( door->edge )
						{
							case door_t::EDGE_SOUTHEAST:
							case door_t::EDGE_NORTHEAST:
								doordir = door_t::DIR_EAST;
								break;
							case door_t::EDGE_NORTHWEST:
							case door_t::EDGE_SOUTHWEST:
								doordir = door_t::DIR_WEST;
								break;
							case door_t::EDGE_SOUTH:
							case door_t::EDGE_NORTH:
								continue; // no need to process this door as it is facing internal map contents
								break;
							default:
								break;
						}
					}
				}

				switch ( doordir )
				{
					case door_t::DIR_EAST: // east
						map.tiles[OBSTACLELAYER + door->y * MAPLAYERS + (door->x + 1)*MAPLAYERS * map.height] = 0;
						for ( node3 = map.entities->first; node3 != nullptr; node3 = nextnode )
						{
							entity = (Entity*)node3->element;
							nextnode = node3->next;
							if ( mapSpriteIsDoorway(entity->sprite) )
							{
								if ( (int)(entity->x / 16) == door->x + 2 && (int)(entity->y / 16) == door->y
									&& (entity->sprite == 3 || entity->sprite == 19 || entity->sprite == 113) ) // north/south doors 2 tiles away
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x + 1 && (int)(entity->y / 16) == door->y )
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x + 1 && (int)(entity->y / 16) == door->y + 1 )
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x + 1 && (int)(entity->y / 16) == door->y - 1 )
								{
									list_RemoveNode(entity->mynode);
								}
							}
						}
						break;
					case door_t::DIR_SOUTH: // south
						map.tiles[OBSTACLELAYER + (door->y + 1)*MAPLAYERS + door->x * MAPLAYERS * map.height] = 0;
						for ( node3 = map.entities->first; node3 != nullptr; node3 = nextnode )
						{
							entity = (Entity*)node3->element;
							nextnode = node3->next;
							if ( mapSpriteIsDoorway(entity->sprite) )
							{
								if ( (int)(entity->x / 16) == door->x && (int)(entity->y / 16) == door->y + 2
									&& (entity->sprite == 2 || entity->sprite == 20 || entity->sprite == 114) ) // east/west doors 2 tiles away
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x && (int)(entity->y / 16) == door->y + 1 )
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x + 1 && (int)(entity->y / 16) == door->y + 1 )
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x - 1 && (int)(entity->y / 16) == door->y + 1 )
								{
									list_RemoveNode(entity->mynode);
								}
							}
						}
						break;
					case door_t::DIR_WEST: // west
						map.tiles[OBSTACLELAYER + door->y * MAPLAYERS + (door->x - 1)*MAPLAYERS * map.height] = 0;
						for ( node3 = map.entities->first; node3 != nullptr; node3 = nextnode )
						{
							entity = (Entity*)node3->element;
							nextnode = node3->next;
							if ( mapSpriteIsDoorway(entity->sprite) )
							{
								if ( (int)(entity->x / 16) == door->x - 2 && (int)(entity->y / 16) == door->y
									&& (entity->sprite == 3 || entity->sprite == 19 || entity->sprite == 113) ) // north/south doors 2 tiles away
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x - 1 && (int)(entity->y / 16) == door->y )
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x - 1 && (int)(entity->y / 16) == door->y + 1 )
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x - 1 && (int)(entity->y / 16) == door->y - 1 )
								{
									list_RemoveNode(entity->mynode);
								}
							}
						}
						break;
					case door_t::DIR_NORTH: // north
						map.tiles[OBSTACLELAYER + (door->y - 1)*MAPLAYERS + door->x * MAPLAYERS * map.height] = 0;
						for ( node3 = map.entities->first; node3 != nullptr; node3 = nextnode )
						{
							entity = (Entity*)node3->element;
							nextnode = node3->next;
							if ( mapSpriteIsDoorway(entity->sprite) )
							{
								if ( (int)(entity->x / 16) == door->x && (int)(entity->y / 16) == door->y - 2
									&& (entity->sprite == 2 || entity->sprite == 20 || entity->sprite == 114) ) // east/west doors 2 tiles away
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x && (int)(entity->y / 16) == door->y - 1 )
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x + 1 && (int)(entity->y / 16) == door->y - 1 )
								{
									list_RemoveNode(entity->mynode);
								}
								else if ( (int)(entity->x / 16) == door->x - 1 && (int)(entity->y / 16) == door->y - 1 )
								{
									list_RemoveNode(entity->mynode);
								}
							}
						}
						break;
				}
			}
		}
	}
	bool foundsubmaptile = false;
	// if for whatever reason some submap 201 tiles didn't get filled in, let's get rid of those.
	for ( z = 0; z < MAPLAYERS; ++z )
	{
		for ( y = 1; y < map.height; ++y )
		{
			for ( x = 1; x < map.height; ++x )
			{
				if ( map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height] == 201 )
				{
					map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height] = 0;
					foundsubmaptile = true;
				}
			}
		}
	}
	if ( foundsubmaptile )
	{
		printlog("[SUBMAP GENERATOR] Found some junk tiles!");
	}

	for ( node = map.entities->first; node != nullptr; node = node->next )
	{
		// fix gate air-gap borders on citadel map next to perimeter gates.
		if ( !strncmp(map.name, "Citadel", 7) )
		{
			Entity* gateEntity = (Entity*)node->element;
			if ( gateEntity->sprite == 19 || gateEntity->sprite == 20 ) // N/S E/W gates take these sprite numbers in the editor.
			{
				int gatex = static_cast<int>(gateEntity->x) / 16;
				int gatey = static_cast<int>(gateEntity->y) / 16;
				for ( z = OBSTACLELAYER; z < MAPLAYERS; ++z )
				{
					if ( gateEntity->x / 16 == 1 ) // along leftmost edge
					{
						if ( !map.tiles[z + gatey * MAPLAYERS + (gatex + 1) * MAPLAYERS * map.height] )
						{
							map.tiles[z + gatey * MAPLAYERS + (gatex + 1) * MAPLAYERS * map.height] = 230;
							//messagePlayer(0, "replaced at: %d, %d", gatex, gatey);
						}
					}
					else if ( gateEntity->x / 16 == 51 ) // along rightmost edge
					{
						if ( !map.tiles[z + gatey * MAPLAYERS + (gatex - 1) * MAPLAYERS * map.height] )
						{
							map.tiles[z + gatey * MAPLAYERS + (gatex - 1) * MAPLAYERS * map.height] = 230;
							//messagePlayer(0, "replaced at: %d, %d", gatex, gatey);
						}
					}
					else if ( gateEntity->y / 16 == 1 ) // along top edge
					{
						if ( !map.tiles[z + (gatey + 1) * MAPLAYERS + gatex * MAPLAYERS * map.height] )
						{
							map.tiles[z + (gatey + 1) * MAPLAYERS + gatex * MAPLAYERS * map.height] = 230;
							//messagePlayer(0, "replaced at: %d, %d", gatex, gatey);
						}
					}
					else if ( gateEntity->y / 16 == 51 ) // along bottom edge
					{
						if ( !map.tiles[z + (gatey - 1) * MAPLAYERS + gatex * MAPLAYERS * map.height] )
						{
							map.tiles[z + (gatey - 1) * MAPLAYERS + gatex * MAPLAYERS * map.height] = 230;
							//messagePlayer(0, "replaced at: %d, %d", gatex, gatey);
						}
					}
				}
			}
		}
	}

	bool customTrapsForMapInUse = false;
	struct CustomTraps
	{
		bool boulders = false;
		bool arrows = false;
		bool spikes = false;
		bool verticalSpelltraps = false;
	} customTraps;

	if ( gameplayCustomManager.inUse() && gameplayCustomManager.mapGenerationExistsForMapName(map.name) )
	{
		auto m = gameplayCustomManager.getMapGenerationForMapName(map.name);
		if ( m && m->usingTrapTypes )
		{
			customTrapsForMapInUse = true;
			for ( auto& traps : m->trapTypes )
			{
				if ( traps.compare("boulders") == 0 )
				{
					customTraps.boulders = true;
				}
				else if ( traps.compare("arrows") == 0 )
				{
					customTraps.arrows = true;
				}
				else if ( traps.compare("spikes") == 0 )
				{
					customTraps.spikes = true;
				}
				else if ( traps.compare("spelltrap_vertical") == 0 )
				{
					customTraps.verticalSpelltraps = true;
				}
			}
		}
	}

	// boulder and arrow traps
	if ( (svFlags & SV_FLAG_TRAPS) && map.flags[MAP_FLAG_DISABLETRAPS] == 0
		&& (!customTrapsForMapInUse || (customTrapsForMapInUse && (customTraps.boulders || customTraps.arrows)) )
		)
	{
		numpossiblelocations = 0;
		for ( c = 0; c < map.width * map.height; ++c )
		{
			possiblelocations[c] = false;
		}
		std::unordered_map<int, int> trapLocationAndSide;
		for ( y = 1; y < map.height - 1; ++y )
		{
			for ( x = 1; x < map.width - 1; ++x )
			{
				int sides = 0;
				if ( firstroomtile[y + x * map.height] )
				{
					continue;
				}
				if ( !map.tiles[OBSTACLELAYER + y * MAPLAYERS + (x + 1)*MAPLAYERS * map.height] )
				{
					sides++;
				}
				if ( !map.tiles[OBSTACLELAYER + (y + 1)*MAPLAYERS + x * MAPLAYERS * map.height] )
				{
					sides++;
				}
				if ( !map.tiles[OBSTACLELAYER + y * MAPLAYERS + (x - 1)*MAPLAYERS * map.height] )
				{
					sides++;
				}
				if ( !map.tiles[OBSTACLELAYER + (y - 1)*MAPLAYERS + x * MAPLAYERS * map.height] )
				{
					sides++;
				}
				int side = 0;
				if ( !map.tiles[OBSTACLELAYER + y * MAPLAYERS + (x + 1)*MAPLAYERS * map.height] )
				{
					side = 0;
				}
				else if ( !map.tiles[OBSTACLELAYER + (y + 1)*MAPLAYERS + x * MAPLAYERS * map.height] )
				{
					side = 1;
				}
				else if ( !map.tiles[OBSTACLELAYER + y * MAPLAYERS + (x - 1)*MAPLAYERS * map.height] )
				{
					side = 2;
				}
				else if ( !map.tiles[OBSTACLELAYER + (y - 1)*MAPLAYERS + x * MAPLAYERS * map.height] )
				{
					side = 3;
				}
				if ( sides == 1 && (map.trapexcludelocations[x + y * map.width] == false) )
				{
					possiblelocations[y + x * map.height] = true;
					numpossiblelocations++;

					int trapTileX = x + (side == 0 ? 1 : 0) + (side == 2 ? -1 : 0);
					int trapTileY = y + (side == 1 ? 1 : 0) + (side == 3 ? -1 : 0);
					trapLocationAndSide[trapTileX + trapTileY * 10000] = side;
				}
			}
		}

		// don't spawn traps in doors
		node_t* doorNode;
		for ( doorNode = doorList.first; doorNode != nullptr; doorNode = doorNode->next )
		{
			door_t* door = (door_t*)doorNode->element;
			int x = std::min<unsigned int>(std::max(0, door->x), map.width - 1); //TODO: Why are const int and unsigned int being compared?
			int y = std::min<unsigned int>(std::max(0, door->y), map.height - 1); //TODO: Why are const int and unsigned int being compared?
			if ( possiblelocations[y + x * map.height] == true )
			{
				possiblelocations[y + x * map.height] = false;
				--numpossiblelocations;
			}
		}

		bool arrowtrappotential = false;
		if ( !strncmp(map.name, "Hell", 4) )
		{
			arrowtrappotential = true;
		}
		else if ( currentlevel > 5 && currentlevel <= 25 )
		{
			arrowtrappotential = true;
		}

		std::vector<Entity*> ceilingTilesConflictingWithBoulders;
		std::vector<Entity*> ceilingTilesToDeleteForBoulders;

		// do a second pass to look for internal doorways
		for ( node = map.entities->first; node != nullptr; node = node->next )
		{
			entity = (Entity*)node->element;
			int x = entity->x / 16;
			int y = entity->y / 16;
			if ( (x >= 1 && x < map.width - 1)
				&& (y >= 1 && y < map.height - 1) )
			{
				if ( mapSpriteIsDoorway(entity->sprite) )
				{
					auto find = trapLocationAndSide.find(x + y * 10000);
					if ( find != trapLocationAndSide.end() )
					{
						int side = find->second;
						int trapx = x + (side == 0 ? -1 : 0) + (side == 2 ? 1 : 0);
						int trapy = y + (side == 1 ? -1 : 0) + (side == 3 ? 1 : 0);
						if ( possiblelocations[trapy + trapx * map.height] )
						{
							possiblelocations[trapy + trapx * map.height] = false;
							--numpossiblelocations;
						}
					}
				}
				else if ( entity->sprite == 119 ) // ceiling tile
				{
					if ( entity->ceilingTileAllowTrap == 0 )
					{
						if ( !arrowtrappotential )
						{
							auto find = trapLocationAndSide.find(x + y * 10000);
							if ( find != trapLocationAndSide.end() )
							{
								int side = find->second;
								int trapx = x + (side == 0 ? -1 : 0) + (side == 2 ? 1 : 0);
								int trapy = y + (side == 1 ? -1 : 0) + (side == 3 ? 1 : 0);
								if ( possiblelocations[trapy + trapx * map.height] )
								{
									possiblelocations[trapy + trapx * map.height] = false;
									--numpossiblelocations;
								}
							}
						}
						else
						{
							ceilingTilesConflictingWithBoulders.push_back(entity);
						}
					}
					else if ( entity->ceilingTileAllowTrap == 1 )
					{
						ceilingTilesToDeleteForBoulders.push_back(entity);
					}
				}
				else if ( entity->sprite == 179 && entity->colliderHasCollision == 1 ) // collider
				{
					auto find = trapLocationAndSide.find(x + y * 10000);
					if ( find != trapLocationAndSide.end() )
					{
						int side = find->second;
						int trapx = x + (side == 0 ? -1 : 0) + (side == 2 ? 1 : 0);
						int trapy = y + (side == 1 ? -1 : 0) + (side == 3 ? 1 : 0);
						if ( possiblelocations[trapy + trapx * map.height] )
						{
							possiblelocations[trapy + trapx * map.height] = false;
							--numpossiblelocations;
						}
					}
				}
			}
		}

		int whatever = map_rng.rand() % 5;
		if ( strncmp(map.name, "Hell", 4) )
			j = std::min(
			        std::min(
			            std::max(1, currentlevel),
			            5
			        )
			        + whatever, numpossiblelocations
			    )
			    / ((strcmp(map.name, "The Mines") == 0) + 1);
		else
		{
			j = std::min(15, numpossiblelocations);
		}
		//printlog("j: %d\n",j);
		//printlog("numpossiblelocations: %d\n",numpossiblelocations);
		for ( c = 0; c < j; ++c )
		{
			// choose a random location from those available
			pickedlocation = map_rng.rand() % numpossiblelocations;
			i = -1;
			//printlog("pickedlocation: %d\n",pickedlocation);
			//printlog("numpossiblelocations: %d\n",numpossiblelocations);
			x = 0;
			y = 0;
			while ( 1 )
			{
				if ( possiblelocations[y + x * map.height] == true )
				{
					i++;
					if ( i == pickedlocation )
					{
						break;
					}
				}
				x++;
				if ( x >= map.width )
				{
					x = 0;
					y++;
					if ( y >= map.height )
					{
						y = 0;
					}
				}
			}
			int side = 0;
			bool nofloor = false;
			if ( !map.tiles[OBSTACLELAYER + y * MAPLAYERS + (x + 1)*MAPLAYERS * map.height] )
			{
				side = 0;
				if ( !map.tiles[y * MAPLAYERS + (x + 1)*MAPLAYERS * map.height] )
				{
					nofloor = true;
				}
			}
			else if ( !map.tiles[OBSTACLELAYER + (y + 1)*MAPLAYERS + x * MAPLAYERS * map.height] )
			{
				side = 1;
				if ( !map.tiles[(y + 1)*MAPLAYERS + x * MAPLAYERS * map.height] )
				{
					nofloor = true;
				}
			}
			else if ( !map.tiles[OBSTACLELAYER + y * MAPLAYERS + (x - 1)*MAPLAYERS * map.height] )
			{
				side = 2;
				if ( !map.tiles[y * MAPLAYERS + (x - 1)*MAPLAYERS * map.height] )
				{
					nofloor = true;
				}
			}
			else if ( !map.tiles[OBSTACLELAYER + (y - 1)*MAPLAYERS + x * MAPLAYERS * map.height] )
			{
				side = 3;
				if ( !map.tiles[(y - 1)*MAPLAYERS + x * MAPLAYERS * map.height] )
				{
					nofloor = true;
				}
			}
			bool arrowtrap = false;
			bool noceiling = false;
			bool arrowtrapspawn = false;
			if ( !strncmp(map.name, "Hell", 4) )
			{
				if ( side == 0 && !map.tiles[(MAPLAYERS - 1) + y * MAPLAYERS + (x + 1)*MAPLAYERS * map.height] )
				{
					noceiling = true;
				}
				if ( side == 1 && !map.tiles[(MAPLAYERS - 1) + (y + 1)*MAPLAYERS + x * MAPLAYERS * map.height] )
				{
					noceiling = true;
				}
				if ( side == 2 && !map.tiles[(MAPLAYERS - 1) + y * MAPLAYERS + (x - 1)*MAPLAYERS * map.height] )
				{
					noceiling = true;
				}
				if ( side == 3 && !map.tiles[(MAPLAYERS - 1) + (y - 1)*MAPLAYERS + x * MAPLAYERS * map.height] )
				{
					noceiling = true;
				}
				if ( noceiling )
				{
					arrowtrapspawn = true;
				}
			}
			else
			{
				if ( !strncmp(map.name, "Underworld", 10) )
				{
					arrowtrapspawn = true; // no boulders in underworld
				}
				else if ( map_rng.rand() % 2 && (arrowtrappotential) )
				{
					arrowtrapspawn = true;
				}
			}

			if ( customTrapsForMapInUse )
			{
				arrowtrapspawn = customTraps.arrows;
				if ( customTraps.boulders && map_rng.rand() % 2 )
				{
					arrowtrapspawn = false;
				}
			}

			// check if ceiling tiles prevent boulders
			if ( !arrowtrapspawn )
			{
				for ( auto itr = ceilingTilesConflictingWithBoulders.begin();
					itr != ceilingTilesConflictingWithBoulders.end(); ++itr )
				{
					auto ceilingTile = *itr;
					int tx = ceilingTile->x / 16;
					int ty = ceilingTile->y / 16;

					int trapLocationX = x + ((side == 0) ? 1 : 0) + ((side == 2) ? -1 : 0);
					int trapLocationY = y + ((side == 1) ? 1 : 0) + ((side == 3) ? -1 : 0);
					if ( tx == trapLocationX && ty == trapLocationY )
					{
						arrowtrapspawn = true;
						break;
					}
				}
			}

			if ( arrowtrapspawn || noceiling || (nofloor && arrowtrappotential) )
			{
				arrowtrap = true;
				entity = newEntity(32, 1, map.entities, nullptr); // arrow trap
				entity->behavior = &actArrowTrap;
				map.tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map.height] = 53; // trap wall
			}
			else
			{
				//messagePlayer(0, "Included at x: %d, y: %d", x, y);
				entity = newEntity(38, 1, map.entities, nullptr); // boulder trap
				entity->behavior = &actBoulderTrap;

				// delete ceiling tiles if need be
				for ( auto itr = ceilingTilesToDeleteForBoulders.begin();
					itr != ceilingTilesToDeleteForBoulders.end(); )
				{
					auto ceilingTile = *itr;
					int tx = ceilingTile->x / 16;
					int ty = ceilingTile->y / 16;
					
					int trapLocationX = x + ((side == 0) ? 1 : 0) + ((side == 2) ? -1 : 0);
					int trapLocationY = y + ((side == 1) ? 1 : 0) + ((side == 3) ? -1 : 0);
					if ( tx == trapLocationX && ty == trapLocationY )
					{
						list_RemoveNode(ceilingTile->mynode);
						itr = ceilingTilesToDeleteForBoulders.erase(itr);
					}
					else
					{
						++itr;
					}
				}
			}
			entity->x = x * 16;
			entity->y = y * 16;
			//printlog("2 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",entity->sprite,entity->getUID(),entity->x,entity->y);
			//entity = newEntity(18, 1, map.entities, nullptr); // electricity node
			//entity->x = x * 16;
			//entity->y = y * 16;
			//printlog("4 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",entity->sprite,entity->getUID(),entity->x,entity->y);
			// make torches
			if ( arrowtrap )
			{
				entity = newEntity(4 + side, 1, map.entities, nullptr);
				Entity* entity2 = newEntity(4 + side, 1, map.entities, nullptr);
				switch ( side )
				{
					case 0:
						entity->x = x * 16 + 16;
						entity->y = y * 16 + 4;
						entity2->x = x * 16 + 16;
						entity2->y = y * 16 - 4;
						break;
					case 1:
						entity->x = x * 16 + 4;
						entity->y = y * 16 + 16;
						entity2->x = x * 16 - 4;
						entity2->y = y * 16 + 16;
						break;
					case 2:
						entity->x = x * 16 - 16;
						entity->y = y * 16 + 4;
						entity2->x = x * 16 - 16;
						entity2->y = y * 16 - 4;
						break;
					case 3:
						entity->x = x * 16 + 4;
						entity->y = y * 16 - 16;
						entity2->x = x * 16 - 4;
						entity2->y = y * 16 - 16;
						break;
				}
				//printlog("5 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",entity->sprite,entity->getUID(),entity->x,entity->y);
				//printlog("6 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",entity2->sprite,entity2->getUID(),entity2->x,entity2->y);
			}
			i = 0;
			int testx = 0, testy = 0;
			do
			{
				if ( i == 0 )
				{
					// get rid of extraneous torch
					node_t* tempNode;
					node_t* nextTempNode;
					for ( tempNode = map.entities->first; tempNode != nullptr; tempNode = nextTempNode )
					{
						nextTempNode = tempNode->next;
						Entity* tempEntity = (Entity*)tempNode->element;
						if ( tempEntity->sprite >= 4 && tempEntity->sprite <= 7 )
						{
							if ( ((int)floor(tempEntity->x + 8)) / 16 == x && ((int)floor(tempEntity->y + 8)) / 16 == y )
							{
								list_RemoveNode(tempNode);
							}
						}
					}
				}
				else
				{
					if ( arrowtrap )
					{
						entity = newEntity(33, 1, map.entities, nullptr); // pressure plate
					}
					else
					{
						entity = newEntity(34, 1, map.entities, nullptr); // pressure plate
					}
					entity->x = x * 16;
					entity->y = y * 16;
					//printlog("7 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",entity->sprite,entity->getUID(),entity->x,entity->y);
					entity = newEntity(18, 1, map.entities, nullptr); // electricity node
					entity->x = x * 16;
					entity->y = y * 16;
					//printlog("8 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",entity->sprite,entity->getUID(),entity->x,entity->y);
				}
				switch ( side )
				{
					case 0:
						x++;
						break;
					case 1:
						y++;
						break;
					case 2:
						x--;
						break;
					case 3:
						y--;
						break;
				}
				testx = std::min(std::max<unsigned int>(0, x), map.width - 1); //TODO: Why are const int and unsigned int being compared?
				testy = std::min(std::max<unsigned int>(0, y), map.height - 1); //TODO: Why are const int and unsigned int being compared?
				i++;
			}
			while ( !map.tiles[OBSTACLELAYER + testy * MAPLAYERS + testx * MAPLAYERS * map.height] 
				&& !map.trapexcludelocations[testx + testy * map.width]
				&& !(!arrowtrap && !map.tiles[testy * MAPLAYERS + testx * MAPLAYERS * map.height]) // boulders stop wiring at pit edges
				&& i <= 10 );
		}
	}

	// check start room for accessibility to rest of level
	if ( strncmp(map.name, "Underworld", 10) )
	{
		startRoomInfo.checkBorderAccessibility();
	}

	std::vector<int> underworldEmptyTiles;
	std::vector<int> waterEmptyTiles;
	std::vector<int> lavaEmptyTiles;

	// monsters, decorations, and items
	numpossiblelocations = map.width * map.height;
	for ( y = 0; y < map.height; y++ )
	{
		for ( x = 0; x < map.width; x++ )
		{
			if ( firstroomtile[y + x * map.height] )
			{
				possiblelocations[y + x * map.height] = false;
				numpossiblelocations--;
			}
			else if ( x < getMapPossibleLocationX1() || x >= getMapPossibleLocationX2()
				|| y < getMapPossibleLocationY1() || y >= getMapPossibleLocationY2() )
			{
				possiblelocations[y + x * map.height] = false;
				--numpossiblelocations;
			}
			else if ( lavatiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] )
			{
				possiblelocations[y + x * map.height] = false;
				numpossiblelocations--;

				if ( map.monsterexcludelocations[x + y * map.width] == false )
				{
					if ( !checkObstacle(x * 16 + 8, y * 16 + 8, NULL, NULL, false, true, false) )
					{
						lavaEmptyTiles.push_back(x + y * 1000);
					}
				}
			}
			else if ( swimmingtiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] )
			{
				possiblelocations[y + x * map.height] = false;
				numpossiblelocations--;

				if ( map.monsterexcludelocations[x + y * map.width] == false )
				{
					if ( !checkObstacle(x * 16 + 8, y * 16 + 8, NULL, NULL, false, true, false) )
					{
						waterEmptyTiles.push_back(x + y * 1000);
					}
				}
			}
			else
			{
				if ( checkObstacle(x * 16 + 8, y * 16 + 8, NULL, NULL, false, true, false) )
				{
					possiblelocations[y + x * map.height] = false;
					--numpossiblelocations;
				}
				else if ( !map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height] )
				{
					possiblelocations[y + x * map.height] = false;
					numpossiblelocations--;

					if ( !strncmp(map.name, "Underworld", 10) )
					{
						underworldEmptyTiles.push_back(x + y * 1000);
					}
				}
				else
				{
					possiblelocations[y + x * map.height] = true;
				}
			}
		}
	}
	for ( node = map.entities->first; node != nullptr; node = node->next )
	{
		entity = (Entity*)node->element;
		x = entity->x / 16;
		y = entity->y / 16;
		if ( x >= 0 && x < map.width && y >= 0 && y < map.height )
		{
			if ( possiblelocations[y + x * map.height] )
			{
				possiblelocations[y + x * map.height] = false;
				--numpossiblelocations;
			}
		}
	}

	// read some editor map data if available:
	int genEntityMin = 0;
	int genEntityMax = 0;
	int genMonsterMin = 0;
	int genMonsterMax = 0;
	int genLootMin = 0;
	int genLootMax = 0;
	int genDecorationMin = 0;
	int genDecorationMax = 0;

	if ( map.flags[MAP_FLAG_GENBYTES1] != 0 || map.flags[MAP_FLAG_GENBYTES2] != 0 )
	{
		genEntityMin = (map.flags[MAP_FLAG_GENBYTES1] >> 24) & 0xFF; // first leftmost byte
		genEntityMax = (map.flags[MAP_FLAG_GENBYTES1] >> 16) & 0xFF; // second leftmost byte

		genMonsterMin = (map.flags[MAP_FLAG_GENBYTES1] >> 8) & 0xFF; // third leftmost byte
		genMonsterMax = (map.flags[MAP_FLAG_GENBYTES1] >> 0) & 0xFF; // fourth leftmost byte
		
		genLootMin = (map.flags[MAP_FLAG_GENBYTES2] >> 24) & 0xFF; // first leftmost byte
		genLootMax = (map.flags[MAP_FLAG_GENBYTES2] >> 16) & 0xFF; // second leftmost byte

		genDecorationMin = (map.flags[MAP_FLAG_GENBYTES2] >> 8) & 0xFF; // third leftmost byte
		genDecorationMax = (map.flags[MAP_FLAG_GENBYTES2] >> 0) & 0xFF; // fourth leftmost byte
	}

	int entitiesToGenerate = 30;
	int randomEntities = 10;

	if ( genEntityMin > 0 || genEntityMax > 0 )
	{
		genEntityMin = std::max(genEntityMin, 2); // make sure there's room for a ladder.
		entitiesToGenerate = genEntityMin;
		randomEntities = std::max(genEntityMax - genEntityMin, 1); // difference between min and max is the extra chances.
		//Needs to be 1 for map_rng.rand() % to not divide by 0.
		j = std::min<Uint32>(entitiesToGenerate + map_rng.rand() % randomEntities, numpossiblelocations); //TODO: Why are Uint32 and Sin32 being compared?
	}
	else
	{
		// revert to old mechanics.
		j = std::min<Uint32>(30 + map_rng.rand() % 10, numpossiblelocations); //TODO: Why are Uint32 and Sin32 being compared?
	}
	int forcedMonsterSpawns = 0;
	int forcedLootSpawns = 0;
	int forcedDecorationSpawns = 0;

	if ( genMonsterMin > 0 || genMonsterMax > 0 )
	{
		forcedMonsterSpawns = genMonsterMin + map_rng.rand() % std::max(genMonsterMax - genMonsterMin, 1);
	}
	if ( genLootMin > 0 || genLootMax > 0 )
	{
		forcedLootSpawns = genLootMin + map_rng.rand() % std::max(genLootMax - genLootMin, 1);
	}
	if ( genDecorationMin > 0 || genDecorationMax > 0 )
	{
		forcedDecorationSpawns = genDecorationMin + map_rng.rand() % std::max(genDecorationMax - genDecorationMin, 1);
	}

	//messagePlayer(0, "Num locations: %d of %d possible, force monsters: %d, force loot: %d, force decorations: %d", j, numpossiblelocations, forcedMonsterSpawns, forcedLootSpawns, forcedDecorationSpawns);
	printlog("Num locations: %d of %d possible, force monsters: %d, force loot: %d, force decorations: %d", j, numpossiblelocations, forcedMonsterSpawns, forcedLootSpawns, forcedDecorationSpawns);
	int numGenItems = 0;
	int numGenGold = 0;
	int numGenDecorations = 0;

	std::vector<Uint32> itemsGeneratedList;
	static ConsoleVariable<bool> cvar_underworldshrinetest("/underworldshrinetest", false);

	int exit_x = -1;
	int exit_y = -1;
	//printlog("j: %d\n",j);
	//printlog("numpossiblelocations: %d\n",numpossiblelocations);
	for ( c = 0; c < std::min(j, numpossiblelocations); ++c )
	{
		// choose a random location from those available
		pickedlocation = map_rng.rand() % numpossiblelocations;
		i = -1;
		//printlog("pickedlocation: %d\n",pickedlocation);
		//printlog("numpossiblelocations: %d\n",numpossiblelocations);
		x = 0;
		y = 0;
		bool skipPossibleLocationsDecrement = false;
		while ( 1 )
		{
			if ( possiblelocations[y + x * map.height] == true )
			{
				++i;
				if ( i == pickedlocation )
				{
					break;
				}
			}
			++x;
			if ( x >= map.width )
			{
				x = 0;
				++y;
				if ( y >= map.height )
				{
					y = 0;
				}
			}
		}

		// create entity
		entity = nullptr;
		if ( (c == 0 || (minotaurlevel && c < 2)) && (!secretlevel || currentlevel != 7) && (!secretlevel || currentlevel != 20)
			&& std::get<LEVELPARAM_DISABLE_NORMAL_EXIT>(mapParameters) == 0 )
		{
			if ( !strcmp(map.name, "Hell") && secretlevelexit == 8 )
			{
				continue; // no generate exit
			}

			// daedalus shrine
			if ( c == 1 && minotaurlevel && !(secretlevel && (currentlevel == 7 || currentlevel == 20)) )
			{
				int numShrines = 1;
				/*if ( !strncmp(map.name, "The Labyrinth", 13) )
				{
					numShrines = 3;
				}*/
				std::map<int, std::vector<int>> goodspots;

				// generate in different quadrant than exit
				int exitquadrant = 0;
				if ( exit_x >= map.width / 2 )
				{
					if ( exit_y >= map.height / 2 )
					{
						exitquadrant = 3; // northwest is opposite
					}
					else
					{
						exitquadrant = 2; // southwest is opposite
					}
				}
				else
				{
					if ( exit_y >= map.height / 2 )
					{
						exitquadrant = 0; // northeast is opposite
					}
					else
					{
						exitquadrant = 1; // southeast is opposite
					}
				}

				std::vector<int> quadrantOrder;
				switch ( exitquadrant )
				{
				case 0:
					quadrantOrder = { 0, 1, 3, 2 };
					break;
				case 1:
					quadrantOrder = { 1, 2, 0, 3 };
					break;
				case 2:
					quadrantOrder = { 2, 3, 1, 0 };
					break;
				case 3:
					quadrantOrder = { 3, 0, 2, 1 };
					break;
				default:
					break;
				}

				for ( int y = 0; y < map.height; ++y )
				{
					for ( int x = 0; x < map.width; ++x )
					{
						if ( possiblelocations[y + x * map.height] == true )
						{
							int quadrant = 0;
							if ( x >= map.width / 2 )
							{
								if ( y >= map.height / 2 )
								{
									quadrant = 1; // southeast
								}
								else
								{
									quadrant = 0; // northeast
								}
							}
							else
							{
								if ( y >= map.height / 2 )
								{
									quadrant = 2; // southwest
								}
								else
								{
									quadrant = 3; // northwest
								}
							}
							goodspots[quadrant].push_back(x + y * 1000);
						}
					}
				}

				std::set<int> obstacleSpots;
				bool foundspot = false;
				while ( quadrantOrder.size() > 0 )
				{
					int quadrant = quadrantOrder[0];
					quadrantOrder.erase(quadrantOrder.begin());

					while ( goodspots[quadrant].size() > 0 )
					{
						int index = map_rng.rand() % goodspots[quadrant].size();
						int picked = goodspots[quadrant][index];

						goodspots[quadrant].erase(goodspots[quadrant].begin() + index);

						int x = picked % 1000;
						int y = picked / 1000;
						int obstacles = 0;
						for ( int x2 = -1; x2 <= 1; x2++ )
						{
							for ( int y2 = -1; y2 <= 1; y2++ )
							{
								if ( obstacleSpots.find((x + x2) + 1000 * (y + y2)) != obstacleSpots.end()
									|| checkObstacle((x + x2) * 16, (y + y2) * 16, NULL, NULL, false) )
								{
									obstacles++;
									obstacleSpots.insert((x + x2) + 1000 * (y + y2));
									if ( obstacles > 1 )
									{
										break;
									}
								}
							}
							if ( obstacles > 1 )
							{
								break;
							}
						}
						if ( obstacles > 1 )
						{
							continue;
						}

						// good spot
						Entity* entity = newEntity(11, 1, map.entities, nullptr);
						entity->behavior = &actLadder;

						// determine if the ladder generated in a viable location
						if ( strncmp(map.name, "Underworld", 10) )
						{
							bool nopath = false;
							bool hellLadderFix = !strncmp(map.name, "Hell", 4);
							std::vector<Entity*> tempPassableEntities;
							if ( hellLadderFix )
							{
								for ( node = map.entities->first; node != NULL; node = node->next )
								{
									if ( (entity2 = (Entity*)node->element) )
									{
										if ( entity2->sprite == 19 || entity2->sprite == 20
											|| entity2->sprite == 113 || entity2->sprite == 114 )
										{
											int entx = entity2->x / 16;
											int enty = entity2->y / 16;
											if ( !entity2->flags[PASSABLE] )
											{
												if ( entx >= startRoomInfo.x1 && entx <= startRoomInfo.x2
													&& enty >= startRoomInfo.y1 && enty <= startRoomInfo.y2 )
												{
													tempPassableEntities.push_back(entity2);
													entity2->flags[PASSABLE] = true;
												}
											}
										}
									}
								}
							}
							for ( node = map.entities->first; node != NULL; node = node->next )
							{
								entity2 = (Entity*)node->element;
								if ( entity2->sprite == 1 ) // note entity->behavior == nullptr at this point
								{
									list_t* path = generatePath(x, y, entity2->x / 16, entity2->y / 16,
										entity, entity2, GeneratePathTypes::GENERATE_PATH_CHECK_EXIT, hellLadderFix);
									if ( path == NULL )
									{
										nopath = true;
									}
									else
									{
										list_FreeAll(path);
										free(path);
									}
									break;
								}
							}
							for ( auto ent : tempPassableEntities )
							{
								ent->flags[PASSABLE] = false;
							}
							if ( nopath )
							{
								// try again
								list_RemoveNode(entity->mynode);
								entity = NULL;
								break;
							}
						}

						entity->sprite = 190;
						entity->behavior = &actDaedalusShrine;
						entity->x = 16.0 * x;
						entity->y = 16.0 * y;

						--numpossiblelocations;
						possiblelocations[y + x * map.height] = false;

						skipPossibleLocationsDecrement = true;
						foundspot = true;
						break;
					}

					if ( foundspot )
					{
						foundspot = false;
						--numShrines;
						if ( numShrines <= 0 )
						{
							break;
						}
					}
				}
			}
			else
			{
				// normal exits
				if ( strcmp(map.name, "Hell") )
				{
					entity = newEntity(11, 1, map.entities, nullptr); // ladder
					entity->behavior = &actLadder;
				}
				else
				{
					entity = newEntity(45, 1, map.entities, nullptr); // hell uses portals instead
					entity->behavior = &actPortal;
					entity->skill[3] = 1; // not secret portals though
				}

				// determine if the ladder generated in a viable location
				if ( strncmp(map.name, "Underworld", 10) )
				{
					bool nopath = false;
					bool hellLadderFix = !strncmp(map.name, "Hell", 4);
					std::vector<Entity*> tempPassableEntities;
					if ( hellLadderFix )
					{
						for ( node = map.entities->first; node != NULL; node = node->next )
						{
							if ( (entity2 = (Entity*)node->element) )
							{
								if ( entity2->sprite == 19 || entity2->sprite == 20
									|| entity2->sprite == 113 || entity2->sprite == 114 )
								{
									int entx = entity2->x / 16;
									int enty = entity2->y / 16;
									if ( !entity2->flags[PASSABLE] )
									{
										if ( entx >= startRoomInfo.x1 && entx <= startRoomInfo.x2
											&& enty >= startRoomInfo.y1 && enty <= startRoomInfo.y2 )
										{
											tempPassableEntities.push_back(entity2);
											entity2->flags[PASSABLE] = true;
										}
									}
								}
							}
						}
					}
					for ( node = map.entities->first; node != NULL; node = node->next )
					{
						entity2 = (Entity*)node->element;
						if ( entity2->sprite == 1 ) // note entity->behavior == nullptr at this point
						{
							list_t* path = generatePath(x, y, entity2->x / 16, entity2->y / 16,
								entity, entity2, GeneratePathTypes::GENERATE_PATH_CHECK_EXIT, hellLadderFix);
							if ( path == NULL )
							{
								nopath = true;
							}
							else
							{
								list_FreeAll(path);
								free(path);
							}
							break;
						}
					}
					for ( auto ent : tempPassableEntities )
					{
						ent->flags[PASSABLE] = false;
					}
					if ( nopath )
					{
						// try again
						c--;
						list_RemoveNode(entity->mynode);
						entity = NULL;
					}
					else
					{
						exit_x = x;
						exit_y = y;
					}
				}
			}
		}
		else if ( c == 1 && secretlevel && currentlevel == 7 && !strncmp(map.name, "Underworld", 10) )
		{
			entity = newEntity(89, 1, map.entities, nullptr);
			entity->monsterStoreType = 1;
			entity->skill[5] = nummonsters;
			++nummonsters;
			//entity = newEntity(68, 1, map.entities, nullptr); // magic (artifact) bow
		}
		else if ( *cvar_underworldshrinetest && !strncmp(map.name, "Underworld", 10) 
			&& ((c == 1 && !(secretlevel && currentlevel == 7)) || (c == 2 && secretlevel && currentlevel == 7)) )
		{
			std::set<int> walkableTiles;
			for ( int isley = 1; isley < map.width - 1; ++isley )
			{
				for ( int islex = 1; islex < map.width - 1; ++islex )
				{
					if ( !map.tiles[OBSTACLELAYER + isley * MAPLAYERS + (islex) * MAPLAYERS * map.height]
						&& map.tiles[isley * MAPLAYERS + (islex) * MAPLAYERS * map.height]
						&& !swimmingtiles[map.tiles[isley * MAPLAYERS + islex * MAPLAYERS * map.height]]
						&& !lavatiles[map.tiles[isley * MAPLAYERS + islex * MAPLAYERS * map.height]] )
					{
						walkableTiles.insert(islex + isley * 1000);
					}
				}
			}

			int numIslands = 0;
			std::set<int> reachedTiles;
			struct IslandNode_t
			{
				int neighbours = 0;
			};
			std::map<int, std::map<int, IslandNode_t>> islands;
			for ( auto it = walkableTiles.begin(); it != walkableTiles.end(); ++it )
			{
				if ( reachedTiles.find(*it) == reachedTiles.end() )
				{
					// new island
					std::queue<int> frontier;
					frontier.push(*it);
					reachedTiles.insert(*it);
					while ( !frontier.empty() )
					{
						auto currentKey = frontier.front();
						frontier.pop();

						const int ix = (currentKey) % 1000;
						const int iy = (currentKey) / 1000;

						islands[numIslands][currentKey] = IslandNode_t();

						int checkKey = (ix + 1) + ((iy) * 1000);
						if ( walkableTiles.find(checkKey) != walkableTiles.end()
							&& reachedTiles.find(checkKey) == reachedTiles.end() )
						{
							frontier.push(checkKey);
							reachedTiles.insert(checkKey);
						}
						checkKey = (ix - 1) + ((iy) * 1000);
						if ( walkableTiles.find(checkKey) != walkableTiles.end()
							&& reachedTiles.find(checkKey) == reachedTiles.end() )
						{
							frontier.push(checkKey);
							reachedTiles.insert(checkKey);
						}
						checkKey = (ix) + ((iy + 1) * 1000);
						if ( walkableTiles.find(checkKey) != walkableTiles.end() 
							&& reachedTiles.find(checkKey) == reachedTiles.end() )
						{
							frontier.push(checkKey);
							reachedTiles.insert(checkKey);
						}
						checkKey = (ix) + ((iy - 1) * 1000);
						if ( walkableTiles.find(checkKey) != walkableTiles.end()
							&& reachedTiles.find(checkKey) == reachedTiles.end() )
						{
							frontier.push(checkKey);
							reachedTiles.insert(checkKey);
						}
					}
					if ( !islands[numIslands].empty() )
					{
						++numIslands;
					}
				}
			}

			for ( int isleIndex = 0; isleIndex < numIslands; ++isleIndex )
			{
				int min_x = 1000;
				int max_x = 0;
				int min_y = 1000;
				int max_y = 0;

				std::vector<int> locations3x3;
				for ( auto& isle : islands[isleIndex] )
				{
					const int ix = (isle.first) % 1000;
					const int iy = (isle.first) / 1000;

					max_x = std::max(max_x, ix);
					min_x = std::min(min_x, ix);

					max_y = std::max(max_y, iy);
					min_y = std::min(min_y, iy);

					int checkKey = (ix + 1) + ((iy) * 1000);
					if ( islands[isleIndex].find(checkKey) != islands[isleIndex].end() )
					{
						++isle.second.neighbours;
					}
					checkKey = (ix - 1) + ((iy) * 1000);
					if ( islands[isleIndex].find(checkKey) != islands[isleIndex].end() )
					{
						++isle.second.neighbours;
					}
					checkKey = (ix) + ((iy - 1) * 1000);
					if ( islands[isleIndex].find(checkKey) != islands[isleIndex].end() )
					{
						++isle.second.neighbours;
					}
					checkKey = (ix) + ((iy + 1) * 1000);
					if ( islands[isleIndex].find(checkKey) != islands[isleIndex].end() )
					{
						++isle.second.neighbours;
					}

					checkKey = (ix + 1) + ((iy + 1) * 1000);
					if ( islands[isleIndex].find(checkKey) != islands[isleIndex].end() )
					{
						++isle.second.neighbours;
					}
					checkKey = (ix - 1) + ((iy + 1) * 1000);
					if ( islands[isleIndex].find(checkKey) != islands[isleIndex].end() )
					{
						++isle.second.neighbours;
					}
					checkKey = (ix + 1) + ((iy - 1) * 1000);
					if ( islands[isleIndex].find(checkKey) != islands[isleIndex].end() )
					{
						++isle.second.neighbours;
					}
					checkKey = (ix - 1) + ((iy - 1) * 1000);
					if ( islands[isleIndex].find(checkKey) != islands[isleIndex].end() )
					{
						++isle.second.neighbours;
					}

					if ( isle.second.neighbours == 8 )
					{
						locations3x3.push_back(isle.first);
					}
				}
				printlog("Isle: %d [%d %d] to [%d %d]", isleIndex, min_x, min_y, max_x, max_y);

				if ( !locations3x3.empty() )
				{
					int chosenKey = locations3x3.at(map_rng.rand() % locations3x3.size());
					int dir = map_rng.rand() % 4;
					entity = newEntity(177, 1, map.entities, nullptr);
					setSpriteAttributes(entity, nullptr, nullptr);
					int ix = (chosenKey) % 1000;
					int iy = (chosenKey) / 1000;
					entity->shrineDir = dir;
					if ( dir == 0 )
					{
						ix = ix - 1;
						iy = iy - 1 + map_rng.rand() % 3;
					}
					else if ( dir == 2 )
					{
						ix = ix + 1;
						iy = iy - 1 + map_rng.rand() % 3;
					}
					else if ( dir == 1 )
					{
						ix = ix - 1 + map_rng.rand() % 3;
						iy = iy - 1;
					}
					else if ( dir == 3 )
					{
						ix = ix - 1 + map_rng.rand() % 3;
						iy = iy + 1;
					}
					x = ix;
					y = iy;
					entity->x = x * 16.0;
					entity->y = y * 16.0;
					skipPossibleLocationsDecrement = true;
					possiblelocations[iy + ix * map.height] = false;
					--numpossiblelocations;
				}
			}
		}
		else
		{
			int x2, y2;
			bool nodecoration = false;
			int obstacles = 0;
			for ( x2 = -1; x2 <= 1; x2++ )
			{
				for ( y2 = -1; y2 <= 1; y2++ )
				{
					if ( checkObstacle((x + x2) * 16, (y + y2) * 16, NULL, NULL, false) )
					{
						obstacles++;
						if ( obstacles > 1 )
						{
							break;
						}
					}
				}
				if ( obstacles > 1 )
				{
					break;
				}
			}
			if ( obstacles > 1 )
			{
				nodecoration = true;
			}
			if ( forcedMonsterSpawns > 0 || forcedLootSpawns > 0 || (forcedDecorationSpawns > 0 && !nodecoration) )
			{
				// force monsters, then loot, then decorations.
				if ( forcedMonsterSpawns > 0 )
				{
					--forcedMonsterSpawns;
					if ( map.monsterexcludelocations[x + y * map.width] == false )
					{
						bool doNPC = false;
						if ( gameplayCustomManager.processedPropertyForFloor(currentlevel, secretlevel, map.name, GameplayCustomManager::PROPERTY_NPC, doNPC) )
						{
							// doNPC processed by function
						}
						else if ( map_rng.rand() % 10 == 0 && currentlevel > 1 )
						{
							doNPC = true;
						}

						if ( doNPC )
						{
							if ( currentlevel > 15 && map_rng.rand() % 4 > 0 )
							{
								entity = newEntity(93, 1, map.entities, map.creatures);  // automaton
								if ( currentlevel < 25 )
								{
									entity->monsterStoreType = 1; // weaker version
								}
							}
							else
							{
								entity = newEntity(27, 1, map.entities, map.creatures);  // human
								if ( multiplayer != CLIENT && currentlevel > 5 )
								{
									entity->monsterStoreType = (currentlevel / 5) * 3 + (map_server_rng.rand() % 4); // scale humans with depth.  3 LVL each 5 floors, + 0-3.
								}
							}
						}
						else
						{
							entity = newEntity(10, 1, map.entities, map.creatures);  // monster
						}
						entity->skill[5] = nummonsters;
						++nummonsters;
					}
				}
				else if ( forcedLootSpawns > 0 )
				{
					--forcedLootSpawns;
					if ( map.lootexcludelocations[x + y * map.width] == false )
					{
						if ( map_rng.rand() % 10 == 0 )   // 10% chance
						{
							entity = newEntity(9, 1, map.entities, nullptr);  // gold
							entity->goldAmount = 0;
							numGenGold++;
						}
						else
						{
							entity = newEntity(8, 1, map.entities, nullptr);  // item
							itemsGeneratedList.push_back(entity->getUID());
							setSpriteAttributes(entity, nullptr, nullptr);
							numGenItems++;
						}
					}
				}
				else if ( forcedDecorationSpawns > 0 && !nodecoration )
				{
					--forcedDecorationSpawns;
					// decorations
					if ( (map_rng.rand() % 4 == 0 || (currentlevel <= 10 && !customTrapsForMapInUse)) && strcmp(map.name, "Hell") )
					{
						switch ( map_rng.rand() % 7 )
						{
							case 0:
								entity = newEntity(12, 1, map.entities, nullptr); //Firecamp.
								break; //Firecamp
							case 1:
								entity = newEntity(14, 1, map.entities, nullptr); //Fountain.
								break; //Fountain
							case 2:
								entity = newEntity(15, 1, map.entities, nullptr); //Sink.
								break; //Sink
							case 3:
								entity = newEntity(21, 1, map.entities, nullptr); //Chest.
								setSpriteAttributes(entity, nullptr, nullptr);
								entity->chestLocked = -1;
								break; //Chest
							case 4:
								entity = newEntity(39, 1, map.entities, nullptr); //Tomb.
								break; //Tomb
							case 5:
								entity = newEntity(59, 1, map.entities, nullptr); //Table.
								setSpriteAttributes(entity, nullptr, nullptr);
								break; //Table
							case 6:
								entity = newEntity(60, 1, map.entities, nullptr); //Chair.
								setSpriteAttributes(entity, nullptr, nullptr);
								break; //Chair
						}
					}
					else
					{
						if ( customTrapsForMapInUse )
						{
							if ( !customTraps.spikes && !customTraps.verticalSpelltraps )
							{
								continue;
							}
							else if ( customTraps.verticalSpelltraps && map_rng.rand() % 2 == 0 )
							{
								entity = newEntity(120, 1, map.entities, nullptr); // vertical spell trap.
								setSpriteAttributes(entity, nullptr, nullptr);
							}
							else if ( customTraps.spikes )
							{
								entity = newEntity(64, 1, map.entities, nullptr); // spear trap
							}
						}
						else
						{
							if ( currentlevel <= 25 )
							{
								entity = newEntity(64, 1, map.entities, nullptr); // spear trap
							}
							else
							{
								if ( map_rng.rand() % 2 == 0 )
								{
									entity = newEntity(120, 1, map.entities, nullptr); // vertical spell trap.
									setSpriteAttributes(entity, nullptr, nullptr);
								}
								else
								{
									entity = newEntity(64, 1, map.entities, nullptr); // spear trap
								}
							}
						}
						Entity* also = newEntity(33, 1, map.entities, nullptr);
						also->x = x * 16;
						also->y = y * 16;
						//printlog("15 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",also->sprite,also->getUID(),also->x,also->y);
					}
					numGenDecorations++;
				}
			}
			else
			{
				// return to normal generation
				if ( map_rng.rand() % 2 || nodecoration )
				{
					// balance for total number of players
					int balance = 0;
					for ( i = 0; i < MAXPLAYERS; i++ )
					{
						if ( !client_disconnected[i] )
						{
							balance++;
						}
					}
					switch ( balance )
					{
						case 1:
							balance = 4;
							break;
						case 2:
							balance = 3;
							break;
						case 3:
							balance = 2;
							break;
						case 4:
							balance = 2;
							break;
						default:
							balance = 2;
							break;
					}

					// monsters/items
					if ( balance )
					{
						if ( map_rng.rand() % balance )
						{
							if ( map.lootexcludelocations[x + y * map.width] == false )
							{
								if ( map_rng.rand() % 10 == 0 )   // 10% chance
								{
									entity = newEntity(9, 1, map.entities, nullptr);  // gold
									entity->goldAmount = 0;
									numGenGold++;
								}
								else
								{
									entity = newEntity(8, 1, map.entities, nullptr);  // item
									itemsGeneratedList.push_back(entity->getUID());
									setSpriteAttributes(entity, nullptr, nullptr);
									numGenItems++;
								}
							}
						}
						else
						{
							if ( map.monsterexcludelocations[x + y * map.width] == false )
							{
								bool doNPC = false;
								if ( gameplayCustomManager.processedPropertyForFloor(currentlevel, secretlevel, map.name, GameplayCustomManager::PROPERTY_NPC, doNPC) )
								{
									// doNPC processed by function
								}
								else if ( map_rng.rand() % 10 == 0 && currentlevel > 1 )
								{
									doNPC = true;
								}

								if ( doNPC )
								{
									if ( currentlevel > 15 && map_rng.rand() % 4 > 0 )
									{
										entity = newEntity(93, 1, map.entities, map.creatures);  // automaton
										if ( currentlevel < 25 )
										{
											entity->monsterStoreType = 1; // weaker version
										}
									}
									else
									{
										entity = newEntity(27, 1, map.entities, map.creatures);  // human
										if ( multiplayer != CLIENT && currentlevel > 5 )
										{
											entity->monsterStoreType = (currentlevel / 5) * 3 + (map_server_rng.rand() % 4); // scale humans with depth. 3 LVL each 5 floors, + 0-3.
										}
									}
								}
								else
								{
									entity = newEntity(10, 1, map.entities, map.creatures);  // monster
								}
								entity->skill[5] = nummonsters;
								nummonsters++;
							}
						}
					}
				}
				else
				{
					// decorations
					if ( (map_rng.rand() % 4 == 0 || (currentlevel <= 10 && !customTrapsForMapInUse)) && strcmp(map.name, "Hell") )
					{
						switch ( map_rng.rand() % 7 )
						{
							case 0:
								entity = newEntity(12, 1, map.entities, nullptr); //Firecamp entity.
								break; //Firecamp
							case 1:
								entity = newEntity(14, 1, map.entities, nullptr); //Fountain entity.
								break; //Fountain
							case 2:
								entity = newEntity(15, 1, map.entities, nullptr); //Sink entity.
								break; //Sink
							case 3:
								entity = newEntity(21, 1, map.entities, nullptr); //Chest entity.
								setSpriteAttributes(entity, nullptr, nullptr);
								entity->chestLocked = -1;
								break; //Chest
							case 4:
								entity = newEntity(39, 1, map.entities, nullptr); //Tomb entity.
								break; //Tomb
							case 5:
								entity = newEntity(59, 1, map.entities, nullptr); //Table entity.
								setSpriteAttributes(entity, nullptr, nullptr);
								break; //Table
							case 6:
								entity = newEntity(60, 1, map.entities, nullptr); //Chair entity.
								setSpriteAttributes(entity, nullptr, nullptr);
								break; //Chair
						}
					}
					else
					{
						if ( customTrapsForMapInUse )
						{
							if ( !customTraps.spikes && !customTraps.verticalSpelltraps )
							{
								continue;
							}
							else if ( customTraps.verticalSpelltraps && map_rng.rand() % 2 == 0 )
							{
								entity = newEntity(120, 1, map.entities, nullptr); // vertical spell trap.
								setSpriteAttributes(entity, nullptr, nullptr);
							}
							else if ( customTraps.spikes )
							{
								entity = newEntity(64, 1, map.entities, nullptr); // spear trap
							}
						}
						else
						{
							if ( currentlevel <= 25 )
							{
								entity = newEntity(64, 1, map.entities, nullptr); // spear trap
							}
							else
							{
								if ( map_rng.rand() % 2 == 0 )
								{
									entity = newEntity(120, 1, map.entities, nullptr); // vertical spell trap.
									setSpriteAttributes(entity, nullptr, nullptr);
								}
								else
								{
									entity = newEntity(64, 1, map.entities, nullptr); // spear trap
								}
							}
						}
						Entity* also = newEntity(33, 1, map.entities, nullptr);
						also->x = x * 16;
						also->y = y * 16;
						//printlog("15 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",also->sprite,also->getUID(),also->x,also->y);
					}
					numGenDecorations++;
				}
			}
		}
		if ( entity != nullptr )
		{
			entity->x = x * 16;
			entity->y = y * 16;
			//printlog("9 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",entity->sprite,entity->getUID(),entity->x,entity->y);
		}
		// mark this location as inelligible for reselection
		if ( !skipPossibleLocationsDecrement )
		{
			possiblelocations[y + x * map.height] = false;
			numpossiblelocations--;
		}
	}

	if ( svFlags & SV_FLAG_TRAPS )
	{
		int numSlimebushes = 0;
		if ( currentlevel > 5 && currentlevel < 10 )
		{
			numSlimebushes += 2 + map_rng.rand() % 3;
		}
		else if ( currentlevel >= 10 )
		{
			numSlimebushes += 4 + map_rng.rand() % 3;
		}

		std::set<int> visited;
		while ( numSlimebushes > 0 && (waterEmptyTiles.size() > 0 || lavaEmptyTiles.size() > 0) )
		{
			size_t totalSize = waterEmptyTiles.size() + lavaEmptyTiles.size();
			int pick = map_rng.rand() % totalSize;

			int x = 0;
			int y = 0;
			if ( pick < waterEmptyTiles.size() )
			{
				x = (waterEmptyTiles[pick]) % 1000;
				y = (waterEmptyTiles[pick]) / 1000;
				waterEmptyTiles.erase(waterEmptyTiles.begin() + pick);
			}
			else if ( pick >= waterEmptyTiles.size() && ((pick - waterEmptyTiles.size()) < lavaEmptyTiles.size()) )
			{
				x = (lavaEmptyTiles[pick - waterEmptyTiles.size()]) % 1000;
				y = (lavaEmptyTiles[pick - waterEmptyTiles.size()]) / 1000;
				lavaEmptyTiles.erase(lavaEmptyTiles.begin() + (pick - waterEmptyTiles.size()));
			}

			bool skip = false;
			for ( auto coord : visited )
			{
				int tx = coord % 1000;
				int ty = coord / 1000;

				real_t dx, dy;
				dx = tx - x;
				dy = ty - y;
				if ( sqrt(dx * dx + dy * dy) < 4.0 ) // too close to other regions, within X tiles
				{
					skip = true;
					break;
				}
			}

			visited.insert(x + y * 1000);

			if ( skip )
			{
				continue;
			}

			if ( x != 0 && y != 0 )
			{
				Entity* summonTrap = newEntity(97, 1, map.entities, nullptr);
				summonTrap->x = x * 16.0;
				summonTrap->y = y * 16.0;
				setSpriteAttributes(summonTrap, nullptr, nullptr);
				summonTrap->skill[9] = 3; // x tile auto activate
				summonTrap->skill[0] = SLIME;

				//Entity* ent = newEntity(245, 0, map.entities, nullptr);
				////ent->behavior = &actBoulder;
				//ent->x = x * 16.0 + 8;
				//ent->y = y * 16.0 + 8;
				//ent->z = 24.0;
				//ent->flags[PASSABLE] = true;
			}

			--numSlimebushes;
		}
	}

	{
		if ( !strcmp(map.name, "The Ruins") || !strcmp(map.name, "Citadel") )
		{
			int numBells = 0;
			if ( currentlevel % 2 == 0 )
			{
				numBells = 1 + map_rng.rand() % 2;
			}
			else
			{
				numBells = 2 + map_rng.rand() % 2;
			}
			std::vector<int> goodSpots;
			for ( int x = 0; x < map.width; ++x )
			{
				for ( int y = 0; y < map.height; ++y )
				{
					if ( possiblelocations[y + x * map.height] == true )
					{
						goodSpots.push_back(x + 10000 * y);
					}
				}
			}

			for ( int c = 0; c < std::min(numBells, (int)goodSpots.size()); ++c )
			{
				// choose a random location from those available
				int pick = map_rng.rand() % goodSpots.size();
				int x = goodSpots[pick] % 10000;
				int y = goodSpots[pick] / 10000;

				goodSpots.erase(goodSpots.begin() + pick);

				bool bellSpot = true;
				for ( int x2 = -1; x2 <= 1; x2++ )
				{
					for ( int y2 = -1; y2 <= 1; y2++ )
					{
						int checkx = x + x2;
						int checky = y + y2;
						if ( checkx >= 0 && checkx < map.width )
						{
							if ( checky >= 0 && checky < map.height )
							{
								if ( !possiblelocations[checky + checkx * map.height] )
								{
									bellSpot = false;
								}
								else if ( map.tiles[(MAPLAYERS - 1) + checky * MAPLAYERS + checkx * MAPLAYERS * map.height]
									|| map.tiles[OBSTACLELAYER + checky * MAPLAYERS + checkx * MAPLAYERS * map.height] )
								{
									bellSpot = false;
								}
								else if ( checkObstacle((checkx) * 16, (checky) * 16, NULL, NULL, false, false) )
								{
									bellSpot = false;
								}
							}
						}
					}
				}
				if ( bellSpot )
				{
					Entity* bell = newEntity(191, 1, map.entities, nullptr); //Bell entity.
					bell->x = x * 16.0;
					bell->y = y * 16.0;

					possiblelocations[y + x * map.height] = false;
					numpossiblelocations--;
				}
				else
				{
					--c;
					continue;
				}
			}
		}
	}

	int numBreakables = std::min(15, numpossiblelocations / 10);
	struct BreakableNode_t
	{
		BreakableNode_t(int _walls, int _x, int _y, int _dir, int _id = -1)
		{
			walls = _walls;
			x = _x;
			y = _y;
			dir = _dir;
			id = _id;
		};
		int walls;
		int x;
		int y;
		int dir;
		int id = -1;
	};
	auto findBreakables = EditorEntityData_t::colliderRandomGenPool.find(map.name);
	if ( findBreakables == EditorEntityData_t::colliderRandomGenPool.end() )
	{
		numBreakables = 0;
	}
	int numOpenAreaBreakables = 0;
	std::vector<BreakableNode_t> breakableLocations;
	if ( findBreakables->first == "Underworld" )
	{
		numOpenAreaBreakables = 10;

		std::vector<int> picked;
		while ( numOpenAreaBreakables > 0 && underworldEmptyTiles.size() > 0 )
		{
			int pick = map_rng.rand() % underworldEmptyTiles.size();
			picked.push_back(underworldEmptyTiles[pick]);

			underworldEmptyTiles.erase(underworldEmptyTiles.begin() + pick);
		}

		for ( auto& coord : picked )
		{
			int x = (coord) % 1000;
			int y = (coord) / 1000;

			if ( numOpenAreaBreakables > 0 )
			{
				int obstacles = 0;
				// add some hanging cages
				for ( int x2 = -1; x2 <= 1; x2++ )
				{
					for ( int y2 = -1; y2 <= 1; y2++ )
					{
						if ( x2 == 0 && y2 == 0 ) { continue; }
						int checkx = x + x2;
						int checky = y + y2;
						if ( checkx >= 0 && checkx < map.width )
						{
							if ( checky >= 0 && checky < map.height )
							{
								int index = (checky)*MAPLAYERS + (checkx)*MAPLAYERS * map.height;
								if ( map.tiles[index] )
								{
									++obstacles;
									break;
								}
								if ( checkObstacle((checkx) * 16, (checky) * 16, NULL, NULL, false, true, false) )
								{
									++obstacles;
									break;
								}
							}
						}
					}
				}

				if ( obstacles == 0 )
				{
					breakableLocations.push_back(BreakableNode_t(1, x, y, map_rng.rand() % 4, 
						map_rng.rand() % 2 ? 14 : 40)); // random dir, hanging cage ids
					--numOpenAreaBreakables;

					if ( possiblelocations[y + x * map.height] )
					{
						possiblelocations[y + x * map.height] = false;
						--numpossiblelocations;
					}
				}
			}
		}
	}

	for ( c = 0; c < std::min(numBreakables, numpossiblelocations); ++c )
	{
		// choose a random location from those available
		pickedlocation = map_rng.rand() % numpossiblelocations;
		i = -1;
		int x = 0;
		int y = 0;
		bool skipPossibleLocationsDecrement = false;
		while ( 1 )
		{
			if ( possiblelocations[y + x * map.height] == true )
			{
				++i;
				if ( i == pickedlocation )
				{
					break;
				}
			}
			++x;
			if ( x >= map.width )
			{
				x = 0;
				++y;
				if ( y >= map.height )
				{
					y = 0;
				}
			}
		}

		std::set<int> walls;
		std::set<int> corners;
		for ( int x2 = -1; x2 <= 1; x2++ )
		{
			for ( int y2 = -1; y2 <= 1; y2++ )
			{
				if ( x2 == 0 && y2 == 0 )
				{
					continue;
				}

				int checkx = x + x2;
				int checky = y + y2;
				if ( checkx >= 0 && checkx < map.width )
				{
					if ( checky >= 0 && checky < map.height )
					{
						int index = (checky) * MAPLAYERS + (checkx) * MAPLAYERS * map.height;
						if ( map.tiles[OBSTACLELAYER + index] )
						{
							if ( (x2 == -1 && y2 == -1) || (x2 == 1 && y2 == 1)
								|| (x2 == -1 && y2 == 1) || (x2 == 1 && y2 == -1) )
							{
								corners.insert(checkx + checky * 1000);
							}
							else
							{
								walls.insert(checkx + checky * 1000);
							}
						}
					}
				}
			}
		}

		possiblelocations[y + x * map.height] = false;
		numpossiblelocations--;

		//if ( walls.size() == 0 && findBreakables->first == "Underworld" && numOpenAreaBreakables > 0 )
		//{
		//	int obstacles = 0;
		//	// add some hanging cages
		//	for ( int x2 = -1; x2 <= 1; x2++ )
		//	{
		//		for ( int y2 = -1; y2 <= 1; y2++ )
		//		{
		//			if ( x2 == 0 && y2 == 0 ) { continue; }
		//			int checkx = x + x2;
		//			int checky = y + y2;
		//			if ( checkObstacle((checkx) * 16, (checky) * 16, NULL, NULL, false, true, false) )
		//			{
		//				++obstacles;
		//				break;
		//			}
		//		}
		//	}

		//	if ( obstacles == 0 )
		//	{
		//		breakableLocations.push(BreakableNode_t(1, x, y, map_rng.rand() % 4, 
		//			map_rng.rand() % 2 ? 14 : 40)); // random dir, low prio, hanging cage ids
		//		--numOpenAreaBreakables;
		//	}
		//	--c;
		//	continue;
		//}

		if ( walls.size() == 0 || walls.size() >= 4 )
		{
			// try again
			--c;
			continue;
		}

		std::set<int> freespaces;
		for ( int x2 = -1; x2 <= 1; x2++ )
		{
			for ( int y2 = -1; y2 <= 1; y2++ )
			{
				if ( x2 == 0 && y2 == 0 ) { continue; }
				int checkx = x + x2;
				int checky = y + y2;
				if ( walls.find(checkx + checky * 1000) != walls.end() 
					|| corners.find(checkx + checky * 1000) != corners.end() )
				{
					continue;
				}
				if ( checkx >= 0 && checkx < map.width )
				{
					if ( checky >= 0 && checky < map.height )
					{
						int index = (checky) * MAPLAYERS + (checkx) * MAPLAYERS * map.height;
						if ( swimmingtiles[map.tiles[index]] || lavatiles[map.tiles[index]] )
						{
							continue;
						}
						if ( !checkObstacle((checkx) * 16, (checky) * 16, NULL, NULL, false, false) )
						{
							freespaces.insert(checkx + checky * 1000);
						}
					}
				}
			}
		}

		bool foundSpace = false;
		if ( (walls.size() == 1 && freespaces.size() >= 5)
			|| (walls.size() == 2 && freespaces.size() >= 3)
			|| (walls.size() == 3 && freespaces.size() >= 1) )
		{
			int numIslands = 0;
			std::set<int> reachedTiles;
			std::map<int, std::set<int>> islands;
			for ( auto it = freespaces.begin(); it != freespaces.end(); ++it )
			{
				if ( reachedTiles.find(*it) == reachedTiles.end() )
				{
					// new island
					std::queue<int> frontier;
					frontier.push(*it);
					reachedTiles.insert(*it);
					while ( !frontier.empty() )
					{
						auto currentKey = frontier.front();
						frontier.pop();

						const int ix = (currentKey) % 1000;
						const int iy = (currentKey) / 1000;

						islands[numIslands].insert(currentKey);

						int checkKey = (ix + 1) + ((iy) * 1000);
						if ( freespaces.find(checkKey) != freespaces.end()
							&& reachedTiles.find(checkKey) == reachedTiles.end() )
						{
							frontier.push(checkKey);
							reachedTiles.insert(checkKey);
						}
						checkKey = (ix - 1) + ((iy) * 1000);
						if ( freespaces.find(checkKey) != freespaces.end()
							&& reachedTiles.find(checkKey) == reachedTiles.end() )
						{
							frontier.push(checkKey);
							reachedTiles.insert(checkKey);
						}
						checkKey = (ix) + ((iy + 1) * 1000);
						if ( freespaces.find(checkKey) != freespaces.end()
							&& reachedTiles.find(checkKey) == reachedTiles.end() )
						{
							frontier.push(checkKey);
							reachedTiles.insert(checkKey);
						}
						checkKey = (ix) + ((iy - 1) * 1000);
						if ( freespaces.find(checkKey) != freespaces.end()
							&& reachedTiles.find(checkKey) == reachedTiles.end() )
						{
							frontier.push(checkKey);
							reachedTiles.insert(checkKey);
						}
					}
					if ( !islands[numIslands].empty() )
					{
						++numIslands;
					}
				}
			}

			for ( auto& island : islands )
			{
				if ( (walls.size() == 1 && island.second.size() >= 5)
					|| (walls.size() == 2 && island.second.size() >= 3)
					|| (walls.size() == 3 && island.second.size() >= 1) )
				{
					std::vector<unsigned int> dirs;
					if ( walls.size() == 3 )
					{
						if ( walls.find((x + 1) + (y + 0) * 1000) == walls.end() )
						{
							dirs.push_back(0);
						}
						else if ( walls.find((x - 1) + (y + 0) * 1000) == walls.end() )
						{
							dirs.push_back(4);
						}
						else if ( walls.find((x + 0) + (y + 1) * 1000) == walls.end() )
						{
							dirs.push_back(2);
						}
						else if ( walls.find((x + 0) + (y - 1) * 1000) == walls.end() )
						{
							dirs.push_back(6);
						}
					}
					else
					{
						if ( walls.find((x + 1) + (y + 0) * 1000) != walls.end() )
						{
							dirs.push_back(4);
						}
						else if ( walls.find((x - 1) + (y + 0) * 1000) != walls.end() )
						{
							dirs.push_back(0);
						}
						else if ( walls.find((x + 0) + (y + 1) * 1000) != walls.end() )
						{
							dirs.push_back(6);
						}
						else if ( walls.find((x + 0) + (y - 1) * 1000) != walls.end() )
						{
							dirs.push_back(2);
						}
					}
					int picked = dirs[map_rng.rand() % dirs.size()];
					breakableLocations.push_back(BreakableNode_t(walls.size(), x, y, picked));
					foundSpace = true;
					break;
				}
			}
		}

		if ( !foundSpace )
		{
			--c;
		}
	}

	int breakableGoodies = breakableLocations.size() * 80 / 100;
	int breakableMonsters = 0;
	int breakableMonsterLimit = 2 + (currentlevel / LENGTH_OF_LEVEL_REGION) * (1 + map_rng.rand() % 2);
	static ConsoleVariable<int> cvar_breakableMonsterLimit("/breakable_monster_limit", 0);
	if ( svFlags & SV_FLAG_CHEATS )
	{
		breakableMonsterLimit = std::max(*cvar_breakableMonsterLimit, breakableMonsterLimit);
	}
	if ( findBreakables != EditorEntityData_t::colliderRandomGenPool.end() && findBreakables->second.size() > 0 && breakableGoodies > 0 )
	{
		int breakableItemsFromGround = 0;
		std::vector<unsigned int> chances;
		std::vector<unsigned int> ids;
		for ( auto& pair : findBreakables->second )
		{
			ids.push_back(pair.first);
			chances.push_back(pair.second);
		}
		Monster lastMonsterEvent = NOTHING;
		while ( !breakableLocations.empty() )
		{
			int maxNumWalls = 0;
			for ( auto& b : breakableLocations )
			{
				maxNumWalls = std::max(b.walls, maxNumWalls);
			}
			std::vector<unsigned int> posChances;
			int pickedPos = 0;
			for ( auto& b : breakableLocations )
			{
				posChances.push_back(b.walls == maxNumWalls ? 1 : 0);
			}

			pickedPos = map_rng.discrete(posChances.data(), posChances.size());

			auto& top = breakableLocations.at(pickedPos);
			int x = top.x;
			int y = top.y;

			Entity* breakable = newEntity(179, 1, map.entities, nullptr);
			breakable->x = x * 16.0;
			breakable->y = y * 16.0;
			breakable->colliderDecorationRotation = top.dir;

			if ( top.id >= 0 )
			{
				breakable->colliderDamageTypes = top.id;
			}
			else
			{
				int picked = map_rng.discrete(chances.data(), chances.size());
				breakable->colliderDamageTypes = ids[picked];
			}

			bool monsterEventExists = false;
			auto findData = EditorEntityData_t::colliderData.find(breakable->colliderDamageTypes);
			if ( findData != EditorEntityData_t::colliderData.end() )
			{
				auto findMap = findData->second.hideMonsters.find(map.name);
				if ( findMap != findData->second.hideMonsters.end() )
				{
					if ( findMap->second.size() > 0 )
					{
						for ( auto m : findMap->second )
						{
							if ( m > NOTHING && m < NUMMONSTERS )
							{
								monsterEventExists = true;
							}
						}
					}
				}
			}

			if ( breakableGoodies > 0 )
			{
				--breakableGoodies;

				int index = (y) * MAPLAYERS + (x) * MAPLAYERS * map.height;

				static ConsoleVariable<int> cvar_breakableMonsterChance("/breakable_monster_chance", 10);

				if ( !map.tiles[index] && map_rng.rand() % 2 == 1 )
				{
					// nothing over pits 50%
				}
				else if ( (breakableMonsters < breakableMonsterLimit && monsterEventExists 
					&& map_rng.rand() % ((svFlags & SV_FLAG_CHEATS) ? std::min(10, *cvar_breakableMonsterChance) : 10) == 0)
					&& map.monsterexcludelocations[x + y * map.width] == false ) // 10% monster inside
				{
					Monster monsterEvent = NOTHING;
					auto findMap = findData->second.hideMonsters.find(map.name);
					if ( findMap != findData->second.hideMonsters.end() )
					{
						if ( findMap->second.size() > 0 )
						{
							std::vector<unsigned int> chances;
							bool avoidLastMonster = false;
							for ( auto m : findMap->second )
							{
								chances.push_back(1);
								if ( lastMonsterEvent != NOTHING && m != lastMonsterEvent )
								{
									avoidLastMonster = true;
								}
							}
							if ( avoidLastMonster )
							{
								for ( size_t i = 0; i < chances.size(); ++i )
								{
									if ( findMap->second[i] == lastMonsterEvent )
									{
										chances[i] = 0;
									}
								}
							}
							int pickIndex = map_rng.discrete(chances.data(), chances.size());
							int picked = findMap->second[pickIndex];
							if ( picked > NOTHING && picked < NUMMONSTERS )
							{
								monsterEvent = (Monster)picked;
								lastMonsterEvent = monsterEvent;
							}
						}
					}

					if ( (svFlags & SV_FLAG_TRAPS) )
					{
						if ( map_rng.rand() % 2 == 0 )
						{
							breakable->colliderHideMonster = monsterEvent;
						}
						else
						{
							breakable->colliderHideMonster = 1000 + monsterEvent;
						}
					}
					++breakableMonsters;
				}
				else if ( !map.tiles[index] || map_rng.rand() % 2 == 1 )   // 50% chance (or floating over a pit is just gold)
				{
					std::vector<Entity*> genGold;
					int numGold = 3 + map_rng.rand() % 3;
					while ( numGold > 0 )
					{
						--numGold;
						Entity* entity = newEntity(9, 1, map.entities, nullptr);  // gold
						genGold.push_back(entity);
						entity->x = breakable->x;
						entity->y = breakable->y;
						entity->goldAmount = 2 + map_rng.rand() % 3;
						entity->flags[INVISIBLE] = true;
						entity->yaw = breakable->yaw;
						entity->goldInContainer = breakable->getUID();
						breakable->colliderContainedEntity = entity->getUID();
						numGenGold++;
					}
					int index = -1;
					for ( auto gold : genGold )
					{
						++index;
						gold->yaw += (index * PI) / genGold.size();
					}
				}
				else
				{
					if ( itemsGeneratedList.size() > 10 && breakableItemsFromGround < 6 )
					{
						// steal an item from the ground
						size_t index = map_rng.rand() % itemsGeneratedList.size();
						Uint32 uid = itemsGeneratedList.at(index);
						itemsGeneratedList.erase(itemsGeneratedList.begin() + index);
						if ( Entity* entity = uidToEntity(uid) )
						{
							entity->x = breakable->x;
							entity->y = breakable->y;
							entity->flags[INVISIBLE] = true;
							entity->itemContainer = breakable->getUID();
							entity->yaw = breakable->yaw;
							breakable->colliderContainedEntity = entity->getUID();
							++breakableItemsFromGround;
						}
					}
					else
					{
						Entity* entity = newEntity(8, 1, map.entities, nullptr);  // item
						setSpriteAttributes(entity, nullptr, nullptr);
						entity->x = breakable->x;
						entity->y = breakable->y;
						entity->flags[INVISIBLE] = true;
						entity->itemContainer = breakable->getUID();
						entity->yaw = breakable->yaw;
						breakable->colliderContainedEntity = entity->getUID();
						numGenItems++;
					}
				}
			}

			if ( false )
			{
				//messagePlayer(0, MESSAGE_DEBUG, "pick: %d | x: %d y: %d", picked, x, y);
				Entity* ent = newEntity(245, 0, map.entities, nullptr);
				//ent->behavior = &actBoulder;
				ent->x = x * 16.0 + 8;
				ent->y = y * 16.0 + 8;
				ent->z = 24.0;
				ent->flags[PASSABLE] = true;
			}
			breakableLocations.erase(breakableLocations.begin() + pickedPos);
		}
	}

	if ( darkmap && map.skybox == 0 )
	{
		std::vector<std::map<int, std::vector<int>>> batAreasGood;
		std::vector<std::map<int, std::vector<int>>> batAreasOk;
		for ( int x = 1; x < map.width - 1; ++x )
		{
			for ( int y = 1; y < map.height - 1; ++y )
			{
				if ( possiblelocations[y + x * map.height] )
				{
					std::vector<int> testAreas = {
						(x - 1) + 1000 * (y + 0),
						(x + 1) + 1000 * (y + 0),
						(x + 0) + 1000 * (y + 1),
						(x + 0) + 1000 * (y - 1),
						(x + 0) + 1000 * (y + 0),
						(x + 1) + 1000 * (y + 1),
						(x - 1) + 1000 * (y + 1),
						(x + 1) + 1000 * (y - 1),
						(x - 1) + 1000 * (y - 1)
					};
					std::map<int, std::vector<int>> goodSpots;
					int openCeilings = 0;
					for ( auto coord : testAreas )
					{
						int tx = coord % 1000;
						int ty = coord / 1000;
						if ( tx >= 1 && tx < map.width - 1 && ty >= 1 && ty < map.height - 1 )
						{
							if ( possiblelocations[ty + tx * map.height] )
							{
								int mapIndex = (ty)*MAPLAYERS + (tx)*MAPLAYERS * map.height;
								if ( !map.tiles[OBSTACLELAYER + mapIndex] )
								{
									if ( !map.tiles[(MAPLAYERS - 1) + mapIndex] )
									{
										++openCeilings;
										goodSpots[0].push_back(coord);
									}
									else
									{
										goodSpots[1].push_back(coord);
									}
								}
							}
						}
					}
					if ( openCeilings >= 5 )
					{
						batAreasGood.push_back(goodSpots);
					}
					else if ( (goodSpots[0].size() + goodSpots[1].size()) >= 5 )
					{
						batAreasOk.push_back(goodSpots);
					}
				}
			}
		}

		std::unordered_set<int> visited;
		std::vector<int> previousAreas;
		int numBatAreas = std::max(2, std::min(5, 1 + (currentlevel / LENGTH_OF_LEVEL_REGION)));
		while ( numBatAreas > 0 )
		{
			if ( batAreasGood.size() == 0 && batAreasOk.size() == 0 )
			{
				break;
			}

			auto& areas = batAreasGood.size() > 0 ? batAreasGood : batAreasOk;
			if ( areas.size() > 0 )
			{
				size_t picked = map_rng.rand() % areas.size();
				auto& coords = areas[picked];

				bool skip = false;
				for ( auto coord : coords[0] )
				{
					if ( visited.find(coord) != visited.end() )
					{
						// no good
						skip = true;
					}
					else
					{
						visited.insert(coord);
					}
				}
				for ( auto coord : coords[1] )
				{
					if ( visited.find(coord) != visited.end() )
					{
						// no good
						skip = true;
					}
					else
					{
						visited.insert(coord);
					}
				}

				int currentCoord = 0;
				if ( coords[0].size() > 0 )
				{
					currentCoord = coords[0][0];
				}
				else if ( coords[1].size() > 0 )
				{
					currentCoord = coords[1][0];
				}

				int checkx = currentCoord % 1000;
				int checky = currentCoord / 1000;
				for ( auto previousCoord : previousAreas )
				{
					int ox = previousCoord % 1000;
					int oy = previousCoord / 1000;

					real_t dx, dy;
					dx = checkx - ox;
					dy = checky - oy;
					if ( sqrt(dx * dx + dy * dy) < 8.0 ) // too close to other regions, within 8 tiles
					{
						skip = true;
						break;
					}
				}

				if ( skip )
				{
					areas.erase(areas.begin() + picked);
					continue;
				}

				if ( coords[0].size() > 0 )
				{
					previousAreas.push_back(coords[0][0]);
				}
				else if ( coords[1].size() > 0 )
				{
					previousAreas.push_back(coords[1][0]);
				}

				int numSpawns = std::max(2, std::min(4, 1 + (currentlevel / LENGTH_OF_LEVEL_REGION)));
				for ( size_t i = 0; i < (coords[0].size() + coords[1].size()) && numSpawns > 0; ++i )
				{
					auto coord = (i < coords[0].size()) ? coords[0][i] : coords[1][i - coords[0].size()];
					int tx = coord % 1000;
					int ty = coord / 1000;

					{
						Entity* ent = newEntity(188, 0, map.entities, nullptr);
						ent->x = tx * 16.0;
						ent->y = ty * 16.0;
					}

					//Entity* ent = newEntity(245, 0, map.entities, nullptr);
					////ent->behavior = &actBoulder;
					//ent->x = tx * 16.0 + 8;
					//ent->y = ty * 16.0 + 8;
					//ent->z = 24.0;
					//ent->flags[PASSABLE] = true;
					visited.insert(coord);
					--numSpawns;

					possiblelocations[ty + tx * map.height] = false;
					--numpossiblelocations;
				}

				--numBatAreas;

				areas.erase(areas.begin() + picked);
				continue;
			}
		}
	}

	// on hell levels, lava doesn't bubble. helps performance
	/*if( !strcmp(map.name,"Hell") ) {
		for( node=map.entities->first; node!=NULL; node=node->next ) {
			Entity *entity = (Entity *)node->element;
			if( entity->sprite == 41 ) { // lava.png
				entity->skill[4] = 1; // LIQUID_LAVANOBUBBLE =
			}
		}
	}*/

	free(possiblelocations);
	free(firstroomtile);
	free(subRoomName);
	free(sublevelname);
	list_FreeAll(&subRoomMapList);
	list_FreeAll(&mapList);
	list_FreeAll(&doorList);

	printlog("successfully generated a dungeon with %d rooms, %d monsters, %d gold, %d items, %d decorations.\n", roomcount, nummonsters, numGenGold, numGenItems, numGenDecorations);
	//messagePlayer(0, "successfully generated a dungeon with %d rooms, %d monsters, %d gold, %d items, %d decorations.", roomcount, nummonsters, numGenGold, numGenItems, numGenDecorations);
	return secretlevelexit;
}

bool allowedGenerateMimicOnChest(int x, int y, map_t& map)
{
	if ( gameModeManager.getMode() == gameModeManager.GAME_MODE_TUTORIAL
		|| gameModeManager.getMode() == gameModeManager.GAME_MODE_TUTORIAL_INIT )
	{
		return false;
	}
	if ( !(svFlags & SV_FLAG_TRAPS) )
	{
		return false;
	}
	/*if ( map.trapexcludelocations )
	{
		if ( x >= 0 && x < map.width && y >= 0 && y < map.height )
		{
			if ( map.trapexcludelocations[x + y * map.width] )
			{
				return false;
			}
		}
	}*/
	return true;
}

void debugMap(map_t* map)
{
	return;
	if ( !map )
	{
		return;
	}

	std::set<Uint32> takenSlots;
	for ( auto node = map->entities->first; node != nullptr; )
	{
		Entity* postProcessEntity = (Entity*)node->element;
		node = node->next;
		if ( postProcessEntity )
		{
			if ( postProcessEntity->behavior == &actItem && postProcessEntity->z > 4 )
			{
				int x = (int)postProcessEntity->x >> 4;
				int y = (int)postProcessEntity->y >> 4;
				takenSlots.insert(x + y * 10000);
			}
		}
	}

	for ( int x = 0; x < map->width; ++x )
	{
		for ( int y = 0; y < map->height; ++y )
		{
			if ( takenSlots.find(x + y * 10000) != takenSlots.end() )
			{
				int numWalls = 0;
				std::vector<std::pair<int, int>> coords = {
					{x + 1, y},
					{x - 1, y},
					{x, y + 1},
					{x, y - 1}
				};
				for ( auto& pair : coords )
				{
					if ( pair.first >= 0 && pair.first < map->width )
					{
						if ( pair.second >= 0 && pair.second < map->height )
						{
							if ( map->tiles[pair.second * MAPLAYERS + pair.first * MAPLAYERS * map->height] ) // floor
							{
								numWalls += map->tiles[OBSTACLELAYER + pair.second * MAPLAYERS + pair.first * MAPLAYERS * map->height] != 0 ? 1 : 0;
							}
						}
					}
				}
				if ( numWalls > 0 )
				{
					//Entity* ent = newEntity(245, 0, map->entities, nullptr);
					////ent->behavior = &actBoulder;
					//ent->x = x * 16.0 + 8;
					//ent->y = y * 16.0 + 8;
					//ent->z = 24.0;
				}
				//int numObstacles = checkObstacle((checkx) * 16, (checky) * 16, NULL, NULL, true);
			}
		}
	}
	mapLevel2(0);

	/*int num5x5s = 0;
	// open area debugging tool
	for ( int x = 0; x < map->width; ++x )
	{
		for ( int y = 0; y < map->height; ++y )
		{
			if ( takenSlots.find(x + y * 10000) == takenSlots.end() )
			{
				if ( !map->tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map->height]
					&& !map->tiles[2 + y * MAPLAYERS + x * MAPLAYERS * map->height] )
				{
					int numTiles = 0;
					for ( int x1 = x; x1 < map->width && x1 < x + 5; ++x1 )
					{
						for ( int y1 = y; y1 < map->height && y1 < y + 5; ++y1 )
						{
							if ( takenSlots.find(x1 + y1 * 10000) == takenSlots.end() )
							{
								if ( !map->tiles[OBSTACLELAYER + y1 * MAPLAYERS + x1 * MAPLAYERS * map->height]
									&& !map->tiles[2 + y1 * MAPLAYERS + x1 * MAPLAYERS * map->height] )
								{
									++numTiles;
								}
							}
						}
					}
					if ( numTiles == 25 )
					{
						++num5x5s;
						Entity* ent = newEntity(245, 0, map->entities, nullptr);
						ent->behavior == &actBoulder;
						ent->x = x * 16.0 + 8;
						ent->y = y * 16.0 + 8;
					}
				}
			}
		}
	}
	messagePlayer(0, MESSAGE_DEBUG, "%d 5x5s", num5x5s);*/
}

/*-------------------------------------------------------------------------------

	assignActions

	configures a map to be playable from a default state

-------------------------------------------------------------------------------*/

void assignActions(map_t* map)
{
	bool itemsdonebefore = false;
	Entity* vampireQuestChest = nullptr;

	if ( map == nullptr )
	{
		return;
	}

	// update arachnophobia filter
	arachnophobia_filter = GameplayPreferences_t::getGameConfigValue(GameplayPreferences_t::GOPT_ARACHNOPHOBIA);
	colorblind_lobby = GameplayPreferences_t::getGameConfigValue(GameplayPreferences_t::GOPT_COLORBLIND);

	// add lava lights
	for ( int y = 0; y < map->height; ++y )
	{
		for ( int x = 0; x < map->width; ++x )
		{
			if ( lavatiles[map->tiles[y * MAPLAYERS + x * MAPLAYERS * map->height]] )
			{
				addLight(x, y, "lava");
			}
		}
	}

	// seed the random generator

	map_rng.seedBytes(&mapseed, sizeof(mapseed));
	map_server_rng.seedBytes(&mapseed, sizeof(mapseed));

	int balance = 0;
	for ( int i = 0; i < MAXPLAYERS; i++ )
	{
		if ( !client_disconnected[i] )
		{
			balance++;
		}
	}

	bool customMonsterCurveExists = false;
	monsterCurveCustomManager.followersToGenerateForLeaders.clear();
	if ( !monsterCurveCustomManager.inUse() )
	{
		monsterCurveCustomManager.readFromFile(mapseed);
	}
	if ( monsterCurveCustomManager.curveExistsForCurrentMapName(map->name) )
	{
		customMonsterCurveExists = true;
		conductGameChallenges[CONDUCT_MODDED] = 1;
		Mods::disableSteamAchievements = true;
	}
	if ( gameplayCustomManager.inUse() )
	{
		conductGameChallenges[CONDUCT_MODDED] = 1;
		Mods::disableSteamAchievements = true;
	}

	// assign entity behaviors
    node_t* nextnode;
	for ( auto node = map->entities->first; node != nullptr; node = nextnode )
	{
		auto entity = (Entity*)node->element;
		nextnode = node->next;
		if ( !entity )
		{
			continue;
		}
		switch ( entity->sprite )
		{
			// null:
			case 0:
			{
				list_RemoveNode(entity->mynode);
				entity = nullptr;
				break;
				// player:
			}
			case 1:
			{
				if ( numplayers >= 0 && numplayers < MAXPLAYERS )
				{
					if ( client_disconnected[numplayers] && !intro )
					{
						// don't spawn missing players
						++numplayers;
						list_RemoveNode(entity->mynode);
						entity = nullptr;
						break;
					}
                    if ( stats[numplayers]->HP <= 0 )
                    {
                        if (!keepInventoryGlobal)
                        {
                            Item** items[] = {
                                &stats[numplayers]->helmet,
                                &stats[numplayers]->breastplate,
                                &stats[numplayers]->gloves,
                                &stats[numplayers]->shoes,
                                &stats[numplayers]->shield,
                                &stats[numplayers]->weapon,
                                &stats[numplayers]->cloak,
                                &stats[numplayers]->amulet,
                                &stats[numplayers]->ring,
                                &stats[numplayers]->mask,
                            };
                            constexpr int num_slots = sizeof(items) / sizeof(items[0]);
                            for (int c = 0; c < num_slots; ++c) {
                                if (*(items[c])) {
                                    if ((*(items[c]))->node) {
                                        list_RemoveNode((*(items[c]))->node);
                                    } else {
                                        free((*(items[c])));
                                    }
                                }
                                *(items[c]) = nullptr;
                            }
                            node_t *node, *nextnode;
                            for ( node = stats[numplayers]->inventory.first; node != nullptr; node = nextnode )
                            {
                                nextnode = node->next;
                                Item* item = (Item*)node->element;
                                if ( itemCategory(item) == SPELL_CAT )
                                {
                                    continue;    // don't drop spells on death, stupid!
                                }
                                list_RemoveNode(node);
                            }
                        }
                        if ( multiplayer != CLIENT )
                        {
                            messagePlayer(numplayers, MESSAGE_STATUS, Language::get(1109));
							stats[numplayers]->HP = stats[numplayers]->MAXHP / 2;
							stats[numplayers]->MP = stats[numplayers]->MAXMP / 2;
							stats[numplayers]->HUNGER = 500;
							for ( int c = 0; c < NUMEFFECTS; ++c )
							{
								if ( !(c == EFF_VAMPIRICAURA && stats[numplayers]->EFFECTS_TIMERS[c] == -2)
									&& c != EFF_WITHDRAWAL && c != EFF_SHAPESHIFT )
								{
									stats[numplayers]->EFFECTS[c] = false;
									stats[numplayers]->EFFECTS_TIMERS[c] = 0;
								}
							}
						}
					}
					entity->behavior = &actPlayer;
					entity->addToCreatureList(map->creatures);

					players[numplayers]->ghost.initStartRoomLocation(entity->x / 16, entity->y / 16);

					entity->x += 8;
					entity->y += 8;
					entity->z = -1;
					entity->focalx = limbs[HUMAN][0][0]; // 0
					entity->focaly = limbs[HUMAN][0][1]; // 0
					entity->focalz = limbs[HUMAN][0][2]; // -1.5
					entity->sprite = 113; // head model
					entity->sizex = 4;
					entity->sizey = 4;
					entity->flags[GENIUS] = true;
					if ( numplayers == clientnum && multiplayer == CLIENT )
					{
						entity->flags[UPDATENEEDED] = false;
					}
					else
					{
						entity->flags[UPDATENEEDED] = true;
					}
					entity->flags[BLOCKSIGHT] = true;
					entity->skill[2] = numplayers; // skill[2] == PLAYER_NUM
					players[numplayers]->entity = entity;
					if ( entity->playerStartDir == -1 )
					{
						entity->yaw = (map_rng.rand() % 8) * 45 * (PI / 180.f);
					}
					else
					{
						entity->yaw = entity->playerStartDir * 45 * (PI / 180.f);
					}
					entity->playerStartDir = 0;
					if ( multiplayer != CLIENT )
					{
						if ( numplayers == 0 && minotaurlevel )
						{
							createMinotaurTimer(entity, map, map_server_rng.getU32());
						}
					}
					++numplayers;
				}
				if ( balance > 4 )
				{
					// if MAXPLAYERS > 4, then add some new player markers
					--balance;
					Entity* extraPlayer = newEntity(1, 1, map->entities, nullptr);
					extraPlayer->x = entity->x - 8;
					extraPlayer->y = entity->y - 8;
				}
				if ( numplayers > MAXPLAYERS )
				{
					printlog("warning: too many player objects in level!\n");
				}
				break;
			}
			// east/west door:
			case 2:
			{
				entity->x += 8;
				entity->y += 8;
				entity->sprite = doorFrameSprite();
				entity->flags[PASSABLE] = true;
				entity->behavior = &actDoorFrame;
				auto childEntity = newEntity(2, 0, map->entities, nullptr); //Door frame entity.
				childEntity->x = entity->x;
				childEntity->y = entity->y;
				TileEntityList.addEntity(*childEntity);
				//printlog("16 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->sizex = 1;
				childEntity->sizey = 8;
				childEntity->behavior = &actDoor;
				childEntity->flags[BLOCKSIGHT] = true;
				childEntity->skill[0] = 0; // signify behavior code of DOOR_DIR
				childEntity->seedEntityRNG(map_server_rng.getU32());

				// copy editor options from frame to door itself.
				childEntity->doorDisableLockpicks = entity->doorDisableLockpicks;
				childEntity->doorForceLockedUnlocked = entity->doorForceLockedUnlocked;
				childEntity->doorDisableOpening = entity->doorDisableOpening;

				childEntity = newEntity(doorFrameSprite(), 0, map->entities, nullptr); //Door entity.
				childEntity->flags[INVISIBLE] = true;
				childEntity->flags[BLOCKSIGHT] = true;
				childEntity->x = entity->x;
				childEntity->y = entity->y - 7;
				TileEntityList.addEntity(*childEntity);

				//printlog("17 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->sizex = 2;
				childEntity->sizey = 2;
				childEntity->behavior = &actDoorFrame;
				childEntity = newEntity(doorFrameSprite(), 0, map->entities, nullptr); //Door frame entity.
				childEntity->flags[INVISIBLE] = true;
				childEntity->flags[BLOCKSIGHT] = true;
				childEntity->x = entity->x;
				childEntity->y = entity->y + 7;
				TileEntityList.addEntity(*childEntity);
				//printlog("18 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->sizex = 2;
				childEntity->sizey = 2;
				childEntity->behavior = &actDoorFrame;
				break;
			}
			// north/south door:
			case 3:
			{
				entity->x += 8;
				entity->y += 8;
				entity->yaw -= PI / 2.0;
				entity->sprite = doorFrameSprite();
				entity->flags[PASSABLE] = true;
				entity->behavior = &actDoorFrame;
				auto childEntity = newEntity(2, 0, map->entities, nullptr); //Door frame entity.
				childEntity->x = entity->x;
				childEntity->y = entity->y;
				TileEntityList.addEntity(*childEntity);
				//printlog("19 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->sizex = 8;
				childEntity->sizey = 1;
				childEntity->yaw -= PI / 2.0;
				childEntity->behavior = &actDoor;
				childEntity->flags[BLOCKSIGHT] = true;
				childEntity->skill[0] = 1; // signify behavior code of DOOR_DIR
				childEntity->seedEntityRNG(map_server_rng.getU32());

				// copy editor options from frame to door itself.
				childEntity->doorDisableLockpicks = entity->doorDisableLockpicks;
				childEntity->doorForceLockedUnlocked = entity->doorForceLockedUnlocked;
				childEntity->doorDisableOpening = entity->doorDisableOpening;

				childEntity = newEntity(doorFrameSprite(), 0, map->entities, nullptr); //Door entity.
				childEntity->flags[INVISIBLE] = true;
				childEntity->flags[BLOCKSIGHT] = true;
				childEntity->x = entity->x - 7;
				childEntity->y = entity->y;
				childEntity->yaw -= PI / 2.0;

				TileEntityList.addEntity(*childEntity);
				//printlog("20 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->sizex = 2;
				childEntity->sizey = 2;
				childEntity->behavior = &actDoorFrame;

				childEntity = newEntity(doorFrameSprite(), 0, map->entities, nullptr); //Door frame entity.
				childEntity->flags[INVISIBLE] = true;
				childEntity->flags[BLOCKSIGHT] = true;
				childEntity->x = entity->x + 7;
				childEntity->y = entity->y;
				childEntity->yaw -= PI / 2.0;

				TileEntityList.addEntity(*childEntity);
				//printlog("21 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->sizex = 2;
				childEntity->sizey = 2;
				childEntity->behavior = &actDoorFrame;
				break;
			}
			// east torch:
			case 4:
			{
				if ( darkmap )
				{
					list_RemoveNode(entity->mynode);
					entity = NULL;
					break;
				}
				entity->behavior = &actTorch;
				entity->x += 1;
				entity->y += 8;
				entity->z -= 1;
				entity->sprite = 3;
				entity->flags[PASSABLE] = true;
				break;
				// south torch:
			}
			case 5:
			{
				if ( darkmap )
				{
					list_RemoveNode(entity->mynode);
					entity = NULL;
					break;
				}
				entity->behavior = &actTorch;
				entity->x += 8;
				entity->y += 1;
				entity->z -= 1;
				entity->yaw += PI / 2.0;
				entity->sprite = 3;
				entity->flags[PASSABLE] = true;
				break;
			}
			// west torch:
			case 6:
			{
				if ( darkmap )
				{
					list_RemoveNode(entity->mynode);
					entity = NULL;
					break;
				}
				entity->behavior = &actTorch;
				entity->x += 15;
				entity->y += 8;
				entity->z -= 1;
				entity->yaw += PI;
				entity->sprite = 3;
				entity->flags[PASSABLE] = true;
				break;
			}
			// north torch:
			case 7:
			{
				if ( darkmap )
				{
					list_RemoveNode(entity->mynode);
					entity = NULL;
					break;
				}
				entity->behavior = &actTorch;
				entity->x += 8;
				entity->y += 15;
				entity->z -= 1;
				entity->yaw += 3 * PI / 2.0;
				entity->sprite = 3;
				entity->flags[PASSABLE] = true;
				break;
			}
			// item:
			case 68:
			case 69:
			case 8:
			{
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x += 8;
				entity->y += 8;
				entity->roll = PI / 2.0;
				if ( entity->itemContainer == 0 )
				{
					entity->yaw = (map_rng.rand() % 360) * PI / 180.0;
				}
				entity->flags[PASSABLE] = true;
				entity->behavior = &actItem;
				if ( entity->sprite == 68 )   // magic_bow.png
				{
					entity->skill[10] = ARTIFACT_BOW;
				}
				else if ( entity->sprite == 69 )     // magic_spear.png
				{
					entity->skill[10] = ARTIFACT_SPEAR;
					entity->x += 8;
					entity->y += 8;
				}
				else if ( entity->skill[10] == 0 || entity->skill[10] == 1 )
				{
					if ( entity->skill[16] == 0 )
					{
						// random category default, either set by editor or spawn naturally.
						if ( !itemsdonebefore && !strcmp(map->name, "Start Map") )
						{
							entity->skill[10] = READABLE_BOOK;
						}
						else
						{
							bool extrafood = false;
							switch ( balance )
							{
								case 2:
									if ( map_rng.rand() % 8 == 0 )
									{
										extrafood = true;
									}
									break;
								case 3:
									if ( map_rng.rand() % 6 == 0 )
									{
										extrafood = true;
									}
									break;
								case 4:
									if ( map_rng.rand() % 5 == 0 )
									{
										extrafood = true;
									}
									break;
								default:
									extrafood = false;
									break;
							}
							if ( !extrafood )
							{
								if ( map_rng.rand() % 2 == 0 )
								{
									// possible magicstaff
									int randType = map_rng.rand() % (NUMCATEGORIES - 1);
									if ( randType == THROWN && map_rng.rand() % 3 ) // THROWN items 66% to be re-roll.
									{
										randType = map_rng.rand() % (NUMCATEGORIES - 1);
									}
									entity->skill[10] = itemLevelCurve(static_cast<Category>(randType), 0, currentlevel, map_rng);
								}
								else
								{
									// impossible magicstaff
									int randType = map_rng.rand() % (NUMCATEGORIES - 2);
									if ( randType >= MAGICSTAFF )
									{
										randType++;
									}
									if ( randType == THROWN && map_rng.rand() % 3 ) // THROWN items 66% to be re-roll.
									{
										randType = map_rng.rand() % (NUMCATEGORIES - 2);
										if ( randType >= MAGICSTAFF )
										{
											randType++;
										}
									}
									entity->skill[10] = itemLevelCurve(static_cast<Category>(randType), 0, currentlevel, map_rng);
								}
							}
							else
							{
								entity->skill[10] = itemLevelCurve(FOOD, 0, currentlevel, map_rng);
							}
						}
					}
					else
					{
						// editor set the random category of the item to be spawned.
						if ( entity->skill[16] > 0 && entity->skill[16] <= 13 )
						{
							entity->skill[10] = itemLevelCurve(static_cast<Category>(entity->skill[16] - 1), 0, currentlevel, map_rng);
						}
						else
						{
							int randType = 0;
							if ( entity->skill[16] == 14 )
							{
								// equipment
								randType = map_rng.rand() % 2;
								if ( randType == 0 )
								{
									entity->skill[10] = itemLevelCurve(static_cast<Category>(WEAPON), 0, currentlevel, map_rng);
								}
								else if ( randType == 1 )
								{
									entity->skill[10] = itemLevelCurve(static_cast<Category>(ARMOR), 0, currentlevel, map_rng);
								}
							}
							else if ( entity->skill[16] == 15 )
							{
								// jewelry
								randType = map_rng.rand() % 2;
								if ( randType == 0 )
								{
									entity->skill[10] = itemLevelCurve(static_cast<Category>(AMULET), 0, currentlevel, map_rng);
								}
								else
								{
									entity->skill[10] = itemLevelCurve(static_cast<Category>(RING), 0, currentlevel, map_rng);
								}
							}
							else if ( entity->skill[16] == 16 )
							{
								// magical
								randType = map_rng.rand() % 3;
								if ( randType == 0 )
								{
									entity->skill[10] = itemLevelCurve(static_cast<Category>(SCROLL), 0, currentlevel, map_rng);
								}
								else if ( randType == 1 )
								{
									entity->skill[10] = itemLevelCurve(static_cast<Category>(MAGICSTAFF), 0, currentlevel, map_rng);
								}
								else
								{
									entity->skill[10] = itemLevelCurve(static_cast<Category>(SPELLBOOK), 0, currentlevel, map_rng);
								}
							}
						}
					}
				}
				else if ( entity->skill[10] != 0 && entity->skill[10] != 1 ) //editor set the item type
				{
					entity->skill[10] = entity->skill[10] - 2; //reduce by 2 as the editor treats 1 as random, 0 is NULL
				}

				if ( entity->sprite == 8 )
				{
					if ( entity->skill[11] == 0 ) //random
					{
						entity->skill[11] = 1 + map_rng.rand() % 4; // status
					}
					else
					{
						entity->skill[11]--; //editor set number, sets this value to 0-5, with 1 being BROKEN, 5 being EXCELLENT
					}
				}
				else
				{
					entity->skill[11] = DECREPIT + (currentlevel > 5) + (currentlevel > 15) + (currentlevel > 20);
				}
				if ( entity->sprite == 8 )
				{
					if ( entity->skill[12] == 10 ) //random, else the value of this variable is the curse/bless
					{
						if ( map_rng.rand() % 2 == 0 )   // 50% chance of curse/bless
						{
							entity->skill[12] = -2 + map_rng.rand() % 5;
						}
						else
						{
							entity->skill[12] = 0;
						}
					}
				}
				else
				{
					entity->skill[12] = 1;
				}
				if ( entity->sprite == 8 )
				{
					if ( entity->skill[13] == 0 )
					{
						entity->skill[13] = 1; // count set by maps.cpp, otherwise set by editor
					}
					else if ( entity->skill[13] == 1 )
					{
						if ( items[entity->skill[10]].category == FOOD )
						{
							switch ( balance )
							{
								case 2:
									if ( map_rng.rand() % 3 == 0 )
									{
										entity->skill[13] += map_rng.rand() % 2;
									}
									break;
								case 3:
									if ( map_rng.rand() % 3 == 0 )
									{
										entity->skill[13] += map_rng.rand() % 3;
									}
									break;
								case 4:
									if ( map_rng.rand() % 2 == 0 )
									{
										entity->skill[13] += map_rng.rand() % 3;
									}
									break;
								default:
									break;
							}
						}
					}
				}

				if ( !itemsdonebefore && !strcmp(map->name, "Start Map") )
				{
					entity->skill[14] = getBook("My Journal");
				}
				else
				{
					if ( items[entity->skill[10]].category == SCROLL
						|| items[entity->skill[10]].variations > 1
						|| entity->skill[10] == FOOD_TIN )
					{
						entity->skill[14] = map_rng.rand();    // appearance
					}
					else
					{
						entity->skill[14] = 0;    // appearance
					}
				}
				if ( entity->skill[15] == 1 ) // editor set as identified
				{
					entity->skill[15] = 1;
				}
				else if ( entity->skill[15] == 0 ) // unidentified (default)
				{
					entity->skill[15] = 0;
				}
				else  if ( entity->skill[15] == 2 ) // editor set as random
				{
					entity->skill[15] = map_rng.rand() % 2;
				}
				else
				{
					entity->skill[15] = 0; // unidentified.
				}

				if ( entity->skill[10] == ENCHANTED_FEATHER )
				{
					entity->skill[14] = 75 + 25 * (map_rng.rand() % 2);    // appearance
				}
				else if ( entity->skill[10] >= BRONZE_TOMAHAWK && entity->skill[10] <= CRYSTAL_SHURIKEN )
				{
					// thrown weapons always fixed status. (tomahawk = decrepit, shuriken = excellent)
					entity->skill[11] = std::min(DECREPIT + (entity->skill[10] - BRONZE_TOMAHAWK), static_cast<int>(EXCELLENT));
				}

				auto item = newItemFromEntity(entity);
				entity->sprite = itemModel(item);
				if ( !entity->itemNotMoving )
				{
					// shurikens and chakrams need to lie flat on floor as their models are rotated.
					if ( item->type == CRYSTAL_SHURIKEN || item->type == STEEL_CHAKRAM || item->type == BOOMERANG )
					{
						entity->roll = PI;
						if ( item->type == CRYSTAL_SHURIKEN )
						{
							entity->z = 8.5 - models[entity->sprite]->sizey * .25;
						}
						else if ( item->type == BOOMERANG )
						{
							entity->z = 9.0 - models[entity->sprite]->sizey * .25;
						}
						else
						{
							entity->z = 8.75 - models[entity->sprite]->sizey * .25;
						}
					}
					else if ( item->type == TOOL_BOMB || item->type == TOOL_FREEZE_BOMB
						|| item->type == TOOL_SLEEP_BOMB || item->type == TOOL_TELEPORT_BOMB )
					{
						entity->roll = PI;
						entity->z = 7 - models[entity->sprite]->sizey * .25;
					}
					else
					{
						entity->z = 7.5 - models[entity->sprite]->sizey * .25;
					}
				}
				entity->itemNotMoving = 1; // so the item retains its position
				entity->itemNotMovingClient = 1; // so the item retains its position for clients
				entity->skill[16] = 0; // reset this as we used it for the category, but now skill[16] is ITEM_LIFE
				itemsdonebefore = true;
				if ( !strcmp(map->name, "Sokoban") && item->type == ARTIFACT_GLOVES ) // artifact gloves.
				{
					entity->flags[INVISIBLE] = true;
					entity->itemSokobanReward = 1;
				}
				free(item);
				break;
			}
			// gold:
			case 9:
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x += 8;
				entity->y += 8;
				if ( entity->goldInContainer == 0 )
				{
					entity->yaw = (map_rng.rand() % 360) * PI / 180.0;
				}
				entity->flags[PASSABLE] = true;
				entity->behavior = &actGoldBag;
				entity->goldBouncing = 1;
				if ( entity->goldAmount == 0 )
				{
					entity->goldAmount = 10 + map_rng.rand() % 100 + (currentlevel); // amount
				}
				if ( entity->goldAmount < 5 )
				{
					entity->sprite = 1379;
					entity->z = 7.75;
				}
				else
				{
					entity->sprite = 130; // gold bag model
					entity->z = 6.25;
				}
				if ( !strcmp(map->name, "Sokoban") )
				{
					entity->flags[INVISIBLE] = true;
					entity->goldSokoban = 1;
				}
				break;
			// monster:
			case 71:
			case 70:
			case 62:
			case 48:
			case 36:
			case 35:
			case 30:
			case 27:
			case 10:
			case 75:
			case 76:
			case 77:
			case 78:
			case 79:
			case 193:
			case 194:
			case 195:
			case 196:
			case 197:
			case 80:
			case 81:
			case 82:
			case 83:
			case 84:
			case 85:
			case 86:
			case 87:
			case 88:
			case 89:
			case 90:
			case 91:
			case 92:
			case 93:
			case 94:
			case 95:
			case 163:
			case 164:
			case 165:
			case 166:
			case 188:
			case 189:
			{
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x += 8;
				entity->y += 8;
				entity->z = 6;
				entity->yaw = (map_rng.rand() % 360) * PI / 180.0;
				entity->behavior = &actMonster;
				entity->flags[UPDATENEEDED] = true;
				entity->flags[INVISIBLE] = true;
				entity->skill[5] = -1;
				Stat* myStats = NULL;
				if ( multiplayer != CLIENT )
				{
					myStats = entity->getStats();
				}
				//Assign entity creature list pointer.
				entity->addToCreatureList(map->creatures);

				Monster monsterType = editorSpriteTypeToMonster(entity->sprite);
				bool monsterIsFixedSprite = true;

				if ( monsterType == NOTHING )
				{
					monsterIsFixedSprite = false;
					monsterType = static_cast<Monster>(monsterCurve(currentlevel));
					if ( customMonsterCurveExists )
					{
						Monster customMonsterType = static_cast<Monster>(monsterCurveCustomManager.rollMonsterFromCurve(map->name));
						if ( customMonsterType != NOTHING )
						{
							monsterType = customMonsterType;
						}
						else
						{
							customMonsterCurveExists = false;
						}
					}
				}

				if ( monsterType == MIMIC )
				{
					entity->yaw = 90 * (map_rng.rand() % 4) * PI / 180.0;
					entity->monsterLookDir = entity->yaw;
				}
				else if ( monsterType == BAT_SMALL )
				{
					entity->monsterSpecialState = BAT_REST;
				}

				entity->seedEntityRNG(map_server_rng.getU32());

				if ( multiplayer != CLIENT )
				{
					if ( myStats == nullptr )
					{
						// need to give the entity its list stuff.
						// create an empty first node for traversal purposes
						node_t* node2 = list_AddNodeFirst(&entity->children);
						node2->element = nullptr;
						node2->deconstructor = &emptyDeconstructor;

						if ( entity->sprite == 10 )
						{
							// if the sprite is 10, then choose from monsterCurve.
							// Create the stat struct again for the new monster
							myStats = new Stat(monsterType + 1000);
						}
						else
						{
							// if monster not random, then create the stat struct here
							// should not occur (unless we hack it)
							myStats = new Stat(entity->sprite);
						}
						node2 = list_AddNodeLast(&entity->children);
						node2->element = myStats;
						node2->deconstructor = &statDeconstructor;
						node2->size = sizeof(myStats);
					}
					else if ( entity->sprite == 10 )
					{
						// monster is random, but generated from editor
						// stat struct is already created, need to set stats
						setDefaultMonsterStats(myStats, monsterType + 1000);

						Uint32 monsterseed = 0;
						entity->entity_rng->getSeed(&monsterseed, sizeof(monsterseed));
						BaronyRNG tmpRng;
						tmpRng.seedBytes(&monsterseed, sizeof(monsterseed));
						setRandomMonsterStats(myStats, tmpRng);
					}

					std::string checkName = myStats->name;
					if ( checkName.find(".json") != std::string::npos )
					{
						monsterCurveCustomManager.createMonsterFromFile(entity, myStats, checkName, monsterType);
					}
					else if ( customMonsterCurveExists )
					{
						std::string variantName = "default";
						if ( monsterIsFixedSprite )
						{
							if ( isMonsterStatsDefault(*myStats) )
							{
								variantName = monsterCurveCustomManager.rollFixedMonsterVariant(map->name, monsterType);
							}
						}
						else
						{
							variantName = monsterCurveCustomManager.rollMonsterVariant(map->name, monsterType);
						}
						
						if ( variantName.compare("default") != 0 )
						{
							// find a custom file name.
							monsterCurveCustomManager.createMonsterFromFile(entity, myStats, variantName, monsterType);
						}
					}
				}
				if ( multiplayer != CLIENT )
				{
					myStats->type = monsterType;
					if ( myStats->type == SLIME )
					{
						switch ( entity->sprite )
						{
						case 193:
							myStats->setAttribute("slime_type", "slime green");
							break;
						case 194:
							myStats->setAttribute("slime_type", "slime blue");
							break;
						case 195:
							myStats->setAttribute("slime_type", "slime red");
							break;
						case 196:
							myStats->setAttribute("slime_type", "slime tar");
							break;
						case 197:
							myStats->setAttribute("slime_type", "slime metal");
							break;
						default:
							break;
						}
					}
					if ( myStats->type == DEVIL )
					{
						auto childEntity = newEntity(72, 1, map->entities, nullptr);
						//printlog("Generated devil spawner. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
						childEntity->x = entity->x - 8;
						childEntity->y = entity->y - 8;
						childEntity->setUID(-3);
						entity_uids--;
					}
				}
				break;
			}
			// ladder:
			case 11:
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x += 8;
				entity->y += 8;
				entity->z = 5.5;
				entity->flags[PASSABLE] = true;
				entity->behavior = &actLadder;
				entity->sprite = 161; // ladder
				break;
			// campfire:
			case 12:
				if ( darkmap )
				{
					list_RemoveNode(entity->mynode);
					entity = NULL;
					break;
				}
				entity->sizex = 3;
				entity->sizey = 3;
				entity->x += 8;
				entity->y += 8;
				entity->z = 6;
				entity->flags[PASSABLE] = true;
				entity->behavior = &actCampfire;
				entity->sprite = 162; // firepit
				break;
			//The Mystical Fountain of TODO:
			case 14:
			{
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x += 8;
				entity->y += 8;
				entity->z = 6.5;
				entity->behavior = &actFountain;
				entity->sprite = 163; //Fountain
				entity->skill[0] = 1; //Fountain is full.
				entity->seedEntityRNG(map_server_rng.getU32());

				//Randomly determine effect.
				int effect = map_rng.rand() % 10; //3 possible effects.
				entity->skill[28] = 1; //TODO: This is just for testing purposes.
				switch (effect)
				{
					case 0:
						//10% chance
						entity->skill[1] = 3; //Will bless all equipment.
						if ( (map_rng.rand() % 4) != 0 )
						{
							entity->skill[1] = 4; //Will bless only one piece of equipment.
						}
						break;
					case 1:
					case 2:
						//20% chance.
						entity->skill[1] = 0; //Will spawn a succubus.
						break;
					case 3:
					case 4:
					case 5:
						//30% chance.
						entity->skill[1] = 1; //Will raise nutrients.
						break;
					case 6:
					case 7:
					case 8:
					case 9:
						//40% chance.
						//Random potion effect.
						entity->skill[1] = 2;
						entity->skill[3] = map_rng.rand() % 15; //Randomly choose from the number of potion effects there are.
						break;
					default:
						break; //Should never happen.
				}
				break;
			}
			//Sink.
			case 15:
			{
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x += 8;
				entity->y += 8;
				entity->z = 5;
				entity->behavior = &actSink;
				entity->sprite = 164;
				entity->skill[0] = 1 + map_rng.rand() % 4; // number of uses
				switch ( map_rng.rand() % 10 )
				{
					case 0:
						//10% chance.
						entity->skill[3] = 0; //Player will find a ring.
						break;
					case 1:
						//10% chance.
						entity->skill[3] = 1; //Will spawn a slime.
						break;
					case 2:
					case 3:
					case 4:
					case 5:
					case 6:
					case 7:
						//60% chance.
						entity->skill[3] = 2; //Will raise nutrition.
						break;
					case 8:
					case 9:
						//20% chance.
						entity->skill[3] = 3; //Player will lose 1 HP.
						break;
					default:
						break;
				}

				entity->seedEntityRNG(map_server_rng.getU32());
				break;
			}
			//Switch.
			case 17:
            {
                entity->sizex = 1;
                entity->sizey = 1;
                entity->x += 8;
                entity->y += 8;
                entity->z = 7.5;
                entity->sprite = 184; // this is the switch base.
                entity->flags[PASSABLE] = true;
                auto childEntity = newEntity(186, 0, map->entities, nullptr); //Switch entity.
                childEntity->x = entity->x;
                childEntity->y = entity->y;
                TileEntityList.addEntity(*childEntity);
                //printlog("22 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
                childEntity->z = 8.5;
                childEntity->focalz = -4.5;
                childEntity->sizex = 1;
                childEntity->sizey = 1;
                childEntity->sprite = 185; // this is the switch handle.
                childEntity->roll = PI / 4; // "off" position
                childEntity->flags[PASSABLE] = true;
                childEntity->behavior = &actSwitch;
                entity->parent = childEntity->getUID();
                break;
            }
			//Circuit.
			case 18:
				entity->sizex = 3;
				entity->sizey = 3;
				entity->x += 8;
				entity->y += 8;
				entity->z = 5;
				entity->behavior = &actCircuit;
				entity->flags[PASSABLE] = true;
				entity->flags[INVISIBLE] = true;
				entity->flags[NOUPDATE] = true;
				//entity->sprite = 164; //No sprite.
				entity->skill[28] = 1; //It's a depowered powerable.
				break;
			//North/South gate: //TODO: Adjust this. It's a copypaste of door.
			case 19:
            {
                entity->x += 8;
                entity->y += 8;
                entity->yaw -= PI / 2.0;
                entity->sprite = doorFrameSprite();
                entity->flags[PASSABLE] = true;
                entity->behavior = &actDoorFrame;
                
                //entity->skill[28] = 1; //It's a mechanism.
                auto childEntity = newEntity(186, 0, map->entities, nullptr); //Gate entity.
                childEntity->x = entity->x;
                childEntity->y = entity->y;
                TileEntityList.addEntity(*childEntity);
                //printlog("23 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
                childEntity->sizex = 8;
                childEntity->sizey = 1;
                childEntity->yaw -= PI / 2.0;
                childEntity->gateInverted = 0; // non-inverted
                childEntity->gateStatus = 0; // closed.
                childEntity->skill[28] = 1; //It's a mechanism.
                childEntity->behavior = &actGate;
                childEntity->skill[0] = 1; // signify behavior code of DOOR_DIR
                
                // copy editor options from frame to gate itself.
                childEntity->gateDisableOpening = entity->gateDisableOpening;
                
                childEntity = newEntity(doorFrameSprite(), 0, map->entities, nullptr); //Door frame entity.
                childEntity->flags[INVISIBLE] = true;
                childEntity->flags[BLOCKSIGHT] = true;
                childEntity->x = entity->x - 7;
                childEntity->y = entity->y;
				childEntity->yaw -= PI / 2.0;
                TileEntityList.addEntity(*childEntity);
                //printlog("24 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
                childEntity->sizex = 2;
                childEntity->sizey = 2;
                childEntity->behavior = &actDoorFrame;
                
                childEntity = newEntity(doorFrameSprite(), 0, map->entities, nullptr); //Door frame entity.
                childEntity->flags[INVISIBLE] = true;
                childEntity->flags[BLOCKSIGHT] = true;
                childEntity->x = entity->x + 7;
                childEntity->y = entity->y;
				childEntity->yaw -= PI / 2.0;
                TileEntityList.addEntity(*childEntity);
                //printlog("25 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
                childEntity->sizex = 2;
                childEntity->sizey = 2;
                childEntity->behavior = &actDoorFrame;
                break;
            }
			//East/west gate: //TODO: Adjust this. It's a copypaste of door.
			case 20:
            {
                entity->x += 8;
                entity->y += 8;
                entity->sprite = doorFrameSprite();
                entity->flags[PASSABLE] = true;
                entity->behavior = &actDoorFrame;
                
                auto childEntity = newEntity(186, 0, map->entities, nullptr); //Gate entity.
                childEntity->x = entity->x;
                childEntity->y = entity->y;
                TileEntityList.addEntity(*childEntity);
                //printlog("26 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
                childEntity->sizex = 1;
                childEntity->sizey = 8;
                childEntity->gateInverted = 0; // non-inverted
                childEntity->gateStatus = 0; // closed.
                childEntity->skill[28] = 1; //It's a mechanism.
                childEntity->behavior = &actGate;
                childEntity->skill[0] = 0; // signify behavior code of DOOR_DIR
                
                // copy editor options from frame to gate itself.
                childEntity->gateDisableOpening = entity->gateDisableOpening;
                
                childEntity = newEntity(doorFrameSprite(), 0, map->entities, nullptr); //Door frame entity.
                childEntity->flags[INVISIBLE] = true;
                childEntity->flags[BLOCKSIGHT] = true;
                childEntity->x = entity->x;
                childEntity->y = entity->y - 7;
                TileEntityList.addEntity(*childEntity);
                //printlog("27 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
                childEntity->sizex = 2;
                childEntity->sizey = 2;
                childEntity->behavior = &actDoorFrame;
                
                childEntity = newEntity(doorFrameSprite(), 0, map->entities, nullptr); //Door frame entity.
                childEntity->flags[INVISIBLE] = true;
                childEntity->flags[BLOCKSIGHT] = true;
                childEntity->x = entity->x;
                childEntity->y = entity->y + 7;
                TileEntityList.addEntity(*childEntity);
                //printlog("28 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
                childEntity->sizex = 2;
                childEntity->sizey = 2;
                childEntity->behavior = &actDoorFrame;
                break;
            }
			//Chest.
			case 21:
			{
				entity->sizex = 3;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->z = 6;
				entity->yaw = entity->yaw * (PI / 2); //set to 0 by default in editor, can be set 0-3
				entity->behavior = &actChest;
				entity->sprite = 188;
				//entity->skill[9] = -1; //Set default chest as random category < 0

				entity->seedEntityRNG(map_server_rng.getU32());

				auto childEntity = newEntity(216, 0, map->entities, nullptr); //Chest lid entity.
				childEntity->parent = entity->getUID();
				entity->parent = childEntity->getUID();
				if ( entity->yaw == 0 ) //EAST FACING
				{
					childEntity->x = entity->x - 3;
					childEntity->y = entity->y;
				}
				else if ( entity->yaw == PI / 2 ) //SOUTH FACING
				{
					childEntity->x = entity->x;
					childEntity->y = entity->y - 3;
				}
				else if ( entity->yaw == PI ) //WEST FACING
				{
					childEntity->x = entity->x + 3;
					childEntity->y = entity->y;
				}
				else if (entity->yaw == 3 * PI/2 ) //NORTH FACING
				{
					childEntity->x = entity->x;
					childEntity->y = entity->y + 3;
				}
				else
				{
					childEntity->x = entity->x;
					childEntity->y = entity->y - 3;
				}
				TileEntityList.addEntity(*childEntity);
				//printlog("29 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->z = entity->z - 2.75;
				childEntity->focalx = 3;
				childEntity->focalz = -.75;
				childEntity->yaw = entity->yaw;
				childEntity->sizex = 2;
				childEntity->sizey = 2;
				childEntity->behavior = &actChestLid;
				childEntity->flags[PASSABLE] = true;

				//Chest inventory.
				node_t* tempNode = list_AddNodeFirst(&entity->children);
				tempNode->element = NULL;
				tempNode->deconstructor = &emptyDeconstructor;

				if ( !strcmp(map->name, "The Mystic Library") && !vampireQuestChest )
				{
					vampireQuestChest = entity;
				}
				break;
			}
			// liquid marker
			case 28:
				entity->sizex = 8;
				entity->sizey = 8;
				entity->x += 8;
				entity->y += 8;
				entity->behavior = &actLiquid;
				entity->flags[SPRITE] = true;
				entity->flags[INVISIBLE] = true;
				entity->skill[2] = -9; // so clients know it's a liquid
				break;
			// arrow trap
			case 32:
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->behavior = &actArrowTrap;
				entity->flags[SPRITE] = true;
				entity->flags[INVISIBLE] = true;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				entity->skill[28] = 1; // is a mechanism
				entity->skill[1] = QUIVER_SILVER + map_rng.rand() % 7; // random arrow type.
				if ( currentlevel <= 15 )
				{
					while ( entity->skill[1] == QUIVER_CRYSTAL || entity->skill[1] == QUIVER_PIERCE )
					{
						entity->skill[1] = QUIVER_SILVER + map_rng.rand() % 7; // random arrow type.
					}
				}
				entity->skill[3] = 0; // refire type.
				break;
			// trap (pressure plate thingy)
			case 33:
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->behavior = &actTrap;
				entity->flags[SPRITE] = true;
				entity->flags[INVISIBLE] = true;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				break;
			// trap permanent (pressure plate thingy) (remains on)
			case 34:
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->behavior = &actTrapPermanent;
				entity->flags[SPRITE] = true;
				entity->flags[INVISIBLE] = true;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				break;
			// minotaur spawn trap
			case 37:
			{
				entity->skill[28] = 1; // is a mechanism
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->behavior = &actMinotaurTrap;
				entity->flags[SPRITE] = true;
				entity->flags[INVISIBLE] = true;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				entity->seedEntityRNG(map_server_rng.getU32());
				break;
			}
			// summon monster trap
			case 97:
			{
				if ( entity->skill[9] == 0 ) // no auto activate
				{
					entity->skill[28] = 1; // is a mechanism
				}
				if ( entity->skill[1] == 0 )
				{
					// not generated by editor, set monster qty to 1.
					entity->skill[1] = 1;
				}
				if ( entity->skill[2] == 0 )
				{
					// not generated by editor, set time between spawns to 1.
					entity->skill[2] = 1;
				}
				if ( entity->skill[3] == 0 )
				{
					// not generated by editor, amount spawns to 1.
					entity->skill[3] = 1;
				}
				if ( entity->skill[4] > 1 )
				{
					// catch invalid input by editor, require power flag
					entity->skill[4] = 0;
				}
				if ( entity->skill[5] > 100 )
				{
					// catch invalid input by editor, chance to fail set to 0
					entity->skill[5] = 0;
				}
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->behavior = &actSummonTrap;
				entity->flags[SPRITE] = true;
				entity->flags[INVISIBLE] = true;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				entity->seedEntityRNG(map_server_rng.getU32());
				break;
			}
			// boulder trap
			case 38:
			{
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->behavior = &actBoulderTrap;
				entity->flags[SPRITE] = true;
				entity->flags[INVISIBLE] = true;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				entity->skill[28] = 1; // is a mechanism
				for ( int c = 0; c < 4; c++ )
				{
                    int x, y;
					switch ( c )
					{
						case 0:
							x = 16;
							y = 0;
							break;
						case 1:
							x = 0;
							y = 16;
							break;
						case 2:
							x = -16;
							y = 0;
							break;
						case 3:
							x = 0;
							y = -16;
							break;
					}
					x = ((int)(x + entity->x)) >> 4;
					y = ((int)(y + entity->y)) >> 4;
					if ( x >= 0 && y >= 0 && x < map->width && y < map->height )
					{
						if ( !map->tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map->height] )
						{
							Entity* childEntity = newEntity(252, 1, map->entities, nullptr);
							childEntity->x = (x << 4) + 8;
							childEntity->y = (y << 4) + 8;
							//printlog("30 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
							childEntity->flags[PASSABLE] = true;
							if ( !map->tiles[(MAPLAYERS - 1) + y * MAPLAYERS + x * MAPLAYERS * map->height] )
							{
								childEntity->z = -26.99;
							}
							else
							{
								childEntity->z = -10.99;
							}
							childEntity->behavior = &actBoulderTrapHole;
							TileEntityList.addEntity(*childEntity);
							entity->boulderTrapRocksToSpawn |= (1 << c); // add this location to spawn a boulder below the trapdoor model.
						}
					}
				}
				break;
			}
			// headstone
			case 39:
			{
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x += 8;
				entity->y += 8;
				entity->sprite = 224; // gravestone_faded.vox
				entity->z = 8 - models[entity->sprite]->sizez / 4;
				entity->behavior = &actHeadstone;
				entity->skill[28] = 1; // is a mechanism
				if ( !strcmp(map->name, "The Haunted Castle") )
				{
					entity->flags[INVISIBLE] = true;
					entity->flags[PASSABLE] = true;
				}
				entity->seedEntityRNG(map_server_rng.getU32());
				break;
			}
			// model tester
			case 40:
				entity->behavior = &actRotate;
				entity->sprite = 0;
				break;
			// lava
			case 41:
				entity->sizex = 8;
				entity->sizey = 8;
				entity->x += 8;
				entity->y += 8;
				entity->behavior = &actLiquid;
				entity->flags[USERFLAG1] = true; // this is lava
				entity->flags[SPRITE] = true;
				entity->flags[INVISIBLE] = true;
				entity->skill[2] = -9; // so clients know it's a liquid
				break;
			// ladder hole
			case 43:
            {
                entity->x += 8;
                entity->y += 8;
                entity->sprite = 253;
                entity->flags[PASSABLE] = true;
                entity->behavior = &actLadderUp;
                int x = entity->x / 16;
                int y = entity->y / 16;
                if ( x >= 0 && y >= 0 && x < map->width && y < map->height )
                {
                    if ( !map->tiles[(MAPLAYERS - 1) + y * MAPLAYERS + x * MAPLAYERS * map->height] )
                    {
                        entity->z = -6.25 - 16.0;
                    }
                    else
                    {
                        entity->z = -6.25;
                    }
                }
                break;
            }
			// boulder
			case 44:
			{
				entity->x += 8;
				entity->y += 8;
				entity->sprite = 245;
				entity->sizex = 7;
				entity->sizey = 7;
				entity->behavior = &actBoulder;
				entity->skill[0] = 1; // BOULDER_STOPPED
				entity->seedEntityRNG(map_server_rng.getU32());
				break;
			}
			// portal
			case 45:
				entity->x += 8;
				entity->y += 8;
				entity->sprite = 254;
				entity->sizex = 4;
				entity->sizey = 4;
				entity->yaw = PI / 2;
				entity->behavior = &actPortal;
				if ( !strcmp(map->name, "Mages Guild") )
				{
					entity->skill[3] = 1; // not secret portal, just aesthetic.
				}
				else if ( !strcmp(map->name, "Hell") && currentlevel == 23 )
				{
					entity->portalNotSecret = 1; // not secret portal, just aesthetic.
				}
				entity->flags[PASSABLE] = true;
				break;
			// secret ladder:
			case 46:
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x += 8;
				entity->y += 8;
				entity->z = 5.5;
				entity->flags[PASSABLE] = true;
				entity->behavior = &actLadder;
				entity->sprite = 161; // ladder
				entity->skill[3] = 1; // LADDER_SECRET = 1;
				break;
			// table
			case 59:
			{
				entity->sizex = 5;
				entity->sizey = 4;
				entity->x += 8;
				entity->y += 8;
				entity->z = 8;
				entity->focalz = -3;
				entity->sprite = 271;
				entity->behavior = &actFurniture;
				entity->seedEntityRNG(map_server_rng.getU32());
				entity->flags[BURNABLE] = true;
				entity->furnitureType = FURNITURE_TABLE;
				if ( entity->furnitureDir != -1 )
				{
					entity->yaw = entity->furnitureDir * 45 * (PI / 180.f);
					if ( entity->furnitureDir == 0 || entity->furnitureDir == 4 )
					{
						entity->sizex = 5;
						entity->sizey = 4;
					}
					else if ( entity->furnitureDir == 2 || entity->furnitureDir == 6 )
					{
						entity->sizex = 4;
						entity->sizey = 5;
					}
					else
					{
						entity->sizex = 6;
						entity->sizey = 6;
					}
				}
				bool doItem = false;
				if ( entity->furnitureTableRandomItemChance == -1 )
				{
					if ( map_rng.rand() % 4 == 0 || !strcmp(map->name, "Start Map") )
					{
						doItem = true;
					}
				}
				else if ( entity->furnitureTableRandomItemChance > 1 )
				{
					if ( map_rng.rand() % 100 < entity->furnitureTableRandomItemChance )
					{
						doItem = true;
					}
				}
				if ( doItem )
				{
					// put an item on the table
					auto childEntity = newEntity(8, 1, map->entities, nullptr);
					setSpriteAttributes(childEntity, nullptr, nullptr);
					childEntity->x = entity->x - 8;
					childEntity->y = entity->y - 8;
					TileEntityList.addEntity(*childEntity);
					//printlog("31 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
					childEntity->z = 0;
					childEntity->itemNotMoving = 1; // so the item retains its position
					childEntity->itemNotMovingClient = 1; // so the item retains its position for clients
					entity->parent = childEntity->getUID();
				}

				bool doChairs = false;
				int numChairs = 0;
				if ( entity->furnitureTableSpawnChairs == -1 )
				{
					if ( map_rng.rand() % 2 == 0 )
					{
						doChairs = true;
					}
				}
				else if ( entity->furnitureTableSpawnChairs > 0 )
				{
					doChairs = true;
				}
				if ( doChairs )
				{
					// surround the table with chairs
					if ( entity->furnitureTableSpawnChairs == -1 )
					{
						numChairs = map_rng.rand() % 4 + 1;
					}
					else
					{
						numChairs = entity->furnitureTableSpawnChairs;
					}
					for ( int c = 0; c < numChairs; c++ )
					{
						auto childEntity = newEntity(60, 1, map->entities, nullptr);
						setSpriteAttributes(childEntity, nullptr, nullptr);
						childEntity->x = entity->x - 8;
						childEntity->y = entity->y - 8;
						//printlog("32 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
						childEntity->yaw = ((int)(map_rng.rand() % 80) - 40 + c * 90) * (PI / 180.f);
						if ( childEntity->yaw >= PI * 2 )
						{
							childEntity->yaw -= PI * 2;
						}
						else if ( childEntity->yaw < 0 )
						{
							childEntity->yaw += PI * 2;
						}
						childEntity->x -= cos(childEntity->yaw) * 7;
						childEntity->y -= sin(childEntity->yaw) * 7;
						TileEntityList.addEntity(*childEntity);
					}
				}
				break;
			}
			// chair
			case 60:
			{
				entity->furnitureType = FURNITURE_CHAIR; // so everything knows I'm a chair
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->z = 8;
				entity->focalz = -5;
				entity->sprite = 272;
				entity->behavior = &actFurniture;
				entity->seedEntityRNG(map_server_rng.getU32());
				entity->flags[BURNABLE] = true;
				if ( entity->furnitureDir == -1 )
				{
					if ( !entity->yaw )
					{
						entity->yaw = (map_rng.rand() % 360) * (PI / 180.f);
					}
				}
				else
				{
					entity->yaw = entity->furnitureDir * 45 * (PI / 180.f);
				}
				break;
			}
			// MC easter egg:
			case 61:
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->z = 4;
				entity->sprite = 273;
				entity->behavior = &actMCaxe;
				entity->flags[PASSABLE] = true;
				break;
			// end game portal:
			case 63:
				entity->x += 8;
				entity->y += 8;
				entity->sprite = 278;
				entity->sizex = 4;
				entity->sizey = 4;
				entity->yaw = PI / 2;
				entity->behavior = &actWinningPortal;
				entity->flags[PASSABLE] = true;
				if ( strstr(map->name, "Boss") )
				{
					entity->flags[INVISIBLE] = true;
					entity->skill[28] = 1; // is a mechanism
				}
				if ( strstr(map->name, "Hell") )
				{
					entity->skill[4] = 2;
					entity->flags[INVISIBLE] = true;
					entity->skill[28] = 1; // is a mechanism
				}
				else
				{
					entity->skill[4] = 1;
				}
				break;
			// speartrap:
			case 64:
            {
                entity->sizex = 6;
                entity->sizey = 6;
                entity->x += 8;
                entity->y += 8;
                entity->z = 16;
                entity->focalz = 7;
                entity->sprite = 282;
                entity->behavior = &actSpearTrap;
                entity->skill[28] = 1; // is a mechanism
                entity->flags[PASSABLE] = true;
                auto childEntity = newEntity(283, 0, map->entities, nullptr);
                childEntity->x = entity->x;
                childEntity->y = entity->y;
                TileEntityList.addEntity(*childEntity);
                //printlog("33 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
                childEntity->z = entity->z - 7.75 - 0.01;
                childEntity->flags[PASSABLE] = true;
                break;
            }
			// magic trap:
			case 65:
			{
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->behavior = &actMagicTrap;
				entity->flags[SPRITE] = true;
				entity->flags[INVISIBLE] = true;
				entity->flags[PASSABLE] = true;
				entity->skill[28] = 1; // is a mechanism
				entity->seedEntityRNG(map_server_rng.getU32());
				break;
			}
			// wall buster:
			case 66:
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->behavior = &actWallBuster;
				entity->flags[SPRITE] = true;
				entity->flags[INVISIBLE] = true;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				entity->skill[28] = 1; // is a mechanism
				break;
			// wall builder:
			case 67:
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->behavior = &actWallBuilder;
				entity->flags[SPRITE] = true;
				entity->flags[INVISIBLE] = true;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				entity->skill[28] = 1; // is a mechanism
				break;
			// devil/other teleport location:
			case 72:
			case 73:
			case 74:
			case 128:
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->behavior = &actDevilTeleport;
				entity->flags[SPRITE] = true;
				entity->flags[INVISIBLE] = true;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				break;

			// east crystal shard:
			case 98:
			{
				if ( darkmap )
				{
					list_RemoveNode(entity->mynode);
					entity = NULL;
					break;
				}
				entity->behavior = &actCrystalShard;
				entity->x += 1;
				entity->y += 8;
				entity->z -= 1;
				entity->sprite = 587;
				entity->flags[PASSABLE] = true;
				break;
			}
			// south crystal shard:
			case 99:
			{
				if ( darkmap )
				{
					list_RemoveNode(entity->mynode);
					entity = NULL;
					break;
				}
				entity->behavior = &actCrystalShard;
				entity->x += 8;
				entity->y += 1;
				entity->z -= 1;
				entity->yaw += PI / 2.0;
				entity->sprite = 587;
				entity->flags[PASSABLE] = true;
				break;
			}
			// west crystal shard:
			case 100:
			{
				if ( darkmap )
				{
					list_RemoveNode(entity->mynode);
					entity = NULL;
					break;
				}
				entity->behavior = &actCrystalShard;
				entity->x += 15;
				entity->y += 8;
				entity->z -= 1;
				entity->yaw += PI;
				entity->sprite = 587;
				entity->flags[PASSABLE] = true;
				break;
			}
			// north crystal shard:
			case 101:
			{
				if ( darkmap )
				{
					list_RemoveNode(entity->mynode);
					entity = NULL;
					break;
				}
				entity->behavior = &actCrystalShard;
				entity->x += 8;
				entity->y += 15;
				entity->z -= 1;
				entity->yaw += 3 * PI / 2.0;
				entity->sprite = 587;
				entity->flags[PASSABLE] = true;
				break;
			}

			// boulder trap east
			case 102:
			{
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->behavior = &actBoulderTrapEast;
				entity->flags[SPRITE] = true;
				entity->flags[INVISIBLE] = true;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				entity->skill[28] = 1; // is a mechanism
				entity->boulderTrapPreDelay = entity->boulderTrapPreDelay * TICKS_PER_SECOND; // convert seconds to ticks from editor

				const int x = ((int)(entity->x)) >> 4;
				const int y = ((int)(entity->y)) >> 4;
				if ( x >= 0 && y >= 0 && x < map->width && y < map->height )
				{
					if ( !map->tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map->height] )
					{
						Entity* childEntity = newEntity(252, 1, map->entities, nullptr);
						childEntity->x = (x << 4) + 8;
						childEntity->y = (y << 4) + 8;
						//printlog("30 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
						childEntity->flags[PASSABLE] = true;
						if ( !map->tiles[(MAPLAYERS - 1) + y * MAPLAYERS + x * MAPLAYERS * map->height] )
						{
							childEntity->z = -26.99;
						}
						else
						{
							childEntity->z = -10.99;
						}
						childEntity->behavior = &actBoulderTrapHole;
						TileEntityList.addEntity(*childEntity);
					}
				}
				break;
			}

			// boulder trap south
			case 103:
			{
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->behavior = &actBoulderTrapSouth;
				entity->flags[SPRITE] = true;
				entity->flags[INVISIBLE] = true;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				entity->skill[28] = 1; // is a mechanism
				entity->boulderTrapPreDelay = entity->boulderTrapPreDelay * TICKS_PER_SECOND; // convert seconds to ticks from editor

				const int x = ((int)(entity->x)) >> 4;
				const int y = ((int)(entity->y)) >> 4;
				if ( x >= 0 && y >= 0 && x < map->width && y < map->height )
				{
					if ( !map->tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map->height] )
					{
						Entity* childEntity = newEntity(252, 1, map->entities, nullptr);
						childEntity->x = (x << 4) + 8;
						childEntity->y = (y << 4) + 8;
						//printlog("30 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
						childEntity->flags[PASSABLE] = true;
						if ( !map->tiles[(MAPLAYERS - 1) + y * MAPLAYERS + x * MAPLAYERS * map->height] )
						{
							childEntity->z = -26.99;
						}
						else
						{
							childEntity->z = -10.99;
						}
						childEntity->behavior = &actBoulderTrapHole;
						TileEntityList.addEntity(*childEntity);
					}
				}
				break;
			}

			// boulder trap west
			case 104:
			{
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->behavior = &actBoulderTrapWest;
				entity->flags[SPRITE] = true;
				entity->flags[INVISIBLE] = true;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				entity->skill[28] = 1; // is a mechanism
				entity->boulderTrapPreDelay = entity->boulderTrapPreDelay * TICKS_PER_SECOND; // convert seconds to ticks from editor

				const int x = ((int)(entity->x)) >> 4;
				const int y = ((int)(entity->y)) >> 4;
				if ( x >= 0 && y >= 0 && x < map->width && y < map->height )
				{
					if ( !map->tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map->height] )
					{
						Entity* childEntity = newEntity(252, 1, map->entities, nullptr);
						childEntity->x = (x << 4) + 8;
						childEntity->y = (y << 4) + 8;
						//printlog("30 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
						childEntity->flags[PASSABLE] = true;
						if ( !map->tiles[(MAPLAYERS - 1) + y * MAPLAYERS + x * MAPLAYERS * map->height] )
						{
							childEntity->z = -26.99;
						}
						else
						{
							childEntity->z = -10.99;
						}
						childEntity->behavior = &actBoulderTrapHole;
						TileEntityList.addEntity(*childEntity);
					}
				}
				break;
			}

			// boulder trap north
			case 105:
			{
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->behavior = &actBoulderTrapNorth;
				entity->flags[SPRITE] = true;
				entity->flags[INVISIBLE] = true;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				entity->skill[28] = 1; // is a mechanism
				entity->boulderTrapPreDelay = entity->boulderTrapPreDelay * TICKS_PER_SECOND; // convert seconds to ticks from editor

				const int x = ((int)(entity->x)) >> 4;
				const int y = ((int)(entity->y)) >> 4;
				if ( x >= 0 && y >= 0 && x < map->width && y < map->height )
				{
					if ( !map->tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map->height] )
					{
						Entity* childEntity = newEntity(252, 1, map->entities, nullptr);
						childEntity->x = (x << 4) + 8;
						childEntity->y = (y << 4) + 8;
						//printlog("30 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
						childEntity->flags[PASSABLE] = true;
						if ( !map->tiles[(MAPLAYERS - 1) + y * MAPLAYERS + x * MAPLAYERS * map->height] )
						{
							childEntity->z = -26.99;
						}
						else
						{
							childEntity->z = -10.99;
						}
						childEntity->behavior = &actBoulderTrapHole;
						TileEntityList.addEntity(*childEntity);
					}
				}
				break;
			}

			// power crystal
			case 106:
			{
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x += 8;
				entity->y += 8;
				entity->z = 6.5;
				entity->behavior = &actPowerCrystalBase;
				entity->sprite = 577; //crystal base
				entity->yaw = entity->yaw * (PI / 2); // rotate as set in editor
				entity->flags[PASSABLE] = false;

				auto childEntity = newEntity(578, 0, map->entities, nullptr); //floating crystal
				childEntity->parent = entity->getUID();

				childEntity->x = entity->x;
				childEntity->y = entity->y;
				childEntity->sizex = 4;
				childEntity->sizey = 4;
				childEntity->crystalStartZ = entity->z - 10; //start position
				childEntity->z = childEntity->crystalStartZ - 0.4 + ((map_rng.rand() % 8) * 0.1); // start the height randomly
				childEntity->crystalMaxZVelocity = 0.02; //max velocity
				childEntity->crystalMinZVelocity = 0.001; //min velocity
				childEntity->crystalTurnVelocity = 0.2; //yaw turning velocity
				childEntity->vel_z = childEntity->crystalMaxZVelocity * ((map_rng.rand() % 99) * 0.01 + 0.01); // start the velocity randomly

				childEntity->crystalNumElectricityNodes = entity->crystalNumElectricityNodes; //number of electricity nodes to generate in facing direction.
				childEntity->crystalTurnReverse = entity->crystalTurnReverse;
				childEntity->crystalSpellToActivate = entity->crystalSpellToActivate;
				if ( childEntity->crystalSpellToActivate )
				{
					childEntity->z = childEntity->crystalStartZ + 5;
					childEntity->vel_z = childEntity->crystalMaxZVelocity * 2;
				}
				childEntity->yaw = entity->yaw;
				childEntity->sizex = 4;
				childEntity->sizey = 4;
				childEntity->behavior = &actPowerCrystal;
				childEntity->flags[PASSABLE] = true;
				TileEntityList.addEntity(*childEntity);

				node_t* tempNode = list_AddNodeLast(&entity->children);
				tempNode->element = childEntity; // add the node to the children list.
				tempNode->deconstructor = &emptyDeconstructor;
				tempNode->size = sizeof(Entity*);

				break;
			}
			// set beartrap
			case 107:
			{
				entity->skill[0] = 0;
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x += 8;
				entity->y += 8;
				entity->z = 6.75;

				//entity->focalz = -5;
				entity->sprite = 668;

				entity->behavior = &actBeartrap;
				entity->flags[PASSABLE] = true;
				entity->flags[UPDATENEEDED] = true;
				if ( !entity->yaw )
				{
					entity->yaw = (map_rng.rand() % 360) * (PI / 180.f);
				}
				entity->roll = -PI / 2; // flip the model

				entity->skill[11] = DECREPIT; //status
				entity->skill[12] = 0; //beatitude
				entity->skill[13] = 1; //qty
				entity->skill[14] = 0; //appearance
				entity->skill[15] = 0; //identified
				entity->skill[17] = -1; //owner
				break;
			}
			case 108: //stalag column
				entity->x += 8;
				entity->y += 8;
				entity->sprite = 580;
				entity->sizex = 8;
				entity->sizey = 8;
				entity->z = -7.75;
				entity->flags[BLOCKSIGHT] = false;
				entity->behavior = &actStalagColumn;
				break;
			case 109: //stalagmite single
				entity->x += 8;
				entity->y += 8;
				entity->sprite = 581;
				entity->sizex = 4;
				entity->sizey = 4;
				entity->z = 1.75;
				entity->flags[BLOCKSIGHT] = false;
				entity->behavior = &actStalagFloor;
				break;
			case 110: //stalagmite multiple
				entity->x += 8;
				entity->y += 8;
				entity->sprite = 582;
				entity->sizex = 7;
				entity->sizey = 7;
				entity->z = -1;
				entity->flags[BLOCKSIGHT] = false;
				entity->behavior = &actStalagFloor;
				break;
			case 111: //stalagtite single
            {
                entity->x += 8;
                entity->y += 8;
                entity->sprite = 583;
                entity->sizex = 4;
                entity->sizey = 4;
                entity->z = -1.75;
                const int x = entity->x / 16;
                const int y = entity->y / 16;
                entity->flags[BLOCKSIGHT] = false;
                entity->behavior = &actStalagCeiling;
                if ( x >= 0 && y >= 0 && x < map->width && y < map->height )
                {
                    if ( !map->tiles[(MAPLAYERS - 1) + y * MAPLAYERS + x * MAPLAYERS * map->height] )
                    {
                        entity->flags[PASSABLE] = true;
                        entity->z -= 16;
                    }
                }
                break;
            }
			case 112: //stalagtite multiple
            {
                entity->x += 8;
                entity->y += 8;
                entity->sprite = 584;
                entity->sizex = 7;
                entity->sizey = 7;
                entity->z = 1;
                const int x = entity->x / 16;
                const int y = entity->y / 16;
                entity->flags[BLOCKSIGHT] = false;
                entity->behavior = &actStalagCeiling;
                if ( x >= 0 && y >= 0 && x < map->width && y < map->height )
                {
                    if ( !map->tiles[(MAPLAYERS - 1) + y * MAPLAYERS + x * MAPLAYERS * map->height] )
                    {
                        entity->flags[PASSABLE] = true;
                        entity->z -= 16;
                    }
                }
                break;
            }
			//North/South gate inverted: //TODO: Adjust this. It's a copypaste of door.
			case 113:
            {
                entity->x += 8;
                entity->y += 8;
                entity->yaw -= PI / 2.0;
                entity->sprite = doorFrameSprite();
                entity->flags[PASSABLE] = true;
                entity->behavior = &actDoorFrame;
                
                //entity->skill[28] = 1; //It's a mechanism.
                auto childEntity = newEntity(186, 0, map->entities, nullptr);
                childEntity->x = entity->x;
                childEntity->y = entity->y;
                TileEntityList.addEntity(*childEntity);
                //printlog("23 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
                childEntity->sizex = 8;
                childEntity->sizey = 1;
                childEntity->yaw -= PI / 2.0;
                childEntity->gateInverted = 1; // inverted.
                childEntity->gateStatus = 1; // open.
                childEntity->skill[28] = 1; //It's a mechanism.
                childEntity->behavior = &actGate;
                childEntity->skill[0] = 1; // signify behavior code of DOOR_DIR
                
                // copy editor options from frame to gate itself.
                childEntity->gateDisableOpening = entity->gateDisableOpening;
                
                childEntity = newEntity(doorFrameSprite(), 0, map->entities, nullptr);
                childEntity->flags[INVISIBLE] = true;
                childEntity->flags[BLOCKSIGHT] = true;
                childEntity->x = entity->x - 7;
                childEntity->y = entity->y;
				childEntity->yaw -= PI / 2.0;
                TileEntityList.addEntity(*childEntity);
                //printlog("24 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
                childEntity->sizex = 2;
                childEntity->sizey = 2;
                childEntity->behavior = &actDoorFrame;
                
                childEntity = newEntity(doorFrameSprite(), 0, map->entities, nullptr);
                childEntity->flags[INVISIBLE] = true;
                childEntity->flags[BLOCKSIGHT] = true;
                childEntity->x = entity->x + 7;
                childEntity->y = entity->y;
				childEntity->yaw -= PI / 2.0;
                TileEntityList.addEntity(*childEntity);
                //printlog("25 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
                childEntity->sizex = 2;
                childEntity->sizey = 2;
                childEntity->behavior = &actDoorFrame;
                break;
            }
			//East/west gate inverted: //TODO: Adjust this. It's a copypaste of door.
			case 114:
            {
                entity->x += 8;
                entity->y += 8;
                entity->sprite = doorFrameSprite();
                entity->flags[PASSABLE] = true;
                entity->behavior = &actDoorFrame;
                
                auto childEntity = newEntity(186, 0, map->entities, nullptr);
                childEntity->x = entity->x;
                childEntity->y = entity->y;
                TileEntityList.addEntity(*childEntity);
                //printlog("26 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
                childEntity->sizex = 1;
                childEntity->gateInverted = 1; // inverted.
                childEntity->gateStatus = 1; // open.
                childEntity->sizey = 8;
                childEntity->skill[28] = 1; //It's a mechanism.
                childEntity->behavior = &actGate;
                childEntity->skill[0] = 0; // signify behavior code of DOOR_DIR
                
                // copy editor options from frame to gate itself.
                childEntity->gateDisableOpening = entity->gateDisableOpening;
                
                childEntity = newEntity(doorFrameSprite(), 0, map->entities, nullptr);
                childEntity->flags[INVISIBLE] = true;
                childEntity->flags[BLOCKSIGHT] = true;
                childEntity->x = entity->x;
                childEntity->y = entity->y - 7;
                TileEntityList.addEntity(*childEntity);
                //printlog("27 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
                childEntity->sizex = 2;
                childEntity->sizey = 2;
                childEntity->behavior = &actDoorFrame;
                
                childEntity = newEntity(doorFrameSprite(), 0, map->entities, nullptr);
                childEntity->flags[INVISIBLE] = true;
                childEntity->flags[BLOCKSIGHT] = true;
                childEntity->x = entity->x;
                childEntity->y = entity->y + 7;
                TileEntityList.addEntity(*childEntity);
                //printlog("28 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
                childEntity->sizex = 2;
                childEntity->sizey = 2;
                childEntity->behavior = &actDoorFrame;
                break;
            }
			//Switch with timer.
			case 115:
            {
                entity->sizex = 1;
                entity->sizey = 1;
                entity->x += 8;
                entity->y += 8;
                entity->z = 7.5;
                entity->sprite = 585; // this is the switch base.
                entity->flags[PASSABLE] = true;
                auto childEntity = newEntity(586, 0, map->entities, nullptr);
                childEntity->x = entity->x;
                childEntity->y = entity->y;
                TileEntityList.addEntity(*childEntity);
                //printlog("22 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
                childEntity->z = 8.5;
                childEntity->leverTimerTicks = std::max(entity->leverTimerTicks, 1) * TICKS_PER_SECOND; // convert seconds to ticks from editor, make sure not less than 1
                childEntity->leverStatus = 0; // set default to off.
                childEntity->focalz = -4.5;
                childEntity->sizex = 1;
                childEntity->sizey = 1;
                childEntity->sprite = 586; // this is the switch handle.
                childEntity->roll = -PI / 4; // "off" position
                childEntity->flags[PASSABLE] = true;
                childEntity->behavior = &actSwitchWithTimer;
                entity->parent = childEntity->getUID();
                break;
            }
			// pedestal
			case 116:
			{
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x += 8;
				entity->y += 8;
				entity->z = 4.5;
				entity->behavior = &actPedestalBase;
				entity->sprite = 601; //pedestal base
				entity->flags[PASSABLE] = false;
				entity->pedestalOrbType = entity->pedestalOrbType + 1;// set in editor as 0-3, need 1-4.
				if ( entity->pedestalHasOrb == 1 ) // set in editor
				{
					entity->pedestalHasOrb = entity->pedestalOrbType;
				}
				//entity->pedestalInvertedPower // set in editor
				entity->pedestalInit = 0;
				//entity->pedestalInGround = 0; // set in editor
				//entity->pedestalLockOrb // set in editor
				if ( entity->pedestalInGround )
				{
					entity->z += 11;
					entity->flags[PASSABLE] = true;
				}

				auto childEntity = newEntity(602 + entity->pedestalOrbType - 1, 0, map->entities, nullptr); //floating orb
				childEntity->parent = entity->getUID();
				childEntity->behavior = &actPedestalOrb;
				childEntity->x = entity->x;
				childEntity->y = entity->y;
				TileEntityList.addEntity(*childEntity);
				childEntity->z = -2;
				childEntity->sizex = 2;
				childEntity->sizey = 2;
				childEntity->flags[UNCLICKABLE] = true;
				childEntity->flags[PASSABLE] = true;
				childEntity->flags[INVISIBLE] = false;
				if ( entity->pedestalInGround )
				{
					childEntity->z += 11;
					childEntity->orbStartZ = -2;
				}
				childEntity->pedestalOrbInit();

				node_t* tempNode = list_AddNodeLast(&entity->children);
				tempNode->element = childEntity; // add the node to the children list.
				tempNode->deconstructor = &emptyDeconstructor;
				tempNode->size = sizeof(Entity*);
				break;
			}
			// mid game portal:
			case 117:
				entity->x += 8;
				entity->y += 8;
				entity->sprite = 614;
				entity->sizex = 4;
				entity->sizey = 4;
				entity->yaw = PI / 2;
				entity->behavior = &actMidGamePortal;
				entity->flags[PASSABLE] = true;
				if ( strstr(map->name, "Boss") )
				{
					entity->flags[INVISIBLE] = true;
					entity->skill[28] = 1; // is a mechanism
				}
				/*if ( strstr(map->name, "Hell") )
				{
					entity->skill[4] = 2;
				}
				else
				{
					entity->skill[4] = 1;
				}*/
				break;
			// teleporter.
			case 118:
				entity->x += 8;
				entity->y += 8;
				entity->flags[PASSABLE] = true;
				if ( entity->teleporterType == 0 )
				{
					entity->sprite = 618; // ladder hole
					entity->behavior = &actTeleporter;
					const int x = entity->x / 16;
					const int y = entity->y / 16;
					if ( x >= 0 && y >= 0 && x < map->width && y < map->height )
					{
						if ( !map->tiles[(MAPLAYERS - 1) + y * MAPLAYERS + x * MAPLAYERS * map->height] )
						{
							entity->z = -6.25 - 16.0;
						}
						else
						{
							entity->z = -6.25;
						}
					}
				}
				else if ( entity->teleporterType == 1 )
				{
					entity->sizex = 4;
					entity->sizey = 4;
					entity->z = 5.5;
					entity->flags[PASSABLE] = true;
					entity->behavior = &actTeleporter;
					entity->sprite = 619; // ladder
				}
				else
				{
					entity->sprite = 254;
					entity->sizex = 4;
					entity->sizey = 4;
					entity->yaw = PI / 2;
					entity->behavior = &actTeleporter;
					entity->flags[PASSABLE] = true;
				}
				break;
			// ceiling tile:
			case 119:
				entity->x += 8;
				entity->y += 8;
				entity->z = -24;
				if ( entity->ceilingTileModel != 0 )
				{
					entity->sprite = entity->ceilingTileModel;
				}
				else
				{
					entity->sprite = 621;
				}
				entity->sizex = 8;
				entity->sizey = 8;
				entity->yaw = entity->ceilingTileDir * 90 * (PI / 180.f);
				entity->behavior = &actCeilingTile;
				entity->flags[PASSABLE] = true;
				entity->flags[BLOCKSIGHT] = false;
				break;
			// spell trap ceiling
			case 120:
			{
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->behavior = &actMagicTrapCeiling;
				entity->flags[SPRITE] = true;
				entity->flags[INVISIBLE] = true;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				entity->skill[28] = 1; // is a mechanism
				entity->spellTrapRefireRate = entity->spellTrapRefireRate * TICKS_PER_SECOND; // convert seconds to ticks from editor
				entity->seedEntityRNG(map_server_rng.getU32());

				const int x = ((int)(entity->x)) >> 4;
				const int y = ((int)(entity->y)) >> 4;
				//map->tiles[y * MAPLAYERS + x * MAPLAYERS * map->height] = 208; //entity->spellTrapCeilingModel
				Entity* childEntity = nullptr;
				if ( x >= 0 && y >= 0 && x < map->width && y < map->height )
				{
					if ( !map->tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map->height] )
					{
						childEntity = newEntity(644, 1, map->entities, nullptr);
						childEntity->parent = entity->getUID();
						childEntity->x = entity->x;
						childEntity->y = entity->y;
						//printlog("30 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
						childEntity->flags[PASSABLE] = true;
						if ( !map->tiles[(MAPLAYERS - 1) + y * MAPLAYERS + x * MAPLAYERS * map->height] )
						{
							childEntity->z = -22.99;
						}
						else
						{
							childEntity->z = -6.99;
						}
						TileEntityList.addEntity(*childEntity);
						node_t* tempNode = list_AddNodeLast(&entity->children);
						tempNode->element = childEntity; // add the node to the children list.
						tempNode->deconstructor = &emptyDeconstructor;
						tempNode->size = sizeof(Entity*);

						childEntity = newEntity(645, 1, map->entities, nullptr);
						childEntity->parent = entity->getUID();
						childEntity->x = entity->x;
						childEntity->y = entity->y;
						childEntity->z = 8.24;
						TileEntityList.addEntity(*childEntity);
						//printlog("30 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
						childEntity->flags[PASSABLE] = true;
						tempNode = list_AddNodeLast(&entity->children);
						tempNode->element = childEntity; // add the node to the children list.
						tempNode->deconstructor = &emptyDeconstructor;
						tempNode->size = sizeof(Entity*);
					}
				}
				break;
			}
			// arcane chair
			case 121:
			{
				entity->furnitureType = FURNITURE_CHAIR; // so everything knows I'm a chair
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->z = 8;
				entity->focalz = -5;
				entity->sprite = 626;
				entity->behavior = &actFurniture;
				entity->seedEntityRNG(map_server_rng.getU32());
				entity->flags[BURNABLE] = true;
				if ( entity->furnitureDir == -1 && !entity->yaw )
				{
					entity->yaw = (map_rng.rand() % 360) * (PI / 180.f);
				}
				else
				{
					entity->yaw = entity->furnitureDir * 45 * (PI / 180.f);
				}
				break;
			}
			// arcane bed
			case 122:
			{
				entity->furnitureType = FURNITURE_BED; // so everything knows I'm a bed
				entity->x += 8;
				entity->y += 8;
				entity->z = 4;
				entity->sprite = 627;
				entity->behavior = &actFurniture;
				entity->seedEntityRNG(map_server_rng.getU32());
				entity->flags[BURNABLE] = true;
				if ( entity->furnitureDir == -1 && !entity->yaw )
				{
					entity->furnitureDir = (map_rng.rand() % 4);
					entity->furnitureDir *= 2; // create an even number
					entity->yaw = entity->furnitureDir * 45 * (PI / 180.f);
				}
				else
				{
					entity->yaw = entity->furnitureDir * 45 * (PI / 180.f);
				}
				if ( entity->furnitureDir == 0 || entity->furnitureDir == 4 )
				{
					entity->sizex = 8;
					entity->sizey = 4;
				}
				else if ( entity->furnitureDir == 2 || entity->furnitureDir == 6 )
				{
					entity->sizex = 4;
					entity->sizey = 8;
				}
				else
				{
					entity->sizex = 8;
					entity->sizey = 8;
				}
				break;
			}
			// bunk bed
			case 123:
			{
				entity->furnitureType = FURNITURE_BUNKBED; // so everything knows I'm a bunkbed
				entity->x += 8;
				entity->y += 8;
				entity->z = 1.75;
				entity->sprite = 628;
				entity->behavior = &actFurniture;
				entity->seedEntityRNG(map_server_rng.getU32());
				entity->flags[BURNABLE] = true;
				if ( entity->furnitureDir == -1 && !entity->yaw )
				{
					entity->furnitureDir = (map_rng.rand() % 4);
					entity->furnitureDir *= 2; // create an even number
					entity->yaw = entity->furnitureDir * 45 * (PI / 180.f);
				}
				else
				{
					entity->yaw = entity->furnitureDir * 45 * (PI / 180.f);
				}
				if ( entity->furnitureDir == 0 || entity->furnitureDir == 4 )
				{
					entity->sizex = 8;
					entity->sizey = 4;
				}
				else if ( entity->furnitureDir == 2 || entity->furnitureDir == 6 )
				{
					entity->sizex = 4;
					entity->sizey = 8;
				}
				else
				{
					entity->sizex = 8;
					entity->sizey = 8;
				}
				break;
			}
			// column.
			case 124:
			{
				entity->x += 8;
				entity->y += 8;
				entity->sprite = 629;
				entity->sizex = 6;
				entity->sizey = 6;
				entity->z = -7.75;
				entity->flags[BLOCKSIGHT] = false;
				entity->behavior = &actColumn;
				break;
			}
			// podium
			case 125:
			{
				entity->sizex = 3;
				entity->sizey = 3;
				entity->x += 8;
				entity->y += 8;
				entity->z = 7.75;
				entity->focalz = -3;
				entity->sprite = 630;
				entity->behavior = &actFurniture;
				entity->seedEntityRNG(map_server_rng.getU32());
				entity->furnitureType = FURNITURE_PODIUM;
				entity->flags[BURNABLE] = true;
				if ( entity->furnitureDir == -1 && !entity->yaw )
				{
					entity->furnitureDir = (map_rng.rand() % 4);
					entity->furnitureDir *= 2; // create an even number
					entity->yaw = entity->furnitureDir * 45 * (PI / 180.f);
				}
				else
				{
					entity->yaw = entity->furnitureDir * 45 * (PI / 180.f);
				}
				break;
			}
			// piston.
			case 126:
			{
				entity->x += 8;
				entity->y += 8;
				entity->sprite = 631;
				entity->sizex = 8;
				entity->sizey = 8;
				entity->z = 0;
				entity->flags[BLOCKSIGHT] = false;
				entity->behavior = &actPistonBase;

				auto childEntity = newEntity(632, 1, map->entities, nullptr); //cam1
				childEntity->parent = entity->getUID();
				childEntity->x = entity->x + 2.25;
				childEntity->y = entity->y + 2.25;
				childEntity->behavior = &actPistonCam;
				childEntity->pistonCamRotateSpeed = 0.2;
				childEntity->flags[UNCLICKABLE] = true;
				TileEntityList.addEntity(*childEntity);
				/*if ( multiplayer != CLIENT )
				{
					entity_uids--;
				}
				childEntity->setUID(-3);*/
				childEntity = newEntity(633, 1, map->entities, nullptr); //cam2
				childEntity->parent = entity->getUID();
				childEntity->x = entity->x - 2.25;
				childEntity->y = entity->y - 2.25;
				childEntity->behavior = &actPistonCam;
				childEntity->pistonCamRotateSpeed = -0.2;
				childEntity->flags[UNCLICKABLE] = true;
				TileEntityList.addEntity(*childEntity);
				/*if ( multiplayer != CLIENT )
				{
					entity_uids--;
				}
				childEntity->setUID(-3);*/
				break;
			}
			// grass texture
			case 127:
			{
				entity->x += 8;
				entity->y += 8;
				entity->sprite = entity->floorDecorationModel;
				entity->sizex = 0;
				entity->sizey = 0;
				entity->z = 7.5 - entity->floorDecorationHeightOffset * 0.25;
				entity->x += entity->floorDecorationXOffset * 0.25;
				entity->y += entity->floorDecorationYOffset * 0.25;
				if ( entity->floorDecorationRotation == -1 )
				{
					entity->yaw = (map_rng.rand() % 8) * (PI / 4);
				}
				else
				{
					entity->yaw = entity->floorDecorationRotation * (PI / 4);
				}
				entity->flags[BLOCKSIGHT] = false;
				entity->flags[PASSABLE] = true;
				if ( entity->floorDecorationInteractText1 == 0 )
				{
					entity->flags[UNCLICKABLE] = true;
				}
				entity->behavior = &actFloorDecoration;
				/*if ( multiplayer != CLIENT )
				{
					entity_uids--;
				}
				entity->setUID(-3);*/
				break;
			}
			// expansion end game portal:
			case 129: {
				entity->x += 8;
				entity->y += 8;
				entity->sprite = 614;
				entity->sizex = 4;
				entity->sizey = 4;
				entity->yaw = PI / 2;
				entity->behavior = &actExpansionEndGamePortal;
				entity->flags[PASSABLE] = true;
				//entity->flags[INVISIBLE] = true;
				int victoryType;
				switch (stats[clientnum]->playerRace) {
				default: victoryType = 3; break;
	            case RACE_HUMAN: victoryType = 4; break;
	            case RACE_SKELETON: victoryType = 5; break;
	            case RACE_VAMPIRE: victoryType = 5; break;
	            case RACE_SUCCUBUS: victoryType = 5; break;
	            case RACE_GOATMAN: victoryType = 3; break;
	            case RACE_AUTOMATON: victoryType = 4; break;
	            case RACE_INCUBUS: victoryType = 5; break;
	            case RACE_GOBLIN: victoryType = 3; break;
	            case RACE_INSECTOID: victoryType = 3; break;
	            case RACE_RAT: victoryType = 3; break;
	            case RACE_TROLL: victoryType = 3; break;
	            case RACE_SPIDER: victoryType = 3; break;
	            case RACE_IMP: victoryType = 5; break;
				}
				entity->portalVictoryType = victoryType;
				entity->skill[28] = 1; // is a mechanism
				break;
			}
			//sound source
			case 130: 
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->behavior = &actSoundSource;
				entity->flags[SPRITE] = true;
				entity->flags[INVISIBLE] = true;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				entity->skill[28] = 1; // is a mechanism
				break;
			//light source
			case 131:
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->behavior = &actLightSource;
				entity->flags[SPRITE] = true;
				entity->flags[INVISIBLE] = true;
				entity->flags[PASSABLE] = true;
				//entity->flags[NOUPDATE] = true;
				entity->skill[28] = 1; // is a mechanism
				break;
			//text source
			case 132:
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->behavior = &actTextSource;
				entity->flags[SPRITE] = true;
				entity->flags[INVISIBLE] = true;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				entity->skill[28] = 1; // is a mechanism
				break;
			//signal timer
			case 133:
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->behavior = &actSignalTimer;
				entity->flags[SPRITE] = true;
				entity->flags[INVISIBLE] = true;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				entity->skill[28] = 1; // is a mechanism
				break;
			//custom teleporter
			case 161:
				entity->x += 8;
				entity->y += 8;
				entity->sprite = entity->portalCustomSprite;
				entity->sizex = 4;
				entity->sizey = 4;
				entity->yaw = PI / 2;
				entity->behavior = &actCustomPortal;
				entity->flags[PASSABLE] = true;
				if ( entity->portalCustomRequiresPower )
				{
					entity->flags[INVISIBLE] = true;
				}
				entity->z = 7.5 - entity->portalCustomZOffset * 0.25;
				if ( entity->portalCustomRequiresPower == 1 )
				{
					entity->skill[28] = 1; // is a mechanism
				}
				break;
			case 162:
			{
				// readable book
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x += 8;
				entity->y += 8;
				entity->roll = PI / 2.0;
				if ( entity->itemContainer == 0 )
				{
					entity->yaw = (map_rng.rand() % 360) * PI / 180.0;
				}
				entity->flags[PASSABLE] = true;
				entity->behavior = &actItem;
				entity->skill[10] = READABLE_BOOK;
				if ( entity->skill[11] == 0 ) //random
				{
					entity->skill[11] = 1 + map_rng.rand() % 4; // status
				}
				else
				{
					entity->skill[11]--; //editor set number, sets this value to 0-5, with 1 being BROKEN, 5 being EXCELLENT
				}
				if ( entity->skill[12] == 10 ) //random, else the value of this variable is the curse/bless
				{
					if ( map_rng.rand() % 2 == 0 )   // 50% chance of curse/bless
					{
						entity->skill[12] = -2 + map_rng.rand() % 5;
					}
					else
					{
						entity->skill[12] = 0;
					}
				}
				entity->skill[13] = 1; // qty

				// assemble the book string.
				char buf[256] = "";
				int totalChars = 0;
				for ( int i = 40; i <= 52; ++i )
				{
					if ( i == 28 ) // circuit_status
					{
						continue;
					}
					if ( entity->skill[i] != 0 )
					{
						for ( int c = 0; c < 4; ++c )
						{
							buf[totalChars] = static_cast<char>((entity->skill[i] >> (c * 8)) & 0xFF);
							++totalChars;
						}
					}
				}
				if ( buf[totalChars] != '\0' )
				{
					buf[totalChars] = '\0';
				}
				std::string output = buf;
				size_t found = output.find("\\n");
				while ( found != std::string::npos )
				{
					output.erase(found, 2);
					output.insert(found, 1, '\n');
					found = output.find("\\n");
				}
				strcpy(buf, output.c_str());

				int index = -1;
				bool foundBook = false;
				for ( auto& book : allBooks )
				{
					++index;
					if ( book.default_name == buf )
					{
						foundBook = true;
						entity->skill[14] = getBook(buf);
						break;
					}
				}
				if ( !foundBook && allBooks.size() > 0 )
				{
					entity->skill[14] = map_rng.rand() % allBooks.size();
				}
					
				if ( entity->skill[15] == 1 ) // editor set as identified
				{
					entity->skill[15] = 1;
				}
				else if ( entity->skill[15] == 0 ) // unidentified (default)
				{
					entity->skill[15] = 0;
				}
				else  if ( entity->skill[15] == 2 ) // editor set as random
				{
					entity->skill[15] = map_rng.rand() % 2;
				}
				else
				{
					entity->skill[15] = 0; // unidentified.
				}

				auto item = newItemFromEntity(entity);
				entity->sprite = itemModel(item);
				if ( !entity->itemNotMoving )
				{
					entity->z = 7.5 - models[entity->sprite]->sizey * .25;
				}
				entity->itemNotMoving = 1; // so the item retains its position
				entity->itemNotMovingClient = 1; // so the item retains its position for clients
				free(item);
				item = nullptr;
                break;
			}
			case 168: 
				//Statue Animator
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x += 8;
				entity->y += 8;
				entity->z = 3.5;
				entity->behavior = &actStatueAnimator;
				entity->sprite = 995;
				entity->skill[0] = 0;
				break;
			case 169:
				//Statue
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x += 8;
				entity->y += 8;
				entity->z = 3.5;
				entity->behavior = &actStatue;
				entity->sprite = 995;
				entity->yaw = entity->statueDir * PI / 2;
				break;
			case 177:
				// teleport shrine
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x += 8;
				entity->y += 8;
				entity->z = 3.5 - entity->shrineZ * 0.25;
				entity->behavior = &actTeleportShrine;
				entity->sprite = 1192;
				entity->yaw = entity->shrineDir * PI / 2;
				break;
			case 178:
				// spell shrine
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x += 8;
				entity->y += 8;
				entity->z = 3.5 - entity->shrineZ * 0.25;
				//entity->behavior = &actSpellShrine;
				entity->sprite = 1193;
				entity->yaw = entity->shrineDir * PI / 2;
				break;
			case 179:
			{
				// collider decoration
				entity->x += 8;
				entity->y += 8;
				int dir = entity->colliderDecorationRotation;
				if ( dir == -1 )
				{
					dir = map_rng.rand() % 8;
					entity->colliderDecorationRotation = dir;
				}
				entity->yaw = dir * (PI / 4);
				/*static ConsoleVariable<int> debugColliderType("/collider_type", 14);
				entity->colliderDamageTypes = *debugColliderType;*/
				auto find = EditorEntityData_t::colliderData.find(entity->colliderDamageTypes);
				if ( find != EditorEntityData_t::colliderData.end() )
				{
					auto& data = find->second;
					if ( data.hasOverride("dir_offset") )
					{
						entity->yaw = ((dir + data.getOverride("dir_offset")) * (PI / 4));
					}
					if ( data.hasOverride("model") )
					{
						entity->colliderDecorationModel = data.getOverride("model");
					}
					if ( data.hasOverride("height") )
					{
						entity->colliderDecorationHeightOffset = data.getOverride("height");
					}
					if ( dir == 0 )
					{
						if ( data.hasOverride("east_x") )
						{
							entity->colliderDecorationXOffset = data.getOverride("east_x");
						}
						if ( data.hasOverride("east_y") )
						{
							entity->colliderDecorationYOffset = data.getOverride("east_y");
						}
					}
					else if ( dir == 2 )
					{
						if ( data.hasOverride("south_x") )
						{
							entity->colliderDecorationXOffset = data.getOverride("south_x");
						}
						if ( data.hasOverride("south_y") )
						{
							entity->colliderDecorationYOffset = data.getOverride("south_y");
						}
					}
					else if ( dir == 4 )
					{
						if ( data.hasOverride("west_x") )
						{
							entity->colliderDecorationXOffset = data.getOverride("west_x");
						}
						if ( data.hasOverride("west_y") )
						{
							entity->colliderDecorationYOffset = data.getOverride("west_y");
						}
					}
					else if ( dir == 6 )
					{
						if ( data.hasOverride("north_x") )
						{
							entity->colliderDecorationXOffset = data.getOverride("north_x");
						}
						if ( data.hasOverride("north_y") )
						{
							entity->colliderDecorationYOffset = data.getOverride("north_y");
						}
					}
					if ( data.hasOverride("collision") )
					{
						entity->colliderHasCollision = data.getOverride("collision");
					}
					if ( data.hasOverride("collision_x") )
					{
						entity->colliderSizeX = data.getOverride("collision_x");
					}
					if ( data.hasOverride("collision_y") )
					{
						entity->colliderSizeY = data.getOverride("collision_y");
					}
					if ( data.hasOverride("hp") )
					{
						entity->colliderMaxHP = data.getOverride("hp");
					}
					if ( data.hasOverride("diggable") )
					{
						entity->colliderDiggable = data.getOverride("diggable");
					}
				}

				entity->sprite = entity->colliderDecorationModel;
				entity->sizex = entity->colliderSizeX;
				entity->sizey = entity->colliderSizeY;
				entity->x += entity->colliderDecorationXOffset * 0.25;
				entity->y += entity->colliderDecorationYOffset * 0.25;
				entity->z = 7.5 - entity->colliderDecorationHeightOffset * 0.25;

				entity->flags[PASSABLE] = entity->colliderHasCollision == 0;
				entity->flags[BLOCKSIGHT] = false;
				entity->behavior = &actColliderDecoration;
				entity->colliderCurrentHP = entity->colliderMaxHP;
				entity->colliderOldHP = entity->colliderMaxHP;
				if ( entity->isDamageableCollider() )
				{
					entity->flags[UNCLICKABLE] = false;
				}
				else
				{
					entity->flags[UNCLICKABLE] = true;
				}
				/*if ( multiplayer != CLIENT )
				{
				entity_uids--;
				}
				entity->setUID(-3);*/
				break;
			}
			//AND gate
			case 185:
			case 186:
			case 187:
			{
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->behavior = &actSignalGateAND;
				entity->flags[SPRITE] = true;
				entity->flags[INVISIBLE] = true;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				entity->skill[28] = 1; // is a mechanism
				if ( entity->sprite == 186 ) { entity->signalInputDirection += 4; }
				if ( entity->sprite == 187 ) { entity->signalInputDirection += 8; }
				entity->sprite = -1;
				break;
			}
			case 190:
				entity->x += 8;
				entity->y += 8;
				entity->sizex = 4;
				entity->sizey = 4;
				entity->behavior = &actDaedalusShrine;
				entity->flags[PASSABLE] = false;
				entity->z = -0.25;
				entity->sprite = 1481;
				//entity->focalx = 0.75;
				entity->yaw = (map_rng.rand() % 360) * PI / 180.0;
				entity->seedEntityRNG(map_rng.getU32());
				{
					Entity* childEntity = newEntity(1480, 1, map->entities, nullptr); // base
					childEntity->parent = entity->getUID();
					childEntity->x = entity->x;
					childEntity->y = entity->y;
					childEntity->z = entity->z + 6.5;
					childEntity->yaw = 0.0;
					childEntity->sizex = 4;
					childEntity->sizey = 4;
					childEntity->flags[PASSABLE] = true;
					childEntity->flags[UNCLICKABLE] = false;
					TileEntityList.addEntity(*childEntity);
					//node_t* tempNode = list_AddNodeLast(&entity->children);
					//tempNode->element = childEntity; // add the node to the children list.
					//tempNode->deconstructor = &emptyDeconstructor;
					//tempNode->size = sizeof(Entity*);
				}
				break;
			case 191:
			{
				entity->x += 8;
				entity->y += 8;
				entity->sizex = 2;
				entity->sizey = 2;
				entity->z = 0.0;
				entity->behavior = &actBell;
				entity->flags[PASSABLE] = true;
				entity->flags[BLOCKSIGHT] = false;
				entity->flags[BURNABLE] = true;
				entity->sprite = 1478; // rope
				entity->seedEntityRNG(map_rng.getU32());
				entity->skill[11] = map_rng.rand(); // buff type
				{
					Entity* childEntity = newEntity(1475, 1, map->entities, nullptr); // bell
					childEntity->parent = entity->getUID();
					childEntity->x = entity->x - 2 * cos(entity->yaw);
					childEntity->y = entity->y - 2 * sin(entity->yaw);
					childEntity->z = -22.25;
					childEntity->yaw = entity->yaw;
					childEntity->sizex = 6;
					childEntity->sizey = 6;
					childEntity->flags[PASSABLE] = true;
					childEntity->flags[UNCLICKABLE] = false;
					childEntity->flags[UPDATENEEDED] = true;
					childEntity->flags[NOCLIP_CREATURES] = true;
					childEntity->z = entity->z;
					TileEntityList.addEntity(*childEntity);
					node_t* tempNode = list_AddNodeLast(&entity->children);
					tempNode->element = childEntity; // add the node to the children list.
					tempNode->deconstructor = &emptyDeconstructor;
					tempNode->size = sizeof(Entity*);
				}

				auto& bellRng = entity->entity_rng ? *entity->entity_rng : map_rng;
				int roll = bellRng.rand() % 4;
				if ( roll == 0 )
				{
					Entity* itemEntity = newEntity(8, 1, map->entities, nullptr);  // item
					setSpriteAttributes(itemEntity, nullptr, nullptr);
					itemEntity->x = entity->x - 8.0;
					itemEntity->y = entity->y - 8.0;
					itemEntity->z = -16;
					itemEntity->flags[INVISIBLE] = true;
					itemEntity->itemContainer = entity->getUID();
					itemEntity->yaw = entity->yaw;
					itemEntity->skill[16] = SPELLBOOK + 1;
					entity->skill[1] = itemEntity->getUID();
				}
				else if ( roll == 1 )
				{
					Entity* goldEntity = newEntity(9, 1, map->entities, nullptr);  // gold
					goldEntity->x = entity->x - 8.0;
					goldEntity->y = entity->y - 8.0;
					goldEntity->z = -16;
					goldEntity->goldAmount = 50 + bellRng.rand() % 50;
					goldEntity->flags[INVISIBLE] = true;
					entity->skill[1] = goldEntity->getUID();
				}
			}
				break;
			case 201:
				entity->x += 8;
				entity->y += 8;
				entity->sizex = 4;
				entity->sizey = 4;
				entity->behavior = &actAssistShrine;
				entity->flags[PASSABLE] = false;
				entity->z = 8.0;
				entity->sprite = 1484;
				//entity->focalx = 0.75;
				entity->yaw = 0.0;// (270)* PI / 180.0;
				entity->seedEntityRNG(map_rng.getU32());
				break;
            default:
                break;
		}
		if ( entity )
		{
			nextnode = node->next;
			TileEntityList.addEntity(*entity);
		}
	}

	for ( auto node = map->entities->first; node != nullptr; )
	{
		Entity* postProcessEntity = (Entity*)node->element;
		node = node->next;
		if ( postProcessEntity )
		{
			if ( postProcessEntity->behavior == &actGoldBag )
			{
				if ( postProcessEntity->goldInContainer != 0 && postProcessEntity->flags[INVISIBLE] == true )
				{
					if ( auto parent = uidToEntity(postProcessEntity->itemContainer) )
					{
						postProcessEntity->x = parent->x;
						postProcessEntity->y = parent->y;
					}
				}
			}
			if ( postProcessEntity->behavior == &actItem )
			{
				if ( postProcessEntity->itemContainer != 0 && postProcessEntity->flags[INVISIBLE] == true )
				{
					if ( auto parent = uidToEntity(postProcessEntity->itemContainer) )
					{
						postProcessEntity->x = parent->x;
						postProcessEntity->y = parent->y;
					}
				}

				// see if there's any platforms to set items upon.
				for ( node_t* tmpnode = map->entities->first; tmpnode != nullptr; tmpnode = tmpnode->next )
				{
					Entity* tmpentity = (Entity*)tmpnode->element;
					if ( (tmpentity->behavior == &actFurniture
							&& (tmpentity->x == postProcessEntity->x) && (tmpentity->y == postProcessEntity->y)
						) )
					{
						if ( postProcessEntity->z > 4 )
						{
							if ( tmpentity->sprite == 271 )
							{
								// is table
								postProcessEntity->z -= 6;
								tmpentity->parent = postProcessEntity->getUID();
							}
							else if ( tmpentity->sprite == 630 )
							{
								// is podium
								postProcessEntity->z -= 6;
								tmpentity->parent = postProcessEntity->getUID();
							}
						}
						break;
					}
				}
			}
			else if ( postProcessEntity->sprite == 252 && postProcessEntity->z <= -10 )
			{
				// trapdoor for boulder traps.
				int findx = static_cast<int>(postProcessEntity->x) >> 4;
				int findy = static_cast<int>(postProcessEntity->y) >> 4;
				list_t* entitiesOnTile = TileEntityList.getTileList(findx, findy);
				for ( node_t* tmpnode = entitiesOnTile->first; tmpnode != nullptr; tmpnode = tmpnode->next )
				{
					Entity* tmpentity = (Entity*)tmpnode->element;
					if ( tmpentity && tmpentity != postProcessEntity )
					{
						if ( tmpentity->behavior != &actMonster
							&& !tmpentity->flags[PASSABLE]
							&& tmpentity->behavior != &actFurniture )
						{
							// remove the trapdoor since we've spawned over a gate, chest, door etc.
							list_RemoveNode(postProcessEntity->mynode);
							break;
						}
					}
				}
			}
			else if ( postProcessEntity->sprite == 586 || postProcessEntity->sprite == 585
				|| postProcessEntity->sprite == 184 || postProcessEntity->sprite == 185 )
			{
				int findx = static_cast<int>(postProcessEntity->x) >> 4;
				int findy = static_cast<int>(postProcessEntity->y) >> 4;
				if ( !map->tiles[findy * MAPLAYERS + findx * MAPLAYERS * map->height] )
				{
					// remove the lever as it is over a pit.
					printlog("[MAP GENERATOR] Removed switch over a pit at x:%d y:%d.", findx, findy);
					list_RemoveNode(postProcessEntity->mynode);
				}
			}
		}
	}
	if ( vampireQuestChest )
	{
		for ( int c = 0; c < MAXPLAYERS; ++c )
		{
			if ( client_classes[c] == CLASS_ACCURSED )
			{
				vampireQuestChest->chestHasVampireBook = 1;
				break;
			}
		}
	}

	std::vector<Entity*> chests;
	for ( auto node = map->entities->first; node != nullptr; )
	{
		Entity* postProcessEntity = (Entity*)node->element;
		node = node->next;
		if ( postProcessEntity )
		{
			if ( postProcessEntity->behavior == &actTextSource )
			{
				textSourceScript.parseScriptInMapGeneration(*postProcessEntity);
			}
			if ( postProcessEntity->behavior == &actChest )
			{
				chests.push_back(postProcessEntity);
			}
		}
	}

	debugMap(map);

	if ( true /*currentlevel == 0*/ )
	{
		numChests = 0;
		numMimics = 0;
	}

	static ConsoleVariable<int> cvar_mimic_chance("/mimic_chance", 10);
	static ConsoleVariable<bool> cvar_mimic_debug("/mimic_debug", false);

	std::vector<Entity*> mimics;
	if ( chests.size() > 0 )
	{
		if ( mimic_generator.bForceSpawnForCurrentFloor() )
		{
			auto chosen = map_rng.rand() % chests.size();
			if ( allowedGenerateMimicOnChest(chests[chosen]->x / 16, chests[chosen]->y / 16, *map) )
			{
				if ( chests[chosen]->chestMimicChance != 0 )
				{
					mimics.push_back(chests[chosen]);
					chests.erase(chests.begin() + chosen);
				}
			}
		}

		for ( auto it = chests.begin(); it != chests.end(); )
		{
			bool doMimic = false;
			Entity* chest = *it;
			if ( allowedGenerateMimicOnChest(chest->x / 16, chest->y / 16, *map) )
			{
				int chance = 10;
				if ( svFlags & SV_FLAG_CHEATS )
				{
					chance = std::min(100, std::max(0, *cvar_mimic_chance));
				}
				if ( chest->chestMimicChance >= 0 )
				{
					doMimic = chest->entity_rng->rand() % 100 < chest->chestMimicChance;
				}
				else
				{
					doMimic = chest->entity_rng->rand() % 100 < chance;
				}
			}

			if ( doMimic )
			{
				mimics.push_back(chest);
				it = chests.erase(it);
			}
			else
			{
				createChestInventory(chest, chest->chestType);
				++numChests;
				++it;
			}
		}
	}

	if ( *cvar_mimic_debug && (svFlags & SV_FLAG_CHEATS) )
	{
		messagePlayer(clientnum, MESSAGE_INSPECTION, "Mimics: [%d]", mimics.size());
	}

	for ( auto chest : mimics )
	{
		if ( vampireQuestChest && chest == vampireQuestChest )
		{
			createChestInventory(chest, chest->chestType);
			continue;
		}

		// mimic
		numMimics++;
		Entity* entity = newEntity(10, 1, map->entities, map->creatures);
		entity->sizex = 4;
		entity->sizey = 4;
		entity->x = chest->x;
		entity->y = chest->y;
		entity->z = 6;
		entity->yaw = chest->yaw;
		entity->behavior = &actMonster;
		entity->flags[UPDATENEEDED] = true;
		entity->flags[INVISIBLE] = true;
		entity->skill[5] = -1;
		//Assign entity creature list pointer.
		entity->addToCreatureList(map->creatures);

		Monster monsterType = MIMIC;
		entity->monsterLookDir = entity->yaw;

		bool monsterIsFixedSprite = true;
		Stat* myStats = nullptr;
		if ( multiplayer != CLIENT )
		{
			if ( myStats == nullptr )
			{
				// need to give the entity its list stuff.
				// create an empty first node for traversal purposes
				node_t* node2 = list_AddNodeFirst(&entity->children);
				node2->element = nullptr;
				node2->deconstructor = &emptyDeconstructor;

				// Create the stat struct again for the new monster
				myStats = new Stat(monsterType + 1000);
				myStats->type = monsterType;

				node2 = list_AddNodeLast(&entity->children);
				node2->element = myStats;
				node2->deconstructor = &statDeconstructor;
				node2->size = sizeof(myStats);
			}

		}

		Uint32 chestseed = 0;
		chest->entity_rng->getSeed(&chestseed, sizeof(chestseed));
		entity->seedEntityRNG(chestseed);
		createChestInventory(entity, chest->chestType);

		// remove chest entities
		Entity* parentEntity = uidToEntity(chest->parent);
		if ( parentEntity )
		{
			list_RemoveNode(parentEntity->mynode);    // remove lid
		}
		list_RemoveNode(chest->mynode);
	}

	if ( monsterCurveCustomManager.inUse() )
	{
		monsterCurveCustomManager.generateFollowersForLeaders();
	}

    keepInventoryGlobal = svFlags & SV_FLAG_KEEPINVENTORY;
}

void mapLevel(int player)
{
	int x, y;
	for ( y = 0; y < map.height; ++y )
	{
		for ( x = 0; x < map.width; ++x )
		{
			if ( map.tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map.height] )
			{
				if ( !minimap[y][x] )
				{
					minimap[y][x] = 4;
				}
			}
			else if ( map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height] )
			{
				if ( !minimap[y][x] )
				{
					minimap[y][x] = 3;
				}
			}
			else
			{
				minimap[y][x] = 0;
			}
		}
	}
}

void mapLevel2(int player)
{
	for ( int y = 0; y < map.height; ++y )
	{
		for ( int x = 0; x < map.width; ++x )
		{
			if ( map.tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map.height] )
			{
				if ( !minimap[y][x] )
				{
					minimap[y][x] = 2;
				}
			}
			else if ( map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height] )
			{
				if ( !minimap[y][x] )
				{
					minimap[y][x] = 1;
				}
			}
			else
			{
				minimap[y][x] = 0;
			}
		}
	}
}

void mapFoodOnLevel(int player)
{
	int numFood = 0;
	bool previouslyIdentifiedFood = false;
	for ( node_t* node = map.entities->first; node != nullptr; node = node->next )
	{
		Entity* entity = (Entity*)node->element;
		if ( entity && entity->behavior == &actItem )
		{
			Item* item = newItemFromEntity(entity);
			if ( item )
			{
				if ( itemCategory(item) == FOOD )
				{
					if ( entity->itemShowOnMap != 0 )
					{
						previouslyIdentifiedFood = true;
					}
					else
					{
						++numFood;
					}
					entity->itemShowOnMap = 1;
				}
				free(item);
			}
		}
	}
	if ( numFood == 0 && previouslyIdentifiedFood )
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(3425));
	}
	else if ( numFood == 0 )
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(3423));
	}
	else
	{
		messagePlayerColor(player, MESSAGE_HINT, makeColorRGB(0, 255, 0),Language::get(3424));
	}
}

int loadMainMenuMap(bool blessedAdditionMaps, bool forceVictoryMap, int forcemap)
{
	bool foundVictory = false;
	for ( node_t* node = topscores.first; node != nullptr && !foundVictory; node = node->next )
	{
		score_t* score = (score_t*)node->element;
		if ( score && (score->victory == 3 || score->victory == 4 || score->victory == 5) )
		{
			foundVictory = true;
		}
	}
	for ( node_t* node = topscoresMultiplayer.first; node != nullptr && !foundVictory; node = node->next )
	{
		score_t* score = (score_t*)node->element;
		if ( score && (score->victory == 3 || score->victory == 4 || score->victory == 5) )
		{
			foundVictory = true;
		}
	}

	std::string fullMapName;

	int selection = forcemap;
	if (selection <= 0) {
		if ( forceVictoryMap || (foundVictory && local_rng.rand() % 5 == 0) )
		{
			selection = 9;
		}
		else if ( blessedAdditionMaps )
		{
			selection = 5 + local_rng.rand() % 4;
		}
		else
		{
			selection = 1 + local_rng.rand() % 4;
		}
	}

	switch ( selection )
	{
	case 1:
		fullMapName = physfsFormatMapName("mainmenu1");
		loadMap(fullMapName.c_str(), &map, map.entities, map.creatures);
		menucam.x = 8;
		menucam.y = 4.5;
		menucam.z = 0;
		menucam.ang = 0.6;
		return 0;
	case 2:
		fullMapName = physfsFormatMapName("mainmenu2");
		loadMap(fullMapName.c_str(), &map, map.entities, map.creatures);
		menucam.x = 7;
		menucam.y = 4;
		menucam.z = -4;
		menucam.ang = 1.0;
		return 0;
	case 3:
		fullMapName = physfsFormatMapName("mainmenu3");
		loadMap(fullMapName.c_str(), &map, map.entities, map.creatures);
		menucam.x = 5;
		menucam.y = 3;
		menucam.z = 0;
		menucam.ang = 1.0;
		return 0;
	case 4:
		fullMapName = physfsFormatMapName("mainmenu4");
		loadMap(fullMapName.c_str(), &map, map.entities, map.creatures);
		menucam.x = 6;
		menucam.y = 14.5;
		menucam.z = -24;
		menucam.ang = 5.0;
		return 0;
	case 5:
		fullMapName = physfsFormatMapName("mainmenu5");
		loadMap(fullMapName.c_str(), &map, map.entities, map.creatures);
		menucam.x = 12.1;
		menucam.y = 5.8;
		menucam.z = 0;
		menucam.ang = 2.72;
		return 0;
	case 6:
		fullMapName = physfsFormatMapName("mainmenu6");
		loadMap(fullMapName.c_str(), &map, map.entities, map.creatures);
		menucam.x = 11;
		menucam.y = 4;
		menucam.z = 0;
		menucam.ang = 2.4;
		return 0;
	case 7:
		fullMapName = physfsFormatMapName("mainmenu7");
		loadMap(fullMapName.c_str(), &map, map.entities, map.creatures);
		menucam.x = 8.7;
		menucam.y = 9.3;
		menucam.z = 0;
		menucam.ang = 5.8;
		return 0;
	case 8:
		fullMapName = physfsFormatMapName("mainmenu8");
		loadMap(fullMapName.c_str(), &map, map.entities, map.creatures);
		menucam.x = 3.31;
		menucam.y = 5.34;
		menucam.z = 0;
		menucam.ang = 0.96;
		return 0;
	case 9:
		fullMapName = physfsFormatMapName("mainmenu9");
		loadMap(fullMapName.c_str(), &map, map.entities, map.creatures);
		menucam.x = 34.3;
		menucam.y = 15;
		menucam.z = -20;
		menucam.ang = 5.84;
		return 1;
	default:
		assert(0 && "selected invalid main menu map");
		return -1;
	}
}