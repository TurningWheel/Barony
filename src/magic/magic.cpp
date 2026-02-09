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
#include "../engine/audio/sound.hpp"
#include "../items.hpp"
#include "../net.hpp"
#include "../player.hpp"
#include "magic.hpp"
#include "../collision.hpp"
#include "../classdescriptions.hpp"
#include "../scores.hpp"
#include "../prng.hpp"
#include "../mod_tools.hpp"

std::map<Uint32, std::map<Uint32, ParticleEmitterHit_t>> particleTimerEmitterHitEntities;
std::map<Uint32, ParticleTimerEffect_t> particleTimerEffects;
ParticleEmitterHit_t* getParticleEmitterHitProps(Uint32 emitterUid, Entity* hitentity)
{
	if ( emitterUid == 0 || !hitentity ) { return nullptr; }

	if ( (Sint32)(hitentity->getUID()) >= 0 )
	{
		auto& emitterHit = particleTimerEmitterHitEntities[emitterUid];
		auto find = emitterHit.find(hitentity->getUID());
		if ( find != emitterHit.end() )
		{
			return &find->second;
		}
		else
		{
			auto& entry = emitterHit[hitentity->getUID()];
			return &entry;
		}
	}
	return nullptr;
}

void freeSpells()
{
	for ( auto it = allGameSpells.begin(); it != allGameSpells.end(); ++it )
	{
		spell_t& spell = *it->second;
		list_FreeAll(&spell.elements);
	}
}

void spell_magicMap(int player, int radius, int x, int y)
{
	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	if ( multiplayer == SERVER && player > 0 && !players[player]->isLocalPlayer() )
	{
		//Tell the client to map the magic.
		strcpy((char*)net_packet->data, "MMAP");
		SDLNet_Write16(radius, &net_packet->data[4]);
		SDLNet_Write16(x, &net_packet->data[6]);
		SDLNet_Write16(y, &net_packet->data[8]);
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 10;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
		return;
	}

	messagePlayer(player, MESSAGE_HINT, Language::get(412));
	mapLevel(player, radius, x, y, true);
}

void spell_detectFoodEffectOnMap(int player)
{
	if ( players[player] == nullptr || players[player]->entity == nullptr )
	{
		return;
	}

	if ( multiplayer == SERVER && player > 0 && !players[player]->isLocalPlayer() )
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
    // deprecated
}

bool spellEffectDominate(Entity& my, spellElement_t& element, Entity& caster, Entity* parent)
{
	if ( !hit.entity )
	{
		return false;
	}

	if ( hit.entity->behavior != &actMonster || hit.entity->isInertMimic() )
	{
		return false;
	}

	Stat* hitstats = hit.entity->getStats();
	if ( !hitstats )
	{
		return false;
	}

	//Abort if invalid creature (boss, shopkeep, etc).
	if ( hit.entity->isBossMonster()
		|| hitstats->type == MIMIC
		|| hitstats->type == MINIMIMIC
		|| hitstats->type == BAT_SMALL
		|| hitstats->type == HOLOGRAM
		|| hit.entity->monsterIsTinkeringCreation()
		|| hit.entity->monsterAllySummonRank != 0
		|| (hitstats->type == INCUBUS && !strncmp(hitstats->name, "inner demon", strlen("inner demon")))
		)
	{
		Uint32 color = makeColorRGB(255, 0, 0);
		if ( parent )
		{
			messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, color, Language::get(2429));
		}
		playSoundEntity(hit.entity, 163, 128);
		return false;
	}

	if ( caster.behavior == &actPlayer )
	{
		int numDominated = 0;
		for ( node_t* node = stats[caster.skill[2]]->FOLLOWERS.first; node != nullptr; node = node->next )
		{
			Entity* follower = nullptr;
			if ( (Uint32*)(node)->element )
			{
				follower = uidToEntity(*((Uint32*)(node)->element));
			}
			if ( follower )
			{
				Stat* followerStats = follower->getStats();
				if ( followerStats && followerStats->getAttribute("DOMINATED_CREATURE") != "" )
				{
					++numDominated;
				}
			}
		}

		int maxDominate = getSpellDamageFromID(SPELL_DOMINATE, &caster, nullptr, &caster, my.actmagicSpellbookBonus / 100.f);
		if ( numDominated >= maxDominate )
		{
			messagePlayerColor(caster.isEntityPlayer(), MESSAGE_COMBAT, makeColorRGB(255, 0, 0), Language::get(6962));
			playSoundEntity(hit.entity, 163, 128);
			return false;
		}
	}

	playSoundEntity(hit.entity, 174, 64); //TODO: Dominate spell sound effect.

	bool previousLeaderMatching = hit.entity->monsterAllyGetPlayerLeader() == parent;

	//Make the monster a follower.
	bool dominated = forceFollower(caster, *hit.entity);

	if ( parent && dominated )
	{
		Uint32 color = makeColorRGB(0, 255, 0);
		if ( parent->behavior == &actPlayer )
		{
			if ( previousLeaderMatching )
			{
				steamAchievementClient(parent->skill[2], "BARONY_ACH_CONFESSOR");
			}

			messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(2428), Language::get(2427), MSG_COMBAT);
			if ( hit.entity->monsterAllyIndex != parent->skill[2] )
			{
				Compendium_t::Events_t::eventUpdateMonster(parent->skill[2], Compendium_t::CPDM_RECRUITED, hit.entity, 1);
				if ( hitstats->type == HUMAN && hitstats->getAttribute("special_npc") == "merlin" )
				{
					Compendium_t::Events_t::eventUpdateWorld(parent->skill[2], Compendium_t::CPDM_MERLINS, "magicians guild", 1);
				}
			}
		}

		hit.entity->monsterAllyIndex = parent->skill[2];
		hit.entity->setEffect(EFF_CONFUSED, false, 0, true);
		if ( multiplayer == SERVER )
		{
			serverUpdateEntitySkill(hit.entity, 42); // update monsterAllyIndex for clients.
		}
		// change the color of the hit entity.

		if ( hit.entity->getStats() )
		{
			hit.entity->getStats()->setAttribute("DOMINATED_CREATURE", "1");
			hit.entity->getStats()->monsterIsCharmed = 0;
		}

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
			if ( casterStats->amulet && casterStats->amulet->type == AMULET_LIFESAVING 
				&& (casterStats->amulet->beatitude >= 0 || shouldInvertEquipmentBeatitude(casterStats)) )
			{
				// we're good!
				steamAchievementEntity(&caster, "BARONY_ACH_LIFE_FOR_A_LIFE");
			}
		}
	}

	spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my.sprite);
	return true;
}

void spellEffectAcid(Entity& my, spellElement_t& element, Entity* parent, int damage, int resistance)
{
	playSoundEntity(&my, 173, 128);
		
	if ( hit.entity )
	{
		if ( (hit.entity->behavior == &actMonster && !hit.entity->isInertMimic()) || hit.entity->behavior == &actPlayer )
		{
			Entity* parent = uidToEntity(my.parent);
			if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
			{
				// test for friendly fire
				if ( parent && parent->checkFriend(hit.entity) && parent->friendlyFireProtection(hit.entity) )
				{
					return;
				}
			}
			//playSoundEntity(&my, 173, 64);
			//playSoundEntity(hit.entity, 28, 64);

			Stat* hitstats = hit.entity->getStats();
			if ( !hitstats )
			{
				return;
			}
			bool hasamulet = false;
			bool hasgoggles = false;
			if ( (hitstats->amulet && hitstats->amulet->type == AMULET_POISONRESISTANCE) || hitstats->type == INSECTOID )
			{
				resistance += 2;
				hasamulet = true;
			}
			if ( hitstats->mask && hitstats->mask->type == MASK_HAZARD_GOGGLES )
			{
				if ( !(hit.entity->behavior == &actPlayer && hit.entity->effectShapeshift != NOTHING) )
				{
					hasgoggles = true;
					resistance += 2;
				}
			}

			if ( hasamulet && !hasgoggles )
			{
				hit.entity->degradeAmuletProc(hitstats, AMULET_POISONRESISTANCE);
			}

			DamageGib dmgGib = DMG_DEFAULT;
			real_t damageMultiplier = Entity::getDamageTableMultiplier(hit.entity, *hitstats, DAMAGE_TABLE_MAGIC, &resistance);

			Entity::modifyDamageMultipliersFromEffects(hit.entity, parent, damageMultiplier, DAMAGE_TABLE_MAGIC, &my, SPELL_ACID_SPRAY);

			if ( damageMultiplier <= 0.75 )
			{
				dmgGib = DMG_WEAKEST;
			}
			else if ( damageMultiplier <= 0.85 )
			{
				dmgGib = DMG_WEAKER;
			}
			else if ( damageMultiplier >= 1.25 )
			{
				dmgGib = resistance == 0 ? DMG_STRONGEST : DMG_WEAKER;
			}
			else if ( damageMultiplier >= 1.15 )
			{
				dmgGib = resistance == 0 ? DMG_STRONGER : DMG_WEAKER;
			}
			else if ( resistance > 0 )
			{
				dmgGib = DMG_WEAKEST;
			}

			int oldHP = hitstats->HP;
			Sint32 preResistanceDamage = damage;
			damage *= damageMultiplier;
			if ( !hasgoggles )
			{
				hit.entity->modHP(-damage);
				magicOnEntityHit(parent, &my, hit.entity, hitstats, preResistanceDamage, damage, oldHP, SPELL_ACID_SPRAY);
			}
			else
			{
				magicOnEntityHit(parent, &my, hit.entity, hitstats, preResistanceDamage, 0, oldHP, SPELL_ACID_SPRAY);
			}

			// write the obituary
			if ( parent )
			{
				parent->killedByMonsterObituary(hit.entity, true);
			}

			int previousDuration = hitstats->EFFECTS_TIMERS[EFF_POISONED];
			int duration = element.duration;
			duration = convertResistancePointsToMagicValue(duration, resistance);
			bool recentlyHitBySameSpell = false;
			if ( !hasamulet && !hasgoggles )
			{
				hitstats->setEffectActive(EFF_POISONED, 1);
				hitstats->EFFECTS_TIMERS[EFF_POISONED] = duration; // 6 seconds.
				if ( abs(duration - previousDuration) > 10 ) // message if not recently acidified
				{
					recentlyHitBySameSpell = false;
				}
				else
				{
					recentlyHitBySameSpell = true;
				}
				hitstats->poisonKiller = my.parent;
			}
			
			if ( !recentlyHitBySameSpell && !hasgoggles )
			{
				playSoundEntity(hit.entity, 249, 64);
			}
			if ( hit.entity->behavior == &actPlayer )
			{
				serverUpdateEffects(hit.entity->skill[2]);
			}
			// hit messages
			if ( parent )
			{
				Uint32 color = makeColorRGB(0, 255, 0);
				if ( parent->behavior == &actPlayer )
				{
					if ( !recentlyHitBySameSpell )
					{
						messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(2431), Language::get(2430), MSG_COMBAT);
					}
				}
			}

			// update enemy bar for attacker
			if ( !strcmp(hitstats->name, "") )
			{
				updateEnemyBar(parent, hit.entity, getMonsterLocalizedName(hitstats->type).c_str(), hitstats->HP, hitstats->MAXHP,
					false, dmgGib);
			}
			else
			{
				updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP,
					false, dmgGib);
			}

			if ( oldHP > 0 && hitstats->HP <= 0 && parent )
			{
				parent->awardXP(hit.entity, true, true);
				spawnBloodVialOnMonsterDeath(hit.entity, hitstats, parent);
			}

			Uint32 color = makeColorRGB(255, 0, 0);

			int player = -1;
			if ( hit.entity->behavior == &actPlayer )
			{
				player = hit.entity->skill[2];
			}
			if ( player >= 0 )
			{
				if ( !recentlyHitBySameSpell )
				{
					messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(2432));
					if ( hasgoggles )
					{
						messagePlayerColor(player, MESSAGE_COMBAT, makeColorRGB(0, 255, 0), Language::get(6088));
					}
				}
			}

			if ( hitstats->HP > 0 && !hasgoggles )
			{
				// damage armor
				Item* armor = nullptr;
				int armornum = -1;
				if ( hitstats->defending && (local_rng.rand() % (8 + resistance) == 0) ) // 1 in 8 to corrode shield
				{
					armornum = hitstats->pickRandomEquippedItem(&armor, true, false, true, true);
				}
				if ( armornum != -1 && !hitstats->defending && (local_rng.rand() % (4 + resistance) == 0) ) // 1 in 4 to corrode armor
				{
					armornum = hitstats->pickRandomEquippedItem(&armor, true, false, false, false);
				}
				//messagePlayer(0, "armornum: %d", armornum);
				if ( armornum != -1 && armor != nullptr )
				{
					hit.entity->degradeArmor(*hitstats, *armor, armornum);
					//messagePlayerColor(player, color, "Armor piece: %s", armor->getName());

					if ( armor->status == BROKEN )
					{
						if ( parent && parent->behavior == &actPlayer )
						{
							if ( armornum == 4 && hitstats->type == BUGBEAR 
								&& (hitstats->defending || hit.entity->monsterAttack == MONSTER_POSE_BUGBEAR_SHIELD) )
							{
								steamAchievementClient(parent->skill[2], "BARONY_ACH_BEAR_WITH_ME");
							}
						}
					}
				}
			}
		}
		else if ( hit.entity->behavior == &actDoor || hit.entity->behavior == &actIronDoor )
		{
			hit.entity->doorHandleDamageMagic(damage, my, parent);
		}
		else if ( hit.entity->isDamageableCollider() && hit.entity->isColliderDamageableByMagic() )
		{
			hit.entity->colliderHandleDamageMagic(damage, my, parent);
		}
		spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, my.sprite);
	}
	else
	{
		spawnMagicEffectParticles(my.x, my.y, my.z, my.sprite);
	}
}

void spellEffectPoison(Entity& my, spellElement_t& element, Entity* parent, int damage, int resistance)
{
	playSoundEntity(&my, 173, 128);
	if ( hit.entity )
	{
		if ( (hit.entity->behavior == &actMonster && !hit.entity->isInertMimic()) || hit.entity->behavior == &actPlayer )
		{
			Entity* parent = uidToEntity(my.parent);
			if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
			{
				// test for friendly fire
				if ( parent && parent->checkFriend(hit.entity) && parent->friendlyFireProtection(hit.entity) )
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

			if ( hasamulet )
			{
				hit.entity->degradeAmuletProc(hitstats, AMULET_POISONRESISTANCE);
			}

			DamageGib dmgGib = DMG_DEFAULT;
			real_t damageMultiplier = Entity::getDamageTableMultiplier(hit.entity, *hitstats, DAMAGE_TABLE_MAGIC, &resistance);

			Entity::modifyDamageMultipliersFromEffects(hit.entity, parent, damageMultiplier, DAMAGE_TABLE_MAGIC, &my, SPELL_POISON);

			if ( damageMultiplier <= 0.75 )
			{
				dmgGib = DMG_WEAKEST;
			}
			else if ( damageMultiplier <= 0.85 )
			{
				dmgGib = DMG_WEAKER;
			}
			else if ( damageMultiplier >= 1.25 )
			{
				dmgGib = resistance == 0 ? DMG_STRONGEST : DMG_WEAKER;
			}
			else if ( damageMultiplier >= 1.15 )
			{
				dmgGib = resistance == 0 ? DMG_STRONGER : DMG_WEAKER;
			}
			else if ( resistance > 0 )
			{
				dmgGib = DMG_WEAKEST;
			}

			Sint32 preResistanceDamage = damage;
			damage *= damageMultiplier;
			Sint32 oldHP = hitstats->HP;
			hit.entity->modHP(-damage);

			magicOnEntityHit(parent, &my, hit.entity, hitstats, preResistanceDamage, damage, oldHP, SPELL_POISON);

			// write the obituary
			if ( parent )
			{
				parent->killedByMonsterObituary(hit.entity, true);
			}

			if ( !hasamulet )
			{
				if ( my.actmagicCastByMagicstaff == 1 )
				{
					hit.entity->setEffect(EFF_POISONED, true, 320, true); // 6 seconds.
				}
				else
				{
					hit.entity->setEffect(EFF_POISONED, true, std::max(200, element.duration - hit.entity->getCON() * 5), true); // 4-7 seconds.
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
				Uint32 color = makeColorRGB(0, 255, 0);
				if ( parent->behavior == &actPlayer )
				{
					messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(3427), Language::get(3426), MSG_COMBAT);
				}
			}

			// update enemy bar for attacker
			if ( !strcmp(hitstats->name, "") )
			{
				updateEnemyBar(parent, hit.entity, getMonsterLocalizedName(hitstats->type).c_str(), hitstats->HP, hitstats->MAXHP,
					false, dmgGib);
			}
			else
			{
				updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP,
					false, dmgGib);
			}

			if ( hitstats->HP <= 0 && parent )
			{
				parent->awardXP(hit.entity, true, true);
				spawnBloodVialOnMonsterDeath(hit.entity, hitstats, parent);
			}

			Uint32 color = makeColorRGB(255, 0, 0);

			int player = -1;
			if ( hit.entity->behavior == &actPlayer )
			{
				player = hit.entity->skill[2];
			}
			if ( player >= 0 )
			{
				messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(3428));
			}
		}
		else if ( hit.entity->behavior == &actDoor || hit.entity->behavior == &actIronDoor )
		{
			hit.entity->doorHandleDamageMagic(damage, my, parent);
		}
		else if ( hit.entity->isDamageableCollider() && hit.entity->isColliderDamageableByMagic() )
		{
			hit.entity->colliderHandleDamageMagic(damage, my, parent);
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

	if ( (target->behavior == &actMonster && !target->isInertMimic()) || target->behavior == &actPlayer )
	{
		Entity* parent = forceParent;
		if ( my && !parent )
		{
			parent = uidToEntity(my->parent);
		}
		if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
		{
			// test for friendly fire
			if ( parent && parent->checkFriend(target) && parent->friendlyFireProtection(target) )
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
		duration = convertResistancePointsToMagicValue(duration, resistance);
		if ( target->setEffect(EFF_FEAR, true, duration, true) )
		{
			playSoundEntity(target, 687, 128); // fear.ogg
			Uint32 color = 0;
			if ( parent )
			{
				magicOnEntityHit(parent, parent, target, target->getStats(), 0, 0, 0, SPELL_FEAR);
				// update enemy bar for attacker
				/*if ( !strcmp(hitstats->name, "") )
				{
					updateEnemyBar(parent, target, getMonsterLocalizedName(hitstats->type).c_str(), hitstats->HP, hitstats->MAXHP);
				}
				else
				{
					updateEnemyBar(parent, target, hitstats->name, hitstats->HP, hitstats->MAXHP);
				}*/
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
				Uint32 color = makeColorRGB(255, 0, 0);
				if ( parent->behavior == &actPlayer )
				{
					messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(2905), Language::get(2906), MSG_COMBAT);
				}
			}
			return false;
		}

		// hit messages
		if ( parent )
		{
			Uint32 color = makeColorRGB(0, 255, 0);
			if ( parent->behavior == &actPlayer )
			{
				messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(3434), Language::get(3435), MSG_COMBAT);
			}
		}

		Uint32 color = makeColorRGB(255, 0, 0);

		int player = -1;
		if ( target->behavior == &actPlayer )
		{
			player = target->skill[2];
		}
		if ( player >= 0 )
		{
			messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(3436));
		}
	}
	spawnMagicEffectParticles(target->x, target->y, target->z, 863);
	return true;
}

void spellEffectSprayWeb(Entity& my, spellElement_t& element, Entity* parent, int resistance)
{
	if ( hit.entity )
	{
		if ( (hit.entity->behavior == &actMonster && !hit.entity->isInertMimic()) || hit.entity->behavior == &actPlayer )
		{
			Entity* parent = uidToEntity(my.parent);
			if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
			{
				// test for friendly fire
				if ( parent && parent->checkFriend(hit.entity) && parent->friendlyFireProtection(hit.entity) )
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
			if ( hitstats->getEffectActive(EFF_WEBBED) )
			{
				spawnParticles = false;
			}
			int previousDuration = hitstats->EFFECTS_TIMERS[EFF_WEBBED];
			int duration = 400;
			duration = convertResistancePointsToMagicValue(duration, resistance);
			if ( hit.entity->setEffect(EFF_WEBBED, true, duration, true) ) // 8 seconds.
			{
				magicOnEntityHit(parent, &my, hit.entity, hitstats, 0, 0, 0, SPELL_SPRAY_WEB);
				if ( abs(duration - previousDuration) > 10 )
				{
					playSoundEntity(hit.entity, 396 + local_rng.rand() % 3, 64); // play sound only if not recently webbed. (triple shot makes many noise)
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
					Uint32 color = makeColorRGB(255, 0, 0);
					if ( parent->behavior == &actPlayer )
					{
						messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(2905), Language::get(2906), MSG_COMBAT);
					}
				}
				return;
			}

			// hit messages
			if ( parent )
			{
				Uint32 color = makeColorRGB(0, 255, 0);
				if ( parent->behavior == &actPlayer )
				{
					if ( abs(duration - previousDuration) > 10 ) // message if not recently webbed
					{
						messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(3430), Language::get(3429), MSG_COMBAT);
					}
				}
			}

			Uint32 color = makeColorRGB(255, 0, 0);

			int player = -1;
			if ( hit.entity->behavior == &actPlayer )
			{
				player = hit.entity->skill[2];
			}
			if ( player >= 0 )
			{
				if ( abs(duration - previousDuration) > 10 ) // message if not recently webbed
				{
					messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(3431));
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
		if ( (hit.entity->behavior == &actMonster && !hit.entity->isInertMimic()) || hit.entity->behavior == &actPlayer )
		{
			Entity* parent = uidToEntity(my.parent);
			if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
			{
				// test for friendly fire
				if ( parent && parent->checkFriend(hit.entity) && parent->friendlyFireProtection(hit.entity) )
				{
					return;
				}
			}

			Stat* hitstats = hit.entity->getStats();
			if ( !hitstats )
			{
				return;
			}

			if ( hitstats->type == LICH 
				|| hitstats->type == LICH_FIRE 
				|| hitstats->type == LICH_ICE 
				|| hitstats->type == DEVIL
				|| hitstats->type == SHADOW
				|| hitstats->type == SHOPKEEPER )
			{
				return;
			}

			if ( hit.entity->behavior == &actMonster 
				&& (hit.entity->monsterAllySummonRank != 0 
					/*|| hitstats->type == MONSTER_ADORCISED_WEAPON*/
					|| (hitstats->type == INCUBUS && !strncmp(hitstats->name, "inner demon", strlen("inner demon"))))
				)
			{
				return;
			}

			// update enemy bar for attacker
			/*if ( !strcmp(hitstats->name, "") )
			{
				updateEnemyBar(parent, hit.entity, getMonsterLocalizedName(hitstats->type).c_str(), hitstats->HP, hitstats->MAXHP);
			}
			else
			{
				updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP);
			}*/

			Uint32 color = makeColorRGB(255, 0, 0);

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

					magicOnEntityHit(parent, &my, hit.entity, hitstats, 0, 0, 0, SPELL_STEAL_WEAPON);

					// hit messages
					if ( player >= 0 )
					{
						color = makeColorRGB(255, 0, 0);
						messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(2435), hitstats->weapon->getName());
					}

					if ( parent )
					{
						color = makeColorRGB(0, 255, 0);
						if ( parent->behavior == &actPlayer )
						{
							messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(2434), Language::get(2433), MSG_STEAL_WEAPON);
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
						if ( player > 0 && multiplayer == SERVER && !players[player]->isLocalPlayer() )
						{
							strcpy((char*)net_packet->data, "STLA");
							net_packet->data[4] = 5; // steal weapon index in STLA netcode.
							SDLNet_Write32(static_cast<Uint32>(weapon->type), &net_packet->data[5]);
							SDLNet_Write32(static_cast<Uint32>(weapon->status), &net_packet->data[9]);
							SDLNet_Write32(static_cast<Uint32>(weapon->beatitude), &net_packet->data[13]);
							SDLNet_Write32(static_cast<Uint32>(weapon->count), &net_packet->data[17]);
							SDLNet_Write32(static_cast<Uint32>(weapon->appearance), &net_packet->data[21]);
							net_packet->data[25] = weapon->identified;
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 26;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}

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
					}
				}
			}
			else
			{
				playSoundEntity(&my, 163, 128); // failed spell sound
				// hit messages
				if ( player >= 0 )
				{
					color = makeColorRGB(0, 255, 0);
					messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(2438));
				}

				if ( parent )
				{
					color = makeColorRGB(255, 255, 255);
					if ( parent->behavior == &actPlayer )
					{
						messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(2437), Language::get(2436), MSG_COMBAT);
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

void spellEffectDrainSoul(Entity& my, spellElement_t& element, Entity* parent, int damage, int resistance)
{
	if ( hit.entity )
	{
		if ( (hit.entity->behavior == &actMonster && !hit.entity->isInertMimic()) || hit.entity->behavior == &actPlayer )
		{
			Entity* parent = uidToEntity(my.parent);
			if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
			{
				// test for friendly fire
				if ( parent && parent->checkFriend(hit.entity) && parent->friendlyFireProtection(hit.entity) )
				{
					return;
				}
			}

			Stat* hitstats = hit.entity->getStats();
			if ( !hitstats )
			{
				return;
			}

			DamageGib dmgGib = DMG_DEFAULT;
			real_t damageMultiplier = Entity::getDamageTableMultiplier(hit.entity, *hitstats, DAMAGE_TABLE_MAGIC, &resistance);
			if ( damageMultiplier <= 0.75 )
			{
				dmgGib = DMG_WEAKEST;
			}
			else if ( damageMultiplier <= 0.85 )
			{
				dmgGib = DMG_WEAKER;
			}
			else if ( damageMultiplier >= 1.25 )
			{
				dmgGib = resistance == 0 ? DMG_STRONGEST : DMG_WEAKER;
			}
			else if ( damageMultiplier >= 1.15 )
			{
				dmgGib = resistance == 0 ? DMG_STRONGER : DMG_WEAKER;
			}
			else if ( resistance > 0 )
			{
				dmgGib = DMG_WEAKEST;
			}

			Sint32 preResistanceDamage = damage;
			damage *= damageMultiplier;

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
			Sint32 oldHP = hitstats->HP;
			hit.entity->modHP(-damage);

			magicOnEntityHit(parent, &my, hit.entity, hitstats, preResistanceDamage, damage, oldHP, SPELL_DRAIN_SOUL);

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
				parent->killedByMonsterObituary(hit.entity, true);
			}

			// update enemy bar for attacker
			if ( !strcmp(hitstats->name, "") )
			{
				updateEnemyBar(parent, hit.entity, getMonsterLocalizedName(hitstats->type).c_str(), hitstats->HP, hitstats->MAXHP,
					false, dmgGib);
			}
			else
			{
				updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP,
					false, dmgGib);
			}

			Uint32 color = makeColorRGB(255, 0, 0);

			int player = -1;
			if ( hit.entity->behavior == &actPlayer )
			{
				player = hit.entity->skill[2];
			}

			if ( hitstats->HP <= 0 && parent )
			{
				parent->awardXP(hit.entity, true, true);
				spawnBloodVialOnMonsterDeath(hit.entity, hitstats, parent);
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
						color = makeColorRGB(255, 0, 0);
						messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(2441));
					}

					if ( parent )
					{
						color = makeColorRGB(0, 255, 0);
						if ( parent->behavior == &actPlayer )
						{
							messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(2440), Language::get(2439), MSG_COMBAT);
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
					color = makeColorRGB(0, 255, 0);
					messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(2444));
				}

				if ( parent )
				{
					color = makeColorRGB(255, 255, 255);
					if ( parent->behavior == &actPlayer )
					{
						messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(2443), Language::get(2442), MSG_COMBAT);
					}
				}
			}
		}
		else
		{
			bool forceFurnitureDamage = false;
			if ( parent )
			{
				Stat* casterStats = parent->getStats();
				if ( casterStats && casterStats->type == SHOPKEEPER )
				{
					forceFurnitureDamage = true;
				}
			}

			if ( forceFurnitureDamage )
			{
				if ( hit.entity->isDamageableCollider() && hit.entity->isColliderDamageableByMagic() )
				{
					hit.entity->colliderHandleDamageMagic(damage, my, parent);
					return;
				}
				else if ( hit.entity->behavior == &actChest || hit.entity->isInertMimic() )
				{
					hit.entity->chestHandleDamageMagic(damage, my, parent);
					return;
				}
				else if ( hit.entity->behavior == &actFurniture )
				{
					hit.entity->furnitureHandleDamageMagic(damage, my, parent);
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

spell_t* spellEffectVampiricAura(Entity* caster, spell_t* spell)
{
	if ( !caster )
	{
		return nullptr;
	}
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

	node_t* spellnode = list_AddNodeLast(&myStats->magic_effects);
	spellnode->element = copySpell(spell); //We need to save the spell since this is a channeled spell.
	spell_t* channeled_spell = (spell_t*)(spellnode->element);
	channeled_spell->magic_effects_node = spellnode;
	spellnode->size = sizeof(spell_t);
	((spell_t*)spellnode->element)->caster = caster->getUID();
	spellnode->deconstructor = &spellDeconstructor;
	
	int duration = element->duration; // duration in ticks.
	channeled_spell->channel_duration = duration; //Tell the spell how long it's supposed to last so that it knows what to reset its timer to.
	caster->setEffect(EFF_VAMPIRICAURA, true, duration, true);
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( players[i] && caster && (caster == players[i]->entity) )
		{
			serverUpdateEffects(i);
			Uint32 color = makeColorRGB(0, 255, 0);
			messagePlayerColor(i, MESSAGE_COMBAT, color, Language::get(2477));
			playSoundPlayer(i, 403, 32);
		}
	}

	playSoundEntity(caster, 167, 128);
	createParticleDropRising(caster, 600, 0.7);
	serverSpawnMiscParticles(caster, PARTICLE_EFFECT_VAMPIRIC_AURA, 600);
	return channeled_spell;
}

int getCharmMonsterDifficulty(Entity& my, Stat& myStats)
{
	int difficulty = 0;

	switch ( myStats.type )
	{
	default:
		difficulty = 0;
		break;
	case HUMAN:
	case RAT:
	case SLIME:
	case SPIDER:
	case SKELETON:
	case SCORPION:
	case SHOPKEEPER:
	case REVENANT_SKULL:
		difficulty = 0;
		break;
	case GOBLIN:
	case TROLL:
	case GHOUL:
	case GNOME:
	case SCARAB:
	case AUTOMATON:
	case SUCCUBUS:
	case GREMLIN:
		difficulty = 1;
		break;
	case CREATURE_IMP:
	case DEMON:
	case KOBOLD:
	case INCUBUS:
	case INSECTOID:
	case GOATMAN:
	case BUGBEAR:
	case MONSTER_ADORCISED_WEAPON:
	case FLAME_ELEMENTAL:
	case EARTH_ELEMENTAL:
	case MOTH_SMALL:
	case DRYAD:
	case MYCONID:
	case SALAMANDER:
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
	case MIMIC:
	case BAT_SMALL:
	case MINIMIMIC:
	case HOLOGRAM:
	case DUCK_SMALL:
	case MONSTER_UNUSED_6:
	case MONSTER_UNUSED_7:
	case MONSTER_UNUSED_8:
		difficulty = 666;
		break;
	}

	if ( my.monsterCanTradeWith(-1) )
	{
		difficulty = 666;
	}

	/************** CHANCE CALCULATION ***********/
	if ( myStats.getEffectActive(EFF_CONFUSED) || myStats.getEffectActive(EFF_DRUNK) || my.behavior == &actPlayer )
	{
		difficulty -= 1; // players and confused/drunk monsters have lower resistance.
	}
	if ( strcmp(myStats.name, "") && !monsterNameIsGeneric(myStats) )
	{
		difficulty += 1; // minibosses +1 difficulty.
	}
	return difficulty;
}

void spellEffectCharmMonster(Entity& my, spellElement_t& element, Entity* parent, int resistance, bool magicstaff)
{
	if ( hit.entity )
	{
		if ( (hit.entity->behavior == &actMonster && !hit.entity->isInertMimic()) || hit.entity->behavior == &actPlayer )
		{
			if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
			{
				// test for friendly fire
				if ( parent && parent->checkFriend(hit.entity) && parent->friendlyFireProtection(hit.entity) )
				{
					return;
				}
			}

			Stat* hitstats = hit.entity->getStats();
			if ( !hitstats )
			{
				return;
			}

			Uint32 color = makeColorRGB(0, 255, 0);

			int player = -1;
			if ( hit.entity->behavior == &actPlayer )
			{
				player = hit.entity->skill[2];
			}

			int difficulty = getCharmMonsterDifficulty(*hit.entity, *hitstats);

			int chance = 80;
			chance -= difficulty * 30;
			bool allowStealFollowers = false;
			Stat* casterStats = nullptr;
			int currentCharmedFollowerCount = 0;
			int numFollowers = 0;
			if ( parent )
			{
				casterStats = parent->getStats();
				if ( casterStats )
				{
					if ( magicstaff )
					{
						chance += ((parent->getCHR() + std::max(casterStats->getModifiedProficiency(PRO_MYSTICISM), casterStats->getModifiedProficiency(PRO_LEADERSHIP))) / 20) * 10;
					}
					else
					{
						chance += ((parent->getCHR() + std::max(casterStats->getModifiedProficiency(PRO_MYSTICISM), casterStats->getModifiedProficiency(PRO_LEADERSHIP))) / 20) * 5;
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

						if ( Entity* leader = parent->monsterAllyGetPlayerLeader() )
						{
							if ( Stat* leaderStats = leader->getStats() )
							{
								// search followers for charmed.
								for ( node_t* node = leaderStats->FOLLOWERS.first; node != NULL; node = node->next )
								{
									Uint32* c = (Uint32*)node->element;
									Entity* follower = nullptr;
									if ( c )
									{
										follower = uidToEntity(*c);
									}
									if ( follower )
									{
										++numFollowers;
									}
								}
							}
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
			chance = convertResistancePointsToMagicValue(chance, resistance);
			/************** END CHANCE CALCULATION ***********/

			// special cases:
			if ( hit.entity->isBossMonster() && hitstats->type != SHOPKEEPER )
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

			bool doPacify = false;

			if ( parent && hit.entity == parent )
			{
				// caster hit themselves somehow... get pacified.
				int duration = element.duration;
				duration = convertResistancePointsToMagicValue(duration, resistance);
				if ( hit.entity->setEffect(EFF_PACIFY, true, duration, true) )
				{
					playSoundEntity(hit.entity, 168, 128); // Healing.ogg
					if ( player >= 0 )
					{
						color = makeColorRGB(255, 0, 0);
						messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(3144));
					}
					if ( parent )
					{
						if ( parent->behavior == &actPlayer )
						{
							messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(3139), Language::get(3140), MSG_COMBAT);
						}
						// update enemy bar for attacker
						/*if ( !strcmp(hitstats->name, "") )
						{
							updateEnemyBar(parent, hit.entity, getMonsterLocalizedName(hitstats->type).c_str(), hitstats->HP, hitstats->MAXHP);
						}
						else
						{
							updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP);
						}*/
					}
				}
			}
			else if ( chance <= 0 )
			{
				// no effect.
				playSoundEntity(hit.entity, 163, 64); // FailedSpell1V1.ogg
				if ( parent && parent->behavior == &actPlayer )
				{
					messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(2905), Language::get(2906), MSG_COMBAT);
					steamAchievementClient(parent->skill[2], "BARONY_ACH_OFF_LIMITS");
				}
				if ( player >= 0 )
				{
					color = makeColorRGB(0, 255, 0);
					messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(3141));
				}
			}
			else if ( parent && local_rng.rand() % 100 < chance
				&& ( (hitstats->leader_uid == 0 && hit.entity->getUID() != casterStats->leader_uid)
					|| (allowStealFollowers 
						&& hitstats->leader_uid != parent->getUID() // my target is not already following me
						&& hit.entity->getUID() != casterStats->leader_uid // my target is not my leader (otherwise we're each other's leader and that's bad)
						) ) 
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
				else if ( parent->behavior == &actMonster )
				{
					if (Entity* leaderOfTarget = uidToEntity(casterStats->leader_uid))
					{
						whoToFollow = leaderOfTarget;
						if ( whoToFollow->getUID() == casterStats->leader_uid )
						{
							// this is my leader, ignore
							doPacify = true;
						}
						if ( Stat* whoToFollowStats = whoToFollow->getStats() )
						{
							if ( whoToFollowStats->leader_uid == parent->getUID() )
							{
								// i am their leader, ignore
								doPacify = true;
							}
						}

						if ( numFollowers >= 8 )
						{
							doPacify = true; // stop after a point
						}
					}
				}

				if ( !doPacify && forceFollower(*whoToFollow, *hit.entity) )
				{
					serverSpawnMiscParticles(hit.entity, PARTICLE_EFFECT_CHARM_MONSTER, 0);
					createParticleCharmMonster(hit.entity);
					playSoundEntity(hit.entity, 174, 64); // WeirdSpell.ogg
					if ( whoToFollow->behavior == &actPlayer )
					{
						whoToFollow->increaseSkill(PRO_LEADERSHIP);
						messagePlayerMonsterEvent(whoToFollow->skill[2], color, *hitstats, Language::get(3137), Language::get(3138), MSG_COMBAT);
						Compendium_t::Events_t::eventUpdateMonster(whoToFollow->skill[2], Compendium_t::CPDM_RECRUITED, hit.entity, 1);
						if ( hitstats->type == HUMAN && hitstats->getAttribute("special_npc") == "merlin" )
						{
							Compendium_t::Events_t::eventUpdateWorld(whoToFollow->skill[2], Compendium_t::CPDM_MERLINS, "magicians guild", 1);
						}
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

					hit.entity->setEffect(EFF_CONFUSED, false, 0, true);
					magicOnEntityHit(parent, &my, hit.entity, hitstats, 0, 0, 0, SPELL_CHARM_MONSTER);

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
				doPacify = true;
			}

			if ( doPacify )
			{
				// had a chance, or currently in service of another monster, or a player, or spell no parent, failed to completely charm. 
				// loses will to attack.
				int duration = element.duration;
				duration = convertResistancePointsToMagicValue(duration, resistance);
				if ( hitstats->type == SHOPKEEPER )
				{
					duration = 100;
				}
				if ( hit.entity->setEffect(EFF_PACIFY, true, duration, true) )
				{
					magicOnEntityHit(parent, &my, hit.entity, hitstats, 0, 0, 0, SPELL_CHARM_MONSTER);
					playSoundEntity(hit.entity, 168, 128); // Healing.ogg
					if ( player >= 0 )
					{
						color = makeColorRGB(255, 0, 0);
						messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(3144));
					}
					if ( parent )
					{
						if ( parent->behavior == &actPlayer )
						{
							messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(3139), Language::get(3140), MSG_COMBAT);
							if ( currentCharmedFollowerCount > 0 && hit.entity->monsterAllyGetPlayerLeader() != parent )
							{
								messagePlayer(parent->skill[2], MESSAGE_MISC, Language::get(3327));
							}
						}
						// update enemy bar for attacker
						/*if ( !strcmp(hitstats->name, "") )
						{
							updateEnemyBar(parent, hit.entity, getMonsterLocalizedName(hitstats->type).c_str(), hitstats->HP, hitstats->MAXHP);
						}
						else
						{
							updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP);
						}*/
					}
					if ( hitstats->type == SHOPKEEPER && parent && parent->behavior == &actPlayer )
					{
						// reverses shop keeper grudges.
						hit.entity->monsterReleaseAttackTarget();
						for ( node_t* node = map.creatures->first; node != nullptr; node = node->next )
						{
							Entity* entity = (Entity*)node->element;
							if ( !entity ) { continue; }
							if ( entity->behavior == &actMonster && entity != hit.entity )
							{
								if ( entity->monsterAllyGetPlayerLeader() && ((Uint32)entity->monsterTarget == hit.entity->getUID()) )
								{
									entity->monsterReleaseAttackTarget(); // player allies stop attacking this target
								}
							}
						}
						ShopkeeperPlayerHostility.resetPlayerHostility(parent->skill[2]);
					}
				}
				else
				{
					// resists the charm.
					playSoundEntity(hit.entity, 163, 64); // FailedSpell1V1.ogg
					if ( parent && parent->behavior == &actPlayer )
					{
						color = makeColorRGB(255, 0, 0);
						messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(3142), Language::get(3143), MSG_COMBAT);
					}
					if ( player >= 0 )
					{
						color = makeColorRGB(0, 255, 0);
						messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(3141));
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

Entity* spellEffectPolymorph(Entity* target, Entity* parent, bool fromMagicSpell, int customDuration, Monster customMonster)
{
	int effectDuration = 0;
	effectDuration = TICKS_PER_SECOND * 60 * (4 + local_rng.rand() % 3); // 4-6 minutes
	if ( customDuration > 0 )
	{
		effectDuration = customDuration;
	}
	if ( !target || !target->getStats() )
	{
		if ( parent && parent->behavior == &actPlayer && customMonster == NOTHING )
		{
			messagePlayer(parent->skill[2], MESSAGE_HINT, Language::get(3191)); // had no effect
		}
		return nullptr;
	}

	Stat* targetStats = target->getStats();

	if ( customMonster == NOTHING )
	{
		if ( targetStats->type == LICH || targetStats->type == SHOPKEEPER || targetStats->type == DEVIL
			|| targetStats->type == MINOTAUR || targetStats->type == LICH_FIRE || targetStats->type == LICH_ICE
			|| (target->behavior == &actMonster && target->monsterAllySummonRank != 0)
			|| target->monsterCanTradeWith(-1)
			|| (targetStats->type == SKELETON && targetStats->getAttribute("revenant_skeleton") != "" )
			|| targetStats->type == MIMIC || targetStats->type == BAT_SMALL
			|| targetStats->type == MONSTER_ADORCISED_WEAPON
			|| targetStats->type == MOTH_SMALL
			|| targetStats->type == MINIMIMIC
			|| targetStats->type == REVENANT_SKULL
			|| targetStats->type == FLAME_ELEMENTAL
			|| targetStats->type == EARTH_ELEMENTAL
			|| (targetStats->type == VAMPIRE && (targetStats->getAttribute("special_npc") == "bram kindly"))
			|| (targetStats->type == INCUBUS && (targetStats->getAttribute("special_npc") == "johann"))
			|| (targetStats->type == INCUBUS && !strncmp(targetStats->name, "inner demon", strlen("inner demon")))
			|| targetStats->type == SENTRYBOT || targetStats->type == SPELLBOT || targetStats->type == GYROBOT
			|| targetStats->type == DUMMYBOT
			)
		{
			if ( parent && parent->behavior == &actPlayer )
			{
				messagePlayer(parent->skill[2], MESSAGE_HINT, Language::get(3191)); // had no effect
			}
			return nullptr;
		}
	}

	if ( target->behavior == &actMonster )
	{
		auto& rng = target->entity_rng ? *target->entity_rng : local_rng;
		Monster monsterSummonType;

		if ( customMonster != NOTHING )
		{
			monsterSummonType = customMonster;
		}
		else if ( targetStats->type == SHADOW )
		{
			monsterSummonType = CREATURE_IMP; // shadows turn to imps
		}
		else
		{
	        std::set<Monster> typesToSkip
			{
	            LICH, SHOPKEEPER, DEVIL, MIMIC, CRAB, BAT_SMALL,
	            MINOTAUR, LICH_FIRE, LICH_ICE, NOTHING,
	            HUMAN, SENTRYBOT, SPELLBOT, GYROBOT,
	            DUMMYBOT, REVENANT_SKULL,
				MINIMIMIC,
				MONSTER_ADORCISED_WEAPON,
				FLAME_ELEMENTAL,
				HOLOGRAM,
				EARTH_ELEMENTAL,
				DUCK_SMALL,
				MYCONID,
				DRYAD,
				GREMLIN,
				SALAMANDER,
				MONSTER_UNUSED_6,
				MONSTER_UNUSED_7,
				MONSTER_UNUSED_8,
				MOTH_SMALL
	        };
			typesToSkip.insert(targetStats->type);
			if ( target->monsterAllyGetPlayerLeader() )
			{
				typesToSkip.insert(SHADOW);
			}

			std::vector<Monster> possibleTypes;
			for ( int i = 0; i < NUMMONSTERS; ++i )
			{
				const Monster mon = static_cast<Monster>(i);
				if ( typesToSkip.find(mon) == typesToSkip.end() )
				{
					possibleTypes.push_back(mon);
				}
			}
			monsterSummonType = possibleTypes.at(rng.rand() % possibleTypes.size());
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
			case BUGBEAR:
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
			case BUGBEAR:
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

		if ( targetStats->getEffectActive(EFF_LEVITATING)
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
			if ( !summonedEntity && customMonster != NOTHING )
			{
				/*monsterSummonType = REVENANT_SKULL;
				summonCanEquipItems = false;
				summonedEntity = summonMonster(monsterSummonType, target->x, target->y, true);
				fellToDeath = false;
				fellInLava = false;
				fellInWater = false;*/
			}
			else if ( !summonedEntity && (fellToDeath || fellInLava) )
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
			if ( parent && parent->behavior == &actPlayer && customMonster == NOTHING )
			{
				if ( fellInWater )
				{
					messagePlayer(parent->skill[2], MESSAGE_STATUS, Language::get(3192)); // water make no work :<
				}
				else
				{
					messagePlayer(parent->skill[2], MESSAGE_STATUS, Language::get(3191)); // failed for some other reason
				}
			}
			return nullptr;
		}

		summonedEntity->seedEntityRNG(rng.getU32());

		Stat* summonedStats = summonedEntity->getStats();
		if ( !summonedStats )
		{
			if ( parent && parent->behavior == &actPlayer )
			{
				messagePlayer(parent->skill[2], MESSAGE_HINT, Language::get(3191));
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
		summonedStats->MISC_FLAGS[STAT_FLAG_MONSTER_DISABLE_HC_SCALING] = 1;
		summonedStats->leader_uid = targetStats->leader_uid;
		summonedStats->monsterIsCharmed = targetStats->monsterIsCharmed;
		summonedStats->setAttribute("DOMINATED_CREATURE", targetStats->getAttribute("DOMINATED_CREATURE"));
		Entity* leader = nullptr;
		if ( summonedStats->leader_uid != 0 && summonedStats->type != SHADOW )
		{
			leader = uidToEntity(summonedStats->leader_uid);
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
											if ( !players[c]->isLocalPlayer() )
											{
												serverRemoveClientFollower(c, target->getUID());
											}
											else
											{
												if ( FollowerMenu[c].recentEntity && (FollowerMenu[c].recentEntity->getUID() == 0
													|| FollowerMenu[c].recentEntity->getUID() == target->getUID()) )
												{
													FollowerMenu[c].recentEntity = nullptr;
												}
											}
											target->monsterAllyIndex = -1;
											if ( multiplayer == SERVER )
											{
												serverUpdateEntitySkill(target, 42); // update monsterAllyIndex for clients.
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
			}
		}

		if ( customMonster != NOTHING )
		{
			if ( parent )
			{
				if ( Stat* parentStats = parent->getStats() )
				{
					summonedStats->leader_uid = parent->getUID();
					leader = parent;
				}
			}
		}

		if ( leader && forceFollower(*leader, *summonedEntity) )
		{
			summonedEntity->monsterAllyIndex = -1;
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
		if ( targetStats->type == HUMAN && customMonster == NOTHING )
		{
			strcpy(summonedStats->name, targetStats->name);
		}

		if ( hitMonsterCanTransferEquipment && summonCanEquipItems )
		{
			// weapon
			Item** slot = itemSlot(targetStats, targetStats->weapon);
			if ( slot )
			{
				if ( targetStats->weapon && itemCategory(targetStats->weapon) == SPELLBOOK )
				{
					// spellbooks are not dropped
					if ( targetStats->weapon->node )
					{
						list_RemoveNode(targetStats->weapon->node);
					}
					else
					{
						free(targetStats->weapon);
					}
					targetStats->weapon = nullptr;
				}
				else
				{
					summonedStats->weapon = newItem((*slot)->type, (*slot)->status, (*slot)->beatitude,
						(*slot)->count, (*slot)->appearance, (*slot)->identified, nullptr);
				}
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
					if ( (*slot)->type == HAT_HOOD
						|| (*slot)->type == HAT_HOOD_SILVER
						|| (*slot)->type == HAT_HOOD_RED
						|| (*slot)->type == HAT_HOOD_APPRENTICE
						|| (*slot)->type == HAT_HOOD_WHISPERS
						|| (*slot)->type == HAT_HOOD_ASSASSIN )
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
			Entity* dropped = nullptr;
			if ( targetStats->weapon && itemCategory(targetStats->weapon) == SPELLBOOK )
			{
				// spellbooks are not dropped
				if ( targetStats->weapon->node )
				{
					list_RemoveNode(targetStats->weapon->node);
				}
				else
				{
					free(targetStats->weapon);
				}
				targetStats->weapon = nullptr;
			}
			else
			{
				dropped = dropItemMonster(targetStats->weapon, target, targetStats);
				if ( dropped )
				{
					dropped->flags[USERFLAG1] = true;
				}
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
			if ( item && item->appearance != MONSTER_ITEM_UNDROPPABLE_APPEARANCE 
				&& item->isDroppable
				&& itemSlot(targetStats, item) == nullptr )
			{
				if ( targetStats->type != SHOPKEEPER || (targetStats->type == SHOPKEEPER && local_rng.rand() % 2) )
				{
					Item* copiedItem = newItem(item->type, item->status, item->beatitude, item->count, item->appearance, item->identified, &summonedStats->inventory);
				}
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
			Uint32 color = makeColorRGB(0, 255, 0);
			bool namedMonsterAsGeneric = monsterNameIsGeneric(*targetStats);
			// the %s polymorph into a %s!
			if ( customMonster != NOTHING )
			{

			}
			else if ( !strcmp((*targetStats).name, "") || namedMonsterAsGeneric )
			{
				messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, color, Language::get(3187), getMonsterLocalizedName((*targetStats).type).c_str(), getMonsterLocalizedName(summonedStats->type).c_str());
			}
			else
			{
				messagePlayerColor(parent->skill[2], MESSAGE_COMBAT, color, Language::get(3188), (*targetStats).name, getMonsterLocalizedName(summonedStats->type).c_str());
			}
		}

		if ( customMonster == NOTHING )
		{
			playSoundEntity(target, 400, 92);
			spawnExplosion(target->x, target->y, target->z);
			createParticleDropRising(target, 593, 1.f);
			serverSpawnMiscParticles(target, PARTICLE_EFFECT_RISING_DROP, 593);
		}

		if ( fellToDeath )
		{
			summonedEntity->setObituary(Language::get(3010)); // fell to their death.
			summonedStats->HP = 0; // kill me instantly
			summonedStats->killer = KilledBy::BOTTOMLESS_PIT;
		}
		else if ( fellInLava )
		{
			summonedEntity->setObituary(Language::get(1506)); // goes for a swim in some lava.
			summonedStats->HP = 0; // kill me instantly
			summonedStats->killer = KilledBy::LAVA;
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

			if ( targetStats->playerRace == RACE_HUMAN || (targetStats->playerRace != RACE_HUMAN && targetStats->stat_appearance != 0) )
			{
				std::vector<int> chances =
				{
					RACE_SKELETON,
					RACE_VAMPIRE,
					RACE_SUCCUBUS,
					RACE_GOATMAN,
					RACE_AUTOMATON,
					RACE_INCUBUS,
					RACE_GOBLIN,
					RACE_INSECTOID,
					RACE_GNOME,
					RACE_GREMLIN,
					RACE_DRYAD,
					RACE_MYCONID,
					RACE_SALAMANDER
				};
				int roll = chances[local_rng.rand() % chances.size()];
				
				if ( target->effectPolymorph == 0 )
				{
					target->effectPolymorph = target->getMonsterFromPlayerRace(roll);
				}
				else
				{
					while ( target->effectPolymorph == target->getMonsterFromPlayerRace(roll) )
					{
						roll = chances[local_rng.rand() % chances.size()]; // re roll to not polymorph into the same thing
					}
					target->effectPolymorph = target->getMonsterFromPlayerRace(roll);
				}
			}
			else if ( (targetStats->playerRace != RACE_HUMAN && targetStats->stat_appearance == 0) )
			{
				target->effectPolymorph = 100 + local_rng.rand() % NUMAPPEARANCES;
			}
			serverUpdateEntitySkill(target, 50);

			Uint32 color = makeColorRGB(0, 255, 0);
			Monster race = NOTHING;
			if ( target->effectPolymorph > NUMMONSTERS )
			{
				race = HUMAN;
			}
			else
			{
				race = static_cast<Monster>(target->effectPolymorph);
			}
			messagePlayerColor(target->skill[2], MESSAGE_COMBAT, color, Language::get(3186), getMonsterLocalizedName(race).c_str());

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
			messagePlayer(target->skill[2], MESSAGE_STATUS | MESSAGE_HINT, Language::get(3189));
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

		if ( (target->behavior == &actMonster && !target->isInertMimic()) || target->behavior == &actPlayer
			/*|| target->behavior == &actDoor || target->behavior == &actChest*/ )
		{
			Stat* hitstats = target->getStats();
			if ( hitstats )
			{
				//if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
				//{
				//	// test for friendly fire
				//	if ( parent && parent->checkFriend(target) && parent->friendlyFireProtection(target) )
				//	{
				//		return false;
				//	}
				//}
			}
			//playSoundEntity(target, 249, 64);

			if ( parent )
			{
				if ( target->behavior == &actPlayer )
				{
					if ( MFLAG_DISABLETELEPORT )
					{
						// can't teleport here.
						Uint32 color = makeColorRGB(255, 0, 255);
						messagePlayerColor(target->skill[2], MESSAGE_STATUS, color, Language::get(2381));
						if ( parent->behavior == &actPlayer )
						{
							messagePlayerColor(parent->skill[2], MESSAGE_HINT, color, Language::get(3452));
						}
						return false;
					}
				}
				if ( target->behavior == &actMonster && target->isBossMonster() )
				{
					if ( parent->behavior == &actPlayer )
					{
						Uint32 color = makeColorRGB(255, 0, 0);
						if ( hitstats )
						{
							messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(2905), Language::get(2906), MSG_COMBAT);
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
							messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(3453));
						}
						return false;
					}

					if ( !spotsWithLineOfSight.empty() )
					{
						std::pair<int, int> tmpPair = spotsWithLineOfSight[local_rng.rand() % spotsWithLineOfSight.size()];
						tx = tmpPair.first;
						ty = tmpPair.second;
					}
					else if ( !goodspots.empty() )
					{
						std::pair<int, int> tmpPair = goodspots[local_rng.rand() % goodspots.size()];
						tx = tmpPair.first;
						ty = tmpPair.second;
					}
					else
					{
						if ( parent->behavior == &actPlayer )
						{
							// no room to teleport!
							messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(3453));
						}
						return false;
					}
				}

				// this timer is the entity spawn location.
				Entity* locationTimer = createParticleTimer(parent, 40, 593); 
				locationTimer->x = tx * 16.0 + 8;
				locationTimer->y = ty * 16.0 + 8;
				locationTimer->z = 0;
				locationTimer->particleTimerCountdownAction = PARTICLE_TIMER_TELEPORT_PULL_TARGET_LOCATION;
				locationTimer->particleTimerCountdownSprite = 593;
				locationTimer->particleTimerTarget = static_cast<Sint32>(target->getUID()); // get the target to teleport around.
				locationTimer->particleTimerEndAction = PARTICLE_EFFECT_TELEPORT_PULL; // teleport behavior of timer.
				locationTimer->particleTimerEndSprite = 593; // sprite to use for end of timer function.
				locationTimer->flags[PASSABLE] = false; // so this location is reserved for teleporting the entity.
				locationTimer->sizex = 4;
				locationTimer->sizey = 4;
				if ( !locationTimer->myTileListNode )
				{
					locationTimer->setUID(-2); // to avoid being excluded by TileEntityList
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

				Uint32 color = makeColorRGB(0, 255, 0);
				if ( parent->behavior == &actPlayer )
				{
					// play a sound for the player to confirm the hit.
					playSoundPlayer(parent->skill[2], 251, 128);
				}

				// update enemy bar for attacker
				/*if ( hitstats )
				{
					if ( !strcmp(hitstats->name, "") )
					{
						updateEnemyBar(parent, target, getMonsterLocalizedName(hitstats->type).c_str(), hitstats->HP, hitstats->MAXHP);
					}
					else
					{
						updateEnemyBar(parent, target, hitstats->name, hitstats->HP, hitstats->MAXHP);
					}
				}*/
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
		if ( (hit.entity->behavior == &actMonster && !hit.entity->isInertMimic()) || hit.entity->behavior == &actPlayer )
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
					hit.entity->setEffect(EFF_SHADOW_TAGGED, true, element.duration, true);
				}
				magicOnEntityHit(parent, &my, hit.entity, hitstats, 0, 0, 0, SPELL_SHADOW_TAG);
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
				Uint32 color = makeColorRGB(0, 255, 0);
				if ( parent->behavior == &actPlayer )
				{
					messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(3463), Language::get(3464), MSG_COMBAT);
				}
			}

			// update enemy bar for attacker
			/*if ( !strcmp(hitstats->name, "") )
			{
				updateEnemyBar(parent, hit.entity, getMonsterLocalizedName(hitstats->type).c_str(), hitstats->HP, hitstats->MAXHP);
			}
			else
			{
				updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP);
			}*/

			Uint32 color = makeColorRGB(255, 0, 0);
			int player = -1;
			if ( hit.entity->behavior == &actPlayer )
			{
				player = hit.entity->skill[2];
				if ( player >= 0 )
				{
					messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(3465));
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
		if ( (target->behavior == &actMonster && !target->isInertMimic()) || target->behavior == &actPlayer )
		{
			Stat* hitstats = target->getStats();
			if ( !hitstats )
			{
				return false;
			}

			if ( hitstats->type == INCUBUS || hitstats->type == SUCCUBUS 
				|| hitstats->type == AUTOMATON || hitstats->type == DEVIL || hitstats->type == DEMON || hitstats->type == CREATURE_IMP
				|| hitstats->type == SHADOW
				|| hitstats->type == MIMIC
				|| hitstats->type == MINIMIMIC
				|| hitstats->type == MONSTER_ADORCISED_WEAPON
				|| hitstats->type == MOTH_SMALL
				|| hitstats->type == REVENANT_SKULL
				|| hitstats->type == FLAME_ELEMENTAL
				|| hitstats->type == EARTH_ELEMENTAL
				|| (hitstats->type == SKELETON && hitstats->getAttribute("revenant_skeleton") != "")
				|| (hitstats->type == INCUBUS && !strncmp(hitstats->name, "inner demon", strlen("inner demon"))) )
			{
				if ( parent && parent->behavior == &actPlayer )
				{
					// unable to taunt!
					Uint32 color = makeColorRGB(255, 255, 255);
					messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(3472), Language::get(3473), MSG_COMBAT);
				}
				return false;
			}
			else if ( hitstats->monsterDemonHasBeenExorcised != 0 
				&& target->behavior != &actPlayer )
			{
				if ( parent && parent->behavior == &actPlayer )
				{
					// already exorcised!
					Uint32 color = makeColorRGB(255, 255, 255);
					messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(3735), Language::get(3736), MSG_COMBAT);
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
						messagePlayer(parent->skill[2], MESSAGE_MISC, Language::get(3471));
					}
					return false;
				}
				std::pair<int, int> tmpPair = goodspots[local_rng.rand() % goodspots.size()];
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
							Uint32 color = makeColorRGB(255, 255, 0);
							messagePlayerColor(parent->skill[2], MESSAGE_STATUS, color, Language::get(621));
						}
						parent->modHP(-(parentStats->MAXHP / 10));
						if ( parentStats->sex == MALE )
						{
							parent->setObituary(Language::get(1528));
							parentStats->killer = KilledBy::FAILED_INVOCATION;
						}
						else
						{
							parent->setObituary(Language::get(1529));
							parentStats->killer = KilledBy::FAILED_INVOCATION;
						}
					}

					hitstats->monsterDemonHasBeenExorcised++;

					// hit messages
					Uint32 color = makeColorRGB(0, 255, 0);
					if ( parent->behavior == &actPlayer )
					{
						messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(3469), Language::get(3470), MSG_COMBAT);
					}
				}
			}

			Uint32 color = makeColorRGB(255, 0, 0);
			int player = -1;
			if ( target->behavior == &actPlayer )
			{
				player = target->skill[2];
				if ( player >= 0 )
				{
					messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(3468));
					if ( hitstats->monsterDemonHasBeenExorcised == 3 )
					{
						Uint32 color = makeColorRGB(0, 255, 0);
						messagePlayerColor(player, MESSAGE_COMBAT, color, Language::get(3737));
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

Entity* spellEffectHologram(Entity& caster, spellElement_t& element, real_t x, real_t y)
{
	Entity* monster = nullptr;
	{
		// try find a summon location around the entity.
		int tx = static_cast<int>(std::floor(x)) >> 4;
		int ty = static_cast<int>(std::floor(y)) >> 4;
		int dist = 1;
		std::vector<std::pair<int, int>> goodspots;
		for ( int iy = std::max(1, ty - dist); iy < std::min(ty + dist, static_cast<int>(map.height)); ++iy )
		{
			for ( int ix = std::max(1, tx - dist); ix < std::min(tx + dist, static_cast<int>(map.width)); ++ix )
			{
				if ( !checkObstacle((ix << 4) + 8, (iy << 4) + 8, &caster, NULL, true, true, false) )
				{
					goodspots.push_back(std::make_pair(ix, iy));
				}
			}
		}
		if ( goodspots.size() == 0 )
		{
			return nullptr;
		}
		else
		{
			if ( !checkObstacle((tx << 4) + 8, (ty << 4) + 8, &caster, NULL, true, true, false, false) )
			{
				monster = summonMonster(HOLOGRAM, tx * 16.0 + 8, ty * 16.0 + 8, true);
			}
			while ( !monster && goodspots.size() )
			{
				int pick = local_rng.rand() % goodspots.size();
				std::pair<int, int> tmpPair = goodspots[pick];
				tx = tmpPair.first;
				ty = tmpPair.second;
				goodspots.erase(goodspots.begin() + pick);
				monster = summonMonster(HOLOGRAM, tx * 16.0 + 8, ty * 16.0 + 8, true);
			}

			if ( monster )
			{
				playSoundEntity(monster, 166, 128);
				//playSoundEntity(&my, 178, 128);
				createParticleErupt(monster, 983);
				serverSpawnMiscParticles(monster, PARTICLE_EFFECT_ERUPT, 983);

				if ( Stat* monsterStats = monster->getStats() )
				{
					monsterStats->monsterNoDropItems = 1;
					monsterStats->leader_uid = caster.getUID();
					monster->parent = caster.getUID();
					monster->setEffect(EFF_STUNNED, true, -1, false);
				}
			}
		}
	}
	return monster;
}

Entity* spellEffectAdorcise(Entity& caster, spellElement_t& element, real_t x, real_t y, Item* itemToAdorcise)
{
	Entity* monster = nullptr;
	{
		// try find a summon location around the entity.
		int tx = static_cast<int>(std::floor(x)) >> 4;
		int ty = static_cast<int>(std::floor(y)) >> 4;
		int dist = 1;
		std::vector<std::pair<int, int>> goodspots;
		for ( int iy = std::max(1, ty - dist); iy < std::min(ty + dist, static_cast<int>(map.height)); ++iy )
		{
			for ( int ix = std::max(1, tx - dist); ix < std::min(tx + dist, static_cast<int>(map.width)); ++ix )
			{
				if ( !checkObstacle((ix << 4) + 8, (iy << 4) + 8, &caster, NULL, true, true, false) )
				{
					goodspots.push_back(std::make_pair(ix, iy));
				}
			}
		}
		if ( goodspots.size() == 0 )
		{
			return nullptr;
		}
		else
		{
			if ( !checkObstacle((tx << 4) + 8, (ty << 4) + 8, &caster, NULL, true, true, false, false) )
			{
				monster = summonMonster(MONSTER_ADORCISED_WEAPON, tx * 16.0 + 8, ty * 16.0 + 8, true);
			}
			while ( !monster && goodspots.size() )
			{
				int pick = local_rng.rand() % goodspots.size();
				std::pair<int, int> tmpPair = goodspots[pick];
				tx = tmpPair.first;
				ty = tmpPair.second;
				goodspots.erase(goodspots.begin() + pick);
				monster = summonMonster(MONSTER_ADORCISED_WEAPON, tx * 16.0 + 8, ty * 16.0 + 8, true);
			}

			if ( monster )
			{
				playSoundEntity(monster, 171, 128);
				//playSoundEntity(&my, 178, 128);
				createParticleErupt(monster, 983);
				serverSpawnMiscParticles(monster, PARTICLE_EFFECT_ERUPT, 983);


				Stat* monsterStats = monster->getStats();
				if ( monsterStats )
				{
					if ( &element == &spellElementMap[SPELL_SPIRIT_WEAPON] )
					{
						int duration = getSpellEffectDurationFromID(SPELL_SPIRIT_WEAPON, &caster, nullptr, &caster);
						monsterStats->setAttribute("spirit_weapon", std::to_string(duration));
						monsterStats->MISC_FLAGS[STAT_FLAG_MONSTER_DISABLE_HC_SCALING] = 1;
						monster->setEffect(EFF_ROOTED, true, -1, false);

						ItemType type = IRON_SWORD;
						switch ( local_rng.rand() % 4 )
						{
						case 1:
							type = IRON_SPEAR;
							break;
						case 2:
							type = IRON_MACE;
							break;
						case 3:
							type = IRON_AXE;
							break;
						case 0:
						default:
							type = IRON_SWORD;
							break;
						}

						monsterStats->STR = getSpellDamageFromID(SPELL_SPIRIT_WEAPON, &caster, nullptr, &caster);
						monsterStats->MAXHP = getSpellDamageSecondaryFromID(SPELL_SPIRIT_WEAPON, &caster, nullptr, &caster);
						monsterStats->HP = monsterStats->MAXHP;
						monsterStats->OLDHP = monsterStats->MAXHP;

						if ( monsterStats->weapon = newItem(type, EXCELLENT, 0, 1, local_rng.rand(), true, nullptr) )
						{
							monsterStats->weapon->isDroppable = false;
						}
						monsterStats->monsterNoDropItems = 1;
						monsterStats->leader_uid = caster.getUID();
						monster->parent = caster.getUID();

						monster->monsterHitTime = HITRATE * 1.5 - 10;
						const real_t lookDist = 40.0;
						real_t dist = lookDist;
						Entity* newTarget = nullptr;
						for ( node_t* node = map.creatures->first; node != nullptr; node = node->next )
						{
							Entity* target = (Entity*)node->element;
							if ( target->behavior == &actMonster && monster->checkEnemy(target) )
							{
								real_t oldDist = dist;
								dist = sqrt(pow(monster->x - target->x, 2) + pow(monster->y - target->y, 2));
								if ( dist < lookDist && dist <= oldDist )
								{
									double tangent = atan2(target->y - monster->y, target->x - monster->x);
									lineTrace(monster, monster->x, monster->y, tangent, lookDist, 0, false);
									if ( hit.entity == target )
									{
										newTarget = target;
									}
								}
							}
						}

						if ( newTarget )
						{
							monster->lookAtEntity(*newTarget);
							if ( entityDist(monster, newTarget) < TOUCHRANGE )
							{
								monster->monsterAcquireAttackTarget(*newTarget, MONSTER_STATE_ATTACK);
							}
						}

					}
					else if ( &element == &spellElementMap[SPELL_ADORCISM] )
					{
						int duration = getSpellEffectDurationFromID(SPELL_ADORCISM, &caster, nullptr, &caster);
						monsterStats->setAttribute("adorcised_weapon", std::to_string(duration));
						monsterStats->MISC_FLAGS[STAT_FLAG_MONSTER_DISABLE_HC_SCALING] = 1;
						if ( itemToAdorcise )
						{
							monsterStats->weapon = newItem(itemToAdorcise->type, itemToAdorcise->status,
								itemToAdorcise->beatitude, 1, itemToAdorcise->appearance, itemToAdorcise->identified, nullptr);
							if ( caster.behavior == &actPlayer )
							{
								messagePlayer(caster.skill[2], MESSAGE_INVENTORY, Language::get(6577), itemToAdorcise->getName());
							}
						}
						else
						{
							ItemType type = IRON_SWORD;
							switch ( local_rng.rand() % 4 )
							{
							case 1:
								type = IRON_SPEAR;
								break;
							case 2:
								type = IRON_MACE;
								break;
							case 3:
								type = IRON_AXE;
								break;
							case 0:
							default:
								type = IRON_SWORD;
								break;
							}

							monsterStats->STR = getSpellDamageFromID(SPELL_ADORCISM, &caster, nullptr, &caster);
							monsterStats->MAXHP = getSpellDamageSecondaryFromID(SPELL_ADORCISM, &caster, nullptr, &caster);
							monsterStats->HP = monsterStats->MAXHP;
							monsterStats->OLDHP = monsterStats->MAXHP;

							if ( monsterStats->weapon = newItem(type, EXCELLENT, 0, 1, local_rng.rand(), true, nullptr) )
							{
								monsterStats->weapon->isDroppable = false;
							}
							monsterStats->monsterNoDropItems = 1;
						}
						if ( forceFollower(caster, *monster) )
						{
							if ( caster.behavior == &actPlayer )
							{
								Compendium_t::Events_t::eventUpdateMonster(caster.skill[2], Compendium_t::CPDM_RECRUITED, monster, 1);
								monster->monsterAllyIndex = caster.skill[2];
								if ( multiplayer == SERVER )
								{
									serverUpdateEntitySkill(monster, 42); // update monsterAllyIndex for clients.
								}
							}
						}
					}

					if ( caster.behavior == &actPlayer )
					{
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
				}
			}
		}
	}
	return monster;
}

Entity* spellEffectFlameSprite(Entity& caster, spellElement_t& element, real_t x, real_t y)
{
	Entity* monster = nullptr;
	{
		// try find a summon location around the entity.
		int tx = static_cast<int>(std::floor(x)) >> 4;
		int ty = static_cast<int>(std::floor(y)) >> 4;
		int dist = 1;
		std::vector<std::pair<int, int>> goodspots;
		for ( int iy = std::max(1, ty - dist); iy < std::min(ty + dist, static_cast<int>(map.height)); ++iy )
		{
			for ( int ix = std::max(1, tx - dist); ix < std::min(tx + dist, static_cast<int>(map.width)); ++ix )
			{
				if ( !checkObstacle((ix << 4) + 8, (iy << 4) + 8, &caster, NULL, true, true, false) )
				{
					goodspots.push_back(std::make_pair(ix, iy));
				}
			}
		}
		if ( goodspots.size() == 0 )
		{
			return nullptr;
		}
		else
		{
			Monster type = &element == &spellElementMap[SPELL_FIRE_SPRITE] ? MOTH_SMALL : FLAME_ELEMENTAL;
			if ( !checkObstacle((tx << 4) + 8, (ty << 4) + 8, &caster, NULL, true, true, false, false) )
			{
				monster = summonMonster(type, tx * 16.0 + 8, ty * 16.0 + 8, true);
			}
			while ( !monster && goodspots.size() )
			{
				int pick = local_rng.rand() % goodspots.size();
				std::pair<int, int> tmpPair = goodspots[pick];
				tx = tmpPair.first;
				ty = tmpPair.second;
				goodspots.erase(goodspots.begin() + pick);
				monster = summonMonster(type, tx * 16.0 + 8, ty * 16.0 + 8, true);
			}

			if ( monster )
			{
				playSoundEntity(monster, 164, 128);
				//playSoundEntity(&my, 178, 128);
				//createParticleErupt(monster, 983);
				//serverSpawnMiscParticles(monster, PARTICLE_EFFECT_ERUPT, 983);
				spawnMagicEffectParticles(monster->x, monster->y, monster->z, 2207);
				for ( int i = 0; i < 3; ++i )
				{
					if ( Entity* fx = createParticleAestheticOrbit(monster, 233, TICKS_PER_SECOND, PARTICLE_EFFECT_IGNITE_ORBIT) )
					{
						fx->flags[SPRITE] = true;
						fx->x = monster->x;
						fx->y = monster->y;
						fx->fskill[0] = fx->x;
						fx->fskill[1] = fx->y;
						fx->z = -7.5;
						fx->vel_z = 0.25;
						fx->actmagicOrbitDist = 4;
						fx->fskill[2] = monster->yaw + (i) * 2 * PI / 3.0;
						fx->yaw = fx->fskill[2];
						fx->actmagicNoLight = 1;

					}
				}
				serverSpawnMiscParticles(monster, PARTICLE_EFFECT_SUMMON_FLAMES, 233, 0, TICKS_PER_SECOND);

				Stat* monsterStats = monster->getStats();
				if ( monsterStats )
				{
					if ( &element == &spellElementMap[SPELL_FIRE_SPRITE] )
					{
						int duration = getSpellEffectDurationSecondaryFromID(SPELL_FIRE_SPRITE, &caster, nullptr, &caster);
						monsterStats->setAttribute("fire_sprite", std::to_string(duration));
						monsterStats->monsterNoDropItems = 1;
						monsterStats->MISC_FLAGS[STAT_FLAG_MONSTER_DISABLE_HC_SCALING] = 1;
						
						monsterStats->setAttribute("special_npc", "fire sprite");
						strcpy(monsterStats->name, MonsterData_t::getSpecialNPCName(*monsterStats).c_str());

						int lvl = getSpellDamageFromID(SPELL_FIRE_SPRITE, &caster, nullptr, &caster);
						int maxlvl = getSpellDamageSecondaryFromID(SPELL_FIRE_SPRITE, &caster, nullptr, &caster);
						lvl = std::min(lvl, maxlvl);
						monsterStats->LVL = lvl;

						if ( forceFollower(caster, *monster) )
						{
							if ( caster.behavior == &actPlayer )
							{
								Compendium_t::Events_t::eventUpdateMonster(caster.skill[2], Compendium_t::CPDM_RECRUITED, monster, 1);
								monster->monsterAllyIndex = caster.skill[2];
								if ( multiplayer == SERVER )
								{
									serverUpdateEntitySkill(monster, 42); // update monsterAllyIndex for clients.
								}
							}
						}
					}
					else if ( &element == &spellElementMap[SPELL_FLAME_ELEMENTAL] )
					{
						int duration = getSpellEffectDurationSecondaryFromID(SPELL_FLAME_ELEMENTAL, &caster, nullptr, &caster);
						monsterStats->setAttribute("flame_elemental", std::to_string(duration));
						monsterStats->monsterNoDropItems = 1;
						monsterStats->MISC_FLAGS[STAT_FLAG_MONSTER_DISABLE_HC_SCALING] = 1;

						int lvl = getSpellDamageFromID(SPELL_FLAME_ELEMENTAL, &caster, nullptr, &caster);
						int maxlvl = getSpellDamageSecondaryFromID(SPELL_FLAME_ELEMENTAL, &caster, nullptr, &caster);
						lvl = std::min(lvl, maxlvl);
						monsterStats->LVL = lvl;

						if ( forceFollower(caster, *monster) )
						{
							if ( caster.behavior == &actPlayer )
							{
								Compendium_t::Events_t::eventUpdateMonster(caster.skill[2], Compendium_t::CPDM_RECRUITED, monster, 1);
								monster->monsterAllyIndex = caster.skill[2];
								if ( multiplayer == SERVER )
								{
									serverUpdateEntitySkill(monster, 42); // update monsterAllyIndex for clients.
								}
							}
						}
					}

					if ( caster.behavior == &actPlayer )
					{
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
				}
			}
		}
	}
	return monster;
}

bool Entity::spellEffectPreserveItem(Item* item)
{
	if ( !item ) { return false; }
	if ( Stat* myStats = getStats() )
	{
		if ( behavior != &actPlayer )
		{
			return false;
		}
		if ( !itemIsEquipped(item, skill[2]) )
		{
			return false;
		}
		if ( myStats->getEffectActive(EFF_PRESERVE) )
		{
			if ( spell_t* preserveSpell = getActiveMagicEffect(SPELL_PRESERVE) )
			{
				int cost = getSpellDamageFromID(SPELL_PRESERVE, this, nullptr, this);
				cost = std::max(1, std::max(getSpellDamageSecondaryFromID(SPELL_PRESERVE, this, nullptr, this), cost));
				if ( item->type == AMULET_LIFESAVING )
				{
					cost *= 10;
				}
				if ( item->type == AMULET_MAGICREFLECTION || item->type == CLOAK_MAGICREFLECTION )
				{
					cost *= 2;
				}
				if ( !safeConsumeMP(cost) )
				{
					if ( myStats->MP > 0 )
					{
						modMP(-myStats->MP);
					}
					preserveSpell->sustain = false;
					if ( behavior == &actPlayer )
					{
						playSoundEntity(this, 163, 128);
						messagePlayerColor(skill[2], MESSAGE_COMBAT, makeColorRGB(255, 0, 0), Language::get(6657), item->getName());
					}
					return false;
				}
				else
				{
					if ( behavior == &actPlayer )
					{
						spawnMagicEffectParticles(this->x, this->y, this->z / 2, 174);
						playSoundEntity(this, 166, 128);
						messagePlayerColor(skill[2], MESSAGE_COMBAT, makeColorRGB(0, 255, 0), Language::get(6654), item->getName());

						players[skill[2]]->mechanics.updateSustainedSpellEvent(SPELL_PRESERVE, 10.0, 1.0, nullptr);
					}
					return true;
				}
			}
		}
	}
	return false;
}

int thaumSpellArmorProc(Entity* my, Stat& myStats, bool checkEffectActiveOnly, Entity* attacker, int effectID)
{
	int player = -1;
	if ( my && my->behavior == &actPlayer )
	{
		player = my->skill[2];
	}
	if ( !my && player == -1 )
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( stats[i] == &myStats )
			{
				player = i;
				break;
			}
		}
	}

	int spellID = -1;
	if ( effectID == EFF_GUARD_BODY )
	{
		spellID = SPELL_GUARD_BODY;
	}
	else if ( effectID == EFF_GUARD_SPIRIT )
	{
		spellID = SPELL_GUARD_SPIRIT;
	}
	else if ( effectID == EFF_DIVINE_GUARD )
	{
		spellID = SPELL_DIVINE_GUARD;
	}

	if ( spellID < 0 ) { return 0; }

	if ( myStats.getEffectActive(effectID) )
	{
		if ( player >= 0 )
		{
			int result = myStats.getEffectActive(effectID);

			if ( !checkEffectActiveOnly )
			{
				if ( my )
				{
					int baseMinValue = (effectID == EFF_GUARD_SPIRIT) ? 1 : 3;
					//int minValue = std::max(baseMinValue, getSpellDamageFromID(spellID, my, nullptr, my));
					//minValue = std::min(minValue, getSpellEffectDurationSecondaryFromID(spellID, my, nullptr, my));
					my->setEffect(effectID, (Uint8)std::max(baseMinValue, myStats.getEffectActive(effectID) - 1),
						myStats.EFFECTS_TIMERS[effectID], false, true, true);
					if ( my->getActiveMagicEffect(spellID) )
					{
						if ( result != myStats.getEffectActive(effectID) )
						{
							players[player]->mechanics.updateSustainedSpellEvent(spellID, std::min(150.0, effectID == EFF_GUARD_SPIRIT ? 128.0 : 50.0 + 10 * result), 1.0, attacker);
						}
						else
						{
							players[player]->mechanics.updateSustainedSpellEvent(spellID, std::min(150.0, effectID == EFF_GUARD_SPIRIT ? 128.0 : 50.0 + 10 * result) / 10.0, 1.0, attacker);
						}
					}
				}
			}
			return result;
		}
		else
		{
			if ( !checkEffectActiveOnly )
			{
				if ( my && my->behavior == &actMonster )
				{
					myStats.EFFECTS_TIMERS[effectID] = std::max(1, myStats.EFFECTS_TIMERS[effectID] - TICKS_PER_SECOND);
				}
			}
			return myStats.getEffectActive(effectID);
		}
	}

	return 0;
}

bool Entity::pinpointDamageProc(Entity* attacker, int damage)
{
	if ( multiplayer == CLIENT || !attacker ) { return false; }
	if ( Stat* myStats = getStats() )
	{
		if ( myStats->HP == 0 ) { return false; }
		if ( !(attacker->behavior == &actPlayer || (attacker->behavior == &actMonster && attacker->monsterAllyGetPlayerLeader())) )
		{
			return false;
		}

		if ( myStats->getEffectActive(EFF_PINPOINT) || myStats->getEffectActive(EFF_PINPOINT_DAMAGE) )
		{
			if ( damage > 0 )
			{
				// find particle to update
				bool found = false;
				auto entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(this, 1);
				for ( auto it : entLists )
				{
					node_t* node;
					for ( node = it->first; node != nullptr && !found; node = node->next )
					{
						if ( Entity* entity = (Entity*)node->element )
						{
							if ( entity->behavior == &actParticleAestheticOrbit
								&& entity->parent == this->getUID()
								&& entity->skill[1] == PARTICLE_EFFECT_SMITE_PINPOINT
								&& entity->actmagicNoLight == 0 )
							{
								Entity* caster = uidToEntity(entity->skill[3]);
								real_t damageMult = getSpellDamageSecondaryFromID(SPELL_PINPOINT, caster, caster ? caster->getStats() : nullptr,
									entity, entity->actmagicSpellbookBonus / 100.0) / 100.0;
								entity->skill[4] += std::max(0, (damage)) * damageMult;
								found = true;
								break;
							}
							else if ( entity->behavior == &actParticlePinpointTarget
								&& entity->skill[4] == SPELL_PINPOINT
								&& entity->parent == this->getUID()
								&& entity->skill[0] >= 0 )
							{
								Uint32 casterUid = static_cast<Uint32>(entity->skill[2]);
								Entity* caster = uidToEntity(casterUid);

								for ( int i = 0; i < 3; ++i )
								{
									Entity* fx1 = createParticleAestheticOrbit(this, 2401, 2 * TICKS_PER_SECOND, PARTICLE_EFFECT_SMITE_PINPOINT);
									fx1->yaw = this->yaw + PI / 2 + 2 * i * PI / 3;
									fx1->fskill[4] = this->x;
									fx1->fskill[5] = this->y;
									fx1->x = this->x;
									fx1->y = this->y;
									fx1->fskill[6] = fx1->yaw;
									fx1->skill[3] = caster ? caster->getUID() : 0;
									if ( i != 0 )
									{
										fx1->actmagicNoLight = 1;
									}
									if ( i == 0 )
									{
										fx1->actmagicSpellbookBonus = entity->actmagicSpellbookBonus;
										real_t damageMult = getSpellDamageSecondaryFromID(SPELL_PINPOINT, caster, caster ? caster->getStats() : nullptr,
											entity, entity->actmagicSpellbookBonus / 100.0) / 100.0;
										fx1->skill[4] += std::max(0, (damage)) * damageMult;
										fx1->actmagicFromSpellbook = entity->actmagicFromSpellbook;
									}
								}

								setEffect(EFF_PINPOINT_DAMAGE, true, 2 * TICKS_PER_SECOND, false);
								serverSpawnMiscParticles(this, PARTICLE_EFFECT_SMITE_PINPOINT, 2401, 0, 0);

								entity->skill[0] = -1; // expire this
								found = true;
								break;
							}
						}
					}
					if ( found )
					{
						break;
					}
				}

				if ( found )
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool Entity::defyFleshProc(Entity* attacker)
{
	if ( multiplayer == CLIENT ) { return false; }
	if ( Stat* myStats = getStats() )
	{
		if ( myStats->getEffectActive(EFF_DEFY_FLESH) )
		{
			// find particle to update
			auto entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(this, 1);
			for ( auto it : entLists )
			{
				node_t* node;
				for ( node = it->first; node != nullptr; node = node->next )
				{
					if ( Entity* entity = (Entity*)node->element )
					{
						if ( entity->behavior == &actParticleAestheticOrbit 
							&& entity->parent == this->getUID()
							&& entity->skill[1] == PARTICLE_EFFECT_DEFY_FLESH_ORBIT )
						{
							if ( entity->skill[8] == 0 ) // cooldown timer
							{
								entity->skill[7] = 1;
								Uint8 charges = myStats->getEffectActive(EFF_DEFY_FLESH) & 0xF;
								if ( charges == 0 )
								{
									setEffect(EFF_DEFY_FLESH, false, 0, true);
								}
								else
								{
									--charges;
									/*if ( charges == 0 )
									{
										setEffect(EFF_DEFY_FLESH, false, 0, true);
									}
									else*/
									{
										charges |= (myStats->getEffectActive(EFF_DEFY_FLESH) & 0xF0);
										setEffect(EFF_DEFY_FLESH, charges, myStats->EFFECTS_TIMERS[EFF_DEFY_FLESH], true, true, true);
									}
								}
							}
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

bool Entity::mistFormDodge(bool checkEffectActiveOnly, Entity* attacker)
{
	if ( Stat* myStats = getStats() )
	{
		if ( myStats->getEffectActive(EFF_MIST_FORM) )
		{
			if ( checkEffectActiveOnly )
			{
				return true;
			}

			if ( behavior == &actPlayer )
			{
				if ( spell_t* spell = getActiveMagicEffect(SPELL_MIST_FORM) )
				{
					int chance = getSpellEffectDurationSecondaryFromID(SPELL_MIST_FORM, this, nullptr, this);
					if ( local_rng.rand() % 100 < chance )
					{
						int cost = getSpellDamageFromID(SPELL_MIST_FORM, this, nullptr, this);
						cost = std::max(1, std::max(cost, getSpellDamageSecondaryFromID(SPELL_MIST_FORM, this, nullptr, this)));
						if ( !safeConsumeMP(cost) )
						{
							if ( myStats->MP > 0 )
							{
								modMP(-myStats->MP);
							}
							spell->sustain = false;
							if ( behavior == &actPlayer )
							{
								playSoundEntity(this, 163, 128);
								messagePlayerColor(skill[2], MESSAGE_COMBAT, makeColorRGB(255, 0, 0), Language::get(6665));
							}
							return false;
						}
						else
						{
							if ( behavior == &actPlayer )
							{
								spawnPoof(this->x, this->y, this->z / 2, 1.0, true);
								playSoundEntity(this, 166, 128);
								messagePlayerColor(skill[2], MESSAGE_COMBAT, makeColorRGB(0, 255, 0), Language::get(6666));
								magicOnSpellCastEvent(this, this, attacker, SPELL_MIST_FORM, spell_t::SPELL_LEVEL_EVENT_DEFAULT | spell_t::SPELL_LEVEL_EVENT_MINOR_CHANCE, 1);
							}
							return true;
						}
					}
					else
					{
						return false;
					}
				}
			}
			else
			{
				if ( behavior == &actMonster )
				{
					myStats->EFFECTS_TIMERS[EFF_MIST_FORM] = std::max(1, myStats->EFFECTS_TIMERS[EFF_MIST_FORM] - TICKS_PER_SECOND);
				}
				return true;
			}
		}
	}
	return false;
}

bool applyGenericMagicDamage(Entity* caster, Entity* hitentity, Entity& damageSourceProjectile, int spellID, 
	int damage, bool alertMonsters, bool monsterCollisionOnly, int usingSpellbookID)
{
	if ( !hitentity )
	{
		return false;
	}

	int trapResist = 0;
	int resistance = 0;
	DamageGib dmgGib = DMG_DEFAULT;
	real_t damageMultiplier = 1.0;
	magicSetResistance(hitentity, caster, resistance, damageMultiplier, dmgGib, trapResist, spellID);
	Stat* targetStats = hitentity->getStats();
	if ( monsterCollisionOnly )
	{
		if ( !targetStats )
		{
			return false;
		}
		if ( hitentity->isInertMimic() )
		{
			return false;
		}
	}

	Entity::modifyDamageMultipliersFromEffects(hitentity, caster, damageMultiplier, DAMAGE_TABLE_MAGIC, &damageSourceProjectile, spellID);

	if ( damageSourceProjectile.behavior == &actBoulder )
	{
		// pure dmg
		dmgGib = DMG_STRONGEST;
		damageMultiplier = 1.0;
		resistance = 0;
		trapResist = 0;
	}
	else if ( spellID == SPELL_EARTH_ELEMENTAL && damageSourceProjectile.behavior == &actMonster
		&& damageSourceProjectile.getMonsterTypeFromSprite() == EARTH_ELEMENTAL )
	{
		// pure dmg
		dmgGib = DMG_STRONGEST;
		damageMultiplier = 1.0;
		resistance = 0;
		trapResist = 0;
	}
	if ( spellID == SPELL_HOLY_FIRE )
	{
		// pure dmg
		dmgGib = DMG_STRONGER;
		damageMultiplier = 1.0;
		resistance = 0;
		trapResist = 0;
	}

	absorbMagicEvent(hitentity, caster, damageSourceProjectile, spellID, nullptr, damageMultiplier, dmgGib);

	if ( hitentity->behavior == &actChest || hitentity->isInertMimic() )
	{
		damage *= damageMultiplier;
		hitentity->chestHandleDamageMagic(damage, damageSourceProjectile, caster);
		return true;
	}
	else if ( (hitentity->behavior == &actMonster || hitentity->behavior == &actPlayer) && targetStats )
	{
		bool alertTarget = false;
		if ( alertMonsters && caster && caster != hitentity )
		{
			alertTarget = hitentity->monsterAlertBeforeHit(caster);

			// alert the monster!
			if ( hitentity->monsterState != MONSTER_STATE_ATTACK && (targetStats->type < LICH || targetStats->type >= SHOPKEEPER)
				&& targetStats->type != GYROBOT )
			{
				if ( alertTarget )
				{
					bool oldPassable = caster->flags[PASSABLE];
					if ( spellID == SPELL_EARTH_ELEMENTAL && caster->behavior == &actMonster
						&& caster->getMonsterTypeFromSprite() == EARTH_ELEMENTAL )
					{
						caster->flags[PASSABLE] = false; // let monsters aggro
					}
					hitentity->monsterAcquireAttackTarget(*caster, MONSTER_STATE_PATH, true);
					caster->flags[PASSABLE] = oldPassable;
				}
			}

			// alert other monsters too
			if ( alertTarget )
			{
				hitentity->alertAlliesOnBeingHit(caster);
			}
		}
		hitentity->updateEntityOnHit(caster, alertTarget);

		if ( spellID == SPELL_IGNITE 
			|| spellID == SPELL_DISARM 
			|| spellID == SPELL_STRIP
			|| spellID == SPELL_MAXIMISE
			|| spellID == SPELL_MINIMISE
			|| spellID == SPELL_COWARDICE
			|| spellID == SPELL_SEEK_ALLY
			|| spellID == SPELL_SEEK_FOE
			|| spellID == SPELL_COMMAND
			|| spellID == SPELL_CURSE_FLESH
			|| spellID == SPELL_REVENANT_CURSE )
		{
			// alert entities only
			return true;
		}

		playSoundEntity(hitentity, 28, 128);
		int oldHP = targetStats->HP;

		Sint32 preResistanceDamage = damage;
		damage *= damageMultiplier;

		if ( spellID == SPELL_ICE_WAVE )
		{
			real_t coldMultiplier = 1.0;
			if ( targetStats && targetStats->helmet && targetStats->helmet->type == HAT_WARM )
			{
				if ( !(hitentity->behavior == &actPlayer && hitentity->effectShapeshift != NOTHING) )
				{
					if ( targetStats->helmet->beatitude >= 0 || shouldInvertEquipmentBeatitude(targetStats) )
					{
						coldMultiplier = std::max(0.0, 0.5 - 0.25 * (abs(targetStats->helmet->beatitude)));
					}
					else
					{
						coldMultiplier = 0.50;
					}
				}
			}
			damage *= coldMultiplier;
		}
		else if ( spellID == SPELL_FIRE_WALL )
		{
			real_t fireMultiplier = 1.0;
			if ( targetStats && targetStats->type == DRYAD )
			{
				fireMultiplier += 0.2;
				if ( !targetStats->helmet && targetStats->getEffectActive(EFF_GROWTH) > 1 )
				{
					int bonus = std::min(3, targetStats->getEffectActive(EFF_GROWTH) - 1);
					fireMultiplier += 0.05;
				}
			}
			if ( targetStats->type == SALAMANDER )
			{
				if ( targetStats->getEffectActive(EFF_SALAMANDER_HEART) == 1
					|| targetStats->getEffectActive(EFF_SALAMANDER_HEART) == 2 )
				{
					fireMultiplier *= 0.25;
				}
				else
				{
					fireMultiplier *= 0.5;
				}
			}
			damage *= fireMultiplier;
		}
		if ( spellID == SPELL_DEFY_FLESH )
		{
			damage = std::max(1, damage);
		}

		hitentity->modHP(-damage);
		if ( damage > 0 )
		{
			Entity* gib = spawnGib(hitentity);
			serverSpawnGibForClient(gib);
		}
		magicOnEntityHit(caster, &damageSourceProjectile, hitentity, targetStats, preResistanceDamage, damage, oldHP, spellID, usingSpellbookID);
		magicTrapOnHit(caster, hitentity, targetStats, oldHP, spellID);

		// write the obituary
		if ( caster )
		{
			caster->killedByMonsterObituary(hitentity, true);
		}

		// update enemy bar for attacker
		if ( !strcmp(targetStats->name, "") )
		{
			updateEnemyBar(caster, hitentity, getMonsterLocalizedName(targetStats->type).c_str(), targetStats->HP, targetStats->MAXHP,
				false, dmgGib);
		}
		else
		{
			updateEnemyBar(caster, hitentity, targetStats->name, targetStats->HP, targetStats->MAXHP,
				false, dmgGib);
		}
		if ( oldHP > 0 && targetStats->HP <= 0 && caster )
		{
			bool xp = true;
			if ( damageSourceProjectile.behavior == &actBoulder )
			{
				if ( spellID == SPELL_NONE )
				{
					xp = false;
				}
			}
			if ( xp )
			{
				caster->awardXP(hitentity, true, true);
			}

			if ( spellID == SPELL_BOOBY_TRAP && caster->behavior == &actPlayer )
			{
				steamStatisticUpdateClient(caster->skill[2], STEAM_STAT_BOOM_DYNAMITE, STEAM_STAT_INT, 1);
			}

			spawnBloodVialOnMonsterDeath(hitentity, targetStats, caster);
		}

		if ( hitentity->behavior == &actPlayer )
		{
			if ( oldHP > targetStats->HP && targetStats->HP > 0 )
			{
				int strength = 10;
				if ( oldHP - targetStats->HP < 10 )
				{
					strength = 5;
				}
				int player = hitentity->skill[2];
				// entity took damage, shake screen.
				if ( multiplayer == SERVER && player > 0 )
				{
					strcpy((char*)net_packet->data, "SHAK");
					net_packet->data[4] = strength; // turns into .1
					net_packet->data[5] = strength;
					net_packet->address.host = net_clients[player - 1].host;
					net_packet->address.port = net_clients[player - 1].port;
					net_packet->len = 6;
					sendPacketSafe(net_sock, -1, net_packet, player - 1);
				}
				else if ( player == 0 || (splitscreen && player > 0) )
				{
					cameravars[player].shakex += strength * 0.01;
					cameravars[player].shakey += strength;
				}
			}
		}

		return true;
	}
	else if ( hitentity->behavior == &actDoor
		|| hitentity->behavior == &::actIronDoor )
	{
		hitentity->doorHandleDamageMagic(damage, damageSourceProjectile, caster);
		return true;
	}
	else if ( hitentity->isDamageableCollider() && hitentity->isColliderDamageableByMagic() )
	{
		hitentity->colliderHandleDamageMagic(damage, damageSourceProjectile, caster);
		return true;
	}
	else if ( hitentity->behavior == &::actFurniture )
	{
		hitentity->furnitureHandleDamageMagic(damage, damageSourceProjectile, caster);
		return true;
	}

	return false;
}

Entity* spellEffectDemesneDoor(Entity& caster, Entity& target)
{
	auto entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(&target, 1);
	int mapx = target.x / 16;
	int mapy = target.y / 16;
	for ( auto it : entLists )
	{
		node_t* node;
		for ( node = it->first; node != nullptr; node = node->next )
		{
			if ( Entity* entity = (Entity*)node->element )
			{
				if ( static_cast<int>(entity->x / 16) == mapx && static_cast<int>(entity->y / 16) == mapy )
				{
					if ( entity->behavior == &actDoor )
					{
						entity->doorHealth = 0;
					}
					if ( entity->behavior == &actGate && entity->gateStatus == 0 )
					{
						return nullptr; // no room
					}
					if ( entity->behavior == &actIronDoor && entity->doorStatus == 0 )
					{
						return nullptr; // no room
					}
					if ( entity->behavior == &actParticleDemesneDoor )
					{
						return nullptr;
					}
				}
			}
		}
	}

	Entity* door = newEntity(1809, 1, map.entities, nullptr);
	door->x = target.x;
	door->y = target.y;
	door->parent = caster.getUID();
	door->z = 0.0;
	door->flags[UNCLICKABLE] = true;
	door->flags[BLOCKSIGHT] = false;
	door->flags[PASSABLE] = true;
	door->flags[UPDATENEEDED] = true;
	/*door->lightBonus = vec4(0.25, 0.25,
		0.25, 0.f);*/
	door->ditheringOverride = 4;
	door->yaw = target.yaw;
	if ( ((door->yaw > -PI / 4 && door->yaw < 1 * PI / 4) || door->yaw >= 7 * PI / 4)
		|| (door->yaw >= 3 * PI / 4 && door->yaw < 5 * PI / 4) )
	{
		door->sizex = 1;
		door->sizey = 8;
	}
	else
	{
		door->sizex = 8;
		door->sizey = 1;
	}
	door->behavior = &actParticleDemesneDoor;
	TileEntityList.addEntity(*door);
	return door;
}

int getSpellDamageFromID(int spellID, Entity* parent, Stat* parentStats, Entity* magicSourceParticle, real_t addSpellBonus, bool applyingDamageOnCast)
{
	int damage = 0;
	spellElement_t* element = nullptr;
	int skillID = NUMPROFICIENCIES;
	if ( auto spell = getSpellFromID(spellID) )
	{
		skillID = spell->skillID;
		if ( spell->elements.first )
		{
			if ( element = (spellElement_t*)spell->elements.first->element )
			{
				if ( element->elements.first && element->elements.first->element )
				{
					element = (spellElement_t*)element->elements.first->element;
				}
			}
		}
	}
	if ( element )
	{
		Stat* myStats = parentStats;
		if ( !myStats && parent )
		{
			myStats = parent->getStats();
		}
		damage = element->getDamage();
		if ( abs(element->getDamageMult()) > 0.01 )
		{
			real_t bonus = (getBonusFromCasterOfSpellElement(parent, myStats, element, spellID, skillID));
			bonus += addSpellBonus;
			if ( applyingDamageOnCast )
			{
				if ( parent && parent->behavior == &actPlayer )
				{
					Compendium_t::Events_t::eventUpdateCodex(parent->skill[2], Compendium_t::CPDM_CLASS_PWR_MAX_CASTED, "pwr",
						(Sint32)(bonus * 100.0));
				}
			}

			damage += damage * bonus * element->getDamageMult();
			if ( element->getDamageMult() > 0.01 && element->getDamage() > 0 )
			{
				// range checking for PWR penalties, if we should do _some_ damage, then do at least 1
				damage = std::max(1, damage);
			}
		}
	}
	return damage;
}

int getSpellDamageSecondaryFromID(int spellID, Entity* parent, Stat* parentStats, Entity* magicSourceParticle, real_t addSpellBonus, bool applyingDamageOnCast)
{
	int damage = 0;
	spellElement_t* element = nullptr;
	int skillID = NUMPROFICIENCIES;
	if ( auto spell = getSpellFromID(spellID) )
	{
		skillID = spell->skillID;
		if ( spell->elements.first )
		{
			if ( element = (spellElement_t*)spell->elements.first->element )
			{
				if ( element->elements.first && element->elements.first->element )
				{
					element = (spellElement_t*)element->elements.first->element;
				}
			}
		}
	}
	if ( element )
	{
		Stat* myStats = parentStats;
		if ( !myStats && parent )
		{
			myStats = parent->getStats();
		}
		damage = element->getDamageSecondary();
		if ( abs(element->getDamageSecondaryMult()) > 0.01 )
		{
			real_t bonus = (getBonusFromCasterOfSpellElement(parent, myStats, element, spellID, skillID));
			bonus += addSpellBonus;
			if ( applyingDamageOnCast )
			{
				if ( parent && parent->behavior == &actPlayer )
				{
					Compendium_t::Events_t::eventUpdateCodex(parent->skill[2], Compendium_t::CPDM_CLASS_PWR_MAX_CASTED, "pwr",
						(Sint32)(bonus * 100.0));
				}
			}
			damage += damage * bonus * element->getDamageSecondaryMult();
			if ( element->getDamageSecondaryMult() > 0.01 && element->getDamageSecondary() > 0 )
			{
				// range checking for PWR penalties, if we should do _some_ damage, then do at least 1
				damage = std::max(1, damage);
			}
		}
	}
	return damage;
}
int getSpellEffectDurationFromID(int spellID, Entity* parent, Stat* parentStats, Entity* magicSourceParticle, real_t addSpellBonus)
{
	int duration = 0;
	spellElement_t* element = nullptr;
	if ( auto spell = getSpellFromID(spellID) )
	{
		if ( spell->elements.first )
		{
			if ( element = (spellElement_t*)spell->elements.first->element )
			{
				if ( element->elements.first && element->elements.first->element )
				{
					element = (spellElement_t*)element->elements.first->element;
				}
			}
		}
	}
	if ( element )
	{
		duration = element->duration;
		//real_t bonus = (getBonusFromCasterOfSpellElement(parent, parent ? parent->getStats() : nullptr, element, spellID));
		//bonus += addSpellBonus;
		//duration += duration * bonus;
	}
	return duration;
}

int getSpellEffectDurationSecondaryFromID(int spellID, Entity* parent, Stat* parentStats, Entity* magicSourceParticle, real_t addSpellBonus)
{
	int duration = 0;
	spellElement_t* element = nullptr;
	if ( auto spell = getSpellFromID(spellID) )
	{
		if ( spell->elements.first )
		{
			if ( element = (spellElement_t*)spell->elements.first->element )
			{
				if ( element->elements.first && element->elements.first->element )
				{
					element = (spellElement_t*)element->elements.first->element;
				}
			}
		}
	}
	if ( element )
	{
		duration = element->getDurationSecondary();
		//real_t bonus = (getBonusFromCasterOfSpellElement(parent, parent ? parent->getStats() : nullptr, element, spellID));
		//bonus += addSpellBonus;
		//duration += duration * bonus;
	}
	return duration;
}

real_t getSpellPropertyFromID(spell_t::SpellBasePropertiesFloat prop, int spellID, Entity* parent, Stat* parentStats, Entity* magicSourceParticle, real_t addSpellBonus)
{
	spellElement_t* element = nullptr;
	spell_t* spell = nullptr;
	real_t result = 1.0;
	if ( spell = getSpellFromID(spellID) )
	{
		if ( spell->elements.first )
		{
			if ( element = (spellElement_t*)spell->elements.first->element )
			{
				if ( element->elements.first && element->elements.first->element )
				{
					element = (spellElement_t*)element->elements.first->element;
				}
			}
		}

		Stat* myStats = parentStats;
		if ( !myStats && parent )
		{
			myStats = parent->getStats();
		}
		if ( prop == spell_t::SpellBasePropertiesFloat::SPELLPROP_CAST_TIME )
		{
			result = spell->cast_time;
		}
		else if ( prop == spell_t::SpellBasePropertiesFloat::SPELLPROP_DAMAGE_MULT )
		{
			result = element->getDamageMult();
		}
		else if ( prop == spell_t::SpellBasePropertiesFloat::SPELLPROP_DAMAGE_SECONDARY_MULT )
		{
			result = element->getDamageSecondaryMult();
		}
		else if ( prop == spell_t::SpellBasePropertiesFloat::SPELLPROP_CAST_TIME_MULT )
		{
			result = spell->cast_time_mult;
		}
		else if ( prop == spell_t::SpellBasePropertiesFloat::SPELLPROP_MODIFIED_FOCI_CAST_TIME )
		{
			result = spell->cast_time;
			real_t modifier = 1.0;// +spell->cast_time_mult;
			result *= modifier;
		}
		else if ( prop == spell_t::SpellBasePropertiesFloat::SPELLPROP_MODIFIED_SPELL_CAST_TIME )
		{
			result = spell->cast_time;
			if ( spell->cast_time_mult > 0.01 )
			{
				real_t equipmentModifier = 0.0;
				if ( myStats )
				{
					if ( myStats->breastplate )
					{
						if ( (myStats->breastplate->type == ROBE_HEALER && spell->skillID == PRO_THAUMATURGY)
							|| (myStats->breastplate->type == ROBE_WIZARD && spell->skillID == PRO_SORCERY)
							|| (myStats->breastplate->type == ROBE_CULTIST && spell->skillID == PRO_MYSTICISM)
							|| (myStats->breastplate->type == ROBE_MONK && spell->skillID == PRO_THAUMATURGY) )
						{
							if ( myStats->breastplate->beatitude >= 0 || shouldInvertEquipmentBeatitude(myStats) )
							{
								equipmentModifier = std::min(0.5, 0.2 + 0.1 * abs(myStats->breastplate->beatitude));
							}
							else
							{
								equipmentModifier = -0.5;
							}
						}
					}
				}
				if ( parent && parent->behavior == &actPlayer )
				{
					equipmentModifier += 0.05 * players[parent->skill[2]]->mechanics.getBreakableCounterTier();
				}
				real_t bonus = (getBonusFromCasterOfSpellElement(parent, myStats, element, spellID, spell->skillID));
				if ( spell->skillID == PRO_MYSTICISM || spell->skillID == PRO_SORCERY || spell->skillID == PRO_THAUMATURGY )
				{
					if ( myStats && myStats->getModifiedProficiency(spell->skillID) >= SKILL_LEVEL_LEGENDARY )
					{
						bonus += 1.0;
					}
				}
				real_t modifier = (statGetDEX(myStats, parent) * (1.0 + std::max(0.0, bonus)) * spell->cast_time_mult) / 100.0;
				result += -modifier;
				if ( bonus < -0.05 )
				{
					result *= (1 - bonus);
				}
				result *= (1 - std::min(1.0, equipmentModifier));
				if ( spell->cast_time < 1.01 )
				{
					result = std::max(0.5, result);
				}
				else
				{
					result = std::max(1.0, result);
				}
			}
		}
		else if ( prop == spell_t::SpellBasePropertiesFloat::SPELLPROP_MODIFIED_DISTANCE )
		{
			if ( spell->distance > 0.0 )
			{
				real_t equipmentModifier = 0.0;
				if ( myStats )
				{
					if ( myStats->breastplate )
					{
						if ( (myStats->breastplate->type == ROBE_HEALER && spell->skillID == PRO_THAUMATURGY)
							|| (myStats->breastplate->type == ROBE_WIZARD && spell->skillID == PRO_SORCERY)
							|| (myStats->breastplate->type == ROBE_CULTIST && spell->skillID == PRO_MYSTICISM)
							|| (myStats->breastplate->type == ROBE_MONK && spell->skillID == PRO_THAUMATURGY) )
						{
							if ( myStats->breastplate->beatitude >= 0 || shouldInvertEquipmentBeatitude(myStats) )
							{
								equipmentModifier = std::min(0.5, 0.2 + 0.1 * abs(myStats->breastplate->beatitude));
							}
							else
							{
								equipmentModifier = -0.5;
							}
						}
						else if ( myStats->breastplate->type == SHAWL )
						{
							if ( myStats->breastplate->beatitude >= 0 || shouldInvertEquipmentBeatitude(myStats) )
							{
								equipmentModifier = std::min(1.0, 0.35 + 0.35 * abs(myStats->breastplate->beatitude));
							}
							else
							{
								equipmentModifier = -1.0;
							}
						}
					}
				}
				real_t bonus = (getBonusFromCasterOfSpellElement(parent, myStats, element, spellID, spell->skillID));
				real_t modifier = (statGetPER(myStats, parent) * (1.0 + std::max(0.0, bonus)) * spell->distance_mult);
				real_t maxDist = 96.0;
				if ( equipmentModifier > 0.01 )
				{
					maxDist = 128.0;
				}
				static ConsoleVariable<float> cvar_spell_max_distance("/spell_max_distance", 96.0);
				if ( svFlags & SV_FLAG_CHEATS )
				{
					maxDist = *cvar_spell_max_distance;
				}
				result = std::min(maxDist, (spell->distance + modifier) * (1.0 + equipmentModifier));
				if ( bonus < -0.05 )
				{
					result *= (1 + bonus);
				}
				result = std::max(16.0, result);
			}
			else
			{
				return 0.0;
			}
		}
	}
	return result;
}

int getSpellPropertyFromID(spell_t::SpellBasePropertiesInt prop, int spellID, Entity* parent, Stat* parentStats, Entity* magicSourceParticle, real_t addSpellBonus)
{
	spellElement_t* element = nullptr;
	spell_t* spell = nullptr;
	int result = 1.0;
	if ( spell = getSpellFromID(spellID) )
	{
		int propulsion = 0;
		if ( spell->elements.first )
		{
			if ( element = (spellElement_t*)spell->elements.first->element )
			{
				if ( element->elements.first && element->elements.first->element )
				{
					propulsion = element->elementID;
					element = (spellElement_t*)element->elements.first->element;
				}
			}
		}

		Stat* myStats = parentStats;
		if ( !myStats && parent )
		{
			myStats = parent->getStats();
		}
		if ( prop == spell_t::SpellBasePropertiesInt::SPELLPROP_FOCI_REFIRE_TICKS )
		{
			if ( propulsion == SPELL_ELEMENT_PROPULSION_FOCI_SPRAY )
			{
				if ( element )
				{
					result = element->getChanneledManaDuration();
					real_t mult = 1.0;
					if ( myStats )
					{
						real_t modifier = std::min(100, myStats->getModifiedProficiency(spell->skillID)) / 100.0;
						modifier *= element->getChanneledManaMult();
						int percent = modifier * 100;
						percent += 100;
						percent = std::max(0, percent);
						result *= percent / 100.0;
					}
				}
			}
		}
		else if ( prop == spell_t::SpellBasePropertiesInt::SPELLPROP_FOCI_SECONDARY_MANA_COST )
		{
			if ( propulsion == SPELL_ELEMENT_PROPULSION_FOCI_SPRAY )
			{
				if ( element )
				{
					result = element->channeledMana;
				}
			}
		}
		else if ( prop == spell_t::SpellBasePropertiesInt::SPELLPROP_MODIFIED_RADIUS )
		{
			result = spell->radius;
			real_t radiusScale = 0.0;
			real_t modifier = 1.0 + spell->radius_mult * radiusScale;
			result *= modifier;
		}
	}
	return result;
}

int getSpellFromSummonedEntityForSpellEvent(Entity* summon)
{
	if ( !summon ) { return SPELL_NONE; }
	if ( summon->behavior != &actMonster ) { return SPELL_NONE; }

	Stat* destStats = summon->getStats();
	if ( !destStats ) { return SPELL_NONE; }

	if ( !(summon->monsterAllyGetPlayerLeader() 
		|| achievementObserver.checkUidIsFromPlayer(destStats->leader_uid) >= 0) ) 
	{ 
		return SPELL_NONE;
	}

	if ( summon->monsterAllySummonRank != 0 )
	{
		if ( destStats->type == EARTH_ELEMENTAL )
		{
			return SPELL_EARTH_ELEMENTAL;
		}
		else if ( destStats->type == SKELETON )
		{
			return SPELL_SUMMON;
		}
	}
	else if ( destStats->type == MONSTER_ADORCISED_WEAPON )
	{
		if ( destStats->getAttribute("spirit_weapon") != "" )
		{
			return SPELL_SPIRIT_WEAPON;
		}
		else if ( destStats->getAttribute("adorcised_weapon") != "" )
		{
			return SPELL_ADORCISM;
		}
	}
	else if ( destStats->type == MOTH_SMALL && destStats->getAttribute("fire_sprite") != "" )
	{
		return SPELL_FIRE_SPRITE;
	}
	else if ( destStats->type == SKELETON && destStats->getAttribute("revenant_skeleton") != "" )
	{
		return SPELL_REVENANT_CURSE;
	}
	else if ( destStats->type == REVENANT_SKULL && destStats->getAttribute("revenant_skull") != "" )
	{
		return SPELL_CURSE_FLESH;
	}

	return SPELL_NONE;
}

int getSpellDamageFromStatic(int spellID, Stat* hitstats)
{
	if ( !hitstats )
	{
		return 0;
	}

	if ( hitstats->getEffectActive(EFF_STATIC) )
	{
		return hitstats->getEffectActive(EFF_STATIC);
	}

	return 0;
}

void updateEntityOldHPBeforeMagicHit(Entity& my, Entity& projectile)
{
	if ( projectile.behavior == &actMagicMissile && projectile.actmagicUpdateOLDHPOnHit == 1 )
	{
		if ( my.getStats() )
		{
			my.getStats()->OLDHP = my.getStats()->HP;
		}
		else
		{
			if ( my.behavior == &actDoor || my.behavior == &actIronDoor )
			{
				my.doorOldHealth = my.doorHealth;
			}
			else if ( my.behavior == &actFurniture )
			{
				my.furnitureOldHealth = my.furnitureHealth;
			}
			else if ( my.behavior == &actChest )
			{
				my.chestOldHealth = my.chestHealth;
			}
			else if ( my.isDamageableCollider() )
			{
				my.colliderOldHP = my.colliderCurrentHP;
			}
		}
	}
}