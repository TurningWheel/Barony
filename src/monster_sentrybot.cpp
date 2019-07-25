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

	my->initMonster(my->sprite);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = -1;
		MONSTER_SPOTVAR = 1;
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

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

			// count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			//my->setHardcoreStats(*myStats);

			if ( myStats->weapon == nullptr && my->sprite == 872/*&& myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1*/ )
			{
				myStats->weapon = newItem(CROSSBOW, EXCELLENT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
			}
			else if ( myStats->weapon == nullptr && my->sprite == 875/*&& myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1*/ )
			{
				myStats->weapon = newItem(SPELLBOOK_SLOW, EXCELLENT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
			}
		}
	}

	int race = my->getMonsterTypeFromSprite();

	// tripod
	Entity* entity = newEntity(873, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[race][1][0];
	entity->focaly = limbs[race][1][1];
	entity->focalz = limbs[race][1][2];
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
	entity->flags[INVISIBLE] = my->flags[INVISIBLE];
	entity->yaw = my->yaw;
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[race][2][0];
	entity->focaly = limbs[race][2][1];
	entity->focalz = limbs[race][2][2];
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
	entity->flags[INVISIBLE] = my->flags[INVISIBLE];
	entity->yaw = my->yaw;
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[race][2][0];
	entity->focaly = -limbs[race][2][1];
	entity->focalz = limbs[race][2][2];
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
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[race][3][0];
	entity->focaly = limbs[race][3][1];
	entity->focalz = limbs[race][3][2];
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
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[race][3][0];
	entity->focaly = -limbs[race][3][1];
	entity->focalz = limbs[race][3][2];
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
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[race][4][0];
	entity->focaly = limbs[race][4][1];
	entity->focalz = limbs[race][4][2];
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
	entity->flags[INVISIBLE] = my->flags[INVISIBLE];
	entity->yaw = my->yaw;
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[race][5][0];
	entity->focaly = limbs[race][5][1];
	entity->focalz = limbs[race][5][2];
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
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[race][6][0];
	entity->focaly = limbs[race][6][1];
	entity->focalz = limbs[race][6][2];
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

void initGyroBot(Entity* my, Stat* myStats)
{
	node_t* node;

	my->initMonster(886);

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = -1;
		MONSTER_SPOTVAR = 1;
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

			myStats->EFFECTS[EFF_LEVITATING] = true;
			myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 0;

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

			//my->setHardcoreStats(*myStats);
		}
	}

	// rotor large
	Entity* entity = newEntity(887, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[GYROBOT][1][0];
	entity->focaly = limbs[GYROBOT][1][1];
	entity->focalz = limbs[GYROBOT][1][2];
	entity->behavior = &actGyroBotLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// rotor small
	entity = newEntity(888, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[GYROBOT][2][0];
	entity->focaly = limbs[GYROBOT][2][1];
	entity->focalz = limbs[GYROBOT][2][2];
	entity->behavior = &actGyroBotLimb;
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
	bool gibs = true;
	if ( my->monsterSpecialState == DUMMYBOT_RETURN_FORM )
	{
		// don't make noises etc.
		Stat* myStats = my->getStats();
		if ( myStats && !strncmp(myStats->obituary, language[3631], strlen(language[3631])) )
		{
			// returning to land, don't explode into gibs.
			gibs = false;
		}
	}

	my->removeMonsterDeathNodes();
	if ( gibs )
	{
		int c;
		for ( c = 0; c < 6; c++ )
		{
			Entity* entity = spawnGib(my);
			if ( entity )
			{
				switch ( c )
				{
					case 0:
						entity->sprite = 873;
						break;
					case 1:
						entity->sprite = 874;
						break;
					case 2:
						entity->sprite = 874;
						break;
					case 3:
						entity->sprite = 874;
						break;
					case 4:
						entity->sprite = 874;
						break;
					case 5:
						entity->sprite = 875;
						break;
					default:
						break;
				}
				serverSpawnGibForClient(entity);
			}
		}
	}

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
		if ( my->monsterSpecialState == DUMMYBOT_RETURN_FORM )
		{
			if ( limbAnimateToLimit(my, ANIMATE_PITCH, 0.01, PI / 8, false, 0.0) )
			{
				int appearance = myStats->HP;
				ItemType type = TOOL_SENTRYBOT;
				if ( myStats->type == SPELLBOT )
				{
					type = TOOL_SPELLBOT;
				}
				Item* item = newItem(type, static_cast<Status>(myStats->monsterTinkeringStatus), 0, 1, appearance, true, &myStats->inventory);
				myStats->HP = 0;
				my->setObituary(language[3631]);
				return;
			}
		}
		else
		{
			//my->z = 2.25;
			my->pitch = 0;
		}
	}

	int race = my->getMonsterTypeFromSprite();

	my->focalx = limbs[race][0][0];
	my->focaly = limbs[race][0][1];
	my->focalz = limbs[race][0][2];
	if ( multiplayer != CLIENT )
	{
		my->z = limbs[race][11][2];
	}

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
		if ( bodypart == WEAPON_LOADER || bodypart == WEAPON_LIMB )
		{
			if ( my->monsterSpecialState == DUMMYBOT_RETURN_FORM )
			{
				entity->pitch = my->pitch;
			}
		}

		if ( bodypart == GEAR_MIDDLE && !my->flags[INVISIBLE] )
		{
			if ( my->monsterSpecialState == DUMMYBOT_RETURN_FORM )
			{
				entity->pitch += 0.02;
			}
			else
			{
				entity->pitch += 0.1;
			}
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
		else if ( (bodypart == GEAR_BODY_LEFT || bodypart == GEAR_BODY_RIGHT)
			&& !my->flags[INVISIBLE] )
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
			if ( bodypart == GEAR_BODY_LEFT && my->monsterSpecialState == DUMMYBOT_RETURN_FORM )
			{
				entity->pitch -= 0.1;
			}
			else if ( bodypart == GEAR_BODY_RIGHT )
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

		switch ( bodypart )
		{
			case BODY_TRIPOD:
				tripod = entity;
				entity->focalx = limbs[race][1][0];
				entity->focaly = limbs[race][1][1];
				entity->focalz = limbs[race][1][2];
				entity->x += limbs[race][7][0];
				entity->y += limbs[race][7][1];
				entity->z += limbs[race][7][2];
				break;
			case GEAR_HEAD_LEFT:
				entity->flags[INVISIBLE] = my->flags[INVISIBLE];
				entity->focalx = limbs[race][2][0];
				entity->focaly = limbs[race][2][1];
				entity->focalz = limbs[race][2][2];
				if ( tripod )
				{
					entity->x += limbs[race][8][0] * cos(tripod->yaw + PI / 2) + limbs[race][8][1] * cos(tripod->yaw);
					entity->y += limbs[race][8][0] * sin(tripod->yaw + PI / 2) + limbs[race][8][1] * sin(tripod->yaw);
					entity->z += limbs[race][8][2];
				}
				break;
			case GEAR_HEAD_RIGHT:
				entity->flags[INVISIBLE] = my->flags[INVISIBLE];
				if ( gearHeadLeft )
				{
					entity->pitch = gearHeadLeft->pitch;
				}
				entity->focalx = limbs[race][2][0];
				entity->focaly = -limbs[race][2][1];
				entity->focalz = limbs[race][2][2];
				if ( tripod )
				{
					entity->x -= limbs[race][8][0] * cos(tripod->yaw + PI / 2) + limbs[race][8][1] * cos(tripod->yaw);
					entity->y -= limbs[race][8][0] * sin(tripod->yaw + PI / 2) + limbs[race][8][1] * sin(tripod->yaw);
					entity->z += limbs[race][8][2];
				}
				break;
			case GEAR_BODY_LEFT:
				gearBodyLeft = entity;
				entity->focalx = limbs[race][3][0];
				entity->focaly = limbs[race][3][1];
				entity->focalz = limbs[race][3][2];
				if ( tripod )
				{
					entity->x += limbs[race][12][0] * cos(tripod->yaw + PI / 2) + limbs[race][12][1] * cos(tripod->yaw);
					entity->y += limbs[race][12][0] * sin(tripod->yaw + PI / 2) + limbs[race][12][1] * sin(tripod->yaw);
					entity->z += limbs[race][12][2];
				}
				break;
			case GEAR_BODY_RIGHT:
				entity->focalx = limbs[race][3][0];
				entity->focaly = -limbs[race][3][1];
				entity->focalz = limbs[race][3][2];
				if ( tripod )
				{
					entity->x -= limbs[race][12][0] * cos(tripod->yaw + PI / 2) + limbs[race][12][1] * cos(tripod->yaw);
					entity->y -= limbs[race][12][0] * sin(tripod->yaw + PI / 2) + limbs[race][12][1] * sin(tripod->yaw);
					entity->z += limbs[race][12][2];
				}
				break;
			case GEAR_MIDDLE:
				entity->focalx = limbs[race][4][0];
				entity->focaly = limbs[race][4][1];
				entity->focalz = limbs[race][4][2];
				entity->yaw = tripod->yaw + PI / 2;
				if ( tripod )
				{
					entity->x += limbs[race][9][0] * cos(tripod->yaw + PI / 2) + limbs[race][9][1] * cos(tripod->yaw);
					entity->y += limbs[race][9][0] * sin(tripod->yaw + PI / 2) + limbs[race][9][1] * sin(tripod->yaw);
					entity->z += limbs[race][9][2];
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
				entity->focalx = limbs[race][5][0];
				entity->focaly = limbs[race][5][1];
				entity->focalz = limbs[race][5][2];
				entity->x += limbs[race][10][0];
				entity->y += limbs[race][10][1];
				entity->z += limbs[race][10][2];
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
				if ( race == SPELLBOT )
				{
					entity->flags[INVISIBLE] = true;
				}
				else
				{
					entity->flags[INVISIBLE] = my->flags[INVISIBLE];
				}
				break;
			case WEAPON_LIMB:
				entity->focalx = limbs[race][6][0];
				entity->focaly = limbs[race][6][1];
				entity->focalz = limbs[race][6][2];
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
				if ( race == SPELLBOT )
				{
					entity->flags[INVISIBLE] = true;
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

#define GYRO_ROTOR_LARGE 2
#define GYRO_ROTOR_SMALL 3

void gyroBotAnimate(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = nullptr;
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
			//my->pitch = 0;
		}
		if ( my->monsterSpecialState == GYRO_RETURN_LANDING || my->monsterSpecialState == GYRO_INTERACT_LANDING )
		{
			my->flags[PASSABLE] = false;
		}
		else
		{
			my->flags[PASSABLE] = true;
		}
	}

	if ( my->ticks % TICKS_PER_SECOND == 0 && my->monsterAllyIndex == clientnum )
	{
		Entity* playerLeader = my->monsterAllyGetPlayerLeader();
		for ( node_t* searchNode = map.entities->first; searchNode != nullptr; searchNode = searchNode->next )
		{
			Entity* ent = (Entity*)searchNode->element;
			if ( !ent || ent == my )
			{
				continue;
			}
			ent->entityShowOnMap = 0;
			if ( playerLeader )
			{
				if ( my->monsterAllyPickupItems == ALLY_GYRO_DETECT_MONSTERS )
				{
					if ( ent->behavior == &actMonster && ent->monsterAllyIndex < 0 )
					{
						if ( entityDist(my, ent) < TOUCHRANGE * 3 )
						{
							ent->entityShowOnMap = 1;
						}
					}
				}
				else if ( my->monsterAllyPickupItems == ALLY_GYRO_DETECT_TRAPS )
				{
					if ( ent->behavior == &actBoulderTrap || ent->behavior == &actArrowTrap
						|| ent->behavior == &actMagicTrap || ent->behavior == &actMagicTrapCeiling
						|| ent->behavior == &actBoulderTrapEast || ent->behavior == &actBoulderTrapWest
						|| ent->behavior == &actBoulderTrapNorth || ent->behavior == &actBoulderTrapSouth
						|| ent->behavior == &actSummonTrap || ent->behavior == &actSpearTrap )
					{
						if ( entityDist(my, ent) < TOUCHRANGE * 3 )
						{
							ent->entityShowOnMap = 1;
						}
					}
				}
				else if ( my->monsterAllyPickupItems == ALLY_GYRO_DETECT_EXITS )
				{
					if ( ent->behavior == &actLadder || ent->behavior == &actPortal )
					{
						if ( entityDist(my, ent) < TOUCHRANGE * 3 )
						{
							ent->entityShowOnMap = 1;
						}
					}
				}
				else if ( my->monsterAllyPickupItems >= ALLY_GYRO_DETECT_ITEMS_BLESSED
					&& my->monsterAllyPickupItems < ALLY_GYRO_DETECT_END )
				{
					if ( ent->behavior == &actItem )
					{
						if ( entityDist(my, ent) < TOUCHRANGE * 3 )
						{
							ItemType type = static_cast<ItemType>(ent->skill[10]);
							Status status = static_cast<Status>(ent->skill[11]);
							int beatitude = ent->skill[12];
							if ( status > BROKEN && type >= WOODEN_SHIELD && type < NUMITEMS )
							{
								if ( my->monsterAllyPickupItems == ALLY_GYRO_DETECT_ITEMS_BLESSED
									&& beatitude > 0 )
								{
									ent->entityShowOnMap = 1;
								}
								else if ( my->monsterAllyPickupItems == ALLY_GYRO_DETECT_ITEMS_RARE
									&& (items[type].level >= (currentlevel + 5) || items[type].level == -1) )
								{
									ent->entityShowOnMap = 1;
								}
								else if ( my->monsterAllyPickupItems == ALLY_GYRO_DETECT_ITEMS_VALUABLE
									&& items[type].value >= 400 )
								{
									ent->entityShowOnMap = 1;
								}
							}
						}
					}
				}
			}
		}
	}

	my->removeLightField();
	if ( my->monsterAllyClass > ALLY_GYRO_LIGHT_NONE )
	{
		switch ( my->monsterAllyClass )
		{
			case ALLY_GYRO_LIGHT_FAINT:
				my->light = lightSphereShadow(my->x / 16, my->y / 16, 3, 128);
				break;
			case ALLY_GYRO_LIGHT_BRIGHT:
				my->light = lightSphereShadow(my->x / 16, my->y / 16, 8, 128);
				break;
			default:
				break;
		}
	}

	my->focalx = limbs[GYROBOT][0][0];
	my->focaly = limbs[GYROBOT][0][1];
	my->focalz = limbs[GYROBOT][0][2];
	if ( multiplayer != CLIENT )
	{
		//my->z = limbs[GYROBOT][3][2];
		if ( my->ticks % (TICKS_PER_SECOND * 10) == 0 
			&& my->monsterSpecialTimer == 0
			&& my->monsterSpecialState == 0 )
		{
			// doACoolFlip = true!
			my->attack(MONSTER_POSE_RANGED_WINDUP1, 0, nullptr);
			my->monsterSpecialTimer = TICKS_PER_SECOND * 8;
		}

		if ( my->monsterSpecialState == GYRO_RETURN_LANDING )
		{
			if ( limbAnimateToLimit(my, ANIMATE_Z, 0.05, 0, false, 0.0) )
			{
				int appearance = myStats->HP;
				Item* item = newItem(TOOL_GYROBOT, static_cast<Status>(myStats->monsterTinkeringStatus), 0, 1, appearance, true, &myStats->inventory);
				myStats->HP = 0;
				my->setObituary(language[3631]);
				return;
			}
		}
		else if ( my->monsterSpecialState == GYRO_INTERACT_LANDING )
		{
			if ( limbAnimateToLimit(my, ANIMATE_Z, 0.1, 0, false, 0.0) )
			{
				my->attack(MONSTER_POSE_RANGED_WINDUP1, 0, nullptr);
				my->monsterSpecialTimer = TICKS_PER_SECOND * 5;
				if ( my->monsterAllySetInteract() )
				{
					// do interact.
					my->monsterAllyInteractTarget = 0;
					my->monsterAllyState = ALLY_STATE_DEFAULT;
				}
				my->monsterSpecialState = 0;
				serverUpdateEntitySkill(my, 33);
				my->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_ENDPOINT;
			}
		}
		else
		{
			if ( my->z > -4.5 )
			{
				limbAnimateToLimit(my, ANIMATE_Z, -0.1, -5, false, 0.0);
			}
			else
			{
				if ( my->monsterSpecialState == GYRO_START_FLYING )
				{
					if ( limbAnimateToLimit(my, ANIMATE_Z, -0.1, -6, false, 0.0) )
					{
						my->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_SETPOINT;
						my->monsterSpecialState = 0;
						serverUpdateEntitySkill(my, 33);
					}
				}
				else
				{
					if ( my->monsterAnimationLimbOvershoot == ANIMATE_OVERSHOOT_NONE )
					{
						my->z = -6;
						my->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_SETPOINT;
					}
					if ( dist < 0.1 )
					{
						// not moving, float.
						limbAnimateWithOvershoot(my, ANIMATE_Z, 0.01, -4.5, 0.01, -6, ANIMATE_DIR_POSITIVE);
					}
				}
			}
		}
		if ( !myStats->EFFECTS[EFF_LEVITATING] )
		{
			myStats->EFFECTS[EFF_LEVITATING] = true;
			myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 0;
		}
	}

	//Move bodyparts
	for ( bodypart = 0, node = my->children.first; node != nullptr; node = node->next, ++bodypart )
	{
		if ( bodypart < GYRO_ROTOR_LARGE )
		{
			continue;
		}

		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;

		if ( bodypart == GYRO_ROTOR_SMALL )
		{
			entity->yaw = my->yaw; // face the monster's direction
			if ( my->monsterAttack == MONSTER_POSE_RANGED_WINDUP1 )
			{
				entity->skill[0] = 1;
				my->monsterAttack = 0;
				my->pitch = 0;
				entity->fskill[0] = 0.05;
			}
			if ( entity->skill[0] == 1 )
			{
				my->pitch = (entity->fskill[0] * 2);
				entity->fskill[0] -= 0.05;
				if ( entity->fskill[0] < -PI )
				{
					entity->skill[0] = 0;
					entity->fskill[0] = 0;
					my->pitch = 0;
				}
			}
			else
			{
				if ( multiplayer != CLIENT )
				{
					if ( dist > 0.1 )
					{
						my->pitch = PI / 16;
					}
					else
					{
						my->pitch = 0;
					}
				}
			}
		}

		if ( bodypart == GYRO_ROTOR_LARGE )
		{
			entity->pitch = my->pitch + PI / 2;
			entity->yaw = my->yaw;
			entity->roll += 0.1;

			if ( (my->z > -4 && my->monsterSpecialState == 0) || my->monsterSpecialState == GYRO_START_FLYING )
			{
				entity->roll += 1;
			}
			else if ( dist > 0.1 )
			{
				entity->roll += 0.5;
			}
			else
			{
				entity->roll += 0.2;
			}
			if ( entity->yaw > 2 * PI )
			{
				entity->yaw -= 2 * PI;
			}
		}
		else if ( bodypart == GYRO_ROTOR_SMALL )
		{
			entity->pitch += 0.4;
			if ( entity->pitch > 2 * PI )
			{
				entity->pitch -= 2 * PI;
			}
		}

		switch ( bodypart )
		{
			case GYRO_ROTOR_LARGE:
				entity->x += limbs[GYROBOT][4][0] * sin(my->pitch) * cos(my->yaw);
				entity->y += limbs[GYROBOT][4][1] * sin(my->pitch) * sin(my->yaw);
				entity->z += limbs[GYROBOT][4][2] * cos(my->pitch);
				entity->focalx = limbs[GYROBOT][1][0];
				entity->focaly = limbs[GYROBOT][1][1];
				entity->focalz = limbs[GYROBOT][1][2];
				//entity->x += limbs[GYROBOT][4][0] * cos(my->yaw + PI / 2) + limbs[GYROBOT][4][1] * cos(my->yaw);
				//entity->y += limbs[GYROBOT][4][0] * sin(my->yaw + PI / 2) + limbs[GYROBOT][4][1] * sin(my->yaw);
				//entity->z += limbs[GYROBOT][4][2];
				break;
			case GYRO_ROTOR_SMALL:
				entity->x += (limbs[GYROBOT][5][0] * cos(my->pitch + PI / 8)) * cos(my->yaw);
				entity->y += (limbs[GYROBOT][5][0] * cos(my->pitch + PI / 8)) * sin(my->yaw);
				entity->z += limbs[GYROBOT][5][2] * sin(my->pitch + PI / 8);
				entity->focalx = limbs[GYROBOT][2][0];
				entity->focaly = limbs[GYROBOT][2][1];
				entity->focalz = limbs[GYROBOT][2][2];
				break;
			default:
				break;
		}
	}
}

void actGyroBotLimb(Entity* my)
{
	my->actMonsterLimb(false);
}

void gyroBotDie(Entity* my)
{
	bool gibs = true;
	if ( my->monsterSpecialState == GYRO_RETURN_LANDING )
	{
		// don't make noises etc.
		Stat* myStats = my->getStats();
		if ( myStats && !strncmp(myStats->obituary, language[3631], strlen(language[3631])) )
		{
			// returning to land, don't explode into gibs.
			gibs = false;
		}
	}
	
	my->removeMonsterDeathNodes();
	if ( gibs )
	{
		// playSoundEntity(my, 298 + rand() % 4, 128);
		int c;
		for ( c = 0; c < 4; c++ )
		{
			Entity* entity = spawnGib(my);
			if ( entity )
			{
				switch ( c )
				{
					case 0:
						entity->sprite = 886;
						break;
					case 1:
						entity->sprite = 887;
						break;
					case 2:
						entity->sprite = 888;
						break;
					case 3:
						entity->sprite = 874;
						break;
					default:
						break;
				}
				serverSpawnGibForClient(entity);
			}
		}
	}

	list_RemoveNode(my->mynode);
	return;
}

void initDummyBot(Entity* my, Stat* myStats)
{
	node_t* node;

	my->initMonster(889);
	my->flags[INVISIBLE] = true; // hide the "AI" bodypart
	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = -1;
		MONSTER_SPOTVAR = 1;
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

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

			// count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			//my->setHardcoreStats(*myStats);
		}
	}

	// head
	Entity* entity = newEntity(889, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->z = 6;
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[DUMMYBOT][1][0];
	entity->focaly = limbs[DUMMYBOT][1][1];
	entity->focalz = limbs[DUMMYBOT][1][2];
	entity->behavior = &actDummyBotLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// body
	entity = newEntity(890, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[DUMMYBOT][2][0];
	entity->focaly = limbs[DUMMYBOT][2][1];
	entity->focalz = limbs[DUMMYBOT][2][2];
	entity->behavior = &actDummyBotLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// shield
	entity = newEntity(891, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[DUMMYBOT][3][0];
	entity->focaly = limbs[DUMMYBOT][3][1];
	entity->focalz = limbs[DUMMYBOT][3][2];
	entity->behavior = &actDummyBotLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// box
	entity = newEntity(892, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	real_t prevYaw = entity->yaw;
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[DUMMYBOT][4][0];
	entity->focaly = limbs[DUMMYBOT][4][1];
	entity->focalz = limbs[DUMMYBOT][4][2];
	entity->behavior = &actDummyBotLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// lid
	entity = newEntity(893, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = prevYaw;
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[DUMMYBOT][5][0];
	entity->focaly = limbs[DUMMYBOT][5][1];
	entity->focalz = limbs[DUMMYBOT][5][2];
	entity->scalex = 1.01;
	entity->scaley = 1.01;
	entity->scalez = 1.01;
	entity->pitch = PI;
	entity->behavior = &actDummyBotLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// crank
	entity = newEntity(895, 0, map.entities, nullptr); //Limb entity.
	entity->sizex = 1;
	entity->sizey = 1;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = prevYaw;
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[DUMMYBOT][11][0];
	entity->focaly = limbs[DUMMYBOT][11][1];
	entity->focalz = limbs[DUMMYBOT][11][2];
	entity->fskill[0] = 1;
	entity->behavior = &actDummyBotLimb;
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

void actDummyBotLimb(Entity* my)
{
	my->actMonsterLimb(false);
}

void dummyBotDie(Entity* my)
{
	// playSoundEntity(my, 298 + rand() % 4, 128);
	bool gibs = true;
	if ( my->monsterSpecialState == DUMMYBOT_RETURN_FORM )
	{
		// don't make noises etc.
		Stat* myStats = my->getStats();
		if ( myStats && !strncmp(myStats->obituary, language[3643], strlen(language[3643])) )
		{
			// returning to box, don't explode into gibs.
			gibs = false;
		}
	}

	my->removeMonsterDeathNodes();
	if ( gibs )
	{
		int c;
		for ( c = 0; c < 5; c++ )
		{
			Entity* entity = spawnGib(my);
			if ( entity )
			{
				switch ( c )
				{
					case 0:
						entity->sprite = 889;
						break;
					case 1:
						entity->sprite = 890;
						break;
					case 2:
						entity->sprite = 891;
						break;
					case 3:
						entity->sprite = 892;
						break;
					case 4:
						entity->sprite = 893;
						break;
					default:
						break;
				}
				serverSpawnGibForClient(entity);
			}
		}
	}

	list_RemoveNode(my->mynode);
	return;
}

#define DUMMY_HEAD 2
#define DUMMY_BODY 3
#define DUMMY_SHIELD 4
#define DUMMY_BOX 5
#define DUMMY_LID 6
#define DUMMY_CRANK 7

void dummyBotAnimate(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = nullptr;
	Entity* head = nullptr;
	int bodypart;

	my->flags[INVISIBLE] = true; // hide the "AI" bodypart

	my->focalx = limbs[DUMMYBOT][0][0];
	my->focaly = limbs[DUMMYBOT][0][1];
	my->focalz = limbs[DUMMYBOT][0][2];
	if ( multiplayer != CLIENT )
	{
		my->z = 0;
	}

	//Move bodyparts
	for ( bodypart = 0, node = my->children.first; node != nullptr; node = node->next, ++bodypart )
	{
		if ( bodypart < DUMMY_HEAD )
		{
			continue;
		}

		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		if ( bodypart == DUMMY_HEAD )
		{
			head = entity;
			if ( multiplayer != CLIENT && entity->skill[0] == 2 )
			{
				if ( entity->skill[3] > 0 && myStats->HP < entity->skill[3] )
				{
					// on hit, bounce a bit.
					my->attack(MONSTER_POSE_RANGED_WINDUP1, 0, nullptr);
				}
				entity->skill[3] = myStats->HP;
			}

			if ( my->monsterSpecialState == DUMMYBOT_RETURN_FORM )
			{
				bool pitchZero = false;
				while ( entity->pitch > 2 * PI )
				{
					entity->pitch -= 2 * PI;
				}
				while ( entity->pitch < 0 )
				{
					entity->pitch += 2 * PI;
				}
				if ( entity->pitch > 0 && entity->pitch <= PI )
				{
					if ( limbAnimateToLimit(entity, ANIMATE_PITCH, -0.1, 0.0, false, 0.0) )
					{
						pitchZero = true;
					}
				}
				else
				{
					if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.1, 0.0, false, 0.0) )
					{
						pitchZero = true;
					}
				}
				entity->skill[0] = 3;
				real_t rate = 0.5;
				if ( entity->z > 12 )
				{
					rate = 0.1;
				}
				if ( pitchZero && limbAnimateToLimit(entity, ANIMATE_Z, rate, 7.5 + limbs[DUMMYBOT][6][2], false, 0.0) )
				{
					if ( multiplayer != CLIENT )
					{
						// kill me!
						int appearance = myStats->HP;
						Item* item = newItem(TOOL_DUMMYBOT, static_cast<Status>(myStats->monsterTinkeringStatus), 0, 1, appearance, true, &myStats->inventory);
						myStats->HP = 0;
						my->setObituary(language[3643]);
						return;
					}
				}
			}
			else if ( entity->skill[0] == 0 ) // non initialized.
			{
				entity->skill[0] = 1;
				entity->fskill[0] = -1;
				entity->z = 6 + limbs[DUMMYBOT][6][2];
			}
			else if ( entity->skill[0] == 1 ) // rising.
			{
				if ( limbAnimateToLimit(entity, ANIMATE_Z, entity->fskill[0], limbs[DUMMYBOT][6][2], false, 0.0) )
				{
					entity->skill[0] = 2;
					entity->z = my->z;
				}
				else
				{
					entity->fskill[0] *= 0.85;
				}
			}
			else
			{
				entity->z = my->z;
				if ( my->monsterAttack == MONSTER_POSE_RANGED_WINDUP1 )
				{
					my->monsterAttack = 0;
					entity->skill[4] = 1;
					entity->fskill[1] = 0.06;
					entity->fskill[2] = PI / 6;
					entity->monsterAnimationLimbOvershoot = ANIMATE_OVERSHOOT_TO_SETPOINT;
				}
				if ( entity->skill[4] > 0 )
				{
					if ( entity->monsterAnimationLimbOvershoot == ANIMATE_OVERSHOOT_NONE )
					{
						if ( entity->pitch > 0 )
						{
							if ( limbAnimateToLimit(entity, ANIMATE_PITCH, -entity->fskill[1], 0.0, false, 0.0) )
							{
								entity->skill[4] = 0;
							}
						}
						else
						{
							if ( limbAnimateToLimit(entity, ANIMATE_PITCH, entity->fskill[1], 0.0, false, 0.0) )
							{
								entity->skill[4] = 0;
							}
						}
					}
					else
					{
						limbAnimateWithOvershoot(entity, ANIMATE_PITCH, entity->fskill[1], 2 * PI - entity->fskill[2], 
							entity->fskill[1], PI / 12, ANIMATE_DIR_NEGATIVE);
					}
				}
			}
		}
		else if ( bodypart != DUMMY_LID && bodypart != DUMMY_BOX && bodypart != DUMMY_CRANK )
		{
			if ( head )
			{
				if ( head->skill[0] == 2 )
				{
					entity->z = my->z;
					entity->pitch = head->pitch;
				}
				else
				{
					if ( head->skill[0] == 3 ) // returning to box
					{
						entity->pitch = head->pitch;
					}
					entity->z = head->z - limbs[DUMMYBOT][6][2];
				}
			}
		}

		if ( bodypart == DUMMY_BODY || bodypart == DUMMY_SHIELD || bodypart == DUMMY_HEAD )
		{
			entity->yaw = my->yaw;
		}
		else if ( bodypart == DUMMY_LID )
		{
			if ( my->monsterSpecialState == DUMMYBOT_RETURN_FORM )
			{
				if ( head && head->z > 11 )
				{
					limbAnimateToLimit(entity, ANIMATE_PITCH, 0.5, PI, false, 0.0);
				}
			}
			else if ( entity->skill[0] == 0 )
			{
				if ( limbAnimateToLimit(entity, ANIMATE_PITCH, -0.5, 7 * PI / 4, false, 0.0) )
				{
					entity->skill[0] = 1;
				}
			}
		}
		else if ( bodypart == DUMMY_CRANK )
		{
			if ( my->monsterSpecialState == DUMMYBOT_RETURN_FORM )
			{
				entity->pitch -= 0.5;
				if ( entity->pitch < 0 )
				{
					entity->pitch += 2 * PI;
				}
			}
			else if ( entity->fskill[0] > 0.08 )
			{
				entity->pitch += entity->fskill[0];
				if ( entity->pitch > 2 * PI )
				{
					entity->pitch -= 2 * PI;
				}
				entity->fskill[0] *= 0.95;
			}
			else if ( entity->skill[0] == 0 )
			{
				// fall to rest on a 90 degree angle.
				if ( entity->pitch < PI / 2 )
				{
					if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.08, PI / 2, false, 0.f) )
					{
						entity->skill[0] = 1;
					}
				}
				else if ( entity->pitch < PI )
				{
					if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.08, PI, false, 0.f) )
					{
						entity->skill[0] = 1;
					}
				}
				else if ( entity->pitch < (3 * PI / 2) )
				{
					if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.08, 3 * PI / 2, false, 0.f) )
					{
						entity->skill[0] = 1;
					}
				}
				else
				{
					if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.08, 0.f, false, 0.f) )
					{
						entity->skill[0] = 1;
					}
				}
			}
		}

		switch ( bodypart )
		{
			case DUMMY_HEAD:
				entity->x += limbs[DUMMYBOT][6][0] * cos(my->yaw);
				entity->y += limbs[DUMMYBOT][6][1] * sin(my->yaw);
				if ( entity->skill[0] == 2 )
				{
					entity->z += limbs[DUMMYBOT][6][2];
				}
				entity->focalx = limbs[DUMMYBOT][1][0];
				entity->focaly = limbs[DUMMYBOT][1][1];
				entity->focalz = limbs[DUMMYBOT][1][2];
				break;
			case DUMMY_BODY:
				entity->x += limbs[DUMMYBOT][7][0] * cos(my->yaw);
				entity->y += limbs[DUMMYBOT][7][1] * sin(my->yaw);
				entity->z += limbs[DUMMYBOT][7][2];
				entity->focalx = limbs[DUMMYBOT][2][0];
				entity->focaly = limbs[DUMMYBOT][2][1];
				entity->focalz = limbs[DUMMYBOT][2][2];
				break;
			case DUMMY_SHIELD:
				if ( head && head->skill[0] != 2 )
				{
					entity->flags[INVISIBLE] = true;
				}
				else
				{
					if ( entity->flags[INVISIBLE] )
					{
						playSoundEntityLocal(my, 44 + rand() % 3, 92);
					}
					entity->flags[INVISIBLE] = false;
				}
				entity->x += limbs[DUMMYBOT][8][0] * cos(my->yaw) + limbs[DUMMYBOT][8][1] * cos(my->yaw + PI / 2);
				entity->y += limbs[DUMMYBOT][8][0] * sin(my->yaw) + limbs[DUMMYBOT][8][1] * sin(my->yaw + PI / 2);
				entity->z += limbs[DUMMYBOT][8][2];
				entity->focalx = limbs[DUMMYBOT][3][0];
				entity->focaly = limbs[DUMMYBOT][3][1];
				entity->focalz = limbs[DUMMYBOT][3][2];
				break;
			case DUMMY_BOX:
				entity->x += limbs[DUMMYBOT][9][0] * cos(entity->yaw);
				entity->y += limbs[DUMMYBOT][9][1] * sin(entity->yaw);
				entity->z = limbs[DUMMYBOT][9][2];
				entity->focalx = limbs[DUMMYBOT][4][0];
				entity->focaly = limbs[DUMMYBOT][4][1];
				entity->focalz = limbs[DUMMYBOT][4][2];
				break;
			case DUMMY_LID:
				entity->x += limbs[DUMMYBOT][10][0] * cos(entity->yaw);
				entity->y += limbs[DUMMYBOT][10][1] * sin(entity->yaw);
				entity->z = limbs[DUMMYBOT][10][2];
				entity->focalx = limbs[DUMMYBOT][5][0];
				entity->focaly = limbs[DUMMYBOT][5][1];
				entity->focalz = limbs[DUMMYBOT][5][2];
				break;
			case DUMMY_CRANK:
				entity->x += limbs[DUMMYBOT][12][0] * cos(entity->yaw) + limbs[DUMMYBOT][12][1] * cos(entity->yaw + PI / 2);
				entity->y += limbs[DUMMYBOT][12][0] * sin(entity->yaw) + limbs[DUMMYBOT][12][1] * sin(entity->yaw + PI / 2);
				entity->z = limbs[DUMMYBOT][12][2];
				entity->focalx = limbs[DUMMYBOT][11][0];
				entity->focaly = limbs[DUMMYBOT][11][1];
				entity->focalz = limbs[DUMMYBOT][11][2];
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