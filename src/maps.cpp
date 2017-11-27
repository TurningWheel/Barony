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

int startfloor = 0;

/*-------------------------------------------------------------------------------

	monsterCurve

	selects a monster randomly, taking into account the region the monster
	is being spawned in

-------------------------------------------------------------------------------*/

int monsterCurve(int level)
{
	if ( !strncmp(map.name, "The Mines", 9) )   // the mines
	{
		switch ( rand() % 10 )
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
		switch ( rand() % 10 )
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
		switch ( rand() % 10 )
		{
			case 0:
			case 1:
				return GOBLIN;
			case 2:
			case 3:
			case 4:
			case 5:
				return SCORPION;
			case 6:
			case 7:
			case 8:
			case 9:
				return TROLL;
		}
	}
	else if ( !strncmp(map.name, "The Ruins", 9) )     // blue ruins
	{
		switch ( rand() % 10 )
		{
			case 0:
				return GOBLIN;
			case 1:
			case 2:
			case 3:
				return GNOME;
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
				return TROLL;
			case 9:
				return DEMON;
		}
	}
	else if ( !strncmp(map.name, "Underworld", 10) )     // underworld
	{
		switch ( rand() % 10 )
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
		switch ( rand() % 10 )
		{
			case 0:
			case 1:
				return SUCCUBUS;
			case 2:
			case 3:
				if ( strstr(map.name, "Boss") )
				{
					return DEMON;    // we would otherwise lag bomb on the boss level
				}
				else
				{
					return CREATURE_IMP;
				}
			case 4:
			case 5:
				return SHADOW;
			case 6:
			case 7:
			case 8:
			case 9:
				return DEMON;
		}
	}
	else if ( !strncmp(map.name, "Caves", 5) )
	{
		switch ( rand() % 20 )
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
			case 8:
			case 9:
				return AUTOMATON;
			case 10:
			case 11:
				return CRYSTALGOLEM;
			case 12:
			case 13:
			case 14:
				return INSECTOID;
			case 15:
			case 16:
			case 17:
				return GOATMAN;
			case 18:
				return INCUBUS;
			case 19:
				return COCKATRICE;
		}
	}
	else if ( !strncmp(map.name, "Citadel", 7) )
	{
		switch ( rand() % 20 )
		{
			case 0:
			case 1:
				return KOBOLD;
			case 2:
			case 3:
				return SCARAB;
			case 4:
			case 5:
			case 6:
				return CRYSTALGOLEM;
			case 7:
				return VAMPIRE;
			case 8:
			case 9:
			case 10:
				return SHADOW;
			case 11:
			case 12:
			case 13:
			case 14:
				return AUTOMATON;
			case 15:
			case 16:
			case 17:
				return GOATMAN;
			case 18:
			case 19:
				return COCKATRICE;
		}
	}
	return SKELETON; // basic monster
}

/*-------------------------------------------------------------------------------

	generateDungeon

	generates a level by drawing data from numerous files and connecting
	their rooms together with tunnels.

-------------------------------------------------------------------------------*/

int generateDungeon(char* levelset, Uint32 seed)
{
	char* sublevelname, *fullname, *subRoomName;
	char sublevelnum[3];
	map_t* tempMap, *subRoomMap;
	list_t mapList, *newList, *subRoomList, subRoomMapList;
	node_t* node, *node2, *node3, *nextnode, *subRoomNode;
	Sint32 c, i, j;
	Sint32 numlevels, levelnum, levelnum2, subRoomNumLevels;
	Sint32 x, y, z;
	Sint32 x0, y0, x1, y1;
	door_t* door, *newDoor;
	bool* possiblelocations, *possiblelocations2, *possiblerooms;
	bool* firstroomtile;
	Sint32 numpossiblelocations, pickedlocation, subroomPickRoom;
	Entity* entity, *entity2, *childEntity;
	Uint32 levellimit;
	list_t doorList;
	node_t* doorNode, *subRoomDoorNode;
	bool shoplevel = false;
	map_t shopmap;
	map_t secretlevelmap;
	int secretlevelexit = 0;
	bool *trapexcludelocations;
	bool *monsterexcludelocations;
	bool *lootexcludelocations;

	printlog("generating a dungeon from level set '%s' (seed %d)...\n", levelset, seed);
	if (loadMap(levelset, &map, map.entities) == -1)
	{
		printlog("error: no level of set '%s' could be found.\n", levelset);
		return -1;
	}

	// store this map's seed
	mapseed = seed;
	prng_seed_bytes(&mapseed, sizeof(mapseed));

	// determine whether shop level or not
	if ( prng_get_uint() % 2 && currentlevel > 1 && strncmp(map.name, "Underworld", 10) && strncmp(map.name, "Hell", 4) )
	{
		shoplevel = true;
	}

	// determine whether minotaur level or not
	if ( currentlevel % LENGTH_OF_LEVEL_REGION == 2 || currentlevel % LENGTH_OF_LEVEL_REGION == 3 )
		if ( prng_get_uint() % 2 && (svFlags & SV_FLAG_MINOTAURS) )
		{
			minotaurlevel = 1;
		}

	// dark level
	if ( !secretlevel && currentlevel % LENGTH_OF_LEVEL_REGION >= 2 )
	{
		if ( prng_get_uint() % 4 == 0 )
		{
			darkmap = true;
			messagePlayer(clientnum, language[1108]);
		}
	}

	// secret stuff
	if ( !secretlevel )
	{
		if ( (currentlevel == 3 && prng_get_uint() % 2) || currentlevel == 2 )
		{
			secretlevelexit = 1;
		}
		else if ( (currentlevel == 7 && prng_get_uint() % 2) || currentlevel == 8 )
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
	}

	mapList.first = NULL;
	mapList.last = NULL;
	doorList.first = NULL;
	doorList.last = NULL;

	// load shop room
	if ( shoplevel )
	{
		sublevelname = (char*) malloc(sizeof(char) * 128);
		fullname = (char*) malloc(sizeof(char) * 128);
		for ( numlevels = 0; numlevels < 100; numlevels++ )
		{
			strcpy(sublevelname, "shop");
			snprintf(sublevelnum, 3, "%02d", numlevels);
			strcat(sublevelname, sublevelnum);
			snprintf(fullname, strlen(levelset) + 13, "maps/%s.lmp", sublevelname);
			if ( !dataPathExists(fullname) )
			{
				break;    // no more levels to load
			}
		}
		if ( numlevels )
		{
			int shopleveltouse = prng_get_uint() % numlevels;
			strcpy(sublevelname, "shop");
			snprintf(sublevelnum, 3, "%02d", shopleveltouse);
			strcat(sublevelname, sublevelnum);

			shopmap.tiles = NULL;
			shopmap.entities = (list_t*) malloc(sizeof(list_t));
			shopmap.entities->first = NULL;
			shopmap.entities->last = NULL;
			if ( loadMap(sublevelname, &shopmap, shopmap.entities) == -1 )
			{
				list_FreeAll(shopmap.entities);
				free(shopmap.entities);
				if ( shopmap.tiles )
				{
					free(shopmap.tiles);
				}
			}
		}
		else
		{
			shoplevel = false;
		}
		free( sublevelname );
		free( fullname );

	}

	sublevelname = (char*) malloc(sizeof(char) * 128);
	fullname = (char*) malloc(sizeof(char) * 128);

	// a maximum of 100 (0-99 inclusive) sublevels can be added to the pool
	for ( numlevels = 0; numlevels < 100; numlevels++ )
	{
		strcpy(sublevelname, levelset);
		snprintf(sublevelnum, 3, "%02d", numlevels);
		strcat(sublevelname, sublevelnum);

		// check if there is another sublevel to load
		snprintf(fullname, strlen(levelset) + 13, "maps/%s.lmp", sublevelname);
		if ( !dataPathExists(fullname) )
		{
			break;    // no more levels to load
		}

		// allocate memory for the next sublevel and attempt to load it
		tempMap = (map_t*) malloc(sizeof(map_t));
		tempMap->tiles = NULL;
		tempMap->entities = (list_t*) malloc(sizeof(list_t));
		tempMap->entities->first = NULL;
		tempMap->entities->last = NULL;
		if ( loadMap(sublevelname, tempMap, tempMap->entities) == -1 )
		{
			mapDeconstructor((void*)tempMap);
			continue; // failed to load level
		}

		// level is successfully loaded, add it to the pool
		newList = (list_t*) malloc(sizeof(list_t));
		newList->first = NULL;
		newList->last = NULL;
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
							door->dir = 0;
						}
						else if ( y == tempMap->height - 1 )
						{
							door->dir = 1;
						}
						else if ( x == 0 )
						{
							door->dir = 2;
						}
						else if ( y == 0 )
						{
							door->dir = 3;
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
	subRoomMapList.first = NULL;
	subRoomMapList.last = NULL;
	char letterString[2];
	letterString[1] = '\0';
	int subroomCount[100] = {0};

	// a maximum of 100 (0-99 inclusive) sublevels can be added to the pool
	for ( subRoomNumLevels = 0; subRoomNumLevels <= numlevels; subRoomNumLevels++ )
	{
		for ( char letter = 'a'; letter <= 'z'; letter++ )
		{
			// look for mapnames ending in a letter a to z
			strcpy(subRoomName, levelset);
			snprintf(sublevelnum, 3, "%02d", subRoomNumLevels);
			letterString[0] = letter;
			strcat(subRoomName, sublevelnum);
			strcat(subRoomName, letterString);

			// check if there is another subroom to load
			snprintf(fullname, strlen(levelset) + 14, "maps/%s.lmp", subRoomName);
			if ( access(fullname, F_OK) == -1 )
			{
				break;    // no more levels to load
			}

			printlog("Found map lv %s, count: %d", subRoomName, subroomCount[subRoomNumLevels]);
			subroomCount[subRoomNumLevels]++;

			// allocate memory for the next subroom and attempt to load it
			subRoomMap = (map_t*)malloc(sizeof(map_t));
			subRoomMap->tiles = NULL;
			subRoomMap->entities = (list_t*)malloc(sizeof(list_t));
			subRoomMap->entities->first = NULL;
			subRoomMap->entities->last = NULL;
			if ( loadMap(subRoomName, subRoomMap, subRoomMap->entities) == -1 )
			{
				mapDeconstructor((void*)subRoomMap);
				continue; // failed to load level
			}

			// level is successfully loaded, add it to the pool
			subRoomList = (list_t*)malloc(sizeof(list_t));
			subRoomList->first = NULL;
			subRoomList->last = NULL;
			node = list_AddNodeLast(&subRoomMapList);
			node->element = subRoomList;
			node->deconstructor = &listDeconstructor;

			node = list_AddNodeLast(subRoomList);
			node->element = subRoomMap;
			node->deconstructor = &mapDeconstructor;

			// more nodes are created to record the exit points on the sublevel
			/*for ( y = 0; y < subRoomMap->height; y++ )
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
								door->dir = 0;
							}
							else if ( y == subRoomMap->height - 1 )
							{
								door->dir = 1;
							}
							else if ( x == 0 )
							{
								door->dir = 2;
							}
							else if ( y == 0 )
							{
								door->dir = 3;
							}
							node2 = list_AddNodeLast(subRoomList);
							node2->element = door;
							node2->deconstructor = &defaultDeconstructor;
						}
					}
				}
			}*/
		}
	}

	// generate dungeon level...
	int roomcount = 0;
	if ( numlevels > 1 )
	{
		possiblelocations = (bool*) malloc(sizeof(bool) * map.width * map.height);
		trapexcludelocations = (bool*)malloc(sizeof(bool) * map.width * map.height);
		monsterexcludelocations = (bool*)malloc(sizeof(bool) * map.width * map.height);
		lootexcludelocations = (bool*)malloc(sizeof(bool) * map.width * map.height);
		for ( y = 0; y < map.height; y++ )
		{
			for ( x = 0; x < map.width; x++ )
			{
				if ( x < 2 || y < 2 || x > map.width - 3 || y > map.height - 3 )
				{
					possiblelocations[x + y * map.width] = false;
				}
				else
				{
					possiblelocations[x + y * map.width] = true;
				}
				trapexcludelocations[x + y * map.width] = false;
				if ( map.flags[MAP_FLAG_DISABLEMONSTERS] == true )
				{
					// the base map excludes all monsters
					monsterexcludelocations[x + y * map.width] = true;
				}
				else
				{
					monsterexcludelocations[x + y * map.width] = false;
				}
				if ( map.flags[MAP_FLAG_DISABLELOOT] == true )
				{
					// the base map excludes all monsters
					lootexcludelocations[x + y * map.width] = true;
				}
				else
				{
					lootexcludelocations[x + y * map.width] = false;
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
				for ( x = 0; x < map.width; x++ )
				{
					possiblelocations2[x + y * map.width] = true;
				}

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
				secretlevelmap.tiles = NULL;
				secretlevelmap.entities = (list_t*) malloc(sizeof(list_t));
				secretlevelmap.entities->first = NULL;
				secretlevelmap.entities->last = NULL;
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
					default:
						break;
				}
				if ( loadMap(secretmapname, &secretlevelmap, secretlevelmap.entities) == -1 )
				{
					list_FreeAll(secretlevelmap.entities);
					free(secretlevelmap.entities);
					if ( secretlevelmap.tiles )
					{
						free(secretlevelmap.tiles);
					}
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
				levelnum = prng_get_uint() % (numlevels); // draw randomly from the pool

				// traverse the map list to the picked level
				node = mapList.first;
				i = 0;
				j = -1;
				while (1)
				{
					if (possiblerooms[i])
					{
						j++;
						if (j == levelnum)
						{
							break;
						}
					}
					node = node->next;
					i++;
				}
				levelnum2 = i;
				node = ((list_t*)node->element)->first;
				doorNode = node->next;
				tempMap = (map_t*)node->element;
			}

			// find locations where the selected room can be added to the level
			numpossiblelocations = map.width * map.height;
			for ( y0 = 0; y0 < map.height; y0++ )
			{
				for ( x0 = 0; x0 < map.width; x0++ )
				{
					for ( y1 = y0; y1 < std::min(y0 + tempMap->height, map.height); y1++ )
					{
						for ( x1 = x0; x1 < std::min(x0 + tempMap->width, map.width); x1++ )
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
				numlevels--;
				if ( levelnum2 == 0 )
				{
					// if we couldn't even fit the entrance hall into the dungeon, we have a problem
					free(possiblerooms);
					free(possiblelocations);
					free(possiblelocations2);
					free(trapexcludelocations);
					free(monsterexcludelocations);
					free(lootexcludelocations);
					free(firstroomtile);
					free(sublevelname);
					free(fullname);
					free(subRoomName);
					list_FreeAll(&subRoomMapList);
					list_FreeAll(&mapList);
					if ( shoplevel && c == 2 )
					{
						list_FreeAll(shopmap.entities);
						free(shopmap.entities);
						if ( shopmap.tiles )
						{
							free(shopmap.tiles);
						}
					}
					if ( secretlevelexit && c == 1 )
					{
						list_FreeAll(secretlevelmap.entities);
						free(secretlevelmap.entities);
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
				while ( 1 )
				{
					if ( possiblelocations2[x + y * map.width] == true )
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
							pickedlocation++;
						}
					}
				}
			}
			else
			{
				pickedlocation = prng_get_uint() % numpossiblelocations;
				i = -1;
				x = 0;
				y = 0;
				while ( 1 )
				{
					if ( possiblelocations2[x + y * map.width] == true )
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
			}

			// now copy all the geometry from the sublevel to the chosen location
			if ( c == 0 )
				for ( z = 0; z < map.width * map.height; z++ )
				{
					firstroomtile[z] = false;
				}
			x1 = x + tempMap->width;
			y1 = y + tempMap->height;


			//**********pick subroom if available
			int pickSubRoom = 0;
			int k = 0;
			int subRoom_tilex = 0;
			int subRoom_tiley = 0;
			int subRoom_tileStartx = -1;
			int subRoom_tileStarty = -1;
			int foundSubRoom = 0;

			if ( subroomCount[levelnum + 1] > 0 )
			{
				int jumps = 0;
				pickSubRoom = prng_get_uint() % subroomCount[levelnum + 1];
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
				k = 0;

				while ( 1 )
				{
					if ( k == pickSubRoom )
					{
						break;
					}
					subRoomNode = subRoomNode->next;
					k++;
				}
				messagePlayer(0, "%d + %d jumps!", jumps, k + 1);
				subRoomNode = ((list_t*)subRoomNode->element)->first;
				subRoomMap = (map_t*)subRoomNode->element;
				subRoomDoorNode = subRoomNode->next;
			}

			for ( z = 0; z < MAPLAYERS; z++ )
			{
				for ( y0 = y; y0 < y1; y0++ )
				{
					for ( x0 = x; x0 < x1; x0++ )
					{
						if ( subroomCount[levelnum + 1] > 0 && tempMap->tiles[z + (y0 - y) * MAPLAYERS + (x0 - x) * MAPLAYERS * tempMap->height] == 201 )
						{
							if ( !foundSubRoom )
							{
								subRoom_tileStartx = x0;
								subRoom_tileStarty = y0;
								foundSubRoom = 1;
								messagePlayer(0, "Picked level: %d from %d possible rooms in submap %d", pickSubRoom + 1, subroomCount[levelnum + 1], levelnum + 1);
							}

							map.tiles[z + y0 * MAPLAYERS + x0 * MAPLAYERS * map.height] = subRoomMap->tiles[z + (subRoom_tiley)* MAPLAYERS + (subRoom_tilex)* MAPLAYERS * subRoomMap->height];

							subRoom_tilex++;
							if ( subRoom_tilex >= subRoomMap->width )
							{
								subRoom_tilex = 0;
								subRoom_tiley++;
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
								trapexcludelocations[x0 + y0 * map.width] = true;
								map.tiles[z + y0 * MAPLAYERS + x0 * MAPLAYERS * map.height] = 83;
							}
							if ( tempMap->flags[MAP_FLAG_DISABLEMONSTERS] == 1 )
							{
								monsterexcludelocations[x0 + y0 * map.width] = true;
							}
							if ( tempMap->flags[MAP_FLAG_DISABLELOOT] == 1 )
							{
								lootexcludelocations[x0 + y0 * map.width] = true;
							}
							if ( c == 0 )
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
						for ( node = map.entities->first; node != NULL; node = nextnode )
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
			for ( node = tempMap->entities->first; node != NULL; node = node->next )
			{
				entity = (Entity*)node->element;
				childEntity = newEntity(entity->sprite, 1, map.entities);

				// entity will return nullptr on getStats called in setSpriteAttributes as behaviour &actmonster is not set.
				// check if the monster sprite is correct and set the behaviour manually for getStats.
				if ( checkSpriteType(entity->sprite) == 1 && multiplayer != CLIENT )
				{
					entity->behavior = &actMonster;
				}

				setSpriteAttributes(childEntity, entity, entity);
				childEntity->x = entity->x + x * 16;
				childEntity->y = entity->y + y * 16;
				//printlog("1 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
			}

			if ( foundSubRoom )
			{
				// copy the entities from subroom
				for ( subRoomNode = subRoomMap->entities->first; subRoomNode != NULL; subRoomNode = subRoomNode->next )
				{
					entity = (Entity*)subRoomNode->element;
					childEntity = newEntity(entity->sprite, 1, map.entities);

					// entity will return nullptr on getStats called in setSpriteAttributes as behaviour &actmonster is not set.
					// check if the monster sprite is correct and set the behaviour manually for getStats.
					if ( checkSpriteType(entity->sprite) == 1 && multiplayer != CLIENT )
					{
						entity->behavior = &actMonster;
					}

					setSpriteAttributes(childEntity, entity, entity);
					childEntity->x = entity->x + subRoom_tileStartx * 16;
					childEntity->y = entity->y + subRoom_tileStarty * 16;

					//messagePlayer(0, "1 Generated entity. Sprite: %d X: %.2f Y: %.2f", childEntity->sprite, childEntity->x / 16, childEntity->y / 16);
				}
			}

			// finally, copy the doors into a single doors list
			while (doorNode != NULL)
			{
				door = (door_t*)doorNode->element;
				newDoor = (door_t*) malloc(sizeof(door_t));
				newDoor->x = door->x + x;
				newDoor->y = door->y + y;
				newDoor->dir = door->dir;
				node = list_AddNodeLast(&doorList);
				node->element = newDoor;
				node->deconstructor = &defaultDeconstructor;
				doorNode = doorNode->next;
			}

			if ( foundSubRoom )
			{
				// copy subroom doors
				while ( subRoomDoorNode != NULL )
				{
					door = (door_t*)subRoomDoorNode->element;
					newDoor = (door_t*)malloc(sizeof(door_t));
					newDoor->x = door->x + subRoom_tileStartx;
					newDoor->y = door->y + subRoom_tileStarty;
					newDoor->dir = door->dir;
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
				if ( shopmap.tiles )
				{
					free(shopmap.tiles);
				}
			}
			if ( secretlevelexit && c == 1 )
			{
				list_FreeAll(secretlevelmap.entities);
				free(secretlevelmap.entities);
				if ( secretlevelmap.tiles )
				{
					free(secretlevelmap.tiles);
				}
			}
			roomcount++;
		}
		free(possiblerooms);
		free(possiblelocations2);
	}
	else
	{
		free(subRoomName);
		free(sublevelname);
		free(fullname);
		list_FreeAll(&subRoomMapList);
		list_FreeAll(&mapList);
		list_FreeAll(&doorList);
		printlog("error: not enough levels to begin generating dungeon.\n");
		return -1;
	}

	// post-processing:

	// doors
	for ( node = doorList.first; node != NULL; node = node->next )
	{
		door = (door_t*)node->element;
		for (node2 = map.entities->first; node2 != NULL; node2 = node2->next)
		{
			entity = (Entity*)node2->element;
			if ( entity->x / 16 == door->x && entity->y / 16 == door->y && (entity->sprite == 2 || entity->sprite == 3) )
			{
				switch ( door->dir )
				{
					case 0: // east
						map.tiles[OBSTACLELAYER + door->y * MAPLAYERS + (door->x + 1)*MAPLAYERS * map.height] = 0;
						for ( node3 = map.entities->first; node3 != NULL; node3 = nextnode )
						{
							entity = (Entity*)node3->element;
							nextnode = node3->next;
							if ( entity->sprite == 2 || entity->sprite == 3 )
							{
								if ( (int)(entity->x / 16) == door->x + 2 && (int)(entity->y / 16) == door->y )
								{
									list_RemoveNode(entity->mynode);
									break;
								}
								else if ( (int)(entity->x / 16) == door->x + 1 && (int)(entity->y / 16) == door->y )
								{
									list_RemoveNode(entity->mynode);
									break;
								}
								else if ( (int)(entity->x / 16) == door->x + 1 && (int)(entity->y / 16) == door->y + 1 )
								{
									list_RemoveNode(entity->mynode);
									break;
								}
								else if ( (int)(entity->x / 16) == door->x + 1 && (int)(entity->y / 16) == door->y - 1 )
								{
									list_RemoveNode(entity->mynode);
									break;
								}
							}
						}
						break;
					case 1: // south
						map.tiles[OBSTACLELAYER + (door->y + 1)*MAPLAYERS + door->x * MAPLAYERS * map.height] = 0;
						for ( node3 = map.entities->first; node3 != NULL; node3 = nextnode )
						{
							entity = (Entity*)node3->element;
							nextnode = node3->next;
							if ( entity->sprite == 2 || entity->sprite == 3 )
							{
								if ( (int)(entity->x / 16) == door->x && (int)(entity->y / 16) == door->y + 2 )
								{
									list_RemoveNode(entity->mynode);
									break;
								}
								else if ( (int)(entity->x / 16) == door->x && (int)(entity->y / 16) == door->y + 1 )
								{
									list_RemoveNode(entity->mynode);
									break;
								}
								else if ( (int)(entity->x / 16) == door->x + 1 && (int)(entity->y / 16) == door->y + 1 )
								{
									list_RemoveNode(entity->mynode);
									break;
								}
								else if ( (int)(entity->x / 16) == door->x - 1 && (int)(entity->y / 16) == door->y + 1 )
								{
									list_RemoveNode(entity->mynode);
									break;
								}
							}
						}
						break;
					case 2: // west
						map.tiles[OBSTACLELAYER + door->y * MAPLAYERS + (door->x - 1)*MAPLAYERS * map.height] = 0;
						for ( node3 = map.entities->first; node3 != NULL; node3 = nextnode )
						{
							entity = (Entity*)node3->element;
							nextnode = node3->next;
							if ( entity->sprite == 2 || entity->sprite == 3 )
							{
								if ( (int)(entity->x / 16) == door->x - 2 && (int)(entity->y / 16) == door->y )
								{
									list_RemoveNode(entity->mynode);
									break;
								}
								else if ( (int)(entity->x / 16) == door->x - 1 && (int)(entity->y / 16) == door->y )
								{
									list_RemoveNode(entity->mynode);
									break;
								}
								else if ( (int)(entity->x / 16) == door->x - 1 && (int)(entity->y / 16) == door->y + 1 )
								{
									list_RemoveNode(entity->mynode);
									break;
								}
								else if ( (int)(entity->x / 16) == door->x - 1 && (int)(entity->y / 16) == door->y - 1 )
								{
									list_RemoveNode(entity->mynode);
									break;
								}
							}
						}
						break;
					case 3: // north
						map.tiles[OBSTACLELAYER + (door->y - 1)*MAPLAYERS + door->x * MAPLAYERS * map.height] = 0;
						for ( node3 = map.entities->first; node3 != NULL; node3 = nextnode )
						{
							entity = (Entity*)node3->element;
							nextnode = node3->next;
							if ( entity->sprite == 2 || entity->sprite == 3 )
							{
								if ( (int)(entity->x / 16) == door->x && (int)(entity->y / 16) == door->y - 2 )
								{
									list_RemoveNode(entity->mynode);
									break;
								}
								else if ( (int)(entity->x / 16) == door->x && (int)(entity->y / 16) == door->y - 1 )
								{
									list_RemoveNode(entity->mynode);
									break;
								}
								else if ( (int)(entity->x / 16) == door->x + 1 && (int)(entity->y / 16) == door->y - 1 )
								{
									list_RemoveNode(entity->mynode);
									break;
								}
								else if ( (int)(entity->x / 16) == door->x - 1 && (int)(entity->y / 16) == door->y - 1 )
								{
									list_RemoveNode(entity->mynode);
									break;
								}
							}
						}
						break;
				}
			}
		}
	}

	// boulder and arrow traps
	if ( (svFlags & SV_FLAG_TRAPS) && map.flags[MAP_FLAG_DISABLETRAPS] == 0 )
	{
		numpossiblelocations = 0;
		for ( c = 0; c < map.width * map.height; c++ )
		{
			possiblelocations[c] = false;
		}
		for ( y = 1; y < map.height - 1; y++ )
		{
			for ( x = 1; x < map.width - 1; x++ )
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
				if ( sides == 1 && (trapexcludelocations[x + y * map.width] == false) )
				{
					possiblelocations[y + x * map.height] = true;
					numpossiblelocations++;
				}
			}
		}

		// don't spawn traps in doors
		node_t* doorNode;
		for ( doorNode = doorList.first; doorNode != NULL; doorNode = doorNode->next )
		{
			door_t* door = (door_t*)doorNode->element;
			int x = std::min<unsigned int>(std::max(0, door->x), map.width); //TODO: Why are const int and unsigned int being compared?
			int y = std::min<unsigned int>(std::max(0, door->y), map.height); //TODO: Why are const int and unsigned int being compared?
			if ( possiblelocations[y + x * map.height] == true )
			{
				possiblelocations[y + x * map.height] = false;
				numpossiblelocations--;
			}
		}

		int whatever = prng_get_uint() % 5;
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
		for ( c = 0; c < j; c++ )
		{
			// choose a random location from those available
			pickedlocation = prng_get_uint() % numpossiblelocations;
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
				if ( prng_get_uint() % 2 && (currentlevel > 5 && currentlevel <= 25) )
				{
					arrowtrapspawn = true;
				}
			}
			if ( arrowtrapspawn || noceiling )
			{
				arrowtrap = true;
				entity = newEntity(32, 1, map.entities); // arrow trap
				entity->behavior = &actArrowTrap;
				map.tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map.height] = 53; // trap wall
			}
			else
			{
				//messagePlayer(0, "Included at x: %d, y: %d", x, y);
				entity = newEntity(38, 1, map.entities); // boulder trap
				entity->behavior = &actBoulderTrap;
			}
			entity->x = x * 16;
			entity->y = y * 16;
			//printlog("2 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",entity->sprite,entity->getUID(),entity->x,entity->y);
			entity = newEntity(18, 1, map.entities); // electricity node
			entity->x = x * 16 - (side == 3) * 16 + (side == 1) * 16;
			entity->y = y * 16 - (side == 0) * 16 + (side == 2) * 16;
			//printlog("4 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",entity->sprite,entity->getUID(),entity->x,entity->y);
			// make torches
			if ( arrowtrap )
			{
				entity = newEntity(4 + side, 1, map.entities);
				Entity* entity2 = newEntity(4 + side, 1, map.entities);
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
					for ( tempNode = map.entities->first; tempNode != NULL; tempNode = nextTempNode )
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
				entity = newEntity(34, 1, map.entities); // pressure plate
				entity->x = x * 16;
				entity->y = y * 16;
				//printlog("7 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",entity->sprite,entity->getUID(),entity->x,entity->y);
				entity = newEntity(18, 1, map.entities); // electricity node
				entity->x = x * 16 - (side == 3) * 16 + (side == 1) * 16;
				entity->y = y * 16 - (side == 0) * 16 + (side == 2) * 16;
				//printlog("8 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",entity->sprite,entity->getUID(),entity->x,entity->y);
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
				i++;
				testx = std::min(std::max<unsigned int>(0, x), map.width - 1); //TODO: Why are const int and unsigned int being compared?
				testy = std::min(std::max<unsigned int>(0, y), map.height - 1); //TODO: Why are const int and unsigned int being compared?
			}
			while ( !map.tiles[OBSTACLELAYER + testy * MAPLAYERS + testx * MAPLAYERS * map.height] && i <= 10 );
		}
	}

	// monsters, decorations, and items
	numpossiblelocations = map.width * map.height;
	for ( y = 0; y < map.height; y++ )
	{
		for ( x = 0; x < map.width; x++ )
		{
			if ( checkObstacle( x * 16 + 8, y * 16 + 8, NULL, NULL ) || firstroomtile[y + x * map.height] )
			{
				possiblelocations[y + x * map.height] = false;
				numpossiblelocations--;
			}
			else if ( lavatiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] )
			{
				possiblelocations[y + x * map.height] = false;
				numpossiblelocations--;
			}
			else if ( swimmingtiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] )
			{
				possiblelocations[y + x * map.height] = false;
				numpossiblelocations--;
			}
			else
			{
				possiblelocations[y + x * map.height] = true;
			}
		}
	}
	for ( node = map.entities->first; node != NULL; node = node->next )
	{
		entity = (Entity*)node->element;
		x = entity->x / 16;
		y = entity->y / 16;
		if ( x >= 0 && x < map.width && y >= 0 && y < map.height )
		{
			if ( possiblelocations[y + x * map.height] )
			{
				possiblelocations[y + x * map.height] = false;
				numpossiblelocations--;
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
		genEntityMin = std::max(genEntityMin, 2); // make sure there's room for a ladder.

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
		entitiesToGenerate = genEntityMin;
		randomEntities = std::max(genEntityMax - genEntityMin, 1); // difference between min and max is the extra chances.
		//Needs to be 1 for prng_get_uint() % to not divide by 0.
		j = std::min<Uint32>(entitiesToGenerate + prng_get_uint() % randomEntities, numpossiblelocations); //TODO: Why are Uint32 and Sin32 being compared?
	}
	else
	{
		// revert to old mechanics.
		j = std::min<Uint32>(30 + prng_get_uint() % 10, numpossiblelocations); //TODO: Why are Uint32 and Sin32 being compared?
	}

	int forcedMonsterSpawns = 0;
	int forcedLootSpawns = 0;
	int forcedDecorationSpawns = 0;

	if ( genMonsterMin > 0 || genMonsterMax > 0 )
	{
		forcedMonsterSpawns = genMonsterMin + prng_get_uint() % std::max(genMonsterMax - genMonsterMin, 1);
	}
	if ( genLootMin > 0 || genLootMax > 0 )
	{
		forcedLootSpawns = genLootMin + prng_get_uint() % std::max(genLootMax - genLootMin, 1);
	}
	if ( genDecorationMin > 0 || genDecorationMax > 0 )
	{
		forcedDecorationSpawns = genDecorationMin + prng_get_uint() % std::max(genDecorationMax - genDecorationMin, 1);
	}

	messagePlayer(0, "Num locations: %d of %d possible, force monsters: %d, force loot: %d, force decorations: %d", j, numpossiblelocations, forcedMonsterSpawns, forcedLootSpawns, forcedDecorationSpawns);
	printlog("Num locations: %d of %d possible, force monsters: %d, force loot: %d, force decorations: %d", j, numpossiblelocations, forcedMonsterSpawns, forcedLootSpawns, forcedDecorationSpawns);
	int numGenItems = 0;
	int numGenGold = 0;
	int numGenDecorations = 0;

	//printlog("j: %d\n",j);
	//printlog("numpossiblelocations: %d\n",numpossiblelocations);
	for ( c = 0; c < std::min(j, numpossiblelocations); c++ )
	{
		// choose a random location from those available
		pickedlocation = prng_get_uint() % numpossiblelocations;
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

		// create entity
		entity = NULL;
		if ( (c == 0 || (minotaurlevel && c < 2)) && (!secretlevel || currentlevel != 7) && (!secretlevel || currentlevel != 20) )
		{
			if ( strcmp(map.name, "Hell") )
			{
				entity = newEntity(11, 1, map.entities); // ladder
				entity->behavior = &actLadder;
			}
			else
			{
				entity = newEntity(45, 1, map.entities); // hell uses portals instead
				entity->behavior = &actPortal;
				entity->skill[3] = 1; // not secret portals though
			}

			// determine if the ladder generated in a viable location
			if ( strncmp(map.name, "Underworld", 10) )
			{
				bool nopath = false;
				bool hellLadderFix = !strncmp(map.name, "Hell", 4);
				/*if ( !hellLadderFix )
				{
					hellLadderFix = !strncmp(map.name, "Caves", 4);
				}*/
				for ( node = map.entities->first; node != NULL; node = node->next )
				{
					entity2 = (Entity*)node->element;
					if ( entity2->sprite == 1 )
					{
						list_t* path = generatePath(x, y, entity2->x / 16, entity2->y / 16, entity, entity2, hellLadderFix);
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
				if ( nopath )
				{
					// try again
					c--;
					list_RemoveNode(entity->mynode);
					entity = NULL;
				}
			}
		}
		else if ( c == 1 && secretlevel && currentlevel == 7 )
		{
			entity = newEntity(68, 1, map.entities); // magic (artifact) bow
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
					if ( checkObstacle((x + x2) * 16, (y + y2) * 16, NULL, NULL) )
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
					if ( monsterexcludelocations[x + y * map.width] == false )
					{
						if ( prng_get_uint() % 10 == 0 && currentlevel > 1 )
						{
							entity = newEntity(27, 1, map.entities);  // human
						}
						else
						{
							entity = newEntity(10, 1, map.entities);  // monster
						}
						entity->skill[5] = nummonsters;
						nummonsters++;
					}
				}
				else if ( forcedLootSpawns > 0 )
				{
					--forcedLootSpawns;
					if ( lootexcludelocations[x + y * map.width] == false )
					{
						if ( prng_get_uint() % 10 == 0 )   // 10% chance
						{
							entity = newEntity(9, 1, map.entities);  // gold
							numGenGold++;
						}
						else
						{
							entity = newEntity(8, 1, map.entities);  // item
							numGenItems++;
						}
					}
				}
				else if ( forcedDecorationSpawns > 0 && !nodecoration )
				{
					--forcedDecorationSpawns;
					// decorations
					if ( (prng_get_uint() % 4 == 0 || currentlevel <= 10) && strcmp(map.name, "Hell") )
					{
						switch ( prng_get_uint() % 7 )
						{
							case 0:
								entity = newEntity(12, 1, map.entities);
								break; //Firecamp
							case 1:
								entity = newEntity(14, 1, map.entities);
								break; //Fountain
							case 2:
								entity = newEntity(15, 1, map.entities);
								break; //Sink
							case 3:
								entity = newEntity(21, 1, map.entities);
								entity->chestLocked = -1;
								break; //Chest
							case 4:
								entity = newEntity(39, 1, map.entities);
								break; //Tomb
							case 5:
								entity = newEntity(59, 1, map.entities);
								break; //Table
							case 6:
								entity = newEntity(60, 1, map.entities);
								break; //Chair
						}
					}
					else
					{
						if ( currentlevel <= 25 )
						{
							entity = newEntity(64, 1, map.entities); // spear trap
						}
						else
						{
							if ( prng_get_uint() % 2 == 0 )
							{
								entity = newEntity(120, 1, map.entities); // vertical spell trap.
							}
							else
							{
								entity = newEntity(64, 1, map.entities); // spear trap
							}
						}
						Entity* also = newEntity(33, 1, map.entities);
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
				if ( prng_get_uint() % 2 || nodecoration )
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
						if ( prng_get_uint() % balance )
						{
							if ( lootexcludelocations[x + y * map.width] == false )
							{
								if ( prng_get_uint() % 10 == 0 )   // 10% chance
								{
									entity = newEntity(9, 1, map.entities);  // gold
									numGenGold++;
								}
								else
								{
									entity = newEntity(8, 1, map.entities);  // item
									numGenItems++;
								}
							}
						}
						else
						{
							if ( monsterexcludelocations[x + y * map.width] == false )
							{
								if ( prng_get_uint() % 10 == 0 && currentlevel > 1 )
								{
									entity = newEntity(27, 1, map.entities);  // human
								}
								else
								{
									entity = newEntity(10, 1, map.entities);  // monster
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
					if ( (prng_get_uint() % 4 == 0 || currentlevel <= 10) && strcmp(map.name, "Hell") )
					{
						switch ( prng_get_uint() % 7 )
						{
							case 0:
								entity = newEntity(12, 1, map.entities);
								break; //Firecamp
							case 1:
								entity = newEntity(14, 1, map.entities);
								break; //Fountain
							case 2:
								entity = newEntity(15, 1, map.entities);
								break; //Sink
							case 3:
								entity = newEntity(21, 1, map.entities);
								entity->chestLocked = -1;
								break; //Chest
							case 4:
								entity = newEntity(39, 1, map.entities);
								break; //Tomb
							case 5:
								entity = newEntity(59, 1, map.entities);
								break; //Table
							case 6:
								entity = newEntity(60, 1, map.entities);
								break; //Chair
						}
					}
					else
					{
						if ( currentlevel <= 25 )
						{
							entity = newEntity(64, 1, map.entities); // spear trap
						}
						else
						{
							if ( prng_get_uint() % 2 == 0 )
							{
								entity = newEntity(120, 1, map.entities); // vertical spell trap.
							}
							else
							{
								entity = newEntity(64, 1, map.entities); // spear trap
							}
						}
						Entity* also = newEntity(33, 1, map.entities);
						also->x = x * 16;
						also->y = y * 16;
						//printlog("15 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",also->sprite,also->getUID(),also->x,also->y);
					}
					numGenDecorations++;
				}
			}
		}
		if ( entity != NULL )
		{
			entity->x = x * 16;
			entity->y = y * 16;
			//printlog("9 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",entity->sprite,entity->getUID(),entity->x,entity->y);
		}
		// mark this location as inelligible for reselection
		possiblelocations[y + x * map.height] = false;
		numpossiblelocations--;
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
	free(trapexcludelocations);
	free(monsterexcludelocations);
	free(lootexcludelocations);
	free(firstroomtile);
	free(subRoomName);
	free(sublevelname);
	free(fullname);
	list_FreeAll(&subRoomMapList);
	list_FreeAll(&mapList);
	list_FreeAll(&doorList);
	printlog("successfully generated a dungeon with %d rooms, %d monsters, %d gold, %d items, %d decorations.\n", roomcount, nummonsters, numGenGold, numGenItems, numGenDecorations);
	messagePlayer(0, "successfully generated a dungeon with %d rooms, %d monsters, %d gold, %d items, %d decorations.\n", roomcount, nummonsters, numGenGold, numGenItems, numGenDecorations);
	return secretlevelexit;
}

/*-------------------------------------------------------------------------------

	assignActions

	configures a map to be playable from a default state

-------------------------------------------------------------------------------*/

void assignActions(map_t* map)
{
	Sint32 x, y, c;
	//Sint32 z;
	node_t* node, *nextnode;
	Entity* entity, *childEntity;
	Item* item;
	bool itemsdonebefore = false;

	if (map == NULL)
	{
		return;
	}

	// add lava lights
	for ( y = 0; y < map->height; y++ )
	{
		for ( x = 0; x < map->width; x++ )
		{
			if ( lavatiles[map->tiles[y * MAPLAYERS + x * MAPLAYERS * map->height]] )
			{
				lightSphereShadow(x, y, 2, 128);
			}
		}
	}

	// seed the random generator

	prng_seed_bytes(&mapseed, sizeof(mapseed));

	// assign entity behaviors
	for ( node = map->entities->first; node != NULL; node = nextnode )
	{
		entity = (Entity*)node->element;
		nextnode = node->next;
		switch ( entity->sprite )
		{
			// null:
			case 0:
			{
				list_RemoveNode(entity->mynode);
				entity = NULL;
				break;
				// player:
			}
			case 1:
			{
				if (numplayers >= 0 && numplayers < MAXPLAYERS)
				{
					if ( client_disconnected[numplayers] )
					{
						// don't spawn missing players
						numplayers++;
						list_RemoveNode(entity->mynode);
						entity = NULL;
						break;
					}
					if ( multiplayer != CLIENT )
					{
						if ( stats[numplayers]->HP <= 0 )
						{
							messagePlayer(numplayers, language[1109]);
							stats[numplayers]->HP = stats[numplayers]->MAXHP / 2;
							stats[numplayers]->MP = stats[numplayers]->MAXMP / 2;
							stats[numplayers]->HUNGER = 500;
							for ( c = 0; c < NUMEFFECTS; c++ )
							{
								stats[numplayers]->EFFECTS[c] = false;
								stats[numplayers]->EFFECTS_TIMERS[c] = 0;
							}
						}
					}
				}
				entity->behavior = &actPlayer;
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
				if ( multiplayer != CLIENT )
				{
					if ( numplayers == 0 && minotaurlevel )
					{
						createMinotaurTimer(entity, map);
					}
				}
				numplayers++;
				if (numplayers > MAXPLAYERS)
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
				entity->sprite = 1;
				entity->flags[PASSABLE] = true;
				entity->behavior = &actDoorFrame;
				childEntity = newEntity(2, 0, map->entities);
				childEntity->x = entity->x;
				childEntity->y = entity->y;
				//printlog("16 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->sizex = 1;
				childEntity->sizey = 8;
				childEntity->behavior = &actDoor;
				childEntity->flags[BLOCKSIGHT] = true;
				childEntity->skill[0] = 0; // signify behavior code of DOOR_DIR
				childEntity = newEntity(1, 0, map->entities);
				childEntity->flags[INVISIBLE] = true;
				childEntity->flags[BLOCKSIGHT] = true;
				childEntity->x = entity->x;
				childEntity->y = entity->y - 7;
				//printlog("17 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->sizex = 2;
				childEntity->sizey = 2;
				childEntity->behavior = &actDoorFrame;
				childEntity = newEntity(1, 0, map->entities);
				childEntity->flags[INVISIBLE] = true;
				childEntity->flags[BLOCKSIGHT] = true;
				childEntity->x = entity->x;
				childEntity->y = entity->y + 7;
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
				entity->sprite = 1;
				entity->flags[PASSABLE] = true;
				entity->behavior = &actDoorFrame;
				childEntity = newEntity(2, 0, map->entities);
				childEntity->x = entity->x;
				childEntity->y = entity->y;
				//printlog("19 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->sizex = 8;
				childEntity->sizey = 1;
				childEntity->yaw -= PI / 2.0;
				childEntity->behavior = &actDoor;
				childEntity->flags[BLOCKSIGHT] = true;
				childEntity->skill[0] = 1; // signify behavior code of DOOR_DIR
				childEntity = newEntity(1, 0, map->entities);
				childEntity->flags[INVISIBLE] = true;
				childEntity->flags[BLOCKSIGHT] = true;
				childEntity->x = entity->x - 7;
				childEntity->y = entity->y;
				//printlog("20 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->sizex = 2;
				childEntity->sizey = 2;
				childEntity->behavior = &actDoorFrame;
				childEntity = newEntity(1, 0, map->entities);
				childEntity->flags[INVISIBLE] = true;
				childEntity->flags[BLOCKSIGHT] = true;
				childEntity->x = entity->x + 7;
				childEntity->y = entity->y;
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
				entity->flags[BRIGHT] = true;
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
				entity->flags[BRIGHT] = true;
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
				entity->flags[BRIGHT] = true;
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
				entity->flags[BRIGHT] = true;
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
				entity->yaw = (prng_get_uint() % 360) * PI / 180.0;
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
							int balance = 0;
							int i;
							for ( i = 0; i < MAXPLAYERS; i++ )
							{
								if ( !client_disconnected[i] )
								{
									balance++;
								}
							}
							bool extrafood = false;
							switch ( balance )
							{
								case 2:
									if ( prng_get_uint() % 8 == 0 )
									{
										extrafood = true;
									}
									break;
								case 3:
									if ( prng_get_uint() % 5 == 0 )
									{
										extrafood = true;
									}
									break;
								case 4:
									if ( prng_get_uint() % 4 == 0 )
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
								if ( prng_get_uint() % 2 == 0 )
								{
									// possible magicstaff
									int randType = prng_get_uint() % (NUMCATEGORIES - 1);
									if ( randType == THROWN && prng_get_uint() % 3 ) // THROWN items 66% to be re-roll.
									{
										randType = prng_get_uint() % (NUMCATEGORIES - 1);
									}
									entity->skill[10] = itemLevelCurve(static_cast<Category>(randType));
								}
								else
								{
									// impossible magicstaff
									int randType = prng_get_uint() % (NUMCATEGORIES - 2);
									if ( randType >= MAGICSTAFF )
									{
										randType++;
									}
									if ( randType == THROWN && prng_get_uint() % 3 ) // THROWN items 66% to be re-roll.
									{
										randType = prng_get_uint() % (NUMCATEGORIES - 2);
										if ( randType >= MAGICSTAFF )
										{
											randType++;
										}
									}
									entity->skill[10] = itemLevelCurve(static_cast<Category>(randType));
								}
							}
							else
							{
								entity->skill[10] = itemLevelCurve(FOOD);
							}
						}
					}
					else
					{
						// editor set the random category of the item to be spawned.
						if ( entity->skill[16] > 0 && entity->skill[16] <= 13 )
						{
							entity->skill[10] = itemLevelCurve(static_cast<Category>(entity->skill[16] - 1));
						}
						else
						{
							int randType = 0;
							if ( entity->skill[16] == 14 )
							{
								// equipment
								randType = prng_get_uint() % 2;
								if ( randType == 0 )
								{
									entity->skill[10] = itemLevelCurve(static_cast<Category>(WEAPON));
								}
								else if ( randType == 1 )
								{
									entity->skill[10] = itemLevelCurve(static_cast<Category>(ARMOR));
								}
							}
							else if ( entity->skill[16] == 15 )
							{
								// jewelry
								randType = prng_get_uint() % 2;
								if ( randType == 0 )
								{
									entity->skill[10] = itemLevelCurve(static_cast<Category>(AMULET));
								}
								else
								{
									entity->skill[10] = itemLevelCurve(static_cast<Category>(RING));
								}
							}
							else if ( entity->skill[16] == 16 )
							{
								// magical
								randType = prng_get_uint() % 3;
								if ( randType == 0 )
								{
									entity->skill[10] = itemLevelCurve(static_cast<Category>(SCROLL));
								}
								else if ( randType == 1 )
								{
									entity->skill[10] = itemLevelCurve(static_cast<Category>(MAGICSTAFF));
								}
								else
								{
									entity->skill[10] = itemLevelCurve(static_cast<Category>(SPELLBOOK));
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
						entity->skill[11] = 1 + prng_get_uint() % 4; // status
					}
					else
					{
						entity->skill[11]--; //editor set number, sets this value to 0-5, with 1 being BROKEN, 5 being EXCELLENT
					}
				}
				else
				{
					entity->skill[11] = EXCELLENT;
				}
				if ( entity->sprite == 8 )
				{
					if ( entity->skill[12] == 10 ) //random, else the value of this variable is the curse/bless
					{
						if ( prng_get_uint() % 2 == 0 )   // 50% chance of curse/bless
						{
							entity->skill[12] = -2 + prng_get_uint() % 5;
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
				if ( entity->sprite == 8 && entity->skill[13] == 0 )
				{
					entity->skill[13] = 1; // count set by maps.cpp, otherwise set by editor
				}
				if ( !itemsdonebefore && !strcmp(map->name, "Start Map") )
				{
					entity->skill[14] = getBook("My Journal");
				}
				else
				{
					if ( items[entity->skill[10]].category == SCROLL || items[entity->skill[10]].variations > 1 )
					{
						entity->skill[14] = prng_get_uint();    // appearance
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
					entity->skill[15] = prng_get_uint() % 2;
				}
				else
				{
					entity->skill[15] = 0; // unidentified.
				}
				item = newItemFromEntity(entity);
				entity->sprite = itemModel(item);
				free(item);
				if ( !entity->skill[18] )
				{
					// shurikens and chakrams need to lie flat on floor as their models are rotated.
					if ( item->type == CRYSTAL_SHURIKEN || item->type == STEEL_CHAKRAM )
					{
						entity->roll = PI;
						if ( item->type == CRYSTAL_SHURIKEN )
						{
							entity->z = 8.5 - models[entity->sprite]->sizey * .25;
						}
						else
						{
							entity->z = 8.75 - models[entity->sprite]->sizey * .25;
						}
					}
					else
					{
						entity->z = 7.5 - models[entity->sprite]->sizey * .25;
					}
				}
				entity->skill[18] = 1; // so the item retains its position
				itemsdonebefore = true;
				break;
			}
			// gold:
			case 9:
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x += 8;
				entity->y += 8;
				entity->z = 6.5;
				entity->yaw = (prng_get_uint() % 360) * PI / 180.0;
				entity->flags[PASSABLE] = true;
				entity->behavior = &actGoldBag;
				entity->skill[0] = 10 + rand() % 100; // amount
				entity->sprite = 130; // gold bag model
				if ( !strcmp(map->name, "Sokoban") )
				{
					entity->flags[INVISIBLE] = true;
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
			{
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x += 8;
				entity->y += 8;
				entity->z = 6;
				entity->yaw = (rand() % 360) * PI / 180.0;
				entity->behavior = &actMonster;
				entity->flags[UPDATENEEDED] = true;
				entity->skill[5] = -1;
				Stat* myStats = NULL;
				if ( multiplayer != CLIENT )
				{
					myStats = entity->getStats();
				}

				Monster monsterType = SKELETON;

				if ( entity->sprite == 27 )   // human.png
				{
					monsterType = HUMAN;
				}
				else if ( entity->sprite == 30 )     // troll.png
				{
					monsterType = TROLL;
				}
				else if ( entity->sprite == 35 )     // shop.png
				{
					monsterType = SHOPKEEPER;
				}
				else if ( entity->sprite == 36 )     // goblin.png
				{
					monsterType = GOBLIN;
				}
				else if ( entity->sprite == 48 )     // spider.png
				{
					monsterType = SPIDER;
				}
				else if ( entity->sprite == 62 )     // herx.png
				{
					monsterType = LICH;
				}
				else if ( entity->sprite == 70 )     // gnome.png
				{
					monsterType = GNOME;
				}
				else if ( entity->sprite == 71 )     // devil.png
				{
					monsterType = DEVIL;
				}
				else if ( entity->sprite == 83 )     // devil.png
				{
					monsterType = SKELETON;
				}
				else if ( entity->sprite == 84 )     // devil.png
				{
					monsterType = KOBOLD;
				}
				else if ( entity->sprite == 85 )     // devil.png
				{
					monsterType = SCARAB;
				}
				else if ( entity->sprite == 86 )     // devil.png
				{
					monsterType = CRYSTALGOLEM;
				}
				else if ( entity->sprite == 87 )     // devil.png
				{
					monsterType = INCUBUS;
				}
				else if ( entity->sprite == 88 )     // devil.png
				{
					monsterType = VAMPIRE;
				}
				else if ( entity->sprite == 89 )     // devil.png
				{
					monsterType = SHADOW;
				}
				else if ( entity->sprite == 90 )     // devil.png
				{
					monsterType = COCKATRICE;
				}
				else if ( entity->sprite == 91 )     // devil.png
				{
					monsterType = INSECTOID;
				}
				else if ( entity->sprite == 92 )     // devil.png
				{
					monsterType = GOATMAN;
				}
				else if ( entity->sprite == 93 )     // devil.png
				{
					monsterType = AUTOMATON;
				}
				else if ( entity->sprite == 94 )     // devil.png
				{
					monsterType = LICH_ICE;
				}
				else if ( entity->sprite == 95 )     // devil.png
				{
					monsterType = LICH_FIRE;
				}
				else if ( entity->sprite == 81 )     // devil.png
				{
					monsterType = RAT;
				}
				else if ( entity->sprite == 75 )     // devil.png
				{
					monsterType = DEMON;
				}
				else if ( entity->sprite == 76 )     // devil.png
				{
					monsterType = CREATURE_IMP;
				}
				else if ( entity->sprite == 77 )     // devil.png
				{
					monsterType = MINOTAUR;
				}
				else if ( entity->sprite == 78 )     // devil.png
				{
					monsterType = SCORPION;
				}
				else if ( entity->sprite == 79 )     // devil.png
				{
					monsterType = SLIME;
				}
				else if ( entity->sprite == 80 )     // devil.png
				{
					monsterType = SUCCUBUS;
				}
				else if ( entity->sprite == 82 )     // devil.png
				{
					monsterType = GHOUL;
				}
				else
				{
					monsterType = static_cast<Monster>(monsterCurve(currentlevel));

				}

				if ( multiplayer != CLIENT )
				{
					if ( myStats == nullptr )
					{
						// need to give the entity its list stuff.
						// create an empty first node for traversal purposes
						node_t* node2 = list_AddNodeFirst(&entity->children);
						node2->element = NULL;
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
							// should not occur
							myStats = new Stat(entity->sprite);
						}
						node2 = list_AddNodeLast(&entity->children);
						node2->element = myStats;
						//					node2->deconstructor = &myStats->~Stat;
						node2->size = sizeof(myStats);
					}
					else if ( entity->sprite == 10 )
					{
						// monster is random, but generated from editor
						// stat struct is already created, need to set stats
						setDefaultMonsterStats(myStats, monsterType + 1000);
						setRandomMonsterStats(myStats);
					}
				}

				switch ( monsterType )
				{
					case RAT:
						entity->focalx = limbs[RAT][0][0]; // 0
						entity->focaly = limbs[RAT][0][1]; // 0
						entity->focalz = limbs[RAT][0][2]; // 0
						break;
					case SCORPION:
						entity->focalx = limbs[SCORPION][0][0]; // 0
						entity->focaly = limbs[SCORPION][0][1]; // 0
						entity->focalz = limbs[SCORPION][0][2]; // 0
						break;
					case HUMAN:
						entity->z = -1;
						entity->focalx = limbs[HUMAN][0][0]; // 0
						entity->focaly = limbs[HUMAN][0][1]; // 0
						entity->focalz = limbs[HUMAN][0][2]; // -1.5
						break;
					case GOBLIN:
						entity->z = 0;
						entity->focalx = limbs[GOBLIN][0][0]; // 0
						entity->focaly = limbs[GOBLIN][0][1]; // 0
						entity->focalz = limbs[GOBLIN][0][2]; // -1.75
						break;
					case SLIME:
						if ( multiplayer != CLIENT )
						{
							myStats->LVL = 7;
						}
						break;
					case SUCCUBUS:
						entity->z = -1;
						entity->focalx = limbs[SUCCUBUS][0][0]; // 0
						entity->focaly = limbs[SUCCUBUS][0][1]; // 0
						entity->focalz = limbs[SUCCUBUS][0][2]; // -1.5
						break;
					case TROLL:
						entity->z = -1.5;
						entity->focalx = limbs[TROLL][0][0]; // 1
						entity->focaly = limbs[TROLL][0][1]; // 0
						entity->focalz = limbs[TROLL][0][2]; // -2
						break;
					case SHOPKEEPER:
						entity->z = -1;
						entity->focalx = limbs[SHOPKEEPER][0][0]; // 0
						entity->focaly = limbs[SHOPKEEPER][0][1]; // 0
						entity->focalz = limbs[SHOPKEEPER][0][2]; // -1.5
						break;
					case SKELETON:
						entity->z = -.5;
						entity->focalx = limbs[SKELETON][0][0]; // 0
						entity->focaly = limbs[SKELETON][0][1]; // 0
						entity->focalz = limbs[SKELETON][0][2]; // -1.5
						break;
					case MINOTAUR:
						entity->z = -6;
						entity->focalx = limbs[MINOTAUR][0][0]; // 0
						entity->focaly = limbs[MINOTAUR][0][1]; // 0
						entity->focalz = limbs[MINOTAUR][0][2]; // 0
						break;
					case GHOUL:
						entity->z = -.25;
						entity->focalx = limbs[GHOUL][0][0]; // 0
						entity->focaly = limbs[GHOUL][0][1]; // 0
						entity->focalz = limbs[GHOUL][0][2]; // -1.5
						break;
					case DEMON:
						entity->z = -8.5;
						entity->focalx = limbs[DEMON][0][0]; // -1
						entity->focaly = limbs[DEMON][0][1]; // 0
						entity->focalz = limbs[DEMON][0][2]; // -1.25
						break;
					case SPIDER:
						entity->z = 4.5;
						entity->focalx = limbs[SPIDER][0][0]; // -3
						entity->focaly = limbs[SPIDER][0][1]; // 0
						entity->focalz = limbs[SPIDER][0][2]; // -1
						break;
					case LICH:
						entity->focalx = limbs[LICH][0][0]; // -0.75
						entity->focaly = limbs[LICH][0][1]; // 0
						entity->focalz = limbs[LICH][0][2]; // 0
						entity->z = -2;
						entity->yaw = PI;
						entity->sprite = 274;
						entity->skill[29] = 120;
						break;
					case CREATURE_IMP:
						entity->z = -4.5;
						entity->focalx = limbs[CREATURE_IMP][0][0]; // 0
						entity->focaly = limbs[CREATURE_IMP][0][1]; // 0
						entity->focalz = limbs[CREATURE_IMP][0][2]; // -1.75
						break;
					case GNOME:
						entity->z = 2.25;
						entity->focalx = limbs[GNOME][0][0]; // 0
						entity->focaly = limbs[GNOME][0][1]; // 0
						entity->focalz = limbs[GNOME][0][2]; // -2
						break;
					case DEVIL:
						entity->focalx = limbs[DEVIL][0][0]; // 0
						entity->focaly = limbs[DEVIL][0][1]; // 0
						entity->focalz = limbs[DEVIL][0][2]; // 0
						entity->z = -4;
						entity->sizex = 20;
						entity->sizey = 20;
						entity->yaw = PI;
						break;
					case KOBOLD:
						entity->z = 2.25;
						entity->focalx = limbs[KOBOLD][0][0]; // 0
						entity->focaly = limbs[KOBOLD][0][1]; // 0
						entity->focalz = limbs[KOBOLD][0][2]; // -2
						break;
					case SCARAB:
						entity->focalx = limbs[SCARAB][0][0]; // 0
						entity->focaly = limbs[SCARAB][0][1]; // 0
						entity->focalz = limbs[SCARAB][0][2]; // 0
						break;
					case CRYSTALGOLEM:
						entity->z = -1.5;
						entity->focalx = limbs[CRYSTALGOLEM][0][0]; // 1
						entity->focaly = limbs[CRYSTALGOLEM][0][1]; // 0
						entity->focalz = limbs[CRYSTALGOLEM][0][2]; // -2
						break;
					case INCUBUS:
						entity->z = -1;
						entity->focalx = limbs[INCUBUS][0][0]; // 0
						entity->focaly = limbs[INCUBUS][0][1]; // 0
						entity->focalz = limbs[INCUBUS][0][2]; // -1.5
						break;
					case VAMPIRE:
						entity->z = -1;
						entity->focalx = limbs[VAMPIRE][0][0]; // 0
						entity->focaly = limbs[VAMPIRE][0][1]; // 0
						entity->focalz = limbs[VAMPIRE][0][2]; // -1.5
						break;
					case SHADOW:
						entity->z = -1;
						entity->focalx = limbs[SHADOW][0][0]; // 0
						entity->focaly = limbs[SHADOW][0][1]; // 0
						entity->focalz = limbs[SHADOW][0][2]; // -1.75
						break;
					case COCKATRICE:
						entity->z = -4.5;
						entity->focalx = limbs[COCKATRICE][0][0]; // 0
						entity->focaly = limbs[COCKATRICE][0][1]; // 0
						entity->focalz = limbs[COCKATRICE][0][2]; // -1.75
						break;
					case INSECTOID:
						entity->z = 0;
						entity->focalx = limbs[INSECTOID][0][0]; // 0
						entity->focaly = limbs[INSECTOID][0][1]; // 0
						entity->focalz = limbs[INSECTOID][0][2]; // -1.75
						break;
					case GOATMAN:
						entity->z = 0;
						entity->focalx = limbs[GOATMAN][0][0]; // 0
						entity->focaly = limbs[GOATMAN][0][1]; // 0
						entity->focalz = limbs[GOATMAN][0][2]; // -1.75
						break;
					case AUTOMATON:
						entity->z = -.5;
						entity->focalx = limbs[AUTOMATON][0][0]; // 0
						entity->focaly = limbs[AUTOMATON][0][1]; // 0
						entity->focalz = limbs[AUTOMATON][0][2]; // -1.5
						break;
					case LICH_ICE:
						entity->focalx = limbs[LICH_ICE][0][0]; // -0.75
						entity->focaly = limbs[LICH_ICE][0][1]; // 0
						entity->focalz = limbs[LICH_ICE][0][2]; // 0
						entity->z = -2;
						entity->yaw = PI;
						entity->sprite = 274;
						entity->skill[29] = 120;
						break;
					case LICH_FIRE:
						entity->focalx = limbs[LICH_FIRE][0][0]; // -0.75
						entity->focaly = limbs[LICH_FIRE][0][1]; // 0
						entity->focalz = limbs[LICH_FIRE][0][2]; // 0
						entity->z = -2;
						entity->yaw = PI;
						entity->sprite = 274;
						entity->skill[29] = 120;
						break;
					default:
						break;
				}
				if ( multiplayer != CLIENT )
				{
					myStats->type = monsterType;
					if ( myStats->type == DEVIL )
					{
						childEntity = newEntity(72, 1, map->entities);
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
				entity->z = 5.45;
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
				entity->flags[BRIGHT] = true;
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
				//Randomly determine effect.
				int effect = rand() % 10; //3 possible effects.
				entity->skill[28] = 1; //TODO: This is just for testing purposes.
				switch (effect)
				{
					case 0:
						//10% chance
						entity->skill[1] = 3; //Will bless equipment
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
						entity->skill[3] = rand() % 15; //Randomly choose from the number of potion effects there are.
						break;
					default:
						break; //Should never happen.
				}
				break;
			}
			//Sink.
			case 15:
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x += 8;
				entity->y += 8;
				entity->z = 5;
				entity->behavior = &actSink;
				entity->sprite = 164;
				entity->skill[0] = 1 + rand() % 4; // number of uses
				switch ( rand() % 10 )
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
				break;
			//Switch.
			case 17:
				entity->sizex = 1;
				entity->sizey = 1;
				entity->x += 8;
				entity->y += 8;
				entity->z = 7;
				entity->sprite = 184; // this is the switch base.
				entity->flags[PASSABLE] = true;
				childEntity = newEntity(186, 0, map->entities);
				childEntity->x = entity->x;
				childEntity->y = entity->y;
				//printlog("22 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->z = 8;
				childEntity->focalz = -4.5;
				childEntity->sizex = 1;
				childEntity->sizey = 1;
				childEntity->sprite = 185; // this is the switch handle.
				childEntity->roll = PI / 4; // "off" position
				childEntity->flags[PASSABLE] = true;
				childEntity->behavior = &actSwitch;
				break;
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
				entity->x += 8;
				entity->y += 8;
				entity->yaw -= PI / 2.0;
				entity->sprite = 1;
				entity->flags[PASSABLE] = true;
				entity->behavior = &actDoorFrame;

				//entity->skill[28] = 1; //It's a mechanism.
				childEntity = newEntity(186, 0, map->entities);
				childEntity->x = entity->x;
				childEntity->y = entity->y;
				//printlog("23 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->sizex = 8;
				childEntity->sizey = 1;
				childEntity->yaw -= PI / 2.0;
				childEntity->gateInverted = 0; // non-inverted
				childEntity->gateStatus = 0; // closed.
				childEntity->skill[28] = 1; //It's a mechanism.
				childEntity->behavior = &actGate;
				childEntity->skill[0] = 1; // signify behavior code of DOOR_DIR

				childEntity = newEntity(1, 0, map->entities);
				childEntity->flags[INVISIBLE] = true;
				childEntity->flags[BLOCKSIGHT] = true;
				childEntity->x = entity->x - 7;
				childEntity->y = entity->y;
				//printlog("24 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->sizex = 2;
				childEntity->sizey = 2;
				childEntity->behavior = &actDoorFrame;

				childEntity = newEntity(1, 0, map->entities);
				childEntity->flags[INVISIBLE] = true;
				childEntity->flags[BLOCKSIGHT] = true;
				childEntity->x = entity->x + 7;
				childEntity->y = entity->y;
				//printlog("25 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->sizex = 2;
				childEntity->sizey = 2;
				childEntity->behavior = &actDoorFrame;
				break;
			//East/west gate: //TODO: Adjust this. It's a copypaste of door.
			case 20:
				entity->x += 8;
				entity->y += 8;
				entity->sprite = 1;
				entity->flags[PASSABLE] = true;
				entity->behavior = &actDoorFrame;

				childEntity = newEntity(186, 0, map->entities);
				childEntity->x = entity->x;
				childEntity->y = entity->y;
				//printlog("26 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->sizex = 1;
				childEntity->sizey = 8;
				childEntity->gateInverted = 0; // non-inverted
				childEntity->gateStatus = 0; // closed.
				childEntity->skill[28] = 1; //It's a mechanism.
				childEntity->behavior = &actGate;
				childEntity->skill[0] = 0; // signify behavior code of DOOR_DIR

				childEntity = newEntity(1, 0, map->entities);
				childEntity->flags[INVISIBLE] = true;
				childEntity->flags[BLOCKSIGHT] = true;
				childEntity->x = entity->x;
				childEntity->y = entity->y - 7;
				//printlog("27 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->sizex = 2;
				childEntity->sizey = 2;
				childEntity->behavior = &actDoorFrame;

				childEntity = newEntity(1, 0, map->entities);
				childEntity->flags[INVISIBLE] = true;
				childEntity->flags[BLOCKSIGHT] = true;
				childEntity->x = entity->x;
				childEntity->y = entity->y + 7;
				//printlog("28 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->sizex = 2;
				childEntity->sizey = 2;
				childEntity->behavior = &actDoorFrame;
				break;
			//Chest.
			case 21:
			{
				entity->sizex = 3;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->z = 5.5;
				entity->yaw = entity->yaw * (PI / 2); //set to 0 by default in editor, can be set 0-3
				entity->behavior = &actChest;
				entity->sprite = 188;
				//entity->skill[9] = -1; //Set default chest as random category < 0

				childEntity = newEntity(216, 0, map->entities);
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
				break;
			// summon monster trap
			case 97:
				entity->skill[28] = 1; // is a mechanism
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
				break;
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
				for ( c = 0; c < 4; c++ )
				{
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
							Entity* childEntity = newEntity(252, 1, map->entities);
							childEntity->x = (x << 4) + 8;
							childEntity->y = (y << 4) + 8;
							//printlog("30 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
							entity->flags[PASSABLE] = true;
							if ( !map->tiles[(MAPLAYERS - 1) + y * MAPLAYERS + x * MAPLAYERS * map->height] )
							{
								childEntity->z = -26.99;
							}
							else
							{
								childEntity->z = -10.99;
							}
						}
					}
				}
				break;
			}
			// headstone
			case 39:
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
				break;
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
				entity->x += 8;
				entity->y += 8;
				entity->sprite = 253;
				entity->flags[PASSABLE] = true;
				entity->behavior = &actLadderUp;
				x = entity->x / 16;
				y = entity->y / 16;
				if ( x >= 0 && y >= 0 && x < map->width && y < map->height )
				{
					if ( !map->tiles[(MAPLAYERS - 1) + y * MAPLAYERS + x * MAPLAYERS * map->height] )
					{
						entity->z = -21.49;
					}
					else
					{
						entity->z = -5.49;
					}
				}
				break;
			// boulder
			case 44:
				entity->x += 8;
				entity->y += 8;
				entity->sprite = 245;
				entity->sizex = 7;
				entity->sizey = 7;
				entity->behavior = &actBoulder;
				entity->skill[0] = 1; // BOULDER_STOPPED
				break;
			// portal
			case 45:
				entity->x += 8;
				entity->y += 8;
				entity->sprite = 254;
				entity->sizex = 4;
				entity->sizey = 4;
				entity->yaw = PI / 2;
				entity->behavior = &actPortal;
				entity->flags[PASSABLE] = true;
				entity->flags[BRIGHT] = true;
				break;
			// secret ladder:
			case 46:
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x += 8;
				entity->y += 8;
				entity->z = 5.45;
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
				entity->flags[BURNABLE] = true;
				entity->furnitureType = FURNITURE_TABLE;
				if ( prng_get_uint() % 4 == 0 || !strcmp(map->name, "Start Map") )
				{
					// put an item on the table
					childEntity = newEntity(8, 1, map->entities);
					childEntity->x = entity->x - 8;
					childEntity->y = entity->y - 8;
					//printlog("31 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
					childEntity->z = 0;
					childEntity->skill[18] = 1; // so the item retains its position
					entity->parent = childEntity->getUID();
				}
				if ( prng_get_uint() % 2 == 0 )
				{
					// surround the table with chairs
					int c, j;
					j = prng_get_uint() % 4 + 1;
					for ( c = 0; c < j; c++ )
					{
						childEntity = newEntity(60, 1, map->entities);
						childEntity->x = entity->x - 8;
						childEntity->y = entity->y - 8;
						//printlog("32 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
						childEntity->yaw = ((int)(prng_get_uint() % 80) - 40 + c * 90) * (PI / 180.f);
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
					}
				}
				break;
			}
			// chair
			case 60:
				entity->furnitureType = FURNITURE_CHAIR; // so everything knows I'm a chair
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->z = 8;
				entity->focalz = -5;
				entity->sprite = 272;
				entity->behavior = &actFurniture;
				entity->flags[BURNABLE] = true;
				if ( !entity->yaw )
				{
					entity->yaw = (prng_get_uint() % 360) * (PI / 180.f);
				}
				break;
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
				entity->flags[BRIGHT] = true;
				if ( strstr(map->name, "Boss") )
				{
					entity->flags[INVISIBLE] = true;
					entity->skill[28] = 1; // is a mechanism
				}
				if ( strstr(map->name, "Hell") )
				{
					entity->skill[4] = 2;
				}
				else
				{
					entity->skill[4] = 1;
				}
				break;
			// speartrap:
			case 64:
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
				childEntity = newEntity(283, 0, map->entities);
				childEntity->x = entity->x;
				childEntity->y = entity->y;
				//printlog("33 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->z = entity->z - 7.75 - 0.01;
				childEntity->flags[PASSABLE] = true;
				break;
			// magic trap:
			case 65:
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->behavior = &actMagicTrap;
				entity->flags[SPRITE] = true;
				entity->flags[INVISIBLE] = true;
				entity->flags[PASSABLE] = true;
				entity->skill[28] = 1; // is a mechanism
				break;
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
			// devil teleport location:
			case 72:
			case 73:
			case 74:
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
				entity->flags[BRIGHT] = true;
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
				entity->flags[BRIGHT] = true;
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
				entity->flags[BRIGHT] = true;
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
				entity->flags[BRIGHT] = true;
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

				x = ((int)(entity->x)) >> 4;
				y = ((int)(entity->y)) >> 4;
				if ( x >= 0 && y >= 0 && x < map->width && y < map->height )
				{
					if ( !map->tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map->height] )
					{
						Entity* childEntity = newEntity(252, 1, map->entities);
						childEntity->x = (x << 4) + 8;
						childEntity->y = (y << 4) + 8;
						//printlog("30 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
						entity->flags[PASSABLE] = true;
						if ( !map->tiles[(MAPLAYERS - 1) + y * MAPLAYERS + x * MAPLAYERS * map->height] )
						{
							childEntity->z = -26.99;
						}
						else
						{
							childEntity->z = -10.99;
						}
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

				x = ((int)(entity->x)) >> 4;
				y = ((int)(entity->y)) >> 4;
				if ( x >= 0 && y >= 0 && x < map->width && y < map->height )
				{
					if ( !map->tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map->height] )
					{
						Entity* childEntity = newEntity(252, 1, map->entities);
						childEntity->x = (x << 4) + 8;
						childEntity->y = (y << 4) + 8;
						//printlog("30 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
						entity->flags[PASSABLE] = true;
						if ( !map->tiles[(MAPLAYERS - 1) + y * MAPLAYERS + x * MAPLAYERS * map->height] )
						{
							childEntity->z = -26.99;
						}
						else
						{
							childEntity->z = -10.99;
						}
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

				x = ((int)(entity->x)) >> 4;
				y = ((int)(entity->y)) >> 4;
				if ( x >= 0 && y >= 0 && x < map->width && y < map->height )
				{
					if ( !map->tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map->height] )
					{
						Entity* childEntity = newEntity(252, 1, map->entities);
						childEntity->x = (x << 4) + 8;
						childEntity->y = (y << 4) + 8;
						//printlog("30 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
						entity->flags[PASSABLE] = true;
						if ( !map->tiles[(MAPLAYERS - 1) + y * MAPLAYERS + x * MAPLAYERS * map->height] )
						{
							childEntity->z = -26.99;
						}
						else
						{
							childEntity->z = -10.99;
						}
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

				x = ((int)(entity->x)) >> 4;
				y = ((int)(entity->y)) >> 4;
				if ( x >= 0 && y >= 0 && x < map->width && y < map->height )
				{
					if ( !map->tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map->height] )
					{
						Entity* childEntity = newEntity(252, 1, map->entities);
						childEntity->x = (x << 4) + 8;
						childEntity->y = (y << 4) + 8;
						//printlog("30 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
						entity->flags[PASSABLE] = true;
						if ( !map->tiles[(MAPLAYERS - 1) + y * MAPLAYERS + x * MAPLAYERS * map->height] )
						{
							childEntity->z = -26.99;
						}
						else
						{
							childEntity->z = -10.99;
						}
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

				childEntity = newEntity(578, 0, map->entities); //floating crystal
				childEntity->parent = entity->getUID();

				childEntity->x = entity->x;
				childEntity->y = entity->y;
				childEntity->sizex = 4;
				childEntity->sizey = 4;
				childEntity->crystalStartZ = entity->z - 10; //start position
				childEntity->z = childEntity->crystalStartZ - 0.4 + ((prng_get_uint() % 8) * 0.1); // start the height randomly
				childEntity->crystalMaxZVelocity = 0.02; //max velocity
				childEntity->crystalMinZVelocity = 0.001; //min velocity
				childEntity->crystalTurnVelocity = 0.2; //yaw turning velocity
				childEntity->vel_z = childEntity->crystalMaxZVelocity * ((prng_get_uint() % 100) * 0.01); // start the velocity randomly

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

				node_t* tempNode = list_AddNodeLast(&entity->children);
				tempNode->element = childEntity; // add the node to the children list.
				tempNode->deconstructor = &emptyDeconstructor;
				tempNode->size = sizeof(Entity*);

				break;
			}
			// set beartrap
			case 107:
			{
				entity->skill[0] = 1; // so everything knows I'm a chair
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x += 8;
				entity->y += 8;
				entity->z = 6.75;

				entity->focalz = -5;
				entity->sprite = 98;

				entity->behavior = &actBeartrap;
				entity->flags[PASSABLE] = true;
				entity->flags[UPDATENEEDED] = true;
				if ( !entity->yaw )
				{
					entity->yaw = (prng_get_uint() % 360) * (PI / 180.f);
				}
				entity->roll = -PI / 2; // flip the model

				entity->skill[11] = EXCELLENT; //status
				entity->skill[12] = 0; //beatitude
				entity->skill[13] = 1; //qty
				entity->skill[14] = 0; //appearance
				entity->skill[15] = 0; //identified
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
				entity->x += 8;
				entity->y += 8;
				entity->sprite = 583;
				entity->sizex = 4;
				entity->sizey = 4;
				entity->z = -1.75;
				x = entity->x / 16;
				y = entity->y / 16;
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
			case 112: //stalagtite multiple
				entity->x += 8;
				entity->y += 8;
				entity->sprite = 584;
				entity->sizex = 7;
				entity->sizey = 7;
				entity->z = 1;
				x = entity->x / 16;
				y = entity->y / 16;
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
			//North/South gate inverted: //TODO: Adjust this. It's a copypaste of door.
			case 113:
				entity->x += 8;
				entity->y += 8;
				entity->yaw -= PI / 2.0;
				entity->sprite = 1;
				entity->flags[PASSABLE] = true;
				entity->behavior = &actDoorFrame;

				//entity->skill[28] = 1; //It's a mechanism.
				childEntity = newEntity(186, 0, map->entities);
				childEntity->x = entity->x;
				childEntity->y = entity->y;
				//printlog("23 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->sizex = 8;
				childEntity->sizey = 1;
				childEntity->yaw -= PI / 2.0;
				childEntity->gateInverted = 1; // inverted.
				childEntity->gateStatus = 1; // open.
				childEntity->skill[28] = 1; //It's a mechanism.
				childEntity->behavior = &actGate;
				childEntity->skill[0] = 1; // signify behavior code of DOOR_DIR

				childEntity = newEntity(1, 0, map->entities);
				childEntity->flags[INVISIBLE] = true;
				childEntity->flags[BLOCKSIGHT] = true;
				childEntity->x = entity->x - 7;
				childEntity->y = entity->y;
				//printlog("24 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->sizex = 2;
				childEntity->sizey = 2;
				childEntity->behavior = &actDoorFrame;

				childEntity = newEntity(1, 0, map->entities);
				childEntity->flags[INVISIBLE] = true;
				childEntity->flags[BLOCKSIGHT] = true;
				childEntity->x = entity->x + 7;
				childEntity->y = entity->y;
				//printlog("25 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->sizex = 2;
				childEntity->sizey = 2;
				childEntity->behavior = &actDoorFrame;
				break;
			//East/west gate inverted: //TODO: Adjust this. It's a copypaste of door.
			case 114:
				entity->x += 8;
				entity->y += 8;
				entity->sprite = 1;
				entity->flags[PASSABLE] = true;
				entity->behavior = &actDoorFrame;

				childEntity = newEntity(186, 0, map->entities);
				childEntity->x = entity->x;
				childEntity->y = entity->y;
				//printlog("26 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->sizex = 1;
				childEntity->gateInverted = 1; // inverted.
				childEntity->gateStatus = 1; // open.
				childEntity->sizey = 8;
				childEntity->skill[28] = 1; //It's a mechanism.
				childEntity->behavior = &actGate;
				childEntity->skill[0] = 0; // signify behavior code of DOOR_DIR

				childEntity = newEntity(1, 0, map->entities);
				childEntity->flags[INVISIBLE] = true;
				childEntity->flags[BLOCKSIGHT] = true;
				childEntity->x = entity->x;
				childEntity->y = entity->y - 7;
				//printlog("27 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->sizex = 2;
				childEntity->sizey = 2;
				childEntity->behavior = &actDoorFrame;

				childEntity = newEntity(1, 0, map->entities);
				childEntity->flags[INVISIBLE] = true;
				childEntity->flags[BLOCKSIGHT] = true;
				childEntity->x = entity->x;
				childEntity->y = entity->y + 7;
				//printlog("28 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->sizex = 2;
				childEntity->sizey = 2;
				childEntity->behavior = &actDoorFrame;
				break;
			//Switch with timer.
			case 115:
				entity->sizex = 1;
				entity->sizey = 1;
				entity->x += 8;
				entity->y += 8;
				entity->z = 7;
				entity->sprite = 585; // this is the switch base.
				entity->flags[PASSABLE] = true;
				childEntity = newEntity(586, 0, map->entities);
				childEntity->x = entity->x;
				childEntity->y = entity->y;
				//printlog("22 Generated entity. Sprite: %d Uid: %d X: %.2f Y: %.2f\n",childEntity->sprite,childEntity->getUID(),childEntity->x,childEntity->y);
				childEntity->z = 8;
				childEntity->leverTimerTicks = std::max(entity->leverTimerTicks, 1) * TICKS_PER_SECOND; // convert seconds to ticks from editor, make sure not less than 1
				childEntity->leverStatus = 0; // set default to off.
				childEntity->focalz = -4.5;
				childEntity->sizex = 1;
				childEntity->sizey = 1;
				childEntity->sprite = 586; // this is the switch handle.
				childEntity->roll = -PI / 4; // "off" position
				childEntity->flags[PASSABLE] = true;
				childEntity->behavior = &actSwitchWithTimer;
				break;
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
				if ( entity->pedestalInGround )
				{
					entity->z += 11;
					entity->flags[PASSABLE] = true;
				}

				childEntity = newEntity(602 + entity->pedestalOrbType - 1, 0, map->entities); //floating orb
				childEntity->parent = entity->getUID();
				childEntity->behavior = &actPedestalOrb;
				childEntity->x = entity->x;
				childEntity->y = entity->y;
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
				entity->sprite = 278;
				entity->sizex = 4;
				entity->sizey = 4;
				entity->yaw = PI / 2;
				entity->behavior = &actMidGamePortal;
				entity->flags[PASSABLE] = true;
				entity->flags[BRIGHT] = true;
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
					entity->sprite = 253; // ladder hole
					entity->behavior = &actTeleporter;
					x = entity->x / 16;
					y = entity->y / 16;
					if ( x >= 0 && y >= 0 && x < map->width && y < map->height )
					{
						if ( !map->tiles[(MAPLAYERS - 1) + y * MAPLAYERS + x * MAPLAYERS * map->height] )
						{
							entity->z = -21.49;
						}
						else
						{
							entity->z = -5.49;
						}
					}
				}
				else if ( entity->teleporterType == 1 )
				{
					entity->sizex = 4;
					entity->sizey = 4;
					entity->z = 5.45;
					entity->flags[PASSABLE] = true;
					entity->behavior = &actTeleporter;
					entity->sprite = 161; // ladder
				}
				else
				{
					entity->sprite = 254;
					entity->sizex = 4;
					entity->sizey = 4;
					entity->yaw = PI / 2;
					entity->behavior = &actTeleporter;
					entity->flags[PASSABLE] = true;
					entity->flags[BRIGHT] = true;
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
				//entity->yaw = PI / 2;
				entity->behavior = &actCeilingTile;
				entity->flags[PASSABLE] = true;
				//entity->flags[BRIGHT] = true;
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

				x = ((int)(entity->x)) >> 4;
				y = ((int)(entity->y)) >> 4;
				map->tiles[y * MAPLAYERS + x * MAPLAYERS * map->height] = 208; //entity->spellTrapCeilingModel
				Entity* childEntity = nullptr;
				if ( x >= 0 && y >= 0 && x < map->width && y < map->height )
				{
					if ( !map->tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map->height] )
					{
						childEntity = newEntity(252, 1, map->entities);
						childEntity->parent = entity->getUID();
						childEntity->x = entity->x;
						childEntity->y = entity->y;
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
						node_t* tempNode = list_AddNodeLast(&entity->children);
						tempNode->element = childEntity; // add the node to the children list.
						tempNode->deconstructor = &emptyDeconstructor;
						tempNode->size = sizeof(Entity*);
					}
				}
				break;
			}
			// arcane chair
			case 121:
				entity->furnitureType = FURNITURE_CHAIR; // so everything knows I'm a chair
				entity->sizex = 2;
				entity->sizey = 2;
				entity->x += 8;
				entity->y += 8;
				entity->z = 8;
				entity->focalz = -5;
				entity->sprite = 626;
				entity->behavior = &actFurniture;
				entity->flags[BURNABLE] = true;
				if ( entity->furnitureDir == -1 && !entity->yaw )
				{
					entity->yaw = (prng_get_uint() % 360) * (PI / 180.f);
				}
				else
				{
					entity->yaw = entity->furnitureDir * 45 * (PI / 180.f);
				}
				break;
			// arcane bed
			case 122:
				entity->furnitureType = FURNITURE_BED; // so everything knows I'm a bed
				entity->x += 8;
				entity->y += 8;
				entity->z = 4;
				entity->sprite = 627;
				entity->behavior = &actFurniture;
				entity->flags[BURNABLE] = true;
				if ( entity->furnitureDir == -1 && !entity->yaw )
				{
					entity->furnitureDir = (prng_get_uint() % 4);
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
			// bunk bed
			case 123:
				entity->furnitureType = FURNITURE_BUNKBED; // so everything knows I'm a bunkbed
				entity->x += 8;
				entity->y += 8;
				entity->z = 1.75;
				entity->sprite = 628;
				entity->behavior = &actFurniture;
				entity->flags[BURNABLE] = true;
				if ( entity->furnitureDir == -1 && !entity->yaw )
				{
					entity->furnitureDir = (prng_get_uint() % 4);
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
				entity->furnitureType = FURNITURE_PODIUM;
				entity->flags[BURNABLE] = true;
				if ( entity->furnitureDir == -1 && !entity->yaw )
				{
					entity->furnitureDir = (prng_get_uint() % 4);
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

				childEntity = newEntity(632, 1, map->entities); //cam1
				childEntity->parent = entity->getUID();
				childEntity->x = entity->x + 2.25;
				childEntity->y = entity->y + 2.25;
				childEntity->behavior = &actPistonCam;
				childEntity->pistonCamRotateSpeed = 0.2;
				childEntity->flags[UNCLICKABLE] = true;
				if ( multiplayer != CLIENT )
				{
					childEntity->setUID(-3);
					entity_uids--;
				}
				childEntity = newEntity(633, 1, map->entities); //cam2
				childEntity->parent = entity->getUID();
				childEntity->x = entity->x - 2.25;
				childEntity->y = entity->y - 2.25;
				childEntity->behavior = &actPistonCam;
				childEntity->pistonCamRotateSpeed = -0.2;
				childEntity->flags[UNCLICKABLE] = true;
				if ( multiplayer != CLIENT )
				{
					childEntity->setUID(-3);
					entity_uids--;
				}
				break;
			}
			default:
				break;
		}
		if ( entity )
		{
			nextnode = node->next;
		}
	}

	for ( node = map->entities->first; node != nullptr; node = node->next )
	{
		Entity* itemEnt = (Entity*)node->element;
		if ( itemEnt->behavior == &actItem )
		{
			// see if there's any platforms to set items upon.
			for ( node_t* tmpnode = map->entities->first; tmpnode != nullptr; tmpnode = tmpnode->next )
			{
				Entity* tmpentity = (Entity*)tmpnode->element;
				if ( (tmpentity->behavior == &actFurniture
						&& (tmpentity->x == itemEnt->x) && (tmpentity->y == itemEnt->y)
					) )
				{
					if ( itemEnt->z > 4 )
					{
						if ( tmpentity->sprite == 271 )
						{
							// is table
							itemEnt->z -= 6;
						}
						else if ( tmpentity->sprite == 630 )
						{
							// is podium
							itemEnt->z -= 6;
						}
					}
					break;
				}
			}
		}
	}
}

void mapLevel(int player)
{
	int x, y;
	for ( y = 0; y < 64; ++y )
	{
		for ( x = 0; x < 64; ++x )
		{
			if ( x < map.width && y < map.height )
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
}

Entity* map_t::getEntityWithUID(Uint32 uid)
{
	Entity* entity = nullptr;

	for ( node_t* node = entities->first; node; node = node->next )
	{
		entity = (node->element? static_cast<Entity*>(node->element) : nullptr);

		if ( !entity )
		{
			continue;
		}

		if ( entity->getUID() == uid )
		{
			return entity;
		}
	}

	return nullptr;
}
