/*-------------------------------------------------------------------------------

	BARONY
	File: steam.hpp
	Desc: various definitions and prototypes for steam.cpp

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

//TODO: Bugger all void pointers and helper funcs on these.
void steam_OnP2PSessionRequest(void* p_Callback); //TODO: Finalize porting.
//void steam_OnGameOverlayActivated(void *callback);
void steam_OnLobbyMatchListCallback(void* pCallback, bool bIOFailure);
void steam_OnLobbyDataUpdatedCallback(void* pCallback);
void steam_OnLobbyCreated(void* pCallback, bool bIOFailure);
void steam_OnGameJoinRequested(void* pCallback);
void steam_ConnectToLobby(const char* arg);
void steam_OnLobbyEntered(void* pCallback, bool bIOFailure);
void steam_GameServerPingOnServerResponded(void* steamID);
void steam_OnP2PSessionConnectFail(void* pCallback);
void steam_OnRequestEncryptedAppTicket(void* pCallback, bool bIOFailure);

// determine if the given lobby is using a savegame; if it is, load our compatible save.
// @return true if we are able to join the lobby, otherwise false (because no compatible save was found)
bool processLobbyInvite(void* lobby);

#define MAX_STEAM_LOBBIES 100

extern Uint32 numSteamLobbies;
extern int selectedSteamLobby;
extern char lobbyText[MAX_STEAM_LOBBIES][64];
extern char lobbyVersion[MAX_STEAM_LOBBIES][64];
extern void* lobbyIDs[MAX_STEAM_LOBBIES];
extern int lobbyPlayers[MAX_STEAM_LOBBIES];
extern int lobbyNumMods[MAX_STEAM_LOBBIES];
extern bool lobbyModDisableAchievements[MAX_STEAM_LOBBIES];
extern char lobbyChallengeRun[MAX_STEAM_LOBBIES][64];

extern void* steamIDRemote[MAXPLAYERS]; //TODO: Bugger void pointer.

extern bool requestingLobbies;

#include <string>

extern void* currentLobby; // CSteamID to the current game lobby
extern std::string cmd_line; // for game join requests
#ifdef STEAMWORKS
extern char currentLobbyName[32];
extern ELobbyType currentLobbyType;
extern ELobbyType steamLobbyTypeUserConfigured;
extern bool steamLobbyFriendsOnlyUserConfigured;
extern bool steamLobbyInviteOnlyUserConfigured;
extern bool connectingToLobby, connectingToLobbyWindow;
extern bool joinLobbyWaitingForHostResponse;
extern bool denyLobbyJoinEvent;
extern int connectingToLobbyStatus;
extern bool steamAwaitingLobbyCreation;
#endif
const char* getRoomCode();



//These are all an utter bodge.
//They should not exist, but potato.
//TODO: Remove all of these wrappers and access the steam stuff directly.
SteamAPICall_t cpp_SteamMatchmaking_RequestLobbyList(const char* roomkey);
SteamAPICall_t cpp_SteamMatchmaking_JoinLobby(CSteamID steamIDLobby);
SteamAPICall_t cpp_SteamMatchmaking_CreateLobby(ELobbyType eLobbyType, int cMaxMembers);
SteamAPICall_t cpp_SteamMatchmaking_RequestAppTicket();

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
void SteamClientConsumeAuthTicket();
std::string SteamClientRequestAuthTicket();

enum ESteamLeaderboardTitles : int
{
    LEADERBOARD_NONE,
    LEADERBOARD_NORMAL_TIME,
    LEADERBOARD_NORMAL_SCORE,
    LEADERBOARD_MULTIPLAYER_TIME,
    LEADERBOARD_MULTIPLAYER_SCORE,
    LEADERBOARD_HELL_TIME,
    LEADERBOARD_HELL_SCORE,
    LEADERBOARD_HARDCORE_TIME,
    LEADERBOARD_HARDCORE_SCORE,
    LEADERBOARD_CLASSIC_TIME,
    LEADERBOARD_CLASSIC_SCORE,
    LEADERBOARD_CLASSIC_HARDCORE_TIME,
    LEADERBOARD_CLASSIC_HARDCORE_SCORE,
    LEADERBOARD_MULTIPLAYER_CLASSIC_TIME,
    LEADERBOARD_MULTIPLAYER_CLASSIC_SCORE,
    LEADERBOARD_MULTIPLAYER_HELL_TIME,
    LEADERBOARD_MULTIPLAYER_HELL_SCORE,
    LEADERBOARD_DLC_NORMAL_TIME,
    LEADERBOARD_DLC_NORMAL_SCORE,
    LEADERBOARD_DLC_MULTIPLAYER_TIME,
    LEADERBOARD_DLC_MULTIPLAYER_SCORE,
    LEADERBOARD_DLC_HELL_TIME,
    LEADERBOARD_DLC_HELL_SCORE,
    LEADERBOARD_DLC_HARDCORE_TIME,
    LEADERBOARD_DLC_HARDCORE_SCORE,
    LEADERBOARD_DLC_CLASSIC_TIME,
    LEADERBOARD_DLC_CLASSIC_SCORE,
    LEADERBOARD_DLC_CLASSIC_HARDCORE_TIME,
    LEADERBOARD_DLC_CLASSIC_HARDCORE_SCORE,
    LEADERBOARD_DLC_MULTIPLAYER_CLASSIC_TIME,
    LEADERBOARD_DLC_MULTIPLAYER_CLASSIC_SCORE,
    LEADERBOARD_DLC_MULTIPLAYER_HELL_TIME,
    LEADERBOARD_DLC_MULTIPLAYER_HELL_SCORE
};

class CSteamLeaderboards
{
private:
	SteamLeaderboard_t m_CurrentLeaderboard; // Handle to leaderboard
public:
	int m_nLeaderboardEntries; // How many entries do we have?
	static const int k_numEntriesToRetrieve = 100;
	LeaderboardEntry_t m_leaderboardEntries[k_numEntriesToRetrieve]; // The entries
	std::string leaderBoardSteamUsernames[k_numEntriesToRetrieve];
	static const int k_numLeaderboardTags = 64;
	int downloadedTags[k_numEntriesToRetrieve][k_numLeaderboardTags];

	bool b_ScoresDownloaded;
	bool b_LeaderboardInit;
	int currentLeaderBoardIndex;
	bool b_ScoreUploaded;
	bool b_ShowDLCScores;

	static const int k_numLeaderboards = 33;
	static const std::string leaderboardNames[k_numLeaderboards];

	CSteamLeaderboards() :
		m_CurrentLeaderboard(0),
		m_nLeaderboardEntries(0),
		b_ScoresDownloaded(false),
		b_LeaderboardInit(false),
		currentLeaderBoardIndex(0),
		b_ScoreUploaded(false),
		b_ShowDLCScores(false)
	{
		for ( int i = 0; i < k_numEntriesToRetrieve; ++i )
		{
			for ( int j = 0; j < k_numLeaderboardTags; ++j )
			{
				downloadedTags[i][j] = 0;
			}
			leaderBoardSteamUsernames[i] = "";
		}
	}
	~CSteamLeaderboards() {};

	class LastUploadResult_t {
	public:
		bool b_ScoreUploadComplete;
		int scoreUploaded;
		bool b_ScoreChanged;
		int globalRankNew;
		int globalRankPrev;

		LastUploadResult_t() :
			b_ScoreUploadComplete(false),
			scoreUploaded(0),
			b_ScoreChanged(false),
			globalRankNew(0),
			globalRankPrev(0)
		{}
	} LastUploadResult;

	class LeaderboardUpload_t {
	public:
		int status;
		int score;
		int time;
		int tags[k_numLeaderboardTags];
		int boardIndex;
		bool uploadInit;

		LeaderboardUpload_t() :
			status(0),
			score(0),
			time(0),
			boardIndex(0),
			uploadInit(false)
		{
			for ( int i = 0; i < k_numLeaderboardTags; ++i )
			{
				tags[i] = 0;
			}
		}
	} LeaderboardUpload;

	class LeaderboardView_t {
	public:
		int boardToDownload;
		ELeaderboardDataRequest requestType;
		int rangeStart;
		int rangeEnd;
		int scrollIndex;

		LeaderboardView_t() :
			boardToDownload(LEADERBOARD_NORMAL_TIME),
			requestType(k_ELeaderboardDataRequestGlobal),
			rangeStart(0),
			rangeEnd(k_numEntriesToRetrieve),
			scrollIndex(0)
		{}
	} LeaderboardView;

	void FindLeaderboard(const char *pchLeaderboardName);
	//bool UploadScore(int score);
	bool DownloadScores(ELeaderboardDataRequest dataRequestType, int rangeStart, int rangeEnd);
	void UploadScore(int scoreToSet, int tags[k_numLeaderboardTags]);
	void ClearUploadData();
	void ProcessLeaderboardUpload();

	void OnFindLeaderboard(LeaderboardFindResult_t *pResult, bool bIOFailure);
	CCallResult<CSteamLeaderboards, LeaderboardFindResult_t> m_callResultFindLeaderboard;
	void OnUploadScore(LeaderboardScoreUploaded_t *pResult, bool bIOFailure);
	CCallResult<CSteamLeaderboards, LeaderboardScoreUploaded_t> m_callResultUploadScore;
	void OnDownloadScore(LeaderboardScoresDownloaded_t *pResult, bool bIOFailure);
	CCallResult<CSteamLeaderboards, LeaderboardScoresDownloaded_t> m_callResultDownloadScore;
};

enum LeaderboardUploadState : int
{
	LEADERBOARD_STATE_NONE,
	LEADERBOARD_STATE_FIND_LEADERBOARD_TIME,
	LEADERBOARD_STATE_READY_TIME,
	LEADERBOARD_STATE_UPLOADING_TIME,
	LEADERBOARD_STATE_UPLOADING_SCORE
};

class CSteamWorkshop
{
private:
public:
	SteamUGCDetails_t m_subscribedItemListDetails[50]; // The entries
	std::array<std::string, 50> m_subscribedItemPreviewURL;
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
	void SubmitItemUpdate(const char *const changeNote);
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

class CSteamStatistics
{
private:
	SteamStat_t *m_pStats; // Stats data
	SteamGlobalStat_t *m_pGlobalStats; // Stats data
	int m_iNumStats; // The number of Stats
	int m_iNumGlobalStats; // The number of Global Stats
public:

	bool m_bInitialized; // Have we called Request stats and received the callback?
	CSteamStatistics(SteamStat_t* gStats, SteamGlobalStat_t* gGlobalStats, int numStatistics);
	~CSteamStatistics() {};

	bool RequestStats();
	bool StoreStats();
	bool ClearAllStats();
	bool RequestGlobalStats();

	STEAM_CALLBACK(CSteamStatistics, OnUserStatsReceived, UserStatsReceived_t, m_CallbackUserStatsReceived);
	STEAM_CALLBACK(CSteamStatistics, OnUserStatsStored, UserStatsStored_t, m_CallbackUserStatsStored);
	//STEAM_CALLBACK(CSteamStatistics, OnGlobalStatsReceived, GlobalStatsReceived_t, m_CallbackGlobalStatsReceived);
	CCallResult<CSteamStatistics, GlobalStatsReceived_t> m_CallbackGlobalStatsReceived;
	void OnGlobalStatsReceived(GlobalStatsReceived_t *pCallback, bool bIOFailure);
	//NOTE FOR FUTURE GLOBAL STATS NEEDS CCallResult NOT STEAM_CALLBACK!
};

enum LeaderboardScoreTags : int
{
	TAG_MONSTER_KILLS_1, // store 4 monster kills per index.
	TAG_MONSTER_KILLS_2,
	TAG_MONSTER_KILLS_3,
	TAG_MONSTER_KILLS_4,
	TAG_MONSTER_KILLS_5,
	TAG_MONSTER_KILLS_6,
	TAG_MONSTER_KILLS_7,
	TAG_MONSTER_KILLS_8,
	TAG_MONSTER_KILLS_9,
	TAG_MONSTER_KILLS_10,
	TAG_NAME1, // 4 chars per index
	TAG_NAME2,
	TAG_NAME3,
	TAG_NAME4,
	TAG_NAME5,
	TAG_NAME6,
	TAG_NAME7,
	TAG_NAME8,
	TAG_RACESEXAPPEARANCECLASS, // each offset by 8 bits
	TAG_VICTORYDUNGEONLEVELCONDUCTORIGINAL, // offset by 8 bits, 8 bits, 16 bits (4 original conducts fit into the 16 wide field)
	TAG_CONDUCT_2W_1, // each offset by 2 bits
	TAG_CONDUCT_2W_2, // each offset by 2 bits
	TAG_CONDUCT_4W_1, // each offset by 4 bits
	TAG_CONDUCT_4W_2, // each offset by 4 bits
	TAG_CONDUCT_4W_3, // each offset by 4 bits
	TAG_CONDUCT_4W_4, // each offset by 4 bits
	TAG_GAMEPLAY_STATS_16W_1, // each offset by 16 bits
	TAG_GAMEPLAY_STATS_16W_2, // each offset by 16 bits
	TAG_GAMEPLAY_STATS_16W_3, // each offset by 16 bits
	TAG_GAMEPLAY_STATS_16W_4, // each offset by 16 bits
	TAG_GAMEPLAY_STATS_8W_1, // each offset by 8 bits
	TAG_GAMEPLAY_STATS_8W_2, // each offset by 8 bits
	TAG_GAMEPLAY_STATS_4W_1, // each offset by 4 bits
	TAG_GAMEPLAY_STATS_4W_2, // each offset by 4 bits
	TAG_GAMEPLAY_STATS_2W_1, // each offset by 2 bits
	TAG_GAMEPLAY_STATS_2W_2, // each offset by 2 bits
	TAG_HEALTH, // offset by 16 bits
	TAG_MANA, // offset by 16 bits
	TAG_STRDEXCONINT, // offset by 8 bits
	TAG_PERCHREXPLVL, // offset by 8 bits
	TAG_GOLD,
	TAG_PROFICIENCY1, // each offset by 8 bits
	TAG_PROFICIENCY2, // each offset by 8 bits
	TAG_PROFICIENCY3, // each offset by 8 bits
	TAG_PROFICIENCY4, // each offset by 8 bits
	TAG_PROFICIENCY5, // each offset by 8 bits
	TAG_EQUIPMENT1, // each offset by 8 bits
	TAG_EQUIPMENT2, // each offset by 8 bits
	TAG_EQUIPMENT3, // each offset by 8 bits
	TAG_EQUIPMENT_BEATITUDE1, // each offset by 8 bits
	TAG_EQUIPMENT_BEATITUDE2, // each offset by 8 bits
	TAG_EQUIPMENT_BEATITUDE3, // each offset by 8 bits
	TAG_EQUIPMENT_APPEARANCE, // helm, cloak, breastplate, shield offset 4 bits each 
	TAG_TOTAL_SCORE,
	TAG_COMPLETION_TIME
};
