/*-------------------------------------------------------------------------------

	BARONY
	File: actflame.cpp
	Desc: behavior function for flame particles

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "collision.hpp"
#include "entity.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define FLAME_LIFE my->skill[0]
#define FLAME_VELX my->vel_x
#define FLAME_VELY my->vel_y
#define FLAME_VELZ my->vel_z

void actFlame(Entity* my)
{
	if ( FLAME_LIFE <= 0 )
	{
		list_RemoveNode(my->mynode);
		return;
	}
	my->x += FLAME_VELX;
	my->y += FLAME_VELY;
	my->z += FLAME_VELZ;
	FLAME_LIFE--;
}

/*-------------------------------------------------------------------------------

	spawnFlame

	Spawns a flame particle for the entity supplied as an argument

-------------------------------------------------------------------------------*/

Entity* spawnFlame(Entity* parentent, Sint32 sprite )
{
	Entity* entity;
	double vel;

	entity = newEntity(sprite, 1, map.entities); // flame particle
	if ( intro )
	{
		entity->setUID(0);
	}
	entity->x = parentent->x;
	entity->y = parentent->y;
	entity->z = parentent->z;
	entity->sizex = 6;
	entity->sizey = 6;
	entity->yaw = (rand() % 360) * PI / 180.0;
	entity->pitch = (rand() % 360) * PI / 180.0;
	entity->roll = (rand() % 360) * PI / 180.0;
	vel = (rand() % 10) / 10.0;
	entity->vel_x = vel * cos(entity->yaw) * .1;
	entity->vel_y = vel * sin(entity->yaw) * .1;
	entity->vel_z = -.25;
	entity->skill[0] = 5;
	entity->flags[NOUPDATE] = true;
	entity->flags[PASSABLE] = true;
	entity->flags[SPRITE] = true;
	entity->flags[BRIGHT] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->behavior = &actFlame;
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);

	return entity;
}