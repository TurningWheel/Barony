
#include "Config.hpp"

#ifdef USE_EOS

#ifdef WINDOWS
#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)
#define BUILD_ENV_PR STRINGIZE(BUILD_PR)
#define BUILD_ENV_SA STRINGIZE(BUILD_SA)
#define BUILD_ENV_DE STRINGIZE(BUILD_DE)
#define BUILD_ENV_CC STRINGIZE(BUILD_CC)
#define BUILD_ENV_CS STRINGIZE(BUILD_CS)
#define BUILD_ENV_GSE STRINGIZE(BUILD_GSE)
#endif

#include <cassert>
#include "main.hpp"
#include "menu.hpp"
#include "game.hpp"
#include "eos.hpp"
#include "json.hpp"
#include "scores.hpp"
#include "files.hpp"
#include "draw.hpp"
#include "interface/interface.hpp"
#include "interface/ui.hpp"
#include "lobbies.hpp"
#include "prng.hpp"
#include "ui/MainMenu.hpp"
#include "mod_tools.hpp"

EOSFuncs EOS;

void LobbyLeaveCleanup(EOSFuncs::LobbyData_t& lobby)
{
	EOS.P2PConnectionInfo.resetPeersAndServerData();
	lobby.bAwaitingLeaveCallback = false;
	lobby.UnsubscribeFromLobbyUpdates();
	lobby.ClearData();
}

void EOS_CALL EOSFuncs::LoggingCallback(const EOS_LogMessage* log)
{
	if ( !strcmp(log->Category, "LogHttp") )
	{
		return;
	}
	printlog("[EOS Logging]: %s:%s", log->Category, log->Message);
}

std::string EOSFuncs::getAuthToken()
{
	if ( AccountManager.authTokenRefresh > 0 )
	{
		logInfo("Recieved cached auth token");
		return AccountManager.authToken;
	}

	if ( !AuthHandle
		|| AccountManager.AccountAuthenticationStatus != EOS_EResult::EOS_Success )
	{
		//logInfo("Recieved cached auth token, unknown login status");
		return AccountManager.authToken;
	}

	EOS_Auth_Token* UserAuthToken = nullptr;
	EOS_Auth_CopyUserAuthTokenOptions CopyTokenOptions = { 0 };
	CopyTokenOptions.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;
	if ( EOS_Auth_CopyUserAuthToken(AuthHandle, &CopyTokenOptions,
		EOSFuncs::Helpers_t::epicIdFromString(CurrentUserInfo.epicAccountId.c_str()), &UserAuthToken) == EOS_EResult::EOS_Success )
	{
		std::string str = UserAuthToken->AccessToken;
		EOS_Auth_Token_Release(UserAuthToken);

		AccountManager.authToken = str;
		AccountManager.authTokenRefresh = TICKS_PER_SECOND * 60 * 60 * 5; // x hour refresh
		logInfo("Generated auth token");
		return str;
	}
	logInfo("Unable to generate auth token");
	return AccountManager.authToken;
}

void EOS_CALL EOSFuncs::AuthLoginCompleteCallback(const EOS_Auth_LoginCallbackInfo* data)
{
	if (!EOS.PlatformHandle)
	{
		return;
	}
	EOS_HAuth AuthHandle = EOS_Platform_GetAuthInterface(EOS.PlatformHandle);
	EOS.AccountManager.waitingForCallback = false;
	if (!data)
	{
		EOSFuncs::logError("Login Callback error: null data");
	}
	else if (data->ResultCode == EOS_EResult::EOS_Success)
	{
		EOSFuncs::logInfo("Login Callback: success");
		EOS.AccountManager.AccountAuthenticationStatus = EOS_EResult::EOS_Success;

		const int numAccounts = EOS_Auth_GetLoggedInAccountsCount(AuthHandle);
		for (int accIndex = 0; accIndex < numAccounts; ++accIndex)
		{
			EOS.CurrentUserInfo.epicAccountId = EOSFuncs::Helpers_t::epicIdToString(EOS_Auth_GetLoggedInAccountByIndex(AuthHandle, accIndex));
			EOS_ELoginStatus LoginStatus;
			LoginStatus = EOS_Auth_GetLoginStatus(AuthHandle, data->LocalUserId);

			EOSFuncs::logInfo("Account index: %d Status: %d UserID: %s", accIndex,
				static_cast<int>(LoginStatus), EOS.CurrentUserInfo.epicAccountId.c_str());

			EOS.getUserInfo(EOSFuncs::Helpers_t::epicIdFromString(EOS.CurrentUserInfo.epicAccountId.c_str()),
				UserInfoQueryType::USER_INFO_QUERY_LOCAL, accIndex);
			EOS.initConnectLogin();
			EOS.queryDLCOwnership();
			EOS.getAuthToken();
		}
		return;
	}
	else if (data->ResultCode == EOS_EResult::EOS_OperationWillRetry)
	{
		EOSFuncs::logError("Login Callback: retrying");
		return;
	}
	else if (data->ResultCode == EOS_EResult::EOS_Auth_PinGrantCode)
	{
		EOSFuncs::logError("Login Callback: PIN required");
	}
	else if (data->ResultCode == EOS_EResult::EOS_Auth_MFARequired)
	{
		EOSFuncs::logError("Login Callback: MFA required");
	}
#ifdef NINTENDO
	else if (data->ResultCode == EOS_EResult::EOS_InvalidUser)
	{
		// need to sign in with Nintendo Account and relogin
	}
#endif
	else
	{
		EOSFuncs::logError("Login Callback: General fail: %d", static_cast<int>(data->ResultCode));
		EOS.AccountManager.AccountAuthenticationStatus = data->ResultCode;
		return;
	}
	EOS.AccountManager.AccountAuthenticationStatus = EOS_EResult::EOS_InvalidAuth;
}

void EOS_CALL EOSFuncs::ConnectLoginCompleteCallback(const EOS_Connect_LoginCallbackInfo* data)
{
	if (!EOS.PlatformHandle)
	{
		return;
	}

	if (!data)
	{
		EOSFuncs::logError("Connect Login Callback: null data");
		EOS.CurrentUserInfo.setProductUserIdHandle(nullptr);
		EOS.CurrentUserInfo.bUserLoggedIn = false;
		return;
	}

	if (data->ResultCode == EOS_EResult::EOS_Success)
	{
		EOS.CurrentUserInfo.setProductUserIdHandle(data->LocalUserId);
		EOS.CurrentUserInfo.bUserLoggedIn = true;
		EOS.SubscribeToConnectionRequests();
		EOSFuncs::logInfo("Connect Login Callback success: %s", EOS.CurrentUserInfo.getProductUserIdStr());

		// load achievement data
#ifndef LOCAL_ACHIEVEMENTS
		if (!EOS.Achievements.bAchievementsInit)
		{
			EOS.loadAchievementData();
		}
#endif

		// cache friend data
		EOS.queryFriends();
	}
	else if (data->ResultCode == EOS_EResult::EOS_InvalidUser)
	{
		EOSFuncs::logInfo("Connect Login Callback: creating new user");
		EOS_Connect_CreateUserOptions CreateUserOptions{};
		CreateUserOptions.ApiVersion = EOS_CONNECT_CREATEUSER_API_LATEST;
		CreateUserOptions.ContinuanceToken = data->ContinuanceToken;

		EOS.ConnectHandle = EOS_Platform_GetConnectInterface(EOS.PlatformHandle);
		EOS_Connect_CreateUser(EOS.ConnectHandle, &CreateUserOptions, nullptr, OnCreateUserCallback);
	}
	else
	{
		EOSFuncs::logError("Connect Login Callback: General fail: %d", static_cast<int>(data->ResultCode));
		EOS.CurrentUserInfo.setProductUserIdHandle(nullptr);
		EOS.CurrentUserInfo.bUserLoggedIn = false;
	}
}

void EOS_CALL EOSFuncs::ConnectLoginCrossplayCompleteCallback(const EOS_Connect_LoginCallbackInfo* data)
{
	EOS.CrossplayAccountManager.awaitingConnectCallback = false;

	if (!EOS.PlatformHandle)
	{
		return;
	}

	if (!data)
	{
		EOSFuncs::logError("Crossplay Connect Login Callback: null data");
		EOS.CurrentUserInfo.setProductUserIdHandle(nullptr);
		EOS.CurrentUserInfo.bUserLoggedIn = false;
		return;
	}

	EOS.CrossplayAccountManager.connectLoginStatus = data->ResultCode;

	if (data->ResultCode == EOS_EResult::EOS_Success)
	{
		EOS.CurrentUserInfo.setProductUserIdHandle(data->LocalUserId);
		EOS.CurrentUserInfo.bUserLoggedIn = true;
		EOS.SubscribeToConnectionRequests();
		EOS.AddConnectAuthExpirationNotification();
#if defined(STEAMWORKS) || defined(NINTENDO)
		EOS_ELoginStatus authLoginStatus = EOS_Auth_GetLoginStatus(EOS_Platform_GetAuthInterface(EOS.PlatformHandle),
			EOSFuncs::Helpers_t::epicIdFromString(EOS.CurrentUserInfo.epicAccountId.c_str()));
		if (authLoginStatus != EOS_ELoginStatus::EOS_LS_LoggedIn)
		{
			EOS.queryLocalExternalAccountId(EOS_EExternalAccountType::EOS_EAT_STEAM);
		}
		EOS.StatGlobalManager.queryGlobalStatUser();
#endif
#ifdef NINTENDO
		// load achievement data
#ifndef LOCAL_ACHIEVEMENTS
		if (!EOS.Achievements.bAchievementsInit)
		{
			EOS.loadAchievementData();
		}
#endif
#endif
		EOSFuncs::logInfo("Crossplay Connect Login Callback success: %s", EOS.CurrentUserInfo.getProductUserIdStr());
	}
	else if (data->ResultCode == EOS_EResult::EOS_InvalidUser)
	{
		if (EOS.CrossplayAccountManager.acceptedEula)
		{
			EOS_Connect_CreateUserOptions CreateUserOptions{};
			CreateUserOptions.ApiVersion = EOS_CONNECT_CREATEUSER_API_LATEST;
			CreateUserOptions.ContinuanceToken = data->ContinuanceToken;

			EOS.CrossplayAccountManager.awaitingCreateUserCallback = true;
			EOS.ConnectHandle = EOS_Platform_GetConnectInterface(EOS.PlatformHandle);
			EOS_Connect_CreateUser(EOS.ConnectHandle, &CreateUserOptions, nullptr, OnCreateUserCrossplayCallback);
		}
		else
		{
			EOS.CrossplayAccountManager.continuanceToken = data->ContinuanceToken;
		}
	}
	else
	{
		EOSFuncs::logError("Crossplay Connect Login Callback: General fail: %d", static_cast<int>(data->ResultCode));
		EOS.CurrentUserInfo.setProductUserIdHandle(nullptr);
		EOS.CurrentUserInfo.bUserLoggedIn = false;
	}
}

void EOS_CALL EOSFuncs::OnCreateUserCallback(const EOS_Connect_CreateUserCallbackInfo* data)
{
	if (!data)
	{
		EOSFuncs::logError("OnCreateUserCallback: null data");
		EOS.CurrentUserInfo.setProductUserIdHandle(nullptr);
		EOS.CurrentUserInfo.bUserLoggedIn = false;
		return;
	}
	if (data->ResultCode == EOS_EResult::EOS_Success)
	{
		EOS.CurrentUserInfo.setProductUserIdHandle(data->LocalUserId);
		EOS.CurrentUserInfo.bUserLoggedIn = true;
		EOS.SubscribeToConnectionRequests();
		EOSFuncs::logInfo("OnCreateUserCallback success, new user: %s", EOS.CurrentUserInfo.getProductUserIdStr());

		// load achievement data
#ifndef LOCAL_ACHIEVEMENTS
		if (!EOS.Achievements.bAchievementsInit)
		{
			EOS.loadAchievementData();
		}
#endif

		// cache friend data
		EOS.queryFriends();
	}
	else
	{
		EOSFuncs::logError("OnCreateUserCallback: General fail: %d", static_cast<int>(data->ResultCode));
		EOS.CurrentUserInfo.setProductUserIdHandle(nullptr);
		EOS.CurrentUserInfo.bUserLoggedIn = false;
	}
}

void EOS_CALL EOSFuncs::OnCreateUserCrossplayCallback(const EOS_Connect_CreateUserCallbackInfo* data)
{
	if (!EOS.PlatformHandle)
	{
		return;
	}

	EOS.CrossplayAccountManager.awaitingCreateUserCallback = false;
	EOS.CrossplayAccountManager.connectLoginStatus = EOS_EResult::EOS_NotConfigured;
	if (!data)
	{
		EOSFuncs::logError("OnCreateUserCrossplayCallback: null data");
		EOS.CurrentUserInfo.setProductUserIdHandle(nullptr);
		EOS.CurrentUserInfo.bUserLoggedIn = false;
		return;
	}
	if (data->ResultCode == EOS_EResult::EOS_Success)
	{
		EOS.CurrentUserInfo.setProductUserIdHandle(data->LocalUserId);
		EOS.CurrentUserInfo.bUserLoggedIn = true;
		EOS.SubscribeToConnectionRequests();
		EOS.AddConnectAuthExpirationNotification();
#ifdef STEAMWORKS
		EOS.CrossplayAccountManager.connectLoginStatus = EOS_EResult::EOS_Success;
		EOS_ELoginStatus authLoginStatus = EOS_Auth_GetLoginStatus(EOS_Platform_GetAuthInterface(EOS.PlatformHandle),
			EOSFuncs::Helpers_t::epicIdFromString(EOS.CurrentUserInfo.epicAccountId.c_str()));
		if (authLoginStatus != EOS_ELoginStatus::EOS_LS_LoggedIn)
		{
			EOS.queryLocalExternalAccountId(EOS_EExternalAccountType::EOS_EAT_STEAM);
		}
#endif
		EOSFuncs::logInfo("OnCreateUserCrossplayCallback success, new user: %s", EOS.CurrentUserInfo.getProductUserIdStr());
	}
	else
	{
		EOS.CrossplayAccountManager.connectLoginStatus = data->ResultCode;
		EOSFuncs::logError("OnCreateUserCrossplayCallback: General fail: %d", static_cast<int>(data->ResultCode));
		EOS.CurrentUserInfo.setProductUserIdHandle(nullptr);
		EOS.CurrentUserInfo.bUserLoggedIn = false;
	}
}

void EOS_CALL EOSFuncs::FriendsQueryCallback(const EOS_Friends_QueryFriendsCallbackInfo* data)
{
	if (!data)
	{
		EOSFuncs::logError("FriendsQueryCallback: null data");
		return;
	}
	if (!EOS.PlatformHandle)
	{
		return;
	}
	EOS_HFriends FriendsHandle = EOS_Platform_GetFriendsInterface(EOS.PlatformHandle);
	EOS_Friends_GetFriendsCountOptions FriendsCountOptions{};
	FriendsCountOptions.ApiVersion = EOS_FRIENDS_GETFRIENDSCOUNT_API_LATEST;
	FriendsCountOptions.LocalUserId = data->LocalUserId;
	int numFriends = EOS_Friends_GetFriendsCount(FriendsHandle, &FriendsCountOptions);
	EOSFuncs::logInfo("FriendsQueryCallback: Friends num: %d", numFriends);

	EOS.CurrentUserInfo.Friends.clear();

	EOS_Friends_GetFriendAtIndexOptions IndexOptions{};
	IndexOptions.ApiVersion = EOS_FRIENDS_GETFRIENDATINDEX_API_LATEST;
	IndexOptions.LocalUserId = data->LocalUserId;
	for (int i = 0; i < numFriends; ++i)
	{
		IndexOptions.Index = i;
		EOS_EpicAccountId FriendUserId = EOS_Friends_GetFriendAtIndex(FriendsHandle, &IndexOptions);

		if (EOSFuncs::Helpers_t::epicIdIsValid(FriendUserId))
		{
			EOS_Friends_GetStatusOptions StatusOptions{};
			StatusOptions.ApiVersion = EOS_FRIENDS_GETSTATUS_API_LATEST;
			StatusOptions.LocalUserId = data->LocalUserId;
			StatusOptions.TargetUserId = FriendUserId;
			EOS_EFriendsStatus FriendStatus = EOS_Friends_GetStatus(FriendsHandle, &StatusOptions);

			EOSFuncs::logInfo("FriendsQueryCallback: FriendStatus: Id: %s Status: %d", EOSFuncs::Helpers_t::epicIdToString(FriendUserId), static_cast<int>(FriendStatus));

			FriendInfo_t newFriend(FriendStatus, EOSFuncs::Helpers_t::epicIdToString(FriendUserId));
			EOS.CurrentUserInfo.Friends.push_back(newFriend);
			EOS.getUserInfo(FriendUserId, EOS.USER_INFO_QUERY_FRIEND, i);
		}
		else
		{
			EOSFuncs::logError("FriendsQueryCallback: Friend ID was invalid!");
		}
	}
}

void EOS_CALL EOSFuncs::UserInfoCallback(const EOS_UserInfo_QueryUserInfoCallbackInfo* data)
{
	if (!data)
	{
		EOSFuncs::logError("UserInfoCallback: null data");
		return;
	}
	if (!EOS.PlatformHandle)
	{
		return;
	}
	UserInfoQueryData_t* userInfoQueryData = (static_cast<UserInfoQueryData_t*>(data->ClientData));
	if (data->ResultCode == EOS_EResult::EOS_Success)
	{
		EOS_UserInfo_CopyUserInfoOptions UserInfoOptions{};
		UserInfoOptions.ApiVersion = EOS_USERINFO_COPYUSERINFO_API_LATEST;
		UserInfoOptions.LocalUserId = data->LocalUserId;
		UserInfoOptions.TargetUserId = data->TargetUserId;
		EOS_UserInfo* userInfo; // result is managed by application to free the memory.
		if (EOS_UserInfo_CopyUserInfo(EOS_Platform_GetUserInfoInterface(EOS.PlatformHandle),
			&UserInfoOptions, &userInfo) == EOS_EResult::EOS_Success)
		{
			if (userInfoQueryData->queryType == EOS.USER_INFO_QUERY_LOCAL)
			{
				// local user
				EOS.CurrentUserInfo.Name = userInfo->DisplayName;
				EOS.CurrentUserInfo.bUserInfoRequireUpdate = false;
				EOSFuncs::logInfo("UserInfoCallback: Current User Name: %s", userInfo->DisplayName);
			}
			else if (userInfoQueryData->queryType == EOS.USER_INFO_QUERY_FRIEND)
			{
				if (EOS.CurrentUserInfo.Friends.empty())
				{
					EOSFuncs::logInfo("UserInfoCallback: friend info request failed due empty friends list");
				}
				else
				{
					bool foundFriend = false;
					std::string queryTarget = EOSFuncs::Helpers_t::epicIdToString(userInfoQueryData->epicAccountId);
					for (auto& it : EOS.CurrentUserInfo.Friends)
					{
						if (it.EpicAccountId.compare(queryTarget) == 0)
						{
							foundFriend = true;
							it.Name = userInfo->DisplayName;
							it.bUserInfoRequireUpdate = false;
							EOSFuncs::logInfo("UserInfoCallback: found friend username: %s", userInfo->DisplayName);
							break;
						}
					}
					if (!foundFriend)
					{
						EOSFuncs::logInfo("UserInfoCallback: could not find player in current lobby with account %s",
							EOSFuncs::Helpers_t::epicIdToString(userInfoQueryData->epicAccountId));
					}
				}
			}
			else if (userInfoQueryData->queryType == EOS.USER_INFO_QUERY_LOBBY_MEMBER)
			{
				if (!EOS.CurrentLobbyData.currentLobbyIsValid() || EOS.CurrentLobbyData.playersInLobby.empty())
				{
					EOSFuncs::logInfo("UserInfoCallback: lobby member request failed due to invalid or no player data in lobby");
				}
				else
				{
					bool foundMember = false;
					std::string queryTarget = EOSFuncs::Helpers_t::epicIdToString(userInfoQueryData->epicAccountId);
					for (auto& it : EOS.CurrentLobbyData.playersInLobby)
					{
						if (queryTarget.compare(it.memberEpicAccountId.c_str()) == 0)
						{
							foundMember = true;
							it.name = userInfo->DisplayName;
							it.bUserInfoRequireUpdate = false;
							EOSFuncs::logInfo("UserInfoCallback: found lobby username: %s", userInfo->DisplayName);
							break;
						}
					}
					if (!foundMember)
					{
						EOSFuncs::logInfo("UserInfoCallback: could not find player in current lobby with account %s",
							EOSFuncs::Helpers_t::epicIdToString(userInfoQueryData->epicAccountId));
					}
				}
			}
			EOS_UserInfo_Release(userInfo);
		}
		else
		{
			EOSFuncs::logError("UserInfoCallback: Error copying user info");
		}
	}
	delete userInfoQueryData;
}

void EOS_CALL EOSFuncs::OnCreateLobbyFinished(const EOS_Lobby_CreateLobbyCallbackInfo* data)
{
	EOS.CurrentLobbyData.bAwaitingCreationCallback = false;
	if (!data)
	{
		EOSFuncs::logError("OnCreateLobbyFinished: null data");
		return;
	}

	EOS.CurrentLobbyData.LobbyCreationResult = data->ResultCode;
	if (data->ResultCode == EOS_EResult::EOS_Success)
	{
		EOS.CurrentLobbyData.LobbyId = data->LobbyId;

#ifdef NINTENDO
		EOS.CurrentLobbyData.LobbyAttributes.lobbyName = MainMenu::getHostname();
#else
		EOS.CurrentLobbyData.LobbyAttributes.lobbyName = EOS.CurrentUserInfo.Name + "'s lobby";
#endif
		strncpy(EOS.currentLobbyName, EOS.CurrentLobbyData.LobbyAttributes.lobbyName.c_str(), 31);

		Uint32 keygen = local_rng.uniform(0, (36 * 36 * 36 * 36) - 1); // limit of 'zzzz' as base-36 string
		EOS.CurrentLobbyData.LobbyAttributes.gameJoinKey = EOS.getLobbyCodeFromGameKey(keygen);
		std::chrono::system_clock::duration epochDuration = std::chrono::system_clock::now().time_since_epoch();
		EOS.CurrentLobbyData.LobbyAttributes.lobbyCreationTime = std::chrono::duration_cast<std::chrono::seconds>(epochDuration).count();

		EOSFuncs::logInfo("OnCreateLobbyFinished: Generated game code %s", EOS.CurrentLobbyData.LobbyAttributes.gameJoinKey.c_str());

		EOS.CurrentLobbyData.updateLobbyForHost(EOSFuncs::LobbyData_t::HostUpdateLobbyTypes::LOBBY_UPDATE_MAIN_MENU);
		EOS.CurrentLobbyData.SubscribeToLobbyUpdates();
	}
	else
	{
		EOSFuncs::logError("OnCreateLobbyFinished: Callback failure: %d", static_cast<int>(data->ResultCode));
	}
}

void EOS_CALL EOSFuncs::OnLobbySearchFinished(const EOS_LobbySearch_FindCallbackInfo* data)
{
	EOS.bRequestingLobbies = false;
	if (!data)
	{
		EOSFuncs::logError("OnLobbySearchFinished: null data");
		return;
	}
	else if (data->ResultCode == EOS_EResult::EOS_Success)
	{
		EOS_LobbySearch_GetSearchResultCountOptions SearchResultOptions{};
		SearchResultOptions.ApiVersion = EOS_LOBBYSEARCH_GETSEARCHRESULTCOUNT_API_LATEST;
		int NumSearchResults = EOS_LobbySearch_GetSearchResultCount(EOS.LobbySearchResults.CurrentLobbySearch, &SearchResultOptions);
		int* searchOptions = static_cast<int*>(data->ClientData);

		EOS_LobbySearch_CopySearchResultByIndexOptions IndexOptions{};
		IndexOptions.ApiVersion = EOS_LOBBYSEARCH_COPYSEARCHRESULTBYINDEX_API_LATEST;
		for (int i = 0; i < NumSearchResults; ++i)
		{
			LobbyData_t newLobby;
			EOS_HLobbyDetails LobbyDetails = nullptr;
			IndexOptions.LobbyIndex = i;
			EOS_EResult Result = EOS_LobbySearch_CopySearchResultByIndex(EOS.LobbySearchResults.CurrentLobbySearch,
				&IndexOptions, &LobbyDetails);
			if (Result == EOS_EResult::EOS_Success && LobbyDetails)
			{
				EOS.setLobbyDetailsFromHandle(LobbyDetails, &newLobby);
				EOSFuncs::logInfo("OnLobbySearchFinished: Found lobby: %s, Owner: %s, MaxPlayers: %d",
					newLobby.LobbyId.c_str(), newLobby.OwnerProductUserId.c_str(), newLobby.MaxPlayers);

				EOS.LobbySearchResults.results.push_back(newLobby);

				if (searchOptions[EOSFuncs::LobbyParameters_t::JOIN_OPTIONS]
					== static_cast<int>(EOSFuncs::LobbyParameters_t::LOBBY_JOIN_FIRST_SEARCH_RESULT))
				{
					// set the handle to be used for joining.
					EOS.LobbyParameters.lobbyToJoin = LobbyDetails;
					EOS.joinLobby(&newLobby);
				}
				else if (searchOptions[EOSFuncs::LobbyParameters_t::JOIN_OPTIONS]
					== static_cast<int>(EOSFuncs::LobbyParameters_t::LOBBY_DONT_JOIN))
				{
					// we can release this handle.
					EOS_LobbyDetails_Release(LobbyDetails);
				}
				else if (searchOptions[EOSFuncs::LobbyParameters_t::JOIN_OPTIONS]
					== static_cast<int>(EOSFuncs::LobbyParameters_t::LOBBY_UPDATE_CURRENTLOBBY))
				{
					// TODO need?
					EOS.setLobbyDetailsFromHandle(LobbyDetails, &EOS.CurrentLobbyData);

					// we can release this handle.
					EOS_LobbyDetails_Release(LobbyDetails);
				}
			}
			else
			{
				EOS_LobbyDetails_Release(LobbyDetails);
			}
		}

		if (NumSearchResults == 0)
		{
			EOSFuncs::logInfo("OnLobbySearchFinished: Found 0 lobbies!");

			int* searchOptions = static_cast<int*>(data->ClientData);
			if (searchOptions[EOSFuncs::LobbyParameters_t::JOIN_OPTIONS]
				== static_cast<int>(EOSFuncs::LobbyParameters_t::LOBBY_JOIN_FIRST_SEARCH_RESULT))
			{
				// we were trying to join a lobby, set error message.
				EOS.bConnectingToLobbyWindow = false;
				EOS.bConnectingToLobby = false;
				EOS.ConnectingToLobbyStatus = static_cast<int>(EOS_EResult::EOS_NoChange);
				multiplayer = SINGLE;
			}
		}
		EOS.LobbySearchResults.sortResults();
		return;
	}
	else if (data->ResultCode == EOS_EResult::EOS_NotFound)
	{
		EOSFuncs::logError("OnLobbySearchFinished: Requested lobby no longer exists", static_cast<int>(data->ResultCode));
	}
	else
	{
		EOSFuncs::logError("OnLobbySearchFinished: Callback failure: %d", static_cast<int>(data->ResultCode));
	}

	if (data->ResultCode != EOS_EResult::EOS_OperationWillRetry)
	{
		int* searchOptions = static_cast<int*>(data->ClientData);
		if (searchOptions[EOSFuncs::LobbyParameters_t::JOIN_OPTIONS]
			== static_cast<int>(EOSFuncs::LobbyParameters_t::LOBBY_JOIN_FIRST_SEARCH_RESULT))
		{
			// we were trying to join a lobby, set error message.
			EOS.bConnectingToLobbyWindow = false;
			EOS.bConnectingToLobby = false;
			EOS.ConnectingToLobbyStatus = static_cast<int>(data->ResultCode);
			multiplayer = SINGLE;
		}
	}
}

void EOS_CALL EOSFuncs::OnLobbyJoinCallback(const EOS_Lobby_JoinLobbyCallbackInfo* data)
{
	if (!data)
	{
		EOSFuncs::logError("OnLobbyJoinCallback: null data");
		EOS.bConnectingToLobby = false;
		EOS.bConnectingToLobbyWindow = false;
		EOS.ConnectingToLobbyStatus = static_cast<int>(EOS_EResult::EOS_UnexpectedError);
	}
	else if (data->ResultCode == EOS_EResult::EOS_Success)
	{
		EOSFuncs::logInfo("OnLobbyJoinCallback: Joined lobby id: %s", data->LobbyId);
		/*if ( static_cast<LobbyData_t*>(data->ClientData) == &EOS.CurrentLobbyData )
		{
			EOS.CurrentLobbyData.LobbyId = data->LobbyId;
		}*/
		EOS.bConnectingToLobby = false;

		if (EOS.CurrentLobbyData.bDenyLobbyJoinEvent && EOS.CurrentLobbyData.LobbyId.compare(data->LobbyId) == 0)
		{
			// early return, immediately exit lobby.
			EOS.CurrentLobbyData.bDenyLobbyJoinEvent = false;
			EOS.leaveLobby();
			logInfo("OnLobbyJoinCallback: forcibly denying joining current lobby id: %s", EOS.CurrentLobbyData.LobbyId.c_str());
			return;
		}

		EOS.ConnectingToLobbyStatus = static_cast<int>(data->ResultCode);
		EOS.searchLobbies(EOSFuncs::LobbyParameters_t::LobbySearchOptions::LOBBY_SEARCH_BY_LOBBYID,
			EOSFuncs::LobbyParameters_t::LobbyJoinOptions::LOBBY_UPDATE_CURRENTLOBBY, data->LobbyId);

		EOS.P2PConnectionInfo.serverProductId = EOSFuncs::Helpers_t::productIdFromString(EOS.CurrentLobbyData.OwnerProductUserId.c_str());
		EOS.CurrentLobbyData.SubscribeToLobbyUpdates();
		EOS.P2PConnectionInfo.peerProductIds.clear();
		EOS.P2PConnectionInfo.insertProductIdIntoPeers(EOS.CurrentUserInfo.getProductUserIdHandle());
		return;
	}
	else
	{
		EOSFuncs::logError("OnLobbyJoinCallback: Callback failure: %d", static_cast<int>(data->ResultCode));
		EOS.bConnectingToLobbyWindow = false;
		EOS.bConnectingToLobby = false;
		EOS.ConnectingToLobbyStatus = static_cast<int>(data->ResultCode);
	}

	LobbyLeaveCleanup(EOS.CurrentLobbyData);
	EOS.LobbyParameters.clearLobbyToJoin();
}

void EOS_CALL EOSFuncs::OnLobbyLeaveCallback(const EOS_Lobby_LeaveLobbyCallbackInfo* data)
{
	if (!data)
	{
		EOSFuncs::logError("OnLobbyLeaveCallback: null data");
	}
	else if (data->ResultCode == EOS_EResult::EOS_Success)
	{
		EOSFuncs::logInfo("OnLobbyLeaveCallback: Left lobby id: %s", data->LobbyId);
		if (EOS.CurrentLobbyData.LobbyId.compare(data->LobbyId) == 0)
		{
			LobbyLeaveCleanup(EOS.CurrentLobbyData);
		}
		return;
	}
	else if (data->ResultCode == EOS_EResult::EOS_NotFound)
	{
		EOSFuncs::logInfo("OnLobbyLeaveCallback: Could not find lobby id to leave: %s", data->LobbyId);
		if (EOS.CurrentLobbyData.LobbyId.compare(data->LobbyId) == 0)
		{
			LobbyLeaveCleanup(EOS.CurrentLobbyData);
		}
		return;
	}
	else
	{
		EOSFuncs::logError("OnLobbyLeaveCallback: Callback failure: %d", static_cast<int>(data->ResultCode));
	}
}

void EOS_CALL EOSFuncs::OnIncomingConnectionRequest(const EOS_P2P_OnIncomingConnectionRequestInfo* data)
{
	if (!EOS.PlatformHandle)
	{
		return;
	}
	if (data)
	{
		std::string SocketName = data->SocketId->SocketName;
		if (SocketName != "CHAT")
		{
			EOSFuncs::logError("OnIncomingConnectionRequest: bad socket id: %s", SocketName.c_str());
			return;
		}

		EOS_HP2P P2PHandle = EOS_Platform_GetP2PInterface(EOS.PlatformHandle);
		EOS_P2P_AcceptConnectionOptions Options{};
		Options.ApiVersion = EOS_P2P_ACCEPTCONNECTION_API_LATEST;
		Options.LocalUserId = EOS.CurrentUserInfo.getProductUserIdHandle();
		Options.RemoteUserId = data->RemoteUserId;

		EOS_P2P_SocketId SocketId;
		SocketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
		strncpy(SocketId.SocketName, "CHAT", 5);
		Options.SocketId = &SocketId;

		EOS_EResult Result = EOS_P2P_AcceptConnection(P2PHandle, &Options);
		if (Result != EOS_EResult::EOS_Success)
		{
			EOSFuncs::logError("OnIncomingConnectionRequest: error while accepting connection, code: %d", static_cast<int>(Result));
		}
		else
		{
			EOS.P2PConnectionInfo.insertProductIdIntoPeers(Options.RemoteUserId);
		}
	}
	else
	{
		EOSFuncs::logError("OnIncomingConnectionRequest: null data");
	}
}

void EOS_CALL EOSFuncs::OnLobbyUpdateFinished(const EOS_Lobby_UpdateLobbyCallbackInfo* data)
{
	if (data)
	{
		if (EOS.LobbyModificationHandle)
		{
			EOS_LobbyModification_Release(EOS.LobbyModificationHandle);
			EOS.LobbyModificationHandle = nullptr;
		}

		if (data->ResultCode != EOS_EResult::EOS_Success)
		{
			EOSFuncs::logError("OnLobbyUpdateFinished: Callback failure: %d", static_cast<int>(data->ResultCode));
			return;
		}
		else
		{
			EOSFuncs::logInfo("OnLobbyUpdateFinished: Success");
		}
	}
	else
	{
		EOSFuncs::logError("OnLobbyUpdateFinished: null data");
	}
}

void EOS_CALL EOSFuncs::OnLobbyMemberUpdateFinished(const EOS_Lobby_UpdateLobbyCallbackInfo* data)
{
	if (data)
	{
		if (EOS.LobbyMemberModificationHandle)
		{
			EOS_LobbyModification_Release(EOS.LobbyMemberModificationHandle);
			EOS.LobbyMemberModificationHandle = nullptr;
		}

		if (data->ResultCode != EOS_EResult::EOS_Success && data->ResultCode != EOS_EResult::EOS_NoChange)
		{
			EOSFuncs::logError("OnLobbyMemberUpdateFinished: Callback failure: %d", static_cast<int>(data->ResultCode));
			return;
		}
		else
		{
			EOSFuncs::logInfo("OnLobbyMemberUpdateFinished: Success");
		}
	}
	else
	{
		EOSFuncs::logError("OnLobbyMemberUpdateFinished: null data");
	}
}

void EOS_CALL EOSFuncs::OnQueryAccountMappingsCallback(const EOS_Connect_QueryProductUserIdMappingsCallbackInfo* data)
{
	if (!EOS.PlatformHandle)
	{
		return;
	}
	if (data)
	{
		if (data->ResultCode == EOS_EResult::EOS_Success)
		{
			EOSFuncs::logInfo("OnQueryAccountMappingsCallback: Success");

			bool authEnabledForLocalUser = false; // if we're using auth() interface we can use EpicAccountIds for queries
			EOS_ELoginStatus loginStatus = EOS_Auth_GetLoginStatus(EOS_Platform_GetAuthInterface(EOS.PlatformHandle),
				EOSFuncs::Helpers_t::epicIdFromString(EOS.CurrentUserInfo.epicAccountId.c_str()));
			if (loginStatus == EOS_ELoginStatus::EOS_LS_LoggedIn)
			{
				authEnabledForLocalUser = true;
			}

			std::vector<EOS_ProductUserId> MappingsReceived;
			for (const EOS_ProductUserId& productId : EOS.ProductIdsAwaitingAccountMappingCallback)
			{
				EOS_Connect_GetProductUserIdMappingOptions Options{};
				Options.ApiVersion = EOS_CONNECT_GETPRODUCTUSERIDMAPPING_API_LATEST;
				Options.AccountIdType = EOS_EExternalAccountType::EOS_EAT_EPIC;
				Options.LocalUserId = EOS.CurrentUserInfo.getProductUserIdHandle();
				Options.TargetProductUserId = productId;

				EOS_HConnect ConnectHandle = EOS_Platform_GetConnectInterface(EOS.PlatformHandle);
				char buffer[EOS_CONNECT_EXTERNAL_ACCOUNT_ID_MAX_LENGTH];
				int bufferSize = EOS_CONNECT_EXTERNAL_ACCOUNT_ID_MAX_LENGTH;
				EOS_EResult Result = EOS_Connect_GetProductUserIdMapping(ConnectHandle, &Options, buffer, &bufferSize);
				if (Result == EOS_EResult::EOS_Success)
				{
					std::string receivedStr(buffer, bufferSize);
					EOS_EpicAccountId epicAccountId = EOSFuncs::Helpers_t::epicIdFromString(receivedStr.c_str());

					// insert the ids into the global map
					if (authEnabledForLocalUser)
					{
						EOS.AccountMappings.insert(std::pair<EOS_ProductUserId, EOS_EpicAccountId>(productId, epicAccountId));
					}
					else
					{
						EOS.ExternalAccountMappings.insert(std::pair<EOS_ProductUserId, std::string>(productId, buffer));
					}

					for (LobbyData_t::PlayerLobbyData_t& player : EOS.CurrentLobbyData.playersInLobby)
					{
						if (EOSFuncs::Helpers_t::isMatchingProductIds(productId, EOSFuncs::Helpers_t::productIdFromString(player.memberProductUserId.c_str())))
						{
							player.memberEpicAccountId = receivedStr;
							if (authEnabledForLocalUser)
							{
								EOS.getUserInfo(epicAccountId, EOSFuncs::USER_INFO_QUERY_LOBBY_MEMBER, 0);
								EOSFuncs::logInfo("OnQueryAccountMappingsCallback: product id: %s, epic account id: %s",
									player.memberProductUserId.c_str(), player.memberEpicAccountId.c_str());
							}
							else
							{
								EOS.getExternalAccountUserInfo(productId, EOSFuncs::USER_INFO_QUERY_LOBBY_MEMBER);
							}
							break;
						}
					}
					MappingsReceived.push_back(productId);
				}
				else if (Result == EOS_EResult::EOS_NotFound)
				{
					// try different account types
					Options.AccountIdType = EOS_EExternalAccountType::EOS_EAT_STEAM;
					Result = EOS_Connect_GetProductUserIdMapping(ConnectHandle, &Options, buffer, &bufferSize);
					if (Result == EOS_EResult::EOS_Success)
					{
						EOS.ExternalAccountMappings.insert(std::pair<EOS_ProductUserId, std::string>(productId, buffer));
						if (EOSFuncs::Helpers_t::isMatchingProductIds(EOS.CurrentUserInfo.getProductUserIdHandle(), productId))
						{
							EOS.getExternalAccountUserInfo(productId, EOSFuncs::USER_INFO_QUERY_LOCAL);
						}
						for (LobbyData_t::PlayerLobbyData_t& player : EOS.CurrentLobbyData.playersInLobby)
						{
							if (EOSFuncs::Helpers_t::isMatchingProductIds(productId, EOSFuncs::Helpers_t::productIdFromString(player.memberProductUserId.c_str())))
							{
								EOS.getExternalAccountUserInfo(productId, EOSFuncs::USER_INFO_QUERY_LOBBY_MEMBER);
							}
						}
						MappingsReceived.push_back(productId);
					}
					else if (Result == EOS_EResult::EOS_NotFound)
					{
						// try different account types
						Options.AccountIdType = EOS_EExternalAccountType::EOS_EAT_NINTENDO;
						Result = EOS_Connect_GetProductUserIdMapping(ConnectHandle, &Options, buffer, &bufferSize);
						if (Result == EOS_EResult::EOS_Success)
						{
							EOS.ExternalAccountMappings.insert(std::pair<EOS_ProductUserId, std::string>(productId, buffer));
							if (EOSFuncs::Helpers_t::isMatchingProductIds(EOS.CurrentUserInfo.getProductUserIdHandle(), productId))
							{
								EOS.getExternalAccountUserInfo(productId, EOSFuncs::USER_INFO_QUERY_LOCAL);
							}
							for (LobbyData_t::PlayerLobbyData_t& player : EOS.CurrentLobbyData.playersInLobby)
							{
								if (EOSFuncs::Helpers_t::isMatchingProductIds(productId, EOSFuncs::Helpers_t::productIdFromString(player.memberProductUserId.c_str())))
								{
									EOS.getExternalAccountUserInfo(productId, EOSFuncs::USER_INFO_QUERY_LOBBY_MEMBER);
								}
							}
							MappingsReceived.push_back(productId);
						}
						else
						{
							// Result does not return EOS_Success querying type EOS_EExternalAccountType::EOS_EAT_NINTENDO, so try get info anyway.
							EOS.ExternalAccountMappings.insert(std::pair<EOS_ProductUserId, std::string>(productId, ""));
							for (LobbyData_t::PlayerLobbyData_t& player : EOS.CurrentLobbyData.playersInLobby)
							{
								if (EOSFuncs::Helpers_t::isMatchingProductIds(productId, EOSFuncs::Helpers_t::productIdFromString(player.memberProductUserId.c_str())))
								{
									EOS.getExternalAccountUserInfo(productId, EOSFuncs::USER_INFO_QUERY_LOBBY_MEMBER);
								}
							}
							MappingsReceived.push_back(productId);
						}
					}
				}
			}

			for (const EOS_ProductUserId& productId : MappingsReceived)
			{
				EOS.ProductIdsAwaitingAccountMappingCallback.erase(productId);
			}
		}
		else if (data->ResultCode != EOS_EResult::EOS_OperationWillRetry)
		{
			EOSFuncs::logError("OnQueryAccountMappingsCallback: retrying");
		}
	}
}

void EOSFuncs::getExternalAccountUserInfo(EOS_ProductUserId targetId, UserInfoQueryType queryType)
{
	EOS_Connect_CopyProductUserInfoOptions CopyProductUserInfoOptions{};
	CopyProductUserInfoOptions.ApiVersion = EOS_CONNECT_COPYPRODUCTUSERINFO_API_LATEST;
	CopyProductUserInfoOptions.TargetUserId = targetId;

	EOS_Connect_ExternalAccountInfo* ExternalAccountInfo = nullptr;
	EOS_EResult CopyResult = EOS_Connect_CopyProductUserInfo(ConnectHandle, &CopyProductUserInfoOptions, &ExternalAccountInfo);
	if (CopyResult != EOS_EResult::EOS_Success)
	{
		EOSFuncs::logInfo("getExternalAccountUserInfo: Error %d", static_cast<int>(CopyResult));
		EOS_Connect_ExternalAccountInfo_Release(ExternalAccountInfo);
		return;
	}
	else if (!ExternalAccountInfo)
	{
		EOSFuncs::logInfo("getExternalAccountUserInfo: Info was null");
		return;
	}

	EOSFuncs::logInfo("getExternalAccountUserInfo: Received info for product id: %s", EOSFuncs::Helpers_t::shortProductIdToString(targetId).c_str());

	if (queryType == UserInfoQueryType::USER_INFO_QUERY_LOCAL)
	{
		// local user
		EOS.CurrentUserInfo.Name = ExternalAccountInfo->DisplayName;
		EOS.CurrentUserInfo.bUserInfoRequireUpdate = false;
		EOSFuncs::logInfo("getExternalAccountUserInfo: Current User Name: %s", ExternalAccountInfo->DisplayName);
	}
	else if (queryType == UserInfoQueryType::USER_INFO_QUERY_LOBBY_MEMBER)
	{
		if (!CurrentLobbyData.currentLobbyIsValid() || CurrentLobbyData.playersInLobby.empty())
		{
			EOSFuncs::logInfo("getExternalAccountUserInfo: lobby member request failed due to invalid or no player data in lobby");
		}
		else
		{
			bool foundMember = false;
			std::string queryTarget = EOSFuncs::Helpers_t::productIdToString(targetId);
			for (auto& it : EOS.CurrentLobbyData.playersInLobby)
			{
				if (queryTarget.compare(it.memberProductUserId.c_str()) == 0)
				{
					foundMember = true;
					it.name = ExternalAccountInfo->DisplayName;
					it.accountType = ExternalAccountInfo->AccountIdType;
					it.bUserInfoRequireUpdate = false;
					EOSFuncs::logInfo("getExternalAccountUserInfo: found lobby username: %s, account type: %d", ExternalAccountInfo->DisplayName, ExternalAccountInfo->AccountIdType);
					break;
				}
			}
			if (!foundMember)
			{
				EOSFuncs::logInfo("getExternalAccountUserInfo: could not find player in current lobby with product id %s",
					EOSFuncs::Helpers_t::shortProductIdToString(targetId).c_str());
			}
		}
	}
	EOS_Connect_ExternalAccountInfo_Release(ExternalAccountInfo);
}

void EOS_CALL EOSFuncs::OnMemberUpdateReceived(const EOS_Lobby_LobbyMemberUpdateReceivedCallbackInfo* data)
{
	if (data)
	{
		if (EOS.CurrentLobbyData.LobbyId.compare(data->LobbyId) == 0)
		{
			EOS.CurrentLobbyData.updateLobby();
			EOSFuncs::logInfo("OnMemberUpdateReceived: received user: %s, updating lobby", EOSFuncs::Helpers_t::shortProductIdToString(data->TargetUserId).c_str());
		}
		else
		{
			EOSFuncs::logInfo("OnMemberUpdateReceived: success. lobby id was not CurrentLobbyData");
		}
	}
	else
	{
		EOSFuncs::logError("OnMemberUpdateReceived: null data");
	}
}

void EOS_CALL EOSFuncs::OnMemberStatusReceived(const EOS_Lobby_LobbyMemberStatusReceivedCallbackInfo* data)
{
	if (!EOS.PlatformHandle)
	{
		return;
	}
	if (data)
	{
		switch (data->CurrentStatus)
		{
		case EOS_ELobbyMemberStatus::EOS_LMS_CLOSED:
		case EOS_ELobbyMemberStatus::EOS_LMS_DISCONNECTED:
		case EOS_ELobbyMemberStatus::EOS_LMS_KICKED:
		case EOS_ELobbyMemberStatus::EOS_LMS_LEFT:
			if (EOS.P2PConnectionInfo.isPeerIndexed(data->TargetUserId))
			{
				EOS_P2P_CloseConnectionOptions closeOptions{};
				closeOptions.ApiVersion = EOS_P2P_CLOSECONNECTIONS_API_LATEST;
				closeOptions.LocalUserId = EOS.CurrentUserInfo.getProductUserIdHandle();
				closeOptions.RemoteUserId = data->TargetUserId;

				EOS_P2P_SocketId SocketId = {};
				SocketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
				strncpy(SocketId.SocketName, "CHAT", 5);
				closeOptions.SocketId = &SocketId;
				EOS_EResult result = EOS_P2P_CloseConnection(EOS_Platform_GetP2PInterface(EOS.PlatformHandle), &closeOptions);
				EOSFuncs::logInfo("OnMemberStatusReceived: closing P2P connections, result: %d", static_cast<int>(result));

				EOS.P2PConnectionInfo.assignPeerIndex(data->TargetUserId, -1);
			}

			if (EOS.CurrentLobbyData.LobbyId.compare(data->LobbyId) == 0)
			{
				if (data->CurrentStatus == EOS_ELobbyMemberStatus::EOS_LMS_CLOSED
					|| (data->CurrentStatus == EOS_ELobbyMemberStatus::EOS_LMS_KICKED
					&& data->TargetUserId == EOS.CurrentUserInfo.getProductUserIdHandle())
					)
				{
					// if lobby closed or we got kicked, then clear data.
					LobbyLeaveCleanup(EOS.CurrentLobbyData);
					switch (data->CurrentStatus) {
					case EOS_ELobbyMemberStatus::EOS_LMS_CLOSED:
						//MainMenu::disconnectedFromServer("The host has shutdown the lobby.");
						break;
					case EOS_ELobbyMemberStatus::EOS_LMS_KICKED:
						//MainMenu::disconnectedFromServer("You have been kicked\nfrom the online lobby.");
						break;
					default:
						break;
					}
				}
				else
				{
					EOS.CurrentLobbyData.updateLobby();
					EOSFuncs::logInfo("OnMemberStatusReceived: received user: %s, event: %d, updating lobby",
						EOSFuncs::Helpers_t::shortProductIdToString(data->TargetUserId).c_str(),
						static_cast<int>(data->CurrentStatus));
					return;
				}
			}
			break;
		case EOS_ELobbyMemberStatus::EOS_LMS_JOINED:
		case EOS_ELobbyMemberStatus::EOS_LMS_PROMOTED:
			if (EOS.CurrentLobbyData.LobbyId.compare(data->LobbyId) == 0)
			{
				EOS.CurrentLobbyData.updateLobby();
				EOSFuncs::logInfo("OnMemberStatusReceived: received user: %s, event: %d, updating lobby",
					EOSFuncs::Helpers_t::shortProductIdToString(data->TargetUserId).c_str(),
					static_cast<int>(data->CurrentStatus));
				return;
			}
			break;
		default:
			break;
		}
		EOSFuncs::logInfo("OnMemberStatusReceived: success, received user: %s | status: %d",
			EOSFuncs::Helpers_t::shortProductIdToString(data->TargetUserId).c_str(), static_cast<int>(data->CurrentStatus));
	}
	else
	{
		EOSFuncs::logError("OnMemberStatusReceived: null data");
	}

	//	bool UpdateLobby = true;
	//	//Current player updates need special handling
	//	if ( Data->TargetUserId == Player->GetProductUserID() )
	//	{
	//		if ( Data->CurrentStatus == EOS_ELobbyMemberStatus::EOS_LMS_CLOSED ||
	//			Data->CurrentStatus == EOS_ELobbyMemberStatus::EOS_LMS_KICKED )
	//		{
	//			FGame::Get().GetLobbies()->OnKickedFromLobby(Data->LobbyId);
	//			UpdateLobby = false;
	//		}
	//	}

	//	if ( UpdateLobby )
	//	{
	//		//Simply update the whole lobby
	//		FGame::Get().GetLobbies()->OnLobbyUpdate(Data->LobbyId);
	//	}
}

void EOS_CALL EOSFuncs::OnLobbyUpdateReceived(const EOS_Lobby_LobbyUpdateReceivedCallbackInfo* data)
{
	if (data)
	{
		if (EOS.CurrentLobbyData.LobbyId.compare(data->LobbyId) == 0)
		{
			EOS.CurrentLobbyData.updateLobby();
		}
		EOSFuncs::logInfo("OnLobbyUpdateReceived: success");
	}
	else
	{
		EOSFuncs::logError("OnLobbyUpdateReceived: null data");
	}
}

void EOS_CALL EOSFuncs::OnDestroyLobbyFinished(const EOS_Lobby_DestroyLobbyCallbackInfo* data)
{
	if (data)
	{
		if (data->ResultCode != EOS_EResult::EOS_Success)
		{
			EOSFuncs::logError("OnDestroyLobbyFinished: error destroying lobby id: %s code: %d", data->LobbyId, static_cast<int>(data->ResultCode));
		}
		else
		{
			EOSFuncs::logInfo("OnDestroyLobbyFinished: success, id %s", data->LobbyId);
		}
	}
	else
	{
		EOSFuncs::logError("OnDestroyLobbyFinished: null data");
	}
}

void EOS_CALL EOSFuncs::ConnectAuthExpirationCallback(const EOS_Connect_AuthExpirationCallbackInfo* data)
{
	if (data)
	{
		EOSFuncs::logInfo("ConnectAuthExpirationCallback: connect auth expiring - product id: %s",
			EOSFuncs::Helpers_t::productIdToString(data->LocalUserId));
#if defined(STEAMWORKS) || defined(NINTENDO)
		if (LobbyHandler.crossplayEnabled)
		{
			EOSFuncs::logInfo("ConnectAuthExpirationCallback: Reconnecting crossplay account");
			EOS.CrossplayAccountManager.autologin = true;
			EOS.CrossplayAccountManager.connectLoginCompleted = EOS_EResult::EOS_NotConfigured;
		}
#else
		EOS.initConnectLogin();
#endif // STEAMWORKS
	}
	else
	{
		EOSFuncs::logError("ConnectAuthExpirationCallback: null data");
	}
}

bool EOSFuncs::serialize(void* file) {
	int version = 0;
	FileInterface* fileInterface = static_cast<FileInterface*>(file);
	fileInterface->property("version", version);
	fileInterface->property("credentialhost", CredentialHost);
	fileInterface->property("credentialname", CredentialName);
	return true;
}

#ifdef NINTENDO
static void EOS_CALL CustomFree(void* Ptr)
{
	free(Ptr);
}

static void* EOS_CALL CustomMalloc(size_t Size, size_t Alignment)
{
	return aligned_alloc(Alignment, Size);
}

static void* EOS_CALL CustomRealloc(void* Ptr, size_t Size, size_t Alignment)
{
	// Realloc.
	void* NewPtr = realloc(Ptr, Size);

	// Did the alignment break?
	if ((uintptr_t)NewPtr & (Alignment - 1))
	{
		// Alloc a fresh space.
		void* AlignedPtr = CustomMalloc(Size, Alignment);
		memcpy(AlignedPtr, NewPtr, Size);
		CustomFree(NewPtr);
		NewPtr = AlignedPtr;
	}

	return NewPtr;
}
#endif

#ifdef NINTENDO
EOS_Bool EOS_CALL Game_OnNetworkRequested()
{
	return nxConnectedToNetwork() ? EOS_TRUE : EOS_FALSE;
}
#endif

bool EOSFuncs::initPlatform(bool enableLogging)
{
	if (initialized)
	{
		printlog("EOS::initPlatform() called, but we're already initialized (ignored)");
		return true;
	}
#ifdef NINTENDO
	EOS_Switch_InitializeOptions SwitchOptions{};
	SwitchOptions.ApiVersion = EOS_SWITCH_INITIALIZEOPTIONS_API_LATEST;
	//SwitchOptions.OnNetworkRequested = &Game_OnNetworkRequested;
	SwitchOptions.OnNetworkRequested_DEPRECATED = nullptr;
	SwitchOptions.CacheStorageSizekB = 0; // EOS_SWITCH_MIN_CACHE_STORAGE_SIZE_KB
	SwitchOptions.CacheStorageIndex = 0;

	EOS_InitializeOptions InitializeOptions{};
	InitializeOptions.ProductName = "Barony";
	InitializeOptions.ProductVersion = VERSION;
	InitializeOptions.ApiVersion = EOS_INITIALIZE_API_LATEST;
	InitializeOptions.AllocateMemoryFunction = CustomMalloc;
	InitializeOptions.ReallocateMemoryFunction = CustomRealloc;
	InitializeOptions.ReleaseMemoryFunction = CustomFree;
	InitializeOptions.Reserved = nullptr;
	InitializeOptions.SystemInitializeOptions = &SwitchOptions;
	InitializeOptions.OverrideThreadAffinity = nullptr;
#else
	EOS_InitializeOptions InitializeOptions{};
	InitializeOptions.ProductName = "Barony";
	InitializeOptions.ProductVersion = VERSION;
	InitializeOptions.ApiVersion = EOS_INITIALIZE_API_LATEST;
	InitializeOptions.AllocateMemoryFunction = nullptr;
	InitializeOptions.ReallocateMemoryFunction = nullptr;
	InitializeOptions.ReleaseMemoryFunction = nullptr;
	InitializeOptions.Reserved = nullptr;
	InitializeOptions.SystemInitializeOptions = nullptr;
	InitializeOptions.OverrideThreadAffinity = nullptr;
#endif
	EOS_EResult result = EOS_Initialize(&InitializeOptions);
	if (result != EOS_EResult::EOS_Success && result != EOS_EResult::EOS_AlreadyConfigured)
	{
		logError("initPlatform: Failure to initialize - error code: %d", static_cast<int>(result));
		return false;
	}
	else
	{
		logInfo("initPlatform: Initialize success");
	}

	if (enableLogging)
	{
		EOS_EResult SetLogCallbackResult = EOS_Logging_SetCallback(&this->LoggingCallback);
		if (SetLogCallbackResult != EOS_EResult::EOS_Success &&
			SetLogCallbackResult != EOS_EResult::EOS_AlreadyConfigured)
		{
			logError("SetLogCallbackResult: Set Logging Callback Failed!");
		}
		else
		{
			logInfo("SetLogCallbackResult: Logging Callback set");
			EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_ALL_CATEGORIES, EOS_ELogLevel::EOS_LOG_Info);
		}
	}

	EOS_Platform_Options PlatformOptions{};
	PlatformOptions.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
	PlatformOptions.Reserved = nullptr;
	PlatformOptions.ProductId = BUILD_ENV_PR;
	PlatformOptions.SandboxId = BUILD_ENV_SA;
	PlatformOptions.DeploymentId = BUILD_ENV_DE;
	PlatformOptions.ClientCredentials.ClientId = BUILD_ENV_CC;
	PlatformOptions.ClientCredentials.ClientSecret = BUILD_ENV_CS;
	PlatformOptions.OverrideCountryCode = nullptr;
	PlatformOptions.OverrideLocaleCode = nullptr;
	PlatformOptions.bIsServer = EOS_FALSE;
	PlatformOptions.Flags = EOS_PF_DISABLE_OVERLAY;
#ifdef WINDOWS
#ifndef STEAMWORKS
	PlatformOptions.Flags = EOS_PF_DISABLE_OVERLAY;
#endif
#endif
	static std::string EncryptionKey(64, '1');
	PlatformOptions.EncryptionKey = EncryptionKey.c_str();
	PlatformOptions.CacheDirectory = nullptr; // important - needs double slashes and absolute path
	PlatformOptions.TickBudgetInMilliseconds = 0; // do all available work

	//EOS_Platform_RTCOptions rtcOptions{};
	//rtcOptions.ApiVersion = EOS_PLATFORM_RTCOPTIONS_API_LATEST;
	//rtcOptions.BackgroundMode = EOS_ERTCBackgroundMode::EOS_RTCBM_KeepRoomsAlive;
	//rtcOptions.PlatformSpecificOptions = nullptr; // must be null initialized
	PlatformOptions.RTCOptions = nullptr;

	PlatformOptions.IntegratedPlatformOptionsContainerHandle = nullptr; // must be null initialized
#ifndef LINUX
	PlatformOptions.SystemSpecificOptions = nullptr; // must be null initialized
#endif

	PlatformHandle = EOS_Platform_Create(&PlatformOptions);
	PlatformOptions.bIsServer = EOS_TRUE;
	ServerPlatformHandle = EOS_Platform_Create(&PlatformOptions);

	PlatformOptions.ClientCredentials.ClientId = nullptr;
	PlatformOptions.ClientCredentials.ClientSecret = nullptr;
	PlatformOptions.ProductId = nullptr;
	PlatformOptions.SandboxId = nullptr;
	PlatformOptions.DeploymentId = nullptr;

	if (!PlatformHandle || !ServerPlatformHandle)
	{
		logError("PlatformHandle: Platform failed to initialize - invalid handle");
		return false;
	}

#ifdef NINTENDO
	const bool connected = nxConnectedToNetwork();
	auto networkStatus = connected ?
		EOS_ENetworkStatus::EOS_NS_Online : EOS_ENetworkStatus::EOS_NS_Disabled;
	EOS_Platform_SetNetworkStatus(PlatformHandle, networkStatus);
	EOS_Platform_SetNetworkStatus(ServerPlatformHandle, networkStatus);
	oldNetworkStatus = connected;
#endif

#if !defined(STEAMWORKS) && !defined(NINTENDO)
#ifdef WINDOWS
#ifdef NDEBUG
	appRequiresRestart = EOS_Platform_CheckForLauncherAndRestart(EOS.PlatformHandle);
#endif
#else
	// #ifdef APPLE
	// 	SDL_Event event;
	// 	Uint32 startAuthTicks = SDL_GetTicks();
	// 	Uint32 currentAuthTicks = startAuthTicks;
	// 	while (1)
	// 	{
	// 		while ( SDL_PollEvent(&event) != 0 )
	// 		{
	// 			//Makes Mac work because Apple had to do it different.
	// 		}
	// 		EOS_Platform_Tick(PlatformHandle);
	// 		SDL_Delay(50);

	// 		currentAuthTicks = SDL_GetTicks();
	// 		logInfo("*whirl*");
	// 		if ( currentAuthTicks - startAuthTicks >= 5000 ) // spin the wheels for 5 seconds
	// 		{
	// 			break;
	// 		}
	// 	}
	// #endif
	appRequiresRestart = EOS_Platform_CheckForLauncherAndRestart(EOS.PlatformHandle);
	// printlog("See you on the other side!");
	// while ( SDL_PollEvent(&event) != 0 )
	// {
	// 	//Makes Mac work because Apple had to do it different.
	// }
	// EOS_Platform_Tick(PlatformHandle);
	// while ( SDL_PollEvent(&event) != 0 )
	// {
	// 	//Makes Mac work because Apple had to do it different.
	// }
	// exit(1);
#endif
#endif

#if defined(STEAMWORKS)
	EOS.StatGlobalManager.queryGlobalStatUser();
#endif
	initialized = true;
	return true;
}

void EOSFuncs::initConnectLogin() // should not handle for Steam connect logins
{
#ifdef NINTENDO
	if (!nxConnectedToNetwork()) {
		return;
	}
#endif

	if (!EOS.PlatformHandle)
	{
		return;
	}

	ConnectHandle = EOS_Platform_GetConnectInterface(PlatformHandle);

#ifdef NINTENDO
	EOS_Auth_Token* UserAuthToken = nullptr;
	EOS_Auth_CopyUserAuthTokenOptions CopyTokenOptions = { 0 };
	CopyTokenOptions.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

	if (EOS_Auth_CopyUserAuthToken(AuthHandle, &CopyTokenOptions,
		EOSFuncs::Helpers_t::epicIdFromString(CurrentUserInfo.epicAccountId.c_str()), &UserAuthToken) == EOS_EResult::EOS_Success)
	{
		logInfo("initConnectLogin: Auth expires: %f", UserAuthToken->ExpiresIn);

		static char token[4096];

		EOS_Connect_Credentials Credentials;
		Credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
		Credentials.Type = EOS_EExternalCredentialType::EOS_ECT_NINTENDO_NSA_ID_TOKEN;
		Credentials.Token = nxGetNSAID(token, sizeof(token));

		if (!Credentials.Token) {
			logError("initConnectLogin: Credential token not available, is the user online?");
			return;
		}

		EOS_Connect_UserLoginInfo Info{};
		Info.ApiVersion = EOS_CONNECT_USERLOGININFO_API_LATEST;
		Info.DisplayName = MainMenu::getUsername();

		EOS_Connect_LoginOptions Options{};
		Options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
		Options.Credentials = &Credentials;
		Options.UserLoginInfo = &Info;

		EOS_Connect_Login(ConnectHandle, &Options, nullptr, ConnectLoginCompleteCallback);
		EOS_Auth_Token_Release(UserAuthToken);
	}
#else

	EOS_Auth_Token* UserAuthToken = nullptr;
	EOS_Auth_CopyUserAuthTokenOptions CopyTokenOptions = { 0 };
	CopyTokenOptions.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

	if (EOS_Auth_CopyUserAuthToken(AuthHandle, &CopyTokenOptions,
		EOSFuncs::Helpers_t::epicIdFromString(CurrentUserInfo.epicAccountId.c_str()), &UserAuthToken) == EOS_EResult::EOS_Success)
	{
		logInfo("initConnectLogin: Auth expires: %f", UserAuthToken->ExpiresIn);

		EOS_Connect_Credentials Credentials;
		Credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
		Credentials.Type = EOS_EExternalCredentialType::EOS_ECT_EPIC; // change this to steam etc for different account providers.
		Credentials.Token = UserAuthToken->AccessToken;

		EOS_Connect_LoginOptions Options{};
		Options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
		Options.Credentials = &Credentials;
		Options.UserLoginInfo = nullptr;

		EOS_Connect_Login(ConnectHandle, &Options, nullptr, ConnectLoginCompleteCallback);
		EOS_Auth_Token_Release(UserAuthToken);
	}
#endif
}

void EOSFuncs::readFromFile()
{
	if (PHYSFS_getRealDir("/data/eos.json"))
	{
		std::string inputPath = PHYSFS_getRealDir("/data/eos.json");
		inputPath.append("/data/eos.json");
		if (FileHelper::readObject(inputPath.c_str(), *this))
		{
			EOSFuncs::logInfo("[JSON]: Successfully read json file %s", inputPath.c_str());
		}
	}
}

void EOSFuncs::readFromCmdLineArgs()
{
	for (auto& arg : CommandLineArgs)
	{
		if (arg.find("-AUTH_PASSWORD=") != std::string::npos)
		{
			EOSFuncs::logInfo("Launching from store...");
			CredentialName = arg.substr(strlen("-AUTH_PASSWORD="));
		}
		else if (arg.find("-AUTH_TYPE=exchangecode") != std::string::npos)
		{
			EOS.AccountManager.AuthType = EOS_ELoginCredentialType::EOS_LCT_ExchangeCode;
		}
	}
}

bool EOSFuncs::HandleReceivedMessages(EOS_ProductUserId* remoteIdReturn)
{
	if (!CurrentUserInfo.isValid())
	{
		//logError("HandleReceivedMessages: Invalid local user Id: %s", CurrentUserInfo.getProductUserIdStr());
		return false;
	}

	if (!net_packet)
	{
		return false;
	}

	if (!EOS.PlatformHandle)
	{
		return false;
	}

	EOS_HP2P P2PHandle = EOS_Platform_GetP2PInterface(PlatformHandle);

	EOS_P2P_ReceivePacketOptions ReceivePacketOptions{};
	ReceivePacketOptions.ApiVersion = EOS_P2P_RECEIVEPACKET_API_LATEST;
	ReceivePacketOptions.LocalUserId = CurrentUserInfo.getProductUserIdHandle();
	ReceivePacketOptions.MaxDataSizeBytes = 512;
	ReceivePacketOptions.RequestedChannel = nullptr;

	EOS_P2P_SocketId SocketId;
	SocketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
	uint8_t Channel = 0;

	Uint32 bytesWritten = 0;
	EOS_EResult result = EOS_P2P_ReceivePacket(P2PHandle, &ReceivePacketOptions, remoteIdReturn, &SocketId, &Channel, net_packet->data, &bytesWritten);
	if (result == EOS_EResult::EOS_NotFound
		|| result == EOS_EResult::EOS_InvalidAuth
		|| result == EOS_EResult::EOS_InvalidUser)
	{
		//no more packets, just end
		return false;
	}
	else if (result == EOS_EResult::EOS_Success)
	{
		net_packet->len = bytesWritten;
		//char buffer[512] = "";
		//strncpy_s(buffer, (char*)net_packet->data, 512 - 1);
		//buffer[4] = '\0';
		//logInfo("HandleReceivedMessages: remote id: %s received: %s", EOSFuncs::Helpers_t::productIdToString(*remoteIdReturn).c_str(), buffer);
		return true;
	}
	else
	{
		logError("HandleReceivedMessages: error while reading data, code: %d", static_cast<int>(result));
		return false;
	}
}

// function to empty the packet queue on main lobby.
bool EOSFuncs::HandleReceivedMessagesAndIgnore(EOS_ProductUserId* remoteIdReturn)
{
	if (!CurrentUserInfo.isValid())
	{
		//logError("HandleReceivedMessages: Invalid local user Id: %s", CurrentUserInfo.getProductUserIdStr());
		return false;
	}

	if (!EOS.PlatformHandle)
	{
		return false;
	}

	EOS_HP2P P2PHandle = EOS_Platform_GetP2PInterface(PlatformHandle);

	EOS_P2P_ReceivePacketOptions ReceivePacketOptions{};
	ReceivePacketOptions.ApiVersion = EOS_P2P_RECEIVEPACKET_API_LATEST;
	ReceivePacketOptions.LocalUserId = CurrentUserInfo.getProductUserIdHandle();
	ReceivePacketOptions.MaxDataSizeBytes = 512;
	ReceivePacketOptions.RequestedChannel = nullptr;

	EOS_P2P_SocketId SocketId;
	SocketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
	uint8_t Channel = 0;

	Uint32 bytesWritten = 0;
	Uint8 dummyData[512];
	EOS_EResult result = EOS_P2P_ReceivePacket(P2PHandle, &ReceivePacketOptions, remoteIdReturn, &SocketId, &Channel, dummyData, &bytesWritten);
	if (result == EOS_EResult::EOS_NotFound
		|| result == EOS_EResult::EOS_InvalidAuth
		|| result == EOS_EResult::EOS_InvalidUser)
	{
		//no more packets, just end
		return false;
	}
	else if (result == EOS_EResult::EOS_Success)
	{
		char buffer[512] = "";
		strncpy(buffer, (char*)dummyData, 512 - 1);
		buffer[4] = '\0';
		std::string remoteStr = EOSFuncs::Helpers_t::productIdToString(*remoteIdReturn);
		if ((int)buffer[3] < '0'
			&& (int)buffer[0] == 0
			&& (int)buffer[1] == 0
			&& (int)buffer[2] == 0)
		{
			logInfo("Clearing P2P packet queue: remote id: %s received: %d", remoteStr.c_str(), (int)buffer[3]);
		}
		else
		{
			logInfo("Clearing P2P packet queue: remote id: %s received: %s", remoteStr.c_str(), buffer);
		}
		return true;
	}
	else
	{
		logError("HandleReceivedMessagesAndIgnore: error while reading data, code: %d", static_cast<int>(result));
		return false;
	}
}

void EOSFuncs::SendMessageP2P(EOS_ProductUserId RemoteId, const void* data, int len)
{
	if (!EOSFuncs::Helpers_t::productIdIsValid(RemoteId))
	{
		logError("SendMessageP2P: Invalid remote Id: %s", EOSFuncs::Helpers_t::productIdToString(RemoteId));
		return;
	}

	if (!CurrentUserInfo.isValid())
	{
		logError("SendMessageP2P: Invalid local user Id: %s", CurrentUserInfo.getProductUserIdStr());
		return;
	}

	if (!EOS.PlatformHandle)
	{
		return;
	}

	EOS_HP2P P2PHandle = EOS_Platform_GetP2PInterface(PlatformHandle);

	EOS_P2P_SocketId SocketId;
	SocketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
	strncpy(SocketId.SocketName, "CHAT", 5);

	EOS_P2P_SendPacketOptions SendPacketOptions{};
	SendPacketOptions.ApiVersion = EOS_P2P_SENDPACKET_API_LATEST;
	SendPacketOptions.LocalUserId = CurrentUserInfo.getProductUserIdHandle();
	SendPacketOptions.RemoteUserId = RemoteId;
	SendPacketOptions.SocketId = &SocketId;
	SendPacketOptions.bAllowDelayedDelivery = EOS_TRUE;
	SendPacketOptions.Channel = 0;
	SendPacketOptions.Reliability = EOS_EPacketReliability::EOS_PR_UnreliableUnordered;

	SendPacketOptions.DataLengthBytes = len;
	SendPacketOptions.Data = (char*)data;

	EOS_EResult Result = EOS_P2P_SendPacket(P2PHandle, &SendPacketOptions);
	if (Result != EOS_EResult::EOS_Success)
	{
		logError("SendMessageP2P: error while sending data, code: %d", static_cast<int>(Result));
	}
}

void EOSFuncs::LobbyData_t::setLobbyAttributesFromGame(HostUpdateLobbyTypes updateType)
{
	if (updateType == LOBBY_UPDATE_MAIN_MENU)
	{
		LobbyAttributes.lobbyName = EOS.currentLobbyName;
		LobbyAttributes.gameVersion = VERSION;
		LobbyAttributes.isLobbyLoadingSavedGame = loadingsavegame;
		LobbyAttributes.lobbyKey = loadinglobbykey;
		LobbyAttributes.challengeLid = gameModeManager.currentSession.challengeRun.isActive() ?
			gameModeManager.currentSession.challengeRun.lid : "";
		LobbyAttributes.serverFlags = svFlags;
		LobbyAttributes.numServerMods = Mods::numCurrentModsLoaded;
		LobbyAttributes.modsDisableAchievements = Mods::disableSteamAchievements;
		LobbyAttributes.PermissionLevel = static_cast<Uint32>(EOS.currentPermissionLevel);
		LobbyAttributes.friendsOnly = EOS.bFriendsOnly;
		LobbyAttributes.maxplayersCompatible = MAXPLAYERS;
	}
	else if (updateType == LOBBY_UPDATE_DURING_GAME)
	{
		LobbyAttributes.serverFlags = svFlags;
		LobbyAttributes.gameCurrentLevel = currentlevel;
	}
}

void EOSFuncs::LobbyData_t::setBasicCurrentLobbyDataFromInitialJoin(LobbyData_t* lobbyToJoin)
{
	if (!lobbyToJoin)
	{
		logError("setBasicCurrentLobbyDataFromInitialJoin: invalid lobby passed.");
		return;
	}

	MaxPlayers = lobbyToJoin->MaxPlayers;
	OwnerProductUserId = lobbyToJoin->OwnerProductUserId;
	LobbyId = lobbyToJoin->LobbyId;
	FreeSlots = lobbyToJoin->FreeSlots;
	bLobbyHasBasicDetailsRead = true;

	EOS.P2PConnectionInfo.serverProductId = EOSFuncs::Helpers_t::productIdFromString(OwnerProductUserId.c_str());

	EOS.P2PConnectionInfo.peerProductIds.clear();
	for (PlayerLobbyData_t& player : playersInLobby)
	{
		EOS_ProductUserId productId = EOSFuncs::Helpers_t::productIdFromString(player.memberProductUserId.c_str());
		EOS.P2PConnectionInfo.insertProductIdIntoPeers(productId);
	}
}

bool EOSFuncs::LobbyData_t::currentUserIsOwner()
{
	if (currentLobbyIsValid() && OwnerProductUserId.compare(EOS.CurrentUserInfo.getProductUserIdStr()) == 0)
	{
		return true;
	}
	return false;
};

bool EOSFuncs::LobbyData_t::updateLobbyForHost(HostUpdateLobbyTypes updateType)
{
	if (!EOSFuncs::Helpers_t::isMatchingProductIds(EOSFuncs::Helpers_t::productIdFromString(this->OwnerProductUserId.c_str()),
		EOS.CurrentUserInfo.getProductUserIdHandle()))
	{
		EOSFuncs::logError("updateLobby: current user is not lobby owner");
		return false;
	}

	if (!EOS.PlatformHandle)
	{
		return false;
	}

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(EOS.PlatformHandle);
	EOS_Lobby_UpdateLobbyModificationOptions ModifyOptions{};
	ModifyOptions.ApiVersion = EOS_LOBBY_UPDATELOBBYMODIFICATION_API_LATEST;
	ModifyOptions.LobbyId = this->LobbyId.c_str();
	ModifyOptions.LocalUserId = EOS.CurrentUserInfo.getProductUserIdHandle();

	if (EOS.LobbyModificationHandle)
	{
		EOS_LobbyModification_Release(EOS.LobbyModificationHandle);
		EOS.LobbyModificationHandle = nullptr;
	}
	EOS_HLobbyModification LobbyModification = nullptr;
	EOS_EResult result = EOS_Lobby_UpdateLobbyModification(LobbyHandle, &ModifyOptions, &LobbyModification);
	if (result != EOS_EResult::EOS_Success)
	{
		EOSFuncs::logError("updateLobby: Could not create lobby modification. Error code: %d", static_cast<int>(result));
		return false;
	}

	EOS.LobbyModificationHandle = LobbyModification;
	setLobbyAttributesFromGame(updateType);

	/*EOS_LobbyModification_SetPermissionLevelOptions permissionOptions{};
	permissionOptions.ApiVersion = EOS_LOBBYMODIFICATION_SETPERMISSIONLEVEL_API_LATEST;
	permissionOptions.PermissionLevel = EOS.CurrentLobbyData.PermissionLevel;
	EOS_LobbyModification_SetPermissionLevel(LobbyModification, &permissionOptions);*/

	// build the list of attributes:
	for (int i = 0; i < EOSFuncs::LobbyData_t::kNumAttributes; ++i)
	{
		EOS_Lobby_AttributeData data = {};
		data.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
		data.ValueType = EOS_ELobbyAttributeType::EOS_AT_STRING;
		std::pair<std::string, std::string> dataPair = getAttributePair(static_cast<AttributeTypes>(i));
		if (dataPair.first.compare("empty") == 0 || dataPair.second.compare("empty") == 0)
		{
			EOSFuncs::logError("updateLobby: invalid data value index: %d first: %s, second: %s", i, dataPair.first.c_str(), dataPair.second.c_str());
			continue;
		}
		else
		{
			EOSFuncs::logInfo("updateLobby: keypair %s | %s", dataPair.first.c_str(), dataPair.second.c_str());
		}
		data.Key = dataPair.first.c_str();
		data.Value.AsUtf8 = dataPair.second.c_str();

		if (dataPair.first.compare("MAXPLAYERS") == 0)
		{
			data.Value.AsInt64 = LobbyAttributes.maxplayersCompatible;
			data.ValueType = EOS_ELobbyAttributeType::EOS_AT_INT64;
		}

		EOS_LobbyModification_AddAttributeOptions addAttributeOptions{};
		addAttributeOptions.ApiVersion = EOS_LOBBYMODIFICATION_ADDATTRIBUTE_API_LATEST;
		addAttributeOptions.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;
		addAttributeOptions.Attribute = &(data);

		result = EOS_LobbyModification_AddAttribute(LobbyModification, &addAttributeOptions);
		if (result != EOS_EResult::EOS_Success)
		{
			EOSFuncs::logError("updateLobby: Could not add attribute %s. Error code: %d", addAttributeOptions.Attribute->Value.AsUtf8, static_cast<int>(result));
		}
		else
		{
			//EOSFuncs::logInfo("updateLobby: Added key: %s attribute: %s", AddAttributeOptions.Attribute->Key, AddAttributeOptions.Attribute->Value.AsUtf8);
		}
	}

	// update our client number on the lobby backend
	if (assignClientnumMemberAttribute(EOS.CurrentUserInfo.getProductUserIdHandle(), 0))
	{
		modifyLobbyMemberAttributeForCurrentUser();
	}

	//Trigger lobby update
	EOS_Lobby_UpdateLobbyOptions UpdateOptions{};
	UpdateOptions.ApiVersion = EOS_LOBBY_UPDATELOBBY_API_LATEST;
	UpdateOptions.LobbyModificationHandle = EOS.LobbyModificationHandle;
	EOS_Lobby_UpdateLobby(LobbyHandle, &UpdateOptions, nullptr, OnLobbyUpdateFinished);

	bLobbyHasFullDetailsRead = true;
	return true;
}

bool EOSFuncs::LobbyData_t::modifyLobbyMemberAttributeForCurrentUser()
{
	if (!EOS.CurrentUserInfo.isValid())
	{
		EOSFuncs::logError("modifyLobbyMemberAttributeForCurrentUser: current user is not valid");
		return false;
	}

	if (!EOS.PlatformHandle)
	{
		return false;
	}

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(EOS.PlatformHandle);
	EOS_Lobby_UpdateLobbyModificationOptions ModifyOptions{};
	ModifyOptions.ApiVersion = EOS_LOBBY_UPDATELOBBYMODIFICATION_API_LATEST;
	ModifyOptions.LobbyId = this->LobbyId.c_str();
	ModifyOptions.LocalUserId = EOS.CurrentUserInfo.getProductUserIdHandle();

	if (EOS.LobbyMemberModificationHandle)
	{
		EOS_LobbyModification_Release(EOS.LobbyMemberModificationHandle);
		EOS.LobbyMemberModificationHandle = nullptr;
	}
	EOS_HLobbyModification LobbyMemberModification = nullptr;
	EOS_EResult result = EOS_Lobby_UpdateLobbyModification(LobbyHandle, &ModifyOptions, &LobbyMemberModification);
	if (result != EOS_EResult::EOS_Success)
	{
		EOSFuncs::logError("updateLobby: Could not create lobby modification. Error code: %d", static_cast<int>(result));
		return false;
	}

	EOS.LobbyMemberModificationHandle = LobbyMemberModification;

	// add attributes for current member
	for (auto& player : playersInLobby)
	{
		EOS_ProductUserId productId = EOSFuncs::Helpers_t::productIdFromString(player.memberProductUserId.c_str());
		if (productId != EOS.CurrentUserInfo.getProductUserIdHandle())
		{
			continue;
		}

		EOS_Lobby_AttributeData memberData;
		memberData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
		memberData.ValueType = EOS_ELobbyAttributeType::EOS_AT_INT64;
		memberData.Key = "CLIENTNUM";
		memberData.Value.AsInt64 = player.clientNumber;

		EOS_LobbyModification_AddMemberAttributeOptions addMemberData{};
		addMemberData.ApiVersion = EOS_LOBBYMODIFICATION_ADDMEMBERATTRIBUTE_API_LATEST;
		addMemberData.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;
		addMemberData.Attribute = &memberData;

		result = EOS_LobbyModification_AddMemberAttribute(LobbyMemberModification, &addMemberData);
		if (result != EOS_EResult::EOS_Success)
		{
			EOSFuncs::logError("updateLobby: Could not add member attribute %d. Error code: %d",
				addMemberData.Attribute->Value.AsInt64, static_cast<int>(result));
		}
	}

	//Trigger lobby update
	EOS_Lobby_UpdateLobbyOptions UpdateOptions{};
	UpdateOptions.ApiVersion = EOS_LOBBY_UPDATELOBBY_API_LATEST;
	UpdateOptions.LobbyModificationHandle = EOS.LobbyMemberModificationHandle;
	EOS_Lobby_UpdateLobby(LobbyHandle, &UpdateOptions, nullptr, OnLobbyMemberUpdateFinished);

	//bLobbyHasFullDetailsRead = true;
	return true;
}

bool EOSFuncs::LobbyData_t::assignClientnumMemberAttribute(EOS_ProductUserId targetId, int clientNumToSet)
{
	if (!EOS.CurrentUserInfo.isValid())
	{
		EOSFuncs::logError("assignClientnumMemberAttribute: current user is not valid");
		return false;
	}

	if (!currentLobbyIsValid())
	{
		EOSFuncs::logError("assignClientnumMemberAttribute: current lobby is not valid");
		return false;
	}

	for (auto& player : playersInLobby)
	{
		EOS_ProductUserId playerId = EOSFuncs::Helpers_t::productIdFromString(player.memberProductUserId.c_str());
		if (playerId && playerId == targetId)
		{
			player.clientNumber = clientNumToSet;
			return true;
		}
	}
	return false;
}

int EOSFuncs::LobbyData_t::getClientnumMemberAttribute(EOS_ProductUserId targetId)
{
	if (!EOS.CurrentUserInfo.isValid())
	{
		EOSFuncs::logError("getClientnumMemberAttribute: current user is not valid");
		return -2;
	}

	if (!currentLobbyIsValid())
	{
		EOSFuncs::logError("getClientnumMemberAttribute: current lobby is not valid");
		return -2;
	}

	for (auto& player : playersInLobby)
	{
		EOS_ProductUserId playerId = EOSFuncs::Helpers_t::productIdFromString(player.memberProductUserId.c_str());
		if (playerId && playerId == targetId)
		{
			return player.clientNumber;
		}
	}
	return -2;
}

void EOSFuncs::LobbyData_t::getLobbyAttributes(EOS_HLobbyDetails LobbyDetails)
{
	if (!currentLobbyIsValid())
	{
		EOSFuncs::logError("getLobbyAttributes: invalid current lobby - no ID set");
		return;
	}

	if (!EOS.PlatformHandle)
	{
		return;
	}

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(EOS.PlatformHandle);

	/*EOS_Lobby_CopyLobbyDetailsHandleOptions CopyHandleOptions{};
	CopyHandleOptions.ApiVersion = EOS_LOBBY_COPYLOBBYDETAILSHANDLE_API_LATEST;
	CopyHandleOptions.LobbyId = this->LobbyId.c_str();
	CopyHandleOptions.LocalUserId = EOSFuncs::Helpers_t::productIdFromString(EOS.CurrentUserInfo.getProductUserIdStr());

	EOS_HLobbyDetails LobbyDetailsHandle = nullptr;
	EOS_EResult result = EOS_Lobby_CopyLobbyDetailsHandle(LobbyHandle, &CopyHandleOptions, &LobbyDetailsHandle);
	if ( result != EOS_EResult::EOS_Success )
	{
		EOSFuncs::logError("getLobbyAttributes: can't get lobby info handle. Error code: %d", static_cast<int>(result));
		return;
	}*/

	EOS_LobbyDetails_GetAttributeCountOptions CountOptions{};
	CountOptions.ApiVersion = EOS_LOBBYDETAILS_GETATTRIBUTECOUNT_API_LATEST;
	int numAttributes = EOS_LobbyDetails_GetAttributeCount(LobbyDetails, &CountOptions);

	for (int i = 0; i < numAttributes; ++i)
	{
		EOS_LobbyDetails_CopyAttributeByIndexOptions AttrOptions{};
		AttrOptions.ApiVersion = EOS_LOBBYDETAILS_COPYATTRIBUTEBYINDEX_API_LATEST;
		AttrOptions.AttrIndex = i;

		EOS_Lobby_Attribute* attributePtr = nullptr;
		EOS_EResult result = EOS_LobbyDetails_CopyAttributeByIndex(LobbyDetails, &AttrOptions, &attributePtr);
		if (result == EOS_EResult::EOS_Success && attributePtr->Data)
		{
			EOS_Lobby_AttributeData data;
			data.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
			data.ValueType = attributePtr->Data->ValueType;
			data.Key = attributePtr->Data->Key;
			data.Value.AsUtf8 = attributePtr->Data->Value.AsUtf8;
			this->setLobbyAttributesAfterReading(&data);
		}
		EOS_Lobby_Attribute_Release(attributePtr);
	}

	if (!EOS.CurrentLobbyData.currentUserIsOwner())
	{
		strncpy(EOS.currentLobbyName, LobbyAttributes.lobbyName.c_str(), 31);
	}
	this->bLobbyHasFullDetailsRead = true;
}

void EOSFuncs::createLobby()
{
	/*if ( CurrentLobbyData.currentLobbyIsValid() )
	{
	logInfo("");
	return;
	}*/

	if (CurrentLobbyData.bAwaitingLeaveCallback)
	{
		logInfo("createLobby: CurrentLobbyData.bAwaitingLeaveCallback is true");
	}
	if (CurrentLobbyData.bAwaitingCreationCallback)
	{
		logInfo("createLobby: CurrentLobbyData.bAwaitingCreationCallback is true");
	}

	CurrentLobbyData.bAwaitingCreationCallback = true;

#ifdef NINTENDO
	bFriendsOnly = false;
	bFriendsOnlyUserConfigured = false;
#else
	bFriendsOnly = loadingsavegame ? false : bFriendsOnlyUserConfigured;
#endif
	if (loadingsavegame)
	{
		currentPermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED;
	}
	else
	{
		currentPermissionLevel = currentPermissionLevelUserConfigured;
		if (currentPermissionLevel == EOS_ELobbyPermissionLevel::EOS_LPL_JOINVIAPRESENCE)
		{
			bFriendsOnly = false;
			bFriendsOnlyUserConfigured = false;
		}
	}

	if (!EOS.PlatformHandle)
	{
		return;
	}

	LobbyHandle = EOS_Platform_GetLobbyInterface(PlatformHandle);
	EOS_Lobby_CreateLobbyOptions CreateOptions{};
	CreateOptions.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
	CreateOptions.LocalUserId = CurrentUserInfo.getProductUserIdHandle();
	CreateOptions.MaxLobbyMembers = MAXPLAYERS;
	CreateOptions.PermissionLevel = currentPermissionLevel;
	CreateOptions.bPresenceEnabled = false;
	CreateOptions.bAllowInvites = true;
	CreateOptions.BucketId = EOS_LOBBY_SEARCH_BUCKET_ID;
	CreateOptions.bDisableHostMigration = true;
	CreateOptions.bEnableRTCRoom = false;
	CreateOptions.LocalRTCOptions = nullptr;
	CreateOptions.LobbyId = nullptr;

	EOS_Lobby_CreateLobby(LobbyHandle, &CreateOptions, nullptr, OnCreateLobbyFinished);
	CurrentLobbyData.MaxPlayers = CreateOptions.MaxLobbyMembers;
	CurrentLobbyData.OwnerProductUserId = CurrentUserInfo.getProductUserIdStr();
	strcpy(EOS.currentLobbyName, "Lobby creation in progress...");
}

void EOSFuncs::joinLobby(LobbyData_t* lobby)
{
	if (!lobby)
	{
		return;
	}
	
	if (!EOS.PlatformHandle)
	{
		return;
	}

	LobbyHandle = EOS_Platform_GetLobbyInterface(PlatformHandle);

	if (CurrentLobbyData.currentLobbyIsValid())
	{
		if (CurrentLobbyData.LobbyId.compare(lobby->LobbyId) == 0)
		{
			logInfo("joinLobby: attempting to join current lobby");
			return;
		}
		if (CurrentLobbyData.bAwaitingLeaveCallback)
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

	bool errorOnJoin = false;
	if (CurrentLobbyData.OwnerProductUserId.compare("NULL") == 0)
	{
		// this is unexpected - perhaps an attempt to join a lobby that was freshly abandoned
		ConnectingToLobbyStatus = LobbyHandler_t::EResult_LobbyFailures::LOBBY_NO_OWNER;
		logError("joinLobby: attempting to join a lobby with a NULL owner: %s, aborting.", CurrentLobbyData.LobbyId.c_str());
		errorOnJoin = true;
	}
	else if (lobby->LobbyAttributes.isLobbyLoadingSavedGame != loadingsavegame
		|| lobby->LobbyAttributes.lobbyKey != loadinglobbykey )
	{
		// loading save game, but incorrect assertion from client side.
		if (loadingsavegame == 0)
		{
			// try reload from your other savefiles since this didn't match the default savegameIndex.
			bool foundSave = false;
			int checkDLC = VALID_OK_CHARACTER;
			for (int c = 0; c < SAVE_GAMES_MAX; ++c) {
				auto info = getSaveGameInfo(false, c);
				if (info.game_version != -1) {
					if (info.gamekey == lobby->LobbyAttributes.isLobbyLoadingSavedGame && info.lobbykey == lobby->LobbyAttributes.lobbyKey) {
						if ( info.player_num < info.players.size() )
						{
							checkDLC = info.players[info.player_num].isCharacterValidFromDLC();
							if ( checkDLC != VALID_OK_CHARACTER )
							{
								foundSave = false;
								break;
							}
						}
						savegameCurrentFileIndex = c;
						foundSave = true;
						break;
					}
				}
			}

			if ( checkDLC != VALID_OK_CHARACTER )
			{
				ConnectingToLobbyStatus = LobbyHandler_t::EResult_LobbyFailures::LOBBY_SAVEGAME_REQUIRES_DLC;
				errorOnJoin = true;
			}
			else if (foundSave) {
				loadingsavegame = lobby->LobbyAttributes.isLobbyLoadingSavedGame;
				loadinglobbykey = lobby->LobbyAttributes.lobbyKey;
				auto info = getSaveGameInfo(false, savegameCurrentFileIndex);
				for (int c = 0; c < MAXPLAYERS; ++c) {
					if (info.players_connected[c]) {
						loadGame(c, info);
					}
				}
			}
			else
			{
				ConnectingToLobbyStatus = LobbyHandler_t::EResult_LobbyFailures::LOBBY_USING_SAVEGAME;
				errorOnJoin = true;
			}
		}
		else if (loadingsavegame > 0 && lobby->LobbyAttributes.isLobbyLoadingSavedGame == 0)
		{
			ConnectingToLobbyStatus = LobbyHandler_t::EResult_LobbyFailures::LOBBY_NOT_USING_SAVEGAME;
			errorOnJoin = true;
		}
		else if ( (loadingsavegame > 0 && lobby->LobbyAttributes.isLobbyLoadingSavedGame > 0)
			|| (loadinglobbykey > 0 && lobby->LobbyAttributes.lobbyKey > 0) )
		{
			ConnectingToLobbyStatus = LobbyHandler_t::EResult_LobbyFailures::LOBBY_WRONG_SAVEGAME;
			errorOnJoin = true;
		}
		else
		{
			ConnectingToLobbyStatus = LobbyHandler_t::EResult_LobbyFailures::LOBBY_UNHANDLED_ERROR;
			errorOnJoin = true;
		}
	}
	else if (lobby->LobbyAttributes.gameCurrentLevel >= 0)
	{
		/*if ( lobby->LobbyAttributes.gameCurrentLevel == 0 )
		{
			if (  )
		}
		else
		{
		}*/
		ConnectingToLobbyStatus = LobbyHandler_t::EResult_LobbyFailures::LOBBY_GAME_IN_PROGRESS;
		errorOnJoin = true;
	}

	if (errorOnJoin)
	{
		bConnectingToLobbyWindow = false;
		bConnectingToLobby = false;
		multiplayer = SINGLE;

		LobbyParameters.clearLobbyToJoin();
		LobbyLeaveCleanup(EOS.CurrentLobbyData);
		return;
	}

	EOS_Lobby_JoinLobbyOptions JoinOptions{};
	JoinOptions.ApiVersion = EOS_LOBBY_JOINLOBBY_API_LATEST;
	JoinOptions.LocalUserId = CurrentUserInfo.getProductUserIdHandle();
	JoinOptions.LobbyDetailsHandle = LobbyParameters.lobbyToJoin;
	EOS_Lobby_JoinLobby(LobbyHandle, &JoinOptions, nullptr, OnLobbyJoinCallback);

	LobbyParameters.clearLobbyToJoin();
}

void EOSFuncs::leaveLobby()
{
	//if ( CurrentLobbyData.bAwaitingLeaveCallback )
	//{
	//	// no action needed
	//	logInfo("leaveLobby: attempting to leave lobby with callback already requested, ignoring");
	//	return;
	//}

	// attempt to destroy the lobby if leaving and we are the owner.
	if (CurrentLobbyData.currentUserIsOwner())
	{
		CurrentLobbyData.destroyLobby();
		return;
	}

	if (!EOS.PlatformHandle)
	{
		return;
	}

	LobbyHandle = EOS_Platform_GetLobbyInterface(PlatformHandle);

	EOS_Lobby_LeaveLobbyOptions LeaveOptions{};
	LeaveOptions.ApiVersion = EOS_LOBBY_LEAVELOBBY_API_LATEST;
	LeaveOptions.LocalUserId = CurrentUserInfo.getProductUserIdHandle();
	LeaveOptions.LobbyId = CurrentLobbyData.LobbyId.c_str();

	CurrentLobbyData.bAwaitingLeaveCallback = true;
	EOS_Lobby_LeaveLobby(LobbyHandle, &LeaveOptions, nullptr, OnLobbyLeaveCallback);

}

void EOSFuncs::searchLobbies(LobbyParameters_t::LobbySearchOptions searchType,
	LobbyParameters_t::LobbyJoinOptions joinOptions, EOS_LobbyId lobbyIdToSearch)
{
	LobbySearchResults.lastResultWasFiltered = false;

	if (LobbySearchResults.useLobbyCode)
	{
		for (int c = 0; c < 4 && EOS.lobbySearchByCode[c] != 0; ++c)
		{
			if (EOS.lobbySearchByCode[c] >= 'A' && EOS.lobbySearchByCode[c] <= 'Z')
			{
				EOS.lobbySearchByCode[c] = 'a' + (EOS.lobbySearchByCode[c] - 'A'); // to lowercase.
			}
		}
	}

	if (!EOS.PlatformHandle)
	{
		return;
	}

	LobbyHandle = EOS_Platform_GetLobbyInterface(PlatformHandle);
	logInfo("searchLobbies: starting search");
	EOS_Lobby_CreateLobbySearchOptions CreateSearchOptions{};
	CreateSearchOptions.ApiVersion = EOS_LOBBY_CREATELOBBYSEARCH_API_LATEST;
	CreateSearchOptions.MaxResults = kMaxLobbiesToSearch;

	EOS_HLobbySearch LobbySearch = nullptr;
	if (LobbySearchResults.CurrentLobbySearch != nullptr)
	{
		EOS_LobbySearch_Release(LobbySearchResults.CurrentLobbySearch);
		LobbySearchResults.CurrentLobbySearch = nullptr;
	}

	EOS_EResult result = EOS_Lobby_CreateLobbySearch(LobbyHandle, &CreateSearchOptions, &LobbySearch);
	if (result != EOS_EResult::EOS_Success)
	{
		logError("searchLobbies: EOS_Lobby_CreateLobbySearch failure: %d", static_cast<int>(result));
		return;
	}
	LobbySearchResults.CurrentLobbySearch = LobbySearch;
	for (auto& result : LobbySearchResults.results)
	{
		result.ClearData();
	}
	LobbySearchResults.results.clear();
	LobbySearchResults.resultsSortedForDisplay.clear();

	/*EOS_LobbySearch_SetTargetUserIdOptions SetLobbyOptions{};
	SetLobbyOptions.ApiVersion = EOS_LOBBYSEARCH_SETLOBBYID_API_LATEST;
	SetLobbyOptions.TargetUserId = CurrentUserInfo.Friends.at(0).UserId;
	Result = EOS_LobbySearch_SetTargetUserId(LobbySearch, &SetLobbyOptions);*/
	if (searchType != LobbyParameters_t::LOBBY_SEARCH_BY_LOBBYID)
	{
		EOS_LobbySearch_SetParameterOptions ParamOptions{};
		ParamOptions.ApiVersion = EOS_LOBBYSEARCH_SETPARAMETER_API_LATEST;
		ParamOptions.ComparisonOp = EOS_EComparisonOp::EOS_CO_NOTANYOF;

		EOS_Lobby_AttributeData AttrData;
		AttrData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
		ParamOptions.Parameter = &AttrData;
		AttrData.Key = "VER";
		AttrData.Value.AsUtf8 = "0.0.0";
		AttrData.ValueType = EOS_ELobbyAttributeType::EOS_AT_STRING;
		EOS_EResult resultParameter = EOS_LobbySearch_SetParameter(LobbySearch, &ParamOptions);

		if (LobbySearchResults.useLobbyCode && strcmp(lobbySearchByCode, ""))
		{
			ParamOptions.ComparisonOp = EOS_EComparisonOp::EOS_CO_EQUAL;
			AttrData.Key = "JOINKEY";
			AttrData.Value.AsUtf8 = lobbySearchByCode;
			AttrData.ValueType = EOS_ELobbyAttributeType::EOS_AT_STRING;
			resultParameter = EOS_LobbySearch_SetParameter(LobbySearch, &ParamOptions);
		}
		else
		{
			/*ParamOptions.ComparisonOp = EOS_EComparisonOp::EOS_CO_EQUAL;
			AttrData.Key = "PERMISSIONLEVEL";
			AttrData.Value.AsUtf8 = "0";
			AttrData.ValueType = EOS_ELobbyAttributeType::EOS_AT_STRING;
			resultParameter = EOS_LobbySearch_SetParameter(LobbySearch, &ParamOptions);*/
		}

		ParamOptions.ComparisonOp = EOS_EComparisonOp::EOS_CO_EQUAL;
		AttrData.Key = "MAXPLAYERS";
		AttrData.Value.AsInt64 = MAXPLAYERS;
		AttrData.ValueType = EOS_ELobbyAttributeType::EOS_AT_INT64;
		resultParameter = EOS_LobbySearch_SetParameter(LobbySearch, &ParamOptions);

		if (!LobbySearchResults.showLobbiesInProgress)
		{
			ParamOptions.ComparisonOp = EOS_EComparisonOp::EOS_CO_EQUAL;
			AttrData.Key = "CURRENTLEVEL";
			AttrData.Value.AsUtf8 = "-1";
			AttrData.ValueType = EOS_ELobbyAttributeType::EOS_AT_STRING;
			resultParameter = EOS_LobbySearch_SetParameter(LobbySearch, &ParamOptions);
		}
	}
	else if (searchType == LobbyParameters_t::LOBBY_SEARCH_BY_LOBBYID)
	{
		// appends criteria to search for within the normal search function
		EOS_LobbySearch_SetLobbyIdOptions SetLobbyOptions{};
		SetLobbyOptions.ApiVersion = EOS_LOBBYSEARCH_SETLOBBYID_API_LATEST;
		SetLobbyOptions.LobbyId = lobbyIdToSearch;
		EOS_LobbySearch_SetLobbyId(LobbySearch, &SetLobbyOptions);
	}

	EOS_LobbySearch_FindOptions FindOptions{};
	FindOptions.ApiVersion = EOS_LOBBYSEARCH_FIND_API_LATEST;
	FindOptions.LocalUserId = CurrentUserInfo.getProductUserIdHandle();

	LobbyParameters.clearLobbySearchOptions();
	LobbyParameters.setLobbySearchOptions(searchType, joinOptions);
	EOS_LobbySearch_Find(LobbySearch, &FindOptions, LobbyParameters.lobbySearchOptions, OnLobbySearchFinished);
}

void EOSFuncs::LobbyData_t::destroyLobby()
{
	if (!currentLobbyIsValid())
	{
		EOSFuncs::logError("destroyLobby: invalid current lobby - no ID set");
		return;
	}

	if (!currentUserIsOwner())
	{
		EOSFuncs::logError("destroyLobby: current user is not lobby owner");
		return;
	}

	if (!EOS.PlatformHandle)
	{
		return;
	}

	EOS.LobbyHandle = EOS_Platform_GetLobbyInterface(EOS.PlatformHandle);

	EOS_Lobby_DestroyLobbyOptions DestroyOptions{};
	DestroyOptions.ApiVersion = EOS_LOBBY_DESTROYLOBBY_API_LATEST;
	DestroyOptions.LocalUserId = EOS.CurrentUserInfo.getProductUserIdHandle();
	DestroyOptions.LobbyId = LobbyId.c_str();

	EOS_Lobby_DestroyLobby(EOS.LobbyHandle, &DestroyOptions, nullptr, OnDestroyLobbyFinished);

	EOS.P2PConnectionInfo.resetPeersAndServerData();
	bAwaitingLeaveCallback = false;
	UnsubscribeFromLobbyUpdates();
	ClearData();
}

void EOSFuncs::LobbyData_t::updateLobbyDuringGameLoop()
{
	if (!currentLobbyIsValid())
	{
		return;
	}
	bool doUpdate = false;
	if (LobbyAttributes.gameCurrentLevel != currentlevel)
	{
		doUpdate = true;
	}
	if (LobbyAttributes.serverFlags != svFlags)
	{
		doUpdate = true;
	}

	if (doUpdate)
	{
		updateLobbyForHost(LOBBY_UPDATE_DURING_GAME);
	}
}

void EOSFuncs::LobbyData_t::updateLobby()
{
	if (!EOS.CurrentUserInfo.isValid())
	{
		EOSFuncs::logError("updateLobby: invalid current user");
		return;
	}

	if (!EOS.PlatformHandle)
	{
		return;
	}

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(EOS.PlatformHandle);

	EOS_Lobby_CopyLobbyDetailsHandleOptions CopyHandleOptions{};
	CopyHandleOptions.ApiVersion = EOS_LOBBY_COPYLOBBYDETAILSHANDLE_API_LATEST;
	CopyHandleOptions.LobbyId = LobbyId.c_str();
	CopyHandleOptions.LocalUserId = EOS.CurrentUserInfo.getProductUserIdHandle();

	EOS_HLobbyDetails LobbyDetailsHandle = nullptr;
	EOS_EResult result = EOS_Lobby_CopyLobbyDetailsHandle(LobbyHandle, &CopyHandleOptions, &LobbyDetailsHandle);
	if (result != EOS_EResult::EOS_Success)
	{
		EOSFuncs::logError("OnLobbyUpdateReceived: can't get lobby info handle. Error code: %d", static_cast<int>(result));
		return;
	}

	EOS.setLobbyDetailsFromHandle(LobbyDetailsHandle, this);
	EOS_LobbyDetails_Release(LobbyDetailsHandle);
}


void EOSFuncs::LobbyData_t::getLobbyMemberInfo(EOS_HLobbyDetails LobbyDetails)
{
	if (!currentLobbyIsValid())
	{
		EOSFuncs::logError("getLobbyMemberInfo: invalid current lobby - no ID set");
		return;
	}

	if (!EOS.PlatformHandle)
	{
		return;
	}

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(EOS.PlatformHandle);
	EOS_LobbyDetails_GetMemberCountOptions MemberCountOptions{};
	MemberCountOptions.ApiVersion = EOS_LOBBYDETAILS_GETMEMBERCOUNT_API_LATEST;

	Uint32 numPlayers = EOS_LobbyDetails_GetMemberCount(LobbyDetails, &MemberCountOptions);
	EOSFuncs::logInfo("getLobbyMemberInfo: NumPlayers in lobby: %d", numPlayers);

	// so we don't have to wait for a new callback to retrieve names
	std::unordered_map<EOS_ProductUserId, std::string> previousPlayerNames;
	for (auto& player : playersInLobby)
	{
		EOS_ProductUserId id = EOSFuncs::Helpers_t::productIdFromString(player.memberProductUserId.c_str());
		if (id)
		{
			previousPlayerNames.insert(std::pair<EOS_ProductUserId, std::string>(id, player.name));
		}
	}
	this->playersInLobby.clear();

	EOS_LobbyDetails_GetMemberByIndexOptions MemberByIndexOptions{};
	MemberByIndexOptions.ApiVersion = EOS_LOBBYDETAILS_GETMEMBERBYINDEX_API_LATEST;
	for (Uint32 i = 0; i < numPlayers; ++i)
	{
		MemberByIndexOptions.MemberIndex = i;
		EOS_ProductUserId memberId = EOS_LobbyDetails_GetMemberByIndex(LobbyDetails, &MemberByIndexOptions);
		EOSFuncs::logInfo("getLobbyMemberInfo: Lobby Player ID: %s", EOSFuncs::Helpers_t::shortProductIdToString(memberId).c_str());

		PlayerLobbyData_t newPlayer;
		newPlayer.memberProductUserId = EOSFuncs::Helpers_t::productIdToString(memberId);
		newPlayer.name = "Pending...";
		auto idMapping = EOS.AccountMappings.find(memberId);
		if (idMapping != EOS.AccountMappings.end() && idMapping->second != nullptr)
		{
			newPlayer.memberEpicAccountId = EOSFuncs::Helpers_t::epicIdToString(idMapping->second);
		}

		auto previousPlayer = previousPlayerNames.find(memberId);
		if (previousPlayer != previousPlayerNames.end())
		{
			// replace "pending..." with the player name we previously knew about.
			newPlayer.name = previousPlayer->second;
		}

		//member attributes
		EOS_LobbyDetails_GetMemberAttributeCountOptions MemberAttributeCountOptions{};
		MemberAttributeCountOptions.ApiVersion = EOS_LOBBYDETAILS_GETMEMBERATTRIBUTECOUNT_API_LATEST;
		MemberAttributeCountOptions.TargetUserId = memberId;
		const Uint32 numAttributes = EOS_LobbyDetails_GetMemberAttributeCount(LobbyDetails, &MemberAttributeCountOptions);
		for (Uint32 j = 0; j < numAttributes; ++j)
		{
			EOS_LobbyDetails_CopyMemberAttributeByIndexOptions MemberAttributeCopyOptions{};
			MemberAttributeCopyOptions.ApiVersion = EOS_LOBBYDETAILS_COPYMEMBERATTRIBUTEBYINDEX_API_LATEST;
			MemberAttributeCopyOptions.AttrIndex = j;
			MemberAttributeCopyOptions.TargetUserId = memberId;
			EOS_Lobby_Attribute* MemberAttribute = nullptr;
			EOS_EResult result = EOS_LobbyDetails_CopyMemberAttributeByIndex(LobbyDetails, &MemberAttributeCopyOptions, &MemberAttribute);
			if (result != EOS_EResult::EOS_Success)
			{
				EOSFuncs::logError("getLobbyMemberInfo: can't copy member attribute, error code: %d",
					static_cast<int>(result));
				continue;
			}

			std::string key = MemberAttribute->Data->Key;
			if (key.compare("CLIENTNUM") == 0)
			{
				newPlayer.clientNumber = static_cast<int>(MemberAttribute->Data->Value.AsInt64);
				EOSFuncs::logInfo("Read clientnum: %d for user: %s", newPlayer.clientNumber, newPlayer.memberProductUserId.c_str());
			}
			EOS_Lobby_Attribute_Release(MemberAttribute);
		}


		this->playersInLobby.push_back(newPlayer);
		this->lobbyMembersQueueToMappingUpdate.push_back(memberId);
	}

	EOS.queryAccountIdFromProductId(this);
}

void EOSFuncs::queryLocalExternalAccountId(EOS_EExternalAccountType accountType)
{
	EOS_Connect_QueryProductUserIdMappingsOptions QueryOptions{};
	QueryOptions.ApiVersion = EOS_CONNECT_QUERYPRODUCTUSERIDMAPPINGS_API_LATEST;
	QueryOptions.AccountIdType_DEPRECATED = accountType;
	QueryOptions.LocalUserId = CurrentUserInfo.getProductUserIdHandle();

	std::vector<EOS_ProductUserId> ids = { EOS.CurrentUserInfo.getProductUserIdHandle() };
	QueryOptions.ProductUserIdCount = ids.size();
	QueryOptions.ProductUserIds = ids.data();

	for (EOS_ProductUserId& id : ids)
	{
		auto findExternalAccounts = ExternalAccountMappings.find(id);
		if (findExternalAccounts == ExternalAccountMappings.end())
		{
			// if the mapping doesn't exist, add to set. otherwise we already know the account id for the given product id
			ProductIdsAwaitingAccountMappingCallback.insert(id);
		}
		else
		{
			// kick off the user info query since we know the data for the external account
			getExternalAccountUserInfo(id, EOSFuncs::USER_INFO_QUERY_LOCAL);
		}
	}
	if (!EOS.PlatformHandle)
	{
		return;
	}
	EOS_HConnect ConnectHandle = EOS_Platform_GetConnectInterface(PlatformHandle);
	EOS_Connect_QueryProductUserIdMappings(ConnectHandle, &QueryOptions, nullptr, OnQueryAccountMappingsCallback);
}

void EOSFuncs::queryAccountIdFromProductId(LobbyData_t* lobby/*, std::vector<EOS_ProductUserId>& accountsToQuery*/)
{
	if (!lobby)
	{
		return;
	}
	if (lobby->lobbyMembersQueueToMappingUpdate.empty())
	{
		return;
	}
	EOS_Connect_QueryProductUserIdMappingsOptions QueryOptions{};
	QueryOptions.ApiVersion = EOS_CONNECT_QUERYPRODUCTUSERIDMAPPINGS_API_LATEST;
	//QueryOptions.AccountIdType_DEPRECATED = EOS_EExternalAccountType::EOS_EAT_EPIC;
	QueryOptions.LocalUserId = CurrentUserInfo.getProductUserIdHandle();

	QueryOptions.ProductUserIdCount = lobby->lobbyMembersQueueToMappingUpdate.size();
	QueryOptions.ProductUserIds = lobby->lobbyMembersQueueToMappingUpdate.data();

	for (EOS_ProductUserId& id : lobby->lobbyMembersQueueToMappingUpdate)
	{
		auto findEpicAccounts = AccountMappings.find(id);
		auto findExternalAccounts = ExternalAccountMappings.find(id);
		if (findEpicAccounts == AccountMappings.end() || (*findEpicAccounts).second == nullptr)
		{
			if (findExternalAccounts == ExternalAccountMappings.end())
			{
				// if the mapping doesn't exist, add to set. otherwise we already know the account id for the given product id
				ProductIdsAwaitingAccountMappingCallback.insert(id);
			}
			else
			{
				// kick off the user info query since we know the data for the external account
				getExternalAccountUserInfo(id, EOSFuncs::USER_INFO_QUERY_LOBBY_MEMBER);
			}
		}
		else
		{
			// kick off the user info query since we know the data.
			getUserInfo(AccountMappings[id], EOSFuncs::USER_INFO_QUERY_LOBBY_MEMBER, 0);
		}
	}

	if (!EOS.PlatformHandle)
	{
		return;
	}

	EOS_HConnect ConnectHandle = EOS_Platform_GetConnectInterface(PlatformHandle);
	EOS_Connect_QueryProductUserIdMappings(ConnectHandle, &QueryOptions, nullptr, OnQueryAccountMappingsCallback);

	lobby->lobbyMembersQueueToMappingUpdate.clear();
}

std::pair<std::string, std::string> EOSFuncs::LobbyData_t::getAttributePair(AttributeTypes type)
{
	//char buffer[128] = "";
	std::pair<std::string, std::string> attributePair;
	attributePair.first = "empty";
	attributePair.second = "empty";
	switch (type)
	{
	case LOBBY_NAME:
		attributePair.first = "NAME";
		attributePair.second = this->LobbyAttributes.lobbyName;
		break;
	case GAME_VERSION:
		attributePair.first = "VER";
		attributePair.second = this->LobbyAttributes.gameVersion;
		break;
	case GAME_MAXPLAYERS:
		attributePair.first = "MAXPLAYERS";
		attributePair.second = "";
		break;
	case SERVER_FLAGS:
		attributePair.first = "SVFLAGS";
		char svFlagsChar[32];
		snprintf(svFlagsChar, 31, "%d", this->LobbyAttributes.serverFlags);
		attributePair.second = svFlagsChar;
		break;
	case LOADING_SAVEGAME:
		attributePair.first = "LOADINGSAVEGAME";
		attributePair.second = std::to_string(this->LobbyAttributes.isLobbyLoadingSavedGame);
		break;
	case LOBBY_KEY:
		attributePair.first = "LOADINGLOBBYKEY";
		attributePair.second = std::to_string(this->LobbyAttributes.lobbyKey);
		break;
	case CHALLENGE_LID:
		attributePair.first = "CHALLENGELID";
		attributePair.second = this->LobbyAttributes.challengeLid;
		break;
	case GAME_MODS:
		attributePair.first = "SVNUMMODS";
		attributePair.second = std::to_string(this->LobbyAttributes.numServerMods);
		break;
	case GAME_MODS_DISABLE_ACHIEVEMENTS:
		attributePair.first = "SVMODDISABLEACH";
		attributePair.second = this->LobbyAttributes.modsDisableAchievements ? "1" : "0";
		break;
	case CREATION_TIME:
		attributePair.first = "CREATETIME";
		attributePair.second = std::to_string(this->LobbyAttributes.lobbyCreationTime);
		break;
	case GAME_CURRENT_LEVEL:
		attributePair.first = "CURRENTLEVEL";
		attributePair.second = std::to_string(this->LobbyAttributes.gameCurrentLevel);
		break;
	case GAME_JOIN_KEY:
		attributePair.first = "JOINKEY";
		attributePair.second = this->LobbyAttributes.gameJoinKey;
		break;
	case LOBBY_PERMISSION_LEVEL:
		attributePair.first = "PERMISSIONLEVEL";
		char permissionLevel[32];
		snprintf(permissionLevel, sizeof(permissionLevel), "%d", this->LobbyAttributes.PermissionLevel);
		attributePair.second = permissionLevel;
		break;
	case FRIENDS_ONLY:
		attributePair.first = "FRIENDSONLY";
		attributePair.second = this->LobbyAttributes.friendsOnly ? "true" : "false";
		break;
	default:
		break;
	}
	return attributePair;
}

void EOSFuncs::LobbyData_t::setLobbyAttributesAfterReading(EOS_Lobby_AttributeData* data)
{
	std::string keyName = data->Key;
	if (keyName.compare("NAME") == 0)
	{
		this->LobbyAttributes.lobbyName = data->Value.AsUtf8;
	}
	else if (keyName.compare("VER") == 0)
	{
		this->LobbyAttributes.gameVersion = data->Value.AsUtf8;
	}
	else if (keyName.compare("MAXPLAYERS") == 0)
	{
		this->LobbyAttributes.maxplayersCompatible = data->Value.AsInt64;
	}
	else if (keyName.compare("SVFLAGS") == 0)
	{
		this->LobbyAttributes.serverFlags = std::stoi(data->Value.AsUtf8);
	}
	else if (keyName.compare("LOADINGSAVEGAME") == 0)
	{
		this->LobbyAttributes.isLobbyLoadingSavedGame = std::stoul(data->Value.AsUtf8);
	}
	else if ( keyName.compare("LOADINGLOBBYKEY") == 0 )
	{
		this->LobbyAttributes.lobbyKey = std::stoul(data->Value.AsUtf8);
	}
	else if ( keyName.compare("CHALLENGELID") == 0 )
	{
		this->LobbyAttributes.challengeLid = data->Value.AsUtf8;
	}
	else if (keyName.compare("SVNUMMODS") == 0)
	{
		this->LobbyAttributes.numServerMods = std::stoi(data->Value.AsUtf8);
	}
	else if (keyName.compare("SVMODDISABLEACH") == 0)
	{
		this->LobbyAttributes.modsDisableAchievements = std::stoi(data->Value.AsUtf8) > 0 ? 1 : 0;
	}
	else if (keyName.compare("CREATETIME") == 0)
	{
		this->LobbyAttributes.lobbyCreationTime = std::stoll(data->Value.AsUtf8);
	}
	else if (keyName.compare("CURRENTLEVEL") == 0)
	{
		this->LobbyAttributes.gameCurrentLevel = std::stoi(data->Value.AsUtf8);
	}
	else if (keyName.compare("JOINKEY") == 0)
	{
		this->LobbyAttributes.gameJoinKey = data->Value.AsUtf8;
	}
	else if (keyName.compare("PERMISSIONLEVEL") == 0)
	{
		this->LobbyAttributes.PermissionLevel = std::stoi(data->Value.AsUtf8);
	}
	else if (keyName.compare("FRIENDSONLY") == 0)
	{
		this->LobbyAttributes.friendsOnly = strcmp(data->Value.AsUtf8, "true") == 0;
	}
}

EOS_ProductUserId EOSFuncs::P2PConnectionInfo_t::getPeerIdFromIndex(int index) const
{
	if (index == 0 && multiplayer == CLIENT)
	{
		return serverProductId;
	}

	for (auto& pair : peerProductIds)
	{
		if (pair.second == index)
		{
			return pair.first;
		}
	}
	//logError("getPeerIdFromIndex: no peer with index: %d", index);
	return nullptr;
}

int EOSFuncs::P2PConnectionInfo_t::getIndexFromPeerId(EOS_ProductUserId id) const
{
	if (!id)
	{
		return 0;
	}
	for (auto& pair : peerProductIds)
	{
		if (pair.first == id)
		{
			return pair.second;
		}
	}
	logError("getPeerIdFromIndex: no peer with id: %s", EOSFuncs::Helpers_t::productIdToString(id));
	return 0;
}

bool EOSFuncs::P2PConnectionInfo_t::isPeerIndexed(EOS_ProductUserId id)
{
	if (id == serverProductId)
	{
		return true;
	}
	for (auto& pair : peerProductIds)
	{
		if (pair.first == id)
		{
			return true;
		}
	}
	return false;
}

bool EOSFuncs::P2PConnectionInfo_t::assignPeerIndex(EOS_ProductUserId id, int index)
{
	for (auto& pair : peerProductIds)
	{
		if (pair.first == id)
		{
			pair.second = index;
			return true;
		}
	}
	return false;
}

bool EOSFuncs::P2PConnectionInfo_t::isPeerStillValid(int index) const
{
	if (!getPeerIdFromIndex(index))
	{
		return false;
	}
	if (!Helpers_t::productIdIsValid(getPeerIdFromIndex(index)))
	{
		return false;
	}
	return true;
}

void EOSFuncs::P2PConnectionInfo_t::resetPeersAndServerData()
{
	if (!EOS.PlatformHandle)
	{
		return;
	}

	EOS_P2P_CloseConnectionsOptions closeOptions{};
	closeOptions.ApiVersion = EOS_P2P_CLOSECONNECTIONS_API_LATEST;
	closeOptions.LocalUserId = EOS.CurrentUserInfo.getProductUserIdHandle();
	EOS_P2P_SocketId SocketId{};
	SocketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
	strncpy(SocketId.SocketName, "CHAT", 5);
	closeOptions.SocketId = &SocketId;

	EOS_EResult result = EOS_P2P_CloseConnections(EOS_Platform_GetP2PInterface(EOS.PlatformHandle), &closeOptions);
	EOSFuncs::logInfo("resetPeersAndServerData: closing P2P connections, result: %d", static_cast<int>(result));

	peerProductIds.clear();
	serverProductId = nullptr;
}

void EOSFuncs::LobbyData_t::SubscribeToLobbyUpdates()
{
	UnsubscribeFromLobbyUpdates();

	if (!EOS.PlatformHandle)
	{
		return;
	}

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(EOS.PlatformHandle);

	EOS_Lobby_AddNotifyLobbyUpdateReceivedOptions UpdateNotifyOptions{};
	UpdateNotifyOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYUPDATERECEIVED_API_LATEST;

	LobbyUpdateNotification = EOS_Lobby_AddNotifyLobbyUpdateReceived(LobbyHandle, &UpdateNotifyOptions, nullptr, OnLobbyUpdateReceived);

	EOS_Lobby_AddNotifyLobbyMemberUpdateReceivedOptions MemberUpdateOptions{};
	MemberUpdateOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYMEMBERUPDATERECEIVED_API_LATEST;
	LobbyMemberUpdateNotification = EOS_Lobby_AddNotifyLobbyMemberUpdateReceived(LobbyHandle, &MemberUpdateOptions, nullptr, OnMemberUpdateReceived);

	EOS_Lobby_AddNotifyLobbyMemberStatusReceivedOptions MemberStatusOptions{};
	MemberStatusOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYMEMBERSTATUSRECEIVED_API_LATEST;
	LobbyMemberStatusNotification = EOS_Lobby_AddNotifyLobbyMemberStatusReceived(LobbyHandle, &MemberStatusOptions, nullptr, OnMemberStatusReceived);

	EOSFuncs::logInfo("SubscribeToLobbyUpdates: %s", this->LobbyId.c_str());
}

void EOSFuncs::LobbyData_t::UnsubscribeFromLobbyUpdates()
{
	if (LobbyUpdateNotification != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(EOS.PlatformHandle);

		EOS_Lobby_RemoveNotifyLobbyUpdateReceived(LobbyHandle, LobbyUpdateNotification);
		LobbyUpdateNotification = EOS_INVALID_NOTIFICATIONID;
	}

	if (LobbyMemberUpdateNotification != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(EOS.PlatformHandle);

		EOS_Lobby_RemoveNotifyLobbyMemberUpdateReceived(LobbyHandle, LobbyMemberUpdateNotification);
		LobbyMemberUpdateNotification = EOS_INVALID_NOTIFICATIONID;
	}

	if (LobbyMemberStatusNotification != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(EOS.PlatformHandle);

		EOS_Lobby_RemoveNotifyLobbyMemberStatusReceived(LobbyHandle, LobbyMemberStatusNotification);
		LobbyMemberStatusNotification = EOS_INVALID_NOTIFICATIONID;
	}

	EOSFuncs::logInfo("UnsubscribeFromLobbyUpdates: %s", this->LobbyId.c_str());
}

void EOS_CALL EOSFuncs::OnAchievementQueryComplete(const EOS_Achievements_OnQueryDefinitionsCompleteCallbackInfo* data)
{
	assert(data != NULL);

	EOS.Achievements.definitionsAwaitingCallback = false;

	if (data->ResultCode != EOS_EResult::EOS_Success)
	{
		logError("OnAchievementQueryComplete: Failed to load achievements");
		return;
	}

	EOS_Achievements_GetAchievementDefinitionCountOptions AchievementDefinitionsCountOptions{};
	AchievementDefinitionsCountOptions.ApiVersion = EOS_ACHIEVEMENTS_GETACHIEVEMENTDEFINITIONCOUNT_API_LATEST;

	uint32_t AchievementDefinitionsCount = EOS_Achievements_GetAchievementDefinitionCount(EOS.AchievementsHandle, &AchievementDefinitionsCountOptions);

	EOS_Achievements_CopyAchievementDefinitionV2ByIndexOptions CopyOptions{};
	CopyOptions.ApiVersion = EOS_ACHIEVEMENTS_COPYDEFINITIONV2BYACHIEVEMENTID_API_LATEST;

	for (CopyOptions.AchievementIndex = 0; CopyOptions.AchievementIndex < AchievementDefinitionsCount; ++CopyOptions.AchievementIndex)
	{
		EOS_Achievements_DefinitionV2* AchievementDef = NULL;

		EOS_EResult CopyAchievementDefinitionsResult = EOS_Achievements_CopyAchievementDefinitionV2ByIndex(EOS.AchievementsHandle, &CopyOptions, &AchievementDef);
		if (CopyAchievementDefinitionsResult != EOS_EResult::EOS_Success)
		{
			logError("CopyAchievementDefinitions Failure!");
			return;
		}

		if (AchievementDef->bIsHidden)
		{
			//achievementHidden.emplace(std::string(AchievementDef->AchievementId));
		}

		if (AchievementDef->UnlockedDisplayName)
		{
			/*achievementNames.emplace(std::make_pair(
				std::string(AchievementDef->AchievementId),
				std::string(AchievementDef->UnlockedDisplayName)));*/
		}

		if (AchievementDef->UnlockedDescription)
		{
			//auto result = achievementDesc.emplace(std::make_pair(
			//	std::string(AchievementDef->AchievementId),
			//	std::string(AchievementDef->UnlockedDescription)));
			//if (result.second == true) // insertion success
			//{
			//	if (result.first->second.back() != '.') // add punctuation if missing.
			//	{
			//		result.first->second += '.';
			//	}
			//}
		}

		// Release Achievement Definition
		EOS_Achievements_DefinitionV2_Release(AchievementDef);
	}

	EOS.Achievements.definitionsLoaded = true;
	sortAchievementsForDisplay();

	logInfo("Successfully loaded achievements");
}

void EOSFuncs::OnPlayerAchievementQueryComplete(const EOS_Achievements_OnQueryPlayerAchievementsCompleteCallbackInfo* data)
{
	assert(data != NULL);

	EOS.Achievements.playerDataAwaitingCallback = false;

	if (data->ResultCode != EOS_EResult::EOS_Success)
	{
		logError("OnPlayerAchievementQueryComplete: Failed to load player achievement status");
		return;
	}

	EOS_Achievements_GetPlayerAchievementCountOptions AchievementsCountOptions{};
	AchievementsCountOptions.ApiVersion = EOS_ACHIEVEMENTS_GETPLAYERACHIEVEMENTCOUNT_API_LATEST;
	AchievementsCountOptions.UserId = EOS.CurrentUserInfo.getProductUserIdHandle();

	uint32_t AchievementsCount = EOS_Achievements_GetPlayerAchievementCount(EOS.AchievementsHandle, &AchievementsCountOptions);

	EOS_Achievements_CopyPlayerAchievementByIndexOptions CopyOptions{};
	CopyOptions.ApiVersion = EOS_ACHIEVEMENTS_COPYPLAYERACHIEVEMENTBYINDEX_API_LATEST;
	CopyOptions.TargetUserId = EOS.CurrentUserInfo.getProductUserIdHandle();
	CopyOptions.LocalUserId = EOS.CurrentUserInfo.getProductUserIdHandle();

	for (CopyOptions.AchievementIndex = 0; CopyOptions.AchievementIndex < AchievementsCount; ++CopyOptions.AchievementIndex)
	{
		EOS_Achievements_PlayerAchievement* PlayerAchievement = NULL;

		EOS_EResult CopyPlayerAchievementResult = EOS_Achievements_CopyPlayerAchievementByIndex(EOS.AchievementsHandle, &CopyOptions, &PlayerAchievement);
		if (CopyPlayerAchievementResult == EOS_EResult::EOS_Success)
		{
			if (PlayerAchievement->UnlockTime != EOS_ACHIEVEMENTS_ACHIEVEMENT_UNLOCKTIME_UNDEFINED)
			{
				Compendium_t::AchievementData_t::achievementUnlockedLookup.insert(PlayerAchievement->AchievementId);
				Compendium_t::achievements[PlayerAchievement->AchievementId].unlockTime = PlayerAchievement->UnlockTime;
				auto& achData = Compendium_t::achievements[PlayerAchievement->AchievementId];
				achData.unlocked = true;
				achData.unlockTime = PlayerAchievement->UnlockTime;
			}

			if (PlayerAchievement->StatInfoCount > 0)
			{
				/*for (int statNum = 0; statNum < NUM_STEAM_STATISTICS; ++statNum)
				{
					if (steamStatAchStringsAndMaxVals[statNum].first.compare(PlayerAchievement->AchievementId) == 0)
					{
						Compendium_t::achievements[PlayerAchievement->AchievementId].achievementProgress = statNum;
						break;
					}
				}*/
			}
		}
		EOS_Achievements_PlayerAchievement_Release(PlayerAchievement);
	}

	Compendium_t::AchievementData_t::achievementsNeedFirstData = false;
	EOS.Achievements.playerDataLoaded = true;
	sortAchievementsForDisplay();

	logInfo("OnPlayerAchievementQueryComplete: success");
}

void EOSFuncs::OnIngestStatComplete(const EOS_Stats_IngestStatCompleteCallbackInfo* data)
{
	assert(data != NULL);
	if (data->ResultCode == EOS_EResult::EOS_Success)
	{
		logInfo("Stats updated");
	}
	else
	{
		logError("OnIngestStatComplete: Callback failure: %d", static_cast<int>(data->ResultCode));
	}
}

bool EOSFuncs::initAchievements()
{
	logInfo("Initializing EOS achievements");
	if (!PlatformHandle)
	{
		return false;
	}
	if ((AchievementsHandle = EOS_Platform_GetAchievementsInterface(PlatformHandle)) == nullptr)
	{
		return false;
	}
	if ((StatsHandle = EOS_Platform_GetStatsInterface(PlatformHandle)) == nullptr)
	{
		return false;
	}
	return true;
}

void EOS_CALL EOSFuncs::OnUnlockAchievement(const EOS_Achievements_OnUnlockAchievementsCompleteCallbackInfo* data)
{
	assert(data != NULL);
	//int64_t t = getTime();
	//achievementUnlockTime.emplace(std::make_pair(std::string((const char*)data->ClientData), t));
	if (data->ResultCode == EOS_EResult::EOS_Success)
	{
		logInfo("EOS achievement successfully unlocked");
		return;
	}
	else
	{
		logError("OnUnlockAchievement: Callback failure: %d", static_cast<int>(data->ResultCode));
		return;
	}
}

void EOSFuncs::loadAchievementData()
{
	if (Achievements.definitionsAwaitingCallback || Achievements.playerDataAwaitingCallback)
	{
		return;
	}
	if (!CurrentUserInfo.isValid() || !CurrentUserInfo.isLoggedIn())
	{
		return;
	}

	Achievements.bAchievementsInit = true;

	logInfo("Loading EOS achievements");

	if (!Achievements.definitionsLoaded)
	{
		Achievements.definitionsAwaitingCallback = true;
		//achievementNames.clear();
		//achievementDesc.clear();
		//achievementHidden.clear();
		EOS_Achievements_QueryDefinitionsOptions Options{};
		Options.ApiVersion = EOS_ACHIEVEMENTS_QUERYDEFINITIONS_API_LATEST;
		//Options.EpicUserId = EOSFuncs::Helpers_t::epicIdFromString(EOS.CurrentUserInfo.epicAccountId.c_str());
		Options.LocalUserId = CurrentUserInfo.getProductUserIdHandle();
		//Options.HiddenAchievementsCount = 0;
		//Options.HiddenAchievementIds = nullptr;
		EOS_Achievements_QueryDefinitions(AchievementsHandle, &Options, nullptr, OnAchievementQueryComplete);

		EOS.StatGlobalManager.queryGlobalStatUser();
	}

	Achievements.playerDataAwaitingCallback = true;
	//achievementProgress.clear();
	//achievementUnlockTime.clear();
	EOS_Achievements_QueryPlayerAchievementsOptions PlayerAchievementOptions{};
	PlayerAchievementOptions.ApiVersion = EOS_ACHIEVEMENTS_QUERYPLAYERACHIEVEMENTS_API_LATEST;
	PlayerAchievementOptions.LocalUserId = CurrentUserInfo.getProductUserIdHandle();
	PlayerAchievementOptions.TargetUserId = CurrentUserInfo.getProductUserIdHandle();
	EOS_Achievements_QueryPlayerAchievements(AchievementsHandle, &PlayerAchievementOptions, nullptr, OnPlayerAchievementQueryComplete);

	EOS.queryAllStats();
}

void EOSFuncs::unlockAchievement(const char* name)
{
	//logInfo("unlocking EOS achievement '%s'", name);
	EOS_Achievements_UnlockAchievementsOptions UnlockAchievementsOptions{};
	UnlockAchievementsOptions.ApiVersion = EOS_ACHIEVEMENTS_UNLOCKACHIEVEMENTS_API_LATEST;
	UnlockAchievementsOptions.UserId = CurrentUserInfo.getProductUserIdHandle();
	UnlockAchievementsOptions.AchievementsCount = 1;
	UnlockAchievementsOptions.AchievementIds = &name;
	EOS_Achievements_UnlockAchievements(AchievementsHandle, &UnlockAchievementsOptions, nullptr, OnUnlockAchievement);
	UIToastNotificationManager.createAchievementNotification(name);
}

void EOSFuncs::ingestStat(int stat_num, int value)
{
	//logInfo("updating EOS stat '%s'", g_SteamStats[stat_num].m_pchStatName);

	StatsHandle = EOS_Platform_GetStatsInterface(PlatformHandle);

	EOS_Stats_IngestData StatsToIngest[1];
	StatsToIngest[0].ApiVersion = EOS_STATS_INGESTDATA_API_LATEST;
	StatsToIngest[0].StatName = g_SteamStats[stat_num].m_pchStatName;
	StatsToIngest[0].IngestAmount = value;

	EOS_Stats_IngestStatOptions Options{};
	Options.ApiVersion = EOS_STATS_INGESTSTAT_API_LATEST;
	Options.Stats = StatsToIngest;
	Options.StatsCount = sizeof(StatsToIngest) / sizeof(StatsToIngest[0]);
	Options.LocalUserId = CurrentUserInfo.getProductUserIdHandle();
	Options.TargetUserId = CurrentUserInfo.getProductUserIdHandle();
	EOS_Stats_IngestStat(StatsHandle, &Options, nullptr, OnIngestStatComplete);
}

static void EOS_CALL OnIngestGlobalStatComplete(const EOS_Stats_IngestStatCompleteCallbackInfo* data)
{
	/*assert(data != NULL);
	if (data->ResultCode == EOS_EResult::EOS_Success)
	{
		EOSFuncs::logInfo("Successfully stored global stats");
		for (Uint32 i = 0; i < NUM_GLOBAL_STEAM_STATISTICS; ++i)
		{
			g_SteamGlobalStats[i].m_iValue = 0;
		}
		return;
	}
	else if (data->ResultCode == EOS_EResult::EOS_TooManyRequests)
	{
		return;
	}
	else
	{
		EOSFuncs::logError("OnIngestGlobalStatComplete: Callback failure: %d", static_cast<int>(data->ResultCode));
	}
	EOS.StatGlobalManager.bDataQueued = true;*/
}

void EOSFuncs::queueGlobalStatUpdate(int stat_num, int value)
{
	/*if (stat_num <= STEAM_GSTAT_INVALID || stat_num >= NUM_GLOBAL_STEAM_STATISTICS)
	{
		return;
	}
	if (StatGlobalManager.bIsDisabled)
	{
		return;
	}
	g_SteamGlobalStats[stat_num].m_iValue += value;
	StatGlobalManager.bDataQueued = true;*/
}

void EOSFuncs::StatGlobal_t::updateQueuedStats()
{
	if (!bDataQueued || bIsDisabled)
	{
		return;
	}
	if ((ticks - lastUpdateTicks) < TICKS_PER_SECOND * 30)
	{
		return;
	}
	EOS.ingestGlobalStats();
	lastUpdateTicks = ticks;
	bDataQueued = false;
}

void EOSFuncs::ingestGlobalStats()
{
	//if (StatGlobalManager.bIsDisabled)
	//{
	//	return;
	//}
	//if (!ServerPlatformHandle)
	//{
	//	return;
	//}

	//Uint32 numStats = 0;
	//std::vector<std::string> StatNames;
	//for (Uint32 i = 0; i < NUM_GLOBAL_STEAM_STATISTICS; ++i)
	//{
	//	if (g_SteamGlobalStats[i].m_iValue > 0)
	//	{
	//		StatNames.push_back(g_SteamGlobalStats[i].m_pchStatName);
	//		++numStats;
	//	}
	//}

	//if (numStats == 0)
	//{
	//	return;
	//}

	//EOS_Stats_IngestData* StatsToIngest = new EOS_Stats_IngestData[numStats];
	//Uint32 currentIndex = 0;
	//for (Uint32 i = 0; i < NUM_GLOBAL_STEAM_STATISTICS && currentIndex < StatNames.size(); ++i)
	//{
	//	if (g_SteamGlobalStats[i].m_iValue > 0)
	//	{
	//		StatsToIngest[currentIndex].ApiVersion = EOS_STATS_INGESTDATA_API_LATEST;
	//		StatsToIngest[currentIndex].StatName = StatNames[currentIndex].c_str();
	//		StatsToIngest[currentIndex].IngestAmount = g_SteamGlobalStats[i].m_iValue;
	//		//logInfo("Updated %s | %d", StatsToIngest[currentIndex].StatName, StatsToIngest[currentIndex].IngestAmount);
	//		++currentIndex;
	//	}
	//}

	//EOS_Stats_IngestStatOptions Options{};
	//Options.ApiVersion = EOS_STATS_INGESTSTAT_API_LATEST;
	//Options.Stats = StatsToIngest;
	//Options.StatsCount = numStats;
	//Options.LocalUserId = StatGlobalManager.getProductUserIdHandle();
	//Options.TargetUserId = StatGlobalManager.getProductUserIdHandle();

	//EOS_Stats_IngestStat(EOS_Platform_GetStatsInterface(ServerPlatformHandle), &Options, nullptr, OnIngestGlobalStatComplete);

	//delete[] StatsToIngest;
}

void EOS_CALL EOSFuncs::OnQueryAllStatsCallback(const EOS_Stats_OnQueryStatsCompleteCallbackInfo* data)
{
	if (!data)
	{
		EOSFuncs::logError("OnQueryAllStatsCallback: null data");
		return;
	}
	else if (data->ResultCode == EOS_EResult::EOS_Success)
	{

		EOS.StatsHandle = EOS_Platform_GetStatsInterface(EOS.PlatformHandle);
		EOS_Stats_GetStatCountOptions StatCountOptions{};
		StatCountOptions.ApiVersion = EOS_STATS_GETSTATCOUNT_API_LATEST;
		StatCountOptions.TargetUserId = EOS.CurrentUserInfo.getProductUserIdHandle();

		Uint32 numStats = EOS_Stats_GetStatsCount(EOS.StatsHandle, &StatCountOptions);

		EOSFuncs::logInfo("OnQueryAllStatsCallback: read %d stats", numStats);

		EOS_Stats_CopyStatByIndexOptions CopyByIndexOptions{};
		CopyByIndexOptions.ApiVersion = EOS_STATS_COPYSTATBYINDEX_API_LATEST;
		CopyByIndexOptions.TargetUserId = EOS.CurrentUserInfo.getProductUserIdHandle();

		EOS_Stats_Stat* copyStat = NULL;

		for (Uint32 i = 0; i < NUM_STEAM_STATISTICS; ++i)
		{
			g_SteamStats[i].m_iValue = 0;
		}

		for (CopyByIndexOptions.StatIndex = 0; CopyByIndexOptions.StatIndex < numStats; ++CopyByIndexOptions.StatIndex)
		{
			EOS_EResult result = EOS_Stats_CopyStatByIndex(EOS.StatsHandle, &CopyByIndexOptions, &copyStat);
			if (result == EOS_EResult::EOS_Success && copyStat)
			{
				SteamStat_t* statLookup = nullptr;
				if (statLookup = EOS.getStatStructFromString(std::string(copyStat->Name)))
				{
					statLookup->m_iValue = copyStat->Value;
				}
				//EOSFuncs::logInfo("OnQueryAllStatsCallback: stat %s | %d", copyStat->Name, copyStat->Value);
				EOS_Stats_Stat_Release(copyStat);
			}
		}
	}
	else
	{
		EOSFuncs::logError("OnQueryAllStatsCallback: Callback failure: %d", static_cast<int>(data->ResultCode));
	}
}

SteamStat_t* EOSFuncs::getStatStructFromString(const std::string& str)
{
	if (!statMappings.size())
	{
		EOSFuncs::logError("getStatStructFromString: Empty stat mappings");
		return nullptr;
	}

	auto find = statMappings.find(str);
	if (find != statMappings.end())
	{
		return (*find).second;
	}
	return nullptr;
}

void EOSFuncs::queryAllStats()
{
	if (!statMappings.size())
	{
		for (Uint32 i = 0; i < NUM_STEAM_STATISTICS; ++i)
		{
			statMappings[g_SteamStats[i].m_pchStatName] = &g_SteamStats[i];
		}
	}

	StatsHandle = EOS_Platform_GetStatsInterface(PlatformHandle);

	// Query Player Stats
	EOS_Stats_QueryStatsOptions StatsQueryOptions{};
	StatsQueryOptions.ApiVersion = EOS_STATS_QUERYSTATS_API_LATEST;
	StatsQueryOptions.TargetUserId = CurrentUserInfo.getProductUserIdHandle();
	StatsQueryOptions.LocalUserId = CurrentUserInfo.getProductUserIdHandle();

	// Optional params
	StatsQueryOptions.StartTime = EOS_STATS_TIME_UNDEFINED;
	StatsQueryOptions.EndTime = EOS_STATS_TIME_UNDEFINED;

	StatsQueryOptions.StatNamesCount = NUM_STEAM_STATISTICS;
	StatsQueryOptions.StatNames = new const char* [NUM_STEAM_STATISTICS];

	for (int i = 0; i < NUM_STEAM_STATISTICS; ++i)
	{
		StatsQueryOptions.StatNames[i] = g_SteamStats[i].m_pchStatName;
	}

	EOS_Stats_QueryStats(StatsHandle, &StatsQueryOptions, nullptr, OnQueryAllStatsCallback);
	delete[] StatsQueryOptions.StatNames;
}

void EOSFuncs::showFriendsOverlay()
{
	UIHandle = EOS_Platform_GetUIInterface(PlatformHandle);
	EOS_UI_SetDisplayPreferenceOptions DisplayOptions{};
	DisplayOptions.ApiVersion = EOS_UI_SETDISPLAYPREFERENCE_API_LATEST;
	DisplayOptions.NotificationLocation = EOS_UI_ENotificationLocation::EOS_UNL_TopRight;

	EOS_EResult result = EOS_UI_SetDisplayPreference(UIHandle, &DisplayOptions);
	EOSFuncs::logInfo("showFriendsOverlay: result: %d", static_cast<int>(result));

	UIHandle = EOS_Platform_GetUIInterface(PlatformHandle);
	EOS_UI_ShowFriendsOptions Options{};
	Options.ApiVersion = EOS_UI_SHOWFRIENDS_API_LATEST;
	Options.LocalUserId = EOSFuncs::Helpers_t::epicIdFromString(CurrentUserInfo.epicAccountId.c_str());

	EOS_UI_ShowFriends(UIHandle, &Options, nullptr, ShowFriendsCallback);
}

void EOS_CALL EOSFuncs::ShowFriendsCallback(const EOS_UI_ShowFriendsCallbackInfo* data)
{
	if (data)
	{
		EOSFuncs::logInfo("ShowFriendsCallback: result: %d", static_cast<int>(data->ResultCode));
	}
}

bool EOSFuncs::initAuth(std::string hostname, std::string tokenName)
{
	AuthHandle = EOS_Platform_GetAuthInterface(PlatformHandle);
	ConnectHandle = EOS_Platform_GetConnectInterface(PlatformHandle);

	EOS_Auth_Credentials Credentials = {};
	Credentials.ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
	if (hostname.compare("") == 0)
	{
		Credentials.Id = nullptr;
	}
	else
	{
		Credentials.Id = hostname.c_str();
	}
	if (tokenName.compare("") == 0)
	{
#ifdef NINTENDO
		static char token[4096];
		Credentials.Token = nxGetNSAID(token, sizeof(token));
		Credentials.ExternalType = EOS_EExternalCredentialType::EOS_ECT_NINTENDO_NSA_ID_TOKEN;
#else
		Credentials.Token = nullptr;
#endif
	}
	else
	{
		Credentials.Token = tokenName.c_str();
	}
	Credentials.Type = EOS.AccountManager.AuthType;
	switch (Credentials.Type) {
	case EOS_ELoginCredentialType::EOS_LCT_Developer:
		EOSFuncs::logInfo("Connecting to \'%s\'...", hostname.c_str());
		break;
	case EOS_ELoginCredentialType::EOS_LCT_ExchangeCode:
		EOSFuncs::logInfo("Connecting via exchange token...");
		break;
	case EOS_ELoginCredentialType::EOS_LCT_ExternalAuth:
		EOSFuncs::logInfo("Connecting via external authorization token...");
		break;
	default:
		EOSFuncs::logInfo("Unknown authorization method, possible problem?");
		break;
	}

	EOS_Auth_LoginOptions LoginOptions{};
	LoginOptions.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;
	LoginOptions.ScopeFlags = static_cast<EOS_EAuthScopeFlags>(EOS_EAuthScopeFlags::EOS_AS_BasicProfile | EOS_EAuthScopeFlags::EOS_AS_FriendsList | EOS_EAuthScopeFlags::EOS_AS_Presence);
	LoginOptions.Credentials = &Credentials;

	EOS_Auth_Login(AuthHandle, &LoginOptions, NULL, AuthLoginCompleteCallback);
	AddConnectAuthExpirationNotification();

	EOS.AccountManager.waitingForCallback = true;
	Uint32 startAuthTicks = SDL_GetTicks();
	Uint32 currentAuthTicks = startAuthTicks;
#if !defined(STEAMWORKS)
	while (EOS.AccountManager.AccountAuthenticationStatus == EOS_EResult::EOS_NotConfigured)
	{
#if defined(APPLE)
		SDL_Event event;
		while (SDL_PollEvent(&event) != 0)
		{
			//Makes Mac work because Apple had to do it different.
		}
#endif
		EOS_Platform_Tick(PlatformHandle);
		SDL_Delay(1);
		currentAuthTicks = SDL_GetTicks();
		if (currentAuthTicks - startAuthTicks >= 3000) // spin the wheels for 3 seconds
		{
			break;
		}
	}
#endif // !STEAMWORKS

	bool achResult = initAchievements();
	assert(achResult == true);
	return true;
}

void EOSFuncs::LobbySearchResults_t::sortResults()
{
	resultsSortedForDisplay.clear();
	if (results.empty())
	{
		return;
	}

	int index = 0;
	for (auto it = results.begin(); it != results.end(); ++it)
	{
		resultsSortedForDisplay.push_back(std::make_pair(EOSFuncs::LobbyData_t::LobbyAttributes_t((*it).LobbyAttributes), index));
		++index;
	}
	std::sort(resultsSortedForDisplay.begin(), resultsSortedForDisplay.end(),
		[](const std::pair<EOSFuncs::LobbyData_t::LobbyAttributes_t, int>& lhs, const std::pair<EOSFuncs::LobbyData_t::LobbyAttributes_t, int>& rhs)
		{
			if (lhs.first.gameCurrentLevel == -1 && rhs.first.gameCurrentLevel == -1)
			{
				return (lhs.first.lobbyCreationTime > rhs.first.lobbyCreationTime);
			}
			return lhs.first.gameCurrentLevel < rhs.first.gameCurrentLevel;
		}
	);
}

void EOSFuncs::Accounts_t::handleLogin()
{
	if ( authTokenTicks != ticks )
	{
		if ( authTokenRefresh > 0 )
		{
			--authTokenRefresh;
		}
		authTokenTicks = ticks;
	}

#if defined(STEAMWORKS) || defined(NINTENDO)
	// can return early
	return;
#endif


	if (!initPopupWindow && popupType == POPUP_TOAST)
	{
		UIToastNotificationManager.createEpicLoginNotification();
		initPopupWindow = true;
	}

	if (!waitingForCallback && (AccountAuthenticationStatus == EOS_EResult::EOS_Success || AccountAuthenticationCompleted == EOS_EResult::EOS_Success))
	{
		firstTimeSetupCompleted = true;
		if (AccountAuthenticationCompleted != EOS_EResult::EOS_Success)
		{
			if (popupType == POPUP_TOAST)
			{
				UIToastNotification* n = UIToastNotificationManager.getNotificationSingle(UIToastNotification::CardType::UI_CARD_EOS_ACCOUNT);
				if (n)
				{
					n->showMainCard();
					n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_AUTO_HIDE);
					n->setSecondaryText("Logged in successfully!");
					n->updateCardEvent(false, true);
					n->setIdleSeconds(5);
				}
			}
			AccountAuthenticationCompleted = EOS_EResult::EOS_Success;
		}
		else
		{
			if ( popupType == POPUP_TOAST )
			{
				UIToastNotification* n = UIToastNotificationManager.getNotificationSingle(UIToastNotification::CardType::UI_CARD_EOS_ACCOUNT);
				if ( n )
				{
					if ( n->getDisplayedText() != "Logged in successfully!" )
					{
						n->updateCardEvent(false, true);
					}
				}
			}
		}
		return;
	}
	AccountAuthenticationCompleted = EOS_EResult::EOS_NotConfigured;

	// handle errors below...
	if (initPopupWindow && popupType == POPUP_TOAST)
	{
		if (!waitingForCallback)
		{
			if (AccountAuthenticationStatus != EOS_EResult::EOS_NotConfigured)
			{
				UIToastNotification* n = UIToastNotificationManager.getNotificationSingle(UIToastNotification::CardType::UI_CARD_EOS_ACCOUNT);
				// update the status here.
				if (n)
				{
					n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_ACTION_BUTTON);
					n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_AUTO_HIDE);
					char buf[128] = "";
					snprintf(buf, sizeof(buf), "Login has failed.\nError code: %d\n", static_cast<int>(AccountAuthenticationStatus));
					n->showMainCard();
					n->setSecondaryText(buf);
					n->updateCardEvent(false, true);
					n->setIdleSeconds(10);
				}
			}
			AccountAuthenticationStatus = EOS_EResult::EOS_NotConfigured;
		}
	}
}

void EOSFuncs::CrossplayAccounts_t::createNotification()
{
#ifndef NINTENDO
	UIToastNotificationManager.createEpicCrossplayLoginNotification();
#endif
}

#ifdef NINTENDO
static void nxTokenRequest()
{
	char token[1024] = "";
	nxGetNSAID(token, sizeof(token));

	EOS.ConnectHandle = EOS_Platform_GetConnectInterface(EOS.PlatformHandle);
	EOS_Connect_Credentials Credentials;
	Credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
	Credentials.Token = token;
	Credentials.Type = EOS_EExternalCredentialType::EOS_ECT_NINTENDO_NSA_ID_TOKEN; // change this to steam etc for different account providers.

	EOS_Connect_UserLoginInfo Info{};
	Info.ApiVersion = EOS_CONNECT_USERLOGININFO_API_LATEST;
	Info.DisplayName = MainMenu::getUsername();

	EOS_Connect_LoginOptions Options{};
	Options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
	Options.Credentials = &Credentials;
	Options.UserLoginInfo = &Info;

	EOS_Connect_Login(EOS.ConnectHandle, &Options, nullptr, EOS.ConnectLoginCrossplayCompleteCallback);
	EOS.CrossplayAccountManager.awaitingConnectCallback = true;
	EOS.CrossplayAccountManager.awaitingAppTicketResponse = false;
	printlog("[NX]: AppTicket request success");
}
#endif

void EOSFuncs::CrossplayAccounts_t::handleLogin()
{
#if !defined(STEAMWORKS) && !defined(NINTENDO) // or nintendo, can use this if we only want product users
	return;
#endif // !STEAMWORKS

	if (!EOS.initialized) {
		return;
	}

	if (logOut)
	{
		if (awaitingAppTicketResponse || awaitingConnectCallback || awaitingCreateUserCallback)
		{
			EOSFuncs::logInfo("Crossplay logout pending callbacks...");
			if (awaitingAppTicketResponse)
			{
				EOSFuncs::logInfo("Callback AppTicket still pending...");
			}
			if (awaitingConnectCallback)
			{
				EOSFuncs::logInfo("Callback Connect still pending...");
			}
			if (awaitingCreateUserCallback)
			{
				EOSFuncs::logInfo("Callback CreateUser still pending...");
			}
			return;
		}
		EOSFuncs::logInfo("Crossplay logout request received.");
		EOS.UnsubscribeFromConnectionRequests();
		EOS.CurrentUserInfo.setProductUserIdHandle(nullptr);
		EOS.CurrentUserInfo.bUserLoggedIn = false;

		resetOnFailure();
		for (auto it = UIToastNotificationManager.allNotifications.begin(); it != UIToastNotificationManager.allNotifications.end(); )
		{
			if ((*it).cardType == UIToastNotification::CardType::UI_CARD_CROSSPLAY_ACCOUNT)
			{
				it = UIToastNotificationManager.allNotifications.erase(it);
				continue;
			}
			++it;
		}
		logOut = false;
		EOSFuncs::logInfo("Crossplay logout completed.");
		return;
	}

	bool initLogin = false;
	if (autologin)
	{
		initLogin = true;
		autologin = false;
	}
	if (trySetupFromSettingsMenu)
	{
		initLogin = true;
		trySetupFromSettingsMenu = false;
		if (subwindow)
		{
			buttonCloseSubwindow(nullptr);
		}
	}

	if (initLogin)
	{
#ifdef STEAMWORKS
		cpp_SteamMatchmaking_RequestAppTicket();
		createNotification();
		awaitingAppTicketResponse = true;
		EOSFuncs::logInfo("Crossplay login request started...");
#elif defined(NINTENDO)
		if (nxConnectedToNetwork()) {
			awaitingAppTicketResponse = true;
			nxTokenRequest();
		}
		else {
			printlog("[NX] not connected to network, can't login to EOS");
		}
#endif // STEAMWORKS
		return;
	}

	if (awaitingAppTicketResponse)
	{
		return;
	}
	if (awaitingConnectCallback)
	{
		return;
	}
	if (connectLoginStatus == EOS_EResult::EOS_Success)
	{
		if (connectLoginCompleted != EOS_EResult::EOS_Success)
		{
			connectLoginCompleted = EOS_EResult::EOS_Success;
			LobbyHandler.crossplayEnabled = true;

#ifndef NINTENDO
			UIToastNotification* n = UIToastNotificationManager.getNotificationSingle(UIToastNotification::CardType::UI_CARD_CROSSPLAY_ACCOUNT);
			if (n)
			{
				n->actionFlags &= ~(UIToastNotification::ActionFlags::UI_NOTIFICATION_ACTION_BUTTON);
				n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_AUTO_HIDE);
				n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_CLOSE);
				n->showMainCard();
#ifdef STEAMWORKS
				n->setMainText("Steam account linked.\nCrossplay enabled.");
#else
				n->setMainText("Successfully logged into\nEpic Online Services (tm)");
#endif
				n->updateCardEvent(true, false);
				n->setIdleSeconds(5);
			}
#endif
		}
		return;
	}

	if (connectLoginCompleted == EOS_EResult::EOS_Success)
	{
		return;
	}

	if (connectLoginStatus != EOS_EResult::EOS_NotConfigured)
	{
		if (connectLoginStatus == EOS_EResult::EOS_InvalidUser)
		{
#ifndef NINTENDO
			UIToastNotification* n = UIToastNotificationManager.getNotificationSingle(UIToastNotification::CardType::UI_CARD_CROSSPLAY_ACCOUNT);
			if (n)
			{
				n->actionFlags &= ~(UIToastNotification::ActionFlags::UI_NOTIFICATION_ACTION_BUTTON);
				n->showMainCard();
				n->setSecondaryText("New user.\nAccept EULA to proceed.");
				n->updateCardEvent(false, true);
				n->setIdleSeconds(10);
			}
#endif
			EOSFuncs::logInfo("New EOS user, awaiting user response.");
			createDialogue();
		}
		else
		{
#ifndef NINTENDO
			UIToastNotification* n = UIToastNotificationManager.getNotificationSingle(UIToastNotification::CardType::UI_CARD_CROSSPLAY_ACCOUNT);
			// update the status here.
			if (n)
			{
				n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_AUTO_HIDE);
				char buf[128] = "";
				snprintf(buf, sizeof(buf), "Setup has failed.\nError code: %d\n", static_cast<int>(connectLoginStatus));
				n->showMainCard();
				n->setSecondaryText(buf);
				n->updateCardEvent(false, true);
				n->setIdleSeconds(10);
				n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_ACTION_BUTTON);
				n->buttonAction = &EOSFuncs::CrossplayAccounts_t::retryCrossplaySetupOnFailure;
			}
#endif
			EOSFuncs::logError("Crossplay setup has failed. Error code: %d", static_cast<int>(connectLoginStatus));
			resetOnFailure();
		}
		connectLoginStatus = EOS_EResult::EOS_NotConfigured;
	}
}

void EOSFuncs::CrossplayAccounts_t::retryCrossplaySetupOnFailure()
{
	EOS.CrossplayAccountManager.trySetupFromSettingsMenu = true;
	EOS.CrossplayAccountManager.connectLoginCompleted = EOS_EResult::EOS_NotConfigured;
	EOS.CrossplayAccountManager.connectLoginStatus = EOS_EResult::EOS_NotConfigured;
}

void EOSFuncs::CrossplayAccounts_t::resetOnFailure()
{
	LobbyHandler.crossplayEnabled = false;
	connectLoginCompleted = EOS_EResult::EOS_NotConfigured;
	connectLoginStatus = EOS_EResult::EOS_NotConfigured;
	continuanceToken = nullptr;
}

void EOSFuncs::CrossplayAccounts_t::acceptCrossplay()
{
	promptActive = false;
	acceptedEula = true;
	if (continuanceToken)
	{
		EOS_Connect_CreateUserOptions CreateUserOptions{};
		CreateUserOptions.ApiVersion = EOS_CONNECT_CREATEUSER_API_LATEST;
		CreateUserOptions.ContinuanceToken = continuanceToken;

		awaitingCreateUserCallback = true;
		EOS.ConnectHandle = EOS_Platform_GetConnectInterface(EOS.PlatformHandle);
		EOS_Connect_CreateUser(EOS.ConnectHandle, &CreateUserOptions, nullptr, EOSFuncs::OnCreateUserCrossplayCallback);

		continuanceToken = nullptr;
	}
	else
	{
		resetOnFailure();
	}
	EOSFuncs::logInfo("Crossplay account link has been accepted by user");
}

void EOSFuncs::CrossplayAccounts_t::denyCrossplay()
{
	promptActive = false;
	acceptedEula = false;
	logOut = true;
	resetOnFailure();
	EOSFuncs::logInfo("Crossplay account link has been denied by user");
}

void EOSFuncs::CrossplayAccounts_t::viewPrivacyPolicy()
{
	openURLTryWithOverlay("https://www.baronygame.com/privacypolicy");
}

void EOSFuncs::CrossplayAccounts_t::createDialogue()
{
	promptActive = true;
	MainMenu::crossplayPrompt();
}

bool EOSFuncs::CrossplayAccounts_t::isLoggingIn()
{
	return
		promptActive |
		trySetupFromSettingsMenu |
		awaitingConnectCallback |
		awaitingAppTicketResponse |
		awaitingCreateUserCallback;
}

std::string EOSFuncs::getLobbyCodeFromGameKey(Uint32 key)
{
	const char allChars[37] = "0123456789abcdefghijklmnppqrstuvwxyz";
	std::string code = "";
	while (key != 0)
	{
		code += (allChars[key % 36]);
		key /= 36;
	}
	while (code.size() < 4)
	{
		code += '0';
	}
	return code;
}
Uint32 EOSFuncs::getGameKeyFromLobbyCode(std::string& code)
{
	const char allChars[37] = "0123456789abcdefghijklmnppqrstuvwxyz";
	Uint32 result = 0;
	Uint32 bit = 0;
	for (int i = 0; i < code.size(); ++i)
	{
		if (code[i] >= 'A' && code[i] <= 'Z')
		{
			code[i] = 'a' + (code[i] - 'A');
		}

		if (code[i] >= '0' && code[i] <= '9')
		{
			result += static_cast<Uint32>(code[i] - '0') * static_cast<Uint32>(pow(36, bit));
			++bit;
		}
		else if (code[i] >= 'a' && code[i] <= 'z')
		{
			result += (static_cast<Uint32>(code[i] - 'a') + 10) * static_cast<Uint32>(pow(36, bit));
			++bit;
		}
	}
	return result;
}

void EOSFuncs::queryDLCOwnership()
{
	EcomHandle = EOS_Platform_GetEcomInterface(PlatformHandle);

	EOS_Ecom_QueryEntitlementsOptions options{};
	options.ApiVersion = EOS_ECOM_QUERYENTITLEMENTS_API_LATEST;
	options.bIncludeRedeemed = true;
	std::vector<EOS_Ecom_EntitlementName> entitlements;
	entitlements.push_back("fced51d547714291869b8847fdd770e8");
	entitlements.push_back("7ea3754f8bfa4069938fd0bee3e7197b");

	options.EntitlementNames = entitlements.data();
	options.EntitlementNameCount = entitlements.size();
	options.LocalUserId = EOSFuncs::Helpers_t::epicIdFromString(CurrentUserInfo.epicAccountId.c_str());

	EOS_Ecom_QueryEntitlements(EcomHandle, &options, nullptr, OnEcomQueryEntitlementsCallback);
}

void EOS_CALL EOSFuncs::OnEcomQueryEntitlementsCallback(const EOS_Ecom_QueryEntitlementsCallbackInfo* data)
{
	if (!data)
	{
		EOSFuncs::logError("OnEcomQueryEntitlementsCallback: null data");
		return;
	}
	else if (data->ResultCode == EOS_EResult::EOS_Success)
	{
		EOSFuncs::logInfo("OnEcomQueryEntitlementsCallback: callback success");

		EOS.EcomHandle = EOS_Platform_GetEcomInterface(EOS.PlatformHandle);
		EOS_Ecom_GetEntitlementsCountOptions countOptions{};
		countOptions.ApiVersion = EOS_ECOM_GETENTITLEMENTSCOUNT_API_LATEST;
		countOptions.LocalUserId = EOSFuncs::Helpers_t::epicIdFromString(EOS.CurrentUserInfo.epicAccountId.c_str());

		Uint32 numEntitlements = EOS_Ecom_GetEntitlementsCount(EOS.EcomHandle, &countOptions);
		//EOSFuncs::logInfo("OnEcomQueryEntitlementsCallback: %d entitlements", numEntitlements);
		for (int i = 0; i < numEntitlements; ++i)
		{
			EOS_Ecom_CopyEntitlementByIndexOptions copyOptions{};
			copyOptions.ApiVersion = EOS_ECOM_COPYENTITLEMENTBYINDEX_API_LATEST;
			copyOptions.EntitlementIndex = i;
			copyOptions.LocalUserId = EOSFuncs::Helpers_t::epicIdFromString(EOS.CurrentUserInfo.epicAccountId.c_str());

			EOS_Ecom_Entitlement* e = nullptr;
			EOS_EResult result = EOS_Ecom_CopyEntitlementByIndex(EOS.EcomHandle, &copyOptions, &e);
			//EOSFuncs::logInfo("%d:", static_cast<int>(result));
			if ((result == EOS_EResult::EOS_Success || result == EOS_EResult::EOS_Ecom_EntitlementStale) && e)
			{
				std::string id = e->EntitlementName;
				if (id.compare("fced51d547714291869b8847fdd770e8") == 0)
				{
					enabledDLCPack1 = true;
					EOSFuncs::logInfo("Myths & Outcasts DLC Enabled");
				}
				else if (id.compare("7ea3754f8bfa4069938fd0bee3e7197b") == 0)
				{
					enabledDLCPack2 = true;
					EOSFuncs::logInfo("Legends & Pariahs DLC Enabled");
				}
				//EOSFuncs::logInfo("Index: %d | Id %s: | Entitlement Name: %s | CatalogItemId: %s | Redeemed: %d", i, e->EntitlementId, e->EntitlementName, e->CatalogItemId, (e->bRedeemed == EOS_TRUE) ? 1 : 0);
			}
			EOS_Ecom_Entitlement_Release(e);

		}
	}
	else
	{
		EOSFuncs::logError("OnEcomQueryEntitlementsCallback: Callback failure: %d", static_cast<int>(data->ResultCode));
	}
}

void EOS_CALL EOSFuncs::OnEcomQueryOwnershipCallback(const EOS_Ecom_QueryOwnershipCallbackInfo* data)
{
	if (!data)
	{
		EOSFuncs::logError("OnEcomQueryOwnershipCallback: null data");
		return;
	}
	else if (data->ResultCode == EOS_EResult::EOS_Success)
	{
		for (int i = 0; i < data->ItemOwnershipCount; ++i)
		{
			EOSFuncs::logInfo("OnEcomQueryOwnershipCallback: Ownership status: %d, %d", static_cast<int>(data->ItemOwnership[i].OwnershipStatus), data->ItemOwnershipCount);
		}
	}
	else
	{
		EOSFuncs::logError("OnEcomQueryOwnershipCallback: Callback failure: %d", static_cast<int>(data->ResultCode));
	}
}

static void EOS_CALL OnQueryGlobalStatsCallback(const EOS_Stats_OnQueryStatsCompleteCallbackInfo* data)
{
	if (!EOS.ServerPlatformHandle)
	{
		return;
	}
	if (!data)
	{
		EOSFuncs::logError("OnQueryGlobalStatsCallback: null data");
		return;
	}
	else if (data->ResultCode == EOS_EResult::EOS_Success)
	{
		EOS_Stats_GetStatCountOptions StatCountOptions{};
		StatCountOptions.ApiVersion = EOS_STATS_GETSTATCOUNT_API_LATEST;
		StatCountOptions.TargetUserId = EOS.StatGlobalManager.getProductUserIdHandle();

		Uint32 numStats = EOS_Stats_GetStatsCount(EOS_Platform_GetStatsInterface(EOS.ServerPlatformHandle),
			&StatCountOptions);

		EOSFuncs::logInfo("OnQueryGlobalStatsCallback: read %d stats", numStats);

		EOS_Stats_CopyStatByIndexOptions CopyByIndexOptions{};
		CopyByIndexOptions.ApiVersion = EOS_STATS_COPYSTATBYINDEX_API_LATEST;
		CopyByIndexOptions.TargetUserId = EOS.StatGlobalManager.getProductUserIdHandle();

		EOS_Stats_Stat* copyStat = NULL;

		for (CopyByIndexOptions.StatIndex = 0; CopyByIndexOptions.StatIndex < numStats; ++CopyByIndexOptions.StatIndex)
		{
			EOS_EResult result = EOS_Stats_CopyStatByIndex(EOS_Platform_GetStatsInterface(EOS.ServerPlatformHandle),
				&CopyByIndexOptions, &copyStat);
			if (result == EOS_EResult::EOS_Success && copyStat)
			{
				if (!strcmp(copyStat->Name, "STAT_GLOBAL_DISABLE"))
				{
					if (copyStat->Value == 1)
					{
						//EOSFuncs::logInfo("OnQueryGlobalStatsCallback: disabled");
						EOS.StatGlobalManager.bIsDisabled = true;
					}
				}
				else if (!strcmp(copyStat->Name, "STAT_GLOBAL_PROMO"))
				{
					if (copyStat->Value == 1)
					{
						EOS.StatGlobalManager.bPromoEnabled = true;
						//EOSFuncs::logInfo("OnQueryGlobalStatsCallback: received");
					}
				}

				//EOSFuncs::logInfo("OnQueryGlobalStatsCallback: stat %s | %d", copyStat->Name, copyStat->Value);
				EOS_Stats_Stat_Release(copyStat);
			}
		}
	}
	else
	{
		EOSFuncs::logError("OnQueryGlobalStatsCallback: Callback failure: %d", static_cast<int>(data->ResultCode));
	}
}

void EOSFuncs::StatGlobal_t::init()
{
	bIsInit = true;
	bIsDisabled = false;
	productUserId = EOS_ProductUserId_FromString(BUILD_ENV_GSE);
}

void EOSFuncs::StatGlobal_t::queryGlobalStatUser()
{
	//init();

	//// Query Player Stats
	//EOS_Stats_QueryStatsOptions StatsQueryOptions{};
	//StatsQueryOptions.ApiVersion = EOS_STATS_QUERYSTATS_API_LATEST;
	//StatsQueryOptions.LocalUserId = getProductUserIdHandle();
	//StatsQueryOptions.TargetUserId = getProductUserIdHandle();

	//// Optional params
	//StatsQueryOptions.StartTime = EOS_STATS_TIME_UNDEFINED;
	//StatsQueryOptions.EndTime = EOS_STATS_TIME_UNDEFINED;

	//StatsQueryOptions.StatNamesCount = NUM_GLOBAL_STEAM_STATISTICS;
	//StatsQueryOptions.StatNames = new const char* [NUM_GLOBAL_STEAM_STATISTICS];

	//for (int i = 0; i < NUM_GLOBAL_STEAM_STATISTICS; ++i)
	//{
	//	StatsQueryOptions.StatNames[i] = g_SteamGlobalStats[i].m_pchStatName;
	//}

	//if (EOS.ServerPlatformHandle)
	//{
	//	return;
	//}

	//EOS_Stats_QueryStats(EOS_Platform_GetStatsInterface(EOS.ServerPlatformHandle),
	//	&StatsQueryOptions, nullptr, OnQueryGlobalStatsCallback);
	//delete[] StatsQueryOptions.StatNames;
}

#endif //USE_EOS
