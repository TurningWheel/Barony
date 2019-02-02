/*-------------------------------------------------------------------------------

	BARONY
	File: actbeartrap.cpp
	Desc: behavior function for set beartraps

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "messages.hpp"
#include "sound.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define BEARTRAP_CAUGHT my->skill[0]
#define BEARTRAP_STATUS my->skill[11]
#define BEARTRAP_BEATITUDE my->skill[12]
#define BEARTRAP_APPEARANCE my->skill[14]
#define BEARTRAP_IDENTIFIED my->skill[15]

void actBeartrap(Entity* my)
{
	int i;
	if ( my->sprite == 667 )
	{	
		my->roll = 0;
		my->z = 6.75;
	}

	if ( multiplayer == CLIENT )
	{
		return;
	}

	// undo beartrap
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if ( (i == 0 && selectedEntity == my) || (client_selected[i] == my) )
		{
			if (inrange[i])
			{
				Entity* entity = newEntity(-1, 1, map.entities, nullptr); //Item entity.
				entity->flags[INVISIBLE] = true;
				entity->flags[UPDATENEEDED] = true;
				entity->flags[PASSABLE] = true;
				entity->x = my->x;
				entity->y = my->y;
				entity->z = my->z;
				entity->sizex = my->sizex;
				entity->sizey = my->sizey;
				entity->yaw = my->yaw;
				entity->pitch = my->pitch;
				entity->roll = PI / 2;
				entity->behavior = &actItem;
				entity->skill[10] = TOOL_BEARTRAP;
				entity->skill[11] = BEARTRAP_STATUS;
				entity->skill[12] = BEARTRAP_BEATITUDE;
				entity->skill[13] = 1;
				entity->skill[14] = BEARTRAP_APPEARANCE;
				entity->skill[15] = BEARTRAP_IDENTIFIED;
				entity->itemNotMoving = 1;
				entity->itemNotMovingClient = 1;
				messagePlayer(i, language[1300]);
				list_RemoveNode(my->mynode);
				return;
			}
		}
	}

	if ( BEARTRAP_CAUGHT == 1 )
	{
		return;
	}

	// launch beartrap
	node_t* node;
	for ( node = map.creatures->first; node != nullptr; node = node->next )
	{
		Entity* entity = (Entity*)node->element;
		if ( my->parent == entity->getUID() )
		{
			continue;
		}
		if ( entity->behavior == &actMonster || entity->behavior == &actPlayer )
		{
			Stat* stat = entity->getStats();
			if ( stat )
			{
				Entity* parent = uidToEntity(my->parent);
				if ( (parent && parent->checkFriend(entity)) )
				{
					continue;
				}
				if ( entityDist(my, entity) < 6.5 )
				{
					entity->setEffect(EFF_PARALYZED, true, 200, false);
					entity->setEffect(EFF_BLEEDING, true, 300, false);
					int damage = 10 + 3 * (BEARTRAP_STATUS + BEARTRAP_BEATITUDE);
					Stat* trapperStat = nullptr;
					if ( parent && (trapperStat = parent->getStats()) )
					{
						damage += trapperStat->PROFICIENCIES[PRO_LOCKPICKING] / 5;
					}
					//messagePlayer(0, "dmg: %d", damage);
					entity->modHP(-damage);
					//// alert the monster! DOES NOT WORK DURING PARALYZE.
					//if ( entity->behavior == &actMonster && entity->monsterState != MONSTER_STATE_ATTACK && (stat->type < LICH || stat->type >= SHOPKEEPER) )
					//{
					//	Entity* attackTarget = uidToEntity(my->parent);
					//	if ( attackTarget )
					//	{
					//		entity->monsterAcquireAttackTarget(*attackTarget, MONSTER_STATE_PATH);
					//	}
					//}
					// set obituary
					entity->setObituary(language[1504]);

					if ( stat->HP <= 0 )
					{
						if ( parent )
						{
							parent->awardXP( entity, true, true );
						}
					}
					if ( entity->behavior == &actPlayer )
					{
						int player = entity->skill[2];
						Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
						messagePlayerColor(player, color, language[454]);
						if ( player > 0 )
						{
							serverUpdateEffects(player);
						}
						if ( player == clientnum )
						{
							camera_shakex += .1;
							camera_shakey += 10;
						}
						else if ( player > 0 && multiplayer == SERVER )
						{
							strcpy((char*)net_packet->data, "SHAK");
							net_packet->data[4] = 10; // turns into .1
							net_packet->data[5] = 10;
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}
					}
					else if ( parent && parent->behavior == &actPlayer )
					{
						int player = parent->skill[2];
						if ( player >= 0 )
						{
							if ( entityDist(my, parent) >= 64 && entityDist(my, parent) < 128 )
							{
								messagePlayer(player, language[2521]);
							}
							else
							{
								messagePlayer(player, language[2522]);
							}
							parent->increaseSkill(PRO_LOCKPICKING);
							if ( rand() % 2 == 0 )
							{
								parent->increaseSkill(PRO_RANGED);
							}
							// update enemy bar for attacker
							if ( !strcmp(stat->name, "") )
							{
								if ( stat->type < KOBOLD ) //Original monster count
								{
									updateEnemyBar(parent, entity, language[90 + stat->type], stat->HP, stat->MAXHP);
								}
								else if ( stat->type >= KOBOLD ) //New monsters
								{
									updateEnemyBar(parent, entity, language[2000 + (stat->type - KOBOLD)], stat->HP, stat->MAXHP);
								}
							}
							else
							{
								updateEnemyBar(parent, entity, stat->name, stat->HP, stat->MAXHP);
							}
						}
					}
					playSoundEntity(my, 76, 64);
					playSoundEntity(entity, 28, 64);
					Entity* gib = spawnGib(entity);
					serverSpawnGibForClient(gib);

					--BEARTRAP_STATUS;
					if ( BEARTRAP_STATUS < DECREPIT )
					{
						// make first arm
						entity = newEntity(668, 1, map.entities, nullptr); //Special effect entity.
						entity->behavior = &actBeartrapLaunched;
						entity->flags[PASSABLE] = true;
						entity->flags[UPDATENEEDED] = true;
						entity->x = my->x;
						entity->y = my->y;
						entity->z = my->z + 1;
						entity->yaw = my->yaw;
						entity->pitch = my->pitch;
						entity->roll = 0;

						// and then the second
						entity = newEntity(668, 1, map.entities, nullptr); //Special effect entity.
						entity->behavior = &actBeartrapLaunched;
						entity->flags[PASSABLE] = true;
						entity->flags[UPDATENEEDED] = true;
						entity->x = my->x;
						entity->y = my->y;
						entity->z = my->z + 1;
						entity->yaw = my->yaw;
						entity->pitch = my->pitch;
						entity->roll = PI;
						list_RemoveNode(my->mynode);
					}
					else
					{
						BEARTRAP_CAUGHT = 1;
						my->sprite = 667;
						serverUpdateEntitySkill(my, 0);
					}
					return;
				}
			}
		}
	}
}

void actBeartrapLaunched(Entity* my)
{
	if ( my->ticks >= 200 )
	{
		list_RemoveNode(my->mynode);
		return;
	}
}
