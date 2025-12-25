/*-------------------------------------------------------------------------------

	BARONY
	File: actmagic.cpp
	Desc: behavior function for magic balls

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../entity.hpp"
#include "../interface/interface.hpp"
#include "../engine/audio/sound.hpp"
#include "../items.hpp"
#include "../monster.hpp"
#include "../net.hpp"
#include "../collision.hpp"
#include "../paths.hpp"
#include "../player.hpp"
#include "../scores.hpp"
#include "../prng.hpp"
#include "magic.hpp"
#include "../mod_tools.hpp"

static ConsoleVariable<float> cvar_magic_fx_light_bonus("/magic_fx_light_bonus", 0.25f);
static ConsoleVariable<bool> cvar_magic_fx_use_vismap("/magic_fx_use_vismap", true);

void spawnAdditionalParticleForMissile(Entity* my)
{
	if ( !my ) { return; }

	if ( my->ticks == 1 )
	{
		if ( my->sprite == 2191 )
		{
			if ( Entity* fx = createParticleAestheticOrbit(my, 2192, 4 * TICKS_PER_SECOND, PARTICLE_EFFECT_SCEPTER_BLAST_ORBIT1) )
			{
				fx->x = my->x;
				fx->y = my->y;
				fx->z = my->z;
				fx->yaw = my->yaw + PI / 4;
			}
			if ( Entity* fx = createParticleAestheticOrbit(my, 2193, 4 * TICKS_PER_SECOND, PARTICLE_EFFECT_SCEPTER_BLAST_ORBIT1) )
			{
				fx->x = my->x;
				fx->y = my->y;
				fx->z = my->z;
				fx->yaw = my->yaw - PI / 4;
			}
		}
		else if ( my->sprite == 2364 )
		{
			if ( Entity* fx = createParticleAestheticOrbit(my, 2365, 4 * TICKS_PER_SECOND, PARTICLE_EFFECT_BLOOD_WAVES_ORBIT) )
			{
				fx->x = my->x;
				fx->y = my->y;
				fx->z = my->z;
				fx->fskill[1] = 1.0;
			}
			if ( Entity* fx = createParticleAestheticOrbit(my, 2366, 4 * TICKS_PER_SECOND, PARTICLE_EFFECT_BLOOD_WAVES_ORBIT) )
			{
				fx->x = my->x;
				fx->y = my->y;
				fx->z = my->z;
				fx->fskill[1] = -1.0;
			}
		}
		else if ( my->sprite == 2407 )
		{
			if ( Entity* fx = createParticleAestheticOrbit(my, 2406, 4 * TICKS_PER_SECOND, PARTICLE_EFFECT_HOLY_BEAM_ORBIT) )
			{
				fx->x = my->x;
				fx->y = my->y;
				fx->z = my->z;
				fx->fskill[1] = 1.0;
			}
			if ( Entity* fx = createParticleAestheticOrbit(my, 2406, 4 * TICKS_PER_SECOND, PARTICLE_EFFECT_HOLY_BEAM_ORBIT) )
			{
				fx->x = my->x;
				fx->y = my->y;
				fx->z = my->z;
				fx->fskill[1] = -1.0;
			}
		}
		else if ( my->sprite == 2209 )
		{
			if ( Entity* fx = createParticleAestheticOrbit(my, 2210, 4 * TICKS_PER_SECOND, PARTICLE_EFFECT_SCEPTER_BLAST_ORBIT1) )
			{
				fx->x = my->x;
				fx->y = my->y;
				fx->z = my->z;
				fx->yaw = my->yaw + PI / 4;
			}
			if ( Entity* fx = createParticleAestheticOrbit(my, 2211, 4 * TICKS_PER_SECOND, PARTICLE_EFFECT_SCEPTER_BLAST_ORBIT1) )
			{
				fx->x = my->x;
				fx->y = my->y;
				fx->z = my->z;
				fx->yaw = my->yaw - PI / 4;
			}
			if ( multiplayer == SERVER )
			{
				if ( my->actmagicDelayMove > 2 * TICKS_PER_SECOND || (my->actmagicDelayMove == TICKS_PER_SECOND - 1) )
				{
					serverSpawnMiscParticlesAtLocation(my->x, my->y, my->z, PARTICLE_EFFECT_METEOR_STATIONARY_ORBIT, 2209, my->actmagicDelayMove, my->yaw * 256.0);
				}
			}
		}
	}
}

void spawnBasicMagicParticleForMissile(Entity* my)
{
	if ( !my ) { return; }

	if ( my->sprite == 2407 )
	{
		my->lightBonus = vec4{ *cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus, 0.f };
	}

	if ( *cvar_magic_fx_use_vismap && !intro )
	{
		int x = my->x / 16.0;
		int y = my->y / 16.0;
		if ( x >= 0 && x < map.width && y >= 0 && y < map.height )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( !client_disconnected[i] && players[i]->isLocalPlayer() && cameras[i].vismap[y + x * map.height] )
				{
					if ( my->sprite == 233 && my->flags[SPRITE] )
					{
						if ( my->sprite >= 233 && my->sprite <= 244 )
						{
							if ( my->ticks % 2 == 0 )
							{
								if ( Entity* fx = spawnMagicParticleCustom(my, my->sprite, 1.0, 1.0) )
								{
									fx->sprite = my->sprite;
									fx->flags[SPRITE] = my->flags[SPRITE];
									fx->ditheringDisabled = true;
									fx->fskill[1] = 0.025; // decay size
									fx->focalx = my->focalx;
									fx->focaly = my->focaly;
									fx->focalz = my->focalz;

									real_t dir = atan2(my->vel_y, my->vel_x);
									fx->x -= 2.0 * cos(dir);
									fx->y -= 2.0 * sin(dir);
								}
							}
						}
					}
					else if ( Entity* particle = spawnMagicParticle(my) )
					{
						particle->flags[SPRITE] = my->flags[SPRITE];
						particle->flags[INVISIBLE] = my->flags[INVISIBLE];
						particle->flags[INVISIBLE_DITHER] = my->flags[INVISIBLE_DITHER];
						if ( my->sprite == 2191 || my->sprite == 2209 )
						{
							particle->scalex = my->scalex * 0.5;
							particle->scaley = my->scaley * 0.5;
							particle->scalez = my->scalez * 0.5;
							particle->ditheringDisabled = true;
							particle->x -= 2.0 * cos(my->yaw);
							particle->y -= 2.0 * sin(my->yaw);
							particle->lightBonus = vec4_t{ 0.f, 0.f, 0.f, 0.f };
						}
					}
					break;
				}
			}
		}
	}
	else
	{
		if ( Entity* particle = spawnMagicParticle(my) )
		{
			particle->flags[SPRITE] = my->flags[SPRITE];
			particle->flags[INVISIBLE] = my->flags[INVISIBLE];
			particle->flags[INVISIBLE_DITHER] = my->flags[INVISIBLE_DITHER];
			if ( my->sprite == 2191 || my->sprite == 2209 )
			{
				particle->scalex = my->scalex * 0.5;
				particle->scaley = my->scaley * 0.5;
				particle->scalez = my->scalez * 0.5;
				particle->ditheringDisabled = true;
				particle->x -= 2.0 * cos(my->yaw);
				particle->y -= 2.0 * sin(my->yaw);
				particle->lightBonus = vec4_t{ 0.f, 0.f, 0.f, 0.f };
			}
		}
	}
}

const char* magicLightColorForSprite(Entity* my, int sprite, bool darker) {
	if ( my && my->flags[SPRITE] )
	{
		if ( darker )
		{
			switch ( sprite )
			{
			case 233:
			case 234:
			case 235:
			case 236:
			case 237:
			case 238:
			case 239:
			case 240:
			case 241:
			case 242:
			case 243:
			case 244:
				return "magic_foci_red";
			case 256:
				return "magic_foci_blue";
			default:
				break;
			}
		}
		else
		{
			switch ( sprite )
			{
			case 233:
			case 234:
			case 235:
			case 236:
			case 237:
			case 238:
			case 239:
			case 240:
			case 241:
			case 242:
			case 243:
			case 244:
				return "magic_foci_red_flicker";
			case 256:
				return "magic_foci_blue_flicker";
			default:
				break;
			}
		}
		return nullptr;
	}
    if (darker) {
        switch (sprite) {
        case 672:
		case 2209:
		case 2361:
        case 168: return "magic_red_flicker";
        case 169: return "magic_orange_flicker";
        case 670:
        case 170: return "magic_yellow_flicker";
        case 983:
        case 171: return "magic_green_flicker";
        case 592:
		case 1244:
        case 172:
		case 1801:
		case 1818: return "magic_blue_flicker";
        case 625:
		case 2191:
		case 2357:
        case 173: return "magic_purple_flicker";
        default:
        case 669:
        case 680:
		case 2356:
        case 174: return "magic_white_flicker";
        case 593:
        case 175: return "magic_black_flicker";
        case 678:
		case 1817:
		case 1886:
		case 2355:
		case 2364:
			return "magic_pink_flicker";
		case 1816: return "magic_green_flicker";
		case 2407:
		case 2153: return "magic_foci_yellow_flicker";
		case 2179:
		case 2180:
		case 2181:
		case 2182:
		case 2183:
		case 2154: return "magic_foci_purple_flicker";
		case 2156: return "magic_foci_brown_flicker";
        }
    } else {
        switch (sprite) {
        case 672:
		case 2209:
		case 2361:
        case 168: return "magic_red";
        case 169: return "magic_orange";
        case 670:
        case 170: return "magic_yellow";
        case 983:
        case 171: return "magic_green";
        case 592:
		case 1244:
        case 172:
		case 1801:
		case 1818: return "magic_blue";
        case 625:
		case 2191:
		case 2357:
        case 173: return "magic_purple";
        default:
        case 669:
        case 680:
		case 2356:
        case 174: return "magic_white";
        case 593:
        case 175: return "magic_black";
        case 678:
		case 1817:
		case 1886: 
		case 2355: 
		case 2364:
			return "magic_pink";
		case 1816: return "magic_green";
		case 2407:
		case 2153: return "magic_foci_yellow";
		case 2179:
		case 2180:
		case 2181:
		case 2182:
		case 2183:
		case 2154: return "magic_foci_purple";
		case 2156: return "magic_foci_brown";
        }
    }
	return nullptr;
}

void actMagiclightMoving(Entity* my)
{
	Entity* caster = NULL;
	if ( !my )
	{
		return;
	}

	my->removeLightField();
	if ( my->skill[0] <= 0 )
	{
		my->removeLightField();
		list_RemoveNode(my->mynode);
		return;
	}
	--my->skill[0];

	if ( my->skill[1] == 1 )
	{
		my->light = addLight(my->x / 16, my->y / 16, "magic_daedalus_reveal");
	}

	if ( my->sprite >= 0 )
	{
		if ( Entity* particle = spawnMagicParticle(my) )
		{
			particle->flags[INVISIBLE] = my->flags[INVISIBLE];
			particle->flags[INVISIBLE_DITHER] = my->flags[INVISIBLE_DITHER];
			if ( my->skill[3] == 1 )
			{
				particle->scalex = my->scalex * 0.8;
				particle->scaley = my->scaley * 0.8;
				particle->scalez = my->scalez * 0.8;
			}
			else
			{
				particle->x = my->x;
				particle->y = my->y;
				//particle->z = my->z;
			}
		}
	}

	if ( my->skill[3] == 1 )
	{
		my->roll += 0.3;
	}

	real_t dist = clipMove(&my->x, &my->y, my->vel_x, my->vel_y, my);
	if ( dist != sqrt(my->vel_x * my->vel_x + my->vel_y * my->vel_y) )
	{
		my->removeLightField();
		list_RemoveNode(my->mynode);
		return;
	}

	if ( my->parent != 0 )
	{
		if ( Entity* parent = uidToEntity(my->parent) )
		{
			if ( parent->behavior == &actDaedalusShrine && parent->skill[13] != 0 ) // shrine source
			{
				if ( Entity* exitEntity = uidToEntity(parent->skill[13]) )
				{
					if ( (int)(my->x / 16) == (int)(exitEntity->x / 16) )
					{
						if ( (int)(my->y / 16) == (int)(exitEntity->y / 16) )
						{
							my->removeLightField();
							list_RemoveNode(my->mynode);
							return;
						}
					}
				}
			}
		}
	}
}

void actMagiclightBall(Entity* my)
{
	if (!my)
	{
		return;
	}

	my->skill[2] = -10; // so the client sets the behavior of this entity
	const char* light = (my->sprite == 1800 || my->sprite == 1801) ? "magic_shade" : "magic_light";
	const char* light_flicker = (my->sprite == 1800 || my->sprite == 1801) ? "magic_shade_flicker" : "magic_light_flicker";
	auto& lightball_travelled_distance = my->fskill[3];
	/*if ( my->sprite == 174 )
	{
		my->ditheringOverride = 4;
	}*/
	if (clientnum != 0 && multiplayer == CLIENT)
	{
		my->removeLightField();

		//Light up the area.
		my->light = addLight(my->x / 16, my->y / 16, light);


		if ( flickerLights )
		{
			//Magic light ball will never flicker if this setting is disabled.
			lightball_flicker++;
		}

		if (lightball_flicker > 5)
		{
			lightball_lighting = (lightball_lighting == 1) + 1;

			if (lightball_lighting == 1)
			{
				my->removeLightField();
				my->light = addLight(my->x / 16, my->y / 16, light);
			}
			else
			{
				my->removeLightField();
				my->light = addLight(my->x / 16, my->y / 16, light_flicker);
			}
			lightball_flicker = 0;
		}

		lightball_timer--;
		const real_t diff = std::max(.01, (1.0 - my->fskill[0])) / 15.0;
		my->fskill[0] += diff;
		my->fskill[0] = std::min(1.0, my->fskill[0]);
		my->yaw += .01 + 0.5 * (1.0 - my->fskill[0]);
		if ( my->yaw >= PI * 2 )
		{
			my->yaw -= PI * 2;
		}
		my->new_yaw = my->yaw;
		return;
	}

	if ( magic_init )
	{
		const real_t diff = std::max(.01, (1.0 - my->fskill[0])) / 15.0;
		my->fskill[0] += diff;
		my->fskill[0] = std::min(1.0, my->fskill[0]);
		my->yaw += .01 + 0.5 * (1.0 - my->fskill[0]);
		if ( my->yaw >= PI * 2 )
		{
			my->yaw -= PI * 2;
		}
	}

	/*if (!my->parent) { //This means that it doesn't have a caster. In other words, magic light staff.
		return;
	})*/

	//TODO: Follow player around (at a distance -- and delay before starting to follow).
	//TODO: Circle around player's head if they stand still for a little bit. Continue circling even if the player walks away -- until the player is far enough to trigger move (or if the player moved for a bit and then stopped, then update position).
	//TODO: Don't forget to cast flickering light all around it.
	//TODO: Move out of creatures' way if they collide.

	/*if (!my->children) {
		list_RemoveNode(my->mynode); //Delete the light spell.
		return;
	}*/
	if (!my->children.first)
	{
		list_RemoveNode(my->mynode); //Delete the light spell.C
		return;
	}
	node_t* node = NULL;

	spell_t* spell = NULL;
	node = my->children.first;
	spell = (spell_t*)node->element;
	if (!spell)
	{
		list_RemoveNode(my->mynode);
		return; //We need the source spell!
	}

	Entity* caster = (spell->caster != 0 ? uidToEntity(spell->caster) : nullptr);
	if (caster)
	{
		Stat* stats = caster->getStats();
		if (stats)
		{
			if (stats->HP <= 0)
			{
				my->removeLightField();
				list_RemoveNode(my->mynode); //Delete the light spell.
				return;
			}
		}
	}
	else if ( spell->caster != 0 )
	{
		my->removeLightField();
		list_RemoveNode(my->mynode);
		return;
	}

	// if the spell has been unsustained, remove it
	if ( !spell->sustain )
	{
        if (!spell->magicstaff)
        {
            int player = -1;
            for (int i = 0; i < MAXPLAYERS; ++i)
            {
                if (players[i]->entity == caster)
                {
                    player = i;
                }
            }
            if (player > 0 && multiplayer == SERVER)
            {
                strcpy( (char*)net_packet->data, "UNCH");
                net_packet->data[4] = player;
                SDLNet_Write32(spell->ID, &net_packet->data[5]);
                net_packet->address.host = net_clients[player - 1].host;
                net_packet->address.port = net_clients[player - 1].port;
                net_packet->len = 9;
                sendPacketSafe(net_sock, -1, net_packet, player - 1);
            }
        }
		my->removeLightField();
		list_RemoveNode(my->mynode);
		return;
	}

	bool followParent = my->sprite == 174 || my->sprite == 1800 || (my->sprite == 1801 && my->parent != 0) || (my->sprite == 1802 && my->parent != 0);

	if (magic_init)
	{
		my->removeLightField();

		if (lightball_timer <= 0)
		{
			if ( my->sprite == 1801 || my->sprite == 1802 )
			{
				my->removeLightField();
				list_RemoveNode(my->mynode);
				return;
			}
			else if ( spell->sustain )
			{
				//Attempt to sustain the magic light.
				if (caster)
				{
					//Deduct mana from caster. Cancel spell if not enough mana (simply leave sustained at false).
					if ( spell->magicstaff )
					{
						lightball_timer = spell->channel_duration;
					}
					else if ( caster->safeConsumeMP(1) )
					{
						if ( caster->behavior == &actPlayer && !spell->magicstaff )
						{
							players[caster->skill[2]]->mechanics.sustainedSpellIncrementMP(1, spell->skillID);
						}
						lightball_timer = spell->channel_duration;
					}
					else
					{
						int i = 0;
						int player = -1;
						for (i = 0; i < MAXPLAYERS; ++i)
						{
							if (players[i]->entity == caster)
							{
								player = i;
							}
						}
						if (player > 0 && multiplayer == SERVER)
						{
							strcpy( (char*)net_packet->data, "UNCH");
							net_packet->data[4] = player;
							SDLNet_Write32(spell->ID, &net_packet->data[5]);
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 9;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}
						my->removeLightField();
						list_RemoveNode(my->mynode);
						return;
					}
				}
			}
		}

		//TODO: Make hovering always smooth. For example, when transitioning from ceiling to no ceiling, don't just have it jump to a new position. Figure out  away to transition between the two.
		if (lightball_hoverangle > 360)
		{
			lightball_hoverangle = 0;
		}

		if (map.tiles[(int)((my->y / 16) * MAPLAYERS + (my->x / 16) * MAPLAYERS * map.height)])
		{
			//Ceiling.
			my->z = lightball_hover_basez + ((lightball_hover_basez + LIGHTBALL_HOVER_HIGHPEAK + lightball_hover_basez + LIGHTBALL_HOVER_LOWPEAK) / 2) * sin(lightball_hoverangle * (12.568f / 360.0f)) * 0.1f;
		}
		else
		{
			//No ceiling. //TODO: Float higher?
			//my->z = lightball_hover_basez - 4 + ((lightball_hover_basez + LIGHTBALL_HOVER_HIGHPEAK - 4 + lightball_hover_basez + LIGHTBALL_HOVER_LOWPEAK - 4) / 2) * sin(lightball_hoverangle * (12.568f / 360.0f)) * 0.1f;
			my->z = lightball_hover_basez + ((lightball_hover_basez + LIGHTBALL_HOVER_HIGHPEAK + lightball_hover_basez + LIGHTBALL_HOVER_LOWPEAK) / 2) * sin(lightball_hoverangle * (12.568f / 360.0f)) * 0.1f;
		}

		my->fskill[2] = std::min(1.0, my->fskill[2] + 0.025);
		my->z -= my->fskill[1] - my->fskill[1] * sin(my->fskill[2] * PI / 2);

		lightball_hoverangle += 1;

		//Lightball moving.
		//messagePlayer(0, "*");

		if ( !followParent )
		{
			// stay still
		}
		else if ( followParent )
		{
			Entity* parent = uidToEntity(my->parent);
			if ( !parent )
			{
				return;
			}

			real_t follow_x = parent->x;
			real_t follow_y = parent->y;
			if ( spell && (spell->ID == SPELL_LIGHT || spell->ID == SPELL_DEEP_SHADE) && spell->caster == my->parent )
			{
				real_t vel = sqrt(pow(parent->vel_x, 2) + pow(parent->vel_y, 2));
				//if ( abs(vel) > 0.1 )
				{
					real_t dir = parent->yaw;// atan2(parent->vel_y, parent->vel_x);

					// draw line from the leaders direction until we hit a wall or 24 dist
					real_t startx = follow_x + 16.0 * cos(dir + PI / 4);
					real_t starty = follow_y + 16.0 * sin(dir + PI / 4);
					real_t previousx = startx;
					real_t previousy = starty;
					real_t followDist = spell->ID == SPELL_DEEP_SHADE ? 32.0 : 48.0;
					std::map<int, bool> checkedTiles;
					real_t furthestDist = 0.0;
					for ( int iterations = 0; iterations < 7; ++iterations )
					{
						startx += 8 * cos(dir);
						starty += 8 * sin(dir);
						if ( sqrt(pow(startx - follow_x, 2) + pow(starty - follow_y, 2)) > followDist )
						{
							break;
						}
						int mapx = (static_cast<int>(startx) >> 4);
						int mapy = (static_cast<int>(starty) >> 4);
						if ( !(mapx >= 0 && mapx < map.width && mapy >= 0 && mapy < map.height) )
						{
							continue;
						}
						int index = (mapy)*MAPLAYERS + (mapx)*MAPLAYERS * map.height;
						if ( !map.tiles[OBSTACLELAYER + index] )
						{
							bool foundObstacle = false;
							if ( checkedTiles.find(mapx + mapy * 10000) == checkedTiles.end() )
							{
								if ( checkObstacle((mapx << 4) + 8, (mapy << 4) + 8, my, nullptr, true, true, false, false) )
								{
									foundObstacle = true;
								}
								checkedTiles[mapx + mapy * 10000] = foundObstacle;
							}
							else
							{
								foundObstacle = checkedTiles[mapx + mapy * 10000];
							}
							if ( foundObstacle )
							{
								break;
							}

							// store the last known good coordinate
							real_t dist = sqrt(pow(follow_x - startx, 2) + pow(follow_y - starty, 2));
							if ( dist > furthestDist )
							{
								furthestDist = dist;
								previousx = startx;
								previousy = starty;
							}
						}
						else if ( map.tiles[OBSTACLELAYER + index] )
						{
							break;
						}
					}

					startx = follow_x; // retry straight line
					starty = follow_y;
					for ( int iterations = 0; iterations < 7; ++iterations )
					{
						startx += 8 * cos(dir);
						starty += 8 * sin(dir);
						if ( sqrt(pow(startx - follow_x, 2) + pow(starty - follow_y, 2)) > followDist )
						{
							break;
						}
						int mapx = (static_cast<int>(startx) >> 4);
						int mapy = (static_cast<int>(starty) >> 4);
						if ( !(mapx >= 0 && mapx < map.width && mapy >= 0 && mapy < map.height) )
						{
							continue;
						}
						int index = (mapy)*MAPLAYERS + (mapx)*MAPLAYERS * map.height;
						if ( !map.tiles[OBSTACLELAYER + index] )
						{
							bool foundObstacle = false;
							if ( checkedTiles.find(mapx + mapy * 10000) == checkedTiles.end() )
							{
								if ( checkObstacle((mapx << 4) + 8, (mapy << 4) + 8, my, nullptr, true, true, false, false) )
								{
									foundObstacle = true;
								}
								checkedTiles[mapx + mapy * 10000] = foundObstacle;
							}
							else
							{
								foundObstacle = checkedTiles[mapx + mapy * 10000];
							}
							if ( foundObstacle )
							{
								break;
							}

							// store the last known good coordinate
							real_t dist = sqrt(pow(follow_x - startx, 2) + pow(follow_y - starty, 2));
							dist -= 8.0;

							/*Entity* particle = spawnMagicParticle(my);
							particle->sprite = 576;
							particle->x = startx;
							particle->y = starty;
							particle->z = 7.5;*/

							if ( dist > furthestDist )
							{
								furthestDist = dist;
								previousx = startx;
								previousy = starty;
							}
						}
						else if ( map.tiles[OBSTACLELAYER + index] )
						{
							break;
						}
					}
					follow_x = previousx;
					follow_y = previousy;

					/*Entity* particle = spawnMagicParticle(my);
					particle->sprite = 942;
					particle->x = follow_x;
					particle->y = follow_y;
					particle->z = 7.5;*/

				}
			}

			double distance = sqrt(pow(my->x - follow_x, 2) + pow(my->y - follow_y, 2));
			if ( distance > MAGICLIGHT_BALL_FOLLOW_DISTANCE || my->path)
			{
				lightball_player_lastmove_timer = 0;
				if (lightball_movement_timer > 0)
				{
					lightball_movement_timer--;
				}
				else
				{
					//messagePlayer(0, "****Moving.");
					double tangent = atan2(follow_y - my->y, follow_x - my->x);
					//lineTraceTarget(my, my->x, my->y, tangent, 1024, LINETRACE_IGNORE_ENTITIES, false, parent);
					if ( true/*!hit.entity || hit.entity == parent*/ )   //Line of sight to caster?
					{
						if (my->path != NULL)
						{
							list_FreeAll(my->path);
							my->path = NULL;
						}
						
						my->vel_x = cos(tangent) * ((distance - MAGICLIGHT_BALL_FOLLOW_DISTANCE) / MAGICLIGHTBALL_DIVIDE_CONSTANT);
						my->vel_y = sin(tangent) * ((distance - MAGICLIGHT_BALL_FOLLOW_DISTANCE) / MAGICLIGHTBALL_DIVIDE_CONSTANT);
						real_t xMove = (my->vel_x < MAGIC_LIGHTBALL_SPEEDLIMIT) ? my->vel_x : MAGIC_LIGHTBALL_SPEEDLIMIT;
						real_t yMove = (my->vel_y < MAGIC_LIGHTBALL_SPEEDLIMIT) ? my->vel_y : MAGIC_LIGHTBALL_SPEEDLIMIT;
						my->x += xMove;
						my->y += yMove;

						if ( spell )
						{
							if ( caster )
							{
								if ( lightball_travelled_distance >= 0 )
								{
									real_t dist = sqrt(pow(xMove, 2) + pow(yMove, 2));
									if ( dist > 0.05 )
									{
										lightball_travelled_distance += abs(dist);
										if ( lightball_travelled_distance >= 16 * 16.0 )
										{
											lightball_travelled_distance = 0.0;
											if ( spell->magicstaff )
											{
												Uint32 flags = spell_t::SPELL_LEVEL_EVENT_SUSTAIN;
												flags |= spell_t::SPELL_LEVEL_EVENT_MAGICSTAFF;
												if ( magicOnSpellCastEvent(caster, my, nullptr, spell->ID, 
													flags, 1) )
												{
													lightball_travelled_distance = -1.0;
												}
											}
											else
											{
												if ( caster->behavior == &actPlayer )
												{
													if ( players[caster->skill[2]]->mechanics.updateSustainedSpellEvent(spell->ID, 150.0, 1.0) )
													{
														//lightball_travelled_distance = -1.0;
													}
												}
											}
										}
									}
								}
							}
						}
						
						if ( map.tiles[(int)(OBSTACLELAYER + static_cast<int>(my->y / 16) * MAPLAYERS + static_cast<int>(my->x / 16) * MAPLAYERS * map.height)] )   //If the ball has come to rest in a wall, move its butt.
						{
							double tangent = atan2(parent->y - my->y, parent->x - my->x);
							my->vel_x = cos(tangent) * ((distance) / MAGICLIGHTBALL_DIVIDE_CONSTANT);
							my->vel_y = sin(tangent) * ((distance) / MAGICLIGHTBALL_DIVIDE_CONSTANT);
							my->x += my->vel_x;
							my->y += my->vel_y;
						}
					}
				}
			}
			else
			{
				lightball_movement_timer = 0;// LIGHTBALL_MOVE_DELAY;
				if (map.tiles[(int)(OBSTACLELAYER + static_cast<int>(my->y / 16) * MAPLAYERS + static_cast<int>(my->x / 16) * MAPLAYERS * map.height)])   //If the ball has come to rest in a wall, move its butt.
				{
					double tangent = atan2(parent->y - my->y, parent->x - my->x);
					my->vel_x = cos(tangent) * ((distance) / MAGICLIGHTBALL_DIVIDE_CONSTANT);
					my->vel_y = sin(tangent) * ((distance) / MAGICLIGHTBALL_DIVIDE_CONSTANT);
					my->x += my->vel_x;
					my->y += my->vel_y;
				}
			}
		}

		//Light up the area.
		my->light = addLight(my->x / 16, my->y / 16, light);

		if ( flickerLights )
		{
			//Magic light ball  will never flicker if this setting is disabled.
			lightball_flicker++;
		}

		if (lightball_flicker > 5)
		{
			lightball_lighting = (lightball_lighting == 1) + 1;

			if (lightball_lighting == 1)
			{
				my->removeLightField();
				my->light = addLight(my->x / 16, my->y / 16, light);
			}
			else
			{
				my->removeLightField();
				my->light = addLight(my->x / 16, my->y / 16, light_flicker);
			}
			lightball_flicker = 0;
		}

		lightball_timer--;
	}
	else
	{
		//Init the lightball. That is, shoot out from the player.

		//Move out from the player.
		if ( followParent )
		{
			/*my->vel_x = cos(my->yaw) * 4;
			my->vel_y = sin(my->yaw) * 4;
			double dist = clipMove(&my->x, &my->y, my->vel_x, my->vel_y, my);

			unsigned int distance = sqrt(pow(my->x - lightball_player_startx, 2) + pow(my->y - lightball_player_starty, 2));
			if (distance > MAGICLIGHT_BALL_FOLLOW_DISTANCE * 2 || dist != sqrt(my->vel_x * my->vel_x + my->vel_y * my->vel_y))
			{
			}*/
			magic_init = 1;
			lightball_lighting = 1;
			lightball_movement_timer = 0; //Start off at 0 so that it moves towards the player as soon as it's created (since it's created farther away from the player).
		}
		else
		{
			magic_init = 1;
			lightball_lighting = 1;
			lightball_movement_timer = 0;
		}
	}
}

void spawnBloodVialOnMonsterDeath(Entity* entity, Stat* hitstats, Entity* killer)
{
	if ( !entity || !hitstats ) { return; }
	if ( entity->behavior == &actMonster )
	{
		bool tryBloodVial = false;
		if ( gibtype[hitstats->type] == 1 || gibtype[hitstats->type] == 2 )
		{
			for ( int c = 0; c < MAXPLAYERS; ++c )
			{
				if ( playerRequiresBloodToSustain(c) )
				{
					tryBloodVial = true;
					break;
				}
			}
			if ( tryBloodVial )
			{
				bool spawnBloodVial = false;
				if ( hitstats->getEffectActive(EFF_BLEEDING) )
				{
					if ( hitstats->EFFECTS_TIMERS[EFF_BLEEDING] >= 250 )
					{
						spawnBloodVial = (local_rng.rand() % 2 == 0);
					}
					else if ( hitstats->EFFECTS_TIMERS[EFF_BLEEDING] >= 150 )
					{
						spawnBloodVial = (local_rng.rand() % 4 == 0);
					}
					else
					{
						spawnBloodVial = (local_rng.rand() % 8 == 0);
					}
				}
				else
				{
					spawnBloodVial = (local_rng.rand() % 10 == 0);
				}
				if ( spawnBloodVial )
				{
					Item* blood = newItem(FOOD_BLOOD, EXCELLENT, 0, 1, gibtype[hitstats->type] - 1, true, &hitstats->inventory);
				}
			}
		}

		if ( killer && (killer->behavior == &actMonster || killer->behavior == &actPlayer) )
		{
			if ( Stat* killerStats = killer->getStats() )
			{
				if ( killerStats->helmet && killerStats->helmet->type == HAT_CHEF )
				{
					if ( gibtype[hitstats->type] == 1 )
					{
						int chance = 20;
						bool cursedChef = false;
						if ( killerStats->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(killerStats) )
						{
							chance -= 5 * abs(killerStats->helmet->beatitude);
							chance = std::max(10, chance);
						}
						else
						{
							chance -= 5 * abs(killerStats->helmet->beatitude);
							chance = std::max(10, chance);
							cursedChef = true;
						}
						if ( local_rng.rand() % chance == 0 )
						{
							Item* meat = newItem(FOOD_MEAT, (Status)(DECREPIT + local_rng.rand() % 4),
								0, 1, gibtype[hitstats->type], false, &hitstats->inventory);
							if ( cursedChef )
							{
								meat->status = DECREPIT;
								meat->beatitude = -(local_rng.rand() % 3);
							}
						}
					}
				}
			}
		}
	}
}

bool magicOnSpellCastEvent(Entity* parent, Entity* projectile, Entity* hitentity, int spellID, Uint32 eventType, int eventValue, bool allowedLevelup)
{
	if ( !parent )
	{
		return false;
	}

	if ( parent->behavior != &actPlayer )
	{
		return false;
	}

	if ( multiplayer == CLIENT )
	{
		if ( players[clientnum]->entity && players[clientnum]->entity == parent )
		{
			if ( projectile == parent || !projectile )
			{
				// request to server
				strcpy((char*)net_packet->data, "SPLV");
				net_packet->data[4] = clientnum;
				SDLNet_Write16(spellID, &net_packet->data[5]);
				SDLNet_Write32(eventType, &net_packet->data[7]);
				SDLNet_Write32(eventValue, &net_packet->data[11]);

				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 15;
				sendPacketSafe(net_sock, -1, net_packet, 0);
			}
		}
		return false;
	}

	if ( spellID <= SPELL_NONE )
	{
		return false;
	}

	bool magicstaff = eventType & spell_t::SPELL_LEVEL_EVENT_MAGICSTAFF;
	bool spellbook = eventType & spell_t::SPELL_LEVEL_EVENT_SPELLBOOK;

	if ( projectile && projectile->behavior == &actMagicMissile )
	{
		magicstaff = projectile->actmagicCastByMagicstaff == 1;
		spellbook = projectile->actmagicFromSpellbook == 1;

		if ( projectile->actmagicSpray > 0 )
		{
			if ( spellID == SPELL_BREATHE_FIRE )
			{
			}
			else
			{
				allowedLevelup = false; // foci/sprays
			}
		}
	}
	
	auto spell = getSpellFromID(spellID);
	if ( !spell ) { return false; }

	int player = parent->skill[2];
	if ( player < 0 || player >= MAXPLAYERS )
	{
		return false;
	}
	auto findSpellDef = ItemTooltips.spellItems.find(spellID);
	if ( findSpellDef == ItemTooltips.spellItems.end() )
	{
		return false;
	}
	auto& spellDef = findSpellDef->second;
	bool skillTooHigh = false;
	if ( allowedLevelup )
	{
		if ( hitentity )
		{
			Stat* hitstats = (hitentity->behavior == &actMonster || hitentity->behavior == &actPlayer) ? hitentity->getStats() : nullptr;
			if ( hitstats )
			{
				if ( hitstats->getEffectActive(EFF_STASIS) )
				{
					allowedLevelup = false;
				}
				else if ( (hitentity->behavior == &actMonster 
						&& (hitentity->monsterAllyGetPlayerLeader() || (hitstats && achievementObserver.checkUidIsFromPlayer(hitstats->leader_uid) >= 0)))
					|| hitentity->behavior == &actPlayer )
				{
					if ( spellDef.spellTags.find(ItemTooltips_t::SPELL_TAG_BUFF) == spellDef.spellTags.end()
						&& spellDef.spellTags.find(ItemTooltips_t::SPELL_TAG_HEALING) == spellDef.spellTags.end()
						&& spellDef.spellTags.find(ItemTooltips_t::SPELL_TAG_CURE) == spellDef.spellTags.end() )
					{
						allowedLevelup = false; // no level up on allies
					}
				}

				if ( allowedLevelup )
				{
					if ( parent->isInvisible() && parent->checkEnemy(hitentity) )
					{
						players[player]->mechanics.updateSustainedSpellEvent(SPELL_INVISIBILITY, 1.0, 1.0);
					}
					if ( projectile && projectile->behavior == &actMagicMissile && projectile->actmagicOrbitCastFromSpell == 1 )
					{
						if ( stats[player]->getEffectActive(EFF_MAGICAMPLIFY) )
						{
							players[player]->mechanics.updateSustainedSpellEvent(SPELL_AMPLIFY_MAGIC, 5.0, 1.0);
						}
					}
				}
			}
		}

		if ( magicstaff )
		{
			if ( stats[player]->getProficiency(spell->skillID) >= SKILL_LEVEL_BASIC )
			{
				allowedLevelup = false;
			}
		}
		else
		{
			if ( stats[player]->getProficiency(spell->skillID) >= std::min(SKILL_LEVEL_LEGENDARY, (spell->difficulty + 20)) )
			{
				allowedLevelup = false;
				skillTooHigh = true;
			}
		}
	}

	for ( int flag = 0; flag < 32; ++flag )
	{
		Uint32 tag = 1 << flag;
		if ( tag >= spell_t::SPELL_LEVEL_EVENT_ENUM_END )
		{
			break;
		}
		if ( eventType & tag )
		{
			if ( tag != spell_t::SPELL_LEVEL_EVENT_MAGICSTAFF 
				&& tag != spell_t::SPELL_LEVEL_EVENT_SPELLBOOK 
				&& tag != spell_t::SPELL_LEVEL_EVENT_MINOR_CHANCE
				&& tag != spell_t::SPELL_LEVEL_EVENT_ALWAYS )
			{
				bool found = spellDef.spellLevelTags.find((spell_t::SpellOnCastEventTypes)tag) != spellDef.spellLevelTags.end();
				assert(found);
			}
		}
	}

	bool nothingElseToLearnMsg = false;
	bool skillIncreased = false;

	if ( spell->skillID > 0 )
	{
		if ( magicstaff )
		{
			if ( local_rng.rand() % 8 == 0 ) //16.67%
			{
				if ( allowedLevelup )
				{
					parent->increaseSkill(spell->skillID);
					skillIncreased = true;
				}
				else
				{
					nothingElseToLearnMsg = true;
				}
			}
		}
		else
		{
			int chance = 4 + (stats[player]->getProficiency(spell->skillID) / 20) * 1; // 4 - 8
			if ( eventType & spell_t::SPELL_LEVEL_EVENT_SHAPESHIFT
				|| eventType & spell_t::SPELL_LEVEL_EVENT_SUMMON )
			{
				chance /= 2;
			}

			if ( (eventType & spell_t::SPELL_LEVEL_EVENT_SUSTAIN) )
			{
				bool sustainedChance = players[player]->mechanics.sustainedSpellLevelChance(spell->skillID);
				int baseSpellChance = players[player]->mechanics.baseSpellLevelChance(spell->skillID);
				if ( eventType & spell_t::SPELL_LEVEL_EVENT_MINOR_CHANCE )
				{
					chance += 8;
				}
				chance = std::max(2, chance - baseSpellChance);

				if ( sustainedChance && (local_rng.rand() % chance == 0) )
				{
					if ( allowedLevelup )
					{
						players[player]->mechanics.sustainedSpellClearMP(spell->skillID);
						players[player]->mechanics.baseSpellClearMP(spell->skillID);
						parent->increaseSkill(spell->skillID);
						skillIncreased = true;
					}
					else
					{
						nothingElseToLearnMsg = true;
					}
				}
			}
			else
			{
				int baseSpellChance = players[player]->mechanics.baseSpellLevelChance(spell->skillID);
				if ( eventType & spell_t::SPELL_LEVEL_EVENT_MINOR_CHANCE )
				{
					chance += 8;
				}
				chance = std::max(2, chance - baseSpellChance);
				if ( eventType & spell_t::SPELL_LEVEL_EVENT_ALWAYS )
				{
					chance = 1;
				}

				if ( local_rng.rand() % chance == 0 )
				{
					if ( allowedLevelup )
					{
						int& procsToLevel = players[player]->mechanics.baseSpellLevelUpProcs[spell->ID];
						int mpSpent = players[player]->mechanics.baseSpellMPSpent(spell->skillID);
						int threshold = 5 + 5 * (stats[player]->getProficiency(spell->skillID) / 20);
						if ( (procsToLevel == 0 && mpSpent >= threshold) || (eventType & spell_t::SPELL_LEVEL_EVENT_ALWAYS) )
						{
							players[player]->mechanics.baseSpellClearMP(spell->skillID);
							parent->increaseSkill(spell->skillID);
							skillIncreased = true;
							//++procsToLevel;
						}
						else
						{
							/*++procsToLevel;
							if ( procsToLevel >= (2 - (spell->difficulty / 20)) )
							{
								procsToLevel = 0;
							}*/
							players[player]->mechanics.baseSpellIncrementMP(5 + (spell->difficulty / 20), spell->skillID);
						}
					}
					else
					{
						if ( skillTooHigh )
						{
							nothingElseToLearnMsg = true;
							players[player]->mechanics.baseSpellIncrementMP(5 + (spell->difficulty / 20), spell->skillID);
						}
					}
				}
			}

			/*if ( nothingElseToLearnMsg )
			{
				if ( local_rng.rand() % 20 == 0 )
				{
					messagePlayer(player, MESSAGE_HINT, Language::get(2591));
				}
			}*/

			if ( spellbook )
			{
				if ( skillIncreased )
				{
					if ( stats[player] && stats[player]->playerRace == RACE_INSECTOID && stats[player]->stat_appearance == 0 )
					{
						steamStatisticUpdateClient(player, STEAM_STAT_BOOKWORM, STEAM_STAT_INT, 1);
					}
				}
			}
		}
	}

	return skillIncreased;
}

void magicOnEntityHit(Entity* parent, Entity* particle, Entity* hitentity, Stat* hitstats, Sint32 preResistanceDamage, Sint32 damage, Sint32 oldHP, int spellID, int selfCastUsingItem)
{
	if ( !hitentity  ) { return; }

	if ( hitstats )
	{
		Sint32 damageTaken = oldHP - hitstats->HP;
		if ( damageTaken > 0 )
		{
			if ( hitstats->getEffectActive(EFF_DEFY_FLESH) && spellID != SPELL_DEFY_FLESH )
			{
				hitentity->defyFleshProc(parent);
			}
			hitentity->pinpointDamageProc(parent, damageTaken);
		}
	}

 	if ( hitentity->behavior == &actPlayer && spellID > SPELL_NONE )
	{
		Compendium_t::Events_t::eventUpdateCodex(hitentity->skill[2], Compendium_t::CPDM_RES_SPELLS_HIT, "res", 1);

		if ( hitstats )
		{
			Sint32 damageTaken = oldHP - hitstats->HP;
			if ( damageTaken > 0 )
			{
				Compendium_t::Events_t::eventUpdateCodex(hitentity->skill[2], Compendium_t::CPDM_RES_DMG_TAKEN, "res", damageTaken);
				Compendium_t::Events_t::eventUpdateCodex(hitentity->skill[2], Compendium_t::CPDM_HP_MOST_DMG_LOST_ONE_HIT, "hp", damageTaken);
				if ( preResistanceDamage > damage )
				{
					Sint32 noResistDmgTaken = oldHP - std::max(0, oldHP - preResistanceDamage);
					if ( noResistDmgTaken > damageTaken )
					{
						Compendium_t::Events_t::eventUpdateCodex(hitentity->skill[2], Compendium_t::CPDM_RES_DMG_RESISTED, "res", noResistDmgTaken - damageTaken);
						Compendium_t::Events_t::eventUpdateCodex(hitentity->skill[2], Compendium_t::CPDM_RES_DMG_RESISTED_RUN, "res", noResistDmgTaken - damageTaken);
					}
				}
			}

			if ( damage > 0 )
			{
				if ( hitstats->getEffectActive(EFF_GUARD_SPIRIT) )
				{
					thaumSpellArmorProc(hitentity, *hitstats, false, nullptr, EFF_GUARD_SPIRIT);
				}
				if ( hitstats->getEffectActive(EFF_DIVINE_GUARD) )
				{
					thaumSpellArmorProc(hitentity, *hitstats, false, nullptr, EFF_DIVINE_GUARD);
				}
			}
		}
	}

	if ( parent && parent->behavior == &actMonster )
	{
		int summonSpellID = getSpellFromSummonedEntityForSpellEvent(parent);
		if ( summonSpellID != SPELL_NONE )
		{
			if ( Entity* leader = parent->monsterAllyGetPlayerLeader() )
			{
				magicOnSpellCastEvent(leader, nullptr, hitentity, summonSpellID, spell_t::SPELL_LEVEL_EVENT_ASSIST, 1);
			}
		}
	}

	if ( parent && parent->behavior == &actMonster && spellID > SPELL_NONE )
	{
		if ( hitstats )
		{
			Sint32 damageTaken = oldHP - hitstats->HP;
			if ( damageTaken > 0 )
			{
				if ( Stat* stats = parent->getStats() )
				{
					if ( stats->type == SPELLBOT )
					{
						if ( Entity* leader = parent->monsterAllyGetPlayerLeader() )
						{
							Compendium_t::Events_t::eventUpdate(leader->skill[2],
								Compendium_t::CPDM_SENTRY_DEPLOY_DMG, TOOL_SPELLBOT, damageTaken);
						}
						return;
					}
				}
			}
		}
	}
	else if ( parent && parent->behavior == &actPlayer && spellID > SPELL_NONE )
	{
		Sint32 damageTaken = 0;
		if ( hitstats )
		{
			damageTaken = oldHP - hitstats->HP;
		}

		Uint32 additionalFlags = 0;
		if ( hitstats && hitstats->HP > 0 )
		{
			if ( spellID == SPELL_FIRE_WALL || spellID == SPELL_DISRUPT_EARTH || spellID == SPELL_EARTH_SPINES || spellID == SPELL_ICE_WAVE || spellID == SPELL_HOLY_FIRE )
			{
				additionalFlags |= spell_t::SPELL_LEVEL_EVENT_MINOR_CHANCE;
			}
			else if ( spellID == SPELL_METEOR || spellID == SPELL_METEOR_SHOWER )
			{
				if ( particle && particle->flags[SPRITE] ) // weak hits
				{
					additionalFlags |= spell_t::SPELL_LEVEL_EVENT_MINOR_CHANCE;
				}
			}
		}
		if ( spellID == SPELL_FLAMES || spellID == SPELL_BREATHE_FIRE )
		{
			additionalFlags |= spell_t::SPELL_LEVEL_EVENT_MINOR_CHANCE;
		}
		if ( spellID == SPELL_DEFY_FLESH && damageTaken > 0 )
		{
			additionalFlags |= spell_t::SPELL_LEVEL_EVENT_MINOR_CHANCE;
		}

		if ( particle && particle->behavior == &actPlayer )
		{
			if ( selfCastUsingItem > 0 && selfCastUsingItem < NUMITEMS )
			{
				if ( items[selfCastUsingItem].category == MAGICSTAFF )
				{
					auto find = ItemTooltips.spellItems.find(spellID);
					if ( find != ItemTooltips.spellItems.end() )
					{
						if ( damageTaken > 0 )
						{
							if ( find->second.magicstaffId >= 0 && find->second.magicstaffId < NUMITEMS && items[find->second.magicstaffId].category == MAGICSTAFF )
							{
								Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_SPELL_DMG, (ItemType)find->second.magicstaffId, damageTaken);
							}
							magicOnSpellCastEvent(parent, particle, hitentity, spellID, additionalFlags | spell_t::SPELL_LEVEL_EVENT_DMG | spell_t::SPELL_LEVEL_EVENT_MAGICSTAFF, damageTaken);
						}
						else if ( damage == 0 && oldHP == 0 )
						{
							if ( find->second.spellTags.find(ItemTooltips_t::SpellTagTypes::SPELL_TAG_TRACK_HITS) != find->second.spellTags.end() )
							{
								Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_SPELL_TARGETS,
									(ItemType)find->second.magicstaffId, 1);
							}
							magicOnSpellCastEvent(parent, particle, hitentity, spellID, additionalFlags | spell_t::SPELL_LEVEL_EVENT_EFFECT | spell_t::SPELL_LEVEL_EVENT_MAGICSTAFF, 1);
						}
					}
				}
				else if ( items[selfCastUsingItem].category == SPELLBOOK )
				{
					auto find = ItemTooltips.spellItems.find(spellID);
					if ( find != ItemTooltips.spellItems.end() )
					{
						if ( find->second.spellbookId >= 0 && find->second.spellbookId < NUMITEMS && items[find->second.spellbookId].category == SPELLBOOK )
						{
							if ( damageTaken > 0 )
							{
								if ( find->second.spellTags.find(ItemTooltips_t::SpellTagTypes::SPELL_TAG_HEALING) != find->second.spellTags.end() )
								{
									Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_SPELL_HEAL, (ItemType)find->second.spellbookId, damageTaken);
								}
								else
								{
									Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_SPELL_DMG, (ItemType)find->second.spellbookId, damageTaken);
								}
								magicOnSpellCastEvent(parent, particle, hitentity, spellID, additionalFlags | spell_t::SPELL_LEVEL_EVENT_DMG | spell_t::SPELL_LEVEL_EVENT_SPELLBOOK, damageTaken);
							}
							else if ( damage == 0 && oldHP == 0 )
							{
								if ( find->second.spellTags.find(ItemTooltips_t::SpellTagTypes::SPELL_TAG_TRACK_HITS) != find->second.spellTags.end() )
								{
									Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_SPELL_TARGETS,
										(ItemType)find->second.spellbookId, 1);
								}
								magicOnSpellCastEvent(parent, particle, hitentity, spellID, additionalFlags | spell_t::SPELL_LEVEL_EVENT_EFFECT | spell_t::SPELL_LEVEL_EVENT_SPELLBOOK, 1);
							}
						}
					}
				}
			}
			else
			{
				// normal spellcasts
				auto find = ItemTooltips.spellItems.find(spellID);
				if ( find != ItemTooltips.spellItems.end() )
				{
					if ( find->second.id > SPELL_NONE && find->second.id < NUM_SPELLS )
					{
						if ( damageTaken > 0 )
						{
							Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_SPELL_DMG, SPELL_ITEM, damageTaken, false, find->second.id);
							magicOnSpellCastEvent(parent, particle, hitentity, spellID, additionalFlags | spell_t::SPELL_LEVEL_EVENT_DMG, damageTaken);
						}
						else if ( damage == 0 && oldHP == 0 )
						{
							if ( find->second.spellTags.find(ItemTooltips_t::SpellTagTypes::SPELL_TAG_TRACK_HITS) != find->second.spellTags.end() )
							{
								Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_SPELL_TARGETS, SPELL_ITEM, 1, false, find->second.id);
							}
							magicOnSpellCastEvent(parent, particle, hitentity, spellID, additionalFlags | spell_t::SPELL_LEVEL_EVENT_EFFECT, 1);
						}
					}
				}
			}
		}
		if ( !particle || (particle && particle->behavior != &actMagicMissile)) { return; }
		if ( particle->actmagicCastByMagicstaff == 1 )
		{
			auto find = ItemTooltips.spellItems.find(spellID);
			if ( find != ItemTooltips.spellItems.end() )
			{
				if ( damageTaken > 0 )
				{
					if ( find->second.magicstaffId >= 0 && find->second.magicstaffId < NUMITEMS && items[find->second.magicstaffId].category == MAGICSTAFF )
					{
						Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_SPELL_DMG, (ItemType)find->second.magicstaffId, damageTaken);
					}
					magicOnSpellCastEvent(parent, particle, hitentity, spellID, additionalFlags | spell_t::SPELL_LEVEL_EVENT_DMG | spell_t::SPELL_LEVEL_EVENT_MAGICSTAFF, damageTaken);
				}
				else if ( damage == 0 && oldHP == 0 )
				{
					if ( find->second.spellTags.find(ItemTooltips_t::SpellTagTypes::SPELL_TAG_TRACK_HITS) != find->second.spellTags.end() )
					{
						Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_SPELL_TARGETS, 
							(ItemType)find->second.magicstaffId, 1);
					}
					magicOnSpellCastEvent(parent, particle, hitentity, spellID, additionalFlags | spell_t::SPELL_LEVEL_EVENT_EFFECT | spell_t::SPELL_LEVEL_EVENT_MAGICSTAFF, 1);
				}
			}
		}
		else if ( particle->actmagicSpray == 2 && spellID != SPELL_BREATHE_FIRE )
		{
			// foci items
		}
		else if ( particle->actmagicFromSpellbook != 0 )
		{
			auto find = ItemTooltips.spellItems.find(spellID);
			if ( find != ItemTooltips.spellItems.end() )
			{
				if ( find->second.spellbookId >= 0 && find->second.spellbookId < NUMITEMS && items[find->second.spellbookId].category == SPELLBOOK )
				{
					if ( damageTaken > 0 )
					{
						if ( find->second.spellTags.find(ItemTooltips_t::SpellTagTypes::SPELL_TAG_HEALING) != find->second.spellTags.end() )
						{
							Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_SPELL_HEAL, (ItemType)find->second.spellbookId, damageTaken);
						}
						else
						{
							Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_SPELL_DMG, (ItemType)find->second.spellbookId, damageTaken);
						}
						magicOnSpellCastEvent(parent, particle, hitentity, spellID, additionalFlags | spell_t::SPELL_LEVEL_EVENT_DMG | spell_t::SPELL_LEVEL_EVENT_SPELLBOOK, damageTaken);
					}
					else if ( damage == 0 && oldHP == 0 )
					{
						if ( find->second.spellTags.find(ItemTooltips_t::SpellTagTypes::SPELL_TAG_TRACK_HITS) != find->second.spellTags.end() )
						{
							Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_SPELL_TARGETS,
								(ItemType)find->second.spellbookId, 1);
						}
						magicOnSpellCastEvent(parent, particle, hitentity, spellID, additionalFlags | spell_t::SPELL_LEVEL_EVENT_EFFECT | spell_t::SPELL_LEVEL_EVENT_SPELLBOOK, 1);
					}
				}
			}
		}
		else if ( particle->actmagicCastByTinkerTrap == 0 )
		{
			if ( particle->actmagicIsOrbiting == 2 && particle->actmagicOrbitCastFromSpell == 0 )
			{
				// cast by firestorm potion etc
				if ( damageTaken > 0 && parent != hitentity )
				{
					auto find = ItemTooltips.spellItems.find(spellID);
					if ( find != ItemTooltips.spellItems.end() )
					{
						if ( find->second.id > SPELL_NONE && find->second.id < NUM_SPELLS )
						{
							ItemType type = WOODEN_SHIELD;
							if ( find->second.id == SPELL_FIREBALL )
							{
								type = POTION_FIRESTORM;
							}
							else if ( find->second.id == SPELL_COLD )
							{
								type = POTION_ICESTORM;
							}
							else if ( find->second.id == SPELL_LIGHTNING )
							{
								type = POTION_THUNDERSTORM;
							}
							if ( type != WOODEN_SHIELD )
							{
								Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_THROWN_DMG_TOTAL, type, damageTaken);
							}
						}
					}
				}
			}
			else
			{
				// normal spellcasts
				auto find = ItemTooltips.spellItems.find(spellID);
				if ( find != ItemTooltips.spellItems.end() )
				{
					if ( find->second.id > SPELL_NONE && find->second.id < NUM_SPELLS )
					{
						if ( damageTaken > 0 )
						{
							Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_SPELL_DMG, SPELL_ITEM, damageTaken, false, find->second.id);
							magicOnSpellCastEvent(parent, particle, hitentity, spellID, additionalFlags | spell_t::SPELL_LEVEL_EVENT_DMG, damageTaken);
						}
						else if ( damage == 0 && oldHP == 0 )
						{
							if ( find->second.spellTags.find(ItemTooltips_t::SpellTagTypes::SPELL_TAG_TRACK_HITS) != find->second.spellTags.end() )
							{
								Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_SPELL_TARGETS, SPELL_ITEM, 1, false, find->second.id);
							}
							magicOnSpellCastEvent(parent, particle, hitentity, spellID, additionalFlags | spell_t::SPELL_LEVEL_EVENT_EFFECT, 1);
						}
					}
				}
			}
		}
		else if ( particle->actmagicCastByTinkerTrap == 1 )
		{
			if ( damageTaken > 0 )
			{
				if ( spellID == SPELL_FIREBALL )
				{
					Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_BOMB_DMG, TOOL_BOMB, damageTaken);
				}
				else if ( spellID == SPELL_COLD )
				{
					Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_BOMB_DMG, TOOL_FREEZE_BOMB, damageTaken);
				}
			}
		}
	}
}

void magicTrapOnHit(Entity* parent, Entity* hitentity, Stat* hitstats, Sint32 oldHP, int spellID)
{
	if ( !parent || !hitentity || !hitstats ) { return; }
	if ( spellID == SPELL_NONE ) { return; }
	if ( parent->behavior == &actMagicTrap || parent->behavior == &actMagicTrapCeiling )
	{
		const char* category = parent->behavior == &actMagicTrap ? "magic trap" : "ceiling trap";
		if ( oldHP == 0 )
		{
			if ( hitentity->behavior == &actPlayer )
			{
				Compendium_t::Events_t::eventUpdateWorld(hitentity->skill[2], Compendium_t::CPDM_TRAP_MAGIC_STATUSED, category, 1);
			}
		}
		else if ( oldHP > hitstats->HP )
		{
			if ( hitentity->behavior == &actPlayer )
			{
				Compendium_t::Events_t::eventUpdateWorld(hitentity->skill[2], Compendium_t::CPDM_TRAP_DAMAGE, category, oldHP - hitstats->HP);
				if ( hitstats->HP <= 0 )
				{
					Compendium_t::Events_t::eventUpdateWorld(hitentity->skill[2], Compendium_t::CPDM_TRAP_KILLED_BY, category, 1);
				}
			}
			else if ( hitentity->behavior == &actMonster )
			{
				if ( auto leader = hitentity->monsterAllyGetPlayerLeader() )
				{
					if ( hitstats->HP <= 0 )
					{
						Compendium_t::Events_t::eventUpdateWorld(hitentity->monsterAllyIndex, Compendium_t::CPDM_TRAP_FOLLOWERS_KILLED, category, 1);
					}
				}
			}
		}
	}
}

bool absorbMagicEvent(Entity* entity, Entity* parent, Entity& damageSourceProjectile, int spellID, real_t* result, real_t& damageMultiplier, DamageGib& dmgGib)
{
	if ( !entity || !parent ) { return false; }
	if ( Stat* parentStats = parent->getStats() )
	{
		if ( parentStats->getEffectActive(EFF_ABSORB_MAGIC) > 1 && spellID > SPELL_NONE )
		{
			if ( entity->behavior == &actMonster && parent && parent->checkEnemy(entity) )
			{
				if ( auto spell = getSpellFromID(spellID) )
				{
					auto find = ItemTooltips.spellItems.find(spellID);
					if ( find != ItemTooltips.spellItems.end() )
					{
						if ( find->second.spellTags.find(ItemTooltips_t::SPELL_TAG_DAMAGE) != find->second.spellTags.end() )
						{
							real_t bonus = (parentStats->getEffectActive(EFF_ABSORB_MAGIC) - 1) / 100.0;
							bonus = std::min(getSpellDamageSecondaryFromID(SPELL_ABSORB_MAGIC, parent, nullptr, parent) / 100.0, bonus);
							if ( result )
							{
								*result = bonus;
							}
							damageMultiplier += bonus;
							dmgGib = DMG_STRONGEST;
							parent->setEffect(EFF_ABSORB_MAGIC, false, 0, false);
							if ( parent->behavior == &actPlayer )
							{
								messagePlayerColor(parent->skill[2], MESSAGE_STATUS, makeColorRGB(0, 255, 0), Language::get(6858));
								magicOnSpellCastEvent(parent, &damageSourceProjectile, entity, SPELL_ABSORB_MAGIC, spell_t::SPELL_LEVEL_EVENT_DEFAULT | spell_t::SPELL_LEVEL_EVENT_MINOR_CHANCE, 1);
							}
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

Sint32 convertResistancePointsToMagicValue(Sint32 value, int resistance)
{
	if ( resistance > 0 )
	{
		real_t mult = 1.0;
		for ( int i = 0; i < resistance; ++i, mult *= (1 - Entity::magicResistancePerPoint) ) {}

		return std::max(1.0, value * (mult));
	}
	else
	{
		return value;
	}
}

void magicSetResistance(Entity* entity, Entity* parent, int& resistance, real_t& damageMultiplier, DamageGib& dmgGib, int& trapResist, int spellID)
{
	if ( entity )
	{
		resistance = Entity::getMagicResistance(entity->getStats());
		if ( (entity->behavior == &actMonster || entity->behavior == &actPlayer) && entity->getStats() )
		{
			damageMultiplier = Entity::getDamageTableMultiplier(entity, *entity->getStats(), DAMAGE_TABLE_MAGIC, &resistance);

			if ( damageMultiplier <= 0.75 )
			{
				dmgGib = DMG_WEAKEST;
			}
			else if ( damageMultiplier <= 0.85 )
			{
				dmgGib = DMG_WEAKER;
			}
			else if ( damageMultiplier >= 1.25 )
			{
				dmgGib = resistance == 0 ? DMG_STRONGEST : DMG_WEAKER;
			}
			else if ( damageMultiplier >= 1.15 )
			{
				dmgGib = resistance == 0 ? DMG_STRONGER : DMG_WEAKER;
			}
			else if ( resistance > 0 )
			{
				dmgGib = DMG_WEAKEST;
			}
		}

		if ( parent && (parent->behavior == &actMagicTrap || parent->behavior == &actMagicTrapCeiling) )
		{
			trapResist = entity ? entity->getEntityBonusTrapResist() : 0;
			if ( trapResist != 0 )
			{
				damageMultiplier += -(trapResist / 100.0);
				damageMultiplier = std::max(0.0, damageMultiplier);
			}
		}
	}
}

std::map<int, Uint32> lastMagicSoundPlayed;

void actMagicMissile(Entity* my)   //TODO: Verify this function.
{
	if (!my || !my->children.first || !my->children.first->element)
	{
		return;
	}
	spell_t* spell = (spell_t*)my->children.first->element;
	if (!spell)
	{
		return;
	}
	//node_t *node = NULL;
	spellElement_t* element = NULL;
	node_t* node = NULL;
	double tangent;

	Entity* parent = uidToEntity(my->parent);

	if ( !magic_init )
	{
		//Any init stuff that needs to happen goes here.
		magic_init = 1;
		my->skill[2] = -7; // ordinarily the client won't do anything with this entity
		if ( my->actmagicIsOrbiting == 1 || my->actmagicIsOrbiting == 2 )
		{
			MAGIC_MAXLIFE = my->actmagicOrbitLifetime;
		}
		else if ( my->actmagicIsVertical != MAGIC_ISVERTICAL_NONE )
		{
			MAGIC_MAXLIFE = 512;
		}
	}

	if (magic_init)
	{
		my->removeLightField();

		if ( multiplayer != CLIENT )
		{
			//Handle the missile's life.
			if ( my->actmagicDelayMove > 0 )
			{
				--my->actmagicDelayMove;
				my->flags[INVISIBLE] = true;
				if ( my->actmagicDelayMove == 0 )
				{
					my->flags[INVISIBLE] = false;
					my->flags[UPDATENEEDED] = true;
					my->vel_x = my->actmagicVelXStore;
					my->vel_y = my->actmagicVelYStore;
					my->vel_z = my->actmagicVelZStore;

					if ( spell->ID == SPELL_METEOR || spell->ID == SPELL_METEOR_SHOWER )
					{
						if ( node_t* node = spell->elements.first )
						{
							spellElement_t* element = (spellElement_t*)node->element;
							if ( node_t* node2 = element->elements.first )
							{
								if ( element = (spellElement_t*)node2->element )
								{
									if ( !strcmp(element->element_internal_name, "spell_element_flames") )
									{
										playSoundEntity(my, 814, 128);
									}
									else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_METEOR_SHOWER].element_internal_name)
										|| !strcmp(element->element_internal_name, spellElementMap[SPELL_METEOR].element_internal_name) )
									{
										playSoundEntity(my, 164, 128);
									}
								}
							}
						}
					}
				}
			}
			
			if ( my->actmagicDelayMove == 0 )
			{
				MAGIC_LIFE++;
			}

			if (MAGIC_LIFE >= MAGIC_MAXLIFE)
			{
				my->removeLightField();
				list_RemoveNode(my->mynode);
				return;
			}

			if ( spell->ID == SPELL_CHARM_MONSTER || spell->ID == SPELL_ACID_SPRAY )
			{
				Entity* caster = uidToEntity(spell->caster);
				if ( !caster )
				{
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
			}
			else if ( spell->ID == SPELL_NONE )
			{
				my->removeLightField();
				list_RemoveNode(my->mynode);
				return;
			}

			node = spell->elements.first;
			//element = (spellElement_t *) spell->elements->first->element;
			element = (spellElement_t*)node->element;
			Sint32 entityHealth = 0;
			double dist = 0.f;
			bool hitFromAbove = false;
			bool hitFromSide = false;
			ParticleEmitterHit_t* particleEmitterHitProps = nullptr;
			if ( my->actmagicDelayMove > 0 )
			{
				// no movement or collision
			}
			else if ( my->actmagicSpray == 2 )
			{
				// foci auto-hit particle
				if ( Entity* autohitTarget = uidToEntity(my->actmagicOrbitHitTargetUID4) )
				{
					hit.entity = autohitTarget;
					hitFromSide = true;
				}
			}
			else if ( my->actmagicSpray == 1 )
			{
				my->vel_z += my->actmagicSprayGravity;
				my->z += my->vel_z;
				my->roll += 0.1;
				dist = clipMove(&my->x, &my->y, my->vel_x, my->vel_y, my); //normal flat projectiles

				if ( my->z < 8.0 )
				{
					// if we didn't hit the floor, process normal horizontal movement collision if we aren't too high
					if ( dist != sqrt(my->vel_x * my->vel_x + my->vel_y * my->vel_y) )
					{
						hitFromAbove = true;

						if ( particleEmitterHitProps = getParticleEmitterHitProps(my->actmagicEmitter, hit.entity) )
						{
							particleEmitterHitProps->hits++;
							particleEmitterHitProps->tick = ticks;
						}
					}
				}
				else
				{
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
			}
			else if ( (parent && parent->behavior == &actMagicTrapCeiling) || my->actmagicIsVertical == MAGIC_ISVERTICAL_Z )
			{
				// moving vertically.
				my->z += my->vel_z;
				hitFromAbove = my->magicFallingCollision();
				if ( !hitFromAbove )
				{
					// nothing hit yet, let's keep trying...
				}
			}
			else if ( my->actmagicIsOrbiting != 0 )
			{
				int turnRate = 4;
				if ( parent && my->actmagicIsOrbiting == 1 )
				{
					my->yaw += 0.1;
					my->x = parent->x + my->actmagicOrbitDist * cos(my->yaw);
					my->y = parent->y + my->actmagicOrbitDist * sin(my->yaw);
				}
				else if ( my->actmagicIsOrbiting == 2 )
				{
					my->yaw += 0.2;
					turnRate = 4;
					my->x = my->actmagicOrbitStationaryX + my->actmagicOrbitStationaryCurrentDist * cos(my->yaw);
					my->y = my->actmagicOrbitStationaryY + my->actmagicOrbitStationaryCurrentDist * sin(my->yaw);
					my->actmagicOrbitStationaryCurrentDist =
						std::min(my->actmagicOrbitStationaryCurrentDist + 0.5, static_cast<real_t>(my->actmagicOrbitDist));
				}
				hitFromAbove = my->magicOrbitingCollision();
				my->z += my->vel_z * my->actmagicOrbitVerticalDirection;

				if ( my->actmagicIsOrbiting == 2 )
				{
					// we don't change direction, upwards we go!
					// target speed is actmagicOrbitVerticalSpeed.
					my->vel_z = std::min(my->actmagicOrbitVerticalSpeed, my->vel_z / 0.95);
					my->roll += (PI / 8) / (turnRate / my->vel_z) * my->actmagicOrbitVerticalDirection;
					my->roll = std::max(my->roll, -PI / 4);
				}
				else if ( my->z > my->actmagicOrbitStartZ )
				{
					if ( my->actmagicOrbitVerticalDirection == 1 )
					{
						my->vel_z = std::max(0.01, my->vel_z * 0.95);
						my->roll -= (PI / 8) / (turnRate / my->vel_z) * my->actmagicOrbitVerticalDirection;
					}
					else
					{
						my->vel_z = std::min(my->actmagicOrbitVerticalSpeed, my->vel_z / 0.95);
						my->roll += (PI / 8) / (turnRate / my->vel_z) * my->actmagicOrbitVerticalDirection;
					}
				}
				else
				{
					if ( my->actmagicOrbitVerticalDirection == 1 )
					{
						my->vel_z = std::min(my->actmagicOrbitVerticalSpeed, my->vel_z / 0.95);
						my->roll += (PI / 8) / (turnRate / my->vel_z) * my->actmagicOrbitVerticalDirection;
					}
					else
					{
						my->vel_z = std::max(0.01, my->vel_z * 0.95);
						my->roll -= (PI / 8) / (turnRate / my->vel_z) * my->actmagicOrbitVerticalDirection;
					}
				}
				
				if ( my->actmagicIsOrbiting == 1 )
				{
					if ( (my->z > my->actmagicOrbitStartZ + 4) && my->actmagicOrbitVerticalDirection == 1 )
					{
						my->actmagicOrbitVerticalDirection = -1;
					}
					else if ( (my->z < my->actmagicOrbitStartZ - 4) && my->actmagicOrbitVerticalDirection != 1 )
					{
						my->actmagicOrbitVerticalDirection = 1;
					}
				}
			}
			else
			{
				if ( my->actmagicIsVertical == MAGIC_ISVERTICAL_XYZ )
				{
					// moving vertically and horizontally, check if we hit the floor
					my->z += my->vel_z;
					hitFromAbove = my->magicFallingCollision();
					my->processEntityWind();

					bool halfSpeedCheck = false;
					static ConsoleVariable<bool> cvar_magic_clip("/magic_clip_test", true);
					real_t speed = sqrt(pow(my->vel_x, 2) + pow(my->vel_y, 2));
					if ( speed > 4.0 ) // can clip through thin gates
					{
						auto entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 1);
						for ( auto it : entLists )
						{
							if ( !*cvar_magic_clip && (svFlags & SV_FLAG_CHEATS) )
							{
								break;
							}
							for ( node_t* node = it->first; node != nullptr; node = node->next )
							{
								Entity* entity = (Entity*)node->element;
								if ( entity->behavior == &actGate || entity->behavior == &actDoor || entity->behavior == &actIronDoor )
								{
									if ( entityDist(my, entity) <= speed )
									{
										halfSpeedCheck = true;
										break;
									}
								}
							}
							if ( halfSpeedCheck )
							{
								break;
							}
						}
					}

					if ( !halfSpeedCheck )
					{
						dist = clipMove(&my->x, &my->y, my->vel_x, my->vel_y, my);
						if ( !hitFromAbove && my->z > -5 )
						{
							// if we didn't hit the floor, process normal horizontal movement collision if we aren't too high
							if ( dist != sqrt(my->vel_x * my->vel_x + my->vel_y * my->vel_y) )
							{
								hitFromAbove = true;
							}
						}
					}
					else
					{
						real_t vel_x = my->vel_x / 2.0;
						real_t vel_y = my->vel_y / 2.0;
						real_t dist = clipMove(&my->x, &my->y, vel_x, vel_y, my);
						if ( !hitFromAbove && my->z > -5 )
						{
							// if we didn't hit the floor, process normal horizontal movement collision if we aren't too high
							if ( dist != sqrt(vel_x * vel_x + vel_y * vel_y) )
							{
								hitFromAbove = true;
							}
						}
						if ( !hitFromAbove )
						{
							dist = clipMove(&my->x, &my->y, vel_x, vel_y, my);
							if ( !hitFromAbove && my->z > -5 )
							{
								// if we didn't hit the floor, process normal horizontal movement collision if we aren't too high
								if ( dist != sqrt(vel_x * vel_x + vel_y * vel_y) )
								{
									hitFromAbove = true;
								}
							}
						}
					}

					if ( my->sprite == 2209 )
					{
						my->roll += 0.25;
					}

					if ( my->actmagicProjectileArc > 0 )
					{
						real_t z = -1 - my->z;
						if ( z > 0 )
						{
							my->pitch = -atan(z * 0.1 / sqrt(my->vel_x * my->vel_x + my->vel_y * my->vel_y));
						}
						else
						{
							my->pitch = -atan(z * 0.15 / sqrt(my->vel_x * my->vel_x + my->vel_y * my->vel_y));
						}
						if ( my->actmagicProjectileArc == 1 )
						{
							//messagePlayer(0, "z: %f vel: %f", my->z, my->vel_z);
							my->vel_z = my->vel_z * 0.9;
							if ( my->vel_z > -0.1 )
							{
								//messagePlayer(0, "arc down");
								my->actmagicProjectileArc = 2;
								my->vel_z = 0.01;
							}
						}
						else if ( my->actmagicProjectileArc == 2 )
						{
							//messagePlayer(0, "z: %f vel: %f", my->z, my->vel_z);
							my->vel_z = std::min(0.8, my->vel_z * 1.2);
						}
					}
				}
				else
				{
					my->processEntityWind();

					bool halfSpeedCheck = false;
					static ConsoleVariable<bool> cvar_magic_clip2("/magic_clip_test2", true);
					real_t speed = sqrt(pow(my->vel_x, 2) + pow(my->vel_y, 2));
					if ( speed > 4.0 ) // can clip through thin gates
					{
						auto entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 1);
						for ( auto it : entLists )
						{
							if ( !*cvar_magic_clip2 && (svFlags & SV_FLAG_CHEATS) )
							{
								break;
							}
							for ( node_t* node = it->first; node != nullptr; node = node->next )
							{
								Entity* entity = (Entity*)node->element;
								if ( entity->behavior == &actGate || entity->behavior == &actDoor || entity->behavior == &actIronDoor )
								{
									if ( entityDist(my, entity) <= speed )
									{
										halfSpeedCheck = true;
										break;
									}
								}
							}
							if ( halfSpeedCheck )
							{
								break;
							}
						}
					}
					if ( !halfSpeedCheck )
					{
						dist = clipMove(&my->x, &my->y, my->vel_x, my->vel_y, my); //normal flat projectiles
						hitFromSide = dist != sqrt(my->vel_x * my->vel_x + my->vel_y * my->vel_y);
					}
					else
					{
						real_t vel_x = my->vel_x / 2.0;
						real_t vel_y = my->vel_y / 2.0;
						dist = clipMove(&my->x, &my->y, vel_x, vel_y, my);
						hitFromSide = dist != sqrt(vel_x * vel_x + vel_y * vel_y);
						if ( !hitFromSide )
						{
							dist = clipMove(&my->x, &my->y, vel_x, vel_y, my);
							hitFromSide = dist != sqrt(vel_x * vel_x + vel_y * vel_y);
						}
					}
				}
			}

			if ( my->actmagicDelayMove == 0 
				&& (hitFromAbove
				|| (my->actmagicIsVertical != MAGIC_ISVERTICAL_XYZ && my->actmagicSpray != 1 && hitFromSide)) )
			{
				node = element->elements.first;
				//element = (spellElement_t *) element->elements->first->element;
				element = (spellElement_t*)node->element;
				//if (hit.entity != NULL) {
				bool mimic = hit.entity && hit.entity->isInertMimic();
				Stat* hitstats = nullptr;
				int player = -1;

				if ( hit.entity )
				{
					hitstats = hit.entity->getStats();
					if ( hit.entity->behavior == &actPlayer )
					{
						player = hit.entity->skill[2];
					}
					if ( hit.entity && hitstats )
					{
						if ( hitstats->getEffectActive(EFF_MAGICIANS_ARMOR)
							&& !(!(svFlags & SV_FLAG_FRIENDLYFIRE) && parent 
									&& parent->checkFriend(hit.entity) 
									&& parent->friendlyFireProtection(hit.entity)) )
						{
							Uint8 effectStrength = hitstats->getEffectActive(EFF_MAGICIANS_ARMOR);
							int duration = hitstats->EFFECTS_TIMERS[EFF_MAGICIANS_ARMOR];
							if ( effectStrength == 1 )
							{
								if ( hitstats->EFFECTS_TIMERS[EFF_MAGICIANS_ARMOR] > 0 )
								{
									hitstats->EFFECTS_TIMERS[EFF_MAGICIANS_ARMOR] = 1;
								}
							}
							else if ( effectStrength > 1 )
							{
								--effectStrength;
								hitstats->setEffectValueUnsafe(EFF_MAGICIANS_ARMOR, effectStrength);
								hit.entity->setEffect(EFF_MAGICIANS_ARMOR, effectStrength, hitstats->EFFECTS_TIMERS[EFF_MAGICIANS_ARMOR], true);
							}

							magicOnSpellCastEvent(hit.entity, hit.entity, parent, SPELL_MAGICIANS_ARMOR, spell_t::SPELL_LEVEL_EVENT_DEFAULT, 1);

							if ( (parent && parent->behavior == &actPlayer) || (parent && parent->behavior == &actMonster && parent->monsterAllyGetPlayerLeader())
								|| hit.entity->behavior == &actPlayer || hit.entity->monsterAllyGetPlayerLeader() )
							{
								spawnDamageGib(hit.entity, 0, DamageGib::DMG_GUARD, DamageGibDisplayType::DMG_GIB_GUARD, true);
							}

							if ( hit.entity->behavior == &actPlayer )
							{
								messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, makeColorRGB(0, 255, 0), Language::get(6470));
							}
							if ( parent && parent->behavior == &actPlayer )
							{
								messagePlayerMonsterEvent(parent->skill[2], makeColorRGB(255, 255, 255),
									*hitstats, Language::get(6466), Language::get(6467), MSG_COMBAT); // %s guards the spell
							}

							Entity* fx = createParticleAestheticOrbit(hit.entity, 1817, TICKS_PER_SECOND / 4, PARTICLE_EFFECT_NULL_PARTICLE);
							fx->x = hit.entity->x;
							fx->y = hit.entity->y;
							fx->z = hit.entity->z;
							real_t tangent = atan2(my->y - hit.entity->y, my->x - hit.entity->x);
							fx->x += 4.0 * cos(tangent);
							fx->y += 4.0 * sin(tangent);
							fx->yaw = tangent;
							fx->actmagicOrbitDist = 0;
							fx->actmagicNoLight = 0;
							serverSpawnMiscParticlesAtLocation(fx->x, fx->y, fx->z, PARTICLE_EFFECT_NULL_PARTICLE, 1817, 0, fx->yaw * 256.0);

							//playSoundEntity(hit.entity, 166, 128);
							my->removeLightField();
							list_RemoveNode(my->mynode);
							return;
						}
					}

					if ( parent && (parent->behavior == &actMagicTrap || parent->behavior == &actMagicTrapCeiling) )
					{
						if ( hit.entity->onEntityTrapHitSacredPath(parent) )
						{
							if ( hit.entity->behavior == &actPlayer )
							{
								messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, makeColorRGB(0, 255, 0),
									Language::get(6470));
							}
							playSoundEntity(hit.entity, 166, 128);
							my->removeLightField();
							list_RemoveNode(my->mynode);
							return;
						}
					}
				}

				// count reflection
				int reflection = 0;
				if ( hitstats )
				{
					if ( parent &&
						((hit.entity->getRace() == LICH_ICE && parent->getRace() == LICH_FIRE)
							|| ((hit.entity->getRace() == LICH_FIRE || hitstats->leader_uid == parent->getUID()) && parent->getRace() == LICH_ICE)
							|| (parent->getRace() == LICH_ICE) && !strncmp(hitstats->name, "corrupted automaton", 19)
							)
						)
					{
						reflection = 3;
					}
					if ( !reflection )
					{
						reflection = hit.entity->getReflection();
					}
					if ( my->actmagicCastByTinkerTrap == 1 )
					{
						reflection = 0;
					}
					if ( reflection == 3 && hitstats->shield 
						&& (hitstats->shield->type == MIRROR_SHIELD || hitstats->getEffectActive(EFF_REFLECTOR_SHIELD) > 0) && hitstats->defending )
					{
						if ( my->actmagicIsVertical == MAGIC_ISVERTICAL_Z )
						{
							reflection = 0;
						}
						// calculate facing angle to projectile, need to be facing projectile to reflect.
						else if ( player >= 0 && players[player] && players[player]->entity )
						{
							real_t yawDiff = my->yawDifferenceFromEntity(players[player]->entity);
							if ( yawDiff < (6 * PI / 5) )
							{
								reflection = 0;
							}
							else
							{
								reflection = 3;
								if ( hitstats->shield->type == MIRROR_SHIELD )
								{
									if ( parent && (parent->behavior == &actMonster || parent->behavior == &actPlayer) )
									{
										my->actmagicMirrorReflected = 1;
										my->actmagicMirrorReflectedCaster = parent->getUID();
									}
									Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_SHIELD_REFLECT, hitstats->shield->type, 1);
								}
								else
								{
									if ( hitstats->getEffectActive(EFF_REFLECTOR_SHIELD) )
									{
										magicOnSpellCastEvent(hit.entity, hit.entity, parent, SPELL_REFLECTOR, spell_t::SpellOnCastEventTypes::SPELL_LEVEL_EVENT_DEFAULT, 1);
									}
								}
							}
						}
					}
				}

				bool yourSpellHitsTheMonster = false;
				bool youAreHitByASpell = false;
				if ( hit.entity )
				{
					if ( hit.entity->behavior == &actPlayer )
					{
						bool skipMessage = false;
						if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) && my->actmagicTinkerTrapFriendlyFire == 0 && my->actmagicAllowFriendlyFireHit == 0 )
						{
							if ( parent 
								&& (parent->behavior == &actMonster || parent->behavior == &actPlayer) 
								&& parent->checkFriend(hit.entity)
								&& parent->friendlyFireProtection(hit.entity) )
							{
								skipMessage = true;
							}
						}

						if ( my->actmagicCastByTinkerTrap == 1 )
						{
							skipMessage = true;
						}
						if ( !skipMessage )
						{
							Uint32 color = makeColorRGB(255, 0, 0);
							if ( reflection == 0 )
							{
								if ( particleEmitterHitProps )
								{
									if ( particleEmitterHitProps->hits == 1 )
									{
										if ( my->actmagicSpray == 1 || my->actmagicSpray == 2 )
										{
											//messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(6238));
											//youAreHitByASpell = true;
										}
										else
										{
											messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(376));
											youAreHitByASpell = true;
										}
									}
								}
								else
								{
									messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(376));
									youAreHitByASpell = true;
								}
							}
						}
						if ( hitstats )
						{
							entityHealth = hitstats->HP;
						}
					}
					if ( parent && hitstats )
					{
						if ( parent->behavior == &actPlayer )
						{
							Uint32 color = makeColorRGB(0, 255, 0);
							if ( strcmp(element->element_internal_name, spellElement_charmMonster.element_internal_name) )
							{
								if ( my->actmagicCastByTinkerTrap == 1 || my->actmagicNoHitMessage != 0 )
								{
									// no message
									//messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(3498), Language::get(3499), MSG_COMBAT);
								}
								else
								{
									if ( reflection == 0 )
									{
										yourSpellHitsTheMonster = true;
										if ( !hit.entity->isInertMimic() )
										{
											if ( ItemTooltips.bSpellHasBasicHitMessage(spell->ID) )
											{
												messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(378), Language::get(377), MSG_COMBAT_BASIC);
											}
											else
											{
												messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(378), Language::get(377), MSG_COMBAT);
											}
										}
									}
								}
							}
						}
					}
				}

				// Only degrade the equipment if Friendly Fire is ON or if it is (OFF && target is an enemy)
				bool bShouldEquipmentDegrade = false;
				if ( parent && parent->behavior == &actDeathGhost )
				{
					bShouldEquipmentDegrade = false;
				}
				else if ( (svFlags & SV_FLAG_FRIENDLYFIRE) )
				{
					// Friendly Fire is ON, equipment should always degrade, as hit will register
					bShouldEquipmentDegrade = true;
				}
				else
				{
					// Friendly Fire is OFF, is the target an enemy?
					if ( parent != nullptr && (parent->checkFriend(hit.entity)) == false )
					{
						// Target is an enemy, equipment should degrade
						bShouldEquipmentDegrade = true;
					}
				}

				// Handling reflecting the missile
				if ( reflection )
				{
					spell_t* spellIsReflectingMagic = nullptr;
					if ( hit.entity )
					{
						spellIsReflectingMagic = hit.entity->getActiveMagicEffect(SPELL_REFLECT_MAGIC);
						playSoundEntity(hit.entity, 166, 128);
						if ( hit.entity->behavior == &actPlayer )
						{
							if ( youAreHitByASpell )
							{
								if ( !spellIsReflectingMagic )
								{
									messagePlayer(player, MESSAGE_COMBAT, Language::get(379)); // but it bounces off!
								}
								else
								{
									messagePlayer(player, MESSAGE_COMBAT, Language::get(2475)); // but it bounces off!
								}
							}
							else
							{
								messagePlayer(player, MESSAGE_COMBAT, Language::get(4325)); // you reflected a spell!
							}
						}
					}
					if ( parent && hitstats )
					{
						if ( parent->behavior == &actPlayer )
						{
							if ( yourSpellHitsTheMonster )
							{
								messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(379)); // but it bounces off!
							}
							else
							{
								messagePlayerMonsterEvent(parent->skill[2], makeColorRGB(255, 255, 255), 
									*hitstats, Language::get(4322), Language::get(4323), MSG_COMBAT); // your spell bounces off the monster!
							}
						}
					}

					if ( hit.side == 0 || hit.entity )
					{
						my->vel_x *= -1;
						my->vel_y *= -1;
						my->yaw = atan2(my->vel_y, my->vel_x);
					}
					else if ( hit.side == HORIZONTAL )
					{
						my->vel_x *= -1;
						my->yaw = atan2(my->vel_y, my->vel_x);
					}
					else if ( hit.side == VERTICAL )
					{
						my->vel_y *= -1;
						my->yaw = atan2(my->vel_y, my->vel_x);
					}
					if ( hit.entity )
					{
						if ( (parent && parent->behavior == &actMagicTrapCeiling) || my->actmagicIsVertical == MAGIC_ISVERTICAL_Z )
						{
							// this missile came from the ceiling, let's redirect it..
							my->x = hit.entity->x + cos(hit.entity->yaw);
							my->y = hit.entity->y + sin(hit.entity->yaw);
							my->yaw = hit.entity->yaw;
							my->z = -1;
							my->vel_x = 4 * cos(hit.entity->yaw);
							my->vel_y = 4 * sin(hit.entity->yaw);
							my->vel_z = 0;
							my->actmagicIsVertical = 0;
							my->pitch = 0;
						}
						my->parent = hit.entity->getUID();
						++my->actmagicReflectionCount;
					}

					if ( bShouldEquipmentDegrade )
					{
						// Reflection of 3 does not degrade equipment
						bool chance = false;
						if ( my->actmagicSpray == 1 || my->actmagicSpray == 2 )
						{
							chance = local_rng.rand() % 10 == 0;
						}
						else
						{
							chance = local_rng.rand() % 2 == 0;
						}
						if ( chance && hitstats && reflection < 3 )
						{
							// set armornum to the relevant equipment slot to send to clients
							int armornum = 5 + reflection;
							if ( (player >= 0 && players[player]->isLocalPlayer()) || player < 0 )
							{
								if ( reflection == 1 )
								{
									if ( hitstats->cloak )
									{
										if ( hitstats->cloak->count > 1 )
										{
											newItem(hitstats->cloak->type, hitstats->cloak->status, hitstats->cloak->beatitude, hitstats->cloak->count - 1, hitstats->cloak->appearance, hitstats->cloak->identified, &hitstats->inventory);
										}
									}
								}
								else if ( reflection == 2 )
								{
									if ( hitstats->amulet )
									{
										if ( hitstats->amulet->count > 1 )
										{
											newItem(hitstats->amulet->type, hitstats->amulet->status, hitstats->amulet->beatitude, hitstats->amulet->count - 1, hitstats->amulet->appearance, hitstats->amulet->identified, &hitstats->inventory);
										}
									}
								}
								else if ( reflection == -1 )
								{
									if ( hitstats->shield )
									{
										if ( hitstats->shield->count > 1 )
										{
											newItem(hitstats->shield->type, hitstats->shield->status, hitstats->shield->beatitude, hitstats->shield->count - 1, hitstats->shield->appearance, hitstats->shield->identified, &hitstats->inventory);
										}
									}
								}
							}
							if ( reflection == 1 )
							{
								if ( hitstats->cloak 
									&& !hit.entity->spellEffectPreserveItem(hitstats->cloak) )
								{
									hitstats->cloak->count = 1;
									hitstats->cloak->status = static_cast<Status>(std::max(static_cast<int>(BROKEN), hitstats->cloak->status - 1));
									if ( hitstats->cloak->status != BROKEN )
									{
										messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(380));
									}
									else
									{
										messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(381));
										playSoundEntity(hit.entity, 76, 64);
										if ( player >= 0 )
										{
											Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_BROKEN, hitstats->cloak->type, 1);
										}
									}
								}
							}
							else if ( reflection == 2 )
							{
								if ( hitstats->amulet 
									&& !hit.entity->spellEffectPreserveItem(hitstats->amulet) )
								{
									hitstats->amulet->count = 1;
									hitstats->amulet->status = static_cast<Status>(std::max(static_cast<int>(BROKEN), hitstats->amulet->status - 1));
									if ( hitstats->amulet->status != BROKEN )
									{
										messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(382));
									}
									else
									{
										messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(383));
										playSoundEntity(hit.entity, 76, 64);
										if ( player >= 0 )
										{
											Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_BROKEN, hitstats->amulet->type, 1);
										}
									}
								}
							}
							else if ( reflection == -1 )
							{
								if ( hitstats->shield
									&& !hit.entity->spellEffectPreserveItem(hitstats->shield) )
								{
									hitstats->shield->count = 1;
									hitstats->shield->status = static_cast<Status>(std::max(static_cast<int>(BROKEN), hitstats->shield->status - 1));
									if ( hitstats->shield->status != BROKEN )
									{
										messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(384));
									}
									else
									{
										messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(385));
										playSoundEntity(hit.entity, 76, 64);
										if ( player >= 0 )
										{
											Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_BROKEN, hitstats->shield->type, 1);
										}
									}
								}
							}
							if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
							{
								strcpy((char*)net_packet->data, "ARMR");
								net_packet->data[4] = armornum;
								if ( reflection == 1 )
								{
									net_packet->data[5] = hitstats->cloak->status;
									SDLNet_Write16(hitstats->cloak->type, &net_packet->data[6]);
								}
								else if ( reflection == 2 )
								{
									net_packet->data[5] = hitstats->amulet->status;
									SDLNet_Write16(hitstats->amulet->type, &net_packet->data[6]);
								}
								else
								{
									net_packet->data[5] = hitstats->shield->status;
									SDLNet_Write16(hitstats->shield->type, &net_packet->data[6]);
								}
								net_packet->address.host = net_clients[player - 1].host;
								net_packet->address.port = net_clients[player - 1].port;
								net_packet->len = 8;
								sendPacketSafe(net_sock, -1, net_packet, player - 1);
							}
						}
					}

					if ( spellIsReflectingMagic )
					{
						int spellCost = getCostOfSpell(spell);
						bool unsustain = false;
						if ( spellCost >= hit.entity->getMP() ) //Unsustain the spell if expended all mana.
						{
							unsustain = true;
						}

						hit.entity->drainMP(spellCost);
						spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z / 2, 174);
						playSoundEntity(hit.entity, 166, 128); //TODO: Custom sound effect?

						if ( unsustain )
						{
							spellIsReflectingMagic->sustain = false;
							if ( hitstats )
							{
								hit.entity->setEffect(EFF_MAGICREFLECT, false, 0, true);
								messagePlayer(player, MESSAGE_STATUS, Language::get(2476));
							}
						}
					}

					if ( my->actmagicReflectionCount >= 3 )
					{
						my->removeLightField();
						list_RemoveNode(my->mynode);
						return;
					}

					if ( my->actmagicSpray == 2 )
					{
						if ( Entity* gib = spawnFociGib(my->x, my->y, 1.0, my->yaw, 0.0, hit.entity->getUID(), my->sprite, local_rng.rand()) )
						{
							gib->actmagicReflectionCount = my->actmagicReflectionCount;

							if ( spell )
							{
								node_t* node = list_AddNodeFirst(&gib->children);
								node->element = copySpell(spell);
								((spell_t*)node->element)->caster = hit.entity->getUID();
								node->deconstructor = &spellDeconstructor;
								node->size = sizeof(spell_t);
							}
						}

						my->removeLightField();
						list_RemoveNode(my->mynode);
						return;
					}
					return;
				}

				// Test for Friendly Fire, if Friendly Fire is OFF, delete the missile
				if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
				{
					if ( my->actmagicAllowFriendlyFireHit == 1
						|| my->actmagicTinkerTrapFriendlyFire == 1 )
					{
						// these spells can hit allies no penalty.
					}
					else if ( parent && parent->checkFriend(hit.entity) && parent->friendlyFireProtection(hit.entity) )
					{
						my->removeLightField();
						list_RemoveNode(my->mynode);
						return;
					}
				}

				// check for magic resistance...
				// resistance stacks diminishingly
				int trapResist = 0;
				int resistance = 0;
				DamageGib dmgGib = DMG_DEFAULT;
				real_t damageMultiplier = 1.0;
				magicSetResistance(hit.entity, parent, resistance, damageMultiplier, dmgGib, trapResist, spell->ID);

				bool alertTarget = false;
				bool alertAllies = false;

				// Alerting the hit Entity
				if ( hit.entity )
				{
					// alert the hit entity if it was a monster
					if ( hit.entity->behavior == &actMonster && parent != nullptr && parent->behavior != &actDeathGhost )
					{
						if ( parent->behavior == &actMagicTrap || parent->behavior == &actMagicTrapCeiling )
						{
							bool ignoreMoveAside = false;
							if ( trapResist == 100 )
							{
								ignoreMoveAside = true;
							}

							if ( parent->behavior == &actMagicTrap && !ignoreMoveAside )
							{
								if ( static_cast<int>(parent->y / 16) == static_cast<int>(hit.entity->y / 16) )
								{
									// avoid y axis.
									int direction = 1;
									if ( local_rng.rand() % 2 == 0 )
									{
										direction = -1;
									}
									if ( hit.entity->monsterSetPathToLocation(hit.entity->x / 16, (hit.entity->y / 16) + 1 * direction, 0,
										GeneratePathTypes::GENERATE_PATH_MOVEASIDE) )
									{
										hit.entity->monsterState = MONSTER_STATE_HUNT;
										serverUpdateEntitySkill(hit.entity, 0);
									}
									else if ( hit.entity->monsterSetPathToLocation(hit.entity->x / 16, (hit.entity->y / 16) - 1 * direction, 0,
										GeneratePathTypes::GENERATE_PATH_MOVEASIDE) )
									{
										hit.entity->monsterState = MONSTER_STATE_HUNT;
										serverUpdateEntitySkill(hit.entity, 0);
									}
									else
									{
										monsterMoveAside(hit.entity, hit.entity);
									}
								}
								else if ( static_cast<int>(parent->x / 16) == static_cast<int>(hit.entity->x / 16) )
								{
									int direction = 1;
									if ( local_rng.rand() % 2 == 0 )
									{
										direction = -1;
									}
									// avoid x axis.
									if ( hit.entity->monsterSetPathToLocation((hit.entity->x / 16) + 1 * direction, hit.entity->y / 16, 0,
										GeneratePathTypes::GENERATE_PATH_MOVEASIDE) )
									{
										hit.entity->monsterState = MONSTER_STATE_HUNT;
										serverUpdateEntitySkill(hit.entity, 0);
									}
									else if ( hit.entity->monsterSetPathToLocation((hit.entity->x / 16) - 1 * direction, hit.entity->y / 16, 0,
										GeneratePathTypes::GENERATE_PATH_MOVEASIDE) )
									{
										hit.entity->monsterState = MONSTER_STATE_HUNT;
										serverUpdateEntitySkill(hit.entity, 0);
									}
									else
									{
										monsterMoveAside(hit.entity, hit.entity);
									}
								}
								else
								{
									monsterMoveAside(hit.entity, hit.entity);
								}
							}
							else if ( !ignoreMoveAside )
							{
								monsterMoveAside(hit.entity, hit.entity);
							}
						}
						else
						{
							alertTarget = hit.entity->monsterAlertBeforeHit(parent);
							alertAllies = true;
							if ( !strcmp(element->element_internal_name, spellElement_telePull.element_internal_name) 
								|| !strcmp(element->element_internal_name, spellElement_shadowTag.element_internal_name) )
							{
								alertTarget = false;
								alertAllies = false;
							}

							if ( spell->ID == SPELL_HOLY_BEAM && parent && parent->checkFriend(hit.entity) )
							{
								alertTarget = false;
								alertAllies = false;
							}
							if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_NUMBING_BOLT].element_internal_name) )
							{
								alertTarget = false;
								alertAllies = false;
							}
							if ( hitstats->type == SHOPKEEPER && !strcmp(element->element_internal_name, spellElement_charmMonster.element_internal_name) )
							{
								if ( parent->behavior == &actPlayer )
								{
									alertTarget = false;
									alertAllies = false;
								}
							}
							/*if ( hitstats->type == SHOPKEEPER && parent->getMonsterTypeFromSprite() == SHOPKEEPER )
							{
								alertTarget = false;
							}*/
							if ( my->actmagicCastByTinkerTrap == 1 )
							{
								if ( entityDist(hit.entity, parent) > TOUCHRANGE )
								{
									// don't alert if bomb thrower far away.
									alertTarget = false;
									alertAllies = false;
								}
							}

							if ( spell->ID != SPELL_SLIME_TAR ) // processed manually later
							{
								if ( alertTarget && hit.entity->monsterState != MONSTER_STATE_ATTACK && (hitstats->type < LICH || hitstats->type >= SHOPKEEPER) )
								{
									hit.entity->monsterAcquireAttackTarget(*parent, MONSTER_STATE_PATH, true);
								}

								hit.entity->updateEntityOnHit(parent, alertTarget);
							}
							if ( parent->behavior == &actPlayer || parent->monsterAllyIndex != -1 )
							{
								if ( hit.entity->behavior == &actPlayer || (hit.entity->behavior == &actMonster && hit.entity->monsterAllyIndex != -1) )
								{
									// if a player ally + hit another ally or player, don't alert other allies.
									alertAllies = false;
								}
							}

							// alert other monsters too
							if ( alertAllies )
							{
								hit.entity->alertAlliesOnBeingHit(parent);
							}
						}
					}
				}

				real_t spellbookDamageBonus = (my->actmagicSpellbookBonus / 100.f);
				if ( parent && parent->behavior == &actDeathGhost )
				{
					// no extra bonus here
				}
				else
				{
					if ( my->actmagicCastByMagicstaff == 1 && spell && spell->ID == SPELL_SCEPTER_BLAST )
					{
						spellbookDamageBonus += getBonusFromCasterOfSpellElement(parent, nullptr, element, spell ? spell->ID : SPELL_NONE, spell->skillID);
					}
					if ( my->actmagicCastByMagicstaff != 1 && my->actmagicCastByTinkerTrap == 0 )
					{
						if ( spell->ID != SPELL_FORCEBOLT )
						{
							spellbookDamageBonus += getBonusFromCasterOfSpellElement(parent, nullptr, element, spell ? spell->ID : SPELL_NONE, spell->skillID);
						}
						if ( parent && parent->behavior == &actPlayer )
						{
							Compendium_t::Events_t::eventUpdateCodex(parent->skill[2], Compendium_t::CPDM_CLASS_PWR_MAX_CASTED, "pwr",
								100 + (Sint32)(spellbookDamageBonus * 100.0));
						}
						absorbMagicEvent(hit.entity, parent, *my, spell->ID, nullptr, damageMultiplier, dmgGib);
					}
					if ( parent && (parent->behavior == &actMagicTrap || parent->behavior == &actMagicTrapCeiling) )
					{
						if ( gameModeManager.currentSession.challengeRun.isActive(GameModeManager_t::CurrentSession_t::ChallengeRun_t::CHEVENT_STRONG_TRAPS) )
						{
							spellbookDamageBonus += 1.0;
						}
					}
				}

				int damageTmp = 0;
				Sint32 preResistanceDamageTmp = 0;
				{
					int magicDmg = element->getDamage();
					magicDmg += (spellbookDamageBonus * magicDmg * (abs(element->getDamageMult()) > 0.01 ? element->getDamageMult() : 1.0));
					if ( element->getDamageMult() > 0.01 && element->getDamage() > 0 )
					{
						// range checking for PWR penalties, if we should do _some_ damage, then do at least 1
						magicDmg = std::max(1, magicDmg);
					}
					if ( hit.entity && !mimic && (hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer) )
					{
						if ( my->actmagicIsOrbiting == 2 )
						{
							updateEntityOldHPBeforeMagicHit(*hit.entity, *my);

							if ( !strcmp(element->element_internal_name, spellElement_magicmissile.element_internal_name) )
							{
								if ( parent && my->actmagicOrbitCastFromSpell == 1 )
								{
									// cast through amplify magic effect
									real_t mult = getSpellDamageFromID(SPELL_AMPLIFY_MAGIC, parent, nullptr, my) / 100.0;
									mult = std::min(mult, getSpellDamageSecondaryFromID(SPELL_AMPLIFY_MAGIC, parent, nullptr, my) / 100.0);
									magicDmg *= mult;
								}
								magicDmg = magicDmg - local_rng.rand() % ((magicDmg / 8) + 1);
							}
							else if ( !strcmp(element->element_internal_name, spellElement_fire.element_internal_name) )
							{
								if ( my->actmagicIsOrbiting == 2 )
								{
									if ( parent && my->actmagicOrbitCastFromSpell == 0 )
									{
										if ( parent->behavior == &actParticleDot )
										{
											magicDmg = parent->skill[1];
										}
										else if ( parent->behavior == &actPlayer )
										{
											Stat* playerStats = parent->getStats();
											if ( playerStats )
											{
												int skillLVL = playerStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
												magicDmg = (14 + skillLVL * 1.5);
											}
										}
										else
										{
											magicDmg = 14;
										}
									}
									else if ( parent && my->actmagicOrbitCastFromSpell == 1 )
									{
										// cast through amplify magic effect
										real_t mult = getSpellDamageFromID(SPELL_AMPLIFY_MAGIC, parent, nullptr, my) / 100.0;
										mult = std::min(mult, getSpellDamageSecondaryFromID(SPELL_AMPLIFY_MAGIC, parent, nullptr, my) / 100.0);
										magicDmg *= mult;
									}
									else
									{
										magicDmg = 14;
									}
									magicDmg = magicDmg - local_rng.rand() % ((magicDmg / 8) + 1);
								}
							}
							else if ( !strcmp(element->element_internal_name, spellElement_cold.element_internal_name) )
							{
								if ( my->actmagicIsOrbiting == 2 )
								{
									if ( parent && my->actmagicOrbitCastFromSpell == 0 )
									{
										if ( parent->behavior == &actParticleDot )
										{
											magicDmg = parent->skill[1];
										}
										else if ( parent->behavior == &actPlayer )
										{
											Stat* playerStats = parent->getStats();
											if ( playerStats )
											{
												int skillLVL = playerStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
												magicDmg = (18 + skillLVL * 1.5);
											}
										}
										else
										{
											magicDmg = 18;
										}
									}
									else if ( parent && my->actmagicOrbitCastFromSpell == 1 )
									{
										// cast through amplify magic effect
										real_t mult = getSpellDamageFromID(SPELL_AMPLIFY_MAGIC, parent, nullptr, my) / 100.0;
										mult = std::min(mult, getSpellDamageSecondaryFromID(SPELL_AMPLIFY_MAGIC, parent, nullptr, my) / 100.0);
										magicDmg *= mult;
									}
									else
									{
										magicDmg = 18;
									}
									magicDmg = magicDmg - local_rng.rand() % ((magicDmg / 8) + 1);
								}
							}
							else if ( !strcmp(element->element_internal_name, spellElement_lightning.element_internal_name) )
							{
								if ( my->actmagicIsOrbiting == 2 )
								{
									if ( parent && my->actmagicOrbitCastFromSpell == 0 )
									{
										if ( parent->behavior == &actParticleDot )
										{
											magicDmg = parent->skill[1];
										}
										else if ( parent->behavior == &actPlayer )
										{
											Stat* playerStats = parent->getStats();
											if ( playerStats )
											{
												int skillLVL = playerStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
												magicDmg = (12 + skillLVL * 1.5);
											}
										}
										else
										{
											magicDmg = 12;
										}
									}
									else if ( parent && my->actmagicOrbitCastFromSpell == 1 )
									{
										// cast through amplify magic effect
										real_t mult = getSpellDamageFromID(SPELL_AMPLIFY_MAGIC, parent, nullptr, my) / 100.0;
										mult = std::min(mult, getSpellDamageSecondaryFromID(SPELL_AMPLIFY_MAGIC, parent, nullptr, my) / 100.0);
										magicDmg *= mult;
									}
									else
									{
										magicDmg = 12;
									}
									magicDmg = magicDmg - local_rng.rand() % ((magicDmg / 8) + 1);
								}
							}
						}

						if ( Stat* hitstats = hit.entity->getStats() )
						{
							if ( spell->ID == SPELL_SLIME_WATER )
							{
								if ( hitstats->type == VAMPIRE )
								{
									magicDmg *= 2;
								}
							}
						}
					}

					Sint32 preResistanceDamage = magicDmg;
					if ( hit.entity && (hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer) )
					{
						if ( mimic )
						{
							//magicDmg = convertResistancePointsToMagicValue(magicDmg, resistance);
						}
						else
						{
							if ( spell->ID == SPELL_LIGHTNING || spell->ID == SPELL_LIGHTNING_BOLT || spell->ID == SPELL_LIGHTNING_NEXUS
								|| spell->ID == SPELL_FOCI_ARCS )
							{
								if ( hitstats && hitstats->getEffectActive(EFF_STATIC) )
								{
									int extraDamage = getSpellDamageSecondaryFromID(spell->ID, parent, nullptr, my);
									if ( extraDamage > 0 )
									{
										extraDamage *= getSpellDamageFromStatic(spell->ID, hitstats);
										if ( my->actmagicIsOrbiting == 2 )
										{
											if ( parent && my->actmagicOrbitCastFromSpell == 1 )
											{
												// cast through amplify magic effect
												real_t mult = getSpellDamageFromID(SPELL_AMPLIFY_MAGIC, parent, nullptr, my) / 100.0;
												mult = std::min(mult, getSpellDamageSecondaryFromID(SPELL_AMPLIFY_MAGIC, parent, nullptr, my) / 100.0);
												extraDamage *= mult;
											}
										}
										magicDmg += std::max(1, extraDamage);
									}
								}
							}

							Entity::modifyDamageMultipliersFromEffects(hit.entity, parent, damageMultiplier, DAMAGE_TABLE_MAGIC, my, spell->ID);

							magicDmg *= damageMultiplier;

							if ( parent )
							{
								if ( Stat* casterStats = parent->getStats() )
								{
									if ( casterStats && casterStats->type == LICH_FIRE && parent->monsterLichAllyStatus == LICH_ALLY_DEAD )
									{
										if ( !strcmp(element->element_internal_name, spellElement_bleed.element_internal_name)
											|| !strcmp(element->element_internal_name, spellElement_fire.element_internal_name) )
										{
											magicDmg *= 2;
										}
									}
								}
							}

							if ( spell->ID == SPELL_HOLY_BEAM )
							{
								if ( hit.entity->isSmiteWeakMonster() )
								{
									magicDmg *= 2;
								}
							}
							if ( !strcmp(element->element_internal_name, spellElement_cold.element_internal_name)
								|| spell->ID == SPELL_FOCI_SNOW )
							{
								real_t coldMultiplier = 1.0;
								if ( hitstats && hitstats->helmet && hitstats->helmet->type == HAT_WARM )
								{
									if ( !(hit.entity->behavior == &actPlayer && hit.entity->effectShapeshift != NOTHING) )
									{
										if ( hitstats->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(hitstats) )
										{
											coldMultiplier = std::max(0.0, 0.5 - 0.25 * (abs(hitstats->helmet->beatitude)));
										}
										else
										{
											coldMultiplier = 0.50;
										}
									}
								}
								magicDmg *= coldMultiplier;
							}
							if ( !strcmp(element->element_internal_name, spellElement_fire.element_internal_name)
								|| !strcmp(element->element_internal_name, "spell_element_flames")
								|| spell->ID == SPELL_FOCI_FIRE
								|| spell->ID == SPELL_BREATHE_FIRE
								|| !strcmp(element->element_internal_name, spellElementMap[SPELL_METEOR_SHOWER].element_internal_name)
								|| !strcmp(element->element_internal_name, spellElementMap[SPELL_METEOR].element_internal_name) )
							{
								real_t fireMultiplier = 1.0;
								//if ( hitstats->helmet && hitstats->helmet->type == HAT_WARM )
								//{
								//	if ( !(hit.entity->behavior == &actPlayer && hit.entity->effectShapeshift != NOTHING) )
								//	{
								//		if ( hitstats->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(hitstats) )
								//		{
								//			fireMultiplier += 0.5;
								//		}
								//		else
								//		{
								//			fireMultiplier += 0.5 + 0.5 * abs(hitstats->helmet->beatitude); // cursed, extra fire damage
								//		}
								//	}
								//}
								if ( hitstats->type == MONSTER_D )
								{
									fireMultiplier += 0.2;
									if ( !hitstats->helmet && hitstats->getEffectActive(EFF_GROWTH) > 1 )
									{
										int bonus = std::min(3, hitstats->getEffectActive(EFF_GROWTH) - 1);
										fireMultiplier += 0.05;
									}
								}
								if ( hitstats->type == MONSTER_S )
								{
									if ( hitstats->getEffectActive(EFF_SALAMANDER_HEART) == 1
										|| hitstats->getEffectActive(EFF_SALAMANDER_HEART) == 2 )
									{
										fireMultiplier *= 0.25;
									}
									else
									{
										fireMultiplier *= 0.5;
									}
								}
								magicDmg *= fireMultiplier;
							}
						}
					}

					if ( spell->ID >= SPELL_SLIME_ACID && spell->ID <= SPELL_SLIME_METAL || spell->ID == SPELL_GREASE_SPRAY )
					{
						preResistanceDamage = std::max(2, preResistanceDamage);
						magicDmg = std::max(2, magicDmg);
					}
					else if ( spell->ID == SPELL_FOCI_FIRE || spell->ID == SPELL_FOCI_ARCS || spell->ID == SPELL_FOCI_SNOW
						|| spell->ID == SPELL_FOCI_NEEDLES || spell->ID == SPELL_FOCI_SANDBLAST
						|| spell->ID == SPELL_BREATHE_FIRE )
					{
						preResistanceDamage = std::max(1, preResistanceDamage);
						magicDmg = std::max(1, magicDmg);
					}
					else if ( !strcmp(element->element_internal_name, "spell_element_flames") )
					{
						preResistanceDamage = std::max(2, preResistanceDamage);
						magicDmg = std::max(element->getDamage(), magicDmg);
					}
					damageTmp = magicDmg;
					preResistanceDamageTmp = preResistanceDamage;
				}
				const Sint32 preResistanceDamage = preResistanceDamageTmp;
				const int damage = damageTmp;

				if ( hit.entity && hitstats && hitstats->getEffectActive(EFF_ABSORB_MAGIC) )
				{
					Uint8 effectStrength = hitstats->getEffectActive(EFF_ABSORB_MAGIC);
					if ( effectStrength >= 101 )
					{
						if ( hit.entity->behavior == &actPlayer )
						{
							messagePlayerColor(hit.entity->skill[2], MESSAGE_STATUS, makeColorRGB(255, 0, 0), Language::get(6856));
						}
					}
					else
					{
						int increase = std::max(damage, getSpellDamageFromID(SPELL_ABSORB_MAGIC, hit.entity, nullptr, hit.entity));
						effectStrength = std::min(101, (int)(effectStrength + increase));
						hit.entity->setEffect(EFF_ABSORB_MAGIC, effectStrength, hitstats->EFFECTS_TIMERS[EFF_ABSORB_MAGIC], false, true, true);
						if ( hit.entity->behavior == &actPlayer )
						{
							messagePlayerColor(hit.entity->skill[2], MESSAGE_STATUS, makeColorRGB(0, 255, 0), Language::get(6857));
						}

						Entity* fx = createParticleAestheticOrbit(hit.entity, 1817, TICKS_PER_SECOND / 4, PARTICLE_EFFECT_NULL_PARTICLE);
						fx->x = my->x;
						fx->y = my->y;
						fx->z = my->z;
						real_t tangent = atan2(my->y - hit.entity->y, my->x - hit.entity->x);
						fx->yaw = tangent;
						fx->actmagicOrbitDist = 0;
						fx->actmagicNoLight = 0;
						serverSpawnMiscParticlesAtLocation(fx->x, fx->y, fx->z, PARTICLE_EFFECT_NULL_PARTICLE, 1817, 0, fx->yaw * 256.0);

						magicOnSpellCastEvent(hit.entity, nullptr, parent, SPELL_ABSORB_MAGIC, spell_t::SPELL_LEVEL_EVENT_DEFAULT | spell_t::SPELL_LEVEL_EVENT_MINOR_CHANCE, 1);

						my->removeLightField();
						list_RemoveNode(my->mynode);
						return;
					}
				}

				if (!strcmp(element->element_internal_name, spellElement_force.element_internal_name)
					|| !strcmp(element->element_internal_name, spellElementMap[SPELL_MERCURY_BOLT].element_internal_name)
					|| !strcmp(element->element_internal_name, spellElementMap[SPELL_LEAD_BOLT].element_internal_name)
					|| !strcmp(element->element_internal_name, spellElementMap[SPELL_SPORE_BOMB].element_internal_name)
					|| !strcmp(element->element_internal_name, spellElementMap[SPELL_MYCELIUM_BOMB].element_internal_name)
					|| spell->ID == SPELL_FOCI_ARCS
					|| spell->ID == SPELL_FOCI_SANDBLAST
					|| spell->ID == SPELL_FOCI_NEEDLES
					|| spell->ID == SPELL_FOCI_SNOW )
				{
					if (hit.entity)
					{
						bool doSound = true;
						int customSoundVolume = 0;
						if ( spell->ID == SPELL_FOCI_ARCS
							|| spell->ID == SPELL_FOCI_SANDBLAST
							|| spell->ID == SPELL_FOCI_NEEDLES
							|| spell->ID == SPELL_FOCI_SNOW )
						{
							if ( ticks - lastMagicSoundPlayed[spell->ID] < 10 )
							{
								doSound = false;
							}
							else if ( ticks - lastMagicSoundPlayed[spell->ID] < 25 )
							{
								doSound = false;
								customSoundVolume = 32;
							}
							else
							{
								lastMagicSoundPlayed[spell->ID] = ticks;
							}
						}

						if ( mimic )
						{
							hit.entity->chestHandleDamageMagic(damage, *my, parent, doSound);
							if ( !doSound && customSoundVolume > 0 )
							{
								playSoundEntity(hit.entity, 28, customSoundVolume);
							}
							if ( spell->ID == SPELL_SPORE_BOMB || spell->ID == SPELL_MYCELIUM_BOMB )
							{
								floorMagicCreateSpores(hit.entity, my->x, my->y, parent, damage, spell->ID);
							}

							my->removeLightField();
							list_RemoveNode(my->mynode);
							return;
						}
						else if (hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer)
						{
							Entity* parent = uidToEntity(my->parent);
							if ( doSound )
							{
								playSoundEntity(hit.entity, 28, 128);
							}
							else if ( !doSound && customSoundVolume > 0 )
							{
								playSoundEntity(hit.entity, 28, customSoundVolume);
							}

							Sint32 oldHP = hitstats->HP;
							hit.entity->modHP(-damage);
							magicOnEntityHit(parent, my, hit.entity, hitstats, preResistanceDamage, damage, oldHP, spell ? spell->ID : SPELL_NONE);
							magicTrapOnHit(parent, hit.entity, hitstats, oldHP, spell ? spell->ID : SPELL_NONE);

							int numGibs = damage / 2;
							numGibs = std::min(5, numGibs);
							if ( my->actmagicSpray == 2 )
							{
								numGibs = std::min(1, numGibs);
							}
							for ( int i = 0; i < numGibs; ++i)   //Spawn a gib for every two points of damage.
							{
								Entity* gib = spawnGib(hit.entity);
								serverSpawnGibForClient(gib);
							}

							if (parent)
							{
								parent->killedByMonsterObituary(hit.entity);
							}


							if ( spell->ID == SPELL_FOCI_SANDBLAST )
							{
								if ( !hitstats->getEffectActive(EFF_KNOCKBACK) && hit.entity->setEffect(EFF_KNOCKBACK, true, element->duration, false) )
								{
									real_t pushbackMultiplier = 0.3;
									if ( parent )
									{
										real_t dist = entityDist(parent, hit.entity);
										if ( dist < TOUCHRANGE )
										{
											pushbackMultiplier += 0.5;
										}
									}
									if ( hit.entity->behavior == &actMonster )
									{
										real_t tangent = atan2(hit.entity->y - my->y, hit.entity->x - my->x);
										hit.entity->vel_x = cos(tangent) * pushbackMultiplier;
										hit.entity->vel_y = sin(tangent) * pushbackMultiplier;
										hit.entity->monsterKnockbackVelocity = 0.01;
										hit.entity->monsterKnockbackTangentDir = tangent;
										hit.entity->monsterKnockbackUID = parent ? parent->getUID() : 0;
									}
									else if ( hit.entity->behavior == &actPlayer )
									{
										if ( !players[hit.entity->skill[2]]->isLocalPlayer() )
										{
											hit.entity->monsterKnockbackVelocity = pushbackMultiplier;
											hit.entity->monsterKnockbackTangentDir = my->yaw;
											serverUpdateEntityFSkill(hit.entity, 11);
											serverUpdateEntityFSkill(hit.entity, 9);
										}
										else
										{
											hit.entity->monsterKnockbackVelocity = pushbackMultiplier;
											hit.entity->monsterKnockbackTangentDir = my->yaw;
										}
									}
								}
							}
							else if ( spell->ID == SPELL_FOCI_SNOW )
							{
								Sint32 duration = element->duration;
								duration = convertResistancePointsToMagicValue(duration, resistance);
								int prevDuration = hitstats->getEffectActive(EFF_SLOW) ? hitstats->EFFECTS_TIMERS[EFF_SLOW] : 0;
								if ( hit.entity->setEffect(EFF_SLOW, true, 
									std::min(element->getDurationSecondary(), prevDuration + duration), false) )
								{
									/*if ( prevDuration )
									{
										playSoundEntity(hit.entity, 396 + local_rng.rand() % 3, 64);
									}*/
								}
							}
							else if ( spell->ID == SPELL_FOCI_ARCS )
							{
								Sint32 duration = element->duration;
								duration = convertResistancePointsToMagicValue(duration, resistance);

								int prevDuration = hitstats->getEffectActive(EFF_STATIC) ? hitstats->EFFECTS_TIMERS[EFF_STATIC] : 0;
								int maxDuration = element->getDurationSecondary();
								bool updateClient = !hitstats->getEffectActive(EFF_STATIC);

								duration = std::min(prevDuration + duration, maxDuration);
								Uint8 effectStrength = 1;
								if ( duration >= 8 * element->duration )
								{
									effectStrength = 3;
								}
								else if ( duration >= 4 * element->duration )
								{
									effectStrength = 2;
								}

								if ( hit.entity->setEffect(EFF_STATIC, effectStrength, duration, updateClient) )
								{

								}
							}
							else if ( spell->ID == SPELL_FOCI_NEEDLES )
							{
								Sint32 duration = element->duration;
								duration = convertResistancePointsToMagicValue(duration, resistance);
								int prevDuration = hitstats->getEffectActive(EFF_BLEEDING) ? hitstats->EFFECTS_TIMERS[EFF_BLEEDING] : 0;
								if ( hit.entity->setEffect(EFF_BLEEDING, true, 
									std::min(element->getDurationSecondary(), prevDuration + duration), false) )
								{
									//playSoundEntity(hit.entity, 396 + local_rng.rand() % 3, 64);
									if ( parent )
									{
										hitstats->bleedInflictedBy = parent->getUID();
									}
								}
							}
							else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_MERCURY_BOLT].element_internal_name)
								|| !strcmp(element->element_internal_name, spellElementMap[SPELL_LEAD_BOLT].element_internal_name) )
							{
								if ( hit.entity->setEffect(EFF_KNOCKBACK, true, 30, false) )
								{
									real_t pushbackMultiplier = 0.6;
									if ( parent )
									{
										real_t dist = entityDist(parent, hit.entity);
										if ( dist < TOUCHRANGE )
										{
											pushbackMultiplier += 0.5;
										}
									}
									if ( hit.entity->behavior == &actMonster )
									{
										real_t tangent = atan2(hit.entity->y - my->y, hit.entity->x - my->x);
										hit.entity->vel_x = cos(tangent) * pushbackMultiplier;
										hit.entity->vel_y = sin(tangent) * pushbackMultiplier;
										hit.entity->monsterKnockbackVelocity = 0.01;
										hit.entity->monsterKnockbackTangentDir = tangent;
										hit.entity->monsterKnockbackUID = parent ? parent->getUID() : 0;
									}
									else if ( hit.entity->behavior == &actPlayer )
									{
										if ( !players[hit.entity->skill[2]]->isLocalPlayer() )
										{
											hit.entity->monsterKnockbackVelocity = pushbackMultiplier;
											hit.entity->monsterKnockbackTangentDir = my->yaw;
											serverUpdateEntityFSkill(hit.entity, 11);
											serverUpdateEntityFSkill(hit.entity, 9);
										}
										else
										{
											hit.entity->monsterKnockbackVelocity = pushbackMultiplier;
											hit.entity->monsterKnockbackTangentDir = my->yaw;
										}
									}
								}

								if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_MERCURY_BOLT].element_internal_name) )
								{
									Sint32 duration = element->duration;
									duration = convertResistancePointsToMagicValue(duration, resistance);
									if ( hit.entity->setEffect(EFF_SLOW, true, duration, false) )
									{
										playSoundEntity(hit.entity, 396 + local_rng.rand() % 3, 64);
									}

									duration = element->duration;
									duration = convertResistancePointsToMagicValue(duration, resistance);
									if ( hit.entity->setEffect(EFF_POISONED, true, duration, false) )
									{
										hitstats->poisonKiller = my->parent;
									}
								}

							}

							if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_SPORE_BOMB].element_internal_name) )
							{
								Sint32 duration = 6 * TICKS_PER_SECOND + 10;
								duration = convertResistancePointsToMagicValue(duration, resistance);
								if ( hit.entity->setEffect(EFF_SLOW, true, duration, false, true, false, false) )
								{
									playSoundEntity(hit.entity, 396 + local_rng.rand() % 3, 64);
								}

								duration = 6 * TICKS_PER_SECOND + 10;
								duration = convertResistancePointsToMagicValue(duration, resistance);
								if ( hit.entity->setEffect(EFF_POISONED, true, duration, false, true, false, false) )
								{
									hitstats->poisonKiller = my->parent;
								}
							}
							else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_MYCELIUM_BOMB].element_internal_name) )
							{
								Sint32 duration = 6 * TICKS_PER_SECOND + 10;
								duration = convertResistancePointsToMagicValue(duration, resistance);
								if ( hit.entity->setEffect(EFF_SLOW, true, duration, false, true, false, false) )
								{
									playSoundEntity(hit.entity, 396 + local_rng.rand() % 3, 64);
								}
							}

							// update enemy bar for attacker
							if ( !strcmp(hitstats->name, "") )
							{
								updateEnemyBar(parent, hit.entity, getMonsterLocalizedName(hitstats->type).c_str(), hitstats->HP, hitstats->MAXHP,
									false, dmgGib);
							}
							else
							{
								updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP,
									false, dmgGib);
							}

							if ( hitstats->HP <= 0 && parent)
							{
								parent->awardXP( hit.entity, true, true );
								spawnBloodVialOnMonsterDeath(hit.entity, hitstats, parent);
							}
						}
						else if (hit.entity->behavior == &actDoor || hit.entity->behavior == &actIronDoor )
						{
							hit.entity->doorHandleDamageMagic(damage, *my, parent, true, doSound);
							if ( !doSound && customSoundVolume > 0 )
							{
								playSoundEntity(hit.entity, 28, customSoundVolume);
							}
							if ( spell->ID == SPELL_SPORE_BOMB || spell->ID == SPELL_MYCELIUM_BOMB )
							{
								floorMagicCreateSpores(hit.entity, my->x, my->y, parent, damage, spell->ID);
							}

							my->removeLightField();
							list_RemoveNode(my->mynode);
							return;
						}
						else if ( hit.entity->isDamageableCollider() && hit.entity->isColliderDamageableByMagic() )
						{
							hit.entity->colliderHandleDamageMagic(damage, *my, parent, true, doSound);
							if ( !doSound && customSoundVolume > 0 )
							{
								playSoundEntity(hit.entity, hit.entity->getColliderSfxOnHit() > 0 ? 
									hit.entity->getColliderSfxOnHit() : 28, customSoundVolume);
							}
							if ( spell->ID == SPELL_SPORE_BOMB || spell->ID == SPELL_MYCELIUM_BOMB )
							{
								floorMagicCreateSpores(hit.entity, my->x, my->y, parent, damage, spell->ID);
							}

							my->removeLightField();
							list_RemoveNode(my->mynode);
							return;
						}
						else if ( hit.entity->behavior == &actChest )
						{
							hit.entity->chestHandleDamageMagic(damage, *my, parent, doSound);
							if ( !doSound && customSoundVolume > 0 )
							{
								playSoundEntity(hit.entity, 28, customSoundVolume);
							}
							if ( spell->ID == SPELL_SPORE_BOMB || spell->ID == SPELL_MYCELIUM_BOMB )
							{
								floorMagicCreateSpores(hit.entity, my->x, my->y, parent, damage, spell->ID);
							}

							my->removeLightField();
							list_RemoveNode(my->mynode);
							return;
						}
						else if (hit.entity->behavior == &actFurniture )
						{
							hit.entity->furnitureHandleDamageMagic(damage, *my, parent, true, doSound);
							if ( !doSound && customSoundVolume > 0 )
							{
								playSoundEntity(hit.entity, 28, customSoundVolume);
							}
						}
					}
				}
				else if (!strcmp(element->element_internal_name, spellElement_magicmissile.element_internal_name)
					|| spell->ID == SPELL_SCEPTER_BLAST
					|| spell->ID == SPELL_BLOOD_WAVES
					|| spell->ID == SPELL_HOLY_BEAM )
				{
					if ( spell->ID == SPELL_HOLY_BEAM )
					{
						bool doSound = true;
						if ( ticks - lastMagicSoundPlayed[spell->ID] < 10 )
						{
							doSound = false;
						}
						else
						{
							lastMagicSoundPlayed[spell->ID] = ticks;
						}
						if ( hit.entity && !(parent && parent->checkFriend(hit.entity)) )
						{
							createParticleFociLight(hit.entity, SPELL_HOLY_BEAM, true);
							if ( doSound )
							{
								playSoundEntity(hit.entity, 849, 128);
							}
						}
						/*else
						{
							createParticleFociLight(my, SPELL_HOLY_BEAM, true);
							if ( doSound )
							{
								playSoundEntity(my, 849, 128);
							}
						}*/
					}
					else if ( spell->ID == SPELL_BLOOD_WAVES )
					{
						playSoundEntity(my, 173, 128);
						for ( int i = 0; i < 4; ++i )
						{
							if ( Entity* gib = spawnGib(my, 5) )
							{
								gib->sprite = 5;
							}

							Entity* fx = createParticleAestheticOrbit(nullptr, 283, 1.5 * TICKS_PER_SECOND + i * 10, PARTICLE_EFFECT_BLOOD_BUBBLE);
							real_t dir = (local_rng.rand() % 360) * PI / 180.f;
							fx->x = (hit.entity ? hit.entity->x : my->x) + 4.0 * cos(dir);
							fx->y = (hit.entity ? hit.entity->y : my->y) + 4.0 * sin(dir);
							fx->z = my->z - (local_rng.rand() % 5);
							fx->flags[SPRITE] = true;

							fx->fskill[2] = 2 * PI * (local_rng.rand() % 10) / 10.0;
							fx->fskill[3] = 0.025; // speed osc
							fx->scalex = 0.0125;
							fx->scaley = fx->scalex;
							fx->scalez = fx->scalex;
							fx->actmagicOrbitDist = 2;
							fx->actmagicOrbitStationaryX = my->x;
							fx->actmagicOrbitStationaryY = my->y;
						}
						if ( hit.entity )
						{
							spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my->sprite);
							serverSpawnMiscParticlesAtLocation(hit.entity->x, hit.entity->y, my->z, PARTICLE_EFFECT_BLOOD_BUBBLE, 283);
						}
						else
						{
							spawnMagicEffectParticles(my->x, my->y, my->z, my->sprite);
							serverSpawnMiscParticlesAtLocation(my->x, my->y, my->z, PARTICLE_EFFECT_BLOOD_BUBBLE, 283);
						}
					}
					else if ( spell->ID == SPELL_SCEPTER_BLAST )
					{
						spawnExplosionFromSprite(135, my->x, my->y, my->z);
					}
					else
					{
						spawnExplosion(my->x, my->y, my->z);
					}
					if (hit.entity)
					{
						if ( mimic )
						{
							hit.entity->chestHandleDamageMagic(damage, *my, parent);
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}

							if ( spell->ID == SPELL_SCEPTER_BLAST || spell->ID == SPELL_BLOOD_WAVES || spell->ID == SPELL_HOLY_BEAM )
							{
								if ( hit.entity->getStats() && hit.entity->getStats()->HP > 0 )
								{
									my->removeLightField();
									list_RemoveNode(my->mynode);
								}
								else
								{
									my->collisionIgnoreTargets.insert(hit.entity->getUID());
								}
							}
							else if ( !(my->actmagicIsOrbiting == 2) )
							{
								my->removeLightField();
								list_RemoveNode(my->mynode);
							}
							else
							{
								spawnExplosion(my->x, my->y, my->z);
							}
							return;
						}
						else if (hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer)
						{
							Entity* parent = uidToEntity(my->parent);

							if ( spell->ID == SPELL_SCEPTER_BLAST || spell->ID == SPELL_BLOOD_WAVES || spell->ID == SPELL_HOLY_BEAM )
							{
								my->collisionIgnoreTargets.insert(hit.entity->getUID());
							}

							if ( my->actmagicIsOrbiting == 2 )
							{
								spawnExplosion(my->x, my->y, my->z);
							}

							if ( spell->ID == SPELL_HOLY_BEAM && parent && parent->checkFriend(hit.entity) )
							{
								Sint32 oldHP = hitstats->HP;

								int holyHeal = getSpellDamageSecondaryFromID(SPELL_HOLY_BEAM, parent, parent ? parent->getStats() : nullptr, my);
								spell_changeHealth(hit.entity, holyHeal);
								int heal = std::max(hitstats->HP - oldHP, 0);
								if ( heal >= 0 )
								{
									if ( heal > 0 )
									{
										spawnDamageGib(hit.entity, -heal, DamageGib::DMG_HEAL, DamageGibDisplayType::DMG_GIB_NUMBER, true);
									}
									playSoundEntity(hit.entity, 168, 128);
									spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, 169);

									if ( parent && parent->behavior == &actPlayer && heal > 0 )
									{
										serverUpdatePlayerGameplayStats(parent->skill[2], STATISTICS_HEAL_BOT, heal);
										players[parent->skill[2]]->mechanics.updateSustainedSpellEvent(spell->ID, heal, 1.0);
									}
								}
							}
							else
							{
								playSoundEntity(hit.entity, 28, 128);

								Sint32 oldHP = hitstats->HP;
								hit.entity->modHP(-damage);
								magicTrapOnHit(parent, hit.entity, hitstats, oldHP, spell ? spell->ID : SPELL_NONE);
								magicOnEntityHit(parent, my, hit.entity, hitstats, preResistanceDamage, damage, oldHP, spell ? spell->ID : SPELL_NONE);
								int numGibs = damage / 2;
								numGibs = std::min(5, numGibs);
								for ( int i = 0; i < numGibs; ++i )   //Spawn a gib for every two points of damage.
								{
									Entity* gib = spawnGib(hit.entity);
									serverSpawnGibForClient(gib);
								}

								// write the obituary
								if ( parent )
								{
									parent->killedByMonsterObituary(hit.entity);
								}

								// update enemy bar for attacker
								if ( !strcmp(hitstats->name, "") )
								{
									updateEnemyBar(parent, hit.entity, getMonsterLocalizedName(hitstats->type).c_str(), hitstats->HP, hitstats->MAXHP,
										false, dmgGib);
								}
								else
								{
									updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP,
										false, dmgGib);
								}

								if ( oldHP > 0 && hitstats->HP <= 0 && parent)
								{
									parent->awardXP( hit.entity, true, true );
									spawnBloodVialOnMonsterDeath(hit.entity, hitstats, parent);
								}
							}
						}
						else if ( hit.entity->behavior == &actDoor || hit.entity->behavior == &actIronDoor )
						{
							hit.entity->doorHandleDamageMagic(damage, *my, parent);
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}

							if ( spell->ID == SPELL_SCEPTER_BLAST || spell->ID == SPELL_BLOOD_WAVES || spell->ID == SPELL_HOLY_BEAM )
							{
								if ( hit.entity->doorHealth > 0 )
								{
									my->removeLightField();
									list_RemoveNode(my->mynode);
								}
								else
								{
									my->collisionIgnoreTargets.insert(hit.entity->getUID());
								}
							}
							else if ( !(my->actmagicIsOrbiting == 2) )
							{
								my->removeLightField();
								list_RemoveNode(my->mynode);
							}
							else
							{
								spawnExplosion(my->x, my->y, my->z);
							}
							return;
						}
						else if ( hit.entity->isDamageableCollider() && hit.entity->isColliderDamageableByMagic() )
						{
							hit.entity->colliderHandleDamageMagic(damage, *my, parent);
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}

							if ( spell->ID == SPELL_SCEPTER_BLAST || spell->ID == SPELL_BLOOD_WAVES || spell->ID == SPELL_HOLY_BEAM )
							{
								if ( hit.entity->colliderCurrentHP > 0 )
								{
									my->removeLightField();
									list_RemoveNode(my->mynode);
								}
								else
								{
									my->collisionIgnoreTargets.insert(hit.entity->getUID());
								}
							}
							else if ( !(my->actmagicIsOrbiting == 2) )
							{
								my->removeLightField();
								list_RemoveNode(my->mynode);
							}
							else
							{
								spawnExplosion(my->x, my->y, my->z);
							}
							return;
						}
						else if ( hit.entity->behavior == &actChest )
						{
							hit.entity->chestHandleDamageMagic(damage, *my, parent);
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}

							if ( spell->ID == SPELL_SCEPTER_BLAST || spell->ID == SPELL_BLOOD_WAVES || spell->ID == SPELL_HOLY_BEAM )
							{
								if ( hit.entity->chestHealth > 0 )
								{
									my->removeLightField();
									list_RemoveNode(my->mynode);
								}
								else
								{
									my->collisionIgnoreTargets.insert(hit.entity->getUID());
								}
							}
							else if ( !(my->actmagicIsOrbiting == 2) )
							{
								my->removeLightField();
								list_RemoveNode(my->mynode);
							}
							else
							{
								spawnExplosion(my->x, my->y, my->z);
							}
							return;
						}
						else if (hit.entity->behavior == &actFurniture )
						{
							hit.entity->furnitureHandleDamageMagic(damage, *my, parent);
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}

							if ( spell->ID == SPELL_SCEPTER_BLAST || spell->ID == SPELL_BLOOD_WAVES || spell->ID == SPELL_HOLY_BEAM )
							{
								if ( hit.entity->furnitureHealth > 0 )
								{
									my->removeLightField();
									list_RemoveNode(my->mynode);
								}
								else
								{
									my->collisionIgnoreTargets.insert(hit.entity->getUID());
								}
							}
							else if ( !(my->actmagicIsOrbiting == 2) )
							{
								my->removeLightField();
								list_RemoveNode(my->mynode);
							}
							else
							{
								spawnExplosion(my->x, my->y, my->z);
							}
							return;
						}
					}
				}
				else if (!strcmp(element->element_internal_name, spellElement_fire.element_internal_name)
					|| !strcmp(element->element_internal_name, "spell_element_flames")
					|| !strcmp(element->element_internal_name, spellElementMap[SPELL_METEOR_SHOWER].element_internal_name)
					|| !strcmp(element->element_internal_name, spellElementMap[SPELL_METEOR].element_internal_name)
					|| spell->ID == SPELL_FOCI_FIRE 
					|| spell->ID == SPELL_BREATHE_FIRE )
				{
					bool explode = !strcmp(element->element_internal_name, spellElement_fire.element_internal_name)
						|| !strcmp(element->element_internal_name, spellElementMap[SPELL_METEOR_SHOWER].element_internal_name)
						|| !strcmp(element->element_internal_name, spellElementMap[SPELL_METEOR].element_internal_name);
					if ( !(my->actmagicIsOrbiting == 2) )
					{
						if ( explode )
						{
							spawnExplosion(my->x, my->y, my->z);
						}
					}
					if (hit.entity)
					{
						bool doSound = true;
						int customSoundVolume = 0;
						if ( spell->ID == SPELL_FOCI_FIRE 
							|| spell->ID == SPELL_BREATHE_FIRE )
						{
							if ( ticks - lastMagicSoundPlayed[spell->ID] < 10 )
							{
								doSound = false;
							}
							else if ( ticks - lastMagicSoundPlayed[spell->ID] < 25 )
							{
								doSound = false;
								customSoundVolume = 32;
							}
							else
							{
								lastMagicSoundPlayed[spell->ID] = ticks;
							}
						}

						// Attempt to set the Entity on fire
						int prevBurningCounter = hit.entity->flags[BURNING] ? hit.entity->char_fire : 0;
						hit.entity->SetEntityOnFire((parent&& parent->getStats()) ? parent : nullptr);
						if ( hit.entity->flags[BURNING] 
							&& (prevBurningCounter == 0 || (spell->ID == SPELL_FOCI_FIRE || spell->ID == SPELL_BREATHE_FIRE)) )
						{
							if ( (parent && parent->behavior == &actPlayer) || spell->ID == SPELL_FLAMES )
							{
								if ( spell->ID == SPELL_FOCI_FIRE 
									|| spell->ID == SPELL_BREATHE_FIRE )
								{
									hit.entity->char_fire = std::min(element->getDurationSecondary(), prevBurningCounter + element->duration);
								}
								else
								{
									hit.entity->char_fire = std::min(hit.entity->char_fire, element->duration);
								}
							}
						}

						if ( !strcmp(element->element_internal_name, "spell_element_flames") )
						{
							if ( Entity* fx = createParticleAestheticOrbit(hit.entity, 233, TICKS_PER_SECOND / 2, PARTICLE_EFFECT_IGNITE_ORBIT) )
							{
								fx->flags[SPRITE] = true;
								fx->x = hit.entity->x;
								fx->y = hit.entity->y;
								fx->fskill[0] = fx->x;
								fx->fskill[1] = fx->y;
								fx->vel_z = -0.05;
								fx->actmagicOrbitDist = 2;
								fx->fskill[2] = hit.entity->yaw + (local_rng.rand() % 8) * PI / 4.0;
								fx->yaw = fx->fskill[2];
								fx->actmagicNoLight = 1;

								serverSpawnMiscParticles(hit.entity, PARTICLE_EFFECT_FLAMES, 233, 0, fx->skill[0]);
							}
						}

						if ( mimic )
						{
							hit.entity->chestHandleDamageMagic(damage, *my, parent, doSound);
							if ( !doSound && customSoundVolume > 0 )
							{
								playSoundEntity(hit.entity, 28, customSoundVolume);
							}
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}
							if ( (spell->ID == SPELL_METEOR || spell->ID == SPELL_METEOR_SHOWER)
								&& explode )
							{
								Entity* caster = uidToEntity(spell->caster);
								createSpellExplosionArea(spell->ID, caster, my->x, my->y, my->z, 32.0, preResistanceDamage, hit.entity);
							}
							if ( !(my->actmagicIsOrbiting == 2) )
							{
								my->removeLightField();
								list_RemoveNode(my->mynode);
							}
							else
							{
								if ( explode )
								{
									spawnExplosion(my->x, my->y, my->z);
								}
							}
							return;
						}
						else if (hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer)
						{
							//playSoundEntity(my, 153, 64);
							if ( doSound )
							{
								playSoundEntity(hit.entity, 28, 128);
							}
							else if ( !doSound && customSoundVolume > 0 )
							{
								playSoundEntity(hit.entity, 28, customSoundVolume);
							}
							//TODO: Apply fire resistances/weaknesses.
							if ( my->actmagicIsOrbiting == 2 )
							{
								if ( explode )
								{
									spawnExplosion(my->x, my->y, my->z);
								}
							}

							if ( spell->ID == SPELL_BREATHE_FIRE )
							{
								Stat* parentStats = parent ? parent->getStats() : nullptr;
								Uint8 effectStrength = (hitstats->getEffectActive(EFF_DIVINE_FIRE) & 0xF);
								int maxStrength = 10;
								if ( parentStats )
								{
									int minStrength = 2;
									if ( parentStats->type == MONSTER_S
										&& parentStats->getEffectActive(EFF_SALAMANDER_HEART) == 2 )
									{
										minStrength += 3;
									}
									maxStrength = std::min(maxStrength, minStrength + std::max(0, statGetCHR(parent->getStats(), parent)) / 5);
								}
								if ( effectStrength < maxStrength )
								{
									effectStrength += 1;
								}
								int duration = std::min(element->getDurationSecondary(), hitstats->EFFECTS_TIMERS[EFF_DIVINE_FIRE] 
									+ element->duration);
								if ( parent )
								{
									if ( parent->behavior == &actPlayer )
									{
										effectStrength |= (1 + parent->skill[2]) << 4;
									}
									else if ( parent->behavior == &actMonster && parent->monsterAllyGetPlayerLeader() )
									{
										effectStrength |= (1 + parent->monsterAllyGetPlayerLeader()->skill[2]) << 4;
									}
								}
								if ( hit.entity->setEffect(EFF_DIVINE_FIRE, effectStrength, duration, false, true, true) )
								{
									if ( parentStats && parentStats->type == MONSTER_S )
									{
										parent->modMP(1 + (effectStrength & 0xF) / 4);
									}
								}
							}

							int oldHP = hitstats->HP;
							hit.entity->modHP(-damage);
							magicOnEntityHit(parent, my, hit.entity, hitstats, preResistanceDamage, damage, oldHP, spell ? spell->ID : SPELL_NONE);
							magicTrapOnHit(parent, hit.entity, hitstats, oldHP, spell ? spell->ID : SPELL_NONE);
							//for (i = 0; i < damage; i += 2) { //Spawn a gib for every two points of damage.
							Entity* gib = spawnGib(hit.entity);
							serverSpawnGibForClient(gib);
							//}

							// write the obituary
							if ( parent )
							{
								if ( my->actmagicIsOrbiting == 2 
									&& parent->behavior == &actParticleDot
									&& parent->skill[1] > 0 )
								{
									if ( hitstats && !strcmp(hitstats->obituary, Language::get(3898)) )
									{
										// was caused by a flaming boulder.
										hit.entity->setObituary(Language::get(3898));
										hitstats->killer = KilledBy::BOULDER;
									}
									else
									{
										// blew the brew (alchemy)
										hit.entity->setObituary(Language::get(3350));
										hitstats->killer = KilledBy::FAILED_ALCHEMY;
									}
								}
								else
								{
									parent->killedByMonsterObituary(hit.entity);
								}
							}
							if ( hitstats )
							{
								hitstats->burningInflictedBy = static_cast<Sint32>(my->parent);
							}

							// update enemy bar for attacker
							if ( !strcmp(hitstats->name, "") )
							{
								updateEnemyBar(parent, hit.entity, getMonsterLocalizedName(hitstats->type).c_str(), hitstats->HP, hitstats->MAXHP,
									false, dmgGib);
							}
							else
							{
								updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP,
									false, dmgGib);
							}
							if ( oldHP > 0 && hitstats->HP <= 0 )
							{
								if ( parent )
								{
									if ( my->actmagicIsOrbiting == 2 && my->actmagicOrbitCastFromSpell == 0 && parent->behavior == &actPlayer )
									{
										if ( hitstats->type == LICH || hitstats->type == LICH_ICE || hitstats->type == LICH_FIRE )
										{
											if ( true/*client_classes[parent->skill[2]] == CLASS_BREWER*/ )
											{
												steamAchievementClient(parent->skill[2], "BARONY_ACH_SECRET_WEAPON");
											}
										}
										steamStatisticUpdateClient(parent->skill[2], STEAM_STAT_BOMBARDIER, STEAM_STAT_INT, 1);
									}
									if ( my->actmagicCastByTinkerTrap == 1 && parent->behavior == &actPlayer && hitstats->type == MINOTAUR )
									{
										steamAchievementClient(parent->skill[2], "BARONY_ACH_TIME_TO_PLAN");
									}
									parent->awardXP( hit.entity, true, true );
									spawnBloodVialOnMonsterDeath(hit.entity, hitstats, parent);
								}
								else
								{
									if ( achievementObserver.checkUidIsFromPlayer(my->parent) >= 0 )
									{
										steamAchievementClient(achievementObserver.checkUidIsFromPlayer(my->parent), "BARONY_ACH_TAKING_WITH");
									}
								}
							}
						}
						else if ( hit.entity->behavior == &actDoor || hit.entity->behavior == &actIronDoor )
						{
							hit.entity->doorHandleDamageMagic(damage, *my, parent, true, doSound);
							if ( !doSound && customSoundVolume > 0 )
							{
								playSoundEntity(hit.entity, 28, customSoundVolume);
							}
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}
							if ( (spell->ID == SPELL_METEOR || spell->ID == SPELL_METEOR_SHOWER)
								&& explode )
							{
								Entity* caster = uidToEntity(spell->caster);
								createSpellExplosionArea(spell->ID, caster, my->x, my->y, my->z, 32.0, preResistanceDamage, hit.entity);
							}
							if ( !(my->actmagicIsOrbiting == 2) )
							{
								my->removeLightField();
								list_RemoveNode(my->mynode);
							}
							else
							{
								if ( explode )
								{
									spawnExplosion(my->x, my->y, my->z);
								}
							}
							return;
						} 
						else if ( hit.entity->isDamageableCollider() && hit.entity->isColliderDamageableByMagic() )
						{
							hit.entity->colliderHandleDamageMagic(damage, *my, parent, true, doSound);
							if ( !doSound && customSoundVolume > 0 )
							{
								playSoundEntity(hit.entity, hit.entity->getColliderSfxOnHit() > 0 ? 
									hit.entity->getColliderSfxOnHit() : 28, customSoundVolume);
							}
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}
							if ( (spell->ID == SPELL_METEOR || spell->ID == SPELL_METEOR_SHOWER)
								&& explode )
							{
								Entity* caster = uidToEntity(spell->caster);
								createSpellExplosionArea(spell->ID, caster, my->x, my->y, my->z, 32.0, preResistanceDamage, hit.entity);
							}
							if ( !(my->actmagicIsOrbiting == 2) )
							{
								my->removeLightField();
								list_RemoveNode(my->mynode);
							}
							else
							{
								if ( explode )
								{
									spawnExplosion(my->x, my->y, my->z);
								}
							}
							return;
						}
						else if (hit.entity->behavior == &actChest) 
						{
							hit.entity->chestHandleDamageMagic(damage, *my, parent, doSound);
							if ( !doSound && customSoundVolume > 0 )
							{
								playSoundEntity(hit.entity, 28, customSoundVolume);
							}
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}
							if ( (spell->ID == SPELL_METEOR || spell->ID == SPELL_METEOR_SHOWER)
								&& explode )
							{
								Entity* caster = uidToEntity(spell->caster);
								createSpellExplosionArea(spell->ID, caster, my->x, my->y, my->z, 32.0, preResistanceDamage, hit.entity);
							}
							if ( !(my->actmagicIsOrbiting == 2) )
							{
								my->removeLightField();
								list_RemoveNode(my->mynode);
							}
							else
							{
								if ( explode )
								{
									spawnExplosion(my->x, my->y, my->z);
								}
							}
							return;
						}
						else if (hit.entity->behavior == &actFurniture )
						{
							hit.entity->furnitureHandleDamageMagic(damage, *my, parent, true, doSound);
							if ( !doSound && customSoundVolume > 0 )
							{
								playSoundEntity(hit.entity, 28, customSoundVolume);
							}
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}
							if ( (spell->ID == SPELL_METEOR || spell->ID == SPELL_METEOR_SHOWER)
								&& explode )
							{
								Entity* caster = uidToEntity(spell->caster);
								createSpellExplosionArea(spell->ID, caster, my->x, my->y, my->z, 32.0, preResistanceDamage, hit.entity);
							}
							if ( !(my->actmagicIsOrbiting == 2) )
							{
								my->removeLightField();
								list_RemoveNode(my->mynode);
							}
							else
							{
								if ( explode )
								{
									spawnExplosion(my->x, my->y, my->z);
								}
							}
							return;
						}
					}
				}
				else if (!strcmp(element->element_internal_name, spellElement_confuse.element_internal_name))
				{
					if (hit.entity)
					{
						if ( (!mimic && hit.entity->behavior == &actMonster) || hit.entity->behavior == &actPlayer)
						{
							int duration = element->duration;
							duration = convertResistancePointsToMagicValue(duration, resistance);

							if ( parent && (parent->behavior == &actMagicTrap || parent->behavior == &actMagicTrapCeiling) )
							{
								if ( trapResist > 0 )
								{
									if ( local_rng.rand() % 100 < trapResist )
									{
										duration = 0;
									}
								}
							}

							Uint8 effectStrength = MAXPLAYERS + 1;
							if ( parent && parent->behavior == &actPlayer )
							{
								effectStrength = parent->skill[2] + 1;
							}
							else if ( parent && parent->monsterAllyGetPlayerLeader() )
							{
								effectStrength = parent->monsterAllyGetPlayerLeader()->skill[2] + 1;
							}
							if ( duration > 0 && hit.entity->setEffect(EFF_CONFUSED, effectStrength, duration, true, true, true) )
							{
								magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
								magicTrapOnHit(parent, hit.entity, hitstats, 0, spell ? spell->ID : SPELL_NONE);
								playSoundEntity(hit.entity, 174, 64);
								if ( hit.entity->behavior == &actMonster )
								{
									hit.entity->monsterTarget = 0; // monsters forget what they're doing
								}
								if ( parent )
								{
									Uint32 color = makeColorRGB(0, 255, 0);
									if ( parent->behavior == &actPlayer )
									{
										messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(391), Language::get(390), MSG_COMBAT);
									}
								}
								Uint32 color = makeColorRGB(255, 0, 0);
								if ( player >= 0 )
								{
									messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(392));
								}
							}
							else
							{
								if ( parent )
								{
									Uint32 color = makeColorRGB(255, 0, 0);
									if ( parent->behavior == &actPlayer )
									{
										messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(2905), Language::get(2906), MSG_COMBAT);
									}
								}
							}
							spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my->sprite);
						}
					}
				}
				else if (!strcmp(element->element_internal_name, spellElement_cold.element_internal_name))
				{
					playSoundEntity(my, 197, 128);
					if (hit.entity)
					{
						if ( (!mimic && hit.entity->behavior == &actMonster) || hit.entity->behavior == &actPlayer)
						{
							bool warmHat = false;
							if ( hitstats->helmet && hitstats->helmet->type == HAT_WARM )
							{
								if ( !(hit.entity->behavior == &actPlayer && hit.entity->effectShapeshift != NOTHING) )
								{
									warmHat = true;
								}
							}

							playSoundEntity(hit.entity, 28, 128);

							if ( !warmHat )
							{
								hitstats->setEffectActive(EFF_SLOW, 1);
								hitstats->EFFECTS_TIMERS[EFF_SLOW] = convertResistancePointsToMagicValue(element->duration, resistance);

								// If the Entity hit is a Player, update their status to be Slowed
								if ( hit.entity->behavior == &actPlayer )
								{
									serverUpdateEffects(hit.entity->skill[2]);
								}
							}

							int oldHP = hitstats->HP;
							if ( damage > 0 )
							{
								hit.entity->modHP(-damage);
								magicOnEntityHit(parent, my, hit.entity, hitstats, preResistanceDamage, damage, oldHP, spell ? spell->ID : SPELL_NONE);
								Entity* gib = spawnGib(hit.entity);
								serverSpawnGibForClient(gib);
								magicTrapOnHit(parent, hit.entity, hitstats, oldHP, spell ? spell->ID : SPELL_NONE);
							}

							// write the obituary
							if ( parent )
							{
								parent->killedByMonsterObituary(hit.entity);
							}

							// update enemy bar for attacker
							if ( !strcmp(hitstats->name, "") )
							{
								updateEnemyBar(parent, hit.entity, getMonsterLocalizedName(hitstats->type).c_str(), hitstats->HP, hitstats->MAXHP,
									false, dmgGib);
							}
							else
							{
								updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP,
									false, dmgGib);
							}
							if ( parent )
							{
								Uint32 color = makeColorRGB(0, 255, 0);
								if ( parent->behavior == &actPlayer )
								{
									messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(394), Language::get(393), MSG_COMBAT);
								}
							}
							Uint32 color = makeColorRGB(255, 0, 0);
							if ( player >= 0 )
							{
								messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(395));
							}
							spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my->sprite);
							if ( oldHP > 0 && hitstats->HP <= 0 )
							{
								if ( parent )
								{
									parent->awardXP(hit.entity, true, true);
									if ( my->actmagicIsOrbiting == 2 && my->actmagicOrbitCastFromSpell == 0 && parent->behavior == &actPlayer )
									{
										if ( hitstats->type == LICH || hitstats->type == LICH_ICE || hitstats->type == LICH_FIRE )
										{
											if ( true/*client_classes[parent->skill[2]] == CLASS_BREWER*/ )
											{
												steamAchievementClient(parent->skill[2], "BARONY_ACH_SECRET_WEAPON");
											}
										}
										steamStatisticUpdateClient(parent->skill[2], STEAM_STAT_BOMBARDIER, STEAM_STAT_INT, 1);
									}
									if ( my->actmagicCastByTinkerTrap == 1 && parent->behavior == &actPlayer && hitstats->type == MINOTAUR )
									{
										steamAchievementClient(parent->skill[2], "BARONY_ACH_TIME_TO_PLAN");
									}
									spawnBloodVialOnMonsterDeath(hit.entity, hitstats, parent);
								}
								else
								{
									if ( achievementObserver.checkUidIsFromPlayer(my->parent) >= 0 )
									{
										steamAchievementClient(achievementObserver.checkUidIsFromPlayer(my->parent), "BARONY_ACH_TAKING_WITH");
									}
								}
							}
						}
					}
				}
				else if (!strcmp(element->element_internal_name, spellElement_slow.element_internal_name))
				{
					if (hit.entity)
					{
						if ( (!mimic && hit.entity->behavior == &actMonster) || hit.entity->behavior == &actPlayer)
						{
							playSoundEntity(hit.entity, 396 + local_rng.rand() % 3, 64);
							hitstats->setEffectActive(EFF_SLOW, 1);
							hitstats->EFFECTS_TIMERS[EFF_SLOW] = convertResistancePointsToMagicValue(element->duration, resistance);

							magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
							magicTrapOnHit(parent, hit.entity, hitstats, 0, spell ? spell->ID : SPELL_NONE);

							// If the Entity hit is a Player, update their status to be Slowed
							if ( hit.entity->behavior == &actPlayer )
							{
								serverUpdateEffects(hit.entity->skill[2]);
							}

							// update enemy bar for attacker
							if ( parent )
							{
								Uint32 color = makeColorRGB(0, 255, 0);
								if ( parent->behavior == &actPlayer )
								{
									messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(394), Language::get(393), MSG_COMBAT);
								}
							}
							Uint32 color = makeColorRGB(255, 0, 0);
							if ( player >= 0 )
							{
								messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(395));
							}
							spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my->sprite);
						}
					}
				}
				else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_NUMBING_BOLT].element_internal_name) )
				{
					if ( hit.entity )
					{
						if ( (!mimic && hit.entity->behavior == &actMonster) || hit.entity->behavior == &actPlayer )
						{
							if ( hit.entity->setEffect(EFF_NUMBING_BOLT, true, element->duration, false) )
							{
								spawnFloatingSpriteMisc(134, hit.entity->x + (-4 + local_rng.rand() % 9) + cos(hit.entity->yaw) * 2,
									hit.entity->y + (-4 + local_rng.rand() % 9) + sin(hit.entity->yaw) * 2, hit.entity->z + local_rng.rand() % 4);

								magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
								magicTrapOnHit(parent, hit.entity, hitstats, 0, spell ? spell->ID : SPELL_NONE);

								// update enemy bar for attacker
								if ( parent )
								{
									Uint32 color = makeColorRGB(0, 255, 0);
									if ( parent->behavior == &actPlayer )
									{
										messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(6623), Language::get(6624), MSG_COMBAT);
									}
								}
								Uint32 color = makeColorRGB(255, 0, 0);
								if ( player >= 0 )
								{
									messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(6625));
								}
							}
							else
							{
								if ( parent )
								{
									Uint32 color = makeColorRGB(255, 0, 0);
									if ( parent->behavior == &actPlayer )
									{
										messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(2905), Language::get(2906), MSG_COMBAT);
									}
								}
							}
							spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my->sprite);
						}
					}
				}
				else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_PSYCHIC_SPEAR].element_internal_name) )
				{
					if ( hit.entity )
					{
						if ( (!mimic && hit.entity->behavior == &actMonster) || hit.entity->behavior == &actPlayer )
						{
							spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my->sprite);

							//for ( int i = 0; i < 3; ++i )
							{
								Entity* fx = createParticleAestheticOrbit(hit.entity, 2362, 5 * TICKS_PER_SECOND, PARTICLE_EFFECT_PSYCHIC_SPEAR);
								fx->yaw = my->yaw;
								fx->skill[3] = spell->caster;
								fx->pitch = 0;// PI / 4;
								fx->fskill[0] = fx->yaw + PI / 2 + (local_rng.rand() % 6) * PI / 3;
								fx->fskill[1] = PI / 4 + PI / 8;// +(i + 1) * 2 * PI / 3;
								fx->x = hit.entity->x - 8.0 * cos(fx->yaw);
								fx->y = hit.entity->y - 8.0 * sin(fx->yaw);
								fx->z = hit.entity->z;// -8.0;
								fx->scalex = 0.0;
								fx->scaley = 0.0;
								fx->scalez = 0.0;
							}
						}
					}
				}
				else if ( !strcmp(element->element_internal_name, spellElement_weakness.element_internal_name)
					|| !strcmp(element->element_internal_name, spellElementMap[SPELL_INCOHERENCE].element_internal_name) )
				{
					if ( hit.entity )
					{
						if ( (!mimic && hit.entity->behavior == &actMonster) || hit.entity->behavior == &actPlayer )
						{
							int effectID = 0;
							if ( spell )
							{
								if ( spell->ID == SPELL_WEAKNESS )
								{
									effectID = EFF_WEAKNESS;
								}
								else if ( spell->ID == SPELL_INCOHERENCE )
								{
									effectID = EFF_INCOHERENCE;
								}
							}

							Uint8 effectStrength = 1;
							if ( parent && parent->behavior == &actPlayer )
							{
								effectStrength = std::min(7, getSpellDamageFromID(spell->ID, parent, parent ? parent->getStats() : nullptr, my));
							}
							else
							{
								effectStrength = 5;
							}

							if ( hit.entity->setEffect(effectID, effectStrength, element->duration, false, true, true) )
							{
								if ( effectID == EFF_WEAKNESS )
								{
									if ( parent )
									{
										Uint32 color = makeColorRGB(0, 255, 0);
										if ( parent->behavior == &actPlayer )
										{
											messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(6527), Language::get(6526), MSG_COMBAT);
										}
									}
									Uint32 color = makeColorRGB(255, 0, 0);
									if ( player >= 0 )
									{
										messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(6528));
									}
									playSoundEntity(hit.entity, 824, 64);
								}
								else if ( effectID == EFF_INCOHERENCE )
								{
									if ( parent )
									{
										Uint32 color = makeColorRGB(0, 255, 0);
										if ( parent->behavior == &actPlayer )
										{
											messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(6910), Language::get(6909), MSG_COMBAT);
										}
									}
									Uint32 color = makeColorRGB(255, 0, 0);
									if ( player >= 0 )
									{
										messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(6911));
									}
									playSoundEntity(hit.entity, 825, 64);
								}

							}
							else
							{
								if ( parent )
								{
									Uint32 color = makeColorRGB(255, 0, 0);
									if ( parent->behavior == &actPlayer )
									{
										messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(2905), Language::get(2906), MSG_COMBAT);
									}
								}
							}

							magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
							magicTrapOnHit(parent, hit.entity, hitstats, 0, spell ? spell->ID : SPELL_NONE);
							
							spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my->sprite);
						}
					}
				}
				else if (!strcmp(element->element_internal_name, spellElement_sleep.element_internal_name))
				{
					if (hit.entity)
					{
						if ( (!mimic && hit.entity->behavior == &actMonster) || hit.entity->behavior == &actPlayer)
						{
							int effectDuration = 0;
							if ( parent && parent->behavior == &actMagicTrapCeiling )
							{
								effectDuration = 200 + local_rng.rand() % 150; // 4 seconds + 0 to 3 seconds.
							}
							else
							{
								effectDuration = element->duration;
								if ( hitstats )
								{
									effectDuration = std::max(50, effectDuration - ((hitstats->CON / 3) * 50)); // reduce 1 sec every 3 CON.
								}
							}
							effectDuration = convertResistancePointsToMagicValue(effectDuration, resistance);

							bool magicTrapReapplySleep = true;

							if ( parent && (parent->behavior == &actMagicTrap || parent->behavior == &actMagicTrapCeiling) )
							{
								if ( hitstats && hitstats->getEffectActive(EFF_ASLEEP) )
								{
									// check to see if we're reapplying the sleep effect.
									int preventSleepRoll = (local_rng.rand() % 4) - resistance;
									if ( hit.entity->behavior == &actPlayer || (preventSleepRoll <= 0) )
									{
										magicTrapReapplySleep = false;
										//messagePlayer(0, "Target already asleep!");
									}
								}

								int trapResist = hit.entity->getEntityBonusTrapResist();
								if ( trapResist > 0 )
								{
									if ( local_rng.rand() % 100 < trapResist )
									{
										magicTrapReapplySleep = false;
									}
								}
							}

							if ( magicTrapReapplySleep )
							{
								if ( hit.entity->setEffect(EFF_ASLEEP, true, effectDuration, false) )
								{
									magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
									magicTrapOnHit(parent, hit.entity, hitstats, 0, spell ? spell->ID : SPELL_NONE);
									playSoundEntity(hit.entity, 174, 64);
									hitstats->OLDHP = hitstats->HP;
									if ( hit.entity->behavior == &actPlayer )
									{
										serverUpdateEffects(hit.entity->skill[2]);
										Uint32 color = makeColorRGB(255, 0, 0);
										messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(396));
									}
									if ( parent )
									{
										Uint32 color = makeColorRGB(0, 255, 0);
										if ( parent->behavior == &actPlayer )
										{
											messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(398), Language::get(397), MSG_COMBAT);
										}
									}
								}
								else
								{
									if ( parent )
									{
										Uint32 color = makeColorRGB(255, 0, 0);
										if ( parent->behavior == &actPlayer )
										{
											messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(2905), Language::get(2906), MSG_COMBAT);
										}
									}
								}
							}
							spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my->sprite);
						}
					}
				}
				else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_DEFY_FLESH].element_internal_name) )
				{
					if ( hit.entity && hitstats )
					{
						if ( (!mimic && hit.entity->behavior == &actMonster) || hit.entity->behavior == &actPlayer )
						{
							int charges = std::min(15, std::max(1, 
								std::min(getSpellDamageSecondaryFromID(SPELL_DEFY_FLESH, parent, nullptr, my), 
								hitstats->HP / std::max(1, element->getDurationSecondary()))));
							Uint8 effectStrength = charges;
							if ( parent )
							{
								if ( parent->behavior == &actPlayer )
								{
									effectStrength |= ((parent->skill[2] + 1) << 4) & 0xF0;
								}
							}
							
							if ( hitstats->getEffectActive(EFF_DEFY_FLESH) )
							{
								if ( parent )
								{
									Uint32 color = makeColorRGB(255, 0, 0);
									messagePlayerColor(parent->isEntityPlayer(), MESSAGE_COMBAT, color, Language::get(6908));
								}
								playSoundEntity(hit.entity, 163, 128);
								spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my->sprite);
							}
							else if ( hit.entity->setEffect(EFF_DEFY_FLESH, effectStrength, element->duration, true, true, true) )
							{
								if ( Entity* fx = createParticleAestheticOrbit(hit.entity, 2363, element->duration, PARTICLE_EFFECT_DEFY_FLESH_ORBIT) )
								{
									fx->skill[3] = spell->caster;
									fx->flags[INVISIBLE] = true;
								}

								serverSpawnMiscParticles(hit.entity, PARTICLE_EFFECT_DEFY_FLESH_ORBIT, 2363, 0, element->duration);

								magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
								magicTrapOnHit(parent, hit.entity, hitstats, 0, spell ? spell->ID : SPELL_NONE);

								// update enemy bar for attacker
								if ( parent )
								{
									Uint32 color = makeColorRGB(0, 255, 0);
									if ( parent->behavior == &actPlayer )
									{
										messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(6905), Language::get(6906), MSG_COMBAT);
									}
								}
								Uint32 color = makeColorRGB(255, 0, 0);
								if ( player >= 0 )
								{
									messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(6907));
								}
								spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my->sprite);
							}
						}
					}
				}
				else if (!strcmp(element->element_internal_name, spellElement_lightning.element_internal_name))
				{
					playSoundEntity(my, 173, 128);
					if (hit.entity)
					{
						if ( mimic )
						{
							hit.entity->chestHandleDamageMagic(damage, *my, parent);
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}
							if ( !(my->actmagicIsOrbiting == 2) )
							{
								my->removeLightField();
								list_RemoveNode(my->mynode);
							}
							return;
						}
						else if (hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer)
						{
							Entity* parent = uidToEntity(my->parent);
							playSoundEntity(my, 173, 64);
							playSoundEntity(hit.entity, 28, 128);

							int oldHP = hitstats->HP;
							hit.entity->modHP(-damage);
							magicOnEntityHit(parent, my, hit.entity, hitstats, preResistanceDamage, damage, oldHP, spell ? spell->ID : SPELL_NONE);
							magicTrapOnHit(parent, hit.entity, hitstats, oldHP, spell ? spell->ID : SPELL_NONE);

							// write the obituary
							if (parent)
							{
								parent->killedByMonsterObituary(hit.entity);
							}

							if ( spell->ID == SPELL_LIGHTNING )
							{
								Uint8 effectStrength = hitstats->getEffectActive(EFF_STATIC);
								if ( effectStrength < getSpellEffectDurationSecondaryFromID(SPELL_LIGHTNING, parent, nullptr, my) )
								{
									effectStrength += 1;
								}
								if ( hit.entity->setEffect(EFF_STATIC, effectStrength,
									getSpellEffectDurationFromID(spell->ID, parent, nullptr, my), true, true, false, false) )
								{

								}
							}

							// update enemy bar for attacker
							if ( !strcmp(hitstats->name, "") )
							{
								updateEnemyBar(parent, hit.entity, getMonsterLocalizedName(hitstats->type).c_str(), hitstats->HP, hitstats->MAXHP,
									false, dmgGib);
							}
							else
							{
								updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP,
									false, dmgGib);
							}
							if ( oldHP > 0 && hitstats->HP <= 0 && parent)
							{
								parent->awardXP( hit.entity, true, true );
								if ( my->actmagicIsOrbiting == 2 && my->actmagicOrbitCastFromSpell == 0 && parent->behavior == &actPlayer )
								{
									if ( hitstats->type == LICH || hitstats->type == LICH_ICE || hitstats->type == LICH_FIRE )
									{
										if ( true/*client_classes[parent->skill[2]] == CLASS_BREWER*/ )
										{
											steamAchievementClient(parent->skill[2], "BARONY_ACH_SECRET_WEAPON");
										}
									}
									steamStatisticUpdateClient(parent->skill[2], STEAM_STAT_BOMBARDIER, STEAM_STAT_INT, 1);
								}
								spawnBloodVialOnMonsterDeath(hit.entity, hitstats, parent);
							}
						}
						else if ( hit.entity->behavior == &actDoor || hit.entity->behavior == &actIronDoor )
						{
							hit.entity->doorHandleDamageMagic(damage, *my, parent);
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}
							if ( !(my->actmagicIsOrbiting == 2) )
							{
								my->removeLightField();
								list_RemoveNode(my->mynode);
							}
							return;
						}
						else if ( hit.entity->isDamageableCollider() && hit.entity->isColliderDamageableByMagic() )
						{
							hit.entity->colliderHandleDamageMagic(damage, *my, parent);
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}
							if ( !(my->actmagicIsOrbiting == 2) )
							{
								my->removeLightField();
								list_RemoveNode(my->mynode);
							}
							return;
						}
						else if ( hit.entity->behavior == &actChest )
						{
							hit.entity->chestHandleDamageMagic(damage, *my, parent);
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}
							if ( !(my->actmagicIsOrbiting == 2) )
							{
								my->removeLightField();
								list_RemoveNode(my->mynode);
							}
							return;
						}
						else if (hit.entity->behavior == &actFurniture )
						{
							hit.entity->furnitureHandleDamageMagic(damage, *my, parent);
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}
						}
					}
				}
				else if ( (spell->ID >= SPELL_SLIME_ACID && spell->ID <= SPELL_SLIME_METAL) || spell->ID == SPELL_GREASE_SPRAY )
				{
					if ( hit.entity )
					{
						int volume = 128;
						static ConsoleVariable cvar_slimehit_sfx("/slimehit_sfx", 173);
						int hitsfx = *cvar_slimehit_sfx;
						int hitvolume = (particleEmitterHitProps && particleEmitterHitProps->hits == 1) ? 128 : 64;
						if ( spell->ID == SPELL_SLIME_WATER || spell->ID == SPELL_SLIME_TAR || spell->ID == SPELL_GREASE_SPRAY )
						{
							hitsfx = 665;
						}
						if ( particleEmitterHitProps && particleEmitterHitProps->hits > 1 )
						{
							volume = 0;
							if ( spell->ID == SPELL_SLIME_WATER || spell->ID == SPELL_SLIME_TAR )
							{
								hitvolume = 32;
							}
						}

						bool hasgoggles = false;
						if ( hitstats && hitstats->mask && hitstats->mask->type == MASK_HAZARD_GOGGLES )
						{
							if ( !(hit.entity->behavior == &actPlayer && hit.entity->effectShapeshift != NOTHING) )
							{
								hasgoggles = true;
							}
						}

						if ( mimic )
						{
							playSoundEntity(hit.entity, hitsfx, hitvolume);
							hit.entity->chestHandleDamageMagic(damage, *my, parent);
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}
							if ( !(my->actmagicIsOrbiting == 2) )
							{
								my->removeLightField();
								list_RemoveNode(my->mynode);
							}
							if ( spell->ID == SPELL_SLIME_FIRE )
							{
								hit.entity->SetEntityOnFire((parent && parent->getStats()) ? parent : nullptr);
							}
							return;
						}
						else if ( hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer )
						{
							playSoundEntity(hit.entity, hitsfx, hitvolume);
							Entity* parent = uidToEntity(my->parent);
							playSoundEntity(hit.entity, 28, volume);

							int oldHP = hitstats->HP;

							if ( spell->ID == SPELL_SLIME_FIRE )
							{
								if ( particleEmitterHitProps && particleEmitterHitProps->hits == 1 )
								{
									if ( parent && parent->behavior == &actPlayer )
									{
										Uint32 color = makeColorRGB(0, 255, 0);
										messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(6239), Language::get(6238), MSG_COMBAT);
									}
									if ( hit.entity->behavior == &actPlayer )
									{
										Uint32 color = makeColorRGB(255, 0, 0);
										messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(6237));
									}
									if ( hasgoggles )
									{
										if ( hit.entity->behavior == &actPlayer )
										{
											messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, makeColorRGB(0, 255, 0), Language::get(6088));
										}
									}
								}

								if ( !hasgoggles )
								{
									if ( hit.entity->SetEntityOnFire((parent && parent->getStats()) ? parent : nullptr) )
									{
										if ( parent && parent->behavior == &actPlayer )
										{
											hit.entity->char_fire = std::min(hit.entity->char_fire, element->duration);
										}
									}
								}
							}
							if ( spell->ID == SPELL_GREASE_SPRAY )
							{
								if ( particleEmitterHitProps && particleEmitterHitProps->hits == 1 )
								{
									if ( parent && parent->behavior == &actPlayer )
									{
										Uint32 color = makeColorRGB(0, 255, 0);
										messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(6244), Language::get(6243), MSG_COMBAT);
									}
									if ( hit.entity->behavior == &actPlayer )
									{
										Uint32 color = makeColorRGB(255, 0, 0);
										messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(6236));
									}
								}

								if ( particleEmitterHitProps && particleEmitterHitProps->hits == 1 )
								{
									int duration = element->duration;
									duration = convertResistancePointsToMagicValue(duration, resistance);

									if ( hasgoggles )
									{
										if ( hit.entity->behavior == &actPlayer )
										{
											messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, makeColorRGB(0, 255, 0), Language::get(6088));
										}
									}
									else if ( hit.entity->setEffect(EFF_MAGIC_GREASE, true, duration, true) )
									{
										hit.entity->setEffect(EFF_GREASY, true, duration / 2, false);
										Uint32 color = makeColorRGB(255, 0, 0);
										if ( hit.entity->behavior == &actPlayer )
										{
											messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(6245));
										}
									}
								}

								hit.entity->updateEntityOnHit(parent, alertTarget);
							}
							if ( spell->ID == SPELL_SLIME_TAR )
							{
								if ( particleEmitterHitProps && particleEmitterHitProps->hits == 1 )
								{
									if ( parent && parent->behavior == &actPlayer )
									{
										Uint32 color = makeColorRGB(0, 255, 0);
										messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(6244), Language::get(6243), MSG_COMBAT);
									}
									if ( hit.entity->behavior == &actPlayer )
									{
										Uint32 color = makeColorRGB(255, 0, 0);
										messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(6236));
									}
								}

								if ( particleEmitterHitProps && particleEmitterHitProps->hits == 1 )
								{
									if ( local_rng.rand() % 2 == 0 )
									{
										int duration = element->duration / 2;
										duration = convertResistancePointsToMagicValue(duration, resistance);

										int status = hit.entity->behavior == &actPlayer ? EFF_MESSY : EFF_BLIND;

										if ( hasgoggles )
										{
											if ( hit.entity->behavior == &actPlayer )
											{
												messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, makeColorRGB(0, 255, 0), Language::get(6088));
											}
										}
										else if ( hit.entity->setEffect(status, true, duration / 2, false) )
										{
											if ( hit.entity->behavior == &actPlayer )
											{
												Uint32 color = makeColorRGB(255, 0, 0);
												messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(765));
											}
											if ( parent && parent->behavior == &actPlayer )
											{
												Uint32 color = makeColorRGB(0, 255, 0);
												messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(3878), Language::get(3879), MSG_COMBAT);
											}
										}
									}
									else
									{
										int duration = element->duration;
										duration = convertResistancePointsToMagicValue(duration, resistance);

										if ( hasgoggles )
										{
											if ( hit.entity->behavior == &actPlayer )
											{
												messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, makeColorRGB(0, 255, 0), Language::get(6088));
											}
										}
										else if ( hit.entity->setEffect(EFF_GREASY, true, duration, false) )
										{
											Uint32 color = makeColorRGB(255, 0, 0);
											if ( hit.entity->behavior == &actPlayer )
											{
												messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(6245));
											}
										}
									}
								}

								if ( !hit.entity->isBlind() )
								{
									if ( alertTarget && hit.entity->monsterState != MONSTER_STATE_ATTACK && (hitstats->type < LICH || hitstats->type >= SHOPKEEPER) )
									{
										hit.entity->monsterAcquireAttackTarget(*parent, MONSTER_STATE_PATH, true);
									}
								}
								else
								{
									alertTarget = false;
									hit.entity->monsterReleaseAttackTarget();
								}
								hit.entity->updateEntityOnHit(parent, alertTarget);
							}
							if ( spell->ID == SPELL_SLIME_ACID || spell->ID == SPELL_SLIME_METAL )
							{
								bool hasamulet = false;
								if ( (hitstats->amulet && hitstats->amulet->type == AMULET_POISONRESISTANCE) || hitstats->type == INSECTOID )
								{
									resistance += 2;
									hasamulet = true;
								}
								if ( hasgoggles )
								{
									resistance += 2;
								}

								if ( hasamulet && !hasgoggles )
								{
									hit.entity->degradeAmuletProc(hitstats, AMULET_POISONRESISTANCE);
								}

								int duration = (spell->ID == SPELL_SLIME_METAL ? 10 : 6) * TICKS_PER_SECOND;
								duration = convertResistancePointsToMagicValue(duration, resistance);
								if ( spell->ID == SPELL_SLIME_ACID )
								{
									if ( !hasamulet && !hasgoggles )
									{
										if ( hit.entity->setEffect(EFF_POISONED, true, duration, false) )
										{
											hitstats->poisonKiller = my->parent;
										}
									}
								}

								if ( particleEmitterHitProps && particleEmitterHitProps->hits == 1 )
								{
									if ( parent && parent->behavior == &actPlayer )
									{
										Uint32 color = makeColorRGB(0, 255, 0);
										if ( spell->ID == SPELL_SLIME_METAL )
										{
											messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(6241), Language::get(6240), MSG_COMBAT);
										}
										else
										{
											messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(2431), Language::get(2430), MSG_COMBAT);
										}
									}
									if ( hit.entity->behavior == &actPlayer )
									{
										Uint32 color = makeColorRGB(255, 0, 0);
										if ( spell->ID == SPELL_SLIME_METAL )
										{
											messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(6242));
										}
										else
										{
											messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(2432));
										}
										if ( hasgoggles )
										{
											messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, makeColorRGB(0, 255, 0), Language::get(6088));
										}
									}
								}

								if ( spell->ID == SPELL_SLIME_METAL )
								{
									if ( !hasgoggles && hit.entity->setEffect(EFF_SLOW, true, duration, false) )
									{
										if ( particleEmitterHitProps && particleEmitterHitProps->hits == 1 )
										{
											playSoundEntity(hit.entity, 396 + local_rng.rand() % 3, 64);
											if ( parent && parent->behavior == &actPlayer )
											{
												Uint32 color = makeColorRGB(0, 255, 0);
												messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(394), Language::get(393), MSG_COMBAT);
											}
											if ( hit.entity->behavior == &actPlayer )
											{
												Uint32 color = makeColorRGB(255, 0, 0);
												messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(395));
											}
										}
									}
								}

								if ( hitstats->HP > 0 && !hasgoggles )
								{
									// damage armor
									Item* armor = nullptr;
									int armornum = -1;
									int acidChance = spell->ID == SPELL_SLIME_METAL ? 2 : 4;
									if ( hitstats->defending && (local_rng.rand() % ((acidChance * 2) + resistance) == 0) ) // 1 in 8 to corrode shield
									{
										armornum = hitstats->pickRandomEquippedItem(&armor, true, false, true, true);
									}
									else if ( !hitstats->defending && (local_rng.rand() % (acidChance + resistance) == 0) ) // 1 in 4 to corrode armor
									{
										armornum = hitstats->pickRandomEquippedItem(&armor, true, false, false, false);
									}
									if ( armornum != -1 && armor != nullptr )
									{
										hit.entity->degradeArmor(*hitstats, *armor, armornum);
									}
								}
							}
							else if ( spell->ID == SPELL_SLIME_WATER  )
							{
								if ( !hasgoggles && hit.entity->setEffect(EFF_KNOCKBACK, true, 30, false) )
								{
									real_t pushbackMultiplier = 0.6;
									if ( hit.entity->behavior == &actMonster )
									{
										if ( !hit.entity->isMobile() )
										{
											pushbackMultiplier += 0.3;
										}
										if ( parent )
										{
											real_t tangent = atan2(hit.entity->y - parent->y, hit.entity->x - parent->x);
											hit.entity->vel_x = cos(tangent) * pushbackMultiplier;
											hit.entity->vel_y = sin(tangent) * pushbackMultiplier;
											hit.entity->monsterKnockbackVelocity = 0.01;
											hit.entity->monsterKnockbackUID = my->parent;
											hit.entity->monsterKnockbackTangentDir = tangent;
										}
										else
										{
											real_t tangent = atan2(hit.entity->y - my->y, hit.entity->x - my->x);
											hit.entity->vel_x = cos(tangent) * pushbackMultiplier;
											hit.entity->vel_y = sin(tangent) * pushbackMultiplier;
											hit.entity->monsterKnockbackVelocity = 0.01;
											hit.entity->monsterKnockbackTangentDir = tangent;
										}
									}
									else if ( hit.entity->behavior == &actPlayer )
									{
										/*if ( parent )
										{
											real_t dist = entityDist(parent, hit.entity);
											if ( dist < TOUCHRANGE )
											{
												pushbackMultiplier += 0.5;
											}
										}*/
										if ( !players[hit.entity->skill[2]]->isLocalPlayer() )
										{
											hit.entity->monsterKnockbackVelocity = pushbackMultiplier;
											hit.entity->monsterKnockbackTangentDir = my->yaw;
											serverUpdateEntityFSkill(hit.entity, 11);
											serverUpdateEntityFSkill(hit.entity, 9);
										}
										else
										{
											hit.entity->monsterKnockbackVelocity = pushbackMultiplier;
											hit.entity->monsterKnockbackTangentDir = my->yaw;
										}
									}
								}

								if ( particleEmitterHitProps && particleEmitterHitProps->hits == 1 )
								{
									if ( parent && parent->behavior == &actPlayer )
									{
										Uint32 color = makeColorRGB(0, 255, 0);
										messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(3215), Language::get(3214), MSG_COMBAT);
									}
									if ( hit.entity->behavior == &actPlayer )
									{
										Uint32 color = makeColorRGB(255, 0, 0);
										if ( !hasgoggles )
										{
											messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(6235));
										}
										else
										{
											messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(6309));
										}

										if ( hitstats->type == VAMPIRE )
										{
											messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(644));
										}
										else if ( hasgoggles )
										{
											messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, makeColorRGB(0, 255, 0), Language::get(6088));
										}
									}
								}

								if ( hit.entity->behavior == &actPlayer )
								{
									if ( hitstats && hitstats->getEffectActive(EFF_POLYMORPH) )
									{
										hit.entity->setEffect(EFF_POLYMORPH, false, 0, true);
										hit.entity->effectPolymorph = 0;
										serverUpdateEntitySkill(hit.entity, 50);

										messagePlayer(hit.entity->skill[2], MESSAGE_STATUS, Language::get(3192));
										if ( !hitstats->getEffectActive(EFF_SHAPESHIFT) )
										{
											messagePlayer(hit.entity->skill[2], MESSAGE_STATUS, Language::get(3185));
										}
										else
										{
											messagePlayer(hit.entity->skill[2], MESSAGE_STATUS, Language::get(4303));
										}
										playSoundEntity(hit.entity, 400, 92);
										createParticleDropRising(hit.entity, 593, 1.f);
										serverSpawnMiscParticles(hit.entity, PARTICLE_EFFECT_RISING_DROP, 593);
									}
								}
							}

							hit.entity->modHP(-damage);
							magicOnEntityHit(parent, my, hit.entity, hitstats, preResistanceDamage, damage, oldHP, spell ? spell->ID : SPELL_NONE);
							magicTrapOnHit(parent, hit.entity, hitstats, oldHP, spell ? spell->ID : SPELL_NONE);

							// write the obituary
							if ( parent )
							{
								parent->killedByMonsterObituary(hit.entity);
							}

							if ( damage > 0 )
							{
								// update enemy bar for attacker
								if ( !strcmp(hitstats->name, "") )
								{
									updateEnemyBar(parent, hit.entity, getMonsterLocalizedName(hitstats->type).c_str(), hitstats->HP, hitstats->MAXHP,
										false, dmgGib);
								}
								else
								{
									updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP,
										false, dmgGib);
								}
								if ( oldHP > 0 && hitstats->HP <= 0 && parent )
								{
									parent->awardXP(hit.entity, true, true);
									spawnBloodVialOnMonsterDeath(hit.entity, hitstats, parent);
								}
							}
						}
						else if ( hit.entity->behavior == &actDoor || hit.entity->behavior == &actIronDoor )
						{
							if ( spell->ID == SPELL_SLIME_FIRE )
							{
								hit.entity->SetEntityOnFire((parent && parent->getStats()) ? parent : nullptr);
							}
							playSoundEntity(hit.entity, hitsfx, hitvolume);

							hit.entity->doorHandleDamageMagic(damage, *my, parent);
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}
							if ( !(my->actmagicIsOrbiting == 2) )
							{
								my->removeLightField();
								list_RemoveNode(my->mynode);
							}
							return;
						}
						else if ( hit.entity->isDamageableCollider() && hit.entity->isColliderDamageableByMagic() )
						{
							if ( spell->ID == SPELL_SLIME_FIRE )
							{
								hit.entity->SetEntityOnFire((parent && parent->getStats()) ? parent : nullptr);
							}
							playSoundEntity(hit.entity, hitsfx, hitvolume);

							hit.entity->colliderHandleDamageMagic(damage, *my, parent);
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}
							if ( !(my->actmagicIsOrbiting == 2) )
							{
								my->removeLightField();
								list_RemoveNode(my->mynode);
							}
							return;
						}
						else if ( hit.entity->behavior == &actChest )
						{
							if ( spell->ID == SPELL_SLIME_FIRE )
							{
								hit.entity->SetEntityOnFire((parent && parent->getStats()) ? parent : nullptr);
							}
							playSoundEntity(hit.entity, hitsfx, hitvolume);
							hit.entity->chestHandleDamageMagic(damage, *my, parent);
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}
							if ( !(my->actmagicIsOrbiting == 2) )
							{
								my->removeLightField();
								list_RemoveNode(my->mynode);
							}
							return;
						}
						else if ( hit.entity->behavior == &actFurniture )
						{
							if ( spell->ID == SPELL_SLIME_FIRE )
							{
								hit.entity->SetEntityOnFire((parent && parent->getStats()) ? parent : nullptr);
							}
							playSoundEntity(hit.entity, hitsfx, hitvolume);
							
							hit.entity->furnitureHandleDamageMagic(damage, *my, parent);
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}
						}
					}
				}
				else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_SPIN].element_internal_name)
					|| !strcmp(element->element_internal_name, spellElementMap[SPELL_DIZZY].element_internal_name) )
				{
					if ( hit.entity )
					{
						if ( (!mimic && hit.entity->behavior == &actMonster) )
						{
							if ( hitstats )
							{
								bool effect = false;
								Entity* caster = uidToEntity(spell->caster);
								bool dizzy = !strcmp(element->element_internal_name, spellElementMap[SPELL_DIZZY].element_internal_name);
								if ( !hitstats->getEffectActive(EFF_DISORIENTED) && hit.entity->isMobile() )
								{
									int duration = element->duration;
									if ( dizzy )
									{
										if ( hit.entity->setEffect(EFF_SPIN, true, duration, false) )
										{
											effect = true;
											if ( hit.entity->setEffect(EFF_KNOCKBACK, true, duration, false) )
											{
												real_t pushbackMultiplier = 1.0;
												if ( parent )
												{
													real_t tangent = atan2(hit.entity->y - parent->y, hit.entity->x - parent->x);
													tangent -= PI / 2;
													tangent += (local_rng.rand() % 5) * PI / 4;
													hit.entity->vel_x = cos(tangent) * pushbackMultiplier;
													hit.entity->vel_y = sin(tangent) * pushbackMultiplier;
													hit.entity->monsterKnockbackVelocity = 0.01;
													hit.entity->monsterKnockbackUID = my->parent;
													hit.entity->monsterKnockbackTangentDir = tangent;
													//hit.entity->lookAtEntity(*parent);
												}
												else
												{
													real_t tangent = atan2(hit.entity->y - my->y, hit.entity->x - my->x);
													tangent -= PI / 2;
													tangent += (local_rng.rand() % 5) * PI / 4;
													hit.entity->vel_x = cos(tangent) * pushbackMultiplier;
													hit.entity->vel_y = sin(tangent) * pushbackMultiplier;
													hit.entity->monsterKnockbackVelocity = 0.01;
													hit.entity->monsterKnockbackTangentDir = tangent;
													hit.entity->monsterKnockbackUID = 0;
													//hit.entity->lookAtEntity(*my);
												}
											}
											magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
											createParticleSpin(hit.entity);
											serverSpawnMiscParticles(hit.entity, PARTICLE_EFFECT_SPIN, -1);
											if ( caster )
											{
												messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(0, 255, 0),
													*hitstats, Language::get(6706), Language::get(6707), MSG_COMBAT);
											}
										}
									}

									if ( !dizzy && hit.entity->setEffect(EFF_DISORIENTED, (Uint8)2, duration, false) )
									{
										if ( hit.entity->monsterReleaseAttackTarget() )
										{
											magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
											effect = true;
											/*if ( caster )
											{
												hit.entity->lookAtEntity(*caster);
											}*/
											hit.entity->monsterLookDir = hit.entity->yaw;
											hit.entity->monsterLookDir += (PI - PI / 4 + (local_rng.rand() % 10) * PI / 40);
											spawnFloatingSpriteMisc(134, hit.entity->x + (-4 + local_rng.rand() % 9) + cos(hit.entity->yaw) * 2,
												hit.entity->y + (-4 + local_rng.rand() % 9) + sin(hit.entity->yaw) * 2, hit.entity->z + local_rng.rand() % 4);
											createParticleSpin(hit.entity);
											serverSpawnMiscParticles(hit.entity, PARTICLE_EFFECT_SPIN, -1);

											if ( caster )
											{
												messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(0, 255, 0),
													*hitstats, Language::get(6704), Language::get(6705), MSG_COMBAT);
											}
										}
									}
								}

								if ( !effect )
								{
									if ( caster )
									{
										messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(255, 0, 0),
											*hitstats, Language::get(2905), Language::get(2906), MSG_COMBAT);
									}
								}
							}
							spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, 1856);
						}
					}
				}
				else if ( !strcmp(element->element_internal_name, spellElement_ghostBolt.element_internal_name) )
				{
					if ( hit.entity )
					{
						if ( (!mimic && hit.entity->behavior == &actMonster) )
						{
							Entity* parent = uidToEntity(my->parent);
							real_t pushbackMultiplier = 0.6;
							if ( !hit.entity->isMobile() )
							{
								pushbackMultiplier += 0.3;
							}

							bool doSlow = true;
							const int duration = TICKS_PER_SECOND * 2;
							if ( hitstats )
							{
								if ( hitstats->getEffectActive(EFF_SLOW) || hitstats->EFFECTS_TIMERS[EFF_SLOW] > duration )
								{
									doSlow = false;
								}
							}

							if ( doSlow )
							{
								if ( hit.entity->setEffect(EFF_SLOW, true, duration, false) )
								{
									//playSoundEntity(hit.entity, 396 + local_rng.rand() % 3, 64);
									if ( parent )
									{
										Uint32 color = makeColorRGB(0, 255, 0);
										if ( parent->behavior == &actPlayer || parent->behavior == &actDeathGhost )
										{
											messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(394), Language::get(393), MSG_COMBAT);
										}
									}
									/*Uint32 color = makeColorRGB(255, 0, 0);
									if ( player >= 0 )
									{
										messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(395));
									}*/
								}
							}

							if ( hit.entity->setEffect(EFF_KNOCKBACK, true, 30, false) )
							{
								if ( parent )
								{
									real_t tangent = atan2(hit.entity->y - parent->y, hit.entity->x - parent->x);
									hit.entity->vel_x = cos(tangent) * pushbackMultiplier;
									hit.entity->vel_y = sin(tangent) * pushbackMultiplier;
									hit.entity->monsterKnockbackVelocity = 0.01;
									hit.entity->monsterKnockbackUID = my->parent;
									hit.entity->monsterKnockbackTangentDir = tangent;
									//hit.entity->lookAtEntity(*parent);
								}
								else
								{
									real_t tangent = atan2(hit.entity->y - my->y, hit.entity->x - my->x);
									hit.entity->vel_x = cos(tangent) * pushbackMultiplier;
									hit.entity->vel_y = sin(tangent) * pushbackMultiplier;
									hit.entity->monsterKnockbackVelocity = 0.01;
									hit.entity->monsterKnockbackTangentDir = tangent;
									hit.entity->monsterKnockbackUID = 0;
									//hit.entity->lookAtEntity(*my);
								}
							}
							/*if ( hit.entity->monsterAttack == 0 )
							{
								hit.entity->monsterHitTime = std::max(HITRATE - 12, hit.entity->monsterHitTime);
							}*/
						}
						else
						{
							//if ( parent )
							//{
							//	if ( parent->behavior == &actPlayer || parent->behavior == &actDeathGhost )
							//	{
							//		messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(401)); // "No telling what it did..."
							//	}
							//}
						}

						spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my->sprite);
					}
				}
				else if (!strcmp(element->element_internal_name, spellElement_locking.element_internal_name))
				{
					if ( hit.entity )
					{
						if (hit.entity->behavior == &actDoor || hit.entity->behavior == &actIronDoor )
						{
							if ( MFLAG_DISABLEOPENING || hit.entity->doorDisableOpening == 1 )
							{
								if ( parent && parent->behavior == &actPlayer )
								{
									Uint32 color = makeColorRGB(255, 0, 255);
									if ( hit.entity->behavior == &actIronDoor )
									{
										messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, 0xFFFFFFFF, Language::get(3096), Language::get(6414));
										messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, color, Language::get(6419)); // disabled locking spell.
									}
									else
									{
										messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, 0xFFFFFFFF, Language::get(3096), Language::get(3097));
										messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, color, Language::get(3101)); // disabled locking spell.
									}
								}
							}
							else
							{
								playSoundEntity(hit.entity, 92, 64);
								auto prevLocked = hit.entity->doorLocked;
								hit.entity->doorLocked = 1; //Lock the door.
								if ( parent )
								{
									if ( parent->behavior == &actPlayer )
									{
										if ( hit.entity->behavior == &actIronDoor )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(6408));
										}
										else
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(399));
										}
										if ( prevLocked != hit.entity->doorLocked )
										{
											magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
										}
									}
								}
							}
						}
						else if (hit.entity->behavior == &actChest)
						{
							//Lock chest
							playSoundEntity(hit.entity, 92, 64);
							if ( !hit.entity->chestLocked )
							{
								if ( parent && parent->behavior == &actPlayer && MFLAG_DISABLEOPENING )
								{
									Uint32 color = makeColorRGB(255, 0, 255);
									messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, 0xFFFFFFFF, Language::get(3096), Language::get(3099));
									messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, color, Language::get(3100)); // disabled locking spell.
								}
								else
								{
									auto prevLocked = hit.entity->chestLocked;
									hit.entity->lockChest();
									if ( parent )
									{
										if ( parent->behavior == &actPlayer )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(400));
											if ( prevLocked != hit.entity->chestLocked )
											{
												magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
											}
										}
									}
								}
							}
						}
						else if ( hit.entity->behavior == &actMonster && hit.entity->getMonsterTypeFromSprite() == MIMIC )
						{
							//Lock chest
							playSoundEntity(hit.entity, 92, 64);

							if ( hitstats )
							{
								//if ( MFLAG_DISABLEOPENING )
								//{
								//	if ( parent && parent->behavior == &actPlayer )
								//	{
								//		Uint32 color = makeColorRGB(255, 0, 255);
								//		messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, 0xFFFFFFFF, Language::get(3096), Language::get(3099));
								//		messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, color, Language::get(3100)); // disabled locking spell.
								//	}
								//}
								//else
								{
									if ( parent && parent->behavior == &actPlayer )
									{
										if ( mimic )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(400));
										}
										else
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(6083));
										}
									}
									if ( !hitstats->getEffectActive(EFF_MIMIC_LOCKED) )
									{
										if ( hit.entity->setEffect(EFF_MIMIC_LOCKED, true, TICKS_PER_SECOND * 5, false) )
										{
											hit.entity->monsterHitTime = HITRATE - 2;
											hitstats->monsterMimicLockedBy = parent ? parent->getUID() : 0;

											if ( parent && parent->behavior == &actPlayer )
											{
												magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
											}
										}
									}
								}
							}
						}
						else
						{
							if ( parent )
							{
								if ( parent->behavior == &actPlayer )
								{
									messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(401));
								}
							}
							if ( player >= 0 )
							{
								messagePlayer(player, MESSAGE_COMBAT, Language::get(401));
							}
						}
						spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my->sprite);
					}
				}
				else if (!strcmp(element->element_internal_name, spellElement_opening.element_internal_name))
				{
					if (hit.entity)
					{
						if ( hit.entity->behavior == &actDoor || hit.entity->behavior == &actIronDoor )
						{
							if ( MFLAG_DISABLEOPENING || hit.entity->doorDisableOpening == 1 )
							{
								if ( parent && parent->behavior == &actPlayer )
								{
									Uint32 color = makeColorRGB(255, 0, 255);
									if ( hit.entity->behavior == &actIronDoor )
									{
										messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, 0xFFFFFFFF, Language::get(3096), Language::get(6414));
										messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, color, Language::get(6419)); // disabled opening spell.
									}
									else
									{
										messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, 0xFFFFFFFF, Language::get(3096), Language::get(3097));
										messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, color, Language::get(3101)); // disabled opening spell.
									}
								}
							}
							else
							{
								// Open the Door
								playSoundEntity(hit.entity, 91, 64); // "UnlockDoor.ogg"
								if ( hit.entity->doorLocked )
								{
									if ( parent && parent->behavior == &actPlayer )
									{
										if ( hit.entity->behavior == &actIronDoor )
										{
											Compendium_t::Events_t::eventUpdateWorld(parent->skill[2], Compendium_t::CPDM_DOOR_UNLOCKED, "iron door", 1);
										}
										else
										{
											Compendium_t::Events_t::eventUpdateWorld(parent->skill[2], Compendium_t::CPDM_DOOR_UNLOCKED, "door", 1);
										}
									}
								}
								magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
								hit.entity->doorLocked = 0; // Unlocks the Door
								hit.entity->doorPreventLockpickExploit = 1;

								if ( !hit.entity->skill[0] && !hit.entity->skill[3] )
								{
									hit.entity->skill[3] = 1 + (my->x > hit.entity->x); // Opens the Door
									playSoundEntity(hit.entity, 21, 96); // "UnlockDoor.ogg"
								}
								else if ( hit.entity->skill[0] && !hit.entity->skill[3] )
								{
									hit.entity->skill[3] = 1 + (my->x < hit.entity->x); // Opens the Door
									playSoundEntity(hit.entity, 21, 96); // "UnlockDoor.ogg"
								}
								if ( parent )
								{
									if ( parent->behavior == &actPlayer)
									{
										if ( hit.entity->behavior == &actIronDoor )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(6411));
										}
										else
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(402));
										}
									}
								}
							}
						}
						else if ( hit.entity->behavior == &actGate )
						{
							if ( MFLAG_DISABLEOPENING || hit.entity->gateDisableOpening == 1 )
							{
								if ( parent && parent->behavior == &actPlayer )
								{
									Uint32 color = makeColorRGB(255, 0, 255);
									messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, 0xFFFFFFFF, Language::get(3096), Language::get(3098));
									messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, color, Language::get(3102)); // disabled opening spell.
								}
							}
							else
							{
								// Open the Gate
								if ( (hit.entity->skill[28] != 2 && hit.entity->gateInverted == 0)
									|| (hit.entity->skill[28] != 1 && hit.entity->gateInverted == 1) )
								{
									if ( hit.entity->gateInverted == 1 )
									{
										hit.entity->skill[28] = 1; // Depowers the Gate
									}
									else
									{
										hit.entity->skill[28] = 2; // Powers the Gate
									}
									if ( parent )
									{
										if ( parent->behavior == &actPlayer )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(403)); // "The spell opens the gate!"
											Compendium_t::Events_t::eventUpdateWorld(parent->skill[2], Compendium_t::CPDM_GATE_OPENED_SPELL, "portcullis", 1);
											magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
										}
									}
								}
							}
						}
						else if ( hit.entity->behavior == &actChest )
						{
							// Unlock the Chest
							if ( hit.entity->chestLocked )
							{
								if ( MFLAG_DISABLEOPENING )
								{
									if ( parent && parent->behavior == &actPlayer )
									{
										Uint32 color = makeColorRGB(255, 0, 255);
										messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, 0xFFFFFFFF, Language::get(3096), Language::get(3099));
										messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, color, Language::get(3100)); // disabled opening spell.
									}
								}
								else
								{
									playSoundEntity(hit.entity, 91, 64); // "UnlockDoor.ogg"
									auto prevLocked = hit.entity->chestLocked;
									hit.entity->unlockChest();
									if ( parent )
									{
										if ( parent->behavior == &actPlayer)
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(404)); // "The spell unlocks the chest!"
											Compendium_t::Events_t::eventUpdateWorld(parent->skill[2], Compendium_t::CPDM_CHESTS_UNLOCKED, "chest", 1);
											if ( prevLocked != hit.entity->chestLocked )
											{
												magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
											}
										}
									}
								}
							}
						}
						else if ( hit.entity->behavior == &actPowerCrystalBase )
						{
							Entity* childentity = nullptr;
							if ( hit.entity->children.first )
							{
								childentity = static_cast<Entity*>((&hit.entity->children)->first->element);
								if ( childentity != nullptr )
								{
									//Unlock crystal
									if ( childentity->crystalSpellToActivate )
									{
										playSoundEntity(hit.entity, 151, 128);
										childentity->crystalSpellToActivate = 0;
										// send the clients the updated skill.
										serverUpdateEntitySkill(childentity, 10);
										if ( parent )
										{
											if ( parent->behavior == &actPlayer )
											{
												messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(2358));
												magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
											}
										}
									}
								}
							}
						}
						else if ( hit.entity->behavior == &actMonster && hit.entity->getMonsterTypeFromSprite() == MIMIC )
						{
							if ( hit.entity->isInertMimic() )
							{
								if ( hitstats->getEffectActive(EFF_MIMIC_LOCKED) )
								{
									hit.entity->setEffect(EFF_MIMIC_LOCKED, false, 0, false);
								}
								if ( hit.entity->disturbMimic(parent, false, true) )
								{
									if ( parent )
									{
										if ( parent->behavior == &actPlayer )
										{
											messagePlayer(parent->skill[2], MESSAGE_INTERACTION, Language::get(6081));
										}
									}
									magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
								}
							}
							else
							{
								if ( hitstats )
								{
									//if ( MFLAG_DISABLEOPENING )
									//{
									//	if ( parent && parent->behavior == &actPlayer )
									//	{
									//		Uint32 color = makeColorRGB(255, 0, 255);
									//		messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, 0xFFFFFFFF, Language::get(3096), Language::get(3099));
									//		messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, color, Language::get(3100)); // disabled locking spell.
									//	}
									//}
									//else
									{
										if ( hitstats->getEffectActive(EFF_MIMIC_LOCKED) )
										{
											if ( hit.entity->setEffect(EFF_MIMIC_LOCKED, false, 0, false) )
											{
												hit.entity->monsterHitTime = std::max(hit.entity->monsterHitTime, HITRATE / 2);
												magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
											}
										}
									}
								}
							}
						}
						else
						{
							if ( parent )
							{
								if ( parent->behavior == &actPlayer )
								{
									messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(401)); // "No telling what it did..."
								}
							}

							if ( player >= 0 )
							{
								messagePlayer(player, MESSAGE_COMBAT, Language::get(401)); // "No telling what it did..."
							}
						}

						spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my->sprite);
					}
				}
				else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_SPLINTER_GEAR].element_internal_name) )
				{
					if ( hit.entity )
					{
						real_t ratio = 1.0;
						bool found = false;
						bool criticalEffect = false;
						Entity* parent = uidToEntity(my->parent);
						if ( mimic || hit.entity->behavior == &actChest )
						{
							found = true;
							ratio = std::max(100, getSpellDamageSecondaryFromID(SPELL_SPLINTER_GEAR, parent, nullptr, my)) / 100.0;
							if ( applyGenericMagicDamage(parent, hit.entity, *my, spell->ID, damage * ratio, false) )
							{
								criticalEffect = true;
							}
						}
						else if ( hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer )
						{
							Item* armor = nullptr;
							bool shields = hitstats->shield && itemCategory(hitstats->shield) == ARMOR;
							int armornum = hitstats->pickRandomEquippedItemToDegradeOnHit(&armor, true, !shields, false, true);

							if ( armor != nullptr && armor->status > BROKEN )
							{
								ItemType type = armor->type;
								if ( hit.entity->degradeArmor(*hitstats, *armor, armornum) )
								{
									found = true;
									if ( !armor || (armor && armor->status == BROKEN) )
									{
										criticalEffect = true;
										ratio = std::max(100, getSpellDamageSecondaryFromID(SPELL_SPLINTER_GEAR, parent, nullptr, my)) / 100.0;
									}

									if ( parent )
									{
										if ( armor->status > BROKEN )
										{
											const char* msg = Language::get(6678); // named
											if ( !strcmp(hitstats->name, "") || monsterNameIsGeneric(*hitstats) )
											{
												msg = Language::get(6677);
											}

											if ( !strcmp(hitstats->name, "") )
											{
												messagePlayerColor(parent->isEntityPlayer(), MESSAGE_COMBAT, makeColorRGB(0, 255, 0),
													msg, getMonsterLocalizedName(hitstats->type).c_str(), items[type].getIdentifiedName());
											}
											else
											{
												messagePlayerColor(parent->isEntityPlayer(), MESSAGE_COMBAT, makeColorRGB(0, 255, 0),
													msg, hitstats->name, items[type].getIdentifiedName());
											}
										}
										else
										{
											const char* msg = Language::get(6680); // named
											if ( !strcmp(hitstats->name, "") || monsterNameIsGeneric(*hitstats) )
											{
												msg = Language::get(6679);
											}
											if ( !strcmp(hitstats->name, "") )
											{
												messagePlayerColor(parent->isEntityPlayer(), MESSAGE_COMBAT, makeColorRGB(0, 255, 0),
													msg, getMonsterLocalizedName(hitstats->type).c_str(), items[type].getIdentifiedName());
											}
											else
											{
												messagePlayerColor(parent->isEntityPlayer(), MESSAGE_COMBAT, makeColorRGB(0, 255, 0),
													msg, hitstats->name, items[type].getIdentifiedName());
											}
										}
									}
								}
							}
							if ( hitstats->type == CRYSTALGOLEM
								|| hitstats->type == AUTOMATON
								|| hitstats->type == SENTRYBOT
								|| hitstats->type == SPELLBOT
								|| hitstats->type == DUMMYBOT
								|| hitstats->type == MINIMIMIC
								|| hitstats->type == MIMIC )
							{
								if ( !found )
								{
									if ( parent )
									{
										messagePlayerMonsterEvent(parent->isEntityPlayer(), makeColorRGB(0, 255, 0),
											*hitstats, Language::get(6809), Language::get(6810), MSG_COMBAT);
									}
								}
								found = true;
								if ( !criticalEffect )
								{
									criticalEffect = true;
									ratio = std::max(100, getSpellDamageSecondaryFromID(SPELL_SPLINTER_GEAR, parent, nullptr, my)) / 100.0;
								}
							}
						}

						if ( found )
						{
							if ( hit.entity->behavior == &actPlayer )
							{
								if ( criticalEffect )
								{
									messagePlayerColor(hit.entity->isEntityPlayer(), MESSAGE_STATUS, makeColorRGB(255, 0, 0), Language::get(6812));
								}
								else
								{
									messagePlayerColor(hit.entity->isEntityPlayer(), MESSAGE_STATUS, makeColorRGB(255, 0, 0), Language::get(6811));
								}
							}

							for ( int i = 0; i < 5; ++i )
							{
								Entity* gib = spawnGib(hit.entity, 2208);
								gib->sprite = 2208;
								serverSpawnGibForClient(gib);
							}
							if ( criticalEffect )
							{
								Entity* spellTimer = createParticleTimer(hit.entity, 15, -1);
								spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SPLINTER_GEAR;
								spellTimer->particleTimerCountdownSprite = -1;
								spellTimer->yaw = hit.entity->yaw;
								spellTimer->x = hit.entity->x;
								spellTimer->y = hit.entity->y;
								spellTimer->flags[NOUPDATE] = false; // spawn for client
								spellTimer->flags[UPDATENEEDED] = true;
								Sint32 val = (1 << 31);
								val |= (Uint8)(19);
								val |= (((Uint16)(spellTimer->particleTimerDuration) & 0xFFF) << 8);
								val |= (Uint8)(spellTimer->particleTimerCountdownAction & 0xFF) << 20;
								spellTimer->skill[2] = val;
							}

							if ( hitstats )
							{
								if ( hit.entity->setEffect(EFF_BLEEDING, true, element->duration, false) )
								{
									if ( parent )
									{
										hitstats->bleedInflictedBy = parent->getUID();
									}
								}
								hit.entity->setEffect(EFF_SLOW, true, element->duration, false);
								if ( !mimic )
								{
									if ( applyGenericMagicDamage(parent, hit.entity, *my, spell->ID, damage * ratio, false) )
									{

									}
								}
							}
							spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, 171);
							playSoundEntity(hit.entity, 167, 128);
						}
						else
						{
							if ( parent && hitstats )
							{
								messagePlayerMonsterEvent(parent->isEntityPlayer(), makeColorRGB(255, 0, 0),
									*hitstats, Language::get(2905), Language::get(2906), MSG_COMBAT);
							}
						}
					}
				}
				else if (!strcmp(element->element_internal_name, spellElement_dig.element_internal_name))
				{
					if ( !hit.entity )
					{
						if ( hit.mapx >= 1 && hit.mapx < map.width - 1 && hit.mapy >= 1 && hit.mapy < map.height - 1 )
						{
							magicDig(parent, my, 8, 4);
						}
					}
					else
					{
						if ( hit.entity->behavior == &actBoulder )
						{
							if ( hit.entity->sprite == 989 || hit.entity->sprite == 990 )
							{
								magicDig(parent, my, 0, 1);
							}
							else
							{
								magicDig(parent, my, 8, 4);
							}
						}
						else if ( hit.entity->behavior == &actColliderDecoration && hit.entity->colliderDiggable != 0 )
						{
							magicDig(parent, my, 1, 0);
						}
						else
						{
							if ( parent )
							{
								if ( parent->behavior == &actPlayer )
								{
									messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(401));
								}
							}
							if ( player >= 0 )
							{
								messagePlayer(player, MESSAGE_COMBAT, Language::get(401));
							}
						}
					}
				}
				else if ( !strcmp(element->element_internal_name, spellElement_stoneblood.element_internal_name) )
				{
					if ( hit.entity )
					{
						if ( (!mimic && hit.entity->behavior == &actMonster) || hit.entity->behavior == &actPlayer )
						{
							int effectDuration = element->duration;
							effectDuration = convertResistancePointsToMagicValue(effectDuration, resistance);
							int oldDuration = !hitstats->getEffectActive(EFF_PARALYZED) ? 0 : hitstats->EFFECTS_TIMERS[EFF_PARALYZED];
							if ( hit.entity->setEffect(EFF_PARALYZED, true, effectDuration, false) )
							{
								magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
								if ( hit.entity->behavior == &actPlayer )
								{
									serverUpdateEffects(hit.entity->skill[2]);
								}
								
								// notify if effect wasn't active with identical duration, few ticks leeway
								if ( abs(hitstats->EFFECTS_TIMERS[EFF_PARALYZED] - oldDuration) > 10 ) 
								{
									playSoundEntity(hit.entity, 172, 64); //TODO: Paralyze spell sound.
									if ( parent )
									{
										Uint32 color = makeColorRGB(0, 255, 0);
										if ( parent->behavior == &actPlayer )
										{
											messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(2421), Language::get(2420), MSG_COMBAT);
										}
									}

									Uint32 color = makeColorRGB(255, 0, 0);
									if ( player >= 0 )
									{
										messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(2422));
									}
								}
							}
							else
							{
								if ( parent )
								{
									Uint32 color = makeColorRGB(255, 0, 0);
									if ( parent->behavior == &actPlayer )
									{
										messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(2905), Language::get(2906), MSG_COMBAT);
									}
								}
							}
							spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my->sprite);
						}
					}
				}
				else if ( !strcmp(element->element_internal_name, spellElement_bleed.element_internal_name) )
				{
					playSoundEntity(my, 173, 128);
					if ( hit.entity )
					{
						if ( (hit.entity->behavior == &actMonster && !mimic) || hit.entity->behavior == &actPlayer )
						{
							Entity* parent = uidToEntity(my->parent);
							playSoundEntity(my, 173, 64);
							playSoundEntity(hit.entity, 28, 128);

							Stat* casterStats = nullptr;
							if ( parent )
							{
								casterStats = parent->getStats();
							}
							
							Sint32 oldHP = hitstats->HP;
							hit.entity->modHP(-damage);
							magicOnEntityHit(parent, my, hit.entity, hitstats, preResistanceDamage, damage, oldHP, spell ? spell->ID : SPELL_NONE);
							magicTrapOnHit(parent, hit.entity, hitstats, oldHP, spell ? spell->ID : SPELL_NONE);

							// write the obituary
							if ( parent )
							{
								parent->killedByMonsterObituary(hit.entity);
							}

							int bleedDuration = element->duration;
							bleedDuration = convertResistancePointsToMagicValue(bleedDuration, resistance);
							bool wasBleeding = hit.entity->getStats() ? hit.entity->getStats()->getEffectActive(EFF_BLEEDING) : false;
							if ( bleedDuration > 0 && hit.entity->setEffect(EFF_BLEEDING, true, bleedDuration, false) )
							{
								if ( parent )
								{
									hitstats->bleedInflictedBy = static_cast<Sint32>(my->parent);
								}
								if ( !wasBleeding && parent && casterStats )
								{
									// energize if wearing punisher hood!
									if ( casterStats->helmet && casterStats->helmet->type == PUNISHER_HOOD )
									{
										parent->modMP(1 + local_rng.rand() % 2);
										Uint32 color = makeColorRGB(0, 255, 0);
										parent->setEffect(EFF_MP_REGEN, true, std::max(casterStats->EFFECTS_TIMERS[EFF_MP_REGEN], 10 * TICKS_PER_SECOND), false);
										if ( parent->behavior == &actPlayer )
										{
											messagePlayerColor(parent->skill[2], MESSAGE_HINT, color, Language::get(3753));
											steamStatisticUpdateClient(parent->skill[2], STEAM_STAT_ITS_A_LIVING, STEAM_STAT_INT, 1);
										}
										playSoundEntity(parent, 168, 128);
									}
								}
							}

							int slowDuration = element->duration / 4;
							slowDuration = convertResistancePointsToMagicValue(slowDuration, resistance);
							if ( slowDuration > 0 && hit.entity->setEffect(EFF_SLOW, true, slowDuration, false) )
							{

							}
							if ( hit.entity->behavior == &actPlayer )
							{
								serverUpdateEffects(hit.entity->skill[2]);
							}
							// update enemy bar for attacker
							if ( parent )
							{
								Uint32 color = makeColorRGB(0, 255, 0);
								if ( parent->behavior == &actPlayer )
								{
									messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(2424), Language::get(2423), MSG_COMBAT);
								}
							}

							// write the obituary
							if ( parent )
							{
								parent->killedByMonsterObituary(hit.entity);
							}

							// update enemy bar for attacker
							if ( !strcmp(hitstats->name, "") )
							{
								updateEnemyBar(parent, hit.entity, getMonsterLocalizedName(hitstats->type).c_str(), hitstats->HP, hitstats->MAXHP,
									false, dmgGib);
							}
							else
							{
								updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP,
									false, dmgGib);
							}

							if ( hitstats->HP <= 0 && parent )
							{
								parent->awardXP(hit.entity, true, true);

								if ( hit.entity->behavior == &actMonster )
								{
									bool tryBloodVial = false;
									if ( gibtype[hitstats->type] == 1 || gibtype[hitstats->type] == 2 )
									{
										for ( int c = 0; c < MAXPLAYERS; ++c )
										{
											if ( playerRequiresBloodToSustain(c) )
											{
												tryBloodVial = true;
												break;
											}
										}
										if ( tryBloodVial )
										{
											Item* blood = newItem(FOOD_BLOOD, EXCELLENT, 0, 1, gibtype[hitstats->type] - 1, true, &hitstats->inventory);
										}
									}
								}
							}

							Uint32 color = makeColorRGB(255, 0, 0);
							if ( player >= 0 )
							{
								messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(2425));
							}
							spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my->sprite);
							for ( int gibs = 0; gibs < 10; ++gibs )
							{
								Entity* gib = spawnGib(hit.entity);
								serverSpawnGibForClient(gib);
							}
						}
						else
						{
							Entity* caster = uidToEntity(spell->caster);
							bool forceFurnitureDamage = false;
							if ( caster && caster->behavior == &actMonster && caster->getMonsterTypeFromSprite() == SHOPKEEPER )
							{
								forceFurnitureDamage = true;
							}

							if ( forceFurnitureDamage )
							{
								if ( hit.entity->isDamageableCollider() && hit.entity->isColliderDamageableByMagic() )
								{
									hit.entity->colliderHandleDamageMagic(damage, *my, parent);
									if ( my->actmagicProjectileArc > 0 )
									{
										Entity* caster = uidToEntity(spell->caster);
										spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
									}
									if ( !(my->actmagicIsOrbiting == 2) )
									{
										my->removeLightField();
										list_RemoveNode(my->mynode);
									}
									return;
								}
								else if ( hit.entity->behavior == &actChest || mimic )
								{
									hit.entity->chestHandleDamageMagic(damage, *my, parent);
									if ( my->actmagicProjectileArc > 0 )
									{
										Entity* caster = uidToEntity(spell->caster);
										spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
									}
									if ( !(my->actmagicIsOrbiting == 2) )
									{
										my->removeLightField();
										list_RemoveNode(my->mynode);
									}
									return;
								}
								else if ( hit.entity->behavior == &actFurniture )
								{
									hit.entity->furnitureHandleDamageMagic(damage, *my, parent);
									if ( my->actmagicProjectileArc > 0 )
									{
										Entity* caster = uidToEntity(spell->caster);
										spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
									}
								}
							}
						}
					}
				}
				else if ( !strcmp(element->element_internal_name, spellElement_dominate.element_internal_name) )
				{
					Entity *caster = uidToEntity(spell->caster);
					if ( caster )
					{
						if ( spellEffectDominate(*my, *element, *caster, parent) )
						{
							//Success
							magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
						}
					}
				}
				else if ( !strcmp(element->element_internal_name, spellElement_acidSpray.element_internal_name) )
				{
					Entity* caster = uidToEntity(spell->caster);
					if ( caster )
					{
						spellEffectAcid(*my, *element, parent, preResistanceDamage, resistance);
					}
				}
				else if ( !strcmp(element->element_internal_name, spellElement_poison.element_internal_name) )
				{
					Entity* caster = uidToEntity(spell->caster);
					if ( caster )
					{
						spellEffectPoison(*my, *element, parent, preResistanceDamage, resistance);
					}
				}
				else if ( !strcmp(element->element_internal_name, spellElement_sprayWeb.element_internal_name) )
				{
					Entity* caster = uidToEntity(spell->caster);
					if ( caster )
					{
						spellEffectSprayWeb(*my, *element, parent, resistance);
					}
				}
				else if ( !strcmp(element->element_internal_name, spellElement_stealWeapon.element_internal_name) )
				{
					Entity* caster = uidToEntity(spell->caster);
					if ( caster )
					{
						spellEffectStealWeapon(*my, *element, parent, resistance);
					}
				}
				else if ( !strcmp(element->element_internal_name, spellElement_drainSoul.element_internal_name) )
				{
					Entity* caster = uidToEntity(spell->caster);
					if ( caster )
					{
						spellEffectDrainSoul(*my, *element, parent, preResistanceDamage, resistance);
					}
				}
				else if ( !strcmp(element->element_internal_name, spellElement_charmMonster.element_internal_name) )
				{
					Entity* caster = uidToEntity(spell->caster);
					if ( caster )
					{
						spellEffectCharmMonster(*my, *element, parent, resistance, static_cast<bool>(my->actmagicCastByMagicstaff));
					}
				}
				else if ( !strcmp(element->element_internal_name, spellElement_telePull.element_internal_name) )
				{
					Entity* caster = uidToEntity(spell->caster);
					if ( caster )
					{
						if ( spellEffectTeleportPull(my, *element, parent, hit.entity, resistance) )
						{
							magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
						}
					}
				}
				else if ( !strcmp(element->element_internal_name, spellElement_shadowTag.element_internal_name) )
				{
					Entity* caster = uidToEntity(spell->caster);
					if ( caster )
					{
						spellEffectShadowTag(*my, *element, parent, resistance);
					}
				}
				else if ( !strcmp(element->element_internal_name, spellElement_demonIllusion.element_internal_name) )
				{
					Entity* caster = uidToEntity(spell->caster);
					if ( caster )
					{
						if ( spellEffectDemonIllusion(*my, *element, parent, hit.entity, resistance) )
						{
							magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
						}
					}
				}

				if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_SPHERE_SILENCE].element_internal_name) )
				{
					real_t spawnx = my->x;
					real_t spawny = my->y;
					Entity* follow = nullptr;
					if ( !hit.entity )
					{
						if ( hit.mapx >= 0 && hit.mapx < map.width - 1 && hit.mapy >= 1 && hit.mapy < map.height - 1 )
						{
							real_t x = my->x - 4.0 * cos(my->yaw);
							real_t y = my->y - 4.0 * sin(my->yaw);
							int mapx = x / 16;
							int mapy = y / 16;
							if ( mapx >= 1 && mapx < map.width - 2 && mapy >= 1 && mapy < map.height - 2 )
							{
								spawnx = x;
								spawny = y;
							}
						}
					}
					else
					{
						if ( (hit.entity->behavior == &actMonster && !mimic) || hit.entity->behavior == &actPlayer )
						{
							spawnx = hit.entity->x;
							spawny = hit.entity->y;
							follow = hit.entity;
						}
						else if ( hit.entity->behavior == &actChest || mimic )
						{
							spawnx = hit.entity->x;
							spawny = hit.entity->y;
							follow = hit.entity;
						}
					}


					createRadiusMagic(spell->ID, uidToEntity(spell->caster), spawnx, spawny, 
						std::max(16, std::min(255, getSpellEffectDurationSecondaryFromID(SPELL_SPHERE_SILENCE, parent, nullptr, my))),
						getSpellEffectDurationFromID(SPELL_SPHERE_SILENCE, parent, nullptr, my),
						follow);
				}
				else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_SHADE_BOLT].element_internal_name)
					|| !strcmp(element->element_internal_name, spellElementMap[SPELL_WONDERLIGHT].element_internal_name) )
				{
					real_t spawnx = my->x;
					real_t spawny = my->y;
					Uint32 lightParent = 0;
					if ( !hit.entity )
					{
						if ( hit.mapx >= 0 && hit.mapx < map.width - 1 && hit.mapy >= 1 && hit.mapy < map.height - 1 )
						{
							real_t x = my->x - 4.0 * cos(my->yaw);
							real_t y = my->y - 4.0 * sin(my->yaw);
							int mapx = x / 16;
							int mapy = y / 16;
							if ( mapx >= 1 && mapx < map.width - 2 && mapy >= 1 && mapy < map.height - 2 )
							{
								spawnx = x;
								spawny = y;
							}
						}
					}
					else
					{
						if ( (hit.entity->behavior == &actMonster && !mimic) || hit.entity->behavior == &actPlayer )
						{
							spawnx = hit.entity->x;
							spawny = hit.entity->y;
							lightParent = hit.entity->getUID();
							if ( spell->ID == SPELL_SHADE_BOLT )
							{
								if ( hit.entity->behavior == &actMonster
									&& hit.entity->setEffect(EFF_BLIND, true, element->duration, false) )
								{
									if ( hit.entity->behavior == &actMonster && !hit.entity->isBossMonster() )
									{
										hit.entity->monsterReleaseAttackTarget();
									}
									if ( hit.entity->behavior == &actPlayer )
									{
										Uint32 color = makeColorRGB(255, 0, 0);
										messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(765));
									}
									if ( parent && parent->behavior == &actPlayer )
									{
										Uint32 color = makeColorRGB(0, 255, 0);
										messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(3878), Language::get(3879), MSG_COMBAT);
									}
								}
							}
						}
						else if ( hit.entity->behavior == &actChest || mimic )
						{
							spawnx = hit.entity->x;
							spawny = hit.entity->y;
							lightParent = hit.entity->getUID();
						}
					}

					Entity* entity = newEntity(spell->ID == SPELL_SHADE_BOLT ? 1801 : 1802, 1, map.entities, nullptr); // black magic ball
					entity->parent = lightParent; // who to follow
					entity->x = spawnx;
					entity->y = spawny;
					entity->z = my->z;
					entity->skill[7] = -5; //Base z.
					entity->fskill[1] = entity->skill[7] - entity->z;
					entity->fskill[2] = 0.0;
					entity->sizex = 1;
					entity->sizey = 1;
					entity->yaw = my->yaw + PI;
					entity->flags[UPDATENEEDED] = true;
					entity->flags[PASSABLE] = true;
					entity->behavior = &actMagiclightBall;
					entity->skill[4] = spawnx; //Store what x it started shooting out from the player at.
					entity->skill[5] = spawny; //Store what y it started shooting out from the player at.
					entity->skill[0] = 1; // set init to not shoot out

					entity->skill[12] = getSpellEffectDurationSecondaryFromID(spell->ID, parent, nullptr, my);
					node_t* spellnode = list_AddNodeLast(&entity->children);
					spellnode->element = copySpell(getSpellFromID(spell->ID == SPELL_SHADE_BOLT ? SPELL_DEEP_SHADE : SPELL_LIGHT)); //We need to save the spell since this is a channeled spell.
					spellnode->size = sizeof(spell_t);
					((spell_t*)spellnode->element)->caster = spell->caster;
					((spell_t*)spellnode->element)->magicstaff = true;
					spellnode->deconstructor = &spellDeconstructor;
					playSoundEntity(entity, 165, 128);
				}

				if ( hitstats )
				{
					if ( player >= 0 )
					{
						entityHealth -= hitstats->HP;
						if ( entityHealth > 0 )
						{
							// entity took damage, shake screen.
							if ( multiplayer == SERVER && player > 0 )
							{
								strcpy((char*)net_packet->data, "SHAK");
								net_packet->data[4] = 10; // turns into .1
								net_packet->data[5] = 10;
								net_packet->address.host = net_clients[player - 1].host;
								net_packet->address.port = net_clients[player - 1].port;
								net_packet->len = 6;
								sendPacketSafe(net_sock, -1, net_packet, player - 1);
							}
							else if (player == 0 || (splitscreen && player > 0) )
							{
								cameravars[player].shakex += .1;
								cameravars[player].shakey += 10;
							}
						}
					}
					else
					{
						if ( parent && parent->behavior == &actPlayer )
						{
							if ( hitstats->HP <= 0 )
							{
								if ( hitstats->type == SCARAB )
								{
									// killed a scarab with magic.
									steamAchievementEntity(parent, "BARONY_ACH_THICK_SKULL");
								}
								if ( my->actmagicMirrorReflected == 1 && static_cast<Uint32>(my->actmagicMirrorReflectedCaster) == hit.entity->getUID() )
								{
									// killed a monster with it's own spell with mirror reflection.
									steamAchievementEntity(parent, "BARONY_ACH_NARCISSIST");
								}
								if ( stats[parent->skill[2]] && stats[parent->skill[2]]->playerRace == RACE_INSECTOID && stats[parent->skill[2]]->stat_appearance == 0 )
								{
									if ( !achievementObserver.playerAchievements[parent->skill[2]].gastricBypass )
									{
										if ( achievementObserver.playerAchievements[parent->skill[2]].gastricBypassSpell.first == spell->ID )
										{
											Uint32 oldTicks = achievementObserver.playerAchievements[parent->skill[2]].gastricBypassSpell.second;
											if ( parent->ticks - oldTicks < TICKS_PER_SECOND * 5 )
											{
												steamAchievementEntity(parent, "BARONY_ACH_GASTRIC_BYPASS");
												achievementObserver.playerAchievements[parent->skill[2]].gastricBypass = true;
											}
										}
									}
								}
							}
						}
					}
				}

				if ( my->actmagicProjectileArc > 0 )
				{
					Entity* caster = uidToEntity(spell->caster);
					if ( spell->ID == SPELL_SPORE_BOMB || spell->ID == SPELL_MYCELIUM_BOMB )
					{
						floorMagicCreateSpores(hit.entity, my->x, my->y, parent, preResistanceDamage, spell->ID);
					}
					else
					{
						spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
					}
				}

				if ( (spell->ID == SPELL_METEOR || spell->ID == SPELL_METEOR_SHOWER)
					&& (!strcmp(element->element_internal_name, spellElementMap[SPELL_METEOR_SHOWER].element_internal_name)
					|| !strcmp(element->element_internal_name, spellElementMap[SPELL_METEOR].element_internal_name)) )
				{
					Entity* caster = uidToEntity(spell->caster);
					createSpellExplosionArea(spell->ID, caster, my->x, my->y, my->z, 32.0, preResistanceDamage, hit.entity);
				}

				if ( spell->ID == SPELL_SCEPTER_BLAST || spell->ID == SPELL_BLOOD_WAVES || spell->ID == SPELL_HOLY_BEAM )
				{
					if ( hit.entity && ((hit.entity->behavior == &actMonster && !mimic) || hit.entity->behavior == &actPlayer) )
					{
						// phase through
					}
					else
					{
						my->removeLightField();
						if ( my->mynode )
						{
							list_RemoveNode(my->mynode);
						}
					}
				}
				else if ( !(my->actmagicIsOrbiting == 2) )
				{
					my->removeLightField();
					if ( my->mynode )
					{
						list_RemoveNode(my->mynode);
					}
				}
				return;
			}

			if ( my->actmagicSpray == 1 )
			{
				my->vel_x = my->vel_x * .95;
				my->vel_y = my->vel_y * .95;
			}
		}

		//Go down two levels to the next element. This will need to get re-written shortly.
		node = spell->elements.first;
		element = (spellElement_t*)node->element;
		//element = (spellElement_t *)spell->elements->first->element;
		//element = (spellElement_t *)element->elements->first->element; //Go down two levels to the second element.
		node = element->elements.first;
		element = (spellElement_t*)node->element;
		//if (!strcmp(element->element_internal_name, spellElement_fire.element_internal_name)
		//	|| !strcmp(element->element_internal_name, spellElement_lightning.element_internal_name))
        if (1)
		{
			//Make the ball light up stuff as it travels.
			if ( !my->flags[INVISIBLE] || my->flags[INVISIBLE_DITHER] )
			{
				my->light = addLight(my->x / 16, my->y / 16, magicLightColorForSprite(my, my->sprite, false));
			}

			if ( flickerLights )
			{
				//Magic light ball will never flicker if this setting is disabled.
				lightball_flicker++;
			}
			my->skill[2] = -11; // so clients know to create a light field

			if (lightball_flicker > 5)
			{
				lightball_lighting = (lightball_lighting == 1) + 1;

				if (lightball_lighting == 1)
				{
					my->removeLightField();
					if ( !my->flags[INVISIBLE] || my->flags[INVISIBLE_DITHER] )
					{
						my->light = addLight(my->x / 16, my->y / 16, magicLightColorForSprite(my, my->sprite, false));
					}
				}
				else
				{
					my->removeLightField();
					if ( !my->flags[INVISIBLE] || my->flags[INVISIBLE_DITHER] )
					{
						my->light = addLight(my->x / 16, my->y / 16, magicLightColorForSprite(my, my->sprite, true));
					}
				}
				lightball_flicker = 0;
			}
		}
		else
		{
			my->skill[2] = -12; // so clients know to simply spawn particles
		}

		// spawn particles
		if ( my->actmagicSpray == 1 )
		{
		}
		else if ( !my->flags[INVISIBLE] || my->flags[INVISIBLE_DITHER] )
		{
			spawnBasicMagicParticleForMissile(my);
		}

		spawnAdditionalParticleForMissile(my);
	}
}

void actMagicClient(Entity* my)
{
	my->removeLightField();
	if ( !my->flags[INVISIBLE] || my->flags[INVISIBLE_DITHER] )
	{
		my->light = addLight(my->x / 16, my->y / 16, magicLightColorForSprite(my, my->sprite, false));
	}

	if ( flickerLights )
	{
		//Magic light ball will never flicker if this setting is disabled.
		lightball_flicker++;
	}
	my->skill[2] = -11; // so clients know to create a light field

	if (lightball_flicker > 5)
	{
		lightball_lighting = (lightball_lighting == 1) + 1;

		if (lightball_lighting == 1)
		{
			my->removeLightField();
			if ( !my->flags[INVISIBLE] || my->flags[INVISIBLE_DITHER] )
			{
				my->light = addLight(my->x / 16, my->y / 16, magicLightColorForSprite(my, my->sprite, false));
			}
		}
		else
		{
			my->removeLightField();
			if ( !my->flags[INVISIBLE] || my->flags[INVISIBLE_DITHER] )
			{
				my->light = addLight(my->x / 16, my->y / 16, magicLightColorForSprite(my, my->sprite, true));
			}
		}
		lightball_flicker = 0;
	}

	// spawn particles
	if ( !my->flags[INVISIBLE] || my->flags[INVISIBLE_DITHER] )
	{
		spawnBasicMagicParticleForMissile(my);
	}

	spawnAdditionalParticleForMissile(my);
}

void actMagicClientNoLight(Entity* my)
{
	// simply spawn particles
	if ( !my->flags[INVISIBLE] || my->flags[INVISIBLE_DITHER] )
	{
		if ( *cvar_magic_fx_use_vismap && !intro )
		{
			int x = my->x / 16.0;
			int y = my->y / 16.0;
			if ( x >= 0 && x < map.width && y >= 0 && y < map.height )
			{
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( !client_disconnected[i] && players[i]->isLocalPlayer() && cameras[i].vismap[y + x * map.height] )
					{
						if ( Entity* particle = spawnMagicParticle(my) )
						{
							particle->flags[SPRITE] = my->flags[SPRITE];
							particle->flags[INVISIBLE] = my->flags[INVISIBLE];
							particle->flags[INVISIBLE_DITHER] = my->flags[INVISIBLE_DITHER];
						}
						break;
					}
				}
			}
		}
		else
		{
			if ( Entity* particle = spawnMagicParticle(my) )
			{
				particle->flags[SPRITE] = my->flags[SPRITE];
				particle->flags[INVISIBLE] = my->flags[INVISIBLE];
				particle->flags[INVISIBLE_DITHER] = my->flags[INVISIBLE_DITHER];
			}
		}
	}
}

void actMagicParticle(Entity* my)
{
	my->x += my->vel_x;
	my->y += my->vel_y;
	my->z += my->vel_z;
	if ( my->sprite == 943 || my->sprite == 979 )
	{
		my->scalex -= 0.05;
		my->scaley -= 0.05;
		my->scalez -= 0.05;
	}
	else if ( my->sprite == 1764 ) // lightning impact
	{
		if ( my->ticks > 5 )
		{
			my->scalex -= 0.1;
			my->scaley -= 0.1;
			my->scalez -= 0.1;
			if ( my->ticks > 10 )
			{
				//list_RemoveNode(my->mynode);
				//return;
			}
		}
	}
	else if ( my->sprite == 1787 || (my->sprite == 245 && my->flags[SPRITE]) )
	{
		// grease droplet
		my->vel_z += 0.04;
		my->scalex -= std::max(0.01, my->scalex * 0.05);
		my->scaley -= std::max(0.01, my->scaley * 0.05);
		my->scalez -= std::max(0.01, my->scalez * 0.05);
		if ( my->z >= 8.0 )
		{
			list_RemoveNode(my->mynode);
			return;
		}
	}
	else if ( my->sprite == 1479 )
	{
		my->scalex -= 0.025;
		my->scaley -= 0.025;
		my->scalez -= 0.025;
		my->pitch += 0.25;
		my->yaw += 0.25;
		//if ( my->parent == 0 && local_rng.rand() % 10 == 0 )
		//{
		//	if ( Entity* particle = spawnMagicParticle(my) )
		//	{
		//		particle->parent = my->getUID();
		//		particle->x = my->x;
		//		particle->y = my->y;
		//		//particle->z = my->z;
		//	}
		//}
	}
	else if ( my->sprite == 1866 || my->sprite == 2374 )
	{
		my->scalex -= 0.01;
		my->scaley -= 0.01;
		my->scalez -= 0.01;
	}
	else if ( my->sprite == 2191 )
	{
		my->scalex -= 0.025;
		my->scaley -= 0.025;
		my->scalez -= 0.025;
	}
	else if ( my->sprite == 2364 || my->sprite == 2365 || my->sprite == 2366 )
	{
		my->scalex -= 0.05;
		my->scaley -= 0.05;
		my->scalez -= 0.05;

		my->pitch += 0.25;
		//my->vel_z += 0.04;
	}
	else if ( my->sprite == 2406 || my->sprite == 2407 )
	{
		my->scalex -= 0.025;
		my->scaley -= 0.025;
		my->scalez -= 0.025;
		if ( my->sprite == 2406 )
		{
			my->yaw += 0.25;
			my->vel_x *= 0.9;
			my->vel_y *= 0.9;
			my->vel_z *= 0.9;
		}
	}
	else if ( my->sprite == 2209 )
	{
		my->scalex -= 0.025;
		my->scaley -= 0.025;
		my->scalez -= 0.025;
	}
	else if ( my->sprite == 2410 )
	{
		my->scalex -= 0.025;
		my->scaley -= 0.025;
		my->scalez -= 0.025;
		my->roll += 0.2;

		my->z += my->fskill[0];
	}
	else if ( my->sprite == 2363 )
	{
		my->scalex -= 0.05 * (1.0 - my->fskill[1]);
		my->scaley -= 0.05 * (1.0 - my->fskill[1]);
		my->scalez -= 0.05 * (1.0 - my->fskill[1]);
	}
	else if ( my->sprite == 262 && my->flags[SPRITE] )
	{
		my->scalex -= 0.025;
		my->scaley -= 0.025;
		my->scalez -= 0.025;
	}
	else if ( my->sprite == 2192 || my->sprite == 2193 || my->sprite == 2210 || my->sprite == 2211 )
	{
		my->scalex -= 0.025;
		my->scaley -= 0.025;
		my->scalez -= 0.025;
		my->pitch += 0.1;
	}
	else if ( my->sprite >= 2152 && my->sprite <= 2157 )
	{
		my->scalex -= std::max(0.01, my->fskill[1]);
		my->scaley -= std::max(0.01, my->fskill[1]);
		my->scalez -= std::max(0.01, my->fskill[1]);
		my->vel_z += my->fskill[0];

		if ( (my->sprite == 2153 || my->sprite == 2155) && ticks % 4 == 0 )
		{
			my->roll = (local_rng.rand() % 8) * PI / 4;
		}
	}
	else if ( my->sprite >= PINPOINT_PARTICLE_START && my->sprite < PINPOINT_PARTICLE_END )
	{
		my->scalex -= std::max(0.01, my->fskill[1]);
		my->scaley -= std::max(0.01, my->fskill[1]);
		my->scalez -= std::max(0.01, my->fskill[1]);
	}
	else if ( my->sprite >= 233 && my->sprite <= 244 && my->flags[SPRITE] )
	{
		if ( my->skill[0] > 0 )
		{
			--my->skill[0];
		}
		else
		{
			my->scalex -= std::max(0.01, my->fskill[1]);
			my->scaley -= std::max(0.01, my->fskill[1]);
			my->scalez -= std::max(0.01, my->fskill[1]);
		}
		my->vel_z += my->fskill[0];
		if ( my->ticks % 4 == 0 )
		{
			if ( my->sprite < 244 )
			{
				++my->sprite;
			}
		}
	}
	else if ( my->sprite >= 263 && my->sprite <= 274 && my->flags[SPRITE] )
	{
		if ( my->skill[0] > 0 )
		{
			--my->skill[0];
		}
		else
		{
			my->scalex -= std::max(0.01, my->fskill[1]);
			my->scaley -= std::max(0.01, my->fskill[1]);
			my->scalez -= std::max(0.01, my->fskill[1]);
		}
		my->vel_z += my->fskill[0];
		if ( my->ticks % 4 == 0 )
		{
			if ( my->sprite < 274 )
			{
				++my->sprite;
			}
		}
	}
	else if ( my->sprite >= 288 && my->sprite <= 299 && my->flags[SPRITE] )
	{
		if ( my->skill[0] > 0 )
		{
			--my->skill[0];
		}
		else
		{
			my->scalex -= std::max(0.01, my->fskill[1]);
			my->scaley -= std::max(0.01, my->fskill[1]);
			my->scalez -= std::max(0.01, my->fskill[1]);
		}
		my->vel_z += my->fskill[0];
		if ( my->ticks % 4 == 0 )
		{
			if ( my->sprite < 299 )
			{
				++my->sprite;
			}
		}
	}
	else
	{
		my->scalex -= 0.05;
		my->scaley -= 0.05;
		my->scalez -= 0.05;
	}
	if ( my->scalex <= 0 )
	{
		my->scalex = 0;
		my->scaley = 0;
		my->scalez = 0;
		list_RemoveNode(my->mynode);
		return;
	}
}

void actTouchCastThirdPersonParticle(Entity* my)
{
	my->x += my->vel_x;
	my->y += my->vel_y;
	my->z += my->vel_z;
	my->scalex -= 0.05;
	my->scaley -= 0.05;
	my->scalez -= 0.05;
	if ( my->scalex <= 0 )
	{
		my->scalex = 0;
		my->scaley = 0;
		my->scalez = 0;
		list_RemoveNode(my->mynode);
		return;
	}
}

void actHUDMagicParticle(Entity* my)
{
	my->x += my->vel_x;
	my->y += my->vel_y;
	my->z += my->vel_z;
	my->scalex -= 0.05;
	my->scaley -= 0.05;
	my->scalez -= 0.05;
	if ( my->scalex <= 0 )
	{
		my->scalex = 0;
		my->scaley = 0;
		my->scalez = 0;
		list_RemoveNode(my->mynode);
		return;
	}
}

void createEnsembleTargetParticleCircling(Entity* parent)
{
	if ( !parent ) { return; }

	// world particle
	Entity* entity = newEntity(198, 1, map.entities, nullptr);
	entity->yaw = (local_rng.rand() % 3) * 2 * PI / 3;
	entity->x = parent->x;
	entity->y = parent->y;
	entity->z = 0.0;
	double missile_speed = 4;
	entity->vel_x = 0.0;
	entity->vel_y = 0.0;
	entity->actmagicIsOrbiting = 2;
	entity->actmagicOrbitDist = 4.0;
	entity->actmagicOrbitStationaryCurrentDist = 0.0;
	entity->actmagicOrbitStartZ = entity->z;
	entity->actmagicOrbitVerticalSpeed = -0.025;
	entity->actmagicOrbitVerticalDirection = 1;
	entity->actmagicOrbitLifetime = TICKS_PER_SECOND;
	entity->actmagicOrbitStationaryX = entity->x;
	entity->actmagicOrbitStationaryY = entity->y;
	entity->vel_z = -0.1;
	entity->scalex = 0.3;
	entity->scaley = 0.3;
	entity->scalez = 0.3;
	entity->behavior = &actMagicParticleEnsembleCircling;

	entity->flags[BRIGHT] = true;
	entity->flags[SPRITE] = true;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->flags[UPDATENEEDED] = false;
	//entity->flags[OVERDRAW] = true;
	entity->skill[11] = parent->behavior == &actPlayer ? parent->skill[2] : -1;
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);
}

void createEnsembleHUDParticleCircling(Entity* parent)
{
	if ( !parent ) { return; }

	if ( (parent->behavior == &actPlayer && players[parent->skill[2]]->isLocalPlayer() && parent->skill[3] == 0)
		|| parent->behavior == &actDeathGhost )
	{
		// create overdraw HUD particle
		Entity* entity = newEntity(198, 1, map.entities, nullptr);
		float x = 6 * 10;
		float y = 0.1;
		float z = 7;
		entity->yaw = (local_rng.rand() % 3) * 2 * PI / 3;
		entity->x = x;
		entity->y = y;
		entity->z = z;
		double missile_speed = 4;
		entity->vel_x = 0.0;
		entity->vel_y = 0.0;
		entity->actmagicIsOrbiting = 2;
		entity->actmagicOrbitDist = 16.0;
		entity->actmagicOrbitStationaryCurrentDist = 0.0;
		entity->actmagicOrbitStartZ = entity->z;
		entity->actmagicOrbitVerticalSpeed = -0.3;
		entity->actmagicOrbitVerticalDirection = 1;
		entity->actmagicOrbitLifetime = TICKS_PER_SECOND;
		entity->actmagicOrbitStationaryX = x;
		entity->actmagicOrbitStationaryY = y;
		entity->vel_z = -0.1;
		entity->behavior = &actHUDMagicParticleCircling;

		entity->flags[BRIGHT] = true;
		entity->flags[SPRITE] = true;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[UNCLICKABLE] = true;
		entity->flags[UPDATENEEDED] = false;
		entity->flags[OVERDRAW] = true;
		entity->skill[11] = (parent->behavior == &actPlayer || parent->behavior == &actDeathGhost) ? parent->skill[2] : -1;
		if ( multiplayer != CLIENT )
		{
			entity_uids--;
		}
		entity->setUID(-3);
	}
}

void actMagicParticleEnsembleCircling(Entity* my)
{
	real_t turnRate = 0.25;
	my->yaw += 0.2;
	my->x = my->actmagicOrbitStationaryX + my->actmagicOrbitStationaryCurrentDist * cos(my->yaw);
	my->y = my->actmagicOrbitStationaryY + my->actmagicOrbitStationaryCurrentDist * sin(my->yaw);
	my->actmagicOrbitStationaryCurrentDist =
		std::min(my->actmagicOrbitStationaryCurrentDist + 0.5, static_cast<real_t>(my->actmagicOrbitDist));
	my->z += my->vel_z * my->actmagicOrbitVerticalDirection;

	my->vel_z = std::min(my->actmagicOrbitVerticalSpeed, my->vel_z / 0.95);
	my->roll += (PI / 8) / (turnRate / my->vel_z) * my->actmagicOrbitVerticalDirection;
	my->roll = std::max(my->roll, -PI / 4);

	--my->actmagicOrbitLifetime;
	if ( my->actmagicOrbitLifetime <= 0 )
	{
		list_RemoveNode(my->mynode);
		return;
	}
}

void actHUDMagicParticleCircling(Entity* my)
{
	int turnRate = 4;
	my->yaw += 0.2;
	turnRate = 4;
	my->x = my->actmagicOrbitStationaryX + my->actmagicOrbitStationaryCurrentDist * cos(my->yaw);
	my->y = my->actmagicOrbitStationaryY + my->actmagicOrbitStationaryCurrentDist * sin(my->yaw);
	my->actmagicOrbitStationaryCurrentDist =
		std::min(my->actmagicOrbitStationaryCurrentDist + 0.5, static_cast<real_t>(my->actmagicOrbitDist));
	my->z += my->vel_z * my->actmagicOrbitVerticalDirection;

	my->vel_z = std::min(my->actmagicOrbitVerticalSpeed, my->vel_z / 0.95);
	my->roll += (PI / 8) / (turnRate / my->vel_z) * my->actmagicOrbitVerticalDirection;
	my->roll = std::max(my->roll, -PI / 4);

	--my->actmagicOrbitLifetime;
	if ( my->actmagicOrbitLifetime <= 0 )
	{
		list_RemoveNode(my->mynode);
		return;
	}

	if ( !my->flags[SPRITE] )
	{
		Entity* entity;

		entity = newEntity(my->sprite, 1, map.entities, nullptr); //Particle entity.

		entity->x = my->x + (local_rng.rand() % 50 - 25) / 200.f;
		entity->y = my->y + (local_rng.rand() % 50 - 25) / 200.f;
		entity->z = my->z + (local_rng.rand() % 50 - 25) / 200.f;
		entity->scalex = 0.7;
		entity->scaley = 0.7;
		entity->scalez = 0.7;
		entity->sizex = 1;
		entity->sizey = 1;
		entity->yaw = my->yaw;
		entity->pitch = my->pitch;
		entity->roll = my->roll;
		entity->flags[NOUPDATE] = true;
		entity->flags[PASSABLE] = true;
		entity->flags[UNCLICKABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[UPDATENEEDED] = false;
		entity->flags[OVERDRAW] = true;
		entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
			*cvar_magic_fx_light_bonus, 0.f);
		entity->behavior = &actHUDMagicParticle;
		entity->skill[11] = my->skill[11];
		if ( multiplayer != CLIENT )
		{
			entity_uids--;
		}
		entity->setUID(-3);
	}
}

void actMagicParticleCircling2(Entity* my)
{
	int turnRate = 4;
	my->yaw += 0.2;
	turnRate = 4;
	my->x = my->actmagicOrbitStationaryX + my->actmagicOrbitStationaryCurrentDist * cos(my->yaw);
	my->y = my->actmagicOrbitStationaryY + my->actmagicOrbitStationaryCurrentDist * sin(my->yaw);
	my->actmagicOrbitStationaryCurrentDist =
		std::min(my->actmagicOrbitStationaryCurrentDist + 0.5, static_cast<real_t>(my->actmagicOrbitDist));
	my->z += my->vel_z * my->actmagicOrbitVerticalDirection;

	my->vel_z = std::min(my->actmagicOrbitVerticalSpeed, my->vel_z / 0.95);
	my->roll += (PI / 8) / (turnRate / my->vel_z) * my->actmagicOrbitVerticalDirection;
	my->roll = std::max(my->roll, -PI / 4);

	--my->actmagicOrbitLifetime;
	if ( my->actmagicOrbitLifetime <= 0 )
	{
		list_RemoveNode(my->mynode);
		return;
	}

	if ( !my->flags[SPRITE] )
	{
		Entity* entity;

		entity = newEntity(my->sprite, 1, map.entities, nullptr); //Particle entity.

		entity->x = my->x + (local_rng.rand() % 50 - 25) / 200.f;
		entity->y = my->y + (local_rng.rand() % 50 - 25) / 200.f;
		entity->z = my->z + (local_rng.rand() % 50 - 25) / 200.f;
		entity->scalex = 0.7;
		entity->scaley = 0.7;
		entity->scalez = 0.7;
		entity->sizex = 1;
		entity->sizey = 1;
		entity->yaw = my->yaw;
		entity->pitch = my->pitch;
		entity->roll = my->roll;
		entity->flags[NOUPDATE] = true;
		entity->flags[PASSABLE] = true;
		entity->flags[UNCLICKABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[UPDATENEEDED] = false;
		entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
			*cvar_magic_fx_light_bonus, 0.f);
		entity->behavior = &actHUDMagicParticle;
		if ( multiplayer != CLIENT )
		{
			entity_uids--;
		}
		entity->setUID(-3);
	}
}

Entity* spawnMagicParticle(Entity* parentent)
{
	if ( !parentent )
	{
		return nullptr;
	}
	Entity* entity;

	entity = newEntity(parentent->sprite, 1, map.entities, nullptr); //Particle entity.

	entity->x = parentent->x + (local_rng.rand() % 50 - 25) / 20.f;
	entity->y = parentent->y + (local_rng.rand() % 50 - 25) / 20.f;
	entity->z = parentent->z + (local_rng.rand() % 50 - 25) / 20.f;
	entity->parent = 0;
	entity->scalex = 0.7;
	entity->scaley = 0.7;
	entity->scalez = 0.7;
	entity->sizex = 1;
	entity->sizey = 1;
	entity->yaw = parentent->yaw;
	entity->pitch = parentent->pitch;
	entity->roll = parentent->roll;
	entity->flags[NOUPDATE] = true;
	entity->flags[PASSABLE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UPDATENEEDED] = false;
	entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
		*cvar_magic_fx_light_bonus, 0.f);
	entity->behavior = &actMagicParticle;
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);

	return entity;
}

Entity* spawnMagicParticleCustom(Entity* parentent, int sprite, real_t scale, real_t spreadReduce)
{
	if ( !parentent )
	{
		return nullptr;
	}
	Entity* entity;

	entity = newEntity(sprite, 1, map.entities, nullptr); //Particle entity.

	int size = 50 / spreadReduce;
	entity->x = parentent->x + (local_rng.rand() % size - size / 2) / 20.f;
	entity->y = parentent->y + (local_rng.rand() % size - size / 2) / 20.f;
	entity->z = parentent->z + (local_rng.rand() % size - size / 2) / 20.f;
	entity->scalex = scale;
	entity->scaley = scale;
	entity->scalez = scale;
	entity->sizex = 1;
	entity->sizey = 1;
	entity->yaw = parentent->yaw;
	entity->pitch = parentent->pitch;
	entity->roll = parentent->roll;
	entity->flags[NOUPDATE] = true;
	entity->flags[PASSABLE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UPDATENEEDED] = false;
	entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
		*cvar_magic_fx_light_bonus, 0.f);
	entity->behavior = &actMagicParticle;
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);

	return entity;
}

void spawnMagicEffectParticles(Sint16 x, Sint16 y, Sint16 z, Uint32 sprite)
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
			strcpy((char*)net_packet->data, "MAGE");
			SDLNet_Write16(x, &net_packet->data[4]);
			SDLNet_Write16(y, &net_packet->data[6]);
			SDLNet_Write16(z, &net_packet->data[8]);
			SDLNet_Write32(sprite, &net_packet->data[10]);
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 14;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}

	// boosty boost
	for ( c = 0; c < 10; c++ )
	{
		Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Particle entity.
		entity->x = x - 5 + local_rng.rand() % 11;
		entity->y = y - 5 + local_rng.rand() % 11;
		entity->z = z - 10 + local_rng.rand() % 21;
		entity->scalex = 0.7;
		entity->scaley = 0.7;
		entity->scalez = 0.7;
		entity->sizex = 1;
		entity->sizey = 1;
		entity->yaw = (local_rng.rand() % 360) * PI / 180.f;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[UNCLICKABLE] = true;
		entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus, 
			*cvar_magic_fx_light_bonus, 0.f);
		entity->behavior = &actMagicParticle;
		entity->vel_z = -1;
		if ( multiplayer != CLIENT )
		{
			entity_uids--;
		}
		entity->setUID(-3);
	}
}

void createParticleCircling(Entity* parent, int duration, int sprite)
{
	if ( !parent )
	{
		return;
	}

	Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Particle entity.
	entity->sizex = 1;
	entity->sizey = 1;
	entity->x = parent->x;
	entity->y = parent->y;
	entity->focalx = 8;
	entity->z = -7;
	entity->vel_z = 0.15;
	entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
	entity->skill[0] = duration;
	entity->skill[1] = -1;
	//entity->scalex = 0.01;
	//entity->scaley = 0.01;
	entity->fskill[0] = -0.1;
	entity->behavior = &actParticleCircle;
	entity->flags[PASSABLE] = true;
	entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
		*cvar_magic_fx_light_bonus, 0.f);
	entity->setUID(-3);

	real_t tmp = entity->yaw;

	entity = newEntity(sprite, 1, map.entities, nullptr); //Particle entity.
	entity->sizex = 1;
	entity->sizey = 1;
	entity->x = parent->x;
	entity->y = parent->y;
	entity->focalx = 8;
	entity->z = -7;
	entity->vel_z = 0.15;
	entity->yaw = tmp + (2 * PI / 3);
	entity->particleDuration = duration;
	entity->skill[1] = -1;
	//entity->scalex = 0.01;
	//entity->scaley = 0.01;
	entity->fskill[0] = -0.1;
	entity->behavior = &actParticleCircle;
	entity->flags[PASSABLE] = true;
	entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
		*cvar_magic_fx_light_bonus, 0.f);
	entity->setUID(-3);

	entity = newEntity(sprite, 1, map.entities, nullptr); //Particle entity.
	entity->sizex = 1;
	entity->sizey = 1;
	entity->x = parent->x;
	entity->y = parent->y;
	entity->focalx = 8;
	entity->z = -7;
	entity->vel_z = 0.15;
	entity->yaw = tmp - (2 * PI / 3);
	entity->particleDuration = duration;
	entity->skill[1] = -1;
	//entity->scalex = 0.01;
	//entity->scaley = 0.01;
	entity->fskill[0] = -0.1;
	entity->behavior = &actParticleCircle;
	entity->flags[PASSABLE] = true;
	entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
		*cvar_magic_fx_light_bonus, 0.f);
	entity->setUID(-3);

	entity = newEntity(sprite, 1, map.entities, nullptr); //Particle entity.
	entity->sizex = 1;
	entity->sizey = 1;
	entity->x = parent->x;
	entity->y = parent->y;
	entity->focalx = 16;
	entity->z = -12;
	entity->vel_z = 0.2;
	entity->yaw = tmp;
	entity->particleDuration = duration;
	entity->skill[1] = -1;
	//entity->scalex = 0.01;
	//entity->scaley = 0.01;
	entity->fskill[0] = 0.1;
	entity->behavior = &actParticleCircle;
	entity->flags[PASSABLE] = true;
	entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
		*cvar_magic_fx_light_bonus, 0.f);
	entity->setUID(-3);

	entity = newEntity(sprite, 1, map.entities, nullptr); //Particle entity.
	entity->sizex = 1;
	entity->sizey = 1;
	entity->x = parent->x;
	entity->y = parent->y;
	entity->focalx = 16;
	entity->z = -12;
	entity->vel_z = 0.2;
	entity->yaw = tmp + (2 * PI / 3);
	entity->particleDuration = duration;
	entity->skill[1] = -1;
	//entity->scalex = 0.01;
	//entity->scaley = 0.01;
	entity->fskill[0] = 0.1;
	entity->behavior = &actParticleCircle;
	entity->flags[PASSABLE] = true;
	entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
		*cvar_magic_fx_light_bonus, 0.f);
	entity->setUID(-3);

	entity = newEntity(sprite, 1, map.entities, nullptr); //Particle entity.
	entity->sizex = 1;
	entity->sizey = 1;
	entity->x = parent->x;
	entity->y = parent->y;
	entity->focalx = 16;
	entity->z = -12;
	entity->vel_z = 0.2;
	entity->yaw = tmp - (2 * PI / 3);
	entity->particleDuration = duration;
	entity->skill[1] = -1;
	//entity->scalex = 0.01;
	//entity->scaley = 0.01;
	entity->fskill[0] = 0.1;
	entity->behavior = &actParticleCircle;
	entity->flags[PASSABLE] = true;
	entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
		*cvar_magic_fx_light_bonus, 0.f);
	entity->setUID(-3);
}

#define PARTICLE_LIFE my->skill[0]

void actParticleCircle(Entity* my)
{
	if ( PARTICLE_LIFE < 0 )
	{
		list_RemoveNode(my->mynode);
		return;
	}
	else
	{
		--PARTICLE_LIFE;
		my->yaw += my->fskill[0];
		if ( my->fskill[0] < 0.4 && my->fskill[0] > (-0.4) )
		{
			my->fskill[0] = my->fskill[0] * 1.05;
		}
		my->z += my->vel_z;
		if ( my->focalx > 0.05 )
		{
			if ( my->vel_z == 0.15 )
			{
				my->focalx = my->focalx * 0.97;
			}
			else
			{
				my->focalx = my->focalx * 0.97;
			}
		}
		my->scalex *= 0.995;
		my->scaley *= 0.995;
		my->scalez *= 0.995;
	}
}

void createParticleDot(Entity* parent)
{
	if ( !parent )
	{
		return;
	}
	for ( int c = 0; c < 50; c++ )
	{
		Entity* entity = newEntity(576, 1, map.entities, nullptr); //Particle entity.
		entity->sizex = 1;
		entity->sizey = 1;
		entity->x = parent->x + (-4 + local_rng.rand() % 9);
		entity->y = parent->y + (-4 + local_rng.rand() % 9);
		entity->z = 7.5 + local_rng.rand()%50;
		entity->vel_z = -1;
		//entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
		entity->skill[0] = 10 + local_rng.rand()% 50;
		entity->behavior = &actParticleDot;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[UNCLICKABLE] = true;
		entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
			*cvar_magic_fx_light_bonus, 0.f);
		if ( multiplayer != CLIENT )
		{
			entity_uids--;
		}
		entity->setUID(-3);
	}
}

Entity* createParticleBolas(Entity* parent, int sprite, int duration, Item* item)
{
	if ( !parent ) { return nullptr; }
	Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Particle entity.
	entity->sizex = 1;
	entity->sizey = 1;
	entity->parent = parent->getUID();
	entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
	entity->skill[0] = duration;
	entity->behavior = &actParticleBolas;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
		*cvar_magic_fx_light_bonus, 0.f);
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);

	if ( multiplayer != CLIENT && item )
	{
		entity->skill[10] = item->type;
		entity->skill[11] = item->status;
		entity->skill[12] = item->beatitude;
		entity->skill[13] = item->count;
		entity->skill[14] = item->appearance;
		entity->skill[15] = item->identified;
		entity->skill[16] = item->ownerUid;
	}
	return entity;
}

Entity* createParticleAestheticOrbit(Entity* parent, int sprite, int duration, int effectType)
{
	if ( effectType == PARTICLE_EFFECT_NULL_PARTICLE
		|| effectType == PARTICLE_EFFECT_IGNITE_ORBIT
		|| effectType == PARTICLE_EFFECT_IGNITE_ORBIT_LOOP
		|| effectType == PARTICLE_EFFECT_METEOR_STATIONARY_ORBIT
		|| effectType == PARTICLE_EFFECT_BLOOD_BUBBLE )
	{
		// no need parent
	}
	else
	{
		if ( !parent )
		{
			return nullptr;
		}
	}
	Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Particle entity.
	entity->sizex = 1;
	entity->sizey = 1;
	entity->actmagicOrbitDist = 6;
	entity->yaw = parent ? parent->yaw : 0.0;
	entity->x = parent ? parent->x + entity->actmagicOrbitDist * cos(entity->yaw) : 0.0;
	entity->y = parent ? parent->y + entity->actmagicOrbitDist * sin(entity->yaw) : 0.0;
	entity->z = parent ? parent->z : 0.0;
	entity->skill[1] = effectType;
	entity->parent = parent ? parent->getUID() : 0;
	//entity->vel_z = -1;
	//entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
	entity->skill[0] = duration;
	entity->fskill[0] = entity->x;
	entity->fskill[1] = entity->y;
	entity->behavior = &actParticleAestheticOrbit;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
		*cvar_magic_fx_light_bonus, 0.f);
	if ( effectType == PARTICLE_EFFECT_DEFY_FLESH_ORBIT || effectType == PARTICLE_EFFECT_SMITE_PINPOINT )
	{
		TileEntityList.addEntity(*entity);
	}
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);
	return entity;
}

void createParticleRock(Entity* parent, int sprite, bool light)
{
	if ( !parent )
	{
		return;
	}
	for ( int c = 0; c < 5; c++ )
	{
		Entity* entity = newEntity(sprite != -1 ? sprite : 78, 1, map.entities, nullptr); //Particle entity.
		if ( entity->sprite == 1336 )
		{
			entity->sprite = 1336 + local_rng.rand() % 3;
		}
		entity->sizex = 1;
		entity->sizey = 1;
		entity->x = parent->x + (-4 + local_rng.rand() % 9);
		entity->y = parent->y + (-4 + local_rng.rand() % 9);
		entity->z = 7.5;
		entity->yaw = c * 2 * PI / 5;//(local_rng.rand() % 360) * PI / 180.0;
		entity->roll = (local_rng.rand() % 360) * PI / 180.0;

		entity->vel_x = 0.2 * cos(entity->yaw);
		entity->vel_y = 0.2 * sin(entity->yaw);
		entity->vel_z = 3;// 0.25 - (local_rng.rand() % 5) / 10.0;

		entity->skill[0] = 50; // particle life
		entity->skill[1] = 0; // particle direction, 0 = upwards, 1 = downwards.

		entity->behavior = &actParticleRock;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[UNCLICKABLE] = true;
		if ( !light )
		{
			entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
				*cvar_magic_fx_light_bonus, 0.f);
		}
		if ( multiplayer != CLIENT )
		{
			entity_uids--;
		}
		entity->setUID(-3);
	}
}

void createParticleShatteredGem(real_t x, real_t y, real_t z, int sprite, Entity* parent)
{
	for ( int c = 0; c < 5; c++ )
	{
		Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Particle entity.
		entity->sizex = 1;
		entity->sizey = 1;
		if ( parent )
		{
			entity->x = parent->x + (-4 + local_rng.rand() % 9);
			entity->y = parent->y + (-4 + local_rng.rand() % 9);
		}
		else
		{
			entity->x = x + (-4 + local_rng.rand() % 9);
			entity->y = y + (-4 + local_rng.rand() % 9);
		}
		entity->z = z;
		entity->yaw = c * 2 * PI / 5;//(local_rng.rand() % 360) * PI / 180.0;
		entity->roll = (local_rng.rand() % 360) * PI / 180.0;

		entity->vel_x = 0.2 * cos(entity->yaw);
		entity->vel_y = 0.2 * sin(entity->yaw);
		entity->vel_z = 3;// 0.25 - (local_rng.rand() % 5) / 10.0;

		real_t scale = .4;
		entity->scalex = scale;
		entity->scaley = scale;
		entity->scalez = scale;

		entity->skill[0] = 50; // particle life
		entity->skill[1] = 0; // particle direction, 0 = upwards, 1 = downwards.

		entity->behavior = &actParticleRock;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[UNCLICKABLE] = true;
		entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
			*cvar_magic_fx_light_bonus, 0.f);
		if ( multiplayer != CLIENT )
		{
			entity_uids--;
		}
		entity->setUID(-3);
	}
}

void actParticleRock(Entity* my)
{
	if ( PARTICLE_LIFE < 0 || my->z > 10 )
	{
		list_RemoveNode(my->mynode);
	}
	else
	{
		--PARTICLE_LIFE;
		my->x += my->vel_x;
		my->y += my->vel_y;

		my->roll += 0.1;

		if ( my->vel_z < 0.01 )
		{
			my->skill[1] = 1; // start moving downwards
			my->vel_z = 0.1;
		}

		if ( my->skill[1] == 0 ) // upwards motion
		{
			my->z -= my->vel_z;
			my->vel_z *= 0.7;
		}
		else // downwards motion
		{
			my->z += my->vel_z;
			my->vel_z *= 1.1;
		}
	}
	return;
}

void actParticleDot(Entity* my)
{
	if ( PARTICLE_LIFE < 0 )
	{
		list_RemoveNode(my->mynode);
	}
	else
	{
		--PARTICLE_LIFE;
		my->z += my->vel_z;
		//my->z -= 0.01;
	}
	return;
}

void actParticleBolas(Entity* my)
{
	Entity* parent = uidToEntity(my->parent);
	bool destroy = false;
	if ( !parent )
	{
		destroy = true;
	}
	if ( PARTICLE_LIFE < 0 )
	{
		destroy = true;
	}

	if ( parent )
	{
		my->x = parent->x;
		my->y = parent->y;
		my->z = parent->z + 1.0;
	}

	if ( my->skill[1] == 0 )
	{
		my->skill[1] = 1;
		my->fskill[1] = 1.0;
	}

	my->z = std::min(6.0, my->z);
	my->fskill[0] += 0.25;
	my->yaw += 0.25 * sin(my->fskill[1]);
	my->fskill[1] *= 0.9;
	my->scalex = 1.1 + 0.1 * sin(my->fskill[0]);
	my->scaley = 1.1 + 0.1 * sin(my->fskill[0]);
	my->scalez = 1.1 - 0.1 * sin(my->fskill[0]);

	if ( destroy )
	{
		if ( multiplayer != CLIENT && my->skill[10] > 0 && my->skill[12] >= 0 ) // not cursed
		{
			Entity* entity = newEntity(-1, 1, map.entities, nullptr); //Item entity.
			entity->flags[INVISIBLE] = true;
			entity->flags[UPDATENEEDED] = true;
			entity->flags[PASSABLE] = true;
			entity->x = my->x;
			entity->y = my->y;
			entity->z = my->z;
			entity->sizex = 2;
			entity->sizey = 2;
			entity->yaw = my->yaw;
			entity->pitch = my->pitch;
			entity->roll = my->roll;
			entity->vel_x = 0.25;
			entity->vel_y = 0.25;
			entity->vel_z = -0.5;
			entity->behavior = &actItem;
			entity->skill[10] = my->skill[10];
			entity->skill[11] = my->skill[11];
			entity->skill[12] = my->skill[12];
			entity->skill[13] = my->skill[13];
			entity->skill[14] = my->skill[14];
			entity->skill[15] = my->skill[15];
			entity->parent = my->skill[16]; // owner UID
			if ( Entity* owner = uidToEntity(entity->parent) )
			{
				if ( Stat* stats = owner->getStats() )
				{
					if ( stats->getEffectActive(EFF_RETURN_ITEM) )
					{
						entity->itemReturnUID = owner->getUID();
					}
				}
			}
		}

		list_RemoveNode(my->mynode);
		return;
	}

	--PARTICLE_LIFE;
}

Uint32 nullParticleSfxTick = 0;
void actParticleAestheticOrbit(Entity* my)
{
	if ( PARTICLE_LIFE < 0 )
	{
		my->removeLightField();
		list_RemoveNode(my->mynode);
	}
	else
	{
		Entity* parent = uidToEntity(my->parent);
		if ( !parent )
		{
			if ( my->skill[1] == PARTICLE_EFFECT_NULL_PARTICLE 
				|| my->skill[1] == PARTICLE_EFFECT_NULL_PARTICLE_NOSOUND
				|| my->skill[1] == PARTICLE_EFFECT_SHATTER_EARTH_ORBIT
				|| my->skill[1] == PARTICLE_EFFECT_IGNITE_ORBIT
				|| my->skill[1] == PARTICLE_EFFECT_IGNITE_ORBIT_LOOP
				|| my->skill[1] == PARTICLE_EFFECT_METEOR_STATIONARY_ORBIT
				|| my->skill[1] == PARTICLE_EFFECT_BLOOD_BUBBLE
				|| my->skill[1] == PARTICLE_EFFECT_SMITE_PINPOINT 
				|| my->skill[1] == PARTICLE_EFFECT_FOCI_LIGHT
				|| my->skill[1] == PARTICLE_EFFECT_FOCI_DARK )
			{
				// no need for parent
			}
			else
			{
				my->removeLightField();
				list_RemoveNode(my->mynode);
				return;
			}
		}
		if ( my->skill[1] == PARTICLE_EFFECT_SPELLBOT_ORBIT )
		{
			my->yaw = parent->yaw;
			my->x = parent->x + 2 * cos(parent->yaw);
			my->y = parent->y + 2 * sin(parent->yaw);
			my->z = parent->z - 1.5;
			Entity* particle = spawnMagicParticle(my);
			if ( particle )
			{
				particle->x = my->x + (-10 + local_rng.rand() % 21) / (50.f);
				particle->y = my->y + (-10 + local_rng.rand() % 21) / (50.f);
				particle->z = my->z + (-10 + local_rng.rand() % 21) / (50.f);
				particle->scalex = my->scalex;
				particle->scaley = my->scaley;
				particle->scalez = my->scalez;
			}
			//spawnMagicParticle(my);
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_SPELL_WEB_ORBIT )
		{
			Stat* stats = parent->getStats();
			if ( my->sprite == 863 && (!stats || !stats->getEffectActive(EFF_WEBBED)) )
			{
				my->removeLightField();
				list_RemoveNode(my->mynode);
				return;
			}
			my->yaw += 0.2;
			spawnMagicParticle(my);
			my->x = parent->x + my->actmagicOrbitDist * cos(my->yaw);
			my->y = parent->y + my->actmagicOrbitDist * sin(my->yaw);
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_HOLY_FIRE )
		{
			Stat* stats = parent->getStats();
			if ( stats && !stats->getEffectActive(EFF_HOLY_FIRE) )
			{
				my->removeLightField();
				list_RemoveNode(my->mynode);
				return;
			}

			my->flags[INVISIBLE] = true;
			my->flags[SPRITE] = true;
			my->removeLightField();
			my->light = addLight(my->x / 16, my->y / 16, "blue_fire");

			/*if ( my->ticks % 10 == 0 )
			{
			}*/
			if ( my->ticks % 40 == 0 )
			{
				if ( Entity* fx = spawnFlameSprites(parent, 288) )
				{
				}
				if ( multiplayer != CLIENT )
				{
					Entity* caster = uidToEntity(my->skill[3]);
					int burnDamage = getSpellDamageFromID(SPELL_HOLY_FIRE, caster, caster ? caster->getStats() : nullptr,
						caster);
					applyGenericMagicDamage(caster, parent, caster ? *caster : *my, SPELL_HOLY_FIRE, burnDamage, true, true);
				}
			}
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_IGNITE_ORBIT || my->skill[1] == PARTICLE_EFFECT_IGNITE_ORBIT_LOOP
			|| my->skill[1] == PARTICLE_EFFECT_IGNITE_ORBIT_FOLLOW
			|| my->skill[1] == PARTICLE_EFFECT_RADIANT_ORBIT_FOLLOW
			|| my->skill[1] == PARTICLE_EFFECT_FLAMES_BURNING
			|| my->skill[1] == PARTICLE_EFFECT_HEAT_ORBIT_SPIN )
		{
			//my->fskill[2] += 0.01;
			my->yaw = my->fskill[2];
			my->yaw += (PI / 32) * sin(my->fskill[3]);
			if ( my->skill[1] == PARTICLE_EFFECT_RADIANT_ORBIT_FOLLOW )
			{
				my->fskill[3] += my->fskill[4];
			}
			else if ( my->skill[1] == PARTICLE_EFFECT_HEAT_ORBIT_SPIN )
			{
				my->yaw = my->fskill[2];
				my->fskill[2] += my->fskill[4];
			}
			else
			{
				my->fskill[3] += 0.1;
			}

			if ( my->skill[1] == PARTICLE_EFFECT_FLAMES_BURNING )
			{
				if ( parent )
				{
					my->x = parent->x + my->actmagicOrbitDist * cos(my->yaw);
					my->y = parent->y + my->actmagicOrbitDist * sin(my->yaw);
					my->z = parent->z;
					my->z += local_rng.rand() % 7 - 4;
					int sizex = parent->sizex;
					int sizey = parent->sizey;
					if ( parent->behavior == &actBell )
					{
						my->x += parent->focalx * cos(parent->yaw) + parent->focaly * cos(parent->yaw + PI / 2);
						my->y += parent->focalx * sin(parent->yaw) + parent->focaly * sin(parent->yaw + PI / 2);
					}
					else if ( parent->behavior == &actDoor )
					{
						if ( abs(parent->focaly) > 0 )
						{
							sizex = parent->sizey;
							sizey = parent->sizex;
							my->x += parent->focaly * cos(parent->yaw + PI / 2);
							my->y += parent->focaly * sin(parent->yaw + PI / 2);
						}
					}
					int sizeModX = local_rng.rand() % (sizex * 2 + 1) - sizex;
					int sizeModY = local_rng.rand() % (sizey * 2 + 1) - sizey;
					if ( abs(sizex) <= 2 )
					{
						sizeModX += sizeModX > 0 ? 1 : -1;
					}
					if ( abs(sizey) <= 2 )
					{
						sizeModY += sizeModY > 0 ? 1 : -1;
					}

					if ( parent->behavior == &actPlayer )
					{
						my->x += 1.0 * cos(parent->yaw + PI);
						my->y += 1.0 * sin(parent->yaw + PI);
					}
					my->x += sizeModX;
					my->y += sizeModY;
					my->fskill[0] = my->x;
					my->fskill[1] = my->y;
				}
			}
			else if ( my->skill[1] == PARTICLE_EFFECT_IGNITE_ORBIT_FOLLOW )
			{
				if ( parent )
				{
					my->yaw = parent->yaw + PI;
					my->yaw += (PI / 32) * sin(my->fskill[3]);
					my->x = parent->x + my->actmagicOrbitDist * cos(my->yaw);
					my->y = parent->y + my->actmagicOrbitDist * sin(my->yaw);

					if ( !my->actmagicNoLight )
					{
						if ( !my->light )
						{
							my->light = addLight(my->x / 16, my->y / 16, "magic_spray_orange_flicker");
						}
					}
				}
			}
			else if ( my->skill[1] == PARTICLE_EFFECT_RADIANT_ORBIT_FOLLOW )
			{
				if ( parent )
				{
					my->yaw = parent->yaw + PI;
					my->yaw += (PI / 32) * sin(my->fskill[3]) + my->fskill[5];
					my->x = parent->x + my->actmagicOrbitDist * cos(my->yaw);
					my->y = parent->y + my->actmagicOrbitDist * sin(my->yaw);

					if ( !my->actmagicNoLight )
					{
						if ( !my->light )
						{
							my->light = addLight(my->x / 16, my->y / 16, "magic_spray_orange_flicker");
						}
					}
				}
			}
			else if ( my->skill[1] == PARTICLE_EFFECT_HEAT_ORBIT_SPIN )
			{
				if ( parent )
				{
					my->x = parent->x + my->actmagicOrbitDist * cos(my->yaw);
					my->y = parent->y + my->actmagicOrbitDist * sin(my->yaw);
					/*if ( !my->actmagicNoLight )
					{
						if ( !my->light )
						{
							my->light = addLight(my->x / 16, my->y / 16, "magic_spray_orange_flicker");
						}
					}*/
				}
			}
			else
			{
				my->x = my->fskill[0] + my->actmagicOrbitDist * cos(my->yaw);
				my->y = my->fskill[1] + my->actmagicOrbitDist * sin(my->yaw);
			}
			my->z += my->vel_z;

			if ( ((my->sprite >= 233 && my->sprite <= 244) 
				|| (my->sprite >= 263 && my->sprite <= 274)
				|| (my->sprite >= 288 && my->sprite <= 299)) && my->flags[SPRITE] )
			{
				int endSprite = 244;
				if ( (my->sprite >= 263 && my->sprite <= 274) )
				{
					endSprite = 274;
				}
				else if ( (my->sprite >= 288 && my->sprite <= 299) )
				{
					endSprite = 299;
				}
				if ( my->ticks % 4 == 0 )
				{
					if ( my->sprite < endSprite )
					{
						++my->sprite;
						if ( my->sprite == endSprite && my->skill[1] == PARTICLE_EFFECT_IGNITE_ORBIT_LOOP )
						{
							my->z = 7.5;
							my->fskill[2] += PI / 4;
							my->yaw = my->fskill[2];
							my->bNeedsRenderPositionInit = true;
						}
					}
				}

				if ( my->ticks % 2 == 0 )
				{
					bool doParticle = true;
					if ( *cvar_magic_fx_use_vismap && !intro )
					{
						if ( !parent 
							|| (parent && parent->behavior != actPlayer
								&& parent->behavior != actPlayerLimb
								&& !parent->flags[OVERDRAW]
								&& !parent->flags[GENIUS]) )
						{
							int x = my->x / 16.0;
							int y = my->y / 16.0;
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
									doParticle = false;
								}
							}
						}
					}

					if ( doParticle )
					{
						real_t spread = 1.0;
						if ( Entity* fx = spawnMagicParticleCustom(my, my->sprite, 1.0, spread) )
						{
							fx->sprite = my->sprite;
							fx->flags[SPRITE] = my->flags[SPRITE];
							fx->ditheringDisabled = true;
							fx->fskill[1] = 0.025; // decay size
							fx->focalx = my->focalx;
							fx->focaly = my->focaly;
							fx->focalz = my->focalz;
							if ( my->skill[1] == PARTICLE_EFFECT_FLAMES_BURNING )
							{
								fx->scalex = 0.5;
								fx->scaley = fx->scalex;
								fx->scalez = fx->scalex;
								fx->skill[0] = 4 + (endSprite - fx->sprite) * 4; // life before decay
								fx->fskill[1] = 0.5; // decay size
								//fx->vel_z = -0.25;
								//fx->fskill[0] = -0.005; //gravity
							}
							else if ( my->skill[1] == PARTICLE_EFFECT_RADIANT_ORBIT_FOLLOW )
							{
								//fx->x = my->x;
								//fx->y = my->y;
								fx->scalex = my->scalex;
								fx->scaley = fx->scalex;
								fx->scalez = fx->scalex;
								//fx->fskill[1] = 0.5; // decay size
							}
							else if ( my->skill[1] == PARTICLE_EFFECT_HEAT_ORBIT_SPIN )
							{
								//fx->flags[BRIGHT] = true;
								fx->lightBonus = my->lightBonus;
							}
							/*if ( my->skill[1] == PARTICLE_EFFECT_IGNITE_ORBIT || my->skill[1] == PARTICLE_EFFECT_IGNITE_ORBIT_LOOP )
							{
								real_t dir = atan2(my->vel_y, my->vel_x);
								fx->x -= 2.0 * cos(dir);
								fx->y -= 2.0 * sin(dir);
							}*/
						}
					}
				}
			}

			if ( PARTICLE_LIFE <= 10 )
			{
				my->scalex -= 0.1;
				my->scaley -= 0.1;
				my->scalez -= 0.1;
				if ( my->scalex <= 0.0 )
				{
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
			}
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_SABOTAGE_ORBIT )
		{
			my->yaw = my->fskill[2];
			my->yaw += (PI / 32) * sin(my->fskill[3]);
			my->fskill[3] += 0.1;
			my->x = my->fskill[0] + 4.0 * cos(my->yaw);
			my->y = my->fskill[1] + 4.0 * sin(my->yaw);
			my->z += my->vel_z;

			if ( my->ticks % 4 == 0 )
			{
				my->sprite++;
				if ( my->sprite > 274 )
				{
					my->sprite = 263;
				}
			}
			//spawnMagicParticleCustom(my, my->sprite, my->scalex, 1.0);
			if ( PARTICLE_LIFE <= 10 )
			{
				my->scalex -= 0.05;
				my->scaley -= 0.05;
				my->scalez -= 0.05;
				if ( my->scalex <= 0.0 )
				{
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
			}
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_CONFUSE_ORBIT )
		{
			my->fskill[2] += 0.05;
			my->yaw -= 0.05;
			my->x = parent->x + my->actmagicOrbitDist * cos(my->fskill[2]);
			my->y = parent->y + my->actmagicOrbitDist * sin(my->fskill[2]);

			Stat* stats = parent->getStats();
			if ( PARTICLE_LIFE <= 10 || (!stats || !stats->getEffectActive(EFF_CONFUSED)) )
			{
				my->scalex -= 0.05;
				my->scaley -= 0.05;
				my->scalez -= 0.05;
				if ( my->scalex <= 0.0 )
				{
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
			}
			else
			{
				my->scalex = std::min(my->scalex + 0.05, 0.5);
				my->scaley = std::min(my->scaley + 0.05, 0.5);
				my->scalez = std::min(my->scalez + 0.05, 0.5);
			}
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_BLOOD_BUBBLE )
		{
			my->z -= 0.1;
			my->fskill[2] += my->fskill[3];
			my->x = my->actmagicOrbitStationaryX + my->actmagicOrbitDist * cos(my->fskill[2]);
			my->y = my->actmagicOrbitStationaryY + my->actmagicOrbitDist * sin(my->fskill[2]);

			if ( PARTICLE_LIFE <= 40 )
			{
				if ( my->ticks % 4 == 0 )
				{
					if ( my->sprite < 286 )
					{
						my->sprite++;
					}
					else if ( my->ticks % 16 == 0 )
					{
						my->removeLightField();
						list_RemoveNode(my->mynode);
						return;
					}
				}
			}

			if ( PARTICLE_LIFE <= 10 )
			{
				my->scalex -= 0.0125;
				my->scaley -= 0.0125;
				my->scalez -= 0.0125;
				if ( my->scalex <= 0.0 )
				{
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
			}
			else
			{
				my->scalex = std::min(my->scalex + 0.0025, 0.15);
				my->scaley = std::min(my->scaley + 0.0025, 0.15);
				my->scalez = std::min(my->scalez + 0.0025, 0.15);
			}
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_MAGICIANS_ARMOR_ORBIT )
		{
			my->fskill[2] += 0.05;
			my->yaw -= 0.05;
			my->x = parent->x + my->actmagicOrbitDist * cos(my->fskill[2]);
			my->y = parent->y + my->actmagicOrbitDist * sin(my->fskill[2]);
			my->z += my->vel_z;
			Stat* stats = parent->getStats();

			if ( PARTICLE_LIFE <= 40 )
			{
				if ( my->ticks % 8 == 0 )
				{
					if ( my->sprite < 279 )
					{
						my->sprite++;
					}
					else if ( my->ticks % 16 == 0 )
					{
						my->removeLightField();
						list_RemoveNode(my->mynode);
						return;
					}
				}
			}

			if ( PARTICLE_LIFE <= 10 || (!stats || !stats->getEffectActive(EFF_MAGICIANS_ARMOR)) )
			{
				my->scalex -= 0.0125;
				my->scaley -= 0.0125;
				my->scalez -= 0.0125;
				if ( my->scalex <= 0.0 )
				{
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
			}
			else
			{
				my->scalex = std::min(my->scalex + 0.0025, 0.15);
				my->scaley = std::min(my->scaley + 0.0025, 0.15);
				my->scalez = std::min(my->scalez + 0.0025, 0.15);
			}
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_GUARD_BODY_ORBIT
			|| my->skill[1] == PARTICLE_EFFECT_GUARD_SPIRIT_ORBIT
			|| my->skill[1] == PARTICLE_EFFECT_GUARD_DIVINE_ORBIT
			|| my->skill[1] == PARTICLE_EFFECT_BLOOD_WARD_ORBIT )
		{
			my->fskill[2] += 0.05;
			my->yaw -= 0.05;
			my->x = parent->x + my->actmagicOrbitDist * cos(my->fskill[2]);
			my->y = parent->y + my->actmagicOrbitDist * sin(my->fskill[2]);
			my->z += my->vel_z;
			Stat* stats = parent->getStats();
			int effectID = my->skill[1] == PARTICLE_EFFECT_GUARD_BODY_ORBIT ? EFF_GUARD_BODY
				: (my->skill[1] == PARTICLE_EFFECT_GUARD_SPIRIT_ORBIT ? EFF_GUARD_SPIRIT
				: (my->skill[1] == PARTICLE_EFFECT_GUARD_DIVINE_ORBIT ? EFF_DIVINE_GUARD
				: (my->skill[1] == PARTICLE_EFFECT_BLOOD_WARD_ORBIT ? EFF_BLOOD_WARD : -1)));
			if ( PARTICLE_LIFE <= 10 || (!stats || effectID < 0 || !stats->getEffectActive(effectID)) )
			{
				my->scalex -= 0.05;
				my->scaley -= 0.05;
				my->scalez -= 0.05;
				if ( my->scalex <= 0.0 )
				{
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
			}
			else
			{
				my->scalex = std::min(my->scalex + 0.05, 0.25);
				my->scaley = std::min(my->scaley + 0.05, 0.25);
				my->scalez = std::min(my->scalez + 0.05, 0.25);
			}
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_MUSHROOM_SPELL )
		{
			int mapx = static_cast<int>(my->x) >> 4;
			int mapy = static_cast<int>(my->y) >> 4;
			int mapIndex = (mapy)*MAPLAYERS + (mapx)*MAPLAYERS * map.height;
			if ( mapx > 0 && mapy > 0 && mapx < map.width - 1 && mapy < map.height - 1 )
			{
				if ( map.tiles[OBSTACLELAYER + mapIndex] )
				{
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
			}

			spawnMagicParticle(my);
			if ( my->ticks % 4 == 0 )
			{
				my->yaw += PI / 2;
			}
			my->x = parent->x + (my->actmagicOrbitDist * my->fskill[2]) * cos(my->fskill[4] + my->fskill[3]);
			my->y = parent->y + (my->actmagicOrbitDist * my->fskill[2]) * sin(my->fskill[4] + my->fskill[3]);

			my->fskill[2] = std::min(my->fskill[2] + 0.05, 1.0);

			if ( my->skill[3] == 0 )
			{
				my->pitch = -PI + (3 *  PI / 4) * sin(my->fskill[2] * PI / 2);
				my->z = 0.0 - 7.55 * sin(my->fskill[2] * PI / 2);
				if ( my->z <= -7.5 )
				{
					my->z = -7.5;
					my->skill[3] = 1;
					my->vel_z = 0.0;
				}
			}
			else if ( my->skill[3] == 1 )
			{
				my->fskill[3] += 0.005;
				my->vel_z += 0.02;
				my->pitch = std::min(0.0, my->pitch + my->vel_z);
				my->z += my->vel_z;
				if ( my->z > 8.0 )
				{
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
			}
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_VORTEX_ORBIT )
		{
			my->yaw += 0.2;
			if ( Entity* fx = spawnMagicParticle(my) )
			{
				fx->scalex = my->scalex * 0.9;
				fx->scaley = my->scaley * 0.9;
				fx->scalez = my->scalez * 0.9;
			}
			my->x = parent->x + my->actmagicOrbitDist * cos(my->yaw);
			my->y = parent->y + my->actmagicOrbitDist * sin(my->yaw);
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_FOCI_ORBIT )
		{
			if ( Entity* fx = spawnMagicParticleCustom(my, my->sprite, my->scalex, 1.0) )
			{
				fx->focalx = my->focalx;
				fx->focaly = my->focaly;
				fx->focalz = my->focalz;
				fx->ditheringDisabled = true;
				fx->flags[SPRITE] = my->flags[SPRITE];
			}
			my->x = parent->x + 8.0 * cos(parent->yaw);
			my->y = parent->y + 8.0 * sin(parent->yaw);
			my->yaw = parent->yaw + PI + my->fskill[0];
			my->focalx = 0.5;
			my->focalz = 0.5;
			my->pitch -= 0.1;
			//my->roll = PI / 2;
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_SHATTER_EARTH_ORBIT )
		{
			//my->yaw += 0.2;
			if ( Entity* fx = spawnMagicParticle(my) )
			{
				fx->scalex = my->scalex * 0.9;
				fx->scaley = my->scaley * 0.9;
				fx->scalez = my->scalez * 0.9;
			}
			my->x = my->actmagicOrbitStationaryX + my->actmagicOrbitDist * cos(my->yaw);
			my->y = my->actmagicOrbitStationaryY + my->actmagicOrbitDist * sin(my->yaw);
			my->z += my->vel_z;

			if ( abs(my->vel_x) > 0.01 )
			{
				my->actmagicOrbitStationaryX += my->vel_x;
			}
			if ( abs(my->vel_y) > 0.01 )
			{
				my->actmagicOrbitStationaryY += my->vel_y;
			}

			if ( my->sprite == 2205 )
			{
				my->pitch = PI / 2 + atan(my->vel_z / (sqrt(pow(my->vel_x, 2) + pow(my->vel_y, 2))));
			}
			my->vel_z += 0.04;
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_STATIC_ORBIT )
		{
			//my->yaw += 0.2;
			my->z -= 0.1;
			my->actmagicOrbitDist = std::min(80, my->actmagicOrbitDist + 1);
			my->bNeedsRenderPositionInit = true;
			my->x = parent->x + (my->actmagicOrbitDist / 10.0) * cos(my->yaw);
			my->y = parent->y + (my->actmagicOrbitDist / 10.0) * sin(my->yaw);
			my->flags[INVISIBLE] = true;
			Uint32 anim = (my->ticks % 50) / 10;
			my->removeLightField();
			if ( !my->actmagicNoLight )
			{
				if ( (my->sprite >= 1758 && my->sprite <= 1763) )
				{
					my->light = addLight(my->x / 16, my->y / 16, "orb_blue");
				}
				else if ( my->sprite >= 2335 && my->sprite <= 2340 )
				{
					my->light = addLight(my->x / 16, my->y / 16, "orb_red");
				}
				else if ( my->sprite >= 2347 && my->sprite <= 2352 )
				{
					my->light = addLight(my->x / 16, my->y / 16, "orb_purple");
				}
				else if ( my->sprite >= 2341 && my->sprite <= 2346 )
				{
					my->light = addLight(my->x / 16, my->y / 16, "orb_blue");
				}
			}

			if ( anim == 0 || anim == 2 || anim >= 4 )
			{
				my->flags[INVISIBLE] = false;
			}
			if ( my->ticks % 4 == 0 )
			{
				int prevSprite = my->sprite;
				my->sprite++;
				my->yaw += 1 * PI / 3;
				if ( prevSprite == 1763 )
				{
					my->sprite = 1758;
				}
				else if ( prevSprite == 2340 )
				{
					my->sprite = 2335;
				}
				else if ( prevSprite == 2346 )
				{
					my->sprite = 2341;
				}
				else if ( prevSprite == 2352 )
				{
					my->sprite = 2347;
				}

				if ( local_rng.rand() % 4 == 0 )
				{
					bool doSound = true;
					if ( !(my->sprite >= 1758 && my->sprite <= 1763) /*&& parent->behavior == &actPlayer && players[parent->skill[2]]->isLocalPlayer()*/ )
					{
						if ( local_rng.rand() % 4 > 0 )
						{
							doSound = false;
						}
					}
					if ( doSound )
					{
						playSoundEntityLocal(my, 808 + local_rng.rand() % 5, 32);
					}
				}
			}
			if ( !my->flags[INVISIBLE] )
			{
				spawnMagicParticle(my);
			}
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_ETERNALS_GAZE_STATIC )
		{
			//my->yaw += 0.2;
			my->z -= 0.1;
			my->actmagicOrbitDist = std::min(80, my->actmagicOrbitDist + 1);
			my->bNeedsRenderPositionInit = true;
			my->x = parent->x + (my->actmagicOrbitDist / 10.0) * cos(my->yaw);
			my->y = parent->y + (my->actmagicOrbitDist / 10.0) * sin(my->yaw);
			my->flags[INVISIBLE] = true;
			Uint32 anim = (my->ticks % 50) / 10;
			my->removeLightField();
			if ( !my->actmagicNoLight )
			{
				my->light = addLight(my->x / 16, my->y / 16, "orb_blue");
			}

			if ( anim == 0 || anim == 2 || anim >= 4 )
			{
				my->flags[INVISIBLE] = false;
			}
			if ( !my->flags[INVISIBLE] )
			{
				spawnMagicParticle(my);
			}
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_NULL_PARTICLE || my->skill[1] == PARTICLE_EFFECT_NULL_PARTICLE_NOSOUND )
		{
			if ( my->ticks == 1 && my->skill[1] == PARTICLE_EFFECT_NULL_PARTICLE )
			{
				if ( nullParticleSfxTick == 0 || (ticks - nullParticleSfxTick) > 5 )
				{
					playSoundEntityLocal(my, 166, 128);
					nullParticleSfxTick = ticks;
				}
			}
			//my->yaw += 0.2;
			my->z -= 0.1;
			//my->actmagicOrbitDist = std::min(80, my->actmagicOrbitDist + 1);
			my->bNeedsRenderPositionInit = true;
			//my->x = parent->x + (my->actmagicOrbitDist / 10.0) * cos(my->yaw);
			//my->y = parent->y + (my->actmagicOrbitDist / 10.0) * sin(my->yaw);
			my->flags[INVISIBLE] = true;
			Uint32 anim = (my->ticks % 50) / 10;
			/*my->removeLightField();
			if ( !my->actmagicNoLight )
			{
				my->light = addLight(my->x / 16, my->y / 16, "orb_blue");
			}*/

			if ( anim == 0 || anim == 2 || anim >= 4 )
			{
				my->flags[INVISIBLE] = false;
			}
			/*if ( my->ticks % 4 == 0 )
			{
				my->sprite++;
				my->yaw += 1 * PI / 3;
				if ( my->sprite > 1763 )
				{
					my->sprite = 1758;
				}
			}*/
			if ( !my->flags[INVISIBLE] )
			{
				spawnMagicParticle(my);
			}
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_REVENANT_CURSE )
		{
			if ( parent )
			{
				my->x = parent->x;
				my->y = parent->y;
				real_t setpoint = -7.5;
				real_t dist = setpoint - 7.5;
				my->z = 7.5 + dist * (sin(std::min(1.0, my->ticks / ((real_t)1.5 * TICKS_PER_SECOND)) * PI / 2));

				if ( my->ticks >= TICKS_PER_SECOND )
				{
					my->pitch += 0.025;
					my->pitch = std::min(PI / 4, my->pitch);
				}

				my->yaw = parent->yaw;
				my->ditheringOverride = 6;

				Entity* fx = spawnMagicParticle(my);
				fx->vel_z = 0.25;
				fx->ditheringOverride = 6;
			}
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_PSYCHIC_SPEAR )
		{
			if ( parent )
			{
				Uint32 tickOffset = TICKS_PER_SECOND;
				real_t anim = my->ticks < tickOffset ? (0.5 - 0.5 * (my->ticks / (real_t)tickOffset)) 
					: std::min(1.0, (my->ticks - tickOffset) / ((real_t)0.15 * TICKS_PER_SECOND));
				my->yaw = my->fskill[0];// +anim * (my->fskill[1] - my->fskill[0]);
				my->pitch = my->fskill[1];

				if ( my->ticks == 1 )
				{
					playSoundEntityLocal(my, 822, 92);
				}

				my->scalex = std::min(1.0, (my->ticks / (real_t)tickOffset));
				my->scaley = my->scalex;
				my->scalez = my->scalex;
				my->flags[INVISIBLE] = my->scalex < 0.99;

				my->x = parent->x - (sin((1.0 - anim) * PI / 2)) * (20.0 * cos(my->pitch)) * cos(my->yaw);
				my->y = parent->y - (sin((1.0 - anim) * PI / 2)) * (20.0 * cos(my->pitch)) * sin(my->yaw);
				my->z = parent->z - (sin((1.0 - anim) * PI / 2)) * 20.0;

				Uint32 impactTick = (Uint32)(0.25 * TICKS_PER_SECOND + tickOffset);
				if ( my->ticks == impactTick )
				{
					playSoundEntityLocal(my, 821, 92);

					Entity* caster = uidToEntity(my->skill[3]);
					int damage = getSpellDamageFromID(SPELL_PSYCHIC_SPEAR, caster, caster ? caster->getStats() : nullptr, my);
					if ( Stat* parentStats = parent->getStats() )
					{
						real_t hpThreshold = getSpellEffectDurationSecondaryFromID(SPELL_PSYCHIC_SPEAR, caster, caster ? caster->getStats() : nullptr, my) / 100.0;
						real_t parentHPRatio = std::min(1.0, parentStats->HP / std::max(1.0, (real_t)parentStats->MAXHP));
						if ( parentHPRatio >= hpThreshold )
						{
							real_t scale = std::min(1.0, std::max(0.0, (parentHPRatio - hpThreshold) / (1.0 - hpThreshold)));
							damage += scale * getSpellDamageSecondaryFromID(SPELL_PSYCHIC_SPEAR, caster, caster ? caster->getStats() : nullptr, my);
						}
					}

					applyGenericMagicDamage(caster, parent, caster ? *caster : *my, SPELL_PSYCHIC_SPEAR, damage, true);

					if ( Entity* fx = createParticleAOEIndicator(my, my->x, my->y, 0.0, TICKS_PER_SECOND, 16.0) )
					{
						//fx->actSpriteFollowUID = 0;
						fx->actSpriteCheckParentExists = 0;
						fx->scalex = 0.8;
						fx->scaley = 0.8;
						if ( auto indicator = AOEIndicators_t::getIndicator(fx->skill[10]) )
						{
							//indicator->arc = PI / 2;
							Uint32 color = makeColorRGB(255, 0, 255);
							indicator->indicatorColor = color;
							indicator->loop = false;
							indicator->gradient = 4;
							indicator->framesPerTick = 4;
							indicator->ticksPerUpdate = 1;
							indicator->delayTicks = 0;
							indicator->expireAlphaRate = 0.9;
							indicator->cacheType = AOEIndicators_t::CACHE_PSYCHIC_SPEAR;
						}
					}
					for ( int i = 0; i < 10; ++i )
					{
						if ( Entity* gib = multiplayer == CLIENT ? spawnGibClient(0, 0, 0, 2360) : spawnGib(my, 2360) )
						{
							gib->sprite = 2360;
							gib->x = my->x;
							gib->y = my->y;
							gib->z = my->z;
						}
					}
				}

				if ( my->ticks >= impactTick + 15 )
				{
					list_RemoveNode(my->mynode);
					return;
				}

				spawnMagicParticleCustom(my, my->sprite, std::max(0.0, my->scalex - 0.3), 1.0);

				if ( my->ticks < tickOffset )
				{
					my->fskill[2] += 0.25;
					//if ( my->ticks % 2 == 0 )
					{
						for ( int i = 0; i < 3; ++i )
						{
							if ( Entity* fx = spawnMagicParticleCustom(my, 2360, 1.0, 10.0) )
							{
								fx->yaw = my->fskill[2] + i * 2 * PI / 3;
								fx->x = parent->x + 4.0 * cos(fx->yaw);
								fx->y = parent->y + 4.0 * sin(fx->yaw);
								fx->z = 7.5 - my->fskill[2] * 3;
								fx->vel_z = -0.05;
							}
						}
					}
				}
			}
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_SMITE_PINPOINT || my->skill[1] == PARTICLE_EFFECT_TURN_UNDEAD )
		{
			if ( my->skill[5] == 0 )
			{
				my->fskill[0] = 0.0;
				my->fskill[1] = 0.0;
				my->fskill[2] = 0.0;
				my->z = -7.5;
				my->skill[5] = 1;
				if ( !my->actmagicNoLight )
				{
					if ( my->skill[1] == PARTICLE_EFFECT_SMITE_PINPOINT )
					{
						createParticleFociLight(parent ? parent : my, SPELL_PINPOINT, false);
					}
					playSoundEntityLocal(my, 822, 128);
				}
			}
			if ( parent )
			{
				my->fskill[4] = parent->x;
				my->fskill[5] = parent->y;
			}
			my->focalz = 0.5;

			my->fskill[0] = std::min(my->fskill[0] + 0.05, 1.0);
			my->z = -10.0;

			my->removeLightField();
			if ( !my->actmagicNoLight )
			{
				my->light = addLight(my->x / 16, my->y / 16, "magic_white");
			}

			int breakpoint1 = 40;
			if ( my->fskill[0] >= 1.0 )
			{
				if ( my->ticks >= breakpoint1 )
				{
					my->fskill[1] = std::min(my->fskill[1] + 0.15, 1.0);
					if ( my->fskill[1] >= 1.0 )
					{
						if ( my->ticks >= breakpoint1 + 20 )
						{
							if ( my->ticks == breakpoint1 + 20 )
							{
								if ( !my->actmagicNoLight )
								{
									createParticleFociLight(parent ? parent : my, SPELL_PINPOINT, false);
								}
							}

							if ( my->ticks == breakpoint1 + 25 )
							{
								if ( !my->actmagicNoLight )
								{
									playSoundEntityLocal(my, 849, 128);
									// main particle
									if ( multiplayer != CLIENT )
									{
										if ( parent )
										{
											Entity* caster = uidToEntity(my->skill[3]);

											if ( my->skill[1] == PARTICLE_EFFECT_SMITE_PINPOINT )
											{
												int flatDamage = getSpellDamageFromID(SPELL_PINPOINT, caster, caster ? caster->getStats() : nullptr,
													my);
												if ( parent->isSmiteWeakMonster() )
												{
													flatDamage *= 2.0;
													playSoundEntity(parent, 249, 64);
												}
												flatDamage += my->skill[4];
												applyGenericMagicDamage(caster, parent, caster ? *caster : *my, SPELL_PINPOINT, flatDamage + my->skill[4], true);
												//spawnMagicEffectParticles(parent->x, parent->y, parent->z, 981);

												if ( caster && parent->getStats() )
												{
													messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(0, 255, 0),
														*parent->getStats(), Language::get(6950), Language::get(6951), MSG_COMBAT);
												}
											}
											else if ( my->skill[1] == PARTICLE_EFFECT_TURN_UNDEAD )
											{
												int damage = getSpellDamageFromID(SPELL_TURN_UNDEAD, caster, caster ? caster->getStats() : nullptr,
													my);
												applyGenericMagicDamage(caster, parent, caster ? *caster : *my, SPELL_TURN_UNDEAD, damage, true);

												if ( my->skill[6] == EFF_HOLY_FIRE )
												{
													if ( parent->setEffect(EFF_HOLY_FIRE, true, 5 * TICKS_PER_SECOND, true) )
													{
														int burnDuration = getSpellEffectDurationFromID(SPELL_HOLY_FIRE, caster, caster ? caster->getStats() : nullptr,
															my);
														if ( Entity* fx = createParticleAestheticOrbit(parent, 288, burnDuration, PARTICLE_EFFECT_HOLY_FIRE) )
														{
															fx->flags[SPRITE] = true;
															fx->flags[INVISIBLE] = true;
															fx->skill[3] = caster ? caster->getUID() : 0;
														}
														serverSpawnMiscParticles(parent, PARTICLE_EFFECT_HOLY_FIRE, 288, 0, burnDuration);
													}
												}
												//spawnMagicEffectParticles(parent->x, parent->y, parent->z, 981);
											}
										}
										else
										{
											//spawnMagicEffectParticles(my->x, my->y, my->z, 981);
										}
									}
								}
							}

							my->fskill[2] = std::min(my->fskill[2] + 0.06, 1.0);
							if ( my->fskill[2] >= 1.0 )
							{
								my->removeLightField();
								list_RemoveNode(my->mynode);
								return;
							}
						}
					}
				}
			}
			if ( my->ticks < breakpoint1 )
			{
				my->fskill[6] += 0.05 * sin(my->fskill[0] * PI / 2);
			}
			my->yaw = my->fskill[6];
				
			my->z -= 4.0 * sin(my->fskill[1] * PI / 2);
			my->z += 20.0 * sin(my->fskill[2] * PI / 2);

			my->x = my->fskill[4];
			my->y = my->fskill[5];
			if ( my->skill[1] == PARTICLE_EFFECT_SMITE_PINPOINT )
			{
				my->roll -= 0.25;
				my->pitch = PI / 2 + my->fskill[1] * PI / 6;
				my->x += 6.0 * cos(my->yaw) * sin(my->fskill[0] * PI / 2);
				my->y += 6.0 * sin(my->yaw) * sin(my->fskill[0] * PI / 2);
				my->x += cos(my->yaw) * (4.0 * sin(my->fskill[1] * PI / 2) - 16.0 * sin(my->fskill[2] * PI / 2));
				my->y += sin(my->yaw) * (4.0 * sin(my->fskill[1] * PI / 2) - 16.0 * sin(my->fskill[2] * PI / 2));
				if ( my->fskill[1] > 0.05 )
				{
					if ( my->ticks % 2 == 0 )
					{
						if ( Entity* fx = spawnMagicParticleCustom(my, my->sprite, my->scalex, 10.0) )
						{
							fx->focalx = my->focalx;
							fx->focaly = my->focaly;
							fx->focalz = my->focalz;
						}
					}
				}
			}
			else if ( my->skill[1] == PARTICLE_EFFECT_TURN_UNDEAD )
			{
				if ( my->fskill[1] > 0.05 )
				{
					my->fskill[7] = std::min(my->fskill[7] + 0.1, 1.0);
					if ( my->ticks % 2 == 0 )
					{
						if ( Entity* fx = spawnMagicParticleCustom(my, my->sprite, my->scalex, 10.0) )
						{
							fx->focalx = my->focalx;
							fx->focaly = my->focaly;
							fx->focalz = my->focalz;
						}
					}
				}
				else
				{
					my->roll += 0.25;
				}
				my->pitch = PI / 2 + my->fskill[7] * 2 * PI;
			}


		}
		else if ( my->skill[1] == PARTICLE_EFFECT_DEFY_FLESH_ORBIT )
		{
			if ( parent )
			{
				Stat* stats = parent->getStats();
				if ( !stats || (stats && !stats->getEffectActive(EFF_DEFY_FLESH)) )
				{
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}

				if ( my->skill[5] == 0 )
				{
					my->skill[5] = 1;
					playSoundEntityLocal(parent, 166, 128);
				}

				my->removeLightField();
				my->light = addLight(my->x / 16, my->y / 16, "orb_red");

				my->fskill[0] += 0.15;
				my->x = parent->x + 8.0 * cos(my->fskill[0]);
				my->y = parent->y + 8.0 * sin(my->fskill[0]);
				my->z = 0.0 + 0.75 * sin(2 * PI * (my->ticks % 50) / 50.0);
				my->pitch = -0.15 * sin(2 * PI * (my->ticks % 50) / 50.0);
				//my->pitch = PI / 2;
				my->yaw = my->fskill[0] + PI / 2;
				my->roll += 0.25;
				my->focalz = 0.25;
				my->flags[INVISIBLE] = false;

				my->scalex = 0.5;
				my->scaley = 0.5;
				my->scalez = 0.5;

				/*Uint32 anim = (my->ticks % 50) / 10;
				if ( anim == 0 || anim == 2 || anim >= 4 )
				{
					my->flags[INVISIBLE] = false;
				}
				if ( my->ticks % 4 == 0 )
				{
					int prevSprite = my->sprite;
					my->sprite++;
					my->yaw += 1 * PI / 3;
					if ( !(my->sprite >= 2335 && my->sprite < 2340) )
					{
						my->sprite = 2335;
					}
					if ( prevSprite == 2340 )
					{
						my->sprite = 2335;
					}
				}*/

				if ( my->ticks % 2 >= 0 )
				{
					if ( Entity* fx = spawnMagicParticleCustom(my, my->sprite, my->scalex, 10.0) )
					{
						fx->focalz = my->focalz;
						fx->fskill[1] = 0.5;
					}
				}

				bool trigger = false;
				if ( my->skill[8] > 0 ) // cooldown
				{
					--my->skill[8];
				}
				else
				{
					if ( my->skill[7] > 0 )  // trigger
					{
						my->skill[7] = 0;
						trigger = true;
					}
				}

				if ( multiplayer != CLIENT )
				{
					if ( trigger )
					{
						my->skill[8] = TICKS_PER_SECOND / 10;
						Entity* fx = createParticleAestheticOrbit(parent, 2363, 3 * TICKS_PER_SECOND, PARTICLE_EFFECT_DEFY_FLESH);
						fx->yaw = my->fskill[0] + PI;
						fx->yaw = fmod(fx->yaw, 2 * PI);
						fx->skill[3] = my->skill[3]; // caster
						fx->pitch = PI / 2;
						fx->fskill[0] = fx->yaw;
						fx->fskill[1] = PI / 4 - PI / 8;
						fx->fskill[2] = parent->z;
						fx->x = parent->x - 8.0 * cos(fx->yaw);
						fx->y = parent->y - 8.0 * sin(fx->yaw);
						fx->z = parent->z;
						fx->scalex = 0.0;
						fx->scaley = 0.0;
						fx->scalez = 0.0;

						serverSpawnMiscParticles(parent, PARTICLE_EFFECT_DEFY_FLESH, 2363, 0, 3 * TICKS_PER_SECOND, fx->yaw * 256.0);

						if ( stats )
						{
							Uint8 charges = stats->getEffectActive(EFF_DEFY_FLESH) & 0xF;
							if ( charges == 0 )
							{
								parent->setEffect(EFF_DEFY_FLESH, false, 0, true);
							}
						}
					}
				}
			}
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_DEFY_FLESH )
		{
			if ( parent )
			{
				Uint32 tickOffset = 0;// TICKS_PER_SECOND;
				bool trigger = my->ticks >= tickOffset;

				if ( trigger )
				{
					my->skill[4]++;
					if ( my->skill[5] == 0 )
					{
						playSoundEntityLocal(my, 823, 64);
						my->skill[5] = 1;
					}
				}
				real_t anim = std::min(my->skill[4] / 25.0, 1.0);
				if ( anim >= 0.75 )
				{
					my->skill[4]++;
				}

				my->yaw = my->fskill[0];
				my->pitch = PI / 2 - anim * ((PI / 2) - my->fskill[1]);
				my->roll += 0.25;
				my->focalz = 0.25;

				my->scalex = std::min(1.0, 0.5 + 0.5 * anim);
				my->scaley = my->scalex;
				my->scalez = my->scalex;
				my->flags[INVISIBLE] = false;// my->scalex < 0.01;

				my->x = parent->x - (8.0 * (1.0 - std::min(0.8, anim)) + (sin((1.0 - anim) * PI)) * (8.0)) * cos(my->yaw);
				my->y = parent->y - (8.0 * (1.0 - std::min(0.8, anim)) + (sin((1.0 - anim) * PI)) * (8.0)) * sin(my->yaw);
				my->z = my->fskill[2] - (sin((1.0 - anim) * PI)) * 4.0;

				Uint32 impactTick = TICKS_PER_SECOND / 2 + tickOffset;

				if ( Entity* fx = spawnMagicParticleCustom(my, my->sprite, std::max(0.0, my->scalex - 0.3), 10.0) )
				{
					real_t dir = my->yaw;
					fx->x -= 2.0 * cos(dir) * cos(my->pitch);
					fx->y -= 2.0 * sin(dir) * cos(my->pitch);
					fx->fskill[1] = 0.25;
				}

				if ( my->ticks == impactTick )
				{
					if ( multiplayer != CLIENT )
					{
						Entity* caster = uidToEntity(my->skill[3]);
						int damage = getSpellDamageFromID(SPELL_DEFY_FLESH, caster, caster ? caster->getStats() : nullptr,
							my);
						applyGenericMagicDamage(caster, parent, caster ? *caster : *my, SPELL_DEFY_FLESH, damage, true);
					}

					for ( int i = 0; i < 1; ++i )
					{
						if ( Entity* gib = multiplayer == CLIENT ? spawnGibClient(0, 0, 0, 2360) : spawnGib(my, 2360) )
						{
							gib->sprite = 5;
							gib->x = parent->x;
							gib->y = parent->y;
							gib->z = parent->z;
						}
					}
				}
				/*if ( my->ticks >= impactTick )
				{
					my->x += ((my->ticks - impactTick) / 5.0) * 8 * cos(my->yaw);
					my->y += ((my->ticks - impactTick) / 5.0) * 8 * sin(my->yaw);
					my->z += ((my->ticks - impactTick) / 5.0) * 4;
				}*/
				if ( my->ticks >= impactTick + 5 )
				{
					list_RemoveNode(my->mynode);
					return;
				}
			}
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_FOCI_LIGHT
			|| my->skill[1] == PARTICLE_EFFECT_FOCI_DARK )
		{
			bool particle = false;
			if ( true )
			{
				if ( my->skill[3] == 0 )
				{
					real_t diff = std::max(0.1, (32.0 - my->scaley) / 2.5);
					my->scaley = std::min(my->scaley + diff, 32.0);
					if ( my->scaley >= 32.0 )
					{
						my->skill[3] = 1;
					}
					particle = true;
				}
				else if ( my->skill[3] == 1 )
				{
					real_t diff = std::max(0.1, (my->scaley - 1.0) / 2.5);
					my->scaley = std::max(my->scaley - diff, 1.0);
					if ( my->scaley <= 1.0 )
					{
						my->skill[3] = 2;
						my->flags[INVISIBLE] = true;
					}
					particle = true;
				}

				{
					real_t diff = std::min(-0.05, (my->fskill[3] - my->fskill[2]) / 10.0);
					my->fskill[2] += diff;
					my->fskill[2] = std::max(my->fskill[2], my->fskill[3]);
				}
			}

			//my->bNeedsRenderPositionInit = true;
			if ( parent )
			{
				my->fskill[4] = parent->x;
				my->fskill[5] = parent->y;
			}
			my->x = my->fskill[4] + (my->actmagicOrbitDist * my->fskill[7]) * cos(my->yaw - 3 * PI / 4);
			my->y = my->fskill[5] + (my->actmagicOrbitDist * my->fskill[7]) * sin(my->yaw - 3 * PI / 4);
			my->yaw += my->fskill[6];
			/*if ( my->ticks >= TICKS_PER_SECOND / 2 )
			{
				real_t diff = std::max(0.01, (0.15 - my->fskill[6]) / 50.0);
				my->fskill[6] = std::min(0.15, my->fskill[6] + diff);
			}*/
			{
				real_t diff = std::max(0.01, (1.0 - my->fskill[7]) / 25.0);
				real_t prev = my->fskill[7];
				my->fskill[7] = std::min(1.0, my->fskill[7] + diff);
			}
			/*if ( my->ticks >= TICKS_PER_SECOND && my->ticks < TICKS_PER_SECOND + 5 )
			{
				particle = true;
			}*/
			my->z = my->fskill[2]; // baseline z
			my->z += (my->scaley - 1.0) / 2; // center it along z
			//my->z -= (my->scaley - 1.0) / 4;

			if ( particle )
			{
				Entity* fx = spawnMagicParticleCustom(my, my->sprite, 0.25, 1.0);
				fx->vel_z = -0.3;
				fx->ditheringDisabled = true;
			}
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_ETERNALS_GAZE1 )
		{
			bool particle = false;
			if ( my->ticks >= 2 * TICKS_PER_SECOND )
			{
				static ConsoleVariable<float> cvar_magic_egaze3("/magic_egaze3", 2.5);
				if ( my->skill[3] == 0 )
				{
					real_t diff = std::max(0.1, (32.0 - my->scaley) / *cvar_magic_egaze3);
					my->scaley = std::min(my->scaley + diff, 32.0);
					if ( my->scaley >= 32.0 )
					{
						my->skill[3] = 1;
					}
					particle = true;
				}
				else if ( my->skill[3] == 1 )
				{
					real_t diff = std::max(0.1, (my->scaley - 1.0) / *cvar_magic_egaze3);
					my->scaley = std::max(my->scaley - diff, 1.0);
					if ( my->scaley <= 1.0 )
					{
						my->skill[3] = 2;
						my->flags[INVISIBLE] = true;
					}
					particle = true;
				}

				{
					static ConsoleVariable<float> cvar_magic_egaze4("/magic_egaze4", 10.0);
					real_t diff = std::min(-0.05, (my->fskill[3] - my->fskill[2]) / *cvar_magic_egaze4);
					my->fskill[2] += diff;
					my->fskill[2] = std::max(my->fskill[2], my->fskill[3]);
				}
			}

			//my->bNeedsRenderPositionInit = true;
			my->x = parent->x + (my->actmagicOrbitDist * my->fskill[7]) * cos(my->yaw - 3 * PI / 4);
			my->y = parent->y + (my->actmagicOrbitDist * my->fskill[7]) * sin(my->yaw - 3 * PI / 4);
			my->yaw += my->fskill[6];
			/*if ( my->ticks >= TICKS_PER_SECOND / 2 )
			{
				real_t diff = std::max(0.01, (0.15 - my->fskill[6]) / 50.0);
				my->fskill[6] = std::min(0.15, my->fskill[6] + diff);
			}*/
			{
				real_t diff = std::max(0.01, (1.0 - my->fskill[7]) / 25.0);
				real_t prev = my->fskill[7];
				my->fskill[7] = std::min(1.0, my->fskill[7] + diff);
			}
			/*if ( my->ticks >= TICKS_PER_SECOND && my->ticks < TICKS_PER_SECOND + 5 )
			{
				particle = true;
			}*/
			my->z = my->fskill[2]; // baseline z
			my->z += (my->scaley - 1.0) / 2; // center it along z
			//my->z -= (my->scaley - 1.0) / 4;

			if ( particle )
			{
				static ConsoleVariable<float> cvar_magic_egaze5("/magic_egaze5", 0.25);
				static ConsoleVariable<float> cvar_magic_egaze6("/magic_egaze6", 1.0);
				Entity* fx = spawnMagicParticleCustom(my, 1866, *cvar_magic_egaze5, *cvar_magic_egaze6);
				fx->vel_z = -0.3;
				fx->ditheringDisabled = true;
			}
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_ETERNALS_GAZE2 )
		{
			static ConsoleVariable<float> cvar_magic_egaze8("/eg8", 2.5);
			if ( my->skill[3] == 0 )
			{
				real_t diff = std::max(0.1, (64.0 - my->scaley) / *cvar_magic_egaze8);
				my->scaley = std::min(my->scaley + diff, 64.0);
				if ( my->scaley >= 64.0 )
				{
					my->skill[3] = 1;
				}
				//particle = true;
			}

			if ( my->skill[3] == 1 )
			{
				if ( my->ticks >= TICKS_PER_SECOND )
				{
					real_t diff = std::max(0.1, (my->scaley - 0.0) / *cvar_magic_egaze8);
					my->scaley = std::max(my->scaley - diff, 0.0);
					if ( my->scaley <= 1.0 )
					{
						my->scalex = my->scaley;
						my->scalez = my->scalex;
						if ( my->scalex <= 0.0 )
						{
							my->removeLightField();
							list_RemoveNode(my->mynode);
							return;
						}
					}

					{
						real_t diff = std::min(-0.05, (my->fskill[3] - my->fskill[2]) / 5.0);
						my->fskill[2] += diff;
						my->fskill[2] = std::max(my->fskill[2], my->fskill[3]);
					}

					Entity* fx = spawnMagicParticleCustom(my, 1866, 0.25, 1.0);
					fx->vel_z = -0.3;
					fx->ditheringDisabled = true;
				}
			}

			my->z = my->fskill[2]; // baseline z
			my->z += (my->scaley - 1.0) / 2; // center it along z
			my->x = parent->x + (my->actmagicOrbitDist) * cos(my->yaw - 3 * PI / 4);
			my->y = parent->y + (my->actmagicOrbitDist) * sin(my->yaw - 3 * PI / 4);

			my->fskill[4] = (local_rng.rand() % 30 - 10) / 50.f;
			my->fskill[5] = (local_rng.rand() % 30 - 10) / 50.f;
			my->x += my->fskill[4] * cos(my->yaw - 3 * PI / 4 + PI / 2);
			my->y += my->fskill[5] * sin(my->yaw - 3 * PI / 4 + PI / 2);

			my->yaw += my->fskill[6];
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_HOLY_BEAM_ORBIT )
		{
			if ( parent )
			{
				my->x = parent->x + (0.0 + my->fskill[1]) * cos(my->yaw + PI);
				my->y = parent->y + (0.0 + my->fskill[1]) * sin(my->yaw + PI);
				my->z = parent->z - 0.5;

				my->focalx = 0.0;
				my->focaly = 0.0;
				my->focalz = 0.5;

				//my->roll += 0.1;// *my->fskill[1];

				if ( my->ticks % 4 == 0 )
				{
					if ( Entity* fx = spawnMagicParticleCustom(my, my->sprite, my->scalex, 5.0) )
					{
						fx->ditheringDisabled = true;
						fx->yaw = my->yaw;
						fx->pitch = my->pitch;
						fx->roll = my->roll;
						fx->focalx = my->focalx;
						fx->focaly = my->focaly;
						fx->focalz = my->focalz;
						int dir = (my->ticks % 16) / 4;
						fx->vel_x = my->fskill[1] * 0.25 * cos(dir * PI / 2) * cos(fx->yaw + PI / 2);
						fx->vel_y = my->fskill[1] * 0.25 * cos(dir * PI / 2) * sin(fx->yaw + PI / 2);
						fx->vel_z = my->fskill[1] * 0.25 * sin(dir * PI / 2);
					}
				}
			}
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_BLOOD_WAVES_ORBIT )
		{
			if ( parent )
			{
				my->x = parent->x;
				my->y = parent->y;
				my->z = parent->z;

				my->fskill[0] += 0.5;
				my->pitch = my->fskill[0];

				my->skill[3]++;
				real_t anim = std::min(1.0, my->skill[3] / 20.0);
				my->scalez = anim;
				my->z += anim * 2.0 * sin(0.5 * my->fskill[0]) * my->fskill[1];

				my->removeLightField();
				if ( my->fskill[1] > 0.0 )
				{
					my->light = addLight(my->x / 16, my->y / 16, "orb_red");
				}

				if ( my->ticks % 2 == 0 )
				{
					if ( Entity* fx = spawnMagicParticleCustom(my, my->sprite, my->scalex, 5.0) )
					{
						fx->ditheringDisabled = true;
						fx->yaw = my->yaw;
						fx->pitch = my->pitch;
						fx->roll = my->roll;
						fx->focalx = my->focalx;
						fx->focaly = my->focaly;
						fx->focalz = my->focalz;
					}
				}

				if ( my->ticks % 10 == 0 )
				{
					Entity* fx = createParticleAestheticOrbit(my, 283, TICKS_PER_SECOND, PARTICLE_EFFECT_BLOOD_BUBBLE);
					fx->x = my->x - (4.0 + (local_rng.rand() % 9)) * cos(my->yaw + PI / 2);
					fx->y = my->y - (4.0 + (local_rng.rand() % 9)) * sin(my->yaw + PI / 2);
					fx->z = my->z;
					fx->flags[SPRITE] = true;

					fx->fskill[2] = 2 * PI * (local_rng.rand() % 10) / 10.0;
					fx->fskill[3] = 0.05; // speed osc
					fx->scalex = 0.0125;
					fx->scaley = fx->scalex;
					fx->scalez = fx->scalex;
					fx->actmagicOrbitDist = 2;
					fx->actmagicOrbitStationaryX = my->x;
					fx->actmagicOrbitStationaryY = my->y;
				}
			}
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_SCEPTER_BLAST_ORBIT1
			|| my->skill[1] == PARTICLE_EFFECT_METEOR_STATIONARY_ORBIT /* doesn't need parent entity */ )
		{
			if ( my->sprite == 2192 || my->sprite == 2210 )
			{
				if ( parent )
				{
					my->x = parent->x;
					my->y = parent->y;
					my->z = parent->z - 0.5;
				}
				my->roll = 0.0;
				my->pitch -= 0.1;
				my->scalex = 0.75;
				my->scaley = 0.75;
				my->scalez = 0.75;
				my->focalx = 0.0;
				my->focaly = 0.0;
				my->focalz = 0.5;

				if ( my->sprite == 2210 )
				{
					if ( multiplayer == CLIENT && my->skill[1] == PARTICLE_EFFECT_SCEPTER_BLAST_ORBIT1 )
					{
						my->scalex = 0.75;
						my->scaley = 0.75;
						my->scalez = 0.75;
					}
					else
					{
						my->scalex = 0.75 * std::min(50, (int)my->ticks) / 50.0;
						my->scaley = 0.75 * std::min(50, (int)my->ticks) / 50.0;
						my->scalez = 0.75 * std::min(50, (int)my->ticks) / 50.0;
					}

					if ( Entity* fx = spawnMagicParticle(my) )
					{
						fx->sprite = 13;
						fx->flags[SPRITE] = true;
					}

					if ( my->ticks % 8 == 4 )
					{
						if ( Entity* fx = spawnMagicParticleCustom(my, my->sprite, my->scalex, 50.0) )
						{
							fx->ditheringDisabled = true;
							fx->yaw = my->yaw;
							fx->pitch = my->pitch;
							fx->roll = my->roll;
							fx->focalx = my->focalx;
							fx->focaly = my->focaly;
							fx->focalz = my->focalz;
						}
					}
				}
				else
				{
					if ( my->ticks % 8 == 4 )
					{
						if ( Entity* fx = spawnMagicParticleCustom(my, my->sprite, my->scalex, 10.0) )
						{
							fx->ditheringDisabled = true;
							fx->yaw = my->yaw;
							fx->pitch = my->pitch;
							fx->roll = my->roll;
							fx->focalx = my->focalx;
							fx->focaly = my->focaly;
							fx->focalz = my->focalz;
						}
					}
				}
			}
			else if ( my->sprite == 2193 || my->sprite == 2211 )
			{
				if ( parent )
				{
					my->x = parent->x;
					my->y = parent->y;
					my->z = parent->z - 0.5;
				}
				my->pitch += 0.1;
				my->roll = 0.0;
				my->scalex = 0.75;
				my->scaley = 0.75;
				my->scalez = 0.75;
				my->focalx = 0.0;
				my->focaly = 0.0;
				my->focalz = 0.5;

				if ( my->sprite == 2211 )
				{
					if ( multiplayer == CLIENT && my->skill[1] == PARTICLE_EFFECT_SCEPTER_BLAST_ORBIT1 )
					{
						my->scalex = 0.75;
						my->scaley = 0.75;
						my->scalez = 0.75;
					}
					else
					{
						my->scalex = 0.75 * std::min(50, (int)my->ticks) / 50.0;
						my->scaley = 0.75 * std::min(50, (int)my->ticks) / 50.0;
						my->scalez = 0.75 * std::min(50, (int)my->ticks) / 50.0;
					}

					if ( my->ticks % 8 == 0 )
					{
						if ( Entity* fx = spawnMagicParticleCustom(my, my->sprite, my->scalex, 50.0) )
						{
							fx->ditheringDisabled = true;
							fx->yaw = my->yaw;
							fx->pitch = my->pitch;
							fx->roll = my->roll;
							fx->focalx = my->focalx;
							fx->focaly = my->focaly;
							fx->focalz = my->focalz;
						}
					}
				}
				else 
				{
					if ( my->ticks % 8 == 0 )
					{
						if ( Entity* fx = spawnMagicParticleCustom(my, my->sprite, my->scalex, 10.0) )
						{
							fx->ditheringDisabled = true;
							fx->yaw = my->yaw;
							fx->pitch = my->pitch;
							fx->roll = my->roll;
							fx->focalx = my->focalx;
							fx->focaly = my->focaly;
							fx->focalz = my->focalz;
						}
					}
				}
			}
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_STASIS_RIFT_ORBIT )
		{
			Stat* stats = parent->getStats();
			if ( stats && !stats->getEffectActive(EFF_STASIS) )
			{
				my->removeLightField();
				list_RemoveNode(my->mynode);
				return;
			}

			my->yaw += 0.2;
			//spawnMagicParticle(my);
			my->x = parent->x + my->actmagicOrbitDist * cos(my->yaw);
			my->y = parent->y + my->actmagicOrbitDist * sin(my->yaw);
			my->z = parent->z;
			//my->z -= 0.1;
			//my->bNeedsRenderPositionInit = true;
			my->scalex = 1.0;
			my->scaley = my->scalex;
			my->scalez = my->scalex;

			if ( Entity* fx = spawnMagicParticle(my) )
			{
				fx->yaw += PI / 2;
				fx->scalex = my->scalex;
				fx->scaley = my->scaley;
				fx->scalez = my->scalez;
			}
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_THORNS_ORBIT )
		{
			Stat* stats = parent->getStats();
			if ( stats && !stats->getEffectActive(EFF_THORNS) )
			{
				my->removeLightField();
				list_RemoveNode(my->mynode);
				return;
			}

			my->x = parent->x + 4.0 * cos(my->yaw);
			my->y = parent->y + 4.0 * sin(my->yaw);

			//my->z -= 0.05;
			my->scalex -= 0.025;
			my->scaley = my->scalex;
			my->scalez = my->scalex;

			if ( my->scalex <= 0.0 )
			{
				my->removeLightField();
				list_RemoveNode(my->mynode);
				return;
			}

			/*if ( Entity* fx = spawnMagicParticle(my) )
			{
				fx->yaw += PI / 2;
				fx->scalex = my->scalex;
				fx->scaley = my->scaley;
				fx->scalez = my->scalez;
			}*/
		}
		--PARTICLE_LIFE;
	}
	return;
}

void actParticleTest(Entity* my)
{
	if ( PARTICLE_LIFE < 0 )
	{
		list_RemoveNode(my->mynode);
		return;
	}
	else
	{
		--PARTICLE_LIFE;
		my->x += my->vel_x;
		my->y += my->vel_y;
		my->z += my->vel_z;
		//my->z -= 0.01;
	}
}

void createParticleErupt(real_t x, real_t y, int sprite)
{
	real_t yaw = 0;
	int numParticles = 8;
	for ( int c = 0; c < 8; c++ )
	{
		Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Particle entity.
		entity->sizex = 1;
		entity->sizey = 1;
		entity->x = x;
		entity->y = y;
		entity->z = 7.5; // start from the ground.
		entity->yaw = yaw;
		entity->vel_x = 0.2;
		entity->vel_y = 0.2;
		entity->vel_z = -2;
		entity->skill[0] = 100;
		entity->skill[1] = 0; // direction.
		entity->fskill[0] = 0.1;
		entity->behavior = &actParticleErupt;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[UNCLICKABLE] = true;
		entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
			*cvar_magic_fx_light_bonus, 0.f);
		if ( multiplayer != CLIENT )
		{
			entity_uids--;
		}
		entity->setUID(-3);
		yaw += 2 * PI / numParticles;
	}
}

void createParticleErupt(Entity* parent, int sprite)
{
	if ( !parent )
	{
		return;
	}

	createParticleErupt(parent->x, parent->y, sprite);
}

Entity* createParticleSapCenter(Entity* parent, Entity* target, int spell, int sprite, int endSprite)
{
	if ( !parent || !target )
	{
		return nullptr;
	}
	// spawns the invisible 'center' of the magic particle
	Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Particle entity.
	entity->sizex = 1;
	entity->sizey = 1;
	entity->x = target->x;
	entity->y = target->y;
	entity->parent = (parent->getUID());
	entity->yaw = parent->yaw + PI; // face towards the caster.
	entity->skill[0] = 45;
	entity->skill[2] = -13; // so clients know my behavior.
	entity->skill[3] = 0; // init
	entity->skill[4] = sprite; // visible sprites.
	entity->skill[5] = endSprite; // sprite to spawn on return to caster.
	entity->skill[6] = spell;
	entity->behavior = &actParticleSapCenter;
	if ( target->sprite == 977 )
	{
		// boomerang.
		entity->yaw = target->yaw;
		entity->roll = target->roll;
		entity->pitch = target->pitch;
		entity->z = target->z;
	}
	entity->flags[INVISIBLE] = true;
	entity->flags[PASSABLE] = true;
	entity->flags[UPDATENEEDED] = true;
	entity->flags[UNCLICKABLE] = true;
	return entity;
}

void createParticleSap(Entity* parent)
{
	real_t speed = 0.4;
	if ( !parent )
	{
		return;
	}
	for ( int c = 0; c < 4; c++ )
	{
		// 4 particles, in an 'x' pattern around parent sprite.
		int sprite = parent->sprite;
		if ( parent->sprite == 977 )
		{
			if ( c > 0 )
			{
				continue;
			}
			// boomerang return.
			sprite = parent->sprite;
		}
		if ( parent->sprite == 2178 ) // testing particle
		{
			if ( c >= 2 )
			{
				continue;
			}
		}
		if ( parent->skill[6] == SPELL_STEAL_WEAPON || parent->skill[6] == SHADOW_SPELLCAST )
		{
			sprite = parent->sprite;
		}
		else if ( parent->skill[6] == SPELL_DRAIN_SOUL )
		{
			if ( c == 0 || c == 3 )
			{
				sprite = parent->sprite;
			}
			else
			{
				sprite = 599;
			}
		}
		else if ( parent->skill[6] == SPELL_SUMMON )
		{
			sprite = parent->sprite;
		}
		else if ( parent->skill[6] == SPELL_FEAR )
		{
			sprite = parent->sprite;
		}
		else if ( multiplayer == CLIENT )
		{
			// client won't receive the sprite skill data in time, fix for this until a solution is found!
			if ( sprite == 598 )
			{
				if ( c == 0 || c == 3 )
				{
				// drain HP particle
					sprite = parent->sprite;
				}
				else
				{
					// drain MP particle
					sprite = 599;
				}
			}
		}
		Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Particle entity.
		entity->sizex = 1;
		entity->sizey = 1;
		entity->x = parent->x;
		entity->y = parent->y;
		entity->z = 0;
		entity->scalex = 0.9;
		entity->scaley = 0.9;
		entity->scalez = 0.9;
		if ( sprite == 598 || sprite == 599 )
		{
			entity->scalex = 0.5;
			entity->scaley = 0.5;
			entity->scalez = 0.5;
		}
		if ( parent->sprite == 2178 )
		{
			entity->scalex = 0.5;
			entity->scaley = 0.5;
			entity->scalez = 0.5;
		}
		entity->parent = (parent->getUID());
		entity->yaw = parent->yaw;
		if ( c == 0 )
		{
			entity->vel_z = -speed;
			entity->vel_x = speed * cos(entity->yaw + PI / 2);
			entity->vel_y = speed * sin(entity->yaw + PI / 2);
			entity->yaw += PI / 3;
			entity->pitch -= PI / 6;
			entity->fskill[2] = -(PI / 3) / 25; // yaw rate of change.
			entity->fskill[3] = (PI / 6) / 25; // pitch rate of change.
		}
		else if ( c == 1 )
		{
			entity->vel_z = -speed;
			entity->vel_x = speed * cos(entity->yaw - PI / 2);
			entity->vel_y = speed * sin(entity->yaw - PI / 2);
			entity->yaw -= PI / 3;
			entity->pitch -= PI / 6;
			entity->fskill[2] = (PI / 3) / 25; // yaw rate of change.
			entity->fskill[3] = (PI / 6) / 25; // pitch rate of change.
		}
		else if ( c == 2 )
		{
			entity->vel_x = speed * cos(entity->yaw + PI / 2);
			entity->vel_y = speed * sin(entity->yaw + PI / 2);
			entity->vel_z = speed;
			entity->yaw += PI / 3;
			entity->pitch += PI / 6;
			entity->fskill[2] = -(PI / 3) / 25; // yaw rate of change.
			entity->fskill[3] = -(PI / 6) / 25; // pitch rate of change.
		}
		else if ( c == 3 )
		{
			entity->vel_x = speed * cos(entity->yaw - PI / 2);
			entity->vel_y = speed * sin(entity->yaw - PI / 2);
			entity->vel_z = speed;
			entity->yaw -= PI / 3;
			entity->pitch += PI / 6;
			entity->fskill[2] = (PI / 3) / 25; // yaw rate of change.
			entity->fskill[3] = -(PI / 6) / 25; // pitch rate of change.
		}

		entity->skill[3] = c; // particle index
		entity->fskill[0] = entity->vel_x; // stores the accumulated x offset from center
		entity->fskill[1] = entity->vel_y; // stores the accumulated y offset from center
		entity->skill[0] = 200; // lifetime
		entity->skill[1] = 0; // direction outwards
		entity->behavior = &actParticleSap;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		if ( multiplayer != CLIENT )
		{
			entity_uids--;
		}
		entity->setUID(-3);

		if ( sprite == 977 ) // boomerang
		{
			entity->z = parent->z;
			entity->scalex = 1.f;
			entity->scaley = 1.f;
			entity->scalez = 1.f;
			entity->skill[0] = 175;
			entity->fskill[2] = -((PI / 3) + (PI / 6)) / (150); // yaw rate of change over 3 seconds
			entity->fskill[3] = 0.f;
			entity->focalx = 2;
			entity->focalz = 0.5;
			entity->pitch = parent->pitch;
			entity->yaw = parent->yaw;
			entity->roll = parent->roll;

			entity->vel_x = 1 * cos(entity->yaw);
			entity->vel_y = 1 * sin(entity->yaw);
			int x = entity->x / 16;
			int y = entity->y / 16;
			if ( !map.tiles[(MAPLAYERS - 1) + y * MAPLAYERS + x * MAPLAYERS * map.height] )
			{
				// no ceiling, bounce higher.
				entity->vel_z = -0.4;
				entity->skill[3] = 1; // high bounce.
			}
			else
			{
				entity->vel_z = -0.08;
			}
			entity->yaw += PI / 3;
		}
		else
		{
			entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
				*cvar_magic_fx_light_bonus, 0.f);
		}
	}
}

void createParticleDropRising(Entity* parent, int sprite, double scale)
{
	if ( !parent )
	{
		return;
	}

	for ( int c = 0; c < 50; c++ )
	{
		// shoot drops to the sky
		Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Particle entity.
		entity->sizex = 1;
		entity->sizey = 1;
		entity->x = parent->x - 4 + local_rng.rand() % 9;
		entity->y = parent->y - 4 + local_rng.rand() % 9;
		entity->z = 7.5 + local_rng.rand() % 50;
		entity->vel_z = -1;
		//entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
		entity->particleDuration = 10 + local_rng.rand() % 50;
		entity->scalex *= scale;
		entity->scaley *= scale;
		entity->scalez *= scale;
		entity->behavior = &actParticleDot;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[UNCLICKABLE] = true;
		entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
			*cvar_magic_fx_light_bonus, 0.f);
		if ( multiplayer != CLIENT )
		{
			entity_uids--;
		}
		entity->setUID(-3);
	}
}

Entity* createParticleTimer(Entity* parent, int duration, int sprite)
{
	Entity* entity = newEntity(-1, 1, map.entities, nullptr); //Timer entity.
	entity->sizex = 1;
	entity->sizey = 1;
	if ( parent )
	{
		entity->x = parent->x;
		entity->y = parent->y;
		entity->parent = (parent->getUID());
	}
	entity->behavior = &actParticleTimer;
	entity->particleTimerDuration = duration;
	entity->flags[INVISIBLE] = true;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	/*if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);*/

	return entity;
}

void actParticleErupt(Entity* my)
{
	if ( PARTICLE_LIFE < 0 )
	{
		list_RemoveNode(my->mynode);
		return;
	}
	else
	{
		// particles jump up from the ground then back down again.
		--PARTICLE_LIFE;
		my->x += my->vel_x * cos(my->yaw);
		my->y += my->vel_y * sin(my->yaw);
		my->scalex *= 0.99;
		my->scaley *= 0.99;
		my->scalez *= 0.99;
		if ( *cvar_magic_fx_use_vismap && !intro )
		{
			int x = my->x / 16.0;
			int y = my->y / 16.0;
			if ( x >= 0 && x < map.width && y >= 0 && y < map.height )
			{
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( !client_disconnected[i] && players[i]->isLocalPlayer() && cameras[i].vismap[y + x * map.height] )
					{
						spawnMagicParticle(my);
						break;
					}
				}
			}
		}
		else
		{
			spawnMagicParticle(my);
		}
		if ( my->skill[1] == 0 ) // rising
		{
			my->z += my->vel_z;
			my->vel_z *= 0.8;
			my->pitch = std::min<real_t>(my->pitch + my->fskill[0], PI / 2);
			my->fskill[0] = std::max<real_t>(my->fskill[0] * 0.85, 0.05);
			if ( my->vel_z > -0.02 )
			{
				my->skill[1] = 1;
			}
		}
		else // falling
		{
			my->pitch = std::min<real_t>(my->pitch + my->fskill[0], 15 * PI / 16);
			my->fskill[0] = std::min<real_t>(my->fskill[0] * (1 / 0.99), 0.1);
			my->z -= my->vel_z;
			my->vel_z *= (1 / 0.8);
			my->vel_z = std::max<real_t>(my->vel_z, -0.8);
		}
	}
}

void floorMagicClientReceive(Entity* my)
{
	if ( !my ) { return; }
	if ( multiplayer != CLIENT ) { return; }

	if ( my->actfloorMagicClientReceived != 0 ) { return; }

	my->actfloorMagicType = (my->skill[2] >> 20) & 0xFF;
	/*if ( my->actfloorMagicType == ParticleTimerEffect_t::EFFECT_LIGHTNING_BOLT )
	{
		floorMagicSetLightningParticle(my);
	}*/
	if ( my->actfloorMagicType == ParticleTimerEffect_t::EFFECT_ICE_WAVE )
	{
		my->sizex = 4;
		my->sizey = 4;
		my->scalex = 0.25;
		my->scaley = 0.25;
		my->scalez = 0.25;
	}
	else if ( my->actfloorMagicType == ParticleTimerEffect_t::EFFECT_DISRUPT_EARTH )
	{
		my->sizex = 4;
		my->sizey = 4;
		my->scalex = 0.25;
		my->scaley = 0.25;
		my->scalez = 0.25;
		my->actmagicNoParticle = 1;
	}
	else if ( my->actfloorMagicType == ParticleTimerEffect_t::EFFECT_ROOTS_PATH
		|| my->actfloorMagicType == ParticleTimerEffect_t::EFFECT_ROOTS_SELF
		|| my->actfloorMagicType == ParticleTimerEffect_t::EFFECT_ROOTS_TILE_VOID
		|| my->actfloorMagicType == ParticleTimerEffect_t::EFFECT_ROOTS_SELF_SUSTAIN
		|| my->actfloorMagicType == ParticleTimerEffect_t::EFFECT_ROOTS_TILE )
	{
		my->scalex = 0.25;
		my->scaley = 0.25;
		my->scalez = 0.25;
		my->sizex = 6;
		my->sizey = 6;
		my->skill[0] = (my->skill[2] >> 8) & 0xFFF; // duration
		my->actmagicNoParticle = 1;
	}

	my->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
		*cvar_magic_fx_light_bonus, 0.f);

	my->actfloorMagicClientReceived = 1;
}

void floorMagicParticleSetUID(Entity& fx, bool noupdate)
{
	Sint32 val = (1 << 31);
	val |= (Uint8)(noupdate ? 21 : 20);
	val |= (((Uint16)(0) & 0xFFF) << 8);
	if ( fx.actfloorMagicType == ParticleTimerEffect_t::EFFECT_ROOTS_PATH
		|| fx.actfloorMagicType == ParticleTimerEffect_t::EFFECT_ROOTS_SELF
		|| fx.actfloorMagicType == ParticleTimerEffect_t::EFFECT_ROOTS_SELF_SUSTAIN
		|| fx.actfloorMagicType == ParticleTimerEffect_t::EFFECT_ROOTS_TILE )
	{
		val |= (((Uint16)(fx.skill[0]) & 0xFFF) << 8);
	}

	val |= (Uint8)(fx.actfloorMagicType & 0xFF) << 20;
	fx.skill[2] = val;
}

Entity* floorMagicCreateRoots(real_t x, real_t y, Entity* caster, int damage, int spellID, int duration, int particleTimerAction)
{
	int mapx = static_cast<int>(x) >> 4;
	int mapy = static_cast<int>(y) >> 4;
	int mapIndex = (mapy)*MAPLAYERS + (mapx) * MAPLAYERS * map.height;
	if ( mapx > 0 && mapy > 0 && mapx < map.width - 1 && mapy < map.height - 1 )
	{
		if ( !map.tiles[mapIndex] 
			|| swimmingtiles[map.tiles[mapIndex]]
				|| lavatiles[map.tiles[mapIndex]] )
		{
			return nullptr;
		}
	}
	else
	{
		return nullptr;
	}

	Entity* spellTimer = createParticleTimer(caster, duration + 10, -1);
	spellTimer->particleTimerCountdownAction = particleTimerAction;
	spellTimer->particleTimerCountdownSprite = -1;
	spellTimer->flags[UPDATENEEDED] = false; // no update clients
	spellTimer->flags[NOUPDATE] = true;
	if ( caster )
	{
		spellTimer->yaw = caster->yaw;
	}
	else
	{
		spellTimer->yaw = 0.0;
	}
	spellTimer->x = x;
	spellTimer->y = y;
	Sint32 val = (1 << 31);
	val |= (Uint8)(19);
	val |= (((Uint16)(spellTimer->particleTimerDuration) & 0xFFF) << 8);
	val |= (Uint8)(spellTimer->particleTimerCountdownAction & 0xFF) << 20;
	spellTimer->skill[2] = val;

	spellTimer->particleTimerVariable1 = damage;
	spellTimer->particleTimerVariable2 = spellID;
	return spellTimer;
}

void floorMagicCreateSpores(Entity* spawnOnEntity, real_t x, real_t y, Entity* caster, int damage, int spellID)
{
	if ( multiplayer == CLIENT )
	{
		return;
	}

	if ( spawnOnEntity )
	{
		x = static_cast<int>(spawnOnEntity->x / 16) * 16.0 + 8.0;
		y = static_cast<int>(spawnOnEntity->y / 16) * 16.0 + 8.0;
	}
	else
	{
		x = static_cast<int>(x / 16) * 16.0 + 8.0;
		y = static_cast<int>(y / 16) * 16.0 + 8.0;
	}

	int mapx = static_cast<int>(x) >> 4;
	int mapy = static_cast<int>(y) >> 4;

	int mapIndex = (mapy)*MAPLAYERS + (mapx)*MAPLAYERS * map.height;
	if ( mapx > 0 && mapy > 0 && mapx < map.width - 1 && mapy < map.height - 1 )
	{
		if ( map.tiles[OBSTACLELAYER + mapIndex] )
		{
			return;
		}
	}
	else
	{
		return;
	}
	bool freeSpot = true;
	if ( spellID == SPELL_SPORE_BOMB || spellID == SPELL_MYCELIUM_BOMB )
	{
		// allow overlap
	}
	else
	{
		auto entLists = TileEntityList.getEntitiesWithinRadius(mapx, mapy, 0);
		for ( auto it : entLists )
		{
			if ( !freeSpot )
			{
				break;
			}
			for ( node_t* node = it->first; node != nullptr; node = node->next )
			{
				if ( Entity* entity = (Entity*)node->element )
				{
					if ( entity->behavior == &actParticleTimer && entity->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_SPORES )
					{
						freeSpot = false;
						break;
					}
				}
			}
		}
	}

	if ( !freeSpot )
	{
		return;
	}

	Uint32 lifetime = TICKS_PER_SECOND * 6;
	Entity* spellTimer = createParticleTimer(caster, lifetime + TICKS_PER_SECOND, -1);
	spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SPORES;
	spellTimer->particleTimerCountdownSprite = (spellID == SPELL_MYCELIUM_BOMB || spellID == SPELL_MYCELIUM_SPORES) ? 248: 227;
	spellTimer->yaw = 0.0;
	spellTimer->x = x;
	spellTimer->y = y;
	spellTimer->particleTimerVariable1 = damage;
	spellTimer->particleTimerVariable2 = spellID;

	auto& timerEffects = particleTimerEffects[spellTimer->getUID()];

	std::vector<std::pair<int, int>> coords;
	std::map<int, std::vector<ParticleTimerEffect_t::EffectLocations_t>> effLocations;
	auto particleEffectType = (spellID == SPELL_MYCELIUM_BOMB || spellID == SPELL_MYCELIUM_SPORES) ? ParticleTimerEffect_t::EffectType::EFFECT_MYCELIUM
		: ParticleTimerEffect_t::EffectType::EFFECT_SPORES;
	for ( int i = -1; i < 2; ++i )
	{
		for ( int j = -1; j < 2; ++j )
		{
			coords.push_back(std::make_pair(i, j));
			effLocations[particleEffectType].push_back(ParticleTimerEffect_t::EffectLocations_t());
			auto& data = effLocations[particleEffectType].back();
			if ( spellID == SPELL_SPORE_BOMB || spellID == SPELL_MYCELIUM_BOMB )
			{
				data.seconds = 0.0;
			}
			else
			{
				data.seconds = 1 / 4.0;
			}
		}
	}

	int index = -1;
	Uint32 lifetime_tick = 1;
	while ( lifetime_tick <= lifetime )
	{
		++index;
		auto& effect = timerEffects.effectMap[lifetime_tick == 0 ? 1 : lifetime_tick]; // first behavior tick only occurs at 1
		effect.effectType = particleEffectType;
		if ( timerEffects.effectMap.size() == 1 )
		{
			effect.firstEffect = true;
		}

		auto& data = effLocations[effect.effectType][index];
		effect.sfx = data.sfx;

		int pick = local_rng.rand() % coords.size();
		auto coord = coords[pick];
		coords.erase(coords.begin() + pick);

		effect.x = spellTimer->x + coord.first * 16.0;
		effect.y = spellTimer->y + coord.second * 16.0;
		effect.yaw = 0.0;

		lifetime_tick += std::max(1.0, TICKS_PER_SECOND * data.seconds);
		if ( index + 1 >= effLocations[effect.effectType].size() )
		{
			break;
		}
	}

	if ( damage > 0 )
	{
		if ( spawnOnEntity )
		{
			if ( auto particleEmitterHitPropsTimer = getParticleEmitterHitProps(spellTimer->getUID(), spawnOnEntity) )
			{
				particleEmitterHitPropsTimer->hits++;
				particleEmitterHitPropsTimer->tick = ticks;
			}
		}

		int gibSprite = (spellID == SPELL_MYCELIUM_BOMB || spellID == SPELL_MYCELIUM_SPORES) ? 1886 : 1816;
		for ( int i = 0; i < 16; ++i )
		{
			Entity* gib = spawnGib(spellTimer);
			gib->sprite = gibSprite;
			gib->yaw = i * PI / 4 + (-2 + local_rng.rand() % 5) * PI / 64;
			gib->vel_x = 1.75 * cos(gib->yaw);
			gib->vel_y = 1.75 * sin(gib->yaw);
			gib->scalex = 0.5;
			gib->scaley = 0.5;
			gib->scalez = 0.5;
			gib->z = local_rng.uniform(8, spellTimer->z - 4);
			gib->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
				*cvar_magic_fx_light_bonus, 0.f);
		}

		serverSpawnMiscParticlesAtLocation(spellTimer->x, spellTimer->y, spellTimer->z, PARTICLE_EFFECT_SPORE_BOMB, gibSprite);
	}
}

void floorMagicCreateLightningSequence(Entity* spellTimer, int startTickOffset)
{
	if ( !spellTimer ) { return; }

	int lifetime_tick = startTickOffset;
	if ( lifetime_tick == 0 )
	{
		lifetime_tick = 100;// getSpellEffectDurationFromID(SPELL_LIGHTNING_BOLT, uidToEntity(spellTimer->parent), nullptr, spellTimer);
	}

	auto& timerEffects = particleTimerEffects[spellTimer->getUID()];
	std::map<int, std::vector<ParticleTimerEffect_t::EffectLocations_t>> effLocations;
	auto particleEffectType = ParticleTimerEffect_t::EffectType::EFFECT_LIGHTNING_BOLT;
	for ( int i = 0; i < 8; ++i )
	{
		effLocations[particleEffectType].push_back(ParticleTimerEffect_t::EffectLocations_t());
		auto& data = effLocations[particleEffectType].back();
		data.seconds = 1 / 10.0;
		data.dist = 1.0;
		data.xOffset = ((local_rng.rand() % 21) / 10.0) + -1.0;
		switch ( i % 4 )
		{
		case 0:
			data.yawOffset = 0 * (PI / 2);
			break;
		case 1:
			data.yawOffset = 2 * (PI / 2);
			break;
		case 2:
			data.yawOffset = 1 * (PI / 2);
			break;
		case 3:
			data.yawOffset = 3 * (PI / 2);
			break;
		default:
			data.yawOffset = i * (PI / 2);
			break;
		}

		if ( i == 0 )
		{
			data.sfx = 171;
		}
		else if ( i == 1 )
		{
			data.sfx = 517;
		}
	}

	int index = -1;
	Uint32 lifetime = spellTimer->particleTimerEffectLifetime > 0
		? std::min(spellTimer->particleTimerDuration, spellTimer->particleTimerEffectLifetime)
		: spellTimer->particleTimerDuration;
	while ( lifetime_tick <= lifetime )
	{
		++index;
		auto& effect = timerEffects.effectMap[lifetime_tick == 0 ? 1 : lifetime_tick]; // first behavior tick only occurs at 1
		effect.effectType = particleEffectType;
		if ( timerEffects.effectMap.size() == 1 )
		{
			effect.firstEffect = true;
		}

		auto& data = effLocations[effect.effectType][index];
		effect.sfx = data.sfx;
		effect.x = data.xOffset * cos(spellTimer->yaw + PI / 2);
		effect.y = data.xOffset * sin(spellTimer->yaw + PI / 2);
		effect.yaw = spellTimer->yaw + data.yawOffset;

		lifetime_tick += std::max(1.0, TICKS_PER_SECOND * data.seconds);
		if ( index + 1 >= effLocations[effect.effectType].size() )
		{
			break;
		}
	}
}

Entity* floorMagicSetLightningParticle(Entity* my)
{
	if ( !my ) { return nullptr; }
	my->scalex = 1.0;
	my->scaley = 1.0;
	my->scalez = 1.0;
	my->ditheringDisabled = true;
	my->actmagicNoParticle = 1;
	my->sizex = 10;
	my->sizey = 10;

	my->flags[NOUPDATE] = true;
	my->flags[UPDATENEEDED] = false;
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	my->setUID(-3);

	if ( multiplayer == CLIENT )
	{
		my->actfloorMagicClientReceived = 1;
	}

	if ( flickerLights )
	{
		my->light = addLight(my->x / 16, my->y / 16, "explosion");
	}
	else
	{
		Entity* parent = uidToEntity(my->parent);
		if ( parent )
		{
			if ( !parent->light )
			{
				parent->light = addLight(my->x / 16, my->y / 16, "explosion");
			}
		}
	}
	return my;
}

void actParticleTimer(Entity* my)
{
	if ( PARTICLE_LIFE < 0 )
	{
		if ( multiplayer != CLIENT )
		{
			if ( my->particleTimerEndAction == PARTICLE_EFFECT_INCUBUS_TELEPORT_STEAL )
			{
				// teleport to random location spell.
				Entity* parent = uidToEntity(my->parent);
				if ( parent )
				{
					createParticleErupt(parent, my->particleTimerEndSprite);

					int x1 = 0;
					int x2 = 0;
					int y1 = 0;
					int y2 = 0;
					if ( Stat* stats = parent->getStats() )
					{
						if ( stats->type == INCUBUS && stats->getAttribute("special_npc") == "johann" )
						{
							if ( !strcmp(map.filename, "fraternity.lmp") )
							{
								x1 = 17;
								y1 = 1;
								x2 = 30;
								y2 = 12;
							}
						}
					}
					if ( parent->teleportRandom(x1, x2, y1, y2) )
					{
						// teleport success.
						if ( multiplayer == SERVER )
						{
							serverSpawnMiscParticles(parent, PARTICLE_EFFECT_ERUPT, my->particleTimerEndSprite);
						}
					}
				}
			}
			else if ( my->particleTimerEndAction == PARTICLE_EFFECT_INCUBUS_TELEPORT_TARGET )
			{
				// teleport to target spell.
				Entity* parent = uidToEntity(my->parent);
				Entity* target = uidToEntity(static_cast<Uint32>(my->particleTimerTarget));
				if ( parent && target )
				{
					createParticleErupt(parent, my->particleTimerEndSprite);
					if ( parent->teleportAroundEntity(target, my->particleTimerVariable1) )
					{
						// teleport success.
						if ( multiplayer == SERVER )
						{
							serverSpawnMiscParticles(parent, PARTICLE_EFFECT_ERUPT, my->particleTimerEndSprite);
						}
					}
				}
			}
			else if ( my->particleTimerEndAction == PARTICLE_EFFECT_TELEPORT_PULL )
			{
				// teleport to target spell.
				Entity* parent = uidToEntity(my->parent);
				Entity* target = uidToEntity(static_cast<Uint32>(my->particleTimerTarget));
				if ( parent && target )
				{
					real_t oldx = target->x;
					real_t oldy = target->y;
					my->flags[PASSABLE] = true;
					int tx = static_cast<int>(std::floor(my->x)) >> 4;
					int ty = static_cast<int>(std::floor(my->y)) >> 4;
					if ( !target->isBossMonster() &&
						target->teleport(tx, ty) )
					{
						// teleport success.
						if ( parent->behavior == &actPlayer )
						{
							Uint32 color = makeColorRGB(0, 255, 0);
							if ( target->getStats() )
							{
								messagePlayerMonsterEvent(parent->skill[2], color, *(target->getStats()), Language::get(3450), Language::get(3451), MSG_COMBAT);
							}
						}
						if ( target->behavior == &actPlayer )
						{
							Uint32 color = makeColorRGB(255, 255, 255);
							messagePlayerColor(target->skill[2], MESSAGE_COMBAT, color, Language::get(3461));
						}
						real_t distance =  sqrt((target->x - oldx) * (target->x - oldx) + (target->y - oldy) * (target->y - oldy)) / 16.f;
						//real_t distance = (entityDist(parent, target)) / 16;
						createParticleErupt(target, my->particleTimerEndSprite);
						int durationToStun = 0;
						if ( distance >= 2 )
						{
							durationToStun = 25 + std::min((distance - 4) * 10, 50.0);
						}
						if ( target->behavior == &actMonster )
						{
							if ( durationToStun > 0 && target->setEffect(EFF_DISORIENTED, true, durationToStun, false) )
							{
								int numSprites = std::min(3, durationToStun / 25);
								for ( int i = 0; i < numSprites; ++i )
								{
									spawnFloatingSpriteMisc(134, target->x + (-4 + local_rng.rand() % 9) + cos(target->yaw) * 2, 
										target->y + (-4 + local_rng.rand() % 9) + sin(target->yaw) * 2, target->z + local_rng.rand() % 4);
								}
							}
							target->monsterReleaseAttackTarget();
							target->lookAtEntity(*parent);
							target->monsterLookDir += (PI - PI / 4 + (local_rng.rand() % 10) * PI / 40);
						}
						else if ( target->behavior == &actPlayer )
						{
							durationToStun = std::max(50, durationToStun);
							target->setEffect(EFF_DISORIENTED, true, durationToStun, false);
							int numSprites = std::min(3, durationToStun / 50);
							for ( int i = 0; i < numSprites; ++i )
							{
								spawnFloatingSpriteMisc(134, target->x + (-4 + local_rng.rand() % 9) + cos(target->yaw) * 2,
									target->y + (-4 + local_rng.rand() % 9) + sin(target->yaw) * 2, target->z + local_rng.rand() % 4);
							}
							Uint32 color = makeColorRGB(255, 255, 255);
							messagePlayerColor(target->skill[2], MESSAGE_COMBAT, color, Language::get(3462));
						}
						if ( multiplayer == SERVER )
						{
							serverSpawnMiscParticles(target, PARTICLE_EFFECT_ERUPT, my->particleTimerEndSprite);
						}
					}
				}
			}
			else if ( my->particleTimerEndAction == PARTICLE_EFFECT_PORTAL_SPAWN )
			{
				Entity* parent = uidToEntity(my->parent);
				if ( parent )
				{
					parent->flags[INVISIBLE] = false;
					serverUpdateEntityFlag(parent, INVISIBLE);
					playSoundEntity(parent, 164, 128);
				}
				spawnExplosion(my->x, my->y, 0);
			}
			else if ( my->particleTimerEndAction == PARTICLE_EFFECT_SUMMON_MONSTER 
				|| my->particleTimerEndAction == PARTICLE_EFFECT_DEVIL_SUMMON_MONSTER )
			{
				playSoundEntity(my, 164, 128);
				spawnExplosion(my->x, my->y, -4.0);
				bool forceLocation = false;
				if ( my->particleTimerEndAction == PARTICLE_EFFECT_DEVIL_SUMMON_MONSTER &&
					!map.tiles[static_cast<int>(my->y / 16) * MAPLAYERS + static_cast<int>(my->x / 16) * MAPLAYERS * map.height] )
				{
					if ( my->particleTimerVariable1 == SHADOW || my->particleTimerVariable1 == CREATURE_IMP )
					{
						forceLocation = true;
					}
				}
				Entity* monster = summonMonster(static_cast<Monster>(my->particleTimerVariable1), my->x, my->y, forceLocation);
				if ( monster )
				{
					if ( Stat* monsterStats = monster->getStats() )
					{
						if ( my->parent != 0 )
						{
							Entity* parent = uidToEntity(my->parent);
							if ( parent )
							{
								if ( parent->getRace() == LICH_ICE )
								{
									//monsterStats->leader_uid = my->parent;
									switch ( monsterStats->type )
									{
										case AUTOMATON:
											strcpy(monsterStats->name, "corrupted automaton");
											monster->setEffect(EFF_CONFUSED, Uint8(MAXPLAYERS + 1), -1, true, true, true, true);
											break;
										default:
											break;
									}

									if ( Stat* parentStats = parent->getStats() )
									{
										monsterStats->monsterNoDropItems = 1;
										monsterStats->MISC_FLAGS[STAT_FLAG_DISABLE_MINIBOSS] = 1;
										monsterStats->LVL = 5;
										std::string lich_num_summons = parentStats->getAttribute("lich_num_summons");
										if ( lich_num_summons == "" )
										{
											parentStats->setAttribute("lich_num_summons", "1");
										}
										else
										{
											int numSummons = std::stoi(parentStats->getAttribute("lich_num_summons"));
											if ( numSummons >= 25 )
											{
												monsterStats->MISC_FLAGS[STAT_FLAG_XP_PERCENT_AWARD] = 1;
											}
											++numSummons;
											parentStats->setAttribute("lich_num_summons", std::to_string(numSummons));
										}
									}
								}
								else if ( parent->getRace() == DEVIL )
								{
									monsterStats->monsterNoDropItems = 1;
									monsterStats->MISC_FLAGS[STAT_FLAG_DISABLE_MINIBOSS] = 1;
									monsterStats->LVL = 5;
									if ( parent->monsterDevilNumSummons >= 25 )
									{
										monsterStats->MISC_FLAGS[STAT_FLAG_XP_PERCENT_AWARD] = 1;
									}
									if ( my->particleTimerVariable2 >= 0 
										&& players[my->particleTimerVariable2] && players[my->particleTimerVariable2]->entity )
									{
										monster->monsterAcquireAttackTarget(*(players[my->particleTimerVariable2]->entity), MONSTER_STATE_ATTACK);
									}
								}
							}
						}
					}
				}
			}
			else if ( my->particleTimerEndAction == PARTICLE_EFFECT_SPELL_SUMMON )
			{
				//my->removeLightField();
			}
			else if ( my->particleTimerEndAction == PARTICLE_EFFECT_SHADOW_TELEPORT )
			{
				// teleport to target spell.
				Entity* parent = uidToEntity(my->parent);
				if ( parent )
				{
					if ( parent->monsterSpecialState == SHADOW_TELEPORT_ONLY )
					{
						//messagePlayer(0, "Resetting shadow's monsterSpecialState!");
						parent->monsterSpecialState = 0;
						serverUpdateEntitySkill(parent, 33); // for clients to keep track of animation
					}
				}
				Entity* target = uidToEntity(static_cast<Uint32>(my->particleTimerTarget));
				if ( parent )
				{
					bool teleported = false;
					createParticleErupt(parent, my->particleTimerEndSprite);
					if ( target )
					{
						teleported = parent->teleportAroundEntity(target, my->particleTimerVariable1);
					}
					else
					{
						teleported = parent->teleportRandom();
					}

					if ( teleported )
					{
						// teleport success.
						if ( multiplayer == SERVER )
						{
							serverSpawnMiscParticles(parent, PARTICLE_EFFECT_ERUPT, my->particleTimerEndSprite);
						}
					}
				}
			}
			else if ( my->particleTimerEndAction == PARTICLE_EFFECT_LICHFIRE_TELEPORT_STATIONARY )
			{
				// teleport to fixed location spell.
				node_t* node;
				int c = 0 + local_rng.rand() % 3;
				Entity* target = nullptr;
				for ( node = map.entities->first; node != nullptr; node = node->next )
				{
					target = (Entity*)node->element;
					if ( target->behavior == &actDevilTeleport )
					{
						if ( (c == 0 && target->sprite == 72)
							|| (c == 1 && target->sprite == 73)
							|| (c == 2 && target->sprite == 74) )
						{
							break;
						}
					}
				}
				Entity* parent = uidToEntity(my->parent);
				if ( parent && target )
				{
					createParticleErupt(parent, my->particleTimerEndSprite);
					if ( parent->teleport(target->x / 16, target->y / 16) )
					{
						// teleport success.
						if ( multiplayer == SERVER )
						{
							serverSpawnMiscParticles(parent, PARTICLE_EFFECT_ERUPT, my->particleTimerEndSprite);
						}
					}
				}
			}
			else if ( my->particleTimerEndAction == PARTICLE_EFFECT_LICH_TELEPORT_ROAMING )
			{
				bool teleported = false;
				// teleport to target spell.
				node_t* node;
				Entity* parent = uidToEntity(my->parent);
				Entity* target = nullptr;
				if ( parent )
				{
					for ( node = map.entities->first; node != nullptr; node = node->next )
					{
						target = (Entity*)node->element;
						if ( target->behavior == &actDevilTeleport
							&& target->sprite == 128 )
						{
								break; // found specified center of map
						}
					}

					if ( target )
					{
						createParticleErupt(parent, my->particleTimerEndSprite);
						teleported = parent->teleport((target->x / 16) - 11 + local_rng.rand() % 23, (target->y / 16) - 11 + local_rng.rand() % 23);

						if ( teleported )
						{
							// teleport success.
							if ( multiplayer == SERVER )
							{
								serverSpawnMiscParticles(parent, PARTICLE_EFFECT_ERUPT, my->particleTimerEndSprite);
							}
						}
					}
				}
			}
			else if ( my->particleTimerEndAction == PARTICLE_EFFECT_LICHICE_TELEPORT_STATIONARY )
			{
				// teleport to fixed location spell.
				node_t* node;
				Entity* target = nullptr;
				for ( node = map.entities->first; node != nullptr; node = node->next )
				{
					target = (Entity*)node->element;
					if ( target->behavior == &actDevilTeleport
						&& target->sprite == 128 )
					{
							break;
					}
				}
				Entity* parent = uidToEntity(my->parent);
				if ( parent && target )
				{
					createParticleErupt(parent, my->particleTimerEndSprite);
					if ( parent->teleport(target->x / 16, target->y / 16) )
					{
						// teleport success.
						if ( multiplayer == SERVER )
						{
							serverSpawnMiscParticles(parent, PARTICLE_EFFECT_ERUPT, my->particleTimerEndSprite);
						}
						parent->lichIceCreateCannon();
					}
				}
			}
			else if ( my->particleTimerEndAction == PARTICLE_EFFECT_SHRINE_TELEPORT )
			{
				// teleport to target spell.
				Entity* toTeleport = uidToEntity(my->particleTimerVariable2);
				Entity* target = uidToEntity(static_cast<Uint32>(my->particleTimerTarget));
				if ( toTeleport && target )
				{
					bool teleported = false;
					createParticleErupt(toTeleport, my->particleTimerEndSprite);
					serverSpawnMiscParticles(toTeleport, PARTICLE_EFFECT_ERUPT, my->particleTimerEndSprite);
					teleported = toTeleport->teleportAroundEntity(target, my->particleTimerVariable1);
					if ( teleported )
					{
						createParticleErupt(toTeleport, my->particleTimerEndSprite);
						// teleport success.
						if ( multiplayer == SERVER )
						{
							serverSpawnMiscParticles(toTeleport, PARTICLE_EFFECT_ERUPT, my->particleTimerEndSprite);
						}
					}
				}
			}
			else if ( my->particleTimerEndAction == PARTICLE_EFFECT_DESTINY_TELEPORT )
			{
				// teleport to target spell.
				Entity* toTeleport = uidToEntity(my->particleTimerVariable2);
				Entity* target = uidToEntity(static_cast<Uint32>(my->particleTimerTarget));
				if ( toTeleport && target )
				{
					bool teleported = false;
					createParticleErupt(toTeleport, my->particleTimerEndSprite);
					serverSpawnMiscParticles(toTeleport, PARTICLE_EFFECT_ERUPT, my->particleTimerEndSprite);
					teleported = toTeleport->teleportAroundEntity(target, my->particleTimerVariable1);
					if ( teleported )
					{
						createParticleErupt(toTeleport, my->particleTimerEndSprite);
						// teleport success.
						if ( multiplayer == SERVER )
						{
							serverSpawnMiscParticles(toTeleport, PARTICLE_EFFECT_ERUPT, my->particleTimerEndSprite);
						}
					}
				}
			}
			else if ( my->particleTimerEndAction == PARTICLE_EFFECT_GHOST_TELEPORT )
			{
				// teleport to target spell.
				if ( Entity* parent = uidToEntity(my->parent) )
				{
					if ( my->particleTimerTarget != 0 )
					{
						if ( Entity* target = uidToEntity(static_cast<Uint32>(my->particleTimerTarget)) )
						{
							bool teleported = false;
							createParticleErupt(parent, my->particleTimerEndSprite);
							serverSpawnMiscParticles(parent, PARTICLE_EFFECT_ERUPT, my->particleTimerEndSprite);
							teleported = parent->teleportAroundEntity(target, my->particleTimerVariable1);
							if ( teleported )
							{
								createParticleErupt(parent, my->particleTimerEndSprite);
								// teleport success.
								if ( multiplayer == SERVER )
								{
									serverSpawnMiscParticles(parent, PARTICLE_EFFECT_ERUPT, my->particleTimerEndSprite);
								}

								if ( parent->behavior == &actDeathGhost && target->behavior == &actPlayer
									&& parent->skill[2] == target->skill[2] )
								{
									if ( stats[target->skill[2]]->getEffectActive(EFF_PROJECT_SPIRIT) )
									{
										stats[target->skill[2]]->EFFECTS_TIMERS[EFF_PROJECT_SPIRIT] = 1;
									}
								}
							}
						}
					}
					else
					{
						int tx = (my->particleTimerVariable2 >> 16) & 0xFFFF;
						int ty = (my->particleTimerVariable2 >> 0) & 0xFFFF;
						int dist = my->particleTimerVariable1;
						bool forceSpot = false;
						std::vector<std::pair<int, int>> goodspots;
						for ( int iy = std::max(1, ty - dist); !forceSpot && iy <= std::min(ty + dist, static_cast<int>(map.height) - 1); ++iy )
						{
							for ( int ix = std::max(1, tx - dist); !forceSpot && ix <= std::min(tx + dist, static_cast<int>(map.width) - 1); ++ix )
							{
								if ( !checkObstacle((ix << 4) + 8, (iy << 4) + 8, parent, NULL) )
								{
									real_t tmpx = parent->x;
									real_t tmpy = parent->y;
									parent->x = (ix << 4) + 8;
									parent->y = (iy << 4) + 8;
									if ( !entityInsideSomething(parent) )
									{
										if ( ix == tx && iy == ty )
										{
											forceSpot = true; // directly ontop
											goodspots.clear();
										}
										goodspots.push_back(std::make_pair(ix, iy));
									}
									// restore coordinates.
									parent->x = tmpx;
									parent->y = tmpy;
								}
							}
						}

						if ( !goodspots.empty() )
						{
							auto picked = goodspots.at(goodspots.size() - 1);
							bool teleported = false;
							createParticleErupt(parent, my->particleTimerEndSprite);
							serverSpawnMiscParticles(parent, PARTICLE_EFFECT_ERUPT, my->particleTimerEndSprite);
							teleported = parent->teleport(picked.first, picked.second);
							if ( teleported )
							{
								createParticleErupt(parent, my->particleTimerEndSprite);
								// teleport success.
								if ( multiplayer == SERVER )
								{
									serverSpawnMiscParticles(parent, PARTICLE_EFFECT_ERUPT, my->particleTimerEndSprite);
								}
							}
						}
					}

				}
			}
		}

		particleTimerEffects.erase(my->getUID());
		my->removeLightField();
		list_RemoveNode(my->mynode);
		return;
	}
	else
	{
		--PARTICLE_LIFE;
		if ( my->particleTimerPreDelay <= 0 )
		{
			// shoot particles for the duration of the timer, centered at caster.
			if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_SHOOT_PARTICLES )
			{
				Entity* parent = uidToEntity(my->parent);
				// shoot drops to the sky
				if ( parent && my->particleTimerCountdownSprite != 0 )
				{
					Entity* entity = newEntity(my->particleTimerCountdownSprite, 1, map.entities, nullptr); //Particle entity.
					entity->sizex = 1;
					entity->sizey = 1;
					entity->x = parent->x - 4 + local_rng.rand() % 9;
					entity->y = parent->y - 4 + local_rng.rand() % 9;
					entity->z = 7.5;
					entity->vel_z = -1;
					entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
					entity->particleDuration = 10 + local_rng.rand() % 30;
					entity->behavior = &actParticleDot;
					entity->flags[PASSABLE] = true;
					entity->flags[NOUPDATE] = true;
					entity->flags[UNCLICKABLE] = true;
					entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
						*cvar_magic_fx_light_bonus, 0.f);
					if ( multiplayer != CLIENT )
					{
						entity_uids--;
					}
					entity->setUID(-3);
				}
			}
			// fire once off.
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_SPAWN_PORTAL )
			{
				Entity* parent = uidToEntity(my->parent);
				if ( parent && my->particleTimerCountdownAction < 100 )
				{
					playSoundEntityLocal(parent, 167, 128);
					createParticleDot(parent);
					createParticleCircling(parent, 100, my->particleTimerCountdownSprite);
					my->particleTimerCountdownAction = 0;
				}
			}
			// fire once off.
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_SUMMON_MONSTER )
			{
				if ( my->particleTimerCountdownAction < 100 )
				{
					if ( !my->light )
					{
						my->light = addLight(my->x / 16, my->y / 16, magicLightColorForSprite(my, my->sprite, true));
					}
					playSoundEntityLocal(my, 167, 128);
					createParticleDropRising(my, 680, 1.0);
					createParticleCircling(my, 70, my->particleTimerCountdownSprite);
					my->particleTimerCountdownAction = 0;
				}
			}
			// fire once off.
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_DEVIL_SUMMON_MONSTER )
			{
				if ( my->particleTimerCountdownAction < 100 )
				{
					if ( !my->light )
					{
						my->light = addLight(my->x / 16, my->y / 16, magicLightColorForSprite(my, my->sprite, true));
					}
					playSoundEntityLocal(my, 167, 128);
					createParticleDropRising(my, 593, 1.0);
					createParticleCircling(my, 70, my->particleTimerCountdownSprite);
					my->particleTimerCountdownAction = 0;
				}
			}
			// continually fire
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_SPELL_SUMMON )
			{
				if ( multiplayer != CLIENT && my->particleTimerPreDelay != -100 )
				{
					// once-off hack :)
					spawnExplosion(my->x, my->y, -1);
					playSoundEntity(my, 171, 128);
					my->particleTimerPreDelay = -100;

					createParticleErupt(my, my->particleTimerCountdownSprite);
					serverSpawnMiscParticles(my, PARTICLE_EFFECT_ERUPT, my->particleTimerCountdownSprite);
				}
			}
			// fire once off.
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_TELEPORT_PULL_TARGET_LOCATION )
			{
				createParticleDropRising(my, my->particleTimerCountdownSprite, 1.0);
				my->particleTimerCountdownAction = 0;
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_MAGIC_SPRAY )
			{
				Entity* parent = uidToEntity(my->parent);
				if ( !parent )
				{
					PARTICLE_LIFE = 0;
				}
				else
				{
					int sound = 0;
					int spellID = SPELL_NONE;
					switch ( my->particleTimerCountdownSprite )
					{
					case 180:
						sound = 169;
						spellID = SPELL_SLIME_ACID;
						break;
					case 181:
						sound = 169;
						spellID = SPELL_SLIME_WATER;
						break;
					case 182:
						sound = 164;
						spellID = SPELL_SLIME_FIRE;
						break;
					case 183:
						sound = 169;
						spellID = SPELL_SLIME_TAR;
						break;
					case 184:
						sound = 169;
						spellID = SPELL_SLIME_METAL;
						break;
					case 245:
						sound = 169;
						spellID = SPELL_GREASE_SPRAY;
						break;
					default:
						break;
					}
					if ( my->particleTimerVariable1 == 0 )
					{
						// first fired after delay
						my->particleTimerVariable1 = 1;
						if ( sound > 0 )
						{
							playSoundEntityLocal(parent, sound, 128);
						}
					}

					my->x = parent->x;
					my->y = parent->y;
					my->yaw = parent->yaw;

					Entity* entity = nullptr;
					if ( multiplayer != CLIENT && my->ticks % 2 == 0 )
					{
						// damage frames
						entity = newEntity(my->particleTimerCountdownSprite, 1, map.entities, nullptr);
						entity->behavior = &actMagicMissile;
					}
					else
					{
						entity = multiplayer == CLIENT ? spawnGibClient(0, 0, 0, -1) : spawnGib(my);
					}
					if ( entity )
					{
						entity->sprite = my->particleTimerCountdownSprite;
						entity->x = parent->x;
						entity->y = parent->y;
						if ( parent->behavior == &actMonster && parent->getMonsterTypeFromSprite() == SLIME )
						{
							entity->z = -2 + parent->z + parent->focalz;
						}
						else
						{
							entity->z = parent->z;
						}
						entity->parent = parent->getUID();

						entity->ditheringDisabled = true;
						entity->flags[SPRITE] = true;
						entity->flags[INVISIBLE] = false;
						entity->flags[PASSABLE] = true;
						entity->flags[NOUPDATE] = true;
						entity->flags[UNCLICKABLE] = true;
						entity->flags[BRIGHT] = true;

						entity->sizex = 2;
						entity->sizey = 2;
						entity->yaw = my->yaw - 0.2 + (local_rng.rand() % 20) * 0.02;
						entity->pitch = (local_rng.rand() % 360) * PI / 180.0;
						entity->roll = (local_rng.rand() % 360) * PI / 180.0;
						double vel = (20 + (local_rng.rand() % 5)) / 10.f;
						entity->vel_x = vel * cos(entity->yaw) + parent->vel_x;
						entity->vel_y = vel * sin(entity->yaw) + parent->vel_y;
						entity->vel_z = -.5;

						if ( entity->behavior == &actGib && spellID == SPELL_GREASE_SPRAY )
						{
							if ( multiplayer != CLIENT )
							{
								if ( my->ticks % 5 == 0 )
								{
									entity->actGibHitGroundEvent = 1;
								}
							}
						}
						if ( entity->behavior == &actGib )
						{
							if ( my->ticks % 5 == 0 )
							{
								// add lighting
								entity->skill[6] = 1;
							}
						}
						else if ( entity->behavior == &actMagicMissile )
						{
							spell_t* spell = getSpellFromID(spellID);
							entity->skill[4] = 0; // life start
							entity->skill[5] = TICKS_PER_SECOND; //lifetime
							entity->actmagicSpray = 1;
							entity->actmagicSprayGravity = 0.04;
							entity->actmagicEmitter = my->getUID();
							node_t* node = list_AddNodeFirst(&entity->children);
							node->element = copySpell(spell);
							((spell_t*)node->element)->caster = parent->getUID();
							node_t* elementNode = ((spell_t*)node->element)->elements.first;
							spellElement_t* element = (spellElement_t*)elementNode->element;
							{
								elementNode = element->elements.first;
								element = (spellElement_t*)elementNode->element;
								if ( spellID == SPELL_GREASE_SPRAY )
								{
								}
								else
								{
									if ( Stat* stats = parent->getStats() )
									{
										element->setDamage(element->getDamage() + stats->getProficiency(PRO_SORCERY) / 10);
									}
								}
								((spell_t*)node->element)->mana = 5;
							}

							node->deconstructor = &spellDeconstructor;
							node->size = sizeof(spell_t);
							TileEntityList.addEntity(*entity);
							--entity_uids;
							entity->setUID(-3);
						}
					}
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_FOCI_SPRAY )
			{
				Entity* parent = uidToEntity(my->parent);
				if ( !parent )
				{
					PARTICLE_LIFE = 0;
				}
				else
				{
					if ( my->particleTimerVariable3 <= 0 )
					{
						// done with particles
						PARTICLE_LIFE = 0;
					}
					else if ( my->particleTimerVariable1 == 0 )
					{
						my->particleTimerVariable1 = 10; // refire rate
						--my->particleTimerVariable3;
						spell_t* spell = getSpellFromID(my->particleTimerVariable2);
						if ( spell )
						{
							if ( spell_t* newSpell = copySpell(spell) )
							{
								if ( newSpell->elements.first )
								{
									if ( spellElement_t* element = (spellElement_t*)(newSpell->elements.first->element) )
									{
										// rename the propulsion
										auto find = spellElementMap.find(SPELL_ELEMENT_PROPULSION_FOCI_SPRAY);
										if ( find != spellElementMap.end() )
										{
											strcpy(element->element_internal_name, find->second.element_internal_name);
										}
									}
								}
								Entity* missile = castSpell(parent->getUID(), newSpell, true, false);
								spellDeconstructor(newSpell);
							}
						}
					}
					else
					{
						my->particleTimerVariable1--;
					}
				}
			}
			//else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_FOCI_SPRAY )
			//{
			//	Entity* parent = uidToEntity(my->parent);
			//	if ( !parent )
			//	{
			//		PARTICLE_LIFE = 0;
			//	}
			//	else
			//	{
			//		int sound = 164;
			//		int spellID = SPELL_FOCI_FIRE;
			//		if ( my->particleTimerVariable1 == 0 )
			//		{
			//			// first fired after delay
			//			my->particleTimerVariable1 = 1;
			//			if ( sound > 0 )
			//			{
			//				playSoundEntityLocal(parent, sound, 128);
			//			}
			//		}

			//		my->x = parent->x;
			//		my->y = parent->y;
			//		my->yaw = parent->yaw;

			//		Entity* entity = nullptr;
			//		if ( multiplayer != CLIENT && my->ticks % 2 == 0 && false )
			//		{
			//			// damage frames
			//			entity = newEntity(my->particleTimerCountdownSprite, 1, map.entities, nullptr);
			//			entity->behavior = &actMagicMissile;
			//		}
			//		else if ( my->ticks % 10 == 2 )
			//		{
			//			//entity = multiplayer == CLIENT ? spawnGibClient(0, 0, 0, -1) : spawnGib(my);
			//			real_t velocityBonus = 0.0;
			//			{
			//				real_t velocityDir = atan2(parent->vel_y, parent->vel_x);
			//				real_t casterDir = fmod(my->yaw, 2 * PI);
			//				real_t yawDiff = velocityDir - casterDir;
			//				while ( yawDiff > PI )
			//				{
			//					yawDiff -= 2 * PI;
			//				}
			//				while ( yawDiff <= -PI )
			//				{
			//					yawDiff += 2 * PI;
			//				}
			//				if ( abs(yawDiff) <= PI )
			//				{
			//					real_t vel = sqrt(pow(parent->vel_x, 2) + pow(parent->vel_y, 2));
			//					velocityBonus = std::max(0.0, cos(yawDiff) * vel);
			//				}
			//			}
			//			entity = spawnFociGib(parent->x, parent->y, parent->z + 2, parent->yaw, velocityBonus, parent->getUID(), my->particleTimerCountdownSprite, local_rng.rand());

			//			/*Entity* fx = createParticleAestheticOrbit(parent, 16, 50, PARTICLE_EFFECT_FOCI_ORBIT);
			//			fx->scalex = 0.5;
			//			fx->scaley = 0.5;
			//			fx->scalez = 0.5;
			//			fx->flags[SPRITE] = true;
			//			fx->x = parent->x + 8.0 * cos(parent->yaw);
			//			fx->y = parent->y + 8.0 * sin(parent->yaw);
			//			fx->yaw = parent->yaw;
			//			fx->fskill[0] = (PI / 4) + (local_rng.rand() % 2) * 3 * PI / 2;
			//			fx->z = 0.0;*/
			//			//fx->actmagicOrbitDist = 4;
			//		}
			//	}
			//}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_IGNITE )
			{
				my->removeLightField();
				if ( Entity* caster = uidToEntity(my->parent) )
				{
					my->x = caster->x;
					my->y = caster->y;
					if ( PARTICLE_LIFE <= 5 )
					{
						my->light = addLight(my->x / 16, my->y / 16, "explosion");
						if ( PARTICLE_LIFE == 5 )
						{
							for ( int i = 0; i < 4; ++i )
							{
								if ( Entity* fx = createParticleAestheticOrbit(caster, 233, 25, PARTICLE_EFFECT_IGNITE_ORBIT) )
								{
									fx->flags[SPRITE] = true;
									fx->fskill[2] = (i / 8.0) * 2 * PI;
									fx->fskill[3] += (local_rng.rand() % 10) * PI / 10.0;
									fx->z = 7.5;
									fx->vel_z = -0.1 + (local_rng.rand() % 10) * -.025;
									fx->actmagicOrbitDist = 30;
								}
							}

							for ( int i = 0; i < 8; ++i )
							{
								if ( Entity* fx = createParticleAestheticOrbit(caster, 233, 25, PARTICLE_EFFECT_IGNITE_ORBIT) )
								{
									fx->flags[SPRITE] = true;
									fx->fskill[2] = (i / 8.0) * 2 * PI + PI / 8;
									fx->fskill[3] += (local_rng.rand() % 10) * PI / 10.0;
									fx->z = 7.5;
									fx->vel_z = -0.1 + (local_rng.rand() % 10) * -.025;
									fx->actmagicOrbitDist = 16;
								}
							}

							if ( multiplayer != CLIENT )
							{
								int numTargets = 0;
								std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(caster, 3);
								for ( auto it : entLists )
								{
									node_t* node;
									for ( node = it->first; node != nullptr; node = node->next )
									{
										Entity* entity = (Entity*)node->element;
										if ( !entity->flags[BURNABLE] || entity->flags[BURNING] )
										{
											continue;
										}
										if ( entityDist(caster, entity) > 32.0 + 4.0 )
										{
											continue;
										}
										Stat* stats = (entity->behavior == &actMonster || entity->behavior == &actPlayer) ? entity->getStats() : nullptr;
										if ( stats )
										{
											if ( caster && caster->getStats() )
											{
												if ( caster == entity ) { continue; }

												//if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
												{
													if ( caster->checkFriend(entity) && caster->friendlyFireProtection(entity) )
													{
														continue;
													}
												}
											}
											if ( entity->monsterIsTargetable() )
											{
												//if ( caster->checkEnemy(entity) )
												{
													if ( entity->SetEntityOnFire(caster) )
													{
														applyGenericMagicDamage(caster, entity, *caster, SPELL_IGNITE, 0, true);
														stats->burningInflictedBy = caster->getUID();
														if ( caster )
														{
															entity->char_fire = std::min(entity->char_fire, getSpellEffectDurationFromID(SPELL_IGNITE, caster, nullptr, my));
														}
													}
												}
											}
										}
										else
										{
											if ( entity->behavior == &actDoor
												|| entity->behavior == &::actIronDoor
												|| entity->behavior == &actBell
												|| entity->behavior == &actGreasePuddleSpawner
												|| entity->behavior == &::actFurniture || entity->behavior == &::actChest
												|| (entity->isDamageableCollider() && entity->isColliderDamageableByMagic()) )
											{
												if ( entity->SetEntityOnFire(caster) )
												{
													++numTargets;
												}
											}
										}
									}
								}

								if ( numTargets > 0 )
								{
									while ( numTargets > 0 )
									{
										--numTargets;
										magicOnSpellCastEvent(caster, caster, nullptr, SPELL_IGNITE, spell_t::SPELL_LEVEL_EVENT_DEFAULT, 1);
									}
								}
							}
						}
					}
					else
					{
						my->light = addLight(my->x / 16, my->y / 16, "campfire");
					}
				}
				else
				{
					PARTICLE_LIFE = 0;
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_VORTEX )
			{
				my->removeLightField();
				if ( my->particleTimerVariable1 == 0 )
				{
					//Entity* fx = createParticleCastingIndicator(my, my->x, my->y, my->z, PARTICLE_LIFE, 0);
					//fx->skill[11] = my->getUID();

					for ( int i = 0; i < 3; ++i )
					{
						static ConsoleVariable<int> cvar_magic_vortex_sprite("/magic_vortex_sprite", 1719);
						static ConsoleVariable<float> cvar_magic_vortex_scale("/magic_vortex_scale", 0.5);
						Entity* fx = createParticleAestheticOrbit(my, *cvar_magic_vortex_sprite, PARTICLE_LIFE, PARTICLE_EFFECT_VORTEX_ORBIT);
						fx->scalex = *cvar_magic_vortex_scale;
						fx->scaley = *cvar_magic_vortex_scale;
						fx->scalez = *cvar_magic_vortex_scale;
						fx->yaw += i * 2 * PI / 3;
						fx->z = 7.5;
						fx->actmagicOrbitDist = 4;
					}

					auto poof = spawnPoof(my->x, my->y, 6, 0.5);
					my->particleTimerVariable1 = 1;
					my->entity_sound = playSoundEntity(my, 757, 128);
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

				Entity* parent = nullptr;
				if ( multiplayer != CLIENT )
				{
					parent = uidToEntity(my->parent);
					if ( PARTICLE_LIFE == 1 )
					{
						auto poof = spawnPoof(my->x, my->y, 6, 0.5, true);
						playSoundEntity(my, 512, 128);
					}

					if ( parent && parent->behavior == &actLeafPile )
					{
						my->x = parent->x;
						my->y = parent->y;
					}
					else
					{
						my->vel_x *= .95;
						my->vel_y *= .95;
						my->flags[NOCLIP_CREATURES] = true;
						if ( abs(my->vel_x) > 0.01 || abs(my->vel_y) > 0.01 )
						{
							clipMove(&my->x, &my->y, my->vel_x, my->vel_y, my);
						}
					}
				}

				static ConsoleVariable<int> cvar_vortex_particle_interval("/vortex_particle_interval", 30);
				static ConsoleVariable<float> cvar_vortex_particle_decay("/vortex_particle_decay", 0.9);
				if ( my->particleTimerVariable1 == 1 || my->particleTimerVariable1 % *cvar_vortex_particle_interval == 0 )
				{
					real_t offset = PI * (local_rng.rand() % 360) / 180.0;// -((my->ticks % 50) / 50.0) * 2 * PI;
					int lifetime = PARTICLE_LIFE / 10;

					constexpr auto color = makeColor(255, 255, 255, 255);
					for ( int i = 0; i < 24; ++i )
					{
						if ( Entity* fx = createParticleAOEIndicator(my, my->x, my->y, -7.5, TICKS_PER_SECOND * 5, 16 + (i / 2) * 2) )
						{
							fx->yaw = my->yaw + PI / 2 - (i / 2) * PI / 3;
							fx->pitch += PI / 32;
							if ( i % 2 == 1 )
							{
								fx->pitch += PI;
							}
							fx->z = 8.0;
							fx->z -= (i / 2) * 0.5;
							fx->vel_z -= 0.25;
							fx->fskill[0] = 0.3; // rotate
							fx->scalex = 0.5;// + (i / 2) * 0.25 / 12;
							fx->scaley = 0.5;// + (i / 2) * 0.25 / 12;
							fx->flags[ENTITY_SKIP_CULLING] = false;
							if ( auto indicator = AOEIndicators_t::getIndicator(fx->skill[10]) )
							{
								indicator->expireAlphaRate = *cvar_vortex_particle_decay;
								indicator->cacheType = AOEIndicators_t::CACHE_VORTEX;
								indicator->arc = PI / 4;
								indicator->indicatorColor = color;
								indicator->loop = false;
								indicator->framesPerTick = 1;
								indicator->ticksPerUpdate = 1;
								indicator->delayTicks = 0;
							}
						}
					}
				}
				++my->particleTimerVariable1;

				if ( multiplayer != CLIENT )
				{
					{
						//my->x = parent->x;
						//my->y = parent->y;

						std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 1);
						for ( auto it : entLists )
						{
							if ( PARTICLE_LIFE <= 0 )
							{
								break;
							}
							node_t* node;
							for ( node = it->first; node != nullptr; node = node->next )
							{
								Entity* entity = (Entity*)node->element;
								if ( !(entity->behavior == &actPlayer || entity->behavior == &actMonster) )
								{
									continue;
								}
								if ( !entity->monsterIsTargetable() )
								{
									continue;
								}
								if ( entityDist(my, entity) > 8.0 )
								{
									continue;
								}

								Stat* stats = entity->getStats();
								if ( !stats ) { continue; }

								if ( PARTICLE_LIFE == 1 )
								{
									if ( Uint8 effectStrength = stats->getEffectActive(EFF_LIFT) )
									{
										if ( isLevitating(stats) && !(stats->type == MINOTAUR) )
										{
											continue;
										}

										// check for neighbouring vortexes
										bool foundAnother = false;
										for ( auto it : entLists )
										{
											for ( node_t* node = it->first; node != nullptr; node = node->next )
											{
												if ( Entity* entity2 = (Entity*)node->element )
												{
													if ( entity2->behavior == &actParticleTimer
														&& entity2->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_VORTEX )
													{
														if ( entityDist(entity2, entity) <= 8.0 )
														{
															if ( entity2->skill[0] > 1 ) // still has life
															{
																auto props = getParticleEmitterHitProps(entity2->getUID(), entity);
																if ( props->hits > 0 )
																{
																	foundAnother = true;
																	break;
																}
															}
														}
													}
												}
												if ( foundAnother )
												{
													break;
												}
											}
											if ( foundAnother )
											{
												break;
											}
										}

										if ( foundAnother )
										{
											continue;
										}

										stats->EFFECTS_TIMERS[EFF_LIFT] = 1;
										if ( stats->getEffectActive(EFF_ROOTED) )
										{
											stats->EFFECTS_TIMERS[EFF_ROOTED] = 1;
										}

										auto poof = spawnPoof(entity->x, entity->y, 4, 1.0, true);

										if ( effectStrength >= 3 )
										{
											createParticleRock(entity, 78);
											playSoundEntity(entity, 181, 128);
											if ( multiplayer == SERVER )
											{
												serverSpawnMiscParticles(entity, PARTICLE_EFFECT_ABILITY_ROCK, 78);
											}

											int damage = getSpellDamageFromID(my->particleTimerVariable2, parent ? parent : my, nullptr, my);
											int perStatDmg = getSpellDamageSecondaryFromID(my->particleTimerVariable2, parent ? parent : my, nullptr, my);
											real_t perStatMult = getSpellEffectDurationSecondaryFromID(my->particleTimerVariable2, parent ? parent : my, nullptr, my) / 100.0;
											perStatDmg *= (statGetSTR(stats, entity) + statGetCON(stats, entity)) * perStatMult;
											damage += perStatDmg;
											real_t mult = 0.1 * (std::min(10, effectStrength - 3));
											damage *= mult;
											if ( applyGenericMagicDamage(parent, entity, parent ? *parent : *my, SPELL_SLAM, damage, true) )
											{
												messagePlayerColor(entity->isEntityPlayer(), MESSAGE_STATUS, 
													makeColorRGB(255, 0, 0), Language::get(6758));
												if ( parent && parent->behavior == &actLeafPile )
												{
													stats->killer = KilledBy::LEAVES;
													entity->setObituary(Language::get(6760));
												}
											}
										}
									}
									continue;
								}

								if ( parent && parent->behavior == &actMonster )
								{
									if ( parent == entity || parent->checkFriend(entity) )
									{
										continue;
									}
								}
								if ( parent && parent->behavior == &actPlayer )
								{
									if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
									{
										if ( parent->checkFriend(entity) && parent->friendlyFireProtection(entity) )
										{
											continue;
										}
									}
								}

								auto props = getParticleEmitterHitProps(my->getUID(), entity);
								if ( !props )
								{
									continue;
								}
								if ( props->hits > 0 && (ticks - props->tick) < 20 )
								{
									continue;
								}
								Uint8 strength = std::min(13, 1 + stats->getEffectActive(EFF_LIFT));
								if ( entity->setEffect(EFF_LIFT, strength, std::max(5, PARTICLE_LIFE + 21), true) )
								{
									my->vel_x = 0.0;
									my->vel_y = 0.0;

									if ( parent && parent->behavior == &actLeafPile )
									{
										parent->vel_x = 0.0;
										parent->vel_y = 0.0;
									}

									entity->setEffect(EFF_ROOTED, strength, std::max(5, PARTICLE_LIFE), false);
									if ( strength == 1 )
									{
										messagePlayer(entity->isEntityPlayer(), MESSAGE_STATUS, Language::get(6757));
										auto poof = spawnPoof(entity->x, entity->y, 4, 0.5, true);
										playSoundEntity(entity, 178, 128);
									}
								}
								props->hits++;
								props->tick = ticks;
							}
						}
					}
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_SHATTER )
			{
				my->removeLightField();
				if ( Entity* caster = uidToEntity(my->parent) )
				{
					my->x = caster->x;
					my->y = caster->y;
					if ( PARTICLE_LIFE == 5 )
					{
						my->light = addLight(my->x / 16, my->y / 16, "magic_purple");
						if ( PARTICLE_LIFE == 5 )
						{
							for ( int i = 0; i < 16; ++i )
							{
								if ( Entity* fx = createParticleAestheticOrbit(caster, 261, 25, PARTICLE_EFFECT_IGNITE_ORBIT) )
								{
									fx->flags[SPRITE] = true;
									fx->fskill[2] = (i / 16.0) * 2 * PI;
									fx->fskill[3] += (local_rng.rand() % 10) * PI / 10.0;
									fx->z = 7.5;
									fx->vel_z = -0.1 + (local_rng.rand() % 10) * -.025;
									fx->actmagicOrbitDist = 30;
								}
							}

							if ( multiplayer != CLIENT )
							{
								int numTargets = 0;
								std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(caster, 3);
								for ( auto it : entLists )
								{
									node_t* node;
									for ( node = it->first; node != nullptr; node = node->next )
									{
										Entity* entity = (Entity*)node->element;
										if ( entityDist(caster, entity) > 32.0 + 4.0 )
										{
											continue;
										}
										Stat* stats = (entity->behavior == &actMonster || entity->behavior == &actPlayer) ? entity->getStats() : nullptr;
										if ( stats )
										{
											if ( !(stats->type == MIMIC || stats->type == MINIMIMIC) )
											{
												continue;
											}
											//if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
											{
												if ( caster->checkFriend(entity) && caster->friendlyFireProtection(entity) )
												{
													continue;
												}
											}
										}

										if ( entity->behavior == &actDoor
											|| entity->behavior == &::actIronDoor
											|| entity->behavior == &::actChest
											|| entity->behavior == &actMonster
											|| (entity->isDamageableCollider() && entity->isColliderDamageableByMagic())
											|| entity->behavior == &::actFurniture )
										{
											int damage = getSpellDamageFromID(SPELL_SHATTER_OBJECTS, caster, nullptr, my);
											if ( applyGenericMagicDamage(caster, entity, *caster, SPELL_SHATTER_OBJECTS, damage, true) )
											{
												if ( entity->behavior != &::actIronDoor )
												{
													++numTargets;
												}
											}
										}
									}
								}

								if ( numTargets > 0 )
								{
									while ( numTargets > 0 )
									{
										--numTargets;
										magicOnSpellCastEvent(caster, caster, nullptr, SPELL_SHATTER_OBJECTS, spell_t::SPELL_LEVEL_EVENT_DEFAULT, 1);
									}
								}
							}
						}
					}
					else
					{
						my->light = addLight(my->x / 16, my->y / 16, "magic_purple_flicker");
					}
				}
				else
				{
					PARTICLE_LIFE = 0;
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_DAMAGE_LOS_AREA )
			{
				if ( PARTICLE_LIFE == 1 || PARTICLE_LIFE == 3 ) // double tap for enemies inside crates etc
				{
					Entity* caster = uidToEntity(my->parent);
					doSpellExplosionArea(my->particleTimerVariable2, my, caster, my->x, my->y, my->z, my->particleTimerVariable3);
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_SWEEP_ATTACK )
			{
				Entity* parent = uidToEntity(my->parent);
				Stat* parentStats = parent ? parent->getStats() : nullptr;
				if ( parent && parentStats && ((parent->behavior == &actPlayer ? parent->skill[9] : parent->monsterAttack) != 0) )
				{
					std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadius(parent->x / 16, parent->y / 16, 2);
					for ( auto it : entLists )
					{
						node_t* node;
						for ( node = it->first; node != nullptr; node = node->next )
						{
							Entity* entity = (Entity*)node->element;
							if ( entity == parent )
							{
								continue;
							}
							/*if ( !entity->getStats() )
							{
								continue;
							}*/
							if ( !(entity->behavior == &actMonster
								|| entity->behavior == &actPlayer
								|| (entity->isDamageableCollider() && entity->isColliderDamageableByMelee())
								|| entity->behavior == &actDoor
								|| entity->behavior == &actFurniture
								|| entity->behavior == &::actChest
								|| entity->behavior == &::actIronDoor) )
							{
								continue;
							}

							if ( parent && parent->getStats() )
							{
								//if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
								{
									if ( parent->checkFriend(entity) && parent->friendlyFireProtection(entity) )
									{
										continue;
									}
								}
							}

							auto hitProps = getParticleEmitterHitProps(my->getUID(), entity);
							if ( !hitProps )
							{
								continue;
							}
							if ( hitProps->hits > 0 )
							{
								continue;
							}
							if ( entity->getStats() )
							{
								if ( !entity->monsterIsTargetable() ) { continue; }
							}

							if ( entityDist(parent, entity) > STRIKERANGE + 8.0 ) { continue; }

							real_t tangent = atan2(entity->y - parent->y, entity->x - parent->x);
							real_t angle = parent->yaw - tangent;
							while ( angle >= PI )
							{
								angle -= PI * 2;
							}
							while ( angle < -PI )
							{
								angle += PI * 2;
							}
							if ( abs(angle) > 1 * PI / 3 ) { continue; }
							bool oldPassable = entity->flags[PASSABLE];
							entity->flags[PASSABLE] = false;
							real_t d = lineTraceTarget(parent, parent->x, parent->y, tangent, STRIKERANGE, LINETRACE_ATK_CHECK_FRIENDLYFIRE
								| LINETRACE_TELEKINESIS, false, entity);
							entity->flags[PASSABLE] = oldPassable;
							if ( hit.entity != entity )
							{
								continue;
							}

							{
								Sint32 prevAtk = parent->monsterAttack;
								Sint32 prevAtkTime = parent->monsterAttackTime;
								parent->attack(MONSTER_POSE_SWEEP_ATTACK_NO_UPDATE, Stat::getMaxAttackCharge(parent->getStats()), entity);
								parent->monsterAttack = prevAtk;
								parent->monsterAttackTime = prevAtkTime;

								++hitProps->hits;
								hitProps->tick = ticks;
							}
						}
					}
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_SPIRIT_WEAPON_ATTACK )
			{
				Entity* parent = uidToEntity(my->parent);
				Stat* parentStats = parent ? parent->getStats() : nullptr;
				if ( parent && parentStats && parent->monsterAttack != 0 && parentStats->getEffectActive(EFF_KNOCKBACK) )
				{
					std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadius(parent->x / 16, parent->y / 16, 1);
					for ( auto it : entLists )
					{
						node_t* node;
						for ( node = it->first; node != nullptr; node = node->next )
						{
							Entity* entity = (Entity*)node->element;
							if ( entity == parent )
							{
								continue;
							}
							/*if ( !entity->getStats() )
							{
								continue;
							}*/

							if ( !(entity->behavior == &actMonster
								|| entity->behavior == &actPlayer
								|| (entity->isDamageableCollider() && entity->isColliderDamageableByMelee())
								|| entity->behavior == &actDoor
								|| entity->behavior == &actFurniture
								|| entity->behavior == &::actChest
								|| entity->behavior == &::actIronDoor) )
							{
								continue;
							}

							if ( !entityInsideEntity(parent, entity) )
							{
								continue;
							}
							if ( parent && parent->getStats() )
							{
								//if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
								{
									if ( parent->checkFriend(entity) && parent->friendlyFireProtection(entity) )
									{
										continue;
									}
								}
							}

							auto hitProps = getParticleEmitterHitProps(my->getUID(), entity);
							if ( !hitProps )
							{
								continue;
							}
							if ( hitProps->hits > 0 )
							{
								continue;
							}

							if ( entity->getStats() )
							{
								if ( !entity->monsterIsTargetable() ) { continue; }
							}

							real_t tangent = atan2(entity->y - parent->y, entity->x - parent->x);
							bool oldPassable = entity->flags[PASSABLE];
							entity->flags[PASSABLE] = false;
							real_t d = lineTraceTarget(parent, parent->x, parent->y, tangent, 8.0, 0, false, entity);
							entity->flags[PASSABLE] = oldPassable;
							if ( hit.entity != entity )
							{
								continue;
							}

							{
								Sint32 prevAtk = parent->monsterAttack;
								Sint32 prevAtkTime = parent->monsterAttackTime;
								parent->attack(0, 0, entity);
								parent->monsterAttack = prevAtk;
								parent->monsterAttackTime = prevAtkTime;

								++hitProps->hits;
								hitProps->tick = ticks;
							}
						}
					}
				}
				else
				{
					PARTICLE_LIFE = 0;
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_EARTH_ELEMENTAL_ROLL )
			{
				Entity* parent = uidToEntity(my->parent);
				Stat* parentStats = parent ? parent->getStats() : nullptr;
				if ( parent && parentStats && parent->monsterAttack == MONSTER_POSE_EARTH_ELEMENTAL_ROLL && parentStats->getEffectActive(EFF_KNOCKBACK) )
				{
					std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadius(parent->x / 16, parent->y / 16, 1);
					for ( auto it : entLists )
					{
						node_t* node;
						for ( node = it->first; node != nullptr; node = node->next )
						{
							Entity* entity = (Entity*)node->element;
							if ( entity == parent )
							{
								continue;
							}
							if ( !entity->getStats() )
							{
								continue;
							}
							if ( !entityInsideEntity(parent, entity) )
							{
								continue;
							}
							if ( parent && parent->getStats() )
							{
								if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
								{
									if ( parent->checkFriend(entity) && parent->friendlyFireProtection(entity) )
									{
										continue;
									}
								}
							}

							auto hitProps = getParticleEmitterHitProps(my->getUID(), entity);
							if ( !hitProps )
							{
								continue;
							}
							if ( hitProps->hits > 0 )
							{
								continue;
							}
							if ( !entity->monsterIsTargetable() ) { continue; }

							real_t tangent = atan2(entity->y - parent->y, entity->x - parent->x);
							bool oldPassable = entity->flags[PASSABLE];
							entity->flags[PASSABLE] = false;
							real_t d = lineTraceTarget(parent, parent->x, parent->y, tangent, 8.0, 0, false, entity);
							entity->flags[PASSABLE] = oldPassable;
							if ( hit.entity != entity )
							{
								continue;
							}

							{
								int damage = std::max(10, statGetCON(parentStats, parent));
								if ( entity->getStats() )
								{
									if ( applyGenericMagicDamage(parent, entity, *parent, SPELL_NONE, damage, true, true) )
									{
										++hitProps->hits;
										hitProps->tick = ticks;
									}
								}
							}
						}
					}
				}
				else
				{
					PARTICLE_LIFE = 0;
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_BOOBY_TRAP )
			{
				if ( !my->light )
				{
					my->light = addLight(my->x / 16, my->y / 16, "magic_purple");
				}
				if ( PARTICLE_LIFE == 7 )
				{
					if ( multiplayer != CLIENT )
					{
						Entity* target = uidToEntity(my->particleTimerTarget);
						if ( target )
						{
							Entity* caster = uidToEntity(my->parent);
							if ( !caster )
							{
								caster = my;
							}

							if ( target->behavior == &actDoor )
							{
								target->doorHandleDamageMagic(target->doorHealth, *my, caster);
								magicOnSpellCastEvent(caster, caster, target, SPELL_BOOBY_TRAP, spell_t::SPELL_LEVEL_EVENT_DEFAULT, 1);
							}
							else if ( target->behavior == &::actChest || target->getMonsterTypeFromSprite() == MIMIC )
							{
								target->chestHandleDamageMagic(my->particleTimerVariable1, *my, caster);
								magicOnSpellCastEvent(caster, caster, target, SPELL_BOOBY_TRAP, spell_t::SPELL_LEVEL_EVENT_DEFAULT, 1);
							}
							else if ( target->isDamageableCollider() )
							{
								target->colliderHandleDamageMagic(target->colliderCurrentHP, *my, caster);
								magicOnSpellCastEvent(caster, caster, target, SPELL_BOOBY_TRAP, spell_t::SPELL_LEVEL_EVENT_DEFAULT, 1);
							}
							else if ( target->behavior == &::actFurniture )
							{
								target->furnitureHandleDamageMagic(target->furnitureHealth, *my, caster);
								magicOnSpellCastEvent(caster, caster, target, SPELL_BOOBY_TRAP, spell_t::SPELL_LEVEL_EVENT_DEFAULT, 1);
							}
						}
					}
				}
				if ( PARTICLE_LIFE == 5 )
				{
					if ( multiplayer != CLIENT )
					{
						spawnExplosion(my->x, my->y, my->z);
						Entity* caster = uidToEntity(my->parent);
						if ( !caster )
						{
							caster = my;
						}

						Entity* target = uidToEntity(my->particleTimerTarget);

						std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 2);
						for ( auto it : entLists )
						{
							node_t* node;
							for ( node = it->first; node != nullptr; node = node->next )
							{
								Entity* entity = (Entity*)node->element;
								if ( entityDist(my, entity) > 32.0 + 4.0 )
								{
									continue;
								}
								if ( entity == target || entity == my || entity == caster )
								{
									continue;
								}
								bool mimic = entity->isInertMimic();
								if ( caster && caster->getStats() )
								{
									//if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
									{
										if ( caster->checkFriend(entity) && caster->friendlyFireProtection(entity) )
										{
											continue;
										}
									}
								}

								real_t tangent = atan2(entity->y - my->y, entity->x - my->x);
								bool oldPassable = entity->flags[PASSABLE];
								if ( entity->behavior == &actGreasePuddleSpawner )
								{
									entity->flags[PASSABLE] = false;
								}
								real_t d = lineTraceTarget(my, my->x, my->y, tangent, 32.0, 0, false, entity);
								entity->flags[PASSABLE] = oldPassable;
								if ( hit.entity != entity )
								{
									continue;
								}

								int damage = my->particleTimerVariable1;
								if ( (entity->behavior == &actMonster && !mimic) || entity->behavior == &actPlayer )
								{
									if ( !entity->monsterIsTargetable() ) { continue; }
									if ( Stat* stats = entity->getStats() )
									{
										if ( stats->getEffectActive(EFF_MAGIC_GREASE) )
										{
											damage *= 2;
										}
										applyGenericMagicDamage(caster, entity, *caster, SPELL_NONE, damage, true);
										if ( entity->SetEntityOnFire(caster) )
										{
											if ( caster )
											{
												entity->char_fire = std::min(entity->char_fire, getSpellEffectDurationFromID(SPELL_BOOBY_TRAP, caster, nullptr, my));
											}
										}
										if ( caster )
										{
											stats->burningInflictedBy = caster->getUID();
										}
									}
								}
								else if ( entity->behavior == &actGreasePuddleSpawner )
								{
									entity->SetEntityOnFire(caster);
								}
								else
								{
									if ( applyGenericMagicDamage(caster, entity, *caster, SPELL_NONE, damage, true) )
									{
										entity->SetEntityOnFire(caster);
									}
								}
							}
						}
					}
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_LIGHTNING )
			{
				if ( my->ticks == 1 )
				{
					//createParticleCastingIndicator(my, my->x, my->y, my->z, PARTICLE_LIFE, my->getUID());
					for ( int i = 0; i < 3; ++i )
					{
						Entity* fx = createParticleAestheticOrbit(my, 1758, 2 * TICKS_PER_SECOND, PARTICLE_EFFECT_STATIC_ORBIT);
						//fx->x = my->x + 40.0/*dist * (data.dist)*/ * cos(my->yaw);
						//fx->y = my->y + 40.0/*dist * (data.dist)*/ * sin(my->yaw);
						fx->z = 7.5 - 2.0 * i;
						fx->scalex = 1.0;
						fx->scaley = 1.0;
						fx->scalez = 1.0;
						fx->actmagicOrbitDist = 20;
						fx->yaw += i * 2 * PI / 3;
						if ( i != 0 )
						{
							fx->actmagicNoLight = 1;
						}
					}

					if ( multiplayer != CLIENT )
					{
						playSoundEntity(my, 806, 128);
					}
				}

				if ( multiplayer != CLIENT )
				{
					Entity* parent = uidToEntity(my->parent);
					Entity* closestEntity = nullptr;
					if ( my->actmagicOrbitHitTargetUID1 != 0 )
					{
						closestEntity = uidToEntity(my->actmagicOrbitHitTargetUID1);
						if ( !closestEntity )
						{
							my->actmagicOrbitHitTargetUID1 = 0;
						}
					}
					if ( my->actmagicOrbitHitTargetUID1 == 0 && my->ticks < 100 )
					{
						std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 2);
						real_t dist = 10000.0;
						Uint8 hadStaticEffect = 0;
						for ( auto it : entLists )
						{
							for ( node_t* node = it->first; node != nullptr; node = node->next )
							{
								Entity* entity = (Entity*)node->element;
								if ( parent && entity == parent )
								{
									continue;
								}
								if ( entity->behavior == &actMonster || entity->behavior == &actPlayer )
								{
									if ( parent && parent->checkFriend(entity) )
									{
										continue;
									}
									if ( !entity->monsterIsTargetable() ) { continue; }
									real_t newDist = entityDist(my, entity);
									Uint8 effectStrength = entity->getStats() ? entity->getStats()->getEffectActive(EFF_STATIC) : 0;
									if ( ((newDist < dist) || (effectStrength >= hadStaticEffect)) && newDist < 64.0 )
									{
										real_t tangent = atan2(entity->y - my->y, entity->x - my->x);
										real_t d = lineTraceTarget(my, my->x, my->y, tangent, 64.0, 0, false, entity);
										if ( hit.entity == entity )
										{
											if ( effectStrength > hadStaticEffect )
											{
												if ( effectStrength > hadStaticEffect )
												{
													closestEntity = entity;
													dist = newDist;
													hadStaticEffect = effectStrength;
												}
												else if ( effectStrength == hadStaticEffect )
												{
													if ( newDist < dist )
													{
														closestEntity = entity;
														dist = newDist;
														hadStaticEffect = effectStrength;
													}
												}
											}
											else if ( effectStrength == hadStaticEffect )
											{
												if ( newDist < dist )
												{
													closestEntity = entity;
													dist = newDist;
												}
											}
										}
									}
								}
							}
						}

						if ( closestEntity )
						{
							if ( dist < 8.0 )
							{
								my->actmagicOrbitHitTargetUID1 = closestEntity->getUID();
							}
						}
					}

					my->vel_x = 0.0;
					my->vel_y = 0.0;
					if ( closestEntity )
					{
						real_t dist = entityDist(my, closestEntity);
						real_t tangent = atan2(closestEntity->y - my->y, closestEntity->x - my->x);
						real_t speedMult = 100.0 - 0.5 * std::min((Uint32)100, my->ticks); // build up faster to impact
						real_t staticEffectSpeedMult = 0.0;
						if ( Stat* targetStats = closestEntity->getStats() )
						{
							staticEffectSpeedMult += (0.1 + std::min(1.6, targetStats->getEffectActive(EFF_STATIC) * 0.3));
						}
						if ( my->ticks >= 100 ) // first impact
						{
							if ( my->ticks >= (100 - 5 * staticEffectSpeedMult) )
							{
								if ( my->actmagicOrbitHitTargetUID2 != 0 )
								{
									if ( dist < 16.0 )
									{
										my->actmagicOrbitHitTargetUID2 = closestEntity->getUID(); // follow this target
									}
								}
							}

							Entity* followTarget = my->actmagicOrbitHitTargetUID2 != 0 ? uidToEntity(my->actmagicOrbitHitTargetUID2) : nullptr;
							if ( followTarget )
							{
								my->x = followTarget->x;
								my->y = followTarget->y;
							}
							else
							{
								speedMult = 100.0;
								real_t speed = std::min(dist, std::max(16.0, 64.0 - dist) / speedMult);
								my->vel_x = speed * cos(tangent) * staticEffectSpeedMult;
								my->vel_y = speed * sin(tangent) * staticEffectSpeedMult;
							}
						}
						else
						{
							real_t speed = std::min(dist, std::max(16.0, 64.0 - dist) / speedMult);
							my->vel_x = speed * cos(tangent) * staticEffectSpeedMult;
							my->vel_y = speed * sin(tangent) * staticEffectSpeedMult;
						}
					}

					my->x += my->vel_x;
					my->y += my->vel_y;
				}
				auto findEffects = particleTimerEffects.find(my->getUID());
				if ( findEffects != particleTimerEffects.end() )
				{
					auto findEffect = findEffects->second.effectMap.find(my->ticks);
					if ( findEffect != findEffects->second.effectMap.end() )
					{
						auto& data = findEffect->second;
						if ( data.effectType == ParticleTimerEffect_t::EFFECT_LIGHTNING_BOLT )
						{
							if ( data.firstEffect )
							{
								serverSpawnMiscParticles(my, PARTICLE_EFFECT_LIGHTNING_SEQ, 0);
								if ( Entity* fx = createParticleAOEIndicator(my, my->x, my->y, 0.0, TICKS_PER_SECOND, 32.0) )
								{
									//fx->actSpriteFollowUID = 0;
									fx->actSpriteCheckParentExists = 0;
									fx->scalex = 0.8;
									fx->scaley = 0.8;
									if ( auto indicator = AOEIndicators_t::getIndicator(fx->skill[10]) )
									{
										//indicator->arc = PI / 2;
										Uint32 color = makeColorRGB(252, 224, 0);
										indicator->indicatorColor = color;
										indicator->loop = false;
										indicator->gradient = 4;
										indicator->framesPerTick = 2;
										indicator->ticksPerUpdate = 1;
										indicator->delayTicks = 0;
										indicator->expireAlphaRate = 0.95;
									}
								}
							}
							Entity* fx = createFloorMagic(data.effectType, 1757, my->x + data.x, my->y + data.y, -8.5, data.yaw, TICKS_PER_SECOND / 8);
							fx->parent = my->getUID();

							fx->actmagicOrbitHitTargetUID1 = my->actmagicOrbitHitTargetUID2;
							if ( data.sfx )
							{
								playSoundEntityLocal(fx, data.sfx, 128);
							}
							floorMagicSetLightningParticle(fx);
						}
					}
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_ETERNALS_GAZE2 )
			{
				if ( my->ticks == 1 )
				{
					static ConsoleVariable<float> cvar_magic_egaze7("/magic_egaze7", -8.25);
					static ConsoleVariable<float> cvar_magic_egaze9("/magic_egaze9", 0.05);
					for ( int i = 0; i < 5; ++i )
					{
						Entity* fx = createParticleAestheticOrbit(my, 1865, 4 * TICKS_PER_SECOND, PARTICLE_EFFECT_ETERNALS_GAZE2);
						fx->fskill[2] = *cvar_magic_egaze7;
						fx->fskill[3] = -24.25;
						fx->z = *cvar_magic_egaze7;
						fx->scaley = 1.0;
						fx->actmagicOrbitDist = 12.0;
						fx->fskill[6] = *cvar_magic_egaze9; // yaw
						if ( i <= 2 )
						{
							fx->yaw = i * 2 * PI / 3 + 3 * PI / 4;
						}
						else
						{
							fx->yaw = (i - 2) * PI + 3 * PI / 4;
							fx->actmagicOrbitDist = 1.0;
							fx->fskill[6] *= -2.0;
							fx->flags[GENIUS] = true;
						}
						fx->sizex = 4;
						fx->sizey = 4;
					}

					if ( Entity* fx = createParticleAOEIndicator(my, my->x, my->y, 0.0, TICKS_PER_SECOND, 32.0) )
					{
						//fx->actSpriteFollowUID = 0;
						//fx->actSpriteCheckParentExists = 0;
						fx->scalex = 0.8;
						fx->scaley = 0.8;
						if ( auto indicator = AOEIndicators_t::getIndicator(fx->skill[10]) )
						{
							//indicator->arc = PI / 2;
							Uint32 color = makeColorRGB(236, 240, 144);
							indicator->indicatorColor = color;
							indicator->loop = false;
							indicator->gradient = 4;
							indicator->framesPerTick = 2;
							indicator->ticksPerUpdate = 1;
							indicator->delayTicks = 0;
							indicator->expireAlphaRate = 0.95;
						}
					}
				}

				if ( multiplayer != CLIENT )
				{
					if ( my->ticks == 1
						|| my->ticks == 1 + 10
						|| my->ticks == 1 + 20 )
					{
						Entity* caster = uidToEntity(my->parent);
						createSpellExplosionArea(SPELL_ETERNALS_GAZE, caster, my->x, my->y, my->z, 12.0, 0, nullptr);
					}

					Entity* target = uidToEntity(my->actmagicOrbitHitTargetUID1);
					my->vel_x = 0.0;
					my->vel_y = 0.0;
					if ( target )
					{
						real_t dist = entityDist(my, target);
						real_t tangent = atan2(target->y - my->y, target->x - my->x);
						real_t speed = std::min(dist, std::max(16.0, 64.0 - dist) / 100.0);
						my->vel_x = speed * cos(tangent);
						my->vel_y = speed * sin(tangent);
					}

					my->x += my->vel_x;
					my->y += my->vel_y;
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_ETERNALS_GAZE )
			{
				if ( multiplayer != CLIENT )
				{
					Entity* parent = uidToEntity(my->parent);
					Entity* closestEntity = nullptr;
					if ( my->actmagicOrbitHitTargetUID1 != 0 )
					{
						closestEntity = uidToEntity(my->actmagicOrbitHitTargetUID1);
						if ( !closestEntity )
						{
							my->actmagicOrbitHitTargetUID1 = 0;
						}
					}
					if ( my->actmagicOrbitHitTargetUID1 == 0 )
					{
						std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 2);
						real_t dist = 10000.0;
						for ( auto it : entLists )
						{
							for ( node_t* node = it->first; node != nullptr; node = node->next )
							{
								Entity* entity = (Entity*)node->element;
								if ( parent && entity == parent )
								{
									continue;
								}
								if ( entity->behavior == &actMonster || entity->behavior == &actPlayer )
								{
									if ( parent && parent->checkFriend(entity) )
									{
										continue;
									}
									if ( !entity->monsterIsTargetable() ) { continue; }
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
							if ( dist < 8.0 )
							{
								my->actmagicOrbitHitTargetUID1 = closestEntity->getUID();
							}
						}
					}

					my->vel_x = 0.0;
					my->vel_y = 0.0;
					if ( closestEntity )
					{
						real_t dist = entityDist(my, closestEntity);
						real_t tangent = atan2(closestEntity->y - my->y, closestEntity->x - my->x);
						real_t speed = std::min(dist, std::max(16.0, 64.0 - dist) / 100.0);
						my->vel_x = speed * cos(tangent);
						my->vel_y = speed * sin(tangent);
					}

					my->x += my->vel_x;
					my->y += my->vel_y;

					if ( my->ticks == 3 * TICKS_PER_SECOND )
					{
						Entity* parent = uidToEntity(my->parent);
						Entity* target = uidToEntity(my->actmagicOrbitHitTargetUID1);
						Entity* spellTimer = createParticleTimer(target, 2 * TICKS_PER_SECOND, -1);
						spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_ETERNALS_GAZE2;
						spellTimer->particleTimerCountdownSprite = -1;
						spellTimer->flags[UPDATENEEDED] = true;
						spellTimer->flags[NOUPDATE] = false;
						spellTimer->yaw = my->yaw;
						if ( !target )
						{
							spellTimer->x = my->x;
							spellTimer->y = my->y;
						}
						spellTimer->parent = parent ? parent->getUID() : 0;
						spellTimer->actmagicOrbitHitTargetUID1 = target ? my->actmagicOrbitHitTargetUID1 : 0;
						Sint32 val = (1 << 31);
						val |= (Uint8)(19);
						val |= (((Uint16)(spellTimer->particleTimerDuration) & 0xFFF) << 8);
						val |= (Uint8)(spellTimer->particleTimerCountdownAction & 0xFF) << 20;
						spellTimer->skill[2] = val;

						my->removeLightField();
						list_RemoveNode(my->mynode);
						return;
					}
				}
				if ( my->ticks == 1 )
				{
					static ConsoleVariable<float> cvar_magic_egaze1("/magic_egaze1", -16.25);
					static ConsoleVariable<float> cvar_magic_egaze2("/magic_egaze2", 7.75);
					for ( int i = 0; i < 4; ++i )
					{
						Entity* fx = createParticleAestheticOrbit(my, 1865, 4 * TICKS_PER_SECOND, PARTICLE_EFFECT_ETERNALS_GAZE1);
						fx->yaw = i * PI / 2 + 3 * PI / 4;
						fx->fskill[2] = *cvar_magic_egaze2;
						fx->fskill[3] = *cvar_magic_egaze1;
						fx->fskill[6] = 0.15; // yaw
						fx->fskill[7] = 0.1;
						fx->z = *cvar_magic_egaze2;
						fx->scaley = 1.0;
						//fx->ditheringOverride = 6;
					}

					//for ( int i = 0; i < 3; ++i )
					//{
					//	Entity* fx = createParticleAestheticOrbit(my, 1867, 2 * TICKS_PER_SECOND, PARTICLE_EFFECT_ETERNALS_GAZE_STATIC);
					//	//fx->x = my->x + 40.0/*dist * (data.dist)*/ * cos(my->yaw);
					//	//fx->y = my->y + 40.0/*dist * (data.dist)*/ * sin(my->yaw);
					//	fx->z = 7.5 - 2.0 * i;
					//	fx->scalex = 0.25;
					//	fx->scaley = 0.25;
					//	fx->scalez = 0.25;
					//	fx->actmagicOrbitDist = 20;
					//	fx->yaw += i * 2 * PI / 3;
					//	if ( i != 0 )
					//	{
					//		fx->actmagicNoLight = 1;
					//	}
					//}
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_SHATTER_EARTH
				|| my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_EARTH_ELEMENTAL )
			{
				int x = static_cast<int>(my->x / 16);
				int y = static_cast<int>(my->y / 16);
				if ( x > 0 && x < map.width - 2 && y > 0 && y < map.height - 2 )
				{
					if ( my->ticks == 1 )
					{
						createParticleShatterEarth(my, uidToEntity(my->parent), my->x, my->y, 
							my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_SHATTER_EARTH ? SPELL_SHATTER_EARTH
						: SPELL_EARTH_ELEMENTAL);
					}
					if ( my->ticks < TICKS_PER_SECOND )
					{
						int mapIndex = (y)*MAPLAYERS + (x)*MAPLAYERS * map.height;
						bool tallCeiling = false;
						if ( !map.tiles[(MAPLAYERS - 1) + mapIndex] )
						{
							tallCeiling = true;
						}
						if ( my->ticks % 4 == 0 )
						{
							//for ( int i = 0; i < 6; ++i )
							int i = local_rng.rand() % 8;
							Entity* fx = createParticleAestheticOrbit(my, 1870, TICKS_PER_SECOND, PARTICLE_EFFECT_SHATTER_EARTH_ORBIT);
							fx->parent = 0;
							fx->z = -8.0 + (tallCeiling ? -16.0 : 0.0);
							fx->actmagicOrbitDist = 4;
							fx->actmagicOrbitStationaryX = x * 16.0 + 8.0;
							fx->actmagicOrbitStationaryY = y * 16.0 + 8.0;
							fx->yaw = i * PI / 8;
							fx->x = fx->actmagicOrbitStationaryX + fx->actmagicOrbitDist * cos(fx->yaw);
							fx->y = fx->actmagicOrbitStationaryY + fx->actmagicOrbitDist * sin(fx->yaw);
						}
					}
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_SPLINTER_GEAR )
			{
				if ( my->ticks % 4 == 0 )
				{
					//for ( int i = 0; i < 6; ++i )
					int i = (my->ticks / 4) % 3;
					Entity* fx = createParticleAestheticOrbit(my, 2205, TICKS_PER_SECOND, PARTICLE_EFFECT_SHATTER_EARTH_ORBIT);
					fx->parent = 0;
					fx->z = my->z;
					fx->yaw = my->yaw + 2 * i * PI / 3;
					fx->vel_x = 0.5 * cos(fx->yaw);
					fx->vel_y = 0.5 * sin(fx->yaw);
					fx->actmagicOrbitDist = 2;
					fx->actmagicOrbitStationaryX = my->x;
					fx->actmagicOrbitStationaryY = my->y;
					fx->x = fx->actmagicOrbitStationaryX + fx->actmagicOrbitDist * cos(fx->yaw);
					fx->y = fx->actmagicOrbitStationaryY + fx->actmagicOrbitDist * sin(fx->yaw);
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_BASTION_MUSHROOM )
			{
				if ( multiplayer != CLIENT )
				{
					Entity* parent = uidToEntity(my->parent);
					if ( parent )
					{
						my->x = parent->x;
						my->y = parent->y;
					}
					if ( my->ticks == 1 )
					{
						createMushroomSpellEffect(parent, my->x, my->y);
						serverSpawnMiscParticlesAtLocation(my->x, my->y, 0, PARTICLE_EFFECT_BASTION_MUSHROOM, 0, 0, 0, my->parent);

						if ( my->particleTimerVariable3 == SPELL_SPORE_BOMB )
						{
							floorMagicCreateSpores(parent ? parent : my, my->x, my->y, parent, 0, SPELL_SPORES);
						}
					}

					if ( my->ticks <= 50 && parent )
					{
						real_t range = 32.0;
						std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 1 + (range / 16));
						for ( auto it : entLists )
						{
							node_t* node;
							for ( node = it->first; node != nullptr; node = node->next )
							{
								Entity* entity = (Entity*)node->element;
								if ( !(entity->behavior == &actPlayer || entity->behavior == &actMonster) )
								{
									continue;
								}
								if ( !entity->monsterIsTargetable() )
								{
									continue;
								}
								if ( entityDist(my, entity) > (real_t)(range + 4.0) )
								{
									continue;
								}

								Stat* stats = entity->getStats();
								if ( !stats ) { continue; }

								if ( parent == entity )
								{
									continue;
								}
								if ( parent && parent->behavior == &actMonster )
								{
									if ( parent->checkFriend(entity) )
									{
										continue;
									}
								}
								if ( parent && parent->behavior == &actPlayer )
								{
									//if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
									{
										if ( parent->checkFriend(entity) && parent->friendlyFireProtection(entity) )
										{
											continue;
										}
									}
								}

								auto props = getParticleEmitterHitProps(my->getUID(), entity);
								if ( !props )
								{
									continue;
								}
								if ( props->hits > 0 )
								{
									continue;
								}

								real_t tangent = atan2(entity->y - parent->y, entity->x - parent->x);
								bool oldPassable = entity->flags[PASSABLE];
								entity->flags[PASSABLE] = false;
								real_t d = lineTraceTarget(parent, parent->x, parent->y, tangent, (real_t)(range + 4.0), 0, false, entity);
								entity->flags[PASSABLE] = oldPassable;
								if ( hit.entity != entity )
								{
									continue;
								}

								int damage = getSpellDamageFromID(SPELL_BASTION_MUSHROOM, parent, nullptr, my);
								if ( applyGenericMagicDamage(parent, entity, *parent, SPELL_BASTION_MUSHROOM, damage, true, true) )
								{
									props->hits++;
									props->tick = ticks;

									bool wasEffected = stats->getEffectActive(EFF_POISONED);
									if ( entity->setEffect(EFF_POISONED, true, 3 * TICKS_PER_SECOND + 10, false) )
									{
										spawnMagicEffectParticles(entity->x, entity->y, entity->z, 944);
										stats->poisonKiller = parent->getUID();
									}

									if ( entity->setEffect(EFF_KNOCKBACK, true, 30, false) )
									{
										real_t pushbackMultiplier = 0.9;
										real_t tangent = atan2(entity->y - parent->y, entity->x - parent->x);
										if ( entity->behavior == &actPlayer )
										{
											if ( !players[entity->skill[2]]->isLocalPlayer() )
											{
												entity->monsterKnockbackVelocity = pushbackMultiplier;
												entity->monsterKnockbackTangentDir = tangent;
												serverUpdateEntityFSkill(entity, 11);
												serverUpdateEntityFSkill(entity, 9);
											}
											else
											{
												entity->monsterKnockbackVelocity = pushbackMultiplier;
												entity->monsterKnockbackTangentDir = tangent;
											}
										}
										else if ( entity->behavior == &actMonster )
										{
											entity->vel_x = cos(tangent) * pushbackMultiplier;
											entity->vel_y = sin(tangent) * pushbackMultiplier;
											entity->monsterKnockbackVelocity = 0.01;
											entity->monsterKnockbackUID = parent->colliderCreatedParent;
											entity->monsterKnockbackTangentDir = tangent;
										}
									}
								}
							}
						}
					}
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_ROOTS1 )
			{
				if ( my->ticks == 1 && multiplayer != CLIENT )
				{
					Entity* fx = createFloorMagic(ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_SELF,
						1765, my->x, my->y, 7.5, my->yaw, PARTICLE_LIFE);
					//playSoundEntity(fx, data.sfx, 64);
					fx->parent = my->getUID();
					fx->actmagicNoParticle = 1;
					fx->sizex = 6;
					fx->sizey = 6;
					floorMagicParticleSetUID(*fx, true);
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_ROOTS_SUSTAIN )
			{
				if ( my->ticks == 1 && multiplayer != CLIENT )
				{
					Entity* fx = createFloorMagic(ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_SELF_SUSTAIN,
						1765, my->x, my->y, 7.5, my->yaw, PARTICLE_LIFE);
					//playSoundEntity(fx, data.sfx, 64);
					fx->parent = my->getUID();
					fx->actmagicNoParticle = 1;
					fx->sizex = 6;
					fx->sizey = 6;
					floorMagicParticleSetUID(*fx, true);
				}

				if ( multiplayer != CLIENT )
				{
					Entity* parent = uidToEntity(my->parent);
					bool sustain = false;
					if ( parent && parent->behavior == &actPlayer )
					{
						if ( Stat* stats = parent->getStats() )
						{
							for ( node_t* node = stats->magic_effects.first; node; node = node->next )
							{
								if ( spell_t* spell = (spell_t*)node->element )
								{
									if ( spell->ID == SPELL_BASTION_ROOTS && spell->sustain )
									{
										sustain = true;
										break;
									}
								}
							}
						}
					}

					if ( !sustain )
					{
						PARTICLE_LIFE = -1;
					}
					else
					{
						PARTICLE_LIFE = std::max(PARTICLE_LIFE, TICKS_PER_SECOND);
					}
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_ROOTS_SINGLE_TILE )
			{
				if ( my->ticks == 1 && multiplayer != CLIENT )
				{
					Entity* fx = createFloorMagic(ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_TILE,
						1765, my->x, my->y, 7.5, my->yaw, PARTICLE_LIFE);
					//playSoundEntity(fx, data.sfx, 64);
					fx->parent = my->getUID();
					fx->actmagicNoParticle = 1;
					fx->sizex = 6;
					fx->sizey = 6;
					floorMagicParticleSetUID(*fx, true);
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_ROOTS_SINGLE_TILE_VOID )
			{
				if ( my->ticks == 1 && multiplayer != CLIENT )
				{
					Entity* fx = createFloorMagic(ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_TILE_VOID,
						2199, my->x, my->y, 7.5, my->yaw, PARTICLE_LIFE);
					//playSoundEntity(fx, data.sfx, 64);
					fx->parent = my->getUID();
					fx->actmagicNoParticle = 1;
					fx->sizex = 6;
					fx->sizey = 6;
					floorMagicParticleSetUID(*fx, true);
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_ROOTS_PATH )
			{
				if ( my->ticks == 1 && multiplayer != CLIENT )
				{
					Entity* fx = createFloorMagic(ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_PATH,
						1765, my->x, my->y, 7.5, my->yaw, PARTICLE_LIFE);
					//playSoundEntity(fx, data.sfx, 64);
					fx->parent = my->getUID();
					fx->actmagicNoParticle = 1;
					fx->sizex = 6;
					fx->sizey = 6;
					floorMagicParticleSetUID(*fx, true);
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_EARTH_ELEMENTAL_DIE )
			{
				if ( my->ticks == 1 )
				{
					int x = static_cast<int>(my->x) >> 4;
					int y = static_cast<int>(my->y) >> 4;
					int mapIndex = (y)*MAPLAYERS + (x) * MAPLAYERS * map.height;
					if ( x > 0 && y > 0 && x < map.width - 1 && y < map.height - 1 )
					{
						if ( map.tiles[mapIndex] )
						{
							Entity* entity = newEntity(1869, 1, map.entities, nullptr); //Gib entity.
							entity->x = my->x;
							entity->y = my->y;
							entity->z = 7.0;
							entity->sizex = 2;
							entity->sizey = 2;
							entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
							entity->behavior = &actEarthElementalDeathGib;
							entity->ditheringDisabled = true;
							entity->flags[PASSABLE] = true;
							entity->flags[NOUPDATE] = true;
							entity->flags[UPDATENEEDED] = false;
							entity->flags[UNCLICKABLE] = true;
							entity->scalex = 0.05;
							entity->scaley = 0.05;
							entity->scalez = 0.05;
							entity->skill[0] = 5 * TICKS_PER_SECOND;
							if ( multiplayer != CLIENT )
							{
								entity_uids--;
							}
							entity->setUID(-3);

						}
					}
				}
				my->scalex -= 0.05;
				my->scaley -= 0.05;
				my->scalez -= 0.05;
				my->vel_z = -0.05;
				my->z += my->vel_z;
				if ( my->ticks % 4 == 0 )
				{
					int i = local_rng.rand() % 8;
					Entity* fx = createParticleAestheticOrbit(my, 1870, 30, PARTICLE_EFFECT_SHATTER_EARTH_ORBIT);
					fx->parent = 0;
					fx->z = my->z;
					fx->actmagicOrbitDist = 2;
					fx->actmagicOrbitStationaryX = my->x;
					fx->actmagicOrbitStationaryY = my->y;
					fx->yaw = i * PI / 8;
					fx->x = fx->actmagicOrbitStationaryX + fx->actmagicOrbitDist * cos(fx->yaw);
					fx->y = fx->actmagicOrbitStationaryY + fx->actmagicOrbitDist * sin(fx->yaw);
				}

				if ( my->scalex <= 0.0 )
				{
					list_RemoveNode(my->mynode);
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_TRAP_SABOTAGED )
			{
				if ( my->ticks == 1 )
				{
					int x = static_cast<int>(my->x) >> 4;
					int y = static_cast<int>(my->y) >> 4;
					int mapIndex = (y)*MAPLAYERS + (x)*MAPLAYERS * map.height;
					if ( x > 0 && y > 0 && x < map.width - 1 && y < map.height - 1 )
					{
						if ( map.tiles[mapIndex] )
						{
							Entity* entity = newEntity(1869, 1, map.entities, nullptr); //Gib entity.
							entity->x = my->x;
							entity->y = my->y;
							entity->z = 7.0;
							entity->sizex = 2;
							entity->sizey = 2;
							entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
							entity->behavior = &actEarthElementalDeathGib;
							entity->ditheringDisabled = true;
							entity->flags[PASSABLE] = true;
							entity->flags[NOUPDATE] = true;
							entity->flags[UPDATENEEDED] = false;
							entity->flags[UNCLICKABLE] = true;
							entity->scalex = 0.05;
							entity->scaley = 0.05;
							entity->scalez = 0.05;
							entity->skill[0] = -1;
							if ( multiplayer != CLIENT )
							{
								entity_uids--;
							}
							entity->setUID(-3);

						}
					}
				}
				my->scalex -= 0.05;
				my->scaley -= 0.05;
				my->scalez -= 0.05;
				my->vel_z = -0.05;
				my->z += my->vel_z;
				if ( my->ticks % 4 == 0 )
				{
					int i = local_rng.rand() % 8;
					Entity* fx = createParticleAestheticOrbit(my, 1870, TICKS_PER_SECOND, PARTICLE_EFFECT_SHATTER_EARTH_ORBIT);
					fx->parent = 0;
					fx->z = my->z;
					fx->actmagicOrbitDist = 2;
					fx->actmagicOrbitStationaryX = my->x;
					fx->actmagicOrbitStationaryY = my->y;
					fx->yaw = i * PI / 8;
					fx->x = fx->actmagicOrbitStationaryX + fx->actmagicOrbitDist * cos(fx->yaw);
					fx->y = fx->actmagicOrbitStationaryY + fx->actmagicOrbitDist * sin(fx->yaw);
				}

				if ( my->scalex <= 0.0 )
				{
					list_RemoveNode(my->mynode);
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_SPORES_TRAIL )
			{
				auto findEffects = particleTimerEffects.find(my->getUID());
				if ( findEffects != particleTimerEffects.end() )
				{
					// moved to actThrown
					//if ( auto projectile = uidToEntity(my->particleTimerVariable4) )
					//{
					//	int x = static_cast<int>(projectile->x) / 16;
					//	int y = static_cast<int>(projectile->y) / 16;
					//	bool freeSpot = true;
					//	Uint32 lastTick = 1;
					//	for ( auto& eff : findEffects->second.effectMap )
					//	{
					//		if ( static_cast<int>(eff.second.x) / 16 == x
					//			&& static_cast<int>(eff.second.x) / 16 == y )
					//		{
					//			freeSpot = false;
					//		}
					//		lastTick = std::max(eff.first, lastTick);
					//	}
					//	if ( freeSpot )
					//	{
					//		auto& effect = findEffects->second.effectMap[std::max(my->ticks + 1, lastTick + 2)]; // insert x ticks beyond last effect
					//		if ( findEffects->second.effectMap.size() == 1 )
					//		{
					//			effect.firstEffect = true;
					//		}
					//		int spellID = my->particleTimerVariable2;
					//		auto particleEffectType = (spellID == SPELL_SPORES || spellID == SPELL_MYCELIUM_SPORES) 
					//			? ParticleTimerEffect_t::EffectType::EFFECT_MYCELIUM
					//			: ParticleTimerEffect_t::EffectType::EFFECT_SPORES;
					//		effect.effectType = particleEffectType;
					//		effect.x = x * 16.0 + 8.0;
					//		effect.y = y * 16.0 + 8.0;
					//		effect.yaw = 0.0;
					//	}
					//}

					auto findEffect = findEffects->second.effectMap.find(my->ticks);
					if ( findEffect != findEffects->second.effectMap.end() )
					{
						auto& data = findEffect->second;
						int x = static_cast<int>(data.x) >> 4;
						int y = static_cast<int>(data.y) >> 4;
						int mapIndex = (y)*MAPLAYERS + (x)*MAPLAYERS * map.height;
						if ( x > 0 && y > 0 && x < map.width - 1 && y < map.height - 1
							&& !map.tiles[OBSTACLELAYER + mapIndex] )
						{
							auto entLists = TileEntityList.getEntitiesWithinRadius(x, y, 0);
							bool freeSpot = true;
							std::vector<Entity*> toDelete;
							for ( auto it : entLists )
							{
								if ( !freeSpot )
								{
									break;
								}
								for ( node_t* node = it->first; node != nullptr; node = node->next )
								{
									if ( Entity* entity = (Entity*)node->element )
									{
										if ( entity->behavior == &actParticleFloorMagic &&
											(entity->actfloorMagicType == data.effectType
												|| entity->actfloorMagicType == ParticleTimerEffect_t::EFFECT_SPORES
												|| entity->actfloorMagicType == ParticleTimerEffect_t::EFFECT_MYCELIUM) )
										{
											if ( my->particleTimerVariable2 == SPELL_SPORE_BOMB
												|| my->particleTimerVariable2 == SPELL_MYCELIUM_BOMB )
											{
												toDelete.push_back(entity);
											}
											else
											{
												freeSpot = false;
												break;
											}
										}
									}
								}
							}
							for ( auto ent : toDelete )
							{
								ent->removeLightField();
								list_RemoveNode(ent->mynode);
							}
							if ( freeSpot )
							{
								Entity* fx = createFloorMagic(data.effectType, my->particleTimerCountdownSprite, data.x, data.y, 4.0, data.yaw, PARTICLE_LIFE);
								fx->sizex = 8;
								fx->sizey = 8;
								fx->parent = my->getUID();
								floorMagicParticleSetUID(*fx, true);
							}
						}
					}
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_SPORES )
			{
				auto findEffects = particleTimerEffects.find(my->getUID());
				if ( findEffects != particleTimerEffects.end() )
				{
					auto findEffect = findEffects->second.effectMap.find(my->ticks);
					if ( findEffect != findEffects->second.effectMap.end() )
					{
						auto& data = findEffect->second;
						int x = static_cast<int>(data.x) >> 4;
						int y = static_cast<int>(data.y) >> 4;
						int mapIndex = (y)*MAPLAYERS + (x) * MAPLAYERS * map.height;
						if ( x > 0 && y > 0 && x < map.width - 1 && y < map.height - 1
							&& !map.tiles[OBSTACLELAYER + mapIndex] )
						{
							auto entLists = TileEntityList.getEntitiesWithinRadius(x, y, 0);
							bool freeSpot = true;
							std::vector<Entity*> toDelete;
							for ( auto it : entLists )
							{
								if ( !freeSpot )
								{
									break;
								}
								for ( node_t* node = it->first; node != nullptr; node = node->next )
								{
									if ( Entity* entity = (Entity*)node->element )
									{
										if ( entity->behavior == &actParticleFloorMagic && 
											(entity->actfloorMagicType == data.effectType
												|| entity->actfloorMagicType == ParticleTimerEffect_t::EFFECT_SPORES
												|| entity->actfloorMagicType == ParticleTimerEffect_t::EFFECT_MYCELIUM) )
										{
											if ( my->particleTimerVariable2 == SPELL_SPORE_BOMB 
												|| my->particleTimerVariable2 == SPELL_MYCELIUM_BOMB )
											{
												toDelete.push_back(entity);
											}
											else
											{
												freeSpot = false;
												break;
											}
										}
									}
								}
							}
							for ( auto ent : toDelete )
							{
								ent->removeLightField();
								list_RemoveNode(ent->mynode);
							}
							if ( freeSpot )
							{
								Entity* fx = createFloorMagic(data.effectType, my->particleTimerCountdownSprite, data.x, data.y, 4.0, data.yaw, PARTICLE_LIFE);
								fx->sizex = 8;
								fx->sizey = 8;
								fx->parent = my->getUID();
								floorMagicParticleSetUID(*fx, true);
							}
						}
					}
				}
			}
			else if ( my->particleTimerCountdownAction == PARTICLE_TIMER_ACTION_MAGIC_WAVE )
			{
				Entity* parent = uidToEntity(my->parent);
				if ( my->ticks == 1 )
				{
					//createParticleCastingIndicator(my, my->x, my->y, my->z, PARTICLE_LIFE, 0);
				}

				auto findEffects = particleTimerEffects.find(my->getUID());
				if ( findEffects != particleTimerEffects.end() )
				{
					auto findEffect = findEffects->second.effectMap.find(my->ticks);
					if ( findEffect != findEffects->second.effectMap.end() )
					{
						auto& data = findEffect->second;
						if ( data.effectType == ParticleTimerEffect_t::EFFECT_TEST_1 )
						{
							createParticleErupt(data.x, data.y, 592);
						}
						else if ( data.effectType == ParticleTimerEffect_t::EFFECT_ICE_WAVE )
						{
							Entity* fx = createFloorMagic(data.effectType, my->particleTimerCountdownSprite, data.x, data.y, 4.0, data.yaw, PARTICLE_LIFE + 3 * TICKS_PER_SECOND);
							fx->sizex = 4;
							fx->sizey = 4;
							fx->parent = my->getUID();
							if ( data.sfx )
							{
								playSoundEntity(fx, data.sfx, 64);
							}

							floorMagicParticleSetUID(*fx, true);
						}
						else if ( data.effectType == ParticleTimerEffect_t::EFFECT_DISRUPT_EARTH )
						{
							int mapx = static_cast<int>(data.x) >> 4;
							int mapy = static_cast<int>(data.y) >> 4;

							int mapIndex = (mapy)*MAPLAYERS + (mapx)*MAPLAYERS * map.height;
							if ( mapx > 0 && mapy > 0 && mapx < map.width - 1 && mapy < map.height - 1 )
							{
								if ( !map.tiles[OBSTACLELAYER + mapIndex] && map.tiles[mapIndex] 
									&& !swimmingtiles[map.tiles[mapIndex] && !lavatiles[map.tiles[mapIndex]]] )
								{
									Entity* fx = createFloorMagic(data.effectType, my->particleTimerCountdownSprite, data.x, data.y, 7.8, data.yaw, PARTICLE_LIFE + 3 * TICKS_PER_SECOND);
									fx->parent = my->getUID();
									fx->sizex = 6;
									fx->sizey = 6;
									Uint32 nextTickEffect = std::numeric_limits<Uint32>::max();
									bool foundNext = false;
									for ( auto& eff : findEffects->second.effectMap )
									{
										if ( eff.first > my->ticks )
										{
											if ( eff.first < nextTickEffect )
											{
												nextTickEffect = eff.first;
												foundNext = true;
											}
										}
									}
									if ( foundNext )
									{
										if ( findEffects->second.effectMap.find(nextTickEffect) != findEffects->second.effectMap.end() )
										{
											auto& data = findEffects->second.effectMap[nextTickEffect];
											real_t tangent = atan2(fx->y - data.y, fx->x - data.x);
											fx->monsterKnockbackTangentDir = tangent;
										}
									}
									if ( data.sfx )
									{
										playSoundEntity(fx, data.sfx, 64);
									}
									fx->actmagicNoParticle = 1;
									floorMagicParticleSetUID(*fx, true);
								}
							}
						}
						else if ( data.effectType == ParticleTimerEffect_t::EFFECT_LIGHTNING_BOLT )
						{
							Entity* fx = createFloorMagic(data.effectType, my->particleTimerCountdownSprite, data.x, data.y, -8.5, data.yaw, TICKS_PER_SECOND / 8);
							fx->scalex = 1.0;
							fx->scaley = 1.0;
							fx->scalez = 1.0;
							fx->ditheringDisabled = true;
							fx->actmagicNoParticle = 1;
							fx->parent = my->getUID();
							if ( flickerLights )
							{
								fx->light = addLight(fx->x / 16, fx->y / 16, "explosion");
							}
							else
							{
								if ( !my->light )
								{
									my->light = addLight(fx->x / 16, fx->y / 16, "explosion");
								}
							}
							if ( data.sfx )
							{
								playSoundEntity(fx, data.sfx, 64);
							}
							if ( data.firstEffect )
							{
								if ( Entity* fx = createParticleAOEIndicator(my, my->x, my->y, 0.0, TICKS_PER_SECOND, 32.0) )
								{
									//fx->actSpriteFollowUID = 0;
									fx->actSpriteCheckParentExists = 0;
									fx->scalex = 0.8;
									fx->scaley = 0.8;
									if ( auto indicator = AOEIndicators_t::getIndicator(fx->skill[10]) )
									{
										//indicator->arc = PI / 2;
										Uint32 color = makeColorRGB(252, 224, 0);
										indicator->indicatorColor = color;
										indicator->loop = false;
										indicator->gradient = 4;
										indicator->framesPerTick = 2;
										indicator->ticksPerUpdate = 1;
										indicator->delayTicks = 0;
										indicator->expireAlphaRate = 0.95;
									}
								}
							}
							/*if ( Entity* gib = spawnGib(fx) )
							{
								gib->sprite = 670;
							}*/

							//spawnExplosion(fx->x, fx->y, 7.5);

							//createParticleErupt(my, 670);
							//fx->flags[INVISIBLE] = true;
						}
						else if ( data.effectType == ParticleTimerEffect_t::EFFECT_TEST_3 )
						{
							Entity* fx = createFloorMagic(data.effectType, my->particleTimerCountdownSprite, data.x, data.y, 7.5, my->yaw, PARTICLE_LIFE + 3 * TICKS_PER_SECOND);
							if ( data.sfx )
							{
								playSoundEntity(fx, data.sfx, 64);
							}
							fx->parent = my->getUID();
							fx->actmagicNoParticle = 1;
						}
						else if ( data.effectType == ParticleTimerEffect_t::EFFECT_ROOTS_SELF
							|| data.effectType == ParticleTimerEffect_t::EFFECT_ROOTS_SELF_SUSTAIN
							|| data.effectType == ParticleTimerEffect_t::EFFECT_ROOTS_TILE
							|| data.effectType == ParticleTimerEffect_t::EFFECT_ROOTS_PATH )
						{
							Entity* fx = createFloorMagic(data.effectType, my->particleTimerCountdownSprite, data.x, data.y, 7.5, my->yaw, PARTICLE_LIFE);
							if ( data.sfx )
							{
								playSoundEntity(fx, data.sfx, 64);
							}
							fx->sizex = 6;
							fx->sizey = 6;
							fx->parent = my->getUID();
							fx->actmagicNoParticle = 1;
						}
						else if ( data.effectType == ParticleTimerEffect_t::EFFECT_TEST_2 )
						{
							real_t offset = PI * (local_rng.rand() % 360) / 180.0;// -((my->ticks % 50) / 50.0) * 2 * PI;
							int lifetime = PARTICLE_LIFE / 10;
							createVortexMagic(my->particleTimerCountdownSprite, my->x, my->y, 7.5, offset + 0.0, lifetime + 1 * TICKS_PER_SECOND);
							createVortexMagic(my->particleTimerCountdownSprite, my->x, my->y, 7.5, offset + 2 * PI / 3, lifetime + 1 * TICKS_PER_SECOND);
							createVortexMagic(my->particleTimerCountdownSprite, my->x, my->y, 7.5, offset + 4 * PI / 3, lifetime + 1 * TICKS_PER_SECOND);
						}
					}
				}
			}
		}
		else
		{
			--my->particleTimerPreDelay;
		}
	}
}

void actParticleSap(Entity* my)
{
	real_t decel = 0.9;
	real_t accel = 0.9;
	real_t z_accel = accel;
	real_t z_decel = decel;
	real_t minSpeed = 0.05;

	if ( PARTICLE_LIFE < 0 )
	{
		list_RemoveNode(my->mynode);
		return;
	}
	else
	{
		if ( my->sprite == 977 ) // boomerang
		{
			if ( my->skill[3] == 1 )
			{
				// specific for the animation I want...
				// magic numbers that take approximately 75 frames (50% of travel time) to go outward or inward.
				// acceleration is a little faster to overshoot into the right hand side.
				decel = 0.9718; 
				accel = 0.9710;
				z_decel = decel;
				z_accel = z_decel;
			}
			else
			{
				decel = 0.95;
				accel = 0.949;
				z_decel = 0.9935;
				z_accel = z_decel;
			}
			Entity* particle = spawnMagicParticleCustom(my, (local_rng.rand() % 2) ? 943 : 979, 1, 10);
			if ( particle )
			{
				particle->lightBonus = vec4(0.5f, 0.5f, 0.5f, 0.f);
				particle->focalx = 2;
				particle->focaly = -2;
				particle->focalz = 2.5;
				particle->ditheringDisabled = true;
			}
			if ( PARTICLE_LIFE < 100 && my->ticks % 6 == 0 )
			{
				if ( PARTICLE_LIFE < 70 )
				{
					playSoundEntityLocal(my, 434 + local_rng.rand() % 10, 64);
				}
				else
				{
					playSoundEntityLocal(my, 434 + local_rng.rand() % 10, 32);
				}
			}
			//particle->flags[SPRITE] = true;
		}
		else
		{
			if ( *cvar_magic_fx_use_vismap && !intro )
			{
				int x = my->x / 16.0;
				int y = my->y / 16.0;
				if ( x >= 0 && x < map.width && y >= 0 && y < map.height )
				{
					for ( int i = 0; i < MAXPLAYERS; ++i )
					{
						if ( !client_disconnected[i] && players[i]->isLocalPlayer() && cameras[i].vismap[y + x * map.height] )
						{
							spawnMagicParticle(my);
							break;
						}
					}
				}
			}
			else
			{
				spawnMagicParticle(my);
			}
		}
		Entity* parent = uidToEntity(my->parent);
		if ( parent )
		{
			my->x = parent->x + my->fskill[0];
			my->y = parent->y + my->fskill[1];
		}
		else
		{
			list_RemoveNode(my->mynode);
			return;
		}

		if ( my->skill[1] == 0 )
		{
			// move outwards diagonally.
			if ( abs(my->vel_z) > minSpeed )
			{
				my->fskill[0] += my->vel_x;
				my->fskill[1] += my->vel_y;
				my->vel_x *= decel;
				my->vel_y *= decel;

				my->z += my->vel_z;
				my->vel_z *= z_decel;

				my->yaw += my->fskill[2];
				my->pitch += my->fskill[3];
			}
			else
			{
				my->skill[1] = 1;
				my->vel_x *= -1;
				my->vel_y *= -1;
				my->vel_z *= -1;
			}
		}
		else if ( my->skill[1] == 1 )
		{
			// move inwards diagonally.
			if ( (abs(my->vel_z) < 0.08 && my->skill[3] == 0) || (abs(my->vel_z) < 0.4 && my->skill[3] == 1) )
			{
				my->fskill[0] += my->vel_x;
				my->fskill[1] += my->vel_y;
				my->vel_x /= accel;
				my->vel_y /= accel;

				my->z += my->vel_z;
				my->vel_z /= z_accel;

				my->yaw += my->fskill[2];
				my->pitch += my->fskill[3];
			}
			else
			{
				// movement completed.
				my->skill[1] = 2;
			}
		}

		my->scalex *= 0.99;
		my->scaley *= 0.99;
		my->scalez *= 0.99;
		if ( my->sprite == 977 )
		{
			my->scalex = 1.f;
			my->scaley = 1.f;
			my->scalez = 1.f;
			my->roll -= 0.5;
			my->pitch = std::max(my->pitch - 0.015, 0.0);
		}
		--PARTICLE_LIFE;
	}
}

void actParticleSapCenter(Entity* my)
{
	// init
	if ( my->skill[3] == 0 )
	{
		// for clients and server spawn the visible arcing particles.
		my->skill[3] = 1;
		createParticleSap(my);
	}

	if ( multiplayer == CLIENT )
	{
		return;
	}

	Entity* parent = uidToEntity(my->parent);
	if ( parent )
	{
		// if reached the caster, delete self and spawn some particles.
		if ( my->sprite == 977 && PARTICLE_LIFE > 1 )
		{
			// store these in case parent dies.
			// boomerang doesn't check for collision until end of life.
			my->fskill[4] = parent->x; 
			my->fskill[5] = parent->y;
		}
		else if ( entityInsideEntity(my, parent) || (my->sprite == 977 && PARTICLE_LIFE == 0) )
		{
			if ( my->skill[6] == SPELL_STEAL_WEAPON )
			{
				if ( my->skill[7] == 1 )
				{
					// found stolen item.
					Item* item = newItemFromEntity(my);
					if ( parent->behavior == &actPlayer )
					{
						itemPickup(parent->skill[2], item);
					}
					else if ( parent->behavior == &actMonster )
					{
						Stat* myStats = parent->getStats();
						if ( myStats && myStats->type == INCUBUS && myStats->getAttribute("special_npc") == "johann" )
						{
							if ( dropItemMonster(item, parent, parent->getStats(), item->count) )
							{
								parent->monsterSpecialState = INCUBUS_TELEPORT_STEAL;
								parent->monsterSpecialTimer = 100 + local_rng.rand() % MONSTER_SPECIAL_COOLDOWN_INCUBUS_TELEPORT_RANDOM;
								item = nullptr;
							}
						}

						if ( item )
						{
							parent->addItemToMonsterInventory(item);
							if ( myStats )
							{
								node_t* weaponNode = itemNodeInInventory(myStats, -1, WEAPON);
								if ( weaponNode )
								{
									swapMonsterWeaponWithInventoryItem(parent, myStats, weaponNode, false, true);
									if ( myStats->type == INCUBUS )
									{
										parent->monsterSpecialState = INCUBUS_TELEPORT_STEAL;
										parent->monsterSpecialTimer = 100 + local_rng.rand() % MONSTER_SPECIAL_COOLDOWN_INCUBUS_TELEPORT_RANDOM;
									}
								}
							}
						}
					}
					item = nullptr;
				}
				playSoundEntity(parent, 168, 128);
				spawnMagicEffectParticles(parent->x, parent->y, parent->z, my->skill[5]);
			}
			else if ( my->skill[6] == SPELL_DRAIN_SOUL )
			{
				parent->modHP(my->skill[7]);
				parent->modMP(my->skill[8]);
				if ( parent->behavior == &actPlayer )
				{
					Uint32 color = makeColorRGB(0, 255, 0);
					messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, color, Language::get(2445));
				}
				playSoundEntity(parent, 168, 128);
				spawnMagicEffectParticles(parent->x, parent->y, parent->z, 169);
			}
			else if ( my->skill[6] == SHADOW_SPELLCAST )
			{
				parent->shadowSpecialAbility(parent->monsterShadowInitialMimic);
				playSoundEntity(parent, 166, 128);
				spawnMagicEffectParticles(parent->x, parent->y, parent->z, my->skill[5]);
			}
			else if ( my->skill[6] == SPELL_SUMMON )
			{
				parent->modMP(my->skill[7]);
				/*if ( parent->behavior == &actPlayer )
				{
					Uint32 color = makeColorRGB(0, 255, 0);
					messagePlayerColor(parent->skill[2], MESSAGE_STATUS, color, Language::get(774));
				}*/
				playSoundEntity(parent, 168, 128);
				spawnMagicEffectParticles(parent->x, parent->y, parent->z, 169);
			}
			else if ( my->skill[6] == SPELL_FEAR )
			{
				playSoundEntity(parent, 168, 128);
				spawnMagicEffectParticles(parent->x, parent->y, parent->z, 174);
				Entity* caster = uidToEntity(my->skill[7]);
				if ( caster )
				{
					spellEffectFear(nullptr, spellElement_fear, caster, parent, 0);
				}
			}
			else if ( my->sprite == 977 ) // boomerang
			{
				Item* item = newItemFromEntity(my);
				if ( parent->behavior == &actPlayer )
				{
					item->ownerUid = parent->getUID();
					Item* pickedUp = itemPickup(parent->skill[2], item);
					Uint32 color = makeColorRGB(0, 255, 0);
					messagePlayerColor(parent->skill[2], MESSAGE_EQUIPMENT, color, Language::get(3746), items[item->type].getUnidentifiedName());
					achievementObserver.awardAchievementIfActive(parent->skill[2], parent, AchievementObserver::BARONY_ACH_IF_YOU_LOVE_SOMETHING);
					if ( pickedUp )
					{
						if ( parent->skill[2] == 0 || (parent->skill[2] > 0 && splitscreen) )
						{
							// pickedUp is the new inventory stack for server, free the original items
							free(item);
							item = nullptr;
							if ( multiplayer != CLIENT && !stats[parent->skill[2]]->weapon )
							{
								useItem(pickedUp, parent->skill[2]);
							}
							auto& hotbar_t = players[parent->skill[2]]->hotbar;
							if ( hotbar_t.magicBoomerangHotbarSlot >= 0 )
							{
								auto& hotbar = hotbar_t.slots();
								hotbar[hotbar_t.magicBoomerangHotbarSlot].item = pickedUp->uid;
								for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
								{
									if ( i != hotbar_t.magicBoomerangHotbarSlot
										&& hotbar[i].item == pickedUp->uid )
									{
										hotbar[i].item = 0;
										hotbar[i].resetLastItem();
									}
								}
							}
						}
						else
						{
							free(pickedUp); // item is the picked up items (item == pickedUp)
						}
					}
				}
				else if ( parent->behavior == &actMonster )
				{
					parent->addItemToMonsterInventory(item);
					Stat *myStats = parent->getStats();
					if ( myStats )
					{
						node_t* weaponNode = itemNodeInInventory(myStats, -1, WEAPON);
						if ( weaponNode )
						{
							swapMonsterWeaponWithInventoryItem(parent, myStats, weaponNode, false, true);
						}
					}
				}
				playSoundEntity(parent, 431 + local_rng.rand() % 3, 92);
				item = nullptr;
			}
			list_RemoveNode(my->mynode);
			return;
		}

		// calculate direction to caster and move.
		real_t tangent = atan2(parent->y - my->y, parent->x - my->x);
		real_t dist = sqrt(pow(my->x - parent->x, 2) + pow(my->y - parent->y, 2));
		real_t speed = dist / std::max(PARTICLE_LIFE, 1);
		my->vel_x = speed * cos(tangent);
		my->vel_y = speed * sin(tangent);
		my->x += my->vel_x;
		my->y += my->vel_y;
	}
	else
	{
		if ( my->skill[6] == SPELL_SUMMON )
		{
			real_t dist = sqrt(pow(my->x - my->skill[8], 2) + pow(my->y - my->skill[9], 2));
			if ( dist < 4 )
			{
				spawnMagicEffectParticles(my->skill[8], my->skill[9], 0, my->skill[5]);
				Entity* caster = uidToEntity(my->skill[7]);
				if ( caster && caster->behavior == &actPlayer && stats[caster->skill[2]] )
				{
					// kill old summons.
					for ( node_t* node = stats[caster->skill[2]]->FOLLOWERS.first; node != nullptr; node = node->next )
					{
						Entity* follower = nullptr;
						if ( (Uint32*)(node)->element )
						{
							follower = uidToEntity(*((Uint32*)(node)->element));
						}
						if ( follower && follower->monsterAllySummonRank != 0 )
						{
							Stat* followerStats = follower->getStats();
							if ( followerStats && followerStats->HP > 0 )
							{
								follower->setMP(followerStats->MAXMP * (followerStats->HP / static_cast<float>(followerStats->MAXHP)));
								follower->setHP(0);
							}
						}
					}

					Monster creature = SKELETON;
					Entity* monster = summonMonster(creature, my->skill[8], my->skill[9]);
					if ( monster )
					{
						Stat* monsterStats = monster->getStats();
						monster->yaw = my->yaw - PI;
						if ( monsterStats )
						{
							int magicLevel = 1;
							magicLevel = std::min(7, 1 + (stats[caster->skill[2]]->playerSummonLVLHP >> 16) / 5);

							monster->monsterAllySummonRank = magicLevel;
							monsterStats->setAttribute("special_npc", "skeleton knight");
							strcpy(monsterStats->name, MonsterData_t::getSpecialNPCName(*monsterStats).c_str());
							forceFollower(*caster, *monster);

							monster->setEffect(EFF_STUNNED, true, 20, false);
							bool spawnSecondAlly = false;
							if ( auto spell = getSpellFromID(SPELL_SUMMON) )
							{
								if ( (caster->getINT() + stats[caster->skill[2]]->getModifiedProficiency(spell->skillID)) >= SKILL_LEVEL_EXPERT )
								{
									spawnSecondAlly = true;
								}
							}
							//parent->increaseSkill(PRO_LEADERSHIP);
							monster->monsterAllyIndex = caster->skill[2];
							if ( multiplayer == SERVER )
							{
								serverUpdateEntitySkill(monster, 42); // update monsterAllyIndex for clients.
							}

							// change the color of the hit entity.
							monster->flags[USERFLAG2] = true;
							serverUpdateEntityFlag(monster, USERFLAG2);
							if ( monsterChangesColorWhenAlly(monsterStats) )
							{
								int bodypart = 0;
								for ( node_t* node = (monster)->children.first; node != nullptr; node = node->next )
								{
									if ( bodypart >= LIMB_HUMANOID_TORSO )
									{
										Entity* tmp = (Entity*)node->element;
										if ( tmp )
										{
											tmp->flags[USERFLAG2] = true;
											serverUpdateEntityFlag(tmp, USERFLAG2);
										}
									}
									++bodypart;
								}
							}

							if ( spawnSecondAlly )
							{
								Entity* monster = summonMonster(creature, my->skill[8], my->skill[9]);
								if ( monster )
								{
									if ( multiplayer != CLIENT )
									{
										spawnExplosion(monster->x, monster->y, -1);
										playSoundEntity(monster, 171, 128);

										createParticleErupt(monster, 791);
										serverSpawnMiscParticles(monster, PARTICLE_EFFECT_ERUPT, 791);
									}

									Stat* monsterStats = monster->getStats();
									monster->yaw = my->yaw - PI;
									if ( monsterStats )
									{
										monsterStats->setAttribute("special_npc", "skeleton sentinel");
										strcpy(monsterStats->name, MonsterData_t::getSpecialNPCName(*monsterStats).c_str());
										magicLevel = 1;
										if ( stats[caster->skill[2]] )
										{
											magicLevel = std::min(7, 1 + (stats[caster->skill[2]]->playerSummon2LVLHP >> 16) / 5);
										}
										monster->monsterAllySummonRank = magicLevel;

										forceFollower(*caster, *monster);
										monster->setEffect(EFF_STUNNED, true, 20, false);

										monster->monsterAllyIndex = caster->skill[2];
										if ( multiplayer == SERVER )
										{
											serverUpdateEntitySkill(monster, 42); // update monsterAllyIndex for clients.
										}

										if ( caster && caster->behavior == &actPlayer )
										{
											steamAchievementClient(caster->skill[2], "BARONY_ACH_SKELETON_CREW");
										}

										// change the color of the hit entity.
										monster->flags[USERFLAG2] = true;
										serverUpdateEntityFlag(monster, USERFLAG2);
										if ( monsterChangesColorWhenAlly(monsterStats) )
										{
											int bodypart = 0;
											for ( node_t* node = (monster)->children.first; node != nullptr; node = node->next )
											{
												if ( bodypart >= LIMB_HUMANOID_TORSO )
												{
													Entity* tmp = (Entity*)node->element;
													if ( tmp )
													{
														tmp->flags[USERFLAG2] = true;
														serverUpdateEntityFlag(tmp, USERFLAG2);
													}
												}
												++bodypart;
											}
										}
									}
								}
							}
						}
					}
				}
				list_RemoveNode(my->mynode);
				return;
			}

			// calculate direction to caster and move.
			real_t tangent = atan2(my->skill[9] - my->y, my->skill[8] - my->x);
			real_t speed = dist / PARTICLE_LIFE;
			my->vel_x = speed * cos(tangent);
			my->vel_y = speed * sin(tangent);
			my->x += my->vel_x;
			my->y += my->vel_y;
		}
		else if ( my->skill[6] == SPELL_STEAL_WEAPON )
		{
			Entity* entity = newEntity(-1, 1, map.entities, nullptr); //Item entity.
			entity->flags[INVISIBLE] = true;
			entity->flags[UPDATENEEDED] = true;
			entity->x = my->x;
			entity->y = my->y;
			entity->sizex = 4;
			entity->sizey = 4;
			entity->yaw = my->yaw;
			entity->vel_x = (local_rng.rand() % 20 - 10) / 10.0;
			entity->vel_y = (local_rng.rand() % 20 - 10) / 10.0;
			entity->vel_z = -.5;
			entity->flags[PASSABLE] = true;
			entity->flags[USERFLAG1] = true; // speeds up game when many items are dropped
			entity->behavior = &actItem;
			entity->skill[10] = my->skill[10];
			entity->skill[11] = my->skill[11];
			entity->skill[12] = my->skill[12];
			entity->skill[13] = my->skill[13];
			entity->skill[14] = my->skill[14];
			entity->skill[15] = my->skill[15];
			entity->itemOriginalOwner = my->itemOriginalOwner;
			entity->parent = 0;

			// no parent, no target to travel to.
			list_RemoveNode(my->mynode);
			return;
		}
		else if ( my->sprite == 977 )
		{
			// calculate direction to caster and move.
			real_t tangent = atan2(my->fskill[5] - my->y, my->fskill[4] - my->x);
			real_t dist = sqrt(pow(my->x - my->fskill[4], 2) + pow(my->y - my->fskill[5], 2));
			real_t speed = dist / std::max(PARTICLE_LIFE, 1);

			if ( dist < 4 || (abs(my->fskill[5]) < 0.001 && abs(my->fskill[4]) < 0.001) )
			{
				// reached goal, or goal not set then spawn the item.
				Entity* entity = newEntity(-1, 1, map.entities, nullptr); //Item entity.
				entity->flags[INVISIBLE] = true;
				entity->flags[UPDATENEEDED] = true;
				entity->x = my->x;
				entity->y = my->y;
				entity->sizex = 4;
				entity->sizey = 4;
				entity->yaw = my->yaw;
				entity->vel_x = (local_rng.rand() % 20 - 10) / 10.0;
				entity->vel_y = (local_rng.rand() % 20 - 10) / 10.0;
				entity->vel_z = -.5;
				entity->flags[PASSABLE] = true;
				entity->flags[USERFLAG1] = true; // speeds up game when many items are dropped
				entity->behavior = &actItem;
				entity->skill[10] = my->skill[10];
				entity->skill[11] = my->skill[11];
				entity->skill[12] = my->skill[12];
				entity->skill[13] = my->skill[13];
				entity->skill[14] = my->skill[14];
				entity->skill[15] = my->skill[15];
				entity->itemOriginalOwner = 0;
				entity->parent = 0;

				list_RemoveNode(my->mynode);
				return;
			}
			my->vel_x = speed * cos(tangent);
			my->vel_y = speed * sin(tangent);
			my->x += my->vel_x;
			my->y += my->vel_y;
		}
		else
		{
			// no parent, no target to travel to.
			list_RemoveNode(my->mynode);
			return;
		}
	}

	if ( PARTICLE_LIFE < 0 )
	{
		list_RemoveNode(my->mynode);
		return;
	}
	else
	{
		--PARTICLE_LIFE;
	}
}

void createParticleExplosionCharge(Entity* parent, int sprite, int particleCount, double scale)
{
	if ( !parent )
	{
		return;
	}

	for ( int c = 0; c < particleCount; c++ )
	{
		// shoot drops to the sky
		Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Particle entity.
		entity->sizex = 1;
		entity->sizey = 1;
		entity->x = parent->x - 3 + local_rng.rand() % 7;
		entity->y = parent->y - 3 + local_rng.rand() % 7;
		entity->z = 0 + local_rng.rand() % 190;
		if ( parent && parent->behavior == &actPlayer )
		{
			entity->z /= 2;
		}
		entity->vel_z = -1;
		entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
		entity->particleDuration = entity->z + 10;
		/*if ( local_rng.rand() % 5 > 0 )
		{
			entity->vel_x = 0.5*cos(entity->yaw);
			entity->vel_y = 0.5*sin(entity->yaw);
			entity->particleDuration = 6;
			entity->z = 0;
			entity->vel_z = 0.5 *(-1 + local_rng.rand() % 3);
		}*/
		entity->scalex *= scale;
		entity->scaley *= scale;
		entity->scalez *= scale;
		entity->behavior = &actParticleExplosionCharge;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[UNCLICKABLE] = true;
		entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
			*cvar_magic_fx_light_bonus, 0.f);
		entity->parent = parent->getUID();
		if ( multiplayer != CLIENT )
		{
			entity_uids--;
		}
		entity->setUID(-3);
	}

	int radius = STRIKERANGE * 2 / 3;
	real_t arc = PI / 16;
	int randScale = 1;
	for ( int c = 0; c < 128; c++ )
	{
		// shoot drops to the sky
		Entity* entity = newEntity(670, 1, map.entities, nullptr); //Particle entity.
		entity->sizex = 1;
		entity->sizey = 1;
		entity->yaw = 0 + c * arc;

		entity->x = parent->x + (radius * cos(entity->yaw));// - 2 + local_rng.rand() % 5;
		entity->y = parent->y + (radius * sin(entity->yaw));// - 2 + local_rng.rand() % 5;
		entity->z = radius + 150;
		entity->particleDuration = entity->z + local_rng.rand() % 3;
		entity->vel_z = -1;
		if ( parent && parent->behavior == &actPlayer )
		{
			entity->z /= 2;
		}
		randScale = 1 + local_rng.rand() % 3;

		entity->scalex *= (scale / randScale);
		entity->scaley *= (scale / randScale);
		entity->scalez *= (scale / randScale);
		entity->behavior = &actParticleExplosionCharge;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[UNCLICKABLE] = true;
		entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
			*cvar_magic_fx_light_bonus, 0.f);
		entity->parent = parent->getUID();
		if ( multiplayer != CLIENT )
		{
			entity_uids--;
		}
		entity->setUID(-3);
		if ( c > 0 && c % 16 == 0 )
		{
			radius -= 2;
		}
	}
}

void actParticleExplosionCharge(Entity* my)
{
	if ( PARTICLE_LIFE < 0 || (my->z < -4 && local_rng.rand() % 4 == 0) || (ticks % 14 == 0 && uidToEntity(my->parent) == nullptr) )
	{
		list_RemoveNode(my->mynode);
	}
	else
	{
		--PARTICLE_LIFE;
		my->yaw += 0.1;
		my->x += my->vel_x;
		my->y += my->vel_y;
		my->z += my->vel_z;
		my->scalex /= 0.99;
		my->scaley /= 0.99;
		my->scalez /= 0.99;
		//my->z -= 0.01;
	}
	return;
}

bool Entity::magicFallingCollision()
{
	hit.entity = nullptr;
	real_t zLimitHigh = -5;
	if ( sprite == 2209 || sprite == 233 ) // meteor
	{
		zLimitHigh = -5;
	}
	if ( z <= zLimitHigh || fabs(vel_z) < 0.01 )
	{
		// check if particle stopped or too high.
		return false;
	}

	if ( z >= 7.5 )
	{
		return true;
	}

	if ( actmagicIsVertical == MAGIC_ISVERTICAL_Z )
	{
		std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(this, 1);
		for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
		{
			list_t* currentList = *it;
			node_t* node;
			for ( node = currentList->first; node != nullptr; node = node->next )
			{
				Entity* entity = (Entity*)node->element;
				if ( entity )
				{
					if ( entity == this )
					{
						continue;
					}
					if ( entityInsideEntity(this, entity) && !entity->flags[PASSABLE] && (entity->getUID() != this->parent) )
					{
						if ( collisionProjectileMiss(uidToEntity(this->parent), this) )
						{
							continue;
						}
						hit.entity = entity;
						//hit.side = HORIZONTAL;
						return true;
					}
				}
			}
		}
	}

	return false;
}

bool Entity::magicOrbitingCollision()
{
	hit.entity = nullptr;

	if ( this->actmagicIsOrbiting == 2 )
	{
		if ( this->ticks == 5 && this->actmagicOrbitHitTargetUID4 != 0 )
		{
			// hit this target automatically
			Entity* tmp = uidToEntity(actmagicOrbitHitTargetUID4);
			if ( tmp )
			{
				hit.entity = tmp;
				return true;
			}
		}
		if ( this->z < -8 || this->z > 3 )
		{
			return false;
		}
		else if ( this->ticks >= 12 && this->ticks % 4 != 0 ) // check once every 4 ticks, after the missile is alive for a bit
		{
			return false;
		}
	}
	else if ( this->z < -10 )
	{
		return false;
	}

	if ( this->actmagicIsOrbiting == 2 )
	{
		if ( this->actmagicOrbitStationaryHitTarget >= 3 )
		{
			return false;
		}
	}

	Entity* caster = uidToEntity(parent);

	std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(this, 1);

	for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
	{
		list_t* currentList = *it;
		node_t* node;
		for ( node = currentList->first; node != NULL; node = node->next )
		{
			Entity* entity = (Entity*)node->element;
			if ( entity == this )
			{
				continue;
			}
			if ( entity->behavior != &actMonster 
				&& entity->behavior != &actPlayer
				&& entity->behavior != &actDoor
				&& entity->behavior != &::actIronDoor
				&& !(entity->isDamageableCollider() && entity->isColliderDamageableByMagic())
				&& !entity->isInertMimic()
				&& entity->behavior != &::actChest 
				&& entity->behavior != &::actFurniture )
			{
				continue;
			}
			if ( caster && !(svFlags & SV_FLAG_FRIENDLYFIRE) && caster->checkFriend(entity) && caster->friendlyFireProtection(entity) )
			{
				continue;
			}
			if ( actmagicIsOrbiting == 2 )
			{
				if ( static_cast<Uint32>(actmagicOrbitHitTargetUID1) == entity->getUID()
					|| static_cast<Uint32>(actmagicOrbitHitTargetUID2) == entity->getUID()
					|| static_cast<Uint32>(actmagicOrbitHitTargetUID3) == entity->getUID()
					|| static_cast<Uint32>(actmagicOrbitHitTargetUID4) == entity->getUID() )
				{
					// we already hit these guys.
					continue;
				}
			}
			if ( entityInsideEntity(this, entity) && !entity->flags[PASSABLE] && (entity->getUID() != this->parent) )
			{
				hit.entity = entity;
				if ( hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer )
				{
					if ( actmagicIsOrbiting == 2 )
					{
						if ( actmagicOrbitHitTargetUID4 != 0 && caster && caster->behavior == &actPlayer )
						{
							if ( actmagicOrbitHitTargetUID1 == 0 
								&& actmagicOrbitHitTargetUID2 == 0
								&& actmagicOrbitHitTargetUID3 == 0
								&& hit.entity->behavior == &actMonster )
							{
								steamStatisticUpdateClient(caster->skill[2], STEAM_STAT_VOLATILE, STEAM_STAT_INT, 1);
							}
						}
						++actmagicOrbitStationaryHitTarget;
						if ( actmagicOrbitHitTargetUID1 == 0 )
						{
							actmagicOrbitHitTargetUID1 = entity->getUID();
						}
						else if ( actmagicOrbitHitTargetUID2 == 0 )
						{
							actmagicOrbitHitTargetUID2 = entity->getUID();
						}
						else if ( actmagicOrbitHitTargetUID3 == 0 )
						{
							actmagicOrbitHitTargetUID3 = entity->getUID();
						}
					}
				}
				return true;
			}
		}
	}

	return false;
}

void Entity::castFallingMagicMissile(int spellID, real_t distFromCaster, real_t angleFromCasterDirection, int heightDelay)
{
	spell_t* spell = getSpellFromID(spellID);
	Entity* entity = castSpell(getUID(), spell, false, true);
	if ( entity )
	{
		entity->x = x + distFromCaster * cos(yaw + angleFromCasterDirection);
		entity->y = y + distFromCaster * sin(yaw + angleFromCasterDirection);
		entity->z = -25 - heightDelay;
		double missile_speed = 4;
		entity->vel_x = 0.0;
		entity->vel_y = 0.0;
		entity->vel_z = 0.5 * (missile_speed);
		entity->pitch = PI / 2;
		entity->actmagicIsVertical = MAGIC_ISVERTICAL_Z;
		spawnMagicEffectParticles(entity->x, entity->y, 0, 174);
		playSoundEntity(entity, spellGetCastSound(spell), 128);
	}
}

Entity* Entity::castOrbitingMagicMissile(int spellID, real_t distFromCaster, real_t angleFromCasterDirection, int duration)
{
	spell_t* spell = getSpellFromID(spellID);
	Entity* entity = castSpell(getUID(), spell, false, true);
	if ( entity )
	{
		if ( spellID == SPELL_FIREBALL )
		{
			entity->sprite = 671;
		}
		else if ( spellID == SPELL_MAGICMISSILE )
		{
			entity->sprite = 679;
		}
		entity->yaw = angleFromCasterDirection;
		entity->x = x + distFromCaster * cos(yaw + entity->yaw);
		entity->y = y + distFromCaster * sin(yaw + entity->yaw);
		entity->z = -2.5;
		double missile_speed = 4;
		entity->vel_x = 0.0;
		entity->vel_y = 0.0;
		entity->actmagicIsOrbiting = 1;
		entity->actmagicOrbitDist = distFromCaster;
		entity->actmagicOrbitStartZ = entity->z;
		entity->z += 4 * sin(angleFromCasterDirection);
		entity->roll += (PI / 8) * (1 - abs(sin(angleFromCasterDirection)));
		entity->actmagicOrbitVerticalSpeed = 0.1;
		entity->actmagicOrbitVerticalDirection = 1;
		entity->actmagicOrbitLifetime = duration;
		entity->vel_z = entity->actmagicOrbitVerticalSpeed;
		playSoundEntity(entity, spellGetCastSound(spell), 128);
		//spawnMagicEffectParticles(entity->x, entity->y, 0, 174);
	}
	return entity;
}

Entity* castStationaryOrbitingMagicMissile(Entity* parent, int spellID, real_t centerx, real_t centery,
	real_t distFromCenter, real_t angleFromCenterDirection, int duration)
{
	spell_t* spell = getSpellFromID(spellID);
	if ( !parent )
	{
		Entity* entity = newEntity(-1, 1, map.entities, nullptr); //Particle entity.
		entity->sizex = 1;
		entity->sizey = 1;
		entity->x = centerx;
		entity->y = centery;
		entity->z = 15;
		entity->vel_z = 0;
		//entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
		entity->skill[0] = 100;
		entity->skill[1] = 10;
		entity->behavior = &actParticleDot;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[UNCLICKABLE] = true;
		entity->flags[INVISIBLE] = true;
		entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
			*cvar_magic_fx_light_bonus, 0.f);
		parent = entity;
	}
	Stat* stats = parent->getStats();
	Uint8 amplify = 0;
	if ( stats )
	{
		amplify = stats->getEffectActive(EFF_MAGICAMPLIFY);
		stats->clearEffect(EFF_MAGICAMPLIFY); // temporary skip amplify effects otherwise recursion.
	}
	Entity* entity = castSpell(parent->getUID(), spell, false, true);
	if ( stats )
	{
		stats->setEffectValueUnsafe(EFF_MAGICAMPLIFY, amplify);
	}
	if ( entity )
	{
		if ( spellID == SPELL_FIREBALL )
		{
			entity->sprite = 671;
		}
		else if ( spellID == SPELL_COLD )
		{
			entity->sprite = 797;
		}
		else if ( spellID == SPELL_LIGHTNING )
		{
			entity->sprite = 798;
		}
		else if ( spellID == SPELL_MAGICMISSILE )
		{
			entity->sprite = 679;
		}
		entity->yaw = angleFromCenterDirection;
		entity->x = centerx;
		entity->y = centery;
		entity->z = 4;
		double missile_speed = 4;
		entity->vel_x = 0.0;
		entity->vel_y = 0.0;
		entity->actmagicIsOrbiting = 2;
		entity->actmagicOrbitDist = distFromCenter;
		entity->actmagicOrbitStationaryCurrentDist = 0.0;
		entity->actmagicOrbitStartZ = entity->z;
		//entity->roll -= (PI / 8);
		entity->actmagicOrbitVerticalSpeed = -0.3;
		entity->actmagicOrbitVerticalDirection = 1;
		entity->actmagicOrbitLifetime = duration;
		entity->actmagicOrbitStationaryX = centerx;
		entity->actmagicOrbitStationaryY = centery;
		entity->vel_z = -0.1;
		playSoundEntity(entity, spellGetCastSound(spell), 128);

		//spawnMagicEffectParticles(entity->x, entity->y, 0, 174);
	}
	return entity;
}

void createParticleFollowerCommand(real_t x, real_t y, real_t z, int sprite, Uint32 uid)
{
	Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Particle entity.
	//entity->sizex = 1;
	//entity->sizey = 1;
	entity->x = x;
	entity->y = y;
	entity->z = 0;
	entity->vel_z = -0.8;
	//entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
	entity->skill[0] = 50;
	entity->skill[1] = 0;
	entity->parent = uid;
	entity->fskill[1] = entity->z; // start z
	entity->behavior = &actParticleFollowerCommand;
	entity->scalex = 0.8;
	entity->scaley = 0.8;
	entity->scalez = 0.8;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->flags[BRIGHT] = true;
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);

	// boosty boost
	for ( int c = 0; c < 10; c++ )
	{
		entity = newEntity(174, 1, map.entities, nullptr); //Particle entity.
		entity->x = x - 4 + local_rng.rand() % 9;
		entity->y = y - 4 + local_rng.rand() % 9;
		entity->z = z - 0 + local_rng.rand() % 11;
		entity->scalex = 0.5;
		entity->scaley = 0.5;
		entity->scalez = 0.5;
		entity->sizex = 1;
		entity->sizey = 1;
		entity->yaw = (local_rng.rand() % 360) * PI / 180.f;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[UNCLICKABLE] = true;
		entity->flags[BRIGHT] = true;
		entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
			*cvar_magic_fx_light_bonus, 0.f);
		entity->behavior = &actMagicParticle;
		entity->vel_z = -1;
		if ( multiplayer != CLIENT )
		{
			entity_uids--;
		}
		entity->setUID(-3);
	}

}

static ConsoleVariable<float> cvar_follower_particle_speed("/follower_particle_speed", 2.0);
void actParticleFollowerCommand(Entity* my)
{
	if ( PARTICLE_LIFE < 0 )
	{
		list_RemoveNode(my->mynode);
		return;
	}
	else
	{
		--PARTICLE_LIFE;
		if ( my->parent != 0 )
		{
			if ( Entity* parent = uidToEntity(my->parent) )
			{
				my->x = parent->x;
				my->y = parent->y;
			}
		}
		my->z += my->vel_z;
		my->yaw += (std::min(my->vel_z * 2, -0.1)) / *cvar_follower_particle_speed;
		if ( my->z < (my->fskill[1] - 3) )
		{
			my->skill[1] = 1;
			my->vel_z *= 0.9;
		}
		if ( my->skill[1] != 0 && abs(my->vel_z) < 0.1 )
		{
			my->z += .1 * sin(my->fskill[0]);
			my->fskill[0] += 1.0 * PI / 32;
			/*if ( my->fskill[0] > PI / 2 )
			{
				my->scalex *= .9;
				my->scaley *= .9;
				my->scalez *= .9;
			}*/
		}
	}
}

void actParticleShadowTag(Entity* my)
{
	if ( PARTICLE_LIFE < 0 )
	{
		// once off, fire some erupt dot particles at end of life.
		real_t yaw = 0;
		int numParticles = 8;
		for ( int c = 0; c < 8; c++ )
		{
			Entity* entity = newEntity(871, 1, map.entities, nullptr); //Particle entity.
			entity->sizex = 1;
			entity->sizey = 1;
			entity->x = my->x;
			entity->y = my->y;
			entity->z = -10 + my->fskill[0];
			entity->yaw = yaw;
			entity->vel_x = 0.2;
			entity->vel_y = 0.2;
			entity->vel_z = -0.02;
			entity->skill[0] = 100;
			entity->skill[1] = 0; // direction.
			entity->fskill[0] = 0.1;
			entity->behavior = &actParticleErupt;
			entity->flags[PASSABLE] = true;
			entity->flags[NOUPDATE] = true;
			entity->flags[UNCLICKABLE] = true;
			entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
				*cvar_magic_fx_light_bonus, 0.f);
			if ( multiplayer != CLIENT )
			{
				entity_uids--;
			}
			entity->setUID(-3);
			yaw += 2 * PI / numParticles;
		}

		if ( multiplayer != CLIENT )
		{
			Uint32 casterUid = static_cast<Uint32>(my->skill[2]);
			Entity* caster = uidToEntity(casterUid);
			Entity* parent = uidToEntity(my->parent);
			if ( caster && caster->behavior == &actPlayer
				&& parent )
			{
				// caster is alive, notify they lost their mark
				Uint32 color = makeColorRGB(255, 255, 255);
				if ( parent->getStats() )
				{
					messagePlayerMonsterEvent(caster->skill[2], color, *(parent->getStats()), Language::get(3466), Language::get(3467), MSG_COMBAT);
					parent->setEffect(EFF_SHADOW_TAGGED, false, 0, true);
				}
			}
		}
		my->removeLightField();
		list_RemoveNode(my->mynode);
		return;
	}
	else
	{
		--PARTICLE_LIFE;
		my->removeLightField();
		my->light = addLight(my->x / 16, my->y / 16, magicLightColorForSprite(my, my->sprite, true));

		Entity* parent = uidToEntity(my->parent);
		if ( parent )
		{
			my->x = parent->x;
			my->y = parent->y;
		}

		if ( my->skill[1] >= 50 ) // stop changing size
		{
			real_t maxspeed = .03;
			real_t acceleration = 0.95;
			if ( my->skill[3] == 0 ) 
			{
				// once off, store the normal height of the particle.
				my->skill[3] = 1;
				my->vel_z = -maxspeed;
			}
			if ( my->skill[1] % 5 == 0 )
			{
				Uint32 casterUid = static_cast<Uint32>(my->skill[2]);
				Entity* caster = uidToEntity(casterUid);
				if ( caster && caster->creatureShadowTaggedThisUid == my->parent && parent )
				{
					// caster is alive, and they have still marked the parent this particle is following.
				}
				else
				{
					PARTICLE_LIFE = 0;
				}
			}

			if ( PARTICLE_LIFE > 0 && PARTICLE_LIFE < TICKS_PER_SECOND )
			{
				if ( parent && parent->getStats() && parent->getStats()->getEffectActive(EFF_SHADOW_TAGGED) )
				{
					++PARTICLE_LIFE;
				}
			}
			// bob up and down movement.
			if ( my->skill[3] == 1 )
			{
				my->vel_z *= acceleration;
				if ( my->vel_z > -0.005 )
				{
					my->skill[3] = 2;
					my->vel_z = -0.005;
				}
				my->z += my->vel_z;
			}
			else if ( my->skill[3] == 2 )
			{
				my->vel_z /= acceleration;
				if ( my->vel_z < -maxspeed )
				{
					my->skill[3] = 3;
					my->vel_z = -maxspeed;
				}
				my->z -= my->vel_z;
			}
			else if ( my->skill[3] == 3 )
			{
				my->vel_z *= acceleration;
				if ( my->vel_z > -0.005 )
				{
					my->skill[3] = 4;
					my->vel_z = -0.005;
				}
				my->z -= my->vel_z;
			}
			else if ( my->skill[3] == 4 )
			{
				my->vel_z /= acceleration;
				if ( my->vel_z < -maxspeed )
				{
					my->skill[3] = 1;
					my->vel_z = -maxspeed;
				}
				my->z += my->vel_z;
			}
			my->yaw += 0.01;
		}
		else
		{
			my->z += my->vel_z;
			my->yaw += my->vel_z * 2;
			if ( my->scalex < 0.5 )
			{
				my->scalex += 0.02;
			}
			else
			{
				my->scalex = 0.5;
			}
			my->scaley = my->scalex;
			my->scalez = my->scalex;
			if ( my->z < -3 + my->fskill[0] )
			{
				my->vel_z *= 0.9;
			}
		}

		// once off, fire some erupt dot particles at start.
		if ( my->skill[1] == 0 )
		{
			real_t yaw = 0;
			int numParticles = 8;
			for ( int c = 0; c < 8; c++ )
			{
				Entity* entity = newEntity(871, 1, map.entities, nullptr); //Particle entity.
				entity->sizex = 1;
				entity->sizey = 1;
				entity->x = my->x;
				entity->y = my->y;
				entity->z = -10 + my->fskill[0];
				entity->yaw = yaw;
				entity->vel_x = 0.2;
				entity->vel_y = 0.2;
				entity->vel_z = -0.02;
				entity->skill[0] = 100;
				entity->skill[1] = 0; // direction.
				entity->fskill[0] = 0.1;
				entity->behavior = &actParticleErupt;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				entity->flags[UNCLICKABLE] = true;
				entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
					*cvar_magic_fx_light_bonus, 0.f);
				if ( multiplayer != CLIENT )
				{
					entity_uids--;
				}
				entity->setUID(-3);
				yaw += 2 * PI / numParticles;
			}
		}
		++my->skill[1];
	}
}

void createParticleShadowTag(Entity* parent, Uint32 casterUid, int duration)
{
	if ( !parent )
	{
		return;
	}
	Entity* entity = newEntity(870, 1, map.entities, nullptr); //Particle entity.
	entity->parent = parent->getUID();
	entity->x = parent->x;
	entity->y = parent->y;
	entity->z = 7.5;
	entity->fskill[0] = parent->z;
	entity->vel_z = -0.8;
	entity->scalex = 0.1;
	entity->scaley = 0.1;
	entity->scalez = 0.1;
	entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
	entity->skill[0] = duration;
	entity->skill[1] = 0;
	entity->skill[2] = static_cast<Sint32>(casterUid);
	entity->skill[3] = 0;
	entity->behavior = &actParticleShadowTag;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;
	/*entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
		*cvar_magic_fx_light_bonus, 0.f);*/
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);
}

void actParticlePinpointTarget(Entity* my)
{
	Sint32& spellID = my->skill[4];

	if ( PARTICLE_LIFE < 0 )
	{
		// once off, fire some erupt dot particles at end of life.
		real_t yaw = 0;

		for ( int i = 0; i < 4; ++i )
		{
			Entity* fx = spawnMagicParticle(my);
			fx->vel_x = 0.5 * cos(my->yaw + i * PI / 2);
			fx->vel_y = 0.5 * sin(my->yaw + i * PI / 2);
			fx->fskill[1] = 0.05;
			fx->yaw = my->yaw + i * PI / 2;
			fx->scalex = 0.7;
			fx->scaley = fx->scalex;
			fx->scalez = fx->scalex;
		}

		/*Entity* fx = createParticleAestheticOrbit(my, my->sprite, TICKS_PER_SECOND / 4, PARTICLE_EFFECT_NULL_PARTICLE_NOSOUND);
		fx->x = my->x;
		fx->y = my->y;
		fx->z = my->z;
		fx->yaw = my->yaw;
		fx->actmagicOrbitDist = 0;
		fx->actmagicNoLight = 1;*/

		//int numParticles = 8;
		//for ( int c = 0; c < 8; c++ )
		//{
		//	Entity* entity = newEntity(my->skill[5], 1, map.entities, nullptr); //Particle entity.
		//	entity->sizex = 1;
		//	entity->sizey = 1;
		//	entity->x = my->x;
		//	entity->y = my->y;
		//	entity->z = -10 + my->fskill[0];
		//	entity->yaw = yaw;
		//	entity->vel_x = 0.2;
		//	entity->vel_y = 0.2;
		//	entity->vel_z = -0.02;
		//	entity->skill[0] = 100;
		//	entity->skill[1] = 0; // direction.
		//	entity->fskill[0] = 0.1;
		//	entity->scalex = 0.1;
		//	entity->scaley = 0.1;
		//	entity->scalez = 0.1;
		//	entity->behavior = &actParticleErupt;
		//	entity->flags[PASSABLE] = true;
		//	entity->flags[NOUPDATE] = true;
		//	entity->flags[UNCLICKABLE] = true;
		//	entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
		//		*cvar_magic_fx_light_bonus, 0.f);
		//	if ( multiplayer != CLIENT )
		//	{
		//		entity_uids--;
		//	}
		//	entity->setUID(-3);
		//	yaw += 2 * PI / numParticles;
		//}

		if ( multiplayer != CLIENT )
		{
			Uint32 casterUid = static_cast<Uint32>(my->skill[2]);
			Entity* caster = uidToEntity(casterUid);
			Entity* parent = uidToEntity(my->parent);
			if ( caster && caster->behavior == &actPlayer
				&& parent )
			{
				// caster is alive, notify they lost their mark
				//Uint32 color = makeColorRGB(255, 255, 255);
				//messagePlayerMonsterEvent(caster->skill[2], color, *(parent->getStats()), Language::get(3466), Language::get(3467), MSG_COMBAT);
			}
			if ( parent && parent->getStats() )
			{
				if ( spellID == SPELL_PINPOINT )
				{
					parent->setEffect(EFF_PINPOINT, false, 0, true);
				}
				else if ( spellID == SPELL_PENANCE )
				{
					parent->setEffect(EFF_PENANCE, false, 0, true);
				}
				else if ( spellID == SPELL_TABOO )
				{
					parent->setEffect(EFF_TABOO, false, 0, true);
				}
				else if ( spellID == SPELL_DETECT_ENEMY || spellID == SPELL_DETECT_ENEMIES )
				{
					parent->setEffect(EFF_DETECT_ENEMY, false, 0, true);
				}
			}
		}
		my->removeLightField();
		list_RemoveNode(my->mynode);
		return;
	}
	else
	{
		Entity* parent = uidToEntity(my->parent);
		if ( spellID == SPELL_DONATION )
		{
			// don't decay
			if ( parent && parent->behavior == &actColliderDecoration )
			{
				Entity* donation = uidToEntity(parent->colliderContainedEntity);
				if ( donation )
				{
					my->parent = parent->colliderContainedEntity;
					parent = donation;
				}
			}
		}
		else
		{
			--PARTICLE_LIFE;
		}
		my->removeLightField();
		my->light = addLight(my->x / 16, my->y / 16, magicLightColorForSprite(my, my->sprite, true));

		if ( parent )
		{
			my->x = parent->x;
			my->y = parent->y;
			Entity::EntityShowMapSource mapSource = Entity::SHOW_MAP_SCRY;
			if ( spellID == SPELL_DONATION )
			{
				mapSource = Entity::SHOW_MAP_DONATION;
			}
			else if ( spellID == SPELL_PINPOINT )
			{
				mapSource = Entity::SHOW_MAP_PINPOINT;
			}
			else if ( spellID == SPELL_DETECT_ENEMY || spellID == SPELL_DETECT_ENEMIES )
			{
				mapSource = Entity::SHOW_MAP_DETECT_MONSTER;
			}

			if ( parent->getEntityShowOnMapDuration() == 0
				|| (parent->getEntityShowOnMapSource() == mapSource) )
			{
				parent->setEntityShowOnMap(mapSource, std::max(parent->getEntityShowOnMapDuration(), 5));
			}

			if ( multiplayer != CLIENT )
			{
				if ( spellID == SPELL_SCRY_TREASURES )
				{
					if ( parent->behavior == &actChest )
					{
						if ( parent->chestVoidState != 0 )
						{
							PARTICLE_LIFE = -1;
						}
						else if ( list_t* inventory = parent->getChestInventoryList() )
						{
							if ( !inventory->first )
							{
								PARTICLE_LIFE = -1;
							}
						}
					}
				}
			}
		}

		if ( my->skill[1] >= 50 ) // stop changing size
		{
			real_t maxspeed = .03;
			real_t acceleration = 0.95;
			if ( my->skill[3] == 0 )
			{
				// once off, store the normal height of the particle.
				my->skill[3] = 1;
				my->vel_z = -maxspeed;
			}
			if ( my->skill[1] % 5 == 0 )
			{
				Uint32 casterUid = static_cast<Uint32>(my->skill[2]);
				Entity* caster = uidToEntity(casterUid);
				if ( caster && parent )
				{
					// caster is alive, and they have still marked the parent this particle is following.
					if ( spellID == SPELL_PENANCE )
					{
						if ( parent->monsterAllyGetPlayerLeader() )
						{
							PARTICLE_LIFE = -1;
							if ( multiplayer != CLIENT )
							{
								parent->setEffect(SPELL_PENANCE, false, 0, true);
							}
						}
						if ( Stat* parentStats = parent->getStats() )
						{
							if ( !parentStats->getEffectActive(EFF_PENANCE) )
							{
								PARTICLE_LIFE = -1;
							}
						}
					}
					else if ( spellID == SPELL_DETECT_ENEMY || spellID == SPELL_DETECT_ENEMIES )
					{
						if ( Stat* parentStats = parent->getStats() )
						{
							if ( !parentStats->getEffectActive(EFF_DETECT_ENEMY) )
							{
								PARTICLE_LIFE = -1;
							}
						}
					}
					else if ( spellID == SPELL_TABOO )
					{
						if ( Stat* parentStats = parent->getStats() )
						{
							if ( !parentStats->getEffectActive(EFF_TABOO) )
							{
								PARTICLE_LIFE = -1;
							}
						}
					}
				}
				else
				{
					PARTICLE_LIFE = -1;
				}
			}

			if ( PARTICLE_LIFE > 0 && PARTICLE_LIFE < TICKS_PER_SECOND )
			{
				if ( spellID == SPELL_PINPOINT )
				{
					if ( parent && parent->getStats() && parent->getStats()->getEffectActive(EFF_PINPOINT) )
					{
						++PARTICLE_LIFE;
					}
				}
				else if ( spellID == SPELL_PENANCE )
				{
					if ( parent && parent->getStats() && parent->getStats()->getEffectActive(EFF_PENANCE) )
					{
						++PARTICLE_LIFE;
					}
				}
				else if ( spellID == SPELL_TABOO )
				{
					if ( parent && parent->getStats() && parent->getStats()->getEffectActive(EFF_TABOO) )
					{
						++PARTICLE_LIFE;
					}
				}
				else if ( spellID == SPELL_DETECT_ENEMY || spellID == SPELL_DETECT_ENEMIES )
				{
					if ( parent && parent->getStats() && parent->getStats()->getEffectActive(EFF_DETECT_ENEMY) )
					{
						++PARTICLE_LIFE;
					}
				}
				else
				{
					/*if ( parent && parent->getStats() && parent->getStats()->getEffectActive(EFF_SHADOW_TAGGED) )
					{
						++PARTICLE_LIFE;
					}*/
				}
			}
			// bob up and down movement.
			if ( my->skill[3] == 1 )
			{
				my->vel_z *= acceleration;
				if ( my->vel_z > -0.005 )
				{
					my->skill[3] = 2;
					my->vel_z = -0.005;
				}
				my->z += my->vel_z;
			}
			else if ( my->skill[3] == 2 )
			{
				my->vel_z /= acceleration;
				if ( my->vel_z < -maxspeed )
				{
					my->skill[3] = 3;
					my->vel_z = -maxspeed;
				}
				my->z -= my->vel_z;
			}
			else if ( my->skill[3] == 3 )
			{
				my->vel_z *= acceleration;
				if ( my->vel_z > -0.005 )
				{
					my->skill[3] = 4;
					my->vel_z = -0.005;
				}
				my->z -= my->vel_z;
			}
			else if ( my->skill[3] == 4 )
			{
				my->vel_z /= acceleration;
				if ( my->vel_z < -maxspeed )
				{
					my->skill[3] = 1;
					my->vel_z = -maxspeed;
				}
				my->z += my->vel_z;
			}
			my->yaw += 0.01;
		}
		else
		{
			my->z += my->vel_z;
			my->yaw += my->vel_z * 2;
			if ( my->scalex < 0.5 )
			{
				my->scalex += 0.02;
			}
			else
			{
				my->scalex = 0.5;
			}
			my->scaley = my->scalex;
			my->scalez = my->scalex;
			if ( my->z < -3 + my->fskill[0] )
			{
				my->vel_z *= 0.9;
			}
		}

		// once off, fire some erupt dot particles at start.
		if ( my->skill[1] == 0 )
		{
			for ( int i = 0; i < 4; ++i )
			{
				Entity* fx = spawnMagicParticle(my);
				fx->vel_x = 0.5 * cos(my->yaw + i * PI / 2);
				fx->vel_y = 0.5 * sin(my->yaw + i * PI / 2);
				fx->fskill[1] = 0.05;
				fx->yaw = my->yaw + i * PI / 2;
				fx->scalex = 0.7;
				fx->scaley = fx->scalex;
				fx->scalez = fx->scalex;
			}
			//real_t yaw = 0;
			//int numParticles = 8;
			//for ( int c = 0; c < 8; c++ )
			//{
			//	Entity* entity = newEntity(my->skill[5], 1, map.entities, nullptr); //Particle entity.
			//	entity->sizex = 1;
			//	entity->sizey = 1;
			//	entity->x = my->x;
			//	entity->y = my->y;
			//	entity->z = -10 + my->fskill[0];
			//	entity->yaw = yaw;
			//	entity->vel_x = 0.2;
			//	entity->vel_y = 0.2;
			//	entity->vel_z = -0.02;
			//	entity->skill[0] = 100;
			//	entity->skill[1] = 0; // direction.
			//	entity->fskill[0] = 0.1;
			//	entity->scalex = 0.1;
			//	entity->scaley = 0.1;
			//	entity->scalez = 0.1;
			//	entity->behavior = &actParticleErupt;
			//	entity->flags[PASSABLE] = true;
			//	entity->flags[NOUPDATE] = true;
			//	entity->flags[UNCLICKABLE] = true;
			//	entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
			//		*cvar_magic_fx_light_bonus, 0.f);
			//	if ( multiplayer != CLIENT )
			//	{
			//		entity_uids--;
			//	}
			//	entity->setUID(-3);
			//	yaw += 2 * PI / numParticles;
			//}
		}
		++my->skill[1];
	}
}

void createParticleSpellPinpointTarget(Entity* parent, Uint32 casterUid, int sprite, int duration, int spellID)
{
	if ( !parent )
	{
		return;
	}
	Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Particle entity.
	entity->parent = parent->getUID();
	entity->x = parent->x;
	entity->y = parent->y;
	entity->z = 7.5;
	static ConsoleVariable<float> cvar_pinpoint_z("/pinpoint_z", 0.0);
	entity->fskill[0] = parent->z + *cvar_pinpoint_z;
	if ( parent->behavior == &actBoulderTrapHole )
	{
		entity->fskill[0] += 15.0;
	}
	entity->vel_z = -0.8;
	entity->scalex = 0.1;
	entity->scaley = 0.1;
	entity->scalez = 0.1;
	entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
	entity->skill[0] = duration;
	entity->skill[1] = 0;
	entity->skill[2] = static_cast<Sint32>(casterUid);
	entity->skill[3] = 0;
	entity->skill[4] = spellID;
	entity->skill[5] = sprite;// +(PINPOINT_PARTICLE_END - PINPOINT_PARTICLE_START); // start/end particle drops
	entity->behavior = &actParticlePinpointTarget;
	entity->ditheringOverride = 6;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->flags[ENTITY_SKIP_CULLING] = true;
	entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
		*cvar_magic_fx_light_bonus, 0.f);
	TileEntityList.addEntity(*entity);
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);

	for ( auto node = map.entities->first; node; node = node->next )
	{
		if ( Entity* entity2 = (Entity*)node->element )
		{
			if ( entity2->behavior == &actParticlePinpointTarget
				&& entity2 != entity
				&& entity2->parent == entity->parent )
			{
				//if ( entity2->skill[4] == spellID )
				{
					entity2->skill[0] = -1; // kill off existing particle
				}
			}
		}
	}
}

void createParticleCharmMonster(Entity* parent)
{
	if ( !parent )
	{
		return;
	}
	Entity* entity = newEntity(685, 1, map.entities, nullptr); //Particle entity.
	//entity->sizex = 1;
	//entity->sizey = 1;
	entity->parent = parent->getUID();
	entity->x = parent->x;
	entity->y = parent->y;
	entity->z = 7.5;
	entity->vel_z = -0.8;
	entity->scalex = 0.1;
	entity->scaley = 0.1;
	entity->scalez = 0.1;
	entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
	entity->skill[0] = 45;
	entity->behavior = &actParticleCharmMonster;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
		*cvar_magic_fx_light_bonus, 0.f);
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);
}

void actParticleCharmMonster(Entity* my)
{
	if ( PARTICLE_LIFE < 0 )
	{
		real_t yaw = 0;
		int numParticles = 8;
		for ( int c = 0; c < 8; c++ )
		{
			Entity* entity = newEntity(576, 1, map.entities, nullptr); //Particle entity.
			entity->sizex = 1;
			entity->sizey = 1;
			entity->x = my->x;
			entity->y = my->y;
			entity->z = -10;
			entity->yaw = yaw;
			entity->vel_x = 0.2;
			entity->vel_y = 0.2;
			entity->vel_z = -0.02;
			entity->skill[0] = 100;
			entity->skill[1] = 0; // direction.
			entity->fskill[0] = 0.1;
			entity->behavior = &actParticleErupt;
			entity->flags[PASSABLE] = true;
			entity->flags[NOUPDATE] = true;
			entity->flags[UNCLICKABLE] = true;
			entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
				*cvar_magic_fx_light_bonus, 0.f);
			if ( multiplayer != CLIENT )
			{
				entity_uids--;
			}
			entity->setUID(-3);
			yaw += 2 * PI / numParticles;
		}
		list_RemoveNode(my->mynode);
		return;
	}
	else
	{
		--PARTICLE_LIFE;
		Entity* parent = uidToEntity(my->parent);
		if ( parent )
		{
			my->x = parent->x;
			my->y = parent->y;
		}
		my->z += my->vel_z;
		my->yaw += my->vel_z * 2;
		if ( my->scalex < 0.8 )
		{
			my->scalex += 0.02;
		}
		else
		{
			my->scalex = 0.8;
		}
		my->scaley = my->scalex;
		my->scalez = my->scalex;
		if ( my->z < -3 )
		{
			my->vel_z *= 0.9;
		}
	}
}

void spawnMagicTower(Entity* parent, real_t x, real_t y, int spellID, Entity* autoHitTarget, bool castedSpell)
{
	bool autoHit = false;
	if ( autoHitTarget && (autoHitTarget->behavior == &actPlayer || autoHitTarget->behavior == &actMonster) )
	{
		autoHit = true;
		if ( parent )
		{
			if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) && parent->checkFriend(autoHitTarget) && parent->friendlyFireProtection(autoHitTarget) )
			{
				autoHit = false; // don't hit friendlies
			}
		}
	}
	Entity* orbit = castStationaryOrbitingMagicMissile(parent, spellID, x, y, 16.0, 0.0, 40);
	if ( orbit )
	{
		orbit->actmagicUpdateOLDHPOnHit = 1;
		if ( castedSpell )
		{
			orbit->actmagicOrbitCastFromSpell = 1;
		}
		if ( autoHit )
		{
			orbit->actmagicOrbitHitTargetUID4 = autoHitTarget->getUID();
		}
	}
	orbit = castStationaryOrbitingMagicMissile(parent, spellID, x, y, 16.0, 2 * PI / 3, 40);
	if ( orbit )
	{
		orbit->actmagicUpdateOLDHPOnHit = 1;
		if ( castedSpell )
		{
			orbit->actmagicOrbitCastFromSpell = 1;
		}
		if ( autoHit )
		{
			orbit->actmagicOrbitHitTargetUID4 = autoHitTarget->getUID();
		}
	}
	orbit = castStationaryOrbitingMagicMissile(parent, spellID, x, y, 16.0, 4 * PI / 3, 40);
	if ( orbit )
	{
		orbit->actmagicUpdateOLDHPOnHit = 1;
		if ( castedSpell )
		{
			orbit->actmagicOrbitCastFromSpell = 1;
		}
		if ( autoHit )
		{
			orbit->actmagicOrbitHitTargetUID4 = autoHitTarget->getUID();
		}
	}
	spawnMagicEffectParticles(x, y, 0, 174);
	spawnExplosion(x, y, -4 + local_rng.rand() % 8);
}

bool magicDig(Entity* parent, Entity* projectile, int numRocks, int randRocks)
{
	if ( !hit.entity )
	{
		if ( map.tiles[(int)(OBSTACLELAYER + hit.mapy * MAPLAYERS + hit.mapx * MAPLAYERS * map.height)] != 0 )
		{
			if ( MFLAG_DISABLEDIGGING )
			{
				if ( parent && parent->behavior == &actPlayer )
				{
					Uint32 color = makeColorRGB(255, 0, 255);
					messagePlayerColor(parent->skill[2], MESSAGE_HINT, color, Language::get(2380)); // disabled digging.
				}
				playSoundPos(hit.x, hit.y, 66, 128); // strike wall
			}
			else if ( swimmingtiles[map.tiles[OBSTACLELAYER + hit.mapy * MAPLAYERS + hit.mapx * MAPLAYERS * map.height]]
				|| lavatiles[map.tiles[OBSTACLELAYER + hit.mapy * MAPLAYERS + hit.mapx * MAPLAYERS * map.height]] )
			{
				// no effect for lava/water tiles.
			}
			else if ( !mapTileDiggable(hit.mapx, hit.mapy) )
			{
				if ( parent && parent->behavior == &actPlayer )
				{
					messagePlayer(parent->skill[2], MESSAGE_HINT, Language::get(706));
				}
				playSoundPos(hit.x, hit.y, 66, 128); // strike wall
			}
			else
			{
				if ( projectile )
				{
					playSoundEntity(projectile, 66, 128);
					playSoundEntity(projectile, 67, 128);
				}

				// spawn several rock items
				if ( randRocks <= 0 )
				{
					randRocks = 1;
				}
				int i = numRocks + local_rng.rand() % randRocks;
				for ( int c = 0; c < i; c++ )
				{
					Entity* rock = newEntity(-1, 1, map.entities, nullptr); //Rock entity.
					rock->flags[INVISIBLE] = true;
					rock->flags[UPDATENEEDED] = true;
					rock->x = hit.mapx * 16 + 4 + local_rng.rand() % 8;
					rock->y = hit.mapy * 16 + 4 + local_rng.rand() % 8;
					rock->z = -6 + local_rng.rand() % 12;
					rock->sizex = 4;
					rock->sizey = 4;
					rock->yaw = local_rng.rand() % 360 * PI / 180;
					rock->vel_x = (local_rng.rand() % 20 - 10) / 10.0;
					rock->vel_y = (local_rng.rand() % 20 - 10) / 10.0;
					rock->vel_z = -.25 - (local_rng.rand() % 5) / 10.0;
					rock->flags[PASSABLE] = true;
					rock->behavior = &actItem;
					rock->flags[USERFLAG1] = true; // no collision: helps performance
					rock->skill[10] = GEM_ROCK;    // type
					rock->skill[11] = WORN;        // status
					rock->skill[12] = 0;           // beatitude
					rock->skill[13] = 1;           // count
					rock->skill[14] = 0;           // appearance
					rock->skill[15] = 1;		   // identified
				}

				if ( map.tiles[(int)(OBSTACLELAYER + hit.mapy * MAPLAYERS + hit.mapx * MAPLAYERS * map.height)] >= 41
					&& map.tiles[(int)(OBSTACLELAYER + hit.mapy * MAPLAYERS + hit.mapx * MAPLAYERS * map.height)] <= 49 )
				{
					steamAchievementEntity(parent, "BARONY_ACH_BAD_REVIEW");
				}

				map.tiles[(int)(OBSTACLELAYER + hit.mapy * MAPLAYERS + hit.mapx * MAPLAYERS * map.height)] = 0;

				// send wall destroy info to clients
				if ( multiplayer == SERVER )
				{
					for ( int c = 1; c < MAXPLAYERS; c++ )
					{
						if ( players[c]->isLocalPlayer() || client_disconnected[c] == true )
						{
							continue;
						}
						strcpy((char*)net_packet->data, "WALD");
						SDLNet_Write16((Uint16)hit.mapx, &net_packet->data[4]);
						SDLNet_Write16((Uint16)hit.mapy, &net_packet->data[6]);
						net_packet->address.host = net_clients[c - 1].host;
						net_packet->address.port = net_clients[c - 1].port;
						net_packet->len = 8;
						sendPacketSafe(net_sock, -1, net_packet, c - 1);
					}
				}

				if ( projectile && projectile->behavior == &actMagicMissile )
				{
					magicOnSpellCastEvent(parent, projectile, nullptr, SPELL_DIG, spell_t::SpellOnCastEventTypes::SPELL_LEVEL_EVENT_DEFAULT, 1);
				}

				generatePathMaps();
				return true;
			}
		}
		return false;
	}
	else if ( hit.entity->behavior == &actColliderDecoration && hit.entity->colliderDiggable != 0 )
	{
		int sprite = EditorEntityData_t::colliderData[hit.entity->colliderDamageTypes].gib;
		if ( sprite > 0 )
		{
			createParticleRock(hit.entity, sprite);
			if ( multiplayer == SERVER )
			{
				serverSpawnMiscParticles(hit.entity, PARTICLE_EFFECT_ABILITY_ROCK, sprite);
			}
		}

		hit.entity->colliderOnDestroy();
		if ( parent )
		{
			if ( parent->behavior == &actPlayer && hit.entity->isDamageableCollider() )
			{
				messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(4337),
					Language::get(hit.entity->getColliderLangName())); // you destroy the %s!
				if ( hit.entity->isColliderWall() )
				{
					Compendium_t::Events_t::eventUpdateWorld(parent->skill[2], Compendium_t::CPDM_BARRIER_DESTROYED, "breakable barriers", 1);
				}
			}
		}

		if ( projectile && projectile->behavior == &actMagicMissile )
		{
			magicOnEntityHit(parent, projectile, hit.entity, nullptr, 0, 0, 0, SPELL_DIG);
		}

		// destroy the object
		playSoundEntity(hit.entity, 67, 128);
		list_RemoveNode(hit.entity->mynode);
		return true;
	}
	else if ( hit.entity->behavior == &::actDaedalusShrine )
	{
		createParticleRock(hit.entity);
		if ( multiplayer == SERVER )
		{
			serverSpawnMiscParticles(hit.entity, PARTICLE_EFFECT_ABILITY_ROCK, 78);
		}

		if ( projectile && projectile->behavior == &actMagicMissile )
		{
			magicOnEntityHit(parent, projectile, hit.entity, nullptr, 0, 0, 0, SPELL_DIG);
		}

		playSoundEntity(hit.entity, 67, 128);
		list_RemoveNode(hit.entity->mynode);
	}
	else if ( hit.entity->behavior == &actBoulder )
	{
		int i = numRocks + local_rng.rand() % 4;

		// spawn several rock items //TODO: This should really be its own function.
		for ( int c = 0; c < i; c++ )
		{
			Entity* entity = newEntity(-1, 1, map.entities, nullptr); //Rock entity.
			entity->flags[INVISIBLE] = true;
			entity->flags[UPDATENEEDED] = true;
			entity->x = hit.entity->x - 4 + local_rng.rand() % 8;
			entity->y = hit.entity->y - 4 + local_rng.rand() % 8;
			entity->z = -6 + local_rng.rand() % 12;
			entity->sizex = 4;
			entity->sizey = 4;
			entity->yaw = local_rng.rand() % 360 * PI / 180;
			entity->vel_x = (local_rng.rand() % 20 - 10) / 10.0;
			entity->vel_y = (local_rng.rand() % 20 - 10) / 10.0;
			entity->vel_z = -.25 - (local_rng.rand() % 5) / 10.0;
			entity->flags[PASSABLE] = true;
			entity->behavior = &actItem;
			entity->flags[USERFLAG1] = true; // no collision: helps performance
			entity->skill[10] = GEM_ROCK;    // type
			entity->skill[11] = WORN;        // status
			entity->skill[12] = 0;           // beatitude
			entity->skill[13] = 1;           // count
			entity->skill[14] = 0;           // appearance
			entity->skill[15] = false;       // identified
		}

		double ox = hit.entity->x;
		double oy = hit.entity->y;

		boulderLavaOrArcaneOnDestroy(hit.entity, hit.entity->sprite, nullptr);

		auto& rng = hit.entity->entity_rng ? *hit.entity->entity_rng : local_rng;
		Uint32 monsterSpawnSeed = rng.getU32();

		if ( projectile && projectile->behavior == &actMagicMissile )
		{
			magicOnEntityHit(parent, projectile, hit.entity, nullptr, 0, 0, 0, SPELL_DIG);
		}

		// destroy the boulder
		playSoundEntity(hit.entity, 67, 128);
		list_RemoveNode(hit.entity->mynode);
		if ( parent )
		{
			if ( parent->behavior == &actPlayer )
			{
				messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(405));
			}
		}

		// on sokoban, destroying boulders spawns scorpions
		if ( !strcmp(map.name, "Sokoban") )
		{
			Entity* monster = nullptr;
			if ( local_rng.rand() % 2 == 0 )
			{
				monster = summonMonster(INSECTOID, ox, oy);
			}
			else
			{
				monster = summonMonster(SCORPION, ox, oy);
			}
			if ( monster )
			{
				monster->seedEntityRNG(monsterSpawnSeed);
				for ( int c = 0; c < MAXPLAYERS; c++ )
				{
					Uint32 color = makeColorRGB(255, 128, 0);
					messagePlayerColor(c, MESSAGE_HINT, color, Language::get(406));
				}
			}
			boulderSokobanOnDestroy(false);
		}
		return true;
	}
	return false;
}

Entity* createParticleCastingIndicator(Entity* parent, real_t x, real_t y, real_t z, Uint32 lifetime, Uint32 followUid)
{
	Uint32 uid = 0;
	if ( parent )
	{
		uid = parent->getUID();
	}

	Entity* entity = newEntity(222, 1, map.entities, nullptr); //Sprite entity.
	entity->x = x;
	entity->y = y;
	entity->z = 7.470;
	static ConsoleVariable<float> cvar_sprite_cast_indicator_scale("/sprite_cast_indicator_scale", 0.025);
	static ConsoleVariable<float> cvar_sprite_cast_indicator_rotate("/sprite_cast_indicator_rotate", 0.025);
	static ConsoleVariable<float> cvar_sprite_cast_indicator_alpha("/sprite_cast_indicator_alpha", 0.5);
	static ConsoleVariable<float> cvar_sprite_cast_indicator_alpha_glow("/sprite_cast_indicator_alpha_glow", 0.0625);
	entity->ditheringDisabled = true;
	entity->flags[SPRITE] = true;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->flags[BRIGHT] = true;
	entity->scalex = *cvar_sprite_cast_indicator_scale;
	entity->scaley = *cvar_sprite_cast_indicator_scale;
	entity->behavior = &actSprite;
	entity->yaw = local_rng.rand() % 360 * PI / 180;
	entity->pitch = 0;
	entity->roll = -PI / 2;
	entity->skill[0] = 1;
	entity->skill[1] = 1;
	entity->skill[2] = lifetime;
	entity->fskill[0] = *cvar_sprite_cast_indicator_rotate;
	entity->fskill[2] = *cvar_sprite_cast_indicator_alpha; // alpha
	entity->fskill[3] = *cvar_sprite_cast_indicator_alpha_glow;
	entity->actSpriteUseAlpha = 1; // use alpha
	entity->actSpriteNoBillboard = 1; // no billboard
	entity->actSpriteCheckParentExists = uid;
	entity->flags[ENTITY_SKIP_CULLING] = true;
	if ( followUid )
	{
		entity->actSpriteFollowUID = followUid;
	}
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);
	return entity;
}

void AOEIndicators_t::cleanup()
{
	indicators.clear();
	
	for ( auto& m1 : surfaceCache )
	{
		for ( auto& m2 : m1.second )
		{
			SDL_FreeSurface(m2.second);
			m2.second = nullptr;
		}
	}
	surfaceCache.clear();
}
std::map<Uint32, AOEIndicators_t::Indicator_t> AOEIndicators_t::indicators;
Uint32 AOEIndicators_t::uids = 1;
void AOEIndicators_t::update()
{
	std::vector<Uint32> toDelete;
	for ( auto& i : indicators )
	{
		i.second.lifetime--;

		if ( i.second.castingTarget ) { i.second.lifetime = 1; } // never expire

		if ( i.second.lifetime <= 0 )
		{
			toDelete.push_back(i.second.uid);
		}
		else if ( i.second.expired )
		{
			Uint8 alpha = 0;
			getColor(i.second.indicatorColor, nullptr, nullptr, nullptr, &alpha);
			if ( alpha == 0 )
			{
				toDelete.push_back(i.second.uid);
			}
			else
			{
				i.second.updateIndicator();
			}
		}
		else
		{
			i.second.updateIndicator();
		}
	}

	for ( auto& uid : toDelete )
	{
		indicators.erase(uid);
	}
}

Uint32 AOEIndicators_t::createIndicator(int _radiusMin, int _radiusMax, int _size, int _lifetime)
{
	Uint32 uid = uids;
	uids++;
	indicators.insert(std::make_pair(uid, Indicator_t(_radiusMin, _radiusMax, _size, _lifetime, uid)));
	return uid;
}

std::map<int, std::map<std::tuple<Uint8, Uint8, Uint8, Uint8, real_t, real_t, int>, SDL_Surface*>> AOEIndicators_t::surfaceCache;
void AOEIndicators_t::Indicator_t::updateIndicator()
{
	if ( !(!gamePaused || (multiplayer && !client_disconnected[0])) )
	{
		// paused game
		return;
	}

	if ( delayTicks > 0 )
	{
		--delayTicks;
		return;
	}

	//auto t1 = std::chrono::high_resolution_clock::now();

	const int ringSize = 1;
	const int gradientSize = gradient;
	int ring = radius - ringSize;
	int center = size / 2;
	//for ( int r = std::max(0, radius - gradientSize); r <= radius; r += 1 )
	//{
	//	/*real_t minAngle = 0.9 * acos(1.0 - 1.0 / r);
	//	for ( real_t i = 0; i < 360; i += minAngle )
	//	{
	//		x = (size / 2) + r * std::min(0.9999, cos(i));
	//		y = (size / 2) + r * std::min(0.9999, sin(i));
	//		assert(x >= 0 && x < size);
	//		assert(y >= 0 && y < size);

	//		if ( r < ring )
	//		{
	//			real_t alphaRatio = std::min(1.0, std::max(0.0, 1.0 + (ringSize + r - radius) / (real_t)gradientSize));
	//			putPixel(surfaceNew, x, y, makeColor(255, 255, 255, std::min(255.0, 255 * alphaRatio)));
	//		}
	//		else if ( r == ring )
	//		{
	//			putPixel(surfaceNew, x, y, makeColor(255, 255, 255, 255));
	//		}
	//		else
	//		{
	//			putPixel(surfaceNew, x, y, makeColor(212, 212, 212, 255));
	//		}
	//	}*/

	//	/*x = r;
	//	y = 0;
	//	int error = 3 - 2 * radius;
	//	while ( x >= y )
	//	{
	//		putPixel(surfaceNew, center + x, center + y, makeColor(255, 255, 255, 255));
	//		putPixel(surfaceNew, center + x, center - y, makeColor(255, 255, 255, 255));
	//		putPixel(surfaceNew, center -x, center + y, makeColor(255, 255, 255, 255));
	//		putPixel(surfaceNew, center -x, center - y, makeColor(255, 255, 255, 255));

	//		putPixel(surfaceNew, center + y, center + x, makeColor(255, 255, 255, 255));
	//		putPixel(surfaceNew, center + y, center - x, makeColor(255, 255, 255, 255));
	//		putPixel(surfaceNew, center - y, center + x, makeColor(255, 255, 255, 255));
	//		putPixel(surfaceNew, center - y, center - x, makeColor(255, 255, 255, 255));

	//		if (error > 0)
	//		{
	//			error -= 4 * (--x);
	//		}
	//		error += 4 * (++y) + 2;
	//	}*/

	//}

	Uint8 red, green, blue, alpha;
	getColor(indicatorColor, &red, &green, &blue, &alpha);
	if ( expired )
	{
		alpha *= expireAlphaRate;
		indicatorColor = makeColor(red, green, blue, alpha);
	}
	if ( loopType == 1 )
	{
		if ( radius >= radiusMax )
		{
			if ( loopTimer > 0 )
			{
				alpha *= (loopTimer - loopTicks) / (real_t)loopTimer;
			}
		}
	}

	//auto t5 = std::chrono::high_resolution_clock::now();
	bool circle = !castingTarget;

	static ConsoleVariable<bool> cvar_aoe_indicator_cache("/aoe_indicator_cache", true);
	if ( !*cvar_aoe_indicator_cache )
	{
		cacheType = CACHE_NONE;
	}

	bool needsUpdate = true;
	if ( prevData.r == red &&
		prevData.g == green &&
		prevData.b == blue &&
		prevData.a == alpha &&
		prevData.radMax == radius &&
		prevData.radMin == std::max(0, radius - gradientSize) + 0.0 &&
		prevData.size == size )
	{
		needsUpdate = false;
	}

	SDL_Surface* surfaceNew = nullptr;
	if ( needsUpdate )
	{
		auto tup = std::make_tuple(
			red, green, blue, alpha, radius, std::max(0, radius - gradientSize) + 0.0, size);

		if ( cacheType != CACHE_NONE )
		{
			auto& cache = AOEIndicators_t::surfaceCache[cacheType];
			auto find = cache.find(tup);
			if ( find != cache.end() )
			{
				surfaceNew = find->second;
				needsUpdate = false;
			}
		}

		if ( needsUpdate )
		{
			surfaceNew = SDL_CreateRGBSurface(0, size, size, 32,
				0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
			SDL_LockSurface(surfaceNew);

			for ( real_t rad = std::max(0, radius - gradientSize) + 0.0; rad <= radius; rad += 0.5 )
			{
				Uint32 color = 0;
				Uint8 alphaUsed = alpha;
				if ( rad < ring + 0.5 )
				{
					real_t alphaRatio = std::min(1.0, std::max(0.0, 1.0 + (ringSize + rad - (radius + 0.5)) / (real_t)gradientSize));
					alphaUsed = std::min(255.0, alpha * alphaRatio);
					color = makeColor(red, green, blue, alphaUsed);
				}
				else if ( rad == ring + 0.5 )
				{
					color = makeColor(red, green, blue, alpha);
				}
				else
				{
					color = makeColor(red * .8, green * .8, blue * .8, alpha);
				}

				if ( !circle )
				{
					real_t radius = rad + .5;
					real_t r2 = radius * radius;
					const int dist = radius;// floor(radius * sqrt(0.5));
					const int d = radius;//floor(sqrt(r2 - r * r));
					for ( int r = 0; r <= dist; r++ ) {
						if ( !(center - d >= 0 && center + d < size) )
						{
							continue;
						}
						if ( !(center - r >= 0 && center + r < size) )
						{
							continue;
						}
						if ( d < 8 )
						{
							Uint8 red, green, blue, alpha;
							getColor(color, &red, &green, &blue, &alpha);
							Uint8 prevAlpha = alpha;
							alpha *= d / 8.0;
							if ( alpha == 0 )
							{
								continue;
							}
							color = makeColor(red, green, blue, alpha);

							putPixel(surfaceNew, center + d, center + r, color);
							putPixel(surfaceNew, center + d, center - r, color);

							putPixel(surfaceNew, center - d, center + r, color);
							putPixel(surfaceNew, center - d, center - r, color);

							putPixel(surfaceNew, center + r, center + d, color);
							putPixel(surfaceNew, center - r, center + d, color);

							putPixel(surfaceNew, center + r, center - d, color);
							putPixel(surfaceNew, center - r, center - d, color);

							color = makeColor(red, green, blue, prevAlpha);
							continue;
						}
						if ( r < 8 )
						{
							Uint8 red, green, blue, alpha;
							getColor(color, &red, &green, &blue, &alpha);
							Uint8 prevAlpha = alpha;
							alpha *= r / 8.0;
							if ( alpha == 0 )
							{
								continue;
							}
							color = makeColor(red, green, blue, alpha);
							putPixel(surfaceNew, center + d, center + r, color);
							putPixel(surfaceNew, center + d, center - r, color);

							putPixel(surfaceNew, center - d, center + r, color);
							putPixel(surfaceNew, center - d, center - r, color);

							putPixel(surfaceNew, center + r, center + d, color);
							putPixel(surfaceNew, center - r, center + d, color);

							putPixel(surfaceNew, center + r, center - d, color);
							putPixel(surfaceNew, center - r, center - d, color);

							color = makeColor(red, green, blue, prevAlpha);
							continue;
						}
						if ( alphaUsed == 0 )
						{
							continue;
						}
						putPixel(surfaceNew, center + d, center + r, color);
						putPixel(surfaceNew, center + d, center - r, color);

						putPixel(surfaceNew, center - d, center + r, color);
						putPixel(surfaceNew, center - d, center - r, color);

						putPixel(surfaceNew, center + r, center + d, color);
						putPixel(surfaceNew, center - r, center + d, color);

						putPixel(surfaceNew, center + r, center - d, color);
						putPixel(surfaceNew, center - r, center - d, color);
					}
				}
				else if ( circle )
				{
					const real_t radius = rad + .5;
					const real_t r2 = radius * radius;
					const int dist = floor(radius * sqrt(0.5));

					for ( int r = 0; r <= dist; r++ ) {
						if ( alphaUsed == 0 )
						{
							break;
						}
						int d = floor(sqrt(r2 - r * r));
						if ( !(center - d >= 0 && center + d < size) )
						{
							continue;
						}
						if ( !(center - r >= 0 && center + r < size) )
						{
							continue;
						}

						if ( arc > 0.001 )
						{
							real_t tangent = atan2(r, d);
							if ( tangent > arc )
							{
								continue;
							}

							putPixel(surfaceNew, center + r, center - d, color);
							putPixel(surfaceNew, center - r, center - d, color);
						}
						else
						{
							putPixel(surfaceNew, center + d, center + r, color);
							putPixel(surfaceNew, center + d, center - r, color);

							putPixel(surfaceNew, center - d, center + r, color);
							putPixel(surfaceNew, center - d, center - r, color);

							putPixel(surfaceNew, center + r, center + d, color);
							putPixel(surfaceNew, center - r, center + d, color);

							putPixel(surfaceNew, center + r, center - d, color);
							putPixel(surfaceNew, center - r, center - d, color);
						}
					}
				}
			}

			if ( cacheType > CACHE_NONE )
			{
				surfaceCache[cacheType][tup] = surfaceNew;
			}
		}
	}

	//auto t6 = std::chrono::high_resolution_clock::now();
	//std::chrono::steady_clock::time_point t2;
	//std::chrono::steady_clock::time_point t3;
	//std::chrono::steady_clock::time_point t4;
	//t2 = std::chrono::high_resolution_clock::now();
	//t3 = t2;
	//t4 = t2;

	//std::chrono::steady_clock::time_point new1 = t2;
	//std::chrono::steady_clock::time_point new2 = t2;
	//std::chrono::steady_clock::time_point new3 = t2;
	//std::chrono::steady_clock::time_point new4 = t2;
	//std::chrono::steady_clock::time_point new5 = t2;
	//std::chrono::steady_clock::time_point new6 = t2;

	auto m1 = surfaceNew;
	auto m2 = surfaceOld;
	if ( surfaceNew )
	{
		//new1 = std::chrono::high_resolution_clock::now();
		if ( surfaceOld ) {
			SDL_LockSurface(surfaceOld);
		}

		//new2 = std::chrono::high_resolution_clock::now();
		const auto size1 = m1->w * m1->h * m1->format->BytesPerPixel;
		const auto size2 = m2 ? (m2->w * m2->h * m2->format->BytesPerPixel) : 0;
		if ( size1 != size2 || memcmp(m1, m2, size2) ) {
			if ( !texture ) {
				texture = new TempTexture();
			}
			//new3 = std::chrono::high_resolution_clock::now();
			texture->load(surfaceNew, false, true);
			//new4 = std::chrono::high_resolution_clock::now();
			if ( surfaceOld ) {
				SDL_UnlockSurface(surfaceOld);
				if ( cacheType == CACHE_NONE )
				{
					SDL_FreeSurface(surfaceOld);
				}
			}
			//new5 = std::chrono::high_resolution_clock::now();
			SDL_UnlockSurface(surfaceNew);
			surfaceOld = surfaceNew;

			//new6 = std::chrono::high_resolution_clock::now();

			prevData.r = red;
			prevData.g = green;
			prevData.b = blue;
			prevData.a = alpha;
			prevData.radMax = radius;
			prevData.radMin = std::max(0, radius - gradientSize) + 0.0;
			prevData.size = size;
			//t3 = std::chrono::high_resolution_clock::now();
		}
		else {
			if ( surfaceOld ) {
				SDL_UnlockSurface(surfaceOld);
			}
			SDL_UnlockSurface(surfaceNew);
			if ( cacheType == CACHE_NONE )
			{
				SDL_FreeSurface(surfaceNew);
			}
			surfaceNew = nullptr;
			//t4 = std::chrono::high_resolution_clock::now();
		}
	}
	//auto t7 = std::chrono::high_resolution_clock::now();

	if ( ticks % ticksPerUpdate == 0 )
	{
		if ( radius < radiusMax )
		{
			radius += framesPerTick;
		}
		if ( radius >= radiusMax )
		{
			if ( loop )
			{
				if ( loopType == 1 ) // linger on max radius
				{
					radius = radiusMax;
					++loopTicks;
					if ( loopTicks >= loopTimer )
					{
						radius = radiusMin;
						loopTicks = 0;
					}
				}
				else
				{
					radius = radiusMin;
				}
			}
			else
			{
				expired = true;
			}
		}
	}

	//auto t8 = std::chrono::high_resolution_clock::now();
	//static double maxTotal;
	//if ( keystatus[SDLK_t] )
	//{
	//	static double out1 = 0.0;
	//	static double out2 = 0.0;
	//	static double out3 = 0.0;
	//	static double out4 = 0.0;
	//	static double out5 = 0.0;
	//	static double out6 = 0.0;
	//	static double out7 = 0.0;
	//	out1 += 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(new1 - t2).count();
	//	out2 += 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(new2 - new1).count();
	//	out3 += 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(new3 - new2).count();
	//	out4 += 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(new4 - new3).count();
	//	out5 += 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(new5 - new4).count();
	//	out6 += 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(new6 - new5).count();

	//	//out1 += 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t5 - t1).count();
	//	//out2 += std::max(0.0, 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t3 - t2).count());
	//	//out3 += std::max(0.0, 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t4 - t2).count());
	//	//out4 += 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t5 - t5).count();
	//	//out5 += 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t6 - t5).count();
	//	//out6 += 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t7 - t6).count();
	//	//out7 += 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t8 - t7).count();
	//	double newMax = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t8 - t1).count();
	//	if ( newMax > maxTotal )
	//	{
	//		printlog("new max: %4.5fms, old: %4.5fms | rad: %.2f", newMax, maxTotal, radius);
	//	}
	//	maxTotal = std::max(maxTotal, newMax);
	//	char debugOutput[1024];
	//	snprintf(debugOutput, 1023,
	//		"t1: %4.5fms t2: %4.5fms t3: %4.5fms t4: %4.5fms t5: %4.5fms t6: %4.5fms t7: %4.5fms max: %4.5fms",
	//		out1, out2, out3, out4, out5, out6, out7, maxTotal);
	//	messagePlayer(0, MESSAGE_DEBUG, "%s", debugOutput);
	//}
	//else
	//{
	//	maxTotal = 0.0;
	//}
}

Entity* createParticleAOEIndicator(Entity* parent, real_t x, real_t y, real_t z, Uint32 lifetime, int size)
{
	Uint32 uid = 0;
	if ( parent )
	{
		uid = parent->getUID();
	}

	Entity* entity = newEntity(222, 1, map.entities, nullptr); //Sprite entity.
	entity->x = x;
	entity->y = y;
	entity->z = z + 7.49;
	static ConsoleVariable<float> cvar_sprite_aoe_indicator_scale("/sprite_aoe_indicator_scale", 2.0);
	static ConsoleVariable<float> cvar_sprite_aoe_indicator_rotate("/sprite_aoe_indicator_rotate", 0.0);
	static ConsoleVariable<float> cvar_sprite_aoe_indicator_alpha("/sprite_aoe_indicator_alpha", 0.5);
	static ConsoleVariable<float> cvar_sprite_aoe_indicator_alpha_glow("/sprite_aoe_indicator_alpha_glow", 0.0625);
	entity->ditheringDisabled = true;
	entity->flags[SPRITE] = true;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->flags[BRIGHT] = true;
	//entity->lightBonus = vec4(0.5f, 0.5f, 0.5f, 0.f);
	entity->scalex = *cvar_sprite_aoe_indicator_scale;
	entity->scaley = *cvar_sprite_aoe_indicator_scale;
	entity->behavior = &actSprite;
	entity->yaw = parent ? floor(parent->yaw / (PI / 2)) * PI / 2 : 0.0;
	entity->pitch = 0;
	entity->roll = -PI / 2;
	entity->skill[0] = 1;
	entity->skill[1] = 1;
	entity->skill[2] = lifetime;
	entity->fskill[0] = *cvar_sprite_aoe_indicator_rotate;
	entity->fskill[1] = *cvar_sprite_aoe_indicator_alpha;
	entity->fskill[2] = *cvar_sprite_aoe_indicator_alpha; // alpha
	entity->fskill[3] = *cvar_sprite_aoe_indicator_alpha_glow;
	entity->actSpriteUseAlpha = 1; // use alpha
	entity->actSpriteNoBillboard = 1; // no billboard
	entity->actSpriteCheckParentExists = uid;
	entity->flags[ENTITY_SKIP_CULLING] = true; // ignore LOS culling
	entity->actSpriteUseCustomSurface = AOEIndicators_t::createIndicator(4, size, size * 2 + 4, lifetime);
	entity->actSpriteFollowUID = uid; // follow parent
	entity->z -= 2 * (entity->actSpriteUseCustomSurface % 50 / 10000.0);
	entity->setEntityString("aoe_indicator");
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);
	return entity;
}

Entity* createFloorMagic(ParticleTimerEffect_t::EffectType particleType, int sprite, real_t x, real_t y, real_t z, real_t dir, Uint32 lifetime)
{
	Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Sprite entity.
	entity->x = x;
	entity->y = y;
	entity->z = z;
	entity->yaw = dir;
	entity->scalex = 0.25;
	entity->scaley = 0.25;
	entity->scalez = 0.25;
	entity->skill[0] = lifetime;
	entity->actfloorMagicType = particleType;
	entity->behavior = &actParticleFloorMagic;
	entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
		*cvar_magic_fx_light_bonus, 0.f);
	entity->flags[PASSABLE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->flags[UPDATENEEDED] = true;
	entity->skill[2] = -18;
	return entity;
}

Entity* createParticleRoot(int sprite, real_t x, real_t y, real_t z, real_t dir, Uint32 lifetime)
{
	Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Sprite entity.
	entity->x = x;
	entity->y = y;
	entity->z = z;
	entity->yaw = dir;
	entity->scalex = 0.25;
	entity->scaley = 0.25;
	entity->scalez = 0.25;
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[0] = lifetime;
	entity->behavior = &actParticleRoot;
	entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
		*cvar_magic_fx_light_bonus, 0.f);
	entity->flags[PASSABLE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->flags[NOUPDATE] = true;
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	TileEntityList.addEntity(*entity);
	entity->setUID(-3);
	return entity;
}

void createMushroomSpellEffect(Entity* caster, real_t x, real_t y)
{
	for ( int i = 0; i < 8; ++i )
	{
		Entity* fx = createParticleAestheticOrbit(caster, 1885, 3 * TICKS_PER_SECOND, PARTICLE_EFFECT_MUSHROOM_SPELL);
		fx->x = caster ? caster->x : x;
		fx->y = caster ? caster->y : y;
		fx->actmagicOrbitDist = 32;
		fx->yaw = (caster ? caster->yaw : 0.0) + (i * PI / 4.0);
		fx->pitch = -PI;
		fx->fskill[4] = fx->yaw;

		Entity* gib = nullptr;
		if ( multiplayer == CLIENT )
		{
			gib = spawnGibClient(x, y, caster ? caster->z : 0, 1885);
		}
		else
		{
			gib = spawnGib(caster, 1885);
		}

		if ( gib )
		{
			if ( caster )
			{
				gib->z = caster->z;
			}
			gib->vel_x = 1.5 * cos(fx->yaw);
			gib->vel_y = 1.5 * sin(fx->yaw);
			gib->lightBonus = vec4{ 0.25f, 0.25f, 0.25f, 0.f };
		}
	}

	if ( Entity* fx = createParticleAOEIndicator(caster, caster ? caster->x : x, caster ? caster->y : y, 0.0, TICKS_PER_SECOND * 2, 32) )
	{
		fx->actSpriteFollowUID = caster ? caster->getUID() : 0;
		fx->actSpriteCheckParentExists = 0;
		//fx->scalex = 0.8;
		//fx->scaley = 0.8;
		if ( auto indicator = AOEIndicators_t::getIndicator(fx->skill[10]) )
		{
			//indicator->arc = PI / 2;
			indicator->indicatorColor = makeColorRGB(0, 145, 16);
			indicator->loop = false;
			indicator->gradient = 4;
			indicator->framesPerTick = 2;
			indicator->ticksPerUpdate = 1;
			indicator->delayTicks = 0;
			indicator->expireAlphaRate = 0.95;
		}
	}
	playSoundPosLocal(x, y, 169, 128);
	playSoundPosLocal(x, y, 717 + local_rng.rand() % 3, 128);
}

Entity* createVortexMagic(int sprite, real_t x, real_t y, real_t z, real_t dir, Uint32 lifetime)
{
	Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Sprite entity.
	entity->x = x;
	entity->y = y;
	entity->z = z;
	entity->yaw = dir;
	entity->scalex = 0.25;
	entity->scaley = 0.25;
	entity->scalez = 0.25;
	entity->skill[0] = lifetime;
	entity->behavior = &actParticleVortex;
	entity->flags[PASSABLE] = true;
	entity->flags[UNCLICKABLE] = true;
	//entity->flags[UPDATENEEDED] = true;
	return entity;
}

void actParticleRoot(Entity* my)
{
	int x = my->x / 16;
	int y = my->y / 16;

	if ( x <= 0 || x >= map.width - 1 || y <= 0 || y >= map.height - 1 )
	{
		my->flags[INVISIBLE] = true;
		list_RemoveNode(my->mynode);
		return;
	}

	if ( my->sprite != 2200 ) // void root
	{
		int mapIndex = (y)*MAPLAYERS + (x)*MAPLAYERS * map.height;
		if ( !map.tiles[mapIndex] || swimmingtiles[map.tiles[mapIndex]] || lavatiles[map.tiles[mapIndex]] || map.tiles[OBSTACLELAYER + mapIndex] )
		{
			my->flags[INVISIBLE] = true;
			list_RemoveNode(my->mynode);
			return;
		}
	}

	if ( my->actmagicDelayMove > 0 )
	{
		my->flags[INVISIBLE] = true;
		--my->actmagicDelayMove;
		return;
	}

	my->flags[INVISIBLE] = false;

	if ( PARTICLE_LIFE < 0 )
	{
		my->scalex -= 0.1;
		my->scaley = my->scalex;
		my->scalez = my->scalex;
		if ( my->scalex < 0.0 )
		{
			list_RemoveNode(my->mynode);
			return;
		}
	}
	else
	{
		if ( my->skill[6] == 1 ) // check parent exists instead of countdown 
		{
			if ( !uidToEntity(my->parent) )
			{
				PARTICLE_LIFE = -1;
			}
		}
		else
		{
			--PARTICLE_LIFE;
		}
		if ( my->fskill[1] < 1.0 )
		{
			my->fskill[1] += 0.05;
			my->fskill[1] = std::min(my->fskill[1], 1.0);
			if ( my->fskill[1] >= 1.0 )
			{
				my->skill[5] = my->ticks; // animation tick start
			}
		}
		my->roll = -PI + PI * sin(my->fskill[1] * PI / 2);
		if ( my->skill[1] == 0 )
		{
			my->scalex = std::min(my->scalex + 0.1, 1.0);
			my->scaley = my->scalex;
			my->scalez = my->scalex;
			if ( my->scalex >= 1.0 )
			{
				my->skill[1] = 1;
			}
		}
		else if ( my->skill[1] == 1 )
		{
			my->fskill[0] += my->skill[4] == 0 ? 0.5 : 0.05;
			if ( my->fskill[0] >= 2 * PI )
			{
				my->fskill[0] = 0.0;
				my->skill[3]++;
				if ( my->skill[3] == 3 )
				{
					my->skill[4] = my->skill[4] > 0 ? 0 : 1;
				}
			}
			my->scalex = 1.0 + 0.05 * sin(my->fskill[0]);
			my->scaley = my->scalex;
			my->scalez = my->scalex;
		}

		if ( (my->ticks - my->skill[5]) % 50 == 0 )
		{
			if ( true )
			{
				int sprite = 226;
				if ( my->sprite == 2200 ) // void root
				{
					sprite = 261;
				}
				Entity* particle = spawnMagicParticleCustom(my, sprite, .7, 1.0);
				particle->flags[SPRITE] = true;
				particle->vel_z = -0.25;
				particle->z = 6.0;
			}

			if ( false )
			{
				Entity* entity = newEntity(227, 1, map.entities, nullptr); //Sprite entity.
				entity->x = my->x;
				entity->y = my->y;
				entity->z = 6.0;
				entity->ditheringDisabled = true;
				entity->flags[SPRITE] = true;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				entity->flags[UNCLICKABLE] = true;
				entity->flags[BRIGHT] = true;
				entity->behavior = &actSprite;
				entity->skill[0] = 1;
				entity->skill[1] = 6;
				entity->skill[2] = 4;
				entity->vel_z = -0.25;
				if ( multiplayer != CLIENT )
				{
					entity_uids--;
				}
				entity->setUID(-3);
			}

			if ( false )
			{
				Entity* entity = newEntity(233, 1, map.entities, nullptr); //Sprite entity.
				entity->x = my->x;
				entity->y = my->y;
				entity->z = 6.0;
				entity->ditheringDisabled = true;
				entity->flags[SPRITE] = true;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				entity->flags[UNCLICKABLE] = true;
				entity->flags[BRIGHT] = true;
				entity->behavior = &actSprite;
				entity->skill[0] = 1;
				entity->skill[1] = 12;
				entity->skill[2] = 4;
				entity->vel_z = -0.25;
				if ( multiplayer != CLIENT )
				{
					entity_uids--;
				}
				entity->setUID(-3);
			}
		}
	}
}

void actParticleVortex(Entity* my)
{
	if ( PARTICLE_LIFE < 0 )
	{
		list_RemoveNode(my->mynode);
		return;
	}
	else
	{
		--PARTICLE_LIFE;
		if ( my->skill[1] == 0 )
		{
			my->skill[1] = 1;
			my->fskill[0] = my->x;
			my->fskill[1] = my->y;
		}


		static ConsoleVariable<float> cvar_particle_speed("/particle_speed", 1.0);
		static ConsoleVariable<float> cvar_particle_z("/particle_z", 1.0);
		static ConsoleVariable<float> cvar_particle_yaw("/particle_yaw", 1.0);
		static ConsoleVariable<float> cvar_particle_radius("/particle_radius", 1.0);
		static ConsoleVariable<float> cvar_particle_scale("/particle_scale", 1.0);
		real_t dist = std::max(0.25, (1.0 + cos(PI + PI * my->fskill[3])) / 2);
		my->x = my->fskill[0] + 4.0 * *cvar_particle_radius * dist * cos(my->yaw);
		my->y = my->fskill[1] + 4.0 * *cvar_particle_radius * dist * sin(my->yaw);
		my->z = my->z - 0.05 * *cvar_particle_z;
		my->yaw += 0.05 * *cvar_particle_yaw;
		real_t scale = *cvar_particle_scale * (1.0 + cos(PI + PI * my->fskill[2])) / 2;
		if ( my->z <= 0 ) //-8.0 )
		{
			my->scalex -= 0.1;
			my->scaley -= 0.1;
			my->scalez -= 0.1;
			if ( my->scalex <= 0.0 )
			{
				list_RemoveNode(my->mynode);
				return;
			}
		}
		else
		{
			my->scalex = scale;
			my->scaley = scale;
			my->scalez = scale;
		}
		my->fskill[2] = std::min(1.0, 0.005 * *cvar_particle_speed + my->fskill[2]); // scale
		my->fskill[3] = std::min(1.0, 0.01 * *cvar_particle_speed + my->fskill[3]); // dist from center

		Entity* particle = spawnMagicParticleCustom(my, my->sprite, scale * .7, 10.0);
		particle->ditheringDisabled = true;
	}
}

void actParticleFloorMagic(Entity* my)
{
	Entity* parentTimer = uidToEntity(my->parent);
	if ( PARTICLE_LIFE < 0 || (multiplayer != CLIENT && !parentTimer) )
	{
		my->removeLightField();
		list_RemoveNode(my->mynode);
		return;
	}

	bool doEffect = my->actmagicDelayMove == 0;
	if ( my->actmagicDelayMove > 0 )
	{
		--my->actmagicDelayMove;
		my->flags[INVISIBLE] = true;
		if ( my->actmagicDelayMove == 0 )
		{
			spawnGib(my);
			my->flags[INVISIBLE] = false;
			my->flags[UPDATENEEDED] = true;
		}
	}

	if ( doEffect )
	{
		if ( multiplayer != CLIENT )
		{
			--PARTICLE_LIFE;
			my->x += my->vel_x * cos(my->yaw);
			my->y += my->vel_y * sin(my->yaw);

			if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_SELF_SUSTAIN )
			{
				PARTICLE_LIFE = std::max(TICKS_PER_SECOND, PARTICLE_LIFE); // wait for parent timer to expire
			}
		}
		else
		{
			if ( my->actfloorMagicClientReceived == 1 )
			{
				if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_LIGHTNING_BOLT )
				{
					--PARTICLE_LIFE; // client allowed decay
				}
			}
		}

		if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_SELF
			|| my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_TILE
			|| my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_TILE_VOID
			|| my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_SELF_SUSTAIN
			|| my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_PATH )
		{
			if ( my->scalex < 1.0 )
			{
				my->scalex = std::min(my->scalex * 1.25, 1.0);
				my->scaley = std::min(my->scaley * 1.25, 1.0);
				my->scalez = std::min(my->scalez * 1.25, 1.0);
			}
			else
			{
				my->fskill[0] += 0.05;
				my->scalex = 1.025 - 0.025 * sin(my->fskill[0] + PI / 2);
				my->scaley = my->scalex;
				my->scalez = my->scalex;
			}
		}
		else
		{
			my->scalex = std::min(my->scalex * 1.25, 1.0);
			my->scaley = std::min(my->scaley * 1.25, 1.0);
			my->scalez = std::min(my->scalez * 1.25, 1.0);
		}
	}

	if ( my->ticks == 1 ) // lightning bolt
	{
		if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_LIGHTNING_BOLT )
		{
			if ( Entity* particle = spawnMagicParticleCustom(my, 1764, my->scalex * 1.0, 1) )
			{
				particle->z = 6.0;
				particle->x = my->x;
				particle->y = my->y;
				particle->yaw = local_rng.rand() % 360 * PI / 180;
				particle->vel_x = 0.05 * cos(particle->yaw);
				particle->vel_y = 0.05 * sin(particle->yaw);
				particle->ditheringDisabled = true;
			}
		}
		else if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ICE_WAVE )
		{
			//if ( Entity* gib = multiplayer == CLIENT ? spawnGibClient(my,) : spawnGib(my) )
			//{
			//	gib->sprite = 1374;
			//}
		}
		else if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_DISRUPT_EARTH )
		{
			if ( Entity* gib = multiplayer == CLIENT ? spawnGibClient(my->x, my->y, my->z, -1) : spawnGib(my) )
			{
				gib->sprite = 78;
			}
		}
		else if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_SPORES
			|| my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_MYCELIUM )
		{
			my->flags[INVISIBLE] = true;
			my->scalex = 1.0;
		}
		else if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_TILE
			|| my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_TILE_VOID )
		{
			BaronyRNG rng;
			Uint32 rootSeed = my->getUID();
			rng.seedBytes(&rootSeed, sizeof(rootSeed));

			std::vector<float> locations =
			{
				0 * PI / 4,
				1 * PI / 4,
				2 * PI / 4,
				3 * PI / 4,
				4 * PI / 4,
				5 * PI / 4,
				6 * PI / 4,
				7 * PI / 4
			};

			int numLocations = locations.size();

			real_t dist = 16.0;
			while ( locations.size() >= 4 )
			{
				int pick = rng.rand() % locations.size();
				float yaw = locations[pick];
				/*if ( i % 2 == 1 )
				{
					yaw += PI / 8;
				}*/
				int sprite = 1766;
				if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_TILE_VOID )
				{
					sprite = 2200;
				}
				Entity* root = createParticleRoot(sprite, my->x + dist * cos(yaw), my->y + dist * sin(yaw),
					7.5, rng.rand() % 360 * (PI / 180.0), PARTICLE_LIFE);
				root->focalz = -0.5;
				int roll = rng.rand() % 8;
				real_t angle = (pick / (float)numLocations) * PI + ((roll) / 8.0) * PI;
				real_t xoffset = 4.0 * sin(angle);
				xoffset += -2.0 + 4.0 * (rng.rand() % 16) / 16.0;
				root->x += xoffset * cos(root->yaw + PI / 2);
				root->y += xoffset * sin(root->yaw + PI / 2);
				root->actmagicDelayMove = (TICKS_PER_SECOND / 8) * (locations.size());
				root->skill[0] -= root->actmagicDelayMove;
				root->parent = my->getUID();
				locations.erase(locations.begin() + pick);
			}
		}
		else if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_PATH )
		{
			BaronyRNG rng;
			Uint32 rootSeed = my->getUID();
			rng.seedBytes(&rootSeed, sizeof(rootSeed));

			std::vector<float> locations = {
					0 * PI / 8,
					1 * PI / 8,
					2 * PI / 8,
					3 * PI / 8,
					-1 * PI / 8,
					-2 * PI / 8,
					-3 * PI / 8,
					-4 * PI / 8,
					0 * PI / 8,
					1 * PI / 8,
					2 * PI / 8,
					3 * PI / 8,
					-1 * PI / 8,
					-2 * PI / 8,
					-3 * PI / 8,
					-4 * PI / 8
				};
			int numLocations = locations.size();

			real_t dist = 8.0;
			while ( locations.size() )
			{
				if ( dist >= 80.0 )
				{
					break;
				}
				int pick = rng.rand() % locations.size();
				float yaw = my->yaw + locations[pick] / 64;
				dist += 8.0;
				/*if ( i % 2 == 1 )
				{
					yaw += PI / 8;
				}*/
				Entity* root = createParticleRoot(1766, my->x + dist * cos(yaw), my->y + dist * sin(yaw),
					7.5, rng.rand() % 360 * (PI / 180.0), PARTICLE_LIFE);
				root->focalz = -0.5;
				int roll = rng.rand() % 8;
				real_t angle = (pick / (float)numLocations) * PI + ((roll) / 8.0) * PI;
				real_t xoffset = 4.0 * sin(angle);
				xoffset += -2.0 + 4.0 * (rng.rand() % 16) / 16.0;
				root->x += xoffset * cos(root->yaw + PI / 2);
				root->y += xoffset * sin(root->yaw + PI / 2);
				root->actmagicDelayMove = (TICKS_PER_SECOND / 8) * ((numLocations - locations.size()));
				root->skill[0] -= root->actmagicDelayMove;
				root->parent = my->getUID();
				locations.erase(locations.begin() + pick);
			}
			/*int roll = local_rng.rand() % 8;
			for ( int i = 0; i < 16; ++i )
			{
				real_t angle = (i / 8.0) * PI + ((roll) / 8.0) * PI;
				real_t yawOffset = cos(angle) + ((local_rng.rand() % 4) / 4.0) * 2 * PI;
				real_t dist = 40.0 * (0.25 + (0.75 * i / 16.0));
				Entity* root = createParticleRoot(1766, my->x + dist * cos(yawOffset), my->y + dist * sin(yawOffset),
					7.5, local_rng.rand() % 360 * (PI / 180.0), PARTICLE_LIFE);
				root->focalz = -0.5;
				real_t xoffset = 8.0 * sin(angle);
				xoffset += 2.0 * (local_rng.rand() % 16) / 16.0;
				root->x += xoffset * cos(my->yaw + PI / 2);
				root->y += xoffset * sin(my->yaw + PI / 2);

				root->actmagicDelayMove = (TICKS_PER_SECOND / 4) * i;
				root->skill[0] -= root->actmagicDelayMove;
			}*/
		}
		else if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_SELF
			|| my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_SELF_SUSTAIN )
		{
			BaronyRNG rng;
			Uint32 rootSeed = my->getUID();
			rng.seedBytes(&rootSeed, sizeof(rootSeed));
			for ( int i = 0; i < 2; ++i )
			{
				// circular
				std::vector<float> locations = 
				{
					0 * PI / 8,
					1 * PI / 8,
					2 * PI / 8,
					3 * PI / 8,
					4 * PI / 8,
					5 * PI / 8,
					6 * PI / 8,
					7 * PI / 8,
					8 * PI / 8,
					9 * PI / 8,
					10 * PI / 8,
					11 * PI / 8,
					12 * PI / 8,
					13 * PI / 8,
					14 * PI / 8,
					15 * PI / 8
				};
				if ( i == 1 )
				{
					locations =
					{
						1 * PI / 8,
						3 * PI / 8,
						5 * PI / 8,
						7 * PI / 8,
						9 * PI / 8,
						11 * PI / 8,
						13 * PI / 8,
						15 * PI / 8
					};
				}
				int numLocations = locations.size();

				real_t dist = 24.0 - 8.0 * i;
				while ( locations.size() )
				{
					int pick = rng.rand() % locations.size();
					float yaw = locations[pick];
					Entity* root = createParticleRoot(1766, my->x + dist * cos(yaw), my->y + dist * sin(yaw),
						7.5, rng.rand() % 360 * (PI / 180.0), PARTICLE_LIFE);
					root->focalz = -0.5;
					int roll = rng.rand() % 8;
					real_t angle = (pick / (float)numLocations) * PI + ((roll) / 8.0) * PI;
					real_t xoffset = 4.0 * sin(angle);
					xoffset += -2.0 + 4.0 * (rng.rand() % 16) / 16.0;
					root->x += xoffset * cos(root->yaw + PI / 2);
					root->y += xoffset * sin(root->yaw + PI / 2);
					root->actmagicDelayMove = (TICKS_PER_SECOND / 8) * (locations.size() + i * 8);
					root->skill[0] -= root->actmagicDelayMove;
					root->parent = my->getUID();
					locations.erase(locations.begin() + pick);
					if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_SELF_SUSTAIN )
					{
						root->skill[6] = 1; // check parent exists instead of countdown
					}
				}
			}
		}
	}

	bool doParticle = false;
	if ( my->actmagicDelayMove == 0 && my->actmagicNoParticle == 0 )
	{
		if ( *cvar_magic_fx_use_vismap && !intro )
		{
			int x = my->x / 16.0;
			int y = my->y / 16.0;
			if ( x >= 0 && x < map.width && y >= 0 && y < map.height )
			{
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( !client_disconnected[i] && players[i]->isLocalPlayer() && cameras[i].vismap[y + x * map.height] )
					{
						doParticle = true;
						break;
					}
				}
			}
		}
		else
		{
			doParticle = true;
		}
	}

	if ( doEffect )
	{
		if ( multiplayer != CLIENT )
		{
			if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_LIGHTNING_BOLT )
			{
				if ( Entity* target = uidToEntity(my->actmagicOrbitHitTargetUID1) )
				{
					my->x = target->x;
					my->y = target->y;
				}
			}
		}

		if ( multiplayer != CLIENT && my->scalex >= .9 )
		{
			my->skill[1]++; // active ticks
			Entity* caster = uidToEntity(parentTimer->parent);

			int radius = 1;
			if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_SELF
				|| my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_SELF_SUSTAIN
				|| my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_TILE )
			{
				radius = 2;
			}
			else if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_PATH )
			{
				radius = 5;
			}

			if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_MYCELIUM )
			{
				if ( local_rng.rand() % 200 == 0 || PARTICLE_LIFE == 1 )
				{
					bool skip = false;
					if ( parentTimer->particleTimerVariable4 != 0 )
					{
						if ( !(PARTICLE_LIFE == 1) )
						{
							if ( uidToEntity(parentTimer->particleTimerVariable4) )
							{
								skip = true; // skip while projectile alive
							}
						}
					}
					if ( !skip && parentTimer->particleTimerVariable3 == 0 )
					{
						if ( Entity* breakable = Entity::createBreakableCollider(EditorEntityData_t::getColliderIndexFromName("mushroom_spell_fragile"),
							my->x, my->y, caster) )
						{
							parentTimer->particleTimerVariable3 = 1;
							breakable->colliderSpellEvent = 1 + local_rng.rand() % 5;
							if ( !caster && achievementObserver.checkUidIsFromPlayer(parentTimer->parent) >= 0 || (caster && caster->behavior == &actPlayer) )
							{
								breakable->colliderCreatedParent = parentTimer->parent;
								breakable->colliderSpellEvent = 6;
							}
							if ( breakable->colliderSpellEvent > 0 && breakable->colliderSpellEvent < 1000 )
							{
								breakable->colliderSpellEvent += 1000;
							}
							breakable->colliderSetServerSkillOnSpawned(); // to update the variables modified from create()
						}
					}
				}
			}

			std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, radius);
			for ( auto it : entLists )
			{
				if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_TILE_VOID )
				{
					break;
				}

				node_t* node;
				for ( node = it->first; node != nullptr; node = node->next )
				{
					Entity* entity = (Entity*)node->element;
					if ( entity->behavior == &actPlayer || (entity->behavior == &actMonster && !entity->isInertMimic())
						|| my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_DISRUPT_EARTH /*hits furniture*/
						|| my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_LIGHTNING_BOLT /*hits furniture*/ )
					{
						if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_DISRUPT_EARTH
							|| my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_LIGHTNING_BOLT )
						{
							if ( entity->behavior != &actMonster
								&& entity->behavior != &actPlayer
								&& entity->behavior != &actDoor
								&& entity->behavior != &::actIronDoor
								&& !(entity->isDamageableCollider() && entity->isColliderDamageableByMagic())
								&& !entity->isInertMimic()
								&& entity->behavior != &::actChest
								&& entity->behavior != &::actFurniture )
							{
								continue;
							}
						}

						if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_SPORES
							|| my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_MYCELIUM )
						{
							auto particleEmitterHitPropsTimer = getParticleEmitterHitProps(my->parent, entity);
							if ( !particleEmitterHitPropsTimer )
							{
								continue;
							}
							if ( particleEmitterHitPropsTimer->hits > 0 )
							{
								continue;
							}

							if ( caster && caster->behavior == &actMonster )
							{
								if ( caster == entity || caster->checkFriend(entity) )
								{
									continue;
								}
							}
							if ( caster && caster->behavior == &actPlayer )
							{
								if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
								{
									if ( caster->checkFriend(entity) && caster->friendlyFireProtection(entity) )
									{
										continue;
									}
								}
							}

							Stat* stats = entity->getStats();
							if ( stats && entityInsideEntity(my, entity) )
							{
								bool targetNonPlayer = false;
								if ( caster && caster->behavior == &actPlayer )
								{
									targetNonPlayer = true;
								}
								else if ( !caster && achievementObserver.checkUidIsFromPlayer(parentTimer->parent) >= 0 )
								{
									targetNonPlayer = true;
								}

								if ( !entity->monsterIsTargetable() ) { continue; }

								bool isEnemy = false;
								if ( targetNonPlayer )
								{
									isEnemy = entity->behavior == &actMonster && !entity->monsterAllyGetPlayerLeader();
								}
								else
								{
									isEnemy = (caster && !caster->checkFriend(entity) || !caster);
								}

								if ( isEnemy )
								{
									bool effected = false;
									if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_SPORES )
									{
										int duration = getSpellEffectDurationSecondaryFromID(SPELL_SPORES, caster, nullptr, my);
										bool prevEff = stats->getEffectActive(EFF_POISONED);
										bool rollLevel = false;
										if ( entity->setEffect(EFF_POISONED, true, duration + 10, false, true, false, false) )
										{
											effected = true;
											stats->poisonKiller = caster ? caster->getUID() : 0;
											if ( !prevEff )
											{
												rollLevel = true;
											}
										}

										prevEff = stats->getEffectActive(EFF_SLOW);
										if ( entity->setEffect(EFF_SLOW, true, duration + 10, false, true, false, false) )
										{
											effected = true;
											if ( !prevEff )
											{
												rollLevel = true;
											}
										}

										if ( rollLevel )
										{
											if ( caster && caster->behavior == &actPlayer && parentTimer && parentTimer->particleTimerVariable2 == SPELL_SPORES )
											{
												players[caster->skill[2]]->mechanics.updateSustainedSpellEvent(SPELL_SPORES, 50.0, 1.0);
											}
										}
									}
									else if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_MYCELIUM )
									{
										if ( parentTimer->particleTimerVariable3 == 0 )
										{
											if ( Entity* breakable = Entity::createBreakableCollider(EditorEntityData_t::getColliderIndexFromName("mushroom_spell_fragile"),
												entity->x, entity->y, caster) )
											{
												parentTimer->particleTimerVariable3 = 1;
												breakable->colliderSpellEvent = 1 + local_rng.rand() % 5;
												if ( targetNonPlayer )
												{
													breakable->colliderCreatedParent = parentTimer->parent;
													breakable->colliderSpellEvent = 6;
												}
												if ( breakable->colliderSpellEvent > 0 && breakable->colliderSpellEvent < 1000 )
												{
													breakable->colliderSpellEvent += 1000;
												}
												breakable->colliderSetServerSkillOnSpawned(); // to update the variables modified from create()
											}
										}

										if ( entity->setEffect(EFF_SLOW, true, 6 * TICKS_PER_SECOND + 10, false, true, false, false) )
										{
											effected = true;
										}
									}

									if ( caster )
									{
										bool alertTarget = entity->monsterAlertBeforeHit(caster);
										if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_MYCELIUM )
										{
											alertTarget = false;
										}

										// alert the monster!
										if ( entity->monsterState != MONSTER_STATE_ATTACK && (stats->type < LICH || stats->type >= SHOPKEEPER) )
										{
											if ( alertTarget )
											{
												entity->monsterAcquireAttackTarget(*caster, MONSTER_STATE_PATH, true);
											}
										}

										// alert other monsters too
										if ( alertTarget )
										{
											entity->alertAlliesOnBeingHit(caster);
										}
										entity->updateEntityOnHit(caster, alertTarget);
									}

									if ( parentTimer && parentTimer->particleTimerVariable1 > 0 && my->ticks < 5 )
									{
										int damage = parentTimer->particleTimerVariable1;
										applyGenericMagicDamage(caster, entity, caster ? *caster : *my, parentTimer->particleTimerVariable2, damage, true, true);
									}

									particleEmitterHitPropsTimer->hits++;
									particleEmitterHitPropsTimer->tick = ticks;
								}
							}
						}
						else if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_LIGHTNING_BOLT )
						{
							auto particleEmitterHitPropsTimer = getParticleEmitterHitProps(my->parent, entity);
							if ( !particleEmitterHitPropsTimer )
							{
								continue;
							}
							if ( particleEmitterHitPropsTimer->hits > 0 )
							{
								continue;
							}

							if ( caster && caster->behavior == &actMonster )
							{
								if ( caster == entity || caster->checkFriend(entity) )
								{
									continue;
								}
							}
							if ( caster && caster->behavior == &actPlayer )
							{
								if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
								{
									if ( caster->checkFriend(entity) && caster->friendlyFireProtection(entity) )
									{
										continue;
									}
								}
							}

							Stat* stats = entity->getStats();
							if ( stats && entityDist(my, entity) <= 16.0 )
							{
								if ( !entity->monsterIsTargetable(true) && !entity->isUntargetableBat() ) { continue; }
								int damage = getSpellDamageFromID(SPELL_LIGHTNING_BOLT, caster, nullptr, my);
								if ( stats->getEffectActive(EFF_STATIC) )
								{
									int extraDamage = getSpellDamageSecondaryFromID(SPELL_LIGHTNING_BOLT, caster, nullptr, my);
									if ( extraDamage > 0 )
									{
										extraDamage *= getSpellDamageFromStatic(SPELL_LIGHTNING_BOLT, stats);
										damage += std::max(1, extraDamage);
									}
								}
								if ( applyGenericMagicDamage(caster, entity, caster ? *caster : *my, SPELL_LIGHTNING_BOLT, damage, true, true) )
								{
									Uint8 effectStrength = stats->getEffectActive(EFF_STATIC);
									if ( effectStrength < getSpellEffectDurationSecondaryFromID(SPELL_LIGHTNING_BOLT, caster, nullptr, my) )
									{
										effectStrength += 1;
									}
									if ( entity->setEffect(EFF_STATIC, effectStrength,
										getSpellEffectDurationFromID(SPELL_LIGHTNING_BOLT, caster, nullptr, my), true, true, false, false) )
									{
										Entity* fx = createParticleAestheticOrbit(entity, 1758, 2 * TICKS_PER_SECOND, PARTICLE_EFFECT_STATIC_ORBIT);
										fx->z = 7.5;
										fx->actmagicOrbitDist = 20;
										fx->actmagicNoLight = 1;
										serverSpawnMiscParticles(entity, PARTICLE_EFFECT_STATIC_ORBIT, 1758);
									}
								}

								particleEmitterHitPropsTimer->hits++;
								particleEmitterHitPropsTimer->tick = ticks;
							}
						}
						else if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_DISRUPT_EARTH )
						{
							if ( entityInsideEntity(my, entity) )
							{
								auto particleEmitterHitPropsTimer = getParticleEmitterHitProps(my->getUID(), entity);
								if ( !particleEmitterHitPropsTimer )
								{
									continue;
								}
								Stat* stats = entity->getStats();
								if ( stats )
								{
									if ( !entity->monsterIsTargetable(true) ) { continue; }
									if ( isLevitating(stats) ) { continue; }
									if ( my->ticks >= 10 )
									{
										if ( caster && caster->behavior == &actPlayer )
										{
											if ( caster->checkFriend(entity) || caster == entity )
											{
												// smaller hitbox for lingering terrain
												auto sizex = my->sizex;
												auto sizey = my->sizey;
												my->sizex = 4;
												my->sizey = 4;
												bool result = entityInsideEntity(my, entity);
												my->sizex = sizex;
												my->sizey = sizey;
												if ( result ) { continue; }
											}
										}
									}
								}

								int spellID = parentTimer ? parentTimer->particleTimerVariable3 : SPELL_DISRUPT_EARTH;

								if ( stats )
								{
									int prevDuration = stats->getEffectActive(EFF_DISRUPTED) ? stats->EFFECTS_TIMERS[EFF_DISRUPTED] : 0;
									int duration = getSpellEffectDurationFromID(spellID, caster, nullptr, my);
									if ( !stats->getEffectActive(EFF_DISRUPTED) )
									{
										entity->setEffect(EFF_DISRUPTED, true, duration, false, true, false, false); // don't override strength
									}
									else
									{
										stats->EFFECTS_TIMERS[EFF_DISRUPTED] = std::max(stats->EFFECTS_TIMERS[EFF_DISRUPTED], duration);
									}
								}

								if ( particleEmitterHitPropsTimer->hits > 0 && spellID != SPELL_EARTH_SPINES )
								{
									continue;
								}
								if ( caster && caster->behavior == &actMonster )
								{
									if ( stats )
									{
										if ( caster == entity || caster->checkFriend(entity) )
										{
											++particleEmitterHitPropsTimer->hits;
											continue;
										}
									}
								}
								else if ( caster && caster->behavior == &actPlayer )
								{
									if ( stats )
									{
										if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) && caster->checkFriend(entity) && caster->friendlyFireProtection(entity) )
										{
											++particleEmitterHitPropsTimer->hits;
											continue;
										}
									}
								}

								int damage = 0;

								if ( my->ticks < 10 || spellID == SPELL_EARTH_SPINES )
								{
									if ( auto particleEmitterHitPropsTimer2 = getParticleEmitterHitProps(my->parent, entity) )
									{
										if ( my->ticks < 10 && (particleEmitterHitPropsTimer2->hits < 3) )
										{
											if ( particleEmitterHitPropsTimer2->hits == 0 || ((ticks - particleEmitterHitPropsTimer2->tick) > 15) )
											{
												damage = getSpellDamageFromID(spellID, caster, nullptr, parentTimer);
												++particleEmitterHitPropsTimer2->hits;
												particleEmitterHitPropsTimer2->tick = ticks;
											}
										}
										else if ( spellID == SPELL_EARTH_SPINES && sqrt(pow(entity->vel_x, 2) + pow(entity->vel_y, 2)) > 0.25 )
										{
											if ( ((ticks - particleEmitterHitPropsTimer2->tick) > 50) )
											{
												damage = getSpellDamageFromID(spellID, caster, nullptr, parentTimer);
												++particleEmitterHitPropsTimer2->hits;
												particleEmitterHitPropsTimer2->tick = ticks;
											}
										}
									}

									if ( damage > 0 )
									{
										if ( applyGenericMagicDamage(caster, entity, caster ? *caster : *my, spellID, damage, true) )
										{
										}
									}
									if ( my->ticks < 10 && stats && entity->setEffect(EFF_KNOCKBACK, true, 30, false) )
									{
										real_t pushbackMultiplier = 0.5;
										real_t tangent = my->monsterKnockbackTangentDir + PI; // hack to store knockback
										if ( entity->behavior == &actPlayer )
										{
											if ( !players[entity->skill[2]]->isLocalPlayer() )
											{
												entity->monsterKnockbackVelocity = pushbackMultiplier;
												entity->monsterKnockbackTangentDir = tangent;
												serverUpdateEntityFSkill(entity, 11);
												serverUpdateEntityFSkill(entity, 9);
											}
											else
											{
												entity->monsterKnockbackVelocity = pushbackMultiplier;
												entity->monsterKnockbackTangentDir = tangent;
											}
										}
										else
										{
											entity->vel_x = cos(tangent) * pushbackMultiplier;
											entity->vel_y = sin(tangent) * pushbackMultiplier;
											entity->monsterKnockbackVelocity = 0.01;
											entity->monsterKnockbackTangentDir = tangent;
											entity->monsterKnockbackUID = parentTimer ? parentTimer->parent : 0;
										}
									}

									if ( my->ticks < 10 && stats )
									{
										int prevDuration = stats->getEffectActive(EFF_DISRUPTED) ? stats->EFFECTS_TIMERS[EFF_DISRUPTED] : 0;
										int durationAdd = getSpellEffectDurationSecondaryFromID(spellID, caster, nullptr, my);
										if ( stats->getEffectActive(EFF_DISRUPTED) != 1 )
										{
											if ( spellID == SPELL_EARTH_SPINES )
											{
												entity->setEffect(EFF_DISRUPTED, Uint8(3), prevDuration + durationAdd, false, true, true, false);
											}
											else
											{
												entity->setEffect(EFF_DISRUPTED, Uint8(2), prevDuration + durationAdd, false, true, true, false);
											}
										}
										else
										{
											stats->EFFECTS_TIMERS[EFF_DISRUPTED] += durationAdd;
										}
									}

									particleEmitterHitPropsTimer->hits++;
									particleEmitterHitPropsTimer->tick = ticks;
								}
							}
						}
						else if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_SELF
							|| my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_SELF_SUSTAIN
							|| my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_TILE
							|| my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_PATH )
						{
							auto particleEmitterHitPropsTimer = getParticleEmitterHitProps(my->parent, entity);
							if ( !particleEmitterHitPropsTimer )
							{
								continue;
							}
							if ( particleEmitterHitPropsTimer->hits > 0 && (ticks - particleEmitterHitPropsTimer->tick) < 1.5 * TICKS_PER_SECOND )
							{
								continue;
							}

							Stat* stats = entity->getStats();
							if ( stats && entityDist(my, entity) < radius * 16.0 + 16.0 )
							{
								if ( !entity->monsterIsTargetable() || entity == caster ) { continue; }
								if ( caster )
								{
									if ( caster && caster->behavior == &actMonster )
									{
										if ( caster->checkFriend(entity) )
										{
											continue;
										}
									}
									if ( caster && caster->behavior == &actPlayer )
									{
										//if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
										{
											if ( caster->checkFriend(entity) && caster->friendlyFireProtection(entity) )
											{
												continue;
											}
										}
									}

									// check overlapping roots

									bool found = false;
									if ( entityInsideEntity(my, entity) )
									{
										found = true;
									}
									else
									{
										std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, radius);
										for ( auto it : entLists )
										{
											if ( found )
											{
												break;
											}
											node_t* node;
											for ( node = it->first; node != nullptr; node = node->next )
											{
												Entity* entity2 = (Entity*)node->element;
												if ( entity2->behavior == &actParticleRoot 
													&& !entity2->flags[INVISIBLE]
													&& entity2->parent == my->getUID()
													&& entityInsideEntity(entity2, entity) )
												{
													found = true;
													break;
												}
											}
										}
									}

									if ( found )
									{
										int damage = 0;
										if ( parentTimer && parentTimer->particleTimerVariable1 != 0 )
										{
											damage = parentTimer->particleTimerVariable1;
										}
										else
										{
											if ( parentTimer && parentTimer->particleTimerVariable2 == 0 )
											{
												damage = getSpellDamageFromID(SPELL_ROOTS, caster, nullptr, my);
											}
											else
											{
												damage = getSpellDamageFromID(parentTimer ? parentTimer->particleTimerVariable2 : SPELL_ROOTS, caster, nullptr, my);
											}
										}

										if ( applyGenericMagicDamage(caster, entity, caster ? *caster : *my, parentTimer->particleTimerVariable1, damage, true, true) )
										{
											if ( entity->setEffect(EFF_ROOTED, true, TICKS_PER_SECOND, false) )
											{
												spawnMagicEffectParticles(entity->x, entity->y, entity->z, 1758);
											}
											if ( parentTimer && parentTimer->particleTimerVariable3 == SPELL_BLADEVINES )
											{
												if ( !stats->getEffectActive(EFF_BLEEDING) )
												{
													if ( entity->setEffect(EFF_BLEEDING, true, 3 * TICKS_PER_SECOND, false) )
													{
														stats->bleedInflictedBy = caster ? caster->getUID() : 0;
														for ( int gibs = 0; gibs < 3; ++gibs )
														{
															Entity* gib = spawnGib(entity);
															serverSpawnGibForClient(gib);
														}
													}
												}
											}
										}
										particleEmitterHitPropsTimer->hits++;
										particleEmitterHitPropsTimer->tick = ticks;
									}
								}
							}
						}
						else if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ICE_WAVE )
						{
							auto particleEmitterHitPropsTimer = getParticleEmitterHitProps(my->parent, entity);
							if ( !particleEmitterHitPropsTimer )
							{
								continue;
							}
							auto particleEmitterHitPropsFloorMagic = getParticleEmitterHitProps(my->getUID(), entity);
							if ( !particleEmitterHitPropsFloorMagic )
							{
								continue;
							}
							if ( particleEmitterHitPropsTimer->hits > 0 && ((ticks - particleEmitterHitPropsTimer->tick) < 30) )
							{
								if ( particleEmitterHitPropsFloorMagic->hits > 5 )
								{
									continue;
								}
								if ( particleEmitterHitPropsFloorMagic->hits == 0 && my->skill[1] < 10 )
								{
									// allowed big hit
								}
								else
								{
									continue;
								}
							}

							if ( caster && caster->behavior == &actMonster )
							{
								if ( caster == entity || caster->checkFriend(entity) )
								{
									continue;
								}
							}
							if ( caster && caster->behavior == &actPlayer )
							{
								if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
								{
									if ( caster->checkFriend(entity) && caster->friendlyFireProtection(entity) )
									{
										continue;
									}
								}
							}
							Stat* stats = entity->getStats();
							if ( stats && entityInsideEntity(my, entity) )
							{
								if ( !entity->monsterIsTargetable() ) { continue; }
								int damage = 1;
								
								int duration = 0;
								Uint8 strength = 0;
								if ( particleEmitterHitPropsFloorMagic->hits == 0 && my->skill[1] < 10 )
								{
									strength = 1;
									duration = getSpellEffectDurationFromID(SPELL_ICE_WAVE, caster ? caster : my, nullptr, my);
									damage = getSpellDamageFromID(SPELL_ICE_WAVE, caster ? caster : my, nullptr, my); // big damage region
								}
								else
								{
									strength = std::min(8, stats->getEffectActive(EFF_SLOW) + 1);
									duration = getSpellEffectDurationSecondaryFromID(SPELL_ICE_WAVE, caster ? caster : my, nullptr, my);
									damage += strength * getSpellDamageSecondaryFromID(SPELL_ICE_WAVE, caster ? caster : my, nullptr, my);
								}
								if ( damage > 0 )
								{
									if ( applyGenericMagicDamage(caster, entity, caster ? *caster : *my, SPELL_ICE_WAVE, damage, true, true) )
									{
										if ( stats->getEffectActive(EFF_SLOW) )
										{
											duration = std::max(stats->EFFECTS_TIMERS[EFF_SLOW], duration);
										}

										if ( entity->setEffect(EFF_SLOW, strength, duration, false) )
										{

										}
									}
								}
								particleEmitterHitPropsTimer->hits++;
								particleEmitterHitPropsTimer->tick = ticks;

								particleEmitterHitPropsFloorMagic->hits++;
								particleEmitterHitPropsFloorMagic->tick = ticks;
							}
						}
					}
				}
			}
		}
	}

	if ( doParticle )
	{
		if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ICE_WAVE )
		{
			Entity* particle = spawnMagicParticleCustom(my, 225, 0.5, 4);
			if ( particle )
			{
				particle->lightBonus = vec4(0.5f, 0.5f, 0.5f, 0.f);
				particle->flags[SPRITE] = true;
				particle->ditheringDisabled = true;
				particle->vel_x = 0.25 * cos(my->yaw);
				particle->vel_y = 0.25 * sin(my->yaw);
				particle->vel_z = -0.05;

				real_t forward = 2.0 + local_rng.rand() % 5;
				particle->z += -4.0 + local_rng.rand() % 7;
				real_t side = -2.0 + (local_rng.rand() % 5) * 1.0;
				particle->x += forward * cos(my->yaw) + side * cos(my->yaw + PI / 2);
				particle->y += forward * sin(my->yaw) + side * sin(my->yaw + PI / 2);
			}
		}
		else if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_SPORES )
		{
			if ( my->ticks % 10 == 0 || my->ticks == 1 )
			{
				Entity* entity = newEntity(227, 1, map.entities, nullptr); //Sprite entity.

				int cycle = (my->ticks / 10) % 5;
				if ( cycle > 0 )
				{
					entity->x = my->x + 8.0 * cos(my->yaw + cycle * PI / 2 + PI / 4);
					entity->y = my->y + 8.0 * sin(my->yaw + cycle * PI / 2 + PI / 4);
				}
				else
				{
					entity->x = my->x;
					entity->y = my->y;
				}
				entity->z = 6.0;
				entity->ditheringDisabled = true;
				entity->flags[SPRITE] = true;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				entity->flags[UNCLICKABLE] = true;
				entity->flags[BRIGHT] = true;
				entity->behavior = &actSprite;
				entity->skill[0] = 1;
				entity->skill[1] = 6;
				entity->skill[2] = 4;
				entity->vel_z = -0.25;
				if ( multiplayer != CLIENT )
				{
					entity_uids--;
				}
				entity->setUID(-3);
			}
		}
		else if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_MYCELIUM )
		{
			if ( my->ticks % 10 == 0 || my->ticks == 1 )
			{
				Entity* entity = newEntity(248, 1, map.entities, nullptr); //Sprite entity.

				int cycle = (my->ticks / 10) % 5;
				if ( cycle > 0 )
				{
					entity->x = my->x + 8.0 * cos(my->yaw + cycle * PI / 2 + PI / 4);
					entity->y = my->y + 8.0 * sin(my->yaw + cycle * PI / 2 + PI / 4);
				}
				else
				{
					entity->x = my->x;
					entity->y = my->y;
				}
				entity->z = 6.0;
				entity->ditheringDisabled = true;
				entity->flags[SPRITE] = true;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				entity->flags[UNCLICKABLE] = true;
				entity->flags[BRIGHT] = true;
				entity->behavior = &actSprite;
				entity->skill[0] = 1;
				entity->skill[1] = 6;
				entity->skill[2] = 4;
				entity->vel_z = -0.125;
				if ( multiplayer != CLIENT )
				{
					entity_uids--;
				}
				entity->setUID(-3);
			}
		}
		else if ( Entity* particle = spawnMagicParticleCustom(my, my->sprite, my->scalex * 0.7, 1) )
		{
			particle->vel_x = cos(my->yaw);
			particle->vel_y = sin(my->yaw);
		}
	}
}

void waveParticleSetUID(Entity& fx, bool noupdate)
{
	Sint32 val = (1 << 31);
	val |= (Uint8)(22);
	val |= (((Uint16)(fx.actParticleWaveStartFrame) & 0xFFF) << 8);
	val |= (Uint8)(fx.actParticleWaveMagicType & 0xFF) << 20;
	val |= (fx.actParticleWaveLight != 0 ? 1 : 0) << 28;
	fx.skill[2] = val;
}

void particleWaveClientReceive(Entity* my)
{
	if ( !my ) { return; }
	if ( multiplayer != CLIENT ) { return; }

	if ( my->actParticleWaveClientReceived != 0 ) { return; }

	my->actParticleWaveStartFrame = (my->skill[2] >> 8) & 0xFFF;
	my->sprite = my->actParticleWaveStartFrame;
	my->actParticleWaveMagicType = (my->skill[2] >> 20) & 0xFF;
	my->actParticleWaveLight = (my->skill[2] >> 28) & 1;
	if ( my->actParticleWaveMagicType == ParticleTimerEffect_t::EFFECT_FIRE_WAVE )
	{
		my->skill[1] = 6; // frames
		my->skill[5] = 4; // frame time
		my->ditheringOverride = 6;
		real_t startScale = 0.1;
		my->scalex = startScale;
		my->scaley = startScale;
		my->scalez = startScale;
		real_t grouping = 13.75;
		real_t scale = 1.0;
		my->focaly = startScale * grouping;
		my->fskill[0] = scale; // final scale
		my->fskill[1] = grouping; // final grouping
		my->skill[6] = 1; // grow to scale
	}
	else if ( my->actParticleWaveMagicType == ParticleTimerEffect_t::EFFECT_KINETIC_FIELD )
	{
		my->skill[1] = 12; // frames
		my->skill[5] = 4; // frame time
		my->ditheringOverride = 6;

		real_t startScale = 0.1;
		real_t scale = 1.0;
		my->scalex = startScale;
		my->scaley = startScale;
		my->scalez = startScale;
		my->fskill[0] = scale; // final scale
		my->skill[6] = 1; // grow to scale
	}
	else if ( my->actParticleWaveMagicType == ParticleTimerEffect_t::EFFECT_CHRONOMIC_FIELD )
	{
		my->skill[1] = 8; // frames
		my->skill[5] = 4; // frame time
		my->ditheringOverride = 6;

		real_t startScale = 0.1;
		real_t scale = 1.0;
		my->scalex = startScale;
		my->scaley = startScale;
		my->scalez = startScale;
		my->fskill[0] = scale; // final scale
		my->skill[6] = 1; // grow to scale
	}
	else if ( my->actParticleWaveMagicType == ParticleTimerEffect_t::EFFECT_PULSE )
	{
		my->skill[1] = 6; // frames
		my->skill[5] = 4; // frame time
		my->ditheringOverride = 6;
		my->scalex = 1.0;
		my->scalez = 1.0;
	}

	my->actParticleWaveClientReceived = 1;
}

Entity* createParticleWave(ParticleTimerEffect_t::EffectType particleType, int sprite, real_t x, real_t y, real_t z, real_t dir, Uint32 lifetime, bool light)
{
	Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Sprite entity.
	entity->x = x;
	entity->y = y;
	entity->z = z;
	//entity->ditheringDisabled = true;
	entity->flags[SPRITE] = false;
	entity->flags[PASSABLE] = true;
	//entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->flags[BRIGHT] = false;
	entity->scalex = 1.0;
	entity->scaley = 1.0;
	entity->scalez = 1.0;
	entity->behavior = &actParticleWave;
	entity->yaw = dir;
	entity->pitch = 0;
	entity->roll = 0.0;
	entity->skill[0] = lifetime;
	entity->skill[1] = 12; // frames
	entity->skill[3] = 0; // current frame
	entity->skill[4] = entity->sprite; // start frame
	entity->skill[5] = 5; // frame time
	entity->ditheringOverride = 4;
	entity->actParticleWaveMagicType = particleType;
	entity->actParticleWaveLight = light ? 1 : 0;
	entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
		*cvar_magic_fx_light_bonus, 0.f);
	/*if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);*/
	waveParticleSetUID(*entity, true);
	return entity;
}

void createParticleDemesneDoor(real_t x, real_t y, real_t dir)
{
	for ( int c = 0; c <= 8; c++ )
	{
		Entity* entity = newEntity(576, 1, map.entities, nullptr); //Particle entity.
		entity->sizex = 1;
		entity->sizey = 1;
		entity->x = x + (-4.0 + c) * cos(dir + PI / 2);
		entity->y = y + (-4.0 + c) * sin(dir + PI / 2);
		entity->z = -4.0;
		entity->yaw = dir;
		entity->vel_x = 0.0;//0.2;
		entity->vel_y = 0.0;//0.2;
		entity->vel_z = -0.02;
		entity->skill[0] = 100;
		entity->skill[1] = 0; // direction.
		entity->fskill[0] = 0.1;
		entity->behavior = &actParticleErupt;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[UNCLICKABLE] = true;
		entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
			*cvar_magic_fx_light_bonus, 0.f);
		if ( multiplayer != CLIENT )
		{
			entity_uids--;
		}
		entity->setUID(-3);
	}
}

void actParticleDemesneDoor(Entity* my)
{
	if ( PARTICLE_LIFE < 0 )
	{
		createParticleDemesneDoor(my->x, my->y, my->yaw);
		serverSpawnMiscParticlesAtLocation(my->x, my->y, my->yaw * 256.0, PARTICLE_EFFECT_DEMESNE_DOOR, 0);
		my->removeLightField();
		list_RemoveNode(my->mynode);
		if ( Entity* caster = uidToEntity(my->parent) )
		{
			messagePlayer(caster->isEntityPlayer(), MESSAGE_WORLD, Language::get(6691));
		}
		return;
	}

	if ( !my->light )
	{
		my->light = addLight(my->x / 16, my->y / 16, "demesne_door");
	}

	if ( my->skill[1] == 0 )
	{
		my->skill[1] = 1;
		createParticleDemesneDoor(my->x, my->y, my->yaw);
	}

	my->ditheringOverride = 4;

	if ( multiplayer != CLIENT )
	{
		--PARTICLE_LIFE;

		int mapx = my->x / 16;
		int mapy = my->y / 16;
		bool interrupted = false;
		auto entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 1);
		for ( auto it : entLists )
		{
			node_t* node;
			for ( node = it->first; node != nullptr; node = node->next )
			{
				if ( Entity* entity = (Entity*)node->element )
				{
					if ( static_cast<int>(entity->x / 16) == mapx && static_cast<int>(entity->y / 16) == mapy )
					{
						if ( entity == my ) { continue; }
						if ( entity->behavior == &actDoor )
						{
							entity->doorHealth = 0;
						}
						if ( entity->behavior == &actGate && entity->gateStatus == 0 )
						{
							interrupted = true;
						}
						if ( entity->behavior == &actIronDoor && entity->doorStatus == 0 )
						{
							interrupted = true;
						}
						if ( entity->behavior == &actParticleDemesneDoor )
						{
							interrupted = true;
						}
						if ( entity->behavior == &actPlayer || entity->behavior == &actMonster )
						{
							if ( Stat* stats = entity->getStats() )
							{
								if ( stats->type != VAMPIRE )
								{
									if ( entityInsideEntity(my, entity) )
									{
										if ( auto hitProps = getParticleEmitterHitProps(my->getUID(), entity) )
										{
											if ( hitProps->hits == 0 )
											{
												hitProps->hits++;
												hitProps->tick = ticks;
											}
										}
									}
								}
							}
						}
					}
				}
			}

			for ( auto& hitProp : particleTimerEmitterHitEntities[my->getUID()] )
			{
				if ( hitProp.second.hits == 1 )
				{
					if ( Entity* entity = uidToEntity(hitProp.first) )
					{
						if ( Stat* stats = entity->getStats() )
						{
							if ( stats->type != VAMPIRE )
							{
								if ( !entityInsideEntity(my, entity) )
								{
									hitProp.second.hits++;
									hitProp.second.tick = ticks;
									Entity* caster = uidToEntity(my->parent);
									if ( caster && (caster == entity || caster->checkFriend(entity)) )
									{
										int effectStrength = std::min(255, 
											std::min(getSpellDamageSecondaryFromID(SPELL_DEMESNE_DOOR, caster, nullptr, my), 
												std::max(1, getSpellDamageFromID(SPELL_DEMESNE_DOOR, caster, nullptr, my))));
										if ( entity->setEffect(EFF_DEMESNE_DOOR, (Uint8)effectStrength,
											getSpellEffectDurationSecondaryFromID(SPELL_DEMESNE_DOOR, caster, nullptr, my), false) )
										{
											magicOnSpellCastEvent(caster, caster, nullptr, SPELL_DEMESNE_DOOR, spell_t::SPELL_LEVEL_EVENT_EFFECT, 1);
										}
									}
								}
							}
						}
					}
				}
			}
		}

		if ( interrupted )
		{
			if ( Entity* caster = uidToEntity(my->parent) )
			{
				messagePlayer(caster->isEntityPlayer(), MESSAGE_WORLD, Language::get(6690));
			}

			createParticleDemesneDoor(my->x, my->y, my->yaw);
			serverSpawnMiscParticlesAtLocation(my->x, my->y, my->yaw * 256.0, PARTICLE_EFFECT_DEMESNE_DOOR, 0);
			my->removeLightField();
			list_RemoveNode(my->mynode);
			return;
		}
	}
}

void actParticleWave(Entity* my)
{
	Entity* parentTimer = uidToEntity(my->parent);
	if ( PARTICLE_LIFE < 0 || (multiplayer != CLIENT && !parentTimer) )
	{
		my->removeLightField();
		list_RemoveNode(my->mynode);
		return;
	}

	if ( multiplayer != CLIENT && my->scalex >= my->fskill[0] * .9 )
	{
		Entity* caster = uidToEntity(parentTimer->parent);
		if ( my->actParticleWaveMagicType == ParticleTimerEffect_t::EFFECT_CHRONOMIC_FIELD )
		{
			int chronomicLimit = getSpellDamageFromID(SPELL_CHRONOMIC_FIELD, caster, nullptr, my);
			std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 2);
			for ( auto it : entLists )
			{
				if ( my->actParticleWaveVariable1 >= chronomicLimit )
				{
					break;
				}
				node_t* node;
				for ( node = it->first; node != nullptr; node = node->next )
				{
					if ( my->actParticleWaveVariable1 >= chronomicLimit )
					{
						break;
					}
					Entity* entity = (Entity*)node->element;
					if ( entity->behavior == &actArrow || entity->behavior == &actMagicMissile
						|| entity->behavior == &actThrown )
					{
						auto particleEmitterHitProps = getParticleEmitterHitProps(my->parent, entity);
						if ( !particleEmitterHitProps )
						{
							continue;
						}
						if ( particleEmitterHitProps->hits > 0 )
						{
							continue;
						}
						if ( entityInsideEntity(my, entity) )
						{
							particleEmitterHitProps->hits++;
							my->actParticleWaveVariable1++; // total hits

							playSoundEntity(my, 166, 128);
							real_t spd = sqrt(entity->vel_x * entity->vel_x + entity->vel_y * entity->vel_y);
							if ( Entity* parent = uidToEntity(entity->parent) )
							{
								real_t dir = atan2(entity->y - parent->y, entity->x - parent->x) + PI;
								entity->vel_x = spd * cos(dir);
								entity->vel_y = spd * sin(dir);
								if ( entity->behavior == &actArrow || entity->behavior == &actMagicMissile )
								{
									entity->yaw = dir;
								}

								if ( caster && caster != parent )
								{
									magicOnSpellCastEvent(caster, caster, parent,
										SPELL_CHRONOMIC_FIELD, spell_t::SPELL_LEVEL_EVENT_DEFAULT, 1);
								}
							}
							else
							{
								real_t spd = sqrt(entity->vel_x * entity->vel_x + entity->vel_y * entity->vel_y);
								real_t dir = atan2(entity->vel_y, entity->vel_x) + PI;
								entity->vel_x = spd * cos(dir);
								entity->vel_y = spd * sin(dir);
								if ( entity->behavior == &actArrow || entity->behavior == &actMagicMissile )
								{
									entity->yaw = dir;
								}
							}
							if ( parentTimer->parent == entity->parent ) // own spell
							{
								entity->parent = my->getUID();
							}
							else
							{
								entity->parent = parentTimer->parent;
							}
						}
					}
				}
			}

			if ( my->actParticleWaveVariable1 >= chronomicLimit )
			{
				parentTimer->skill[0] = 1; // decay parent
				my->removeLightField();
				list_RemoveNode(my->mynode);
				return;
			}
		}
		else if ( my->actParticleWaveMagicType == ParticleTimerEffect_t::EFFECT_KINETIC_FIELD )
		{
			std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 2);
			for ( auto it : entLists )
			{
				node_t* node;
				for ( node = it->first; node != nullptr; node = node->next )
				{
					Entity* entity = (Entity*)node->element;
					if ( entity->getStats() )
					{
						if ( !entity->monsterIsTargetable() ) { continue; }
						if ( entityInsideEntity(my, entity) )
						{
							auto particleEmitterHitProps = getParticleEmitterHitProps(my->parent, entity);
							if ( !particleEmitterHitProps )
							{
								continue;
							}
							if ( particleEmitterHitProps->hits > 0 && ((ticks - particleEmitterHitProps->tick) < 60) )
							{
								continue;
							}

							real_t dir = (my->yaw - PI / 2);
							while ( dir < 0.0 )
							{
								dir += 2 * PI;
							}
							while ( dir >= 2 * PI )
							{
								dir -= 2 * PI;
							}

							if ( entity->behavior == &actPlayer )
							{
								particleEmitterHitProps->hits++;
								particleEmitterHitProps->tick = ticks;
								playSoundEntity(entity, 180, 128);
								spawnMagicEffectParticles(entity->x, entity->y, entity->z, 982);
								Uint8 effectStrength = 2 + (1 * (MAXPLAYERS + 1));
								if ( caster && caster->behavior == &actPlayer )
								{
									effectStrength += caster->skill[2];
								}
								else
								{
									effectStrength += MAXPLAYERS;
								}
								entity->setEffect(EFF_DASH, effectStrength, 60, false);
								int player = entity->skill[2];
								if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
								{
									strcpy((char*)net_packet->data, "KINE");
									SDLNet_Write32((dir) * 256.0, &net_packet->data[4]);
									net_packet->address.host = net_clients[player - 1].host;
									net_packet->address.port = net_clients[player - 1].port;
									net_packet->len = 8;
									sendPacketSafe(net_sock, -1, net_packet, player - 1);
								}
								else
								{
									real_t vel = sqrt(pow(entity->vel_y, 2) + pow(entity->vel_x, 2));
									entity->monsterKnockbackVelocity = std::min(2.25, std::max(1.0, vel));
									entity->monsterKnockbackTangentDir = dir;
								}
								if ( particleEmitterHitProps->hits == 1 )
								{
									magicOnSpellCastEvent(caster, caster, nullptr, SPELL_KINETIC_FIELD, spell_t::SPELL_LEVEL_EVENT_DEFAULT | spell_t::SPELL_LEVEL_EVENT_MINOR_CHANCE, 1);
								}
							}
							else if ( entity->behavior == &actMonster )
							{
								particleEmitterHitProps->hits++;
								particleEmitterHitProps->tick = ticks;
								if ( entity->setEffect(EFF_KNOCKBACK, true, 30, false) )
								{
									playSoundEntity(entity, 180, 128);
									spawnMagicEffectParticles(entity->x, entity->y, entity->z, 982);
									entity->setEffect(EFF_DASH, true, 15, false);

									real_t push = 1.5;
									entity->vel_x = cos(dir) * push;
									entity->vel_y = sin(dir) * push;
									entity->monsterKnockbackVelocity = 0.01;
									entity->monsterKnockbackTangentDir = dir;
									entity->monsterKnockbackUID = parentTimer->parent;

									if ( particleEmitterHitProps->hits == 1 )
									{
										magicOnSpellCastEvent(caster, caster, entity, SPELL_KINETIC_FIELD, spell_t::SPELL_LEVEL_EVENT_DEFAULT | spell_t::SPELL_LEVEL_EVENT_MINOR_CHANCE, 1);
									}
								}
							}
						}
					}
				}
			}
		}
		else if ( my->actParticleWaveMagicType == ParticleTimerEffect_t::EFFECT_FIRE_WAVE )
		{
			std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 2);
			real_t size = 2;
			for ( auto it : entLists )
			{
				node_t* node;
				for ( node = it->first; node != nullptr; node = node->next )
				{
					Entity* entity = (Entity*)node->element;
					if ( true/*entity->behavior == &actPlayer || (entity->behavior == &actMonster && !entity->isInertMimic())*/ )
					{
						real_t yaw = my->yaw + PI / 2;
						real_t x = my->x + my->focaly * cos(yaw) - 8.0 * cos(yaw + PI / 2);
						real_t y = my->y + my->focaly * sin(yaw) - 8.0 * sin(yaw + PI / 2);

						real_t tangent = yaw + PI / 2;
						while ( tangent > PI )
						{
							tangent -= 2 * PI;
						}
						while ( tangent <= -PI )
						{
							tangent += 2 * PI;
						}

						auto particleEmitterHitProps = getParticleEmitterHitProps(my->parent, entity);
						if ( !particleEmitterHitProps )
						{
							continue;
						}
						if ( particleEmitterHitProps->hits >= 5 || (particleEmitterHitProps->hits > 0 && ((ticks - particleEmitterHitProps->tick) < 20)) )
						{
							continue;
						}

						if ( caster && caster->behavior == &actMonster )
						{
							if ( caster == entity || caster->checkFriend(entity) )
							{
								continue;
							}
						}
						
						int damage = getSpellDamageFromID(SPELL_FIRE_WALL, caster, nullptr, my);
						for ( int i = 0; i < 8; ++i )
						{
							real_t ix = x + i * 2.0 * cos(tangent);
							real_t iy = y + i * 2.0 * sin(tangent);

							if ( ix + size > entity->x - entity->sizex )
							{
								if ( ix - size < entity->x + entity->sizex )
								{
									if ( iy + size > entity->y - entity->sizey )
									{
										if ( iy - size < entity->y + entity->sizey )
										{
											//Entity* particle = spawnMagicParticle(my);
											//particle->sprite = 942;
											//particle->x = ix;
											//particle->y = iy;
											//particle->z = 0;
											Stat* stats = (entity->behavior == &actPlayer || entity->behavior == &actMonster) ? entity->getStats() : nullptr;
											if ( !stats || entity->isInertMimic() )
											{
												if ( applyGenericMagicDamage(caster, entity, caster ? *caster : *my, SPELL_FIRE_WALL, damage, true) )
												{
													if ( entity->flags[BURNABLE] && !entity->flags[BURNING] )
													{
														if ( local_rng.rand() % 3 < (particleEmitterHitProps->hits + 1) )
														{
															entity->SetEntityOnFire(caster);
														}
													}
												}
												else if ( entity->behavior == &actGreasePuddleSpawner )
												{
													entity->SetEntityOnFire(caster);
												}
												particleEmitterHitProps->hits++;
												particleEmitterHitProps->tick = ticks;
											}
											else if ( stats )
											{
												if ( !entity->monsterIsTargetable() ) { continue; }

												bool doKnockback = true;
												bool doDamage = true;

												if ( particleEmitterHitProps->hits >= 3 )
												{
													doDamage = false;
												}

												if ( caster && caster->behavior == &actPlayer )
												{
													if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
													{
														if ( caster->checkFriend(entity) && caster->friendlyFireProtection(entity) )
														{
															doDamage = false;
															particleEmitterHitProps->hits++;
															particleEmitterHitProps->tick = ticks;
														}
													}
												}

												if ( doDamage && applyGenericMagicDamage(caster, entity, caster ? *caster : *my, SPELL_FIRE_WALL, damage, true, true) )
												{
													particleEmitterHitProps->hits++;
													particleEmitterHitProps->tick = ticks;

													if ( entity->flags[BURNABLE] && !entity->flags[BURNING] )
													{
														if ( local_rng.rand() % 10 < (particleEmitterHitProps->hits + 1) )
														{
															if ( entity->SetEntityOnFire(caster) )
															{
																if ( caster )
																{
																	stats->burningInflictedBy = caster->getUID();
																}
																entity->char_fire = std::min(entity->char_fire, getSpellEffectDurationFromID(SPELL_FIRE_WALL, caster, nullptr, my));
															}
														}
													}
												}

												if ( doKnockback && !stats->getEffectActive(EFF_KNOCKBACK)
													/*&& !entity->flags[BURNING]*/
													&& entity->setEffect(EFF_KNOCKBACK, true, 20, false) )
												{
													real_t tangent2 = atan2(iy - entity->y, ix - entity->x);
													real_t yawDiff = tangent2 - tangent;
													while ( yawDiff > PI )
													{
														yawDiff -= 2 * PI;
													}
													while ( yawDiff <= -PI )
													{
														yawDiff += 2 * PI;
													}
													real_t pushback = 0.3;
													if ( entity->behavior == &actPlayer )
													{
														entity->monsterKnockbackVelocity = pushback;
														entity->monsterKnockbackTangentDir = yawDiff >= 0 ? yaw : yaw + PI;
														if ( !players[entity->skill[2]]->isLocalPlayer() )
														{
															serverUpdateEntityFSkill(entity, 11);
															serverUpdateEntityFSkill(entity, 9);
														}
													}
													else if ( entity->behavior == &actMonster )
													{
														entity->vel_x = cos(yawDiff >= 0 ? yaw : yaw + PI) * pushback;
														entity->vel_y = sin(yawDiff >= 0 ? yaw : yaw + PI) * pushback;
														entity->monsterKnockbackVelocity = 0.01;
														entity->monsterKnockbackTangentDir = tangent;
														entity->monsterKnockbackUID = parentTimer->parent;
													}
												}
											}
											break;
										}
									}
								}
							}
						}
					}
				}
			}
		}


		//real_t dist = lineTraceTarget(my, x, y, tangent, 64, 0, false, entity);
		/*if ( hit.entity == entity )
		{
			messagePlayer(0, MESSAGE_DEBUG, "Hit");
		}*/
	}

	//my->yaw += 0.025;
	if ( my->skill[6] == 1 ) // grow to scale
	{
		real_t increment = std::max(.05, (1.0 - my->scalex)) / 3.0;
		my->scalex = std::min(my->fskill[0], my->scalex + increment);
		my->scaley = std::min(my->fskill[0], my->scaley + increment);
		my->scalez = std::min(my->fskill[0], my->scalez + increment);
		my->focaly = my->fskill[1] * my->scalex;
		//if ( Entity* particle = spawnMagicParticleCustom(my, my->sprite, my->scalex * 0.7, 1) )
		//{
		//	particle->focaly = my->focaly / 2;
		//	particle->x = my->x;
		//	particle->y = my->y;
		//	particle->z = my->z;
		//	particle->ditheringOverride = my->ditheringOverride;
		//	//particle->vel_x = cos(my->yaw + PI / 2);
		//	//particle->vel_y = sin(my->yaw + PI / 2);
		//}

		if ( my->actParticleWaveMagicType == ParticleTimerEffect_t::EFFECT_FIRE_WAVE )
		{
			if ( my->ticks == 1 )
			{
				for ( int i = 0; i < 10; ++i )
				{
					Entity* entity = newEntity(16, 1, map.entities, nullptr); //Sprite entity.
					entity->behavior = &actFlame;
					entity->x = my->x;
					entity->y = my->y;
					entity->z = my->z;
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
			}

			if ( my->skill[7] == 1 )
			{
				if ( !my->light )
				{
					playSoundEntityLocal(my, 164, 128);
					my->light = addLight(my->x / 16, my->y / 16, "explosion");
				}
			}
		}
		else if ( my->actParticleWaveMagicType == ParticleTimerEffect_t::EFFECT_KINETIC_FIELD )
		{
			if ( my->skill[7] == 1 )
			{
				if ( !my->light )
				{
					//playSoundEntityLocal(my, 164, 128);
					my->light = addLight(my->x / 16, my->y / 16, "magic_purple");
				}
			}
		}
		else if ( my->actParticleWaveMagicType == ParticleTimerEffect_t::EFFECT_CHRONOMIC_FIELD )
		{
			if ( my->skill[7] == 1 )
			{
				if ( !my->light )
				{
					//playSoundEntityLocal(my, 164, 128);
					my->light = addLight(my->x / 16, my->y / 16, "magic_green");
				}
			}
		}

		if ( my->scalex >= my->fskill[0] )
		{
			my->skill[6] = 0;
			my->removeLightField();
		}
	}
	else
	{
		if ( my->skill[7] == 1 )
		{
			const char* color = nullptr;
			const char* color_flicker = nullptr;
			if ( my->actParticleWaveMagicType == ParticleTimerEffect_t::EFFECT_CHRONOMIC_FIELD )
			{
				color = "magic_green_flicker";
				color_flicker = "magic_green_flicker";
			}
			else if ( my->actParticleWaveMagicType == ParticleTimerEffect_t::EFFECT_KINETIC_FIELD )
			{
				color = "magic_purple_flicker";
				color_flicker = "magic_purple_flicker";
			}
			else if ( my->actParticleWaveMagicType == ParticleTimerEffect_t::EFFECT_FIRE_WAVE )
			{
				color = "campfire";
				color_flicker = "campfire_flicker";
			}
			if ( flickerLights )
			{
				if ( my->ticks % 10 < 5 )
				{
					my->removeLightField();
					my->light = addLight(my->x / 16, my->y / 16, color);
				}
				else
				{
					my->removeLightField();
					my->light = addLight(my->x / 16, my->y / 16, color_flicker);
				}
			}
			else
			{
				if ( !my->light )
				{
					my->light = addLight(my->x / 16, my->y / 16, color);
				}
			}
		}
	}

	if ( my->skill[7] == 1 && my->actParticleWaveMagicType == ParticleTimerEffect_t::EFFECT_FIRE_WAVE )
	{
#ifdef USE_FMOD
		if ( my->skill[8] == 0 )
		{
			my->skill[8]--;
			my->stopEntitySound();
			my->entity_sound = playSoundEntityLocal(my, 710, 64);
		}
		if ( my->entity_sound )
		{
			bool playing = false;
			my->entity_sound->isPlaying(&playing);
			if ( !playing )
			{
				my->entity_sound = nullptr;
			}
			else
			{
				FMOD_VECTOR position;
				position.x = (float)(my->x / (real_t)16.0);
				position.y = (float)(0.0);
				position.z = (float)(my->y / (real_t)16.0);
				fmod_result = my->entity_sound->set3DAttributes(&position, nullptr);
			}
		}
#else
		my->skill[8]--;
		if ( my->skill[8] <= 0 )
		{
			my->skill[8] = 480;
			playSoundEntityLocal(my, 133, 128);
		}
#endif
	}

	if ( multiplayer != CLIENT )
	{
		--PARTICLE_LIFE;
	}
	if ( ::ticks % my->skill[5] == 0 )
	{
		my->sprite = my->skill[4] + my->skill[3];
		++my->skill[3];
		if ( my->skill[3] >= my->skill[1] )
		{
			my->skill[3] = 0;
		}
	}
}

Entity* createParticleBoobyTrapExplode(Entity* caster, real_t x, real_t y)
{
	Entity* spellTimer = createParticleTimer(caster, 25, -1);
	spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_BOOBY_TRAP;
	spellTimer->particleTimerCountdownSprite = -1;
	spellTimer->x = x;
	spellTimer->y = y;

	Uint32 color = makeColor(255, 0, 255, 255);
	if ( Entity* fx = createParticleAOEIndicator(spellTimer, spellTimer->x, spellTimer->y, 0.0, TICKS_PER_SECOND, 32) )
	{
		fx->actSpriteCheckParentExists = 0;
		if ( auto indicator = AOEIndicators_t::getIndicator(fx->skill[10]) )
		{
			Uint8 r, g, b, a;
			getColor(color, &r, &g, &b, &a);
			a *= 0.5;
			indicator->indicatorColor = makeColor(r, g, b, a);
			indicator->loop = false;
			indicator->framesPerTick = 4;
			indicator->ticksPerUpdate = 1;
			indicator->delayTicks = 0;
			indicator->cacheType = AOEIndicators_t::CACHE_BOOBY_TRAP;
		}
	}
	for ( int i = 0; i < 2; ++i )
	{
		if ( Entity* fx = createParticleAOEIndicator(spellTimer, spellTimer->x, spellTimer->y, -7.5, TICKS_PER_SECOND, 32) )
		{
			fx->actSpriteCheckParentExists = 0;
			if ( i == 1 )
			{
				fx->pitch = PI;
			}
			if ( auto indicator = AOEIndicators_t::getIndicator(fx->skill[10]) )
			{
				indicator->indicatorColor = color;
				indicator->loop = false;
				indicator->framesPerTick = 4;
				indicator->ticksPerUpdate = 1;
				indicator->delayTicks = 15;
				indicator->cacheType = AOEIndicators_t::CACHE_BOOBY_TRAP2;
			}
		}
	}
	return spellTimer;
}

Entity* createParticleShatterObjects(Entity* caster)
{
	if ( !caster ) { return nullptr; }
	Entity* spellTimer = createParticleTimer(caster, 25, -1);
	spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SHATTER;
	spellTimer->particleTimerCountdownSprite = -1;
	spellTimer->x = caster->x;
	spellTimer->y = caster->y;

	Uint32 color = makeColor(128, 128, 255, 255);
	if ( Entity* fx = createParticleAOEIndicator(spellTimer, spellTimer->x, spellTimer->y, 0.0, 1.25 * TICKS_PER_SECOND, 32) )
	{
		fx->actSpriteCheckParentExists = 0;
		if ( auto indicator = AOEIndicators_t::getIndicator(fx->skill[10]) )
		{
			Uint8 r, g, b, a;
			getColor(color, &r, &g, &b, &a);
			a *= 0.5;
			indicator->indicatorColor = makeColor(r, g, b, a);
			indicator->loop = false;
			indicator->framesPerTick = 2;
			indicator->ticksPerUpdate = 1;
			indicator->delayTicks = 0;
			indicator->cacheType = AOEIndicators_t::CACHE_SHATTER_OBJECTS;
		}
	}
	for ( int i = 0; i < 2; ++i )
	{
		if ( Entity* fx = createParticleAOEIndicator(spellTimer, spellTimer->x, spellTimer->y, -7.5, 1.25 * TICKS_PER_SECOND, 32) )
		{
			fx->actSpriteCheckParentExists = 0;
			if ( i == 1 )
			{
				fx->pitch = PI;
			}
			if ( auto indicator = AOEIndicators_t::getIndicator(fx->skill[10]) )
			{
				indicator->indicatorColor = color;
				indicator->loop = false;
				indicator->framesPerTick = 2;
				indicator->ticksPerUpdate = 1;
				indicator->delayTicks = 15;
				indicator->cacheType = AOEIndicators_t::CACHE_SHATTER_OBJECTS2;
			}
		}
	}
	return spellTimer;
}

Entity* createParticleIgnite(Entity* caster)
{
	if ( !caster ) { return nullptr; }
	Entity* spellTimer = createParticleTimer(caster, 25, -1);
	spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_IGNITE;
	spellTimer->particleTimerCountdownSprite = -1;
	spellTimer->x = caster->x;
	spellTimer->y = caster->y;

	Uint32 color = makeColor(255, 128, 0, 255);
	if ( Entity* fx = createParticleAOEIndicator(spellTimer, spellTimer->x, spellTimer->y, 0.0, 1.25 * TICKS_PER_SECOND, 32) )
	{
		fx->actSpriteCheckParentExists = 0;
		if ( auto indicator = AOEIndicators_t::getIndicator(fx->skill[10]) )
		{
			Uint8 r, g, b, a;
			getColor(color, &r, &g, &b, &a);
			a *= 0.5;
			indicator->indicatorColor = makeColor(r, g, b, a);
			indicator->loop = false;
			indicator->framesPerTick = 2;
			indicator->ticksPerUpdate = 1;
			indicator->delayTicks = 0;
			indicator->cacheType = AOEIndicators_t::CACHE_IGNITE;
		}
	}
	for ( int i = 0; i < 2; ++i )
	{
		if ( Entity* fx = createParticleAOEIndicator(spellTimer, spellTimer->x, spellTimer->y, -7.5, 1.25 * TICKS_PER_SECOND, 32) )
		{
			fx->actSpriteCheckParentExists = 0;
			if ( i == 1 )
			{
				fx->pitch = PI;
			}
			if ( auto indicator = AOEIndicators_t::getIndicator(fx->skill[10]) )
			{
				indicator->indicatorColor = color;
				indicator->loop = false;
				indicator->framesPerTick = 2;
				indicator->ticksPerUpdate = 1;
				indicator->delayTicks = 15;
				indicator->cacheType = AOEIndicators_t::CACHE_IGNITE2;
			}
		}
	}
	return spellTimer;
}

void tunnelPortalSetAttributes(Entity* portal, int duration, int dir)
{
	if ( !portal ) { return; }

	portal->z = 0.0;
	portal->yaw = (dir + 2) * PI / 2;

	portal->sprite = 1810;
	portal->scalex = 0.1;
	portal->scaley = 0.1;
	portal->scalez = 0.1;
	portal->sizex = 4;
	portal->sizey = 4;
	portal->behavior = &actTeleporter;
	portal->teleporterType = 3;
	portal->teleporterStartFrame = 1810;
	portal->teleporterCurrentFrame = 0;
	portal->teleporterNumFrames = 4;
	portal->teleporterDuration = duration;
	portal->flags[PASSABLE] = true;

	if ( multiplayer != CLIENT )
	{
		portal->flags[UPDATENEEDED] = true;
		Uint32 val = (1 << 31);
		val |= (Uint8)(26);
		val |= ((Uint8)(duration & 0xFFFF) << 8);
		val |= ((Uint8)(dir & 0xF) << 24);
		portal->skill[2] = val;
	}
}

Entity* createTunnelPortal(real_t x, real_t y, int duration, int dir, Entity* caster)
{
	if ( dir == 0 ) { return nullptr; }
	int checkx = x;
	int checky = y;
	if ( dir == 1 )
	{
		checkx += 1;
	}
	else if ( dir == 3 )
	{
		checkx -= 1;
	}
	else if ( dir == 2 )
	{
		checky += 1;
	}
	else if ( dir == 4 )
	{
		checky -= 1;
	}

	if ( checkObstacle((checkx << 4) + 8, (checky << 4) + 8, caster, nullptr, true, true, false, false) )
	{
		return nullptr;
	}

	// check destination
	int lastx = x;
	int lasty = y;
	int startx = x;
	int starty = y;
	checkx = x;
	checky = y;
	if ( !mapTileDiggable(checkx, checky) )
	{
		return nullptr;
	}
	int walls = 0;
	while ( true )
	{
		bool goodspot = false;
		if ( checkx > 0 && checkx < map.width - 1 && checky > 0 && checky < map.height - 1 )
		{
			int mapIndex = (checky)*MAPLAYERS + (checkx) * MAPLAYERS * map.height;
			if ( !map.tiles[OBSTACLELAYER + mapIndex] )
			{
				if ( !checkObstacle((checkx << 4) + 8, (checky << 4) + 8, caster, nullptr, true, true, false, false) )
				{
					if ( walls > 0 )
					{
						lastx = checkx;
						lasty = checky;
						goodspot = true;
						break;
					}
				}
			}
			else
			{
				++walls;
			}
		}
		else
		{
			break;
		}

		if ( dir == 1 )
		{
			checkx -= 1;
		}
		else if ( dir == 3 )
		{
			checkx += 1;
		}
		else if ( dir == 2 )
		{
			checky -= 1;
		}
		else if ( dir == 4 )
		{
			checky += 1;
		}
	}

	if ( lastx == startx && lasty == starty )
	{
		return nullptr;
	}

	Entity* portal = newEntity(1810, 1, map.entities, nullptr);
	const real_t wallDist = 0.5;
	if ( dir == 1 )
	{
		portal->x = x * 16.0 + 16.0 + wallDist;
		portal->y = y * 16.0 + 8.0;
	}
	else if ( dir == 3 )
	{
		portal->x = x * 16.0 - wallDist;
		portal->y = y * 16.0 + 8.0;
	}
	else if ( dir == 2 )
	{
		portal->x = x * 16.0 + 8.0;
		portal->y = y * 16.0 + 16.0 + wallDist;
	}
	else if ( dir == 4 )
	{
		portal->x = x * 16.0 + 8.0;
		portal->y = y * 16.0 - wallDist;
	}
	tunnelPortalSetAttributes(portal, duration, dir);
	if ( caster )
	{
		portal->parent = caster->getUID();
	}
	portal->teleporterX = lastx;
	portal->teleporterY = lasty;
	return portal;
}

Entity* createWindMagic(Uint32 casterUID, int x, int y, int duration, int dir, int length)
{
	Entity* caster = uidToEntity(casterUID);
	Entity* fx = createParticleAOEIndicator(caster, x, y, 0.0, duration, 32);
	fx->roll = PI;
	fx->z = 0.0;
	fx->yaw = dir * PI / 2;
	fx->actSpriteFollowUID = 0;
	//fx->actSpritePitchRotate = 0.1;
	fx->scalex = 0.5;
	fx->scaley = 0.5;
	if ( auto indicator = AOEIndicators_t::getIndicator(fx->skill[10]) )
	{
		//indicator->arc = PI / 4;
		indicator->framesPerTick = 4;
		indicator->ticksPerUpdate = 4;
	}
	if ( dir == 1 )
	{
		fx->x = fx->x * 16.0 + 16.001;
		fx->y = fx->y * 16.0 + 8.0;
	}
	else if ( dir == 3 )
	{
		fx->x = fx->x * 16.0 - 0.001;
		fx->y = fx->y * 16.0 + 8.0;
	}
	else if ( dir == 2 )
	{
		fx->x = fx->x * 16.0 + 8.0;
		fx->y = fx->y * 16.0 + 16.001;
	}
	else if ( dir == 4 )
	{
		fx->x = fx->x * 16.0 + 8.0;
		fx->y = fx->y * 16.0 - 0.001;
	}

	Entity* wind = newEntity(-1, 1, map.entities, nullptr);
	wind->behavior = &actWind;
	wind->yaw = (dir - 1) * PI / 2;
	if ( dir == 1 )
	{
		wind->x = x * 16.0 + 16.0 + 8.0;
		wind->y = y * 16.0 + 8.0;
	}
	else if ( dir == 3 )
	{
		wind->x = x * 16.0 - 16.0 + 8.0;
		wind->y = y * 16.0 + 8.0;
	}
	else if ( dir == 2 )
	{
		wind->x = x * 16.0 + 8.0;
		wind->y = y * 16.0 + 16.0 + 8.0;
	}
	else if ( dir == 4 )
	{
		wind->x = x * 16.0 + 8.0;
		wind->y = y * 16.0 - 16.0 + 8.0;
	}
	wind->flags[PASSABLE] = true;
	wind->flags[INVISIBLE] = true;
	wind->flags[UPDATENEEDED] = false;
	wind->flags[NOUPDATE] = true;
	wind->sizex = 6;
	wind->sizey = 6;
	wind->actWindParticleEffect = 1;
	wind->actWindEffectsProjectiles = 1;
	wind->actWindTileBonusLength = length;
	wind->actWindLifetime = duration;
	wind->parent = casterUID;
	/*if ( multiplayer != CLIENT )
	{
		--entity_uids;
	}
	wind->setUID(-3);*/
	return wind;
}

void actRadiusMagicBadge(Entity* my)
{
	Entity* parent = uidToEntity(my->parent);
	if ( !parent )
	{
		for ( int i = 0; i < 4; ++i )
		{
			Entity* fx = spawnMagicParticle(my);
			fx->vel_x = 0.5 * cos(my->yaw + i * PI / 2);
			fx->vel_y = 0.5 * sin(my->yaw + i * PI / 2);
		}

		Entity* fx = createParticleAestheticOrbit(my, my->sprite, TICKS_PER_SECOND / 4, PARTICLE_EFFECT_NULL_PARTICLE_NOSOUND);
		fx->x = my->x;
		fx->y = my->y;
		fx->z = my->z;
		fx->yaw = my->yaw;
		fx->actmagicOrbitDist = 0;
		fx->actmagicNoLight = 1;

		list_RemoveNode(my->mynode);
		return;
	}

	if ( my->actRadiusMagicFollowUID != 0 )
	{
		// if we want this to bubble in and out as an entity walks between the radius distance
		bool invis = true;
		if ( Entity* follow = uidToEntity(my->actRadiusMagicFollowUID) )
		{
			my->x = follow->x;
			my->y = follow->y;
			if ( entityDist(parent, follow) <= parent->actRadiusMagicDist + 4.0 )
			{
				invis = false;
			}
		}

		if ( invis && !my->flags[INVISIBLE] )
		{
			for ( int i = 0; i < 4; ++i )
			{
				Entity* fx = spawnMagicParticle(my);
				fx->vel_x = 0.5 * cos(my->yaw + i * PI / 2);
				fx->vel_y = 0.5 * sin(my->yaw + i * PI / 2);
			}
			Entity* fx = createParticleAestheticOrbit(my, my->sprite, TICKS_PER_SECOND / 4, PARTICLE_EFFECT_NULL_PARTICLE_NOSOUND);
			fx->x = my->x;
			fx->y = my->y;
			fx->z = my->z;
			fx->yaw = my->yaw;
			fx->actmagicOrbitDist = 0;
			fx->actmagicNoLight = 1;

			my->skill[1] = 0;
			my->skill[3] = 0;
			my->z = 4.0;
			my->vel_z = -0.8;
			my->scalex = 0.1;
			my->scaley = 0.1;
			my->scalez = 0.1;
		}
		my->flags[INVISIBLE] = invis;
	}
	else
	{
		my->x = parent->x;
		my->y = parent->y;
	}

	if ( my->flags[INVISIBLE] )
	{
		return;
	}

	if ( my->skill[1] >= 50 ) // stop changing size
	{
		real_t maxspeed = .03;
		real_t acceleration = 0.95;
		if ( my->skill[3] == 0 )
		{
			// once off, store the normal height of the particle.
			my->skill[3] = 1;
			my->vel_z = -maxspeed;
		}

		// bob up and down movement.
		if ( my->skill[3] == 1 )
		{
			my->vel_z *= acceleration;
			if ( my->vel_z > -0.005 )
			{
				my->skill[3] = 2;
				my->vel_z = -0.005;
			}
			my->z += my->vel_z;
		}
		else if ( my->skill[3] == 2 )
		{
			my->vel_z /= acceleration;
			if ( my->vel_z < -maxspeed )
			{
				my->skill[3] = 3;
				my->vel_z = -maxspeed;
			}
			my->z -= my->vel_z;
		}
		else if ( my->skill[3] == 3 )
		{
			my->vel_z *= acceleration;
			if ( my->vel_z > -0.005 )
			{
				my->skill[3] = 4;
				my->vel_z = -0.005;
			}
			my->z -= my->vel_z;
		}
		else if ( my->skill[3] == 4 )
		{
			my->vel_z /= acceleration;
			if ( my->vel_z < -maxspeed )
			{
				my->skill[3] = 1;
				my->vel_z = -maxspeed;
			}
			my->z += my->vel_z;
		}
		my->yaw += 0.01;
	}
	else
	{
		my->z += my->vel_z;
		my->yaw += my->vel_z * 2;

		real_t maxScale = 1.0;
		real_t badgeScale = 1.0;
		if ( my->sprite == 2385 || my->sprite == 2386
			|| my->sprite == 2387
			|| my->sprite == 2388
			|| my->sprite == 2389
			|| my->sprite == 2390
			|| my->sprite == 2391 
			|| my->sprite == 2392 
			|| my->sprite == 2393
			|| my->sprite == 2394
			|| my->sprite == 2395
			|| my->sprite == 2396
			|| my->sprite == 2397
			|| my->sprite == 2398
			|| my->sprite == 2399
			|| my->sprite == 2400
			|| my->sprite == 2402
			|| my->sprite == 2403
			|| my->sprite == 2408
			|| my->sprite == 2409 )
		{
			badgeScale = 0.5;
		}

		if ( my->scalex < badgeScale )
		{
			my->scalex += 0.04 * badgeScale / maxScale;
		}
		else
		{
			my->scalex = badgeScale;
		}
		my->scaley = my->scalex;
		my->scalez = my->scalex;
		if ( my->z < -3 + my->fskill[0] )
		{
			my->vel_z *= 0.9;
		}
	}
	++my->skill[1];
}

void radiusMagicClientReceive(Entity* entity)
{
	if ( entity )
	{
		entity->flags[NOUPDATE] = (entity->skill[2] >> 30) & 1;
		entity->actRadiusMagicDist = (entity->skill[2] >> 20) & 0xFF;
		entity->actRadiusMagicID = (entity->skill[2] >> 8) & 0xFFF;
		entity->flags[PASSABLE] = true;
		entity->flags[UNCLICKABLE] = true;
		entity->flags[UPDATENEEDED] = true;
		entity->flags[INVISIBLE] = true;
	}
}

void radiusMagicSetUID(Entity& fx, bool noupdate)
{
	Sint32 val = (1 << 31);
	if ( noupdate )
	{
		val |= (1 << 30);
	}
	val |= (Uint8)(24);
	val |= (((Uint16)(fx.actRadiusMagicID) & 0xFFF) << 8);
	val |= (((Uint8)(fx.actRadiusMagicDist) & 0xFF) << 20);
	fx.skill[2] = val;
}

Entity* createRadiusMagic(int spellID, Entity* caster, real_t x, real_t y, real_t radius, Uint32 lifetime, Entity* follow)
{
	if ( !caster )
	{
		return nullptr;
	}

	int sprite = -1;
	switch ( spellID )
	{
	case SPELL_NULL_AREA:
		sprite = 1817;
		break;
	case SPELL_SPHERE_SILENCE:
		sprite = 1818;
		break;
	case SPELL_HEAL_PULSE:
		sprite = 1686;
		break;
	case SPELL_FOCI_DARK_LIFE:
		sprite = 2179;
		break;
	case SPELL_FOCI_DARK_SILENCE:
		sprite = 2180;
		break;
	case SPELL_FOCI_DARK_SUPPRESS:
		sprite = 2181;
		break;
	case SPELL_FOCI_DARK_VENGEANCE:
		sprite = 2182;
		break;
	case SPELL_FOCI_DARK_RIFT:
		sprite = 2183;
		break;
	case SPELL_FOCI_LIGHT_JUSTICE:
		sprite = 2184;
		break;
	case SPELL_FOCI_LIGHT_PEACE:
		sprite = 2185;
		break;
	case SPELL_FOCI_LIGHT_PROVIDENCE:
		sprite = 2186;
		break;
	case SPELL_FOCI_LIGHT_PURITY:
		sprite = 2187;
		break;
	case SPELL_FOCI_LIGHT_SANCTUARY:
		sprite = 2188;
		break;
	case SPELL_HEAL_MINOR:
	case SPELL_HEAL_OTHER:
		sprite = 2384;
		break;
	case SPELL_DIVINE_ZEAL:
		sprite = 2385;
		break;
	case SPELL_PROF_STURDINESS:
		sprite = 2386;
		break;
	case SPELL_PROF_GREATER_MIGHT:
		sprite = 2387;
		break;
	case SPELL_PROF_NIMBLENESS:
		sprite = 2388;
		break;
	case SPELL_PROF_COUNSEL:
		sprite = 2389;
		break;
	case SPELL_COMMAND:
		sprite = 2390;
		break;
	case SPELL_SEEK_ALLY:
		sprite = 2392;
		break;
	case SPELL_SEEK_FOE:
		sprite = 2393;
		break;
	case SPELL_TABOO:
		sprite = 2391;
		break;
	case SPELL_DETECT_ENEMY:
		sprite = 2395;
		break;
	case SPELL_SCRY_ALLIES:
		sprite = 2394;
		break;
	case SPELL_DONATION:
		sprite = 2396;
		break;
	case SPELL_BLESS_FOOD:
		sprite = 2400;
		break;
	case SPELL_SCRY_TRAPS:
		sprite = 2399;
		break;
	case SPELL_SCRY_TREASURES:
		sprite = 2398;
		break;
	case SPELL_SCRY_SHRINES:
		sprite = 2397;
		break;
	case SPELL_PINPOINT:
		sprite = 2402;
		break;
	case SPELL_TURN_UNDEAD:
		sprite = 2403;
		break;
	case SPELL_SIGIL:
		sprite = 2404;
		break;
	case SPELL_SANCTUARY:
		sprite = 2405;
		break;
	case SPELL_MAGICMAPPING:
		sprite = 2408;
		break;
	case SPELL_FORGE_JEWEL:
		sprite = 2409;
		break;
	default:
		break;
	}

	if ( sprite == -1 )
	{
		return nullptr;
	}

	Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Sprite entity.
	entity->x = x;
	entity->y = y;
	entity->z = 7.5;
	entity->yaw = 0.0;
	entity->skill[0] = lifetime;
	entity->parent = caster->getUID();
	entity->actRadiusMagicID = spellID;
	entity->actRadiusMagicDist = radius;
	if ( follow )
	{
		entity->actRadiusMagicFollowUID = follow->getUID();
	}
	entity->behavior = &actRadiusMagic;
	entity->flags[PASSABLE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->flags[UPDATENEEDED] = true;
	entity->flags[INVISIBLE] = true;
	bool noupdate = !follow;
	radiusMagicSetUID(*entity, noupdate);
	return entity;
}

Entity* createMagicRadiusBadge(Entity& parent)
{
	Entity* entity = newEntity(parent.sprite, 1, map.entities, nullptr); //Particle entity.
	entity->parent = parent.getUID();
	entity->x = parent.x;
	entity->y = parent.y;
	static ConsoleVariable<float> cvar_magic_radius_badge1("/magic_radius_badge1", 4.0);
	static ConsoleVariable<float> cvar_magic_radius_badge2("/magic_radius_badge2", 0.0);
	entity->z = 4.0;
	int mapx = entity->x / 16;
	int mapy = entity->y / 16;
	if ( mapx >= 0 && mapx < map.width && mapy >= 0 && mapy < map.height )
	{
		if ( !map.tiles[(MAPLAYERS - 1) + mapy * MAPLAYERS + mapx * MAPLAYERS * map.height] )
		{
			// no ceiling
			entity->fskill[0] = *cvar_magic_radius_badge2;
		}
		else
		{
			entity->fskill[0] = *cvar_magic_radius_badge1;
		}
	}
	else
	{
		entity->fskill[0] = *cvar_magic_radius_badge2;
	}
	entity->vel_z = -0.8;
	entity->scalex = 0.1;
	entity->scaley = 0.1;
	entity->scalez = 0.1;
	entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
	entity->behavior = &actRadiusMagicBadge;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
		*cvar_magic_fx_light_bonus, 0.f);
	if ( multiplayer != CLIENT )
	{
		entity_uids--;
	}
	entity->setUID(-3);
	return entity;
}

void actRadiusMagicOnFade(Entity* my)
{
	Entity* caster = uidToEntity(my->parent);
	Entity* follow = uidToEntity(my->actRadiusMagicFollowUID);
	if ( caster )
	{
		if ( my->actRadiusMagicID == SPELL_HEAL_OTHER )
		{
			if ( follow && (follow != caster) && follow->getStats() )
			{
				messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(255, 255, 255), *follow->getStats(),
					Language::get(6927), Language::get(6926), MSG_COMBAT);
			}
		}
		else if ( my->actRadiusMagicID == SPELL_HEAL_MINOR )
		{
			if ( follow == caster )
			{
				messagePlayer(caster->isEntityPlayer(), MESSAGE_STATUS, Language::get(6931));
			}
		}
	}
}

void actRadiusMagic(Entity* my)
{
	Entity* caster = nullptr;
	if ( multiplayer != CLIENT )
	{
		caster = uidToEntity(my->parent);
	}
	if ( PARTICLE_LIFE < 0 || (!caster && multiplayer != CLIENT) )
	{
		my->removeLightField();
		list_RemoveNode(my->mynode);
		return;
	}

	if ( multiplayer != CLIENT )
	{
		if ( my->actRadiusMagicID == SPELL_HEAL_MINOR || my->actRadiusMagicID == SPELL_HEAL_OTHER )
		{
			if ( caster && caster->getStats() )
			{
				if ( !caster->getStats()->getEffectActive(EFF_HEALING_WORD) )
				{
					messagePlayerColor(caster->isEntityPlayer(), MESSAGE_STATUS, makeColorRGB(255, 0, 0), Language::get(6933));
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
			}
		}

		if ( my->actRadiusMagicFollowUID != 0 )
		{
			Entity* follow = uidToEntity(my->actRadiusMagicFollowUID);
			if ( !follow )
			{
				my->removeLightField();
				list_RemoveNode(my->mynode);
				return;
			}
			else
			{
				my->x = follow->x;
				my->y = follow->y;
			}
		}

		//if ( my->actRadiusMagicID == SPELL_FOCI_DARK_LIFE
		//	|| my->actRadiusMagicID == SPELL_FOCI_DARK_RIFT
		//	|| my->actRadiusMagicID == SPELL_FOCI_DARK_SILENCE 
		//	|| my->actRadiusMagicID == SPELL_FOCI_DARK_SUPPRESS 
		//	|| my->actRadiusMagicID == SPELL_FOCI_DARK_VENGEANCE )
		//{
		//	bool endEffect = false;
		//	if ( Stat* casterStats = caster->getStats() )
		//	{
		//		if ( !casterStats->defending )
		//		{
		//			endEffect = true;

		//			Entity* spellEntity = createParticleSapCenter(caster, my, 0, 2178, 2178);
		//			if ( spellEntity )
		//			{
		//				spellEntity->skill[0] = 25; // duration
		//				spellEntity->skill[7] = my->getUID();
		//			}

		//			my->removeLightField();
		//			list_RemoveNode(my->mynode);
		//			return;
		//		}
		//	}
		//}
	}
	else
	{
		if ( my->actRadiusMagicFollowUID != 0 )
		{
			if ( Entity* follow = uidToEntity(my->actRadiusMagicFollowUID) )
			{
				my->x = follow->x;
				my->y = follow->y;
				my->new_x = my->x; // skip dead reckoning interpolation since we know the follow target
				my->new_y = my->y;
			}
		}
	}

	my->removeLightField();
	my->light = addLight(my->x / 16, my->y / 16, magicLightColorForSprite(my, my->sprite, false));

	my->flags[INVISIBLE] = true;

	bool refireLoop = true;
	if ( my->actRadiusMagicID == SPELL_FOCI_DARK_LIFE
		|| my->actRadiusMagicID == SPELL_FOCI_DARK_RIFT
		|| my->actRadiusMagicID == SPELL_FOCI_DARK_SILENCE
		|| my->actRadiusMagicID == SPELL_FOCI_DARK_SUPPRESS
		|| my->actRadiusMagicID == SPELL_FOCI_DARK_VENGEANCE )
	{
		refireLoop = false;
	}

	if ( my->actRadiusMagicInit == 0 
		|| (refireLoop && ((ticks - my->actRadiusMagicDoPulseTick) == 1)
			|| (my->actRadiusMagicAutoPulseTick > 0 && my->actRadiusMagicDoPulseTick > 0 &&
			(ticks - my->actRadiusMagicDoPulseTick) % my->actRadiusMagicAutoPulseTick == 1)) )
	{
		if ( Entity* fx = createParticleAOEIndicator(my, my->x, my->y, 0.0, TICKS_PER_SECOND * 99, my->actRadiusMagicDist) )
		{
			//fx->actSpriteCheckParentExists = 0;
			if ( auto indicator = AOEIndicators_t::getIndicator(fx->skill[10]) )
			{
				Uint32 color = 0xFFFFFFFF;
				switch ( my->sprite )
				{
				case 1817:
					color = makeColorRGB(195, 48, 165);
					break;
				case 1818:
					color = makeColorRGB(124, 107, 209);
					break;
				case 1686:
				case 2384:
					color = makeColorRGB(252, 107, 35);
					break;
				case 2179:
				case 2180:
				case 2181:
				case 2182:
				case 2183:
					color = makeColorRGB(132, 47, 241);
					break;
				case 2390:
				case 2391:
				case 2392:
				case 2393:
					color = makeColorRGB(102, 117, 204);
					break;
				case 2404:
				case 2409:
					color = makeColorRGB(252, 208, 88);
					break;
				case 2405:
					color = makeColorRGB(104, 188, 252);
					break;
				default:
					break;
				}
				Uint8 r, g, b, a;
				getColor(color, &r, &g, &b, &a);
				a *= 0.8;
				indicator->indicatorColor = makeColor(r, g, b, a);
				indicator->loop = true;
				indicator->framesPerTick = 1;
				indicator->ticksPerUpdate = 1;
				indicator->gradient = 4;
				indicator->delayTicks = 0;
				indicator->radiusMin = 4;
				indicator->radius = 4;
				indicator->loopType = 1;
				indicator->loopTimer = 50;
				indicator->cacheType = AOEIndicators_t::CACHE_RADIUS_MAGIC_GENERIC;

				if ( my->actRadiusMagicID == SPELL_HEAL_PULSE )
				{
					indicator->loopTimer = 3.5 * TICKS_PER_SECOND;
				}
				else if ( my->actRadiusMagicID == SPELL_HEAL_MINOR || my->actRadiusMagicID == SPELL_HEAL_OTHER )
				{
					//indicator->loopTimer = 3.5 * TICKS_PER_SECOND;
					fx->scalex = 0.8;
					fx->scaley = 0.8;
				}
				else if ( my->actRadiusMagicID == SPELL_FOCI_DARK_LIFE
					|| my->actRadiusMagicID == SPELL_FOCI_DARK_RIFT
					|| my->actRadiusMagicID == SPELL_FOCI_DARK_SILENCE
					|| my->actRadiusMagicID == SPELL_FOCI_DARK_SUPPRESS
					|| my->actRadiusMagicID == SPELL_FOCI_DARK_VENGEANCE )
				{
					indicator->loop = false;
					//indicator->lifetime = 100;
					//indicator->loopType = 0;
					//indicator->loopTimer = 0;
				}
				else if ( my->actRadiusMagicID == SPELL_FOCI_LIGHT_PEACE
					|| my->actRadiusMagicID == SPELL_FOCI_LIGHT_JUSTICE
					|| my->actRadiusMagicID == SPELL_FOCI_LIGHT_PROVIDENCE
					|| my->actRadiusMagicID == SPELL_FOCI_LIGHT_PURITY
					|| my->actRadiusMagicID == SPELL_FOCI_LIGHT_SANCTUARY )
				{
					indicator->loop = false;
					indicator->loopType = 0;
					indicator->loopTimer = 0;
				}
				else if (
					my->actRadiusMagicID == SPELL_DIVINE_ZEAL
					|| my->actRadiusMagicID == SPELL_PROF_COUNSEL
					|| my->actRadiusMagicID == SPELL_PROF_GREATER_MIGHT
					|| my->actRadiusMagicID == SPELL_PROF_NIMBLENESS
					|| my->actRadiusMagicID == SPELL_PROF_STURDINESS )
				{
					fx->scalex = 0.8;
					fx->scaley = 0.8;
					indicator->loop = false;
					indicator->loopType = 0;
					indicator->loopTimer = 0;
				}
				else if (
					my->actRadiusMagicID == SPELL_COMMAND
					|| my->actRadiusMagicID == SPELL_SEEK_ALLY
					|| my->actRadiusMagicID == SPELL_SEEK_FOE
					|| my->actRadiusMagicID == SPELL_TABOO
					|| my->actRadiusMagicID == SPELL_DETECT_ENEMY
					|| my->actRadiusMagicID == SPELL_SCRY_ALLIES
					|| my->actRadiusMagicID == SPELL_DONATION
					|| my->actRadiusMagicID == SPELL_BLESS_FOOD
					|| my->actRadiusMagicID == SPELL_SCRY_TRAPS
					|| my->actRadiusMagicID == SPELL_SCRY_TREASURES
					|| my->actRadiusMagicID == SPELL_SCRY_SHRINES
					|| my->actRadiusMagicID == SPELL_PINPOINT
					|| my->actRadiusMagicID == SPELL_MAGICMAPPING
					|| my->actRadiusMagicID == SPELL_FORGE_JEWEL
						)
				{
					fx->scalex = 0.8;
					fx->scaley = 0.8;
					indicator->loop = false;
					indicator->loopType = 0;
					indicator->loopTimer = 0;
				}
			}
		}

		if ( my->actRadiusMagicID == SPELL_FOCI_DARK_LIFE
			|| my->actRadiusMagicID == SPELL_FOCI_DARK_RIFT
			|| my->actRadiusMagicID == SPELL_FOCI_DARK_SILENCE
			|| my->actRadiusMagicID == SPELL_FOCI_DARK_SUPPRESS
			|| my->actRadiusMagicID == SPELL_FOCI_DARK_VENGEANCE )
		{
			createParticleFociDark(my, my->actRadiusMagicID, false);
		}
		spawnMagicEffectParticles(my->x, my->y, my->z, my->sprite);
		if ( my->actRadiusMagicInit == 0 )
		{
			createMagicRadiusBadge(*my);

			if ( my->actRadiusMagicID == SPELL_TURN_UNDEAD
				|| my->actRadiusMagicID == SPELL_SIGIL
				|| my->actRadiusMagicID == SPELL_SANCTUARY )
			{
				createParticleFociLight(my, my->actRadiusMagicID, false);
			}
		}
		my->actRadiusMagicInit = 1;
	}

	my->vel_x = 0.0;
	my->vel_y = 0.0;

	bool checkArea = true;
	if ( my->actRadiusMagicID == SPELL_HEAL_PULSE )
	{
		checkArea = false;
		if ( my->ticks % (4 * TICKS_PER_SECOND) == 1 )
		{
			checkArea = true;
		}
	}
	else if ( my->actRadiusMagicID == SPELL_HEAL_MINOR || my->actRadiusMagicID == SPELL_HEAL_OTHER )
	{
		checkArea = false;
		int interval = getSpellEffectDurationSecondaryFromID(my->actRadiusMagicID, caster, nullptr, caster);
		if ( my->ticks % (interval) == 1 )
		{
			checkArea = true;
		}
	}
	else if ( my->actRadiusMagicID == SPELL_FOCI_DARK_LIFE
		|| my->actRadiusMagicID == SPELL_FOCI_DARK_RIFT
		|| my->actRadiusMagicID == SPELL_FOCI_DARK_SILENCE
		|| my->actRadiusMagicID == SPELL_FOCI_DARK_SUPPRESS
		|| my->actRadiusMagicID == SPELL_FOCI_DARK_VENGEANCE )
	{
		checkArea = false;
		if ( my->ticks == 1 || (ticks - my->actRadiusMagicDoPulseTick) == 1 )
		{
			checkArea = true;
		}
		else if ( my->actRadiusMagicAutoPulseTick > 0 && my->actRadiusMagicDoPulseTick > 0 &&
			(ticks - my->actRadiusMagicDoPulseTick) % my->actRadiusMagicAutoPulseTick == 1 )
		{
			checkArea = true;
		}

		//if ( my->actRadiusMagicFollowUID == 0 )
		//{
		//	real_t prevDist = 10000.0;
		//	Entity* target = nullptr;
		//	/*if ( my->actmagicOrbitHitTargetUID4 != 0 )
		//	{
		//		target = uidToEntity(my->actmagicOrbitHitTargetUID4);
		//	}*/
		//	if ( !target )
		//	{
		//		for ( node_t* node = map.creatures->first; node; node = node->next )
		//		{
		//			if ( Entity* entity = (Entity*)node->element )
		//			{
		//				if ( Stat* entityStats = entity->getStats() )
		//				{
		//					if ( caster->checkEnemy(entity) && entity->monsterIsTargetable() )
		//					{
		//						real_t tangent = atan2(entity->y - my->y, entity->x - my->x);
		//						real_t targetdist = sqrt(pow(my->x - entity->x, 2) + pow(my->y - entity->y, 2));
		//						real_t dist = lineTraceTarget(my, my->x, my->y, tangent, 64.0, 0, true, entity);
		//						if ( hit.entity == entity && dist < prevDist )
		//						{
		//							prevDist = dist;
		//							target = entity;
		//						}
		//					}
		//				}
		//			}
		//		}
		//	}

		//	if ( target )
		//	{
		//		my->actRadiusMagicFollowUID = target->getUID();
		//		serverUpdateEntitySkill(my, 5); // update follow UID
		//		//real_t dist = entityDist(my, target);
		//		//if ( dist < 8.0 )
		//		//{
		//		//	my->actRadiusMagicFollowUID = target->getUID();
		//		//	serverUpdateEntitySkill(my, 5); // update follow UID
		//		//}
		//		//else
		//		//{
		//		//	real_t tangent = atan2(target->y - my->y, target->x - my->x);
		//		//	real_t speed = std::min(dist, std::max(16.0, 64.0 - dist) / 100.0);
		//		//	my->vel_x = speed * cos(tangent);
		//		//	my->vel_y = speed * sin(tangent);
		//		//}

		//		//my->x += my->vel_x;
		//		//my->y += my->vel_y;
		//	}
		//}
	}
	else if ( my->actRadiusMagicID == SPELL_FOCI_LIGHT_PEACE
		|| my->actRadiusMagicID == SPELL_FOCI_LIGHT_JUSTICE
		|| my->actRadiusMagicID == SPELL_FOCI_LIGHT_PROVIDENCE
		|| my->actRadiusMagicID == SPELL_FOCI_LIGHT_PURITY
		|| my->actRadiusMagicID == SPELL_FOCI_LIGHT_SANCTUARY
		|| my->actRadiusMagicID == SPELL_DIVINE_ZEAL
		|| my->actRadiusMagicID == SPELL_PROF_COUNSEL
		|| my->actRadiusMagicID == SPELL_PROF_GREATER_MIGHT
		|| my->actRadiusMagicID == SPELL_PROF_NIMBLENESS
		|| my->actRadiusMagicID == SPELL_PROF_STURDINESS
		|| my->actRadiusMagicID == SPELL_COMMAND
		|| my->actRadiusMagicID == SPELL_SEEK_ALLY
		|| my->actRadiusMagicID == SPELL_SEEK_FOE
		|| my->actRadiusMagicID == SPELL_TABOO
		|| my->actRadiusMagicID == SPELL_DETECT_ENEMY
		|| my->actRadiusMagicID == SPELL_SCRY_ALLIES
		|| my->actRadiusMagicID == SPELL_DONATION
		|| my->actRadiusMagicID == SPELL_BLESS_FOOD
		|| my->actRadiusMagicID == SPELL_SCRY_TRAPS
		|| my->actRadiusMagicID == SPELL_SCRY_TREASURES
		|| my->actRadiusMagicID == SPELL_SCRY_SHRINES
		|| my->actRadiusMagicID == SPELL_PINPOINT
		|| my->actRadiusMagicID == SPELL_MAGICMAPPING
		|| my->actRadiusMagicID == SPELL_FORGE_JEWEL
		)
	{
		checkArea = false; // visual effect only
	}

	if ( multiplayer == SERVER )
	{
		if ( my->ticks >= *cvar_entity_bodypart_sync_tick )
		{
			if ( (my->ticks - *cvar_entity_bodypart_sync_tick) % (2 * TICKS_PER_SECOND) == 0 )
			{
				if ( my->actRadiusMagicFollowUID != 0 )
				{
					serverUpdateEntitySkill(my, 5); // update follow UID
				}
			}
		}
	}

	if ( multiplayer != CLIENT )
	{
		--PARTICLE_LIFE;
		if ( PARTICLE_LIFE == -1 )
		{
			actRadiusMagicOnFade(my);
		}

		if ( checkArea )
		{
			auto entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 1 + my->actRadiusMagicDist / 16);
			std::vector<Entity*> applyEffects;
			for ( auto it : entLists )
			{
				node_t* node;
				for ( node = it->first; node != nullptr; node = node->next )
				{
					if ( Entity* entity = (Entity*)node->element )
					{
						if ( my->actRadiusMagicID == SPELL_HEAL_PULSE )
						{
							if ( entity->behavior == &actPlayer || (entity->behavior == &actMonster && !entity->isInertMimic()) )
							{
								if ( entity->monsterIsTargetable() )
								{
									if ( entityDist(my, entity) <= (real_t)my->actRadiusMagicDist + 4.0 )
									{
										if ( caster && caster->checkFriend(entity) )
										{
											auto props = getParticleEmitterHitProps(my->getUID(), entity);
											if ( !props )
											{
												continue;
											}
											/*if ( props->hits > 5 )
											{
												continue;
											}*/
											props->hits++;
											applyEffects.push_back(entity);
										}
									}
								}
							}
						}
						else if ( my->actRadiusMagicID == SPELL_HEAL_MINOR || my->actRadiusMagicID == SPELL_HEAL_OTHER )
						{
							if ( entity->behavior == &actPlayer || (entity->behavior == &actMonster && !entity->isInertMimic()) )
							{
								if ( my->actRadiusMagicFollowUID == entity->getUID() )
								{
									auto props = getParticleEmitterHitProps(my->getUID(), entity);
									if ( !props )
									{
										continue;
									}
									props->hits++;
									applyEffects.push_back(entity);
								}
							}
						}
						else if ( my->actRadiusMagicID == SPELL_NULL_AREA )
						{
							if ( entity->behavior == &actMagicMissile )
							{
								if ( entityDist(my, entity) <= (real_t)my->actRadiusMagicDist + 4.0 )
								{
									auto props = getParticleEmitterHitProps(my->getUID(), entity);
									if ( !props )
									{
										continue;
									}
									if ( props->hits > 0 )
									{
										continue;
									}
									props->hits++;
									applyEffects.push_back(entity);
								}
							}
						}
						else if ( my->actRadiusMagicID == SPELL_SPHERE_SILENCE )
						{
							if ( entity->behavior == &actMagicMissile )
							{
								if ( entityDist(my, entity) <= (real_t)my->actRadiusMagicDist + 4.0 )
								{
									auto props = getParticleEmitterHitProps(my->getUID(), entity);
									if ( !props )
									{
										continue;
									}
									if ( props->hits > 0 )
									{
										continue;
									}
									props->hits++;
									Entity* parent = uidToEntity(entity->parent);
									if ( parent && entityDist(my, parent) < (real_t)my->actRadiusMagicDist + 4.0 )
									{
										applyEffects.push_back(entity);
									}
								}
							}
						}
						else if ( my->actRadiusMagicID == SPELL_SANCTUARY )
						{
							if ( entity->behavior == &actPlayer || (entity->behavior == &actMonster && !entity->isInertMimic()) )
							{
								if ( entity->monsterIsTargetable() )
								{
									if ( entityDist(my, entity) <= (real_t)my->actRadiusMagicDist + 4.0 )
									{
										if ( (caster && caster->checkFriend(entity)) )
										{
											/*if ( entity->getStats() && entity->getStats()->getEffectActive(EFF_SANCTUARY)
												&& entity->getStats()->EFFECTS_TIMERS[EFF_SANCTUARY] > TICKS_PER_SECOND )
											{
												continue;
											}*/

											auto props = getParticleEmitterHitProps(my->getUID(), entity);
											if ( !props )
											{
												continue;
											}
											if ( props->hits > 0 && (ticks - props->tick) < TICKS_PER_SECOND / 2 )
											{
												continue;
											}
											props->hits++;
											props->tick = ticks;
											applyEffects.push_back(entity);
										}
									}
								}
							}
						}
						else if ( my->actRadiusMagicID == SPELL_SIGIL )
						{
							if ( entity->behavior == &actPlayer || (entity->behavior == &actMonster && !entity->isInertMimic()) )
							{
								if ( entity->monsterIsTargetable() )
								{
									if ( entityDist(my, entity) <= (real_t)my->actRadiusMagicDist + 4.0 )
									{
										/*if ( caster && caster != entity && caster->checkEnemy(entity) )
										{
											real_t tangent = atan2(entity->y - my->y, entity->x - my->x);
											real_t dist = lineTraceTarget(my, my->x, my->y, tangent, my->actRadiusMagicDist, 0, false, entity);
											if ( hit.entity == entity )
											{
												auto props = getParticleEmitterHitProps(my->getUID(), entity);
												if ( !props )
												{
													continue;
												}
												if ( props->hits > 0 && (ticks - props->tick) < TICKS_PER_SECOND / 2 )
												{
													continue;
												}
												props->hits++;
												props->tick = ticks;
												applyEffects.push_back(entity);
											}
										}
										else*/
										{
											/*if ( entity->getStats() && entity->getStats()->getEffectActive(EFF_SIGIL)
												&& entity->getStats()->EFFECTS_TIMERS[EFF_SIGIL] > TICKS_PER_SECOND )
											{
												continue;
											}*/

											auto props = getParticleEmitterHitProps(my->getUID(), entity);
											if ( !props )
											{
												continue;
											}
											if ( props->hits > 0 && (ticks - props->tick) < TICKS_PER_SECOND / 2 )
											{
												continue;
											}


											props->hits++;
											props->tick = ticks;
											applyEffects.push_back(entity);
										}
									}
								}
							}
						}
						else if ( my->actRadiusMagicID == SPELL_TURN_UNDEAD )
						{
							if ( entity->behavior == &actPlayer || (entity->behavior == &actMonster && !entity->isInertMimic()) )
							{
								if ( entity->monsterIsTargetable() && entity->isSmiteWeakMonster() )
								{
									if ( entityDist(my, entity) <= (real_t)my->actRadiusMagicDist + 4.0 )
									{
										if ( caster && caster != entity && caster->checkEnemy(entity) )
										{
											real_t tangent = atan2(entity->y - my->y, entity->x - my->x);
											real_t dist = lineTraceTarget(my, my->x, my->y, tangent, my->actRadiusMagicDist + 4.0, 0, false, entity);
											if ( hit.entity == entity )
											{
												auto props = getParticleEmitterHitProps(my->getUID(), entity);
												if ( !props )
												{
													continue;
												}
												if ( props->hits > 0 )
												{
													continue;
												}
												props->hits++;
												applyEffects.push_back(entity);
											}
										}
									}
								}
							}
						}
						else if ( my->actRadiusMagicID == SPELL_FOCI_DARK_RIFT
							|| my->actRadiusMagicID == SPELL_FOCI_DARK_SUPPRESS )
						{
							if ( entity->behavior == &actPlayer || (entity->behavior == &actMonster && !entity->isInertMimic()) )
							{
								if ( entity->monsterIsTargetable() )
								{
									if ( entityDist(my, entity) <= (real_t)my->actRadiusMagicDist + 4.0 )
									{
										if ( caster && caster != entity && caster->checkEnemy(entity) )
										{
											real_t tangent = atan2(entity->y - my->y, entity->x - my->x);
											real_t dist = lineTraceTarget(my, my->x, my->y, tangent, my->actRadiusMagicDist + 4.0, 0, false, entity);
											if ( hit.entity == entity )
											{
												auto props = getParticleEmitterHitProps(my->getUID(), entity);
												if ( !props )
												{
													continue;
												}
												props->hits++;
												applyEffects.push_back(entity);
											}
										}
									}
								}
							}
						}
						else
						{
							/*if ( entity->behavior == &actPlayer || (entity->behavior == &actMonster && !entity->isInertMimic()) )
							{
								if ( entity->monsterIsTargetable() )
								{
									if ( entityDist(my, entity) <= (real_t)my->actRadiusMagicDist )
									{
										if ( caster->checkFriend(entity) || caster == entity )
										{
											entity->setEffect(EFF_SLOW, true, 15, false);
										}
									}
								}
							}*/
						}
					}
				}
			}

			/*if ( my->actRadiusMagicID == SPELL_FOCI_DARK_LIFE )
			{
				while ( applyEffects.size() > 1 )
				{
					unsigned int pick = local_rng.rand() % applyEffects.size();
					applyEffects.erase(applyEffects.begin() + pick);
				}
			}*/

			bool firstEffect = true;
			int totalHeal = 0;
			for ( auto ent : applyEffects )
			{
				if ( my->actRadiusMagicID == SPELL_HEAL_PULSE )
				{
					int amount = getSpellDamageFromID(SPELL_HEAL_PULSE, caster, nullptr, my);
					if ( firstEffect )
					{
						playSoundEntity(my, 168, 128);
						spawnMagicEffectParticles(my->x, my->y, my->z, 169);
					}

					int oldHP = ent->getHP();
					spell_changeHealth(ent, amount);
					int heal = std::max(ent->getHP() - oldHP, 0);
					totalHeal += heal;
					if ( heal > 0 )
					{
						spawnDamageGib(ent, -heal, DamageGib::DMG_HEAL, DamageGibDisplayType::DMG_GIB_NUMBER, true);
						if ( caster )
						{
							magicOnSpellCastEvent(caster, caster, uidToEntity(ent->parent), my->actRadiusMagicID,
								spell_t::SPELL_LEVEL_EVENT_DEFAULT | spell_t::SPELL_LEVEL_EVENT_MINOR_CHANCE, 1);
						}
					}
					spawnMagicEffectParticles(ent->x, ent->y, ent->z, 169);
					firstEffect = false;
				}
				else if ( my->actRadiusMagicID == SPELL_HEAL_MINOR || my->actRadiusMagicID == SPELL_HEAL_OTHER )
				{
					int amount = getSpellDamageFromID(my->actRadiusMagicID, caster, nullptr, my);
					/*if ( firstEffect )
					{
						playSoundEntity(my, 168, 128);
						spawnMagicEffectParticles(my->x, my->y, my->z, 169);
					}*/

					int oldHP = ent->getHP();
					spell_changeHealth(ent, amount, false, false);
					int heal = std::max(ent->getHP() - oldHP, 0);
					totalHeal += heal;
					if ( heal > 0 )
					{
						spawnDamageGib(ent, -heal, DamageGib::DMG_HEAL, DamageGibDisplayType::DMG_GIB_NUMBER, true);
						if ( caster )
						{
							magicOnSpellCastEvent(caster, caster, uidToEntity(ent->parent), my->actRadiusMagicID,
								spell_t::SPELL_LEVEL_EVENT_DEFAULT | spell_t::SPELL_LEVEL_EVENT_MINOR_CHANCE, 1);
						}
					}
					//spawnMagicEffectParticles(ent->x, ent->y, ent->z, 169);
					firstEffect = false;
				}
				else if ( my->actRadiusMagicID == SPELL_NULL_AREA || my->actRadiusMagicID == SPELL_SPHERE_SILENCE )
				{
					Entity* fx = createParticleAestheticOrbit(my, my->sprite, TICKS_PER_SECOND / 4, PARTICLE_EFFECT_NULL_PARTICLE);
					fx->x = ent->x;
					fx->y = ent->y;
					fx->z = ent->z;
					real_t tangent = atan2(ent->y - my->y, ent->x - my->x);
					fx->yaw = tangent;
					fx->actmagicOrbitDist = 0;
					fx->actmagicNoLight = 0;
					serverSpawnMiscParticlesAtLocation(fx->x, fx->y, fx->z, PARTICLE_EFFECT_NULL_PARTICLE, my->sprite, 0, fx->yaw * 256.0);

					if ( caster )
					{
						magicOnSpellCastEvent(caster, caster, uidToEntity(ent->parent), my->actRadiusMagicID, 
							spell_t::SPELL_LEVEL_EVENT_DEFAULT | spell_t::SPELL_LEVEL_EVENT_MINOR_CHANCE, 1);
					}

					ent->removeLightField();
					list_RemoveNode(ent->mynode);
					firstEffect = false;
				}
				else if ( my->actRadiusMagicID == SPELL_SIGIL )
				{
					int effectDuration = getSpellEffectDurationSecondaryFromID(SPELL_SIGIL, caster, nullptr, my);
					Uint8 effectStrength = std::min(getSpellDamageSecondaryFromID(SPELL_SIGIL, caster, nullptr, my),
						std::max(1, getSpellDamageFromID(SPELL_SIGIL, caster, nullptr, my)));

					if ( caster && caster->behavior == &actPlayer )
					{
						effectStrength |= ((caster->skill[2] + 1) << 4);
					}
					else
					{
						effectStrength |= ((MAXPLAYERS + 1) << 4);
					}

					if ( Stat* entitystats = ent->getStats() )
					{
						if ( !entitystats->getEffectActive(EFF_SIGIL) )
						{
							if ( ent->setEffect(EFF_SIGIL, effectStrength, effectDuration, false, true, true) )
							{
								spawnMagicEffectParticles(ent->x, ent->y, ent->z, 174);
								firstEffect = false;

								/*if ( Entity* fx = createMagicRadiusBadge(*my) )
								{
									fx->actRadiusMagicFollowUID = ent->getUID();
								}*/
							}
						}
						else
						{
							if ( entitystats->EFFECTS_TIMERS[EFF_SIGIL] < TICKS_PER_SECOND )
							{
								spawnMagicEffectParticles(ent->x, ent->y, ent->z, 174);
							}
							entitystats->setEffectActive(EFF_SIGIL, effectStrength);
							entitystats->EFFECTS_TIMERS[EFF_SIGIL] = effectDuration;
							firstEffect = false;
						}
					}
				}
				else if ( my->actRadiusMagicID == SPELL_SANCTUARY )
				{
					int effectDuration = getSpellEffectDurationSecondaryFromID(SPELL_SANCTUARY, caster, nullptr, my);
					Uint8 effectStrength = std::min(getSpellDamageSecondaryFromID(SPELL_SANCTUARY, caster, nullptr, my),
						std::max(1, getSpellDamageFromID(SPELL_SANCTUARY, caster, nullptr, my)));

					if ( caster && caster->behavior == &actPlayer )
					{
						effectStrength |= ((caster->skill[2] + 1) << 4);
					}
					else
					{
						effectStrength |= ((MAXPLAYERS + 1) << 4);
					}

					if ( Stat* entitystats = ent->getStats() )
					{
						if ( !entitystats->getEffectActive(EFF_SANCTUARY) )
						{
							if ( ent->setEffect(EFF_SANCTUARY, effectStrength, effectDuration, false, true, true) )
							{
								spawnMagicEffectParticles(ent->x, ent->y, ent->z, 174);
								firstEffect = false;

								/*if ( Entity* fx = createMagicRadiusBadge(*my) )
								{
									fx->actRadiusMagicFollowUID = ent->getUID();
								}*/
							}
						}
						else
						{
							if ( entitystats->EFFECTS_TIMERS[EFF_SANCTUARY] < TICKS_PER_SECOND )
							{
								spawnMagicEffectParticles(ent->x, ent->y, ent->z, 174);
							}
							entitystats->setEffectActive(EFF_SANCTUARY, effectStrength);
							entitystats->EFFECTS_TIMERS[EFF_SANCTUARY] = effectDuration;
							firstEffect = false;
						}
					}
				}
				else if ( my->actRadiusMagicID == SPELL_TURN_UNDEAD )
				{
					int effectDuration = getSpellEffectDurationSecondaryFromID(SPELL_TURN_UNDEAD, caster, nullptr, my);
					int effectStrength = getSpellDamageSecondaryFromID(SPELL_TURN_UNDEAD, caster, nullptr, my);
					if ( effectStrength >= 2 )
					{
						effectDuration *= 2;
					}

					if ( effectStrength >= 2 )
					{
						if ( Entity* fx1 = createParticleAestheticOrbit(ent, 2401, 3 * TICKS_PER_SECOND, PARTICLE_EFFECT_TURN_UNDEAD) )
						{
							fx1->yaw = ent->yaw;
							fx1->fskill[4] = ent->x;
							fx1->fskill[5] = ent->y;
							fx1->x = ent->x;
							fx1->y = ent->y;
							fx1->fskill[6] = fx1->yaw;
							fx1->skill[3] = caster ? caster->getUID() : 0;
							if ( effectStrength >= 3 )
							{
								fx1->skill[6] = EFF_HOLY_FIRE;
							}
						}

						serverSpawnMiscParticles(ent, PARTICLE_EFFECT_TURN_UNDEAD, 2401);

						if ( firstEffect )
						{
							if ( caster )
							{
								messagePlayerColor(caster->isEntityPlayer(),
									MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6497));
							}
						}
						firstEffect = false;
					}
					if ( ent->setEffect(EFF_FEAR, true, effectDuration, true)
						|| (effectStrength >= 2 && ent->setEffect(EFF_PARALYZED, true, effectDuration / 2, false)) )
					{
						playSoundEntity(ent, 687, 128); // fear.ogg
						spawnMagicEffectParticles(ent->x, ent->y, ent->z, 174);

						ent->monsterAcquireAttackTarget(*my, MONSTER_STATE_WAIT);
						ent->monsterFearfulOfUid = my->getUID();

						if ( firstEffect )
						{
							if ( caster )
							{
								messagePlayerColor(caster->isEntityPlayer(),
									MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6497));
							}
						}
						firstEffect = false;
					}

					if ( !firstEffect )
					{
						if ( caster )
						{
							if ( caster->behavior == &actPlayer )
							{
								players[caster->skill[2]]->mechanics.updateSustainedSpellEvent(SPELL_TURN_UNDEAD, 100.0, 1.0);
							}
						}
					}
				}
				else if ( my->actRadiusMagicID == SPELL_FOCI_DARK_RIFT
					|| my->actRadiusMagicID == SPELL_FOCI_DARK_SUPPRESS )
				{
					Entity* spellEntity = createParticleSapCenter(ent, my, 0, 2178, 2178);
					if ( spellEntity )
					{
						/*spellEntity->x = ent->x;
						spellEntity->y = ent->y;*/
						spellEntity->skill[0] = 25; // duration
						spellEntity->skill[7] = my->getUID();
					}
					int effectID = EFF_STASIS;
					if ( my->actRadiusMagicID == SPELL_FOCI_DARK_SUPPRESS )
					{
						effectID = EFF_ROOTED;
					}
					if ( Stat* entStats = ent->getStats() )
					{
						int duration = my->actRadiusMagicEffectPower;
						if ( Uint8 effectStrength = entStats->getEffectActive(effectID) )
						{
							duration = std::max(entStats->EFFECTS_TIMERS[effectID], duration);
							duration = std::min(duration, getSpellEffectDurationSecondaryFromID(my->actRadiusMagicID, caster, nullptr, caster));
						}
						if ( duration > 0 )
						{
							bool updateClients = effectID == EFF_STASIS;
							if ( ent->setEffect(effectID, true, duration, updateClients) )
							{
								if ( my->actRadiusMagicID == SPELL_FOCI_DARK_SUPPRESS )
								{
									if ( Entity* spellTimer = createParticleTimer(caster, duration, -1) )
									{
										spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_ROOTS_SINGLE_TILE_VOID;
										spellTimer->particleTimerCountdownSprite = -1;
										spellTimer->x = ent->x;
										spellTimer->y = ent->y;
									}
								}

								if ( firstEffect )
								{
									if ( caster )
									{
										if ( my->actRadiusMagicID == SPELL_FOCI_DARK_SUPPRESS )
										{
											messagePlayerColor(caster->isEntityPlayer(), MESSAGE_COMBAT, makeColorRGB(0, 255, 0), Language::get(6841));
										}
										else
										{
											messagePlayerColor(caster->isEntityPlayer(), MESSAGE_COMBAT, makeColorRGB(0, 255, 0), Language::get(6840));
										}
									}
								}
								firstEffect = false;
							}
						}
					}
				}
			}

			if ( totalHeal > 0 )
			{
				int player = caster ? caster->isEntityPlayer() : -1;
				serverUpdatePlayerGameplayStats(player, STATISTICS_HEAL_BOT, totalHeal);
				Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_SPELL_HEAL, SPELL_ITEM, totalHeal, false, my->actRadiusMagicID);
			}
		}
	}
}

Entity* createSpellExplosionArea(int spellID, Entity* caster, real_t x, real_t y, real_t z, real_t radius, int damage, Entity* ohitentity)
{
	Entity* spellTimer = nullptr;
	if ( multiplayer != CLIENT )
	{
		spellTimer = createParticleTimer(caster, 5, -1);
		spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_DAMAGE_LOS_AREA;
		spellTimer->particleTimerCountdownSprite = -1;

		if ( damage == 0 )
		{
			damage = getSpellDamageFromID(spellID, caster, nullptr, caster);
		}

		spellTimer->particleTimerVariable1 = damage; // damage
		spellTimer->particleTimerVariable2 = spellID;
		spellTimer->particleTimerVariable3 = radius + 4.0;
		spellTimer->particleTimerVariable4 = ohitentity ? ohitentity->getUID() : 0;
		spellTimer->yaw = 0.0;
		spellTimer->x = x;
		spellTimer->y = y;
		spellTimer->z = z;
	}

	if ( spellID == SPELL_EARTH_ELEMENTAL )
	{
		spellTimer->particleTimerDuration = 2;

		if ( multiplayer != CLIENT )
		{
			Uint32 color = makeColorRGB(112, 104, 96);
			if ( multiplayer == SERVER )
			{
				serverSpawnMiscParticlesAtLocation(x, y, z, PARTICLE_EFFECT_EARTH_ELEMENTAL_SUMMON_AOE, 0, radius, color);
			}
			if ( Entity* fx = createParticleAOEIndicator(spellTimer, x, y, 0.0, TICKS_PER_SECOND, radius) )
			{
				fx->actSpriteFollowUID = 0;
				fx->actSpriteCheckParentExists = 0;
				if ( auto indicator = AOEIndicators_t::getIndicator(fx->skill[10]) )
				{
					indicator->indicatorColor = color;
					indicator->loop = false;
					indicator->gradient = 4;
					indicator->framesPerTick = 2;
					indicator->ticksPerUpdate = 1;
					indicator->delayTicks = 0;
				}
			}
		}
		return spellTimer;
	}
	if ( spellID == SPELL_PROJECT_SPIRIT )
	{
		spellTimer->particleTimerDuration = 2;

		if ( multiplayer != CLIENT )
		{
			Uint32 color = makeColorRGB(112, 104, 96);
			if ( multiplayer == SERVER )
			{
				serverSpawnMiscParticlesAtLocation(x, y, z, PARTICLE_EFFECT_EARTH_ELEMENTAL_SUMMON_AOE, 0, radius, color);
			}
			if ( Entity* fx = createParticleAOEIndicator(spellTimer, x, y, 0.0, TICKS_PER_SECOND, radius) )
			{
				fx->actSpriteFollowUID = 0;
				fx->actSpriteCheckParentExists = 0;
				if ( auto indicator = AOEIndicators_t::getIndicator(fx->skill[10]) )
				{
					indicator->indicatorColor = color;
					indicator->loop = false;
					indicator->gradient = 4;
					indicator->framesPerTick = 2;
					indicator->ticksPerUpdate = 1;
					indicator->delayTicks = 0;
					indicator->expireAlphaRate = 0.975;
				}
			}
		}
		return spellTimer;
	}
	if ( spellID == SPELL_ETERNALS_GAZE )
	{
		spawnExplosionFromSprite(135,
			spellTimer->x - 2.0 + local_rng.rand() % 5,
			spellTimer->y - 2.0 + local_rng.rand() % 5, 
			-4.0 + local_rng.rand() % 9);
		return spellTimer;
	}

	Uint32 color = makeColor(255, 128, 0, 255);
	for ( int i = 0; i < 4; ++i )
	{
		if ( Entity* fx = createParticleAOEIndicator(spellTimer, x, y, -7.5, TICKS_PER_SECOND, radius) )
		{
			//fx->yaw = my->yaw + PI / 2;
			if ( i == 0 )
			{
				if ( multiplayer != CLIENT )
				{
					spawnExplosion(fx->x, fx->y, 0.0);
				}
			}
			if ( i >= 2 )
			{
				fx->pitch -= PI / 8;
			}
			else
			{
				fx->pitch += PI / 8;
			}
			if ( i % 2 == 1 )
			{
				fx->pitch += PI;
			}
			fx->z = 0.0;
			fx->actSpriteFollowUID = 0;
			fx->actSpriteCheckParentExists = 0;
			fx->fskill[0] = 0.125; // rotate
			if ( auto indicator = AOEIndicators_t::getIndicator(fx->skill[10]) )
			{
				//indicator->arc = PI / 2;
				indicator->indicatorColor = color;
				indicator->loop = false;
				indicator->framesPerTick = 1;
				indicator->ticksPerUpdate = 1;
				indicator->delayTicks = 0;
				indicator->expireAlphaRate = 0.9;
				indicator->cacheType = AOEIndicators_t::CACHE_EXPLOSION_AREA;
			}
		}
	}

	if ( Entity* fx = createParticleAOEIndicator(spellTimer, x, y, 0.0, TICKS_PER_SECOND, radius) )
	{
		fx->actSpriteFollowUID = 0;
		fx->actSpriteCheckParentExists = 0;
		if ( auto indicator = AOEIndicators_t::getIndicator(fx->skill[10]) )
		{
			//indicator->arc = PI / 2;
			indicator->indicatorColor = color;
			indicator->loop = false;
			indicator->gradient = 4;
			indicator->framesPerTick = 2;
			indicator->ticksPerUpdate = 1;
			indicator->delayTicks = 0;
			indicator->expireAlphaRate = 0.9;
			indicator->cacheType = AOEIndicators_t::CACHE_EXPLOSION_AREA;
		}
	}

	if ( spellID == SPELL_METEOR || spellID == SPELL_METEOR_SHOWER || spellID == SPELL_FIREBALL )
	{
		playSoundPosLocal(x, y, 819, 128);
		for ( int i = 0; i < 8; ++i )
		{
			if ( Entity* fx = createParticleAestheticOrbit(nullptr, 233, 25 + (10 * (local_rng.rand() % 6)), PARTICLE_EFFECT_IGNITE_ORBIT) )
			{
				fx->flags[SPRITE] = true;
				real_t rad = radius * 0.25 * (1 + (i / 2));
				real_t ang = 2 * PI * (i / 4.0) + (i >= 4 ? PI / 4 : 0.0);
				fx->x = x + rad * cos(ang);
				fx->y = y + rad * sin(ang);
				fx->z = 6.0 + -2.0 + local_rng.rand() % 5;
				fx->fskill[0] = x;
				fx->fskill[1] = y;
				fx->fskill[2] = ang;
				fx->fskill[3] += (local_rng.rand() % 10) * PI / 10.0;
				fx->z = 7.5;
				fx->vel_z = -0.1 + (local_rng.rand() % 10) * -.025;
				fx->actmagicOrbitDist = rad;
			}
			//Entity* entity = newEntity(233, 1, map.entities, nullptr); //Sprite entity.
			//entity->z = 6.0;
			//entity->ditheringDisabled = true;
			//entity->flags[SPRITE] = true;
			//entity->flags[PASSABLE] = true;
			//entity->flags[NOUPDATE] = true;
			//entity->flags[UNCLICKABLE] = true;
			//entity->flags[BRIGHT] = true;
			//entity->behavior = &actSprite;
			//entity->skill[0] = 1;
			//entity->skill[1] = 12;
			//entity->skill[2] = 4;
			//entity->vel_z = -0.25;
			//if ( multiplayer != CLIENT )
			//{
			//	entity_uids--;
			//}
			//entity->setUID(-3);
		}
	}

	serverSpawnMiscParticlesAtLocation(x, y, z, PARTICLE_EFFECT_AREA_EFFECT, spellID, radius);
	return spellTimer;
}

void doSpellExplosionArea(int spellID, Entity* my, Entity* caster, real_t x, real_t y, real_t z, real_t radius)
{
	if ( my->particleTimerVariable4 != 0 )
	{
		if ( Entity* ignoreEntity = uidToEntity(my->particleTimerVariable4) )
		{
			if ( auto hitProps = getParticleEmitterHitProps(my->getUID(), ignoreEntity) )
			{
				hitProps->hits++;
			}
		}
	}

	std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadius(x / 16, y / 16, 1 + (radius / 16));
	for ( auto it : entLists )
	{
		node_t* node;
		for ( node = it->first; node != nullptr; node = node->next )
		{
			Entity* entity = (Entity*)node->element;
			if ( entityDist(my, entity) > radius )
			{
				continue;
			}
			if ( entity == my )
			{
				continue;
			}
			bool mimic = entity->isInertMimic();

			if ( caster && caster->getStats() )
			{
				if ( caster->behavior == &actMonster && !caster->monsterAllyGetPlayerLeader() )
				{
					if ( caster->checkFriend(entity) )
					{
						continue;
					}
				}
				if ( caster->behavior == &actPlayer )
				{
					if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
					{
						if ( caster->checkFriend(entity) && caster->friendlyFireProtection(entity) )
						{
							continue;
						}
					}
				}
			}

			auto hitProps = getParticleEmitterHitProps(my->getUID(), entity);
			if ( !hitProps )
			{
				continue;
			}
			if ( hitProps->hits > 0 )
			{
				continue;
			}

			real_t tangent = atan2(entity->y - my->y, entity->x - my->x);
			bool oldPassable = entity->flags[PASSABLE];
			entity->flags[PASSABLE] = false;
			real_t d = lineTraceTarget(my, my->x, my->y, tangent, radius, 0, false, entity);
			entity->flags[PASSABLE] = oldPassable;
			if ( hit.entity != entity )
			{
				continue;
			}

			if ( entity->behavior == &actGreasePuddleSpawner )
			{
				if ( spellID != SPELL_EARTH_ELEMENTAL && spellID != SPELL_PROJECT_SPIRIT )
				{
					++hitProps->hits;
					entity->SetEntityOnFire(caster);
				}
			}
			else
			{
				int damage = my->particleTimerVariable1;
				if ( entity->getStats() )
				{
					if ( spellID != SPELL_ETERNALS_GAZE && spellID != SPELL_EARTH_ELEMENTAL && spellID != SPELL_PROJECT_SPIRIT )
					{
						if ( !entity->monsterIsTargetable(true) ) { continue; }
					}
					else if ( entity->flags[PASSABLE] )
					{
						continue;
					}
				}
				if ( applyGenericMagicDamage(caster, entity, caster ? *caster : *my, spellID, damage, true) )
				{
					++hitProps->hits;
					if ( spellID != SPELL_EARTH_ELEMENTAL && spellID != SPELL_PROJECT_SPIRIT )
					{
						if ( entity->SetEntityOnFire(caster) )
						{
							if ( entity->getStats() )
							{
								if ( caster )
								{
									entity->getStats()->burningInflictedBy = caster->getUID();
								}
							}
						}
					}
				}
			}
		}
	}
}

void createParticleSpin(Entity* entity)
{
	if ( entity )
	{
		constexpr auto color = makeColor(255, 255, 255, 255);
		for ( int i = 0; i < 24; ++i )
		{
			if ( Entity* fx = createParticleAOEIndicator(entity, entity->x, entity->y, -7.5, TICKS_PER_SECOND * 5, 16 + (i / 2) * 2) )
			{
				fx->yaw = entity->yaw + PI / 2 - (i / 2) * PI / 3;
				fx->pitch += PI / 32;
				if ( i % 2 == 1 )
				{
					fx->pitch += PI;
				}
				fx->z = 8.0;
				fx->z -= (i / 2) * 0.5;
				fx->vel_z -= 0.25;
				fx->fskill[0] = 0.3; // rotate
				fx->scalex = 0.5;// + (i / 2) * 0.25 / 12;
				fx->scaley = 0.5;// + (i / 2) * 0.25 / 12;
				if ( auto indicator = AOEIndicators_t::getIndicator(fx->skill[10]) )
				{
					indicator->arc = PI / 4;
					indicator->indicatorColor = color;
					indicator->loop = false;
					indicator->framesPerTick = 1;
					indicator->ticksPerUpdate = 1;
					indicator->delayTicks = 0;
				}
			}
		}
	}
}

void actParticleShatterEarth(Entity* my)
{
	if ( PARTICLE_LIFE < 0 )
	{
		list_RemoveNode(my->mynode);
		return;
	}

	--PARTICLE_LIFE;

	bool noground = false;
	bool tallCeiling = false;
	int x = my->x / 16;
	int y = my->y / 16;
	if ( x < 0 || x >= map.width || y < 0 || y >= map.height )
	{
		noground = true;
	}
	else
	{
		int mapIndex = (y)*MAPLAYERS + (x)*MAPLAYERS * map.height;
		if ( !map.tiles[mapIndex] || swimmingtiles[map.tiles[mapIndex]]
			|| lavatiles[map.tiles[mapIndex]] )
		{
			noground = true;
		}

		if ( !map.tiles[(MAPLAYERS - 1) + mapIndex] )
		{
			tallCeiling = true;
		}
	}

	if ( my->sprite == 1869 )
	{
		my->x = my->fskill[0];
		my->y = my->fskill[1];
		if ( my->ticks < TICKS_PER_SECOND + my->actmagicDelayMove )
		{
			if ( my->ticks >= TICKS_PER_SECOND / 2 )
			{
				my->fskill[2] = (local_rng.rand() % 31 - 15) / 50.f;
				my->fskill[3] = (local_rng.rand() % 31 - 15) / 50.f;
				my->x += my->fskill[2];
				my->y += my->fskill[3];
			}
			{
				real_t diff = std::max(0.01, (my->fskill[4] - my->z) / 10);
				my->z = std::min(my->z + diff, my->fskill[4]);
			}

			my->scalex = std::min(1.0, my->scalex + 0.05);
			my->scaley = my->scalex;
			my->scalez = my->scalex;
		}

		int tallCeilingDelay = 0;
		if ( tallCeiling )
		{
			if ( enableDebugKeys && (svFlags & SV_FLAG_CHEATS) )
			{
				static ConsoleVariable<int> cvar_se9("/se9", -5);
				tallCeilingDelay = *cvar_se9;
			}
			else
			{
				tallCeilingDelay = -5;
			}
		}

		static ConsoleVariable<int> cvar_se4("/se4", 20);
		int boulderSpawn = 20 + tallCeilingDelay;
		if ( enableDebugKeys && (svFlags & SV_FLAG_CHEATS) )
		{
			boulderSpawn = *cvar_se4 + tallCeilingDelay;
		}
		boulderSpawn += my->actmagicDelayMove;
		if ( my->ticks == 1 )
		{
			playSoundEntity(my, 799, 128);
		}
		else if ( my->ticks == boulderSpawn )
		{
			if ( multiplayer != CLIENT )
			{
				if ( my->skill[1] == SPELL_SHATTER_EARTH )
				{
					Entity* entity = newEntity(245, 1, map.entities, nullptr); // boulder
					entity->parent = my->getUID();
					entity->x = static_cast<int>(my->x / 16) * 16.0 + 8.0;
					entity->y = static_cast<int>(my->y / 16) * 16.0 + 8.0;
					entity->z = -64;
					entity->yaw = 0.0;
					entity->sizex = 7;
					entity->sizey = 7;
					entity->behavior = &actBoulder;
					entity->boulderShatterEarthSpell = my->parent;
					entity->boulderShatterEarthDamage = getSpellDamageFromID(SPELL_SHATTER_EARTH, uidToEntity(my->parent), nullptr, uidToEntity(my->parent));
					entity->flags[UPDATENEEDED] = true;
					entity->flags[PASSABLE] = true;

					playSoundEntity(entity, 150, 128);
					playSoundPlayer(clientnum, 150, 64);
					for ( int c = 0; c < MAXPLAYERS; c++ )
					{
						if ( players[c]->isLocalPlayer() )
						{
							inputs.addRumbleForHapticType(c, Inputs::HAPTIC_SFX_BOULDER_LAUNCH_VOL, entity->getUID());
						}
						else
						{
							playSoundPlayer(c, 150, 64);
							inputs.addRumbleRemotePlayer(c, Inputs::HAPTIC_SFX_BOULDER_LAUNCH_VOL, entity->getUID());
						}
					}
				}
				else if ( my->skill[1] == SPELL_EARTH_ELEMENTAL )
				{
					Entity* caster = uidToEntity(my->parent);
					if ( auto monster = summonMonsterNoSmoke(EARTH_ELEMENTAL, static_cast<int>(my->x / 16) * 16.0 + 8.0,
						static_cast<int>(my->y / 16) * 16.0 + 8.0) )
					{
						if ( caster )
						{
							if ( forceFollower(*caster, *monster) )
							{
								if ( caster->behavior == &actPlayer )
								{
									//Compendium_t::Events_t::eventUpdateMonster(caster->skill[2], Compendium_t::CPDM_RECRUITED, monster, 1);
									monster->monsterAllyIndex = caster->skill[2];
									monster->monsterAllySummonRank = 1;
									if ( Stat* monsterStats = monster->getStats() )
									{
										monsterStats->setAttribute("SUMMONED_CREATURE", "1");
										monsterStats->MISC_FLAGS[STAT_FLAG_MONSTER_DISABLE_HC_SCALING] = 1;
										int lvl = getSpellDamageFromID(SPELL_EARTH_ELEMENTAL, caster, nullptr, caster);
										int maxlvl = getSpellDamageSecondaryFromID(SPELL_EARTH_ELEMENTAL, caster, nullptr, caster);
										lvl = std::min(lvl, maxlvl);
										monsterStats->LVL = lvl;
									}
									if ( multiplayer == SERVER )
									{
										serverUpdateEntitySkill(monster, 42); // update monsterAllyIndex for clients.
									}
								}
							}
						}
					}
				}
			}
		}

		static ConsoleVariable<int> cvar_se5("/se5", 45);
		if ( my->ticks == *cvar_se5 + tallCeilingDelay + my->actmagicDelayMove )
		{
			for ( int i = 0; i < 8; ++i )
			{
				Entity* entity = newEntity(78, 1, map.entities, nullptr); //rubble
				entity->yaw = i * PI / 4;
				entity->x = my->x + 4.0 * cos(entity->yaw);
				entity->y = my->y + 4.0 * sin(entity->yaw);
				entity->z = my->z;
				entity->vel_x = (0.1 + (local_rng.rand() % 20) / 50.0) * cos(entity->yaw);
				entity->vel_y = (0.1 + (local_rng.rand() % 20) / 50.0) * sin(entity->yaw);
				entity->pitch = (local_rng.rand() % 360) * PI / 180.0;
				entity->roll = (local_rng.rand() % 360) * PI / 180.0;
				entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
				entity->behavior = &actParticleShatterEarthRock;
				static ConsoleVariable<float> cvar_se2("/se2", 3.0);
				entity->vel_z = *cvar_se2;
				entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
					*cvar_magic_fx_light_bonus, 0.f);
				entity->flags[PASSABLE] = true;
				entity->flags[UNCLICKABLE] = true;
				entity->flags[UPDATENEEDED] = false;
				entity->flags[NOUPDATE] = true;
				if ( multiplayer != CLIENT )
				{
					entity_uids--;
				}
				entity->setUID(-3);
			}
			my->flags[INVISIBLE] = true;
		}

		static ConsoleVariable<int> cvar_se6("/se6", 50);
		static ConsoleVariable<int> cvar_se7("/se7", 87);
		static ConsoleVariable<int> cvar_se8("/se8", 20);

		if ( noground )
		{
			if ( my->ticks == *cvar_se6 + my->actmagicDelayMove )
			{
				list_RemoveNode(my->mynode);
				return;
			}
		}
		else if ( !noground )
		{
			if ( my->ticks == *cvar_se6 + my->actmagicDelayMove )
			{
				my->flags[INVISIBLE] = false;
				my->pitch = 0.0;
				my->z = 7.0;
				my->scalex = 0.05;
				my->scaley = 0.05;
				my->scalez = 0.05;
				my->bNeedsRenderPositionInit = true;
			}
			else if ( my->ticks >= *cvar_se7 + my->actmagicDelayMove )
			{
				if ( my->ticks >= *cvar_se7 + *cvar_se8 + my->actmagicDelayMove )
				{
					my->z += 0.25;
					my->scalex = std::max(0.0, my->scalex - 0.05);
					my->scaley = my->scalex;
					my->scalez = my->scalex;
					if ( my->scalex <= 0.0 )
					{
						list_RemoveNode(my->mynode);
						return;
					}
				}
				/*else
				{
					my->scalex = std::min(1.0, my->scalex + 0.1);
					my->scaley = my->scalex;
					my->scalez = my->scalex;
					my->z = std::max(my->z - 0.1, 6.5);
				}*/
			}
			else if ( my->ticks >= *cvar_se6 + my->actmagicDelayMove )
			{
				my->scalex = std::min(0.5, my->scalex + 0.1);
				my->scaley = my->scalex;
				my->scalez = my->scalex;
			}
		}
	}
	else if ( my->sprite == 1868 )
	{
		static ConsoleVariable<int> cvar_se10("/se10", 42);
		int tallCeilingDelay = 0;
		if ( tallCeiling )
		{
			static ConsoleVariable<int> cvar_se11("/se11", -5);
			tallCeilingDelay = *cvar_se11;
		}
		if ( my->ticks >= TICKS_PER_SECOND * 2 + my->actmagicDelayMove )
		{
			my->scalex = std::max(0.0, my->scalex - 0.1);
			//my->scaley = my->scalex;
			my->scalez = my->scalex;
			if ( my->scalex <= 0.0 )
			{
				list_RemoveNode(my->mynode);
				return;
			}
		}
		else if ( my->ticks >= *cvar_se10 + tallCeilingDelay + my->actmagicDelayMove )
		{
			my->flags[INVISIBLE] = false;
		}
		else
		{
			my->scalex = std::min(1.0, my->scalex + 0.05);
			my->scaley = my->scalex;
			my->scalez = my->scalex;
		}
	}
}

void actParticleShatterEarthRock(Entity* my)
{
	my->x += my->vel_x;
	my->y += my->vel_y;
	my->z += my->vel_z;
	my->vel_z += 0.04;
	my->vel_x *= 0.95;
	my->vel_y *= 0.95;

	bool noground = false;
	int x = my->x / 16;
	int y = my->y / 16;
	if ( x < 0 || x >= map.width || y < 0 || y >= map.height )
	{
		noground = true;
	}
	else
	{
		int mapIndex = (y)*MAPLAYERS + (x)*MAPLAYERS * map.height;
		if ( !map.tiles[mapIndex] || swimmingtiles[map.tiles[mapIndex]]
			|| lavatiles[map.tiles[mapIndex]] )
		{
			noground = true;
		}
	}

	if ( noground )
	{
		my->roll += 0.04;
		my->pitch += 0.04;
		if ( my->z >= 7.5 )
		{
			++my->skill[1];
			if ( my->skill[1] >= TICKS_PER_SECOND / 2 )
			{
				list_RemoveNode(my->mynode);
				return;
			}
		}
	}
	else
	{
		if ( my->skill[0] == 0 )
		{
			if ( my->z >= 7.5 )
			{
				static ConsoleVariable<float> cvar_se3("/se3", -0.5);
				my->z = 7.5;
				my->vel_z = *cvar_se3;
				my->skill[0] = 1;
			}
		}
		else if ( my->skill[0] == 1 )
		{
			my->z = std::min(my->z, 7.5);
			if ( my->z >= 7.5 )
			{
				++my->skill[1];
				if ( my->skill[1] >= TICKS_PER_SECOND / 2 )
				{
					list_RemoveNode(my->mynode);
					return;
				}
			}
		}

		if ( my->z < 7.45 )
		{
			my->roll += 0.04;
			my->pitch += 0.04;
		}
	}
}

void createParticleShatterEarth(Entity* my, Entity* caster, real_t _x, real_t _y, int spellID)
{
	int x = static_cast<int>(_x / 16);
	int y = static_cast<int>(_y / 16);

	if ( !(x > 0 && x < map.width - 2 && y > 0 && y < map.height - 2) )
	{
		return;
	}

	int mapIndex = (y)*MAPLAYERS + (x)*MAPLAYERS * map.height;
	bool tallCeiling = false;
	if ( !map.tiles[(MAPLAYERS - 1) + mapIndex] )
	{
		tallCeiling = true;
	}

	{
		Entity* entity = newEntity(1869, 1, map.entities, nullptr); //rubble
		entity->x = x * 16.0 + 8.0;
		entity->y = y * 16.0 + 8.0;

		static ConsoleVariable<float> cvar_se("/se", -10.5);
		entity->z = *cvar_se + (tallCeiling ? -16.0 : 0.0);
		entity->yaw = 0.0;
		entity->pitch = PI;
		entity->fskill[0] = entity->x; // start coord
		entity->fskill[1] = entity->y; // start coord
		entity->fskill[4] = -7.5 + (tallCeiling ? -16.0 : 0.0);
		entity->scalex = 0.25;
		entity->scaley = 0.25;
		entity->scalez = 0.25;
		entity->skill[0] = 5 * TICKS_PER_SECOND;
		entity->skill[1] = spellID;
		entity->behavior = &actParticleShatterEarth;
		entity->parent = caster ? caster->getUID() : 0;
		entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
			*cvar_magic_fx_light_bonus, 0.f);
		entity->actmagicDelayMove = TICKS_PER_SECOND;
		entity->flags[PASSABLE] = true;
		entity->flags[UNCLICKABLE] = true;
		entity->flags[UPDATENEEDED] = false;
		entity->flags[NOUPDATE] = true;
		if ( multiplayer != CLIENT )
		{
			entity_uids--;
		}
		entity->setUID(-3);
	}

	{
		Entity* entity = newEntity(1868, 1, map.entities, nullptr); //boulder hole
		entity->x = x * 16.0 + 8.0;
		entity->y = y * 16.0 + 8.0;

		static ConsoleVariable<float> cvar_se1("/se1", -8.0);
		entity->z = *cvar_se1 + (tallCeiling ? -16.0 : 0.0);
		entity->yaw = 0.0;
		entity->scalex = 0.25;
		entity->scaley = 0.25;
		entity->scalez = 0.25;
		entity->skill[0] = 5 * TICKS_PER_SECOND;
		entity->flags[INVISIBLE] = true;
		entity->behavior = &actParticleShatterEarth;
		/*entity->lightBonus = vec4(*cvar_magic_fx_light_bonus, *cvar_magic_fx_light_bonus,
			*cvar_magic_fx_light_bonus, 0.f);*/
		entity->actmagicDelayMove = TICKS_PER_SECOND;
		entity->flags[PASSABLE] = true;
		entity->flags[UNCLICKABLE] = true;
		entity->flags[UPDATENEEDED] = false;
		entity->flags[NOUPDATE] = true;
		if ( multiplayer != CLIENT )
		{
			entity_uids--;
		}
		entity->setUID(-3);
	}
}