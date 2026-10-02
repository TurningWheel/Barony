/*-------------------------------------------------------------------------------

	BARONY
	File: actsprite.cpp
	Desc: behavior function for sprite effects

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "engine/audio/sound.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "entity.hpp"
#include "interface/interface.hpp"
#include "draw.hpp"
#include "items.hpp"
#include "player.hpp"
#include "prng.hpp"
#include "files.hpp"
#include "monster.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define SPRITE_DESTROY my->skill[0]
#define SPRITE_FRAMES my->skill[1]
#define SPRITE_ANIMSPEED my->skill[2]
#define SPRITE_LIT my->skill[5]
#define SPRITE_ROTATE my->fskill[0]
#define SPRITE_CURRENT_ALPHA my->fskill[1]
#define SPRITE_ALPHA_VAR my->fskill[2]
#define SPRITE_ALPHA_ANIM_SIZE my->fskill[3]
#define SPRITE_CHECK_PARENT_EXISTS my->skill[8]

void actSprite(Entity* my)
{
	if ( SPRITE_CHECK_PARENT_EXISTS > 0 )
	{
		if ( !uidToEntity(SPRITE_CHECK_PARENT_EXISTS) )
		{
			if ( my->actSpriteUseCustomSurface != 0 )
			{
				if ( auto fx = AOEIndicators_t::getIndicator(my->actSpriteUseCustomSurface) )
				{
					fx->expired = true;
				}
			}

			my->removeLightField();
			list_RemoveNode(my->mynode);
			return;
		}
	}
	if ( my->actSpriteFollowUID > 0 )
	{
		if ( Entity* parent = uidToEntity(my->actSpriteFollowUID) )
		{
			my->x = parent->x;
			my->y = parent->y;
		}
	}

	if ( !my->actSpriteHasLightInit && SPRITE_LIT )
	{
		my->actSpriteHasLightInit = 1;
		my->light = addLight(my->x / 16, my->y / 16, "explosion");
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

	if ( SPRITE_ROTATE > 0.0001 )
	{
		my->yaw += SPRITE_ROTATE;
	}
	if ( my->actSpritePitchRotate > 0.0001 )
	{
		my->pitch += my->actSpritePitchRotate;
	}
	if ( SPRITE_ALPHA_VAR > 0.0001 )
	{
		SPRITE_CURRENT_ALPHA = SPRITE_ALPHA_VAR + SPRITE_ALPHA_ANIM_SIZE * sin(2 * PI * (my->ticks % TICKS_PER_SECOND) / (real_t)(TICKS_PER_SECOND));
	}
	if ( abs(my->vel_z) > 0.001 )
	{
		my->z += my->vel_z;
	}
	if ( my->actSpriteVelXY != 0 )
	{
		my->x += my->vel_x;
		my->y += my->vel_y;
	}
}

// mod add: Resolve the smoke model from the currently mounted models.txt so the
// source does not depend on a guessed mod-specific model index.
static Sint32 getMusketBlackSmokeModelIndex()
{
	static Sint32 modelIndex = -2;
	if ( modelIndex != -2 )
	{
		return modelIndex;
	}

	modelIndex = -1;
	const char* realDir = PHYSFS_getRealDir("models/models.txt");
	if ( realDir )
	{
		std::string modelsPath = realDir;
		modelsPath.append(PHYSFS_getDirSeparator()).append("models/models.txt");
		if ( File* fp = openDataFile(modelsPath.c_str(), "rb") )
		{
			char modelName[PATH_MAX];
			for ( Sint32 index = 0; !fp->eof(); ++index )
			{
				if ( !fp->gets2(modelName, PATH_MAX) )
				{
					break;
				}
				std::string normalizedPath = modelName;
				std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
				if ( normalizedPath == "models/particles/black_smoke.vox" )
				{
					modelIndex = index;
					break;
				}
			}
			FileIO::close(fp);
		}
	}

	if ( modelIndex < 0 || modelIndex >= static_cast<Sint32>(nummodels) )
	{
		modelIndex = -1;
		printlog("[MODELS]: models/particles/black_smoke.vox is not registered in the mounted models.txt; Musket smoke is disabled.");
	}
	return modelIndex;
}

static void actMusketSmokeParticle(Entity* my)
{
	static constexpr Sint32 RAPID_RISE_TICKS = 8;
	static constexpr Sint32 FADE_START_TICKS = 30;
	static constexpr Sint32 LIFETIME_TICKS = 70;

	my->x += my->vel_x;
	my->y += my->vel_y;
	my->z += my->vel_z;
	my->vel_x *= 0.94;
	my->vel_y *= 0.94;
	if ( my->ticks < RAPID_RISE_TICKS )
	{
		my->vel_z *= 0.74;
	}
	else
	{
		my->vel_z *= 0.98;
	}
	my->yaw += my->fskill[0];
	my->roll += my->fskill[1];

	if ( my->ticks >= FADE_START_TICKS )
	{
		my->scalex *= 0.95;
		my->scaley *= 0.95;
		my->scalez *= 0.95;
	}
	if ( my->ticks >= LIFETIME_TICKS || my->scalex <= 0.05 )
	{
		list_RemoveNode(my->mynode);
	}
}

static void spawnMusketBlackSmoke(Sint16 x, Sint16 y, Sint16 z)
{
	const Sint32 smokeModel = getMusketBlackSmokeModelIndex();
	if ( smokeModel < 0 )
	{
		return;
	}

	for ( Sint32 i = 0; i < 4; ++i )
	{
		Entity* smoke = newEntity(smokeModel, 1, map.entities, nullptr);
		smoke->x = x + (-8 + local_rng.rand() % 17) / 10.0;
		smoke->y = y + (-8 + local_rng.rand() % 17) / 10.0;
		smoke->z = z + (-3 + local_rng.rand() % 7) / 10.0;
		smoke->vel_x = (-12 + local_rng.rand() % 25) / 100.0;
		smoke->vel_y = (-12 + local_rng.rand() % 25) / 100.0;
		smoke->vel_z = -(0.85 + (local_rng.rand() % 26) / 100.0);
		const real_t scale = 0.55 + (local_rng.rand() % 16) / 100.0;
		smoke->scalex = scale;
		smoke->scaley = scale;
		smoke->scalez = scale;
		smoke->yaw = (local_rng.rand() % 360) * PI / 180.0;
		smoke->roll = (local_rng.rand() % 360) * PI / 180.0;
		smoke->fskill[0] = (-5 + local_rng.rand() % 11) / 500.0;
		smoke->fskill[1] = (-5 + local_rng.rand() % 11) / 500.0;
		smoke->sizex = 1;
		smoke->sizey = 1;
		smoke->ditheringDisabled = true;
		smoke->flags[PASSABLE] = true;
		smoke->flags[NOCLIP_CREATURES] = true;
		smoke->flags[UNCLICKABLE] = true;
		smoke->flags[NOUPDATE] = true;
		smoke->flags[UPDATENEEDED] = false;
		smoke->behavior = &actMusketSmokeParticle;
		if ( multiplayer != CLIENT )
		{
			--entity_uids;
		}
		smoke->setUID(-3);
	}
}
// mod add end

void actSpriteNametag(Entity* my)
{
	Entity* parent = uidToEntity(my->parent);
	if ( parent )
	{
        my->flags[INVISIBLE] = false;
		if ( hide_playertags )
		{
			if ( my->skill[3] == 0 )
			{
				my->skill[3] = 1; // keep track but don't draw for voice icons
			}
		}
		else
		{
			if ( my->skill[3] == 1 )
			{
				my->skill[3] = 0;
			}
		}
        my->x = parent->x;
        my->y = parent->y;
        my->z = parent->z - 6;
		if ( parent->getMonsterTypeFromSprite() == SLIME )
		{
			my->z -= 3.0;
			if ( parent->monsterAttack == MONSTER_POSE_MAGIC_WINDUP2 )
			{
				my->z += parent->focalz / 2;
			}
		}
	}
	else
	{
		my->flags[INVISIBLE] = true;
		list_RemoveNode(my->mynode);
	}
}

void actSpriteWorldTooltip(Entity* my)
{
	//list_RemoveNode(my->mynode);
	//return;
	Entity* parent = uidToEntity(my->parent);
	if ( parent )
	{
		my->x = parent->x;
		my->y = parent->y;

		if ( parent->behavior == &actDoor || parent->behavior == &actIronDoor )
		{
			if ( parent->flags[PASSABLE] )
			{
				if ( parent->doorStartAng == 0 )
				{
					my->y -= 5;
				}
				else
				{
					my->x -= 5;
				}
			}
		}
		else if ( parent->behavior == &actBell )
		{
			my->x += parent->focalx * cos(parent->yaw) + parent->focaly * cos(parent->yaw + PI / 2);
			my->y += parent->focalx * sin(parent->yaw) + parent->focaly * sin(parent->yaw + PI / 2);
		}

		bool inrange = (my->worldTooltipActive == 1);
		bool skipUpdating = true;
		if ( players[my->worldTooltipPlayer]->worldUI.bTooltipActiveForPlayer(*my) )
		{
			if ( players[my->worldTooltipPlayer]->worldUI.gimpDisplayTimer == 0 )
			{
				skipUpdating = false;
			}
		}

		if ( parent->flags[INVISIBLE] && !isAmuletFamiliarMimic(parent) )
		{
			skipUpdating = true;
		}

		if ( inrange && !skipUpdating )
		{
			my->worldTooltipAlpha = std::min(1.0, my->worldTooltipAlpha + .15);
			if ( my->worldTooltipInit == 0 )
			{
				my->worldTooltipInit = 1;
				my->worldTooltipZ = 1.5;
			}
			else
			{
				my->worldTooltipZ -= my->worldTooltipZ * 0.25;
				my->worldTooltipZ = std::max(0.1, my->worldTooltipZ);
			}

			if ( players[my->worldTooltipPlayer]->worldUI.bTooltipActiveForPlayer(*my) )
			{
				// in range of player
				my->worldTooltipFadeDelay = 25; // tick draw time, so when inrange = false it'll still be drawn as it fades
			}
			//messagePlayer(0, "%.2f", my->worldTooltipZ);
			if ( my->worldTooltipZ <= 0.15 ) // wait until animation before being able to be clicked on.
			{
				my->flags[UNCLICKABLE] = false;
			}
			my->flags[INVISIBLE] = false;
		}
		else
		{
			my->worldTooltipAlpha = std::max(0.0, my->worldTooltipAlpha - .12);
			if ( my->worldTooltipInit == 1 )
			{
				my->worldTooltipInit = 0;
				my->worldTooltipZ = 0.1;
			}
			else
			{
				my->worldTooltipZ -= 0.05;
				my->worldTooltipZ = std::max(-1.0, my->worldTooltipZ);
			}
		
			--my->worldTooltipFadeDelay; // decrement fade timer
			my->worldTooltipFadeDelay = std::max(0, my->worldTooltipFadeDelay);

			if ( my->worldTooltipAlpha <= 0.01 )
			{
				my->flags[INVISIBLE] = true;
			}
			my->flags[UNCLICKABLE] = true;
		}
		my->z = -.75 + std::max(0.0, parent->z - 7.75) - my->worldTooltipZ + Player::WorldUI_t::tooltipHeightOffsetZ;
		if ( parent->behavior == &actItem && parent->z < 4.0 )
		{
			if ( (multiplayer != CLIENT && parent->itemNotMoving)
				|| (multiplayer == CLIENT && parent->itemNotMovingClient) )
			{
				my->z -= 3;
			}
		}
	}
	else
	{
		my->flags[INVISIBLE] = true;
		// delete this entity, remove from any lists
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			int index = 0;
			bool bFound = false;
			for ( auto& tooltip : players[i]->worldUI.tooltipsInRange )
			{
				if ( tooltip.first == my )
				{
					bFound = true;
					break;
				}
				++index;
			}
			if ( bFound && index >= 0 && index < players[i]->worldUI.tooltipsInRange.size() )
			{
				if ( players[i]->worldUI.bTooltipActiveForPlayer(*my) && players[i]->worldUI.tooltipsInRange.size() > 1 )
				{
					players[i]->worldUI.cycleToNextTooltip();
				}
				players[i]->worldUI.tooltipsInRange.erase(players[i]->worldUI.tooltipsInRange.begin() + index);
			}
		}

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
			if ( client_disconnected[c] || players[c]->isLocalPlayer() )
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
    entity->ditheringDisabled = true;
	entity->flags[SPRITE] = true;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->flags[BRIGHT] = true;
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
			if ( client_disconnected[c] || players[c]->isLocalPlayer() )
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
    entity->ditheringDisabled = true;
	entity->flags[SPRITE] = true;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->behavior = &actSprite;
	entity->skill[0] = 1;
	entity->skill[1] = 4;
	entity->skill[2] = 4;
	Entity* my = entity;
	SPRITE_FRAMES = 10;
	SPRITE_ANIMSPEED = 2;
	SPRITE_LIT = 1;
	playSoundEntityLocal(entity, 153, 128);
	Entity* explosion = entity;
	for (i = 0; i < 10; ++i)
	{
		entity = newEntity(16, 1, map.entities, nullptr); //Sprite entity.
		entity->behavior = &actFlame;
		entity->x = explosion->x;
		entity->y = explosion->y;
		entity->z = explosion->z;
        entity->ditheringDisabled = true;
		entity->flags[SPRITE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[UPDATENEEDED] = false;
		entity->flags[PASSABLE] = true;
		//entity->scalex = 0.25f; //MAKE 'EM SMALL PLEASE!
		//entity->scaley = 0.25f;
		//entity->scalez = 0.25f;
		entity->vel_x = (-40 + local_rng.rand() % 81) / 8.f;
		entity->vel_y = (-40 + local_rng.rand() % 81) / 8.f;
		entity->vel_z = (-40 + local_rng.rand() % 81) / 8.f;
		entity->skill[0] = 15 + local_rng.rand() % 10;
	}
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);
	return explosion;
}

Entity* spawnFirearmMuzzleFlash(Sint16 x, Sint16 y, Sint16 z, real_t scale,
	bool spawnMusketSmoke)
{
	// mod add: Replicate only a short-lived sprite effect. Unlike spawnExplosion(),
	// this creates no sound, flames, damage, collision, or gameplay light.
	if ( multiplayer == SERVER )
	{
		for ( int c = 1; c < MAXPLAYERS; ++c )
		{
			if ( client_disconnected[c] || players[c]->isLocalPlayer() )
			{
				continue;
			}
			strcpy((char*)net_packet->data, "FMUZ");
			SDLNet_Write16(x, &net_packet->data[4]);
			SDLNet_Write16(y, &net_packet->data[6]);
			SDLNet_Write16(z, &net_packet->data[8]);
			SDLNet_Write16(static_cast<Uint16>(std::max<real_t>(0.0, scale) * 100.0), &net_packet->data[10]);
			net_packet->data[12] = spawnMusketSmoke ? 1 : 0;
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 13;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}

	Entity* entity = nullptr;
	//mod add: A zero scale reuses the Musket smoke replication without
	// manufacturing an invisible muzzle-flash entity for the quest awakening.
	if ( scale > 0.0 )
	{
		entity = newEntity(49, 1, map.entities, nullptr);
		entity->x = x;
		entity->y = y;
		entity->z = z;
		entity->scalex = scale;
		entity->scaley = scale;
		entity->scalez = scale;
		entity->ditheringDisabled = true;
		entity->flags[SPRITE] = true;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[UNCLICKABLE] = true;
		entity->flags[BRIGHT] = true;
		entity->behavior = &actSprite;
		entity->skill[0] = 1;
		entity->skill[1] = 10;
		entity->skill[2] = 1;
	}
	if ( spawnMusketSmoke )
	{
		spawnMusketBlackSmoke(x, y, z);
	}
	if ( entity && multiplayer != CLIENT )
	{
		entity_uids--;
	}
	if ( entity )
	{
		entity->setUID(-3);
	}
	return entity;
}

Entity* spawnExplosionFromSprite(Uint16 sprite, Sint16 x, Sint16 y, Sint16 z)
{
	if ( multiplayer == SERVER )
	{
		for ( int c = 1; c < MAXPLAYERS; c++ )
		{
			if ( client_disconnected[c] || players[c]->isLocalPlayer() )
			{
				continue;
			}
			strcpy((char*)net_packet->data, "EXPS");
			SDLNet_Write16(sprite, &net_packet->data[4]);
			SDLNet_Write16(x, &net_packet->data[6]);
			SDLNet_Write16(y, &net_packet->data[8]);
			SDLNet_Write16(z, &net_packet->data[10]);
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 12;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}

	// boom
	Entity* entity;
	if ( sprite == 0 )
	{
		entity = newEntity(-1, 1, map.entities, nullptr); //Sprite entity.
		entity->flags[INVISIBLE] = true;
	}
	else
	{
		entity = newEntity(sprite, 1, map.entities, nullptr); //Sprite entity.
	}
	entity->x = x;
	entity->y = y;
	entity->z = z;
    entity->ditheringDisabled = true;
	entity->flags[SPRITE] = true;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->behavior = &actSprite;
	entity->skill[0] = 1;
	entity->skill[1] = 4;
	entity->skill[2] = 4;
	Entity* my = entity;
	SPRITE_FRAMES = 10;
	SPRITE_ANIMSPEED = 2;
	SPRITE_LIT = 1;
	playSoundEntityLocal(entity, 153, 128);
	Entity* explosion = entity;
	for ( int i = 0; i < 10; ++i )
	{
		entity = newEntity(16, 1, map.entities, nullptr); //Sprite entity.
		entity->behavior = &actFlame;
		entity->x = explosion->x;
		entity->y = explosion->y;
		entity->z = explosion->z;
        entity->ditheringDisabled = true;
		entity->flags[SPRITE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[UPDATENEEDED] = false;
		entity->flags[PASSABLE] = true;
		//entity->scalex = 0.25f; //MAKE 'EM SMALL PLEASE!
		//entity->scaley = 0.25f;
		//entity->scalez = 0.25f;
		entity->vel_x = (-40 + local_rng.rand() % 81) / 8.f;
		entity->vel_y = (-40 + local_rng.rand() % 81) / 8.f;
		entity->vel_z = (-40 + local_rng.rand() % 81) / 8.f;
		entity->skill[0] = 15 + local_rng.rand() % 10;
	}
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);
	return explosion;
}

Entity* spawnPoof(Sint16 x, Sint16 y, Sint16 z, real_t scale, bool updateClients)
{
	// poof
	auto entity = newEntity(170, 1, map.entities, nullptr);
	entity->x = x;
	entity->y = y;
	entity->z = z;
	entity->scalex = scale;
	entity->scaley = scale;
	entity->scalez = scale;
    entity->ditheringDisabled = true;
	entity->flags[SPRITE] = true;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->behavior = &actSprite;
	Entity* my = entity;
	SPRITE_DESTROY = 1;
	SPRITE_FRAMES = 7;
	SPRITE_ANIMSPEED = 2;
	SPRITE_LIT = 0;
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);

	if ( updateClients && multiplayer == SERVER )
	{
		for ( int c = 1; c < MAXPLAYERS; c++ )
		{
			if ( client_disconnected[c] == true || players[c]->isLocalPlayer() )
			{
				continue;
			}
			strcpy((char*)net_packet->data, "PUFF");
			SDLNet_Write16(x, &net_packet->data[4]);
			SDLNet_Write16(y, &net_packet->data[6]);
			SDLNet_Write16(z, &net_packet->data[8]);
			SDLNet_Write16(static_cast<Uint16>(scale * 100), &net_packet->data[10]);
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 12;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}

	return entity;
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
	if ( my->z < -16 )
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
			if ( client_disconnected[c] || players[c]->isLocalPlayer() )
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
    entity->ditheringDisabled = true;
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

Entity* spawnFloatingSpriteMisc(int sprite, Sint16 x, Sint16 y, Sint16 z)
{
	int c;

	if ( multiplayer == SERVER )
	{
		for ( c = 1; c < MAXPLAYERS; c++ )
		{
			if ( client_disconnected[c] || players[c]->isLocalPlayer() )
			{
				continue;
			}
			strcpy((char*)net_packet->data, "SLEM");
			SDLNet_Write16(x, &net_packet->data[4]);
			SDLNet_Write16(y, &net_packet->data[6]);
			SDLNet_Write16(z, &net_packet->data[8]);
			SDLNet_Write16(sprite, &net_packet->data[10]);
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 12;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}

	Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Sprite entity.
	entity->behavior = &actSleepZ;
	entity->x = x;
	entity->y = y;
	entity->z = z;
    entity->ditheringDisabled = true;
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
