/*-------------------------------------------------------------------------------

	BARONY
	File: actgold.cpp
	Desc: behavior function for gold

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "messages.hpp"
#include "sound.hpp"
#include "net.hpp"
#include "entity.hpp"
#include "player.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

#define GOLDBAG_AMOUNT my->skill[0]
#define GOLDBAG_AMBIENCE my->skill[1]

void actGoldBag(Entity* my)
{
	int i;

	if ( my->flags[INVISIBLE] )
	{
		if ( localPlayerNetworkType != NetworkType::CLIENT )
		{
			node_t* node;
			for ( node = map.entities->first; node != NULL; node = node->next )
			{
				Entity* entity = (Entity*)node->element;
				if ( entity->sprite == 245 )   // boulder.vox
				{
					return;
				}
			}
			my->flags[INVISIBLE] = false;
			serverUpdateEntityFlag(my, INVISIBLE);
			if ( !strcmp(map.name, "Sokoban") )
			{
				for ( i = 0; i < MAXPLAYERS; i++ )
				{
					steamAchievementClient(i, "BARONY_ACH_PUZZLE_MASTER");
				}
			}
		}
		else
		{
			return;
		}
	}

	GOLDBAG_AMBIENCE--;
	if ( GOLDBAG_AMBIENCE <= 0 )
	{
		GOLDBAG_AMBIENCE = TICKS_PER_SECOND * 30;
		playSoundEntityLocal( my, 149, 16 );
	}

	// pick up gold
	if ( localPlayerNetworkType != NetworkType::CLIENT )
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if ( (i == 0 && selectedEntity == my) || (client_selected[i] == my) )
			{
				if (inrange[i])
				{
					if (players[i] && players[i]->entity)
					{
						playSoundEntity(players[i]->entity, 242 + rand() % 4, 64 );
					}
					stats[i]->GOLD += GOLDBAG_AMOUNT;
					if ( i != 0 )
					{
						if ( localPlayerNetworkType == NetworkType::SERVER )
						{
							// send the client info on the gold it picked up
							strcpy((char*)net_packet->data, "GOLD");
							SDLNet_Write32(stats[i]->GOLD, &net_packet->data[4]);
							net_packet->address.host = net_clients[i - 1].host;
							net_packet->address.port = net_clients[i - 1].port;
							net_packet->len = 8;
							sendPacketSafe(net_sock, -1, net_packet, i - 1);
						}
					}

					// message for item pickup
					if ( GOLDBAG_AMOUNT == 1 )
					{
						messagePlayer(i, language[483]);
					}
					else
					{
						messagePlayer(i, language[484], GOLDBAG_AMOUNT);
					}

					// remove gold entity
					list_RemoveNode(my->mynode);
					return;
				}
			}
		}
	}
	else
	{
		my->flags[NOUPDATE] = true;
	}
}
