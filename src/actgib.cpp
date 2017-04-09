/*-------------------------------------------------------------------------------

	BARONY
	File: actgib.cpp
	Desc: behavior function for gibs

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "monster.hpp"
#include "entity.hpp"
#include "net.hpp"
#include "collision.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define GIB_VELX my->vel_x
#define GIB_VELY my->vel_y
#define GIB_VELZ my->vel_z
#define GIB_GRAVITY my->fskill[3]
#define GIB_LIFESPAN my->skill[4]

void actGib(Entity* my)
{
	// don't update gibs that have no velocity
	if ( my->z == 8 && fabs(GIB_VELX) < .01 && fabs(GIB_VELY) < .01 )
	{
		list_RemoveNode(my->mynode);
		return;
	}

	// remove gibs that have exceeded their life span
	if ( my->ticks > GIB_LIFESPAN && GIB_LIFESPAN )
	{
		list_RemoveNode(my->mynode);
		return;
	}

	// horizontal motion
	my->yaw += sqrt(GIB_VELX * GIB_VELX + GIB_VELY * GIB_VELY) * .05;
	my->x += GIB_VELX;
	my->y += GIB_VELY;
	GIB_VELX = GIB_VELX * .95;
	GIB_VELY = GIB_VELY * .95;

	// gravity
	if ( my->z < 8 )
	{
		GIB_VELZ += GIB_GRAVITY;
		my->z += GIB_VELZ;
		my->roll += 0.1;
	}
	else
	{
		if ( my->x >= 0 && my->y >= 0 && my->x < map.width << 4 && my->y < map.height << 4 )
		{
			if ( !map.tiles[(int)(floor(my->y / 16)*MAPLAYERS + floor(my->x / 16)*MAPLAYERS * map.height)] )
			{
				GIB_VELZ += GIB_GRAVITY;
				my->z += GIB_VELZ;
				my->roll += 0.1;
			}
			else
			{
				GIB_VELZ = 0;
				my->z = 8;
				my->roll = PI / 2.0;
			}
		}
		else
		{
			GIB_VELZ += GIB_GRAVITY;
			my->z += GIB_VELZ;
			my->roll += 0.1;
		}
	}

	// gibs disappear after falling to a certain point
	if ( my->z > 128 )
	{
		list_RemoveNode(my->mynode);
		return;
	}
}

/*-------------------------------------------------------------------------------

	spawnGib

	Spawns a gib with a random velocity for the entity supplied as an
	argument

-------------------------------------------------------------------------------*/

Entity* spawnGib(Entity* parentent)
{
	Entity* entity;
	Stat* parentstats;
	double vel;
	int gibsprite = 5;

	if ( parentent == NULL )
	{
		return NULL;
	}
	if ( (parentstats = parentent->getStats()) != NULL )
	{
		switch ( gibtype[(int)parentstats->type] )
		{
			case 0:
				return NULL;
			case 1:
				gibsprite = 5;
				break;
			case 2:
				gibsprite = 211;
				break;
			case 3:
				if ( parentent->sprite == 210 )
				{
					gibsprite = 211;
				}
				else
				{
					gibsprite = 215;
				}
				break;
			default:
				gibsprite = 5;
				break;
		}
	}

	entity = newEntity(gibsprite, 1, map.entities);
	entity->x = parentent->x;
	entity->y = parentent->y;
	entity->z = parentent->z;
	entity->parent = parentent->getUID();
	entity->sizex = 2;
	entity->sizey = 2;
	entity->yaw = (rand() % 360) * PI / 180.0;
	entity->pitch = (rand() % 360) * PI / 180.0;
	entity->roll = (rand() % 360) * PI / 180.0;
	vel = (rand() % 10) / 10.f;
	entity->vel_x = vel * cos(entity->yaw);
	entity->vel_y = vel * sin(entity->yaw);
	entity->vel_z = -.5;
	entity->fskill[3] = 0.04;
	entity->behavior = &actGib;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;
	if ( !spawn_blood )
	{
		entity->flags[INVISIBLE] = true;
	}
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);

	return entity;
}

Entity* spawnGibClient(Sint16 x, Sint16 y, Sint16 z, Sint16 sprite)
{
	double vel;

	Entity* entity = newEntity(sprite, 1, map.entities);
	entity->x = x;
	entity->y = y;
	entity->z = z;
	entity->sizex = 2;
	entity->sizey = 2;
	entity->yaw = (rand() % 360) * PI / 180.0;
	entity->pitch = (rand() % 360) * PI / 180.0;
	entity->roll = (rand() % 360) * PI / 180.0;
	vel = (rand() % 10) / 10.f;
	entity->vel_x = vel * cos(entity->yaw);
	entity->vel_y = vel * sin(entity->yaw);
	entity->vel_z = -.5;
	entity->fskill[3] = 0.04;
	entity->behavior = &actGib;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;

	return entity;
}

void serverSpawnGibForClient(Entity* gib)
{
	int c;
	if ( !gib )
	{
		return;
	}
	if ( multiplayer == SERVER )
	{
		for ( c = 1; c < MAXPLAYERS; c++ )
		{
			if ( client_disconnected[c] )
			{
				continue;
			}
			strcpy((char*)net_packet->data, "SPGB");
			SDLNet_Write16((Sint16)gib->x, &net_packet->data[4]);
			SDLNet_Write16((Sint16)gib->y, &net_packet->data[6]);
			SDLNet_Write16((Sint16)gib->z, &net_packet->data[8]);
			SDLNet_Write16((Sint16)gib->sprite, &net_packet->data[10]);
			net_packet->data[12] = gib->flags[SPRITE];
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 13;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}
}
