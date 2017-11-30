/*-------------------------------------------------------------------------------

	BARONY
	File: actboulder.cpp
	Desc: implements boulder and boulder trap code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "sound.hpp"
#include "items.hpp"
#include "net.hpp"
#include "monster.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "magic/magic.hpp"

/*-------------------------------------------------------------------------------

act*

The following function describes an entity behavior. The function
takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/
void actMagicTrapCeiling(Entity* my)
{
	if ( !my )
	{
		return;
	}

	my->actMagicTrapCeiling();
}


void Entity::actMagicTrapCeiling()
{
	spellTrapAmbience--;
	if ( spellTrapAmbience <= 0 )
	{
		spellTrapAmbience = TICKS_PER_SECOND * 120;
		playSoundEntity(this, 149, 64);
	}

	if ( multiplayer == CLIENT )
	{
		return;
	}
	if ( circuit_status != CIRCUIT_ON )
	{
		spellTrapReset = 0;
		spellTrapCounter = spellTrapRefireRate; //shoost instantly!
		return;
	}

	if ( !spellTrapInit )
	{
		spellTrapInit = 1;
		if ( spellTrapType == -1 )
		{
			switch ( rand() % 8 )
			{
				case 0:
					spellTrapType = SPELL_FORCEBOLT;
					break;
				case 1:
					spellTrapType = SPELL_MAGICMISSILE;
					break;
				case 2:
					spellTrapType = SPELL_COLD;
					break;
				case 3:
					spellTrapType = SPELL_FIREBALL;
					break;
				case 4:
					spellTrapType = SPELL_LIGHTNING;
					break;
				case 5:
					spellTrapType = SPELL_SLEEP;
					spellTrapRefireRate = 225; // stop getting stuck forever!
					break;
				case 6:
					spellTrapType = SPELL_CONFUSE;
					break;
				case 7:
					spellTrapType = SPELL_SLOW;
					break;
				default:
					spellTrapType = SPELL_MAGICMISSILE;
					break;
			}
		}
		//light = lightSphere(my->x / 16, my->y / 16, 3, 192);
	}

	++spellTrapCounter;

	node_t* node = children.first;
	Entity* ceilingModel = (Entity*)(node->element);
	int triggerSprite = 0;
	switch ( spellTrapType )
	{
		case SPELL_FORCEBOLT:
		case SPELL_MAGICMISSILE:
		case SPELL_CONFUSE:
			triggerSprite = 173;
			break;
		case SPELL_FIREBALL:
			triggerSprite = 168;
			break;
		case SPELL_LIGHTNING:
			triggerSprite = 170;
			break;
		case SPELL_COLD:
		case SPELL_SLEEP:
			triggerSprite = 172;
			break;
		case SPELL_SLOW:
		default:
			triggerSprite = 171;
			break;
	}

	if ( spellTrapCounter > spellTrapRefireRate )
	{
		spellTrapCounter = 0; // reset timer.
		if ( spellTrapReset == 0 )
		{
			// once off magic particles. reset once power is cut.
			spawnMagicEffectParticles(x, y, z, triggerSprite);
			playSoundEntity(this, 252, 128);
			spellTrapReset = 1;
			/*spellTrapCounter = spellTrapRefireRate - 5; // delay?
			return;*/
		}
		Entity* entity = castSpell(getUID(), getSpellFromID(spellTrapType), false, true);
		if ( ceilingModel && entity )
		{
			entity->x = x;
			entity->y = y;
			entity->z = ceilingModel->z - 5;
			double missile_speed = 4 * ((double)(((spellElement_t*)(getSpellFromID(spellTrapType)->elements.first->element))->mana) / ((spellElement_t*)(getSpellFromID(spellTrapType)->elements.first->element))->overload_multiplier);
			entity->vel_x = 0.0;
			entity->vel_y = 0.0;
			entity->vel_z = 0.5 * (missile_speed);
			entity->pitch = PI / 2;
		}
	}
}

/*-------------------------------------------------------------------------------

act*

The following function describes an entity behavior. The function
takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define MAGICTRAP_INIT my->skill[0]
#define MAGICTRAP_SPELL my->skill[1]
#define MAGICTRAP_DIRECTION my->skill[3]

void actMagicTrap(Entity* my)
{
	if ( !MAGICTRAP_INIT )
	{
		MAGICTRAP_INIT = 1;
		switch ( rand() % 8 )
		{
			case 0:
				MAGICTRAP_SPELL = SPELL_FORCEBOLT;
				break;
			case 1:
				MAGICTRAP_SPELL = SPELL_MAGICMISSILE;
				break;
			case 2:
				MAGICTRAP_SPELL = SPELL_COLD;
				break;
			case 3:
				MAGICTRAP_SPELL = SPELL_FIREBALL;
				break;
			case 4:
				MAGICTRAP_SPELL = SPELL_LIGHTNING;
				break;
			case 5:
				MAGICTRAP_SPELL = SPELL_SLEEP;
				break;
			case 6:
				MAGICTRAP_SPELL = SPELL_CONFUSE;
				break;
			case 7:
				MAGICTRAP_SPELL = SPELL_SLOW;
				break;
			default:
				MAGICTRAP_SPELL = SPELL_MAGICMISSILE;
				break;
		}
		my->light = lightSphere(my->x / 16, my->y / 16, 3, 192);
	}

	// eliminate traps that have been destroyed.
	if ( !checkObstacle(my->x, my->y, my, NULL) )
	{
		my->removeLightField();
		list_RemoveNode(my->mynode);
		return;
	}

	if ( multiplayer == CLIENT )
	{
		return;
	}

	if ( my->ticks % TICKS_PER_SECOND == 0 )
	{
		int oldir = 0;
		int x = 0, y = 0;
		switch ( MAGICTRAP_DIRECTION )
		{
			case 0:
				x = 12;
				y = 0;
				oldir = MAGICTRAP_DIRECTION;
				MAGICTRAP_DIRECTION++;
				break;
			case 1:
				x = 0;
				y = 12;
				oldir = MAGICTRAP_DIRECTION;
				MAGICTRAP_DIRECTION++;
				break;
			case 2:
				x = -12;
				y = 0;
				oldir = MAGICTRAP_DIRECTION;
				MAGICTRAP_DIRECTION++;
				break;
			case 3:
				x = 0;
				y = -12;
				oldir = MAGICTRAP_DIRECTION;
				MAGICTRAP_DIRECTION = 0;
				break;
		}
		int u = std::min<int>(std::max<int>(0.0, (my->x + x) / 16), map.width - 1);
		int v = std::min<int>(std::max<int>(0.0, (my->y + y) / 16), map.height - 1);
		if ( !map.tiles[OBSTACLELAYER + v * MAPLAYERS + u * MAPLAYERS * map.height] )
		{
			Entity* entity = castSpell(my->getUID(), getSpellFromID(MAGICTRAP_SPELL), false, true);
			entity->x = my->x + x;
			entity->y = my->y + y;
			entity->z = my->z;
			entity->yaw = oldir * (PI / 2.f);
			double missile_speed = 4 * ((double)(((spellElement_t*)(getSpellFromID(MAGICTRAP_SPELL)->elements.first->element))->mana) / ((spellElement_t*)(getSpellFromID(MAGICTRAP_SPELL)->elements.first->element))->overload_multiplier);
			entity->vel_x = cos(entity->yaw) * (missile_speed);
			entity->vel_y = sin(entity->yaw) * (missile_speed);
		}
	}
}