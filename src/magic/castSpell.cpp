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
#include "magic.hpp"

void castSpellInit(Uint32 caster_uid, spell_t *spell){
	Entity *caster = uidToEntity(caster_uid);
	node_t *node = NULL;
	if (!caster || !spell)
	{
		//Need a spell and caster to cast a spell.
		return;
	}

	if (!spell->elements.first) {
		return;
	}

	if (hudweapon) {
		if (hudweapon->skill[0] != 0) { //HUDWEAPON_CHOP.
			return; //Can't cast spells while attacking.
		}
	}

	if (cast_animation.active) {
		//Already casting spell.
		return;
	}

	int player = -1;
	int i = 0;
	for (i = 0; i < numplayers; ++i) {
		if (caster == players[i]) {
			player = i; //Set the player.
		}
	}

	if (player > -1) {
		if( stats[player]->defending ) {
			messagePlayer(player,language[407]);
			return;
		}
		if (spell_isChanneled(spell)) {
			if (channeledSpells[clientnum], spell) {
				for (node = channeledSpells[player].first; node; node = node->next) {
					spell_t *spell_search = (spell_t*)node->element;
					if (spell_search->ID == spell->ID) {
						//list_RemoveNode(node);
						//node = NULL;
						spell_search->sustain = FALSE;
						//if (spell->magic_effects)
						//	list_RemoveNode(spell->magic_effects);
						messagePlayer(player, language[408], spell->name);
						if (multiplayer == CLIENT) {
							list_RemoveNode(node);
							strcpy( (char *)net_packet->data, "UNCH");
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
		}
	}

	int magiccost = 0;
	//Entity *entity = NULL;
	//node_t *node = spell->elements->first;

	Stat *stat = caster->getStats();
	if( !stat )
		return;
	
	if (stat->EFFECTS[EFF_PARALYZED]) {
		return;		
	}

	magiccost = getCostOfSpell(spell);
	if (magiccost > stat->MP) {
		if (player >= 0)
			messagePlayer(player, language[375]); //TODO: Allow overexpending at the cost of extreme danger? (maybe an immensely powerful tree of magic actually likes this -- using your life-force to power spells instead of mana)
		return;
	}
	if (magiccost < 0) {
		if (player >= 0)
			messagePlayer(player, "Error: Invalid spell. Mana cost is negative?");
		return;
	}

	//Hand the torch off to the spell animator. And stuff. Stuff. I mean spell animation handler thingymabobber.
	fireOffSpellAnimation(&cast_animation, caster->uid, spell);

	//castSpell(caster, spell); //For now, do this while the spell animations are worked on.
}

Entity* castSpell(Uint32 caster_uid, spell_t *spell, bool using_magicstaff, bool trap) {
	Entity *caster = uidToEntity(caster_uid);

	if (!caster || !spell)
	{
		//Need a spell and caster to cast a spell.
		return NULL;
	}

	Entity *result = NULL; //If the spell spawns an entity (like a magic light ball or a magic missile), it gets stored here and returned.
	#define spellcasting std::min(std::max(0,stat->PROFICIENCIES[PRO_SPELLCASTING]+statGetINT(stat)),100) //Shortcut!

	if (clientnum != 0 && multiplayer == CLIENT) {
		strcpy( (char *)net_packet->data, "SPEL" );
		net_packet->data[4] = clientnum;
		SDLNet_Write32(spell->ID, &net_packet->data[5]);
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 9;
		sendPacketSafe(net_sock, -1, net_packet, 0);
		return NULL;
	}

	if (!spell->elements.first) {
		return NULL;
	}
	
	//node_t *node = spell->types->first;

	#define PROPULSION_MISSILE 1
	int i = 0;
	int chance = 0;
	int propulsion = 0;
	int traveltime = 0;
	int magiccost = 0;
	int extramagic = 0; //Extra magic drawn in from the caster being a newbie.
	int extramagic_to_use = 0; //Instead of doing element->mana (which causes bugs), this is an extra factor in the mana equations. Pumps extra mana into elements from extramagic.
	Entity *entity = NULL;
	spell_t *channeled_spell=NULL; //Pointer to the spell if it's a channeled spell. For the purpose of giving it its node in the channeled spell list.
	node_t *node = spell->elements.first;

	Stat *stat = caster->getStats();

	int player = -1;
	for (i = 0; i < numplayers; ++i) {
		if (caster == players[i]) {
			player = i; //Set the player.
		}
	}

	bool newbie = FALSE;
	if( !using_magicstaff && !trap) {
		if (stat->PROFICIENCIES[PRO_SPELLCASTING] < SPELLCASTING_BEGINNER) {
			newbie = TRUE; //The caster has lower spellcasting skill. Cue happy fun times.
		}

		/*magiccost = getCostOfSpell(spell);
		if (magiccost < 0) {
			if (player >= 0)
				messagePlayer(player, "Error: Invalid spell. Mana cost is negative?");
			return NULL;
		}*/
		if (multiplayer == SINGLE) {
			magiccost = cast_animation.mana_left;
			caster->drainMP(magiccost);
		} else {
			magiccost = getCostOfSpell(spell);
			caster->drainMP(magiccost);
		}
	}

	if (newbie) {
		//So This wizard is a newbie.

		//First, drain some extra mana maybe.
		int chance = rand()%10;
		if (chance >= spellcasting/10) { //At skill 20, there's an 80% chance you'll use extra mana. At 70, there's a 30% chance.
			extramagic = rand()%(300/(spellcasting+1)); //Use up extra mana. More mana used the lower your spellcasting skill.
			extramagic = std::min(extramagic, stat->MP / 10); //To make sure it doesn't draw, say, 5000 mana. Cause dammit, if you roll a 1 here...you're doomed.
			caster->drainMP(extramagic);
		}

		//Now, there's a chance they'll fumble the spell.
		chance = rand()%10;
		if (chance >= spellcasting/10) {
			if (rand()%3 == 1) {
				//Fizzle the spell.
				//TODO: Cool effects.
				playSoundEntity(caster,163,128);
				if (player >= 0)
					messagePlayer(player, language[409]);
				return NULL;
			}
		}
	}

	//Check if the bugger is levitating.
	bool levitating = FALSE;
	if (!trap) {
		if (stat->EFFECTS[EFF_LEVITATING] == TRUE )
			levitating=TRUE;
		if (stat->ring != NULL )
			if (stat->ring->type == RING_LEVITATION )
				levitating = TRUE;
		if (stat->shoes != NULL)
			if (stat->shoes->type == STEEL_BOOTS_LEVITATION )
				levitating = TRUE;
	}

	//Water walking boots
	bool waterwalkingboots = FALSE;
	if (!trap) {
		if (stat->shoes != NULL)
			if (stat->shoes->type == IRON_BOOTS_WATERWALKING )
				waterwalkingboots = TRUE;
	}

	node_t *node2; //For traversing the map looking for...liquids?
	//Check if swimming.
	if (!waterwalkingboots && !levitating && !trap && player>=0) {
		bool swimming=FALSE;
		if( players[player] ) {
			int x = std::min<int>(std::max(0.0,floor(caster->x/16)),map.width-1);
			int y = std::min<int>(std::max(0.0,floor(caster->y/16)),map.height-1);
			if( animatedtiles[map.tiles[y*MAPLAYERS+x*MAPLAYERS*map.height]] )
				swimming=TRUE;
		}
		if( swimming ) {
			//Can't cast spells while swimming if not levitating or water walking.
			if (player >= 0)
				messagePlayer(player, language[410]);
			return NULL;
		}
	}

	//Right. First, grab the root element, which is what determines the delivery system.
	//spellElement_t *element = (spellElement_t *)spell->elements->first->element;
	spellElement_t *element = (spellElement_t *)node->element;
	if (element) {
		extramagic_to_use = 0;
		/*if (magiccost > stat->MP) {
			if (player >= 0)
				messagePlayer(player, "Insufficient mana!"); //TODO: Allow overexpending at the cost of extreme danger? (maybe an immensely powerful tree of magic actually likes this -- using your life-force to power spells instead of mana)
			return NULL;
		}*/

		if (extramagic > 0) {
			//Extra magic. Pump it in here?
			chance = rand()%5;
			if (chance == 1) {
				//Use some of that extra magic in this element.
				int amount = rand()%extramagic;
				extramagic -= amount;
				extramagic_to_use += amount;
			}
		}

		if (!strcmp(element->name, spellElement_missile.name)) {
			//Set the propulsion to missile.
			propulsion = PROPULSION_MISSILE;
			traveltime = element->duration;
			if (newbie) {
				//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
				chance = rand()%10;
				if (chance >= spellcasting/10)
					traveltime -= rand()%(1000/(spellcasting+1));
				if (traveltime < 30)
					traveltime = 30; //Range checking.
			}
			traveltime += (((element->mana + extramagic_to_use) - element->base_mana) / element->overload_multiplier) * element->duration;
		} else if (!strcmp(element->name, spellElement_light.name)) {
			entity = newEntity(175, 1, map.entities); // black magic ball
			entity->parent = caster->uid;
			entity->x = caster->x;
			entity->y = caster->y;
			entity->z = -5.5 + ((-6.5f + -4.5f) / 2) * sin(0);
			entity->skill[7] = -5.5; //Base z.
			entity->sizex = 1;
			entity->sizey = 1;
			entity->yaw = caster->yaw;
			entity->flags[UPDATENEEDED]=TRUE;
			entity->flags[PASSABLE]=TRUE;
			entity->flags[BRIGHT]=TRUE;
			entity->behavior=&actMagiclightBall;
			entity->skill[4] = entity->x; //Store what x it started shooting out from the player at.
			entity->skill[5] = entity->y; //Store what y it started shooting out from the player at.
			entity->skill[12] = (element->duration * (((element->mana + extramagic_to_use) / element->base_mana) * element->overload_multiplier)); //How long this thing lives.
			node_t *spellnode = list_AddNodeLast(&entity->children);
			spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
			channeled_spell = (spell_t*)(spellnode->element);
			spellnode->size = sizeof(spell_t);
			((spell_t *)spellnode->element)->caster = caster->uid;
			if( using_magicstaff )
				((spell_t *)spellnode->element)->magicstaff = TRUE;
			spellnode->deconstructor = &spellDeconstructor;
			if (newbie) {
				//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
				chance = rand()%10;
				if (chance >= spellcasting/10) {
					// lifespan of the lightball
					entity->skill[12] -= rand()%(2000/(spellcasting+1));
					if (entity->skill[12] < 180)
						entity->skill[12] = 180; //Range checking.
				}
			}
			if (using_magicstaff || trap) {
				entity->skill[12] = MAGICSTAFF_LIGHT_DURATION; //TODO: Grab the duration from the magicstaff or trap?
				((spell_t *)spellnode->element)->sustain = FALSE;
			} else {
				entity->skill[12] /= getCostOfSpell((spell_t *)spellnode->element);
			}
			((spell_t *)spellnode->element)->channel_duration = entity->skill[12]; //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
			result = entity;

			playSoundEntity(entity, 165, 128 );
		} else if (!strcmp(element->name, spellElement_invisible.name)) {
			int duration = element->duration;
			duration += (((element->mana + extramagic_to_use) - element->base_mana) / element->overload_multiplier) * element->duration;
			node_t *spellnode = list_AddNodeLast(&caster->getStats()->magic_effects);
			spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
			channeled_spell = (spell_t*)(spellnode->element);
			channeled_spell->magic_effects_node = spellnode;
			spellnode->size = sizeof(spell_t);
			((spell_t *)spellnode->element)->caster = caster->uid;
			spellnode->deconstructor = &spellDeconstructor;
			if (newbie) {
				//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
				chance = rand()%10;
				if (chance >= spellcasting/10)
					duration -= rand()%(1000/(spellcasting+1));
				if (duration < 180)
					duration = 180; //Range checking.
			}
			duration /= getCostOfSpell((spell_t *)spellnode->element);
			channeled_spell->channel_duration = duration; //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
			stat->EFFECTS[EFF_INVISIBLE] = TRUE;
			stat->EFFECTS_TIMERS[EFF_INVISIBLE] = duration;
			for (i = 0; i < numplayers; ++i) {
				if (caster == players[i]) {
					serverUpdateEffects(i);
				}
			}

			playSoundEntity(caster, 166, 128 );
			spawnMagicEffectParticles(caster->x,caster->y,caster->z,174);
		} else if (!strcmp(element->name, spellElement_levitation.name)) {
			int duration = element->duration;
			duration += (((element->mana + extramagic_to_use) - element->base_mana) / element->overload_multiplier) * element->duration;
			node_t *spellnode = list_AddNodeLast(&caster->getStats()->magic_effects);
			spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
			channeled_spell = (spell_t*)(spellnode->element);
			channeled_spell->magic_effects_node = spellnode;
			spellnode->size = sizeof(spell_t);
			((spell_t *)spellnode->element)->caster = caster->uid;
			spellnode->deconstructor = &spellDeconstructor;
			if (newbie) {
				//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
				chance = rand()%10;
				if (chance >= spellcasting/10)
					duration -= rand()%(1000/(spellcasting+1));
				if (duration < 180)
					duration = 180; //Range checking.
			}
			duration /= getCostOfSpell((spell_t *)spellnode->element);
			channeled_spell->channel_duration = duration; //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
			stat->EFFECTS[EFF_LEVITATING] = TRUE;
			stat->EFFECTS_TIMERS[EFF_LEVITATING] = duration;
			for (i = 0; i < numplayers; ++i) {
				if (caster == players[i]) {
					serverUpdateEffects(i);
				}
			}

			playSoundEntity(caster, 178, 128 );
			spawnMagicEffectParticles(caster->x,caster->y,caster->z,170);
		} else if (!strcmp(element->name, spellElement_teleportation.name)) {
			caster->teleportRandom();
		} else if (!strcmp(element->name, spellElement_identify.name)) {
			for (i = 0; i < numplayers; ++i) {
				if (caster == players[i]) {
					spawnMagicEffectParticles(caster->x,caster->y,caster->z,171);
					if (i != 0) {
						//Tell the client to identify an item.
						strcpy((char *)net_packet->data,"IDEN");
						net_packet->address.host = net_clients[i - 1].host;
						net_packet->address.port = net_clients[i - 1].port;
						net_packet->len = 4;
						sendPacketSafe(net_sock, -1, net_packet, i-1);
					} else {
						//Identify an item.
						shootmode = FALSE;
						gui_mode = GUI_MODE_INVENTORY; //Reset the GUI to the inventory.
						identifygui_active = TRUE;
						identifygui_appraising = FALSE;
						//identifygui_mode = TRUE;
					}
				}
			}

			playSoundEntity(caster, 167, 128 );
		} else if (!strcmp(element->name, spellElement_removecurse.name)) {
			for (i = 0; i < numplayers; ++i) {
				if (caster == players[i]) {
					spawnMagicEffectParticles(caster->x,caster->y,caster->z,169);
					if (i != 0) {
						//Tell the client to uncurse an item.
						strcpy((char *)net_packet->data,"RCUR");
						net_packet->address.host = net_clients[i - 1].host;
						net_packet->address.port = net_clients[i - 1].port;
						net_packet->len = 4;
						sendPacketSafe(net_sock, -1, net_packet, i-1);
					} else {
						//Uncurse an item
						shootmode = FALSE;
						gui_mode = GUI_MODE_INVENTORY; //Reset the GUI to the inventory.
						removecursegui_active = TRUE;
					}
				}
			}

			playSoundEntity(caster, 167, 128 );
		} else if (!strcmp(element->name, spellElement_magicmapping.name)) {
			for (i = 0; i < numplayers; ++i) {
				if (caster == players[i]) {
					spawnMagicEffectParticles(caster->x,caster->y,caster->z,171);
					spell_magicMap(i);
				}
			}

			playSoundEntity(caster, 167, 128 );
		} else if (!strcmp(element->name, spellElement_heal.name)) { //TODO: Make it work for NPCs.
			for (i = 0; i < numplayers; ++i) {
				if (caster == players[i]) {
					int amount = element->damage * (((element->mana + extramagic_to_use) / element->base_mana) * element->overload_multiplier); //Amount to heal.
					if (newbie) {
						//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
						chance = rand()%10;
						if (chance >= spellcasting/10)
							amount -= rand()%(1000/(spellcasting+1));
						if (amount < 8)
							amount = 8; //Range checking.
					}
					spell_changeHealth(players[i], amount);
					playSoundEntity(caster, 168, 128);

					for(node = map.entities->first; node->next; node = node->next) {
						entity = (Entity *)(node->element);
						if ( !entity ||  entity==caster )
							continue;
						if( entity->behavior!=&actPlayer && entity->behavior!=&actMonster )
							continue;

						if (entityDist(entity, caster) <= HEAL_RADIUS && entity->checkFriend(caster)) {
							spell_changeHealth(entity, amount);
							playSoundEntity(entity, 168, 128);
							spawnMagicEffectParticles(entity->x,entity->y,entity->z,169);
						}
					}
					break;
				}
			}

			playSoundEntity(caster, 168, 128);
			spawnMagicEffectParticles(caster->x,caster->y,caster->z,169);
		} else if (!strcmp(element->name, spellElement_cure_ailment.name)) { //TODO: Generalize it for NPCs too?
			for (i = 0; i < numplayers; ++i) {
				if (caster == players[i]) {
					Uint32 color = SDL_MapRGB(mainsurface->format,0,255,0);
					messagePlayerColor(i,color,language[411]);
					int c = 0;
					for (c = 0; c < NUMEFFECTS; ++c) { //This does a whole lot more than just cure ailments.
						stats[i]->EFFECTS[c]=FALSE;
						stats[i]->EFFECTS_TIMERS[c]=0;
					}
					serverUpdateEffects(player);
					playSoundEntity(entity, 168, 128);

					for(node = map.entities->first; node->next; node = node->next) {
						entity = (Entity *)(node->element);
						if( !entity || entity==caster )
							continue;
						if( entity->behavior!=&actPlayer && entity->behavior!=&actMonster )
							continue;
						Stat *target_stat = entity->getStats();
						if( target_stat ) {
							if (entityDist(entity, caster) <= HEAL_RADIUS && entity->checkFriend(caster)) {
								for (c = 0; c < NUMEFFECTS; ++c) { //This does a whole lot more than just cure ailments.
									target_stat->EFFECTS[c]=FALSE;
									target_stat->EFFECTS_TIMERS[c]=0;
								}
								if( entity->behavior==&actPlayer )
									serverUpdateEffects(entity->skill[2]);
								if( entity->flags[BURNING] ) {
									entity->flags[BURNING] = FALSE;
									serverUpdateEntityFlag(entity,BURNING);
								}
								playSoundEntity(entity, 168, 128);
								spawnMagicEffectParticles(entity->x,entity->y,entity->z,169);
							}
						}
					}
					break;
				}
			}

			playSoundEntity(caster, 168, 128 );
			spawnMagicEffectParticles(caster->x,caster->y,caster->z,169);
		}

		if (propulsion == PROPULSION_MISSILE) {
			entity = newEntity(168, 1, map.entities); // red magic ball
			entity->parent = caster->uid;
			entity->x = caster->x;
			entity->y = caster->y;
			entity->z = -1;
			entity->sizex = 1;
			entity->sizey = 1;
			entity->yaw = caster->yaw;
			entity->flags[UPDATENEEDED]=TRUE;
			entity->flags[PASSABLE]=TRUE;
			entity->flags[BRIGHT]=TRUE;
			entity->behavior = &actMagicMissile;
			
			double missile_speed = 4 * ((double)element->mana / element->overload_multiplier); //TODO: Factor in base mana cost?
			entity->vel_x = cos(entity->yaw) * (missile_speed);
			entity->vel_y = sin(entity->yaw) * (missile_speed);

			entity->skill[4] = 0;
			entity->skill[5] = traveltime;
			node = list_AddNodeFirst(&entity->children);
			node->element = copySpell(spell);
			((spell_t *)node->element)->caster = caster->uid;
			node->deconstructor = &spellDeconstructor;
			node->size = sizeof(spell_t);

			if( !strcmp(spell->name, spell_fireball.name) )
				playSoundEntity(entity, 164, 128 );
			else if( !strcmp(spell->name, spell_lightning.name) )
				playSoundEntity(entity, 171, 128 );
			else if( !strcmp(spell->name, spell_cold.name) )
				playSoundEntity(entity, 172, 128 );
			else
				playSoundEntity(entity, 169, 128 );
			result = entity;
		}

		extramagic_to_use = 0;
		if (extramagic > 0) {
			//Extra magic. Pump it in here?
			chance = rand()%5;
			if (chance == 1) {
				//Use some of that extra magic in this element.
				int amount = rand()%extramagic;
				extramagic -= amount;
				extramagic_to_use += amount; //TODO: Make the elements here use this? Looks like they won't, currently. Oh well.
			}
		}
		//TODO: Add the status/conditional elements/modifiers (probably best as elements) too. Like onCollision or something.
		//element = (spellElement_t *)element->elements->first->element;
		node = element->elements.first;
		if( node ) {
			element = (spellElement_t *)node->element;
			if (!strcmp(element->name, spellElement_force.name)) {
				//Give the spell force properties.
				if (propulsion == PROPULSION_MISSILE) {
					entity->sprite = 173;
				}
				if (newbie) {
					//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
					chance = rand()%10;
					if (chance >= spellcasting/10)
						element->damage -= rand()%(100/(spellcasting+1));
					if (element->damage < 10)
						element->damage = 10; //Range checking.
				}
			} else if (!strcmp(element->name, spellElement_fire.name)) {
				if (propulsion == PROPULSION_MISSILE) {
					entity->sprite = 168;
					//entity->skill[4] = entity->x; //Store what x it started shooting out from the player at.
					//entity->skill[5] = entity->y; //Store what y it started shooting out from the player at.
					//entity->skill[12] = (100 * stat->PROFICIENCIES[PRO_SPELLCASTING]) + (100 * stat->PROFICIENCIES[PRO_MAGIC]) + (100 * (rand()%10)) + (10 * (rand()%10)) + (rand()%10); //How long this thing lives.

					//playSoundEntity( entity, 59, 128 );
				}
				if (newbie) {
					//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
					chance = rand()%10;
					if (chance >= spellcasting/10)
						element->damage -= rand()%(100/(spellcasting+1));
					if (element->damage < 10)
						element->damage = 10; //Range checking.
				}
			} else if (!strcmp(element->name, spellElement_lightning.name)) {
				if (propulsion == PROPULSION_MISSILE) {
					entity->sprite = 170;
				}
				if (newbie) {
					//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
					chance = rand()%10;
					if (chance >= spellcasting/10)
						element->damage -= rand()%(100/(spellcasting+1));
					if (element->damage < 10)
						element->damage = 10; //Range checking.
				}
			} else if (!strcmp(element->name, spellElement_confuse.name)) {
				if (propulsion == PROPULSION_MISSILE) {
					entity->sprite = 173;
				}
			} else if (!strcmp(element->name, spellElement_cold.name)) {
				if (propulsion == PROPULSION_MISSILE) {
					entity->sprite = 172;
				}
			} else if (!strcmp(element->name, spellElement_dig.name)) {
				if (propulsion == PROPULSION_MISSILE) {
					entity->sprite = 171;
				}
			} else if (!strcmp(element->name, spellElement_locking.name)) {
				if (propulsion == PROPULSION_MISSILE) {
					entity->sprite = 171;
				}
			} else if (!strcmp(element->name, spellElement_opening.name)) {
				if (propulsion == PROPULSION_MISSILE) {
					entity->sprite = 171;
				}
			} else if (!strcmp(element->name, spellElement_slow.name)) {
				if (propulsion == PROPULSION_MISSILE) {
					entity->sprite = 171;
				}
			} else if (!strcmp(element->name, spellElement_sleep.name)) {
				if (propulsion == PROPULSION_MISSILE) {
					entity->sprite = 172;
				}
			} else if (!strcmp(spell->name, spell_magicmissile.name)) {
				if (propulsion == PROPULSION_MISSILE) {
					entity->sprite = 173;
				}
			}
		}
	}

	//Random chance to level up spellcasting skill.
	if(rand()%4==0) {
		caster->increaseSkill(PRO_SPELLCASTING);
	}
	if(rand()%5==0) {
		caster->increaseSkill(PRO_MAGIC); // otherwise you will basically never be able to learn all the spells in the game...
	}

	if (spell_isChanneled(spell) && !using_magicstaff) { //TODO: What about magic traps and channeled spells?
		if (!channeled_spell) {
				printlog( "What. Spell is channeled but no channeled_spell pointer? What sorcery is this?\n");
		} else {
			int target_client = 0;
			for (i = 0; i < numplayers; ++i) {
				if (players[i] == caster) {
					target_client = i;
				}
			}
			//printlog( "Client is: %d\n", target_client);
			if (multiplayer == SERVER && target_client != 0) {
				strcpy( (char *)net_packet->data, "CHAN" );
				net_packet->data[4] = clientnum;
				SDLNet_Write32(spell->ID, &net_packet->data[5]);
				net_packet->address.host = net_clients[target_client - 1].host;
				net_packet->address.port = net_clients[target_client - 1].port;
				net_packet->len = 9;
				sendPacketSafe(net_sock, -1, net_packet, target_client-1);
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