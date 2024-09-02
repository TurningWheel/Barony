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
								FollowerMenu[monsterInteracting->monsterAllyIndex].entityToInteractWith = nullptr; // in lieu of my->clearMonsterInteract, my might have been deleted.
								return;
							}
						}
					}
					else if ( items[my->skill[10]].category == Category::FOOD && monsterInteracting->getMonsterTypeFromSprite() != SLIME )
					{
						if ( monsterInteracting->monsterConsumeFoodEntity(my, monsterInteracting->getStats()) && monsterInteracting->monsterAllyIndex >= 0 )
						{
							FollowerMenu[monsterInteracting->monsterAllyIndex].entityToInteractWith = nullptr; // in lieu of my->clearMonsterInteract, my might have been deleted.
							return;
						}
					}
					else
					{
						if ( monsterInteracting->monsterAddNearbyItemToInventory(monsterInteracting->getStats(), 24, 9, my) && monsterInteracting->monsterAllyIndex >= 0 )
						{
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
	default:
		break;
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

	if ( my->z < groundheight )
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
			if ( tile || (my->sprite >= 610 && my->sprite <= 613) || (my->sprite >= 1206 && my->sprite <= 1209) )
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
						Compendium_t::Events_t::eventUpdateWorld(playerOwner, Compendium_t::CPDM_PITS_ITEMS_VALUE_LOST, "pits", items[ITEM_TYPE].value);
					}
				}
			}
		}
		if ( ITEM_TYPE == ARTIFACT_MACE && my->parent != 0 )
		{
			steamAchievementEntity(uidToEntity(my->parent), "BARONY_ACH_STFU");
		}
		list_RemoveNode(my->mynode);
		return;
	}

	// don't perform unneeded computations on items that have basically no velocity
	if (!overWater && onground &&
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
