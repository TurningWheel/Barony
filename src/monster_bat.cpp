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

void initBat(Entity* my, Stat* myStats)
{
	node_t* node;

	my->z = 0;
	my->initMonster(1408);
	my->flags[INVISIBLE] = true; // hide the "AI" bodypart
	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 666;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = 667;
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

			if ( isMonsterStatsDefault(*myStats) )
			{
				myStats->STR += std::min(5, currentlevel / 5);
				myStats->DEX += std::min(3, currentlevel / 5);
				myStats->HP += std::min(30, 5 * (currentlevel / 5));
				myStats->MAXHP = myStats->HP;
				myStats->OLDHP = myStats->HP;
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

			my->setHardcoreStats(*myStats);
		}
	}

	// body
	Entity* entity = newEntity(1408, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->z = 6;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[BAT_SMALL][1][0];
	entity->focaly = limbs[BAT_SMALL][1][1];
	entity->focalz = limbs[BAT_SMALL][1][2];
	entity->behavior = &actBatLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// head
	entity = newEntity(1409, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[BAT_SMALL][2][0];
	entity->focaly = limbs[BAT_SMALL][2][1];
	entity->focalz = limbs[BAT_SMALL][2][2];
	entity->behavior = &actBatLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// wingleft
	entity = newEntity(1410, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[BAT_SMALL][3][0];
	entity->focaly = limbs[BAT_SMALL][3][1];
	entity->focalz = limbs[BAT_SMALL][3][2];
	entity->behavior = &actBatLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// wingright
	entity = newEntity(1411, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[BAT_SMALL][4][0];
	entity->focaly = limbs[BAT_SMALL][4][1];
	entity->focalz = limbs[BAT_SMALL][4][2];
	entity->behavior = &actBatLimb;
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

void actBatLimb(Entity* my)
{
	my->actMonsterLimb(false);
}

void batDie(Entity* my)
{
	int c;
	for ( c = 0; c < 4; c++ )
	{
		Entity* entity = spawnGib(my);
		if ( entity )
		{
			entity->skill[5] = 1; // poof

			switch ( c )
			{
			case 0:
				entity->sprite = 1408;
				break;
			case 1:
				entity->sprite = 1409;
				break;
			case 2:
				entity->sprite = 1410;
				break;
			case 3:
				entity->sprite = 1411;
				break;
			default:
				break;
			}

			serverSpawnGibForClient(entity);
		}
	}

	my->spawnBlood();

	playSoundEntity(my, 670 + local_rng.rand() % 2, 128);

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define BAT_BODY 2
#define BAT_HEAD 3
#define BAT_LEFTWING 4
#define BAT_RIGHTWING 5

#define BAT_FLOAT_X body->fskill[2]
#define BAT_FLOAT_Y body->fskill[3]
#define BAT_FLOAT_Z body->fskill[4]
#define BAT_FLOAT_ATK body->fskill[5]
#define BAT_REST_FLY_Z body->fskill[6]
#define BAT_REST_STATE body->skill[3]
#define BAT_REST_ROTATE body->fskill[7]

bool Entity::disturbBat(Entity* touched, bool takenDamage, bool doMessage)
{
	if ( monsterSpecialState == BAT_REST )
	{
		monsterSpecialState = BAT_REST_DISTURBED;
		serverUpdateEntitySkill(this, 33);

		monsterHitTime = HITRATE;

		setEffect(EFF_STUNNED, true, 10, false);
		if ( bodyparts.size() >= 1 )
		{
			auto& body = bodyparts[0];
			if ( body->z < -15 )
			{
				setEffect(EFF_STUNNED, true, 30, false);
			}
			else if ( body->z < -10 )
			{
				setEffect(EFF_STUNNED, true, 20, false);
			}
		}

		if ( touched )
		{
			lookAtEntity(*touched);
			if ( !uidToEntity(monsterTarget) )
			{
				if ( checkEnemy(touched) )
				{
					monsterAcquireAttackTarget(*touched, MONSTER_STATE_PATH, true);
				}
				if ( doMessage )
				{
					if ( touched->behavior == &actPlayer )
					{
						messagePlayerColor(touched->skill[2], MESSAGE_INTERACTION,
							makeColorRGB(255, 0, 0), Language::get(6252));
					}
				}
			}
		}
		return true;
	}
	return false;
}

void batAnimate(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = nullptr;
	Entity* head = nullptr;
	int bodypart;

	my->flags[INVISIBLE] = true; // hide the "AI" bodypart
	//my->flags[PASSABLE] = true;

	my->sizex = 2;
	my->sizey = 2;

	my->focalx = limbs[BAT_SMALL][0][0];
	my->focaly = limbs[BAT_SMALL][0][1];
	my->focalz = limbs[BAT_SMALL][0][2];
	if ( multiplayer != CLIENT )
	{
		my->z = limbs[BAT_SMALL][5][2];
		if ( !myStats->EFFECTS[EFF_LEVITATING] )
		{
			myStats->EFFECTS[EFF_LEVITATING] = true;
			myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 0;
		}
		if ( !my->isMobile() )
		{
			my->monsterRotate();
		}
	}

	if ( enableDebugKeys && (svFlags & SV_FLAG_CHEATS) )
	{
		if ( keystatus[SDLK_KP_5] )
		{
			my->yaw += 0.05;
		}
		if ( keystatus[SDLK_g] )
		{
			keystatus[SDLK_g] = 0;
			MONSTER_ATTACK = MONSTER_POSE_MELEE_WINDUP1;
			MONSTER_ATTACKTIME = 0;
		}
		if ( keystatus[SDLK_h] )
		{
			keystatus[SDLK_h] = 0;
			myStats->EFFECTS[EFF_STUNNED] = !myStats->EFFECTS[EFF_STUNNED];
			myStats->EFFECTS_TIMERS[EFF_STUNNED] = myStats->EFFECTS[EFF_STUNNED] ? -1 : 0;
		}
		if ( keystatus[SDLK_j] )
		{
			keystatus[SDLK_j] = 0;
			my->monsterSpecialState = my->monsterSpecialState == 0 ? BAT_REST : 0;
			if ( my->monsterSpecialState == BAT_REST )
			{
				my->monsterReleaseAttackTarget();
			}
			serverUpdateEntitySkill(my, 33);
		}
	}

	//Move bodyparts
	Entity* leftWing = nullptr;
	Entity* body = nullptr;
	for ( bodypart = 0, node = my->children.first; node != nullptr; node = node->next, ++bodypart )
	{
		if ( bodypart < BAT_BODY )
		{
			continue;
		}

		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		entity->yaw = my->yaw;
		if ( body && bodypart != BAT_BODY )
		{
			entity->yaw += PI * sin(BAT_REST_ROTATE * PI / 2);
		}

		if ( bodypart == BAT_HEAD )
		{
			entity->fskill[0] = fmod(entity->fskill[0], 2 * PI);
			while ( entity->fskill[0] >= PI )
			{
				entity->fskill[0] -= 2 * PI;
			}
			while ( entity->fskill[0] < -PI )
			{
				entity->fskill[0] += 2 * PI;
			}
			if ( my->monsterSpecialState == BAT_REST && BAT_REST_STATE == 1 )
			{
				entity->fskill[0] = std::max(body->fskill[0], entity->fskill[0]);
				real_t speed = 0.2;
				if ( limbAngleWithinRange(entity->fskill[0], speed, 3 * PI / 4) )
				{
					entity->fskill[0] = 3 * PI / 4;
				}
				else
				{
					entity->fskill[0] += speed;
					entity->fskill[0] = std::min(entity->fskill[0], 3 * PI / 4);
				}
			}
			else if ( MONSTER_ATTACK == 0 )
			{
				real_t speed = -0.2;
				real_t setpoint = PI / 16;
				if ( entity->fskill[0] < (setpoint - 0.01) || limbAngleWithinRange(entity->fskill[0], speed, setpoint) )
				{
					entity->fskill[0] = setpoint;
				}
				else
				{
					entity->fskill[0] += speed;
					entity->fskill[0] = std::max(entity->fskill[0], setpoint);
				}
			}

			if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP1 || MONSTER_ATTACK == 1 )
			{
				entity->fskill[0] = PI / 16;
			}
		}
		else if ( bodypart == BAT_BODY )
		{
			body = entity;
			entity->fskill[0] = fmod(entity->fskill[0], 2 * PI);
			while ( entity->fskill[0] >= PI )
			{
				entity->fskill[0] -= 2 * PI;
			}
			while ( entity->fskill[0] < -PI )
			{
				entity->fskill[0] += 2 * PI;
			}
			if ( MONSTER_ATTACK == 0 )
			{
				BAT_FLOAT_ATK = 0.0;
			}

			if ( my->monsterSpecialState == BAT_REST && BAT_REST_STATE == 1 )
			{
				real_t speed = 0.2;
				if ( limbAngleWithinRange(entity->fskill[0], speed, PI / 2) )
				{
					entity->fskill[0] = PI / 2;
				}
				else
				{
					entity->fskill[0] += speed;
					entity->fskill[0] = std::min(entity->fskill[0], PI / 2);
				}
			}
			else if ( MONSTER_ATTACK == 0 )
			{
				real_t speed = -0.2;
				if ( entity->fskill[0] < -0.01 || limbAngleWithinRange(entity->fskill[0], speed, 0.0) )
				{
					entity->fskill[0] = 0.0;
				}
				else
				{
					entity->fskill[0] += speed;
					entity->fskill[0] = std::max(entity->fskill[0], 0.0);
				}
			}

			if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP1 || MONSTER_ATTACK == 1 )
			{
				if ( MONSTER_ATTACKTIME == 0 )
				{
					entity->fskill[0] = 0.0;
					entity->skill[1] = 0;
					BAT_FLOAT_ATK = 0.0;
				}
				else
				{
					if ( MONSTER_ATTACKTIME >= (int)limbs[BAT_SMALL][15][0] )
					{
						if ( MONSTER_ATTACKTIME == (int)limbs[BAT_SMALL][15][0] )
						{
							if ( multiplayer != CLIENT )
							{
								const Sint32 temp = MONSTER_ATTACKTIME;
								my->attack(1, 0, nullptr); // slop
								MONSTER_ATTACKTIME = temp;
							}
						}

						if ( entity->skill[1] == 0 )
						{
							real_t speed = limbs[BAT_SMALL][13][2];
							if ( limbAngleWithinRange(entity->fskill[0], -speed, -((PI / 2) + PI / 4)) )
							{
								entity->fskill[0] = -((PI / 2) + PI / 4);
								entity->skill[1] = 1;
							}
							else
							{
								entity->fskill[0] -= speed;
								entity->fskill[0] = std::max(entity->fskill[0], -((PI / 2) + PI / 4));
							}
						}
						else
						{
							real_t speed = limbs[BAT_SMALL][13][1];
							entity->fskill[0] += speed;
							entity->fskill[0] = std::min(entity->fskill[0], 0.0);
						}
					}
					else
					{
						real_t speed = limbs[BAT_SMALL][13][0];
						entity->fskill[0] -= speed;
						entity->fskill[0] = std::max(entity->fskill[0], -((PI / 2) + PI / 32));
					}

					if ( MONSTER_ATTACKTIME >= (int)limbs[BAT_SMALL][18][0] )
					{
						BAT_FLOAT_ATK -= limbs[BAT_SMALL][18][1];
						BAT_FLOAT_ATK = std::max(BAT_FLOAT_ATK, (real_t)limbs[BAT_SMALL][18][2]);
					}
					else if ( MONSTER_ATTACKTIME >= (int)limbs[BAT_SMALL][17][0] )
					{
						BAT_FLOAT_ATK += limbs[BAT_SMALL][17][1];
						BAT_FLOAT_ATK = std::min(BAT_FLOAT_ATK, (real_t)limbs[BAT_SMALL][17][2]);
					}
					else if ( MONSTER_ATTACKTIME >= (int)limbs[BAT_SMALL][16][0] )
					{
						BAT_FLOAT_ATK -= limbs[BAT_SMALL][16][1];
						BAT_FLOAT_ATK = std::max(BAT_FLOAT_ATK, (real_t)limbs[BAT_SMALL][16][2]);
					}
				}

				if ( MONSTER_ATTACKTIME >= (int)limbs[BAT_SMALL][15][1] )
				{
					MONSTER_ATTACK = 0;
				}
			}
		}
		else if ( bodypart == BAT_LEFTWING )
		{
			entity->fskill[1] = fmod(entity->fskill[1], 2 * PI);
			while ( entity->fskill[1] >= PI )
			{
				entity->fskill[1] -= 2 * PI;
			}
			while ( entity->fskill[1] < -PI )
			{
				entity->fskill[1] += 2 * PI;
			}
			entity->fskill[0] = fmod(entity->fskill[0], 2 * PI);
			while ( entity->fskill[0] >= PI )
			{
				entity->fskill[0] -= 2 * PI;
			}
			while ( entity->fskill[0] < -PI )
			{
				entity->fskill[0] += 2 * PI;
			}

			if ( my->monsterSpecialState == BAT_REST )
			{
				{
					real_t speed = -0.1;
					real_t setpoint = -5 * PI / 8;
					if ( limbAngleWithinRange(entity->fskill[1], speed, setpoint) )
					{
						entity->fskill[1] = setpoint;
					}
					else
					{
						entity->fskill[1] += speed;
						entity->fskill[1] = std::max(entity->fskill[1], setpoint);
					}
				}
				{
					real_t setpoint = 0.0;
					if ( entity->fskill[0] < 0.01 )
					{
						real_t speed = 0.1;
						if ( limbAngleWithinRange(entity->fskill[0], speed, setpoint) )
						{
							entity->fskill[0] = setpoint;
						}
						else
						{
							entity->fskill[0] += speed;
							entity->fskill[0] = std::min(entity->fskill[0], setpoint);
						}
					}
					else if ( entity->fskill[0] > 0.01 )
					{
						real_t speed = -0.1;
						if ( limbAngleWithinRange(entity->fskill[0], speed, setpoint) )
						{
							entity->fskill[0] = setpoint;
						}
						else
						{
							entity->fskill[0] += speed;
							entity->fskill[0] = std::max(entity->fskill[0], setpoint);
						}
					}
					else
					{
						entity->fskill[0] = setpoint;
					}
				}
			}

			if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP1 || MONSTER_ATTACK == 1 )
			{
				if ( MONSTER_ATTACKTIME == 0 )
				{
					entity->fskill[1] = -PI / 2;
					entity->skill[1] = 0;
				}


				if ( entity->skill[1] == 0 )
				{
					real_t speed = limbs[BAT_SMALL][14][0];
					if ( limbAngleWithinRange(entity->fskill[1], speed, 0.0) )
					{
						entity->fskill[1] = 0.0;
						entity->skill[1] = 1;
					}
					else
					{
						if ( entity->fskill[1] > 0.01 )
						{
							entity->fskill[1] -= speed;
						}
						else if ( entity->fskill[1] < -0.01 )
						{
							entity->fskill[1] += speed;
						}
					}
					entity->fskill[0] = body->fskill[0];
				}
				else if ( entity->skill[1] == 1 )
				{
					real_t speed = limbs[BAT_SMALL][14][0];
					if ( limbAngleWithinRange(entity->fskill[1], speed, PI / 4) )
					{
						entity->fskill[1] = PI / 4;
						entity->skill[1] = 2;
					}
					else
					{
						entity->fskill[1] += speed;
					}
					entity->fskill[0] = body->fskill[0];
				}
				else if ( entity->skill[1] == 2 )
				{
					real_t speed = limbs[BAT_SMALL][14][1];
					real_t setpoint = -PI / 2 - PI / 8;
					if ( limbAngleWithinRange(entity->fskill[1], -speed, setpoint) )
					{
						entity->fskill[1] = setpoint;
						entity->skill[1] = 3;
					}
					else
					{
						entity->fskill[1] -= speed;
					}
					entity->fskill[0] = body->fskill[0];
				}
				else if ( entity->skill[1] == 3 )
				{
					real_t speed = limbs[BAT_SMALL][14][2];
					if ( limbAngleWithinRange(entity->fskill[1], speed, 0.0) )
					{
						entity->fskill[1] = 0.0;
						entity->skill[1] = 4;

						entity->skill[0] = 0; // reset flap to raise
					}
					else
					{
						entity->fskill[1] += speed;
					}
					if ( limbAngleWithinRange(entity->fskill[0], speed * 2, 0.0) )
					{
						entity->fskill[0] = 0.0;
					}
					else
					{
						entity->fskill[0] += speed * 2;
					}
				}
			}
		}

		switch ( bodypart )
		{
			case BAT_BODY:
			{
				if ( my->monsterSpecialState == BAT_REST && BAT_REST_STATE == 1 )
				{
					real_t diff = std::max(0.002, (1.0 - BAT_REST_ROTATE) / 20.0);
					BAT_REST_ROTATE += diff;
					BAT_REST_ROTATE = std::min(1.0, BAT_REST_ROTATE);
				}
				else
				{
					real_t diff = std::max(0.05, BAT_REST_ROTATE / 20.0);
					BAT_REST_ROTATE -= diff;
					BAT_REST_ROTATE = std::max(0.0, BAT_REST_ROTATE);
				}
				entity->yaw += PI * sin(BAT_REST_ROTATE * PI / 2);

				entity->x += limbs[BAT_SMALL][6][0] * cos(entity->yaw);
				entity->y += limbs[BAT_SMALL][6][1] * sin(entity->yaw);
				entity->z += limbs[BAT_SMALL][6][2];
				entity->focalx = limbs[BAT_SMALL][1][0];
				entity->focaly = limbs[BAT_SMALL][1][1];
				entity->focalz = limbs[BAT_SMALL][1][2];

				if ( enableDebugKeys && (svFlags & SV_FLAG_CHEATS) )
				{
					if ( keystatus[SDLK_KP_6] )
					{
						entity->fskill[0] -= 0.05;
					}
					else if ( keystatus[SDLK_KP_4] )
					{
						entity->fskill[0] += 0.05;
					}
				}
				entity->pitch = entity->fskill[0];

				if ( enableDebugKeys && (svFlags & SV_FLAG_CHEATS) )
				{
					if ( keystatus[SDLK_KP_PLUS] )
					{
						keystatus[SDLK_KP_PLUS] = 0;
						entity->skill[0] = entity->skill[0] == 0 ? 1 : 0;
					}
				}
				if ( entity->skill[0] == 0 )
				{
					entity->fskill[1] += 0.1;
				}

				if ( my->monsterSpecialState == BAT_REST )
				{
					BAT_FLOAT_X = 0.0;
					BAT_FLOAT_Y = 0.0;
					BAT_FLOAT_Z = 0.0;

					int mapx = static_cast<int>(my->x) >> 4;
					int mapy = static_cast<int>(my->y) >> 4;
					if ( mapx >= 0 && mapx < map.width && mapy >= 0 && mapy < map.height )
					{
						int mapIndex = (mapy)*MAPLAYERS + (mapx) * MAPLAYERS * map.height;
						if ( !map.tiles[(MAPLAYERS - 1) + mapIndex] )
						{
							// no ceiling
							if ( BAT_REST_FLY_Z <= -19.0 )
							{
								BAT_REST_FLY_Z = -19.0;
								BAT_REST_STATE = 1;
							}
							else
							{
								real_t diff = std::max(0.05, (BAT_REST_FLY_Z - -19.0) / 25.0);
								BAT_REST_FLY_Z -= diff;
							}
						}
						else
						{
							if ( BAT_REST_FLY_Z <= -3.0 )
							{
								BAT_REST_FLY_Z = -3.0;
								BAT_REST_STATE = 1;
							}
							else
							{
								real_t diff = std::max(0.05, (BAT_REST_FLY_Z - -3.0) / 25.0);
								BAT_REST_FLY_Z -= diff;
							}
						}
					}
				}
				else
				{
					BAT_REST_STATE = 0;

					if ( multiplayer != CLIENT )
					{
						if ( my->monsterSpecialState == BAT_REST_DISTURBED )
						{
							my->monsterSpecialState = 0;
						}
					}

					if ( BAT_REST_FLY_Z >= 0.0 )
					{
						BAT_REST_FLY_Z = 0.0;
					}
					else
					{
						real_t diff = std::max(0.1, (-BAT_REST_FLY_Z) / 25.0);
						BAT_REST_FLY_Z += diff;
					}

					BAT_FLOAT_X = limbs[BAT_SMALL][10][0] * sin(body->fskill[1] * limbs[BAT_SMALL][11][0]) * cos(entity->yaw + PI / 2);
					BAT_FLOAT_Y = limbs[BAT_SMALL][10][1] * sin(body->fskill[1] * limbs[BAT_SMALL][11][1]) * sin(entity->yaw + PI / 2);
					BAT_FLOAT_Z = limbs[BAT_SMALL][10][2] * sin(body->fskill[1] * limbs[BAT_SMALL][11][2]);
					real_t floatAtkZ = BAT_FLOAT_ATK < 0 ? 2 * sin(BAT_FLOAT_ATK * PI / 8) : 0.5 * sin(BAT_FLOAT_ATK * PI / 8);
					BAT_FLOAT_Z += floatAtkZ;
				}

				BAT_FLOAT_X += BAT_FLOAT_ATK * cos(entity->yaw);
				BAT_FLOAT_Y += BAT_FLOAT_ATK * sin(entity->yaw);


				entity->x += BAT_FLOAT_X;
				entity->y += BAT_FLOAT_Y;
				entity->z += BAT_FLOAT_Z;
				entity->z += BAT_REST_FLY_Z;
				break;
			}
			case BAT_HEAD:
				entity->x += limbs[BAT_SMALL][7][0] * cos(entity->yaw);
				entity->y += limbs[BAT_SMALL][7][1] * sin(entity->yaw);
				entity->z += limbs[BAT_SMALL][7][2];
				entity->focalx = limbs[BAT_SMALL][2][0];
				entity->focaly = limbs[BAT_SMALL][2][1];
				entity->focalz = limbs[BAT_SMALL][2][2];
				if ( enableDebugKeys && (svFlags & SV_FLAG_CHEATS) )
				{
					if ( keystatus[SDLK_KP_7] )
					{
						entity->fskill[0] -= 0.05;
					}
					else if ( keystatus[SDLK_KP_9] )
					{
						entity->fskill[0] += 0.05;
					}
				}
				entity->pitch = entity->fskill[0];

				if ( body )
				{
					entity->x += BAT_FLOAT_X;
					entity->y += BAT_FLOAT_Y;
					entity->z += BAT_FLOAT_Z;
					entity->z += BAT_REST_FLY_Z;
				}
				break;
			case BAT_LEFTWING:
				entity->x += limbs[BAT_SMALL][8][0] * cos(entity->yaw + PI / 2);
				entity->y += limbs[BAT_SMALL][8][1] * sin(entity->yaw + PI / 2);
				entity->z += limbs[BAT_SMALL][8][2];
				entity->focalx = limbs[BAT_SMALL][3][0];
				entity->focaly = limbs[BAT_SMALL][3][1];
				entity->focalz = limbs[BAT_SMALL][3][2];

				if ( enableDebugKeys && (svFlags & SV_FLAG_CHEATS) )
				{
					if ( keystatus[SDLK_KP_1] )
					{
						entity->fskill[0] -= 0.05;
					}
					else if ( keystatus[SDLK_KP_3] )
					{
						entity->fskill[0] += 0.05;
					}
					if ( keystatus[SDLK_KP_0] )
					{
						entity->fskill[1] -= 0.05;
					}
					else if ( keystatus[SDLK_KP_ENTER] )
					{
						entity->fskill[1] += 0.05;
					}

					if ( keystatus[SDLK_KP_MINUS] )
					{
						keystatus[SDLK_KP_MINUS] = 0;
						entity->skill[3] = entity->skill[3] == 0 ? 1 : 0;
					}
				}

				if ( (entity->skill[3] == 0) && MONSTER_ATTACK == 0 && !(my->monsterSpecialState == BAT_REST && BAT_REST_STATE == 1) )
				{
					if ( entity->fskill[1] >= 0.8 )
					{
						entity->skill[0] = 1;
					}
					if ( entity->fskill[1] <= -1.3 )
					{
						entity->skill[0] = 0;
					}
					if ( entity->skill[0] == 0 )
					{
						entity->fskill[1] += 0.35;
					}
					else if ( entity->skill[0] == 1)
					{
						entity->fskill[1] -= 0.35;
					}
				}
				entity->pitch = entity->fskill[0];
				entity->roll = entity->fskill[1];
				leftWing = entity;

				if ( body )
				{
					entity->x += BAT_FLOAT_X;
					entity->y += BAT_FLOAT_Y;
					entity->z += BAT_FLOAT_Z;
					entity->z += BAT_REST_FLY_Z;

					entity->x += limbs[BAT_SMALL][12][0] * sin(body->pitch) * cos(entity->yaw);
					entity->y += limbs[BAT_SMALL][12][1] * sin(body->pitch) * sin(entity->yaw);
					entity->z += limbs[BAT_SMALL][12][2] * sin(body->pitch);
				}
				break;
			case BAT_RIGHTWING:
				entity->x += limbs[BAT_SMALL][9][0] * cos(entity->yaw + PI / 2);
				entity->y += limbs[BAT_SMALL][9][1] * sin(entity->yaw + PI / 2);
				entity->z += limbs[BAT_SMALL][9][2];
				entity->focalx = limbs[BAT_SMALL][4][0];
				entity->focaly = limbs[BAT_SMALL][4][1];
				entity->focalz = limbs[BAT_SMALL][4][2];
				if ( leftWing )
				{
					entity->fskill[0] = leftWing->fskill[0];
					entity->fskill[1] = -leftWing->fskill[1];
				}
				entity->pitch = entity->fskill[0];
				entity->roll = entity->fskill[1];

				if ( body )
				{
					entity->x += BAT_FLOAT_X;
					entity->y += BAT_FLOAT_Y;
					entity->z += BAT_FLOAT_Z;
					entity->z += BAT_REST_FLY_Z;

					entity->x += limbs[BAT_SMALL][12][0] * sin(body->pitch) * cos(entity->yaw);
					entity->y += limbs[BAT_SMALL][12][1] * sin(body->pitch) * sin(entity->yaw);
					entity->z += limbs[BAT_SMALL][12][2] * sin(body->pitch);

				}
				break;
			default:
				break;
		}
	}

	if ( MONSTER_ATTACK > 0 )
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