/*-------------------------------------------------------------------------------

	BARONY
	File: item_tool.cpp
	Desc: implementation functions for the tool category items

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "sound.hpp"
#include "net.hpp"
#include "player.hpp"
#include "stat.hpp"
#include "colors.hpp"
#include "items.hpp"
#include "magic/magic.hpp"
#include "scores.hpp"

void Item::applySkeletonKey(int player, Entity& entity)
{
	if ( entity.behavior == &actChest )
	{
		playSoundEntity(&entity, 91, 64);
		if ( entity.skill[4] )
		{
			messagePlayer(player, language[1097]);
			entity.unlockChest();
		}
		else
		{
			messagePlayer(player, language[1098]);
			entity.lockChest();
		}
	}
	else if ( entity.behavior == &actDoor )
	{
		playSoundEntity(&entity, 91, 64);
		if ( entity.skill[5] )
		{
			messagePlayer(player, language[1099]);
			entity.skill[5] = 0;
		}
		else
		{
			messagePlayer(player, language[1100]);
			entity.skill[5] = 1;
		}
	}
	else
	{
		messagePlayer(player, language[1101], getName());
	}
}


void Item::applyLockpick(int player, Entity& entity)
{
	bool capstoneUnlocked = (stats[player]->PROFICIENCIES[PRO_LOCKPICKING] >= CAPSTONE_LOCKPICKING_UNLOCK);
	if ( entity.behavior == &actChest )
	{
		if ( entity.chestLocked )
		{
			if ( capstoneUnlocked || stats[player]->PROFICIENCIES[PRO_LOCKPICKING] > rand() % 400 )
			{
				//Unlock chest.
				playSoundEntity(&entity, 91, 64);
				messagePlayer(player, language[1097]);
				if ( capstoneUnlocked && !entity.chestPreventLockpickCapstoneExploit )
				{
					int goldAmount = CAPSTONE_LOCKPICKING_CHEST_GOLD_AMOUNT;
					stats[player]->GOLD += goldAmount;
					messagePlayerColor(player, uint32ColorGreen(*mainsurface), "You found %d gold pieces in the chest!", goldAmount);
				}
				entity.unlockChest();
				players[player]->entity->increaseSkill(PRO_LOCKPICKING);
			}
			else
			{
				//Failed to unlock chest.
				playSoundEntity(&entity, 92, 64);
				messagePlayer(player, language[1102]);
				if ( rand() % 10 == 0 )
				{
					players[player]->entity->increaseSkill(PRO_LOCKPICKING);
				}
				else
				{
					if ( rand() % 5 == 0 )
					{
						if ( player == clientnum )
						{
							if ( count > 1 )
							{
								newItem(type, status, beatitude, count - 1, appearance, identified, &stats[player]->inventory);
							}
						}
						stats[player]->weapon->count = 1;
						stats[player]->weapon->status = static_cast<Status>(stats[player]->weapon->status - 1);
						if ( status != BROKEN )
						{
							messagePlayer(player, language[1103]);
						}
						else
						{
							messagePlayer(player, language[1104]);
						}
						if ( player > 0 && multiplayer == SERVER )
						{
							strcpy((char*) (net_packet->data), "ARMR");
							net_packet->data[4] = 5;
							net_packet->data[5] = stats[player]->weapon->status;
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}
					}
				}
			}
		}
		else
		{
			messagePlayer(player, language[1105]);
		}
	}
	else if ( entity.behavior == &actDoor )
	{
		if ( entity.skill[5] )
		{
			if ( capstoneUnlocked || stats[player]->PROFICIENCIES[PRO_LOCKPICKING] > rand() % 400 )
			{
				//Unlock door.
				playSoundEntity(&entity, 91, 64);
				messagePlayer(player, language[1099]);
				entity.skill[5] = 0;
				players[player]->entity->increaseSkill(PRO_LOCKPICKING);
			}
			else
			{
				//Failed to unlock door.
				playSoundEntity(&entity, 92, 64);
				messagePlayer(player, language[1106]);
				if ( rand() % 10 == 0 )
				{
					players[player]->entity->increaseSkill(PRO_LOCKPICKING);
				}
				else
				{
					if ( rand() % 5 == 0 )
					{
						if ( player == clientnum )
						{
							if ( count > 1 )
							{
								newItem(type, status, beatitude, count - 1, appearance, identified, &stats[player]->inventory);
							}
						}
						stats[player]->weapon->count = 1;
						stats[player]->weapon->status = static_cast<Status>(stats[player]->weapon->status - 1);
						if ( status != BROKEN )
						{
							messagePlayer(player, language[1103]);
						}
						else
						{
							messagePlayer(player, language[1104]);
						}
						if ( player > 0 && multiplayer == SERVER )
						{
							strcpy((char*) (net_packet->data), "ARMR");
							net_packet->data[4] = 5;
							net_packet->data[5] = stats[player]->weapon->status;
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}
					}
				}
			}
		}
		else
		{
			messagePlayer(player, language[1107]);
		}
	}
	else if ( entity.behavior == &actMonster )
	{
		Stat* myStats = entity.getStats();
		if ( myStats && myStats->type == AUTOMATON 
			&& entity.monsterSpecialState == 0
			&& !myStats->EFFECTS[EFF_CONFUSED] )
		{
			if ( players[player] && players[player]->entity )
			{
				// calculate facing direction from player, < PI is facing away from player
				real_t yawDiff = entity.yawDifferenceFromPlayer(player);
				if ( yawDiff < PI )
				{
					messagePlayer(player, language[2524], getName(), entity.getMonsterLangEntry());
					int chance = stats[player]->PROFICIENCIES[PRO_LOCKPICKING] / 20 + 1;
					if ( rand() % chance > 1 )
					{
						entity.monsterSpecialState = AUTOMATON_MALFUNCTION_START;
						entity.monsterSpecialTimer = MONSTER_SPECIAL_COOLDOWN_AUTOMATON_MALFUNCTION;
						serverUpdateEntitySkill(&entity, 33);

						myStats->EFFECTS[EFF_PARALYZED] = true;
						myStats->EFFECTS_TIMERS[EFF_PARALYZED] = -1;
						playSoundEntity(&entity, 76, 128);
						messagePlayer(player, language[2527], entity.getMonsterLangEntry());

						if ( rand() % 3 == 0 )
						{
							players[player]->entity->increaseSkill(PRO_LOCKPICKING);
						}
						serverUpdatePlayerGameplayStats(player, STATISTICS_BOMB_SQUAD, 1);
						players[player]->entity->awardXP(&entity, true, true);
					}
					else
					{
						messagePlayer(player, language[2526], entity.getMonsterLangEntry());
						myStats->EFFECTS[EFF_CONFUSED] = true;
						myStats->EFFECTS_TIMERS[EFF_CONFUSED] = -1;
						myStats->EFFECTS[EFF_PARALYZED] = true;
						myStats->EFFECTS_TIMERS[EFF_PARALYZED] = 25;
						playSoundEntity(&entity, 263, 128);
						spawnMagicEffectParticles(entity.x, entity.y, entity.z, 170);
						entity.monsterAcquireAttackTarget(*players[player]->entity, MONSTER_STATE_PATH);

						if ( rand() % 5 == 0 )
						{
							players[player]->entity->increaseSkill(PRO_LOCKPICKING);
						}
					}
					if ( rand() % 2 == 0 )
					{
						if ( player == clientnum )
						{
							if ( count > 1 )
							{
								newItem(type, status, beatitude, count - 1, appearance, identified, &stats[player]->inventory);
							}
						}
						stats[player]->weapon->count = 1;
						stats[player]->weapon->status = static_cast<Status>(stats[player]->weapon->status - 1);
						if ( status != BROKEN )
						{
							messagePlayer(player, language[1103]);
						}
						else
						{
							messagePlayer(player, language[1104]);
						}
						if ( player > 0 && multiplayer == SERVER )
						{
							strcpy((char*)(net_packet->data), "ARMR");
							net_packet->data[4] = 5;
							net_packet->data[5] = stats[player]->weapon->status;
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}
					}
				}
				else
				{
					messagePlayer(player, language[2525], entity.getMonsterLangEntry());
				}
			}
		}
		else
		{
			messagePlayer(player, language[2528], getName());
		}
	}
	else
	{
		messagePlayer(player, language[1101], getName());
	}
}

void Item::applyOrb(int player, ItemType type, Entity& entity)
{
	if ( entity.behavior == &actPedestalBase && entity.pedestalHasOrb == 0 )
	{
		if ( multiplayer == CLIENT )
		{
			Item* item = stats[player]->weapon;
			stats[player]->weapon = nullptr;
			consumeItem(item);
			return;
		}
		messagePlayer(player, language[2368]);
		bool playSound = true;

		if ( type == ARTIFACT_ORB_BLUE && entity.pedestalOrbType == 1 )
		{
			messagePlayer(player, language[2370]);
		}
		else if ( type == ARTIFACT_ORB_RED && entity.pedestalOrbType == 2 )
		{
			messagePlayer(player, language[2370]);
		}
		else if ( type == ARTIFACT_ORB_PURPLE && entity.pedestalOrbType == 3 )
		{
			messagePlayer(player, language[2370]);
		}
		else if ( type == ARTIFACT_ORB_GREEN && entity.pedestalOrbType == 4 )
		{
			messagePlayer(player, language[2370]);
		}
		else
		{
			// incorrect orb.
			messagePlayer(player, language[2369]);
			playSound = false;
		}

		if ( multiplayer != CLIENT )
		{
			if ( playSound )
			{
				playSoundEntity(&entity, 166, 128); // invisible.ogg
				createParticleDropRising(&entity, entity.pedestalOrbType + 605, 1.0);
				serverSpawnMiscParticles(&entity, PARTICLE_EFFECT_RISING_DROP, entity.pedestalOrbType + 605);
			}
			entity.pedestalHasOrb = type - ARTIFACT_ORB_BLUE + 1;
			serverUpdateEntitySkill(&entity, 0); // update orb status.
			Item* item = stats[player]->weapon;
			consumeItem(item);
			stats[player]->weapon = nullptr;
		}
	}
	else
	{
		messagePlayer(player, language[2371]);
	}
}
