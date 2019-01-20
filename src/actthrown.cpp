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
					switch ( item->type )
					{
						case POTION_FIRESTORM:
							spawnMagicTower(parent, my->x, my->y, SPELL_FIREBALL);
							break;
						case POTION_ICESTORM:
							spawnMagicTower(parent, my->x, my->y, SPELL_COLD);
							break;
						case POTION_THUNDERSTORM:
							spawnMagicTower(parent, my->x, my->y, SPELL_LIGHTNING);
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
			bool friendlyHit = false;
			if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
			{
				// test for friendly fire
				if ( parent && parent->checkFriend(hit.entity) )
				{
					friendlyHit = true;
				}
			}
			if ( hit.entity->behavior == &actMonster || hit.entity->behavior == &actPlayer )
			{
				int oldHP = 0;
				oldHP = hit.entity->getHP();
				int damage = (BASE_THROWN_DAMAGE + item->beatitude); // thrown takes half of armor into account.
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
							int skillLVL = parentStats->PROFICIENCIES[PRO_RANGED] / 20;
							int dex = parent->getDEX() / 4;
							damage = (damage + dex) * thrownDamageSkillMultipliers[std::min(skillLVL, 5)];
							damage -= (AC(hit.entity->getStats()) / 4);
						}
						else
						{
							int dex = parent->getDEX() / 4;
							damage += dex;
							damage += parentStats->PROFICIENCIES[PRO_RANGED] / 10; // 0 to 10 bonus attack.
							damage -= (AC(hit.entity->getStats()) / 2);
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
					// thrown weapons do damage if absorbed by armor.
					case BRONZE_TOMAHAWK:
					case IRON_DAGGER:
					case STEEL_CHAKRAM:
					case CRYSTAL_SHURIKEN:
					{
						int skillLVL = parentStats->PROFICIENCIES[PRO_RANGED] / 20;
						damage += (thrownDamageSkillMultipliers[std::min(skillLVL, 5)] * item->weaponGetAttack(parentStats));
						break;
					}
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


				if ( hitstats )
				{
					if ( rand() % 5 == 0 && parent != NULL && itemCategory(item) == POTION && item->type != POTION_EMPTY )
					{
						parent->increaseSkill(PRO_ALCHEMY);
					}

					int postDmgHP = hit.entity->getHP();
					if ( hitstats->type < LICH || hitstats->type >= SHOPKEEPER )   // this makes it impossible to bork the end boss :)
					{
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
									if ( hit.entity->behavior == &actMonster && parent->behavior == &actPlayer )
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
												if ( hitstats->type != HUMAN && hitstats->type != AUTOMATON )
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
								item_PotionBlindness(item, hit.entity, parent);
								usedpotion = true;
								break;
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
							case POTION_PARALYSIS:
								item_PotionParalysis(item, hit.entity, parent);
								usedpotion = true;
								break;
							case POTION_POLYMORPH:
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								if ( hit.entity->behavior == &actMonster )
								{
									if ( parent->behavior == &actPlayer )
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

				if ( friendlyHit )
				{
					list_RemoveNode(my->mynode);
					return;
				}

				if ( hitstats->HP <= 0 && parent )
				{
					parent->awardXP(hit.entity, true, true);
				}

				// alert the monster
				if ( hit.entity->behavior == &actMonster && hitstats && parent != nullptr )
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
				else if ( hit.entity->behavior == &actPlayer && !skipMessage )
				{
					Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
					messagePlayerColor(hit.entity->skill[2], color, language[588], itemname);
					if ( damage == 0 )
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
						spawnMagicTower(parent, my->x, my->y, SPELL_FIREBALL);
						break;
					case POTION_ICESTORM:
						spawnMagicTower(parent, my->x, my->y, SPELL_COLD);
						break;
					case POTION_THUNDERSTORM:
						spawnMagicTower(parent, my->x, my->y, SPELL_LIGHTNING);
						break;
					default:
						break;
				}
			}
		}
		else
		{
			switch ( item->type )
			{
				case POTION_FIRESTORM:
					spawnMagicTower(parent, my->x, my->y, SPELL_FIREBALL);
					break;
				case POTION_ICESTORM:
					spawnMagicTower(parent, my->x, my->y, SPELL_COLD);
					break;
				case POTION_THUNDERSTORM:
					spawnMagicTower(parent, my->x, my->y, SPELL_LIGHTNING);
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
