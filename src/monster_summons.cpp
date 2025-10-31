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
	my->flags[BURNABLE] = false;
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
	my->flags[BURNABLE] = false;
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

			if ( myStats->weapon == nullptr && myStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] == 1 )
			{
				int pick = rng.rand() % 8;
				switch ( pick )
				{
				case 0:
				case 1:
					myStats->weapon = newItem(IRON_SWORD, static_cast<Status>(DECREPIT + rng.rand() % 4), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
					break;
				case 2:
				case 3:
					myStats->weapon = newItem(IRON_SPEAR, static_cast<Status>(DECREPIT + rng.rand() % 4), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
					break;
				case 4:
				case 5:
					myStats->weapon = newItem(IRON_MACE, static_cast<Status>(DECREPIT + rng.rand() % 4), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
					break;
				case 6:
				case 7:
					myStats->weapon = newItem(IRON_AXE, static_cast<Status>(DECREPIT + rng.rand() % 4), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
					break;
				case 8:
				case 9:
					myStats->weapon = newItem(MAGICSTAFF_FIRE, static_cast<Status>(DECREPIT + rng.rand() % 4), -1 + rng.rand() % 3, 1, rng.rand(), false, nullptr);
					break;
				}
			}
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
	my->flags[BURNABLE] = false;
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

	if ( myStats && myStats->MP > 0 )
	{
		int damage = myStats->LVL;
		Entity* caster = uidToEntity(myStats->leader_uid);
		damage += getSpellDamageFromID(SPELL_FIREBALL, caster, nullptr, caster);
		real_t radius = getSpellEffectDurationFromID(SPELL_FLAME_ELEMENTAL, caster, nullptr, caster);
		createSpellExplosionArea(SPELL_FIREBALL, caster, my->x, my->y, 0.0, radius, damage, my);
	}

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

		if ( monsterType == REVENANT_SKULL && myStats )
		{
			if ( myStats->getAttribute("revenant_skull") != "" )
			{
				if ( my->parent != 0 )
				{
					Entity* parent = uidToEntity(my->parent);
					if ( !parent )
					{
						my->setHP(0);
						my->setObituary(Language::get(6806));
					}
				}

				int lifetime = std::stoi(myStats->getAttribute("revenant_skull"));
				--lifetime;
				if ( lifetime <= 0 )
				{
					my->setHP(0);
					my->setObituary(Language::get(6806));
				}
				else
				{
					myStats->setAttribute("revenant_skull", std::to_string(lifetime));
				}
			}
		}
		else if ( monsterType == MONSTER_ADORCISED_WEAPON && myStats )
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
	my->flags[BURNABLE] = false;
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

void actEarthElementalDeathGib(Entity* my)
{
	if ( my->skill[0] == 0 )
	{
		my->z += 0.05;
		if ( my->z >= 7.5 )
		{
			my->scalex -= 0.05;
			my->scaley -= 0.05;
			my->scalez -= 0.05;
			if ( my->scalex <= 0.0 )
			{
				list_RemoveNode(my->mynode);
				return;
			}
		}
	}
	else
	{
		if ( my->skill[0] > 0 )
		{
			--my->skill[0];
		}

		my->scalex = std::min(my->scalex + 0.0125, 0.5);
		my->scaley = std::min(my->scaley + 0.0125, 0.5);
		my->scalez = std::min(my->scalez + 0.0125, 0.5);
	}
}

void earthElementalDie(Entity* my)
{
	int index = -1;
	for ( auto bodypart : my->bodyparts )
	{
		++index;
		if ( index == 1 ) // eyes
		{
			continue;
		}
		Entity* entity = spawnGib(my, bodypart->sprite);
		entity->x = bodypart->x;
		entity->y = bodypart->y;
		entity->z = bodypart->z;
		if ( index == 0 )
		{
			entity->skill[5] = 1; // poof
		}
		entity->vel_x *= 0.1;
		entity->vel_y *= 0.1;
		serverSpawnGibForClient(entity);
	}

	Entity* spellTimer = createParticleTimer(nullptr, TICKS_PER_SECOND, -1);
	spellTimer->x = my->x;
	spellTimer->y = my->y;
	spellTimer->z = my->z;
	spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_EARTH_ELEMENTAL_DIE;
	serverSpawnMiscParticlesAtLocation(spellTimer->x, spellTimer->y, spellTimer->z, PARTICLE_EFFECT_EARTH_ELEMENTAL_DIE, 0);

	my->removeMonsterDeathNodes();
	//spawnPoof(my->x, my->y, my->z, 1.0, true);
	list_RemoveNode(my->mynode);
	return;
}

void actEarthElementalLimb(Entity* my)
{
	my->actMonsterLimb(false);
}

void initEarthElemental(Entity* my, Stat* myStats)
{
	node_t* node;

	my->z = 0;

	int sprite = 1871; // default sprite, summon anim
	if ( multiplayer != CLIENT )
	{
		if ( MONSTER_INIT )
		{
			sprite = 1876; // no summon anim
		}
	}
	my->initMonster(sprite);
	my->flags[BURNABLE] = false;
	my->flags[INVISIBLE] = true; // hide the "AI" bodypart
	my->flags[PASSABLE] = true;
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

			if ( myStats->getAttribute("SUMMONED_CREATURE") == "1" )
			{
				// min 5, max 20
				myStats->HP = 40 + std::max(0, (myStats->LVL - 5)) * 5; //40 - 115
				myStats->MAXHP = myStats->HP;
				myStats->OLDHP = myStats->HP;
				myStats->STR = 5 + std::max(0, (myStats->LVL - 5)) * 2; //5-35
				myStats->DEX = myStats->LVL / 5; // 1-5
				myStats->CON = 5 + myStats->LVL; // 10-25
				myStats->PER = 5 + myStats->LVL / 4; // 6-10
			}
		}
	}

	// body
	Entity* entity = newEntity(1872, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->z = 6;
	entity->flags[USERFLAG2] = monsterChangesColorWhenAlly(myStats, my) ? my->flags[USERFLAG2] : false;
	entity->focalx = limbs[EARTH_ELEMENTAL][3][0];
	entity->focaly = limbs[EARTH_ELEMENTAL][3][1];
	entity->focalz = limbs[EARTH_ELEMENTAL][3][2];
	entity->behavior = &actEarthElementalLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// eyes
	entity = newEntity(1873, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->z = 6;
	entity->flags[USERFLAG2] = monsterChangesColorWhenAlly(myStats, my) ? my->flags[USERFLAG2] : false;
	entity->focalx = limbs[EARTH_ELEMENTAL][1][0];
	entity->focaly = limbs[EARTH_ELEMENTAL][1][1];
	entity->focalz = limbs[EARTH_ELEMENTAL][1][2];
	entity->behavior = &actEarthElementalLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	//fistleft
	entity = newEntity(1875, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->z = 6;
	entity->flags[USERFLAG2] = monsterChangesColorWhenAlly(myStats, my) ? my->flags[USERFLAG2] : false;
	entity->focalx = limbs[EARTH_ELEMENTAL][6][0];
	entity->focaly = limbs[EARTH_ELEMENTAL][6][1];
	entity->focalz = limbs[EARTH_ELEMENTAL][6][2];
	entity->behavior = &actEarthElementalLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	//fistright
	entity = newEntity(1875, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->z = 6;
	entity->flags[USERFLAG2] = monsterChangesColorWhenAlly(myStats, my) ? my->flags[USERFLAG2] : false;
	entity->focalx = limbs[EARTH_ELEMENTAL][6][0];
	entity->focaly = limbs[EARTH_ELEMENTAL][6][1];
	entity->focalz = limbs[EARTH_ELEMENTAL][6][2];
	entity->behavior = &actEarthElementalLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	//pebble1
	entity = newEntity(1874, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->z = 6;
	entity->flags[USERFLAG2] = monsterChangesColorWhenAlly(myStats, my) ? my->flags[USERFLAG2] : false;
	entity->focalx = limbs[EARTH_ELEMENTAL][9][0];
	entity->focaly = limbs[EARTH_ELEMENTAL][9][1];
	entity->focalz = limbs[EARTH_ELEMENTAL][9][2];
	entity->behavior = &actEarthElementalLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	//pebble1
	entity = newEntity(1874, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->z = 6;
	entity->flags[USERFLAG2] = monsterChangesColorWhenAlly(myStats, my) ? my->flags[USERFLAG2] : false;
	entity->focalx = limbs[EARTH_ELEMENTAL][9][0];
	entity->focaly = limbs[EARTH_ELEMENTAL][9][1];
	entity->focalz = limbs[EARTH_ELEMENTAL][9][2];
	entity->behavior = &actEarthElementalLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	//pebble1
	entity = newEntity(1874, 1, map.entities, nullptr); //Limb entity.
	entity->sizex = 2;
	entity->sizey = 2;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->z = 6;
	entity->flags[USERFLAG2] = monsterChangesColorWhenAlly(myStats, my) ? my->flags[USERFLAG2] : false;
	entity->focalx = limbs[EARTH_ELEMENTAL][9][0];
	entity->focaly = limbs[EARTH_ELEMENTAL][9][1];
	entity->focalz = limbs[EARTH_ELEMENTAL][9][2];
	entity->behavior = &actEarthElementalLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);
}

#define EARTH_BODY 2
#define EARTH_EYES 3
#define EARTH_LEFTARM 4
#define EARTH_RIGHTARM 5
#define EARTH_PEBBLE1 6
#define EARTH_PEBBLE2 7
#define EARTH_PEBBLE3 8
#define EARTH_LIMB_FSKILL_YAW entity->fskill[0]
#define EARTH_LIMB_FSKILL_PITCH entity->fskill[1]
#define EARTH_LIMB_FSKILL_ROLL entity->fskill[2]
#define EARTH_FLOAT_X body->fskill[3]
#define EARTH_FLOAT_Y body->fskill[4]
#define EARTH_FLOAT_Z body->fskill[5]
#define EARTH_FLOAT_ANIM body->fskill[6]
#define EARTH_PEBBLE_IDLE_ANIM entity->fskill[3]
#define EARTH_ATTACK_1 body->fskill[7]
#define EARTH_ATTACK_FLOAT body->fskill[8]
#define EARTH_ATTACK_2 body->fskill[9]
#define EARTH_ATTACK_3 body->fskill[10]
#define EARTH_DEFEND body->fskill[11]
#define EARTH_SPAWN_STATE body->skill[0]
#define EARTH_SPAWN_ANIM body->fskill[12]
#define EARTH_SPAWN_ANIM2 body->fskill[13]

void earthElementalAnimate(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = nullptr;
	Entity* head = nullptr;
	int bodypart;

	my->flags[INVISIBLE] = true; // hide the "AI" bodypart
	//my->flags[PASSABLE] = true;

	my->sizex = 4;
	my->sizey = 4;

	my->focalx = limbs[EARTH_ELEMENTAL][0][0];
	my->focaly = limbs[EARTH_ELEMENTAL][0][1];
	my->focalz = limbs[EARTH_ELEMENTAL][0][2];
	if ( multiplayer != CLIENT )
	{
		my->z = limbs[EARTH_ELEMENTAL][5][2];
		if ( !myStats->getEffectActive(EFF_LEVITATING) )
		{
			myStats->setEffectActive(EFF_LEVITATING, 1);
			myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 0;
		}

		my->creatureHandleLiftZ();
	}

	//my->setEffect(EFF_STUNNED, true, -1, false);
	//my->monsterLookDir = 0.0;
	//my->yaw = 0.0;
	//static ConsoleVariable<int> cvar_ee_yaw("/ee_yaw", 0);
	//static ConsoleVariable<int> cvar_ee_pitch("/ee_pitch", 0);
	//static ConsoleVariable<int> cvar_ee_roll("/ee_roll", 0);
	if ( enableDebugKeys && (svFlags & SV_FLAG_CHEATS) )
	{
		if ( keystatus[SDLK_KP_5] )
		{
			my->yaw += 0.05;
			my->monsterLookDir = my->yaw;
		}
		if ( keystatus[SDLK_KP_4] )
		{
			my->setEffect(EFF_STUNNED, true, -1, false);
		}
	}
	if ( keystatus[SDLK_g] )
	{
		keystatus[SDLK_g] = 0;
		//MONSTER_ATTACK = mothGetAttackPose(my, MONSTER_POSE_MELEE_WINDUP1);
		MONSTER_ATTACK = MONSTER_POSE_MELEE_WINDUP1;// mothGetAttackPose(my, MONSTER_POSE_MAGIC_WINDUP1);
		MONSTER_ATTACKTIME = 0;
	}
	if ( keystatus[SDLK_h] )
	{
		keystatus[SDLK_h] = 0;
		//MONSTER_ATTACK = mothGetAttackPose(my, MONSTER_POSE_MELEE_WINDUP1);
		MONSTER_ATTACK = MONSTER_POSE_MELEE_WINDUP3;// mothGetAttackPose(my, MONSTER_POSE_MAGIC_WINDUP1);
		MONSTER_ATTACKTIME = 0;
	}
	if ( keystatus[SDLK_n] )
	{
		keystatus[SDLK_n] = 0;
		//MONSTER_ATTACK = mothGetAttackPose(my, MONSTER_POSE_MELEE_WINDUP1);
		MONSTER_ATTACK = MONSTER_POSE_RANGED_WINDUP1;// mothGetAttackPose(my, MONSTER_POSE_MAGIC_WINDUP1);
		MONSTER_ATTACKTIME = 0;
	}
	if ( keystatus[SDLK_y] )
	{
		keystatus[SDLK_y] = 0;
		//MONSTER_ATTACK = mothGetAttackPose(my, MONSTER_POSE_MELEE_WINDUP1);
		MONSTER_ATTACK = MONSTER_POSE_MELEE_WINDUP2;// mothGetAttackPose(my, MONSTER_POSE_MAGIC_WINDUP1);
		MONSTER_ATTACKTIME = 0;
	}

	//	if ( keystatus[SDLK_h] )
	//	{
	//		keystatus[SDLK_h] = 0;
	//		myStats->setEffectValueUnsafe(EFF_STUNNED, myStats->getEffectActive(EFF_STUNNED) ? 0 : 1);
	//		myStats->EFFECTS_TIMERS[EFF_STUNNED] = myStats->getEffectActive(EFF_STUNNED) ? -1 : 0;
	//	}
	//}

	//Move bodyparts
	Entity* body = nullptr;
	for ( bodypart = 0, node = my->children.first; node != nullptr; node = node->next, ++bodypart )
	{
		if ( bodypart < EARTH_BODY )
		{
			continue;
		}

		entity = (Entity*)node->element;

		if ( bodypart == EARTH_BODY )
		{
			body = entity;

			if ( my->sprite == 1876 )
			{
				// skip spawn anim
				EARTH_SPAWN_ANIM = 0.0;
				EARTH_SPAWN_STATE = 2;
				EARTH_SPAWN_ANIM2 = 0.0;
			}
			if ( multiplayer != CLIENT )
			{
				if ( EARTH_SPAWN_STATE < 2 )
				{
					my->flags[PASSABLE] = true;
					my->setEffect(EFF_STUNNED, true, 25, false);
				}
				else
				{
					my->flags[PASSABLE] = false;
				}
			}
			//if ( keystatus[SDLK_u] )
			//{
			//	keystatus[SDLK_u] = 0;
			//	//MONSTER_ATTACK = mothGetAttackPose(my, MONSTER_POSE_MELEE_WINDUP1);
			//	my->monsterDefend = my->monsterDefend ? 0 : 1;
			//	//EARTH_SPAWN_ANIM = 0.0;
			//	//EARTH_SPAWN_STATE = 0;
			//	//EARTH_SPAWN_ANIM2 = 0.0;
			//}
		}

		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		entity->yaw = my->yaw;

		entity->pitch = 0.0;
		entity->roll = 0.0;
		/*if ( *cvar_ee_yaw == bodypart )
		{
			EARTH_LIMB_FSKILL_YAW += 0.05;
		}*/
		entity->yaw += EARTH_LIMB_FSKILL_YAW;
		/*if ( *cvar_ee_pitch == bodypart )
		{
			EARTH_LIMB_FSKILL_PITCH += 0.05;
		}*/
		entity->pitch = EARTH_LIMB_FSKILL_PITCH;
		/*if ( *cvar_ee_roll == bodypart )
		{
			EARTH_LIMB_FSKILL_ROLL += 0.05;
		}*/
		entity->roll = EARTH_LIMB_FSKILL_ROLL;

		if ( bodypart == EARTH_BODY )
		{
			if ( MONSTER_ATTACK == 0 || MONSTER_ATTACK == MONSTER_POSE_EARTH_ELEMENTAL_ROLL )
			{
				{
					real_t setpoint = 0.0;
					if ( EARTH_DEFEND > 0.01 )
					{
						setpoint = EARTH_DEFEND * PI / 16;
					}
					else
					{
						setpoint = -PI / 8;
					}

					if ( EARTH_ATTACK_1 > setpoint )
					{
						EARTH_ATTACK_1 -= limbs[EARTH_ELEMENTAL][16][0] / 2;
						EARTH_ATTACK_1 = std::max(setpoint, EARTH_ATTACK_1);
					}
					else if ( EARTH_ATTACK_1 < setpoint )
					{
						EARTH_ATTACK_1 += limbs[EARTH_ELEMENTAL][16][0] / 2;
						EARTH_ATTACK_1 = std::min(setpoint, EARTH_ATTACK_1);
					}
				}

				if ( EARTH_ATTACK_2 > 0.0 )
				{
					EARTH_ATTACK_2 -= limbs[EARTH_ELEMENTAL][16][0];
					EARTH_ATTACK_2 = std::max(0.0, EARTH_ATTACK_2);
				}
				else if ( EARTH_ATTACK_2 < 0.0 )
				{
					EARTH_ATTACK_2 += limbs[EARTH_ELEMENTAL][16][0];
					EARTH_ATTACK_2 = std::min(0.0, EARTH_ATTACK_2);
				}

				if ( EARTH_ATTACK_3 > 0.0 )
				{
					EARTH_ATTACK_3 -= limbs[EARTH_ELEMENTAL][16][0];
					EARTH_ATTACK_3 = std::max(0.0, EARTH_ATTACK_3);
				}
				else if ( EARTH_ATTACK_3 < 0.0 )
				{
					EARTH_ATTACK_3 += limbs[EARTH_ELEMENTAL][16][0];
					EARTH_ATTACK_3 = std::min(0.0, EARTH_ATTACK_3);
				}
			}

			if ( MONSTER_ATTACK == MONSTER_POSE_EARTH_ELEMENTAL_ROLL )
			{
				if ( MONSTER_ATTACKTIME == 0 )
				{
					EARTH_LIMB_FSKILL_YAW = 0.0;
					if ( multiplayer != CLIENT )
					{
						Entity* spellTimer = createParticleTimer(my, 35, -1);
						spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_EARTH_ELEMENTAL_ROLL;
					}
				}
				else 
				{
					EARTH_LIMB_FSKILL_YAW += -0.3;
					EARTH_LIMB_FSKILL_YAW = std::max(EARTH_LIMB_FSKILL_YAW, -4 * PI);

					if ( MONSTER_ATTACKTIME >= 40 )
					{
						EARTH_LIMB_FSKILL_YAW = 0.0;
						MONSTER_ATTACK = 0;
					}
				}
			}

			if ( !my->monsterDefend || MONSTER_ATTACK != 0 )
			{
				if ( EARTH_DEFEND > 0.0 )
				{
					EARTH_DEFEND -= limbs[EARTH_ELEMENTAL][16][0] * 3;
					EARTH_DEFEND = std::max(0.0, EARTH_DEFEND);
				}
				else if ( EARTH_DEFEND < 0.0 )
				{
					EARTH_DEFEND += limbs[EARTH_ELEMENTAL][16][0] * 3;
					EARTH_DEFEND = std::min(0.0, EARTH_DEFEND);
				}
			}
			else
			{
				EARTH_DEFEND += limbs[EARTH_ELEMENTAL][16][0];
				EARTH_DEFEND = std::min(1.0, EARTH_DEFEND);
			}

			if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP1 )
			{
				if ( MONSTER_ATTACKTIME == 0 )
				{
					EARTH_ATTACK_1 = 0.0;
					EARTH_ATTACK_2 = 0.0;
					EARTH_ATTACK_3 = 0.0;
					EARTH_LIMB_FSKILL_YAW = 0.0;
				}
				else
				{
					if ( MONSTER_ATTACKTIME >= limbs[EARTH_ELEMENTAL][15][0] )
					{
						//EARTH_ATTACK_1 += limbs[EARTH_ELEMENTAL][16][2];
						EARTH_LIMB_FSKILL_YAW += limbs[EARTH_ELEMENTAL][16][2];
						EARTH_LIMB_FSKILL_YAW = std::max(EARTH_LIMB_FSKILL_YAW, -4 * PI);

						EARTH_ATTACK_1 -= limbs[EARTH_ELEMENTAL][16][0];
						EARTH_ATTACK_1 = std::max(EARTH_ATTACK_1, -(real_t)limbs[EARTH_ELEMENTAL][16][1]);
					}
					else
					{
						EARTH_ATTACK_1 += limbs[EARTH_ELEMENTAL][16][0];
						EARTH_ATTACK_1 = std::min(EARTH_ATTACK_1, (real_t)limbs[EARTH_ELEMENTAL][16][1]);
					}

					if ( MONSTER_ATTACKTIME >= 55 )
					{
						MONSTER_ATTACK = 0;
						EARTH_LIMB_FSKILL_YAW = 0.0;
					}
					else
					{
						if ( MONSTER_ATTACKTIME == 20 
							|| MONSTER_ATTACKTIME == 35
							|| MONSTER_ATTACKTIME == 50 )
						{
							if ( multiplayer != CLIENT )
							{
								const Sint32 temp = MONSTER_ATTACKTIME;
								const Sint32 temp2 = MONSTER_ATTACK;
								my->attack(1, 0, nullptr); // slop
								MONSTER_ATTACKTIME = temp;
								MONSTER_ATTACK = temp2;
							}
						}
					}
				}
			}
			else if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP2 )
			{
				if ( MONSTER_ATTACKTIME == 0 )
				{
					EARTH_ATTACK_1 = 0.0;
					EARTH_ATTACK_2 = 0.0;
					EARTH_ATTACK_3 = 0.0;
					EARTH_LIMB_FSKILL_YAW = 0.0;
				}
				else
				{
					if ( MONSTER_ATTACKTIME >= limbs[EARTH_ELEMENTAL][18][2] )
					{
						EARTH_ATTACK_3 = std::min(1.0, EARTH_ATTACK_3 + limbs[EARTH_ELEMENTAL][13][1]);
						EARTH_ATTACK_2 = std::max(0.0, EARTH_ATTACK_2 - limbs[EARTH_ELEMENTAL][16][0]);
						EARTH_ATTACK_1 -= limbs[EARTH_ELEMENTAL][16][0] * 4;
						EARTH_ATTACK_1 = std::max(EARTH_ATTACK_1, -(real_t)limbs[EARTH_ELEMENTAL][16][1]);
					}
					else if ( MONSTER_ATTACKTIME >= limbs[EARTH_ELEMENTAL][18][0] )
					{
						EARTH_ATTACK_2 = std::min(1.0, EARTH_ATTACK_2 + limbs[EARTH_ELEMENTAL][13][1]);
						EARTH_ATTACK_3 = std::max(-1.0, EARTH_ATTACK_3 - 0.1);
						EARTH_ATTACK_1 += limbs[EARTH_ELEMENTAL][16][0] * 3;
						EARTH_ATTACK_1 = std::min(EARTH_ATTACK_1, (real_t)limbs[EARTH_ELEMENTAL][16][1]);
					}
					else
					{
						EARTH_ATTACK_2 = std::max(-1.0, EARTH_ATTACK_2 - 0.1);
						EARTH_ATTACK_1 -= limbs[EARTH_ELEMENTAL][16][0];
						EARTH_ATTACK_1 = std::max(EARTH_ATTACK_1, -(real_t)limbs[EARTH_ELEMENTAL][16][1]);
					}

					if ( MONSTER_ATTACKTIME >= 55 )
					{
						MONSTER_ATTACK = 0;
						EARTH_LIMB_FSKILL_YAW = 0.0;
					}
					else
					{
						if ( MONSTER_ATTACKTIME == 20 || MONSTER_ATTACKTIME == 35 )
						{
							if ( multiplayer != CLIENT )
							{
								const Sint32 temp = MONSTER_ATTACKTIME;
								const Sint32 temp2 = MONSTER_ATTACK;
								my->attack(1, 0, nullptr); // slop
								MONSTER_ATTACKTIME = temp;
								MONSTER_ATTACK = temp2;
							}
						}
					}
				}
			}
			else if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP3 )
			{
				if ( MONSTER_ATTACKTIME == 0 )
				{
					EARTH_ATTACK_1 = 0.0;
					EARTH_ATTACK_2 = 0.0;
					EARTH_ATTACK_3 = 0.0;
					EARTH_LIMB_FSKILL_YAW = 0.0;
				}
				else
				{
					if ( MONSTER_ATTACKTIME >= limbs[EARTH_ELEMENTAL][18][2] )
					{
						//EARTH_ATTACK_3 = std::min(1.0, EARTH_ATTACK_3 + limbs[EARTH_ELEMENTAL][13][1]);
						EARTH_ATTACK_2 = std::max(0.0, EARTH_ATTACK_2 - limbs[EARTH_ELEMENTAL][16][0]);
						EARTH_ATTACK_1 -= limbs[EARTH_ELEMENTAL][16][0] * 1;
						EARTH_ATTACK_1 = std::max(EARTH_ATTACK_1, -(real_t)limbs[EARTH_ELEMENTAL][16][1] / 3);
					}
					else if ( MONSTER_ATTACKTIME >= limbs[EARTH_ELEMENTAL][18][0] )
					{
						EARTH_ATTACK_2 = std::min(1.0, EARTH_ATTACK_2 + limbs[EARTH_ELEMENTAL][13][1]);
						EARTH_ATTACK_1 += limbs[EARTH_ELEMENTAL][16][0] * 3;
						EARTH_ATTACK_1 = std::min(EARTH_ATTACK_1, (real_t)limbs[EARTH_ELEMENTAL][16][1]);
					}
					else
					{
						EARTH_ATTACK_2 = std::max(-1.0, EARTH_ATTACK_2 - 0.1);
						EARTH_ATTACK_1 -= limbs[EARTH_ELEMENTAL][16][0];
						EARTH_ATTACK_1 = std::max(EARTH_ATTACK_1, -(real_t)limbs[EARTH_ELEMENTAL][16][1]);
					}

					if ( MONSTER_ATTACKTIME >= 40 )
					{
						MONSTER_ATTACK = 0;
						EARTH_LIMB_FSKILL_YAW = 0.0;
					}
					else
					{
						if ( MONSTER_ATTACKTIME == 20 )
						{
							if ( multiplayer != CLIENT )
							{
								const Sint32 temp = MONSTER_ATTACKTIME;
								const Sint32 temp2 = MONSTER_ATTACK;
								my->attack(1, 0, nullptr); // slop
								MONSTER_ATTACKTIME = temp;
								MONSTER_ATTACK = temp2;
							}
						}
					}
				}
			}
			else if ( MONSTER_ATTACK == MONSTER_POSE_RANGED_WINDUP1 )
			{
				if ( MONSTER_ATTACKTIME == 0 )
				{
					EARTH_ATTACK_1 = 0.0;
					EARTH_ATTACK_2 = 0.0;
					EARTH_ATTACK_3 = 0.0;
					EARTH_LIMB_FSKILL_YAW = 0.0;
				}
				else
				{
					if ( MONSTER_ATTACKTIME >= limbs[EARTH_ELEMENTAL][18][2] )
					{
						//EARTH_ATTACK_3 = std::min(1.0, EARTH_ATTACK_3 + limbs[EARTH_ELEMENTAL][13][1]);
						EARTH_ATTACK_3 = std::max(0.0, EARTH_ATTACK_3 - limbs[EARTH_ELEMENTAL][16][0]);
						EARTH_ATTACK_1 += limbs[EARTH_ELEMENTAL][16][0] * 1;
						EARTH_ATTACK_1 = std::min(EARTH_ATTACK_1, (real_t)limbs[EARTH_ELEMENTAL][16][1] / 3);
					}
					else if ( MONSTER_ATTACKTIME >= limbs[EARTH_ELEMENTAL][18][0] )
					{
						EARTH_ATTACK_3 = std::min(1.0, EARTH_ATTACK_3 + limbs[EARTH_ELEMENTAL][13][1]);
						EARTH_ATTACK_1 -= limbs[EARTH_ELEMENTAL][16][0] * 3;
						EARTH_ATTACK_1 = std::max(EARTH_ATTACK_1, -(real_t)limbs[EARTH_ELEMENTAL][16][1]);
					}
					else
					{
						EARTH_ATTACK_3 = std::max(-1.0, EARTH_ATTACK_3 - 0.1);
						EARTH_ATTACK_1 += limbs[EARTH_ELEMENTAL][16][0];
						EARTH_ATTACK_1 = std::min(EARTH_ATTACK_1, (real_t)limbs[EARTH_ELEMENTAL][16][1]);
					}

					if ( MONSTER_ATTACKTIME >= 40 )
					{
						MONSTER_ATTACK = 0;
						EARTH_LIMB_FSKILL_YAW = 0.0;
					}
					else
					{
						if ( MONSTER_ATTACKTIME == 20 )
						{
							if ( multiplayer != CLIENT )
							{
								const Sint32 temp = MONSTER_ATTACKTIME;
								const Sint32 temp2 = MONSTER_ATTACK;
								my->attack(1, 0, nullptr); // slop
								MONSTER_ATTACKTIME = temp;
								MONSTER_ATTACK = temp2;
							}
						}
					}
				}
			}
		}

		switch ( bodypart )
		{
		case EARTH_BODY:
		{
			entity->x += limbs[EARTH_ELEMENTAL][4][0] * cos(my->yaw) + limbs[EARTH_ELEMENTAL][4][1] * cos(my->yaw + PI / 2);
			entity->y += limbs[EARTH_ELEMENTAL][4][0] * sin(my->yaw) + limbs[EARTH_ELEMENTAL][4][1] * sin(my->yaw + PI / 2);
			entity->z += limbs[EARTH_ELEMENTAL][4][2];
			entity->focalx = limbs[EARTH_ELEMENTAL][3][0];
			entity->focaly = limbs[EARTH_ELEMENTAL][3][1];
			entity->focalz = limbs[EARTH_ELEMENTAL][3][2];

			if ( EARTH_SPAWN_STATE >= 0 && EARTH_SPAWN_STATE <= 1 )
			{
				EARTH_FLOAT_X = 0.0;
				EARTH_FLOAT_Y = 0.0;

				real_t targetZ = 8.0;
				real_t startZ = 64.0;
				EARTH_FLOAT_Z = (targetZ - startZ) + EARTH_SPAWN_ANIM;
				if ( EARTH_SPAWN_STATE == 0 )
				{
					EARTH_SPAWN_ANIM2 = std::min(EARTH_SPAWN_ANIM2 + .1, 3.0);
				}
				else
				{
					EARTH_SPAWN_ANIM2 = std::min(EARTH_SPAWN_ANIM2 + .04, 3.0);
				}
				EARTH_SPAWN_ANIM = std::min(startZ, EARTH_SPAWN_ANIM + EARTH_SPAWN_ANIM2);

				EARTH_LIMB_FSKILL_PITCH += 0.04;
				EARTH_LIMB_FSKILL_ROLL += 0.04;

				if ( EARTH_SPAWN_ANIM >= startZ )
				{
					EARTH_SPAWN_STATE += 1; // bounce
					EARTH_SPAWN_ANIM2 = -(EARTH_SPAWN_ANIM2 / 4);
					if ( EARTH_SPAWN_STATE == 1 )
					{
						if ( multiplayer != CLIENT && myStats )
						{
							int damage = getSpellEffectDurationFromID(SPELL_EARTH_ELEMENTAL, my, nullptr, my);
							damage += statGetCON(myStats, my);
							createSpellExplosionArea(SPELL_EARTH_ELEMENTAL, my, my->x, my->y, my->z, 16.0, damage, my);
						}
					}
					else if ( EARTH_SPAWN_STATE == 2 )
					{
						EARTH_SPAWN_ANIM = 1.0;
						EARTH_SPAWN_ANIM2 = 0.0;

						EARTH_LIMB_FSKILL_PITCH = fmod(EARTH_LIMB_FSKILL_PITCH, 2 * PI);
						while ( EARTH_LIMB_FSKILL_PITCH >= 2 * PI )
						{
							EARTH_LIMB_FSKILL_PITCH -= 2 * PI;
						}
						while ( EARTH_LIMB_FSKILL_PITCH < 0 )
						{
							EARTH_LIMB_FSKILL_PITCH += 2 * PI;
						}
						EARTH_LIMB_FSKILL_ROLL = fmod(EARTH_LIMB_FSKILL_ROLL, 2 * PI);
						while ( EARTH_LIMB_FSKILL_ROLL >= 2 * PI )
						{
							EARTH_LIMB_FSKILL_ROLL -= 2 * PI;
						}
						while ( EARTH_LIMB_FSKILL_ROLL < 0 )
						{
							EARTH_LIMB_FSKILL_ROLL += 2 * PI;
						}
					}
				}

				entity->x += 2.0 * cos(my->yaw);
				entity->y += 2.0 * sin(my->yaw);
			}
			else
			{
				EARTH_SPAWN_ANIM = std::max(0.0, EARTH_SPAWN_ANIM - 0.05);
				entity->x += EARTH_SPAWN_ANIM * 2.0 * cos(my->yaw);
				entity->y += EARTH_SPAWN_ANIM * 2.0 * sin(my->yaw);
				if ( EARTH_LIMB_FSKILL_ROLL > PI )
				{
					real_t diff = std::max(0.05, (2 * PI - EARTH_LIMB_FSKILL_ROLL) / 10);
					EARTH_LIMB_FSKILL_ROLL = std::min(2 * PI, EARTH_LIMB_FSKILL_ROLL + diff);
				}
				else
				{
					real_t diff = std::max(0.05, (EARTH_LIMB_FSKILL_ROLL) / 10);
					EARTH_LIMB_FSKILL_ROLL = std::max(0.0, EARTH_LIMB_FSKILL_ROLL - diff);
				}

				EARTH_LIMB_FSKILL_PITCH = fmod(EARTH_LIMB_FSKILL_PITCH, 2 * PI);
				while ( EARTH_LIMB_FSKILL_PITCH >= 2 * PI )
				{
					EARTH_LIMB_FSKILL_PITCH -= 2 * PI;
				}
				while ( EARTH_LIMB_FSKILL_PITCH < 0 )
				{
					EARTH_LIMB_FSKILL_PITCH += 2 * PI;
				}
				if ( EARTH_LIMB_FSKILL_PITCH > PI )
				{
					real_t diff = std::max(0.05, (2 * PI - EARTH_LIMB_FSKILL_PITCH) / 10);
					EARTH_LIMB_FSKILL_PITCH = std::min(2 * PI, EARTH_LIMB_FSKILL_PITCH + diff);
				}
				else
				{
					real_t diff = std::max(0.05, (EARTH_LIMB_FSKILL_PITCH) / 10);
					EARTH_LIMB_FSKILL_PITCH = std::max(0.0, EARTH_LIMB_FSKILL_PITCH - diff);
				}

				real_t spawnRate = std::max(0.0, (1.0 - EARTH_SPAWN_ANIM));

				real_t zAngle = EARTH_FLOAT_ANIM * limbs[EARTH_ELEMENTAL][11][2];
				zAngle = fmod(zAngle, 2 * PI);
				while ( zAngle >= 2 * PI )
				{
					zAngle -= 2 * PI;
				}
				while ( zAngle < 0 )
				{
					zAngle += 2 * PI;
				}
				if ( zAngle >= PI / 2 )
				{
					EARTH_FLOAT_ANIM += spawnRate * 0.2;
				}
				else
				{
					EARTH_FLOAT_ANIM += spawnRate * 0.1;
				}
				EARTH_FLOAT_X = spawnRate * limbs[EARTH_ELEMENTAL][12][0] * sin(EARTH_FLOAT_ANIM * limbs[EARTH_ELEMENTAL][11][0]) * cos(my->yaw);
				EARTH_FLOAT_Y = spawnRate * limbs[EARTH_ELEMENTAL][12][1] * sin(EARTH_FLOAT_ANIM * limbs[EARTH_ELEMENTAL][11][0]) * sin(my->yaw);

				EARTH_FLOAT_Z = EARTH_SPAWN_ANIM * 8.0;
				EARTH_FLOAT_Z += spawnRate * limbs[EARTH_ELEMENTAL][12][2] * sin(zAngle);
			}

			real_t pitchAngle = EARTH_FLOAT_ANIM * 1.0 * limbs[EARTH_ELEMENTAL][11][2] + PI / 4;
			while ( pitchAngle >= 2 * PI )
			{
				pitchAngle -= 2 * PI;
			}
			while ( pitchAngle < 0 )
			{
				pitchAngle += 2 * PI;
			}
			entity->yaw += sin(EARTH_ATTACK_1);
			entity->pitch += limbs[EARTH_ELEMENTAL][13][0] * sin(pitchAngle);

			entity->x += EARTH_FLOAT_X;
			entity->y += EARTH_FLOAT_Y;
			entity->z += EARTH_FLOAT_Z;

			if ( EARTH_DEFEND > 0.0 )
			{
				entity->x -= EARTH_DEFEND * cos(my->yaw);
				entity->y -= EARTH_DEFEND * sin(my->yaw);
				entity->pitch += 0.25 * sin(EARTH_DEFEND * PI / 2);
			}


			if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP1 || MONSTER_ATTACK == MONSTER_POSE_EARTH_ELEMENTAL_ROLL )
			{
				if ( EARTH_LIMB_FSKILL_YAW > -3 * PI )
				{
					EARTH_ATTACK_FLOAT = sin((PI / 2) * EARTH_LIMB_FSKILL_YAW / (-3 * PI));
				}
				else
				{
					EARTH_ATTACK_FLOAT = sin((PI / 2) * (-4 * PI - EARTH_LIMB_FSKILL_YAW) / -PI);
				}
			}
			EARTH_ATTACK_FLOAT = -2.0 * sin(EARTH_ATTACK_1 / 2);
			entity->z -= EARTH_ATTACK_FLOAT;
			break;
		}
		case EARTH_EYES:
		{
			entity->x += limbs[EARTH_ELEMENTAL][2][0] * cos(my->yaw) + limbs[EARTH_ELEMENTAL][2][1] * cos(my->yaw + PI / 2);
			entity->y += limbs[EARTH_ELEMENTAL][2][0] * sin(my->yaw) + limbs[EARTH_ELEMENTAL][2][1] * sin(my->yaw + PI / 2);
			entity->z += limbs[EARTH_ELEMENTAL][2][2];
			entity->focalx = limbs[EARTH_ELEMENTAL][1][0];
			entity->focaly = limbs[EARTH_ELEMENTAL][1][1];
			entity->focalz = limbs[EARTH_ELEMENTAL][1][2];

			if ( body )
			{
				entity->x += EARTH_FLOAT_X;
				entity->y += EARTH_FLOAT_Y;

				if ( EARTH_SPAWN_STATE <= 1 )
				{
					entity->z += EARTH_FLOAT_Z;
					entity->z = std::min(7.5, entity->z);
					entity->x += 4.0 * cos(my->yaw);
					entity->y += 4.0 * sin(my->yaw);
				}
				else
				{
					entity->z += 7.5 * EARTH_SPAWN_ANIM;
					entity->x += 4.0 * EARTH_SPAWN_ANIM * cos(my->yaw);
					entity->y += 4.0 * EARTH_SPAWN_ANIM * sin(my->yaw);

					//entity->roll = sin(EARTH_SPAWN_ANIM * PI);
				}

				real_t zAngle = EARTH_FLOAT_ANIM * limbs[EARTH_ELEMENTAL][11][2] + 3 * PI / 4;
				zAngle = fmod(zAngle, 2 * PI);
				while ( zAngle >= 2 * PI )
				{
					zAngle -= 2 * PI;
				}
				while ( zAngle < 0 )
				{
					zAngle += 2 * PI;
				}
				real_t zMag = 0.5 * sin(zAngle);
				entity->z -= zMag;

				if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP1 || MONSTER_ATTACK == MONSTER_POSE_EARTH_ELEMENTAL_ROLL )
				{
					if ( MONSTER_ATTACKTIME >= limbs[EARTH_ELEMENTAL][15][0] )
					{
						entity->fskill[7] = std::min(1.0, entity->fskill[7] + 0.1); // extra raise
					}
				}
				else
				{
					entity->fskill[7] = std::max(0.0, entity->fskill[7] - 0.1);
				}
				entity->z -= EARTH_ATTACK_FLOAT;
				entity->z -= 1.0 * sin(entity->fskill[7] * PI / 2);
			}
			break;
		}
		case EARTH_LEFTARM:
		{
			real_t yaw = my->yaw;
			if ( body )
			{
				yaw = body->yaw;
				if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP1 )
				{
					real_t bodyRotate = fmod(body->fskill[0], 2 * PI);
					while ( bodyRotate >= 2 * PI )
					{
						bodyRotate -= 2 * PI;
					}
					while ( bodyRotate < 0 )
					{
						bodyRotate += 2 * PI;
					}
					if ( bodyRotate < PI )
					{
						entity->x += 4.0 * sin(bodyRotate) * cos(my->yaw);
						entity->y += 4.0 * sin(bodyRotate) * sin(my->yaw);
					}
				}
				if ( EARTH_ATTACK_2 < 0.0 )
				{
					entity->x += 2.0 * sin(EARTH_ATTACK_2 * PI / 2) * cos(my->yaw);
					entity->y += 2.0 * sin(EARTH_ATTACK_2 * PI / 2) * sin(my->yaw);
				}
				else
				{
					entity->x += 4.0 * std::max(0.0, sin(EARTH_ATTACK_2 * PI / 2)) * cos(my->yaw);
					entity->y += 4.0 * std::max(0.0, sin(EARTH_ATTACK_2 * PI / 2)) * sin(my->yaw);
				}
			}

			real_t sideDist = (1.0 - 0.5 * std::max(0.0, EARTH_ATTACK_2));
			sideDist = std::max(0.25, sideDist - EARTH_DEFEND);
			sideDist *= limbs[EARTH_ELEMENTAL][7][1];
			entity->x += 2.0 * EARTH_DEFEND * cos(yaw);
			entity->y += 2.0 * EARTH_DEFEND * sin(yaw);

			if ( EARTH_SPAWN_STATE <= 1 )
			{
				entity->x += 2.0 * cos(my->yaw + 1 * PI / 6);
				entity->y += 2.0 * sin(my->yaw + 1 * PI / 6);
			}
			else
			{
				entity->x += EARTH_SPAWN_ANIM * cos(my->yaw + 1 * PI / 6);
				entity->y += EARTH_SPAWN_ANIM * sin(my->yaw + 1 * PI / 6);
			}

			entity->x += limbs[EARTH_ELEMENTAL][7][0] * cos(yaw) + sideDist * cos(yaw + PI / 2);
			entity->y += limbs[EARTH_ELEMENTAL][7][0] * sin(yaw) + sideDist * sin(yaw + PI / 2);
			entity->z += limbs[EARTH_ELEMENTAL][7][2];
			entity->focalx = limbs[EARTH_ELEMENTAL][6][0];
			entity->focaly = limbs[EARTH_ELEMENTAL][6][1];
			entity->focalz = limbs[EARTH_ELEMENTAL][6][2];

			if ( body )
			{
				entity->x += EARTH_FLOAT_X;
				entity->y += EARTH_FLOAT_Y;
				entity->z += EARTH_FLOAT_Z;
				entity->pitch = body->pitch;

				real_t pitchAngle = EARTH_FLOAT_ANIM * 1.0 * limbs[EARTH_ELEMENTAL][11][2] + PI / 2;
				while ( pitchAngle >= 2 * PI )
				{
					pitchAngle -= 2 * PI;
				}
				while ( pitchAngle < 0 )
				{
					pitchAngle += 2 * PI;
				}
				entity->x += 1.0 * sin(-pitchAngle) * cos(my->yaw);
				entity->y += 1.0 * sin(-pitchAngle) * sin(my->yaw);
				entity->z += 0.5 * cos(-pitchAngle);

				entity->z -= EARTH_ATTACK_FLOAT;
			}
			break;
		}
		case EARTH_RIGHTARM:
		{
			real_t yaw = my->yaw;
			if ( body )
			{
				yaw = body->yaw;
				if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP1 )
				{
					real_t bodyRotate = fmod(body->fskill[0], 2 * PI);
					while ( bodyRotate >= 2 * PI )
					{
						bodyRotate -= 2 * PI;
					}
					while ( bodyRotate < 0 )
					{
						bodyRotate += 2 * PI;
					}
					if ( bodyRotate >= PI )
					{
						entity->x -= 4.0 * sin(bodyRotate) * cos(my->yaw);
						entity->y -= 4.0 * sin(bodyRotate) * sin(my->yaw);
					}
				}

				if ( EARTH_ATTACK_3 < 0.0 )
				{
					entity->x += 2.0 * sin(EARTH_ATTACK_3 * PI / 2) * cos(my->yaw);
					entity->y += 2.0 * sin(EARTH_ATTACK_3 * PI / 2) * sin(my->yaw);
				}
				else
				{
					entity->x += 4.0 * std::max(0.0, sin(EARTH_ATTACK_3 * PI / 2)) * cos(my->yaw);
					entity->y += 4.0 * std::max(0.0, sin(EARTH_ATTACK_3 * PI / 2)) * sin(my->yaw);
				}
			}

			real_t sideDist = (1.0 - 0.5 * std::max(0.0, EARTH_ATTACK_3));
			sideDist = std::max(0.25, sideDist - EARTH_DEFEND);
			sideDist *= limbs[EARTH_ELEMENTAL][8][1];
			entity->x += 2.0 * EARTH_DEFEND * cos(yaw);
			entity->y += 2.0 * EARTH_DEFEND * sin(yaw);

			if ( EARTH_SPAWN_STATE <= 1 )
			{
				entity->x += 2.0 * cos(my->yaw + 4 * PI / 6);
				entity->y += 2.0 * sin(my->yaw + 4 * PI / 6);
			}
			else
			{
				entity->x += EARTH_SPAWN_ANIM * cos(my->yaw + 4 * PI / 6);
				entity->y += EARTH_SPAWN_ANIM * sin(my->yaw + 4 * PI / 6);
			}

			entity->x += limbs[EARTH_ELEMENTAL][8][0] * cos(yaw) + sideDist * cos(yaw + PI / 2);
			entity->y += limbs[EARTH_ELEMENTAL][8][0] * sin(yaw) + sideDist * sin(yaw + PI / 2);
			entity->z += limbs[EARTH_ELEMENTAL][8][2];
			entity->focalx = limbs[EARTH_ELEMENTAL][6][0];
			entity->focaly = limbs[EARTH_ELEMENTAL][6][1];
			entity->focalz = limbs[EARTH_ELEMENTAL][6][2];

			if ( body )
			{
				entity->x += EARTH_FLOAT_X;
				entity->y += EARTH_FLOAT_Y;
				entity->z += EARTH_FLOAT_Z;
				entity->pitch = body->pitch;

				real_t pitchAngle = EARTH_FLOAT_ANIM * 1.0 * limbs[EARTH_ELEMENTAL][11][2] + PI / 8 + PI / 2;
				while ( pitchAngle >= 2 * PI )
				{
					pitchAngle -= 2 * PI;
				}
				while ( pitchAngle < 0 )
				{
					pitchAngle += 2 * PI;
				}
				entity->x += 1.0 * sin(-pitchAngle) * cos(my->yaw);
				entity->y += 1.0 * sin(-pitchAngle) * sin(my->yaw);
				entity->z += 0.75 * cos(-pitchAngle);

				entity->z -= EARTH_ATTACK_FLOAT;
			}
			break;
		}
		case EARTH_PEBBLE1:
		{
			entity->x += limbs[EARTH_ELEMENTAL][10][0] * cos(my->yaw) + limbs[EARTH_ELEMENTAL][10][1] * cos(my->yaw + PI / 2);
			entity->y += limbs[EARTH_ELEMENTAL][10][0] * sin(my->yaw) + limbs[EARTH_ELEMENTAL][10][1] * sin(my->yaw + PI / 2);
			entity->z += limbs[EARTH_ELEMENTAL][10][2];
			entity->focalx = limbs[EARTH_ELEMENTAL][9][0];
			entity->focaly = limbs[EARTH_ELEMENTAL][9][1];
			entity->focalz = limbs[EARTH_ELEMENTAL][9][2];

			if ( body )
			{
				entity->x += EARTH_FLOAT_X;
				entity->y += EARTH_FLOAT_Y;
				entity->z += EARTH_FLOAT_Z;
				entity->pitch = body->pitch;
			}

			if ( EARTH_SPAWN_STATE <= 1 )
			{
				entity->x += 2.0 * cos(my->yaw + 3 * PI / 6);
				entity->y += 2.0 * sin(my->yaw + 3 * PI / 6);
				entity->z = std::min(7.5, entity->z);
			}
			else
			{
				EARTH_PEBBLE_IDLE_ANIM += MONSTER_ATTACK > 0 ? 0.2 : 0.1;
				entity->x += EARTH_SPAWN_ANIM * cos(my->yaw + 3 * PI / 6);
				entity->y += EARTH_SPAWN_ANIM * sin(my->yaw + 3 * PI / 6);
				entity->z += 2.0;
			}
			entity->x += 1.0 * cos(EARTH_PEBBLE_IDLE_ANIM);
			entity->y += 1.0 * sin(EARTH_PEBBLE_IDLE_ANIM);
			break;
		}
		case EARTH_PEBBLE2:
		{
			entity->x += limbs[EARTH_ELEMENTAL][10][0] * cos(my->yaw) + limbs[EARTH_ELEMENTAL][10][1] * cos(my->yaw + PI / 2);
			entity->y += limbs[EARTH_ELEMENTAL][10][0] * sin(my->yaw) + limbs[EARTH_ELEMENTAL][10][1] * sin(my->yaw + PI / 2);
			entity->z += limbs[EARTH_ELEMENTAL][10][2];
			entity->focalx = limbs[EARTH_ELEMENTAL][9][0];
			entity->focaly = limbs[EARTH_ELEMENTAL][9][1];
			entity->focalz = limbs[EARTH_ELEMENTAL][9][2];

			if ( body )
			{
				entity->x += EARTH_FLOAT_X;
				entity->y += EARTH_FLOAT_Y;
				entity->z += EARTH_FLOAT_Z;
				entity->pitch = body->pitch + PI / 32;
			}

			if ( EARTH_SPAWN_STATE <= 1 )
			{
				entity->x += 2.0 * cos(my->yaw + 5 * PI / 6);
				entity->y += 2.0 * sin(my->yaw + 5 * PI / 6);
				entity->z = std::min(7.5, entity->z);
			}
			else
			{
				EARTH_PEBBLE_IDLE_ANIM += MONSTER_ATTACK > 0 ? 0.4 : 0.2;
				entity->z += 1.0;
				entity->x += EARTH_SPAWN_ANIM * cos(my->yaw + 5 * PI / 6);
				entity->y += EARTH_SPAWN_ANIM * sin(my->yaw + 5 * PI / 6);
			}
			entity->x += 1.5 * (cos(my->yaw + PI / 2 + EARTH_PEBBLE_IDLE_ANIM));
			entity->y += 1.5 * (sin(my->yaw + PI / 2 + EARTH_PEBBLE_IDLE_ANIM));
			break;
		}
		case EARTH_PEBBLE3:
		{
			entity->x += limbs[EARTH_ELEMENTAL][10][0] * cos(my->yaw) + limbs[EARTH_ELEMENTAL][10][1] * cos(my->yaw + PI / 2);
			entity->y += limbs[EARTH_ELEMENTAL][10][0] * sin(my->yaw) + limbs[EARTH_ELEMENTAL][10][1] * sin(my->yaw + PI / 2);
			entity->z += limbs[EARTH_ELEMENTAL][10][2];
			entity->focalx = limbs[EARTH_ELEMENTAL][9][0];
			entity->focaly = limbs[EARTH_ELEMENTAL][9][1];
			entity->focalz = limbs[EARTH_ELEMENTAL][9][2];

			if ( body )
			{
				entity->x += EARTH_FLOAT_X;
				entity->y += EARTH_FLOAT_Y;
				entity->z += EARTH_FLOAT_Z;
				entity->pitch = body->pitch - PI / 32;
			}

			if ( EARTH_SPAWN_STATE <= 1 )
			{
				entity->z = std::min(7.5, entity->z);
				entity->x += 2.0 * cos(my->yaw + 2 * PI / 6);
				entity->y += 2.0 * sin(my->yaw + 2 * PI / 6);
			}
			else
			{
				EARTH_PEBBLE_IDLE_ANIM += MONSTER_ATTACK > 0 ? 0.3 : 0.15;
				entity->x += EARTH_SPAWN_ANIM * cos(my->yaw + 2 * PI / 6);
				entity->y += EARTH_SPAWN_ANIM * sin(my->yaw + 2 * PI / 6);
			}
			entity->x += 1.0 * (cos(my->yaw + EARTH_PEBBLE_IDLE_ANIM));
			entity->y += 1.0 * (sin(my->yaw + EARTH_PEBBLE_IDLE_ANIM));
			entity->x += 2.0 * (cos(my->yaw + PI / 2 + EARTH_PEBBLE_IDLE_ANIM));
			entity->y += 2.0 * (sin(my->yaw + PI / 2 + EARTH_PEBBLE_IDLE_ANIM));
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