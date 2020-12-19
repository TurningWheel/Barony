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
#include "../sound.hpp"
#include "../items.hpp"
#include "../net.hpp"
#include "../collision.hpp"
#include "../player.hpp"
#include "magic.hpp"
#include "../scores.hpp"
#include "../colors.hpp"

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
			messagePlayer(player, language[407]);
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
					messagePlayer(player, language[408], spell->name);
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
					//messagePlayer(player, language[408], spell->name);
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
					messagePlayerColor(player, uint32ColorGreen(*mainsurface), language[3241]);
					messagePlayerColor(player, uint32ColorGreen(*mainsurface), language[3242]);
					//messagePlayer(player, language[408], spell->name);
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
			messagePlayer(player, language[375]);    //TODO: Allow overexpending at the cost of extreme danger? (maybe an immensely powerful tree of magic actually likes this -- using your life-force to power spells instead of mana)
		}
		return;
	}
	if (magiccost < 0)
	{
		if (player >= 0)
		{
			messagePlayer(player, "Error: Invalid spell. Mana cost is negative?");
		}
		return;
	}

	//Hand the torch off to the spell animator. And stuff. Stuff. I mean spell animation handler thingymabobber.
	fireOffSpellAnimation(&cast_animation[player], caster->getUID(), spell, usingSpellbook);

	//castSpell(caster, spell); //For now, do this while the spell animations are worked on.
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
#define spellcasting std::min(std::max(0,stat->PROFICIENCIES[PRO_SPELLCASTING]+statGetINT(stat, caster)),100) //Shortcut!

	if (clientnum != 0 && multiplayer == CLIENT)
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
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 10;
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
	Entity* entity = NULL;
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
	if ( !using_magicstaff && !trap)
	{
		newbie = caster->isSpellcasterBeginner();

		if ( usingSpellbook && stat->shield && itemCategory(stat->shield) == SPELLBOOK )
		{
			spellBookBonusPercent += (caster->getINT() * 0.5);
			if ( stat->shield->beatitude > 0 
				|| (shouldInvertEquipmentBeatitude(stat) && stat->shield->beatitude < 0) )
			{
				spellBookBonusPercent += abs(stat->shield->beatitude) * 25;
			}
			if ( spellcasting >= spell->difficulty || playerLearnedSpellbook(player, stat->shield) )
			{
				// bypass newbie penalty since we're good enough to cast the spell.
				newbie = false; 
				playerCastingFromKnownSpellbook = true;
			}
			else
			{
				newbie = true;
			}
			if ( stat->shield->beatitude < 0 && !shouldInvertEquipmentBeatitude(stat) )
			{
				newbie = true;
				playerCastingFromKnownSpellbook = false;
			}
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
					if ( player > 0 && multiplayer == SERVER )
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
					else if ( player == 0 || (splitscreen && player > 0) )
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
			if ( caster->behavior == &actPlayer && stat->playerRace == RACE_INSECTOID && stat->appearance == 0 )
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

	if ( newbie )
	{
		//So This wizard is a newbie.

		//First, drain some extra mana maybe.
		int chance = rand() % 10;
		int spellcastingAbility = spellcasting;
		if ( usingSpellbook )
		{
			// penalty for not knowing spellbook. e.g 40 spellcasting, 80 difficulty = 40% more chance to fumble/use mana.
			if ( spellcastingAbility >= 20 )
			{
				spellcastingAbility = std::max(10, spellcastingAbility - spell->difficulty);
			}
			else
			{
				spellcastingAbility = std::max(0, spellcastingAbility - spell->difficulty);
			}
			if ( stat->shield && (stat->shield->beatitude < 0 && !shouldInvertEquipmentBeatitude(stat)) )
			{
				if ( stat->shield->beatitude == -1 )
				{
					spellcastingAbility = std::min(30, spellcastingAbility); // 70% chance to use more mana at least
				}
				else
				{
					spellcastingAbility = std::min(10, spellcastingAbility); // 90% chance to use more mana at least
				}
			}
		}
		if (chance >= spellcastingAbility / 10)   //At skill 20, there's an 80% chance you'll use extra mana. At 70, there's a 30% chance.
		{
			extramagic = rand() % (300 / (spellcastingAbility + 1)); //Use up extra mana. More mana used the lower your spellcasting skill.
			extramagic = std::min<real_t>(extramagic, stat->MP / 10); //To make sure it doesn't draw, say, 5000 mana. Cause dammit, if you roll a 1 here...you're doomed.
			caster->drainMP(extramagic);
		}

		bool fizzleSpell = false;
		chance = rand() % 10;
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
			if ( rand() % 3 == 1 )
			{
				//Fizzle the spell.
				//TODO: Cool effects.
				playSoundEntity(caster, 163, 128);
				if ( player >= 0 )
				{
					messagePlayer(player, language[409]);
				}
				if ( usingSpellbook && stat->shield && itemCategory(stat->shield) == SPELLBOOK 
					&& (stat->shield->beatitude < 0 && !shouldInvertEquipmentBeatitude(stat)) )
				{
					caster->degradeArmor(*stat, *(stat->shield), 4);
					if ( stat->shield->status == BROKEN )
					{
						Item* toBreak = stat->shield;
						consumeItem(toBreak, player);
					}
				}
				return NULL;
			}
		}
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
		if ( stat->shoes != NULL )
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
				messagePlayer(player, language[410]);
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
			chance = rand() % 5;
			if (chance == 1)
			{
				//Use some of that extra magic in this element.
				int amount = rand() % extramagic;
				extramagic -= amount;
				extramagic_to_use += amount;
			}
		}

		if (!strcmp(element->name, spellElement_missile.name))
		{
			//Set the propulsion to missile.
			propulsion = PROPULSION_MISSILE;
			traveltime = element->duration;
			if (newbie)
			{
				//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
				chance = rand() % 10;
				if (chance >= spellcasting / 10)
				{
					traveltime -= rand() % (1000 / (spellcasting + 1));
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
		else if ( !strcmp(element->name, spellElement_missile_trio.name) )
		{
			//Set the propulsion to missile.
			propulsion = PROPULSION_MISSILE_TRIO;
			traveltime = element->duration;
			if ( newbie )
			{
				//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
				chance = rand() % 10;
				if ( chance >= spellcasting / 10 )
				{
					traveltime -= rand() % (1000 / (spellcasting + 1));
				}
				if ( traveltime < 30 )
				{
					traveltime = 30;    //Range checking.
				}
			}
			traveltime += (((element->mana + extramagic_to_use) - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->duration;
		}
		else if (!strcmp(element->name, spellElement_light.name))
		{
			entity = newEntity(175, 1, map.entities, nullptr); // black magic ball
			entity->parent = caster->getUID();
			entity->x = caster->x;
			entity->y = caster->y;
			entity->z = -5.5 + ((-6.5f + -4.5f) / 2) * sin(0);
			entity->skill[7] = -5.5; //Base z.
			entity->sizex = 1;
			entity->sizey = 1;
			entity->yaw = caster->yaw;
			entity->flags[UPDATENEEDED] = true;
			entity->flags[PASSABLE] = true;
			entity->flags[BRIGHT] = true;
			entity->behavior = &actMagiclightBall;
			entity->skill[4] = entity->x; //Store what x it started shooting out from the player at.
			entity->skill[5] = entity->y; //Store what y it started shooting out from the player at.
			entity->skill[12] = (element->duration * (((element->mana + extramagic_to_use) / static_cast<double>(element->base_mana)) * element->overload_multiplier)); //How long this thing lives.
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
			if (newbie)
			{
				//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
				chance = rand() % 10;
				if (chance >= spellcasting / 10)
				{
					// lifespan of the lightball
					entity->skill[12] -= rand() % (2000 / (spellcasting + 1));
					if (entity->skill[12] < 180)
					{
						entity->skill[12] = 180;    //Range checking.
					}
				}
			}
			if (using_magicstaff || trap)
			{
				entity->skill[12] = MAGICSTAFF_LIGHT_DURATION; //TODO: Grab the duration from the magicstaff or trap?
				((spell_t*)spellnode->element)->sustain = false;
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
		else if (!strcmp(element->name, spellElement_invisible.name))
		{
			int duration = element->duration;
			duration += (((element->mana + extramagic_to_use) - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->duration;
			node_t* spellnode = list_AddNodeLast(&caster->getStats()->magic_effects);
			spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
			channeled_spell = (spell_t*)(spellnode->element);
			channeled_spell->magic_effects_node = spellnode;
			spellnode->size = sizeof(spell_t);
			((spell_t*)spellnode->element)->caster = caster->getUID();
			spellnode->deconstructor = &spellDeconstructor;
			if (newbie)
			{
				//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
				chance = rand() % 10;
				if (chance >= spellcasting / 10)
				{
					duration -= rand() % (1000 / (spellcasting + 1));
				}
				if (duration < 180)
				{
					duration = 180;    //Range checking.
				}
			}
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
		else if (!strcmp(element->name, spellElement_levitation.name))
		{
			int duration = element->duration;
			duration += (((element->mana + extramagic_to_use) - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->duration;
			node_t* spellnode = list_AddNodeLast(&caster->getStats()->magic_effects);
			spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
			channeled_spell = (spell_t*)(spellnode->element);
			channeled_spell->magic_effects_node = spellnode;
			spellnode->size = sizeof(spell_t);
			((spell_t*)spellnode->element)->caster = caster->getUID();
			spellnode->deconstructor = &spellDeconstructor;
			if (newbie)
			{
				//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
				chance = rand() % 10;
				if (chance >= spellcasting / 10)
				{
					duration -= rand() % (1000 / (spellcasting + 1));
				}
				if (duration < 180)
				{
					duration = 180;    //Range checking.
				}
			}
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
		else if (!strcmp(element->name, spellElement_teleportation.name))
		{
			if ( caster->creatureShadowTaggedThisUid != 0 )
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
		else if ( !strcmp(element->name, spellElement_selfPolymorph.name) )
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
		else if ( !strcmp(element->name, spellElement_strike.name) )
		{
			caster->attack(MONSTER_POSE_SPECIAL_WINDUP1, MAXCHARGE, nullptr); // this is server only, tells client to attack.
		}
		else if ( !strcmp(element->name, spellElement_fear.name) )
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
					messagePlayer(caster->skill[2], language[3437]);
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
						messagePlayer(caster->skill[2], language[3438]);
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
						messagePlayer(caster->skill[2], language[3438]);
					}
				}
			}
		}
		else if (!strcmp(element->name, spellElement_identify.name))
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
						net_packet->address.host = net_clients[i - 1].host;
						net_packet->address.port = net_clients[i - 1].port;
						net_packet->len = 4;
						sendPacketSafe(net_sock, -1, net_packet, i - 1);
					}
					else
					{
						//Identify an item.
						GenericGUI[i].openGUI(GUI_TYPE_IDENTIFY, nullptr);
					}
				}
			}

			playSoundEntity(caster, 167, 128 );
		}
		else if (!strcmp(element->name, spellElement_removecurse.name))
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i] && (caster == players[i]->entity) )
				{
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 169);
					if ( i != 0 && !players[i]->isLocalPlayer() )
					{
						//Tell the client to uncurse an item.
						strcpy((char*)net_packet->data, "CRCU");
						net_packet->address.host = net_clients[i - 1].host;
						net_packet->address.port = net_clients[i - 1].port;
						net_packet->len = 4;
						sendPacketSafe(net_sock, -1, net_packet, i - 1);
					}
					else
					{
						//Uncurse an item
						GenericGUI[i].openGUI(GUI_TYPE_REMOVECURSE, nullptr);
					}
				}
			}

			playSoundEntity(caster, 167, 128 );
		}
		else if (!strcmp(element->name, spellElement_magicmapping.name))
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
		else if ( !strcmp(element->name, spellElement_detectFood.name) )
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
		else if ( !strcmp(element->name, spellElement_salvageItem.name) )
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
							if ( itemEntity && itemEntity->behavior == &actItem && entityDist(itemEntity, caster) < TOUCHRANGE )
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
						messagePlayer(i, language[3713]);
						playSoundEntity(caster, 163, 128);
					}
					else
					{
						messagePlayerColor(i, SDL_MapRGB(mainsurface->format, 0, 255, 0), language[3712], numItems);
						playSoundEntity(caster, 167, 128);
					}

					if ( totalMetal > 0 )
					{
						Item* crafted = newItem(TOOL_METAL_SCRAP, DECREPIT, 0, totalMetal, 0, true, nullptr);
						if ( crafted )
						{
							Item* pickedUp = itemPickup(player, crafted);
							messagePlayer(player, language[3665], totalMetal, items[pickedUp->type].name_identified);
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
					if ( totalMagic > 0 )
					{
						Item* crafted = newItem(TOOL_MAGIC_SCRAP, DECREPIT, 0, totalMagic, 0, true, nullptr);
						if ( crafted )
						{
							Item* pickedUp = itemPickup(player, crafted);
							messagePlayer(player, language[3665], totalMagic, items[pickedUp->type].name_identified);
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
					if ( !effectCoordinates.empty() )
					{
						for ( auto it = effectCoordinates.begin(); it != effectCoordinates.end(); ++it )
						{
							std::pair<int, int> coords = *it;
							spawnMagicEffectParticles(coords.first * 16 + 8, coords.second * 16 + 8, 7.5, 171);
						}
					}
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
				}
			}
		}
		else if ( !strcmp(element->name, spellElement_trollsBlood.name) )
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
						messagePlayerColor(player, SDL_MapRGB(mainsurface->format, 255, 255, 255), language[3400]);
					}

					caster->setEffect(EFF_TROLLS_BLOOD, true, amount, true);
					Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
					messagePlayerColor(i, color, language[3490]);
					for ( node = map.creatures->first; node; node = node->next )
					{
						entity = (Entity*)(node->element);
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
								messagePlayerColor(entity->skill[2], color, language[3490]);
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
		else if ( !strcmp(element->name, spellElement_flutter.name) )
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
						messagePlayerColor(i, uint32ColorGreen(*mainsurface), language[3767]);
						playSoundEntity(caster, 178, 128);
						spawnMagicEffectParticles(caster->x, caster->y, caster->z, 170);
					}
					break;
				}
			}
		}
		else if ( !strcmp(element->name, spellElement_dash.name) )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i] && caster && (caster == players[i]->entity) )
				{
					playSoundEntity(caster, 180, 128);
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 982);
					caster->setEffect(EFF_DASH, true, 60, true);
					if ( i > 0 && multiplayer == SERVER )
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
		else if ( !strcmp(element->name, spellElement_speed.name) )
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
					//	chance = rand() % 10;
					//	if ( chance >= spellcasting / 10 )
					//	{
					//		duration -= rand() % (1000 / (spellcasting + 1));
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
					messagePlayerColor(i, uint32ColorGreen(*mainsurface), language[768]);
					for ( node = map.creatures->first; node; node = node->next )
					{
						entity = (Entity*)(node->element);
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
								messagePlayerColor(entity->skill[2], uint32ColorGreen(*mainsurface), language[768]);
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
		else if (!strcmp(element->name, spellElement_heal.name))     //TODO: Make it work for NPCs.
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i] && caster && (caster == players[i]->entity) )
				{
					int amount = element->damage * (((element->mana + extramagic_to_use) / static_cast<double>(element->base_mana)) * element->overload_multiplier); //Amount to heal.
					if (newbie)
					{
						//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
						chance = rand() % 10;
						if (chance >= spellcasting / 10)
						{
							amount -= rand() % (1000 / (spellcasting + 1));
						}
						if (amount < 8)
						{
							amount = 8;    //Range checking.
						}
					}
					// spellbook 100-150%, 50 INT = 200%.
					amount += amount * ((spellBookBonusPercent * 1 / 100.f) + getBonusFromCasterOfSpellElement(caster, element));

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

					playSoundEntity(caster, 168, 128);

					for ( node = map.creatures->first; node; node = node->next )
					{
						entity = (Entity*)(node->element);
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
							totalHeal += std::max(entity->getHP() - oldHP, 0);

							playSoundEntity(entity, 168, 128);
							spawnMagicEffectParticles(entity->x, entity->y, entity->z, 169);
						}
					}
					if ( totalHeal > 0 )
					{
						serverUpdatePlayerGameplayStats(i, STATISTICS_HEAL_BOT, totalHeal);
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
		else if ( !strcmp(element->name, spellElement_shapeshift.name) && caster && caster->behavior == &actPlayer )
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
					entity = (Entity*)(node->element);
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

				Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
				if ( caster->effectShapeshift < KOBOLD )
				{
					messagePlayerColor(caster->skill[2], color, language[3419], language[90 + caster->effectShapeshift]);
				}
				else
				{
					messagePlayerColor(caster->skill[2], color, language[3419], language[2000 + caster->effectShapeshift - KOBOLD]);
				}
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

						messagePlayer(caster->skill[2], language[3417]);
						playSoundEntity(caster, 400, 92);
						createParticleDropRising(caster, 593, 1.f);
						serverSpawnMiscParticles(caster, PARTICLE_EFFECT_RISING_DROP, 593);
					}
					else
					{
						messagePlayer(caster->skill[2], language[3715]);
						playSoundEntity(caster, 163, 128);
					}
				}
				else
				{
					messagePlayer(caster->skill[2], language[3420]);
				}
			}
		}
		else if (!strcmp(element->name, spellElement_cure_ailment.name))     //TODO: Generalize it for NPCs too?
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( players[i] && caster && (caster == players[i]->entity) )
				{
					Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
					messagePlayerColor(i, color, language[411]);
					int c = 0;
					for (c = 0; c < NUMEFFECTS; ++c)   //This does a whole lot more than just cure ailments.
					{
						if ( c == EFF_LEVITATING && stats[i]->EFFECTS[EFF_LEVITATING] )
						{
							// don't unlevitate
						}
						else
						{
							if ( !(c == EFF_VAMPIRICAURA && stats[i]->EFFECTS_TIMERS[c] == -2) && c != EFF_WITHDRAWAL
								&& c != EFF_SHAPESHIFT )
							{
								stats[i]->EFFECTS[c] = false;
								stats[i]->EFFECTS_TIMERS[c] = 0;
							}
						}
					}
					if ( stats[i]->EFFECTS[EFF_WITHDRAWAL] )
					{
						players[i]->entity->setEffect(EFF_WITHDRAWAL, false, EFFECT_WITHDRAWAL_BASE_TIME, true);
						serverUpdatePlayerGameplayStats(i, STATISTICS_FUNCTIONAL, 1);
					}
					if ( players[i]->entity->flags[BURNING] )
					{
						players[i]->entity->flags[BURNING] = false;
						serverUpdateEntityFlag(players[i]->entity, BURNING);
					}
					serverUpdateEffects(player);
					playSoundEntity(entity, 168, 128);

					if ( spellBookBonusPercent >= 25 )
					{
						int bonus = 10 * ((spellBookBonusPercent * 4) / 100.f); // 25% = 10 seconds, 50% = 20 seconds.
						caster->setEffect(EFF_HP_REGEN, true, bonus * TICKS_PER_SECOND, true);
					}

					for ( node = map.creatures->first; node->next; node = node->next )
					{
						entity = (Entity*)(node->element);
						if ( !entity || entity == caster )
						{
							continue;
						}
						if ( entity->behavior != &actPlayer && entity->behavior != &actMonster )
						{
							continue;
						}
						Stat* target_stat = entity->getStats();
						if ( target_stat )
						{
							if (entityDist(entity, caster) <= HEAL_RADIUS && entity->checkFriend(caster))
							{
								for (c = 0; c < NUMEFFECTS; ++c)   //This does a whole lot more than just cure ailments.
								{
									if ( !(c == EFF_VAMPIRICAURA && target_stat->EFFECTS_TIMERS[c] == -2) 
										&& c != EFF_WITHDRAWAL && c != EFF_SHAPESHIFT )
									{
										target_stat->EFFECTS[c] = false;
										target_stat->EFFECTS_TIMERS[c] = 0;
									}
								}
								if ( target_stat->EFFECTS[EFF_WITHDRAWAL] )
								{
									entity->setEffect(EFF_WITHDRAWAL, false, EFFECT_WITHDRAWAL_BASE_TIME, true);
									serverUpdatePlayerGameplayStats(i, STATISTICS_FUNCTIONAL, 1);
								}
								if ( spellBookBonusPercent >= 25 )
								{
									int bonus = 10 * ((spellBookBonusPercent * 4) / 100.f); // 25% = 10 seconds, 50% = 20 seconds.
									entity->setEffect(EFF_HP_REGEN, true, bonus * TICKS_PER_SECOND, true);
								}
								if ( entity->behavior == &actPlayer )
								{
									serverUpdateEffects(entity->skill[2]);
								}
								if ( entity->flags[BURNING] )
								{
									entity->flags[BURNING] = false;
									serverUpdateEntityFlag(entity, BURNING);
								}
								playSoundEntity(entity, 168, 128);
								spawnMagicEffectParticles(entity->x, entity->y, entity->z, 169);
							}
						}
					}
					break;
				}
			}

			playSoundEntity(caster, 168, 128 );
			spawnMagicEffectParticles(caster->x, caster->y, caster->z, 169);
		}
		else if ( !strcmp(element->name, spellElement_summon.name) )
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
		else if ( !strcmp(element->name, spellElement_reflectMagic.name) )
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
				duration += (((element->mana + extramagic_to_use) - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->duration;
				node_t* spellnode = list_AddNodeLast(&caster->getStats()->magic_effects);
				spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
				channeled_spell = (spell_t*)(spellnode->element);
				channeled_spell->magic_effects_node = spellnode;
				spellnode->size = sizeof(spell_t);
				((spell_t*)spellnode->element)->caster = caster->getUID();
				spellnode->deconstructor = &spellDeconstructor;
				if ( newbie )
				{
					//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
					chance = rand() % 10;
					if ( chance >= spellcasting / 10 )
					{
						duration -= rand() % (1000 / (spellcasting + 1));
					}
					if ( duration < 180 )
					{
						duration = 180;    //Range checking.
					}
				}
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
		else if ( !strcmp(element->name, spellElement_amplifyMagic.name) )
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
				duration += (((element->mana + extramagic_to_use) - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->duration;
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
				//	chance = rand() % 10;
				//	if ( chance >= spellcasting / 10 )
				//	{
				//		duration -= rand() % (1000 / (spellcasting + 1));
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
						messagePlayer(i, language[3442]);
					}
				}

				playSoundEntity(caster, 166, 128);
				spawnMagicEffectParticles(caster->x, caster->y, caster->z, 174);
			}
		}
		else if ( !strcmp(element->name, spellElement_vampiricAura.name) )
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

		if ( !strcmp(spell->name, spellElement_telePull.name) )
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
		
		if ( propulsion == PROPULSION_MISSILE )
		{
			entity = newEntity(168, 1, map.entities, nullptr); // red magic ball
			entity->parent = caster->getUID();
			entity->x = caster->x;
			entity->y = caster->y;
			entity->z = -1;
			entity->sizex = 1;
			entity->sizey = 1;
			entity->yaw = caster->yaw;
			entity->flags[UPDATENEEDED] = true;
			entity->flags[PASSABLE] = true;
			entity->flags[BRIGHT] = true;
			entity->behavior = &actMagicMissile;
			double missile_speed = 4 * (element->mana / static_cast<double>(element->overload_multiplier)); //TODO: Factor in base mana cost?
			entity->vel_x = cos(entity->yaw) * (missile_speed);
			entity->vel_y = sin(entity->yaw) * (missile_speed);

			entity->skill[4] = 0;
			entity->skill[5] = traveltime;
			if ( using_magicstaff )
			{
				entity->actmagicCastByMagicstaff = 1;
			}
			else if ( usingSpellbook && spellBookBonusPercent > 0 )
			{
				entity->actmagicSpellbookBonus = spellBookBonusPercent;
			}
			Stat* casterStats = caster->getStats();
			if ( !trap && !using_magicstaff && casterStats && casterStats->EFFECTS[EFF_MAGICAMPLIFY] )
			{
				if ( spell->ID == SPELL_FIREBALL || spell->ID == SPELL_COLD || spell->ID == SPELL_LIGHTNING || spell->ID == SPELL_MAGICMISSILE )
				{
					missile_speed *= 0.75;
					entity->vel_x = cos(entity->yaw) * (missile_speed);
					entity->vel_y = sin(entity->yaw) * (missile_speed);
					entity->actmagicProjectileArc = 1;
					entity->actmagicIsVertical = MAGIC_ISVERTICAL_XYZ;
					entity->vel_z = -0.3;
					entity->pitch = -PI / 32;
				}
			}
			node = list_AddNodeFirst(&entity->children);
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

			if( !strcmp(spell->name, spell_cold.name) )
			{
				if ( volume > 64 )
				{
					volume = 64;
				}
			}
			
			if ( !strcmp(spell->name, spell_acidSpray.name) )
			{
				traveltime = 15;
				entity->skill[5] = traveltime;
			}
			else if ( !strcmp(spell->name, spell_sprayWeb.name) )
			{
				traveltime = 15;
				entity->skill[5] = traveltime;
			}

			int sound = spellGetCastSound(spell);
			if ( volume > 0 && sound > 0 )
			{
				playSoundEntity(entity, sound, volume);
			}
			result = entity;

			if ( trap )
			{
				if ( caster->behavior == &actMagicTrapCeiling )
				{
					node_t* node = caster->children.first;
					Entity* ceilingModel = (Entity*)(node->element);
					entity->z = ceilingModel->z;
				}
			}
		}
		else if ( propulsion == PROPULSION_MISSILE_TRIO )
		{
			real_t angle = PI / 6;
			real_t baseSpeed = 2;
			real_t baseSideSpeed = 1;
			int sprite = 170;
			if ( !strcmp(spell->name, spell_stoneblood.name) )
			{
				angle = PI / 6;
				baseSpeed = 2;
			}
			else if ( !strcmp(spell->name, spell_acidSpray.name) )
			{
				sprite = 597;
				angle = PI / 16;
				baseSpeed = 3;
				baseSideSpeed = 2;
				traveltime = 20;
			}
			else if ( !strcmp(spell->name, spell_sprayWeb.name) )
			{
				sprite = 861;
				angle = PI / 16;
				baseSpeed = 3;
				baseSideSpeed = 2;
				traveltime = 20;
			}

			entity = newEntity(168, 1, map.entities, nullptr); // red magic ball
			entity->parent = caster->getUID();
			entity->x = caster->x;
			entity->y = caster->y;
			entity->z = -1;
			entity->sizex = 1;
			entity->sizey = 1;
			entity->yaw = caster->yaw;
			entity->flags[UPDATENEEDED] = true;
			entity->flags[PASSABLE] = true;
			entity->flags[BRIGHT] = true;
			entity->behavior = &actMagicMissile;
			entity->sprite = sprite;

			double missile_speed = baseSpeed * (element->mana / static_cast<double>(element->overload_multiplier)); //TODO: Factor in base mana cost?
			entity->vel_x = cos(entity->yaw) * (missile_speed);
			entity->vel_y = sin(entity->yaw) * (missile_speed);

			entity->skill[4] = 0;
			entity->skill[5] = traveltime;
			if ( using_magicstaff )
			{
				entity->actmagicCastByMagicstaff = 1;
			}
			else if ( usingSpellbook && spellBookBonusPercent > 0 )
			{
				entity->actmagicSpellbookBonus = spellBookBonusPercent;
			}
			node = list_AddNodeFirst(&entity->children);
			node->element = copySpell(spell);
			((spell_t*)node->element)->caster = caster->getUID();
			node->deconstructor = &spellDeconstructor;
			node->size = sizeof(spell_t);

			playSoundEntity(entity, spellGetCastSound(spell), 128);

			result = entity;

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
			entity1->flags[BRIGHT] = true;
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
			else if ( usingSpellbook && spellBookBonusPercent > 0 )
			{
				entity1->actmagicSpellbookBonus = spellBookBonusPercent;
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
			entity2->flags[BRIGHT] = true;
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
			else if ( usingSpellbook && spellBookBonusPercent > 0 )
			{
				entity2->actmagicSpellbookBonus = spellBookBonusPercent;
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
			chance = rand() % 5;
			if (chance == 1)
			{
				//Use some of that extra magic in this element.
				int amount = rand() % extramagic;
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
			if (!strcmp(element->name, spellElement_force.name))
			{
				//Give the spell force properties.
				if (propulsion == PROPULSION_MISSILE)
				{
					entity->sprite = 173;
				}
				if (newbie)
				{
					//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
					chance = rand() % 10;
					if (chance >= spellcasting / 10)
					{
						element->damage -= rand() % (100 / (spellcasting + 1));
					}
					if (element->damage < 10)
					{
						element->damage = 10;    //Range checking.
					}
				}
			}
			else if (!strcmp(element->name, spellElement_fire.name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					entity->sprite = 168;
					//entity->skill[4] = entity->x; //Store what x it started shooting out from the player at.
					//entity->skill[5] = entity->y; //Store what y it started shooting out from the player at.
					//entity->skill[12] = (100 * stat->PROFICIENCIES[PRO_SPELLCASTING]) + (100 * stat->PROFICIENCIES[PRO_MAGIC]) + (100 * (rand()%10)) + (10 * (rand()%10)) + (rand()%10); //How long this thing lives.

					//playSoundEntity( entity, 59, 128 );
				}
				if (newbie)
				{
					//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
					chance = rand() % 10;
					if (chance >= spellcasting / 10)
					{
						element->damage -= rand() % (100 / (spellcasting + 1));
					}
					if (element->damage < 10)
					{
						element->damage = 10;    //Range checking.
					}
				}
			}
			else if ( !strcmp(element->name, spellElement_lightning.name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					entity->sprite = 170;
				}
				if ( newbie )
				{
					//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
					chance = rand() % 10;
					if ( chance >= spellcasting / 10 )
					{
						element->damage -= rand() % (100 / (spellcasting + 1));
					}
					if ( element->damage < 10 )
					{
						element->damage = 10;    //Range checking.
					}
				}
			}
			else if ( !strcmp(element->name, spellElement_stoneblood.name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					entity->sprite = 170;
				}
			}
			else if ( !strcmp(element->name, spellElement_bleed.name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					entity->sprite = 643;
				}
			}
			else if (!strcmp(element->name, spellElement_confuse.name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					entity->sprite = 173;
				}
			}
			else if (!strcmp(element->name, spellElement_cold.name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					entity->sprite = 172;
				}
			}
			else if (!strcmp(element->name, spellElement_dig.name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					entity->sprite = 171;
				}
			}
			else if (!strcmp(element->name, spellElement_locking.name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					entity->sprite = 171;
				}
			}
			else if (!strcmp(element->name, spellElement_opening.name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					entity->sprite = 171;
				}
			}
			else if (!strcmp(element->name, spellElement_slow.name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					entity->sprite = 171;
				}
			}
			else if ( !strcmp(element->name, spellElement_poison.name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					entity->sprite = 597;
				}
			}
			else if (!strcmp(element->name, spellElement_sleep.name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					entity->sprite = 172;
				}
			}
			else if (!strcmp(element->name, spell_magicmissile.name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					entity->sprite = 173;
				}
			}
			else if ( !strcmp(element->name, spell_dominate.name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					entity->sprite = 168;
				}
			}
			else if ( !strcmp(element->name, spellElement_acidSpray.name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					entity->sprite = 171;
				}
			}
			else if ( !strcmp(element->name, spellElement_stealWeapon.name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					entity->sprite = 175;
				}
			}
			else if ( !strcmp(element->name, spellElement_drainSoul.name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					entity->sprite = 598;
				}
			}
			else if ( !strcmp(element->name, spellElement_charmMonster.name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					entity->sprite = 173;
				}
			}
			else if ( !strcmp(element->name, spellElement_telePull.name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					entity->sprite = 175;
				}
			}
			else if ( !strcmp(element->name, spellElement_shadowTag.name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					entity->sprite = 175;
				}
			}
			else if ( !strcmp(element->name, spellElement_demonIllusion.name) )
			{
				if ( propulsion == PROPULSION_MISSILE )
				{
					entity->sprite = 171;
				}
			}
		}
	}

	//Random chance to level up spellcasting skill.
	if ( !trap )
	{
		if ( using_magicstaff )
		{
			if ( stat )
			{
				// spellcasting increase chances.
				if ( stat->PROFICIENCIES[PRO_SPELLCASTING] < 60 )
				{
					if ( rand() % 6 == 0 ) //16.67%
					{
						caster->increaseSkill(PRO_SPELLCASTING);
					}
				}
				else if ( stat->PROFICIENCIES[PRO_SPELLCASTING] < 80 )
				{
					if ( rand() % 9 == 0 ) //11.11%
					{
						caster->increaseSkill(PRO_SPELLCASTING);
					}
				}
				else // greater than 80
				{
					if ( rand() % 12 == 0 ) //8.33%
					{
						caster->increaseSkill(PRO_SPELLCASTING);
					}
				}

				// magic increase chances.
				if ( stat->PROFICIENCIES[PRO_MAGIC] < 60 )
				{
					if ( rand() % 7 == 0 ) //14.2%
					{
						caster->increaseSkill(PRO_MAGIC);
					}
				}
				else if ( stat->PROFICIENCIES[PRO_MAGIC] < 80 )
				{
					if ( rand() % 10 == 0 ) //10.00%
					{
						caster->increaseSkill(PRO_MAGIC);
					}
				}
				else // greater than 80
				{
					if ( rand() % 13 == 0 ) //7.69%
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
				int castDifficulty = stat->PROFICIENCIES[PRO_SPELLCASTING] / 20 - spell->difficulty / 20;
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
				if ( (!strcmp(element->name, spellElement_light.name) || spell->ID == SPELL_REVERT_FORM)
						&& stat->PROFICIENCIES[PRO_SPELLCASTING] >= SKILL_LEVEL_SKILLED
						&& stat->PROFICIENCIES[PRO_MAGIC] >= SKILL_LEVEL_SKILLED )
				{
					// light provides no levelling past 40 in both spellcasting and magic.
					if ( rand() % 20 == 0 )
					{
						for ( int i = 0; i < MAXPLAYERS; ++i )
						{
							if ( players[i] && caster && (caster == players[i]->entity) )
							{
								messagePlayer(i, language[2591]);
							}
						}
					}
				}
				else
				{
					if ( rand() % spellCastChance == 0 )
					{
						caster->increaseSkill(PRO_SPELLCASTING);
					}

					if ( rand() % magicChance == 0 )
					{
						caster->increaseSkill(PRO_MAGIC); // otherwise you will basically never be able to learn all the spells in the game...
						if ( usingSpellbook && caster->behavior == &actPlayer )
						{
							if ( stats[caster->skill[2]] && stats[caster->skill[2]]->playerRace == RACE_INSECTOID && stats[caster->skill[2]]->appearance == 0 )
							{
								steamStatisticUpdateClient(caster->skill[2], STEAM_STAT_BOOKWORM, STEAM_STAT_INT, 1);
							}
						}
					}
				}
			}
		}
	}

	if ( !trap && usingSpellbook ) // degrade spellbooks on use.
	{
		int chance = 8;
		if ( stat->type == GOBLIN )
		{
			chance = 16;

			if ( caster && caster->behavior == &actPlayer && stat->playerRace == RACE_GOBLIN && stat->appearance == 0 )
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
		if ( rand() % chance == 0 && stat->shield && itemCategory(stat->shield) == SPELLBOOK )
		{
			caster->degradeArmor(*stat, *(stat->shield), 4);
			if ( stat->shield->status == BROKEN && player >= 0 )
			{
				if ( caster && caster->behavior == &actPlayer && stat->playerRace == RACE_GOBLIN && stat->appearance == 0 )
				{
					steamStatisticUpdateClient(player, STEAM_STAT_DYSLEXIA, STEAM_STAT_INT, 1);
				}
				Item* toBreak = stat->shield;
				consumeItem(toBreak, player);
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
	if ( !strcmp(spell->name, spell_fireball.name) )
	{
		return 164;
	}
	else if ( !strcmp(spell->name, spell_lightning.name) )
	{
		return 171;
	}
	else if ( !strcmp(spell->name, spell_cold.name) )
	{
		return 172;
	}
	else if ( !strcmp(spell->name, spell_bleed.name) )
	{
		return 171;
	}
	else if ( !strcmp(spell->name, spell_stoneblood.name) )
	{
		return 171;
	}
	else if ( !strcmp(spell->name, spell_acidSpray.name) )
	{
		return 164;
	}
	else if ( !strcmp(spell->name, spell_sprayWeb.name) )
	{
		return 169;
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
	if ( stat.playerRace == RACE_INSECTOID && stat.appearance == 0 && (spellID == SPELL_DASH || spellID == SPELL_FLUTTER || spellID == SPELL_ACID_SPRAY) )
	{
		return true;
	}
	else if ( stat.playerRace == RACE_VAMPIRE && stat.appearance == 0 && (spellID == SPELL_LEVITATION || spellID == SPELL_BLEED) )
	{
		return true;
	}
	else if ( stat.playerRace == RACE_SUCCUBUS && stat.appearance == 0 && (spellID == SPELL_TELEPORTATION || spellID == SPELL_SELF_POLYMORPH) )
	{
		return true;
	}
	else if ( stat.playerRace == RACE_INCUBUS && stat.appearance == 0 && (spellID == SPELL_TELEPORTATION || spellID == SPELL_SHADOW_TAG) )
	{
		return true;
	}
	else if ( stat.playerRace == RACE_AUTOMATON && stat.appearance == 0 && (spellID == SPELL_SALVAGE) )
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