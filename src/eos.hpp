#pragma once

#include "Config.hpp"

#ifdef USE_EOS
#ifdef NINTENDO
#define EOS_BUILD_PLATFORM_NAME Switch
#include "eos_platform_prereqs.h"
#include "eos/Switch/eos_Switch.h"
#endif
#include "eos_sdk.h"
#include "eos_logging.h"
#include "eos_auth.h"
#include "eos_friends.h"
#include "eos_userinfo.h"
#include "eos_lobby.h"
#include "eos_p2p.h"
#include "eos_ui.h"
#include "eos_ecom.h"
#include "eos_achievements.h"
#include "eos_stats.h"
#include "eos_metrics.h"
#include <vector>
#include <iostream>
#include <map>
#include <atomic>
#include "net.hpp"
#include "physfs.h"

class EOSFuncs
{
	std::unordered_map<std::string, SteamStat_t*> statMappings;
	bool initialized = false;

public:
	bool isInitialized() const { return initialized; }

	std::string ProductId = "";
	std::string SandboxId = "";
	std::string DeploymentId = "";
	std::string ClientCredentialsId = "";
	std::string ClientCredentialsSecret = "";
	std::string CredentialName = "";
	std::string CredentialHost = "";
	std::vector<std::string> CommandLineArgs;
	class Accounts_t 
	{
	public:
#ifdef NINTENDO
		EOS_ELoginCredentialType AuthType = EOS_ELoginCredentialType::EOS_LCT_ExternalAuth;
#else
		EOS_ELoginCredentialType AuthType = EOS_ELoginCredentialType::EOS_LCT_Developer;
#endif
		EOS_EResult AccountAuthenticationStatus = EOS_EResult::EOS_NotConfigured;
		EOS_EResult AccountAuthenticationCompleted = EOS_EResult::EOS_NotConfigured;

		bool waitingForCallback = false;
		bool firstTimeSetupCompleted = false;
		bool initPopupWindow = false;
		Uint32 popupInitTicks = 0;
		Uint32 popupCurrentTicks = 0;
		Uint32 loadingTicks = 0;
		bool loginCriticalErrorOccurred = false;
		std::string authToken = "";
		Uint32 authTokenRefresh = 0;
		Uint32 authTokenTicks = 0;
		enum PopupType
		{
			POPUP_FULL,
			POPUP_TOAST
		};
		PopupType popupType = POPUP_TOAST;

		void handleLogin();
		void deinit()
		{

		}
	} AccountManager;


	class CrossplayAccounts_t
	{
	public:
		EOS_ContinuanceToken continuanceToken = nullptr;
		EOS_EResult connectLoginStatus = EOS_EResult::EOS_NotConfigured;
		EOS_EResult connectLoginCompleted = EOS_EResult::EOS_NotConfigured;
		bool promptActive = false;
		bool acceptedEula = false;

		bool trySetupFromSettingsMenu = false;
		bool awaitingConnectCallback = false;
		bool awaitingCreateUserCallback = false;
		bool awaitingAppTicketResponse = false;

		bool logOut = false;
		bool autologin = false;

		void handleLogin();
		void createDialogue();
		void createNotification();
		void acceptCrossplay();
		void denyCrossplay();
		void viewPrivacyPolicy();
		bool isLoggingIn();

		void resetOnFailure();
		static void retryCrossplaySetupOnFailure();
	} CrossplayAccountManager;

	const int kMaxLobbiesToSearch = 200;

	// global shenanigans
	bool bRequestingLobbies = false; // client is waiting for lobby data to display
	bool bConnectingToLobby = false; // if true, client is waiting for lobby join callback
	bool bConnectingToLobbyWindow = false; // client has a valid lobby window and has not encountered a new error window
	int ConnectingToLobbyStatus = static_cast<int>(EOS_EResult::EOS_Success); // if invalid lobby join attempt, set to non-success
	bool bJoinLobbyWaitingForHostResponse = false;
	//bool bStillConnectingToLobby = false; // TODO: client got a lobby invite and booted up the game with this?
	char currentLobbyName[32] = "";
	EOS_ELobbyPermissionLevel currentPermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED;
	EOS_ELobbyPermissionLevel currentPermissionLevelUserConfigured = EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED;
	bool bFriendsOnly = false; // if true the current lobby can only be found by friends
	bool bFriendsOnlyUserConfigured = true;

	char lobbySearchByCode[32] = "";

	std::unordered_set<EOS_ProductUserId> ProductIdsAwaitingAccountMappingCallback;
	std::unordered_map<EOS_ProductUserId, EOS_EpicAccountId> AccountMappings;
	std::unordered_map<EOS_ProductUserId, std::string> ExternalAccountMappings;

	EOS_EResult appRequiresRestart = EOS_EResult::EOS_NotConfigured;

	class Achievements_t
	{
	public:
		bool playerDataAwaitingCallback = false;
		bool definitionsAwaitingCallback = false;
		bool playerDataLoaded = false;
		bool definitionsLoaded = false;
		bool bAchievementsInit = false;
	} Achievements;

	class StatGlobal_t
	{
		EOS_ProductUserId productUserId = nullptr;
		bool bIsInit = false;
		Uint32 lastUpdateTicks = 0;
	public:
		bool bPromoEnabled = false;
		bool bDataQueued = false;
		bool bIsDisabled = false;
		EOS_ProductUserId getProductUserIdHandle() { return productUserId; }
		void init();
		void queryGlobalStatUser();
		void updateQueuedStats();
	} StatGlobalManager;

	// actually all pointers...
	EOS_HPlatform PlatformHandle = nullptr; 
	EOS_HPlatform ServerPlatformHandle = nullptr;
	EOS_HAuth AuthHandle = nullptr;
	EOS_HConnect ConnectHandle = nullptr;
	EOS_HFriends FriendsHandle = nullptr;
	EOS_HUserInfo UserInfoHandle = nullptr;
	EOS_HLobby LobbyHandle = nullptr;
	EOS_HLobbyModification LobbyModificationHandle = nullptr;
	EOS_HLobbyModification LobbyMemberModificationHandle = nullptr;
	EOS_HUI UIHandle = nullptr;
	EOS_HEcom EcomHandle = nullptr;
	EOS_HAchievements AchievementsHandle = nullptr;
	EOS_HStats StatsHandle = nullptr;

	class LobbyParameters_t {
	public:
		enum LobbySearchOptions : int
		{
			LOBBY_SEARCH_ALL,
			LOBBY_SEARCH_BY_LOBBYID
		};
		enum LobbyJoinOptions : int
		{
			LOBBY_DONT_JOIN,
			LOBBY_JOIN_FIRST_SEARCH_RESULT,
			LOBBY_UPDATE_CURRENTLOBBY
		};
		enum LobbySearchOptionIndices : int
		{
			SEARCH_OPTIONS,
			JOIN_OPTIONS
		};

		static const int kNumLobbySearchOptions = 2;
		EOS_HLobbyDetails lobbyToJoin = nullptr;
		int lobbySearchOptions[kNumLobbySearchOptions] = { 0 };
		LobbyParameters_t()
		{
			clearLobbySearchOptions();
		};
		void setLobbySearchOptions(LobbyParameters_t::LobbySearchOptions searchType, LobbyParameters_t::LobbyJoinOptions joinOptions)
		{
			lobbySearchOptions[SEARCH_OPTIONS] = searchType;
			lobbySearchOptions[JOIN_OPTIONS] = joinOptions;
		}
		void clearLobbySearchOptions()
		{
			for ( int i = 0; i < kNumLobbySearchOptions; ++i )
			{
				lobbySearchOptions[i] = 0;
			}
		}
		void setLobbyToJoin(EOS_HLobbyDetails newLobby)
		{
			clearLobbyToJoin();
			lobbyToJoin = newLobby;
		}
		void clearLobbyToJoin()
		{
			if ( lobbyToJoin )
			{
				EOS_LobbyDetails_Release(lobbyToJoin);
				lobbyToJoin = nullptr;
			}
		}

	} LobbyParameters;

	EOSFuncs() {};
	~EOSFuncs() {};

	static void EOS_CALL LoggingCallback(const EOS_LogMessage* log);
	static void EOS_CALL AuthLoginCompleteCallback(const EOS_Auth_LoginCallbackInfo* data);
	static void EOS_CALL FriendsQueryCallback(const EOS_Friends_QueryFriendsCallbackInfo* data);
	static void EOS_CALL UserInfoCallback(const EOS_UserInfo_QueryUserInfoCallbackInfo* data);
	static void EOS_CALL ConnectLoginCompleteCallback(const EOS_Connect_LoginCallbackInfo* Data);
	static void EOS_CALL ConnectLoginCrossplayCompleteCallback(const EOS_Connect_LoginCallbackInfo* Data);
	static void EOS_CALL OnCreateLobbyFinished(const EOS_Lobby_CreateLobbyCallbackInfo* Data);
	static void EOS_CALL OnLobbySearchFinished(const EOS_LobbySearch_FindCallbackInfo* data);
	static void EOS_CALL OnLobbyJoinCallback(const EOS_Lobby_JoinLobbyCallbackInfo* data);
	static void EOS_CALL OnLobbyLeaveCallback(const EOS_Lobby_LeaveLobbyCallbackInfo* data);
	static void EOS_CALL OnIncomingConnectionRequest(const EOS_P2P_OnIncomingConnectionRequestInfo* data);
	static void EOS_CALL OnLobbyUpdateFinished(const EOS_Lobby_UpdateLobbyCallbackInfo* data);
	static void EOS_CALL OnLobbyMemberUpdateFinished(const EOS_Lobby_UpdateLobbyCallbackInfo* data);
	static void EOS_CALL OnQueryAccountMappingsCallback(const EOS_Connect_QueryProductUserIdMappingsCallbackInfo* Data);
	static void EOS_CALL OnMemberUpdateReceived(const EOS_Lobby_LobbyMemberUpdateReceivedCallbackInfo* data);
	static void EOS_CALL OnMemberStatusReceived(const EOS_Lobby_LobbyMemberStatusReceivedCallbackInfo* data);
	static void EOS_CALL OnDestroyLobbyFinished(const EOS_Lobby_DestroyLobbyCallbackInfo* data);
	static void EOS_CALL OnLobbyUpdateReceived(const EOS_Lobby_LobbyUpdateReceivedCallbackInfo* data);
	static void EOS_CALL ConnectAuthExpirationCallback(const EOS_Connect_AuthExpirationCallbackInfo* data);
	static void EOS_CALL ShowFriendsCallback(const EOS_UI_ShowFriendsCallbackInfo* data);
	static void EOS_CALL OnCreateUserCallback(const EOS_Connect_CreateUserCallbackInfo* data);
	static void EOS_CALL OnCreateUserCrossplayCallback(const EOS_Connect_CreateUserCallbackInfo* data);
	static void EOS_CALL OnEcomQueryOwnershipCallback(const EOS_Ecom_QueryOwnershipCallbackInfo* data);
	static void EOS_CALL OnEcomQueryEntitlementsCallback(const EOS_Ecom_QueryEntitlementsCallbackInfo* data);
	static void EOS_CALL OnUnlockAchievement(const EOS_Achievements_OnUnlockAchievementsCompleteCallbackInfo* data);
	static void EOS_CALL OnAchievementQueryComplete(const EOS_Achievements_OnQueryDefinitionsCompleteCallbackInfo* data);
	static void EOS_CALL OnPlayerAchievementQueryComplete(const EOS_Achievements_OnQueryPlayerAchievementsCompleteCallbackInfo* data);
	static void EOS_CALL OnIngestStatComplete(const EOS_Stats_IngestStatCompleteCallbackInfo* data);
	static void EOS_CALL OnQueryAllStatsCallback(const EOS_Stats_OnQueryStatsCompleteCallbackInfo* data);

	class FriendInfo_t {
	public:
		std::string EpicAccountId = "";
		std::string Name = "Pending...";
		EOS_EFriendsStatus Status;
		bool bUserInfoRequireUpdate = true;
		FriendInfo_t(EOS_EFriendsStatus status, std::string epicIdStr)
		{
			EpicAccountId = epicIdStr;
			Status = status;
		};
	};
	class LobbyData_t {
	public:
		Uint32 MaxPlayers = 0;
		std::string LobbyId = "";
		std::string OwnerProductUserId = "";
		Uint32 FreeSlots = 0;
		bool bLobbyHasFullDetailsRead = false;
		bool bLobbyHasBasicDetailsRead = false;
		bool bAwaitingLeaveCallback = false;
		bool bAwaitingCreationCallback = false;
		EOS_EResult LobbyCreationResult = EOS_EResult::EOS_Success;
		bool bDenyLobbyJoinEvent = false;

		class PlayerLobbyData_t {
		public:
			std::string memberProductUserId = "";
			std::string memberEpicAccountId = "";
			std::string name = "";
			int clientNumber = -1;
			bool bUserInfoRequireUpdate = true;
			EOS_EExternalAccountType accountType = EOS_EExternalAccountType::EOS_EAT_EPIC;
		};
		std::vector<PlayerLobbyData_t> playersInLobby;
		std::vector<EOS_ProductUserId> lobbyMembersQueueToMappingUpdate;

		EOS_NotificationId LobbyMemberUpdateNotification = EOS_INVALID_NOTIFICATIONID;
		EOS_NotificationId LobbyMemberStatusNotification = EOS_INVALID_NOTIFICATIONID;
		EOS_NotificationId LobbyUpdateNotification = EOS_INVALID_NOTIFICATIONID;
		//std::vector<EOS_ProductUserId> lobbyMembersReceivedUpdate;

		void ClearData()
		{
			MaxPlayers = 0;
			FreeSlots = 0;
			LobbyId = "";
			OwnerProductUserId = "";
			playersInLobby.clear();
			bLobbyHasFullDetailsRead = false;
			bLobbyHasBasicDetailsRead = false;
			bDenyLobbyJoinEvent = false;
			LobbyAttributes.ClearData();
		};
		bool currentLobbyIsValid()
		{
			if ( LobbyId.compare("") == 0 )
			{
				return false;
			}
			return true;
		}
		class LobbyAttributes_t
		{
			public:
				std::string lobbyName = "";
				std::string gameVersion = "";
				std::string gameJoinKey = "";
				std::string challengeLid = "";
				Uint32 isLobbyLoadingSavedGame = 0;
				Uint32 lobbyKey = 0;
				Uint32 serverFlags = 0;
				int numServerMods = 0;
				bool modsDisableAchievements = false;
				long long lobbyCreationTime = 0;
				int gameCurrentLevel = -1;
				Uint32 maxplayersCompatible = MAXPLAYERS;
				Uint32 PermissionLevel = static_cast<Uint32>(EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED);
				bool friendsOnly = false;
				void ClearData()
				{
					lobbyName = "";
					gameVersion = "";
					gameJoinKey = "";
					challengeLid = "";
					isLobbyLoadingSavedGame = 0;
					lobbyKey = 0;
					serverFlags = 0;
					numServerMods = 0;
					modsDisableAchievements = false;
					lobbyCreationTime = 0;
					gameCurrentLevel = -1;
					maxplayersCompatible = MAXPLAYERS;
					PermissionLevel = static_cast<Uint32>(EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED);
					friendsOnly = false;
				}
		} LobbyAttributes;
		enum HostUpdateLobbyTypes : int
		{
			LOBBY_UPDATE_MAIN_MENU,
			LOBBY_UPDATE_DURING_GAME
		};
		bool updateLobbyForHost(HostUpdateLobbyTypes updateType);
		void getLobbyAttributes(EOS_HLobbyDetails LobbyDetails);
		void getLobbyMemberInfo(EOS_HLobbyDetails LobbyDetails);
		void setLobbyAttributesAfterReading(EOS_Lobby_AttributeData* data);
		void setLobbyAttributesFromGame(HostUpdateLobbyTypes updateType);
		void setBasicCurrentLobbyDataFromInitialJoin(LobbyData_t* lobbyToJoin);
		void destroyLobby();
		bool currentUserIsOwner();
		void updateLobby();
		void updateLobbyDuringGameLoop();
		bool assignClientnumMemberAttribute(EOS_ProductUserId targetId, int clientNumToSet);
		int getClientnumMemberAttribute(EOS_ProductUserId targetId);
		bool modifyLobbyMemberAttributeForCurrentUser();

		void SubscribeToLobbyUpdates();
		void UnsubscribeFromLobbyUpdates();

		enum AttributeTypes : int
		{
			LOBBY_NAME,
			GAME_VERSION,
			LOADING_SAVEGAME,
			SERVER_FLAGS,
			GAME_MODS,
			CREATION_TIME,
			GAME_CURRENT_LEVEL,
			GAME_JOIN_KEY,
			LOBBY_PERMISSION_LEVEL,
			FRIENDS_ONLY,
			GAME_MAXPLAYERS,
			GAME_MODS_DISABLE_ACHIEVEMENTS,
			LOBBY_KEY,
			CHALLENGE_LID,
			ATTRIBUTE_TYPE_SIZE
		};
		const int kNumAttributes = ATTRIBUTE_TYPE_SIZE;
		std::pair<std::string, std::string> getAttributePair(AttributeTypes type);

	} CurrentLobbyData;

	class LobbySearchResults_t {
	public:
		EOS_HLobbySearch CurrentLobbySearch = nullptr;
		std::vector<LobbyData_t> results;
		std::vector<std::pair<LobbyData_t::LobbyAttributes_t, int>> resultsSortedForDisplay;
		int selectedLobby = 0;
		void sortResults();
		bool showLobbiesInProgress = false;
		bool useLobbyCode = false;
		bool lastResultWasFiltered = false;
		char lobbyLastSearchByCode[32] = "";
		LobbyData_t* getResultFromDisplayedIndex(int index)
		{
			return &results.at(resultsSortedForDisplay.at(index).second);
		}
	} LobbySearchResults;

	class CurrentUserInfo_t {
		EOS_ProductUserId productUserIdHandle = nullptr;
		std::string productUserId = "";
	public:
		std::string epicAccountId = "";
		bool bUserLoggedIn = false;
		std::vector<FriendInfo_t> Friends;
		bool bUserInfoRequireUpdate = true;
		bool bFriendsUserInfoRequireUpdate = false;
		std::string Name = "Pending...";

		bool isLoggedIn()
		{
			return bUserLoggedIn;
		}
		bool isValid()
		{
			if ( !productUserIdHandle || productUserId.compare("") == 0 )
			{
				return false;
			}
			return true;
		}
		void setProductUserIdHandle(EOS_ProductUserId id)
		{
			productUserIdHandle = id;
			productUserId = EOSFuncs::Helpers_t::productIdToString(id);
		}
		EOS_ProductUserId getProductUserIdHandle() { return productUserIdHandle; }
		const char* getProductUserIdStr() { return productUserId.c_str(); }
	} CurrentUserInfo;

	class Notifications_t {
	public:
		EOS_NotificationId P2PConnection = EOS_INVALID_NOTIFICATIONID;
		EOS_NotificationId ConnectAuthExpirationId = EOS_INVALID_NOTIFICATIONID;
	} NotificationIds;

	class P2PConnectionInfo_t {
	public:
		std::vector<std::pair<EOS_ProductUserId, int>> peerProductIds;
		EOS_ProductUserId serverProductId = nullptr;
		EOS_ProductUserId getPeerIdFromIndex(int index) const;
		int getIndexFromPeerId(EOS_ProductUserId id) const;
		void insertProductIdIntoPeers(EOS_ProductUserId newId)
		{
			if ( newId == nullptr )
			{
				return;
			}
			bool found = false;
			for ( auto& pair : peerProductIds )
			{
				if ( pair.first == newId )
				{
					found = true;
					break;
				}
			}
			if ( !found )
			{
				peerProductIds.push_back(std::make_pair(newId, -1));
			}
		}
		bool isPeerIndexed(EOS_ProductUserId id);
		bool assignPeerIndex(EOS_ProductUserId id, int index);
		bool isPeerStillValid(int index) const;
		void resetPeersAndServerData();
	} P2PConnectionInfo;

	enum UserInfoQueryType : int
	{
		USER_INFO_QUERY_NONE,
		USER_INFO_QUERY_LOCAL,
		USER_INFO_QUERY_LOBBY_MEMBER,
		USER_INFO_QUERY_FRIEND
	};

	struct UserInfoQueryData_t
	{
		EOS_EpicAccountId epicAccountId;
		UserInfoQueryType queryType;
		int optionalIndex;
	};

	void stop()
	{
		if (!initialized)
		{
			return;
		}
		if (CurrentLobbyData.currentLobbyIsValid())
		{
			leaveLobby();

			Uint32 shutdownTicks = SDL_GetTicks();
			while (CurrentLobbyData.bAwaitingLeaveCallback)
			{
#ifdef APPLE
				SDL_Event event;
				while (SDL_PollEvent(&event) != 0)
				{
					//Makes Mac work because Apple had to do it different.
				}
#endif]
				if (PlatformHandle) {
					EOS_Platform_Tick(PlatformHandle);
				}
				SDL_Delay(1);
				if (SDL_GetTicks() - shutdownTicks >= 1000)
				{
					// only give 1 second for the leave callback to complete.
					break;
				}
			}
		}
		if ( LobbySearchResults.CurrentLobbySearch )
		{
			EOS_LobbySearch_Release(LobbySearchResults.CurrentLobbySearch);
			LobbySearchResults.CurrentLobbySearch = nullptr;
		}
		AccountManager.deinit();
		UnsubscribeFromConnectionRequests();
		SetNetworkAvailable(false);
		if (PlatformHandle)
		{
			EOS_Platform_Release(PlatformHandle);
			PlatformHandle = nullptr;
		}
		if (ServerPlatformHandle)
		{
			EOS_Platform_Release(ServerPlatformHandle);
			ServerPlatformHandle = nullptr;
		}
		initialized = false;
		logInfo("Stop completed.");
	}

	void quit()
	{
		EOS_EResult result = EOS_Shutdown();
		if (result != EOS_EResult::EOS_Success)
		{
			logError("Shutdown error! code: %d", static_cast<int>(result));
		}
		else
		{
			logInfo("Shutdown completed.");
		}
	}

	bool initPlatform(bool enableLogging);
	bool initAuth()
	{
		return initAuth(CredentialHost, CredentialName);
	}
	bool initAchievements();
	bool initAuth(std::string hostname, std::string tokenName);
	void initConnectLogin();
	std::string getAuthToken();

	void queryFriends()
	{
		FriendsHandle = EOS_Platform_GetFriendsInterface(PlatformHandle);
		EOS_Friends_QueryFriendsOptions FriendsOptions;
		FriendsOptions.ApiVersion = EOS_FRIENDS_QUERYFRIENDS_API_LATEST;
		FriendsOptions.LocalUserId = EOSFuncs::Helpers_t::epicIdFromString(CurrentUserInfo.epicAccountId.c_str());
		EOS_Friends_QueryFriends(FriendsHandle, &FriendsOptions, nullptr, FriendsQueryCallback);
	}

	void getUserInfo(EOS_EpicAccountId targetId, UserInfoQueryType queryType, int index)
	{
		UserInfoHandle = EOS_Platform_GetUserInfoInterface(PlatformHandle);
		EOS_UserInfo_QueryUserInfoOptions UserInfoQueryOptions;
		UserInfoQueryOptions.ApiVersion = EOS_USERINFO_QUERYUSERINFO_API_LATEST;
		UserInfoQueryOptions.LocalUserId = EOSFuncs::Helpers_t::epicIdFromString(CurrentUserInfo.epicAccountId.c_str());
		UserInfoQueryOptions.TargetUserId = targetId;

		UserInfoQueryData_t* userInfoQueryData = new UserInfoQueryData_t();
		userInfoQueryData->epicAccountId = targetId;
		userInfoQueryData->queryType = queryType;
		userInfoQueryData->optionalIndex = index;

		EOS_UserInfo_QueryUserInfo(UserInfoHandle, &UserInfoQueryOptions, userInfoQueryData, UserInfoCallback);
	}

	void getExternalAccountUserInfo(EOS_ProductUserId targetId, UserInfoQueryType queryType);

	void createLobby();
	void joinLobby(LobbyData_t* lobby);
	void leaveLobby();
	void searchLobbies(LobbyParameters_t::LobbySearchOptions searchType,
		LobbyParameters_t::LobbyJoinOptions joinOptions, EOS_LobbyId lobbyIdToSearch);
	std::string getLobbyCodeFromGameKey(Uint32 key);
	Uint32 getGameKeyFromLobbyCode(std::string& code);

	void setLobbyDetailsFromHandle(EOS_HLobbyDetails LobbyDetails, LobbyData_t* LobbyToSet)
	{
		//get owner
		EOS_LobbyDetails_GetLobbyOwnerOptions GetOwnerOptions;
		GetOwnerOptions.ApiVersion = EOS_LOBBYDETAILS_GETLOBBYOWNER_API_LATEST;
		EOS_ProductUserId lobbyOwner = EOS_LobbyDetails_GetLobbyOwner(LobbyDetails, &GetOwnerOptions);
		/*if (NewLobbyOwner != LobbyOwner)
		{
		LobbyOwner = NewLobbyOwner;
		LobbyOwnerAccountId = FEpicAccountId();
		LobbyOwnerDisplayName.clear();
		}*/

		//copy lobby info
		EOS_LobbyDetails_CopyInfoOptions CopyInfoDetails;
		CopyInfoDetails.ApiVersion = EOS_LOBBYDETAILS_COPYINFO_API_LATEST;
		EOS_LobbyDetails_Info* LobbyInfo = nullptr;
		EOS_EResult result = EOS_LobbyDetails_CopyInfo(LobbyDetails, &CopyInfoDetails, &LobbyInfo);
		if ( result != EOS_EResult::EOS_Success || !LobbyInfo )
		{
			logError("setLobbyDetailsFromHandle: Lobbies can't copy lobby info: %d", static_cast<int>(result));
			return;
		}

		LobbyToSet->LobbyId = LobbyInfo->LobbyId;
		LobbyToSet->MaxPlayers = LobbyInfo->MaxMembers;
		LobbyToSet->FreeSlots = LobbyInfo->AvailableSlots;
		LobbyToSet->OwnerProductUserId = EOSFuncs::Helpers_t::productIdToString(lobbyOwner);
		EOS_LobbyDetails_Info_Release(LobbyInfo);

		LobbyToSet->getLobbyAttributes(LobbyDetails);
		LobbyToSet->getLobbyMemberInfo(LobbyDetails);

	}

	void SubscribeToConnectionRequests()
	{
		if ( NotificationIds.P2PConnection == EOS_INVALID_NOTIFICATIONID )
		{
			EOS_HP2P P2PHandle = EOS_Platform_GetP2PInterface(PlatformHandle);

			EOS_P2P_SocketId SocketId = {};
			SocketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
			strncpy(SocketId.SocketName, "CHAT", 5);

			EOS_P2P_AddNotifyPeerConnectionRequestOptions ConnectionRequestOptions;
			ConnectionRequestOptions.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONREQUEST_API_LATEST;
			ConnectionRequestOptions.LocalUserId = CurrentUserInfo.getProductUserIdHandle();
			ConnectionRequestOptions.SocketId = &SocketId;

			NotificationIds.P2PConnection = EOS_P2P_AddNotifyPeerConnectionRequest(P2PHandle,
				&ConnectionRequestOptions, nullptr, OnIncomingConnectionRequest);
			if ( NotificationIds.P2PConnection == EOS_INVALID_NOTIFICATIONID )
			{
				logError("SubscribeToConnectionRequests: could not subscribe, bad notification id returned.");
			}
		}
	}

	void UnsubscribeFromConnectionRequests()
	{
		if ( NotificationIds.P2PConnection == EOS_INVALID_NOTIFICATIONID )
		{
			return;
		}
		EOS_HP2P P2PHandle = EOS_Platform_GetP2PInterface(PlatformHandle);
		EOS_P2P_RemoveNotifyPeerConnectionRequest(P2PHandle, NotificationIds.P2PConnection);
		NotificationIds.P2PConnection = EOS_INVALID_NOTIFICATIONID;
	}

	void AddConnectAuthExpirationNotification()
	{
		EOS_HConnect ConnectHandle = EOS_Platform_GetConnectInterface(PlatformHandle);
		if ( NotificationIds.ConnectAuthExpirationId == EOS_INVALID_NOTIFICATIONID )
		{
			EOS_Connect_AddNotifyAuthExpirationOptions Options;
			Options.ApiVersion = EOS_CONNECT_ADDNOTIFYAUTHEXPIRATION_API_LATEST;
			NotificationIds.ConnectAuthExpirationId = EOS_Connect_AddNotifyAuthExpiration(ConnectHandle, &Options, NULL, ConnectAuthExpirationCallback);
		}
	}

	class Helpers_t
	{
		public:
			static bool productIdIsValid(EOS_ProductUserId id)
			{
				return (EOS_ProductUserId_IsValid(id) == EOS_TRUE);
			}
			static const char* productIdToString(EOS_ProductUserId id)
			{
				if ( !id )
				{
					return "NULL";
				}
				if ( !productIdIsValid(id) )
				{
					return "Invalid EOS_ProductUserId";
				}
				else
				{
					static char buffer[EOS_PRODUCTUSERID_MAX_LENGTH + 1];
					int bufferSize = sizeof(buffer);
					EOS_EResult result = EOS_ProductUserId_ToString(id, buffer, &bufferSize);
					if ( result == EOS_EResult::EOS_Success )
					{
						return buffer;
					}
					return "Error";
				}
			}
			static std::string shortProductIdToString(EOS_ProductUserId id)
			{
				std::string str = productIdToString(id);
				if ( id && productIdIsValid(id) && str.compare("Error") )
				{
					std::string shortStr = str.substr(0, 3);
					shortStr.append("...");
					shortStr.append(str.substr(str.size() - 3));
					return shortStr;
				}
				return str;
			}
			static bool epicIdIsValid(EOS_EpicAccountId id)
			{
				return (EOS_EpicAccountId_IsValid(id) == EOS_TRUE);
			}
			static const char* epicIdToString(EOS_EpicAccountId id)
			{
				if ( !id )
				{
					return "NULL";
				}
				if ( !epicIdIsValid(id) )
				{
					return "Invalid EOS_EpicAccountId";
				}
				else
				{
					static char buffer[EOS_EPICACCOUNTID_MAX_LENGTH + 1];
					int bufferSize = sizeof(buffer);
					EOS_EResult result = EOS_EpicAccountId_ToString(id, buffer, &bufferSize);
					if ( result == EOS_EResult::EOS_Success )
					{
						return buffer;
					}
					return "Error";
				}
			}
			static bool isMatchingProductIds(std::string str1, std::string str2)
			{
				EOS_ProductUserId id1 = productIdFromString(str1.c_str());
				EOS_ProductUserId id2 = productIdFromString(str2.c_str());
				return isMatchingProductIds(id1, id2);
			}
			static bool isMatchingProductIds(EOS_ProductUserId id1, EOS_ProductUserId id2)
			{
				if ( !id1 || !id2 )
				{
					return false;
				}
				if ( !productIdIsValid(id1) || !productIdIsValid(id2) )
				{
					logError("isMatchingProductIds: mismatch one or more id invalid: %s | %s", productIdToString(id1), productIdToString(id2));
					return false;
				}
				//std::string str1 = productIdToString(id1);
				//std::string str2 = productIdToString(id2);
				if ( id1 == id2 )
				{
					return true;
				}
				//logError("isMatchingProductIds: mismatch: %s | %s", str1.c_str(), str2.c_str());
				return false;
			}
			static EOS_EpicAccountId epicIdFromString(const char* string)
			{
				if ( !string )
				{
					return nullptr;
				}
				return EOS_EpicAccountId_FromString(string);
			}
			static EOS_ProductUserId productIdFromString(const char* string)
			{
				if ( !string )
				{
					return nullptr;
				}
				return EOS_ProductUserId_FromString(string);
			}
	};

	void SetSleepStatus(bool asleep)
	{
#ifdef NINTENDO
		if (!PlatformHandle)
		{
			return;
		}
		static bool oldStatus = false;
		if (oldStatus != asleep) {
			oldStatus = asleep;
			auto status = asleep ? EOS_EApplicationStatus::EOS_AS_BackgroundSuspended : EOS_EApplicationStatus::EOS_AS_Foreground;
			EOS_Platform_SetApplicationStatus(PlatformHandle, status);
		}
#endif
	}

	bool oldNetworkStatus = false;
	void SetNetworkAvailable(bool available)
	{
#ifdef NINTENDO
		if (oldNetworkStatus != available) {
			oldNetworkStatus = available;
			auto status = available ? EOS_ENetworkStatus::EOS_NS_Online : EOS_ENetworkStatus::EOS_NS_Disabled;
			if (PlatformHandle) {
				EOS_Platform_SetNetworkStatus(PlatformHandle, status);
			}
			if (ServerPlatformHandle) {
				EOS_Platform_SetNetworkStatus(ServerPlatformHandle, status);
			}
#ifdef NINTENDO
			if (!available) {
				if (CurrentUserInfo.isValid() && CurrentUserInfo.isLoggedIn()) {
					printlog("[NX] auto-logging out user from EOS!");
					CrossplayAccountManager.logOut = true;
				}
			}
#endif
		}
#endif
	}

	bool HandleReceivedMessages(EOS_ProductUserId* remoteIdReturn);
	bool HandleReceivedMessagesAndIgnore(EOS_ProductUserId* remoteIdReturn); // function to empty the packet queue on main lobby.
	void SendMessageP2P(EOS_ProductUserId RemoteId, const void* data, int len);
	bool serialize(void* file);
	void readFromFile();
	void readFromCmdLineArgs();
	void queryAccountIdFromProductId(LobbyData_t* lobby/*, std::vector<EOS_ProductUserId>& accountsToQuery*/);
	void queryLocalExternalAccountId(EOS_EExternalAccountType accountType);
	void showFriendsOverlay();
	void queryDLCOwnership();
	void unlockAchievement(const char* name);
	void loadAchievementData();
	void ingestStat(int stat_num, int value);
	void ingestGlobalStats();
	void queueGlobalStatUpdate(int stat_num, int value);
	void queryAllStats();
	SteamStat_t* getStatStructFromString(const std::string& str);
	static void logInfo(const char* str, ...)
	{
		char newstr[1024] = { 0 };
		va_list argptr;

		// format the content
		va_start(argptr, str);
		vsnprintf(newstr, 1023, str, argptr);
		va_end(argptr);
		printlog("[EOS Info]: %s", newstr);
	}
	static void logError(const char* str, ...)
	{
		char newstr[1024] = { 0 };
		va_list argptr;

		// format the content
		va_start(argptr, str);
		vsnprintf(newstr, 1023, str, argptr);
		va_end(argptr);
		printlog("[EOS Error]: %s", newstr);
	}
};

extern EOSFuncs EOS;

#endif //USE_EOS
