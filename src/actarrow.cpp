/*-------------------------------------------------------------------------------

	BARONY
	File: actarrow.cpp
	Desc: behavior function for arrows/bolts

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "monster.hpp"
#include "engine/audio/sound.hpp"
#include "interface/interface.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "items.hpp"
#include "magic/magic.hpp"
#include "scores.hpp"
#include "player.hpp"
#include "prng.hpp"
#include "mod_tools.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define ARROW_STUCK my->skill[0]
#define ARROW_LIFE my->skill[1]
#define ARROW_VELX my->vel_x
#define ARROW_VELY my->vel_y
#define ARROW_VELZ my->vel_z
#define ARROW_OLDX my->fskill[2]
#define ARROW_OLDY my->fskill[3]
#define ARROW_MAXLIFE 600
#define ARROW_INIT my->skill[10]
#define ARROW_FLICKER my->skill[11]
#define ARROW_LIGHTING my->skill[12]
#define ARROW_STUCK_CLIENT my->skill[13]

enum ArrowSpriteTypes : int
{
	PROJECTILE_ROCK_SPRITE = 78,
	PROJECTILE_NORMAL_SPRITE = 166,
	PROJECTILE_BOLT_SPRITE = 167,
	PROJECTILE_SILVER_SPRITE = 924,
	PROJECTILE_PIERCE_SPRITE,
	PROJECTILE_SWIFT_SPRITE,
	PROJECTILE_FIRE_SPRITE,
	PROJECTILE_HEAVY_SPRITE,
	PROJECTILE_CRYSTAL_SPRITE,
	PROJECTILE_HUNTING_SPRITE,
	PROJECTILE_SEED_ROOT_SPRITE = 1881,
	PROJECTILE_SEED_POISON_SPRITE = 1882,
	PROJECTILE_BONE_SPRITE = 2304,
	PROJECTILE_BLACKIRON_SPRITE = 2305
};

void actArrow(Entity* my)
{
	node_t* node = nullptr;

	// lifespan
	ARROW_LIFE++;
	my->removeLightField();

	if ( ARROW_LIFE >= ARROW_MAXLIFE )
	{
		list_RemoveNode(my->mynode);
		return;
	}

	// falling out of the map
	if ( my->z > 32 )
	{
		list_RemoveNode(my->mynode);
		return;
	}

	if ( my->sprite == PROJECTILE_SEED_ROOT_SPRITE
		|| my->sprite == PROJECTILE_SEED_POISON_SPRITE )
	{
		if ( ARROW_LIFE > 1 )
		{
			if ( ARROW_STUCK == 0 )
			{
				my->light = addLight(my->x / 16, my->y / 16, "magic_green");
				if ( flickerLights )
				{
					//Torches will never flicker if this setting is disabled.
					ARROW_FLICKER++;
				}
				if ( ARROW_FLICKER > 5 )
				{
					ARROW_LIGHTING = (ARROW_LIGHTING == 1) + 1;

					if ( ARROW_LIGHTING == 1 )
					{
						my->removeLightField();
						my->light = addLight(my->x / 16, my->y / 16, "magic_green");
					}
					else
					{
						my->removeLightField();
						my->light = addLight(my->x / 16, my->y / 16, "magic_green_flicker");
					}
					ARROW_FLICKER = 0;
				}
			}

			if ( ARROW_STUCK == 0 )
			{
				my->removeLightField();
				Entity* particle = spawnMagicParticleCustom(my, 1816, 0.5, 4);
				if ( particle )
				{
					particle->lightBonus = vec4(0.5f, 0.5f, 0.5f, 0.f);
					//particle->flags[SPRITE] = true;
					particle->ditheringDisabled = true;
				}
			}
		}
	}
	else if ( (my->arrowQuiverType == QUIVER_PIERCE || my->sprite == PROJECTILE_PIERCE_SPRITE) )
	{
		if ( ARROW_STUCK == 0 )
		{
			Entity* particle = spawnMagicParticleCustom(my, 158, 0.5, 4);
			if ( particle )
			{
				particle->lightBonus = vec4(0.5f, 0.5f, 0.5f, 0.f);
				particle->flags[SPRITE] = true;
				particle->ditheringDisabled = true;
			}
		}
	}
	else if ( my->arrowQuiverType == QUIVER_FIRE || my->sprite == PROJECTILE_FIRE_SPRITE )
	{
		if ( ARROW_LIFE > 1 )
		{
			my->light = addLight(my->x / 16, my->y / 16, "fire_arrow");
			if ( flickerLights )
			{
				//Torches will never flicker if this setting is disabled.
				ARROW_FLICKER++;
			}
			if ( ARROW_FLICKER > 5 )
			{
				ARROW_LIGHTING = (ARROW_LIGHTING == 1) + 1;

				if ( ARROW_LIGHTING == 1 )
				{
					my->removeLightField();
					my->light = addLight(my->x / 16, my->y / 16, "fire_arrow");
				}
				else
				{
					my->removeLightField();
					my->light = addLight(my->x / 16, my->y / 16, "fire_arrow_flicker");
				}
				ARROW_FLICKER = 0;
			}

			if ( ARROW_STUCK != 0 )
			{
				if ( Entity* entity = spawnFlame(my, SPRITE_FLAME) )
				{
					if ( ARROW_STUCK == 1 )
					{
						entity->x += .5 * cos(my->yaw);
						entity->y += .5 * sin(my->yaw);
					}
					else
					{
						entity->x += 1.5 * cos(my->yaw);
						entity->y += 1.5 * sin(my->yaw);
					}
					entity->z = my->z;
				}
			}
			else
			{
				Entity* flame = spawnMagicParticleCustom(my, SPRITE_FLAME, 0.5, 4); // this looks nicer than the spawnFlame :)
				if ( flame )
				{
					flame->lightBonus = vec4(0.5f, 0.5f, 0.5f, 0.f);
					flame->flags[SPRITE] = true;
                    flame->ditheringDisabled = true;
				}
			}
		}
	}
	else if ( my->arrowQuiverType == QUIVER_KNOCKBACK || my->sprite == PROJECTILE_HEAVY_SPRITE )
	{
		if ( ARROW_STUCK == 0 )
		{
			Entity* particle = spawnMagicParticleCustom(my, 159, 0.5, 4);
			if ( particle )
			{
				particle->lightBonus = vec4(0.5f, 0.5f, 0.5f, 0.f);
				particle->flags[SPRITE] = true;
                particle->ditheringDisabled = true;
			}
		}
	}
	else if ( my->arrowQuiverType == QUIVER_SILVER || my->sprite == PROJECTILE_SILVER_SPRITE )
	{
		if ( ARROW_STUCK == 0 )
		{
			Entity* particle = spawnMagicParticleCustom(my, 160, 0.5, 4);
			if ( particle )
			{
				particle->lightBonus = vec4(0.5f, 0.5f, 0.5f, 0.f);
				particle->flags[SPRITE] = true;
                particle->ditheringDisabled = true;
			}
		}
	}
	else if ( my->arrowQuiverType == QUIVER_CRYSTAL || my->sprite == PROJECTILE_CRYSTAL_SPRITE )
	{
		if ( ARROW_STUCK == 0 )
		{
			Entity* particle = spawnMagicParticleCustom(my, 155, 0.5, 4);
			if ( particle )
			{
				particle->lightBonus = vec4(0.5f, 0.5f, 0.5f, 0.f);
				particle->flags[SPRITE] = true;
                particle->ditheringDisabled = true;
			}
		}
	}
	else if ( my->arrowQuiverType == QUIVER_LIGHTWEIGHT || my->sprite == PROJECTILE_SWIFT_SPRITE )
	{
		if ( ARROW_STUCK == 0 )
		{
			Entity* particle = spawnMagicParticleCustom(my, 156, 0.5, 4);
			if ( particle )
			{
				particle->lightBonus = vec4(0.5f, 0.5f, 0.5f, 0.f);
				particle->flags[SPRITE] = true;
                particle->ditheringDisabled = true;
			}
		}
	}
	else if ( my->arrowQuiverType == QUIVER_HUNTING || my->sprite == PROJECTILE_HUNTING_SPRITE )
	{
		if ( ARROW_STUCK == 0 )
		{
			Entity* particle = spawnMagicParticleCustom(my, 157, 0.5, 4);
			if ( particle )
			{
				particle->lightBonus = vec4(0.5f, 0.5f, 0.5f, 0.f);
				particle->flags[SPRITE] = true;
                particle->ditheringDisabled = true;
			}
		}
	}
	else if ( my->arrowQuiverType == QUIVER_BLACKIRON || my->sprite == PROJECTILE_BLACKIRON_SPRITE )
	{
		if ( ARROW_STUCK == 0 )
		{
			Entity* particle = spawnMagicParticleCustom(my, 155, 0.5, 4);
			if ( particle )
			{
				particle->lightBonus = vec4(0.5f, 0.5f, 0.5f, 0.f);
				particle->flags[SPRITE] = true;
				particle->ditheringDisabled = true;
			}
		}
	}
	else if ( my->arrowQuiverType == QUIVER_BONE || my->sprite == PROJECTILE_BONE_SPRITE )
	{
		if ( ARROW_STUCK == 0 )
		{
			Entity* particle = spawnMagicParticleCustom(my, 155, 0.5, 4);
			if ( particle )
			{
				particle->lightBonus = vec4(0.5f, 0.5f, 0.5f, 0.f);
				particle->flags[SPRITE] = true;
				particle->ditheringDisabled = true;
			}
		}
	}

	if ( my->arrowArmorPierce > 0 && ARROW_STUCK == 0 && !(my->arrowQuiverType == QUIVER_PIERCE || my->sprite == PROJECTILE_PIERCE_SPRITE) )
	{
		Entity* particle = spawnMagicParticleCustom(my, 158, 0.5, 4);
		if ( particle )
		{
			particle->lightBonus = vec4(0.5f, 0.5f, 0.5f, 0.f);
			particle->flags[SPRITE] = true;
			particle->ditheringDisabled = true;
		}
	}

	if ( multiplayer != CLIENT )
	{
		Sint32 val = (1 << 31);
		val |= (Uint8)(17);
		val |= (((Uint16)(my->arrowShotByWeapon) & 0xFFF) << 8);
		val |= (my->arrowDropOffEquipmentModifier + 8) << 20;
		my->skill[2] = val;//-(1000 + my->arrowShotByWeapon); // invokes actArrow for clients.
		my->flags[INVISIBLE] = false;
	}

	if ( !ARROW_INIT )
	{
		if ( multiplayer == CLIENT )
		{
			if ( my->setArrowProjectileProperties(my->arrowShotByWeapon) )
			{
				ARROW_INIT = 1;
			}
			else
			{
				return;
			}
		}
		else
		{
			if ( my->arrowPower == 0 )
			{
				my->arrowPower = 10 + (my->sprite == PROJECTILE_BOLT_SPRITE);
			}
			if ( my->arrowShotByParent == 0 ) // shot by trap
			{
				my->arrowSpeed = 7;
			}
			ARROW_INIT = 1;
		}
	}

	if ( ARROW_STUCK == 0 )
	{
		if ( my->arrowFallSpeed > 0 )
		{
			real_t pitchChange = 0.02;
			if ( my->arrowShotByWeapon == LONGBOW || my->arrowShotByWeapon == BRANCH_BOW 
				|| my->arrowShotByWeapon == BRANCH_BOW_INFECTED )
			{
				pitchChange = 0.005;
			}
			if ( my->arrowBoltDropOffRange > 0 )
			{
				if ( my->ticks >= my->arrowBoltDropOffRange )
				{
					ARROW_VELZ += my->arrowFallSpeed;
					my->z += ARROW_VELZ;
					my->pitch = std::min(my->pitch + pitchChange, PI / 8);
				}
			}
			else
			{
				ARROW_VELZ += my->arrowFallSpeed;
				my->z += ARROW_VELZ;
				my->pitch = std::min(my->pitch + pitchChange, PI / 8);
			}
		}

		Entity* arrowSpawnedInsideEntity = nullptr;
		if ( ARROW_LIFE == 1 && my->arrowShotByParent == 0 && multiplayer != CLIENT ) // shot by trap
		{
			Entity* parent = uidToEntity(my->parent);
			if ( parent && parent->behavior == &actArrowTrap )
			{
				for ( node_t* node = map.creatures->first; node != nullptr; node = node->next )
				{
					Entity* entity = (Entity*)node->element;
					if ( entity && (entity->behavior == &actMonster || entity->behavior == &actPlayer) )
					{
						if ( entityInsideEntity(my, entity) )
						{
							arrowSpawnedInsideEntity = entity;
						}
						break;
					}
				}
			}
		}

		bool hitSomething = false;
		if ( multiplayer != CLIENT )
		{
			// horizontal motion
			ARROW_VELX = cos(my->yaw) * my->arrowSpeed;
			ARROW_VELY = sin(my->yaw) * my->arrowSpeed;
			ARROW_OLDX = my->x;
			ARROW_OLDY = my->y;

			my->processEntityWind();
			bool halfSpeedCheck = false;
			static ConsoleVariable<bool> cvar_arrow_clip("/arrow_clip_test", true);
			if ( my->arrowSpeed > 4.0 ) // can clip through thin gates
			{
				auto entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 1);
				for ( auto it : entLists )
				{
					if ( !*cvar_arrow_clip && (svFlags & SV_FLAG_CHEATS) )
					{
						break;
					}
					for ( node_t* node = it->first; node != nullptr; node = node->next )
					{
						Entity* entity = (Entity*)node->element;
						if ( entity->behavior == &actGate || entity->behavior == &actDoor || entity->behavior == &actIronDoor )
						{
							if ( entityDist(my, entity) <= my->arrowSpeed )
							{
								halfSpeedCheck = true;
								break;
							}
						}
					}
					if ( halfSpeedCheck )
					{
						break;
					}
				}
			}
			
			if ( !halfSpeedCheck )
			{
				real_t dist = clipMove(&my->x, &my->y, ARROW_VELX, ARROW_VELY, my);
				hitSomething = dist != sqrt(ARROW_VELX * ARROW_VELX + ARROW_VELY * ARROW_VELY);
			}
			else
			{
				real_t vel_x = ARROW_VELX / 2.0;
				real_t vel_y = ARROW_VELY / 2.0;
				real_t dist = clipMove(&my->x, &my->y, vel_x, vel_y, my);
				hitSomething = dist != sqrt(vel_x * vel_x + vel_y * vel_y);
				if ( !hitSomething )
				{
					dist = clipMove(&my->x, &my->y, vel_x, vel_y, my);
					hitSomething = dist != sqrt(vel_x * vel_x + vel_y * vel_y);
				}
			}
		}

		bool arrowInGround = false;
		int index = (int)(my->y / 16) * MAPLAYERS + (int)(my->x / 16) * MAPLAYERS * map.height;
		index = std::clamp(index, 0, (int)(MAPLAYERS * map.width * map.height) - 1);
		if ( map.tiles[index] )
		{
			if ( my->sprite == PROJECTILE_BOLT_SPRITE || my->sprite == PROJECTILE_ROCK_SPRITE ) // bolt/rock
			{
				if ( my->z >= 7 )
				{
					arrowInGround = true;
				}
			}
			else 
			{
				if ( my->pitch >= PI / 12 ) // heavily pitched
				{
					if ( my->z >= 6.5 )
					{
						arrowInGround = true;
					}
				}
				else
				{
					if ( my->z >= 7 ) // shallow pitch, make ground criteria lower.
					{
						arrowInGround = true;
					}
				}
			}

			if ( arrowInGround && (swimmingtiles[map.tiles[index]] || lavatiles[map.tiles[index]]) )
			{
				my->removeLightField();
				list_RemoveNode(my->mynode);
				return;
			}
		}

		if ( multiplayer == CLIENT )
		{
			if ( arrowInGround )
			{
				ARROW_STUCK = 2;
			}
			if ( fabs(ARROW_VELX) < 0.01 && fabs(ARROW_VELY) < 0.01 )
			{
				++ARROW_STUCK_CLIENT;
			}
			if ( ARROW_STUCK_CLIENT > 5 )
			{
				// sometimes the arrow may have 0 velocity during normal movement updates from the server.
				// let it hit 5 iterations before the client assumes it's stuck so z velocity updates no longer happen
				ARROW_STUCK = 1;
			}
			return;
		}

		// damage monsters
		if ( arrowSpawnedInsideEntity || hitSomething || arrowInGround )
		{
			if ( arrowInGround )
			{
				ARROW_STUCK = 2;
				serverUpdateEntitySkill(my, 0);
			}
			else
			{
				ARROW_STUCK = 1;
				if ( !arrowSpawnedInsideEntity && !arrowInGround && hitSomething && hit.entity )
				{
					if ( my->arrowArmorPierce > 0 && hit.entity && hit.entity->getUID() > 0 )
					{
						if ( hit.entity->getStats() || hit.entity->isDamageableCollider() )
						{
							ARROW_STUCK = 0;
							my->collisionIgnoreTargets.insert(hit.entity->getUID());
						}
					}
				}
				if ( ARROW_STUCK > 0 )
				{
					serverUpdateEntitySkill(my, 0);
				}
			}
			my->x = ARROW_OLDX;
			my->y = ARROW_OLDY;

			if ( arrowSpawnedInsideEntity )
			{
				hit.entity = arrowSpawnedInsideEntity;
			}
			Entity* oentity = hit.entity;
			lineTrace(my, my->x, my->y, my->yaw, sqrt(ARROW_VELX * ARROW_VELX + ARROW_VELY * ARROW_VELY), 0, false);
			hit.entity = oentity;
			my->x = hit.x - cos(my->yaw);
			my->y = hit.y - sin(my->yaw);
			ARROW_VELX = 0;
			ARROW_VELY = 0;
			ARROW_VELZ = 0;
			my->entityCheckIfTriggeredBomb(true);
			if ( !hit.entity )
			{
				my->entityCheckIfTriggeredWallButton();
			}
			if ( hit.entity != NULL )
			{
				Entity* parent = uidToEntity(my->parent);
				Stat* hitstats = hit.entity->getStats();
				playSoundEntity(my, 72 + local_rng.rand() % 3, 64);
				if ( hit.entity->behavior == &actChest || hit.entity->isInertMimic() )
				{
					playSoundEntity(hit.entity, 66, 64); //*tink*

					if ( parent )
					{
						if ( parent->behavior == &actPlayer )
						{
							messagePlayer(parent->skill[2], MESSAGE_COMBAT_BASIC, Language::get(667));
							messagePlayer(parent->skill[2], MESSAGE_COMBAT_BASIC, Language::get(447));
						}
						if ( hit.entity->behavior == &actMonster )
						{
							if ( hitstats )
							{
								updateEnemyBar(parent, hit.entity, Language::get(675), hitstats->HP, hitstats->MAXHP,
									false, DamageGib::DMG_WEAKEST);
							}
						}
						else
						{
							updateEnemyBar(parent, hit.entity, Language::get(675), hit.entity->chestHealth, hit.entity->chestMaxHealth,
								false, DamageGib::DMG_WEAKEST);
						}
					}
				}
				else if ( hit.entity->isDamageableCollider() )
				{
					int damage = 1;
					int axe = 0;
					if ( hit.entity->isColliderResistToSkill(PRO_RANGED) || hit.entity->isColliderWall() )
					{
						damage = 1;
					}
					else
					{
						damage = 2 + local_rng.rand() % 3;
					}
					if ( hit.entity->isColliderWeakToSkill(PRO_RANGED) )
					{
						if ( parent && parent->getStats() )
						{
							axe = 2 * (parent->getStats()->getModifiedProficiency(PRO_RANGED) / 20);
						}
						axe = std::min(axe, 9);
					}
					damage += axe;

					int& entityHP = hit.entity->colliderCurrentHP;
					int oldHP = entityHP;
					entityHP -= damage;

					int sound = 28; //damage.ogg
					if ( hit.entity->getColliderSfxOnHit() > 0 )
					{
						sound = hit.entity->getColliderSfxOnHit();
					}
					playSoundEntity(hit.entity, sound, 64);

					if ( entityHP > 0 )
					{
						if ( parent && parent->behavior == &actPlayer )
						{
							messagePlayer(parent->skill[2], MESSAGE_COMBAT_BASIC, Language::get(hit.entity->getColliderOnHitLangEntry()),
								Language::get(hit.entity->getColliderLangName()));
						}
					}
					else
					{
						entityHP = 0;

						hit.entity->colliderKillerUid = parent ? parent->getUID() : 0;
						if ( parent && parent->behavior == &actPlayer )
						{
							if ( oldHP > 0 )
							{
								if ( parent->getStats() && parent->getStats()->getProficiency(PRO_RANGED) < SKILL_LEVEL_BASIC
									&& local_rng.rand() % 20 == 0 )
								{
									parent->increaseSkill(PRO_RANGED);
								}
							}

							if ( hit.entity->getColliderOnBreakLangEntry() != 0 )
							{
								messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(hit.entity->getColliderOnBreakLangEntry()),
									Language::get(hit.entity->getColliderLangName()));
							}
							if ( hit.entity->isColliderWall() )
							{
								Compendium_t::Events_t::eventUpdateWorld(parent->skill[2], Compendium_t::CPDM_BARRIER_DESTROYED, "breakable barriers", 1);
							}

							if ( hit.entity->colliderOldHP > 0 )
							{
								players[parent->skill[2]]->mechanics.incrementBreakableCounter(Player::PlayerMechanics_t::BreakableEvent::GBREAK_COMMON, hit.entity);
							}
						}
					}

					if ( parent )
					{
						updateEnemyBar(parent, hit.entity, Language::get(hit.entity->getColliderLangName()), entityHP, hit.entity->colliderMaxHP, false,
							DamageGib::DMG_DEFAULT);
					}
				}
				else if ( hitstats != NULL && hit.entity != parent )
				{
					if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
					{
						// test for friendly fire
						if ( parent && parent->checkFriend(hit.entity) && parent->friendlyFireProtection(hit.entity) )
						{
							if ( ARROW_STUCK > 0 )
							{
								my->removeLightField();
								list_RemoveNode(my->mynode);
							}
							return;
						}
					}

					//if ( hit.entity && hitstats )
					//{
					//	if ( hitstats->getEffectActive(EFF_NULL_RANGED) )
					//	{
					//		Uint8 effectStrength = hitstats->getEffectActive(EFF_NULL_RANGED);
					//		int duration = hitstats->EFFECTS_TIMERS[EFF_NULL_RANGED];
					//		if ( effectStrength == 1 )
					//		{
					//			if ( hitstats->EFFECTS_TIMERS[EFF_NULL_RANGED] > 0 )
					//			{
					//				hitstats->EFFECTS_TIMERS[EFF_NULL_RANGED] = 1;
					//			}
					//		}
					//		else if ( effectStrength > 1 )
					//		{
					//			--effectStrength;
					//			hitstats->setEffectValueUnsafe(EFF_NULL_RANGED, effectStrength);
					//			hit.entity->setEffect(EFF_NULL_RANGED, effectStrength, hitstats->EFFECTS_TIMERS[EFF_NULL_RANGED], false);
					//		}
					//		if ( (parent && parent->behavior == &actPlayer) || (parent && parent->behavior == &actMonster && parent->monsterAllyGetPlayerLeader())
					//			|| hit.entity->behavior == &actPlayer || hit.entity->monsterAllyGetPlayerLeader() )
					//		{
					//			spawnDamageGib(hit.entity, 0, DamageGib::DMG_GUARD, DamageGibDisplayType::DMG_GIB_GUARD, true);
					//		}

					//		if ( hit.entity->behavior == &actPlayer )
					//		{
					//			messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, makeColorRGB(0, 255, 0), Language::get(6465));
					//		}
					//		if ( parent && parent->behavior == &actPlayer )
					//		{
					//			messagePlayerMonsterEvent(parent->skill[2], makeColorRGB(255, 255, 255),
					//				*hitstats, Language::get(6468), Language::get(6469), MSG_COMBAT); // %s guards the attack
					//		}
					//		playSoundEntity(hit.entity, 166, 128);
					//		my->removeLightField();
					//		list_RemoveNode(my->mynode);
					//		return;
					//	}
					//}

					bool silverDamage = false;
					bool huntingDamage = false;
					if ( my->arrowQuiverType == QUIVER_SILVER )
					{
						if ( hit.entity->isSmiteWeakMonster() )
						{
							// smite these creatures
							silverDamage = true;
							spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, 981);
							playSoundEntity(hit.entity, 249, 64);
						}
						else
						{
							silverDamage = false;
						}
					}
					else if ( my->arrowQuiverType == QUIVER_HUNTING )
					{
						switch ( hitstats->type )
						{
							case RAT:
							case SPIDER:
							case SCORPION:
							case SCARAB:
							case SLIME:
							case CREATURE_IMP:
							case DEMON:
							case MINOTAUR:
							case KOBOLD:
							case COCKATRICE:
							case TROLL:
							case BUGBEAR:
								// more damage to these creatures
								huntingDamage = true;
								for ( int gibs = 0; gibs < 10; ++gibs )
								{
									Entity* gib = spawnGib(hit.entity);
									serverSpawnGibForClient(gib);
								}
								break;
							default:
								huntingDamage = false;
								break;
						}
					}

					int enemyAC = AC(hitstats);

					// do damage
					if ( my->arrowArmorPierce > 0 && enemyAC > 0 )
					{
						if ( my->arrowQuiverType == QUIVER_PIERCE )
						{
							bool oldDefend = hitstats->defending;
							hitstats->defending = false;
							enemyAC = AC(hitstats);
							enemyAC /= 2; // pierce half armor not caring about shield
							hitstats->defending = oldDefend;
						}
						else
						{
							enemyAC /= 2; // pierce half armor.
						}
					}
					else
					{
						// normal damage.
					}

					int numBlessings = 0;
					real_t targetACEffectiveness = Entity::getACEffectiveness(hit.entity, hitstats, hit.entity->behavior == &actPlayer, parent, parent ? parent->getStats() : nullptr, numBlessings);
					int attackAfterReductions = static_cast<int>(std::max(0.0, ((my->arrowPower * targetACEffectiveness - enemyAC))) + (1.0 - targetACEffectiveness) * my->arrowPower);
					int damage = attackAfterReductions;

					bool backstab = false;
					bool flanking = false;
					if ( parent && parent->getStats() )
					{
						Stat* parentStats = parent->getStats();
						if ( parentStats->helmet && parentStats->helmet->type == HAT_HOOD_WHISPERS 
							&& !monsterIsImmobileTurret(hit.entity, hitstats) && !hitstats->getEffectActive(EFF_STASIS) 
							&& !(hitstats->type == MIMIC
								|| hitstats->type == MINIMIMIC 
								|| hitstats->type == MONSTER_ADORCISED_WEAPON) )
						{
							real_t hitAngle = hit.entity->yawDifferenceFromEntity(my);
							if ( (hitAngle >= 0 && hitAngle <= 2 * PI / 3) ) // 120 degree arc
							{
								int stealthCapstoneBonus = 1;
								if ( parent->skillCapstoneUnlockedEntity(PRO_STEALTH) )
								{
									stealthCapstoneBonus = 2;
								}

								real_t equipmentModifier = 0.0;
								real_t bonusModifier = 1.0;
								if ( parentStats->helmet && parentStats->helmet->type == HAT_HOOD_WHISPERS )
								{
									if ( parentStats->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(parentStats) )
									{
										equipmentModifier += (std::min(50 + (10 * abs(parentStats->helmet->beatitude)), 100)) / 100.0;
									}
									else
									{
										equipmentModifier = 0.5;
										bonusModifier = 0.5;
									}
								}

								if ( (hit.entity->monsterState == MONSTER_STATE_WAIT
									|| hit.entity->monsterState == MONSTER_STATE_PATH
									|| (hit.entity->monsterState == MONSTER_STATE_HUNT /*&& uidToEntity(hit.entity->monsterTarget) == nullptr*/))
									&& !hitstats->getEffectActive(EFF_ROOTED) )
								{
									// unaware monster, get backstab damage.
									int bonus = (parentStats->getModifiedProficiency(PRO_STEALTH) / 20 + 2) * (2 * stealthCapstoneBonus);
									int chance = 4;
									if ( hit.entity->monsterState == MONSTER_STATE_HUNT && uidToEntity(hit.entity->monsterTarget) != nullptr )
									{
										chance = 8;
										bonus = (parentStats->getModifiedProficiency(PRO_STEALTH) / 20 + 1) * (stealthCapstoneBonus);
									}
									damage += ((bonus * equipmentModifier) * bonusModifier);
									if ( local_rng.rand() % chance == 0
										&& hit.entity->behavior != &actPlayer
										&& !(parent->behavior == &actPlayer && hit.entity->monsterAllyGetPlayerLeader()) )
									{
										if ( parent->behavior == &actPlayer && players[parent->skill[2]]->mechanics.allowedRaiseStealthAgainstEntity(*hit.entity) )
										{
											parent->increaseSkill(PRO_STEALTH);
											players[parent->skill[2]]->mechanics.enemyRaisedStealthAgainst[hit.entity->getUID()]++;
										}
									}
									backstab = true;
								}
								else if ( local_rng.rand() % 2 == 0 )
								{
									// monster currently engaged in some form of combat maneuver
									// 1 in 2 chance to flank defenses.
									int bonus = (parentStats->getModifiedProficiency(PRO_STEALTH) / 20 + 1) * (stealthCapstoneBonus);
									damage += ((bonus * equipmentModifier) * bonusModifier);
									if ( local_rng.rand() % 20 == 0 
										&& hit.entity->behavior != &actPlayer
										&& !(parent->behavior == &actPlayer && hit.entity->monsterAllyGetPlayerLeader()) )
									{
										if ( parent->behavior == &actPlayer && players[parent->skill[2]]->mechanics.allowedRaiseStealthAgainstEntity(*hit.entity) )
										{
											parent->increaseSkill(PRO_STEALTH);
											players[parent->skill[2]]->mechanics.enemyRaisedStealthAgainst[hit.entity->getUID()]++;
										}
									}
									flanking = true;
								}
							}
						}
					}

					damage = std::max(0, damage);

					if ( silverDamage || huntingDamage )
					{
						damage *= 1.5;
					}

					bool hitWeaklyOnTarget = false;
					int nominalDamage = damage;
					if ( parent )
					{
						if ( my->arrowFallSpeed > 0 )
						{
							if ( my->z >= 5.5 )
							{
								switch ( hitstats->type )
								{
									case RAT:
									case SPIDER:
									case SCORPION:
									case SCARAB:
									case GNOME:
									case KOBOLD:
										// small creatures, no penalty for low shot.
										hitWeaklyOnTarget = false;
										break;
									default:
										hitWeaklyOnTarget = true;
										break;
								}
							}
						}
					}
					real_t damageMultiplier = Entity::getDamageTableMultiplier(hit.entity, *hitstats, DAMAGE_TABLE_RANGED);
					if ( huntingDamage || silverDamage )
					{
						damageMultiplier = std::max(0.75, damageMultiplier);
					}

					Entity::modifyDamageMultipliersFromEffects(hit.entity, parent, damageMultiplier, DAMAGE_TABLE_RANGED, my);

					if ( my->arrowArmorPierce > 0 && parent && parent->behavior == &actPlayer )
					{
						if ( parent->getStats() )
						{
							/*real_t mult = 1.0;
							if ( parent->getStats()->getModifiedProficiency(PRO_RANGED) >= SKILL_LEVEL_LEGENDARY )
							{
								mult = 2.0;
							}*/
							if ( parent->behavior == &actMonster )
							{
								damageMultiplier += std::min(25, std::max(0, statGetPER(parent->getStats(), parent))) / 100.0;
							}
							else
							{
								damageMultiplier += std::max(0, statGetPER(parent->getStats(), parent)) / 100.0;
							}
						}
					}

					if ( hitWeaklyOnTarget )
					{
						damage = damage * (std::max(0.1, damageMultiplier - 0.5));
					}
					else
					{
						damage *= damageMultiplier;
					}

					int trapResist = 0;
					if ( parent )
					{
						if ( parent->behavior == &actArrowTrap )
						{
							trapResist = hit.entity->getEntityBonusTrapResist();
							if ( trapResist != 0 )
							{
								real_t mult = std::max(0.0, 1.0 - (trapResist / 100.0));
								damage *= mult;
							}
						}
					}

					if ( hitstats && hitstats->getEffectActive(EFF_GUARD_BODY) )
					{
						thaumSpellArmorProc(hit.entity, *hitstats, false, parent, EFF_GUARD_BODY);
					}
					if ( hitstats && hitstats->getEffectActive(EFF_DIVINE_GUARD) )
					{
						thaumSpellArmorProc(hit.entity, *hitstats, false, parent, EFF_DIVINE_GUARD);
					}

					/*messagePlayer(0, "My damage: %d, AC: %d, Pierce: %d", my->arrowPower, AC(hitstats), my->arrowArmorPierce);
					messagePlayer(0, "Resolved to %d damage.", damage);*/
					Sint32 oldHP = hitstats->HP;
					hit.entity->modHP(-damage);

					if ( hitstats )
					{
						Sint32 damageTaken = oldHP - hitstats->HP;
						if ( damageTaken > 0 )
						{
							if ( hitstats->getEffectActive(EFF_DEFY_FLESH) )
							{
								hit.entity->defyFleshProc(parent);
							}
							hit.entity->pinpointDamageProc(parent, damageTaken);
						}
						if ( hitstats->getEffectActive(EFF_SPORES) )
						{
							if ( hit.entity->behavior == &actPlayer 
								&& hitstats->type == MYCONID && hitstats->getEffectActive(EFF_GROWTH) >= 4 )
							{
								floorMagicCreateSpores(hit.entity, hit.entity->x, hit.entity->y, hit.entity, 0, SPELL_SPORES);
							}
						}
					}

					// write obituary
					if ( parent )
					{
						if ( parent->behavior == &actArrowTrap )
						{
							hit.entity->setObituary(Language::get(1503));
							hitstats->killer = KilledBy::TRAP_ARROW;

							if ( oldHP > hitstats->HP )
							{
								if ( hit.entity->behavior == &actPlayer )
								{
										Compendium_t::Events_t::eventUpdateWorld(hit.entity->skill[2], Compendium_t::CPDM_TRAP_DAMAGE, "arrow trap", oldHP - hitstats->HP);
										if ( hitstats->HP <= 0 )
										{
											Compendium_t::Events_t::eventUpdateWorld(hit.entity->skill[2], Compendium_t::CPDM_TRAP_KILLED_BY, "arrow trap", 1);
										}
								}
								else if ( hit.entity->behavior == &actMonster )
								{
									if ( auto leader = hit.entity->monsterAllyGetPlayerLeader() )
									{
										Compendium_t::Events_t::eventUpdateWorld(hit.entity->monsterAllyIndex, Compendium_t::CPDM_TRAP_FOLLOWERS_KILLED, "arrow trap", 1);
									}
								}
							}

							if ( hit.entity->onEntityTrapHitSacredPath(parent) )
							{
								if ( hit.entity->behavior == &actPlayer )
								{
									messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, makeColorRGB(0, 255, 0), 
										Language::get(6472), Language::get(6291));
								}
								playSoundEntity(hit.entity, 166, 128);
							}
						}
						else
						{
							parent->killedByMonsterObituary(hit.entity);
						}

						if ( hit.entity->behavior == &actPlayer )
						{
							Compendium_t::Events_t::eventUpdateCodex(hit.entity->skill[2], Compendium_t::CPDM_HP_MOST_DMG_LOST_ONE_HIT, "hp", oldHP - hitstats->HP);
						}
						if ( parent->behavior == &actPlayer )
						{
							if ( oldHP > hitstats->HP )
							{
								if ( itemTypeIsQuiver((ItemType)my->arrowQuiverType) )
								{
									Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_RANGED_DMG_TOTAL, (ItemType)my->arrowQuiverType, oldHP - hitstats->HP);
								}
								if ( isRangedWeapon((ItemType)my->arrowShotByWeapon) )
								{
									Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_RANGED_DMG_TOTAL, (ItemType)my->arrowShotByWeapon, oldHP - hitstats->HP);
								}
								Compendium_t::Events_t::eventUpdateCodex(parent->skill[2], Compendium_t::CPDM_RANGED_DMG_TOTAL, "missiles", oldHP - hitstats->HP);
								Compendium_t::Events_t::eventUpdateCodex(parent->skill[2], Compendium_t::CPDM_RANGED_HITS, "missiles", 1);
								Compendium_t::Events_t::eventUpdateCodex(parent->skill[2], Compendium_t::CPDM_CLASS_RANGED_HITS_RUN, "missiles", 1);
							}

							if ( itemTypeIsQuiver((ItemType)my->arrowQuiverType) )
							{
								Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_DMG_MAX, (ItemType)my->arrowQuiverType, damage);
								Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_AMMO_HIT, (ItemType)my->arrowQuiverType, 1);
							}
							if ( isRangedWeapon((ItemType)my->arrowShotByWeapon) )
							{
								Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_DMG_MAX, (ItemType)my->arrowShotByWeapon, damage);
								Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_SHOTS_HIT, (ItemType)my->arrowShotByWeapon, 1);
							}
							if ( my->arrowShotByWeapon == SLING && damage == 0 )
							{
								Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_DMG_0, (ItemType)my->arrowShotByWeapon, 1);
							}
						}
						else if ( parent->behavior == &actMonster )
						{
							if ( oldHP > hitstats->HP )
							{
								if ( Stat* parentStats = parent->getStats() )
								{
									if ( parentStats->type == SENTRYBOT )
									{
										if ( auto leader = parent->monsterAllyGetPlayerLeader() )
										{
											Compendium_t::Events_t::eventUpdate(leader->skill[2],
												Compendium_t::CPDM_SENTRY_DEPLOY_DMG, TOOL_SENTRYBOT, oldHP - hitstats->HP);
										}
									}
								}
							}
						}

						if ( hit.entity->behavior == &actMonster && parent->behavior == &actPlayer )
						{
							if ( damage >= 80 && hitstats->type != HUMAN && !parent->checkFriend(hit.entity) )
							{
								achievementObserver.awardAchievement(parent->skill[2], AchievementObserver::BARONY_ACH_FELL_BEAST);
							}
							if ( my->arrowQuiverType == QUIVER_LIGHTWEIGHT
								&& my->arrowShotByWeapon == COMPOUND_BOW )
							{
								achievementObserver.updatePlayerAchievement(parent->skill[2], AchievementObserver::BARONY_ACH_STRUNG_OUT, AchievementObserver::ACH_EVENT_NONE);
							}
						}
					}

					if ( damage > 0 )
					{
						Entity* gib = spawnGib(hit.entity);
						serverSpawnGibForClient(gib);
						playSoundEntity(hit.entity, 28, 64);
						if ( hit.entity->behavior == &actPlayer )
						{
							if ( players[hit.entity->skill[2]]->isLocalPlayer() )
							{
								cameravars[hit.entity->skill[2]].shakex += .1;
								cameravars[hit.entity->skill[2]].shakey += 10;
							}
							else
							{
								if ( hit.entity->skill[2] > 0 )
								{
									strcpy((char*)net_packet->data, "SHAK");
									net_packet->data[4] = 10; // turns into .1
									net_packet->data[5] = 10;
									net_packet->address.host = net_clients[hit.entity->skill[2] - 1].host;
									net_packet->address.port = net_clients[hit.entity->skill[2] - 1].port;
									net_packet->len = 6;
									sendPacketSafe(net_sock, -1, net_packet, hit.entity->skill[2] - 1);
								}
							}
						}

						bool doSkillIncrease = true;
						if ( monsterIsImmobileTurret(hit.entity, hitstats) )
						{
							if ( hitstats->type == DUMMYBOT && hitstats->HP > 0 )
							{
								doSkillIncrease = true; // can train on dummybots.
							}
							else
							{
								doSkillIncrease = false; // no skill for killing/hurting other turrets.
							}
						}
						else if ( hit.entity->behavior == &actMonster 
							&& (hit.entity->monsterAllyGetPlayerLeader() || (hitstats && achievementObserver.checkUidIsFromPlayer(hitstats->leader_uid) >= 0))
							&& parent && parent->behavior == &actPlayer )
						{
							doSkillIncrease = false; // no level up on allies
						}
						if ( hit.entity->behavior == &actPlayer && parent && parent->behavior == &actPlayer )
						{
							doSkillIncrease = false; // no skill for killing/hurting players
						}
						if ( hitstats->getEffectActive(EFF_STASIS) )
						{
							doSkillIncrease = false;
						}

						if ( doSkillIncrease && parent && parent->behavior == &actPlayer )
						{
							if ( parent->isInvisible() && parent->checkEnemy(hit.entity) )
							{
								players[parent->skill[2]]->mechanics.updateSustainedSpellEvent(SPELL_INVISIBILITY, 10.0, 1.0, hit.entity);
							}
						}

						int chance = 10;
						if ( doSkillIncrease && (local_rng.rand() % chance == 0) && parent && parent->getStats() )
						{
							if ( hitstats->type != DUMMYBOT 
								|| (hitstats->type == DUMMYBOT && parent->getStats()->getProficiency(PRO_RANGED) < SKILL_LEVEL_BASIC) )
							{
								parent->increaseSkill(PRO_RANGED);
							}
						}
					}
					else
					{
						// playSoundEntity(hit.entity, 66, 64); //*tink*
						if ( hit.entity->behavior == &actPlayer )
						{
							if ( players[hit.entity->skill[2]]->isLocalPlayer() )
							{
								cameravars[hit.entity->skill[2]].shakex += .05;
								cameravars[hit.entity->skill[2]].shakey += 5;
							}
							else
							{
								if ( hit.entity->skill[2] > 0 )
								{
									strcpy((char*)net_packet->data, "SHAK");
									net_packet->data[4] = 5; // turns into .05
									net_packet->data[5] = 5;
									net_packet->address.host = net_clients[hit.entity->skill[2] - 1].host;
									net_packet->address.port = net_clients[hit.entity->skill[2] - 1].port;
									net_packet->len = 6;
									sendPacketSafe(net_sock, -1, net_packet, hit.entity->skill[2] - 1);
								}
							}
						}
					}

					bool envenomWeapon = false;
					if ( parent )
					{
						Stat* parentStats = parent->getStats();
						if ( parentStats && parentStats->getEffectActive(EFF_ENVENOM_WEAPON) && hitstats )
						{
							if ( local_rng.rand() % 2 == 0 )
							{
								int envenomDamage = std::min(
									getSpellDamageSecondaryFromID(SPELL_ENVENOM_WEAPON, parent, parentStats, parent),
									getSpellDamageFromID(SPELL_ENVENOM_WEAPON, parent, parentStats, parent));

								hit.entity->modHP(-envenomDamage); // do the damage
								for ( int tmp = 0; tmp < 3; ++tmp )
								{
									Entity* gib = spawnGib(hit.entity, 211);
									serverSpawnGibForClient(gib);
								}
								if ( !hitstats->getEffectActive(EFF_POISONED) )
								{
									envenomWeapon = true;
									hitstats->setEffectActive(EFF_POISONED, 1);

									int duration = TICKS_PER_SECOND * envenomDamage + 10;
									hitstats->EFFECTS_TIMERS[EFF_POISONED] = std::max(200, duration - hit.entity->getCON() * 20);
									hitstats->poisonKiller = parent->getUID();
									if ( hit.entity->isEntityPlayer() )
									{
										messagePlayerMonsterEvent(hit.entity->isEntityPlayer(), makeColorRGB(255, 0, 0), *parentStats, Language::get(6531), Language::get(6532), MSG_COMBAT);
										serverUpdateEffects(hit.entity->isEntityPlayer());
									}

									if ( parent->behavior == &actPlayer )
									{
										players[parent->skill[2]]->mechanics.updateSustainedSpellEvent(SPELL_ENVENOM_WEAPON, 50.0, 1.0, hit.entity);
									}
								}
							}
						}
					}

					if ( hitstats->HP <= 0 && parent)
					{
						parent->awardXP( hit.entity, true, true );
						spawnBloodVialOnMonsterDeath(hit.entity, hitstats, parent);

						if ( parent->behavior == &actPlayer )
						{
							Compendium_t::Events_t::eventUpdateCodex(parent->skill[2], Compendium_t::CPDM_RANGED_KILLS, "missiles", 1);
						}
					}

					// alert the monster
					if ( hit.entity->behavior == &actMonster && parent != nullptr )
					{
						bool alertTarget = hit.entity->monsterAlertBeforeHit(parent);

						if ( alertTarget && hit.entity->monsterState != MONSTER_STATE_ATTACK && (hitstats->type < LICH || hitstats->type >= SHOPKEEPER) )
						{
							hit.entity->monsterAcquireAttackTarget(*parent, MONSTER_STATE_PATH, true);
						}

						bool alertAllies = true;
						if ( parent->behavior == &actPlayer || parent->monsterAllyIndex != -1 )
						{
							if ( hit.entity->behavior == &actPlayer || (hit.entity->behavior == &actMonster && hit.entity->monsterAllyIndex != -1) )
							{
								// if a player ally + hit another ally or player, don't alert other allies.
								alertAllies = false;
							}
						}

						// alert other monsters too
						if ( alertAllies )
						{
							hit.entity->alertAlliesOnBeingHit(parent);
						}
						hit.entity->updateEntityOnHit(parent, alertTarget);
						if ( parent->behavior == &actPlayer )
						{
							Uint32 color = makeColorRGB(0, 255, 0);
							if ( huntingDamage )
							{
								// plunges into the %s!
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(3750), Language::get(3751), MSG_COMBAT);
							}
							else if ( silverDamage )
							{
								// smites the %s!
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(3743), Language::get(3744), MSG_COMBAT);
							}
							else if ( backstab && hitstats->HP > 0 )
							{
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(6104), Language::get(6105), MSG_COMBAT);
							}
							else if ( damage <= (nominalDamage * .7) && hitstats->HP > 0 )
							{
								// weak shot.
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(3733), Language::get(3734), MSG_COMBAT);
							}
							else
							{
								// you shot the %s!
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(446), Language::get(448), MSG_COMBAT_BASIC);
							}
							if ( my->arrowArmorPierce > 0 /*&& AC(hitstats) > 0*/ )
							{
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(2513), Language::get(2514), MSG_COMBAT);
							}
						}
					}
					if ( hit.entity->behavior == &actPlayer )
					{
						Uint32 color = makeColorRGB(255, 0, 0);
						if ( silverDamage )
						{
							messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(3745)); // you are smited!
						}
						else if ( huntingDamage )
						{
							messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(3752)); // arrow plunged into you!
						}
						else if ( my->arrowQuiverType == QUIVER_KNOCKBACK )
						{
							// no "hit by arrow!" message, let the knockback do the work.
						}
						else if ( my->arrowQuiverType == QUIVER_HUNTING && !(hitstats->amulet && hitstats->amulet->type == AMULET_POISONRESISTANCE)
							&& !(hitstats->type == INSECTOID) )
						{
							// no "hit by arrow!" message, let the hunting arrow effect do the work.
						}
						else
						{
							if ( my )
							{
								if ( my->sprite == PROJECTILE_ROCK_SPRITE )
								{
									// rock.
									messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT_BASIC, color, Language::get(2512));
								}
								else if (my->sprite == PROJECTILE_BOLT_SPRITE )
								{
									// bolt.
									messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT_BASIC, color, Language::get(2511));
								}
								else
								{
									// arrow.
									messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT_BASIC, color, Language::get(451));
								}
							}
							else
							{
								messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT_BASIC, color, Language::get(451));
							}
						}

						if ( my->arrowArmorPierce > 0 /*&& AC(hitstats) > 0*/ )
						{
							messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(2515));
						}
					}

					if ( my->sprite == PROJECTILE_SEED_POISON_SPRITE )
					{
						floorMagicCreateSpores(nullptr, hit.entity->x, hit.entity->y, parent, 15, SPELL_SPORES);
					}
					else if ( my->sprite == PROJECTILE_SEED_ROOT_SPRITE )
					{
						floorMagicCreateRoots(hit.entity->x, hit.entity->y, parent, 3, SPELL_ROOTS, 3 * TICKS_PER_SECOND, PARTICLE_TIMER_ACTION_ROOTS_SINGLE_TILE);
					}

					bool statusEffectApplied = false;
					if ( hitstats->HP > 0 )
					{
						bool procEffect = true;
						if ( trapResist > 0 )
						{
							if ( local_rng.rand() % 100 < trapResist )
							{
								procEffect = false;
							}
						}

						if ( hitstats->HP > 0 && hitstats->OLDHP > hitstats->HP && parent )
						{
							// assist damage from summons
							if ( parent->behavior == &actMonster )
							{
								int summonSpellID = getSpellFromSummonedEntityForSpellEvent(parent);
								if ( summonSpellID != SPELL_NONE )
								{
									if ( Stat* parentStats = parent->getStats() )
									{
										if ( Entity* leader = uidToEntity(parentStats->leader_uid) )
										{
											if ( local_rng.rand() % 8 == 0 )
											{
												magicOnSpellCastEvent(leader, nullptr, hit.entity, summonSpellID, spell_t::SPELL_LEVEL_EVENT_ASSIST | spell_t::SPELL_LEVEL_EVENT_MINOR_CHANCE, 1);
											}
										}
									}
								}
							}
						}

						if ( my->sprite == PROJECTILE_SEED_ROOT_SPRITE )
						{
							if ( hit.entity->setEffect(EFF_ROOTED, true, TICKS_PER_SECOND, false) )
							{
								statusEffectApplied = true;
							}
							if ( parent && parent->behavior == &actPlayer )
							{
								Uint32 color = makeColorRGB(0, 255, 0);
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(6743), Language::get(6742), MSG_COMBAT);
							}
							if ( hit.entity->behavior == &actPlayer )
							{
								Uint32 color = makeColorRGB(255, 0, 0);
								messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(6744));
							}
						}
						else if ( my->sprite == PROJECTILE_SEED_POISON_SPRITE )
						{
							statusEffectApplied = true;
							if ( parent && parent->behavior == &actPlayer )
							{
								Uint32 color = makeColorRGB(0, 255, 0);
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(6746), Language::get(6745), MSG_COMBAT);
							}
							if ( hit.entity->behavior == &actPlayer )
							{
								Uint32 color = makeColorRGB(255, 0, 0);
								messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(6747));
							}
						}
						else if ( my->arrowQuiverType == QUIVER_FIRE && procEffect )
						{
							bool burning = hit.entity->flags[BURNING];
							hit.entity->SetEntityOnFire(my);
							if ( hitstats )
							{
								hitstats->burningInflictedBy = static_cast<Sint32>(my->parent);
							}
							if ( !burning && hit.entity->flags[BURNING] )
							{
								if ( parent && parent->behavior == &actPlayer )
								{
									Uint32 color = makeColorRGB(0, 255, 0);
									if ( hitstats )
									{
										messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(3739), Language::get(3740), MSG_COMBAT);
										if ( hit.entity->behavior == &actMonster )
										{
											achievementObserver.addEntityAchievementTimer(hit.entity, AchievementObserver::BARONY_ACH_PLEASE_HOLD, 150, true, 0);
										}
									}
								}
								if ( hit.entity->behavior == &actPlayer )
								{
									Uint32 color = makeColorRGB(255, 0, 0);
									messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(3741));
								}
								statusEffectApplied = true;
							}
						}
						else if ( my->arrowQuiverType == QUIVER_KNOCKBACK && procEffect && hit.entity->setEffect(EFF_KNOCKBACK, true, 30, false) )
						{
							real_t pushbackMultiplier = 0.6;
							if ( !hit.entity->isMobile() )
							{
								pushbackMultiplier += 0.3;
							}
							/*if ( hitWeaklyOnTarget )
							{
								pushbackMultiplier -= 0.3;
							}*/

							if ( hit.entity->behavior == &actMonster )
							{
								if ( parent )
								{
									real_t tangent = atan2(hit.entity->y - parent->y, hit.entity->x - parent->x);
									hit.entity->vel_x = cos(tangent) * pushbackMultiplier;
									hit.entity->vel_y = sin(tangent) * pushbackMultiplier;
									hit.entity->monsterKnockbackVelocity = 0.01;
									hit.entity->monsterKnockbackUID = my->parent;
									hit.entity->monsterKnockbackTangentDir = tangent;
									//hit.entity->lookAtEntity(*parent);
								}
								else
								{
									real_t tangent = atan2(hit.entity->y - my->y, hit.entity->x - my->x);
									hit.entity->vel_x = cos(tangent) * pushbackMultiplier;
									hit.entity->vel_y = sin(tangent) * pushbackMultiplier;
									hit.entity->monsterKnockbackVelocity = 0.01;
									hit.entity->monsterKnockbackTangentDir = tangent;
									//hit.entity->lookAtEntity(*my);
								}
							}
							else if ( hit.entity->behavior == &actPlayer )
							{
								if ( parent )
								{
									real_t dist = entityDist(parent, hit.entity);
									if ( dist < TOUCHRANGE )
									{
										pushbackMultiplier += 0.5;
									}
								}
								if ( !players[hit.entity->skill[2]]->isLocalPlayer() )
								{
									hit.entity->monsterKnockbackVelocity = pushbackMultiplier;
									hit.entity->monsterKnockbackTangentDir = my->yaw;
									serverUpdateEntityFSkill(hit.entity, 11);
									serverUpdateEntityFSkill(hit.entity, 9);
								}
								else
								{
									hit.entity->monsterKnockbackVelocity = pushbackMultiplier;
									hit.entity->monsterKnockbackTangentDir = my->yaw;
								}
							}

							if ( parent && parent->behavior == &actPlayer )
							{
								Uint32 color = makeColorRGB(0, 255, 0);
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(3215), Language::get(3214), MSG_COMBAT);

								if ( hit.entity->behavior == &actMonster )
								{
									achievementObserver.awardAchievementIfActive(parent->skill[2], hit.entity, AchievementObserver::BARONY_ACH_PLEASE_HOLD);
								}
							}
							if ( hit.entity->behavior == &actPlayer )
							{
								Uint32 color = makeColorRGB(255, 0, 0);
								messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(3742));
							}

							if ( hit.entity->monsterAttack == 0 )
							{
								hit.entity->monsterHitTime = std::max(HITRATE - 12, hit.entity->monsterHitTime);
							}
							statusEffectApplied = true;
						}
						else if ( my->arrowQuiverType == QUIVER_HUNTING && !(hitstats->amulet && hitstats->amulet->type == AMULET_POISONRESISTANCE)
							&& !(hitstats->type == INSECTOID) && procEffect )
						{
							if ( !hitstats->getEffectActive(EFF_POISONED) )
							{
								hitstats->poisonKiller = my->parent;
								hitstats->setEffectActive(EFF_POISONED, 1);
								hitstats->setEffectActive(EFF_SLOW, 1);
								if ( my->arrowPoisonTime > 0 )
								{
									hitstats->EFFECTS_TIMERS[EFF_POISONED] = my->arrowPoisonTime;
									hitstats->EFFECTS_TIMERS[EFF_SLOW] = my->arrowPoisonTime;
								}
								else
								{
									hitstats->EFFECTS_TIMERS[EFF_POISONED] = 160;
									hitstats->EFFECTS_TIMERS[EFF_SLOW] = 160;
								}
								statusEffectApplied = true;
							}
							if ( statusEffectApplied )
							{
								serverUpdateEffects(hit.entity->skill[2]);
								if ( parent && parent->behavior == &actPlayer )
								{
									Uint32 color = makeColorRGB(0, 255, 0);
									messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(3747), Language::get(3748), MSG_COMBAT);

									achievementObserver.addEntityAchievementTimer(hit.entity, AchievementObserver::BARONY_ACH_PLEASE_HOLD, 150, true, 0);
								}
								if ( hit.entity->behavior == &actPlayer )
								{
									if ( local_rng.rand() % 8 == 0 && hit.entity->char_gonnavomit == 0 && !hitstats->getEffectActive(EFF_VOMITING) )
									{
										// maybe vomit
										messagePlayer(hit.entity->skill[2], MESSAGE_STATUS, Language::get(634));
										if ( hit.entity->entityCanVomit() )
										{
											hit.entity->char_gonnavomit = 140 + local_rng.rand() % 60;
										}
									}
									Uint32 color = makeColorRGB(255, 0, 0);
									messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(3749));
								}
							}
							else if ( hit.entity->behavior == &actPlayer )
							{
								Uint32 color = makeColorRGB(255, 0, 0);
								messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT_BASIC, color, Language::get(451)); // you are hit by an arrow!
							}
						}
						else if ( envenomWeapon )
						{
							statusEffectApplied = true;
							if ( parent && parent->behavior == &actPlayer )
							{
								Uint32 color = makeColorRGB(0, 255, 0);
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(6533), Language::get(6534), MSG_COMBAT);
							}
						}

						if ( my->arrowQuiverType == QUIVER_HUNTING && procEffect )
						{
							hit.entity->degradeAmuletProc(hitstats, AMULET_POISONRESISTANCE);
						}
					}
					else
					{
						// HP <= 0
						if ( parent && parent->behavior == &actPlayer )
						{
							Uint32 color = makeColorRGB(0, 255, 0);
							if ( backstab )
							{
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(2547), Language::get(2548), MSG_COMBAT);
							}
							else
							{
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(692), Language::get(697), MSG_COMBAT);
							}
						}
					}

					if ( hit.entity->behavior == &actPlayer )
					{
						if ( parent )
						{
							if ( hitstats->defending )
							{
								updateAchievementThankTheTank(hit.entity->skill[2], parent, false);
							}
							else
							{
								achievementThankTheTankPair[hit.entity->skill[2]].erase(parent->getUID());
							}
						}
					}

					bool armorDegraded = false;

					// hit armor degrade
					if ( hitstats && parent && parent->getStats() )
					{
						Item* armor = NULL;
						int armornum = 0;
						bool isWeakArmor = false;
						if ( damage > 0 || (damage == 0 && !(hitstats->shield && hitstats->defending)) )
						{
							armornum = hitstats->pickRandomEquippedItemToDegradeOnHit(&armor, true, false, false, true);
							if ( armor != NULL && armor->status > BROKEN )
							{
								switch ( armor->type )
								{
								case CRYSTAL_HELM:
								case CRYSTAL_SHIELD:
								case CRYSTAL_BREASTPIECE:
								case CRYSTAL_BOOTS:
								case CRYSTAL_GLOVES:
									isWeakArmor = true;
									break;
								default:
									isWeakArmor = false;
									break;
								}
							}

							int armorDegradeChance = 30;
							if ( isWeakArmor )
							{
								armorDegradeChance = 25;
							}
							if ( hitstats->type == GOBLIN )
							{
								armorDegradeChance += 10;
							}

							if ( hit.entity->behavior == &actPlayer && armornum == 4 && hitstats->shield )
							{
								if ( skillCapstoneUnlocked(hit.entity->skill[2], PRO_SHIELD) )
								{
									armorDegradeChance = 100; // don't break.
								}
								else
								{
									if ( itemCategory(hitstats->shield) == ARMOR )
									{
										armorDegradeChance += 2 * (hitstats->getModifiedProficiency(PRO_SHIELD) / 10);
										armorDegradeChance += 10;
										if ( !players[hit.entity->skill[2]]->mechanics.itemDegradeRoll(hitstats->shield) )
										{
											armorDegradeChance = 100; // don't break.
										}
									}
									else
									{
										armorDegradeChance += (hitstats->getModifiedProficiency(PRO_SHIELD) / 10);
									}
								}
							}

							if ( armorDegradeChance == 100 || (local_rng.rand() % armorDegradeChance > 0) )
							{
								armor = NULL;
								armornum = 0;
							}
						}

						// if nothing chosen to degrade, check extra shield chances to degrade
						if ( hitstats->shield != NULL && hitstats->shield->status > BROKEN && armor == NULL
							&& !itemTypeIsQuiver(hitstats->shield->type) && itemCategory(hitstats->shield) != SPELLBOOK
							&& !itemTypeIsFoci(hitstats->shield->type)
							&& !(hitstats->shield->type >= INSTRUMENT_FLUTE && hitstats->shield->type <= INSTRUMENT_HORN)
							&& hitstats->shield->type != TOOL_TINKERING_KIT && hitstats->shield->type != TOOL_FRYING_PAN )
						{
							if ( hitstats->shield->type == TOOL_CRYSTALSHARD && hitstats->defending )
							{
								// shards degrade by 1 stage each hit.
								armor = hitstats->shield;
								armornum = 4;
							}
							else if ( hitstats->shield->type == MIRROR_SHIELD && hitstats->defending )
							{
								// mirror shield degrade by 1 stage each hit.
								armor = hitstats->shield;
								armornum = 4;
							}
							{
								// if no armor piece was chosen to break, grant chance to improve shield skill.
								if ( itemCategory(hitstats->shield) == ARMOR
									|| (hitstats->defending) )
								{
									int roll = 20;
									int hitskill = hitstats->getProficiency(PRO_SHIELD) / 20;
									roll += hitskill * 5;
									if ( damage == 0 )
									{
										roll /= 2;
									}
									if ( roll > 0 )
									{
										bool success = (local_rng.rand() % roll == 0);
										if ( !success && hit.entity->behavior == &actPlayer && hitstats->defending )
										{
											if ( players[hit.entity->skill[2]]->mechanics.defendTicks != 0 )
											{
												if ( (::ticks - players[hit.entity->skill[2]]->mechanics.defendTicks) < (TICKS_PER_SECOND / 3) )
												{
													// perfect block timing, roll again
													success = (local_rng.rand() % roll == 0);
												}
											}
										}

										if ( success )
										{
											bool increaseSkill = true;
											if ( hit.entity->behavior == &actPlayer && parent->behavior == &actPlayer )
											{
												increaseSkill = false;
											}
											else if ( hit.entity->behavior == &actPlayer && parent->monsterAllyGetPlayerLeader() )
											{
												increaseSkill = false;
											}
											else if ( hit.entity->behavior == &actPlayer
												&& !players[hit.entity->skill[2]]->mechanics.allowedRaiseBlockingAgainstEntity(*parent) )
											{
												increaseSkill = false;
											}
											else if ( hitstats->getEffectActive(EFF_SHAPESHIFT) )
											{
												increaseSkill = false;
											}
											else if ( itemCategory(hitstats->shield) != ARMOR
												&& hitstats->getProficiency(PRO_SHIELD) >= SKILL_LEVEL_SKILLED )
											{
												increaseSkill = false; // non-shield offhands dont increase skill past 40.
											}
											if ( increaseSkill )
											{
												hit.entity->increaseSkill(PRO_SHIELD); // increase shield skill
												if ( hit.entity->behavior == &actPlayer )
												{
													players[hit.entity->skill[2]]->mechanics.enemyRaisedBlockingAgainst[parent->getUID()]++;
												}
											}
										}
									}
								}

								// shield still has chance to degrade after raising skill.
								int shieldDegradeChance = 20;
								if ( svFlags & SV_FLAG_HARDCORE )
								{
									shieldDegradeChance = 40;
								}
								if ( hitstats->type == GOBLIN )
								{
									shieldDegradeChance += 10;
								}

								if ( hit.entity->behavior == &actPlayer )
								{
									if ( itemCategory(hitstats->shield) == ARMOR )
									{
										shieldDegradeChance += 2 * (hitstats->getModifiedProficiency(PRO_SHIELD) / 10); // 2x shield bonus offhand
										if ( !players[hit.entity->skill[2]]->mechanics.itemDegradeRoll(hitstats->shield) )
										{
											shieldDegradeChance = 100; // don't break.
										}
									}
									else
									{
										shieldDegradeChance += (hitstats->getModifiedProficiency(PRO_SHIELD) / 10);
									}
									if ( skillCapstoneUnlocked(hit.entity->skill[2], PRO_SHIELD) )
									{
										shieldDegradeChance = 100; // don't break.
									}
								}
								if ( shieldDegradeChance < 100 && armor == NULL &&
									(hitstats->defending && local_rng.rand() % shieldDegradeChance == 0)
									)
								{
									armor = hitstats->shield;
									armornum = 4;
								}
							}
						}

						if ( armor != NULL && armor->status > BROKEN )
						{
							if ( hit.entity->degradeArmor(*hitstats, *armor, armornum) )
							{
								armorDegraded = true;
							}
							if ( armor->status == BROKEN )
							{
								if ( parent && parent->behavior == &actPlayer && hit.entity->behavior == &actMonster )
								{
									steamStatisticUpdateClient(parent->skill[2], STEAM_STAT_UNSTOPPABLE_FORCE, STEAM_STAT_INT, 1);
									players[parent->skill[2]]->mechanics.incrementBreakableCounter(Player::PlayerMechanics_t::BreakableEvent::GBREAK_DEGRADE, hit.entity);
									if ( armornum == 4 && hitstats->type == BUGBEAR
										&& (hitstats->defending || hit.entity->monsterAttack == MONSTER_POSE_BUGBEAR_SHIELD) )
									{
										steamAchievementClient(parent->skill[2], "BARONY_ACH_BEAR_WITH_ME");
									}
								}
							}
						}
					}

					if ( damage == 0 && !statusEffectApplied && !armorDegraded )
					{
						playSoundEntity(hit.entity, 66, 64); //*tink*
						if ( hit.entity->behavior == &actPlayer )
						{
							messagePlayer(hit.entity->skill[2], MESSAGE_COMBAT_BASIC, Language::get(452)); // player notified no damage.
						}
						if ( parent && parent->behavior == &actPlayer )
						{
							if ( hitstats->type == HUMAN )
							{
								if ( hitstats->sex )
								{
									messagePlayer(parent->skill[2], MESSAGE_COMBAT_BASIC, Language::get(449));
								}
								else
								{
									messagePlayer(parent->skill[2], MESSAGE_COMBAT_BASIC, Language::get(450));
								}
							}
							else
							{
								messagePlayer(parent->skill[2], MESSAGE_COMBAT_BASIC, Language::get(447));
							}
						}
					}
					else if ( damage == 0 && (statusEffectApplied || armorDegraded) )
					{
						playSoundEntity(hit.entity, 28, 64);
					}

					// update enemy bar for attacker
					DamageGib dmgGib = DMG_DEFAULT;
					if ( damageMultiplier <= 0.75 )
					{
						dmgGib = DMG_WEAKEST;
					}
					else if ( damageMultiplier <= 0.85 )
					{
						dmgGib = DMG_WEAKER;
					}
					else if ( damageMultiplier >= 1.25 )
					{
						dmgGib = DMG_STRONGEST;
					}
					else if ( damageMultiplier >= 1.15 )
					{
						dmgGib = DMG_STRONGER;
					}
					if ( !strcmp(hitstats->name, "") )
					{
						updateEnemyBar(parent, hit.entity, getMonsterLocalizedName(hitstats->type).c_str(), hitstats->HP, hitstats->MAXHP, 
							false, dmgGib);
					}
					else
					{
						updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP,
							false, dmgGib);
					}
				}

				if ( ARROW_STUCK > 0 )
				{
					if ( my->sprite == PROJECTILE_SEED_POISON_SPRITE )
					{
						if ( !hitstats || hit.entity->isInertMimic() )
						{
							floorMagicCreateSpores(nullptr, hit.entity->x, hit.entity->y, parent, 15, SPELL_SPORES);
						}
					}
					else if ( my->sprite == PROJECTILE_SEED_ROOT_SPRITE )
					{
						if ( !hitstats || hit.entity->isInertMimic() )
						{
							floorMagicCreateRoots(hit.entity->x, hit.entity->y, parent, 3, SPELL_ROOTS, 3 * TICKS_PER_SECOND, PARTICLE_TIMER_ACTION_ROOTS_SINGLE_TILE);
						}
					}

					my->removeLightField();
					list_RemoveNode(my->mynode);
				}
			}
			else if ( my->sprite == PROJECTILE_ROCK_SPRITE )
			{
				my->removeLightField();
				list_RemoveNode(my->mynode); // rocks don't stick to walls...
			}
			else if ( my->sprite == PROJECTILE_SEED_POISON_SPRITE )
			{
				floorMagicCreateSpores(nullptr, my->x, my->y, uidToEntity(my->parent), 15, SPELL_SPORES);
				my->removeLightField();
				list_RemoveNode(my->mynode); // rocks don't stick to walls...
			}
			else if ( my->sprite == PROJECTILE_SEED_ROOT_SPRITE )
			{
				floorMagicCreateRoots(my->x, my->y, uidToEntity(my->parent), 3, SPELL_ROOTS, 3 * TICKS_PER_SECOND, PARTICLE_TIMER_ACTION_ROOTS_SINGLE_TILE);
				my->removeLightField();
				list_RemoveNode(my->mynode); // rocks don't stick to walls...
			}
			else
			{
				playSoundEntity(my, 72 + local_rng.rand() % 3, 64);
			}
		}
	}
}
