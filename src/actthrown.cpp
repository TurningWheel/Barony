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

void actThrown(Entity* my)
{
	Item* item = nullptr;
	Category cat = GEM;
	char* itemname = nullptr;
	node_t* node;

	item = newItemFromEntity(my);
	if ( item )
	{
		cat = itemCategory(item);
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
			free(item);
		}
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

	// gravity
	if ( my->z < 7.5 - models[my->sprite]->sizey * .25 )
	{
		// fall
		if ( cat == THROWN )
		{
			// todo: adjust falling rates for thrown items if need be
			if ( specialMonster )
			{
				THROWN_VELZ += 0.01;
			}
			else
			{
				THROWN_VELZ += 0.03;
			}
			my->z += THROWN_VELZ;
			if ( item->type == BRONZE_TOMAHAWK || item->type == IRON_DAGGER )
			{
				// axe and dagger spin vertically
				my->pitch += 0.2;
			}
			else
			{
				if ( specialMonster )
				{
					my->roll += 0.003;
				}
				else
				{
					my->roll += 0.01;
				}
				my->yaw += 0.5;
			}
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
			if ( map.tiles[(int)(my->y / 16)*MAPLAYERS + (int)(my->x / 16)*MAPLAYERS * map.height] )
			{
				item = newItemFromEntity(my);
				if ( itemCategory(item) == POTION )
				{
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
				else
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
			if ( (i == 0 && selectedEntity == my) || (client_selected[i] == my) )
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
							if ( i == 0 )
							{
								free(item2);
							}
							int oldcount = item->count;
							item->count = 1;
							messagePlayer(i, language[504], item->description());
							item->count = oldcount;
							if ( i != 0 )
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
		list_RemoveNode(my->mynode);
		return;
	}

	// horizontal motion
	double ox = my->x;
	double oy = my->y;
	double oz = my->z;
	double result = clipMove(&my->x, &my->y, THROWN_VELX, THROWN_VELY, my);

	bool usedpotion = false;
	if ( result != sqrt(THROWN_VELX * THROWN_VELX + THROWN_VELY * THROWN_VELY) )
	{
		item = newItemFromEntity(my);
		if ( itemCategory(item) == THROWN && (item->type == STEEL_CHAKRAM || item->type == CRYSTAL_SHURIKEN) )
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

		cat = itemCategory(item);
		itemname = item->getName();
		item->count = 1;
		if ( hit.entity != nullptr )
		{
			Entity* parent = uidToEntity(my->parent);
			Stat* parentStats = nullptr;
			if ( parent )
			{
				parentStats = parent->getStats();
			}
			Stat* hitstats = hit.entity->getStats();

			if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
			{
				// test for friendly fire
				if ( parent && parent->checkFriend(hit.entity) )
				{
					list_RemoveNode(my->mynode);
					return;
				}
			}
			if ( hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer )
			{
				int damage = (BASE_THROWN_DAMAGE - (AC(hit.entity->getStats()) / 2) + item->beatitude); // thrown takes half of armor into account.
				if ( parentStats )
				{
					damage += parentStats->PROFICIENCIES[PRO_RANGED] / 5; // 0 to 20 increase.
				}
				if ( hitstats && !hitstats->defending )
				{
					// zero out the damage if negative, thrown weapons will do piercing damage if not blocking.
					damage = std::max(0, damage);
				}
				switch ( item->type )
				{
					// thrown weapons do damage if absorbed by armor.
					case BRONZE_TOMAHAWK:
					case IRON_DAGGER:
					case STEEL_CHAKRAM:
					case CRYSTAL_SHURIKEN:
						damage += item->weaponGetAttack();
						break;
					default:
						break;
				}
				damage = std::max(0, damage);
				hit.entity->modHP(-damage);

				// set the obituary
				char whatever[256];
				snprintf(whatever, 255, language[1508], itemname);
				hit.entity->setObituary(whatever);

				if ( hitstats )
				{
					if ( hitstats->type < LICH || hitstats->type >= SHOPKEEPER )   // this makes it impossible to bork the end boss :)
					{
						switch ( item->type )
						{
							case POTION_WATER:
								usedpotion = true;
								if ( item->beatitude > 0
									 && hit.entity->behavior == &actMonster
									 && (hit.entity->getRace() == GHOUL ||
										 hit.entity->getRace() == LICH || //TODO: Won't work on liches.
										 hit.entity->getRace() == LICH_FIRE ||
										 hit.entity->getRace() == LICH_ICE ||
										 hit.entity->getRace() == SHADOW ||
										 hit.entity->getRace() == SKELETON ||
										 hit.entity->getRace() == VAMPIRE) )
								{
									//Blessed water damages undead more.
									int damage = -(20 * item->beatitude);
									hit.entity->modHP(damage);
									consumeItem(item);
								}
								else
								{
									item_PotionWater(item, hit.entity);
								}
								break;
							case POTION_BOOZE:
								item_PotionBooze(item, hit.entity);
								if ( parentStats && parentStats->EFFECTS[EFF_DRUNK] )
								{
									steamAchievementEntity(parent, "BARONY_ACH_CHEERS");
								}
								usedpotion = true;
								break;
							case POTION_JUICE:
								item_PotionJuice(item, hit.entity);
								usedpotion = true;
								break;
							case POTION_SICKNESS:
								item_PotionSickness(item, hit.entity);
								usedpotion = true;
								break;
							case POTION_CONFUSION:
								item_PotionConfusion(item, hit.entity);
								usedpotion = true;
								break;
							case POTION_EXTRAHEALING:
							{
								int oldHP = hit.entity->getHP();
								item_PotionExtraHealing(item, hit.entity);
								if ( parent && parent->behavior == &actPlayer )
								{
									if ( parent->checkFriend(hit.entity) )
									{
										steamAchievementClient(parent->skill[2], "BARONY_ACH_THANK_ME_LATER");
									}
									int heal = std::max(hit.entity->getHP() - oldHP, 0);
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
								int oldHP = hit.entity->getHP();
								item_PotionHealing(item, hit.entity);
								if ( parent && parent->behavior == &actPlayer )
								{
									if ( parent->checkFriend(hit.entity) )
									{
										steamAchievementClient(parent->skill[2], "BARONY_ACH_THANK_ME_LATER");
									}
									int heal = std::max(hit.entity->getHP() - oldHP, 0);
									if ( heal > 0 )
									{
										serverUpdatePlayerGameplayStats(parent->skill[2], STATISTICS_HEAL_BOT, heal);
									}
								}
								usedpotion = true;
							}
								break;
							case POTION_CUREAILMENT:
								item_PotionCureAilment(item, hit.entity);
								usedpotion = true;
								break;
							case POTION_BLINDNESS:
								item_PotionBlindness(item, hit.entity);
								usedpotion = true;
								break;
							case POTION_RESTOREMAGIC:
								item_PotionRestoreMagic(item, hit.entity);
								usedpotion = true;
								break;
							case POTION_INVISIBILITY:
								item_PotionInvisibility(item, hit.entity);
								usedpotion = true;
								break;
							case POTION_LEVITATION:
								item_PotionLevitation(item, hit.entity);
								usedpotion = true;
								break;
							case POTION_SPEED:
								item_PotionSpeed(item, hit.entity);
								usedpotion = true;
								break;
							case POTION_ACID:
								item_PotionAcid(item, hit.entity);
								usedpotion = true;
								break;
							case POTION_PARALYSIS:
								item_PotionParalysis(item, hit.entity);
								usedpotion = true;
								break;
							default:
								break;
						}
					}
					if ( THROWN_BOUNCES >= 3 && hitstats->HP <= 0 )
					{
						if ( parent->checkEnemy(hit.entity) )
						{
							steamAchievementEntity(parent, "BARONY_ACH_SEE_THAT");
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

				if ( damage > 0 )
				{
					Entity* gib = spawnGib(hit.entity);
					serverSpawnGibForClient(gib);
					playSoundEntity(hit.entity, 28, 64);
					if ( hit.entity->behavior == &actPlayer )
					{
						if ( hit.entity->skill[2] == clientnum )
						{
							camera_shakex += .1;
							camera_shakey += 10;
						}
						else
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
						if ( hit.entity->skill[2] == clientnum )
						{
							camera_shakex += .05;
							camera_shakey += 5;
						}
						else
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

				// alert the monster
				if ( hit.entity->behavior == &actMonster && parent != nullptr )
				{
					if ( hit.entity->monsterState != MONSTER_STATE_ATTACK && (hitstats->type < LICH || hitstats->type >= SHOPKEEPER) )
					{
						hit.entity->monsterAcquireAttackTarget(*parent, MONSTER_STATE_PATH);
					}
					// alert other monsters too
					Entity* ohitentity = hit.entity;
					node_t* node;
					for ( node = map.creatures->first; node != nullptr; node = node->next ) //Searching for monsters? Creature list, not entity list.
					{
						Entity* entity = (Entity*)node->element;
						if ( entity && entity->behavior == &actMonster && entity != ohitentity )
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
					if ( parent->behavior == &actPlayer )
					{
						if ( !strcmp(hitstats->name, "") )
						{
							messagePlayerMonsterEvent(parent->skill[2], color, *hitstats, language[690], language[690], MSG_COMBAT);
							if ( damage == 0 )
							{
								messagePlayer(parent->skill[2], language[447]);
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
				else if ( hit.entity->behavior == &actPlayer )
				{
					Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
					messagePlayerColor(hit.entity->skill[2], color, language[588], itemname);
					if ( damage == 0 )
					{
						messagePlayer(hit.entity->skill[2], language[452]);
					}
				}
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
		else if ( itemCategory(item) == THROWN && (item->type == STEEL_CHAKRAM || item->type == CRYSTAL_SHURIKEN) && hit.entity == NULL )
		{
			// chakram, shurikens bounce off walls until entity or floor is hit.
			playSoundEntity(my, 66, 64);
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
		THROWN_VELX = THROWN_VELX * .99;
		THROWN_VELY = THROWN_VELY * .99;
		//my->pitch += result * .01;
	}
	else
	{
		THROWN_VELX = THROWN_VELX * .99;
		THROWN_VELY = THROWN_VELY * .99;
		my->pitch += result * .01;
	}
}
