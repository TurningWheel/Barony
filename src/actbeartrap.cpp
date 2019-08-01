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
#include "sound.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "magic/magic.hpp"

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

void actBeartrap(Entity* my)
{
	int i;
	if ( my->sprite == 667 )
	{	
		my->roll = 0;
		my->z = 6.75;
	}

	if ( multiplayer == CLIENT )
	{
		return;
	}

	// undo beartrap
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if ( (i == 0 && selectedEntity == my) || (client_selected[i] == my) )
		{
			if (inrange[i])
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
				messagePlayer(i, language[1300]);
				list_RemoveNode(my->mynode);
				return;
			}
		}
	}

	if ( BEARTRAP_CAUGHT == 1 )
	{
		return;
	}

	// launch beartrap
	node_t* node;
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
				Entity* parent = uidToEntity(my->parent);
				if ( (parent && parent->checkFriend(entity)) )
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
					Stat* trapperStat = nullptr;
					if ( parent && (trapperStat = parent->getStats()) )
					{
						damage += trapperStat->PROFICIENCIES[PRO_LOCKPICKING] / 5;
					}
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
					// set obituary
					entity->setObituary(language[1504]);

					if ( stat->HP <= 0 )
					{
						if ( parent )
						{
							parent->awardXP( entity, true, true );
						}
					}
					if ( entity->behavior == &actPlayer )
					{
						int player = entity->skill[2];
						Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
						messagePlayerColor(player, color, language[454]);
						if ( player > 0 )
						{
							serverUpdateEffects(player);
						}
						if ( player == clientnum )
						{
							camera_shakex += .1;
							camera_shakey += 10;
						}
						else if ( player > 0 && multiplayer == SERVER )
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
							if ( entityDist(my, parent) >= 64 && entityDist(my, parent) < 128 )
							{
								messagePlayer(player, language[2521]);
							}
							else
							{
								messagePlayer(player, language[2522]);
							}
							if ( rand() % 10 == 0 )
							{
								parent->increaseSkill(PRO_LOCKPICKING);
							}
							if ( rand() % 5 == 0 )
							{
								parent->increaseSkill(PRO_RANGED);
							}
							// update enemy bar for attacker
							if ( !strcmp(stat->name, "") )
							{
								if ( stat->type < KOBOLD ) //Original monster count
								{
									updateEnemyBar(parent, entity, language[90 + stat->type], stat->HP, stat->MAXHP);
								}
								else if ( stat->type >= KOBOLD ) //New monsters
								{
									updateEnemyBar(parent, entity, language[2000 + (stat->type - KOBOLD)], stat->HP, stat->MAXHP);
								}
							}
							else
							{
								updateEnemyBar(parent, entity, stat->name, stat->HP, stat->MAXHP);
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

void bombDoEffect(Entity* my, Entity* triggered, real_t entityDistance, bool spawnMagicOnTriggeredMonster, bool hitByAOE )
{
	if ( !triggered || !my )
	{
		return;
	}
	Entity* parent = uidToEntity(my->parent);
	Stat* stat = triggered->getStats();
	int damage = 0;
	//messagePlayer(0, "dmg: %d", damage);
	int doSpell = SPELL_NONE;
	bool doVertical = false;
	switch ( BOMB_ITEMTYPE )
	{
		case TOOL_BOMB:
			doSpell = SPELL_FIREBALL;
			damage = 5;
			break;
		case TOOL_SLEEP_BOMB:
			doSpell = SPELL_SLEEP;
			damage = 0;
			break;
		case TOOL_FREEZE_BOMB:
			doSpell = SPELL_COLD;
			damage = 5;
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

	// stumbled into the trap!
	Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
	if ( parent && parent->behavior == &actPlayer && triggered != parent )
	{
		if ( !hitByAOE )
		{
			messagePlayerMonsterEvent(parent->skill[2], color, *triggered->getStats(), language[3498], language[3499], MSG_TOOL_BOMB, my);
		}
		else
		{
			messagePlayerMonsterEvent(parent->skill[2], color, *triggered->getStats(), language[3613], language[3614], MSG_TOOL_BOMB, my);
		}
	}
	if ( triggered->behavior == &actPlayer )
	{
		int player = triggered->skill[2];
		Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
		// you stumbled into the trap!
		if ( !hitByAOE )
		{
			messagePlayerColor(player, color, language[3497], items[BOMB_ITEMTYPE].name_identified);
		}
		else
		{
			messagePlayerColor(player, color, language[3612], items[BOMB_ITEMTYPE].name_identified);
		}
	}

	if ( doSpell == SPELL_TELEPORTATION )
	{
		if ( triggered->isBossMonster() )
		{
			// no effect.
			if ( parent && parent->behavior == &actPlayer )
			{
				Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
				messagePlayerMonsterEvent(parent->skill[2], color, *triggered->getStats(), language[3603], language[3604], MSG_COMBAT);
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
				if ( entity->skill[21] == TOOL_TELEPORT_BOMB && entity->skill[22] == 1 )
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
			Entity* targetLocation = parentgoodspots[rand() % parentgoodspots.size()];
			teleported = triggered->teleportAroundEntity(targetLocation, 2);
		}
		else if ( !goodspots.empty() )
		{
			Entity* targetLocation = goodspots[rand() % goodspots.size()];
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
					Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
					messagePlayerMonsterEvent(parent->skill[2], color, *triggered->getStats(), language[3601], language[3602], MSG_COMBAT);
				}
			}
			if ( triggered->behavior == &actPlayer )
			{
				Uint32 color = SDL_MapRGB(mainsurface->format, 255, 255, 255);
				messagePlayerColor(triggered->skill[2], color, language[3611], items[BOMB_ITEMTYPE].name_identified);
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
				Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
				messagePlayerMonsterEvent(parent->skill[2], color, *triggered->getStats(), language[3615], language[3616], MSG_COMBAT);
			}
		}
		return;
	}
	else if ( doSpell != SPELL_NONE )
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
	// set obituary
	int oldHP = stat->HP;
	triggered->modHP(-damage);
	triggered->setObituary(language[3496]);

	if ( stat->HP <= 0 && oldHP > 0 )
	{
		if ( parent )
		{
			parent->awardXP(triggered, true, true);
		}
	}

	if ( triggered->behavior == &actPlayer )
	{
		int player = triggered->skill[2];
		
		if ( player == clientnum )
		{
			camera_shakex += .1;
			camera_shakey += 10;
		}
		else if ( player > 0 && multiplayer == SERVER )
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
					messagePlayer(player, language[3494]);
				}
				else
				{
					messagePlayer(player, language[3495]);
				}
			}
			if ( triggered->behavior == &actMonster )
			{
				if ( oldHP > 0 && stat->HP == 0 ) // got a kill
				{
					if ( rand() % 2 == 0 )
					{
						parent->increaseSkill(PRO_LOCKPICKING);
					}
				}
				else if ( oldHP > stat->HP )
				{
					if ( rand() % 5 == 0 ) // wounded
					{
						parent->increaseSkill(PRO_LOCKPICKING);
					}
				}
				else if( rand() % 10 == 0) // any other effect
				{
					parent->increaseSkill(PRO_LOCKPICKING);
				}
			}
			// update enemy bar for attacker
			if ( damage > 0 )
			{
				if ( !strcmp(stat->name, "") )
				{
					if ( stat->type < KOBOLD ) //Original monster count
					{
						updateEnemyBar(parent, triggered, language[90 + stat->type], stat->HP, stat->MAXHP);
					}
					else if ( stat->type >= KOBOLD ) //New monsters
					{
						updateEnemyBar(parent, triggered, language[2000 + (stat->type - KOBOLD)], stat->HP, stat->MAXHP);
					}
				}
				else
				{
					updateEnemyBar(parent, triggered, stat->name, stat->HP, stat->MAXHP);
				}
				Entity* gib = spawnGib(triggered);
				serverSpawnGibForClient(gib);
			}
		}
	}
}

void actBomb(Entity* my)
{
	my->removeLightField();
	if ( multiplayer == CLIENT )
	{
		if ( BOMB_TRIGGER_TYPE == Item::ItemBombTriggerType::BOMB_TELEPORT_RECEIVER )
		{
			my->spawnAmbientParticles(25, 579, 10 + rand() % 40, 1.0, false);
			my->light = lightSphereShadow(my->x / 16, my->y / 16, 3, 92);
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
		if ( (i == 0 && selectedEntity == my) || (client_selected[i] == my) )
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
				messagePlayer(i, language[3600], items[BOMB_ITEMTYPE].name_identified);
				list_RemoveNode(my->mynode);
				return;
			}
		}
	}

	if ( BOMB_ITEMTYPE == TOOL_TELEPORT_BOMB && BOMB_TRIGGER_TYPE == Item::ItemBombTriggerType::BOMB_TELEPORT_RECEIVER )
	{
		my->spawnAmbientParticles(25, 579, 10 + rand() % 40, 1.0, false);
		my->light = lightSphereShadow(my->x / 16, my->y / 16, 3, 92);
		return;
	}

	// launch bomb
	std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 1);
	Entity* triggered = false;
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

	if ( BOMB_ENTITY_ATTACHED_TO != 0 || BOMB_HIT_BY_PROJECTILE == 1 || BOMB_PLACEMENT == Item::ItemBombPlacement::BOMB_WALL )
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
		}
		else if ( onEntity )
		{
			if ( onEntity->behavior == &actDoor )
			{
				if ( onEntity->doorHealth < BOMB_ENTITY_ATTACHED_START_HP || onEntity->flags[PASSABLE]
					|| BOMB_HIT_BY_PROJECTILE == 1 )
				{
					if ( onEntity->doorHealth > 0 )
					{
						onEntity->doorHandleDamageMagic(50, *my, uidToEntity(my->parent));
					}
					shouldExplode = true;
				}
			}
			else if ( onEntity->behavior == &actChest )
			{
				if ( onEntity->skill[3] < BOMB_ENTITY_ATTACHED_START_HP || BOMB_CHEST_STATUS != onEntity->skill[1]
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
		}
		else
		{
			shouldExplode = true; // my attached entity died.
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

	for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end() && !triggered; ++it )
	{
		list_t* currentList = *it;
		node_t* node;
		for ( node = currentList->first; node != nullptr && !triggered; node = node->next )
		{
			Entity* entity = (Entity*)node->element;
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
							spawnExplosionFromSprite(explosionSprite, my->x - 4 + rand() % 9, my->y + rand() % 9, my->z - 2);
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
									spawnExplosionFromSprite(explosionSprite, my->x - rand() % 9, my->y - 4 + rand() % 9, my->z);
								}
								break;
							case Item::ItemBombFacingDirection::BOMB_WEST:
								my->x -= 8;
								entityDistance = entityDist(my, entity);
								if ( entityDistance < 12 )
								{
									triggered = entity;
									spawnExplosionFromSprite(explosionSprite, my->x + rand() % 9, my->y - 4 + rand() % 9, my->z);
								}
								break;
							case Item::ItemBombFacingDirection::BOMB_SOUTH:
								my->y += 8;
								entityDistance = entityDist(my, entity);
								if ( entityDistance < 12 )
								{
									triggered = entity;
									spawnExplosionFromSprite(explosionSprite, my->x - 4 + rand() % 9, my->y - rand() % 9, my->z);
								}
								break;
							case Item::ItemBombFacingDirection::BOMB_NORTH:
								my->y -= 8;
								entityDistance = entityDist(my, entity);
								if ( entityDistance < 12 )
								{
									triggered = entity;
									spawnExplosionFromSprite(explosionSprite, my->x - 4 + rand() % 9, my->y + rand() % 9, my->z);
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
	}

	if ( !bombExplodeAOETargets && triggered 
		&& (BOMB_PLACEMENT == Item::ItemBombPlacement::BOMB_DOOR || BOMB_PLACEMENT == Item::ItemBombPlacement::BOMB_CHEST) )
	{
		// found enemy, do AoE effect.
		BOMB_HIT_BY_PROJECTILE = 1;
	}
	else if ( bombExplodeAOETargets )
	{
		my->removeLightField();
		playSoundEntity(my, 76, 64);
		list_RemoveNode(my->mynode);
		return;
	}
	else if ( triggered )
	{
		my->removeLightField();
		bombDoEffect(my, triggered, entityDistance, false, false);
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
	std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(this, 1);
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
	}
	if ( multiplayer == CLIENT )
	{
		return;
	}

	if ( my->ticks % TICKS_PER_SECOND == 0 )
	{
		Entity* parent = uidToEntity(my->parent);
		std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 10);
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

		entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 5);
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
					if ( entity->monsterState == MONSTER_STATE_WAIT || entity->monsterTarget == 0 )
					{
						Stat* myStats = entity->getStats();
						if ( !entity->isBossMonster() && !entity->monsterIsTinkeringCreation()
							&& myStats && !uidToEntity(myStats->leader_uid) && entityDist(my, entity) > TOUCHRANGE )
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
									if ( entityDist(decoy, entity) < (5 * 16) ) // less than 5 tiles from our monster
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
							if ( foundMoreRecentDecoy )
							{
								break;
							}
							if ( entity->monsterSetPathToLocation(my->x / 16, my->y / 16, 2) && entity->children.first )
							{
								entity->monsterTarget = my->getUID();
								entity->monsterState = MONSTER_STATE_HUNT; // hunt state
								serverUpdateEntitySkill(entity, 0);
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
											}
											messagePlayer(parent->skill[2], language[3671]);
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
	}

	if ( my->ticks > TICKS_PER_SECOND * 5 )
	{
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
		list_RemoveNode(my->mynode);
		return;
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