/*-------------------------------------------------------------------------------

	BARONY
	File: actthrown.cpp
	Desc: behavior function for a thrown item

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "items.hpp"
#include "engine/audio/sound.hpp"
#include "monster.hpp"
#include "interface/interface.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "scores.hpp"
#include "player.hpp"
#include "magic/magic.hpp"
#include "paths.hpp"
#include "prng.hpp"
#include "mod_tools.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define THROWN_VELX my->vel_x
#define THROWN_VELY my->vel_y
#define THROWN_VELZ my->vel_z
#define THROWN_TYPE (ItemType)my->skill[10]
#define THROWN_STATUS (Status)my->skill[11]
#define THROWN_BEATITUDE my->skill[12]
#define THROWN_COUNT my->skill[13]
#define THROWN_APPEARANCE my->skill[14]
#define THROWN_IDENTIFIED my->skill[15]
#define THROWN_LIFE my->skill[16]
#define THROWN_BOUNCES my->skill[17]
#define THROWN_LINGER my->skill[18]
#define THROWN_BOOMERANG_STOP_Z my->skill[21]

#define BOOMERANG_PARTICLE 977
#define BOLAS_PARTICLE 1916

void onThrownLandingParticle(Entity* my)
{
	if ( my )
	{
		int itemType = THROWN_TYPE;
		if ( itemType >= WOODEN_SHIELD && itemType < NUMITEMS )
		{
			if ( items[itemType].category == POTION )
			{
				int gibsprite = -1;
				if ( my->sprite >= 50 && my->sprite <= 58 )
				{
					gibsprite = 1895 + my->sprite - 50;
				}
				if ( itemType == POTION_EMPTY )
				{
					gibsprite = 1911;
				}
				if ( gibsprite >= 0 )
				{
					for ( int i = 0; i < 5; ++i )
					{
						if ( Entity* gib = spawnGib(hit.entity ? hit.entity : my, gibsprite) )
						{
							gib->sprite = gibsprite;
							if ( !hit.entity )
							{
								gib->z = my->z;
							}
							serverSpawnGibForClient(gib);
						}
					}
				}
			}
			else
			{
				switch ( itemType )
				{
				case GREASE_BALL:
					for ( int i = 0; i < 5; ++i )
					{
						if ( Entity* gib = spawnGib(hit.entity ? hit.entity : my, 245) )
						{
							gib->sprite = 245;
							gib->flags[SPRITE] = true;
							if ( !hit.entity )
							{
								gib->z = my->z;
							}
							serverSpawnGibForClient(gib);
						}
					}
					break;
				case DUST_BALL:
					for ( int i = 0; i < 5; ++i )
					{
						if ( Entity* gib = spawnGib(hit.entity ? hit.entity : my, 1886) )
						{
							gib->sprite = 1886;
							if ( !hit.entity )
							{
								gib->z = my->z;
							}
							serverSpawnGibForClient(gib);
						}
					}
					break;
				case SLOP_BALL:
					for ( int i = 0; i < 5; ++i )
					{
						if ( Entity* gib = spawnGib(hit.entity ? hit.entity : my, 1926) )
						{
							gib->sprite = 1926;
							if ( !hit.entity )
							{
								gib->z = my->z;
							}
							serverSpawnGibForClient(gib);
						}
					}
					break;
				default:
					break;
				}
			}
		}
	}
}

void actThrown(Entity* my)
{
	Item* item = nullptr;
	Category cat = GEM;
	ItemType type = WOODEN_SHIELD;
	char* itemname = nullptr;

	item = newItemFromEntity(my, true);
	if ( item )
	{
		cat = itemCategory(item);
		type = item->type;
		free(item);
		item = nullptr;
	}

	my->removeLightField();

	if ( multiplayer == CLIENT )
	{
		if ( THROWN_LIFE == 0 )
		{
			Entity* tempEntity = players[clientnum]->entity;
			if ( tempEntity )
			{
				if ( entityInsideEntity(my, tempEntity) )
				{
					my->parent = tempEntity->getUID();
				}
				else
				{
					node_t* node;
					for ( node = map.creatures->first; node != nullptr; node = node->next ) //Since searching for players and monsters, don't search full map.entities.
					{
						Entity* entity = (Entity*)node->element;
						if ( entity->behavior == &actPlayer || entity->behavior == &actMonster )
						{
							if ( entityInsideEntity(my, entity) )
							{
								my->parent = entity->getUID();
								break;
							}
						}
					}
				}
			}
			else
			{
				node_t* node;
				for ( node = map.creatures->first; node != nullptr; node = node->next ) //Monsters and players? Creature list, not entity list.
				{
					Entity* entity = (Entity*)node->element;
					if ( entity->behavior == &actPlayer || entity->behavior == &actMonster )
					{
						if ( entityInsideEntity(my, entity) )
						{
							my->parent = entity->getUID();
							break;
						}
					}
				}
			}

			if ( my->parent && cat == THROWN && uidToEntity(my->parent) && uidToEntity(my->parent)->behavior != &actPlayer )
			{
				my->createWorldUITooltip();
			}
		}
		if ( my->sprite == BOOMERANG_PARTICLE )
		{
			my->focalx = 2;
			my->focaly = 0;
			my->focalz = 0.5;
			if ( my->ticks > 0 && my->ticks % 7 == 0 )
			{
				playSoundEntityLocal(my, 434 + local_rng.rand() % 10, 64);
			}
		}

		if ( my->sprite == items[GREASE_BALL].index )
		{
			if ( Entity* fx = spawnMagicParticleCustom(my, 245, 1.0, 1.0) )
			{
				fx->ditheringDisabled = true;
				real_t dir = atan2(my->vel_y, my->vel_x);
				//dir += local_rng.rand() % 2 == 0 ? PI / 32 : -PI / 32;
				real_t spd = sqrt(my->vel_x * my->vel_x + my->vel_y * my->vel_y);
				fx->vel_x = spd * 0.05 * cos(dir);
				fx->vel_y = spd * 0.05 * sin(dir);
				fx->flags[BRIGHT] = true;
				fx->pitch = PI / 2;
				fx->roll = 0.0;
				fx->yaw = dir;
				fx->vel_z = 0.0;
				fx->flags[SPRITE] = true;
			}
		}
		else if ( my->sprite == items[DUST_BALL].index )
		{
			if ( Entity* fx = spawnMagicParticleCustom(my, 1886, 1.0, 1.0) )
			{
				fx->ditheringDisabled = true;
				real_t dir = atan2(my->vel_y, my->vel_x);
				//dir += local_rng.rand() % 2 == 0 ? PI / 32 : -PI / 32;
				real_t spd = sqrt(my->vel_x * my->vel_x + my->vel_y * my->vel_y);
				fx->vel_x = spd * 0.05 * cos(dir);
				fx->vel_y = spd * 0.05 * sin(dir);
				fx->lightBonus = vec4{ 0.25f, 0.25f, 0.25f, 0.f };
				fx->pitch = PI / 2;
				fx->roll = 0.0;
				fx->yaw = dir;
				fx->vel_z = 0.0;
			}
		}
		else if ( my->sprite == items[SLOP_BALL].index )
		{
			if ( Entity* fx = spawnMagicParticleCustom(my, 1926, 1.0, 1.0) )
			{
				fx->ditheringDisabled = true;
				real_t dir = atan2(my->vel_y, my->vel_x);
				//dir += local_rng.rand() % 2 == 0 ? PI / 32 : -PI / 32;
				real_t spd = sqrt(my->vel_x * my->vel_x + my->vel_y * my->vel_y);
				fx->vel_x = spd * 0.05 * cos(dir);
				fx->vel_y = spd * 0.05 * sin(dir);
				fx->lightBonus = vec4{ 0.25f, 0.25f, 0.25f, 0.f };
				fx->pitch = PI / 2;
				fx->roll = 0.0;
				fx->yaw = dir;
				fx->vel_z = 0.0;
			}
		}
		else if ( my->sprite == items[GEM_JEWEL].index )
		{
			if ( my->ticks % 4 == 0 )
			{
				if ( Entity* fx = spawnMagicParticleCustom(my, 2410, 1.0, 1.0) )
				{
					fx->ditheringDisabled = true;
					fx->focalz = 0.25;
					fx->vel_z = 0.04;
					fx->fskill[0] = 0.04;
					real_t dir = atan2(my->vel_y, my->vel_x);
					if ( local_rng.rand() % 2 )
					{
						dir += PI;
					}
					real_t spd = sqrt(my->vel_x * my->vel_x + my->vel_y * my->vel_y);
					fx->vel_x = spd * 0.05 * cos(dir + PI / 2);
					fx->vel_y = spd * 0.05 * sin(dir + PI / 2);
				}
			}
			my->light = addLight(my->x / 16, my->y / 16, "jewel_yellow");
		}
	}
	else
	{
		// select appropriate model
		my->skill[2] = -8;
		my->flags[INVISIBLE] = false;
		item = newItemFromEntity(my, true);
		if ( item )
		{
			my->sprite = itemModel(item);
			if ( item->type == BOOMERANG )
			{
				my->sprite = BOOMERANG_PARTICLE;
				if ( my->ticks > 0 && my->ticks % 7 == 0 )
				{
					playSoundEntityLocal(my, 434 + local_rng.rand() % 10, 64);
				}
			}
			else if ( item->type == BOLAS )
			{
				my->sprite = BOLAS_PARTICLE;
			}
			free(item);
			item = nullptr;
		}
	}

	++THROWN_LIFE;

	if ( my->sprite == items[TOOL_BOMB].index || my->sprite == items[TOOL_FREEZE_BOMB].index
		|| my->sprite == items[TOOL_SLEEP_BOMB].index || my->sprite == items[TOOL_TELEPORT_BOMB].index )
	{
		my->focalz = 0.5;
	}

	if ( THROWN_LINGER != 0 )
	{
		if ( my->ticks > (THROWN_LINGER + 1) )
		{
			my->removeLightField();
			list_RemoveNode(my->mynode);
			return;
		}
		return;
	}

	if ( multiplayer == CLIENT )
	{
		return;
	}

	Entity* parent = uidToEntity(my->parent);
	bool specialMonster = false;
	if ( parent && parent->getRace() == LICH_ICE )
	{
		specialMonster = true;
	}

	if ( THROWN_LIFE == 1 && cat == THROWN && parent && parent->behavior != &actPlayer )
	{
		my->createWorldUITooltip();
	}

	// gravity
	real_t groundHeight = 7.5 - models[my->sprite]->sizey * .25;
	if ( type == TOOL_SENTRYBOT || type == TOOL_SPELLBOT )
	{
		groundHeight = 3;
	}
	else if ( type == BOOMERANG )
	{
		groundHeight = 10.0 - models[my->sprite]->sizey * .25;
	}
	bool processXYCollision = true;
	if ( my->z < groundHeight )
	{
		// fall
		if ( cat == THROWN )
		{
			// todo: adjust falling rates for thrown items if need be
			if ( type == BOOMERANG )
			{
				if ( !THROWN_BOOMERANG_STOP_Z )
				{
					if ( THROWN_VELZ > 0.001 )
					{
						THROWN_VELZ += 0.005;
						if ( THROWN_VELZ > 0.05 )
						{
							THROWN_VELZ = -0.001;
						}
					}
					else
					{
						THROWN_VELZ -= 0.005;
						if ( THROWN_VELZ < -0.05 )
						{
							THROWN_BOOMERANG_STOP_Z = 1;
							THROWN_VELZ = 0.f;
						}
					}
					my->z += THROWN_VELZ;
				}
			}
			else if ( specialMonster )
			{
				THROWN_VELZ += 0.01;
				my->z += THROWN_VELZ;
			}
			else
			{
				THROWN_VELZ += 0.03;
				my->z += THROWN_VELZ;
			}
			/*THROWN_VELX = 0.f;
			THROWN_VELY = 0.f;
			THROWN_VELZ = 0.f;*/
			if ( type == BRONZE_TOMAHAWK || type == IRON_DAGGER || type == BONE_THROWING )
			{
				// axe and dagger spin vertically
				my->pitch += 0.2;
			}
			else if ( type == BLACKIRON_DART || type == SILVER_PLUMBATA )
			{
				my->roll += 0.2;
			}
			else
			{
				if ( type == BOOMERANG )
				{
					my->pitch = std::max(my->pitch - 0.03, 0.0);
					my->roll -= 0.5;
					my->focalx = 2;
					my->focaly = 0;
					my->focalz = 0.5;
				}
				else if ( specialMonster )
				{
					my->roll += 0.003;
					my->yaw += 0.5;
				}
				else
				{
					my->roll += 0.01;
					my->yaw += 0.5;
				}
			}
		}
		else if ( itemIsThrowableTinkerTool(item) )
		{
			if ( type >= TOOL_BOMB && type <= TOOL_TELEPORT_BOMB )
			{
				my->yaw += 0.2;
			}
			else if ( type == TOOL_SENTRYBOT || type == TOOL_SPELLBOT )
			{
				my->roll += 0.07;
				my->roll = std::min(my->roll, 0.0);
			}
			else
			{
				my->roll += 0.05;
			}
			THROWN_VELZ += 0.04;
			my->z += THROWN_VELZ;
		}
		else
		{
			THROWN_VELZ += 0.04;
			my->z += THROWN_VELZ;
			my->roll += 0.04;
		}

		if ( my->sprite == items[GREASE_BALL].index )
		{
			if ( my->ticks % 5 == 0 )
			{
				spawnGreasePuddleSpawner(my->parent == 0 ? nullptr : uidToEntity(my->parent), my->x, my->y, 30 * TICKS_PER_SECOND);
			}
			if ( Entity* fx = spawnMagicParticleCustom(my, 245, 1.0, 1.0) )
			{
				fx->ditheringDisabled = true;
				real_t dir = atan2(my->vel_y, my->vel_x);
				//dir += local_rng.rand() % 2 == 0 ? PI / 32 : -PI / 32;
				real_t spd = sqrt(my->vel_x * my->vel_x + my->vel_y * my->vel_y);
				fx->vel_x = spd * 0.05 * cos(dir);
				fx->vel_y = spd * 0.05 * sin(dir);
				fx->flags[BRIGHT] = true;
				fx->pitch = PI / 2;
				fx->roll = 0.0;
				fx->yaw = dir;
				fx->vel_z = 0.0;
				fx->flags[SPRITE] = true;
			}
		}
		else if ( my->sprite == items[DUST_BALL].index )
		{
			if ( my->ticks == 1 )
			{
				Uint32 lifetime = TICKS_PER_SECOND * 3;
				Entity* spellTimer = createParticleTimer(my->parent == 0 ? nullptr : uidToEntity(my->parent), lifetime + TICKS_PER_SECOND, -1);
				spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SPORES_TRAIL;
				spellTimer->particleTimerCountdownSprite = 248;
				spellTimer->yaw = 0.0;
				spellTimer->x = my->x;
				spellTimer->y = my->y;
				spellTimer->particleTimerVariable1 = 0;
				spellTimer->particleTimerVariable2 = SPELL_MYCELIUM_SPORES;
				spellTimer->particleTimerVariable4 = my->getUID();

				my->thrownProjectileParticleTimerUID = spellTimer->getUID();
				particleTimerEffects.emplace(std::pair<Uint32, ParticleTimerEffect_t>(spellTimer->getUID(), ParticleTimerEffect_t()));
			}
			if ( Entity* fx = spawnMagicParticleCustom(my, 1886, 1.0, 1.0) )
			{
				fx->ditheringDisabled = true;
				real_t dir = atan2(my->vel_y, my->vel_x);
				//dir += local_rng.rand() % 2 == 0 ? PI / 32 : -PI / 32;
				real_t spd = sqrt(my->vel_x * my->vel_x + my->vel_y * my->vel_y);
				fx->vel_x = spd * 0.05 * cos(dir);
				fx->vel_y = spd * 0.05 * sin(dir);
				fx->lightBonus = vec4{ 0.25f, 0.25f, 0.25f, 0.f };
				fx->roll = 0.0;
				fx->yaw = dir;
				fx->vel_z = 0.0;
			}
			thrownItemUpdateSpellTrail(*my, my->x, my->y);
		}
		else if ( my->sprite == items[SLOP_BALL].index )
		{
			if ( Entity* fx = spawnMagicParticleCustom(my, 1926, 1.0, 1.0) )
			{
				fx->ditheringDisabled = true;
				real_t dir = atan2(my->vel_y, my->vel_x);
				//dir += local_rng.rand() % 2 == 0 ? PI / 32 : -PI / 32;
				real_t spd = sqrt(my->vel_x * my->vel_x + my->vel_y * my->vel_y);
				fx->vel_x = spd * 0.05 * cos(dir);
				fx->vel_y = spd * 0.05 * sin(dir);
				fx->lightBonus = vec4{ 0.25f, 0.25f, 0.25f, 0.f };
				fx->roll = 0.0;
				fx->yaw = dir;
				fx->vel_z = 0.0;
			}
		}
		else if ( my->sprite == items[GEM_JEWEL].index )
		{
			if ( my->ticks % 4 == 0 )
			{
				if ( Entity* fx = spawnMagicParticleCustom(my, 2410, 1.0, 1.0) )
				{
					fx->ditheringDisabled = true;
					fx->focalz = 0.25;
					fx->vel_z = 0.04;
					fx->fskill[0] = 0.04;
					real_t dir = atan2(my->vel_y, my->vel_x);
					if ( local_rng.rand() % 2 )
					{
						dir += PI;
					}
					real_t spd = sqrt(my->vel_x * my->vel_x + my->vel_y * my->vel_y);
					fx->vel_x = spd * 0.05 * cos(dir + PI / 2);
					fx->vel_y = spd * 0.05 * sin(dir + PI / 2);
				}
			}
			my->light = addLight(my->x / 16, my->y / 16, "jewel_yellow");
		}
	}
	else
	{
		if ( my->x >= 0 && my->y >= 0 && my->x < map.width << 4 && my->y < map.height << 4 )
		{
			if ( my->sprite == items[GREASE_BALL].index )
			{
				spawnGreasePuddleSpawner(my->parent == 0 ? nullptr : uidToEntity(my->parent), my->x, my->y, 30 * TICKS_PER_SECOND);
			}
			if ( my->sprite == items[DUST_BALL].index )
			{
				thrownItemUpdateSpellTrail(*my, my->x, my->y);
			}

			// landing on the ground.
			int index = (int)(my->y / 16)*MAPLAYERS + (int)(my->x / 16)*MAPLAYERS * map.height;
			if ( map.tiles[index] )
			{
				item = newItemFromEntity(my);
				bool tinkeringItemCanBePlaced = true;
				if ( item && item->isTinkeringItemWithThrownLimit() )
				{
					if ( !parent )
					{
						tinkeringItemCanBePlaced = true;
					}
					else if ( parent->behavior == &actMonster )
					{
						tinkeringItemCanBePlaced = true;
					}
					else if ( parent->behavior == &actPlayer )
					{
						tinkeringItemCanBePlaced = playerCanSpawnMoreTinkeringBots(stats[parent->skill[2]]);
					}
				}
				
				if ( item && item->type == TOOL_GYROBOT && tinkeringItemCanBePlaced )
				{
					if ( parent )
					{
						item->applyTinkeringCreation(parent, my);
					}
					free(item);
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
				else if ( item && item->type == TOOL_DUCK )
				{
					item->applyDuck(my->parent, my->x, my->y, nullptr, false);
					free(item);
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
				else if ( itemCategory(item) == POTION )
				{
					switch ( item->type )
					{
						case POTION_FIRESTORM:
							spawnMagicTower(parent, my->x, my->y, SPELL_FIREBALL, nullptr);
							break;
						case POTION_ICESTORM:
							spawnMagicTower(parent, my->x, my->y, SPELL_COLD, nullptr);
							break;
						case POTION_THUNDERSTORM:
							spawnMagicTower(parent, my->x, my->y, SPELL_LIGHTNING, nullptr);
							break;
						default:
							break;
					}
					playSoundEntity(my, 162, 64);
					free(item);
					onThrownLandingParticle(my);
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
				else if ( specialMonster )
				{
					free(item);
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
				else if ( item && (item->type == DUST_BALL 
					|| item->type == GREASE_BALL
					|| item->type == SLOP_BALL) )
				{
					free(item);
					onThrownLandingParticle(my);
					playSoundEntity(my, 764, 64);
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
				else if ( itemCategory(item) == GEM && (item->beatitude < 0 || local_rng.rand() % 5 == 0)
					&& item->type != GEM_JEWEL )
				{
					// cursed gem, explode
					createParticleShatteredGem(my->x, my->y, 7.5, my->sprite, nullptr);
					serverSpawnMiscParticlesAtLocation(my->x, my->y, 7.5, PARTICLE_EFFECT_SHATTERED_GEM, my->sprite);
					free(item);
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
				else if ( item->type == BOOMERANG && uidToEntity(my->parent) )
				{
					Entity* parent = uidToEntity(my->parent);
					Entity* spellEntity = createParticleSapCenter(parent, my, 0, my->sprite, -1);
					if ( spellEntity )
					{
						spellEntity->skill[0] = 150; // 3 second lifetime.
						// store weapon data
						spellEntity->skill[10] = item->type;
						spellEntity->skill[11] = item->status;
						spellEntity->skill[12] = item->beatitude;
						spellEntity->skill[13] = 1;
						spellEntity->skill[14] = item->appearance;
						spellEntity->skill[15] = item->identified;
					}
				}
				else if ( item && (item->type >= TOOL_BOMB && item->type <= TOOL_TELEPORT_BOMB)
					&& !(swimmingtiles[map.tiles[index]] || lavatiles[map.tiles[index]]) )
				{
					// don't deploy on swimming/lava tiles.
					if ( parent )
					{
						item->applyBomb(parent, item->type, Item::ItemBombPlacement::BOMB_FLOOR, Item::ItemBombFacingDirection::BOMB_UP, my, nullptr);
					}
					free(item);
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
				else if ( item && itemIsThrowableTinkerTool(item) && !(item->type >= TOOL_BOMB && item->type <= TOOL_TELEPORT_BOMB)
					&& !(swimmingtiles[map.tiles[index]] || lavatiles[map.tiles[index]])
					&& tinkeringItemCanBePlaced )
				{
					// don't deploy on swimming/lava tiles.
					if ( parent )
					{
						item->applyTinkeringCreation(parent, my);
					}
					if ( item->type == TOOL_SENTRYBOT || item->type == TOOL_SPELLBOT )
					{
						// have the thrown particle hang out for a sec to avoid temporary flashing of nothingness.
						THROWN_LINGER = my->ticks;
						THROWN_VELX = 0.0;
						THROWN_VELY = 0.0;
						THROWN_VELZ = 0.0;
						my->z = groundHeight;
						free(item);
						return;
					}
					free(item);
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
				else
				{
					if ( item && parent && parent->behavior == &actPlayer )
					{
						if ( itemIsThrowableTinkerTool(item) && tinkeringItemCanBePlaced )
						{
							// we can place it, just not on water/lava.
							messagePlayer(parent->skill[2], MESSAGE_HINT, Language::get(3900));
						}
						else if ( item->isTinkeringItemWithThrownLimit() && !tinkeringItemCanBePlaced )
						{
							if ( stats[parent->skill[2]]->getModifiedProficiency(PRO_LOCKPICKING) >= SKILL_LEVEL_LEGENDARY )
							{
								messagePlayer(parent->skill[2], MESSAGE_MISC, Language::get(3884));
							}
							else
							{
								messagePlayer(parent->skill[2], MESSAGE_MISC, Language::get(3883));
							}
						}
					}
					Entity* entity = newEntity(-1, 1, map.entities, nullptr); //Item entity.
					entity->flags[INVISIBLE] = true;
					entity->flags[UPDATENEEDED] = true;
					entity->flags[PASSABLE] = true;
					entity->x = my->x;
					entity->y = my->y;
					entity->z = my->z;
					entity->sizex = my->sizex;
					entity->sizey = my->sizey;
					entity->yaw = my->yaw;
					entity->pitch = my->pitch;
					entity->roll = my->roll;
					entity->vel_x = THROWN_VELX;
					entity->vel_y = THROWN_VELY;
					entity->vel_z = my->vel_z;
					entity->behavior = &actItem;
					entity->skill[10] = item->type;
					entity->skill[11] = item->status;
					entity->skill[12] = item->beatitude;
					entity->skill[13] = item->count;
					entity->skill[14] = item->appearance;
					entity->skill[15] = item->identified;
					if ( item->type == GEM_JEWEL )
					{
						entity->parent = my->parent;
					}
					if ( itemCategory(item) == THROWN )
					{
						//Hack to make monsters stop catching your shurikens and chakrams.
						entity->parent = my->parent;
						if ( parent )
						{
							if ( Stat* parentStats = parent->getStats() )
							{
								if ( parentStats->getEffectActive(EFF_RETURN_ITEM) )
								{
									entity->itemReturnUID = parent->getUID();
								}
							}
						}
					}
					free(item);
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
			}
			else
			{
				if ( my->skill[10] == TOOL_GYROBOT )
				{
					item = newItemFromEntity(my);
					bool tinkeringItemCanBePlaced = true;
					if ( item && item->isTinkeringItemWithThrownLimit() )
					{
						if ( !parent )
						{
							tinkeringItemCanBePlaced = true;
						}
						else if ( parent->behavior == &actMonster )
						{
							tinkeringItemCanBePlaced = true;
						}
						else if ( parent->behavior == &actPlayer )
						{
							tinkeringItemCanBePlaced = playerCanSpawnMoreTinkeringBots(stats[parent->skill[2]]);
						}
					}
					if ( parent )
					{
						item->applyTinkeringCreation(parent, my);
					}
					free(item);
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
				else if ( my->skill[10] == TOOL_DUCK )
				{
					item = newItemFromEntity(my);
					item->applyDuck(my->parent, my->x, my->y, nullptr, false);
					free(item);
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}

				// fall
				if ( cat == THROWN )
				{
					// todo: adjust falling rates for thrown items if need be
					THROWN_VELZ += 0.04;
					my->z += THROWN_VELZ;
					my->roll += 0.04;
				}
				else
				{
					THROWN_VELZ += 0.04;
					my->z += THROWN_VELZ;
					my->roll += 0.04;
				}

				if ( my->z > groundHeight + 4 ) // if entity is 4 height below the ground, then fall straight down.
				{
					processXYCollision = false;
				}
			}
		}
		else
		{
			// fall out of x and y bounds
			if ( cat == THROWN )
			{
				// todo: adjust falling rates for thrown items if need be
				THROWN_VELZ += 0.04;
				my->z += THROWN_VELZ;
				my->roll += 0.04;
			}
			else
			{
				THROWN_VELZ += 0.04;
				my->z += THROWN_VELZ;
				my->roll += 0.04;
			}
		}
	}

	// pick up item
	if ( multiplayer != CLIENT && cat == THROWN )
	{
		for ( int i = 0; i < MAXPLAYERS; i++ )
		{
			if ( selectedEntity[i] == my || client_selected[i] == my )
			{
				if ( inrange[i] )
				{
					if ( players[i] != nullptr && players[i]->entity != nullptr )
					{
						playSoundEntity(players[i]->entity, 66, 64);
					}
					Item* item2 = newItemFromEntity(my);
					if ( item2 )
					{
						item = itemPickup(i, item2);
						if ( item )
						{
							if ( parent && (parent->getRace() == GOATMAN || parent->getRace() == INSECTOID) )
							{
								steamAchievementClient(i, "BARONY_ACH_ALL_IN_REFLEXES");
							}
							if ( players[i]->isLocalPlayer() )
							{
								free(item2);
							}
							int oldcount = item->count;
							item->count = 1;
							messagePlayer(i, MESSAGE_INTERACTION | MESSAGE_INVENTORY, Language::get(504), item->description());
							item->count = oldcount;
							if ( i != 0 && !players[i]->isLocalPlayer() )
							{
								free(item);
							}
							my->removeLightField();
							list_RemoveNode(my->mynode);
							return;
						}
					}
				}
			}
		}
	}

	// falling out of the map
	if ( my->z > 128 )
	{
		if ( my->sprite == BOOMERANG_PARTICLE ) // boomerang
		{
			item = newItemFromEntity(my);
			Entity* parent = uidToEntity(my->parent);
			if ( parent && item )
			{
				Entity* spellEntity = createParticleSapCenter(parent, my, 0, my->sprite, -1);
				if ( spellEntity )
				{
					spellEntity->skill[0] = 150; // 3 second lifetime.
					// store weapon data
					spellEntity->skill[10] = item->type;
					spellEntity->skill[11] = item->status;
					spellEntity->skill[12] = item->beatitude;
					spellEntity->skill[13] = 1;
					spellEntity->skill[14] = item->appearance;
					spellEntity->skill[15] = item->identified;
				}
			}
			free(item);
		}
		else if ( my->skill[10] >= WOODEN_SHIELD && my->skill[10] < NUMITEMS && items[my->skill[10]].category == THROWN )
		{
			if ( parent )
			{
				if ( Stat* parentStats = parent->getStats() )
				{
					if ( parentStats->getEffectActive(EFF_RETURN_ITEM) )
					{
						if ( Item* item = newItemFromEntity(my, true) )
						{
							Entity* entity = newEntity(-1, 1, map.entities, nullptr); //Item entity.
							entity->flags[INVISIBLE] = true;
							entity->flags[UPDATENEEDED] = true;
							entity->flags[PASSABLE] = true;
							entity->x = my->x;
							entity->y = my->y;
							entity->z = my->z;
							entity->sizex = my->sizex;
							entity->sizey = my->sizey;
							entity->yaw = my->yaw;
							entity->pitch = my->pitch;
							entity->roll = my->roll;
							entity->vel_x = THROWN_VELX;
							entity->vel_y = THROWN_VELY;
							entity->vel_z = my->vel_z;
							entity->behavior = &actItem;
							entity->skill[10] = item->type;
							entity->skill[11] = item->status;
							entity->skill[12] = item->beatitude;
							entity->skill[13] = item->count;
							entity->skill[14] = item->appearance;
							entity->skill[15] = item->identified;
							entity->itemReturnUID = parent->getUID();

							free(item);
						}
					}
				}
			}
		}
		my->removeLightField();
		list_RemoveNode(my->mynode);
		return;
	}

	// horizontal motion
	double ox = my->x;
	double oy = my->y;
	double oz = my->z;
	bool usedpotion = false;
	if ( !processXYCollision )
	{
		return;
	}
	my->processEntityWind();

	bool hitSomething = false;
	real_t result = 0.0;
	bool halfSpeedCheck = false;
	static ConsoleVariable<bool> cvar_thrown_clip("/thrown_clip_test", true);
	real_t speed = sqrt(THROWN_VELX * THROWN_VELX + THROWN_VELY * THROWN_VELY);
	if ( speed > 4.0 ) // can clip through thin gates
	{
		auto entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 1);
		for ( auto it : entLists )
		{
			if ( !*cvar_thrown_clip && (svFlags & SV_FLAG_CHEATS) )
			{
				break;
			}
			for ( node_t* node = it->first; node != nullptr; node = node->next )
			{
				Entity* entity = (Entity*)node->element;
				if ( entity->behavior == &actGate || entity->behavior == &actDoor || entity->behavior == &actIronDoor )
				{
					if ( entityDist(my, entity) <= speed )
					{
						halfSpeedCheck = true;
						break;
					}
				}
			}
			if ( halfSpeedCheck )
			{
				break;
			}
		}
	}

	if ( !halfSpeedCheck )
	{
		result = clipMove(&my->x, &my->y, THROWN_VELX, THROWN_VELY, my);
		hitSomething = result != sqrt(THROWN_VELX * THROWN_VELX + THROWN_VELY * THROWN_VELY);
	}
	else
	{
		real_t vel_x = THROWN_VELX / 2.0;
		real_t vel_y = THROWN_VELY / 2.0;
		real_t dist = clipMove(&my->x, &my->y, vel_x, vel_y, my);
		result = dist;
		hitSomething = dist != sqrt(vel_x * vel_x + vel_y * vel_y);
		if ( !hitSomething )
		{
			dist = clipMove(&my->x, &my->y, vel_x, vel_y, my);
			result += dist;
			hitSomething = dist != sqrt(vel_x * vel_x + vel_y * vel_y);
		}
	}
	if ( processXYCollision && hitSomething )
	{
		item = newItemFromEntity(my);
		if ( !item )
		{
			return;
		}
		if ( itemCategory(item) == THROWN 
			&& (item->type == STEEL_CHAKRAM || item->type == CRYSTAL_SHURIKEN) )
		{
			real_t bouncePenalty = 0.85;
			// shurikens and chakrams bounce off walls.
			if ( hit.side == HORIZONTAL )
			{
				THROWN_VELX = -THROWN_VELX * bouncePenalty;
			}
			else if ( hit.side == VERTICAL )
			{
				THROWN_VELY = -THROWN_VELY * bouncePenalty;
			}
			else if ( hit.side == 0 )
			{
				THROWN_VELY = -THROWN_VELY * bouncePenalty;
				THROWN_VELX = -THROWN_VELX * bouncePenalty;
			}
			++THROWN_BOUNCES;
		}

		if ( item->type == BOOMERANG )
		{
			Entity* parent = uidToEntity(my->parent);
			if ( parent )
			{
				Entity* spellEntity = createParticleSapCenter(parent, my, 0, my->sprite, -1);
				if ( spellEntity )
				{
					spellEntity->skill[0] = 150; // 3 second lifetime.
					// store weapon data
					spellEntity->skill[10] = item->type;
					spellEntity->skill[11] = item->status;
					spellEntity->skill[12] = item->beatitude;
					spellEntity->skill[13] = 1;
					spellEntity->skill[14] = item->appearance;
					spellEntity->skill[15] = item->identified;
				}
			}
		}

		cat = itemCategory(item);
		itemname = item->getName();
		item->count = 1;

		if ( itemCategory(item) == THROWN || itemCategory(item) == GEM || itemCategory(item) == POTION )
		{
			my->entityCheckIfTriggeredBomb(true);
			if ( !hit.entity )
			{
				my->entityCheckIfTriggeredWallButton();
			}
		}
		bool tryHitEntity = true;
		if ( itemIsThrowableTinkerTool(item) && !(item->type >= TOOL_BOMB && item->type <= TOOL_TELEPORT_BOMB) )
		{
			tryHitEntity = false;
		}
		if ( item->type == GEM_JEWEL )
		{
			tryHitEntity = false;
			if ( hit.entity != nullptr )
			{
				if ( hit.entity->behavior == &actMonster && hit.entity->getStats() )
				{
					Entity* parent = uidToEntity(my->parent);
					if ( parent && parent->behavior == &actPlayer )
					{
						if ( entityWantsJewel(item->status, *hit.entity, *hit.entity->getStats(), false) )
						{
							if ( jewelItemRecruit(parent, hit.entity, item->status, nullptr) )
							{
								free(item);
								my->removeLightField();
								list_RemoveNode(my->mynode);
								return;
							}
						}
					}
				}
			}
		}

		if ( my->sprite == items[GREASE_BALL].index )
		{
			spawnGreasePuddleSpawner(my->parent == 0 ? nullptr : uidToEntity(my->parent), my->x, my->y, 30 * TICKS_PER_SECOND);
			if ( hit.entity != nullptr && tryHitEntity )
			{
				spawnGreasePuddleSpawner(my->parent == 0 ? nullptr : uidToEntity(my->parent), hit.entity->x, hit.entity->y, 30 * TICKS_PER_SECOND);
			}
		}
		if ( my->sprite == items[DUST_BALL].index )
		{
			thrownItemUpdateSpellTrail(*my, my->x, my->y);
			if ( hit.entity != nullptr && tryHitEntity )
			{
				thrownItemUpdateSpellTrail(*my, hit.entity->x, hit.entity->y);
			}
		}

		if ( hit.entity != nullptr && tryHitEntity )
		{
			Entity* parent = uidToEntity(my->parent);
			Stat* parentStats = nullptr;
			if ( parent )
			{
				parentStats = parent->getStats();
			}
			Stat* hitstats = hit.entity->getStats();
			bool friendlyHit = false;
			if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
			{
				// test for friendly fire
				if ( parent && parent->checkFriend(hit.entity) && parent->friendlyFireProtection(hit.entity) )
				{
					friendlyHit = true;
				}
			}
			if ( item->type >= TOOL_BOMB && item->type <= TOOL_TELEPORT_BOMB )
			{
				if ( hit.entity->behavior == &actChest || hit.entity->isInertMimic() )
				{
					item->applyBomb(parent, item->type, Item::ItemBombPlacement::BOMB_CHEST, Item::ItemBombFacingDirection::BOMB_UP, my, hit.entity);
					free(item);
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
				else if ( hit.entity->behavior == &actDoor || hit.entity->behavior == &actIronDoor )
				{
					item->applyBomb(parent, item->type, Item::ItemBombPlacement::BOMB_DOOR, Item::ItemBombFacingDirection::BOMB_UP, my, hit.entity);
					free(item);
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
				else if ( hit.entity->isDamageableCollider() && hit.entity->isColliderDamageableByMagic()
					&& hit.entity->isColliderAttachableToBombs() )
				{
					item->applyBomb(parent, item->type, Item::ItemBombPlacement::BOMB_COLLIDER, Item::ItemBombFacingDirection::BOMB_UP, my, hit.entity);
					free(item);
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
			}
			else if ( (hit.entity->behavior == &actMonster && !hit.entity->isInertMimic())
				|| hit.entity->behavior == &actPlayer )
			{
				int oldHP = 0;
				oldHP = hit.entity->getHP();
				int damage = (BASE_THROWN_DAMAGE + item->weaponGetAttack(parentStats));
				bool thrownTypeWeapon = false;
				if ( parentStats )
				{
					if ( itemCategory(item) == POTION )
					{
						int skillLVL = parentStats->getModifiedProficiency(PRO_ALCHEMY) / 20;
						//int dex = parent->getDEX() / 4;
						//damage += dex;
						damage = damage * potionDamageSkillMultipliers[std::min(skillLVL, 5)];
						damage -= local_rng.rand() % ((damage / 4) + 1);
					}
					else
					{
						if ( itemCategory(item) == THROWN 
							&& !(item->type == DUST_BALL
							|| item->type == SLOP_BALL
							|| item->type == GREASE_BALL) )
						{
							int enemyAC = AC(hitstats);
							damage = my->thrownProjectilePower;
							if ( my->thrownProjectileCharge >= 1 )
							{
								damage += my->thrownProjectileCharge / 5; //0-3 base +damage
								real_t bypassArmor = 1 - my->thrownProjectileCharge * 0.05; //100-25% of armor taken into account
								enemyAC *= bypassArmor;
							}
							else
							{
								enemyAC *= .5;
							}

							int numBlessings = 0;
							real_t targetACEffectiveness = Entity::getACEffectiveness(hit.entity, hitstats, hit.entity->behavior == &actPlayer, parent, parentStats, numBlessings);
							int attackAfterReductions = static_cast<int>(std::max(0.0, ((damage * targetACEffectiveness - enemyAC))) + (1.0 - targetACEffectiveness) * damage);
							damage = attackAfterReductions;

							thrownTypeWeapon = true;
						}
						else
						{
							damage = my->thrownProjectilePower;
							if ( my->thrownProjectileCharge >= 1 )
							{
								damage += my->thrownProjectileCharge / 5;
							}
							damage -= (AC(hit.entity->getStats()) * .5);
						}

						if ( hitstats && hitstats->getEffectActive(EFF_GUARD_BODY) )
						{
							thaumSpellArmorProc(hit.entity, *hitstats, false, parent, EFF_GUARD_BODY);
						}
						if ( hitstats && hitstats->getEffectActive(EFF_DIVINE_GUARD) )
						{
							thaumSpellArmorProc(hit.entity, *hitstats, false, parent, EFF_DIVINE_GUARD);
						}
					}
				}
				if ( hitstats && !hitstats->defending )
				{
					// zero out the damage if negative, thrown weapons will do piercing damage if not blocking.
					damage = std::max(0, damage);
				}
				switch ( item->type )
				{
					// thrown weapons do some base damage if absorbed by armor.
					case BRONZE_TOMAHAWK:
					case IRON_DAGGER:
					case STEEL_CHAKRAM:
					case CRYSTAL_SHURIKEN:
					case BOOMERANG:
					{
						if ( damage <= 0 && hit.entity->behavior == &actPlayer )
						{
							damage += item->weaponGetAttack(parentStats);
						}
						break;
					}
					case FOOD_CREAMPIE:
						damage = 0;
						break;
					case TOOL_DUCK:
						damage = 1;
						break;
					default:
						break;
				}
				damage = std::max(0, damage);
				//messagePlayer(0, "damage: %d", damage);
				if ( parent && parent->behavior == &actPlayer && parent->checkFriend(hit.entity) && itemCategory(item) == POTION )
				{
					if ( !item->doesPotionHarmAlliesOnThrown() )
					{
						damage = 0;
					}
					damage = std::min(10, damage); // impact damage is 10 max on allies.
				}

				bool envenomWeapon = false;
				if ( parent )
				{
					Stat* parentStats = parent->getStats();
					if ( parentStats && parentStats->getEffectActive(EFF_ENVENOM_WEAPON) && hitstats )
					{
						if ( local_rng.rand() % 2 == 0 )
						{
							int envenomDamage = std::min(
								getSpellDamageSecondaryFromID(SPELL_ENVENOM_WEAPON, parent, parentStats, parent),
								getSpellDamageFromID(SPELL_ENVENOM_WEAPON, parent, parentStats, parent));

							hit.entity->modHP(-envenomDamage); // do the damage
							for ( int tmp = 0; tmp < 3; ++tmp )
							{
								Entity* gib = spawnGib(hit.entity, 211);
								serverSpawnGibForClient(gib);
							}
							if ( !hitstats->getEffectActive(EFF_POISONED) )
							{
								envenomWeapon = true;
								hitstats->setEffectActive(EFF_POISONED, 1);

								int duration = 160 * envenomDamage;
								hitstats->EFFECTS_TIMERS[EFF_POISONED] = std::max(200, duration - hit.entity->getCON() * 20);
								hitstats->poisonKiller = parent->getUID();
								if ( hit.entity->isEntityPlayer() )
								{
									messagePlayerMonsterEvent(hit.entity->isEntityPlayer(), makeColorRGB(255, 0, 0), *parentStats, Language::get(6531), Language::get(6532), MSG_COMBAT);
									serverUpdateEffects(hit.entity->isEntityPlayer());
								}

								if ( parent->behavior == &actPlayer )
								{
									players[parent->skill[2]]->mechanics.updateSustainedSpellEvent(SPELL_ENVENOM_WEAPON, 50.0, 1.0, hit.entity);
								}
							}
						}
					}
				}

				char whatever[256] = "";
				if ( !friendlyHit )
				{
					Sint32 oldHP = hitstats->HP;
					hit.entity->modHP(-damage);

					if ( hitstats )
					{
						Sint32 damageTaken = oldHP - hitstats->HP;
						if ( damageTaken > 0 )
						{
							if ( hitstats->getEffectActive(EFF_DEFY_FLESH) )
							{
								hit.entity->defyFleshProc(parent);
							}
							hit.entity->pinpointDamageProc(parent, damageTaken);
						}
					}

					if ( hit.entity->behavior == &actPlayer )
					{
						Compendium_t::Events_t::eventUpdateCodex(hit.entity->skill[2], Compendium_t::CPDM_HP_MOST_DMG_LOST_ONE_HIT, "hp", oldHP - hitstats->HP);
					}
					if ( parent && parent->behavior == &actPlayer )
					{
						Compendium_t::Events_t::eventUpdate(parent->skill[2], 
							Compendium_t::CPDM_DMG_MAX, item->type, damage);
						if ( oldHP > hitstats->HP )
						{
							Compendium_t::Events_t::eventUpdate(parent->skill[2],
								Compendium_t::CPDM_THROWN_DMG_TOTAL, item->type, oldHP - hitstats->HP);
						}
						if ( cat == THROWN )
						{
							if ( oldHP > hitstats->HP )
							{
								Compendium_t::Events_t::eventUpdateCodex(parent->skill[2], Compendium_t::CPDM_THROWN_DMG_TOTAL, "thrown", oldHP - hitstats->HP);
								Compendium_t::Events_t::eventUpdateCodex(parent->skill[2], Compendium_t::CPDM_THROWN_TOTAL_HITS, "thrown", 1);
								Compendium_t::Events_t::eventUpdateCodex(parent->skill[2], Compendium_t::CPDM_CLASS_THROWN_HITS_RUN, "thrown", 1);
							}
						}
					}
				}

				if ( parent && parent->behavior == &actPlayer )
				{
					Compendium_t::Events_t::eventUpdate(parent->skill[2],
						Compendium_t::CPDM_THROWN_HITS, item->type, 1);
				}

				// set the obituary
				snprintf(whatever, 255, Language::get(1508), itemname);
				hit.entity->setObituary(whatever);
				if (hitstats) {
				    hitstats->killer = KilledBy::ITEM;
				    hitstats->killer_item = item->type;
				}
				bool skipMessage = false;
				bool goggleProtection = false;
				Entity* polymorphedTarget = nullptr;
				bool disableAlertBlindStatus = false;
				bool ignorePotion = false;
				bool wasPotion = itemCategory(item) == POTION;
				ItemType itemType = item->type;
				bool wasConfused = (hitstats && hitstats->getEffectActive(EFF_CONFUSED));
				Uint32 prevTarget = hit.entity->behavior == &actMonster ? hit.entity->monsterTarget : 0;
				bool healingPotion = false;

				if ( hitstats )
				{
					int postDmgHP = hit.entity->getHP();
					if ( hitstats->type == LICH || hitstats->type == SHOPKEEPER || hitstats->type == DEVIL
						|| hitstats->type == MINOTAUR || hitstats->type == LICH_FIRE || hitstats->type == LICH_ICE )
					{
						switch ( item->type )
						{
							case POTION_SICKNESS:
							case POTION_SPEED:
							case POTION_ACID:
							case POTION_FIRESTORM:
							case POTION_ICESTORM:
							case POTION_THUNDERSTORM:
							case POTION_POLYMORPH:
							case POTION_WATER:
								ignorePotion = false;
								break;
							case POTION_EXTRAHEALING:
							case POTION_HEALING:
							case POTION_BOOZE:
							case POTION_JUICE:
							case POTION_CONFUSION:
							case POTION_CUREAILMENT:
							case POTION_BLINDNESS:
							case POTION_RESTOREMAGIC:
							case POTION_INVISIBILITY:
							case POTION_LEVITATION:
							case POTION_STRENGTH:
							case POTION_PARALYSIS:
							case FOOD_CREAMPIE:
							case TOOL_DUCK:
								ignorePotion = true;
								break;
							default:
								break;
						}

						if ( ignorePotion )
						{
							if ( parent && parent->behavior == &actPlayer && itemCategory(item) == POTION )
							{
								Uint32 color = makeColorRGB(255, 0, 0);
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(4320), Language::get(4321), MSG_COMBAT);
							}
						}
					}
					else
					{
						if ( parent && parent->behavior == &actPlayer && parent->checkFriend(hit.entity) && itemCategory(item) == POTION )
						{
							if ( damage != 0 && friendlyHit )
							{
								ignorePotion = true;
							}
						}

						if ( hitstats->mask && hitstats->mask->type == MASK_HAZARD_GOGGLES )
						{
							if ( !(hit.entity->behavior == &actPlayer && hit.entity->effectShapeshift != NOTHING) )
							{
								if ( itemCategory(item) == POTION && item->doesPotionHarmAlliesOnThrown() )
								{
									ignorePotion = true;
									goggleProtection = true;
								}
							}
						}
					}
					if ( !ignorePotion )   // this makes it impossible to bork the end boss :)
					{
						if ( local_rng.rand() % 4 == 0 && parent != NULL && itemCategory(item) == POTION && item->type != POTION_EMPTY )
						{
							parent->increaseSkill(PRO_ALCHEMY);
						}
						switch ( itemType )
						{
							case POTION_WATER:
								usedpotion = true;
								item_PotionWater(item, hit.entity, parent);
								healingPotion = true;
								break;
							case POTION_BOOZE:
								item_PotionBooze(item, hit.entity, parent);
								if ( parentStats && parentStats->getEffectActive(EFF_DRUNK) )
								{
									steamAchievementEntity(parent, "BARONY_ACH_CHEERS");
									if ( hit.entity->behavior == &actMonster && parent && parent->behavior == &actPlayer )
									{
										if ( parentStats->type == GOATMAN
											&& (hitstats->type == HUMAN || hitstats->type == GOBLIN
												|| hitstats->type == INCUBUS || hitstats->type == SUCCUBUS )
											&& hitstats->leader_uid == 0 )
										{
											if ( forceFollower(*parent, *hit.entity) )
											{
												spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, 685);
												parent->increaseSkill(PRO_LEADERSHIP);
												messagePlayerMonsterEvent(parent->skill[2], makeColorRGB(0, 255, 0), 
													*hitstats, Language::get(3252), Language::get(3251), MSG_COMBAT);
												if ( hit.entity->monsterAllyIndex != parent->skill[2] )
												{
													Compendium_t::Events_t::eventUpdateMonster(parent->skill[2], Compendium_t::CPDM_RECRUITED, hit.entity, 1);
													Compendium_t::Events_t::eventUpdateCodex(parent->skill[2], Compendium_t::CPDM_RACE_RECRUITS, "races", 1);
													if ( hitstats->type == HUMAN && hitstats->getAttribute("special_npc") == "merlin" )
													{
														Compendium_t::Events_t::eventUpdateWorld(parent->skill[2], Compendium_t::CPDM_MERLINS, "magicians guild", 1);
													}
												}
												hit.entity->monsterAllyIndex = parent->skill[2];
												if ( multiplayer == SERVER )
												{
													serverUpdateEntitySkill(hit.entity, 42); // update monsterAllyIndex for clients.
												}

												if ( hit.entity->monsterTarget == parent->getUID() )
												{
													hit.entity->monsterReleaseAttackTarget();
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
												friendlyHit = true;
											}
										}
									}
								}
								healingPotion = true;
								usedpotion = true;
								break;
							case POTION_JUICE:
								item_PotionJuice(item, hit.entity, parent);
								usedpotion = true;
								healingPotion = true;
								break;
							case POTION_SICKNESS:
								item_PotionSickness(item, hit.entity, parent);
								usedpotion = true;
								break;
							case POTION_CONFUSION:
								item_PotionConfusion(item, hit.entity, parent);
								disableAlertBlindStatus = true; // don't aggro target.
								usedpotion = true;
								break;
							case POTION_EXTRAHEALING:
							{
								item_PotionExtraHealing(item, hit.entity, parent);
								if ( parent && parent->behavior == &actPlayer )
								{
									if ( parent->checkFriend(hit.entity) )
									{
										steamAchievementClient(parent->skill[2], "BARONY_ACH_THANK_ME_LATER");
									}
									int heal = std::max(hit.entity->getHP() - postDmgHP, 0);
									if ( heal > 0 )
									{
										serverUpdatePlayerGameplayStats(parent->skill[2], STATISTICS_HEAL_BOT, heal);
									}
								}
								usedpotion = true;
								healingPotion = true;
							}
								break;
							case POTION_HEALING:
							{
								item_PotionHealing(item, hit.entity, parent);
								if ( parent && parent->behavior == &actPlayer )
								{
									if ( parent->checkFriend(hit.entity) )
									{
										steamAchievementClient(parent->skill[2], "BARONY_ACH_THANK_ME_LATER");
									}
									int heal = std::max(hit.entity->getHP() - postDmgHP, 0);
									if ( heal > 0 )
									{
										serverUpdatePlayerGameplayStats(parent->skill[2], STATISTICS_HEAL_BOT, heal);
									}
								}
								usedpotion = true;
								healingPotion = true;
							}
								break;
							case POTION_CUREAILMENT:
								item_PotionCureAilment(item, hit.entity, parent);
								usedpotion = true;
								break;
							case POTION_BLINDNESS:
							{
								item_PotionBlindness(item, hit.entity, parent);
								disableAlertBlindStatus = true; // don't aggro target.
								usedpotion = true;
								break;
							}
							case POTION_RESTOREMAGIC:
								item_PotionRestoreMagic(item, hit.entity, parent);
								usedpotion = true;
								break;
							case POTION_INVISIBILITY:
								item_PotionInvisibility(item, hit.entity, parent);
								usedpotion = true;
								break;
							case POTION_LEVITATION:
								item_PotionLevitation(item, hit.entity, parent);
								usedpotion = true;
								break;
							case POTION_SPEED:
								item_PotionSpeed(item, hit.entity, parent);
								usedpotion = true;
								break;
							case POTION_ACID:
								item_PotionAcid(item, hit.entity, parent);
								usedpotion = true;
								break;
							case POTION_FIRESTORM:
							case POTION_ICESTORM:
							case POTION_THUNDERSTORM:
								item_PotionUnstableStorm(item, hit.entity, parent, my);
								usedpotion = true;
								break;
							case POTION_STRENGTH:
								item_PotionStrength(item, hit.entity, parent);
								usedpotion = true;
								break;
							case POTION_PARALYSIS:
								item_PotionParalysis(item, hit.entity, parent);
								usedpotion = true;
								break;
							case GREASE_BALL:
								item_PotionGrease(item, hit.entity, parent);
								//usedpotion = true;
								break;
							case DUST_BALL:
								if ( hit.entity->setEffect(EFF_DUSTED, true, 5 * TICKS_PER_SECOND + 10, true) )
								{
									if ( hit.entity->behavior == &actPlayer )
									{
										messagePlayerColor(hit.entity->skill[2], MESSAGE_STATUS, makeColorRGB(255, 0, 0), Language::get(6752));
									}
									if ( parent && parent->behavior == &actPlayer )
									{
										Uint32 color = makeColorRGB(0, 255, 0);
										messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(6762), Language::get(6761), MSG_COMBAT);
									}
									if ( hit.entity->behavior == &actMonster )
									{
										disableAlertBlindStatus = true; // don't aggro target.
									}
								}
								break;
							case SLOP_BALL:
							{
								bool wasBlind = false;
								if ( hitstats )
								{
									wasBlind = hitstats->getEffectActive(EFF_BLIND) > 0;
								}
								if ( hit.entity->setEffect(EFF_BLIND, true, 5 * TICKS_PER_SECOND, false) )
								{
									if ( hit.entity->behavior == &actPlayer )
									{
										messagePlayerColor(hit.entity->skill[2], MESSAGE_STATUS, makeColorRGB(255, 0, 0), Language::get(6990));
									}
									if ( parent && parent->behavior == &actPlayer )
									{
										Uint32 color = makeColorRGB(0, 255, 0);
										messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(3878), Language::get(3879), MSG_COMBAT);
										if ( !wasBlind )
										{
											achievementObserver.addEntityAchievementTimer(parent, AchievementObserver::BARONY_ACH_FOOD_FIGHT, 5 * TICKS_PER_SECOND, false, 1);
											achievementObserver.awardAchievementIfActive(parent->skill[2], parent, AchievementObserver::BARONY_ACH_FOOD_FIGHT);
										}
									}
									if ( hit.entity->behavior == &actMonster )
									{
										disableAlertBlindStatus = true; // don't aggro target.
									}
								}
								break;
							}
							case BOLAS:
							{
								int duration = 3 * TICKS_PER_SECOND;
								if ( parent && parentStats )
								{
									duration += statGetPER(parentStats, parent) * 5;
									duration += statGetDEX(parentStats, parent) * 5;
									duration -= statGetSTR(hitstats, hit.entity) * 5;
									duration -= statGetDEX(hitstats, hit.entity) * 5;
									duration = std::max(1 * TICKS_PER_SECOND, duration);
									if ( parent->behavior == &actPlayer )
									{
										real_t charge = my->thrownProjectileCharge / 15.0; // 0-1
										duration *= (0.25 + 1.25 * charge); // 0.25-1.5
									}
								}
								if ( hit.entity->setEffect(EFF_ROOTED, true, duration, false) )
								{
									achievementObserver.addEntityAchievementTimer(hit.entity, AchievementObserver::BARONY_ACH_THATS_A_WRAP, duration, true, 0);

									if ( hit.entity->behavior == &actPlayer )
									{
										messagePlayerColor(hit.entity->skill[2], MESSAGE_STATUS, makeColorRGB(255, 0, 0), Language::get(6763));
									}
									if ( parent && parent->behavior == &actPlayer )
									{
										Uint32 color = makeColorRGB(0, 255, 0);
										messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(6764), Language::get(6765), MSG_COMBAT);
									}
									playSoundEntity(hit.entity, 763, 128);
								}
								else
								{
									if ( parent && parent->behavior == &actPlayer )
									{
										Uint32 color = makeColorRGB(255, 0, 0);
										messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(6766), Language::get(6767), MSG_COMBAT);
									}
								}
								break;
							}
							case TOOL_DUCK:
							{
								// set disoriented and start a cooldown on being distracted.
								if ( hit.entity->behavior == &actMonster && hitstats
									&& !hitstats->getEffectActive(EFF_DISORIENTED)
									&& !hitstats->getEffectActive(EFF_DISTRACTED_COOLDOWN) )
								{
									if ( hit.entity->monsterReleaseAttackTarget() )
									{
										hit.entity->monsterLookDir = hit.entity->yaw;
										hit.entity->monsterLookDir += (PI - PI / 4 + (local_rng.rand() % 10) * PI / 40);
										if ( hit.entity->monsterState == MONSTER_STATE_WAIT || hit.entity->monsterTarget == 0 )
										{
											// not attacking, duration longer.
											hit.entity->setEffect(EFF_DISORIENTED, true, TICKS_PER_SECOND * 2, false);
											hit.entity->setEffect(EFF_DISTRACTED_COOLDOWN, true, TICKS_PER_SECOND * 3, false);
										}
										else
										{
											hit.entity->setEffect(EFF_DISORIENTED, true, TICKS_PER_SECOND * 1, false);
											hit.entity->setEffect(EFF_DISTRACTED_COOLDOWN, true, TICKS_PER_SECOND * 3, false);
										}
									}
									spawnFloatingSpriteMisc(134, hit.entity->x + (-4 + local_rng.rand() % 9) + cos(hit.entity->yaw) * 2,
										hit.entity->y + (-4 + local_rng.rand() % 9) + sin(hit.entity->yaw) * 2, hit.entity->z + local_rng.rand() % 4);
								}
								usedpotion = true;
								break;
							}
							case FOOD_CREAMPIE:
							{
								skipMessage = true;
								playSoundEntity(hit.entity, 28, 64);
								Uint32 color = makeColorRGB(0, 255, 0);
								friendlyHit = false;
								if ( parent && parent->behavior == &actPlayer )
								{
									messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(3875), Language::get(3876), MSG_COMBAT);
								}
								if ( hit.entity->behavior == &actMonster )
								{
									if ( hit.entity->setEffect(EFF_BLIND, true, 250, false) )
									{
										if ( parent && parent->behavior == &actPlayer )
										{
											messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(3878), Language::get(3879), MSG_COMBAT);
										}
										disableAlertBlindStatus = true; // don't aggro target.
									}
								}
								else if ( hit.entity->behavior == &actPlayer )
								{
									Uint32 color = makeColorRGB(255, 0, 0);
									messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(3877));
									if ( hit.entity->setEffect(EFF_MESSY, true, 250, false) )
									{
										messagePlayer(hit.entity->skill[2], MESSAGE_STATUS, Language::get(910));
									}
									else
									{
										if ( hitstats->mask && hitstats->mask->type == MASK_HAZARD_GOGGLES )
										{
											if ( !(hit.entity->behavior == &actPlayer && hit.entity->effectShapeshift != NOTHING) )
											{
												messagePlayerColor(hit.entity->skill[2], MESSAGE_STATUS, makeColorRGB(0, 255, 0), Language::get(6088));
											}
										}
									}
								}
								for ( int i = 0; i < 5; ++i )
								{
									Entity* gib = spawnGib(hit.entity, 863);
									serverSpawnGibForClient(gib);
								}
								usedpotion = true;
								break;
							}
							case POTION_POLYMORPH:
							{
								Uint32 color = makeColorRGB(0, 255, 0);
								if ( hit.entity->behavior == &actMonster )
								{
									if ( parent && parent->behavior == &actPlayer )
									{
										if ( !strcmp(hitstats->name, "") )
										{
											messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(690), Language::get(690), MSG_COMBAT_BASIC);
										}
										else
										{
											messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(690), Language::get(694), MSG_COMBAT_BASIC);
										}
									}
								}
								else if ( hit.entity->behavior == &actPlayer )
								{
									Uint32 color = makeColorRGB(255, 0, 0);
									messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(588), itemname); // hit by a flying
								}
								Entity* newTarget = item_PotionPolymorph(item, hit.entity, parent);
								if ( newTarget )
								{
									polymorphedTarget = hit.entity;
									hit.entity = newTarget;
									hit.entity->setObituary(whatever);
									hitstats = newTarget->getStats();
				                    if (hitstats) {
				                        hitstats->killer = KilledBy::ITEM;
				                        hitstats->killer_item = itemType;
				                    }
								}
								skipMessage = true;
								usedpotion = true;
								break;
							}
							default:
								break;
						}
					}
					if ( THROWN_BOUNCES >= 3 && hitstats->HP <= 0 )
					{
						if ( parent && parent->checkEnemy(hit.entity) )
						{
							steamAchievementEntity(parent, "BARONY_ACH_SEE_THAT");
						}
					}
					else if ( cat == POTION && hitstats->HP <= 0 )
					{
						if ( parent && parent->behavior == &actPlayer )
						{
							if ( hitstats->type == LICH || hitstats->type == LICH_ICE || hitstats->type == LICH_FIRE )
							{
								if ( true/*client_classes[parent->skill[2]] == CLASS_BREWER*/ )
								{
									steamAchievementClient(parent->skill[2], "BARONY_ACH_SECRET_WEAPON");
								}
							}
							steamStatisticUpdateClient(parent->skill[2], STEAM_STAT_BOMBARDIER, STEAM_STAT_INT, 1);
						}
					}
					if ( itemType == BOOMERANG )
					{
						if ( parent && parent->behavior == &actPlayer && hit.entity->behavior == &actMonster )
						{
							achievementObserver.addEntityAchievementTimer(parent, AchievementObserver::BARONY_ACH_IF_YOU_LOVE_SOMETHING, 6 * TICKS_PER_SECOND, true, 0);
						}
					}
				}

				// update enemy bar for attacker
				if ( !friendlyHit || healingPotion )
				{
					if ( !strcmp(hitstats->name, "") )
					{
						updateEnemyBar(parent, hit.entity, getMonsterLocalizedName(hitstats->type).c_str(), hitstats->HP, hitstats->MAXHP,
							false, DamageGib::DMG_DEFAULT);
					}
					else
					{
						updateEnemyBar(parent, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP,
							false, DamageGib::DMG_DEFAULT);
					}
				}

				if ( friendlyHit && !usedpotion )
				{
					if ( item && itemCategory(item) != POTION && item->type != BOOMERANG
						&& item->type != GREASE_BALL && item->type != DUST_BALL 
						&& item->type != SLOP_BALL )
					{
						Entity* entity = newEntity(-1, 1, map.entities, nullptr); //Item entity.
						entity->flags[INVISIBLE] = true;
						entity->flags[UPDATENEEDED] = true;
						entity->flags[PASSABLE] = true;
						entity->x = ox;
						entity->y = oy;
						entity->z = oz;
						entity->sizex = my->sizex;
						entity->sizey = my->sizey;
						entity->yaw = my->yaw;
						entity->pitch = my->pitch;
						entity->roll = my->roll;
						entity->vel_x = THROWN_VELX / 2;
						entity->vel_y = THROWN_VELY / 2;
						entity->vel_z = my->vel_z;
						entity->behavior = &actItem;
						entity->skill[10] = item->type;
						entity->skill[11] = item->status;
						entity->skill[12] = item->beatitude;
						entity->skill[13] = item->count;
						entity->skill[14] = item->appearance;
						entity->skill[15] = item->identified;
						if ( item->type == GEM_JEWEL )
						{
							entity->parent = my->parent;
						}
						if ( itemCategory(item) == THROWN )
						{
							//Hack to make monsters stop catching your shurikens and chakrams.
							entity->parent = my->parent;
							if ( parent )
							{
								if ( Stat* parentStats = parent->getStats() )
								{
									if ( parentStats->getEffectActive(EFF_RETURN_ITEM) )
									{
										entity->itemReturnUID = parent->getUID();
									}
								}
							}
						}
					}
					free(item);
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}

				if ( damage > 0 )
				{
					Entity* gib = spawnGib(hit.entity);
					serverSpawnGibForClient(gib);
					playSoundEntity(hit.entity, 28, 64);
					if ( hit.entity->behavior == &actPlayer )
					{
						if ( players[hit.entity->skill[2]]->isLocalPlayer() )
						{
							cameravars[hit.entity->skill[2]].shakex += .1;
							cameravars[hit.entity->skill[2]].shakey += 10;
						}
						else if ( hit.entity->skill[2] > 0 && !players[hit.entity->skill[2]]->isLocalPlayer() )
						{
							strcpy((char*)net_packet->data, "SHAK");
							net_packet->data[4] = 10; // turns into .1
							net_packet->data[5] = 10;
							net_packet->address.host = net_clients[hit.entity->skill[2] - 1].host;
							net_packet->address.port = net_clients[hit.entity->skill[2] - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, hit.entity->skill[2] - 1);
						}
					}

					bool doSkillIncrease = true;
					if ( monsterIsImmobileTurret(hit.entity, hitstats) )
					{
						if ( hitstats->type == DUMMYBOT && hitstats->HP > 0 )
						{
							doSkillIncrease = true; // can train on dummybots.
						}
						else
						{
							doSkillIncrease = false; // no skill for killing/hurting other turrets.
						}
					}
					else if ( hit.entity->behavior == &actMonster
						&& (hit.entity->monsterAllyGetPlayerLeader() || (hitstats && achievementObserver.checkUidIsFromPlayer(hitstats->leader_uid) >= 0))
						&& parent && parent->behavior == &actPlayer )
					{
						doSkillIncrease = false; // no level up on allies
					}
					if ( hit.entity->behavior == &actPlayer && parent && parent->behavior == &actPlayer )
					{
						doSkillIncrease = false; // no skill for killing/hurting players
					}

					if ( doSkillIncrease && parent && parent->behavior == &actPlayer )
					{
						if ( parent->isInvisible() && parent->checkEnemy(hit.entity) )
						{
							players[parent->skill[2]]->mechanics.updateSustainedSpellEvent(SPELL_INVISIBILITY, 10.0, 1.0, hit.entity);
						}
					}

					int chance = 5;
					if ( doSkillIncrease && (local_rng.rand() % chance == 0) && parent && parent->getStats() )
					{
						if ( hitstats->type != DUMMYBOT 
							|| (hitstats->type == DUMMYBOT && parent->getStats()->getProficiency(PRO_RANGED) < SKILL_LEVEL_BASIC) )
						{
							parent->increaseSkill(PRO_RANGED);
						}
					}
				}
				else
				{
					if ( cat == THROWN 
						&& !(item && (item->type == GREASE_BALL 
							|| item->type == DUST_BALL
							|| item->type == SLOP_BALL)) )
					{
						playSoundEntity(hit.entity, 66, 64); //*tink*
					}
					if ( hit.entity->behavior == &actPlayer )
					{
						if ( players[hit.entity->skill[2]]->isLocalPlayer() )
						{
							cameravars[hit.entity->skill[2]].shakex += .05;
							cameravars[hit.entity->skill[2]].shakey += 5;
						}
						else if ( hit.entity->skill[2] > 0 && !players[hit.entity->skill[2]]->isLocalPlayer() )
						{
							strcpy((char*)net_packet->data, "SHAK");
							net_packet->data[4] = 5; // turns into .05
							net_packet->data[5] = 5;
							net_packet->address.host = net_clients[hit.entity->skill[2] - 1].host;
							net_packet->address.port = net_clients[hit.entity->skill[2] - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, hit.entity->skill[2] - 1);
						}
					}
				}

				if ( hitstats->HP <= 0 && parent )
				{
					parent->awardXP(hit.entity, true, true);
					spawnBloodVialOnMonsterDeath(hit.entity, hitstats, parent);

					if ( parent->behavior == &actPlayer )
					{
						if ( cat == THROWN )
						{
							Compendium_t::Events_t::eventUpdateCodex(parent->skill[2], Compendium_t::CPDM_THROWN_KILLS, "thrown", 1);
						}
						if ( cat == GEM )
						{
							if ( itemType == GEM_ROCK || itemType == GEM_LUCK )
							{
								Compendium_t::Events_t::eventUpdateWorld(parent->skill[2], Compendium_t::CPDM_COMBAT_MASONRY_ROCKS, "masons guild", 1);
							}
							else
							{
								Compendium_t::Events_t::eventUpdateWorld(parent->skill[2], Compendium_t::CPDM_COMBAT_MASONRY_GEMS, "masons guild", 1);
							}
						}
					}
				}

				bool doAlert = true;
				// fix for confuse potion aggro'ing monsters on impact.
				if ( !wasConfused && hitstats && hitstats->getEffectActive(EFF_CONFUSED) && hit.entity->behavior == &actMonster && parent )
				{
					doAlert = false;
					if ( hit.entity->monsterTarget == parent->getUID() || prevTarget == parent->getUID() )
					{
						hit.entity->monsterReleaseAttackTarget();
					}
				}

				// alert the monster
				if ( hit.entity->behavior == &actMonster && hitstats && parent != nullptr && doAlert )
				{
					bool alertTarget = hit.entity->monsterAlertBeforeHit(parent);
					bool targetHealed = false;

					if ( disableAlertBlindStatus )
					{
						alertTarget = false;
						if ( hitstats->getEffectActive(EFF_BLIND) || hitstats->getEffectActive(EFF_DUSTED) )
						{
							hit.entity->monsterReleaseAttackTarget();
						}
					}

					if ( alertTarget && hit.entity->monsterState != MONSTER_STATE_ATTACK && (hitstats->type < LICH || hitstats->type >= SHOPKEEPER) )
					{
						if ( polymorphedTarget && hitstats->leader_uid == parent->getUID() )
						{
							// don't aggro your leader if they hit you with polymorph
						}
						else if ( (hitstats->leader_uid == parent->getUID() || hit.entity->checkFriend(parent))
							&& (hit.entity->getHP() - oldHP) >= 0 )
						{
							// don't aggro your leader or allies if they healed you
							targetHealed = true;
						}
						else
						{
							hit.entity->monsterAcquireAttackTarget(*parent, MONSTER_STATE_PATH, true);
						}
					}

					bool alertAllies = true;
					if ( parent->behavior == &actPlayer || parent->monsterAllyIndex != -1 )
					{
						if ( hit.entity->behavior == &actPlayer || (hit.entity->behavior == &actMonster && hit.entity->monsterAllyIndex != -1) )
						{
							// if a player ally + hit another ally or player, don't alert other allies.
							alertAllies = false;
						}
					}

					// alert other monsters too
					if ( alertAllies && !targetHealed )
					{
						std::unordered_set<Entity*> entitiesToSkip = { polymorphedTarget };
						hit.entity->alertAlliesOnBeingHit(parent, &entitiesToSkip);
					}
					hit.entity->updateEntityOnHit(parent, alertTarget);
					Uint32 color = makeColorRGB(0, 255, 0);
					if ( parent->behavior == &actPlayer && !skipMessage )
					{
						if ( !strcmp(hitstats->name, "") )
						{
							if ( hitstats->HP <= 0 )
							{
								// HP <= 0
								if ( parent && parent->behavior == &actPlayer )
								{
									messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(692), Language::get(697), MSG_COMBAT);
								}
							}
							else
							{
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(690), Language::get(690), MSG_COMBAT_BASIC);
								if ( damage == 0 )
								{
									messagePlayer(parent->skill[2], MESSAGE_COMBAT_BASIC, Language::get(447));
								}
							}
						}
						else
						{
							if ( hitstats->HP <= 0 )
							{
								// HP <= 0
								if ( parent && parent->behavior == &actPlayer )
								{
									messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(692), Language::get(697), MSG_COMBAT);
								}
							}
							else
							{
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(690), Language::get(694), MSG_COMBAT_BASIC);
								if ( damage == 0 )
								{
									if ( hitstats->sex )
									{
										messagePlayer(parent->skill[2], MESSAGE_COMBAT_BASIC, Language::get(449));
									}
									else
									{
										messagePlayer(parent->skill[2], MESSAGE_COMBAT_BASIC, Language::get(450));
									}
								}
							}
						}
					}
				}
				if ( hit.entity->behavior == &actPlayer && !skipMessage )
				{
					Uint32 color = makeColorRGB(255, 0, 0);
					messagePlayerColor(hit.entity->skill[2], MESSAGE_COMBAT, color, Language::get(588), itemname); // hit by a flying
					if ( damage == 0 && !wasPotion )
					{
						messagePlayer(hit.entity->skill[2], MESSAGE_COMBAT, Language::get(452));
					}
					if ( goggleProtection )
					{
						messagePlayerColor(hit.entity->skill[2], MESSAGE_STATUS, makeColorRGB(0, 255, 0), Language::get(6088));
					}
				}
				if ( parent && parent->behavior == &actPlayer && envenomWeapon && hitstats && hitstats->HP > 0 )
				{
					Uint32 color = makeColorRGB(0, 255, 0);
					messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, Language::get(6533), Language::get(6534), MSG_COMBAT);
				}
			}
			else
			{
				switch ( item->type )
				{
					case POTION_FIRESTORM:
						spawnMagicTower(parent, my->x, my->y, SPELL_FIREBALL, hit.entity);
						if ( hit.entity->behavior == &actBoulder )
						{
							if ( hit.entity->sprite == 989 || hit.entity->sprite == 990 )
							{
								magicDig(parent, my, 0, 1);
							}
							else
							{
								magicDig(parent, my, 2, 4);
							}
							hit.entity = nullptr;
						}
						break;
					case POTION_ICESTORM:
						spawnMagicTower(parent, my->x, my->y, SPELL_COLD, hit.entity);
						break;
					case POTION_THUNDERSTORM:
						spawnMagicTower(parent, my->x, my->y, SPELL_LIGHTNING, hit.entity);
						break;
					default:
						if ( hit.entity->behavior == &actChest || hit.entity->isInertMimic() )
						{
							if ( cat == THROWN )
							{
								playSoundEntity(hit.entity, 66, 64); //*tink*
							}

							if ( parent )
							{
								if ( parent->behavior == &actPlayer )
								{
									messagePlayer(parent->skill[2], MESSAGE_COMBAT_BASIC, Language::get(667));
									if ( cat == THROWN )
									{
										messagePlayer(parent->skill[2], MESSAGE_COMBAT_BASIC, Language::get(447));
									}
								}
								if ( hit.entity->behavior == &actMonster )
								{
									if ( hitstats )
									{
										updateEnemyBar(parent, hit.entity, Language::get(675), hitstats->HP, hitstats->MAXHP,
											false, DamageGib::DMG_WEAKEST);
									}
								}
								else
								{
									updateEnemyBar(parent, hit.entity, Language::get(675), hit.entity->chestHealth, hit.entity->chestMaxHealth,
										false, DamageGib::DMG_WEAKEST);
								}
							}
						}
						else if ( hit.entity->isDamageableCollider() )
						{
							int damage = 1;
							int axe = 0;
							if ( hit.entity->isColliderResistToSkill(PRO_RANGED) || hit.entity->isColliderWall() )
							{
								damage = 1;
							}
							else
							{
								damage = 2 + local_rng.rand() % 3;
							}
							if ( hit.entity->isColliderWeakToSkill(PRO_RANGED) && cat == THROWN )
							{
								if ( parent && parent->getStats() )
								{
									axe = 2 * (parent->getStats()->getModifiedProficiency(PRO_RANGED) / 20);
								}
								axe = std::min(axe, 9);
							}
							damage += axe;
							if ( my->thrownProjectileCharge >= 1 )
							{
								damage += my->thrownProjectileCharge / 5;
							}

							int& entityHP = hit.entity->colliderCurrentHP;
							int oldHP = entityHP;
							entityHP -= damage;

							int sound = 28; //damage.ogg
							if ( hit.entity->getColliderSfxOnHit() > 0 )
							{
								sound = hit.entity->getColliderSfxOnHit();
							}
							playSoundEntity(hit.entity, sound, 64);

							if ( entityHP > 0 )
							{
								if ( parent && parent->behavior == &actPlayer )
								{
									messagePlayer(parent->skill[2], MESSAGE_COMBAT_BASIC, Language::get(hit.entity->getColliderOnHitLangEntry()),
										Language::get(hit.entity->getColliderLangName()));
								}
							}
							else
							{
								entityHP = 0;

								hit.entity->colliderKillerUid = parent ? parent->getUID() : 0;
								if ( parent && parent->behavior == &actPlayer )
								{
									if ( hit.entity->getColliderOnBreakLangEntry() != 0 )
									{
										messagePlayer(parent->skill[2], MESSAGE_COMBAT, Language::get(hit.entity->getColliderOnBreakLangEntry()),
											Language::get(hit.entity->getColliderLangName()));
									}
									if ( hit.entity->isColliderWall() )
									{
										Compendium_t::Events_t::eventUpdateWorld(parent->skill[2], Compendium_t::CPDM_BARRIER_DESTROYED, "breakable barriers", 1);
									}
								}
							}

							if ( parent )
							{
								updateEnemyBar(parent, hit.entity, Language::get(hit.entity->getColliderLangName()), entityHP, hit.entity->colliderMaxHP, false,
									DamageGib::DMG_DEFAULT);
							}
						}
						break;
				}
			}
		}
		else
		{
			//!hit.entity
			switch ( item->type )
			{
				case POTION_FIRESTORM:
					if ( hit.mapx >= 1 && hit.mapx < map.width - 1 && hit.mapy >= 1 && hit.mapy < map.height - 1 )
					{
						magicDig(parent, my, 2, 4);
					}
					spawnMagicTower(parent, my->x, my->y, SPELL_FIREBALL, nullptr);
					break;
				case POTION_ICESTORM:
					spawnMagicTower(parent, my->x, my->y, SPELL_COLD, nullptr);
					break;
				case POTION_THUNDERSTORM:
					spawnMagicTower(parent, my->x, my->y, SPELL_LIGHTNING, nullptr);
					break;
				case TOOL_BOMB:
				case TOOL_SLEEP_BOMB:
				case TOOL_TELEPORT_BOMB:
				case TOOL_FREEZE_BOMB:
					if ( hit.side == 0 )
					{
						// pick a random side to be on.
						if ( local_rng.rand() % 2 == 0 )
						{
							hit.side = HORIZONTAL;
						}
						else
						{
							hit.side = VERTICAL;
						}
					}

					if ( hit.side == HORIZONTAL )
					{
						if ( THROWN_VELX > 0 )
						{
							item->applyBomb(parent, item->type, Item::ItemBombPlacement::BOMB_WALL, Item::ItemBombFacingDirection::BOMB_WEST, my, nullptr);
						}
						else
						{
							item->applyBomb(parent, item->type, Item::ItemBombPlacement::BOMB_WALL, Item::ItemBombFacingDirection::BOMB_EAST, my, nullptr);
						}
					}
					else if ( hit.side == VERTICAL )
					{
						if ( THROWN_VELY > 0 )
						{
							item->applyBomb(parent, item->type, Item::ItemBombPlacement::BOMB_WALL, Item::ItemBombFacingDirection::BOMB_NORTH, my, nullptr);
						}
						else
						{
							item->applyBomb(parent, item->type, Item::ItemBombPlacement::BOMB_WALL, Item::ItemBombFacingDirection::BOMB_SOUTH, my, nullptr);
						}
					}
					free(item);
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
					break;
				default:
					break;
			}
		}
		if ( cat == POTION )
		{
			// potions shatter on impact
			playSoundEntity(my, 162, 64);
			if ( !usedpotion )
			{
				free(item);
			}
			onThrownLandingParticle(my);
			my->removeLightField();
			list_RemoveNode(my->mynode);
			return;
		}
		else if ( item && item->type == FOOD_CREAMPIE )
		{
			if ( !usedpotion )
			{
				for ( int i = 0; i < 5; ++i )
				{
					Entity* gib = spawnGib(my, 863);
					serverSpawnGibForClient(gib);
				}
			}
			free(item);
			item = nullptr;
			my->removeLightField();
			list_RemoveNode(my->mynode);
			return;
		}
		else if ( item && (item->type == DUST_BALL 
			|| item->type == GREASE_BALL
			|| item->type == SLOP_BALL) )
		{
			free(item);
			item = nullptr;
			onThrownLandingParticle(my);
			playSoundEntity(my, 764, 64);
			if ( hit.entity &&
				(hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer) )
			{
				// become passable, go through creatures
				//my->flags[NOCLIP_CREATURES] = true;
				my->collisionIgnoreTargets.insert(hit.entity->getUID());
			}
			else
			{
				my->removeLightField();
				list_RemoveNode(my->mynode);
				return;
			}
		}
		else if ( itemCategory(item) == THROWN && (item->type == STEEL_CHAKRAM 
			|| item->type == CRYSTAL_SHURIKEN || (item->type == BOOMERANG && uidToEntity(my->parent))) 
				&& hit.entity == NULL )
		{
			// chakram, shurikens bounce off walls until entity or floor is hit.
			playSoundEntity(my, 66, 64);
			if ( item->type == BOOMERANG )
			{
				// boomerang always tink and return to owner.
				free(item);
				my->removeLightField();
				list_RemoveNode(my->mynode);
				return;
			}
		}
		else if ( item->type == BOOMERANG && hit.entity && uidToEntity(my->parent) )
		{
			// boomerang always return to owner.
			free(item);
			my->removeLightField();
			list_RemoveNode(my->mynode);
			return;
		}
		else if ( item && itemIsThrowableTinkerTool(item) /*&& !(item->type >= TOOL_BOMB && item->type <= TOOL_TELEPORT_BOMB)*/ )
		{
			// non-bomb tools will fall to the ground and get placed.
		}
		else if ( item && item->type == GEM_JEWEL )
		{
			// will fall to the ground and get placed.
		}
		else if ( item && item->type == TOOL_DUCK )
		{
			item->applyDuck(my->parent, my->x, my->y, hit.entity, false);
			free(item);
			my->removeLightField();
			list_RemoveNode(my->mynode);
			return;
		}
		else
		{
			bool dropItem = true;
			if ( itemCategory(item) == GEM )
			{
				if ( (hit.entity && local_rng.rand() % 2 == 0) || item->beatitude < 0 || local_rng.rand() % 5 == 0 )
				{
					dropItem = false;
					if ( hit.entity )
					{
						createParticleShatteredGem(hit.entity->x, hit.entity->y, 7.5, my->sprite, hit.entity);
						serverSpawnMiscParticles(hit.entity, PARTICLE_EFFECT_SHATTERED_GEM, my->sprite);
					}
					else
					{
						createParticleShatteredGem(my->x, my->y, my->z, my->sprite, nullptr);
						serverSpawnMiscParticlesAtLocation(my->x, my->y, my->z, PARTICLE_EFFECT_SHATTERED_GEM, my->sprite);
					}
				}
			}
			else if ( item && item->type == BOLAS )
			{
				if ( hit.entity && !hit.entity->isInertMimic() && hit.entity->getStats() )
				{
					dropItem = false;
					int duration = 3 * TICKS_PER_SECOND;
					if ( hit.entity->getStats()->getEffectActive(EFF_ROOTED) )
					{
						duration = hit.entity->getStats()->EFFECTS_TIMERS[EFF_ROOTED];
					}
					item->ownerUid = parent ? parent->getUID() : 0;
					createParticleBolas(hit.entity, 1917, duration, item);
					serverSpawnMiscParticles(hit.entity, PARTICLE_EFFECT_BOLAS, 1917, 0, duration, 0);
				}
			}
			if ( dropItem )
			{
				Entity* entity = newEntity(-1, 1, map.entities, nullptr); //Item entity.
				entity->flags[INVISIBLE] = true;
				entity->flags[UPDATENEEDED] = true;
				entity->flags[PASSABLE] = true;
				entity->x = ox;
				entity->y = oy;
				entity->z = oz;
				entity->sizex = my->sizex;
				entity->sizey = my->sizey;
				entity->yaw = my->yaw;
				entity->pitch = my->pitch;
				entity->roll = my->roll;
				entity->vel_x = THROWN_VELX / 2;
				entity->vel_y = THROWN_VELY / 2;
				entity->vel_z = my->vel_z;
				entity->behavior = &actItem;
				entity->skill[10] = item->type;
				entity->skill[11] = item->status;
				entity->skill[12] = item->beatitude;
				entity->skill[13] = item->count;
				entity->skill[14] = item->appearance;
				entity->skill[15] = item->identified;
				if ( item->type == GEM_JEWEL )
				{
					entity->parent = my->parent;
				}
				if ( itemCategory(item) == THROWN )
				{
					//Hack to make monsters stop catching your shurikens and chakrams.
					entity->parent = my->parent;
					if ( parent )
					{
						if ( Stat* parentStats = parent->getStats() )
						{
							if ( parentStats->getEffectActive(EFF_RETURN_ITEM) )
							{
								entity->itemReturnUID = parent->getUID();
							}
						}
					}
				}
			}
			free(item);
			my->removeLightField();
			list_RemoveNode(my->mynode);
			return;
		}

		if ( item )
		{
			free(item);
		}
	}

	if ( cat == THROWN )
	{
		if ( my->sprite == BOOMERANG_PARTICLE )
		{
			if ( (THROWN_VELX * THROWN_VELX + THROWN_VELY * THROWN_VELY) > 2.0 )
			{
				THROWN_VELX = THROWN_VELX * .99;
				THROWN_VELY = THROWN_VELY * .99;
			}
		}
		else
		{
			THROWN_VELX = THROWN_VELX * .99;
			THROWN_VELY = THROWN_VELY * .99;
		}
		//my->pitch += result * .01;
		//messagePlayer(0, "%.4f, %.4f", THROWN_VELX, THROWN_VELY);
		/*if ( my->sprite == BOOMERANG_PARTICLE )
		{
			THROWN_VELX = std::max(0.05, THROWN_VELX);
			THROWN_VELY = std::max(0.05, THROWN_VELY);
		}*/
	}
	else
	{
		THROWN_VELX = THROWN_VELX * .99;
		THROWN_VELY = THROWN_VELY * .99;
		if ( my->sprite == 897 ) // sentrybot head item
		{
			my->pitch = 0.0;
		}
		else
		{
			my->pitch += result * .01;
		}
	}
}

void thrownItemUpdateSpellTrail(Entity& my, real_t _x, real_t _y)
{
	if ( my.sprite == items[DUST_BALL].index )
	{
		auto findEffects = particleTimerEffects.find(my.thrownProjectileParticleTimerUID);
		if ( findEffects != particleTimerEffects.end() )
		{
			if ( auto spellTimer = uidToEntity(my.thrownProjectileParticleTimerUID) )
			{
				int x = static_cast<int>(_x) / 16;
				int y = static_cast<int>(_y) / 16;
				bool freeSpot = true;
				Uint32 lastTick = 1;
				for ( auto& eff : findEffects->second.effectMap )
				{
					if ( static_cast<int>(eff.second.x) / 16 == x
						&& static_cast<int>(eff.second.y) / 16 == y )
					{
						freeSpot = false;
					}
					lastTick = std::max(eff.first, lastTick);
				}
				if ( freeSpot )
				{
					auto& effect = findEffects->second.effectMap[std::max(spellTimer->ticks + 1, lastTick + 2)]; // insert x ticks beyond last effect
					if ( findEffects->second.effectMap.size() == 1 )
					{
						effect.firstEffect = true;
					}
					int spellID = spellTimer->particleTimerVariable2;
					auto particleEffectType = (spellID == SPELL_MYCELIUM_BOMB || spellID == SPELL_MYCELIUM_SPORES)
						? ParticleTimerEffect_t::EffectType::EFFECT_MYCELIUM
						: ParticleTimerEffect_t::EffectType::EFFECT_SPORES;
					effect.effectType = particleEffectType;
					effect.x = x * 16.0 + 8.0;
					effect.y = y * 16.0 + 8.0;
					effect.yaw = 0.0;
				}
			}
		}
	}
}