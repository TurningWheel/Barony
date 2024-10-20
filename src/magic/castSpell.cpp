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
				stats[player]->EFFECTS[EFF_VAMPIRICAURA] && players[player]->entity->playerVampireCurse == 1 )
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
	if ( stat->EFFECTS[EFF_PARALYZED] || stat->EFFECTS[EFF_ASLEEP] )
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

Entity* castSpell(Uint32 caster_uid, spell_t* spell, bool using_magicstaff, bool trap, bool usingSpellbook)
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
			net_packet->len = 10;
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

	node_t* node2; //For traversing the map looking for...liquids?
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
	spellElement_t* element = (spellElement_t*)node->element;
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
		else if (!strcmp(element->element_internal_name, spellElement_light.element_internal_name))
		{
            if (using_magicstaff) {
                for (auto node = map.entities->first; node != nullptr; node = node->next) {
                    auto entity = (Entity*)node->element;
                    if (entity->behavior == &actMagiclightBall) {
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
			//if (newbie)
			//{
			//	//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
			//	chance = local_rng.rand() % 10;
			//	if (chance >= spellcasting / 10)
			//	{
			//		// lifespan of the lightball
			//		entity->skill[12] -= local_rng.rand() % (2000 / (spellcasting + 1));
			//		if (entity->skill[12] < 180)
			//		{
			//			entity->skill[12] = 180;    //Range checking.
			//		}
			//	}
			//}
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

					if ( caster->getStats() && !caster->getStats()->EFFECTS[EFF_FLUTTER] )
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

					if ( stats[i]->EFFECTS[EFF_SLOW] )
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
				if ( caster->getStats()->EFFECTS[EFF_SLOW] )
				{
					caster->setEffect(EFF_SLOW, false, 0, true);
				}
				caster->setEffect(EFF_FAST, true, element->duration, true);
			}

			playSoundEntity(caster, 178, 128);
			spawnMagicEffectParticles(caster->x, caster->y, caster->z, 174);
		}
		else if (!strcmp(element->element_internal_name, spellElement_heal.element_internal_name))     //TODO: Make it work for NPCs.
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i] && caster && (caster == players[i]->entity) )
				{
					int amount = element->damage * (((element->mana + extramagic_to_use) / static_cast<double>(element->base_mana)) * element->overload_multiplier); //Amount to heal.
					if (newbie)
					{
						//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
						chance = local_rng.rand() % 10;
						if (chance >= spellcasting / 10)
						{
							amount -= local_rng.rand() % (1000 / (spellcasting + 1));
						}
						if (amount < 8)
						{
							amount = 8;    //Range checking.
						}
					}
					// spellbook 100-150%, 50 INT = 200%.
					real_t bonus = ((spellBookBonusPercent * 1 / 100.f) + getBonusFromCasterOfSpellElement(caster, nullptr, element, spell ? spell->ID : SPELL_NONE));
					amount += amount * bonus;
					if ( caster && caster->behavior == &actPlayer )
					{
						Compendium_t::Events_t::eventUpdateCodex(caster->skill[2], Compendium_t::CPDM_CLASS_PWR_MAX_CASTED, "pwr",
							(Sint32)(bonus * 100.0));
					}

					int totalHeal = 0;
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
					playSoundEntity(caster, 168, 128);

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
					if ( stats[caster->skill[2]]->EFFECTS[EFF_SHAPESHIFT] )
					{
						int previousShapeshift = caster->effectShapeshift;
						caster->setEffect(EFF_SHAPESHIFT, false, 0, true);
						caster->effectShapeshift = 0;
						serverUpdateEntitySkill(caster, 53);
						if ( previousShapeshift == CREATURE_IMP && !isLevitating(stats[caster->skill[2]]) )
						{
							stats[caster->skill[2]]->EFFECTS[EFF_LEVITATING] = true;
							stats[caster->skill[2]]->EFFECTS_TIMERS[EFF_LEVITATING] = 5;
						}

						if ( stats[caster->skill[2]]->EFFECTS[EFF_POLYMORPH] )
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
					else if ( stats[caster->skill[2]]->EFFECTS[EFF_POLYMORPH] )
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
							if ( stats[i]->EFFECTS[c] )
							{
								stats[i]->EFFECTS[c] = false;
								if ( stats[i]->EFFECTS_TIMERS[c] > 0 )
								{
									stats[i]->EFFECTS_TIMERS[c] = 1;
								}
								++numEffectsCured;
							}
						}
					}
					if ( stats[i]->EFFECTS[EFF_WITHDRAWAL] )
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
										if ( target_stat->EFFECTS[c] )
										{
											target_stat->EFFECTS[c] = false;
											if ( target_stat->EFFECTS_TIMERS[c] > 0 )
											{
												target_stat->EFFECTS_TIMERS[c] = 1;
											}
											++numAlliesEffectsCured;
											++entityEffectsCured;
										}
									}
								}
								if ( target_stat->EFFECTS[EFF_WITHDRAWAL] )
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
				caster->getStats()->EFFECTS[EFF_VAMPIRICAURA] = true;
				caster->getStats()->EFFECTS_TIMERS[EFF_VAMPIRICAURA] = 600;
			}
			else if ( caster->behavior == &actPlayer )
			{
				channeled_spell = spellEffectVampiricAura(caster, spell, extramagic_to_use);
			}
			//Also refactor the duration determining code.
		}
		else if ( !strcmp(element->element_internal_name, spellElement_slime_spray.element_internal_name) )
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
			double missile_speed = 4 * (element->mana / static_cast<double>(element->overload_multiplier)); //TODO: Factor in base mana cost?
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
			Stat* casterStats = caster->getStats();
			if ( !trap && !using_magicstaff && casterStats && casterStats->EFFECTS[EFF_MAGICAMPLIFY] )
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
		//element = (spellElement_t *)element->elements->first->element;
		node = element->elements.first;
		if ( node )
		{
			element = (spellElement_t*)node->element;
			if (!strcmp(element->element_internal_name, spellElement_force.element_internal_name))
			{
				//Give the spell force properties.
				if (propulsion == PROPULSION_MISSILE)
				{
					missileEntity->sprite = 173;
				}

				// !-- DONT MODIFY element->damage since this affects every subsequent spellcast, removing this aspect as unnecessary 05/04/22
				//if (newbie)
				//{
				//	//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
				//	chance = local_rng.rand() % 10;
				//	if (chance >= spellcasting / 10)
				//	{
				//		element->damage -= local_rng.rand() % (100 / (spellcasting + 1));
				//	}
				//	if (element->damage < 10)
				//	{
				//		element->damage = 10;    //Range checking.
				//	}
				//}
			}
			else if (!strcmp(element->element_internal_name, spellElement_fire.element_internal_name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					missileEntity->sprite = 168;
					//missileEntity->skill[4] = missileEntity->x; //Store what x it started shooting out from the player at.
					//missileEntity->skill[5] = missileEntity->y; //Store what y it started shooting out from the player at.
					//missileEntity->skill[12] = (100 * stat->PROFICIENCIES[PRO_SPELLCASTING]) + (100 * stat->PROFICIENCIES[PRO_MAGIC]) + (100 * (local_rng.rand()%10)) + (10 * (local_rng.rand()%10)) + (local_rng.rand()%10); //How long this thing lives.

					//playSoundEntity( entity, 59, 128 );
				}
				// !-- DONT MODIFY element->damage since this affects every subsequent spellcast, removing this aspect as unnecessary 05/04/22
				//if (newbie)
				//{
				//	//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
				//	chance = local_rng.rand() % 10;
				//	if (chance >= spellcasting / 10)
				//	{
				//		element->damage -= local_rng.rand() % (100 / (spellcasting + 1));
				//	}
				//	if (element->damage < 10)
				//	{
				//		element->damage = 10;    //Range checking.
				//	}
				//}
			}
			else if ( !strcmp(element->element_internal_name, spellElement_lightning.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 170;
				}
				// !-- DONT MODIFY element->damage since this affects every subsequent spellcast, removing this aspect as unnecessary 05/04/22
				//if ( newbie )
				//{
				//	//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
				//	chance = local_rng.rand() % 10;
				//	if ( chance >= spellcasting / 10 )
				//	{
				//		element->damage -= local_rng.rand() % (100 / (spellcasting + 1));
				//	}
				//	if ( element->damage < 10 )
				//	{
				//		element->damage = 10;    //Range checking.
				//	}
				//}
			}
			else if ( !strcmp(element->element_internal_name, spellElement_stoneblood.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 170;
				}
			}
			else if ( !strcmp(element->element_internal_name, spellElement_ghostBolt.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 1244;
				}
			}
			else if ( !strcmp(element->element_internal_name, spellElement_bleed.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 643;
				}
			}
			else if (!strcmp(element->element_internal_name, spellElement_confuse.element_internal_name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					missileEntity->sprite = 173;
				}
			}
			else if (!strcmp(element->element_internal_name, spellElement_cold.element_internal_name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					missileEntity->sprite = 172;
				}
			}
			else if (!strcmp(element->element_internal_name, spellElement_dig.element_internal_name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					missileEntity->sprite = 171;
				}
			}
			else if (!strcmp(element->element_internal_name, spellElement_locking.element_internal_name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					missileEntity->sprite = 171;
				}
			}
			else if (!strcmp(element->element_internal_name, spellElement_opening.element_internal_name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					missileEntity->sprite = 171;
				}
			}
			else if (!strcmp(element->element_internal_name, spellElement_slow.element_internal_name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					missileEntity->sprite = 171;
				}
			}
			else if ( !strcmp(element->element_internal_name, spellElement_poison.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 597;
				}
			}
			else if (!strcmp(element->element_internal_name, spellElement_sleep.element_internal_name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					missileEntity->sprite = 172;
				}
			}
			else if (!strcmp(element->element_internal_name, spellElement_magicmissile.element_internal_name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					missileEntity->sprite = 173;
				}
			}
			else if ( !strcmp(element->element_internal_name, spellElement_dominate.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 168;
				}
			}
			else if ( !strcmp(element->element_internal_name, spellElement_acidSpray.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 171;
				}
			}
			else if ( !strcmp(element->element_internal_name, spellElement_stealWeapon.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 175;
				}
			}
			else if ( !strcmp(element->element_internal_name, spellElement_drainSoul.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 598;
				}
			}
			else if ( !strcmp(element->element_internal_name, spellElement_charmMonster.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 173;
				}
			}
			else if ( !strcmp(element->element_internal_name, spellElement_telePull.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 175;
				}
			}
			else if ( !strcmp(element->element_internal_name, spellElement_shadowTag.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 175;
				}
			}
			else if ( !strcmp(element->element_internal_name, spellElement_demonIllusion.element_internal_name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					missileEntity->sprite = 171;
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
	else if ( stat.EFFECTS[EFF_SHAPESHIFT] )
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
