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

static const char* colorForSprite(Entity* my, int sprite, bool darker) {
	if ( my && my->flags[SPRITE] )
	{
		return nullptr;
	}
    if (darker) {
        switch (sprite) {
        case 672:
        case 168: return "magic_red_flicker";
        case 169: return "magic_orange_flicker";
        case 670:
        case 170: return "magic_yellow_flicker";
        case 983:
        case 171: return "magic_green_flicker";
        case 592:
		case 1244:
        case 172: return "magic_blue_flicker";
        case 625:
        case 173: return "magic_purple_flicker";
        default:
        case 669:
        case 680:
        case 174: return "magic_white_flicker";
        case 593:
        case 175: return "magic_black_flicker";
        case 678: return "magic_pink_flicker";
        }
    } else {
        switch (sprite) {
        case 672:
        case 168: return "magic_red";
        case 169: return "magic_orange";
        case 670:
        case 170: return "magic_yellow";
        case 983:
        case 171: return "magic_green";
        case 592:
		case 1244:
        case 172: return "magic_blue";
        case 625:
        case 173: return "magic_purple";
        default:
        case 669:
        case 680:
        case 174: return "magic_white";
        case 593:
        case 175: return "magic_black";
        case 678: return "magic_pink";
        }
    }
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
			particle->x = my->x;
			particle->y = my->y;
			//particle->z = my->z;
			particle->flags[INVISIBLE] = my->flags[INVISIBLE];
			particle->flags[INVISIBLE_DITHER] = my->flags[INVISIBLE_DITHER];
		}
	}

	real_t dist = clipMove(&my->x, &my->y, my->vel_x, my->vel_y, my);
	if ( dist != sqrt(my->vel_x * my->vel_x + my->vel_y * my->vel_y) )
	{
		my->removeLightField();
		list_RemoveNode(my->mynode);
		return;
	}

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

void actMagiclightBall(Entity* my)
{
	Entity* caster = NULL;
	if (!my)
	{
		return;
	}

	my->skill[2] = -10; // so the client sets the behavior of this entity

	if (clientnum != 0 && multiplayer == CLIENT)
	{
		my->removeLightField();

		//Light up the area.
		my->light = addLight(my->x / 16, my->y / 16, "magic_light");


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
				my->light = addLight(my->x / 16, my->y / 16, "magic_light");
			}
			else
			{
				my->removeLightField();
				my->light = addLight(my->x / 16, my->y / 16, "magic_light_flicker");
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

	caster = uidToEntity(spell->caster);
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
	else if (spell->caster >= 1)     //So no caster...but uidToEntity returns NULL if entity is already dead, right? And if the uid is supposed to point to an entity, but it doesn't...it means the caster has died.
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

	if (magic_init)
	{
		my->removeLightField();

		if (lightball_timer <= 0)
		{
			if ( spell->sustain )
			{
				//Attempt to sustain the magic light.
				if (caster)
				{
					//Deduct mana from caster. Cancel spell if not enough mana (simply leave sustained at false).
					bool deducted = caster->safeConsumeMP(1); //Consume 1 mana every duration / mana seconds
					if (deducted)
					{
						lightball_timer = spell->channel_duration / getCostOfSpell(spell);
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
		lightball_hoverangle += 1;

		//Lightball moving.
		//messagePlayer(0, "*");
		Entity* parent = uidToEntity(my->parent);
		if ( !parent )
		{
			return;
		}
		double distance = sqrt(pow(my->x - parent->x, 2) + pow(my->y - parent->y, 2));
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
				double tangent = atan2(parent->y - my->y, parent->x - my->x);
				lineTraceTarget(my, my->x, my->y, tangent, 1024, LINETRACE_IGNORE_ENTITIES, false, parent);
				if ( !hit.entity || hit.entity == parent )   //Line of sight to caster?
				{
					if (my->path != NULL)
					{
						list_FreeAll(my->path);
						my->path = NULL;
					}
					//double tangent = atan2(parent->y - my->y, parent->x - my->x);
					my->vel_x = cos(tangent) * ((distance - MAGICLIGHT_BALL_FOLLOW_DISTANCE) / MAGICLIGHTBALL_DIVIDE_CONSTANT);
					my->vel_y = sin(tangent) * ((distance - MAGICLIGHT_BALL_FOLLOW_DISTANCE) / MAGICLIGHTBALL_DIVIDE_CONSTANT);
					my->x += (my->vel_x < MAGIC_LIGHTBALL_SPEEDLIMIT) ? my->vel_x : MAGIC_LIGHTBALL_SPEEDLIMIT;
					my->y += (my->vel_y < MAGIC_LIGHTBALL_SPEEDLIMIT) ? my->vel_y : MAGIC_LIGHTBALL_SPEEDLIMIT;
					//} else if (!map.tiles[(int)(OBSTACLELAYER + (my->y / 16) * MAPLAYERS + (my->x / 16) * MAPLAYERS * map.height)]) { //If not in wall..
				}
				else
				{
					//messagePlayer(0, "******Pathfinding.");
					//Caster is not in line of sight. Calculate a move path.
					/*if (my->children->first != NULL) {
						list_RemoveNode(my->children->first);
						my->children->first = NULL;
					}*/
					if (!my->path)
					{
						//messagePlayer(0, "[Light ball] Generating path.");
						list_t* path = generatePath((int)floor(my->x / 16), (int)floor(my->y / 16), 
							(int)floor(parent->x / 16), (int)floor(parent->y / 16), my, parent,
							GeneratePathTypes::GENERATE_PATH_DEFAULT);
						if ( path != NULL )
						{
							my->path = path;
						}
						else
						{
							//messagePlayer(0, "[Light ball] Failed to generate path (%s line %d).", __FILE__, __LINE__);
						}
					}

					if (my->path)
					{
						double total_distance = 0; //Calculate the total distance to the player to get the right move speed.
						double prevx = my->x;
						double prevy = my->y;
						if (my->path != NULL)
						{
							for (node = my->path->first; node != NULL; node = node->next)
							{
								if (node->element)
								{
									auto pathnode = (pathnode_t*)node->element;
									//total_distance += sqrt(pow(pathnode->y - prevy, 2) + pow(pathnode->x - prevx, 2) );
									total_distance += sqrt(pow(prevx - pathnode->x, 2) + pow(prevy - pathnode->y, 2) );
									prevx = pathnode->x;
									prevy = pathnode->y;
								}
							}
						}
						else if (my->path)     //If the path has been traversed, reset it.
						{
							list_FreeAll(my->path);
							my->path = NULL;
						}
						total_distance -= MAGICLIGHT_BALL_FOLLOW_DISTANCE;

						if (my->path != NULL)
						{
							if (my->path->first != NULL)
							{
								auto pathnode = (pathnode_t*)my->path->first->element;
								//double distance = sqrt(pow(pathnode->y * 16 + 8 - my->y, 2) + pow(pathnode->x * 16 + 8 - my->x, 2) );
								//double distance = sqrt(pow((my->y) - ((pathnode->y + 8) * 16), 2) + pow((my->x) - ((pathnode->x + 8) * 16), 2));
								double distance = sqrt(pow(((pathnode->y * 16) + 8) - (my->y), 2) + pow(((pathnode->x * 16) + 8) - (my->x), 2));
								if (distance <= 4)
								{
									list_RemoveNode(my->path->first);
									if (!my->path->first)
									{
										list_FreeAll(my->path);
										my->path = NULL;
									}
								}
								else
								{
									double target_tangent = atan2((pathnode->y * 16) + 8 - my->y, (pathnode->x * 16) + 8 - my->x);
									if (target_tangent > my->yaw)   //TODO: Do this yaw thing for all movement.
									{
										my->yaw = (target_tangent >= my->yaw + MAGIC_LIGHTBALL_TURNSPEED) ? my->yaw + MAGIC_LIGHTBALL_TURNSPEED : target_tangent;
									}
									else if (target_tangent < my->yaw)
									{
										my->yaw = (target_tangent <= my->yaw - MAGIC_LIGHTBALL_TURNSPEED) ? my->yaw - MAGIC_LIGHTBALL_TURNSPEED : target_tangent;
									}
									my->vel_x = cos(my->yaw) * (total_distance / MAGICLIGHTBALL_DIVIDE_CONSTANT);
									my->vel_y = sin(my->yaw) * (total_distance / MAGICLIGHTBALL_DIVIDE_CONSTANT);
									my->x += (my->vel_x < MAGIC_LIGHTBALL_SPEEDLIMIT) ? my->vel_x : MAGIC_LIGHTBALL_SPEEDLIMIT;
									my->y += (my->vel_y < MAGIC_LIGHTBALL_SPEEDLIMIT) ? my->vel_y : MAGIC_LIGHTBALL_SPEEDLIMIT;
								}
							}
						} //else assertion error, hehehe
					}
					else     //Path failed to generate. Fallback on moving straight to the player.
					{
						//messagePlayer(0, "**************NO PATH WHEN EXPECTED PATH.");
						my->vel_x = cos(tangent) * ((distance) / MAGICLIGHTBALL_DIVIDE_CONSTANT);
						my->vel_y = sin(tangent) * ((distance) / MAGICLIGHTBALL_DIVIDE_CONSTANT);
						my->x += (my->vel_x < MAGIC_LIGHTBALL_SPEEDLIMIT) ? my->vel_x : MAGIC_LIGHTBALL_SPEEDLIMIT;
						my->y += (my->vel_y < MAGIC_LIGHTBALL_SPEEDLIMIT) ? my->vel_y : MAGIC_LIGHTBALL_SPEEDLIMIT;
					}
				} /*else {
					//In a wall. Get out of it.
					double tangent = atan2(parent->y - my->y, parent->x - my->x);
					my->vel_x = cos(tangent) * ((distance) / 100);
					my->vel_y = sin(tangent) * ((distance) / 100);
					my->x += my->vel_x;
					my->y += my->vel_y;
				}*/
			}
		}
		else
		{
			lightball_movement_timer = LIGHTBALL_MOVE_DELAY;
			/*if (lightball_player_lastmove_timer < LIGHTBALL_CIRCLE_TIME) {
				lightball_player_lastmove_timer++;
			} else {
				//TODO: Orbit the player. Maybe.
				my->x = parent->x + (lightball_orbit_length * cos(lightball_orbit_angle));
				my->y = parent->y + (lightball_orbit_length * sin(lightball_orbit_angle));

				lightball_orbit_angle++;
				if (lightball_orbit_angle > 360) {
					lightball_orbit_angle = 0;
				}
			}*/
			if (my->path != NULL)
			{
				list_FreeAll(my->path);
				my->path = NULL;
			}
			if (map.tiles[(int)(OBSTACLELAYER + (my->y / 16) * MAPLAYERS + (my->x / 16) * MAPLAYERS * map.height)])   //If the ball has come to rest in a wall, move its butt.
			{
				double tangent = atan2(parent->y - my->y, parent->x - my->x);
				my->vel_x = cos(tangent) * ((distance) / MAGICLIGHTBALL_DIVIDE_CONSTANT);
				my->vel_y = sin(tangent) * ((distance) / MAGICLIGHTBALL_DIVIDE_CONSTANT);
				my->x += my->vel_x;
				my->y += my->vel_y;
			}
		}

		//Light up the area.
		my->light = addLight(my->x / 16, my->y / 16, "magic_light");

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
				my->light = addLight(my->x / 16, my->y / 16, "magic_light");
			}
			else
			{
				my->removeLightField();
				my->light = addLight(my->x / 16, my->y / 16, "magic_light_flicker");
			}
			lightball_flicker = 0;
		}

		lightball_timer--;
	}
	else
	{
		//Init the lightball. That is, shoot out from the player.

		//Move out from the player.
		my->vel_x = cos(my->yaw) * 4;
		my->vel_y = sin(my->yaw) * 4;
		double dist = clipMove(&my->x, &my->y, my->vel_x, my->vel_y, my);

		unsigned int distance = sqrt(pow(my->x - lightball_player_startx, 2) + pow(my->y - lightball_player_starty, 2));
		if (distance > MAGICLIGHT_BALL_FOLLOW_DISTANCE * 2 || dist != sqrt(my->vel_x * my->vel_x + my->vel_y * my->vel_y))
		{
			magic_init = 1;
			my->sprite = 174; //Go from black ball to white ball.
			lightball_lighting = 1;
			lightball_movement_timer = 0; //Start off at 0 so that it moves towards the player as soon as it's created (since it's created farther away from the player).
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
				if ( hitstats->EFFECTS[EFF_BLEEDING] )
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

static ConsoleVariable<bool> cvar_magic_fx_use_vismap("/magic_fx_use_vismap", true);
void magicOnEntityHit(Entity* parent, Entity* particle, Entity* hitentity, Stat* hitstats, Sint32 preResistanceDamage, Sint32 damage, Sint32 oldHP, int spellID)
{
	if ( !hitentity  ) { return; }

 	if ( hitentity->behavior == &actPlayer )
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
		if ( !particle || (particle && particle->behavior != &actMagicMissile)) { return; }
		if ( particle->actmagicCastByMagicstaff != 0 )
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
				}
				else if ( damage == 0 && oldHP == 0 )
				{
					if ( find->second.spellTags.find(ItemTooltips_t::SpellTagTypes::SPELL_TAG_TRACK_HITS) != find->second.spellTags.end() )
					{
						Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_SPELL_TARGETS, 
							(ItemType)find->second.magicstaffId, 1);
					}
				}
			}
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
					}
					else if ( damage == 0 && oldHP == 0 )
					{
						if ( find->second.spellTags.find(ItemTooltips_t::SpellTagTypes::SPELL_TAG_TRACK_HITS) != find->second.spellTags.end() )
						{
							Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_SPELL_TARGETS,
								(ItemType)find->second.spellbookId, 1);
						}
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
						}
						else if ( damage == 0 && oldHP == 0 )
						{
							if ( find->second.spellTags.find(ItemTooltips_t::SpellTagTypes::SPELL_TAG_TRACK_HITS) != find->second.spellTags.end() )
							{
								Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_SPELL_TARGETS, SPELL_ITEM, 1, false, find->second.id);
							}
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
					Compendium_t::Events_t::eventUpdateWorld(hitentity->monsterAllyIndex, Compendium_t::CPDM_TRAP_FOLLOWERS_KILLED, category, 1);
				}
			}
		}
	}
}

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
	int i = 0;
	int c = 0;
	Entity* entity = NULL;
	double tangent;

	Entity* parent = uidToEntity(my->parent);

	if (magic_init)
	{
		my->removeLightField();

		if ( multiplayer != CLIENT )
		{
			//Handle the missile's life.
			MAGIC_LIFE++;

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
			ParticleEmitterHit_t* particleEmitterHitProps = nullptr;
			if ( my->actmagicSpray == 1 )
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
					dist = clipMove(&my->x, &my->y, my->vel_x, my->vel_y, my);
					if ( !hitFromAbove && my->z > -5 )
					{
						// if we didn't hit the floor, process normal horizontal movement collision if we aren't too high
						if ( dist != sqrt(my->vel_x * my->vel_x + my->vel_y * my->vel_y) )
						{
							hitFromAbove = true;
						}
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
					dist = clipMove(&my->x, &my->y, my->vel_x, my->vel_y, my); //normal flat projectiles
				}
			}

			if ( hitFromAbove 
				|| (my->actmagicIsVertical != MAGIC_ISVERTICAL_XYZ && my->actmagicSpray != 1 && dist != sqrt(my->vel_x * my->vel_x + my->vel_y * my->vel_y)) )
			{
				node = element->elements.first;
				//element = (spellElement_t *) element->elements->first->element;
				element = (spellElement_t*)node->element;
				//if (hit.entity != NULL) {
				bool mimic = hit.entity && hit.entity->isInertMimic();
				Stat* hitstats = nullptr;
				int player = -1;

				// count reflection
				int reflection = 0;
				if ( hit.entity )
				{
					hitstats = hit.entity->getStats();
					if ( hit.entity->behavior == &actPlayer )
					{
						player = hit.entity->skill[2];
					}
				}
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
					if ( reflection == 3 && hitstats->shield && hitstats->shield->type == MIRROR_SHIELD && hitstats->defending )
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
								if ( parent && (parent->behavior == &actMonster || parent->behavior == &actPlayer) )
								{
									my->actmagicMirrorReflected = 1;
									my->actmagicMirrorReflectedCaster = parent->getUID();
								}
								Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_SHIELD_REFLECT, hitstats->shield->type, 1);
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
						if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) && my->actmagicTinkerTrapFriendlyFire == 0 )
						{
							if ( parent && (parent->behavior == &actMonster || parent->behavior == &actPlayer) && parent->checkFriend(hit.entity) )
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
										if ( my->actmagicSpray == 1 )
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
								if ( my->actmagicCastByTinkerTrap == 1 )
								{
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
					if ( hit.side == HORIZONTAL )
					{
						my->vel_x *= -1;
						my->yaw = atan2(my->vel_y, my->vel_x);
					}
					else if ( hit.side == VERTICAL )
					{
						my->vel_y *= -1;
						my->yaw = atan2(my->vel_y, my->vel_x);
					}
					else if ( hit.side == 0 )
					{
						my->vel_x *= -1;
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
							my->pitch = 0;
						}
						my->parent = hit.entity->getUID();
						++my->actmagicReflectionCount;
					}

					if ( bShouldEquipmentDegrade )
					{
						// Reflection of 3 does not degrade equipment
						bool chance = false;
						if ( my->actmagicSpray == 1 )
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
								if ( hitstats->cloak )
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
								if ( hitstats->amulet )
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
								if ( hitstats->shield )
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
								}
								else if ( reflection == 2 )
								{
									net_packet->data[5] = hitstats->amulet->status;
								}
								else
								{
									net_packet->data[5] = hitstats->shield->status;
								}
								net_packet->address.host = net_clients[player - 1].host;
								net_packet->address.port = net_clients[player - 1].port;
								net_packet->len = 6;
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
					return;
				}

				// Test for Friendly Fire, if Friendly Fire is OFF, delete the missile
				if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
				{
					if ( !strcmp(element->element_internal_name, spellElement_telePull.element_internal_name)
						|| !strcmp(element->element_internal_name, spellElement_shadowTag.element_internal_name)
						|| my->actmagicTinkerTrapFriendlyFire == 1 )
					{
						// these spells can hit allies no penalty.
					}
					else if ( parent && parent->checkFriend(hit.entity) )
					{
						my->removeLightField();
						list_RemoveNode(my->mynode);
						return;
					}
				}

				int trapResist = 0;
				if ( parent && (parent->behavior == &actMagicTrap || parent->behavior == &actMagicTrapCeiling) )
				{
					trapResist = hit.entity ? hit.entity->getEntityBonusTrapResist() : 0;
				}

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
							alertTarget = true;
							alertAllies = true;
							if ( parent->behavior == &actMonster && parent->monsterAllyIndex != -1 )
							{
								if ( hit.entity->behavior == &actMonster && hit.entity->monsterAllyIndex != -1 )
								{
									// if a player ally + hit another ally, don't aggro back
									alertTarget = false;
								}
							}
							if ( !strcmp(element->element_internal_name, spellElement_telePull.element_internal_name) 
								|| !strcmp(element->element_internal_name, spellElement_shadowTag.element_internal_name) )
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

				// check for magic resistance...
				// resistance stacks diminishingly
				int resistance = 0;
				DamageGib dmgGib = DMG_DEFAULT;
				real_t damageMultiplier = 1.0;
				if ( hit.entity )
				{
					resistance = Entity::getMagicResistance(hit.entity->getStats());
					if ( (hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer) && hitstats )
					{
						damageMultiplier = Entity::getDamageTableMultiplier(hit.entity, *hitstats, DAMAGE_TABLE_MAGIC);
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
				}

				if ( hit.entity )
				{
					if ( parent && (parent->behavior == &actMagicTrap || parent->behavior == &actMagicTrapCeiling) )
					{
						if ( trapResist != 0 )
						{
							damageMultiplier += -(trapResist / 100.0);
							damageMultiplier = std::max(0.0, damageMultiplier);
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
					if ( my->actmagicCastByMagicstaff == 0 && my->actmagicCastByTinkerTrap == 0 )
					{
						spellbookDamageBonus += getBonusFromCasterOfSpellElement(parent, nullptr, element, spell ? spell->ID : SPELL_NONE);
						if ( parent && parent->behavior == &actPlayer )
						{
							Compendium_t::Events_t::eventUpdateCodex(parent->skill[2], Compendium_t::CPDM_CLASS_PWR_MAX_CASTED, "pwr",
								100 + (Sint32)(spellbookDamageBonus * 100.0));
						}
					}
					if ( parent && (parent->behavior == &actMagicTrap || parent->behavior == &actMagicTrapCeiling) )
					{
						if ( gameModeManager.currentSession.challengeRun.isActive(GameModeManager_t::CurrentSession_t::ChallengeRun_t::CHEVENT_STRONG_TRAPS) )
						{
							spellbookDamageBonus += 1.0;
						}
					}
				}

				if (!strcmp(element->element_internal_name, spellElement_force.element_internal_name))
				{
					if (hit.entity)
					{
						if ( mimic )
						{
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);
							hit.entity->chestHandleDamageMagic(damage, *my, parent);
							my->removeLightField();
							list_RemoveNode(my->mynode);
							return;
						}
						else if (hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer)
						{
							Entity* parent = uidToEntity(my->parent);
							playSoundEntity(hit.entity, 28, 128);
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
							Sint32 preResistanceDamage = damage;
							damage *= damageMultiplier;
							damage /= (1 + (int)resistance);
							Sint32 oldHP = hitstats->HP;
							hit.entity->modHP(-damage);
							magicOnEntityHit(parent, my, hit.entity, hitstats, preResistanceDamage, damage, oldHP, spell ? spell->ID : SPELL_NONE);
							magicTrapOnHit(parent, hit.entity, hitstats, oldHP, spell ? spell->ID : SPELL_NONE);
							for (i = 0; i < damage; i += 2)   //Spawn a gib for every two points of damage.
							{
								Entity* gib = spawnGib(hit.entity);
								serverSpawnGibForClient(gib);
							}

							if (parent)
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

							if ( hitstats->HP <= 0 && parent)
							{
								parent->awardXP( hit.entity, true, true );
								spawnBloodVialOnMonsterDeath(hit.entity, hitstats, parent);
							}
						}
						else if (hit.entity->behavior == &actDoor)
						{
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);
							hit.entity->doorHandleDamageMagic(damage, *my, parent);
							my->removeLightField();
							list_RemoveNode(my->mynode);
							return;
						}
						else if ( hit.entity->isDamageableCollider() && hit.entity->isColliderDamageableByMagic() )
						{
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);
							hit.entity->colliderHandleDamageMagic(damage, *my, parent);
							my->removeLightField();
							list_RemoveNode(my->mynode);
							return;
						}
						else if ( hit.entity->behavior == &actChest )
						{
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);
							hit.entity->chestHandleDamageMagic(damage, *my, parent);
							my->removeLightField();
							list_RemoveNode(my->mynode);
							return;
						}
						else if (hit.entity->behavior == &actFurniture )
						{
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);
							int oldHP = hit.entity->furnitureHealth;
							hit.entity->furnitureHealth -= damage;
							if ( parent )
							{
								if ( parent->behavior == &actPlayer )
								{
									bool destroyed = oldHP > 0 && hit.entity->furnitureHealth <= 0;
									if ( destroyed )
									{
										gameModeManager.currentSession.challengeRun.updateKillEvent(hit.entity);
									}
									switch ( hit.entity->furnitureType )
									{
										case FURNITURE_CHAIR:
											if ( destroyed )
											{
												messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(388));
											}
											updateEnemyBar(parent, hit.entity, Language::get(677), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
												false, DamageGib::DMG_DEFAULT);
											break;
										case FURNITURE_TABLE:
											if ( destroyed )
											{
												messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(389));
											}
											updateEnemyBar(parent, hit.entity, Language::get(676), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
												false, DamageGib::DMG_DEFAULT);
											break;
										case FURNITURE_BED:
											if ( destroyed )
											{
												messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(2508), Language::get(2505));
											}
											updateEnemyBar(parent, hit.entity, Language::get(2505), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
												false, DamageGib::DMG_DEFAULT);
											break;
										case FURNITURE_BUNKBED:
											if ( destroyed )
											{
												messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(2508), Language::get(2506));
											}
											updateEnemyBar(parent, hit.entity, Language::get(2506), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
												false, DamageGib::DMG_DEFAULT);
											break;
										case FURNITURE_PODIUM:
											if ( destroyed )
											{
												messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(2508), Language::get(2507));
											}
											updateEnemyBar(parent, hit.entity, Language::get(2507), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
												false, DamageGib::DMG_DEFAULT);
											break;
										default:
											break;
									}
								}
							}
							playSoundEntity(hit.entity, 28, 128);
						}
					}
				}
				else if (!strcmp(element->element_internal_name, spellElement_magicmissile.element_internal_name))
				{
					spawnExplosion(my->x, my->y, my->z);
					if (hit.entity)
					{
						if ( mimic )
						{
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);
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
							else
							{
								spawnExplosion(my->x, my->y, my->z);
							}
							return;
						}
						else if (hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer)
						{
							Entity* parent = uidToEntity(my->parent);
							playSoundEntity(hit.entity, 28, 128);
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
							if ( my->actmagicIsOrbiting == 2 )
							{
								spawnExplosion(my->x, my->y, my->z);
								if ( parent && my->actmagicOrbitCastFromSpell == 1 )
								{
									// cast through amplify magic effect
									damage /= 2;
								}
								damage = damage - local_rng.rand() % ((damage / 8) + 1);
							}

							Sint32 preResistanceDamage = damage;
							damage *= damageMultiplier;
							damage /= (1 + (int)resistance);
							Sint32 oldHP = hitstats->HP;
							hit.entity->modHP(-damage);
							magicTrapOnHit(parent, hit.entity, hitstats, oldHP, spell ? spell->ID : SPELL_NONE);
							magicOnEntityHit(parent, my, hit.entity, hitstats, preResistanceDamage, damage, oldHP, spell ? spell->ID : SPELL_NONE);
							for (i = 0; i < damage; i += 2)   //Spawn a gib for every two points of damage.
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

							if ( hitstats->HP <= 0 && parent)
							{
								parent->awardXP( hit.entity, true, true );
								spawnBloodVialOnMonsterDeath(hit.entity, hitstats, parent);
							}
						}
						else if ( hit.entity->behavior == &actDoor )
						{
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
							damage /= (1 + (int)resistance);
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
							else
							{
								spawnExplosion(my->x, my->y, my->z);
							}
							return;
						}
						else if ( hit.entity->isDamageableCollider() && hit.entity->isColliderDamageableByMagic() )
						{
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);

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
							else
							{
								spawnExplosion(my->x, my->y, my->z);
							}
							return;
						}
						else if ( hit.entity->behavior == &actChest )
						{
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);
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
							else
							{
								spawnExplosion(my->x, my->y, my->z);
							}
							return;
						}
						else if (hit.entity->behavior == &actFurniture )
						{
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);
							int oldHP = hit.entity->furnitureHealth;
							hit.entity->furnitureHealth -= damage;
							if ( parent )
							{
								if ( parent->behavior == &actPlayer )
								{
									bool destroyed = oldHP > 0 && hit.entity->furnitureHealth <= 0;
									if ( destroyed )
									{
										gameModeManager.currentSession.challengeRun.updateKillEvent(hit.entity);
									}
									switch ( hit.entity->furnitureType )
									{
									case FURNITURE_CHAIR:
										if ( destroyed )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(388));
										}
										updateEnemyBar(parent, hit.entity, Language::get(677), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
											false, DamageGib::DMG_DEFAULT);
										break;
									case FURNITURE_TABLE:
										if ( destroyed )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(389));
										}
										updateEnemyBar(parent, hit.entity, Language::get(676), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
											false, DamageGib::DMG_DEFAULT);
										break;
									case FURNITURE_BED:
										if ( destroyed )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(2508), Language::get(2505));
										}
										updateEnemyBar(parent, hit.entity, Language::get(2505), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
											false, DamageGib::DMG_DEFAULT);
										break;
									case FURNITURE_BUNKBED:
										if ( destroyed )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(2508), Language::get(2506));
										}
										updateEnemyBar(parent, hit.entity, Language::get(2506), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
											false, DamageGib::DMG_DEFAULT);
										break;
									case FURNITURE_PODIUM:
										if ( destroyed )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(2508), Language::get(2507));
										}
										updateEnemyBar(parent, hit.entity, Language::get(2507), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
											false, DamageGib::DMG_DEFAULT);
										break;
									default:
										break;
									}
								}
							}
							playSoundEntity(hit.entity, 28, 128);
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
							else
							{
								spawnExplosion(my->x, my->y, my->z);
							}
							return;
						}
					}
				}
				else if (!strcmp(element->element_internal_name, spellElement_fire.element_internal_name))
				{
					if ( !(my->actmagicIsOrbiting == 2) )
					{
						spawnExplosion(my->x, my->y, my->z);
					}
					if (hit.entity)
					{
						// Attempt to set the Entity on fire
						hit.entity->SetEntityOnFire();

						if ( mimic )
						{
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);
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
							else
							{
								spawnExplosion(my->x, my->y, my->z);
							}
							return;
						}
						else if (hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer)
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

							//playSoundEntity(my, 153, 64);
							playSoundEntity(hit.entity, 28, 128);
							//TODO: Apply fire resistances/weaknesses.
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
							if ( my->actmagicIsOrbiting == 2 )
							{
								spawnExplosion(my->x, my->y, my->z);
								if ( parent && my->actmagicOrbitCastFromSpell == 0 )
								{
									if ( parent->behavior == &actParticleDot )
									{
										damage = parent->skill[1];
									}
									else if ( parent->behavior == &actPlayer )
									{
										Stat* playerStats = parent->getStats();
										if ( playerStats )
										{
											int skillLVL = playerStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
											damage = (14 + skillLVL * 1.5);
										}
									}
									else
									{
										damage = 14;
									}
								}
								else if ( parent && my->actmagicOrbitCastFromSpell == 1 )
								{
									// cast through amplify magic effect
									damage /= 2;
								}
								else
								{
									damage = 14;
								}
								damage = damage - local_rng.rand() % ((damage / 8) + 1);
							}
							Sint32 preResistanceDamage = damage;
							damage *= damageMultiplier;
							if ( parent )
							{
								Stat* casterStats = parent->getStats();
								if ( casterStats && casterStats->type == LICH_FIRE && parent->monsterLichAllyStatus == LICH_ALLY_DEAD )
								{
									damage *= 2;
								}
							}
							int oldHP = hitstats->HP;
							damage *= fireMultiplier;
							damage /= (1 + (int)resistance);
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
						else if (hit.entity->behavior == &actDoor)
						{
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);

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
							else
							{
								spawnExplosion(my->x, my->y, my->z);
							}
							return;
						} 
						else if ( hit.entity->isDamageableCollider() && hit.entity->isColliderDamageableByMagic() )
						{
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);

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
							else
							{
								spawnExplosion(my->x, my->y, my->z);
							}
							return;
						}
						else if (hit.entity->behavior == &actChest) 
						{
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);
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
							else
							{
								spawnExplosion(my->x, my->y, my->z);
							}
							return;
						}
						else if (hit.entity->behavior == &actFurniture )
						{
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);
							int oldHP = hit.entity->furnitureHealth;
							hit.entity->furnitureHealth -= damage;
							if ( parent )
							{
								if ( parent->behavior == &actPlayer )
								{
									bool destroyed = oldHP > 0 && hit.entity->furnitureHealth <= 0;
									if ( destroyed )
									{
										gameModeManager.currentSession.challengeRun.updateKillEvent(hit.entity);
									}
									switch ( hit.entity->furnitureType )
									{
									case FURNITURE_CHAIR:
										if ( destroyed )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(388));
										}
										updateEnemyBar(parent, hit.entity, Language::get(677), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
											false, DamageGib::DMG_DEFAULT);
										break;
									case FURNITURE_TABLE:
										if ( destroyed )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(389));
										}
										updateEnemyBar(parent, hit.entity, Language::get(676), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
											false, DamageGib::DMG_DEFAULT);
										break;
									case FURNITURE_BED:
										if ( destroyed )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(2508), Language::get(2505));
										}
										updateEnemyBar(parent, hit.entity, Language::get(2505), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
											false, DamageGib::DMG_DEFAULT);
										break;
									case FURNITURE_BUNKBED:
										if ( destroyed )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(2508), Language::get(2506));
										}
										updateEnemyBar(parent, hit.entity, Language::get(2506), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
											false, DamageGib::DMG_DEFAULT);
										break;
									case FURNITURE_PODIUM:
										if ( destroyed )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(2508), Language::get(2507));
										}
										updateEnemyBar(parent, hit.entity, Language::get(2507), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
											false, DamageGib::DMG_DEFAULT);
										break;
									default:
										break;
									}
								}
							}
							playSoundEntity(hit.entity, 28, 128);
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
							else
							{
								spawnExplosion(my->x, my->y, my->z);
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
							int duration = (element->duration * (((element->mana) / static_cast<double>(element->base_mana)) * element->overload_multiplier));
							duration /= (1 + (int)resistance);

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

							if ( duration > 0 && hit.entity->setEffect(EFF_CONFUSED, true, duration, false) )
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
							real_t coldMultiplier = 1.0;
							bool warmHat = false;
							if ( hitstats->helmet && hitstats->helmet->type == HAT_WARM )
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
									warmHat = true;
								}
							}

							playSoundEntity(hit.entity, 28, 128);

							if ( !warmHat )
							{
								hitstats->EFFECTS[EFF_SLOW] = true;
								hitstats->EFFECTS_TIMERS[EFF_SLOW] = (element->duration * (((element->mana) / static_cast<double>(element->base_mana)) * element->overload_multiplier));
								hitstats->EFFECTS_TIMERS[EFF_SLOW] /= (1 + (int)resistance);

								// If the Entity hit is a Player, update their status to be Slowed
								if ( hit.entity->behavior == &actPlayer )
								{
									serverUpdateEffects(hit.entity->skill[2]);
								}
							}

							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							//messagePlayer(0, "damage: %d", damage);
							if ( my->actmagicIsOrbiting == 2 )
							{
								if ( parent && my->actmagicOrbitCastFromSpell == 0 )
								{
									if ( parent->behavior == &actParticleDot )
									{
										damage = parent->skill[1];
									}
									else if ( parent->behavior == &actPlayer )
									{
										Stat* playerStats = parent->getStats();
										if ( playerStats )
										{
											int skillLVL = playerStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
											damage = (18 + skillLVL * 1.5);
										}
									}
									else
									{
										damage = 18;
									}
								}
								else if ( parent && my->actmagicOrbitCastFromSpell == 1 )
								{
									// cast through amplify magic effect
									damage /= 2;
								}
								else
								{
									damage = 18;
								}
								damage = damage - local_rng.rand() % ((damage / 8) + 1);
							}
							//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
							int oldHP = hitstats->HP;
							Sint32 preResistanceDamage = damage;
							damage *= damageMultiplier;
							damage *= coldMultiplier;
							damage /= (1 + (int)resistance);

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
							hitstats->EFFECTS[EFF_SLOW] = true;
							hitstats->EFFECTS_TIMERS[EFF_SLOW] = (element->duration * (((element->mana) / static_cast<double>(element->base_mana)) * element->overload_multiplier));
							hitstats->EFFECTS_TIMERS[EFF_SLOW] /= (1 + (int)resistance);

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
								effectDuration = 600 + local_rng.rand() % 300; // 12 seconds + 0 to 6 seconds.
								if ( hitstats )
								{
									effectDuration = std::max(0, effectDuration - ((hitstats->CON % 10) * 50)); // reduce 1 sec every 10 CON.
								}
							}
							effectDuration /= (1 + (int)resistance);

							bool magicTrapReapplySleep = true;

							if ( parent && (parent->behavior == &actMagicTrap || parent->behavior == &actMagicTrapCeiling) )
							{
								if ( hitstats && hitstats->EFFECTS[EFF_ASLEEP] )
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
				else if (!strcmp(element->element_internal_name, spellElement_lightning.element_internal_name))
				{
					playSoundEntity(my, 173, 128);
					if (hit.entity)
					{
						if ( mimic )
						{
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);
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
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							if ( my->actmagicIsOrbiting == 2 )
							{
								if ( parent && my->actmagicOrbitCastFromSpell == 0 )
								{
									if ( parent->behavior == &actParticleDot )
									{
										damage = parent->skill[1];
									}
									else if ( parent->behavior == &actPlayer )
									{
										Stat* playerStats = parent->getStats();
										if ( playerStats )
										{
											int skillLVL = playerStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
											damage = (22 + skillLVL * 1.5);
										}
									}
									else
									{
										damage = 22;
									}
								}
								else if ( parent && my->actmagicOrbitCastFromSpell == 1 )
								{
									// cast through amplify magic effect
									damage /= 2;
								}
								else
								{
									damage = 22;
								}
								damage = damage - local_rng.rand() % ((damage / 8) + 1);
							}
							//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
							int oldHP = hitstats->HP;
							Sint32 preResistanceDamage = damage;
							damage *= damageMultiplier;
							damage /= (1 + (int)resistance);
							hit.entity->modHP(-damage);
							magicOnEntityHit(parent, my, hit.entity, hitstats, preResistanceDamage, damage, oldHP, spell ? spell->ID : SPELL_NONE);
							magicTrapOnHit(parent, hit.entity, hitstats, oldHP, spell ? spell->ID : SPELL_NONE);

							// write the obituary
							if (parent)
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
						else if ( hit.entity->behavior == &actDoor )
						{
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
							damage /= (1 + (int)resistance);

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
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
							damage /= (1 + (int)resistance);

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
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);
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
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);
							int oldHP = hit.entity->furnitureHealth;
							hit.entity->furnitureHealth -= damage;
							if ( parent )
							{
								if ( parent->behavior == &actPlayer )
								{
									bool destroyed = oldHP > 0 && hit.entity->furnitureHealth <= 0;
									if ( destroyed )
									{
										gameModeManager.currentSession.challengeRun.updateKillEvent(hit.entity);
									}
									switch ( hit.entity->furnitureType )
									{
									case FURNITURE_CHAIR:
										if ( destroyed )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(388));
										}
										updateEnemyBar(parent, hit.entity, Language::get(677), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
											false, DamageGib::DMG_DEFAULT);
										break;
									case FURNITURE_TABLE:
										if ( destroyed )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(389));
										}
										updateEnemyBar(parent, hit.entity, Language::get(676), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
											false, DamageGib::DMG_DEFAULT);
										break;
									case FURNITURE_BED:
										if ( destroyed )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(2508), Language::get(2505));
										}
										updateEnemyBar(parent, hit.entity, Language::get(2505), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
											false, DamageGib::DMG_DEFAULT);
										break;
									case FURNITURE_BUNKBED:
										if ( destroyed )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(2508), Language::get(2506));
										}
										updateEnemyBar(parent, hit.entity, Language::get(2506), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
											false, DamageGib::DMG_DEFAULT);
										break;
									case FURNITURE_PODIUM:
										if ( destroyed )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(2508), Language::get(2507));
										}
										updateEnemyBar(parent, hit.entity, Language::get(2507), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
											false, DamageGib::DMG_DEFAULT);
										break;
									default:
										break;
									}
								}
							}
							playSoundEntity(hit.entity, 28, 128);
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}
						}
					}
				}
				else if ( spell->ID >= SPELL_SLIME_ACID && spell->ID <= SPELL_SLIME_METAL )
				{
					if ( hit.entity )
					{
						int volume = 128;
						static ConsoleVariable cvar_slimehit_sfx("/slimehit_sfx", 173);
						int hitsfx = *cvar_slimehit_sfx;
						int hitvolume = (particleEmitterHitProps && particleEmitterHitProps->hits == 1) ? 128 : 64;
						if ( spell->ID == SPELL_SLIME_WATER || spell->ID == SPELL_SLIME_TAR )
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
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);
							damage = std::max(2, damage);
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
								hit.entity->SetEntityOnFire();
							}
							return;
						}
						else if ( hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer )
						{
							playSoundEntity(hit.entity, hitsfx, hitvolume);
							Entity* parent = uidToEntity(my->parent);
							playSoundEntity(hit.entity, 28, volume);
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							if ( spell->ID == SPELL_SLIME_WATER && hitstats->type == VAMPIRE )
							{
								damage *= 2;
							}

							//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
							int oldHP = hitstats->HP;
							Sint32 preResistanceDamage = damage;
							damage *= damageMultiplier;
							damage /= (1 + (int)resistance);

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
									hit.entity->SetEntityOnFire();
								}
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
										int duration = 6 * TICKS_PER_SECOND;
										duration /= (1 + (int)resistance);

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
										int duration = 10 * TICKS_PER_SECOND;
										duration /= (1 + (int)resistance);

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

								int duration = (spell->ID == SPELL_SLIME_METAL ? 10 : 6) * TICKS_PER_SECOND;
								duration /= (1 + (int)resistance);
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
							}

							damage = std::max(2, damage);
							hit.entity->modHP(-damage);
							magicOnEntityHit(parent, my, hit.entity, hitstats, preResistanceDamage, damage, oldHP, spell ? spell->ID : SPELL_NONE);
							magicTrapOnHit(parent, hit.entity, hitstats, oldHP, spell ? spell->ID : SPELL_NONE);

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
							if ( oldHP > 0 && hitstats->HP <= 0 && parent )
							{
								parent->awardXP(hit.entity, true, true);
								spawnBloodVialOnMonsterDeath(hit.entity, hitstats, parent);
							}
						}
						else if ( hit.entity->behavior == &actDoor )
						{
							if ( spell->ID == SPELL_SLIME_FIRE )
							{
								hit.entity->SetEntityOnFire();
							}
							playSoundEntity(hit.entity, hitsfx, hitvolume);
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
							damage /= (1 + (int)resistance);

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
								hit.entity->SetEntityOnFire();
							}
							playSoundEntity(hit.entity, hitsfx, hitvolume);
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
							damage /= (1 + (int)resistance);

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
								hit.entity->SetEntityOnFire();
							}
							playSoundEntity(hit.entity, hitsfx, hitvolume);
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);
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
								hit.entity->SetEntityOnFire();
							}
							playSoundEntity(hit.entity, hitsfx, hitvolume);
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);
							int oldHP = hit.entity->furnitureHealth;
							hit.entity->furnitureHealth -= damage;
							if ( parent )
							{
								if ( parent->behavior == &actPlayer )
								{
									bool destroyed = oldHP > 0 && hit.entity->furnitureHealth <= 0;
									if ( destroyed )
									{
										gameModeManager.currentSession.challengeRun.updateKillEvent(hit.entity);
									}
									switch ( hit.entity->furnitureType )
									{
									case FURNITURE_CHAIR:
										if ( destroyed )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(388));
										}
										updateEnemyBar(parent, hit.entity, Language::get(677), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
											false, DamageGib::DMG_DEFAULT);
										break;
									case FURNITURE_TABLE:
										if ( destroyed )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(389));
										}
										updateEnemyBar(parent, hit.entity, Language::get(676), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
											false, DamageGib::DMG_DEFAULT);
										break;
									case FURNITURE_BED:
										if ( destroyed )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(2508), Language::get(2505));
										}
										updateEnemyBar(parent, hit.entity, Language::get(2505), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
											false, DamageGib::DMG_DEFAULT);
										break;
									case FURNITURE_BUNKBED:
										if ( destroyed )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(2508), Language::get(2506));
										}
										updateEnemyBar(parent, hit.entity, Language::get(2506), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
											false, DamageGib::DMG_DEFAULT);
										break;
									case FURNITURE_PODIUM:
										if ( destroyed )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(2508), Language::get(2507));
										}
										updateEnemyBar(parent, hit.entity, Language::get(2507), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
											false, DamageGib::DMG_DEFAULT);
										break;
									default:
										break;
									}
								}
							}
							playSoundEntity(hit.entity, 28, volume);
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}
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
							real_t pushbackMultiplier = 0.6;// +(0.2 * spellbookDamageBonus);
							if ( !hit.entity->isMobile() )
							{
								pushbackMultiplier += 0.3;
							}

							bool doSlow = true;
							const int duration = TICKS_PER_SECOND * 2;
							if ( hitstats )
							{
								if ( hitstats->EFFECTS[EFF_SLOW] || hitstats->EFFECTS_TIMERS[EFF_SLOW] > duration )
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
						if (hit.entity->behavior == &actDoor)
						{
							if ( parent && parent->behavior == &actPlayer && MFLAG_DISABLEOPENING )
							{
								Uint32 color = makeColorRGB(255, 0, 255);
								messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, 0xFFFFFFFF, Language::get(3096), Language::get(3097));
								messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, color, Language::get(3101)); // disabled locking spell.
							}
							else
							{
								playSoundEntity(hit.entity, 92, 64);
								hit.entity->skill[5] = 1; //Lock the door.
								if ( parent )
								{
									if ( parent->behavior == &actPlayer )
									{
										messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(399));
										magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
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
									hit.entity->lockChest();
									if ( parent )
									{
										if ( parent->behavior == &actPlayer )
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(400));
											magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
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
										magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
									}
									if ( !hitstats->EFFECTS[EFF_MIMIC_LOCKED] )
									{
										if ( hit.entity->setEffect(EFF_MIMIC_LOCKED, true, TICKS_PER_SECOND * 5, false) )
										{
											hit.entity->monsterHitTime = HITRATE - 2;
											hitstats->monsterMimicLockedBy = parent ? parent->getUID() : 0;
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
						if (hit.entity->behavior == &actDoor)
						{
							if ( MFLAG_DISABLEOPENING || hit.entity->doorDisableOpening == 1 )
							{
								if ( parent && parent->behavior == &actPlayer )
								{
									Uint32 color = makeColorRGB(255, 0, 255);
									messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, 0xFFFFFFFF, Language::get(3096), Language::get(3097));
									messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, color, Language::get(3101)); // disabled opening spell.
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
										Compendium_t::Events_t::eventUpdateWorld(parent->skill[2], Compendium_t::CPDM_DOOR_UNLOCKED, "door", 1);
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
										messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(402));
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
									hit.entity->unlockChest();
									if ( parent )
									{
										if ( parent->behavior == &actPlayer)
										{
											messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(404)); // "The spell unlocks the chest!"
											Compendium_t::Events_t::eventUpdateWorld(parent->skill[2], Compendium_t::CPDM_CHESTS_UNLOCKED, "chest", 1);
											magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
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
								if ( hitstats->EFFECTS[EFF_MIMIC_LOCKED] )
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
								}
								magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
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
										if ( hitstats->EFFECTS[EFF_MIMIC_LOCKED] )
										{
											if ( hit.entity->setEffect(EFF_MIMIC_LOCKED, false, 0, false) )
											{
												hit.entity->monsterHitTime = std::max(hit.entity->monsterHitTime, HITRATE / 2);
											}
										}
										magicOnEntityHit(parent, my, hit.entity, hitstats, 0, 0, 0, spell ? spell->ID : SPELL_NONE);
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
							int effectDuration = (element->duration * (((element->mana) / static_cast<double>(element->base_mana)) * element->overload_multiplier));
							effectDuration /= (1 + (int)resistance);
							int oldDuration = !hitstats->EFFECTS[EFF_PARALYZED] ? 0 : hitstats->EFFECTS_TIMERS[EFF_PARALYZED];
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
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
							Sint32 preResistanceDamage = damage;
							damage *= damageMultiplier;
							Stat* casterStats = nullptr;
							if ( parent )
							{
								casterStats = parent->getStats();
								if ( casterStats && casterStats->type == LICH_FIRE && parent->monsterLichAllyStatus == LICH_ALLY_DEAD )
								{
									damage *= 2;
								}
							}
							damage /= (1 + (int)resistance);
							
							Sint32 oldHP = hitstats->HP;
							hit.entity->modHP(-damage);
							magicOnEntityHit(parent, my, hit.entity, hitstats, preResistanceDamage, damage, oldHP, spell ? spell->ID : SPELL_NONE);
							magicTrapOnHit(parent, hit.entity, hitstats, oldHP, spell ? spell->ID : SPELL_NONE);

							// write the obituary
							if ( parent )
							{
								parent->killedByMonsterObituary(hit.entity);
							}

							int bleedDuration = (element->duration * (((element->mana) / static_cast<double>(element->base_mana)) * element->overload_multiplier));
							bleedDuration /= (1 + (int)resistance);
							bool wasBleeding = hit.entity->getStats() ? hit.entity->getStats()->EFFECTS[EFF_BLEEDING] : false;
							if ( hit.entity->setEffect(EFF_BLEEDING, true, bleedDuration, true) )
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
										parent->setEffect(EFF_MP_REGEN, true, 250, true);
										if ( parent->behavior == &actPlayer )
										{
											messagePlayerColor(parent->skill[2], MESSAGE_HINT, color, Language::get(3753));
											steamStatisticUpdateClient(parent->skill[2], STEAM_STAT_ITS_A_LIVING, STEAM_STAT_INT, 1);
										}
										playSoundEntity(parent, 168, 128);
									}
								}
							}
							hitstats->EFFECTS[EFF_SLOW] = true;
							hitstats->EFFECTS_TIMERS[EFF_SLOW] = (element->duration * (((element->mana) / static_cast<double>(element->base_mana)) * element->overload_multiplier));
							hitstats->EFFECTS_TIMERS[EFF_SLOW] /= 4;
							hitstats->EFFECTS_TIMERS[EFF_SLOW] /= (1 + (int)resistance);
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
										for ( c = 0; c < MAXPLAYERS; ++c )
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
									int damage = element->damage;
									damage += (spellbookDamageBonus * damage);
									//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
									damage /= (1 + (int)resistance);

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
									int damage = element->damage;
									damage += (spellbookDamageBonus * damage);
									damage /= (1 + (int)resistance);
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
									int damage = element->damage;
									damage += (spellbookDamageBonus * damage);
									damage /= (1 + (int)resistance);
									int oldHP = hit.entity->furnitureHealth;
									hit.entity->furnitureHealth -= damage;
									if ( parent )
									{
										if ( parent->behavior == &actPlayer )
										{
											bool destroyed = oldHP > 0 && hit.entity->furnitureHealth <= 0;
											if ( destroyed )
											{
												gameModeManager.currentSession.challengeRun.updateKillEvent(hit.entity);
											}
											switch ( hit.entity->furnitureType )
											{
											case FURNITURE_CHAIR:
												if ( destroyed )
												{
													messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(388));
												}
												updateEnemyBar(parent, hit.entity, Language::get(677), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
													false, DamageGib::DMG_DEFAULT);
												break;
											case FURNITURE_TABLE:
												if ( destroyed )
												{
													messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(389));
												}
												updateEnemyBar(parent, hit.entity, Language::get(676), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
													false, DamageGib::DMG_DEFAULT);
												break;
											case FURNITURE_BED:
												if ( destroyed )
												{
													messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(2508), Language::get(2505));
												}
												updateEnemyBar(parent, hit.entity, Language::get(2505), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
													false, DamageGib::DMG_DEFAULT);
												break;
											case FURNITURE_BUNKBED:
												if ( destroyed )
												{
													messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(2508), Language::get(2506));
												}
												updateEnemyBar(parent, hit.entity, Language::get(2506), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
													false, DamageGib::DMG_DEFAULT);
												break;
											case FURNITURE_PODIUM:
												if ( destroyed )
												{
													messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(2508), Language::get(2507));
												}
												updateEnemyBar(parent, hit.entity, Language::get(2507), hit.entity->furnitureHealth, hit.entity->furnitureMaxHealth,
													false, DamageGib::DMG_DEFAULT);
												break;
											default:
												break;
											}
										}
									}
									playSoundEntity(hit.entity, 28, 128);
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
						spellEffectAcid(*my, *element, parent, resistance);
					}
				}
				else if ( !strcmp(element->element_internal_name, spellElement_poison.element_internal_name) )
				{
					Entity* caster = uidToEntity(spell->caster);
					if ( caster )
					{
						spellEffectPoison(*my, *element, parent, resistance);
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
						spellEffectDrainSoul(*my, *element, parent, resistance);
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
					spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
				}

				if ( !(my->actmagicIsOrbiting == 2) )
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
			my->light = addLight(my->x / 16, my->y / 16, colorForSprite(my, my->sprite, false));

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
					my->light = addLight(my->x / 16, my->y / 16, colorForSprite(my, my->sprite, false));
				}
				else
				{
					my->removeLightField();
					my->light = addLight(my->x / 16, my->y / 16, colorForSprite(my, my->sprite, true));
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
		else if ( *cvar_magic_fx_use_vismap && !intro )
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
	else
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
}

void actMagicClient(Entity* my)
{
	my->removeLightField();
	my->light = addLight(my->x / 16, my->y / 16, colorForSprite(my, my->sprite, false));

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
			my->light = addLight(my->x / 16, my->y / 16, colorForSprite(my, my->sprite, false));
		}
		else
		{
			my->removeLightField();
			my->light = addLight(my->x / 16, my->y / 16, colorForSprite(my, my->sprite, true));
		}
		lightball_flicker = 0;
	}

	// spawn particles
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

void actMagicClientNoLight(Entity* my)
{
	// simply spawn particles
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

	if ( my->sprite == 1479 )
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

static ConsoleVariable<float> cvar_magic_fx_light_bonus("/magic_fx_light_bonus", 0.25f);

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

Entity* createParticleAestheticOrbit(Entity* parent, int sprite, int duration, int effectType)
{
	if ( !parent )
	{
		return nullptr;
	}
	Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Particle entity.
	entity->sizex = 1;
	entity->sizey = 1;
	entity->actmagicOrbitDist = 6;
	entity->yaw = parent->yaw;
	entity->x = parent->x + entity->actmagicOrbitDist * cos(entity->yaw);
	entity->y = parent->y + entity->actmagicOrbitDist * sin(entity->yaw);
	entity->z = parent->z;
	entity->skill[1] = effectType;
	entity->parent = parent->getUID();
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

void actParticleAestheticOrbit(Entity* my)
{
	if ( PARTICLE_LIFE < 0 )
	{
		list_RemoveNode(my->mynode);
	}
	else
	{
		Entity* parent = uidToEntity(my->parent);
		if ( !parent )
		{
			list_RemoveNode(my->mynode);
			return;
		}
		Stat* stats = parent->getStats();
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
			if ( my->sprite == 863 && (!stats || !stats->EFFECTS[EFF_WEBBED]) )
			{
				list_RemoveNode(my->mynode);
				return;
			}
			my->yaw += 0.2;
			spawnMagicParticle(my);
			my->x = parent->x + my->actmagicOrbitDist * cos(my->yaw);
			my->y = parent->y + my->actmagicOrbitDist * sin(my->yaw);
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

void createParticleErupt(Entity* parent, int sprite)
{
	if ( !parent )
	{
		return;
	}

	real_t yaw = 0;
	int numParticles = 8;
	for ( int c = 0; c < 8; c++ )
	{
		Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Particle entity.
		entity->sizex = 1;
		entity->sizey = 1;
		entity->x = parent->x;
		entity->y = parent->y;
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
					if ( parent->teleportRandom() )
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
											monsterStats->EFFECTS[EFF_CONFUSED] = true;
											monsterStats->EFFECTS_TIMERS[EFF_CONFUSED] = -1;
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
					my->light = addLight(my->x / 16, my->y / 16, colorForSprite(my, my->sprite, true));
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
					my->light = addLight(my->x / 16, my->y / 16, colorForSprite(my, my->sprite, true));
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
			else if ( my->particleTimerCountdownAction == PARTICLE_EFFECT_TELEPORT_PULL_TARGET_LOCATION )
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
								element->damage = 2;
								if ( Stat* stats = parent->getStats() )
								{
									element->damage += stats->getProficiency(PRO_MAGIC) / 10;
								}
								element->mana = 5;
							}

							node->deconstructor = &spellDeconstructor;
							node->size = sizeof(spell_t);

							--entity_uids;
							entity->setUID(-3);
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
						parent->addItemToMonsterInventory(item);
						Stat *myStats = parent->getStats();
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
							
							if ( (caster->getINT() + stats[caster->skill[2]]->getModifiedProficiency(PRO_MAGIC)) >= SKILL_LEVEL_EXPERT )
							{
								spawnSecondAlly = true;
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
	if ( z <= -5 || fabs(vel_z) < 0.01 )
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
				&& !(entity->isDamageableCollider() && entity->isColliderDamageableByMagic())
				&& !entity->isInertMimic()
				&& entity->behavior != &::actChest 
				&& entity->behavior != &::actFurniture )
			{
				continue;
			}
			if ( caster && !(svFlags & SV_FLAG_FRIENDLYFIRE) && caster->checkFriend(entity) )
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
		double missile_speed = 4 * ((double)(((spellElement_t*)(spell->elements.first->element))->mana) 
			/ ((spellElement_t*)(spell->elements.first->element))->overload_multiplier);
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
		double missile_speed = 4 * ((double)(((spellElement_t*)(spell->elements.first->element))->mana)
			/ ((spellElement_t*)(spell->elements.first->element))->overload_multiplier);
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
	bool amplify = false;
	if ( stats )
	{
		amplify = stats->EFFECTS[EFF_MAGICAMPLIFY];
		stats->EFFECTS[EFF_MAGICAMPLIFY] = false; // temporary skip amplify effects otherwise recursion.
	}
	Entity* entity = castSpell(parent->getUID(), spell, false, true);
	if ( stats )
	{
		stats->EFFECTS[EFF_MAGICAMPLIFY] = amplify;
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
		double missile_speed = 4 * ((double)(((spellElement_t*)(spell->elements.first->element))->mana)
			/ ((spellElement_t*)(spell->elements.first->element))->overload_multiplier);
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
		my->light = addLight(my->x / 16, my->y / 16, colorForSprite(my, my->sprite, true));

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
				if ( parent && parent->getStats() && parent->getStats()->EFFECTS[EFF_SHADOW_TAGGED] )
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
			if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) && parent->checkFriend(autoHitTarget) )
			{
				autoHit = false; // don't hit friendlies
			}
		}
	}
	Entity* orbit = castStationaryOrbitingMagicMissile(parent, spellID, x, y, 16.0, 0.0, 40);
	if ( orbit )
	{
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
