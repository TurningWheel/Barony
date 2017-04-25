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

// function prototypes
Uint32 heuristic(int x1, int y1, int x2, int y2);
list_t* generatePath(int x1, int y1, int x2, int y2, Entity* my, Entity* target, bool lavaIsPassable = false);
void generatePathMaps();
