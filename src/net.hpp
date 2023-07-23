/*-------------------------------------------------------------------------------

	BARONY
	File: net.hpp
	Desc: prototypes and definitions for net.cpp

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include "game.hpp"
#include <queue>

#define DEFAULT_PORT 57165
#define LOBBY_CHATBOX_LENGTH 62
#define PACKET_LIMIT 200
#define TIMEOUT_TIME 60
#define TIMEOUT_WARNING_TIME 5

extern char lobbyChatbox[LOBBY_CHATBOX_LENGTH];
extern list_t lobbyChatboxMessages;

// function prototypes for net.c:
int power(int a, int b);
int sendPacket(UDPsocket sock, int channel, UDPpacket* packet, int hostnum, bool tryReliable = false);
int sendPacketSafe(UDPsocket sock, int channel, UDPpacket* packet, int hostnum);
bool messagePlayer(int player, Uint32 type, char const * const message, ...);
bool messageLocalPlayers(Uint32 type, char const * const message, ...);
bool messagePlayerColor(int player, Uint32 type, Uint32 color, char const * const message, ...);
bool messageLocalPlayersColor(Uint32 color, Uint32 type, char const * const message, ...);
void sendEntityUDP(Entity* entity, int c, bool guarantee);
void sendEntityTCP(Entity* entity, int c);
void sendMapSeedTCP(int c);
void sendMapTCP(int c);
void serverUpdateEntitySprite(Entity* entity);
void serverUpdateEntitySkill(Entity* entity, int skill);
void serverUpdateEntityFSkill(Entity* entity, int fskill);
void serverUpdateEntityStatFlag(Entity* entity, int flag);
void serverSpawnMiscParticles(Entity* entity, int particleType, int particleSprite, Uint32 optionalUid = 0);
void serverSpawnMiscParticlesAtLocation(Sint16 x, Sint16 y, Sint16 z, int particleType, int particleSprite);
void serverUpdateEntityFlag(Entity* entity, int flag);
void serverUpdateBodypartIDs(Entity* entity);
void serverUpdateEntityBodypart(Entity* entity, int bodypart);
void serverUpdateEffects(int player);
void serverUpdateHunger(int player);
void serverUpdateSexChange(int player);
void serverUpdatePlayerStats();
void serverUpdatePlayerGameplayStats(int player, int gameplayStat, int changeval);
void serverUpdatePlayerConduct(int player, int conduct, int value);
void serverUpdatePlayerLVL();
void serverRemoveClientFollower(int player, Uint32 uidToRemove);
void serverSendItemToPickupAndEquip(int player, Item* item);
void serverUpdateAllyStat(int player, Uint32 uidToUpdate, int LVL, int HP, int MAXHP, int type);
void serverUpdatePlayerSummonStrength(int player);
void serverUpdateAllyHP(int player, Uint32 uidToUpdate, int HP, int MAXHP, bool guarantee = false);
void sendMinimapPing(Uint8 player, Uint8 x, Uint8 y, Uint8 pingType = 0);
void sendAllyCommandClient(int player, Uint32 uid, int command, Uint8 x, Uint8 y, Uint32 targetUid = 0);
enum NetworkingLobbyJoinRequestResult : int
{
	NET_LOBBY_JOIN_P2P_FAILURE,
	NET_LOBBY_JOIN_P2P_SUCCESS,
	NET_LOBBY_JOIN_DIRECTIP_FAILURE,
	NET_LOBBY_JOIN_DIRECTIP_SUCCESS
};
NetworkingLobbyJoinRequestResult lobbyPlayerJoinRequest(int& outResult, bool lockedSlots[4]);
Entity* receiveEntity(Entity* entity);
void clientActions(Entity* entity);
void clientHandleMessages(Uint32 framerateBreakInterval);
void serverHandleMessages(Uint32 framerateBreakInterval);
bool handleSafePacket();

void pollNetworkForShutdown();
void closeNetworkInterfaces();

// server/game flags
extern Uint32 svFlags;
extern Uint32 settings_svFlags;
const Uint32 SV_FLAG_CHEATS  = 1 << 0;
const Uint32 SV_FLAG_FRIENDLYFIRE = 1 << 1;
const Uint32 SV_FLAG_MINOTAURS = 1 << 2;
const Uint32 SV_FLAG_HUNGER  = 1 << 3;
const Uint32 SV_FLAG_TRAPS = 1 << 4;
const Uint32 SV_FLAG_HARDCORE = 1 << 5;
const Uint32 SV_FLAG_CLASSIC = 1 << 6;
const Uint32 SV_FLAG_KEEPINVENTORY = 1 << 7;
const Uint32 SV_FLAG_LIFESAVING = 1 << 8;
const Uint32 NUM_SERVER_FLAGS =  9;

extern bool keepInventoryGlobal;

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
	SDL_mutex* game_packets_lock;
public:
	NetHandler();
	~NetHandler();
	std::queue<SteamPacketWrapper* > game_packets;

	void initializeMultithreadedPacketHandling();
	void stopMultithreadedPacketHandling();
	void toggleMultithreading(bool disableMultithreading);

	bool getContinueMultithreadingSteamPackets();

	void addGamePacket(SteamPacketWrapper* packet);

	/*
	 * This function will take the next packet in the queue, pop it off, and then return it.
	 * Returns nullptr if no packets.
	 * NOTE: You *MUST* free the data returned by this, or else you will leak memory! Such is the way of things.
	 */
	SteamPacketWrapper* getGamePacket();

	SDL_mutex* continue_multithreading_steam_packets_lock;
};
extern NetHandler* net_handler;

extern bool disableMultithreadedSteamNetworking;
extern bool disableFPSLimitOnNetworkMessages;

int steamPacketThread(void* data);
int EOSPacketThread(void* data);

void deleteMultiplayerSaveGames(); //Server function, deletes its own save and broadcasts delete packet to clients.

void handleScanPacket(); // when we receive a SCAN packet (request for lobby info)

struct PingNetworkStatus_t
{
	std::map<Uint32, Uint32> pings;
	Uint32 lastPingtime = 0;
	Uint32 lastSequence = 0;
	Uint32 oldestSequenceTicks = 0;
	Uint32 sequence = 0;
	Uint32 displayMillis = 0;
	Uint32 displayMillisImmediate = 0;
	Uint32 hudDisplayOKTicks = 0;
	bool needsUpdate = true;
	void saveDisplayMillis(bool forceUpdate = false);
	void clear()
	{
		pings.clear();
		needsUpdate = true;
		hudDisplayOKTicks = 0;
		lastPingtime = 0;
		lastSequence = 0;
		oldestSequenceTicks = 0;
		displayMillis = 0;
		sequence = 0;
		displayMillisImmediate = 0;
	}
	static bool bEnabled;
	static int pingLimitGreen;
	static int pingLimitYellow;
	static int pingLimitOrange;
	static bool pingHUDDisplayGreen;
	static bool pingHUDDisplayYellow;
	static bool pingHUDDisplayOrange;
	static bool pingHUDDisplayRed;
	static bool pingHUDShowOKBriefly;
	static bool pingHUDShowNumericValue;
	static void receive();
	static void respond();
	static void update();
	static void reset();
};
extern PingNetworkStatus_t PingNetworkStatus[MAXPLAYERS];