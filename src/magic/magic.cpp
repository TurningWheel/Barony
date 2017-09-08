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

	//Abort if invalid creature (boss, shopkeep, etc).
	Stat* hitstats = hit.entity->getStats();
	if ( !hitstats )
	{
		return false;
	}

	if ( hitstats->type ==  MINOTAUR || hitstats->type == LICH || hitstats->type == DEVIL || hitstats->type == SHOPKEEPER || hitstats->type == LICH_ICE || hitstats->type == LICH_FIRE )
	{
		Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
		messagePlayerColor(parent->skill[2], color, language[2429]);
		return false;
	}

	playSoundEntity(hit.entity, 174, 64); //TODO: Dominate spell sound effect.

	//Make the monster a follower.
	bool dominated = forceFollower(caster, *hit.entity);

	//TODO: Update lang entries.
	if ( parent && dominated )
	{
		Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
		if ( parent->behavior == &actPlayer )
		{
			if ( strcmp(hitstats->name, "") )
			{
				messagePlayerColor(parent->skill[2], color, language[2427], hitstats->name);
			}
			else
			{
				if ( hitstats->type < KOBOLD ) //Original monster count
				{
					messagePlayerColor(parent->skill[2], color, language[2428], language[90 + hitstats->type]);
				}
				else if ( hitstats->type >= KOBOLD ) //New monsters
				{
					messagePlayerColor(parent->skill[2], color, language[2428], language[2000 + (hitstats->type - KOBOLD)]);
				}
			}
		}

		caster.drainMP(hitstats->HP); //Drain additional MP equal to health of monster.
	}

	spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my.sprite);
	my.removeLightField();
	list_RemoveNode(my.mynode);
	return true;
}

void spellEffectAcid(Entity& my, spellElement_t& element, Entity* parent, int resistance)
{
	if ( !hit.entity )
	{
		return;
	}

	int player = -1;
	if ( hit.entity->behavior == &actPlayer )
	{
		player = hit.entity->skill[2];
	}

	playSoundEntity(&my, 173, 128);
	if ( hit.entity )
	{
		int damage = element.damage;
		//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
		damage /= (1 + (int)resistance);

		if ( hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer )
		{
			Entity* parent = uidToEntity(my.parent);
			if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
			{
				// test for friendly fire
				if ( parent && parent->checkFriend(hit.entity) )
				{
					my.removeLightField();
					list_RemoveNode(my.mynode);
					return;
				}
			}
			playSoundEntity(&my, 173, 64);
			playSoundEntity(hit.entity, 249, 64);
			//playSoundEntity(hit.entity, 28, 64);

			Stat* hitstats = hit.entity->getStats();
			if ( !hitstats )
			{
				return;
			}

			damage *= damagetables[hitstats->type][5];
			hit.entity->modHP(-damage);

			// write the obituary
			if ( parent )
			{
				parent->killedByMonsterObituary(hit.entity);
			}

			hitstats->EFFECTS[EFF_POISONED] = true;
			hitstats->EFFECTS_TIMERS[EFF_POISONED] = (element.duration * (((element.mana) / static_cast<double>(element.base_mana)) * element.overload_multiplier));
			hitstats->EFFECTS_TIMERS[EFF_POISONED] /= (1 + (int)resistance);
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
					if ( strcmp(hitstats->name, "") )
					{
						messagePlayerColor(parent->skill[2], color, language[2430], hitstats->name);
					}
					else
					{
						if ( hitstats->type < KOBOLD ) //Original monster count
						{
							messagePlayerColor(parent->skill[2], color, language[2431], language[90 + hitstats->type]);
						}
						else if ( hitstats->type >= KOBOLD ) //New monsters
						{
							messagePlayerColor(parent->skill[2], color, language[2431], language[2000 + (hitstats->type - KOBOLD)]);
						}
					}
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
			if ( player >= 0 )
			{
				messagePlayerColor(player, color, language[2432]);
			}

			if ( hitstats->HP > 0 )
			{
				// damage armor
				Item* armor = nullptr;
				int armornum = -1;
				if ( hitstats->defending && (rand() % (6 + resistance) == 0) ) // 1 in 6 to corrode shield
				{
					armornum = hitstats->pickRandomEquippedItem(&armor, true, false, true, true);
				}
				else if ( !hitstats->defending && (rand() % (3 + resistance) == 0) ) // 1 in 3 to corrode armor
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
	}

	spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my.sprite);
	my.removeLightField();
	list_RemoveNode(my.mynode);
	return;
}