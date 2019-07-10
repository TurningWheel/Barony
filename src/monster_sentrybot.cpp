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

			if ( myStats->weapon == nullptr /*&& myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1*/ )
			{
				myStats->weapon = newItem(CROSSBOW, EXCELLENT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
			}
		}
	}

	// tripod
	Entity* entity = newEntity(873, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SENTRYBOT][1][0];
	entity->focaly = limbs[SENTRYBOT][1][1];
	entity->focalz = limbs[SENTRYBOT][1][2];
	entity->behavior = &actSentryBotLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// gear 1 left head
	entity = newEntity(874, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SENTRYBOT][2][0];
	entity->focaly = limbs[SENTRYBOT][2][1];
	entity->focalz = limbs[SENTRYBOT][2][2];
	entity->behavior = &actSentryBotLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// gear 1 right head
	entity = newEntity(874, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SENTRYBOT][2][0];
	entity->focaly = -limbs[SENTRYBOT][2][1];
	entity->focalz = limbs[SENTRYBOT][2][2];
	entity->behavior = &actSentryBotLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// gear 1 left body
	entity = newEntity(874, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SENTRYBOT][3][0];
	entity->focaly = limbs[SENTRYBOT][3][1];
	entity->focalz = limbs[SENTRYBOT][3][2];
	entity->behavior = &actSentryBotLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// gear 1 right body
	entity = newEntity(874, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SENTRYBOT][3][0];
	entity->focaly = -limbs[SENTRYBOT][3][1];
	entity->focalz = limbs[SENTRYBOT][3][2];
	entity->behavior = &actSentryBotLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// gear 2 middle
	entity = newEntity(875, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SENTRYBOT][4][0];
	entity->focaly = limbs[SENTRYBOT][4][1];
	entity->focalz = limbs[SENTRYBOT][4][2];
	entity->scalex = 0.99;
	entity->scaley = 0.99;
	entity->scalez = 0.99;
	entity->behavior = &actSentryBotLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// loader
	entity = newEntity(876, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SENTRYBOT][5][0];
	entity->focaly = limbs[SENTRYBOT][5][1];
	entity->focalz = limbs[SENTRYBOT][5][2];
	entity->behavior = &actSentryBotLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// world weapon
	entity = newEntity(167, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[INVISIBLE] = true;
	entity->yaw = my->yaw;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[SENTRYBOT][6][0];
	entity->focaly = limbs[SENTRYBOT][6][1];
	entity->focalz = limbs[SENTRYBOT][6][2];
	entity->scalex = 0.99;
	entity->scaley = 0.99;
	entity->scalez = 0.99;
	entity->behavior = &actSentryBotLimb;
	entity->parent = my->getUID();
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
#define BODY_TRIPOD 2
#define GEAR_HEAD_LEFT 3
#define GEAR_HEAD_RIGHT 4
#define GEAR_BODY_LEFT 5
#define GEAR_BODY_RIGHT 6
#define GEAR_MIDDLE 7
#define WEAPON_LOADER 8
#define WEAPON_LIMB 9

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

	my->focalx = limbs[SENTRYBOT][0][0];
	my->focaly = limbs[SENTRYBOT][0][1];
	my->focalz = limbs[SENTRYBOT][0][2];
	my->z = limbs[SENTRYBOT][11][2];

	Entity* tripod = nullptr;
	Entity* gearBodyLeft = nullptr;
	Entity* gearHeadLeft = nullptr;
	Entity* weaponLoader = nullptr;
	//Move bodyparts
	for (bodypart = 0, node = my->children.first; node != nullptr; node = node->next, ++bodypart)
	{
		if ( bodypart < BODY_TRIPOD )
		{
			continue;
		}
		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;

		if ( bodypart == WEAPON_LOADER || bodypart == GEAR_HEAD_LEFT 
			|| bodypart == GEAR_HEAD_RIGHT || bodypart == WEAPON_LIMB )
		{
			entity->yaw = my->yaw; // face the monster's direction
		}

		if ( bodypart == GEAR_MIDDLE )
		{
			entity->pitch += 0.1;
			if ( entity->pitch > 2 * PI )
			{
				entity->pitch -= 2 * PI;
			}
		}
		else if ( bodypart == GEAR_HEAD_LEFT )
		{
			gearHeadLeft = entity;
			if ( entity->pitch < 0 )
			{
				entity->pitch += 2 * PI;
			}
			else if ( entity->pitch > 2 * PI )
			{
				entity->pitch -= 2 * PI;
			}

			if ( my->monsterAttack > 0 )
			{
				if ( my->monsterAttack == MONSTER_POSE_RANGED_WINDUP1 )
				{
					if ( my->monsterAttackTime == 0 )
					{
						entity->fskill[0] = -0.2;
					}

					entity->pitch += entity->fskill[0];

					if ( my->monsterAttackTime >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
					{
						if ( multiplayer != CLIENT )
						{
							my->attack(MONSTER_POSE_RANGED_SHOOT1, 0, nullptr);
						}
					}
				}
				else if ( my->monsterAttack == MONSTER_POSE_RANGED_SHOOT1 )
				{
					if ( entity->fskill[0] < 0.01 )
					{
						entity->fskill[0] = 1;
					}
					else
					{
						entity->pitch += entity->fskill[0];
						entity->fskill[0] = std::max(entity->fskill[0] * 0.95, 0.01);
					}

					if ( my->monsterAttackTime >= 20 )
					{
						my->monsterAttack = 0;
					}
				}
			}
			else
			{
				if ( abs(entity->fskill[0]) > 0.01 )
				{
					entity->skill[0] = 1;
					entity->pitch += entity->fskill[0];
					entity->fskill[0] = std::max(entity->fskill[0] * 0.95, 0.01);
				}
				else if ( entity->skill[0] == 1 )
				{
					// fall to rest on a 90 degree angle.
					if ( entity->pitch < PI / 2 )
					{
						if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.01, PI / 2, false, 0.f) )
						{
							entity->skill[0] = 0;
						}
					}
					else if ( entity->pitch < PI )
					{
						if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.01, PI, false, 0.f) )
						{
							entity->skill[0] = 0;
						}
					}
					else if ( entity->pitch < (3 * PI / 2) )
					{
						if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.01, 3 * PI / 2, false, 0.f) )
						{
							entity->skill[0] = 0;
						}
					}
					else
					{
						if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.01, 0.f, false, 0.f) )
						{
							entity->skill[0] = 0;
						}
					}
				}
			}
			//entity->pitch += 0.1;
			//if ( entity->pitch > 2 * PI )
			//{
			//	entity->pitch -= 2 * PI;
			//}
		}
		else if ( bodypart == GEAR_BODY_LEFT || bodypart == GEAR_BODY_RIGHT )
		{
			// normalize rotations
			if ( my->yaw > 2 * PI )
			{
				my->yaw -= 2 * PI;
			}
			else if ( my->yaw < 0 )
			{
				my->yaw += 2 * PI;
			}
			if ( entity->pitch > 4 * PI )
			{
				entity->pitch -= 4 * PI;
			}
			else if ( entity->pitch < 0 )
			{
				entity->pitch += 4 * PI;
			}

			// spin the gear as the head turns.
			if ( bodypart == GEAR_BODY_RIGHT )
			{
				if ( gearBodyLeft )
				{
					entity->pitch = -gearBodyLeft->pitch;
				}
			}
			else if ( !limbAngleWithinRange(entity->pitch, entity->fskill[0], my->yaw * 2) && abs(entity->fskill[0]) < 0.01 )
			{
				if ( entity->pitch <= my->yaw * 2 )
				{
					if ( (my->yaw * 2 - entity->pitch) > 2 * PI ) // quicker to go the opposite way.
					{
						entity->fskill[0] = -0.4;
					}
					else
					{
						entity->fskill[0] = 0.4;
					}
				}
				else if ( entity->pitch > my->yaw * 2 )
				{
					if ( (entity->pitch - my->yaw * 2) > 2 * PI ) // quicker to go the opposite way.
					{
						entity->fskill[0] = 0.4;
					}
					else
					{
						entity->fskill[0] = -0.4;
					}
				}
			}
			else
			{
				if ( limbAngleWithinRange(entity->pitch, entity->fskill[0], my->yaw * 2) )
				{
					entity->pitch = my->yaw * 2;
				}
				else
				{
					entity->pitch += entity->fskill[0];
				}
				entity->fskill[0] *= 0.95;
			}
		}
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
			case BODY_TRIPOD:
				tripod = entity;
				entity->focalx = limbs[SENTRYBOT][1][0];
				entity->focaly = limbs[SENTRYBOT][1][1];
				entity->focalz = limbs[SENTRYBOT][1][2];
				entity->x += limbs[SENTRYBOT][7][0];
				entity->y += limbs[SENTRYBOT][7][1];
				entity->z += limbs[SENTRYBOT][7][2];
				break;
			case GEAR_HEAD_LEFT:
				entity->focalx = limbs[SENTRYBOT][2][0];
				entity->focaly = limbs[SENTRYBOT][2][1];
				entity->focalz = limbs[SENTRYBOT][2][2];
				if ( tripod )
				{
					entity->x += limbs[SENTRYBOT][8][0] * cos(tripod->yaw + PI / 2) + limbs[SENTRYBOT][8][1] * cos(tripod->yaw);
					entity->y += limbs[SENTRYBOT][8][0] * sin(tripod->yaw + PI / 2) + limbs[SENTRYBOT][8][1] * sin(tripod->yaw);
					entity->z += limbs[SENTRYBOT][8][2];
				}
				break;
			case GEAR_HEAD_RIGHT:
				if ( gearHeadLeft )
				{
					entity->pitch = gearHeadLeft->pitch;
				}
				entity->focalx = limbs[SENTRYBOT][2][0];
				entity->focaly = -limbs[SENTRYBOT][2][1];
				entity->focalz = limbs[SENTRYBOT][2][2];
				if ( tripod )
				{
					entity->x -= limbs[SENTRYBOT][8][0] * cos(tripod->yaw + PI / 2) + limbs[SENTRYBOT][8][1] * cos(tripod->yaw);
					entity->y -= limbs[SENTRYBOT][8][0] * sin(tripod->yaw + PI / 2) + limbs[SENTRYBOT][8][1] * sin(tripod->yaw);
					entity->z += limbs[SENTRYBOT][8][2];
				}
				break;
			case GEAR_BODY_LEFT:
				gearBodyLeft = entity;
				entity->focalx = limbs[SENTRYBOT][3][0];
				entity->focaly = limbs[SENTRYBOT][3][1];
				entity->focalz = limbs[SENTRYBOT][3][2];
				if ( tripod )
				{
					entity->x += limbs[SENTRYBOT][12][0] * cos(tripod->yaw + PI / 2) + limbs[SENTRYBOT][12][1] * cos(tripod->yaw);
					entity->y += limbs[SENTRYBOT][12][0] * sin(tripod->yaw + PI / 2) + limbs[SENTRYBOT][12][1] * sin(tripod->yaw);
					entity->z += limbs[SENTRYBOT][12][2];
				}
				break;
			case GEAR_BODY_RIGHT:
				entity->focalx = limbs[SENTRYBOT][3][0];
				entity->focaly = -limbs[SENTRYBOT][3][1];
				entity->focalz = limbs[SENTRYBOT][3][2];
				if ( tripod )
				{
					entity->x -= limbs[SENTRYBOT][12][0] * cos(tripod->yaw + PI / 2) + limbs[SENTRYBOT][12][1] * cos(tripod->yaw);
					entity->y -= limbs[SENTRYBOT][12][0] * sin(tripod->yaw + PI / 2) + limbs[SENTRYBOT][12][1] * sin(tripod->yaw);
					entity->z += limbs[SENTRYBOT][12][2];
				}
				break;
			case GEAR_MIDDLE:
				entity->focalx = limbs[SENTRYBOT][4][0];
				entity->focaly = limbs[SENTRYBOT][4][1];
				entity->focalz = limbs[SENTRYBOT][4][2];
				entity->yaw = tripod->yaw + PI / 2;
				if ( tripod )
				{
					entity->x += limbs[SENTRYBOT][9][0] * cos(tripod->yaw + PI / 2) + limbs[SENTRYBOT][9][1] * cos(tripod->yaw);
					entity->y += limbs[SENTRYBOT][9][0] * sin(tripod->yaw + PI / 2) + limbs[SENTRYBOT][9][1] * sin(tripod->yaw);
					entity->z += limbs[SENTRYBOT][9][2];
				}
				//if ( multiplayer != CLIENT )
				//{
				//	entity->sprite = 875;
				//	if ( multiplayer == SERVER )
				//	{
				//		// update sprites for clients
				//		if ( entity->skill[10] != entity->sprite )
				//		{
				//			entity->skill[10] = entity->sprite;
				//			serverUpdateEntityBodypart(my, bodypart);
				//		}
				//		if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
				//		{
				//			serverUpdateEntityBodypart(my, bodypart);
				//		}
				//	}
				//}
				break;
			case WEAPON_LOADER:
				weaponLoader = entity;
				entity->focalx = limbs[SENTRYBOT][5][0];
				entity->focaly = limbs[SENTRYBOT][5][1];
				entity->focalz = limbs[SENTRYBOT][5][2];
				entity->x += limbs[SENTRYBOT][10][0];
				entity->y += limbs[SENTRYBOT][10][1];
				entity->z += limbs[SENTRYBOT][10][2];
				if ( my->monsterAttack == MONSTER_POSE_RANGED_SHOOT1 )
				{
					entity->fskill[0] = std::min(3.5, 2 + entity->fskill[0]);
					entity->focalx += entity->fskill[0];
				}
				else
				{
					entity->fskill[0] = std::max(0.0, entity->fskill[0] - 0.1);
					entity->focalx += entity->fskill[0];
				}
				break;
			case WEAPON_LIMB:
				entity->focalx = limbs[SENTRYBOT][6][0];
				entity->focaly = limbs[SENTRYBOT][6][1];
				entity->focalz = limbs[SENTRYBOT][6][2];
				if ( my->monsterAttack == MONSTER_POSE_RANGED_SHOOT1 )
				{
					entity->flags[INVISIBLE] = true;
				}
				else
				{
					if ( weaponLoader )
					{
						entity->fskill[0] = weaponLoader->fskill[0];
						entity->focalx += entity->fskill[0];
					}
					entity->flags[INVISIBLE] = false;
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
