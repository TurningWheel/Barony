/*-------------------------------------------------------------------------------

	BARONY
	File: actmagictrap.cpp
	Desc: implements magic trap code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "engine/audio/sound.hpp"
#include "items.hpp"
#include "net.hpp"
#include "monster.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "magic/magic.hpp"
#include "prng.hpp"
#include "mod_tools.hpp"

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
#ifdef USE_FMOD
	if ( spellTrapAmbience == 0 )
	{
		spellTrapAmbience--;
		stopEntitySound();
		entity_sound = playSoundEntityLocal(this, 149, 16);
	}
	if ( entity_sound )
	{
		bool playing = false;
		entity_sound->isPlaying(&playing);
		if ( !playing )
		{
			entity_sound = nullptr;
		}
	}
#else
	spellTrapAmbience--;
	if ( spellTrapAmbience <= 0 )
	{
		spellTrapAmbience = TICKS_PER_SECOND * 30;
		playSoundEntityLocal(this, 149, 16);
	}
#endif

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
		auto& rng = entity_rng ? *entity_rng : local_rng;
		if ( spellTrapType == -1 )
		{
			switch ( rng.rand() % 8 )
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
					spellTrapRefireRate = 275; // stop getting stuck forever!
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
			entity->z = ceilingModel->z - 2;
			double missile_speed = 4 * ((double)(((spellElement_t*)(getSpellFromID(spellTrapType)->elements.first->element))->mana) / ((spellElement_t*)(getSpellFromID(spellTrapType)->elements.first->element))->overload_multiplier);
			entity->vel_x = 0.0;
			entity->vel_y = 0.0;
			entity->vel_z = 0.5 * (missile_speed);
			entity->pitch = PI / 2;
			entity->actmagicIsVertical = MAGIC_ISVERTICAL_Z;
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
		auto& rng = my->entity_rng ? *my->entity_rng : local_rng;
		switch ( rng.rand() % 8 )
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
		my->light = addLight(my->x / 16, my->y / 16, "magictrap");
	}

	// eliminate traps that have been destroyed.
	// check wall inside me.
	int checkx = static_cast<int>(my->x) >> 4;
	int checky = static_cast<int>(my->y) >> 4;
	if ( !map.tiles[OBSTACLELAYER + checky * MAPLAYERS + checkx * MAPLAYERS * map.height] )   // wall
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

void actTeleportShrine(Entity* my)
{
	if ( !my )
	{
		return;
	}

	my->actTeleportShrine();
}


void Entity::actTeleportShrine()
{
	if ( this->ticks == 1 )
	{
		this->createWorldUITooltip();
	}

	this->removeLightField();
	if ( shrineActivateDelay == 0 )
	{
		this->light = addLight(this->x / 16, this->y / 16, "teleport_shrine");
		spawnAmbientParticles(80, 576, 10 + local_rng.rand() % 40, 1.0, false);
	}

#ifdef USE_FMOD
	if ( shrineAmbience == 0 )
	{
		shrineAmbience--;
		stopEntitySound();
		entity_sound = playSoundEntityLocal(this, 149, 16);
	}
	if ( entity_sound )
	{
		bool playing = false;
		entity_sound->isPlaying(&playing);
		if ( !playing )
		{
			entity_sound = nullptr;
		}
	}
#else
	shrineAmbience--;
	if ( shrineAmbience <= 0 )
	{
		shrineAmbience = TICKS_PER_SECOND * 30;
		playSoundEntityLocal(this, 149, 16);
	}
#endif

	if ( multiplayer == CLIENT )
	{
		return;
	}


	if ( !shrineInit )
	{
		shrineInit = 1;
		shrineSpellEffect = SPELL_TELEPORTATION;
	}

	if ( shrineActivateDelay > 0 )
	{
		--shrineActivateDelay;
		if ( shrineActivateDelay == 0 )
		{
			serverUpdateEntitySkill(this, 7);
		}
	}

	// using
	if ( this->isInteractWithMonster() )
	{
		Entity* monsterInteracting = uidToEntity(this->interactedByMonster);
		if ( monsterInteracting )
		{
			if ( shrineActivateDelay == 0 )
			{
				std::vector<std::pair<Entity*, std::pair<int, int>>> allShrines;
				for ( node_t* node = map.entities->first; node; node = node->next )
				{
					Entity* entity = (Entity*)node->element;
					if ( !entity ) { continue; }
					if ( entity->behavior == &::actTeleportShrine )
					{
						allShrines.push_back(std::make_pair(entity, std::make_pair((int)(entity->x / 16), (int)(entity->y / 16))));
					}
				}

				Entity* selectedShrine = nullptr;
				for ( size_t s = 0; s < allShrines.size(); ++s )
				{
					if ( allShrines[s].first == this )
					{
						// find next one in list
						if ( (s + 1) >= allShrines.size() )
						{
							selectedShrine = allShrines[0].first;
						}
						else
						{
							selectedShrine = allShrines[s + 1].first;
						}
						break;
					}
				}

				if ( selectedShrine )
				{
					playSoundEntity(this, 252, 128);
					//messagePlayer(i, MESSAGE_INTERACTION, Language::get(4301));

					if ( auto leader = monsterInteracting->monsterAllyGetPlayerLeader() )
					{
						Compendium_t::Events_t::eventUpdateWorld(leader->skill[2], Compendium_t::CPDM_OBELISK_FOLLOWER_USES, "obelisk", 1);
					}

					Entity* spellTimer = createParticleTimer(this, 200, 625);
					spellTimer->particleTimerPreDelay = 0; // wait x ticks before animation.
					spellTimer->particleTimerEndAction = PARTICLE_EFFECT_SHRINE_TELEPORT; // teleport behavior of timer.
					spellTimer->particleTimerEndSprite = 625; // sprite to use for end of timer function.
					spellTimer->particleTimerCountdownAction = 1;
					spellTimer->particleTimerCountdownSprite = 625;
					spellTimer->particleTimerTarget = static_cast<Sint32>(selectedShrine->getUID()); // get the target to teleport around.
					spellTimer->particleTimerVariable1 = 1; // distance of teleport in tiles
					spellTimer->particleTimerVariable2 = monsterInteracting->getUID(); // which player to teleport
					if ( multiplayer == SERVER )
					{
						serverSpawnMiscParticles(this, PARTICLE_EFFECT_SHRINE_TELEPORT, 625);
					}
					shrineActivateDelay = 250;
					serverUpdateEntitySkill(this, 7);
				}
			}
			this->clearMonsterInteract();
			return;
		}
		this->clearMonsterInteract();
	}
	for ( int i = 0; i < MAXPLAYERS; i++ )
	{
		if ( selectedEntity[i] == this || client_selected[i] == this )
		{
			if ( inrange[i] && Player::getPlayerInteractEntity(i) )
			{
				if ( shrineActivateDelay > 0 )
				{
					messagePlayer(i, MESSAGE_INTERACTION, Language::get(4300));
					break;
				}

				std::vector<std::pair<Entity*, std::pair<int, int>>> allShrines;
				for ( node_t* node = map.entities->first; node; node = node->next )
				{
					Entity* entity = (Entity*)node->element;
					if ( !entity ) { continue; }
					if ( entity->behavior == &::actTeleportShrine )
					{
						allShrines.push_back(std::make_pair(entity, std::make_pair((int)(entity->x / 16), (int)(entity->y / 16))));
					}
				}

				Entity* selectedShrine = nullptr;
				for ( size_t s = 0; s < allShrines.size(); ++s )
				{
					if ( allShrines[s].first == this )
					{
						// find next one in list
						if ( (s + 1) >= allShrines.size() )
						{
							selectedShrine = allShrines[0].first;
						}
						else
						{
							selectedShrine = allShrines[s + 1].first;
						}
						break;
					}
				}

				if ( selectedShrine )
				{
					playSoundEntity(this, 252, 128);
					messagePlayer(i, MESSAGE_INTERACTION, Language::get(4301));

					Compendium_t::Events_t::eventUpdateWorld(i, Compendium_t::CPDM_OBELISK_USES, "obelisk", 1);

					Entity* spellTimer = createParticleTimer(this, 200, 625);
					spellTimer->particleTimerPreDelay = 0; // wait x ticks before animation.
					spellTimer->particleTimerEndAction = PARTICLE_EFFECT_SHRINE_TELEPORT; // teleport behavior of timer.
					spellTimer->particleTimerEndSprite = 625; // sprite to use for end of timer function.
					spellTimer->particleTimerCountdownAction = 1;
					spellTimer->particleTimerCountdownSprite = 625;
					spellTimer->particleTimerTarget = static_cast<Sint32>(selectedShrine->getUID()); // get the target to teleport around.
					spellTimer->particleTimerVariable1 = 1; // distance of teleport in tiles
					spellTimer->particleTimerVariable2 = Player::getPlayerInteractEntity(i)->getUID(); // which player to teleport
					if ( multiplayer == SERVER )
					{
						serverSpawnMiscParticles(this, PARTICLE_EFFECT_SHRINE_TELEPORT, 625);
					}
					shrineActivateDelay = 250;
					serverUpdateEntitySkill(this, 7);
				}

				break;
			}
		}
	}
}

void actDaedalusShrine(Entity* my)
{
	if ( !my )
	{
		return;
	}

	my->actDaedalusShrine();
}

#define SHRINE_DAEDALUS_EXIT_UID my->skill[13]
#define SHRINE_TURN_DIR my->skill[14]
#define SHRINE_LAST_TOUCHED my->skill[15]
#define SHRINE_START_DIR my->fskill[0]
#define SHRINE_TARGET_DIR my->fskill[1]

void daedalusShrineInteract(Entity* my, Entity* touched)
{
	if ( !my ) { return; }

	if ( multiplayer == SERVER )
	{
		for ( int i = 1; i < MAXPLAYERS; ++i )
		{
			if ( !client_disconnected[i] )
			{
				strcpy((char*)net_packet->data, "DAED");
				SDLNet_Write32(static_cast<Uint32>(my->getUID()), &net_packet->data[4]);
				net_packet->address.host = net_clients[i - 1].host;
				net_packet->address.port = net_clients[i - 1].port;
				net_packet->len = 8;
				sendPacketSafe(net_sock, -1, net_packet, i - 1);
			}
		}
	}

	if ( my->shrineDaedalusState == 0 ) // default
	{
		if ( multiplayer != CLIENT )
		{
			SHRINE_LAST_TOUCHED = touched ? touched->getUID() : 0;
		}

		Entity* exitEntity = nullptr;
		for ( node_t* node = map.entities->first; node; node = node->next )
		{
			Entity* entity = (Entity*)node->element;
			if ( !entity ) { continue; }
			if ( (entity->behavior == &actLadder && strcmp(map.name, "Hell")) || (entity->behavior == &actPortal && !strcmp(map.name, "Hell")) )
			{
				if ( entity->behavior == &actLadder && entity->skill[3] != 1 )
				{
					exitEntity = entity;
					break;
				}
				if ( entity->behavior == &actPortal && entity->portalNotSecret == 1 )
				{
					exitEntity = entity;
					break;
				}
			}
		}

		if ( exitEntity )
		{
			real_t tangent = atan2(exitEntity->y - my->y, exitEntity->x - my->x);
			while ( tangent >= 2 * PI )
			{
				tangent -= 2 * PI;
			}
			while ( tangent < 0 )
			{
				tangent += 2 * PI;
			}
			while ( my->yaw >= 2 * PI )
			{
				my->yaw -= 2 * PI;
			}
			while ( my->yaw < 0 )
			{
				my->yaw += 2 * PI;
			}
			SHRINE_TARGET_DIR = tangent;
			SHRINE_DAEDALUS_EXIT_UID = exitEntity->getUID();
			SHRINE_START_DIR = my->yaw;
			playSoundEntityLocal(my, 248, 128);
			my->shrineDaedalusState = 1;
		}
	}
	else if ( my->shrineDaedalusState == 2 )
	{
		shrineDaedalusRevealMap(*my);

		if ( multiplayer != CLIENT )
		{
			SHRINE_LAST_TOUCHED = touched ? touched->getUID() : 0;
			spawnMagicEffectParticles(my->x, my->y, my->z, 174);

			if ( my->ticks >= (getMinotaurTimeToArrive() - TICKS_PER_SECOND * 30) && my->ticks <= getMinotaurTimeToArrive() )
			{
				if ( touched && touched->getStats() )
				{
					Stat* myStats = touched->getStats();
					if ( myStats->EFFECTS[EFF_SLOW] )
					{
						touched->setEffect(EFF_SLOW, false, 0, true);
					}
					if ( touched->setEffect(EFF_FAST, true, std::max(TICKS_PER_SECOND * 15, myStats->EFFECTS_TIMERS[EFF_FAST]), true) )
					{
						playSoundEntity(touched, 178, 128);
						spawnMagicEffectParticles(touched->x, touched->y, touched->z, 174);
						if ( touched->behavior == &actPlayer )
						{
							messagePlayerColor(touched->skill[2], MESSAGE_STATUS, makeColorRGB(0, 255, 0), Language::get(768));
							steamAchievementClient(touched->skill[2], "BARONY_ACH_BULL_RUSH");
							Compendium_t::Events_t::eventUpdateWorld(touched->skill[2], Compendium_t::CPDM_DAED_SPEED_BUFFS, "daedalus", 1);
						}
					}
				}
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					messagePlayerColor(i, MESSAGE_INTERACTION, makeColorRGB(255, 0, 255), Language::get(6285));
				}
			}
			else
			{
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					messagePlayerColor(i, MESSAGE_INTERACTION, makeColorRGB(255, 0, 255), Language::get(6267));
				}
			}
		}
	}
}

void Entity::actDaedalusShrine()
{
	if ( this->ticks == 1 )
	{
		this->createWorldUITooltip();
	}

	this->removeLightField();
	if ( shrineActivateDelay == 0 )
	{
		this->light = addLight(this->x / 16, this->y / 16, "teleport_shrine");
		spawnAmbientParticles(80, 576, 10 + local_rng.rand() % 40, 1.0, false);
	}

#ifdef USE_FMOD
	if ( shrineAmbience == 0 )
	{
		shrineAmbience--;
		stopEntitySound();
		entity_sound = playSoundEntityLocal(this, 149, 16);
	}
	if ( entity_sound )
	{
		bool playing = false;
		entity_sound->isPlaying(&playing);
		if ( !playing )
		{
			entity_sound = nullptr;
		}
	}
#else
	shrineAmbience--;
	if ( shrineAmbience <= 0 )
	{
		shrineAmbience = TICKS_PER_SECOND * 30;
		playSoundEntityLocal(this, 149, 16);
	}
#endif

	if ( !shrineInit )
	{
		shrineInit = 1;
		shrineSpellEffect = SPELL_SPEED;
	}


	if ( shrineActivateDelay > 0 )
	{
		--shrineActivateDelay;
		if ( shrineActivateDelay == 0 )
		{
			if ( multiplayer != CLIENT )
			{
				serverUpdateEntitySkill(this, 7);
			}
		}
	}

	if ( shrineDaedalusState == 1 )
	{
		// point to exit
		int dir = 0;
		real_t startDir = fskill[0];
		real_t targetDir = fskill[1];

		int diff = static_cast<int>((startDir - targetDir) * 180.0 / PI) % 360;
		if ( diff < 0 )
		{
			diff += 360;
		}
		if ( diff < 180 )
		{
			dir = 1;
		}

		real_t speed = dir == 1 ? -2.f : 2.f;
		real_t scale = 1.0;
		{
			if ( diff > 180 )
			{
				diff -= 360;
			}
			scale = std::max(0.05, (abs(diff) / 180.0)) / (real_t)TICKS_PER_SECOND;
			speed *= scale;
		}

		if ( limbAnimateToLimit(this, ANIMATE_YAW, speed, targetDir, false, 0.0) )
		{
			shrineDaedalusState = 2;
			shrineDaedalusRevealMap(*this);

			if ( multiplayer != CLIENT )
			{
				spawnMagicEffectParticles(x, y, z, 174);
				Entity* touched = nullptr;
				if ( skill[15] != 0 )
				{
					touched = uidToEntity(skill[15]);
				}

				if ( this->ticks >= (getMinotaurTimeToArrive() - TICKS_PER_SECOND * 30) && this->ticks <= getMinotaurTimeToArrive() )
				{
					if ( touched && touched->getStats() )
					{
						Stat* myStats = touched->getStats();
						if ( myStats->EFFECTS[EFF_SLOW] )
						{
							touched->setEffect(EFF_SLOW, false, 0, true);
						}
						if ( touched->setEffect(EFF_FAST, true, std::max(TICKS_PER_SECOND * 15, myStats->EFFECTS_TIMERS[EFF_FAST]), true) )
						{
							playSoundEntity(touched, 178, 128);
							spawnMagicEffectParticles(touched->x, touched->y, touched->z, 174);
							if ( touched->behavior == &actPlayer )
							{
								messagePlayerColor(touched->skill[2], MESSAGE_STATUS, makeColorRGB(0, 255, 0), Language::get(768));
								steamAchievementClient(touched->skill[2], "BARONY_ACH_BULL_RUSH");
								Compendium_t::Events_t::eventUpdateWorld(touched->skill[2], Compendium_t::CPDM_DAED_SPEED_BUFFS, "daedalus", 1);
							}
						}
					}
					for ( int i = 0; i < MAXPLAYERS; ++i )
					{
						messagePlayerColor(i, MESSAGE_INTERACTION, makeColorRGB(255, 0, 255), Language::get(6285));
					}
				}
				else
				{
					for ( int i = 0; i < MAXPLAYERS; ++i )
					{
						messagePlayerColor(i, MESSAGE_INTERACTION, makeColorRGB(255, 0, 255), Language::get(6267));
					}
				}
			}
		}
	}

	if ( multiplayer == CLIENT )
	{
		return;
	}

	// using
	//if ( this->isInteractWithMonster() )
	//{
	//	Entity* monsterInteracting = uidToEntity(this->interactedByMonster);
	//	if ( monsterInteracting )
	//	{
	//		if ( shrineActivateDelay == 0 )
	//		{
	//			std::vector<std::pair<Entity*, std::pair<int, int>>> allShrines;
	//			for ( node_t* node = map.entities->first; node; node = node->next )
	//			{
	//				Entity* entity = (Entity*)node->element;
	//				if ( !entity ) { continue; }
	//				if ( entity->behavior == &::actTeleportShrine )
	//				{
	//					allShrines.push_back(std::make_pair(entity, std::make_pair((int)(entity->x / 16), (int)(entity->y / 16))));
	//				}
	//			}

	//			Entity* selectedShrine = nullptr;
	//			for ( size_t s = 0; s < allShrines.size(); ++s )
	//			{
	//				if ( allShrines[s].first == this )
	//				{
	//					// find next one in list
	//					if ( (s + 1) >= allShrines.size() )
	//					{
	//						selectedShrine = allShrines[0].first;
	//					}
	//					else
	//					{
	//						selectedShrine = allShrines[s + 1].first;
	//					}
	//					break;
	//				}
	//			}

	//			if ( selectedShrine )
	//			{
	//				playSoundEntity(this, 252, 128);
	//				//messagePlayer(i, MESSAGE_INTERACTION, Language::get(4301));

	//				if ( auto leader = monsterInteracting->monsterAllyGetPlayerLeader() )
	//				{
	//					Compendium_t::Events_t::eventUpdateWorld(leader->skill[2], Compendium_t::CPDM_OBELISK_FOLLOWER_USES, "obelisk", 1);
	//				}

	//				Entity* spellTimer = createParticleTimer(this, 200, 625);
	//				spellTimer->particleTimerPreDelay = 0; // wait x ticks before animation.
	//				spellTimer->particleTimerEndAction = PARTICLE_EFFECT_SHRINE_TELEPORT; // teleport behavior of timer.
	//				spellTimer->particleTimerEndSprite = 625; // sprite to use for end of timer function.
	//				spellTimer->particleTimerCountdownAction = 1;
	//				spellTimer->particleTimerCountdownSprite = 625;
	//				spellTimer->particleTimerTarget = static_cast<Sint32>(selectedShrine->getUID()); // get the target to teleport around.
	//				spellTimer->particleTimerVariable1 = 1; // distance of teleport in tiles
	//				spellTimer->particleTimerVariable2 = monsterInteracting->getUID(); // which player to teleport
	//				if ( multiplayer == SERVER )
	//				{
	//					serverSpawnMiscParticles(this, PARTICLE_EFFECT_SHRINE_TELEPORT, 625);
	//				}
	//				shrineActivateDelay = 250;
	//				serverUpdateEntitySkill(this, 7);
	//			}
	//		}
	//		this->clearMonsterInteract();
	//		return;
	//	}
	//	this->clearMonsterInteract();
	//}
	for ( int i = 0; i < MAXPLAYERS; i++ )
	{
		if ( selectedEntity[i] == this || client_selected[i] == this )
		{
			if ( inrange[i] && Player::getPlayerInteractEntity(i) )
			{
				if ( shrineActivateDelay > 0 )
				{
					messagePlayer(i, MESSAGE_INTERACTION, Language::get(4300));
					break;
				}

				daedalusShrineInteract(this, Player::getPlayerInteractEntity(i));
				Compendium_t::Events_t::eventUpdateWorld(i, Compendium_t::CPDM_DAED_USES, "daedalus", 1);
				shrineActivateDelay = TICKS_PER_SECOND * 5;
				serverUpdateEntitySkill(this, 7);
				break;
			}
		}
	}
}

void actAssistShrine(Entity* my)
{
	if ( !my )
	{
		return;
	}

	my->actAssistShrine();
}

void Entity::actAssistShrine()
{
	if ( this->ticks == 1 )
	{
		this->createWorldUITooltip();
	}

	this->removeLightField();
#ifdef USE_FMOD
	if ( shrineAmbience == 0 )
	{
		shrineAmbience--;
		stopEntitySound();
		entity_sound = playSoundEntityLocal(this, 149, 16);
	}
	if ( entity_sound )
	{
		bool playing = false;
		entity_sound->isPlaying(&playing);
		if ( !playing )
		{
			entity_sound = nullptr;
		}
	}
#else
	shrineAmbience--;
	if ( shrineAmbience <= 0 )
	{
		shrineAmbience = TICKS_PER_SECOND * 30;
		playSoundEntityLocal(this, 149, 16);
	}
#endif

	if ( !shrineInit )
	{
		shrineInit = 1;
	}

	Sint32& shrineInteracting = this->skill[0];

	Uint32 numFlames = 0;
	Uint32 redFlames = 0;
	Entity* interacting = uidToEntity(shrineInteracting);
	if ( interacting && interacting->behavior == &actPlayer )
	{
		redFlames |= (1 << interacting->skill[2]);
		numFlames |= (1 << interacting->skill[2]);
	}
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( !client_disconnected[i] )
		{
			if ( stats[i]->MISC_FLAGS[STAT_FLAG_ASSISTANCE_PLAYER_PTS] > 0 )
			{
				numFlames |= (1 << i);
			}
		}
	}

	static ConsoleVariable<float> cvar_assist_flame_x1("/assist_flame_x1", 0.5);
	static ConsoleVariable<float> cvar_assist_flame_y1("/assist_flame_y1", -3.f);
	static ConsoleVariable<float> cvar_assist_flame_x2("/assist_flame_x2", -1.f);
	static ConsoleVariable<float> cvar_assist_flame_y2("/assist_flame_y2", -1.f);
	static ConsoleVariable<float> cvar_assist_flame_x3("/assist_flame_x3", -1.f);
	static ConsoleVariable<float> cvar_assist_flame_y3("/assist_flame_y3", -3.5);
	static ConsoleVariable<float> cvar_assist_flame_x4("/assist_flame_x4", 0.5);
	static ConsoleVariable<float> cvar_assist_flame_y4("/assist_flame_y4", -1.f);
	static ConsoleVariable<float> cvar_assist_flame_z1("/assist_flame_z1", 6.5);
	static ConsoleVariable<float> cvar_assist_flame_z2("/assist_flame_z2", 9.f);
	static ConsoleVariable<float> cvar_assist_flame_z3("/assist_flame_z3", 6.5);
	static ConsoleVariable<float> cvar_assist_flame_z4("/assist_flame_z4", 10.f);
	const int spriteCandle = 202;
	const int spriteCandleBlue = 203;
	if ( numFlames > 0 && ( flickerLights || this->ticks % TICKS_PER_SECOND == 1 ) )
	{
		Entity* entity = nullptr;
		if ( numFlames & (1 << 3) )
		{
			if ( entity = spawnFlame(this, redFlames & (1 << 3) ? spriteCandle : spriteCandleBlue) )
			{
				entity->x += *cvar_assist_flame_x4 * cos(this->yaw);
				entity->y += *cvar_assist_flame_x4 * sin(this->yaw);
				entity->x += *cvar_assist_flame_y4 * cos(this->yaw + PI / 2);
				entity->y += *cvar_assist_flame_y4 * sin(this->yaw + PI / 2);
				entity->z -= *cvar_assist_flame_z4;
				entity->flags[GENIUS] = false;
				entity->setUID(-3);
			}
		}
		if ( numFlames & (1 << 2) )
		{
			if ( entity = spawnFlame(this, redFlames & (1 << 2) ? spriteCandle : spriteCandleBlue) )
			{
				entity->x += *cvar_assist_flame_x3 * cos(this->yaw);
				entity->y += *cvar_assist_flame_x3 * sin(this->yaw);
				entity->x += *cvar_assist_flame_y3 * cos(this->yaw + PI / 2);
				entity->y += *cvar_assist_flame_y3 * sin(this->yaw + PI / 2);
				entity->z -= *cvar_assist_flame_z3;
				entity->flags[GENIUS] = false;
				entity->setUID(-3);
			}
		}
		if ( numFlames & (1 << 1) )
		{
			if ( entity = spawnFlame(this, redFlames & (1 << 1) ? spriteCandle : spriteCandleBlue) )
			{
				entity->x += *cvar_assist_flame_x2 * cos(this->yaw);
				entity->y += *cvar_assist_flame_x2 * sin(this->yaw);
				entity->x += *cvar_assist_flame_y2 * cos(this->yaw + PI / 2);
				entity->y += *cvar_assist_flame_y2 * sin(this->yaw + PI / 2);
				entity->z -= *cvar_assist_flame_z2;
				entity->flags[GENIUS] = false;
				entity->setUID(-3);
			}
		}
		if ( numFlames & (1 << 0) )
		{
			if ( entity = spawnFlame(this, redFlames & (1 << 0) ? spriteCandle : spriteCandleBlue) )
			{
				entity->x += *cvar_assist_flame_x1 * cos(this->yaw);
				entity->y += *cvar_assist_flame_x1 * sin(this->yaw);
				entity->x += *cvar_assist_flame_y1 * cos(this->yaw + PI / 2);
				entity->y += *cvar_assist_flame_y1 * sin(this->yaw + PI / 2);
				entity->z -= *cvar_assist_flame_z1;
				entity->flags[GENIUS] = false;
				entity->setUID(-3);
			}
		}

		if ( redFlames )
		{
			this->light = addLight(this->x / 16, this->y / 16, "assist_shrine_red");
		}
		else
		{
			std::string lightname = "assist_shrine_flame";
			int flames = 0;
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( numFlames & (1 << i) )
				{
					++flames;
				}
			}
			lightname += std::to_string(std::min(MAXPLAYERS, flames));
			this->light = addLight(this->x / 16, this->y / 16, lightname.c_str());
		}
	}
	else
	{
		this->light = addLight(this->x / 16, this->y / 16, "assist_shrine");
	}

	if ( multiplayer == CLIENT )
	{
		return;
	}

	if ( shrineInteracting > 0 )
	{
		if ( !interacting || (entityDist(interacting, this) > TOUCHRANGE) )
		{
			int playernum = -1;
			if ( interacting->behavior == &actPlayer )
			{
				playernum = interacting->skill[2];
			}
			shrineInteracting = 0;
			serverUpdateEntitySkill(this, 0);
			if ( multiplayer == SERVER && playernum > 0 )
			{
				strcpy((char*)net_packet->data, "ASCL");
				net_packet->data[4] = playernum;
				SDLNet_Write32(this->getUID(), &net_packet->data[5]);
				net_packet->address.host = net_clients[playernum - 1].host;
				net_packet->address.port = net_clients[playernum - 1].port;
				net_packet->len = 9;
				sendPacketSafe(net_sock, -1, net_packet, playernum - 1);
			}
			else if ( multiplayer == SINGLE || playernum == 0 )
			{
				GenericGUI[playernum].assistShrineGUI.closeAssistShrine();
			}
		}
	}

	// using
	for ( int i = 0; i < MAXPLAYERS; i++ )
	{
		if ( selectedEntity[i] == this || client_selected[i] == this )
		{
			if ( inrange[i] && players[i]->entity )
			{
				if ( shrineInteracting != 0 )
				{
					if ( Entity* interacting = uidToEntity(shrineInteracting) )
					{
						if ( interacting != players[i]->entity )
						{
							messagePlayer(i, MESSAGE_INTERACTION, Language::get(6351));
						}
					}
				}
				else
				{
					shrineInteracting = players[i]->entity->getUID();
					if ( multiplayer == SERVER )
					{
						serverUpdateEntitySkill(this, 0);
					}
					if ( players[i]->isLocalPlayer() )
					{
						GenericGUI[i].openGUI(GUI_TYPE_ASSIST, this);
					}
					else if ( multiplayer == SERVER && i > 0 )
					{
						strcpy((char*)net_packet->data, "ASSO");
						SDLNet_Write32(this->getUID(), &net_packet->data[4]);
						net_packet->address.host = net_clients[i - 1].host;
						net_packet->address.port = net_clients[i - 1].port;
						net_packet->len = 8;
						sendPacketSafe(net_sock, -1, net_packet, i - 1);
					}
				}
				break;
			}
		}
	}
}