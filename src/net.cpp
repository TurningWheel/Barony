/*-------------------------------------------------------------------------------

	BARONY
	File: net.cpp
	Desc: support functions for networking

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "net.hpp"
#include "messages.hpp"
#include "entity.hpp"
#include "monster.hpp"
#include "interface/interface.hpp"
#include "magic/magic.hpp"
#include "sound.hpp"
#include "items.hpp"
#include "shops.hpp"
#include "menu.hpp"
#include "scores.hpp"
#include "collision.hpp"
#include "paths.hpp"
#ifdef STEAMWORKS
#include <steam/steam_api.h>
#include "steam.hpp"
#endif
#include "player.hpp"

NetHandler* net_handler = nullptr;

char last_ip[64] = "";
char last_port[64] = "";
char lobbyChatbox[LOBBY_CHATBOX_LENGTH];
list_t lobbyChatboxMessages;

// uncomment this to have the game log packet info
//#define PACKETINFO

void packetDeconstructor(void* data)
{
	packetsend_t* packetsend = (packetsend_t*)data;
	SDLNet_FreePacket(packetsend->packet);
	free(data);
}

/*-------------------------------------------------------------------------------

	sendPacket

	when STEAMWORKS is undefined, works like SDLNet_UDP_Send and last argument
	is ignored. Otherwise, the first two arguments are ignored and the packet
	is sent with SteamNetworking()->SendP2PPacket, using the hostnum variable
	to get the steam ID of the same player number and the first two arguments
	are ignored.

-------------------------------------------------------------------------------*/

int sendPacket(UDPsocket sock, int channel, UDPpacket* packet, int hostnum)
{
	if ( directConnect )
	{
		return SDLNet_UDP_Send(sock, channel, packet);
	}
	else
	{
#ifdef STEAMWORKS
		if ( steamIDRemote[hostnum] && !client_disconnected[hostnum] )
		{
			return SteamNetworking()->SendP2PPacket(*static_cast<CSteamID* >(steamIDRemote[hostnum]), packet->data, packet->len, k_EP2PSendUnreliable, 0);
		}
		else
		{
			return 0;
		}
#else
		return 0;
#endif
	}
}

/*-------------------------------------------------------------------------------

	sendPacketSafe

	works like sendPacket, but adds an additional layer of insurance to
	increase the chance of a successful transmission. When STEAMWORKS is
	undefined, the game uses its own system to increase the transmission
	success rate of a packet. Otherwise it just sends the packet with
	SendP2PPacket's k_EP2PSendReliable flag

-------------------------------------------------------------------------------*/

Uint32 packetnum = 0;
int sendPacketSafe(UDPsocket sock, int channel, UDPpacket* packet, int hostnum)
{
	packetsend_t* packetsend = (packetsend_t*) malloc(sizeof(packetsend_t));
	if (!(packetsend->packet = SDLNet_AllocPacket(NET_PACKET_SIZE)))
	{
		printlog("warning: packet allocation failed: %s\n", SDLNet_GetError());
		return 0;
	}

	packetsend->hostnum = hostnum;
	packetsend->sock = sock;
	packetsend->channel = channel;
	packetsend->packet->channel = channel;
	memcpy(packetsend->packet->data + 9, packet->data, NET_PACKET_SIZE - 9);
	packetsend->packet->len = packet->len + 9;
	packetsend->packet->address.host = packet->address.host;
	packetsend->packet->address.port = packet->address.port;
	strcpy((char*)packetsend->packet->data, "SAFE");
	if ( receivedclientnum || multiplayer != CLIENT )
	{
		packetsend->packet->data[4] = clientnum;
	}
	else
	{
		packetsend->packet->data[4] = MAXPLAYERS;
	}
	SDLNet_Write32(packetnum, &packetsend->packet->data[5]);
	packetsend->num = packetnum;
	packetsend->tries = 0;
	packetnum++;

	node_t* node = list_AddNodeFirst(&safePacketsSent);
	node->element = packetsend;
	node->deconstructor = &packetDeconstructor;

	if ( directConnect )
	{
		return SDLNet_UDP_Send(sock, channel, packetsend->packet);
	}
	else
	{
#ifdef STEAMWORKS
		if ( steamIDRemote[hostnum] && !client_disconnected[hostnum] )
		{
			return SteamNetworking()->SendP2PPacket(*static_cast<CSteamID* >(steamIDRemote[hostnum]), packetsend->packet->data, packetsend->packet->len, k_EP2PSendReliable, 0);
		}
		else
		{
			return 0;
		}
#else
		return 0;
#endif
	}
}

/*-------------------------------------------------------------------------------

	power

	A simple power function designed to work only with integers.

-------------------------------------------------------------------------------*/

int power(int a, int b)
{
	int c, result = 1;
	for ( c = 0; c < b; c++ )
	{
		result *= a;
	}
	return result;
}

/*-------------------------------------------------------------------------------

	messagePlayer

	Support function, messages the player number given by "player" with the
	message "message"

-------------------------------------------------------------------------------*/

void messagePlayer(int player, char* message, ...)
{
	char str[256] = { 0 };

	va_list argptr;
	va_start( argptr, message );
	vsnprintf( str, 255, message, argptr );
	va_end( argptr );

	messagePlayerColor(player, 0xFFFFFFFF, str);
}

/*-------------------------------------------------------------------------------

	messagePlayerColor

	Messages the player number given by "player" with the message "message"
	and color "color"

-------------------------------------------------------------------------------*/

void messagePlayerColor(int player, Uint32 color, char* message, ...)
{
	char str[256] = { 0 };
	va_list argptr;

	if ( message == NULL )
	{
		return;
	}
	if ( player < 0 || player >= MAXPLAYERS )
	{
		return;
	}

	// format the content
	va_start( argptr, message );
	vsnprintf( str, 255, message, argptr );
	va_end( argptr );

	// fixes crash when reading config at start of game
	if (!initialized)
	{
		printlog("%s\n", str);
		return;
	}

	if ( player == clientnum )
	{
		newString(&messages, color, str);
		if ( !disable_messages )
		{
			addMessage(color, str);
		}
		printlog("%s\n", str);
	}
	else if ( multiplayer == SERVER )
	{
		strcpy((char*)net_packet->data, "MSGS");
		SDLNet_Write32(color, &net_packet->data[4]);
		strcpy((char*)(&net_packet->data[8]), str);
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 8 + strlen(str) + 1;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
	}

	int c;
	char tempstr[256];
	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		if ( client_disconnected[c] )
		{
			continue;
		}
		snprintf(tempstr, 256, language[697], stats[c]->name);
		if ( !strcmp(str, tempstr) )
		{
			steamAchievementClient(player, "BARONY_ACH_NOT_A_TEAM_PLAYER");
		}
	}
}

/*-------------------------------------------------------------------------------

	sendEntityUDP / sendEntityTCP

	Updates given entity data for given client. Server -> client functions

-------------------------------------------------------------------------------*/

void sendEntityTCP(Entity* entity, int c)
{
	int j;

	if ( entity == NULL )
	{
		return;
	}
	if ( client_disconnected[c] == true )
	{
		return;
	}

	// send entity data to the client
	strcpy((char*)net_packet->data, "ENTU");
	SDLNet_Write32((Uint32)entity->getUID(), &net_packet->data[4]);
	SDLNet_Write16((Uint16)entity->sprite, &net_packet->data[8]);
	SDLNet_Write16((Sint16)(entity->x * 32), &net_packet->data[10]);
	SDLNet_Write16((Sint16)(entity->y * 32), &net_packet->data[12]);
	SDLNet_Write16((Sint16)(entity->z * 32), &net_packet->data[14]);
	net_packet->data[16] = (Sint8)entity->sizex;
	net_packet->data[17] = (Sint8)entity->sizey;
	net_packet->data[18] = (Uint8)(entity->scalex * 128);
	net_packet->data[19] = (Uint8)(entity->scaley * 128);
	net_packet->data[20] = (Uint8)(entity->scalez * 128);
	SDLNet_Write16((Sint16)(entity->yaw * 256), &net_packet->data[21]);
	SDLNet_Write16((Sint16)(entity->pitch * 256), &net_packet->data[23]);
	SDLNet_Write16((Sint16)(entity->roll * 256), &net_packet->data[25]);
	net_packet->data[27] = (char)(entity->focalx * 8);
	net_packet->data[28] = (char)(entity->focaly * 8);
	net_packet->data[29] = (char)(entity->focalz * 8);
	SDLNet_Write32(entity->skill[2], &net_packet->data[30]);
	net_packet->data[34] = 0;
	net_packet->data[35] = 0;
	for (j = 0; j < 16; j++)
	{
		if ( entity->flags[j] )
		{
			net_packet->data[34 + j / 8] |= power(2, j - (j / 8) * 8);
		}
	}
	SDLNet_Write32((Uint32)ticks, &net_packet->data[36]);
	SDLNet_Write16((Sint16)(entity->vel_x * 32), &net_packet->data[40]);
	SDLNet_Write16((Sint16)(entity->vel_y * 32), &net_packet->data[42]);
	SDLNet_Write16((Sint16)(entity->vel_z * 32), &net_packet->data[44]);
	net_packet->address.host = net_clients[c - 1].host;
	net_packet->address.port = net_clients[c - 1].port;
	net_packet->len = ENTITY_PACKET_LENGTH;
	if ( directConnect )
	{
		SDLNet_TCP_Send(net_tcpclients[c - 1], net_packet->data, net_packet->len);
	}
	else
	{
#ifdef STEAMWORKS
		SteamNetworking()->SendP2PPacket(*static_cast<CSteamID* >(steamIDRemote[c - 1]), net_packet->data, net_packet->len, k_EP2PSendReliable, 0);
#endif
	}
	SDL_Delay(50);
	if ( entity->clientsHaveItsStats )
	{
		entity->serverUpdateEffectsForEntity(false);
	}
}

void sendEntityUDP(Entity* entity, int c, bool guarantee)
{
	int j;

	if ( entity == NULL )
	{
		return;
	}
	if ( client_disconnected[c] == true )
	{
		return;
	}

	// send entity data to the client
	strcpy((char*)net_packet->data, "ENTU");
	SDLNet_Write32((Uint32)entity->getUID(), &net_packet->data[4]);
	SDLNet_Write16((Uint16)entity->sprite, &net_packet->data[8]);
	SDLNet_Write16((Sint16)(entity->x * 32), &net_packet->data[10]);
	SDLNet_Write16((Sint16)(entity->y * 32), &net_packet->data[12]);
	SDLNet_Write16((Sint16)(entity->z * 32), &net_packet->data[14]);
	net_packet->data[16] = (Sint8)entity->sizex;
	net_packet->data[17] = (Sint8)entity->sizey;
	net_packet->data[18] = (Uint8)(entity->scalex * 128);
	net_packet->data[19] = (Uint8)(entity->scaley * 128);
	net_packet->data[20] = (Uint8)(entity->scalez * 128);
	SDLNet_Write16((Sint16)(entity->yaw * 256), &net_packet->data[21]);
	SDLNet_Write16((Sint16)(entity->pitch * 256), &net_packet->data[23]);
	SDLNet_Write16((Sint16)(entity->roll * 256), &net_packet->data[25]);
	net_packet->data[27] = (char)(entity->focalx * 8);
	net_packet->data[28] = (char)(entity->focaly * 8);
	net_packet->data[29] = (char)(entity->focalz * 8);
	SDLNet_Write32(entity->skill[2], &net_packet->data[30]);
	net_packet->data[34] = 0;
	net_packet->data[35] = 0;
	for (j = 0; j < 16; j++)
	{
		if ( entity->flags[j] )
		{
			net_packet->data[34 + j / 8] |= power(2, j - (j / 8) * 8);
		}
	}
	SDLNet_Write32((Uint32)ticks, &net_packet->data[36]);
	SDLNet_Write16((Sint16)(entity->vel_x * 32), &net_packet->data[40]);
	SDLNet_Write16((Sint16)(entity->vel_y * 32), &net_packet->data[42]);
	SDLNet_Write16((Sint16)(entity->vel_z * 32), &net_packet->data[44]);
	net_packet->address.host = net_clients[c - 1].host;
	net_packet->address.port = net_clients[c - 1].port;
	net_packet->len = ENTITY_PACKET_LENGTH;

	// sometimes you want more insurance that the entity update arrives
	if ( guarantee )
	{
		sendPacketSafe(net_sock, -1, net_packet, c - 1);
	}
	else
	{
		sendPacket(net_sock, -1, net_packet, c - 1);
	}
	if ( entity->clientsHaveItsStats )
	{
		entity->serverUpdateEffectsForEntity(false);
	}
}

/*-------------------------------------------------------------------------------

	sendMapSeedTCP

	Sends the seed necessary to generate the next map

-------------------------------------------------------------------------------*/

void sendMapSeedTCP(int c)
{
	if ( client_disconnected[c] == true )
	{
		return;
	}

	SDLNet_Write32(mapseed, &net_packet->data[0]);
	net_packet->address.host = net_clients[c - 1].host;
	net_packet->address.port = net_clients[c - 1].port;
	net_packet->len = 4;
	if ( directConnect )
	{
		SDLNet_TCP_Send(net_tcpclients[c - 1], net_packet->data, net_packet->len);
	}
	else
	{
#ifdef STEAMWORKS
		SteamNetworking()->SendP2PPacket(*static_cast<CSteamID* >(steamIDRemote[c - 1 ]), net_packet->data, net_packet->len, k_EP2PSendReliable, 0);
#endif
	}
	SDL_Delay(50);
}

/*-------------------------------------------------------------------------------

	sendMapTCP

	Sends all map data to clients

-------------------------------------------------------------------------------*/

void sendMapTCP(int c)
{
	Uint32 x, y, z;

	if ( client_disconnected[c] == true )
	{
		return;
	}

	// send all the map data to the client
	for ( z = 0; z < MAPLAYERS; z++ )
	{
		for ( y = 0; y < map.height; y++ )
		{
			for ( x = 0; x < map.width; x++ )
			{
				net_packet->data[x] = map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height];
			}
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = map.width;
			if ( directConnect )
			{
				SDLNet_TCP_Send(net_tcpclients[c - 1], net_packet->data, net_packet->len);
			}
			else
			{
#ifdef STEAMWORKS
				SteamNetworking()->SendP2PPacket(*static_cast<CSteamID* >(steamIDRemote[c - 1]), net_packet->data, net_packet->len, k_EP2PSendReliable, 0);
#endif
			}
			SDL_Delay(50);
		}
	}
}

/*-------------------------------------------------------------------------------

	serverUpdateBodypartIDs

	Updates the uid numbers of all the given bodyparts for the given entity

-------------------------------------------------------------------------------*/

void serverUpdateBodypartIDs(Entity* entity)
{
	int c;
	if ( multiplayer != SERVER )
	{
		return;
	}
	for ( c = 1; c < MAXPLAYERS; c++ )
	{
		if ( !client_disconnected[c] )
		{
			strcpy((char*)net_packet->data, "BDYI");
			SDLNet_Write32(entity->getUID(), &net_packet->data[4]);
			node_t* node;
			int i;
			for ( i = 0, node = entity->children.first; node != NULL; node = node->next, i++ )
			{
				if ( i < 1 || (i < 2 && entity->behavior == &actMonster) )
				{
					continue;
				}
				Entity* tempEntity = (Entity*)node->element;
				if ( entity->behavior == &actMonster )
				{
					SDLNet_Write32(tempEntity->getUID(), &net_packet->data[8 + 4 * (i - 2)]);
				}
				else
				{
					SDLNet_Write32(tempEntity->getUID(), &net_packet->data[8 + 4 * (i - 1)]);
				}
			}
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 8 + (list_Size(&entity->children) - 2) * 4;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}
}

/*-------------------------------------------------------------------------------

	serverUpdateEntityBodypart

	Updates the given bodypart of the given entity for all clients

-------------------------------------------------------------------------------*/

void serverUpdateEntityBodypart(Entity* entity, int bodypart)
{
	int c;
	if ( multiplayer != SERVER )
	{
		return;
	}
	for ( c = 1; c < MAXPLAYERS; c++ )
	{
		if ( !client_disconnected[c] )
		{
			strcpy((char*)net_packet->data, "ENTB");
			SDLNet_Write32(entity->getUID(), &net_packet->data[4]);
			net_packet->data[8] = bodypart;
			node_t* node = list_Node(&entity->children, bodypart);
			if ( node )
			{
				Entity* tempEntity = (Entity*)node->element;
				SDLNet_Write32(tempEntity->sprite, &net_packet->data[9]);
				net_packet->data[13] = tempEntity->flags[INVISIBLE];
				net_packet->address.host = net_clients[c - 1].host;
				net_packet->address.port = net_clients[c - 1].port;
				net_packet->len = 14;
				sendPacketSafe(net_sock, -1, net_packet, c - 1);
			}
		}
	}
}

/*-------------------------------------------------------------------------------

	serverUpdateEntitySprite

	Updates the given entity's sprite for all clients

-------------------------------------------------------------------------------*/

void serverUpdateEntitySprite(Entity* entity)
{
	int c;
	if ( multiplayer != SERVER )
	{
		return;
	}
	for ( c = 1; c < MAXPLAYERS; c++ )
	{
		if ( !client_disconnected[c] )
		{
			strcpy((char*)net_packet->data, "ENTA");
			SDLNet_Write32(entity->getUID(), &net_packet->data[4]);
			SDLNet_Write32(entity->sprite, &net_packet->data[8]);
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 12;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}
}

/*-------------------------------------------------------------------------------

	serverUpdateEntitySkill

	Updates a specific entity skill for all clients

-------------------------------------------------------------------------------*/

void serverUpdateEntitySkill(Entity* entity, int skill)
{
	int c;
	if ( multiplayer != SERVER )
	{
		return;
	}
	for ( c = 1; c < MAXPLAYERS; c++ )
	{
		if ( !client_disconnected[c] )
		{
			strcpy((char*)net_packet->data, "ENTS");
			SDLNet_Write32(entity->getUID(), &net_packet->data[4]);
			net_packet->data[8] = skill;
			SDLNet_Write32(entity->skill[skill], &net_packet->data[9]);
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 13;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}
}

/*-------------------------------------------------------------------------------

serverUpdateEntityFSkill

Updates a specific entity fskill for all clients

-------------------------------------------------------------------------------*/

void serverUpdateEntityFSkill(Entity* entity, int fskill)
{
	int c;
	if ( multiplayer != SERVER )
	{
		return;
	}
	for ( c = 1; c < MAXPLAYERS; c++ )
	{
		if ( !client_disconnected[c] )
		{
			strcpy((char*)net_packet->data, "ENFS");
			SDLNet_Write32(entity->getUID(), &net_packet->data[4]);
			net_packet->data[8] = fskill;
			SDLNet_Write32(static_cast<int>(entity->fskill[fskill]), &net_packet->data[9]);
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 13;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}
}

/*-------------------------------------------------------------------------------

serverSpawnMiscParticles

Spawns misc particle effects for all clients

-------------------------------------------------------------------------------*/

void serverSpawnMiscParticles(Entity* entity, int particleType)
{
	int c;
	if ( multiplayer != SERVER )
	{
		return;
	}
	for ( c = 1; c < MAXPLAYERS; c++ )
	{
		if ( !client_disconnected[c] )
		{
			strcpy((char*)net_packet->data, "SPPE");
			SDLNet_Write32(entity->getUID(), &net_packet->data[4]);
			net_packet->data[8] = particleType;
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 10;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}
}

/*-------------------------------------------------------------------------------

	serverUpdateEntityFlag

	Updates a specific entity flag for all clients

-------------------------------------------------------------------------------*/

void serverUpdateEntityFlag(Entity* entity, int flag)
{
	int c;
	if ( multiplayer != SERVER )
	{
		return;
	}
	for ( c = 1; c < MAXPLAYERS; c++ )
	{
		if ( !client_disconnected[c] )
		{
			strcpy((char*)net_packet->data, "ENTF");
			SDLNet_Write32(entity->getUID(), &net_packet->data[4]);
			net_packet->data[8] = flag;
			net_packet->data[9] = entity->flags[flag];
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 10;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}
}

/*-------------------------------------------------------------------------------

	serverUpdateEffects

	Updates the status of the EFFECTS variables (blindness, drunkenness, etc.)
	for the specified client.

-------------------------------------------------------------------------------*/

void serverUpdateEffects(int player)
{
	int j;

	if ( multiplayer != SERVER || clientnum == player )
	{
		return;
	}
	if ( player < 0 || player >= MAXPLAYERS )
	{
		return;
	}
	if ( client_disconnected[player] == true )
	{
		return;
	}

	strcpy((char*)net_packet->data, "UPEF");
	net_packet->data[4] = 0;
	net_packet->data[5] = 0;
	net_packet->data[6] = 0;
	net_packet->data[7] = 0;
	for (j = 0; j < NUMEFFECTS; j++)
	{
		if ( stats[player]->EFFECTS[j] == true )
		{
			net_packet->data[4 + j / 8] |= power(2, j - (j / 8) * 8);
		}
	}
	net_packet->address.host = net_clients[player - 1].host;
	net_packet->address.port = net_clients[player - 1].port;
	net_packet->len = 8;
	sendPacketSafe(net_sock, -1, net_packet, player - 1);
}

/*-------------------------------------------------------------------------------

	serverUpdateHunger

	Updates the HUNGER variable for the specified client

-------------------------------------------------------------------------------*/

void serverUpdateHunger(int player)
{
	if ( multiplayer != SERVER || clientnum == player )
	{
		return;
	}
	if ( player < 0 || player >= MAXPLAYERS )
	{
		return;
	}
	if ( client_disconnected[player] == true )
	{
		return;
	}

	strcpy((char*)net_packet->data, "HNGR");
	SDLNet_Write32(stats[player]->HUNGER, &net_packet->data[4]);
	net_packet->address.host = net_clients[player - 1].host;
	net_packet->address.port = net_clients[player - 1].port;
	net_packet->len = 8;
	sendPacketSafe(net_sock, -1, net_packet, player - 1);
}

/*-------------------------------------------------------------------------------

	receiveEntity

	receives entity data from server

-------------------------------------------------------------------------------*/

Entity* receiveEntity(Entity* entity)
{
	bool newentity = false;
	int c;

	//TODO: Find out if this is needed.
	/*bool oldeffects[NUMEFFECTS];
	Stat* entityStats = entity->getStats();

	for ( int i = 0; i < NUMEFFECTS; ++i )
	{
		if ( !entityStats )
		{
			oldeffects[i] = 0;
		}
		else
		{
			oldeffects[i] = entityStats->EFFECTS[i];
		}
	}
	//Yes, it is necessary. I don't think I like this solution though, will try something else.
	*/

	if ( entity == NULL )
	{
		newentity = true;
		entity = newEntity((int)SDLNet_Read16(&net_packet->data[8]), 0, map.entities);
	}
	else
	{
		entity->sprite = (int)SDLNet_Read16(&net_packet->data[8]);
	}
	entity->lastupdate = ticks;
	entity->lastupdateserver = (Uint32)SDLNet_Read32(&net_packet->data[36]);
	entity->setUID((int)SDLNet_Read32(&net_packet->data[4])); // remember who I am
	entity->new_x = ((Sint16)SDLNet_Read16(&net_packet->data[10])) / 32.0;
	entity->new_y = ((Sint16)SDLNet_Read16(&net_packet->data[12])) / 32.0;
	entity->new_z = ((Sint16)SDLNet_Read16(&net_packet->data[14])) / 32.0;
	entity->sizex = (Sint8)net_packet->data[16];
	entity->sizey = (Sint8)net_packet->data[17];
	entity->scalex = ((Uint8)net_packet->data[18]) / 128.f;
	entity->scaley = ((Uint8)net_packet->data[19]) / 128.f;
	entity->scalez = ((Uint8)net_packet->data[20]) / 128.f;
	entity->new_yaw = ((Sint16)SDLNet_Read16(&net_packet->data[21])) / 256.0;
	entity->new_pitch = ((Sint16)SDLNet_Read16(&net_packet->data[23])) / 256.0;
	entity->new_roll = ((Sint16)SDLNet_Read16(&net_packet->data[25])) / 256.0;
	if ( newentity )
	{
		entity->x = entity->new_x;
		entity->y = entity->new_y;
		entity->z = entity->new_z;
		entity->yaw = entity->new_yaw;
		entity->pitch = entity->new_pitch;
		entity->roll = entity->new_roll;
	}
	entity->focalx = ((char)net_packet->data[27]) / 8.0;
	entity->focaly = ((char)net_packet->data[28]) / 8.0;
	entity->focalz = ((char)net_packet->data[29]) / 8.0;
	for (c = 0; c < 16; c++)
	{
		if ( net_packet->data[34 + c / 8]&power(2, c - (c / 8) * 8) )
		{
			entity->flags[c] = true;
		}
	}
	entity->vel_x = ((Sint16)SDLNet_Read16(&net_packet->data[40])) / 32.0;
	entity->vel_y = ((Sint16)SDLNet_Read16(&net_packet->data[42])) / 32.0;
	entity->vel_z = ((Sint16)SDLNet_Read16(&net_packet->data[44])) / 32.0;

	return entity;
}

/*-------------------------------------------------------------------------------

	clientActions

	Assigns an action to a given entity based mainly on its sprite

-------------------------------------------------------------------------------*/

void clientActions(Entity* entity)
{
	int playernum;

	// this code assigns behaviors based on the sprite (model) number
	switch ( entity->sprite )
	{
		case 1:
			entity->behavior = &actDoorFrame;
			break;
		case 2:
			entity->behavior = &actDoor;
			break;
		case 3:
			entity->behavior = &actTorch;
			entity->flags[NOUPDATE] = 1;
			break;
		case 113:
		case 114:
		case 115:
		case 116:
		case 117:
		case 125:
		case 126:
		case 127:
		case 128:
		case 129:
		case 332:
		case 333:
		case 341:
		case 342:
		case 343:
		case 344:
		case 345:
		case 346:
		case 354:
		case 355:
		case 356:
		case 357:
		case 358:
		case 359:
		case 367:
		case 368:
		case 369:
		case 370:
		case 371:
		case 372:
		case 380:
		case 381:
		case 382:
		case 383:
		case 384:
		case 385:
			// these are all human heads
			playernum = SDLNet_Read32(&net_packet->data[30]);
			if ( playernum >= 0 && playernum < MAXPLAYERS )
			{
				players[playernum]->entity = entity;
				entity->skill[2] = playernum;
				entity->behavior = &actPlayer;
			}
			break;
		case 160:
		case 203:
		case 212:
		case 213:
		case 214:
			entity->flags[NOUPDATE] = true;
			break;
		case 162:
			entity->behavior = &actCampfire;
			entity->flags[NOUPDATE] = true;
			break;
		case 163:
			entity->skill[2] = (int)SDLNet_Read32(&net_packet->data[30]);
			entity->behavior = &actFountain;
			break;
		case 174:
			if (SDLNet_Read32(&net_packet->data[30]) != 0)
			{
				entity->behavior = &actMagiclightBall; //TODO: Finish this here. I think this gets reassigned every time the entity is recieved? Make sure.
			}
			break;
		case 185:
			entity->behavior = &actSwitch;
			break;
		case 186:
			entity->behavior = &actGate;
			break;
		case 216:
			entity->behavior = &actChestLid;
			break;
		case 254:
		case 255:
		case 256:
		case 257:
			entity->behavior = &actPortal;
			break;
		case 273:
			entity->behavior = &actMCaxe;
			break;
		case 278:
		case 279:
		case 280:
		case 281:
			entity->behavior = &actWinningPortal;
			break;
		case 282:
			entity->behavior = &actSpearTrap;
			break;
		case 578:
			entity->behavior = &actPowerCrystal;
			break;
		default:
			break;
	}

	// if the above method failed, we check the value of skill[2] (stored in net_packet->data[30]) and assign an action based on that
	if ( entity->behavior == NULL )
	{
		int c = (int)SDLNet_Read32(&net_packet->data[30]);
		if ( c < 0 )
		{
			switch ( c )
			{
				case -4:
					entity->behavior = &actMonster;
					break;
				case -5:
					entity->behavior = &actItem;
					break;
				case -6:
					entity->behavior = &actGib;
					break;
				case -7:
					entity->behavior = &actEmpty;
					break;
				case -8:
					entity->behavior = &actThrown;
					break;
				case -9:
					entity->behavior = &actLiquid;
					break;
				case -10:
					entity->behavior = &actMagiclightBall;
					break;
				case -11:
					entity->behavior = &actMagicClient;
					break;
				case -12:
					entity->behavior = &actMagicClientNoLight;
					break;
				default:
					break;
			}
		}
	}
}

/*-------------------------------------------------------------------------------

	clientHandlePacket

	Called by clientHandleMessages. Does the actual handling of a packet.

-------------------------------------------------------------------------------*/

void clientHandlePacket()
{
	if (handleSafePacket())
	{
		return;
	}

	Uint32 x, y;
	node_t* node;
	node_t* nextnode;
	Entity* entity, *entity2;
	int c = 0;
	Uint32 i = 0, j;
	Item* item = NULL;
	FILE* fp;

#ifdef PACKETINFO
	char packetinfo[NET_PACKET_SIZE];
	strncpy( packetinfo, (char*)net_packet->data, net_packet->len );
	packetinfo[net_packet->len] = 0;
	printlog("info: client packet: %s\n", packetinfo);
#endif

	// keep alive
	if (!strncmp((char*)net_packet->data, "KPAL", 4))
	{
		client_keepalive[0] = ticks;
		return;
	}

	// ping
	else if (!strncmp((char*)net_packet->data, "PING", 4))
	{
		messagePlayer(clientnum, language[1117], (SDL_GetTicks() - pingtime));
		return;
	}

	// unlock steam achievement
	else if (!strncmp((char*)net_packet->data, "SACH", 4))
	{
		steamAchievement((char*)(&net_packet->data[4]));
		return;
	}

	// pause game
	else if (!strncmp((char*)net_packet->data, "PAUS", 4))
	{
		messagePlayer(clientnum, language[1118], stats[net_packet->data[4]]->name);
		pauseGame(2, 0);
		return;
	}

	// unpause game
	else if (!strncmp((char*)net_packet->data, "UNPS", 4))
	{
		messagePlayer(clientnum, language[1119], stats[net_packet->data[4]]->name);
		pauseGame(1, 0);
		return;
	}

	// server or player shut down
	else if (!strncmp((char*)net_packet->data, "DISCONNECT", 10))
	{
		if ( net_packet->data[10] == 0 )
		{
			// server shutdown
			if ( !victory )
			{
				button_t* button;

				printlog("The remote server has shut down.\n");
				pauseGame(2, 0);

				// close current window
				buttonCloseSubwindow(NULL);
				for ( node = button_l.first; node != NULL; node = nextnode )
				{
					nextnode = node->next;
					button = (button_t*)node->element;
					if ( button->focused )
					{
						list_RemoveNode(button->node);
					}
				}

				// create new window
				subwindow = 1;
				subx1 = xres / 2 - 256;
				subx2 = xres / 2 + 256;
				suby1 = yres / 2 - 56;
				suby2 = yres / 2 + 56;
				strcpy(subtext, language[1126]);

				// close button
				button = newButton();
				strcpy(button->label, "x");
				button->x = subx2 - 20;
				button->y = suby1;
				button->sizex = 20;
				button->sizey = 20;
				button->action = &buttonCloseAndEndGameConfirm;
				button->visible = 1;
				button->focused = 1;

				// okay button
				button = newButton();
				strcpy(button->label, language[732]);
				button->x = subx2 - (subx2 - subx1) / 2 - 28;
				button->y = suby2 - 28;
				button->sizex = 56;
				button->sizey = 20;
				button->action = &buttonCloseAndEndGameConfirm;
				button->visible = 1;
				button->focused = 1;
				button->key = SDL_SCANCODE_RETURN;
			}
		}
		client_disconnected[net_packet->data[10]] = true;
		return;
	}

	// player movement correction
	else if (!strncmp((char*)net_packet->data, "PMOV", 4))
	{
		if (players[clientnum] == nullptr || players[clientnum]->entity == nullptr)
		{
			return;
		}
		players[clientnum]->entity->x = ((Sint16)SDLNet_Read16(&net_packet->data[4])) / 32.0;
		players[clientnum]->entity->y = ((Sint16)SDLNet_Read16(&net_packet->data[6])) / 32.0;
		return;
	}

	// teleport player
	else if (!strncmp((char*)net_packet->data, "TELE", 4))
	{
		if (players[clientnum] == nullptr || players[clientnum]->entity == nullptr)
		{
			return;
		}
		x = net_packet->data[4];
		y = net_packet->data[5];
		players[clientnum]->entity->x = x;
		players[clientnum]->entity->y = y;
		return;
	}

	// delete entity
	else if (!strncmp((char*)net_packet->data, "ENTD", 4))
	{
		for ( node = map.entities->first; node != NULL; node = nextnode )
		{
			nextnode = node->next;
			entity = (Entity*)node->element;
			if ( entity->getUID() == (int)SDLNet_Read32(&net_packet->data[4]) )
			{
				entity2 = newEntity(entity->sprite, 1, &removedEntities);
				entity2->setUID(entity->getUID());
				for ( j = 0; j < MAXPLAYERS; j++ )
					if (entity == players[j]->entity )
					{
						players[j]->entity = NULL;
					}
				if ( entity->light )
				{
					list_RemoveNode(entity->light->node);
					entity->light = NULL;
				}
				list_RemoveNode(entity->mynode);

				// inform the server that we deleted the entity
				strcpy((char*)net_packet->data, "ENTD");
				net_packet->data[4] = clientnum;
				SDLNet_Write32(entity2->getUID(), &net_packet->data[5]);
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 9;
				sendPacket(net_sock, -1, net_packet, 0);
				break;
			}
		}
		return;
	}

	// shake screen
	else if (!strncmp((char*)net_packet->data, "SHAK", 4))
	{
		camera_shakex += ((char)(net_packet->data[4])) / 100.f;
		camera_shakey += ((char)(net_packet->data[5]));
		return;
	}

	// update armor quality
	else if (!strncmp((char*)net_packet->data, "ARMR", 4))
	{
		switch ( net_packet->data[4] )
		{
			case 0:
				item = stats[clientnum]->helmet;
				break;
			case 1:
				item = stats[clientnum]->breastplate;
				break;
			case 2:
				item = stats[clientnum]->gloves;
				break;
			case 3:
				item = stats[clientnum]->shoes;
				break;
			case 4:
				item = stats[clientnum]->shield;
				break;
			case 5:
				item = stats[clientnum]->weapon;
				break;
			case 6:
				item = stats[clientnum]->cloak;
				break;
			case 7:
				item = stats[clientnum]->amulet;
				break;
			case 8:
				item = stats[clientnum]->ring;
				break;
			case 9:
				item = stats[clientnum]->mask;
				break;
			default:
				item = NULL;
				break;
		}
		if ( item != NULL )
		{
			if ( item->count > 1 )
			{
				newItem(item->type, item->status, item->beatitude, item->count - 1, item->appearance, item->identified, &stats[clientnum]->inventory);
				item->count = 1;
			}
			item->status = static_cast<Status>(net_packet->data[5]);
		}
		return;
	}

	// steal armor (destroy it)
	else if (!strncmp((char*)net_packet->data, "STLA", 4))
	{
		switch ( net_packet->data[4] )
		{
			case 0:
				item = stats[clientnum]->helmet;
				break;
			case 1:
				item = stats[clientnum]->breastplate;
				break;
			case 2:
				item = stats[clientnum]->gloves;
				break;
			case 3:
				item = stats[clientnum]->shoes;
				break;
			case 4:
				item = stats[clientnum]->shield;
				break;
			case 5:
				item = stats[clientnum]->weapon;
				break;
			case 6:
				item = stats[clientnum]->cloak;
				break;
			case 7:
				item = stats[clientnum]->amulet;
				break;
			case 8:
				item = stats[clientnum]->ring;
				break;
			case 9:
				item = stats[clientnum]->mask;
				break;
			default:
				item = NULL;
				break;
		}
		Item** slot = itemSlot(stats[clientnum], item);
		if ( slot != NULL )
		{
			*slot = NULL;
		}
		if ( item )
		{
			list_RemoveNode(item->node);
		}
		return;
	}

	// enemy hp bar
	else if (!strncmp((char*)net_packet->data, "ENHP", 4))
	{
		enemy_hp = SDLNet_Read32(&net_packet->data[4]);
		enemy_maxhp = SDLNet_Read32(&net_packet->data[8]);
		enemy_timer = ticks;
		strcpy(enemy_name, (char*)(&net_packet->data[12]));
		return;
	}

	// damage indicator
	else if (!strncmp((char*)net_packet->data, "DAMI", 4))
	{
		newDamageIndicator(SDLNet_Read32(&net_packet->data[4]), SDLNet_Read32(&net_packet->data[8]));
		return;
	}

	// play sound position
	else if (!strncmp((char*)net_packet->data, "SNDP", 4))
	{
		playSoundPos(SDLNet_Read32(&net_packet->data[4]), SDLNet_Read32(&net_packet->data[8]), SDLNet_Read32(&net_packet->data[12]), SDLNet_Read32(&net_packet->data[16]));
		return;
	}

	// play sound global
	else if (!strncmp((char*)net_packet->data, "SNDG", 4))
	{
		playSound(SDLNet_Read32(&net_packet->data[4]), SDLNet_Read32(&net_packet->data[8]));
		return;
	}

	// new light, shadowed
	else if (!strncmp((char*)net_packet->data, "LITS", 4))
	{
		lightSphereShadow(SDLNet_Read16(&net_packet->data[4]), SDLNet_Read16(&net_packet->data[6]), SDLNet_Read16(&net_packet->data[8]), SDLNet_Read16(&net_packet->data[10]));
		return;
	}

	// new light, unshadowed
	else if (!strncmp((char*)net_packet->data, "LITU", 4))
	{
		lightSphere(SDLNet_Read16(&net_packet->data[4]), SDLNet_Read16(&net_packet->data[6]), SDLNet_Read16(&net_packet->data[8]), SDLNet_Read16(&net_packet->data[10]));
		return;
	}

	// create wall
	else if (!strncmp((char*)net_packet->data, "WALC", 4))
	{
		int y = SDLNet_Read16(&net_packet->data[6]);
		int x = SDLNet_Read16(&net_packet->data[4]);
		if ( x >= 0 && x < map.width && y >= 0 && y < map.height )
		{
			map.tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map.height] = map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height];
		}
		return;
	}

	// destroy wall
	else if (!strncmp((char*)net_packet->data, "WALD", 4))
	{
		int y = SDLNet_Read16(&net_packet->data[6]);
		int x = SDLNet_Read16(&net_packet->data[4]);
		if ( x >= 0 && x < map.width && y >= 0 && y < map.height )
		{
			map.tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map.height] = 0;
		}
		return;
	}

	// destroy wall + ceiling
	else if (!strncmp((char*)net_packet->data, "WACD", 4))
	{
		int y = SDLNet_Read16(&net_packet->data[6]);
		int x = SDLNet_Read16(&net_packet->data[4]);
		if ( x >= 0 && x < map.width && y >= 0 && y < map.height )
		{
			map.tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map.height] = 0;
			map.tiles[(MAPLAYERS - 1) + y * MAPLAYERS + x * MAPLAYERS * map.height] = 0;
		}
		return;
	}

	// monster music
	else if (!strncmp((char*)net_packet->data, "MUSM", 3))
	{
		combat = (bool)net_packet->data[3];
		return;
	}

	// get item
	else if (!strncmp((char*)net_packet->data, "ITEM", 4))
	{
		item = newItem(static_cast<ItemType>(SDLNet_Read32(&net_packet->data[4])), static_cast<Status>(SDLNet_Read32(&net_packet->data[8])), SDLNet_Read32(&net_packet->data[12]), SDLNet_Read32(&net_packet->data[16]), SDLNet_Read32(&net_packet->data[20]), net_packet->data[24], NULL);
		itemPickup(clientnum, item);
		free(item);
		return;
	}

	// unequip and remove item
	else if (!strncmp((char*)net_packet->data, "DROP", 4))
	{
		Item** armor = NULL;
		switch ( net_packet->data[4] )
		{
			case 0:
				armor = &stats[clientnum]->helmet;
				break;
			case 1:
				armor = &stats[clientnum]->breastplate;
				break;
			case 2:
				armor = &stats[clientnum]->gloves;
				break;
			case 3:
				armor = &stats[clientnum]->shoes;
				break;
			case 4:
				armor = &stats[clientnum]->shield;
				break;
			case 5:
				armor = &stats[clientnum]->weapon;
				break;
			case 6:
				armor = &stats[clientnum]->cloak;
				break;
			case 7:
				armor = &stats[clientnum]->amulet;
				break;
			case 8:
				armor = &stats[clientnum]->ring;
				break;
			case 9:
				armor = &stats[clientnum]->mask;
				break;
		}
		if ( !armor )
		{
			return;
		}
		if ( !(*armor) )
		{
			return;
		}
		if ( (*armor)->count > 1 )
		{
			(*armor)->count--;
		}
		else
		{
			if ( (*armor)->node )
			{
				list_RemoveNode((*armor)->node);
			}
			else
			{
				free(*armor);
			}
		}
		*armor = NULL;
		return;
	}

	// get gold
	else if (!strncmp((char*)net_packet->data, "GOLD", 4))
	{
		stats[clientnum]->GOLD = SDLNet_Read32(&net_packet->data[4]);
		return;
	}

	// open shop
	else if (!strncmp((char*)net_packet->data, "SHOP", 4))
	{
		shopkeeper = (Uint32)SDLNet_Read32(&net_packet->data[4]);
		shopkeepertype = net_packet->data[8];
		strcpy( shopkeepername_client, (char*)(&net_packet->data[9]) );
		shopkeepername = shopkeepername_client;
		shootmode = false;
		gui_mode = GUI_MODE_SHOP;
		shoptimer = ticks - 1;
		shopspeech = language[194 + rand() % 3];
		shopinventorycategory = 7;
		sellitem = NULL;
		shopitemscroll = 0;
		identifygui_active = false; //Really need a centralized function to open up whatever screen/inventory.
		closeRemoveCurseGUI();

		//Initialize shop gamepad code here.
		if ( shopinvitems[0] != nullptr )
		{
			selectedShopSlot = 0;
			warpMouseToSelectedShopSlot();
		}
		else
		{
			selectedShopSlot = -1;
		}

		return;
	}

	// shop item
	else if (!strncmp((char*)net_packet->data, "SHPI", 4))
	{
		if ( !shopInv )
		{
			return;
		}
		newItem(static_cast<ItemType>(SDLNet_Read32(&net_packet->data[4])), static_cast<Status>((char)net_packet->data[8]), (char)net_packet->data[9], (unsigned char)net_packet->data[10], SDLNet_Read32(&net_packet->data[11]), (bool)net_packet->data[15], shopInv);
	}

	// close shop
	else if (!strncmp((char*)net_packet->data, "SHPC", 4))
	{
		gui_mode = GUI_MODE_INVENTORY;
		shootmode = true;
		shopkeeper = 0;
		list_FreeAll(shopInv);
		//Clean up shop gamepad code here.
		selectedShopSlot = -1;
		return;
	}

	// textbox message
	else if (!strncmp((char*)net_packet->data, "MSGS", 4))
	{
		if ( ticks != 1 )
		{
			Uint32 color = SDLNet_Read32(&net_packet->data[4]);
			messagePlayerColor(clientnum, color, (char*)(&net_packet->data[8]));
		}
		for ( c = 0; c < MAXPLAYERS; c++ )
		{
			if ( !strncmp( (char*)(&net_packet->data[8]), stats[c]->name, std::min<size_t>(strlen(stats[c]->name), 10) ) )    //TODO: Why are size_t and int being compared?
			{
				if ( net_packet->data[8 + std::min<size_t>(strlen(stats[c]->name), 10)] == ':' )   //TODO: Why are size_t and int being compared?
				{
					playSound(238, 64);
				}
			}
		}
		if ( !strcmp((char*)(&net_packet->data[8]), language[577]) )    //TODO: Replace with a UDIE packet.
		{
			// this is how the client knows it died...
			Entity* entity = newEntity(-1, 1, map.entities);
			entity->x = camera.x * 16;
			entity->y = camera.y * 16;
			entity->z = -2;
			entity->flags[NOUPDATE] = true;
			entity->flags[PASSABLE] = true;
			entity->flags[INVISIBLE] = true;
			entity->behavior = &actDeathCam;
			entity->yaw = camera.ang;
			entity->pitch = PI / 8;

			deleteSaveGame(); // stops save scumming c:

			closeBookGUI();

#ifdef SOUND
			levelmusicplaying = true;
			combatmusicplaying = false;
			fadein_increment = default_fadein_increment * 4;
			fadeout_increment = default_fadeout_increment * 4;
			playmusic( sounds[209], false, true, false );
#endif
			combat = false;

			for ( node = stats[clientnum]->inventory.first; node != NULL; node = nextnode )
			{
				nextnode = node->next;
				Item* item = (Item*)node->element;
				if ( itemCategory(item) == SPELL_CAT )
				{
					continue;    // don't drop spells on death, stupid!
				}
				if ( itemIsEquipped(item, clientnum) )
				{
					Item** slot = itemSlot(stats[clientnum], item);
					if ( slot != NULL )
					{
						*slot = NULL;
					}
					list_RemoveNode(node);
					continue;
				}
				strcpy((char*)net_packet->data, "DIEI");
				SDLNet_Write32((Uint32)item->type, &net_packet->data[4]);
				SDLNet_Write32((Uint32)item->status, &net_packet->data[8]);
				SDLNet_Write32((Uint32)item->beatitude, &net_packet->data[12]);
				SDLNet_Write32((Uint32)item->count, &net_packet->data[16]);
				SDLNet_Write32((Uint32)item->appearance, &net_packet->data[20]);
				net_packet->data[24] = item->identified;
				net_packet->data[25] = clientnum;
				net_packet->data[26] = (Uint8)camera.x;
				net_packet->data[27] = (Uint8)camera.y;
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 28;
				sendPacketSafe(net_sock, -1, net_packet, 0);
				list_RemoveNode(node);
			}
			stats[clientnum]->helmet = NULL;
			stats[clientnum]->breastplate = NULL;
			stats[clientnum]->gloves = NULL;
			stats[clientnum]->shoes = NULL;
			stats[clientnum]->shield = NULL;
			stats[clientnum]->weapon = NULL;
			stats[clientnum]->cloak = NULL;
			stats[clientnum]->amulet = NULL;
			stats[clientnum]->ring = NULL;
			stats[clientnum]->mask = NULL;
		}
		else if ( !strcmp((char*)(&net_packet->data[8]), language[1109]) )
		{
			// ... or lived
			stats[clientnum]->HP = stats[clientnum]->MAXHP / 2;
			stats[clientnum]->HUNGER = 500;
			for ( c = 0; c < NUMEFFECTS; c++ )
			{
				stats[clientnum]->EFFECTS[c] = false;
				stats[clientnum]->EFFECTS_TIMERS[c] = 0;
			}
		}
		else if ( !strncmp((char*)(&net_packet->data[8]), language[1114], 28) )
		{
#ifdef MUSIC
			fadein_increment = default_fadein_increment * 20;
			fadeout_increment = default_fadeout_increment * 5;
			playmusic( sounds[175], false, true, false );
#endif
		}
		else if ( (strstr((char*)(&net_packet->data[8]), language[1160])) != NULL )
		{
			for ( c = 0; c < MAXPLAYERS; c++ )
			{
				if ( !strncmp(stats[c]->name, (char*)(&net_packet->data[8]), strlen(stats[c]->name)) )
				{
					if (players[clientnum] && players[clientnum]->entity && players[c] && players[c]->entity)
					{
						double tangent = atan2(players[clientnum]->entity->y - players[c]->entity->y, players[clientnum]->entity->x - players[c]->entity->x);
						players[clientnum]->entity->vel_x += cos(tangent);
						players[clientnum]->entity->vel_y += sin(tangent);
					}
					break;
				}
			}
		}
		return;
	}

	// update health
	else if (!strncmp((char*)net_packet->data, "UPHP", 4))
	{
		if ( (Monster)SDLNet_Read32(&net_packet->data[8]) != NOTHING )
		{
			if ( SDLNet_Read32(&net_packet->data[4]) < stats[clientnum]->HP )
			{
				camera_shakex += .1;
				camera_shakey += 10;
			}
			else
			{
				camera_shakex += .05;
				camera_shakey += 5;
			}
		}
		stats[clientnum]->HP = SDLNet_Read32(&net_packet->data[4]);
		return;
	}

	// update magic
	else if (!strncmp((char*)net_packet->data, "UPMP", 4))
	{
		stats[clientnum]->MP = SDLNet_Read32(&net_packet->data[4]);
		return;
	}

	// update effects flags
	else if (!strncmp((char*)net_packet->data, "UPEF", 4))
	{
		for (c = 0; c < NUMEFFECTS; c++)
		{
			if ( net_packet->data[4 + c / 8]&power(2, c - (c / 8) * 8) )
			{
				stats[clientnum]->EFFECTS[c] = true;
			}
			else
			{
				stats[clientnum]->EFFECTS[c] = false;
			}
		}
		return;
	}

	// update attributes
	else if (!strncmp((char*)net_packet->data, "ATTR", 4))
	{
		stats[clientnum]->STR = (Sint8)net_packet->data[5];
		stats[clientnum]->DEX = (Sint8)net_packet->data[6];
		stats[clientnum]->CON = (Sint8)net_packet->data[7];
		stats[clientnum]->INT = (Sint8)net_packet->data[8];
		stats[clientnum]->PER = (Sint8)net_packet->data[9];
		stats[clientnum]->CHR = (Sint8)net_packet->data[10];
		stats[clientnum]->EXP = (Sint8)net_packet->data[11];
		stats[clientnum]->LVL = (Sint8)net_packet->data[12];
		stats[clientnum]->HP = (Sint16)SDLNet_Read16(&net_packet->data[13]);
		stats[clientnum]->MAXHP = (Sint16)SDLNet_Read16(&net_packet->data[15]);
		stats[clientnum]->MP = (Sint16)SDLNet_Read16(&net_packet->data[17]);
		stats[clientnum]->MAXMP = (Sint16)SDLNet_Read16(&net_packet->data[19]);
		return;
	}

	// killed a monster
	else if (!strncmp((char*)net_packet->data, "MKIL", 4))
	{
		kills[net_packet->data[4]]++;
		return;
	}

	// update skill
	else if (!strncmp((char*)net_packet->data, "SKIL", 4))
	{
		stats[clientnum]->PROFICIENCIES[net_packet->data[5]] = net_packet->data[6];

		int statBonusSkill = getStatForProficiency(net_packet->data[5]);

		if ( statBonusSkill >= STAT_STR )
		{
			// stat has chance for bonus point if the relevant proficiency has been trained.
			// write the last proficiency that effected the skill.
			stats[clientnum]->PLAYER_LVL_STAT_BONUS[statBonusSkill] = net_packet->data[5];
		}

		return;
	}

	//Add spell.
	else if ( !strncmp((char*)net_packet->data, "ASPL", 4) )
	{
		if ( net_packet->len != 6 ) //Need to get the actual length, not reported...Should be a generic check at the top of the function, if len != actual len, then abort.
		{
			printlog("Received malformed ASPL packet.");
			return;
		}

		addSpell(net_packet->data[5], clientnum);

		return;
	}

	// update hunger
	else if (!strncmp((char*)net_packet->data, "HNGR", 4))
	{
		stats[clientnum]->HUNGER = (Sint32)SDLNet_Read32(&net_packet->data[4]);
		return;
	}

	// current game level
	else if (!strncmp((char*)net_packet->data, "LVLC", 4))
	{
		if ( currentlevel == net_packet->data[13] )
		{
			// the server's just doing a routine check
			return;
		}

		// hack to fix these things from breaking everything...
		hudarm = NULL;
		hudweapon = NULL;
		magicLeftHand = NULL;
		magicRightHand = NULL;

		// stop all sounds
#ifdef HAVE_FMOD
		if ( sound_group )
		{
			FMOD_ChannelGroup_Stop(sound_group);
		}
#elif defined HAVE_OPENAL
		if ( sound_group )
		{
			OPENAL_ChannelGroup_Stop(sound_group);
		}
#endif

		// show loading message
#define LOADSTR language[709]
		loading = true;
		drawClearBuffers();
		int w, h;
		TTF_SizeUTF8(ttf16, LOADSTR, &w, &h);
		ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, LOADSTR);

		GO_SwapBuffers(screen);

		// unlock some steam achievements
		if ( !secretlevel )
		{
			switch ( currentlevel )
			{
				case 0:
					steamAchievement("BARONY_ACH_ENTER_THE_DUNGEON");
					break;
				case 4:
					steamAchievement("BARONY_ACH_TWISTY_PASSAGES");
					break;
				case 9:
					steamAchievement("BARONY_ACH_JUNGLE_FEVER");
					break;
				case 14:
					steamAchievement("BARONY_ACH_SANDMAN");
					break;

			}
		}

		// setup level change
		printlog("Received order to change level.\n");
		currentlevel = net_packet->data[13];
		list_FreeAll(&removedEntities);
		for ( node = map.entities->first; node != NULL; node = node->next )
		{
			entity = (Entity*)node->element;
			entity2 = newEntity(entity->sprite, 1, &removedEntities);
			entity2->setUID(entity->getUID());
		}
		for ( i = 0; i < MAXPLAYERS; i++ )
		{
			list_FreeAll(&stats[i]->FOLLOWERS);
		}

		// load next level
		darkmap = false;
		secretlevel = net_packet->data[4];
		mapseed = SDLNet_Read32(&net_packet->data[5]);
		int result = 0;
		/*Uint32 oldtime = SDL_GetTicks();
		while( SDLNet_TCP_Recv(net_tcpsock, net_packet->data, 4)!=4 ) {
			if( SDL_GetTicks()-oldtime>10000 )
				printlog("warning: game has taken more than 10 seconds to receive map seed\n");
		}*/
		numplayers = 0;
		entity_uids = (Uint32)SDLNet_Read32(&net_packet->data[9]);
		printlog("Received map seed: %d. Entity UID start: %d\n", mapseed, entity_uids);
		if ( !secretlevel )
		{
			fp = openDataFile(LEVELSFILE, "r");
		}
		else
		{
			fp = openDataFile(SECRETLEVELSFILE, "r");
		}
		for ( i = 0; i < currentlevel; i++ )
			while ( fgetc(fp) != '\n' ) if ( feof(fp) )
				{
					break;
				}
		fscanf(fp, "%s", tempstr);
		while ( fgetc(fp) != ' ' ) if ( feof(fp) )
			{
				break;
			}
		if ( !strcmp(tempstr, "gen:") )
		{
			fscanf(fp, "%s", tempstr);
			while ( fgetc(fp) != '\n' ) if ( feof(fp) )
				{
					break;
				}
			result = generateDungeon(tempstr, mapseed);
		}
		else if ( !strcmp(tempstr, "map:") )
		{
			fscanf(fp, "%s", tempstr);
			while ( fgetc(fp) != '\n' ) if ( feof(fp) )
				{
					break;
				}
			result = loadMap(tempstr, &map, map.entities);
		}
		fclose(fp);
		numplayers = 0;
		assignActions(&map);
		generatePathMaps();
		for ( node = map.entities->first; node != NULL; node = nextnode )
		{
			nextnode = node->next;
			Entity* entity = (Entity*)node->element;
			if ( entity->flags[NOUPDATE] )
			{
				list_RemoveNode(entity->mynode);    // we're anticipating this entity data from server
			}
		}

		saveGame();

		// (special) unlock temple achievement
		if ( secretlevel && currentlevel == 8 )
		{
			steamAchievement("BARONY_ACH_TRICKS_AND_TRAPS");
		}

		printlog("Done.\n");

		if ( !secretlevel )
		{
			messagePlayer(clientnum, language[710], currentlevel);
		}
		else
		{
			messagePlayer(clientnum, language[711], map.name);
		}
		if ( !secretlevel && result )
		{
			switch ( currentlevel )
			{
				case 2:
					messagePlayer(clientnum, language[712]);
					break;
				case 3:
					messagePlayer(clientnum, language[713]);
					break;
				case 7:
					messagePlayer(clientnum, language[714]);
					break;
				case 8:
					messagePlayer(clientnum, language[715]);
					break;
				case 11:
					messagePlayer(clientnum, language[716]);
					break;
				case 13:
					messagePlayer(clientnum, language[717]);
					break;
				case 16:
					messagePlayer(clientnum, language[718]);
					break;
				case 18:
					messagePlayer(clientnum, language[719]);
					break;
				default:
					break;
			}
		}

		loading = false;
		fadeout = false;
		fadealpha = 255;
		return;
	}

	// spawn magical effect particles
	else if (!strncmp((char*)net_packet->data, "MAGE", 4))
	{
		Sint16 x = (Sint16)SDLNet_Read16(&net_packet->data[4]);
		Sint16 y = (Sint16)SDLNet_Read16(&net_packet->data[6]);
		Sint16 z = (Sint16)SDLNet_Read16(&net_packet->data[8]);
		Uint32 sprite = (Uint32)SDLNet_Read32(&net_packet->data[10]);
		spawnMagicEffectParticles( x, y, z, sprite );
		return;
	}

	// spawn misc particle effect 
	else if ( !strncmp((char*)net_packet->data, "SPPE", 4) )
	{
		i = (int)SDLNet_Read32(&net_packet->data[4]);
		for ( node = map.entities->first; node != NULL; node = node->next )
		{
			entity = (Entity*)node->element;
			if ( entity->getUID() == i )
			{
				int particleType = net_packet->data[8];
				switch ( particleType )
				{
					case PARTICLE_EFFECT_ABILITY_PURPLE:
						createParticleDot(entity);
						break;
					case PARTICLE_EFFECT_ABILITY_ROCK:
						createParticleRock(entity);
						break;
					default:
						break;
				}
			}
		}
		return;
	}

	// spawn an explosion
	else if (!strncmp((char*)net_packet->data, "EXPL", 4))
	{
		Sint16 x = (Sint16)SDLNet_Read16(&net_packet->data[4]);
		Sint16 y = (Sint16)SDLNet_Read16(&net_packet->data[6]);
		Sint16 z = (Sint16)SDLNet_Read16(&net_packet->data[8]);
		spawnExplosion( x, y, z );
		return;
	}

	// spawn a bang sprite
	else if (!strncmp((char*)net_packet->data, "BANG", 4))
	{
		Sint16 x = (Sint16)SDLNet_Read16(&net_packet->data[4]);
		Sint16 y = (Sint16)SDLNet_Read16(&net_packet->data[6]);
		Sint16 z = (Sint16)SDLNet_Read16(&net_packet->data[8]);
		spawnBang( x, y, z );
		return;
	}

	// spawn a gib
	else if (!strncmp((char*)net_packet->data, "SPGB", 4))
	{
		Sint16 x = (Sint16)SDLNet_Read16(&net_packet->data[4]);
		Sint16 y = (Sint16)SDLNet_Read16(&net_packet->data[6]);
		Sint16 z = (Sint16)SDLNet_Read16(&net_packet->data[8]);
		Sint16 sprite = (Sint16)SDLNet_Read16(&net_packet->data[10]);
		Entity* gib = spawnGibClient( x, y, z, sprite );
		gib->flags[SPRITE] = net_packet->data[12];
		if ( !spawn_blood && (!gib->flags[SPRITE] || gib->sprite != 29) )
		{
			gib->flags[INVISIBLE] = true;
		}
		return;
	}

	// spawn a sleep Z
	else if (!strncmp((char*)net_packet->data, "SLEZ", 4))
	{
		Sint16 x = (Sint16)SDLNet_Read16(&net_packet->data[4]);
		Sint16 y = (Sint16)SDLNet_Read16(&net_packet->data[6]);
		Sint16 z = (Sint16)SDLNet_Read16(&net_packet->data[8]);
		spawnSleepZ( x, y, z );
		return;
	}

	// lead a monster
	else if (!strncmp((char*)net_packet->data, "LEAD", 4))
	{
		Uint32* uidnum = (Uint32*) malloc(sizeof(Uint32));
		*uidnum = (Uint32)SDLNet_Read32(&net_packet->data[4]);
		node_t* node = list_AddNodeLast(&stats[clientnum]->FOLLOWERS);
		node->element = uidnum;
		node->deconstructor = &defaultDeconstructor;
		node->size = sizeof(Uint32);
		return;
	}

	// bless my equipment
	else if (!strncmp((char*)net_packet->data, "BLES", 4))
	{
		if ( stats[clientnum]->helmet )
		{
			stats[clientnum]->helmet->beatitude++;
		}
		if ( stats[clientnum]->breastplate )
		{
			stats[clientnum]->breastplate->beatitude++;
		}
		if ( stats[clientnum]->gloves )
		{
			stats[clientnum]->gloves->beatitude++;
		}
		if ( stats[clientnum]->shoes )
		{
			stats[clientnum]->shoes->beatitude++;
		}
		if ( stats[clientnum]->shield )
		{
			stats[clientnum]->shield->beatitude++;
		}
		if ( stats[clientnum]->weapon )
		{
			stats[clientnum]->weapon->beatitude++;
		}
		if ( stats[clientnum]->cloak )
		{
			stats[clientnum]->cloak->beatitude++;
		}
		if ( stats[clientnum]->amulet )
		{
			stats[clientnum]->amulet->beatitude++;
		}
		if ( stats[clientnum]->ring )
		{
			stats[clientnum]->ring->beatitude++;
		}
		if ( stats[clientnum]->mask )
		{
			stats[clientnum]->mask->beatitude++;
		}
		return;
	}

	// update entity appearance (sprite)
	else if (!strncmp((char*)net_packet->data, "ENTA", 4))
	{
		i = (int)SDLNet_Read32(&net_packet->data[4]);
		for ( node = map.entities->first; node != NULL; node = node->next )
		{
			entity = (Entity*)node->element;
			if ( entity->getUID() == i )
			{
				entity->sprite = SDLNet_Read32(&net_packet->data[8]);
			}
		}
		return;
	}

	// bodypart ids
	else if (!strncmp((char*)net_packet->data, "BDYI", 4))
	{
		i = (int)SDLNet_Read32(&net_packet->data[4]);
		for ( node = map.entities->first; node != NULL; node = node->next )
		{
			entity = (Entity*)node->element;
			if ( entity->getUID() == i )
			{
				node_t* tempNode;
				int c;
				for ( c = 0, tempNode = entity->children.first; tempNode != NULL; tempNode = tempNode->next, c++ )
				{
					if ( c < 1 || (c < 2 && entity->behavior == &actMonster) )
					{
						continue;
					}
					Entity* tempEntity = (Entity*)tempNode->element;
					if ( entity->behavior == &actMonster )
					{
						tempEntity->setUID(SDLNet_Read32(&net_packet->data[8 + 4 * (c - 2)]));
					}
					else
					{
						tempEntity->setUID(SDLNet_Read32(&net_packet->data[8 + 4 * (c - 1)]));
					}
				}
			}
		}
		return;
	}

	// monster summon
	else if (!strncmp((char*)net_packet->data, "SUMM", 4))
	{
		Monster monster = (Monster)SDLNet_Read32(&net_packet->data[4]);
		Sint32 x = (Sint32)SDLNet_Read32(&net_packet->data[8]);
		Sint32 y = (Sint32)SDLNet_Read32(&net_packet->data[12]);
		Uint32 uid = SDLNet_Read32(&net_packet->data[16]);
		summonMonsterClient(monster, x, y, uid);
		return;
	}

	// update entity bodypart
	else if (!strncmp((char*)net_packet->data, "ENTB", 4))
	{
		i = (Uint32)SDLNet_Read32(&net_packet->data[4]);
		for ( node = map.entities->first; node != NULL; node = node->next )
		{
			entity = (Entity*)node->element;
			if ( entity->getUID() == i )
			{
				node_t* tempNode = list_Node(&entity->children, net_packet->data[8]);
				if ( tempNode )
				{
					Entity* tempEntity = (Entity*)tempNode->element;
					tempEntity->sprite = SDLNet_Read32(&net_packet->data[9]);
					tempEntity->skill[7] = tempEntity->sprite;
					tempEntity->flags[INVISIBLE] = net_packet->data[13];
				}
			}
		}
		return;
	}

	// update entity skill
	else if (!strncmp((char*)net_packet->data, "ENTS", 4))
	{
		i = (int)SDLNet_Read32(&net_packet->data[4]);
		for ( node = map.entities->first; node != NULL; node = node->next )
		{
			entity = (Entity*)node->element;
			if ( entity->getUID() == i )
			{
				entity->skill[net_packet->data[8]] = SDLNet_Read32(&net_packet->data[9]);
			}
		}
		return;
	}

	// update entity fskill
	else if ( !strncmp((char*)net_packet->data, "ENFS", 4) )
	{
		i = (int)SDLNet_Read32(&net_packet->data[4]);
		for ( node = map.entities->first; node != NULL; node = node->next )
		{
			entity = (Entity*)node->element;
			if ( entity->getUID() == i )
			{
				entity->fskill[net_packet->data[8]] = SDLNet_Read32(&net_packet->data[9]);
			}
		}
		return;
	}

	// update entity flag
	else if (!strncmp((char*)net_packet->data, "ENTF", 4))
	{
		i = (int)SDLNet_Read32(&net_packet->data[4]);
		for ( node = map.entities->first; node != NULL; node = node->next )
		{
			entity = (Entity*)node->element;
			if ( entity->getUID() == i )
			{
				entity->flags[net_packet->data[8]] = net_packet->data[9];
			}
		}
		return;
	}

	// entity update
	else if (!strncmp((char*)net_packet->data, "ENTU", 4))
	{
		client_keepalive[0] = ticks; // don't timeout
		i = (int)SDLNet_Read32(&net_packet->data[4]);
		for ( node = map.entities->first; node != NULL; node = nextnode )
		{
			nextnode = node->next;
			entity2 = (Entity*)node->element;
			if (entity2->getUID() == i)
			{
				if ( (Uint32)SDLNet_Read32(&net_packet->data[36]) < (Uint32)entity2->lastupdateserver )
				{
					i = -1; // old packet, not used
				}
				else if ( entity2->behavior == &actPlayer && entity2->skill[2] == clientnum )
				{
					i = -1; // don't update my player
				}
				else if (entity2->flags[NOUPDATE])
				{
					// inform the server that it tried to update a no-update entity
					strcpy((char*)net_packet->data, "NOUP");
					net_packet->data[4] = clientnum;
					SDLNet_Write32(entity2->getUID(), &net_packet->data[5]);
					net_packet->address.host = net_server.host;
					net_packet->address.port = net_server.port;
					net_packet->len = 9;
					sendPacket(net_sock, -1, net_packet, 0);
				}
				else
				{
					// receive the entity
					receiveEntity(entity2);
					entity2->behavior = NULL;
					clientActions(entity2);
				}
				i = -1;
				break;
			}
		}
		if ( i == -1 )
		{
			return;
		}
		for ( node = removedEntities.first; node != NULL; node = node->next )
		{
			entity2 = (Entity*)node->element;
			if ( entity2->getUID() == (int)SDLNet_Read32(&net_packet->data[4]) )
			{
				i = -1;
				break;
			}
		}
		if ( i == -1 )
		{
			return;
		}
		entity = receiveEntity(NULL);

		// IMPORTANT! Assign actions to the objects the client has control over
		clientActions(entity);
		return;
	}

	//Multiplayer chest code (client).
	else if (!strncmp((char*)net_packet->data, "CHST", 4))
	{
		i = (int)SDLNet_Read32(&net_packet->data[4]);

		if (openedChest[clientnum])
		{
			//Close the chest.
			closeChestClientside();
		}

		for (node = map.entities->first; node != NULL; node = nextnode)
		{
			nextnode = node->next;
			entity2 = (Entity*)node->element;
			if (entity2->getUID() == i)
			{
				openedChest[clientnum] = entity2; //Set the opened chest to this.
				if ( removecursegui_active )
				{
					closeRemoveCurseGUI();
				}
				identifygui_active = false;
				list_FreeAll(&chestInv);
				chestInv.first = nullptr;
				chestInv.last = nullptr;
				openStatusScreen(GUI_MODE_INVENTORY, INVENTORY_MODE_ITEM);
			}
		}

		return;
	}

	//Add an item to the chest.
	else if (!strncmp((char*)net_packet->data, "CITM", 4))
	{
		Item* newitem = NULL;
		if ( (newitem = (Item*) malloc(sizeof(Item))) == NULL)
		{
			printlog( "failed to allocate memory for new item!\n" );
			return; //TODO: Error or something.
		}
		newitem->node = NULL;
		newitem->type = static_cast<ItemType>(SDLNet_Read32(&net_packet->data[4]));
		newitem->status = static_cast<Status>(SDLNet_Read32(&net_packet->data[8]));
		newitem->beatitude = SDLNet_Read32(&net_packet->data[12]);
		newitem->count = SDLNet_Read32(&net_packet->data[16]);
		newitem->appearance = SDLNet_Read32(&net_packet->data[20]);
		if ( net_packet->data[24])   //TODO: Is this right?
		{
			newitem->identified = true;
		}
		else
		{
			newitem->identified = false;
		}

		addItemToChestClientside(newitem);
		return;
	}

	//Close the chest.
	else if (!strncmp((char*)net_packet->data, "CCLS", 4))
	{
		closeChestClientside();
		return;
	}

	//Open up the GUI to identify an item.
	else if (!strncmp((char*)net_packet->data, "IDEN", 4))
	{
		//identifygui_mode = true;
		identifygui_active = true;
		identifygui_appraising = false;
		shootmode = false;
		gui_mode = GUI_MODE_INVENTORY; //Reset the GUI to the inventory.
		if ( removecursegui_active )
		{
			closeRemoveCurseGUI();
		}
		if ( openedChest[clientnum] )
		{
			openedChest[clientnum]->closeChest();
		}

		//Initialize Identify GUI game controller code here.
		initIdentifyGUIControllerCode();

		return;
	}

    //Open up the Remove Curse GUI
    else if ( !strncmp((char*)net_packet->data, "CRCU", 4) )
    {
        removecursegui_active = true;
        shootmode = false;
        gui_mode = GUI_MODE_INVENTORY; //Reset the GUI to the inventory.
        if ( identifygui_active )
        {
            closeIdentifyGUI();
        }
        if ( openedChest[clientnum] )
        {
            openedChest[clientnum]->closeChest();
        }

        //Initialize Remove Curse GUI game controller code here.
        initRemoveCurseGUIControllerCode();

        return;
    }


	//Add a spell to the channeled spells list.
	else if (!strncmp((char*)net_packet->data, "CHAN", 4))
	{
		spell_t* thespell = getSpellFromID(SDLNet_Read32(&net_packet->data[5]));
		node = list_AddNodeLast(&channeledSpells[clientnum]);
		node->element = thespell;
		node->size = sizeof(spell_t);
		//node->deconstructor = &spellDeconstructor_Channeled;
		node->deconstructor = &emptyDeconstructor;
		((spell_t*)(node->element))->sustain_node = node;
		return;
	}

	//Remove a spell from the channeled spells list.
	else if (!strncmp((char*)net_packet->data, "UNCH", 4))
	{
		spell_t* thespell = getSpellFromID(SDLNet_Read32(&net_packet->data[5]));
		if (spellInList(&channeledSpells[clientnum], thespell))
		{
			node_t* nextnode;
			for (node = channeledSpells[clientnum].first; node; node = nextnode)
			{
				nextnode = node->next;
				spell_t* spell_search = (spell_t*)node->element;
				if (spell_search->ID == thespell->ID)
				{
					list_RemoveNode(node);
					node = NULL;
				}
			}
		}
		return;
	}

	//Map the magic. I mean magic the map. I mean magically map the level (client).
	else if (!strncmp((char*)net_packet->data, "MMAP", 4))
	{
		spell_magicMap(clientnum);
		return;
	}

	// boss death
	else if (!strncmp((char*)net_packet->data, "BDTH", 4))
	{
		for ( node = map.entities->first; node != NULL; node = node->next )
		{
			Entity* entity = (Entity*)node->element;
			if ( entity->behavior == &actWinningPortal )
			{
				entity->flags[INVISIBLE] = false;
			}
		}
		if ( strstr(map.name, "Hell") )
		{
			int x, y;
			for ( y = map.height / 2 - 1; y < map.height / 2 + 2; y++ )
			{
				for ( x = 3; x < map.width / 2; x++ )
				{
					if ( !map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height] )
					{
						map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height] = 72;
					}
				}
			}
		}
		return;
	}

	// update svFlags
	else if (!strncmp((char*)net_packet->data, "SVFL", 4))
	{
		svFlags = SDLNet_Read32(&net_packet->data[4]);
		return;
	}

	// kick
	else if (!strncmp((char*)net_packet->data, "KICK", 4))
	{
		button_t* button;

		printlog("kicked from server.\n");
		pauseGame(2, 0);

		// close current window
		buttonCloseSubwindow(NULL);
		for ( node = button_l.first; node != NULL; node = nextnode )
		{
			nextnode = node->next;
			button = (button_t*)node->element;
			if ( button->focused )
			{
				list_RemoveNode(button->node);
			}
		}

		// create new window
		subwindow = 1;
		subx1 = xres / 2 - 256;
		subx2 = xres / 2 + 256;
		suby1 = yres / 2 - 56;
		suby2 = yres / 2 + 56;
		strcpy(subtext, language[1127]);

		// close button
		button = newButton();
		strcpy(button->label, "x");
		button->x = subx2 - 20;
		button->y = suby1;
		button->sizex = 20;
		button->sizey = 20;
		button->action = &buttonCloseAndEndGameConfirm;
		button->visible = 1;
		button->focused = 1;

		// okay button
		button = newButton();
		strcpy(button->label, language[732]);
		button->x = subx2 - (subx2 - subx1) / 2 - 28;
		button->y = suby2 - 28;
		button->sizex = 56;
		button->sizey = 20;
		button->action = &buttonCloseAndEndGameConfirm;
		button->visible = 1;
		button->focused = 1;
		button->key = SDL_SCANCODE_RETURN;

		client_disconnected[0] = true;
		return;
	}

	// win the game
	else if (!strncmp((char*)net_packet->data, "WING", 4))
	{
		victory = net_packet->data[4];
		subwindow = 0;
		introstage = 5; // prepares win game sequence
		fadeout = true;
		if ( !intro )
		{
			pauseGame(2, false);
		}
		return;
	}

	else if ( !strncmp((char*)net_packet->data, "EFFE", 4))
	{
		/*
		 * Packet breakdown:
		 * [0][1][2][3]: "EFFE"
		 * [4][5][6][7]: Entity's UID.
		 * [8][9][10][11]: Entity's effects.
		 */

		Uint32 uid = static_cast<int>(SDLNet_Read32(&net_packet->data[4]));

		Entity* entity = map.getEntityWithUID(uid);

		if ( entity )
		{
			if ( entity->behavior == &actPlayer && entity->skill[2] == clientnum )
			{
				//Don't update this client's entity! Use the dedicated function for that.
				return;
			}

			Stat *stats = entity->getStats();
			if ( !stats )
			{
				entity->giveClientStats();
				stats = entity->getStats();
				if ( !stats )
				{
					return;
				}
			}

			for ( int i = 0; i < NUMEFFECTS; ++i )
			{
				if ( net_packet->data[8 + i / 8]&power(2, i - (i / 8) * 8) )
				{
					stats->EFFECTS[i] = true;
				}
				else
				{
					stats->EFFECTS[i] = false;
				}
			}
		}

		return;
	}

	// game restart
	if (!strncmp((char*)net_packet->data, "BARONY_GAME_START", 17))
	{
		intro = true;
		client_disconnected[0] = true;
		svFlags = SDLNet_Read32(&net_packet->data[17]);
		uniqueGameKey = SDLNet_Read32(&net_packet->data[21]);
		buttonCloseSubwindow(NULL);
		numplayers = 0;
		introstage = 3;
		fadeout = true;
		return;
	}
}

/*-------------------------------------------------------------------------------

	clientHandleMessages

	Parses messages received from the server

-------------------------------------------------------------------------------*/

void clientHandleMessages()
{
#ifdef STEAMWORKS
	if (!directConnect && !net_handler)
	{
		net_handler = new NetHandler();
		net_handler->initializeMultithreadedPacketHandling();
	}
#endif

	if (!directConnect)
	{
		//Steam stuff goes here.
		SteamPacketWrapper* packet = nullptr;
		while (packet = net_handler->getGamePacket())
		{
			memcpy(net_packet->data, packet->data(), packet->len());
			net_packet->len = packet->len();

			clientHandlePacket(); //Uses net_packet.

			delete packet;
		}
	}
	else
	{
		//Direct-connect goes here.

		// receive packets from server
		while (SDLNet_UDP_Recv(net_sock, net_packet))
		{
			// filter out broken packets
			if ( !net_packet->data[0] )
			{
				continue;
			}

			clientHandlePacket();
		}
	}
}

/*-------------------------------------------------------------------------------

	serverHandlePacket

	Called by serverHandleMessages. Does the actual handling of a packet.

-------------------------------------------------------------------------------*/

void serverHandlePacket()
{
	if (handleSafePacket())
	{
		return;
	}

	node_t* node;
	Entity* entity;
	int c = 0;
	Uint32 j;
	Item* item;
	char shortname[11];
	double dx, dy, velx, vely, yaw, pitch, dist;
	deleteent_t* deleteent;

#ifdef PACKETINFO
	char packetinfo[NET_PACKET_SIZE];
	strncpy( packetinfo, (char*)net_packet->data, net_packet->len );
	packetinfo[net_packet->len] = 0;
	printlog("info: server packet: %s\n", packetinfo);
#endif

	// keep alive
	if (!strncmp((char*)net_packet->data, "KPAL", 4))
	{
		client_keepalive[net_packet->data[4]] = ticks;
		return;
	}

	// ping
	else if (!strncmp((char*)net_packet->data, "PING", 4))
	{
		j = net_packet->data[4];
		if ( client_disconnected[j] )
		{
			return;
		}
		strcpy((char*)net_packet->data, "PING");
		net_packet->address.host = net_clients[j - 1].host;
		net_packet->address.port = net_clients[j - 1].port;
		net_packet->len = 5;
		sendPacketSafe(net_sock, -1, net_packet, j - 1);
		return;
	}

	// pause game
	else if (!strncmp((char*)net_packet->data, "PAUS", 4))
	{
		messagePlayer(clientnum, language[1118], stats[net_packet->data[4]]->name);
		j = net_packet->data[4];
		pauseGame(2, j);
		return;
	}

	// unpause game
	else if (!strncmp((char*)net_packet->data, "UNPS", 4))
	{
		messagePlayer(clientnum, language[1119], stats[net_packet->data[4]]->name);
		j = net_packet->data[4];
		pauseGame(1, j);
		return;
	}

	// check entity existence
	else if (!strncmp((char*)net_packet->data, "ENTE", 4))
	{
		int x = net_packet->data[4];
		Uint32 uid = SDLNet_Read32(&net_packet->data[5]);
		bool foundit = false;
		for ( node = map.entities->first; node != NULL; node = node->next )
		{
			entity = (Entity*)node->element;
			if ( entity->getUID() == uid )
			{
				foundit = true;
				break;
			}
		}
		if ( foundit )
		{
			return;
		}
		strcpy((char*)net_packet->data, "ENTD");
		SDLNet_Write32(uid, &net_packet->data[4]);
		net_packet->address.host = net_clients[x - 1].host;
		net_packet->address.port = net_clients[x - 1].port;
		net_packet->len = 8;
		sendPacketSafe(net_sock, -1, net_packet, x - 1);
		return;
	}

	// player move
	else if (!strncmp((char*)net_packet->data, "PMOV", 4))
	{
		client_keepalive[net_packet->data[4]] = ticks;
		if (players[net_packet->data[4]] == nullptr || players[net_packet->data[4]]->entity == nullptr)
		{
			return;
		}

		// check if the info is outdated
		if ( net_packet->data[5] != currentlevel )
		{
			return;
		}

		// get info from client
		dx = ((Sint16)SDLNet_Read16(&net_packet->data[6])) / 32.0;
		dy = ((Sint16)SDLNet_Read16(&net_packet->data[8])) / 32.0;
		velx = ((Sint16)SDLNet_Read16(&net_packet->data[10])) / 128.0;
		vely = ((Sint16)SDLNet_Read16(&net_packet->data[12])) / 128.0;
		yaw = ((Sint16)SDLNet_Read16(&net_packet->data[14])) / 128.0;
		pitch = ((Sint16)SDLNet_Read16(&net_packet->data[16])) / 128.0;

		// update rotation
		players[net_packet->data[4]]->entity->yaw = yaw;
		players[net_packet->data[4]]->entity->pitch = pitch;

		// update player's internal velocity variables
		players[net_packet->data[4]]->entity->vel_x = velx; // PLAYER_VELX
		players[net_packet->data[4]]->entity->vel_y = vely; // PLAYER_VELY

		// calculate distance
		dx -= players[net_packet->data[4]]->entity->x;
		dy -= players[net_packet->data[4]]->entity->y;
		dist = sqrt( dx * dx + dy * dy );

		// make water passable
		for ( node = map.entities->first; node != NULL; node = node->next )
		{
			Entity* tempentity = (Entity*)node->element;
			if ( tempentity->sprite == 28 && tempentity->flags[SPRITE] == true )
			{
				tempentity->flags[PASSABLE] = true;
			}
		}

		// move player with collision detection
		if (clipMove(&players[net_packet->data[4]]->entity->x, &players[net_packet->data[4]]->entity->y, dx, dy, players[net_packet->data[4]]->entity) < dist - .025 )
		{
			// player encountered obstacle on path
			// stop updating position on server side and send client corrected position
			j = net_packet->data[4];
			strcpy((char*)net_packet->data, "PMOV");
			SDLNet_Write16((Sint16)(players[j]->entity->x * 32), &net_packet->data[4]);
			SDLNet_Write16((Sint16)(players[j]->entity->y * 32), &net_packet->data[6]);
			net_packet->address.host = net_clients[j - 1].host;
			net_packet->address.port = net_clients[j - 1].port;
			net_packet->len = 8;
			sendPacket(net_sock, -1, net_packet, j - 1);
		}

		// make water unpassable
		for ( node = map.entities->first; node != NULL; node = node->next )
		{
			Entity* tempentity = (Entity*)node->element;
			if ( tempentity->sprite == 28 && tempentity->flags[SPRITE] == true )
			{
				tempentity->flags[PASSABLE] = false;
			}
		}

		return;
	}

	// tried to update
	else if (!strncmp((char*)net_packet->data, "NOUP", 4))
	{
		for ( node = map.entities->first; node != NULL; node = node->next )
		{
			Entity* tempEntity = (Entity*)node->element;
			if (tempEntity->getUID() == SDLNet_Read32(&net_packet->data[5]))
			{
				tempEntity->flags[UPDATENEEDED] = false;
			}
		}
		return;
	}

	// client deleted entity
	else if (!strncmp((char*)net_packet->data, "ENTD", 4))
	{
		for ( node = entitiesToDelete[net_packet->data[4]].first; node != NULL; node = node->next )
		{
			deleteent = (deleteent_t*)node->element;
			if ( deleteent->uid == SDLNet_Read32(&net_packet->data[5]) )
			{
				list_RemoveNode(node);
				break;
			}
		}
		return;
	}

	// clicked entity in range
	else if (!strncmp((char*)net_packet->data, "CKIR", 4))
	{
		client_keepalive[net_packet->data[4]] = ticks;
		for (node = map.entities->first; node != NULL; node = node->next)
		{
			entity = (Entity*)node->element;
			if (entity->getUID() == SDLNet_Read32(&net_packet->data[5]))
			{
				client_selected[net_packet->data[4]] = entity;
				inrange[net_packet->data[4]] = true;
				break;
			}
		}
		return;
	}

	// clicked entity out of range
	else if (!strncmp((char*)net_packet->data, "CKOR", 4))
	{
		for (node = map.entities->first; node != NULL; node = node->next)
		{
			entity = (Entity*)node->element;
			if (entity->getUID() == SDLNet_Read32(&net_packet->data[5]))
			{
				client_selected[net_packet->data[4]] = entity;
				inrange[net_packet->data[4]] = false;
				break;
			}
		}
		return;
	}

	// disconnect
	else if (!strncmp((char*)net_packet->data, "DISCONNECT", 10))
	{
		int playerDisconnected = net_packet->data[10];
		strncpy(shortname, stats[playerDisconnected]->name, 10);
		shortname[10] = 0;
		client_disconnected[playerDisconnected] = true;
		for ( c = 1; c < MAXPLAYERS; c++ )
		{
			if ( client_disconnected[c] == true )
			{
				return;
			}
			strncpy((char*)net_packet->data, "DISCONNECT", 10);
			net_packet->data[10] = playerDisconnected;
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 11;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
			messagePlayer(c, language[1120], shortname);
		}
		messagePlayer(clientnum, language[1120], shortname);
		return;
	}

	// message
	else if (!strncmp((char*)net_packet->data, "MSGS", 4))
	{
		char tempstr[1024];

		int pnum = net_packet->data[4];
		client_keepalive[pnum] = ticks;
		Uint32 color = SDLNet_Read32(&net_packet->data[5]);

		strncpy(tempstr, stats[pnum]->name, std::min<size_t>(strlen(stats[pnum]->name), 10)); //TODO: Why are size_t and int being compared?
		tempstr[std::min<size_t>(strlen(stats[pnum]->name), 10)] = 0; //TODO: Why are size_t and int being compared?
		strcat(tempstr, ": ");
		strcat(tempstr, (char*)(&net_packet->data[9]));
		messagePlayerColor(clientnum, color, tempstr);

		playSound(238, 64);

		// relay message to all clients
		for ( c = 1; c < MAXPLAYERS; c++ )
		{
			if ( c == pnum || client_disconnected[c] == true )
			{
				return;
			}
			strcpy((char*)net_packet->data, "MSGS");
			SDLNet_Write32(color, &net_packet->data[4]);
			strcpy((char*)(&net_packet->data[8]), tempstr);
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 8 + strlen(tempstr) + 1;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
		return;
	}

	// spotting (examining)
	else if (!strncmp((char*)net_packet->data, "SPOT", 4))
	{
		client_keepalive[net_packet->data[4]] = ticks;
		j = net_packet->data[4]; // player number
		for (node = map.entities->first; node != NULL; node = node->next)
		{
			entity = (Entity*)node->element;
			if (entity->getUID() == SDLNet_Read32(&net_packet->data[5]))
			{
				clickDescription(j, entity);
				break;
			}
		}
		return;
	}

	// item drop
	else if (!strncmp((char*)net_packet->data, "DROP", 4))
	{
		client_keepalive[net_packet->data[25]] = ticks;
		item = newItem(static_cast<ItemType>(SDLNet_Read32(&net_packet->data[4])), static_cast<Status>(SDLNet_Read32(&net_packet->data[8])), SDLNet_Read32(&net_packet->data[12]), SDLNet_Read32(&net_packet->data[16]), SDLNet_Read32(&net_packet->data[20]), net_packet->data[24], &stats[net_packet->data[25]]->inventory);
		dropItem(item, net_packet->data[25]);
		return;
	}

	// item drop (on death)
	else if (!strncmp((char*)net_packet->data, "DIEI", 4))
	{
		item = newItem(static_cast<ItemType>(SDLNet_Read32(&net_packet->data[4])), static_cast<Status>(SDLNet_Read32(&net_packet->data[8])), SDLNet_Read32(&net_packet->data[12]), SDLNet_Read32(&net_packet->data[16]), SDLNet_Read32(&net_packet->data[20]), net_packet->data[24], &stats[net_packet->data[25]]->inventory);
		entity = newEntity(-1, 1, map.entities);
		entity->x = net_packet->data[26];
		entity->x = entity->x * 16 + 8;
		entity->y = net_packet->data[27];
		entity->y = entity->y * 16 + 8;
		entity->flags[NOUPDATE] = true;
		entity->flags[PASSABLE] = true;
		entity->flags[INVISIBLE] = true;
		for ( c = item->count; c > 0; c-- )
		{
			dropItemMonster(item, entity, stats[net_packet->data[25]]);
		}
		list_RemoveNode(entity->mynode);
		return;
	}

	// raise/lower shield
	else if (!strncmp((char*)net_packet->data, "SHLD", 4))
	{
		stats[net_packet->data[4]]->defending = net_packet->data[5];
		return;
	}

	// close shop
	else if (!strncmp((char*)net_packet->data, "SHPC", 4))
	{
		Entity* entity = uidToEntity((Uint32)SDLNet_Read32(&net_packet->data[4]));
		if ( entity )
		{
			entity->skill[0] = 0;
			monsterMoveAside(entity, uidToEntity(entity->skill[1]));
			entity->skill[1] = 0;
		}
		return;
	}

	// buy item from shop
	else if (!strncmp((char*)net_packet->data, "SHPB", 4))
	{
		Uint32 uidnum = (Uint32)SDLNet_Read32(&net_packet->data[4]);
		int client = net_packet->data[25];
		Entity* entity = uidToEntity(uidnum);
		if ( !entity )
		{
			printlog("warning: client %d bought item from non-existent shop! (uid=%d)\n", client, uidnum);
			return;
		}
		Stat* entitystats = entity->getStats();
		if ( !entitystats )
		{
			printlog("warning: client %d bought item from a \"shop\" that has no stats! (uid=%d)\n", client, uidnum);
			return;
		}
		Item* item = (Item*) malloc(sizeof(Item));
		item->type = static_cast<ItemType>(SDLNet_Read32(&net_packet->data[8]));
		item->status = static_cast<Status>(SDLNet_Read32(&net_packet->data[12]));
		item->beatitude = SDLNet_Read32(&net_packet->data[16]);
		item->count = 1;
		item->appearance = SDLNet_Read32(&net_packet->data[20]);
		if ( net_packet->data[24] )
		{
			item->identified = true;
		}
		else
		{
			item->identified = false;
		}
		node_t* nextnode;
		for ( node = entitystats->inventory.first; node != NULL; node = nextnode )
		{
			nextnode = node->next;
			Item* item2 = (Item*)node->element;
			if (!itemCompare(item, item2))
			{
				printlog("client %d bought item from shop (uid=%d)\n", client, uidnum);
				consumeItem(item2);
				break;
			}
		}
		entitystats->GOLD += item->buyValue(client);
		stats[client]->GOLD -= item->buyValue(client);
		if (rand() % 2)
		{
			players[client]->entity->increaseSkill(PRO_TRADING);
		}
		free(item);
		return;
	}

	//Remove a spell from the channeled spells list.
	else if (!strncmp((char*)net_packet->data, "UNCH", 4))
	{
		int client = net_packet->data[4];
		spell_t* thespell = getSpellFromID(SDLNet_Read32(&net_packet->data[5]));
		if (spellInList(&channeledSpells[client], thespell))
		{
			for (node = channeledSpells[client].first; node; node = node->next)
			{
				spell_t* spell_search = (spell_t*)node->element;
				if (spell_search->ID == thespell->ID)
				{
					spell_search->sustain = false;
					break; //Found it!
				}
			}
		}
		return;
	}

	// sell item to shop
	else if (!strncmp((char*)net_packet->data, "SHPS", 4))
	{
		Uint32 uidnum = (Uint32)SDLNet_Read32(&net_packet->data[4]);
		int client = net_packet->data[25];
		Entity* entity = uidToEntity(uidnum);
		if ( !entity )
		{
			printlog("warning: client %d sold item to non-existent shop! (uid=%d)\n", client, uidnum);
			return;
		}
		Stat* entitystats = entity->getStats();
		if ( !entitystats )
		{
			printlog("warning: client %d sold item to a \"shop\" that has no stats! (uid=%d)\n", client, uidnum);
			return;
		}
		if ( net_packet->data[24] )
		{
			item = newItem(static_cast<ItemType>(SDLNet_Read32(&net_packet->data[8])), static_cast<Status>(SDLNet_Read32(&net_packet->data[12])), SDLNet_Read32(&net_packet->data[16]), 1, SDLNet_Read32(&net_packet->data[20]), true, &entitystats->inventory);
		}
		else
		{
			item = newItem(static_cast<ItemType>(SDLNet_Read32(&net_packet->data[8])), static_cast<Status>(SDLNet_Read32(&net_packet->data[12])), SDLNet_Read32(&net_packet->data[16]), 1, SDLNet_Read32(&net_packet->data[20]), false, &entitystats->inventory);
		}
		printlog("client %d sold item to shop (uid=%d)\n", client, uidnum);
		stats[client]->GOLD += item->sellValue(client);
		if (rand() % 2)
		{
			players[client]->entity->increaseSkill(PRO_TRADING);
		}
		return;
	}

	// use item
	else if (!strncmp((char*)net_packet->data, "USEI", 4))
	{
		item = newItem(static_cast<ItemType>(SDLNet_Read32(&net_packet->data[4])), static_cast<Status>(SDLNet_Read32(&net_packet->data[8])), SDLNet_Read32(&net_packet->data[12]), SDLNet_Read32(&net_packet->data[16]), SDLNet_Read32(&net_packet->data[20]), net_packet->data[24], &stats[net_packet->data[25]]->inventory);
		useItem(item, net_packet->data[25]);
		return;
	}

	// equip item (as a weapon)
	else if (!strncmp((char*)net_packet->data, "EQUI", 4))
	{
		item = newItem(static_cast<ItemType>(SDLNet_Read32(&net_packet->data[4])), static_cast<Status>(SDLNet_Read32(&net_packet->data[8])), SDLNet_Read32(&net_packet->data[12]), SDLNet_Read32(&net_packet->data[16]), SDLNet_Read32(&net_packet->data[20]), net_packet->data[24], &stats[net_packet->data[25]]->inventory);
		equipItem(item, &stats[net_packet->data[25]]->weapon, net_packet->data[25]);
		return;
	}

	// apply item to entity
	else if (!strncmp((char*)net_packet->data, "APIT", 4))
	{
		item = newItem(static_cast<ItemType>(SDLNet_Read32(&net_packet->data[4])), static_cast<Status>(SDLNet_Read32(&net_packet->data[8])), SDLNet_Read32(&net_packet->data[12]), SDLNet_Read32(&net_packet->data[16]), SDLNet_Read32(&net_packet->data[20]), net_packet->data[24], NULL);
		Entity* entity = uidToEntity(SDLNet_Read32(&net_packet->data[26]));
		if ( entity )
		{
			item->apply(net_packet->data[25], entity);
		}
		else
		{
			printlog("warning: client applied item to entity that does not exist\n");
		}
		free(item);
		return;
	}

	// attacking
	else if (!strncmp((char*)net_packet->data, "ATAK", 4))
	{
		if (players[net_packet->data[4]] && players[net_packet->data[4]]->entity)
		{
			players[net_packet->data[4]]->entity->attack(net_packet->data[5], net_packet->data[6], nullptr);
		}
		return;
	}

	//Multiplayer chest code (server).
	else if (!strncmp((char*)net_packet->data, "CCLS", 4))    //Close the chest.
	{
		int the_client = net_packet->data[4];
		if (openedChest[the_client])
		{
			openedChest[the_client]->closeChestServer();
		}
	}

	//The client cast a spell.
	else if (!strncmp((char*)net_packet->data, "SPEL", 4))
	{
		int the_client = net_packet->data[4];

		spell_t* thespell = getSpellFromID(SDLNet_Read32(&net_packet->data[5]));
		castSpell(players[the_client]->entity->getUID(), thespell, false, false);
		return;
	}

	//The client added an item to the chest.
	else if (!strncmp((char*)net_packet->data, "CITM", 4))
	{
		int the_client = net_packet->data[4];
		if (!openedChest[the_client])
		{
			return;
		}

		Item* newitem = NULL;
		if ( (newitem = (Item*) malloc(sizeof(Item))) == NULL)
		{
			printlog( "failed to allocate memory for new item!\n" );
			return; //Should error instead or something?
		}
		newitem->node = NULL;
		newitem->type = static_cast<ItemType>(SDLNet_Read32(&net_packet->data[5]));
		newitem->status = static_cast<Status>(SDLNet_Read32(&net_packet->data[9]));
		newitem->beatitude = SDLNet_Read32(&net_packet->data[13]);
		newitem->count = SDLNet_Read32(&net_packet->data[17]);
		newitem->appearance = SDLNet_Read32(&net_packet->data[21]);
		if ( net_packet->data[25])
		{
			newitem->identified = true;
		}
		else
		{
			newitem->identified = false;
		}

		openedChest[the_client]->addItemToChestServer(newitem);

		return;
	}

	//The client removed an item from the chest.
	else if (!strncmp((char*)net_packet->data, "RCIT", 4))
	{
		int the_client = net_packet->data[4];
		if (!openedChest[the_client])
		{
			return;
		}

		Item* theitem = NULL;
		if ( (theitem = (Item*) malloc(sizeof(Item))) == NULL)
		{
			printlog( "failed to allocate memory for new item!\n" );
			return; //Should it error instead or somesuch?
		}
		theitem->node = NULL;
		theitem->type = static_cast<ItemType>(SDLNet_Read32(&net_packet->data[5]));
		theitem->status = static_cast<Status>(SDLNet_Read32(&net_packet->data[9]));
		theitem->beatitude = SDLNet_Read32(&net_packet->data[13]);
		theitem->count = SDLNet_Read32(&net_packet->data[17]);
		theitem->appearance = SDLNet_Read32(&net_packet->data[21]);
		if ( net_packet->data[25])   //HNGH NUMBERS.
		{
			theitem->identified = true;
		}
		else
		{
			theitem->identified = false;
		}

		openedChest[the_client]->removeItemFromChestServer(theitem, theitem->count);
		return;
	}

	// the client removed a curse on his equipment
	else if (!strncmp((char*)net_packet->data, "RCUR", 4))
	{
		int player = net_packet->data[4];
		switch ( net_packet->data[5] )
		{
			case 0:
				item = stats[player]->helmet;
				break;
			case 1:
				item = stats[player]->breastplate;
				break;
			case 2:
				item = stats[player]->gloves;
				break;
			case 3:
				item = stats[player]->shoes;
				break;
			case 4:
				item = stats[player]->shield;
				break;
			case 5:
				item = stats[player]->weapon;
				break;
			case 6:
				item = stats[player]->cloak;
				break;
			case 7:
				item = stats[player]->amulet;
				break;
			case 8:
				item = stats[player]->ring;
				break;
			case 9:
				item = stats[player]->mask;
				break;
			default:
				item = NULL;
				break;
		}
		if ( item != NULL )
		{
			item->beatitude = 0;
		}
		return;
	}
}

/*-------------------------------------------------------------------------------

	serverHandleMessages

	Parses messages received from clients

-------------------------------------------------------------------------------*/

void serverHandleMessages()
{
#ifdef STEAMWORKS
	if (!directConnect && !net_handler)
	{
		net_handler = new NetHandler();
		net_handler->initializeMultithreadedPacketHandling();
	}
#endif

	if (!directConnect)
	{
		//Steam stuff goes here.
		SteamPacketWrapper* packet = nullptr;
		while (packet = net_handler->getGamePacket())
		{
			memcpy(net_packet->data, packet->data(), packet->len());
			net_packet->len = packet->len();

			serverHandlePacket(); //Uses net_packet;

			delete packet;
		}
	}
	else
	{
		//Direct-connect goes here.

		while (SDLNet_UDP_Recv(net_sock, net_packet))
		{
			// filter out broken packets
			if ( !net_packet->data[0] )
			{
				continue;
			}

			serverHandlePacket(); //Uses net_packet.
		}
	}
}

/*-------------------------------------------------------------------------------

	handleSafePacket()

	Handles potentially safe packets. Returns true if this is not a packet to
	be handled.

-------------------------------------------------------------------------------*/

bool handleSafePacket()
{
	packetsend_t* packet;
	node_t* node;
	int c, j;

	// safe packet
	if (!strncmp((char*)net_packet->data, "SAFE", 4))
	{
		if ( net_packet->data[4] != MAXPLAYERS )
		{
			for ( node = safePacketsReceived[net_packet->data[4]].first; node != NULL; node = node->next )
			{
				packet = (packetsend_t*)node->element;
				if ( packet->num == SDLNet_Read32(&net_packet->data[5]) )
				{
					return true;
				}
			}
			node = list_AddNodeFirst(&safePacketsReceived[net_packet->data[4]]);
			packet = (packetsend_t*) malloc(sizeof(packetsend_t));
			packet->num = SDLNet_Read32(&net_packet->data[5]);
			node->element = packet;
			node->deconstructor = &defaultDeconstructor;

			// send an ack
			j = net_packet->data[4];
			net_packet->data[4] = clientnum;
			strcpy((char*)net_packet->data, "GOTP");
			if ( multiplayer == CLIENT )
			{
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
			}
			else
			{
				net_packet->address.host = net_clients[j - 1].host;
				net_packet->address.port = net_clients[j - 1].port;
			}
			c = net_packet->len;
			net_packet->len = 9;
			if ( multiplayer == CLIENT )
			{
				sendPacket(net_sock, -1, net_packet, 0);
			}
			else
			{
				sendPacket(net_sock, -1, net_packet, j - 1);
			}
			net_packet->len = c - 9;
			Uint8 bytedata[NET_PACKET_SIZE];
			memcpy(&bytedata, net_packet->data + 9, net_packet->len);
			memcpy(net_packet->data, &bytedata, net_packet->len);
		}
	}

	// they got the safe packet
	else if (!strncmp((char*)net_packet->data, "GOTP", 4))
	{
		for ( node = safePacketsSent.first; node != NULL; node = node->next )
		{
			packetsend_t* packet = (packetsend_t*)node->element;
			if ( packet->num == SDLNet_Read32(&net_packet->data[5]) )
			{
				list_RemoveNode(node);
				break;
			}
		}
		return true;
	}

	return false;
}

/*-------------------------------------------------------------------------------

	closeNetworkInterfaces()

	Consolidating all of the network interfaces close code into one place.

-------------------------------------------------------------------------------*/

void closeNetworkInterfaces()
{
	printlog("closing network interfaces...\n");

	if (net_handler)
	{
		delete net_handler; //Close steam multithreading and stuff.
		net_handler = nullptr;
	}

	if (net_packet != nullptr)
	{
		SDLNet_FreePacket(net_packet);
		net_packet = nullptr;
	}
	if (net_clients != nullptr)
	{
		free(net_clients);
		net_clients = nullptr;
	}
	if (net_sock != nullptr)
	{
		SDLNet_UDP_Close(net_sock);
		net_sock = nullptr;
	}
	if (net_tcpclients != nullptr)
	{
		for (int c = 0; c < MAXPLAYERS; c++)
		{
			if (net_tcpclients[c] != nullptr)
			{
				SDLNet_TCP_Close(net_tcpclients[c]);
			}
		}
		free(net_tcpclients);
		net_tcpclients = nullptr;
	}
	if (net_tcpsock != nullptr)
	{
		SDLNet_TCP_Close(net_tcpsock);
		net_tcpsock = nullptr;
	}
	if (tcpset)
	{
		SDLNet_FreeSocketSet(tcpset);
		tcpset = nullptr;
	}
}





/* ***** MULTITHREADED STEAM PACKET HANDLING ***** */

SteamPacketWrapper::SteamPacketWrapper(Uint8* data, int len)
{
	_data = data;
	_len = len;
}

SteamPacketWrapper::~SteamPacketWrapper()
{
	free(_data);
}

Uint8*& SteamPacketWrapper::data()
{
	return _data;
}

int& SteamPacketWrapper::len()
{
	return _len;
}

NetHandler::NetHandler()
{
	steam_packet_thread = nullptr;
	continue_multithreading_steam_packets = false;
	game_packets_lock = SDL_CreateMutex();
	continue_multithreading_steam_packets_lock = SDL_CreateMutex();
}

NetHandler::~NetHandler()
{
	//First, must join with the worker thread.
	printlog("Waiting for steam_packet_thread to finish...");
	stopMultithreadedPacketHandling();
	SDL_WaitThread(steam_packet_thread, NULL); //Wait for the thread to finish.
	printlog("Done.\n");

	SDL_DestroyMutex(game_packets_lock);
	game_packets_lock = nullptr;
	SDL_DestroyMutex(continue_multithreading_steam_packets_lock);
	continue_multithreading_steam_packets_lock = nullptr;

	//Free up packet memory.
	while (!game_packets.empty())
	{
		SteamPacketWrapper* packet = game_packets.front();
		delete packet;
		game_packets.pop();
	}
}

void NetHandler::initializeMultithreadedPacketHandling()
{
#ifdef STEAMWORKS

	printlog("Initializing multithreaded packet handling.");

	steam_packet_thread = SDL_CreateThread(steamPacketThread, "steamPacketThread", static_cast<void* >(this));
	continue_multithreading_steam_packets = true;

#endif
}

void NetHandler::stopMultithreadedPacketHandling()
{
	SDL_LockMutex(continue_multithreading_steam_packets_lock); //NOTE: Will block.
	continue_multithreading_steam_packets = false;
	SDL_UnlockMutex(continue_multithreading_steam_packets_lock);
}

bool NetHandler::getContinueMultithreadingSteamPackets()
{
	return continue_multithreading_steam_packets;
	//SDL_UnlockMutex(continue_multithreading_steam_packets_lock);
}

void NetHandler::addGamePacket(SteamPacketWrapper* packet)
{
	SDL_LockMutex(game_packets_lock);
	game_packets.push(packet);
	SDL_UnlockMutex(game_packets_lock);
}

SteamPacketWrapper* NetHandler::getGamePacket()
{
	SteamPacketWrapper* packet = nullptr;
	SDL_LockMutex(game_packets_lock);
	if (!game_packets.empty())
	{
		packet = game_packets.front();
		game_packets.pop();
	}
	SDL_UnlockMutex(game_packets_lock);
	return packet;
}

int steamPacketThread(void* data)
{
#ifdef STEAMWORKS

	if (!data)
	{
		return -1;    //Some generic error?
	}

	NetHandler& handler = *static_cast<NetHandler* >(data); //Basically, our this.

	Uint32 packetlen = 0;
	Uint32 bytes_read = 0;
	CSteamID steam_id_remote;
	Uint8* packet = nullptr;
	CSteamID mySteamID = SteamUser()->GetSteamID();
	bool run = true;
	std::queue<SteamPacketWrapper* > packets; //TODO: Expose to game? Use lock-free packets?

	while (run)   //1. Check if thread is supposed to be running.
	{
		//2. Game not over. Grab/poll for packet.

		//while (handler.getContinueMultithreadingSteamPackets() && SteamNetworking()->IsP2PPacketAvailable(&packetlen)) //Burst read in a bunch of packets.
		while (SteamNetworking()->IsP2PPacketAvailable(&packetlen))
		{
			packetlen = std::min<uint32_t>(packetlen, NET_PACKET_SIZE - 1);
			//Read packets and push into queue.
			packet = static_cast<Uint8* >(malloc(packetlen));
			if (SteamNetworking()->ReadP2PPacket(packet, packetlen, &bytes_read, &steam_id_remote, 0))
			{
				if (packetlen > sizeof(DWORD) && mySteamID.ConvertToUint64() != steam_id_remote.ConvertToUint64() && net_packet->data[0])
				{
					//Push packet into queue.
					//TODO: Use lock-free queues?
					packets.push(new SteamPacketWrapper(packet, packetlen));
					packet = nullptr;
				}
			}
			if (packet)
			{
				free(packet);
			}
		}


		//3. Now push our local packetstack onto the game's network stack.
		//Well, that is: analyze packet.
		//If packet is good, push into queue.
		//If packet is bad, loop back to start of function.

		while (!packets.empty())
		{
			//Copy over the packets read in so far, and expose them to the game.
			SteamPacketWrapper* packet = packets.front();
			packets.pop();
			handler.addGamePacket(packet);
		}

		SDL_LockMutex(handler.continue_multithreading_steam_packets_lock);
		run = handler.getContinueMultithreadingSteamPackets();
		SDL_UnlockMutex(handler.continue_multithreading_steam_packets_lock);
	}

#endif

	return 0; //If it isn't supposed to be running anymore, exit.

	//NOTE: This thread is to be created when the gameplay starts. NOT in the steam lobby.
	//If it's desired that it be created right when the network interfaces are opened, menu.c would need to be modified to support this, and the packet wrapper would need to include CSteamID.
}

/* ***** END MULTITHREADED STEAM PACKET HANDLING ***** */
