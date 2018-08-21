/*-------------------------------------------------------------------------------

	BARONY
	File: paths.hpp
	Desc: paths.cpp header file

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include <thread>

extern int* pathMapFlying;
extern int* pathMapGrounded;
extern int pathMapZone;

// function prototypes
Uint32 heuristic(int x1, int y1, int x2, int y2);
list_t* generatePath(int x1, int y1, int x2, int y2, Entity* my, Entity* target, bool lavaIsPassable = false);
void generatePathMaps();
void generatePathMapsThreaded();
// return true if an entity is blocks pathing
bool isPathObstacle(Entity* entity);

class PathMapQueueHandler
{
public:
	int nInQueue;
	int kThreadStatus;
	std::thread currentThread;
	int currentMap;
	int currentMapX;
	int currentMapY;
	int localGroundPathMap[MAP_MAX_DIMENSION_X * MAP_MAX_DIMENSION_Y];
	int localFlyingPathMap[MAP_MAX_DIMENSION_X * MAP_MAX_DIMENSION_Y];

	PathMapQueueHandler() :
		nInQueue(0),
		kThreadStatus(PATHMAP_THREAD_IDLE),
		currentMap(0),
		currentMapX(0),
		currentMapY(0)
	{
		memset(localGroundPathMap, 0, MAP_MAX_DIMENSION_X * MAP_MAX_DIMENSION_Y);
		memset(localFlyingPathMap, 0, MAP_MAX_DIMENSION_X * MAP_MAX_DIMENSION_Y);
	}

	void addRequest();
	bool processThread();
	void copyPathMap();
	void initThread();
	void deinit();
};
extern PathMapQueueHandler PathMapQueue;