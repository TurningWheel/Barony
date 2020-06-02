#pragma once

#include "Config.hpp"

#ifdef USE_EOS
#include "eos_sdk.h"
#include "eos_logging.h"
#include "eos_auth.h"
#include "eos_friends.h"
#include "eos_userinfo.h"
#include "eos_lobby.h"
#include "eos_p2p.h"
#include "eos_ui.h"
#include <vector>
#include <iostream>
#include <map>
#include "net.hpp"
#include "stat.hpp"
#include "physfs.h"

class EOSFuncs
{
public:
	std::string ProductId = "";
	std::string SandboxId = "";
	std::string DeploymentId = "";
	std::string ClientCredentialsId = "";
	std::string ClientCredentialsSecret = "";
	std::string CredentialName = "cred1";
	std::string CredentialHost = "localhost:12345";

	const int kMaxLobbiesToSearch = 100;
	EOS_EResult AccountAuthenticationCompleted = EOS_EResult::EOS_NotConfigured;

	// global shenanigans
	bool bRequestingLobbies = false; // client is waiting for lobby data to display
	bool bConnectingToLobby = false; // if true, client is waiting for lobby join callback
	bool bConnectingToLobbyWindow = false; // client has a valid lobby window and has not encountered a new error window
	//bool bStillConnectingToLobby = false; // TODO: client got a lobby invite and booted up the game with this?
	char currentLobbyName[32] = "";

	std::unordered_set<EOS_ProductUserId> ProductIdsAwaitingAccountMappingCallback;
	std::unordered_map<EOS_ProductUserId, EOS_EpicAccountId> AccountMappings;

	// actually all pointers...
	EOS_HPlatform PlatformHandle = nullptr; 
	EOS_HAuth AuthHandle = nullptr;
	EOS_HConnect ConnectHandle = nullptr;
	EOS_HFriends FriendsHandle = nullptr;
	EOS_HUserInfo UserInfoHandle = nullptr;
	EOS_HLobby LobbyHandle = nullptr;
	EOS_HLobbyModification LobbyModificationHandle = nullptr;
	EOS_HLobbyModification LobbyMemberModificationHandle = nullptr;
	EOS_HUI UIHandle = nullptr;

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
		int MaxPlayers = 0;
		std::string LobbyId = "";
		std::string OwnerProductUserId = "";
		bool bLobbyHasFullDetailsRead = false;
		bool bLobbyHasBasicDetailsRead = false;
		bool bAwaitingLeaveCallback = false;
		bool bAwaitingCreationCallback = false;

		class PlayerLobbyData_t {
		public:
			std::string memberProductUserId = "";
			std::string memberEpicAccountId = "";
			std::string name = "";
			int clientNumber = -1;
			bool bUserInfoRequireUpdate = true;
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
			LobbyId = "";
			OwnerProductUserId = "";
			playersInLobby.clear();
			bLobbyHasFullDetailsRead = false;
			bLobbyHasBasicDetailsRead = false;
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
				Uint32 isLobbyLoadingSavedGame = 0;
				Uint32 serverFlags = 0;
				Uint32 numServerMods = 0;
				void ClearData()
				{
					lobbyName = "";
					gameVersion = "";
					isLobbyLoadingSavedGame = 0;
					serverFlags = 0;
					numServerMods = 0;
				}
		} LobbyAttributes;
		bool updateLobbyForHost();
		void getLobbyAttributes(EOS_HLobbyDetails LobbyDetails);
		void getLobbyMemberInfo(EOS_HLobbyDetails LobbyDetails);
		void setLobbyAttributesAfterReading(EOS_Lobby_AttributeData* data);
		void setLobbyAttributesFromGame();
		void setBasicCurrentLobbyDataFromInitialJoin(LobbyData_t* lobbyToJoin);
		void destroyLobby();
		bool currentUserIsOwner();
		void updateLobby();
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
			GAME_MODS
		};
		const int kNumAttributes = 5;
		std::pair<std::string, std::string> getAttributePair(AttributeTypes type);

	} CurrentLobbyData;

	class LobbySearchResults_t {
	public:
		EOS_HLobbySearch CurrentLobbySearch = nullptr;
		std::vector<LobbyData_t> results;
		int selectedLobby = 0;
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
		EOS_ProductUserId getPeerIdFromIndex(int index);
		int getIndexFromPeerId(EOS_ProductUserId id);
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
		void resetPeersAndServerData()
		{
			peerProductIds.clear();
			serverProductId = nullptr;
		}
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

	void shutdown()
	{
		if ( PlatformHandle )
		{
			EOS_Platform_Release(PlatformHandle);
			PlatformHandle = nullptr;
		}
		EOS_EResult result = EOS_Shutdown();
		if ( result != EOS_EResult::EOS_Success )
		{
			logError("Shutdown error! code: %d", static_cast<int>(result));
		}

		logInfo("Shutdown completed.");
	}

	bool initPlatform(bool enableLogging)
	{
		EOS_InitializeOptions InitializeOptions;
		InitializeOptions.ProductName = "Barony";
		InitializeOptions.ProductVersion = "v3.3.3";
		InitializeOptions.ApiVersion = EOS_INITIALIZE_API_LATEST;
		InitializeOptions.AllocateMemoryFunction = nullptr;
		InitializeOptions.ReallocateMemoryFunction = nullptr;
		InitializeOptions.ReleaseMemoryFunction = nullptr;
		InitializeOptions.Reserved = nullptr;
		InitializeOptions.SystemInitializeOptions = nullptr;
		EOS_EResult result = EOS_Initialize(&InitializeOptions);
		if ( result != EOS_EResult::EOS_Success )
		{
			logError("initPlatform: Failure to initialize - error code: %d", static_cast<int>(result));
			return false;
		}
		else
		{
			logInfo("initPlatform: Initialize success");
		}

		if ( enableLogging )
		{
			EOS_EResult SetLogCallbackResult = EOS_Logging_SetCallback(&this->LoggingCallback);
			if ( SetLogCallbackResult != EOS_EResult::EOS_Success )
			{
				logError("SetLogCallbackResult: Set Logging Callback Failed!");
			}
			else
			{
				logInfo("SetLogCallbackResult: Logging Callback set");
				EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_ALL_CATEGORIES, EOS_ELogLevel::EOS_LOG_Info);
			}
		}

		EOS_Platform_Options PlatformOptions = {};
		PlatformOptions.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
		PlatformOptions.Reserved = nullptr;
		PlatformOptions.ProductId = ProductId.c_str();
		PlatformOptions.SandboxId = SandboxId.c_str();
		PlatformOptions.DeploymentId = DeploymentId.c_str();
		PlatformOptions.ClientCredentials.ClientId = ClientCredentialsId.c_str();
		PlatformOptions.ClientCredentials.ClientSecret = ClientCredentialsSecret.c_str();
		PlatformOptions.OverrideCountryCode = nullptr;
		PlatformOptions.OverrideLocaleCode = nullptr;
		PlatformOptions.bIsServer = EOS_FALSE;
		PlatformOptions.Flags = 0;
		static std::string EncryptionKey(64, '1');
		PlatformOptions.EncryptionKey = EncryptionKey.c_str();
		PlatformOptions.CacheDirectory = nullptr; // important - needs double slashes and absolute path

		PlatformHandle = EOS_Platform_Create(&PlatformOptions);
		if ( !PlatformHandle )
		{
			logError("PlatformHandle: Platform failed to initialize - invalid handle");
			return false;
		}
		return true;
	}

	bool initAuth()
	{
		return initAuth(CredentialHost, CredentialName);
	}
	bool initAuth(std::string hostname, std::string tokenName);

	void initConnectLogin()
	{
		ConnectHandle = EOS_Platform_GetConnectInterface(PlatformHandle);

		EOS_Auth_Token* UserAuthToken = nullptr;
		EOS_Auth_CopyUserAuthTokenOptions CopyTokenOptions = { 0 };
		CopyTokenOptions.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

		if ( EOS_Auth_CopyUserAuthToken(AuthHandle, &CopyTokenOptions,
			EOSFuncs::Helpers_t::epicIdFromString(CurrentUserInfo.epicAccountId.c_str()), &UserAuthToken) == EOS_EResult::EOS_Success )
		{
			logInfo("initConnectLogin: Auth expires: %f", UserAuthToken->ExpiresIn);

			EOS_Connect_Credentials Credentials;
			Credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
			Credentials.Token = UserAuthToken->AccessToken;
			Credentials.Type = EOS_EExternalCredentialType::EOS_ECT_EPIC; // change this to steam etc for different account providers.

			EOS_Connect_LoginOptions Options;
			Options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
			Options.Credentials = &Credentials;
			Options.UserLoginInfo = nullptr;

			EOS_Connect_Login(ConnectHandle, &Options, nullptr, ConnectLoginCompleteCallback);
			EOS_Auth_Token_Release(UserAuthToken);
		}
	}

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

	void createLobby()
	{
		/*if ( CurrentLobbyData.currentLobbyIsValid() )
		{
			logInfo("");
			return;
		}*/
		
		if ( CurrentLobbyData.bAwaitingLeaveCallback )
		{
			logInfo("createLobby: CurrentLobbyData.bAwaitingLeaveCallback is true");
		}
		if ( CurrentLobbyData.bAwaitingCreationCallback )
		{
			logInfo("createLobby: CurrentLobbyData.bAwaitingCreationCallback is true");
		}

		CurrentLobbyData.bAwaitingCreationCallback = true;

		LobbyHandle = EOS_Platform_GetLobbyInterface(PlatformHandle);
		EOS_Lobby_CreateLobbyOptions CreateOptions;
		CreateOptions.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
		CreateOptions.LocalUserId = CurrentUserInfo.getProductUserIdHandle();
		CreateOptions.MaxLobbyMembers = 4;
		CreateOptions.PermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED;

		EOS_Lobby_CreateLobby(LobbyHandle, &CreateOptions, nullptr, OnCreateLobbyFinished);
		CurrentLobbyData.MaxPlayers = CreateOptions.MaxLobbyMembers;
		CurrentLobbyData.OwnerProductUserId = CurrentUserInfo.getProductUserIdStr();
	}

	void joinLobby(LobbyData_t* lobby)
	{
		LobbyHandle = EOS_Platform_GetLobbyInterface(PlatformHandle);

		if ( CurrentLobbyData.currentLobbyIsValid() )
		{
			if ( CurrentLobbyData.LobbyId.compare(lobby->LobbyId) == 0 )
			{
				logInfo("joinLobby: attempting to join current lobby");
				return;
			}
			if ( CurrentLobbyData.bAwaitingLeaveCallback )
			{
				logInfo("joinLobby: CurrentLobbyData.bAwaitingLeaveCallback is true");
			}
			else
			{
				leaveLobby();
				logInfo("joinLobby: leaving current lobby id: %s", CurrentLobbyData.LobbyId.c_str());
			}
		}
		CurrentLobbyData.ClearData();
		CurrentLobbyData.setBasicCurrentLobbyDataFromInitialJoin(lobby);

		EOS_Lobby_JoinLobbyOptions JoinOptions;
		JoinOptions.ApiVersion = EOS_LOBBY_JOINLOBBY_API_LATEST;
		JoinOptions.LocalUserId = CurrentUserInfo.getProductUserIdHandle();
		JoinOptions.LobbyDetailsHandle = LobbyParameters.lobbyToJoin;
		EOS_Lobby_JoinLobby(LobbyHandle, &JoinOptions, nullptr, OnLobbyJoinCallback);

		LobbyParameters.clearLobbyToJoin();
	}

	void leaveLobby()
	{
		//if ( CurrentLobbyData.bAwaitingLeaveCallback )
		//{
		//	// no action needed
		//	logInfo("leaveLobby: attempting to leave lobby with callback already requested, ignoring");
		//	return;
		//}

		// attempt to destroy the lobby if leaving and we are the owner.
		if ( CurrentLobbyData.currentUserIsOwner() )
		{
			CurrentLobbyData.destroyLobby();
			return;
		}

		LobbyHandle = EOS_Platform_GetLobbyInterface(PlatformHandle);

		EOS_Lobby_LeaveLobbyOptions LeaveOptions;
		LeaveOptions.ApiVersion = EOS_LOBBY_LEAVELOBBY_API_LATEST;
		LeaveOptions.LocalUserId = CurrentUserInfo.getProductUserIdHandle();
		LeaveOptions.LobbyId = CurrentLobbyData.LobbyId.c_str();

		CurrentLobbyData.bAwaitingLeaveCallback = true;
		EOS_Lobby_LeaveLobby(LobbyHandle, &LeaveOptions, nullptr, OnLobbyLeaveCallback);

	}

	void searchLobbies(LobbyParameters_t::LobbySearchOptions searchType,
		LobbyParameters_t::LobbyJoinOptions joinOptions, EOS_LobbyId lobbyIdToSearch)
	{
		LobbyHandle = EOS_Platform_GetLobbyInterface(PlatformHandle);
		logInfo("searchLobbies: starting search");
		EOS_Lobby_CreateLobbySearchOptions CreateSearchOptions;
		CreateSearchOptions.ApiVersion = EOS_LOBBY_CREATELOBBYSEARCH_API_LATEST;
		CreateSearchOptions.MaxResults = kMaxLobbiesToSearch;

		EOS_HLobbySearch LobbySearch = nullptr;
		if ( LobbySearchResults.CurrentLobbySearch != nullptr )
		{
			EOS_LobbySearch_Release(LobbySearchResults.CurrentLobbySearch);
			LobbySearchResults.CurrentLobbySearch = nullptr;
		}

		EOS_EResult result = EOS_Lobby_CreateLobbySearch(LobbyHandle, &CreateSearchOptions, &LobbySearch);
		if ( result != EOS_EResult::EOS_Success )
		{
			logError("searchLobbies: EOS_Lobby_CreateLobbySearch failure: %d", static_cast<int>(result));
			return;
		}
		LobbySearchResults.CurrentLobbySearch = LobbySearch;
		for ( auto& result : LobbySearchResults.results )
		{
			result.ClearData();
		}
		LobbySearchResults.results.clear();

		/*EOS_LobbySearch_SetTargetUserIdOptions SetLobbyOptions = {};
		SetLobbyOptions.ApiVersion = EOS_LOBBYSEARCH_SETLOBBYID_API_LATEST;
		SetLobbyOptions.TargetUserId = CurrentUserInfo.Friends.at(0).UserId;
		Result = EOS_LobbySearch_SetTargetUserId(LobbySearch, &SetLobbyOptions);*/

		if ( searchType == LobbyParameters_t::LOBBY_SEARCH_BY_LOBBYID )
		{
			// appends criteria to search for within the normal search function
			EOS_LobbySearch_SetLobbyIdOptions SetLobbyOptions = {};
			SetLobbyOptions.ApiVersion = EOS_LOBBYSEARCH_SETLOBBYID_API_LATEST;
			SetLobbyOptions.LobbyId = lobbyIdToSearch;
			EOS_LobbySearch_SetLobbyId(LobbySearch, &SetLobbyOptions);
		}

		EOS_LobbySearch_FindOptions FindOptions;
		FindOptions.ApiVersion = EOS_LOBBYSEARCH_FIND_API_LATEST;
		FindOptions.LocalUserId = CurrentUserInfo.getProductUserIdHandle();

		LobbyParameters.clearLobbySearchOptions();
		LobbyParameters.setLobbySearchOptions(searchType, joinOptions);
		EOS_LobbySearch_Find(LobbySearch, &FindOptions, LobbyParameters.lobbySearchOptions, OnLobbySearchFinished);
	}

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

			EOS_P2P_SocketId SocketId;
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
					static char buffer[EOS_PRODUCTUSERID_MAX_LENGTH];
					int bufferSize = sizeof(buffer);
					EOS_EResult result = EOS_ProductUserId_ToString(id, buffer, &bufferSize);
					if ( result == EOS_EResult::EOS_Success )
					{
						return buffer;
					}
					return "Error";
				}
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
					static char buffer[EOS_EPICACCOUNTID_MAX_LENGTH];
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

	bool HandleReceivedMessages(EOS_ProductUserId* remoteIdReturn);
	void SendMessageP2P(EOS_ProductUserId RemoteId, const void* data, int len);
	void serialize(void* file);
	void readFromFile();
	void queryAccountIdFromProductId(LobbyData_t* lobby/*, std::vector<EOS_ProductUserId>& accountsToQuery*/);
	void showFriendsOverlay();
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
