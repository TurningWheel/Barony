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
#include "paths.hpp"

void initDuck(Entity* my, Stat* myStats)
{
	node_t* node;

	bool spiritDuck = my && my->behavior == &actDeathGhostLimb;

	my->z = 0;

	int sprite = 2225;
	int appearance = 0;
	if ( myStats && myStats->getAttribute("duck_type") != "" )
	{
		int duckType = std::stoi(myStats->getAttribute("duck_type"));
		if ( duckType >= MAXPLAYERS && duckType < 2 * MAXPLAYERS )
		{
			sprite = 2231;
			appearance = 1;
		}
		else if ( duckType >= 2 * MAXPLAYERS && duckType < 3 * MAXPLAYERS )
		{
			sprite = 2237;
			appearance = 2;
		}
		else if ( duckType >= 3 * MAXPLAYERS && duckType < 4 * MAXPLAYERS )
		{
			sprite = 2307;
			appearance = 3;
		}
	}

	if ( !spiritDuck )
	{
		my->initMonster(sprite);
		my->flags[INVISIBLE] = true; // hide the "AI" bodypart
	}
	if ( multiplayer != CLIENT && myStats )
	{
		MONSTER_SPOTSND = -1;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = 789;
		MONSTER_IDLEVAR = 5;
	}

	if ( multiplayer != CLIENT && !MONSTER_INIT && myStats )
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

			my->setHardcoreStats(*myStats);
		}
	}

	// body
	Entity* entity = newEntity(appearance == 3 ? 2308 : 2226 + appearance * 6, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->z = 6;
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[DUCK_SMALL][1][0];
	entity->focaly = limbs[DUCK_SMALL][1][1];
	entity->focalz = limbs[DUCK_SMALL][1][2];
	entity->behavior = &actDuckLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	if ( spiritDuck )
	{
		entity->skill[2] = my->skill[2];
		entity->behavior = my->behavior;
		entity->flags[GENIUS] = true;
	}
	my->bodyparts.push_back(entity);

	// body sit
	entity = newEntity(appearance == 3 ? 2307 : 2225 + appearance * 6, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[DUCK_SMALL][2][0];
	entity->focaly = limbs[DUCK_SMALL][2][1];
	entity->focalz = limbs[DUCK_SMALL][2][2];
	entity->behavior = &actDuckLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	if ( spiritDuck )
	{
		entity->skill[2] = my->skill[2];
		entity->behavior = my->behavior;
		entity->flags[GENIUS] = true;
	}
	my->bodyparts.push_back(entity);

	// wingleft
	entity = newEntity(appearance == 3 ? 2309 : 2227 + appearance * 6, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[DUCK_SMALL][3][0];
	entity->focaly = limbs[DUCK_SMALL][3][1];
	entity->focalz = limbs[DUCK_SMALL][3][2];
	entity->behavior = &actDuckLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	if ( spiritDuck )
	{
		entity->skill[2] = my->skill[2];
		entity->behavior = my->behavior;
		entity->flags[GENIUS] = true;
	}
	my->bodyparts.push_back(entity);

	// wingright
	entity = newEntity(appearance == 3 ? 2310 : 2228 + appearance * 6, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[DUCK_SMALL][4][0];
	entity->focaly = limbs[DUCK_SMALL][4][1];
	entity->focalz = limbs[DUCK_SMALL][4][2];
	entity->behavior = &actDuckLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	if ( spiritDuck )
	{
		entity->skill[2] = my->skill[2];
		entity->behavior = my->behavior;
		entity->flags[GENIUS] = true;
	}
	my->bodyparts.push_back(entity);

	// leg left
	entity = newEntity(2229, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[DUCK_SMALL][20][0];
	entity->focaly = limbs[DUCK_SMALL][20][1];
	entity->focalz = limbs[DUCK_SMALL][20][2];
	entity->behavior = &actDuckLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	if ( spiritDuck )
	{
		entity->skill[2] = my->skill[2];
		entity->behavior = my->behavior;
		entity->flags[GENIUS] = true;
	}
	my->bodyparts.push_back(entity);

	// leg right
	entity = newEntity(2230, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[DUCK_SMALL][22][0];
	entity->focaly = limbs[DUCK_SMALL][22][1];
	entity->focalz = limbs[DUCK_SMALL][22][2];
	entity->behavior = &actDuckLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	if ( spiritDuck )
	{
		entity->skill[2] = my->skill[2];
		entity->behavior = my->behavior;
		entity->flags[GENIUS] = true;
	}
	my->bodyparts.push_back(entity);
}

void actDuckLimb(Entity* my)
{
	my->actMonsterLimb(false);
}

void duckSpawnFeather(int sprite, real_t x, real_t y, real_t z, Entity* my)
{
	int featherSprite = 2249;
	if ( sprite == 2225 || sprite == 2226 )
	{
		featherSprite = 2249;
	}
	else if ( sprite == 2231 || sprite == 2232 )
	{
		featherSprite = 2250;
	}
	else if ( sprite == 2237 || sprite == 2238 )
	{
		featherSprite = 2251;
	}
	else if ( sprite == 2307 || sprite == 2308 )
	{
		featherSprite = 2314;
	}
	real_t yawOffset = ((local_rng.rand() % 8) / 4.0) * PI;
	for ( int i = 0; i < 3; ++i )
	{
		real_t leafEndZ = z - 7.5;
		Entity* leaf = newEntity(featherSprite, 1, map.entities, nullptr); //Gib entity.
		if ( leaf != NULL )
		{
			leaf->x = x;
			leaf->y = y;
			leaf->z = z - i * 0.5;
			leaf->fskill[6] = leaf->z;
			leaf->fskill[7] = leafEndZ - leaf->fskill[6];
			leaf->vel_z = 0.0;
			leaf->yaw = yawOffset + i * 2 * PI / 3;
			leaf->sizex = 2;
			leaf->sizey = 2;
			leaf->scalex = 1.0;
			leaf->scaley = 1.0;
			leaf->scalez = 1.0;
			leaf->fskill[4] = x;
			leaf->fskill[5] = y;
			leaf->fskill[9] = i * 2 * PI / 3;
			leaf->parent = 0;
			leaf->skill[0] = TICKS_PER_SECOND * 15;
			leaf->behavior = &actLeafParticle;
			leaf->flags[NOCLIP_CREATURES] = true;
			leaf->flags[UPDATENEEDED] = false;
			leaf->flags[NOUPDATE] = true;
			leaf->flags[PASSABLE] = true;
			leaf->flags[UNCLICKABLE] = true;
			if ( multiplayer != CLIENT )
			{
				--entity_uids;
			}
			leaf->setUID(-3);
			if ( my )
			{
				leaf->ditheringOverride = my->ditheringOverride;
				leaf->mistformGLRender = my->mistformGLRender;
			}
		}
	}
}

void duckDie(Entity* my)
{
	//int c;
	//for ( c = 0; c < 4; c++ )
	//{
	//	Entity* entity = spawnGib(my);
	//	if ( entity )
	//	{
	//		entity->skill[5] = 1; // poof

	//		switch ( c )
	//		{
	//		case 0:
	//			entity->sprite = 1408;
	//			break;
	//		case 1:
	//			entity->sprite = 1409;
	//			break;
	//		case 2:
	//			entity->sprite = 1410;
	//			break;
	//		case 3:
	//			entity->sprite = 1411;
	//			break;
	//		default:
	//			break;
	//		}

	//		serverSpawnGibForClient(entity);
	//	}
	//}

	//my->spawnBlood();

	//playSoundEntity(my, 670 + local_rng.rand() % 2, 128);

	spawnPoof(my->x, my->y, 7.5, 0.5, true);
	duckSpawnFeather(my->sprite, my->x, my->y, my->z, my);
	serverSpawnMiscParticlesAtLocation(my->x, my->y, my->z, PARTICLE_EFFECT_DUCK_SPAWN_FEATHER, my->sprite);

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define DUCK_BODY 2
#define DUCK_HEAD 3
#define DUCK_LEFTWING 4
#define DUCK_RIGHTWING 5
#define DUCK_LEFTLEG 6
#define DUCK_RIGHTLEG 7

#define DUCK_FLOAT_X body->fskill[2]
#define DUCK_FLOAT_Y body->fskill[3]
#define DUCK_FLOAT_Z body->fskill[4]
#define DUCK_FLOAT_ATK body->fskill[5]
#define DUCK_LAST_SPECIAL_STATE body->skill[4]
#define DUCK_SPECIAL_TIMER body->skill[5]
#define DUCK_INWATER body->skill[6]
#define DUCK_INIT body->skill[7]
#define DUCK_FLOAT_ATK_DIVE body->fskill[8]
#define DUCK_FLOAT_Z_MULT body->fskill[9]
#define DUCK_DIVE_ANIM body->fskill[10]
#define DUCK_INERT_ANIM body->fskill[11]
#define DUCK_INERT_ANIM_COMPLETE body->fskill[12]
#define DUCK_WALK_CYCLE body->fskill[13]
#define DUCK_WALK_CYCLE_ANIM body->fskill[14]
#define DUCK_WALK_CYCLE_ANIM2 body->fskill[15]
#define DUCK_WALK_CYCLE2 body->fskill[16]
#define DUCK_BOB_WATER body->fskill[17]
#define DUCK_CAM_Z body->fskill[18]

void actWaterSplash(Entity* my)
{
	if ( my->skill[0] <= 0 )
	{
		list_RemoveNode(my->mynode);
		return;
	}
	--my->skill[0];
	if ( my->sprite == 2246 )
	{
		my->z = std::max(8.05, my->z - 0.1);
	}
	if ( my->sprite == 2247 && my->ticks > 20 )
	{
		my->z += 0.1;
	}
	if ( my->ticks % 10 == 0 )
	{
		if ( my->sprite == 2246 )
		{
			my->sprite = 2247;
			my->z += 0.2;
		}
	}
}

void actWaterSplashParticle(Entity* my)
{
	if ( my->skill[0] <= 0 )
	{
		list_RemoveNode(my->mynode);
		return;
	}
	--my->skill[0];
	my->x += my->vel_x;
	my->y += my->vel_y;
	my->z += my->vel_z;
	my->vel_z += 0.04;
}

void createWaterSplash(real_t x, real_t y, int lifetime)
{
	{
		Entity* splash = newEntity(2246, 1, map.entities, nullptr); //Gib entity.
		real_t centerx = static_cast<int>(x / 16) * 16.0 + 8.0;
		real_t centery = static_cast<int>(y / 16) * 16.0 + 8.0;
		splash->x = std::max(-2.5, std::min(2.5, (x - centerx))) + centerx;
		splash->y = std::max(-2.5, std::min(2.5, (y - centery))) + centery;
		splash->z = 8.75;
		splash->yaw = 0.0;
		splash->skill[0] = lifetime;
		splash->behavior = &actWaterSplash;
		splash->flags[UPDATENEEDED] = false;
		splash->flags[NOUPDATE] = true;
		splash->flags[PASSABLE] = true;
		splash->flags[UNCLICKABLE] = true;
		if ( multiplayer != CLIENT )
		{
			--entity_uids;
		}
		splash->setUID(-3);
	}

	real_t offsetYaw = ((local_rng.rand() % 9) / 8.0) * PI / 4;
	for ( int i = 0; i < 4; ++i )
	{
		Entity* splashParticle = newEntity(2248, 1, map.entities, nullptr); //Gib entity.
		splashParticle->yaw = offsetYaw + i * (PI / 2);
		splashParticle->x = static_cast<int>(x / 16) * 16.0 + 8.0 + 2.0 * cos(splashParticle->yaw);
		splashParticle->y = static_cast<int>(y / 16) * 16.0 + 8.0 + 2.0 * sin(splashParticle->yaw);
		splashParticle->z = 7.5;
		splashParticle->pitch = (local_rng.rand() % 360) * PI / 180.0;
		splashParticle->roll = (local_rng.rand() % 360) * PI / 180.0;
		splashParticle->vel_x = 0.25 * cos(splashParticle->yaw);
		splashParticle->vel_y = 0.25 * sin(splashParticle->yaw);
		splashParticle->vel_z = -0.5 + 0.4 * (local_rng.rand() % 11) / 10.0;
		splashParticle->skill[0] = 50;
		splashParticle->behavior = &actWaterSplashParticle;
		splashParticle->flags[UPDATENEEDED] = false;
		splashParticle->flags[NOUPDATE] = true;
		splashParticle->flags[PASSABLE] = true;
		splashParticle->flags[UNCLICKABLE] = true;
		if ( multiplayer != CLIENT )
		{
			--entity_uids;
		}
		splashParticle->setUID(-3);
	}
}

bool duckAreaQuck(Entity* my)
{
	if ( !my ) { return false; }

	Entity* caster = my;
	if ( my->behavior == &actDeathGhost )
	{
		caster = nullptr;
		if ( my->skill[2] >= 0 && my->skill[2] < MAXPLAYERS )
		{
			if ( players[my->skill[2]]->entity )
			{
				caster = players[my->skill[2]]->entity;
			}
		}
		if ( !caster )
		{
			return false;
		}
	}
	else if ( my->behavior != &actMonster )
	{
		return false;
	}

	bool anyTarget = false;
	for ( auto node = map.creatures->first; node; node = node->next )
	{
		if ( Entity* target = (Entity*)node->element )
		{
			if ( target->monsterIsTargetable() && entityDist(target, my) < 2 * TOUCHRANGE )
			{
				if ( caster->checkEnemy(target) || (my->behavior == &actMonster && target->getUID() == my->monsterTarget) )
				{
					//if ( Entity* target = uidToEntity(monsterTarget) )
					{
						if ( Stat* targetStats = target->getStats() )
						{
							if ( !targetStats->getEffectActive(EFF_DISORIENTED)
								&& !targetStats->getEffectActive(EFF_DISTRACTED_COOLDOWN)
								&& target->behavior == &actMonster && target->isMobile()
								&& !monsterIsImmobileTurret(target, targetStats)
								&& !target->isBossMonster() && targetStats && !uidToEntity(targetStats->leader_uid) )
							{
								//if ( /*(entity->monsterState == MONSTER_STATE_WAIT || entity->monsterTarget == 0) || */
								//	(entityDist(target, this) < 2 * TOUCHRANGE /*&& (Uint32)(target->monsterLastDistractedByNoisemaker) != this->getUID()*/) )
								{
									real_t tangent = atan2(target->y - my->y, target->x - my->x);
									lineTraceTarget(my, my->x, my->y, tangent, 32.0, 0, false, target);
									if ( hit.entity == target )
									{
										if ( target->monsterSetPathToLocation(my->x / 16, my->y / 16, 2,
											GeneratePathTypes::GENERATE_PATH_DEFAULT) && target->children.first )
										{
											target->monsterLastDistractedByNoisemaker = my->getUID();
											target->monsterTarget = my->getUID();
											target->monsterState = MONSTER_STATE_HUNT; // hunt state
											serverUpdateEntitySkill(target, 0);


											if ( my->behavior == &actDeathGhost )
											{
												if ( target->setEffect(EFF_DISORIENTED, true, 2 * TICKS_PER_SECOND, false) )
												{
													anyTarget = true;
													target->setEffect(EFF_DISTRACTED_COOLDOWN, true, TICKS_PER_SECOND * 2, false);
													spawnFloatingSpriteMisc(134, target->x + (-4 + local_rng.rand() % 9) + cos(target->yaw) * 2,
														target->y + (-4 + local_rng.rand() % 9) + sin(target->yaw) * 2, target->z + local_rng.rand() % 4);
												}
											}
											else
											{
												if ( target->setEffect(EFF_DISORIENTED, true, 2 * TICKS_PER_SECOND, false) )
												{
													anyTarget = true;
													target->setEffect(EFF_DISTRACTED_COOLDOWN, true, TICKS_PER_SECOND * 2, false);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	if ( anyTarget || my->behavior == &actDeathGhost )
	{
		playSoundEntity(my, 784 + local_rng.rand() % 2, 128);
		spawnDamageGib(my, 198, DamageGib::DMG_STRONGEST, DamageGibDisplayType::DMG_GIB_SPRITE, true);
	}
}

void duckAnimate(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = nullptr;
	int bodypart;

	my->flags[INVISIBLE] = true; // hide the "AI" bodypart
	my->flags[PASSABLE] = true;
	my->flags[NOCLIP_CREATURES] = true;

	my->sizex = 4;
	my->sizey = 4;

	my->focalx = limbs[DUCK_SMALL][0][0];
	my->focaly = limbs[DUCK_SMALL][0][1];
	my->focalz = limbs[DUCK_SMALL][0][2];

	bool spiritDuck = my->behavior == &actDeathGhostLimb;
	if ( spiritDuck )
	{
		my->z = limbs[DUCK_SMALL][5][2];
	}
	if ( multiplayer != CLIENT )
	{
		my->z = limbs[DUCK_SMALL][5][2];
		if ( myStats && !myStats->getEffectActive(EFF_LEVITATING) )
		{
			myStats->setEffectActive(EFF_LEVITATING, 1);
			myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 0;
		}

		if ( myStats )
		{
			if ( !my->isMobile() )
			{
				my->monsterRotate();
			}

			my->creatureHandleLiftZ();

			if ( myStats->getAttribute("duck_time") != "" )
			{
				int lifetime = std::stoi(myStats->getAttribute("duck_time"));
				bool ghostActive = false;
				if ( Entity* leader = uidToEntity(myStats->leader_uid) )
				{
					if ( leader->behavior == &actPlayer && players[leader->skill[2]]->ghost.isActive() )
					{
						ghostActive = true;
					}
				}
				if ( my->monsterSpecialState != DUCK_DIVE && !ghostActive )
				{
					--lifetime;
					if ( !uidToEntity(myStats->leader_uid) )
					{
						--lifetime;
					}
				}
				lifetime = std::max(0, lifetime);
				myStats->setAttribute("duck_time", std::to_string(lifetime));
				if ( lifetime <= 0 )
				{
					if ( my->monsterSpecialState != DUCK_RETURN )
					{
						my->monsterSpecialState = DUCK_RETURN;
						serverUpdateEntitySkill(my, 33);
						myStats->setEffectActive(EFF_STUNNED, 1);
						myStats->EFFECTS_TIMERS[EFF_STUNNED] = 0;
						my->flags[NOCLIP_WALLS] = true;
						playSoundEntity(my, 794 + local_rng.rand() % 2, 128);
						//playSoundEntity(my, 786 + local_rng.rand() % 3, 128);
					}
				}
			}
		}
	}

	if ( enableDebugKeys && (svFlags & SV_FLAG_CHEATS) && myStats )
	{
		if ( keystatus[SDLK_KP_5] )
		{
			keystatus[SDLK_KP_5] = 0;
			static real_t dir = 0.0;
			my->yaw = dir;
			my->monsterLookDir = my->yaw;
			if ( keystatus[SDLK_LSHIFT] )
			{
				dir += PI / 2;
			}
			else
			{
				dir += 0.1;
			}

			createWaterSplash(my->x, my->y, 30);
			playSoundEntityLocal(my, 136, 64);
		}
		if ( keystatus[SDLK_g] )
		{
			keystatus[SDLK_g] = 0;
			if ( my->monsterSpecialState == 0 )
			{
				if ( keystatus[SDLK_LSHIFT] )
				{
					my->monsterSpecialState = DUCK_DIVE;
				}
				else
				{
					my->monsterSpecialState = DUCK_INERT;
				}
			}
			else if ( my->monsterSpecialState == 2 )
			{
				if ( keystatus[SDLK_LSHIFT] )
				{
					my->monsterSpecialState = 0;
				}
				else
				{
					my->monsterSpecialState = DUCK_DIVE;
				}
			}
			else
			{
				if ( keystatus[SDLK_LSHIFT] )
				{
					my->monsterSpecialState = 0;
				}
				else
				{
					my->monsterSpecialState = DUCK_INERT;
				}
			}
			//my->monsterSpecialState = 1;
		}
		if ( keystatus[SDLK_h] )
		{
			keystatus[SDLK_h] = 0;
			myStats->setEffectValueUnsafe(EFF_STUNNED, myStats->getEffectActive(EFF_STUNNED) ? 0 : 1);
			myStats->EFFECTS_TIMERS[EFF_STUNNED] = myStats->getEffectActive(EFF_STUNNED) ? -1 : 0;
		}
		if ( keystatus[SDLK_j] )
		{
			keystatus[SDLK_j] = 0;
			//MONSTER_ATTACK = local_rng.rand() % 2 ? MONSTER_POSE_MELEE_WINDUP2 : MONSTER_POSE_MELEE_WINDUP3;
			//MONSTER_ATTACKTIME = 0;
			my->attack(local_rng.rand() % 2 ? MONSTER_POSE_MELEE_WINDUP2 : MONSTER_POSE_MELEE_WINDUP3, 0, nullptr);
		}
	}

	Entity* body = nullptr;
	Entity* head = nullptr;
	for ( bodypart = 0, node = my->children.first; node != nullptr; node = node->next, ++bodypart )
	{
		if ( bodypart < DUCK_BODY )
		{
			continue;
		}
		entity = (Entity*)node->element;
		if ( bodypart == DUCK_HEAD )
		{
			head = entity;
		}
		else if ( bodypart == DUCK_BODY )
		{
			body = entity;
		}
	}

	bool inWater = false;
	bool waterTile = false;
	bool noFloor = false;
	bool lavaTile = false;
	bool safeTile = false;
	real_t inertHeight = 7.5;
	int mapx = my->x / 16;
	int mapy = my->y / 16;
	if ( body )
	{
		if ( mapx >= 0 && mapx < map.width && mapy >= 0 && mapy < map.height )
		{
			int mapIndex = (mapy)*MAPLAYERS + (mapx)*MAPLAYERS * map.height;
			if ( !map.tiles[mapIndex] )
			{
				noFloor = true;
			}
			if ( map.tiles[mapIndex] && lavatiles[map.tiles[mapIndex]] )
			{
				lavaTile = true;
			}
			if ( !map.tiles[OBSTACLELAYER + mapIndex] && map.tiles[mapIndex]
				&& swimmingtiles[map.tiles[mapIndex]] )
			{
				waterTile = true;
				inertHeight += 0.6;
			}
			if ( (!lavaTile && !noFloor) || swimmingtiles[map.tiles[mapIndex]] )
			{
				safeTile = true;
			}
		}
	}

	if ( multiplayer != CLIENT )
	{
		if ( !waterTile && my->monsterSpecialState == DUCK_DIVE )
		{
			my->monsterSpecialState = 0;
			serverUpdateEntitySkill(my, 33);
		}
		if ( (lavaTile || noFloor) && !waterTile )
		{
			if ( my->monsterSpecialState == DUCK_DIVE || my->monsterSpecialState == DUCK_INERT )
			{
				my->monsterSpecialState = 0;
				serverUpdateEntitySkill(my, 33);
			}
		}

		if ( spiritDuck && my->monsterSpecialState == DUCK_INERT )
		{
			if ( my->skill[2] >= 0 && my->skill[2] < MAXPLAYERS )
			{
				if ( players[my->skill[2]]->ghost.isActive() )
				{
					if ( players[my->skill[2]]->ghost.my && players[my->skill[2]]->ghost.my->skill[11] == 1 ) // high profile
					{
						my->monsterSpecialState = 0;
						serverUpdateEntitySkill(my, 33);
					}
				}
			}
		}

		if ( safeTile && body && my->isMobile() )
		{
			if ( my->monsterSpecialTimer == 0 )
			{
				my->monsterSpecialTimer = spiritDuck ? 0 : 2 * TICKS_PER_SECOND;
				if ( my->monsterSpecialState == DUCK_INERT && waterTile && !spiritDuck )
				{
					if ( DUCK_INERT_ANIM_COMPLETE >= 0.95 )
					{
						if ( my->monsterTarget == 0 || !uidToEntity(my->monsterTarget) )
						{
							my->monsterSpecialState = DUCK_DIVE;
							serverUpdateEntitySkill(my, 33);
							playSoundEntity(my, 794 + local_rng.rand() % 2, 128);
							playSoundEntity(my, 786 + local_rng.rand() % 3, 128);
						}
					}
				}
				if ( my->monsterSpecialState == 0 )
				{
					if ( spiritDuck )
					{
						if ( my->skill[2] >= 0 && my->skill[2] < MAXPLAYERS )
						{
							if ( players[my->skill[2]]->ghost.isActive() )
							{
								if ( players[my->skill[2]]->ghost.my && players[my->skill[2]]->ghost.my->skill[11] == 0 ) // low profile
								{
									my->monsterSpecialState = DUCK_INERT;
									serverUpdateEntitySkill(my, 33);
								}
							}
						}
					}
					else
					{
						my->monsterSpecialState = DUCK_INERT;
						serverUpdateEntitySkill(my, 33);
					}
				}
			}
			else
			{
				if ( spiritDuck )
				{
					if ( my->monsterSpecialTimer > 0 )
					{
						--my->monsterSpecialTimer;
					}
				}
			}
		}
	}

	if ( body )
	{
		if ( waterTile )
		{
			if ( (my->monsterSpecialState == DUCK_INERT || my->monsterSpecialState == DUCK_RETURN) && abs(DUCK_FLOAT_ATK) < 0.05
				&& DUCK_INERT_ANIM_COMPLETE >= 0.01 && abs(DUCK_FLOAT_ATK_DIVE - inertHeight) < 0.01 )
			{
				inWater = true;
			}
		}

		DUCK_WALK_CYCLE *= 0.8;
		DUCK_WALK_CYCLE2 *= 0.8;
		if ( dist > 0.05 && !DUCK_INWATER )
		{
			if ( my->monsterSpecialState == DUCK_RETURN )
			{
				DUCK_WALK_CYCLE = 1.0;
			}
			else
			{
				DUCK_WALK_CYCLE2 = 1.0;
			}
		}

		if ( my->monsterSpecialState != DUCK_LAST_SPECIAL_STATE )
		{
			DUCK_SPECIAL_TIMER = 0;
		}
		DUCK_LAST_SPECIAL_STATE = my->monsterSpecialState;
		if ( my->monsterSpecialState )
		{
			++DUCK_SPECIAL_TIMER;
		}
		else
		{
			DUCK_SPECIAL_TIMER = 0;
		}
	}

	int appearance = 0;
	if ( my->sprite == 2231 )
	{
		appearance = 1;
	}
	else if ( my->sprite == 2237 )
	{
		appearance = 2;
	}
	else if ( my->sprite == 2307 )
	{
		appearance = 3;
	}

	if ( spiritDuck )
	{
		my->ditheringOverride = 6;
		my->mistformGLRender = 1.0;
	}

	//Move bodyparts
	Entity* leftWing = nullptr;
	Entity* leftLeg = nullptr;
	for ( bodypart = 0, node = my->children.first; node != nullptr; node = node->next, ++bodypart )
	{
		if ( bodypart < DUCK_BODY )
		{
			continue;
		}

		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		entity->ditheringDisabled = true;
		entity->yaw = my->yaw;

		if ( spiritDuck )
		{
			entity->ditheringOverride = my->ditheringOverride;
			entity->mistformGLRender = my->mistformGLRender;
		}

		real_t dodgeSpinDir = DUCK_FLOAT_ATK >= 0.0 ? (PI / 8) : -PI / 8;
		entity->yaw -= dodgeSpinDir * sin((PI / 2) * abs(DUCK_FLOAT_ATK));
		
		if ( bodypart == DUCK_HEAD )
		{
			if ( body && (my->monsterSpecialState == DUCK_INERT || my->monsterSpecialState == DUCK_RETURN) && abs(DUCK_FLOAT_ATK) < 0.05
				&& DUCK_INERT_ANIM_COMPLETE >= 0.01 && abs(DUCK_FLOAT_ATK_DIVE - inertHeight) < 0.01 )
			{
				entity->flags[INVISIBLE] = false;
			}
			else
			{
				entity->flags[INVISIBLE] = true;
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
			entity->fskill[1] = fmod(entity->fskill[1], 2 * PI);
			while ( entity->fskill[1] >= PI )
			{
				entity->fskill[1] -= 2 * PI;
			}
			while ( entity->fskill[1] < -PI )
			{
				entity->fskill[1] += 2 * PI;
			}
		}
		else if ( bodypart == DUCK_BODY )
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
				DUCK_FLOAT_ATK *= 0.8;
			}

			if ( my->monsterSpecialState == DUCK_DIVE )
			{
				DUCK_INERT_ANIM *= 0.95;
				DUCK_INERT_ANIM_COMPLETE *= 0.95;
			}
			else if ( my->monsterSpecialState == DUCK_INERT || my->monsterSpecialState == DUCK_RETURN )
			{
				body->fskill[0] *= 0.8;
				DUCK_DIVE_ANIM *= 0.8;
			}
			else
			{
				DUCK_FLOAT_ATK_DIVE *= 0.95;
				DUCK_FLOAT_Z_MULT *= 0.95;
				body->fskill[0] *= 0.95;
				DUCK_DIVE_ANIM *= 0.95;
				DUCK_INERT_ANIM *= 0.95;
				DUCK_INERT_ANIM_COMPLETE *= 0.95;
			}

			if ( MONSTER_ATTACK > 0 && (my->monsterSpecialState == DUCK_DIVE || my->monsterSpecialState == DUCK_RETURN) )
			{
				MONSTER_ATTACK = 0;
				MONSTER_ATTACKTIME = 0;
			}
			if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP2 )
			{
				if ( MONSTER_ATTACKTIME == 0 )
				{
					DUCK_FLOAT_ATK = std::max(DUCK_FLOAT_ATK, 0.0);
					if ( multiplayer != CLIENT && myStats )
					{
						myStats->setEffectActive(EFF_STUNNED, 1);
						myStats->EFFECTS_TIMERS[EFF_STUNNED] = 50;
					}
					playSoundEntityLocal(my, 794 + local_rng.rand() % 2, 128);
					playSoundEntityLocal(my, 786 + local_rng.rand() % 3, 128);
				}
				DUCK_FLOAT_ATK += std::max(0.1, 1.0 / 10.0);
				DUCK_FLOAT_ATK = std::min(1.0, DUCK_FLOAT_ATK);

				if ( MONSTER_ATTACKTIME >= 35 )
				{
					MONSTER_ATTACK = 0;
				}
			}
			else if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP3 )
			{
				if ( MONSTER_ATTACKTIME == 0 )
				{
					DUCK_FLOAT_ATK = std::min(DUCK_FLOAT_ATK, 0.0);
					if ( multiplayer != CLIENT && myStats )
					{
						myStats->setEffectActive(EFF_STUNNED, 1);
						myStats->EFFECTS_TIMERS[EFF_STUNNED] = 50;
					}
					playSoundEntityLocal(my, 794 + local_rng.rand() % 2, 128);
					playSoundEntityLocal(my, 786 + local_rng.rand() % 3, 128);
				}
				DUCK_FLOAT_ATK -= std::max(0.1, 1.0 / 10.0);
				DUCK_FLOAT_ATK = std::max(-1.0, DUCK_FLOAT_ATK);

				if ( MONSTER_ATTACKTIME >= 35 )
				{
					MONSTER_ATTACK = 0;
				}
			}

			if ( my->monsterSpecialState == DUCK_INERT || my->monsterSpecialState == DUCK_RETURN )
			{
				real_t start = -5;
				real_t end = 15.5;
				real_t midpoint = start + (end - start) / 2;

				DUCK_FLOAT_Z_MULT = std::min(1.0, DUCK_FLOAT_Z_MULT + 0.025);

				real_t ratio = 1.0 - cos((PI / 2) * std::min(DUCK_SPECIAL_TIMER, 35) / (real_t)35);
				DUCK_INERT_ANIM = ratio;
				real_t floatHeight = inertHeight - 2.0 * abs(DUCK_FLOAT_ATK);

				if ( DUCK_INERT_ANIM >= 0.95 )
				{
					if ( abs(DUCK_FLOAT_ATK_DIVE - floatHeight) < 0.01 )
					{
						DUCK_INERT_ANIM_COMPLETE += std::max(0.01, DUCK_INERT_ANIM_COMPLETE / 10.0);
						DUCK_INERT_ANIM_COMPLETE = std::min(1.0, DUCK_INERT_ANIM_COMPLETE) * (1.0 - abs(DUCK_FLOAT_ATK));
						entity->fskill[1] += 0.2 * abs(DUCK_FLOAT_ATK);

						if ( my->monsterSpecialState == DUCK_RETURN )
						{
							if ( my->ticks % 4 == 0 )
							{
								spawnPoof(entity->x, entity->y, 7.5, 0.25, false);
							}
						}
					}
					else
					{
						entity->fskill[1] += 0.2;
					}
				}
				else
				{
					entity->fskill[1] += 0.1;
					entity->fskill[1] += 0.1 * abs(DUCK_FLOAT_ATK);
				}

				if ( DUCK_FLOAT_ATK_DIVE > floatHeight )
				{
					DUCK_FLOAT_ATK_DIVE = std::max(floatHeight, DUCK_FLOAT_ATK_DIVE - 0.25);
				}
				else
				{
					DUCK_FLOAT_ATK_DIVE = std::min(floatHeight, DUCK_FLOAT_ATK_DIVE + 0.1);
				}

				if ( DUCK_WALK_CYCLE )
				{
					DUCK_WALK_CYCLE_ANIM += 0.3;
				}
				if ( DUCK_WALK_CYCLE2 )
				{
					DUCK_WALK_CYCLE_ANIM2 += 0.35;
				}
				while ( DUCK_WALK_CYCLE_ANIM >= 2 * PI )
				{
					DUCK_WALK_CYCLE_ANIM -= 2 * PI;
				}
				while ( DUCK_WALK_CYCLE_ANIM2 >= 2 * PI )
				{
					DUCK_WALK_CYCLE_ANIM2 -= 2 * PI;
				}

				if ( DUCK_INERT_ANIM_COMPLETE * DUCK_WALK_CYCLE2 >= 0.95 && (abs(fmod(DUCK_WALK_CYCLE_ANIM2, PI) < 0.35)) )
				{
					playSoundEntityLocal(my, 779 + local_rng.rand() % 5, 12);
				}
				else if ( DUCK_INERT_ANIM_COMPLETE * DUCK_WALK_CYCLE >= 0.95 && (abs(fmod(DUCK_WALK_CYCLE_ANIM, PI) < 0.3)) )
				{
					playSoundEntityLocal(my, 779 + local_rng.rand() % 5, 32);
				}
			}
			else if ( my->monsterSpecialState == DUCK_DIVE )
			{
				int interval = 15;
				int interval2 = 50;
				int interval3 = 150;
				if ( DUCK_SPECIAL_TIMER < interval )
				{
					if ( entity->fskill[0] < 0.01 )
					{
						entity->fskill[0] = std::max(-PI / 8, entity->fskill[0] - PI / 64);
					}
					else
					{
						entity->fskill[0] *= 0.75;
					}
					entity->fskill[1] += 0.1;


					entity->skill[1] = 0;
					DUCK_DIVE_ANIM *= 0.8; // decay if previous dive
					if ( DUCK_FLOAT_ATK_DIVE >= 5.0 )
					{
						DUCK_FLOAT_ATK_DIVE -= (DUCK_FLOAT_ATK_DIVE - 5.0) * (DUCK_SPECIAL_TIMER / (real_t)interval);
						DUCK_FLOAT_ATK_DIVE = std::max(5.0, DUCK_FLOAT_ATK_DIVE);
					}
					else
					{
						DUCK_FLOAT_ATK_DIVE = 5 * sin((DUCK_SPECIAL_TIMER / (real_t)(4 * interval)) * 2 * PI);
					}
				}
				else if ( DUCK_SPECIAL_TIMER >= interval && (DUCK_SPECIAL_TIMER - interval) <= interval2 )
				{
					if ( DUCK_SPECIAL_TIMER == interval )
					{
						entity->fskill[0] = -PI / 8;
					}
					int currentTick = (DUCK_SPECIAL_TIMER - interval);

					entity->fskill[1] += 0.1;

					DUCK_FLOAT_ATK_DIVE = 5 * (1 - 2 * sin((PI / 2) * std::min(interval2, currentTick) / (real_t)interval2));
				}
				else if ( (DUCK_SPECIAL_TIMER - interval - interval2) >= 0 )
				{
					int currentTick = (DUCK_SPECIAL_TIMER - interval - interval2);
					entity->fskill[1] += 0.1;
					DUCK_FLOAT_Z_MULT = std::min(1.0, DUCK_FLOAT_Z_MULT + 0.025);

					real_t ratio = 1.0 - cos((PI / 2) * std::min(currentTick, 35) / (real_t)35);
					DUCK_DIVE_ANIM = ratio;
					entity->fskill[0] = std::min(6 * PI / 8, - PI / 8 + ratio * 6 * PI / 4);

					real_t start = -5;
					real_t end = 15.5;
					real_t midpoint = start + (end - start) / 2;

					real_t ratio2 = 1.0 - cos((PI / 2) * std::min(std::max(0, currentTick - 5), 25) / (real_t)25);
					DUCK_FLOAT_ATK_DIVE = midpoint - ((end - start) / 2) * sin(PI / 2 - PI * ratio2);
					if ( currentTick >= 22 )
					{
						inWater = true;
					}

					int huntInterval = 150;
					/*if ( currentTick == 22 && spiritDuck && !waterTile )
					{
						if ( multiplayer != CLIENT )
						{
							if ( lavaTile )
							{
								my->monsterSpecialState = 0;
								serverUpdateEntitySkill(my, 33);
							}
							else if ( !noFloor )
							{
								Entity* spellTimer = createParticleTimer(nullptr, TICKS_PER_SECOND, -1);
								spellTimer->x = my->x;
								spellTimer->y = my->y;
								spellTimer->z = 0.0;
								spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_TRAP_SABOTAGED;
								serverSpawnMiscParticlesAtLocation(spellTimer->x, spellTimer->y, spellTimer->z, PARTICLE_EFFECT_SABOTAGE_TRAP, 0);
								my->monsterSpecialState = 0;
								serverUpdateEntitySkill(my, 33);
								playSoundEntity(my, 807, 128);
								createSpellExplosionArea(SPELL_PROJECT_SPIRIT, my->skill[2] >= 0 && my->skill[2] < MAXPLAYERS ? players[my->skill[2]]->entity : my, my->x, my->y, 7.5, 8.0, 5, nullptr);
							}
						}
					}*/
					if ( DUCK_DIVE_ANIM >= 0.975 && currentTick >= huntInterval )
					{
						int interval4 = 300;
						int raiseLowerInterval = 10;
						const real_t diveDepth = 5.0;
						const real_t bobDepth = 0.25;
						if ( (currentTick - huntInterval) % interval4 < raiseLowerInterval )
						{
							DUCK_FLOAT_ATK_DIVE -= diveDepth * sin(PI / 2 * ((currentTick - huntInterval) % interval4) / (real_t)(raiseLowerInterval));
						}
						else if ( (currentTick - huntInterval) % interval4 < interval4 / 2 )
						{
							if ( (currentTick - huntInterval) % interval4 == raiseLowerInterval )
							{
								if ( waterTile )
								{
									createWaterSplash(my->x, my->y, 30);
									playSoundEntityLocal(my, 136, 64);
								}
							}
							DUCK_FLOAT_ATK_DIVE -= diveDepth;
						}
						else if ( (currentTick - huntInterval) % interval4 < (interval4 / 2 + raiseLowerInterval) )
						{
							DUCK_FLOAT_ATK_DIVE += -diveDepth + diveDepth * sin(PI / 2 * ((currentTick - huntInterval - interval4 / 2) % interval4) / (real_t)(raiseLowerInterval));
						}

						DUCK_FLOAT_ATK_DIVE += bobDepth * sin((currentTick % TICKS_PER_SECOND / (real_t)TICKS_PER_SECOND) * 2 * PI);

						if ( currentTick > huntInterval && (currentTick % (2 * huntInterval) == huntInterval / 2) )
						{
							if ( spiritDuck && !waterTile )
							{
								my->monsterSpecialState = 0;
								serverUpdateEntitySkill(my, 33);
							}
							else if ( multiplayer != CLIENT )
							{
								int bless = 0;
								if ( myStats )
								{
									if ( myStats->getAttribute("duck_bless") != "" )
									{
										bless = std::stoi(myStats->getAttribute("duck_bless"));
									}
								}
								int chance = std::max(1, 5 - bless);
								if ( local_rng.rand() % chance == 0 )
								{
									Item* item = nullptr;
									if ( bless > 0 && local_rng.rand() % 3 == 0 )
									{
										int charge = std::min(ENCHANTED_FEATHER_MAX_DURABILITY - 1, 25 + local_rng.rand() % (bless * 25));
										item = newItem(ENCHANTED_FEATHER, EXCELLENT, 0, 1, charge, false, nullptr);
										--bless;
										myStats->setAttribute("duck_bless", std::to_string(bless));
									}
									else
									{
										ItemType type = FOOD_FISH;
										item = newItem(FOOD_FISH, static_cast<Status>(DECREPIT + local_rng.rand() % 4), -1 + local_rng.rand() % 3, 1, local_rng.rand(), false, nullptr);
										if ( abs(bless) > 0 )
										{
											item->beatitude = bless;
										}
									}
									if ( Entity* dropped = dropItemMonster(item, my, myStats, 1) )
									{
										
									}
									else
									{
										free(item);
									}
									playSoundEntity(my, 789 + local_rng.rand() % 5, 128);
									my->monsterSpecialState = 0;
									serverUpdateEntitySkill(my, 33);
								}
								else if ( currentTick >= (2 * huntInterval) * 3 )
								{
									my->monsterSpecialState = 0;
									serverUpdateEntitySkill(my, 33);
								}
							}
						}
					}

				}
			}
		}
		else if ( bodypart == DUCK_LEFTWING )
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
		}

		switch ( bodypart )
		{
		case DUCK_BODY:
		{
			if ( appearance == 3 )
			{
				entity->sprite = 2308;
			}
			else
			{
				entity->sprite = 2226 + appearance * 6;
			}

			if ( head )
			{
				entity->flags[INVISIBLE] = !head->flags[INVISIBLE];
			}

			entity->x += limbs[DUCK_SMALL][6][0] * cos(entity->yaw);
			entity->y += limbs[DUCK_SMALL][6][1] * sin(entity->yaw);
			entity->z += limbs[DUCK_SMALL][6][2];
			entity->focalx = limbs[DUCK_SMALL][1][0];
			entity->focaly = limbs[DUCK_SMALL][1][1];
			entity->focalz = limbs[DUCK_SMALL][1][2];

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
			entity->pitch += DUCK_WALK_CYCLE * PI / 8;
			if ( my->monsterState != MONSTER_STATE_WAIT )
			{
				entity->pitch += DUCK_WALK_CYCLE2 * PI / 16;
			}
			entity->pitch -= abs(DUCK_FLOAT_ATK) * PI / 16;

			if ( enableDebugKeys && (svFlags & SV_FLAG_CHEATS) )
			{
				if ( keystatus[SDLK_KP_PLUS] )
				{
					keystatus[SDLK_KP_PLUS] = 0;
					entity->skill[0] = entity->skill[0] == 0 ? 1 : 0;
				}
			}
			entity->roll = DUCK_INERT_ANIM_COMPLETE * DUCK_WALK_CYCLE * 0.25 * (sin(DUCK_WALK_CYCLE_ANIM));

			if ( my->monsterSpecialState == DUCK_DIVE || my->monsterSpecialState == DUCK_INERT || my->monsterSpecialState == DUCK_RETURN )
			{
				//entity->fskill[1] += 0.25;
			}
			else if ( entity->skill[0] == 0 )
			{
				entity->fskill[1] += 0.1;
				entity->fskill[1] += 0.05 * abs(DUCK_FLOAT_ATK);
			}

			{
				DUCK_FLOAT_X = limbs[DUCK_SMALL][10][0] * sin(body->fskill[1] * limbs[DUCK_SMALL][11][0]) * cos(entity->yaw + PI / 2) * (1.0 - DUCK_FLOAT_Z_MULT);
				DUCK_FLOAT_Y = limbs[DUCK_SMALL][10][1] * sin(body->fskill[1] * limbs[DUCK_SMALL][11][1]) * sin(entity->yaw + PI / 2) * (1.0 - DUCK_FLOAT_Z_MULT);
				DUCK_FLOAT_Z = limbs[DUCK_SMALL][10][2] * sin(body->fskill[1] * limbs[DUCK_SMALL][11][2]) * (1.0 - DUCK_FLOAT_Z_MULT) * (1.0 - abs(DUCK_FLOAT_ATK));

				DUCK_FLOAT_Z += DUCK_FLOAT_ATK_DIVE;
				DUCK_FLOAT_Z -= DUCK_INERT_ANIM_COMPLETE * DUCK_WALK_CYCLE * 0.5 * abs(sin(DUCK_WALK_CYCLE_ANIM));
			}

			if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP2 || MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP3 )
			{
				if ( MONSTER_ATTACKTIME == 0 )
				{
					spawnPoof(entity->x + DUCK_FLOAT_X, entity->y + DUCK_FLOAT_Y, entity->z + DUCK_FLOAT_Z, 0.5);
				}
			}

			real_t floatAtkZ = -2.0 * sin((PI / 2) * abs(DUCK_FLOAT_ATK));
			DUCK_FLOAT_X += 4.0 * sin((PI / 2) * abs(DUCK_FLOAT_ATK)) * cos(entity->yaw + (PI - dodgeSpinDir));
			DUCK_FLOAT_Y += 4.0 * sin((PI / 2) * abs(DUCK_FLOAT_ATK)) * sin(entity->yaw + (PI - dodgeSpinDir));
			DUCK_FLOAT_Z += floatAtkZ;

			if ( DUCK_INWATER )
			{
				DUCK_BOB_WATER += 0.025;
				DUCK_FLOAT_Z += 0.125 * (1.0 + sin(DUCK_BOB_WATER * 2 * PI + PI / 2));
				entity->pitch += (PI / 64) * (1.0 + sin(DUCK_BOB_WATER * 2 * PI));
			}
			else
			{
				DUCK_BOB_WATER = 0.0;
			}

			entity->x += DUCK_FLOAT_X;
			entity->y += DUCK_FLOAT_Y;
			entity->z += DUCK_FLOAT_Z;

			DUCK_CAM_Z = entity->z;

			DUCK_CAM_Z -= (limbs[DUCK_SMALL][10][2]) * sin(body->fskill[1] * limbs[DUCK_SMALL][11][2]) * (1.0 - DUCK_FLOAT_Z_MULT) * (1.0 - abs(DUCK_FLOAT_ATK));
			DUCK_CAM_Z += (limbs[DUCK_SMALL][10][2] / 5.0) * sin(body->fskill[1] * limbs[DUCK_SMALL][11][2] * 0.25) * (1.0 - DUCK_FLOAT_Z_MULT) * (1.0 - abs(DUCK_FLOAT_ATK));
			break;
		}
		case DUCK_HEAD:
			if ( appearance == 3 )
			{
				entity->sprite = 2307;
			}
			else
			{
				entity->sprite = 2225 + appearance * 6;
			}

			entity->x += limbs[DUCK_SMALL][7][0] * cos(entity->yaw);
			entity->y += limbs[DUCK_SMALL][7][1] * sin(entity->yaw);
			entity->z += limbs[DUCK_SMALL][7][2];
			entity->focalx = limbs[DUCK_SMALL][2][0];
			entity->focaly = limbs[DUCK_SMALL][2][1];
			entity->focalz = limbs[DUCK_SMALL][2][2];
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

			entity->fskill[0] = body->pitch;
			entity->pitch = entity->fskill[0];

			if ( body )
			{
				if ( spiritDuck )
				{
					if ( my->skill[2] >= 0 && my->skill[2] < MAXPLAYERS )
					{
						if ( players[my->skill[2]]->ghost.isActive() )
						{
							if ( players[my->skill[2]]->ghost.my )
							{
								real_t pitch = players[my->skill[2]]->ghost.my->pitch;
								while ( pitch >= PI )
								{
									pitch -= 2 * PI;
								}
								entity->fskill[1] = -pitch / (PI / 3);
							}
						}
					}
				}
				else
				{

					if ( my->monsterState == MONSTER_STATE_ATTACK )
					{
						entity->fskill[1] += 0.1;
						entity->fskill[1] = std::min(1.0, entity->fskill[1]);
					}
					else
					{
						entity->fskill[1] -= 0.1;
						entity->fskill[1] = std::max(0.0, entity->fskill[1]);
					}
				}
				entity->pitch -= sin(entity->fskill[1] * PI / 2) * PI / 8;
				entity->roll = body->roll;
				entity->roll += DUCK_INERT_ANIM_COMPLETE * DUCK_WALK_CYCLE2 * 0.125 * (sin(DUCK_WALK_CYCLE_ANIM2 + PI + PI / 4));
				entity->x += DUCK_FLOAT_X;
				entity->y += DUCK_FLOAT_Y;
				entity->z += DUCK_FLOAT_Z;

				entity->x += DUCK_INERT_ANIM_COMPLETE * DUCK_WALK_CYCLE2 * 1.5 * sin(entity->roll) * cos(entity->yaw + PI / 2);
				entity->y += DUCK_INERT_ANIM_COMPLETE * DUCK_WALK_CYCLE2 * 1.5 * sin(entity->roll) * sin(entity->yaw + PI / 2);
				entity->z -= DUCK_INERT_ANIM_COMPLETE * DUCK_WALK_CYCLE2 * (0.2 + 0.2 * (sin(2 * DUCK_WALK_CYCLE_ANIM2 + PI / 2)));
			}
			break;
		case DUCK_LEFTWING:
			if ( appearance == 3 )
			{
				entity->sprite = 2309;
			}
			else
			{
				entity->sprite = 2227 + appearance * 6;
			}

			entity->x += limbs[DUCK_SMALL][8][0] * cos(entity->yaw) + limbs[DUCK_SMALL][8][1] * cos(entity->yaw + PI / 2);
			entity->y += limbs[DUCK_SMALL][8][0] * sin(entity->yaw) + limbs[DUCK_SMALL][8][1] * sin(entity->yaw + PI / 2);
			entity->z += limbs[DUCK_SMALL][8][2];
			entity->focalx = limbs[DUCK_SMALL][3][0];
			entity->focaly = limbs[DUCK_SMALL][3][1];
			entity->focalz = limbs[DUCK_SMALL][3][2];

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

			if ( (entity->skill[3] == 0) )
			{
				real_t minRot = -1.3;
				real_t maxRot = 0.8;

				if ( my->monsterSpecialState == DUCK_INERT || my->monsterSpecialState == DUCK_RETURN )
				{
					minRot = 0.0;
					maxRot = 0.8;
				}

				real_t midpoint = minRot + (maxRot - minRot) / 2;
				entity->fskill[1] = midpoint + ((maxRot - minRot) / 2) * sin(PI / 2 + body->fskill[1] * limbs[DUCK_SMALL][11][2]);
			}

			entity->pitch = entity->fskill[0] * (1.0 - DUCK_INERT_ANIM) * (1.0 - DUCK_DIVE_ANIM);
			entity->pitch += (DUCK_DIVE_ANIM)*PI;
			entity->roll = entity->fskill[1] * std::max(abs(DUCK_FLOAT_ATK), (1.0 - DUCK_DIVE_ANIM) * (1.0 - DUCK_INERT_ANIM_COMPLETE));

			entity->pitch -= DUCK_INERT_ANIM_COMPLETE * DUCK_WALK_CYCLE * (PI / 2);
			entity->roll += DUCK_INERT_ANIM_COMPLETE * DUCK_WALK_CYCLE * (PI / 2);
			leftWing = entity;

			if ( head->flags[INVISIBLE] )
			{
				entity->scalex += 0.1;
				entity->scalex = std::min(1.0, entity->scalex);
			}
			else
			{
				entity->scalex -= 0.1;
				entity->scalex = std::max(0.0, entity->scalex);
			}
			entity->scaley = entity->scalex;
			entity->scalez = entity->scalex;

			entity->x += (1.0 - entity->scalex) * limbs[DUCK_SMALL][24][1] * cos(entity->yaw + PI / 2);
			entity->y += (1.0 - entity->scalex) * limbs[DUCK_SMALL][24][1] * sin(entity->yaw + PI / 2);
			entity->z += (1.0 - entity->scalex) * limbs[DUCK_SMALL][24][2];

			if ( body )
			{
				entity->x += DUCK_FLOAT_X;
				entity->y += DUCK_FLOAT_Y;
				entity->z += DUCK_FLOAT_Z;

				entity->x += limbs[DUCK_SMALL][12][0] * sin(body->pitch) * cos(entity->yaw);
				entity->y += limbs[DUCK_SMALL][12][1] * sin(body->pitch) * sin(entity->yaw);
				entity->z += limbs[DUCK_SMALL][12][2] * sin(body->pitch);
			}
			break;
		case DUCK_RIGHTWING:
			if ( appearance == 3 )
			{
				entity->sprite = 2310;
			}
			else
			{
				entity->sprite = 2228 + appearance * 6;
			}
			/*if ( body )
			{
				entity->flags[INVISIBLE] = body->flags[INVISIBLE];
			}*/

			entity->x += limbs[DUCK_SMALL][9][0] * cos(entity->yaw) + limbs[DUCK_SMALL][9][1] * cos(entity->yaw + PI / 2);
			entity->y += limbs[DUCK_SMALL][9][0] * sin(entity->yaw) + limbs[DUCK_SMALL][9][1] * sin(entity->yaw + PI / 2);
			entity->z += limbs[DUCK_SMALL][9][2];
			entity->focalx = limbs[DUCK_SMALL][4][0];
			entity->focaly = limbs[DUCK_SMALL][4][1];
			entity->focalz = limbs[DUCK_SMALL][4][2];
			if ( leftWing )
			{
				entity->fskill[0] = leftWing->fskill[0];
				entity->fskill[1] = -leftWing->fskill[1];
			}

			entity->pitch = entity->fskill[0] * (1.0 - DUCK_INERT_ANIM) * (1.0 - DUCK_DIVE_ANIM);
			entity->pitch += (DUCK_DIVE_ANIM)*PI;
			entity->roll = entity->fskill[1] * std::max(abs(DUCK_FLOAT_ATK), (1.0 - DUCK_DIVE_ANIM) * (1.0 - DUCK_INERT_ANIM_COMPLETE));

			entity->pitch -= DUCK_INERT_ANIM_COMPLETE * DUCK_WALK_CYCLE * (PI / 2);
			entity->roll -= DUCK_INERT_ANIM_COMPLETE * DUCK_WALK_CYCLE * (PI / 2);

			entity->scalex = leftWing->scalex;
			entity->scaley = leftWing->scalex;
			entity->scalez = leftWing->scalex;

			entity->x += (1.0 - entity->scalex) * limbs[DUCK_SMALL][24][0] * cos(entity->yaw + PI / 2);
			entity->y += (1.0 - entity->scalex) * limbs[DUCK_SMALL][24][0] * sin(entity->yaw + PI / 2);
			entity->z += (1.0 - entity->scalex) * limbs[DUCK_SMALL][24][2];

			if ( body )
			{
				entity->x += DUCK_FLOAT_X;
				entity->y += DUCK_FLOAT_Y;
				entity->z += DUCK_FLOAT_Z;

				entity->x += limbs[DUCK_SMALL][12][0] * sin(body->pitch) * cos(entity->yaw);
				entity->y += limbs[DUCK_SMALL][12][1] * sin(body->pitch) * sin(entity->yaw);
				entity->z += limbs[DUCK_SMALL][12][2] * sin(body->pitch);

			}
			break;
		case DUCK_LEFTLEG:
			{
				entity->focalx = limbs[DUCK_SMALL][20][0];
				entity->focaly = limbs[DUCK_SMALL][20][1];
				entity->focalz = limbs[DUCK_SMALL][20][2];
				bool webFoot = (head && !head->flags[INVISIBLE]) && (my->monsterSpecialState != DUCK_RETURN);
				if ( webFoot )
				{
					entity->focalz -= 0.25;
					entity->focaly += 0.25;
					entity->z += 0.25;
				}

				entity->sprite = 2229;
				if ( my->sprite == 2225 || my->sprite == 2226 )
				{
					if ( webFoot )
					{
						entity->sprite = 2243;
					}
					else
					{
						entity->sprite = 2229;
					}
				}
				else if ( my->sprite == 2231 || my->sprite == 2232 )
				{
					if ( webFoot )
					{
						entity->sprite = 2244;
					}
					else
					{
						entity->sprite = 2235;
					}
				}
				else if ( my->sprite == 2237 || my->sprite == 2238 )
				{
					if ( webFoot )
					{
						entity->sprite = 2245;
					}
					else
					{
						entity->sprite = 2241;
					}
				}
				else if ( my->sprite == 2307 || my->sprite == 2308 )
				{
					if ( webFoot )
					{
						entity->sprite = 2313;
					}
					else
					{
						entity->sprite = 2311;
					}
				}
				if ( body && head )
				{
					entity->flags[INVISIBLE] = body->flags[INVISIBLE] && head->flags[INVISIBLE];
				}

				entity->x += limbs[DUCK_SMALL][19][0] * cos(entity->yaw) + limbs[DUCK_SMALL][19][1] * cos(entity->yaw + PI / 2);
				entity->y += limbs[DUCK_SMALL][19][0] * sin(entity->yaw) + limbs[DUCK_SMALL][19][1] * sin(entity->yaw + PI / 2);
				entity->z += limbs[DUCK_SMALL][19][2];

				if ( body )
				{
					entity->x += DUCK_FLOAT_X;
					entity->y += DUCK_FLOAT_Y;
					entity->z += DUCK_FLOAT_Z;

					entity->x += limbs[DUCK_SMALL][23][0] * sin(body->pitch) * cos(body->yaw);
					entity->y += limbs[DUCK_SMALL][23][1] * sin(body->pitch) * sin(body->yaw);
					entity->z += limbs[DUCK_SMALL][23][2] * cos(body->pitch);

					entity->roll = body->roll;
				}

				leftLeg = entity;

				entity->fskill[0] = -leftWing->roll * (1.0 - DUCK_INERT_ANIM_COMPLETE);
				if ( my->monsterSpecialState == DUCK_DIVE )
				{
					real_t minRot = -1.3;
					real_t maxRot = 0.8;
					real_t midpoint = minRot + (maxRot - minRot) / 2;
					entity->fskill[0] = midpoint + ((maxRot - minRot) / 2) * sin(PI / 2 + ((ticks % 15) / 7.5) * PI);
				}
				entity->pitch = entity->fskill[0] + entity->fskill[1] + body->pitch;
				//entity->pitch += (PI / 4) * DUCK_INERT_ANIM_COMPLETE;
				entity->pitch += DUCK_INERT_ANIM_COMPLETE * DUCK_WALK_CYCLE * (PI / 4) * sin(DUCK_WALK_CYCLE_ANIM);

				entity->x += DUCK_INERT_ANIM_COMPLETE * DUCK_WALK_CYCLE2 * (0.5 + 0.5 * sin(DUCK_WALK_CYCLE_ANIM2 + PI)) * cos(entity->yaw);
				entity->y += DUCK_INERT_ANIM_COMPLETE * DUCK_WALK_CYCLE2 * (0.5 + 0.5 * sin(DUCK_WALK_CYCLE_ANIM2 + PI)) * sin(entity->yaw);
				entity->z -= DUCK_INERT_ANIM_COMPLETE * DUCK_WALK_CYCLE2 * 0.4 * std::max(0.0, sin(DUCK_WALK_CYCLE_ANIM2 + PI + PI / 2));
				entity->pitch -= DUCK_INERT_ANIM_COMPLETE * DUCK_WALK_CYCLE2 * (PI / 4) * std::max(0.0, sin(DUCK_WALK_CYCLE_ANIM2 + PI + PI / 2));
				break;
			}
			case DUCK_RIGHTLEG:
			{
				entity->focalx = limbs[DUCK_SMALL][22][0];
				entity->focaly = limbs[DUCK_SMALL][22][1];
				entity->focalz = limbs[DUCK_SMALL][22][2];

				bool webFoot = (head && !head->flags[INVISIBLE]) && (my->monsterSpecialState != DUCK_RETURN);
				if ( webFoot )
				{
					entity->focalz -= 0.25;
					entity->focaly -= 0.25;
					entity->z += 0.25;
				}

				entity->sprite = 2230;
				if ( my->sprite == 2225 || my->sprite == 2226 )
				{
					if ( webFoot )
					{
						entity->sprite = 2243;
					}
					else
					{
						entity->sprite = 2230;
					}
				}
				else if ( my->sprite == 2231 || my->sprite == 2232 )
				{
					if ( webFoot )
					{
						entity->sprite = 2244;
					}
					else
					{
						entity->sprite = 2236;
					}
				}
				else if ( my->sprite == 2237 || my->sprite == 2238 )
				{
					if ( webFoot )
					{
						entity->sprite = 2245;
					}
					else
					{
						entity->sprite = 2242;
					}
				}
				else if ( my->sprite == 2307 || my->sprite == 2308 )
				{
					if ( webFoot )
					{
						entity->sprite = 2313;
					}
					else
					{
						entity->sprite = 2312;
					}
				}
				if ( body && head )
				{
					entity->flags[INVISIBLE] = body->flags[INVISIBLE] && head->flags[INVISIBLE];
				}

				entity->x += limbs[DUCK_SMALL][21][0] * cos(entity->yaw) + limbs[DUCK_SMALL][21][1] * cos(entity->yaw + PI / 2);
				entity->y += limbs[DUCK_SMALL][21][0] * sin(entity->yaw) + limbs[DUCK_SMALL][21][1] * sin(entity->yaw + PI / 2);
				entity->z += limbs[DUCK_SMALL][21][2];

				if ( body )
				{
					entity->x += DUCK_FLOAT_X;
					entity->y += DUCK_FLOAT_Y;
					entity->z += DUCK_FLOAT_Z;

					entity->x += limbs[DUCK_SMALL][23][0] * sin(body->pitch) * cos(entity->yaw);
					entity->y += limbs[DUCK_SMALL][23][1] * sin(body->pitch) * sin(entity->yaw);
					entity->z += limbs[DUCK_SMALL][23][2] * cos(body->pitch);

					entity->roll = body->roll;
				}

				if ( leftLeg && body )
				{
					if ( my->monsterSpecialState == 0 )
					{
						entity->pitch = leftLeg->fskill[0]; // swing same
					}
					else
					{
						entity->pitch = -leftLeg->fskill[0];
					}
					entity->pitch += leftLeg->fskill[1] + body->pitch;
					//entity->pitch += (PI / 4) * DUCK_INERT_ANIM_COMPLETE;
					entity->pitch -= DUCK_INERT_ANIM_COMPLETE * DUCK_WALK_CYCLE * (PI / 4) * sin(DUCK_WALK_CYCLE_ANIM * 2);

					entity->x += DUCK_INERT_ANIM_COMPLETE * DUCK_WALK_CYCLE2 * (0.5 + 0.5 * sin(DUCK_WALK_CYCLE_ANIM2)) * cos(entity->yaw);
					entity->y += DUCK_INERT_ANIM_COMPLETE * DUCK_WALK_CYCLE2 * (0.5 + 0.5 * sin(DUCK_WALK_CYCLE_ANIM2)) * sin(entity->yaw);
					entity->z -= DUCK_INERT_ANIM_COMPLETE * DUCK_WALK_CYCLE2 * 0.4 * std::max(0.0, sin(DUCK_WALK_CYCLE_ANIM2 + PI / 2));
					entity->pitch -= DUCK_INERT_ANIM_COMPLETE * DUCK_WALK_CYCLE2 * (PI / 4) * std::max(0.0, sin(DUCK_WALK_CYCLE_ANIM2 + PI / 2));
				}
				break;
			}
			default:
				break;
		}
	}

	if ( body )
	{
		if ( (inWater ? 1 : 0) != DUCK_INWATER )
		{
			if ( waterTile )
			{
				createWaterSplash(my->x, my->y, 30);
				playSoundEntityLocal(my, 136, 64);
			}
		}

		if ( !DUCK_INIT ) // spawn feathers
		{
			DUCK_INIT = 1;
			duckSpawnFeather(my->sprite, my->x, my->y, my->z, my);
		}
		DUCK_INWATER = inWater;
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