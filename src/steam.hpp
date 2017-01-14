/*-------------------------------------------------------------------------------

	BARONY
	File: steam.hpp
	Desc: various definitions and prototypes for steam.cpp

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

//TODO: Bugger all void pointers and helper funcs on these.
void steam_OnP2PSessionRequest(void *p_Callback); //TODO: Finalize porting.
//void steam_OnGameOverlayActivated(void *callback);
void steam_OnLobbyMatchListCallback(void *pCallback, bool bIOFailure);
void steam_OnLobbyDataUpdatedCallback(void *pCallback);
void steam_OnLobbyCreated(void *pCallback, bool bIOFailure);
void processLobbyInvite();
void steam_OnGameJoinRequested(void *pCallback);
void steam_ConnectToLobby();
void steam_OnLobbyEntered(void *pCallback, bool bIOFailure);
void steam_GameServerPingOnServerResponded(void *steamID);
void steam_OnP2PSessionConnectFail(void *pCallback);

#define MAX_STEAM_LOBBIES 100

extern int numSteamLobbies;
extern int selectedSteamLobby;
extern char lobbyText[MAX_STEAM_LOBBIES][32];
extern void *lobbyIDs[MAX_STEAM_LOBBIES];
extern int lobbyPlayers[MAX_STEAM_LOBBIES];

extern void *steamIDRemote[MAXPLAYERS]; //TODO: Bugger void pointer.

extern bool requestingLobbies;

extern bool serverLoadingSaveGame; // determines whether lobbyToConnectTo is loading a savegame or not
extern void *currentLobby; // CSteamID to the current game lobby
extern void *lobbyToConnectTo; // CSteamID of the game lobby that user has been invited to
extern char pchCmdLine[1024]; // for game join requests
extern char currentLobbyName[32];
#ifdef STEAMWORKS
extern ELobbyType currentLobbyType;
#endif

extern bool connectingToLobby, connectingToLobbyWindow;
extern bool stillConnectingToLobby;


//These are all an utter bodge.
//They should not exist, but potato.
//TODO: Remove all of these wrappers and access the steam stuff directly.
SteamAPICall_t cpp_SteamMatchmaking_RequestLobbyList();

SteamAPICall_t cpp_SteamMatchmaking_JoinLobby(CSteamID steamIDLobby);

SteamAPICall_t cpp_SteamMatchmaking_CreateLobby(ELobbyType eLobbyType, int cMaxMembers);

void cpp_SteamServerWrapper_Instantiate();

void cpp_SteamServerWrapper_Destroy();

void cpp_SteamServerClientWrapper_Instantiate();

void cpp_SteamServerClientWrapper_Destroy();

//This function is an utter bodge.
void cpp_Free_CSteamID(void *steamID);

//TODO: Ugh. Bugger these, replace with directly accessing the relevant stuff.
extern void (*cpp_SteamServerWrapper_OnSteamServersConnected)(void *pLogonSuccess);
extern void (*cpp_SteamServerWrapper_OnSteamServersConnectFailure)(void *pConnectFailure);
extern void (*cpp_SteamServerWrapper_OnSteamServersDisconnected)(void *pLoggedOff);
extern void (*cpp_SteamServerWrapper_OnPolicyResponse)(void *pPolicyResponse);
extern void (*cpp_SteamServerWrapper_OnValidateAuthTicketResponse)(void *pResponse);
extern void (*cpp_SteamServerWrapper_OnP2PSessionRequest)(void *p_Callback);
extern void (*cpp_SteamServerWrapper_OnP2PSessionConnectFail)(void *pCallback);

extern void (*cpp_SteamServerClientWrapper_OnLobbyDataUpdate)(void *pCallback);
extern void (*cpp_SteamServerClientWrapper_OnLobbyGameCreated)(void *pCallback);
extern void (*cpp_SteamServerClientWrapper_OnGameJoinRequested)(void *pCallback);
extern void (*cpp_SteamServerClientWrapper_OnAvatarImageLoaded)(void *pCallback);
extern void (*cpp_SteamServerClientWrapper_OnSteamServersConnected)(void *callback);
extern void (*cpp_SteamServerClientWrapper_OnSteamServersDisconnected)(void *callback);
extern void (*cpp_SteamServerClientWrapper_OnSteamServerConnectFailure)(void *callback);
//extern void (*cpp_SteamServerClientWrapper_OnGameOverlayActivated)(void *callback);
extern void (*cpp_SteamServerClientWrapper_OnGameWebCallback)(void *callback);
extern void (*cpp_SteamServerClientWrapper_OnWorkshopItemInstalled)(void *pParam);
extern void (*cpp_SteamServerClientWrapper_OnP2PSessionConnectFail)(void *pCallback);
extern void (*cpp_SteamServerClientWrapper_OnP2PSessionRequest)(void *pCallback);
extern void (*cpp_SteamServerClientWrapper_OnIPCFailure)(void *failure);
extern void (*cpp_SteamServerClientWrapper_OnSteamShutdown)(void *callback);
extern void (*cpp_SteamServerClientWrapper_OnLobbyCreated)(void *pCallback, bool bIOFailure); //Where pCallback is a pointer to type LobbyCreated_t.
extern void (*cpp_SteamServerClientWrapper_OnLobbyEntered)(void *pCallback, bool bIOFailure); //Where pCallback is a pointer to type LobbyEnter_t.
extern void (*cpp_SteamServerClientWrapper_OnLobbyMatchListCallback)(void *pCallback, bool bIOFailure); //Where pCallback is a pointer to type LobbyMatchList_t.
extern void (*cpp_SteamServerClientWrapper_OnRequestEncryptedAppTicket)(void *pEncryptedAppTicketResponse, bool bIOFailure); //Where pEncryptedAppTicketResponse is of type
extern void (*cpp_SteamServerClientWrapper_GameServerPingOnServerResponded)(void *steamID);
