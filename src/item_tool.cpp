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
#include "shops.hpp"

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
		if ( entity.doorLocked )
		{
			if ( entity.doorDisableLockpicks == 1 )
			{
				Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 255);
				messagePlayerColor(player, color, language[3101]); // disabled.
			}
			else
			{
				messagePlayer(player, language[1099]);
				entity.doorLocked = 0;
			}
		}
		else
		{
			messagePlayer(player, language[1100]);
			entity.doorLocked = 1;
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
	if ( entity.behavior == &actBomb )
	{
		Entity* gyrobotUsing = nullptr;
		if ( entity.isInteractWithMonster() )
		{
			Entity* monsterInteracting = uidToEntity(entity.interactedByMonster);
			if ( monsterInteracting && monsterInteracting->getMonsterTypeFromSprite() == GYROBOT )
			{
				gyrobotUsing = monsterInteracting;
			}
		}

		if ( entity.skill[21] == TOOL_TELEPORT_BOMB )
		{
			++entity.skill[22];
			if ( entity.skill[22] > BOMB_TRIGGER_ALL )
			{
				entity.skill[22] = BOMB_TRIGGER_ENEMIES;
			}
		}
		else
		{
			entity.skill[22] = (entity.skill[22] == BOMB_TRIGGER_ENEMIES) ? BOMB_TRIGGER_ALL : BOMB_TRIGGER_ENEMIES;
		}
		if ( entity.skill[22] == BOMB_TRIGGER_ENEMIES )
		{
			if ( gyrobotUsing )
			{
				messagePlayer(player, language[3865]);
			}
			else
			{
				messagePlayer(player, language[3605]);
			}
			messagePlayerColor(player, uint32ColorGreen(*mainsurface), language[3606]);
		}
		else if ( entity.skill[22] == BOMB_TRIGGER_ALL )
		{
			if ( gyrobotUsing )
			{
				messagePlayer(player, language[3866]);
			}
			else
			{
				messagePlayer(player, language[3607]);
			}
			messagePlayerColor(player, uint32ColorRed(*mainsurface), language[3608]);
		}
		else if ( entity.skill[22] == BOMB_TELEPORT_RECEIVER )
		{
			if ( gyrobotUsing )
			{
				messagePlayer(player, language[3867]);
			}
			else
			{
				messagePlayer(player, language[3609]);
			}
			messagePlayer(player, language[3610]);

			playSoundEntity(&entity, 166, 128); // invisible.ogg
			createParticleDropRising(&entity, 576, 1.0);
			serverSpawnMiscParticles(&entity, PARTICLE_EFFECT_RISING_DROP, 576);
		}
		serverUpdateEntitySkill(&entity, 22);
		playSoundEntity(&entity, 253, 64);
	}
	else if ( entity.behavior == &actChest )
	{
		if ( entity.chestLocked )
		{
			// 3-17 damage on lockpick depending on skill
			// 0 skill is 3 damage
			// 20 skill is 4-5 damage
			// 60 skill is 6-11 damage
			// 100 skill is 8-17 damage
			int lockpickDamageToChest = 3 + stats[player]->PROFICIENCIES[PRO_LOCKPICKING] / 20
				+ rand() % std::max(1, stats[player]->PROFICIENCIES[PRO_LOCKPICKING] / 10);
			entity.chestLockpickHealth = std::max(0, entity.chestLockpickHealth - lockpickDamageToChest);
			bool unlockedFromLockpickHealth = (entity.chestLockpickHealth == 0);

			if ( capstoneUnlocked || stats[player]->PROFICIENCIES[PRO_LOCKPICKING] > rand() % 200
				|| unlockedFromLockpickHealth )
			{
				//Unlock chest.
				playSoundEntity(&entity, 91, 64);
				messagePlayer(player, language[1097]);
				if ( capstoneUnlocked && !entity.chestPreventLockpickCapstoneExploit )
				{
					if ( rand() % 2 == 0 )
					{
						Item* generated = newItem(itemTypeWithinGoldValue(-1, 80, 600), static_cast<Status>(SERVICABLE + rand() % 2), 0 + rand() % 2, 1, rand(), false, nullptr);
						entity.addItemToChest(generated);
						messagePlayer(player, language[3897]);
					}
					else
					{
						int goldAmount = CAPSTONE_LOCKPICKING_CHEST_GOLD_AMOUNT;
						stats[player]->GOLD += goldAmount;
						messagePlayerColor(player, uint32ColorGreen(*mainsurface), "You found %d gold pieces in the chest!", goldAmount);
					}
				}
				if ( !entity.chestPreventLockpickCapstoneExploit )
				{
					if ( stats[player]->PROFICIENCIES[PRO_LOCKPICKING] < SKILL_LEVEL_EXPERT )
					{
						players[player]->entity->increaseSkill(PRO_LOCKPICKING);
					}
					else
					{
						if ( rand() % 20 == 0 )
						{
							messagePlayer(player, language[3689], language[675]);
						}
					}

					// based on tinkering skill, add some bonus scrap materials inside chest. (50-150%)
					if ( (50 + 10 * (stats[player]->PROFICIENCIES[PRO_LOCKPICKING] / 10)) > rand() % 100 )
					{
						int metalscrap = 5 + rand() % 6;
						int magicscrap = 5 + rand() % 11;
						if ( entity.children.first )
						{
							list_t* inventory = static_cast<list_t* >(entity.children.first->element);
							if ( inventory )
							{
								newItem(TOOL_METAL_SCRAP, DECREPIT, 0, metalscrap, 0, true, inventory);
								newItem(TOOL_MAGIC_SCRAP, DECREPIT, 0, magicscrap, 0, true, inventory);
							}
						}
					}
				}
				entity.unlockChest();
			}
			else
			{
				//Failed to unlock chest.
				playSoundEntity(&entity, 92, 64);
				messagePlayer(player, language[1102]);
				bool tryDegradeLockpick = true;
				if ( !entity.chestPreventLockpickCapstoneExploit )
				{
					if ( stats[player]->PROFICIENCIES[PRO_LOCKPICKING] < SKILL_LEVEL_EXPERT )
					{
						if ( rand() % 10 == 0 )
						{
							players[player]->entity->increaseSkill(PRO_LOCKPICKING);
							tryDegradeLockpick = false;
						}
					}
					else
					{
						if ( rand() % 20 == 0 )
						{
							messagePlayer(player, language[3689], language[675]);
							tryDegradeLockpick = false;
						}
					}
				}
				
				if ( tryDegradeLockpick )
				{
					if ( rand() % 5 == 0 )
					{
						if ( player >= 0 && players[player]->isLocalPlayer() )
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
		if ( entity.doorLocked )
		{
			// 3-17 damage on lockpick depending on skill
			// 0 skill is 3 damage
			// 20 skill is 4-5 damage
			// 60 skill is 6-11 damage
			// 100 skill is 8-17 damage
			int lockpickDamageToDoor = 3 + stats[player]->PROFICIENCIES[PRO_LOCKPICKING] / 20
				+ rand() % std::max(1, stats[player]->PROFICIENCIES[PRO_LOCKPICKING] / 10);
			entity.doorLockpickHealth = std::max(0, entity.doorLockpickHealth - lockpickDamageToDoor);
			bool unlockedFromLockpickHealth = (entity.doorLockpickHealth == 0);

			if ( entity.doorDisableLockpicks == 1 )
			{
				Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 255);
				messagePlayerColor(player, color, language[3101]); // disabled.
			}
			else if ( capstoneUnlocked 
				|| stats[player]->PROFICIENCIES[PRO_LOCKPICKING] > rand() % 200
				|| unlockedFromLockpickHealth )
			{
				//Unlock door.
				playSoundEntity(&entity, 91, 64);
				messagePlayer(player, language[1099]);
				entity.doorLocked = 0;
				if ( !entity.doorPreventLockpickExploit )
				{
					if ( stats[player]->PROFICIENCIES[PRO_LOCKPICKING] < SKILL_LEVEL_SKILLED )
					{
						players[player]->entity->increaseSkill(PRO_LOCKPICKING);
					}
					else
					{
						if ( rand() % 20 == 0 )
						{
							messagePlayer(player, language[3689], language[674]);
						}
					}
				}
				entity.doorPreventLockpickExploit = 1;
			}
			else
			{
				//Failed to unlock door.
				playSoundEntity(&entity, 92, 64);
				messagePlayer(player, language[1106]);
				bool tryDegradeLockpick = true;
				if ( !entity.doorPreventLockpickExploit )
				{
					if ( stats[player]->PROFICIENCIES[PRO_LOCKPICKING] < SKILL_LEVEL_SKILLED )
					{
						if ( rand() % 10 == 0 )
						{
							players[player]->entity->increaseSkill(PRO_LOCKPICKING);
							tryDegradeLockpick = false;
						}
					}
					else
					{
						if ( rand() % 20 == 0 )
						{
							messagePlayer(player, language[3689], language[674]);
							tryDegradeLockpick = false;
						}
					}
				}
				
				if ( tryDegradeLockpick )
				{
					if ( rand() % 5 == 0 )
					{
						if ( player >= 0 && players[player]->isLocalPlayer() )
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
					if ( stats[player]->PROFICIENCIES[PRO_LOCKPICKING] >= 60 || (rand() % chance > 0) )
					{
						// 100% >= 60 lockpicking. 40 = 66%, 20 = 50%, 0 = 0%
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

						int qtyMetalScrap = 5 + rand() % 6;
						int qtyMagicScrap = 8 + rand() % 6;
						if ( stats[player] )
						{
							if ( stats[player]->PROFICIENCIES[PRO_LOCKPICKING] >= SKILL_LEVEL_MASTER )
							{
								qtyMetalScrap += 5 + rand() % 6; // 10-20 total
								qtyMagicScrap += 8 + rand() % 11; // 16-31 total
							}
							else if ( stats[player]->PROFICIENCIES[PRO_LOCKPICKING] >= SKILL_LEVEL_EXPERT )
							{
								qtyMetalScrap += 3 + rand() % 4; // 8-16 total
								qtyMagicScrap += 5 + rand() % 8; // 13-25 total
							}
							else if ( stats[player]->PROFICIENCIES[PRO_LOCKPICKING] >= SKILL_LEVEL_SKILLED )
							{
								qtyMetalScrap += 1 + rand() % 4; // 6-14 total
								qtyMagicScrap += 3 + rand() % 4; // 11-19 total
							}
						}
						Item* item = newItem(TOOL_METAL_SCRAP, DECREPIT, 0, qtyMetalScrap, 0, true, &myStats->inventory);
						item = newItem(TOOL_MAGIC_SCRAP, DECREPIT, 0, qtyMagicScrap, 0, true, &myStats->inventory);
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
						if ( player >= 0 && players[player]->isLocalPlayer() )
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
	else if ( entity.behavior == &actMonster || entity.behavior == &actPlayer )
	{
		if ( entity.getMonsterTypeFromSprite() == SHOPKEEPER && shopIsMysteriousShopkeeper(&entity) && this->type != ARTIFACT_ORB_PURPLE )
		{
			if ( multiplayer == CLIENT )
			{
				Item* item = stats[player]->weapon;
				stats[player]->weapon = nullptr;
				consumeItem(item, player);
				return;
			}

			switch ( this->type )
			{
				case ARTIFACT_ORB_BLUE:
					messagePlayer(player, language[3889], entity.getStats()->name);
					break;
				case ARTIFACT_ORB_RED:
					messagePlayer(player, language[3890], entity.getStats()->name);
					break;
				case ARTIFACT_ORB_GREEN:
					messagePlayer(player, language[3888], entity.getStats()->name);
					break;
				default:
					break;
			}

			playSoundEntity(&entity, 35 + rand() % 3, 64);

			Item* item = stats[player]->weapon;
			entity.addItemToMonsterInventory(newItem(item->type, item->status, item->beatitude, 1, item->appearance, item->identified, nullptr));
			consumeItem(item, player);
			stats[player]->weapon = nullptr;
		}
		else
		{
			if ( multiplayer != CLIENT )
			{
				messagePlayerMonsterEvent(player, uint32ColorWhite(*mainsurface), *entity.getStats(), language[3892], language[3891], MSG_COMBAT);
			}
			return;
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
			if ( player >= 0 && players[player]->isLocalPlayer() )
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
				if ( player > 0 && !splitscreen )
				{
					client_selected[player] = &entity;
					entity.skill[8] = 1; // disables polymorph being washed away.
					actSink(&entity);
					entity.skill[8] = 0;
				}
				else if ( player == 0 || (player > 0 && splitscreen) )
				{
					selectedEntity[player] = &entity;
					entity.skill[8] = 1; // disables polymorph being washed away.
					actSink(&entity);
					entity.skill[8] = 0;
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
		else if ( entity.skill[1] == 3 || entity.skill[1] == 4 )
		{
			// fountain would bless equipment.
			entity.skill[0] = 0; //Dry up fountain.
			serverUpdateEntitySkill(&entity, 0);
		}
		else if ( skillLVL < 2 || (skillLVL >= 2 && rand() % (skillLVL) == 0 ) )
		{
			if ( player > 0 && !splitscreen )
			{
				client_selected[player] = &entity;
				actFountain(&entity);
			}
			else if ( player == 0 || (player > 0 && splitscreen) )
			{
				selectedEntity[player] = &entity;
				actFountain(&entity);
			}
		}
	}
	else
	{
		messagePlayer(player, language[2371]);
	}
}

void Item::applyBomb(Entity* parent, ItemType type, ItemBombPlacement placement, ItemBombFacingDirection dir, Entity* thrown, Entity* onEntity)
{
	if ( multiplayer == CLIENT )
	{
		return;
	}

	int sprite = items[TOOL_BOMB].index + 1;
	if ( type >= TOOL_BOMB && type <= TOOL_TELEPORT_BOMB )
	{
		sprite = items[type].index + 1; // unpacked bomb model.
	}
	
	if ( placement == BOMB_FLOOR )
	{
		if ( thrown )
		{
			Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Beartrap entity.
			entity->behavior = &actBomb;
			entity->flags[PASSABLE] = true;
			entity->flags[UPDATENEEDED] = true;
			entity->x = thrown->x;
			entity->y = thrown->y;
			entity->z = 6.5;
			entity->yaw = thrown->yaw;
			entity->roll = -PI / 2; // flip the model
			if ( parent )
			{
				entity->parent = parent->getUID();
				if ( parent->behavior == &actPlayer )
				{
					entity->skill[17] = parent->skill[2];
				}
				else
				{
					entity->skill[17] = -1;
				}
			}
			entity->sizex = 4;
			entity->sizey = 4;
			entity->skill[11] = this->status;
			entity->skill[12] = this->beatitude;
			entity->skill[14] = this->appearance;
			entity->skill[15] = this->identified;
			entity->skill[16] = placement;
			entity->skill[20] = dir;
			entity->skill[21] = type;
		}
	}
	else if ( placement == BOMB_WALL )
	{
		if ( thrown )
		{
			Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Beartrap entity.
			entity->behavior = &actBomb;
			entity->flags[PASSABLE] = true;
			entity->flags[UPDATENEEDED] = true;
			entity->x = thrown->x;
			entity->y = thrown->y;
			entity->z = std::min(4.0, thrown->z);
			int height = 1;
			switch ( dir )
			{
				case BOMB_EAST:
					entity->yaw = 3 * PI / 2;
					entity->x = (static_cast<int>(std::floor(entity->x + 8)) >> 4) * 16;
					entity->x += height;
					break;
				case BOMB_SOUTH:
					entity->yaw = 0;
					entity->y = (static_cast<int>(std::floor(entity->y + 8)) >> 4) * 16;
					entity->y += height;
					break;
				case BOMB_WEST:
					entity->yaw = PI / 2;
					entity->x = (static_cast<int>(std::floor(entity->x + 8)) >> 4) * 16;
					entity->x -= height;
					break;
				case BOMB_NORTH:
					entity->yaw = PI;
					entity->y = (static_cast<int>(std::floor(entity->y + 8)) >> 4) * 16;
					entity->y -= height;
					break;
				default:
					break;
			}

			// check the wall because there is an edge case where the model clips a wall edge and there's technically no wall behind it
			// = boom for player.
			int checkx = entity->x;
			int checky = entity->y;
			switch ( dir )
			{
				case BOMB_EAST:
					checkx = static_cast<int>(entity->x - 8) >> 4;
					checky = static_cast<int>(entity->y) >> 4;
					break;
				case BOMB_WEST:
					checkx = static_cast<int>(entity->x + 8) >> 4;
					checky = static_cast<int>(entity->y) >> 4;
					break;
				case BOMB_SOUTH:
					checky = static_cast<int>(entity->y - 8) >> 4;
					checkx = static_cast<int>(entity->x) >> 4;
					break;
				case BOMB_NORTH:
					checky = static_cast<int>(entity->y + 8) >> 4;
					checkx = static_cast<int>(entity->x) >> 4;
					break;
				default:
					break;
			}
			if ( !map.tiles[OBSTACLELAYER + checky * MAPLAYERS + checkx * MAPLAYERS * map.height] )
			{
				// no wall.
				switch ( dir )
				{
					case BOMB_EAST:
					case BOMB_WEST:
						if ( map.tiles[OBSTACLELAYER + (static_cast<int>(entity->y + 4) >> 4) * MAPLAYERS + checkx * MAPLAYERS * map.height] )
						{
							// try 4 units away.
							entity->y += 4; // coordinates good.
						}
						else if ( map.tiles[OBSTACLELAYER + (static_cast<int>(entity->y - 4) >> 4) * MAPLAYERS + checkx * MAPLAYERS * map.height] )
						{
							// try 4 units away other direction.
							entity->y -= 4; // coordinates good.
						}
						break;
					case BOMB_NORTH:
					case BOMB_SOUTH:
						if ( map.tiles[OBSTACLELAYER + checky * MAPLAYERS + (static_cast<int>(entity->x + 4) >> 4)  * MAPLAYERS * map.height] )
						{
							// try 4 units away.
							entity->x += 4; // coordinates good.
						}
						else if ( map.tiles[OBSTACLELAYER + checky * MAPLAYERS + (static_cast<int>(entity->x - 4) >> 4)  * MAPLAYERS * map.height] )
						{
							// try 4 units away other direction.
							entity->x -= 4; // coordinates good.
						}
						break;
					default:
						break;
				}
			}

			entity->roll = 0; // flip the model
			if ( parent )
			{
				entity->parent = parent->getUID();
				if ( parent->behavior == &actPlayer )
				{
					entity->skill[17] = parent->skill[2];
				}
				else
				{
					entity->skill[17] = -1;
				}
			}
			entity->sizex = 4;
			entity->sizey = 4;
			entity->skill[11] = this->status;
			entity->skill[12] = this->beatitude;
			entity->skill[14] = this->appearance;
			entity->skill[15] = this->identified;
			entity->skill[16] = placement;
			entity->skill[20] = dir;
			entity->skill[21] = type;
		}
	}
	else if ( placement == BOMB_CHEST || placement == BOMB_DOOR )
	{
		if ( thrown && onEntity && (hit.entity == onEntity) )
		{
			Entity* entity = newEntity(sprite, 1, map.entities, nullptr); //Beartrap entity.
			entity->behavior = &actBomb;
			entity->flags[PASSABLE] = true;
			entity->flags[UPDATENEEDED] = true;
			entity->x = thrown->x;
			entity->y = thrown->y;
			entity->z = std::min(4.0, thrown->z);
			if ( placement == BOMB_CHEST )
			{
				entity->z = std::max(2.0, entity->z);
			}

			if ( hit.side == 0 )
			{
				// pick a random side to be on.
				if ( rand() % 2 == 0 )
				{
					hit.side = HORIZONTAL;
				}
				else
				{
					hit.side = VERTICAL;
				}
			}

			if ( hit.side == HORIZONTAL )
			{
				if ( thrown->vel_x > 0 )
				{
					dir = BOMB_WEST;
				}
				else
				{
					dir = BOMB_EAST;
				}
			}
			else if ( hit.side == VERTICAL )
			{
				if ( thrown->vel_y > 0 )
				{
					dir = BOMB_NORTH;
				}
				else
				{
					dir = BOMB_SOUTH;
				}
			}
			real_t height = 0;
			if ( placement == BOMB_CHEST )
			{
				if ( onEntity->yaw == 0 || onEntity->yaw == PI ) //EAST/WEST FACING
				{
					if ( hit.side == HORIZONTAL )
					{
						height = 5.25;
					}
					else
					{
						height = 4.25;
					}
				}
				else if ( onEntity->yaw == PI / 2 || onEntity->yaw == 3 * PI / 2 ) //SOUTH/NORTH FACING
				{
					if ( hit.side == VERTICAL )
					{
						height = 4.25;
					}
					else
					{
						height = 5.25;
					}
				}
			}
			else if ( placement == BOMB_DOOR )
			{
				if ( onEntity->yaw == 0 || onEntity->yaw == PI ) //EAST/WEST FACING
				{
					if ( hit.side == HORIZONTAL )
					{
						height = 2;
					}
				}
				else if ( onEntity->yaw == -PI / 2 ) //SOUTH/NORTH FACING
				{
					if ( hit.side == VERTICAL )
					{
						height = 2;
					}
				}
			}

			switch ( dir )
			{
				case BOMB_EAST:
					entity->yaw = 3 * PI / 2;
					entity->x = onEntity->x;
					if ( onEntity->yaw == 0 && placement == BOMB_CHEST )
					{
						height += 0.5;
					}
					entity->x += height;
					break;
				case BOMB_SOUTH:
					entity->yaw = 0;
					entity->y = onEntity->y;
					if ( onEntity->yaw == PI / 2 && placement == BOMB_CHEST )
					{
						height -= 0.5;
					}
					entity->y += height;
					break;
				case BOMB_WEST:
					entity->yaw = PI / 2;
					entity->x = onEntity->x;
					if ( onEntity->yaw == PI && placement == BOMB_CHEST )
					{
						height -= 0.5;
					}
					entity->x -= height;
					break;
				case BOMB_NORTH:
					entity->yaw = PI;
					entity->y = onEntity->y;
					if ( onEntity->yaw == 3 * PI / 2 && placement == BOMB_CHEST )
					{
						height += 0.5;
					}
					entity->y -= height;
					break;
				default:
					break;
			}
			entity->roll = 0; // flip the model
			if ( parent )
			{
				entity->parent = parent->getUID();
				if ( parent->behavior == &actPlayer )
				{
					entity->skill[17] = parent->skill[2];
				}
				else
				{
					entity->skill[17] = -1;
				}
			}
			entity->sizex = 4;
			entity->sizey = 4;
			entity->skill[11] = this->status;
			entity->skill[12] = this->beatitude;
			entity->skill[14] = this->appearance;
			entity->skill[15] = this->identified;
			entity->skill[16] = placement;
			entity->skill[18] = static_cast<Sint32>(onEntity->getUID());
			if ( placement == BOMB_DOOR )
			{
				entity->skill[19] = onEntity->doorHealth;
			}
			else if ( placement == BOMB_CHEST )
			{
				entity->skill[19] = onEntity->skill[3]; //chestHealth
				entity->skill[22] = onEntity->skill[1];
			}
			entity->skill[20] = dir;
			entity->skill[21] = type;
		}
	}
}

void Item::applyTinkeringCreation(Entity* parent, Entity* thrown)
{
	if ( !thrown )
	{
		return;
	}
	if ( type == TOOL_DECOY )
	{
		Entity* entity = newEntity(894, 1, map.entities, nullptr); //Decoy box.
		entity->behavior = &actDecoyBox;
		entity->flags[PASSABLE] = true;
		entity->flags[UPDATENEEDED] = true;
		entity->x = thrown->x;
		entity->y = thrown->y;
		entity->z = limbs[DUMMYBOT][9][2] - 0.25;
		entity->yaw = thrown->yaw;
		entity->skill[2] = -14;
		if ( parent )
		{
			entity->parent = parent->getUID();
		}
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[10] = this->type;
		entity->skill[11] = this->status;
		entity->skill[12] = this->beatitude;
		entity->skill[13] = 1;
		entity->skill[14] = this->appearance;
		entity->skill[15] = 1;
	}
	else if ( type == TOOL_DUMMYBOT || type == TOOL_GYROBOT || type == TOOL_SENTRYBOT || type == TOOL_SPELLBOT )
	{
		Monster monsterType = DUMMYBOT;
		if ( type == TOOL_GYROBOT )
		{
			monsterType = GYROBOT;
		}
		else if ( type == TOOL_SENTRYBOT )
		{
			monsterType = SENTRYBOT;
		}
		else if ( type == TOOL_SPELLBOT )
		{
			monsterType = SPELLBOT;
		}

		bool exactLocation = true;
		Entity* summon = summonMonster(monsterType, thrown->x, thrown->y, true);
		if ( !summon )
		{
			exactLocation = false;
			summon = summonMonster(monsterType, floor(thrown->x / 16) * 16 + 8, floor(thrown->y / 16) * 16 + 8, false);
		}
		if ( summon )
		{
			Stat* summonedStats = summon->getStats();
			if ( parent && parent->behavior == &actPlayer && summonedStats )
			{
				if ( summonedStats->type == GYROBOT )
				{
					summon->yaw = thrown->yaw;
					summon->monsterSpecialState = GYRO_START_FLYING;
					serverUpdateEntitySkill(summon, 33);
					playSoundPos(summon->x, summon->y, 415, 128);
				}
				else if ( summonedStats->type == SENTRYBOT || summonedStats->type == SPELLBOT )
				{
					summon->yaw = thrown->yaw;
					if ( exactLocation )
					{
						summon->x = thrown->x;
						summon->y = thrown->y;
					}
					summonedStats->EFFECTS[EFF_STUNNED] = true;
					summonedStats->EFFECTS_TIMERS[EFF_STUNNED] = 30;
					playSoundEntity(summon, 453 + rand() % 2, 192);
				}
				else
				{
					summon->yaw = thrown->yaw + ((PI / 2) * (rand() % 4));
					if ( summonedStats->type == DUMMYBOT )
					{
						playSoundEntity(summon, 417 + rand() % 3, 128);
					}
				}
				summonedStats->monsterTinkeringStatus = static_cast<Sint32>(this->status); // store the type of item that was used to summon me.
				summon->tinkerBotSetStats(summonedStats, this->status);
				if ( !this->tinkeringBotIsMaxHealth() )
				{
					summon->setHP(monsterTinkeringConvertAppearanceToHP(summonedStats, this->appearance));
				}
				if ( forceFollower(*parent, *summon) )
				{
					if ( parent->behavior == &actPlayer )
					{
						summon->monsterAllyIndex = parent->skill[2];
						if ( multiplayer == SERVER )
						{
							serverUpdateEntitySkill(summon, 42); // update monsterAllyIndex for clients.
						}
					}
					// change the color of the hit entity.
					summon->flags[USERFLAG2] = true;
					serverUpdateEntityFlag(summon, USERFLAG2);
				}
			}
		}
	}
}

bool itemIsThrowableTinkerTool(const Item* item)
{
	if ( !item )
	{
		return false;
	}
	if ( (item->type >= TOOL_BOMB && item->type <= TOOL_TELEPORT_BOMB)
		|| item->type == TOOL_DECOY
		|| item->type == TOOL_SENTRYBOT 
		|| item->type == TOOL_SPELLBOT 
		|| item->type == TOOL_GYROBOT
		|| item->type == TOOL_DUMMYBOT )
	{
		return true;
	}
	return false;
}