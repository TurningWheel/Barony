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
double entityDist(Entity *my, Entity *your);
Entity *entityClicked();
bool entityInsideTile(Entity *entity, int x, int y, int z);
bool entityInsideEntity(Entity *entity1, Entity *entity2);
bool entityInsideSomething(Entity *entity);
int barony_clear(double tx, double ty, Entity *my);
double clipMove(double *x, double *y, double vx, double vy, Entity *my);
Entity *findEntityInLine(Entity *my, double x1, double y1, double angle, int entities, Entity *target);
double lineTrace(Entity *my, double x1, double y1, double angle, double range, int entities, bool ground);
double lineTraceTarget(Entity *my, double x1, double y1, double angle, double range, int entities, bool ground, Entity *target); //If the linetrace function encounters the linetrace entity, it returns even if it's invisible or passable.
int checkObstacle(long x, long y, Entity *my, Entity *target);
