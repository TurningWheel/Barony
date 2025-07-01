/*-------------------------------------------------------------------------------

	BARONY
	File: monster_skeleton.cpp
	Desc: implements all of the skeleton monster's code

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
#include "magic/magic.hpp"
#include "prng.hpp"
#include "scores.hpp"
#include "mod_tools.hpp"

void initRevenantSkull(Entity* my, Stat* myStats)
{
	node_t* node;

	my->z = 0;
	my->initMonster(1796);
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
	Entity* entity = newEntity(1796, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->z = 6;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[REVENANT_SKULL][1][0];
	entity->focaly = limbs[REVENANT_SKULL][1][1];
	entity->focalz = limbs[REVENANT_SKULL][1][2];
	entity->behavior = &actRevenantSkullLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);
}

void initAdorcisedWeapon(Entity* my, Stat* myStats)
{
	node_t* node;

	my->z = 0;
	my->initMonster(1797);
	my->flags[INVISIBLE] = true; // hide the "AI" bodypart
	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = -1;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = -1;
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

			my->setHardcoreStats(*myStats);
		}
	}

	// body
	Entity* entity = newEntity(15, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->z = 6;
	entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[MONSTER_ADORCISED_WEAPON][1][0];
	entity->focaly = limbs[MONSTER_ADORCISED_WEAPON][1][1];
	entity->focalz = limbs[MONSTER_ADORCISED_WEAPON][1][2];
	entity->behavior = &actAdorcisedWeaponLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);
}

void initFlameElemental(Entity* my, Stat* myStats)
{
	node_t* node;

	my->z = 0;
	my->initMonster(1804);
	my->flags[INVISIBLE] = true; // hide the "AI" bodypart
	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = -1;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = -1;
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

			my->setHardcoreStats(*myStats);
		}
	}

	// body
	Entity* entity = newEntity(1804, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->z = 6;
	//entity->flags[USERFLAG2] = my->flags[USERFLAG2];
	entity->focalx = limbs[FLAME_ELEMENTAL][1][0];
	entity->focaly = limbs[FLAME_ELEMENTAL][1][1];
	entity->focalz = limbs[FLAME_ELEMENTAL][1][2];
	entity->behavior = &actFlameElementalLimb;
	entity->parent = my->getUID();
	entity->lightBonus = vec4_t{ 0.25, 0.25, 0.25, 0.0 };
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);
}

void actRevenantSkullLimb(Entity* my)
{
	if ( my->light )
	{
		list_RemoveNode(my->light->node);
		my->light = nullptr;
	}

	my->light = addLight(my->x / 16, my->y / 16, "revenant_skull_glow");
	my->actMonsterLimb(false);
}

void actAdorcisedWeaponLimb(Entity* my)
{
	if ( my->light )
	{
		list_RemoveNode(my->light->node);
		my->light = nullptr;
	}

	my->light = addLight(my->x / 16, my->y / 16, "adorcised_weapon_glow");
	my->actMonsterLimb(false);
}

void actHologramLimb(Entity* my)
{
	if ( my->light )
	{
		list_RemoveNode(my->light->node);
		my->light = nullptr;
	}

	my->light = addLight(my->x / 16, my->y / 16, "summoned_skeleton_glow");
	my->actMonsterLimb(false);
}

void actFlameElementalLimb(Entity* my)
{
	if ( my->light )
	{
		list_RemoveNode(my->light->node);
		my->light = nullptr;
	}

	my->light = addLight(my->x / 16, my->y / 16, "flame_elemental_glow");
	my->actMonsterLimb(false);
}

void revenantSkullDie(Entity* my)
{
	my->removeMonsterDeathNodes();

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
				entity->sprite = 229;
				break;
			case 1:
				entity->sprite = 230;
				break;
			case 2:
				entity->sprite = 231;
				break;
			case 3:
				entity->sprite = 233;
				break;
			case 4:
				entity->sprite = 235;
				break;
			case 5:
				entity->sprite = 236;
				break;
			}
			serverSpawnGibForClient(entity);
		}
	}
	playSoundEntity(my, 94, 128);
	list_RemoveNode(my->mynode);
	return;
}

void adorcisedWeaponDie(Entity* my)
{
	my->removeMonsterDeathNodes();
	spawnPoof(my->x, my->y, my->z, 1.0, true);
	list_RemoveNode(my->mynode);
	return;
}

void flameElementalDie(Entity* my)
{
	Stat* myStats = my->getStats();
	createSpellExplosionArea(SPELL_FIREBALL, myStats ? uidToEntity(myStats->leader_uid) : nullptr, my->x, my->y, 0.0, 16.0);

	my->removeMonsterDeathNodes();
	spawnPoof(my->x, my->y, my->z, 1.0, true);
	list_RemoveNode(my->mynode);
	return;
}

#define REVENANT_SKULL_BODY 2
#define SKULL_FLOAT_X body->fskill[2]
#define SKULL_FLOAT_Y body->fskill[3]
#define SKULL_FLOAT_Z body->fskill[4]
#define SKULL_FLOAT_ATK body->fskill[5]
#define SKULL_CIRCLE_ANIM body->fskill[6]
#define SKULL_CIRCLE_SCALE body->fskill[7]
#define SKULL_BOB_ANIM body->fskill[8]
#define SKULL_FLIP body->fskill[9]
#define SKULL_BOBS body->skill[3]
#define SKULL_CIRCLES body->skill[4]
#define SKULL_CIRCLES_DECREMENT_MODE body->skill[5]
#define SKULL_NEXTBOB body->skill[6]
#define SKULL_IDLE_TIMER body->skill[7]

void revenantSkullAnimate(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = nullptr;
	Entity* head = nullptr;
	int bodypart;

	my->flags[INVISIBLE] = true; // hide the "AI" bodypart

	my->sizex = 2;
	my->sizey = 2;

	Monster monsterType = my->sprite == 1804 ? FLAME_ELEMENTAL : (my->sprite == 1797 ? MONSTER_ADORCISED_WEAPON : REVENANT_SKULL);

	my->focalx = limbs[monsterType][0][0];
	my->focaly = limbs[monsterType][0][1];
	my->focalz = limbs[monsterType][0][2];
	if ( multiplayer != CLIENT )
	{
		my->z = limbs[monsterType][5][2];
		if ( myStats && !myStats->getEffectActive(EFF_LEVITATING) )
		{
			myStats->setEffectActive(EFF_LEVITATING, 1);
			myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 0;
		}

		my->creatureHandleLiftZ();

		if ( monsterType == MONSTER_ADORCISED_WEAPON )
		{
			if ( myStats->getAttribute("spirit_weapon") != "" )
			{
				my->flags[PASSABLE] = true;
			}

			if ( !myStats->weapon )
			{
				my->setHP(0);
				char buf[120];
				snprintf(buf, sizeof(buf), Language::get(6620));
				my->setObituary(buf);
			}
			else
			{
				if ( myStats->getAttribute("spirit_weapon") != "" )
				{
					if ( my->parent != 0 )
					{
						Entity* parent = uidToEntity(my->parent);
						if ( !parent )
						{
							my->setHP(0);
							if ( myStats->weapon )
							{
								my->setObituary(Language::get(6619));
							}
						}
					}
					if ( my->ticks > std::stoi(myStats->getAttribute("spirit_weapon")) )
					{
						my->setHP(0);
						if ( myStats->weapon )
						{
							my->setObituary(Language::get(6619));
						}
					}
				}
				if ( myStats->getAttribute("adorcised_weapon") != "" )
				{
					if ( my->ticks > std::stoi(myStats->getAttribute("adorcised_weapon")) )
					{
						my->setHP(0);
						if ( myStats->weapon )
						{
							my->setObituary(Language::get(6619));
						}
					}
				}
			}
		}
	}

	if ( enableDebugKeys && (svFlags & SV_FLAG_CHEATS) )
	{
		if ( keystatus[SDLK_g] )
		{
			keystatus[SDLK_g] = 0;
			MONSTER_ATTACK = MONSTER_POSE_MELEE_WINDUP1;
			MONSTER_ATTACKTIME = 0;
		}
		if ( keystatus[SDLK_h] )
		{
			keystatus[SDLK_h] = 0;
			MONSTER_ATTACK = MONSTER_POSE_MAGIC_WINDUP1;
			MONSTER_ATTACKTIME = 0;
		}
		if ( keystatus[SDLK_j] && myStats )
		{
			keystatus[SDLK_j] = 0;
			myStats->setEffectValueUnsafe(EFF_PARALYZED, myStats->getEffectActive(EFF_PARALYZED) ? 0 : 1);
		}
	}

	bool adorcisedWeapon = monsterType == MONSTER_ADORCISED_WEAPON;
	bool poke = false;

	Entity* body = nullptr;
	for ( bodypart = 0, node = my->children.first; node != nullptr; node = node->next, ++bodypart )
	{
		if ( bodypart < REVENANT_SKULL_BODY )
		{
			continue;
		}

		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		entity->yaw = my->yaw;

		if ( bodypart == REVENANT_SKULL_BODY )
		{
			if ( adorcisedWeapon )
			{
				if (   entity->sprite == items[QUARTERSTAFF].index
					|| entity->sprite == items[IRON_SPEAR].index
					|| entity->sprite == items[STEEL_HALBERD].index
					|| entity->sprite == items[ARTIFACT_SPEAR].index
					|| entity->sprite == items[CRYSTAL_SPEAR].index )
				{
					poke = true;
				}
				if ( entity->sprite ==   items[BRONZE_SWORD].index
					|| entity->sprite == items[IRON_SWORD].index
					|| entity->sprite == items[STEEL_SWORD].index
					|| entity->sprite == items[ARTIFACT_SWORD].index
					|| entity->sprite == items[CRYSTAL_SWORD].index
					|| entity->sprite == items[RAPIER].index )
				{
					poke = true;
				}
			}

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

			real_t basePitchSetpoint = (limbs[monsterType][14][0] * PI / 180.0);
			if ( MONSTER_ATTACK == 0 )
			{
				real_t speed = -0.05;
				if ( adorcisedWeapon )
				{
					speed = -0.2;
				}
				real_t setpoint = basePitchSetpoint;
				if ( limbAngleWithinRange(entity->fskill[0], speed, setpoint) )
				{
					entity->fskill[0] = setpoint;
				}
				else if ( entity->fskill[0] < (setpoint - 0.01) )
				{
					entity->fskill[0] += -speed;
					entity->fskill[0] = std::min(entity->fskill[0], setpoint);
				}
				else
				{
					entity->fskill[0] += speed;
					entity->fskill[0] = std::max(entity->fskill[0], setpoint);
				}
			}

			if ( !adorcisedWeapon )
			{
				if ( MONSTER_ATTACK > 0 )
				{
					entity->fskill[0] = basePitchSetpoint;
				}
			}

			if ( MONSTER_ATTACK == 0 )
			{
				SKULL_FLOAT_ATK = 0.0;
			}

			if ( multiplayer != CLIENT )
			{
				if ( SKULL_IDLE_TIMER == 0 )
				{
					if ( my->ticks < TICKS_PER_SECOND / 10 )
					{
						SKULL_IDLE_TIMER = TICKS_PER_SECOND / 10;
					}
					else
					{
						SKULL_IDLE_TIMER = (local_rng.rand() % 7 + 5) * TICKS_PER_SECOND;
					}
				}
				else if ( MONSTER_ATTACK == 0 )
				{
					if ( SKULL_IDLE_TIMER > 0 )
					{
						--SKULL_IDLE_TIMER;
						if ( SKULL_IDLE_TIMER == 0 )
						{
							int pick = MONSTER_POSE_RANGED_WINDUP1 + local_rng.rand() % 3;
							if ( my->ticks < TICKS_PER_SECOND * 5 )
							{
								pick = MONSTER_POSE_RANGED_WINDUP1;
							}

							if ( pick == MONSTER_POSE_RANGED_WINDUP2 )
							{
								if ( adorcisedWeapon && myStats->getAttribute("spirit_weapon") != "" )
								{
									pick = MONSTER_POSE_RANGED_WINDUP3;
								}
								else if ( my->monsterState != MONSTER_STATE_WAIT )
								{
									pick = MONSTER_POSE_RANGED_WINDUP3;
								}
							}

							if ( pick == MONSTER_POSE_RANGED_WINDUP2 )
							{
								my->setEffect(EFF_STUNNED, true, 4 * TICKS_PER_SECOND, false);
							}
							my->attack(pick, 0, nullptr);
						}
					}
				}
			}

			if ( MONSTER_ATTACK == MONSTER_POSE_RANGED_WINDUP1 )
			{
				if ( MONSTER_ATTACKTIME == 0 )
				{
					if ( SKULL_FLIP <= 0.0 )
					{
						SKULL_FLIP = 1.0;
					}
					if ( SKULL_BOBS == 0 )
					{
						SKULL_BOBS = 5;
						SKULL_BOB_ANIM = 0.0;
					}
					else
					{
						SKULL_NEXTBOB = 5;
					}
				}
				MONSTER_ATTACK = 0;
			}
			else if ( MONSTER_ATTACK == MONSTER_POSE_RANGED_WINDUP2 )
			{
				if ( MONSTER_ATTACKTIME == 0 )
				{
					if ( SKULL_FLIP <= 0.0 )
					{
						SKULL_FLIP = 1.0;
					}
					if ( SKULL_CIRCLES == 0 )
					{
						SKULL_CIRCLES = 2;
						SKULL_CIRCLE_ANIM = 0.0;
						SKULL_CIRCLE_SCALE = 0.0;
						SKULL_CIRCLES_DECREMENT_MODE = 0;
					}
					if ( SKULL_BOBS == 0 )
					{
						SKULL_BOBS = 5;
						SKULL_BOB_ANIM = 0.0;
					}
					else
					{
						SKULL_NEXTBOB = 5;
					}
				}
				MONSTER_ATTACK = 0;
			}
			else if ( MONSTER_ATTACK == MONSTER_POSE_RANGED_WINDUP3 )
			{
				if ( MONSTER_ATTACKTIME == 0 )
				{
					if ( SKULL_BOBS == 0 )
					{
						SKULL_BOBS = 5;
						SKULL_BOB_ANIM = 0.0;
					}
					else
					{
						SKULL_NEXTBOB = 5;
					}
				}
				MONSTER_ATTACK = 0;
			}
			else if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP1 || MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 
				|| MONSTER_ATTACK == 1
				|| MONSTER_ATTACK == MONSTER_POSE_MAGIC_CAST1 )
			{
				int delay = (MONSTER_ATTACK == MONSTER_POSE_MAGIC_CAST1 || MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1) ? 25 : 0;
				if ( MONSTER_ATTACKTIME == 0 )
				{
					entity->fskill[0] = basePitchSetpoint;
					entity->skill[1] = 0;
					SKULL_FLOAT_ATK = 0.0;

					if ( MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 )
					{
						if ( multiplayer != CLIENT )
						{
							my->setEffect(EFF_STUNNED, true, delay, false);
						}
						createParticleDot(my);
						// play casting sound
						playSoundEntityLocal(my, 170, 64);
						if ( SKULL_CIRCLES == 0 )
						{
							SKULL_CIRCLE_ANIM = 0.0;
							SKULL_CIRCLE_SCALE = 0.0;
						}
						SKULL_CIRCLES = std::min(2, SKULL_CIRCLES + 2);
						if ( SKULL_BOBS == 0 )
						{
							SKULL_BOBS = 3;
							SKULL_BOB_ANIM = 0.0;
						}
						else
						{
							SKULL_NEXTBOB = 3;
						}
					}
				}
				else
				{
					if ( MONSTER_ATTACKTIME >= (int)limbs[monsterType][15][0] + delay )
					{
						if ( MONSTER_ATTACKTIME == (int)limbs[monsterType][15][0] + delay )
						{
							SKULL_CIRCLES = 0;

							if ( multiplayer != CLIENT )
							{
								const Sint32 temp = MONSTER_ATTACKTIME;
								my->attack(MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 ? MONSTER_POSE_MAGIC_CAST1 : 1, 0, nullptr); // slop
								MONSTER_ATTACKTIME = temp;

								if ( adorcisedWeapon && myStats->getAttribute("spirit_weapon") != "" )
								{
									// knockback to lunge forward
									if ( my->setEffect(EFF_KNOCKBACK, true, 30, false) )
									{
										real_t pushbackMultiplier = 3.0;
										real_t tangent = my->yaw;
										my->vel_x = cos(tangent) * pushbackMultiplier;
										my->vel_y = sin(tangent) * pushbackMultiplier;
										my->monsterKnockbackVelocity = 0.025;
										my->monsterKnockbackUID = 0;
										my->monsterKnockbackTangentDir = tangent;
									}
								}
							}
						}

						if ( !adorcisedWeapon )
						{
							if ( entity->skill[1] == 0 )
							{
								real_t speed = limbs[monsterType][13][2];
								real_t setpoint = (limbs[monsterType][14][2] * PI / 180.0);
								if ( limbAngleWithinRange(entity->fskill[0], -speed, setpoint) )
								{
									entity->fskill[0] = setpoint;
									entity->skill[1] = 1;
								}
								else
								{
									entity->fskill[0] -= speed;
									entity->fskill[0] = std::max(entity->fskill[0], setpoint);
								}
							}
							else
							{
								real_t speed = limbs[monsterType][13][1];
								entity->fskill[0] += speed;
								entity->fskill[0] = std::min(entity->fskill[0], basePitchSetpoint);
							}
						}
						else
						{
							if ( poke )
							{
								static ConsoleVariable<float> cvar_revenant_pokeset("/revenant_pokeset", 90);
								static ConsoleVariable<float> cvar_revenant_pokespd("/revenant_pokespd", 0.01);
								static ConsoleVariable<float> cvar_revenant_pokespd2("/revenant_pokespd2", -0.25);
								if ( entity->skill[1] == 0 )
								{
									real_t speed = *cvar_revenant_pokespd;
									real_t setpoint = (*cvar_revenant_pokeset * PI / 180.0);
									if ( limbAngleWithinRange(entity->fskill[0], -speed, setpoint) )
									{
										entity->fskill[0] = setpoint;
										entity->skill[1] = 1;
									}
									else
									{
										entity->fskill[0] -= speed;
									}
								}
								else
								{
									real_t speed = *cvar_revenant_pokespd2;
									if ( limbAngleWithinRange(entity->fskill[0], speed, basePitchSetpoint) )
									{
										entity->fskill[0] = basePitchSetpoint;
									}
									else
									{
										entity->fskill[0] += speed;
									}
								}
							}
							else
							{
								if ( entity->skill[1] == 0 )
								{
									real_t speed = limbs[monsterType][3][2];
									real_t setpoint = (limbs[monsterType][4][2] * PI / 180.0);
									if ( limbAngleWithinRange(entity->fskill[0], -speed, setpoint) )
									{
										entity->fskill[0] = setpoint;
										entity->skill[1] = 1;
									}
									else
									{
										entity->fskill[0] -= speed;
										entity->fskill[0] = std::max(entity->fskill[0], setpoint);
									}
								}
								else
								{
									real_t speed = limbs[monsterType][3][1];
									entity->fskill[0] += speed;
									entity->fskill[0] = std::min(entity->fskill[0], basePitchSetpoint);
								}
							}
						}
					}
					else
					{
						if ( !adorcisedWeapon )
						{
							real_t speed = limbs[monsterType][13][0];
							real_t setpoint = (limbs[monsterType][14][1] * PI / 180.0);
							entity->fskill[0] -= speed;
							if ( setpoint >= 0 )
							{
								entity->fskill[0] = std::min(entity->fskill[0], setpoint);
							}
							else
							{
								entity->fskill[0] = std::max(entity->fskill[0], setpoint);
							}
						}
						else
						{
							if ( poke )
							{
								static ConsoleVariable<float> cvar_revenant_pokeset1("/revenant_pokeset1", 110.0);
								static ConsoleVariable<float> cvar_revenant_pokespd1("/revenant_pokespd1", -0.25);
								real_t speed = *cvar_revenant_pokespd1;
								real_t setpoint = (*cvar_revenant_pokeset1 * PI / 180.0);
								if ( limbAngleWithinRange(entity->fskill[0], -speed, setpoint) )
								{
									entity->fskill[0] = setpoint;
								}
								else
								{
									entity->fskill[0] -= speed;
								}
							}
							else
							{
								real_t speed = limbs[monsterType][3][0];
								real_t setpoint = (limbs[monsterType][4][1] * PI / 180.0);
								entity->fskill[0] -= speed;
								if ( setpoint >= 0 )
								{
									entity->fskill[0] = std::min(entity->fskill[0], setpoint);
								}
								else
								{
									entity->fskill[0] = std::max(entity->fskill[0], setpoint);
								}
							}
						}
					}

					if ( MONSTER_ATTACKTIME >= (int)limbs[monsterType][18][0] + delay )
					{
						if ( MONSTER_ATTACK == MONSTER_POSE_MAGIC_CAST1 || MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 )
						{
							SKULL_FLOAT_ATK += limbs[monsterType][12][1];
							SKULL_FLOAT_ATK = std::min(SKULL_FLOAT_ATK, (real_t)limbs[monsterType][12][2]);
						}
						else
						{
							SKULL_FLOAT_ATK -= limbs[monsterType][18][1];
							SKULL_FLOAT_ATK = std::max(SKULL_FLOAT_ATK, (real_t)limbs[monsterType][18][2]);
						}
					}
					else if ( MONSTER_ATTACKTIME >= (int)limbs[monsterType][17][0] + delay )
					{
						if ( MONSTER_ATTACK == MONSTER_POSE_MAGIC_CAST1 || MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 )
						{
							SKULL_FLOAT_ATK += limbs[monsterType][19][1];
							SKULL_FLOAT_ATK = std::min(SKULL_FLOAT_ATK, (real_t)limbs[monsterType][19][2]);
						}
						else
						{
							SKULL_FLOAT_ATK += limbs[monsterType][17][1];
							SKULL_FLOAT_ATK = std::min(SKULL_FLOAT_ATK, (real_t)limbs[monsterType][17][2]);
						}
					}
					else if ( MONSTER_ATTACKTIME >= (int)limbs[monsterType][16][0] + delay )
					{
						SKULL_FLOAT_ATK -= limbs[monsterType][16][1];
						SKULL_FLOAT_ATK = std::max(SKULL_FLOAT_ATK, (real_t)limbs[monsterType][16][2]);
					}
				}

				if ( MONSTER_ATTACKTIME >= (int)limbs[monsterType][15][1] + delay )
				{
					MONSTER_ATTACK = 0;
				}
			}
		}

		switch ( bodypart )
		{
			case REVENANT_SKULL_BODY:
			{
				entity->x += limbs[monsterType][6][0] * cos(entity->yaw);
				entity->y += limbs[monsterType][6][1] * sin(entity->yaw);
				entity->z += limbs[monsterType][6][2];
				entity->focalx = limbs[monsterType][1][0];
				entity->focaly = limbs[monsterType][1][1];
				entity->focalz = limbs[monsterType][1][2];

				entity->pitch = entity->fskill[0];

				if ( enableDebugKeys && (svFlags & SV_FLAG_CHEATS) )
				{
					if ( keystatus[SDLK_KP_PLUS] )
					{
						keystatus[SDLK_KP_PLUS] = 0;
						entity->skill[0] = entity->skill[0] == 0 ? 1 : 0;
					}
					if ( keystatus[SDLK_KP_6] )
					{
						keystatus[SDLK_KP_6] = 0;
						SKULL_CIRCLES = 2;
						SKULL_CIRCLE_ANIM = 0.0;
						SKULL_CIRCLE_SCALE = 0.0;
						SKULL_CIRCLES_DECREMENT_MODE = 0;
					}
					if ( keystatus[SDLK_KP_4] )
					{
						keystatus[SDLK_KP_4] = 0;
						SKULL_CIRCLES = 0;
					}
					if ( keystatus[SDLK_KP_2] )
					{
						keystatus[SDLK_KP_2] = 0;
						SKULL_FLIP = 1.0;

					}
					if ( keystatus[SDLK_KP_3] )
					{
					}
					if ( keystatus[SDLK_KP_1] )
					{
					}
					if ( keystatus[SDLK_KP_0] )
					{
						keystatus[SDLK_KP_0] = 0;
						SKULL_BOBS = 5;
						SKULL_BOB_ANIM = 0.0;
					}
				}

				entity->pitch -= PI * (-1 + sin(PI / 2 + SKULL_FLIP * PI));
				SKULL_FLIP -= 0.05;
				SKULL_FLIP = std::max(0.0, SKULL_FLIP);

				if ( SKULL_CIRCLES > 0 )
				{
					real_t prev = SKULL_CIRCLE_ANIM;
					real_t mult = MONSTER_ATTACK == MONSTER_POSE_MAGIC_WINDUP1 ? 5.0 : 1.0;
					SKULL_CIRCLE_ANIM += limbs[monsterType][8][2] * mult;
					SKULL_CIRCLE_SCALE += limbs[monsterType][8][1] * mult;
					SKULL_CIRCLE_SCALE = std::min(1.0, SKULL_CIRCLE_SCALE);
					if ( prev < PI && SKULL_CIRCLE_ANIM >= PI )
					{
						SKULL_CIRCLES--;
					}
				}
				if ( SKULL_CIRCLES == 0 )
				{
					if ( SKULL_CIRCLES_DECREMENT_MODE == 0 )
					{
						SKULL_CIRCLE_SCALE -= limbs[monsterType][8][1];
						SKULL_CIRCLE_SCALE = std::max(0.0, SKULL_CIRCLE_SCALE);
						if ( SKULL_CIRCLE_ANIM >= PI )
						{
							SKULL_CIRCLE_ANIM += limbs[monsterType][8][2] * 4;
							SKULL_CIRCLE_ANIM = std::min(SKULL_CIRCLE_ANIM, 2 * PI);
						}
						else
						{
							SKULL_CIRCLE_ANIM -= limbs[monsterType][8][2] * 4;
							SKULL_CIRCLE_ANIM = std::max(SKULL_CIRCLE_ANIM, 0.0);
						}
					}
					else
					{
						SKULL_CIRCLE_SCALE -= limbs[monsterType][8][1] * 2;
						SKULL_CIRCLE_SCALE = std::max(0.0, SKULL_CIRCLE_SCALE);

						// spin fast
						if ( SKULL_CIRCLE_SCALE <= 0.0 )
						{
							SKULL_CIRCLE_ANIM = 0.0;
						}
					}
				}
				while ( SKULL_CIRCLE_ANIM >= 2 * PI )
				{
					SKULL_CIRCLE_ANIM -= 2 * PI;
				}
				while ( SKULL_CIRCLE_ANIM < 0.0 )
				{
					SKULL_CIRCLE_ANIM += 2 * PI;
				}
				if ( entity->skill[0] == 0 )
				{
					entity->fskill[1] += 0.1;
				}

				SKULL_FLOAT_X = limbs[monsterType][10][0] * sin(entity->fskill[1] * limbs[monsterType][11][0]) * cos(entity->yaw + PI / 2);
				SKULL_FLOAT_Y = limbs[monsterType][10][1] * sin(entity->fskill[1] * limbs[monsterType][11][1]) * sin(entity->yaw + PI / 2);
				SKULL_FLOAT_Z = limbs[monsterType][10][2] * sin(entity->fskill[1] * limbs[monsterType][11][2]);
				real_t floatAtkZ = SKULL_FLOAT_ATK < 0 ? 2 * sin(SKULL_FLOAT_ATK * PI / 8) : 0.5 * sin(SKULL_FLOAT_ATK * PI / 8);
				SKULL_FLOAT_Z += floatAtkZ;

				SKULL_FLOAT_X += SKULL_FLOAT_ATK * cos(entity->yaw);
				SKULL_FLOAT_Y += SKULL_FLOAT_ATK * sin(entity->yaw);

				{
					SKULL_FLOAT_X += SKULL_CIRCLE_SCALE * limbs[monsterType][8][0] * cos(entity->yaw + SKULL_CIRCLE_ANIM);
					SKULL_FLOAT_Y += SKULL_CIRCLE_SCALE * limbs[monsterType][8][0] * sin(entity->yaw + SKULL_CIRCLE_ANIM);

					if ( SKULL_CIRCLES_DECREMENT_MODE == 1 )
					{
						// spin fast
						entity->yaw += SKULL_CIRCLE_SCALE * (PI / 2 + SKULL_CIRCLE_ANIM);
					}
					else
					{
						// ease to normal
						entity->yaw += SKULL_CIRCLE_SCALE * PI / 2 + SKULL_CIRCLE_ANIM;
					}

					if ( SKULL_BOBS > 0 )
					{
						SKULL_BOB_ANIM += limbs[monsterType][9][0];

						real_t scale = SKULL_BOBS * 0.5;

						SKULL_FLOAT_Z += scale * sin(PI / 4) - scale * sin(SKULL_BOB_ANIM * 2 * PI + PI / 4);

						if ( SKULL_BOB_ANIM >= 1.0 )
						{
							SKULL_BOB_ANIM = 0.0;
							if ( SKULL_NEXTBOB > 0 )
							{
								SKULL_BOBS = SKULL_NEXTBOB;
								SKULL_NEXTBOB = 0;
							}
							else
							{
								--SKULL_BOBS;
							}
						}
					}
				}

				entity->x += SKULL_FLOAT_X;
				entity->y += SKULL_FLOAT_Y;
				entity->z += SKULL_FLOAT_Z;

				if ( monsterType == FLAME_ELEMENTAL )
				{
					entity->roll = (PI / 32) * sin(entity->fskill[1] * limbs[monsterType][11][0]);
					entity->yaw += (PI / 32) * sin(entity->fskill[1] * limbs[monsterType][11][0]);
				}
				else if ( adorcisedWeapon )
				{
					entity->roll = (PI / 32) * sin(entity->fskill[1] * limbs[monsterType][11][0]);
					entity->yaw += (PI / 32) * sin(entity->fskill[1] * limbs[monsterType][11][0]);

					if ( multiplayer != CLIENT )
					{
						if ( myStats->weapon )
						{
							entity->sprite = itemModel(myStats->weapon);
						}
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
				}

				if ( monsterType == FLAME_ELEMENTAL )
				{
					if ( my->ticks % 5 == 0 )
					{
						Entity* fx = spawnMagicParticleCustom(entity, 233, 0.5, 1.0);
						fx->x -= 2.0 * cos(entity->yaw);
						fx->y -= 2.0 * cos(entity->yaw);
						fx->vel_x = 0.1 * cos(entity->yaw + PI);
						fx->vel_y = 0.1 * sin(entity->yaw + PI);
						fx->vel_z = -0.15;
						fx->behavior = &actSprite;
						fx->flags[SPRITE] = true;
						fx->ditheringDisabled = true;
						fx->skill[0] = 1;
						fx->skill[1] = 12;
						fx->skill[2] = 4;
						fx->actSpriteVelXY = 1;
					}

					const real_t squishRate = dist < 0.1 ? 1.5 : 3.0;
					const real_t squishFactor = 0.1;// dist < 0.1 ? 0.05 : 0.3;
					const real_t inc = squishRate * (PI / TICKS_PER_SECOND);
					real_t& slimeBob = entity->fskill[24];
					slimeBob = fmod(slimeBob + inc, PI * 2);
					entity->scalex = 0.9 - sin(slimeBob) * squishFactor;
					entity->scaley = 0.9 - sin(slimeBob) * squishFactor;
					entity->scalez = 0.9 + sin(slimeBob) * squishFactor;
				}
				else
				{
					Entity* fx = spawnMagicParticleCustom(entity, 96, 0.5, 1.0);
					fx->vel_x = 0.25 * cos(entity->yaw + PI);
					fx->vel_y = 0.25 * sin(entity->yaw + PI);
					fx->vel_z = 0.3;
					fx->flags[SPRITE] = true;
					fx->ditheringDisabled = true;
				}
				break;
			}
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

void hologramDie(Entity* my)
{
	my->removeMonsterDeathNodes();
	spawnPoof(my->x, my->y, my->z, 1.0, true);
	list_RemoveNode(my->mynode);
	return;
}

void initHologram(Entity* my, Stat* myStats)
{
	node_t* node;

	my->z = 0;
	my->initMonster(1803);
	my->flags[INVISIBLE] = true; // hide the "AI" bodypart
	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = -1;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = -1;
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

			my->setHardcoreStats(*myStats);
		}
	}

	// body
	for ( int i = 0; i < 30; ++i )
	{
		Entity* entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 2;
		entity->sizey = 2;
		entity->skill[2] = my->getUID();
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[INVISIBLE] = true;
		entity->flags[INVISIBLE_DITHER] = false;
		entity->yaw = my->yaw;
		entity->z = 6;
		entity->flags[USERFLAG2] = my->flags[USERFLAG2];
		entity->focalx = 0.0;
		entity->focaly = 0.0;
		entity->focalz = 0.0;
		entity->behavior = &actHologramLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);
	}

	my->mistformGLRender = 2.0;
}

void hologramAnimate(Entity* my, Stat* myStats, double dist)
{
	my->flags[INVISIBLE] = true; // hide the "AI" bodypart
	my->sizex = 4;
	my->sizey = 4;
	my->mistformGLRender = 2.0;

	Entity* hologramParent = nullptr;
	if ( my->monsterSpecialState != 0 )
	{
		hologramParent = uidToEntity(my->monsterSpecialState);
	}
	if ( multiplayer != CLIENT )
	{
		if ( !hologramParent || (myStats && !myStats->getEffectActive(EFF_MIST_FORM)) )
		{
			my->setHP(0);
			my->setObituary(Language::get(6668));
		}

		my->z = 0.0;
		my->creatureHandleLiftZ();
	}

	int bodypart = 0;
	node_t* node = nullptr;
	std::vector<Entity*> myLimbs;
	for ( bodypart = 0, node = my->children.first; node != nullptr; node = node->next, ++bodypart )
	{
		if ( bodypart < LIMB_HUMANOID_TORSO )
		{
			continue;
		}

		if ( Entity* entity = (Entity*)node->element )
		{
			entity->flags[INVISIBLE] = true;
			entity->flags[INVISIBLE_DITHER] = false;
			entity->mistformGLRender = my->mistformGLRender;
			myLimbs.push_back(entity);
		}
	}

	if ( hologramParent && (hologramParent->behavior == &actMonster || hologramParent->behavior == &actPlayer) )
	{
		int parentOffset = 0;
		if ( hologramParent->behavior == &actPlayer )
		{
			parentOffset = -1;
		}
		std::vector<Entity*> limbsCopy;
		limbsCopy.push_back(hologramParent);
		
		int listSize = list_Size(&hologramParent->children);
		for ( int i = LIMB_HUMANOID_TORSO + parentOffset; i < listSize; ++i )
		{
			if ( node_t* nodeCopy = list_Node(&hologramParent->children, i) )
			{
				if ( Entity* limb = (Entity*)nodeCopy->element )
				{
					limbsCopy.push_back(limb);
				}
			}
		}

		int index = -1;
		Entity* firstLimb = nullptr;
		for ( auto entity : myLimbs )
		{
			++index;
			if ( index == 0 )
			{
				firstLimb = entity;
				firstLimb->fskill[0] += 0.05;
			}
			if ( index < limbsCopy.size() )
			{
				Entity* limb = limbsCopy.at(index);
				entity->sprite = limb->sprite;
				entity->flags[INVISIBLE] = limb->flags[INVISIBLE];
				entity->flags[INVISIBLE_DITHER] = limb->flags[INVISIBLE_DITHER];
				entity->yaw = limb->yaw + firstLimb->fskill[0];
				entity->pitch = limb->pitch;
				entity->roll = limb->roll;
				if ( index == 0 )
				{
					entity->x = my->x;
					entity->y = my->y;
				}
				else
				{
					real_t x = (hologramParent->x - limb->x);
					real_t y = (hologramParent->y - limb->y);
					real_t tangent = atan2(y, x);
					real_t length = sqrt(x * x + y * y);
					tangent += firstLimb->fskill[0];
					x = length * cos(tangent);
					y = length * sin(tangent);

					entity->x = my->x - (hologramParent->x - limb->x);
					entity->y = my->y - (hologramParent->y - limb->y);
					entity->x = my->x - x;
					entity->y = my->y - y;
				}
				entity->z = limb->z;
				entity->focalx = limb->focalx;
				entity->focaly = limb->focaly;
				entity->focalz = limb->focalz;
				entity->scalex = limb->scalex;
				entity->scaley = limb->scaley;
				entity->scalez = limb->scalez;
				entity->sizex = limb->sizex;
				entity->sizey = limb->sizey;


			}
		}
	}
}