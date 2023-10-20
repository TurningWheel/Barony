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
#include "engine/audio/sound.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "paths.hpp"
#include "player.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

void actWallBuster(Entity* my)
{
	int c;

	if ( !my->skill[28] )
	{
		return;
	}

	// received on signal
	if ( my->skill[28] == 2)
	{
		Uint16 x = std::min<Uint16>(std::max<int>(0.0, my->x / 16), map.width - 1);
		Uint16 y = std::min<Uint16>(std::max<int>(0.0, my->y / 16), map.height - 1);
		map.tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map.height] = 0;
		map.tiles[(MAPLAYERS - 1) + y * MAPLAYERS + x * MAPLAYERS * map.height] = 0;
		spawnExplosion(my->x, my->y, my->z - 8);
		if ( multiplayer == SERVER )
		{
			for ( c = 1; c < MAXPLAYERS; c++ )
			{
				if ( client_disconnected[c] == true || players[c]->isLocalPlayer() )
				{
					continue;
				}
				strcpy((char*)net_packet->data, "WACD");
				SDLNet_Write16(x, &net_packet->data[4]);
				SDLNet_Write16(y, &net_packet->data[6]);
				net_packet->address.host = net_clients[c - 1].host;
				net_packet->address.port = net_clients[c - 1].port;
				net_packet->len = 8;
				sendPacketSafe(net_sock, -1, net_packet, c - 1);
			}
		}
		generatePathMaps();
		list_RemoveNode(my->mynode);
	}
}

void actWallBuilder(Entity* my)
{
	int c;

	if ( !my->skill[28] )
	{
		return;
	}

	// received on signal
	if ( my->skill[28] == 2)
	{
		bool somebodyinside = false;
		std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 1);
		for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end() && !somebodyinside; ++it )
		{
			list_t* currentList = *it;
			for ( node_t* node = currentList->first; node != nullptr; node = node->next )
			{
				Entity* entity = (Entity*)node->element;
				if ( entity == my || (entity->flags[PASSABLE] && entity->behavior != &actDeathGhost)
					|| entity->behavior == &actDoorFrame 
					|| (entity->behavior != &actMonster 
						&& entity->behavior != &actPlayer
						&& entity->behavior != &actDeathGhost) )
				{
					continue;
				}
				if ( my->x + 8 > entity->x - entity->sizex )
				{
					if ( my->x - 8 < entity->x + entity->sizex )
					{
						if ( my->y + 8 > entity->y - entity->sizey )
						{
							if ( my->y - 8 < entity->y + entity->sizey )
							{
								somebodyinside = true;
								if ( entity->behavior == &actMonster )
								{
									if ( !strncmp(map.name, "Sanctum", 7)
										|| !strncmp(map.name, "Boss", 4) )
									{
										// on boss maps let monsters get stuck
										somebodyinside = false;
									}
								}
								if ( somebodyinside )
								{
									break;
								}
							}
						}
					}
				}
			}
		}

		if ( somebodyinside )
		{
			return;
		}

		playSoundEntity( my, 182, 64 );
		Uint16 x = std::min<Uint16>(std::max<int>(0.0, my->x / 16), map.width - 1);
		Uint16 y = std::min<Uint16>(std::max<int>(0.0, my->y / 16), map.height - 1);
		map.tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map.height] = map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height];

		const real_t effectOffset = 2.0;
		spawnPoof(static_cast<Sint16>(x * 16.0 - effectOffset), static_cast<Sint16>(y * 16.0 - effectOffset), 8, 1.0);
		spawnPoof(static_cast<Sint16>(x * 16.0 - effectOffset), static_cast<Sint16>(y * 16.0 + 16.0 + effectOffset), 8, 1.0);
		spawnPoof(static_cast<Sint16>(x * 16.0 + 16.0 + effectOffset), static_cast<Sint16>(y * 16.0 - effectOffset), 8, 1.0);
		spawnPoof(static_cast<Sint16>(x * 16.0 + 16.0 + effectOffset), static_cast<Sint16>(y * 16.0 + 16.0 + effectOffset), 8, 1.0);

		if ( multiplayer == SERVER )
		{
			for ( c = 1; c < MAXPLAYERS; c++ )
			{
				if ( client_disconnected[c] == true || players[c]->isLocalPlayer() )
				{
					continue;
				}
				strcpy((char*)net_packet->data, "WALC");
				SDLNet_Write16(x, &net_packet->data[4]);
				SDLNet_Write16(y, &net_packet->data[6]);
				net_packet->address.host = net_clients[c - 1].host;
				net_packet->address.port = net_clients[c - 1].port;
				net_packet->len = 8;
				sendPacketSafe(net_sock, -1, net_packet, c - 1);
			}
		}
		generatePathMaps();
		list_RemoveNode(my->mynode);
	}
}