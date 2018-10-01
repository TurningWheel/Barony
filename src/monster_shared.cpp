/*-------------------------------------------------------------------------------

	BARONY
	File: monster_shared.cpp
	Desc: contains shared monster implementation and helper functions

	Copyright 2013-2017 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "entity.hpp"

void Entity::initMonster(int mySprite)
{
	sprite = mySprite;

	//Common flags.
	flags[UPDATENEEDED] = true;
	flags[BLOCKSIGHT] = true;
	flags[INVISIBLE] = false;

	int monsterType = this->getMonsterTypeFromSprite();

	switch ( monsterType )
	{
		case GNOME:
			monsterFootstepType = MONSTER_FOOTSTEP_USE_BOOTS;
			monsterSpellAnimation = MONSTER_SPELLCAST_SMALL_HUMANOID;
			break;
		case KOBOLD:
			monsterFootstepType = MONSTER_FOOTSTEP_USE_BOOTS;
			monsterSpellAnimation = MONSTER_SPELLCAST_SMALL_HUMANOID;
			break;
		case HUMAN:
			monsterFootstepType = MONSTER_FOOTSTEP_USE_BOOTS;
			monsterSpellAnimation = MONSTER_SPELLCAST_HUMANOID;
			break;
		case RAT:
			monsterFootstepType = MONSTER_FOOTSTEP_NONE;
			monsterSpellAnimation = MONSTER_SPELLCAST_NONE;
			break;
		case GOBLIN:
			monsterFootstepType = MONSTER_FOOTSTEP_USE_BOOTS;
			monsterSpellAnimation = MONSTER_SPELLCAST_HUMANOID;
			break;
		case SLIME:
			monsterFootstepType = MONSTER_FOOTSTEP_NONE;
			monsterSpellAnimation = MONSTER_SPELLCAST_NONE;
			break;
		case TROLL:
			monsterFootstepType = MONSTER_FOOTSTEP_STOMP;
			monsterSpellAnimation = MONSTER_SPELLCAST_NONE;
			break;
		case OCTOPUS:
			// unused
			break;
		case SPIDER:
			monsterFootstepType = MONSTER_FOOTSTEP_NONE;
			monsterSpellAnimation = MONSTER_SPELLCAST_NONE;
			break;
		case GHOUL:
			monsterFootstepType = MONSTER_FOOTSTEP_NONE;
			monsterSpellAnimation = MONSTER_SPELLCAST_NONE;
			break;
		case SKELETON:
			monsterFootstepType = MONSTER_FOOTSTEP_SKELETON;
			monsterSpellAnimation = MONSTER_SPELLCAST_HUMANOID;
			break;
		case SCORPION:
			monsterFootstepType = MONSTER_FOOTSTEP_NONE;
			monsterSpellAnimation = MONSTER_SPELLCAST_NONE;
			break;
		case CREATURE_IMP:
			monsterFootstepType = MONSTER_FOOTSTEP_NONE;
			monsterSpellAnimation = MONSTER_SPELLCAST_NONE;
			break;
		case BUGBEAR:
			// unused
			break;
		case DEMON:
			monsterFootstepType = MONSTER_FOOTSTEP_NONE;
			monsterSpellAnimation = MONSTER_SPELLCAST_NONE;
			break;
		case SUCCUBUS:
			monsterFootstepType = MONSTER_FOOTSTEP_NONE;
			monsterSpellAnimation = MONSTER_SPELLCAST_HUMANOID;
			break;
		case MIMIC:
			// unused
			break;
		case LICH:
			monsterFootstepType = MONSTER_FOOTSTEP_NONE;
			monsterSpellAnimation = MONSTER_SPELLCAST_NONE;
			break;
		case MINOTAUR:
			monsterFootstepType = MONSTER_FOOTSTEP_STOMP;
			monsterSpellAnimation = MONSTER_SPELLCAST_NONE;
			break;
		case DEVIL:
			monsterFootstepType = MONSTER_FOOTSTEP_NONE;
			monsterSpellAnimation = MONSTER_SPELLCAST_NONE;
			break;
		case SHOPKEEPER:
			monsterFootstepType = MONSTER_FOOTSTEP_LEATHER;
			monsterSpellAnimation = MONSTER_SPELLCAST_HUMANOID;
			break;
		case SCARAB:
			monsterFootstepType = MONSTER_FOOTSTEP_NONE;
			monsterSpellAnimation = MONSTER_SPELLCAST_NONE;
			break;
		case CRYSTALGOLEM:
			monsterFootstepType = MONSTER_FOOTSTEP_STOMP;
			monsterSpellAnimation = MONSTER_SPELLCAST_NONE;
			break;
		case INCUBUS:
			monsterFootstepType = MONSTER_FOOTSTEP_LEATHER;
			monsterSpellAnimation = MONSTER_SPELLCAST_HUMANOID;
			break;
		case VAMPIRE:
			monsterFootstepType = MONSTER_FOOTSTEP_LEATHER;
			monsterSpellAnimation = MONSTER_SPELLCAST_HUMANOID;
			break;
		case SHADOW:
			monsterFootstepType = MONSTER_FOOTSTEP_USE_BOOTS;
			monsterSpellAnimation = MONSTER_SPELLCAST_HUMANOID;
			break;
		case COCKATRICE:
			monsterFootstepType = MONSTER_FOOTSTEP_NONE;
			monsterSpellAnimation = MONSTER_SPELLCAST_NONE;
			break;
		case INSECTOID:
			monsterFootstepType = MONSTER_FOOTSTEP_USE_BOOTS;
			monsterSpellAnimation = MONSTER_SPELLCAST_HUMANOID;
			break;
		case GOATMAN:
			monsterFootstepType = MONSTER_FOOTSTEP_USE_BOOTS;
			monsterSpellAnimation = MONSTER_SPELLCAST_HUMANOID;
			break;
		case AUTOMATON:
			monsterFootstepType = MONSTER_FOOTSTEP_USE_BOOTS;
			monsterSpellAnimation = MONSTER_SPELLCAST_HUMANOID;
			break;
		case LICH_ICE:
			monsterFootstepType = MONSTER_FOOTSTEP_NONE;
			monsterSpellAnimation = MONSTER_SPELLCAST_HUMANOID;
			break;
		case LICH_FIRE:
			monsterFootstepType = MONSTER_FOOTSTEP_NONE;
			monsterSpellAnimation = MONSTER_SPELLCAST_HUMANOID;
			break;
		default:
			monsterFootstepType = MONSTER_FOOTSTEP_NONE;
			monsterSpellAnimation = MONSTER_SPELLCAST_NONE;
			break;
	}
	return;
}

int Entity::getMonsterTypeFromSprite()
{
	int mySprite = this->sprite;

	if ( (mySprite >= 113 && mySprite < 118) ||
		(mySprite >= 125 && mySprite < 130) ||
		(mySprite >= 332 && mySprite < 334) ||
		(mySprite >= 341 && mySprite < 347) ||
		(mySprite >= 354 && mySprite < 360) ||
		(mySprite >= 367 && mySprite < 373) ||
		(mySprite >= 380 && mySprite < 386) )   // human heads
	{
		return HUMAN;
	}
	else if ( mySprite == 131 || mySprite == 265 )     // rat
	{
		return RAT;
	}
	else if ( mySprite == 180 || mySprite == 694 )     // goblin head
	{
		return GOBLIN;
	}
	else if ( mySprite == 196 || mySprite == 266 )     // scorpion body
	{
		return SCORPION;
	}
	else if ( mySprite == 190 || mySprite == 710 )     // succubus head
	{
		return SUCCUBUS;
	}
	else if ( mySprite == 204 )     // troll head
	{
		return TROLL;
	}
	else if ( mySprite == 217 )     // shopkeeper head
	{
		return SHOPKEEPER;
	}
	else if ( mySprite == 229 || mySprite == 686 )     // skeleton head
	{
		return SKELETON;
	}
	else if ( mySprite == 239 )     // minotaur waist
	{
		return MINOTAUR;
	}
	else if ( mySprite == 246 )     // ghoul head
	{
		return GHOUL;
	}
	else if ( mySprite == 258 )     // demon head
	{
		return DEMON;
	}
	else if ( mySprite == 267 )     // spider body
	{
		return SPIDER;
	}
	else if ( mySprite == 274 )     // lich body
	{
		return LICH;
	}
	else if ( mySprite == 289 )     // imp head
	{
		return CREATURE_IMP;
	}
	else if ( mySprite == 295 )     // gnome head
	{
		return GNOME;
	}
	else if ( mySprite == 304 )     // devil torso
	{
		return DEVIL;
	}
	else if ( mySprite == 475 )     // crystal golem head
	{
		return CRYSTALGOLEM;
	}
	else if ( mySprite == 413 )     // cockatrice head
	{
		return COCKATRICE;
	}
	else if ( mySprite == 467 || mySprite == 742 )     // automaton head
	{
		return AUTOMATON;
	}
	else if ( mySprite == 429 || mySprite == 430 )     // scarab
	{
		return SCARAB;
	}
	else if ( mySprite == 421 )     // kobold head
	{
		return KOBOLD;
	}
	else if ( mySprite == 481 )     // shadow head
	{
		return SHADOW;
	}
	else if ( mySprite == 437 || mySprite == 718 )     // vampire head
	{
		return VAMPIRE;
	}
	else if ( mySprite == 445 || mySprite == 702 )     // incubus head
	{
		return INCUBUS;
	}
	else if ( mySprite == 455 || mySprite == 726 )     // insectoid head
	{
		return INSECTOID;
	}
	else if ( mySprite == 463 || mySprite == 734 )     // goatman head
	{
		return GOATMAN;
	}
	else if ( mySprite == 646 )     // lich body
	{
		return LICH_FIRE;
	}
	else if ( mySprite == 650 )     // lich body
	{
		return LICH_ICE;
	}
	else if ( mySprite == 189 || mySprite == 210 )
	{
		return SLIME;
	}
	return NOTHING;
}

void Entity::actMonsterLimb(bool processLight)
{
	//If no longer part of a monster, delete the limb.
	Entity *parentEnt = nullptr;
	if ( (parentEnt = uidToEntity(skill[2])) == nullptr )
	{
		list_RemoveNode(mynode);
		return;
	}

	//Do something magical beyond my comprehension.
	if ( multiplayer != CLIENT )
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( inrange[i] )
			{
				if ( i == 0 && selectedEntity == this )
				{
					parentEnt->skill[13] = i + 1;
				}
				else if ( client_selected[i] == this )
				{
					parentEnt->skill[13] = i + 1;
				}
			}
		}
	}

	if ( processLight )
	{
		//Only run by monsters who can carry stuff (like torches). Sorry, rats.
		if ( light != nullptr )
		{
			list_RemoveNode(light->node);
			light = nullptr;
		}

		int carryingLightSource = 0;
		if ( flags[INVISIBLE] == false )
		{
			if ( sprite == 93 )   // torch
			{
				carryingLightSource = 6;
			}
			else if ( sprite == 94 )     // lantern
			{
				carryingLightSource = 9;
			}
			else if ( sprite == 529 )	// crystal shard
			{
				carryingLightSource = 4;
			}
		}

		if ( carryingLightSource != 0 )
		{
			light = lightSphereShadow(x / 16, y / 16, carryingLightSource, 50 + 15 * carryingLightSource);
		}
	}

	if ( parentEnt && parentEnt->behavior == &actMonster && parentEnt->monsterEntityRenderAsTelepath == 1 )
	{
		monsterEntityRenderAsTelepath = 1;
	}
	else
	{
		monsterEntityRenderAsTelepath = 0;
	}
}

void Entity::removeMonsterDeathNodes()
{
	int i = 0;
	node_t *nextnode = nullptr;
	for ( node_t *node = children.first; node != nullptr; node = nextnode )
	{
		nextnode = node->next;
		if ( node->element != nullptr && i >= 2 )
		{
			Entity* entity = (Entity*)node->element;
			if ( entity->light != nullptr )
			{
				list_RemoveNode(entity->light->node);
			}
			entity->light = nullptr;
			entity->flags[UPDATENEEDED] = false; //TODO: Do only demon & baphy need this?
			list_RemoveNode(entity->mynode);
		}
		list_RemoveNode(node);
		++i;
	}
}

void Entity::spawnBlood(int bloodSprite)
{
	if ( spawn_blood )
	{
		int tileX = std::min<unsigned int>(std::max<int>(0, this->x / 16), map.width - 1);
		int tileY = std::min<unsigned int>(std::max<int>(0, this->y / 16), map.height - 1);
		if ( map.tiles[tileY * MAPLAYERS + tileX * MAPLAYERS * map.height] )
		{
			if ( !checkObstacle(this->x, this->y, this, nullptr) )
			{
				Entity* entity = newEntity(bloodSprite, 1, map.entities, nullptr); //Blood/gib entity.
				entity->x = this->x;
				entity->y = this->y;
				entity->z = 8 + (rand() % 20) / 100.0;
				entity->parent = getUID();
				entity->sizex = 2;
				entity->sizey = 2;
				entity->yaw = (rand() % 360) * PI / 180.0;
				entity->flags[UPDATENEEDED] = true;
				entity->flags[PASSABLE] = true;
			}
		}
	}
}
