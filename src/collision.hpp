/*-------------------------------------------------------------------------------

	BARONY
	File: collision.hpp
	Desc: collision.cpp header file

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#define IGNORE_ENTITIES 1

// function prototypes
DOUBLE entityDist(Entity* my, Entity* your);
Entity* entityClicked();
bool entityInsideTile(Entity* entity, int x, int y, int z);
bool entityInsideEntity(Entity* entity1, Entity* entity2);
bool entityInsideSomething(Entity* entity);
int barony_clear(DOUBLE tx, DOUBLE ty, Entity* my);
DOUBLE clipMove(DOUBLE* x, DOUBLE* y, DOUBLE vx, DOUBLE vy, Entity* my);
Entity* findEntityInLine(Entity* my, DOUBLE x1, DOUBLE y1, DOUBLE angle, int entities, Entity* target);
DOUBLE lineTrace(Entity* my, DOUBLE x1, DOUBLE y1, DOUBLE angle, DOUBLE range, int entities, bool ground);
DOUBLE lineTraceTarget(Entity* my, DOUBLE x1, DOUBLE y1, DOUBLE angle, DOUBLE range, int entities, bool ground, Entity* target); //If the linetrace function encounters the linetrace entity, it returns even if it's invisible or passable.
int checkObstacle(long x, long y, Entity* my, Entity* target);
