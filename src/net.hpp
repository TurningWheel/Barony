/*-------------------------------------------------------------------------------

	BARONY
	File: net.hpp
	Desc: prototypes and definitions for net.cpp

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include <queue>

#define DEFAULT_PORT 57165
#define LOBBY_CHATBOX_LENGTH 62
#define PACKET_LIMIT 200

extern char lobbyChatbox[LOBBY_CHATBOX_LENGTH];
extern list_t lobbyChatboxMessages;

// function prototypes for net.c:
int power(int a, int b);
int sendPacket(UDPsocket sock, int channel, UDPpacket* packet, int hostnum, bool tryReliable = false);
int sendPacketSafe(UDPsocket sock, int channel, UDPpacket* packet, int hostnum);
void messagePlayer(int player, char* message, ...);
void messagePlayerColor(int player, Uint32 color, char* message, ...);
void sendEntityUDP(Entity* entity, int c, bool guarantee);
void sendEntityTCP(Entity* entity, int c);
void sendMapSeedTCP(int c);
void sendMapTCP(int c);
void serverUpdateEntitySprite(Entity* entity);
void serverUpdateEntitySkill(Entity* entity, int skill);
void serverUpdateEntityFSkill(Entity* entity, int fskill);
void serverSpawnMiscParticles(Entity* entity, int particleType, int particleSprite);
void serverSpawnMiscParticlesAtLocation(Sint16 x, Sint16 y, Sint16 z, int particleType, int particleSprite);
void serverUpdateEntityFlag(Entity* entity, int flag);
void serverUpdateBodypartIDs(Entity* entity);
void serverUpdateEntityBodypart(Entity* entity, int bodypart);
void serverUpdateEffects(int player);
void serverUpdateHunger(int player);
void serverUpdatePlayerStats();
void serverUpdatePlayerGameplayStats(int player, int gameplayStat, int changeval);
void serverUpdatePlayerLVL();
void sendMinimapPing(Uint8 player, Uint8 x, Uint8 y);
void sendAllyCommandClient(int player, Uint32 uid, int command, Uint8 x, Uint8 y, Uint32 targetUid = 0);
Entity* receiveEntity(Entity* entity);
void clientActions(Entity* entity);
void clientHandleMessages();
void serverHandleMessages();
bool handleSafePacket();

void closeNetworkInterfaces();

// server/game flags
extern Uint32 svFlags;
const Uint32 NUM_SERVER_FLAGS =  7;
const Uint32 SV_FLAG_CHEATS  = 1;
const Uint32 SV_FLAG_FRIENDLYFIRE = 2;
const Uint32 SV_FLAG_MINOTAURS = 4;
const Uint32 SV_FLAG_HUNGER  = 8;
const Uint32 SV_FLAG_TRAPS = 16;
const Uint32 SV_FLAG_HARDCORE = 32;
const Uint32 SV_FLAG_CLASSIC = 64;

class SteamPacketWrapper
{
	Uint8* _data;
	int _len;
	//TODO: Encapsulate CSteam ID?
public:
	SteamPacketWrapper(Uint8* data, int len);
	~SteamPacketWrapper(); //NOTE: DOES free _data. Don't keep it somewhere else or segfaults will ensue. If you're lucky.

	Uint8*& data();
	int& len();
};

class NetHandler
{
	SDL_Thread* steam_packet_thread;
	bool continue_multithreading_steam_packets;

	std::queue<SteamPacketWrapper* > game_packets;

	SDL_mutex* game_packets_lock;
public:
	NetHandler();
	~NetHandler();

	void initializeMultithreadedPacketHandling();
	void stopMultithreadedPacketHandling();

	bool getContinueMultithreadingSteamPackets();

	void addGamePacket(SteamPacketWrapper* packet);

	/*
	 * This function will take the next packet in the queue, pop it off, and then return it.
	 * Returns nullptr if no packets.
	 * NOTE: You *MUST* free the data returned by this, or else you will leak memory! Such is the way of things.
	 */
	SteamPacketWrapper* getGamePacket();

	SDL_mutex* continue_multithreading_steam_packets_lock;
} extern* net_handler;

int steamPacketThread(void* data);

void deleteMultiplayerSaveGames(); //Server function, deletes its own save and broadcasts delete packet to clients.
