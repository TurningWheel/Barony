/*-------------------------------------------------------------------------------

	BARONY
	File: actwallbuster.cpp
	Desc: implements wall buster code (a mechanism that destroys walls)

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "sound.hpp"
#include "net.hpp"
#include "collision.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

void actWallBuster(Entity *my) {
	int c;

	if( !my->skill[28] )
		return;

	// received on signal
	if( my->skill[28] == 2) {
		Uint16 x = std::min<Uint16>(std::max(0.0,my->x/16),map.width-1);
		Uint16 y = std::min<Uint16>(std::max(0.0,my->y/16),map.height-1);
		map.tiles[OBSTACLELAYER+y*MAPLAYERS+x*MAPLAYERS*map.height] = 0;
		map.tiles[(MAPLAYERS-1)+y*MAPLAYERS+x*MAPLAYERS*map.height] = 0;
		spawnExplosion(my->x,my->y,my->z-8);
		if( multiplayer==SERVER ) {
			for( c=0; c<MAXPLAYERS; c++ ) {
				if( client_disconnected[c]==TRUE )
					continue;
				strcpy((char *)net_packet->data,"WACD");
				SDLNet_Write16(x,&net_packet->data[4]);
				SDLNet_Write16(y,&net_packet->data[6]);
				net_packet->address.host = net_clients[c-1].host;
				net_packet->address.port = net_clients[c-1].port;
				net_packet->len = 8;
				sendPacketSafe(net_sock, -1, net_packet, c-1);
			}
		}
		list_RemoveNode(my->mynode);
	}
}

void actWallBuilder(Entity *my) {
	int c;

	if( !my->skill[28] )
		return;

	// received on signal
	if( my->skill[28] == 2) {
		playSoundEntity( my, 182, 64 );
		Uint16 x = std::min<Uint16>(std::max(0.0,my->x/16),map.width-1);
		Uint16 y = std::min<Uint16>(std::max(0.0,my->y/16),map.height-1);
		map.tiles[OBSTACLELAYER+y*MAPLAYERS+x*MAPLAYERS*map.height] = map.tiles[y*MAPLAYERS+x*MAPLAYERS*map.height];
		if( multiplayer==SERVER ) {
			for( c=0; c<MAXPLAYERS; c++ ) {
				if( client_disconnected[c]==TRUE )
					continue;
				strcpy((char *)net_packet->data,"WALC");
				SDLNet_Write16(x,&net_packet->data[4]);
				SDLNet_Write16(y,&net_packet->data[6]);
				net_packet->address.host = net_clients[c-1].host;
				net_packet->address.port = net_clients[c-1].port;
				net_packet->len = 8;
				sendPacketSafe(net_sock, -1, net_packet, c-1);
			}
		}
		list_RemoveNode(my->mynode);
	}
}