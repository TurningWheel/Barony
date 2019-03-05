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
			if ( capstoneUnlocked || stats[player]->PROFICIENCIES[PRO_LOCKPICKING] > rand() % 200 )
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
			if ( capstoneUnlocked || stats[player]->PROFICIENCIES[PRO_LOCKPICKING] > rand() % 200 )
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

						players[player]->entity->increaseSkill(PRO_LOCKPICKING);
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
						entity.monsterAcquireAttackTarget(*players[player]->entity, MONSTER_STATE_PATH, true);

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
			consumeItem(item, player);
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
			consumeItem(item, player);
			stats[player]->weapon = nullptr;
		}
	}
	else
	{
		messagePlayer(player, language[2371]);
	}
}

void Item::applyEmptyPotion(int player, Entity& entity)
{
	if ( entity.behavior == &actFountain || entity.behavior == &actSink )
	{
		if ( entity.skill[0] <= 0 )
		{
			// fountain is dry, no bueno.
			if ( player == clientnum )
			{
				if ( entity.behavior == &actFountain )
				{
					messagePlayer(player, language[467]);
				}
				else
				{
					messagePlayer(player, language[580]);
				}
			}
			return;
		}
		if ( multiplayer == CLIENT )
		{
			Item* item = stats[player]->weapon;
			consumeItem(item, player);
			return;
		}

		Item* item = stats[player]->weapon;
		consumeItem(item, player);

		int skillLVL = 2; // 0 to 5
		if ( stats[player] )
		{
			int skillLVL = stats[player]->PROFICIENCIES[PRO_ALCHEMY] / 20;
		}

		std::vector<int> potionChances =
		{
			20,	//POTION_WATER,
			20,	//POTION_BOOZE,
			20,	//POTION_JUICE,
			20,	//POTION_SICKNESS,
			10,	//POTION_CONFUSION,
			0,	//POTION_EXTRAHEALING,
			4,	//POTION_HEALING,
			4,	//POTION_CUREAILMENT,
			10,	//POTION_BLINDNESS,
			4,	//POTION_RESTOREMAGIC,
			1,	//POTION_INVISIBILITY,
			1,	//POTION_LEVITATION,
			4,	//POTION_SPEED,
			10,	//POTION_ACID,
			1,	//POTION_PARALYSIS,
			1,	//POTION_POLYMORPH
		};
		if ( skillLVL == 2 ) // 40 skill
		{
			potionChances =
			{
				4,	//POTION_WATER,
				5,	//POTION_BOOZE,
				5,	//POTION_JUICE,
				5,	//POTION_SICKNESS,
				5,	//POTION_CONFUSION,
				1,	//POTION_EXTRAHEALING,
				2,	//POTION_HEALING,
				2,	//POTION_CUREAILMENT,
				5,	//POTION_BLINDNESS,
				2,	//POTION_RESTOREMAGIC,
				1,	//POTION_INVISIBILITY,
				1,	//POTION_LEVITATION,
				5,	//POTION_SPEED,
				5,	//POTION_ACID,
				1,	//POTION_PARALYSIS,
				1,	//POTION_POLYMORPH
			};
		}
		else if ( skillLVL == 3 ) // 60 skill
		{
			potionChances =
			{
				2,	//POTION_WATER,
				4,	//POTION_BOOZE,
				4,	//POTION_JUICE,
				4,	//POTION_SICKNESS,
				4,	//POTION_CONFUSION,
				1,	//POTION_EXTRAHEALING,
				2,	//POTION_HEALING,
				2,	//POTION_CUREAILMENT,
				4,	//POTION_BLINDNESS,
				2,	//POTION_RESTOREMAGIC,
				1,	//POTION_INVISIBILITY,
				1,	//POTION_LEVITATION,
				4,	//POTION_SPEED,
				4,	//POTION_ACID,
				1,	//POTION_PARALYSIS,
				1,	//POTION_POLYMORPH
			};
		}
		else if ( skillLVL == 4 ) // 80 skill
		{
			potionChances =
			{
				0,	//POTION_WATER,
				2,	//POTION_BOOZE,
				2,	//POTION_JUICE,
				2,	//POTION_SICKNESS,
				3,	//POTION_CONFUSION,
				1,	//POTION_EXTRAHEALING,
				2,	//POTION_HEALING,
				2,	//POTION_CUREAILMENT,
				3,	//POTION_BLINDNESS,
				2,	//POTION_RESTOREMAGIC,
				1,	//POTION_INVISIBILITY,
				1,	//POTION_LEVITATION,
				3,	//POTION_SPEED,
				3,	//POTION_ACID,
				1,	//POTION_PARALYSIS,
				1,	//POTION_POLYMORPH
			};
		}
		else if ( skillLVL == 5 ) // 100 skill
		{
			potionChances =
			{
				0,	//POTION_WATER,
				1,	//POTION_BOOZE,
				1,	//POTION_JUICE,
				1,	//POTION_SICKNESS,
				1,	//POTION_CONFUSION,
				1,	//POTION_EXTRAHEALING,
				2,	//POTION_HEALING,
				2,	//POTION_CUREAILMENT,
				2,	//POTION_BLINDNESS,
				2,	//POTION_RESTOREMAGIC,
				1,	//POTION_INVISIBILITY,
				1,	//POTION_LEVITATION,
				2,	//POTION_SPEED,
				2,	//POTION_ACID,
				1,	//POTION_PARALYSIS,
				1,	//POTION_POLYMORPH
			};
		}


		if ( entity.behavior == &actFountain )
		{
			std::discrete_distribution<> potionDistribution(potionChances.begin(), potionChances.end());
			auto generatedPotion = potionStandardAppearanceMap.at(potionDistribution(fountainSeed));
			item = newItem(static_cast<ItemType>(generatedPotion.first), SERVICABLE, 0, 1, generatedPotion.second, false, NULL);
		}
		else
		{
			if ( entity.skill[3] == 1 ) // slime
			{
				item = newItem(POTION_ACID, SERVICABLE, 0, 1, 0, false, NULL);
			}
			else
			{
				item = newItem(POTION_WATER, SERVICABLE, 0, 1, 0, false, NULL);
			}
		}
		if ( item )
		{
			itemPickup(player, item);
			messagePlayer(player, language[3353], item->description());
			if ( players[player] && players[player]->entity )
			{
				playSoundEntity(players[player]->entity, 401, 64);
			}
			free(item);
			steamStatisticUpdateClient(player, STEAM_STAT_FREE_REFILLS, STEAM_STAT_INT, 1);
		}

		if ( entity.behavior == &actSink )
		{
			if ( entity.skill[3] == 1 || entity.skill[3] == 0 ) // ring or a slime
			{
				if ( player > 0 )
				{
					client_selected[player] = &entity;
					actSink(&entity);
				}
				else if ( player == 0 )
				{
					selectedEntity = &entity;
					actSink(&entity);
				}
			}
			else if ( entity.skill[0] > 1 )
			{
				--entity.skill[0];
				// Randomly choose second usage stats.
				int effect = rand() % 10; //4 possible effects.
				switch ( effect )
				{
					case 0:
						//10% chance.
						entity.skill[3] = 0; //Player will find a ring.
					case 1:
						//10% chance.
						entity.skill[3] = 1; //Will spawn a slime.
						break;
					case 2:
					case 3:
					case 4:
					case 5:
					case 6:
					case 7:
						//60% chance.
						entity.skill[3] = 2; //Will raise nutrition.
						break;
					case 8:
					case 9:
						//20% chance.
						entity.skill[3] = 3; //Player will lose 1 HP.
						break;
					default:
						break; //Should never happen.
				}
			}
			else
			{
				--entity.skill[0];
				entity.skill[0] = std::max(entity.skill[0], 0);
				serverUpdateEntitySkill(&entity, 0);
			}
		}
		else if ( entity.skill[1] == 2 || entity.skill[1] == 1 ) // fountain would spawn potions
		{
			//messagePlayer(player, language[474]);
			entity.skill[0] = 0; //Dry up fountain.
			serverUpdateEntitySkill(&entity, 0);
		}
		else if ( skillLVL < 2 || (skillLVL >= 2 && rand() % (skillLVL) == 0 ) )
		{
			if ( player > 0 )
			{
				client_selected[player] = &entity;
				actFountain(&entity);
			}
			else if ( player == 0 )
			{
				selectedEntity = &entity;
				actFountain(&entity);
			}
		}
	}
	else
	{
		messagePlayer(player, language[2371]);
	}
}
