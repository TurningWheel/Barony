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
			/*int oldSprite = my->sprite;
			my->sprite = 174;
			spawnMagicParticle(my);
			my->sprite = oldSprite;*/
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
			entity->flags[GENIUS] = false;
			entity->setUID(-3);
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

			if ( swimmingtiles[map.tiles[index]] || lavatiles[map.tiles[index]] )
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
		if ( dist != sqrt(ARROW_VELX * ARROW_VELX + ARROW_VELY * ARROW_VELY) || arrowInGround )
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
								spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, 174);
								break;
							default:
								silverDamage = false;
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

					if ( silverDamage )
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
					if ( hitWeaklyOnTarget )
					{
						damage = damage * (std::max(0.1, damagetables[hitstats->type][4] - 0.5));
					}
					else
					{
						damage *= damagetables[hitstats->type][4];
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
					}

					if ( damage > 0 )
					{
						Entity* gib = spawnGib(hit.entity);
						serverSpawnGibForClient(gib);
						playSoundEntity(hit.entity, 28, 64);
						if ( hit.entity->behavior == &actPlayer )
						{
							if ( hit.entity->skill[2] == clientnum )
							{
								camera_shakex += .1;
								camera_shakey += 10;
							}
							else
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
						if ( rand() % 10 == 0 && parent )
						{
							parent->increaseSkill(PRO_RANGED);
						}
					}
					else
					{
						playSoundEntity(hit.entity, 66, 64); //*tink*
						if ( hit.entity->behavior == &actPlayer )
						{
							if ( hit.entity->skill[2] == clientnum )
							{
								camera_shakex += .05;
								camera_shakey += 5;
							}
							else
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
							if ( damage <= (nominalDamage * .7) )
							{
								// weak shot.
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[3733], language[3734], MSG_COMBAT);
							}
							else
							{
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[446], language[448], MSG_COMBAT);
							}
							if ( silverDamage )
							{
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[3743], language[3744], MSG_COMBAT);
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
						else if ( my->arrowQuiverType == QUIVER_HEAVY )
						{
							// no "hit by arrow!" message, let the knockback do the work.
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
					if ( my->arrowPoisonTime > 0 && damage > 0 )
					{
						hitstats->poisonKiller = my->parent;
						hitstats->EFFECTS[EFF_POISONED] = true;
						hitstats->EFFECTS_TIMERS[EFF_POISONED] = my->arrowPoisonTime;
						if ( hit.entity->behavior == &actPlayer )
						{
							Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
							messagePlayerColor(hit.entity->skill[2], color, language[453]);
							serverUpdateEffects(hit.entity->skill[2]);
						}
						statusEffectApplied = true;
					}

					if ( my->arrowQuiverType == QUIVER_FIRE )
					{
						bool burning = hit.entity->flags[BURNING];
						hit.entity->SetEntityOnFire();
						if ( hitstats )
						{
							hitstats->poisonKiller = my->parent;
						}
						if ( !burning && hit.entity->flags[BURNING] )
						{
							if ( parent && parent->behavior == &actPlayer )
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[3739], language[3740], MSG_COMBAT);
							}
							if ( hit.entity->behavior == &actPlayer )
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
								messagePlayerColor(hit.entity->skill[2], color, language[3741]);
							}
							statusEffectApplied = true;
						}
					}

					if ( my->arrowQuiverType == QUIVER_HEAVY && hit.entity->setEffect(EFF_KNOCKBACK, true, 30, false) )
					{
						real_t pushbackMultiplier = 0.6;
						if ( !hit.entity->isMobile() )
						{
							pushbackMultiplier += 0.3;
						}
						if ( hitWeaklyOnTarget )
						{
							pushbackMultiplier -= 0.3;
						}

						if ( hit.entity->behavior == &actMonster )
						{
							if ( parent )
							{
								real_t tangent = atan2(hit.entity->y - parent->y, hit.entity->x - parent->x);
								hit.entity->vel_x = cos(tangent) * pushbackMultiplier;
								hit.entity->vel_y = sin(tangent) * pushbackMultiplier;
								hit.entity->monsterKnockbackVelocity = 0.05;
								hit.entity->monsterKnockbackUID = my->parent;
								hit.entity->monsterKnockbackTangentDir = tangent;
								//hit.entity->lookAtEntity(*parent);
							}
							else
							{
								real_t tangent = atan2(hit.entity->y - my->y, hit.entity->x - my->x);
								hit.entity->vel_x = cos(tangent) * pushbackMultiplier;
								hit.entity->vel_y = sin(tangent) * pushbackMultiplier;
								hit.entity->monsterKnockbackVelocity = 0.05;
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
							if ( hit.entity->skill[2] != clientnum )
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

					if ( damage == 0 && !statusEffectApplied )
					{
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
								messagePlayer(parent->skill[2], language[448]);
							}
						}
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
