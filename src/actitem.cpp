/*-------------------------------------------------------------------------------

	BARONY
	File: actitem.cpp
	Desc: behavior function for items

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "items.hpp"
#include "engine/audio/sound.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "interface/interface.hpp"
#include "player.hpp"
#include "scores.hpp"
#include "paths.hpp"
#include "prng.hpp"
#include "mod_tools.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define ITEM_VELX my->vel_x
#define ITEM_VELY my->vel_y
#define ITEM_VELZ my->vel_z
#define ITEM_NOCOLLISION my->flags[USERFLAG1]
#define ITEM_TYPE my->skill[10]
#define ITEM_STATUS (Status)my->skill[11]
#define ITEM_BEATITUDE my->skill[12]
#define ITEM_COUNT my->skill[13]
#define ITEM_APPEARANCE my->skill[14]
#define ITEM_IDENTIFIED my->skill[15]
#define ITEM_LIFE my->skill[16]
#define ITEM_AMBIENCE my->skill[17]
#define ITEM_SPLOOSHED my->skill[27]
#define ITEM_WATERBOB my->fskill[2]

bool itemProcessReturnItemEffect(Entity* my, bool fallingIntoVoid)
{
	if ( Entity* returnToParent = uidToEntity(my->itemReturnUID) )
	{
		int returnTime = std::max(10, std::max(getSpellDamageSecondaryFromID(SPELL_RETURN_ITEMS, returnToParent, nullptr, returnToParent, 0.0, false),
			getSpellDamageFromID(SPELL_RETURN_ITEMS, returnToParent, nullptr, returnToParent, 0.0, false)));
		if ( fallingIntoVoid || (my->ticks >= returnTime && returnToParent->behavior == &actPlayer) )
		{
			int cost = std::max(1, getSpellEffectDurationSecondaryFromID(SPELL_RETURN_ITEMS, returnToParent, nullptr, returnToParent));
			if ( cost > 0 && !returnToParent->safeConsumeMP(cost) )
			{
				Stat* returnStats = returnToParent->getStats();
				if ( returnStats && returnStats->MP > 0 )
				{
					returnToParent->modMP(-returnStats->MP);
				}
				if ( spell_t* sustainSpell = returnToParent->getActiveMagicEffect(SPELL_RETURN_ITEMS) )
				{
					sustainSpell->sustain = false;
				}
				playSoundEntity(returnToParent, 163, 128);
				if ( Item* item = newItemFromEntity(my, true) )
				{
					messagePlayerColor(returnToParent->skill[2], MESSAGE_COMBAT, makeColorRGB(255, 0, 0), Language::get(6813), item->getName());
					free(item);
				}
				my->itemReturnUID = 0;
			}
			else
			{
				int i = returnToParent->skill[2];
				Item* item2 = newItemFromEntity(my);
				if ( item2 )
				{
					int pickedUpCount = item2->count;
					Item* item = itemPickup(i, item2);
					if ( item )
					{
						if ( players[i]->isLocalPlayer() )
						{
							// item is the new inventory stack for server, free the picked up items
							free(item2);
							int oldcount = item->count;
							item->count = pickedUpCount;
							messagePlayer(i, MESSAGE_INTERACTION | MESSAGE_INVENTORY, Language::get(3746), item->getName());
							item->count = oldcount;
						}
						else
						{
							messagePlayer(i, MESSAGE_INTERACTION | MESSAGE_INVENTORY, Language::get(3746), item->getName());
							free(item); // item is the picked up items (item == item2)
						}

						if ( returnToParent->behavior == &actPlayer )
						{
							if ( auto spell = getSpellFromID(SPELL_RETURN_ITEMS) )
							{
								players[returnToParent->skill[2]]->mechanics.sustainedSpellIncrementMP(cost, spell->skillID);
							}
							players[returnToParent->skill[2]]->mechanics.updateSustainedSpellEvent(SPELL_RETURN_ITEMS, 10.0, 1.0, nullptr);
						}

						spawnMagicEffectParticles(my->x, my->y, my->z, 170);
						my->removeLightField();
						list_RemoveNode(my->mynode);
						return true;
					}
				}
			}
		}
	}
	return false;
}

void onItemPickedUp(Entity& who, Uint32 itemUid)
{
	for ( int player = 0; player < MAXPLAYERS; ++player )
	{
		if ( players[player]->mechanics.donationRevealedOnFloor == itemUid )
		{
			if ( who.behavior == &actPlayer )
			{
				messagePlayerColor(who.skill[2], MESSAGE_HINT, makeColorRGB(255, 255, 0), Language::get(6943)); // you discovered a gift

				for ( int player2 = 0; player2 < MAXPLAYERS; ++player2 ) // relay to other players
				{
					if ( player2 != who.skill[2] && !client_disconnected[player2] )
					{
						messagePlayerColor(player2, MESSAGE_HINT, makeColorRGB(255, 255, 0), Language::get(6944), stats[who.skill[2]]->name); // an ally discovered a gift
					}
				}
			}
			else if ( who.behavior == &actMonster && who.monsterAllyIndex >= 0 && who.monsterAllyIndex < MAXPLAYERS && who.getStats() )
			{
				std::string allyName = who.getStats()->name;
				if ( allyName == "" )
				{
					allyName = getMonsterLocalizedName(who.getStats()->type);
				}

				messagePlayerColor(who.monsterAllyIndex, MESSAGE_HINT, makeColorRGB(255, 255, 0), Language::get(6944), allyName.c_str()); // your ally %s discovered a gift

				for ( int player2 = 0; player2 < MAXPLAYERS; ++player2 ) // relay to other players
				{
					if ( player2 != who.monsterAllyIndex && !client_disconnected[player2] )
					{
						messagePlayerColor(player2, MESSAGE_HINT, makeColorRGB(255, 255, 0), Language::get(6945), stats[who.monsterAllyIndex]->name, allyName.c_str()); // %s's ally %s discovered a gift
					}
				}
			}

			players[player]->mechanics.updateSustainedSpellEvent(SPELL_DONATION, 150.0, 1.0, nullptr);
			break;
		}
	}
}

bool entityWantsJewel(int tier, Entity& entity, Stat& stats, bool checkTypeOnly)
{
	int req = -1;
	switch ( stats.type )
	{
		case GNOME:
		case GOBLIN:
			req = 1;
			break;
		case HUMAN:
		case GREMLIN:
		case SUCCUBUS:
		case GOATMAN:
			req = 2;
			break;
		case KOBOLD:
		case INSECTOID:
		case DRYAD:
		case MYCONID:
		case BUGBEAR:
		case INCUBUS:
			req = 3;
			break;
		case VAMPIRE:
		case SALAMANDER:
			req = 4;
			break;
		//case TROLL,
		//case AUTOMATON,
		default:
			break;
	}

	if ( req < 0 )
	{
		return false;
	}

	if ( entity.behavior != &actMonster ) { return false; }
	if ( !entity.monsterIsTargetable() || !entity.isMobile() ) { return false; }
	if ( entity.isBossMonster() ) { return false; }
	if ( entity.monsterAllyGetPlayerLeader() ) { return false; }
	if ( stats.type == INCUBUS && !strncmp(stats.name, "inner demon", strlen("inner demon")) ) { return false; }
	//if ( stats.leader_uid != 0 && uidToEntity(stats.leader_uid) ) { return false; }

	if ( checkTypeOnly )
	{
		return req >= 0;
	}
	else if ( req >= 0 )
	{
		if ( (tier * 5) >= currentlevel )
		{
			return true;
		}
		//return tier >= req;
	}

	return false;
}

bool jewelItemRecruit(Entity* parent, Entity* entity, int itemStatus, const char** msg)
{
	if ( !(entity && parent) )
	{
		return false;
	}

	if ( parent->behavior != &actPlayer )
	{
		return false;
	}

	Stat* entitystats = entity->getStats();
	if ( !entitystats ) 
	{
		return false;
	}

	int allowedFollowers = std::min(8, std::max(4, 2 * (stats[parent->skill[2]]->getModifiedProficiency(PRO_LEADERSHIP) / 20)));
	int numFollowers = 0;
	for ( node_t* node = stats[parent->skill[2]]->FOLLOWERS.first; node; node = node->next )
	{
		Entity* follower = nullptr;
		if ( (Uint32*)node->element )
		{
			follower = uidToEntity(*((Uint32*)node->element));
		}
		if ( follower )
		{
			Stat* followerStats = follower->getStats();
			if ( followerStats )
			{
				if ( !(followerStats->type == SENTRYBOT || followerStats->type == GYROBOT
					|| followerStats->type == SPELLBOT || followerStats->type == DUMMYBOT) )
				{
					++numFollowers;
				}
			}
		}
	}

	if ( numFollowers >= allowedFollowers )
	{
		if ( allowedFollowers >= 8 )
		{
			if ( msg )
			{
				*msg = Language::get(3482);
			}
		}
		else
		{
			if ( msg )
			{
				*msg = Language::get(3480);
			}
		}
		return false;
	}
	else if ( forceFollower(*parent, *entity) )
	{
		Entity* fx = createRadiusMagic(SPELL_FORGE_JEWEL, entity, entity->x, entity->y, 16, 2 * TICKS_PER_SECOND, entity);
		spawnMagicEffectParticles(entity->x, entity->y, entity->z, 2410);
		playSoundEntity(entity, 167, 64);

		if ( parent->behavior == &actPlayer )
		{
			magicOnSpellCastEvent(parent, parent, entity, SPELL_FORGE_JEWEL, spell_t::SPELL_LEVEL_EVENT_DEFAULT, 1);
			messagePlayerMonsterEvent(parent->skill[2], makeColorRGB(0, 255, 0), *entitystats, Language::get(6954), Language::get(6955), MSG_COMBAT);
			Compendium_t::Events_t::eventUpdateMonster(parent->skill[2], Compendium_t::CPDM_RECRUITED, entity, 1);

			if ( itemStatus == DECREPIT )
			{
				Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_JEWEL_RECRUIT_DECREPIT, GEM_JEWEL, 1);
			}
			else if ( itemStatus == WORN )
			{
				Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_JEWEL_RECRUIT_WORN, GEM_JEWEL, 1);
			}
			else if ( itemStatus == SERVICABLE )
			{
				Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_JEWEL_RECRUIT_SERVICABLE, GEM_JEWEL, 1);
			}
			else if ( itemStatus == EXCELLENT )
			{
				Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_JEWEL_RECRUIT_EXCELLENT, GEM_JEWEL, 1);
			}
			Compendium_t::Events_t::eventUpdate(parent->skill[2], Compendium_t::CPDM_RECRUITED, GEM_JEWEL, 1);

			if ( entitystats->type == HUMAN && entitystats->getAttribute("special_npc") == "merlin" )
			{
				Compendium_t::Events_t::eventUpdateWorld(parent->skill[2], Compendium_t::CPDM_MERLINS, "magicians guild", 1);
			}
			entity->monsterAllyIndex = parent->skill[2];
			if ( multiplayer == SERVER )
			{
				serverUpdateEntitySkill(entity, 42); // update monsterAllyIndex for clients.
			}
		}

		if ( entity->monsterTarget == parent->getUID() )
		{
			entity->monsterReleaseAttackTarget();
		}
		entity->setEffect(EFF_CONFUSED, false, 0, true);

		// change the color of the hit entity.
		entity->flags[USERFLAG2] = true;
		serverUpdateEntityFlag(entity, USERFLAG2);
		if ( monsterChangesColorWhenAlly(entitystats) )
		{
			int bodypart = 0;
			for ( node_t* node = (entity)->children.first; node != nullptr; node = node->next )
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

		return true;
	}
	return false;
}

void actItem(Entity* my)
{
	Item* item;
	int i;

	const bool isArtifact =
        my->sprite == items[ARTIFACT_ORB_RED].index ||
        my->sprite == items[ARTIFACT_ORB_GREEN].index ||
        my->sprite == items[ARTIFACT_ORB_BLUE].index ||
        my->sprite == items[ARTIFACT_ORB_PURPLE].index ||
        ((my->sprite >= (items[TOOL_PLAYER_LOOT_BAG].index) 
			&& (my->sprite < (items[TOOL_PLAYER_LOOT_BAG].index + items[TOOL_PLAYER_LOOT_BAG].variations)))
			);
	if (!isArtifact) {
		my->flags[BURNABLE] = true;
	}
	my->z -= ITEM_WATERBOB;
	my->new_z -= ITEM_WATERBOB;
	ITEM_WATERBOB = 0.0;

	if ( my->sprite >= items[TOOL_DUCK].index && my->sprite < items[TOOL_DUCK].index + 4 )
	{
		if ( multiplayer == CLIENT )
		{
			my->flags[INVISIBLE] = true;
		}
		else
		{
			Item* item = newItemFromEntity(my, true);
			if ( item )
			{
				item->applyDuck(my->parent, my->x, my->y, nullptr, false);
			}
			free(item);
			list_RemoveNode(my->mynode);
			return;
		}
	}

	if ( multiplayer == CLIENT )
	{
		my->flags[NOUPDATE] = true;
		if ( ITEM_LIFE == 0 )
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
					for ( node = map.creatures->first; node != nullptr; node = node->next )
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
				for ( node = map.creatures->first; node != nullptr; node = node->next )
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

		// request entity update (check if I've been deleted)
		if ( ticks % (TICKS_PER_SECOND * 5) == my->getUID() % (TICKS_PER_SECOND * 5) )
		{
			strcpy((char*)net_packet->data, "ENTE");
			net_packet->data[4] = clientnum;
			SDLNet_Write32(my->getUID(), &net_packet->data[5]);
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 9;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
		else if ( my->skill[10] == 0 && my->itemReceivedDetailsFromServer == 0 && players[clientnum] )
		{
			// request itemtype and beatitude
			bool requestUpdate = false;
			Uint32 syncTick = my->getUID() % (TICKS_PER_SECOND * 6);
			Uint32 currentTick = ticks % (TICKS_PER_SECOND * 6);
			if ( ITEM_LIFE == 0 )
			{
				if ( currentTick < syncTick)
				{
					if ( syncTick - currentTick >= TICKS_PER_SECOND * 2 )
					{
						// if the cycle would request details more than 2 seconds away, request now
						requestUpdate = true;
					}
				}
				else if ( currentTick > syncTick )
				{
					requestUpdate = true; // more than 2 seconds away
				}
			}

			if ( (currentTick == syncTick) || requestUpdate )
			{
				strcpy((char*)net_packet->data, "ITMU");
				net_packet->data[4] = clientnum;
				SDLNet_Write32(my->getUID(), &net_packet->data[5]);
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 9;
				sendPacketSafe(net_sock, -1, net_packet, 0);
			}
		}
	}
	else
	{
		// select appropriate model
		my->skill[2] = -5;
		if ( my->itemSokobanReward != 1 && my->itemContainer == 0 )
		{
			my->flags[INVISIBLE] = false;
		}
		item = newItemFromEntity(my, true);
		/*if ( currentlevel > 0 && my->ticks == 1 )
		{
			if ( items[item->type].category != TOME_SPELL && items[item->type].category != SPELLBOOK )
			{
				if ( items[item->type].level < 0 )
				{
					if ( item->type != KEY_IRON && item->type != KEY_BRONZE && item->type != KEY_GOLD
						&& item->type != KEY_SILVER )
					{
						printlog("%s", items[item->type].getIdentifiedName());
					}
				}
			}
		}*/
		my->sprite = itemModel(item);
		free(item);
	}

	if ( ITEM_LIFE == 0 )
	{
		my->createWorldUITooltip();
	}

	ITEM_LIFE++;

	// pick up item
	if (multiplayer != CLIENT)
	{
		if ( my->skill[10] == GEM_JEWEL )
		{
			if ( Entity* parent = uidToEntity(my->parent) )
			{
				if ( parent && parent->behavior == &actPlayer )
				{
					int tier = my->skill[11];
					auto entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 2);
					for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
					{
						list_t* currentList = *it;
						node_t* node;
						for ( node = currentList->first; node != nullptr; node = node->next )
						{
							Entity* entity = (Entity*)node->element;
							if ( entity && entity->behavior == &actMonster )
							{
								if ( Stat* entitystats = entity->getStats() )
								{
									auto hitProps = getParticleEmitterHitProps(my->getUID(), entity);
									if ( hitProps->hits > 0 || (ticks - hitProps->tick) < TICKS_PER_SECOND )
									{
										continue;
									}
									if ( entity->getUID() % 10 == ticks % 10 )
									{
										hitProps->tick = ticks;
										if ( entityWantsJewel(tier, *entity, *entitystats, true) )
										{
											if ( entityDist(my, entity) < 16.0 )
											{
												hitProps->hits++;
												if ( entityWantsJewel(tier, *entity, *entitystats, false) )
												{
													const char* msg = nullptr;
													if ( jewelItemRecruit(parent, entity, tier, &msg) )
													{
														my->clearMonsterInteract();
														my->removeLightField();
														list_RemoveNode(my->mynode);
														return;
													}
													else
													{
														if ( msg )
														{
															auto hitProps = getParticleEmitterHitProps(my->getUID(), parent);
															if ( hitProps && hitProps->hits == 0 )
															{
																++hitProps->hits;
																messagePlayer(parent->isEntityPlayer(), MESSAGE_HINT, msg);
															}
														}
													}
												}
												else
												{
													spawnFloatingSpriteMisc(134, entity->x + (-4 + local_rng.rand() % 9) + cos(entity->yaw) * 2,
														entity->y + (-4 + local_rng.rand() % 9) + sin(entity->yaw) * 2, entity->z + local_rng.rand() % 4);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}

		Uint32 myUid = my->getUID();
		if ( my->isInteractWithMonster() )
		{
			Entity* monsterInteracting = uidToEntity(my->interactedByMonster);
			if ( monsterInteracting )
			{
				if ( my->skill[10] >= 0 && my->skill[10] < NUMITEMS )
				{
					if ( monsterInteracting->getMonsterTypeFromSprite() == GYROBOT )
					{
						if ( monsterInteracting->getStats() && list_Size(&(monsterInteracting->getStats())->inventory) > 1 )
						{
							if ( monsterInteracting->monsterAllyGetPlayerLeader() )
							{
								// "can't carry anymore!"
								messagePlayer(monsterInteracting->monsterAllyIndex, MESSAGE_HINT, Language::get(3637));
							}
						}
						else
						{
							bool oldCount = list_Size(&monsterInteracting->getStats()->inventory);
							Entity* copyOfItem = newEntity(-1, 1, map.entities, nullptr);
							copyOfItem->x = my->x;
							copyOfItem->y = my->y;
							copyOfItem->flags[PASSABLE] = true;
							copyOfItem->flags[INVISIBLE] = true;
							copyOfItem->flags[NOUPDATE] = true;

							bool pickedUpItem = monsterInteracting->monsterAddNearbyItemToInventory(monsterInteracting->getStats(), 16, 2, my);
							if ( oldCount > 0 && list_Size(&monsterInteracting->getStats()->inventory) > oldCount )
							{
								// items didn't stack, throw out the new one.
								node_t* inv = monsterInteracting->getStats()->inventory.last;
								if ( inv )
								{
									Item* toDrop = (Item*)inv->element;
									Entity* dropped = dropItemMonster(toDrop, monsterInteracting, monsterInteracting->getStats(), toDrop->count);
									if ( dropped )
									{
										dropped->vel_x = 0.0;
										dropped->vel_y = 0.0;
									}
								}

								if ( monsterInteracting->monsterAllyGetPlayerLeader() )
								{
									// "can't carry anymore!"
									messagePlayer(monsterInteracting->monsterAllyIndex, MESSAGE_HINT, Language::get(3637));
								}
							}
							else
							{
								Entity* leader = monsterInteracting->monsterAllyGetPlayerLeader();
								if ( leader )
								{
									achievementObserver.playerAchievements[monsterInteracting->monsterAllyIndex].checkPathBetweenObjects(leader, copyOfItem, AchievementObserver::BARONY_ACH_LEVITANT_LACKEY);
								}
							}
							list_RemoveNode(copyOfItem->mynode);
							copyOfItem = nullptr;
							if ( pickedUpItem && monsterInteracting->monsterAllyIndex >= 0 )
							{
								onItemPickedUp(*monsterInteracting, myUid);
								FollowerMenu[monsterInteracting->monsterAllyIndex].entityToInteractWith = nullptr; // in lieu of my->clearMonsterInteract, my might have been deleted.
								return;
							}
						}
					}
					else if ( items[my->skill[10]].category == Category::FOOD && monsterInteracting->getMonsterTypeFromSprite() != SLIME )
					{
						if ( monsterInteracting->monsterConsumeFoodEntity(my, monsterInteracting->getStats()) && monsterInteracting->monsterAllyIndex >= 0 )
						{
							onItemPickedUp(*monsterInteracting, myUid);
							FollowerMenu[monsterInteracting->monsterAllyIndex].entityToInteractWith = nullptr; // in lieu of my->clearMonsterInteract, my might have been deleted.
							return;
						}
					}
					else
					{
						if ( monsterInteracting->monsterAddNearbyItemToInventory(monsterInteracting->getStats(), 24, 9, my) && monsterInteracting->monsterAllyIndex >= 0 )
						{
							onItemPickedUp(*monsterInteracting, myUid);
							FollowerMenu[monsterInteracting->monsterAllyIndex].entityToInteractWith = nullptr; // in lieu of my->clearMonsterInteract, my might have been deleted.
							return;
						}
					}
				}
				my->clearMonsterInteract();
				return;
			}
			my->clearMonsterInteract();
		}

		if ( my->itemReturnUID != 0 )
		{
			if ( itemProcessReturnItemEffect(my, false) )
			{
				return;
			}
		}

		for ( i = 0; i < MAXPLAYERS; i++)
		{
			if ( selectedEntity[i] == my || client_selected[i] == my )
			{
				if ( inrange[i] && players[i] && players[i]->ghost.isActive() )
				{
					Compendium_t::Events_t::eventUpdateMonster(i, Compendium_t::CPDM_GHOST_PUSHES, players[i]->ghost.my, 1);
					my->vel_x += 1.0 * cos(players[i]->ghost.my->yaw);
					my->vel_y += 1.0 * sin(players[i]->ghost.my->yaw);
					my->z = std::max(my->z - 0.1, 0.0);
					my->vel_z = 2 * (-10 - local_rng.rand() % 20) * .01;
					my->itemNotMoving = 0;
					my->itemNotMovingClient = 0;
					my->flags[USERFLAG1] = false; // enable collision
					if ( multiplayer == SERVER )
					{
						for ( int c = 1; c < MAXPLAYERS; c++ )
						{
							if ( client_disconnected[c] || players[c]->isLocalPlayer() )
							{
								continue;
							}
							strcpy((char*)net_packet->data, "GHOI");
							SDLNet_Write32(my->getUID(), &net_packet->data[4]);
							SDLNet_Write16((Sint16)(my->x * 32), &net_packet->data[8]);
							SDLNet_Write16((Sint16)(my->y * 32), &net_packet->data[10]);
							SDLNet_Write16((Sint16)(my->z * 32), &net_packet->data[12]);
							SDLNet_Write16((Sint16)(my->vel_x * 32), &net_packet->data[14]);
							SDLNet_Write16((Sint16)(my->vel_y * 32), &net_packet->data[16]);
							SDLNet_Write16((Sint16)(my->vel_z * 32), &net_packet->data[18]);
							net_packet->address.host = net_clients[c - 1].host;
							net_packet->address.port = net_clients[c - 1].port;
							net_packet->len = 20;
							sendPacketSafe(net_sock, -1, net_packet, c - 1);
						}
					}
				}
				else if ( inrange[i] && players[i] && players[i]->entity )
				{
					bool trySalvage = false;
					if ( static_cast<Uint32>(my->itemAutoSalvageByPlayer) == players[i]->entity->getUID() )
					{
						trySalvage = true;
						my->itemAutoSalvageByPlayer = 0; // clear interact flag.
					}
					if ( !trySalvage )
					{
						playSoundEntity( players[i]->entity, 35 + local_rng.rand() % 3, 64 );
					}
					Item* item2 = newItemFromEntity(my);
					if ( my->itemStolen == 1 && item2 && (static_cast<Uint32>(item2->ownerUid) == players[i]->entity->getUID()) )
					{
						steamAchievementClient(i, "BARONY_ACH_REPOSSESSION");
					}
					//messagePlayer(i, "old owner: %d", item2->ownerUid);
					if (item2)
					{
						if ( trySalvage )
						{
							// auto salvage this item, don't pick it up.
							bool salvaged = false;
							if ( GenericGUI[0].isItemSalvageable(item2, i) ) // let the server [0] salvage for client i
							{
								if ( GenericGUI[0].tinkeringSalvageItem(item2, true, i) ) // let the server [0] salvage for client i
								{
									salvaged = true;
								}
							}

							if ( salvaged )
							{
								onItemPickedUp(*players[i]->entity, myUid);
								free(item2);
								my->removeLightField();
								list_RemoveNode(my->mynode);
								return;
							}
							else
							{
								// unable to salvage.
								messagePlayer(i, MESSAGE_INTERACTION, Language::get(3664), item2->description());
								playSoundPlayer(i, 90, 64);
								free(item2);
							}
						}
						else
						{
							int pickedUpCount = item2->count;
							item = itemPickup(i, item2);
							if (item)
							{
								if (players[i]->isLocalPlayer())
								{
									// item is the new inventory stack for server, free the picked up items
									free(item2); 
									int oldcount = item->count;
									item->count = pickedUpCount;
									messagePlayer(i, MESSAGE_INTERACTION | MESSAGE_INVENTORY, Language::get(504), item->description());
									item->count = oldcount;
									if ( itemCategory(item) == FOOD && my->itemShowOnMap != 0
										&& stats[i] && stats[i]->type == RAT )
									{
										Entity* parent = uidToEntity(my->parent);
										if ( !parent || (parent && parent->behavior != &actPlayer) )
										{
											steamStatisticUpdateClient(i, STEAM_STAT_5000_SECOND_RULE, STEAM_STAT_INT, 1);
										}
									}
								}
								else
								{
									messagePlayer(i, MESSAGE_INTERACTION | MESSAGE_INVENTORY, Language::get(504), item->description());
									if ( itemCategory(item) == FOOD	&& stats[i] && stats[i]->type == RAT )
									{
										auto find = achievementObserver
											.playerAchievements[i].rat5000secondRule.find(my->getUID());
										if ( find != achievementObserver.playerAchievements[i].rat5000secondRule.end() )
										{
											Entity* parent = uidToEntity(my->parent);
											if ( !parent || (parent && parent->behavior != &actPlayer) )
											{
												steamStatisticUpdateClient(i, STEAM_STAT_5000_SECOND_RULE, STEAM_STAT_INT, 1);
											}
										}
									}
									free(item); // item is the picked up items (item == item2)
								}
								onItemPickedUp(*players[i]->entity, myUid);
								my->removeLightField();
								list_RemoveNode(my->mynode);
								return;
							}
						}
					}
				}
			}
		}
	}

	my->removeLightField();
	switch ( my->sprite )
	{
	case 610: // orbs (blue)
		if ( !my->light )
		{
			my->light = addLight(my->x / 16, my->y / 16, "orb_blue");
		}
		break;
	case 611: // red
		if ( !my->light )
		{
			my->light = addLight(my->x / 16, my->y / 16, "orb_red");
		}
		break;
	case 612: // purple
		if ( !my->light )
		{
			my->light = addLight(my->x / 16, my->y / 16, "orb_purple");
		}
		break;
	case 613: // green
		if ( !my->light )
		{
			my->light = addLight(my->x / 16, my->y / 16, "orb_green");
		}
		break;
	case 1206: // loot bags (yellow)
		if ( !my->light )
		{
			my->light = addLight(my->x / 16, my->y / 16, "lootbag_yellow");
		}
		break;
	case 1207: // green
		if ( !my->light )
		{
			my->light = addLight(my->x / 16, my->y / 16, "lootbag_green");
		}
		break;
	case 1208: // red
		if ( !my->light )
		{
			my->light = addLight(my->x / 16, my->y / 16, "lootbag_red");
		}
		break;
	case 1209: // pink
		if ( !my->light )
		{
			my->light = addLight(my->x / 16, my->y / 16, "lootbag_pink");
		}
		break;
	case 1210: // white
		if ( !my->light )
		{
			my->light = addLight(my->x / 16, my->y / 16, "lootbag_white");
		}
		break;
	case 2409: // jewel
		if ( !my->light )
		{
			my->light = addLight(my->x / 16, my->y / 16, "jewel_yellow");
		}
		break;
	default:
		break;
	}

	bool levitating = false;
	Entity* leader = nullptr;
	if ( my->itemFollowUID != 0 )
	{
		if ( multiplayer != CLIENT )
		{
			if ( leader = uidToEntity(my->itemFollowUID) )
			{
				Stat* leaderStats = leader->getStats();
				real_t dist = entityDist(leader, my);

				real_t maxDist = std::max(16, std::min(getSpellDamageSecondaryFromID(SPELL_ATTRACT_ITEMS, leader, nullptr, leader),
					getSpellDamageFromID(SPELL_ATTRACT_ITEMS, leader, nullptr, leader)));
				if ( dist > maxDist + 4.0 || (leaderStats && !leaderStats->getEffectActive(EFF_ATTRACT_ITEMS)) )
				{
					my->itemFollowUID = 0;
					serverUpdateEntitySkill(my, 30);
					leader = nullptr;
				}

				if ( leader )
				{
					levitating = true;
					const real_t followDist = 12.0;
					if ( dist > 12.0 )
					{
						real_t tangent = atan2(leader->y - my->y, leader->x - my->x);
						my->vel_x = cos(tangent) * ((dist - followDist) / MAGICLIGHTBALL_DIVIDE_CONSTANT);
						my->vel_y = sin(tangent) * ((dist - followDist) / MAGICLIGHTBALL_DIVIDE_CONSTANT);
						my->vel_x = (my->vel_x < MAGIC_LIGHTBALL_SPEEDLIMIT) ? my->vel_x : MAGIC_LIGHTBALL_SPEEDLIMIT;
						my->vel_y = (my->vel_y < MAGIC_LIGHTBALL_SPEEDLIMIT) ? my->vel_y : MAGIC_LIGHTBALL_SPEEDLIMIT;
					}
					my->flags[UPDATENEEDED] = true;
					my->flags[NOUPDATE] = false;
				}
			}
		}
		else
		{
			levitating = true;
			my->itemNotMoving = 0;
			my->itemNotMovingClient = 0;
			my->flags[UPDATENEEDED] = true;
			my->flags[NOUPDATE] = false;
		}
	}

	if ( my->sprite == items[GEM_JEWEL].index )
	{
		if ( my->ticks % 25 == 0 || my->ticks % 40 == 0 )
		{
			if ( Entity* fx = spawnMagicParticleCustom(my, 2410, 1.0, 1.0) )
			{
				fx->vel_z = -0.2;
				fx->yaw = (local_rng.rand() % 360) * PI / 180.0;
				fx->x = my->x + 3.0 * cos(fx->yaw);
				fx->y = my->y + 3.0 * sin(fx->yaw);
				fx->focalz = 0.25;
			}
		}
	}

	if ( my->itemNotMoving )
	{
		switch ( my->sprite )
		{
			case 610: // orbs (blue)
                my->spawnAmbientParticles(80, my->sprite - 4, 10 + local_rng.rand() % 40, 1.0, false);
                break;
			case 611: // red
                my->spawnAmbientParticles(80, my->sprite - 4, 10 + local_rng.rand() % 40, 1.0, false);
                break;
			case 612: // purple
                my->spawnAmbientParticles(80, my->sprite - 4, 10 + local_rng.rand() % 40, 1.0, false);
                break;
			case 613: // green
				my->spawnAmbientParticles(80, my->sprite - 4, 10 + local_rng.rand() % 40, 1.0, false);
				break;
			default:
				break;
		}
		if ( multiplayer == CLIENT )
		{
			// let the client process some more gravity and make sure it isn't stopping early at an awkward angle.
			if ( my->itemNotMovingClient == 1 )
			{
				return;
			}
		}
		else
		{
			return;
		}
	}

	// check whether we are over water
	bool overWater = false;
	if (my->x >= 0 && my->y >= 0 && my->x < map.width << 4 && my->y < map.height << 4)
	{
		const int tile = map.tiles[(int)(my->y / 16) * MAPLAYERS + (int)(my->x / 16) * MAPLAYERS * map.height];
		overWater = (tile >= 22 && tile < 30) || (tile >= 64 && tile < 72);
	}

	// gravity
	bool onground = false;
	double groundheight;
	if (overWater) {
		groundheight = 8.0;
	} else {
		if (my->sprite == 569)
		{
			groundheight = 8.5 - models[my->sprite]->sizey * .25;
		}
		else if (my->sprite == items[TOOL_BOMB].index || my->sprite == items[TOOL_FREEZE_BOMB].index
			|| my->sprite == items[TOOL_SLEEP_BOMB].index || my->sprite == items[TOOL_TELEPORT_BOMB].index
			|| my->sprite == items[TOOL_DETONATOR_CHARGE].index)
		{
			groundheight = 7.5 - models[my->sprite]->sizey * .25;
		}
		else if (my->sprite == 567)
		{
			groundheight = 8.75 - models[my->sprite]->sizey * .25;
		}
		else if (my->sprite == items[BOOMERANG].index)
		{
			groundheight = 9.0 - models[my->sprite]->sizey * .25;
		}
		else
		{
			groundheight = 7.5 - models[my->sprite]->sizey * .25;
		}
	}

	my->flags[BURNING] = false;

	if ( levitating )
	{
		/*ITEM_VELZ += 0.04;
		ITEM_VELZ = std::min(0.0, ITEM_VELZ);
		my->z += ITEM_VELZ;*/
		my->z = my->itemLevitateStartZ * my->itemLevitate;
		my->z = std::min(groundheight - 0.1, my->z);
		my->z = std::max(my->z, -7.5);
		my->vel_z = 0.0;
		
		real_t diff = std::max(0.025, my->itemLevitate / 10.0);
		my->itemLevitate = std::max(0.0, my->itemLevitate - diff);

		my->yaw += PI / (TICKS_PER_SECOND * 10);
		my->new_yaw = my->yaw;
		ITEM_WATERBOB = sin(((ticks % (TICKS_PER_SECOND * 2)) / ((real_t)TICKS_PER_SECOND * 2.0)) * (2.0 * PI)) * 0.5;
		my->z += ITEM_WATERBOB;
		my->new_z = my->z;
	}
	else if ( my->z < groundheight )
	{
		// fall
		// chakram and shuriken lie flat, needs to use sprites for client
		if ( my->sprite == 567 || my->sprite == 569 || my->sprite == items[BOOMERANG].index )
		{
			// todo: adjust falling rates for thrown items if need be
			ITEM_VELZ += 0.04;
			my->z += ITEM_VELZ;
			my->roll += 0.08;
		}
		// sentry/spellbot only rotate 90 degrees and land on their side.
		else if ( my->sprite == 897 || my->sprite == 898 )
		{
			ITEM_VELZ += 0.04;
			my->z += ITEM_VELZ;
			my->roll += 0.02;
			my->roll = std::min(my->roll, PI / 2);
		}
		else if ( my->sprite == items[TOOL_BOMB].index || my->sprite == items[TOOL_FREEZE_BOMB].index
			|| my->sprite == items[TOOL_SLEEP_BOMB].index || my->sprite == items[TOOL_TELEPORT_BOMB].index
			|| my->sprite == items[TOOL_DETONATOR_CHARGE].index )
		{
			ITEM_VELZ += 0.04;
			my->z += ITEM_VELZ;
			while ( my->roll > 2 * PI )
			{
				my->roll -= 2 * PI;
			}
			while ( my->roll < 0 )
			{
				my->roll += 2 * PI;
			}
			if ( my->roll > PI / 2 && my->roll < 3 * PI / 2 )
			{
				my->roll += 0.08;
				my->roll = std::min(my->roll, 3 * PI / 2);
			}
			else
			{
				my->roll += 0.08;
			}
		}
		else
		{
			ITEM_VELZ += 0.04;
			my->z += ITEM_VELZ;
			my->roll += 0.04;
		}
	}
	else
	{
		if ( my->x >= 0 && my->y >= 0 && my->x < map.width << 4 && my->y < map.height << 4 )
		{
			const int tile = map.tiles[(int)(my->y / 16) * MAPLAYERS + (int)(my->x / 16) * MAPLAYERS * map.height];
			if ( tile || (my->sprite >= 610 && my->sprite <= 613) || (my->sprite >= 1206 && my->sprite <= 1210) )
			{
				onground = true;
				if (!isArtifact && tile >= 64 && tile < 72) { // landing on lava
					my->flags[BURNING] = true;
					my->skill[11] = BROKEN;
				}
				else if (overWater) {
					if (!ITEM_SPLOOSHED) {
						ITEM_SPLOOSHED = true;
						bool splash = true;
						if ( multiplayer == SINGLE && !splitscreen && my->parent == achievementObserver.playerUids[clientnum] )
						{
							if ( !players[clientnum] || (players[clientnum] && !players[clientnum]->entity) )
							{
								splash = false; // otherwise dropping items into water on death makes a ruckus
							}
						}
						if ( splash )
						{
							playSoundEntity(my, 136, 64);
							createWaterSplash(my->x, my->y, 30);
						}
					}
				}
				else {
					ITEM_VELZ *= -.7; // bounce
					if ( ITEM_VELZ > -.35 )
					{
						// velocity too low, land on ground.
						// chakram and shuriken lie flat, needs to use sprites for client
						if ( my->sprite == 567 || my->sprite == 569 || my->sprite == items[BOOMERANG].index )
						{
							if ( my->sprite == items[BOOMERANG].index )
							{
								my->roll = PI;
							}
							else
							{
								my->roll = PI;
							}
							my->pitch = 0;
							if ( my->sprite == 569 )
							{
								my->z = groundheight;
							}
							else if ( my->sprite == items[BOOMERANG].index )
							{
								my->z = groundheight;
							}
							else
							{
								my->z = groundheight;
							}
						}
						else if ( my->sprite == items[TOOL_BOMB].index || my->sprite == items[TOOL_FREEZE_BOMB].index
							|| my->sprite == items[TOOL_SLEEP_BOMB].index || my->sprite == items[TOOL_TELEPORT_BOMB].index
							|| my->sprite == items[TOOL_DETONATOR_CHARGE].index )
						{
							my->roll = 3 * PI / 2;
							my->z = groundheight;
						}
						else
						{
							my->roll = PI / 2.0;
							my->z = groundheight;
						}
						ITEM_VELZ = 0;
					}
					else
					{
						// just bounce off the ground.
						my->z = groundheight - .0001;
					}
				}
			}
			else
			{
				// fall (no ground here)
				ITEM_VELZ += 0.04;
				my->z += ITEM_VELZ;
				my->roll += 0.04;
			}
		}
		else
		{
			// fall (out of bounds)
			ITEM_VELZ += 0.04;
			my->z += ITEM_VELZ;
			my->roll += 0.04;
		}
	}

	// float in water
	if (onground) {
		if (overWater) {
			my->yaw += PI / (TICKS_PER_SECOND * 10);
			ITEM_WATERBOB = sin(((ticks % (TICKS_PER_SECOND * 2)) / ((real_t)TICKS_PER_SECOND * 2.0)) * (2.0 * PI)) * 0.5;
			my->z += ITEM_WATERBOB;
			my->new_z += ITEM_WATERBOB;
			if (my->flags[BURNING]) {
				my->new_z += 0.03;
				my->z += 0.03;
			}
		}
	}

	// falling out of the map (or burning in a pit of lava)
	if ( (my->flags[BURNING] && my->z > 12) || my->z > 128 )
	{
		if ( multiplayer != CLIENT )
		{
			int playerOwner = achievementObserver.checkUidIsFromPlayer(my->itemOriginalOwner);
			if ( (my->flags[BURNING] && my->z > 12) )
			{
				if ( playerOwner >= 0 )
				{
					Compendium_t::Events_t::eventUpdateWorld(playerOwner, Compendium_t::CPDM_LAVA_ITEMS_BURNT, "lava", 1);
				}
			}
			else if ( my->z > 128 )
			{
				if ( playerOwner >= 0 )
				{
					Compendium_t::Events_t::eventUpdateWorld(playerOwner, Compendium_t::CPDM_PITS_ITEMS_LOST, "pits", 1);
					if ( ITEM_TYPE >= 0 && ITEM_TYPE < NUMITEMS )
					{
						Compendium_t::Events_t::eventUpdateWorld(playerOwner, Compendium_t::CPDM_PITS_ITEMS_VALUE_LOST, "pits", items[ITEM_TYPE].gold_value);
					}
				}
			}
			if ( my->itemReturnUID != 0 )
			{
				if ( itemProcessReturnItemEffect(my, true) )
				{
					return;
				}
			}
		}
		if ( ITEM_TYPE == ARTIFACT_MACE && my->parent != 0 )
		{
			steamAchievementEntity(uidToEntity(my->parent), "BARONY_ACH_STFU");
		}
		my->removeLightField();
		list_RemoveNode(my->mynode);
		return;
	}

	// don't perform unneeded computations on items that have basically no velocity
	if (!overWater && onground && !levitating &&
		my->z > groundheight - .0001 && my->z < groundheight + .0001 &&
		fabs(ITEM_VELX) < 0.02 && fabs(ITEM_VELY) < 0.02)
	{
		my->itemNotMoving = 1;
		my->flags[UPDATENEEDED] = false;
		if ( multiplayer != CLIENT )
		{
			serverUpdateEntitySkill(my, 18); //update itemNotMoving flag
		}
		else
		{
			my->itemNotMovingClient = 1;
		}
		return;
	}

	// horizontal motion
	if ( ITEM_NOCOLLISION )
	{
		double newx = my->x + ITEM_VELX;
		double newy = my->y + ITEM_VELY;
		if ( !checkObstacle( newx, newy, my, NULL ) )
		{
			my->x = newx;
			my->y = newy;
			my->yaw += sqrt( ITEM_VELX * ITEM_VELX + ITEM_VELY * ITEM_VELY ) * .05;
		}
	}
	else
	{
		double result = clipMove(&my->x, &my->y, ITEM_VELX, ITEM_VELY, my);
		my->yaw += result * .05;

		if ( multiplayer != CLIENT )
		{
			if ( levitating && leader && leader->behavior == &actPlayer )
			{
				players[leader->skill[2]]->mechanics.updateSustainedSpellEvent(SPELL_ATTRACT_ITEMS, result, 0.025, nullptr);
			}
		}

		if ( result != sqrt( ITEM_VELX * ITEM_VELX + ITEM_VELY * ITEM_VELY ) )
		{
			if ( !hit.side )
			{
				ITEM_VELX *= -.5;
				ITEM_VELY *= -.5;
			}
			else if ( hit.side == HORIZONTAL )
			{
				ITEM_VELX *= -.5;
			}
			else
			{
				ITEM_VELY *= -.5;
			}
		}
	}

	ITEM_VELX = ITEM_VELX * .925;
	ITEM_VELY = ITEM_VELY * .925;
}

static Uint32 lastAttractTick = 0;
void Entity::attractItem(Entity& itemEntity)
{
	if ( itemEntity.itemFollowUID != getUID() && itemEntity.z < 16.0 && itemEntity.ticks > TICKS_PER_SECOND )
	{
		if ( lastAttractTick != ::ticks )
		{
			spawnMagicEffectParticles(itemEntity.x, itemEntity.y, itemEntity.z, 170);
			lastAttractTick = ::ticks;
		}
		itemEntity.itemFollowUID = getUID();

		itemEntity.flags[USERFLAG1] = false;
		itemEntity.itemNotMoving = 0;
		itemEntity.itemNotMovingClient = 0;
		itemEntity.z = std::max(itemEntity.z - 0.1, 0.0);
		itemEntity.vel_z = -0.75;
		itemEntity.itemLevitate = 1.0;
		itemEntity.itemLevitateStartZ = itemEntity.z;
		itemEntity.flags[UPDATENEEDED] = true;
		itemEntity.flags[NOUPDATE] = false;
		if ( multiplayer == SERVER )
		{
			for ( int c = 1; c < MAXPLAYERS; c++ )
			{
				if ( client_disconnected[c] || players[c]->isLocalPlayer() )
				{
					continue;
				}
				strcpy((char*)net_packet->data, "ATTI");
				SDLNet_Write32(itemEntity.getUID(), &net_packet->data[4]);
				SDLNet_Write16((Sint16)(itemEntity.x * 32), &net_packet->data[8]);
				SDLNet_Write16((Sint16)(itemEntity.y * 32), &net_packet->data[10]);
				SDLNet_Write16((Sint16)(itemEntity.z * 32), &net_packet->data[12]);
				SDLNet_Write16((Sint16)(itemEntity.vel_x * 32), &net_packet->data[14]);
				SDLNet_Write16((Sint16)(itemEntity.vel_y * 32), &net_packet->data[16]);
				SDLNet_Write16((Sint16)(itemEntity.vel_z * 32), &net_packet->data[18]);
				SDLNet_Write32(getUID(), & net_packet->data[20]);
				net_packet->address.host = net_clients[c - 1].host;
				net_packet->address.port = net_clients[c - 1].port;
				net_packet->len = 24;
				sendPacketSafe(net_sock, -1, net_packet, c - 1);
			}
		}
	}
}