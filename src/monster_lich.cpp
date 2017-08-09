/*-------------------------------------------------------------------------------

	BARONY
	File: monster_lich.cpp
	Desc: implements all of the lich monster's code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "monster.hpp"
#include "sound.hpp"
#include "items.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"

void initLich(Entity* my, Stat* myStats)
{
	int c;

	my->flags[UPDATENEEDED] = true;
	my->flags[INVISIBLE] = false;

	my->sprite = 274;
	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 120;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 1;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		if ( myStats != NULL )
		{
			if ( !myStats->leader_uid )
			{
				myStats->leader_uid = 0;
			}

			// apply random stat increases if set in stat_shared.cpp or editor
			setRandomMonsterStats(myStats);

			// generate 6 items max, less if there are any forced items from boss variants
			int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;

			// boss variants

			// random effects
			myStats->EFFECTS[EFF_LEVITATING] = true;
			myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 0;

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

														 // count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			// generate the default inventory items for the monster, provided the editor sprite allowed enough default slots
			switch ( defaultItems )
			{
				case 6:
				case 5:
				case 4:
				case 3:
				case 2:
				case 1:
				default:
					break;
			}

			//give weapon
			if ( myStats->weapon == NULL && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
			{
				myStats->weapon = newItem(SPELLBOOK_LIGHTNING, EXCELLENT, 0, 1, 0, false, NULL);
			}
		}
	}

	// right arm
	Entity* entity = newEntity(276, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[LICH][1][0]; // 0
	entity->focaly = limbs[LICH][1][1]; // 0
	entity->focalz = limbs[LICH][1][2]; // 2
	entity->behavior = &actLichLimb;
	entity->parent = my->getUID();
	node_t* node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// left arm
	entity = newEntity(275, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[LICH][2][0]; // 0
	entity->focaly = limbs[LICH][2][1]; // 0
	entity->focalz = limbs[LICH][2][2]; // 2
	entity->behavior = &actLichLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// head
	entity = newEntity(277, 0, map.entities);
	entity->yaw = my->yaw;
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[LICH][3][0]; // 0
	entity->focaly = limbs[LICH][3][1]; // 0
	entity->focalz = limbs[LICH][3][2]; // -2
	entity->behavior = &actLichLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
}

void lichDie(Entity* my)
{
	node_t* node, *nextnode;
	int c;
	for ( c = 0; c < 20; c++ )
	{
		Entity* entity = spawnGib(my);
		if ( entity )
		{
			switch ( c )
			{
				case 0:
					entity->sprite = 230;
					break;
				case 1:
					entity->sprite = 231;
					break;
				case 2:
					entity->sprite = 233;
					break;
				case 3:
					entity->sprite = 235;
					break;
				case 4:
					entity->sprite = 236;
					break;
				case 5:
					entity->sprite = 274;
					break;
				case 6:
					entity->sprite = 275;
					break;
				case 7:
					entity->sprite = 276;
					break;
				case 8:
					entity->sprite = 277;
					break;
				default:
					break;
			}
			serverSpawnGibForClient(entity);
		}
	}

	my->removeMonsterDeathNodes();

	//playSoundEntity(my, 94, 128);
	if ( my->light )
	{
		list_RemoveNode(my->light->node);
		my->light = NULL;
	}
	// kill all other monsters on the level
	for ( node = map.entities->first; node != NULL; node = nextnode )
	{
		nextnode = node->next;
		Entity* entity = (Entity*)node->element;
		if ( entity == my )
		{
			continue;
		}
		if ( entity->behavior == &actMonster )
		{
			spawnExplosion(entity->x, entity->y, entity->z);
			Stat* stats = entity->getStats();
			if ( stats )
				if ( stats->type != HUMAN )
				{
					stats->HP = 0;
				}
		}
	}
	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		playSoundPlayer(c, 153, 128);
		steamAchievementClient(c, "BARONY_ACH_LICH_HUNTER");
	}
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
	spawnExplosion(my->x, my->y, my->z);
	list_RemoveNode(my->mynode);
	return;
}

void actLichLimb(Entity* my)
{
	my->actMonsterLimb();
}

void lichAnimate(Entity* my, double dist)
{
	node_t* node;
	Entity* entity;
	int bodypart;

	// remove old light field
	if ( my->light )
	{
		list_RemoveNode(my->light->node);
		my->light = NULL;
	}

	// set invisibility //TODO: isInvisible()?
	if ( multiplayer != CLIENT )
	{
		Stat* myStats = my->getStats();
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
				entity = (Entity*)node->element;
				if ( entity->flags[INVISIBLE] )
				{
					entity->flags[INVISIBLE] = false;
					serverUpdateEntityBodypart(my, bodypart);
				}
				bodypart++;
			}
		}
		if ( myStats->HP > myStats->MAXHP / 2 )
		{
			my->light = lightSphereShadow(my->x / 16, my->y / 16, 4, 192);
		}
		else if ( !my->skill[27] )
		{
			my->skill[27] = 1;
			serverUpdateEntitySkill(my, 27);
		}
	}
	else
	{
		if ( !my->skill[27] )
		{
			my->light = lightSphereShadow(my->x / 16, my->y / 16, 4, 192);
		}
	}

	// move arms
	Entity* rightarm = NULL;
	for (bodypart = 0, node = my->children.first; node != NULL; node = node->next, bodypart++)
	{
		if ( bodypart < 2 )
		{
			continue;
		}
		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		if ( bodypart != 4 )
		{
			entity->yaw = my->yaw;
		}
		if ( bodypart == 2 )
		{
			rightarm = entity;
			if ( !MONSTER_ATTACK )
			{
				entity->pitch = 0;
			}
			else
			{
				if ( !MONSTER_ATTACKTIME )
				{
					entity->pitch = -3 * PI / 4;
					MONSTER_WEAPONYAW = PI / 3;
					MONSTER_ATTACKTIME = 1;
				}
				else
				{
					entity->pitch += .15;
					MONSTER_WEAPONYAW -= .15;
					if ( entity->pitch > -PI / 4 )
					{
						entity->pitch = 0;
						MONSTER_WEAPONYAW = 0;
						MONSTER_ATTACKTIME = 0;
						MONSTER_ATTACK = 0;
					}
				}
			}
		}
		else if ( bodypart == 3 )
		{
			entity->pitch = rightarm->pitch;
		}
		switch ( bodypart )
		{
			// right arm
			case 2:
				entity->x += 2.75 * cos(my->yaw + PI / 2);
				entity->y += 2.75 * sin(my->yaw + PI / 2);
				entity->z -= 3.25;
				entity->yaw += MONSTER_WEAPONYAW;
				break;
			// left arm
			case 3:
				entity->x -= 2.75 * cos(my->yaw + PI / 2);
				entity->y -= 2.75 * sin(my->yaw + PI / 2);
				entity->z -= 3.25;
				entity->yaw -= MONSTER_WEAPONYAW;
				break;
			// head
			case 4:
			{
				entity->z -= 4.25;
				node_t* tempNode;
				Entity* playertotrack = NULL;
				for ( tempNode = map.entities->first; tempNode != NULL; tempNode = tempNode->next )
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
					entity->yaw = my->yaw;
				}
				break;
			}
			default:
				break;
		}
	}
}
