/*-------------------------------------------------------------------------------

	BARONY
	File: magic.cpp
	Desc: contains magic definitions

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
#include "../player.hpp"
#include "magic.hpp"

void freeSpells()
{
	list_FreeAll(&spell_forcebolt.elements);
	list_FreeAll(&spell_magicmissile.elements);
	list_FreeAll(&spell_cold.elements);
	list_FreeAll(&spell_fireball.elements);
	list_FreeAll(&spell_lightning.elements);
	list_FreeAll(&spell_removecurse.elements);
	list_FreeAll(&spell_light.elements);
	list_FreeAll(&spell_identify.elements);
	list_FreeAll(&spell_magicmapping.elements);
	list_FreeAll(&spell_sleep.elements);
	list_FreeAll(&spell_confuse.elements);
	list_FreeAll(&spell_slow.elements);
	list_FreeAll(&spell_opening.elements);
	list_FreeAll(&spell_locking.elements);
	list_FreeAll(&spell_levitation.elements);
	list_FreeAll(&spell_invisibility.elements);
	list_FreeAll(&spell_teleportation.elements);
	list_FreeAll(&spell_healing.elements);
	list_FreeAll(&spell_extrahealing.elements);
	list_FreeAll(&spell_cureailment.elements);
	list_FreeAll(&spell_dig.elements);
	list_FreeAll(&spell_summon.elements);
	list_FreeAll(&spell_stoneblood.elements);
	list_FreeAll(&spell_bleed.elements);
	list_FreeAll(&spell_dominate.elements);
	list_FreeAll(&spell_reflectMagic.elements);
	list_FreeAll(&spell_acidSpray.elements);
	list_FreeAll(&spell_stealWeapon.elements);
	list_FreeAll(&spell_drainSoul.elements);
	list_FreeAll(&spell_vampiricAura.elements);
	list_FreeAll(&spell_charmMonster.elements);
}

void spell_magicMap(int player)
{
	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	if ( multiplayer == SERVER && player > 0 )
	{
		//Tell the client to map the magic.
		strcpy((char*)net_packet->data, "MMAP");
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 4;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
		return;
	}

	messagePlayer(player, language[412]);
	mapLevel(player);
}

void spell_summonFamiliar(int player)
{
	// server only function
	if ( players[player] == nullptr || players[player]->entity == nullptr )
	{
		return;
	}

	Uint32 numCreatures = 1;
	Monster creature = RAT;

	// spawn something really nasty
	/*numCreatures = 1;
	switch ( rand() % 4 )
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
	}*/
	// spawn moderately nasty things
	//switch ( rand() % 6 )
	//{
	//	case 0:
	//		creature = GNOME;
	//		numCreatures = rand() % 3 + 1;
	//		break;
	//	case 1:
	//		creature = SPIDER;
	//		numCreatures = rand() % 2 + 1;
	//		break;
	//	case 2:
	//		creature = SUCCUBUS;
	//		numCreatures = rand() % 2 + 1;
	//		break;
	//	case 3:
	//		creature = SCORPION;
	//		numCreatures = rand() % 2 + 1;
	//		break;
	//	case 4:
	//		creature = GHOUL;
	//		numCreatures = rand() % 2 + 1;
	//		break;
	//	case 5:
	//		creature = GOBLIN;
	//		numCreatures = rand() % 2 + 1;
	//		break;
	//}

	// spawn weak monster ally
	switch ( rand() % 3 )
	{
		case 0:
			creature = RAT;
			numCreatures = rand() % 3 + 1;
			break;
		case 1:
			creature = GHOUL;
			numCreatures = 1;
			break;
		case 2:
			creature = SLIME;
			numCreatures = rand() % 2 + 1;
			break;
	}

	//// spawn humans
	//creature = HUMAN;
	//numCreatures = rand() % 3 + 1;

	////Spawn many/neat allies
	//switch ( rand() % 2 )
	//{
	//	case 0:
	//		// summon zap brigadiers
	//		numCreatures = rand() % 2 + 4;
	//		creature = HUMAN;
	//		break;
	//	case 1:
	//		// summon demons
	//		numCreatures = rand() % 2 + 4;
	//		creature = DEMON;
	//		break;
	//
	//}

	int i;
	bool spawnedMonster = false;
	for ( i = 0; i < numCreatures; ++i )
	{
		Entity* monster = summonMonster(creature, floor(players[player]->entity->x / 16) * 16 + 8, floor(players[player]->entity->y / 16) * 16 + 8);

		if ( monster )
		{
			spawnedMonster = true;

			Stat* monsterStats = monster->getStats();
			if ( monsterStats )
			{
				monsterStats->leader_uid = players[player]->entity->getUID();
				if ( !monsterally[HUMAN][monsterStats->type] )
				{
					monster->flags[USERFLAG2] = true;
				}
				monster->monsterAllyIndex = player;
				if ( multiplayer == SERVER )
				{
					serverUpdateEntitySkill(monster, 42); // update monsterAllyIndex for clients.
				}

				// update followers for this player
				node_t* newNode = list_AddNodeLast(&stats[player]->FOLLOWERS);
				newNode->deconstructor = &defaultDeconstructor;
				Uint32* myuid = (Uint32*)malloc(sizeof(Uint32));
				newNode->element = myuid;
				*myuid = monster->getUID();

				// update client followers
				if ( player > 0 && multiplayer == SERVER )
				{
					strcpy((char*)net_packet->data, "LEAD");
					SDLNet_Write32((Uint32)monster->getUID(), &net_packet->data[4]);
					net_packet->address.host = net_clients[player - 1].host;
					net_packet->address.port = net_clients[player - 1].port;
					net_packet->len = 8;
					sendPacketSafe(net_sock, -1, net_packet, player - 1);

					serverUpdateAllyStat(player, monster->getUID(), monsterStats->LVL, monsterStats->HP, monsterStats->MAXHP, monsterStats->type);
				}

				if ( !FollowerMenu.recentEntity && player == clientnum )
				{
					FollowerMenu.recentEntity = monster;
				}
			}
		}
	}
	if ( spawnedMonster )
	{
		if ( numCreatures <= 1 )
		{
			if ( creature < KOBOLD ) //Original monster count
			{
				messagePlayer(player, language[879], language[90 + creature]);

			}
			else if ( creature >= KOBOLD ) //New monsters
			{
				messagePlayer(player, language[879], language[2000 + (creature - KOBOLD)]);
			}
			/*if ( item->beatitude >= 2 )
			{
				messagePlayer(player, language[880]);
			}*/
		}
		else
		{
			if ( creature < KOBOLD ) //Original monster count
			{
				messagePlayer(player, language[881], language[111 + creature]);

			}
			else if ( creature >= KOBOLD ) //New monsters
			{
				messagePlayer(player, language[881], language[2050 + (creature - KOBOLD)]);
			}
			//if ( item->beatitude >= 2 )
			//{
			//	messagePlayer(player, language[882]);
			//}
		}
	}
}

bool spellEffectDominate(Entity& my, spellElement_t& element, Entity& caster, Entity* parent)
{
	if ( !hit.entity )
	{
		return false;
	}

	if ( hit.entity->behavior != &actMonster )
	{
		return false;
	}

	Stat* hitstats = hit.entity->getStats();
	if ( !hitstats )
	{
		return false;
	}

	//Abort if invalid creature (boss, shopkeep, etc).
	if ( hitstats->type ==  MINOTAUR 
		|| hitstats->type == LICH 
		|| hitstats->type == DEVIL 
		|| hitstats->type == SHOPKEEPER 
		|| hitstats->type == LICH_ICE 
		|| hitstats->type == LICH_FIRE 
		|| hitstats->type == SHADOW
		|| (hitstats->type == VAMPIRE && !strncmp(hitstats->name, "Bram Kindly", 11))
		|| (hitstats->type == COCKATRICE && !strncmp(map.name, "Cockatrice Lair", 15))
		)
	{
		Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
		if ( parent )
		{
			messagePlayerColor(parent->skill[2], color, language[2429]);
		}
		return false;
	}

	playSoundEntity(hit.entity, 174, 64); //TODO: Dominate spell sound effect.

	//Make the monster a follower.
	bool dominated = forceFollower(caster, *hit.entity);

	if ( parent && dominated )
	{
		Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
		if ( parent->behavior == &actPlayer )
		{
			messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[2428], language[2427], MSG_COMBAT);
		}

		hit.entity->monsterAllyIndex = parent->skill[2];
		if ( multiplayer == SERVER )
		{
			serverUpdateEntitySkill(hit.entity, 42); // update monsterAllyIndex for clients.
		}
		// change the color of the hit entity.
		if ( hitstats->type != HUMAN && hitstats->type != AUTOMATON )
		{
			hit.entity->flags[USERFLAG2] = true;
			serverUpdateEntityFlag(hit.entity, USERFLAG2);
			int bodypart = 0;
			for ( node_t* node = (hit.entity)->children.first; node != nullptr; node = node->next )
			{
				if ( bodypart >= LIMB_HUMANOID_TORSO )
				{
					Entity* tmp = (Entity*)node->element;
					if ( tmp )
					{
						tmp->flags[USERFLAG2] = true;
						serverUpdateEntityFlag(tmp, USERFLAG2);
					}
				}
				++bodypart;
			}
		}

		caster.drainMP(hitstats->HP); //Drain additional MP equal to health of monster.
		Stat* casterStats = caster.getStats();
		if ( casterStats && casterStats->HP <= 0 )
		{
			// uh oh..
			if ( casterStats->amulet && casterStats->amulet->type == AMULET_LIFESAVING && casterStats->amulet->beatitude >= 0 )
			{
				// we're good!
				steamAchievementEntity(&caster, "BARONY_ACH_LIFE_FOR_A_LIFE");
			}
		}
	}

	spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my.sprite);
	return true;
}

void spellEffectAcid(Entity& my, spellElement_t& element, Entity* parent, int resistance)
{
	playSoundEntity(&my, 173, 128);
	if ( hit.entity )
	{
		int damage = element.damage;
		//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;

		if ( hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer )
		{
			Entity* parent = uidToEntity(my.parent);
			if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
			{
				// test for friendly fire
				if ( parent && parent->checkFriend(hit.entity) )
				{
					return;
				}
			}
			//playSoundEntity(&my, 173, 64);
			playSoundEntity(hit.entity, 249, 64);
			//playSoundEntity(hit.entity, 28, 64);

			Stat* hitstats = hit.entity->getStats();
			if ( !hitstats )
			{
				return;
			}
			bool hasamulet = false;
			if ( hitstats->amulet && hitstats->amulet->type == AMULET_POISONRESISTANCE )
			{
				resistance += 2;
				hasamulet = true;
			}
			damage /= (1 + (int)resistance);
			damage *= damagetables[hitstats->type][5];
			hit.entity->modHP(-damage);

			// write the obituary
			if ( parent )
			{
				parent->killedByMonsterObituary(hit.entity);
			}

			if ( !hasamulet )
			{
				hitstats->EFFECTS[EFF_POISONED] = true;
				hitstats->EFFECTS_TIMERS[EFF_POISONED] = 300; // 6 seconds.
				hitstats->EFFECTS_TIMERS[EFF_POISONED] /= (1 + (int)resistance);
			}
			/*hitstats->EFFECTS[EFF_SLOW] = true;
			hitstats->EFFECTS_TIMERS[EFF_SLOW] = (element->duration * (((element->mana) / static_cast<double>(element->base_mana)) * element->overload_multiplier));
			hitstats->EFFECTS_TIMERS[EFF_SLOW] /= (1 + (int)resistance);*/
			if ( hit.entity->behavior == &actPlayer )
			{
				serverUpdateEffects(hit.entity->skill[2]);
			}
			// hit messages
			if ( parent )
			{
				Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
				if ( parent->behavior == &actPlayer )
				{
					messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[2431], language[2430], MSG_COMBAT);
				}
			}

			// update enemy bar for attacker
			if ( !strcmp(hitstats->name, "") )
			{
				if ( hitstats->type < KOBOLD ) //Original monster count
				{
					updateEnemyBar(parent, hit.entity, language[90 + hitstats->type], hitstats->HP, hitstats->MAXHP);
				}
				else if ( hitstats->type >= KOBOLD ) //New monsters
				{
					updateEnemyBar(parent, hit.entity, language[2000 + (hitstats->type - KOBOLD)], hitstats->HP, hitstats->MAXHP);
				}
			}
			else
			{
				updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP);
			}

			if ( hitstats->HP <= 0 && parent )
			{
				parent->awardXP(hit.entity, true, true);
			}

			Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);

			int player = -1;
			if ( hit.entity->behavior == &actPlayer )
			{
				player = hit.entity->skill[2];
			}
			if ( player >= 0 )
			{
				messagePlayerColor(player, color, language[2432]);
			}

			if ( hitstats->HP > 0 )
			{
				// damage armor
				Item* armor = nullptr;
				int armornum = -1;
				if ( hitstats->defending && (rand() % (8 + resistance) == 0) ) // 1 in 8 to corrode shield
				{
					armornum = hitstats->pickRandomEquippedItem(&armor, true, false, true, true);
				}
				else if ( !hitstats->defending && (rand() % (4 + resistance) == 0) ) // 1 in 4 to corrode armor
				{
					armornum = hitstats->pickRandomEquippedItem(&armor, true, false, false, false);
				}
				//messagePlayer(0, "armornum: %d", armornum);
				if ( armornum != -1 && armor != nullptr )
				{
					hit.entity->degradeArmor(*hitstats, *armor, armornum);
					//messagePlayerColor(player, color, "Armor piece: %s", armor->getName());
				}
			}
		}
		else if ( hit.entity->behavior == &actDoor )
		{
			hit.entity->doorHandleDamageMagic(damage, my, parent);
		}
		spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my.sprite);
	}
	else
	{
		spawnMagicEffectParticles(my.x, my.y, my.z, my.sprite);
	}
}

void spellEffectStealWeapon(Entity& my, spellElement_t& element, Entity* parent, int resistance)
{
	if ( hit.entity )
	{
		if ( hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer )
		{
			Entity* parent = uidToEntity(my.parent);
			if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
			{
				// test for friendly fire
				if ( parent && parent->checkFriend(hit.entity) )
				{
					return;
				}
			}

			Stat* hitstats = hit.entity->getStats();
			if ( !hitstats )
			{
				return;
			}

			if ( hitstats->type == LICH || hitstats->type == LICH_FIRE || hitstats->type == LICH_ICE || hitstats->type == DEVIL )
			{
				return;
			}

			// update enemy bar for attacker
			if ( !strcmp(hitstats->name, "") )
			{
				if ( hitstats->type < KOBOLD ) //Original monster count
				{
					updateEnemyBar(parent, hit.entity, language[90 + hitstats->type], hitstats->HP, hitstats->MAXHP);
				}
				else if ( hitstats->type >= KOBOLD ) //New monsters
				{
					updateEnemyBar(parent, hit.entity, language[2000 + (hitstats->type - KOBOLD)], hitstats->HP, hitstats->MAXHP);
				}
			}
			else
			{
				updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP);
			}

			Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);

			int player = -1;
			if ( hit.entity->behavior == &actPlayer )
			{
				player = hit.entity->skill[2];
			}

			if ( hitstats->weapon )
			{
				Entity* spellEntity = createParticleSapCenter(parent, hit.entity, SPELL_STEAL_WEAPON, my.sprite, my.sprite);
				if ( spellEntity )
				{
					playSoundEntity(&my, 174, 128); // succeeded spell sound
					spellEntity->skill[7] = 1; // found weapon

					// store weapon data
					spellEntity->skill[10] = hitstats->weapon->type;
					if ( itemCategory(hitstats->weapon) == SPELLBOOK )
					{
						spellEntity->skill[11] = DECREPIT;
					}
					else
					{
						spellEntity->skill[11] = hitstats->weapon->status;
					}
					spellEntity->skill[12] = hitstats->weapon->beatitude;
					spellEntity->skill[13] = hitstats->weapon->count;
					spellEntity->skill[14] = hitstats->weapon->appearance;
					spellEntity->skill[15] = hitstats->weapon->identified;
					spellEntity->itemOriginalOwner = hit.entity->getUID();
					// hit messages
					if ( player >= 0 )
					{
						color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
						messagePlayerColor(player, color, language[2435], hitstats->weapon->getName());
					}

					if ( parent )
					{
						color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
						if ( parent->behavior == &actPlayer )
						{
							messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[2434], language[2433], MSG_COMBAT);
						}
					}

					if ( hit.entity->behavior == &actMonster && itemCategory(hitstats->weapon) != SPELLBOOK )
					{
						free(hitstats->weapon);
						hitstats->weapon = nullptr;
					}
					else if ( hit.entity->behavior == &actPlayer )
					{
						// player.
						Item* weapon = hitstats->weapon;
						Item** slot = itemSlot(hitstats, weapon);
						if ( slot )
						{
							*slot = nullptr;
						}
						if ( weapon->node )
						{
							list_RemoveNode(weapon->node);
						}
						else
						{
							free(weapon);
						}
						if ( player > 0 && multiplayer == SERVER )
						{
							strcpy((char*)net_packet->data, "STLA");
							net_packet->data[4] = 5; // steal weapon index in STLA netcode.
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 5;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}
					}
				}
			}
			else
			{
				playSoundEntity(&my, 163, 128); // failed spell sound
				// hit messages
				if ( player >= 0 )
				{
					color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
					messagePlayerColor(player, color, language[2438]);
				}

				if ( parent )
				{
					color = SDL_MapRGB(mainsurface->format, 255, 255, 255);
					if ( parent->behavior == &actPlayer )
					{
						messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[2437], language[2436], MSG_COMBAT);
					}
				}
			}
		}
		spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my.sprite);
	}
	else
	{
		spawnMagicEffectParticles(my.x, my.y, my.z, my.sprite);
	}
	return;
}

void spellEffectDrainSoul(Entity& my, spellElement_t& element, Entity* parent, int resistance)
{
	if ( hit.entity )
	{
		if ( hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer )
		{
			Entity* parent = uidToEntity(my.parent);
			if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
			{
				// test for friendly fire
				if ( parent && parent->checkFriend(hit.entity) )
				{
					return;
				}
			}

			Stat* hitstats = hit.entity->getStats();
			if ( !hitstats )
			{
				return;
			}

			int damage = element.damage;
			//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
			damage /= (1 + (int)resistance);
			damage *= damagetables[hitstats->type][5];

			if ( parent )
			{
				Stat* casterStats = parent->getStats();
				if ( casterStats && casterStats->type == LICH_ICE )
				{
					damage *= 2;
				}
			}

			int damageHP = hitstats->HP;
			int damageMP = hitstats->MP;
			hit.entity->modHP(-damage);
			hit.entity->drainMP(damage);

			damageHP -= hitstats->HP;
			damageMP -= hitstats->MP;

			// write the obituary
			if ( parent )
			{
				parent->killedByMonsterObituary(hit.entity);
			}

			// update enemy bar for attacker
			if ( !strcmp(hitstats->name, "") )
			{
				if ( hitstats->type < KOBOLD ) //Original monster count
				{
					updateEnemyBar(parent, hit.entity, language[90 + hitstats->type], hitstats->HP, hitstats->MAXHP);
				}
				else if ( hitstats->type >= KOBOLD ) //New monsters
				{
					updateEnemyBar(parent, hit.entity, language[2000 + (hitstats->type - KOBOLD)], hitstats->HP, hitstats->MAXHP);
				}
			}
			else
			{
				updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP);
			}

			Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);

			int player = -1;
			if ( hit.entity->behavior == &actPlayer )
			{
				player = hit.entity->skill[2];
			}

			if ( hitstats->HP <= 0 && parent )
			{
				parent->awardXP(hit.entity, true, true);
			}

			if ( damageHP > 0 && parent )
			{
				Entity* spellEntity = createParticleSapCenter(parent, hit.entity, SPELL_DRAIN_SOUL, my.sprite, my.sprite);
				if ( spellEntity )
				{
					playSoundEntity(&my, 167, 128); // succeeded spell sound
					playSoundEntity(&my, 28, 128); // damage
					spellEntity->skill[7] = damageHP; // damage taken to HP
					spellEntity->skill[8] = damageMP; // damage taken tp MP

					// hit messages
					if ( player >= 0 )
					{
						color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
						messagePlayerColor(player, color, language[2441]);
					}

					if ( parent )
					{
						color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
						if ( parent->behavior == &actPlayer )
						{
							messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[2440], language[2439], MSG_COMBAT);
						}
					}
				}
			}
			else
			{
				playSoundEntity(&my, 163, 128); // failed spell sound
				// hit messages
				if ( player >= 0 )
				{
					color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
					messagePlayerColor(player, color, language[2444]);
				}

				if ( parent )
				{
					color = SDL_MapRGB(mainsurface->format, 255, 255, 255);
					if ( parent->behavior == &actPlayer )
					{
						messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[2443], language[2442], MSG_COMBAT);
					}
				}
			}
		}
		spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my.sprite);
	}
	else
	{
		spawnMagicEffectParticles(my.x, my.y, my.z, my.sprite);
	}
	return;
}

spell_t* spellEffectVampiricAura(Entity* caster, spell_t* spell, int extramagic_to_use)
{
	//Also refactor the duration determining code.
	node_t* node = spell->elements.first;
	if ( !node )
	{
		return nullptr;
	}
	spellElement_t* element = static_cast<spellElement_t*>(node->element);
	if ( !element )
	{
		return nullptr;
	}
	Stat* myStats = caster->getStats();
	if ( !myStats )
	{
		return nullptr;
	}

	bool newbie = caster->isSpellcasterBeginner();

	int duration = element->duration; // duration in ticks.
	duration += (((element->mana + extramagic_to_use) - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->duration;
	node_t* spellnode = list_AddNodeLast(&myStats->magic_effects);
	spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
	spell_t* channeled_spell = (spell_t*)(spellnode->element);
	channeled_spell->magic_effects_node = spellnode;
	spellnode->size = sizeof(spell_t);
	((spell_t*)spellnode->element)->caster = caster->getUID();
	spellnode->deconstructor = &spellDeconstructor;
	if ( newbie )
	{
		//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
		int chance = rand() % 10;
		// spellcasting power is 0 to 100, based on spellcasting and intelligence.
		int spellcastingPower = std::min(std::max(0, myStats->PROFICIENCIES[PRO_SPELLCASTING] + statGetINT(myStats)), 100);
		if ( chance >= spellcastingPower / 10 )
		{
			duration -= rand() % (1000 / (spellcastingPower + 1)); // reduce the duration by 0-20 seconds
		}
		if ( duration < 50 )
		{
			duration = 50;    //Range checking.
		}
	}
	duration /= getCostOfSpell((spell_t*)spellnode->element);
	channeled_spell->channel_duration = duration; //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
	caster->setEffect(EFF_VAMPIRICAURA, true, duration, true);
	for ( int i = 0; i < numplayers; ++i )
	{
		if ( caster == players[i]->entity )
		{
			serverUpdateEffects(i);
			Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
			messagePlayerColor(i, color, language[2477]);
		}
	}

	playSoundEntity(caster, 167, 128);
	createParticleDropRising(caster, 600, 0.7);
	serverSpawnMiscParticles(caster, PARTICLE_EFFECT_VAMPIRIC_AURA, 600);
	return channeled_spell;
}

void spellEffectCharmMonster(Entity& my, spellElement_t& element, Entity* parent, int resistance, bool magicstaff)
{
	if ( hit.entity )
	{
		if ( hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer )
		{
			if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
			{
				// test for friendly fire
				if ( parent && parent->checkFriend(hit.entity) )
				{
					return;
				}
			}

			Stat* hitstats = hit.entity->getStats();
			if ( !hitstats )
			{
				return;
			}

			Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);

			int player = -1;
			if ( hit.entity->behavior == &actPlayer )
			{
				player = hit.entity->skill[2];
			}

			if ( !parent )
			{
				return;
			}
			Stat* casterStats = parent->getStats();
			if ( !casterStats )
			{
				return;
			}
					
			int difficulty = 0;
			switch ( hitstats->type )
			{
				case HUMAN:
				case RAT:
				case SLIME:
				case SPIDER:
				case SKELETON:
				case SCORPION:
					difficulty = 0;
					break;
				case GOBLIN:
				case TROLL:
				case GHOUL:
				case GNOME:
				case SCARAB:
				case AUTOMATON:
				case SUCCUBUS:
					difficulty = 1;
					break;
				case CREATURE_IMP:
				case DEMON:
				case KOBOLD:
				case INCUBUS:
				case INSECTOID:
				case GOATMAN:
					difficulty = 2;
					break;
				case CRYSTALGOLEM:
				case VAMPIRE:
					difficulty = 5;
					break;
				case COCKATRICE:
				case SHADOW:
				case SHOPKEEPER:
				case LICH:
				case DEVIL:
				case LICH_ICE:
				case LICH_FIRE:
				case MINOTAUR:
					difficulty = 666;
					break;
			}

			int chance = 80;
			// CHANCE CALCULATION
			if ( hitstats->EFFECTS[EFF_CONFUSED] || hitstats->EFFECTS[EFF_DRUNK] )
			{
				difficulty -= 1;
			}
			if ( strcmp(hitstats->name, "") && !monsterNameIsGeneric(*hitstats) )
			{
				difficulty += 1; // minibosses +1 difficulty.
			}
			chance -= difficulty * 30;
			chance += ((parent->getCHR() + casterStats->PROFICIENCIES[PRO_LEADERSHIP]) / 20) * 10;
			if ( !magicstaff )
			{
				chance += ((parent->getINT() + casterStats->PROFICIENCIES[PRO_MAGIC]) / 20) * 10;
			}
			if ( hit.entity->monsterState == MONSTER_STATE_WAIT )
			{
				chance += 10;
			}
			chance /= (1 + resistance);
			// END CHANCE CALCULATION

			if ( (hitstats->type == VAMPIRE && !strncmp(hitstats->name, "Bram Kindly", 11))
				|| (hitstats->type == COCKATRICE && !strncmp(map.name, "Cockatrice Lair", 15))
				)
			{
				chance = 0;
			}

			messagePlayer(0, "Your charm chance: %d", chance);

			if ( chance <= 0 )
			{
				// no effect.
				playSoundEntity(hit.entity, 163, 64); // FailedSpell1V1.ogg
				if ( parent->behavior == &actPlayer )
				{
					messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[2905], language[2906], MSG_COMBAT);
				}
				if ( player >= 0 )
				{
					color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
					//messagePlayerColor(player, color, language[2441]);
				}
			}
			else if ( rand() % 100 < chance && hitstats->leader_uid == 0 && player < 0 )
			{
				// followed or fully charmed. (players not affected here.)
				forceFollower(*parent, *hit.entity);
				createParticleCharmMonster(hit.entity);
				playSoundEntity(hit.entity, 174, 64); // WeirdSpell.ogg
				if ( parent->behavior == &actPlayer )
				{
					messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[3137], language[3138], MSG_COMBAT);
				}
				if ( player >= 0 )
				{
					color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
					//messagePlayerColor(player, color, language[2441]);
				}

				hit.entity->monsterAllyIndex = parent->skill[2];
				if ( multiplayer == SERVER )
				{
					serverUpdateEntitySkill(hit.entity, 42); // update monsterAllyIndex for clients.
				}
				// change the color of the hit entity.
				if ( hitstats->type != HUMAN && hitstats->type != AUTOMATON )
				{
					hit.entity->flags[USERFLAG2] = true;
					serverUpdateEntityFlag(hit.entity, USERFLAG2);
					int bodypart = 0;
					for ( node_t* node = (hit.entity)->children.first; node != nullptr; node = node->next )
					{
						if ( bodypart >= LIMB_HUMANOID_TORSO )
						{
							Entity* tmp = (Entity*)node->element;
							if ( tmp )
							{
								tmp->flags[USERFLAG2] = true;
								serverUpdateEntityFlag(tmp, USERFLAG2);
							}
						}
						++bodypart;
					}
				}
			}
			else
			{
				// had a chance or currently in service of another monster, failed to completely charm. loses will to attack.
				int duration = element.duration;
				duration /= (1 + resistance);
				if ( hit.entity->setEffect(EFF_PACIFY, true, duration, true) )
				{
					playSoundEntity(hit.entity, 168, 128); // Healing.ogg
					if ( parent->behavior == &actPlayer )
					{
						messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[3139], language[3140], MSG_COMBAT);
					}
					if ( player >= 0 )
					{
						color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
						//messagePlayerColor(player, color, language[2441]);
					}
					// update enemy bar for attacker
					if ( !strcmp(hitstats->name, "") )
					{
						if ( hitstats->type < KOBOLD ) //Original monster count
						{
							updateEnemyBar(parent, hit.entity, language[90 + hitstats->type], hitstats->HP, hitstats->MAXHP);
						}
						else if ( hitstats->type >= KOBOLD ) //New monsters
						{
							updateEnemyBar(parent, hit.entity, language[2000 + (hitstats->type - KOBOLD)], hitstats->HP, hitstats->MAXHP);
						}
					}
					else
					{
						updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP);
					}
				}
				else
				{
					playSoundEntity(hit.entity, 163, 64); // FailedSpell1V1.ogg
					if ( parent->behavior == &actPlayer )
					{
						messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[2905], language[2906], MSG_COMBAT);
					}
					if ( player >= 0 )
					{
						color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
						//messagePlayerColor(player, color, language[2441]);
					}
				}
			}
		}
		spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my.sprite);
	}
	else
	{
		spawnMagicEffectParticles(my.x, my.y, my.z, my.sprite);
	}
	return;
}