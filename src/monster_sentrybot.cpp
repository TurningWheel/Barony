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
#include "engine/audio/sound.hpp"
#include "book.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "magic/magic.hpp"
#include "interface/interface.hpp"
#include "prng.hpp"
#include "mod_tools.hpp"

std::unordered_map<Uint32, int> gyroBotDetectedUids;

void initSentryBot(Entity* my, Stat* myStats)
{
	node_t* node;

	my->flags[BURNABLE] = false;
	my->initMonster(my->sprite);
	my->z = 0;

	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 456;
		MONSTER_SPOTVAR = 3;
		MONSTER_IDLESND = -1;
		MONSTER_IDLEVAR = 1;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		auto& rng = my->entity_rng ? *my->entity_rng : local_rng;

		if ( myStats != nullptr )
		{
			if ( !myStats->leader_uid )
			{
				myStats->leader_uid = 0;
			}

			// apply random stat increases if set in stat_shared.cpp or editor
			setRandomMonsterStats(myStats, rng);

			// generate 6 items max, less if there are any forced items from boss variants
			int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats, rng);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate, rng);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

			// count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			//my->setHardcoreStats(*myStats);

			if ( myStats->weapon == nullptr && my->sprite == 872/*&& myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1*/ )
			{
				myStats->weapon = newItem(CROSSBOW, EXCELLENT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
			}
			else if ( myStats->weapon == nullptr && my->sprite == 885/*&& myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1*/ )
			{
				if ( myStats->LVL >= 15 )
				{
					myStats->weapon = newItem(SPELLBOOK_MAGICMISSILE, EXCELLENT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
				else
				{
					myStats->weapon = newItem(SPELLBOOK_FORCEBOLT, EXCELLENT, 0, 1, MONSTER_ITEM_UNDROPPABLE_APPEARANCE, false, nullptr);
				}
			}
		}
	}

	int race = my->getMonsterTypeFromSprite();

	// tripod
	Entity* entity = newEntity(873, 1, map.entities, nullptr); //Limb entity.
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
	entity = newEntity(874, 1, map.entities, nullptr); //Limb entity.
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
	entity = newEntity(874, 1, map.entities, nullptr); //Limb entity.
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
	entity = newEntity(874, 1, map.entities, nullptr); //Limb entity.
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
	entity = newEntity(874, 1, map.entities, nullptr); //Limb entity.
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
	entity = newEntity(875, 1, map.entities, nullptr); //Limb entity.
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
	entity = newEntity(876, 1, map.entities, nullptr); //Limb entity.
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
	entity = newEntity(167, 1, map.entities, nullptr); //Limb entity.
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
	gyroBotDetectedUids.clear();

    my->z = 5;
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
		auto& rng = my->entity_rng ? *my->entity_rng : local_rng;

		if ( myStats != nullptr )
		{
			if ( !myStats->leader_uid )
			{
				myStats->leader_uid = 0;
			}

			// apply random stat increases if set in stat_shared.cpp or editor
			setRandomMonsterStats(myStats, rng);

			myStats->EFFECTS[EFF_LEVITATING] = true;
			myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 0;

			// generate 6 items max, less if there are any forced items from boss variants
			int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats, rng);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate, rng);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

			// count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			//my->setHardcoreStats(*myStats);
		}
	}

	// rotor large
	Entity* entity = newEntity(887, 1, map.entities, nullptr); //Limb entity.
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
	entity = newEntity(888, 1, map.entities, nullptr); //Limb entity.
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

	// bomb
	entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->flags[INVISIBLE] = true;
	entity->yaw = my->yaw;
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[GYROBOT][6][0];
	entity->focaly = limbs[GYROBOT][6][1];
	entity->focalz = limbs[GYROBOT][6][2];
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
		if ( myStats && !strncmp(myStats->obituary, Language::get(3631), strlen(Language::get(3631))) )
		{
			// returning to land, don't explode into gibs.
			gibs = false;
		}
	}
	else
	{
		ItemType type = TOOL_SENTRYBOT;
		Stat* myStats = my->getStats();
		if ( myStats && myStats->type == SPELLBOT )
		{
			type = TOOL_SPELLBOT;
		}
		bool dropBrokenShell = true;
		if ( myStats && myStats->monsterNoDropItems == 1 && !my->monsterAllyGetPlayerLeader() )
		{
			dropBrokenShell = false;
		}
		/*if ( myStats->monsterTinkeringStatus == EXCELLENT && local_rng.rand() % 100 < 90 )
		{
			dropBrokenShell = true;
		}
		else if ( myStats->monsterTinkeringStatus == SERVICABLE && local_rng.rand() % 100 < 80 )
		{
			dropBrokenShell = true;
		}
		else if ( myStats->monsterTinkeringStatus == WORN && local_rng.rand() % 100 < 70 )
		{
			dropBrokenShell = true;
		}
		else if ( myStats->monsterTinkeringStatus == DECREPIT && local_rng.rand() % 100 < 60 )
		{
			dropBrokenShell = true;
		}*/

		if ( dropBrokenShell )
		{
			Item* item = newItem(type, BROKEN, 0, 1, 0, true, nullptr);
			Entity* entity = dropItemMonster(item, my, myStats);
			if ( entity )
			{
				entity->flags[USERFLAG1] = true;    // makes items passable, improves performance
			}
		}
		playSoundEntity(my, 451 + local_rng.rand() % 2, 128);
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
			    entity->skill[5] = 1; // poof
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

	// playSoundEntity(my, 298 + local_rng.rand() % 4, 128);
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
				int appearance = monsterTinkeringConvertHPToAppearance(myStats);
				ItemType type = TOOL_SENTRYBOT;
				if ( myStats->type == SPELLBOT )
				{
					type = TOOL_SPELLBOT;
				}
				Item* item = newItem(type, static_cast<Status>(myStats->monsterTinkeringStatus), 0, 1, appearance, true, &myStats->inventory);
				myStats->HP = 0;
				myStats->killer = KilledBy::NO_FUEL;
				my->setObituary(Language::get(3631));
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

	if ( ticks % (3 * TICKS_PER_SECOND) == 0 && local_rng.rand() % 5 > 0 )
	{
		playSoundEntityLocal(my, 259, 8);
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
				if ( my->monsterAttack == MONSTER_POSE_MAGIC_WINDUP1 )
				{
					if ( my->monsterAttackTime == 0 )
					{
						//createParticleDot(my);
						Entity* particle = createParticleAestheticOrbit(my, 173, 15, PARTICLE_EFFECT_SPELLBOT_ORBIT);
						if ( particle )
						{
							particle->actmagicOrbitDist = 1;
							particle->x = my->x + 2 * cos(my->yaw);
							particle->y = my->y + 2 * sin(my->yaw);
							particle->fskill[0] = particle->x;
							particle->fskill[1] = particle->y;
							particle->z = my->z - 1.5;
							particle->scalex = 0.5;
							particle->scaley = 0.5;
							particle->scalez = 0.5;
						}
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
				else if ( my->monsterAttack == MONSTER_POSE_RANGED_WINDUP1 )
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
							my->attack(MONSTER_POSE_MAGIC_CAST1, 0, nullptr);
						}
					}
				}
				else if ( my->monsterAttack == MONSTER_POSE_RANGED_SHOOT1 || my->monsterAttack == MONSTER_POSE_MAGIC_CAST1 )
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
#define GYRO_BOMB 4

bool gyroBotFoundNewEntity(Entity& ent)
{
	auto find = gyroBotDetectedUids.find(ent.getUID());
	if ( find == gyroBotDetectedUids.end() )
	{
		gyroBotDetectedUids.insert(std::make_pair(ent.getUID(), ticks));
		return true; // new uid.
	}
	else
	{
		if ( ent.behavior == &actItem )
		{
			gyroBotDetectedUids[ent.getUID()] = ticks;
			return false; // items never alert again.
		}
		else if ( ticks - gyroBotDetectedUids[ent.getUID()] > 20 * TICKS_PER_SECOND )
		{
			gyroBotDetectedUids[ent.getUID()] = ticks;
			return true; // count this as new, it will be a monster/trap/something important.
		}
		else
		{
			gyroBotDetectedUids[ent.getUID()] = ticks;
			return false;
		}
	}
	return false;
}

void gyroBotAnimate(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = nullptr;
	int bodypart;

	if ( multiplayer != CLIENT )
	{
		// sleeping
		//if ( myStats->EFFECTS[EFF_ASLEEP] )
		//{
		//	//my->z = 4;
		//	my->pitch = PI / 4;
		//}
		//else
		//{
		//	//my->z = 2.25;
		//	//my->pitch = 0;
		//}
		/*if ( my->monsterSpecialState == GYRO_RETURN_LANDING || my->monsterSpecialState == GYRO_INTERACT_LANDING )
		{
			my->flags[PASSABLE] = false;
		}
		else
		{
		}*/
		my->flags[PASSABLE] = true;

		if ( my->ticks == 25 )
		{
			// drop any bots we collected from the previous level.
			node_t* invNodeNext = nullptr;
			bool dropped = false;
			for ( node_t* invNode = myStats->inventory.first; invNode; invNode = invNodeNext )
			{
				invNodeNext = invNode->next;
				Item* item = (Item*)invNode->element;
				if ( item && (item->type == TOOL_DUMMYBOT || item->type == TOOL_SENTRYBOT || item->type == TOOL_SPELLBOT) )
				{
					for ( int c = item->count; c > 0; c-- )
					{
						Entity* itemDropped = dropItemMonster(item, my, myStats);
						if ( itemDropped )
						{
							dropped = true;
							itemDropped->flags[USERFLAG1] = true;    // makes items passable, improves performance
						}
					}
				}
			}
			if ( dropped )
			{
				int leader = my->monsterAllyIndex;
				if ( leader >= 0 )
				{
					Uint32 color = makeColorRGB(0, 255, 0);
					messagePlayerColor(leader, MESSAGE_HINT, color, Language::get(3651));
				}
			}
		}
	}

	int detectDuration = 5 * TICKS_PER_SECOND;
	if ( my->ticks % (detectDuration) == 0 && my->monsterAllyIndex >= 0 && players[my->monsterAllyIndex]->isLocalPlayer() )
	{
		Entity* playerLeader = my->monsterAllyGetPlayerLeader();
		bool doPing = false;
		int foundGoodSound = 0;
		int foundBadSound = 0;
		for ( node_t* searchNode = map.entities->first; searchNode != nullptr; searchNode = searchNode->next )
		{
			Entity* ent = (Entity*)searchNode->element;
			if ( !ent || ent == my )
			{
				continue;
			}
			if ( ent->skill[28] > 0 ) // mechanism
			{
				if ( my->monsterAllyPickupItems != ALLY_GYRO_DETECT_TRAPS )
				{
					continue;
				}
			}
			if ( playerLeader )
			{
				if ( my->monsterAllyPickupItems == ALLY_GYRO_DETECT_MONSTERS )
				{
					if ( (ent->behavior == &actMonster && ent->monsterAllyIndex < 0)
						|| (ent->isDamageableCollider() && ent->colliderHideMonster != 0) )
					{
						if ( entityDist(my, ent) < TOUCHRANGE * 5 )
						{
							if ( gyroBotFoundNewEntity(*ent) )
							{
								++foundBadSound;
							}
							if ( ent->entityShowOnMap < detectDuration )
							{
								ent->entityShowOnMap = detectDuration;
							}
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
						if ( entityDist(my, ent) < TOUCHRANGE * 5 )
						{
							if ( gyroBotFoundNewEntity(*ent) )
							{
								foundBadSound = 3;
							}
							if ( ent->entityShowOnMap < detectDuration )
							{
								ent->entityShowOnMap = detectDuration;
							}
						}
					}
				}
				else if ( my->monsterAllyPickupItems == ALLY_GYRO_DETECT_EXITS )
				{
					if ( ent->behavior == &actLadder || ent->behavior == &actPortal )
					{
						if ( entityDist(my, ent) < TOUCHRANGE * 5 )
						{
							if ( gyroBotFoundNewEntity(*ent) )
							{
								foundGoodSound = 5;
							}
							if ( ent->entityShowOnMap < detectDuration )
							{
								ent->entityShowOnMap = detectDuration;
							}
						}
					}
				}
				else if ( my->monsterAllyPickupItems == ALLY_GYRO_DETECT_ITEMS_METAL
					|| my->monsterAllyPickupItems == ALLY_GYRO_DETECT_ITEMS_MAGIC
					|| my->monsterAllyPickupItems == ALLY_GYRO_DETECT_ITEMS_VALUABLE )
				{
					if ( ent->behavior == &actItem )
					{
						if ( entityDist(my, ent) < TOUCHRANGE * 5 )
						{
							Item* itemOnGround = newItemFromEntity(ent);
							int metal = 0;
							int magic = 0;
							if ( itemOnGround )
							{
								GenericGUIMenu::tinkeringGetItemValue(itemOnGround, &metal, &magic);
								if ( my->monsterAllyPickupItems == ALLY_GYRO_DETECT_ITEMS_METAL
									&& metal > 0 )
								{
									if ( gyroBotFoundNewEntity(*ent) )
									{
										++foundGoodSound;
									}
									if ( ent->entityShowOnMap < detectDuration )
									{
										ent->entityShowOnMap = detectDuration;
									}
								}
								else if ( my->monsterAllyPickupItems == ALLY_GYRO_DETECT_ITEMS_MAGIC
									&& magic > 0 )
								{
									if ( gyroBotFoundNewEntity(*ent) )
									{
										++foundGoodSound;
									}
									if ( ent->entityShowOnMap < detectDuration )
									{
										ent->entityShowOnMap = detectDuration;
									}
								}
								else if ( my->monsterAllyPickupItems == ALLY_GYRO_DETECT_ITEMS_VALUABLE
									&& items[itemOnGround->type].value >= 400 )
								{
									if ( gyroBotFoundNewEntity(*ent) )
									{
										foundGoodSound = 5;
									}
									if ( ent->entityShowOnMap < detectDuration )
									{
										ent->entityShowOnMap = detectDuration;
									}
								}
								free(itemOnGround);
							}
						}
					}
				}
			}
			if ( ent->entityShowOnMap > 0 )
			{
				doPing = true;
			}
		}
		if ( doPing )
		{
			int pingx = my->x / 16;
			int pingy = my->y / 16;
			MinimapPing radiusPing(ticks, my->monsterAllyIndex, pingx, pingy, true);
			minimapPingAdd(my->monsterAllyIndex, my->monsterAllyIndex, radiusPing);

			if ( foundGoodSound >= 1 )
			{
				playSoundEntity(my, 444 + local_rng.rand() % 5, 128);
			}
			else if ( foundBadSound >= 1 )
			{
				playSoundEntity(my, 450, 128);
			}
		}
	}

	my->removeLightField();
	if ( my->monsterAllyClass > ALLY_GYRO_LIGHT_NONE )
	{
		switch ( my->monsterAllyClass )
		{
			case ALLY_GYRO_LIGHT_FAINT:
				my->light = addLight(my->x / 16, my->y / 16, "gyrobot_faint");
				break;
			case ALLY_GYRO_LIGHT_BRIGHT:
				my->light = addLight(my->x / 16, my->y / 16, "gyrobot_bright");
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
		if ( my->ticks % (TICKS_PER_SECOND * 15) == 0 
			&& my->monsterSpecialTimer == 0
			&& my->monsterSpecialState == 0 )
		{
			bool doACoolFlip = true;

			for ( bodypart = 0, node = my->children.first; node != nullptr; node = node->next, ++bodypart )
			{
				if ( bodypart == GYRO_BOMB )
				{
					entity = (Entity*)node->element;
					if ( entity )
					{
						if ( entity->sprite == items[TOOL_SENTRYBOT].index )
						{
							doACoolFlip = false;
						}
						else if ( entity->sprite == items[TOOL_SPELLBOT].index )
						{
							doACoolFlip = false;
						}
					}
					break;
				}
			}

			if ( doACoolFlip )
			{
				my->attack(MONSTER_POSE_RANGED_WINDUP1, 0, nullptr);
				my->monsterSpecialTimer = TICKS_PER_SECOND * 8;

				if ( auto leader = my->monsterAllyGetPlayerLeader() )
				{
					Compendium_t::Events_t::eventUpdateMonster(leader->skill[2], Compendium_t::CPDM_GYROBOT_FLIPS, my, 1);
				}
			}
		}

		if ( my->monsterSpecialState == GYRO_RETURN_LANDING )
		{
			if ( limbAnimateToLimit(my, ANIMATE_Z, 0.05, 0, false, 0.0) )
			{
				int appearance = monsterTinkeringConvertHPToAppearance(myStats);
				Item* item = newItem(TOOL_GYROBOT, static_cast<Status>(myStats->monsterTinkeringStatus), 0, 1, appearance, true, &myStats->inventory);
				myStats->HP = 0;
				myStats->killer = KilledBy::NO_FUEL;
				my->setObituary(Language::get(3631));
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
		else if ( bodypart == GYRO_BOMB )
		{
			entity->pitch = my->pitch + PI / 2;
			entity->yaw = my->yaw;
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
			case GYRO_BOMB:
				entity->x += limbs[GYROBOT][7][0] * sin(my->pitch) * cos(my->yaw);
				entity->y += limbs[GYROBOT][7][1] * sin(my->pitch) * sin(my->yaw);
				entity->z += limbs[GYROBOT][7][2] * cos(my->pitch);
				entity->focalx = limbs[GYROBOT][6][0];
				entity->focaly = limbs[GYROBOT][6][1];
				entity->focalz = limbs[GYROBOT][6][2];

				if ( multiplayer != CLIENT )
				{
					entity->sprite = -1;
					for ( node_t* inv = myStats->inventory.first; inv; inv = inv->next )
					{
						Item* holding = (Item*)inv->element;
						if ( holding && itemIsThrowableTinkerTool(holding) )
						{
							entity->sprite = items[holding->type].index;
						}
					}
					if ( entity->sprite == -1 )
					{
						entity->flags[INVISIBLE] = true;
					}
					else
					{
						entity->flags[INVISIBLE] = my->flags[INVISIBLE];
					}
				}

				if ( entity->sprite == items[TOOL_SENTRYBOT].index )
				{
					entity->focalx = limbs[GYROBOT][8][0];
					entity->focaly = limbs[GYROBOT][8][1];
					entity->focalz = limbs[GYROBOT][8][2];
				}
				else if ( entity->sprite == items[TOOL_SPELLBOT].index )
				{
					entity->focalx = limbs[GYROBOT][9][0];
					entity->focaly = limbs[GYROBOT][9][1];
					entity->focalz = limbs[GYROBOT][9][2];
				}
				else if ( entity->sprite == items[TOOL_GYROBOT].index )
				{
					entity->focalx = limbs[GYROBOT][10][0];
					entity->focaly = limbs[GYROBOT][10][1];
					entity->focalz = limbs[GYROBOT][10][2];
				}
				else if ( entity->sprite == items[TOOL_DUMMYBOT].index || entity->sprite == items[TOOL_DECOY].index )
				{
					entity->focalx = limbs[GYROBOT][11][0];
					entity->focaly = limbs[GYROBOT][11][1];
					entity->focalz = limbs[GYROBOT][11][2];
				}

				if ( multiplayer == SERVER )
				{
					// update sprites for clients
					if ( entity->ticks >= *cvar_entity_bodypart_sync_tick )
					{
						bool updateBodypart = false;
						if ( entity->skill[10] != entity->sprite )
						{
							entity->skill[10] = entity->sprite;
							updateBodypart = true;
						}
						if ( entity->getUID() % (TICKS_PER_SECOND * 10) == ticks % (TICKS_PER_SECOND * 10) )
						{
							updateBodypart = true;
						}
						if ( updateBodypart )
						{
							serverUpdateEntityBodypart(my, bodypart);
						}
					}
				}
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
		if ( myStats && !strncmp(myStats->obituary, Language::get(3631), strlen(Language::get(3631))) )
		{
			// returning to land, don't explode into gibs.
			gibs = false;
		}
	}
	
	my->removeMonsterDeathNodes();
	if ( gibs )
	{
		playSoundEntity(my, 451 + local_rng.rand() % 2, 128);
		playSoundEntity(my, 450, 128);
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

	my->z = 0;
	my->initMonster(889);
	my->flags[INVISIBLE] = true; // hide the "AI" bodypart
	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = -1;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = 456;
		MONSTER_IDLEVAR = 3;
	}
	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		auto& rng = my->entity_rng ? *my->entity_rng : local_rng;

		if ( myStats != nullptr )
		{
			if ( !myStats->leader_uid )
			{
				myStats->leader_uid = 0;
			}

			// apply random stat increases if set in stat_shared.cpp or editor
			setRandomMonsterStats(myStats, rng);

			// generate 6 items max, less if there are any forced items from boss variants
			int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats, rng);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate, rng);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

			// count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			//my->setHardcoreStats(*myStats);
		}
	}

	// head
	Entity* entity = newEntity(889, 1, map.entities, nullptr); //Limb entity.
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
	entity = newEntity(890, 1, map.entities, nullptr); //Limb entity.
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
	entity = newEntity(891, 1, map.entities, nullptr); //Limb entity.
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
	entity = newEntity(892, 1, map.entities, nullptr); //Limb entity.
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
	entity = newEntity(893, 1, map.entities, nullptr); //Limb entity.
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
	entity = newEntity(895, 1, map.entities, nullptr); //Limb entity.
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
	bool gibs = true;
	if ( my->monsterSpecialState == DUMMYBOT_RETURN_FORM )
	{
		// don't make noises etc.
		Stat* myStats = my->getStats();
		if ( myStats && !strncmp(myStats->obituary, Language::get(3643), strlen(Language::get(3643))) )
		{
			// returning to box, don't explode into gibs.
			gibs = false;
		}
	}
	else
	{
		Stat* myStats = my->getStats();
		bool dropBrokenShell = true;
		if ( myStats && myStats->monsterNoDropItems == 1 && !my->monsterAllyGetPlayerLeader() )
		{
			dropBrokenShell = false;
		}
		/*if ( myStats->monsterTinkeringStatus == EXCELLENT && local_rng.rand() % 100 < 80 )
		{
			dropBrokenShell = true;
		}
		else if ( myStats->monsterTinkeringStatus == SERVICABLE && local_rng.rand() % 100 < 60 )
		{
			dropBrokenShell = true;
		}
		else if ( myStats->monsterTinkeringStatus == WORN && local_rng.rand() % 100 < 40 )
		{
			dropBrokenShell = true;
		}
		else if ( myStats->monsterTinkeringStatus == DECREPIT && local_rng.rand() % 100 < 20 )
		{
			dropBrokenShell = true;
		}*/

		if ( dropBrokenShell )
		{
			Item* item = newItem(TOOL_DUMMYBOT, BROKEN, 0, 1, 0, true, nullptr);
			Entity* entity = dropItemMonster(item, my, myStats);
			if ( entity )
			{
				entity->flags[USERFLAG1] = true;    // makes items passable, improves performance
			}
		}
	}

	my->removeMonsterDeathNodes();
	if ( gibs )
	{
		playSoundEntity(my, 451 + local_rng.rand() % 2, 128);
		int c;
		for ( c = 0; c < 5; c++ )
		{
			Entity* entity = spawnGib(my);
			if ( entity )
			{
			    entity->skill[5] = 1; // poof
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
				if ( myStats )
				{
					if ( entity->skill[3] > 0 && myStats->HP < entity->skill[3] )
					{
						// on hit, bounce a bit.
						my->attack(MONSTER_POSE_RANGED_WINDUP1, 0, nullptr);
						if ( Entity* leader = my->monsterAllyGetPlayerLeader() )
						{
							Compendium_t::Events_t::eventUpdate(leader->skill[2],
								Compendium_t::CPDM_DUMMY_HITS_TAKEN, TOOL_DUMMYBOT, 1);
							Compendium_t::Events_t::eventUpdate(leader->skill[2],
								Compendium_t::CPDM_DUMMY_DMG_TAKEN, TOOL_DUMMYBOT, std::max(0, entity->skill[3] - myStats->HP));
						}
					}
					entity->skill[3] = myStats->HP;
				}
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
						int appearance = monsterTinkeringConvertHPToAppearance(myStats);
						Item* item = newItem(TOOL_DUMMYBOT, static_cast<Status>(myStats->monsterTinkeringStatus), 0, 1, appearance, true, &myStats->inventory);
						myStats->HP = 0;
						myStats->killer = KilledBy::NO_FUEL;
						my->setObituary(Language::get(3643));
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
						playSoundEntityLocal(my, 44 + local_rng.rand() % 3, 92);
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

void Entity::tinkerBotSetStats(Stat* myStats, int rank)
{
	if ( !myStats )
	{
		return;
	}

	if ( myStats->type == SENTRYBOT )
	{
		switch ( rank )
		{
			case DECREPIT:
				myStats->LVL = 3;
				myStats->HP = 50;
				myStats->CON = 0;
				myStats->PER = 4;
				break;
			case WORN:
				myStats->LVL = 5;
				myStats->HP = 75;
				myStats->CON = 3;
				myStats->PER = 8;
				break;
			case SERVICABLE:
				myStats->LVL = 10;
				myStats->HP = 125;
				myStats->CON = 6;
				myStats->PER = 12;
				break;
			case EXCELLENT:
				myStats->LVL = 15;
				myStats->HP = 150;
				myStats->CON = 9;
				myStats->PER = 16;
				break;
			default:
				break;
		}
	}
	else if ( myStats->type == SPELLBOT )
	{
		switch ( rank )
		{
			case DECREPIT:
				myStats->LVL = 3;
				myStats->HP = 50;
				myStats->CON = 0;
				myStats->PER = 4;
				break;
			case WORN:
				myStats->LVL = 5;
				myStats->HP = 75;
				myStats->CON = 3;
				myStats->PER = 8;
				break;
			case SERVICABLE:
				myStats->LVL = 10;
				myStats->HP = 125;
				myStats->CON = 6;
				myStats->PER = 12;
				break;
			case EXCELLENT:
				myStats->LVL = 15;
				myStats->HP = 150;
				myStats->CON = 9;
				myStats->PER = 16;
				break;
			default:
				break;
		}
	}
	else if ( myStats->type == GYROBOT )
	{
		switch ( rank )
		{
			case DECREPIT:
				myStats->LVL = 1;
				myStats->HP = 10;
				break;
			case WORN:
				myStats->LVL = 5;
				myStats->HP = 35;
				break;
			case SERVICABLE:
				myStats->LVL = 10;
				myStats->HP = 60;
				break;
			case EXCELLENT:
				myStats->LVL = 15;
				myStats->HP = 85;
				break;
			default:
				break;
		}
	}
	else if ( myStats->type == DUMMYBOT )
	{
		switch ( rank )
		{
			case DECREPIT:
				myStats->LVL = 3;
				myStats->HP = 50;
				myStats->CON = 5;
				break;
			case WORN:
				myStats->LVL = 5;
				myStats->HP = 100;
				myStats->CON = 8;
				break;
			case SERVICABLE:
				myStats->LVL = 10;
				myStats->HP = 150;
				myStats->CON = 10;
				break;
			case EXCELLENT:
				myStats->LVL = 15;
				myStats->HP = 200;
				myStats->CON = 15;
				break;
			default:
				break;
		}
	}

	myStats->MAXHP = myStats->HP;
	myStats->OLDHP = myStats->HP;
	return;
}
