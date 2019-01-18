/*-------------------------------------------------------------------------------

	BARONY
	File: item_usage_funcs.cpp
	Desc: implements the functions that handle item usage (eg handle
	potion effects, zap a staff, whatever else you can think of).

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "items.hpp"
#include "sound.hpp"
#include "entity.hpp"
#include "magic/magic.hpp"
#include "interface/interface.hpp"
#include "scores.hpp"
#include "net.hpp"
#include "monster.hpp"
#include "player.hpp"
#include "collision.hpp"

void item_PotionWater(Item*& item, Entity* entity, Entity* usedBy)
{
	if ( !entity )
	{
		return;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->PROFICIENCIES[PRO_ALCHEMY];
		}
	}

	node_t* node;
	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return;
	}

	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION )
		{
			if ( player == clientnum )
			{
				messagePlayer(player, language[750]);
			}
			return;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player == clientnum )
		{
			messagePlayer(player, language[751]);
		}
		return;
	}

	if ( multiplayer != CLIENT ) // server/singleplayer
	{
		// play drink sound
		if ( item->beatitude > 0 )
		{
			if ( stats->type == GHOUL ||
				stats->type == LICH ||
				stats->type == LICH_FIRE ||
				stats->type == LICH_ICE ||
				stats->type == SHADOW ||
				stats->type == SKELETON ||
				stats->type == VAMPIRE )
			{
				//Blessed water damages undead.
				int damage = -(20 * item->beatitude);
				entity->modHP(damage);
				playSoundEntity(entity, 28, 64);
				playSoundEntity(entity, 249, 128);
				entity->setObituary(language[1533]);
			}
			else
			{
				entity->modHP(5);
				playSoundEntity(entity, 52, 64);
				playSoundEntity(entity, 168, 128);
				spawnMagicEffectParticles(entity->x, entity->y, entity->z, 169);
			}
		}
		else
		{
			if ( stats->type == VAMPIRE )
			{
				entity->modHP(-5);
				playSoundEntity(entity, 28, 64);
				playSoundEntity(entity, 249, 128);
				entity->setObituary(language[1533]);
			}
			else
			{
				playSoundEntity(entity, 52, 64);
			}
		}
		if ( player != clientnum )
		{
			consumeItem(item);
			return;
		}
	}

	if ( item->beatitude == 0 )
	{
		if ( stats->type == VAMPIRE )
		{
			Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
			messagePlayerColor(player, color, language[3183]);
			camera_shakex += .1;
			camera_shakey += 10;
		}
		else
		{
			messagePlayer(player, language[752]);
		}
	}
	else if ( item->beatitude > 0 )
	{
		if ( stats->type == SKELETON )
		{
			Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
			messagePlayerColor(player, color, language[3184]);
			camera_shakex += .1;
			camera_shakey += 10;
		}
		else if ( stats->type == GHOUL ||
			stats->type == LICH ||
			stats->type == LICH_FIRE ||
			stats->type == LICH_ICE ||
			stats->type == SHADOW ||
			stats->type == VAMPIRE )
		{
			Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
			messagePlayerColor(player, color, language[3183]);
			camera_shakex += .1;
			camera_shakey += 10;
		}
		else
		{
			messagePlayer(player, language[753]);
		}
	}
	else if ( item->beatitude < 0 )
	{
		if ( stats->type == VAMPIRE )
		{
			Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
			messagePlayerColor(player, color, language[3183]);
			camera_shakex += .1;
			camera_shakey += 10;
		}
		messagePlayer(player, language[755]);

		// randomly curse an unidentified item in the entity's inventory
		int items = 0;
		for ( node = stats->inventory.first; node != NULL; node = node->next )
		{
			Item* target = (Item*)node->element;
			if ( target->identified == false )
			{
				items++;
			}
		}
		if ( items == 0 )
		{
			consumeItem(item);
			return;
		}
		int itemToCurse = rand() % items;
		items = 0;
		for ( node = stats->inventory.first; node != NULL; node = node->next )
		{
			Item* target = (Item*)node->element;
			if ( target->identified == false )
			{
				if ( items == itemToCurse )
				{
					target->beatitude = std::max<Sint16>(-1, target->beatitude);
					break;
				}
				items++;
			}
		}
	}
	consumeItem(item);
}

void item_PotionBooze(Item*& item, Entity* entity, Entity* usedBy, bool shouldConsumeItem)
{
	if (!entity)
	{
		return;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->PROFICIENCIES[PRO_ALCHEMY];
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return;
	}

	if ( stats->amulet != nullptr )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION )
		{
			if ( player == clientnum )
			{
				messagePlayer(player, language[750]);
			}
			return;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player == clientnum )
		{
			messagePlayer(player, language[751]);
		}
		return;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item);
		return;
	}

	messagePlayer(player, language[758]);
	messagePlayer(player, language[759]);
	stats->EFFECTS[EFF_DRUNK] = true;
	if ( player >= 0 )
	{
		stats->EFFECTS_TIMERS[EFF_DRUNK] = 2400 + rand() % 1200;
		if ( stats->type != GOATMAN )
		{
			stats->EFFECTS_TIMERS[EFF_DRUNK] = std::max(300, stats->EFFECTS_TIMERS[EFF_DRUNK] - (entity->getPER() + entity->getCON()) * 40);
		}
		if ( stats->EFFECTS[EFF_WITHDRAWAL] )
		{
			int hangoverReliefDuration = EFFECT_WITHDRAWAL_BASE_TIME; // 8 minutes
			switch ( rand() % 3 )
			{
				case 0:
					hangoverReliefDuration += (TICKS_PER_SECOND * 60 + 8); // 8 + 8 minutes
					break;
				case 1:
					hangoverReliefDuration += (TICKS_PER_SECOND * 60 + 4); // 8 + 4 minutes
					break;
				case 2:
					// intentional fall through
				default:
					break;
			}
			entity->setEffect(EFF_WITHDRAWAL, false, hangoverReliefDuration, true);
			messagePlayerColor(player, SDL_MapRGB(mainsurface->format, 0, 255, 0), language[3250]);
		}
		else if ( stats->EFFECTS_TIMERS[EFF_WITHDRAWAL] > 0 && stats->EFFECTS_TIMERS[EFF_WITHDRAWAL] < EFFECT_WITHDRAWAL_BASE_TIME )
		{
			stats->EFFECTS_TIMERS[EFF_WITHDRAWAL] = EFFECT_WITHDRAWAL_BASE_TIME;
		}
	}
	else
	{
		stats->EFFECTS_TIMERS[EFF_DRUNK] = 2400 + rand() % 1200;
	}
	stats->HUNGER += 100;
	entity->modHP(5);
	serverUpdateEffects(player);

	// play drink sound
	playSoundEntity(entity, 52, 64);
	playSoundEntity(entity, 168, 128);
	spawnMagicEffectParticles(entity->x, entity->y, entity->z, 169);
	if ( shouldConsumeItem )
	{
		consumeItem(item);
	}
}

void item_PotionJuice(Item*& item, Entity* entity, Entity* usedBy)
{
	if (!entity)
	{
		return;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->PROFICIENCIES[PRO_ALCHEMY];
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return;
	}

	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION )
		{
			if ( player == clientnum )
			{
				messagePlayer(player, language[750]);
			}
			return;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player == clientnum )
		{
			messagePlayer(player, language[751]);
		}
		return;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item);
		return;
	}

	if ( item->beatitude < 0 )
	{
		//Cursed effect inebriates you.
		messagePlayer(player, language[2900]);
		messagePlayer(player, language[758]);
		messagePlayer(player, language[759]);
		stats->EFFECTS[EFF_DRUNK] = true;
		if ( player >= 0 )
		{
			stats->EFFECTS_TIMERS[EFF_DRUNK] = 1000 + rand() % 300;
			stats->EFFECTS_TIMERS[EFF_DRUNK] = std::max(300, stats->EFFECTS_TIMERS[EFF_DRUNK] - (entity->getPER() + entity->getCON()) * 40);
		}
		else
		{
			stats->EFFECTS_TIMERS[EFF_DRUNK] = 1000 + rand() % 300;
		}
		stats->HUNGER += 50;
		entity->modHP(5);
		serverUpdateEffects(player);
	}
	else
	{
		messagePlayer(player, language[760]);
		stats->HUNGER += 50;
		entity->modHP(5);
	}

	// play drink sound
	playSoundEntity(entity, 52, 64);
	playSoundEntity(entity, 168, 128);
	spawnMagicEffectParticles(entity->x, entity->y, entity->z, 169);
	consumeItem(item);
}

void item_PotionSickness(Item*& item, Entity* entity, Entity* usedBy)
{
	if (!entity)
	{
		return;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->PROFICIENCIES[PRO_ALCHEMY];
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return;
	}

	if ( entity == NULL )
	{
		return;
	}
	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION )
		{
			if ( player == clientnum )
			{
				messagePlayer(player, language[750]);
			}
			return;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player == clientnum )
		{
			messagePlayer(player, language[751]);
		}
		return;
	}
	if ( multiplayer == CLIENT || player == 0 )
	{
		camera_shakex += .1;
		camera_shakey += 10;
		if ( multiplayer == CLIENT )
		{
			consumeItem(item);
			return;
		}
	}

	messagePlayer(player, language[761]);
	entity->modHP(-2);
	stats->EFFECTS[EFF_POISONED] = true;
	playSoundEntity(entity, 28, 64);
	serverUpdateEffects(player);

	// set obituary
	entity->setObituary(language[1535]);

	// play drink sound
	playSoundEntity(entity, 52, 64);
	consumeItem(item);
}

void item_PotionConfusion(Item*& item, Entity* entity, Entity* usedBy)
{
	if (!entity)
	{
		return;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->PROFICIENCIES[PRO_ALCHEMY];
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return;
	}

	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION )
		{
			if ( player == clientnum )
			{
				messagePlayer(player, language[750]);
			}
			return;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player == clientnum )
		{
			messagePlayer(player, language[751]);
		}
		return;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item);
		return;
	}

	messagePlayer(player, language[762]);
	stats->EFFECTS[EFF_CONFUSED] = true;
	if ( player >= 0 )
	{
		stats->EFFECTS_TIMERS[EFF_CONFUSED] = std::max(300, 1800 - (entity->getPER() + entity->getCON()) * 20);
	}
	else
	{
		stats->EFFECTS_TIMERS[EFF_CONFUSED] = 1800;
	}
	if ( entity->behavior == &actMonster )
	{
		entity->monsterTarget = 0; // monsters forget what they're doing
	}
	serverUpdateEffects(player);

	// play drink sound
	playSoundEntity(entity, 52, 64);
	consumeItem(item);
}

void item_PotionCureAilment(Item*& item, Entity* entity, Entity* usedBy)
{
	if (!entity)
	{
		return;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->PROFICIENCIES[PRO_ALCHEMY];
		}
	}

	int player = -1;
	Stat* stats;
	int c;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return;
	}

	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION )
		{
			if ( player == clientnum )
			{
				messagePlayer(player, language[750]);
			}
			return;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player == clientnum )
		{
			messagePlayer(player, language[751]);
		}
		return;
	}
	if ( players[player]->entity->flags[BURNING] )
	{
		players[player]->entity->flags[BURNING] = false;
		serverUpdateEntityFlag(players[player]->entity, BURNING);
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item);
		return;
	}

	if ( item->beatitude < 0 )
	{
		messagePlayer(player, language[2900]);
	}

	Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
	messagePlayerColor(player, color, language[763]);
	for ( c = 0; c < NUMEFFECTS; c++ )   //This does a whole lot more than just cure ailments.
	{
		if ( !(c == EFF_VAMPIRICAURA && stats->EFFECTS_TIMERS[c] == -2) && c != EFF_WITHDRAWAL )
		{
			stats->EFFECTS[c] = false;
			stats->EFFECTS_TIMERS[c] = 0;
		}
	}

	if ( stats->EFFECTS[EFF_WITHDRAWAL] )
	{
		entity->setEffect(EFF_WITHDRAWAL, false, EFFECT_WITHDRAWAL_BASE_TIME, true);
	}

	if ( item->beatitude < 0 )
	{
		messagePlayer(player, language[2903]);
		stats->EFFECTS[EFF_POISONED] = true;
		stats->EFFECTS_TIMERS[EFF_POISONED] = std::max(200, 300 - entity->getCON() * 20);
	}

	serverUpdateEffects(player);

	// play drink sound
	playSoundEntity(entity, 52, 64);
	consumeItem(item);
}

void item_PotionBlindness(Item*& item, Entity* entity, Entity* usedBy)
{
	if (!entity)
	{
		return;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->PROFICIENCIES[PRO_ALCHEMY];
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return;
	}

	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION )
		{
			if ( player == clientnum )
			{
				messagePlayer(player, language[750]);
			}
			return;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player == clientnum )
		{
			messagePlayer(player, language[751]);
		}
		return;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item);
		return;
	}

	messagePlayer(player, language[765]);
	stats->EFFECTS[EFF_BLIND] = true;
	if ( player >= 0 )
	{
		stats->EFFECTS_TIMERS[EFF_BLIND] = 660 + rand() % 480;
		stats->EFFECTS_TIMERS[EFF_BLIND] = std::max(300, stats->EFFECTS_TIMERS[EFF_BLIND] - (entity->getPER() + entity->getCON()) * 5);
	}
	else
	{
		stats->EFFECTS_TIMERS[EFF_BLIND] = 660 + rand() % 480;
	}
	serverUpdateEffects(player);

	// play drink sound
	playSoundEntity(entity, 52, 64);
	consumeItem(item);
}

void item_PotionInvisibility(Item*& item, Entity* entity, Entity* usedBy)
{
	if (!entity)
	{
		return;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->PROFICIENCIES[PRO_ALCHEMY];
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return;
	}

	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION )
		{
			if ( player == clientnum )
			{
				messagePlayer(player, language[750]);
			}
			return;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player == clientnum )
		{
			messagePlayer(player, language[751]);
		}
		return;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item);
		return;
	}

	messagePlayer(player, language[766]);

	if ( !entity->isInvisible() )
	{
		for ( node_t* node = map.creatures->first; node != nullptr; node = node->next )
		{
			Entity* creature = (Entity*)node->element;
			if ( creature && creature->behavior == &actMonster && creature->monsterTarget == entity->getUID() )
			{
				if ( !creature->isBossMonsterOrBossMap() )
				{
					//Abort if invalid creature (boss, shopkeep, etc).
					real_t dist = entityDist(entity, creature);
					if ( dist > STRIKERANGE * 3 )
					{
						// lose track of invis target.
						creature->monsterReleaseAttackTarget();
					}
				}
			}
		}
	}
	stats->EFFECTS[EFF_INVISIBLE] = true;
	stats->EFFECTS_TIMERS[EFF_INVISIBLE] = 1800 + rand() % 1800;
	serverUpdateEffects(player);

	// play drink sound
	playSoundEntity(entity, 52, 64);
	consumeItem(item);
}

void item_PotionLevitation(Item*& item, Entity* entity, Entity* usedBy)
{
	if (!entity)
	{
		return;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->PROFICIENCIES[PRO_ALCHEMY];
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return;
	}

	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION )
		{
			if ( player == clientnum )
			{
				messagePlayer(player, language[750]);
			}
			return;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player == clientnum )
		{
			messagePlayer(player, language[751]);
		}
		return;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item);
		return;
	}

	if ( item->beatitude < 0 )
	{
		//Cursed effect slows you.
		messagePlayer(player, language[2900]);
		messagePlayer(player, language[2901]);
		stats->EFFECTS[EFF_SLOW] = true;
		stats->EFFECTS_TIMERS[EFF_SLOW] = 1800;
	}
	else
	{
		messagePlayer(player, language[767]);
		stats->EFFECTS[EFF_LEVITATING] = true;
		stats->EFFECTS_TIMERS[EFF_LEVITATING] = 1800;
	}
	serverUpdateEffects(player);

	// play drink sound
	playSoundEntity(entity, 52, 64);
	consumeItem(item);
}

void item_PotionSpeed(Item*& item, Entity* entity, Entity* usedBy)
{
	if (!entity)
	{
		return;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->PROFICIENCIES[PRO_ALCHEMY];
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return;
	}

	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION )
		{
			if ( player == clientnum )
			{
				messagePlayer(player, language[750]);
			}
			return;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player == clientnum )
		{
			messagePlayer(player, language[751]);
		}
		return;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item);
		return;
	}


	if ( item->beatitude < 0 )
	{
		messagePlayer(player, language[2900]);
		//Cursed effect slows you.
		if ( stats->EFFECTS[EFF_FAST] )
		{
			messagePlayer(player, language[769]);
			stats->EFFECTS[EFF_FAST] = false;
			stats->EFFECTS_TIMERS[EFF_FAST] = 0;
		}
		else
		{
			messagePlayer(player, language[2902]);
			stats->EFFECTS[EFF_SLOW] = true;
			stats->EFFECTS_TIMERS[EFF_SLOW] = 600;
		}
	}
	else
	{
		if ( !stats->EFFECTS[EFF_SLOW] )
		{
			messagePlayer(player, language[768]);
			stats->EFFECTS[EFF_FAST] = true;
			stats->EFFECTS_TIMERS[EFF_FAST] = 600;
		}
		else
		{
			messagePlayer(player, language[769]);
			stats->EFFECTS[EFF_SLOW] = false;
			stats->EFFECTS_TIMERS[EFF_SLOW] = 0;
		}
	}
	serverUpdateEffects(player);

	// play drink sound
	playSoundEntity(entity, 52, 64);
	consumeItem(item);
}

void item_PotionAcid(Item*& item, Entity* entity, Entity* usedBy)
{
	if (!entity)
	{
		return;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->PROFICIENCIES[PRO_ALCHEMY];
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return;
	}

	if ( entity == NULL )
	{
		return;
	}
	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION )
		{
			if ( player == clientnum )
			{
				messagePlayer(player, language[750]);
			}
			return;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player == clientnum )
		{
			messagePlayer(player, language[751]);
		}
		return;
	}
	if ( multiplayer == CLIENT || player == 0 )
	{
		camera_shakex += .1;
		camera_shakey += 10;
		if ( multiplayer == CLIENT )
		{
			consumeItem(item);
			return;
		}
	}

	messagePlayer(player, language[770]);
	entity->modHP(-8 - rand() % 3);
	playSoundEntity(entity, 28, 64);

	// set obituary
	entity->setObituary(language[1535]);

	if ( usedBy )
	{
		Entity* orbit = usedBy->castStationaryOrbitingMagicMissile(SPELL_FIREBALL, entity->x, entity->y, 16.0, 0.0, 40);
		orbit = usedBy->castStationaryOrbitingMagicMissile(SPELL_FIREBALL, entity->x, entity->y, 16.0, 2 * PI / 3, 40);
		orbit = usedBy->castStationaryOrbitingMagicMissile(SPELL_FIREBALL, entity->x, entity->y, 16.0, 4 * PI / 3, 40);
		spawnMagicEffectParticles(entity->x, entity->y, 0, 174);
	}

	// play drink sound
	playSoundEntity(entity, 52, 64);
	consumeItem(item);
}

void item_PotionParalysis(Item*& item, Entity* entity, Entity* usedBy)
{
	if (!entity)
	{
		return;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->PROFICIENCIES[PRO_ALCHEMY];
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return;
	}

	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION )
		{
			if ( player == clientnum )
			{
				messagePlayer(player, language[750]);
			}
			return;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player == clientnum )
		{
			messagePlayer(player, language[751]);
		}
		return;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item);
		return;
	}

	messagePlayer(player, language[771]);
	int effectDuration = 0;
	if ( player >= 0 )
	{
		effectDuration = 420 + rand() % 180;
		effectDuration = std::max(300, effectDuration - (entity->getCON()) * 5);
	}
	else
	{
		effectDuration = 420 + rand() % 180;
	}
	entity->setEffect(EFF_PARALYZED, true, effectDuration, false);
	serverUpdateEffects(player);

	// play drink sound
	playSoundEntity(entity, 52, 64);
	consumeItem(item);
}

void item_PotionHealing(Item*& item, Entity* entity, Entity* usedBy, bool shouldConsumeItem)
{
	if (!entity)
	{
		return;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->PROFICIENCIES[PRO_ALCHEMY];
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return;
	}

	if ( stats->amulet != nullptr )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION )
		{
			if ( player == clientnum )
			{
				messagePlayer(player, language[750]);
			}
			return;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player == clientnum )
		{
			messagePlayer(player, language[751]);
		}
		return;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item);
		return;
	}
	if ( stats->HP == stats->MAXHP )
	{
		playSoundEntity(entity, 52, 64);
		playSoundEntity(entity, 168, 128);
		spawnMagicEffectParticles(entity->x, entity->y, entity->z, 169);
		if ( item->beatitude < 0 )
		{
			messagePlayer(player, language[2900]);
			messagePlayer(player, language[2903]);
			stats->EFFECTS[EFF_POISONED] = true;
			stats->EFFECTS_TIMERS[EFF_POISONED] = std::max(200, 300 - entity->getCON() * 20);
		}
		else
		{
			messagePlayer(player, language[772]);
			// stop bleeding
			if ( stats->EFFECTS[EFF_BLEEDING] )
			{
				entity->setEffect(EFF_BLEEDING, false, 0, false);
			}
		}
		if ( shouldConsumeItem )
		{
			consumeItem(item);
		}
		return;
	}

	int amount = std::max(7 + item->status, 0);
	int multiplier = std::max(5, item->beatitude + 5);

	amount *= multiplier / 5.f;
	if ( stats->type == GOATMAN && entity->behavior == &actMonster )
	{
		amount *= GOATMAN_HEALINGPOTION_MOD; //Goatman special.
		stats->EFFECTS[EFF_FAST] = true;
		stats->EFFECTS_TIMERS[EFF_FAST] = GOATMAN_HEALING_POTION_SPEED_BOOST_DURATION;
	}

	//Bonus from CON, to scale up healing potions as the game progresses.
	if ( stats->CON > 0 )
	{
		amount += 2 * stats->CON;
	}

	if ( item->beatitude < 0 )
	{
		amount /= (std::abs(item->beatitude) * 2);
	}

	int oldHP = entity->getHP();

	entity->modHP(amount);

	int heal = std::max(entity->getHP() - oldHP, 0);
	if ( heal > 0 )
	{
		serverUpdatePlayerGameplayStats(player, STATISTICS_HEAL_BOT, heal);
	}

	// play drink sound
	playSoundEntity(entity, 52, 64);
	playSoundEntity(entity, 168, 128);
	spawnMagicEffectParticles(entity->x, entity->y, entity->z, 169);
	Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);

	if ( item->beatitude < 0 )
	{
		messagePlayer(player, language[2900]);
		messagePlayer(player, language[2903]);
		stats->EFFECTS[EFF_POISONED] = true;
		stats->EFFECTS_TIMERS[EFF_POISONED] = std::max(200, 300 - entity->getCON() * 20);
	}
	else
	{
		messagePlayerColor(player, color, language[773]);
		// stop bleeding
		if ( stats->EFFECTS[EFF_BLEEDING] )
		{
			entity->setEffect(EFF_BLEEDING, false, 0, false);
		}
	}

	if ( shouldConsumeItem )
	{
		consumeItem(item);
	}
}

void item_PotionExtraHealing(Item*& item, Entity* entity, Entity* usedBy, bool shouldConsumeItem)
{
	if (!entity)
	{
		return;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->PROFICIENCIES[PRO_ALCHEMY];
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return;
	}

	if ( stats->amulet != nullptr )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION )
		{
			if ( player == clientnum )
			{
				messagePlayer(player, language[750]);
			}
			return;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player == clientnum )
		{
			messagePlayer(player, language[751]);
		}
		return;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item);
		return;
	}
	if ( stats->HP == stats->MAXHP )
	{
		playSoundEntity(entity, 52, 64);
		playSoundEntity(entity, 168, 128);
		spawnMagicEffectParticles(entity->x, entity->y, entity->z, 169);
		if ( item->beatitude < 0 )
		{
			messagePlayer(player, language[2900]);
			messagePlayer(player, language[2903]);
			stats->EFFECTS[EFF_POISONED] = true;
			stats->EFFECTS_TIMERS[EFF_POISONED] = std::max(200, 300 - entity->getCON() * 20);
		}
		else
		{
			messagePlayer(player, language[772]);
			// stop bleeding
			if ( stats->EFFECTS[EFF_BLEEDING] )
			{
				entity->setEffect(EFF_BLEEDING, false, 0, false);
			}
		}
		if ( shouldConsumeItem )
		{
			consumeItem(item);
		}
		return;
	}

	int amount = std::max(15 + item->status, 0);
	int multiplier = std::max(5, item->beatitude + 5);

	amount *= multiplier;
	if ( stats->type == GOATMAN && entity->behavior == &actMonster )
	{
		amount *= GOATMAN_HEALINGPOTION_MOD; //Goatman special.
		stats->EFFECTS[EFF_FAST] = true;
		stats->EFFECTS_TIMERS[EFF_FAST] = GOATMAN_HEALING_POTION_SPEED_BOOST_DURATION;
	}

	//Bonus from CON, to scale up healing potions as the game progresses.
	if ( stats->CON > 0 )
	{
		amount += 4 * stats->CON;
	}

	if ( item->beatitude < 0 )
	{
		amount /= (std::abs(item->beatitude) * 2);
	}

	int oldHP = entity->getHP();

	entity->modHP(amount);

	int heal = std::max(entity->getHP() - oldHP, 0);
	if ( heal > 0 )
	{
		serverUpdatePlayerGameplayStats(player, STATISTICS_HEAL_BOT, heal);
	}

	// play drink sound
	playSoundEntity(entity, 52, 64);
	playSoundEntity(entity, 168, 128);
	spawnMagicEffectParticles(entity->x, entity->y, entity->z, 169);
	Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
	if ( item->beatitude < 0 )
	{
		messagePlayer(player, language[2900]);
		messagePlayer(player, language[2903]);
		stats->EFFECTS[EFF_POISONED] = true;
		stats->EFFECTS_TIMERS[EFF_POISONED] = std::max(300, 600 - entity->getCON() * 20);
	}
	else
	{
		messagePlayerColor(player, color, language[773]);
		// stop bleeding
		if ( stats->EFFECTS[EFF_BLEEDING] )
		{
			entity->setEffect(EFF_BLEEDING, false, 0, false);
		}
	}

	if ( shouldConsumeItem )
	{
		consumeItem(item);
	}
}

void item_PotionRestoreMagic(Item*& item, Entity* entity, Entity* usedBy)
{
	if (!entity)
	{
		return;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->PROFICIENCIES[PRO_ALCHEMY];
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return;
	}

	if ( entity == NULL )
	{
		return;
	}
	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION )
		{
			if ( player == clientnum )
			{
				messagePlayer(player, language[750]);
			}
			return;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player == clientnum )
		{
			messagePlayer(player, language[751]);
		}
		return;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item);
		return;
	}
	if ( stats->MP == stats->MAXMP )
	{
		playSoundEntity(entity, 52, 64);
		if ( item->beatitude < 0 )
		{
			messagePlayer(player, language[774]);
			messagePlayer(player, language[2902]);
			stats->EFFECTS[EFF_SLOW] = true;
			stats->EFFECTS_TIMERS[EFF_SLOW] = 600;
		}
		else
		{
			messagePlayer(player, language[772]);
		}
		consumeItem(item);
		return;
	}

	int amount = std::max(7 + item->status, 0);
	int multiplier = std::max(5, item->beatitude + 5);

	amount *= multiplier;

	if ( stats->INT > 0 )
	{
		amount += std::min(30, 2 * stats->INT); // extra mana scaling from 1 to 15 INT, capped at +30 MP
	}

	if ( item->beatitude < 0 )
	{
		amount /= (std::abs(item->beatitude) * 2);
		messagePlayer(player, language[774]);
		messagePlayer(player, language[2902]);
		stats->EFFECTS[EFF_SLOW] = true;
		stats->EFFECTS_TIMERS[EFF_SLOW] = 600;
	}
	else
	{
		messagePlayer(player, language[774]);
	}
	entity->modMP(amount);

	// play drink sound
	playSoundEntity(entity, 52, 64);

	consumeItem(item);
}

Entity* item_PotionPolymorph(Item*& item, Entity* entity, Entity* usedBy)
{
	if ( !entity )
	{
		return nullptr;
	}

	int skillLVL = 0;
	if ( multiplayer != CLIENT && usedBy && usedBy->behavior == &actPlayer )
	{
		Stat* usedByStats = usedBy->getStats();
		if ( usedByStats )
		{
			skillLVL = usedByStats->PROFICIENCIES[PRO_ALCHEMY];
		}
	}

	int player = -1;
	Stat* stats;

	if ( entity->behavior == &actPlayer )
	{
		player = entity->skill[2];
	}
	stats = entity->getStats();
	if ( !stats )
	{
		return nullptr;
	}

	if ( stats->amulet != NULL )
	{
		if ( stats->amulet->type == AMULET_STRANGULATION )
		{
			if ( player == clientnum )
			{
				messagePlayer(player, language[750]);
			}
			return nullptr;
		}
	}
	if ( stats->EFFECTS[EFF_VOMITING] )
	{
		if ( player == clientnum )
		{
			messagePlayer(player, language[751]);
		}
		return nullptr;
	}
	if ( multiplayer == CLIENT )
	{
		consumeItem(item);
		return nullptr;
	}

	// play drink sound
	if ( !usedBy ) // drinking rather than throwing.
	{
		playSoundEntity(entity, 52, 64);
		if ( player >= 0 )
		{
			messagePlayer(player, language[3190]);
		}
	}
	Entity* transformedEntity = false;

	if ( entity->behavior == &actMonster || entity->behavior == &actPlayer )
	{
		transformedEntity = spellEffectPolymorph(entity, stats, usedBy);
	}

	consumeItem(item);

	return transformedEntity;
}

void item_ScrollMail(Item* item, int player)
{
	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	if ( player != clientnum )
	{
		return;
	}

	if (players[player]->entity->isBlind())
	{
		messagePlayer(player, language[775]);
		return;
	}

	if ( player == clientnum )
	{
		conductIlliterate = false;
	}
	item->identified = 1;
	switch ( item->appearance % 25 )
	{
		case 0:
			messagePlayer(player, language[776]);
			break;
		case 1:
			messagePlayer(player, language[780]);
			messagePlayer(player, language[781]);
			break;
		case 2:
			messagePlayer(player, language[786]);
			break;
		case 3:
			messagePlayer(player, language[790]);
			break;
		case 4:
			messagePlayer(player, language[793], language[158 + item->appearance % 26], language[184 + item->appearance % 9]);
			break;
		case 5:
			messagePlayer(player, language[796]);
			break;
		case 6:
			messagePlayer(player, language[799]);
			break;
		case 7:
			messagePlayer(player, language[801]);
			messagePlayer(player, language[802]);
			break;
		case 8:
			messagePlayer(player, language[807]);
			break;
		case 9:
			messagePlayer(player, language[811]);
			messagePlayer(player, language[812]);
			break;
		case 10:
			messagePlayer(player, language[816]);
			break;
		case 11:
			messagePlayer(player, language[817]);
			break;
		case 12:
			messagePlayer(player, language[822]);
			break;
		case 13:
			messagePlayer(player, language[824]);
			break;
		case 14:
			messagePlayer(player, language[826]);
			break;
		case 15:
			messagePlayer(player, language[828]);
			break;
		case 16:
			messagePlayer(player, language[830]);
			break;
		case 17:
			messagePlayer(player, language[834]);
			break;
		case 18:
			messagePlayer(player, language[836]);
			break;
		case 19:
			messagePlayer(player, language[838]);
			break;
		case 20:
			messagePlayer(player, language[840]);
			break;
		case 21:
			messagePlayer(player, language[843]);
			break;
		default:
			messagePlayer(player, language[846]);
			break;
	}
}

void item_ScrollIdentify(Item* item, int player)
{
	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	if (player != clientnum)
	{
		return;
	}

	if (players[player]->entity->isBlind())
	{
		messagePlayer(player, language[775]);
		return;
	}

	//identifygui_mode = true;
	identifygui_active = true;
	identifygui_appraising = false;
	shootmode = false;
	gui_mode = GUI_MODE_INVENTORY; //Reset the GUI to the inventory.

	if ( removecursegui_active )
	{
		closeRemoveCurseGUI();
	}

	GenericGUI.closeGUI();

	if ( openedChest[clientnum] )
	{
		openedChest[clientnum]->closeChest();
	}

	//Initialize Identify GUI game controller code here.
	initIdentifyGUIControllerCode();
}

void item_ScrollLight(Item* item, int player)
{
	int c;

	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	if (multiplayer == CLIENT)
	{
		return;
	}

	if (players[player]->entity->isBlind())
	{
		messagePlayer(player, language[775]);
		return;
	}

	if ( player == clientnum )
	{
		conductIlliterate = false;
	}
	item->identified = 1;
	messagePlayer(player, language[848]);

	messagePlayer(player, language[851]);
	lightSphereShadow(players[player]->entity->x / 16, players[player]->entity->y / 16, 8, 150);

	// send new light info to clients
	if (multiplayer == SERVER)
	{
		for (c = 1; c < MAXPLAYERS; c++)
		{
			if (client_disconnected[c] == true)
			{
				continue;
			}
			strcpy((char*)net_packet->data, "LITS");
			SDLNet_Write16(players[player]->entity->x / 16, &net_packet->data[4]);
			SDLNet_Write16(players[player]->entity->y / 16, &net_packet->data[6]);
			SDLNet_Write16(8, &net_packet->data[8]);
			SDLNet_Write16(150, &net_packet->data[10]);
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 12;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}
}

void item_ScrollBlank(Item* item, int player)
{
	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	if (player != clientnum)
	{
		return;
	}

	if (players[player]->entity->isBlind())
	{
		messagePlayer(player, language[775]);
		return;
	}

	if (player == clientnum)
	{
		conductIlliterate = false;
	}
	item->identified = 1;
	messagePlayer(player, language[852]);
}

void item_ScrollEnchantWeapon(Item* item, int player)
{
	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	if (players[player]->entity->isBlind())
	{
		if (player == clientnum)
		{
			messagePlayer(player, language[775]);
		}
		return;
	}

	if (player == clientnum)
	{
		conductIlliterate = false;
	}
	item->identified = 1;
	if (player == clientnum)
	{
		messagePlayer(player, language[848]);
	}

	Item** toEnchant = nullptr;
	bool hasMeleeGloves = false;
	if ( stats[player]->gloves )
	{
		switch ( stats[player]->gloves->type )
		{
			case BRASS_KNUCKLES:
			case IRON_KNUCKLES:
			case SPIKED_GAUNTLETS:
				hasMeleeGloves = true;
				break;
			default:
				break;
		}
	}

	if ( stats[player]->weapon )
	{
		toEnchant = &stats[player]->weapon;
	}
	else if ( hasMeleeGloves )
	{
		toEnchant = &stats[player]->gloves;
	}

	if ( toEnchant == nullptr)
	{
		if (player == clientnum)
		{
			messagePlayer(player, language[853]);
		}
	}
	else
	{
		if (item->beatitude < 0)
		{
			if (player == clientnum)
			{
				if ( toEnchant == &stats[player]->gloves )
				{
					messagePlayer(player, language[858], (*toEnchant)->getName());
				}
				else
				{
					messagePlayer(player, language[854]);
				}
			}
			if ( (*toEnchant)->beatitude > 0 )
			{
				(*toEnchant)->beatitude = -(*toEnchant)->beatitude;
			}
			else
			{
				(*toEnchant)->beatitude -= 1;
			}
		}
		else
		{
			if (player == clientnum)
			{
				if (item->beatitude == 0)
				{
					if ( toEnchant == &stats[player]->gloves )
					{
						messagePlayer(player, language[859], (*toEnchant)->getName());
					}
					else
					{
						messagePlayer(player, language[855]);
					}
				}
				else
				{
					if ( toEnchant == &stats[player]->gloves )
					{
						messagePlayer(player, language[860], (*toEnchant)->getName());
					}
					else
					{
						messagePlayer(player, language[856]);
					}
				}
			}
			(*toEnchant)->beatitude += 1 + item->beatitude;
		}
	}
}

void item_ScrollEnchantArmor(Item* item, int player)
{
	Item* armor = nullptr;
	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	if ( !item )
	{
		return;
	}

	if (players[player]->entity->isBlind())
	{
		if (player == clientnum)
		{
			messagePlayer(player, language[775]);
		}
		return;
	}

	if (player == clientnum)
	{
		conductIlliterate = false;
	}
	item->identified = 1;
	if (player == clientnum)
	{
		messagePlayer(player, language[848]);
	}

	int armornum = rand() % 6;
	int startIndex = armornum;
	bool breakloop = false;
	while ( !armor && !breakloop )
	{
		switch ( armornum )
		{
			// intentional fall throughs...
			case 0:
				if ( stats[player]->helmet != nullptr )
				{
					armor = stats[player]->helmet;
					break;
				}
			case 1:
				if ( stats[player]->breastplate != nullptr )
				{
					armor = stats[player]->breastplate;
					break;
				}
			case 2:
				if ( stats[player]->gloves != nullptr )
				{
					armor = stats[player]->gloves;
					break;
				}
			case 3:
				if ( stats[player]->shoes != nullptr )
				{
					armor = stats[player]->shoes;
					break;
				}
			case 4:
				if ( stats[player]->shield != nullptr )
				{
					armor = stats[player]->shield;
					break;
				}
			case 5:
				if ( stats[player]->cloak != nullptr )
				{
					armor = stats[player]->cloak;
					break;
				}
				++armornum;
				if ( armornum > 5 )
				{
					// loop back around.
					armornum = 0;
				}
				if ( armornum == startIndex )
				{
					// couldn't find a piece of armor, break.
					breakloop = true;
					armor = nullptr;
					break;
				}
			default:
				break;
		}
	}

	if (armor == nullptr)
	{
		if (player == clientnum)
		{
			messagePlayer(player, language[857]);
		}
	}
	else if ( armor != nullptr )
	{
		if (item->beatitude < 0)
		{
			if (player == clientnum)
			{
				messagePlayer(player, language[858], armor->getName());
			}
			if ( armor->beatitude > 0 )
			{
				armor->beatitude = -armor->beatitude;
			}
			else
			{
				armor->beatitude -= 1;
			}
		}
		else
		{
			if (player == clientnum)
			{
				if (item->beatitude == 0)
				{
					messagePlayer(player, language[859], armor->getName());
				}
				else
				{
					messagePlayer(player, language[860], armor->getName());
				}
			}
			armor->beatitude += 1 + item->beatitude;
		}
	}
}

void item_ScrollRemoveCurse(Item* item, int player)
{
	Item* target = nullptr;

	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	if ( player != clientnum )
	{
		return;
	}

	if (players[player]->entity->isBlind())
	{
		if (player == clientnum)
		{
			messagePlayer(player, language[775]);
		}
		return;
	}

	if (item->beatitude >= 0)
	{
		// Uncurse an item
		shootmode = false;
		gui_mode = GUI_MODE_INVENTORY; // Reset the GUI to the inventory.
		removecursegui_active = true;
		if ( identifygui_active )
		{
			CloseIdentifyGUI();
		}
		GenericGUI.closeGUI();

		if ( openedChest[player] )
		{
			openedChest[player]->closeChest();
		}

		initRemoveCurseGUIControllerCode();

		return;
	}
	else
	{
		// choose a random piece of worn equipment to curse!
		int armornum = rand() % 7;
		int startIndex = armornum;
		bool breakloop = false;
		target = nullptr;
		while ( !target && !breakloop )
		{
			switch ( armornum )
			{
				// intentional fall throughs...
				case 0:
					if ( stats[player]->helmet != nullptr && stats[player]->helmet->beatitude >= 0 )
					{
						target = stats[player]->helmet;
						break;
					}
				case 1:
					if ( stats[player]->breastplate != nullptr && stats[player]->breastplate->beatitude >= 0 )
					{
						target = stats[player]->breastplate;
						break;
					}
				case 2:
					if ( stats[player]->gloves != nullptr && stats[player]->gloves->beatitude >= 0 )
					{
						target = stats[player]->gloves;
						break;
					}
				case 3:
					if ( stats[player]->shoes != nullptr && stats[player]->shoes->beatitude >= 0 )
					{
						target = stats[player]->shoes;
						break;
					}
				case 4:
					if ( stats[player]->shield != nullptr && stats[player]->shield->beatitude >= 0 )
					{
						target = stats[player]->shield;
						break;
					}
				case 5:
					if ( stats[player]->cloak != nullptr && stats[player]->cloak->beatitude >= 0 )
					{
						target = stats[player]->cloak;
						break;
					}
				case 6:
					if ( stats[player]->weapon != nullptr && stats[player]->weapon->beatitude >= 0 )
					{
						target = stats[player]->weapon;
						break;
					}
					++armornum;
					if ( armornum > 6 )
					{
						// loop back around.
						armornum = 0;
					}
					if ( armornum == startIndex )
					{
						// couldn't find a piece of armor, break.
						breakloop = true;
						target = nullptr;
						break;
					}
				default:
					break;
			}
		}
		if ( target )
		{
			if ( target->beatitude == 0 )
			{
				--target->beatitude;
			}
			else
			{
				target->beatitude = -target->beatitude;
			}
			if ( player == clientnum )
			{
				messagePlayer(player, language[858], target->getName());
			}
		}
		else if (player == clientnum)
		{
			messagePlayer(player, language[862]);
		}
	}
}

void item_ScrollFire(Item* item, int player)
{
	if (multiplayer == CLIENT)
	{
		return;
	}

	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	if (players[player]->entity->isBlind())
	{
		messagePlayer(player, language[775]);
		return;
	}

	if (player == clientnum)
	{
		conductIlliterate = false;
	}

	serverUpdatePlayerGameplayStats(player, STATISTICS_FIRE_MAYBE_DIFFERENT, 1);

	item->identified = 1;
	if (item->beatitude < 0)
	{
		messagePlayer(player, language[863]);
	}
	else
	{
		playSoundEntity(players[player]->entity, 153, 128); // "FireballExplode.ogg"
		messagePlayer(player, language[864]); // "The scroll erupts in a tower of flame!"

		// Attempt to set the Player on fire
		players[player]->entity->SetEntityOnFire();

		int c;
		for (c = 0; c < 100; c++)
		{
			Entity* entity = spawnFlame(players[player]->entity, SPRITE_FLAME);
			entity->sprite = 16;
			double vel = rand() % 10;
			entity->vel_x = vel * cos(entity->yaw) * cos(entity->pitch) * .1;
			entity->vel_y = vel * sin(entity->yaw) * cos(entity->pitch) * .1;
			entity->vel_z = vel * sin(entity->pitch) * .2;
			entity->skill[0] = 5 + rand() % 10;
		}
	}
}

void item_ScrollFood(Item* item, int player)
{
	Item* target;
	node_t* node, *nextnode;
	int foundfood = 0;

	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	// this is a CLIENT function
	if (player != clientnum)
	{
		return;
	}

	if (players[player]->entity->isBlind())
	{
		messagePlayer(player, language[775]);
		return;
	}

	if ( player == clientnum )
	{
		conductIlliterate = false;
	}
	item->identified = 1;
	messagePlayer(player, language[848]);
	if ( item->beatitude >= 0 )
	{
		messagePlayer(player, language[865]);
		dropItem(newItem(FOOD_FISH, EXCELLENT, item->beatitude, 1, rand(), true, &stats[player]->inventory), player);
		dropItem(newItem(FOOD_BREAD, EXCELLENT, item->beatitude, 1, rand(), true, &stats[player]->inventory), player);
		dropItem(newItem(FOOD_APPLE, EXCELLENT, item->beatitude, 1, rand(), true, &stats[player]->inventory), player);
		dropItem(newItem(FOOD_CHEESE, EXCELLENT, item->beatitude, 1, rand(), true, &stats[player]->inventory), player);
		dropItem(newItem(FOOD_MEAT, EXCELLENT, item->beatitude, 1, rand(), true, &stats[player]->inventory), player);
		return;
	}
	else
	{
		for ( node = stats[player]->inventory.first; node != NULL; node = nextnode )
		{
			nextnode = node->next;
			target = (Item*)node->element;
			if ( itemCategory(target) == FOOD )
			{
				if ( rand() % 2 == 0 )   // 50% chance of destroying that food item
				{
					consumeItem(target);
				}
				foundfood = 1;
			}
		}
	}
	if ( foundfood == 0 )
	{
		messagePlayer(player, language[866]);
	}
	else
	{
		messagePlayer(player, language[867]);
	}
}

void item_ScrollMagicMapping(Item* item, int player)
{
	int x, y;

	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	// this is a CLIENT function
	if (multiplayer == SERVER && player > 0)
	{
		return;
	}

	if (players[player]->entity->isBlind())
	{
		messagePlayer(player, language[775]);
		return;
	}

	if ( player == clientnum )
	{
		conductIlliterate = false;
	}
	item->identified = 1;
	messagePlayer(player, language[848]);
	if ( item->beatitude >= 0 )
	{
		messagePlayer(player, language[868]);
		mapLevel(player);
	}
	else
	{
		messagePlayer(player, language[869]);
		for ( y = 0; y < map.height; y++ )
		{
			for ( x = 0; x < map.width; x++ )
			{
				minimap[y][x] = 0;
			}
		}
	}
}

void item_ScrollRepair(Item* item, int player)
{
	Item* armor = nullptr;
	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	// client function only
	if ( player != clientnum )
	{
		consumeItem(item);
		return;
	}

	if (players[player]->entity->isBlind())
	{
		messagePlayer(player, language[775]);
		return;
	}

	conductIlliterate = false;
	if ( item->identified && item->beatitude >= 0 )
	{
		// quickly check if we have any items available so we don't waste our scroll
		bool foundRepairableItem = false;
		for ( node_t* node = stats[player]->inventory.first; node != nullptr; node = node->next )
		{
			Item* inventoryItem = (Item*)node->element;
			if ( GenericGUI.isItemRepairable(inventoryItem) )
			{
				foundRepairableItem = true;
				break;
			}
		}
		if ( !foundRepairableItem )
		{
			messagePlayer(player, language[3288]);
			return;
		}
	}

	item->identified = true;
	messagePlayer(player, language[848]);
	if ( item->beatitude < 0 )
	{
		int tryIndex = rand() % 7;
		int startIndex = tryIndex;
		int armornum = 0;
		bool breakloop = false;
		// degrade random equipped item.
		while ( !armor && !breakloop )
		{
			switch ( tryIndex )
			{
				// intentional fall throughs...
				case 0:
					if ( stats[player]->weapon != nullptr && itemCategory(stats[player]->weapon) != POTION )
					{
						armor = stats[player]->weapon;
						armornum = 0;
						break;
					}
				case 1:
					if ( stats[player]->helmet != nullptr )
					{
						armor = stats[player]->helmet;
						armornum = 1;
						break;
					}
				case 2:
					if ( stats[player]->breastplate != nullptr )
					{
						armor = stats[player]->breastplate;
						armornum = 2;
						break;
					}
				case 3:
					if ( stats[player]->gloves != nullptr )
					{
						armor = stats[player]->gloves;
						armornum = 3;
						break;
					}
				case 4:
					if ( stats[player]->shoes != nullptr )
					{
						armor = stats[player]->shoes;
						armornum = 4;
						break;
					}
				case 5:
					if ( stats[player]->shield != nullptr )
					{
						armor = stats[player]->shield;
						armornum = 5;
						break;
					}
				case 6:
					if ( stats[player]->cloak != nullptr )
					{
						armor = stats[player]->cloak;
						armornum = 6;
						break;
					}
					++tryIndex;
					if ( tryIndex > 6 )
					{
						// loop back around.
						tryIndex = 0;
					}
					if ( tryIndex == startIndex )
					{
						// couldn't find a piece of armor, break.
						breakloop = true;
						armor = nullptr;
						break;
					}
				default:
					break;
			}
		}
		if ( armor == nullptr )
		{
			messagePlayer(player, language[870]); // you feel a tingling sensation
		}
		else if ( armor != nullptr )
		{
			messagePlayer(player, language[871], armor->getName());
			armor->status = static_cast<Status>(std::max(armor->status - 2, static_cast<int>(BROKEN)));
			if ( multiplayer == CLIENT )
			{
				strcpy((char*)net_packet->data, "REPA");
				net_packet->data[4] = clientnum;
				net_packet->data[5] = armornum;
				net_packet->data[6] = armor->status;
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 7;
				sendPacketSafe(net_sock, -1, net_packet, 0);

				//messagePlayer(clientnum, "sent server: %d, %d, %d", net_packet->data[4], net_packet->data[5], net_packet->data[6]);
			}
			if ( armor->status > BROKEN )
			{
				if ( armor->type == TOOL_CRYSTALSHARD )
				{
					messagePlayer(player, language[2350], armor->getName());
				}
				else
				{
					messagePlayer(player, language[681], armor->getName());
				}
			}
			else
			{
				if ( armor->type == TOOL_CRYSTALSHARD )
				{
					playSoundPlayer(player, 162, 64);
					messagePlayer(player, language[2351], armor->getName());
				}
				else
				{
					playSoundPlayer(player, 76, 64);
					messagePlayer(player, language[682], armor->getName());
				}
			}
		}
	}
	else
	{
		// Repair an item
		GenericGUI.openGUI(GUI_TYPE_REPAIR, item->beatitude);
	}
	consumeItem(item);
}

void item_ScrollDestroyArmor(Item* item, int player)
{
	Item* armor = nullptr;
	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	// client function only
	if ( player != clientnum )
	{
		consumeItem(item);
		return;
	}

	if (players[player]->entity->isBlind())
	{
		messagePlayer(player, language[775]);
		return;
	}

	conductIlliterate = false;
	item->identified = 1;
	
	messagePlayer(player, language[848]);

	int armornum = 0;
	int tryIndex = 1 + rand() % 6;
	int startIndex = tryIndex;
	bool breakloop = false;
	while ( !armor && !breakloop )
	{
		switch ( tryIndex )
		{
			// intentional fall throughs...
			case 0:
				// shouldn't occur
				/*if ( stats[player]->weapon != nullptr )
				{
					armor = stats[player]->weapon;
					armornum = 0;
					break;
				}*/
			case 1:
				if ( stats[player]->helmet != nullptr )
				{
					armor = stats[player]->helmet;
					armornum = 1;
					break;
				}
			case 2:
				if ( stats[player]->breastplate != nullptr )
				{
					armor = stats[player]->breastplate;
					armornum = 2;
					break;
				}
			case 3:
				if ( stats[player]->gloves != nullptr )
				{
					armor = stats[player]->gloves;
					armornum = 3;
					break;
				}
			case 4:
				if ( stats[player]->shoes != nullptr )
				{
					armor = stats[player]->shoes;
					armornum = 4;
					break;
				}
			case 5:
				if ( stats[player]->shield != nullptr )
				{
					armor = stats[player]->shield;
					armornum = 5;
					break;
				}
			case 6:
				if ( stats[player]->cloak != nullptr )
				{
					armor = stats[player]->cloak;
					armornum = 6;
					break;
				}
				++tryIndex;
				if ( tryIndex > 6 )
				{
					// loop back around.
					tryIndex = 1;
				}
				if ( tryIndex == startIndex )
				{
					// couldn't find a piece of armor, break.
					breakloop = true;
					armor = nullptr;
					break;
				}
			default:
				break;
		}
	}

	if ( armor == nullptr )
	{
		messagePlayer(player, language[873]);
	}
	else if ( armor != nullptr )
	{
		if ( item->beatitude < 0 )
		{
			messagePlayer(player, language[874], armor->getName());
		}
		else
		{
			messagePlayer(player, language[875], armor->getName());

			armor->status = static_cast<Status>(0);

			if ( multiplayer == CLIENT )
			{
				strcpy((char*)net_packet->data, "REPA");
				net_packet->data[4] = clientnum;
				net_packet->data[5] = armornum;
				net_packet->data[6] = armor->status;
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 7;
				sendPacketSafe(net_sock, -1, net_packet, 0);

				//messagePlayer(clientnum, "sent server: %d, %d, %d", net_packet->data[4], net_packet->data[5], net_packet->data[6]);
			}

			if ( armor->status == BROKEN )
			{
				if ( armor->type == TOOL_CRYSTALSHARD )
				{
					playSoundPlayer(player, 162, 64);
					messagePlayer(player, language[2351], armor->getName());
				}
				else
				{
					playSoundPlayer(player, 76, 64);
					messagePlayer(player, language[682], armor->getName());
				}
			}
		}
	}
}

void item_ScrollTeleportation(Item* item, int player)
{
	// server only function
	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}
	if (multiplayer == CLIENT)
	{
		return;
	}

	if (players[player]->entity->isBlind())
	{
		if (player == clientnum)
		{
			messagePlayer(player, language[775]);
		}
		return;
	}

	if (player == clientnum)
	{
		conductIlliterate = false;
	}
	item->identified = 1;
	messagePlayer(player, language[848]);
	if (item->beatitude < 0 && rand() % 2)
	{
		messagePlayer(player, language[876]);
		return;
	}

	players[player]->entity->teleportRandom();
}

void item_ScrollSummon(Item* item, int player)
{
	// server only function
	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}
	if (multiplayer == CLIENT)
	{
		return;
	}

	if (players[player]->entity->isBlind())
	{
		if (player == clientnum)
		{
			messagePlayer(player, language[775]);
		}
		return;
	}

	if ( player == clientnum )
	{
		conductIlliterate = false;
	}
	item->identified = 1;
	messagePlayer(player, language[848]);

	playSoundEntity(players[player]->entity, 153, 64);
	Uint32 numCreatures = 1;
	Monster creature = RAT;

	if (item->beatitude <= -2)
	{
		// spawn something really nasty
		numCreatures = 1;
		switch (rand() % 4)
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
		}
	}
	else if (item->beatitude == -1)
	{
		// spawn moderately nasty things
		switch (rand() % 6)
		{
			case 0:
				creature = GNOME;
				numCreatures = rand() % 3 + 1;
				break;
			case 1:
				creature = SPIDER;
				numCreatures = rand() % 2 + 1;
				break;
			case 2:
				creature = SUCCUBUS;
				numCreatures = rand() % 2 + 1;
				break;
			case 3:
				creature = SCORPION;
				numCreatures = rand() % 2 + 1;
				break;
			case 4:
				creature = GHOUL;
				numCreatures = rand() % 2 + 1;
				break;
			case 5:
				creature = GOBLIN;
				numCreatures = rand() % 2 + 1;
				break;
		}
	}
	else if (item->beatitude == 0)
	{
		// spawn weak monster ally
		switch (rand() % 3)
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
	}
	else if (item->beatitude == 1)
	{
		// spawn humans
		creature = HUMAN;
		numCreatures = rand() % 3 + 1;
	}
	else if (item->beatitude >= 2)
	{
		//Spawn many/neat allies
		switch (rand() % 2)
		{
			case 0:
				// summon zap brigadiers
				numCreatures = rand() % 2 + 4;
				creature = HUMAN;
				break;
			case 1:
				// summon demons
				numCreatures = rand() % 2 + 4;
				creature = DEMON;
				break;
		}
	}

	int i;
	bool spawnedMonster = false;
	for (i = 0; i < numCreatures; ++i)
	{
		Entity* monster = summonMonster(creature, floor(players[player]->entity->x / 16) * 16 + 8, floor(players[player]->entity->y / 16) * 16 + 8);
		if ( monster )
		{
			spawnedMonster = true;
			if ( item->beatitude >= 2 && creature == HUMAN )
			{
				monster->skill[29] = 1; // zap brigadier
			}
			Stat* monsterStats = monster->getStats();
			if ( item->beatitude >= 0 && monsterStats )
			{
				monsterStats->leader_uid = players[player]->entity->getUID();
				monster->flags[USERFLAG2] = true;
				/*if ( !monsterally[HUMAN][monsterStats->type] )
				{
				}*/
				monster->monsterAllyIndex = player;
				if ( multiplayer == SERVER )
				{
					serverUpdateEntitySkill(monster, 42); // update monsterAllyIndex for clients.
				}

				// update followers for this player
				node_t* newNode = list_AddNodeLast(&stats[player]->FOLLOWERS);
				newNode->deconstructor = &defaultDeconstructor;
				Uint32* myuid = (Uint32*) malloc(sizeof(Uint32));
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
		if ( item->beatitude < 0 )
		{
			if ( numCreatures <= 1 )
			{
				if ( creature < KOBOLD ) //Original monster count
				{
					messagePlayer(player, language[877], language[90 + creature]);

				}
				else if ( creature >= KOBOLD ) //New monsters
				{
					messagePlayer(player, language[877], language[2000 + (creature - KOBOLD)]);
				}
			}
			else
			{
				if ( creature < KOBOLD ) //Original monster count
				{
					messagePlayer(player, language[878], language[111 + creature]);

				}
				else if ( creature >= KOBOLD ) //New monsters
				{
					messagePlayer(player, language[878], language[2050 + (creature - KOBOLD)]);
				}
			}
		}
		else
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
				if ( item->beatitude >= 2 )
				{
					messagePlayer(player, language[880]);
				}
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
				if ( item->beatitude >= 2 )
				{
					messagePlayer(player, language[882]);
				}
			}
		}
	}
}

void item_ToolTowel(Item*& item, int player)
{
	if ( player == clientnum )
	{
		messagePlayer(player, language[883]);
	}
	if ( multiplayer != CLIENT )
	{
		if ( stats[player]->EFFECTS[EFF_GREASY]
			|| stats[player]->EFFECTS[EFF_MESSY]
			|| stats[player]->EFFECTS[EFF_BLEEDING] )
		{
			steamAchievementClient(player, "BARONY_ACH_BRING_A_TOWEL");
		}
		stats[player]->EFFECTS[EFF_GREASY] = false;
		stats[player]->EFFECTS[EFF_MESSY] = false;
	}

	// stop bleeding
	if ( stats[player]->EFFECTS[EFF_BLEEDING] )
	{
		if ( player == clientnum )
		{
			messagePlayer(player, language[884]);
			messagePlayer(player, language[885]);
		}
		if ( multiplayer != CLIENT )
		{
			stats[player]->EFFECTS[EFF_BLEEDING] = false;
			stats[player]->EFFECTS_TIMERS[EFF_BLEEDING] = 0;
		}
		consumeItem(item);
	}

	if ( multiplayer != CLIENT )
	{
		serverUpdateEffects(player);
	}
}

void item_ToolTinOpener(Item* item, int player)
{
	if (multiplayer == CLIENT)
	{
		return;
	}

	messagePlayer(player, language[886]);
}

void item_ToolMirror(Item*& item, int player)
{
	if (players[player] == nullptr || players[player]->entity == nullptr)
	{
		return;
	}

	if (item->beatitude > 0 && !stats[player]->EFFECTS[EFF_GREASY])
	{
		if (multiplayer != CLIENT)
		{
			players[player]->entity->teleportRandom();
		}
	}
	if (player != clientnum)   //TODO: Hotseat? (player != clientnum || !players[player]->hotseat
	{
		return;
	}

	if ( stats[player]->EFFECTS[EFF_GREASY] )
	{
		messagePlayer(player, language[887]);
		messagePlayer(player, language[888]);
		consumeItem(item);
		return;
	}
	messagePlayer(player, language[889]);
	if ( players[player]->entity->isInvisible() || (stats[player] && stats[player]->type == VAMPIRE) )
	{
		messagePlayer(player, language[893]);
		return;
	}
	if ( item->beatitude > 0 )
	{
		messagePlayer(player, language[890]);
		return;
	}
	else if ( item->beatitude < 0 )
	{
		if ( stats[player]->EFFECTS[EFF_BLIND] )
		{
			messagePlayer(player, language[892]);
		}
		else
		{
			messagePlayer(player, language[891]);
		}
		consumeItem(item);
		return;
	}
	if ( stats[player]->EFFECTS[EFF_BLIND] )
	{
		messagePlayer(player, language[892]);
		return;
	}
	if ( stats[player]->EFFECTS[EFF_DRUNK] )
	{
		if ( stats[player]->sex == MALE )
		{
			messagePlayer(player, language[894]);
		}
		else
		{
			messagePlayer(player, language[895]);
		}
		return;
	}
	if ( stats[player]->EFFECTS[EFF_CONFUSED] )
	{
		messagePlayer(player, language[896]);
		return;
	}
	if ( stats[player]->EFFECTS[EFF_POISONED] )
	{
		messagePlayer(player, language[897]);
		return;
	}
	if ( stats[player]->EFFECTS[EFF_VOMITING] )
	{
		messagePlayer(player, language[898]);
		return;
	}
	if ( stats[player]->EFFECTS[EFF_MESSY] )
	{
		messagePlayer(player, language[899]);
		return;
	}
	if ( stats[player]->HUNGER < 200 )
	{
		messagePlayer(player, language[900]);
		return;
	}
	if ( stats[player]->HUNGER < 500 )
	{
		messagePlayer(player, language[901]);
		return;
	}

	if ( stats[player]->CHR < 2 )
	{
		messagePlayer(player, language[902]);
		return;
	}
	else
	{
		if ( stats[player]->sex == MALE )
		{
			messagePlayer(player, language[903]);
		}
		else
		{
			messagePlayer(player, language[904]);
		}
		return;
	}
}

void item_ToolBeartrap(Item*& item, int player)
{
	Entity* entity;

	int u, v;
	if ( players[player] && players[player]->entity )
	{
		int x = std::min(std::max<unsigned int>(1, players[player]->entity->x / 16), map.width - 2);
		int y = std::min(std::max<unsigned int>(1, players[player]->entity->y / 16), map.height - 2);
		for ( u = x - 1; u <= x + 1; u++ )
		{
			for ( v = y - 1; v <= y + 1; v++ )
			{
				if ( entityInsideTile(players[player]->entity, u, v, 0) )   // no floor
				{
					if ( multiplayer != CLIENT )
					{
						messagePlayer(player, language[3035]);
					}
					return;
				}
			}
		}
	}

	if ( multiplayer == CLIENT )
	{
		consumeItem(item);
		return;
	}
	bool failed = false;
	switch ( item->status )
	{
		case SERVICABLE:
			if ( rand() % 25 == 0 )
			{
				failed = true;
			}
			break;
		case WORN:
			if ( rand() % 10 == 0 )
			{
				failed = true;
			}
			break;
		case DECREPIT:
			if ( rand() % 4 == 0 )
			{
				failed = true;
			}
			break;
		default:
			break;
	}
	if (item->beatitude < 0 || failed)
	{
		if (player == clientnum)
		{
			messagePlayer(player, language[905]);
		}
		if (multiplayer != CLIENT)
		{
			playSoundEntity(players[player]->entity, 76, 64);
		}
		consumeItem(item);
		return;
	}
	if ( multiplayer != CLIENT )
	{
		playSoundEntity(players[player]->entity, 253, 64);
	}
	entity = newEntity(668, 1, map.entities, nullptr); //Beartrap entity.
	entity->behavior = &actBeartrap;
	entity->flags[PASSABLE] = true;
	entity->flags[UPDATENEEDED] = true;
	entity->x = players[player]->entity->x;
	entity->y = players[player]->entity->y;
	entity->z = 6.75;
	entity->yaw = players[player]->entity->yaw;
	entity->roll = -PI / 2; // flip the model
	entity->parent = players[player]->entity->getUID();
	entity->sizex = 4;
	entity->sizey = 4;
	entity->skill[11] = item->status;
	entity->skill[12] = item->beatitude;
	entity->skill[14] = item->appearance;
	entity->skill[15] = item->identified;
	messagePlayer(player, language[906]);
	consumeItem(item);
	return;
}

void item_Food(Item*& item, int player)
{
	int oldcount;
	int pukeChance;

	if ( !stats[player] )
	{
		return;
	}

	if ( player >= 0 && stats[player]->type != HUMAN && (svFlags & SV_FLAG_HUNGER) ) // hunger on
	{
		if ( stats[player]->type == SKELETON )
		{
			if ( player == clientnum )
			{
				dropItem(item, player); // client drop item
				messagePlayer(clientnum, language[3179]);
			}
			return;
		}
	}

	if ( stats[player]->amulet != NULL )
	{
		if ( stats[player]->amulet->type == AMULET_STRANGULATION )
		{
			if ( player == clientnum )
			{
				messagePlayer(player, language[756]);
			}
			return;
		}
	}

	// can't eat while vomiting
	if ( stats[player]->EFFECTS[EFF_VOMITING] )
	{
		if ( player == clientnum )
		{
			messagePlayer(player, language[757]);
		}
		return;
	}

	if ( player == clientnum )
	{
		conductFoodless = false;
		if ( item->type == FOOD_MEAT || item->type == FOOD_FISH || item->type == FOOD_TOMALLEY || item->type == FOOD_BLOOD )
		{
			conductVegetarian = false;
		}
	}

	if ( multiplayer == CLIENT )
	{
		consumeItem(item);
		return;
	}

	// consumption message
	oldcount = item->count;
	item->count = 1;
	messagePlayer(player, language[907], item->description());
	item->count = oldcount;

	// eating sound
	if ( item->type == FOOD_BLOOD )
	{
		// play drink sound
		playSoundEntity(players[player]->entity, 52, 64);
	}
	else
	{
		playSoundEntity(players[player]->entity, 50 + rand() % 2, 64);
	}

	// chance of rottenness
	switch ( item->status )
	{
		case EXCELLENT:
			pukeChance = 100;
			break;
		case SERVICABLE:
			pukeChance = 25;
			break;
		case WORN:
			pukeChance = 10;
			break;
		case DECREPIT:
			pukeChance = 4;
			break;
		default:
			pukeChance = 100;
			break;
	}

	if ( players[player]->entity->playerRequiresBloodToSustain() )
	{
		if ( item->type == FOOD_BLOOD )
		{
			pukeChance = 100;
		}
		else if ( item->type != FOOD_BLOOD )
		{
			if ( stats[player]->type == VAMPIRE )
			{
				pukeChance = 1;
			}
		}
	}
	else if ( item->type == FOOD_BLOOD )
	{
		pukeChance = 1;
	}

	if (((item->beatitude < 0 && item->type != FOOD_CREAMPIE) || (rand() % pukeChance == 0)) && pukeChance < 100)
	{
		if (players[player] && players[player]->entity && !(svFlags & SV_FLAG_HUNGER))
		{
			players[player]->entity->modHP(-5);
		}
		if ( stats[player]->type == VAMPIRE )
		{
			messagePlayer(player, language[3201]);
		}
		else if ( item->type == FOOD_BLOOD )
		{
			messagePlayer(player, language[3203]);
		}
		else
		{
			messagePlayer(player, language[908]);
		}
		players[player]->entity->skill[26] = 40 + rand() % 10;
		consumeItem(item);
		return;
	}
	if ( item->beatitude < 0 && item->type == FOOD_CREAMPIE )
	{
		messagePlayer(player, language[909]);
		messagePlayer(player, language[910]);
		stats[player]->EFFECTS[EFF_MESSY] = true;
		stats[player]->EFFECTS_TIMERS[EFF_MESSY] = 600; // ten seconds
		serverUpdateEffects(player);
		consumeItem(item);
		return;
	}

	// replenish nutrition points
	if ( svFlags & SV_FLAG_HUNGER )
	{
		switch ( item->type )
		{
			case FOOD_BREAD:
				stats[player]->HUNGER += 400;
				break;
			case FOOD_CREAMPIE:
				stats[player]->HUNGER += 200;
				break;
			case FOOD_CHEESE:
				stats[player]->HUNGER += 100;
				break;
			case FOOD_APPLE:
				stats[player]->HUNGER += 200;
				break;
			case FOOD_MEAT:
				stats[player]->HUNGER += 600;
				break;
			case FOOD_FISH:
				stats[player]->HUNGER += 500;
				break;
			case FOOD_TOMALLEY:
				stats[player]->HUNGER += 400;
				break;
			case FOOD_BLOOD:
				if ( players[player]->entity->playerRequiresBloodToSustain() )
				{
					stats[player]->HUNGER += 250;
				}
				else
				{
					stats[player]->HUNGER += 10;
				}
				break;
			default:
				stats[player]->HUNGER += 10;
				break;
		}
	}
	else
	{
		if (players[player] && players[player]->entity)
		{
			players[player]->entity->modHP(5);
		}
		messagePlayer(player, language[911]);
	}

	if ( item->type == FOOD_TOMALLEY )
	{
		serverUpdatePlayerGameplayStats(player, STATISTICS_TEMPT_FATE, 1);
	}

	// results of eating
	if ( stats[player]->HUNGER <= 250 )
	{
		messagePlayer(player, language[912]);
	}
	else if ( stats[player]->HUNGER < 500 )
	{
		messagePlayer(player, language[913]);
	}
	else if ( stats[player]->HUNGER < 1000 )
	{
		messagePlayer(player, language[914], item->getName());
	}
	else if ( stats[player]->HUNGER < 1500 )
	{
		messagePlayer(player, language[915]);
	}
	else if ( stats[player]->HUNGER < 2000 )
	{
		if (rand() % 3)
		{
			messagePlayer(player, language[916]);
		}
		else
		{
			messagePlayer(player, language[917]);
			players[player]->entity->skill[26] = 40 + rand() % 10;
		}
	}
	else
	{
		messagePlayer(player, language[917]);
		players[player]->entity->skill[26] = 40 + rand() % 10;
	}

	stats[player]->HUNGER = std::min(stats[player]->HUNGER, 2000);
	serverUpdateHunger(player);
	consumeItem(item);
}

void item_FoodTin(Item*& item, int player)
{
	int oldcount;
	int pukeChance;
	bool slippery = false;

	if ( stats[player]->amulet != NULL )
	{
		if ( stats[player]->amulet->type == AMULET_STRANGULATION )
		{
			if ( player == clientnum )
			{
				messagePlayer(player, language[756]);
			}
			return;
		}
	}

	// can't eat while vomiting
	if ( stats[player]->EFFECTS[EFF_VOMITING] )
	{
		if ( player == clientnum )
		{
			messagePlayer(player, language[757]);
		}
		return;
	}

	if ( player == clientnum )
	{
		conductFoodless = false;
		conductVegetarian = false;
	}

	if ( multiplayer == CLIENT )
	{
		consumeItem(item);
		return;
	}

	// consumption message
	char tempstr[128] = { 0 };
	oldcount = item->count;
	item->count = 1;

	bool hpBuff = false;
	bool mpBuff = false;

	// first word
	int word = rand() % 16;
	strcpy(tempstr, language[918 + word]);
	if ( word == 6 || word == 15 )
	{
		slippery = true;
	}
	if ( word < 8 )
	{
		hpBuff = true;
	}

	// second word
	word = rand() % 16;
	strcat(tempstr, language[934 + word]);
	if ( word == 1 || word == 7 || word == 8 || word == 12 )
	{
		slippery = true;
	}
	if ( word >= 8 )
	{
		mpBuff = true;
	}

	// third word
	word = rand() % 16;
	strcat(tempstr, language[950 + word]);
	if ( word == 1 || word == 8 )
	{
		slippery = true;
	}
	if ( word == 8 )
	{
		hpBuff = true;
		mpBuff = true;
	}

	if ( stats[player]->type == GOATMAN )
	{
		messagePlayer(player, language[3220], tempstr);
	}
	else
	{
		messagePlayer(player, language[764], tempstr);
	}
	item->count = oldcount;

	// eating sound
	playSoundEntity(players[player]->entity, 50 + rand() % 2, 64);

	serverUpdatePlayerGameplayStats(player, STATISTICS_YES_WE_CAN, 1);

	// chance of rottenness
	switch ( item->status )
	{
		case EXCELLENT:
			pukeChance = 100;
			break;
		case SERVICABLE:
			pukeChance = 25;
			break;
		case WORN:
			pukeChance = 10;
			break;
		case DECREPIT:
			pukeChance = 3;
			break;
		default:
			pukeChance = 100;
			break;
	}

	if ( stats[player]->type == VAMPIRE )
	{
		pukeChance = 1;
	}

	if ((item->beatitude < 0 || rand() % pukeChance == 0) && pukeChance < 100)
	{
		if (players[player] && players[player]->entity && !svFlags & SV_FLAG_HUNGER)
		{
			players[player]->entity->modHP(-5);
		}
		if ( stats[player]->type == VAMPIRE )
		{
			messagePlayer(player, language[3201]);
		}
		else
		{
			messagePlayer(player, language[908]);
		}
		players[player]->entity->skill[26] = 40 + rand() % 10;
		consumeItem(item);
		return;
	}

	int buffDuration = item->status * TICKS_PER_SECOND * 4; // (4 - 16 seconds)
	if ( item->status > WORN )
	{
		buffDuration -= rand() % ((buffDuration / 2) + 1); // 50-100% duration
	}
	else
	{
		buffDuration -= rand() % ((buffDuration / 4) + 1); // 75-100% duration
	}

	// replenish nutrition points
	if (svFlags & SV_FLAG_HUNGER)
	{
		stats[player]->HUNGER += 600;
		stats[player]->EFFECTS[EFF_HP_REGEN] = hpBuff;
		stats[player]->EFFECTS[EFF_MP_REGEN] = mpBuff;
		stats[player]->EFFECTS_TIMERS[EFF_HP_REGEN] = buffDuration;
		stats[player]->EFFECTS_TIMERS[EFF_MP_REGEN] = buffDuration;
	}
	else
	{
		if (players[player] && players[player]->entity)
		{
			players[player]->entity->modHP(5);
		}
		messagePlayer(player, language[911]);
	}

	// greasy fingers
	if ( slippery )
	{
		messagePlayer(player, language[966]);
		stats[player]->EFFECTS[EFF_GREASY] = true;
		stats[player]->EFFECTS_TIMERS[EFF_GREASY] = TICKS_PER_SECOND * (60 + rand() % 60); // 1-2 minutes of greasy
		serverUpdateEffects(player);
	}

	// results of eating
	if ( stats[player]->HUNGER <= 250 )
	{
		messagePlayer(player, language[912]);
	}
	else if ( stats[player]->HUNGER < 500 )
	{
		messagePlayer(player, language[913]);
	}
	else if ( stats[player]->HUNGER < 1000 )
	{
		messagePlayer(player, language[914], item->getName());
	}
	else if ( stats[player]->HUNGER < 1500 )
	{
		messagePlayer(player, language[915]);
	}
	else if ( stats[player]->HUNGER < 2000 )
	{
		if (rand() % 3)
		{
			messagePlayer(player, language[916]);
		}
		else
		{
			messagePlayer(player, language[917]);
			players[player]->entity->skill[26] = 40 + rand() % 10;
		}
	}
	else
	{
		messagePlayer(player, language[917]);
		players[player]->entity->skill[26] = 40 + rand() % 10;
	}

	stats[player]->HUNGER = std::min(stats[player]->HUNGER, 2000);

	if ( (hpBuff || mpBuff) && (svFlags & SV_FLAG_HUNGER) )
	{
		messagePlayer(player, language[911]);
	}

	serverUpdateHunger(player);
	consumeItem(item);
}

void item_AmuletSexChange(Item* item, int player)
{
	if ( stats[player]->amulet != NULL )
	{
		if ( !stats[player]->amulet->canUnequip(stats[player]) )
		{
			if ( player == clientnum )
			{
				if ( shouldInvertEquipmentBeatitude(stats[player]) && item->beatitude > 0 )
				{
					messagePlayer(player, language[3218]);
				}
				else
				{
					messagePlayer(player, language[1087]);
				}
			}
			return;
		}
	}
	stats[player]->amulet = NULL;
	stats[player]->sex = static_cast<sex_t>((stats[player]->sex == 0));

	if ( player != clientnum )
	{
		return;
	}
	if ( stats[player]->sex == MALE )
	{
		messagePlayer(player, language[967]);
	}
	else
	{
		messagePlayer(player, language[968]);
	}
	messagePlayer(player, language[969]);
}

void item_Spellbook(Item*& item, int player)
{
	node_t* node, *nextnode;

	item->identified = true;
	if (player != clientnum)
	{
		return;
	}

	if (players[player]->entity->isBlind())
	{
		messagePlayer(player, language[970]);
		return;
	}

	conductIlliterate = false;

	if ( item->beatitude < 0 && !shouldInvertEquipmentBeatitude(stats[player]) )
	{
		messagePlayer(clientnum, language[971]);
		if ( list_Size(&spellList) > 0 )
		{
			messagePlayer(clientnum, language[972]);

			// randomly delete a spell
			int spellToDelete = rand() % list_Size(&spellList);
			node = list_Node(&spellList, spellToDelete);
			spell_t* spell = (spell_t*)node->element;
			if ( spell == selected_spell )
			{
				selected_spell = NULL;
			}
			int spellID = spell->ID;
			list_RemoveNode(node);

			// delete its accompanying spell item(s)
			for ( node = stats[player]->inventory.first; node != NULL; node = nextnode )
			{
				nextnode = node->next;
				Item* item = (Item*)node->element;
				if ( item->type == SPELL_ITEM )
				{
					if ( item->appearance == spellID )
					{
						list_RemoveNode(node);
					}
				}
			}
		}
		else
		{
			messagePlayer(clientnum, language[973]);
		}
		consumeItem(item);
		return;
	}
	else
	{
		bool learned = false;
		switch ( item->type )
		{
			case SPELLBOOK_FORCEBOLT:
				learned = addSpell(SPELL_FORCEBOLT, player);
				break;
			case SPELLBOOK_MAGICMISSILE:
				learned = addSpell(SPELL_MAGICMISSILE, player);
				break;
			case SPELLBOOK_COLD:
				learned = addSpell(SPELL_COLD, player);
				break;
			case SPELLBOOK_FIREBALL:
				learned = addSpell(SPELL_FIREBALL, player);
				break;
			case SPELLBOOK_LIGHTNING:
				learned = addSpell(SPELL_LIGHTNING, player);
				break;
			case SPELLBOOK_REMOVECURSE:
				learned = addSpell(SPELL_REMOVECURSE, player);
				break;
			case SPELLBOOK_LIGHT:
				learned = addSpell(SPELL_LIGHT, player);
				break;
			case SPELLBOOK_IDENTIFY:
				learned = addSpell(SPELL_IDENTIFY, player);
				break;
			case SPELLBOOK_MAGICMAPPING:
				learned = addSpell(SPELL_MAGICMAPPING, player);
				break;
			case SPELLBOOK_SLEEP:
				learned = addSpell(SPELL_SLEEP, player);
				break;
			case SPELLBOOK_CONFUSE:
				learned = addSpell(SPELL_CONFUSE, player);
				break;
			case SPELLBOOK_SLOW:
				learned = addSpell(SPELL_SLOW, player);
				break;
			case SPELLBOOK_OPENING:
				learned = addSpell(SPELL_OPENING, player);
				break;
			case SPELLBOOK_LOCKING:
				learned = addSpell(SPELL_LOCKING, player);
				break;
			case SPELLBOOK_LEVITATION:
				learned = addSpell(SPELL_LEVITATION, player);
				break;
			case SPELLBOOK_INVISIBILITY:
				learned = addSpell(SPELL_INVISIBILITY, player);
				break;
			case SPELLBOOK_TELEPORTATION:
				learned = addSpell(SPELL_TELEPORTATION, player);
				break;
			case SPELLBOOK_HEALING:
				learned = addSpell(SPELL_HEALING, player);
				break;
			case SPELLBOOK_EXTRAHEALING:
				learned = addSpell(SPELL_EXTRAHEALING, player);
				break;
			case SPELLBOOK_CUREAILMENT:
				learned = addSpell(SPELL_CUREAILMENT, player);
				break;
			case SPELLBOOK_DIG:
				learned = addSpell(SPELL_DIG, player);
				break;
			case SPELLBOOK_SUMMON:
				learned = addSpell(SPELL_SUMMON, player);
				break;
			case SPELLBOOK_STONEBLOOD:
				learned = addSpell(SPELL_STONEBLOOD, player);
				break;
			case SPELLBOOK_BLEED:
				learned = addSpell(SPELL_BLEED, player);
				break;
			case SPELLBOOK_REFLECT_MAGIC:
				learned = addSpell(SPELL_REFLECT_MAGIC, player);
				break;
			case SPELLBOOK_ACID_SPRAY:
				learned = addSpell(SPELL_ACID_SPRAY, player);
				break;
			case SPELLBOOK_STEAL_WEAPON:
				learned = addSpell(SPELL_STEAL_WEAPON, player);
				break;
			case SPELLBOOK_DRAIN_SOUL:
				learned = addSpell(SPELL_DRAIN_SOUL, player);
				break;
			case SPELLBOOK_VAMPIRIC_AURA:
				learned = addSpell(SPELL_VAMPIRIC_AURA, player);
				break;
			case SPELLBOOK_CHARM_MONSTER:
				learned = addSpell(SPELL_CHARM_MONSTER, player);
				break;
			default:
				learned = addSpell(SPELL_FORCEBOLT, player);
				break;
		}

		if ( learned )
		{
			item->status = static_cast<Status>(item->status - 1);
			if ( item->status != BROKEN )
			{
				messagePlayer(player, language[2595]);
			}
			else
			{
				messagePlayer(player, language[2596]);
				consumeItem(item);
			}

			if ( list_Size(&spellList) >= 20 )
			{
				steamAchievement("BARONY_ACH_MAGIC_MASTERY");
			}
		}
	}
}
