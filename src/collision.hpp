/*-------------------------------------------------------------------------------

	BARONY
	File: collision.hpp
	Desc: collision.cpp header file

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#define LINETRACE_IGNORE_ENTITIES 1
#define LINETRACE_ATK_CHECK_FRIENDLYFIRE 2

// function prototypes
real_t entityDist(Entity* my, Entity* your);
enum EntityClickType
{
	ENTITY_CLICK_USE,
	ENTITY_CLICK_USE_TOOLTIPS_ONLY,
	ENTITY_CLICK_HELD_USE_TOOLTIPS_ONLY,
	ENTITY_CLICK_FOLLOWER_INTERACT,
	ENTITY_CLICK_CALLOUT
};
Entity* entityClicked(bool* clickedOnGUI, bool clickCheckOverride, int player, EntityClickType clicktype);
bool entityInsideTile(Entity* entity, int x, int y, int z, bool checkSafeTiles = false);
bool entityInsideEntity(Entity* entity1, Entity* entity2);
bool entityInsideSomething(Entity* entity);
int barony_clear(real_t tx, real_t ty, Entity* my);
real_t clipMove(real_t* x, real_t* y, real_t vx, real_t vy, Entity* my);
Entity* findEntityInLine(Entity* my, real_t x1, real_t y1, real_t angle, int entities, Entity* target);
real_t lineTrace(Entity* my, real_t x1, real_t y1, real_t angle, real_t range, int entities, bool ground);
real_t lineTraceTarget(Entity* my, real_t x1, real_t y1, real_t angle, real_t range, int entities, bool ground, Entity* target); //If the linetrace function encounters the linetrace entity, it returns even if it's invisible or passable.
int checkObstacle(long x, long y, Entity* my, Entity* target, bool useTileEntityList = true, bool checkWalls = true, bool checkFloor = true);

struct MonsterTrapIgnoreEntities_t
{
	std::set<Uint32> ignoreEntities;
	Uint32 parent = 0;
};
extern std::map<Uint32, MonsterTrapIgnoreEntities_t> monsterTrapIgnoreEntities;
