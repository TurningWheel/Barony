/*-------------------------------------------------------------------------------

	BARONY
	File: item_usage_funcs.cpp
	Desc: implements the functions that handle item usage (eg handle
	potion effects, zap a staff, whatever else you can think of).

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "items.hpp"
#include "engine/audio/sound.hpp"
#include "entity.hpp"
#include "magic/magic.hpp"
#include "interface/interface.hpp"
#include "scores.hpp"
#include "net.hpp"
#include "monster.hpp"
#include "player.hpp"
#include "collision.hpp"
#include "scores.hpp"
#include "prng.hpp"
#include "mod_tools.hpp"
#include "scrolls.hpp"

bool item_PotionWater(Item*& item, Entity* entity, Entity* usedBy)
{
	if ( !entity )
	{
		return false;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
		}
	}

	node_t* node;
	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return false;
	}

	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION
			&& stats->type != SKELETON )
		{
			if ( player >= 0 && players[player]->isLocalPlayer() )
			{
				messagePlayer(player, MESSAGE_STATUS, Language::get(750));
				playSoundPlayer(player, 90, 64);
			}
			return false;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player >= 0 && players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_STATUS, Language::get(751));
			playSoundPlayer(player, 90, 64);
		}
		return false;
	}

	if ( multiplayer != CLIENT ) // server/singleplayer
	{
		// play drink sound
		if ( item->beatitude > 0 )
		{
			if ( stats->type == GHOUL ||
				stats->type == LICH ||
				stats->type == LICH_FIRE ||
				stats->type == LICH_ICE ||
				stats->type == SHADOW ||
				stats->type == SKELETON ||
				stats->type == VAMPIRE )
			{
				//Blessed water damages undead.
				int damage = -(20 * item->beatitude);
				entity->modHP(damage);
				playSoundEntity(entity, 28, 64);
				playSoundEntity(entity, 249, 128);
				entity->setObituary(Language::get(1533));
		        stats->killer = KilledBy::WATER;
			}
			else
			{
				if ( stats->type != AUTOMATON )
				{
					entity->modHP(item->potionGetEffectHealth(entity, stats));
					playSoundEntity(entity, 168, 128);
					spawnMagicEffectParticles(entity->x, entity->y, entity->z, 169);
				}
				playSoundEntity(entity, 52, 64);
			}
		}
		else
		{
			if ( stats->type == VAMPIRE )
			{
				entity->modHP(-5);
				playSoundEntity(entity, 28, 64);
				playSoundEntity(entity, 249, 128);
				entity->setObituary(Language::get(1533));
		        stats->killer = KilledBy::WATER;
			}
			else if ( stats->type != AUTOMATON )
			{
				entity->modHP(item->potionGetEffectHealth(entity, stats));
				playSoundEntity(entity, 168, 128);
				spawnMagicEffectParticles(entity->x, entity->y, entity->z, 169);
			}
			else
			{
				playSoundEntity(entity, 52, 64);
			}
		}
		if ( player >= 0 && player < MAXPLAYERS )
		{
			if ( stats && stats->EFFECTS[EFF_POLYMORPH] )
			{
				if ( stats->EFFECTS[EFF_POLYMORPH] )
				{
					entity->setEffect(EFF_POLYMORPH, false, 0, true);
					entity->effectPolymorph = 0;
					serverUpdateEntitySkill(entity, 50);

					messagePlayer(player, MESSAGE_STATUS, Language::get(3192));
					if ( !stats->EFFECTS[EFF_SHAPESHIFT] )
					{
						messagePlayer(player, MESSAGE_STATUS, Language::get(3185));
					}
					else
					{
						messagePlayer(player, MESSAGE_STATUS, Language::get(4303));
					}
				}
				playSoundEntity(entity, 400, 92);
				createParticleDropRising(entity, 593, 1.f);
				serverSpawnMiscParticles(entity, PARTICLE_EFFECT_RISING_DROP, 593);
			}
			if ( stats->type == AUTOMATON )
			{
				Uint32 color = makeColorRGB(255, 128, 0);
				messagePlayerColor(player, MESSAGE_STATUS, color, Language::get(3700));
				stats->HUNGER -= 200; //Lose boiler
				int mpAmount = 3 + local_rng.rand() % 6;
				if ( item->beatitude > 0 )
				{
					mpAmount += 2 + local_rng.rand() % 4;
				}
				players[player]->entity->modMP(mpAmount); //Raise temperature because steam.
				serverUpdateHunger(player);
			}
		}
		if ( player >= 0 && !players[player]->isLocalPlayer() )
		{
			consumeItem(item, player);
			return true;
		}
	}

	auto& camera_shakex = cameravars[player >= 0 ? player : 0].shakex;
	auto& camera_shakey = cameravars[player >= 0 ? player : 0].shakey;

	// code below is only run by the player that drank the potion.
	// if it was thrown, then the function returns in the above code as processed by the server.

	if ( item->beatitude == 0 )
	{
		if ( stats->type == VAMPIRE )
		{
			Uint32 color = makeColorRGB(255, 0, 0);
			messagePlayerColor(player, MESSAGE_STATUS, color, Language::get(3183));
			camera_shakex += .1;
			camera_shakey += 10;
		}
		else if ( stats->type != AUTOMATON )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(752));
		}
	}
	else if ( item->beatitude > 0 )
	{
		if ( stats->type == SKELETON )
		{
			Uint32 color = makeColorRGB(255, 0, 0);
			messagePlayerColor(player, MESSAGE_STATUS, color, Language::get(3184));
			camera_shakex += .1;
			camera_shakey += 10;
		}
		else if ( stats->type == GHOUL ||
			stats->type == LICH ||
			stats->type == LICH_FIRE ||
			stats->type == LICH_ICE ||
			stats->type == SHADOW ||
			stats->type == VAMPIRE )
		{
			Uint32 color = makeColorRGB(255, 0, 0);
			messagePlayerColor(player, MESSAGE_STATUS, color, Language::get(3183));
			camera_shakex += .1;
			camera_shakey += 10;
		}
		else if ( stats->type != AUTOMATON )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(753));
		}
	}
	else if ( item->beatitude < 0 )
	{
		if ( stats->type == VAMPIRE )
		{
			Uint32 color = makeColorRGB(255, 0, 0);
			messagePlayerColor(player, MESSAGE_STATUS, color, Language::get(3183));
			camera_shakex += .1;
			camera_shakey += 10;
		}
		messagePlayer(player, MESSAGE_HINT, Language::get(755));

		// choose a random piece of worn equipment to curse!
		int tryIndex = local_rng.rand() % 8;
		int startIndex = tryIndex;
		int armornum = 0;
		bool breakloop = false;
		Item* toCurse = nullptr;
		// curse random item.
		while ( !toCurse && !breakloop )
		{
			switch ( tryIndex )
			{
				// intentional fall throughs...
				case 0:
					if ( stats->weapon != nullptr && itemCategory(stats->weapon) != POTION
						&& stats->weapon->beatitude >= 0 )
					{
						toCurse = stats->weapon;
						armornum = 0;
						break;
					}
				case 1:
					if ( stats->helmet != nullptr && stats->helmet->beatitude >= 0 )
					{
						toCurse = stats->helmet;
						armornum = 1;
						break;
					}
				case 2:
					if ( stats->breastplate != nullptr && stats->breastplate->beatitude >= 0 )
					{
						toCurse = stats->breastplate;
						armornum = 2;
						break;
					}
				case 3:
					if ( stats->gloves != nullptr && stats->gloves->beatitude >= 0 )
					{
						toCurse = stats->gloves;
						armornum = 3;
						break;
					}
				case 4:
					if ( stats->shoes != nullptr && stats->shoes->beatitude >= 0 )
					{
						toCurse = stats->shoes;
						armornum = 4;
						break;
					}
				case 5:
					if ( stats->shield != nullptr && stats->shield->beatitude >= 0 )
					{
						toCurse = stats->shield;
						armornum = 5;
						break;
					}
				case 6:
					if ( stats->cloak != nullptr && stats->cloak->beatitude >= 0 )
					{
						toCurse = stats->cloak;
						armornum = 6;
						break;
					}
				case 7:
					if ( stats->mask != nullptr && stats->mask->beatitude >= 0 )
					{
						toCurse = stats->mask;
						armornum = 7;
						break;
					}
					++tryIndex;
					if ( tryIndex > 7 )
					{
						// loop back around.
						tryIndex = 0;
					}
					if ( tryIndex == startIndex )
					{
						// couldn't find a piece of armor, break.
						breakloop = true;
						toCurse = nullptr;
						break;
					}
				default:
					break;
			}
		}

		if ( toCurse )
		{
			if ( toCurse->beatitude <= 0 )
			{
				--toCurse->beatitude;
			}
			else
			{
				toCurse->beatitude = -toCurse->beatitude;
				if ( itemCategory(toCurse) == WEAPON && stats->type == SUCCUBUS )
				{
					steamAchievement("BARONY_ACH_THE_WAY_YOU_LIKE_IT");
				}
			}
			messagePlayer(player, MESSAGE_HINT, Language::get(858), toCurse->getName());
			if ( multiplayer == CLIENT )
			{
				strcpy((char*)net_packet->data, "BEAT");
				net_packet->data[4] = player;
				net_packet->data[5] = armornum;
				net_packet->data[6] = toCurse->beatitude + 100;
				SDLNet_Write16((Sint16)toCurse->type, &net_packet->data[7]);
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 9;
				sendPacketSafe(net_sock, -1, net_packet, 0);
				//messagePlayer(player, "sent server: %d, %d, %d", net_packet->data[4], net_packet->data[5], net_packet->data[6]);
			}
			consumeItem(item, player);
			return true;
		}

		// else randomly curse an item in the entity's inventory, item must be +0 or higher.
		int items = 0;
		for ( node = stats->inventory.first; node != NULL; node = node->next )
		{
			Item* target = (Item*)node->element;
			if ( target && !itemIsEquipped(target, player) && itemCategory(target) != SPELL_CAT && target->beatitude >= 0 )
			{
				items++;
			}
		}
		if ( items == 0 )
		{
			consumeItem(item, player);
			return true;
		}
		int itemToCurse = local_rng.rand() % items;
		items = 0;
		for ( node = stats->inventory.first; node != NULL; node = node->next )
		{
			Item* target = (Item*)node->element;
			if ( target && !itemIsEquipped(target, player) && itemCategory(target) != SPELL_CAT && target->beatitude >= 0 )
			{
				if ( items == itemToCurse )
				{
					messagePlayer(player, MESSAGE_HINT, Language::get(858), target->getName());
					if ( target->beatitude <= 0 )
					{
						--target->beatitude;
					}
					else
					{
						target->beatitude = -target->beatitude;
						if ( itemCategory(target) == WEAPON && stats->type == SUCCUBUS )
						{
							steamAchievement("BARONY_ACH_THE_WAY_YOU_LIKE_IT");
						}
					}
					break;
				}
				items++;
			}
		}
	}
	consumeItem(item, player);
	return true;
}

bool item_PotionBooze(Item*& item, Entity* entity, Entity* usedBy, bool shouldConsumeItem)
{
	if (!entity)
	{
		return false;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return false;
	}

	if ( stats->amulet != nullptr )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION 
			&& stats->type != SKELETON )
		{
			if ( player >= 0 && players[player]->isLocalPlayer() )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(750));
				playSoundPlayer(player, 90, 64);
			}
			return false;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player >= 0 && players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(751));
			playSoundPlayer(player, 90, 64);
		}
		return false;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item, player);
		return true;
	}

	messagePlayer(player, MESSAGE_WORLD, Language::get(758));
	messagePlayer(player, MESSAGE_STATUS, Language::get(759));
	stats->EFFECTS[EFF_DRUNK] = true;
	if ( player >= 0 )
	{
		if ( stats->type == GOATMAN )
		{
			stats->EFFECTS_TIMERS[EFF_DRUNK] = std::max(item->potionGetEffectDurationRandom(entity, stats), stats->EFFECTS_TIMERS[EFF_DRUNK]);
		}
		else if ( stats->type != GOATMAN )
		{
			stats->EFFECTS_TIMERS[EFF_DRUNK] = item->potionGetEffectDurationRandom(entity, stats);
			stats->EFFECTS_TIMERS[EFF_DRUNK] = std::max(300, stats->EFFECTS_TIMERS[EFF_DRUNK] - (entity->getPER() + entity->getCON()) * 40);
		}
		if ( stats->EFFECTS[EFF_WITHDRAWAL] )
		{
			int hangoverReliefDuration = EFFECT_WITHDRAWAL_BASE_TIME; // 8 minutes
			switch ( local_rng.rand() % 3 )
			{
				case 0:
					hangoverReliefDuration += (TICKS_PER_SECOND * 60 * 8); // 8 + 8 minutes
					break;
				case 1:
					hangoverReliefDuration += (TICKS_PER_SECOND * 60 * 4); // 8 + 4 minutes
					break;
				case 2:
					// intentional fall through
				default:
					break;
			}
			entity->setEffect(EFF_WITHDRAWAL, false, hangoverReliefDuration, true);
			serverUpdatePlayerGameplayStats(player, STATISTICS_FUNCTIONAL, 1);
			messagePlayerColor(player, MESSAGE_STATUS, makeColorRGB(0, 255, 0), Language::get(3250));
		}
		else if ( stats->EFFECTS_TIMERS[EFF_WITHDRAWAL] > 0 && stats->EFFECTS_TIMERS[EFF_WITHDRAWAL] < EFFECT_WITHDRAWAL_BASE_TIME )
		{
			stats->EFFECTS_TIMERS[EFF_WITHDRAWAL] = EFFECT_WITHDRAWAL_BASE_TIME;
		}
	}
	else
	{
		stats->EFFECTS_TIMERS[EFF_DRUNK] = item->potionGetEffectDurationRandom(entity, stats);
	}

	if ( svFlags & SV_FLAG_HUNGER )
	{
		if ( entity->behavior == &actPlayer )
		{
			if ( stats->type != SKELETON && stats->type != AUTOMATON )
			{
				if ( stats->HUNGER < 1500 )
				{
					stats->HUNGER = std::min(1499, stats->HUNGER + 100);
				}
			}
			if ( stats->playerRace == RACE_INSECTOID && stats->stat_appearance == 0 )
			{
				stats->HUNGER += 250;
			}
		}
	}
	else
	{
		// hunger off.
		if ( entity->behavior == &actPlayer && stats->playerRace == RACE_INSECTOID && stats->stat_appearance == 0 )
		{
			entity->modMP(5 * (1 + item->beatitude));
		}
	}
	entity->modHP(item->potionGetEffectHealth(entity, stats));
	// results of eating
	if ( entity->behavior == &actPlayer && stats->type != SKELETON && stats->type != AUTOMATON )
	{
		updateHungerMessages(entity, stats, item);
	}
	else if ( player > 0 )
	{
		serverUpdateHunger(player);
	}
	serverUpdateEffects(player);

	// play drink sound
	playSoundEntity(entity, 52, 64);
	playSoundEntity(entity, 168, 128);
	spawnMagicEffectParticles(entity->x, entity->y, entity->z, 169);
	if ( shouldConsumeItem )
	{
		consumeItem(item, player);
		return true;
	}
	return false;
}

bool item_PotionJuice(Item*& item, Entity* entity, Entity* usedBy)
{
	if (!entity)
	{
		return false;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return false;
	}

	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION 
			&& stats->type != SKELETON )
		{
			if ( player >= 0 && players[player]->isLocalPlayer() )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(750));
				playSoundPlayer(player, 90, 64);
			}
			return false;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player >= 0 && players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(751));
			playSoundPlayer(player, 90, 64);
		}
		return false;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item, player);
		return true;
	}

	if ( item->beatitude < 0 )
	{
		//Cursed effect inebriates you.
		messagePlayer(player, MESSAGE_HINT, Language::get(2900));
		messagePlayer(player, MESSAGE_WORLD, Language::get(758));
		messagePlayer(player, MESSAGE_HINT, Language::get(759));
		stats->EFFECTS[EFF_DRUNK] = true;
		if ( player >= 0 )
		{
			if ( stats->type == GOATMAN )
			{
				stats->EFFECTS_TIMERS[EFF_DRUNK] = std::max(item->potionGetCursedEffectDurationRandom(entity, stats), stats->EFFECTS_TIMERS[EFF_DRUNK]);
			}
			else if ( stats->type != GOATMAN )
			{
				stats->EFFECTS_TIMERS[EFF_DRUNK] = item->potionGetCursedEffectDurationRandom(entity, stats);
				stats->EFFECTS_TIMERS[EFF_DRUNK] = std::max(300, stats->EFFECTS_TIMERS[EFF_DRUNK] - (entity->getPER() + entity->getCON()) * 40);
			}
			if ( stats->EFFECTS[EFF_WITHDRAWAL] )
			{
				int hangoverReliefDuration = EFFECT_WITHDRAWAL_BASE_TIME; // 8 minutes
				switch ( local_rng.rand() % 3 )
				{
					case 0:
						hangoverReliefDuration += (TICKS_PER_SECOND * 60 * 8); // 8 + 8 minutes
						break;
					case 1:
						hangoverReliefDuration += (TICKS_PER_SECOND * 60 * 4); // 8 + 4 minutes
						break;
					case 2:
						// intentional fall through
					default:
						break;
				}
				entity->setEffect(EFF_WITHDRAWAL, false, hangoverReliefDuration, true);
				serverUpdatePlayerGameplayStats(player, STATISTICS_FUNCTIONAL, 1);
				messagePlayerColor(player, MESSAGE_STATUS, makeColorRGB(0, 255, 0), Language::get(3250));
			}
			else if ( stats->EFFECTS_TIMERS[EFF_WITHDRAWAL] > 0 && stats->EFFECTS_TIMERS[EFF_WITHDRAWAL] < EFFECT_WITHDRAWAL_BASE_TIME )
			{
				stats->EFFECTS_TIMERS[EFF_WITHDRAWAL] = EFFECT_WITHDRAWAL_BASE_TIME;
			}
		}
		else
		{
			stats->EFFECTS_TIMERS[EFF_DRUNK] = item->potionGetCursedEffectDurationRandom(entity, stats);
		}
		entity->modHP(item->potionGetEffectHealth(entity, stats));

		if ( svFlags & SV_FLAG_HUNGER )
		{
			if ( entity->behavior == &actPlayer )
			{
				if ( stats->type != SKELETON && stats->type != AUTOMATON )
				{
					if ( stats->HUNGER < 1500 )
					{
						stats->HUNGER = std::min(1499, stats->HUNGER + 50);
					}
				}
				if ( stats->playerRace == RACE_INSECTOID && stats->stat_appearance == 0 )
				{
					stats->HUNGER += 200;
				}
			}
		}
		else
		{
			// hunger off.
			if ( entity->behavior == &actPlayer && stats->playerRace == RACE_INSECTOID && stats->stat_appearance == 0 )
			{
				entity->modMP(5);
			}
		}

		serverUpdateEffects(player);
	}
	else
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(760));
		entity->modHP(item->potionGetEffectHealth(entity, stats));

		if ( svFlags & SV_FLAG_HUNGER )
		{
			if ( entity->behavior == &actPlayer )
			{
				if ( stats->type != SKELETON && stats->type != AUTOMATON )
				{
					if ( stats->HUNGER < 1500 )
					{
						stats->HUNGER = std::min(1499, stats->HUNGER + 50);
					}
				}
				if ( stats->playerRace == RACE_INSECTOID && stats->stat_appearance == 0 )
				{
					stats->HUNGER += 200;
				}
			}
		}
		else
		{
			// hunger off.
			if ( entity->behavior == &actPlayer && stats->playerRace == RACE_INSECTOID && stats->stat_appearance == 0 )
			{
				entity->modMP(5 * (1 + item->beatitude));
			}
		}
	}

	// results of eating
	if ( entity->behavior == &actPlayer && stats->type != SKELETON && stats->type != AUTOMATON )
	{
		updateHungerMessages(entity, stats, item);
	}
	else if ( player > 0 )
	{
		serverUpdateHunger(player);
	}

	// play drink sound
	playSoundEntity(entity, 52, 64);
	playSoundEntity(entity, 168, 128);
	spawnMagicEffectParticles(entity->x, entity->y, entity->z, 169);
	consumeItem(item, player);
	return true;
}

bool item_PotionSickness(Item*& item, Entity* entity, Entity* usedBy)
{
	if (!entity)
	{
		return false;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return false;
	}

	auto& camera_shakex = cameravars[player >= 0 ? player : 0].shakex;
	auto& camera_shakey = cameravars[player >= 0 ? player : 0].shakey;

	if ( entity == NULL )
	{
		return false;
	}
	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION 
			&& stats->type != SKELETON )
		{
			if ( player >= 0 && players[player]->isLocalPlayer() )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(750));
				playSoundPlayer(player, 90, 64);
			}
			return false;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player >= 0 && players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(751));
			playSoundPlayer(player, 90, 64);
		}
		return false;
	}
	if ( multiplayer == CLIENT || player == 0 )
	{
		camera_shakex += .1;
		camera_shakey += 10;
		if ( multiplayer == CLIENT )
		{
			consumeItem(item, player);
			return true;
		}
	}

	int damage = (item->potionGetEffectDamage(entity, stats)) * potionDamageSkillMultipliers[std::min(skillLVL, 5)];
	int chance = damage / 8;
	if ( player >= 0 && usedBy == entity )
	{
		damage /= 2;
	}
	else
	{
		damage -= (local_rng.rand() % (1 + chance));
	}
	messagePlayer(player, MESSAGE_HINT, Language::get(761));
	int oldHP = stats->HP;
	entity->modHP(-damage);
	stats->EFFECTS[EFF_POISONED] = true;
	if ( usedBy && usedBy != entity )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			stats->poisonKiller = usedBy->getUID();
		}

		if ( usedBy->behavior == &actPlayer && stats->HP < oldHP)
		{
			Compendium_t::Events_t::eventUpdate(usedBy->skill[2], Compendium_t::CPDM_THROWN_DMG_TOTAL, item->type, oldHP - stats->HP);
		}
	}
	if ( stats->type == LICH || stats->type == SHOPKEEPER || stats->type == DEVIL
		|| stats->type == MINOTAUR || stats->type == LICH_FIRE || stats->type == LICH_ICE )
	{
		stats->EFFECTS_TIMERS[EFF_POISONED] = TICKS_PER_SECOND * 15;
	}
	playSoundEntity(entity, 28, 64);
	serverUpdateEffects(player);

	// set obituary
	entity->setObituary(Language::get(1535));
    stats->killer = KilledBy::FUNNY_POTION;

	// play drink sound
	playSoundEntity(entity, 52, 64);
	consumeItem(item, player);
	return true;
}

bool item_PotionConfusion(Item*& item, Entity* entity, Entity* usedBy)
{
	if (!entity)
	{
		return false;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return false;
	}

	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION
			&& stats->type != SKELETON )
		{
			if ( player >= 0 && players[player]->isLocalPlayer() )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(750));
				playSoundPlayer(player, 90, 64);
			}
			return false;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player >= 0 && players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(751));
			playSoundPlayer(player, 90, 64);
		}
		return false;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item, player);
		return true;
	}

	messagePlayer(player, MESSAGE_HINT, Language::get(762));
	int duration = 0;
	if ( player >= 0 )
	{
		 duration = std::max(300, item->potionGetEffectDurationRandom(entity, stats) - (entity->getPER() + entity->getCON()) * 20);
	}
	else
	{
		duration = item->potionGetEffectDurationRandom(entity, stats);
	}
	if ( entity->setEffect(EFF_CONFUSED, true, duration, false) )
	{
		if ( entity->behavior == &actMonster )
		{
			entity->monsterTarget = 0; // monsters forget what they're doing
		}
		if ( usedBy && entity != usedBy && usedBy->behavior == &actPlayer )
		{
			Uint32 color = makeColorRGB(0, 255, 0);
			messagePlayerMonsterEvent(usedBy->skill[2], color, *stats, Language::get(391), Language::get(390), MSG_COMBAT);
		}
	}
	else
	{
		if ( usedBy && entity != usedBy && usedBy->behavior == &actPlayer )
		{
			Uint32 color = makeColorRGB(255, 0, 0);
			messagePlayerMonsterEvent(usedBy->skill[2], color, *stats, Language::get(4320), Language::get(4321), MSG_COMBAT);
		}
	}

	// play drink sound
	playSoundEntity(entity, 52, 64);
	consumeItem(item, player);
	return true;
}

bool item_PotionCureAilment(Item*& item, Entity* entity, Entity* usedBy)
{
	if (!entity)
	{
		return false;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
		}
	}

	int player = -1;
	Stat* stats;
	int c;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return false;
	}

	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION 
			&& stats->type != SKELETON )
		{
			if ( player >= 0 && players[player]->isLocalPlayer() )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(750));
				playSoundPlayer(player, 90, 64);
			}
			return false;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player >= 0 && players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(751));
			playSoundPlayer(player, 90, 64);
		}
		return false;
	}

	int numEffectsCured = 0;

	if ( entity->flags[BURNING] )
	{
		++numEffectsCured;
		entity->flags[BURNING] = false;
		serverUpdateEntityFlag(entity, BURNING);
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item, player);
		return true;
	}

	if ( item->beatitude < 0 )
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(2900));
	}

	for ( c = 0; c < NUMEFFECTS; c++ )   //This does a whole lot more than just cure ailments.
	{
		if ( stats->statusEffectRemovedByCureAilment(c, entity) )
		{
			if ( stats->EFFECTS[c] )
			{
				stats->EFFECTS[c] = false;
				if ( stats->EFFECTS_TIMERS[c] > 0 )
				{
					stats->EFFECTS_TIMERS[c] = 1;
				}
				++numEffectsCured;
			}
		}
	}

	if ( stats->EFFECTS[EFF_WITHDRAWAL] )
	{
		++numEffectsCured;
		entity->setEffect(EFF_WITHDRAWAL, false, EFFECT_WITHDRAWAL_BASE_TIME, true);
		serverUpdatePlayerGameplayStats(player, STATISTICS_FUNCTIONAL, 1);
	}

	if ( numEffectsCured > 0 || item->beatitude > 0 )
	{
		messagePlayerColor(player, MESSAGE_STATUS, makeColorRGB(0, 255, 0), Language::get(763));
	}
	else
	{
		if ( item->beatitude == 0 )
		{
			messagePlayer(player, MESSAGE_STATUS, Language::get(4312));
		}
	}

	if ( item->beatitude < 0 )
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(2903));
		stats->EFFECTS[EFF_POISONED] = true;
		stats->EFFECTS_TIMERS[EFF_POISONED] = item->potionGetCursedEffectDurationRandom(entity, stats);
	}
	else if ( item->beatitude > 0 )
	{
		stats->EFFECTS[EFF_HP_REGEN] = true;
		stats->EFFECTS[EFF_MP_REGEN] = true;
		stats->EFFECTS_TIMERS[EFF_HP_REGEN] += item->potionGetEffectDurationRandom(entity, stats);
		stats->EFFECTS_TIMERS[EFF_MP_REGEN] += stats->EFFECTS_TIMERS[EFF_HP_REGEN];
	}

	serverUpdateEffects(player);

	// play drink sound
	playSoundEntity(entity, 52, 64);
	consumeItem(item, player);
	return true;
}

bool item_PotionBlindness(Item*& item, Entity* entity, Entity* usedBy)
{
	if (!entity)
	{
		return false;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return false;
	}

	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION
			&& stats->type != SKELETON )
		{
			if ( player >= 0 && players[player]->isLocalPlayer() )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(750));
				playSoundPlayer(player, 90, 64);
			}
			return false;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player >= 0 && players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(751));
			playSoundPlayer(player, 90, 64);
		}
		return false;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item, player);
		return true;
	}

	int duration = item->potionGetEffectDurationRandom(entity, stats);
	if ( player >= 0 )
	{
		duration = std::max(300, stats->EFFECTS_TIMERS[EFF_BLIND] - (entity->getPER() + entity->getCON()) * 5);
	}

	if ( entity->setEffect(EFF_BLIND, true, duration, true) )
	{
		if ( entity->behavior == &actMonster && !entity->isBossMonster() )
		{
			entity->monsterReleaseAttackTarget();
		}
		messagePlayer(player, MESSAGE_HINT, Language::get(765));
	}

	// play drink sound
	playSoundEntity(entity, 52, 64);
	consumeItem(item, player);
	return true;
}

bool item_PotionInvisibility(Item*& item, Entity* entity, Entity* usedBy)
{
	if (!entity)
	{
		return false;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return false;
	}

	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION
			&& stats->type != SKELETON )
		{
			if ( player >= 0 && players[player]->isLocalPlayer() )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(750));
				playSoundPlayer(player, 90, 64);
			}
			return false;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player >= 0 && players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(751));
			playSoundPlayer(player, 90, 64);
		}
		return false;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item, player);
		return true;
	}

	messagePlayer(player, MESSAGE_STATUS | MESSAGE_HINT, Language::get(766));

	if ( !entity->isInvisible() )
	{
		for ( node_t* node = map.creatures->first; node != nullptr; node = node->next )
		{
			Entity* creature = (Entity*)node->element;
			if ( creature && creature->behavior == &actMonster && creature->monsterTarget == entity->getUID() )
			{
				if ( !creature->isBossMonster() )
				{
					//Abort if invalid creature (boss, shopkeep, etc).
					real_t dist = entityDist(entity, creature);
					if ( dist > STRIKERANGE * 3 )
					{
						// lose track of invis target.
						creature->monsterReleaseAttackTarget();
					}
				}
			}
		}
	}
	stats->EFFECTS[EFF_INVISIBLE] = true;
	stats->EFFECTS_TIMERS[EFF_INVISIBLE] = item->potionGetEffectDurationRandom(entity, stats);

	serverUpdateEffects(player);

	// play drink sound
	playSoundEntity(entity, 52, 64);
	consumeItem(item, player);
	return true;
}

bool item_PotionLevitation(Item*& item, Entity* entity, Entity* usedBy)
{
	if (!entity)
	{
		return false;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return false;
	}

	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION
			&& stats->type != SKELETON )
		{
			if ( player >= 0 && players[player]->isLocalPlayer() )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(750));
				playSoundPlayer(player, 90, 64);
			}
			return false;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player >= 0 && players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(751));
			playSoundPlayer(player, 90, 64);
		}
		return false;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item, player);
		return true;
	}

	if ( item->beatitude < 0 )
	{
		//Cursed effect slows you.
		messagePlayer(player, MESSAGE_HINT, Language::get(2900));
		messagePlayer(player, MESSAGE_HINT, Language::get(2901));
		stats->EFFECTS[EFF_SLOW] = true;
		stats->EFFECTS_TIMERS[EFF_SLOW] = item->potionGetCursedEffectDurationRandom(entity, stats);
	}
	else
	{
		messagePlayer(player, MESSAGE_STATUS, Language::get(767));
		stats->EFFECTS[EFF_LEVITATING] = true;
		stats->EFFECTS_TIMERS[EFF_LEVITATING] = item->potionGetEffectDurationRandom(entity, stats);
	}
	serverUpdateEffects(player);

	// play drink sound
	playSoundEntity(entity, 52, 64);
	consumeItem(item, player);
	return true;
}

bool item_PotionSpeed(Item*& item, Entity* entity, Entity* usedBy)
{
	if (!entity)
	{
		return false;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return false;
	}

	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION
			&& stats->type != SKELETON )
		{
			if ( player >= 0 && players[player]->isLocalPlayer() )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(750));
				playSoundPlayer(player, 90, 64);
			}
			return false;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player >= 0 && players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(751));
			playSoundPlayer(player, 90, 64);
		}
		return false;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item, player);
		return true;
	}


	if ( item->beatitude < 0 )
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(2900));
		//Cursed effect slows you.
		if ( stats->EFFECTS[EFF_FAST] )
		{
			messagePlayer(player, MESSAGE_STATUS, Language::get(769));
			stats->EFFECTS[EFF_FAST] = false;
			stats->EFFECTS_TIMERS[EFF_FAST] = 0;
		}
		else
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(2902));
			stats->EFFECTS[EFF_SLOW] = true;
			stats->EFFECTS_TIMERS[EFF_SLOW] = item->potionGetCursedEffectDurationRandom(entity, stats);
		}
	}
	else
	{
		if ( !stats->EFFECTS[EFF_SLOW] )
		{
			messagePlayer(player, MESSAGE_STATUS, Language::get(768));
			stats->EFFECTS[EFF_FAST] = true;
			stats->EFFECTS_TIMERS[EFF_FAST] += item->potionGetEffectDurationRandom(entity, stats);
		}
		else
		{
			messagePlayer(player, MESSAGE_STATUS, Language::get(769));
			stats->EFFECTS[EFF_SLOW] = false;
			stats->EFFECTS_TIMERS[EFF_SLOW] = 0;
		}
	}
	serverUpdateEffects(player);

	// play drink sound
	playSoundEntity(entity, 52, 64);
	consumeItem(item, player);
	return true;
}

bool item_PotionStrength(Item*& item, Entity* entity, Entity* usedBy)
{
	if ( !entity )
	{
		return false;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return false;
	}

	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION
			&& stats->type != SKELETON )
		{
			if ( player >= 0 && players[player]->isLocalPlayer() )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(750));
				playSoundPlayer(player, 90, 64);
			}
			return false;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player >= 0 && players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(751));
			playSoundPlayer(player, 90, 64);
		}
		return false;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item, player);
		return true;
	}


	if ( item->beatitude < 0 )
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(2900));
		//Cursed effect blinds you.
		int duration = item->potionGetCursedEffectDurationRandom(entity, stats);
		if ( player >= 0 )
		{
			duration = std::max(300, stats->EFFECTS_TIMERS[EFF_BLIND] - (entity->getPER() + entity->getCON()) * 5);
		}
		if ( entity->setEffect(EFF_BLIND, true, duration, true) )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(765));
		}
	}
	else
	{
		messagePlayer(player, MESSAGE_STATUS, Language::get(3354));
		stats->EFFECTS[EFF_POTION_STR] = true;
		stats->EFFECTS_TIMERS[EFF_POTION_STR] = item->potionGetEffectDurationRandom(entity, stats);
	}
	serverUpdateEffects(player);

	// play drink sound
	playSoundEntity(entity, 52, 64);
	consumeItem(item, player);
	return true;
}

bool item_PotionAcid(Item*& item, Entity* entity, Entity* usedBy)
{
	if (!entity)
	{
		return false;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return false;
	}

	auto& camera_shakex = cameravars[player >= 0 ? player : 0].shakex;
	auto& camera_shakey = cameravars[player >= 0 ? player : 0].shakey;

	if ( entity == NULL )
	{
		return false;
	}
	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION
			&& stats->type != SKELETON )
		{
			if ( player >= 0 && players[player]->isLocalPlayer() )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(750));
				playSoundPlayer(player, 90, 64);
			}
			return false;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player >= 0 && players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(751));
			playSoundPlayer(player, 90, 64);
		}
		return false;
	}
	if ( multiplayer == CLIENT || player == 0 )
	{
		camera_shakex += .1;
		camera_shakey += 10;
		if ( multiplayer == CLIENT )
		{
			consumeItem(item, player);
			return true;
		}
	}

	int damage = (item->potionGetEffectDamage(entity, stats)) * potionDamageSkillMultipliers[std::min(skillLVL, 5)];
	int chance = damage / 8;
	if ( player >= 0 && usedBy == entity )
	{
		damage /= 2;
	}
	else
	{
		damage -= (local_rng.rand() % (1 + chance));
	}
	messagePlayer(player, MESSAGE_HINT, Language::get(770));
	int oldHP = stats->HP;
	entity->modHP(-damage);
	playSoundEntity(entity, 28, 64);

	if ( usedBy && usedBy != entity )
	{
		if ( usedBy->behavior == &actPlayer && stats->HP < oldHP )
		{
			Compendium_t::Events_t::eventUpdate(usedBy->skill[2], Compendium_t::CPDM_THROWN_DMG_TOTAL, item->type, oldHP - stats->HP);
		}
	}

	// set obituary
	entity->setObituary(Language::get(1535));
    stats->killer = KilledBy::FUNNY_POTION;

	// play drink sound
	playSoundEntity(entity, 52, 64);
	consumeItem(item, player);
	return true;
}

bool item_PotionUnstableStorm(Item*& item, Entity* entity, Entity* usedBy, Entity* thrownPotion)
{
	if ( !entity )
	{
		return false;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return false;
	}

	auto& camera_shakex = cameravars[player >= 0 ? player : 0].shakex;
	auto& camera_shakey = cameravars[player >= 0 ? player : 0].shakey;

	if ( entity == NULL )
	{
		return false;
	}
	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION
			&& stats->type != SKELETON )
		{
			if ( player >= 0 && players[player]->isLocalPlayer() )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(750));
				playSoundPlayer(player, 90, 64);
			}
			return false;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player >= 0 && players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(751));
			playSoundPlayer(player, 90, 64);
		}
		return false;
	}
	if ( multiplayer == CLIENT || (player >= 0 && players[player]->isLocalPlayer()) )
	{
		if ( player >= 0 && players[player]->isLocalPlayer() )
		{
			camera_shakex += .1;
			camera_shakey += 10;
		}
		if ( multiplayer == CLIENT )
		{
			consumeItem(item, player);
			return true;
		}
	}

	bool playerAutomatonDrink = false;
	if ( item->type == POTION_FIRESTORM && !thrownPotion && usedBy && usedBy == entity
		&& usedBy->behavior == &actPlayer && stats->type == AUTOMATON )
	{
		playerAutomatonDrink = true;
	}

	int damage = (item->potionGetEffectDamage(entity, stats)) * potionDamageSkillMultipliers[std::min(skillLVL, 5)];
	int chance = damage / 8;
	if ( player >= 0 && usedBy == entity )
	{
		damage /= 2;
	}
	else
	{
		damage -= (local_rng.rand() % (1 + chance));
	}
	if ( playerAutomatonDrink )
	{
		damage = 0;
	}
	else
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(770));
		int oldHP = stats->HP;
		entity->modHP(-damage);
		playSoundEntity(entity, 28, 64);

		if ( usedBy && usedBy != entity )
		{
			if ( usedBy->behavior == &actPlayer && stats->HP < oldHP )
			{
				Compendium_t::Events_t::eventUpdate(usedBy->skill[2], Compendium_t::CPDM_THROWN_DMG_TOTAL, item->type, oldHP - stats->HP);
			}
		}
	}

	// set obituary
	entity->setObituary(Language::get(1535));
    stats->killer = KilledBy::FUNNY_POTION;

	real_t x = entity->x;
	real_t y = entity->y;
	if ( thrownPotion )
	{
		// rather spawn at the potion impact rather than on the hit entity's coords.
		x = thrownPotion->x;
		y = thrownPotion->y;
	}
	if ( item->type == POTION_FIRESTORM )
	{
		if ( playerAutomatonDrink )
		{
			spawnMagicTower(usedBy, x, y, SPELL_FIREBALL, nullptr);
			stats->HUNGER = std::min(stats->HUNGER + 1500, 1500);
			players[player]->entity->modMP(stats->MAXMP);
			Uint32 color = makeColorRGB(255, 128, 0);
			messagePlayerColor(player, MESSAGE_STATUS, color, Language::get(3699)); // superheats
			serverUpdateHunger(player);
			for ( int c = 0; c < 100; c++ )
			{
				if ( Entity* entity = spawnFlame(players[player]->entity, SPRITE_FLAME) )
				{
					entity->sprite = 16;
					double vel = local_rng.rand() % 10;
					entity->vel_x = vel * cos(entity->yaw) * cos(entity->pitch) * .1;
					entity->vel_y = vel * sin(entity->yaw) * cos(entity->pitch) * .1;
					entity->vel_z = vel * sin(entity->pitch) * .2;
					entity->skill[0] = 5 + local_rng.rand() % 10;
				}
			}
		}
		else
		{
			spawnMagicTower(usedBy, x, y, SPELL_FIREBALL, entity);
		}
	}
	else if ( item->type == POTION_ICESTORM )
	{
		spawnMagicTower(usedBy, x, y, SPELL_COLD, entity);
	}
	else if ( item->type == POTION_THUNDERSTORM )
	{
		spawnMagicTower(usedBy, x, y, SPELL_LIGHTNING, entity);
	}

	// play drink sound
	playSoundEntity(entity, 52, 64);
	consumeItem(item, player);
	return true;
}

bool item_PotionParalysis(Item*& item, Entity* entity, Entity* usedBy)
{
	if (!entity)
	{
		return false;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return false;
	}

	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION
			&& stats->type != SKELETON )
		{
			if ( player >= 0 && players[player]->isLocalPlayer() )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(750));
				playSoundPlayer(player, 90, 64);
			}
			return false;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player >= 0 && players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(751));
			playSoundPlayer(player, 90, 64);
		}
		return false;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item, player);
		return true;
	}

	messagePlayer(player, MESSAGE_HINT, Language::get(771));
	int effectDuration = 0;
	if ( player >= 0 )
	{
		effectDuration = 420 + local_rng.rand() % 180;
		effectDuration = std::max(300, effectDuration - (entity->getCON()) * 5);
	}
	else
	{
		effectDuration = 420 + local_rng.rand() % 180;
	}
	if ( item->beatitude != 0 )
	{
		effectDuration += (abs(item->beatitude) * 100);
	}

	entity->setEffect(EFF_PARALYZED, true, effectDuration, false);
	serverUpdateEffects(player);

	// play drink sound
	playSoundEntity(entity, 52, 64);
	consumeItem(item, player);
	return true;
}

bool item_PotionHealing(Item*& item, Entity* entity, Entity* usedBy, bool shouldConsumeItem)
{
	if (!entity)
	{
		return false;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return false;
	}

	if ( stats->amulet != nullptr )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION
			&& stats->type != SKELETON )
		{
			if ( player >= 0 && players[player]->isLocalPlayer() )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(750));
				playSoundPlayer(player, 90, 64);
			}
			return false;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player >= 0 && players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(751));
			playSoundPlayer(player, 90, 64);
		}
		return false;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item, player);
		return true;
	}
	if ( stats->HP == stats->MAXHP )
	{
		playSoundEntity(entity, 52, 64);
		playSoundEntity(entity, 168, 128);
		spawnMagicEffectParticles(entity->x, entity->y, entity->z, 169);
		if ( item->beatitude < 0 )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(2900));
			messagePlayer(player, MESSAGE_HINT, Language::get(2903));
			stats->EFFECTS[EFF_POISONED] = true;
			stats->EFFECTS_TIMERS[EFF_POISONED] = item->potionGetCursedEffectDurationRandom(entity, stats);
		}
		else
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(772));
			// stop bleeding
			if ( stats->EFFECTS[EFF_BLEEDING] )
			{
				entity->setEffect(EFF_BLEEDING, false, 0, false);
			}
		}
		serverUpdateEffects(player);
		if ( shouldConsumeItem )
		{
			consumeItem(item, player);
			return true;
		}
		return false;
	}

	int amount = item->potionGetEffectHealth(entity, stats);
	
	if ( stats->type == GOATMAN && entity->behavior == &actMonster )
	{
		amount *= GOATMAN_HEALINGPOTION_MOD; //Goatman special.
		stats->EFFECTS[EFF_FAST] = true;
		stats->EFFECTS_TIMERS[EFF_FAST] = GOATMAN_HEALING_POTION_SPEED_BOOST_DURATION;
	}

	//Bonus from CON, to scale up healing potions as the game progresses.
	if ( statGetCON(stats, entity) > 0 )
	{
		amount += 2 * statGetCON(stats, entity);
	}

	if ( item->beatitude < 0 )
	{
		amount /= (std::abs(item->beatitude) * 2);
	}

	int oldHP = entity->getHP();

	entity->modHP(amount);

	int heal = std::max(entity->getHP() - oldHP, 0);
	if ( heal > 0 )
	{
		serverUpdatePlayerGameplayStats(player, STATISTICS_HEAL_BOT, heal);
	}

	// play drink sound
	playSoundEntity(entity, 52, 64);
	playSoundEntity(entity, 168, 128);
	spawnMagicEffectParticles(entity->x, entity->y, entity->z, 169);
	Uint32 color = makeColorRGB(0, 255, 0);

	if ( item->beatitude < 0 )
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(2900));
		messagePlayer(player, MESSAGE_HINT, Language::get(2903));
		stats->EFFECTS[EFF_POISONED] = true;
		stats->EFFECTS_TIMERS[EFF_POISONED] = item->potionGetCursedEffectDurationRandom(entity, stats);
	}
	else
	{
		messagePlayerColor(player, MESSAGE_STATUS, color, Language::get(773));
		// stop bleeding
		if ( stats->EFFECTS[EFF_BLEEDING] )
		{
			entity->setEffect(EFF_BLEEDING, false, 0, false);
		}
	}
	serverUpdateEffects(player);
	if ( shouldConsumeItem )
	{
		consumeItem(item, player);
		return true;
	}
	return false;
}

bool item_PotionExtraHealing(Item*& item, Entity* entity, Entity* usedBy, bool shouldConsumeItem)
{
	if (!entity)
	{
		return false;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return false;
	}

	if ( stats->amulet != nullptr )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION
			&& stats->type != SKELETON )
		{
			if ( player >= 0 && players[player]->isLocalPlayer() )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(750));
				playSoundPlayer(player, 90, 64);
			}
			return false;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player >= 0 && players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(751));
			playSoundPlayer(player, 90, 64);
		}
		return false;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item, player);
		return true;
	}
	if ( stats->HP == stats->MAXHP )
	{
		playSoundEntity(entity, 52, 64);
		playSoundEntity(entity, 168, 128);
		spawnMagicEffectParticles(entity->x, entity->y, entity->z, 169);
		if ( item->beatitude < 0 )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(2900));
			messagePlayer(player, MESSAGE_HINT, Language::get(2903));
			stats->EFFECTS[EFF_POISONED] = true;
			stats->EFFECTS_TIMERS[EFF_POISONED] = item->potionGetCursedEffectDurationRandom(entity, stats);
		}
		else
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(772));
			// stop bleeding
			if ( stats->EFFECTS[EFF_BLEEDING] )
			{
				entity->setEffect(EFF_BLEEDING, false, 0, false);
			}
		}
		serverUpdateEffects(player);
		if ( shouldConsumeItem )
		{
			consumeItem(item, player);
			return true;
		}
		return false;
	}

	int amount = item->potionGetEffectHealth(entity, stats);

	if ( stats->type == GOATMAN && entity->behavior == &actMonster )
	{
		amount *= GOATMAN_HEALINGPOTION_MOD; //Goatman special.
		stats->EFFECTS[EFF_FAST] = true;
		stats->EFFECTS_TIMERS[EFF_FAST] = GOATMAN_HEALING_POTION_SPEED_BOOST_DURATION;
	}

	//Bonus from CON, to scale up healing potions as the game progresses.
	if ( statGetCON(stats, entity) > 0 )
	{
		amount += 4 * statGetCON(stats, entity);
	}

	if ( item->beatitude < 0 )
	{
		amount /= (std::abs(item->beatitude) * 2);
	}

	int oldHP = entity->getHP();

	entity->modHP(amount);

	int heal = std::max(entity->getHP() - oldHP, 0);
	if ( heal > 0 )
	{
		serverUpdatePlayerGameplayStats(player, STATISTICS_HEAL_BOT, heal);
	}

	// play drink sound
	playSoundEntity(entity, 52, 64);
	playSoundEntity(entity, 168, 128);
	spawnMagicEffectParticles(entity->x, entity->y, entity->z, 169);
	Uint32 color = makeColorRGB(0, 255, 0);
	if ( item->beatitude < 0 )
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(2900));
		messagePlayer(player, MESSAGE_HINT, Language::get(2903));
		stats->EFFECTS[EFF_POISONED] = true;
		stats->EFFECTS_TIMERS[EFF_POISONED] = item->potionGetCursedEffectDurationRandom(entity, stats);
	}
	else
	{
		messagePlayerColor(player, MESSAGE_STATUS, color, Language::get(773));
		// stop bleeding
		if ( stats->EFFECTS[EFF_BLEEDING] )
		{
			entity->setEffect(EFF_BLEEDING, false, 0, false);
		}
	}
	serverUpdateEffects(player);
	if ( shouldConsumeItem )
	{
		consumeItem(item, player);
		return true;
	}
	return false;
}

bool item_PotionRestoreMagic(Item*& item, Entity* entity, Entity* usedBy)
{
	if (!entity)
	{
		return false;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return false;
	}

	if ( entity == NULL )
	{
		return false;
	}
	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION
			&& stats->type != SKELETON )
		{
			if ( player >= 0 && players[player]->isLocalPlayer() )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(750));
				playSoundPlayer(player, 90, 64);
			}
			return false;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player >= 0 && players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(751));
			playSoundPlayer(player, 90, 64);
		}
		return false;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item, player);
		return true;
	}
	if ( stats->MP == stats->MAXMP )
	{
		playSoundEntity(entity, 52, 64);
		if ( item->beatitude < 0 )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(774));
			messagePlayer(player, MESSAGE_HINT | MESSAGE_STATUS, Language::get(2902));
			stats->EFFECTS[EFF_SLOW] = true;
			stats->EFFECTS_TIMERS[EFF_SLOW] = item->potionGetCursedEffectDurationRandom(entity, stats);
		}
		else
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(772));
		}
		consumeItem(item, player);
		return true;
	}

	int amount = item->potionGetEffectHealth(entity, stats);

	if ( statGetINT(stats, entity) > 0 )
	{
		amount += std::min(30, 2 * statGetINT(stats, entity)); // extra mana scaling from 1 to 15 INT, capped at +30 MP
	}

	if ( item->beatitude < 0 )
	{
		amount /= (std::abs(item->beatitude) * 2);
		messagePlayer(player, MESSAGE_HINT, Language::get(774));
		messagePlayer(player, MESSAGE_HINT | MESSAGE_STATUS, Language::get(2902));
		stats->EFFECTS[EFF_SLOW] = true;
		stats->EFFECTS_TIMERS[EFF_SLOW] = item->potionGetCursedEffectDurationRandom(entity, stats);
	}
	else
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(774));
	}
	entity->modMP(amount);

	if ( svFlags & SV_FLAG_HUNGER )
	{
		if ( player >= 0 && stats->playerRace == RACE_INSECTOID && stats->stat_appearance == 0 )
		{
			Sint32 hungerPointPerMana = entity->playerInsectoidHungerValueOfManaPoint(*stats);
			stats->HUNGER += amount * hungerPointPerMana;
			stats->HUNGER = std::min(999, stats->HUNGER);
			if ( entity->behavior == &actPlayer && stats->type != SKELETON && stats->type != AUTOMATON )
			{
				updateHungerMessages(entity, stats, item);
			}
			else if ( player > 0 )
			{
				serverUpdateHunger(player);
			}
		}
	}

	// play drink sound
	playSoundEntity(entity, 52, 64);

	consumeItem(item, player);
	return true;
}

Entity* item_PotionPolymorph(Item*& item, Entity* entity, Entity* usedBy)
{
	if ( !entity )
	{
		return nullptr;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return nullptr;
	}

	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION
			&& stats->type != SKELETON )
		{
			if ( player >= 0 && players[player]->isLocalPlayer() )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(750));
				playSoundPlayer(player, 90, 64);
			}
			return nullptr;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player >= 0 && players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(751));
			playSoundPlayer(player, 90, 64);
		}
		return nullptr;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item, player);
		return nullptr;
	}

	// play drink sound
	if ( !usedBy ) // drinking rather than throwing.
	{
		playSoundEntity(entity, 52, 64);
		if ( player >= 0 )
		{
			messagePlayer(player, MESSAGE_STATUS | MESSAGE_INVENTORY, Language::get(3190));
		}
	}
	Entity* transformedEntity = nullptr;

	if ( entity->behavior == &actMonster || entity->behavior == &actPlayer )
	{
		transformedEntity = spellEffectPolymorph(entity, usedBy, false);
	}

	consumeItem(item, player);

	return transformedEntity;
}

void onScrollUseAppraisalIncrease(Item* item, int player)
{
	if ( !item ) { return; }
	if ( item->identified && players[player] && players[player]->isLocalPlayer() )
	{
		Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_CONSUMED, item->type, 1);
	}
	else if ( !item->identified && players[player] && players[player]->isLocalPlayer() )
	{
		Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_CONSUMED_UNIDENTIFIED, item->type, 1);
		if ( stats[player]->getProficiency(PRO_APPRAISAL) < SKILL_LEVEL_BASIC )
		{
			if ( stats[player] && players[player]->entity )
			{
				if ( local_rng.rand() % 4 == 0 )
				{
					if ( multiplayer == CLIENT )
					{
						// request level up
						strcpy((char*)net_packet->data, "CSKL");
						net_packet->data[4] = player;
						net_packet->data[5] = PRO_APPRAISAL;
						net_packet->address.host = net_server.host;
						net_packet->address.port = net_server.port;
						net_packet->len = 6;
						sendPacketSafe(net_sock, -1, net_packet, 0);
					}
					else
					{
						players[player]->entity->increaseSkill(PRO_APPRAISAL);
					}
				}
			}
		}
	}
}

void item_ScrollMail(Item* item, int player)
{
	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	if ( !players[player]->isLocalPlayer() )
	{
		return;
	}

	if (players[player]->entity->isBlind())
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(775));
		playSoundPlayer(player, 90, 64);
		return;
	}

	if ( players[player]->isLocalPlayer() )
	{
		conductIlliterate = false;
	}
	item->identified = true;

	int result = item->appearance % 25;
	switch ( item->appearance % 25 )
	{
		case 0:
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(776));
			break;
		case 1:
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(780));
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(781));
			break;
		case 2:
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(786));
			break;
		case 3:
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(790));
			break;
		case 4:
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(793), Language::get(158 + item->appearance % 26), Language::get(184 + item->appearance % 9));
			break;
		case 5:
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(796));
			break;
		case 6:
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(799));
			break;
		case 7:
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(801));
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(802));
			break;
		case 8:
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(807));
			break;
		case 9:
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(811));
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(812));
			break;
		case 10:
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(816));
			break;
		case 11:
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(817));
			break;
		case 12:
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(822));
			break;
		case 13:
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(824));
			break;
		case 14:
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(826));
			break;
		case 15:
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(828));
			break;
		case 16:
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(830));
			break;
		case 17:
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(834));
			break;
		case 18:
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(836));
			break;
		case 19:
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(838));
			break;
		case 20:
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(840));
			break;
		case 21:
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(843));
			break;
		default:
			messagePlayer(player, MESSAGE_WORLD | MESSAGE_INTERACTION, Language::get(846));
			result = 22;
			break;
	}

	result = result % NUM_SCROLL_MAIL_OPTIONS;
	Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_LORE_READ, SCROLL_MAIL, 1);
	Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_LORE_PERCENT_READ, SCROLL_MAIL, (1 << (result)));
}

void item_ScrollIdentify(Item* item, int player)
{
	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	if (!players[player]->isLocalPlayer())
	{
		consumeItem(item, player);
		return;
	}

	if (players[player]->entity->isBlind())
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(775));
		playSoundPlayer(player, 90, 64);
		return;
	}

	conductIlliterate = false;

	//if ( item->identified )
	//{
	//	// Uncurse an item
	//	// quickly check if we have any items available so we don't waste our scroll
	//	bool foundIdentifiable = false;
	//	for ( node_t* node = stats[player]->inventory.first; node != nullptr; node = node->next )
	//	{
	//		Item* inventoryItem = (Item*)node->element;
	//		if ( GenericGUI[player].isItemIdentifiable(inventoryItem) )
	//		{
	//			foundIdentifiable = true;
	//			break;
	//		}
	//	}
	//	if ( !foundIdentifiable )
	//	{
	//		messagePlayer(player, MESSAGE_HINT, Language::get(3996));
	//		return;
	//	}
	//}

	onScrollUseAppraisalIncrease(item, player);
	item->identified = true;
	GenericGUI[player].openGUI(GUI_TYPE_ITEMFX, item, item->beatitude, item->type, SPELL_NONE);
}

void item_ScrollLight(Item* item, int player)
{
	int c;

	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	if (multiplayer == CLIENT)
	{
		return;
	}

	if (players[player]->entity->isBlind())
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(775));
		playSoundPlayer(player, 90, 64);
		return;
	}

	if ( players[player]->isLocalPlayer() )
	{
		conductIlliterate = false;
	}
	messagePlayer(player, MESSAGE_INVENTORY, Language::get(848));
    
    const auto color = makeColorRGB(150, 150, 150);

	messagePlayer(player, MESSAGE_HINT, Language::get(851));
    
    const char name[] = "scroll_light";
	addLight(
        players[player]->entity->x / 16,
        players[player]->entity->y / 16,
        name);

	// send new light info to clients
	if (multiplayer == SERVER)
	{
		for (c = 1; c < MAXPLAYERS; c++)
		{
			if (client_disconnected[c] == true || players[c]->isLocalPlayer() )
			{
				continue;
			}
			strcpy((char*)net_packet->data, "ALIT");
			SDLNet_Write16(players[player]->entity->x / 16, &net_packet->data[4]);
			SDLNet_Write16(players[player]->entity->y / 16, &net_packet->data[6]);
			SDLNet_Write16(sizeof(name), &net_packet->data[8]);
            stringCopyUnsafe((char*)&net_packet->data[10], name, sizeof(name));
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 10 + sizeof(name);
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}
	onScrollUseAppraisalIncrease(item, player);
	item->identified = true;
}

void item_ScrollBlank(Item* item, int player)
{
	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	if ( !players[player]->isLocalPlayer() )
	{
		return;
	}

	if (players[player]->entity->isBlind())
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(775));
		playSoundPlayer(player, 90, 64);
		return;
	}

	if ( players[player]->isLocalPlayer() )
	{
		conductIlliterate = false;
	}
	messagePlayer(player, MESSAGE_HINT, Language::get(852));
	onScrollUseAppraisalIncrease(item, player);
	item->identified = true;
}

void item_ScrollEnchantWeapon(Item* item, int player)
{
	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	if ( !players[player]->isLocalPlayer() )
	{
		consumeItem(item, player);
		return;
	}

	if (players[player]->entity->isBlind())
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(775));
		playSoundPlayer(player, 90, 64);
		return;
	}

	conductIlliterate = false;

	if ( item->beatitude < 0 )
	{
		Item** toEnchant = nullptr;
		bool hasMeleeGloves = false;
		if ( stats[player]->gloves )
		{
			switch ( stats[player]->gloves->type )
			{
			case BRASS_KNUCKLES:
			case IRON_KNUCKLES:
			case SPIKED_GAUNTLETS:
				hasMeleeGloves = true;
				break;
			default:
				break;
			}
		}

		if ( stats[player]->weapon )
		{
			toEnchant = &stats[player]->weapon;
		}
		else if ( hasMeleeGloves )
		{
			toEnchant = &stats[player]->gloves;
		}
		messagePlayer(player, MESSAGE_INVENTORY, Language::get(848));
		if ( toEnchant == nullptr )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(853));
		}
		else 
		{
			if ( toEnchant == &stats[player]->gloves )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(858), (*toEnchant)->getName());
			}
			else
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(854));
			}

			if ( (*toEnchant)->beatitude > 0 )
			{
				(*toEnchant)->beatitude = -(*toEnchant)->beatitude;
				if ( stats[player]->type == SUCCUBUS )
				{
					steamAchievement("BARONY_ACH_THE_WAY_YOU_LIKE_IT");
				}
			}
			else
			{
				(*toEnchant)->beatitude -= 1;
			}

			if ( multiplayer == CLIENT )
			{
				strcpy((char*)net_packet->data, "BEAT");
				net_packet->data[4] = player;
				if ( toEnchant == &stats[player]->gloves )
				{
					net_packet->data[5] = 3; // glove index
				}
				else
				{
					net_packet->data[5] = 0; // weapon index
				}
				net_packet->data[6] = (*toEnchant)->beatitude + 100;
				SDLNet_Write16((Sint16)(*toEnchant)->type, &net_packet->data[7]);
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 9;
				sendPacketSafe(net_sock, -1, net_packet, 0);
				//messagePlayer(player, "sent server: %d, %d, %d", net_packet->data[4], net_packet->data[5], net_packet->data[6]);
			}
		}
		onScrollUseAppraisalIncrease(item, player);
		item->identified = true;
		consumeItem(item, player);
	}
	else
	{
		// Bless an item
		onScrollUseAppraisalIncrease(item, player);
		item->identified = true;
		GenericGUI[player].openGUI(GUI_TYPE_ITEMFX, item, item->beatitude, item->type, SPELL_NONE);
	}
}

void item_ScrollEnchantArmor(Item* item, int player)
{
	Item* armor = nullptr;
	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	if ( !item )
	{
		return;
	}

	if ( !players[player]->isLocalPlayer() )
	{
		consumeItem(item, player);
		return;
	}

	if (players[player]->entity->isBlind())
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(775));
		playSoundPlayer(player, 90, 64);
		return;
	}

	conductIlliterate = false;

	// choose a random piece of worn equipment to curse!
	int tryIndex = 1 + local_rng.rand() % 7;
	int startIndex = tryIndex;
	int armornum = 0;
	bool breakloop = false;

	// curse random item.
	while ( !armor && !breakloop )
	{
		switch ( tryIndex )
		{
			// intentional fall throughs...
			case 0:
				/*if ( stats[player]->weapon != nullptr && itemCategory(stats[player]->weapon) != POTION )
				{
					toCurse = stats[player]->weapon;
					armornum = 0;
					break;
				}*/
			case 1:
				if ( stats[player]->helmet != nullptr )
				{
					armor = stats[player]->helmet;
					armornum = 1;
					break;
				}
			case 2:
				if ( stats[player]->breastplate != nullptr )
				{
					armor = stats[player]->breastplate;
					armornum = 2;
					break;
				}
			case 3:
				if ( stats[player]->gloves != nullptr )
				{
					armor = stats[player]->gloves;
					armornum = 3;
					break;
				}
			case 4:
				if ( stats[player]->shoes != nullptr )
				{
					armor = stats[player]->shoes;
					armornum = 4;
					break;
				}
			case 5:
				if ( stats[player]->shield != nullptr )
				{
					armor = stats[player]->shield;
					armornum = 5;
					break;
				}
			case 6:
				if ( stats[player]->cloak != nullptr )
				{
					armor = stats[player]->cloak;
					armornum = 6;
					break;
				}
			case 7:
				if ( stats[player]->mask != nullptr )
				{
					armor = stats[player]->mask;
					armornum = 7;
					break;
				}
				++tryIndex;
				if ( tryIndex > 7 )
				{
					// loop back around.
					tryIndex = 0;
				}
				if ( tryIndex == startIndex )
				{
					// couldn't find a piece of armor, break.
					breakloop = true;
					armor = nullptr;
					break;
				}
			default:
				break;
		}
	}

	if (item->beatitude < 0)
	{
		messagePlayer(player, MESSAGE_INVENTORY, Language::get(848));
		if (armor == nullptr)
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(857));
		}
		else if ( armor != nullptr )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(858), armor->getName());

			if ( armor->beatitude > 0 )
			{
				armor->beatitude = -armor->beatitude;
			}
			else
			{
				armor->beatitude -= 1;
			}

			if ( multiplayer == CLIENT )
			{
				strcpy((char*)net_packet->data, "BEAT");
				net_packet->data[4] = player;
				net_packet->data[5] = armornum;
				net_packet->data[6] = armor->beatitude + 100;
				SDLNet_Write16((Sint16)armor->type, &net_packet->data[7]);
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 9;
				sendPacketSafe(net_sock, -1, net_packet, 0);
				//messagePlayer(player, "sent server: %d, %d, %d", net_packet->data[4], net_packet->data[5], net_packet->data[6]);
			}
		}
		onScrollUseAppraisalIncrease(item, player);
		item->identified = true;
		consumeItem(item, player);

	}
	else
	{
		// Bless an item
		onScrollUseAppraisalIncrease(item, player);
		item->identified = true;
		GenericGUI[player].openGUI(GUI_TYPE_ITEMFX, item, item->beatitude, item->type, SPELL_NONE);
	}
}

void item_ScrollRemoveCurse(Item* item, int player)
{
	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	if ( !players[player]->isLocalPlayer() )
	{
		consumeItem(item, player);
		return;
	}

	if (players[player]->entity->isBlind())
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(775));
		playSoundPlayer(player, 90, 64);
		return;
	}

	conductIlliterate = false;
	//if ( item->identified && item->beatitude >= 0 )
	//{
	//	// Uncurse an item
	//	// quickly check if we have any items available so we don't waste our scroll
	//	bool foundUncurseable = false;
	//	for ( node_t* node = stats[player]->inventory.first; node != nullptr; node = node->next )
	//	{
	//		Item* inventoryItem = (Item*)node->element;
	//		if ( GenericGUI[player].isItemRemoveCursable(inventoryItem) )
	//		{
	//			foundUncurseable = true;
	//			break;
	//		}
	//	}
	//	if ( !foundUncurseable )
	//	{
	//		messagePlayer(player, MESSAGE_HINT, Language::get(3995));
	//		return;
	//	}
	//}

	if ( item->beatitude < 0 )
	{
		messagePlayer(player, MESSAGE_INVENTORY, Language::get(848));
		// choose a random piece of worn equipment to curse!
		int tryIndex = local_rng.rand() % 8;
		int startIndex = tryIndex;
		int armornum = 0;
		bool breakloop = false;
		Item* toCurse = nullptr;
		// curse random item.
		while ( !toCurse && !breakloop )
		{
			switch ( tryIndex )
			{
				// intentional fall throughs...
				case 0:
					if ( stats[player]->weapon != nullptr && itemCategory(stats[player]->weapon) != POTION )
					{
						toCurse = stats[player]->weapon;
						armornum = 0;
						break;
					}
				case 1:
					if ( stats[player]->helmet != nullptr )
					{
						toCurse = stats[player]->helmet;
						armornum = 1;
						break;
					}
				case 2:
					if ( stats[player]->breastplate != nullptr )
					{
						toCurse = stats[player]->breastplate;
						armornum = 2;
						break;
					}
				case 3:
					if ( stats[player]->gloves != nullptr )
					{
						toCurse = stats[player]->gloves;
						armornum = 3;
						break;
					}
				case 4:
					if ( stats[player]->shoes != nullptr )
					{
						toCurse = stats[player]->shoes;
						armornum = 4;
						break;
					}
				case 5:
					if ( stats[player]->shield != nullptr )
					{
						toCurse = stats[player]->shield;
						armornum = 5;
						break;
					}
				case 6:
					if ( stats[player]->cloak != nullptr )
					{
						toCurse = stats[player]->cloak;
						armornum = 6;
						break;
					}
				case 7:
					if ( stats[player]->mask != nullptr )
					{
						toCurse = stats[player]->mask;
						armornum = 7;
						break;
					}
					++tryIndex;
					if ( tryIndex > 7 )
					{
						// loop back around.
						tryIndex = 0;
					}
					if ( tryIndex == startIndex )
					{
						// couldn't find a piece of armor, break.
						breakloop = true;
						toCurse = nullptr;
						break;
					}
				default:
					break;
			}
		}
		if ( toCurse )
		{
			if ( toCurse->beatitude <= 0 )
			{
				--toCurse->beatitude;
			}
			else
			{
				toCurse->beatitude = -toCurse->beatitude;
				if ( itemCategory(toCurse) == WEAPON && stats[player]->type == SUCCUBUS )
				{
					steamAchievement("BARONY_ACH_THE_WAY_YOU_LIKE_IT");
				}
			}
			messagePlayer(player, MESSAGE_HINT, Language::get(858), toCurse->getName());
			if ( multiplayer == CLIENT )
			{
				strcpy((char*)net_packet->data, "BEAT");
				net_packet->data[4] = player;
				net_packet->data[5] = armornum;
				net_packet->data[6] = toCurse->beatitude + 100;
				SDLNet_Write16((Sint16)toCurse->type, &net_packet->data[7]);
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 9;
				sendPacketSafe(net_sock, -1, net_packet, 0);
				//messagePlayer(player, "sent server: %d, %d, %d", net_packet->data[4], net_packet->data[5], net_packet->data[6]);
			}
		}
		else
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(862));
		}
		onScrollUseAppraisalIncrease(item, player);
		item->identified = true;
		consumeItem(item, player);
	}
	else
	{
		onScrollUseAppraisalIncrease(item, player);
		item->identified = true;
		GenericGUI[player].openGUI(GUI_TYPE_ITEMFX, item, item->beatitude, item->type, SPELL_NONE);
	}
}

bool item_ScrollFire(Item* item, int player)
{
	if (multiplayer == CLIENT)
	{
		return false;
	}

	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return false;
	}

	if (players[player]->entity->isBlind())
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(775));
		playSoundPlayer(player, 90, 64);
		return false;
	}

	if ( players[player]->isLocalPlayer() )
	{
		conductIlliterate = false;
	}

	if ( stats[player]->type != AUTOMATON )
	{
		serverUpdatePlayerGameplayStats(player, STATISTICS_FIRE_MAYBE_DIFFERENT, 1);
	}

	if (item->beatitude < 0)
	{
		onScrollUseAppraisalIncrease(item, player);
		item->identified = true;
		messagePlayer(player, MESSAGE_HINT | MESSAGE_INVENTORY, Language::get(863));
		return false;
	}
	else
	{
		playSoundEntity(players[player]->entity, 153, 128); // "FireballExplode.ogg"
		messagePlayer(player, MESSAGE_HINT | MESSAGE_STATUS, Language::get(864)); // "The scroll erupts in a tower of flame!"

		// Attempt to set the Player on fire
		players[player]->entity->SetEntityOnFire();

		int c;
		for (c = 0; c < 100; c++)
		{
			if ( Entity* entity = spawnFlame(players[player]->entity, SPRITE_FLAME) )
			{
				entity->sprite = 16;
				double vel = local_rng.rand() % 10;
				entity->vel_x = vel * cos(entity->yaw) * cos(entity->pitch) * .1;
				entity->vel_y = vel * sin(entity->yaw) * cos(entity->pitch) * .1;
				entity->vel_z = vel * sin(entity->pitch) * .2;
				entity->skill[0] = 5 + local_rng.rand() % 10;
			}
		}

		onScrollUseAppraisalIncrease(item, player);
		item->identified = true;
		return true;
	}
	return false;
}

void item_ScrollFood(Item* item, int player)
{
	Item* target;
	node_t* node, *nextnode;
	int foundfood = 0;

	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	// this is a CLIENT function
	if ( !players[player]->isLocalPlayer() )
	{
		return;
	}

	if (players[player]->entity->isBlind())
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(775));
		playSoundPlayer(player, 90, 64);
		return;
	}

	if ( players[player]->isLocalPlayer() )
	{
		conductIlliterate = false;
	}

	messagePlayer(player, MESSAGE_INVENTORY, Language::get(848));
	if ( item->beatitude >= 0 )
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(865));
		dropItem(newItem(FOOD_FISH, EXCELLENT, item->beatitude, 1, local_rng.rand(), true, &stats[player]->inventory), player, false);
		dropItem(newItem(FOOD_BREAD, EXCELLENT, item->beatitude, 1, local_rng.rand(), true, &stats[player]->inventory), player, false);
		dropItem(newItem(FOOD_APPLE, EXCELLENT, item->beatitude, 1, local_rng.rand(), true, &stats[player]->inventory), player, false);
		dropItem(newItem(FOOD_CHEESE, EXCELLENT, item->beatitude, 1, local_rng.rand(), true, &stats[player]->inventory), player, false);
		dropItem(newItem(FOOD_MEAT, EXCELLENT, item->beatitude, 1, local_rng.rand(), true, &stats[player]->inventory), player, false);
		onScrollUseAppraisalIncrease(item, player);
		item->identified = true;
		return;
	}
	else
	{
		for ( node = stats[player]->inventory.first; node != NULL; node = nextnode )
		{
			nextnode = node->next;
			target = (Item*)node->element;
			if ( itemCategory(target) == FOOD )
			{
				if ( local_rng.rand() % 2 == 0 )   // 50% chance of destroying that food item
				{
					consumeItem(target, player);
				}
				foundfood = 1;
			}
		}
	}
	if ( foundfood == 0 )
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(866));
	}
	else
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(867));
	}
	onScrollUseAppraisalIncrease(item, player);
	item->identified = true;
}

void item_ScrollConjureArrow(Item* item, int player)
{
	if ( players[player] == nullptr || players[player]->entity == nullptr )
	{
		return;
	}

	// this is a CLIENT function
	if ( !players[player]->isLocalPlayer() )
	{
		return;
	}

	if ( players[player]->entity->isBlind() )
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(775));
		playSoundPlayer(player, 90, 64);
		return;
	}

	if ( players[player]->isLocalPlayer() )
	{
		conductIlliterate = false;
	}

	messagePlayer(player, MESSAGE_INVENTORY, Language::get(848));
	messagePlayer(player, MESSAGE_HINT, Language::get(3762));
	ItemType type = static_cast<ItemType>(QUIVER_SILVER + local_rng.rand() % 7);

	int amount = 20 + local_rng.rand() % 6;
	if ( item->beatitude < 0 )
	{
		amount -= (15  + local_rng.rand() % 6);
	}
	else if ( item->beatitude > 0 )
	{
		amount += 20 + local_rng.rand() % 6;
	}
	dropItem(newItem(type, SERVICABLE, item->beatitude, amount, ITEM_GENERATED_QUIVER_APPEARANCE, false, &stats[player]->inventory), player, false);
	if ( item->beatitude >= 2 )
	{
		// re-roll!
		type = static_cast<ItemType>(QUIVER_SILVER + local_rng.rand() % 7);
		amount = 40 + local_rng.rand() % 11;
		dropItem(newItem(type, SERVICABLE, item->beatitude, amount, ITEM_GENERATED_QUIVER_APPEARANCE, false, &stats[player]->inventory), player, false);
	}

	onScrollUseAppraisalIncrease(item, player);
	item->identified = true;
}

void item_ScrollMagicMapping(Item* item, int player)
{
	int x, y;

	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	// this is a CLIENT function
	if (multiplayer == SERVER && player > 0)
	{
		return;
	}

	if (players[player]->entity->isBlind())
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(775));
		playSoundPlayer(player, 90, 64);
		return;
	}

	if ( players[player]->isLocalPlayer() )
	{
		conductIlliterate = false;
	}
	
	messagePlayer(player, MESSAGE_INVENTORY, Language::get(848));
	if ( item->beatitude >= 0 )
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(868));
		mapLevel(player);
	}
	else
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(869));
		for ( y = 0; y < map.height; y++ )
		{
			for ( x = 0; x < map.width; x++ )
			{
				minimap[y][x] = 0;
			}
		}
	}
	onScrollUseAppraisalIncrease(item, player);
	item->identified = true;
}

void item_ScrollRepair(Item* item, int player)
{
	Item* armor = nullptr;
	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	// client function only
	if ( !players[player]->isLocalPlayer() )
	{
		consumeItem(item, player);
		return;
	}

	if (players[player]->entity->isBlind())
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(775));
		playSoundPlayer(player, 90, 64);
		return;
	}

	conductIlliterate = false;
	//if ( item->identified && item->beatitude >= 0 )
	//{
	//	// quickly check if we have any items available so we don't waste our scroll
	//	bool foundRepairableItem = false;
	//	for ( node_t* node = stats[player]->inventory.first; node != nullptr; node = node->next )
	//	{
	//		Item* inventoryItem = (Item*)node->element;
	//		if ( GenericGUI[player].isItemRepairable(inventoryItem, item->type) )
	//		{
	//			foundRepairableItem = true;
	//			break;
	//		}
	//	}
	//	if ( !foundRepairableItem )
	//	{
	//		if ( item->type == SCROLL_REPAIR )
	//		{
	//			messagePlayer(player, MESSAGE_HINT, Language::get(3288));
	//		}
	//		else if ( item->type == SCROLL_CHARGING )
	//		{
	//			messagePlayer(player, MESSAGE_HINT, Language::get(3731));
	//		}
	//		return;
	//	}
	//}

	if ( item->beatitude < 0 )
	{
		messagePlayer(player, MESSAGE_INVENTORY, Language::get(848));
		int tryIndex = local_rng.rand() % 7;
		int startIndex = tryIndex;
		int armornum = 0;
		bool breakloop = false;
		// degrade random equipped item.
		while ( !armor && !breakloop )
		{
			switch ( tryIndex )
			{
				// intentional fall throughs...
				case 0:
					if ( stats[player]->weapon != nullptr && itemCategory(stats[player]->weapon) != POTION )
					{
						armor = stats[player]->weapon;
						armornum = 0;
						break;
					}
				case 1:
					if ( stats[player]->helmet != nullptr )
					{
						armor = stats[player]->helmet;
						armornum = 1;
						break;
					}
				case 2:
					if ( stats[player]->breastplate != nullptr )
					{
						armor = stats[player]->breastplate;
						armornum = 2;
						break;
					}
				case 3:
					if ( stats[player]->gloves != nullptr )
					{
						armor = stats[player]->gloves;
						armornum = 3;
						break;
					}
				case 4:
					if ( stats[player]->shoes != nullptr )
					{
						armor = stats[player]->shoes;
						armornum = 4;
						break;
					}
				case 5:
					if ( stats[player]->shield != nullptr )
					{
						armor = stats[player]->shield;
						armornum = 5;
						break;
					}
				case 6:
					if ( stats[player]->cloak != nullptr )
					{
						armor = stats[player]->cloak;
						armornum = 6;
						break;
					}
					++tryIndex;
					if ( tryIndex > 6 )
					{
						// loop back around.
						tryIndex = 0;
					}
					if ( tryIndex == startIndex )
					{
						// couldn't find a piece of armor, break.
						breakloop = true;
						armor = nullptr;
						break;
					}
				default:
					break;
			}
		}
		if ( armor == nullptr )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(870)); // you feel a tingling sensation
		}
		else if ( armor != nullptr )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(871), armor->getName());
			if ( item->type == SCROLL_CHARGING )
			{
				armor->status = BROKEN;
			}
			else
			{
				armor->status = static_cast<Status>(std::max(armor->status - 2, static_cast<int>(BROKEN)));
			}
			if ( multiplayer == CLIENT )
			{
				strcpy((char*)net_packet->data, "REPA");
				net_packet->data[4] = player;
				net_packet->data[5] = armornum;
				net_packet->data[6] = armor->status;
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 7;
				sendPacketSafe(net_sock, -1, net_packet, 0);

				//messagePlayer(player, "sent server: %d, %d, %d", net_packet->data[4], net_packet->data[5], net_packet->data[6]);
			}
			if ( armor->status > BROKEN )
			{
				if ( armor->type == TOOL_CRYSTALSHARD )
				{
					messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(2350), armor->getName());
				}
				else
				{
					messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(681), armor->getName());
				}
			}
			else
			{
				if ( armor->type == TOOL_CRYSTALSHARD )
				{
					playSoundPlayer(player, 162, 64);
					messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(2351), armor->getName());
				}
				else if ( itemCategory(armor) == SPELLBOOK )
				{
					playSoundPlayer(player, 414, 64);
					messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(3459), armor->getName());
				}
				else
				{
					playSoundPlayer(player, 76, 64);
					messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(682), armor->getName());
				}
			}
		}
		onScrollUseAppraisalIncrease(item, player);
		item->identified = true;
		consumeItem(item, player);
	}
	else
	{
		// Repair an item
		onScrollUseAppraisalIncrease(item, player);
		item->identified = true;
		GenericGUI[player].openGUI(GUI_TYPE_ITEMFX, item, item->beatitude, item->type, SPELL_NONE);
	}
}

void item_ScrollDestroyArmor(Item* item, int player)
{
	Item* armor = nullptr;
	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	// client function only
	if ( !players[player]->isLocalPlayer() )
	{
		consumeItem(item, player);
		return;
	}

	if (players[player]->entity->isBlind())
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(775));
		playSoundPlayer(player, 90, 64);
		return;
	}

	conductIlliterate = false;
	
	messagePlayer(player, MESSAGE_INVENTORY, Language::get(848));

	int armornum = 0;
	int tryIndex = 1 + local_rng.rand() % 7;
	int startIndex = tryIndex;
	bool breakloop = false;
	while ( !armor && !breakloop )
	{
		switch ( tryIndex )
		{
			// intentional fall throughs...
			case 0:
				// shouldn't occur
				/*if ( stats[player]->weapon != nullptr )
				{
					armor = stats[player]->weapon;
					armornum = 0;
					break;
				}*/
			case 1:
				if ( stats[player]->helmet != nullptr )
				{
					armor = stats[player]->helmet;
					armornum = 1;
					break;
				}
			case 2:
				if ( stats[player]->breastplate != nullptr )
				{
					armor = stats[player]->breastplate;
					armornum = 2;
					break;
				}
			case 3:
				if ( stats[player]->gloves != nullptr )
				{
					armor = stats[player]->gloves;
					armornum = 3;
					break;
				}
			case 4:
				if ( stats[player]->shoes != nullptr )
				{
					armor = stats[player]->shoes;
					armornum = 4;
					break;
				}
			case 5:
				if ( stats[player]->shield != nullptr )
				{
					armor = stats[player]->shield;
					armornum = 5;
					break;
				}
			case 6:
				if ( stats[player]->cloak != nullptr )
				{
					armor = stats[player]->cloak;
					armornum = 6;
					break;
				}
			case 7:
				if ( stats[player]->mask != nullptr )
				{
					armor = stats[player]->mask;
					armornum = 7;
					break;
				}
				++tryIndex;
				if ( tryIndex > 7 )
				{
					// loop back around.
					tryIndex = 1;
				}
				if ( tryIndex == startIndex )
				{
					// couldn't find a piece of armor, break.
					breakloop = true;
					armor = nullptr;
					break;
				}
			default:
				break;
		}
	}

	if ( armor == nullptr )
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(873));
	}
	else if ( armor != nullptr )
	{
		if ( item->beatitude < 0 )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(874), armor->getName());
		}
		else
		{
			messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(875), armor->getName());

			if ( armor->status > BROKEN )
			{
				if ( player >= 0 )
				{
					Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_BROKEN, armor->type, 1);
				}
			}
			armor->status = static_cast<Status>(0);

			if ( multiplayer == CLIENT )
			{
				strcpy((char*)net_packet->data, "REPA");
				net_packet->data[4] = player;
				net_packet->data[5] = armornum;
				net_packet->data[6] = armor->status;
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 7;
				sendPacketSafe(net_sock, -1, net_packet, 0);

				//messagePlayer(player, "sent server: %d, %d, %d", net_packet->data[4], net_packet->data[5], net_packet->data[6]);
			}

			if ( armor->status == BROKEN )
			{
				if ( armor->type == TOOL_CRYSTALSHARD )
				{
					playSoundPlayer(player, 162, 64);
					messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(2351), armor->getName());
				}
				else
				{
					playSoundPlayer(player, 76, 64);
					messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(682), armor->getName());
				}
			}
		}
	}

	onScrollUseAppraisalIncrease(item, player);
	item->identified = true;
	consumeItem(item, player);
}

void item_ScrollTeleportation(Item* item, int player)
{
	// server only function
	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}
	if (multiplayer == CLIENT)
	{
		return;
	}

	if (players[player]->entity->isBlind())
	{
		if ( players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(775));
			playSoundPlayer(player, 90, 64);
		}
		return;
	}

	if ( players[player]->isLocalPlayer() )
	{
		conductIlliterate = false;
	}

	messagePlayer(player, MESSAGE_INVENTORY, Language::get(848));
	if (item->beatitude < 0 && local_rng.rand() % 2)
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(876));
		onScrollUseAppraisalIncrease(item, player);
		item->identified = true;
		return;
	}

	players[player]->entity->teleportRandom();
	onScrollUseAppraisalIncrease(item, player);
	item->identified = true;
}

void item_ScrollSummon(Item* item, int player)
{
	// server only function
	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}
	if (multiplayer == CLIENT)
	{
		return;
	}

	if ( players[player]->entity->isBlind() )
	{
		if ( players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(775));
			playSoundPlayer(player, 90, 64);
		}
		return;
	}

	if ( players[player]->isLocalPlayer() )
	{
		conductIlliterate = false;
	}

	messagePlayer(player, MESSAGE_INVENTORY, Language::get(848));

	playSoundEntity(players[player]->entity, 153, 64);
	Uint32 numCreatures = 1;
	Monster creature = RAT;

	if (item->beatitude <= -2)
	{
		// spawn something really nasty
		numCreatures = 1;
		switch (local_rng.rand() % 4)
		{
			case 0:
				creature = MINOTAUR;
				break;
			case 1:
				creature = DEMON;
				break;
			case 2:
				creature = CREATURE_IMP;
				break;
			case 3:
				creature = TROLL;
				break;
		}
	}
	else if (item->beatitude == -1)
	{
		// spawn moderately nasty things
		switch (local_rng.rand() % 6)
		{
			case 0:
				creature = GNOME;
				numCreatures = local_rng.rand() % 3 + 1;
				break;
			case 1:
				creature = SPIDER;
				numCreatures = local_rng.rand() % 2 + 1;
				break;
			case 2:
				creature = SUCCUBUS;
				numCreatures = local_rng.rand() % 2 + 1;
				break;
			case 3:
				creature = SCORPION;
				numCreatures = local_rng.rand() % 2 + 1;
				break;
			case 4:
				creature = GHOUL;
				numCreatures = local_rng.rand() % 2 + 1;
				break;
			case 5:
				creature = GOBLIN;
				numCreatures = local_rng.rand() % 2 + 1;
				break;
		}
	}
	else if (item->beatitude == 0)
	{
		// spawn weak monster ally
		switch (local_rng.rand() % 3)
		{
			case 0:
				creature = RAT;
				numCreatures = local_rng.rand() % 3 + 1;
				break;
			case 1:
				creature = GHOUL;
				numCreatures = 1;
				break;
			case 2:
				creature = SLIME;
				numCreatures = local_rng.rand() % 2 + 1;
				break;
		}
	}
	else if (item->beatitude == 1)
	{
		// spawn humans
		creature = HUMAN;
		numCreatures = local_rng.rand() % 3 + 1;
	}
	else if (item->beatitude >= 2)
	{
		//Spawn many/neat allies
		switch (local_rng.rand() % 2)
		{
			case 0:
				// summon zap brigadiers
				numCreatures = local_rng.rand() % 2 + 4;
				creature = HUMAN;
				break;
			case 1:
				// summon demons
				numCreatures = local_rng.rand() % 2 + 4;
				creature = DEMON;
				break;
		}
	}

	int i;
	bool spawnedMonster = false;
	for (i = 0; i < numCreatures; ++i)
	{
		Entity* monster = summonMonster(creature, floor(players[player]->entity->x / 16) * 16 + 8, floor(players[player]->entity->y / 16) * 16 + 8);
		if ( monster )
		{
			spawnedMonster = true;
			if ( item->beatitude >= 2 && creature == HUMAN )
			{
				monster->skill[29] = 1; // zap brigadier
			}
			Stat* monsterStats = monster->getStats();
			if ( item->beatitude >= 0 && monsterStats )
			{
				if ( forceFollower(*players[player]->entity, *monster) )
				{
					monster->monsterAllyIndex = player;
					if ( multiplayer == SERVER )
					{
						serverUpdateEntitySkill(monster, 42); // update monsterAllyIndex for clients.
					}

					// change the color of the hit entity.
					Compendium_t::Events_t::eventUpdateMonster(player, Compendium_t::CPDM_RECRUITED, monster, 1);
					monster->flags[USERFLAG2] = true;
					serverUpdateEntityFlag(monster, USERFLAG2);
					if ( monsterChangesColorWhenAlly(monsterStats) )
					{
						int bodypart = 0;
						for ( node_t* node = monster->children.first; node != nullptr; node = node->next )
						{
							if ( bodypart >= LIMB_HUMANOID_TORSO )
							{
								Entity* tmp = (Entity*)node->element;
								if ( tmp )
								{
									tmp->flags[USERFLAG2] = true;
									//serverUpdateEntityFlag(tmp, USERFLAG2);
								}
							}
							++bodypart;
						}
					}
				}
			}
		}
	}
	if ( spawnedMonster )
	{
		if ( item->beatitude < 0 )
		{
			if ( numCreatures <= 1 )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(877), getMonsterLocalizedName((Monster)creature).c_str());
			}
			else
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(878), getMonsterLocalizedPlural((Monster)creature).c_str());
			}
		}
		else
		{
			if ( numCreatures <= 1 )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(879), getMonsterLocalizedName((Monster)creature).c_str());
				if ( item->beatitude >= 2 )
				{
					messagePlayer(player, MESSAGE_WORLD, Language::get(880));
				}
			}
			else
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(881), getMonsterLocalizedPlural((Monster)creature).c_str());
				if ( item->beatitude >= 2 )
				{
					messagePlayer(player, MESSAGE_WORLD, Language::get(882));
				}
			}
		}
	}

	onScrollUseAppraisalIncrease(item, player);
	item->identified = true;
}

void item_ToolTowel(Item*& item, int player)
{
	if ( !item )
	{
		return;
	}
	if ( players[player]->isLocalPlayer() )
	{
		messagePlayer(player, MESSAGE_STATUS, Language::get(883));
	}
	if ( multiplayer != CLIENT )
	{
		if ( stats[player]->EFFECTS[EFF_GREASY]
			|| stats[player]->EFFECTS[EFF_MESSY]
			|| stats[player]->EFFECTS[EFF_BLEEDING] )
		{
			steamAchievementClient(player, "BARONY_ACH_BRING_A_TOWEL");
			if ( stats[player]->EFFECTS[EFF_GREASY] )
			{
				Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_TOWEL_GREASY, item->type, 1);
			}
			else if ( stats[player]->EFFECTS[EFF_MESSY] )
			{
				Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_TOWEL_MESSY, item->type, 1);
			}
			else if ( stats[player]->EFFECTS[EFF_BLEEDING] )
			{
				Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_TOWEL_BLEEDING, item->type, 1);
			}
			Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_TOWEL_USES, item->type, 1);
		}
		stats[player]->EFFECTS[EFF_GREASY] = false;
		stats[player]->EFFECTS[EFF_MESSY] = false;
	}

	// stop bleeding
	if ( stats[player]->EFFECTS[EFF_BLEEDING] )
	{
		if ( players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_STATUS, Language::get(884));
			messagePlayer(player, MESSAGE_STATUS, Language::get(885));
		}
		if ( multiplayer != CLIENT )
		{
			stats[player]->EFFECTS[EFF_BLEEDING] = false;
			stats[player]->EFFECTS_TIMERS[EFF_BLEEDING] = 0;
		}
		consumeItem(item, player);
	}

	if ( multiplayer != CLIENT )
	{
		serverUpdateEffects(player);
	}
}

void item_ToolTinOpener(Item* item, int player)
{
	if (multiplayer == CLIENT)
	{
		return;
	}
	if ( !players[player]->isLocalPlayer() )
	{
		consumeItem(item, player);
	}
	messagePlayer(player, MESSAGE_HINT, Language::get(886));
}

void item_ToolMirror(Item*& item, int player)
{
	if ( !item )
	{
		return;
	}
	Sint16 beatitude = item->beatitude;
	if ( !players[player]->isLocalPlayer() )
	{
		consumeItem(item, player);
		item = nullptr;
	}

	if (players[player] == nullptr || players[player]->entity == nullptr || stats[player] == nullptr )
	{
		return;
	}

	if ( players[player]->isLocalPlayer() )
	{
		messagePlayer(player, MESSAGE_INTERACTION, Language::get(889));
	}

	bool broken = false;

	// server/local side
	if ( multiplayer != CLIENT )
	{
		if ( stats[player]->EFFECTS[EFF_GREASY] )
		{
			messagePlayer(player, MESSAGE_WORLD, Language::get(887));
			messagePlayer(player, MESSAGE_INVENTORY, Language::get(888));
			playSoundEntity(players[player]->entity, 162, 64); // whoops, *break*
			broken = true;
		}
		else if ( stats[player]->EFFECTS[EFF_BLIND] )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(892));
		}
		else if ( players[player]->entity->isInvisible() || (stats[player]->type == VAMPIRE) )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(893));
		}
		else if ( beatitude > 0 )
		{
			if ( players[player]->entity->teleportRandom() )
			{
				Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_MIRROR_TELEPORTS, TOOL_MIRROR, 1);
			}
			messagePlayer(player, MESSAGE_HINT, Language::get(890));
		}
		else if ( beatitude < 0 )
		{
			messagePlayerColor(player, MESSAGE_HINT, makeColorRGB(255, 0, 0), Language::get(891)); // you look like a monster *break*
			playSoundEntity(players[player]->entity, 162, 64);
			broken = true;
			if ( players[player]->entity->setEffect(EFF_BLEEDING, true, TICKS_PER_SECOND * 15, true) )
			{
				messagePlayerColor(player, MESSAGE_STATUS, makeColorRGB(255, 0, 0), Language::get(701)); // you're bleeding!
			}
		}
		else if ( stats[player]->type == AUTOMATON )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(3698));
		}
		else if ( stats[player]->EFFECTS[EFF_DRUNK] )
		{
			if ( stats[player]->sex == MALE )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(894));
			}
			else
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(895));
			}
		}
		else if ( stats[player]->EFFECTS[EFF_CONFUSED] )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(896));
		}
		else if ( stats[player]->EFFECTS[EFF_POISONED] )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(897));
		}
		else if ( stats[player]->EFFECTS[EFF_VOMITING] )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(898));
		}
		else if ( stats[player]->EFFECTS[EFF_MESSY] )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(899));
		}
		else if ( stats[player]->HUNGER < 200 )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(900));
		}
		else if ( stats[player]->HUNGER < 500 )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(901));
		}
		else if ( stats[player]->CHR < 2 )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(902));
		}
		else
		{
			if ( stats[player]->sex == MALE )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(903));
			}
			else
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(904));
			}
		}
	}

	if ( players[player]->isLocalPlayer() && item )
	{
		if ( stats[player]->EFFECTS[EFF_GREASY] )
		{
			broken = true;
		}
		else if ( stats[player]->EFFECTS[EFF_BLIND] )
		{
		}
		else if ( players[player]->entity->isInvisible() || (stats[player]->type == VAMPIRE) )
		{
		}
		else if ( beatitude > 0 )
		{
			if ( local_rng.rand() % 4 == 0 )
			{
				if ( item->status > DECREPIT )
				{
					item->status = static_cast<Status>((int)item->status - 1);
					messagePlayer(player, MESSAGE_HINT, Language::get(681), item->getName());
				}
				else if ( item->status == DECREPIT )
				{
					item->status = BROKEN;
					messagePlayer(player, MESSAGE_HINT, Language::get(4344), item->getName());

					if ( multiplayer == CLIENT )
					{
						strcpy((char*)net_packet->data, "MIRR");
						net_packet->data[4] = player;
						net_packet->address.host = net_server.host;
						net_packet->address.port = net_server.port;
						net_packet->len = 5;
						sendPacketSafe(net_sock, -1, net_packet, 0);

						playSoundPlayer(player, 162, 64);
					}
					else
					{
						if ( players[player]->entity->setEffect(EFF_BLEEDING, true, TICKS_PER_SECOND * 15, true) )
						{
							messagePlayerColor(player, MESSAGE_STATUS,
								makeColorRGB(255, 0, 0), Language::get(701)); // you're bleeding!
						}
						playSoundEntity(players[player]->entity, 162, 64);
					}

					broken = true;
				}
			}
		}
		else if ( beatitude < 0 )
		{
			broken = true;
		}

		if ( broken )
		{
			consumeItem(item, player);
			item = nullptr;
			Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_BROKEN, TOOL_MIRROR, 1);
		}
	}
}

Entity* item_ToolBeartrap(Item*& item, Entity* usedBy)
{
	if ( !usedBy )
	{
		return nullptr;
	}

	int player = -1;
	if ( usedBy->behavior == &actMonster ) // monster
	{
		int u, v;
		int x = std::min(std::max<unsigned int>(1, usedBy->x / 16), map.width - 2);
		int y = std::min(std::max<unsigned int>(1, usedBy->y / 16), map.height - 2);
		for ( u = x - 1; u <= x + 1; u++ )
		{
			for ( v = y - 1; v <= y + 1; v++ )
			{
				if ( entityInsideTile(usedBy, u, v, 0) )   // no floor
				{
					return nullptr;
				}
			}
		}
		playSoundEntity(usedBy, 253, 64);
		Entity* entity = newEntity(668, 1, map.entities, nullptr); //Beartrap entity.
		entity->behavior = &actBeartrap;
		entity->flags[PASSABLE] = true;
		entity->flags[UPDATENEEDED] = true;
		entity->x = usedBy->x + 16.0 * cos(usedBy->yaw);
		entity->y = usedBy->y + 16.0 * sin(usedBy->yaw);
		entity->z = 6.75;
		entity->yaw = usedBy->yaw;
		entity->roll = -PI / 2; // flip the model
		entity->parent = usedBy->getUID();
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[11] = item->status;
		entity->skill[12] = item->beatitude;
		entity->skill[14] = item->appearance;
		entity->skill[15] = item->identified;
		entity->skill[17] = -1;
		consumeItem(item, player);

		auto& trapProps = monsterTrapIgnoreEntities[entity->getUID()];
		trapProps.parent = entity->parent;
		for ( node_t* node = map.creatures->first; node != nullptr; node = node->next )
		{
			Entity* creature = (Entity*)node->element;
			if ( creature && usedBy->checkFriend(creature) )
			{
				trapProps.ignoreEntities.insert(creature->getUID());
			}
		}
		return entity;
	}
	else if ( usedBy->behavior == &actPlayer )
	{
		player = usedBy->skill[2];
	}

	if ( player < 0 || player >= MAXPLAYERS )
	{
		return nullptr;
	}

	if ( multiplayer == CLIENT )
	{
		consumeItem(item, player);
		return nullptr;
	}
	bool failed = false;
	switch ( item->status )
	{
		case SERVICABLE:
			if ( local_rng.rand() % 25 == 0 )
			{
				failed = true;
			}
			break;
		case WORN:
			if ( local_rng.rand() % 10 == 0 )
			{
				failed = true;
			}
			break;
		case DECREPIT:
			if ( local_rng.rand() % 4 == 0 )
			{
				failed = true;
			}
			break;
		default:
			break;
	}
	if (item->beatitude < 0 || failed)
	{
		if ( players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_EQUIPMENT, Language::get(905));
		}
		if (multiplayer != CLIENT && players[player] )
		{
			playSoundEntity(players[player]->entity, 76, 64);
		}
		consumeItem(item, player);
		return nullptr;
	}
	if ( multiplayer != CLIENT && players[player] )
	{
		playSoundEntity(players[player]->entity, 253, 64);
	}

	if ( players[player] == nullptr || players[player]->entity == nullptr )
	{
		consumeItem(item, player);
		return nullptr;
	}

	Entity* entity = newEntity(668, 1, map.entities, nullptr); //Beartrap entity.
	entity->behavior = &actBeartrap;
	entity->flags[PASSABLE] = true;
	entity->flags[UPDATENEEDED] = true;
	real_t dist = 16.0;
	real_t checkx = players[player]->entity->x + dist * cos(players[player]->entity->yaw);
	real_t checky = players[player]->entity->y + dist * sin(players[player]->entity->yaw);
	int index = (static_cast<int>(checky) >> 4) * MAPLAYERS + (static_cast<int>(checkx) >> 4) * MAPLAYERS * map.height;
	while ( !map.tiles[index] || map.tiles[OBSTACLELAYER + index] || swimmingtiles[map.tiles[index]] || lavatiles[map.tiles[index]] )
	{
		dist -= 4.001;
		if ( dist < 0.0 )
		{
			checkx = players[player]->entity->x;
			checky = players[player]->entity->y;
			break;
		}
		checkx = players[player]->entity->x + dist * cos(players[player]->entity->yaw);
		checky = players[player]->entity->y + dist * sin(players[player]->entity->yaw);
		index = (static_cast<int>(checky) >> 4) * MAPLAYERS + (static_cast<int>(checkx) >> 4) * MAPLAYERS * map.height;
	}
	entity->x = checkx;
	entity->y = checky;
	entity->z = 6.75;
	entity->yaw = players[player]->entity->yaw;
	entity->roll = -PI / 2; // flip the model
	entity->parent = players[player]->entity->getUID();
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[11] = item->status;
	entity->skill[12] = item->beatitude;
	entity->skill[14] = item->appearance;
	entity->skill[15] = item->identified;
	entity->skill[17] = players[player]->entity->skill[2];
	messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(906));
	consumeItem(item, player);

	if ( player >= 0 )
	{
		Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_BEARTRAP_DEPLOYED, TOOL_BEARTRAP, 1);
	}
	return entity;
}

void item_Food(Item*& item, int player)
{
	if ( !item )
	{
		return;
	}
	int oldcount;
	int pukeChance;

	if ( player < 0 || player >= MAXPLAYERS || !stats[player] )
	{
		return;
	}

	if ( player >= 0 && stats[player]->type == AUTOMATON )
	{
		if ( players[player] && players[player]->entity )
		{
			item_FoodAutomaton(item, player);
		}
		return;
	}

	if ( player >= 0 && stats[player]->type != HUMAN && (svFlags & SV_FLAG_HUNGER) ) // hunger on
	{
		if ( stats[player]->type == SKELETON )
		{
			if ( players[player]->isLocalPlayer() )
			{
				steamAchievement("BARONY_ACH_BONEHEADED");
				dropItem(item, player); // client drop item
				messagePlayer(player, MESSAGE_HINT, Language::get(3179));
				playSoundPlayer(player, 90, 64);
			}
			return;
		}
	}

	if ( stats[player]->amulet != NULL )
	{
		if ( stats[player]->amulet->type == AMULET_STRANGULATION
			&& stats[player]->type != SKELETON )
		{
			if ( players[player]->isLocalPlayer() )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(756));
				playSoundPlayer(player, 90, 64);
			}
			return;
		}
	}

	// can't eat while vomiting
	if ( stats[player]->EFFECTS[EFF_VOMITING] )
	{
		if ( players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(757));
			playSoundPlayer(player, 90, 64);
		}
		return;
	}

	if ( players[player]->isLocalPlayer() )
	{
		conductFoodless = false;
		if ( item->type == FOOD_MEAT || item->type == FOOD_FISH || item->type == FOOD_TOMALLEY || item->type == FOOD_BLOOD )
		{
			conductVegetarian = false;
		}
		if ( stats[player]->playerRace == RACE_SKELETON && stats[player]->stat_appearance == 0
			&& players[player] && players[player]->entity->effectPolymorph > NUMMONSTERS )
		{
			steamAchievement("BARONY_ACH_MUSCLE_MEMORY");
		}
		Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_CONSUMED, item->type, 1);
	}

	if ( multiplayer == CLIENT )
	{
		consumeItem(item, player);
		return;
	}

	// consumption message
	oldcount = item->count;
	item->count = 1;
	messagePlayer(player, MESSAGE_STATUS, Language::get(907), item->description());
	item->count = oldcount;

	// eating sound
	if ( item->type == FOOD_BLOOD )
	{
		// play drink sound
		playSoundEntity(players[player]->entity, 52, 64);
	}
	else
	{
		playSoundEntity(players[player]->entity, 50 + local_rng.rand() % 2, 64);
	}

	// chance of rottenness
	pukeChance = item->foodGetPukeChance(stats[player]);

	if ( players[player] 
		&& players[player]->entity && playerRequiresBloodToSustain(player) )
	{
		if ( item->type == FOOD_BLOOD )
		{
			pukeChance = 100;
		}
		else if ( item->type != FOOD_BLOOD )
		{
			if ( stats[player]->type == VAMPIRE )
			{
				pukeChance = 1;
			}
		}
	}
	else if ( item->type == FOOD_BLOOD )
	{
		pukeChance = 1;
	}

	if ( players[player] && players[player]->entity && players[player]->entity->effectShapeshift != NOTHING )
	{
		pukeChance = 100; // shapeshifted players don't puke
	}
	else if ( player >= 0 && stats[player]->type == INSECTOID )
	{
		pukeChance = 100; // insectoids can eat anything.
	}
	else if ( item->beatitude < 0 && item->type != FOOD_CREAMPIE && pukeChance == 100 )
	{
		pukeChance = 99; // make it so you will vomit
	}

	if ( item->beatitude < 0 && item->type == FOOD_CREAMPIE )
	{
		messagePlayer(player, MESSAGE_COMBAT | MESSAGE_STATUS, Language::get(909));
		if ( players[player] && players[player]->entity && players[player]->entity->setEffect(EFF_MESSY, true, 600, false) )
		{
			messagePlayer(player, MESSAGE_COMBAT | MESSAGE_STATUS, Language::get(910));
		}
		else
		{
			if ( stats[player]->mask && stats[player]->mask->type == MASK_HAZARD_GOGGLES )
			{
				if ( !(players[player]->entity->behavior == &actPlayer && players[player]->entity->effectShapeshift != NOTHING) )
				{
					messagePlayerColor(player, MESSAGE_STATUS, makeColorRGB(0, 255, 0), Language::get(6088));
				}
			}
		}
		consumeItem(item, player);
		return;
	}
	if (((item->beatitude < 0 && item->type != FOOD_CREAMPIE) || (local_rng.rand() % pukeChance == 0)) && pukeChance < 100)
	{
		if (players[player] && players[player]->entity && !(svFlags & SV_FLAG_HUNGER))
		{
			//if ( !(stats[player]->mask && stats[player]->mask->type == MASK_MARIGOLD) )
			{
				playSoundEntity(players[player]->entity, 28, 64);
				players[player]->entity->modHP(-5);
			}
		}
		if ( stats[player]->type == VAMPIRE )
		{
			messagePlayer(player, MESSAGE_STATUS, Language::get(3201));
		}
		else if ( item->type == FOOD_BLOOD )
		{
			messagePlayer(player, MESSAGE_STATUS, Language::get(3203));
		}
		else
		{
			messagePlayer(player, MESSAGE_STATUS, Language::get(908));
		}
		if ( stats[player] && players[player] && players[player]->entity )
		{
			if ( players[player]->entity->entityCanVomit() )
			{
				players[player]->entity->char_gonnavomit = 40 + local_rng.rand() % 10;
			}
		}
		consumeItem(item, player);
		return;
	}

	real_t foodMult = 1.0;
	//int bonusFoodHeal = 0;
	if ( stats[player]->helmet && stats[player]->helmet->type == HAT_CHEF )
	{
		if ( stats[player]->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
		{
			if ( svFlags & SV_FLAG_HUNGER )
			{
				foodMult += 0.2 + abs(stats[player]->helmet->beatitude) * 0.1;
			}
			else
			{
				foodMult += 0.5 + abs(stats[player]->helmet->beatitude) * 0.25;
			}
		}
		else
		{
			foodMult = 0.6 - abs(stats[player]->helmet->beatitude) * 0.1;
		}
		foodMult = std::max(0.2, foodMult);
	}

	// replenish nutrition points
	if ( svFlags & SV_FLAG_HUNGER )
	{
		int hungerIncrease = 10;
		switch ( item->type )
		{
			case FOOD_BREAD:
				hungerIncrease = 400;
				break;
			case FOOD_CREAMPIE:
				hungerIncrease = 200;
				break;
			case FOOD_CHEESE:
				hungerIncrease = 100;
				break;
			case FOOD_APPLE:
				hungerIncrease = 200;
				break;
			case FOOD_MEAT:
				hungerIncrease = 600;
				break;
			case FOOD_FISH:
				hungerIncrease = 500;
				break;
			case FOOD_TOMALLEY:
				hungerIncrease = 400;
				break;
			case FOOD_BLOOD:
				if ( players[player] && players[player]->entity 
					&& playerRequiresBloodToSustain(player) )
				{
					hungerIncrease = 250;
				}
				else
				{
					hungerIncrease = 10;
				}
				break;
			default:
				hungerIncrease = 10;
				break;
		}

		/*if ( stats[player]->playerRace == RACE_INSECTOID && stats[player]->stat_appearance == 0 )
		{
			if ( stats[player]->MAXMP >= 50 )
			{
				real_t nominalIncrease = 50 * (hungerIncrease / 1000.0);
				real_t currentIncrease = stats[player]->MAXMP * (hungerIncrease / 1000.0);
				hungerIncrease = hungerIncrease * (nominalIncrease / currentIncrease);
			}
		}*/

		stats[player]->HUNGER += hungerIncrease * foodMult;

		if ( stats[player]->mask && stats[player]->mask->type == MASK_MARIGOLD )
		{
			if ( players[player] && players[player]->entity )
			{
				players[player]->entity->setEffect(EFF_MARIGOLD, true, stats[player]->EFFECTS_TIMERS[EFF_MARIGOLD] + TICKS_PER_SECOND * 30, false);
			}
		}
	}
	else
	{
		if (players[player] && players[player]->entity)
		{
			int foodMod = 5;
			if ( stats[player]->mask && stats[player]->mask->type == MASK_MARIGOLD )
			{
				foodMod += 3;
				if ( stats[player]->mask->beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
				{
					foodMod += 3 * std::min(2, abs(stats[player]->mask->beatitude));
				}
			}

			players[player]->entity->modHP(std::max(1, (int)(foodMod * foodMult)));
			messagePlayer(player, MESSAGE_WORLD, Language::get(911));


			if ( stats[player]->playerRace == RACE_INSECTOID && stats[player]->stat_appearance == 0 )
			{
				real_t manaRegenPercent = 0.f;
				switch ( item->type )
				{
					case FOOD_BREAD:
					case FOOD_TOMALLEY:
						manaRegenPercent = 0.4;
						break;
					case FOOD_CREAMPIE:
						manaRegenPercent = 0.2;
						break;
					case FOOD_CHEESE:
						manaRegenPercent = 0.1;
						break;
					case FOOD_APPLE:
						manaRegenPercent = 0.2;
						break;
					case FOOD_MEAT:
					case FOOD_FISH:
						manaRegenPercent = 0.5;
						break;
					case FOOD_BLOOD:
						if ( players[player] && players[player]->entity
							&& playerRequiresBloodToSustain(player) )
						{
							manaRegenPercent = 0.25;
						}
						else
						{
							manaRegenPercent = 0.1;
						}
						break;
					default:
						break;
				}
				manaRegenPercent *= foodMult;
				int manaAmount = std::min(stats[player]->MAXMP, 50) * manaRegenPercent;
				players[player]->entity->modMP(manaAmount);
			}
		}
	}

	if ( item->type == FOOD_TOMALLEY )
	{
		serverUpdatePlayerGameplayStats(player, STATISTICS_TEMPT_FATE, 1);
	}

	// results of eating
	if ( players[player] )
	{
		updateHungerMessages(players[player]->entity, stats[player], item);
	}
	consumeItem(item, player);
}

void item_FoodTin(Item*& item, int player)
{
	if ( !item )
	{
		return;
	}
	int oldcount;
	int pukeChance;
	bool slippery = false;

	if ( player >= 0 && stats[player]->type == AUTOMATON )
	{
		if ( players[player] && players[player]->entity )
		{
			item_FoodAutomaton(item, player);
		}
		return;
	}

	if ( player >= 0 && stats[player]->type != HUMAN && (svFlags & SV_FLAG_HUNGER) ) // hunger on
	{
		if ( stats[player]->type == SKELETON )
		{
			if ( players[player]->isLocalPlayer() )
			{
				steamAchievement("BARONY_ACH_BONEHEADED");
				dropItem(item, player); // client drop item
				messagePlayer(player, MESSAGE_HINT, Language::get(3179));
				playSoundPlayer(player, 90, 64);
			}
			return;
		}
	}

	if ( stats[player]->amulet != NULL )
	{
		if ( stats[player]->amulet->type == AMULET_STRANGULATION
			&& stats[player]->type != SKELETON )
		{
			if ( players[player]->isLocalPlayer() )
			{
				messagePlayer(player, MESSAGE_HINT, Language::get(756));
				playSoundPlayer(player, 90, 64);
			}
			return;
		}
	}

	// can't eat while vomiting
	if ( stats[player]->EFFECTS[EFF_VOMITING] )
	{
		if ( players[player]->isLocalPlayer() )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(757));
			playSoundPlayer(player, 90, 64);
		}
		return;
	}

	if ( players[player]->isLocalPlayer() )
	{
		conductFoodless = false;
		conductVegetarian = false;
		if ( stats[player]->playerRace == RACE_SKELETON && stats[player]->stat_appearance == 0
			&& players[player] && players[player]->entity->effectPolymorph > NUMMONSTERS )
		{
			steamAchievement("BARONY_ACH_MUSCLE_MEMORY");
		}
		if ( stats[player]->type == GOATMAN )
		{
			steamStatisticUpdate(STEAM_STAT_IRON_GUT, STEAM_STAT_INT, 1);
		}
		Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_CONSUMED, item->type, 1);
	}

	if ( multiplayer == CLIENT )
	{
		consumeItem(item, player);
		return;
	}

	// consumption message
	char tempstr[128] = { 0 };
	oldcount = item->count;
	item->count = 1;

	bool hpBuff = false;
	bool mpBuff = false;

	// first word
	int word = local_rng.rand() % 16;
	strcpy(tempstr, Language::get(918 + word));
	if ( word == 6 || word == 15 )
	{
		slippery = true;
	}
	if ( word < 8 )
	{
		hpBuff = true;
	}

	// second word
	word = local_rng.rand() % 16;
	strcat(tempstr, Language::get(934 + word));
	if ( word == 1 || word == 7 || word == 8 || word == 12 )
	{
		slippery = true;
	}
	if ( word >= 8 )
	{
		mpBuff = true;
	}

	// third word
	word = local_rng.rand() % 16;
	strcat(tempstr, Language::get(950 + word));
	if ( word == 1 || word == 8 )
	{
		slippery = true;
	}
	if ( word == 8 )
	{
		hpBuff = true;
		mpBuff = true;
	}

	if ( stats[player]->type == GOATMAN )
	{
		messagePlayer(player, MESSAGE_STATUS, Language::get(3220), tempstr);
	}
	else
	{
		messagePlayer(player, MESSAGE_STATUS, Language::get(764), tempstr);
	}
	item->count = oldcount;

	// eating sound
	if ( players[player] )
	{
		playSoundEntity(players[player]->entity, 50 + local_rng.rand() % 2, 64);
	}

	serverUpdatePlayerGameplayStats(player, STATISTICS_YES_WE_CAN, 1);

	// chance of rottenness
	pukeChance = item->foodGetPukeChance(stats[player]);

	if ((item->beatitude < 0 || local_rng.rand() % pukeChance == 0) && pukeChance < 100)
	{
		if (players[player] && players[player]->entity && !(svFlags & SV_FLAG_HUNGER))
		{
			//if ( !(stats[player]->mask && stats[player]->mask->type == MASK_MARIGOLD) )
			{
				playSoundEntity(players[player]->entity, 28, 64);
				players[player]->entity->modHP(-5);
			}
		}
		if ( stats[player]->type == VAMPIRE )
		{
			messagePlayer(player, MESSAGE_STATUS | MESSAGE_HINT, Language::get(3201));
		}
		else
		{
			messagePlayer(player, MESSAGE_STATUS | MESSAGE_HINT, Language::get(908));
		}

		if ( stats[player] && players[player] && players[player]->entity )
		{
			if ( players[player]->entity->entityCanVomit() )
			{
				players[player]->entity->char_gonnavomit = 40 + local_rng.rand() % 10;
			}
		}
		consumeItem(item, player);
		return;
	}

	int buffDuration = item->status * TICKS_PER_SECOND * 4; // (4 - 16 seconds)
	if ( item->status > WORN )
	{
		buffDuration -= local_rng.rand() % ((buffDuration / 2) + 1); // 50-100% duration
	}
	else
	{
		buffDuration -= local_rng.rand() % ((buffDuration / 4) + 1); // 75-100% duration
	}

	real_t foodMult = 1.0;
	//int bonusFoodHeal = 0;
	if ( stats[player]->helmet && stats[player]->helmet->type == HAT_CHEF )
	{
		if ( stats[player]->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
		{
			if ( svFlags & SV_FLAG_HUNGER )
			{
				foodMult += 0.2 + abs(stats[player]->helmet->beatitude) * 0.1;
			}
			else
			{
				foodMult += 0.5 + abs(stats[player]->helmet->beatitude) * 0.25;
			}
		}
		else
		{
			foodMult = 0.6 - abs(stats[player]->helmet->beatitude) * 0.1;
		}
		foodMult = std::min(1.0, foodMult);
		foodMult = std::max(0.2, foodMult);
	}

	// replenish nutrition points
	if (svFlags & SV_FLAG_HUNGER)
	{
		stats[player]->HUNGER += 600 * foodMult;

		if ( stats[player]->mask && stats[player]->mask->type == MASK_MARIGOLD )
		{
			if ( players[player] && players[player]->entity )
			{
				players[player]->entity->setEffect(EFF_MARIGOLD, true, stats[player]->EFFECTS_TIMERS[EFF_MARIGOLD] + TICKS_PER_SECOND * 30, false);
			}
		}

		if ( hpBuff )
		{
			stats[player]->EFFECTS[EFF_HP_REGEN] = hpBuff;
			stats[player]->EFFECTS_TIMERS[EFF_HP_REGEN] = buffDuration;
			Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_TIN_REGEN_HP, FOOD_TIN, 1);
		}
		if ( mpBuff )
		{
			stats[player]->EFFECTS[EFF_MP_REGEN] = mpBuff;
			stats[player]->EFFECTS_TIMERS[EFF_MP_REGEN] = buffDuration;
			Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_TIN_REGEN_MP, FOOD_TIN, 1);
		}
	}
	else
	{
		if (players[player] && players[player]->entity)
		{
			int foodMod = 10;
			if ( stats[player]->mask && stats[player]->mask->type == MASK_MARIGOLD )
			{
				foodMod += 3;
				if ( stats[player]->mask->beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
				{
					foodMod += 3 * std::min(2, abs(stats[player]->mask->beatitude));
				}
			}

			players[player]->entity->modHP(std::max(1, (int)(foodMod * foodMult)));
			messagePlayer(player, MESSAGE_WORLD, Language::get(911));
			if ( stats[player]->playerRace == RACE_INSECTOID && stats[player]->stat_appearance == 0 )
			{
				real_t manaRegenPercent = 0.6 * foodMult;
				int manaAmount = std::min(stats[player]->MAXMP, 50) * manaRegenPercent;
				players[player]->entity->modMP(manaAmount);
			}
		}
	}

	// greasy fingers
	if ( slippery )
	{
		// 1-2 minutes of greasy
		if ( players[player] && players[player]->entity )
		{
			if ( players[player]->entity->setEffect(EFF_GREASY, true, TICKS_PER_SECOND * (60 + local_rng.rand() % 60), true) )
			{
				messagePlayer(player, MESSAGE_STATUS | MESSAGE_HINT, Language::get(966));
				Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_TIN_GREASY, FOOD_TIN, 1);
			}
			else
			{
				slippery = false;
			}
		}
	}

	if ( slippery || hpBuff || mpBuff )
	{
		serverUpdateEffects(player);
	}

	// results of eating
	if ( players[player] )
	{
		updateHungerMessages(players[player]->entity, stats[player], item);
	}

	if ( (hpBuff || mpBuff) && (svFlags & SV_FLAG_HUNGER) )
	{
		messagePlayer(player, MESSAGE_WORLD, Language::get(911));
	}

	consumeItem(item, player);
}

void item_AmuletSexChange(Item* item, int player)
{
	if ( !players[player]->isLocalPlayer() )
	{
		consumeItem(item, player);
	}

	if ( stats[player]->amulet != NULL )
	{
		if ( !stats[player]->amulet->canUnequip(stats[player]) )
		{
			if ( players[player]->isLocalPlayer() )
			{
				if ( shouldInvertEquipmentBeatitude(stats[player]) && item->beatitude > 0 )
				{
					messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(3217), stats[player]->amulet->getName());
				}
				else
				{
					messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(1089), stats[player]->amulet->getName());
				}
				playSoundPlayer(player, 90, 64);
			}
			return;
		}
	}

	if ( multiplayer != CLIENT )
	{
		playSoundEntity(players[player]->entity, 33 + local_rng.rand() % 2, 64);
		playSoundEntity(players[player]->entity, 76, 64);
	}

	if ( players[player] && players[player]->isLocalPlayer() )
	{
		messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(1094));
	}

	stats[player]->amulet = NULL;
	stats[player]->sex = static_cast<sex_t>((stats[player]->sex == 0));

	serverUpdateSexChange(player);

	if ( !players[player]->isLocalPlayer() )
	{
		return;
	}

	Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_BROKEN, item->type, 1);
	consumeItem(item, player);

	// find out what creature we are...
	if ( stats[player]->sex == FEMALE 
		&& stats[player]->playerRace == RACE_INCUBUS 
		&& stats[player]->stat_appearance == 0 )
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(4048)); // don't feel like yourself
	}
	else if ( stats[player]->sex == MALE 
		&& stats[player]->playerRace == RACE_SUCCUBUS 
		&& stats[player]->stat_appearance == 0 )
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(4048)); // don't feel like yourself
	}
	else
	{
		if ( stats[player]->sex == MALE )
		{
			messagePlayer(player, MESSAGE_HINT | MESSAGE_STATUS, Language::get(967));
		}
		else
		{
			messagePlayer(player, MESSAGE_HINT | MESSAGE_STATUS, Language::get(968));
		}
	}
	messagePlayer(player, MESSAGE_INVENTORY, Language::get(969));
}

void item_Spellbook(Item*& item, int player)
{
	node_t* node, *nextnode;

	item->identified = true;
	if ( players[player] && !players[player]->isLocalPlayer() )
	{
		return;
	}

	if ( players[player] && players[player]->entity && players[player]->entity->isBlind())
	{
		messagePlayer(player, MESSAGE_HINT, Language::get(970));
		playSoundPlayer(player, 90, 64);
		return;
	}
	if ( itemIsEquipped(item, player) )
	{
		messagePlayer(player, MESSAGE_MISC, Language::get(3460));
		playSoundPlayer(player, 90, 64);
		return;
	}

	if ( players[player] && players[player]->entity )
	{
		if ( players[player]->entity->effectShapeshift != NOTHING )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(3445));
			playSoundPlayer(player, 90, 64);
			return;
		}
		else if ( stats[player] && (stats[player]->type == GOBLIN || (stats[player]->playerRace == RACE_GOBLIN && stats[player]->stat_appearance == 0)) )
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(3444));
			playSoundPlayer(player, 90, 64);
			return;
		}
	}

	conductIlliterate = false;

	if ( item->beatitude < 0 && !shouldInvertEquipmentBeatitude(stats[player]) )
	{
		messagePlayer(player, MESSAGE_INVENTORY, Language::get(971));
		if ( list_Size(&players[player]->magic.spellList) > 0 && stats[player]->type != AUTOMATON )
		{
			// randomly delete a spell
			int spellToDelete = local_rng.rand() % list_Size(&players[player]->magic.spellList);
			node = list_Node(&players[player]->magic.spellList, spellToDelete);
			spell_t* spell = (spell_t*)node->element;
			int spellID = spell->ID;
			bool deleted = false;
			bool rerollSpell = false;

			// delete its accompanying spell item(s)

			if ( client_classes[player] == CLASS_SHAMAN )
			{
				// don't forget your racial spells otherwise borked.
				// special roll checking.
				if ( list_Size(&players[player]->magic.spellList) <= CLASS_SHAMAN_NUM_STARTING_SPELLS )
				{
					// no spells to delete. return early.
					messagePlayer(player, MESSAGE_HINT, Language::get(973));
					consumeItem(item, player);
					return;
				}
				spellToDelete = local_rng.rand() % (list_Size(&players[player]->magic.spellList) - CLASS_SHAMAN_NUM_STARTING_SPELLS);
				spellToDelete += CLASS_SHAMAN_NUM_STARTING_SPELLS; // e.g 16 spells is 0 + 15, 15th index.
				node = list_Node(&players[player]->magic.spellList, spellToDelete);
				spell = (spell_t*)node->element;
				spellID = spell->ID;
			}

			for ( node_t* node2 = stats[player]->inventory.first; node2 != NULL; node2 = nextnode )
			{
				nextnode = node2->next;
				Item* itemInventory = (Item*)node2->element;
				if ( itemInventory && itemInventory->type == SPELL_ITEM )
				{
					if ( rerollSpell )
					{
						// Unused. But if ever shapeshift form spells get added to general classes then will need this checking.
						//if ( itemInventory->appearance < 1000 )
						//{
						//	// can delete non-shapeshift spells. let's delete this one.
						//	spell = getSpellFromItem(itemInventory);
						//	if ( spell )
						//	{
						//		spellID = spell->ID;
						//		if ( spellID == SPELL_RAT_FORM || spellID == SPELL_SPIDER_FORM
						//			|| spellID == SPELL_TROLL_FORM || spellID == SPELL_IMP_FORM
						//			|| spellID == SPELL_REVERT_FORM )
						//		{
						//			continue;
						//		}
						//		// find the node in spellList for the ID
						//		spellToDelete = 0;
						//		for ( node_t* tmpNode = spellList.first; tmpNode != nullptr; tmpNode = tmpNode->next )
						//		{
						//			if ( tmpNode->element )
						//			{
						//				spell_t* tmpSpell = (spell_t*)tmpNode->element;
						//				if ( tmpSpell && tmpSpell->ID == spellID )
						//				{
						//					// found the node in the list, delete this one.
						//					node = list_Node(&spellList, spellToDelete);
						//					list_RemoveNode(node2); // delete inventory spell.
						//					deleted = true;
						//					break;
						//				}
						//			}
						//			++spellToDelete;
						//		}
						//		if ( deleted )
						//		{
						//			break;
						//		}
						//	}
						//}
					}
					else if ( itemInventory->appearance == spellID )
					{
						list_RemoveNode(node2); // delete inventory spell.
						deleted = true;
						break;
					}
				}
			}

			//messagePlayer(0, "%d, %d %d", rerollSpell, spellToDelete, deleted);

			if ( !deleted )
			{
				// maybe we've got an inventory full of shapeshift spells?
				messagePlayer(player, MESSAGE_HINT, Language::get(973));
				consumeItem(item, player);
				return;
			}
			else if ( deleted )
			{
				messagePlayer(player, MESSAGE_STATUS | MESSAGE_PROGRESSION | MESSAGE_HINT, Language::get(972));
				if ( spell == players[player]->magic.selectedSpell() )
				{
					players[player]->magic.equipSpell(nullptr);
				}
				for ( int i = 0; i < NUM_HOTBAR_ALTERNATES; ++i )
				{
					if ( players[player]->magic.selected_spell_alternate[i] == spell )
					{
						players[player]->magic.selected_spell_alternate[i] = nullptr;
					}
				}
				list_RemoveNode(node);
			}
		}
		else
		{
			messagePlayer(player, MESSAGE_HINT, Language::get(973));
		}
		consumeItem(item, player);
		return;
	}
	else
	{
		bool learned = false;
		switch ( item->type )
		{
			case SPELLBOOK_FORCEBOLT:
				learned = addSpell(SPELL_FORCEBOLT, player);
				break;
			case SPELLBOOK_MAGICMISSILE:
				learned = addSpell(SPELL_MAGICMISSILE, player);
				break;
			case SPELLBOOK_COLD:
				learned = addSpell(SPELL_COLD, player);
				break;
			case SPELLBOOK_FIREBALL:
				learned = addSpell(SPELL_FIREBALL, player);
				break;
			case SPELLBOOK_LIGHTNING:
				learned = addSpell(SPELL_LIGHTNING, player);
				break;
			case SPELLBOOK_REMOVECURSE:
				learned = addSpell(SPELL_REMOVECURSE, player);
				break;
			case SPELLBOOK_LIGHT:
				learned = addSpell(SPELL_LIGHT, player);
				break;
			case SPELLBOOK_IDENTIFY:
				learned = addSpell(SPELL_IDENTIFY, player);
				break;
			case SPELLBOOK_MAGICMAPPING:
				learned = addSpell(SPELL_MAGICMAPPING, player);
				break;
			case SPELLBOOK_SLEEP:
				learned = addSpell(SPELL_SLEEP, player);
				break;
			case SPELLBOOK_CONFUSE:
				learned = addSpell(SPELL_CONFUSE, player);
				break;
			case SPELLBOOK_SLOW:
				learned = addSpell(SPELL_SLOW, player);
				break;
			case SPELLBOOK_OPENING:
				learned = addSpell(SPELL_OPENING, player);
				break;
			case SPELLBOOK_LOCKING:
				learned = addSpell(SPELL_LOCKING, player);
				break;
			case SPELLBOOK_LEVITATION:
				learned = addSpell(SPELL_LEVITATION, player);
				break;
			case SPELLBOOK_INVISIBILITY:
				learned = addSpell(SPELL_INVISIBILITY, player);
				break;
			case SPELLBOOK_TELEPORTATION:
				learned = addSpell(SPELL_TELEPORTATION, player);
				break;
			case SPELLBOOK_HEALING:
				learned = addSpell(SPELL_HEALING, player);
				break;
			case SPELLBOOK_EXTRAHEALING:
				learned = addSpell(SPELL_EXTRAHEALING, player);
				break;
			case SPELLBOOK_CUREAILMENT:
				learned = addSpell(SPELL_CUREAILMENT, player);
				break;
			case SPELLBOOK_DIG:
				learned = addSpell(SPELL_DIG, player);
				break;
			case SPELLBOOK_SUMMON:
				learned = addSpell(SPELL_SUMMON, player);
				break;
			case SPELLBOOK_STONEBLOOD:
				learned = addSpell(SPELL_STONEBLOOD, player);
				break;
			case SPELLBOOK_BLEED:
				learned = addSpell(SPELL_BLEED, player);
				break;
			case SPELLBOOK_REFLECT_MAGIC:
				learned = addSpell(SPELL_REFLECT_MAGIC, player);
				break;
			case SPELLBOOK_ACID_SPRAY:
				learned = addSpell(SPELL_ACID_SPRAY, player);
				break;
			case SPELLBOOK_STEAL_WEAPON:
				learned = addSpell(SPELL_STEAL_WEAPON, player);
				break;
			case SPELLBOOK_DRAIN_SOUL:
				learned = addSpell(SPELL_DRAIN_SOUL, player);
				break;
			case SPELLBOOK_VAMPIRIC_AURA:
				learned = addSpell(SPELL_VAMPIRIC_AURA, player);
				break;
			case SPELLBOOK_CHARM_MONSTER:
				learned = addSpell(SPELL_CHARM_MONSTER, player);
				break;
			case SPELLBOOK_REVERT_FORM:
				learned = addSpell(SPELL_REVERT_FORM, player);
				break;
			case SPELLBOOK_RAT_FORM:
				learned = addSpell(SPELL_RAT_FORM, player);
				break;
			case SPELLBOOK_SPIDER_FORM:
				learned = addSpell(SPELL_SPIDER_FORM, player);
				break;
			case SPELLBOOK_TROLL_FORM:
				learned = addSpell(SPELL_TROLL_FORM, player);
				break;
			case SPELLBOOK_IMP_FORM:
				learned = addSpell(SPELL_IMP_FORM, player);
				break;
			case SPELLBOOK_SPRAY_WEB:
				learned = addSpell(SPELL_SPRAY_WEB, player);
				break;
			case SPELLBOOK_POISON:
				learned = addSpell(SPELL_POISON, player);
				break;
			case SPELLBOOK_SPEED:
				learned = addSpell(SPELL_SPEED, player);
				break;
			case SPELLBOOK_FEAR:
				learned = addSpell(SPELL_FEAR, player);
				break;
			case SPELLBOOK_STRIKE:
				learned = addSpell(SPELL_STRIKE, player);
				break;
			case SPELLBOOK_DETECT_FOOD:
				learned = addSpell(SPELL_DETECT_FOOD, player);
				break;
			case SPELLBOOK_WEAKNESS:
				learned = addSpell(SPELL_WEAKNESS, player);
				break;
			case SPELLBOOK_AMPLIFY_MAGIC:
				learned = addSpell(SPELL_AMPLIFY_MAGIC, player);
				break;
			case SPELLBOOK_SHADOW_TAG:
				learned = addSpell(SPELL_SHADOW_TAG, player);
				break;
			case SPELLBOOK_TELEPULL:
				learned = addSpell(SPELL_TELEPULL, player);
				break;
			case SPELLBOOK_DEMON_ILLU:
				learned = addSpell(SPELL_DEMON_ILLUSION, player);
				break;
			case SPELLBOOK_TROLLS_BLOOD:
				learned = addSpell(SPELL_TROLLS_BLOOD, player);
				break;
			case SPELLBOOK_SALVAGE:
				learned = addSpell(SPELL_SALVAGE, player);
				break;
			case SPELLBOOK_FLUTTER:
				learned = addSpell(SPELL_FLUTTER, player);
				break;
			case SPELLBOOK_DASH:
				learned = addSpell(SPELL_DASH, player);
				break;
			case SPELLBOOK_SELF_POLYMORPH:
				learned = addSpell(SPELL_SELF_POLYMORPH, player);
				break;
			case SPELLBOOK_9:
				learned = addSpell(SPELL_CRAB_FORM, player);
				break;
			case SPELLBOOK_10:
				learned = addSpell(SPELL_CRAB_WEB, player);
				break;
			default:
				learned = addSpell(SPELL_FORCEBOLT, player);
				break;
		}

		if ( players[player] )
		{
			players[player]->magic.spellbookUidFromHotbarSlot = 0;
		}

		if ( learned )
		{
			Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_SPELLBOOK_LEARNT, item->type, 1);
			Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_SPELLBOOK_CAST_DEGRADES, item->type, 1);
			if ( item->type >= SPELLBOOK_RAT_FORM && item->type <= SPELLBOOK_IMP_FORM )
			{
				ItemType originalSpellbook = item->type;
				item->type = SPELLBOOK_REVERT_FORM;
				if ( !playerLearnedSpellbook(player, item) ) // have we learnt "revert form"?
				{
					addSpell(SPELL_REVERT_FORM, player, true); // add it.
				}
				item->type = originalSpellbook;
			}
			item->status = static_cast<Status>(item->status - 1);
			if ( item->status != BROKEN )
			{
				messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_EQUIPMENT, Language::get(2595));
			}
			else
			{
				messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_EQUIPMENT, Language::get(2596));
				consumeItem(item, player);
			}

			if ( stats[player] && stats[player]->playerRace == RACE_INSECTOID && stats[player]->stat_appearance == 0 )
			{
				steamStatisticUpdate(STEAM_STAT_BOOKWORM, STEAM_STAT_INT, 1);
			}
			if ( list_Size(&players[player]->magic.spellList) >= 20 )
			{
				steamAchievement("BARONY_ACH_MAGIC_MASTERY");
			}
		}
	}
}

void item_FoodAutomaton(Item*& item, int player)
{
	if ( !stats[player] || !players[player] || !players[player]->entity )
	{
		return;
	}

	if ( player >= 0 && stats[player]->type != AUTOMATON )
	{
		//messagePlayer(player, "Err: You are not an Automaton!");
		return;
	}

	if ( !itemIsConsumableByAutomaton(*item) 
		&& item->type != POTION_FIRESTORM )
	{
		return;
	}

	if ( multiplayer == CLIENT )
	{
		consumeItem(item, player);
		return;
	}

	// consumption message
	if ( item->type != TOOL_MAGIC_SCRAP && item->type != TOOL_METAL_SCRAP )
	{
		int oldcount = item->count;
		item->count = 1;
		messagePlayer(player, MESSAGE_STATUS, Language::get(907), item->description());
		item->count = oldcount;
	}

	// eating sound
	if ( item->type == FOOD_BLOOD || item->type == POTION_FIRESTORM )
	{
		// play drink sound
		playSoundEntity(players[player]->entity, 52, 64);
	}
	else
	{
		playSoundEntity(players[player]->entity, 50 + local_rng.rand() % 2, 64);
	}

	if ( item->beatitude < 0 && item->type == FOOD_CREAMPIE )
	{
		messagePlayer(player, MESSAGE_STATUS | MESSAGE_COMBAT, Language::get(909));
		if ( players[player]->entity && players[player]->entity->setEffect(EFF_MESSY, true, 600, false) )
		{
			messagePlayer(player, MESSAGE_COMBAT | MESSAGE_STATUS, Language::get(910));
		}
		consumeItem(item, player);
		return;
	}

	// automaton hunger/combustion tick is every 0.6 seconds.
	// 600 hunger = 6 minutes
	// 200 hunger = 2 minutes
	// 100 hunger = 60 seconds.
	// 80 hunger = 48 seconds
	// 50 hunger = 30 seconds
	// 40 hunger = 24 seconds
	// 20 hunger = 12 seconds
	int oldHunger = stats[player]->HUNGER;
	Uint32 color = makeColorRGB(255, 128, 0);

	// replenish nutrition points
	// automaton hunger is always in effect
	// hunger disabled will simply add 5 HP and still add fuel to the fire.
	switch ( item->type )
	{
		case FOOD_BREAD:
		case FOOD_CREAMPIE:
		case FOOD_BLOOD:
		case FOOD_CHEESE:
		case FOOD_APPLE:
		case FOOD_TOMALLEY:
		case FOOD_MEAT:
		case FOOD_FISH:
		case FOOD_TIN:
			if ( svFlags & SV_FLAG_HUNGER )
			{
				messagePlayer(player, MESSAGE_STATUS, Language::get(3697)); // no effect.
				consumeItem(item, player);
				return;
			}
			break;
		case GEM_ROCK:
		case GEM_GLASS:
			stats[player]->HUNGER += 50;
			break;
		case GEM_LUCK:
		case GEM_GARNET:
		case GEM_RUBY:
		case GEM_JACINTH:
		case GEM_AMBER:
		case GEM_CITRINE:
		case GEM_JADE:
		case GEM_EMERALD:
		case GEM_SAPPHIRE:
		case GEM_AQUAMARINE:
		case GEM_AMETHYST:
		case GEM_FLUORITE:
		case GEM_OPAL:
		case GEM_DIAMOND:
		case GEM_JETSTONE:
		case GEM_OBSIDIAN:
			stats[player]->HUNGER += 1000;
			players[player]->entity->modMP(10);
			break;
		case READABLE_BOOK:
			stats[player]->HUNGER += 400;
			if ( stats[player]->playerRace == RACE_AUTOMATON && stats[player]->stat_appearance == 0 )
			{
				steamStatisticUpdateClient(player, STEAM_STAT_FASCIST, STEAM_STAT_INT, 1);
			}
			break;
		case SCROLL_MAIL:
		case SCROLL_BLANK:
			stats[player]->HUNGER += 200;
			break;
		case SCROLL_IDENTIFY:
		case SCROLL_LIGHT:
		case SCROLL_REMOVECURSE:
		case SCROLL_FOOD:
		case SCROLL_MAGICMAPPING:
		case SCROLL_REPAIR:
		case SCROLL_DESTROYARMOR:
		case SCROLL_TELEPORTATION:
		case SCROLL_SUMMON:
		case SCROLL_CONJUREARROW:
		case SCROLL_CHARGING:
			players[player]->entity->modMP(20);
			stats[player]->HUNGER += 600;
			break;
		case SCROLL_ENCHANTWEAPON:
		case SCROLL_ENCHANTARMOR:
			players[player]->entity->modMP(40);
			stats[player]->HUNGER += 600;
			break;
		case SCROLL_FIRE:
		{
			stats[player]->HUNGER += 1500;
			players[player]->entity->modMP(stats[player]->MAXMP);
			messagePlayerColor(player, MESSAGE_STATUS, color, Language::get(3699)); // superheats
			if ( stats[player]->playerRace == RACE_AUTOMATON && stats[player]->stat_appearance == 0 )
			{
				steamStatisticUpdateClient(player, STEAM_STAT_SPICY, STEAM_STAT_INT, 1);
			}

			playSoundEntity(players[player]->entity, 153, 128); // "FireballExplode.ogg"
			for ( int c = 0; c < 100; c++ )
			{
				if ( Entity* entity = spawnFlame(players[player]->entity, SPRITE_FLAME) )
				{
					entity->sprite = 16;
					double vel = local_rng.rand() % 10;
					entity->vel_x = vel * cos(entity->yaw) * cos(entity->pitch) * .1;
					entity->vel_y = vel * sin(entity->yaw) * cos(entity->pitch) * .1;
					entity->vel_z = vel * sin(entity->pitch) * .2;
					entity->skill[0] = 5 + local_rng.rand() % 10;
				}
			}
			break;
		}
		case TOOL_METAL_SCRAP:
			if ( stats[player]->playerRace == RACE_AUTOMATON && stats[player]->stat_appearance == 0 )
			{
				achievementObserver.playerAchievements[player].trashCompactor += 1;
			}
			if ( stats[player]->HUNGER > 500 )
			{
				messagePlayer(player, MESSAGE_STATUS, Language::get(3707)); // fails to add any more heat.
				consumeItem(item, player);
				return;
			}
			else
			{
				stats[player]->HUNGER += 50;
				stats[player]->HUNGER = std::min(stats[player]->HUNGER, 550);
			}
			break;
		case TOOL_MAGIC_SCRAP:
			if ( stats[player]->playerRace == RACE_AUTOMATON && stats[player]->stat_appearance == 0 )
			{
				achievementObserver.playerAchievements[player].trashCompactor += 1;
			}
			if ( stats[player]->HUNGER > 1100 )
			{
				messagePlayer(player, MESSAGE_STATUS, Language::get(3707)); // fails to add any more heat.
				consumeItem(item, player);
				return;
			}
			else
			{
				stats[player]->HUNGER += 100;
				stats[player]->HUNGER = std::min(stats[player]->HUNGER, 1199);
			}
			break;
		default:
			messagePlayer(player, MESSAGE_DEBUG, "Unknown food?");
			break;
	}

	if ( item->type == SCROLL_MAIL || item->type == READABLE_BOOK )
	{
		Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_LORE_BURNT, item->type, 1);
	}

	if ( itemCategory(item) == SCROLL )
	{
		if ( stats[player]->playerRace == RACE_AUTOMATON && stats[player]->stat_appearance == 0 )
		{
			steamStatisticUpdateClient(player, STEAM_STAT_FASCIST, STEAM_STAT_INT, 1);
		}
	}

	if ( !(svFlags & SV_FLAG_HUNGER) && oldHunger == stats[player]->HUNGER ) // ate food, hunger is disabled and did not gain heat (normal food items)
	{
		if ( players[player] && players[player]->entity )
		{
			if ( item->beatitude < 0 )
			{
				playSoundEntity(players[player]->entity, 28, 64);
				players[player]->entity->modHP(-5);
				messagePlayer(player, MESSAGE_WORLD, Language::get(908)); // blecch! rotten food!
				consumeItem(item, player);
				return;
			}

			players[player]->entity->modHP(5);
		}
		messagePlayer(player, MESSAGE_WORLD, Language::get(911)); // mmm, tasty!
	}

	stats[player]->HUNGER = std::min(stats[player]->HUNGER, 1500);
	// results of eating
	if ( stats[player]->HUNGER >= 1500 )
	{
		messagePlayerColor(player, MESSAGE_STATUS, color, Language::get(3483)); // at capacity
	}
	else if ( stats[player]->HUNGER >= 1200 && oldHunger < 1200 )
	{
		messagePlayerColor(player, MESSAGE_STATUS, color, Language::get(3484));
	}
	else if ( stats[player]->HUNGER >= 600 && oldHunger < 600 )
	{
		messagePlayerColor(player, MESSAGE_STATUS, color, Language::get(3696));
	}
	else if ( stats[player]->HUNGER >= 300 && oldHunger < 300 )
	{
		messagePlayerColor(player, MESSAGE_STATUS, color, Language::get(3485));
	}
	else if ( stats[player]->HUNGER <= 300 )
	{
		messagePlayerColor(player, MESSAGE_STATUS, color, Language::get(3486));
	}
	else if ( oldHunger < stats[player]->HUNGER )
	{
		messagePlayer(player, MESSAGE_STATUS, Language::get(3704));
	}

	serverUpdateHunger(player);
	consumeItem(item, player);
}

bool itemIsConsumableByAutomaton(const Item& item)
{
	switch ( item.type )
	{
		case FOOD_BREAD:
		case FOOD_CREAMPIE:
		case FOOD_BLOOD:
		case FOOD_CHEESE:
		case FOOD_APPLE:
		case FOOD_TOMALLEY:
		case FOOD_MEAT:
		case FOOD_FISH:
		case FOOD_TIN:

		case GEM_ROCK:
		case GEM_GLASS:
		case GEM_LUCK:
		case GEM_GARNET:
		case GEM_RUBY:
		case GEM_JACINTH:
		case GEM_AMBER:
		case GEM_CITRINE:
		case GEM_JADE:
		case GEM_EMERALD:
		case GEM_SAPPHIRE:
		case GEM_AQUAMARINE:
		case GEM_AMETHYST:
		case GEM_FLUORITE:
		case GEM_OPAL:
		case GEM_DIAMOND:
		case GEM_JETSTONE:
		case GEM_OBSIDIAN:

		case READABLE_BOOK:

		case SCROLL_MAIL:
		case SCROLL_BLANK:
		case SCROLL_IDENTIFY:
		case SCROLL_LIGHT:
		case SCROLL_ENCHANTWEAPON:
		case SCROLL_ENCHANTARMOR:
		case SCROLL_REMOVECURSE:
		case SCROLL_FOOD:
		case SCROLL_MAGICMAPPING:
		case SCROLL_REPAIR:
		case SCROLL_DESTROYARMOR:
		case SCROLL_TELEPORTATION:
		case SCROLL_SUMMON:
		case SCROLL_FIRE:
		case SCROLL_CONJUREARROW:
		case SCROLL_CHARGING:
		case TOOL_MAGIC_SCRAP:
		case TOOL_METAL_SCRAP:
			return true;
			break;
		default:
			return false;
			break;
	}
	return false;
}

void updateHungerMessages(Entity* my, Stat* myStats, Item* eaten)
{
	if ( !myStats || !eaten || !my)
	{
		return;
	}
	if ( my->behavior != &actPlayer )
	{
		return;
	}
	if ( (svFlags & SV_FLAG_HUNGER) )
	{
		if ( myStats->HUNGER <= getEntityHungerInterval(my->skill[2], nullptr, stats[my->skill[2]], HUNGER_INTERVAL_HUNGRY) )
		{
			messagePlayer(my->skill[2], MESSAGE_STATUS, Language::get(912));
		}
		else if ( myStats->HUNGER < 500 )
		{
			messagePlayer(my->skill[2], MESSAGE_STATUS, Language::get(913));
		}
		else if ( myStats->HUNGER < 1000 )
		{
			messagePlayer(my->skill[2], MESSAGE_STATUS, Language::get(914), eaten->getName());
		}
		else if ( myStats->HUNGER < 1500 )
		{
			messagePlayer(my->skill[2], MESSAGE_STATUS, Language::get(915));
		}
		else if ( myStats->HUNGER >= 1500 )
		{
			if ( my->effectShapeshift != NOTHING )
			{
				messagePlayer(my->skill[2], MESSAGE_STATUS, Language::get(916)); // shapeshifted players don't puke
			}
			else if ( local_rng.rand() % 3 )
			{
				messagePlayer(my->skill[2], MESSAGE_STATUS, Language::get(916));
			}
			else
			{
				if ( my->entityCanVomit() )
				{
					messagePlayer(my->skill[2], MESSAGE_STATUS, Language::get(917));
					my->char_gonnavomit = 40 + local_rng.rand() % 10;
				}
			}
		}
	}

	if ( myStats->type == INSECTOID )
	{
		myStats->HUNGER = std::min(myStats->HUNGER, 1000); // smaller hunger range.
	}
	else
	{
		myStats->HUNGER = std::min(myStats->HUNGER, 2000);
	}
	serverUpdateHunger(my->skill[2]);
}

void item_ToolLootBag(Item*& item, int player)
{
	if ( multiplayer == CLIENT )
	{
		strcpy((char*)net_packet->data, "LOOT");
		SDLNet_Write32(static_cast<Uint32>(item->appearance), &net_packet->data[4]);
		net_packet->data[8] = clientnum;
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 9;
		sendPacketSafe(net_sock, -1, net_packet, 0);
		consumeItem(item, player);
		return;
	}

	Stat::emptyLootingBag(player, item->appearance);
	consumeItem(item, player);
	return;
}
