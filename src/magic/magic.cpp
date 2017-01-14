/*-------------------------------------------------------------------------------

	BARONY
	File: magic.cpp
	Desc: contains magic definitions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../entity.hpp"
#include "../interface/interface.hpp"
#include "../sound.hpp"
#include "../items.hpp"
#include "../net.hpp"
#include "../player.hpp"
#include "magic.hpp"

void spell_magicMap(int player) {
	if (players[player] == nullptr || players[player]->entity == nullptr) {
		return;
	}

	if( multiplayer==SERVER && player>0 ) {
		//Tell the client to map the magic.
		strcpy((char *)net_packet->data,"MMAP");
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 4;
		sendPacketSafe(net_sock, -1, net_packet, player-1);
		return;
	}

	int x, y;

	messagePlayer(player, language[412]);
	for( y=0; y<64; y++ ) {
		for( x=0; x<64; x++ ) {
			if( x<map.width && y<map.height ) {
				if( map.tiles[OBSTACLELAYER+y*MAPLAYERS+x*MAPLAYERS*map.height] ) {
					if( !minimap[y][x] ) {
						minimap[y][x] = 4;
					}
				} else if( map.tiles[y*MAPLAYERS+x*MAPLAYERS*map.height] ) {
					if( !minimap[y][x] ) {
						minimap[y][x] = 3;
					}
				} else {
					minimap[y][x] = 0;
				}
			}
		}
	}
}
