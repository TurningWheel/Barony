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
#include "sound.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "interface/interface.hpp"
#include "player.hpp"
#include "scores.hpp"
#include "paths.hpp"

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

void actItem(Entity* my)
{
	Item* item;
	int i;

	if ( multiplayer == CLIENT )
	{
		my->flags[NOUPDATE] = true;
		if ( ITEM_LIFE == 0 )
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
		else if ( my->skill[10] == 0 && my->itemReceivedDetailsFromServer == 0 && players[clientnum] && players[clientnum]->entity )
		{
			// request itemtype and beatitude
			if ( ticks % (TICKS_PER_SECOND * 6) == my->getUID() % (TICKS_PER_SECOND * 6) )
			{
				strcpy((char*)net_packet->data, "ITMU");
				net_packet->data[4] = clientnum;
				SDLNet_Write32(my->getUID(), &net_packet->data[5]);
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 9;
				sendPacketSafe(net_sock, -1, net_packet, 0);
				/*for ( node_t* tmpNode = map.creatures->first; tmpNode != nullptr; tmpNode = tmpNode->next )
				{
					Entity* follower = (Entity*)tmpNode->element;
					if ( follower && players[clientnum]->entity == follower->monsterAllyGetPlayerLeader() )
					{
						if ( follower->getMonsterTypeFromSprite() == GYROBOT )
						{
							break;
						}
					}
				}*/
			}
		}
	}
	else
	{
		// select appropriate model
		my->skill[2] = -5;
		if ( my->itemSokobanReward != 1 )
		{
			my->flags[INVISIBLE] = false;
		}
		item = newItemFromEntity(my);
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
								messagePlayer(monsterInteracting->monsterAllyIndex, language[3637]);
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
									messagePlayer(monsterInteracting->monsterAllyIndex, language[3637]);
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
			if ( (i == 0 && selectedEntity[0] == my) || (client_selected[i] == my) || (splitscreen && selectedEntity[i] == my) )
			{
				if ( inrange[i] && players[i] && players[i]->entity )
				{
					bool trySalvage = false;
					if ( static_cast<Uint32>(my->itemAutoSalvageByPlayer) == players[i]->entity->getUID() )
					{
						trySalvage = true;
						my->itemAutoSalvageByPlayer = 0; // clear interact flag.
					}
					if ( !trySalvage )
					{
						playSoundEntity( players[i]->entity, 35 + rand() % 3, 64 );
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
							if ( GenericGUI[i].isItemSalvageable(item2, i) )
							{
								if ( GenericGUI[i].tinkeringSalvageItem(item2, true, i) )
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
								messagePlayer(i, language[3664], item2->description());
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
								if (i == 0 || (splitscreen && i > 0) )
								{
									// item is the new inventory stack for server, free the picked up items
									free(item2); 
									int oldcount = item->count;
									item->count = pickedUpCount;
									messagePlayer(i, language[504], item->description());
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
									messagePlayer(i, language[504], item->description());
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
								}
								if ( i != 0 && !splitscreen )
								{
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

	if ( my->itemNotMoving )
	{
		switch ( my->sprite )
		{
			case 610:
			case 611:
			case 612:
			case 613:
				my->spawnAmbientParticles(80, my->sprite - 4, 10 + rand() % 40, 1.0, false);
				if ( !my->light )
				{
					my->light = lightSphereShadow(my->x / 16, my->y / 16, 3, 192);
				}
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

	// gravity
	bool onground = false;
	real_t groundHeight = 7.5 - models[my->sprite]->sizey * .25;

	if ( my->z < groundHeight )
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
			if ( map.tiles[(int)(my->y / 16)*MAPLAYERS + (int)(my->x / 16)*MAPLAYERS * map.height] 
				|| (my->sprite >= 610 && my->sprite <= 613) )
			{
				// land
				ITEM_VELZ *= -.7;
				if ( ITEM_VELZ > -.35 )
				{
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
							my->z = 8.5 - models[my->sprite]->sizey * .25;
						}
						else if ( my->sprite == items[BOOMERANG].index )
						{
							my->z = 9.0 - models[my->sprite]->sizey * .25;
						}
						else
						{
							my->z = 8.75 - models[my->sprite]->sizey * .25;
						}
					}
					else if ( my->sprite == items[TOOL_BOMB].index || my->sprite == items[TOOL_FREEZE_BOMB].index
						|| my->sprite == items[TOOL_SLEEP_BOMB].index || my->sprite == items[TOOL_TELEPORT_BOMB].index
						|| my->sprite == items[TOOL_DETONATOR_CHARGE].index )
					{
						my->roll = 3 * PI / 2;
						my->z = 7.5 - models[my->sprite]->sizey * .25;
					}
					else
					{
						my->roll = PI / 2.0;
						my->z = 7.5 - models[my->sprite]->sizey * .25;
					}
					ITEM_VELZ = 0;
					onground = true;
				}
				else
				{
					onground = true;
					my->z = 7.5 - models[my->sprite]->sizey * .25 - .0001;
				}
			}
			else
			{
				// fall
				ITEM_VELZ += 0.04;
				my->z += ITEM_VELZ;
				my->roll += 0.04;
			}
		}
		else
		{
			// fall
			ITEM_VELZ += 0.04;
			my->z += ITEM_VELZ;
			my->roll += 0.04;
		}
	}

	// falling out of the map
	if ( my->z > 128 )
	{
		if ( ITEM_TYPE == ARTIFACT_MACE && my->parent != 0 )
		{
			steamAchievementEntity(uidToEntity(my->parent), "BARONY_ACH_STFU");
		}
		list_RemoveNode(my->mynode);
		return;
	}

	// don't perform unneeded computations on items that have basically no velocity
	double groundheight;
	if ( my->sprite == 569 )
	{
		groundheight = 8.5 - models[my->sprite]->sizey * .25;
	}
	else if ( my->sprite == items[TOOL_BOMB].index || my->sprite == items[TOOL_FREEZE_BOMB].index
		|| my->sprite == items[TOOL_SLEEP_BOMB].index || my->sprite == items[TOOL_TELEPORT_BOMB].index
		|| my->sprite == items[TOOL_DETONATOR_CHARGE].index )
	{
		groundheight = 7.5 - models[my->sprite]->sizey * .25;
	}
	else if ( my->sprite == 567 )
	{
		groundheight = 8.75 - models[my->sprite]->sizey * .25;
	}
	else if ( my->sprite == items[BOOMERANG].index )
	{
		groundheight = 9.0 - models[my->sprite]->sizey * .25;
	}
	else
	{
		groundheight = 7.5 - models[my->sprite]->sizey * .25;
	}

	if ( onground && my->z > groundheight - .0001 && my->z < groundheight + .0001 && fabs(ITEM_VELX) < 0.02 && fabs(ITEM_VELY) < 0.02 )
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
