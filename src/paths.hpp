/*-------------------------------------------------------------------------------

	BARONY
	File: paths.hpp
	Desc: paths.cpp header file

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

extern int* pathMapFlying;
extern int* pathMapGrounded;
extern int pathMapZone;

// pathnode struct
typedef struct pathnode
{
	Sint32 x = 0, y = 0;     // location
	Uint32 g = 0, h = 0;     // heuristic info
	Sint32 px = -1, py = -1; // parent location
} pathnode_t;

// function prototypes
Uint32 heuristic(int x1, int y1, int x2, int y2);
enum GeneratePathTypes
{
	GENERATE_PATH_DEFAULT,
	GENERATE_PATH_BOULDER_BREAK,
	GENERATE_PATH_BOSS_TRACKING_IDLE,
	GENERATE_PATH_BOSS_TRACKING_HUNT,
	GENERATE_PATH_IDLE_WALK,
	GENERATE_PATH_TO_HUNT_MONSTER_TARGET,
	GENERATE_PATH_ALLY_FOLLOW,
	GENERATE_PATH_ALLY_FOLLOW2,
	GENERATE_PATH_INTERACT_MOVE,
	GENERATE_PATH_MONSTER_MOVE_BACKWARDS,
	GENERATE_PATH_PLAYER_ALLY_MOVETO,
	GENERATE_PATH_PLAYER_GYRO_RETURN,
	GENERATE_PATH_CHECK_EXIT,
	GENERATE_PATH_MOVEASIDE,
	GENERATE_PATH_ACHIEVEMENT
};
extern int lastGeneratePathTries;
list_t* generatePath(int x1, int y1, int x2, int y2, Entity* my, Entity* target, GeneratePathTypes pathingType, bool lavaIsPassable = false);
void generatePathMaps();
// return true if an entity is blocks pathing
bool isPathObstacle(Entity* entity);
int pathCheckObstacle(int x, int y, Entity* my, Entity* target);
void updateGatePath(Entity& entity);
