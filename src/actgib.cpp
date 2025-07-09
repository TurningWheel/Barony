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
							if ( !entity->isInertMimic() )
							{
								entity->setEffect(EFF_MAGIC_GREASE, true, std::max(5 * TICKS_PER_SECOND, stats->EFFECTS_TIMERS[EFF_MAGIC_GREASE]),
									true);
							}
						}
						if ( entity->behavior == &actCampfire && my->skill[3] )
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