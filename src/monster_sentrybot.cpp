/*-------------------------------------------------------------------------------

	BARONY
	File: monster_sentrybot.cpp
	Desc: implements all of the kobold monster's code

	Copyright 2013-2019 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "monster.hpp"
#include "sound.hpp"
#include "book.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "magic/magic.hpp"

void initSentryBot(Entity* my, Stat* myStats)
{
	node_t* node;

	my->initMonster(872);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 0;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = 0;
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

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

			// count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			my->setHardcoreStats(*myStats);
		}
	}

	// torso
	Entity* entity = newEntity(873, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SENTRYBOT][1][0]; // 0
	entity->focaly = limbs[SENTRYBOT][1][1]; // 0
	entity->focalz = limbs[SENTRYBOT][1][2]; // 0
	entity->behavior = &actSentryBotLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right leg
	entity = newEntity(874, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SENTRYBOT][2][0]; // .25
	entity->focaly = limbs[SENTRYBOT][2][1]; // 0
	entity->focalz = limbs[SENTRYBOT][2][2]; // 1.5
	entity->behavior = &actSentryBotLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left leg
	entity = newEntity(875, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SENTRYBOT][3][0]; // .25
	entity->focaly = limbs[SENTRYBOT][3][1]; // 0
	entity->focalz = limbs[SENTRYBOT][3][2]; // 1.5
	entity->behavior = &actSentryBotLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// right arm
	entity = newEntity(876, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SENTRYBOT][4][0]; // 0
	entity->focaly = limbs[SENTRYBOT][4][1]; // 0
	entity->focalz = limbs[SENTRYBOT][4][2]; // 2
	entity->behavior = &actSentryBotLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// left arm
	entity = newEntity(878, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SENTRYBOT][5][0]; // 0
	entity->focaly = limbs[SENTRYBOT][5][1]; // 0
	entity->focalz = limbs[SENTRYBOT][5][2]; // 2
	entity->behavior = &actSentryBotLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// world weapon
	entity = newEntity(-1, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[INVISIBLE] = true;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SENTRYBOT][6][0]; // 2
	entity->focaly = limbs[SENTRYBOT][6][1]; // 0
	entity->focalz = limbs[SENTRYBOT][6][2]; // -.5
	entity->behavior = &actSentryBotLimb;
	entity->parent = my->getUID();
	entity->pitch = .25;
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	if ( multiplayer == CLIENT || MONSTER_INIT )
	{
		return;
	}
}

void actSentryBotLimb(Entity* my)
{
	my->actMonsterLimb(false);
}

void sentryBotDie(Entity* my)
{
	/*int c;
	for ( c = 0; c < 6; ++c )
	{
		Entity* entity = spawnGib(my);
		if ( entity )
		{
			serverSpawnGibForClient(entity);
		}
	}*/

	my->removeMonsterDeathNodes();

	// playSoundEntity(my, 298 + rand() % 4, 128);
	list_RemoveNode(my->mynode);
	return;
}

#define SENTRYBOTWALKSPEED .13

void sentryBotAnimate(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = nullptr, *entity2 = nullptr;
	Entity* rightbody = nullptr;
	Entity* weaponarm = nullptr;
	int bodypart;

	if ( multiplayer != CLIENT )
	{
		// sleeping
		if ( myStats->EFFECTS[EFF_ASLEEP] )
		{
			//my->z = 4;
			my->pitch = PI / 4;
		}
		else
		{
			//my->z = 2.25;
			my->pitch = 0;
		}
	}

	//Move bodyparts
	for (bodypart = 0, node = my->children.first; node != nullptr; node = node->next, ++bodypart)
	{
		if ( bodypart < LIMB_HUMANOID_TORSO )
		{
			continue;
		}
		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		entity->yaw = my->yaw;

		//if ( bodypart == LIMB_HUMANOID_RIGHTLEG || bodypart == LIMB_HUMANOID_LEFTARM )
		//{
		//	my->humanoidAnimateWalk(entity, node, bodypart, SENTRYBOTWALKSPEED, dist, 0.4);
		//}
		//else if ( bodypart == LIMB_HUMANOID_LEFTLEG || bodypart == LIMB_HUMANOID_RIGHTARM || bodypart == LIMB_HUMANOID_CLOAK )
		//{
		//	// left leg, right arm, cloak.
		//	if ( bodypart == LIMB_HUMANOID_RIGHTARM )
		//	{
		//		weaponarm = entity;
		//		if ( my->monsterAttack > 0 )
		//		{
		//			my->handleWeaponArmAttack(entity);
		//		}
		//	}
		//	else if ( bodypart == LIMB_HUMANOID_CLOAK )
		//	{
		//		entity->pitch = entity->fskill[0];
		//	}

		//	my->humanoidAnimateWalk(entity, node, bodypart, SENTRYBOTWALKSPEED, dist, 0.4);
		//	
		//	if ( bodypart == LIMB_HUMANOID_CLOAK )
		//	{
		//		entity->fskill[0] = entity->pitch;
		//		entity->roll = my->roll - fabs(entity->pitch) / 2;
		//		entity->pitch = 0;
		//	}
		//}

		switch ( bodypart )
		{
			// torso
			case LIMB_HUMANOID_TORSO:
				entity->x -= .25 * cos(my->yaw);
				entity->y -= .25 * sin(my->yaw);
				entity->z += 1.25;
				break;
			// right leg
			case LIMB_HUMANOID_RIGHTLEG:
				if ( multiplayer != CLIENT )
				{
					entity->sprite = 423;
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				entity->x += 1.25 * cos(my->yaw + PI / 2);
				entity->y += 1.25 * sin(my->yaw + PI / 2);
				entity->z += 2.75;
				break;
			// left leg
			case LIMB_HUMANOID_LEFTLEG:
				if ( multiplayer != CLIENT )
				{
					entity->sprite = 424;
					if ( multiplayer == SERVER )
					{
						// update sprites for clients
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							serverUpdateEntityBodypart(my, bodypart);
						}
						if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
				entity->x -= 1.25 * cos(my->yaw + PI / 2);
				entity->y -= 1.25 * sin(my->yaw + PI / 2);
				entity->z += 2.75;
				break;
			// right arm
			case LIMB_HUMANOID_RIGHTARM:
			{
				// if weapon invisible and I'm not moving, relax arm.
				entity->focalx = limbs[KOBOLD][4][0]; // 0
				entity->focaly = limbs[KOBOLD][4][1]; // 0
				entity->focalz = limbs[KOBOLD][4][2]; // 2
				entity->sprite = 425;

				entity->x += 2.5 * cos(my->yaw + PI / 2) - .75 * cos(my->yaw);
				entity->y += 2.5 * sin(my->yaw + PI / 2) - .75 * sin(my->yaw);
				entity->z -= .25;
				entity->yaw += MONSTER_WEAPONYAW;
				break;
			}
			// left arm
			case LIMB_HUMANOID_LEFTARM:
			{
				// if shield invisible and I'm not moving, relax arm.
				entity->focalx = limbs[KOBOLD][5][0]; // 0
				entity->focaly = limbs[KOBOLD][5][1]; // 0
				entity->focalz = limbs[KOBOLD][5][2]; // 2
				entity->sprite = 427;

				entity->x -= 2.5 * cos(my->yaw + PI / 2) + .75 * cos(my->yaw);
				entity->y -= 2.5 * sin(my->yaw + PI / 2) + .75 * sin(my->yaw);
				entity->z -= .25;
				break;
			}
			// weapon
			case LIMB_HUMANOID_WEAPON:
				if ( multiplayer != CLIENT )
				{
					if ( myStats->weapon == nullptr || myStats->EFFECTS[EFF_INVISIBLE] ) //TODO: isInvisible()?
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
				if ( weaponarm != nullptr )
				{
					my->handleHumanoidWeaponLimb(entity, weaponarm);
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
