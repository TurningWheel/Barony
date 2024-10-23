/*-------------------------------------------------------------------------------

	BARONY
	File: monster_devil.cpp
	Desc: implements all of the devil monster's code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "monster.hpp"
#include "engine/audio/sound.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "scores.hpp"
#include "magic/magic.hpp"
#include "prng.hpp"

void initDevil(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	my->flags[BURNABLE] = false;
	my->initMonster(304);
	my->z = -4;
	my->sizex = 20;
	my->sizey = 20;
	my->yaw = PI;

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = -1;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 3;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		if ( myStats->HP == 1250 )
		{
			for ( c = 0; c < MAXPLAYERS; ++c )
			{
				if ( !client_disconnected[c] )
				{
					myStats->MAXHP += 250;
				}
			}
			myStats->HP = myStats->MAXHP;
			myStats->OLDHP = myStats->HP;
		}

		if (players[0] && players[0]->entity)
		{
			my->monsterTarget = players[0]->entity->getUID();
			my->monsterTargetX = players[0]->entity->x;
			my->monsterTargetY = players[0]->entity->y;
		}

		my->setHardcoreStats(*myStats);
	}

	// head
	Entity* entity = newEntity(303, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[DEVIL][1][0]; // 2.5
	entity->focaly = limbs[DEVIL][1][1]; // 0
	entity->focalz = limbs[DEVIL][1][2]; // -4
	entity->behavior = &actDevilLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right bicep
	entity = newEntity(305, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[DEVIL][2][0]; // 0
	entity->focaly = limbs[DEVIL][2][1]; // 18
	entity->focalz = limbs[DEVIL][2][2]; // 6
	entity->behavior = &actDevilLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right forearm
	entity = newEntity(306, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[DEVIL][3][0]; // 0
	entity->focaly = limbs[DEVIL][3][1]; // 17
	entity->focalz = limbs[DEVIL][3][2]; // 26
	entity->behavior = &actDevilLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left bicep
	entity = newEntity(307, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[DEVIL][4][0]; // 0
	entity->focaly = limbs[DEVIL][4][1]; // -18
	entity->focalz = limbs[DEVIL][4][2]; // 6
	entity->behavior = &actDevilLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left forearm
	entity = newEntity(308, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[DEVIL][5][0]; // 0
	entity->focaly = limbs[DEVIL][5][1]; // -17
	entity->focalz = limbs[DEVIL][5][2]; // 26
	entity->behavior = &actDevilLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);
}

void actDevilLimb(Entity* my)
{
	my->actMonsterLimb();
}

void devilDie(Entity* my)
{
	node_t* node;

	int c;
	for ( c = 0; c < 5; c++ )
	{
		Entity* gib = spawnGib(my);
		serverSpawnGibForClient(gib);
	}
	//playSoundEntity(my, 28, 128);

	my->removeMonsterDeathNodes();

	if ( multiplayer == SERVER )
	{
		for ( c = 1; c < MAXPLAYERS; c++ )
		{
			if ( client_disconnected[c] || players[c]->isLocalPlayer() )
			{
				continue;
			}
			strcpy((char*)net_packet->data, "BDTH");
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 4;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}
	unsigned int x = 0;
	unsigned int y = 0;
	for ( y = map.height / 2 - 1; y < map.height / 2 + 2; y++ )
	{
		for ( x = 3; x < map.width / 2; x++ )
		{
			if ( !map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height] )
			{
				map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height] = 72;
			}
		}
	}
	for ( node = map.entities->first; node != NULL; node = node->next )
	{
		Entity* entity = (Entity*)node->element;
		if ( entity->skill[28] )
		{
			entity->skill[28] = 2;
		}
	}
	list_RemoveNode(my->mynode);
	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		steamAchievementClient(c, "BARONY_ACH_EVIL_INCARNATE");
		if ( completionTime < 20 * 60 * TICKS_PER_SECOND
			&& currentlevel >= 24 )
		{
			//messagePlayer(c, "completion time: %d", completionTime);
			steamAchievementClient(c, "BARONY_ACH_BOOTS_OF_SPEED");
		}
		//messagePlayer(c, Language::get(1112));
		//playSoundPlayer(c, 97, 128);
		//stats[c]->STR += 20;
		//stats[c]->DEX += 5;
		//stats[c]->CON += 20;
		//stats[c]->INT += 5;
		//if ( multiplayer == SERVER && c > 0 )
		//{
		//	strcpy((char*)net_packet->data, "ATTR");
		//	net_packet->data[4] = clientnum;
		//	net_packet->data[5] = (Sint8)stats[c]->STR;
		//	net_packet->data[6] = (Sint8)stats[c]->DEX;
		//	net_packet->data[7] = (Sint8)stats[c]->CON;
		//	net_packet->data[8] = (Sint8)stats[c]->INT;
		//	net_packet->data[9] = (Sint8)stats[c]->PER;
		//	net_packet->data[10] = (Sint8)stats[c]->CHR;
		//	net_packet->data[11] = (Sint8)stats[c]->EXP;
		//	net_packet->data[12] = (Sint8)stats[c]->LVL;
		//	SDLNet_Write16((Sint16)stats[c]->HP, &net_packet->data[13]);
		//	SDLNet_Write16((Sint16)stats[c]->MAXHP, &net_packet->data[15]);
		//	SDLNet_Write16((Sint16)stats[c]->MP, &net_packet->data[17]);
		//	SDLNet_Write16((Sint16)stats[c]->MAXMP, &net_packet->data[19]);
		//	net_packet->address.host = net_clients[c - 1].host;
		//	net_packet->address.port = net_clients[c - 1].port;
		//	net_packet->len = 21;
		//	sendPacketSafe(net_sock, -1, net_packet, c - 1);
		//}
	}
	return;
}

void devilMoveBodyparts(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = NULL;
	Entity* rightbody = NULL;
	Entity* leftbody = NULL;
	int bodypart;

	// set invisibility //TODO: isInvisible()?
	if ( multiplayer != CLIENT )
	{
		if ( myStats->EFFECTS[EFF_INVISIBLE] == true )
		{
			my->flags[INVISIBLE] = true;
			my->flags[BLOCKSIGHT] = false;
			bodypart = 0;
			for (node = my->children.first; node != NULL; node = node->next)
			{
				if ( bodypart < 2 )
				{
					bodypart++;
					continue;
				}
				if ( bodypart >= 7 )
				{
					break;
				}
				entity = (Entity*)node->element;
				if ( !entity->flags[INVISIBLE] )
				{
					entity->flags[INVISIBLE] = true;
					serverUpdateEntityBodypart(my, bodypart);
				}
				bodypart++;
			}
		}
		else
		{
			my->flags[INVISIBLE] = false;
			my->flags[BLOCKSIGHT] = true;
			bodypart = 0;
			for (node = my->children.first; node != NULL; node = node->next)
			{
				if ( bodypart < 2 )
				{
					bodypart++;
					continue;
				}
				if ( bodypart >= 7 )
				{
					break;
				}
				entity = (Entity*)node->element;
				if ( entity->flags[INVISIBLE] )
				{
					entity->flags[INVISIBLE] = false;
					serverUpdateEntityBodypart(my, bodypart);
					serverUpdateEntityFlag(my, INVISIBLE);
				}
				bodypart++;
			}
		}
	}

	/*if( ticks%120-ticks%60 )
		MONSTER_ARMBENDED = 1;
	else
		MONSTER_ARMBENDED = 0;*/

	//Move bodyparts
	for (bodypart = 0, node = my->children.first; node != NULL; node = node->next, bodypart++)
	{
		entity = (Entity*)node->element;
		if ( bodypart < 2 )
		{
			continue;
		}
		else if ( bodypart > 2 )
		{
			//entity->roll += .1;
			//entity->pitch += .1;
		}
		if ( bodypart == 3 )
		{
			rightbody = entity;
		}
		if ( bodypart == 5 )
		{
			leftbody = entity;
		}
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		if ( bodypart != 2 || my->ticks < 60 || MONSTER_ATTACK == 4 )
		{
			entity->yaw = my->yaw;
		}

		if ( MONSTER_ATTACK == 0 && bodypart != 2 )
		{
			entity->pitch = 0;
		}
		if ( bodypart == 3 )
		{
			if ( MONSTER_ATTACK == 1 || MONSTER_ATTACK == 3 )
			{
				if ( MONSTER_ATTACKTIME < 30 )
				{
					entity->pitch = std::max<real_t>(entity->pitch - .1, -PI / 2);
				}
				else if ( MONSTER_ATTACKTIME > 40 )
				{
					if ( entity->pitch < 0 )
					{
						entity->pitch = std::min<real_t>(entity->pitch + .4, 0.0);
						if ( entity->pitch >= 0 )
						{
							playSound(181, 64);
						}
					}
				}
			}
			else if ( MONSTER_ATTACK == 4 )
			{
				entity->pitch = std::max<real_t>(entity->pitch - .1, -4 * PI / 5);
			}
			else if ( MONSTER_ATTACK == 5 )
			{
				entity->pitch = -2 * PI / 5;
			}
			else
			{
				entity->pitch = 0;
			}
		}
		else if ( bodypart == 5 )
		{
			if ( MONSTER_ATTACK == 2 || MONSTER_ATTACK == 3 )
			{
				if ( MONSTER_ATTACKTIME < 30 )
				{
					entity->pitch = std::max<real_t>(entity->pitch - .1, -PI / 2);
				}
				else if ( MONSTER_ATTACKTIME > 40 )
				{
					if ( entity->pitch < 0 )
					{
						entity->pitch = std::min<real_t>(entity->pitch + .4, 0.0);
						if ( entity->pitch >= 0 )
						{
							playSound(181, 64);
						}
					}
				}
			}
			else if ( MONSTER_ATTACK == 4 )
			{
				entity->pitch = std::max<real_t>(entity->pitch - .1, -4 * PI / 5);
			}
			else if ( MONSTER_ATTACK == 6 )
			{
				entity->pitch = -2 * PI / 5;
			}
			else
			{
				entity->pitch = 0;
			}
		}
		if ( MONSTER_ATTACKTIME > 90 && MONSTER_ATTACK != 4 )
		{
			MONSTER_ATTACK = 0;
		}

		if ( MONSTER_WEAPONYAW > PI / 4 )
		{
			MONSTER_WEAPONYAW = 0;
			MONSTER_ATTACK = 0;
			MONSTER_ATTACKTIME = 0;
		}

		if ( MONSTER_ATTACK >= 5 )
		{
			if ( MONSTER_ATTACKTIME == 0 )
			{
				MONSTER_WEAPONYAW = -PI / 3;
			}
			else
			{
				MONSTER_WEAPONYAW += .02;
			}
		}

		switch ( bodypart )
		{
			// head
			case 2:
			{
				entity->z -= 16;
				node_t* tempNode;
				Entity* playertotrack = nullptr;
				for ( tempNode = map.creatures->first; tempNode != nullptr; tempNode = tempNode->next ) //Searching for players only? Don't search full map.entities then.
				{
					Entity* tempEntity = (Entity*)tempNode->element;
					double lowestdist = 5000;
					if ( tempEntity->behavior == &actPlayer )
					{
						double disttoplayer = entityDist(my, tempEntity);
						if ( disttoplayer < lowestdist )
						{
							playertotrack = tempEntity;
						}
					}
				}
				if ( playertotrack && !MONSTER_ATTACK )
				{
					double tangent = atan2( playertotrack->y - entity->y, playertotrack->x - entity->x );
					double dir = entity->yaw - tangent;
					while ( dir >= PI )
					{
						dir -= PI * 2;
					}
					while ( dir < -PI )
					{
						dir += PI * 2;
					}
					entity->yaw -= dir / 8;

					double dir2 = my->yaw - tangent;
					while ( dir2 >= PI )
					{
						dir2 -= PI * 2;
					}
					while ( dir2 < -PI )
					{
						dir2 += PI * 2;
					}
					if ( dir2 > PI / 2 )
					{
						entity->yaw = my->yaw - PI / 2;
					}
					else if ( dir2 < -PI / 2 )
					{
						entity->yaw = my->yaw + PI / 2;
					}
				}
				else
				{
					if ( MONSTER_ATTACKTIME == 0 )
					{
						entity->yaw = my->yaw;
					}
					else
					{
						if ( MONSTER_ATTACK == 1 )
						{
							entity->yaw = std::min<real_t>(entity->yaw + .1, my->yaw + PI / 6);
						}
						else if ( MONSTER_ATTACK == 2 )
						{
							entity->yaw = std::max<real_t>(entity->yaw - .1, my->yaw - PI / 6);
						}
					}
				}
				if ( MONSTER_ATTACK == 4 )
				{
					entity->pitch = std::max<real_t>(entity->pitch - .1, -PI / 6);
				}
				else
				{
					entity->pitch = std::min<real_t>(entity->pitch + .1, 0.0);
				}
				break;
			}
			// right bicep
			case 3:
				entity->z -= 8;
				if ( MONSTER_ATTACK == 1 || MONSTER_ATTACK == 3 || MONSTER_ATTACK == 4 )
				{
					entity->yaw += PI / 4;
				}
				if ( MONSTER_ATTACK == 5 )
				{
					entity->yaw += MONSTER_WEAPONYAW;
				}
				break;
			// right forearm
			case 4:
				if ( !MONSTER_ARMBENDED && MONSTER_ATTACK != 1 && MONSTER_ATTACK != 3 )
				{
					entity->focalx = limbs[DEVIL][3][0]; // 0
					entity->focaly = limbs[DEVIL][3][1]; // 17
					entity->focalz = limbs[DEVIL][3][2]; // 26
					entity->pitch = rightbody->pitch;
				}
				else
				{
					entity->focalx = limbs[DEVIL][3][0] - 18; // -18
					entity->focaly = limbs[DEVIL][3][1]; // 17
					entity->focalz = limbs[DEVIL][3][2] - 16; // 10
					entity->pitch = rightbody->pitch - PI / 2;
				}
				entity->z -= 8;
				if ( MONSTER_ATTACK == 1 || MONSTER_ATTACK == 3 || MONSTER_ATTACK == 4 )
				{
					entity->yaw += PI / 4;
				}
				if ( MONSTER_ATTACK == 5 )
				{
					entity->yaw += MONSTER_WEAPONYAW;
				}
				break;
			// left bicep
			case 5:
				entity->z -= 8;
				if ( MONSTER_ATTACK == 2 || MONSTER_ATTACK == 3 || MONSTER_ATTACK == 4 )
				{
					entity->yaw -= PI / 4;
				}
				if ( MONSTER_ATTACK == 6 )
				{
					entity->yaw -= MONSTER_WEAPONYAW;
				}
				break;
			// left forearm
			case 6:
				if ( !MONSTER_ARMBENDED && MONSTER_ATTACK != 2 && MONSTER_ATTACK != 3 )
				{
					entity->focalx = limbs[DEVIL][5][0]; // 0
					entity->focaly = limbs[DEVIL][5][1]; // -17
					entity->focalz = limbs[DEVIL][5][2]; // 26
					entity->pitch = leftbody->pitch;
				}
				else
				{
					entity->focalx = limbs[DEVIL][5][0] - 18; // -18
					entity->focaly = limbs[DEVIL][5][1]; // -17
					entity->focalz = limbs[DEVIL][5][2] - 16; // 10
					entity->pitch = leftbody->pitch - PI / 2;
				}
				entity->z -= 8;
				if ( MONSTER_ATTACK == 2 || MONSTER_ATTACK == 3 || MONSTER_ATTACK == 4 )
				{
					entity->yaw -= PI / 4;
				}
				if ( MONSTER_ATTACK == 6 )
				{
					entity->yaw -= MONSTER_WEAPONYAW;
				}
				break;
		}
	}
	if ( MONSTER_ATTACK != 0 )
	{
		MONSTER_ATTACKTIME++;
	}
	else
	{
		MONSTER_ATTACKTIME = 0;
	}
}

void actDevilTeleport(Entity* my)
{
	// dummy function
	my->flags[PASSABLE] = true;
}
bool Entity::devilSummonMonster(Entity* summonOnEntity, Monster creature, int radiusFromCenter, int playerToTarget)
{
	Entity* target = nullptr;
	if ( summonOnEntity )
	{
		target = summonOnEntity;
	}
	else
	{
		for ( node_t* searchNode = map.entities->first; searchNode != nullptr; searchNode = searchNode->next )
		{
			target = (Entity*)searchNode->element;
			if ( target->behavior == &actDevilTeleport
				&& target->sprite == 72 )
			{
				break; // found specified center of map
			}
			target = nullptr;
		}
	}
	if ( target )
	{
		int hellArena_x0 = 17;
		int hellArena_x1 = 47;
		int hellArena_y0 = 17;
		int hellArena_y1 = 47;
		int spawn_x = static_cast<int>(target->x / 16);
		int spawn_y = static_cast<int>(target->y / 16);
		std::vector<std::pair<int, int>> goodspots;
		for ( int j = std::max(hellArena_y0, spawn_y - radiusFromCenter); j <= std::min(hellArena_y1, spawn_y + radiusFromCenter); ++j )
		{
			for ( int i = std::max(hellArena_x0, spawn_x - radiusFromCenter); i <= std::min(hellArena_x1, spawn_x + radiusFromCenter); ++i )
			{
				int index = (j)* MAPLAYERS + (i)* MAPLAYERS * map.height;
				if ( !map.tiles[OBSTACLELAYER + index] &&
					((target->behavior == &actPlayer && !map.tiles[index])
						|| (target->behavior != &actPlayer 
								&& (map.tiles[index] || creature != DEMON) && !swimmingtiles[map.tiles[index]] && !lavatiles[map.tiles[index]] )
					) 
				)
				{
					// spawn on no floor, or lava if the target is a player.

					// otherwise, spawn on solid ground.
					real_t oldx = this->x;
					real_t oldy = this->y;
					this->x = i * 16 + 8;
					this->y = j * 16 + 8;
					goodspots.push_back(std::make_pair(i, j));
					this->x = oldx;
					this->y = oldy;
				}
			}
		}
		if ( goodspots.empty() )
		{
			return false;
		}
		std::pair<int,int> chosen = goodspots.at(local_rng.rand() % goodspots.size());
		Entity* timer = createParticleTimer(this, 70, 174);
		timer->x = chosen.first * 16.0 + 8;
		timer->y = chosen.second * 16.0 + 8;
		timer->z = 0;
		timer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_DEVIL_SUMMON_MONSTER;
		timer->particleTimerCountdownSprite = 174;
		timer->particleTimerEndAction = PARTICLE_EFFECT_DEVIL_SUMMON_MONSTER;
		timer->particleTimerVariable1 = creature;
		timer->particleTimerVariable2 = playerToTarget;
		serverSpawnMiscParticlesAtLocation(static_cast<Sint16>(chosen.first), static_cast<Sint16>(chosen.second), 0, PARTICLE_EFFECT_DEVIL_SUMMON_MONSTER, 174);

		monsterDevilNumSummons++;
		return true;
	}
	return false;
}

int Entity::devilGetNumMonstersInArena(Monster creature)
{
	int hellArena_x0 = 15;
	int hellArena_x1 = 49;
	int hellArena_y0 = 15;
	int hellArena_y1 = 49;
	int numMonstersActiveInArena = 0;
	node_t* tempNode;
	for ( tempNode = map.creatures->first; tempNode != nullptr; tempNode = tempNode->next )
	{
		Entity* monster = (Entity*)tempNode->element;
		if ( monster && monster->getMonsterTypeFromSprite() == creature )
		{
			if ( static_cast<int>(monster->x / 16) >= hellArena_x0 && static_cast<int>(monster->x / 16) <= hellArena_x1 )
			{
				if ( static_cast<int>(monster->y / 16) >= hellArena_y0 && static_cast<int>(monster->y / 16) <= hellArena_y1 )
				{
					++numMonstersActiveInArena;
				}
			}
		}
	}
	return numMonstersActiveInArena;
}

bool Entity::devilBoulderSummonIfPlayerIsHiding(int player)
{
	if ( players[player] && players[player]->entity )
	{
		int player_x = static_cast<int>(players[player]->entity->x / 16);
		int player_y = static_cast<int>(players[player]->entity->y / 16);
		int doSummon = 0;
		if ( entityDist(this, players[player]->entity) > 16 * 16 /*16 tiles*/ )
		{
			doSummon = 1;
		}
		else if ( !map.tiles[player_y * MAPLAYERS + player_x * MAPLAYERS * map.height] )
		{
			if ( entityDist(this, players[player]->entity) > 16 * 10 /*10 tiles*/ )
			{
				doSummon = 2;
			}
			else
			{
				// standing on no floor.
				real_t tangent = atan2(players[player]->entity->y - this->y, players[player]->entity->x - this->x);
				Entity* ohitentity = hit.entity;
				lineTraceTarget(this, this->x, this->y, tangent, 1024, 0, false, players[player]->entity);
				if ( hit.entity != players[player]->entity )
				{
					// can't see the player
					doSummon = 2;
				}
				hit.entity = ohitentity;
			}
		}
		//messagePlayer(0, "dosummon: %d, distance: %f", doSummon, entityDist(this, players[player]->entity));
		if ( doSummon )
		{
			int numPlayers = 0;
			for ( int c = 0; c < MAXPLAYERS; ++c )
			{
				if ( !client_disconnected[c] )
				{
					++numPlayers;
				}
			}
			if ( devilGetNumMonstersInArena(SHADOW) <= numPlayers && (local_rng.rand() % 4 == 0) )
			{
				if ( !devilSummonMonster(players[player]->entity, SHADOW, 5, player) )
				{
					devilSummonMonster(players[player]->entity, SHADOW, 21, player);
				}
				return true;
			}
			else
			{
				int numImps = devilGetNumMonstersInArena(CREATURE_IMP);
				if ( numImps <= (4 + numPlayers) && !devilSummonMonster(players[player]->entity, CREATURE_IMP, 5, player) )
				{
					devilSummonMonster(players[player]->entity, CREATURE_IMP, 21, player);
				}
				++numImps;
				if ( doSummon == 1 && numImps <= (4 + numPlayers) )
				{
					// extra imp.
					if ( !devilSummonMonster(players[player]->entity, CREATURE_IMP, 5, player) )
					{
						devilSummonMonster(players[player]->entity, CREATURE_IMP, 21, player);
					}
				}
				return true;
			}
		}
	}
	return false;
}