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
real_t entityDist(Entity* my, Entity* your);
Entity* entityClicked();
bool entityInsideTile(Entity* entity, int x, int y, int z);
bool entityInsideEntity(Entity* entity1, Entity* entity2);
bool EntityInFrontOfEntity(Entity* entity1, Entity* entity2, Sint32 direction);
bool entityInsideSomething(Entity* entity);
int barony_clear(real_t tx, real_t ty, Entity* my);
real_t clipMove(real_t* x, real_t* y, real_t vx, real_t vy, Entity* my);
Entity* findEntityInLine(Entity* my, real_t x1, real_t y1, real_t angle, int entities, Entity* target);
real_t lineTrace(Entity* my, real_t x1, real_t y1, real_t angle, real_t range, int entities, bool ground);
real_t lineTraceTarget(Entity* my, real_t x1, real_t y1, real_t angle, real_t range, int entities, bool ground, Entity* target); //If the linetrace function encounters the linetrace entity, it returns even if it's invisible or passable.
int checkObstacle(long x, long y, Entity* my, Entity* target);
