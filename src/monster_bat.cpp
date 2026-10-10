/*-------------------------------------------------------------------------------

	BARONY
	File: monster_sentrybot.cpp
	Desc: implements all of the kobold monster's code

	Copyright 2013-2019 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "monster.hpp"
#include "engine/audio/sound.hpp"
#include "book.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "magic/magic.hpp"
#include "interface/interface.hpp"
#include "prng.hpp"
#include "mod_tools.hpp"
#include "paths.hpp"
#include <limits>

static constexpr real_t FAMILIAR_TRANSFORM_TRAVEL_SPEED = 16.0;

void spawnAmuletFamiliarIngredientBag(Entity* recipient, Entity* source)
{
	if ( multiplayer == CLIENT || !recipient || recipient->behavior != &actPlayer
		|| !source || source->behavior != &actMonster )
	{
		return;
	}
	const int player = recipient->isEntityPlayer();
	Stat* recipientStats = recipient->getStats();
	if ( player < 0 || player >= MAXPLAYERS || !recipientStats || !recipientStats->amulet
		|| recipientStats->amulet->type != AMULET_FAMILIAR || recipientStats->amulet->status == BROKEN )
	{
		return;
	}
	const WitchIngredientCategory category = getIngredientCategoryForMonster(source);
	if ( category == INGREDIENT_CATEGORY_NONE ) { return; }
	const Stat* sourceStats = source->getStats();
	const bool isHumanOrGreenSlime = sourceStats && (sourceStats->type == HUMAN
		|| (sourceStats->type == SLIME
			&& MonsterData_t::getKeyFromSprite(source->sprite, SLIME) == "slime green"));
	const int generatedCount = 2 + local_rng.rand() % (isHumanOrGreenSlime ? 7 : 3);

	Entity* bag = newEntity(-1, 1, map.entities, nullptr);
	bag->flags[INVISIBLE] = true;
	bag->flags[UPDATENEEDED] = true;
	bag->flags[PASSABLE] = true;
	bag->x = source->x;
	bag->y = source->y;
	bag->z = 0.0;
	bag->sizex = 4;
	bag->sizey = 4;
	bag->yaw = local_rng.rand() % 360 * (PI / 180.0);
	bag->behavior = &actItem;
	bag->skill[10] = ingredientBagForCategory(category);
	bag->skill[11] = EXCELLENT;
	bag->skill[12] = 0;
	bag->skill[13] = 1;
	bag->skill[14] = 0;
	bag->skill[15] = 1;
	bag->itemIngredientCount0 = 0;
	bag->itemIngredientCount1 = 0;
	bag->itemIngredientCount2 = 0;
	bag->itemIngredientCount3 = 0;
	bag->itemIngredientBagOwnerUid = static_cast<Sint32>(recipient->getUID());
	for ( int i = 0; i < generatedCount; ++i )
	{
		switch ( local_rng.rand() % 4 )
		{
			case 0: ++bag->itemIngredientCount0; break;
			case 1: ++bag->itemIngredientCount1; break;
			case 2: ++bag->itemIngredientCount2; break;
			case 3: ++bag->itemIngredientCount3; break;
		}
	}
}
static constexpr real_t FAMILIAR_CAULDRON_BASE_Z = 1;
static constexpr real_t FAMILIAR_CAULDRON_BOB_AMPLITUDE = 0.2;
static constexpr real_t FAMILIAR_CAULDRON_BOB_PERIOD_SECONDS = 2.75;
static constexpr real_t FAMILIAR_CAULDRON_ROTATION_PERIOD_SECONDS = 11.0;
static constexpr Sint32 FAMILIAR_CAULDRON_COLLISION_SIZE = 4;

bool isAmuletFamiliar(const Entity* entity, const Stat* stats)
{
	if ( entity && entity->familiarIdentity == 1 )
	{
		return true;
	}
	if ( stats )
	{
		auto found = stats->attributes.find("AMULET_FAMILIAR");
		return found != stats->attributes.end() && found->second == "1";
	}
	return false;
}

FamiliarForm getAmuletFamiliarForm(const Entity* entity)
{
	if ( !entity || !isAmuletFamiliar(entity, entity->getStats()) )
	{
		return FAMILIAR_FORM_NONE;
	}
	if ( entity->familiarForm < FAMILIAR_FORM_BAT || entity->familiarForm > FAMILIAR_FORM_CAULDRON )
	{
		return FAMILIAR_FORM_BAT;
	}
	return static_cast<FamiliarForm>(entity->familiarForm);
}

bool familiarUsesBatCollisionSemantics(const Entity* entity)
{
	if ( !entity || entity->behavior != &actMonster )
	{
		return false;
	}
	const Stat* stats = entity->getStats();
	if ( isAmuletFamiliar(entity, stats) )
	{
		return getAmuletFamiliarForm(entity) == FAMILIAR_FORM_BAT;
	}
	return (stats && stats->type == BAT_FAMILIAR)
		|| entity->getMonsterTypeFromSprite() == BAT_FAMILIAR;
}

bool isAmuletFamiliarMimic(const Entity* entity)
{
	return entity && entity->behavior == &actMonster
		&& isAmuletFamiliar(entity, entity->getStats())
		&& getAmuletFamiliarForm(entity) == FAMILIAR_FORM_MIMIC;
}

bool familiarFormHasCreatureCollision(const Entity* entity)
{
	if ( !entity || !isAmuletFamiliar(entity, entity->getStats()) )
	{
		return false;
	}
	const FamiliarForm form = getAmuletFamiliarForm(entity);
	return form == FAMILIAR_FORM_MIMIC || form == FAMILIAR_FORM_CAULDRON;
}

bool familiarFormIsCombatTargetable(const Entity* entity)
{
	return isAmuletFamiliarMimic(entity);
}

static bool isBatFamiliar(const Entity* entity, const Stat* stats)
{
	return isAmuletFamiliar(entity, stats) || (stats && stats->type == BAT_FAMILIAR)
		|| (entity && entity->getMonsterTypeFromSprite() == BAT_FAMILIAR);
}

static void removeAmuletFamiliarBodyparts(Entity* familiar)
{
	if ( !familiar ) { return; }
	int index = 0;
	for ( node_t* node = familiar->children.first, *next = nullptr; node; node = next, ++index )
	{
		next = node->next;
		if ( index < 2 ) { continue; }
		Entity* limb = static_cast<Entity*>(node->element);
		if ( limb && limb->mynode )
		{
			limb->flags[UPDATENEEDED] = false;
			list_RemoveNode(limb->mynode);
		}
		list_RemoveNode(node);
	}
	familiar->bodyparts.clear();
}

void spawnAmuletFamiliarTransformParticles(Entity* entity)
{
	if ( !entity ) { return; }
	createParticleErupt(entity, 983);
	if ( multiplayer != CLIENT )
	{
		serverSpawnMiscParticles(entity, PARTICLE_EFFECT_ERUPT, 983);
	}
}

static void playAmuletFamiliarTransformEffect(Entity* familiar)
{
	if ( !familiar ) { return; }
	playSoundEntity(familiar, 171, 128);
	spawnAmuletFamiliarTransformParticles(familiar);
}

static void stopAmuletFamiliarCauldronAmbience(Entity* familiar)
{
	if ( !familiar || !familiar->entity_sound ) { return; }
#ifdef USE_FMOD
	familiar->entity_sound->stop();
#elif defined USE_OPENAL
	OPENAL_Channel_Stop(familiar->entity_sound);
#endif
	familiar->entity_sound = nullptr;
}

static void startAmuletFamiliarCauldronAmbience(Entity* familiar)
{
	if ( !familiar ) { return; }
#ifdef USE_FMOD
	if ( familiar->entity_sound )
	{
		bool playing = false;
		familiar->entity_sound->isPlaying(&playing);
		if ( playing ) { return; }
		familiar->entity_sound = nullptr;
	}
	familiar->entity_sound = playSoundEntityLocal(familiar, SFX_CAULDRON_AMBIANCE, 64);

#elif defined USE_OPENAL
	if ( familiar->entity_sound )
	{
		ALboolean playing = AL_FALSE;
		OPENAL_Channel_IsPlaying(familiar->entity_sound, &playing);
		if ( playing ) { return; }
		familiar->entity_sound = nullptr;
	}
	familiar->entity_sound = playSoundEntityLocal(familiar, SFX_CAULDRON_AMBIANCE, 64);
	if ( familiar->entity_sound )
	{
		OPENAL_SetLoop(static_cast<OPENAL_SOUND*>(familiar->entity_sound), AL_TRUE);
	}
#else
	familiar->entity_sound = playSoundEntityLocal(familiar, SFX_CAULDRON_AMBIANCE, 64);
#endif
}

static void storeFamiliarBatStats(Stat& stats)
{
	stats.setAttribute("AMULET_FAMILIAR_BAT_STR", std::to_string(stats.STR));
	stats.setAttribute("AMULET_FAMILIAR_BAT_DEX", std::to_string(stats.DEX));
	stats.setAttribute("AMULET_FAMILIAR_BAT_CON", std::to_string(stats.CON));
	stats.setAttribute("AMULET_FAMILIAR_BAT_INT", std::to_string(stats.INT));
	stats.setAttribute("AMULET_FAMILIAR_BAT_PER", std::to_string(stats.PER));
	stats.setAttribute("AMULET_FAMILIAR_BAT_CHR", std::to_string(stats.CHR));
	stats.setAttribute("AMULET_FAMILIAR_BAT_LVL", std::to_string(stats.LVL));
}

static void restoreFamiliarBatStats(Stat& stats)
{
	auto restore = [&stats](const char* key, Sint32& value)
	{
		auto found = stats.attributes.find(key);
		if ( found != stats.attributes.end() )
		{
			value = static_cast<Sint32>(strtol(found->second.c_str(), nullptr, 10));
		}
	};
	restore("AMULET_FAMILIAR_BAT_STR", stats.STR);
	restore("AMULET_FAMILIAR_BAT_DEX", stats.DEX);
	restore("AMULET_FAMILIAR_BAT_CON", stats.CON);
	restore("AMULET_FAMILIAR_BAT_INT", stats.INT);
	restore("AMULET_FAMILIAR_BAT_PER", stats.PER);
	restore("AMULET_FAMILIAR_BAT_CHR", stats.CHR);
	restore("AMULET_FAMILIAR_BAT_LVL", stats.LVL);
}

static Sint32 getFamiliarStoredStat(const Stat& stats, const char* key, Sint32 fallback)
{
	auto found = stats.attributes.find(key);
	return found == stats.attributes.end() ? fallback
		: static_cast<Sint32>(strtol(found->second.c_str(), nullptr, 10));
}

static void foldMimicLevelStatGrowthIntoCanonicalBatStats(Stat& stats)
{
	struct StatMapping { const char* canonical; const char* baseline; Sint32* live; };
	StatMapping mappings[] = {
		{ "AMULET_FAMILIAR_BAT_STR", "AMULET_FAMILIAR_MIMIC_BASE_STR", &stats.STR },
		{ "AMULET_FAMILIAR_BAT_DEX", "AMULET_FAMILIAR_MIMIC_BASE_DEX", &stats.DEX },
		{ "AMULET_FAMILIAR_BAT_CON", "AMULET_FAMILIAR_MIMIC_BASE_CON", &stats.CON },
		{ "AMULET_FAMILIAR_BAT_INT", "AMULET_FAMILIAR_MIMIC_BASE_INT", &stats.INT },
		{ "AMULET_FAMILIAR_BAT_PER", "AMULET_FAMILIAR_MIMIC_BASE_PER", &stats.PER },
		{ "AMULET_FAMILIAR_BAT_CHR", "AMULET_FAMILIAR_MIMIC_BASE_CHR", &stats.CHR }
	};
	for ( auto& mapping : mappings )
	{
		const Sint32 baseline = getFamiliarStoredStat(stats, mapping.baseline, *mapping.live);
		const Sint32 growth = std::max<Sint32>(0, *mapping.live - baseline);
		if ( growth > 0 )
		{
			const Sint32 canonical = getFamiliarStoredStat(stats, mapping.canonical, 0);
			stats.setAttribute(mapping.canonical, std::to_string(canonical + growth));
		}
		stats.setAttribute(mapping.baseline, std::to_string(*mapping.live));
	}
}

static Item* findFamiliarAmuletItem(Uint32 instanceId)
{
	if ( instanceId == 0 ) { return nullptr; }
	for ( int player = 0; player < MAXPLAYERS; ++player )
	{
		if ( !stats[player] ) { continue; }
		for ( node_t* node = stats[player]->inventory.first; node; node = node->next )
		{
			Item* item = static_cast<Item*>(node->element);
			if ( item && item->type == AMULET_FAMILIAR && item->familiarInstanceId == instanceId )
			{
				return item;
			}
		}
	}
	for ( node_t* node = map.creatures->first; node; node = node->next )
	{
		Entity* creature = static_cast<Entity*>(node->element);
		Stat* creatureStats = creature ? creature->getStats() : nullptr;
		if ( !creatureStats ) { continue; }
		for ( node_t* itemNode = creatureStats->inventory.first; itemNode; itemNode = itemNode->next )
		{
			Item* item = static_cast<Item*>(itemNode->element);
			if ( item && item->type == AMULET_FAMILIAR && item->familiarInstanceId == instanceId )
			{
				return item;
			}
		}
	}
	return nullptr;
}

bool syncAmuletFamiliarPersistentState(Entity* familiar, Item* amulet, bool notifyOwner)
{
	if ( multiplayer == CLIENT || !familiar || !isAmuletFamiliar(familiar, familiar->getStats())
		|| familiar->familiarInstanceId == 0 )
	{
		return false;
	}
	Stat* familiarStats = familiar->getStats();
	if ( !familiarStats ) { return false; }
	if ( !amulet ) { amulet = findFamiliarAmuletItem(familiar->familiarInstanceId); }
	if ( !amulet || amulet->type != AMULET_FAMILIAR
		|| amulet->familiarInstanceId != familiar->familiarInstanceId )
	{
		return false;
	}

	if ( getAmuletFamiliarForm(familiar) == FAMILIAR_FORM_MIMIC )
	{
		foldMimicLevelStatGrowthIntoCanonicalBatStats(*familiarStats);
	}
	const bool mimic = getAmuletFamiliarForm(familiar) == FAMILIAR_FORM_MIMIC;
	const Sint32 canonicalSTR = mimic ? getFamiliarStoredStat(*familiarStats, "AMULET_FAMILIAR_BAT_STR", amulet->familiarSTR) : familiarStats->STR;
	const Sint32 canonicalDEX = mimic ? getFamiliarStoredStat(*familiarStats, "AMULET_FAMILIAR_BAT_DEX", amulet->familiarDEX) : familiarStats->DEX;
	const Sint32 canonicalCON = mimic ? getFamiliarStoredStat(*familiarStats, "AMULET_FAMILIAR_BAT_CON", amulet->familiarCON) : familiarStats->CON;
	const Sint32 canonicalINT = mimic ? getFamiliarStoredStat(*familiarStats, "AMULET_FAMILIAR_BAT_INT", amulet->familiarINT) : familiarStats->INT;
	const Sint32 canonicalPER = mimic ? getFamiliarStoredStat(*familiarStats, "AMULET_FAMILIAR_BAT_PER", amulet->familiarPER) : familiarStats->PER;
	const Sint32 canonicalCHR = mimic ? getFamiliarStoredStat(*familiarStats, "AMULET_FAMILIAR_BAT_CHR", amulet->familiarCHR) : familiarStats->CHR;
	bool changed = !amulet->familiarInitialized
		|| amulet->familiarHP != familiarStats->HP || amulet->familiarMAXHP != familiarStats->MAXHP
		|| amulet->familiarMP != familiarStats->MP || amulet->familiarMAXMP != familiarStats->MAXMP
		|| amulet->familiarLVL != familiarStats->LVL || amulet->familiarEXP != familiarStats->EXP
		|| amulet->familiarSTR != canonicalSTR || amulet->familiarDEX != canonicalDEX
		|| amulet->familiarCON != canonicalCON || amulet->familiarINT != canonicalINT
		|| amulet->familiarPER != canonicalPER || amulet->familiarCHR != canonicalCHR;
	if ( !changed ) { return false; }
	amulet->familiarInitialized = true;
	amulet->familiarHP = familiarStats->HP;
	amulet->familiarMAXHP = familiarStats->MAXHP;
	amulet->familiarMP = familiarStats->MP;
	amulet->familiarMAXMP = familiarStats->MAXMP;
	amulet->familiarLVL = familiarStats->LVL;
	amulet->familiarEXP = familiarStats->EXP;
	amulet->familiarSTR = canonicalSTR;
	amulet->familiarDEX = canonicalDEX;
	amulet->familiarCON = canonicalCON;
	amulet->familiarINT = canonicalINT;
	amulet->familiarPER = canonicalPER;
	amulet->familiarCHR = canonicalCHR;

	if ( notifyOwner && multiplayer == SERVER )
	{
		serverSyncPersistentCustomItemStateForOwner(amulet);
	}
	return true;
}

void rebuildAmuletFamiliarCauldron(Entity* familiar)
{
	if ( !familiar ) { return; }
	removeAmuletFamiliarBodyparts(familiar);
	static bool loggedMissingCauldronModel = false;
	if ( !loggedMissingCauldronModel
		&& (BAT_FAMILIAR_MODEL_CAULDRON >= static_cast<Sint32>(nummodels)
			|| !models || !models[BAT_FAMILIAR_MODEL_CAULDRON]) )
	{
		printlog("error: Amulet Familiar cauldron model 2504 is unavailable; expected models/decorations/witch_cauldron.vox in slab .vox format.");
		loggedMissingCauldronModel = true;
	}
	familiar->familiarCauldronBaseZ = FAMILIAR_CAULDRON_BASE_Z;
	familiar->z = familiar->familiarCauldronBaseZ;
	familiar->initMonster(BAT_FAMILIAR_MODEL_CAULDRON);
	// The cauldron is a single visible root model. Keeping it on the root avoids
	// a second replicated bodypart that can go missing or remain stale.
	familiar->sprite = BAT_FAMILIAR_MODEL_CAULDRON;
	familiar->flags[INVISIBLE] = false;
	familiar->flags[PASSABLE] = false;
	familiar->flags[BLOCKSIGHT] = true;
	familiar->sizex = FAMILIAR_CAULDRON_COLLISION_SIZE;
	familiar->sizey = FAMILIAR_CAULDRON_COLLISION_SIZE;
	familiar->focalx = 0.0;
	familiar->focaly = 0.0;
	familiar->focalz = 0.0;
	animateAmuletFamiliarCauldron(familiar);
	startAmuletFamiliarCauldronAmbience(familiar);
}

void animateAmuletFamiliarCauldron(Entity* familiar)
{
	if ( !familiar ) { return; }
	const real_t bobPhase = (2.0 * PI * ticks)
		/ (FAMILIAR_CAULDRON_BOB_PERIOD_SECONDS * TICKS_PER_SECOND);
	const real_t turnPhase = (2.0 * PI * ticks)
		/ (FAMILIAR_CAULDRON_ROTATION_PERIOD_SECONDS * TICKS_PER_SECOND);
	familiar->z = familiar->familiarCauldronBaseZ
		+ FAMILIAR_CAULDRON_BOB_AMPLITUDE * sin(bobPhase);
	familiar->yaw = turnPhase;
	familiar->pitch = 0.0;
	familiar->roll = 0.0;
}

static void enforceAmuletFamiliarFormFlags(Entity* familiar, FamiliarForm form)
{
	if ( !familiar ) { return; }
	const bool passable = form == FAMILIAR_FORM_BAT;
	const bool blockSight = form != FAMILIAR_FORM_BAT;
	const bool invisible = form != FAMILIAR_FORM_CAULDRON;
	auto setFlag = [familiar](int flag, bool value)
	{
		if ( familiar->flags[flag] == value ) { return; }
		familiar->flags[flag] = value;
		if ( multiplayer == SERVER )
		{
			serverUpdateEntityFlag(familiar, flag);
		}
	};
	setFlag(PASSABLE, passable);
	setFlag(BLOCKSIGHT, blockSight);
	setFlag(INVISIBLE, invisible);
}

void rebuildAmuletFamiliarVisuals(Entity* familiar)
{
	if ( !familiar || !isAmuletFamiliar(familiar, familiar->getStats()) )
	{
		return;
	}
	stopAmuletFamiliarCauldronAmbience(familiar);
	removeAmuletFamiliarBodyparts(familiar);
	switch ( getAmuletFamiliarForm(familiar) )
	{
		case FAMILIAR_FORM_MIMIC:
			initMimic(familiar, multiplayer == CLIENT ? nullptr : familiar->getStats());
			break;
		case FAMILIAR_FORM_CAULDRON:
			rebuildAmuletFamiliarCauldron(familiar);
			break;
		case FAMILIAR_FORM_BAT:
		default:
			initBat(familiar, multiplayer == CLIENT ? nullptr : familiar->getStats());
			break;
	}
}

void transformAmuletFamiliar(Entity* familiar, FamiliarForm form, bool playEffect)
{
	if ( multiplayer == CLIENT || !familiar || !isAmuletFamiliar(familiar, familiar->getStats()) )
	{
		return;
	}
	Stat* familiarStats = familiar->getStats();
	if ( !familiarStats || form < FAMILIAR_FORM_BAT || form > FAMILIAR_FORM_CAULDRON )
	{
		return;
	}
	const FamiliarForm oldForm = getAmuletFamiliarForm(familiar);
	if ( oldForm == form )
	{
		familiar->familiarPendingForm = FAMILIAR_FORM_NONE;
		return;
	}
	if ( oldForm == FAMILIAR_FORM_MIMIC )
	{
		closeAmuletFamiliarVoidChest(familiar);
	}
	else if ( oldForm == FAMILIAR_FORM_CAULDRON )
	{
		stopAmuletFamiliarCauldronAmbience(familiar);
		closeStewingForCauldron(familiar);
	}

	const Sint32 hp = familiarStats->HP;
	const Sint32 maxhp = familiarStats->MAXHP;
	const Sint32 oldhp = familiarStats->OLDHP;
	const Sint32 persistentLevel = familiarStats->LVL;
	const bool oldPassable = familiar->flags[PASSABLE];
	const bool oldBlockSight = familiar->flags[BLOCKSIGHT];
	const bool oldInvisible = familiar->flags[INVISIBLE];
	removeAmuletFamiliarBodyparts(familiar);
	familiar->monsterReleaseAttackTarget();
	familiar->monsterAttack = 0;
	familiar->monsterAttackTime = 0;
	familiar->familiarForm = form;
	familiar->familiarPendingForm = FAMILIAR_FORM_NONE;
	familiar->flags[USERFLAG2] = false;
	familiarStats->monsterNoDropItems = 1;
	familiarStats->setAttribute("AMULET_FAMILIAR", "1");

	if ( form == FAMILIAR_FORM_MIMIC )
	{
		storeFamiliarBatStats(*familiarStats);
		familiarStats->type = MIMIC;
		familiar->mimicSetStats(familiarStats);
		familiarStats->LVL = persistentLevel;
		familiarStats->setAttribute("AMULET_FAMILIAR_MIMIC_BASE_STR", std::to_string(familiarStats->STR));
		familiarStats->setAttribute("AMULET_FAMILIAR_MIMIC_BASE_DEX", std::to_string(familiarStats->DEX));
		familiarStats->setAttribute("AMULET_FAMILIAR_MIMIC_BASE_CON", std::to_string(familiarStats->CON));
		familiarStats->setAttribute("AMULET_FAMILIAR_MIMIC_BASE_INT", std::to_string(familiarStats->INT));
		familiarStats->setAttribute("AMULET_FAMILIAR_MIMIC_BASE_PER", std::to_string(familiarStats->PER));
		familiarStats->setAttribute("AMULET_FAMILIAR_MIMIC_BASE_CHR", std::to_string(familiarStats->CHR));
		familiarStats->RANDOM_GOLD = 0;
		familiarStats->setEffectValueUnsafe(EFF_MIMIC_LOCKED, 0);
		familiarStats->EFFECTS_TIMERS[EFF_MIMIC_LOCKED] = 0;
		bool clearedFlyingEffect = false;
		for ( const int effect : { EFF_LEVITATING, EFF_FLUTTER } )
		{
			if ( familiarStats->getEffectActive(effect) || familiarStats->EFFECTS_TIMERS[effect] != 0 )
			{
				familiarStats->setEffectValueUnsafe(effect, 0);
				familiarStats->EFFECTS_TIMERS[effect] = 0;
				clearedFlyingEffect = true;
			}
		}
		if ( clearedFlyingEffect && multiplayer == SERVER )
		{
			familiar->serverUpdateEffectsForEntity(true);
		}
		initMimic(familiar, familiarStats);
		familiar->sizex = 4;
		familiar->sizey = 4;
		familiar->familiarLastGroundedX = familiar->x;
		familiar->familiarLastGroundedY = familiar->y;
		familiar->familiarHasGroundedPosition = true;
		familiar->monsterSpecialState = MIMIC_ACTIVE;
		familiar->monsterState = MONSTER_STATE_WAIT;
	}
	else
	{
		familiarStats->type = BAT_FAMILIAR;
		if ( oldForm == FAMILIAR_FORM_MIMIC )
		{
			restoreFamiliarBatStats(*familiarStats);
		}
		familiarStats->LVL = persistentLevel;
		familiar->familiarHasGroundedPosition = false;
		familiar->monsterState = MONSTER_STATE_WAIT;
		if ( form == FAMILIAR_FORM_BAT )
		{
			initBat(familiar, familiarStats);
		}
		else
		{
			rebuildAmuletFamiliarCauldron(familiar);
		}
	}
	enforceAmuletFamiliarFormFlags(familiar, form);
	familiarStats->HP = std::max<Sint32>(1, std::min(hp, maxhp));
	familiarStats->MAXHP = maxhp;
	familiarStats->OLDHP = std::max<Sint32>(1, std::min(oldhp, maxhp));

	if ( multiplayer == SERVER )
	{
		serverUpdateEntitySprite(familiar);
		serverUpdateEntitySkill(familiar, 0);
		serverUpdateEntitySkill(familiar, 1);
		serverUpdateEntitySkill(familiar, 8);
		serverUpdateEntitySkill(familiar, 9);
		serverUpdateEntitySkill(familiar, 33);
		serverUpdateEntitySkill(familiar, 57);
		serverUpdateEntitySkill(familiar, 58);
		if ( oldPassable != familiar->flags[PASSABLE] )
		{
			serverUpdateEntityFlag(familiar, PASSABLE);
		}
		if ( oldBlockSight != familiar->flags[BLOCKSIGHT] )
		{
			serverUpdateEntityFlag(familiar, BLOCKSIGHT);
		}
		if ( oldInvisible != familiar->flags[INVISIBLE] )
		{
			serverUpdateEntityFlag(familiar, INVISIBLE);
		}
		serverUpdateEntityFlag(familiar, USERFLAG2);
		serverUpdateBodypartIDs(familiar);
	}
	if ( playEffect )
	{
		playAmuletFamiliarTransformEffect(familiar);
	}
}

Entity* findAmuletFamiliar(Entity* owner)
{
	if ( !owner ) { return nullptr; }
	for ( node_t* node = map.creatures->first; node; node = node->next )
	{
		Entity* creature = static_cast<Entity*>(node->element);
		Stat* creatureStats = creature ? creature->getStats() : nullptr;
		if ( creatureStats
			&& (isAmuletFamiliar(creature, creatureStats)
				|| creatureStats->type == BAT_FAMILIAR)
			&& creatureStats->leader_uid == owner->getUID() )
		{
			return creature;
		}
	}
	return nullptr;
}

static bool familiarTargetRequestsCauldron(real_t targetX, real_t targetY)
{
	const int tilex = static_cast<int>(floor(targetX / 16.0));
	const int tiley = static_cast<int>(floor(targetY / 16.0));
	if ( tilex < 0 || tilex >= map.width || tiley < 0 || tiley >= map.height )
	{
		return false;
	}
	const int tile = map.tiles[tiley * MAPLAYERS + tilex * MAPLAYERS * map.height];
	if ( tile > 0 && lavatiles[tile] )
	{
		return true;
	}
	list_t* entities = TileEntityList.getTileList(tilex, tiley);
	for ( node_t* node = entities ? entities->first : nullptr; node; node = node->next )
	{
		Entity* entity = static_cast<Entity*>(node->element);
		if ( entity && entity->behavior == &actCampfire && entity->skill[3] > 0
			&& familiarCanBecomeMimicAtTile(tilex, tiley) )
		{
			return true;
		}
	}
	return false;
}

static bool familiarCauldronHasActiveHeat(const Entity* familiar)
{
	if ( !familiar ) { return false; }
	const int tilex = static_cast<int>(floor(familiar->x / 16.0));
	const int tiley = static_cast<int>(floor(familiar->y / 16.0));
	if ( tilex < 0 || tilex >= map.width || tiley < 0 || tiley >= map.height )
	{
		return false;
	}
	const int tile = map.tiles[tiley * MAPLAYERS + tilex * MAPLAYERS * map.height];
	if ( tile > 0 && lavatiles[tile] ) { return true; }
	list_t* entities = TileEntityList.getTileList(tilex, tiley);
	for ( node_t* node = entities ? entities->first : nullptr; node; node = node->next )
	{
		Entity* entity = static_cast<Entity*>(node->element);
		if ( entity && entity->behavior == &actCampfire && entity->skill[3] > 0 )
		{
			return true;
		}
	}
	return false;
}

bool familiarCanBecomeMimicAtTile(int tilex, int tiley)
{
	if ( tilex < 0 || tilex >= map.width || tiley < 0 || tiley >= map.height )
	{
		return false;
	}
	const int base = tiley * MAPLAYERS + tilex * MAPLAYERS * map.height;
	const int floorTile = map.tiles[base];
	if ( floorTile <= 0 || lavatiles[floorTile] || swimmingtiles[floorTile]
		|| map.tiles[OBSTACLELAYER + base] != 0 )
	{
		return false;
	}
	if ( !pathMapGrounded || pathMapGrounded[tiley + tilex * map.height] == 0 )
	{
		return false;
	}
	list_t* entities = TileEntityList.getTileList(tilex, tiley);
	for ( node_t* node = entities ? entities->first : nullptr; node; node = node->next )
	{
		Entity* entity = static_cast<Entity*>(node->element);
		if ( !entity || entity->flags[PASSABLE]
			|| entity->behavior == &actMonster || entity->behavior == &actPlayer )
		{
			continue;
		}
		if ( isPathObstacle(entity) || entity->behavior == &actFurniture
			|| entity->behavior == &actChest || entity->behavior == &actDoor
			|| entity->behavior == &actIronDoor || entity->isDamageableCollider() )
		{
			return false;
		}
	}
	return true;
}

bool amuletFamiliarPolymorphTargetIsValid(Entity* familiar, real_t targetX, real_t targetY)
{
	if ( !familiar || !isAmuletFamiliar(familiar, familiar->getStats())
		|| getAmuletFamiliarForm(familiar) != FAMILIAR_FORM_BAT )
	{
		return false;
	}
	const int tilex = static_cast<int>(floor(targetX / 16.0));
	const int tiley = static_cast<int>(floor(targetY / 16.0));
	if ( tilex < 0 || tilex >= map.width || tiley < 0 || tiley >= map.height )
	{
		return false;
	}
	return familiarTargetRequestsCauldron(targetX, targetY)
		|| familiarCanBecomeMimicAtTile(tilex, tiley);
}

static Sint32 getAmuletFamiliarPolymorphMinHP(const Stat& familiarStats)
{
	if ( familiarStats.MAXHP <= 0 )
	{
		return 0;
	}
	return familiarStats.MAXHP / 4 + (familiarStats.MAXHP % 4 != 0);
}

bool amuletFamiliarIsTooWeakToTransform(const Entity* familiar, const Stat* familiarStats)
{
	if ( !familiar || getAmuletFamiliarForm(familiar) != FAMILIAR_FORM_BAT )
	{
		return false;
	}
	if ( !familiarStats )
	{
		familiarStats = familiar->getStats();
	}
	return familiarStats && familiarStats->MAXHP > 0
		&& familiarStats->HP < getAmuletFamiliarPolymorphMinHP(*familiarStats);
}

bool requestAmuletFamiliarPolymorph(Entity* owner, real_t targetX, real_t targetY)
{
	if ( multiplayer == CLIENT || !owner || owner->behavior != &actPlayer )
	{
		return false;
	}
	Stat* ownerStats = owner->getStats();
	if ( !ownerStats || !ownerStats->amulet || ownerStats->amulet->type != AMULET_FAMILIAR
		|| ownerStats->amulet->status == BROKEN )
	{
		return false;
	}
	Entity* familiar = findAmuletFamiliar(owner);
	if ( !familiar ) { return false; }
	Stat* familiarStats = familiar->getStats();
	if ( !familiarStats ) { return false; }
	// Upgrade a legacy/recreated BAT_FAMILIAR before storing synchronized form
	// state so a cast cannot be lost in the tick between spawn and identification.
	if ( !isAmuletFamiliar(familiar, familiarStats) )
	{
		familiarStats->setAttribute("AMULET_FAMILIAR", "1");
		familiar->familiarIdentity = 1;
		familiar->familiarForm = FAMILIAR_FORM_BAT;
		familiar->familiarPendingForm = FAMILIAR_FORM_NONE;
		if ( multiplayer == SERVER )
		{
			serverUpdateEntitySkill(familiar, 57);
			serverUpdateEntitySkill(familiar, 58);
		}
	}
	if ( getAmuletFamiliarForm(familiar) != FAMILIAR_FORM_BAT )
	{
		transformAmuletFamiliar(familiar, FAMILIAR_FORM_BAT, true);
		return true;
	}
	if ( amuletFamiliarIsTooWeakToTransform(familiar, familiarStats)
		|| !amuletFamiliarPolymorphTargetIsValid(familiar, targetX, targetY) )
	{
		return false;
	}
	const int tilex = std::min(std::max(static_cast<int>(floor(targetX / 16.0)), 0), static_cast<int>(map.width) - 1);
	const int tiley = std::min(std::max(static_cast<int>(floor(targetY / 16.0)), 0), static_cast<int>(map.height) - 1);
	familiar->familiarTransformTargetX = tilex * 16.0 + 8.0;
	familiar->familiarTransformTargetY = tiley * 16.0 + 8.0;
	familiar->familiarPendingForm = familiarTargetRequestsCauldron(
		familiar->familiarTransformTargetX, familiar->familiarTransformTargetY)
		? FAMILIAR_FORM_CAULDRON : FAMILIAR_FORM_MIMIC;
	return true;
}

bool updateAmuletFamiliarSpecialState(Entity* familiar, Stat* familiarStats)
{
	if ( !familiar || !familiarStats || !isAmuletFamiliar(familiar, familiarStats) )
	{
		return false;
	}
	if ( multiplayer == CLIENT )
	{
		if ( getAmuletFamiliarForm(familiar) == FAMILIAR_FORM_CAULDRON )
		{
			startAmuletFamiliarCauldronAmbience(familiar);
			animateAmuletFamiliarCauldron(familiar);
			return true;
		}
		return false;
	}
	Entity* owner = uidToEntity(familiarStats->leader_uid);
	Stat* ownerStats = owner ? owner->getStats() : nullptr;
	const int ownerPlayer = owner ? owner->isEntityPlayer() : -1;
	if ( !owner || owner->behavior != &actPlayer || !ownerStats || ownerStats->HP <= 0
		|| ownerPlayer < 0 || (ownerPlayer > 0 && client_disconnected[ownerPlayer])
		|| !ownerStats->amulet || ownerStats->amulet->type != AMULET_FAMILIAR
		|| ownerStats->amulet->status == BROKEN )
	{
		removeBatFamiliar(familiar);
		return true;
	}
	if ( multiplayer == SERVER && ownerStats->amulet
		&& familiar->ticks % TICKS_PER_SECOND == 0 )
	{
		serverSyncPersistentCustomItemStateForOwner(ownerStats->amulet);
	}

	const FamiliarForm form = getAmuletFamiliarForm(familiar);
	enforceAmuletFamiliarFormFlags(familiar, form);
	if ( form == FAMILIAR_FORM_MIMIC && ownerPlayer >= 0 && ownerPlayer < MAXPLAYERS
		&& openedChest[ownerPlayer] == familiar )
	{
		Entity* opener = players[ownerPlayer] ? players[ownerPlayer]->entity : nullptr;
		if ( !opener || entityDist(familiar, opener) > TOUCHRANGE )
		{
			familiar->closeChest();
		}
	}
	// Bat and Cauldron are invulnerable forms. Some effect paths write HP
	// directly instead of calling setHP()/modHP(); repair that state before the
	// generic monster death branch can repeatedly process this entity.
	if ( form != FAMILIAR_FORM_MIMIC && familiarStats->HP <= 0 )
	{
		familiarStats->MAXHP = std::max<Sint32>(1, familiarStats->MAXHP);
		familiarStats->HP = 1;
		familiarStats->OLDHP = 1;
	}
	const Sint32 polymorphMinHP = getAmuletFamiliarPolymorphMinHP(*familiarStats);
	if ( familiarStats->HP <= 0 || familiarStats->HP >= polymorphMinHP )
	{
		familiar->familiarLowHpRecoveryTicks = 0;
	}
	else if ( ++familiar->familiarLowHpRecoveryTicks >= 60 * TICKS_PER_SECOND )
	{
		familiar->familiarLowHpRecoveryTicks = 0;
		const Sint32 healAmount = std::max<Sint32>(1,
			familiarStats->MAXHP / 10 + (familiarStats->MAXHP % 10 != 0));
		const Sint32 targetHP = std::min<Sint32>(polymorphMinHP,
			familiarStats->HP + healAmount);
		familiar->modHP(targetHP - familiarStats->HP);
		syncAmuletFamiliarPersistentState(familiar, ownerStats->amulet, true);
	}
	if ( form != FAMILIAR_FORM_BAT && entityDist(familiar, owner) > FAMILIAR_MAX_FORM_DISTANCE )
	{
		transformAmuletFamiliar(familiar, FAMILIAR_FORM_BAT, true);
		return true;
	}
	if ( form == FAMILIAR_FORM_CAULDRON )
	{
		if ( !familiarCauldronHasActiveHeat(familiar) )
		{
			transformAmuletFamiliar(familiar, FAMILIAR_FORM_BAT, true);
			return true;
		}
		familiar->vel_x = 0.0;
		familiar->vel_y = 0.0;
		startAmuletFamiliarCauldronAmbience(familiar);
		animateAmuletFamiliarCauldron(familiar);
		return true;
	}

	if ( form == FAMILIAR_FORM_BAT
		&& familiar->familiarPendingForm >= FAMILIAR_FORM_MIMIC
		&& familiar->familiarPendingForm <= FAMILIAR_FORM_CAULDRON )
	{
		const real_t dx = familiar->familiarTransformTargetX - familiar->x;
		const real_t dy = familiar->familiarTransformTargetY - familiar->y;
		const real_t distance = sqrt(dx * dx + dy * dy);
		if ( distance <= FAMILIAR_TRANSFORM_TRAVEL_SPEED )
		{
			familiar->x = familiar->familiarTransformTargetX;
			familiar->y = familiar->familiarTransformTargetY;
			familiar->vel_x = 0.0;
			familiar->vel_y = 0.0;
			const FamiliarForm requested = static_cast<FamiliarForm>(familiar->familiarPendingForm);
			familiar->familiarPendingForm = FAMILIAR_FORM_NONE;
			if ( requested == FAMILIAR_FORM_MIMIC
				&& !familiarCanBecomeMimicAtTile(
					static_cast<int>(floor(familiar->x / 16.0)),
					static_cast<int>(floor(familiar->y / 16.0))) )
			{
				return true;
			}
			transformAmuletFamiliar(familiar, requested, true);
			return true;
		}
		familiar->yaw = atan2(dy, dx);
		const real_t step = std::min(FAMILIAR_TRANSFORM_TRAVEL_SPEED, distance);
		familiar->vel_x = dx / distance * step;
		familiar->vel_y = dy / distance * step;
		familiar->x += familiar->vel_x;
		familiar->y += familiar->vel_y;
		batAnimate(familiar, familiarStats, step);
		return true;
	}
	if ( form == FAMILIAR_FORM_BAT )
	{
		Entity* target = familiar->familiarIngredientTargetUid
			? uidToEntity(familiar->familiarIngredientTargetUid) : nullptr;
		auto validTarget = [owner](Entity* candidate)
		{
			return candidate && candidate->behavior == &actItem
				&& itemTypeIsIngredientBag(static_cast<ItemType>(candidate->skill[10]))
				&& candidate->itemIngredientBagOwnerUid != 0
				&& static_cast<Uint32>(candidate->itemIngredientBagOwnerUid) == owner->getUID();
		};
		if ( !validTarget(target) )
		{
			familiar->familiarIngredientTargetUid = 0;
			real_t nearest = std::numeric_limits<real_t>::max();
			for ( node_t* node = map.entities->first; node; node = node->next )
			{
				Entity* candidate = static_cast<Entity*>(node->element);
				if ( !validTarget(candidate) ) { continue; }
				const real_t distance = entityDist(familiar, candidate);
				if ( distance < nearest )
				{
					nearest = distance;
					target = candidate;
				}
			}
			if ( target ) { familiar->familiarIngredientTargetUid = target->getUID(); }
		}
		if ( target )
		{
			const real_t dx = target->x - familiar->x;
			const real_t dy = target->y - familiar->y;
			const real_t distance = sqrt(dx * dx + dy * dy);
			if ( distance <= FAMILIAR_INGREDIENT_PICKUP_RADIUS )
			{
				Item* bag = newItemFromEntity(target, true);
				if ( bag && !ingredientBagIsEmpty(*bag) )
				{
					mergeIngredientBagIntoVoidChest(*bag);
				}
				if ( bag ) { free(bag); }
				familiar->familiarIngredientTargetUid = 0;
				target->removeLightField();
				list_RemoveNode(target->mynode);
				return true;
			}
			if ( distance > 0.0 )
			{
				const real_t collectionSpeed = FAMILIAR_BAT_MOVEMENT_SPEED_MULTIPLIER
					* FAMILIAR_INGREDIENT_COLLECTION_SPEED_MULT;
				const real_t step = std::min(collectionSpeed, distance);
				familiar->yaw = atan2(dy, dx);
				familiar->vel_x = dx / distance * step;
				familiar->vel_y = dy / distance * step;
				familiar->x += familiar->vel_x;
				familiar->y += familiar->vel_y;
				batAnimate(familiar, familiarStats, step);
				return true;
			}
		}
	}
	return false;
}

void sweepAmuletFamiliarIngredientsOnLevelChange()
{
	if ( multiplayer == CLIENT ) { return; }
	for ( int player = 0; player < MAXPLAYERS; ++player )
	{
		if ( !players[player] || !players[player]->entity || !stats[player]
			|| !stats[player]->amulet || stats[player]->amulet->type != AMULET_FAMILIAR
			|| stats[player]->amulet->status == BROKEN )
		{
			continue;
		}
		Entity* owner = players[player]->entity;
		Entity* familiar = findAmuletFamiliar(owner);
		if ( !familiar ) { continue; }
		familiar->familiarPendingForm = FAMILIAR_FORM_NONE;
		familiar->familiarIngredientTargetUid = 0;
		if ( getAmuletFamiliarForm(familiar) != FAMILIAR_FORM_BAT )
		{
			transformAmuletFamiliar(familiar, FAMILIAR_FORM_BAT, false);
		}
		for ( node_t* node = map.entities->first, *next = nullptr; node; node = next )
		{
			next = node->next;
			Entity* groundBag = static_cast<Entity*>(node->element);
			if ( !groundBag || groundBag->behavior != &actItem
				|| !itemTypeIsIngredientBag(static_cast<ItemType>(groundBag->skill[10]))
				|| static_cast<Uint32>(groundBag->itemIngredientBagOwnerUid) != owner->getUID() )
			{
				continue;
			}
			Item* bag = newItemFromEntity(groundBag, true);
			if ( bag && !ingredientBagIsEmpty(*bag) )
			{
				mergeIngredientBagIntoVoidChest(*bag);
			}
			if ( bag ) { free(bag); }
			groundBag->removeLightField();
			list_RemoveNode(groundBag->mynode);
		}
	}
}

void initBat(Entity* my, Stat* myStats)
{
	node_t* node;
	const bool familiar = isBatFamiliar(my, myStats);
	const real_t scale = familiar ? 0.5 : 1.0;
	const Sint32 models[] = {
		familiar ? BAT_FAMILIAR_MODEL_BODY : 1408,
		familiar ? BAT_FAMILIAR_MODEL_HEAD : 1409,
		familiar ? BAT_FAMILIAR_MODEL_WING_LEFT : 1410,
		familiar ? BAT_FAMILIAR_MODEL_WING_RIGHT : 1411
	};

	my->z = 0;
	my->initMonster(models[0]);
	my->flags[INVISIBLE] = true; // hide the "AI" bodypart
	if ( familiar )
	{
		my->sizex = 1;
		my->sizey = 1;
		my->flags[PASSABLE] = true;
		my->flags[BLOCKSIGHT] = false;
	}
	if ( multiplayer != CLIENT )
	{
		MONSTER_SPOTSND = 666;
		MONSTER_SPOTVAR = 1;
		MONSTER_IDLESND = 667;
		MONSTER_IDLEVAR = 3;
	}

	if ( multiplayer != CLIENT && !MONSTER_INIT )
	{
		auto& rng = my->entity_rng ? *my->entity_rng : local_rng;

		if ( myStats != nullptr )
		{
			if ( !myStats->leader_uid )
			{
				myStats->leader_uid = 0;
			}

			if ( isMonsterStatsDefault(*myStats) )
			{
				myStats->STR += std::min(5, currentlevel / 5);
				myStats->DEX += std::min(3, currentlevel / 5);
				myStats->HP += std::min(30, 5 * (currentlevel / 5));
				myStats->MAXHP = myStats->HP;
				myStats->OLDHP = myStats->HP;
			}

			// apply random stat increases if set in stat_shared.cpp or editor
			setRandomMonsterStats(myStats, rng);

			// generate 6 items max, less if there are any forced items from boss variants
			int customItemsToGenerate = ITEM_CUSTOM_SLOT_LIMIT;

			// generates equipment and weapons if available from editor
			createMonsterEquipment(myStats, rng);

			// create any custom inventory items from editor if available
			createCustomInventory(myStats, customItemsToGenerate, rng);

			// count if any custom inventory items from editor
			int customItems = countCustomItems(myStats); //max limit of 6 custom items per entity.

			// count any inventory items set to default in edtior
			int defaultItems = countDefaultItems(myStats);

			my->setHardcoreStats(*myStats);
		}
	}

	// body
	Entity* entity = newEntity(models[0], 1, map.entities, nullptr); //Limb entity.
	entity->sizex = familiar ? 1 : 2;
	entity->sizey = familiar ? 1 : 2;
	entity->scalex = scale;
	entity->scaley = scale;
	entity->scalez = scale;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->z = 6;
	entity->flags[USERFLAG2] = monsterChangesColorWhenAlly(myStats, my) ? my->flags[USERFLAG2] : false;
	entity->focalx = limbs[BAT_SMALL][1][0] * scale;
	entity->focaly = limbs[BAT_SMALL][1][1] * scale;
	entity->focalz = limbs[BAT_SMALL][1][2] * scale;
	entity->behavior = &actBatLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// head
	entity = newEntity(models[1], 1, map.entities, nullptr); //Limb entity.
	entity->sizex = familiar ? 1 : 2;
	entity->sizey = familiar ? 1 : 2;
	entity->scalex = scale;
	entity->scaley = scale;
	entity->scalez = scale;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->flags[USERFLAG2] = monsterChangesColorWhenAlly(myStats, my) ? my->flags[USERFLAG2] : false;
	entity->focalx = limbs[BAT_SMALL][2][0] * scale;
	entity->focaly = limbs[BAT_SMALL][2][1] * scale;
	entity->focalz = limbs[BAT_SMALL][2][2] * scale;
	entity->behavior = &actBatLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// wingleft
	entity = newEntity(models[2], 1, map.entities, nullptr); //Limb entity.
	entity->sizex = familiar ? 1 : 2;
	entity->sizey = familiar ? 1 : 2;
	entity->scalex = scale;
	entity->scaley = scale;
	entity->scalez = scale;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->flags[USERFLAG2] = monsterChangesColorWhenAlly(myStats, my) ? my->flags[USERFLAG2] : false;
	entity->focalx = limbs[BAT_SMALL][3][0] * scale;
	entity->focaly = limbs[BAT_SMALL][3][1] * scale;
	entity->focalz = limbs[BAT_SMALL][3][2] * scale;
	entity->behavior = &actBatLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	// wingright
	entity = newEntity(models[3], 1, map.entities, nullptr); //Limb entity.
	entity->sizex = familiar ? 1 : 2;
	entity->sizey = familiar ? 1 : 2;
	entity->scalex = scale;
	entity->scaley = scale;
	entity->scalez = scale;
	entity->skill[2] = my->getUID();
	entity->flags[PASSABLE] = true;
	entity->flags[NOUPDATE] = true;
	entity->yaw = my->yaw;
	entity->flags[USERFLAG2] = monsterChangesColorWhenAlly(myStats, my) ? my->flags[USERFLAG2] : false;
	entity->focalx = limbs[BAT_SMALL][4][0] * scale;
	entity->focaly = limbs[BAT_SMALL][4][1] * scale;
	entity->focalz = limbs[BAT_SMALL][4][2] * scale;
	entity->behavior = &actBatLimb;
	entity->parent = my->getUID();
	node = list_AddNodeLast(&my->children);
	node->element = entity;
	node->deconstructor = &emptyDeconstructor;
	node->size = sizeof(Entity*);
	my->bodyparts.push_back(entity);

	if ( multiplayer == CLIENT || MONSTER_INIT )
	{
		return;
	}
}

void actBatLimb(Entity* my)
{
	my->actMonsterLimb(false);
}

void removeBatFamiliar(Entity* my)
{
	if ( !my || multiplayer == CLIENT )
	{
		return;
	}
	stopAmuletFamiliarCauldronAmbience(my);
	closeAmuletFamiliarVoidChest(my);
	closeStewingForCauldron(my);
	Stat* familiarStats = my->getStats();
	Entity* leader = familiarStats ? uidToEntity(familiarStats->leader_uid) : nullptr;
	Item* familiarAmulet = nullptr;
	if ( leader )
	{
		if ( Stat* leaderStats = leader->getStats() )
		{
			familiarAmulet = leaderStats->amulet;
		}
	}
	if ( !familiarAmulet )
	{
		familiarAmulet = findFamiliarAmuletItem(my->familiarInstanceId);
	}
	syncAmuletFamiliarPersistentState(my, familiarAmulet, false);
	if ( multiplayer == SERVER && familiarAmulet )
	{
		serverSyncPersistentCustomItemStateForOwner(familiarAmulet);
	}
	if ( leader )
	{
		if ( Stat* leaderStats = leader->getStats() )
		{
			for ( node_t* node = leaderStats->FOLLOWERS.first; node; node = node->next )
			{
				Uint32* uid = static_cast<Uint32*>(node->element);
				if ( uid && *uid == my->getUID() )
				{
					list_RemoveNode(node);
					break;
				}
			}
		}
		const int player = leader->isEntityPlayer();
		if ( player >= 0 && player < MAXPLAYERS && players[player] )
		{
			if ( multiplayer == SERVER && !players[player]->isLocalPlayer() )
			{
				serverRemoveClientFollower(player, my->getUID());
			}
			if ( players[player]->isLocalPlayer() )
			{
				if ( FollowerMenu[player].recentEntity == my )
				{
					FollowerMenu[player].recentEntity = nullptr;
				}
				if ( FollowerMenu[player].followerToCommand == my )
				{
					FollowerMenu[player].closeFollowerMenuGUI();
				}
			}
		}
	}
	my->removeMonsterDeathNodes();
	list_RemoveNode(my->mynode);
}

void batDie(Entity* my)
{
	int c;
	for ( c = 0; c < 4; c++ )
	{
		Entity* entity = spawnGib(my);
		if ( entity )
		{
			entity->skill[5] = 1; // poof

			switch ( c )
			{
			case 0:
				entity->sprite = 1408;
				break;
			case 1:
				entity->sprite = 1409;
				break;
			case 2:
				entity->sprite = 1410;
				break;
			case 3:
				entity->sprite = 1411;
				break;
			default:
				break;
			}

			serverSpawnGibForClient(entity);
		}
	}

	my->spawnBlood();

	playSoundEntity(my, 670 + local_rng.rand() % 2, 128);

	my->removeMonsterDeathNodes();

	list_RemoveNode(my->mynode);
	return;
}

#define BAT_BODY 2
#define BAT_HEAD 3
#define BAT_LEFTWING 4
#define BAT_RIGHTWING 5

#define BAT_FLOAT_X body->fskill[2]
#define BAT_FLOAT_Y body->fskill[3]
#define BAT_FLOAT_Z body->fskill[4]
#define BAT_FLOAT_ATK body->fskill[5]
#define BAT_REST_FLY_Z body->fskill[6]
#define BAT_REST_STATE body->skill[3]
#define BAT_REST_ROTATE body->fskill[7]

bool Entity::disturbBat(Entity* touched, bool takenDamage, bool doMessage)
{
	if ( monsterSpecialState == BAT_REST )
	{
		monsterSpecialState = BAT_REST_DISTURBED;
		serverUpdateEntitySkill(this, 33);

		monsterHitTime = HITRATE;

		setEffect(EFF_STUNNED, true, 10, false);
		if ( bodyparts.size() >= 1 )
		{
			auto& body = bodyparts[0];
			if ( body->z < -15 )
			{
				setEffect(EFF_STUNNED, true, 30, false);
			}
			else if ( body->z < -10 )
			{
				setEffect(EFF_STUNNED, true, 20, false);
			}
		}

		if ( touched )
		{
			lookAtEntity(*touched);
			if ( !uidToEntity(monsterTarget) )
			{
				if ( checkEnemy(touched) )
				{
					monsterAcquireAttackTarget(*touched, MONSTER_STATE_PATH, true);
				}
				if ( doMessage )
				{
					if ( touched->behavior == &actPlayer )
					{
						messagePlayerColor(touched->skill[2], MESSAGE_INTERACTION,
							makeColorRGB(255, 0, 0), Language::get(6252));
					}
				}
			}
		}
		return true;
	}
	return false;
}

void batAnimate(Entity* my, Stat* myStats, double dist)
{
	node_t* node;
	Entity* entity = nullptr;
	Entity* head = nullptr;
	int bodypart;
	const bool familiar = isBatFamiliar(my, myStats);
	const real_t visualScale = familiar ? 0.5 : 1.0;

	my->flags[INVISIBLE] = true; // hide the "AI" bodypart
	if ( familiar )
	{
		my->flags[PASSABLE] = true;
		my->flags[BLOCKSIGHT] = false;
	}

	my->sizex = familiar ? 1 : 2;
	my->sizey = familiar ? 1 : 2;

	my->focalx = limbs[BAT_SMALL][0][0] * visualScale;
	my->focaly = limbs[BAT_SMALL][0][1] * visualScale;
	my->focalz = limbs[BAT_SMALL][0][2] * visualScale;
	if ( multiplayer != CLIENT )
	{
		my->z = limbs[BAT_SMALL][5][2];
		if ( !myStats->getEffectActive(EFF_LEVITATING) )
		{
			myStats->setEffectActive(EFF_LEVITATING, 1);
			myStats->EFFECTS_TIMERS[EFF_LEVITATING] = 0;
		}
		if ( !my->isMobile() )
		{
			my->monsterRotate();
		}

		if ( !my->isUntargetableBat() )
		{
			my->creatureHandleLiftZ();
		}
	}

	if ( enableDebugKeys && (svFlags & SV_FLAG_CHEATS) )
	{
		if ( keystatus[SDLK_KP_5] )
		{
			my->yaw += 0.05;
		}
		if ( keystatus[SDLK_g] )
		{
			keystatus[SDLK_g] = 0;
			MONSTER_ATTACK = MONSTER_POSE_MELEE_WINDUP1;
			MONSTER_ATTACKTIME = 0;
		}
		/*if ( keystatus[SDLK_h] )
		{
			keystatus[SDLK_h] = 0;
			myStats->EFFECTS[EFF_STUNNED] = !myStats->EFFECTS[EFF_STUNNED];
			myStats->EFFECTS_TIMERS[EFF_STUNNED] = myStats->EFFECTS[EFF_STUNNED] ? -1 : 0;
		}*/
		if ( keystatus[SDLK_j] )
		{
			keystatus[SDLK_j] = 0;
			my->monsterSpecialState = my->monsterSpecialState == 0 ? BAT_REST : 0;
			if ( my->monsterSpecialState == BAT_REST )
			{
				my->monsterReleaseAttackTarget();
			}
			serverUpdateEntitySkill(my, 33);
		}
	}

	//Move bodyparts
	Entity* leftWing = nullptr;
	Entity* body = nullptr;
	for ( bodypart = 0, node = my->children.first; node != nullptr; node = node->next, ++bodypart )
	{
		if ( bodypart < BAT_BODY )
		{
			continue;
		}

		entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		entity->yaw = my->yaw;
		entity->scalex = visualScale;
		entity->scaley = visualScale;
		entity->scalez = visualScale;
		if ( body && bodypart != BAT_BODY )
		{
			entity->yaw += PI * sin(BAT_REST_ROTATE * PI / 2);
		}

		if ( bodypart == BAT_HEAD )
		{
			entity->fskill[0] = fmod(entity->fskill[0], 2 * PI);
			while ( entity->fskill[0] >= PI )
			{
				entity->fskill[0] -= 2 * PI;
			}
			while ( entity->fskill[0] < -PI )
			{
				entity->fskill[0] += 2 * PI;
			}
			if ( my->monsterSpecialState == BAT_REST && BAT_REST_STATE == 1 )
			{
				entity->fskill[0] = std::max(body->fskill[0], entity->fskill[0]);
				real_t speed = 0.2;
				if ( limbAngleWithinRange(entity->fskill[0], speed, 3 * PI / 4) )
				{
					entity->fskill[0] = 3 * PI / 4;
				}
				else
				{
					entity->fskill[0] += speed;
					entity->fskill[0] = std::min(entity->fskill[0], 3 * PI / 4);
				}
			}
			else if ( MONSTER_ATTACK == 0 )
			{
				real_t speed = -0.2;
				real_t setpoint = PI / 16;
				if ( entity->fskill[0] < (setpoint - 0.01) || limbAngleWithinRange(entity->fskill[0], speed, setpoint) )
				{
					entity->fskill[0] = setpoint;
				}
				else
				{
					entity->fskill[0] += speed;
					entity->fskill[0] = std::max(entity->fskill[0], setpoint);
				}
			}

			if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP1 || MONSTER_ATTACK == 1 )
			{
				entity->fskill[0] = PI / 16;
			}
		}
		else if ( bodypart == BAT_BODY )
		{
			body = entity;
			entity->fskill[0] = fmod(entity->fskill[0], 2 * PI);
			while ( entity->fskill[0] >= PI )
			{
				entity->fskill[0] -= 2 * PI;
			}
			while ( entity->fskill[0] < -PI )
			{
				entity->fskill[0] += 2 * PI;
			}
			if ( MONSTER_ATTACK == 0 )
			{
				BAT_FLOAT_ATK = 0.0;
			}

			if ( my->monsterSpecialState == BAT_REST && BAT_REST_STATE == 1 )
			{
				real_t speed = 0.2;
				if ( limbAngleWithinRange(entity->fskill[0], speed, PI / 2) )
				{
					entity->fskill[0] = PI / 2;
				}
				else
				{
					entity->fskill[0] += speed;
					entity->fskill[0] = std::min(entity->fskill[0], PI / 2);
				}
			}
			else if ( MONSTER_ATTACK == 0 )
			{
				real_t speed = -0.2;
				if ( entity->fskill[0] < -0.01 || limbAngleWithinRange(entity->fskill[0], speed, 0.0) )
				{
					entity->fskill[0] = 0.0;
				}
				else
				{
					entity->fskill[0] += speed;
					entity->fskill[0] = std::max(entity->fskill[0], 0.0);
				}
			}

			if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP1 || MONSTER_ATTACK == 1 )
			{
				if ( MONSTER_ATTACKTIME == 0 )
				{
					entity->fskill[0] = 0.0;
					entity->skill[1] = 0;
					BAT_FLOAT_ATK = 0.0;
				}
				else
				{
					if ( MONSTER_ATTACKTIME >= (int)limbs[BAT_SMALL][15][0] )
					{
						if ( MONSTER_ATTACKTIME == (int)limbs[BAT_SMALL][15][0] )
						{
							if ( multiplayer != CLIENT )
							{
								const Sint32 temp = MONSTER_ATTACKTIME;
								my->attack(1, 0, nullptr); // slop
								MONSTER_ATTACKTIME = temp;
							}
						}

						if ( entity->skill[1] == 0 )
						{
							real_t speed = limbs[BAT_SMALL][13][2];
							if ( limbAngleWithinRange(entity->fskill[0], -speed, -((PI / 2) + PI / 4)) )
							{
								entity->fskill[0] = -((PI / 2) + PI / 4);
								entity->skill[1] = 1;
							}
							else
							{
								entity->fskill[0] -= speed;
								entity->fskill[0] = std::max(entity->fskill[0], -((PI / 2) + PI / 4));
							}
						}
						else
						{
							real_t speed = limbs[BAT_SMALL][13][1];
							entity->fskill[0] += speed;
							entity->fskill[0] = std::min(entity->fskill[0], 0.0);
						}
					}
					else
					{
						real_t speed = limbs[BAT_SMALL][13][0];
						entity->fskill[0] -= speed;
						entity->fskill[0] = std::max(entity->fskill[0], -((PI / 2) + PI / 32));
					}

					if ( MONSTER_ATTACKTIME >= (int)limbs[BAT_SMALL][18][0] )
					{
						BAT_FLOAT_ATK -= limbs[BAT_SMALL][18][1];
						BAT_FLOAT_ATK = std::max(BAT_FLOAT_ATK, (real_t)limbs[BAT_SMALL][18][2]);
					}
					else if ( MONSTER_ATTACKTIME >= (int)limbs[BAT_SMALL][17][0] )
					{
						BAT_FLOAT_ATK += limbs[BAT_SMALL][17][1];
						BAT_FLOAT_ATK = std::min(BAT_FLOAT_ATK, (real_t)limbs[BAT_SMALL][17][2]);
					}
					else if ( MONSTER_ATTACKTIME >= (int)limbs[BAT_SMALL][16][0] )
					{
						BAT_FLOAT_ATK -= limbs[BAT_SMALL][16][1];
						BAT_FLOAT_ATK = std::max(BAT_FLOAT_ATK, (real_t)limbs[BAT_SMALL][16][2]);
					}
				}

				if ( MONSTER_ATTACKTIME >= (int)limbs[BAT_SMALL][15][1] )
				{
					MONSTER_ATTACK = 0;
				}
			}
		}
		else if ( bodypart == BAT_LEFTWING )
		{
			entity->fskill[1] = fmod(entity->fskill[1], 2 * PI);
			while ( entity->fskill[1] >= PI )
			{
				entity->fskill[1] -= 2 * PI;
			}
			while ( entity->fskill[1] < -PI )
			{
				entity->fskill[1] += 2 * PI;
			}
			entity->fskill[0] = fmod(entity->fskill[0], 2 * PI);
			while ( entity->fskill[0] >= PI )
			{
				entity->fskill[0] -= 2 * PI;
			}
			while ( entity->fskill[0] < -PI )
			{
				entity->fskill[0] += 2 * PI;
			}

			if ( my->monsterSpecialState == BAT_REST )
			{
				{
					real_t speed = -0.1;
					real_t setpoint = -5 * PI / 8;
					if ( limbAngleWithinRange(entity->fskill[1], speed, setpoint) )
					{
						entity->fskill[1] = setpoint;
					}
					else
					{
						entity->fskill[1] += speed;
						entity->fskill[1] = std::max(entity->fskill[1], setpoint);
					}
				}
				{
					real_t setpoint = 0.0;
					if ( entity->fskill[0] < 0.01 )
					{
						real_t speed = 0.1;
						if ( limbAngleWithinRange(entity->fskill[0], speed, setpoint) )
						{
							entity->fskill[0] = setpoint;
						}
						else
						{
							entity->fskill[0] += speed;
							entity->fskill[0] = std::min(entity->fskill[0], setpoint);
						}
					}
					else if ( entity->fskill[0] > 0.01 )
					{
						real_t speed = -0.1;
						if ( limbAngleWithinRange(entity->fskill[0], speed, setpoint) )
						{
							entity->fskill[0] = setpoint;
						}
						else
						{
							entity->fskill[0] += speed;
							entity->fskill[0] = std::max(entity->fskill[0], setpoint);
						}
					}
					else
					{
						entity->fskill[0] = setpoint;
					}
				}
			}

			if ( MONSTER_ATTACK == MONSTER_POSE_MELEE_WINDUP1 || MONSTER_ATTACK == 1 )
			{
				if ( MONSTER_ATTACKTIME == 0 )
				{
					entity->fskill[1] = -PI / 2;
					entity->skill[1] = 0;
				}


				if ( entity->skill[1] == 0 )
				{
					real_t speed = limbs[BAT_SMALL][14][0];
					if ( limbAngleWithinRange(entity->fskill[1], speed, 0.0) )
					{
						entity->fskill[1] = 0.0;
						entity->skill[1] = 1;
					}
					else
					{
						if ( entity->fskill[1] > 0.01 )
						{
							entity->fskill[1] -= speed;
						}
						else if ( entity->fskill[1] < -0.01 )
						{
							entity->fskill[1] += speed;
						}
					}
					entity->fskill[0] = body->fskill[0];
				}
				else if ( entity->skill[1] == 1 )
				{
					real_t speed = limbs[BAT_SMALL][14][0];
					if ( limbAngleWithinRange(entity->fskill[1], speed, PI / 4) )
					{
						entity->fskill[1] = PI / 4;
						entity->skill[1] = 2;
					}
					else
					{
						entity->fskill[1] += speed;
					}
					entity->fskill[0] = body->fskill[0];
				}
				else if ( entity->skill[1] == 2 )
				{
					real_t speed = limbs[BAT_SMALL][14][1];
					real_t setpoint = -PI / 2 - PI / 8;
					if ( limbAngleWithinRange(entity->fskill[1], -speed, setpoint) )
					{
						entity->fskill[1] = setpoint;
						entity->skill[1] = 3;
					}
					else
					{
						entity->fskill[1] -= speed;
					}
					entity->fskill[0] = body->fskill[0];
				}
				else if ( entity->skill[1] == 3 )
				{
					real_t speed = limbs[BAT_SMALL][14][2];
					if ( limbAngleWithinRange(entity->fskill[1], speed, 0.0) )
					{
						entity->fskill[1] = 0.0;
						entity->skill[1] = 4;

						entity->skill[0] = 0; // reset flap to raise
					}
					else
					{
						entity->fskill[1] += speed;
					}
					if ( limbAngleWithinRange(entity->fskill[0], speed * 2, 0.0) )
					{
						entity->fskill[0] = 0.0;
					}
					else
					{
						entity->fskill[0] += speed * 2;
					}
				}
			}
		}

		switch ( bodypart )
		{
			case BAT_BODY:
			{
				if ( my->monsterSpecialState == BAT_REST && BAT_REST_STATE == 1 )
				{
					real_t diff = std::max(0.002, (1.0 - BAT_REST_ROTATE) / 20.0);
					BAT_REST_ROTATE += diff;
					BAT_REST_ROTATE = std::min(1.0, BAT_REST_ROTATE);
				}
				else
				{
					real_t diff = std::max(0.05, BAT_REST_ROTATE / 20.0);
					BAT_REST_ROTATE -= diff;
					BAT_REST_ROTATE = std::max(0.0, BAT_REST_ROTATE);
				}
				entity->yaw += PI * sin(BAT_REST_ROTATE * PI / 2);

				entity->x += limbs[BAT_SMALL][6][0] * visualScale * cos(entity->yaw);
				entity->y += limbs[BAT_SMALL][6][1] * visualScale * sin(entity->yaw);
				entity->z += limbs[BAT_SMALL][6][2] * visualScale;
				entity->focalx = limbs[BAT_SMALL][1][0] * visualScale;
				entity->focaly = limbs[BAT_SMALL][1][1] * visualScale;
				entity->focalz = limbs[BAT_SMALL][1][2] * visualScale;

				if ( enableDebugKeys && (svFlags & SV_FLAG_CHEATS) )
				{
					if ( keystatus[SDLK_KP_6] )
					{
						entity->fskill[0] -= 0.05;
					}
					else if ( keystatus[SDLK_KP_4] )
					{
						entity->fskill[0] += 0.05;
					}
				}
				entity->pitch = entity->fskill[0];

				if ( enableDebugKeys && (svFlags & SV_FLAG_CHEATS) )
				{
					if ( keystatus[SDLK_KP_PLUS] )
					{
						keystatus[SDLK_KP_PLUS] = 0;
						entity->skill[0] = entity->skill[0] == 0 ? 1 : 0;
					}
				}
				if ( entity->skill[0] == 0 )
				{
					entity->fskill[1] += 0.1;
				}

				if ( my->monsterSpecialState == BAT_REST )
				{
					BAT_FLOAT_X = 0.0;
					BAT_FLOAT_Y = 0.0;
					BAT_FLOAT_Z = 0.0;

					int mapx = static_cast<int>(my->x) >> 4;
					int mapy = static_cast<int>(my->y) >> 4;
					if ( mapx >= 0 && mapx < map.width && mapy >= 0 && mapy < map.height )
					{
						int mapIndex = (mapy)*MAPLAYERS + (mapx) * MAPLAYERS * map.height;
						if ( !map.tiles[(MAPLAYERS - 1) + mapIndex] )
						{
							// no ceiling
							if ( BAT_REST_FLY_Z <= -19.0 )
							{
								BAT_REST_FLY_Z = -19.0;
								BAT_REST_STATE = 1;
							}
							else
							{
								real_t diff = std::max(0.05, (BAT_REST_FLY_Z - -19.0) / 25.0);
								BAT_REST_FLY_Z -= diff;
							}
						}
						else
						{
							if ( BAT_REST_FLY_Z <= -3.0 )
							{
								BAT_REST_FLY_Z = -3.0;
								BAT_REST_STATE = 1;
							}
							else
							{
								real_t diff = std::max(0.05, (BAT_REST_FLY_Z - -3.0) / 25.0);
								BAT_REST_FLY_Z -= diff;
							}
						}
					}
				}
				else
				{
					BAT_REST_STATE = 0;

					if ( multiplayer != CLIENT )
					{
						if ( my->monsterSpecialState == BAT_REST_DISTURBED )
						{
							my->monsterSpecialState = 0;
						}
					}

					if ( BAT_REST_FLY_Z >= 0.0 )
					{
						BAT_REST_FLY_Z = 0.0;
					}
					else
					{
						real_t diff = std::max(0.1, (-BAT_REST_FLY_Z) / 25.0);
						BAT_REST_FLY_Z += diff;
					}

					BAT_FLOAT_X = limbs[BAT_SMALL][10][0] * visualScale * sin(body->fskill[1] * limbs[BAT_SMALL][11][0]) * cos(entity->yaw + PI / 2);
					BAT_FLOAT_Y = limbs[BAT_SMALL][10][1] * visualScale * sin(body->fskill[1] * limbs[BAT_SMALL][11][1]) * sin(entity->yaw + PI / 2);
					BAT_FLOAT_Z = limbs[BAT_SMALL][10][2] * visualScale * sin(body->fskill[1] * limbs[BAT_SMALL][11][2]);
					real_t floatAtkZ = BAT_FLOAT_ATK < 0 ? 2 * sin(BAT_FLOAT_ATK * PI / 8) : 0.5 * sin(BAT_FLOAT_ATK * PI / 8);
					BAT_FLOAT_Z += floatAtkZ;
				}

				BAT_FLOAT_X += BAT_FLOAT_ATK * cos(entity->yaw);
				BAT_FLOAT_Y += BAT_FLOAT_ATK * sin(entity->yaw);


				entity->x += BAT_FLOAT_X;
				entity->y += BAT_FLOAT_Y;
				entity->z += BAT_FLOAT_Z;
				entity->z += BAT_REST_FLY_Z;
				break;
			}
			case BAT_HEAD:
				entity->x += limbs[BAT_SMALL][7][0] * visualScale * cos(entity->yaw);
				entity->y += limbs[BAT_SMALL][7][1] * visualScale * sin(entity->yaw);
				entity->z += limbs[BAT_SMALL][7][2] * visualScale;
				entity->focalx = limbs[BAT_SMALL][2][0] * visualScale;
				entity->focaly = limbs[BAT_SMALL][2][1] * visualScale;
				entity->focalz = limbs[BAT_SMALL][2][2] * visualScale;
				if ( enableDebugKeys && (svFlags & SV_FLAG_CHEATS) )
				{
					if ( keystatus[SDLK_KP_7] )
					{
						entity->fskill[0] -= 0.05;
					}
					else if ( keystatus[SDLK_KP_9] )
					{
						entity->fskill[0] += 0.05;
					}
				}
				entity->pitch = entity->fskill[0];

				if ( body )
				{
					entity->x += BAT_FLOAT_X;
					entity->y += BAT_FLOAT_Y;
					entity->z += BAT_FLOAT_Z;
					entity->z += BAT_REST_FLY_Z;
				}
				break;
			case BAT_LEFTWING:
				entity->x += limbs[BAT_SMALL][8][0] * visualScale * cos(entity->yaw + PI / 2);
				entity->y += limbs[BAT_SMALL][8][1] * visualScale * sin(entity->yaw + PI / 2);
				entity->z += limbs[BAT_SMALL][8][2] * visualScale;
				entity->focalx = limbs[BAT_SMALL][3][0] * visualScale;
				entity->focaly = limbs[BAT_SMALL][3][1] * visualScale;
				entity->focalz = limbs[BAT_SMALL][3][2] * visualScale;

				if ( enableDebugKeys && (svFlags & SV_FLAG_CHEATS) )
				{
					if ( keystatus[SDLK_KP_1] )
					{
						entity->fskill[0] -= 0.05;
					}
					else if ( keystatus[SDLK_KP_3] )
					{
						entity->fskill[0] += 0.05;
					}
					if ( keystatus[SDLK_KP_0] )
					{
						entity->fskill[1] -= 0.05;
					}
					else if ( keystatus[SDLK_KP_ENTER] )
					{
						entity->fskill[1] += 0.05;
					}

					if ( keystatus[SDLK_KP_MINUS] )
					{
						keystatus[SDLK_KP_MINUS] = 0;
						entity->skill[3] = entity->skill[3] == 0 ? 1 : 0;
					}
				}

				if ( (entity->skill[3] == 0) && MONSTER_ATTACK == 0 && !(my->monsterSpecialState == BAT_REST && BAT_REST_STATE == 1) )
				{
					if ( entity->fskill[1] >= 0.8 )
					{
						entity->skill[0] = 1;
					}
					if ( entity->fskill[1] <= -1.3 )
					{
						entity->skill[0] = 0;
					}
					if ( entity->skill[0] == 0 )
					{
						entity->fskill[1] += 0.35;
					}
					else if ( entity->skill[0] == 1)
					{
						entity->fskill[1] -= 0.35;
					}
				}
				entity->pitch = entity->fskill[0];
				entity->roll = entity->fskill[1];
				leftWing = entity;

				if ( body )
				{
					entity->x += BAT_FLOAT_X;
					entity->y += BAT_FLOAT_Y;
					entity->z += BAT_FLOAT_Z;
					entity->z += BAT_REST_FLY_Z;

					entity->x += limbs[BAT_SMALL][12][0] * visualScale * sin(body->pitch) * cos(entity->yaw);
					entity->y += limbs[BAT_SMALL][12][1] * visualScale * sin(body->pitch) * sin(entity->yaw);
					entity->z += limbs[BAT_SMALL][12][2] * visualScale * sin(body->pitch);
				}
				break;
			case BAT_RIGHTWING:
				entity->x += limbs[BAT_SMALL][9][0] * visualScale * cos(entity->yaw + PI / 2);
				entity->y += limbs[BAT_SMALL][9][1] * visualScale * sin(entity->yaw + PI / 2);
				entity->z += limbs[BAT_SMALL][9][2] * visualScale;
				entity->focalx = limbs[BAT_SMALL][4][0] * visualScale;
				entity->focaly = limbs[BAT_SMALL][4][1] * visualScale;
				entity->focalz = limbs[BAT_SMALL][4][2] * visualScale;
				if ( leftWing )
				{
					entity->fskill[0] = leftWing->fskill[0];
					entity->fskill[1] = -leftWing->fskill[1];
				}
				entity->pitch = entity->fskill[0];
				entity->roll = entity->fskill[1];

				if ( body )
				{
					entity->x += BAT_FLOAT_X;
					entity->y += BAT_FLOAT_Y;
					entity->z += BAT_FLOAT_Z;
					entity->z += BAT_REST_FLY_Z;

					entity->x += limbs[BAT_SMALL][12][0] * visualScale * sin(body->pitch) * cos(entity->yaw);
					entity->y += limbs[BAT_SMALL][12][1] * visualScale * sin(body->pitch) * sin(entity->yaw);
					entity->z += limbs[BAT_SMALL][12][2] * visualScale * sin(body->pitch);

				}
				break;
			default:
				break;
		}
	}

	if ( MONSTER_ATTACK > 0 )
	{
		MONSTER_ATTACKTIME++;
	}
	else if ( MONSTER_ATTACK == 0 )
	{
		MONSTER_ATTACKTIME = 0;
	}
	else
	{
		// do nothing, don't reset attacktime or increment it.
	}
}
