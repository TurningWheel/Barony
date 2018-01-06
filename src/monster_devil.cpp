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
#include "sound.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"

void initDevil(Entity* my, Stat* myStats)
{
	int c;
	node_t* node;

	my->initMonster(304);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = -1;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 3;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{

		if (players[0] && players[0]->entity)
		{
			my->monsterTarget = players[0]->entity->getUID();
			my->monsterTargetX = players[0]->entity->x;
			my->monsterTargetY = players[0]->entity->y;
		}
	}

	// head
	Entity* entity = newEntity(303, 0, map.entities, nullptr); //Limb entity.
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

	// right bicep
	entity = newEntity(305, 0, map.entities, nullptr); //Limb entity.
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

	// right forearm
	entity = newEntity(306, 0, map.entities, nullptr); //Limb entity.
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

	// left bicep
	entity = newEntity(307, 0, map.entities, nullptr); //Limb entity.
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

	// left forearm
	entity = newEntity(308, 0, map.entities, nullptr); //Limb entity.
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
			if ( client_disconnected[c] )
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
	int x, y;
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
		messagePlayer(c, language[1112]);
		playSoundPlayer(c, 97, 128);
		stats[c]->STR += 20;
		stats[c]->DEX += 5;
		stats[c]->CON += 20;
		stats[c]->INT += 5;
		if ( multiplayer == SERVER && c > 0 )
		{
			strcpy((char*)net_packet->data, "ATTR");
			net_packet->data[4] = clientnum;
			net_packet->data[5] = (Sint8)stats[c]->STR;
			net_packet->data[6] = (Sint8)stats[c]->DEX;
			net_packet->data[7] = (Sint8)stats[c]->CON;
			net_packet->data[8] = (Sint8)stats[c]->INT;
			net_packet->data[9] = (Sint8)stats[c]->PER;
			net_packet->data[10] = (Sint8)stats[c]->CHR;
			net_packet->data[11] = (Sint8)stats[c]->EXP;
			net_packet->data[12] = (Sint8)stats[c]->LVL;
			SDLNet_Write16((Sint16)stats[c]->HP, &net_packet->data[13]);
			SDLNet_Write16((Sint16)stats[c]->MAXHP, &net_packet->data[15]);
			SDLNet_Write16((Sint16)stats[c]->MP, &net_packet->data[17]);
			SDLNet_Write16((Sint16)stats[c]->MAXMP, &net_packet->data[19]);
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 21;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
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
}
