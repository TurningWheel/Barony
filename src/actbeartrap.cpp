/*-------------------------------------------------------------------------------

	BARONY
	File: actbeartrap.cpp
	Desc: behavior function for set beartraps

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "messages.hpp"
#include "engine/audio/sound.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "magic/magic.hpp"
#include "scores.hpp"
#include "monster.hpp"
#include "prng.hpp"
#include "paths.hpp"
#include "mod_tools.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define BEARTRAP_CAUGHT my->skill[0]
#define BEARTRAP_STATUS my->skill[11]
#define BEARTRAP_BEATITUDE my->skill[12]
#define BEARTRAP_APPEARANCE my->skill[14]
#define BEARTRAP_IDENTIFIED my->skill[15]
#define BEARTRAP_OWNER my->skill[17]

std::map<Uint32, MonsterTrapIgnoreEntities_t> monsterTrapIgnoreEntities;

void actBeartrap(Entity* my)
{
	int i;
	if ( my->sprite == 667 )
	{	
		my->roll = 0;
		my->z = 6.75;
	}

	if ( my->ticks == 1 )
	{
		my->createWorldUITooltip();
	}

	if ( multiplayer == CLIENT )
	{
		return;
	}

	// undo beartrap
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if ( selectedEntity[i] == my || client_selected[i] == my )
		{
			if ( inrange[i] )
			{
				Entity* entity = newEntity(-1, 1, map.entities, nullptr); //Item entity.
				entity->flags[INVISIBLE] = true;
				entity->flags[UPDATENEEDED] = true;
				entity->flags[PASSABLE] = true;
				entity->x = my->x;
				entity->y = my->y;
				entity->z = my->z;
				entity->sizex = my->sizex;
				entity->sizey = my->sizey;
				entity->yaw = my->yaw;
				entity->pitch = my->pitch;
				entity->roll = PI / 2;
				entity->behavior = &actItem;
				entity->skill[10] = TOOL_BEARTRAP;
				entity->skill[11] = BEARTRAP_STATUS;
				entity->skill[12] = BEARTRAP_BEATITUDE;
				entity->skill[13] = 1;
				entity->skill[14] = BEARTRAP_APPEARANCE;
				entity->skill[15] = BEARTRAP_IDENTIFIED;
				entity->itemNotMoving = 1;
				entity->itemNotMovingClient = 1;
				messagePlayer(i, MESSAGE_INTERACTION, Language::get(1300));
				list_RemoveNode(my->mynode);
				return;
			}
		}
	}

	if ( BEARTRAP_CAUGHT == 1 )
	{
		return;
	}

	MonsterTrapIgnoreEntities_t* trapProps = nullptr;
	if ( my->parent != 0 && monsterTrapIgnoreEntities.find(my->getUID()) != monsterTrapIgnoreEntities.end() )
	{
		trapProps = &monsterTrapIgnoreEntities[my->getUID()];
	}

	// launch beartrap
	node_t* node;
	Entity* parent = uidToEntity(my->parent);
	for ( node = map.creatures->first; node != nullptr; node = node->next )
	{
		Entity* entity = (Entity*)node->element;
		if ( my->parent == entity->getUID() )
		{
			continue;
		}
		if ( entity->behavior == &actMonster || entity->behavior == &actPlayer )
		{
			Stat* stat = entity->getStats();
			if ( stat )
			{
				if ( (parent && parent->checkFriend(entity)) )
				{
					continue;
				}
				if ( trapProps && trapProps->parent == my->parent 
					&& trapProps->ignoreEntities.find(entity->getUID()) != trapProps->ignoreEntities.end() )
				{
					continue;
				}
				if ( stat->type == GYROBOT || entity->isUntargetableBat() )
				{
					continue;
				}
				if ( entity->isInertMimic() )
				{
					continue;
				}
				if ( !parent && BEARTRAP_OWNER >= 0 )
				{
					if ( entity->behavior == &actPlayer )
					{
						continue; // players won't trigger if owner dead.
					}
					else if ( entity->monsterAllyGetPlayerLeader() )
					{
						continue; // player followers won't trigger if owner dead.
					}
				}
				if ( entityDist(my, entity) < 6.5 )
				{
					entity->setEffect(EFF_PARALYZED, true, 200, false);
					entity->setEffect(EFF_BLEEDING, true, 300, false);
					int damage = 10 + 3 * (BEARTRAP_STATUS + BEARTRAP_BEATITUDE);
					if ( parent )
					{
						stat->bleedInflictedBy = static_cast<Sint32>(parent->getUID());
						//damage += trapperStat->PROFICIENCIES[PRO_LOCKPICKING] / 20;
					}
					int oldHP = stat->HP;
					//messagePlayer(0, "dmg: %d", damage);
					entity->modHP(-damage);
					//// alert the monster! DOES NOT WORK DURING PARALYZE.
					//if ( entity->behavior == &actMonster && entity->monsterState != MONSTER_STATE_ATTACK && (stat->type < LICH || stat->type >= SHOPKEEPER) )
					//{
					//	Entity* attackTarget = uidToEntity(my->parent);
					//	if ( attackTarget )
					//	{
					//		entity->monsterAcquireAttackTarget(*attackTarget, MONSTER_STATE_PATH);
					//	}
					//}
					
					if ( parent && parent->behavior == &actPlayer )
					{
						Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_BEARTRAP_TRAPPED, TOOL_BEARTRAP, 1);
						if ( stat->HP < oldHP )
						{
							Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_BEARTRAP_DMG, TOOL_BEARTRAP, oldHP - stat->HP);
						}
					}
					
					// set obituary
					entity->updateEntityOnHit(parent, true);
					entity->setObituary(Language::get(1504));
					stat->killer = KilledBy::TRAP_BEAR;

					if ( stat->HP <= 0 && oldHP > 0 )
					{
						if ( parent )
						{
							parent->awardXP( entity, true, true );
						}
					}
					if ( entity->behavior == &actPlayer )
					{
						int player = entity->skill[2];
						Uint32 color = makeColorRGB(255, 0, 0);
						messagePlayerColor(player, MESSAGE_STATUS, color, Language::get(454));
						if ( !players[player]->isLocalPlayer() )
						{
							serverUpdateEffects(player);
						}
						if ( players[player]->isLocalPlayer() )
						{
							cameravars[entity->skill[2]].shakex += .1;
							cameravars[entity->skill[2]].shakey += 10;
						}
						else if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
						{
							strcpy((char*)net_packet->data, "SHAK");
							net_packet->data[4] = 10; // turns into .1
							net_packet->data[5] = 10;
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}
					}
					else if ( parent && parent->behavior == &actPlayer )
					{
						int player = parent->skill[2];
						if ( player >= 0 )
						{
							if ( oldHP > 0 )
							{
								if ( entityDist(my, parent) >= 64 && entityDist(my, parent) < 128 )
								{
									messagePlayer(player, MESSAGE_HINT, Language::get(2521));
								}
								else
								{
									messagePlayer(player, MESSAGE_HINT, Language::get(2522));
								}
								if ( local_rng.rand() % 10 == 0 )
								{
									parent->increaseSkill(PRO_LOCKPICKING);
								}
								//if ( local_rng.rand() % 5 == 0 )
								//{
								//	parent->increaseSkill(PRO_RANGED);
								//}
							}
							// update enemy bar for attacker
							if ( !strcmp(stat->name, "") )
							{
								updateEnemyBar(parent, entity, getMonsterLocalizedName(stat->type).c_str(), stat->HP, stat->MAXHP,
									false, DamageGib::DMG_DEFAULT);
							}
							else
							{
								updateEnemyBar(parent, entity, stat->name, stat->HP, stat->MAXHP,
									false, DamageGib::DMG_DEFAULT);
							}
						}
					}
					playSoundEntity(my, 76, 64);
					playSoundEntity(entity, 28, 64);
					Entity* gib = spawnGib(entity);
					serverSpawnGibForClient(gib);

					--BEARTRAP_STATUS;
					if ( BEARTRAP_STATUS < DECREPIT )
					{
						// make first arm
						entity = newEntity(668, 1, map.entities, nullptr); //Special effect entity.
						entity->behavior = &actBeartrapLaunched;
						entity->flags[PASSABLE] = true;
						entity->flags[UPDATENEEDED] = true;
						entity->x = my->x;
						entity->y = my->y;
						entity->z = my->z + 1;
						entity->yaw = my->yaw;
						entity->pitch = my->pitch;
						entity->roll = 0;

						// and then the second
						entity = newEntity(668, 1, map.entities, nullptr); //Special effect entity.
						entity->behavior = &actBeartrapLaunched;
						entity->flags[PASSABLE] = true;
						entity->flags[UPDATENEEDED] = true;
						entity->x = my->x;
						entity->y = my->y;
						entity->z = my->z + 1;
						entity->yaw = my->yaw;
						entity->pitch = my->pitch;
						entity->roll = PI;
						list_RemoveNode(my->mynode);
					}
					else
					{
						BEARTRAP_CAUGHT = 1;
						my->sprite = 667;
						serverUpdateEntitySkill(my, 0);
					}
					return;
				}
			}
		}
	}
}

void actBeartrapLaunched(Entity* my)
{
	if ( my->ticks >= 200 )
	{
		list_RemoveNode(my->mynode);
		return;
	}
}

#define BOMB_PLACEMENT my->skill[16]
#define BOMB_PLAYER_OWNER my->skill[17]
#define BOMB_ENTITY_ATTACHED_TO my->skill[18]
#define BOMB_ENTITY_ATTACHED_START_HP my->skill[19]
#define BOMB_DIRECTION my->skill[20]
#define BOMB_ITEMTYPE my->skill[21]
#define BOMB_TRIGGER_TYPE my->skill[22]
#define BOMB_CHEST_STATUS my->skill[23]
#define BOMB_HIT_BY_PROJECTILE my->skill[24]
#define BOMB_CURSED_RNG_EXPLODE my->skill[25]

void bombDoEffect(Entity* my, Entity* triggered, real_t entityDistance, bool spawnMagicOnTriggeredMonster, bool hitByAOE )
{
	if ( !triggered || !my )
	{
		return;
	}
	Entity* parent = uidToEntity(my->parent);
	Stat* stat = triggered->getStats();
	Stat* parentStats = nullptr;
	if ( parent )
	{
		parentStats = parent->getStats();
	}
	int damage = 0;
	//messagePlayer(0, "dmg: %d", damage);
	int doSpell = SPELL_NONE;
	bool doVertical = false;
	switch ( BOMB_ITEMTYPE )
	{
		case TOOL_BOMB:
			doSpell = SPELL_FIREBALL;
			damage = 5;
			if ( parentStats )
			{
				damage += std::max(0, parent->getPER() / 2);
			}
			break;
		case TOOL_SLEEP_BOMB:
			doSpell = SPELL_SLEEP;
			damage = 0;
			break;
		case TOOL_FREEZE_BOMB:
			doSpell = SPELL_COLD;
			damage = 5;
			if ( parentStats )
			{
				damage += std::max(0, parent->getPER() / 4);
			}
			break;
		case TOOL_TELEPORT_BOMB:
			doSpell = SPELL_TELEPORTATION;
			damage = 0;
			break;
		default:
			break;
	}
	if ( BOMB_PLACEMENT == Item::ItemBombPlacement::BOMB_FLOOR )
	{
		doVertical = true;
	}

	int oldHP = stat->HP;
	if ( stat )
	{
		damage *= Entity::getDamageTableMultiplier(triggered, *stat, DAMAGE_TABLE_MAGIC); // reduce/increase by magic table.
	}
	bool wasAsleep = false;
	if ( stat )
	{
		wasAsleep = stat->EFFECTS[EFF_ASLEEP];
	}
	if ( damage > 0 )
	{
		triggered->modHP(-damage);
	}

	// stumbled into the trap!
	Uint32 color = makeColorRGB(0, 255, 0);
	if ( parent && parent->behavior == &actPlayer && triggered != parent )
	{
		if ( !hitByAOE )
		{
			messagePlayerMonsterEvent(parent->skill[2], color, *triggered->getStats(), Language::get(3498), Language::get(3499), MSG_TOOL_BOMB, my);
		}
		else
		{
			messagePlayerMonsterEvent(parent->skill[2], color, *triggered->getStats(), Language::get(3613), Language::get(3614), MSG_TOOL_BOMB, my);
		}
	}
	if ( triggered->behavior == &actPlayer )
	{
		int player = triggered->skill[2];
		Uint32 color = makeColorRGB(255, 0, 0);
		// you stumbled into the trap!
		if ( !hitByAOE )
		{
			messagePlayerColor(player, MESSAGE_STATUS, color, Language::get(3497), items[BOMB_ITEMTYPE].getIdentifiedName());
		}
		else
		{
			messagePlayerColor(player, MESSAGE_STATUS, color, Language::get(3612), items[BOMB_ITEMTYPE].getIdentifiedName());
		}
	}

	if ( parent && parent->behavior == &actPlayer )
	{
		if ( triggered && (triggered == parent || parent->checkFriend(triggered)) )
		{
			Compendium_t::Events_t::eventUpdate(parent->skill[2],
				Compendium_t::CPDM_BOMB_DETONATED_ALLY, (ItemType)BOMB_ITEMTYPE, 1);
		}
		else
		{
			Compendium_t::Events_t::eventUpdate(parent->skill[2],
				Compendium_t::CPDM_BOMB_DETONATED, (ItemType)BOMB_ITEMTYPE, 1);
		}
	}

	if ( doSpell == SPELL_TELEPORTATION )
	{
		if ( triggered->isBossMonster() )
		{
			// no effect.
			if ( parent && parent->behavior == &actPlayer )
			{
				Uint32 color = makeColorRGB(255, 0, 0);
				messagePlayerMonsterEvent(parent->skill[2], color, *triggered->getStats(), Language::get(3603), Language::get(3604), MSG_COMBAT);
			}
			return;
		}
		std::vector<Entity*> goodspots;
		bool teleported = false;
		for ( node_t* node = map.entities->first; node != NULL; node = node->next )
		{
			Entity* entity = (Entity*)node->element;
			if ( entity && entity != my && entity->behavior == &actBomb )
			{
				if ( entity->skill[21] == TOOL_TELEPORT_BOMB && entity->skill[22] == Item::ItemBombTriggerType::BOMB_TELEPORT_RECEIVER )
				{
					// receiver location.
					goodspots.push_back(entity);
				}
			}
		}

		std::vector<Entity*> parentgoodspots; // prioritize traps from the player owner, instead of other player's traps.
		for ( auto it = goodspots.begin(); it != goodspots.end(); ++it )
		{
			Entity* entry = *it;
			if ( entry && (entry->parent == my->parent) )
			{
				parentgoodspots.push_back(entry);
			}
		}

		if ( !parentgoodspots.empty() )
		{
			Entity* targetLocation = parentgoodspots[local_rng.rand() % parentgoodspots.size()];
			teleported = triggered->teleportAroundEntity(targetLocation, 2);
		}
		else if ( !goodspots.empty() )
		{
			Entity* targetLocation = goodspots[local_rng.rand() % goodspots.size()];
			teleported = triggered->teleportAroundEntity(targetLocation, 2);
		}
		else
		{
			teleported = triggered->teleportRandom(); // woosh!
		}

		if ( teleported )
		{
			createParticleErupt(my, 593);
			serverSpawnMiscParticles(my, PARTICLE_EFFECT_ERUPT, 593);
			if ( parent && parent->behavior == &actPlayer )
			{
				// whisked away!
				if ( triggered != parent )
				{
					Uint32 color = makeColorRGB(0, 255, 0);
					messagePlayerMonsterEvent(parent->skill[2], color, *triggered->getStats(), Language::get(3601), Language::get(3602), MSG_COMBAT);
				}
			}
			if ( triggered->behavior == &actPlayer )
			{
				Uint32 color = makeColorRGB(255, 255, 255);
				messagePlayerColor(triggered->skill[2], MESSAGE_STATUS, color, Language::get(3611));
				achievementObserver.playerAchievements[triggered->skill[2]].checkPathBetweenObjects(triggered, my, AchievementObserver::BARONY_ACH_WONDERFUL_TOYS);
			}

			if ( triggered->behavior == &actMonster )
			{
				triggered->monsterReleaseAttackTarget();
			}

			createParticleErupt(triggered, 593);
			serverSpawnMiscParticlesAtLocation(triggered->x / 16, triggered->y / 16, 0, PARTICLE_EFFECT_ERUPT, 593);
		}
		else
		{
			if ( parent && parent->behavior == &actPlayer && triggered != parent )
			{
				Uint32 color = makeColorRGB(255, 0, 0);
				messagePlayerMonsterEvent(parent->skill[2], color, *triggered->getStats(), Language::get(3615), Language::get(3616), MSG_COMBAT);
			}
		}
		return;
	}
	else if ( doSpell != SPELL_NONE )
	{
		if ( !stat || (stat && stat->HP > 0) )
		{
			Entity* spell = castSpell(my->getUID(), getSpellFromID(doSpell), false, true);
			spell->parent = my->parent;
			spell->x = triggered->x;
			spell->y = triggered->y;
			if ( !doVertical )
			{
				real_t speed = 1.f;
				real_t ticksToHit = (entityDistance / speed);
				/*real_t predictx = triggered->x + (triggered->vel_x * ticksToHit);
				real_t predicty = triggered->y + (triggered->vel_y * ticksToHit);
				double tangent = atan2(predicty - my->y, predictx - my->x);*/
				double tangent = atan2(triggered->y - my->y, triggered->x - my->x);
				spell->yaw = tangent;
				spell->vel_x = speed * cos(spell->yaw);
				spell->vel_y = speed * sin(spell->yaw);
			}
			else
			{
				spell->x = my->x;
				spell->y = my->y;
				real_t speed = 3.f;
				real_t ticksToHit = (entityDistance / speed);
				real_t predictx = triggered->x + (triggered->vel_x * ticksToHit);
				real_t predicty = triggered->y + (triggered->vel_y * ticksToHit);
				double tangent = atan2(predicty - my->y, predictx - my->x);
				spell->yaw = tangent;
				spell->vel_z = -2.f;
				spell->vel_x = speed * cos(spell->yaw);
				spell->vel_y = speed * sin(spell->yaw);
				spell->pitch = atan2(spell->vel_z, speed);
				spell->actmagicIsVertical = MAGIC_ISVERTICAL_XYZ;
			}
			spell->actmagicCastByTinkerTrap = 1;
			if ( BOMB_TRIGGER_TYPE == Item::ItemBombTriggerType::BOMB_TRIGGER_ALL )
			{
				spell->actmagicTinkerTrapFriendlyFire = 1;
				if ( triggered == parent )
				{
					spell->parent = 0;
				}
			}
			spell->skill[5] = 10; // travel time
		}
	}
	// set obituary
	triggered->setObituary(Language::get(3496));
	triggered->updateEntityOnHit(parent, true);
	stat->killer = KilledBy::TRAP_BOMB;

	if ( stat->HP < oldHP )
	{
		if ( parent && parent->behavior == &actPlayer )
		{
			Compendium_t::Events_t::eventUpdate(parent->skill[2],
				Compendium_t::CPDM_BOMB_DMG, (ItemType)BOMB_ITEMTYPE, oldHP - stat->HP);
		}
	}

	if ( stat->HP <= 0 && oldHP > 0 )
	{
		if ( parent )
		{
			parent->awardXP(triggered, true, true);
			if ( stat->type == MINOTAUR )
			{
				if ( parent->behavior == &actPlayer )
				{
					steamAchievementClient(parent->skill[2], "BARONY_ACH_TIME_TO_PLAN");
				}
			}
		}
		else
		{
			if ( achievementObserver.checkUidIsFromPlayer(my->parent) >= 0 )
			{
				steamAchievementClient(achievementObserver.checkUidIsFromPlayer(my->parent), "BARONY_ACH_TAKING_WITH");
			}
		}
	}

	if ( triggered->behavior == &actPlayer )
	{
		int player = triggered->skill[2];
		
		if ( players[player]->isLocalPlayer() )
		{
			cameravars[triggered->skill[2]].shakex += .1;
			cameravars[triggered->skill[2]].shakey += 10;
		}
		else if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
		{
			strcpy((char*)net_packet->data, "SHAK");
			net_packet->data[4] = 10; // turns into .1
			net_packet->data[5] = 10;
			net_packet->address.host = net_clients[player - 1].host;
			net_packet->address.port = net_clients[player - 1].port;
			net_packet->len = 6;
			sendPacketSafe(net_sock, -1, net_packet, player - 1);
		}
	}
	if ( parent && parent != triggered && parent->behavior == &actPlayer )
	{
		int player = parent->skill[2];
		if ( player >= 0 )
		{
			double tangent = atan2(parent->y - my->y, parent->x - my->x);
			lineTraceTarget(my, my->x, my->y, tangent, 128, 0, false, parent);
			if ( hit.entity != parent )
			{
				if ( entityDist(my, parent) >= 64 && entityDist(my, parent) < 128 )
				{
					messagePlayer(player, MESSAGE_HINT, Language::get(3494));
				}
				else
				{
					messagePlayer(player, MESSAGE_HINT, Language::get(3495));
				}
			}
			if ( triggered->behavior == &actMonster )
			{
				if ( oldHP > 0 && stat->HP == 0 ) // got a kill
				{
					if ( local_rng.rand() % 5 == 0 )
					{
						parent->increaseSkill(PRO_LOCKPICKING);
					}
				}
				else if ( oldHP > stat->HP )
				{
					if ( local_rng.rand() % 20 == 0 ) // wounded
					{
						parent->increaseSkill(PRO_LOCKPICKING);
					}
				}
				else if( local_rng.rand() % 20 == 0) // any other effect
				{
					parent->increaseSkill(PRO_LOCKPICKING);
				}

				if ( !achievementObserver.playerAchievements[player].bombTrack )
				{
					achievementObserver.addEntityAchievementTimer(triggered, AchievementObserver::BARONY_ACH_BOMBTRACK, 250, false, 1);
					achievementObserver.awardAchievementIfActive(player, triggered, AchievementObserver::BARONY_ACH_BOMBTRACK);
				}
				if ( !achievementObserver.playerAchievements[player].calmLikeABomb )
				{
					if ( BOMB_ITEMTYPE == TOOL_SLEEP_BOMB )
					{
						achievementObserver.addEntityAchievementTimer(triggered, AchievementObserver::BARONY_ACH_CALM_LIKE_A_BOMB, 50 * 15, true, 0);
					}
					else if ( wasAsleep && damage > 0 )
					{
						achievementObserver.awardAchievementIfActive(player, triggered, AchievementObserver::BARONY_ACH_CALM_LIKE_A_BOMB);
					}
				}
			}
			// update enemy bar for attacker
			if ( damage > 0 )
			{
				if ( !strcmp(stat->name, "") )
				{
					updateEnemyBar(parent, triggered, getMonsterLocalizedName(stat->type).c_str(), stat->HP, stat->MAXHP,
						false, DamageGib::DMG_DEFAULT);
				}
				else
				{
					updateEnemyBar(parent, triggered, stat->name, stat->HP, stat->MAXHP,
						false, DamageGib::DMG_DEFAULT);
				}
				Entity* gib = spawnGib(triggered);
				serverSpawnGibForClient(gib);
			}
		}
	}
}

void actBomb(Entity* my)
{
	if ( my->ticks == 1 )
	{
		my->createWorldUITooltip();
	}
	my->removeLightField();
	if ( multiplayer == CLIENT )
	{
		if ( BOMB_TRIGGER_TYPE == Item::ItemBombTriggerType::BOMB_TELEPORT_RECEIVER )
		{
			my->spawnAmbientParticles(25, 576, 10 + local_rng.rand() % 40, 1.0, false);
			my->light = addLight(my->x / 16, my->y / 16, "trap_teleport");
		}
		return;
	}
	else
	{
		my->skill[2] = -15;
	}

	// undo bomb
	for ( int i = 0; i < MAXPLAYERS; i++ )
	{
		if ( selectedEntity[i] == my || client_selected[i] == my )
		{
			if ( inrange[i] )
			{
				Entity* entity = newEntity(-1, 1, map.entities, nullptr); //Item entity.
				entity->flags[INVISIBLE] = true;
				entity->flags[UPDATENEEDED] = true;
				entity->flags[PASSABLE] = true;
				entity->x = my->x;
				entity->y = my->y;
				entity->z = my->z;
				entity->sizex = my->sizex;
				entity->sizey = my->sizey;
				entity->yaw = my->yaw;
				entity->pitch = my->pitch;
				entity->roll = 3 * PI / 2;
				entity->behavior = &actItem;
				entity->skill[10] = BOMB_ITEMTYPE;
				entity->skill[11] = BEARTRAP_STATUS;
				entity->skill[12] = BEARTRAP_BEATITUDE;
				entity->skill[13] = 1;
				entity->skill[14] = BEARTRAP_APPEARANCE;
				entity->skill[15] = BEARTRAP_IDENTIFIED;
				if ( BOMB_PLACEMENT == Item::ItemBombPlacement::BOMB_FLOOR )
				{
					// don't fall down
					entity->itemNotMoving = 1;
					entity->itemNotMovingClient = 1;
					serverUpdateEntitySkill(entity, 18); //update both the above flags.
					serverUpdateEntitySkill(entity, 19);
				}
				else
				{
					entity->itemNotMoving = 0;
					entity->itemNotMovingClient = 0;
				}
				messagePlayer(i, MESSAGE_INTERACTION, Language::get(3600), items[BOMB_ITEMTYPE].getIdentifiedName());
				list_RemoveNode(my->mynode);
				return;
			}
		}
	}

	if ( my->isInteractWithMonster() )
	{
		Entity* monsterInteracting = uidToEntity(my->interactedByMonster);
		if ( monsterInteracting && monsterInteracting->getMonsterTypeFromSprite() == GYROBOT )
		{
			if ( monsterInteracting->monsterAllyGetPlayerLeader() )
			{
				Item* tmp = newItemFromEntity(my);
				if ( tmp )
				{
					tmp->applyLockpick(monsterInteracting->monsterAllyIndex, *my);
					free(tmp);
				}
			}
		}
		my->clearMonsterInteract();
	}

	if ( BOMB_ITEMTYPE == TOOL_TELEPORT_BOMB && BOMB_TRIGGER_TYPE == Item::ItemBombTriggerType::BOMB_TELEPORT_RECEIVER )
	{
		my->spawnAmbientParticles(25, 576, 10 + local_rng.rand() % 40, 1.0, false);
		my->light = addLight(my->x / 16, my->y / 16, "trap_teleport");
		my->sprite = 899;
		return;
	}
	else if ( BOMB_ITEMTYPE == TOOL_TELEPORT_BOMB )
	{
		my->sprite = 884;
	}

	// launch bomb
	std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 1);
	std::vector<Entity*> entitiesWithinRadius;
	for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
	{
		list_t* currentList = *it;
		node_t* node;
		for ( node = currentList->first; node != nullptr; node = node->next )
		{
			entitiesWithinRadius.push_back((Entity*)node->element);
		}
	}


	Entity* triggered = nullptr;
	real_t entityDistance = 0.f;
	bool bombExplodeAOETargets = false;


	int explosionSprite = 49;
	if ( BOMB_ITEMTYPE == TOOL_TELEPORT_BOMB )
	{
		explosionSprite = 145;
	}
	else if ( BOMB_ITEMTYPE == TOOL_FREEZE_BOMB )
	{
		explosionSprite = 135;
	}
	else if ( BOMB_ITEMTYPE == TOOL_SLEEP_BOMB )
	{
		explosionSprite = 0;
	}

	bool cursedExplode = false;
	if ( BOMB_CURSED_RNG_EXPLODE )
	{
		if ( my->ticks >= TICKS_PER_SECOND / 2 )
		{
			if ( my->ticks % TICKS_PER_SECOND == (5 * (my->getUID() % 10)) ) // randomly explode
			{
				cursedExplode = true;
			}
		}
	}

	if ( BOMB_ENTITY_ATTACHED_TO != 0 || BOMB_HIT_BY_PROJECTILE == 1 
		|| BOMB_PLACEMENT == Item::ItemBombPlacement::BOMB_WALL
		|| cursedExplode )
	{
		Entity* onEntity = uidToEntity(static_cast<Uint32>(BOMB_ENTITY_ATTACHED_TO));
		bool shouldExplode = false;
		if ( BOMB_PLACEMENT == Item::ItemBombPlacement::BOMB_WALL )
		{
			int checkx = my->x;
			int checky = my->y;
			switch ( BOMB_DIRECTION )
			{
				case Item::ItemBombFacingDirection::BOMB_EAST:
					checkx -= 8;
					break;
				case Item::ItemBombFacingDirection::BOMB_WEST:
					checkx += 8;
					break;
				case Item::ItemBombFacingDirection::BOMB_SOUTH:
					checky -= 8;
					break;
				case Item::ItemBombFacingDirection::BOMB_NORTH:
					checky += 8;
					break;
				default:
					break;
			}
			checkx = checkx >> 4;
			checky = checky >> 4;
			if ( !map.tiles[OBSTACLELAYER + checky * MAPLAYERS + checkx * MAPLAYERS * map.height] )   // wall
			{
				shouldExplode = true;
			}
			else if ( BOMB_HIT_BY_PROJECTILE == 1 )
			{
				shouldExplode = true;
			}
		}
		else if ( onEntity )
		{
			if ( onEntity->behavior == &actDoor )
			{
				if ( onEntity->doorHealth < BOMB_ENTITY_ATTACHED_START_HP || onEntity->flags[PASSABLE] || cursedExplode
					|| BOMB_HIT_BY_PROJECTILE == 1 )
				{
					if ( onEntity->doorHealth > 0 )
					{
						onEntity->doorHandleDamageMagic(50, *my, uidToEntity(my->parent));
					}
					shouldExplode = true;
				}
			}
			else if ( onEntity->behavior == &actMonster && onEntity->getMonsterTypeFromSprite() == MIMIC )
			{
				if ( !onEntity->isInertMimic() || BOMB_HIT_BY_PROJECTILE == 1 || cursedExplode )
				{
					if ( onEntity->isInertMimic() )
					{
						onEntity->chestHandleDamageMagic(20, *my, uidToEntity(my->parent));
					}
					shouldExplode = true;
				}
			}
			else if ( onEntity->behavior == &actChest )
			{
				if ( onEntity->skill[3] < BOMB_ENTITY_ATTACHED_START_HP || BOMB_CHEST_STATUS != onEntity->skill[1]
					|| cursedExplode
					|| BOMB_HIT_BY_PROJECTILE == 1 )
				{
					if ( onEntity->skill[3] > 0 )
					{
						if ( BOMB_ITEMTYPE == TOOL_BOMB ) // fire bomb do more.
						{
							onEntity->chestHandleDamageMagic(50, *my, uidToEntity(my->parent));
						}
						else
						{
							onEntity->chestHandleDamageMagic(20, *my, uidToEntity(my->parent));
						}
					}
					shouldExplode = true;
				}
			}
			else if ( onEntity->behavior == &actColliderDecoration )
			{
				if ( onEntity->colliderCurrentHP < BOMB_ENTITY_ATTACHED_START_HP
					|| cursedExplode
					|| BOMB_HIT_BY_PROJECTILE == 1 )
				{
					if ( onEntity->colliderCurrentHP > 0 )
					{
						if ( BOMB_ITEMTYPE == TOOL_BOMB ) // fire bomb do more.
						{
							onEntity->colliderHandleDamageMagic(50, *my, uidToEntity(my->parent));
						}
						else
						{
							onEntity->colliderHandleDamageMagic(20, *my, uidToEntity(my->parent));
						}
					}
					shouldExplode = true;
				}
			}
		}
		else
		{
			shouldExplode = true; // my attached entity died.
		}

		if ( cursedExplode )
		{
			shouldExplode = true;
		}

		if ( shouldExplode )
		{
			if ( onEntity )
			{
				spawnExplosionFromSprite(explosionSprite, onEntity->x, onEntity->y, onEntity->z);
			}
			else
			{
				spawnExplosionFromSprite(explosionSprite, my->x, my->y, my->z);
			}
			bombExplodeAOETargets = true;
			BOMB_TRIGGER_TYPE = Item::ItemBombTriggerType::BOMB_TRIGGER_ALL;
		}
	}

	MonsterTrapIgnoreEntities_t* trapProps = nullptr;
	if ( my->parent != 0 && monsterTrapIgnoreEntities.find(my->getUID()) != monsterTrapIgnoreEntities.end() )
	{
		trapProps = &monsterTrapIgnoreEntities[my->getUID()];
	}

	for ( auto it = entitiesWithinRadius.begin(); it != entitiesWithinRadius.end() && !triggered; ++it )
	{
		Entity* entity = *it;
		if ( !entity )
		{
			continue;
		}
		if ( my->parent == entity->getUID() && !(BOMB_TRIGGER_TYPE == Item::ItemBombTriggerType::BOMB_TRIGGER_ALL) )
		{
			continue;
		}
		if ( entity->behavior == &actMonster || entity->behavior == &actPlayer )
		{
			Stat* stat = entity->getStats();
			if ( stat )
			{
				Entity* parent = uidToEntity(my->parent);
				if ( parent && parent->checkFriend(entity) && !(BOMB_TRIGGER_TYPE == Item::ItemBombTriggerType::BOMB_TRIGGER_ALL) )
				{
					continue;
				}
				if ( trapProps && trapProps->parent == my->parent && trapProps->ignoreEntities.find(entity->getUID()) != trapProps->ignoreEntities.end()
					 && !(BOMB_TRIGGER_TYPE == Item::ItemBombTriggerType::BOMB_TRIGGER_ALL) )
				{
					continue;
				}
				if ( stat->type == GYROBOT || entity->isUntargetableBat() )
				{
					continue;
				}
				if ( entity->isInertMimic() )
				{
					continue;
				}
				if ( !parent && BOMB_PLAYER_OWNER >= 0 && !(BOMB_TRIGGER_TYPE == Item::ItemBombTriggerType::BOMB_TRIGGER_ALL) )
				{
					if ( entity->behavior == &actPlayer )
					{
						continue; // players won't trigger if owner dead.
					}
					else if ( entity->monsterAllyGetPlayerLeader() )
					{
						continue; // player followers won't trigger if owner dead.
					}
				}
				if ( BOMB_PLACEMENT == Item::ItemBombPlacement::BOMB_FLOOR )
				{
					entityDistance = entityDist(my, entity);
					if ( entityDistance < 6.5 )
					{
						spawnExplosionFromSprite(explosionSprite, my->x - 4 + local_rng.rand() % 9, my->y + local_rng.rand() % 9, my->z - 2);
						triggered = entity;
					}
				}
				else if ( bombExplodeAOETargets )
				{
					Entity* onEntity = uidToEntity(static_cast<Uint32>(BOMB_ENTITY_ATTACHED_TO));
					if ( onEntity )
					{
						entityDistance = entityDist(onEntity, entity);
						if ( entityDistance < STRIKERANGE )
						{
							spawnExplosionFromSprite(explosionSprite, entity->x, entity->y, entity->z);
							bombDoEffect(my, entity, entityDistance, true, true);
						}
					}
					else
					{
						entityDistance = entityDist(my, entity);
						if ( entityDistance < STRIKERANGE )
						{
							spawnExplosionFromSprite(explosionSprite, entity->x, entity->y, entity->z);
							bombDoEffect(my, entity, entityDistance, true, true);
						}
					}
				}
				else
				{
					real_t oldx = my->x;
					real_t oldy = my->y;
					// pretend the bomb is in the center of the tile it's facing.
					switch ( BOMB_DIRECTION )
					{
						case Item::ItemBombFacingDirection::BOMB_EAST:
							my->x += 8;
							entityDistance = entityDist(my, entity);
							if ( entityDistance < 12 )
							{
								triggered = entity;
								spawnExplosionFromSprite(explosionSprite, my->x - local_rng.rand() % 9, my->y - 4 + local_rng.rand() % 9, my->z);
							}
							break;
						case Item::ItemBombFacingDirection::BOMB_WEST:
							my->x -= 8;
							entityDistance = entityDist(my, entity);
							if ( entityDistance < 12 )
							{
								triggered = entity;
								spawnExplosionFromSprite(explosionSprite, my->x + local_rng.rand() % 9, my->y - 4 + local_rng.rand() % 9, my->z);
							}
							break;
						case Item::ItemBombFacingDirection::BOMB_SOUTH:
							my->y += 8;
							entityDistance = entityDist(my, entity);
							if ( entityDistance < 12 )
							{
								triggered = entity;
								spawnExplosionFromSprite(explosionSprite, my->x - 4 + local_rng.rand() % 9, my->y - local_rng.rand() % 9, my->z);
							}
							break;
						case Item::ItemBombFacingDirection::BOMB_NORTH:
							my->y -= 8;
							entityDistance = entityDist(my, entity);
							if ( entityDistance < 12 )
							{
								triggered = entity;
								spawnExplosionFromSprite(explosionSprite, my->x - 4 + local_rng.rand() % 9, my->y + local_rng.rand() % 9, my->z);
							}
							break;
						default:
							break;
					}
					my->x = oldx;
					my->y = oldy;
				}
			}
		}
	}

	if ( !bombExplodeAOETargets && triggered 
		&& (BOMB_PLACEMENT == Item::ItemBombPlacement::BOMB_DOOR 
			|| BOMB_PLACEMENT == Item::ItemBombPlacement::BOMB_CHEST
			|| BOMB_PLACEMENT == Item::ItemBombPlacement::BOMB_COLLIDER) )
	{
		// found enemy, do AoE effect.
		BOMB_HIT_BY_PROJECTILE = 1;
	}
	else if ( bombExplodeAOETargets || triggered )
	{
		//Entity* entity = newEntity(-1, 1, map.entities, nullptr); //Item entity.
		//entity->flags[INVISIBLE] = true;
		//entity->flags[UPDATENEEDED] = true;
		//entity->flags[PASSABLE] = true;
		//entity->x = my->x;
		//entity->y = my->y;
		//entity->z = my->z;
		//entity->sizex = my->sizex;
		//entity->sizey = my->sizey;
		//entity->yaw = my->yaw;
		//entity->pitch = my->pitch;
		//entity->roll = 3 * PI / 2;
		//entity->behavior = &actItem;
		//entity->skill[10] = TOOL_DETONATOR_CHARGE;
		//entity->skill[11] = BROKEN;
		//entity->skill[12] = 0;
		//entity->skill[13] = 1;
		//entity->skill[14] = ITEM_TINKERING_APPEARANCE;
		//entity->skill[15] = 1;
		//if ( BOMB_PLACEMENT == Item::ItemBombPlacement::BOMB_FLOOR )
		//{
		//	// don't fall down
		//	entity->itemNotMoving = 1;
		//	entity->itemNotMovingClient = 1;
		//	serverUpdateEntitySkill(entity, 18); //update both the above flags.
		//	serverUpdateEntitySkill(entity, 19);
		//}
		//else
		//{
		//	entity->itemNotMoving = 0;
		//	entity->itemNotMovingClient = 0;
		//}
		if ( BEARTRAP_BEATITUDE >= 0 )
		{
			Item* charge = newItem(TOOL_DETONATOR_CHARGE, BROKEN, 0, 1, ITEM_TINKERING_APPEARANCE, true, nullptr);
			Entity* dropped = dropItemMonster(charge, my, nullptr);
			if ( dropped )
			{
				dropped->flags[USERFLAG1] = true;
			}
		}
		my->removeLightField();
		if ( triggered )
		{
			bombDoEffect(my, triggered, entityDistance, false, false);
		}
		playSoundEntity(my, 76, 64);
		list_RemoveNode(my->mynode);
		return;
	}
}

bool Entity::entityCheckIfTriggeredBomb(bool triggerBomb)
{
	if ( multiplayer == CLIENT )
	{
		return false;
	}
	if ( this->behavior != &actThrown && this->behavior != &actArrow )
	{
		return false;
	}
	bool foundBomb = false;
	std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(this, 2);
	for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
	{
		list_t* currentList = *it;
		node_t* node;
		for ( node = currentList->first; node != nullptr; node = node->next )
		{
			Entity* entity = (Entity*)node->element;
			if ( entity && entity->behavior == &actBomb && entity->skill[24] == 0 )
			{
				if ( entityInsideEntity(this, entity) )
				{
					if ( triggerBomb )
					{
						entity->skill[24] = 1;
					}
					foundBomb = true;
				}
			}
		}
	}
	return foundBomb;
}

void actDecoyBox(Entity* my)
{
	my->flags[PASSABLE] = true;
	if ( my->skill[0] == 0 )
	{
		my->skill[0] = 1;
		//spawn a crank.
		Entity* entity = newEntity(895, 1, map.entities, nullptr); //Decoy crank.
		entity->behavior = &actDecoyBoxCrank;
		entity->parent = my->getUID();
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		entity->yaw = my->yaw;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[UNCLICKABLE] = true;
		if ( multiplayer != CLIENT )
		{
			entity_uids--;
		}
		entity->setUID(-3);
		playSoundEntityLocal(my, 455, 64);
	}
	else
	{
		// let's make some noise.
		if ( my->ticks % 5 == 0 && local_rng.rand() % 3 == 0 )
		{
			playSoundEntityLocal(my, 472 + local_rng.rand() % 13, 192);
		}
		if ( my->ticks % 20 == 0 && local_rng.rand() % 3 > 0 )
		{
			playSoundEntityLocal(my, 475 + local_rng.rand() % 10, 192);
		}
	}
	if ( multiplayer == CLIENT )
	{
		return;
	}

	if ( my->ticks % TICKS_PER_SECOND == 0 )
	{
		Entity* parent = uidToEntity(my->parent);
		std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, decoyBoxRange * 2 + 1);
		std::vector<Entity*> listOfOtherDecoys;
		// find other decoys (so monsters don't wiggle back and forth.)
		for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
		{
			list_t* currentList = *it;
			node_t* node;
			for ( node = currentList->first; node != nullptr; node = node->next )
			{
				Entity* entity = (Entity*)node->element;
				if ( entity && entity->behavior == &actDecoyBox && entity != my )
				{
					listOfOtherDecoys.push_back(entity);
				}
			}
		}
		entLists.clear();

		entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, decoyBoxRange);
		bool message = false;
		bool detected = false;
		int lured = 0;
		for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
		{
			list_t* currentList = *it;
			node_t* node;
			for ( node = currentList->first; node != nullptr; node = node->next )
			{
				Entity* entity = (Entity*)node->element;
				if ( parent && entity && entity->behavior == &actMonster
					&& parent->checkEnemy(entity) && entity->isMobile() )
				{
					if ( (entity->monsterState == MONSTER_STATE_WAIT || entity->monsterTarget == 0) 
						|| (entityDist(entity,my) < 2 * TOUCHRANGE && (Uint32)(entity->monsterLastDistractedByNoisemaker) != my->getUID()) )
					{
						Stat* myStats = entity->getStats();
						if ( !entity->isBossMonster() && !entity->monsterIsTinkeringCreation()
							&& myStats && !uidToEntity(myStats->leader_uid) )
						{
							// found an eligible monster (far enough away, non-boss)
							// now look through other decoys, if others are in range of our target,
							// then the most recent decoy will pull them (this decoy won't do anything)
							bool foundMoreRecentDecoy = false;
							if ( !listOfOtherDecoys.empty() )
							{
								for ( std::vector<Entity*>::iterator decoyIt = listOfOtherDecoys.begin(); decoyIt != listOfOtherDecoys.end(); ++decoyIt )
								{
									Entity* decoy = *decoyIt;
									if ( entityDist(decoy, entity) < (decoyBoxRange * 16) ) // less than x tiles from our monster
									{
										if ( decoy->ticks < my->ticks )
										{
											// this decoy is newer (less game ticks alive)
											// defer to this decoy.
											foundMoreRecentDecoy = true;
											break;
										}
									}
								}
							}
							if ( (Uint32)(entity->monsterLastDistractedByNoisemaker) == my->getUID() )
							{
								// ignore pathing to this noisemaker as we're already distracted by it.
								if ( entityDist(entity, my) < TOUCHRANGE 
									&& !myStats->EFFECTS[EFF_DISORIENTED]
									&& !myStats->EFFECTS[EFF_DISTRACTED_COOLDOWN] )
								{
									// if we pathed within range
									detected = false; // skip the message.

									// can I see the noisemaker next to me?
									real_t tangent = atan2(entity->y - my->y, entity->x - my->x);
									lineTraceTarget(my, my->x, my->y, tangent, 32.0, 0, false, entity);
									if ( hit.entity == entity )
									{
										// set disoriented and start a cooldown on being distracted.
										if ( entity->monsterState == MONSTER_STATE_WAIT || entity->monsterTarget == 0 )
										{
											// not attacking, duration longer.
											entity->setEffect(EFF_DISORIENTED, true, TICKS_PER_SECOND * 3, false);
											entity->setEffect(EFF_DISTRACTED_COOLDOWN, true, TICKS_PER_SECOND * 5, false);
										}
										else
										{
											entity->setEffect(EFF_DISORIENTED, true, TICKS_PER_SECOND * 1, false);
											entity->setEffect(EFF_DISTRACTED_COOLDOWN, true, TICKS_PER_SECOND * 5, false);
										}
										spawnFloatingSpriteMisc(134, entity->x + (-4 + local_rng.rand() % 9) + cos(entity->yaw) * 2,
											entity->y + (-4 + local_rng.rand() % 9) + sin(entity->yaw) * 2, entity->z + local_rng.rand() % 4);
										++lured;
									}
								}
								break;
							}
							if ( foundMoreRecentDecoy )
							{
								break;
							}
							if ( !myStats->EFFECTS[EFF_DISTRACTED_COOLDOWN] 
								&& entity->monsterSetPathToLocation(my->x / 16, my->y / 16, 2,
									GeneratePathTypes::GENERATE_PATH_DEFAULT) && entity->children.first )
							{
								// path only if we're not on cooldown
								entity->monsterLastDistractedByNoisemaker = my->getUID();
								entity->monsterTarget = my->getUID();
								entity->monsterState = MONSTER_STATE_HUNT; // hunt state
								serverUpdateEntitySkill(entity, 0);
								detected = true;
								++lured;

								if ( entityDist(entity, my) < TOUCHRANGE 
									&& !myStats->EFFECTS[EFF_DISORIENTED]
									&& !myStats->EFFECTS[EFF_DISTRACTED_COOLDOWN] )
								{
									detected = false; // skip the message.

									// can I see the noisemaker next to me?
									real_t tangent = atan2(entity->y - my->y, entity->x - my->x);
									lineTraceTarget(my, my->x, my->y, tangent, 32.0, 0, false, entity);
									if ( hit.entity == entity )
									{
										// set disoriented and start a cooldown on being distracted.
										if ( entity->monsterState == MONSTER_STATE_WAIT || entity->monsterTarget == 0 )
										{
											// not attacking, duration longer.
											entity->setEffect(EFF_DISORIENTED, true, TICKS_PER_SECOND * 3, false);
											entity->setEffect(EFF_DISTRACTED_COOLDOWN, true, TICKS_PER_SECOND * 5, false);
										}
										else
										{
											entity->setEffect(EFF_DISORIENTED, true, TICKS_PER_SECOND * 1, false);
											entity->setEffect(EFF_DISTRACTED_COOLDOWN, true, TICKS_PER_SECOND * 5, false);
										}
										spawnFloatingSpriteMisc(134, entity->x + (-4 + local_rng.rand() % 9) + cos(entity->yaw) * 2,
											entity->y + (-4 + local_rng.rand() % 9) + sin(entity->yaw) * 2, entity->z + local_rng.rand() % 4);
									}
								}

								if ( parent->behavior == &actPlayer && stats[parent->skill[2]] )
								{
									// see if we have a gyrobot follower to tell us what's goin on
									for ( node_t* tmpNode = stats[parent->skill[2]]->FOLLOWERS.first; tmpNode != nullptr; tmpNode = tmpNode->next )
									{
										Uint32* c = (Uint32*)tmpNode->element;
										Entity* gyrobot = uidToEntity(*c);
										if ( gyrobot && gyrobot->getRace() == GYROBOT )
										{
											if ( entity->entityShowOnMap < 250 )
											{
												entity->entityShowOnMap = TICKS_PER_SECOND * 5;
												if ( parent->skill[2] != 0 )
												{
													serverUpdateEntitySkill(entity, 59);
												}
											}
											if ( !message )
											{
												messagePlayer(parent->skill[2], MESSAGE_WORLD, Language::get(3671));
												message = true;
											}
											break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		if ( detected && lured > 0 )
		{
			if ( parent && parent->behavior == &actPlayer )
			{
				Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_NOISEMAKER_LURED, TOOL_DECOY, lured);
				Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_NOISEMAKER_MOST_LURED, TOOL_DECOY, lured);
			}
		}
		if ( !message && detected )
		{
			if ( parent && parent->behavior == &actPlayer )
			{
				messagePlayer(parent->skill[2], MESSAGE_HINT, Language::get(3882));
			}
		}
	}

	if ( my->ticks > TICKS_PER_SECOND * 7 )
	{
		// stop working.
		bool decoyBreak = (local_rng.rand() % 5 == 0);
		Entity* parent = uidToEntity(my->parent);
		playSoundEntity(my, 485 + local_rng.rand() % 3, 192);
		if ( !decoyBreak )
		{
			playSoundEntity(my, 176, 128);
			Item* item = newItem(TOOL_DECOY, static_cast<Status>(BEARTRAP_STATUS), BEARTRAP_BEATITUDE, 1, BEARTRAP_APPEARANCE, true, nullptr);
			Entity* entity = dropItemMonster(item, my, nullptr);
			if ( entity )
			{
				entity->flags[USERFLAG1] = true;    // makes items passable, improves performance
			}
			/*if ( parent && parent->behavior == &actPlayer )
			{
				messagePlayer(parent->skill[2], Language::get(3769));
			}*/
			list_RemoveNode(my->mynode);
			return;
		}
		else
		{
			playSoundEntity(my, 132, 16);
			for ( int c = 0; c < 3; c++ )
			{
				Entity* entity = spawnGib(my);
				if ( entity )
				{
					switch ( c )
					{
						case 0:
							entity->sprite = 895;
							break;
						case 1:
							entity->sprite = 894;
							break;
						case 2:
							entity->sprite = 874;
							break;
						default:
							break;
					}
					serverSpawnGibForClient(entity);
				}
			}
			if ( parent && parent->behavior == &actPlayer )
			{
				messagePlayer(parent->skill[2], MESSAGE_EQUIPMENT, Language::get(3770));
			}
			list_RemoveNode(my->mynode);
			return;
		}
	}
}

void actDecoyBoxCrank(Entity* my)
{
	Entity* parent = uidToEntity(my->parent);
	if ( !parent )
	{
		list_RemoveNode(my->mynode);
		return;
	}
	my->x = parent->x;
	my->y = parent->y;
	my->z = parent->z;
	my->yaw = parent->yaw;

	my->x += limbs[DUMMYBOT][12][0] * cos(parent->yaw) + limbs[DUMMYBOT][12][1] * cos(parent->yaw + PI / 2);
	my->y += limbs[DUMMYBOT][12][0] * sin(parent->yaw) + limbs[DUMMYBOT][12][1] * sin(parent->yaw + PI / 2);
	my->z = limbs[DUMMYBOT][12][2];
	my->focalx = limbs[DUMMYBOT][11][0];
	my->focaly = limbs[DUMMYBOT][11][1];
	my->focalz = limbs[DUMMYBOT][11][2];

	my->pitch += 0.1;
	if ( my->pitch > 2 * PI )
	{
		my->pitch -= 2 * PI;
	}
}
