/*-------------------------------------------------------------------------------

	BARONY
	File: item_tool.cpp
	Desc: implementation functions for the tool category items

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "sound.hpp"
#include "net.hpp"
#include "player.hpp"
#include "items.hpp"

void Item::applySkeletonKey(int player, Entity& entity)
{
	// skeleton key
	if ( entity.behavior == &actChest )
	{
		playSoundEntity(&entity, 91, 64);
		if ( entity.skill[4] )
		{
			messagePlayer(player, language[1097]);
			entity.skill[4] = 0;
		}
		else
		{
			messagePlayer(player, language[1098]);
			entity.skill[4] = 1;
		}
	}
	else if ( entity.behavior == &actDoor )
	{
		playSoundEntity(&entity, 91, 64);
		if ( entity.skill[5] )
		{
			messagePlayer(player, language[1099]);
			entity.skill[5] = 0;
		}
		else
		{
			messagePlayer(player, language[1100]);
			entity.skill[5] = 1;
		}
	}
	else
	{
		messagePlayer(player, language[1101], getName());
	}
}


void Item::applyLockpick(int player, Entity& entity)
{
	if ( entity.behavior == &actChest )
	{
		if ( entity.skill[4] )
		{
			if ( stats[player]->PROFICIENCIES[PRO_LOCKPICKING] > rand() % 400 )
			{
				playSoundEntity(&entity, 91, 64);
				messagePlayer(player, language[1097]);
				entity.skill[4] = 0;
				players[player]->entity->increaseSkill(PRO_LOCKPICKING);
			}
			else
			{
				playSoundEntity(&entity, 92, 64);
				messagePlayer(player, language[1102]);
				if ( rand() % 10 == 0 )
				{
					players[player]->entity->increaseSkill(PRO_LOCKPICKING);
				}
				else
				{
					if ( rand() % 5 == 0 )
					{
						if ( player == clientnum )
						{
							if ( count > 1 )
							{
								newItem(type, status, beatitude, count - 1, appearance, identified, &stats[player]->inventory);
							}
						}
						stats[player]->weapon->count = 1;
						stats[player]->weapon->status = static_cast<Status>(stats[player]->weapon->status - 1);
						if ( status != BROKEN )
						{
							messagePlayer(player, language[1103]);
						}
						else
						{
							messagePlayer(player, language[1104]);
						}
						if ( player > 0 && multiplayer == SERVER )
						{
							strcpy((char*) (net_packet->data), "ARMR");
							net_packet->data[4] = 5;
							net_packet->data[5] = stats[player]->weapon->status;
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}
					}
				}
			}
		}
		else
		{
			messagePlayer(player, language[1105]);
		}
	}
	else if ( entity.behavior == &actDoor )
	{
		if ( entity.skill[5] )
		{
			if ( stats[player]->PROFICIENCIES[PRO_LOCKPICKING] > rand() % 400 )
			{
				playSoundEntity(&entity, 91, 64);
				messagePlayer(player, language[1099]);
				entity.skill[5] = 0;
				players[player]->entity->increaseSkill(PRO_LOCKPICKING);
			}
			else
			{
				playSoundEntity(&entity, 92, 64);
				messagePlayer(player, language[1106]);
				if ( rand() % 10 == 0 )
				{
					players[player]->entity->increaseSkill(PRO_LOCKPICKING);
				}
				else
				{
					if ( rand() % 5 == 0 )
					{
						if ( player == clientnum )
						{
							if ( count > 1 )
							{
								newItem(type, status, beatitude, count - 1, appearance, identified, &stats[player]->inventory);
							}
						}
						stats[player]->weapon->count = 1;
						stats[player]->weapon->status = static_cast<Status>(stats[player]->weapon->status - 1);
						if ( status != BROKEN )
						{
							messagePlayer(player, language[1103]);
						}
						else
						{
							messagePlayer(player, language[1104]);
						}
						if ( player > 0 && multiplayer == SERVER )
						{
							strcpy((char*) (net_packet->data), "ARMR");
							net_packet->data[4] = 5;
							net_packet->data[5] = stats[player]->weapon->status;
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}
					}
				}
			}
		}
		else
		{
			messagePlayer(player, language[1107]);
		}
	}
	else
	{
		messagePlayer(player, language[1101], getName());
	}
}
