/*-------------------------------------------------------------------------------

	BARONY
	File: steam.hpp
	Desc: various definitions and prototypes for steam.cpp

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include "game.hpp"

//TODO: Bugger all void pointers and helper funcs on these.
void steam_OnP2PSessionRequest(void* p_Callback); //TODO: Finalize porting.
//void steam_OnGameOverlayActivated(void *callback);
void steam_OnLobbyMatchListCallback(void* pCallback, bool bIOFailure);
void steam_OnLobbyDataUpdatedCallback(void* pCallback);
void steam_OnLobbyCreated(void* pCallback, bool bIOFailure);
void processLobbyInvite();
void steam_OnGameJoinRequested(void* pCallback);
void steam_ConnectToLobby();
void steam_OnLobbyEntered(void* pCallback, bool bIOFailure);
void steam_GameServerPingOnServerResponded(void* steamID);
void steam_OnP2PSessionConnectFail(void* pCallback);

#define MAX_STEAM_LOBBIES 100

extern int numSteamLobbies;
extern int selectedSteamLobby;
extern char lobbyText[MAX_STEAM_LOBBIES][48];
extern void* lobbyIDs[MAX_STEAM_LOBBIES];
extern int lobbyPlayers[MAX_STEAM_LOBBIES];

extern void* steamIDRemote[MAXPLAYERS]; //TODO: Bugger void pointer.

extern bool requestingLobbies;

extern bool serverLoadingSaveGame; // determines whether lobbyToConnectTo is loading a savegame or not
extern void* currentLobby; // CSteamID to the current game lobby
extern void* lobbyToConnectTo; // CSteamID of the game lobby that user has been invited to
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
void cpp_Free_CSteamID(void* steamID);

//TODO: Ugh. Bugger these, replace with directly accessing the relevant stuff.
extern void (*cpp_SteamServerWrapper_OnSteamServersConnected)(void* pLogonSuccess);
extern void (*cpp_SteamServerWrapper_OnSteamServersConnectFailure)(void* pConnectFailure);
extern void (*cpp_SteamServerWrapper_OnSteamServersDisconnected)(void* pLoggedOff);
extern void (*cpp_SteamServerWrapper_OnPolicyResponse)(void* pPolicyResponse);
extern void (*cpp_SteamServerWrapper_OnValidateAuthTicketResponse)(void* pResponse);
extern void (*cpp_SteamServerWrapper_OnP2PSessionRequest)(void* p_Callback);
extern void (*cpp_SteamServerWrapper_OnP2PSessionConnectFail)(void* pCallback);

extern void (*cpp_SteamServerClientWrapper_OnLobbyDataUpdate)(void* pCallback);
extern void (*cpp_SteamServerClientWrapper_OnLobbyGameCreated)(void* pCallback);
extern void (*cpp_SteamServerClientWrapper_OnGameJoinRequested)(void* pCallback);
extern void (*cpp_SteamServerClientWrapper_OnAvatarImageLoaded)(void* pCallback);
extern void (*cpp_SteamServerClientWrapper_OnSteamServersConnected)(void* callback);
extern void (*cpp_SteamServerClientWrapper_OnSteamServersDisconnected)(void* callback);
extern void (*cpp_SteamServerClientWrapper_OnSteamServerConnectFailure)(void* callback);
//extern void (*cpp_SteamServerClientWrapper_OnGameOverlayActivated)(void *callback);
extern void (*cpp_SteamServerClientWrapper_OnGameWebCallback)(void* callback);
extern void (*cpp_SteamServerClientWrapper_OnWorkshopItemInstalled)(void* pParam);
extern void (*cpp_SteamServerClientWrapper_OnP2PSessionConnectFail)(void* pCallback);
extern void (*cpp_SteamServerClientWrapper_OnP2PSessionRequest)(void* pCallback);
extern void (*cpp_SteamServerClientWrapper_OnIPCFailure)(void* failure);
extern void (*cpp_SteamServerClientWrapper_OnSteamShutdown)(void* callback);
extern void (*cpp_SteamServerClientWrapper_OnLobbyCreated)(void* pCallback, bool bIOFailure); //Where pCallback is a pointer to type LobbyCreated_t.
extern void (*cpp_SteamServerClientWrapper_OnLobbyEntered)(void* pCallback, bool bIOFailure); //Where pCallback is a pointer to type LobbyEnter_t.
extern void (*cpp_SteamServerClientWrapper_OnLobbyMatchListCallback)(void* pCallback, bool bIOFailure); //Where pCallback is a pointer to type LobbyMatchList_t.
extern void (*cpp_SteamServerClientWrapper_OnRequestEncryptedAppTicket)(void* pEncryptedAppTicketResponse, bool bIOFailure); //Where pEncryptedAppTicketResponse is of type
extern void (*cpp_SteamServerClientWrapper_GameServerPingOnServerResponded)(void* steamID);

class CSteamLeaderboards
{
private:
	SteamLeaderboard_t m_CurrentLeaderboard; // Handle to leaderboard
public:
	int m_nLeaderboardEntries; // How many entries do we have?
	LeaderboardEntry_t m_leaderboardEntries[10]; // The entries
	std::string leaderBoardSteamNames[256][10];// todo: requestUserInformation

	CSteamLeaderboards();
	~CSteamLeaderboards() {};

	void FindLeaderboard(const char *pchLeaderboardName);
	//bool UploadScore(int score);
	bool DownloadScores();

	void OnFindLeaderboard(LeaderboardFindResult_t *pResult, bool bIOFailure);
	CCallResult<CSteamLeaderboards, LeaderboardFindResult_t> m_callResultFindLeaderboard;
	/*void OnUploadScore(LeaderboardScoreUploaded_t *pResult, bool bIOFailure);
	CCallResult<CSteamLeaderboards, LeaderboardScoreUploaded_t> m_callResultUploadScore;
	*/
	void OnDownloadScore(LeaderboardScoresDownloaded_t *pResult, bool bIOFailure);
	CCallResult<CSteamLeaderboards, LeaderboardScoresDownloaded_t> m_callResultDownloadScore;
};

class CSteamWorkshop
{
private:
public:
	SteamUGCDetails_t m_subscribedItemListDetails[50]; // The entries
	SteamUGCDetails_t m_myWorkshopItemToModify;
	int numSubcribedItemResults = 50;
	int subscribedCallStatus;

	CSteamWorkshop();
	~CSteamWorkshop() {};

	CreateItemResult_t createItemResult;
	UGCUpdateHandle_t UGCUpdateHandle;
	SubmitItemUpdateResult_t SubmitItemUpdateResult;
	UGCQueryHandle_t UGCQueryHandle;
	SteamUGCQueryCompleted_t SteamUGCQueryCompleted;
	RemoteStorageUnsubscribePublishedFileResult_t UnsubscribePublishedFileResult;
	class LastActionResult_t {
		public:
			EResult lastResult;
			Uint32 creationTick;
			std::string actionMsg;

			LastActionResult_t() :
				lastResult(static_cast<EResult>(0)),
				creationTick(0),
				actionMsg("")
			{}
	} LastActionResult;
	std::list<std::string> workshopItemTags;
	Uint32 uploadSuccessTicks;

	void StoreResultMessage(std::string message, EResult result);
	void CreateItem();
	void StartItemUpdate();
	void SubmitItemUpdate(char* changeNote);
	void StartItemExistingUpdate(PublishedFileId_t fileId);
	void CreateQuerySubscribedItems(EUserUGCList itemListType, EUGCMatchingUGCType searchType, EUserUGCListSortOrder sortOrder);
	void ReadSubscribedItems();
	void UnsubscribeItemFileID(PublishedFileId_t fileId);

	CCallResult<CSteamWorkshop, CreateItemResult_t> m_callResultCreateItem;
	CCallResult<CSteamWorkshop, SubmitItemUpdateResult_t> m_callResultSubmitItemUpdateResult;
	CCallResult<CSteamWorkshop, SteamUGCQueryCompleted_t> m_callResultSendQueryUGCRequest;
	CCallResult<CSteamWorkshop, RemoteStorageUnsubscribePublishedFileResult_t> m_callResultUnsubscribeItemRequest;
	void OnCreateItem(CreateItemResult_t *pResult, bool bIOFailure);
	void OnSubmitItemUpdate(SubmitItemUpdateResult_t *pResult, bool bIOFailure);
	void OnSendQueryUGCRequest(SteamUGCQueryCompleted_t *pResult, bool bIOFailure);
	void OnUnsubscribeItemRequest(RemoteStorageUnsubscribePublishedFileResult_t *pResult, bool bIOFailure);
	//void OnStartItemUpdate(UGCUpdateHandle_t pResult, bool bIOFailure);
};

struct SteamStat_t
{
	int m_ID;
	ESteamStatTypes m_eStatType;
	const char *m_pchStatName;
	int m_iValue;
	float m_flValue;
	float m_flAvgNumerator;
	float m_flAvgDenominator;
};

class CSteamStatistics
{
private:
	SteamStat_t *m_pStats; // Stats data
	int m_iNumStats; // The number of Stats
	bool m_bInitialized; // Have we called Request stats and received the callback?
public:

	CSteamStatistics(SteamStat_t* gStats, int numStatistics);
	~CSteamStatistics() {};

	bool RequestStats();
	bool StoreStats();
	bool ClearAllStats();

	STEAM_CALLBACK(CSteamStatistics, OnUserStatsReceived, UserStatsReceived_t, m_CallbackUserStatsReceived);
	STEAM_CALLBACK(CSteamStatistics, OnUserStatsStored, UserStatsStored_t, m_CallbackUserStatsStored);
};