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
		case 1801: return "magic_blue_flicker";
		case 1818: return "magic_blue_flicker";
        case 625:
        case 173: return "magic_purple_flicker";
        default:
        case 669:
        case 680:
        case 174: return "magic_white_flicker";
        case 593:
        case 175: return "magic_black_flicker";
        case 678: return "magic_pink_flicker";
		case 1817: return "magic_pink_flicker";
		case 1886: return "magic_pink_flicker";
		case 1816: return "magic_green_flicker";
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
		case 1801: return "magic_blue";
		case 1818: return "magic_blue";
        case 625:
        case 173: return "magic_purple";
        default:
        case 669:
        case 680:
        case 174: return "magic_white";
        case 593:
        case 175: return "magic_black";
        case 678: return "magic_pink";
		case 1817: return "magic_pink";
		case 1886: return "magic_pink";
		case 1816: return "magic_green";
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
						my->x += (my->vel_x < MAGIC_LIGHTBALL_SPEEDLIMIT) ? my->vel_x : MAGIC_LIGHTBALL_SPEEDLIMIT;
						my->y += (my->vel_y < MAGIC_LIGHTBALL_SPEEDLIMIT) ? my->vel_y : MAGIC_LIGHTBALL_SPEEDLIMIT;
						
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
				lightball_movement_timer = LIGHTBALL_MOVE_DELAY;
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
			my->vel_x = cos(my->yaw) * 4;
			my->vel_y = sin(my->yaw) * 4;
			double dist = clipMove(&my->x, &my->y, my->vel_x, my->vel_y, my);

			unsigned int distance = sqrt(pow(my->x - lightball_player_startx, 2) + pow(my->y - lightball_player_starty, 2));
			if (distance > MAGICLIGHT_BALL_FOLLOW_DISTANCE * 2 || dist != sqrt(my->vel_x * my->vel_x + my->vel_y * my->vel_y))
			{
				magic_init = 1;
				lightball_lighting = 1;
				lightball_movement_timer = 0; //Start off at 0 so that it moves towards the player as soon as it's created (since it's created farther away from the player).
			}
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

static ConsoleVariable<bool> cvar_magic_fx_use_vismap("/magic_fx_use_vismap", true);
void magicOnEntityHit(Entity* parent, Entity* particle, Entity* hitentity, Stat* hitstats, Sint32 preResistanceDamage, Sint32 damage, Sint32 oldHP, int spellID, int selfCastUsingItem)
{
	if ( !hitentity  ) { return; }

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
					if ( hitstats->HP <= 0 )
					{
						Compendium_t::Events_t::eventUpdateWorld(hitentity->monsterAllyIndex, Compendium_t::CPDM_TRAP_FOLLOWERS_KILLED, category, 1);
					}
				}
			}
		}
	}
}

void magicSetResistance(Entity* entity, Entity* parent, int& resistance, real_t& damageMultiplier, DamageGib& dmgGib, int& trapResist)
{
	if ( entity )
	{
		resistance = Entity::getMagicResistance(entity->getStats());
		if ( (entity->behavior == &actMonster || entity->behavior == &actPlayer) && entity->getStats() )
		{
			damageMultiplier = Entity::getDamageTableMultiplier(entity, *entity->getStats(), DAMAGE_TABLE_MAGIC);
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
					my->processEntityWind();
					dist = clipMove(&my->x, &my->y, my->vel_x, my->vel_y, my); //normal flat projectiles
					hitFromSide = dist != sqrt(my->vel_x * my->vel_x + my->vel_y * my->vel_y);
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
						if ( hitstats->getEffectActive(EFF_NULL_MAGIC)
							&& !(!(svFlags & SV_FLAG_FRIENDLYFIRE) && parent && parent->checkFriend(hit.entity)) )
						{
							auto effectStrength = hitstats->getEffectActive(EFF_NULL_MAGIC);
							int duration = hitstats->EFFECTS_TIMERS[EFF_NULL_MAGIC];
							if ( effectStrength == 1 )
							{
								if ( hitstats->EFFECTS_TIMERS[EFF_NULL_MAGIC] > 0 )
								{
									hitstats->EFFECTS_TIMERS[EFF_NULL_MAGIC] = 1;
								}
							}
							else if ( effectStrength > 1 )
							{
								--effectStrength;
								hitstats->setEffectValueUnsafe(EFF_NULL_MAGIC, effectStrength);
								hit.entity->setEffect(EFF_NULL_MAGIC, effectStrength, hitstats->EFFECTS_TIMERS[EFF_NULL_MAGIC], false);
							}
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
							playSoundEntity(hit.entity, 166, 128);
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

				// check for magic resistance...
				// resistance stacks diminishingly
				int trapResist = 0;
				int resistance = 0;
				DamageGib dmgGib = DMG_DEFAULT;
				real_t damageMultiplier = 1.0;
				magicSetResistance(hit.entity, parent, resistance, damageMultiplier, dmgGib, trapResist);

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

				if (!strcmp(element->element_internal_name, spellElement_force.element_internal_name)
					|| !strcmp(element->element_internal_name, spellElementMap[SPELL_MERCURY_BOLT].element_internal_name)
					|| !strcmp(element->element_internal_name, spellElementMap[SPELL_LEAD_BOLT].element_internal_name)
					|| !strcmp(element->element_internal_name, spellElementMap[SPELL_SPORE_BOMB].element_internal_name)
					|| !strcmp(element->element_internal_name, spellElementMap[SPELL_MYCELIUM_BOMB].element_internal_name) )
				{
					if (hit.entity)
					{
						if ( mimic )
						{
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);
							hit.entity->chestHandleDamageMagic(damage, *my, parent);
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

							if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_MERCURY_BOLT].element_internal_name)
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
									Sint32 duration = 5 * TICKS_PER_SECOND;
									damage /= (1 + (int)resistance);
									if ( hit.entity->setEffect(EFF_SLOW, true, duration, false, true, false, false) )
									{
										playSoundEntity(hit.entity, 396 + local_rng.rand() % 3, 64);
									}

									duration = 5 * TICKS_PER_SECOND;
									damage /= (1 + (int)resistance);
									if ( hit.entity->setEffect(EFF_POISONED, true, duration, false, true, false, false) )
									{
										hitstats->poisonKiller = my->parent;
									}
								}

							}

							if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_SPORE_BOMB].element_internal_name) )
							{
								Sint32 duration = 6 * TICKS_PER_SECOND + 10;
								damage /= (1 + (int)resistance);
								if ( hit.entity->setEffect(EFF_SLOW, true, duration, false, true, false, false) )
								{
									playSoundEntity(hit.entity, 396 + local_rng.rand() % 3, 64);
								}

								duration = 6 * TICKS_PER_SECOND + 10;
								damage /= (1 + (int)resistance);
								if ( hit.entity->setEffect(EFF_POISONED, true, duration, false, true, false, false) )
								{
									hitstats->poisonKiller = my->parent;
								}
							}
							else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_MYCELIUM_BOMB].element_internal_name) )
							{
								Sint32 duration = 6 * TICKS_PER_SECOND + 10;
								damage /= (1 + (int)resistance);
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
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);
							hit.entity->doorHandleDamageMagic(damage, *my, parent);
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
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);
							hit.entity->colliderHandleDamageMagic(damage, *my, parent);
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
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);
							hit.entity->chestHandleDamageMagic(damage, *my, parent);
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
						else if ( hit.entity->behavior == &actDoor || hit.entity->behavior == &actIronDoor )
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
				else if (!strcmp(element->element_internal_name, spellElement_fire.element_internal_name)
					|| !strcmp(element->element_internal_name, "spell_element_flames") )
				{
					bool explode = !strcmp(element->element_internal_name, spellElement_fire.element_internal_name);
					if ( !(my->actmagicIsOrbiting == 2) )
					{
						if ( explode )
						{
							spawnExplosion(my->x, my->y, my->z);
						}
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
							if ( (spell->ID == SPELL_METEOR || spell->ID == SPELL_METEOR_SHOWER)
								&& !strcmp(element->element_internal_name, spellElement_fire.element_internal_name) )
							{
								int damage = element->damage;
								damage += (spellbookDamageBonus * damage);
								Entity* caster = uidToEntity(spell->caster);
								createSpellExplosionArea(spell->ID, caster, my->x, my->y, my->z, 16.0, damage, hit.entity);
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
								if ( explode )
								{
									spawnExplosion(my->x, my->y, my->z);
								}
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
						else if ( hit.entity->behavior == &actDoor || hit.entity->behavior == &actIronDoor )
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
							if ( (spell->ID == SPELL_METEOR || spell->ID == SPELL_METEOR_SHOWER)
								&& !strcmp(element->element_internal_name, spellElement_fire.element_internal_name) )
							{
								int damage = element->damage;
								damage += (spellbookDamageBonus * damage);
								Entity* caster = uidToEntity(spell->caster);
								createSpellExplosionArea(spell->ID, caster, my->x, my->y, my->z, 16.0, damage, hit.entity);
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
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);

							hit.entity->colliderHandleDamageMagic(damage, *my, parent);
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}
							if ( (spell->ID == SPELL_METEOR || spell->ID == SPELL_METEOR_SHOWER)
								&& !strcmp(element->element_internal_name, spellElement_fire.element_internal_name) )
							{
								int damage = element->damage;
								damage += (spellbookDamageBonus * damage);
								Entity* caster = uidToEntity(spell->caster);
								createSpellExplosionArea(spell->ID, caster, my->x, my->y, my->z, 16.0, damage, hit.entity);
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
							int damage = element->damage;
							damage += (spellbookDamageBonus * damage);
							damage /= (1 + (int)resistance);
							hit.entity->chestHandleDamageMagic(damage, *my, parent);
							if ( my->actmagicProjectileArc > 0 )
							{
								Entity* caster = uidToEntity(spell->caster);
								spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
							}
							if ( (spell->ID == SPELL_METEOR || spell->ID == SPELL_METEOR_SHOWER)
								&& !strcmp(element->element_internal_name, spellElement_fire.element_internal_name) )
							{
								int damage = element->damage;
								damage += (spellbookDamageBonus * damage);
								Entity* caster = uidToEntity(spell->caster);
								createSpellExplosionArea(spell->ID, caster, my->x, my->y, my->z, 16.0, damage, hit.entity);
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
							if ( (spell->ID == SPELL_METEOR || spell->ID == SPELL_METEOR_SHOWER)
								&& !strcmp(element->element_internal_name, spellElement_fire.element_internal_name) )
							{
								int damage = element->damage;
								damage += (spellbookDamageBonus * damage);
								Entity* caster = uidToEntity(spell->caster);
								createSpellExplosionArea(spell->ID, caster, my->x, my->y, my->z, 16.0, damage, hit.entity);
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

							Uint8 effectStrength = MAXPLAYERS + 1;
							if ( parent && parent->behavior == &actPlayer )
							{
								effectStrength = parent->skill[2] + 1;
							}
							else if ( parent && parent->monsterAllyGetPlayerLeader() )
							{
								effectStrength = parent->monsterAllyGetPlayerLeader()->skill[2] + 1;
							}
							if ( duration > 0 && hit.entity->setEffect(EFF_CONFUSED, effectStrength, duration, false) )
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
								hitstats->setEffectActive(EFF_SLOW, 1);
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
							hitstats->setEffectActive(EFF_SLOW, 1);
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
				else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_NUMBING_BOLT].element_internal_name) )
				{
					if ( hit.entity )
					{
						if ( (!mimic && hit.entity->behavior == &actMonster) || hit.entity->behavior == &actPlayer )
						{
							if ( hit.entity->setEffect(EFF_NUMBING_BOLT, true, 5 * TICKS_PER_SECOND, false) )
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
				else if ( !strcmp(element->element_internal_name, spellElement_weakness.element_internal_name) )
				{
					if ( hit.entity )
					{
						if ( (!mimic && hit.entity->behavior == &actMonster) || hit.entity->behavior == &actPlayer )
						{
							//playSoundEntity(hit.entity, 396 + local_rng.rand() % 3, 64);
							hitstats->setEffectActive(EFF_WEAKNESS, 1);
							hitstats->EFFECTS_TIMERS[EFF_WEAKNESS] = (element->duration * (((element->mana) / static_cast<double>(element->base_mana)) * element->overload_multiplier));
							hitstats->EFFECTS_TIMERS[EFF_WEAKNESS] /= (1 + (int)resistance);

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
									messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(6527), Language::get(6526), MSG_COMBAT);
								}
							}
							Uint32 color = makeColorRGB(255, 0, 0);
							if ( player >= 0 )
							{
								messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(6528));
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
				else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_HUMILIATE].element_internal_name)
					|| !strcmp(element->element_internal_name, spellElementMap[SPELL_MANA_BURST].element_internal_name)
					|| !strcmp(element->element_internal_name, spellElementMap[SPELL_LVL_DEATH].element_internal_name) )
				{
					playSoundEntity(my, 173, 128);
					if ( hit.entity )
					{
						int damage = element->damage;
						damage += (spellbookDamageBonus * damage);
						//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
						if ( (!mimic && hit.entity->behavior == &actMonster) || hit.entity->behavior == &actPlayer )
						{
							Entity* parent = uidToEntity(my->parent);
							playSoundEntity(my, 173, 64);
							playSoundEntity(hit.entity, 28, 128);
							int oldHP = hitstats->HP;

							if ( spell->ID == SPELL_HUMILIATE )
							{
								int percentHP = static_cast<int>(100.0 * hitstats->HP / hitstats->MAXHP);
								if ( percentHP < 33 )
								{
									damage = 0.1;
								}
								else if ( percentHP < 66 )
								{
									damage *= 0.2;
								}
								else if ( percentHP < 100 )
								{
									damage *= 0.3;
								}
							}
							else if ( spell->ID == SPELL_LVL_DEATH )
							{
								if ( parent && parent->getStats() )
								{
									damage *= std::max(0, (hitstats->LVL - parent->getStats()->LVL));
								}
								else
								{
									damage = 0;
								}
							}
							else if ( spell->ID == SPELL_MANA_BURST )
							{
								if ( parent )
								{
									Stat* parentStats = parent->getStats();
									if ( parentStats )
									{
										damage += parentStats->MP / 4;

										if ( parent->behavior == &actPlayer )
										{
											Uint32 color = makeColorRGB(255, 255, 0);
											messagePlayerColor(parent->skill[2], MESSAGE_STATUS, color, Language::get(621));
										}
										parent->modMP(-parentStats->MP);
										parent->modHP(-(parentStats->MAXHP / 4));
										if ( parentStats->sex == MALE )
										{
											parent->setObituary(Language::get(1528));
											parentStats->killer = KilledBy::FAILED_INVOCATION;
										}
										else
										{
											parent->setObituary(Language::get(1529));
											parentStats->killer = KilledBy::FAILED_INVOCATION;
										}
									}
								}
							}

							Sint32 preResistanceDamage = damage;
							damage *= damageMultiplier;
							damage /= (1 + (int)resistance);
							hit.entity->modHP(-damage);
							magicOnEntityHit(parent, my, hit.entity, hitstats, preResistanceDamage, damage, oldHP, spell ? spell->ID : SPELL_NONE);
							magicTrapOnHit(parent, hit.entity, hitstats, oldHP, spell ? spell->ID : SPELL_NONE);
							for ( i = 0; i < damage && i < 15; i += 2 )   //Spawn a gib for every two points of damage.
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
							if ( oldHP > 0 && hitstats->HP <= 0 && parent )
							{
								parent->awardXP(hit.entity, true, true);
								spawnBloodVialOnMonsterDeath(hit.entity, hitstats, parent);
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
						else if ( hit.entity->behavior == &actDoor || hit.entity->behavior == &actIronDoor )
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
									int duration = 10 * TICKS_PER_SECOND;
									duration /= (1 + (int)resistance);

									if ( hasgoggles )
									{
										if ( hit.entity->behavior == &actPlayer )
										{
											messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, makeColorRGB(0, 255, 0), Language::get(6088));
										}
									}
									else if ( hit.entity->setEffect(EFF_MAGIC_GREASE, true, duration, true) )
									{
										hit.entity->setEffect(EFF_GREASY, true, 5 * TICKS_PER_SECOND, false);
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

							if ( spell->ID == SPELL_GREASE_SPRAY )
							{
							}
							else
							{
								damage = std::max(2, damage);
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
				else if ( spell->ID == SPELL_FOCI_FIRE )
				{
					if ( hit.entity )
					{
						int volume = 128;
						int hitsfx = 173;
						int hitvolume = (particleEmitterHitProps && particleEmitterHitProps->hits == 1) ? 128 : 64;
						if ( particleEmitterHitProps && particleEmitterHitProps->hits > 1 )
						{
							volume = 0;
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
							if ( spell->ID == SPELL_FOCI_FIRE )
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

							//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
							int oldHP = hitstats->HP;
							Sint32 preResistanceDamage = damage;
							damage *= damageMultiplier;
							damage /= (1 + (int)resistance);

							if ( spell->ID == SPELL_FOCI_FIRE )
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
						else if ( hit.entity->behavior == &actDoor || hit.entity->behavior == &actIronDoor )
						{
							hit.entity->SetEntityOnFire();
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
							hit.entity->SetEntityOnFire();
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
							hit.entity->SetEntityOnFire();
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
							hit.entity->SetEntityOnFire();
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
									int duration = dizzy ? TICKS_PER_SECOND * 1.5 : TICKS_PER_SECOND / 2;
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
											createParticleSpin(hit.entity);
											serverSpawnMiscParticles(hit.entity, PARTICLE_EFFECT_SPIN, -1);
											if ( caster )
											{
												messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(0, 255, 0),
													*hitstats, Language::get(6706), Language::get(6707), MSG_COMBAT);
											}
										}
									}

									if ( !dizzy && hit.entity->setEffect(EFF_DISORIENTED, true, duration, false) )
									{
										effect = true;
										hit.entity->monsterReleaseAttackTarget();
										if ( caster )
										{
											hit.entity->lookAtEntity(*caster);
										}
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
							real_t pushbackMultiplier = 0.6;// +(0.2 * spellbookDamageBonus);
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
									if ( !hitstats->getEffectActive(EFF_MIMIC_LOCKED) )
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
										if ( hitstats->getEffectActive(EFF_MIMIC_LOCKED) )
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
							bool wasBleeding = hit.entity->getStats() ? hit.entity->getStats()->getEffectActive(EFF_BLEEDING) : false;
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
							hitstats->setEffectActive(EFF_SLOW, 1);
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

					createRadiusMagic(spell->ID, uidToEntity(spell->caster), spawnx, spawny, 24, 5 * TICKS_PER_SECOND, follow);
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
									&& hit.entity->setEffect(EFF_BLIND, true, 3 * TICKS_PER_SECOND, false) )
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

					entity->skill[12] = TICKS_PER_SECOND * 10;
					node_t* spellnode = list_AddNodeLast(&entity->children);
					spellnode->element = copySpell(allGameSpells[spell->ID == SPELL_SHADE_BOLT ? SPELL_DEEP_SHADE : SPELL_LIGHT]); //We need to save the spell since this is a channeled spell.
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
						int damage = element->damage;
						damage += (spellbookDamageBonus * damage);
						floorMagicCreateSpores(hit.entity, my->x, my->y, parent, damage, spell->ID);
					}
					else
					{
						spawnMagicTower(caster, my->x, my->y, spell->ID, nullptr, true);
					}
				}

				if ( (spell->ID == SPELL_METEOR || spell->ID == SPELL_METEOR_SHOWER)
					&& !strcmp(element->element_internal_name, spellElement_fire.element_internal_name) )
				{
					int damage = element->damage;
					damage += (spellbookDamageBonus * damage);
					Entity* caster = uidToEntity(spell->caster);
					createSpellExplosionArea(spell->ID, caster, my->x, my->y, my->z, 16.0, damage, hit.entity);
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
			if ( !my->flags[INVISIBLE] || my->flags[INVISIBLE_DITHER] )
			{
				my->light = addLight(my->x / 16, my->y / 16, colorForSprite(my, my->sprite, false));
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
						my->light = addLight(my->x / 16, my->y / 16, colorForSprite(my, my->sprite, false));
					}
				}
				else
				{
					my->removeLightField();
					if ( !my->flags[INVISIBLE] || my->flags[INVISIBLE_DITHER] )
					{
						my->light = addLight(my->x / 16, my->y / 16, colorForSprite(my, my->sprite, true));
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
}

void actMagicClient(Entity* my)
{
	my->removeLightField();
	if ( !my->flags[INVISIBLE] || my->flags[INVISIBLE_DITHER] )
	{
		my->light = addLight(my->x / 16, my->y / 16, colorForSprite(my, my->sprite, false));
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
				my->light = addLight(my->x / 16, my->y / 16, colorForSprite(my, my->sprite, false));
			}
		}
		else
		{
			my->removeLightField();
			if ( !my->flags[INVISIBLE] || my->flags[INVISIBLE_DITHER] )
			{
				my->light = addLight(my->x / 16, my->y / 16, colorForSprite(my, my->sprite, true));
			}
		}
		lightball_flicker = 0;
	}

	// spawn particles
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
	else if ( my->sprite == 1866 )
	{
		my->scalex -= 0.01;
		my->scaley -= 0.01;
		my->scalez -= 0.01;
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

	if ( parent->behavior == &actPlayer && players[parent->skill[2]]->isLocalPlayer() && parent->skill[3] == 0 )
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
		entity->skill[11] = parent->behavior == &actPlayer ? parent->skill[2] : -1;
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
	if ( effectType == PARTICLE_EFFECT_NULL_PARTICLE )
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
		if ( multiplayer != CLIENT && my->skill[10] > 0 )
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
				|| my->skill[1] == PARTICLE_EFFECT_SHATTER_EARTH_ORBIT )
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
		else if ( my->skill[1] == PARTICLE_EFFECT_MUSHROOM_SPELL )
		{
			int mapx = static_cast<int>(my->x) >> 4;
			int mapy = static_cast<int>(my->y) >> 4;
			int mapIndex = (mapy)*MAPLAYERS + (mapx)*MAPLAYERS * map.height;
			if ( mapx > 0 && mapy > 0 && mapx < map.width - 1 && mapy < map.width - 1 )
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
			Entity* fx = spawnMagicParticle(my);
			fx->scalex = my->scalex * 0.9;
			fx->scaley = my->scaley * 0.9;
			fx->scalez = my->scalez * 0.9;
			my->x = parent->x + my->actmagicOrbitDist * cos(my->yaw);
			my->y = parent->y + my->actmagicOrbitDist * sin(my->yaw);
		}
		else if ( my->skill[1] == PARTICLE_EFFECT_SHATTER_EARTH_ORBIT )
		{
			//my->yaw += 0.2;
			Entity* fx = spawnMagicParticle(my);
			fx->scalex = my->scalex * 0.9;
			fx->scaley = my->scaley * 0.9;
			fx->scalez = my->scalez * 0.9;
			my->x = my->actmagicOrbitStationaryX + my->actmagicOrbitDist * cos(my->yaw);
			my->y = my->actmagicOrbitStationaryY + my->actmagicOrbitDist * sin(my->yaw);
			my->z += my->vel_z;
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
				my->light = addLight(my->x / 16, my->y / 16, "orb_blue");
			}

			if ( anim == 0 || anim == 2 || anim >= 4 )
			{
				my->flags[INVISIBLE] = false;
			}
			if ( my->ticks % 4 == 0 )
			{
				my->sprite++;
				my->yaw += 1 * PI / 3;
				if ( my->sprite > 1763 )
				{
					my->sprite = 1758;
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
		else if ( my->skill[1] == PARTICLE_EFFECT_NULL_PARTICLE )
		{
			if ( my->ticks == 1 )
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
		|| fx.actfloorMagicType == ParticleTimerEffect_t::EFFECT_ROOTS_TILE )
	{
		val |= (((Uint16)(fx.skill[0]) & 0xFFF) << 8);
	}

	val |= (Uint8)(fx.actfloorMagicType & 0xFF) << 20;
	fx.skill[2] = val;
}

bool floorMagicCreateRoots(real_t x, real_t y, Entity* caster, int damage, int spellID, int duration, int particleTimerAction)
{
	int mapx = static_cast<int>(x) >> 4;
	int mapy = static_cast<int>(y) >> 4;
	int mapIndex = (mapy)*MAPLAYERS + (mapx) * MAPLAYERS * map.height;
	if ( mapx > 0 && mapy > 0 && mapx < map.width - 1 && mapy < map.width - 1 )
	{
		if ( !map.tiles[mapIndex] 
			|| swimmingtiles[map.tiles[mapIndex]]
				|| lavatiles[map.tiles[mapIndex]] )
		{
			return false;
		}
	}
	else
	{
		return false;
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
	return true;
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
	if ( mapx > 0 && mapy > 0 && mapx < map.width - 1 && mapy < map.width - 1 )
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

	int lifetime_tick = startTickOffset == 0 ? 2 * TICKS_PER_SECOND : startTickOffset;

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
											monsterStats->setEffectActive(EFF_CONFUSED, MAXPLAYERS + 1);
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
						my->light = addLight(my->x / 16, my->y / 16, colorForSprite(my, my->sprite, true));
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
						my->light = addLight(my->x / 16, my->y / 16, colorForSprite(my, my->sprite, true));
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
									element->damage = 0;
								}
								else
								{
									element->damage = 2;
									if ( Stat* stats = parent->getStats() )
									{
										element->damage += stats->getProficiency(PRO_MAGIC) / 10;
									}
								}
								element->mana = 5;
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
					int sound = 164;
					int spellID = SPELL_FOCI_FIRE;
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

					static ConsoleVariable<float> cvar_foci_vel("/foci_vel", 1.f);
					static ConsoleVariable<float> cvar_foci_vel_decay("/foci_vel_decay", 0.95);
					static ConsoleVariable<float> cvar_foci_life("/foci_life", 1.f);
					static ConsoleVariable<float> cvar_foci_spread("/foci_spread", 1.f);
					static ConsoleVariable<int> cvar_foci_delay("/foci_delay", 0);
					static ConsoleVariable<bool> cvar_foci_model("/foci_model", false);
					static ConsoleVariable<float> cvar_foci_scale("/foci_scale", 1.f);
					static ConsoleVariable<float> cvar_foci_shrink("/foci_shrink", 0.1);
					static ConsoleVariable<float> cvar_foci_grid("/foci_grid", 0.0);
					static ConsoleVariable<float> cvar_foci_osc_h("/foci_osc_h", 0.0);
					static ConsoleVariable<float> cvar_foci_swirl("/foci_swirl", 0.2);

					Entity* entity = nullptr;
					if ( multiplayer != CLIENT && my->ticks % 2 == 0 && false )
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
						int lifetime = (0.4 * TICKS_PER_SECOND) + (local_rng.rand() % 5);
						entity->sprite = my->particleTimerCountdownSprite;
						entity->x = parent->x;
						entity->y = parent->y;
						entity->z = parent->z;
						entity->parent = parent->getUID();

						entity->ditheringDisabled = true;
						entity->flags[SPRITE] = !*cvar_foci_model;
						entity->flags[INVISIBLE] = false;
						entity->flags[PASSABLE] = true;
						entity->flags[NOUPDATE] = true;
						entity->flags[UNCLICKABLE] = true;
						entity->flags[BRIGHT] = true;

						entity->sizex = 2;
						entity->sizey = 2;
						real_t spread = 0.2 * *cvar_foci_spread;
						entity->yaw = my->yaw - spread + ((local_rng.rand() % 21) * (spread / 10));
						entity->pitch = 0.0; //(local_rng.rand() % 360)* PI / 180.0;
						entity->roll = 0.0; //(local_rng.rand() % 360)* PI / 180.0;
						double vel = *cvar_foci_vel * (20 + (local_rng.rand() % 5)) / 10.f;
						entity->vel_x = vel * cos(entity->yaw) + parent->vel_x;
						entity->vel_y = vel * sin(entity->yaw) + parent->vel_y;
						entity->vel_z = .6;
						entity->scalex = *cvar_foci_scale;
						entity->scaley = *cvar_foci_scale;
						entity->scalez = *cvar_foci_scale;
						real_t gravity = -0.035 - 0.002 * (local_rng.rand() % 6);

						//entity->vel_x = 0.0;
						//entity->vel_y = 0.0;
						//entity->x += 32.0 * cos((my->ticks % 10 / 10.0) * 2 * PI);
						//entity->y += 32.0 * sin((my->ticks % 10 / 10.0) * 2 * PI);

						if ( *cvar_foci_grid > 0.0001 )
						{
							entity->x += 2.0 * cos(my->yaw);
							entity->y += 2.0 * sin(my->yaw);

							int gridSize = 3;
							int grid_x = -gridSize / 2;
							int grid_y = -gridSize / 2;
							int tempTicks = (my->ticks - 1);
							while ( tempTicks > 0 )
							{
								grid_x++;
								if ( grid_x > gridSize / 2 )
								{
									grid_x = -gridSize / 2;
									grid_y++;
								}
								--tempTicks;
							}

							entity->x += *cvar_foci_grid * grid_x * cos(my->yaw + PI / 2);
							entity->y += *cvar_foci_grid * grid_x * sin(my->yaw + PI / 2);
							entity->z += *cvar_foci_grid * grid_y;
						}

						//entity->vel_z = 0.0;
						//real_t gravity = 0.0;

						// swirl
						if ( *cvar_foci_swirl > 0.0001 )
						{
							entity->yaw += (my->ticks % 10) * (2 * PI / 5);
							lifetime += 5;
						}
						lifetime *= *cvar_foci_life;
						if ( entity->behavior == &actGib )
						{
							entity->fskill[3] = gravity;
							if ( my->ticks % 5 == 0 )
							{
								// add lighting
								entity->skill[6] = 1;
							}
							entity->skill[4] = lifetime;
							entity->skill[8] = *cvar_foci_delay; // delay velx/y movement
							entity->fskill[4] = *cvar_foci_shrink * *cvar_foci_scale; // shrink at end of life
							entity->fskill[5] = *cvar_foci_swirl;
							entity->fskill[6] = *cvar_foci_osc_h * ((my->ticks % 2) ? 1 : -1);
							entity->fskill[7] = *cvar_foci_vel_decay;
						}
						else if ( entity->behavior == &actMagicMissile )
						{
							spell_t* spell = getSpellFromID(spellID);
							entity->skill[4] = 0; // life start
							entity->skill[5] = lifetime; //lifetime
							entity->actmagicSpray = 1;
							entity->actmagicSprayGravity = gravity;
							entity->actmagicEmitter = my->getUID();
							node_t* node = list_AddNodeFirst(&entity->children);
							node->element = copySpell(spell);
							((spell_t*)node->element)->caster = parent->getUID();
							node_t* elementNode = ((spell_t*)node->element)->elements.first;
							spellElement_t* element = (spellElement_t*)elementNode->element;
							{
								elementNode = element->elements.first;
								element = (spellElement_t*)elementNode->element;
								static ConsoleVariable<int> cvar_foci_dmg("/foci_dmg", 2);
								element->damage = *cvar_foci_dmg;
								/*if ( Stat* stats = parent->getStats() )
								{
									element->damage += stats->getProficiency(PRO_MAGIC) / 10;
								}*/
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
							for ( int i = 0; i < 32; ++i )
							{
								/*if ( Entity* particle = spawnMagicParticleCustom(my, 13, 1.0, 4) )
								{
									particle->x += 32 * cos(my->yaw + i * 2 * PI / 32.0);
									particle->y += 32 * sin(my->yaw + i * 2 * PI / 32.0);
									particle->lightBonus = vec4(0.5f, 0.5f, 0.5f, 0.f);
									particle->vel_z = -0.05;
									particle->flags[SPRITE] = true;
									particle->ditheringDisabled = true;
								}*/
								if ( Entity* entity = spawnFlame(my, SPRITE_FLAME) )
								{
									//entity->sprite = 16;
									double vel = local_rng.rand() % 10;
									entity->x += 64 * cos(my->yaw + i * 2 * PI / 32.0);
									entity->y += 64 * sin(my->yaw + i * 2 * PI / 32.0);
									entity->vel_x = vel * cos(entity->yaw) * cos(entity->pitch) * .1;
									entity->vel_y = vel * sin(entity->yaw) * cos(entity->pitch) * .1;
									entity->vel_z = vel * sin(entity->pitch) * .2;
									entity->skill[0] = 5 + local_rng.rand() % 10;
								}
							}

							if ( multiplayer != CLIENT )
							{
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
										if ( entityDist(caster, entity) > 64.0 )
										{
											continue;
										}
										if ( Stat* stats = entity->getStats() )
										{
											if ( caster->checkEnemy(entity) )
											{
												entity->SetEntityOnFire(caster);
												stats->burningInflictedBy = caster->getUID();
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
												entity->SetEntityOnFire(caster);
											}
										}
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

											int damage = getSpellDamageFromID(my->particleTimerVariable2, parent ? parent : my);
											real_t mult = 0.5 + 0.1 * (std::min((Uint8)10, effectStrength));
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

								auto props = getParticleEmitterHitProps(my->getUID(), entity);
								if ( !props )
								{
									continue;
								}
								if ( props->hits > 0 && (ticks - props->tick) < 20 )
								{
									continue;
								}
								Uint8 strength = std::min(10, 1 + stats->getEffectActive(EFF_LIFT));
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
							for ( int i = 0; i < 32; ++i )
							{
								/*if ( Entity* particle = spawnMagicParticleCustom(my, 13, 1.0, 4) )
								{
									particle->x += 32 * cos(my->yaw + i * 2 * PI / 32.0);
									particle->y += 32 * sin(my->yaw + i * 2 * PI / 32.0);
									particle->lightBonus = vec4(0.5f, 0.5f, 0.5f, 0.f);
									particle->vel_z = -0.05;
									particle->flags[SPRITE] = true;
									particle->ditheringDisabled = true;
								}*/
								if ( Entity* entity = spawnFlame(my, SPRITE_FLAME) )
								{
									//entity->sprite = 16;
									double vel = local_rng.rand() % 10;
									entity->x += 64 * cos(my->yaw + i * 2 * PI / 32.0);
									entity->y += 64 * sin(my->yaw + i * 2 * PI / 32.0);
									entity->vel_x = vel * cos(entity->yaw) * cos(entity->pitch) * .1;
									entity->vel_y = vel * sin(entity->yaw) * cos(entity->pitch) * .1;
									entity->vel_z = vel * sin(entity->pitch) * .2;
									entity->skill[0] = 5 + local_rng.rand() % 10;
								}
							}

							if ( multiplayer != CLIENT )
							{
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
										if ( entityDist(caster, entity) > 64.0 )
										{
											continue;
										}
										bool mimic = entity->isInertMimic();
										if ( Stat* stats = entity->getStats() )
										{
											if ( !mimic )
											{
												continue;
											}
										}

										int damage = 10;
										if ( entity->behavior == &actDoor
											|| entity->behavior == &::actIronDoor )
										{
											entity->doorHandleDamageMagic(damage, *my, caster);
										}
										else if ( entity->behavior == &::actChest || mimic )
										{
											entity->chestHandleDamageMagic(damage, *my, caster);
										}
										else if ( entity->isDamageableCollider() && entity->isColliderDamageableByMagic() )
										{
											entity->colliderHandleDamageMagic(damage, *my, caster);
										}
										else if ( entity->behavior == &::actFurniture )
										{
											entity->furnitureHandleDamageMagic(damage, *my, caster);
										}
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
				if ( PARTICLE_LIFE == 1 )
				{
					Entity* caster = uidToEntity(my->parent);
					doSpellExplosionArea(my->particleTimerVariable2, my, caster, my->x, my->y, my->z, my->particleTimerVariable3);
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
									if ( parent->checkFriend(entity) )
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
								int damage = std::min(30, std::max(5, statGetCON(parentStats, parent)));
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
							}
							else if ( target->behavior == &::actChest || target->getMonsterTypeFromSprite() == MIMIC )
							{
								target->chestHandleDamageMagic(my->particleTimerVariable1, *my, caster);
							}
							else if ( target->isDamageableCollider() )
							{
								target->colliderHandleDamageMagic(target->colliderCurrentHP, *my, caster);
							}
							else if ( target->behavior == &::actFurniture )
							{
								target->furnitureHandleDamageMagic(target->furnitureHealth, *my, caster);
							}
						}
					}
				}
				if ( PARTICLE_LIFE == 5 )
				{
					if ( multiplayer != CLIENT )
					{
						spawnExplosion(my->x, my->y, my->z);
						int trapResist = 0;
						int resistance = 0;
						DamageGib dmgGib = DMG_DEFAULT;
						real_t damageMultiplier = 1.0;
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
								if ( entityDist(my, entity) > 32.0 )
								{
									continue;
								}
								if ( entity == target || entity == my )
								{
									continue;
								}
								bool mimic = entity->isInertMimic();
								if ( caster && caster->getStats() )
								{
									if ( svFlags & SV_FLAG_FRIENDLYFIRE )
									{
										if ( caster->checkFriend(entity) )
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
										entity->SetEntityOnFire(caster);
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
						real_t speedMult = 100.0 - 0.5 * std::min((Uint32)100, my->ticks); // build up faster to impact
						if ( my->ticks >= 100 ) // first impact
						{
							if ( my->ticks == 100 )
							{
								if ( dist < 16.0 )
								{
									my->actmagicOrbitHitTargetUID2 = closestEntity->getUID(); // follow this target
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
								my->vel_x = speed * cos(tangent);
								my->vel_y = speed * sin(tangent);
							}
						}
						else
						{
							real_t speed = std::min(dist, std::max(16.0, 64.0 - dist) / speedMult);
							my->vel_x = speed * cos(tangent);
							my->vel_y = speed * sin(tangent);
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
								playSoundEntityLocal(fx, data.sfx, 64);
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
					if ( x > 0 && y > 0 && x < map.width - 1 && y < map.width - 1 )
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
						if ( x > 0 && y > 0 && x < map.width - 1 && y < map.width - 1
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
						if ( x > 0 && y > 0 && x < map.width - 1 && y < map.width - 1
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
							Entity* fx = createFloorMagic(data.effectType, my->particleTimerCountdownSprite, data.x, data.y, 7.8, data.yaw, PARTICLE_LIFE + 3 * TICKS_PER_SECOND);
							fx->parent = my->getUID();
							fx->sizex = 6;
							fx->sizey = 6;
							if ( data.sfx )
							{
								playSoundEntity(fx, data.sfx, 64);
							}
							fx->actmagicNoParticle = 1;
							floorMagicParticleSetUID(*fx, true);
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
		int numParticles = 8;
		for ( int c = 0; c < 8; c++ )
		{
			Entity* entity = newEntity(my->skill[5], 1, map.entities, nullptr); //Particle entity.
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
			entity->scalex = 0.1;
			entity->scaley = 0.1;
			entity->scalez = 0.1;
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
		--PARTICLE_LIFE;
		my->removeLightField();
		my->light = addLight(my->x / 16, my->y / 16, colorForSprite(my, my->sprite, true));

		Entity* parent = uidToEntity(my->parent);
		if ( parent )
		{
			my->x = parent->x;
			my->y = parent->y;
			if ( parent->getEntityShowOnMapDuration() == 0
				|| (parent->getEntityShowOnMapSource() == Entity::SHOW_MAP_SCRY) )
			{
				parent->setEntityShowOnMap(Entity::SHOW_MAP_SCRY, std::max(parent->getEntityShowOnMapDuration(), 5));
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
							PARTICLE_LIFE = 0;
							if ( multiplayer != CLIENT )
							{
								parent->setEffect(SPELL_PENANCE, false, 0, true);
							}
						}
						if ( Stat* parentStats = parent->getStats() )
						{
							if ( !parentStats->getEffectActive(EFF_PENANCE) )
							{
								PARTICLE_LIFE = 0;
							}
						}
					}
					else if ( spellID == SPELL_DETECT_ENEMY || spellID == SPELL_DETECT_ENEMIES )
					{
						if ( Stat* parentStats = parent->getStats() )
						{
							if ( !parentStats->getEffectActive(EFF_DETECT_ENEMY) )
							{
								PARTICLE_LIFE = 0;
							}
						}
					}
					else if ( spellID == SPELL_TABOO )
					{
						if ( Stat* parentStats = parent->getStats() )
						{
							if ( !parentStats->getEffectActive(EFF_TABOO) )
							{
								PARTICLE_LIFE = 0;
							}
						}
					}
				}
				else
				{
					PARTICLE_LIFE = 0;
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
			if ( my->scalex < 0.35 )
			{
				my->scalex += 0.02;
			}
			else
			{
				my->scalex = 0.35;
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
				Entity* entity = newEntity(my->skill[5], 1, map.entities, nullptr); //Particle entity.
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
				entity->scalex = 0.1;
				entity->scaley = 0.1;
				entity->scalez = 0.1;
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
	entity->skill[5] = sprite + (PINPOINT_PARTICLE_END - PINPOINT_PARTICLE_START); // start/end particle drops
	entity->behavior = &actParticlePinpointTarget;
	entity->ditheringOverride = 6;
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->flags[ENTITY_SKIP_CULLING] = true;
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
	int mapIndex = (y)*MAPLAYERS + (x)*MAPLAYERS * map.height;
	if ( !map.tiles[mapIndex] || swimmingtiles[map.tiles[mapIndex]] || lavatiles[map.tiles[mapIndex]] || map.tiles[OBSTACLELAYER + mapIndex] )
	{
		my->flags[INVISIBLE] = true;
		list_RemoveNode(my->mynode);
		return;
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
		--PARTICLE_LIFE;
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
				Entity* particle = spawnMagicParticleCustom(my, 226, .7, 1.0);
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
		else if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_TILE )
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
				Entity* root = createParticleRoot(1766, my->x + dist * cos(yaw), my->y + dist * sin(yaw),
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
		else if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_SELF )
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
					if ( parentTimer->particleTimerVariable3 == 0 )
					{
						if ( Entity* breakable = Entity::createBreakableCollider(EditorEntityData_t::getColliderIndexFromName("mushroom_spell_fragile"),
							my->x, my->y, caster) )
						{
							parentTimer->particleTimerVariable3 = 1;
							breakable->colliderSpellEvent = 1 + local_rng.rand() % 5;
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
				node_t* node;
				for ( node = it->first; node != nullptr; node = node->next )
				{
					Entity* entity = (Entity*)node->element;
					if ( entity->behavior == &actPlayer || (entity->behavior == &actMonster && !entity->isInertMimic()) )
					{
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
							Stat* stats = entity->getStats();
							if ( stats && entityInsideEntity(my, entity) )
							{
								if ( !entity->monsterIsTargetable() ) { continue; }
								if ( (caster && !caster->checkFriend(entity) || !caster) )
								{
									bool effected = false;
									if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_SPORES )
									{
										if ( entity->setEffect(EFF_POISONED, true, 3 * TICKS_PER_SECOND + 10, false, true, false, false) )
										{
											effected = true;
											stats->poisonKiller = caster ? caster->getUID() : 0;
										}
										if ( entity->setEffect(EFF_SLOW, true, 3 * TICKS_PER_SECOND + 10, false, true, false, false) )
										{
											effected = true;
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
							Stat* stats = entity->getStats();
							if ( stats && entityDist(my, entity) <= 16.0 )
							{
								int damage = getSpellDamageFromID(SPELL_LIGHTNING_BOLT, caster);
								if ( applyGenericMagicDamage(caster, entity, caster ? *caster : *my, SPELL_LIGHTNING_BOLT, damage, true, true) )
								{
									Entity* fx = createParticleAestheticOrbit(entity, 1758, 2 * TICKS_PER_SECOND, PARTICLE_EFFECT_STATIC_ORBIT);
									fx->z = 7.5;
									fx->actmagicOrbitDist = 20;
									fx->actmagicNoLight = 1;
									serverSpawnMiscParticles(entity, PARTICLE_EFFECT_STATIC_ORBIT, 1758);
								}

								particleEmitterHitPropsTimer->hits++;
								particleEmitterHitPropsTimer->tick = ticks;
							}
						}
						else if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_DISRUPT_EARTH )
						{
							auto particleEmitterHitPropsTimer = getParticleEmitterHitProps(my->parent, entity);
							if ( !particleEmitterHitPropsTimer )
							{
								continue;
							}
							Stat* stats = entity->getStats();
							if ( particleEmitterHitPropsTimer->hits > 0 && ((ticks - particleEmitterHitPropsTimer->tick) < 20) )
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
							if ( stats && entityInsideEntity(my, entity) )
							{
								if ( !entity->monsterIsTargetable() ) { continue; }
								int damage = 0;
								if ( particleEmitterHitPropsTimer->hits == 0 || particleEmitterHitPropsTimer->hits == 3 )
								{
									damage = 5;
								}

								if ( damage > 0 )
								{
									if ( applyGenericMagicDamage(caster, entity, caster ? *caster : *my, SPELL_DISRUPT_EARTH, damage, true, true) )
									{
										if ( particleEmitterHitPropsTimer->hits > 0 )
										{
											if ( entity->setEffect(EFF_ROOTED, true, TICKS_PER_SECOND * 1, false) )
											{
												spawnMagicEffectParticles(entity->x, entity->y, entity->z, 1758);
											}
										}
									}
								}

								particleEmitterHitPropsTimer->hits++;
								particleEmitterHitPropsTimer->tick = ticks;
							}
						}
						else if ( my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_SELF
							|| my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_TILE
							|| my->actfloorMagicType == ParticleTimerEffect_t::EffectType::EFFECT_ROOTS_PATH )
						{
							auto particleEmitterHitPropsTimer = getParticleEmitterHitProps(my->parent, entity);
							if ( !particleEmitterHitPropsTimer )
							{
								continue;
							}
							if ( particleEmitterHitPropsTimer->hits > 0 && (ticks - particleEmitterHitPropsTimer->tick) < 1.25 * TICKS_PER_SECOND )
							{
								continue;
							}

							Stat* stats = entity->getStats();
							if ( stats && entityDist(my, entity) < radius * 16.0 + 16.0 )
							{
								if ( !entity->monsterIsTargetable() || entity == caster ) { continue; }
								if ( caster && !caster->checkFriend(entity) )
								{
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
											damage = getSpellDamageFromID(SPELL_ROOTS, caster);
										}

										if ( applyGenericMagicDamage(caster, entity, caster ? *caster : *my, parentTimer->particleTimerVariable1, damage, true, true) )
										{

										}
										if ( entity->setEffect(EFF_ROOTED, true, TICKS_PER_SECOND, false) )
										{
											spawnMagicEffectParticles(entity->x, entity->y, entity->z, 1758);
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
							Stat* stats = entity->getStats();
							if ( stats && entityInsideEntity(my, entity) )
							{
								if ( !entity->monsterIsTargetable() ) { continue; }
								int damage = 1;
								int duration = TICKS_PER_SECOND;
								Uint8 strength = 0;
								if ( particleEmitterHitPropsFloorMagic->hits == 0 && my->skill[1] < 10 )
								{
									strength = 1;
									damage = 10; // big damage region
									duration = TICKS_PER_SECOND * 3;
								}
								else
								{
									strength = std::min(8, stats->getEffectActive(EFF_SLOW) + 1);
									damage += strength / 2;
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
			int chronomicLimit = 4;
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
								entity->setEffect(EFF_DASH, true, 60, true);
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
							}
							else if ( entity->behavior == &actMonster )
							{
								particleEmitterHitProps->hits++;
								particleEmitterHitProps->tick = ticks;
								if ( entity->setEffect(EFF_KNOCKBACK, true, 30, false) )
								{
									playSoundEntity(entity, 180, 128);
									spawnMagicEffectParticles(entity->x, entity->y, entity->z, 982);
									entity->setEffect(EFF_DASH, true, 15, true);

									real_t push = 1.5;
									entity->vel_x = cos(dir) * push;
									entity->vel_y = sin(dir) * push;
									entity->monsterKnockbackVelocity = 0.01;
									entity->monsterKnockbackTangentDir = dir;
									entity->monsterKnockbackUID = parentTimer->parent;
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
						if ( particleEmitterHitProps->hits >= 3 || (particleEmitterHitProps->hits > 0 && ((ticks - particleEmitterHitProps->tick) < 20)) )
						{
							continue;
						}

						int damage = 5;
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
												if ( applyGenericMagicDamage(caster, entity, caster ? *caster : *my, SPELL_FIRE_WALL, damage, true, true) )
												{
													if ( entity->flags[BURNABLE] && !entity->flags[BURNING] )
													{
														if ( local_rng.rand() % 10 < (particleEmitterHitProps->hits + 1) )
														{
															entity->SetEntityOnFire(caster);
														}
													}
												}
												particleEmitterHitProps->hits++;
												particleEmitterHitProps->tick = ticks;
											}
											else if ( stats )
											{
												if ( !entity->monsterIsTargetable() ) { continue; }

												if ( applyGenericMagicDamage(caster, entity, caster ? *caster : *my, SPELL_FIRE_WALL, damage, true, true) )
												{
													particleEmitterHitProps->hits++;
													particleEmitterHitProps->tick = ticks;

													if ( entity->flags[BURNABLE] && !entity->flags[BURNING] )
													{
														if ( local_rng.rand() % 10 < (particleEmitterHitProps->hits + 1) )
														{
															entity->SetEntityOnFire(caster);
														}
													}

													if ( !stats->getEffectActive(EFF_KNOCKBACK)
														&& !entity->flags[BURNING]
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
			}
		}
	}
	return spellTimer;
}

Entity* createTunnelPortal(real_t x, real_t y, int duration, int dir)
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

	if ( checkObstacle((checkx << 4) + 8, (checky << 4) + 8, nullptr, nullptr, true, true, false, false) )
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
	int walls = 0;
	while ( true )
	{
		bool goodspot = false;
		if ( checkx > 0 && checkx < map.width - 1 && checky > 0 && checky < map.height - 1 )
		{
			int mapIndex = (checky)*MAPLAYERS + (checkx) * MAPLAYERS * map.height;
			if ( !map.tiles[OBSTACLELAYER + mapIndex] )
			{
				if ( !checkObstacle((checkx << 4) + 8, (checky << 4) + 8, nullptr, nullptr, true, true, false, false) )
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
	portal->z = 0.0;
	portal->yaw = (dir + 2) * PI / 2;
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
	portal->teleporterX = lastx;
	portal->teleporterY = lasty;
	portal->flags[PASSABLE] = true;

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
	if ( multiplayer != CLIENT )
	{
		--entity_uids;
	}
	wind->setUID(-3);
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
		list_RemoveNode(my->mynode);
		return;
	}

	my->x = parent->x;
	my->y = parent->y;

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
		if ( my->scalex < 1.0 )
		{
			my->scalex += 0.04;
		}
		else
		{
			my->scalex = 1.0;
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

void createMagicRadiusBadge(Entity& parent)
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
	my->light = addLight(my->x / 16, my->y / 16, colorForSprite(my, my->sprite, false));

	my->flags[INVISIBLE] = true;

	if ( my->actRadiusMagicInit == 0 )
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
			}
		}
		spawnMagicEffectParticles(my->x, my->y, my->z, my->sprite);
		createMagicRadiusBadge(*my);
		my->actRadiusMagicInit = 1;

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

		auto entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 1 + my->actRadiusMagicDist / 16);
		std::vector<Entity*> applyEffects;
		for ( auto it : entLists )
		{
			node_t* node;
			for ( node = it->first; node != nullptr; node = node->next )
			{
				if ( Entity* entity = (Entity*)node->element )
				{
					if ( my->actRadiusMagicID == SPELL_NULL_AREA )
					{
						if ( entity->behavior == &actMagicMissile )
						{
							if ( entityDist(my, entity) <= (real_t)my->actRadiusMagicDist )
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
							if ( entityDist(my, entity) <= (real_t)my->actRadiusMagicDist )
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
								if ( parent && entityDist(my, parent) < (real_t)my->actRadiusMagicDist )
								{
									applyEffects.push_back(entity);
								}
							}
						}
					}
					else
					{
						if ( entity->behavior == &actPlayer || (entity->behavior == &actMonster && !entity->isInertMimic()) )
						{
							if ( entityDist(my, entity) <= (real_t)my->actRadiusMagicDist )
							{
								if ( caster->checkFriend(entity) || caster == entity )
								{
									entity->setEffect(EFF_SLOW, true, 15, false);
								}
							}
						}
					}
				}
			}
		}

		for ( auto ent : applyEffects )
		{
			if ( my->actRadiusMagicID == SPELL_NULL_AREA || my->actRadiusMagicID == SPELL_SPHERE_SILENCE )
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
				ent->removeLightField();
				list_RemoveNode(ent->mynode);
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
			damage = getSpellDamageFromID(spellID, caster);
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
			fx->fskill[0] = 0.25; // rotate
			if ( auto indicator = AOEIndicators_t::getIndicator(fx->skill[10]) )
			{
				//indicator->arc = PI / 2;
				indicator->indicatorColor = color;
				indicator->loop = false;
				indicator->framesPerTick = 1;
				indicator->ticksPerUpdate = 1;
				indicator->delayTicks = 0;
				indicator->expireAlphaRate = 0.9;
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
		}
	}

	serverSpawnMiscParticlesAtLocation(x, y, z, PARTICLE_EFFECT_AREA_EFFECT, spellID, radius);
	return spellTimer;
}

void doSpellExplosionArea(int spellID, Entity* my, Entity* caster, real_t x, real_t y, real_t z, real_t radius)
{
	int trapResist = 0;
	int resistance = 0;
	DamageGib dmgGib = DMG_DEFAULT;
	real_t damageMultiplier = 1.0;

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

	std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadius(x / 16, y / 16, radius);
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
				if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
				{
					if ( caster->checkFriend(entity) )
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
				if ( spellID != SPELL_EARTH_ELEMENTAL )
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
					if ( spellID != SPELL_ETERNALS_GAZE && spellID != SPELL_EARTH_ELEMENTAL )
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
					if ( spellID != SPELL_EARTH_ELEMENTAL )
					{
						entity->SetEntityOnFire(caster);
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
			static ConsoleVariable<int> cvar_se9("/se9", -5);
			tallCeilingDelay = *cvar_se9;
		}

		static ConsoleVariable<int> cvar_se4("/se4", 20);
		int boulderSpawn = 20 + tallCeilingDelay;
		if ( enableDebugKeys && (svFlags & SV_FLAG_CHEATS) )
		{
			boulderSpawn = *cvar_se4 + tallCeilingDelay;
		}
		boulderSpawn += my->actmagicDelayMove;
		if ( my->ticks == boulderSpawn )
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
					entity->boulderShatterEarthDamage = getSpellDamageFromID(SPELL_SHATTER_EARTH, uidToEntity(my->parent));
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
									Compendium_t::Events_t::eventUpdateMonster(caster->skill[2], Compendium_t::CPDM_RECRUITED, monster, 1);
									monster->monsterAllyIndex = caster->skill[2];
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