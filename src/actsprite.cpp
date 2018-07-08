/*-------------------------------------------------------------------------------

	BARONY
	File: actsprite.cpp
	Desc: behavior function for sprite effects

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "sound.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "entity.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define SPRITE_DESTROY my->skill[0]
#define SPRITE_FRAMES my->skill[1]
#define SPRITE_ANIMSPEED my->skill[2]
#define SPRITE_LIT my->skill[5]

void actSprite(Entity* my)
{
	if ( !my->skill[6] && SPRITE_LIT )
	{
		my->skill[6] = 1;
		my->light = lightSphereShadow(my->x / 16, my->y / 16, SPRITE_LIT, 256);
	}
	else if ( !SPRITE_LIT )
	{
		my->light = NULL;
	}
	my->skill[3]++;
	if ( my->skill[3] >= SPRITE_ANIMSPEED )
	{
		my->skill[3] = 0;
		my->skill[4]++;
		my->sprite++;
		if ( my->skill[4] >= SPRITE_FRAMES )
		{
			my->skill[4] -= SPRITE_FRAMES;
			my->sprite -= SPRITE_FRAMES;
			if ( SPRITE_DESTROY )
			{
				my->removeLightField();
				list_RemoveNode(my->mynode);
				return;
			}
		}
	}
}

void actSpriteNametag(Entity* my)
{
	Entity* parent = uidToEntity(my->parent);
	if ( parent )
	{
		my->flags[INVISIBLE] = false;
		my->x = parent->x;
		my->y = parent->y;
	}
	else
	{
		my->flags[INVISIBLE] = true;
		list_RemoveNode(my->mynode);
	}
}

Entity* spawnBang(Sint16 x, Sint16 y, Sint16 z)
{
	int c;
	if ( multiplayer == SERVER )
	{
		for ( c = 1; c < MAXPLAYERS; c++ )
		{
			if ( client_disconnected[c] )
			{
				continue;
			}
			strcpy((char*)net_packet->data, "BANG");
			SDLNet_Write16(x, &net_packet->data[4]);
			SDLNet_Write16(y, &net_packet->data[6]);
			SDLNet_Write16(z, &net_packet->data[8]);
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 10;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}

	// bang
	Entity* entity = newEntity(23, 1, map.entities, nullptr); //Sprite entity.
	entity->x = x;
	entity->y = y;
	entity->z = z;
	entity->flags[SPRITE] = true;
	entity->flags[PASSABLE] = true;
	entity->flags[BRIGHT] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->behavior = &actSprite;
	entity->skill[0] = 1;
	entity->skill[1] = 4;
	entity->skill[2] = 4;
	playSoundEntityLocal(entity, 66, 64);
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);
	return entity;
}

Entity* spawnExplosion(Sint16 x, Sint16 y, Sint16 z)
{
	int c, i;
	if ( multiplayer == SERVER )
	{
		for ( c = 1; c < MAXPLAYERS; c++ )
		{
			if ( client_disconnected[c] )
			{
				continue;
			}
			strcpy((char*)net_packet->data, "EXPL");
			SDLNet_Write16(x, &net_packet->data[4]);
			SDLNet_Write16(y, &net_packet->data[6]);
			SDLNet_Write16(z, &net_packet->data[8]);
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 10;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}

	// boom
	Entity* entity = newEntity(49, 1, map.entities, nullptr); //Sprite entity.
	entity->x = x;
	entity->y = y;
	entity->z = z;
	entity->flags[SPRITE] = true;
	entity->flags[PASSABLE] = true;
	entity->flags[BRIGHT] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->behavior = &actSprite;
	entity->skill[0] = 1;
	entity->skill[1] = 4;
	entity->skill[2] = 4;
	Entity* my = entity;
	SPRITE_FRAMES = 10;
	SPRITE_ANIMSPEED = 2;
	SPRITE_LIT = 4;
	playSoundEntityLocal(entity, 153, 128);
	Entity* explosion = entity;
	for (i = 0; i < 10; ++i)
	{
		entity = newEntity(16, 1, map.entities, nullptr); //Sprite entity.
		entity->behavior = &actFlame;
		entity->x = explosion->x;
		entity->y = explosion->y;
		entity->z = explosion->z;
		entity->flags[SPRITE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[UPDATENEEDED] = false;
		entity->flags[BRIGHT] = true;
		entity->flags[PASSABLE] = true;
		//entity->scalex = 0.25f; //MAKE 'EM SMALL PLEASE!
		//entity->scaley = 0.25f;
		//entity->scalez = 0.25f;
		entity->vel_x = (-40 + rand() % 81) / 8.f;
		entity->vel_y = (-40 + rand() % 81) / 8.f;
		entity->vel_z = (-40 + rand() % 81) / 8.f;
		entity->skill[0] = 15 + rand() % 10;
	}
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);
	return explosion;
}

void actSleepZ(Entity* my)
{
	// spin around
	my->x -= my->fskill[0];
	my->y -= my->fskill[1];
	my->fskill[0] = cos((ticks % 360) * (PI / 180)) * my->scalex * 3;
	my->fskill[1] = sin((ticks % 360) * (PI / 180)) * my->scaley * 3;
	my->x += my->fskill[0];
	my->y += my->fskill[1];

	// go up
	my->z -= .2;
	if ( my->z < -128 )
	{
		list_RemoveNode(my->mynode);
		return;
	}

	// scale up
	my->scalex = fmin(my->scalex + 0.01, 0.75);
	my->scaley = fmin(my->scaley + 0.01, 0.75);
	my->scalez = fmin(my->scalez + 0.01, 0.75);
}

Entity* spawnSleepZ(Sint16 x, Sint16 y, Sint16 z)
{
	int c;

	if ( multiplayer == SERVER )
	{
		for ( c = 1; c < MAXPLAYERS; c++ )
		{
			if ( client_disconnected[c] )
			{
				continue;
			}
			strcpy((char*)net_packet->data, "SLEZ");
			SDLNet_Write16(x, &net_packet->data[4]);
			SDLNet_Write16(y, &net_packet->data[6]);
			SDLNet_Write16(z, &net_packet->data[8]);
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 10;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}

	Entity* entity = newEntity(47, 1, map.entities, nullptr); //Sprite entity.
	entity->behavior = &actSleepZ;
	entity->x = x;
	entity->y = y;
	entity->z = z;
	entity->flags[SPRITE] = true;
	entity->flags[PASSABLE] = true;
	entity->flags[UPDATENEEDED] = false;
	entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->scalex = 0.05;
	entity->scaley = 0.05;
	entity->scalez = 0.05;
	entity->sizex = 1;
	entity->sizey = 1;
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);

	return entity;
}
