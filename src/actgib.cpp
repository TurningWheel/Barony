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
#define GIB_SHRINK my->fskill[4]
#define GIB_LIFESPAN my->skill[4]
#define GIB_PLAYER my->skill[11]
#define GIB_POOF my->skill[5]
#define GIB_LIGHTING my->skill[6]
#define GIB_DMG_MISS my->skill[7]
#define GIB_SWIRL my->fskill[5]
#define GIB_OSC_H my->fskill[6]
#define GIB_VEL_DECAY my->fskill[7]
#define GIB_DELAY_MOVE my->skill[8]
#define GIB_HIT_GROUND my->skill[9]
#define GIB_HIT_GROUND_EVENT my->skill[10]

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
		if ( GIB_SHRINK > 0.0001 )
		{
			my->scalex -= GIB_SHRINK;
			my->scaley -= GIB_SHRINK;
			my->scalez -= GIB_SHRINK;
			if ( my->scalex < 0.0 )
			{
				poof(my);
				my->removeLightField();
				list_RemoveNode(my->mynode);
				return;
			}
		}
		else
		{
			poof(my);
			my->removeLightField();
			list_RemoveNode(my->mynode);
			return;
		}
	}

	if ( my->flags[OVERDRAW] 
		&& players[clientnum] && players[clientnum]->entity && players[clientnum]->entity->skill[3] == 1 )
	{
		// debug cam, don't draw overdrawn.
		my->flags[INVISIBLE] = true;
	}

	// horizontal motion
	if ( GIB_DELAY_MOVE > 0 )
	{
		--GIB_DELAY_MOVE;
		if ( GIB_LIFESPAN )
		{
			++GIB_LIFESPAN;
		}
	}
	else
	{
		my->focalx = 0.0;
		my->focaly = 0.0;
		if ( GIB_SWIRL > 0.00001 )
		{
			my->yaw += GIB_SWIRL;
			Uint32 vortexStart = TICKS_PER_SECOND / 8;
			if ( my->ticks >= vortexStart )
			{
				real_t vortexTime = 0.5 * TICKS_PER_SECOND;
				real_t ratio = 2.0 + 2.0 * sin((-PI / 2) + (PI)*std::min((Uint32)vortexTime, my->ticks - vortexStart) / vortexTime);
				my->focalx += ratio * cos(my->yaw);
				my->focaly += ratio * sin(my->yaw);
			}
		}
		else
		{
			my->yaw += sqrt(GIB_VELX * GIB_VELX + GIB_VELY * GIB_VELY) * .05;
		}

		if ( abs(GIB_OSC_H) > 0.00001 )
		{
			Uint32 vortexStart = 0;// TICKS_PER_SECOND / 8;
			if ( my->ticks >= vortexStart )
			{
				//my->yaw = atan2(my->vel_y, my->vel_x);
				real_t vortexTime = 0.2 * TICKS_PER_SECOND;
				real_t magnitude = GIB_OSC_H;
				real_t ratio = magnitude * sin((PI)*((my->ticks - vortexStart) / vortexTime));
				real_t tangent = 0.0;// my->yaw - atan2(my->vel_y, my->vel_x);
				my->focalx += ratio * cos(tangent);
				my->focaly += ratio * sin(tangent);
			}
		}

		my->x += GIB_VELX;
		my->y += GIB_VELY;
		if ( GIB_VEL_DECAY > 0.001 )
		{
			GIB_VELX = GIB_VELX * GIB_VEL_DECAY;
			GIB_VELY = GIB_VELY * GIB_VEL_DECAY;
		}
		else
		{
			GIB_VELX = GIB_VELX * .95;
			GIB_VELY = GIB_VELY * .95;
		}
	}

	if ( GIB_LIGHTING )
	{
		my->removeLightField();
	}

	if ( my->actGibHitGroundEvent == 1 )
	{
		if ( my->sprite == 245 && my->flags[SPRITE] )
		{
			if ( my->ticks % 5 == 0 )
			{
				spawnGreasePuddleSpawner(my->parent == 0 ? nullptr : uidToEntity(my->parent), my->x, my->y, 30 * TICKS_PER_SECOND);
			}
			if ( GIB_HIT_GROUND == 0 )
			{
				if ( Entity* fx = spawnMagicParticleCustom(my, 245, 1.0, 1.0) )
				{
					fx->ditheringDisabled = true;
					real_t dir = atan2(GIB_VELY, GIB_VELX);
					//dir += local_rng.rand() % 2 == 0 ? PI / 32 : -PI / 32;
					real_t spd = sqrt(GIB_VELX * GIB_VELX + GIB_VELY * GIB_VELY);
					fx->vel_x = spd * 0.05 * cos(dir);
					fx->vel_y = spd * 0.05 * sin(dir);
					fx->flags[BRIGHT] = true;
					fx->pitch = PI / 2;
					fx->roll = 0.0;
					fx->yaw = dir;
					fx->vel_z = 0.0;
					fx->flags[SPRITE] = true;
				}
			}
		}
	}

	// gravity
	if ( my->z < 8 )
	{
		if ( GIB_DELAY_MOVE == 0 )
		{
			GIB_VELZ += GIB_GRAVITY;
			my->z += GIB_VELZ;
			if ( GIB_SWIRL > 0.00001 )
			{
				// don't roll swirling
			}
			else
			{
				my->roll += 0.1;
			}
		}

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
				if ( GIB_HIT_GROUND == 0 )
				{
					GIB_HIT_GROUND = 1;
					if ( my->sprite >= 1895 && my->sprite <= 1903 )
					{
						spawnMiscPuddle(my, my->x, my->y, my->sprite + 8);
						my->removeLightField();
						list_RemoveNode(my->mynode);
						return;
					}
				}
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

	if ( my->skill[3] == DMG_WEAKER || my->skill[3] == DMG_WEAKEST || my->skill[3] == DMG_MISS || my->skill[3] == DMG_GUARD )
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
		|| my->skill[3] == DMG_GUARD
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
		if ( my->skill[3] == DMG_MISS || my->skill[3] == DMG_GUARD )
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
	if ( gibDmgType == DMG_DETECT_MONSTER ) { return nullptr; }
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
	if ( gibDmgType == DMG_STRONGER || gibDmgType == DMG_STRONGEST || gibDmgType == DMG_MISS || gibDmgType == DMG_GUARD )
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
		case DMG_GUARD:
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
	if ( sprite >= 1871 && sprite <= 1876 ) // earth sprite
	{
		vel *= 0.1;
	}
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
			net_packet->data[12] = gib->flags[SPRITE] ? 1 << 0 : 0;
			net_packet->data[12] |= (gib->skill[5] == 1) ? 1 << 1 : 0; // poof
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 13;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}
}

void spawnGreasePuddleSpawner(Entity* caster, real_t x, real_t y, int duration)
{
	if ( multiplayer == CLIENT ) { return; }
	int ox = x / 16;
	int oy = y / 16;

	if ( ox >= 0 && ox < map.width && oy >= 0 && oy < map.height )
	{
		int mapIndex = oy * MAPLAYERS + ox * MAPLAYERS * map.height;
		if ( !map.tiles[mapIndex] )
		{
			return;
		}
		auto entLists = TileEntityList.getEntitiesWithinRadius(ox, oy, 0);
		for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
		{
			list_t* currentList = *it;
			for ( node_t* node = currentList->first; node != nullptr; node = node->next )
			{
				Entity* entity = (Entity*)node->element;
				if ( entity->behavior == &actGreasePuddleSpawner && entity->skill[0] > 0 )
				{
					entity->skill[0] = std::max(entity->skill[0], duration);
					return;
				}
			}
		}
		Entity* entity = newEntity(1786, 1, map.entities, nullptr); //Blood/gib entity.
		real_t x = ox * 16.0 + 8.0;
		real_t y = oy * 16.0 + 8.0;
		entity->x = x;
		entity->y = y;
		entity->z = 7.5;
		entity->parent = caster ? caster->getUID() : 0;
		entity->behavior = &actGreasePuddleSpawner;
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[0] = duration;
		entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
		entity->flags[UPDATENEEDED] = true;
		entity->flags[PASSABLE] = true;
		entity->flags[SPRITE] = true;
		entity->flags[UNCLICKABLE] = true;
		entity->flags[INVISIBLE] = true;
		entity->flags[BURNABLE] = true;
		TileEntityList.addEntity(*entity);
	}
}

void spawnGreasePuddle(Entity* parent, real_t x, real_t y, int duration, int location)
{
	if ( !parent ) { return; }
	int ox = x / 16;
	int oy = y / 16;
	if ( ox >= 0 && ox < map.width && oy >= 0 && oy < map.height )
	{
		Entity* entity = newEntity(1784, 1, map.entities, nullptr); //Blood/gib entity.
		real_t x = ox * 16.0 + 8.0;
		real_t y = oy * 16.0 + 8.0;

		static const std::vector<float> locations = {
			0 * PI / 4,
			5 * PI / 4,
			2 * PI / 4,
			3 * PI / 4,
			4 * PI / 4,
			7 * PI / 4,
			1 * PI / 4,
			6 * PI / 4,
		};

		x += 4.0 * cos(locations[location]) + 2.0 * (local_rng.rand() % 10) / 10.0;
		y += 4.0 * sin(locations[location]) + 2.0 * (local_rng.rand() % 10) / 10.0;

		entity->x = x;
		entity->y = y;
		entity->z = 8 + (local_rng.rand() % 20) / 100.0;
		entity->parent = 0;
		entity->sizex = 2;
		entity->sizey = 2;
		entity->behavior = &actGreasePuddle;
		entity->parent = parent->getUID();
		entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
		entity->flags[UPDATENEEDED] = false;
		entity->flags[NOUPDATE] = true;
		entity->flags[PASSABLE] = true;
		entity->flags[UNCLICKABLE] = true;
		if ( multiplayer != CLIENT )
		{
			--entity_uids;
		}
		entity->setUID(-3);
	}
}

void actGreasePuddle(Entity* my)
{
	if ( my->ticks % 10 == 0 )
	{
		if ( my->parent == 0 || !uidToEntity(my->parent) )
		{
			list_RemoveNode(my->mynode);
			return;
		}
	}
}

void actGreasePuddleSpawner(Entity* my)
{
	my->flags[INVISIBLE] = true;
	int x = my->x / 16;
	int y = my->y / 16;
	if ( multiplayer != CLIENT )
	{
		--my->skill[0];
		if ( my->skill[0] <= 0 )
		{
			if ( x >= 0 && x < map.width && y >= 0 && y < map.height )
			{
				bool foundGrease = false;
				auto entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 0);
				for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end() && !foundGrease; ++it )
				{
					list_t* currentList = *it;
					for ( node_t* node = currentList->first; node != nullptr; node = node->next )
					{
						Entity* entity = (Entity*)node->element;
						if ( entity && entity->behavior == &actGreasePuddleSpawner && entity != my )
						{
							int x2 = entity->x / 16;
							int y2 = entity->y / 16;
							if ( x2 == x && y2 == y )
							{
								foundGrease = true;
								break;
							}
						}
					}
				}

				if ( !foundGrease )
				{
					if ( map.tileHasAttribute(x, y, 0, map_t::TILE_ATTRIBUTE_GREASE) )
					{
						map.tileAttributes[0 + (y * MAPLAYERS) + (x * MAPLAYERS * map.height)] &= ~map_t::TILE_ATTRIBUTE_GREASE;
						serverUpdateMapTileFlag(x, y, 0, 0, map_t::TILE_ATTRIBUTE_GREASE);
					}
				}
			}
			my->removeLightField();
			list_RemoveNode(my->mynode);
			return;
		}
		if ( x >= 0 && x < map.width && y >= 0 && y < map.height )
		{
			if ( !map.tileHasAttribute(x, y, 0, map_t::TILE_ATTRIBUTE_GREASE) )
			{
				map.tileAttributes[0 + (y * MAPLAYERS) + (x * MAPLAYERS * map.height)] |= map_t::TILE_ATTRIBUTE_GREASE;
				serverUpdateMapTileFlag(x, y, 0, map_t::TILE_ATTRIBUTE_GREASE, 0);
			}

			int mapIndex = y * MAPLAYERS + x * MAPLAYERS * map.height;
			if ( lavatiles[map.tiles[mapIndex]] )
			{
				if ( !my->flags[BURNING] )
				{
					my->SetEntityOnFire();
				}
			}

			auto entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 0);
			Entity* parent = my->parent == 0 ? nullptr : uidToEntity(my->parent);
			for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
			{
				list_t* currentList = *it;
				for ( node_t* node = currentList->first; node != nullptr; node = node->next )
				{
					if ( Entity* entity = (Entity*)node->element )
					{
						if ( Stat* stats = entity->getStats() )
						{
							if ( entity->monsterIsTargetable() )
							{
								if ( !stats->getEffectActive(EFF_MAGIC_GREASE) || stats->EFFECTS_TIMERS[EFF_MAGIC_GREASE] < 1 * TICKS_PER_SECOND )
								{
									entity->setEffect(EFF_MAGIC_GREASE, true, std::max(5 * TICKS_PER_SECOND, stats->EFFECTS_TIMERS[EFF_MAGIC_GREASE]),
										true);
								}
							}
						}
						if ( (entity->behavior == &actCampfire && entity->skill[3] > 0 ) || entity->behavior == &actTorch )
						{
							my->SetEntityOnFire();
						}
						if ( my->flags[BURNING] )
						{
							if ( entity->flags[BURNABLE] && !entity->flags[BURNING] )
							{
								if ( Stat* stats = entity->getStats() )
								{
									if ( swimmingtiles[map.tiles[mapIndex]] && entity->behavior == &actPlayer && players[entity->skill[2]]->movement.isPlayerSwimming() )
									{
										continue;
									}
									if ( !entity->monsterIsTargetable(false) )
									{
										continue;
									}
									entity->SetEntityOnFire(parent);
									if ( parent && parent->getStats() )
									{
										if ( entity->flags[BURNING] )
										{
											stats->burningInflictedBy = parent->getUID();

											bool alertTarget = entity->monsterAlertBeforeHit(parent);

											// alert the monster!
											if ( entity->monsterState != MONSTER_STATE_ATTACK && (stats->type < LICH || stats->type >= SHOPKEEPER) )
											{
												if ( alertTarget )
												{
													entity->monsterAcquireAttackTarget(*parent, MONSTER_STATE_PATH, true);
												}
											}

											// alert other monsters too
											/*if ( alertTarget )
											{
												entity->alertAlliesOnBeingHit(parent);
											}*/
											entity->updateEntityOnHit(parent, alertTarget);
										}
									}
								}
								else
								{
									if ( entity->behavior == &actDoor
										|| entity->behavior == &::actIronDoor
										|| entity->behavior == &actBell
										|| entity->behavior == &::actFurniture || entity->behavior == &::actChest
										|| (entity->isDamageableCollider() && entity->isColliderDamageableByMagic()) )
									{
										entity->SetEntityOnFire(parent);
									}
								}
							}
						}
					}
				}
			}

			if ( my->flags[BURNING] )
			{
				++my->skill[6]; // burning spread counter
				if ( my->skill[6] >= TICKS_PER_SECOND )
				{
					my->skill[6] = 0;

					auto entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 1);
					for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
					{
						list_t* currentList = *it;
						for ( node_t* node = currentList->first; node != nullptr; node = node->next )
						{
							Entity* entity = (Entity*)node->element;
							if ( entity && entity->behavior == &actGreasePuddleSpawner && entity != my )
							{
								int x2 = entity->x / 16;
								int y2 = entity->y / 16;
								//if ( x2 == x || y2 == y ) // axis aligned only
								{
									if ( !entity->flags[BURNING] )
									{
										entity->flags[BURNING] = true;
										entity->skill[5] = TICKS_PER_SECOND * 5;
										serverUpdateEntityFlag(entity, BURNING);
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ( x >= 0 && x < map.width && y >= 0 && y < map.height )
	{
		if ( my->flags[BURNING] )
		{
			if ( !my->light )
			{
				my->light = addLight(my->x / 16, my->y / 16, "campfire");
			}
		}
		else
		{
			my->removeLightField();
		}
		if ( my->skill[1] < 4 )
		{
			if ( map.tileHasAttribute(x, y, 0, map_t::TILE_ATTRIBUTE_GREASE) )
			{
				++my->skill[1];
				std::vector<unsigned int> chances(8);
				std::fill(chances.begin(), chances.end(), 1);
				for ( int i = 0; i < 8; ++i )
				{
					if ( (my->skill[3] >> i) & 1 )
					{
						chances[i] = 0;
					}
				}
				int pick = local_rng.discrete(chances.data(), chances.size());
				my->skill[3] |= (1 << pick);
				spawnGreasePuddle(my, my->x, my->y, 10 * TICKS_PER_SECOND, pick);
			}
		}
	}
}

void actMiscPuddle(Entity* my)
{
	if ( !my ) { return; }

	if ( my->skill[0] <= 0 )
	{
		if ( my->scalex <= 0.0 )
		{
			list_RemoveNode(my->mynode);
			return;
		}

		my->scalex -= 0.05;
		my->scalez = my->scalex;
	}
	else
	{
		--my->skill[0];

		if ( my->scalex < my->fskill[0] )
		{
			real_t diff = std::max(0.01, (my->fskill[0] - my->scalex) / 10.0);
			my->scalex = std::min(my->scalex + diff, my->fskill[0]);
		}
		my->scalez = my->scalex;
	}
}

#define LEAF_IDLE_BOUNCE_TIME 50
void actLeafParticle(Entity* my)
{
	Entity* parent = nullptr;

	auto& particle_life = my->skill[0];
	auto& anim_bounce = my->skill[1];
	auto& anim_bounce_timer = my->skill[3];
	auto& float_oscillate_dir = my->skill[4];
	auto& anim_spin = my->skill[5];

	auto& float_oscillate_amt = my->fskill[0];
	auto& anim_bounce_fall = my->fskill[1];
	auto& pos_x_center = my->fskill[4];
	auto& pos_y_center = my->fskill[5];
	auto& pos_z_start = my->fskill[6];
	auto& pos_z_end = my->fskill[7];
	auto& anim_bounce_rise_amt = my->fskill[8];
	auto& rotation_offset = my->fskill[9];

	if ( my->parent != 0 )
	{
		parent = uidToEntity(my->parent);
		if ( !parent )
		{
			spawnPoof(my->x, my->y, my->z, 0.25);
			list_RemoveNode(my->mynode);
			return;
		}
	}
	else
	{
		if ( particle_life <= 0 )
		{
			spawnPoof(my->x, my->y, my->z, 0.25);
			list_RemoveNode(my->mynode);
			return;
		}
		--particle_life;
	}

	my->focalz = 2.0;

	bool grounded = false;
	if ( my->z >= 5.5 )
	{
		grounded = true;

		// reset to 0
		if ( float_oscillate_amt < 0.0 )
		{
			float_oscillate_amt += 0.025;
			float_oscillate_amt = std::min(float_oscillate_amt, 0.0);
		}
		else if ( float_oscillate_amt > 0.0 )
		{
			float_oscillate_amt -= 0.025;
			float_oscillate_amt = std::max(float_oscillate_amt, 0.0);
		}
	}
	else if ( float_oscillate_dir == 1 )
	{
		float_oscillate_amt += 0.025;
		if ( float_oscillate_amt >= 1.0 )
		{
			float_oscillate_amt = 1.0;
			if ( parent && parent->fskill[10] > 0.05 && (parent->skill[7] == 100 || parent->skill[8] >= 20) )
			{
				// spinning with at least 20 ticks left / strong spin, lock to 1 angle
			}
			else
			{
				float_oscillate_dir = 2;
			}
		}
	}
	else
	{
		float_oscillate_amt -= 0.025;
		if ( float_oscillate_amt <= -1.0 )
		{
			float_oscillate_amt = -1.0;
			if ( parent && parent->fskill[10] > 0.05 && (parent->skill[7] == 100 || parent->skill[8] >= 20) )
			{
				// spinning with at least 20 ticks left / strong spin, lock to 1 angle
			}
			else
			{
				float_oscillate_dir = 1;
			}
		}
	}

	if ( anim_bounce == 0 )
	{
		// rise up
		anim_bounce_fall += std::max(0.01, (1.0 - anim_bounce_fall) / std::max(12.0, anim_bounce_rise_amt));
		if ( anim_bounce_fall >= 1.0 )
		{
			anim_bounce_fall = 1.0;
			if ( anim_spin != 0 )
			{
				// don't fall down
			}
			else
			{
				anim_bounce = 1;
			}
		}
	}
	else
	{
		anim_bounce_fall -= std::max(0.01, (anim_bounce_fall) / 10.0);
		anim_bounce_fall = std::max(0.0, anim_bounce_fall);
	}

	real_t rate = (sin(float_oscillate_amt * PI / 2));
	my->roll = (PI / 4) * rate;
	/*if ( keystatus[SDLK_g] )
	{
		my->yaw += 0.05;
	}*/
	if ( parent )
	{
		pos_x_center = parent->x;
		pos_y_center = parent->y;
		my->yaw = parent->yaw + rotation_offset;

		if ( parent->fskill[10] > 0.25 )
		{
			if ( anim_spin == 0 )
			{
				anim_spin = 1;

				anim_bounce = 0;
				anim_bounce_fall = 0.0;
				pos_z_start = my->z;
				
				real_t boost = 4.0 + 0.25 * (local_rng.rand() % 9); // 4-6
				if ( parent->skill[7] != 100 )
				{
					anim_bounce_rise_amt = 12.0 + 0.25 * (local_rng.rand() % 13); // 12-15.0 random rise
					boost /= 3;
					pos_z_end = std::max(my->z - boost, -7.5) - pos_z_start;
				}
				else
				{
					anim_bounce_rise_amt = 12.0 + 0.25 * (local_rng.rand() % 13); // 12-15.0 random rise
					anim_bounce_rise_amt *= 10.0;
					real_t maxHeight = 0.0 + 5.0 * rotation_offset / (2 * PI);
					if ( my->z - boost > maxHeight )
					{
						pos_z_end = std::min(my->z - boost, maxHeight) - pos_z_start;
					}
					else
					{
						pos_z_end = std::max(my->z - boost, maxHeight) - pos_z_start;
					}
				}
			}
		}
		else
		{
			if ( anim_spin == 1 )
			{
				anim_spin = 0;
			}
		}
	}
	real_t faceDir = my->yaw;
	my->x = pos_x_center + 4.0 * cos(faceDir) - 2.0 * rate * cos(faceDir + PI / 2);
	my->y = pos_y_center + 4.0 * sin(faceDir) - 2.0 * rate * sin(faceDir + PI / 2);
	if ( anim_bounce == 0 )
	{
		// rise up
		my->z = pos_z_start + pos_z_end * sin(anim_bounce_fall * PI / 2);
	}
	else
	{
		// fall down faster as anim_bounce_fall 1 - 0
		my->vel_z = 0.1 * (1.0 - anim_bounce_fall) * abs(cos(float_oscillate_amt));
		my->z += my->vel_z;
	}
	my->z = std::min(my->z, 5.5);

	if ( !parent )
	{
		if ( multiplayer != CLIENT )
		{
			my->vel_x *= 0.8;
			my->vel_y *= 0.8;
			if ( abs(my->vel_x) > 0.01 || abs(my->vel_y) > 0.01 )
			{
				clipMove(&pos_x_center, &pos_y_center, my->vel_x, my->vel_y, my);
			}
			if ( anim_bounce_timer == 0 )
			{
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( players[i]->entity && entityInsideEntity(players[i]->entity, my) )
					{
						if ( abs(players[i]->entity->vel_x > 0.1) || abs(players[i]->entity->vel_y) > 0.1 )
						{
							anim_bounce_timer = LEAF_IDLE_BOUNCE_TIME;
							real_t tangent = atan2(my->y - players[i]->entity->y, my->x - players[i]->entity->x);
							my->vel_x = 1.5 * cos(tangent);
							my->vel_y = 1.5 * sin(tangent);
							float_oscillate_dir = 1 + local_rng.rand() % 2;
							anim_bounce = 0;
							anim_bounce_fall = 0.0;
							pos_z_start = my->z;
							anim_bounce_rise_amt = 12.0 + 0.25 * (local_rng.rand() % 13); // 12-15.0 random rise
							real_t boost = 4.0 + 0.25 * (local_rng.rand() % 9); // 4-6
							pos_z_end = std::max(my->z - boost, -7.5) - pos_z_start;
							break;
						}
					}
				}
			}
			else
			{
				--anim_bounce_timer;
			}
		}
	}
	else
	{
		if ( parent->skill[3] == LEAF_IDLE_BOUNCE_TIME - 1 )
		{
			anim_bounce = 0;
			float_oscillate_dir = 1 + local_rng.rand() % 2;
			anim_bounce_fall = 0.0;
			pos_z_start = my->z;
			real_t boost = 4.0 + 0.25 * (local_rng.rand() % 9); // 4-6
			anim_bounce_rise_amt = 12.0 + 0.25 * (local_rng.rand() % 13); // 12-15.0
			pos_z_end = std::max(my->z - boost, -7.5) - pos_z_start;
		}
	}
}

Entity* spawnLeafPile(real_t x, real_t y, bool trap)
{
	if ( multiplayer == CLIENT ) { return nullptr; }
	if ( Entity* leaf = newEntity(1913, 1, map.entities, nullptr) )
	{
		leaf->x = x;
		leaf->y = y;
		leaf->z = 0.0;
		leaf->yaw = map_rng.rand() % 360 * (PI / 180.0);
		leaf->sizex = 4;
		leaf->sizey = 4;
		leaf->behavior = &actLeafPile;
		leaf->skill[0] = 0;
		leaf->skill[10] = 0; // not map gen
		leaf->skill[11] = trap ? 0 : 1;
		leaf->flags[NOCLIP_CREATURES] = true;
		leaf->flags[UPDATENEEDED] = true;
		leaf->flags[NOUPDATE] = false;
		leaf->flags[PASSABLE] = true;
		leaf->flags[UNCLICKABLE] = true;

		int mapx = static_cast<int>(x) / 16;
		int mapy = static_cast<int>(y) / 16;
		int mapIndex = mapy * MAPLAYERS + mapx * MAPLAYERS * map.height;
		if ( mapx > 0 && mapx < map.width && mapy > 0 && mapy < map.height )
		{
			if ( !map.tiles[mapIndex] || swimmingtiles[map.tiles[mapIndex]] || lavatiles[map.tiles[mapIndex]]
				|| map.tiles[OBSTACLELAYER + mapIndex] )
			{
				leaf->skill[0] = 4.0 * TICKS_PER_SECOND; // lifetime on wrong terrain
			}
		}
		else
		{
			leaf->skill[0] = 4.0 * TICKS_PER_SECOND; // lifetime on wrong terrain
		}
		return leaf;
	}
	return nullptr;
}

void actLeafPile(Entity* my)
{
	if ( multiplayer != CLIENT )
	{
		if ( my->skill[0] > 0 )
		{
			--my->skill[0];
			if ( my->skill[0] <= 0 )
			{
				list_RemoveNode(my->mynode);
				return;
			}
		}
	}

	my->flags[INVISIBLE] = true;

	if ( my->skill[1] == 0 )
	{
		my->skill[1] = 1;

		real_t leafEndZ = -7.5;
		if ( my->skill[10] == 1 )
		{
			// map gen
			leafEndZ = 0.0 + local_rng.rand() % 3;
		}
		for ( int i = 0; i < 3; ++i )
		{
			Entity* leaf = newEntity(1912, 1, map.entities, nullptr); //Gib entity.
			if ( leaf != NULL )
			{
				leaf->x = my->x;
				leaf->y = my->y;
				leaf->z = 5.0 - i * 0.5;
				leaf->fskill[6] = leaf->z;
				leaf->fskill[7] = leafEndZ - leaf->fskill[6];
				leaf->vel_z = 0.0;
				leaf->yaw = my->yaw + i * 2 * PI / 3;
				leaf->sizex = 2;
				leaf->sizey = 2;
				leaf->scalex = 0.5;
				leaf->scaley = 0.5;
				leaf->scalez = 0.5;
				leaf->fskill[4] = my->x;
				leaf->fskill[5] = my->y;
				leaf->fskill[9] = i * 2 * PI / 3;
				leaf->parent = my->getUID();
				leaf->behavior = &actLeafParticle;
				leaf->flags[NOCLIP_CREATURES] = true;
				leaf->flags[UPDATENEEDED] = false;
				leaf->flags[NOUPDATE] = true;
				leaf->flags[PASSABLE] = true;
				leaf->flags[UNCLICKABLE] = true;
				if ( multiplayer != CLIENT )
				{
					--entity_uids;
				}
				leaf->setUID(-3);
			}
		}
	}

	auto& spinStrength = my->skill[7];
	auto& spinTimer = my->skill[8];

	if ( multiplayer != CLIENT )
	{
		if ( my->skill[3] == 0 )
		{
			std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 1);
			for ( auto it : entLists )
			{
				if ( my->skill[3] != 0 )
				{
					break;
				}
				for ( node_t* node = it->first; node != nullptr; node = node->next )
				{
					Entity* entity = (Entity*)node->element;
					if ( entity->behavior == &actMonster || entity->behavior == &actPlayer )
					{
						if ( !entity->monsterIsTargetable() ) { continue; }
						if ( abs(entity->vel_x > 0.1) || abs(entity->vel_y) > 0.1 )
						{
							if ( entityInsideEntity(entity, my) )
							{
								my->skill[3] = LEAF_IDLE_BOUNCE_TIME;
								my->skill[4] = 100;
								playSoundEntityLocal(my, 754 + local_rng.rand() % 2, 64);
								entity->setEffect(EFF_NOISE_VISIBILITY, (Uint8)2, 2 * TICKS_PER_SECOND, false);
								if ( multiplayer == SERVER )
								{
									for ( int c = 1; c < MAXPLAYERS; ++c ) // send to other players
									{
										if ( client_disconnected[c] || players[c]->isLocalPlayer() )
										{
											continue;
										}
										strcpy((char*)net_packet->data, "LEAF");
										SDLNet_Write32(my->getUID(), &net_packet->data[4]);
										net_packet->data[8] = 1;
										net_packet->data[9] = (Uint8)my->skill[3];
										net_packet->data[10] = (Uint8)my->skill[4];
										net_packet->address.host = net_clients[c - 1].host;
										net_packet->address.port = net_clients[c - 1].port;
										net_packet->len = 11;
										sendPacketSafe(net_sock, -1, net_packet, c - 1);
									}
								}
								break;
							}
						}
					}
				}
			}
		}

		if ( my->fskill[10] < 0.05 )
		{
			my->vel_x *= 0.95;
			my->vel_y *= 0.95;
		}
		else
		{
			my->vel_x *= 0.995;
			my->vel_y *= 0.995;
		}
		if ( abs(my->vel_x) > 0.01 || abs(my->vel_y) > 0.01 )
		{
			real_t result = clipMove(&my->x, &my->y, my->vel_x, my->vel_y, my);
			if ( result != sqrt(my->vel_x * my->vel_x + my->vel_y * my->vel_y) )
			{
				if ( spinStrength == 100 )
				{
					real_t bouncePenalty = 1.0;
					if ( hit.side == HORIZONTAL )
					{
						my->vel_x = -my->vel_x * bouncePenalty;
					}
					else if ( hit.side == VERTICAL )
					{
						my->vel_y = -my->vel_y * bouncePenalty;
					}
					else if ( hit.side == 0 )
					{
						my->vel_x = -my->vel_y * bouncePenalty;
						my->vel_y = -my->vel_x * bouncePenalty;
					}
				}
				else
				{
					my->vel_x = 0.f;
					my->vel_y = 0.f;
				}
			}
		}
	}

	if ( my->skill[3] > 0 )
	{
		--my->skill[3];
	}

	if ( spinStrength > 0 )
	{
		real_t amt = spinStrength / 100.0;

		my->fskill[10] += std::max(0.01, (amt - my->fskill[10]) / 10.0);
		my->fskill[10] = std::min(amt, my->fskill[10]);
		my->yaw = normaliseAngle2PI(my->yaw);

		if ( spinStrength == 100 )
		{
			my->skill[5]++;
			if ( my->skill[5] == 2 * TICKS_PER_SECOND && multiplayer != CLIENT )
			{
				CastSpellProps_t spellProps;
				spellProps.caster_x = my->x;
				spellProps.caster_y = my->y;
				spellProps.target_x = my->x;
				spellProps.target_y = my->y;
				castSpell(my->getUID(), getSpellFromID(SPELL_SLAM), false, true, false, &spellProps);

				std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 2);
				real_t dist = 10000.0;
				Entity* closestEntity = nullptr;
				for ( auto it : entLists )
				{
					for ( node_t* node = it->first; node != nullptr; node = node->next )
					{
						Entity* entity = (Entity*)node->element;
						if ( entity->behavior == &actMonster || entity->behavior == &actPlayer )
						{
							if ( !entity->monsterIsTargetable() ) { continue; }
							if ( Stat* entityStats = entity->getStats() )
							{
								if ( entityStats->type == MONSTER_M || entityStats->type == MONSTER_D )
								{
									continue;
								}
							}
							real_t newDist = entityDist(my, entity);
							if ( newDist < dist && newDist < 64.0 )
							{
								real_t tangent = atan2(entity->y - my->y, entity->x - my->x);
								real_t d = lineTraceTarget(my, my->x, my->y, tangent, 64.0, 0, false, entity);
								if ( hit.entity == entity )
								{
									closestEntity = entity;
									dist = newDist;
								}
							}
						}
					}
				}

				if ( closestEntity )
				{
					real_t tangent = atan2(closestEntity->y - my->y, closestEntity->x - my->x);
					my->vel_x = 0.75 * cos(tangent);
					my->vel_y = 0.75 * sin(tangent);
				}
			}
		}
	}
	else
	{
		my->skill[5] = 0;
		my->fskill[10] -= std::max(0.01, my->fskill[10] / 100.0);
		my->fskill[10] = std::max(0.0, my->fskill[10]);
	}

	my->yaw += 0.2 * (1 + sin(-PI / 2 + my->fskill[10] * PI / 2));
	my->yaw = normaliseAngle2PI(my->yaw);

	if ( my->skill[4] > 0 )
	{
		my->yaw += 0.025 * (my->skill[4]) / 100.0;
		my->yaw = normaliseAngle2PI(my->yaw);
		--my->skill[4];
	}

	if ( spinTimer > 0 )
	{
		--spinTimer;
		if ( spinTimer == 0 )
		{
			spinStrength = 0;
		}
	}

#ifdef USE_FMOD
	bool isPlaying = false;
	if ( my->entity_sound )
	{
		my->entity_sound->isPlaying(&isPlaying);
		if ( isPlaying )
		{
			FMOD_VECTOR position;
			position.x = (float)(my->x / (real_t)16.0);
			position.y = (float)(0.0);
			position.z = (float)(my->y / (real_t)16.0);
			my->entity_sound->set3DAttributes(&position, nullptr);
		}
	}
#endif

	if ( multiplayer != CLIENT )
	{
		int spinEvent = 0;
		if ( my->skill[6] == 0 )
		{
			my->skill[6] = TICKS_PER_SECOND * 5 + local_rng.rand() % (TICKS_PER_SECOND * 10);
		}
		else
		{
			if ( spinTimer == 0 )
			{
				--my->skill[6];
				if ( my->skill[6] == 0 )
				{
					if ( my->skill[11] == 0 ) // trapped
					{
						spinEvent = local_rng.rand() % 8 == 0 ? 2 : 1;
					}
					else
					{
						spinEvent = 1;
					}
#ifdef USE_FMOD
					bool isPlaying = false;
					if ( my->entity_sound )
					{
						my->entity_sound->isPlaying(&isPlaying);
					}
					if ( !isPlaying )
					{
						my->entity_sound = playSoundEntityLocal(my, 752 + local_rng.rand() % 2, 128);
					}
#endif
				}
			}
		}

		if ( /*keystatus[SDLK_g] ||*/ spinEvent == 2 )
		{
			spinStrength = 100;
			spinTimer = 225;

			if ( local_rng.rand() % 2 == 0 )
			{
				std::vector<real_t> dirs;
				for ( int i = 0; i < 4; ++i )
				{
					if ( !checkObstacle(my->x + 16.0 * cos(i * PI / 2), my->y + 16.0 * sin(i * PI / 2), my, nullptr) )
					{
						dirs.push_back(i * PI / 2);
					}
				}
				if ( dirs.size() > 0 )
				{
					real_t newDir = dirs[local_rng.rand() % dirs.size()];
					my->vel_x = 0.5 * cos(newDir);
					my->vel_y = 0.5 * sin(newDir);
				}
				else
				{
					real_t newDir = (local_rng.rand() % 4) * PI / 2;
					my->vel_x = 0.5 * cos(newDir);
					my->vel_y = 0.5 * sin(newDir);
				}
			}

			if ( multiplayer == SERVER )
			{
				for ( int c = 1; c < MAXPLAYERS; ++c ) // send to other players
				{
					if ( client_disconnected[c] || players[c]->isLocalPlayer() )
					{
						continue;
					}
					strcpy((char*)net_packet->data, "LEAF");
					SDLNet_Write32(my->getUID(), &net_packet->data[4]);
					net_packet->data[8] = 2;
					net_packet->data[9] = (Uint8)spinStrength;
					net_packet->data[10] = (Uint8)spinTimer;
					net_packet->address.host = net_clients[c - 1].host;
					net_packet->address.port = net_clients[c - 1].port;
					net_packet->len = 11;
					sendPacketSafe(net_sock, -1, net_packet, c - 1);
				}
			}
		}
		if ( /*keystatus[SDLK_h] ||*/ spinEvent == 1 )
		{
			//keystatus[SDLK_h] = 0;
			spinStrength = 75;
			spinTimer = 50;

			std::vector<real_t> dirs;
			for ( int i = 0; i < 4; ++i )
			{
				if ( !checkObstacle(my->x + 16.0 * cos(i * PI / 2), my->y + 16.0 * sin(i * PI / 2), my, nullptr) )
				{
					dirs.push_back(i * PI / 2);
				}
			}
			if ( dirs.size() > 0 )
			{
				real_t newDir = dirs[local_rng.rand() % dirs.size()];
				my->vel_x = 0.5 * cos(newDir);
				my->vel_y = 0.5 * sin(newDir);
			}
			else
			{
				real_t newDir = (local_rng.rand() % 4) * PI / 2;
				my->vel_x = 0.5 * cos(newDir);
				my->vel_y = 0.5 * sin(newDir);
			}

			if ( multiplayer == SERVER )
			{
				for ( int c = 1; c < MAXPLAYERS; ++c ) // send to other players
				{
					if ( client_disconnected[c] || players[c]->isLocalPlayer() )
					{
						continue;
					}
					strcpy((char*)net_packet->data, "LEAF");
					SDLNet_Write32(my->getUID(), &net_packet->data[4]);
					net_packet->data[8] = 2;
					net_packet->data[9] = (Uint8)spinStrength;
					net_packet->data[10] = (Uint8)spinTimer;
					net_packet->address.host = net_clients[c - 1].host;
					net_packet->address.port = net_clients[c - 1].port;
					net_packet->len = 11;
					sendPacketSafe(net_sock, -1, net_packet, c - 1);
				}
			}
		}
	}
}

Entity* spawnMiscPuddle(Entity* parentent, real_t x, real_t y, int sprite, bool updateClients)
{
	if ( sprite == 0 )
	{
		return nullptr;
	}
	if ( parentent )
	{
		x = parentent->x;
		y = parentent->y;
	}

	int mapx = static_cast<int>(x) / 16;
	int mapy = static_cast<int>(y) / 16;
	int mapIndex = mapy * MAPLAYERS + mapx * MAPLAYERS * map.height;
	if ( mapx > 0 && mapx < map.width && mapy > 0 && mapy < map.height )
	{
		if ( !map.tiles[mapIndex] || map.tiles[OBSTACLELAYER + mapIndex] )
		{
			return nullptr;
		}

		Entity* puddle = newEntity(sprite, 1, map.entities, nullptr); //Gib entity.
		if ( puddle != NULL )
		{
			puddle->x = x;
			puddle->y = y;
			puddle->z = 8.0 + (local_rng.rand() % 20) / 100.0;
			puddle->sizex = 2;
			puddle->sizey = 2;
			puddle->behavior = &actMiscPuddle;
			int randomScale = local_rng.rand() % 10;
			puddle->fskill[0] = (100 - randomScale) / 100.f; // end scale

			puddle->scalex = 0.0;
			puddle->scalez = puddle->scalex;
			puddle->skill[0] = TICKS_PER_SECOND * 3 + local_rng.rand() % (2 * TICKS_PER_SECOND);
			puddle->yaw = (local_rng.rand() % 360) * PI / 180.0;
			puddle->flags[UPDATENEEDED] = false;
			puddle->flags[NOUPDATE] = true;
			puddle->flags[PASSABLE] = true;
			puddle->flags[UNCLICKABLE] = true;
			if ( multiplayer != CLIENT )
			{
				--entity_uids;
			}
			puddle->setUID(-3);

			if ( updateClients && multiplayer == SERVER )
			{
				serverSpawnMiscParticlesAtLocation(x, y, 0, PARTICLE_EFFECT_MISC_PUDDLE, sprite);
			}
		}
		return puddle;
	}

	return nullptr;
}