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
#include "../sound.hpp"
#include "../items.hpp"
#include "../monster.hpp"
#include "../net.hpp"
#include "../collision.hpp"
#include "../paths.hpp"
#include "../player.hpp"
#include "magic.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define MAGICTRAP_INIT my->skill[0]
#define MAGICTRAP_SPELL my->skill[1]
#define MAGICTRAP_DIRECTION my->skill[3]

void actMagicTrap(Entity* my)
{
	if ( !MAGICTRAP_INIT )
	{
		MAGICTRAP_INIT = 1;
		switch ( rand() % 8 )
		{
			case 0:
				MAGICTRAP_SPELL = SPELL_FORCEBOLT;
				break;
			case 1:
				MAGICTRAP_SPELL = SPELL_MAGICMISSILE;
				break;
			case 2:
				MAGICTRAP_SPELL = SPELL_COLD;
				break;
			case 3:
				MAGICTRAP_SPELL = SPELL_FIREBALL;
				break;
			case 4:
				MAGICTRAP_SPELL = SPELL_LIGHTNING;
				break;
			case 5:
				MAGICTRAP_SPELL = SPELL_SLEEP;
				break;
			case 6:
				MAGICTRAP_SPELL = SPELL_CONFUSE;
				break;
			case 7:
				MAGICTRAP_SPELL = SPELL_SLOW;
				break;
			default:
				MAGICTRAP_SPELL = SPELL_MAGICMISSILE;
				break;
		}
		my->light = lightSphere(my->x / 16, my->y / 16, 3, 192);
	}

	// eliminate traps that have been destroyed.
	if ( !checkObstacle(my->x, my->y, my, NULL) )
	{
		if ( my->light )
		{
			list_RemoveNode(my->light->node);
			my->light = NULL;
		}
		list_RemoveNode(my->mynode);
		return;
	}

	if ( multiplayer == CLIENT )
	{
		return;
	}

	if (my->ticks % TICKS_PER_SECOND == 0)
	{
		int oldir = 0;
		int x = 0, y = 0;
		switch (MAGICTRAP_DIRECTION)
		{
			case 0:
				x = 12;
				y = 0;
				oldir = MAGICTRAP_DIRECTION;
				MAGICTRAP_DIRECTION++;
				break;
			case 1:
				x = 0;
				y = 12;
				oldir = MAGICTRAP_DIRECTION;
				MAGICTRAP_DIRECTION++;
				break;
			case 2:
				x = -12;
				y = 0;
				oldir = MAGICTRAP_DIRECTION;
				MAGICTRAP_DIRECTION++;
				break;
			case 3:
				x = 0;
				y = -12;
				oldir = MAGICTRAP_DIRECTION;
				MAGICTRAP_DIRECTION = 0;
				break;
		}
		int u = std::min<int>(std::max<int>(0.0, (my->x + x) / 16), map.width - 1);
		int v = std::min<int>(std::max<int>(0.0, (my->y + y) / 16), map.height - 1);
		if ( !map.tiles[OBSTACLELAYER + v * MAPLAYERS + u * MAPLAYERS * map.height] )
		{
			Entity* entity = castSpell(my->getUID(), getSpellFromID(MAGICTRAP_SPELL), false, true);
			entity->x = my->x + x;
			entity->y = my->y + y;
			entity->z = my->z;
			entity->yaw = oldir * (PI / 2.f);
			double missile_speed = 4 * ((double)(((spellElement_t*)(getSpellFromID(MAGICTRAP_SPELL)->elements.first->element))->mana) / ((spellElement_t*)(getSpellFromID(MAGICTRAP_SPELL)->elements.first->element))->overload_multiplier);
			entity->vel_x = cos(entity->yaw) * (missile_speed);
			entity->vel_y = sin(entity->yaw) * (missile_speed);
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
		if ( my->light != NULL )
		{
			list_RemoveNode(my->light->node);
			my->light = NULL;
		}

		//Light up the area.
		my->light = lightSphereShadow(my->x / 16, my->y / 16, 8, 192);
		lightball_flicker++;

		if (lightball_flicker > 5)
		{
			lightball_lighting = (lightball_lighting == 1) + 1;

			if (lightball_lighting == 1)
			{
				if ( my->light != NULL )
				{
					list_RemoveNode(my->light->node);
				}
				my->light = lightSphereShadow(my->x / 16, my->y / 16, 8, 192);
			}
			else
			{
				if ( my->light != NULL )
				{
					list_RemoveNode(my->light->node);
				}
				my->light = lightSphereShadow(my->x / 16, my->y / 16, 8, 174);
			}
			lightball_flicker = 0;
		}

		lightball_timer--;
		return;
	}

	my->yaw += .01;
	if ( my->yaw >= PI * 2 )
	{
		my->yaw -= PI * 2;
	}

	/*if (!my->parent) { //This means that it doesn't have a caster. In other words, magic light staff.
		return;
	})*/

	//list_t *path = NULL;
	pathnode_t* pathnode = NULL;

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
				if ( my->light != NULL )
				{
					list_RemoveNode(my->light->node);
					my->light = NULL;
				}
				list_RemoveNode(my->mynode); //Delete the light spell.
				return;
			}
		}
	}
	else if (spell->caster >= 1)     //So no caster...but uidToEntity returns NULL if entity is already dead, right? And if the uid is supposed to point to an entity, but it doesn't...it means the caster has died.
	{
		if ( my->light != NULL )
		{
			list_RemoveNode(my->light->node);
			my->light = NULL;
		}
		list_RemoveNode(my->mynode);
		return;
	}

	// if the spell has been unsustained, remove it
	if ( !spell->magicstaff && !spell->sustain )
	{
		int i = 0;
		int player = -1;
		for (i = 0; i < 4; ++i)
		{
			if (players[i]->entity == caster)
			{
				player = i;
			}
		}
		if (player > -1 && multiplayer == SERVER)
		{
			strcpy( (char*)net_packet->data, "UNCH");
			net_packet->data[4] = player;
			SDLNet_Write32(spell->ID, &net_packet->data[5]);
			net_packet->address.host = net_clients[player - 1].host;
			net_packet->address.port = net_clients[player - 1].port;
			net_packet->len = 9;
			sendPacketSafe(net_sock, -1, net_packet, player - 1);
		}
		if ( my->light )
		{
			list_RemoveNode(my->light->node);
		}
		my->light = NULL;
		list_RemoveNode(my->mynode);
		return;
	}

	if (magic_init)
	{
		if ( my->light != NULL )
		{
			list_RemoveNode(my->light->node);
			my->light = NULL;
		}

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
						for (i = 0; i < 4; ++i)
						{
							if (players[i]->entity == caster)
							{
								player = i;
							}
						}
						if (player > -1 && multiplayer == SERVER)
						{
							strcpy( (char*)net_packet->data, "UNCH");
							net_packet->data[4] = player;
							SDLNet_Write32(spell->ID, &net_packet->data[5]);
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 9;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}
						if ( my->light )
						{
							list_RemoveNode(my->light->node);
						}
						my->light = NULL;
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
				lineTraceTarget(my, my->x, my->y, tangent, 1024, IGNORE_ENTITIES, false, parent);
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
						list_t* path = generatePath((int)floor(my->x / 16), (int)floor(my->y / 16), (int)floor(parent->x / 16), (int)floor(parent->y / 16), my, parent);
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
									pathnode = (pathnode_t*)node->element;
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
								pathnode = (pathnode_t*)my->path->first->element;
								//double distance = sqrt(pow(pathnode->y * 16 + 8 - my->y, 2) + pow(pathnode->x * 16 + 8 - my->x, 2) );
								//double distance = sqrt(pow((my->y) - ((pathnode->y + 8) * 16), 2) + pow((my->x) - ((pathnode->x + 8) * 16), 2));
								double distance = sqrt(pow(((pathnode->y * 16) + 8) - (my->y), 2) + pow(((pathnode->x * 16) + 8) - (my->x), 2));
								if (distance <= 4)
								{
									list_RemoveNode(pathnode->node); //TODO: Make sure it doesn't get stuck here. Maybe remove the node only if it's the last one?
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
		my->light = lightSphereShadow(my->x / 16, my->y / 16, 8, 192);
		lightball_flicker++;

		if (lightball_flicker > 5)
		{
			lightball_lighting = (lightball_lighting == 1) + 1;

			if (lightball_lighting == 1)
			{
				if ( my->light != NULL )
				{
					list_RemoveNode(my->light->node);
				}
				my->light = lightSphereShadow(my->x / 16, my->y / 16, 8, 192);
			}
			else
			{
				if ( my->light != NULL )
				{
					list_RemoveNode(my->light->node);
				}
				my->light = lightSphereShadow(my->x / 16, my->y / 16, 8, 174);
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
		if ( my->light != NULL )
		{
			list_RemoveNode(my->light->node);
			my->light = NULL;
		}

		if (clientnum == 0 || multiplayer == SERVER)
		{
			//Handle the missile's life.
			MAGIC_LIFE++;

			if (MAGIC_LIFE >= MAGIC_MAXLIFE)
			{
				list_RemoveNode(my->mynode);
				return;
			}

			node = spell->elements.first;
			//element = (spellElement_t *) spell->elements->first->element;
			element = (spellElement_t*)node->element;

			double dist = clipMove(&my->x, &my->y, my->vel_x, my->vel_y, my);

			if ( dist != sqrt(my->vel_x * my->vel_x + my->vel_y * my->vel_y) )
			{
				node = element->elements.first;
				//element = (spellElement_t *) element->elements->first->element;
				element = (spellElement_t*)node->element;
				//if (hit.entity != NULL) {
				Stat* hitstats = NULL;
				int player = -1;
				if (hit.entity)
				{
					hitstats = hit.entity->getStats();
					if ( hit.entity->behavior == &actPlayer )
					{
						player = hit.entity->skill[2];
						Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
						messagePlayerColor(player, color, language[376]);
					}
					if ( parent && hitstats )
					{
						if ( parent->behavior == &actPlayer )
						{
							Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
							if ( strcmp(hitstats->name, "") )
							{
								messagePlayerColor(parent->skill[2], color, language[377], hitstats->name);
							}
							else
							{
								if ( hitstats->type < KOBOLD ) //Original monster count
								{
									messagePlayerColor(parent->skill[2], color, language[378], language[90 + hitstats->type]);
								}
								else if ( hitstats->type >= KOBOLD ) //New monsters
								{
									messagePlayerColor(parent->skill[2], color, language[378], language[2000 + (hitstats->type - KOBOLD)]);
								}
							}
						}
					}
				}

				if (hit.entity)
				{
					// alert the hit entity if it was a monster
					if ( hit.entity->behavior == &actMonster && parent != NULL )
					{
						if ( hit.entity->skill[0] != 1 && (hitstats->type < LICH || hitstats->type >= SHOPKEEPER) )
						{
							hit.entity->skill[0] = 2;
							hit.entity->skill[1] = parent->getUID();
							hit.entity->fskill[2] = parent->x;
							hit.entity->fskill[3] = parent->y;
						}

						// alert other monsters too
						Entity* ohitentity = hit.entity;
						for ( node = map.entities->first; node != NULL; node = node->next )
						{
							entity = (Entity*)node->element;
							if ( entity->behavior == &actMonster && entity != ohitentity )
							{
								Stat* buddystats = entity->getStats();
								if ( buddystats != NULL )
								{
									if ( hit.entity && hit.entity->checkFriend(entity) )   //TODO: hit.entity->checkFriend() without first checking if it's NULL crashes because hit.entity turns to NULL somewhere along the line. It looks like ohitentity preserves that value though, so....uh...ya, I don't know.
									{
										if ( entity->skill[0] == 0 )   // monster is waiting
										{
											tangent = atan2( entity->y - ohitentity->y, entity->x - ohitentity->x );
											lineTrace(ohitentity, ohitentity->x, ohitentity->y, tangent, 1024, 0, false);
											if ( hit.entity == entity )
											{
												entity->skill[0] = 2; // path state
												entity->skill[1] = parent->getUID();
												entity->fskill[2] = parent->x;
												entity->fskill[3] = parent->y;
											}
										}
									}
								}
							}
						}
						hit.entity = ohitentity;
					}
				}

				// check for magic reflection...
				int reflection = 0;
				if ( hitstats )
				{
					if ( !strcmp(map.name, "Hell Boss") && hit.entity->behavior == &actPlayer )
					{
						bool founddevil = false;
						node_t* tempNode;
						for ( tempNode = map.entities->first; tempNode != NULL; tempNode = tempNode->next )
						{
							Entity* tempEntity = (Entity*)tempNode->element;
							if ( tempEntity->behavior == &actMonster )
							{
								Stat* stats = tempEntity->getStats();
								if ( stats )
								{
									if ( stats->type == DEVIL )
									{
										founddevil = true;
										break;
									}
								}
							}
						}
						if ( !founddevil )
						{
							reflection = 3;
						}
					}
					if ( !reflection )
					{
						reflection = hit.entity->getReflection();
					}
				}
				if ( reflection )
				{
					spell_t* spellIsReflectingMagic = hit.entity->getActiveMagicEffect(SPELL_REFLECT_MAGIC);

					if (hit.entity)
					{
						if ( hit.entity->behavior == &actPlayer )
						{
							if ( !spellIsReflectingMagic )
							{
								messagePlayer(player, language[379]);
							}
							else
							{
								messagePlayer(player, language[2475]);
							}
						}
					}
					if ( parent )
					{
						if ( parent->behavior == &actPlayer )
						{
							messagePlayer(parent->skill[2], language[379]);
						}
					}
					if ( hit.side == HORIZONTAL )
					{
						my->vel_x *= -1;
					}
					else if ( hit.side == VERTICAL )
					{
						my->vel_y *= -1;
					}
					if (hit.entity)
					{
						my->parent = hit.entity->getUID();
					}
					// reflection of 3 does not degrade.
					if ( rand() % 2 == 0 && hitstats && reflection < 3 )
					{
						// set armornum to the relevant equipment slot to send to clients
						int armornum = 5 + reflection;
						if ( player == clientnum || player < 0 )
						{
							if ( reflection == 1 )
							{
								if ( hitstats->cloak->count > 1 )
								{
									newItem(hitstats->cloak->type, hitstats->cloak->status, hitstats->cloak->beatitude, hitstats->cloak->count - 1, hitstats->cloak->appearance, hitstats->cloak->identified, &hitstats->inventory);
								}
							}
							else if ( reflection == 2 )
							{
								if ( hitstats->amulet->count > 1 )
								{
									newItem(hitstats->amulet->type, hitstats->amulet->status, hitstats->amulet->beatitude, hitstats->amulet->count - 1, hitstats->amulet->appearance, hitstats->amulet->identified, &hitstats->inventory);
								}
							}
							else if ( reflection == -1 )
							{
								if ( hitstats->shield->count > 1 )
								{
									newItem(hitstats->shield->type, hitstats->shield->status, hitstats->shield->beatitude, hitstats->shield->count - 1, hitstats->shield->appearance, hitstats->shield->identified, &hitstats->inventory);
								}
							}
						}
						if ( reflection == 1 )
						{
							hitstats->cloak->count = 1;
							hitstats->cloak->status = static_cast<Status>(hitstats->cloak->status - 1);
							if ( hitstats->cloak->status != BROKEN )
							{
								messagePlayer(player, language[380]);
							}
							else
							{
								messagePlayer(player, language[381]);
								playSoundEntity(hit.entity, 76, 64);
							}
						}
						else if ( reflection == 2 )
						{
							hitstats->amulet->count = 1;
							hitstats->amulet->status = static_cast<Status>(hitstats->amulet->status - 1);
							if ( hitstats->amulet->status != BROKEN )
							{
								messagePlayer(player, language[382]);
							}
							else
							{
								messagePlayer(player, language[383]);
								playSoundEntity(hit.entity, 76, 64);
							}
						}
						else if ( reflection == -1 )
						{
							hitstats->shield->count = 1;
							hitstats->shield->status = static_cast<Status>(hitstats->shield->status - 1);
							if ( hitstats->shield->status != BROKEN )
							{
								messagePlayer(player, language[384]);
							}
							else
							{
								messagePlayer(player, language[385]);
								playSoundEntity(hit.entity, 76, 64);
							}
						}
						if (player > 0 && multiplayer == SERVER)
						{
							strcpy((char*)net_packet->data, "ARMR");
							net_packet->data[4] = armornum;
							if (reflection == 1)
							{
								net_packet->data[5] = hitstats->cloak->status;
							}
							else if (reflection == 2)
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
								messagePlayer(player, language[2476]);
							}
						}
					}
					return;
				}

				// check for magic resistance...
				// resistance stacks diminishingly
				//TODO: EFFECTS[EFF_MAGICRESIST]
				int resistance = 0;
				if ( hitstats )
				{
					if ( hitstats->shield )
					{
						if ( hitstats->shield->type == STEEL_SHIELD_RESISTANCE )
						{
							if ( hitstats->defending )
							{
								resistance += 2;
							}
							else
							{
								resistance += 1;
							}
						}
					}
					if ( hitstats->ring )
					{
						if ( hitstats->ring->type == RING_MAGICRESISTANCE )
						{
							resistance += 1;
						}
					}
					if ( hitstats->gloves )
					{
						if ( hitstats->gloves->type == ARTIFACT_GLOVES )
						{
							resistance += 1;
						}
					}
				}
				if ( resistance > 0 )
				{
					if ( parent )
					{
						if ( parent->behavior == &actPlayer )
						{
							messagePlayer(parent->skill[2], language[386]);
						}
					}
				}

				if (!strcmp(element->name, spellElement_force.name))
				{
					if (hit.entity)
					{
						if (hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer)
						{
							Entity* parent = uidToEntity(my->parent);
							if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
							{
								// test for friendly fire
								if ( parent && parent->checkFriend(hit.entity) )
								{
									list_RemoveNode(my->mynode);
									return;
								}
							}
							playSoundEntity(hit.entity, 28, 128);
							int damage = element->damage;
							//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
							damage *= damagetables[hitstats->type][5];
							damage /= (1 + (int)resistance);
							hit.entity->modHP(-damage);
							for (i = 0; i < damage; i += 2)   //Spawn a gib for every two points of damage.
							{
								spawnGib(hit.entity);
							}

							if (parent)
							{
								parent->killedByMonsterObituary(hit.entity);
							}

							// update enemy bar for attacker
							if ( !strcmp(hitstats->name, "") )
							{
								if ( hitstats->type < KOBOLD ) //Original monster count
								{
									updateEnemyBar(parent, hit.entity, language[90 + hitstats->type], hitstats->HP, hitstats->MAXHP);
								}
								else if ( hitstats->type >= KOBOLD ) //New monsters
								{
									updateEnemyBar(parent, hit.entity, language[2000 + (hitstats->type - KOBOLD)], hitstats->HP, hitstats->MAXHP);
								}
							}
							else
							{
								updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP);
							}

							if ( hitstats->HP <= 0 && parent)
							{
								parent->awardXP( hit.entity, true, true );
							}
							if ( my->light != NULL )
							{
								list_RemoveNode(my->light->node);
								my->light = NULL;
							}
							list_RemoveNode(my->mynode);
							return;
						}
						else if (hit.entity->behavior == &actDoor)
						{
							int damage = element->damage;
							damage /= (1 + (int)resistance);

							hit.entity->skill[4] -= damage; //Decrease door health.
							if ( hit.entity->skill[4] < 0 )
								if ( parent )
									if ( parent->behavior == &actPlayer )
									{
										messagePlayer(parent->skill[2], language[387]);
									}
							playSoundEntity(hit.entity, 28, 128);
							if ( !hit.entity->skill[0] )
							{
								hit.entity->skill[6] = (my->x > hit.entity->x);
							}
							else
							{
								hit.entity->skill[6] = (my->y < hit.entity->y);
							}
							if ( my->light != NULL )
							{
								list_RemoveNode(my->light->node);
								my->light = NULL;
							}
							updateEnemyBar(parent, hit.entity, language[674], hit.entity->skill[4], hit.entity->skill[9]);
							list_RemoveNode(my->mynode);
							return;
							/*} else if (hit.entity->behavior == &actChest) { //TODO: Get right skill values and language file entries.
								int damage = element->damage;
								damage /= (1+(int)resistance);

								hit.entity->skill[3] -= damage; //Decrease chest health.
								if( hit.entity->skill[3] < 0 )
									if( parent )
										if( parent->behavior == &actPlayer )
											messagePlayer(parent->skill[2],language[387]);
								playSoundEntity(hit.entity, 28, 128);
								if( !hit.entity->skill[0] )
									hit.entity->skill[6] = (my->x > hit.entity->x);
								else
									hit.entity->skill[6] = (my->y < hit.entity->y);
								if( my->light != NULL ) {
									list_RemoveNode(my->light->node);
									my->light = NULL;
								}
								updateEnemyBar(parent,hit.entity,language[674],hit.entity->skill[3],hit.entity->skill[9]);
								list_RemoveNode(my->mynode);
								return;*/
						}
						else if (hit.entity->behavior == &actFurniture )
						{
							int damage = element->damage;
							damage /= (1 + (int)resistance);
							hit.entity->skill[4] -= damage;
							if ( hit.entity->skill[4] < 0 )
							{
								if ( parent )
								{
									if ( parent->behavior == &actPlayer )
									{
										if ( hit.entity->skill[0] )
										{
											// chair
											messagePlayer(parent->skill[2], language[388]);
											updateEnemyBar(parent, hit.entity, language[677], hit.entity->skill[4], hit.entity->skill[9]);
										}
										else
										{
											// table
											messagePlayer(parent->skill[2], language[389]);
											updateEnemyBar(parent, hit.entity, language[676], hit.entity->skill[4], hit.entity->skill[9]);
										}
									}
								}
							}
							playSoundEntity(hit.entity, 28, 128);
							if ( my->light != NULL )
							{
								list_RemoveNode(my->light->node);
								my->light = NULL;
							}
							list_RemoveNode(my->mynode);
							return;
						}
					}
				}
				else if (!strcmp(element->name, spellElement_magicmissile.name))
				{
					spawnExplosion(my->x, my->y, my->z);
					if (hit.entity)
					{
						if (hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer)
						{
							Entity* parent = uidToEntity(my->parent);
							if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
							{
								// test for friendly fire
								if ( parent &&  parent->checkFriend(hit.entity) )
								{
									if ( my->light != NULL )
									{
										list_RemoveNode(my->light->node);
										my->light = NULL;
									}
									list_RemoveNode(my->mynode);
									return;
								}
							}
							playSoundEntity(hit.entity, 28, 128);
							int damage = element->damage;
							//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
							damage *= damagetables[hitstats->type][5];
							damage /= (1 + (int)resistance);
							hit.entity->modHP(-damage);
							for (i = 0; i < damage; i += 2)   //Spawn a gib for every two points of damage.
							{
								spawnGib(hit.entity);
							}

							// write the obituary
							if (parent)
							{
								parent->killedByMonsterObituary(hit.entity);
							}

							// update enemy bar for attacker
							if ( !strcmp(hitstats->name, "") )
							{
								if ( hitstats->type < KOBOLD ) //Original monster count
								{
									updateEnemyBar(parent, hit.entity, language[90 + hitstats->type], hitstats->HP, hitstats->MAXHP);
								}
								else if ( hitstats->type >= KOBOLD ) //New monsters
								{
									updateEnemyBar(parent, hit.entity, language[2000 + (hitstats->type - KOBOLD)], hitstats->HP, hitstats->MAXHP);
								}
							}
							else
							{
								updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP);
							}

							if ( hitstats->HP <= 0 && parent)
							{
								parent->awardXP( hit.entity, true, true );
							}
							if ( my->light != NULL )
							{
								list_RemoveNode(my->light->node);
								my->light = NULL;
							}
							list_RemoveNode(my->mynode);
							return;
						}
						else if (hit.entity->behavior == &actDoor)
						{
							int damage = element->damage;
							//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
							damage /= (1 + (int)resistance);

							hit.entity->skill[4] -= damage; //Decrease door health.
							if ( hit.entity->skill[4] < 0 )
								if ( parent )
									if ( parent->behavior == &actPlayer )
									{
										messagePlayer(parent->skill[2], language[387]);
									}
							playSoundEntity(hit.entity, 28, 128);
							if ( !hit.entity->skill[0] )
							{
								hit.entity->skill[6] = (my->x > hit.entity->x);
							}
							else
							{
								hit.entity->skill[6] = (my->y < hit.entity->y);
							}

							if ( my->light != NULL )
							{
								list_RemoveNode(my->light->node);
								my->light = NULL;
							}
							updateEnemyBar(parent, hit.entity, language[674], hit.entity->skill[4], hit.entity->skill[9]);
							list_RemoveNode(my->mynode);
							return;
							/*} else if (hit.entity->behavior == &actChest) { //TODO: Get right skill values and language file entries.
								int damage = element->damage;
								//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
								damage /= (1+(int)resistance);

								hit.entity->skill[3] -= damage; //Decrease chest health.
								if( hit.entity->skill[3] < 0 )
									if( parent )
										if( parent->behavior == &actPlayer )
											messagePlayer(parent->skill[2],language[387]);
								playSoundEntity(hit.entity, 28, 128);
								if( !hit.entity->skill[0] )
									hit.entity->skill[6] = (my->x > hit.entity->x);
								else
									hit.entity->skill[6] = (my->y < hit.entity->y);

								if( my->light != NULL ) {
									list_RemoveNode(my->light->node);
									my->light = NULL;
								}
								updateEnemyBar(parent,hit.entity,language[674],hit.entity->skill[3],hit.entity->skill[9]);
								list_RemoveNode(my->mynode);
								return;*/
						}
						else if (hit.entity->behavior == &actFurniture )
						{
							int damage = element->damage;
							damage /= (1 + (int)resistance);
							hit.entity->skill[4] -= damage;
							if ( hit.entity->skill[4] < 0 )
							{
								if ( parent )
								{
									if ( parent->behavior == &actPlayer )
									{
										if ( hit.entity->skill[0] )
										{
											// chair
											messagePlayer(parent->skill[2], language[388]);
											updateEnemyBar(parent, hit.entity, language[677], hit.entity->skill[4], hit.entity->skill[9]);
										}
										else
										{
											// table
											messagePlayer(parent->skill[2], language[389]);
											updateEnemyBar(parent, hit.entity, language[676], hit.entity->skill[4], hit.entity->skill[9]);
										}
									}
								}
							}
							playSoundEntity(hit.entity, 28, 128);
							if ( my->light != NULL )
							{
								list_RemoveNode(my->light->node);
								my->light = NULL;
							}
							list_RemoveNode(my->mynode);
							return;
						}
					}
				}
				else if (!strcmp(element->name, spellElement_fire.name))
				{
					spawnExplosion(my->x, my->y, my->z);
					if (hit.entity)
					{
						if ( hit.entity->flags[BURNABLE] )
							if ( !hit.entity->flags[BURNING] )
							{
								hit.entity->flags[BURNING] = true;
							}
						if (hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer)
						{
							if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
							{
								// test for friendly fire
								if ( parent && parent->checkFriend(hit.entity) )
								{
									if ( my->light != NULL )
									{
										list_RemoveNode(my->light->node);
										my->light = NULL;
									}
									list_RemoveNode(my->mynode);
									return;
								}
							}
							//playSoundEntity(my, 153, 64);
							playSoundEntity(hit.entity, 28, 128);
							//TODO: Apply fire resistances/weaknesses.
							int damage = element->damage;
							//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
							damage *= damagetables[hitstats->type][5];
							damage /= (1 + (int)resistance);
							hit.entity->modHP(-damage);
							//for (i = 0; i < damage; i += 2) { //Spawn a gib for every two points of damage.
							spawnGib(hit.entity);
							//}

							// write the obituary
							hit.entity->setObituary(language[1501]);
							if ( hitstats )
							{
								hitstats->poisonKiller = my->parent;
							}

							// update enemy bar for attacker
							if ( !strcmp(hitstats->name, "") )
							{
								if ( hitstats->type < KOBOLD ) //Original monster count
								{
									updateEnemyBar(parent, hit.entity, language[90 + hitstats->type], hitstats->HP, hitstats->MAXHP);
								}
								else if ( hitstats->type >= KOBOLD ) //New monsters
								{
									updateEnemyBar(parent, hit.entity, language[2000 + (hitstats->type - KOBOLD)], hitstats->HP, hitstats->MAXHP);
								}
							}
							else
							{
								updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP);
							}
							if ( hitstats->HP <= 0 && parent)
							{
								parent->awardXP( hit.entity, true, true );
							}
							if ( my->light != NULL )
							{
								list_RemoveNode(my->light->node);
								my->light = NULL;
							}
							list_RemoveNode(my->mynode);
							return;
						}
						else if (hit.entity->behavior == &actDoor)
						{
							int damage = element->damage;
							damage /= (1 + (int)resistance);

							hit.entity->skill[4] -= damage; //Decrease door health.
							if ( hit.entity->skill[4] < 0 )
								if ( parent )
									if ( parent->behavior == &actPlayer )
									{
										messagePlayer(parent->skill[2], language[387]);
									}
							playSoundEntity(hit.entity, 28, 128);
							if ( !hit.entity->skill[0] )
							{
								hit.entity->skill[6] = (my->x > hit.entity->x);
							}
							else
							{
								hit.entity->skill[6] = (my->y < hit.entity->y);
							}
							if ( my->light != NULL )
							{
								list_RemoveNode(my->light->node);
								my->light = NULL;
							}
							updateEnemyBar(parent, hit.entity, language[674], hit.entity->skill[4], hit.entity->skill[9]);
							list_RemoveNode(my->mynode);
							return;
							/*} else if (hit.entity->behavior == &actChest) { //TODO: Get right skill values and language file entries.
								int damage = element->damage;
								damage /= (1+(int)resistance);

								hit.entity->skill[3] -= damage; //Decrease chest health.
								if( hit.entity->skill[3] < 0 )
									if( parent )
										if( parent->behavior == &actPlayer )
											messagePlayer(parent->skill[2],language[387]);
								playSoundEntity(hit.entity, 28, 128);
								if( !hit.entity->skill[0] )
									hit.entity->skill[6] = (my->x > hit.entity->x);
								else
									hit.entity->skill[6] = (my->y < hit.entity->y);
								if( my->light != NULL ) {
									list_RemoveNode(my->light->node);
									my->light = NULL;
								}
								updateEnemyBar(parent,hit.entity,language[674],hit.entity->skill[3],hit.entity->skill[9]);
								list_RemoveNode(my->mynode);
								return;*/
						}
						else if (hit.entity->behavior == &actFurniture )
						{
							int damage = element->damage;
							damage /= (1 + (int)resistance);
							hit.entity->skill[4] -= damage;
							if ( hit.entity->skill[4] < 0 )
							{
								if ( parent )
								{
									if ( parent->behavior == &actPlayer )
									{
										if ( hit.entity->skill[0] )
										{
											// chair
											messagePlayer(parent->skill[2], language[388]);
											updateEnemyBar(parent, hit.entity, language[677], hit.entity->skill[4], hit.entity->skill[9]);
										}
										else
										{
											// table
											messagePlayer(parent->skill[2], language[389]);
											updateEnemyBar(parent, hit.entity, language[676], hit.entity->skill[4], hit.entity->skill[9]);
										}
									}
								}
							}
							playSoundEntity(hit.entity, 28, 128);
							if ( my->light != NULL )
							{
								list_RemoveNode(my->light->node);
								my->light = NULL;
							}
							list_RemoveNode(my->mynode);
							return;
						}
					}
				}
				else if (!strcmp(element->name, spellElement_confuse.name))
				{
					if (hit.entity)
					{
						if (hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer)
						{
							if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
							{
								// test for friendly fire
								if ( parent && parent->checkFriend(hit.entity) )
								{
									if ( my->light != NULL )
									{
										list_RemoveNode(my->light->node);
										my->light = NULL;
									}
									list_RemoveNode(my->mynode);
									return;
								}
							}
							playSoundEntity(hit.entity, 174, 64);
							hitstats->EFFECTS[EFF_CONFUSED] = true;
							hitstats->EFFECTS_TIMERS[EFF_CONFUSED] = (element->duration * (((element->mana) / static_cast<double>(element->base_mana)) * element->overload_multiplier));
							hitstats->EFFECTS_TIMERS[EFF_CONFUSED] /= (1 + (int)resistance);
							hit.entity->skill[1] = 0; //Remove the monster's target.
							if ( parent )
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								if ( parent->behavior == &actPlayer )
								{
									if ( strcmp(hitstats->name, "") )
									{
										messagePlayerColor(parent->skill[2], color, language[390], hitstats->name);
									}
									else
									{
										if ( hitstats->type < KOBOLD ) //Original monster count
										{
											messagePlayerColor(parent->skill[2], color, language[391], language[90 + hitstats->type]);
										}
										else if ( hitstats->type >= KOBOLD ) //New monsters
										{
											messagePlayerColor(parent->skill[2], color, language[391], language[2000 + (hitstats->type - KOBOLD)]);
										}
									}
								}
							}
							Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
							if ( player >= 0 )
							{
								messagePlayerColor(player, color, language[392]);
							}
							if ( my->light != NULL )
							{
								list_RemoveNode(my->light->node);
								my->light = NULL;
							}
							spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my->sprite);
							list_RemoveNode(my->mynode);
							return;
						}
					}
				}
				else if (!strcmp(element->name, spellElement_cold.name))
				{
					playSoundEntity(my, 197, 128);
					if (hit.entity)
					{
						if (hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer)
						{
							if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
							{
								// test for friendly fire
								if ( parent && parent->checkFriend(hit.entity) )
								{
									if ( my->light != NULL )
									{
										list_RemoveNode(my->light->node);
										my->light = NULL;
									}
									list_RemoveNode(my->mynode);
									return;
								}
							}
							playSoundEntity(hit.entity, 172, 64);
							hitstats->EFFECTS[EFF_SLOW] = true;
							hitstats->EFFECTS_TIMERS[EFF_SLOW] = (element->duration * (((element->mana) / static_cast<double>(element->base_mana)) * element->overload_multiplier));
							hitstats->EFFECTS_TIMERS[EFF_SLOW] /= (1 + (int)resistance);
							int damage = element->damage;
							//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
							damage *= damagetables[hitstats->type][5];
							damage /= (1 + (int)resistance);
							hit.entity->modHP(-damage);
							spawnGib(hit.entity);

							// write the obituary
							hit.entity->setObituary(language[1502]);

							// update enemy bar for attacker
							if ( !strcmp(hitstats->name, "") )
							{
								if ( hitstats->type < KOBOLD ) //Original monster count
								{
									updateEnemyBar(parent, hit.entity, language[90 + hitstats->type], hitstats->HP, hitstats->MAXHP);
								}
								else if ( hitstats->type >= KOBOLD ) //New monsters
								{
									updateEnemyBar(parent, hit.entity, language[2000 + (hitstats->type - KOBOLD)], hitstats->HP, hitstats->MAXHP);
								}
							}
							else
							{
								updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP);
							}
							if ( parent )
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								if ( parent->behavior == &actPlayer )
								{
									if ( strcmp(hitstats->name, "") )
									{
										messagePlayerColor(parent->skill[2], color, language[393], hitstats->name);
									}
									else
									{
										if ( hitstats->type < KOBOLD ) //Original monster count
										{
											messagePlayerColor(parent->skill[2], color, language[394], language[90 + hitstats->type]);
										}
										else if ( hitstats->type >= KOBOLD ) //New monsters
										{
											messagePlayerColor(parent->skill[2], color, language[394], language[2000 + (hitstats->type - KOBOLD)]);
										}
									}
								}
							}
							Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
							if ( player >= 0 )
							{
								messagePlayerColor(player, color, language[395]);
							}
							if ( my->light != NULL )
							{
								list_RemoveNode(my->light->node);
								my->light = NULL;
							}
							list_RemoveNode(my->mynode);
							return;
						}
					}
				}
				else if (!strcmp(element->name, spellElement_slow.name))
				{
					if (hit.entity)
					{
						if (hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer)
						{
							if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
							{
								// test for friendly fire
								if ( parent && parent->checkFriend(hit.entity) )
								{
									if ( my->light != NULL )
									{
										list_RemoveNode(my->light->node);
										my->light = NULL;
									}
									list_RemoveNode(my->mynode);
									return;
								}
							}
							playSoundEntity(hit.entity, 172, 64); //TODO: Slow spell sound.
							hitstats->EFFECTS[EFF_SLOW] = true;
							hitstats->EFFECTS_TIMERS[EFF_SLOW] = (element->duration * (((element->mana) / static_cast<double>(element->base_mana)) * element->overload_multiplier));
							hitstats->EFFECTS_TIMERS[EFF_SLOW] /= (1 + (int)resistance);
							// update enemy bar for attacker
							if ( parent )
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								if ( parent->behavior == &actPlayer )
								{
									if ( strcmp(hitstats->name, "") )
									{
										messagePlayerColor(parent->skill[2], color, language[393], hitstats->name);
									}
									else
									{
										if ( hitstats->type < KOBOLD ) //Original monster count
										{
											messagePlayerColor(parent->skill[2], color, language[394], language[90 + hitstats->type]);
										}
										else if ( hitstats->type >= KOBOLD ) //New monsters
										{
											messagePlayerColor(parent->skill[2], color, language[394], language[2000 + (hitstats->type - KOBOLD)]);
										}
									}
								}
							}
							Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
							if ( player >= 0 )
							{
								messagePlayerColor(player, color, language[395]);
							}
							if ( my->light != NULL )
							{
								list_RemoveNode(my->light->node);
								my->light = NULL;
							}
							spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my->sprite);
							list_RemoveNode(my->mynode);
							return;
						}
					}
				}
				else if (!strcmp(element->name, spellElement_sleep.name))
				{
					if (hit.entity)
					{
						if (hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer)
						{
							if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
							{
								// test for friendly fire
								if ( parent && parent->checkFriend(hit.entity) )
								{
									if ( my->light != NULL )
									{
										list_RemoveNode(my->light->node);
										my->light = NULL;
									}
									list_RemoveNode(my->mynode);
									return;
								}
							}
							playSoundEntity(hit.entity, 174, 64);
							hitstats->EFFECTS[EFF_ASLEEP] = true;
							hitstats->EFFECTS_TIMERS[EFF_ASLEEP] = 600 + rand() % 300;
							hitstats->EFFECTS_TIMERS[EFF_ASLEEP] /= (1 + (int)resistance);
							hitstats->OLDHP = hitstats->HP;
							if ( hit.entity->behavior == &actPlayer )
							{
								serverUpdateEffects(hit.entity->skill[2]);
								Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
								messagePlayerColor(hit.entity->skill[2], color, language[396]);
							}
							if ( parent )
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								if ( parent->behavior == &actPlayer )
								{
									if ( strcmp(hitstats->name, "") )
									{
										messagePlayerColor(parent->skill[2], color, language[397], hitstats->name);
									}
									else
									{
										if ( hitstats->type < KOBOLD ) //Original monster count
										{
											messagePlayerColor(parent->skill[2], color, language[398], language[90 + hitstats->type]);
										}
										else if ( hitstats->type >= KOBOLD ) //New monsters
										{
											messagePlayerColor(parent->skill[2], color, language[398], language[2000 + (hitstats->type - KOBOLD)]);
										}
									}
								}
							}
							if ( my->light != NULL )
							{
								list_RemoveNode(my->light->node);
								my->light = NULL;
							}
							spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my->sprite);
							list_RemoveNode(my->mynode);
							return;
						}
					}
				}
				else if (!strcmp(element->name, spellElement_lightning.name))
				{
					playSoundEntity(my, 173, 128);
					if (hit.entity)
					{
						if (hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer)
						{
							Entity* parent = uidToEntity(my->parent);
							if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
							{
								// test for friendly fire
								if ( parent && parent->checkFriend(hit.entity) )
								{
									if ( my->light != NULL )
									{
										list_RemoveNode(my->light->node);
										my->light = NULL;
									}
									list_RemoveNode(my->mynode);
									return;
								}
							}
							playSoundEntity(my, 173, 64);
							playSoundEntity(hit.entity, 28, 128);
							int damage = element->damage;
							//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
							damage *= damagetables[hitstats->type][5];
							damage /= (1 + (int)resistance);
							hit.entity->modHP(-damage);

							// write the obituary
							if (parent)
							{
								parent->killedByMonsterObituary(hit.entity);
							}

							// update enemy bar for attacker
							if ( !strcmp(hitstats->name, "") )
							{
								if ( hitstats->type < KOBOLD ) //Original monster count
								{
									updateEnemyBar(parent, hit.entity, language[90 + hitstats->type], hitstats->HP, hitstats->MAXHP);
								}
								else if ( hitstats->type >= KOBOLD ) //New monsters
								{
									updateEnemyBar(parent, hit.entity, language[2000 + (hitstats->type - KOBOLD)], hitstats->HP, hitstats->MAXHP);
								}
							}
							else
							{
								updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP);
							}
							if ( hitstats->HP <= 0 && parent)
							{
								parent->awardXP( hit.entity, true, true );
							}
							if ( my->light != NULL )
							{
								list_RemoveNode(my->light->node);
								my->light = NULL;
							}
							list_RemoveNode(my->mynode);
							return;
						}
						else if (hit.entity->behavior == &actDoor)
						{
							int damage = element->damage;
							//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
							damage /= (1 + (int)resistance);

							hit.entity->skill[4] -= damage; //Decrease door health.
							if ( hit.entity->skill[4] < 0 )
							{
								if ( parent )
								{
									if ( parent->behavior == &actPlayer )
									{
										messagePlayer(parent->skill[2], language[387]);
									}
								}
							}
							playSoundEntity(hit.entity, 28, 128);
							if ( !hit.entity->skill[0] )
							{
								hit.entity->skill[6] = (my->x > hit.entity->x);
							}
							else
							{
								hit.entity->skill[6] = (my->y < hit.entity->y);
							}
							if ( my->light != NULL )
							{
								list_RemoveNode(my->light->node);
								my->light = NULL;
							}
							updateEnemyBar(parent, hit.entity, language[674], hit.entity->skill[4], hit.entity->skill[9]);
							list_RemoveNode(my->mynode);
							return;
							/*} else if (hit.entity->behavior == &actChest) { //TODO: Get right skill values and language file entries.
								int damage = element->damage;
								//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
								damage /= (1+(int)resistance);

								hit.entity->skill[3] -= damage; //Decrease chest health.
								if( hit.entity->skill[3] < 0 ) {
									if( parent ) {
										if( parent->behavior == &actPlayer ) {
											messagePlayer(parent->skill[2],language[387]);
										}
									}
								}
								playSoundEntity(hit.entity, 28, 128);
								if( !hit.entity->skill[0] )
									hit.entity->skill[6] = (my->x > hit.entity->x);
								else
									hit.entity->skill[6] = (my->y < hit.entity->y);
								if( my->light != NULL ) {
									list_RemoveNode(my->light->node);
									my->light = NULL;
								}
								updateEnemyBar(parent,hit.entity,language[674],hit.entity->skill[3],hit.entity->skill[9]);
								list_RemoveNode(my->mynode);
								return;*/
						}
						else if (hit.entity->behavior == &actFurniture )
						{
							int damage = element->damage;
							damage /= (1 + (int)resistance);
							hit.entity->skill[4] -= damage;
							if ( hit.entity->skill[4] < 0 )
							{
								if ( parent )
								{
									if ( parent->behavior == &actPlayer )
									{
										if ( hit.entity->skill[0] )
										{
											// chair
											messagePlayer(parent->skill[2], language[388]);
											updateEnemyBar(parent, hit.entity, language[677], hit.entity->skill[4], hit.entity->skill[9]);
										}
										else
										{
											// table
											messagePlayer(parent->skill[2], language[389]);
											updateEnemyBar(parent, hit.entity, language[676], hit.entity->skill[4], hit.entity->skill[9]);
										}
									}
								}
							}
							playSoundEntity(hit.entity, 28, 128);
							if ( my->light != NULL )
							{
								list_RemoveNode(my->light->node);
								my->light = NULL;
							}
							list_RemoveNode(my->mynode);
							return;
						}
					}
				}
				else if (!strcmp(element->name, spellElement_locking.name))
				{
					if ( hit.entity )
					{
						if (hit.entity->behavior == &actDoor)
						{
							playSoundEntity(hit.entity, 92, 64);
							hit.entity->skill[5] = 1; //Lock the door.
							if ( parent )
							{
								if ( parent->behavior == &actPlayer )
								{
									messagePlayer(parent->skill[2], language[399]);
								}
							}
						}
						else if (hit.entity->behavior == &actChest)
						{
							//Lock chest
							playSoundEntity(hit.entity, 92, 64);
							if ( !hit.entity->chestLocked )
							{
								hit.entity->lockChest();
								if ( parent )
								{
									if ( parent->behavior == &actPlayer )
									{
										messagePlayer(parent->skill[2], language[400]);
									}
								}
							}
						}
						else
						{
							if ( parent )
								if ( parent->behavior == &actPlayer )
								{
									messagePlayer(parent->skill[2], language[401]);
								}
							if ( player >= 0 )
							{
								messagePlayer(player, language[402]);
							}
						}
						spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my->sprite);
						if (my->light != NULL)
						{
							list_RemoveNode(my->light->node);
							my->light = NULL;
						}
						list_RemoveNode(my->mynode);
						return;
					}
				}
				else if (!strcmp(element->name, spellElement_opening.name))
				{
					if (hit.entity)
					{
						if (hit.entity->behavior == &actDoor)
						{
                            playSoundEntity(hit.entity, 91, 64);
                            hit.entity->skill[5] = 0; // Unlock the door.

                            //Open door
                            if ( !hit.entity->skill[0] && !hit.entity->skill[3] )
                            {
                                hit.entity->skill[3] = 1 + (my->x > hit.entity->x);
                                playSoundEntity(hit.entity, 21, 96);
                            }
                            else if ( hit.entity->skill[0] && !hit.entity->skill[3] )
                            {
                                hit.entity->skill[3] = 1 + (my->x < hit.entity->x);
                                playSoundEntity(hit.entity, 21, 96);
                            }

							if ( parent )
								if ( parent->behavior == &actPlayer)
								{
									messagePlayer(parent->skill[2], language[402]);
								}
						}
						else if (hit.entity->behavior == &actGate)
						{
							//Open gate
							if ( hit.entity->skill[28] != 2 )
							{
								hit.entity->skill[28] = 2; // power it
								if ( parent )
									if ( parent->behavior == &actPlayer)
									{
										messagePlayer(parent->skill[2], language[403]);
									}
							}
						}
						else if (hit.entity->behavior == &actChest)
						{
							//Unlock chest
							if ( hit.entity->chestLocked )
							{
								playSoundEntity(hit.entity, 91, 64);
								hit.entity->unlockChest();
								if ( parent )
								{
									if ( parent->behavior == &actPlayer)
									{
										messagePlayer(parent->skill[2], language[404]);
									}
								}
							}
						}
						else if ( hit.entity->behavior == &actPowerCrystalBase )
						{
							Entity* childentity = static_cast<Entity*>((&hit.entity->children)->first->element);
							if ( childentity != nullptr )
							{

								//Unlock crystal
								if ( childentity->crystalSpellToActivate )
								{
									playSoundEntity(hit.entity, 151, 128);
									childentity->crystalSpellToActivate = 0;
									if ( parent )
									{
										if ( parent->behavior == &actPlayer )
										{
											messagePlayer(parent->skill[2], language[2358]);
										}
									}
								}
							}
						}
						else
						{
							if ( parent )
								if ( parent->behavior == &actPlayer )
								{
									messagePlayer(parent->skill[2], language[401]);
								}
							if ( player >= 0 )
							{
								messagePlayer(player, language[401]);
							}
						}
						spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my->sprite);
						if (my->light != NULL)
						{
							list_RemoveNode(my->light->node);
							my->light = NULL;
						}
						list_RemoveNode(my->mynode);
						return;
					}
				}
				else if (!strcmp(element->name, spellElement_dig.name))
				{
					if ( !hit.entity )
					{
						if ( hit.mapx >= 1 && hit.mapx < map.width - 1 && hit.mapy >= 1 && hit.mapy < map.height - 1 )
						{
							if ( map.tiles[(int)(OBSTACLELAYER + hit.mapy * MAPLAYERS + hit.mapx * MAPLAYERS * map.height)] != 0 )
							{
								playSoundEntity(my, 66, 128);
								playSoundEntity(my, 67, 128);

								// spawn several rock items //TODO: This should really be its own function.
								i = 8 + rand() % 4;
								for ( c = 0; c < i; c++ )
								{
									entity = newEntity(-1, 1, map.entities);
									entity->flags[INVISIBLE] = true;
									entity->flags[UPDATENEEDED] = true;
									entity->x = hit.mapx * 16 + 4 + rand() % 8;
									entity->y = hit.mapy * 16 + 4 + rand() % 8;
									entity->z = -6 + rand() % 12;
									entity->sizex = 4;
									entity->sizey = 4;
									entity->yaw = rand() % 360 * PI / 180;
									entity->vel_x = (rand() % 20 - 10) / 10.0;
									entity->vel_y = (rand() % 20 - 10) / 10.0;
									entity->vel_z = -.25 - (rand() % 5) / 10.0;
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

								map.tiles[(int)(OBSTACLELAYER + hit.mapy * MAPLAYERS + hit.mapx * MAPLAYERS * map.height)] = 0;
								// send wall destroy info to clients
								for ( c = 1; c < MAXPLAYERS; c++ )
								{
									if ( client_disconnected[c] == true )
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
						}
					}
					else
					{
						if ( hit.entity->behavior == &actBoulder )
						{
							int i = 8 + rand() % 4;

							int c;
							for ( c = 0; c < i; c++ )
							{
								Entity* entity = newEntity(-1, 1, map.entities);
								entity->flags[INVISIBLE] = true;
								entity->flags[UPDATENEEDED] = true;
								entity->x = hit.entity->x - 4 + rand() % 8;
								entity->y = hit.entity->y - 4 + rand() % 8;
								entity->z = -6 + rand() % 12;
								entity->sizex = 4;
								entity->sizey = 4;
								entity->yaw = rand() % 360 * PI / 180;
								entity->vel_x = (rand() % 20 - 10) / 10.0;
								entity->vel_y = (rand() % 20 - 10) / 10.0;
								entity->vel_z = -.25 - (rand() % 5) / 10.0;
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

							// destroy the boulder
							playSoundEntity(hit.entity, 67, 128);
							list_RemoveNode(hit.entity->mynode);
							if ( parent )
								if ( parent->behavior == &actPlayer )
								{
									messagePlayer(parent->skill[2], language[405]);
								}

							// on sokoban, destroying boulders spawns scorpions
							if ( !strcmp(map.name, "Sokoban") )
							{
								Entity* monster = summonMonster(SCORPION, ox, oy);
								if ( monster )
								{
									int c;
									for ( c = 0; c < MAXPLAYERS; c++ )
									{
										Uint32 color = SDL_MapRGB(mainsurface->format, 255, 128, 0);
										messagePlayerColor(c, color, language[406]);
									}
								}
							}
						}
						else
						{
							if ( parent )
								if ( parent->behavior == &actPlayer )
								{
									messagePlayer(parent->skill[2], language[401]);
								}
							if ( player >= 0 )
							{
								messagePlayer(player, language[401]);
							}
						}
					}
				}
				else if ( !strcmp(element->name, spellElement_stoneblood.name) )
				{
					if ( hit.entity )
					{
						if ( hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer )
						{
							if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
							{
								// test for friendly fire
								if ( parent && parent->checkFriend(hit.entity) )
								{
									if ( my->light != NULL )
									{
										list_RemoveNode(my->light->node);
										my->light = NULL;
									}
									list_RemoveNode(my->mynode);
									return;
								}
							}
							playSoundEntity(hit.entity, 172, 64); //TODO: Paralyze spell sound.
							hitstats->EFFECTS[EFF_PARALYZED] = true;
							hitstats->EFFECTS_TIMERS[EFF_PARALYZED] = (element->duration * (((element->mana) / static_cast<double>(element->base_mana)) * element->overload_multiplier));
							hitstats->EFFECTS_TIMERS[EFF_PARALYZED] /= (1 + (int)resistance);
							if ( hit.entity->behavior == &actPlayer )
							{
								serverUpdateEffects(hit.entity->skill[2]);
							}
							// update enemy bar for attacker
							if ( parent )
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								if ( parent->behavior == &actPlayer )
								{
									if ( strcmp(hitstats->name, "") )
									{
										messagePlayerColor(parent->skill[2], color, language[2420], hitstats->name);
									}
									else
									{
										if ( hitstats->type < KOBOLD ) //Original monster count
										{
											messagePlayerColor(parent->skill[2], color, language[2421], language[90 + hitstats->type]);
										}
										else if ( hitstats->type >= KOBOLD ) //New monsters
										{
											messagePlayerColor(parent->skill[2], color, language[2421], language[2000 + (hitstats->type - KOBOLD)]);
										}
									}
								}
							}

							Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
							if ( player >= 0 )
							{
								messagePlayerColor(player, color, language[2422]);
							}
							if ( my->light != NULL )
							{
								list_RemoveNode(my->light->node);
								my->light = NULL;
							}
							spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my->sprite);
							list_RemoveNode(my->mynode);
							return;
						}
					}
				}
				else if ( !strcmp(element->name, spellElement_bleed.name) )
				{
					playSoundEntity(my, 173, 128);
					if ( hit.entity )
					{
						if ( hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer )
						{
							Entity* parent = uidToEntity(my->parent);
							if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
							{
								// test for friendly fire
								if ( parent && parent->checkFriend(hit.entity) )
								{
									if ( my->light != NULL )
									{
										list_RemoveNode(my->light->node);
										my->light = NULL;
									}
									list_RemoveNode(my->mynode);
									return;
								}
							}
							playSoundEntity(my, 173, 64);
							playSoundEntity(hit.entity, 28, 128);
							int damage = element->damage;
							//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
							damage *= damagetables[hitstats->type][5];
							damage /= (1 + (int)resistance);
							hit.entity->modHP(-damage);

							// write the obituary
							if ( parent )
							{
								parent->killedByMonsterObituary(hit.entity);
							}

							hitstats->EFFECTS[EFF_BLEEDING] = true;
							hitstats->EFFECTS_TIMERS[EFF_BLEEDING] = (element->duration * (((element->mana) / static_cast<double>(element->base_mana)) * element->overload_multiplier));
							hitstats->EFFECTS_TIMERS[EFF_BLEEDING] /= (1 + (int)resistance);
							hitstats->EFFECTS[EFF_SLOW] = true;
							hitstats->EFFECTS_TIMERS[EFF_SLOW] = (element->duration * (((element->mana) / static_cast<double>(element->base_mana)) * element->overload_multiplier));
							hitstats->EFFECTS_TIMERS[EFF_SLOW] /= (1 + (int)resistance);
							if ( hit.entity->behavior == &actPlayer )
							{
								serverUpdateEffects(hit.entity->skill[2]);
							}
							// update enemy bar for attacker
							if ( parent )
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								if ( parent->behavior == &actPlayer )
								{
									if ( strcmp(hitstats->name, "") )
									{
										messagePlayerColor(parent->skill[2], color, language[2423], hitstats->name);
									}
									else
									{
										if ( hitstats->type < KOBOLD ) //Original monster count
										{
											messagePlayerColor(parent->skill[2], color, language[2424], language[90 + hitstats->type]);
										}
										else if ( hitstats->type >= KOBOLD ) //New monsters
										{
											messagePlayerColor(parent->skill[2], color, language[2424], language[2000 + (hitstats->type - KOBOLD)]);
										}
									}
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
								if ( hitstats->type < KOBOLD ) //Original monster count
								{
									updateEnemyBar(parent, hit.entity, language[90 + hitstats->type], hitstats->HP, hitstats->MAXHP);
								}
								else if ( hitstats->type >= KOBOLD ) //New monsters
								{
									updateEnemyBar(parent, hit.entity, language[2000 + (hitstats->type - KOBOLD)], hitstats->HP, hitstats->MAXHP);
								}
							}
							else
							{
								updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP);
							}

							if ( hitstats->HP <= 0 && parent )
							{
								parent->awardXP(hit.entity, true, true);
							}

							Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
							if ( player >= 0 )
							{
								messagePlayerColor(player, color, language[2425]);
							}
							if ( my->light != NULL )
							{
								list_RemoveNode(my->light->node);
								my->light = NULL;
							}
							list_RemoveNode(my->mynode);
							return;
						}
					}
				}
				else if ( !strcmp(element->name, spellElement_dominate.name) )
				{
					Entity *caster = uidToEntity(spell->caster);
					if ( caster )
					{
						if ( spellEffectDominate(*my, *element, *caster, parent) )
						{
							//Abort if successfully run, since do not need to execute the proceeding code..
							return;
						}
					}
				}

				if ( my->light != NULL )
				{
					list_RemoveNode(my->light->node);
					my->light = NULL;
				}
				list_RemoveNode(my->mynode);
				return;
			}
		}

		//Go down two levels to the next element. This will need to get re-written shortly.
		node = spell->elements.first;
		element = (spellElement_t*)node->element;
		//element = (spellElement_t *)spell->elements->first->element;
		//element = (spellElement_t *)element->elements->first->element; //Go down two levels to the second element.
		node = element->elements.first;
		element = (spellElement_t*)node->element;
		if (!strcmp(element->name, spellElement_fire.name) || !strcmp(element->name, spellElement_lightning.name))
		{
			//Make the ball light up stuff as it travels.
			my->light = lightSphereShadow(my->x / 16, my->y / 16, 8, 192);
			lightball_flicker++;
			my->skill[2] = -11; // so clients know to create a light field

			if (lightball_flicker > 5)
			{
				lightball_lighting = (lightball_lighting == 1) + 1;

				if (lightball_lighting == 1)
				{
					if ( my->light != NULL )
					{
						list_RemoveNode(my->light->node);
					}
					my->light = lightSphereShadow(my->x / 16, my->y / 16, 8, 192);
				}
				else
				{
					if ( my->light != NULL )
					{
						list_RemoveNode(my->light->node);
					}
					my->light = lightSphereShadow(my->x / 16, my->y / 16, 8, 174);
				}
				lightball_flicker = 0;
			}
		}
		else
		{
			my->skill[2] = -12; // so clients know to simply spawn particles
		}

		// spawn particles
		spawnMagicParticle(my);
	}
	else
	{
		//Any init stuff that needs to happen goes here.
		magic_init = 1;
		my->skill[2] = -7; // ordinarily the client won't do anything with this entity
	}
}

void actMagicClient(Entity* my)
{
	if ( my->light != NULL )
	{
		list_RemoveNode(my->light->node);
		my->light = NULL;
	}
	my->light = lightSphereShadow(my->x / 16, my->y / 16, 8, 192);
	lightball_flicker++;
	my->skill[2] = -11; // so clients know to create a light field

	if (lightball_flicker > 5)
	{
		lightball_lighting = (lightball_lighting == 1) + 1;

		if (lightball_lighting == 1)
		{
			if ( my->light != NULL )
			{
				list_RemoveNode(my->light->node);
			}
			my->light = lightSphereShadow(my->x / 16, my->y / 16, 8, 192);
		}
		else
		{
			if ( my->light != NULL )
			{
				list_RemoveNode(my->light->node);
			}
			my->light = lightSphereShadow(my->x / 16, my->y / 16, 8, 174);
		}
		lightball_flicker = 0;
	}

	// spawn particles
	spawnMagicParticle(my);
}

void actMagicClientNoLight(Entity* my)
{
	spawnMagicParticle(my); // simply spawn particles
}

void actMagicParticle(Entity* my)
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

Entity* spawnMagicParticle(Entity* parentent)
{
	Entity* entity;

	entity = newEntity(parentent->sprite, 1, map.entities);
	entity->x = parentent->x + (rand() % 50 - 25) / 20.f;
	entity->y = parentent->y + (rand() % 50 - 25) / 20.f;
	entity->z = parentent->z + (rand() % 50 - 25) / 20.f;
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
	entity->flags[BRIGHT] = true;
	entity->flags[UNCLICKABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[UPDATENEEDED] = false;
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
			if ( client_disconnected[c] )
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
		Entity* entity = newEntity(sprite, 1, map.entities);
		entity->x = x - 5 + rand() % 11;
		entity->y = y - 5 + rand() % 11;
		entity->z = z - 10 + rand() % 21;
		entity->scalex = 0.7;
		entity->scaley = 0.7;
		entity->scalez = 0.7;
		entity->sizex = 1;
		entity->sizey = 1;
		entity->yaw = (rand() % 360) * PI / 180.f;
		entity->flags[PASSABLE] = true;
		entity->flags[BRIGHT] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[UNCLICKABLE] = true;
		entity->behavior = &actMagicParticle;
		entity->vel_z = -1;
		if ( multiplayer != CLIENT )
		{
			entity_uids--;
		}
		entity->setUID(-3);
	}
}

void createParticle1(Entity* caster, int player)
{
	Entity* entity = newEntity(-1, 1, map.entities);
	entity->sizex = 0;
	entity->sizey = 0;
	entity->x = caster->x + 32 * cos(caster->yaw);
	entity->y = caster->y + 32 * sin(caster->yaw);
	entity->z = -7;
	entity->vel_z = 0.3;
	entity->yaw = (rand() % 360) * PI / 180.0;
	entity->skill[0] = 50;
	entity->skill[1] = player;
	entity->fskill[0] = 0.03;
	entity->light = lightSphereShadow(entity->x / 16, entity->y / 16, 3, 192);
	entity->behavior = &actParticleCircle;
	entity->flags[PASSABLE] = true;
	entity->flags[INVISIBLE] = true;
	entity->setUID(-3);

	createParticle2(entity);
	createParticleDot(entity);
	/*entity = newEntity(574, 1, map.entities);
	entity->sizex = 1;
	entity->sizey = 1;
	entity->x = x;
	entity->y = y;
	entity->z = 0;
	entity->yaw = (rand() % 360) * PI / 180.0;
	entity->skill[0] = 1000;
	entity->behavior = &actParticleCircle;
	entity->sprite = 574;
	entity->flags[PASSABLE] = true;
	entity->setUID(-3);*/
}

void createParticle2(Entity* parent)
{
	Entity* entity = newEntity(174, 1, map.entities);
	entity->sizex = 1;
	entity->sizey = 1;
	entity->x = parent->x;
	entity->y = parent->y;
	entity->focalx = 8;
	entity->z = -7;
	entity->vel_z = 0.3;
	entity->yaw = (rand() % 360) * PI / 180.0;
	entity->skill[0] = 60;
	entity->skill[1] = -1;
	//entity->scalex = 0.01;
	//entity->scaley = 0.01;
	entity->fskill[0] = -0.1;
	entity->behavior = &actParticleCircle;
	entity->flags[PASSABLE] = true;
	entity->setUID(-3);

	real_t tmp = entity->yaw;

	entity = newEntity(174, 1, map.entities);
	entity->sizex = 1;
	entity->sizey = 1;
	entity->x = parent->x;
	entity->y = parent->y;
	entity->focalx = 8;
	entity->z = -7;
	entity->vel_z = 0.3;
	entity->yaw = tmp + (2 * PI / 3);
	entity->skill[0] = 60;
	entity->skill[1] = -1;
	//entity->scalex = 0.01;
	//entity->scaley = 0.01;
	entity->fskill[0] = -0.1;
	entity->behavior = &actParticleCircle;
	entity->flags[PASSABLE] = true;
	entity->setUID(-3);

	entity = newEntity(174, 1, map.entities);
	entity->sizex = 1;
	entity->sizey = 1;
	entity->x = parent->x;
	entity->y = parent->y;
	entity->focalx = 8;
	entity->z = -7;
	entity->vel_z = 0.3;
	entity->yaw = tmp - (2 * PI / 3);
	entity->skill[0] = 60;
	entity->skill[1] = -1;
	//entity->scalex = 0.01;
	//entity->scaley = 0.01;
	entity->fskill[0] = -0.1;
	entity->behavior = &actParticleCircle;
	entity->flags[PASSABLE] = true;
	entity->setUID(-3);

	entity = newEntity(174, 1, map.entities);
	entity->sizex = 1;
	entity->sizey = 1;
	entity->x = parent->x;
	entity->y = parent->y;
	entity->focalx = 16;
	entity->z = -12;
	entity->vel_z = 0.4;
	entity->yaw = tmp;
	entity->skill[0] = 60;
	entity->skill[1] = -1;
	//entity->scalex = 0.01;
	//entity->scaley = 0.01;
	entity->fskill[0] = 0.1;
	entity->behavior = &actParticleCircle;
	entity->flags[PASSABLE] = true;
	entity->setUID(-3);

	entity = newEntity(174, 1, map.entities);
	entity->sizex = 1;
	entity->sizey = 1;
	entity->x = parent->x;
	entity->y = parent->y;
	entity->focalx = 16;
	entity->z = -12;
	entity->vel_z = 0.4;
	entity->yaw = tmp + (2 * PI / 3);
	entity->skill[0] = 60;
	entity->skill[1] = -1;
	//entity->scalex = 0.01;
	//entity->scaley = 0.01;
	entity->fskill[0] = 0.1;
	entity->behavior = &actParticleCircle;
	entity->flags[PASSABLE] = true;
	entity->setUID(-3);

	entity = newEntity(174, 1, map.entities);
	entity->sizex = 1;
	entity->sizey = 1;
	entity->x = parent->x;
	entity->y = parent->y;
	entity->focalx = 16;
	entity->z = -12;
	entity->vel_z = 0.4;
	entity->yaw = tmp - (2 * PI / 3);
	entity->skill[0] = 60;
	entity->skill[1] = -1;
	//entity->scalex = 0.01;
	//entity->scaley = 0.01;
	entity->fskill[0] = 0.1;
	entity->behavior = &actParticleCircle;
	entity->flags[PASSABLE] = true;
	entity->setUID(-3);
}

#define PARTICLE_LIFE my->skill[0]
#define PARTICLE_CASTER my->skill[1]

void actParticleCircle(Entity* my)
{
	//Entity* entity = newEntity(sprite, 1, map.entities);
	if ( PARTICLE_LIFE < 0 )
	{
		if ( PARTICLE_CASTER != -1 )
		{
			//spawnMagicEffectParticles(my->x, my->y, my->z, 171);
			//spell_summonFamiliar(PARTICLE_CASTER);
			playSoundEntity(my, 164, 128);
			spawnExplosion(my->x, my->y, 0);
			//summonMonster(SKELETON, my->x, my->y);
		}
		if ( my->light != NULL )
		{
			list_RemoveNode(my->light->node);
		}
		my->light = NULL;
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
		if ( PARTICLE_CASTER == -1 )
		{

			if ( my->focalx > 0.05 )
			{
				if ( my->vel_z == 0.3 )
				{
					my->focalx = my->focalx * 0.98;
				}
				else
				{
					my->focalx = my->focalx * 0.95;
				}
			}
			my->scalex *= 0.99;
			my->scaley *= 0.99;
			my->scalez *= 0.99;
			//my->scalex = my->scalex * 1.1;
			//my->scaley = my->scaley * 1.1;
		}
		//my->z -= 0.01;
	}
}

void createParticleDot(Entity* parent)
{
	for ( int c = 0; c < 50; c++ )
	{
		Entity* entity = newEntity(576, 1, map.entities);
		entity->sizex = 1;
		entity->sizey = 1;
		entity->x = parent->x + (-4 + rand() % 9);
		entity->y = parent->y + (-4 + rand() % 9);
		entity->z = 7.5 + rand()%50;
		entity->vel_z = -1;
		//entity->yaw = (rand() % 360) * PI / 180.0;
		entity->skill[0] = 10 + rand()% 50;
		entity->behavior = &actParticleDot;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[UNCLICKABLE] = true;
		if ( multiplayer != CLIENT )
		{
			entity_uids--;
		}
		entity->setUID(-3);
	}
}

void createParticleRock(Entity* parent)
{
	for ( int c = 0; c < 5; c++ )
	{
		Entity* entity = newEntity(78, 1, map.entities);
		entity->sizex = 1;
		entity->sizey = 1;
		entity->x = parent->x + (-4 + rand() % 9);
		entity->y = parent->y + (-4 + rand() % 9);
		entity->z = 7.5;
		entity->yaw = c * 2 * PI / 5;//(rand() % 360) * PI / 180.0;
		entity->roll = (rand() % 360) * PI / 180.0;

		entity->vel_x = 0.2 * cos(entity->yaw);
		entity->vel_y = 0.2 * sin(entity->yaw);
		entity->vel_z = 3;// 0.25 - (rand() % 5) / 10.0;
		
		entity->skill[0] = 50; // particle life
		entity->skill[1] = 0; // particle direction, 0 = upwards, 1 = downwards.

		entity->behavior = &actParticleRock;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[UNCLICKABLE] = true;
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
