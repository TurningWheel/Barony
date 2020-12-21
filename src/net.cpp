/*-------------------------------------------------------------------------------

	BARONY
	File: net.cpp
	Desc: support functions for networking

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "draw.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "net.hpp"
#include "messages.hpp"
#include "entity.hpp"
#include "files.hpp"
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
#include "scores.hpp"
#include "colors.hpp"
#include "mod_tools.hpp"
#include "lobbies.hpp"

NetHandler* net_handler = nullptr;

char last_ip[64] = "";
char last_port[64] = "";
char lobbyChatbox[LOBBY_CHATBOX_LENGTH];
list_t lobbyChatboxMessages;
bool disableMultithreadedSteamNetworking = true;
bool disableFPSLimitOnNetworkMessages = false;

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

int sendPacket(UDPsocket sock, int channel, UDPpacket* packet, int hostnum, bool tryReliable)
{
	if ( directConnect )
	{
		return SDLNet_UDP_Send(sock, channel, packet);
	}
	else
	{
		if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
		{
#ifdef STEAMWORKS
			if ( steamIDRemote[hostnum] && !client_disconnected[hostnum] )
			{
				return SteamNetworking()->SendP2PPacket(*static_cast<CSteamID* >(steamIDRemote[hostnum]), packet->data, packet->len, tryReliable? k_EP2PSendReliable : k_EP2PSendUnreliable, 0);
			}
			else
			{
				return 0;
			}
#endif
		}
		else if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
		{
#if defined USE_EOS
			EOS.SendMessageP2P(EOS.P2PConnectionInfo.getPeerIdFromIndex(hostnum), (char*)packet->data, packet->len);
			return 0;
#endif
		}
		return 0;
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
	if ( hostnum < 0 )
	{
		printlog("[NET]: Error - attempt to send to negative hostnum: %d", hostnum);
		return 0;
	}

	if ( !directConnect )
	{
		if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
		{
#ifdef STEAMWORKS
			if ( !steamIDRemote[hostnum] )
			{
				return 0;
			}
#endif
		}
		else if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
		{
#if defined USE_EOS
			if ( !EOS.P2PConnectionInfo.getPeerIdFromIndex(hostnum) )
			{
				return 0;
			}
#endif
		}
	}

	packetsend_t* packetsend = (packetsend_t*) malloc(sizeof(packetsend_t));
	if (!(packetsend->packet = SDLNet_AllocPacket(NET_PACKET_SIZE)))
	{
		printlog("warning: packet allocation failed: %s\n", SDLNet_GetError());
		free(packetsend);
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
		if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
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
#endif
		}
		else if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
		{
#if defined USE_EOS
			EOS.SendMessageP2P(EOS.P2PConnectionInfo.getPeerIdFromIndex(hostnum), packetsend->packet->data, packetsend->packet->len);
			return 0;
#endif
		}
		return 0;
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

messageLocalPlayers

Support function, messages all local players with the message "message"

-------------------------------------------------------------------------------*/

void messageLocalPlayers(char const * const message, ...)
{
	char str[Player::MessageZone_t::ADD_MESSAGE_BUFFER_LENGTH] = { 0 };

	va_list argptr;
	va_start(argptr, message);
	vsnprintf(str, Player::MessageZone_t::ADD_MESSAGE_BUFFER_LENGTH - 1, message, argptr);
	va_end(argptr);

	for ( int player = 0; player < MAXPLAYERS; ++player )
	{
		if ( players[player]->isLocalPlayer() )
		{
			messagePlayer(player, str);
		}
	}
}

/*-------------------------------------------------------------------------------

	messagePlayer

	Support function, messages the player number given by "player" with the
	message "message"

-------------------------------------------------------------------------------*/

void messagePlayer(int player, char const * const message, ...)
{
	if ( player < 0 || player >= MAXPLAYERS )
	{
		return;
	}
	char str[Player::MessageZone_t::ADD_MESSAGE_BUFFER_LENGTH] = { 0 };

	va_list argptr;
	va_start( argptr, message );
	vsnprintf( str, Player::MessageZone_t::ADD_MESSAGE_BUFFER_LENGTH - 1, message, argptr );
	va_end( argptr );

	messagePlayerColor(player, 0xFFFFFFFF, str);
}

/*-------------------------------------------------------------------------------

messageLocalPlayersColor

Messages all local players with the message "message"
and color "color"

-------------------------------------------------------------------------------*/

void messageLocalPlayersColor(Uint32 color, char const * const message, ...)
{
	char str[Player::MessageZone_t::ADD_MESSAGE_BUFFER_LENGTH] = { 0 };

	va_list argptr;
	va_start(argptr, message);
	vsnprintf(str, Player::MessageZone_t::ADD_MESSAGE_BUFFER_LENGTH - 1, message, argptr);
	va_end(argptr);

	for ( int player = 0; player < MAXPLAYERS; ++player )
	{
		if ( players[player]->isLocalPlayer() )
		{
			messagePlayerColor(player, color, str);
		}
	}
}

/*-------------------------------------------------------------------------------

	messagePlayerColor

	Messages the player number given by "player" with the message "message"
	and color "color"

-------------------------------------------------------------------------------*/

void messagePlayerColor(int player, Uint32 color, char const * const message, ...)
{
	char str[Player::MessageZone_t::ADD_MESSAGE_BUFFER_LENGTH] = { 0 };
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
	vsnprintf( str, Player::MessageZone_t::ADD_MESSAGE_BUFFER_LENGTH - 1, message, argptr );
	va_end( argptr );

	// fixes crash when reading config at start of game
	if (!initialized)
	{
		printlog("%s\n", str);
		return;
	}

	if ( players[player]->isLocalPlayer() )
	{
		printlog("%s\n", str);
		strncpy(str, messageSanitizePercentSign(str, nullptr).c_str(), Player::MessageZone_t::ADD_MESSAGE_BUFFER_LENGTH - 1);
		newString(&messages, color, str);
		while ( list_Size(&messages) > MESSAGE_LIST_SIZE_CAP )
		{
			list_RemoveNode(messages.first);
		}
		if ( !disable_messages )
		{
			players[player]->messageZone.addMessage(color, str);
		}
	}
	else if ( multiplayer == SERVER && !players[player]->isLocalPlayer() )
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
	if ( client_disconnected[c] == true || players[c]->isLocalPlayer() )
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
	if ( client_disconnected[c] == true || players[c]->isLocalPlayer() )
	{
		return;
	}
	if ( c <= 0 )
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
	if ( client_disconnected[c] == true || players[c]->isLocalPlayer() )
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

	if ( client_disconnected[c] == true || players[c]->isLocalPlayer() )
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
		if ( client_disconnected[c] || players[c]->isLocalPlayer() )
		{
			continue;
		}
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

/*-------------------------------------------------------------------------------

	serverUpdateEntityBodypart

	Updates the given bodypart of the given entity for all clients

-------------------------------------------------------------------------------*/

//int numPlayerBodypartUpdates = 0;
//int numMonsterBodypartUpdates = 0;
//Uint32 lastbodypartTick = 0;

void serverUpdateEntityBodypart(Entity* entity, int bodypart)
{
	int c;
	if ( multiplayer != SERVER )
	{
		return;
	}
	for ( c = 1; c < MAXPLAYERS; c++ )
	{
		if ( client_disconnected[c] || players[c]->isLocalPlayer() )
		{
			continue;
		}
		strcpy((char*)net_packet->data, "ENTB");
		SDLNet_Write32(entity->getUID(), &net_packet->data[4]);
		net_packet->data[8] = bodypart;
		node_t* node = list_Node(&entity->children, bodypart);
		if ( !node )
		{
			continue;
		}
		Entity* tempEntity = (Entity*)node->element;
		SDLNet_Write32(tempEntity->sprite, &net_packet->data[9]);
		net_packet->data[13] = tempEntity->flags[INVISIBLE];
		net_packet->address.host = net_clients[c - 1].host;
		net_packet->address.port = net_clients[c - 1].port;
		net_packet->len = 14;
		sendPacketSafe(net_sock, -1, net_packet, c - 1);
	}
	//if ( entity->behavior == &actPlayer )
	//{
	//	++numPlayerBodypartUpdates;
	//}
	//else if ( entity->behavior == &actMonster )
	//{
	//	++numMonsterBodypartUpdates;
	//}
	//if ( lastbodypartTick == 0 )
	//{
	//	lastbodypartTick = ticks;
	//}
	//if ( ticks - lastbodypartTick >= 250 )
	//{
	//	messagePlayer(0, "Bodypart updates (%ds) players: %d, monster: %d", (ticks - lastbodypartTick) / 50, numPlayerBodypartUpdates, numMonsterBodypartUpdates);
	//	lastbodypartTick = 0;
	//	numMonsterBodypartUpdates = 0;
	//	numPlayerBodypartUpdates = 0;
	//}
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
		if ( client_disconnected[c] || players[c]->isLocalPlayer() )
		{
			continue;
		}
		strcpy((char*)net_packet->data, "ENTA");
		SDLNet_Write32(entity->getUID(), &net_packet->data[4]);
		SDLNet_Write32(entity->sprite, &net_packet->data[8]);
		net_packet->address.host = net_clients[c - 1].host;
		net_packet->address.port = net_clients[c - 1].port;
		net_packet->len = 12;
		sendPacketSafe(net_sock, -1, net_packet, c - 1);
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
		if ( client_disconnected[c] || players[c]->isLocalPlayer() )
		{
			continue;
		}
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

/*-------------------------------------------------------------------------------

serverUpdateEntitySkill

Updates a specific entity skill for all clients

-------------------------------------------------------------------------------*/

void serverUpdateEntityStatFlag(Entity* entity, int flag)
{
	int c;
	if ( multiplayer != SERVER )
	{
		return;
	}
	if ( !entity->getStats() )
	{
		return;
	}
	for ( c = 1; c < MAXPLAYERS; c++ )
	{
		if ( client_disconnected[c] || players[c]->isLocalPlayer() )
		{
			continue;
		}
		strcpy((char*)net_packet->data, "ENSF");
		SDLNet_Write32(entity->getUID(), &net_packet->data[4]);
		net_packet->data[8] = flag;
		SDLNet_Write32(entity->getStats()->MISC_FLAGS[flag], &net_packet->data[9]);
		net_packet->address.host = net_clients[c - 1].host;
		net_packet->address.port = net_clients[c - 1].port;
		net_packet->len = 13;
		sendPacketSafe(net_sock, -1, net_packet, c - 1);
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
		if ( client_disconnected[c] || players[c]->isLocalPlayer() )
		{
			continue;
		}
		strcpy((char*)net_packet->data, "ENFS");
		SDLNet_Write32(entity->getUID(), &net_packet->data[4]);
		net_packet->data[8] = fskill;
		SDLNet_Write16(static_cast<Sint16>(entity->fskill[fskill] * 256), &net_packet->data[9]);
		net_packet->address.host = net_clients[c - 1].host;
		net_packet->address.port = net_clients[c - 1].port;
		net_packet->len = 11;
		sendPacketSafe(net_sock, -1, net_packet, c - 1);
	}
}

/*-------------------------------------------------------------------------------

serverSpawnMiscParticles

Spawns misc particle effects for all clients

-------------------------------------------------------------------------------*/

void serverSpawnMiscParticles(Entity* entity, int particleType, int particleSprite, Uint32 optionalUid)
{
	int c;
	if ( multiplayer != SERVER )
	{
		return;
	}
	for ( c = 1; c < MAXPLAYERS; c++ )
	{
		if ( client_disconnected[c] || players[c]->isLocalPlayer() )
		{
			continue;
		}
		strcpy((char*)net_packet->data, "SPPE");
		SDLNet_Write32(entity->getUID(), &net_packet->data[4]);
		net_packet->data[8] = particleType;
		SDLNet_Write16(particleSprite, &net_packet->data[9]);
		SDLNet_Write32(optionalUid, &net_packet->data[11]);
		net_packet->address.host = net_clients[c - 1].host;
		net_packet->address.port = net_clients[c - 1].port;
		net_packet->len = 16;
		sendPacketSafe(net_sock, -1, net_packet, c - 1);
	}
}

/*-------------------------------------------------------------------------------

serverSpawnMiscParticlesAtLocation

Spawns misc particle effects for all clients at given coordinates.

-------------------------------------------------------------------------------*/

void serverSpawnMiscParticlesAtLocation(Sint16 x, Sint16 y, Sint16 z, int particleType, int particleSprite)
{
	int c;
	if ( multiplayer != SERVER )
	{
		return;
	}
	for ( c = 1; c < MAXPLAYERS; c++ )
	{
		if ( client_disconnected[c] || players[c]->isLocalPlayer() )
		{
			continue;
		}
		strcpy((char*)net_packet->data, "SPPL");
		SDLNet_Write16(x, &net_packet->data[4]);
		SDLNet_Write16(y, &net_packet->data[6]);
		SDLNet_Write16(z, &net_packet->data[8]);
		net_packet->data[10] = particleType;
		SDLNet_Write16(particleSprite, &net_packet->data[11]);
		net_packet->address.host = net_clients[c - 1].host;
		net_packet->address.port = net_clients[c - 1].port;
		net_packet->len = 14;
		sendPacketSafe(net_sock, -1, net_packet, c - 1);
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
		if ( client_disconnected[c] || players[c]->isLocalPlayer() )
		{
			continue;
		}
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
	if ( player <= 0 || player >= MAXPLAYERS )
	{
		return;
	}
	if ( client_disconnected[player] == true || players[player]->isLocalPlayer() )
	{
		return;
	}

	strcpy((char*)net_packet->data, "UPEF");
	net_packet->data[4] = 0;
	net_packet->data[5] = 0;
	net_packet->data[6] = 0;
	net_packet->data[7] = 0;
	net_packet->data[8] = 0;
	net_packet->data[9] = 0;
	net_packet->data[10] = 0;
	net_packet->data[11] = 0;
	net_packet->data[12] = 0;
	net_packet->data[13] = 0;
	for (j = 0; j < NUMEFFECTS; j++)
	{
		if ( stats[player]->EFFECTS[j] == true )
		{
			net_packet->data[4 + j / 8] |= power(2, j - (j / 8) * 8);
		}
		if ( stats[player]->EFFECTS_TIMERS[j] < TICKS_PER_SECOND * 5 && stats[player]->EFFECTS_TIMERS[j] > 0 )
		{
			// use these bits to denote if duration is low.
			net_packet->data[9 + j / 8] |= power(2, j - (j / 8) * 8);
		}
	}
	net_packet->address.host = net_clients[player - 1].host;
	net_packet->address.port = net_clients[player - 1].port;
	net_packet->len = 14;
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
	if ( player <= 0 || player >= MAXPLAYERS )
	{
		return;
	}
	if ( client_disconnected[player] == true || players[player]->isLocalPlayer() )
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

serverUpdatePlayerStats

Updates all player current HP/MP for clients

-------------------------------------------------------------------------------*/

void serverUpdatePlayerStats()
{
	int c;
	if ( multiplayer != SERVER )
	{
		return;
	}
	for ( c = 1; c < MAXPLAYERS; c++ )
	{
		if ( client_disconnected[c] || players[c]->isLocalPlayer() )
		{
			continue;
		}
		strcpy((char*)net_packet->data, "STAT");
		Sint32 playerHP = 0;
		Sint32 playerMP = 0;
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( stats[i] )
			{
				playerHP = static_cast<Sint16>(stats[i]->MAXHP);
				playerHP |= static_cast<Sint16>(stats[i]->HP) << 16;
				playerMP = static_cast<Sint16>(stats[i]->MAXMP);
				playerMP |= static_cast<Sint16>(stats[i]->MP) << 16;
			}
			SDLNet_Write32(playerHP, &net_packet->data[4 + i * 8]); // 4/12/20/28 data
			SDLNet_Write32(playerMP, &net_packet->data[8 + i * 8]); // 8/16/24/32 data
			playerHP = 0;
			playerMP = 0;
		}
		net_packet->address.host = net_clients[c - 1].host;
		net_packet->address.port = net_clients[c - 1].port;
		net_packet->len = 4 + 8 * MAXPLAYERS;
		sendPacketSafe(net_sock, -1, net_packet, c - 1);
	}
}

/*-------------------------------------------------------------------------------

serverUpdatePlayerGameplayStats

Updates given players gameplayStatistics value by given increment.

-------------------------------------------------------------------------------*/
void serverUpdatePlayerGameplayStats(int player, int gameplayStat, int changeval)
{
	if ( player < 0 || player >= MAXPLAYERS )
	{
		return;
	}
	if ( client_disconnected[player] )
	{
		return;
	}
	if ( player == 0 )
	{
		if ( gameplayStat == STATISTICS_TEMPT_FATE )
		{
			if ( gameStatistics[STATISTICS_TEMPT_FATE] == -1 )
			{
				// don't change, completed task.
			}
			else
			{
				if ( changeval == 5 )
				{
					gameStatistics[gameplayStat] = changeval;
				}
				else if ( changeval == 1 && gameStatistics[gameplayStat] > 0 )
				{
					gameStatistics[gameplayStat] = -1;
				}
			}
		}
		else if ( gameplayStat == STATISTICS_FORUM_TROLL )
		{
			if ( changeval == AchievementObserver::FORUM_TROLL_BREAK_WALL )
			{
				int walls = gameStatistics[gameplayStat] & 0xFF;
				walls = std::min(walls + 1, 3);
				gameStatistics[gameplayStat] = gameStatistics[gameplayStat] & 0xFFFFFF00;
				gameStatistics[gameplayStat] |= walls;
			}
			else if ( changeval == AchievementObserver::FORUM_TROLL_RECRUIT_TROLL )
			{
				int trolls = (gameStatistics[gameplayStat] >> 8) & 0xFF;
				trolls = std::min(trolls + 1, 3);
				gameStatistics[gameplayStat] = gameStatistics[gameplayStat] & 0xFFFF00FF;
				gameStatistics[gameplayStat] |= (trolls << 8);
			}
			else if ( changeval == AchievementObserver::FORUM_TROLL_FEAR )
			{
				int fears = (gameStatistics[gameplayStat] >> 16) & 0xFF;
				fears = std::min(fears + 1, 3);
				gameStatistics[gameplayStat] = gameStatistics[gameplayStat] & 0xFF00FFFF;
				gameStatistics[gameplayStat] |= (fears << 16);
			}
		}
		else if ( gameplayStat == STATISTICS_POP_QUIZ_1 || gameplayStat == STATISTICS_POP_QUIZ_2 )
		{
			int spellID = changeval;
			if ( spellID >= 30 )
			{
				spellID -= 30;
				int shifted = (1 << spellID);
				gameStatistics[gameplayStat] |= shifted;
			}
			else
			{
				int shifted = (1 << spellID);
				gameStatistics[gameplayStat] |= shifted;
			}
		}
		else
		{
			gameStatistics[gameplayStat] += changeval;
		}
	}
	else if ( !players[player]->isLocalPlayer() )
	{
		strcpy((char*)net_packet->data, "GPST");
		SDLNet_Write32(gameplayStat, &net_packet->data[4]);
		SDLNet_Write32(changeval, &net_packet->data[8]);
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 12;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
	}
	//messagePlayer(clientnum, "[DEBUG]: sent: %d, %d: val %d", gameplayStat, changeval, gameStatistics[gameplayStat]);
}

void serverUpdatePlayerConduct(int player, int conduct, int value)
{
	if ( player <= 0 || player >= MAXPLAYERS )
	{
		return;
	}
	if ( client_disconnected[player] || players[player]->isLocalPlayer() )
	{
		return;
	}
	strcpy((char*)net_packet->data, "COND");
	SDLNet_Write16(conduct, &net_packet->data[4]);
	SDLNet_Write16(value, &net_packet->data[6]);
	net_packet->address.host = net_clients[player - 1].host;
	net_packet->address.port = net_clients[player - 1].port;
	net_packet->len = 8;
	sendPacketSafe(net_sock, -1, net_packet, player - 1);
}

/*-------------------------------------------------------------------------------

serverUpdatePlayerLVL

Updates all player current LVL for clients

-------------------------------------------------------------------------------*/

void serverUpdatePlayerLVL()
{
	int c;
	if ( multiplayer != SERVER )
	{
		return;
	}
	for ( c = 1; c < MAXPLAYERS; c++ )
	{
		if ( client_disconnected[c] || players[c]->isLocalPlayer() )
		{
			continue;
		}
		strcpy((char*)net_packet->data, "UPLV");
		Sint32 playerLevels = 0;
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( stats[i] )
			{
				playerLevels |= static_cast<Uint8>(stats[i]->LVL) << (8 * i); // store uint8 in data, highest bits for player 4.
			}
		}
		SDLNet_Write32(playerLevels, &net_packet->data[4]);
		net_packet->address.host = net_clients[c - 1].host;
		net_packet->address.port = net_clients[c - 1].port;
		net_packet->len = 8;
		sendPacketSafe(net_sock, -1, net_packet, c - 1);
	}
}

void serverRemoveClientFollower(int player, Uint32 uidToRemove)
{
	if ( multiplayer != SERVER || player <= 0 )
	{
		return;
	}
	if ( client_disconnected[player] || players[player]->isLocalPlayer() )
	{
		return;
	}

	strcpy((char*)net_packet->data, "LDEL");
	SDLNet_Write32(uidToRemove, &net_packet->data[4]);
	net_packet->address.host = net_clients[player - 1].host;
	net_packet->address.port = net_clients[player - 1].port;
	net_packet->len = 8;
	sendPacketSafe(net_sock, -1, net_packet, player - 1);
}

void serverSendItemToPickupAndEquip(int player, Item* item)
{
	if ( multiplayer != SERVER || player <= 0 )
	{
		return;
	}
	if ( client_disconnected[player] || players[player]->isLocalPlayer() )
	{
		return;
	}

	// send the client info on the item it just picked up
	strcpy((char*)net_packet->data, "ITEQ");
	SDLNet_Write32((Uint32)item->type, &net_packet->data[4]);
	SDLNet_Write32((Uint32)item->status, &net_packet->data[8]);
	SDLNet_Write32((Uint32)item->beatitude, &net_packet->data[12]);
	SDLNet_Write32((Uint32)item->count, &net_packet->data[16]);
	SDLNet_Write32((Uint32)item->appearance, &net_packet->data[20]);
	SDLNet_Write32((Uint32)item->ownerUid, &net_packet->data[24]);
	net_packet->data[28] = item->identified;
	net_packet->address.host = net_clients[player - 1].host;
	net_packet->address.port = net_clients[player - 1].port;
	net_packet->len = 29;
	sendPacketSafe(net_sock, -1, net_packet, player - 1);
}

void serverUpdateAllyStat(int player, Uint32 uidToUpdate, int LVL, int HP, int MAXHP, int type)
{
	if ( multiplayer != SERVER || player <= 0 )
	{
		return;
	}
	if ( client_disconnected[player] || players[player]->isLocalPlayer() )
	{
		return;
	}

	strcpy((char*)net_packet->data, "NPCI");
	SDLNet_Write32(uidToUpdate, &net_packet->data[4]);
	net_packet->data[8] = static_cast<Uint8>(LVL);
	SDLNet_Write16(HP, &net_packet->data[9]);
	SDLNet_Write16(MAXHP, &net_packet->data[11]);
	net_packet->data[13] = static_cast<Uint8>(type);
	net_packet->address.host = net_clients[player - 1].host;
	net_packet->address.port = net_clients[player - 1].port;
	net_packet->len = 14;
	sendPacketSafe(net_sock, -1, net_packet, player - 1);
}

void serverUpdatePlayerSummonStrength(int player)
{
	if ( multiplayer != SERVER )
	{
		return;
	}
	if ( player <= 0 || player > MAXPLAYERS )
	{
		return;
	}
	if ( client_disconnected[player] || !stats[player] || players[player]->isLocalPlayer() )
	{
		return;
	}

	strcpy((char*)net_packet->data, "SUMS");
	SDLNet_Write32(stats[player]->playerSummonLVLHP, &net_packet->data[4]);
	SDLNet_Write32(stats[player]->playerSummonSTRDEXCONINT, &net_packet->data[8]);
	SDLNet_Write32(stats[player]->playerSummonPERCHR, &net_packet->data[12]);
	SDLNet_Write32(stats[player]->playerSummon2LVLHP, &net_packet->data[16]);
	SDLNet_Write32(stats[player]->playerSummon2STRDEXCONINT, &net_packet->data[20]);
	SDLNet_Write32(stats[player]->playerSummon2PERCHR, &net_packet->data[24]);
	net_packet->address.host = net_clients[player - 1].host;
	net_packet->address.port = net_clients[player - 1].port;
	net_packet->len = 28;
	sendPacketSafe(net_sock, -1, net_packet, player - 1);
}

void serverUpdateAllyHP(int player, Uint32 uidToUpdate, int HP, int MAXHP, bool guarantee)
{
	if ( multiplayer != SERVER )
	{
		return;
	}
	if ( player <= 0 )
	{
		return;
	}
	if ( client_disconnected[player] || players[player]->isLocalPlayer() )
	{
		return;
	}

	strcpy((char*)net_packet->data, "NPCU");
	SDLNet_Write32(uidToUpdate, &net_packet->data[4]);
	SDLNet_Write16(HP, &net_packet->data[8]);
	SDLNet_Write16(MAXHP, &net_packet->data[10]);
	net_packet->address.host = net_clients[player - 1].host;
	net_packet->address.port = net_clients[player - 1].port;
	net_packet->len = 12;
	if ( !guarantee )
	{
		sendPacket(net_sock, -1, net_packet, player - 1);
	}
	else
	{
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
	}
}

void sendMinimapPing(Uint8 player, Uint8 x, Uint8 y)
{
	if ( multiplayer == CLIENT )
	{
		// send to host to relay info.
		strcpy((char*)net_packet->data, "PMAP"); 
		net_packet->data[4] = player;
		net_packet->data[5] = x;
		net_packet->data[6] = y;

		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 7;
		sendPacket(net_sock, -1, net_packet, 0);
	}
	else
	{
		for ( int c = 0; c < MAXPLAYERS; c++ )
		{
			if ( client_disconnected[c] )
			{
				continue;
			}
			if ( players[c]->isLocalPlayer() )
			{
				minimapPingAdd(player, c, MinimapPing(ticks, player, x, y));
				continue;
			}

			if ( multiplayer == SERVER )
			{
				// send to all clients.
				strcpy((char*)net_packet->data, "PMAP");
				net_packet->data[4] = player;
				net_packet->data[5] = x;
				net_packet->data[6] = y;

				net_packet->address.host = net_clients[c - 1].host;
				net_packet->address.port = net_clients[c - 1].port;
				net_packet->len = 7;
				sendPacket(net_sock, -1, net_packet, c - 1);
			}
		}
	}
}

void sendAllyCommandClient(int player, Uint32 uid, int command, Uint8 x, Uint8 y, Uint32 targetUid)
{
	if ( multiplayer != CLIENT )
	{
		return;
	}
	//messagePlayer(clientnum, "%d", targetUid);

	// send to host.
	strcpy((char*)net_packet->data, "ALLY");
	net_packet->data[4] = player;
	net_packet->data[5] = command;
	net_packet->data[6] = x;
	net_packet->data[7] = y;
	SDLNet_Write32(uid, &net_packet->data[8]);
	net_packet->len = 12;
	if ( targetUid != 0 )
	{
		SDLNet_Write32(targetUid, &net_packet->data[12]);
		net_packet->len = 16;
	}
	net_packet->address.host = net_server.host;
	net_packet->address.port = net_server.port;
	sendPacket(net_sock, -1, net_packet, 0);
}

NetworkingLobbyJoinRequestResult lobbyPlayerJoinRequest(int& outResult)
{
	int c = MAXPLAYERS;
	if ( strcmp(VERSION, (char*)net_packet->data + 54) )
	{
		c = MAXPLAYERS + 1; // wrong version number
	}
	else
	{
		Uint32 clientlsg = SDLNet_Read32(&net_packet->data[68]);
		Uint32 clientms = SDLNet_Read32(&net_packet->data[64]);
		if ( net_packet->data[63] == 0 )
		{
			// client will enter any player spot
			for ( c = 0; c < MAXPLAYERS; c++ )
			{
				if ( client_disconnected[c] == true )
				{
					break;    // no more player slots
				}
			}
		}
		else
		{
			// client is joining a particular player spot
			c = net_packet->data[63];
			if ( !client_disconnected[c] )
			{
				c = MAXPLAYERS;  // client wants to fill a space that is already filled
			}
		}
		if ( clientlsg != loadingsavegame && loadingsavegame == 0 )
		{
			c = MAXPLAYERS + 2;  // client shouldn't load save game
		}
		else if ( clientlsg == 0 && loadingsavegame != 0 )
		{
			c = MAXPLAYERS + 3;  // client is trying to join a save game without a save of their own
		}
		else if ( clientlsg != loadingsavegame )
		{
			c = MAXPLAYERS + 4;  // client is trying to join the game with an incompatible save
		}
		else if ( loadingsavegame && getSaveGameMapSeed(false) != clientms )
		{
			c = MAXPLAYERS + 5;  // client is trying to join the game with a slightly incompatible save (wrong level)
		}
	}
	outResult = c;
	if ( c >= MAXPLAYERS )
	{
		// on error, client gets a player number that is invalid (to be interpreted as an error code)
		net_clients[MAXPLAYERS - 1].host = net_packet->address.host;
		net_clients[MAXPLAYERS - 1].port = net_packet->address.port;
		if ( directConnect )
			while ( (net_tcpclients[MAXPLAYERS - 1] = SDLNet_TCP_Accept(net_tcpsock)) == NULL );
		net_packet->address.host = net_clients[MAXPLAYERS - 1].host;
		net_packet->address.port = net_clients[MAXPLAYERS - 1].port;
		net_packet->len = 4;
		SDLNet_Write32(c, &net_packet->data[0]); // error code for client to interpret
		printlog("sending error code %d to client.\n", c);
		if ( directConnect )
		{
			SDLNet_TCP_Send(net_tcpclients[MAXPLAYERS - 1], net_packet->data, net_packet->len);
			SDLNet_TCP_Close(net_tcpclients[MAXPLAYERS - 1]);
			return NET_LOBBY_JOIN_DIRECTIP_FAILURE;
		}
		else
		{
			return NET_LOBBY_JOIN_P2P_FAILURE;
		}
	}
	else
	{
		// on success, client gets legit player number
		strcpy(stats[c]->name, (char*)(&net_packet->data[19]));
		client_disconnected[c] = false;
		client_classes[c] = (int)SDLNet_Read32(&net_packet->data[42]);
		stats[c]->sex = static_cast<sex_t>((int)SDLNet_Read32(&net_packet->data[46]));
		Uint32 raceAndAppearance = (Uint32)SDLNet_Read32(&net_packet->data[50]);
		stats[c]->appearance = (raceAndAppearance & 0xFF00) >> 8;
		stats[c]->playerRace = (raceAndAppearance & 0xFF);
		net_clients[c - 1].host = net_packet->address.host;
		net_clients[c - 1].port = net_packet->address.port;
		if ( directConnect )
		{
			while ( (net_tcpclients[c - 1] = SDLNet_TCP_Accept(net_tcpsock)) == NULL );
			const char* clientaddr = SDLNet_ResolveIP(&net_packet->address);
			printlog("client %d connected from %s:%d\n", c, clientaddr, net_packet->address.port);
		}
		else
		{
			printlog("client %d connected.\n", c);
		}
		client_keepalive[c] = ticks;

		// send existing clients info on new client
		for ( int x = 1; x < MAXPLAYERS; x++ )
		{
			if ( client_disconnected[x] || c == x )
			{
				continue;
			}
			strcpy((char*)(&net_packet->data[0]), "NEWPLAYER");
			net_packet->data[9] = c; // clientnum
			net_packet->data[10] = client_classes[c]; // class
			net_packet->data[11] = stats[c]->sex; // sex
			net_packet->data[12] = (Uint8)stats[c]->appearance; // appearance
			net_packet->data[13] = (Uint8)stats[c]->playerRace; // player race
			char shortname[32] = "";
			strncpy(shortname, stats[c]->name, 22);
			strcpy((char*)(&net_packet->data[14]), shortname);  // name
			net_packet->address.host = net_clients[x - 1].host;
			net_packet->address.port = net_clients[x - 1].port;
			net_packet->len = 14 + strlen(stats[c]->name) + 1;
			sendPacketSafe(net_sock, -1, net_packet, x - 1);
		}
		char shortname[32] = { 0 };
		strncpy(shortname, stats[c]->name, 22);

		newString(&lobbyChatboxMessages, 0xFFFFFFFF, "\n***   %s has joined the game   ***\n", shortname);

		// send new client their id number + info on other clients
		SDLNet_Write32(c, &net_packet->data[0]);
		for ( int x = 0; x < MAXPLAYERS; x++ )
		{
			net_packet->data[4 + x * (5 + 23)] = client_classes[x]; // class
			net_packet->data[5 + x * (5 + 23)] = stats[x]->sex; // sex
			net_packet->data[6 + x * (5 + 23)] = client_disconnected[x]; // connectedness :p
			net_packet->data[7 + x * (5 + 23)] = (Uint8)stats[x]->appearance; // appearance
			net_packet->data[8 + x * (5 + 23)] = (Uint8)stats[x]->playerRace; // player race
			char shortname[32] = "";
			strncpy(shortname, stats[x]->name, 22);
			strcpy((char*)(&net_packet->data[9 + x * (5 + 23)]), shortname);  // name
		}
		net_packet->address.host = net_clients[c - 1].host;
		net_packet->address.port = net_clients[c - 1].port;
		net_packet->len = 4 + MAXPLAYERS * (5 + 23);
		if ( directConnect )
		{
			SDLNet_TCP_Send(net_tcpclients[c - 1], net_packet->data, net_packet->len);
			return NET_LOBBY_JOIN_DIRECTIP_SUCCESS;
		}
		else
		{
			return NET_LOBBY_JOIN_P2P_SUCCESS;
		}
	}
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

	if ( entity == nullptr )
	{
		newentity = true;
		entity = newEntity((int)SDLNet_Read16(&net_packet->data[8]), 0, map.entities, nullptr);
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
	for (c = 0; c < 16; ++c)
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
		case 160:
		case 203:
		case 212:
		case 213:
		case 214:
		case 682:
		case 681:
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
		case 586:
			entity->behavior = &actSwitchWithTimer;
			break;
		case 601:
			entity->behavior = &actPedestalBase;
			break;
		case 602:
		case 603:
		case 604:
		case 605:
			entity->behavior = &actPedestalOrb;
			break;
		case 667:
		case 668:
			entity->behavior = &actBeartrap;
			break;
		case 674:
		case 675:
		case 676:
		case 677:
			entity->behavior = &actCeilingTile;
			entity->flags[NOUPDATE] = true;
			break;
		case 629:
			entity->behavior = &actColumn;
			entity->flags[NOUPDATE] = true;
			break;
		case 632:
		case 633:
			entity->behavior = &actPistonCam;
			entity->flags[NOUPDATE] = true;
			break;
		case 130:
			entity->behavior = &actGoldBag;
			break;
		default:
			if ( entity->isPlayerHeadSprite() )
			{
				// these are all player heads
				playernum = SDLNet_Read32(&net_packet->data[30]);
				if ( playernum >= 0 && playernum < MAXPLAYERS )
				{
					if ( players[playernum] && players[playernum]->entity )
					{
						players[playernum]->entity = entity;
					}
					entity->skill[2] = playernum;
					entity->behavior = &actPlayer;
				}
			}
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
					if ( entity->sprite == 989 ) // boulder_lava.vox
					{
						entity->flags[BURNABLE] = true;
					}
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
				case -13:
					entity->behavior = &actParticleSapCenter;
					break;
				case -14:
					entity->behavior = &actDecoyBox;
					break;
				case -15:
					entity->behavior = &actBomb;
					break;
				case -16:
					entity->behavior = &actBoulder;
					break;
				default:
					if ( c < -1000 && c > -2000 )
					{
						entity->arrowShotByWeapon = -(c + 1000);
						entity->behavior = &actArrow;
					}
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

	node_t* node;
	node_t* nextnode;
	Entity* entity, *entity2;
	int c = 0;
	Uint32 i = 0, j;
	Item* item = NULL;

#ifdef PACKETINFO
	char packetinfo[NET_PACKET_SIZE];
	strncpy( packetinfo, (char*)net_packet->data, net_packet->len );
	packetinfo[net_packet->len] = 0;
	printlog("info: client packet: %s\n", packetinfo);
#endif
	if ( logCheckMainLoopTimers )
	{
		char packetinfo[NET_PACKET_SIZE];
		strncpy(packetinfo, (char*)net_packet->data, net_packet->len);
		packetinfo[net_packet->len] = 0;

		char packetHeader[5];
		strncpy(packetHeader, packetinfo, 4);
		packetHeader[4] = 0;

		std::string tmp = packetHeader;
		unsigned long hash = djb2Hash(packetHeader);
		auto find = DebugStats.networkPackets.find(hash);
		if ( find != DebugStats.networkPackets.end() )
		{
			++DebugStats.networkPackets[hash].second;
		}
		else
		{
			DebugStats.networkPackets.insert(std::make_pair(hash, std::make_pair(tmp, 0)));
			messagePlayer(clientnum, "%s", tmp.c_str());
		}
		if ( !strcmp(packetinfo, "ENTU") )
		{
			int sprite = 0;
			Uint32 uidpacket = static_cast<Uint32>(SDLNet_Read32(&net_packet->data[4]));
			if ( uidToEntity(uidpacket) )
			{
				sprite = uidToEntity(uidpacket)->sprite;
				auto find = DebugStats.entityUpdatePackets.find(sprite);
				if ( find != DebugStats.entityUpdatePackets.end() )
				{
					++DebugStats.entityUpdatePackets[sprite];
				}
				else
				{
					DebugStats.entityUpdatePackets.insert(std::make_pair(sprite, 1));
				}
			}
		}
	}

	// keep alive
	if (!strncmp((char*)net_packet->data, "KPAL", 4))
	{
		client_keepalive[0] = ticks;
		return;
	}

	// entity update
	else if ( !strncmp((char*)net_packet->data, "ENTU", 4) )
	{
		client_keepalive[0] = ticks; // don't timeout
		Entity *entity = uidToEntity((int)SDLNet_Read32(&net_packet->data[4]));
		if ( entity )
		{
			if ( (Uint32)SDLNet_Read32(&net_packet->data[36]) < (Uint32)entity->lastupdateserver )
			{
				// old packet, not used
			}
			else if ( entity->behavior == &actPlayer && entity->skill[2] == clientnum )
			{
				// don't update my player
			}
			else if ( entity->flags[NOUPDATE] )
			{
				// inform the server that it tried to update a no-update entity
				strcpy((char*)net_packet->data, "NOUP");
				net_packet->data[4] = clientnum;
				SDLNet_Write32(entity->getUID(), &net_packet->data[5]);
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 9;
				sendPacket(net_sock, -1, net_packet, 0);
			}
			else
			{
				// receive the entity
				receiveEntity(entity);
				entity->behavior = NULL;
				clientActions(entity);
			}
			return;
		}

		for ( node = removedEntities.first; node != NULL; node = node->next )
		{
			entity2 = (Entity*)node->element;
			if ( entity2->getUID() == (int)SDLNet_Read32(&net_packet->data[4]) )
			{
				return;
			}
		}

		entity = receiveEntity(NULL);
		// IMPORTANT! Assign actions to the objects the client has control over
		clientActions(entity);
		return;
	}

	else if ( !strncmp((char*)net_packet->data, "EFFE", 4) )
	{
		/*
		* Packet breakdown:
		* [0][1][2][3]: "EFFE"
		* [4][5][6][7]: Entity's UID.
		* [8][9][10][11]: Entity's effects.
		*/

		Uint32 uid = static_cast<int>(SDLNet_Read32(&net_packet->data[4]));

		Entity* entity = uidToEntity(uid);

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
				if ( net_packet->data[8 + i / 8] & power(2, i - (i / 8) * 8) )
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

	// update entity skill
	else if ( !strncmp((char*)net_packet->data, "ENTS", 4) )
	{
		Entity *entity = uidToEntity((int)SDLNet_Read32(&net_packet->data[4]));
		if ( entity )
		{
			entity->skill[net_packet->data[8]] = SDLNet_Read32(&net_packet->data[9]);
		}
		return;
	}

	// update entity fskill
	else if ( !strncmp((char*)net_packet->data, "ENFS", 4) )
	{
		Entity *entity = uidToEntity((int)SDLNet_Read32(&net_packet->data[4]));
		if ( entity )
		{
			entity->fskill[net_packet->data[8]] = (SDLNet_Read16(&net_packet->data[9]) / 256.0);
		}
		return;
	}

	// update entity bodypart
	else if ( !strncmp((char*)net_packet->data, "ENTB", 4) )
	{
		Entity *entity = uidToEntity((int)SDLNet_Read32(&net_packet->data[4]));
		if ( entity )
		{
			node_t* childNode = list_Node(&entity->children, net_packet->data[8]);
			if ( childNode )
			{
				Entity* tempEntity = (Entity*)childNode->element;
				tempEntity->sprite = SDLNet_Read32(&net_packet->data[9]);
				tempEntity->skill[7] = tempEntity->sprite;
				tempEntity->flags[INVISIBLE] = net_packet->data[13];
			}
		}
		return;
	}

	// bodypart ids
	else if ( !strncmp((char*)net_packet->data, "BDYI", 4) )
	{
		Entity *entity = uidToEntity((int)SDLNet_Read32(&net_packet->data[4]));
		if ( entity )
		{
			node_t* childNode;
			int c;
			for ( c = 0, childNode = entity->children.first; childNode != nullptr; childNode = childNode->next, c++ )
			{
				if ( c < 1 || (c < 2 && entity->behavior == &actMonster) )
				{
					continue;
				}
				Entity* tempEntity = (Entity*)childNode->element;
				if ( tempEntity )
				{
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

	// update entity flag
	else if ( !strncmp((char*)net_packet->data, "ENTF", 4) )
	{
		Entity *entity = uidToEntity((int)SDLNet_Read32(&net_packet->data[4]));
		if ( entity )
		{
			entity->flags[net_packet->data[8]] = net_packet->data[9];
			if ( entity->behavior == &actMonster && net_packet->data[8] == USERFLAG2 )
			{
				// we should update the flags for all bodyparts (except for human and automaton heads, don't update the other bodyparts).
				if ( !(entity->isPlayerHeadSprite() || entity->sprite == 467 || !monsterChangesColorWhenAlly(nullptr, entity)) )
				{
					int bodypart = 0;
					for ( node_t* node = entity->children.first; node != nullptr; node = node->next )
					{
						if ( bodypart >= LIMB_HUMANOID_TORSO )
						{
							Entity* tmp = (Entity*)node->element;
							if ( tmp )
							{
								tmp->flags[USERFLAG2] = entity->flags[net_packet->data[8]];
							}
						}
						++bodypart;
					}
				}
			}
		}
		return;
	}

	// player movement correction
	else if ( !strncmp((char*)net_packet->data, "PMOV", 4) )
	{
		if ( players[clientnum] == nullptr || players[clientnum]->entity == nullptr )
		{
			return;
		}
		players[clientnum]->entity->x = ((Sint16)SDLNet_Read16(&net_packet->data[4])) / 32.0;
		players[clientnum]->entity->y = ((Sint16)SDLNet_Read16(&net_packet->data[6])) / 32.0;
		return;
	}

	// update health
	else if ( !strncmp((char*)net_packet->data, "UPHP", 4) )
	{
		if ( (Monster)SDLNet_Read32(&net_packet->data[8]) != NOTHING )
		{
			if ( SDLNet_Read32(&net_packet->data[4]) < stats[clientnum]->HP )
			{
				cameravars[clientnum].shakex += .1;
				cameravars[clientnum].shakey += 10;
			}
			else
			{
				cameravars[clientnum].shakex += .05;
				cameravars[clientnum].shakey += 5;
			}
		}
		stats[clientnum]->HP = SDLNet_Read32(&net_packet->data[4]);
		return;
	}

	// server sent item details.
	else if ( !strncmp((char*)net_packet->data, "ITMU", 4) )
	{
		Uint32 uid = SDLNet_Read32(&net_packet->data[4]);
		Entity* entity = uidToEntity(uid);
		if ( entity )
		{
			Uint32 itemTypeAndIdentified = SDLNet_Read32(&net_packet->data[8]);
			Uint32 statusBeatitudeQuantityAppearance = SDLNet_Read32(&net_packet->data[12]);

			entity->skill[10] = static_cast<ItemType>((itemTypeAndIdentified >> 16) & 0xFFFF); //type
			entity->skill[15] = (itemTypeAndIdentified) & 0xFFFF;
			entity->skill[11] = static_cast<Uint8>((statusBeatitudeQuantityAppearance >> 24) & 0xFF); // status
			entity->skill[12] = static_cast<Sint8>((statusBeatitudeQuantityAppearance >> 16) & 0xFF); // beatitude
			entity->skill[13] = static_cast<Uint8>((statusBeatitudeQuantityAppearance >> 8) & 0xFF); // quantity
			entity->skill[14] = static_cast<Uint8>((statusBeatitudeQuantityAppearance) & 0xFF); // appearance
			entity->itemReceivedDetailsFromServer = 1;
		}
		return;
	}


	// spawn an explosion
	else if ( !strncmp((char*)net_packet->data, "EXPL", 4) )
	{
		Sint16 x = (Sint16)SDLNet_Read16(&net_packet->data[4]);
		Sint16 y = (Sint16)SDLNet_Read16(&net_packet->data[6]);
		Sint16 z = (Sint16)SDLNet_Read16(&net_packet->data[8]);
		spawnExplosion(x, y, z);
		return;
	}

	// spawn an explosion, custom sprite
	else if ( !strncmp((char*)net_packet->data, "EXPS", 4) )
	{
		Uint16 sprite = (Uint16)SDLNet_Read16(&net_packet->data[4]);
		Sint16 x = (Sint16)SDLNet_Read16(&net_packet->data[6]);
		Sint16 y = (Sint16)SDLNet_Read16(&net_packet->data[8]);
		Sint16 z = (Sint16)SDLNet_Read16(&net_packet->data[10]);
		spawnExplosionFromSprite(sprite, x, y, z);
		return;
	}

	// spawn a bang sprite
	else if ( !strncmp((char*)net_packet->data, "BANG", 4) )
	{
		Sint16 x = (Sint16)SDLNet_Read16(&net_packet->data[4]);
		Sint16 y = (Sint16)SDLNet_Read16(&net_packet->data[6]);
		Sint16 z = (Sint16)SDLNet_Read16(&net_packet->data[8]);
		spawnBang(x, y, z);
		return;
	}

	// spawn a gib
	else if ( !strncmp((char*)net_packet->data, "SPGB", 4) )
	{
		Sint16 x = (Sint16)SDLNet_Read16(&net_packet->data[4]);
		Sint16 y = (Sint16)SDLNet_Read16(&net_packet->data[6]);
		Sint16 z = (Sint16)SDLNet_Read16(&net_packet->data[8]);
		Sint16 sprite = (Sint16)SDLNet_Read16(&net_packet->data[10]);
		Entity* gib = spawnGibClient(x, y, z, sprite);
		gib->flags[SPRITE] = net_packet->data[12];
		if ( !spawn_blood && (!gib->flags[SPRITE] || gib->sprite != 29) )
		{
			gib->flags[INVISIBLE] = true;
		}
		return;
	}

	// spawn a sleep Z
	else if ( !strncmp((char*)net_packet->data, "SLEZ", 4) )
	{
		Sint16 x = (Sint16)SDLNet_Read16(&net_packet->data[4]);
		Sint16 y = (Sint16)SDLNet_Read16(&net_packet->data[6]);
		Sint16 z = (Sint16)SDLNet_Read16(&net_packet->data[8]);
		spawnSleepZ(x, y, z);
		return;
	}

	// spawn a misc sprite like the sleep Z
	else if ( !strncmp((char*)net_packet->data, "SLEM", 4) )
	{
		Sint16 x = (Sint16)SDLNet_Read16(&net_packet->data[4]);
		Sint16 y = (Sint16)SDLNet_Read16(&net_packet->data[6]);
		Sint16 z = (Sint16)SDLNet_Read16(&net_packet->data[8]);
		Sint16 sprite = (Sint16)SDLNet_Read16(&net_packet->data[10]);
		spawnFloatingSpriteMisc(sprite, x, y, z);
		return;
	}

	// spawn magical effect particles
	else if ( !strncmp((char*)net_packet->data, "MAGE", 4) )
	{
		Sint16 x = (Sint16)SDLNet_Read16(&net_packet->data[4]);
		Sint16 y = (Sint16)SDLNet_Read16(&net_packet->data[6]);
		Sint16 z = (Sint16)SDLNet_Read16(&net_packet->data[8]);
		Uint32 sprite = (Uint32)SDLNet_Read32(&net_packet->data[10]);
		spawnMagicEffectParticles(x, y, z, sprite);
		return;
	}

	// spawn misc particle effect 
	else if ( !strncmp((char*)net_packet->data, "SPPE", 4) )
	{
		Entity *entity = uidToEntity((int)SDLNet_Read32(&net_packet->data[4]));
		if ( entity )
		{
			int particleType = static_cast<int>(net_packet->data[8]);
			int sprite = static_cast<int>(SDLNet_Read16(&net_packet->data[9]));
			switch ( particleType )
			{
				case PARTICLE_EFFECT_ABILITY_PURPLE:
					createParticleDot(entity);
					break;
				case PARTICLE_EFFECT_ABILITY_ROCK:
					createParticleRock(entity);
					break;
				case PARTICLE_EFFECT_SHADOW_INVIS:
					createParticleDropRising(entity, sprite, 1.0);
					break;
				case PARTICLE_EFFECT_INCUBUS_TELEPORT_STEAL:
				{
					Entity* spellTimer = createParticleTimer(entity, 80, sprite);
					spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SHOOT_PARTICLES;
					spellTimer->particleTimerCountdownSprite = sprite;
					spellTimer->particleTimerPreDelay = 40;
				}
				break;
				case PARTICLE_EFFECT_INCUBUS_TELEPORT_TARGET:
				{
					Entity* spellTimer = createParticleTimer(entity, 40, sprite);
					spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SHOOT_PARTICLES;
					spellTimer->particleTimerCountdownSprite = sprite;
				}
				break;
				case PARTICLE_EFFECT_SHADOW_TELEPORT:
				{
					Entity* spellTimer = createParticleTimer(entity, 40, sprite);
					spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SHOOT_PARTICLES;
					spellTimer->particleTimerCountdownSprite = sprite;
				}
				break;
				case PARTICLE_EFFECT_TELEPORT_PULL:
				{
					Entity* spellTimer = createParticleTimer(entity, 40, sprite);
					spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SHOOT_PARTICLES;
					spellTimer->particleTimerCountdownSprite = sprite;
				}
				break;
				case PARTICLE_EFFECT_ERUPT:
					createParticleErupt(entity, sprite);
					break;
				case PARTICLE_EFFECT_VAMPIRIC_AURA:
					createParticleDropRising(entity, sprite, 0.5);
					break;
				case PARTICLE_EFFECT_RISING_DROP:
					createParticleDropRising(entity, sprite, 1.0);
					break;
				case PARTICLE_EFFECT_CHARM_MONSTER:
					createParticleCharmMonster(entity);
					break;
				case PARTICLE_EFFECT_SHADOW_TAG:
				{
					Uint32 uid = SDLNet_Read32(&net_packet->data[11]);
					createParticleShadowTag(entity, uid, 60 * TICKS_PER_SECOND);
					break;
				}
				case PARTICLE_EFFECT_SPELL_WEB_ORBIT:
					createParticleAestheticOrbit(entity, 863, 400, PARTICLE_EFFECT_SPELL_WEB_ORBIT);
					break;
				case PARTICLE_EFFECT_PORTAL_SPAWN:
				{
					Entity* spellTimer = createParticleTimer(entity, 100, sprite);
					spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SPAWN_PORTAL;
					spellTimer->particleTimerCountdownSprite = 174;
					spellTimer->particleTimerEndAction = PARTICLE_EFFECT_PORTAL_SPAWN;
				}
				break;
				case PARTICLE_EFFECT_LICHFIRE_TELEPORT_STATIONARY:
				case PARTICLE_EFFECT_LICHICE_TELEPORT_STATIONARY:
				case PARTICLE_EFFECT_LICH_TELEPORT_ROAMING:
				{
					Entity* spellTimer = createParticleTimer(entity, 40, sprite);
					spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SHOOT_PARTICLES;
					spellTimer->particleTimerCountdownSprite = sprite;
				}
				break;
				case PARTICLE_EFFECT_PLAYER_AUTOMATON_DEATH:
					createParticleExplosionCharge(entity, 174, 100, 0.25);
					if ( entity && entity->behavior == &actPlayer )
					{
						if ( entity->getMonsterTypeFromSprite() == AUTOMATON )
						{
							entity->playerAutomatonDeathCounter = 1;
							if ( entity->skill[2] == clientnum )
							{
								// this is me dying, setup the deathcam.
								entity->playerCreatedDeathCam = 1;
								Entity* entity = newEntity(-1, 1, map.entities, nullptr);
								entity->x = cameras[clientnum].x * 16;
								entity->y = cameras[clientnum].y * 16;
								entity->z = -2;
								entity->flags[NOUPDATE] = true;
								entity->flags[PASSABLE] = true;
								entity->flags[INVISIBLE] = true;
								entity->behavior = &actDeathCam;
								entity->skill[2] = clientnum;
								entity->yaw = cameras[clientnum].ang;
								entity->pitch = PI / 8;
							}
						}
					}
					break;
				default:
					break;
			}
		}
		return;
	}

	// spawn misc particle effect at fixed location 
	else if ( !strncmp((char*)net_packet->data, "SPPL", 4) )
	{
		Sint16 particle_x = static_cast<Sint16>(SDLNet_Read16(&net_packet->data[4]));
		Sint16 particle_y = static_cast<Sint16>(SDLNet_Read16(&net_packet->data[6]));
		Sint16 particle_z = static_cast<Sint16>(SDLNet_Read16(&net_packet->data[8]));
		int particleType = static_cast<int>(net_packet->data[10]);
		int sprite = static_cast<int>(SDLNet_Read16(&net_packet->data[11]));
		//messagePlayer(1, "recv, %d, %d, %d, type: %d", particle_x, particle_y, particle_z, particleType);
		switch ( particleType )
		{
			case PARTICLE_EFFECT_SUMMON_MONSTER:
			{
				Entity* spellTimer = createParticleTimer(nullptr, 70, sprite);
				spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SUMMON_MONSTER;
				spellTimer->particleTimerCountdownSprite = 174;
				spellTimer->particleTimerEndAction = PARTICLE_EFFECT_SUMMON_MONSTER;
				spellTimer->x = particle_x * 16.0 + 8;
				spellTimer->y = particle_y * 16.0 + 8;
				spellTimer->z = particle_z;
			}
			break;
			case PARTICLE_EFFECT_DEVIL_SUMMON_MONSTER:
			{
				Entity* spellTimer = createParticleTimer(nullptr, 70, sprite);
				spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_DEVIL_SUMMON_MONSTER;
				spellTimer->particleTimerCountdownSprite = 174;
				spellTimer->particleTimerEndAction = PARTICLE_EFFECT_SUMMON_MONSTER;
				spellTimer->x = particle_x * 16.0 + 8;
				spellTimer->y = particle_y * 16.0 + 8;
				spellTimer->z = particle_z;
			}
			break;
			case PARTICLE_EFFECT_SPELL_SUMMON:
			{
				Entity* spellTimer = createParticleTimer(nullptr, 55, sprite);
				spellTimer->particleTimerCountdownSprite = 791;
				spellTimer->particleTimerCountdownAction = PARTICLE_TIMER_ACTION_SPELL_SUMMON;
				spellTimer->particleTimerPreDelay = 40;
				spellTimer->particleTimerEndAction = PARTICLE_EFFECT_SPELL_SUMMON;
				spellTimer->x = particle_x * 16.0 + 8;
				spellTimer->y = particle_y * 16.0 + 8;
				spellTimer->z = particle_z;
			}
			break;
			case PARTICLE_EFFECT_TELEPORT_PULL_TARGET_LOCATION:
			{
				Entity* spellTimer = createParticleTimer(nullptr, 40, 593);
				spellTimer->particleTimerCountdownAction = PARTICLE_EFFECT_TELEPORT_PULL_TARGET_LOCATION;
				spellTimer->particleTimerCountdownSprite = 593;
				spellTimer->x = particle_x * 16.0 + 8;
				spellTimer->y = particle_y * 16.0 + 8;
				spellTimer->z = particle_z;
				spellTimer->flags[PASSABLE] = false;
				spellTimer->sizex = 4;
				spellTimer->sizey = 4;
			}
			break;
			default:
				break;
		}
		return;
	}

	// enemy hp bar
	else if ( !strncmp((char*)net_packet->data, "ENHP", 4) )
	{
		Sint32 enemy_hp = SDLNet_Read32(&net_packet->data[4]);
		Sint32 enemy_maxhp = SDLNet_Read32(&net_packet->data[8]);
		Uint32 enemy_bar_color = SDLNet_Read32(&net_packet->data[12]); // receive color enemy bar data for my client.
		Sint32 oldhp = SDLNet_Read32(&net_packet->data[16]);
		Uint32 uid = SDLNet_Read32(&net_packet->data[20]);
		bool lowPriorityTick = false;
		if ( net_packet->data[24] == 1 )
		{
			lowPriorityTick = true;
		}
		char enemy_name[128] = "";
		strcpy(enemy_name, (char*)(&net_packet->data[25]));
		enemyHPDamageBarHandler[clientnum].addEnemyToList(enemy_hp, enemy_maxhp, oldhp, enemy_bar_color, uid, enemy_name, lowPriorityTick);
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

	// update steam statistic
	else if ( !strncmp((char*)net_packet->data, "SSTA", 4) )
	{
		int value = static_cast<int>(SDLNet_Read16(&net_packet->data[6]));
		steamStatisticUpdate(static_cast<int>(net_packet->data[4]), 
			static_cast<ESteamStatTypes>(net_packet->data[5]), value);
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

	// teleport player
	else if (!strncmp((char*)net_packet->data, "TELE", 4))
	{
		if (players[clientnum] == nullptr || players[clientnum]->entity == nullptr)
		{
			return;
		}
		int tele_x = net_packet->data[4];
		int tele_y = net_packet->data[5];
		Sint16 degrees = (Sint16)SDLNet_Read16(&net_packet->data[6]);
		players[clientnum]->entity->yaw = degrees * PI / 180;
		players[clientnum]->entity->x = (tele_x << 4) + 8;
		players[clientnum]->entity->y = (tele_y << 4) + 8;
		return;
	}

	// teleport player
	else if ( !strncmp((char*)net_packet->data, "TELM", 4) )
	{
		if ( players[clientnum] == nullptr || players[clientnum]->entity == nullptr )
		{
			return;
		}
		int tele_x = net_packet->data[4];
		int tele_y = net_packet->data[5];
		int type = net_packet->data[6];
		players[clientnum]->entity->x = (tele_x << 4) + 8;
		players[clientnum]->entity->y = (tele_y << 4) + 8;
		// play sound effect
		if ( type == 0 || type == 1 )
		{
			playSoundEntityLocal(players[clientnum]->entity, 96, 64);
		}
		else if ( type == 2 )
		{
			playSoundEntityLocal(players[clientnum]->entity, 154, 64);
		}
		return;
	}

	// delete entity
	else if (!strncmp((char*)net_packet->data, "ENTD", 4))
	{
		Entity *entity = uidToEntity((int)SDLNet_Read32(&net_packet->data[4]));
		if ( entity )
		{
			entity2 = newEntity(entity->sprite, 1, &removedEntities, nullptr);
			if ( entity2 )
			{
				entity2->setUID(entity->getUID());
				for ( j = 0; j < MAXPLAYERS; ++j )
				{
					if (entity == players[j]->entity )
					{
						if ( stats[j] )
						{
							for ( int effect = 0; effect < NUMEFFECTS; ++effect )
							{
								if ( effect != EFF_VAMPIRICAURA && effect != EFF_WITHDRAWAL && effect != EFF_SHAPESHIFT )
								{
									stats[j]->EFFECTS[effect] = false;
									stats[j]->EFFECTS_TIMERS[effect] = 0;
								}
							}
						}
						players[j]->entity = nullptr;
						players[j]->cleanUpOnEntityRemoval();
					}
				}
				if ( entity->light )
				{
					list_RemoveNode(entity->light->node);
					entity->light = nullptr;
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
			}
		}
		return;
	}

	// shake screen
	else if (!strncmp((char*)net_packet->data, "SHAK", 4))
	{
		cameravars[clientnum].shakex += ((char)(net_packet->data[4])) / 100.f;
		cameravars[clientnum].shakey += ((char)(net_packet->data[5]));
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
			if ( static_cast<int>(net_packet->data[5]) > EXCELLENT )
			{
				item->status = EXCELLENT;
			}
			else if ( static_cast<int>(net_packet->data[5]) < BROKEN )
			{
				item->status = BROKEN;
				if ( net_packet->data[4] == 5 )
				{
					if ( client_classes[clientnum] == CLASS_MESMER )
					{
						if ( stats[clientnum]->weapon->type == MAGICSTAFF_CHARM )
						{
							bool foundCharmSpell = false;
							for ( node_t* spellnode = stats[clientnum]->inventory.first; spellnode != nullptr; spellnode = spellnode->next )
							{
								Item* item = (Item*)spellnode->element;
								if ( item && itemCategory(item) == SPELL_CAT )
								{
									spell_t* spell = getSpellFromItem(clientnum, item);
									if ( spell && spell->ID == SPELL_CHARM_MONSTER )
									{
										foundCharmSpell = true;
										break;
									}
								}
							}
							if ( !foundCharmSpell )
							{
								steamAchievement("BARONY_ACH_WHAT_NOW");
							}
						}
					}
				}
			}
			else
			{
				item->status = static_cast<Status>(net_packet->data[5]);
			}

			// spellbooks in hand crumble to nothing.
			if ( item->status == BROKEN && net_packet->data[4] == 4 && itemCategory(item) == SPELLBOOK )
			{
				consumeItem(item, clientnum);
			}
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

	// damage indicator
	else if (!strncmp((char*)net_packet->data, "DAMI", 4))
	{
		newDamageIndicator(clientnum, SDLNet_Read32(&net_packet->data[4]), SDLNet_Read32(&net_packet->data[8]));
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

	// play sound entity local
	else if ( !strncmp((char*)net_packet->data, "SNEL", 4) )
	{
		Entity* tmp = uidToEntity(SDLNet_Read32(&net_packet->data[6]));
		int sfx = SDLNet_Read16(&net_packet->data[4]);
		if ( tmp )
		{
			if ( tmp->behavior == &actPlayer && mute_player_monster_sounds )
			{
				switch ( sfx )
				{
					case 95:
					case 70:
					case 322:
					case 323:
					case 324:
					case 329:
					case 332:
					case 333:
					case 291:
					case 292:
					case 293:
					case 294:
					case 60:
					case 61:
					case 62:
					case 257:
					case 258:
					case 276:
					case 277:
					case 278:
						// return early, don't play monster noises from players.
						return;
					default:
						break;
				}
			}
			playSoundEntityLocal(tmp, sfx, SDLNet_Read16(&net_packet->data[10]));
		}
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
		item = newItem(static_cast<ItemType>(SDLNet_Read32(&net_packet->data[4])), static_cast<Status>(SDLNet_Read32(&net_packet->data[8])), SDLNet_Read32(&net_packet->data[12]), SDLNet_Read32(&net_packet->data[16]), SDLNet_Read32(&net_packet->data[20]), net_packet->data[28], NULL);
		item->ownerUid = SDLNet_Read32(&net_packet->data[24]);
		Item* pickedUp = itemPickup(clientnum, item);
		free(item);
		if ( players[clientnum] && players[clientnum]->entity )
		{
			if ( pickedUp && pickedUp->type == BOOMERANG && !stats[clientnum]->weapon && item->ownerUid == players[clientnum]->entity->getUID() )
			{
				useItem(pickedUp, clientnum);

				auto& hotbar_t = players[clientnum]->hotbar;
				auto& hotbar = hotbar_t.slots();
				if ( hotbar_t.magicBoomerangHotbarSlot >= 0 )
				{
					hotbar[hotbar_t.magicBoomerangHotbarSlot].item = pickedUp->uid;
					for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
					{
						if ( i != hotbar_t.magicBoomerangHotbarSlot && hotbar[i].item == pickedUp->uid )
						{
							hotbar[i].item = 0;
						}
					}
				}
			}
		}
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

		if ( *armor == inputs.getUIInteraction(clientnum)->selectedItem )
		{
			inputs.getUIInteraction(clientnum)->selectedItem = nullptr;
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
		players[clientnum]->closeAllGUIs(DONT_CHANGE_SHOOTMODE, CLOSEGUI_DONT_CLOSE_SHOP);
		players[clientnum]->openStatusScreen(GUI_MODE_SHOP, INVENTORY_MODE_ITEM);
		shopkeeper[clientnum] = (Uint32)SDLNet_Read32(&net_packet->data[4]);
		shopkeepertype[clientnum] = net_packet->data[8];
		strcpy( shopkeepername_client[clientnum], (char*)(&net_packet->data[9]) );
		shopkeepername[clientnum] = shopkeepername_client[clientnum];
		shoptimer[clientnum] = ticks - 1;
		shopspeech[clientnum] = language[194 + rand() % 3];
		shopinventorycategory[clientnum] = 7;
		sellitem[clientnum] = NULL;
		shopitemscroll[clientnum] = 0;
		//Initialize shop gamepad code here.
		if ( shopinvitems[clientnum][0] != nullptr )
		{
			selectedShopSlot[clientnum] = 0;
			warpMouseToSelectedShopSlot(clientnum);
		}
		else
		{
			selectedShopSlot[clientnum] = -1;
		}
		return;
	}

	// shop item
	else if (!strncmp((char*)net_packet->data, "SHPI", 4))
	{
		if ( !shopInv[clientnum] )
		{
			return;
		}
		newItem(static_cast<ItemType>(SDLNet_Read32(&net_packet->data[4])), static_cast<Status>((char)net_packet->data[8]), (char)net_packet->data[9], (unsigned char)net_packet->data[10], SDLNet_Read32(&net_packet->data[11]), (bool)net_packet->data[15], shopInv[clientnum]);
	}

	// close shop
	else if (!strncmp((char*)net_packet->data, "SHPC", 4))
	{
		Uint32 id = SDLNet_Read32(&net_packet->data[4]);
		if ( id == shopkeeper[clientnum] )
		{
			closeShop(clientnum);
			players[clientnum]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_CLOSE_ALL);
		}
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
			if ( players[clientnum] && players[clientnum]->entity && players[clientnum]->entity->playerCreatedDeathCam != 0 )
			{
				// don't spawn deathcam
			}
			else
			{
				Entity* entity = newEntity(-1, 1, map.entities, nullptr);
				entity->x = cameras[clientnum].x * 16;
				entity->y = cameras[clientnum].y * 16;
				entity->z = -2;
				entity->flags[NOUPDATE] = true;
				entity->flags[PASSABLE] = true;
				entity->flags[INVISIBLE] = true;
				entity->behavior = &actDeathCam;
				entity->skill[2] = clientnum;
				entity->yaw = cameras[clientnum].ang;
				entity->pitch = PI / 8;
			}

			//deleteSaveGame(multiplayer); // stops save scumming c: //Not here, because it'll make the game unresumable if the game crashes but not all players have died.

			players[clientnum]->bookGUI.closeBookGUI();

#ifdef SOUND
			levelmusicplaying = true;
			combatmusicplaying = false;
			fadein_increment = default_fadein_increment * 4;
			fadeout_increment = default_fadeout_increment * 4;
			playmusic( sounds[209], false, true, false );
#endif
			combat = false;
			assailant[clientnum] = false;
			assailantTimer[clientnum] = 0;

			if ( !(svFlags & SV_FLAG_KEEPINVENTORY) )
			{
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
					net_packet->data[26] = (Uint8)cameras[clientnum].x;
					net_packet->data[27] = (Uint8)cameras[clientnum].y;
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
			else
			{
				// to not soft lock at Herx
				for ( node = stats[clientnum]->inventory.first; node != NULL; node = nextnode )
				{
					nextnode = node->next;
					Item* item = (Item*)node->element;
					if ( item->type == ARTIFACT_ORB_PURPLE )
					{
						strcpy((char*)net_packet->data, "DIEI");
						SDLNet_Write32((Uint32)item->type, &net_packet->data[4]);
						SDLNet_Write32((Uint32)item->status, &net_packet->data[8]);
						SDLNet_Write32((Uint32)item->beatitude, &net_packet->data[12]);
						SDLNet_Write32((Uint32)item->count, &net_packet->data[16]);
						SDLNet_Write32((Uint32)item->appearance, &net_packet->data[20]);
						net_packet->data[24] = item->identified;
						net_packet->data[25] = clientnum;
						net_packet->data[26] = (Uint8)cameras[clientnum].x;
						net_packet->data[27] = (Uint8)cameras[clientnum].y;
						net_packet->address.host = net_server.host;
						net_packet->address.port = net_server.port;
						net_packet->len = 28;
						sendPacketSafe(net_sock, -1, net_packet, 0);
						list_RemoveNode(node);
						break;
					}
				}
			}

			for ( node_t* mapNode = map.creatures->first; mapNode != nullptr; mapNode = mapNode->next )
			{
				Entity* mapCreature = (Entity*)mapNode->element;
				if ( mapCreature )
				{
					mapCreature->monsterEntityRenderAsTelepath = 0; // do a final pass to undo any telepath rendering.
				}
			}
		}
		else if ( !strcmp((char*)(&net_packet->data[8]), language[1109]) )
		{
			// ... or lived
			stats[clientnum]->HP = stats[clientnum]->MAXHP * 0.5;
			stats[clientnum]->MP = stats[clientnum]->MAXMP * 0.5;
			stats[clientnum]->HUNGER = 500;
			for ( c = 0; c < NUMEFFECTS; c++ )
			{
				if ( !(c == EFF_VAMPIRICAURA && stats[clientnum]->EFFECTS_TIMERS[c] == -2)
					&& c != EFF_WITHDRAWAL && c != EFF_SHAPESHIFT )
				{
					stats[clientnum]->EFFECTS[c] = false;
					stats[clientnum]->EFFECTS_TIMERS[c] = 0;
				}
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
				if ( net_packet->data[9 + c / 8] & power(2, c - (c / 8) * 8) ) // use these bits to denote if duration is low.
				{
					stats[clientnum]->EFFECTS_TIMERS[c] = 1;
				}
			}
			else
			{
				stats[clientnum]->EFFECTS[c] = false;
				if ( stats[clientnum]->EFFECTS_TIMERS[c] > 0 )
				{
					stats[clientnum]->EFFECTS_TIMERS[c] = 0;
				}
			}
		}
		return;
	}

	// update entity stat flag
	else if ( !strncmp((char*)net_packet->data, "ENSF", 4) )
	{
		Entity *entity = uidToEntity((int)SDLNet_Read32(&net_packet->data[4]));
		if ( entity )
		{
			if ( entity->getStats() )
			{
				entity->getStats()->MISC_FLAGS[net_packet->data[8]] = SDLNet_Read32(&net_packet->data[9]);
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

	// level up icon timers, sets second row of icons if double stat gain is rolled.
	else if (!strncmp((char*)net_packet->data, "LVLI", 4))
	{
		// Note - set to 250 ticks, higher values will require resending/using 16 bit data.
		stats[clientnum]->PLAYER_LVL_STAT_TIMER[STAT_STR] = (Uint8)net_packet->data[5];
		stats[clientnum]->PLAYER_LVL_STAT_TIMER[STAT_DEX] = (Uint8)net_packet->data[6];
		stats[clientnum]->PLAYER_LVL_STAT_TIMER[STAT_CON] = (Uint8)net_packet->data[7];
		stats[clientnum]->PLAYER_LVL_STAT_TIMER[STAT_INT] = (Uint8)net_packet->data[8];
		stats[clientnum]->PLAYER_LVL_STAT_TIMER[STAT_PER] = (Uint8)net_packet->data[9];
		stats[clientnum]->PLAYER_LVL_STAT_TIMER[STAT_CHR] = (Uint8)net_packet->data[10];
		stats[clientnum]->PLAYER_LVL_STAT_TIMER[STAT_STR + NUMSTATS] = (Uint8)net_packet->data[11];
		stats[clientnum]->PLAYER_LVL_STAT_TIMER[STAT_DEX + NUMSTATS] = (Uint8)net_packet->data[12];
		stats[clientnum]->PLAYER_LVL_STAT_TIMER[STAT_CON + NUMSTATS] = (Uint8)net_packet->data[13];
		stats[clientnum]->PLAYER_LVL_STAT_TIMER[STAT_INT + NUMSTATS] = (Uint8)net_packet->data[14];
		stats[clientnum]->PLAYER_LVL_STAT_TIMER[STAT_PER + NUMSTATS] = (Uint8)net_packet->data[15];
		stats[clientnum]->PLAYER_LVL_STAT_TIMER[STAT_CHR + NUMSTATS] = (Uint8)net_packet->data[16];
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

		if ( net_packet->data[5] == PRO_ALCHEMY )
		{
			GenericGUI[clientnum].alchemyLearnRecipeOnLevelUp(stats[clientnum]->PROFICIENCIES[net_packet->data[5]]);
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

	// update player stat values
	else if ( !strncmp((char*)net_packet->data, "STAT", 4) )
	{
		Sint32 buffer = 0;
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			buffer = (Sint32)SDLNet_Read32(&net_packet->data[4 + i * 8]);
			stats[i]->MAXHP = buffer & 0xFFFF;
			stats[i]->HP = (buffer >> 16) & 0xFFFF;
			buffer = (Sint32)SDLNet_Read32(&net_packet->data[8 + i * 8]);
			stats[i]->MAXMP = buffer & 0xFFFF;
			stats[i]->MP = (buffer >> 16) & 0xFFFF;
		}
	}

	else if ( !strncmp((char*)net_packet->data, "COND", 4) )
	{
		int conduct = SDLNet_Read16(&net_packet->data[4]);
		int value = SDLNet_Read16(&net_packet->data[6]);
		conductGameChallenges[conduct] = value;
		//messagePlayer(clientnum, "received %d %d, set to %d", conduct, value, conductGameChallenges[conduct]);
		return;
	}

	// update player statistics
	else if ( !strncmp((char*)net_packet->data, "GPST", 4) )
	{
		int gameplayStat = SDLNet_Read32(&net_packet->data[4]);
		int changeval = SDLNet_Read32(&net_packet->data[8]);
		if ( gameplayStat == STATISTICS_TEMPT_FATE )
		{
			if ( gameStatistics[STATISTICS_TEMPT_FATE] == -1 )
			{
				// don't change, completed task.
			}
			else
			{
				if ( changeval == 5 )
				{
					gameStatistics[gameplayStat] = changeval;
				}
				else if ( changeval == 1 && gameStatistics[gameplayStat] > 0 )
				{
					gameStatistics[gameplayStat] = -1;
				}
			}
		}
		else if ( gameplayStat == STATISTICS_FORUM_TROLL )
		{
			if ( changeval == AchievementObserver::FORUM_TROLL_BREAK_WALL )
			{
				int walls = gameStatistics[gameplayStat] & 0xFF;
				walls = std::min(walls + 1, 3);
				gameStatistics[gameplayStat] = gameStatistics[gameplayStat] & 0xFFFFFF00;
				gameStatistics[gameplayStat] |= walls;
			}
			else if ( changeval == AchievementObserver::FORUM_TROLL_RECRUIT_TROLL )
			{
				int trolls = (gameStatistics[gameplayStat] >> 8) & 0xFF;
				trolls = std::min(trolls + 1, 3);
				gameStatistics[gameplayStat] = gameStatistics[gameplayStat] & 0xFFFF00FF;
				gameStatistics[gameplayStat] |= (trolls << 8);
			}
			else if ( changeval == AchievementObserver::FORUM_TROLL_FEAR )
			{
				int fears = (gameStatistics[gameplayStat] >> 16) & 0xFF;
				fears = std::min(fears + 1, 3);
				gameStatistics[gameplayStat] = gameStatistics[gameplayStat] & 0xFF00FFFF;
				gameStatistics[gameplayStat] |= (fears << 16);
			}
		}
		else if ( gameplayStat == STATISTICS_POP_QUIZ_1 || gameplayStat == STATISTICS_POP_QUIZ_2 )
		{
			int spellID = changeval;
			if ( spellID >= 32 )
			{
				spellID -= 32;
				int shifted = (1 << spellID);
				gameStatistics[gameplayStat] |= shifted;
			}
			else
			{
				int shifted = (1 << spellID);
				gameStatistics[gameplayStat] |= shifted;
			}
		}
		else
		{
			gameStatistics[gameplayStat] += changeval;
		}
		//messagePlayer(clientnum, "received: %d, %d, val: %d", gameplayStat, changeval, gameStatistics[gameplayStat]);
	}

	// update player levels
	else if ( !strncmp((char*)net_packet->data, "UPLV", 4) )
	{
		Sint32 buffer = SDLNet_Read32(&net_packet->data[4]);
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			stats[i]->LVL = static_cast<Sint32>((buffer >> (i * 8) ) & 0xFF);
		}
	}

	// current game level
	else if (!strncmp((char*)net_packet->data, "LVLC", 4) || !strncmp((char*)net_packet->data, "LVLR", 4) )
	{
		if ( strncmp((char*)net_packet->data, "LVLR", 4) )
		{
			if ( currentlevel == net_packet->data[13] && secretlevel == net_packet->data[4] )
			{
				// the server's just doing a routine check
				return;
			}
		}

		if ( net_packet->data[14] != 0 )
		{
			// loading a custom map name.
			char buf[128] = "";
			strcpy(buf, (char*)&net_packet->data[14]);
			loadCustomNextMap = buf;
		}

		if ( introstage == 9
			|| introstage == 11 + MOVIE_MIDGAME_BAPHOMET_HUMAN_AUTOMATON
			|| introstage == 11 + MOVIE_MIDGAME_BAPHOMET_MONSTERS
			|| introstage == 11 + MOVIE_MIDGAME_HERX_MONSTERS )
		{
			thirdendmovietime = 0;
			thirdendmoviestage = 0;
			DLCendmovieStageAndTime[MOVIE_MIDGAME_HERX_MONSTERS][MOVIE_STAGE] = 0;
			DLCendmovieStageAndTime[MOVIE_MIDGAME_HERX_MONSTERS][MOVIE_TIME] = 0;
			DLCendmovieStageAndTime[MOVIE_MIDGAME_BAPHOMET_MONSTERS][MOVIE_STAGE] = 0;
			DLCendmovieStageAndTime[MOVIE_MIDGAME_BAPHOMET_MONSTERS][MOVIE_TIME] = 0;
			DLCendmovieStageAndTime[MOVIE_MIDGAME_BAPHOMET_HUMAN_AUTOMATON][MOVIE_STAGE] = 0;
			DLCendmovieStageAndTime[MOVIE_MIDGAME_BAPHOMET_HUMAN_AUTOMATON][MOVIE_TIME] = 0;

			movie = false; // allow normal pause screen.
			introstage = 1; // return to normal game functionality
			pauseGame(1, false); // unpause game
		}

		// hack to fix these things from breaking everything...
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			players[i]->hud.arm = nullptr;
			players[i]->hud.weapon = nullptr;
			players[i]->hud.magicLeftHand = nullptr;
			players[i]->hud.magicRightHand = nullptr;
		}

		// stop all sounds
#ifdef USE_FMOD
		if ( sound_group )
		{
			FMOD_ChannelGroup_Stop(sound_group);
		}
		if ( soundAmbient_group )
		{
			FMOD_ChannelGroup_Stop(soundAmbient_group);
		}
		if ( soundEnvironment_group )
		{
			FMOD_ChannelGroup_Stop(soundEnvironment_group);
		}
#elif defined USE_OPENAL
		if ( sound_group )
		{
			OPENAL_ChannelGroup_Stop(sound_group);
		}
		if ( soundAmbient_group )
		{
			OPENAL_ChannelGroup_Stop(soundAmbient_group);
		}
		if ( soundEnvironment_group )
		{
			OPENAL_ChannelGroup_Stop(soundEnvironment_group);
		}
#endif
		if ( openedChest[clientnum] )
		{
			closeChestClientside(clientnum);
		}

		// show loading message
#define LOADSTR language[709]
		loading = true;
		drawClearBuffers();
		int w, h;
		getSizeOfText(ttf16, LOADSTR, &w, &h);
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
			default:
				break;
			}
		}

		// setup level change
		printlog("Received order to change level.\n");
		currentlevel = static_cast<Sint8>(net_packet->data[13]);
		
		if ( !secretlevel )
		{
			switch ( currentlevel )
			{
				case 5:
					steamAchievement("BARONY_ACH_TWISTY_PASSAGES");
					break;
				case 10:
					steamAchievement("BARONY_ACH_JUNGLE_FEVER");
					break;
				case 15:
					steamAchievement("BARONY_ACH_SANDMAN");
					break;
				case 30:
					steamAchievement("BARONY_ACH_SPELUNKY");
					break;
				case 35:
					if ( ((completionTime / TICKS_PER_SECOND) / 60) <= 45 )
					{
						conductGameChallenges[CONDUCT_BLESSED_BOOTS_SPEED] = 1;
					}
					break;
				default:
					break;
			}
		}

		list_FreeAll(&removedEntities);
		for ( node = map.entities->first; node != nullptr; node = node->next )
		{
			entity = (Entity*)node->element;
			entity2 = newEntity(entity->sprite, 1, &removedEntities, nullptr);
			entity2->setUID(entity->getUID());
		}
		for ( i = 0; i < MAXPLAYERS; ++i )
		{
			list_FreeAll(&stats[i]->FOLLOWERS);
		}

		// load next level
		darkmap = false;
		secretlevel = net_packet->data[4];
		mapseed = SDLNet_Read32(&net_packet->data[5]);
		/*Uint32 oldtime = SDL_GetTicks();
		while( SDLNet_TCP_Recv(net_tcpsock, net_packet->data, 4)!=4 ) {
			if( SDL_GetTicks()-oldtime>10000 )
				printlog("warning: game has taken more than 10 seconds to receive map seed\n");
		}*/
		numplayers = 0;
		entity_uids = (Uint32)SDLNet_Read32(&net_packet->data[9]);
		printlog("Received map seed: %d. Entity UID start: %d\n", mapseed, entity_uids);

		gameplayCustomManager.readFromFile();

		int checkMapHash = -1;
		physfsLoadMapFile(currentlevel, mapseed, false, &checkMapHash);
		if ( checkMapHash == 0 )
		{
			conductGameChallenges[CONDUCT_MODDED] = 1;
		}

		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			minimapPings[i].clear(); // clear minimap pings
		}
		globalLightModifierActive = GLOBAL_LIGHT_MODIFIER_STOPPED;

		// clear follower menu entities.
		FollowerMenu[clientnum].closeFollowerMenuGUI(true);

		numplayers = 0;
		assignActions(&map);
		generatePathMaps();
		for ( node = map.entities->first; node != nullptr; node = nextnode )
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
		if ( !secretlevel )
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
		if ( MFLAG_DISABLETELEPORT || MFLAG_DISABLEOPENING )
		{
			messagePlayer(clientnum, language[2382]);
		}
		if ( MFLAG_DISABLELEVITATION )
		{
			messagePlayer(clientnum, language[2383]);
		}
		if ( MFLAG_DISABLEDIGGING )
		{
			messagePlayer(clientnum, language[2450]);
		}
		if ( !strncmp(map.name, "Mages Guild", 11) )
		{
			messagePlayer(clientnum, language[2599]);
		}
		loading = false;
		fadeout = false;
		fadealpha = 255;
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

		Entity* monster = uidToEntity(*uidnum);
		if ( monster )
		{
			if ( !monster->clientsHaveItsStats )
			{
				monster->giveClientStats();
			}
			if ( monster->clientStats )
			{
				strcpy(monster->clientStats->name, (char*)&net_packet->data[8]);
			}
			if ( !FollowerMenu[clientnum].recentEntity )
			{
				FollowerMenu[clientnum].recentEntity = monster;
			}
		}
		return;
	}

	// remove a monster from followers list
	else if ( !strncmp((char*)net_packet->data, "LDEL", 4) )
	{
		Uint32 uidnum = (Uint32)SDLNet_Read32(&net_packet->data[4]);
		if ( stats[clientnum] )
		{
			for ( node_t* allyNode = stats[clientnum]->FOLLOWERS.first; allyNode != nullptr; allyNode = allyNode->next )
			{
				if ( (Uint32*)allyNode->element && *((Uint32*)allyNode->element) == uidnum )
				{
					if ( FollowerMenu[clientnum].recentEntity && (FollowerMenu[clientnum].recentEntity->getUID() == 0
						|| FollowerMenu[clientnum].recentEntity->getUID() == uidnum) )
					{
						FollowerMenu[clientnum].recentEntity = nullptr;
					}
					if ( FollowerMenu[clientnum].followerToCommand == uidToEntity(uidnum) )
					{
						FollowerMenu[clientnum].closeFollowerMenuGUI();
					}
					list_RemoveNode(allyNode);
					break;
				}
			}
		}
	}

	// update client's follower data on level up or initial follow.
	else if ( !strncmp((char*)net_packet->data, "NPCI", 4) )
	{
		Uint32 uidnum = (Uint32)SDLNet_Read32(&net_packet->data[4]);
		Entity* monster = uidToEntity(uidnum);
		if ( monster )
		{
			if ( !monster->clientsHaveItsStats )
			{
				monster->giveClientStats();
			}
			if ( monster->clientStats )
			{
				monster->clientStats->LVL = net_packet->data[8];
				monster->clientStats->HP = SDLNet_Read16(&net_packet->data[9]);
				monster->clientStats->MAXHP = SDLNet_Read16(&net_packet->data[11]);
				monster->clientStats->type = static_cast<Monster>(net_packet->data[13]);
			}
		}
	}

	// update client's follower hp/maxhp data at intervals
	else if ( !strncmp((char*)net_packet->data, "NPCU", 4) )
	{
		Uint32 uidnum = (Uint32)SDLNet_Read32(&net_packet->data[4]);
		Entity* monster = uidToEntity(uidnum);
		if ( monster )
		{
			if ( !monster->clientsHaveItsStats )
			{
				monster->giveClientStats();
			}
			if ( monster->clientStats )
			{
				monster->clientStats->HP = SDLNet_Read16(&net_packet->data[8]);
				monster->clientStats->MAXHP = SDLNet_Read16(&net_packet->data[10]);
			}
		}
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

	// bless one piece of my equipment
	else if (!strncmp((char*)net_packet->data, "BLE1", 4))
	{
		Uint32 chosen = static_cast<Uint32>(SDLNet_Read32(&net_packet->data[4]));
		switch ( chosen )
		{
			case 0:
				if ( stats[clientnum]->helmet )
				{
					stats[clientnum]->helmet->beatitude++;
				}
				break;
			case 1:
				if ( stats[clientnum]->breastplate )
				{
					stats[clientnum]->breastplate->beatitude++;
				}
				break;
			case 2:
				if ( stats[clientnum]->gloves )
				{
					stats[clientnum]->gloves->beatitude++;
				}
				break;
			case 3:
				if ( stats[clientnum]->shoes )
				{
					stats[clientnum]->shoes->beatitude++;
				}
				break;
			case 4:
				if ( stats[clientnum]->shield )
				{
					stats[clientnum]->shield->beatitude++;
				}
				break;
			case 5:
				if ( stats[clientnum]->weapon )
				{
					stats[clientnum]->weapon->beatitude++;
				}
				break;
			case 6:
				if ( stats[clientnum]->cloak )
				{
					stats[clientnum]->cloak->beatitude++;
				}
				break;
			case 7:
				if ( stats[clientnum]->amulet )
				{
					stats[clientnum]->amulet->beatitude++;
				}
				break;
			case 8:
				if ( stats[clientnum]->ring )
				{
					stats[clientnum]->ring->beatitude++;
				}
				break;
			case 9:
				if ( stats[clientnum]->mask )
				{
					stats[clientnum]->mask->beatitude++;
				}
				break;
			default:
				break;
		}
		return;
	}

	// update entity appearance (sprite)
	else if (!strncmp((char*)net_packet->data, "ENTA", 4))
	{
		Entity *entity = uidToEntity((int)SDLNet_Read32(&net_packet->data[4]));
		if ( entity )
		{
			entity->sprite = SDLNet_Read32(&net_packet->data[8]);
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

	// monster summon
	else if ( !strncmp((char*)net_packet->data, "SUMS", 4) )
	{
		if ( stats[clientnum] )
		{
			stats[clientnum]->playerSummonLVLHP = (Sint32)SDLNet_Read32(&net_packet->data[4]);
			stats[clientnum]->playerSummonSTRDEXCONINT = (Sint32)SDLNet_Read32(&net_packet->data[8]);
			stats[clientnum]->playerSummonPERCHR = (Sint32)SDLNet_Read32(&net_packet->data[12]);
			stats[clientnum]->playerSummon2LVLHP = (Sint32)SDLNet_Read32(&net_packet->data[16]);
			stats[clientnum]->playerSummon2STRDEXCONINT = (Sint32)SDLNet_Read32(&net_packet->data[20]);
			stats[clientnum]->playerSummon2PERCHR = (Sint32)SDLNet_Read32(&net_packet->data[24]);
		}
		return;
	}

	//Multiplayer chest code (client).
	else if ( !strncmp((char*)net_packet->data, "CHST", 4) )
	{
		if ( openedChest[clientnum] )
		{
			//Close the chest.
			closeChestClientside(clientnum);
		}

		Entity *entity = uidToEntity((int)SDLNet_Read32(&net_packet->data[4]));
		if ( entity )
		{
			openedChest[clientnum] = entity; //Set the opened chest to this.
			GenericGUI[clientnum].closeGUI();
			list_FreeAll(&chestInv[clientnum]);
			chestInv[clientnum].first = nullptr;
			chestInv[clientnum].last = nullptr;
			players[clientnum]->openStatusScreen(GUI_MODE_INVENTORY, INVENTORY_MODE_ITEM);
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

		addItemToChestClientside(clientnum, newitem);
		return;
	}

	//Close the chest.
	else if (!strncmp((char*)net_packet->data, "CCLS", 4))
	{
		closeChestClientside(clientnum);
		return;
	}

	//Open up the GUI to identify an item.
	else if (!strncmp((char*)net_packet->data, "IDEN", 4))
	{
		GenericGUI[clientnum].openGUI(GUI_TYPE_IDENTIFY, nullptr);
		return;
	}

	// Open up the Remove Curse GUI
	else if ( !strncmp((char*)net_packet->data, "CRCU", 4) )
	{
		//Uncurse an item
		GenericGUI[clientnum].openGUI(GUI_TYPE_REMOVECURSE, nullptr);
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

	else if ( !strncmp((char*)net_packet->data, "MFOD", 4) )
	{
		mapFoodOnLevel(clientnum);
		return;
	}

	else if ( !strncmp((char*)net_packet->data, "TKIT", 4) )
	{
		GenericGUI[clientnum].tinkeringKitDegradeOnUse(clientnum);
		return;
	}

	// boss death
	else if ( !strncmp((char*)net_packet->data, "BDTH", 4) )
	{
		for ( node = map.entities->first; node != nullptr; node = node->next )
		{
			Entity* entity = (Entity*)node->element;
			if ( strstr(map.name, "Hell") )
			{
				if ( entity->behavior == &actWinningPortal )
				{
					//entity->flags[INVISIBLE] = false;
				}
			}
			else if ( strstr(map.name, "Boss") )
			{
				if ( entity->behavior == &actPedestalBase )
				{
					entity->pedestalInit = 1;
				}
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

	// mid game movie
	else if ( !strncmp((char*)net_packet->data, "MIDG", 4) )
	{
		subwindow = 0;
		fadeout = true;
		if ( !intro )
		{
			pauseGame(2, false);
		}
		introstage = net_packet->data[4]; // prepares mid game sequence
		return;
	}

	else if ( !strncmp((char*)net_packet->data, "PMAP", 4) )
	{
		MinimapPing newPing(ticks, net_packet->data[4], net_packet->data[5], net_packet->data[6]);
		for ( int c = 0; c < MAXPLAYERS; ++c )
		{
			if ( players[c]->isLocalPlayer() )
			{
				minimapPingAdd(newPing.player, c, newPing);
			}
		}
	}
	else if ( !strncmp((char*)net_packet->data, "DASH", 4) )
	{
		if ( players[clientnum] && players[clientnum]->entity && stats[clientnum] )
		{
			real_t vel = sqrt(pow(players[clientnum]->entity->vel_y, 2) + pow(players[clientnum]->entity->vel_x, 2));
			players[clientnum]->entity->monsterKnockbackVelocity = std::min(2.25, std::max(1.0, vel));
			players[clientnum]->entity->monsterKnockbackTangentDir = atan2(players[clientnum]->entity->vel_y, players[clientnum]->entity->vel_x);
			if ( vel < 0.01 )
			{
				players[clientnum]->entity->monsterKnockbackTangentDir = players[clientnum]->entity->yaw + PI;
			}
		}
	}

	// get item
	else if ( !strncmp((char*)net_packet->data, "ITEQ", 4) )
	{
		item = newItem(static_cast<ItemType>(SDLNet_Read32(&net_packet->data[4])), static_cast<Status>(SDLNet_Read32(&net_packet->data[8])), SDLNet_Read32(&net_packet->data[12]), SDLNet_Read32(&net_packet->data[16]), SDLNet_Read32(&net_packet->data[20]), net_packet->data[28], NULL);
		item->ownerUid = SDLNet_Read32(&net_packet->data[24]);
		Item* pickedUp = itemPickup(clientnum, item);
		free(item);
		if ( players[clientnum] && players[clientnum]->entity && pickedUp )
		{
			bool oldIntro = intro;
			intro = true;
			useItem(pickedUp, clientnum);
			intro = oldIntro;
		}
		return;
	}

	// update attributes from script
	else if ( !strncmp((char*)net_packet->data, "SCRU", 4) )
	{
		if ( net_packet->data[25] )
		{
			bool clearStats = false;
			if ( net_packet->data[26] )
			{
				clearStats = true;
			}
			textSourceScript.playerClearInventory(clearStats);
		}
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
		stats[clientnum]->GOLD = (Sint32)SDLNet_Read32(&net_packet->data[21]);
		for ( int i = 0; i < NUMPROFICIENCIES; ++i )
		{
			stats[clientnum]->PROFICIENCIES[i] = (Sint8)net_packet->data[27 + i];
		}
		return;
	}

	// update class from script
	else if ( !strncmp((char*)net_packet->data, "SCRC", 4) )
	{
		for ( int c = 0; c < MAXPLAYERS; ++c )
		{
			client_classes[c] = net_packet->data[4 + c];
			if ( c == clientnum )
			{
				bool oldIntro = intro;
				intro = true;
				initClass(clientnum);
				intro = oldIntro;
			}
		}
		return;
	}

	// game restart
	if (!strncmp((char*)net_packet->data, "BARONY_GAME_START", 17))
	{
		if ( !intro )
		{
			// intro is true if starting from main menu, otherwise we're restarting the game.
			// set the main menu camera to the player camera coordinates if restarting midgame.
			menucam.x = cameras[clientnum].x;
			menucam.y = cameras[clientnum].y;
			menucam.z = cameras[clientnum].z;
			menucam.ang = cameras[clientnum].ang;
			menucam.vang = cameras[clientnum].vang;
		}
		intro = true;
		client_disconnected[0] = true;
		svFlags = SDLNet_Read32(&net_packet->data[17]);
		uniqueGameKey = SDLNet_Read32(&net_packet->data[21]);
		buttonCloseSubwindow(NULL);
		numplayers = 0;
		introstage = 3;
		if ( net_packet->data[25] == 0 )
		{
			loadingsavegame = 0; // the server said we're not loading a saved game.
		}
		fadeout = true;
		return;
	}

	// delete multiplayer save
	if (!strncmp((char*)net_packet->data, "DSAV", 4))
	{
		if ( multiplayer == CLIENT )
		{
			deleteSaveGame(multiplayer);
		}
		return;
	}
}

/*-------------------------------------------------------------------------------

	clientHandleMessages

	Parses messages received from the server

-------------------------------------------------------------------------------*/

void clientHandleMessages(Uint32 framerateBreakInterval)
{
#ifdef STEAMWORKS
	if (!directConnect && !net_handler)
	{
		net_handler = new NetHandler();
		if ( !disableMultithreadedSteamNetworking )
		{
			net_handler->initializeMultithreadedPacketHandling();
		}
	}
#elif defined USE_EOS
	if ( !directConnect && !net_handler )
	{
		net_handler = new NetHandler();
	}
#endif

	if (!directConnect)
	{
#if defined(STEAMWORKS) || defined(USE_EOS)
		if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
		{
#ifdef STEAMWORKS
			//Steam stuff goes here.
			if ( disableMultithreadedSteamNetworking )
			{
				steamPacketThread(static_cast<void*>(net_handler));
			}
#endif
		}
		else if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
		{
#if defined USE_EOS
			EOSPacketThread(static_cast<void*>(net_handler));
#endif
		}
		SteamPacketWrapper* packet = nullptr;

		if ( logCheckMainLoopTimers )
		{
			DebugStats.messagesT1 = std::chrono::high_resolution_clock::now();
			DebugStats.handlePacketStartLoop = true;
		}

		while (packet = net_handler->getGamePacket())
		{
			memcpy(net_packet->data, packet->data(), packet->len());
			net_packet->len = packet->len();

			clientHandlePacket(); //Uses net_packet.

			if ( logCheckMainLoopTimers )
			{
				DebugStats.messagesT2WhileLoop = std::chrono::high_resolution_clock::now();
				DebugStats.handlePacketStartLoop = false;
			}
			delete packet;

			if ( !disableFPSLimitOnNetworkMessages && !frameRateLimit(framerateBreakInterval, false) )
			{
				if ( logCheckMainLoopTimers )
				{
					printlog("[NETWORK]: Incoming messages exceeded given cycle time, packets remaining: %d", net_handler->game_packets.size());
				}
				break;
			}
		}
#endif
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
		if ( j <= 0 )
		{
			return;
		}
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
		if ( x <= 0 )
		{
			return;
		}
		Uint32 uid = SDLNet_Read32(&net_packet->data[5]);
		Entity* entity = uidToEntity(uid);
		if ( entity )
		{
			return; // found entity.
		}
		// else reply with entity deleted.
		strcpy((char*)net_packet->data, "ENTD");
		SDLNet_Write32(uid, &net_packet->data[4]);
		net_packet->address.host = net_clients[x - 1].host;
		net_packet->address.port = net_clients[x - 1].port;
		net_packet->len = 8;
		sendPacketSafe(net_sock, -1, net_packet, x - 1);
		return;
	}

	// client request item details.
	else if ( !strncmp((char*)net_packet->data, "ITMU", 4) )
	{
		int x = net_packet->data[4];
		if ( x <= 0 )
		{
			return;
		}
		Uint32 uid = SDLNet_Read32(&net_packet->data[5]);
		Entity* entity = uidToEntity(uid);
		if ( entity )
		{
			strcpy((char*)net_packet->data, "ITMU");
			SDLNet_Write32(uid, &net_packet->data[4]);

			Uint32 itemTypeAndIdentified = (static_cast<Uint16>(entity->skill[10]) << 16); // type
			itemTypeAndIdentified |= static_cast<Uint16>(entity->skill[15]); // identified

			SDLNet_Write32(itemTypeAndIdentified, &net_packet->data[8]);

			Uint32 statusBeatitudeQuantityAppearance = 0;
			statusBeatitudeQuantityAppearance |= (static_cast<Uint8>(entity->skill[11]) << 24); // status
			statusBeatitudeQuantityAppearance |= (static_cast<Sint8>(entity->skill[12]) << 16); // beatitude
			statusBeatitudeQuantityAppearance |= (static_cast<Uint8>(entity->skill[13]) << 8); // quantity
			Uint8 appearance = entity->skill[14] % items[entity->skill[10]].variations;
			statusBeatitudeQuantityAppearance |= (static_cast<Uint8>(appearance)); // appearance

			SDLNet_Write32(statusBeatitudeQuantityAppearance, &net_packet->data[12]);

			net_packet->address.host = net_clients[x - 1].host;
			net_packet->address.port = net_clients[x - 1].port;
			net_packet->len = 16;
			sendPacketSafe(net_sock, -1, net_packet, x - 1);
			return; // found entity.
		}
	}

	// player move
	else if (!strncmp((char*)net_packet->data, "PMOV", 4))
	{
		int player = net_packet->data[4];
		if ( player < 0 || player >= MAXPLAYERS )
		{
			return;
		}
		client_keepalive[player] = ticks;
		if (players[player] == nullptr || players[player]->entity == nullptr)
		{
			return;
		}

		// check if the info is outdated
		if ( net_packet->data[5] != currentlevel || net_packet->data[18] != secretlevel )
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
		players[player]->entity->yaw = yaw;
		players[player]->entity->pitch = pitch;

		// update player's internal velocity variables
		players[player]->entity->vel_x = velx; // PLAYER_VELX
		players[player]->entity->vel_y = vely; // PLAYER_VELY

		// store old coordinates
		// since this function runs more often than actPlayer runs, we need to keep track of the accumulated position in new_x/new_y
		real_t ox = players[player]->entity->x;
		real_t oy = players[player]->entity->y;
		players[player]->entity->x = players[player]->entity->new_x;
		players[player]->entity->y = players[player]->entity->new_y;

		// calculate distance
		dx -= players[player]->entity->x;
		dy -= players[player]->entity->y;
		dist = sqrt( dx * dx + dy * dy );

		// move player with collision detection
		real_t result = clipMove(&players[player]->entity->x, &players[player]->entity->y, dx, dy, players[player]->entity);
		if ( result < dist - .025 )
		{
			// player encountered obstacle on path
			// stop updating position on server side and send client corrected position
			j = net_packet->data[4];
			if ( j > 0 )
			{
				strcpy((char*)net_packet->data, "PMOV");
				SDLNet_Write16((Sint16)(players[j]->entity->x * 32), &net_packet->data[4]);
				SDLNet_Write16((Sint16)(players[j]->entity->y * 32), &net_packet->data[6]);
				net_packet->address.host = net_clients[j - 1].host;
				net_packet->address.port = net_clients[j - 1].port;
				net_packet->len = 8;
				sendPacket(net_sock, -1, net_packet, j - 1);
			}
		}

		// clipMove sent any corrections to the client, now let's save the updated coordinates.
		players[player]->entity->new_x = players[player]->entity->x;
		players[player]->entity->new_y = players[player]->entity->y;
		// return x/y to their original state as this can update more than actPlayer and causes stuttering. use new_x/new_y in actPlayer.
		players[player]->entity->x = ox;
		players[player]->entity->y = oy;

		// update the players' head and mask as these will otherwise wait until actPlayer to update their rotation. stops clipping.
		node_t* tmpNode = nullptr;
		int bodypartNum = 0;
		for ( bodypartNum = 0, tmpNode = players[player]->entity->children.first; tmpNode; tmpNode = tmpNode->next, bodypartNum++ )
		{
			if ( bodypartNum == 0 )
			{
				// hudweapon case
				continue;
			}

			Entity* limb = (Entity*)tmpNode->element;
			if ( limb )
			{
				// adjust headgear/mask yaw/pitch variations as these do not update always.
				if ( bodypartNum == 9 || bodypartNum == 10 )
				{
					limb->x = players[player]->entity->x;
					limb->y = players[player]->entity->y;
					limb->pitch = players[player]->entity->pitch;
					limb->yaw = players[player]->entity->yaw;
				}
			}
		}

		return;
	}

	// tried to update
	else if (!strncmp((char*)net_packet->data, "NOUP", 4))
	{
		Uint32 uid = SDLNet_Read32(&net_packet->data[5]);
		Entity* entity = uidToEntity(uid);
		if ( entity )
		{
			entity->flags[UPDATENEEDED] = false;
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
	else if (!strncmp((char*)net_packet->data, "CKIR", 4) 
		|| !strncmp((char*)net_packet->data, "SALV", 4)
		|| !strncmp((char*)net_packet->data, "RATF", 4) )
	{
		client_keepalive[net_packet->data[4]] = ticks;
		Uint32 uid = SDLNet_Read32(&net_packet->data[5]);
		Entity* entity = uidToEntity(uid);
		if ( entity )
		{
			if ( (entity->behavior == &actItem || entity->behavior == &actTorch || entity->behavior == &actCrystalShard) 
				&& !strncmp((char*)net_packet->data, "SALV", 4) )
			{
				// auto salvage this item.
				if ( players[net_packet->data[4]] && players[net_packet->data[4]]->entity )
				{
					entity->itemAutoSalvageByPlayer = static_cast<Sint32>(players[net_packet->data[4]]->entity->getUID());
				}
			}
			else if ( entity->behavior == &actItem && !strncmp((char*)net_packet->data, "RATF", 4) )
			{
				achievementObserver.playerAchievements[net_packet->data[4]].rat5000secondRule.insert(uid);
			}
			client_selected[net_packet->data[4]] = entity;
			inrange[net_packet->data[4]] = true;
		}
		return;
	}

	// clicked entity out of range
	else if (!strncmp((char*)net_packet->data, "CKOR", 4))
	{
		Uint32 uid = SDLNet_Read32(&net_packet->data[5]);
		Entity* entity = uidToEntity(uid);
		if ( entity )
		{
			client_selected[net_packet->data[4]] = entity;
			inrange[net_packet->data[4]] = false;
		}
		return;
	}

	// disconnect
	else if (!strncmp((char*)net_packet->data, "DISCONNECT", 10))
	{
		int playerDisconnected = net_packet->data[10];
		char shortname[32] = { 0 };
		strncpy(shortname, stats[playerDisconnected]->name, 22);
		client_disconnected[playerDisconnected] = true;
		for ( c = 1; c < MAXPLAYERS; c++ )
		{
			if ( client_disconnected[c] == true )
			{
				continue;
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

		// strncpy() does not copy N bytes if a terminating null is encountered first
		// see http://www.cplusplus.com/reference/cstring/strncpy/
		// see https://en.cppreference.com/w/c/string/byte/strncpy
		// GCC throws a warning (intended) when the length argument to strncpy() in
		// any way depends on strlen(src) to discourage this (and related) construct(s).
		strncpy(tempstr, stats[pnum]->name, 10);
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
				continue;
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
		Uint32 uid = SDLNet_Read32(&net_packet->data[5]);
		Entity* entity = uidToEntity(uid);
		if ( entity )
		{
			clickDescription(j, entity);
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
		entity = newEntity(-1, 1, map.entities, nullptr); //Item entity.
		entity->x = net_packet->data[26];
		entity->x = entity->x * 16 + 8;
		entity->y = net_packet->data[27];
		entity->y = entity->y * 16 + 8;
		entity->flags[NOUPDATE] = true;
		entity->flags[PASSABLE] = true;
		entity->flags[INVISIBLE] = true;
		for ( c = item->count; c > 0; c-- )
		{
			int qtyToDrop = 1;
			if ( c >= 10 && (item->type == TOOL_METAL_SCRAP || item->type == TOOL_MAGIC_SCRAP) )
			{
				qtyToDrop = 10;
				c -= 9;
			}
			else if ( itemTypeIsQuiver(item->type) )
			{
				qtyToDrop = item->count;
				c -= item->count;
			}
			dropItemMonster(item, entity, stats[net_packet->data[25]], qtyToDrop);
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

	// sneaking
	else if ( !strncmp((char*)net_packet->data, "SNEK", 4) )
	{
		stats[net_packet->data[4]]->sneaking = net_packet->data[5];
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
		int client = net_packet->data[29];
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
		item->appearance = SDLNet_Read32(&net_packet->data[20]);
		item->count = SDLNet_Read32(&net_packet->data[24]);
		if ( net_packet->data[28] )
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
			if (!itemCompare(item, item2, false))
			{
				printlog("client %d bought item from shop (uid=%d)\n", client, uidnum);
				if ( shopIsMysteriousShopkeeper(entity) )
				{
					buyItemFromMysteriousShopkeepConsumeOrb(client, *entity, *item2);
				}
				consumeItem(item2, client);
				break;
			}
		}
		entitystats->GOLD += item->buyValue(client);
		stats[client]->GOLD -= item->buyValue(client);
		if ( players[client] && players[client]->entity )
		{
			if ( rand() % 2 )
			{
				if ( item->buyValue(client) <= 1 )
				{
					// buying cheap items does not increase trading past basic
					if ( stats[client]->PROFICIENCIES[PRO_TRADING] < SKILL_LEVEL_SKILLED )
					{
						players[client]->entity->increaseSkill(PRO_TRADING);
					}
				}
				else
				{
					players[client]->entity->increaseSkill(PRO_TRADING);
				}
			}
			else if ( item->buyValue(client) >= 150 )
			{
				if ( item->buyValue(client) >= 300 || rand() % 2 )
				{
					players[client]->entity->increaseSkill(PRO_TRADING);
				}
			}
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
			node_t* nextnode;
			for (node = channeledSpells[client].first; node; node = nextnode )
			{
				nextnode = node->next;
				spell_t* spell_search = (spell_t*)node->element;
				if (spell_search->ID == thespell->ID)
				{
					spell_search->sustain = false;
				}
			}
		}
		return;
	}

	// sell item to shop
	else if (!strncmp((char*)net_packet->data, "SHPS", 4))
	{
		Uint32 uidnum = (Uint32)SDLNet_Read32(&net_packet->data[4]);
		int client = net_packet->data[29];
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
		if ( net_packet->data[28] )
		{
			item = newItem(static_cast<ItemType>(SDLNet_Read32(&net_packet->data[8])), static_cast<Status>(SDLNet_Read32(&net_packet->data[12])), 
				SDLNet_Read32(&net_packet->data[16]), SDLNet_Read32(&net_packet->data[24]), SDLNet_Read32(&net_packet->data[20]), true, &entitystats->inventory);
		}
		else
		{
			item = newItem(static_cast<ItemType>(SDLNet_Read32(&net_packet->data[8])), static_cast<Status>(SDLNet_Read32(&net_packet->data[12])), 
				SDLNet_Read32(&net_packet->data[16]), SDLNet_Read32(&net_packet->data[24]), SDLNet_Read32(&net_packet->data[20]), false, &entitystats->inventory);
		}
		printlog("client %d sold item to shop (uid=%d)\n", client, uidnum);
		stats[client]->GOLD += item->sellValue(client);
		if (rand() % 2 && item->type != GEM_GLASS )
		{
			if ( players[client] && players[client]->entity )
			{
				players[client]->entity->increaseSkill(PRO_TRADING);
			}
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

	// equip item (as a shield)
	else if ( !strncmp((char*)net_packet->data, "EQUS", 4) )
	{
		item = newItem(static_cast<ItemType>(SDLNet_Read32(&net_packet->data[4])), static_cast<Status>(SDLNet_Read32(&net_packet->data[8])), SDLNet_Read32(&net_packet->data[12]), SDLNet_Read32(&net_packet->data[16]), SDLNet_Read32(&net_packet->data[20]), net_packet->data[24], &stats[net_packet->data[25]]->inventory);
		equipItem(item, &stats[net_packet->data[25]]->shield, net_packet->data[25]);
		return;
	}

	// equip item (any other slot)
	else if ( !strncmp((char*)net_packet->data, "EQUM", 4) )
	{
		item = newItem(static_cast<ItemType>(SDLNet_Read32(&net_packet->data[4])), static_cast<Status>(SDLNet_Read32(&net_packet->data[8])), SDLNet_Read32(&net_packet->data[12]), SDLNet_Read32(&net_packet->data[16]), SDLNet_Read32(&net_packet->data[20]), net_packet->data[24], &stats[net_packet->data[25]]->inventory);
		
		switch ( net_packet->data[27] )
		{
			case EQUIP_ITEM_SLOT_WEAPON:
				equipItem(item, &stats[net_packet->data[25]]->weapon, net_packet->data[25]);
				break;
			case EQUIP_ITEM_SLOT_SHIELD:
				equipItem(item, &stats[net_packet->data[25]]->shield, net_packet->data[25]);
				break;
			case EQUIP_ITEM_SLOT_MASK:
				equipItem(item, &stats[net_packet->data[25]]->mask, net_packet->data[25]);
				break;
			case EQUIP_ITEM_SLOT_HELM:
				equipItem(item, &stats[net_packet->data[25]]->helmet, net_packet->data[25]);
				break;
			case EQUIP_ITEM_SLOT_GLOVES:
				equipItem(item, &stats[net_packet->data[25]]->gloves, net_packet->data[25]);
				break;
			case EQUIP_ITEM_SLOT_BOOTS:
				equipItem(item, &stats[net_packet->data[25]]->shoes, net_packet->data[25]);
				break;
			case EQUIP_ITEM_SLOT_BREASTPLATE:
				equipItem(item, &stats[net_packet->data[25]]->breastplate, net_packet->data[25]);
				break;
			case EQUIP_ITEM_SLOT_CLOAK:
				equipItem(item, &stats[net_packet->data[25]]->cloak, net_packet->data[25]);
				break;
			case EQUIP_ITEM_SLOT_AMULET:
				equipItem(item, &stats[net_packet->data[25]]->amulet, net_packet->data[25]);
				break;
			case EQUIP_ITEM_SLOT_RING:
				equipItem(item, &stats[net_packet->data[25]]->ring, net_packet->data[25]);
				break;
			default:
				break;
		}
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

	// apply item to entity
	else if ( !strncmp((char*)net_packet->data, "APIW", 4) )
	{
		item = newItem(static_cast<ItemType>(SDLNet_Read32(&net_packet->data[4])), static_cast<Status>(SDLNet_Read32(&net_packet->data[8])), SDLNet_Read32(&net_packet->data[12]), SDLNet_Read32(&net_packet->data[16]), SDLNet_Read32(&net_packet->data[20]), net_packet->data[24], NULL);
		int wallx = (SDLNet_Read16(&net_packet->data[26]));
		int wally = (SDLNet_Read16(&net_packet->data[28]));
		item->applyLockpickToWall(net_packet->data[25], wallx, wally);
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

	//The client failed some alchemy.
	else if ( !strncmp((char*)net_packet->data, "BOOM", 4) )
	{
		int the_client = net_packet->data[4];
		if ( players[the_client] && players[the_client]->entity )
		{
			spawnMagicTower(nullptr, players[the_client]->entity->x, players[the_client]->entity->y, SPELL_FIREBALL, nullptr);
			players[the_client]->entity->setObituary(language[3350]);
		}
		return;
	}

	//The client cast a spell.
	else if (!strncmp((char*)net_packet->data, "SPEL", 4))
	{
		int the_client = net_packet->data[4];

		spell_t* thespell = getSpellFromID(SDLNet_Read32(&net_packet->data[5]));
		if ( players[the_client] && players[the_client]->entity )
		{
			if ( net_packet->data[9] == 1 )
			{
				castSpell(players[the_client]->entity->getUID(), thespell, false, false, true);
			}
			else
			{
				castSpell(players[the_client]->entity->getUID(), thespell, false, false);
			}
		}
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
				item = nullptr;
				break;
		}
		if ( item != nullptr )
		{
			item->beatitude = 0;
		}
		return;
	}

	// the client repaired equipment or otherwise modified status of equipment.
	else if ( !strncmp((char*)net_packet->data, "REPA", 4) )
	{
		int player = net_packet->data[4];
		Item* equipment = nullptr;

		switch ( net_packet->data[5] )
		{
			case 0:
				equipment = stats[player]->weapon;
				break;
			case 1:
				equipment = stats[player]->helmet;
				break;
			case 2:
				equipment = stats[player]->breastplate;
				break;
			case 3:
				equipment = stats[player]->gloves;
				break;
			case 4:
				equipment = stats[player]->shoes;
				break;
			case 5:
				equipment = stats[player]->shield;
				break;
			case 6:
				equipment = stats[player]->cloak;
				break;
			case 7:
				equipment = stats[player]->mask;
				break;
			default:
				equipment = nullptr;
				break;
		}

		if ( !equipment )
		{
			return;
		}
		
		if ( static_cast<int>(net_packet->data[6]) > EXCELLENT )
		{
			equipment->status = EXCELLENT;
		}
		else if ( static_cast<int>(net_packet->data[6]) < BROKEN )
		{
			equipment->status = BROKEN;
		}
		equipment->status = static_cast<Status>(net_packet->data[6]);
		return;
	}

	// the client changed beatitude of equipment.
	else if ( !strncmp((char*)net_packet->data, "BEAT", 4) )
	{
		int player = net_packet->data[4];
		Item* equipment = nullptr;
		//messagePlayer(0, "client: %d, armornum: %d, status %d", player, net_packet->data[5], net_packet->data[6]);

		switch ( net_packet->data[5] )
		{
			case 0:
				equipment = stats[player]->weapon;
				break;
			case 1:
				equipment = stats[player]->helmet;
				break;
			case 2:
				equipment = stats[player]->breastplate;
				break;
			case 3:
				equipment = stats[player]->gloves;
				break;
			case 4:
				equipment = stats[player]->shoes;
				break;
			case 5:
				equipment = stats[player]->shield;
				break;
			case 6:
				equipment = stats[player]->cloak;
				break;
			case 7:
				equipment = stats[player]->mask;
				break;
			default:
				equipment = nullptr;
				break;
		}

		if ( !equipment )
		{
			return;
		}

		equipment->beatitude = net_packet->data[6] - 100; // we sent the data beatitude + 100
		//messagePlayer(0, "%d", equipment->beatitude);
		return;
	}

	// client dropped gold
	else if (!strncmp((char*)net_packet->data, "DGLD", 4))
	{
		int player = net_packet->data[4];
		int amount = SDLNet_Read32(&net_packet->data[5]);
		if ( players[player] && players[player]->entity )
		{
			stats[player]->GOLD -= amount;
			playSoundEntity(players[player]->entity, 242 + rand() % 4, 64);
			//Drop gold.
			int x = std::min<int>(std::max(0, (int)(players[player]->entity->x / 16)), map.width - 1);
			int y = std::min<int>(std::max(0, (int)(players[player]->entity->y / 16)), map.height - 1);
			if ( map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height] )
			{
				entity = newEntity(130, 0, map.entities, nullptr); // 130 = goldbag model
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x = players[player]->entity->x;
				entity->y = players[player]->entity->y;
				entity->z = 6;
				entity->yaw = (rand() % 360) * PI / 180.0;
				entity->flags[PASSABLE] = true;
				entity->flags[UPDATENEEDED] = true;
				entity->behavior = &actGoldBag;
				entity->goldAmount = amount; // amount
			}

		}
		return;
	}

	// client played a sound
	else if ( !strncmp((char*)net_packet->data, "EMOT", 4) )
	{
		int player = net_packet->data[4];
		int sfx = SDLNet_Read16(&net_packet->data[5]);
		if ( players[player] && players[player]->entity )
		{
			playSoundEntityLocal(players[player]->entity, sfx, 92);
			for ( int c = 1; c < MAXPLAYERS; ++c )
			{
				// send to all other players
				if ( c != player && !client_disconnected[c] && !players[c]->isLocalPlayer() )
				{
					strcpy((char*)net_packet->data, "SNEL");
					SDLNet_Write16(sfx, &net_packet->data[4]);
					SDLNet_Write32((Uint32)players[player]->entity->getUID(), &net_packet->data[6]);
					SDLNet_Write16(92, &net_packet->data[10]);
					net_packet->address.host = net_clients[c - 1].host;
					net_packet->address.port = net_clients[c - 1].port;
					net_packet->len = 12;
					sendPacketSafe(net_sock, -1, net_packet, c - 1);
				}
			}
		}
		return;
	}

	// the client asked for a level up
	else if ( !strncmp((char*)net_packet->data, "CLVL", 4) )
	{
		int player = net_packet->data[4];
		if ( players[player] && players[player]->entity )
		{
			players[player]->entity->getStats()->EXP += 100;
		}
		return;
	}

	// the client asked for a level up
	else if ( !strncmp((char*)net_packet->data, "CSKL", 4) )
	{
		int player = net_packet->data[4];
		int skill = net_packet->data[5];
		if ( player > 0 && player < MAXPLAYERS && players[player] && players[player]->entity )
		{
			if ( skill >= 0 && skill < NUMPROFICIENCIES )
			{
				players[player]->entity->increaseSkill(skill);
			}
		}
		return;
	}

	// the client sent a minimap ping packet.
	else if ( !strncmp((char*)net_packet->data, "PMAP", 4) )
	{
		MinimapPing newPing(ticks, net_packet->data[4], net_packet->data[5], net_packet->data[6]);
		sendMinimapPing(net_packet->data[4], newPing.x, newPing.y); // relay self and to other clients.
		return;
	}

	//Remove vampiric aura
	else if ( !strncmp((char*)net_packet->data, "VAMP", 4) )
	{
		int player = net_packet->data[4];
		int spellID = SDLNet_Read32(&net_packet->data[5]);
		if ( players[player] && players[player]->entity && stats[player] )
		{
			if ( client_classes[player] == CLASS_ACCURSED &&
				stats[player]->EFFECTS[EFF_VAMPIRICAURA] && players[player]->entity->playerVampireCurse == 1 )
			{
				players[player]->entity->setEffect(EFF_VAMPIRICAURA, true, 1, true);
				messagePlayerColor(player, uint32ColorGreen(*mainsurface), language[3241]);
				messagePlayerColor(player, uint32ColorGreen(*mainsurface), language[3242]);
				players[player]->entity->playerVampireCurse = 2; // cured.
				serverUpdateEntitySkill(players[player]->entity, 51);
				steamAchievementClient(player, "BARONY_ACH_REVERSE_THIS_CURSE");
				playSoundEntity(players[player]->entity, 402, 128);
				createParticleDropRising(players[player]->entity, 174, 1.0);
				serverSpawnMiscParticles(players[player]->entity, PARTICLE_EFFECT_RISING_DROP, 174);
			}
		}
		return;
	}

	// the client sent a monster command.
	else if ( !strncmp((char*)net_packet->data, "ALLY", 4) )
	{
		int player = net_packet->data[4];
		int allyCmd = net_packet->data[5];
		Uint32 uid = SDLNet_Read32(&net_packet->data[8]);
		//messagePlayer(0, " received %d, %d, %d, %d, %d", player, allyCmd, net_packet->data[6], net_packet->data[7], uid);
		Entity* entity = uidToEntity(uid);
		if ( entity )
		{
			if ( net_packet->len > 12 )
			{
				Uint32 interactUid = SDLNet_Read32(&net_packet->data[12]);
				entity->monsterAllySendCommand(allyCmd, net_packet->data[6], net_packet->data[7], interactUid);
				//messagePlayer(0, "received UID of target: %d, applying...", uid);
				entity->monsterAllyInteractTarget = interactUid;
			}
			else
			{
				entity->monsterAllySendCommand(allyCmd, net_packet->data[6], net_packet->data[7]);
			}
		}
	}

	else if ( !strncmp((char*)net_packet->data, "IDIE", 4) )
	{
		int playerDie = net_packet->data[4];
		if ( playerDie >= 1 && playerDie < MAXPLAYERS )
		{
			if ( players[playerDie] && players[playerDie]->entity )
			{
				players[playerDie]->entity->setHP(0);
			}
		}
	}

	// use automaton food item
	else if ( !strncmp((char*)net_packet->data, "FODA", 4) )
	{
		item = newItem(static_cast<ItemType>(SDLNet_Read32(&net_packet->data[4])), static_cast<Status>(SDLNet_Read32(&net_packet->data[8])), SDLNet_Read32(&net_packet->data[12]), SDLNet_Read32(&net_packet->data[16]), SDLNet_Read32(&net_packet->data[20]), net_packet->data[24], &stats[net_packet->data[25]]->inventory);
		item_FoodAutomaton(item, net_packet->data[25]);
		return;
	}

	else if ( !strncmp((char*)net_packet->data, "BARONY_JOIN_PROGRES", 19) )
	{
		if ( directConnect )
		{
			return;
		}
	}
}

/*-------------------------------------------------------------------------------

	serverHandleMessages

	Parses messages received from clients

-------------------------------------------------------------------------------*/

void serverHandleMessages(Uint32 framerateBreakInterval)
{
#ifdef STEAMWORKS
	if (!directConnect && !net_handler)
	{
		net_handler = new NetHandler();
		if ( !disableMultithreadedSteamNetworking )
		{
			net_handler->initializeMultithreadedPacketHandling();
		}
	}
#elif defined USE_EOS
	if ( !directConnect && !net_handler )
	{
		net_handler = new NetHandler();
	}
#endif

	if (!directConnect)
	{
#if defined(STEAMWORKS) || defined(USE_EOS)
		if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
		{
#ifdef STEAMWORKS
			//Steam stuff goes here.
			if ( disableMultithreadedSteamNetworking )
			{
				steamPacketThread(static_cast<void*>(net_handler));
			}
#endif
		}
		else if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
		{
#if defined USE_EOS
			EOSPacketThread(static_cast<void*>(net_handler));
#endif // USE_EOS
		}
		SteamPacketWrapper* packet = nullptr;

		if ( logCheckMainLoopTimers )
		{
			DebugStats.messagesT1 = std::chrono::high_resolution_clock::now();
			DebugStats.handlePacketStartLoop = true;
		}

		while (packet = net_handler->getGamePacket())
		{
			memcpy(net_packet->data, packet->data(), packet->len());
			net_packet->len = packet->len();

			serverHandlePacket(); //Uses net_packet;

			if ( logCheckMainLoopTimers )
			{
				DebugStats.messagesT2WhileLoop = std::chrono::high_resolution_clock::now();
				DebugStats.handlePacketStartLoop = false;
			}
			delete packet;

			if ( !disableFPSLimitOnNetworkMessages && !frameRateLimit(framerateBreakInterval, false) )
			{
				if ( logCheckMainLoopTimers )
				{
					printlog("[NETWORK]: Incoming messages exceeded given cycle time, packets remaining: %d", net_handler->game_packets.size());
				}
				break;
			}
		}
#endif
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
	node_t* node;
	int c, j;

	// safe packet
	if (!strncmp((char*)net_packet->data, "SAFE", 4))
	{
		if ( net_packet->data[4] != MAXPLAYERS )
		{
			int receivedPacketNum = SDLNet_Read32(&net_packet->data[5]);
			Uint8 fromClientnum = net_packet->data[4];

			if ( ticks > (60 * TICKS_PER_SECOND) && (ticks % (TICKS_PER_SECOND / 2) == 0) )
			{
				// clear old packets > 60 secs
				for ( auto it = safePacketsReceivedMap[fromClientnum].begin(); it != safePacketsReceivedMap[fromClientnum].end(); )
				{
					if ( it->second < (ticks - 60 * TICKS_PER_SECOND) )
					{
						it = safePacketsReceivedMap[fromClientnum].erase(it);
					}
					else
					{
						++it;
					}
				}
			}

			//auto tmp1 = std::chrono::high_resolution_clock::now();
			auto find = safePacketsReceivedMap[fromClientnum].find(receivedPacketNum);
			if ( find != safePacketsReceivedMap[fromClientnum].end() )
			{
				return true;
			}

			/*auto tmp2 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> time_span =
				std::chrono::duration_cast<std::chrono::duration<double>>(tmp2 - tmp1);
			double timer = time_span.count() * 1000;*/

			safePacketsReceivedMap[fromClientnum].insert(std::make_pair(receivedPacketNum, ticks));

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
				if ( j > 0 )
				{
					net_packet->address.host = net_clients[j - 1].host;
					net_packet->address.port = net_clients[j - 1].port;
				}
			}
			c = net_packet->len;
			net_packet->len = 9;
			if ( multiplayer == CLIENT )
			{
				sendPacket(net_sock, -1, net_packet, 0);
			}
			else
			{
				if ( j > 0 )
				{
					sendPacket(net_sock, -1, net_packet, j - 1);
				}
			}
			net_packet->len = c - 9;
			Uint8 bytedata[NET_PACKET_SIZE];
			memcpy(&bytedata, net_packet->data + 9, net_packet->len);
			memcpy(net_packet->data, &bytedata, net_packet->len);

			/*int sprite = -9999;
			char chr[5];
			chr[0] = '\0';
			strncpy(chr, (char*)net_packet->data, 4);
			chr[4] = '\0';
			if ( !strncmp((char*)net_packet->data, "ENTU", 4) )
			{
				Entity *entity = uidToEntity((int)SDLNet_Read32(&net_packet->data[4]));
				if ( entity )
				{
					sprite = entity->sprite;
				}
			}

			if ( !strncmp((char*)net_packet->data, "ENTS", 4) )
			{
				Entity *entity = uidToEntity((int)SDLNet_Read32(&net_packet->data[4]));
				if ( entity )
				{
					messagePlayer(clientnum, "%s | %d : %d - %d | %d %.8f", chr, entity->sprite, net_packet->data[8],
						SDLNet_Read32(&net_packet->data[9]), safePacketsReceivedMap[fromClientnum].size(), timer);
				}
			}
			else
			{
				messagePlayer(clientnum, "%s | %d %.8f", chr, safePacketsReceivedMap[fromClientnum].size(), timer);
			}*/
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
	if ( steam_packet_thread )
	{
		SDL_WaitThread(steam_packet_thread, NULL); //Wait for the thread to finish.
	}
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

void NetHandler::toggleMultithreading(bool disableMultithreading)
{
	if ( disableMultithreading )
	{
		// stop the old thread...
		if ( steam_packet_thread )
		{
			printlog("Waiting for steam_packet_thread to finish...");
			stopMultithreadedPacketHandling();
			if ( steam_packet_thread )
			{
				SDL_WaitThread(steam_packet_thread, NULL); //Wait for the thread to finish.
			}
			printlog("Done.\n");
			SDL_DestroyMutex(game_packets_lock);
			game_packets_lock = nullptr;
			SDL_DestroyMutex(continue_multithreading_steam_packets_lock);
			continue_multithreading_steam_packets_lock = nullptr;
			steam_packet_thread = nullptr;
		}
	}
	else
	{
		// create the new thread...
		steam_packet_thread = nullptr;
		continue_multithreading_steam_packets = false;
		game_packets_lock = SDL_CreateMutex();
		continue_multithreading_steam_packets_lock = SDL_CreateMutex();
		initializeMultithreadedPacketHandling();
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
	if ( !disableMultithreadedSteamNetworking )
	{
		SDL_LockMutex(game_packets_lock);
		game_packets.push(packet);
		SDL_UnlockMutex(game_packets_lock);
	}
	else
	{
		game_packets.push(packet);
	}
}

SteamPacketWrapper* NetHandler::getGamePacket()
{
	SteamPacketWrapper* packet = nullptr;
	if ( !disableMultithreadedSteamNetworking )
	{
		SDL_LockMutex(game_packets_lock);
		if (!game_packets.empty())
		{
			packet = game_packets.front();
			game_packets.pop();
		}
		SDL_UnlockMutex(game_packets_lock);
	}
	else
	{
		if ( !game_packets.empty() )
		{
			packet = game_packets.front();
			game_packets.pop();
		}
	}
	return packet;
}

int EOSPacketThread(void* data)
{
#ifdef USE_EOS
	if ( !EOS.CurrentUserInfo.isValid() )
	{
		//logError("EOSPacketThread: Invalid local user Id: %s", CurrentUserInfo.getProductUserIdStr());
		return -1;
	}

	if ( !data )
	{
		return -1;    //Some generic error?
	}

	NetHandler& handler = *static_cast<NetHandler*>(data); //Basically, our this.
	EOS_ProductUserId remoteId = nullptr;
	Uint32 packetlen = 0;
	Uint32 bytes_read = 0;
	Uint8* packet = nullptr;
	bool run = true;
	std::queue<SteamPacketWrapper* > packets; //TODO: Expose to game? Use lock-free packets?

	while ( run )   //1. Check if thread is supposed to be running.
	{
		//2. Game not over. Grab/poll for packet.

		//while (handler.getContinueMultithreadingSteamPackets() && SteamNetworking()->IsP2PPacketAvailable(&packetlen)) //Burst read in a bunch of packets.
		while (EOS.HandleReceivedMessages(&remoteId) )
		{
			packetlen = std::min<uint32_t>(net_packet->len, NET_PACKET_SIZE - 1);
			//Read packets and push into queue.
			packet = static_cast<Uint8*>(malloc(packetlen));
			memcpy(packet, net_packet->data, packetlen);
			if ( !EOSFuncs::Helpers_t::isMatchingProductIds(remoteId, EOS.CurrentUserInfo.getProductUserIdHandle())
				&& net_packet->data[0] )
			{
				//Push packet into queue.
				//TODO: Use lock-free queues?
				packets.push(new SteamPacketWrapper(packet, packetlen));
				packet = nullptr;
			}
			if ( packet )
			{
				free(packet);
			}
		}


		//3. Now push our local packetstack onto the game's network stack.
		//Well, that is: analyze packet.
		//If packet is good, push into queue.
		//If packet is bad, loop back to start of function.

		while ( !packets.empty() )
		{
			//Copy over the packets read in so far, and expose them to the game.
			SteamPacketWrapper* packet = packets.front();
			packets.pop();
			handler.addGamePacket(packet);
		}

		run = false; // only run thread once if multithreading disabled.
	}
#endif // USE_EOS

	return 0;
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

		if ( !disableMultithreadedSteamNetworking )
		{
			SDL_LockMutex(handler.continue_multithreading_steam_packets_lock);
			run = handler.getContinueMultithreadingSteamPackets();
			SDL_UnlockMutex(handler.continue_multithreading_steam_packets_lock);
		}
		else
		{
			run = false; // only run thread once if multithreading disabled.
		}
	}

#endif

	return 0; //If it isn't supposed to be running anymore, exit.

	//NOTE: This thread is to be created when the gameplay starts. NOT in the steam lobby.
	//If it's desired that it be created right when the network interfaces are opened, menu.c would need to be modified to support this, and the packet wrapper would need to include CSteamID.
}

/* ***** END MULTITHREADED STEAM PACKET HANDLING ***** */

void deleteMultiplayerSaveGames()
{
	if ( multiplayer != SERVER )
	{
		return;
	}

	//Only delete saves if no players are left alive.
	bool lastAlive = true;

	//const int playersAtStartOfMap = numplayers;
	//int currentPlayers = 0;
	//for ( int i = 0; i < MAXPLAYERS; ++i )
	//{
	//	if ( !client_disconnected[i] )
	//	{
	//		++currentPlayers;
	//	}
	//}

	//if ( currentPlayers != playersAtStartOfMap )
	//{
	//	return;
	//}

	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		Stat* stat = nullptr;
		if ( players[i] && players[i]->entity && (stat = players[i]->entity->getStats()) && stat->HP > 0)
		{
			lastAlive = false;
		}
	}
	if ( !lastAlive )
	{
		return;
	}

	deleteSaveGame(multiplayer); // stops save scumming c:

	for ( int i = 1; i < MAXPLAYERS; ++i )
	{
		if ( client_disconnected[i] )
		{
			continue;
		}
		strcpy((char *)net_packet->data,"DSAV"); //Delete save game.
		net_packet->address.host = net_clients[i - 1].host;
		net_packet->address.port = net_clients[i - 1].port;
		net_packet->len = 4;
		sendPacketSafe(net_sock, -1, net_packet, i - 1);
	}
}
