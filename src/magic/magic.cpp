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
#include "../collision.hpp"
#include "../classdescriptions.hpp"
#include "../scores.hpp"

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
	list_FreeAll(&spell_revertForm.elements);
	list_FreeAll(&spell_ratForm.elements);
	list_FreeAll(&spell_spiderForm.elements);
	list_FreeAll(&spell_trollForm.elements);
	list_FreeAll(&spell_impForm.elements);
	list_FreeAll(&spell_sprayWeb.elements);
	list_FreeAll(&spell_poison.elements);
	list_FreeAll(&spell_speed.elements);
	list_FreeAll(&spell_fear.elements);
	list_FreeAll(&spell_strike.elements);
	list_FreeAll(&spell_detectFood.elements);
	list_FreeAll(&spell_weakness.elements);
	list_FreeAll(&spell_amplifyMagic.elements);
	list_FreeAll(&spell_shadowTag.elements);
	list_FreeAll(&spell_telePull.elements);
	list_FreeAll(&spell_demonIllusion.elements);
	list_FreeAll(&spell_trollsBlood.elements);
	list_FreeAll(&spell_salvageItem.elements);
	list_FreeAll(&spell_flutter.elements);
	list_FreeAll(&spell_dash.elements);
	list_FreeAll(&spell_polymorph.elements);
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

void spell_detectFoodEffectOnMap(int player)
{
	if ( players[player] == nullptr || players[player]->entity == nullptr )
	{
		return;
	}

	if ( multiplayer == SERVER && player > 0 )
	{
		//Tell the client to map the food.
		strcpy((char*)net_packet->data, "MFOD");
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 4;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
		return;
	}

	mapFoodOnLevel(player);
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
				monster->flags[USERFLAG2] = true;
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
					strcpy((char*)(&net_packet->data[8]), monsterStats->name);
					net_packet->data[8 + strlen(monsterStats->name)] = 0;
					net_packet->address.host = net_clients[player - 1].host;
					net_packet->address.port = net_clients[player - 1].port;
					net_packet->len = 8 + strlen(monsterStats->name) + 1;
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

		hit.entity->flags[USERFLAG2] = true;
		serverUpdateEntityFlag(hit.entity, USERFLAG2);
		if ( monsterChangesColorWhenAlly(hitstats) )
		{
			int bodypart = 0;
			for ( node_t* node = (hit.entity)->children.first; node != nullptr; node = node->next )
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
		damage += damage * ((my.actmagicSpellbookBonus / 100.f) + getBonusFromCasterOfSpellElement(parent, &element));
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
			if ( (hitstats->amulet && hitstats->amulet->type == AMULET_POISONRESISTANCE) || hitstats->type == INSECTOID )
			{
				resistance += 2;
				hasamulet = true;
			}
			int oldHP = hitstats->HP;
			damage /= (1 + (int)resistance);
			damage *= hit.entity->getDamageTableMultiplier(*hitstats, DAMAGE_TABLE_MAGIC);
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

			if ( oldHP > 0 && hitstats->HP <= 0 && parent )
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

void spellEffectPoison(Entity& my, spellElement_t& element, Entity* parent, int resistance)
{
	playSoundEntity(&my, 173, 128);
	if ( hit.entity )
	{
		int damage = element.damage;
		damage += damage * ((my.actmagicSpellbookBonus / 100.f) + getBonusFromCasterOfSpellElement(parent, &element));
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
			playSoundEntity(hit.entity, 249, 64);

			Stat* hitstats = hit.entity->getStats();
			if ( !hitstats )
			{
				return;
			}
			bool hasamulet = false;
			if ( (hitstats->amulet && hitstats->amulet->type == AMULET_POISONRESISTANCE) || hitstats->type == INSECTOID )
			{
				resistance += 2;
				hasamulet = true;
			}
			damage /= (1 + (int)resistance);
			damage *= hit.entity->getDamageTableMultiplier(*hitstats, DAMAGE_TABLE_MAGIC);
			hit.entity->modHP(-damage);

			// write the obituary
			if ( parent )
			{
				parent->killedByMonsterObituary(hit.entity);
			}

			if ( !hasamulet )
			{
				if ( my.actmagicCastByMagicstaff == 1 )
				{
					hit.entity->setEffect(EFF_POISONED, true, 320, true); // 6 seconds.
				}
				else
				{
					hit.entity->setEffect(EFF_POISONED, true, std::max(200, 350 - hit.entity->getCON() * 5), true); // 4-7 seconds.
				}
				hitstats->poisonKiller = my.parent;
			}

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
					messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[3427], language[3426], MSG_COMBAT);
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
				messagePlayerColor(player, color, language[3428]);
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

bool spellEffectFear(Entity* my, spellElement_t& element, Entity* forceParent, Entity* target, int resistance)
{
	if ( !target )
	{
		//spawnMagicEffectParticles(my.x, my.y, my.z, 863);
		return false;
	}

	if ( target->behavior == &actMonster || target->behavior == &actPlayer )
	{
		Entity* parent = forceParent;
		if ( my && !parent )
		{
			parent = uidToEntity(my->parent);
		}
		if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
		{
			// test for friendly fire
			if ( parent && parent->checkFriend(target) )
			{
				return false;
			}
		}

		Stat* hitstats = target->getStats();
		if ( !hitstats )
		{
			return false;
		}

		int duration = 400; // 8 seconds
		duration = std::max(150, duration - TICKS_PER_SECOND * (hitstats->CON / 5)); // 3-8 seconds, depending on CON.
		duration /= (1 + resistance);
		if ( target->setEffect(EFF_FEAR, true, duration, true) )
		{
			playSoundEntity(target, 168, 128); // Healing.ogg
			Uint32 color = 0;
			if ( parent )
			{
				// update enemy bar for attacker
				if ( !strcmp(hitstats->name, "") )
				{
					if ( hitstats->type < KOBOLD ) //Original monster count
					{
						updateEnemyBar(parent, target, language[90 + hitstats->type], hitstats->HP, hitstats->MAXHP);
					}
					else if ( hitstats->type >= KOBOLD ) //New monsters
					{
						updateEnemyBar(parent, target, language[2000 + (hitstats->type - KOBOLD)], hitstats->HP, hitstats->MAXHP);
					}
				}
				else
				{
					updateEnemyBar(parent, target, hitstats->name, hitstats->HP, hitstats->MAXHP);
				}
				target->monsterAcquireAttackTarget(*parent, MONSTER_STATE_PATH);
				target->monsterFearfulOfUid = parent->getUID();

				if ( parent->behavior == &actPlayer && parent->getStats() && parent->getStats()->type == TROLL )
				{
					serverUpdatePlayerGameplayStats(parent->skill[2], STATISTICS_FORUM_TROLL, AchievementObserver::FORUM_TROLL_FEAR);
				}
			}
		}
		else
		{
			// no effect.
			if ( parent )
			{
				Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
				if ( parent->behavior == &actPlayer )
				{
					messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[2905], language[2906], MSG_COMBAT);
				}
			}
			return false;
		}

		// hit messages
		if ( parent )
		{
			Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
			if ( parent->behavior == &actPlayer )
			{
				messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[3434], language[3435], MSG_COMBAT);
			}
		}

		Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);

		int player = -1;
		if ( target->behavior == &actPlayer )
		{
			player = target->skill[2];
		}
		if ( player >= 0 )
		{
			messagePlayerColor(player, color, language[3436]);
		}
	}
	spawnMagicEffectParticles(target->x, target->y, target->z, 863);
	return true;
}

void spellEffectSprayWeb(Entity& my, spellElement_t& element, Entity* parent, int resistance)
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

			bool spawnParticles = true;
			if ( hitstats->EFFECTS[EFF_WEBBED] )
			{
				spawnParticles = false;
			}
			int previousDuration = hitstats->EFFECTS_TIMERS[EFF_WEBBED];
			int duration = 400;
			duration /= (1 + resistance);
			if ( hit.entity->setEffect(EFF_WEBBED, true, 400, true) ) // 8 seconds.
			{
				if ( duration - previousDuration > 10 )
				{
					playSoundEntity(hit.entity, 396 + rand() % 3, 64); // play sound only if not recently webbed. (triple shot makes many noise)
				}
				hit.entity->creatureWebbedSlowCount = std::min(3, hit.entity->creatureWebbedSlowCount + 1);
				if ( hit.entity->behavior == &actPlayer )
				{
					serverUpdateEntitySkill(hit.entity, 52); // update player.
				}
				if ( spawnParticles )
				{
					createParticleAestheticOrbit(hit.entity, 863, 400, PARTICLE_EFFECT_SPELL_WEB_ORBIT);
					serverSpawnMiscParticles(hit.entity, PARTICLE_EFFECT_SPELL_WEB_ORBIT, 863);
				}
			}
			else
			{
				// no effect.
				if ( parent )
				{
					Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
					if ( parent->behavior == &actPlayer )
					{
						messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[2905], language[2906], MSG_COMBAT);
					}
				}
				return;
			}

			// hit messages
			if ( parent )
			{
				Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
				if ( parent->behavior == &actPlayer )
				{
					if ( duration - previousDuration > 10 ) // message if not recently webbed
					{
						messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[3430], language[3429], MSG_COMBAT);
					}
				}
			}

			Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);

			int player = -1;
			if ( hit.entity->behavior == &actPlayer )
			{
				player = hit.entity->skill[2];
			}
			if ( player >= 0 )
			{
				if ( duration - previousDuration > 10 ) // message if not recently webbed
				{
					messagePlayerColor(player, color, language[3431]);
				}
			}
		}
		spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, 863);
	}
	else
	{
		spawnMagicEffectParticles(my.x, my.y, my.z, 863);
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

			if ( hit.entity->behavior == &actMonster 
				&& (hit.entity->monsterAllySummonRank != 0 
					|| (hitstats->type == INCUBUS && !strncmp(hitstats->name, "inner demon", strlen("inner demon"))))
				)
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
							messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[2434], language[2433], MSG_STEAL_WEAPON);
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
			damage += damage * ((my.actmagicSpellbookBonus / 100.f) + getBonusFromCasterOfSpellElement(parent, &element));
			//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;
			damage /= (1 + (int)resistance);
			damage *= hit.entity->getDamageTableMultiplier(*hitstats, DAMAGE_TABLE_MAGIC);

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
			if ( damage > hitstats->MP )
			{
				damage = hitstats->MP;
			}
			if ( parent && parent->behavior == &actPlayer )
			{
				damage /= 4; // reduced mana steal
			}
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
	//if ( newbie )
	//{
	//	//This guy's a newbie. There's a chance they've screwed up and negatively impacted the efficiency of the spell.
	//	int chance = rand() % 10;
	//	// spellcasting power is 0 to 100, based on spellcasting and intelligence.
	//	int spellcastingPower = std::min(std::max(0, myStats->PROFICIENCIES[PRO_SPELLCASTING] + statGetINT(myStats, caster)), 100);
	//	if ( chance >= spellcastingPower / 10 )
	//	{
	//		duration -= rand() % (1000 / (spellcastingPower + 1)); // reduce the duration by 0-20 seconds
	//	}
	//	if ( duration < 50 )
	//	{
	//		duration = 50;    //Range checking.
	//	}
	//}
	duration /= getCostOfSpell((spell_t*)spellnode->element);
	channeled_spell->channel_duration = duration; //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
	caster->setEffect(EFF_VAMPIRICAURA, true, duration, true);
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( players[i] && caster && (caster == players[i]->entity) )
		{
			serverUpdateEffects(i);
			Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
			messagePlayerColor(i, color, language[2477]);
			playSoundPlayer(i, 403, 32);
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

			int difficulty = 0;
			switch ( hitstats->type )
			{
				case HUMAN:
				case RAT:
				case SLIME:
				case SPIDER:
				case SKELETON:
				case SCORPION:
				case SHOPKEEPER:
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
				case LICH:
				case DEVIL:
				case LICH_ICE:
				case LICH_FIRE:
				case MINOTAUR:
					difficulty = 666;
					break;
			}

			int chance = 80;
			bool allowStealFollowers = false;
			Stat* casterStats = nullptr;
			int currentCharmedFollowerCount = 0;

			/************** CHANCE CALCULATION ***********/
			if ( hitstats->EFFECTS[EFF_CONFUSED] || hitstats->EFFECTS[EFF_DRUNK] || player >= 0 )
			{
				difficulty -= 1; // players and confused/drunk monsters have lower resistance.
			}
			if ( strcmp(hitstats->name, "") && !monsterNameIsGeneric(*hitstats) )
			{
				difficulty += 1; // minibosses +1 difficulty.
			}
			chance -= difficulty * 30;
			if ( parent )
			{
				casterStats = parent->getStats();
				if ( casterStats )
				{
					if ( magicstaff )
					{
						chance += ((parent->getCHR() + casterStats->PROFICIENCIES[PRO_LEADERSHIP]) / 20) * 10;
					}
					else
					{
						chance += ((parent->getCHR() + casterStats->PROFICIENCIES[PRO_LEADERSHIP]) / 20) * 5;
						chance += (parent->getINT() * 2);
					}

					if ( parent->behavior == &actMonster )
					{
						allowStealFollowers = true;
						if ( casterStats->type == INCUBUS || casterStats->type == SUCCUBUS )
						{
							if ( hitstats->type == DEMON || hitstats->type == INCUBUS
								|| hitstats->type == SUCCUBUS || hitstats->type == CREATURE_IMP
								|| hitstats->type == GOATMAN )
							{
								chance = 100; // bonus for demons.
							}
							else if ( difficulty <= 2 )
							{
								chance = 80; // special base chance for monsters.
							}
						}
						else if ( difficulty <= 2 )
						{
							chance = 60; // special base chance for monsters.
						}
					}
					else if ( parent->behavior == &actPlayer )
					{
						// search followers for charmed.
						for ( node_t* node = casterStats->FOLLOWERS.first; node != NULL; node = node->next )
						{
							Uint32* c = (Uint32*)node->element;
							Entity* follower = nullptr;
							if ( c )
							{
								follower = uidToEntity(*c);
							}
							if ( follower )
							{
								if ( Stat* followerStats = follower->getStats() )
								{
									if ( followerStats->monsterIsCharmed == 1 )
									{
										++currentCharmedFollowerCount;
									}
								}
							}
						}
					}
				}
			}
			if ( hit.entity->monsterState == MONSTER_STATE_WAIT )
			{
				chance += 10;
			}
			chance /= (1 + resistance);
			/************** END CHANCE CALCULATION ***********/

			// special cases:
			if ( (hitstats->type == VAMPIRE && !strncmp(hitstats->name, "Bram Kindly", 11))
				|| (hitstats->type == COCKATRICE && !strncmp(map.name, "Cockatrice Lair", 15))
				)
			{
				chance = 0;
			}
			else if ( hit.entity->behavior == &actMonster 
				&& (hit.entity->monsterAllySummonRank != 0
					|| (hitstats->type == INCUBUS && !strncmp(hitstats->name, "inner demon", strlen("inner demon")))) 
				)
			{
				chance = 0; // not allowed to control summons
			}

			if ( parent && hit.entity == parent )
			{
				// caster hit themselves somehow... get pacified.
				int duration = element.duration;
				duration /= (1 + resistance);
				if ( hit.entity->setEffect(EFF_PACIFY, true, duration, true) )
				{
					playSoundEntity(hit.entity, 168, 128); // Healing.ogg
					if ( player >= 0 )
					{
						color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
						messagePlayerColor(player, color, language[3144]);
					}
					if ( parent )
					{
						if ( parent->behavior == &actPlayer )
						{
							messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[3139], language[3140], MSG_COMBAT);
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
				}
			}
			else if ( chance <= 0 )
			{
				// no effect.
				playSoundEntity(hit.entity, 163, 64); // FailedSpell1V1.ogg
				if ( parent && parent->behavior == &actPlayer )
				{
					messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[2905], language[2906], MSG_COMBAT);
					steamAchievementClient(parent->skill[2], "BARONY_ACH_OFF_LIMITS");
				}
				if ( player >= 0 )
				{
					color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
					messagePlayerColor(player, color, language[3141]);
				}
			}
			else if ( parent && rand() % 100 < chance
				&& (hitstats->leader_uid == 0 || (allowStealFollowers && hitstats->leader_uid != parent->getUID()) )
				&& player < 0 
				&& hitstats->type != SHOPKEEPER
				&& currentCharmedFollowerCount == 0
				)
			{
				// fully charmed. (players not affected here.)
				// does not affect shopkeepers
				// succubus/incubus can steal followers from others, checking to see if they don't already follow them.
				Entity* whoToFollow = parent;
				if ( parent->behavior == &actMonster && parent->monsterAllyGetPlayerLeader() )
				{
					whoToFollow = parent->monsterAllyGetPlayerLeader();
				}

				if ( forceFollower(*whoToFollow, *hit.entity) )
				{
					serverSpawnMiscParticles(hit.entity, PARTICLE_EFFECT_CHARM_MONSTER, 0);
					createParticleCharmMonster(hit.entity);
					playSoundEntity(hit.entity, 174, 64); // WeirdSpell.ogg
					if ( whoToFollow->behavior == &actPlayer )
					{
						whoToFollow->increaseSkill(PRO_LEADERSHIP);
						messagePlayerMonsterEvent(whoToFollow->skill[2], color, *hitstats, language[3137], language[3138], MSG_COMBAT);
						hit.entity->monsterAllyIndex = whoToFollow->skill[2];
						if ( multiplayer == SERVER )
						{
							serverUpdateEntitySkill(hit.entity, 42); // update monsterAllyIndex for clients.
						}
						if ( hit.entity->monsterTarget == whoToFollow->getUID() )
						{
							hit.entity->monsterReleaseAttackTarget();
						}
					}

					// change the color of the hit entity.
					hit.entity->flags[USERFLAG2] = true;
					serverUpdateEntityFlag(hit.entity, USERFLAG2);
					hitstats->monsterIsCharmed = 1;
					if ( monsterChangesColorWhenAlly(hitstats) )
					{
						int bodypart = 0;
						for ( node_t* node = (hit.entity)->children.first; node != nullptr; node = node->next )
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
					if ( whoToFollow->behavior == &actMonster )
					{
						if ( whoToFollow->monsterTarget == hit.entity->getUID() )
						{
							whoToFollow->monsterReleaseAttackTarget(); // monsters stop attacking their new friend.
						}

						// handle players losing their allies.
						if ( hit.entity->monsterAllyIndex != -1 )
						{
							hit.entity->monsterAllyIndex = -1;
							if ( multiplayer == SERVER )
							{
								serverUpdateEntitySkill(hit.entity, 42); // update monsterAllyIndex for clients.
							}
						}
					}
				}
			}
			else
			{
				// had a chance, or currently in service of another monster, or a player, or spell no parent, failed to completely charm. 
				// loses will to attack.
				int duration = element.duration;
				duration /= (1 + resistance);
				if ( hitstats->type == SHOPKEEPER )
				{
					duration = 100;
				}
				if ( hit.entity->setEffect(EFF_PACIFY, true, duration, true) )
				{
					playSoundEntity(hit.entity, 168, 128); // Healing.ogg
					if ( player >= 0 )
					{
						color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
						messagePlayerColor(player, color, language[3144]);
					}
					if ( parent )
					{
						if ( parent->behavior == &actPlayer )
						{
							messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[3139], language[3140], MSG_COMBAT);
							if ( currentCharmedFollowerCount > 0 )
							{
								messagePlayer(parent->skill[2], language[3327]);
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
					}
					if ( hitstats->type == SHOPKEEPER && player >= 0 )
					{
						// reverses shop keeper grudges.
						swornenemies[SHOPKEEPER][HUMAN] = false;
						swornenemies[SHOPKEEPER][AUTOMATON] = false;
						monsterally[SHOPKEEPER][HUMAN] = true;
						monsterally[SHOPKEEPER][AUTOMATON] = true;
						hit.entity->monsterReleaseAttackTarget();
					}
				}
				else
				{
					// resists the charm.
					playSoundEntity(hit.entity, 163, 64); // FailedSpell1V1.ogg
					if ( parent && parent->behavior == &actPlayer )
					{
						messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[3142], language[3143], MSG_COMBAT);
					}
					if ( player >= 0 )
					{
						color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
						messagePlayerColor(player, color, language[3141]);
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

Entity* spellEffectPolymorph(Entity* target, Stat* targetStats, Entity* parent, bool fromMagicSpell, int customDuration)
{
	int effectDuration = 0;
	effectDuration = TICKS_PER_SECOND * 60 * (4 + rand() % 3); // 4-6 minutes
	if ( customDuration > 0 )
	{
		effectDuration = customDuration;
	}
	if ( !target || !targetStats )
	{
		if ( parent && parent->behavior == &actPlayer )
		{
			messagePlayer(parent->skill[2], language[3191]); // had no effect
		}
		return nullptr;
	}

	if ( targetStats->type == LICH || targetStats->type == SHOPKEEPER || targetStats->type == DEVIL
		|| targetStats->type == MINOTAUR || targetStats->type == LICH_FIRE || targetStats->type == LICH_ICE
		|| (target->behavior == &actMonster && target->monsterAllySummonRank != 0)
		|| (targetStats->type == INCUBUS && !strncmp(targetStats->name, "inner demon", strlen("inner demon")))
		|| targetStats->type == SENTRYBOT || targetStats->type == SPELLBOT || targetStats->type == GYROBOT
		|| targetStats->type == DUMMYBOT
		)
	{
		if ( parent && parent->behavior == &actPlayer )
		{
			messagePlayer(parent->skill[2], language[3191]); // had no effect
		}
		return nullptr;
	}

	if ( target->behavior == &actMonster )
	{
		Monster monsterSummonType = static_cast<Monster>(rand() % NUMMONSTERS);
		// pick a completely random monster (barring some exceptions).
		// disable shadow spawning if the monster has a leader since it'll aggro the player and bad things
		while ( monsterSummonType == LICH || monsterSummonType == SHOPKEEPER || monsterSummonType == DEVIL
			|| monsterSummonType == MIMIC || monsterSummonType == BUGBEAR || monsterSummonType == OCTOPUS
			|| monsterSummonType == MINOTAUR || monsterSummonType == LICH_FIRE || monsterSummonType == LICH_ICE
			|| monsterSummonType == NOTHING || monsterSummonType == targetStats->type || monsterSummonType == HUMAN
			|| (targetStats->leader_uid != 0 && monsterSummonType == SHADOW) || monsterSummonType == SENTRYBOT
			|| monsterSummonType == SPELLBOT || monsterSummonType == GYROBOT || monsterSummonType == DUMMYBOT )
		{
			monsterSummonType = static_cast<Monster>(rand() % NUMMONSTERS);
		}

		if ( targetStats->type == SHADOW )
		{
			monsterSummonType = CREATURE_IMP; // shadows turn to imps
		}

		bool summonCanEquipItems = false;
		bool hitMonsterCanTransferEquipment = false;

		switch ( monsterSummonType )
		{
			case RAT:
			case SLIME:
			case TROLL:
			case SPIDER:
			case GHOUL:
			case SCORPION:
			case CREATURE_IMP:
			case DEMON:
			case SCARAB:
			case CRYSTALGOLEM:
			case SHADOW:
			case COCKATRICE:
				summonCanEquipItems = false;
				break;
			default:
				summonCanEquipItems = true;
				break;
		}

		switch ( targetStats->type )
		{
			case RAT:
			case SLIME:
			case TROLL:
			case SPIDER:
			case GHOUL:
			case SCORPION:
			case CREATURE_IMP:
			case DEMON:
			case SCARAB:
			case CRYSTALGOLEM:
			case SHADOW:
			case COCKATRICE:
				hitMonsterCanTransferEquipment = false;
				break;
			default:
				hitMonsterCanTransferEquipment = true;
				break;
		}

		bool fellToDeath = false;
		bool tryReposition = false;
		bool fellInLava = false;
		bool fellInWater = false;

		if ( targetStats->EFFECTS[EFF_LEVITATING]
			&& (monsterSummonType != CREATURE_IMP && monsterSummonType != COCKATRICE && monsterSummonType != SHADOW) )
		{
			// check if there's a floor...
			int x, y, u, v;
			x = std::min(std::max<unsigned int>(1, target->x / 16), map.width - 2);
			y = std::min(std::max<unsigned int>(1, target->y / 16), map.height - 2);
			for ( u = x - 1; u <= x + 1; u++ )
			{
				for ( v = y - 1; v <= y + 1; v++ )
				{
					if ( entityInsideTile(target, u, v, 0) )   // no floor
					{
						if ( !map.tiles[0 + u * MAPLAYERS + v * MAPLAYERS * map.height] )
						{
							// no floor.
							fellToDeath = true;
							tryReposition = true;
						}
						else if ( lavatiles[map.tiles[0 + u * MAPLAYERS + v * MAPLAYERS * map.height]] )
						{
							fellInLava = true;
							tryReposition = true;
						}
						else if ( swimmingtiles[map.tiles[0 + u * MAPLAYERS + v * MAPLAYERS * map.height]] )
						{
							fellInWater = true;
							tryReposition = true;
						}
						else
						{
							tryReposition = true; // something else??
						}
						break;
					}
				}
				if ( tryReposition )
				{
					break;
				}
			}
		}

		Entity* summonedEntity = nullptr;
		if ( tryReposition )
		{
			summonedEntity = summonMonster(monsterSummonType, target->x, target->y);
			if ( !summonedEntity && (fellToDeath || fellInLava) )
			{
				summonedEntity = summonMonster(monsterSummonType, target->x, target->y, true); // force try, kill monster later.
			}
		}
		else
		{
			summonedEntity = summonMonster(monsterSummonType, target->x, target->y, true);
		}

		if ( !summonedEntity )
		{
			if ( parent && parent->behavior == &actPlayer )
			{
				if ( fellInWater )
				{
					messagePlayer(parent->skill[2], language[3192]); // water make no work :<
				}
				else
				{
					messagePlayer(parent->skill[2], language[3191]); // failed for some other reason
				}
			}
			return nullptr;
		}

		Stat* summonedStats = summonedEntity->getStats();
		if ( !summonedStats )
		{
			if ( parent && parent->behavior == &actPlayer )
			{
				messagePlayer(parent->skill[2], language[3191]);
			}
			return nullptr;
		}

		// remove equipment from new monster
		if ( summonCanEquipItems )
		{
			// monster does not have generated equipment yet, disable from generating.
			summonedStats->EDITOR_ITEMS[ITEM_SLOT_WEAPON] = 0;
			summonedStats->EDITOR_ITEMS[ITEM_SLOT_SHIELD] = 0;
			summonedStats->EDITOR_ITEMS[ITEM_SLOT_ARMOR] = 0;
			summonedStats->EDITOR_ITEMS[ITEM_SLOT_HELM] = 0;
			summonedStats->EDITOR_ITEMS[ITEM_SLOT_GLOVES] = 0;
			summonedStats->EDITOR_ITEMS[ITEM_SLOT_BOOTS] = 0;
			summonedStats->EDITOR_ITEMS[ITEM_SLOT_RING] = 0;
			summonedStats->EDITOR_ITEMS[ITEM_SLOT_CLOAK] = 0;
			summonedStats->EDITOR_ITEMS[ITEM_SLOT_AMULET] = 0;
		}

		for ( int x = ITEM_SLOT_INV_1; x <= ITEM_SLOT_INV_6; x = x + ITEM_SLOT_NUMPROPERTIES )
		{
			if ( summonedStats->EDITOR_ITEMS[x] == 1 && summonedStats->EDITOR_ITEMS[x + ITEM_SLOT_CATEGORY] == 0 )
			{
				summonedStats->EDITOR_ITEMS[x] = 0; //clear default item in inventory
			}
		}

		// copy stats from target to new creature.
		summonedStats->HP = targetStats->HP;
		summonedStats->OLDHP = targetStats->OLDHP;
		summonedStats->MP = targetStats->MP;
		summonedStats->MAXHP = targetStats->MAXHP;
		summonedStats->MAXMP = targetStats->MAXMP;
		summonedStats->STR = targetStats->STR;
		summonedStats->DEX = targetStats->DEX;
		summonedStats->CON = targetStats->CON;
		summonedStats->INT = targetStats->INT;
		summonedStats->PER = targetStats->PER;
		//summonedStats->CHR = targetStats->CHR;
		summonedStats->LVL = targetStats->LVL;
		summonedStats->GOLD = targetStats->GOLD;

		// don't apply random stats again
		summonedStats->RANDOM_HP = 0;
		summonedStats->RANDOM_MP = 0;
		summonedStats->RANDOM_MAXHP = 0;
		summonedStats->RANDOM_MAXMP = 0;
		summonedStats->RANDOM_STR = 0;
		summonedStats->RANDOM_DEX = 0;
		summonedStats->RANDOM_CON = 0;
		summonedStats->RANDOM_INT = 0;
		summonedStats->RANDOM_PER = 0;
		summonedStats->RANDOM_CHR = 0;
		summonedStats->RANDOM_LVL = 0;
		summonedStats->RANDOM_GOLD = 0;

		summonedStats->leader_uid = targetStats->leader_uid;
		if ( summonedStats->leader_uid != 0 && summonedStats->type != SHADOW )
		{
			Entity* leader = uidToEntity(summonedStats->leader_uid);
			if ( leader )
			{
				// lose old ally
				if ( target->monsterAllyIndex != -1 )
				{
					int playerFollower = MAXPLAYERS;
					for ( int c = 0; c < MAXPLAYERS; c++ )
					{
						if ( players[c] && players[c]->entity )
						{
							if ( targetStats->leader_uid == players[c]->entity->getUID() )
							{
								playerFollower = c;
								if ( stats[c] )
								{
									for ( node_t* allyNode = stats[c]->FOLLOWERS.first; allyNode != nullptr; allyNode = allyNode->next )
									{
										if ( *((Uint32*)allyNode->element) == target->getUID() )
										{
											list_RemoveNode(allyNode);
											if ( c != clientnum )
											{
												serverRemoveClientFollower(c, target->getUID());
											}
											else
											{
												if ( FollowerMenu.recentEntity && (FollowerMenu.recentEntity->getUID() == 0
													|| FollowerMenu.recentEntity->getUID() == target->getUID()) )
												{
													FollowerMenu.recentEntity = nullptr;
												}
											}
											break;
										}
									}
								}
								break;
							}
						}
					}
				}

				if ( forceFollower(*leader, *summonedEntity) )
				{
					if ( leader->behavior == &actPlayer )
					{
						summonedEntity->monsterAllyIndex = leader->skill[2];
						if ( multiplayer == SERVER )
						{
							serverUpdateEntitySkill(summonedEntity, 42); // update monsterAllyIndex for clients.
						}

					}
					// change the color of the hit entity.
					summonedEntity->flags[USERFLAG2] = true;
					serverUpdateEntityFlag(summonedEntity, USERFLAG2);
					if ( monsterChangesColorWhenAlly(summonedStats) )
					{
						int bodypart = 0;
						for ( node_t* node = summonedEntity->children.first; node != nullptr; node = node->next )
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
		if ( targetStats->type == HUMAN )
		{
			strcpy(summonedStats->name, targetStats->name);
		}

		if ( hitMonsterCanTransferEquipment && summonCanEquipItems )
		{
			// weapon
			Item** slot = itemSlot(targetStats, targetStats->weapon);
			if ( slot )
			{
				summonedStats->weapon = newItem((*slot)->type, (*slot)->status, (*slot)->beatitude,
					(*slot)->count, (*slot)->appearance, (*slot)->identified, nullptr);
			}

			// shield
			slot = itemSlot(targetStats, targetStats->shield);
			if ( slot )
			{
				summonedStats->shield = newItem((*slot)->type, (*slot)->status, (*slot)->beatitude,
					(*slot)->count, (*slot)->appearance, (*slot)->identified, nullptr);
			}

			// breastplate
			slot = itemSlot(targetStats, targetStats->breastplate);
			if ( slot )
			{
				if ( monsterSummonType == KOBOLD || monsterSummonType == GNOME )
				{
					// kobold/gnomes can't equip breastplate, drop it!
					Entity* dropped = dropItemMonster(targetStats->breastplate, target, targetStats);
					if ( dropped )
					{
						dropped->flags[USERFLAG1] = true;
					}
				}
				else
				{
					summonedStats->breastplate = newItem((*slot)->type, (*slot)->status, (*slot)->beatitude,
						(*slot)->count, (*slot)->appearance, (*slot)->identified, nullptr);
				}
			}

			// shoes
			slot = itemSlot(targetStats, targetStats->shoes);
			if ( slot )
			{
				summonedStats->shoes = newItem((*slot)->type, (*slot)->status, (*slot)->beatitude,
					(*slot)->count, (*slot)->appearance, (*slot)->identified, nullptr);
			}

			// helm
			slot = itemSlot(targetStats, targetStats->helmet);
			if ( slot )
			{
				if ( monsterSummonType == KOBOLD || monsterSummonType == GNOME )
				{
					// kobold/gnomes can't equip non-hoods, drop the rest
					if ( (*slot)->type == HAT_HOOD )
					{
						summonedStats->helmet = newItem((*slot)->type, (*slot)->status, (*slot)->beatitude,
							(*slot)->count, (*slot)->appearance, (*slot)->identified, nullptr);
					}
					else
					{
						Entity* dropped = dropItemMonster(targetStats->helmet, target, targetStats);
						if ( dropped )
						{
							dropped->flags[USERFLAG1] = true;
						}
					}
				}
				else
				{
					summonedStats->helmet = newItem((*slot)->type, (*slot)->status, (*slot)->beatitude,
						(*slot)->count, (*slot)->appearance, (*slot)->identified, nullptr);
				}
			}

			// amulet
			slot = itemSlot(targetStats, targetStats->amulet);
			if ( slot )
			{
				summonedStats->amulet = newItem((*slot)->type, (*slot)->status, (*slot)->beatitude,
					(*slot)->count, (*slot)->appearance, (*slot)->identified, nullptr);
			}

			// ring
			slot = itemSlot(targetStats, targetStats->ring);
			if ( slot )
			{
				summonedStats->ring = newItem((*slot)->type, (*slot)->status, (*slot)->beatitude,
					(*slot)->count, (*slot)->appearance, (*slot)->identified, nullptr);
			}

			// cloak
			slot = itemSlot(targetStats, targetStats->cloak);
			if ( slot )
			{
				summonedStats->cloak = newItem((*slot)->type, (*slot)->status, (*slot)->beatitude,
					(*slot)->count, (*slot)->appearance, (*slot)->identified, nullptr);
			}

			// gloves
			slot = itemSlot(targetStats, targetStats->gloves);
			if ( slot )
			{
				if ( monsterSummonType == KOBOLD || monsterSummonType == GNOME )
				{
					// kobold/gnomes can't equip gloves, drop it!
					Entity* dropped = dropItemMonster(targetStats->gloves, target, targetStats);
					if ( dropped )
					{
						dropped->flags[USERFLAG1] = true;
					}
				}
				else
				{
					summonedStats->gloves = newItem((*slot)->type, (*slot)->status, (*slot)->beatitude,
						(*slot)->count, (*slot)->appearance, (*slot)->identified, nullptr);
				}
			}
		}
		else if ( hitMonsterCanTransferEquipment && !summonCanEquipItems )
		{
			Entity* dropped = dropItemMonster(targetStats->weapon, target, targetStats);
			if ( dropped )
			{
				dropped->flags[USERFLAG1] = true;
			}
			dropped = dropItemMonster(targetStats->shield, target, targetStats);
			if ( dropped )
			{
				dropped->flags[USERFLAG1] = true;
			}
			dropped = dropItemMonster(targetStats->breastplate, target, targetStats);
			if ( dropped )
			{
				dropped->flags[USERFLAG1] = true;
			}
			dropped = dropItemMonster(targetStats->shoes, target, targetStats);
			if ( dropped )
			{
				dropped->flags[USERFLAG1] = true;
			}
			dropped = dropItemMonster(targetStats->gloves, target, targetStats);
			if ( dropped )
			{
				dropped->flags[USERFLAG1] = true;
			}
			dropped = dropItemMonster(targetStats->ring, target, targetStats);
			if ( dropped )
			{
				dropped->flags[USERFLAG1] = true;
			}
			dropped = dropItemMonster(targetStats->amulet, target, targetStats);
			if ( dropped )
			{
				dropped->flags[USERFLAG1] = true;
			}
			dropped = dropItemMonster(targetStats->cloak, target, targetStats);
			if ( dropped )
			{
				dropped->flags[USERFLAG1] = true;
			}
			dropped = dropItemMonster(targetStats->helmet, target, targetStats);
			if ( dropped )
			{
				dropped->flags[USERFLAG1] = true;
			}
		}

		node_t* nextnode = nullptr;
		for ( node_t* node = targetStats->inventory.first; node; node = nextnode )
		{
			nextnode = node->next;
			Item* item = (Item*)node->element;
			if ( item && item->appearance != MONSTER_ITEM_UNDROPPABLE_APPEARANCE && itemSlot(targetStats, item) == nullptr )
			{
				Item* copiedItem = newItem(item->type, item->status, item->beatitude, item->count, item->appearance, item->identified, &summonedStats->inventory);
				if ( item->node )
				{
					list_RemoveNode(item->node);
				}
				else
				{
					free(item);
				}
			}
		}

		if ( parent && parent->behavior == &actPlayer )
		{
			Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
			bool namedMonsterAsGeneric = monsterNameIsGeneric(*targetStats);
			// the %s polymorph into a %s!
			if ( !strcmp((*targetStats).name, "") || namedMonsterAsGeneric )
			{
				if ( (*targetStats).type < KOBOLD ) //Original monster count
				{
					if ( summonedStats->type < KOBOLD )
					{
						messagePlayerColor(parent->skill[2], color, language[3187], language[90 + (*targetStats).type], language[90 + summonedStats->type]);
					}
					else
					{
						messagePlayerColor(parent->skill[2], color, language[3187], language[90 + (*targetStats).type], language[2000 + summonedStats->type - KOBOLD]);
					}
				}
				else if ( (*targetStats).type >= KOBOLD ) //New monsters
				{
					if ( summonedStats->type < KOBOLD )
					{
						messagePlayerColor(parent->skill[2], color, language[3187], language[2000 + (*targetStats).type - KOBOLD], language[90 + summonedStats->type]);
					}
					else
					{
						messagePlayerColor(parent->skill[2], color, language[3187], language[2000 + (*targetStats).type - KOBOLD], language[2000 + summonedStats->type - KOBOLD]);
					}
				}
			}
			else
			{
				if ( summonedStats->type < KOBOLD )
				{
					messagePlayerColor(parent->skill[2], color, language[3188], (*targetStats).name, language[90 + summonedStats->type]);
				}
				else
				{
					messagePlayerColor(parent->skill[2], color, language[3188], (*targetStats).name, language[2000 + summonedStats->type - KOBOLD]);
				}
			}
		}

		playSoundEntity(target, 400, 92);
		spawnExplosion(target->x, target->y, target->z);
		createParticleDropRising(target, 593, 1.f);
		serverSpawnMiscParticles(target, PARTICLE_EFFECT_RISING_DROP, 593);

		if ( fellToDeath )
		{
			summonedEntity->setObituary(language[3010]); // fell to their death.
			summonedStats->HP = 0; // kill me instantly
		}
		else if ( fellInLava )
		{
			summonedEntity->setObituary(language[1506]); // goes for a swim in some lava.
			summonedStats->HP = 0; // kill me instantly
		}
		else
		{
			for ( node_t* node = map.creatures->first; node != nullptr; node = node->next )
			{
				Entity* creature = (Entity*)node->element;
				if ( creature && creature->behavior == &actMonster && creature != target && creature != summonedEntity )
				{
					if ( creature->monsterTarget == target->getUID() )
					{
						if ( creature->checkEnemy(summonedEntity) )
						{
							creature->monsterAcquireAttackTarget(*summonedEntity, MONSTER_STATE_PATH); // re-acquire new target
						}
						else
						{
							creature->monsterReleaseAttackTarget(); // release if new target is ally.
						}
					}
				}
			}
		}

		list_RemoveNode(target->mynode);
		target = nullptr;
		return summonedEntity;
	}
	else if ( target->behavior == &actPlayer )
	{
		if ( target->setEffect(EFF_POLYMORPH, true, effectDuration, true) )
		{
			spawnExplosion(target->x, target->y, target->z);
			playSoundEntity(target, 400, 92);
			createParticleDropRising(target, 593, 1.f);
			serverSpawnMiscParticles(target, PARTICLE_EFFECT_RISING_DROP, 593);

			if ( targetStats->playerRace == RACE_HUMAN )
			{
				int roll = (RACE_HUMAN + 1) + rand() % 8;
				if ( target->effectPolymorph == 0 )
				{
					target->effectPolymorph = target->getMonsterFromPlayerRace(roll);
				}
				else
				{
					while ( target->effectPolymorph == target->getMonsterFromPlayerRace(roll) )
					{
						roll = (RACE_HUMAN + 1) + rand() % 8; // re roll to not polymorph into the same thing
					}
					target->effectPolymorph = target->getMonsterFromPlayerRace(roll);
				}
			}
			else if ( (targetStats->playerRace != RACE_HUMAN && targetStats->appearance == 0) )
			{
				target->effectPolymorph = 100 + rand() % NUMAPPEARANCES;
			}
			serverUpdateEntitySkill(target, 50);

			Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
			Monster race = NOTHING;
			if ( target->effectPolymorph > NUMMONSTERS )
			{
				race = HUMAN;
			}
			else
			{
				race = static_cast<Monster>(target->effectPolymorph);
			}
			if ( race < KOBOLD )
			{
				messagePlayerColor(target->skill[2], color, language[3186], language[90 + race]);
			}
			else
			{
				messagePlayerColor(target->skill[2], color, language[3186], language[2000 + race - KOBOLD]);
			}

			// change player's type here, don't like this.. will get auto reset in actPlayer() though
			// otherwise the below aggro check will still assume previous race since actPlayer() hasn't run yet.
			targetStats->type = race;

			for ( node_t* node = map.creatures->first; node != nullptr; node = node->next )
			{
				Entity* creature = (Entity*)node->element;
				if ( creature && creature->behavior == &actMonster && creature != target )
				{
					if ( creature->monsterTarget == target->getUID() )
					{
						if ( creature->checkEnemy(target) )
						{
							creature->monsterAcquireAttackTarget(*target, MONSTER_STATE_PATH); // re-acquire new target
						}
						else
						{
							creature->monsterReleaseAttackTarget(); // release if new target is ally.
						}
					}
				}
			}
		}
		else
		{
			messagePlayer(target->skill[2], language[3189]);
		}

	}

	return nullptr;
}

bool spellEffectTeleportPull(Entity* my, spellElement_t& element, Entity* parent, Entity* target, int resistance)
{
	if ( !parent )
	{
		return false;
	}
	if ( target )
	{
		playSoundEntity(target, 173, 128);
		//int damage = element.damage;
		//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;

		if ( target->behavior == &actMonster || target->behavior == &actPlayer 
			/*|| target->behavior == &actDoor || target->behavior == &actChest*/ )
		{
			Stat* hitstats = target->getStats();
			if ( hitstats )
			{
				if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
				{
					// test for friendly fire
					if ( parent && parent->checkFriend(target) )
					{
						return false;
					}
				}
			}
			//playSoundEntity(target, 249, 64);

			if ( parent )
			{
				if ( target->behavior == &actPlayer )
				{
					if ( MFLAG_DISABLETELEPORT )
					{
						// can't teleport here.
						Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 255);
						messagePlayerColor(target->skill[2], color, language[2381]);
						if ( parent->behavior == &actPlayer )
						{
							messagePlayerColor(parent->skill[2], color, language[3452]);
						}
						return false;
					}
				}
				if ( target->behavior == &actMonster && target->isBossMonster() )
				{
					if ( parent->behavior == &actPlayer )
					{
						Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
						if ( hitstats )
						{
							messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[2905], language[2906], MSG_COMBAT);
						}
					}
					return false;
				}

				// try find a teleport location in front of the caster.
				int tx = static_cast<int>(std::floor(parent->x + 32 * cos(parent->yaw))) >> 4;
				int ty = static_cast<int>(std::floor(parent->y + 32 * sin(parent->yaw))) >> 4;
				int dist = 2;
				bool foundLocation = false;
				int numlocations = 0;
				std::vector<std::pair<int, int>> goodspots;
				std::vector<std::pair<int, int>> spotsWithLineOfSight;
				if ( !checkObstacle((tx << 4) + 8, (ty << 4) + 8, target, NULL) ) // try find directly infront of caster.
				{
					Entity* ohitentity = hit.entity;
					real_t ox = target->x;
					real_t oy = target->y;
					target->x = (tx << 4) + 8;
					target->y = (ty << 4) + 8;
					TileEntityList.updateEntity(*target); // important - lineTrace needs the TileEntityListUpdated.

					// pretend the target is in the supposed spawn locations and try linetrace from each position.
					real_t tangent = atan2(target->y - parent->y, target->x - parent->x);
					lineTraceTarget(parent, parent->x, parent->y, tangent, 92, 0, true, target);
					if ( hit.entity == target )
					{
						foundLocation = true;
					}
					
					// reset the coordinates we messed with
					target->x = ox;
					target->y = oy;
					TileEntityList.updateEntity(*target); // important - lineTrace needs the TileEntityListUpdated.
					hit.entity = ohitentity;
				}
				if ( !foundLocation )
				{
					// otherwise, let's search in an area
					for ( int iy = std::max(1, ty - dist); iy < std::min(ty + dist, static_cast<int>(map.height)); ++iy )
					{
						for ( int ix = std::max(1, tx - dist); ix < std::min(tx + dist, static_cast<int>(map.width)); ++ix )
						{
							if ( !checkObstacle((ix << 4) + 8, (iy << 4) + 8, target, NULL) )
							{
								Entity* ohitentity = hit.entity;
								real_t ox = target->x;
								real_t oy = target->y;
								target->x = (ix << 4) + 8;
								target->y = (iy << 4) + 8;
								TileEntityList.updateEntity(*target); // important - lineTrace needs the TileEntityListUpdated.

								// pretend the target is in the supposed spawn locations and try linetrace from each position.
								real_t tangent = atan2(target->y - parent->y, target->x - parent->x);
								lineTraceTarget(parent, parent->x, parent->y, tangent, 92, 0, false, target);
								if ( hit.entity == target )
								{
									spotsWithLineOfSight.push_back(std::make_pair(ix, iy));
								}
								goodspots.push_back(std::make_pair(ix, iy));
								numlocations++;
								// reset the coordinates we messed with
								target->x = ox;
								target->y = oy;
								TileEntityList.updateEntity(*target); // important - lineTrace needs the TileEntityListUpdated.
								hit.entity = ohitentity;
							}
						}
					}
					if ( numlocations == 0 )
					{
						if ( parent->behavior == &actPlayer )
						{
							// no room to teleport!
							messagePlayer(parent->skill[2], language[3453]);
						}
						return false;
					}

					if ( !spotsWithLineOfSight.empty() )
					{
						std::pair<int, int> tmpPair = spotsWithLineOfSight[rand() % spotsWithLineOfSight.size()];
						tx = tmpPair.first;
						ty = tmpPair.second;
					}
					else if ( !goodspots.empty() )
					{
						std::pair<int, int> tmpPair = goodspots[rand() % goodspots.size()];
						tx = tmpPair.first;
						ty = tmpPair.second;
					}
					else
					{
						if ( parent->behavior == &actPlayer )
						{
							// no room to teleport!
							messagePlayer(parent->skill[2], language[3453]);
						}
						return false;
					}
				}

				// this timer is the entity spawn location.
				Entity* locationTimer = createParticleTimer(parent, 40, 593); 
				locationTimer->x = tx * 16.0 + 8;
				locationTimer->y = ty * 16.0 + 8;
				locationTimer->z = 0;
				locationTimer->particleTimerCountdownAction = PARTICLE_EFFECT_TELEPORT_PULL_TARGET_LOCATION;
				locationTimer->particleTimerCountdownSprite = 593;
				locationTimer->particleTimerTarget = static_cast<Sint32>(target->getUID()); // get the target to teleport around.
				locationTimer->particleTimerEndAction = PARTICLE_EFFECT_TELEPORT_PULL; // teleport behavior of timer.
				locationTimer->particleTimerEndSprite = 593; // sprite to use for end of timer function.
				locationTimer->flags[PASSABLE] = false; // so this location is reserved for teleporting the entity.
				locationTimer->sizex = 4;
				locationTimer->sizey = 4;
				if ( !locationTimer->myTileListNode )
				{
					locationTimer->setUID(-2);
					TileEntityList.addEntity(*locationTimer);
					locationTimer->setUID(-3);
				}

				// set a coundown to spawn particles on the monster.
				Entity* spellTimer = createParticleTimer(target, 40, 593);
				spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SHOOT_PARTICLES;
				spellTimer->particleTimerCountdownSprite = 593;
				spellTimer->particleTimerTarget = static_cast<Sint32>(parent->getUID()); // get the target to teleport around.


				if ( multiplayer == SERVER )
				{
					serverSpawnMiscParticles(target, PARTICLE_EFFECT_TELEPORT_PULL, 593);
					serverSpawnMiscParticlesAtLocation(tx, ty, 0, PARTICLE_EFFECT_TELEPORT_PULL_TARGET_LOCATION, 593);
				}

				Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
				if ( parent->behavior == &actPlayer )
				{
					// play a sound for the player to confirm the hit.
					playSoundPlayer(parent->skill[2], 251, 128);
				}

				// update enemy bar for attacker
				if ( hitstats )
				{
					if ( !strcmp(hitstats->name, "") )
					{
						if ( hitstats->type < KOBOLD ) //Original monster count
						{
							updateEnemyBar(parent, target, language[90 + hitstats->type], hitstats->HP, hitstats->MAXHP);
						}
						else if ( hitstats->type >= KOBOLD ) //New monsters
						{
							updateEnemyBar(parent, target, language[2000 + (hitstats->type - KOBOLD)], hitstats->HP, hitstats->MAXHP);
						}
					}
					else
					{
						updateEnemyBar(parent, target, hitstats->name, hitstats->HP, hitstats->MAXHP);
					}
				}
			}
			return true;
		}
		if ( my )
		{
			spawnMagicEffectParticles(target->x, target->y, target->z, my->sprite);
		}
	}
	else if ( my )
	{
		spawnMagicEffectParticles(my->x, my->y, my->z, my->sprite);
	}
	return false;
}

void spellEffectShadowTag(Entity& my, spellElement_t& element, Entity* parent, int resistance)
{
	if ( hit.entity )
	{
		//int damage = element.damage;
		//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;

		if ( hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer )
		{
			playSoundEntity(&my, 174, 128);
			Stat* hitstats = hit.entity->getStats();
			if ( !hitstats )
			{
				return;
			}

			if ( parent )
			{
				bool sameAsPrevious = false;
				if ( parent->creatureShadowTaggedThisUid != 0 )
				{
					Entity* oldTarget = nullptr;
					if ( oldTarget = uidToEntity(parent->creatureShadowTaggedThisUid) )
					{
						if ( oldTarget != hit.entity )
						{
							oldTarget->setEffect(EFF_SHADOW_TAGGED, false, 0, true);
						}
						else
						{
							sameAsPrevious = true;
						}
					}
				}
				if ( parent->checkFriend(hit.entity) )
				{
					hit.entity->setEffect(EFF_SHADOW_TAGGED, true, 60 * TICKS_PER_SECOND, true);
				}
				else
				{
					hit.entity->setEffect(EFF_SHADOW_TAGGED, true, 10 * TICKS_PER_SECOND, true);
				}
				parent->creatureShadowTaggedThisUid = hit.entity->getUID();
				serverUpdateEntitySkill(parent, 54);
				if ( !sameAsPrevious )
				{
					createParticleShadowTag(hit.entity, parent->getUID(), 60 * TICKS_PER_SECOND);
					serverSpawnMiscParticles(hit.entity, PARTICLE_EFFECT_SHADOW_TAG, 870, parent->getUID());
				}
			}

			// hit messages
			if ( parent )
			{
				Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
				if ( parent->behavior == &actPlayer )
				{
					messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[3463], language[3464], MSG_COMBAT);
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

			Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
			int player = -1;
			if ( hit.entity->behavior == &actPlayer )
			{
				player = hit.entity->skill[2];
				if ( player >= 0 )
				{
					messagePlayerColor(player, color, language[3465]);
				}
			}
		}
		spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my.sprite);
	}
	else
	{
		spawnMagicEffectParticles(my.x, my.y, my.z, my.sprite);
	}
}

bool spellEffectDemonIllusion(Entity& my, spellElement_t& element, Entity* parent, Entity* target, int resistance)
{
	if ( target )
	{
		//int damage = element.damage;
		//damage += ((element->mana - element->base_mana) / static_cast<double>(element->overload_multiplier)) * element->damage;

		if ( target->behavior == &actMonster || target->behavior == &actPlayer )
		{
			Stat* hitstats = target->getStats();
			if ( !hitstats )
			{
				return false;
			}

			if ( hitstats->type == INCUBUS || hitstats->type == SUCCUBUS 
				|| hitstats->type == AUTOMATON || hitstats->type == DEVIL || hitstats->type == DEMON || hitstats->type == CREATURE_IMP
				|| hitstats->type == SHADOW
				|| (hitstats->type == INCUBUS && !strncmp(hitstats->name, "inner demon", strlen("inner demon"))) )
			{
				if ( parent && parent->behavior == &actPlayer )
				{
					// unable to taunt!
					Uint32 color = SDL_MapRGB(mainsurface->format, 255, 255, 255);
					messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[3472], language[3473], MSG_COMBAT);
				}
				return false;
			}
			else if ( hitstats->monsterDemonHasBeenExorcised != 0 
				&& target->behavior != &actPlayer )
			{
				if ( parent && parent->behavior == &actPlayer )
				{
					// already exorcised!
					Uint32 color = SDL_MapRGB(mainsurface->format, 255, 255, 255);
					messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[3735], language[3736], MSG_COMBAT);
				}
				return false;
			}

			if ( parent )
			{
				// try find a summon location around the entity.
				int tx = static_cast<int>(std::floor(target->x)) >> 4;
				int ty = static_cast<int>(std::floor(target->y)) >> 4;
				int dist = 3;
				int numlocations = 0;
				std::vector<std::pair<int, int>> goodspots;
				for ( int iy = std::max(1, ty - dist); iy < std::min(ty + dist, static_cast<int>(map.height)); ++iy )
				{
					for ( int ix = std::max(1, tx - dist); ix < std::min(tx + dist, static_cast<int>(map.width)); ++ix )
					{
						if ( !checkObstacle((ix << 4) + 8, (iy << 4) + 8, target, NULL) )
						{
							Entity* ohitentity = hit.entity;
							real_t ox = parent->x;
							real_t oy = parent->y;
							parent->x = (ix << 4) + 8;
							parent->y = (iy << 4) + 8;
							TileEntityList.updateEntity(*parent); // important - lineTrace needs the TileEntityListUpdated.

							// pretend the parent is in the supposed spawn locations and try linetrace from each position.
							real_t tangent = atan2(parent->y - target->y, parent->x - target->x);
							lineTraceTarget(target, target->x, target->y, tangent, 64, 0, false, parent);
							if ( hit.entity == parent )
							{
								goodspots.push_back(std::make_pair(ix, iy));
								numlocations++;
							}
							// reset the coordinates we messed with
							parent->x = ox;
							parent->y = oy;
							TileEntityList.updateEntity(*parent); // important - lineTrace needs the TileEntityListUpdated.
							hit.entity = ohitentity;
						}
					}
				}
				if ( numlocations == 0 )
				{
					if ( parent->behavior == &actPlayer )
					{
						// no room to spawn!
						messagePlayer(parent->skill[2], language[3471]);
					}
					return false;
				}
				std::pair<int, int> tmpPair = goodspots[rand() % goodspots.size()];
				tx = tmpPair.first;
				ty = tmpPair.second;

				Entity* monster = summonMonster(INCUBUS, tx * 16.0 + 8, ty * 16.0 + 8, true);
				if ( monster )
				{
					spawnExplosion(monster->x, monster->y, -1);
					playSoundEntity(monster, 171, 128);
					//playSoundEntity(&my, 178, 128);
					createParticleErupt(monster, 983);
					serverSpawnMiscParticles(monster, PARTICLE_EFFECT_ERUPT, 983);

					monster->parent = parent->getUID();
					monster->monsterIllusionTauntingThisUid = static_cast<Sint32>(target->getUID());
					switch ( target->getRace() )
					{
						case LICH:
						case LICH_FIRE:
						case LICH_ICE:
						case MINOTAUR:
							break;
						default:
							target->monsterAcquireAttackTarget(*monster, MONSTER_STATE_PATH);
							break;
					}
					monster->lookAtEntity(*target);
					Stat* monsterStats = monster->getStats();
					if ( monsterStats )
					{
						monsterStats->leader_uid = 0;
						strcpy(monsterStats->name, "inner demon");
						monster->setEffect(EFF_STUNNED, true, 20, false);
						monster->flags[USERFLAG2] = true;
						serverUpdateEntityFlag(monster, USERFLAG2);
						if ( monsterChangesColorWhenAlly(monsterStats) )
						{
							int bodypart = 0;
							for ( node_t* node = (monster)->children.first; node != nullptr; node = node->next )
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
					Stat* parentStats = parent->getStats();
					if ( parentStats )
					{
						if ( parent->behavior == &actPlayer )
						{
							Uint32 color = SDL_MapRGB(mainsurface->format, 255, 255, 0);
							messagePlayerColor(parent->skill[2], color, language[621]);
						}
						parent->modHP(-(parentStats->MAXHP / 10));
						if ( parentStats->sex == MALE )
						{
							parent->setObituary(language[1528]);
						}
						else
						{
							parent->setObituary(language[1529]);
						}
					}

					hitstats->monsterDemonHasBeenExorcised++;

					// hit messages
					Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
					if ( parent->behavior == &actPlayer )
					{
						messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[3469], language[3470], MSG_COMBAT);
					}
				}
			}

			Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
			int player = -1;
			if ( target->behavior == &actPlayer )
			{
				player = target->skill[2];
				if ( player >= 0 )
				{
					messagePlayerColor(player, color, language[3468]);
					if ( hitstats->monsterDemonHasBeenExorcised == 3 )
					{
						Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
						messagePlayerColor(player, color, language[3468]);
					}
				}
			}
			spawnMagicEffectParticles(target->x, target->y, target->z, my.sprite);
			return true;
		}
		spawnMagicEffectParticles(target->x, target->y, target->z, my.sprite);
	}
	else
	{
		spawnMagicEffectParticles(my.x, my.y, my.z, my.sprite);
	}
	return false;
}