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
#include "player.hpp"
#include "prng.hpp"
#include "ui/GameUI.hpp"

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
#define GIB_PLAYER my->skill[11]
#define GIB_POOF my->skill[5]
#define GIB_LIGHTING my->skill[6]
#define GIB_DMG_MISS my->skill[7]

void poof(Entity* my) {
    if (GIB_POOF) {
        playSoundEntityLocal(my, 512, 128);
        for (int c = 0; c < 3; ++c) {
            const int x = my->x + local_rng.uniform(-4, 4);
            const int y = my->y + local_rng.uniform(-4, 4);
            const int z = my->z + local_rng.uniform(-4, -1);
            auto poof = spawnPoof(x, y, z, 0.3);
        }
    }
}

void actGib(Entity* my)
{
	my->flags[INVISIBLE] = !spawn_blood && !my->flags[SPRITE] && my->sprite == 5;

	// don't update gibs that have no velocity
	if ( my->z == 8 && fabs(GIB_VELX) < .01 && fabs(GIB_VELY) < .01 )
	{
	    poof(my);
		my->removeLightField();
		list_RemoveNode(my->mynode);
		return;
	}

	// remove gibs that have exceeded their life span
	if ( my->ticks > GIB_LIFESPAN && GIB_LIFESPAN )
	{
	    poof(my);
		my->removeLightField();
		list_RemoveNode(my->mynode);
		return;
	}

	if ( my->flags[OVERDRAW] 
		&& players[clientnum] && players[clientnum]->entity && players[clientnum]->entity->skill[3] == 1 )
	{
		// debug cam, don't draw overdrawn.
		my->flags[INVISIBLE] = true;
	}

	// horizontal motion
	my->yaw += sqrt(GIB_VELX * GIB_VELX + GIB_VELY * GIB_VELY) * .05;
	my->x += GIB_VELX;
	my->y += GIB_VELY;
	GIB_VELX = GIB_VELX * .95;
	GIB_VELY = GIB_VELY * .95;

	if ( GIB_LIGHTING )
	{
		my->removeLightField();
	}

	// gravity
	if ( my->z < 8 )
	{
		GIB_VELZ += GIB_GRAVITY;
		my->z += GIB_VELZ;
		my->roll += 0.1;

		if ( GIB_LIGHTING && my->flags[SPRITE] && my->sprite >= 180 && my->sprite <= 184 )
		{
			const char* lightname = nullptr;
			switch ( my->sprite )
			{
			case 180:
				lightname = "magic_spray_green_flicker";
				break;
			case 181:
				lightname = "magic_spray_blue_flicker";
				break;
			case 182:
				lightname = "magic_spray_orange_flicker";
				break;
			case 183:
				lightname = "magic_spray_purple_flicker";
				break;
			case 184:
				lightname = "magic_spray_white_flicker";
				break;
			default:
				break;
			}
			my->light = addLight(my->x / 16, my->y / 16, lightname);
		}
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
	    poof(my);
		my->removeLightField();
		list_RemoveNode(my->mynode);
		return;
	}
}

void actDamageGib(Entity* my)
{
	my->flags[INVISIBLE] = !spawn_blood && !my->flags[SPRITE] && my->sprite == 5;

	// don't update gibs that have no velocity
	if ( my->z >= 8 )
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

	if ( my->skill[3] == DMG_WEAKER || my->skill[3] == DMG_WEAKEST || my->skill[3] == DMG_MISS )
	{
		real_t scale = 0.2;
		if ( my->ticks > 10 )
		{
			scale *= 1.0 - 0.5 * (std::min((int)my->ticks - 10, 20)) / 20.0;
		}
		my->scalex = scale;
		my->scaley = scale;
		my->scalez = scale;
	}
	else if ( my->skill[3] == DMG_STRONGER
		|| my->skill[3] == DMG_STRONGEST
		|| my->skill[3] == DMG_MISS )
	{
		real_t scale = 0.2;
		auto& anim = EnemyHPDamageBarHandler::damageGibAnimCurves[DMG_DEFAULT];
		if ( my->ticks >= anim.size() )
		{
			scale *= anim[anim.size() - 1] / 100.0;
		}
		else
		{
			scale *= anim[my->ticks] / 100.0;
		}
		if ( my->skill[3] == DMG_MISS )
		{
			scale = 0.2;
		}
		my->scalex = scale;
		my->scaley = scale;
		my->scalez = scale;

		//GIB_VELX = GIB_VELX * .95;
		//GIB_VELY = GIB_VELY * .95;

		GIB_VELZ += GIB_GRAVITY / 2;
		if ( my->ticks < anim.size() )
		{
			if ( GIB_VELZ >= -.001 )
			{
				GIB_VELZ = -0.001;
			}
		}
		my->z += GIB_VELZ;

		if ( my->ticks > anim.size() + 10 )
		{
			list_RemoveNode(my->mynode);
			return;
		}

		return;
	}

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
}

/*-------------------------------------------------------------------------------

	spawnGib

	Spawns a gib with a random velocity for the entity supplied as an
	argument

-------------------------------------------------------------------------------*/

Entity* spawnGib(Entity* parentent, int customGibSprite)
{
	Entity* entity = nullptr;
	Stat* parentstats = nullptr;
	double vel;
	int gibsprite = 5;

	if ( !parentent )
	{
		return nullptr;
	}

	if ( (parentstats = parentent->getStats()) != nullptr )
	{
		if ( multiplayer == CLIENT )
		{
			printlog("[%s:%d spawnGib()] spawnGib() called on client, got clientstats. Probably bad?", __FILE__, __LINE__);
		}

		if ( customGibSprite != -1 )
		{
			gibsprite = customGibSprite;
		}
		else
		{
			switch ( gibtype[(int)parentstats->type] )
			{
				case 0:
					return nullptr;
				case 1:
					gibsprite = 5;
					break;
				case 2:
					gibsprite = 211;
					break;
				case 3:
				{
					std::string color = MonsterData_t::getKeyFromSprite(parentent->sprite, SLIME);
					if ( color == "slime green" )
					{
						// green blood
						gibsprite = 211;
					}
					else if ( color == "slime blue" )
					{
						// blue blood
						gibsprite = 215;
					}
					else if ( color == "slime red" )
					{
						// todo blood
						gibsprite = 1401;
					}
					else if ( color == "slime tar" )
					{
						// todo blood
						gibsprite = 1402;
					}
					else if ( color == "slime metal" )
					{
						// todo blood
						gibsprite = 1403;
					}
					break;
				}
				case 4:
					gibsprite = 683;
					break;
				case 5:
					if (parentstats->HP > 0) {
						return nullptr;
					}
					gibsprite = 688;
					break;
				//TODO: Gear gibs for automatons, and crystal gibs for golem.
				default:
					gibsprite = 5;
					break;
			}
		}
	}
	else if ( parentent->behavior == &actThrown )
	{
		if ( customGibSprite != -1 )
		{
			gibsprite = customGibSprite;
		}
	}

	entity = newEntity(gibsprite, 1, map.entities, nullptr); //Gib entity.
	if ( !entity )
	{
		return nullptr;
	}
	entity->x = parentent->x;
	entity->y = parentent->y;
	entity->z = local_rng.uniform(8, parentent->z - 4);
	entity->parent = parentent->getUID();
	entity->sizex = 2;
	entity->sizey = 2;
	entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
	entity->pitch = (local_rng.rand() % 360) * PI / 180.0;
	entity->roll = (local_rng.rand() % 360) * PI / 180.0;
	vel = (local_rng.rand() % 10) / 10.f;
	entity->vel_x = vel * cos(entity->yaw);
	entity->vel_y = vel * sin(entity->yaw);
	entity->vel_z = -.5;
	entity->fskill[3] = 0.04;
	entity->behavior = &actGib;
    entity->ditheringDisabled = true;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->flags[INVISIBLE] = !spawn_blood && !entity->flags[SPRITE] && entity->sprite == 5;
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);

	return entity;
}

Entity* spawnDamageGib(Entity* parentent, Sint32 dmgAmount, int gibDmgType, int displayType, bool updateClients)
{
	if ( !parentent )
	{
		return nullptr;
	}

	Entity* entity = newEntity(displayType == DamageGibDisplayType::DMG_GIB_SPRITE ? dmgAmount : -1, 1, map.entities, nullptr);
	if ( !entity )
	{
		return nullptr;
	}
	entity->x = parentent->x;
	entity->y = parentent->y;
	entity->z = parentent->z - 4;
	entity->parent = parentent->getUID();
	entity->sizex = 1;
	entity->sizey = 1;
	real_t vel = (local_rng.rand() % 10) / 20.f;
	entity->vel_z = -.5;
	if ( gibDmgType == DMG_STRONGER || gibDmgType == DMG_STRONGEST || gibDmgType == DMG_MISS )
	{
		vel = 0.25;
		entity->vel_z = -.4;
	}
	if ( parentent->isDamageableCollider() )
	{
		entity->z -= 4;
	}
	if ( parentent->sprite == 1475 ) // bell
	{
		entity->z += 8.0;
	}
	entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
	entity->vel_x = vel * cos(entity->yaw);
	entity->vel_y = vel * sin(entity->yaw);
	entity->scalex = 0.2;
	entity->scaley = 0.2;
	entity->scalez = 0.2;
	entity->skill[0] = dmgAmount;
	if ( displayType == DamageGibDisplayType::DMG_GIB_SPRITE )
	{
		entity->scalex = 0.05;
		entity->scaley = 0.05;
		entity->scalez = 0.05;
		entity->skill[0] = 0;
		entity->flags[BRIGHT] = true;
	}
	entity->skill[3] = gibDmgType;
	entity->fskill[3] = 0.04;
	entity->skill[7] = displayType;
	entity->behavior = &actDamageGib;
    entity->ditheringDisabled = true;
	entity->flags[SPRITE] = true;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->flags[INVISIBLE] = !spawn_blood && !entity->flags[SPRITE] && entity->sprite == 5;
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->skill[1] = -1;
	if ( parentent->behavior == &actPlayer )
	{
		entity->skill[1] = parentent->skill[2];
	}
	entity->setUID(-3);

	Uint32 color = makeColor(255, 255, 255, 255);
	switch ( (DamageGib)entity->skill[3] )
	{
		case DMG_DEFAULT:
		case DMG_WEAKER:
		case DMG_WEAKEST:
			break;
		case DMG_STRONGER:
			color = makeColor(238, 150, 75, 255);
			break;
		case DMG_STRONGEST:
			color = makeColor(235, 94, 0, 255);
			break;
		case DMG_FIRE:
			break;
		case DMG_BLEED:
			break;
		case DMG_POISON:
			break;
		case DMG_MISS:
			break;
		case DMG_HEAL:
			color = hudColors.characterSheetGreen;
			break;
		case DMG_TODO:
			break;
		default:
			break;
	}
	entity->skill[6] = color;

	if ( updateClients )
	{
		if ( multiplayer == SERVER )
		{
			for ( int c = 1; c < MAXPLAYERS; c++ )
			{
				if ( client_disconnected[c] || players[c]->isLocalPlayer() )
				{
					continue;
				}
				strcpy((char*)net_packet->data, "DMGG");
				SDLNet_Write32(parentent->getUID(), &net_packet->data[4]);
				SDLNet_Write16((Sint16)dmgAmount, &net_packet->data[8]);
				net_packet->data[10] = gibDmgType;
				net_packet->data[11] = displayType;
				net_packet->address.host = net_clients[c - 1].host;
				net_packet->address.port = net_clients[c - 1].port;
				net_packet->len = 12;
				sendPacketSafe(net_sock, -1, net_packet, c - 1);
			}
		}
	}

	return entity;
}

Entity* spawnGibClient(Sint16 x, Sint16 y, Sint16 z, Sint16 sprite)
{
	double vel;

	Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Gib entity.
	entity->x = x;
	entity->y = y;
	entity->z = z;
	entity->sizex = 2;
	entity->sizey = 2;
	entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
	entity->pitch = (local_rng.rand() % 360) * PI / 180.0;
	entity->roll = (local_rng.rand() % 360) * PI / 180.0;
	vel = (local_rng.rand() % 10) / 10.f;
	entity->vel_x = vel * cos(entity->yaw);
	entity->vel_y = vel * sin(entity->yaw);
	entity->vel_z = -.5;
	entity->fskill[3] = 0.04;
	entity->behavior = &actGib;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->flags[INVISIBLE] = !spawn_blood && !entity->flags[SPRITE] && entity->sprite == 5;

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
			if ( client_disconnected[c] || players[c]->isLocalPlayer() )
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
