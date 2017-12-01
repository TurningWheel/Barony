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

void initLichFire(Entity* my, Stat* myStats)
{
	my->initMonster(646);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 120;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 1;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		if ( myStats != nullptr )
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
				//myStats->weapon = newItem(SPELLBOOK_LIGHTNING, EXCELLENT, 0, 1, 0, false, NULL);
			}
		}
	}

	// right arm
	Entity* entity = newEntity(649, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[LICH_FIRE][1][0]; // 0
	entity->focaly = limbs[LICH_FIRE][1][1]; // 0
	entity->focalz = limbs[LICH_FIRE][1][2]; // 2
	entity->behavior = &actLichFireLimb;
	entity->parent = my->getUID();
	node_t* node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// left arm
	entity = newEntity(648, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[LICH_FIRE][2][0]; // 0
	entity->focaly = limbs[LICH_FIRE][2][1]; // 0
	entity->focalz = limbs[LICH_FIRE][2][2]; // 2
	entity->behavior = &actLichFireLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// head
	entity = newEntity(647, 0, map.entities);
	entity->yaw = my->yaw;
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[LICH_FIRE][3][0]; // 0
	entity->focaly = limbs[LICH_FIRE][3][1]; // 0
	entity->focalz = limbs[LICH_FIRE][3][2]; // -2
	entity->behavior = &actLichFireLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);

	// world weapon
	entity = newEntity(-1, 0, map.entities);
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[LICH_FIRE][4][0]; // 1.5
	entity->focaly = limbs[LICH_FIRE][4][1]; // 0
	entity->focalz = limbs[LICH_FIRE][4][2]; // -.5
	entity->behavior = &actLichFireLimb;
	entity->parent = my->getUID();
	entity->pitch = .25;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
}

void lichFireDie(Entity* my)
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
	my->removeLightField();
	// kill all other monsters on the level
	/*for ( node = map.entities->first; node != NULL; node = nextnode )
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
	spawnExplosion(my->x, my->y, my->z);*/
	list_RemoveNode(my->mynode);
	return;
}

void actLichFireLimb(Entity* my)
{
	my->actMonsterLimb();
}

void lichFireAnimate(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = nullptr, *entity2 = nullptr;
	Entity* rightbody = nullptr;
	Entity* weaponarm = nullptr;
	int bodypart;
	bool wearingring = false;

	// remove old light field
	my->removeLightField();

	// set invisibility //TODO: isInvisible()?
	if ( multiplayer != CLIENT )
	{
		if ( myStats->ring != nullptr )
			if ( myStats->ring->type == RING_INVISIBILITY )
			{
				wearingring = true;
			}
		if ( myStats->cloak != nullptr )
			if ( myStats->cloak->type == CLOAK_INVISIBILITY )
			{
				wearingring = true;
			}
		if ( myStats->EFFECTS[EFF_INVISIBLE] == true || wearingring == true )
		{
			my->flags[INVISIBLE] = true;
			my->flags[BLOCKSIGHT] = false;
			bodypart = 0;
			for ( node = my->children.first; node != nullptr; node = node->next )
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
			for ( node = my->children.first; node != nullptr; node = node->next )
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
		/*if ( myStats->HP > myStats->MAXHP / 2 )
		{
			my->light = lightSphereShadow(my->x / 16, my->y / 16, 4, 192);
		}
		else if ( !my->skill[27] )
		{
			my->skill[27] = 1;
			serverUpdateEntitySkill(my, 27);
		}*/
	}
	else
	{
		/*if ( !my->skill[27] )
		{
			my->light = lightSphereShadow(my->x / 16, my->y / 16, 4, 192);
		}*/
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
			weaponarm = entity;
			if ( !MONSTER_ATTACK )
			{
				entity->pitch = 0;
			}
			else
			{
				// vertical chop windup
				if ( my->monsterAttack == MONSTER_POSE_MELEE_WINDUP1 )
				{
					if ( my->monsterAttackTime == 0 )
					{
						// init rotations
						weaponarm->pitch = 0;
						//my->monsterArmbended = 0;
						my->monsterWeaponYaw = 0;
						weaponarm->roll = 0;
						weaponarm->skill[1] = 0;
					}

					limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.25, 5 * PI / 4, false, 0.0);

					if ( my->monsterAttackTime >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
					{
						if ( multiplayer != CLIENT )
						{
							my->attack(1, 0, nullptr);
						}
					}
				}
				// vertical chop attack
				else if ( my->monsterAttack == 1 )
				{
					if ( weaponarm->pitch >= 3 * PI / 2 )
					{
						//my->monsterArmbended = 1;
					}

					if ( weaponarm->skill[1] == 0 )
					{
						// chop forwards
						if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.4, PI / 2, false, 0.0) )
						{
							weaponarm->skill[1] = 1;
						}
					}
					else if ( weaponarm->skill[1] == 1 )
					{
						if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.25, 0, false, 0.0) )
						{
							my->monsterWeaponYaw = 0;
							weaponarm->pitch = 0;
							weaponarm->roll = 0;
							//my->monsterArmbended = 0;
							my->monsterAttack = 0;
							//returnWeaponarmToNeutral(weaponarm, rightbody);
						}
					}
				}
				// horizontal chop windup
				else if ( my->monsterAttack == MONSTER_POSE_MELEE_WINDUP2 )
				{
					if ( my->monsterAttackTime == 0 )
					{
						// init rotations
						weaponarm->pitch = PI / 4;
						weaponarm->roll = 0;
						my->monsterArmbended = 1;
						weaponarm->skill[1] = 0;
						my->monsterWeaponYaw = 6 * PI / 4;
					}

					limbAnimateToLimit(weaponarm, ANIMATE_ROLL, -0.2, 3 * PI / 2, false, 0.0);
					limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.2, 0, false, 0.0);


					if ( my->monsterAttackTime >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
					{
						if ( multiplayer != CLIENT )
						{
							my->attack(2, 0, nullptr);
						}
					}
				}
				// horizontal chop attack
				else if ( my->monsterAttack == 2 )
				{
					if ( weaponarm->skill[1] == 0 )
					{
						// swing
						// this->weaponyaw is OK to change for clients, as server doesn't update it for them.
						if ( limbAnimateToLimit(my, ANIMATE_WEAPON_YAW, 0.3, 2 * PI / 8, false, 0.0) )
						{
							weaponarm->skill[1] = 1;
						}
					}
					else if ( weaponarm->skill[1] == 1 )
					{
						// post-swing return to normal weapon yaw
						if ( limbAnimateToLimit(my, ANIMATE_WEAPON_YAW, -0.5, 0, false, 0.0) )
						{
							// restore pitch and roll after yaw is set
							if ( limbAnimateToLimit(weaponarm, ANIMATE_ROLL, 0.4, 0, false, 0.0)
								&& limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.4, 0, false, 0.0) )
							{
								weaponarm->skill[0] = 0;
								my->monsterWeaponYaw = 0;
								weaponarm->pitch = 0;
								weaponarm->roll = 0;
								my->monsterArmbended = 0;
								my->monsterAttack = 0;
							}
						}
					}
				}
				/*if ( !MONSTER_ATTACKTIME )
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
				}*/
			}
		}
		else if ( bodypart == 3 )
		{
			//entity->pitch = weaponarm->pitch;
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
			case 5:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->weapon == nullptr || myStats->EFFECTS[EFF_INVISIBLE] || wearingring ) //TODO: isInvisible()?
					{
						entity->flags[INVISIBLE] = true;
					}
					else
					{
						entity->sprite = itemModel(myStats->weapon);
						if ( itemCategory(myStats->weapon) == SPELLBOOK )
						{
							entity->flags[INVISIBLE] = true;
						}
						else
						{
							entity->flags[INVISIBLE] = false;
						}
					}
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->skill[11] != entity->flags[INVISIBLE] )
						{
							entity->skill[11] = entity->flags[INVISIBLE];
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				else
				{
					if ( entity->sprite <= 0 )
					{
						entity->flags[INVISIBLE] = true;
					}
				}
				if ( entity != nullptr )
				{
					//my->handleHumanoidWeaponLimb(entity, weaponarm);
					if ( weaponarm == nullptr )
					{
						return;
					}
					entity->x = weaponarm->x;// +1.5 * cos(weaponarm->yaw);// *(my->monsterAttack == 0);
					entity->y = weaponarm->y;// +1.5 * sin(weaponarm->yaw);// * (my->monsterAttack == 0);
					entity->z = weaponarm->z;// -.5 * (my->monsterAttack == 0);
					entity->pitch = weaponarm->pitch;// -.25 * (my->monsterAttack == 0);
					entity->yaw = weaponarm->yaw;
					entity->roll = weaponarm->roll;
					if ( my->monsterAttack == 2 || my->monsterAttack == MONSTER_POSE_MELEE_WINDUP2 )
					{
					}
					else
					{
						entity->pitch += PI / 2;
					}

					entity->focalx = limbs[LICH_FIRE][4][0];
					entity->focaly = limbs[LICH_FIRE][4][1];
					entity->focalz = limbs[LICH_FIRE][4][2];
					if ( my->monsterArmbended )
					{
						entity->focalx = limbs[LICH_FIRE][4][0] - 0.5;
						//entity->focalz = limbs[LICH_FIRE][4][2] - 2;
						entity->pitch += cos(weaponarm->roll) * PI / 2;
						entity->yaw -= sin(weaponarm->roll) * PI / 2;
					}
				}
				break;
			default:
				break;
		}
	}
	if ( MONSTER_ATTACK > 0 && MONSTER_ATTACK <= MONSTER_POSE_MAGIC_CAST3 )
	{
		MONSTER_ATTACKTIME++;
	}
	else if ( MONSTER_ATTACK == 0 )
	{
		MONSTER_ATTACKTIME = 0;
	}
	else
	{
		// do nothing, don't reset attacktime or increment it.
	}
}
