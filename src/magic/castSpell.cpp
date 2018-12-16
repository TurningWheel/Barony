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

void castSpellInit(Uint32 caster_uid, spell_t* spell)
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

	if ( hudweapon )
	{
		if ( hudweapon->skill[0] != 0 )   //HUDWEAPON_CHOP.
		{
			return; //Can't cast spells while attacking.
		}
	}

	if ( cast_animation.active )
	{
		//Already casting spell.
		return;
	}

	int player = -1;
	int i = 0;
	for ( i = 0; i < numplayers; ++i )
	{
		if ( caster == players[i]->entity )
		{
			player = i; //Set the player.
		}
	}

	if ( player > -1 )
	{
		if ( stats[player]->defending )
		{
			messagePlayer(player, language[407]);
			return;
		}
		if (spell_isChanneled(spell))
		{
			if (channeledSpells[clientnum], spell)
			{
				for (node = channeledSpells[player].first; node; node = node->next)
				{
					spell_t* spell_search = (spell_t*)node->element;
					if (spell_search->ID == spell->ID)
					{
						//list_RemoveNode(node);
						//node = NULL;
						spell_search->sustain = false;
						//if (spell->magic_effects)
						//	list_RemoveNode(spell->magic_effects);
						messagePlayer(player, language[408], spell->name);
						if (multiplayer == CLIENT)
						{
							list_RemoveNode(node);
							strcpy( (char*)net_packet->data, "UNCH");
							net_packet->data[4] = clientnum;
							SDLNet_Write32(spell->ID, &net_packet->data[5]);
							net_packet->address.host = net_server.host;
							net_packet->address.port = net_server.port;
							net_packet->len = 9;
							sendPacketSafe(net_sock, -1, net_packet, 0);
						}
						return;
					}
				}
			}
			if ( spell->ID == SPELL_VAMPIRIC_AURA && caster->playerIsVampire() == PLAYER_VAMPIRE_CURSED )
			{
				if ( multiplayer == CLIENT )
				{
					messagePlayer(player, language[408], spell->name);
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
					messagePlayer(player, language[408], spell->name);
					caster->setEffect(EFF_VAMPIRICAURA, true, 1, false);
					caster->playerVampireCurse = 2; // cured.
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
	fireOffSpellAnimation(&cast_animation, caster->getUID(), spell);

	//castSpell(caster, spell); //For now, do this while the spell animations are worked on.
}

Entity* castSpell(Uint32 caster_uid, spell_t* spell, bool using_magicstaff, bool trap)
{
	Entity* caster = uidToEntity(caster_uid);

	if (!caster || !spell)
	{
		//Need a spell and caster to cast a spell.
		return NULL;
	}

	Entity* result = NULL; //If the spell spawns an entity (like a magic light ball or a magic missile), it gets stored here and returned.
#define spellcasting std::min(std::max(0,stat->PROFICIENCIES[PRO_SPELLCASTING]+statGetINT(stat)),100) //Shortcut!

	if (clientnum != 0 && multiplayer == CLIENT)
	{
		strcpy( (char*)net_packet->data, "SPEL" );
		net_packet->data[4] = clientnum;
		SDLNet_Write32(spell->ID, &net_packet->data[5]);
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 9;
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
	int i = 0;
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
	for (i = 0; i < numplayers; ++i)
	{
		if (caster == players[i]->entity)
		{
			player = i; //Set the player.
		}
	}

	bool newbie = false;
	if ( !using_magicstaff && !trap)
	{
		newbie = caster->isSpellcasterBeginner();

		/*magiccost = getCostOfSpell(spell);
		if (magiccost < 0) {
			if (player >= 0)
				messagePlayer(player, "Error: Invalid spell. Mana cost is negative?");
			return NULL;
		}*/
		if ( multiplayer == SINGLE )
		{
			if ( spell->ID == SPELL_FORCEBOLT && skillCapstoneUnlocked(player, PRO_SPELLCASTING) )
			{
				magiccost = 0;
			}
			else
			{
				magiccost = cast_animation.mana_left;
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
					else if ( player == 0 )
					{
						camera_shakex += 0.1;
						camera_shakey += 10;
						playSoundPlayer(player, 28, 92);
					}
				}
				caster->drainMP(magiccost);
			}
		}
	}

	if (newbie)
	{
		//So This wizard is a newbie.

		//First, drain some extra mana maybe.
		int chance = rand() % 10;
		if (chance >= spellcasting / 10)   //At skill 20, there's an 80% chance you'll use extra mana. At 70, there's a 30% chance.
		{
			extramagic = rand() % (300 / (spellcasting + 1)); //Use up extra mana. More mana used the lower your spellcasting skill.
			extramagic = std::min<real_t>(extramagic, stat->MP / 10); //To make sure it doesn't draw, say, 5000 mana. Cause dammit, if you roll a 1 here...you're doomed.
			caster->drainMP(extramagic);
		}

		//Now, there's a chance they'll fumble the spell.
		chance = rand() % 10;
		if (chance >= spellcasting / 10)
		{
			if (rand() % 3 == 1)
			{
				//Fizzle the spell.
				//TODO: Cool effects.
				playSoundEntity(caster, 163, 128);
				if (player >= 0)
				{
					messagePlayer(player, language[409]);
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
		if (players[player] && players[player]->entity)
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
			for (i = 0; i < numplayers; ++i)
			{
				if (caster == players[i]->entity)
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
						if ( !creature->isBossMonsterOrBossMap() )
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
			for (i = 0; i < numplayers; ++i)
			{
				if (caster == players[i]->entity)
				{
					serverUpdateEffects(i);
				}
			}

			playSoundEntity(caster, 178, 128 );
			spawnMagicEffectParticles(caster->x, caster->y, caster->z, 170);
		}
		else if (!strcmp(element->name, spellElement_teleportation.name))
		{
			caster->teleportRandom();
		}
		else if (!strcmp(element->name, spellElement_identify.name))
		{
			for (i = 0; i < numplayers; ++i)
			{
				if (caster == players[i]->entity)
				{
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
					if (i != 0)
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
						shootmode = false;
						gui_mode = GUI_MODE_INVENTORY; //Reset the GUI to the inventory.
						identifygui_active = true;
						identifygui_appraising = false;
						if ( removecursegui_active )
						{
							closeRemoveCurseGUI();
						}
						if ( openedChest[i] )
						{
							openedChest[i]->closeChest();
						}
						//identifygui_mode = true;

						//Initialize Identify GUI game controller code here.
						initIdentifyGUIControllerCode();
					}
				}
			}

			playSoundEntity(caster, 167, 128 );
		}
		else if (!strcmp(element->name, spellElement_removecurse.name))
		{
			for (i = 0; i < numplayers; ++i)
			{
				if (caster == players[i]->entity)
				{
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 169);
					if (i != 0)
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
						shootmode = false;
						gui_mode = GUI_MODE_INVENTORY; //Reset the GUI to the inventory.
						removecursegui_active = true;
						identifygui_active = false;

						if ( identifygui_active )
						{
							CloseIdentifyGUI();
						}

						if ( openedChest[i] )
						{
							openedChest[i]->closeChest();
						}

						initRemoveCurseGUIControllerCode();
					}
				}
			}

			playSoundEntity(caster, 167, 128 );
		}
		else if (!strcmp(element->name, spellElement_magicmapping.name))
		{
			for (i = 0; i < numplayers; ++i)
			{
				if (caster == players[i]->entity)
				{
					spawnMagicEffectParticles(caster->x, caster->y, caster->z, 171);
					spell_magicMap(i);
				}
			}

			playSoundEntity(caster, 167, 128 );
		}
		else if (!strcmp(element->name, spellElement_heal.name))     //TODO: Make it work for NPCs.
		{
			for (i = 0; i < numplayers; ++i)
			{
				if (caster == players[i]->entity)
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

					int totalHeal = 0;
					int oldHP = players[i]->entity->getHP();
					spell_changeHealth(players[i]->entity, amount);
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

			playSoundEntity(caster, 168, 128);
			spawnMagicEffectParticles(caster->x, caster->y, caster->z, 169);
		}
		else if (!strcmp(element->name, spellElement_cure_ailment.name))     //TODO: Generalize it for NPCs too?
		{
			for (i = 0; i < numplayers; ++i)
			{
				if (caster == players[i]->entity)
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
							if ( !(c == EFF_VAMPIRICAURA && stats[i]->EFFECTS_TIMERS[c] == -2) )
							{
								stats[i]->EFFECTS[c] = false;
								stats[i]->EFFECTS_TIMERS[c] = 0;
							}
						}
					}
					if ( players[i]->entity->flags[BURNING] )
					{
						players[i]->entity->flags[BURNING] = false;
						serverUpdateEntityFlag(players[i]->entity, BURNING);
					}
					serverUpdateEffects(player);
					playSoundEntity(entity, 168, 128);

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
									if ( !(c == EFF_VAMPIRICAURA && target_stat->EFFECTS_TIMERS[c] == -2) )
									{
										target_stat->EFFECTS[c] = false;
										target_stat->EFFECTS_TIMERS[c] = 0;
									}
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
					Entity* follower = uidToEntity(*((Uint32*)(node)->element));
					if ( follower->monsterAllySummonRank != 0 )
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
				timer->particleTimerEndAction == PARTICLE_EFFECT_SPELL_SUMMON;
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
			for ( i = 0; i < numplayers; ++i )
			{
				if ( caster == players[i]->entity )
				{
					serverUpdateEffects(i);
				}
			}

			playSoundEntity(caster, 166, 128 );
			spawnMagicEffectParticles(caster->x, caster->y, caster->z, 174);
		}
		else if ( !strcmp(element->name, spellElement_vampiricAura.name) )
		{
			channeled_spell = spellEffectVampiricAura(caster, spell, extramagic_to_use);
			//Also refactor the duration determining code.
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
			node = list_AddNodeFirst(&entity->children);
			node->element = copySpell(spell);
			((spell_t*)node->element)->caster = caster->getUID();
			node->deconstructor = &spellDeconstructor;
			node->size = sizeof(spell_t);

			int volume = 128;
			if ( trap )
			{
				volume = 96;
			}
			if ( !strcmp(spell->name, spell_fireball.name) )
			{
				playSoundEntity(entity, 164, volume);
			}
			else if ( !strcmp(spell->name, spell_lightning.name) )
			{
				playSoundEntity(entity, 171, volume);
			}
			else if ( !strcmp(spell->name, spell_cold.name) )
			{
				playSoundEntity(entity, 172, 64);
			}
			else if ( !strcmp(spell->name, spell_bleed.name) )
			{
				playSoundEntity(entity, 171, volume);
			}
			else if ( !strcmp(spell->name, spell_stoneblood.name) )
			{
				playSoundEntity(entity, 171, volume);
			}
			else if ( !strcmp(spell->name, spell_acidSpray.name) )
			{
				playSoundEntity(entity, 164, volume);
				traveltime = 15;
				entity->skill[5] = traveltime;
			}
			else
			{
				playSoundEntity(entity, 169, volume);
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
			node = list_AddNodeFirst(&entity->children);
			node->element = copySpell(spell);
			((spell_t*)node->element)->caster = caster->getUID();
			node->deconstructor = &spellDeconstructor;
			node->size = sizeof(spell_t);

			if ( !strcmp(spell->name, spell_stoneblood.name) )
			{
				playSoundEntity(entity, 171, 128);
			}
			else if ( !strcmp(spell->name, spell_acidSpray.name) )
			{
				playSoundEntity(entity, 164, 128);
			}

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
			else if (!strcmp(element->name, spellElement_sleep.name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					entity->sprite = 172;
				}
			}
			else if (!strcmp(spell->name, spell_magicmissile.name))
			{
				if (propulsion == PROPULSION_MISSILE)
				{
					entity->sprite = 173;
				}
			}
			else if ( !strcmp(spell->name, spell_dominate.name) )
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
				//messagePlayer(0, "Difficulty: %d, chance 1 in %d, 1 in %d", castDifficulty, spellCastChance, magicChance);
				if ( !strcmp(element->name, spellElement_light.name)
					&& stat->PROFICIENCIES[PRO_SPELLCASTING] >= SKILL_LEVEL_SKILLED
					&& stat->PROFICIENCIES[PRO_MAGIC] >= SKILL_LEVEL_SKILLED )
				{
					// light provides no levelling past 40 in both spellcasting and magic.
					if ( rand() % 20 == 0 )
					{
						for ( i = 0; i < numplayers; ++i )
						{
							if ( caster == players[i]->entity )
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
					}
				}
			}
		}
	}

	if (spell_isChanneled(spell) && !using_magicstaff)   //TODO: What about magic traps and channeled spells?
	{
		if (!channeled_spell)
		{
			printlog( "What. Spell is channeled but no channeled_spell pointer? What sorcery is this?\n");
		}
		else
		{
			int target_client = 0;
			for (i = 0; i < numplayers; ++i)
			{
				if (players[i]->entity == caster)
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
