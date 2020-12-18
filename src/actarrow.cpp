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
#include "sound.hpp"
#include "interface/interface.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "items.hpp"
#include "magic/magic.hpp"
#include "scores.hpp"
#include "player.hpp"

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
	PROJECTILE_HUNTING_SPRITE
};

void actArrow(Entity* my)
{
	double dist;
	int damage;
	Entity* entity;
	node_t* node;
	double tangent;


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

	if ( my->arrowQuiverType == QUIVER_FIRE || my->sprite == PROJECTILE_FIRE_SPRITE )
	{
		if ( ARROW_LIFE > 1 )
		{
			int intensity = 64;
			if ( abs(ARROW_VELX) < 0.01 && abs(ARROW_VELY) < 0.01 )
			{
				intensity = 192;
			}
			my->light = lightSphereShadow(my->x / 16, my->y / 16, 5, intensity);
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
					my->light = lightSphereShadow(my->x / 16, my->y / 16, 5, intensity);
				}
				else
				{
					my->removeLightField();
					my->light = lightSphereShadow(my->x / 16, my->y / 16, 5, intensity - 16);
				}
				ARROW_FLICKER = 0;
			}

			if ( ARROW_STUCK != 0 )
			{
				Entity* entity = spawnFlame(my, SPRITE_FLAME);
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
			else
			{
				Entity* flame = spawnMagicParticleCustom(my, SPRITE_FLAME, 0.5, 4); // this looks nicer than the spawnFlame :)
				if ( flame )
				{
					flame->flags[SPRITE] = true;
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
				particle->flags[SPRITE] = true;
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
				particle->flags[SPRITE] = true;
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
				particle->flags[SPRITE] = true;
			}
		}
	}
	else if ( my->arrowQuiverType == QUIVER_PIERCE || my->sprite == PROJECTILE_PIERCE_SPRITE )
	{
		if ( ARROW_STUCK == 0 )
		{
			Entity* particle = spawnMagicParticleCustom(my, 158, 0.5, 4);
			if ( particle )
			{
				particle->flags[SPRITE] = true;
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
				particle->flags[SPRITE] = true;
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
				particle->flags[SPRITE] = true;
			}
		}
	}

	if ( multiplayer != CLIENT )
	{
		my->skill[2] = -(1000 + my->arrowShotByWeapon); // invokes actArrow for clients.
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
			if ( my->arrowShotByWeapon == LONGBOW )
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
					entity = (Entity*)node->element;
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

		if ( multiplayer != CLIENT )
		{
			// horizontal motion
			ARROW_VELX = cos(my->yaw) * my->arrowSpeed;
			ARROW_VELY = sin(my->yaw) * my->arrowSpeed;
			ARROW_OLDX = my->x;
			ARROW_OLDY = my->y;
			dist = clipMove(&my->x, &my->y, ARROW_VELX, ARROW_VELY, my);
		}

		bool arrowInGround = false;
		int index = (int)(my->y / 16) * MAPLAYERS + (int)(my->x / 16) * MAPLAYERS * map.height;
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
		if ( arrowSpawnedInsideEntity || dist != sqrt(ARROW_VELX * ARROW_VELX + ARROW_VELY * ARROW_VELY) || arrowInGround )
		{
			if ( arrowInGround )
			{
				ARROW_STUCK = 2;
			}
			else
			{
				ARROW_STUCK = 1;
			}
			serverUpdateEntitySkill(my, 0);
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
			if ( hit.entity != NULL )
			{
				Entity* parent = uidToEntity(my->parent);
				Stat* hitstats = hit.entity->getStats();
				playSoundEntity(my, 72 + rand() % 3, 64);
				if ( hitstats != NULL && hit.entity != parent )
				{
					if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
					{
						// test for friendly fire
						if ( parent && parent->checkFriend(hit.entity) )
						{
							my->removeLightField();
							list_RemoveNode(my->mynode);
							return;
						}
					}

					bool silverDamage = false;
					bool huntingDamage = false;
					if ( my->arrowQuiverType == QUIVER_SILVER )
					{
						switch ( hitstats->type )
						{
							case SKELETON:
							case CREATURE_IMP:
							case GHOUL:
							case DEMON:
							case SUCCUBUS:
							case INCUBUS:
							case VAMPIRE:
							case LICH:
							case LICH_ICE:
							case LICH_FIRE:
							case DEVIL:
								// smite these creatures
								silverDamage = true;
								spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, 981);
								playSoundEntity(hit.entity, 249, 64);
								break;
							default:
								silverDamage = false;
								break;
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

					// do damage
					if ( my->arrowArmorPierce > 0 && AC(hitstats) > 0 )
					{
						if ( my->arrowQuiverType == QUIVER_PIERCE )
						{
							bool oldDefend = hitstats->defending;
							hitstats->defending = false;
							damage = std::max(my->arrowPower - (AC(hitstats) / 2), 0); // pierce half armor not caring about shield
							hitstats->defending = oldDefend;
						}
						else
						{
							damage = std::max(my->arrowPower - (AC(hitstats) / 2), 0); // pierce half armor.
						}
					}
					else
					{
						damage = std::max(my->arrowPower - AC(hitstats), 0); // normal damage.
					}

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
					real_t damageMultiplier = hit.entity->getDamageTableMultiplier(*hitstats, DAMAGE_TABLE_RANGED);
					if ( huntingDamage || silverDamage )
					{
						damageMultiplier = std::max(0.75, damageMultiplier);
					}

					if ( hitWeaklyOnTarget )
					{
						damage = damage * (std::max(0.1, damageMultiplier - 0.5));
					}
					else
					{
						damage *= damageMultiplier;
					}
					/*messagePlayer(0, "My damage: %d, AC: %d, Pierce: %d", my->arrowPower, AC(hitstats), my->arrowArmorPierce);
					messagePlayer(0, "Resolved to %d damage.", damage);*/
					hit.entity->modHP(-damage);
					// write obituary
					if ( parent )
					{
						if ( parent->behavior == &actArrowTrap )
						{
							hit.entity->setObituary(language[1503]);
						}
						else
						{
							parent->killedByMonsterObituary(hit.entity);
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
						if ( rand() % 10 == 0 && parent )
						{
							parent->increaseSkill(PRO_RANGED);
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

					if ( hitstats->HP <= 0 && parent)
					{
						parent->awardXP( hit.entity, true, true );
					}

					// alert the monster
					if ( hit.entity->behavior == &actMonster && parent != nullptr )
					{
						bool alertTarget = true;
						if ( parent->behavior == &actMonster && parent->monsterAllyIndex != -1 )
						{
							if ( hit.entity->behavior == &actMonster && hit.entity->monsterAllyIndex != -1 )
							{
								// if a player ally + hit another ally, don't aggro back
								alertTarget = false;
							}
						}

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
						Entity* ohitentity = hit.entity;
						for ( node = map.creatures->first; node != nullptr && alertAllies; node = node->next )
						{
							entity = (Entity*)node->element;
							if ( entity && entity->behavior == &actMonster && entity != ohitentity )
							{
								Stat* buddystats = entity->getStats();
								if ( buddystats != nullptr )
								{
									if ( entity->checkFriend(hit.entity) )
									{
										if ( entity->monsterState == MONSTER_STATE_WAIT ) // monster is waiting
										{
											tangent = atan2( entity->y - ohitentity->y, entity->x - ohitentity->x );
											lineTrace(ohitentity, ohitentity->x, ohitentity->y, tangent, 1024, 0, false);
											if ( hit.entity == entity )
											{
												entity->monsterAcquireAttackTarget(*parent, MONSTER_STATE_PATH);
											}
										}
									}
								}
							}
						}
						hit.entity = ohitentity;
						if ( parent->behavior == &actPlayer )
						{
							Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
							if ( huntingDamage )
							{
								// plunges into the %s!
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[3750], language[3751], MSG_COMBAT);
							}
							else if ( silverDamage )
							{
								// smites the %s!
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[3743], language[3744], MSG_COMBAT);
							}
							else if ( damage <= (nominalDamage * .7) )
							{
								// weak shot.
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[3733], language[3734], MSG_COMBAT);
							}
							else
							{
								// you shot the %s!
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[446], language[448], MSG_COMBAT);
							}
							if ( my->arrowArmorPierce > 0 && AC(hitstats) > 0 )
							{
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[2513], language[2514], MSG_COMBAT);
							}
						}
					}
					if ( hit.entity->behavior == &actPlayer )
					{
						Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
						if ( silverDamage )
						{
							messagePlayerColor(hit.entity->skill[2], color, language[3745]); // you are smited!
						}
						else if ( huntingDamage )
						{
							messagePlayerColor(hit.entity->skill[2], color, language[3752]); // arrow plunged into you!
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
									messagePlayerColor(hit.entity->skill[2], color, language[2512]);
								}
								else if (my->sprite == PROJECTILE_BOLT_SPRITE )
								{
									// bolt.
									messagePlayerColor(hit.entity->skill[2], color, language[2511]);
								}
								else
								{
									// arrow.
									messagePlayerColor(hit.entity->skill[2], color, language[451]);
								}
							}
							else
							{
								messagePlayerColor(hit.entity->skill[2], color, language[451]);
							}
						}

						if ( my->arrowArmorPierce > 0 && AC(hitstats) > 0 )
						{
							messagePlayerColor(hit.entity->skill[2], color, language[2515]);
						}
					}

					bool statusEffectApplied = false;
					if ( hitstats->HP > 0 )
					{
						if ( my->arrowQuiverType == QUIVER_FIRE )
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
									Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
									if ( hitstats )
									{
										messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[3739], language[3740], MSG_COMBAT);
										if ( hit.entity->behavior == &actMonster )
										{
											achievementObserver.addEntityAchievementTimer(hit.entity, AchievementObserver::BARONY_ACH_PLEASE_HOLD, 150, true, 0);
										}
									}
								}
								if ( hit.entity->behavior == &actPlayer )
								{
									Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
									messagePlayerColor(hit.entity->skill[2], color, language[3741]);
								}
								statusEffectApplied = true;
							}
						}
						else if ( my->arrowQuiverType == QUIVER_KNOCKBACK && hit.entity->setEffect(EFF_KNOCKBACK, true, 30, false) )
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
								Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[3215], language[3214], MSG_COMBAT);

								if ( hit.entity->behavior == &actMonster )
								{
									achievementObserver.awardAchievementIfActive(parent->skill[2], hit.entity, AchievementObserver::BARONY_ACH_PLEASE_HOLD);
								}
							}
							if ( hit.entity->behavior == &actPlayer )
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
								messagePlayerColor(hit.entity->skill[2], color, language[3742]);
							}

							if ( hit.entity->monsterAttack == 0 )
							{
								hit.entity->monsterHitTime = std::max(HITRATE - 12, hit.entity->monsterHitTime);
							}
							statusEffectApplied = true;
						}
						else if ( my->arrowQuiverType == QUIVER_HUNTING && !(hitstats->amulet && hitstats->amulet->type == AMULET_POISONRESISTANCE)
							&& !(hitstats->type == INSECTOID) )
						{
							if ( !hitstats->EFFECTS[EFF_POISONED] )
							{
								hitstats->poisonKiller = my->parent;
								hitstats->EFFECTS[EFF_POISONED] = true;
								hitstats->EFFECTS[EFF_SLOW] = true;
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
									Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
									messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[3747], language[3748], MSG_COMBAT);

									achievementObserver.addEntityAchievementTimer(hit.entity, AchievementObserver::BARONY_ACH_PLEASE_HOLD, 150, true, 0);
								}
								if ( hit.entity->behavior == &actPlayer )
								{
									if ( rand() % 8 == 0 && hit.entity->skill[26] == 0 && !hitstats->EFFECTS[EFF_VOMITING] )
									{
										// maybe vomit
										messagePlayer(hit.entity->skill[2], language[634]);
										if ( hitstats->type != SKELETON
											&& hit.entity->effectShapeshift == NOTHING
											&& hitstats->type != AUTOMATON )
										{
											hit.entity->skill[26] = 140 + rand() % 60; 
										}
									}
									Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
									messagePlayerColor(hit.entity->skill[2], color, language[3749]);
								}
							}
							else if ( hit.entity->behavior == &actPlayer )
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
								messagePlayerColor(hit.entity->skill[2], color, language[451]); // you are hit by an arrow!
							}
						}
					}
					else
					{
						// HP <= 0
						if ( parent && parent->behavior == &actPlayer )
						{
							Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
							messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[692], language[697], MSG_COMBAT);
						}
					}

					if ( damage == 0 && !statusEffectApplied )
					{
						playSoundEntity(hit.entity, 66, 64); //*tink*
						if ( hit.entity->behavior == &actPlayer )
						{
							messagePlayer(hit.entity->skill[2], language[452]); // player notified no damage.
						}
						if ( parent && parent->behavior == &actPlayer )
						{
							if ( hitstats->type == HUMAN )
							{
								if ( hitstats->sex )
								{
									messagePlayer(parent->skill[2], language[449]);
								}
								else
								{
									messagePlayer(parent->skill[2], language[450]);
								}
							}
							else
							{
								messagePlayer(parent->skill[2], language[447]);
							}
						}
					}
					else if ( damage == 0 && statusEffectApplied )
					{
						playSoundEntity(hit.entity, 28, 64);
					}

					// update enemy bar for attacker
					if ( !strcmp(hitstats->name, "") )
					{
						if ( hitstats->type < KOBOLD ) //Original monster count
						{
							updateEnemyBar(parent, hit.entity, language[90 + hitstats->type], hitstats->HP, hitstats->MAXHP);
						}
						else if ( hitstats->type >= KOBOLD ) //New monsters
						{
							updateEnemyBar(parent, hit.entity, language[2000 + (hitstats->type - KOBOLD)], hitstats->HP, hitstats->MAXHP);
						}

					}
					else
					{
						updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP);
					}

				}
				my->removeLightField();
				list_RemoveNode(my->mynode);
			}
			else if ( my->sprite == PROJECTILE_ROCK_SPRITE )
			{
				my->removeLightField();
				list_RemoveNode(my->mynode); // rocks don't stick to walls...
			}
			else
			{
				playSoundEntity(my, 72 + rand() % 3, 64);
			}
		}
	}
}
