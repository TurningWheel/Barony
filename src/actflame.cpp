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
#include "prng.hpp"
#include "player.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define FLAME_LIFE my->skill[0]
#define FLAME_VELX my->vel_x
#define FLAME_VELY my->vel_y
#define FLAME_VELZ my->vel_z
#define FLAME_ANG my->fskill[0]
#define FLAME_DIFFX my->fskill[1]
#define FLAME_DIFFY my->fskill[2]
#define FLAME_DIFFZ my->fskill[3]

void actFlame(Entity* my)
{
    if ( FLAME_LIFE > 0 )
    {
	    FLAME_LIFE--;
	    if ( FLAME_LIFE <= 0 )
	    {
		    list_RemoveNode(my->mynode);
		    return;
	    }
    }
    if ( !flickerLights &&
        (my->sprite == SPRITE_FLAME ||
        my->sprite == SPRITE_CRYSTALFLAME) )
    {
        FLAME_ANG += PI / TICKS_PER_SECOND * 2.0;
        if (FLAME_ANG > PI * 2.0)
        {
            FLAME_ANG -= PI * 2.0;
        }
        FLAME_VELZ = -sin(FLAME_ANG) * 0.02;
        Entity* parent = uidToEntity(my->parent);
        if ( parent )
        {
	        my->x += parent->x - FLAME_DIFFX;
	        my->y += parent->y - FLAME_DIFFY;
	        my->z += parent->z - FLAME_DIFFZ;
	        FLAME_DIFFX = parent->x;
	        FLAME_DIFFY = parent->y;
	        FLAME_DIFFZ = parent->z;
	        my->flags[INVISIBLE] = parent->flags[INVISIBLE];
        }
    }
	my->x += FLAME_VELX;
	my->y += FLAME_VELY;
	my->z += FLAME_VELZ;
}

/*-------------------------------------------------------------------------------

	spawnFlame

	Spawns a flame particle for the entity supplied as an argument

-------------------------------------------------------------------------------*/

static ConsoleVariable<bool> cvar_flame_use_vismap("/flame_use_vismap", true);

Entity* spawnFlame(Entity* parentent, Sint32 sprite )
{
	if ( !parentent )
	{
		return nullptr;
	}
	if ( *cvar_flame_use_vismap && !intro )
	{
		if ( parentent->behavior != actPlayer 
			&& parentent->behavior != actPlayerLimb
			&& !parentent->flags[OVERDRAW]
			&& !parentent->flags[GENIUS] )
		{
			int x = parentent->x / 16.0;
			int y = parentent->y / 16.0;
			if ( x >= 0 && x < map.width && y >= 0 && y < map.height )
			{
				bool anyVismap = false;
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( !client_disconnected[i] && players[i]->isLocalPlayer() )
					{
                        if ( cameras[i].vismap && cameras[i].vismap[y + x * map.height] )
                        {
                            anyVismap = true;
                            break;
                        }
					}
				}
				if ( !anyVismap )
				{
					return nullptr;
				}
			}
		}
	}

	double vel;
	Entity* entity = newEntity(sprite, 1, map.entities, nullptr); // flame particle
	if ( intro )
	{
		entity->setUID(0);
	}
	entity->parent = parentent->getUID();
	entity->x = parentent->x;
	entity->y = parentent->y;
	entity->z = parentent->z;
	entity->fskill[1] = parentent->x;
	entity->fskill[2] = parentent->y;
	entity->fskill[3] = parentent->z;
	entity->sizex = 6;
	entity->sizey = 6;
	entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
	entity->pitch = (local_rng.rand() % 360) * PI / 180.0;
	entity->roll = (local_rng.rand() % 360) * PI / 180.0;
	vel = (local_rng.rand() % 10) / 10.0;
	if (flickerLights)
	{
	    entity->skill[0] = 5; // life-span
	    entity->vel_x = vel * cos(entity->yaw) * .1;
	    entity->vel_y = vel * sin(entity->yaw) * .1;
	    entity->vel_z = -.25;
	}
	else
	{
	    entity->skill[0] = TICKS_PER_SECOND + 1;
	    entity->vel_x = 0.0;
	    entity->vel_y = 0.0;
	    entity->vel_z = 0.0;
	    entity->z -= 0.5;
	}
	entity->flags[NOUPDATE] = true;
	entity->flags[PASSABLE] = true;
	entity->flags[SPRITE] = true;
	entity->flags[UNCLICKABLE] = true;
	static ConsoleVariable<float> cvar_flameLightBonus("/flame_light_bonus", 0.5f);
	entity->lightBonus = vec4(*cvar_flameLightBonus, *cvar_flameLightBonus, *cvar_flameLightBonus, 0.f);
    entity->ditheringDisabled = true;
	entity->behavior = &actFlame;
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);

	return entity;
}
