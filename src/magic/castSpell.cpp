/*-------------------------------------------------------------------------------

	BARONY
	File: castSpell.cpp
	Desc: contains the big, fat, lengthy function that CASTS SPELLS!

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../entity.hpp"
#include "../interface/interface.hpp"
#include "../engine/audio/sound.hpp"
#include "../items.hpp"
#include "../net.hpp"
#include "../collision.hpp"
#include "../player.hpp"
#include "../scores.hpp"
#include "../colors.hpp"
#include "../ui/MainMenu.hpp"
#include "magic.hpp"
#include "../prng.hpp"
#include "../mod_tools.hpp"
#include "../paths.hpp"

bool spellIsNaturallyLearnedByRaceOrClass(Entity& caster, Stat& stat, int spellID);

void castSpellInit(Uint32 caster_uid, spell_t* spell, bool usingSpellbook)
{
	Entity* caster = uidToEntity(caster_uid);
	node_t* node = NULL;
	if ( !caster || !spell )
	{
		//Need a spell and caster to cast a spell.
		return;
	}

	if ( !spell->elements.first )
	{
		return;
	}

	int player = -1;
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( players[i] && caster && (caster == players[i]->entity) )
		{
			player = i; //Set the player.
		}
	}

	if ( player >= 0 && players[player]->hud.weapon )
	{
		if ( players[player]->hud.weapon->skill[0] != 0 )   //HUDWEAPON_CHOP.
		{
			return; //Can't cast spells while attacking.
		}
	}

	if ( cast_animation[player].active || cast_animation[player].active_spellbook )
	{
		//Already casting spell.
		if ( player >= 0 && players[player]->isLocalPlayer() )
		{
			if ( cast_animation[player].spellWaitingAttackInput() )
			{
				if ( multiplayer == SINGLE )
				{
					// refund some spent mana
					int refund = (cast_animation[player].mana_cost - cast_animation[player].mana_left) / 2;
					players[player]->entity->modMP(refund);
				}
				spellcastingAnimationManager_deactivate(&cast_animation[player]);
				messagePlayer(player, MESSAGE_COMBAT, Language::get(6496));
				playSoundEntityLocal(players[player]->entity, 163, 64);
			}
		}
		return;
	}

	if ( player > -1 )
	{
		if ( stats[player]->defending )
		{
			messagePlayer(player, MESSAGE_MISC, Language::get(407));
			return;
		}
		if ( spell_isChanneled(spell))
		{
			bool removedSpell = false;
			node_t* nextnode;
			for (node = channeledSpells[player].first; node; node = nextnode)
			{
				nextnode = node->next;
				spell_t* spell_search = (spell_t*)node->element;
				if (spell_search->ID == spell->ID)
				{
					//list_RemoveNode(node);
					//node = NULL;

					if ( multiplayer != CLIENT )
					{
						// 02/12/20 - BP
						// spell_search refers to actual spell definitions for client, like spell_light* 
						// server uses copies of spell elements, so it works as intended
						// clients don't read spell_search->sustain status to know when to stop anyway
						spell_search->sustain = false; 
					}

					//if (spell->magic_effects)
					//	list_RemoveNode(spell->magic_effects);
					messagePlayer(player, MESSAGE_COMBAT, Language::get(408), spell->getSpellName());
					if (multiplayer == CLIENT)
					{
						list_RemoveNode(node);
						node = nullptr;
						strcpy( (char*)net_packet->data, "UNCH");
						net_packet->data[4] = clientnum;
						SDLNet_Write32(spell->ID, &net_packet->data[5]);
						net_packet->address.host = net_server.host;
						net_packet->address.port = net_server.port;
						net_packet->len = 9;
						sendPacketSafe(net_sock, -1, net_packet, 0);
					}
					removedSpell = true;
				}
			}
			if ( removedSpell )
			{
				return;
			}
			if ( spell->ID == SPELL_VAMPIRIC_AURA && player >= 0 && client_classes[player] == CLASS_ACCURSED &&
				stats[player]->getEffectActive(EFF_VAMPIRICAURA) && players[player]->entity->playerVampireCurse == 1 )
			{
				if ( multiplayer == CLIENT )
				{
					//messagePlayer(player, Language::get(408), spell->getSpellName());
					strcpy((char*)net_packet->data, "VAMP");
					net_packet->data[4] = clientnum;
					SDLNet_Write32(spell->ID, &net_packet->data[5]);
					net_packet->address.host = net_server.host;
					net_packet->address.port = net_server.port;
					net_packet->len = 9;
					sendPacketSafe(net_sock, -1, net_packet, 0);
					return;
				}
				else
				{
					messagePlayerColor(player, MESSAGE_STATUS, uint32ColorGreen, Language::get(3241));
					messagePlayerColor(player, MESSAGE_HINT, uint32ColorGreen, Language::get(3242));
					//messagePlayer(player, Language::get(408), spell->getSpellName());
					caster->setEffect(EFF_VAMPIRICAURA, true, 1, false); // apply 1 tick countdown to finish effect.
					caster->playerVampireCurse = 2; // cured.
					steamAchievement("BARONY_ACH_REVERSE_THIS_CURSE");
					playSoundEntity(caster, 402, 128);
					createParticleDropRising(caster, 174, 1.0);
					serverSpawnMiscParticles(caster, PARTICLE_EFFECT_RISING_DROP, 174);
					return;
				}
			}
		}
	}

	int magiccost = 0;
	//Entity *entity = NULL;
	//node_t *node = spell->elements->first;

	Stat* stat = caster->getStats();
	if ( !stat )
	{
		return;
	}

	// Entity cannot cast Spells while Paralyzed or Asleep
	if ( stat->getEffectActive(EFF_PARALYZED) || stat->getEffectActive(EFF_ASLEEP) )
	{
		return;
	}

	// Calculate the cost of the Spell for Singleplayer
	if ( spell->ID == SPELL_FORCEBOLT && skillCapstoneUnlocked(player, PRO_SPELLCASTING) )
	{
		// Reaching Spellcasting capstone makes forcebolt free
		magiccost = 0;
	}
	else
	{
		magiccost = getCostOfSpell(spell, caster);
	}

	if ( player >= 0 )
	{
		int goldCost = getGoldCostOfSpell(spell, player);
		if ( goldCost > 0 )
		{
			if ( goldCost > stat->GOLD )
			{
				if ( players[player]->isLocalPlayer() )
				{
					playSound(563, 64);
				}
				messagePlayer(player, MESSAGE_COMBAT, Language::get(6622));
				return;
			}
			else if ( goldCost > 0 )
			{
				stat->GOLD -= goldCost;
				stat->GOLD = std::max(0, stat->GOLD);
			}
		}
	}

	if ( caster->behavior == &actPlayer && stat->type == VAMPIRE )
	{
		// allow overexpending.
	}
	else if ( magiccost > stat->MP )
	{
		if (player >= 0)
		{
		    //TODO: Allow overexpending at the cost of extreme danger?
		    // (maybe an immensely powerful tree of magic actually likes this --
		    //  using your life-force to power spells instead of mana)
			messagePlayer(player, MESSAGE_MISC, Language::get(375));
			if ( players[player]->isLocalPlayer() )
			{
				playSound(563, 64);
				if ( players[player]->magic.noManaProcessedOnTick == 0 )
				{
					players[player]->magic.flashNoMana();
				}
			}
		}
		return;
	}
	if (magiccost < 0)
	{
		if (player >= 0)
		{
			messagePlayer(player, MESSAGE_DEBUG, "Error: Invalid spell. Mana cost is negative?");
		}
		return;
	}

	//Hand the torch off to the spell animator. And stuff. Stuff. I mean spell animation handler thingymabobber.
	fireOffSpellAnimation(&cast_animation[player], caster->getUID(), spell, usingSpellbook);

	//castSpell(caster, spell); //For now, do this while the spell animations are worked on.
}

int getSpellcastingAbilityFromUsingSpellbook(spell_t* spell, Entity* caster, Stat* casterStats)
{
	if ( !casterStats || !spell ) 
	{ 
		return 0;
	}

	int spellcastingAbility = std::min(std::max(0, casterStats->getModifiedProficiency(PRO_SPELLCASTING) + statGetINT(casterStats, caster)), 100);


	// penalty for not knowing spellbook. e.g 40 spellcasting, 80 difficulty = 40% more chance to fumble/use mana.
	if ( spellcastingAbility >= SKILL_LEVEL_BASIC )
	{
		spellcastingAbility = std::max(10, spellcastingAbility - spell->difficulty);
	}
	else
	{
		spellcastingAbility = std::max(0, spellcastingAbility - spell->difficulty);
	}
	if ( casterStats->shield && (casterStats->shield->beatitude < 0 && !shouldInvertEquipmentBeatitude(casterStats)) )
	{
		if ( casterStats->shield->beatitude == -1 )
		{
			spellcastingAbility = std::min(30, spellcastingAbility); // 70% chance to use more mana at least
		}
		else
		{
			spellcastingAbility = std::min(10, spellcastingAbility); // 90% chance to use more mana at least
		}
	}
	return spellcastingAbility;
}


bool isSpellcasterBeginner(int player, Entity* caster)
{
	if ( !caster && player < 0 )
	{
		return false;
	}
	Stat* myStats = nullptr;
	if ( !caster && player >= 0 )
	{
		myStats = stats[player];
		caster = players[player]->entity;
	}
	else
	{
		myStats = caster->getStats();
	}
	if ( !myStats )
	{
		return false;
	}
	else if ( caster && caster->behavior == &actMonster )
	{
		return false;
	}
	else if ( std::min(std::max(0, myStats->getModifiedProficiency(PRO_SPELLCASTING) + statGetINT(myStats, caster)), 100) < SPELLCASTING_BEGINNER )
	{
		return true; //The caster has lower spellcasting skill. Cue happy fun times.
	}
	return false;
}

bool isSpellcasterBeginnerFromSpellbook(int player, Entity* caster, Stat* stat, spell_t* spell, Item* spellbookItem)
{
	if ( player < 0 || !spell || !stat || !spellbookItem )
	{
		return false;
	}

	int spellcastingLvl = std::min(std::max(0, stat->getModifiedProficiency(PRO_SPELLCASTING) + statGetINT(stat, caster)), 100);
	bool newbie = false;

	if ( spellcastingLvl >= spell->difficulty || playerLearnedSpellbook(player, spellbookItem) )
	{
		// bypass newbie penalty since we're good enough to cast the spell.
		newbie = false;
	}
	else
	{
		newbie = true;
	}
	if ( spellbookItem->beatitude < 0 && !shouldInvertEquipmentBeatitude(stat) )
	{
		newbie = true;
	}
	return newbie;
}

int getSpellbookBonusPercent(Entity* caster, Stat* stat, Item* spellbookItem)
{
	if ( !spellbookItem || itemCategory(spellbookItem) != SPELLBOOK )
	{
		return 0;
	}
	int spellBookBonusPercent = (statGetINT(stat, caster) * 0.5);
	if ( spellbookItem->beatitude > 0
		|| (shouldInvertEquipmentBeatitude(stat) && spellbookItem->beatitude < 0) )
	{
		spellBookBonusPercent += abs(spellbookItem->beatitude) * 25;
	}
	return spellBookBonusPercent;
}

enum SpellTarget_t
{
	TARGET_NEUTRAL = 1,
	TARGET_ENEMY = 2,
	TARGET_FRIEND = 4
};
Entity* getSpellTarget(node_t* node, int radius, Entity* caster, bool targetCaster, SpellTarget_t target)
{
	if ( !node )
	{
		return nullptr;
	}
	Entity* entity = (Entity*)(node->element);
	if ( !entity )
	{
		return nullptr;
	}
	if ( entity == caster && !targetCaster )
	{
		return nullptr;
	}
	if ( entity->behavior != &actPlayer && entity->behavior != &actMonster )
	{
		return nullptr;
	}

	if ( entityDist(entity, caster) <= radius 
		&& ((target & TARGET_ENEMY && entity->checkEnemy(caster)
			|| (target & TARGET_NEUTRAL && !entity->checkEnemy(caster))
			|| (target & TARGET_FRIEND && entity->checkFriend(caster)))) )
	{
		return entity->getStats() ? entity : nullptr;
	}
	return nullptr;
}

bool CastSpellProps_t::setToMonsterCast(Entity* monster, int spellID)
{
	if ( !monster ) { return false; }
	if ( monster->behavior != &actMonster ) { return false; }
	caster_x = monster->x;
	caster_y = monster->y;

	real_t spellDist = 64.0;
	spell_t* spell = getSpellFromID(spellID);
	if ( spell )
	{
		spellDist = spell->distance;
	}
	spellDist += 16.0;

	if ( Entity* target = uidToEntity(monster->monsterTarget) )
	{
		Entity* ohit = hit.entity;
		real_t tangent = atan2(target->y - monster->y, target->x - monster->x);
		real_t dist = lineTraceTarget(monster, monster->x, monster->y, tangent, spellDist, 0, false, target);
		if ( hit.entity == target )
		{
			target_x = target->x;
			target_y = target->y;
			if ( spell && spell->rangefinder == RANGEFINDER_TOUCH )
			{
				targetUID = target->getUID();
			}
		}
		else
		{
			if ( spell && spell->rangefinder == RANGEFINDER_TOUCH )
			{
				hit.entity = ohit;
				return false;
			}
			target_x = caster_x + dist * cos(tangent);
			target_y = caster_y + dist * sin(tangent);
		}
		hit.entity = ohit;
		return true;
	}
	return false;
}

Entity* castSpell(Uint32 caster_uid, spell_t* spell, bool using_magicstaff, bool trap, bool usingSpellbook, CastSpellProps_t* castSpellProps)
{
	Entity* caster = uidToEntity(caster_uid);

	if (!caster || !spell)
	{
		//Need a spell and caster to cast a spell.
		return NULL;
	}

	Entity* result = NULL; //If the spell spawns an entity (like a magic light ball or a magic missile), it gets stored here and returned.
#define spellcasting std::min(std::max(0,stat->getModifiedProficiency(PRO_SPELLCASTING)+statGetINT(stat, caster)),100) //Shortcut!

	if (clientnum != 0 && multiplayer == CLIENT)
	{
		if ( caster->behavior == &actDeathGhost )
		{
			strcpy((char*)net_packet->data, "GHSP");
			net_packet->data[4] = clientnum;
			SDLNet_Write32(spell->ID, &net_packet->data[5]);
			net_packet->len = 9;
		}
		else
		{
			strcpy( (char*)net_packet->data, "SPEL" );
			net_packet->data[4] = clientnum;
			SDLNet_Write32(spell->ID, &net_packet->data[5]);
			if ( usingSpellbook )
			{
				net_packet->data[9] = 1;
			}
			else
			{
				net_packet->data[9] = 0;
			}
			if ( castSpellProps )
			{
				SDLNet_Write32(static_cast<Sint32>(castSpellProps->caster_x * 256.0), &net_packet->data[10]);
				SDLNet_Write32(static_cast<Sint32>(castSpellProps->caster_y * 256.0), &net_packet->data[14]);
				SDLNet_Write32(static_cast<Sint32>(castSpellProps->target_x * 256.0), &net_packet->data[18]);
				SDLNet_Write32(static_cast<Sint32>(castSpellProps->target_y * 256.0), &net_packet->data[22]);
				SDLNet_Write32(castSpellProps->targetUID, &net_packet->data[26]);
				net_packet->data[30] = castSpellProps->wallDir;
				net_packet->len = 31;
			}
			else
			{
				net_packet->len = 10;
			}
		}
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		sendPacketSafe(net_sock, -1, net_packet, 0);
		return NULL;
	}

	if (!spell->elements.first)
	{
		return NULL;
	}

	//node_t *node = spell->types->first;

#define PROPULSION_MISSILE 1
#define PROPULSION_MISSILE_TRIO 2
	int chance = 0;
	int propulsion = 0;
	int traveltime = 0;
	int magiccost = 0;
	int extramagic = 0; //Extra magic drawn in from the caster being a newbie.
	int extramagic_to_use = 0; //Instead of doing element->mana (which causes bugs), this is an extra factor in the mana equations. Pumps extra mana into elements from extramagic.
	spell_t* channeled_spell = NULL; //Pointer to the spell if it's a channeled spell. For the purpose of giving it its node in the channeled spell list.
	node_t* node = spell->elements.first;

	Stat* stat = caster->getStats();

	int player = -1;
	for (int i = 0; i < MAXPLAYERS; ++i)
	{
		if ( players[i] && caster && (caster == players[i]->entity) )
		{
			player = i; //Set the player.
		}
	}

	bool newbie = false;
	bool overdrewIntoHP = false;
	bool playerCastingFromKnownSpellbook = false;
	int spellBookBonusPercent = 0;
	int spellBookBeatitude = 0;
	ItemType spellbookType = WOODEN_SHIELD;
	bool sustainedSpell = false;
	auto findSpellDef = ItemTooltips.spellItems.find(spell->ID);
	if ( findSpellDef != ItemTooltips.spellItems.end() )
	{
		sustainedSpell = (findSpellDef->second.spellType == ItemTooltips_t::SpellItemTypes::SPELL_TYPE_SELF_SUSTAIN);
	}
	int oldMP = caster->getMP();
	if ( !using_magicstaff && !trap && stat )
	{
		newbie = isSpellcasterBeginner(player, caster);

		if ( usingSpellbook && stat->shield && itemCategory(stat->shield) == SPELLBOOK )
		{
			spellbookType = stat->shield->type;
			spellBookBeatitude = stat->shield->beatitude;
			spellBookBonusPercent += getSpellbookBonusPercent(caster, stat, stat->shield);
			if ( spellcasting >= spell->difficulty || playerLearnedSpellbook(player, stat->shield) )
			{
				// bypass newbie penalty since we're good enough to cast the spell.
				playerCastingFromKnownSpellbook = true;
			}

			if ( stat->shield->beatitude < 0 && !shouldInvertEquipmentBeatitude(stat) )
			{
				playerCastingFromKnownSpellbook = false;
			}

			newbie = isSpellcasterBeginnerFromSpellbook(player, caster, stat, spell, stat->shield);
		}

		/*magiccost = getCostOfSpell(spell);
		if (magiccost < 0) {
			if (player >= 0)
				messagePlayer(player, "Error: Invalid spell. Mana cost is negative?");
			return NULL;
		}*/
		int prevHP = caster->getHP();
		if ( multiplayer == SINGLE )
		{
			if ( spell->ID == SPELL_FORCEBOLT && skillCapstoneUnlocked(player, PRO_SPELLCASTING) )
			{
				magiccost = 0;
			}
			else
			{
				magiccost = cast_animation[player].mana_left;
			}

			caster->drainMP(magiccost, false);
		}
		else // Calculate the cost of the Spell for Multiplayer
		{
			if ( spell->ID == SPELL_FORCEBOLT && skillCapstoneUnlocked(player, PRO_SPELLCASTING) )
			{
				// Reaching Spellcasting capstone makes Forcebolt free
				magiccost = 0;
			}
			else
			{
				int goldCost = getGoldCostOfSpell(spell, player);
				if ( goldCost > 0 )
				{
					if ( goldCost > stat->GOLD )
					{
						playSoundEntity(caster, 163, 128);
						messagePlayer(player, MESSAGE_COMBAT, Language::get(6622));
						return nullptr;
					}
					else if ( goldCost > 0 )
					{
						stat->GOLD -= goldCost;
						stat->GOLD = std::max(0, stat->GOLD);

						if ( player >= 1 )
						{
							// send the client info on the gold it picked up
							strcpy((char*)net_packet->data, "GOLD");
							SDLNet_Write32(stats[player]->GOLD, &net_packet->data[4]);
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 8;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}
					}
				}

				magiccost = getCostOfSpell(spell, caster);
				if ( magiccost > stat->MP )
				{
					// damage sound/effect due to overdraw.
					if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
					{
						strcpy((char*)net_packet->data, "SHAK");
						net_packet->data[4] = 10; // turns into .1
						net_packet->data[5] = 10;
						net_packet->address.host = net_clients[player - 1].host;
						net_packet->address.port = net_clients[player - 1].port;
						net_packet->len = 6;
						sendPacketSafe(net_sock, -1, net_packet, player - 1);
						playSoundPlayer(player, 28, 92);
					}
					else if ( player >= 0 && players[player]->isLocalPlayer() )
					{
						cameravars[player].shakex += 0.1;
						cameravars[player].shakey += 10;
						playSoundPlayer(player, 28, 92);
					}
				}
				caster->drainMP(magiccost);
			}
		}

		if ( multiplayer != CLIENT )
		{
			if ( caster->behavior == &actPlayer && stat->playerRace == RACE_INSECTOID && stat->stat_appearance == 0 )
			{
				if ( !achievementObserver.playerAchievements[caster->skill[2]].gastricBypass )
				{
					int fullCostOfSpell = getCostOfSpell(spell, caster);
					achievementObserver.playerAchievements[caster->skill[2]].gastricBypassSpell = std::make_pair(0, 0);
					if ( stat->MP <= 5 && stat->MP + fullCostOfSpell > 5 )
					{
						achievementObserver.playerAchievements[caster->skill[2]].gastricBypassSpell = std::make_pair(spell->ID, caster->ticks);
					}
				}
			}
		}

		if ( caster->getHP() < prevHP )
		{
			overdrewIntoHP = true;
		}
	}

	if ( newbie && stat )
	{
		//So This wizard is a newbie.

		//First, drain some extra mana maybe.
		int chance = local_rng.rand() % 10;
		int spellcastingAbility = spellcasting;
		if ( usingSpellbook )
		{
			spellcastingAbility = getSpellcastingAbilityFromUsingSpellbook(spell, caster, stat);
		}
		if (chance >= spellcastingAbility / 10)   //At skill 20, there's an 80% chance you'll use extra mana. At 70, there's a 30% chance.
		{
			extramagic = local_rng.rand() % (300 / (spellcastingAbility + 1)); //Use up extra mana. More mana used the lower your spellcasting skill.
			extramagic = std::min<real_t>(extramagic, stat->MP / 10); //To make sure it doesn't draw, say, 5000 mana. Cause dammit, if you roll a 1 here...you're doomed.
			caster->drainMP(extramagic);
		}

		if ( caster->behavior == &actPlayer && sustainedSpell )
		{
			players[caster->skill[2]]->mechanics.sustainedSpellIncrementMP(oldMP - stat->MP);
		}

		bool fizzleSpell = false;
		chance = local_rng.rand() % 10;
		if ( chance >= spellcastingAbility / 10 )
		{
			fizzleSpell = true;
		}

		// Check for natural monster spells - we won't fizzle those.
		if ( caster->behavior == &actPlayer )
		{
			if ( spellIsNaturallyLearnedByRaceOrClass(*caster, *stat, spell->ID) )
			{
				fizzleSpell = false;
			}
		}

		//Now, there's a chance they'll fumble the spell.
		if ( fizzleSpell )
		{
			if ( local_rng.rand() % 3 == 1 )
			{
				//Fizzle the spell.
				//TODO: Cool effects.
				playSoundEntity(caster, 163, 128);
				if ( player >= 0 )
				{
					messagePlayer(player, MESSAGE_COMBAT, Language::get(409));
				}
				if ( usingSpellbook && stat->shield && itemCategory(stat->shield) == SPELLBOOK 
					&& (stat->shield->beatitude < 0 && !shouldInvertEquipmentBeatitude(stat)) )
				{
					Status oldStatus = stat->shield->status;
					caster->degradeArmor(*stat, *(stat->shield), 4);
					if ( stat->shield->status < oldStatus )
					{
						if ( player >= 0 )
						{
							Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_SPELLBOOK_CAST_DEGRADES, stat->shield->type, 1);
						}
					}
					if ( stat->shield->status == BROKEN )
					{
						Item* toBreak = stat->shield;
						consumeItem(toBreak, player);
					}
				}
				if ( player >= 0 )
				{
					if ( !using_magicstaff && !trap )
					{
						if ( !usingSpellbook )
						{
							Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_SPELL_FAILURES, SPELL_ITEM, 1, false, spell->ID);
							Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_SPELL_FIZZLES_RUN, "memorized", 1);
						}
						else if ( usingSpellbook )
						{
							if ( items[spellbookType].category == SPELLBOOK )
							{
								Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_SPELL_FAILURES, spellbookType, 1);
								Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_SPELLBOOK_FIZZLES_RUN, "spellbook casting", 1);
							}
						}
					}
				}
				return NULL;
			}
		}
	}

	if ( caster->behavior == &actPlayer && sustainedSpell )
	{
		players[caster->skill[2]]->mechanics.sustainedSpellIncrementMP(oldMP - stat->MP);
	}

	//Check if the bugger is levitating.
	bool levitating = false;
	if (!trap)
	{
		levitating = isLevitating(stat);
	}

	//Water walking boots
	bool waterwalkingboots = false;
	if (!trap)
	{
		if ( player >= 0 && skillCapstoneUnlocked(player, PRO_SWIMMING) )
		{
			waterwalkingboots = true;
		}
		if ( stat && stat->shoes != NULL )
		{
			if (stat->shoes->type == IRON_BOOTS_WATERWALKING )
			{
				waterwalkingboots = true;
			}
		}
	}

	//Check if swimming.
	if (!waterwalkingboots && !levitating && !trap && player >= 0)
	{
		bool swimming = false;
		if ( player >= 0 && players[player] && players[player]->entity)
		{
			int x = std::min<int>(std::max<int>(0, floor(caster->x / 16)), map.width - 1);
			int y = std::min<int>(std::max<int>(0, floor(caster->y / 16)), map.height - 1);
			if ( swimmingtiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] || lavatiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] )
			{
				swimming = true;
			}
		}
		if (swimming)
		{
			//Can't cast spells while swimming if not levitating or water walking.
			if (player >= 0)
			{
				messagePlayer(player, MESSAGE_MISC, Language::get(410));
			}
			return nullptr;
		}
	}

	//Right. First, grab the root element, which is what determines the delivery system.
	//spellElement_t *element = (spellElement_t *)spell->elements->first->element;
	spellElement_t* const element = (spellElement_t*)node->element;
	spellElement_t* const innerElement = element->elements.first ? (spellElement_t*)(element->elements.first->element) : nullptr;
	if (element)
	{
		extramagic_to_use = 0;
		/*if (magiccost > stat->MP) {
			if (player >= 0)
				messagePlayer(player, "Insufficient mana!"); //TODO: Allow overexpending at the cost of extreme danger? (maybe an immensely powerful tree of magic actually likes this -- using your life-force to power spells instead of mana)
			return NULL;
		}*/

		if (extramagic > 0)
		{
			//Extra magic. Pump it in here?
			chance = local_rng.rand() % 5;
			if (chance == 1)
			{
				//Use some of that extra magic in this element.
				int amount = local_rng.rand() % extramagic;
				extramagic -= amount;
				extramagic_to_use += amount;
			}
		}

		if (!strcmp(element->element_internal_name, spellElement_missile.element_internal_name))
		{
			//Set the propulsion to missile.
			propulsion = PROPULSION_MISSILE;
			traveltime = element->duration;
			if (newbie)
			{
				//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
				chance = local_rng.rand() % 10;
				if (chance >= spellcasting / 10)
				{
					traveltime -= local_rng.rand() % (1000 / (spellcasting + 1));
				}
				if (traveltime < 30)
				{
					traveltime = 30;    //Range checking.
				}
			}
			traveltime += (((element->mana + extramagic_to_use) - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->duration;
			if ( caster->behavior == &actBoulder )
			{
				traveltime /= 4; // lava boulder casting.
			}
		}
		else if ( !strcmp(element->element_internal_name, spellElement_missile_trio.element_internal_name) )
		{
			//Set the propulsion to missile.
			propulsion = PROPULSION_MISSILE_TRIO;
			traveltime = element->duration;
			if ( newbie )
			{
				//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
				chance = local_rng.rand() % 10;
				if ( chance >= spellcasting / 10 )
				{
					traveltime -= local_rng.rand() % (1000 / (spellcasting + 1));
				}
				if ( traveltime < 30 )
				{
					traveltime = 30;    //Range checking.
				}
			}
			traveltime += (((element->mana + extramagic_to_use) - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->duration;
		}
		/*else if ( !strcmp(element->element_internal_name, "spell_element_propulsion_floor_tile") )
		{
			if ( castSpellProps )
			{
				int sprite = -1;
				if ( !strcmp(innerElement->element_internal_name, "spell_ice_wave") )
				{
					sprite = 1720;
				}
				if ( sprite >= 0 )
				{
					real_t tangent = atan2(castSpellProps->target_y - castSpellProps->caster_y, castSpellProps->target_x - castSpellProps->caster_x);
					real_t tx = castSpellProps->target_x + castSpellProps->distanceOffset * cos(tangent);
					real_t ty = castSpellProps->target_y + castSpellProps->distanceOffset * sin(tangent);
					int duration = element->duration + innerElement->duration;
					Entity* floorMagic = createFloorMagic(ParticleTimerEffect_t::EffectType::EFFECT_ICE_WAVE, sprite, tx, ty, 7.5, tangent, element->duration + innerElement->duration);
					floorMagic->actmagicDelayMove = castSpellProps->elementIndex * 5;
					if ( floorMagic->actmagicDelayMove > 0 )
					{
						floorMagic->flags[INVISIBLE] = true;
						floorMagic->flags[UPDATENEEDED] = false;
					}
					floorMagic->yaw = ((local_rng.rand() % 32) / 32.0) * 2 * PI;
					node_t* node = list_AddNodeFirst(&floorMagic->children);
					node->element = copySpell(spell, castSpellProps->elementIndex);
					((spell_t*)node->element)->caster = caster->getUID();
					node->deconstructor = &spellDeconstructor;
					node->size = sizeof(spell_t);
					castSpellProps->distanceOffset += 4.0;
				}
			}
		}*/
		else if (!strcmp(element->element_internal_name, spellElement_light.element_internal_name))
		{
            if (using_magicstaff) {
                for (auto node = map.entities->first; node != nullptr; node = node->next) {
                    auto entity = (Entity*)node->element;
                    if (entity->behavior == &actMagiclightBall && entity->sprite == 174) {
                        if (entity->parent == caster->getUID()) {
                            auto spell = (spell_t*)entity->children.first->element;
							if ( spell && spell->magicstaff )
							{
								spell->sustain = false; // remove other lightballs to prevent lightball insanity
							}
                        }
                    }
                }
            }
			Entity* entity = newEntity(174, 1, map.entities, nullptr); // black magic ball
			entity->parent = caster->getUID();
			entity->x = caster->x;
			entity->y = caster->y;
			entity->z = -5.5 + ((-6.5f + -4.5f) / 2) * sin(0);
			entity->skill[7] = -5; //Base z.
			entity->sizex = 1;
			entity->sizey = 1;
			entity->yaw = caster->yaw;
			entity->flags[UPDATENEEDED] = true;
			entity->flags[PASSABLE] = true;
			entity->behavior = &actMagiclightBall;
			entity->skill[4] = entity->x; //Store what x it started shooting out from the player at.
			entity->skill[5] = entity->y; //Store what y it started shooting out from the player at.
			entity->skill[12] = (element->duration);// *(((element->mana + extramagic_to_use) / static_cast<double>(element->base_mana)) * element->overload_multiplier)); //How long this thing lives.
			node_t* spellnode = list_AddNodeLast(&entity->children);
			spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
			channeled_spell = (spell_t*)(spellnode->element);
			spellnode->size = sizeof(spell_t);
			((spell_t*)spellnode->element)->caster = caster->getUID();
			if ( using_magicstaff )
			{
				((spell_t*)spellnode->element)->magicstaff = true;
			}
			spellnode->deconstructor = &spellDeconstructor;
			if (using_magicstaff || trap)
			{
				entity->skill[12] = MAGICSTAFF_LIGHT_DURATION; //TODO: Grab the duration from the magicstaff or trap?
			}
			else
			{
				entity->skill[12] /= getCostOfSpell((spell_t*)spellnode->element);
			}
			((spell_t*)spellnode->element)->channel_duration = entity->skill[12];  //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
			result = entity;

			playSoundEntity(entity, 165, 128 );
			if ( player >= 0 )
			{
				if ( !achievementStatusStrobe[player] )
				{
					achievementStrobeVec[player].push_back(ticks);
					if ( achievementStrobeVec[player].size() > 20 )
					{
						achievementStrobeVec[player].erase(achievementStrobeVec[player].begin());
					}
					Uint32 timeDiff = achievementStrobeVec[player].back() - achievementStrobeVec[player].front();
					if ( achievementStrobeVec[player].size() == 20 )
					{
						if ( timeDiff < 60 * TICKS_PER_SECOND )
						{
							achievementStatusStrobe[player] = true;
							steamAchievementClient(player, "BARONY_ACH_STROBE");
						}
					}
				}
			}
		}
		else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_DEEP_SHADE].element_internal_name) )
		{
			if ( using_magicstaff ) {
				for ( auto node = map.entities->first; node != nullptr; node = node->next ) {
					auto entity = (Entity*)node->element;
					if ( entity->behavior == &actMagiclightBall && entity->sprite == 1800 ) {
						if ( entity->parent == caster->getUID() ) {
							auto spell = (spell_t*)entity->children.first->element;
							if ( spell && spell->magicstaff )
							{
								spell->sustain = false; // remove other lightballs to prevent lightball insanity
							}
						}
					}
				}
			}
			Entity* entity = newEntity(1800, 1, map.entities, nullptr); // black magic ball
			entity->parent = caster->getUID();
			entity->x = caster->x;
			entity->y = caster->y;
			entity->z = -5.5 + ((-6.5f + -4.5f) / 2) * sin(0);
			entity->skill[7] = -5; //Base z.
			entity->fskill[1] = entity->skill[7] - entity->z;
			entity->fskill[2] = 0.0;
			entity->sizex = 1;
			entity->sizey = 1;
			entity->yaw = caster->yaw;
			entity->flags[UPDATENEEDED] = true;
			entity->flags[PASSABLE] = true;
			entity->behavior = &actMagiclightBall;
			entity->skill[4] = entity->x; //Store what x it started shooting out from the player at.
			entity->skill[5] = entity->y; //Store what y it started shooting out from the player at.
			entity->skill[12] = (element->duration);// *(((element->mana + extramagic_to_use) / static_cast<double>(element->base_mana)) * element->overload_multiplier)); //How long this thing lives.
			node_t* spellnode = list_AddNodeLast(&entity->children);
			spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
			channeled_spell = (spell_t*)(spellnode->element);
			spellnode->size = sizeof(spell_t);
			((spell_t*)spellnode->element)->caster = caster->getUID();
			if ( using_magicstaff )
			{
				((spell_t*)spellnode->element)->magicstaff = true;
			}
			spellnode->deconstructor = &spellDeconstructor;
			if ( using_magicstaff || trap )
			{
				entity->skill[12] = MAGICSTAFF_LIGHT_DURATION; //TODO: Grab the duration from the magicstaff or trap?
			}
			else
			{
				entity->skill[12] /= getCostOfSpell((spell_t*)spellnode->element);
			}
			((spell_t*)spellnode->element)->channel_duration = entity->skill[12];  //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
			result = entity;

			playSoundEntity(entity, 165, 128);
		}
		else if (!strcmp(element->element_internal_name, spellElement_invisible.element_internal_name))
		{
			int duration = element->duration;
			//duration += (((element->mana + extramagic_to_use) - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->duration;
			node_t* spellnode = list_AddNodeLast(&caster->getStats()->magic_effects);
			spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
			channeled_spell = (spell_t*)(spellnode->element);
			channeled_spell->magic_effects_node = spellnode;
			spellnode->size = sizeof(spell_t);
			((spell_t*)spellnode->element)->caster = caster->getUID();
			spellnode->deconstructor = &spellDeconstructor;
			//if (newbie)
			//{
			//	//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
			//	chance = local_rng.rand() % 10;
			//	if (chance >= spellcasting / 10)
			//	{
			//		duration -= local_rng.rand() % (1000 / (spellcasting + 1));
			//	}
			//	if (duration < 180)
			//	{
			//		duration = 180;    //Range checking.
			//	}
			//}
			duration /= getCostOfSpell((spell_t*)spellnode->element);
			channeled_spell->channel_duration = duration; //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
			caster->setEffect(EFF_INVISIBLE, true, duration, false);
			bool isPlayer = false;
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i] && caster && (caster == players[i]->entity) )
				{
					serverUpdateEffects(i);
					isPlayer = true;
				}
			}

			if ( isPlayer )
			{
				for ( node_t* node = map.creatures->first; node != nullptr; node = node->next )
				{
					Entity* creature = (Entity*)node->element;
					if ( creature && creature->behavior == &actMonster && creature->monsterTarget == caster->getUID() )
					{
						if ( !creature->isBossMonster() )
						{
							//Abort if invalid creature (boss, shopkeep, etc).
							real_t dist = entityDist(caster, creature);
							if ( dist > STRIKERANGE * 3 )
							{
								// lose track of invis target.
								creature->monsterReleaseAttackTarget();
							}
						}
					}
				}
			}

			playSoundEntity(caster, 166, 128 );
			spawnMagicEffectParticles(caster->x, caster->y, caster->z, 174);
		}
		else if (!strcmp(element->element_internal_name, spellElement_levitation.element_internal_name))
		{
			int duration = element->duration;
			//duration += (((element->mana + extramagic_to_use) - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->duration;
			node_t* spellnode = list_AddNodeLast(&caster->getStats()->magic_effects);
			spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
			channeled_spell = (spell_t*)(spellnode->element);
			channeled_spell->magic_effects_node = spellnode;
			spellnode->size = sizeof(spell_t);
			((spell_t*)spellnode->element)->caster = caster->getUID();
			spellnode->deconstructor = &spellDeconstructor;
			//if (newbie)
			//{
			//	//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
			//	chance = local_rng.rand() % 10;
			//	if (chance >= spellcasting / 10)
			//	{
			//		duration -= local_rng.rand() % (1000 / (spellcasting + 1));
			//	}
			//	if (duration < 180)
			//	{
			//		duration = 180;    //Range checking.
			//	}
			//}
			duration /= getCostOfSpell((spell_t*)spellnode->element);
			channeled_spell->channel_duration = duration; //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
			caster->setEffect(EFF_LEVITATING, true, duration, false);
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i] && caster && (caster == players[i]->entity) )
				{
					serverUpdateEffects(i);
				}
			}

			playSoundEntity(caster, 178, 128 );
			spawnMagicEffectParticles(caster->x, caster->y, caster->z, 170);
		}
		else if (!strcmp(element->element_internal_name, spellElement_teleportation.element_internal_name))
		{
			if ( caster->behavior == &actDeathGhost )
			{
				Compendium_t::Events_t::eventUpdateMonster(caster->skill[2], Compendium_t::CPDM_GHOST_TELEPORTS, caster, 1);
				auto& ghost = players[caster->skill[2]]->ghost;
				int tx = ghost.spawnX;
				int ty = ghost.spawnY;
				Entity* target = nullptr;
				if ( ghost.teleportToPlayer >= MAXPLAYERS )
				{
					ghost.teleportToPlayer = -1;
					tx = ghost.startRoomX;
					ty = ghost.startRoomY;
				}
				else
				{
					++ghost.teleportToPlayer;
					while ( ghost.teleportToPlayer >= 0 && ghost.teleportToPlayer < MAXPLAYERS )
					{
						if ( ghost.teleportToPlayer != caster->skill[2] && Player::getPlayerInteractEntity(ghost.teleportToPlayer) )
						{
							target = Player::getPlayerInteractEntity(ghost.teleportToPlayer);
							tx = static_cast<int>(target->x) / 16;
							ty = static_cast<int>(target->y) / 16;
							break;
						}
						++ghost.teleportToPlayer;
					}
					if ( !target )
					{
						if ( ghost.teleportToPlayer >= MAXPLAYERS )
						{
							tx = ghost.spawnX;
							ty = ghost.spawnY;
						}
					}
				}

				Entity* spellTimer = createParticleTimer(caster, 1, 593);
				spellTimer->particleTimerPreDelay = 0; // wait x ticks before animation.
				spellTimer->particleTimerEndAction = PARTICLE_EFFECT_GHOST_TELEPORT; // teleport behavior of timer.
				spellTimer->particleTimerEndSprite = 593; // sprite to use for end of timer function.
				spellTimer->particleTimerCountdownAction = 0;
				spellTimer->particleTimerCountdownSprite = -1;
				if ( target != nullptr )
				{
					spellTimer->particleTimerTarget = static_cast<Sint32>(target->getUID()); // get the target to teleport around.
				}
				spellTimer->particleTimerVariable1 = 1; // distance of teleport in tiles
				spellTimer->particleTimerVariable2 = (tx & 0xFFFF) << 16;
				spellTimer->particleTimerVariable2 |= ty & 0xFFFF;
				if ( multiplayer == SERVER )
				{
					serverSpawnMiscParticles(caster, PARTICLE_EFFECT_GHOST_TELEPORT, 593);
				}
			}
			else if ( caster->creatureShadowTaggedThisUid != 0 )
			{
				Entity* entityToTeleport = uidToEntity(caster->creatureShadowTaggedThisUid);
				if ( entityToTeleport )
				{
					caster->teleportAroundEntity(entityToTeleport, 3, 0);
					if ( caster->behavior == &actPlayer )
					{
						achievementObserver.addEntityAchievementTimer(caster, AchievementObserver::BARONY_ACH_OHAI_MARK, 100, true, 0);
					}
				}
				else
				{
					caster->teleportRandom();
				}
			}
			else
			{
				caster->teleportRandom();
			}
		}
		else if ( spell->ID == SPELL_JUMP )
		{
			if ( caster && castSpellProps )
			{
				real_t oldx = caster->x;
				real_t oldy = caster->y;
				createParticleErupt(caster, 625);
				serverSpawnMiscParticles(caster, PARTICLE_EFFECT_ERUPT, 625);
				caster->x = castSpellProps->target_x;
				caster->y = castSpellProps->target_y;

				if ( caster->teleportAroundEntity(caster, 1, SPELL_JUMP) )
				{
					createParticleErupt(caster, 625);
					// teleport success.
					if ( multiplayer == SERVER )
					{
						serverSpawnMiscParticles(caster, PARTICLE_EFFECT_ERUPT, 625);
					}
				}
				else
				{
					caster->x = oldx;
					caster->y = oldy;
				}
			}
		}
		else if ( !strcmp(element->element_internal_name, spellElement_selfPolymorph.element_internal_name) )
		{
			if ( caster->behavior == &actPlayer )
			{
				spellEffectPolymorph(caster, caster, true, TICKS_PER_SECOND * 60 * 2); // 2 minutes.
			}
			else if ( caster->behavior == &actMonster )
			{
				spellEffectPolymorph(caster, caster, true);
			}
		}
		else if ( !strcmp(element->element_internal_name, spellElement_strike.element_internal_name) )
		{
			caster->attack(MONSTER_POSE_SPECIAL_WINDUP1, MAXCHARGE, nullptr); // this is server only, tells client to attack.
		}
		else if ( !strcmp(element->element_internal_name, spellElement_fear.element_internal_name) )
		{
			playSoundEntity(caster, 79, 128);
			playSoundEntity(caster, 405, 128);
			if ( caster->behavior == &actPlayer )
			{
				caster->setEffect(EFF_STUNNED, true, 35, true);
				caster->attack(MONSTER_POSE_SPECIAL_WINDUP2, 0, nullptr);
				//spawnMagicEffectParticles(caster->x, caster->y, caster->z, 174);
				int foundTarget = 0;
				if ( caster->behavior == &actPlayer )
				{
					messagePlayer(caster->skill[2], MESSAGE_COMBAT, Language::get(3437));
				}
				for ( node_t* node3 = map.creatures->first; node3 != nullptr; node3 = node3->next )
				{
					Entity* creature = (Entity*)node3->element;
					if ( creature && creature != caster && creature->behavior == &actMonster 
						&& !caster->checkFriend(creature) && entityDist(caster, creature) < TOUCHRANGE * 2 )
					{
						// check LOS
						Entity* ohit = hit.entity;
						real_t tangent = atan2(creature->y - caster->y, creature->x - caster->x);
						lineTraceTarget(caster, caster->x, caster->y, tangent, TOUCHRANGE * 2, 0, false, creature);
						if ( hit.entity == creature )
						{
							Entity* spellEntity = createParticleSapCenter(creature, caster, SPELL_FEAR, 864, 864);
							if ( spellEntity )
							{
								++foundTarget;
								spellEntity->skill[0] = 25; // duration
								spellEntity->skill[7] = caster->getUID();
							}
						}
						hit.entity = ohit;
					}
				}
				if ( foundTarget == 0 )
				{
					createParticleErupt(caster, 864);
					serverSpawnMiscParticles(caster, PARTICLE_EFFECT_ERUPT, 864);
					if ( caster->behavior == &actPlayer )
					{
						messagePlayer(caster->skill[2], MESSAGE_COMBAT, Language::get(3438));
					}
				}
				else
				{
					if ( !using_magicstaff && !trap )
					{
						if ( usingSpellbook )
						{
							Compendium_t::Events_t::eventUpdate(caster->skill[2], Compendium_t::CPDM_SPELL_TARGETS, SPELLBOOK_FEAR, foundTarget);
						}
						else
						{
							Compendium_t::Events_t::eventUpdate(caster->skill[2], Compendium_t::CPDM_SPELL_TARGETS, SPELL_ITEM, foundTarget, false, SPELL_FEAR);
						}
					}
				}
			}
			else if ( caster->behavior == &actMonster )
			{
				caster->setEffect(EFF_STUNNED, true, 35, true);
				caster->attack(MONSTER_POSE_SPECIAL_WINDUP2, 0, nullptr);
				int foundTarget = 0;
				for ( node_t* node3 = map.creatures->first; node3 != nullptr; node3 = node3->next )
				{
					Entity* creature = (Entity*)node3->element;
					if ( creature && creature != caster
						&& !caster->checkFriend(creature) && entityDist(caster, creature) < TOUCHRANGE * 2 )
					{
						// check LOS
						Entity* ohit = hit.entity;
						real_t tangent = atan2(creature->y - caster->y, creature->x - caster->x);
						lineTraceTarget(caster, caster->x, caster->y, tangent, TOUCHRANGE * 2, 0, false, creature);
						if ( hit.entity == creature )
						{
							Entity* spellEntity = createParticleSapCenter(creature, caster, SPELL_FEAR, 864, 864);
							if ( spellEntity )
							{
								++foundTarget;
								spellEntity->skill[0] = 25; // duration
								spellEntity->skill[7] = caster->getUID();
							}
						}
						hit.entity = ohit;
					}
				}
				if ( foundTarget == 0 )
				{
					createParticleErupt(caster, 864);
					serverSpawnMiscParticles(caster, PARTICLE_EFFECT_ERUPT, 864);
					if ( caster->behavior == &actPlayer )
					{
						messagePlayer(caster->skill[2], MESSAGE_COMBAT, Language::get(3438));
					}
				}
			}
		}
		else if (!strcmp(element->element_internal_name, spellElement_identify.element_internal_name))
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i] && caster && (caster == players[i]->entity) )
				{
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
					if (i != 0 && !players[i]->isLocalPlayer() )
					{
						//Tell the client to identify an item.
						strcpy((char*)net_packet->data, "IDEN");
						if ( usingSpellbook )
						{
							net_packet->data[4] = 1;
							net_packet->data[5] = static_cast<Uint8>(spellBookBeatitude);
						}
						else
						{
							net_packet->data[4] = 0;
							net_packet->data[5] = 0;
						}
						net_packet->address.host = net_clients[i - 1].host;
						net_packet->address.port = net_clients[i - 1].port;
						net_packet->len = 6;
						sendPacketSafe(net_sock, -1, net_packet, i - 1);
					}
					else
					{
						//Identify an item.
						if ( usingSpellbook )
						{
							GenericGUI[i].openGUI(GUI_TYPE_ITEMFX, nullptr, spellBookBeatitude, getSpellbookFromSpellID(SPELL_IDENTIFY), SPELL_IDENTIFY);
						}
						else
						{
							GenericGUI[i].openGUI(GUI_TYPE_ITEMFX, nullptr, 0, SPELL_ITEM, SPELL_IDENTIFY);
						}
					}
				}
			}

			playSoundEntity(caster, 167, 128 );
		}
		else if (!strcmp(element->element_internal_name, spellElement_removecurse.element_internal_name))
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i] && (caster == players[i]->entity) )
				{
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 169);
					if ( i != 0 && !players[i]->isLocalPlayer() )
					{
						//Tell the client to identify an item.
						strcpy((char*)net_packet->data, "CRCU");
						if ( usingSpellbook )
						{
							net_packet->data[4] = 1;
							net_packet->data[5] = static_cast<Uint8>(spellBookBeatitude);
						}
						else
						{
							net_packet->data[4] = 0;
							net_packet->data[5] = 0;
						}
						net_packet->address.host = net_clients[i - 1].host;
						net_packet->address.port = net_clients[i - 1].port;
						net_packet->len = 6;
						sendPacketSafe(net_sock, -1, net_packet, i - 1);
					}
					else
					{
						//Identify an item.
						if ( usingSpellbook )
						{
							GenericGUI[i].openGUI(GUI_TYPE_ITEMFX, nullptr, spellBookBeatitude, getSpellbookFromSpellID(SPELL_REMOVECURSE), SPELL_REMOVECURSE);
						}
						else
						{
							GenericGUI[i].openGUI(GUI_TYPE_ITEMFX, nullptr, 0, SPELL_ITEM, SPELL_REMOVECURSE);
						}
					}
				}
			}

			playSoundEntity(caster, 167, 128 );
		}
		else if ( spell->ID == SPELL_ALTER_INSTRUMENT
			|| spell->ID == SPELL_METALLURGY
			|| spell->ID == SPELL_GEOMANCY
			|| spell->ID == SPELL_FORGE_KEY
			|| spell->ID == SPELL_FORGE_JEWEL
			|| spell->ID == SPELL_ENHANCE_WEAPON
			|| spell->ID == SPELL_RESHAPE_WEAPON
			|| spell->ID == SPELL_ALTER_ARROW
			|| spell->ID == SPELL_PUNCTURE_VOID
			|| spell->ID == SPELL_ADORCISM
			|| spell->ID == SPELL_RESTORE
			|| spell->ID == SPELL_VANDALISE
			|| spell->ID == SPELL_DESECRATE
			|| spell->ID == SPELL_SANCTIFY
			|| spell->ID == SPELL_SANCTIFY_WATER
			|| spell->ID == SPELL_CLEANSE_FOOD
			|| spell->ID == SPELL_ADORCISE_INSTRUMENT )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i] && (caster == players[i]->entity) )
				{
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
					if ( i != 0 && !players[i]->isLocalPlayer() )
					{
						//Tell the client to identify an item.
						strcpy((char*)net_packet->data, "FXSP");
						if ( usingSpellbook )
						{
							net_packet->data[4] = 1;
							net_packet->data[5] = static_cast<Uint8>(spellBookBeatitude);
						}
						else
						{
							net_packet->data[4] = 0;
							net_packet->data[5] = 0;
						}
						SDLNet_Write32(spell->ID, &net_packet->data[6]);
						net_packet->address.host = net_clients[i - 1].host;
						net_packet->address.port = net_clients[i - 1].port;
						net_packet->len = 10;
						sendPacketSafe(net_sock, -1, net_packet, i - 1);
					}
					else
					{
						//Identify an item.
						if ( usingSpellbook )
						{
							GenericGUI[i].openGUI(GUI_TYPE_ITEMFX, nullptr, spellBookBeatitude, getSpellbookFromSpellID(spell->ID), spell->ID);
						}
						else
						{
							GenericGUI[i].openGUI(GUI_TYPE_ITEMFX, nullptr, 0, SPELL_ITEM, spell->ID);
						}
					}
				}
			}

			playSoundEntity(caster, 167, 128);
		}
		else if (!strcmp(element->element_internal_name, spellElement_magicmapping.element_internal_name))
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i] && caster && (caster == players[i]->entity) )
				{
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
					spell_magicMap(i);
				}
			}
			playSoundEntity(caster, 167, 128 );
		}
		else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_CONJURE_FOOD].element_internal_name) )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i] && caster && (caster == players[i]->entity) )
				{
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
					spell_magicMap(i);
				}
			}
			playSoundEntity(caster, 167, 128);
		}
		else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_NULL_MELEE].element_internal_name) )
		{
			if ( caster )
			{
				if ( Stat* casterStats = caster->getStats() )
				{
					Uint8 effectStrength = casterStats->getEffectActive(EFF_NULL_MELEE);
					if ( effectStrength == 0 )
					{
						if ( caster->setEffect(EFF_NULL_MELEE, (Uint8)3, 30 * TICKS_PER_SECOND, false) )
						{
							messagePlayerColor(caster->isEntityPlayer(),
								MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6473));
						}
					}
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
			playSoundEntity(caster, 166, 128);
		}
		else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_NULL_MAGIC].element_internal_name) )
		{
			if ( caster )
			{
				if ( Stat* casterStats = caster->getStats() )
				{
					Uint8 effectStrength = casterStats->getEffectActive(EFF_NULL_MAGIC);
					if ( effectStrength == 0 )
					{
						if ( caster->setEffect(EFF_NULL_MAGIC, (Uint8)3, 30 * TICKS_PER_SECOND, false) )
						{
							messagePlayerColor(caster->isEntityPlayer(),
								MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6475));
						}
					}
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
			playSoundEntity(caster, 166, 128);
		}
		else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_NULL_RANGED].element_internal_name) )
		{
			if ( caster )
			{
				if ( Stat* casterStats = caster->getStats() )
				{
					Uint8 effectStrength = casterStats->getEffectActive(EFF_NULL_RANGED);
					if ( effectStrength == 0 )
					{
						if ( caster->setEffect(EFF_NULL_RANGED, (Uint8)3, 30 * TICKS_PER_SECOND, false) )
						{
							messagePlayerColor(caster->isEntityPlayer(),
								MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6474));
						}
					}
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
			playSoundEntity(caster, 166, 128);
		}
		else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_BLESS_FOOD].element_internal_name) )
		{
			if ( caster )
			{
				if ( Stat* casterStats = caster->getStats() )
				{
					Uint8 effectStrength = casterStats->getEffectActive(EFF_BLESS_FOOD);
					if ( effectStrength == 0 )
					{
						if ( caster->setEffect(EFF_BLESS_FOOD, (Uint8)1, 30 * TICKS_PER_SECOND, false) )
						{
							messagePlayerColor(caster->isEntityPlayer(),
								MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6486));
						}
					}
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
			playSoundEntity(caster, 166, 128);
		}
		else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_MAGIC_WELL].element_internal_name) )
		{
			if ( caster )
			{
				if ( Stat* casterStats = caster->getStats() )
				{
					Uint8 effectStrength = casterStats->getEffectActive(EFF_MAGIC_WELL);
					if ( effectStrength == 0 )
					{
						if ( caster->setEffect(EFF_MAGIC_WELL, (Uint8)1, 30 * TICKS_PER_SECOND, false) )
						{
							messagePlayerColor(caster->isEntityPlayer(),
								MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6728));
						}
					}
					else
					{
						messagePlayerColor(caster->isEntityPlayer(),
							MESSAGE_HINT, makeColorRGB(255, 255, 255), Language::get(6727), spell->getSpellName());
					}
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
			playSoundEntity(caster, 166, 128);
		}
		else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_CRITICAL_SPELL].element_internal_name) )
		{
			if ( caster )
			{
				if ( Stat* casterStats = caster->getStats() )
				{
					Uint8 effectStrength = casterStats->getEffectActive(EFF_CRITICAL_SPELL);
					if ( effectStrength == 0 )
					{
						if ( caster->setEffect(EFF_CRITICAL_SPELL, (Uint8)1, 30 * TICKS_PER_SECOND, false) )
						{
							messagePlayerColor(caster->isEntityPlayer(),
								MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6729));
						}
					}
					else
					{
						messagePlayerColor(caster->isEntityPlayer(),
							MESSAGE_HINT, makeColorRGB(255, 255, 255), Language::get(6727), spell->getSpellName());
					}
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
			playSoundEntity(caster, 166, 128);
		}
		else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_ABSORB_MAGIC].element_internal_name) )
		{
			if ( caster )
			{
				if ( Stat* casterStats = caster->getStats() )
				{
					Uint8 effectStrength = casterStats->getEffectActive(EFF_ABSORB_MAGIC);
					if ( effectStrength == 0 )
					{
						if ( caster->setEffect(EFF_ABSORB_MAGIC, (Uint8)1, 30 * TICKS_PER_SECOND, false) )
						{
							messagePlayerColor(caster->isEntityPlayer(),
								MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6735));
						}
					}
					else
					{
						messagePlayerColor(caster->isEntityPlayer(),
							MESSAGE_HINT, makeColorRGB(255, 255, 255), Language::get(6727), spell->getSpellName());
					}
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
			playSoundEntity(caster, 166, 128);
		}
		else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_FLAME_CLOAK].element_internal_name) )
		{
			if ( caster )
			{
				if ( Stat* casterStats = caster->getStats() )
				{
					if ( caster->setEffect(EFF_FLAME_CLOAK, (Uint8)1, 30 * TICKS_PER_SECOND, false) )
					{
						messagePlayerColor(caster->isEntityPlayer(),
							MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6730));
					}
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
			playSoundEntity(caster, 166, 128);
		}
		else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_FLAME_SHIELD].element_internal_name) )
		{
			if ( caster )
			{
				if ( Stat* casterStats = caster->getStats() )
				{
					if ( caster->setEffect(EFF_FLAME_CLOAK, (Uint8)1, 30 * TICKS_PER_SECOND, false) )
					{
						messagePlayerColor(caster->isEntityPlayer(),
							MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6731));
					}
					caster->castOrbitingMagicMissile(SPELL_FIREBALL, 16.0, caster->yaw + 0 * PI / 3, 5 * TICKS_PER_SECOND);
					caster->castOrbitingMagicMissile(SPELL_FIREBALL, 16.0, caster->yaw + 2 * PI / 3, 5 * TICKS_PER_SECOND);
					caster->castOrbitingMagicMissile(SPELL_FIREBALL, 16.0, caster->yaw + 4 * PI / 3, 5 * TICKS_PER_SECOND);
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
			playSoundEntity(caster, 166, 128);
		}
		else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_PROF_NIMBLENESS].element_internal_name) )
		{
			if ( caster )
			{
				if ( Stat* casterStats = caster->getStats() )
				{
					caster->setEffect(EFF_NIMBLENESS, (Uint8)1, 30 * TICKS_PER_SECOND, false);
					messagePlayerColor(caster->isEntityPlayer(),
						MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6476));
					playSoundEntity(caster, 167, 128);
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
			for ( node_t* node = map.creatures->first; node && false; node = node->next )
			{
				if ( Entity* entity = getSpellTarget(node, HEAL_RADIUS, caster, false, TARGET_FRIEND) )
				{
					entity->setEffect(EFF_NIMBLENESS, (Uint8)1, 30 * TICKS_PER_SECOND, false);
					messagePlayerColor(entity->isEntityPlayer(),
						MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6476));
					playSoundEntity(entity, 167, 128);
				}
			}
		}
		else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_PROF_GREATER_MIGHT].element_internal_name) )
		{
			if ( caster )
			{
				if ( Stat* casterStats = caster->getStats() )
				{
					caster->setEffect(EFF_GREATER_MIGHT, (Uint8)1, 30 * TICKS_PER_SECOND, false);
					messagePlayerColor(caster->isEntityPlayer(),
						MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6477));
					playSoundEntity(caster, 167, 128);
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
			for ( node_t* node = map.creatures->first; node && false; node = node->next )
			{
				if ( Entity* entity = getSpellTarget(node, HEAL_RADIUS, caster, false, TARGET_FRIEND) )
				{
					entity->setEffect(EFF_GREATER_MIGHT, (Uint8)1, 30 * TICKS_PER_SECOND, false);
					messagePlayerColor(entity->isEntityPlayer(),
						MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6477));
					playSoundEntity(entity, 167, 128);
				}
			}
		}
		else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_PROF_COUNSEL].element_internal_name) )
		{
			if ( caster )
			{
				if ( Stat* casterStats = caster->getStats() )
				{
					caster->setEffect(EFF_COUNSEL, (Uint8)1, 30 * TICKS_PER_SECOND, false);
					messagePlayerColor(caster->isEntityPlayer(),
						MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6478));
					playSoundEntity(caster, 167, 128);
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
			for ( node_t* node = map.creatures->first; node && false; node = node->next )
			{
				if ( Entity* entity = getSpellTarget(node, HEAL_RADIUS, caster, false, TARGET_FRIEND) )
				{
					entity->setEffect(EFF_COUNSEL, (Uint8)1, 30 * TICKS_PER_SECOND, false);
					messagePlayerColor(entity->isEntityPlayer(),
						MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6478));
					playSoundEntity(entity, 167, 128);
				}
			}
		}
		else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_PROF_STURDINESS].element_internal_name) )
		{
			if ( caster )
			{
				if ( Stat* casterStats = caster->getStats() )
				{
					caster->setEffect(EFF_STURDINESS, (Uint8)1, 30 * TICKS_PER_SECOND, false);
					messagePlayerColor(caster->isEntityPlayer(),
						MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6479));
					playSoundEntity(caster, 167, 128);
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
			for ( node_t* node = map.creatures->first; node && false; node = node->next )
			{
				if ( Entity* entity = getSpellTarget(node, HEAL_RADIUS, caster, false, TARGET_FRIEND) )
				{
					entity->setEffect(EFF_STURDINESS, (Uint8)1, 30 * TICKS_PER_SECOND, false);
					messagePlayerColor(entity->isEntityPlayer(),
						MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6479));
					playSoundEntity(entity, 167, 128);
				}
			}
		}
		else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_PROF_STURDINESS].element_internal_name) )
		{
			if ( caster )
			{
				if ( Stat* casterStats = caster->getStats() )
				{
					caster->setEffect(EFF_STURDINESS, (Uint8)1, 30 * TICKS_PER_SECOND, false);
					messagePlayerColor(caster->isEntityPlayer(),
						MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6479));
					playSoundEntity(caster, 167, 128);
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
			for ( node_t* node = map.creatures->first; node && false; node = node->next )
			{
				if ( Entity* entity = getSpellTarget(node, HEAL_RADIUS, caster, false, TARGET_FRIEND) )
				{
					entity->setEffect(EFF_STURDINESS, (Uint8)1, 30 * TICKS_PER_SECOND, false);
					messagePlayerColor(entity->isEntityPlayer(),
						MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6479));
					playSoundEntity(entity, 167, 128);
				}
			}
		}
		else if ( spell->ID == SPELL_DELAY_PAIN )
		{
			if ( caster )
			{
				if ( Stat* casterStats = caster->getStats() )
				{
					if ( caster->setEffect(EFF_DELAY_PAIN, (Uint8)1, 30 * TICKS_PER_SECOND, false) )
					{
						messagePlayerColor(caster->isEntityPlayer(),
							MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6627));
						playSoundEntity(caster, 166, 128);
					}
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
			}
		else if ( spell->ID == SPELL_SACRED_PATH )
		{
			if ( caster )
			{
				if ( Stat* casterStats = caster->getStats() )
				{
					caster->setEffect(EFF_SACRED_PATH, (Uint8)3, 30 * TICKS_PER_SECOND, false);
					messagePlayerColor(caster->isEntityPlayer(),
						MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6493));
					playSoundEntity(caster, 166, 128);
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
		}
		else if ( spell->ID == SPELL_FORCE_SHIELD )
		{
			if ( caster )
			{
				if ( Stat* casterStats = caster->getStats() )
				{
					caster->setEffect(EFF_REFLECTOR_SHIELD, false, 0, false);
					caster->setEffect(EFF_FORCE_SHIELD, true, 5 * TICKS_PER_SECOND, false);
					messagePlayerColor(caster->isEntityPlayer(),
						MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6699));
				}
				playSoundEntity(caster, 166, 128);
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
		}
		else if ( spell->ID == SPELL_REFLECTOR )
		{
			if ( caster )
			{
				if ( Stat* casterStats = caster->getStats() )
				{
					caster->setEffect(EFF_FORCE_SHIELD, false, 0, false);
					caster->setEffect(EFF_REFLECTOR_SHIELD, true, 5 * TICKS_PER_SECOND, false);
					messagePlayerColor(caster->isEntityPlayer(),
						MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6700));
				}
				playSoundEntity(caster, 166, 128);
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
		}
		else if ( spell->ID == SPELL_MANIFEST_DESTINY )
		{
			if ( caster && caster->behavior == &actPlayer )
			{
				auto& chests = achievementObserver.playerAchievements[caster->skill[2]].manifestDestinyChests;
				auto& sequence = achievementObserver.playerAchievements[caster->skill[2]].manifestDestinyChestSequence;
				bool found = false;
				if ( chests.size() )
				{
					int visited = 0;
					for ( int i = sequence + 1; visited < chests.size(); ++i )
					{
						if ( i >= chests.size() )
						{
							i = 0;
						}
						++visited;
						if ( i < chests.size() )
						{
							if ( chests[i] == 0 ) { continue; }
							if ( !uidToEntity(chests[i]) )
							{
								chests[i] = 0; // chest no longer exists
								continue;
							}
							found = true;
							sequence = i;
							break;
						}
					}
				}

				if ( found )
				{
					Entity* spellTimer = createParticleTimer(caster, 50, 625);
					spellTimer->particleTimerPreDelay = 0; // wait x ticks before animation.
					spellTimer->particleTimerEndAction = PARTICLE_EFFECT_SHRINE_TELEPORT; // teleport behavior of timer.
					spellTimer->particleTimerEndSprite = 625; // sprite to use for end of timer function.
					spellTimer->particleTimerCountdownAction = 1;
					spellTimer->particleTimerCountdownSprite = 625;
					spellTimer->particleTimerTarget = static_cast<Sint32>(chests[sequence]); // get the target to teleport around.
					spellTimer->particleTimerVariable1 = 1; // distance of teleport in tiles
					spellTimer->particleTimerVariable2 = caster->getUID(); // which player to teleport
					if ( multiplayer == SERVER )
					{
						serverSpawnMiscParticles(caster, PARTICLE_EFFECT_DESTINY_TELEPORT, 625, spellTimer->particleTimerDuration);
					}
					//shrineActivateDelay = 250;
					//serverUpdateEntitySkill(this, 7);
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6494));
				}
				else if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(3715));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
		}
		else if ( spell->ID == SPELL_SCRY_TRAPS )
		{
			if ( caster )
			{
				std::vector<Entity*> traps;
				bool found = false;
				for ( node_t* node = map.entities->first; node; node = node->next )
				{
					if ( Entity* ent = (Entity*)node->element )
					{
						if ( ent->behavior == &actBoulderTrap || ent->behavior == &actArrowTrap
							|| ent->behavior == &actMagicTrap || ent->behavior == &actMagicTrapCeiling
							|| ent->behavior == &actBoulderTrapEast || ent->behavior == &actBoulderTrapWest
							|| ent->behavior == &actBoulderTrapNorth || ent->behavior == &actBoulderTrapSouth
							|| ent->behavior == &actSummonTrap || ent->behavior == &actSpearTrap )
						{
							if ( entityDist(caster, ent) >= 10000.0 )
							{
								continue;
							}
							found = true;
							int duration = 30 * TICKS_PER_SECOND;
							createParticleSpellPinpointTarget(ent, caster->getUID(), 1771, duration, spell->ID);
							serverSpawnMiscParticles(ent, PARTICLE_EFFECT_PINPOINT, 1771, caster->getUID(), duration, spell->ID);
						}
					}
				}

				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(3715));
				}
				else
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6483));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
		}
		else if ( spell->ID == SPELL_SCRY_TREASURES )
		{
			if ( caster )
			{
				std::vector<Entity*> treasures;
				bool found = false;
				for ( node_t* node = map.entities->first; node; node = node->next )
				{
					if ( Entity* ent = (Entity*)node->element )
					{
						if ( ent->behavior == &actChest || ent->isInertMimic()
							|| ent->behavior == &actItem || ent->behavior == &actGoldBag )
						{
							if ( ent->behavior == &actItem )
							{
								if ( ent->skill[10] >= KEY_STONE && ent->skill[10] <= KEY_MACHINE )
								{
									// good items
								}
								else
								{
									continue;
								}
							}
							if ( entityDist(caster, ent) >= 10000.0 )
							{
								continue;
							}
							found = true;
							int duration = 30 * TICKS_PER_SECOND;
							createParticleSpellPinpointTarget(ent, caster->getUID(), 1772, duration, spell->ID);
							serverSpawnMiscParticles(ent, PARTICLE_EFFECT_PINPOINT, 1772, caster->getUID(), duration, spell->ID);
						}
					}
				}

				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(3715));
				}
				else
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6483));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
		}
		else if ( spell->ID == SPELL_DONATION )
		{
			if ( caster )
			{
				std::vector<Entity*> breakables;
				for ( node_t* node = map.entities->first; node; node = node->next )
				{
					if ( Entity* entity = (Entity*)node->element )
					{
						if ( entity->isColliderBreakableContainer() )
						{
							if ( entity->colliderHideMonster == 0 && entity->colliderContainedEntity == 0 )
							{
								if ( entityDist(caster, entity) < 10000.0 )
								{
									breakables.push_back(entity);
								}
							}
						}
					}
				}

				bool found = false;
				while ( breakables.size() )
				{
					int pick = local_rng.rand() % breakables.size();
					auto entity = breakables[pick];

					Entity* item = newEntity(-1, 1, map.entities, nullptr); //Rock entity.
					item->flags[INVISIBLE] = true;
					item->flags[UPDATENEEDED] = true;
					item->x = entity->x;
					item->y = entity->y;
					item->z = 7.5;
					item->roll = PI / 2.0;
					item->sizex = 4;
					item->sizey = 4;
					item->yaw = local_rng.rand() % 360 * PI / 180;
					item->flags[PASSABLE] = true;
					item->behavior = &actItem;
					item->flags[USERFLAG1] = true; // no collision: helps performance
					item->skill[10] = GEM_RUBY;    // type
					item->skill[11] = WORN;        // status
					item->skill[12] = 0;           // beatitude
					item->skill[13] = 1;           // count
					item->skill[14] = 0;           // appearance
					item->skill[15] = 1;		   // identified
					item->itemContainer = entity->getUID();
					entity->colliderContainedEntity = item->getUID();
					found = true;

					int duration = 30 * TICKS_PER_SECOND;
					createParticleSpellPinpointTarget(entity, caster->getUID(), 1768, duration, spell->ID);
					serverSpawnMiscParticles(entity, PARTICLE_EFFECT_PINPOINT, 1768, caster->getUID(), duration, spell->ID);
					break;

					if ( !found )
					{
						breakables.erase(breakables.begin() + pick);
					}
				}

				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(3715));
				}
				else
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6488));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 167, 128);
			}
		}
		else if ( spell->ID == SPELL_SCRY_ALLIES )
		{
			bool found = false;
			if ( caster && caster->behavior == &actPlayer )
			{
				std::vector<Entity*> allies;
				for ( node_t* node = map.creatures->first; node; node = node->next )
				{
					if ( Entity* entity = getSpellTarget(node, 10000, caster, false, TARGET_FRIEND) )
					{
						if ( !entity->monsterAllyGetPlayerLeader() && entity->behavior != &actPlayer )
						{
							if ( Stat* stats = entity->getStats() )
							{
								//if ( stats->leader_uid == 0 )
								if ( stats->type != SHOPKEEPER )
								{
									found = true;
									int duration = 5 * TICKS_PER_SECOND;
									createParticleSpellPinpointTarget(entity, caster->getUID(), 1769, duration, spell->ID);
									serverSpawnMiscParticles(entity, PARTICLE_EFFECT_PINPOINT, 1769, caster->getUID(), duration, spell->ID);
								}
							}
						}
					}
				}
				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(3715));
				}
				else
				{
					messagePlayerColor(caster->isEntityPlayer(),
						MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6481));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 167, 128);
			}
		}
		else if ( spell->ID == SPELL_CALL_ALLIES )
		{
			bool found = false;
			if ( caster && caster->behavior == &actPlayer )
			{
				std::vector<Entity*> allies;
				for ( node_t* node = map.creatures->first; node; node = node->next )
				{
					if ( Entity* entity = getSpellTarget(node, 10000, caster, false, TARGET_FRIEND) )
					{
						if ( !entity->monsterAllyGetPlayerLeader() && entity->behavior != &actPlayer )
						{
							if ( entity->monsterState == MONSTER_STATE_WAIT )
							{
								if ( Stat* stats = entity->getStats() )
								{
									//if ( stats->leader_uid == 0 )
									if ( stats->type != SHOPKEEPER && !entity->isInertMimic() )
									{
										if ( entity->monsterSetPathToLocation(caster->x / 16, caster->y / 16,
											3, GeneratePathTypes::GENERATE_PATH_TO_HUNT_MONSTER_TARGET, true) )
										{
											entity->monsterState = MONSTER_STATE_HUNT;
											found = true;
										}
									}
								}
							}
							//createParticleSpellPinpointTarget(entity, caster->getUID(), 1769, 30 * TICKS_PER_SECOND, spell->ID);
							//serverSpawnMiscParticles(entity, PARTICLE_EFFECT_PINPOINT, 1769, caster->getUID());
						}
					}
				}
			}

			if ( !found )
			{
				messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(3715));
			}
			else
			{
				messagePlayerColor(caster->isEntityPlayer(),
					MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6489));
			}
			if ( caster )
			{
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
			playSoundEntity(caster, 167, 128);
		}
		else if ( spell->ID == SPELL_SEEK_ALLY )
		{
			bool foundTarget = false;
			if ( caster && caster->behavior == &actPlayer )
			{
				if ( castSpellProps && castSpellProps->targetUID != 0 )
				{
					if ( Entity* target = uidToEntity(castSpellProps->targetUID) )
					{
						if ( target->getStats() )
						{
							spawnMagicEffectParticles(target->x, target->y, target->z, 171);
							playSoundEntity(target, 167, 128);
							foundTarget = true;
							std::vector<Entity*> allies;
							bool foundAlly = false;
							if ( target->setEffect(EFF_SEEK_CREATURE, true, 10 * TICKS_PER_SECOND, false) )
							{
								for ( node_t* node = map.creatures->first; node; node = node->next )
								{
									if ( Entity* entity = getSpellTarget(node, 10000, target, false, TARGET_FRIEND) )
									{
										if ( !entity->monsterAllyGetPlayerLeader() && entity->behavior != &actPlayer && entity != caster )
										{
											allies.push_back(entity);
										}
									}
								}
								while ( allies.size() )
								{
									int pick = local_rng.rand() % allies.size();
									Entity* ally = allies[pick];
									allies.erase(allies.begin() + pick);
									if ( target->monsterSetPathToLocation(ally->x / 16, ally->y / 16,
										1, GeneratePathTypes::GENERATE_PATH_TO_HUNT_MONSTER_TARGET, true) )
									{
										target->monsterReleaseAttackTarget();
										target->monsterState = MONSTER_STATE_HUNT;
										foundAlly = true;
										break;
									}
								}

								if ( !foundAlly )
								{
									messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(255, 255, 255),
										*target->getStats(), Language::get(6630), Language::get(6631), MSG_COMBAT);
									target->setEffect(EFF_SEEK_CREATURE, false, 0, false);
								}
								else
								{
									messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(0, 255, 0),
										*target->getStats(), Language::get(6633), Language::get(6634), MSG_COMBAT);
								}
							}
							else
							{
								messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(255, 0, 0),
									*target->getStats(), Language::get(2905), Language::get(2906), MSG_COMBAT);
							}
						}
					}
				}
			}

			if ( !foundTarget )
			{
				messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6498));
			}
			if ( caster )
			{
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
			playSoundEntity(caster, 167, 128);
		}
		else if ( spell->ID == SPELL_SEEK_FOE )
		{
			bool foundTarget = false;
			if ( caster && caster->behavior == &actPlayer )
			{
				if ( castSpellProps && castSpellProps->targetUID != 0 )
				{
					if ( Entity* target = uidToEntity(castSpellProps->targetUID) )
					{
						if ( target->getStats() )
						{
							spawnMagicEffectParticles(target->x, target->y, target->z, 171);
							playSoundEntity(target, 167, 128);
							foundTarget = true;
							std::vector<Entity*> enemies;
							bool foundEnemy = false;
							if ( target->setEffect(EFF_SEEK_CREATURE, true, 10 * TICKS_PER_SECOND, false) )
							{
								for ( node_t* node = map.creatures->first; node; node = node->next )
								{
									if ( Entity* entity = getSpellTarget(node, 10000, target, false, TARGET_ENEMY) )
									{
										if ( !entity->monsterAllyGetPlayerLeader() && entity->behavior != &actPlayer && entity != caster )
										{
											enemies.push_back(entity);
										}
									}
								}
								while ( enemies.size() )
								{
									int pick = local_rng.rand() % enemies.size();
									Entity* enemy = enemies[pick];
									enemies.erase(enemies.begin() + pick);
									if ( target->monsterSetPathToLocation(enemy->x / 16, enemy->y / 16,
										1, GeneratePathTypes::GENERATE_PATH_TO_HUNT_MONSTER_TARGET, true) )
									{
										target->monsterReleaseAttackTarget();
										target->monsterAcquireAttackTarget(*enemy, MONSTER_STATE_HUNT, false);
										target->monsterState = MONSTER_STATE_HUNT;
										foundEnemy = true;
										break;
									}
								}

								if ( !foundEnemy )
								{
									messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(255, 255, 255),
										*target->getStats(), Language::get(6635), Language::get(6636), MSG_COMBAT);
									target->setEffect(EFF_SEEK_CREATURE, false, 0, false);
								}
								else
								{
									messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(0, 255, 0),
										*target->getStats(), Language::get(6638), Language::get(6639), MSG_COMBAT);
								}
							}
							else
							{
								messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(255, 0, 0),
									*target->getStats(), Language::get(2905), Language::get(2906), MSG_COMBAT);
							}
						}
					}
				}
			}

			if ( !foundTarget )
			{
				messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6498));
			}
			if ( caster )
			{
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
			}
			playSoundEntity(caster, 167, 128);
		}
		else if ( spell->ID == SPELL_TURN_UNDEAD )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps && castSpellProps->targetUID != 0 )
				{
					if ( Entity* target = uidToEntity(castSpellProps->targetUID) )
					{
						if ( Stat* targetStats = target->getStats() )
						{
							if ( target->behavior == &actMonster && targetStats->type == SKELETON )
							{
								if ( target->setEffect(EFF_FEAR, true, TICKS_PER_SECOND * 5, true) )
								{
									playSoundEntity(target, 687, 128); // fear.ogg
									effect = true;
									messagePlayerColor(caster->isEntityPlayer(),
										MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6497));
									spawnMagicEffectParticles(target->x, target->y, target->z, 171);

									target->monsterAcquireAttackTarget(*caster, MONSTER_STATE_PATH);
									target->monsterFearfulOfUid = caster->getUID();
								}
							}
							found = true;

							if ( !effect )
							{
								messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(255, 0, 0),
									*targetStats, Language::get(2905), Language::get(2906), MSG_COMBAT);
							}

						}
					}
				}
				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6498));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 167, 128);
			}
		}
		else if ( spell->ID == SPELL_SPLINTER_GEAR )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps && castSpellProps->targetUID != 0 )
				{
					if ( Entity* target = uidToEntity(castSpellProps->targetUID) )
					{
						int damage = element->damage;
						if ( Stat* targetStats = target->getStats() )
						{
							if ( target->behavior == &actMonster || target->behavior == &actPlayer )
							{
								Item* armor = nullptr;
								bool shields = targetStats->shield && itemCategory(targetStats->shield) == ARMOR;
								int armornum = targetStats->pickRandomEquippedItemToDegradeOnHit(&armor, true, !shields, false, true);
								
								if ( armor != nullptr && armor->status > BROKEN )
								{
									ItemType type = armor->type;
									if ( target->degradeArmor(*targetStats, *armor, armornum) )
									{
										effect = true;
										if ( !armor || (armor && armor->status == BROKEN) )
										{
											damage *= 2;
										}

										if ( armor->status > BROKEN )
										{
											const char* msg = Language::get(6678); // named
											if ( !strcmp(targetStats->name, "") || monsterNameIsGeneric(*targetStats) )
											{
												msg = Language::get(6677);
											}

											if ( !strcmp(targetStats->name, "") )
											{
												messagePlayerColor(caster->isEntityPlayer(), MESSAGE_COMBAT, makeColorRGB(0, 255, 0),
													msg, getMonsterLocalizedName(targetStats->type).c_str(), items[type].getIdentifiedName());
											}
											else
											{
												messagePlayerColor(caster->isEntityPlayer(), MESSAGE_COMBAT, makeColorRGB(0, 255, 0),
													msg, targetStats->name, items[type].getIdentifiedName());
											}
										}
										else
										{
											const char* msg = Language::get(6680); // named
											if ( !strcmp(targetStats->name, "") || monsterNameIsGeneric(*targetStats) )
											{
												msg = Language::get(6679);
											}
											if ( !strcmp(targetStats->name, "") )
											{
												messagePlayerColor(caster->isEntityPlayer(), MESSAGE_COMBAT, makeColorRGB(0, 255, 0),
													msg, getMonsterLocalizedName(targetStats->type).c_str(), items[type].getIdentifiedName());
											}
											else
											{
												messagePlayerColor(caster->isEntityPlayer(), MESSAGE_COMBAT, makeColorRGB(0, 255, 0),
													msg, targetStats->name, items[type].getIdentifiedName());
											}
										}
									}

								}

								if ( targetStats->type == CRYSTALGOLEM
									|| targetStats->type == AUTOMATON
									|| targetStats->type == MINIMIMIC
									|| targetStats->type == MIMIC )
								{
									effect = true;
								}

								if ( effect )
								{
									applyGenericMagicDamage(caster, target, *caster, spell->ID, damage, true);
									target->setEffect(EFF_BLEEDING, true, 10 * TICKS_PER_SECOND, false);
									target->setEffect(EFF_SLOW, true, 10 * TICKS_PER_SECOND, false);
									
									for ( int i = 0; i < 5; ++i )
									{
										Entity* gib = spawnGib(target, 187);
										gib->sprite = 187;
										serverSpawnGibForClient(gib);
									}
								}
							}
							found = true;

							if ( !effect )
							{
								messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(255, 0, 0),
									*targetStats, Language::get(2905), Language::get(2906), MSG_COMBAT);
							}
						}
						else if ( target->behavior == &actChest )
						{
							applyGenericMagicDamage(caster, target, *caster, spell->ID, damage, true);
							for ( int i = 0; i < 5; ++i )
							{
								Entity* gib = spawnGib(target, 187);
								gib->sprite = 187;
								serverSpawnGibForClient(gib);
							}
							found = true;
						}
					}
				}
				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6498));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 167, 128);
			}
		}
		else if ( spell->ID == SPELL_MAXIMISE )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps && castSpellProps->targetUID != 0 )
				{
					if ( Entity* target = uidToEntity(castSpellProps->targetUID) )
					{
						if ( Stat* targetStats = target->getStats() )
						{
							if ( target->setEffect(EFF_MAXIMISE, true, TICKS_PER_SECOND * 5, false) )
							{
								if ( targetStats->getEffectActive(EFF_MINIMISE) )
								{
									target->setEffect(EFF_MINIMISE, false, 0, false);
								}
								playSoundEntity(caster, 167, 128);
								effect = true;
								messagePlayerColor(caster->isEntityPlayer(),
									MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6516));
								spawnMagicEffectParticles(target->x, target->y, target->z, 171);
							}
							found = true;

							if ( !effect )
							{
								messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(255, 0, 0),
									*targetStats, Language::get(2905), Language::get(2906), MSG_COMBAT);
							}
						}
					}
				}
				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6498));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 167, 128);
			}
		}
		else if ( spell->ID == SPELL_MINIMISE )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps && castSpellProps->targetUID != 0 )
				{
					if ( Entity* target = uidToEntity(castSpellProps->targetUID) )
					{
						if ( Stat* targetStats = target->getStats() )
						{
							if ( target->setEffect(EFF_MINIMISE, true, TICKS_PER_SECOND * 5, false) )
							{
								if ( targetStats->getEffectActive(EFF_MAXIMISE) )
								{
									target->setEffect(EFF_MAXIMISE, false, 0, false);
								}
								playSoundEntity(caster, 167, 128);
								effect = true;
								messagePlayerColor(caster->isEntityPlayer(),
									MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6517));
								spawnMagicEffectParticles(target->x, target->y, target->z, 171);
							}
							found = true;

							if ( !effect )
							{
								messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(255, 0, 0),
									*targetStats, Language::get(2905), Language::get(2906), MSG_COMBAT);
							}
						}
					}
				}
				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6498));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 167, 128);
			}
		}
		else if ( spell->ID == SPELL_INCOHERENCE )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps && castSpellProps->targetUID != 0 )
				{
					if ( Entity* target = uidToEntity(castSpellProps->targetUID) )
					{
						if ( Stat* targetStats = target->getStats() )
						{
							if ( target->setEffect(EFF_INCOHERENCE, true, TICKS_PER_SECOND * 5, false) )
							{
								playSoundEntity(caster, 167, 128);
								effect = true;
								messagePlayerColor(caster->isEntityPlayer(),
									MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6519));
								spawnMagicEffectParticles(target->x, target->y, target->z, 171);
							}
							found = true;

							if ( !effect )
							{
								messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(255, 0, 0),
									*targetStats, Language::get(2905), Language::get(2906), MSG_COMBAT);
							}
						}
					}
				}
				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6498));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 167, 128);
			}
		}
		else if ( spell->ID == SPELL_COWARDICE )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps && castSpellProps->targetUID != 0 )
				{
					if ( Entity* target = uidToEntity(castSpellProps->targetUID) )
					{
						if ( Stat* targetStats = target->getStats() )
						{
							Uint8 effectStrength = 10;
							if ( target->setEffect(EFF_COWARDICE, effectStrength, TICKS_PER_SECOND * 10, false) )
							{
								if ( targetStats->getEffectActive(EFF_COURAGE) )
								{
									target->setEffect(EFF_COURAGE, false, 0, false);
								}
								playSoundEntity(caster, 167, 128);
								effect = true;
								messagePlayerColor(caster->isEntityPlayer(),
									MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6632));
								spawnMagicEffectParticles(target->x, target->y, target->z, 171);
							}
							found = true;

							if ( !effect )
							{
								messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(255, 0, 0),
									*targetStats, Language::get(2905), Language::get(2906), MSG_COMBAT);
							}
						}
					}
				}
				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6498));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 167, 128);
			}
		}
		else if ( spell->ID == SPELL_COURAGE )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps && castSpellProps->targetUID != 0 )
				{
					if ( Entity* target = uidToEntity(castSpellProps->targetUID) )
					{
						if ( Stat* targetStats = target->getStats() )
						{
							Uint8 effectStrength = 10;
							if ( target->setEffect(EFF_COURAGE, effectStrength, TICKS_PER_SECOND * 10, false) )
							{
								if ( targetStats->getEffectActive(EFF_COWARDICE) )
								{
									target->setEffect(EFF_COWARDICE, false, 0, false);
								}
								playSoundEntity(caster, 167, 128);
								effect = true;
								messagePlayerColor(caster->isEntityPlayer(),
									MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6637));
								spawnMagicEffectParticles(target->x, target->y, target->z, 171);
							}
							found = true;

							if ( !effect )
							{
								messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(255, 0, 0),
									*targetStats, Language::get(2905), Language::get(2906), MSG_COMBAT);
							}
						}
					}
				}
				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6498));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 167, 128);
			}
			}
		else if ( spell->ID == SPELL_TABOO )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps && castSpellProps->targetUID != 0 )
				{
					if ( Entity* target = uidToEntity(castSpellProps->targetUID) )
					{
						if ( Stat* targetStats = target->getStats() )
						{
							int duration = TICKS_PER_SECOND * 10;
							if ( target->setEffect(EFF_TABOO, (Uint8)(caster->behavior != &actPlayer ? MAXPLAYERS + 1 : caster->skill[2] + 1), TICKS_PER_SECOND * 10, true) )
							{
								playSoundEntity(caster, 167, 128);
								effect = true;
								if ( target->behavior == &actPlayer )
								{
									messagePlayerColor(caster->isEntityPlayer(),
										MESSAGE_HINT, makeColorRGB(255, 0, 0), Language::get(6641));
								}
								messagePlayerColor(caster->isEntityPlayer(),
									MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6640));

								createParticleSpellPinpointTarget(target, caster->getUID(), 1774, duration, spell->ID);
								serverSpawnMiscParticles(target, PARTICLE_EFFECT_PINPOINT, 1774, caster->getUID(), duration, spell->ID);

								for ( node_t* node = map.creatures->first; node; node = node->next )
								{
									if ( Entity* entity = getSpellTarget(node, 10000, caster, false, TARGET_ENEMY) )
									{
										if ( entity != target )
										{
											if ( entityDist(entity, target) < 16.0 * 4  )
											{
												Entity* entityTarget = uidToEntity(entity->monsterTarget);
												if ( !entityTarget || entityTarget == caster ||
													caster->checkFriend(entityTarget) )
												{
													// other nearby enemies locate the taboo target
													// if target attacking caster / caster allies then it gets redirected too
													Entity* ohit = hit.entity;
													real_t tangent = atan2(target->y - entity->y, target->x - entity->x);
													lineTraceTarget(entity, entity->x, entity->y, tangent, 16.0 * 4, 0, false, target);
													if ( hit.entity == target )
													{
														if ( entity->checkEnemy(target) )
														{
															entity->monsterAcquireAttackTarget(*target, MONSTER_STATE_PATH);
														}
													}
													hit.entity = ohit;
												}
											}
										}
									}
								}
							}
							found = true;

							if ( !effect )
							{
								messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(255, 0, 0),
									*targetStats, Language::get(2905), Language::get(2906), MSG_COMBAT);
							}
						}
					}
				}
				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6498));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 167, 128);
			}
			}
		else if ( spell->ID == SPELL_BOOBY_TRAP )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps && castSpellProps->targetUID != 0 )
				{
					if ( Entity* target = uidToEntity(castSpellProps->targetUID) )
					{
						found = true;

						Entity* spellTimer = createParticleBoobyTrapExplode(caster, target->x, target->y);
						spellTimer->particleTimerVariable1 = element->damage;
						spellTimer->particleTimerVariable2 = SPELL_BOOBY_TRAP;
						spellTimer->particleTimerTarget = target->getUID();

						serverSpawnMiscParticlesAtLocation(spellTimer->x, spellTimer->y, 0, PARTICLE_EFFECT_BOOBY_TRAP, 0);
					}
				}
				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6498));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 167, 128);
			}
		}
		else if ( spell->ID == SPELL_DEMESNE_DOOR )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps && castSpellProps->targetUID != 0 )
				{
					if ( Entity* target = uidToEntity(castSpellProps->targetUID) )
					{
						found = true;
						
						Entity* door = spellEffectDemesneDoor(*caster, *target);
						if ( door )
						{
							door->skill[0] = TICKS_PER_SECOND * 5;
							messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6694));
						}
						else
						{
							messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6692));
						}
					}
				}
				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6498));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 167, 128);
			}
		}
		else if ( spell->ID == SPELL_SPIRIT_WEAPON )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps )
				{
					found = true;
					real_t x = floor(castSpellProps->target_x / 16) * 16 + 8.0;
					real_t y = floor(castSpellProps->target_y / 16) * 16 + 8.0;
					if ( Entity* monster = spellEffectAdorcise(*caster, spellElementMap[SPELL_SPIRIT_WEAPON], x, y, nullptr) )
					{
						
					}
					else
					{
						if ( caster->behavior == &actPlayer )
						{
							// no room to spawn!
							messagePlayer(caster->skill[2], MESSAGE_MISC, Language::get(6578));
						}
					}
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 171, 128);
			}
		}
		else if ( spell->ID == SPELL_FIRE_SPRITE )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps )
				{
					found = true;
					real_t x = floor(castSpellProps->target_x / 16) * 16 + 8.0;
					real_t y = floor(castSpellProps->target_y / 16) * 16 + 8.0;
					if ( Entity* monster = spellEffectFlameSprite(*caster, spellElementMap[SPELL_FIRE_SPRITE], x, y) )
					{

					}
					else
					{
						if ( caster->behavior == &actPlayer )
						{
							// no room to spawn!
							messagePlayer(caster->skill[2], MESSAGE_MISC, Language::get(6578));
						}
					}
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 171, 128);
			}
		}
		else if ( spell->ID == SPELL_FLAME_ELEMENTAL )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps )
				{
					found = true;
					real_t x = floor(castSpellProps->target_x / 16) * 16 + 8.0;
					real_t y = floor(castSpellProps->target_y / 16) * 16 + 8.0;
					if ( Entity* monster = spellEffectFlameSprite(*caster, spellElementMap[SPELL_FLAME_ELEMENTAL], x, y) )
					{

					}
					else
					{
						if ( caster->behavior == &actPlayer )
						{
							// no room to spawn!
							messagePlayer(caster->skill[2], MESSAGE_MISC, Language::get(6578));
						}
					}
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 171, 128);
			}
		}
		else if ( spell->ID == SPELL_NULL_AREA )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps )
				{
					found = true;
					createRadiusMagic(SPELL_NULL_AREA, caster, 
						castSpellProps->target_x, castSpellProps->target_y, 24, 5 * TICKS_PER_SECOND, nullptr);

				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 171, 128);
			}
		}
		else if ( spell->ID == SPELL_LIGHTNING_BOLT )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps )
				{
					found = true;
					
					Uint32 lifetime = TICKS_PER_SECOND * 3;

					Entity* spellTimer = createParticleTimer(caster, lifetime + TICKS_PER_SECOND, -1);
					spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_LIGHTNING;
					spellTimer->particleTimerCountdownSprite = 1757;
					spellTimer->yaw = caster->yaw;
					spellTimer->x = castSpellProps->target_x;
					spellTimer->y = castSpellProps->target_y;
					spellTimer->flags[NOUPDATE] = false; // spawn for client
					spellTimer->flags[UPDATENEEDED] = true;
					Sint32 val = (1 << 31);
					val |= (Uint8)(19);
					val |= (((Uint16)(spellTimer->particleTimerDuration) & 0xFFF) << 8);
					val |= (Uint8)(spellTimer->particleTimerCountdownAction & 0xFF) << 20;
					spellTimer->skill[2] = val;
					spellTimer->particleTimerEffectLifetime = lifetime;
					floorMagicCreateLightningSequence(spellTimer, 0);
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 171, 128);
			}
		}
		else if ( spell->ID == SPELL_ETERNALS_GAZE )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps )
				{
					found = true;

					Entity* spellTimer = createParticleTimer(caster, 3 * TICKS_PER_SECOND + 10, -1);
					spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_ETERNALS_GAZE;
					spellTimer->particleTimerCountdownSprite = -1;
					spellTimer->yaw = caster->yaw;
					spellTimer->x = castSpellProps->target_x;
					spellTimer->y = castSpellProps->target_y;
					spellTimer->flags[NOUPDATE] = false; // spawn for client
					spellTimer->flags[UPDATENEEDED] = true;
					Sint32 val = (1 << 31);
					val |= (Uint8)(19);
					val |= (((Uint16)(spellTimer->particleTimerDuration) & 0xFFF) << 8);
					val |= (Uint8)(spellTimer->particleTimerCountdownAction & 0xFF) << 20;
					spellTimer->skill[2] = val;
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 171, 128);
			}
		}
		else if ( spell->ID == SPELL_SHATTER_EARTH )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps )
				{
					int x = castSpellProps->target_x / 16;
					int y = castSpellProps->target_y / 16;
					bool noroom = false;
					if ( x < 0 || x >= map.width || y < 0 || y >= map.height )
					{
						noroom = true;
					}
					else
					{
						int mapIndex = (y)*MAPLAYERS + (x)*MAPLAYERS * map.height;
						if ( map.tiles[OBSTACLELAYER + mapIndex] )
						{
							noroom = true;
						}
						else if ( map.skybox != 0 )
						{
							if ( !map.tiles[(MAPLAYERS - 1) + mapIndex] )
							{
								noroom = true;
							}
						}
					}

					if ( !noroom )
					{
						found = true;
						Entity* spellTimer = createParticleTimer(caster, 5 * TICKS_PER_SECOND, -1);
						spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SHATTER_EARTH;
						spellTimer->particleTimerCountdownSprite = -1;
						spellTimer->yaw = caster->yaw;
						spellTimer->x = x * 16.0 + 8.0;
						spellTimer->y = y * 16.0 + 8.0;
						spellTimer->flags[NOUPDATE] = false; // spawn for client
						spellTimer->flags[UPDATENEEDED] = true;
						Sint32 val = (1 << 31);
						val |= (Uint8)(19);
						val |= (((Uint16)(spellTimer->particleTimerDuration) & 0xFFF) << 8);
						val |= (Uint8)(spellTimer->particleTimerCountdownAction & 0xFF) << 20;
						spellTimer->skill[2] = val;

						spawnMagicEffectParticles(spellTimer->x, spellTimer->y, 7.5, 171);
					}
				}

				if ( found )
				{
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
					playSoundEntity(caster, 171, 128);
				}
				else
				{
					playSoundEntity(caster, 163, 128);
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6736));
				}
			}
		}
		else if ( spell->ID == SPELL_EARTH_ELEMENTAL )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps )
				{
					int x = castSpellProps->target_x / 16;
					int y = castSpellProps->target_y / 16;
					bool noroom = false;
					if ( x < 0 || x >= map.width || y < 0 || y >= map.height )
					{
						noroom = true;
					}
					else
					{
						int mapIndex = (y)*MAPLAYERS + (x)*MAPLAYERS * map.height;
						if ( map.tiles[OBSTACLELAYER + mapIndex] )
						{
							noroom = true;
						}
						else if ( map.skybox != 0 )
						{
							if ( !map.tiles[(MAPLAYERS - 1) + mapIndex] )
							{
								noroom = true;
							}
						}
					}

					if ( !noroom )
					{
						found = true;
						Entity* spellTimer = createParticleTimer(caster, 5 * TICKS_PER_SECOND, -1);
						spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_EARTH_ELEMENTAL;
						spellTimer->particleTimerCountdownSprite = -1;
						spellTimer->yaw = caster->yaw;
						spellTimer->x = x * 16.0 + 8.0;
						spellTimer->y = y * 16.0 + 8.0;
						spellTimer->flags[NOUPDATE] = false; // spawn for client
						spellTimer->flags[UPDATENEEDED] = true;
						Sint32 val = (1 << 31);
						val |= (Uint8)(19);
						val |= (((Uint16)(spellTimer->particleTimerDuration) & 0xFFF) << 8);
						val |= (Uint8)(spellTimer->particleTimerCountdownAction & 0xFF) << 20;
						spellTimer->skill[2] = val;

						spawnMagicEffectParticles(spellTimer->x, spellTimer->y, 7.5, 171);
					}
				}

				if ( found )
				{
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
					playSoundEntity(caster, 171, 128);
				}
				else
				{
					playSoundEntity(caster, 163, 128);
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6736));
				}
			}
		}
		else if ( spell->ID == SPELL_FIRE_WALL )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps )
				{
					found = true;

					for ( int i = 0; i < 3; ++i )
					{
						if ( i == 0 || i == 2 ) { continue; }
						bool light = false;
						if ( i == 1 )
						{
							light = true;
						}

						int duration = TICKS_PER_SECOND * 5;
						Entity* spellTimer = createParticleTimer(caster, duration, -1);
						spellTimer->x = castSpellProps->target_x;
						spellTimer->y = castSpellProps->target_y;

						real_t tangent = atan2(castSpellProps->target_y - castSpellProps->caster_y,
							castSpellProps->target_x - castSpellProps->caster_x);

						Entity* wave = createParticleWave(ParticleTimerEffect_t::EFFECT_FIRE_WAVE,
							1733, castSpellProps->target_x, castSpellProps->target_y, 2.75,
							-PI / 2 + tangent - PI / 3 + i * PI / 3,
							duration, light);
						real_t grouping = 13.75;
						wave->x -= grouping * cos(tangent);
						wave->y -= grouping * sin(tangent);
						real_t scale = 1.0;
						wave->skill[1] = 6; // frames
						wave->skill[5] = 4; // frame time
						wave->ditheringOverride = 6;
						wave->parent = spellTimer->getUID();
						real_t startScale = 0.1;
						wave->scalex = startScale;
						wave->scaley = startScale;
						wave->scalez = startScale;
						wave->focaly = startScale * grouping;
						wave->fskill[0] = scale; // final scale
						wave->fskill[1] = grouping; // final grouping
						wave->skill[6] = 1; // grow to scale
						wave->flags[UPDATENEEDED] = true;
					}
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 171, 128);
			}
		}
		else if ( spell->ID == SPELL_KINETIC_FIELD )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps )
				{
					found = true;

					int duration = TICKS_PER_SECOND * 5;
					Entity* spellTimer = createParticleTimer(caster, duration, -1);
					spellTimer->x = castSpellProps->target_x;
					spellTimer->y = castSpellProps->target_y;

					real_t tangent = atan2(castSpellProps->target_y - castSpellProps->caster_y,
						castSpellProps->target_x - castSpellProps->caster_x);

					Entity* wave = createParticleWave(ParticleTimerEffect_t::EFFECT_KINETIC_FIELD,
						1739, castSpellProps->target_x, castSpellProps->target_y, 6.25,
						PI / 2 + tangent, duration, true);
					wave->skill[1] = 12; // frames
					wave->skill[5] = 4; // frame time
					wave->ditheringOverride = 6;
					wave->sizex = 8;
					wave->sizey = 8;
					wave->parent = spellTimer->getUID();
					real_t startScale = 0.1;
					real_t scale = 1.0;
					wave->scalex = startScale;
					wave->scaley = startScale;
					wave->scalez = startScale;
					wave->fskill[0] = scale; // final scale
					wave->skill[6] = 1; // grow to scale
					wave->flags[UPDATENEEDED] = true;
					spawnMagicEffectParticles(wave->x, wave->y, wave->z, 171);
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 171, 128);
			}
		}
		else if ( spell->ID == SPELL_CHRONOMIC_FIELD )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps )
				{
					found = true;

					int duration = TICKS_PER_SECOND * 5;
					Entity* spellTimer = createParticleTimer(caster, duration, -1);
					spellTimer->x = castSpellProps->target_x;
					spellTimer->y = castSpellProps->target_y;

					real_t tangent = atan2(castSpellProps->target_y - castSpellProps->caster_y,
						castSpellProps->target_x - castSpellProps->caster_x);

					Entity* wave = createParticleWave(ParticleTimerEffect_t::EFFECT_CHRONOMIC_FIELD,
						1857, castSpellProps->target_x, castSpellProps->target_y, 5.25,
						PI / 2 + tangent, duration, true);
					wave->skill[1] = 8; // frames
					wave->skill[5] = 4; // frame time
					wave->ditheringOverride = 6;
					wave->sizex = 8;
					wave->sizey = 8;
					wave->parent = spellTimer->getUID();
					real_t startScale = 0.1;
					real_t scale = 1.0;
					wave->scalex = startScale;
					wave->scaley = startScale;
					wave->scalez = startScale;
					wave->fskill[0] = scale; // final scale
					wave->skill[6] = 1; // grow to scale
					wave->flags[UPDATENEEDED] = true;
					spawnMagicEffectParticles(wave->x, wave->y, wave->z, 171);
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 171, 128);
			}
		}
		else if ( spell->ID == SPELL_ICE_WAVE )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps )
				{
					found = true;

					real_t tangent = atan2(castSpellProps->target_y - castSpellProps->caster_y,
						castSpellProps->target_x - castSpellProps->caster_x);

					int lifetime = TICKS_PER_SECOND * 2;
					Entity* spellTimer = createParticleTimer(caster, lifetime + TICKS_PER_SECOND, -1);
					spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_MAGIC_WAVE;
					spellTimer->particleTimerCountdownSprite = 1718;
					spellTimer->yaw = tangent;
					spellTimer->x = castSpellProps->target_x;
					spellTimer->y = castSpellProps->target_y;
					int lifetime_tick = 0;
					auto& timerEffects = particleTimerEffects[spellTimer->getUID()];

					static std::vector<ParticleTimerEffect_t::EffectLocations_t> effLocations = {
						{0.0,		0.0, 0.25, -10.0,	172},
						{PI / 16,	8.0, 0.0,	0.0,	172},
						{-PI / 16,	-8.0, 0.0,	0.0,	0},
						{0.0,		0.0, 0.25,	0.0,	0},
						{PI / 16,	10.0, 0.0,	10.0,	0},
						{-PI / 16,	-10.0, 0.0,	10.0,	172}
					};

					int index = -1;
					while ( lifetime_tick <= lifetime )
					{
						++index;
						auto& effect = timerEffects.effectMap[lifetime_tick == 0 ? 1 : lifetime_tick]; // first behavior tick only occurs at 1
						effect.effectType = ParticleTimerEffect_t::EffectType::EFFECT_ICE_WAVE;
						auto& data = effLocations[index];
						effect.sfx = data.sfx;
						effect.yaw = tangent + data.yawOffset;
						effect.x = spellTimer->x + (data.dist) * cos(tangent);
						effect.y = spellTimer->y + (data.dist) * sin(tangent);
						effect.x += data.xOffset * cos(tangent + PI / 2);
						effect.y += data.xOffset * sin(tangent + PI / 2);
						lifetime_tick += std::max(1.0, TICKS_PER_SECOND * data.seconds);
						if ( index + 1 >= effLocations.size() )
						{
							break;
						}
					}
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 171, 128);
			}
		}
		else if ( spell->ID == SPELL_DISRUPT_EARTH
			|| spell->ID == SPELL_EARTH_SPINES )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps )
				{
					found = true;

					real_t tangent = atan2(castSpellProps->target_y - castSpellProps->caster_y,
						castSpellProps->target_x - castSpellProps->caster_x);

					int lifetime = TICKS_PER_SECOND * 2;
					Entity* spellTimer = createParticleTimer(caster, lifetime + TICKS_PER_SECOND, -1);
					spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_MAGIC_WAVE;
					spellTimer->particleTimerCountdownSprite = spell->ID == SPELL_DISRUPT_EARTH ? 1814 : 1815;
					spellTimer->yaw = tangent;
					spellTimer->x = castSpellProps->target_x;
					spellTimer->y = castSpellProps->target_y;
					int lifetime_tick = 0;
					auto& timerEffects = particleTimerEffects[spellTimer->getUID()];

					std::vector<ParticleTimerEffect_t::EffectLocations_t> effLocations;
					int roll = local_rng.rand() % 8;
					int numSprites = 8;
					for ( int i = 0; i < numSprites; ++i )
					{
						effLocations.push_back(ParticleTimerEffect_t::EffectLocations_t());
						auto& data = effLocations.back();
						if ( i == 0 )
						{
							data.sfx = 151;
						}
						data.seconds = 1 / (real_t)numSprites;
						data.dist = 0.25 + (0.75 * i / (real_t)numSprites);
						real_t angle = (i / ((real_t)numSprites / 2)) * PI + ((roll) / ((real_t)numSprites / 2)) * PI;
						data.xOffset = 8.0 * sin(angle);
						data.xOffset += 2.0 * (local_rng.rand() % 16) / 16.0;
						data.yawOffset = cos(angle) + ((local_rng.rand() % 4) / 4.0) * 2 * PI;
					}

					int index = -1;
					real_t dist = sqrt(pow(castSpellProps->target_x - castSpellProps->caster_x, 2)
						+ pow(castSpellProps->target_y - castSpellProps->caster_y, 2));
					dist = std::min(80.0, dist + 16.0);
					real_t minDist = 20.0;
					while ( lifetime_tick <= lifetime )
					{
						++index;
						auto& effect = timerEffects.effectMap[lifetime_tick == 0 ? 1 : lifetime_tick]; // first behavior tick only occurs at 1
						effect.effectType = ParticleTimerEffect_t::EffectType::EFFECT_DISRUPT_EARTH;
						auto& data = effLocations[index];
						effect.sfx = data.sfx;
						effect.yaw = tangent + data.yawOffset;
						effect.x = castSpellProps->caster_x + std::max(minDist, dist * (data.dist)) * cos(tangent);
						effect.y = castSpellProps->caster_y + std::max(minDist, dist * (data.dist)) * sin(tangent);
						effect.x += data.xOffset * cos(tangent + PI / 2);
						effect.y += data.xOffset * sin(tangent + PI / 2);
						lifetime_tick += std::max(1.0, TICKS_PER_SECOND * data.seconds);
						if ( index + 1 >= effLocations.size() )
						{
							break;
						}
					}
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 171, 128);
			}
		}
		else if ( spell->ID == SPELL_SLAM )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps )
				{
					found = true;

					real_t tangent = atan2(castSpellProps->target_y - castSpellProps->caster_y,
						castSpellProps->target_x - castSpellProps->caster_x);
					Entity* spellTimer = createParticleTimer(caster, 4 * TICKS_PER_SECOND, -1);
					spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_VORTEX;
					spellTimer->particleTimerCountdownSprite = -1;
					spellTimer->particleTimerVariable2 = spell->ID;
					spellTimer->flags[UPDATENEEDED] = true;
					spellTimer->flags[NOUPDATE] = false;
					spellTimer->yaw = tangent;
					spellTimer->x = castSpellProps->target_x;
					spellTimer->y = castSpellProps->target_y;

					if ( caster->behavior == &actMonster )
					{
						spellTimer->x = castSpellProps->caster_x + 8.0 * cos(tangent);
						spellTimer->y = castSpellProps->caster_y + 8.0 * sin(tangent);
						spellTimer->vel_x = 3.0 * cos(spellTimer->yaw);
						spellTimer->vel_y = 3.0 * sin(spellTimer->yaw);
					}

					Sint32 val = (1 << 31);
					val |= (Uint8)(19);
					val |= (((Uint16)(spellTimer->particleTimerDuration) & 0xFFF) << 8);
					val |= (Uint8)(spellTimer->particleTimerCountdownAction & 0xFF) << 20;
					spellTimer->skill[2] = val;
				}

				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 1719);
				playSoundEntity(caster, 171, 128);
			}
		}
		else if ( spell->ID == SPELL_ROOTS )
		{
			if ( caster )
			{
				/*Entity* spellTimer = createParticleTimer(caster, 5 * TICKS_PER_SECOND + 10, -1);
				spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_ROOTS1;
				spellTimer->particleTimerCountdownSprite = -1;
				spellTimer->flags[UPDATENEEDED] = true;
				spellTimer->flags[NOUPDATE] = false;
				spellTimer->yaw = caster->yaw;
				spellTimer->x = caster->x;
				spellTimer->y = caster->y;
				Sint32 val = (1 << 31);
				val |= (Uint8)(19);
				val |= (((Uint16)(spellTimer->particleTimerDuration) & 0xFFF) << 8);
				val |= (Uint8)(spellTimer->particleTimerCountdownAction & 0xFF) << 20;
				spellTimer->skill[2] = val;*/

				if ( floorMagicCreateRoots(caster->x, caster->y, caster, 0, SPELL_ROOTS, 5 * TICKS_PER_SECOND, PARTICLE_TIMER_ACTION_ROOTS1) )
				{
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
					playSoundEntity(caster, 171, 128);
				}
				else
				{
					// no room
					playSoundEntity(caster, 163, 128);
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6748));
				}
			}
		}
		else if ( spell->ID == SPELL_MUSHROOM )
		{
			if ( caster )
			{
				if ( castSpellProps )
				{
					if ( Entity* breakable = Entity::createBreakableCollider(EditorEntityData_t::getColliderIndexFromName("mushroom_spell_common"), 
						castSpellProps->target_x, castSpellProps->target_y, caster) )
					{
						spawnPoof(breakable->x, breakable->y, 7.5, 1.0, true);
						spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
					}
					else
					{
						// no room
						playSoundEntity(caster, 163, 128);
						messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6750));
					}
				}
			}
		}
		else if ( spell->ID == SPELL_VOID_CHEST )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps && castSpellProps->targetUID != 0 )
				{
					if ( Entity* target = uidToEntity(castSpellProps->targetUID) )
					{
						if ( target->behavior == &actChest )
						{
							found = true;
							if ( target->chestStatus == 1 )
							{
								messagePlayerColor(caster->isEntityPlayer(),
									MESSAGE_HINT, makeColorRGB(255, 255, 255), Language::get(6558));
							}
							else
							{
								messagePlayerColor(caster->isEntityPlayer(),
									MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6557));
								target->chestVoidState = TICKS_PER_SECOND * 5;
								serverUpdateEntitySkill(target, 17);
								createParticleErupt(target, 625);
								serverSpawnMiscParticles(target, PARTICLE_EFFECT_ERUPT, 625);
							}
						}
						else if ( target->behavior == &actMonster && target->getStats() && target->getStats()->type == MIMIC )
						{
							found = true;
							target->setEffect(EFF_MIMIC_VOID, true, TICKS_PER_SECOND * 30, false);
							if ( target->isInertMimic() )
							{
								target->disturbMimic(caster, false, true);
							}
							else
							{
								if ( !uidToEntity(target->monsterTarget) )
								{
									target->monsterAcquireAttackTarget(*caster, MONSTER_STATE_PATH, true);
								}
							}
							createParticleErupt(target, 625);
							serverSpawnMiscParticles(target, PARTICLE_EFFECT_ERUPT, 625);
						}
					}
				}
				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6498));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 167, 128);
			}
		}
		else if ( spell->ID == SPELL_COMMAND )
		{
			if ( caster && caster->behavior == &actPlayer )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps && castSpellProps->targetUID != 0 )
				{
					if ( Entity* target = uidToEntity(castSpellProps->targetUID) )
					{
						if ( Stat* targetStats = target->getStats() )
						{
							int player = caster->skill[2];
							int duration = 5 * TICKS_PER_SECOND;
							if ( target->setEffect(EFF_COMMAND, (Uint8)(player + 1), duration, true, true, true) )
							{
								playSoundEntity(caster, 167, 128);
								effect = true;
								messagePlayerColor(caster->isEntityPlayer(),
									MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6537));
								spawnMagicEffectParticles(target->x, target->y, target->z, 171);

								createParticleSpellPinpointTarget(target, caster->getUID(), 1774, duration, spell->ID);
								serverSpawnMiscParticles(target, PARTICLE_EFFECT_PINPOINT, 1774, caster->getUID(), duration, spell->ID);
								if ( players[player]->isLocalPlayer() )
								{
									FollowerMenu[player].followerToCommand = target;
									FollowerMenu[player].initfollowerMenuGUICursor(true); // set gui_mode to follower menu
								}
								else
								{
									if ( player >= 1 && player < MAXPLAYERS )
									{
										strcpy((char*)net_packet->data, "COMD");
										SDLNet_Write32(target->getUID(), &net_packet->data[4]);
										net_packet->address.host = net_clients[player - 1].host;
										net_packet->address.port = net_clients[player - 1].port;
										net_packet->len = 8;
										sendPacketSafe(net_sock, -1, net_packet, player - 1);
									}
								}
								target->monsterReleaseAttackTarget();
								for ( node_t* node = map.creatures->first; node != nullptr; node = node->next )
								{
									Entity* entity2 = (Entity*)node->element;
									if ( !entity2 ) { continue; }
									if ( entity2->behavior == &actMonster && entity2 != target )
									{
										if ( entity2->monsterAllyGetPlayerLeader() && ((Uint32)entity2->monsterTarget == target->getUID()) )
										{
											entity2->monsterReleaseAttackTarget(); // player allies stop attacking this target
										}
									}
								}
							}
							found = true;

							if ( !effect )
							{
								messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(255, 0, 0),
									*targetStats, Language::get(2905), Language::get(2906), MSG_COMBAT);
							}
						}
					}
				}
				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6498));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 167, 128);
			}
		}
		else if ( spell->ID == SPELL_CURSE_FLESH )
		{
			if ( caster )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps && castSpellProps->targetUID != 0 )
				{
					if ( Entity* target = uidToEntity(castSpellProps->targetUID) )
					{
						if ( Stat* targetStats = target->getStats() )
						{
							int duration = 5 * TICKS_PER_SECOND;
							if ( target->setEffect(EFF_CURSE_FLESH, (Uint8)(caster->behavior != &actPlayer ? MAXPLAYERS + 1 : caster->skill[2] + 1), duration, false, true, true) )
							{
								playSoundEntity(caster, 167, 128);
								effect = true;
								messagePlayerColor(caster->isEntityPlayer(),
									MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6576));
								spawnMagicEffectParticles(target->x, target->y, target->z, 171);
							}
							found = true;

							if ( !effect )
							{
								messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(255, 0, 0),
									*targetStats, Language::get(2905), Language::get(2906), MSG_COMBAT);
							}
						}
					}
				}
				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6498));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 167, 128);
			}
		}
		else if ( spell->ID == SPELL_WINDGATE )
		{
			if ( caster )
			{
				bool found = false;
				if ( castSpellProps && castSpellProps->wallDir >= 1 )
				{
					found = true;

					int duration = 5 * TICKS_PER_SECOND;
					int length = 2;
					createWindMagic(caster->getUID(), castSpellProps->target_x, castSpellProps->target_y, duration, castSpellProps->wallDir, length);
					Uint32 data = (castSpellProps->wallDir) & 0xF;
					data |= ((length) & 0xF) << 4;
					serverSpawnMiscParticlesAtLocation(castSpellProps->target_x, castSpellProps->target_y, 0,
						PARTICLE_EFFECT_WINDGATE, 982, duration, data);
				}
				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6498));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 982);
				playSoundEntity(caster, 167, 128);
			}
		}
		else if ( spell->ID == SPELL_TUNNEL )
		{
			if ( caster )
			{
				bool found = false;
				if ( castSpellProps && castSpellProps->wallDir >= 1 )
				{
					found = true;

					int duration = 5 * TICKS_PER_SECOND;
					createTunnelPortal(castSpellProps->target_x, castSpellProps->target_y, duration, castSpellProps->wallDir);
				}
				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6498));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 982);
				playSoundEntity(caster, 167, 128);
			}
		}
		else if ( spell->ID == SPELL_SABOTAGE || spell->ID == SPELL_HARVEST_TRAP )
		{
			if ( caster )
			{
				bool found = false;
				int scrapMetal = 0;
				int scrapMagic = 0;
				if ( castSpellProps && castSpellProps->targetUID != 0 )
				{
					if ( Entity* target = uidToEntity(castSpellProps->targetUID) )
					{
						if ( castSpellProps->wallDir > 0 && (target->behavior == &actArrowTrap || target->behavior == &actMagicTrap) )
						{
							real_t x = static_cast<int>(target->x / 16);
							real_t y = static_cast<int>(target->y / 16);
							found = true;
							if ( castSpellProps->wallDir == 1 )
							{
								x = x * 16.0 + 16.001 + 2.0;
								y = y * 16.0 + 8.0;
							}
							else if ( castSpellProps->wallDir == 3 )
							{
								x = x * 16.0 - 0.001 - 2.0;
								y = y * 16.0 + 8.0;
							}
							else if ( castSpellProps->wallDir == 2 )
							{
								x = x * 16.0 + 8.0;
								y = y * 16.0 + 16.001 + 2.0;
							}
							else if ( castSpellProps->wallDir == 4 )
							{
								x = x * 16.0 + 8.0;
								y = y * 16.0 - 0.001 - 2.0;
							}

							if ( target->actTrapSabotaged == 0 )
							{
								spawnExplosion(x, y, target->z);
								target->actTrapSabotaged = caster->isEntityPlayer() ? caster->skill[2] + 1 : MAXPLAYERS + 1;
								serverUpdateEntitySkill(target, 30);
								messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6659));
								if ( spell->ID == SPELL_HARVEST_TRAP )
								{
									scrapMetal = 10;
									scrapMagic = 10;
								}
							}
							else
							{
								messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6658));
							}
						}
						else if ( target->behavior == &actBoulderTrapHole
							|| target->behavior == &actMagicTrapCeiling 
							|| target->behavior == &actSpearTrap )
						{
							if ( target->behavior == &actBoulderTrapHole )
							{
								if ( Entity* parent = uidToEntity(target->parent) )
								{
									found = true;
									if ( parent->actTrapSabotaged == 0 )
									{
										spawnExplosion(target->x, target->y, target->z);
										parent->actTrapSabotaged = caster->isEntityPlayer() ? caster->skill[2] + 1 : MAXPLAYERS + 1;
										serverUpdateEntitySkill(parent, 30);
										messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6659));
										if ( spell->ID == SPELL_HARVEST_TRAP )
										{
											scrapMetal = 10;
											scrapMagic = 10;
										}
									}
									else
									{
										messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6658));
									}
								}
							}
							else
							{
								found = true;
								if ( target->actTrapSabotaged == 0 )
								{
									if ( target->behavior == &actSpearTrap )
									{
										spawnExplosion(target->x, target->y, 7.5);
									}
									else
									{
										spawnExplosion(target->x, target->y, target->z);
									}
									target->actTrapSabotaged = caster->isEntityPlayer() ? caster->skill[2] + 1 : MAXPLAYERS + 1;
									serverUpdateEntitySkill(target, 30);
									messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6659));
									if ( spell->ID == SPELL_HARVEST_TRAP )
									{
										scrapMetal = 10;
										scrapMagic = 10;
									}
								}
								else
								{
									messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6658));
								}
							}
						}
						else if ( spell->ID == SPELL_SABOTAGE && (target->behavior == &actMonster || target->behavior == &actChest) )
						{
							found = true;
							int trapResist = 0;
							int resistance = 0;
							DamageGib dmgGib = DMG_DEFAULT;
							real_t damageMultiplier = 1.0;
							magicSetResistance(target, caster, resistance, damageMultiplier, dmgGib, trapResist);
							bool effect = false;
							int damage = element->damage;
							if ( target->behavior == &actChest )
							{
								if ( effect = applyGenericMagicDamage(caster, target, *caster, spell->ID, damage, true) )
								{
									spawnExplosion(target->x, target->y, 0.0);
								}
							}
							else if ( Stat* targetStats = target->getStats() )
							{
								if ( targetStats->type == CRYSTALGOLEM 
									|| targetStats->type == AUTOMATON
									|| targetStats->type == MINIMIMIC
									|| targetStats->type == MIMIC
									)
								{
									if ( effect = applyGenericMagicDamage(caster, target, *caster, spell->ID, damage, true) )
									{
										spawnExplosion(target->x, target->y, 0.0);
									}
								}
								if ( !effect )
								{
									messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(255, 0, 0),
										*targetStats, Language::get(2905), Language::get(2906), MSG_COMBAT);
								}
							}
						}

						if ( spell->ID == SPELL_HARVEST_TRAP )
						{
							bool anyDrop = false;
							while ( scrapMetal > 0 || scrapMagic > 0 )
							{
								Item* item = nullptr;
								int qty = 3 + local_rng.rand() % 5;
								if ( scrapMetal > 0 )
								{
									qty = std::min(scrapMetal, qty);
									scrapMetal -= qty;
									item = newItem(TOOL_METAL_SCRAP, DECREPIT, 0, qty, 0, true, nullptr);
								}
								else if ( scrapMagic > 0 )
								{
									qty = std::min(scrapMagic, qty);
									scrapMagic -= qty;
									item = newItem(TOOL_MAGIC_SCRAP, DECREPIT, 0, qty, 0, true, nullptr);
								}
								if ( item )
								{
									if ( Entity* dropped = dropItemMonster(item, target, nullptr, item->count) )
									{
										anyDrop = true;
										dropped->yaw = local_rng.rand() % 360 * PI / 180;
										dropped->vel_x = (0.5 + .005 * (local_rng.rand() % 11)) * cos(dropped->yaw);
										dropped->vel_y = (0.5 + .005 * (local_rng.rand() % 11)) * sin(dropped->yaw);
										if ( target->behavior == &actMagicTrap
											|| target->behavior == &actArrowTrap )
										{
											real_t x = static_cast<int>(target->x / 16);
											real_t y = static_cast<int>(target->y / 16);
											if ( castSpellProps->wallDir == 1 )
											{
												x = x * 16.0 + 16.001 + 2.0;
												y = y * 16.0 + 8.0;
											}
											else if ( castSpellProps->wallDir == 3 )
											{
												x = x * 16.0 - 0.001 - 2.0;
												y = y * 16.0 + 8.0;
											}
											else if ( castSpellProps->wallDir == 2 )
											{
												x = x * 16.0 + 8.0;
												y = y * 16.0 + 16.001 + 2.0;
											}
											else if ( castSpellProps->wallDir == 4 )
											{
												x = x * 16.0 + 8.0;
												y = y * 16.0 - 0.001 - 2.0;
											}
											dropped->x = x;
											dropped->y = y;
										}
										dropped->vel_z = (-10 - local_rng.rand() % 20) * .01;
										dropped->z = target->z;
										dropped->z = std::min(5.5, dropped->z);
									}
								}
							}

							if ( anyDrop )
							{
								messagePlayerColor(caster->isEntityPlayer(), MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6662));
							}
						}
					}
				}
				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6498));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 982);
				playSoundEntity(caster, 167, 128);
			}
		}
		else if ( spell->ID == SPELL_DEFACE )
		{
			if ( caster )
			{
				bool found = false;
				if ( castSpellProps && castSpellProps->targetUID != 0 )
				{
					if ( Entity* target = uidToEntity(castSpellProps->targetUID) )
					{
						struct DefaceEvent_t
						{
							Monster type = NOTHING;
							Item* item = nullptr;
							int weight = 1;
							DefaceEvent_t(Monster _type, Item* _item, int _weight) {
								item = _item;
								type = _type;
								weight = _weight;
							}
							~DefaceEvent_t()
							{
								if ( item )
								{
									free(item);
								}
							}
						};
						std::vector<DefaceEvent_t> itemPool;
						auto& rng = target->entity_rng ? *target->entity_rng : local_rng;
						if ( target->behavior == &actSink )
						{
							found = true;
							messagePlayerColor(caster->isEntityPlayer(), MESSAGE_HINT,
								makeColorRGB(0, 255, 0),
								Language::get(6688), Language::get(4354));

							itemPool.reserve(4);
							itemPool.emplace_back(
								NOTHING, 
								newItem(TOOL_METAL_SCRAP, DECREPIT, 0, 5 + rng.rand() % 11, 0, false, nullptr), 
								10);

							itemPool.emplace_back(
								NOTHING,
								newItem(TOOL_MAGIC_SCRAP, DECREPIT, 0, 5 + rng.rand() % 11, 0, false, nullptr),
								10);

							itemPool.emplace_back(
								NOTHING,
								newItem(POTION_SICKNESS, DECREPIT, 0, 2 + rng.rand() % 4, 0, false, nullptr),
								10);

							itemPool.emplace_back(
								SLIME, 
								nullptr, 
								10);

							playSoundEntity(target, 67, 128);
						}
						else if ( target->behavior == &actHeadstone )
						{
							found = true;
							messagePlayerColor(caster->isEntityPlayer(), MESSAGE_HINT, 
								makeColorRGB(0, 255, 0), 
								Language::get(6688), Language::get(4357));

							itemPool.reserve(5);
							itemPool.emplace_back(
								NOTHING,
								newItem(GEM_ROCK, DECREPIT, -1, 2 + rng.rand() % 4, rng.rand(), false, nullptr),
								10);

							itemPool.emplace_back(
								NOTHING,
								newItem(FOOD_MEAT, DECREPIT, -1, 1 + rng.rand() % 2, rng.rand(), false, nullptr),
								10);

							itemPool.emplace_back(
								NOTHING,
								newItem(TUNIC, DECREPIT, -1, 1 + rng.rand() % 2, rng.rand(), false, nullptr),
								10);

							itemPool.emplace_back(
								SHADOW,
								nullptr,
								10);

							itemPool.emplace_back(
								GHOUL,
								nullptr,
								10);

							playSoundEntity(target, 67, 128);
						}

						if ( found )
						{
							target->flags[PASSABLE] = true;
							if ( itemPool.size() )
							{
								std::vector<unsigned int> weights(itemPool.size());
								int index = -1;
								for ( auto& entry : itemPool )
								{
									++index;
									weights[index] = entry.weight;
								}
								int pick = rng.discrete(weights.data(), weights.size());

								if ( itemPool[pick].item )
								{
									if ( Entity* dropped = dropItemMonster(itemPool[pick].item, target, nullptr, itemPool[pick].item->count) )
									{
										dropped->yaw = local_rng.rand() % 360 * PI / 180;
										dropped->vel_x = (0.5 + .005 * (local_rng.rand() % 11)) * cos(dropped->yaw);
										dropped->vel_y = (0.5 + .005 * (local_rng.rand() % 11)) * sin(dropped->yaw);
										dropped->vel_z = (-10 - local_rng.rand() % 20) * .02;
										dropped->z = target->z;
										dropped->z = std::min(5.5, dropped->z);

										itemPool[pick].item = nullptr;
									}
								}
								else if ( itemPool[pick].type != NOTHING )
								{
									if ( Entity* monster = summonMonster(itemPool[pick].type, target->x, target->y) )
									{
										monster->seedEntityRNG(rng.rand());
										monster->monsterAcquireAttackTarget(*caster, MONSTER_STATE_PATH, true);
										if ( itemPool[pick].type == SLIME )
										{
											slimeSetType(monster, monster->getStats(), true, &rng);
										}
										messagePlayerColor(caster->isEntityPlayer(), MESSAGE_HINT,
											makeColorRGB(255, 0, 0),
											Language::get(6689), getMonsterLocalizedName(itemPool[pick].type).c_str());
									}
								}
							}

							spawnMagicEffectParticles(target->x, target->y, target->z, 171);
							createParticleRock(target, 78);
							playSoundEntity(target, 167, 128);
							if ( multiplayer == SERVER )
							{
								serverSpawnMiscParticles(target, PARTICLE_EFFECT_ABILITY_ROCK, 78);
							}
							list_RemoveNode(target->mynode);
						}
					}
				}
				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6498));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 167, 128);
			}
		}
		else if ( spell->ID == SPELL_KINETIC_PUSH )
		{
			if ( caster && (caster->behavior == &actPlayer || caster->behavior == &actMonster) )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps && castSpellProps->targetUID != 0 )
				{
					if ( Entity* target = uidToEntity(castSpellProps->targetUID) )
					{
						found = true;
						if ( target->behavior == &actBoulder && caster->behavior == &actPlayer )
						{
							target->skill[15] = player + 1;
						}
						else if ( Stat* targetStats = target->getStats() )
						{
							if ( targetStats->type == EARTH_ELEMENTAL )
							{
								if ( target->setEffect(EFF_KNOCKBACK, true, 40, false) )
								{
									target->setEffect(EFF_DASH, true, 40, false);
									target->setEffect(EFF_STUNNED, true, 40, false);
									target->attack(MONSTER_POSE_EARTH_ELEMENTAL_ROLL, 0, nullptr);
									effect = true;
									real_t pushbackMultiplier = 2.0;

									real_t tangent = atan2(target->y - caster->y, target->x - caster->x);
									target->vel_x = cos(tangent) * pushbackMultiplier;
									target->vel_y = sin(tangent) * pushbackMultiplier;
									target->monsterKnockbackVelocity = 0.005;
									target->monsterKnockbackTangentDir = tangent;
									target->monsterKnockbackUID = caster->getUID();
								}
							}
							else if ( target->setEffect(EFF_KNOCKBACK, true, 30, false) )
							{
								effect = true;
								real_t pushbackMultiplier = 0.6;
								if ( caster )
								{
									real_t dist = entityDist(caster, target);
									if ( dist < TOUCHRANGE )
									{
										pushbackMultiplier += 0.5;
									}
								}
								if ( target->behavior == &actMonster )
								{
									real_t tangent = atan2(target->y - caster->y, target->x - caster->x);
									target->vel_x = cos(tangent) * pushbackMultiplier;
									target->vel_y = sin(tangent) * pushbackMultiplier;
									target->monsterKnockbackVelocity = 0.01;
									target->monsterKnockbackTangentDir = tangent;
									target->monsterKnockbackUID = caster->getUID();
								}
								else if ( target->behavior == &actPlayer )
								{
									if ( !players[target->skill[2]]->isLocalPlayer() )
									{
										target->monsterKnockbackVelocity = pushbackMultiplier;
										target->monsterKnockbackTangentDir = caster->yaw;
										serverUpdateEntityFSkill(target, 11);
										serverUpdateEntityFSkill(target, 9);
									}
									else
									{
										target->monsterKnockbackVelocity = pushbackMultiplier;
										target->monsterKnockbackTangentDir = caster->yaw;
									}
								}
							}

							if ( !effect )
							{
								messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(255, 0, 0),
									*targetStats, Language::get(2905), Language::get(2906), MSG_COMBAT);
							}
						}
						spawnMagicEffectParticles(target->x, target->y, target->z, 171);
					}
				}
				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6498));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 167, 128);
			}
		}
		else if ( spell->ID == SPELL_TELEKINESIS )
		{
			if ( caster && caster->behavior == &actPlayer )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps && castSpellProps->targetUID != 0 )
				{
					if ( Entity* target = uidToEntity(castSpellProps->targetUID) )
					{
						if ( target->behavior == &actBoulder )
						{
							target->skill[14] = player + 1;
						}
						else if ( target->behavior == &actMonster && target->getMonsterTypeFromSprite() == EARTH_ELEMENTAL )
						{
							if ( target->setEffect(EFF_KNOCKBACK, true, 40, false) )
							{
								target->setEffect(EFF_DASH, true, 40, false);
								target->setEffect(EFF_STUNNED, true, 40, false);
								target->attack(MONSTER_POSE_EARTH_ELEMENTAL_ROLL, 0, nullptr);
								effect = true;
								real_t pushbackMultiplier = 2.0;
								/*if ( caster )
								{
									real_t dist = entityDist(caster, target);
									if ( dist < TOUCHRANGE )
									{
										pushbackMultiplier += 0.5;
									}
								}*/
								real_t tangent = atan2(target->y - caster->y, target->x - caster->x) + PI;
								target->vel_x = cos(tangent) * pushbackMultiplier;
								target->vel_y = sin(tangent) * pushbackMultiplier;
								target->monsterKnockbackVelocity = 0.005;
								target->monsterKnockbackTangentDir = tangent;
								target->monsterKnockbackUID = caster->getUID();
							}
						}
						else
						{
							if ( players[player]->isLocalPlayer() )
							{
								players[player]->magic.telekinesisTarget = castSpellProps->targetUID;
							}
							else
							{
								client_selected[player] = target;
								inrange[player] = true;
							}
						}
						found = true;
						spawnMagicEffectParticles(target->x, target->y, target->z, 171);
					}
				}
				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6498));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 167, 128);
			}
		}
		else if ( spell->ID == SPELL_DISARM || spell->ID == SPELL_STRIP )
		{
			if ( caster && caster->behavior == &actPlayer )
			{
				bool found = false;
				bool effect = false;
				if ( castSpellProps && castSpellProps->targetUID != 0 )
				{
					if ( Entity* target = uidToEntity(castSpellProps->targetUID) )
					{
						if ( Stat* targetStats = target->getStats() )
						{
							bool doEffect = true;

							std::vector<Item**> slots;
							slots.push_back(&targetStats->weapon);
							if ( spell->ID == SPELL_STRIP )
							{
								slots.push_back(&targetStats->shield);
								slots.push_back(&targetStats->helmet);
								slots.push_back(&targetStats->breastplate);
								slots.push_back(&targetStats->gloves);
								slots.push_back(&targetStats->shoes);
								slots.push_back(&targetStats->cloak);
								slots.push_back(&targetStats->amulet);
								slots.push_back(&targetStats->ring);
								slots.push_back(&targetStats->mask);
							}

							for ( auto slot : slots )
							{
								if ( !(*slot) )
								{
									continue;
								}
								if ( target->behavior == &actMonster 
										&& ((itemCategory(*slot) == SPELLBOOK) || !(*slot)->isDroppable) )
								{
									doEffect = false;
								}
								else if ( targetStats->type == LICH || targetStats->type == LICH_FIRE || targetStats->type == LICH_ICE || targetStats->type == DEVIL
									|| targetStats->type == SHADOW )
								{
									doEffect = false;
								}
								else if ( target->behavior == &actMonster
									&& (target->monsterAllySummonRank != 0
										|| (targetStats->type == INCUBUS && !strncmp(targetStats->name, "inner demon", strlen("inner demon"))))
									)
								{
									doEffect = false;
								}

								if ( doEffect )
								{
									if ( Entity* dropped = dropItemMonster(*slot, target, targetStats, (*slot)->count) )
									{
										effect = true;

										dropped->itemDelayMonsterPickingUp = TICKS_PER_SECOND * 5;
										double tangent = atan2(target->y - caster->y, target->x - caster->x) + PI;
										dropped->yaw = tangent + PI;
										dropped->vel_x = (1.5 + .025 * (local_rng.rand() % 11)) * cos(tangent);
										dropped->vel_y = (1.5 + .025 * (local_rng.rand() % 11)) * sin(tangent);
										dropped->vel_z = (-10 - local_rng.rand() % 20) * .01;
										dropped->flags[USERFLAG1] = false;
										dropped->itemOriginalOwner = target->getUID();
									}
								}
							}
							found = true;

							spawnMagicEffectParticles(target->x, target->y, target->z, 171);
							if ( !effect )
							{
								messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(255, 0, 0),
									*targetStats, Language::get(2905), Language::get(2906), MSG_COMBAT);
							}
							else
							{
								playSoundEntity(caster, 167, 128);
								if ( spell->ID == SPELL_DISARM )
								{
									messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(0, 255, 0),
										*targetStats, Language::get(6644), Language::get(6645), MSG_COMBAT);
								}
								else if ( spell->ID == SPELL_STRIP )
								{
									messagePlayerMonsterEvent(caster->isEntityPlayer(), makeColorRGB(0, 255, 0),
										*targetStats, Language::get(6646), Language::get(6647), MSG_COMBAT);
								}
							}
						}
					}
				}
				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6498));
				}
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				playSoundEntity(caster, 167, 128);
			}
		}
		else if ( spell->ID == SPELL_DETECT_ENEMY )
		{
			if ( caster )
			{
				auto compFunc = [](std::pair<Sint32, Entity*>& lhs, std::pair<Sint32, Entity*>& rhs)
				{
					return lhs.first > rhs.first;
				};
				std::priority_queue<std::pair<Sint32, Entity*>, std::vector<std::pair<Sint32, Entity*>>, decltype(compFunc)> enemies(compFunc);
				bool found = false;
				for ( node_t* node = map.creatures->first; node; node = node->next )
				{
					if ( Entity* entity = getSpellTarget(node, 10000, caster, false, TARGET_ENEMY) )
					{
						if ( entity->behavior == actMonster )
						{
							if ( Stat* stats = entity->getStats() )
							{
								enemies.push(std::make_pair(stats->HP, entity));
							}
						}
					}
				}

				if ( enemies.size() )
				{
					auto entity = enemies.top().second;
					if ( Stat* stats = entity->getStats() )
					{
						int duration = 30 * TICKS_PER_SECOND;
						if ( entity->setEffect(EFF_DETECT_ENEMY,
							(Uint8)(caster->behavior != &actPlayer ? MAXPLAYERS + 1 : caster->skill[2] + 1), duration, true, true, true) )
						{
							messagePlayerColor(caster->isEntityPlayer(),
								MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6495));

							createParticleSpellPinpointTarget(entity, caster->getUID(), 1774, duration, spell->ID);
							serverSpawnMiscParticles(entity, PARTICLE_EFFECT_PINPOINT, 1774, caster->getUID(), duration, spell->ID);

							if ( !strcmp(stats->name, "") )
							{
								updateEnemyBar(caster, entity, getMonsterLocalizedName(stats->type).c_str(), stats->HP, stats->MAXHP,
									false, DMG_DETECT_MONSTER);
							}
							else
							{
								updateEnemyBar(caster, entity, stats->name, stats->HP, stats->MAXHP,
									false, DMG_DETECT_MONSTER);
							}
							found = true;
						}
					}
				}

				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(3715));
				}
				if ( caster )
				{
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				}
				playSoundEntity(caster, 167, 128);
			}
		}
		else if ( spell->ID == SPELL_PINPOINT || spell->ID == SPELL_PENANCE )
		{
			if ( caster )
			{
				std::vector<Entity*> enemies;
				bool found = false;
				for ( node_t* node = map.creatures->first; node; node = node->next )
				{
					if ( Entity* entity = getSpellTarget(node, 10000, caster, false, TARGET_ENEMY) )
					{
						if ( Stat* stats = entity->getStats() )
						{
							if ( spell->ID == SPELL_PINPOINT )
							{
								auto effectStrength = stats->getEffectActive(EFF_PINPOINT);
								if ( effectStrength != (caster->skill[2] + 1) )
								{
									enemies.push_back(entity);
								}
							}
							else if ( spell->ID == SPELL_PENANCE )
							{
								auto effectStrength = stats->getEffectActive(EFF_PENANCE);
								if ( effectStrength != (caster->skill[2] + 1) )
								{
									enemies.push_back(entity);
								}
							}
						}
					}
				}
				while ( enemies.size() )
				{
					int pick = local_rng.rand() % enemies.size();
					auto entity = enemies[pick];
					if ( spell->ID == SPELL_PINPOINT )
					{
						int duration = 30 * TICKS_PER_SECOND;
						if ( entity->setEffect(EFF_PINPOINT, (Uint8)(caster->behavior != &actPlayer ? MAXPLAYERS + 1 : caster->skill[2] + 1), duration, true, true, true) )
						{
							messagePlayerColor(caster->isEntityPlayer(),
								MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6480));
							createParticleSpellPinpointTarget(entity, caster->getUID(), 1767, duration, spell->ID);
							serverSpawnMiscParticles(entity, PARTICLE_EFFECT_PINPOINT, 1767, caster->getUID(), duration, spell->ID);
							found = true;
							break;
						}
					}
					else
					{
						int duration = 30 * TICKS_PER_SECOND;
						if ( entity->setEffect(EFF_PENANCE, (Uint8)(caster->behavior != &actPlayer ? MAXPLAYERS + 1 : caster->skill[2] + 1), duration, true, true, true) )
						{
							messagePlayerColor(caster->isEntityPlayer(),
								MESSAGE_HINT, makeColorRGB(0, 255, 0), Language::get(6487));
							createParticleSpellPinpointTarget(entity, caster->getUID(), 1773, duration, spell->ID);
							serverSpawnMiscParticles(entity, PARTICLE_EFFECT_PINPOINT, 1773, caster->getUID(), duration, spell->ID);

							entity->monsterReleaseAttackTarget();
							for ( node_t* node = map.creatures->first; node != nullptr; node = node->next )
							{
								Entity* entity2 = (Entity*)node->element;
								if ( !entity2 ) { continue; }
								if ( entity2->behavior == &actMonster && entity2 != entity )
								{
									if ( entity2->monsterAllyGetPlayerLeader() && ((Uint32)entity2->monsterTarget == entity->getUID()) )
									{
										entity2->monsterReleaseAttackTarget(); // player allies stop attacking this target
									}
								}
							}
							found = true;
							break;
						}
					}
					enemies.erase(enemies.begin() + pick);
				}

				if ( !found )
				{
					messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(3715));
				}
				if ( caster )
				{
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				}
				playSoundEntity(caster, 167, 128);
			}
		}
		else if ( !strcmp(element->element_internal_name, spellElement_detectFood.element_internal_name) )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i] && caster && (caster == players[i]->entity) )
				{
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
					spell_detectFoodEffectOnMap(i);
				}
			}
			playSoundEntity(caster, 167, 128);
		}
		else if ( !strcmp(element->element_internal_name, spellElement_salvageItem.element_internal_name) )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i] && caster && (caster == players[i]->entity) )
				{
					int searchx = static_cast<int>(caster->x + 32 * cos(caster->yaw)) >> 4;
					int searchy = static_cast<int>(caster->y + 32 * sin(caster->yaw)) >> 4;
					std::vector<list_t*> itemsOnGround = TileEntityList.getEntitiesWithinRadius(searchx, searchy, 2);
					int totalMetal = 0;
					int totalMagic = 0;
					int numItems = 0;
					std::set<std::pair<int, int>> effectCoordinates;
					for ( auto it = itemsOnGround.begin(); it != itemsOnGround.end(); ++it )
					{
						list_t* currentList = *it;
						node_t* itemNode;
						node_t* nextItemNode = nullptr;
						for ( itemNode = currentList->first; itemNode != nullptr; itemNode = nextItemNode )
						{
							nextItemNode = itemNode->next;
							Entity* itemEntity = (Entity*)itemNode->element;
							if ( itemEntity && !itemEntity->flags[INVISIBLE] && itemEntity->behavior == &actItem && entityDist(itemEntity, caster) < TOUCHRANGE )
							{
								Item* toSalvage = newItemFromEntity(itemEntity);
								if ( toSalvage && GenericGUI[i].isItemSalvageable(toSalvage, i) )
								{
									int metal = 0;
									int magic = 0;
									GenericGUIMenu::tinkeringGetItemValue(toSalvage, &metal, &magic);
									totalMetal += metal;
									totalMagic += magic;
									++numItems;
									effectCoordinates.insert(std::make_pair(static_cast<int>(itemEntity->x) >> 4, static_cast<int>(itemEntity->y) >> 4));

									// delete item on ground.
									itemEntity->removeLightField();
									list_RemoveNode(itemEntity->mynode);
								}
								free(toSalvage);
							}
						}
					}
					if ( totalMetal == 0 && totalMagic == 0 )
					{
						messagePlayer(i, MESSAGE_COMBAT, Language::get(3713));
						playSoundEntity(caster, 163, 128);
					}
					else
					{
						messagePlayerColor(i, MESSAGE_INVENTORY, makeColorRGB(0, 255, 0), Language::get(3712), numItems);
						playSoundEntity(caster, 167, 128);
					}

					int pickedUpMetal = 0;
					while ( totalMetal > 0 )
					{
						int metal = std::min(totalMetal, SCRAP_MAX_STACK_QTY - 1);
						totalMetal -= metal;
						Item* crafted = newItem(TOOL_METAL_SCRAP, DECREPIT, 0, metal, 0, true, nullptr);
						if ( crafted )
						{
							Item* pickedUp = itemPickup(player, crafted);
							pickedUpMetal += metal;
							if ( i == 0 || players[i]->isLocalPlayer() ) // server/singleplayer
							{
								free(crafted); // if player != clientnum, then crafted == pickedUp
							}
							if ( i != 0 && !players[i]->isLocalPlayer() )
							{
								free(pickedUp);
							}
						}
					}
					if ( pickedUpMetal > 0 )
					{
						messagePlayer(player, MESSAGE_INVENTORY, Language::get(3665), pickedUpMetal, items[TOOL_METAL_SCRAP].getIdentifiedName());
					}
					int pickedUpMagic = 0;
					while ( totalMagic > 0 )
					{
						int magic = std::min(totalMagic, SCRAP_MAX_STACK_QTY - 1);
						totalMagic -= magic;
						Item* crafted = newItem(TOOL_MAGIC_SCRAP, DECREPIT, 0, magic, 0, true, nullptr);
						if ( crafted )
						{
							Item* pickedUp = itemPickup(player, crafted);
							pickedUpMagic += magic;
							if ( i == 0 || players[i]->isLocalPlayer() ) // server/singleplayer
							{
								free(crafted); // if player != clientnum, then crafted == pickedUp
							}
							if ( i != 0 && !players[i]->isLocalPlayer() )
							{
								free(pickedUp);
							}
						}
					}
					if ( pickedUpMagic > 0 )
					{
						messagePlayer(player, MESSAGE_INVENTORY, Language::get(3665), pickedUpMagic, items[TOOL_MAGIC_SCRAP].getIdentifiedName());
					}
					if ( !effectCoordinates.empty() )
					{
						for ( auto it = effectCoordinates.begin(); it != effectCoordinates.end(); ++it )
						{
							std::pair<int, int> coords = *it;
							spawnMagicEffectParticles(coords.first * 16 + 8, coords.second * 16 + 8, 7, 171);
						}
					}
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
					break;
				}
			}
		}
		else if ( !strcmp(element->element_internal_name, spellElement_trollsBlood.element_internal_name) )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i] && caster && (caster == players[i]->entity) )
				{
					int amount = element->duration;
					amount += ((spellBookBonusPercent * 2 / 100.f) * amount); // 100-200%

					if ( overdrewIntoHP )
					{
						amount /= 4;
						messagePlayerColor(player, MESSAGE_COMBAT, makeColorRGB(255, 255, 255), Language::get(3400));
					}

					caster->setEffect(EFF_TROLLS_BLOOD, true, amount, true);
					Uint32 color = makeColorRGB(0, 255, 0);
					messagePlayerColor(i, MESSAGE_HINT, color, Language::get(3490));
					for ( node = map.creatures->first; node; node = node->next )
					{
						Entity* entity = (Entity*)(node->element);
						if ( !entity || entity == caster )
						{
							continue;
						}
						if ( entity->behavior != &actPlayer && entity->behavior != &actMonster )
						{
							continue;
						}

						if ( entityDist(entity, caster) <= HEAL_RADIUS && entity->checkFriend(caster) )
						{
							entity->setEffect(EFF_TROLLS_BLOOD, true, amount, true);
							playSoundEntity(entity, 168, 128);
							spawnMagicEffectParticles(entity->x, entity->y, entity->z, 169);
							if ( entity->behavior == &actPlayer )
							{
								messagePlayerColor(entity->skill[2], MESSAGE_HINT, color, Language::get(3490));
							}
						}
					}
					break;
				}
			}
			if ( caster->behavior == &actMonster )
			{
				caster->setEffect(EFF_TROLLS_BLOOD, true, element->duration, true);
			}

			playSoundEntity(caster, 168, 128);
			spawnMagicEffectParticles(caster->x, caster->y, caster->z, 169);
		}
		else if ( !strcmp(element->element_internal_name, spellElement_flutter.element_internal_name) )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i] && caster && (caster == players[i]->entity) )
				{
					//Duration for flutter.
					int duration = element->duration;

					if ( caster->getStats() && !caster->getStats()->getEffectActive(EFF_FLUTTER) )
					{
						achievementObserver.playerAchievements[i].flutterShyCoordinates = std::make_pair(caster->x, caster->y);
					}

					if ( caster->setEffect(EFF_FLUTTER, true, duration, true) )
					{
						messagePlayerColor(i, MESSAGE_STATUS, uint32ColorGreen, Language::get(3767));
						playSoundEntity(caster, 178, 128);
						spawnMagicEffectParticles(caster->x, caster->y, caster->z, 170);
					}
					break;
				}
			}
		}
		else if ( spell->ID == SPELL_BLOOD_WARD )
		{
			if ( caster )
			{
				int duration = element->duration;
				if ( caster->behavior == &actPlayer )
				{
					node_t* spellnode = list_AddNodeLast(&caster->getStats()->magic_effects);
					spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
					channeled_spell = (spell_t*)(spellnode->element);
					channeled_spell->magic_effects_node = spellnode;
					spellnode->size = sizeof(spell_t);
					((spell_t*)spellnode->element)->caster = caster->getUID();
					spellnode->deconstructor = &spellDeconstructor;
					channeled_spell->channel_duration = duration; //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
				}
				else
				{
					duration = element->duration;
				}
				if ( caster->setEffect(EFF_BLOOD_WARD, true, duration, false) )
				{
					messagePlayerColor(caster->isEntityPlayer(), MESSAGE_STATUS, uint32ColorGreen, Language::get(6502));
					playSoundEntity(caster, 178, 128);
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 170);
				}
			}
		}
		else if ( spell->ID == SPELL_TRUE_BLOOD )
		{
			if ( caster )
			{
				int duration = element->duration;
				if ( caster->behavior == &actPlayer )
				{
					node_t* spellnode = list_AddNodeLast(&caster->getStats()->magic_effects);
					spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
					channeled_spell = (spell_t*)(spellnode->element);
					channeled_spell->magic_effects_node = spellnode;
					spellnode->size = sizeof(spell_t);
					((spell_t*)spellnode->element)->caster = caster->getUID();
					spellnode->deconstructor = &spellDeconstructor;
					channeled_spell->channel_duration = duration; //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
				}
				else
				{
					duration = element->duration;
				}
				if ( caster->setEffect(EFF_TRUE_BLOOD, true, duration, false) )
				{
					messagePlayerColor(caster->isEntityPlayer(), MESSAGE_STATUS, uint32ColorGreen, Language::get(6506));
					playSoundEntity(caster, 178, 128);
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 170);
				}
			}
		}
		else if ( spell->ID == SPELL_DIVINE_ZEAL )
		{
			if ( caster )
			{
				int duration = element->duration;
				if ( caster->behavior == &actPlayer )
				{
					node_t* spellnode = list_AddNodeLast(&caster->getStats()->magic_effects);
					spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
					channeled_spell = (spell_t*)(spellnode->element);
					channeled_spell->magic_effects_node = spellnode;
					spellnode->size = sizeof(spell_t);
					((spell_t*)spellnode->element)->caster = caster->getUID();
					spellnode->deconstructor = &spellDeconstructor;
					channeled_spell->channel_duration = duration; //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
				}
				else
				{
					duration = element->duration;
				}
				if ( caster->setEffect(EFF_DIVINE_ZEAL, true, duration, false) )
				{
					messagePlayerColor(caster->isEntityPlayer(), MESSAGE_STATUS, uint32ColorGreen, Language::get(6508));
					playSoundEntity(caster, 178, 128);
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 170);
				}
			}
		}
		else if ( spell->ID == SPELL_OVERCHARGE )
		{
			if ( caster )
			{
				int duration = element->duration;
				if ( caster->behavior == &actPlayer )
				{
					node_t* spellnode = list_AddNodeLast(&caster->getStats()->magic_effects);
					spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
					channeled_spell = (spell_t*)(spellnode->element);
					channeled_spell->magic_effects_node = spellnode;
					spellnode->size = sizeof(spell_t);
					((spell_t*)spellnode->element)->caster = caster->getUID();
					spellnode->deconstructor = &spellDeconstructor;
					channeled_spell->channel_duration = duration; //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
				}
				else
				{
					duration = element->duration;
				}
				if ( caster->setEffect(EFF_OVERCHARGE, true, duration, false) )
				{
					messagePlayerColor(caster->isEntityPlayer(), MESSAGE_STATUS, uint32ColorGreen, Language::get(6520));
					playSoundEntity(caster, 178, 128);
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 170);
				}
			}
		}
		else if ( spell->ID == SPELL_ENVENOM_WEAPON )
		{
			if ( caster )
			{
				int duration = element->duration;
				if ( caster->behavior == &actPlayer )
				{
					node_t* spellnode = list_AddNodeLast(&caster->getStats()->magic_effects);
					spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
					channeled_spell = (spell_t*)(spellnode->element);
					channeled_spell->magic_effects_node = spellnode;
					spellnode->size = sizeof(spell_t);
					((spell_t*)spellnode->element)->caster = caster->getUID();
					spellnode->deconstructor = &spellDeconstructor;
					channeled_spell->channel_duration = duration; //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
				}
				else
				{
					duration = element->duration;
				}
				if ( caster->setEffect(EFF_ENVENOM_WEAPON, true, duration, false) )
				{
					messagePlayerColor(caster->isEntityPlayer(), MESSAGE_STATUS, uint32ColorGreen, Language::get(6521));
					playSoundEntity(caster, 178, 128);
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 170);
				}
			}
		}
		else if ( spell->ID == SPELL_ABUNDANCE )
		{
			if ( caster )
			{
				int duration = element->duration;
				if ( caster->behavior == &actPlayer )
				{
					node_t* spellnode = list_AddNodeLast(&caster->getStats()->magic_effects);
					spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
					channeled_spell = (spell_t*)(spellnode->element);
					channeled_spell->magic_effects_node = spellnode;
					spellnode->size = sizeof(spell_t);
					((spell_t*)spellnode->element)->caster = caster->getUID();
					spellnode->deconstructor = &spellDeconstructor;
					channeled_spell->channel_duration = duration; //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
				}
				else
				{
					duration = element->duration;
				}
				if ( caster->setEffect(EFF_ABUNDANCE, true, duration, false) )
				{
					messagePlayerColor(caster->isEntityPlayer(), MESSAGE_STATUS, uint32ColorGreen, Language::get(6650));
					playSoundEntity(caster, 178, 128);
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 170);
				}
			}
		}
		else if ( spell->ID == SPELL_GREATER_ABUNDANCE )
		{
			if ( caster )
			{
				int duration = element->duration;
				if ( caster->behavior == &actPlayer )
				{
					node_t* spellnode = list_AddNodeLast(&caster->getStats()->magic_effects);
					spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
					channeled_spell = (spell_t*)(spellnode->element);
					channeled_spell->magic_effects_node = spellnode;
					spellnode->size = sizeof(spell_t);
					((spell_t*)spellnode->element)->caster = caster->getUID();
					spellnode->deconstructor = &spellDeconstructor;
					channeled_spell->channel_duration = duration; //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
				}
				else
				{
					duration = element->duration;
				}
				if ( caster->setEffect(EFF_GREATER_ABUNDANCE, true, duration, false) )
				{
					messagePlayerColor(caster->isEntityPlayer(), MESSAGE_STATUS, uint32ColorGreen, Language::get(6651));
					playSoundEntity(caster, 178, 128);
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 170);
				}
			}
		}
		else if ( spell->ID == SPELL_PRESERVE )
		{
			if ( caster )
			{
				int duration = element->duration;
				if ( caster->behavior == &actPlayer )
				{
					node_t* spellnode = list_AddNodeLast(&caster->getStats()->magic_effects);
					spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
					channeled_spell = (spell_t*)(spellnode->element);
					channeled_spell->magic_effects_node = spellnode;
					spellnode->size = sizeof(spell_t);
					((spell_t*)spellnode->element)->caster = caster->getUID();
					spellnode->deconstructor = &spellDeconstructor;
					channeled_spell->channel_duration = duration; //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
				}
				else
				{
					duration = element->duration;
				}
				if ( caster->setEffect(EFF_PRESERVE, true, duration, false) )
				{
					messagePlayerColor(caster->isEntityPlayer(), MESSAGE_STATUS, uint32ColorGreen, Language::get(6655));
					playSoundEntity(caster, 178, 128);
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 170);
				}
			}
		}
		else if ( spell->ID == SPELL_MIST_FORM )
		{
			if ( caster )
			{
				int duration = element->duration;
				if ( caster->behavior == &actPlayer )
				{
					node_t* spellnode = list_AddNodeLast(&caster->getStats()->magic_effects);
					spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
					channeled_spell = (spell_t*)(spellnode->element);
					channeled_spell->magic_effects_node = spellnode;
					spellnode->size = sizeof(spell_t);
					((spell_t*)spellnode->element)->caster = caster->getUID();
					spellnode->deconstructor = &spellDeconstructor;
					channeled_spell->channel_duration = duration; //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
				}
				else
				{
					duration = element->duration;
				}
				if ( caster->setEffect(EFF_MIST_FORM, true, duration, true) )
				{
					messagePlayerColor(caster->isEntityPlayer(), MESSAGE_STATUS, uint32ColorGreen, Language::get(6663));
					playSoundEntity(caster, 178, 128);
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 170);
				}
			}
		}
		else if ( spell->ID == SPELL_LIGHTEN_LOAD )
		{
			if ( caster )
			{
				int duration = element->duration;
				if ( caster->behavior == &actPlayer )
				{
					node_t* spellnode = list_AddNodeLast(&caster->getStats()->magic_effects);
					spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
					channeled_spell = (spell_t*)(spellnode->element);
					channeled_spell->magic_effects_node = spellnode;
					spellnode->size = sizeof(spell_t);
					((spell_t*)spellnode->element)->caster = caster->getUID();
					spellnode->deconstructor = &spellDeconstructor;
					channeled_spell->channel_duration = duration; //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
				}
				else
				{
					duration = element->duration;
				}
				if ( caster->setEffect(EFF_LIGHTEN_LOAD, (Uint8)50, duration, false) )
				{
					messagePlayerColor(caster->isEntityPlayer(), MESSAGE_STATUS, uint32ColorGreen, Language::get(6681));
					playSoundEntity(caster, 178, 128);
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 170);
				}
			}
		}
		else if ( spell->ID == SPELL_ATTRACT_ITEMS )
		{
			if ( caster )
			{
				int duration = element->duration;
				if ( caster->behavior == &actPlayer )
				{
					node_t* spellnode = list_AddNodeLast(&caster->getStats()->magic_effects);
					spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
					channeled_spell = (spell_t*)(spellnode->element);
					channeled_spell->magic_effects_node = spellnode;
					spellnode->size = sizeof(spell_t);
					((spell_t*)spellnode->element)->caster = caster->getUID();
					spellnode->deconstructor = &spellDeconstructor;
					channeled_spell->channel_duration = duration; //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
				}
				else
				{
					duration = element->duration;
				}
				if ( caster->setEffect(EFF_ATTRACT_ITEMS, true, duration, false) )
				{
					messagePlayerColor(caster->isEntityPlayer(), MESSAGE_STATUS, uint32ColorGreen, Language::get(6683));
					playSoundEntity(caster, 178, 128);
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 170);
				}
			}
		}
		else if ( spell->ID == SPELL_RETURN_ITEMS )
		{
			if ( caster )
			{
				int duration = element->duration;
				if ( caster->behavior == &actPlayer )
				{
					node_t* spellnode = list_AddNodeLast(&caster->getStats()->magic_effects);
					spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
					channeled_spell = (spell_t*)(spellnode->element);
					channeled_spell->magic_effects_node = spellnode;
					spellnode->size = sizeof(spell_t);
					((spell_t*)spellnode->element)->caster = caster->getUID();
					spellnode->deconstructor = &spellDeconstructor;
					channeled_spell->channel_duration = duration; //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
				}
				else
				{
					duration = element->duration;
				}
				if ( caster->setEffect(EFF_RETURN_ITEM, true, duration, false) )
				{
					messagePlayerColor(caster->isEntityPlayer(), MESSAGE_STATUS, uint32ColorGreen, Language::get(6685));
					playSoundEntity(caster, 178, 128);
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 170);
				}
			}
			}
		else if ( spell->ID == SPELL_HOLOGRAM )
		{
			if ( caster )
			{
				if ( castSpellProps )
				{
					if ( Entity* monster = spellEffectHologram(*caster, *element, castSpellProps->target_x, castSpellProps->target_y) )
					{
						messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6667));
						monster->monsterSpecialState = caster->getUID();
						serverUpdateEntitySkill(monster, 33);
						monster->setEffect(EFF_MIST_FORM, true, 10 * TICKS_PER_SECOND, false);
					}
					else
					{
						messagePlayer(caster->isEntityPlayer(), MESSAGE_HINT, Language::get(6578));
					}
				}
			}
		}
		else if ( spell->ID == SPELL_SPORES )
		{
			if ( caster )
			{
				int duration = element->duration;
				if ( caster->behavior == &actPlayer )
				{
					node_t* spellnode = list_AddNodeLast(&caster->getStats()->magic_effects);
					spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
					channeled_spell = (spell_t*)(spellnode->element);
					channeled_spell->magic_effects_node = spellnode;
					spellnode->size = sizeof(spell_t);
					((spell_t*)spellnode->element)->caster = caster->getUID();
					spellnode->deconstructor = &spellDeconstructor;
					channeled_spell->channel_duration = duration; //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
				}
				else
				{
					duration = element->duration;
				}
				if ( caster->setEffect(EFF_SPORES, true, duration, false) )
				{
					messagePlayerColor(caster->isEntityPlayer(), MESSAGE_STATUS, uint32ColorGreen, Language::get(6643));
					playSoundEntity(caster, 178, 128);
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 170);
				}
			}
		}
		else if ( !strcmp(element->element_internal_name, spellElement_dash.element_internal_name) )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i] && caster && (caster == players[i]->entity) )
				{
					playSoundEntity(caster, 180, 128);
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 982);
					caster->setEffect(EFF_DASH, true, 60, true);
					if ( i > 0 && multiplayer == SERVER && !players[i]->isLocalPlayer() )
					{
						strcpy((char*)net_packet->data, "DASH");
						net_packet->address.host = net_clients[i - 1].host;
						net_packet->address.port = net_clients[i - 1].port;
						net_packet->len = 4;
						sendPacketSafe(net_sock, -1, net_packet, i - 1);
					}
					else
					{
						real_t vel = sqrt(pow(caster->vel_y, 2) + pow(caster->vel_x, 2));
						caster->monsterKnockbackVelocity = std::min(2.25, std::max(1.0, vel));
						caster->monsterKnockbackTangentDir = atan2(caster->vel_y, caster->vel_x);
						if ( vel < 0.01 )
						{
							caster->monsterKnockbackTangentDir = caster->yaw + PI;
						}
					}
					break;
				}
			}
			if ( caster->behavior == &actMonster )
			{
				playSoundEntity(caster, 180, 128);
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 982);
				caster->setEffect(EFF_DASH, true, 30, true);
			}
		}
		else if ( !strcmp(element->element_internal_name, spellElement_speed.element_internal_name) )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i] && caster && (caster == players[i]->entity) )
				{
					//Duration for speed.
					int duration = element->duration;
					duration += ((spellBookBonusPercent * 2 / 100.f) * duration); // 100-200%
					
					//duration += (((element->mana + extramagic_to_use) - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->duration;
					//if ( newbie )
					//{
					//	//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
					//	chance = local_rng.rand() % 10;
					//	if ( chance >= spellcasting / 10 )
					//	{
					//		duration -= local_rng.rand() % (1000 / (spellcasting + 1));
					//	}
					//	if ( duration < 100 )
					//	{
					//		duration = 100;    //Range checking.
					//	}
					//}

					if ( stats[i]->getEffectActive(EFF_SLOW) )
					{
						caster->setEffect(EFF_SLOW, false, 0, true);
					}
					caster->setEffect(EFF_FAST, true, duration, true);
					messagePlayerColor(i, MESSAGE_STATUS, uint32ColorGreen, Language::get(768));
					for ( node = map.creatures->first; node; node = node->next )
					{
						Entity* entity = (Entity*)(node->element);
						if ( !entity || entity == caster )
						{
							continue;
						}
						if ( entity->behavior != &actPlayer && entity->behavior != &actMonster )
						{
							continue;
						}

						if ( entityDist(entity, caster) <= HEAL_RADIUS && entity->checkFriend(caster) )
						{
							entity->setEffect(EFF_FAST, true, duration, true);
							playSoundEntity(entity, 178, 128);
							spawnMagicEffectParticles(entity->x, entity->y, entity->z, 174);
							if ( entity->behavior == &actPlayer )
							{
								messagePlayerColor(entity->skill[2], MESSAGE_STATUS, uint32ColorGreen, Language::get(768));
							}
						}
					}
					break;
				}
			}
			if ( caster->behavior == &actMonster )
			{
				if ( caster->getStats()->getEffectActive(EFF_SLOW) )
				{
					caster->setEffect(EFF_SLOW, false, 0, true);
				}
				caster->setEffect(EFF_FAST, true, element->duration, true);
			}

			playSoundEntity(caster, 178, 128);
			spawnMagicEffectParticles(caster->x, caster->y, caster->z, 174);
		}
		else if (!strcmp(element->element_internal_name, spellElement_heal.element_internal_name)
			|| (spell->ID == SPELL_HEAL_OTHER && castSpellProps) )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i] && caster && (caster == players[i]->entity) )
				{
					int amount = element->damage * (((element->mana + extramagic_to_use) / static_cast<double>(element->base_mana)) * element->overload_multiplier); //Amount to heal.
					//if (newbie)
					//{
					//	//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
					//	chance = local_rng.rand() % 10;
					//	if (chance >= spellcasting / 10)
					//	{
					//		amount -= local_rng.rand() % (1000 / (spellcasting + 1));
					//	}
					//	if (amount < 8)
					//	{
					//		amount = 8;    //Range checking.
					//	}
					//}
					// spellbook 100-150%, 50 INT = 200%.
					real_t bonus = ((spellBookBonusPercent * 1 / 100.f) + getBonusFromCasterOfSpellElement(caster, nullptr, element, spell ? spell->ID : SPELL_NONE));
					amount += amount * bonus;
					if ( caster && caster->behavior == &actPlayer )
					{
						Compendium_t::Events_t::eventUpdateCodex(caster->skill[2], Compendium_t::CPDM_CLASS_PWR_MAX_CASTED, "pwr",
							(Sint32)(bonus * 100.0));
					}

					int totalHeal = 0;
					playSoundEntity(caster, 168, 128);
					if ( spell->ID == SPELL_HEAL_OTHER )
					{
						if ( Entity* entity = uidToEntity(castSpellProps->targetUID) )
						{
							int oldHP = entity->getHP();
							spell_changeHealth(entity, amount);
							int heal = std::max(entity->getHP() - oldHP, 0);
							totalHeal += heal;
							if ( heal > 0 )
							{
								spawnDamageGib(entity, -heal, DamageGib::DMG_HEAL, DamageGibDisplayType::DMG_GIB_NUMBER, true);
							}
							playSoundEntity(entity, 168, 128);
							spawnMagicEffectParticles(entity->x, entity->y, entity->z, 169);
						}
					}
					else
					{
						int oldHP = players[i]->entity->getHP();
						if ( overdrewIntoHP )
						{
							amount /= 2;
						}
						if ( oldHP > 0 )
						{
							spell_changeHealth(players[i]->entity, amount, overdrewIntoHP);
						}
						totalHeal += std::max(players[i]->entity->getHP() - oldHP, 0);
						if ( totalHeal > 0 )
						{
							spawnDamageGib(players[i]->entity, -totalHeal, DamageGib::DMG_HEAL, DamageGibDisplayType::DMG_GIB_NUMBER, true);
						}
						for ( node = map.creatures->first; node; node = node->next )
						{
							Entity* entity = (Entity*)(node->element);
							if ( !entity ||  entity == caster )
							{
								continue;
							}
							if ( entity->behavior != &actPlayer && entity->behavior != &actMonster )
							{
								continue;
							}

							if ( entityDist(entity, caster) <= HEAL_RADIUS && entity->checkFriend(caster) )
							{
								oldHP = entity->getHP();
								spell_changeHealth(entity, amount);
								int heal = std::max(entity->getHP() - oldHP, 0);
								totalHeal += heal;
								if ( heal > 0 )
								{
									spawnDamageGib(entity, -heal, DamageGib::DMG_HEAL, DamageGibDisplayType::DMG_GIB_NUMBER, true);
								}
								playSoundEntity(entity, 168, 128);
								spawnMagicEffectParticles(entity->x, entity->y, entity->z, 169);
							}
						}
					}

					if ( totalHeal > 0 )
					{
						serverUpdatePlayerGameplayStats(i, STATISTICS_HEAL_BOT, totalHeal);
						if ( spell && spell->ID > SPELL_NONE )
						{
							if ( !using_magicstaff && !trap )
							{
								if ( usingSpellbook )
								{
									auto find = ItemTooltips.spellItems.find(spell->ID);
									if ( find != ItemTooltips.spellItems.end() )
									{
										if ( find->second.spellbookId >= 0 && find->second.spellbookId < NUMITEMS && items[find->second.spellbookId].category == SPELLBOOK )
										{
											Compendium_t::Events_t::eventUpdate(i, Compendium_t::CPDM_SPELL_HEAL, (ItemType)find->second.spellbookId, totalHeal);
										}
									}
								}
								else
								{
									Compendium_t::Events_t::eventUpdate(i, Compendium_t::CPDM_SPELL_HEAL, SPELL_ITEM, totalHeal, false, spell->ID);
								}
							}
						}
					}
					break;
				}
			}

			if ( caster->behavior == &actMonster )
			{
				spell_changeHealth(caster, element->damage * element->mana);
			}

			playSoundEntity(caster, 168, 128);
			spawnMagicEffectParticles(caster->x, caster->y, caster->z, 169);
		}
		else if ( !strcmp(element->element_internal_name, spellElement_shapeshift.element_internal_name) 
			&& caster && caster->behavior == &actPlayer )
		{
			Monster type = NOTHING;
			switch ( spell->ID )
			{
				case SPELL_RAT_FORM:
					type = RAT;
					break;
				case SPELL_TROLL_FORM:
					type = TROLL;
					break;
				case SPELL_SPIDER_FORM:
					type = SPIDER;
					break;
				case SPELL_IMP_FORM:
					type = CREATURE_IMP;
					break;
				case SPELL_REVERT_FORM:
					break;
				default:
					break;
			}
			int duration = 120 * TICKS_PER_SECOND;
			if ( type != NOTHING && caster->setEffect(EFF_SHAPESHIFT, true, duration, true) )
			{
				spawnExplosion(caster->x, caster->y, caster->z);
				playSoundEntity(caster, 400, 92);
				createParticleDropRising(caster, 593, 1.f);
				serverSpawnMiscParticles(caster, PARTICLE_EFFECT_RISING_DROP, 593);

				caster->effectShapeshift = type;
				serverUpdateEntitySkill(caster, 53);

				for ( node = map.creatures->first; node && stat; node = node->next )
				{
					Entity* entity = (Entity*)(node->element);
					if ( !entity || entity == caster )
					{
						continue;
					}
					if ( entity->behavior != &actMonster )
					{
						continue;
					}
					if ( entity->monsterTarget == caster->getUID() && entity->checkEnemy(caster) )
					{
						Monster oldType = stat->type;
						stat->type = type;
						if ( !entity->checkEnemy(caster) ) // we're now friendly.
						{
							entity->monsterReleaseAttackTarget();
						}
						stat->type = oldType;
					}
				}

				Uint32 color = makeColorRGB(0, 255, 0);
				messagePlayerColor(caster->skill[2], MESSAGE_STATUS, color, Language::get(3419), getMonsterLocalizedName((Monster)caster->effectShapeshift).c_str());
			}
			else
			{
				if ( spell->ID == SPELL_REVERT_FORM )
				{
					if ( stats[caster->skill[2]]->getEffectActive(EFF_SHAPESHIFT) )
					{
						int previousShapeshift = caster->effectShapeshift;
						caster->setEffect(EFF_SHAPESHIFT, false, 0, true);
						caster->effectShapeshift = 0;
						serverUpdateEntitySkill(caster, 53);
						if ( previousShapeshift == CREATURE_IMP && !isLevitating(stats[caster->skill[2]]) )
						{
							stats[caster->skill[2]]->setEffectActive(EFF_LEVITATING, 1);
							stats[caster->skill[2]]->EFFECTS_TIMERS[EFF_LEVITATING] = 5;
						}

						if ( stats[caster->skill[2]]->getEffectActive(EFF_POLYMORPH) )
						{
							messagePlayer(caster->skill[2], MESSAGE_STATUS, Language::get(4302)); // return to your 'abnormal' form
						}
						else
						{
							messagePlayer(caster->skill[2], MESSAGE_STATUS, Language::get(3417));
						}
						playSoundEntity(caster, 400, 92);
						createParticleDropRising(caster, 593, 1.f);
						serverSpawnMiscParticles(caster, PARTICLE_EFFECT_RISING_DROP, 593);
					}
					else if ( stats[caster->skill[2]]->getEffectActive(EFF_POLYMORPH) )
					{
						caster->setEffect(EFF_POLYMORPH, false, 0, true);
						caster->effectPolymorph = 0;
						serverUpdateEntitySkill(caster, 50);

						messagePlayer(player, MESSAGE_STATUS, Language::get(3185));
						playSoundEntity(caster, 400, 92);
						createParticleDropRising(caster, 593, 1.f);
						serverSpawnMiscParticles(caster, PARTICLE_EFFECT_RISING_DROP, 593);
					}
					else
					{
						messagePlayer(caster->skill[2], MESSAGE_HINT, Language::get(3715));
						playSoundEntity(caster, 163, 128);
					}
				}
				else
				{
					messagePlayer(caster->skill[2], MESSAGE_HINT, Language::get(3420));
				}
			}
		}
		else if (!strcmp(element->element_internal_name, spellElement_cure_ailment.element_internal_name))     //TODO: Generalize it for NPCs too?
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i] && caster && (caster == players[i]->entity) )
				{
					int c = 0;
					int numEffectsCured = 0;
					for (c = 0; c < NUMEFFECTS; ++c)   //This does a whole lot more than just cure ailments.
					{
						if ( stats[i] && stats[i]->statusEffectRemovedByCureAilment(c, players[i]->entity) )
						{
							if ( stats[i]->getEffectActive(c) )
							{
								stats[i]->clearEffect(c);
								if ( stats[i]->EFFECTS_TIMERS[c] > 0 )
								{
									stats[i]->EFFECTS_TIMERS[c] = 1;
								}
								++numEffectsCured;
							}
						}
					}
					if ( stats[i]->getEffectActive(EFF_WITHDRAWAL) )
					{
						++numEffectsCured;
						players[i]->entity->setEffect(EFF_WITHDRAWAL, false, EFFECT_WITHDRAWAL_BASE_TIME, true);
						serverUpdatePlayerGameplayStats(i, STATISTICS_FUNCTIONAL, 1);
					}
					if ( players[i]->entity->flags[BURNING] )
					{
						++numEffectsCured;
						players[i]->entity->flags[BURNING] = false;
						serverUpdateEntityFlag(players[i]->entity, BURNING);
					}

					bool regenEffect = spellBookBonusPercent >= 25;
					if ( regenEffect )
					{
						int bonus = 10 * ((spellBookBonusPercent * 4) / 100.f); // 25% = 10 seconds, 50% = 20 seconds.
						caster->setEffect(EFF_HP_REGEN, true, std::max(stats[i]->EFFECTS_TIMERS[EFF_HP_REGEN], bonus * TICKS_PER_SECOND), true);
					}

					if ( numEffectsCured > 0 || regenEffect )
					{
						serverUpdateEffects(player);
					}
					playSoundEntity(players[i]->entity, 168, 128);


					int numAlliesEffectsCured = 0;
					for ( node = map.creatures->first; node; node = node->next )
					{
						Entity* entity = (Entity*)(node->element);
						if ( !entity || entity == caster )
						{
							continue;
						}
						if ( entity->behavior != &actPlayer && entity->behavior != &actMonster )
						{
							continue;
						}
						int entityEffectsCured = 0;
						Stat* target_stat = entity->getStats();
						if ( target_stat )
						{
							if (entityDist(entity, caster) <= HEAL_RADIUS && entity->checkFriend(caster))
							{
								for (c = 0; c < NUMEFFECTS; ++c)   //This does a whole lot more than just cure ailments.
								{
									if ( target_stat->statusEffectRemovedByCureAilment(c, entity) )
									{
										if ( target_stat->getEffectActive(c) )
										{
											target_stat->clearEffect(c);
											if ( target_stat->EFFECTS_TIMERS[c] > 0 )
											{
												target_stat->EFFECTS_TIMERS[c] = 1;
											}
											++numAlliesEffectsCured;
											++entityEffectsCured;
										}
									}
								}
								if ( target_stat->getEffectActive(EFF_WITHDRAWAL) )
								{
									++numAlliesEffectsCured;
									++entityEffectsCured;
									entity->setEffect(EFF_WITHDRAWAL, false, EFFECT_WITHDRAWAL_BASE_TIME, true);
									serverUpdatePlayerGameplayStats(i, STATISTICS_FUNCTIONAL, 1);
								}
								if ( regenEffect )
								{
									int bonus = 10 * ((spellBookBonusPercent * 4) / 100.f); // 25% = 10 seconds, 50% = 20 seconds.
									entity->setEffect(EFF_HP_REGEN, true, std::max(target_stat->EFFECTS_TIMERS[EFF_HP_REGEN], bonus * TICKS_PER_SECOND), true);
								}
								if ( entity->flags[BURNING] )
								{
									++numAlliesEffectsCured;
									++entityEffectsCured;
									entity->flags[BURNING] = false;
									serverUpdateEntityFlag(entity, BURNING);
								}
								if ( entity->behavior == &actPlayer && (entityEffectsCured > 0 || regenEffect) )
								{
									serverUpdateEffects(entity->skill[2]);
									messagePlayerColor(entity->skill[2], MESSAGE_STATUS, makeColorRGB(0, 255, 0), Language::get(411));
								}

								playSoundEntity(entity, 168, 128);
								spawnMagicEffectParticles(entity->x, entity->y, entity->z, 169);
							}
						}
					}

					if ( regenEffect || numEffectsCured > 0 )
					{
						messagePlayerColor(i, MESSAGE_STATUS, makeColorRGB(0, 255, 0), Language::get(411)); // your body feels cleansed
					}
					if ( numAlliesEffectsCured > 0 )
					{
						messagePlayerColor(i, MESSAGE_STATUS, makeColorRGB(0, 255, 0), Language::get(4313)); // your allies feel cleansed
					}
					if ( !regenEffect && numEffectsCured == 0 && numAlliesEffectsCured == 0 )
					{
						messagePlayer(i, MESSAGE_STATUS, Language::get(3715)); // had no effect.
					}
					break;
				}
			}

			playSoundEntity(caster, 168, 128 );
			spawnMagicEffectParticles(caster->x, caster->y, caster->z, 169);
		}
		else if ( !strcmp(element->element_internal_name, spellElement_summon.element_internal_name) )
		{
			playSoundEntity(caster, 251, 128);
			playSoundEntity(caster, 252, 128);
			if ( caster->behavior == &actPlayer && stats[caster->skill[2]] )
			{
				// kill old summons.
				for ( node = stats[caster->skill[2]]->FOLLOWERS.first; node != nullptr; node = node->next )
				{
					Entity* follower = nullptr;
					if ( (Uint32*)(node)->element )
					{
						follower = uidToEntity(*((Uint32*)(node)->element));
					}
					if ( follower && follower->monsterAllySummonRank != 0 )
					{
						Stat* followerStats = follower->getStats();
						if ( followerStats )
						{
							follower->setMP(followerStats->MAXMP * (followerStats->HP / static_cast<float>(followerStats->MAXHP)));
							follower->setHP(0);
						}
					}
				}

				real_t startx = caster->x;
				real_t starty = caster->y;
				real_t startz = -4;
				real_t pitch = caster->pitch;
				if ( pitch < 0 )
				{
					pitch = 0;
				}
				// draw line from the players height and direction until we hit the ground.
				real_t previousx = startx;
				real_t previousy = starty;
				int index = 0;
				for ( ; startz < 0.f; startz += abs(0.05 * tan(pitch)) )
				{
					startx += 0.1 * cos(caster->yaw);
					starty += 0.1 * sin(caster->yaw);
					index = (static_cast<int>(starty + 16 * sin(caster->yaw)) >> 4) * MAPLAYERS 
						+ (static_cast<int>(startx + 16 * cos(caster->yaw)) >> 4) * MAPLAYERS * map.height;
					if ( map.tiles[index] && !map.tiles[OBSTACLELAYER + index] )
					{
						// store the last known good coordinate
						previousx = startx;
						previousy = starty;
					}
					if ( map.tiles[OBSTACLELAYER + index] )
					{
						break;
					}
				}

				Entity* timer = createParticleTimer(caster, 55, 0);
				timer->x = static_cast<int>(previousx / 16) * 16 + 8;
				timer->y = static_cast<int>(previousy / 16) * 16 + 8;
				timer->sizex = 4;
				timer->sizey = 4;
				timer->particleTimerCountdownSprite = 791;
				timer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SPELL_SUMMON;
				timer->particleTimerPreDelay = 40;
				timer->particleTimerEndAction = PARTICLE_EFFECT_SPELL_SUMMON;
				timer->z = 0;
				Entity* sapParticle = createParticleSapCenter(caster, caster, SPELL_SUMMON, 599, 599);
				sapParticle->parent = 0;
				sapParticle->yaw = caster->yaw;
				sapParticle->skill[7] = caster->getUID();
				sapParticle->skill[8] = timer->x;
				sapParticle->skill[9] = timer->y;
				serverSpawnMiscParticlesAtLocation(previousx / 16, previousy / 16, 0, PARTICLE_EFFECT_SPELL_SUMMON, 791);
			}
		}
		else if ( !strcmp(element->element_internal_name, spellElement_reflectMagic.element_internal_name) )
		{
			if ( caster->behavior == &actMonster )
			{
				caster->setEffect(EFF_MAGICREFLECT, true, 600, true);
				playSoundEntity(caster, 166, 128);
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 174);
			}
			else
			{
				//TODO: Refactor into a function that adds magic_effects. Invisibility also makes use of this.
				//Also refactor the duration determining code.
				int duration = element->duration;
				//duration += (((element->mana + extramagic_to_use) - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->duration;
				node_t* spellnode = list_AddNodeLast(&caster->getStats()->magic_effects);
				spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
				channeled_spell = (spell_t*)(spellnode->element);
				channeled_spell->magic_effects_node = spellnode;
				spellnode->size = sizeof(spell_t);
				((spell_t*)spellnode->element)->caster = caster->getUID();
				spellnode->deconstructor = &spellDeconstructor;
				//if ( newbie )
				//{
				//	//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
				//	chance = local_rng.rand() % 10;
				//	if ( chance >= spellcasting / 10 )
				//	{
				//		duration -= local_rng.rand() % (1000 / (spellcasting + 1));
				//	}
				//	if ( duration < 180 )
				//	{
				//		duration = 180;    //Range checking.
				//	}
				//}
				duration /= getCostOfSpell((spell_t*)spellnode->element);
				channeled_spell->channel_duration = duration; //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
				caster->setEffect(EFF_MAGICREFLECT, true, duration, true);
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( players[i] && caster && (caster == players[i]->entity) )
					{
						serverUpdateEffects(i);
					}
				}

				playSoundEntity(caster, 166, 128 );
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 174);
			}
		}
		else if ( !strcmp(element->element_internal_name, spellElement_amplifyMagic.element_internal_name) )
		{
			if ( caster->behavior == &actMonster )
			{
				caster->setEffect(EFF_MAGICAMPLIFY, true, 600, true);
				playSoundEntity(caster, 166, 128);
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 174);
			}
			else
			{
				//TODO: Refactor into a function that adds magic_effects. Invisibility also makes use of this.
				//Also refactor the duration determining code.
				int duration = element->duration;
				//duration += (((element->mana + extramagic_to_use) - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->duration;
				node_t* spellnode = list_AddNodeLast(&caster->getStats()->magic_effects);
				spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
				channeled_spell = (spell_t*)(spellnode->element);
				channeled_spell->magic_effects_node = spellnode;
				spellnode->size = sizeof(spell_t);
				((spell_t*)spellnode->element)->caster = caster->getUID();
				spellnode->deconstructor = &spellDeconstructor;
				//if ( newbie )
				//{
				//	//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
				//	chance = local_rng.rand() % 10;
				//	if ( chance >= spellcasting / 10 )
				//	{
				//		duration -= local_rng.rand() % (1000 / (spellcasting + 1));
				//	}
				//	if ( duration < 180 )
				//	{
				//		duration = 180;    //Range checking.
				//	}
				//}
				duration /= getCostOfSpell((spell_t*)spellnode->element);
				channeled_spell->channel_duration = duration; //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
				caster->setEffect(EFF_MAGICAMPLIFY, true, duration, true);
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( players[i] && caster && (caster == players[i]->entity) )
					{
						serverUpdateEffects(i);
						messagePlayer(i, MESSAGE_PROGRESSION, Language::get(3442));
					}
				}

				playSoundEntity(caster, 166, 128);
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 174);
			}
		}
		else if ( !strcmp(element->element_internal_name, spellElement_vampiricAura.element_internal_name) )
		{
			if ( caster->behavior == &actMonster )
			{
				createParticleDropRising(caster, 600, 0.7);
				serverSpawnMiscParticles(caster, PARTICLE_EFFECT_VAMPIRIC_AURA, 600);
				caster->getStats()->setEffectActive(EFF_VAMPIRICAURA, 1);
				caster->getStats()->EFFECTS_TIMERS[EFF_VAMPIRICAURA] = 600;
			}
			else if ( caster->behavior == &actPlayer )
			{
				channeled_spell = spellEffectVampiricAura(caster, spell, extramagic_to_use);
			}
			//Also refactor the duration determining code.
		}
		else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_FORGE_METAL_SCRAP].element_internal_name)
			|| !strcmp(element->element_internal_name, spellElementMap[SPELL_FORGE_MAGIC_SCRAP].element_internal_name) )
		{
			if ( caster )
			{
				Item* item = nullptr;
				if ( spell->ID == SPELL_FORGE_METAL_SCRAP )
				{
					item = newItem(TOOL_METAL_SCRAP, DECREPIT, 0, 50, 0, true, nullptr);
				}
				else if ( spell->ID == SPELL_FORGE_MAGIC_SCRAP )
				{
					item = newItem(TOOL_MAGIC_SCRAP, DECREPIT, 0, 50, 0, true, nullptr);
				}
				if ( item )
				{
					std::string itemName = item->getName();
					int qty = item->count;
					Entity* dropped = dropItemMonster(item, caster, nullptr, item->count);
					if ( dropped )
					{
						dropped->yaw = caster->yaw;
						dropped->vel_x = (1.5 + .025 * (local_rng.rand() % 11)) * cos(caster->yaw);
						dropped->vel_y = (1.5 + .025 * (local_rng.rand() % 11)) * sin(caster->yaw);
						dropped->flags[USERFLAG1] = false;

						messagePlayerColor(caster->isEntityPlayer(), MESSAGE_INVENTORY, makeColorRGB(0, 255, 0), Language::get(6697), qty, itemName.c_str());
						playSoundEntity(caster, 167, 128);
						spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
					}
					else
					{
						free(item);
					}
				}
			}
		}
		else if ( !strcmp(element->element_internal_name, spellElement_slime_spray.element_internal_name)
			|| !strcmp(element->element_internal_name, spellElementMap[SPELL_ELEMENT_PROPULSION_MAGIC_SPRAY].element_internal_name) )
		{
			int particle = -1;
			switch ( spell->ID )
			{
			case SPELL_SLIME_ACID:
				particle = 180;
				break;
			case SPELL_SLIME_WATER:
				particle = 181;
				break;
			case SPELL_SLIME_FIRE:
				particle = 182;
				break;
			case SPELL_SLIME_TAR:
				particle = 183;
				break;
			case SPELL_SLIME_METAL:
				particle = 184;
				break;
			case SPELL_GREASE_SPRAY:
				particle = 245;
				break;
			default:
				break;
			}
			if ( particle >= 0 )
			{
				Entity* spellTimer = createParticleTimer(caster, 30, -1);
				spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_MAGIC_SPRAY;
				spellTimer->particleTimerCountdownSprite = particle;
				result = spellTimer;
				if ( !(caster && caster->behavior == &actMonster && caster->getStats() && caster->getStats()->type == SLIME) )
				{
					// spawn these if not a slime doing its special attack, client spawns own particles
					serverSpawnMiscParticles(caster, PARTICLE_EFFECT_SLIME_SPRAY, particle);
				}
			}
		}
		else if ( !strcmp(element->element_internal_name, spellElementMap[SPELL_ELEMENT_PROPULSION_FOCI_SPRAY].element_internal_name) )
		{
			static ConsoleVariable<int> cvar_foci_sprite("/foci_sprite", 13);
			int particle = *cvar_foci_sprite;
			if ( particle >= 0 )
			{
				Entity* spellTimer = createParticleTimer(caster, 10, -1);
				spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_FOCI_SPRAY;
				spellTimer->particleTimerCountdownSprite = particle;
				result = spellTimer;
				if ( !(caster && caster->behavior == &actMonster) )
				{
					// spawn these if not a monster doing its special attack, client spawns own particles
					serverSpawnMiscParticles(caster, PARTICLE_EFFECT_FOCI_SPRAY, particle);
				}
			}
		}
		
		// intentional separate from else/if chain.
		// disables propulsion if found a marked target.
		if ( !strcmp(spell->spell_internal_name, spell_telePull.spell_internal_name) )
		{
			if ( caster->creatureShadowTaggedThisUid != 0 )
			{
				Entity* entityToTeleport = uidToEntity(caster->creatureShadowTaggedThisUid);
				if ( entityToTeleport )
				{
					propulsion = 0;
					spellEffectTeleportPull(nullptr, *element, caster, entityToTeleport, 0);
				}
			}
		}

		Entity* missileEntity = nullptr;
		if ( propulsion == PROPULSION_MISSILE )
		{
			missileEntity = newEntity(168, 1, map.entities, nullptr); // red magic ball
			missileEntity->parent = caster->getUID();
			missileEntity->x = caster->x;
			missileEntity->y = caster->y;
			missileEntity->z = -1;
			missileEntity->sizex = 1;
			missileEntity->sizey = 1;
			missileEntity->yaw = caster->yaw;
			missileEntity->flags[UPDATENEEDED] = true;
			missileEntity->flags[PASSABLE] = true;
			missileEntity->behavior = &actMagicMissile;
			double missile_speed = 4 * (std::max(1, element->mana) / static_cast<double>(element->overload_multiplier)); //TODO: Factor in base mana cost?
			missileEntity->vel_x = cos(missileEntity->yaw) * (missile_speed);
			missileEntity->vel_y = sin(missileEntity->yaw) * (missile_speed);

			missileEntity->skill[4] = 0;
			missileEntity->skill[5] = traveltime;
			if ( using_magicstaff )
			{
				missileEntity->actmagicCastByMagicstaff = 1;
			}
			else if ( usingSpellbook )
			{
				missileEntity->actmagicFromSpellbook = 1;
				if ( spellBookBonusPercent > 0 )
				{
					missileEntity->actmagicSpellbookBonus = spellBookBonusPercent;
				}
			}

			if ( spell->ID == SPELL_METEOR && castSpellProps )
			{
				missile_speed = 3.0;
				real_t yaw = missileEntity->yaw;
				int delayMove = TICKS_PER_SECOND;
				if ( innerElement && !strcmp(innerElement->element_internal_name, "spell_element_flames") )
				{
					int spread = 5;
					missile_speed = 3.0 + ((spread - (local_rng.rand() % (spread * 2 + 1))) / 5.0);
					yaw += ((spread - (local_rng.rand() % (spread * 2 + 1))) / 5.0) * PI / 64;
					missileEntity->actmagicNoHitMessage = 1;
					delayMove = std::max(0, 10 * (castSpellProps->elementIndex - 1));
				}
				missileEntity->vel_x = cos(yaw) * (missile_speed);
				missileEntity->vel_y = sin(yaw) * (missile_speed);
				real_t spellDistance = sqrt(pow(castSpellProps->caster_x - castSpellProps->target_x, 2)
					+ pow(castSpellProps->caster_y - castSpellProps->target_y, 2));
				spellDistance += 4.0; // add a little distance
				real_t startZ = -16.0;
				missileEntity->vel_z = -(startZ - 7.5) / (spellDistance / missile_speed);
				missileEntity->actmagicIsVertical = MAGIC_ISVERTICAL_XYZ;
				missileEntity->z = startZ;
				missileEntity->pitch = atan2(missileEntity->vel_z, missile_speed);

				if ( delayMove > 0 )
				{
					missileEntity->flags[INVISIBLE] = true;
					missileEntity->flags[UPDATENEEDED] = false;
					missileEntity->actmagicDelayMove = delayMove;
					missileEntity->actmagicVelXStore = missileEntity->vel_x;
					missileEntity->actmagicVelYStore = missileEntity->vel_y;
					missileEntity->actmagicVelZStore = missileEntity->vel_z;
					missileEntity->vel_x = 0.0;
					missileEntity->vel_y = 0.0;
					missileEntity->vel_z = 0.0;
				}
			}
			else if ( spell->ID == SPELL_METEOR_SHOWER && castSpellProps )
			{
				missile_speed = 3.0;
				real_t yaw = missileEntity->yaw;
				if ( innerElement && !strcmp(innerElement->element_internal_name, "spell_element_flames") )
				{
					int spread = 5;
					missile_speed = 3.0 + ((spread - (local_rng.rand() % (spread * 2 + 1))) / 5.0);
					yaw += ((spread - (local_rng.rand() % (spread * 2 + 1))) / 5.0) * PI / 64;
					missileEntity->actmagicNoHitMessage = 1;
				}

				int delayMove = 0;
				delayMove += (TICKS_PER_SECOND) * (castSpellProps->elementIndex / 2);
				if ( castSpellProps->elementIndex % 2 == 1 )
				{
					delayMove += 30;
				}
				missileEntity->vel_x = cos(yaw) * (missile_speed);
				missileEntity->vel_y = sin(yaw) * (missile_speed);
				real_t spellDistance = sqrt(pow(castSpellProps->caster_x - castSpellProps->target_x, 2)
					+ pow(castSpellProps->caster_y - castSpellProps->target_y, 2));
				spellDistance += 4.0; // add a little distance
				real_t startZ = -16.0;
				missileEntity->vel_z = -(startZ - 7.5) / (spellDistance / missile_speed);
				missileEntity->actmagicIsVertical = MAGIC_ISVERTICAL_XYZ;
				missileEntity->z = startZ;
				missileEntity->pitch = atan2(missileEntity->vel_z, missile_speed);

				if ( delayMove > 0 )
				{
					missileEntity->flags[INVISIBLE] = true;
					missileEntity->flags[UPDATENEEDED] = false;
					missileEntity->actmagicDelayMove = delayMove;
					missileEntity->actmagicVelXStore = missileEntity->vel_x;
					missileEntity->actmagicVelYStore = missileEntity->vel_y;
					missileEntity->actmagicVelZStore = missileEntity->vel_z;
					missileEntity->vel_x = 0.0;
					missileEntity->vel_y = 0.0;
					missileEntity->vel_z = 0.0;
				}
			}

			Stat* casterStats = caster->getStats();
			if ( !trap && !using_magicstaff && casterStats && casterStats->getEffectActive(EFF_MAGICAMPLIFY) )
			{
				if ( spell->ID == SPELL_FIREBALL || spell->ID == SPELL_COLD || spell->ID == SPELL_LIGHTNING || spell->ID == SPELL_MAGICMISSILE )
				{
					missile_speed *= 0.75;
					missileEntity->vel_x = cos(missileEntity->yaw) * (missile_speed);
					missileEntity->vel_y = sin(missileEntity->yaw) * (missile_speed);
					missileEntity->actmagicProjectileArc = 1;
					missileEntity->actmagicIsVertical = MAGIC_ISVERTICAL_XYZ;
					missileEntity->vel_z = -0.3;
					missileEntity->pitch = -PI / 32;
				}
			}

			if ( spell->ID == SPELL_SPORE_BOMB || spell->ID == SPELL_MYCELIUM_BOMB )
			{
				missile_speed *= 0.5;
				missileEntity->vel_x = cos(missileEntity->yaw) * (missile_speed);
				missileEntity->vel_y = sin(missileEntity->yaw) * (missile_speed);
				missileEntity->actmagicProjectileArc = 1;
				missileEntity->actmagicIsVertical = MAGIC_ISVERTICAL_XYZ;
				missileEntity->vel_z = -0.3;
				missileEntity->pitch = -PI / 32;
			}

			node = list_AddNodeFirst(&missileEntity->children);
			node->element = copySpell(spell);
			((spell_t*)node->element)->caster = caster->getUID();
			node->deconstructor = &spellDeconstructor;
			node->size = sizeof(spell_t);

			int volume = 128;

			// IMPORTANT - TRAP IS USED FOR STORM POTIONS AND ORBIT PARTICLES, QUIET SOUND HERE.
			if ( trap && caster && (caster->behavior == &actPlayer || caster->behavior == &actMonster) )
			{
				volume = 8;
			}
			else if ( trap )
			{
				volume = 96;
			}

			if( !strcmp(spell->spell_internal_name, spell_cold.spell_internal_name) )
			{
				if ( volume > 64 )
				{
					volume = 64;
				}
			}
			
			if ( !strcmp(spell->spell_internal_name, spell_acidSpray.spell_internal_name) )
			{
				traveltime = 15;
				missileEntity->skill[5] = traveltime;
			}
			else if ( !strcmp(spell->spell_internal_name, spell_sprayWeb.spell_internal_name) )
			{
				traveltime = 15;
				missileEntity->skill[5] = traveltime;
			}
			else if ( !strcmp(spell->spell_internal_name, spell_ghost_bolt.spell_internal_name) )
			{
				traveltime = 10;
				missileEntity->skill[5] = traveltime;
			}
			if ( caster && caster->getStats() && caster->getStats()->type == FLAME_ELEMENTAL )
			{
				traveltime = 15;
				missileEntity->skill[5] = traveltime;
				if ( node_t* elementNode = ((spell_t*)node->element)->elements.first )
				{
					if ( auto element = (spellElement_t*)elementNode->element )
					{
						if ( elementNode = element->elements.first )
						{
							element = (spellElement_t*)elementNode->element;
							element->damage /= 2;
						}
					}
				}
			}

			int sound = spellGetCastSound(spell);
			if ( volume > 0 && sound > 0 )
			{
				playSoundEntity(missileEntity, sound, volume);
			}
			result = missileEntity;

			if ( trap )
			{
				if ( caster->behavior == &actMagicTrapCeiling )
				{
					node_t* node = caster->children.first;
					Entity* ceilingModel = (Entity*)(node->element);
					missileEntity->z = ceilingModel->z;
				}
			}

			if ( caster->behavior == &actMonster && missileEntity && !trap )
			{
				if ( Stat* casterStats = caster->getStats() )
				{
					int accuracy = casterStats->monsterRangedAccuracy.getAccuracy(caster->monsterTarget);
					if ( accuracy > 0 )
					{
						casterStats->monsterRangedAccuracy.modifyProjectile(*caster, *missileEntity);
					}
					casterStats->monsterRangedAccuracy.incrementAccuracy();
				}
			}
		}
		else if ( propulsion == PROPULSION_MISSILE_TRIO )
		{
			real_t angle = PI / 6;
			real_t baseSpeed = 2;
			real_t baseSideSpeed = 1;
			int sprite = 170;
			if ( !strcmp(spell->spell_internal_name, spell_stoneblood.spell_internal_name) )
			{
				angle = PI / 6;
				baseSpeed = 2;
			}
			else if ( !strcmp(spell->spell_internal_name, spell_acidSpray.spell_internal_name) )
			{
				sprite = 597;
				angle = PI / 16;
				baseSpeed = 3;
				baseSideSpeed = 2;
				traveltime = 20;
			}
			else if ( !strcmp(spell->spell_internal_name, spell_sprayWeb.spell_internal_name) )
			{
				sprite = arachnophobia_filter ? 996 : 861;
				angle = PI / 16;
				baseSpeed = 3;
				baseSideSpeed = 2;
				traveltime = 20;
			}

			missileEntity = newEntity(168, 1, map.entities, nullptr); // red magic ball
			missileEntity->parent = caster->getUID();
			missileEntity->x = caster->x;
			missileEntity->y = caster->y;
			missileEntity->z = -1;
			missileEntity->sizex = 1;
			missileEntity->sizey = 1;
			missileEntity->yaw = caster->yaw;
			missileEntity->flags[UPDATENEEDED] = true;
			missileEntity->flags[PASSABLE] = true;
			missileEntity->behavior = &actMagicMissile;
			missileEntity->sprite = sprite;

			double missile_speed = baseSpeed * (element->mana / static_cast<double>(element->overload_multiplier)); //TODO: Factor in base mana cost?
			missileEntity->vel_x = cos(missileEntity->yaw) * (missile_speed);
			missileEntity->vel_y = sin(missileEntity->yaw) * (missile_speed);

			missileEntity->skill[4] = 0;
			missileEntity->skill[5] = traveltime;
			if ( using_magicstaff )
			{
				missileEntity->actmagicCastByMagicstaff = 1;
			}
			else if ( usingSpellbook )
			{
				missileEntity->actmagicFromSpellbook = 1;
				if ( spellBookBonusPercent > 0 )
				{
					missileEntity->actmagicSpellbookBonus = spellBookBonusPercent;
				}
			}
			node = list_AddNodeFirst(&missileEntity->children);
			node->element = copySpell(spell);
			((spell_t*)node->element)->caster = caster->getUID();
			node->deconstructor = &spellDeconstructor;
			node->size = sizeof(spell_t);

			playSoundEntity(missileEntity, spellGetCastSound(spell), 128);

			result = missileEntity;

			Entity* entity1 = newEntity(168, 1, map.entities, nullptr); // red magic ball
			entity1->parent = caster->getUID();
			entity1->x = caster->x;
			entity1->y = caster->y;
			entity1->z = -1;
			entity1->sizex = 1;
			entity1->sizey = 1;
			entity1->yaw = caster->yaw - angle;
			entity1->flags[UPDATENEEDED] = true;
			entity1->flags[PASSABLE] = true;
			entity1->behavior = &actMagicMissile;
			entity1->sprite = sprite;

			missile_speed = baseSideSpeed * (element->mana / static_cast<double>(element->overload_multiplier)); //TODO: Factor in base mana cost?
			entity1->vel_x = cos(entity1->yaw) * (missile_speed);
			entity1->vel_y = sin(entity1->yaw) * (missile_speed);

			entity1->skill[4] = 0;
			entity1->skill[5] = traveltime;
			if ( using_magicstaff )
			{
				entity1->actmagicCastByMagicstaff = 1;
			}
			else if ( usingSpellbook )
			{
				entity1->actmagicFromSpellbook = 1;
				if ( spellBookBonusPercent > 0 )
				{
					entity1->actmagicSpellbookBonus = spellBookBonusPercent;
				}
			}
			node = list_AddNodeFirst(&entity1->children);
			node->element = copySpell(spell);
			((spell_t*)node->element)->caster = caster->getUID();
			node->deconstructor = &spellDeconstructor;
			node->size = sizeof(spell_t);

			Entity* entity2 = newEntity(168, 1, map.entities, nullptr); // red magic ball
			entity2->parent = caster->getUID();
			entity2->x = caster->x;
			entity2->y = caster->y;
			entity2->z = -1;
			entity2->sizex = 1;
			entity2->sizey = 1;
			entity2->yaw = caster->yaw + angle;
			entity2->flags[UPDATENEEDED] = true;
			entity2->flags[PASSABLE] = true;
			entity2->behavior = &actMagicMissile;
			entity2->sprite = sprite;

			missile_speed = baseSideSpeed * (element->mana / static_cast<double>(element->overload_multiplier)); //TODO: Factor in base mana cost?
			entity2->vel_x = cos(entity2->yaw) * (missile_speed);
			entity2->vel_y = sin(entity2->yaw) * (missile_speed);

			entity2->skill[4] = 0;
			entity2->skill[5] = traveltime;
			if ( using_magicstaff )
			{
				entity2->actmagicCastByMagicstaff = 1;
			}
			else if ( usingSpellbook )
			{
				entity2->actmagicFromSpellbook = 1;
				if ( spellBookBonusPercent > 0 )
				{
					entity2->actmagicSpellbookBonus = spellBookBonusPercent;
				}
			}
			node = list_AddNodeFirst(&entity2->children);
			node->element = copySpell(spell);
			((spell_t*)node->element)->caster = caster->getUID();
			node->deconstructor = &spellDeconstructor;
			node->size = sizeof(spell_t);

			if ( caster->behavior == &actMonster && missileEntity && !trap )
			{
				if ( Stat* casterStats = caster->getStats() )
				{
					int accuracy = casterStats->monsterRangedAccuracy.getAccuracy(caster->monsterTarget);
					if ( accuracy > 0 )
					{
						casterStats->monsterRangedAccuracy.modifyProjectile(*caster, *missileEntity);
					}
					casterStats->monsterRangedAccuracy.incrementAccuracy();
				}
			}
		}

		extramagic_to_use = 0;
		if (extramagic > 0)
		{
			//Extra magic. Pump it in here?
			chance = local_rng.rand() % 5;
			if (chance == 1)
			{
				//Use some of that extra magic in this element.
				int amount = local_rng.rand() % extramagic;
				extramagic -= amount;
				extramagic_to_use += amount; //TODO: Make the elements here use this? Looks like they won't, currently. Oh well.
			}
		}
		//TODO: Add the status/conditional elements/modifiers (probably best as elements) too. Like onCollision or something.
		if ( innerElement )
		{
			if (!strcmp(innerElement->element_internal_name, spellElement_force.element_internal_name))
			{
				//Give the spell force properties.
				if (propulsion == PROPULSION_MISSILE)
				{
					missileEntity->sprite = 173;
				}
			}
			else if (!strcmp(innerElement->element_internal_name, spellElement_fire.element_internal_name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					missileEntity->sprite = 168;
				}
			}
			else if ( !strcmp(innerElement->element_internal_name, spellElement_lightning.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 170;
				}
			}
			else if ( !strcmp(innerElement->element_internal_name, spellElement_stoneblood.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 170;
				}
			}
			else if ( !strcmp(innerElement->element_internal_name, spellElement_ghostBolt.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 1244;
				}
			}
			else if ( !strcmp(innerElement->element_internal_name, spellElement_bleed.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 643;
				}
			}
			else if (!strcmp(innerElement->element_internal_name, spellElement_confuse.element_internal_name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					missileEntity->sprite = 173;
				}
			}
			else if (!strcmp(innerElement->element_internal_name, spellElement_cold.element_internal_name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					missileEntity->sprite = 172;
				}
			}
			else if (!strcmp(innerElement->element_internal_name, spellElement_dig.element_internal_name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					missileEntity->sprite = 171;
				}
			}
			else if (!strcmp(innerElement->element_internal_name, spellElement_locking.element_internal_name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					missileEntity->sprite = 171;
				}
			}
			else if (!strcmp(innerElement->element_internal_name, spellElement_opening.element_internal_name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					missileEntity->sprite = 171;
				}
			}
			else if (!strcmp(innerElement->element_internal_name, spellElement_slow.element_internal_name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					missileEntity->sprite = 171;
				}
			}
			else if ( !strcmp(innerElement->element_internal_name, spellElement_poison.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 597;
				}
			}
			else if (!strcmp(innerElement->element_internal_name, spellElement_sleep.element_internal_name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					missileEntity->sprite = 172;
				}
			}
			else if (!strcmp(innerElement->element_internal_name, spellElement_magicmissile.element_internal_name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					missileEntity->sprite = 173;
				}
			}
			else if ( !strcmp(innerElement->element_internal_name, spellElement_dominate.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 168;
				}
			}
			else if ( !strcmp(innerElement->element_internal_name, spellElement_acidSpray.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 171;
				}
			}
			else if ( !strcmp(innerElement->element_internal_name, spellElement_stealWeapon.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 175;
				}
			}
			else if ( !strcmp(innerElement->element_internal_name, spellElement_drainSoul.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 598;
				}
			}
			else if ( !strcmp(innerElement->element_internal_name, spellElement_charmMonster.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 173;
				}
			}
			else if ( !strcmp(innerElement->element_internal_name, spellElement_telePull.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 175;
				}
			}
			else if ( !strcmp(innerElement->element_internal_name, spellElement_shadowTag.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 175;
				}
			}
			else if ( !strcmp(innerElement->element_internal_name, spellElement_demonIllusion.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 171;
				}
			}
			else if ( !strcmp(innerElement->element_internal_name, spellElementMap[SPELL_MERCURY_BOLT].element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 1799;
				}
			}
			else if ( !strcmp(innerElement->element_internal_name, spellElementMap[SPELL_LEAD_BOLT].element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 1798;
				}
			}
			else if ( !strcmp(innerElement->element_internal_name, spellElementMap[SPELL_SHADE_BOLT].element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 1801;
				}
			}
			else if ( !strcmp(innerElement->element_internal_name, spellElementMap[SPELL_SPHERE_SILENCE].element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 1818;
				}
			}
			else if ( !strcmp(innerElement->element_internal_name, spellElementMap[SPELL_WONDERLIGHT].element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 1802;
				}
			}
			else if ( !strcmp(innerElement->element_internal_name, spellElementMap[SPELL_SPORE_BOMB].element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 1816;
				}
			}
			else if ( !strcmp(innerElement->element_internal_name, spellElementMap[SPELL_MYCELIUM_BOMB].element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 1886;
				}
			}
			else if ( !strcmp(innerElement->element_internal_name, spellElementMap[SPELL_SPIN].element_internal_name) 
				|| !strcmp(innerElement->element_internal_name, spellElementMap[SPELL_DIZZY].element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 1856;
				}
			}
			else if ( !strcmp(innerElement->element_internal_name, "spell_element_flames") )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 13;
					missileEntity->flags[SPRITE] = true;
				}
			}
		}
	}

	//Random chance to level up spellcasting skill.
	if ( !trap )
	{
		if ( player >= 0 )
		{
			if ( !using_magicstaff )
			{
				if ( !usingSpellbook )
				{
					Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_SPELL_CASTS_RUN, "memorized", 1);
					Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_SPELL_CASTS, SPELL_ITEM, 1, false, spell->ID);
				}
				else if ( usingSpellbook )
				{
					if ( items[spellbookType].category == SPELLBOOK )
					{
						Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_CLASS_SPELLBOOK_CASTS_RUN, "spellbook casting", 1);
						Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_SPELLBOOK_CASTS, spellbookType, 1);

						if ( items[spellbookType].hasAttribute("SPELLBOOK_CAST_BONUS") )
						{
							Compendium_t::Events_t::eventUpdateCodex(player, Compendium_t::CPDM_PWR_MAX_SPELLBOOK, "pwr", spellBookBonusPercent);
						}
					}
				}
			}
		}
		if ( using_magicstaff )
		{
			if ( stat )
			{
				// spellcasting increase chances.
				if ( stat->getProficiency(PRO_SPELLCASTING) < 60 )
				{
					if ( local_rng.rand() % 6 == 0 ) //16.67%
					{
						caster->increaseSkill(PRO_SPELLCASTING);
					}
				}
				else if ( stat->getProficiency(PRO_SPELLCASTING) < 80 )
				{
					if ( local_rng.rand() % 9 == 0 ) //11.11%
					{
						caster->increaseSkill(PRO_SPELLCASTING);
					}
				}
				else // greater than 80
				{
					if ( local_rng.rand() % 12 == 0 ) //8.33%
					{
						caster->increaseSkill(PRO_SPELLCASTING);
					}
				}

				// magic increase chances.
				if ( stat->getProficiency(PRO_SPELLCASTING) < 60 )
				{
					if ( local_rng.rand() % 7 == 0 ) //14.2%
					{
						caster->increaseSkill(PRO_MAGIC);
					}
				}
				else if ( stat->getProficiency(PRO_SPELLCASTING) < 80 )
				{
					if ( local_rng.rand() % 10 == 0 ) //10.00%
					{
						caster->increaseSkill(PRO_MAGIC);
					}
				}
				else // greater than 80
				{
					if ( local_rng.rand() % 13 == 0 ) //7.69%
					{
						caster->increaseSkill(PRO_MAGIC);
					}
				}
			}
		}
		else
		{
			if ( stat )
			{
				int spellCastChance = 5; // 20%
				int magicChance = 6; // 16.67%
				int castDifficulty = stat->getProficiency(PRO_SPELLCASTING) / 20 - spell->difficulty / 20;
				if ( castDifficulty <= -1 )
				{
					// spell was harder.
					spellCastChance = 3; // 33%
					magicChance = 3; // 33%
				}
				else if ( castDifficulty == 0 )
				{
					// spell was same level
					spellCastChance = 3; // 33%
					magicChance = 4; // 25%
				}
				else if ( castDifficulty == 1 )
				{
					// spell was easy.
					spellCastChance = 4; // 25%
					magicChance = 5; // 20%
				}
				else if ( castDifficulty > 1 )
				{
					// piece of cake!
					spellCastChance = 6; // 16.67%
					magicChance = 7; // 14.2%
				}
				if ( usingSpellbook && !playerCastingFromKnownSpellbook )
				{
					spellCastChance *= 2;
					magicChance *= 2;
				}
				//messagePlayer(0, "Difficulty: %d, chance 1 in %d, 1 in %d", castDifficulty, spellCastChance, magicChance);
				if ( (!strcmp(element->element_internal_name, spellElement_light.element_internal_name) || spell->ID == SPELL_REVERT_FORM) )
				{
					if ( stat->getProficiency(PRO_SPELLCASTING) >= SKILL_LEVEL_SKILLED )
					{
						spellCastChance = 0;
					}
					if ( stat->getProficiency(PRO_MAGIC) >= SKILL_LEVEL_SKILLED )
					{
						magicChance = 0;
					}

					// light provides no levelling past 40 in both spellcasting and magic.
					if ( (magicChance == 0 && spellCastChance == 0) && local_rng.rand() % 20 == 0 )
					{
						for ( int i = 0; i < MAXPLAYERS; ++i )
						{
							if ( players[i] && caster && (caster == players[i]->entity) )
							{
								messagePlayer(i, MESSAGE_HINT, Language::get(2591));
							}
						}
					}
				}
				else if ( spell->ID == SPELL_FOCI_FIRE )
				{
					spellCastChance = 0;
					magicChance = 0;
				}

				bool sustainedChance = players[caster->skill[2]]->mechanics.sustainedSpellLevelChance();
				if ( spellCastChance > 0 && (local_rng.rand() % spellCastChance == 0) )
				{
					if ( sustainedSpell && caster->behavior == &actPlayer )
					{
						if ( sustainedChance )
						{
							players[caster->skill[2]]->mechanics.sustainedSpellMPUsed = 0;

							caster->increaseSkill(PRO_SPELLCASTING);
						}
					}
					else
					{
						caster->increaseSkill(PRO_SPELLCASTING);
					}
				}

				bool magicIncreased = false;
				if ( magicChance > 0 && (local_rng.rand() % magicChance == 0) )
				{
					if ( sustainedSpell && caster->behavior == &actPlayer )
					{
						if ( sustainedChance )
						{
							players[caster->skill[2]]->mechanics.sustainedSpellMPUsed = 0;

							caster->increaseSkill(PRO_MAGIC); // otherwise you will basically never be able to learn all the spells in the game...
							magicIncreased = true;
						}
					}
					else
					{
						caster->increaseSkill(PRO_MAGIC); // otherwise you will basically never be able to learn all the spells in the game...
						magicIncreased = true;
					}
				}
				if ( magicIncreased && usingSpellbook && caster->behavior == &actPlayer )
				{
					if ( stats[caster->skill[2]] && stats[caster->skill[2]]->playerRace == RACE_INSECTOID && stats[caster->skill[2]]->stat_appearance == 0 )
					{
						steamStatisticUpdateClient(caster->skill[2], STEAM_STAT_BOOKWORM, STEAM_STAT_INT, 1);
					}
				}
			}
		}
	}

	if ( !trap && usingSpellbook && stat ) // degrade spellbooks on use.
	{
		int chance = 8;
		if ( stat->type == GOBLIN )
		{
			chance = 16;

			if ( caster && caster->behavior == &actPlayer && stat->playerRace == RACE_GOBLIN && stat->stat_appearance == 0 )
			{
				if ( spell->ID >= 30 && spell->ID < 60 )
				{
					serverUpdatePlayerGameplayStats(caster->skill[2], STATISTICS_POP_QUIZ_2, spell->ID);
				}
				else
				{
					serverUpdatePlayerGameplayStats(caster->skill[2], STATISTICS_POP_QUIZ_1, spell->ID);
				}
			}
		}
		if ( stat->shield 
			&& ( (stat->shield->beatitude < 0 && !shouldInvertEquipmentBeatitude(stat))
				|| (stat->shield->beatitude > 0 && shouldInvertEquipmentBeatitude(stat)) ) 
			)
		{
			chance = 1; // cursed books always degrade, or blessed books in succubus/incubus
		}
		else
		{
			if ( caster && caster->behavior == &actPlayer )
			{
				if ( stat->shield && itemCategory(stat->shield) == SPELLBOOK )
				{
					if ( !players[caster->skill[2]]->mechanics.itemDegradeRoll(stat->shield) )
					{
						chance = 0;
					}
				}
			}
		}
		if ( chance > 0 && local_rng.rand() % chance == 0 && stat->shield && itemCategory(stat->shield) == SPELLBOOK )
		{
			Status oldStatus = stat->shield->status;
			if ( caster && caster->behavior == &actPlayer )
			{
				players[caster->skill[2]]->mechanics.onItemDegrade(stat->shield);
			}
			if ( oldStatus == DECREPIT && stat->shield->beatitude > 0 && caster && caster->behavior == &actPlayer )
			{
				--stat->shield->beatitude;
				if ( caster->skill[2] >= 0 )
				{
					messagePlayer(caster->skill[2], MESSAGE_EQUIPMENT, Language::get(6308), stat->shield->getName());
				}

				if ( multiplayer == SERVER && caster->skill[2] > 0 )
				{
					strcpy((char*)net_packet->data, "BEAT");
					net_packet->data[4] = caster->skill[2];
					net_packet->data[5] = 5; // shield index
					net_packet->data[6] = stat->shield->beatitude + 100;
					SDLNet_Write16((Sint16)stat->shield->type, &net_packet->data[7]);
					net_packet->address.host = net_clients[caster->skill[2] - 1].host;
					net_packet->address.port = net_clients[caster->skill[2] - 1].port;
					net_packet->len = 9;
					sendPacketSafe(net_sock, -1, net_packet, caster->skill[2] - 1);
				}
			}
			else
			{
				caster->degradeArmor(*stat, *(stat->shield), 4);
				if ( stat->shield->status < oldStatus )
				{
					if ( player >= 0 )
					{
						Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_SPELLBOOK_CAST_DEGRADES, stat->shield->type, 1);
					}
				}

				if ( stat->shield->status == BROKEN && player >= 0 )
				{
					if ( caster && caster->behavior == &actPlayer && stat->playerRace == RACE_GOBLIN && stat->stat_appearance == 0 )
					{
						steamStatisticUpdateClient(player, STEAM_STAT_DYSLEXIA, STEAM_STAT_INT, 1);
					}
					Item* toBreak = stat->shield;
					consumeItem(toBreak, player);
				}
			}
		}
	}

	if (spell_isChanneled(spell) && !using_magicstaff && !trap)   //TODO: What about magic traps and channeled spells?
	{
		if (!channeled_spell)
		{
			printlog( "What. Spell is channeled but no channeled_spell pointer? What sorcery is this?\n");
		}
		else
		{
			int target_client = 0;
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i] && caster && (caster == players[i]->entity) )
				{
					target_client = i;
				}
			}
			//printlog( "Client is: %d\n", target_client);
			if (multiplayer == SERVER && target_client != 0)
			{
				strcpy( (char*)net_packet->data, "CHAN" );
				net_packet->data[4] = clientnum;
				SDLNet_Write32(spell->ID, &net_packet->data[5]);
				net_packet->address.host = net_clients[target_client - 1].host;
				net_packet->address.port = net_clients[target_client - 1].port;
				net_packet->len = 9;
				sendPacketSafe(net_sock, -1, net_packet, target_client - 1);
			}
			//Add this spell to the list of channeled spells.
			node = list_AddNodeLast(&channeledSpells[target_client]);
			node->element = channeled_spell;
			node->size = sizeof(spell_t);
			node->deconstructor = &emptyDeconstructor;
			channeled_spell->sustain_node = node;
		}
	}

	if ( element )
	{
		if ( list_Size(&element->elements) > 1 ) // recursively cast the remaining elements
		{
			int index = 0;
			for ( node_t* node = element->elements.first; node; node = node->next, ++index )
			{
				if ( index == 0 ) { continue; }
				spell_t* subSpell = copySpell(spell, index);
				bool _trap = true;
				if ( castSpellProps )
				{
					castSpellProps->elementIndex = index;
				}
				castSpell(caster_uid, subSpell, using_magicstaff, _trap, usingSpellbook, castSpellProps);
			}
		}
	}
	return result;
}

int spellGetCastSound(spell_t* spell)
{
	if ( !spell )
	{
		return 0;
	}
	if ( !strcmp(spell->spell_internal_name, spell_fireball.spell_internal_name) )
	{
		return 164;
	}
	else if ( !strcmp(spell->spell_internal_name, spell_lightning.spell_internal_name) )
	{
		return 171;
	}
	else if ( !strcmp(spell->spell_internal_name, spell_ghost_bolt.spell_internal_name) )
	{
		return 169;
	}
	else if ( !strcmp(spell->spell_internal_name, spell_cold.spell_internal_name) )
	{
		return 172;
	}
	else if ( !strcmp(spell->spell_internal_name, spell_bleed.spell_internal_name) )
	{
		return 171;
	}
	else if ( !strcmp(spell->spell_internal_name, spell_stoneblood.spell_internal_name) )
	{
		return 171;
	}
	else if ( !strcmp(spell->spell_internal_name, spell_acidSpray.spell_internal_name) )
	{
		return 164;
	}
	else if ( !strcmp(spell->spell_internal_name, spell_sprayWeb.spell_internal_name) )
	{
		return 169;
	}
	else if ( spell->ID >= SPELL_SLIME_ACID && spell->ID <= SPELL_SLIME_METAL )
	{
		return 0;
	}
	else
	{
		return 169;
	}
	return 0;
}

bool spellIsNaturallyLearnedByRaceOrClass(Entity& caster, Stat& stat, int spellID)
{
	if ( caster.behavior != &actPlayer )
	{
		return false;
	}

	// player races:
	if ( stat.playerRace == RACE_INSECTOID && stat.stat_appearance == 0 && (spellID == SPELL_DASH || spellID == SPELL_FLUTTER || spellID == SPELL_ACID_SPRAY) )
	{
		return true;
	}
	else if ( stat.playerRace == RACE_VAMPIRE && stat.stat_appearance == 0 && (spellID == SPELL_LEVITATION || spellID == SPELL_BLEED) )
	{
		return true;
	}
	else if ( stat.playerRace == RACE_SUCCUBUS && stat.stat_appearance == 0 && (spellID == SPELL_TELEPORTATION || spellID == SPELL_SELF_POLYMORPH) )
	{
		return true;
	}
	else if ( stat.playerRace == RACE_INCUBUS && stat.stat_appearance == 0 && (spellID == SPELL_TELEPORTATION || spellID == SPELL_SHADOW_TAG) )
	{
		return true;
	}
	else if ( stat.playerRace == RACE_AUTOMATON && stat.stat_appearance == 0 && (spellID == SPELL_SALVAGE) )
	{
		return true;
	}
	
	// class specific:
	int playernum = caster.skill[2];
	if ( client_classes[playernum] == CLASS_PUNISHER && (spellID == SPELL_TELEPULL || spellID == SPELL_DEMON_ILLUSION) )
	{
		return true;
	}
	else if ( client_classes[playernum] == CLASS_SHAMAN )
	{
		if ( spellID == SPELL_RAT_FORM || spellID == SPELL_SPIDER_FORM || spellID == SPELL_TROLL_FORM
			|| spellID == SPELL_IMP_FORM || spellID == SPELL_REVERT_FORM )
		{
			return true;
		}
	}
	else if ( stat.getEffectActive(EFF_SHAPESHIFT) )
	{
		switch ( spellID )
		{
			case SPELL_SPEED:
			case SPELL_DETECT_FOOD:
				if ( stat.type == RAT )
				{
					return true;
				}
				break;
			case SPELL_POISON:
			case SPELL_SPRAY_WEB:
				if ( stat.type == SPIDER )
				{
					return true;
				}
				break;
			case SPELL_STRIKE:
			case SPELL_FEAR:
			case SPELL_TROLLS_BLOOD:
				if ( stat.type == TROLL )
				{
					return true;
				}
				break;
			case SPELL_LIGHTNING:
			case SPELL_CONFUSE:
			case SPELL_AMPLIFY_MAGIC:
				if ( stat.type == CREATURE_IMP )
				{
					return true;
				}
				break;
			default:
				break;
		}
	}

	return false;
}
