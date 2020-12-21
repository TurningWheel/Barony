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
#include "sound.hpp"
#include "monster.hpp"
#include "interface/interface.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "scores.hpp"
#include "player.hpp"
#include "magic/magic.hpp"
#include "paths.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define THROWN_VELX my->vel_x
#define THROWN_VELY my->vel_y
#define THROWN_VELZ my->vel_z
#define THROWN_TYPE (Item)my->skill[10]
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

void actThrown(Entity* my)
{
	Item* item = nullptr;
	Category cat = GEM;
	ItemType type = WOODEN_SHIELD;
	char* itemname = nullptr;

	item = newItemFromEntity(my);
	if ( item )
	{
		cat = itemCategory(item);
		type = item->type;
		free(item);
	}

	if ( multiplayer == CLIENT )
	{
		if ( THROWN_LIFE == 0 )
		{
			Entity* tempEntity = uidToEntity(clientplayer);
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
				playSoundEntityLocal(my, 434 + rand() % 10, 64);
			}
		}
	}
	else
	{
		// select appropriate model
		my->skill[2] = -8;
		my->flags[INVISIBLE] = false;
		item = newItemFromEntity(my);
		if ( item )
		{
			my->sprite = itemModel(item);
			if ( item->type == BOOMERANG )
			{
				my->sprite = BOOMERANG_PARTICLE;
				if ( my->ticks > 0 && my->ticks % 7 == 0 )
				{
					playSoundEntityLocal(my, 434 + rand() % 10, 64);
				}
			}
			free(item);
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
			if ( type == BRONZE_TOMAHAWK || type == IRON_DAGGER )
			{
				// axe and dagger spin vertically
				my->pitch += 0.2;
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
	}
	else
	{
		if ( my->x >= 0 && my->y >= 0 && my->x < map.width << 4 && my->y < map.height << 4 )
		{
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
					list_RemoveNode(my->mynode);
					return;
				}
				else if ( specialMonster )
				{
					free(item);
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
							messagePlayer(parent->skill[2], language[3900]);
						}
						else if ( item->isTinkeringItemWithThrownLimit() && !tinkeringItemCanBePlaced )
						{
							if ( stats[parent->skill[2]]->PROFICIENCIES[PRO_LOCKPICKING] >= SKILL_LEVEL_LEGENDARY )
							{
								messagePlayer(parent->skill[2], language[3884]);
							}
							else
							{
								messagePlayer(parent->skill[2], language[3883]);
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
					if ( itemCategory(item) == THROWN )
					{
						//Hack to make monsters stop catching your shurikens and chakrams.
						entity->parent = my->parent;
					}
					free(item);
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
			if ( (i == 0 && selectedEntity[0] == my) || (client_selected[i] == my) || (splitscreen && selectedEntity[i] == my) )
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
							messagePlayer(i, language[504], item->description());
							item->count = oldcount;
							if ( i != 0 && !players[i]->isLocalPlayer() )
							{
								free(item);
							}
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
	double result = clipMove(&my->x, &my->y, THROWN_VELX, THROWN_VELY, my);
	if ( processXYCollision && result != sqrt(THROWN_VELX * THROWN_VELX + THROWN_VELY * THROWN_VELY) )
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
		}
		bool tryHitEntity = true;
		if ( itemIsThrowableTinkerTool(item) && !(item->type >= TOOL_BOMB && item->type <= TOOL_TELEPORT_BOMB) )
		{
			tryHitEntity = false;
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
				if ( parent && parent->checkFriend(hit.entity) )
				{
					friendlyHit = true;
				}
			}
			if ( item->type >= TOOL_BOMB && item->type <= TOOL_TELEPORT_BOMB )
			{
				if ( hit.entity->behavior == &actChest )
				{
					item->applyBomb(parent, item->type, Item::ItemBombPlacement::BOMB_CHEST, Item::ItemBombFacingDirection::BOMB_UP, my, hit.entity);
					free(item);
					list_RemoveNode(my->mynode);
					return;
				}
				else if ( hit.entity->behavior == &actDoor )
				{
					item->applyBomb(parent, item->type, Item::ItemBombPlacement::BOMB_DOOR, Item::ItemBombFacingDirection::BOMB_UP, my, hit.entity);
					free(item);
					list_RemoveNode(my->mynode);
					return;
				}
			}
			else if ( hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer )
			{
				int oldHP = 0;
				oldHP = hit.entity->getHP();
				int damage = (BASE_THROWN_DAMAGE + item->beatitude);
				if ( parentStats )
				{
					if ( itemCategory(item) == POTION )
					{
						int skillLVL = parentStats->PROFICIENCIES[PRO_ALCHEMY] / 20;
						//int dex = parent->getDEX() / 4;
						//damage += dex;
						damage = damage * potionDamageSkillMultipliers[std::min(skillLVL, 5)];
						damage -= rand() % ((damage / 4) + 1);
					}
					else
					{
						if ( itemCategory(item) == THROWN )
						{
							damage = my->thrownProjectilePower;
							if ( my->thrownProjectileCharge >= 1 )
							{
								damage += my->thrownProjectileCharge / 5; //0-3 base +damage
								real_t bypassArmor = 1 - my->thrownProjectileCharge * 0.05; //100-35% of armor taken into account
								if ( item->type == BOOMERANG )
								{
									//damage *= damagetables[hitstats->type][4]; // ranged damage tables.
								}
								damage -= (AC(hit.entity->getStats()) * bypassArmor);
							}
							else
							{
								if ( item->type == BOOMERANG )
								{
									//damage *= damagetables[hitstats->type][4]; // ranged damage tables.
								}
								damage -= (AC(hit.entity->getStats()) * .5);
							}
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
					default:
						break;
				}
				damage = std::max(0, damage);
				//messagePlayer(0, "damage: %d", damage);
				if ( parent && parent->behavior == &actPlayer && parent->checkFriend(hit.entity) && itemCategory(item) == POTION )
				{
					switch ( item->type )
					{
						case POTION_HEALING:
						case POTION_EXTRAHEALING:
						case POTION_RESTOREMAGIC:
						case POTION_CUREAILMENT:
						case POTION_WATER:
						case POTION_BOOZE:
						case POTION_JUICE:
						case POTION_STRENGTH:
						case POTION_SPEED:
							damage = 0;
							break;
						default:
							break;
					}
					damage = std::min(10, damage); // impact damage is 10 max on allies.
				}

				char whatever[256] = "";
				if ( !friendlyHit )
				{
					hit.entity->modHP(-damage);
				}
				// set the obituary
				snprintf(whatever, 255, language[1508], itemname);
				hit.entity->setObituary(whatever);
				bool skipMessage = false;
				Entity* polymorphedTarget = nullptr;
				bool disableAlertBlindStatus = false;
				bool ignorePotion = false;
				bool wasPotion = itemCategory(item) == POTION;
				bool wasBoomerang = item->type == BOOMERANG;
				bool wasConfused = (hitstats && hitstats->EFFECTS[EFF_CONFUSED]);

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
								ignorePotion = false;
								break;
							case POTION_EXTRAHEALING:
							case POTION_HEALING:
							case POTION_WATER:
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
								ignorePotion = true;
								break;
							default:
								break;
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
					}
					if ( !ignorePotion )   // this makes it impossible to bork the end boss :)
					{
						if ( rand() % 4 == 0 && parent != NULL && itemCategory(item) == POTION && item->type != POTION_EMPTY )
						{
							parent->increaseSkill(PRO_ALCHEMY);
						}
						switch ( item->type )
						{
							case POTION_WATER:
								usedpotion = true;
								item_PotionWater(item, hit.entity, parent);
								break;
							case POTION_BOOZE:
								item_PotionBooze(item, hit.entity, parent);
								if ( parentStats && parentStats->EFFECTS[EFF_DRUNK] )
								{
									steamAchievementEntity(parent, "BARONY_ACH_CHEERS");
									if ( hit.entity->behavior == &actMonster && parent && parent->behavior == &actPlayer )
									{
										if ( parentStats->type == GOATMAN
											&& (hitstats->type == HUMAN || hitstats->type == GOBLIN)
											&& hitstats->leader_uid == 0 )
										{
											if ( forceFollower(*parent, *hit.entity) )
											{
												spawnMagicEffectParticles(hit.entity->x, hit.entity->y, hit.entity->z, 685);
												parent->increaseSkill(PRO_LEADERSHIP);
												messagePlayerMonsterEvent(parent->skill[2], SDL_MapRGB(mainsurface->format, 0, 255, 0), 
													*hitstats, language[3252], language[3251], MSG_COMBAT);
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
								usedpotion = true;
								break;
							case POTION_JUICE:
								item_PotionJuice(item, hit.entity, parent);
								usedpotion = true;
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
							case FOOD_CREAMPIE:
							{
								skipMessage = true;
								playSoundEntity(hit.entity, 28, 64);
								Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								friendlyHit = false;
								if ( parent && parent->behavior == &actPlayer )
								{
									messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[3875], language[3876], MSG_COMBAT);
								}
								if ( hit.entity->behavior == &actMonster )
								{
									if ( hit.entity->setEffect(EFF_BLIND, true, 250, false) )
									{
										if ( parent && parent->behavior == &actPlayer )
										{
											messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[3878], language[3879], MSG_COMBAT);
										}
									}
									disableAlertBlindStatus = true; // don't aggro target.
								}
								else if ( hit.entity->behavior == &actPlayer )
								{
									hit.entity->setEffect(EFF_MESSY, true, 250, false);
									serverUpdateEffects(hit.entity->skill[2]);
									Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
									messagePlayerColor(hit.entity->skill[2], color, language[3877]);
									messagePlayer(hit.entity->skill[2], language[910]);
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
								Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								if ( hit.entity->behavior == &actMonster )
								{
									if ( parent && parent->behavior == &actPlayer )
									{
										if ( !strcmp(hitstats->name, "") )
										{
											messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[690], language[690], MSG_COMBAT);
										}
										else
										{
											messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[690], language[694], MSG_COMBAT);
										}
									}
								}
								else if ( hit.entity->behavior == &actPlayer )
								{
									Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
									messagePlayerColor(hit.entity->skill[2], color, language[588], itemname);
								}
								Entity* newTarget = item_PotionPolymorph(item, hit.entity, parent);
								if ( newTarget )
								{
									polymorphedTarget = hit.entity;
									hit.entity = newTarget;
									hitstats = newTarget->getStats();
									hit.entity->setObituary(whatever);
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
								steamAchievementClient(parent->skill[2], "BARONY_ACH_SECRET_WEAPON");
							}
							steamStatisticUpdateClient(parent->skill[2], STEAM_STAT_BOMBARDIER, STEAM_STAT_INT, 1);
						}
					}
					if ( wasBoomerang )
					{
						if ( parent && parent->behavior == &actPlayer && hit.entity->behavior == &actMonster )
						{
							achievementObserver.addEntityAchievementTimer(parent, AchievementObserver::BARONY_ACH_IF_YOU_LOVE_SOMETHING, 6 * TICKS_PER_SECOND, true, 0);
						}
					}
				}

				// update enemy bar for attacker
				if ( !friendlyHit )
				{
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

				if ( friendlyHit && !usedpotion )
				{
					if ( item && itemCategory(item) != POTION && item->type != BOOMERANG )
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
						if ( itemCategory(item) == THROWN )
						{
							//Hack to make monsters stop catching your shurikens and chakrams.
							entity->parent = my->parent;
						}
					}
					free(item);
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
						else if ( hit.entity->skill[2] > 0 )
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
					if ( rand() % 5 == 0 && parent != NULL )
					{
						parent->increaseSkill(PRO_RANGED);
					}
				}
				else
				{
					if ( cat == THROWN )
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
						else if ( hit.entity->skill[2] > 0 )
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
				}

				bool doAlert = true;
				// fix for confuse potion aggro'ing monsters on impact.
				if ( !wasConfused && hitstats && hitstats->EFFECTS[EFF_CONFUSED] && hit.entity->behavior == &actMonster && parent )
				{
					doAlert = false;
					if ( hit.entity->monsterTarget == parent->getUID() )
					{
						hit.entity->monsterReleaseAttackTarget();
					}
				}

				// alert the monster
				if ( hit.entity->behavior == &actMonster && hitstats && parent != nullptr && doAlert )
				{
					bool alertTarget = true;
					bool targetHealed = false;
					if ( parent->behavior == &actMonster && parent->monsterAllyIndex != -1 )
					{
						if ( hit.entity->behavior == &actMonster && hit.entity->monsterAllyIndex != -1 )
						{
							// if a player ally + hit another ally, don't aggro back
							alertTarget = false;
						}
					}

					if ( disableAlertBlindStatus )
					{
						alertTarget = false;
						if ( hitstats->EFFECTS[EFF_BLIND] )
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
					Entity* ohitentity = hit.entity;
					node_t* node;
					for ( node = map.creatures->first; node != nullptr && alertAllies && !targetHealed; node = node->next ) //Searching for monsters? Creature list, not entity list.
					{
						Entity* entity = (Entity*)node->element;
						if ( entity && entity->behavior == &actMonster && entity != ohitentity && entity != polymorphedTarget )
						{
							if ( entity->checkFriend(hit.entity) )
							{
								if ( entity->monsterState == MONSTER_STATE_WAIT )
								{
									double tangent = atan2(entity->y - ohitentity->y, entity->x - ohitentity->x);
									lineTrace(ohitentity, ohitentity->x, ohitentity->y, tangent, 1024, 0, false);
									if ( hit.entity == entity )
									{
										entity->monsterAcquireAttackTarget(*parent, MONSTER_STATE_PATH);
									}
								}
							}
						}
					}
					hit.entity = ohitentity;
					Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
					if ( parent->behavior == &actPlayer && !skipMessage )
					{
						if ( !strcmp(hitstats->name, "") )
						{
							if ( hitstats->HP <= 0 )
							{
								// HP <= 0
								if ( parent && parent->behavior == &actPlayer )
								{
									messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[692], language[697], MSG_COMBAT);
								}
							}
							else
							{
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[690], language[690], MSG_COMBAT);
								if ( damage == 0 )
								{
									messagePlayer(parent->skill[2], language[447]);
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
									messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[692], language[697], MSG_COMBAT);
								}
							}
							else
							{
								messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[690], language[694], MSG_COMBAT);
								if ( damage == 0 )
								{
									if ( hitstats->sex )
									{
										messagePlayer(parent->skill[2], language[449]);
									}
									else
									{
										messagePlayer(parent->skill[2], language[450]);
									}
								}
							}
						}
					}
				}
				if ( hit.entity->behavior == &actPlayer && !skipMessage )
				{
					Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
					messagePlayerColor(hit.entity->skill[2], color, language[588], itemname);
					if ( damage == 0 && !wasPotion )
					{
						messagePlayer(hit.entity->skill[2], language[452]);
					}
				}
			}
			else
			{
				switch ( item->type )
				{
					case POTION_FIRESTORM:
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
						}
						spawnMagicTower(parent, my->x, my->y, SPELL_FIREBALL, hit.entity);
						break;
					case POTION_ICESTORM:
						spawnMagicTower(parent, my->x, my->y, SPELL_COLD, hit.entity);
						break;
					case POTION_THUNDERSTORM:
						spawnMagicTower(parent, my->x, my->y, SPELL_LIGHTNING, hit.entity);
						break;
					default:
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
						if ( rand() % 2 == 0 )
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
			list_RemoveNode(my->mynode);
			return;
		}
		else if ( item->type == FOOD_CREAMPIE )
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
			list_RemoveNode(my->mynode);
			return;
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
				list_RemoveNode(my->mynode);
				return;
			}
		}
		else if ( item->type == BOOMERANG && hit.entity && uidToEntity(my->parent) )
		{
			// boomerang always return to owner.
			free(item);
			list_RemoveNode(my->mynode);
			return;
		}
		else if ( item && itemIsThrowableTinkerTool(item) /*&& !(item->type >= TOOL_BOMB && item->type <= TOOL_TELEPORT_BOMB)*/ )
		{
			// non-bomb tools will fall to the ground and get placed.
		}
		else
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
			if ( itemCategory(item) == THROWN )
			{
				//Hack to make monsters stop catching your shurikens and chakrams.
				entity->parent = my->parent;
			}
			free(item);
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
