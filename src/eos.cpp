
#include "Config.hpp"

#ifdef USE_EOS

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
	printlog("[EOS Logging]: %s:%s", log->Category, log->Message);
}
void EOS_CALL EOSFuncs::AuthLoginCompleteCallback(const EOS_Auth_LoginCallbackInfo* data)
{
	EOS_HAuth AuthHandle = EOS_Platform_GetAuthInterface(EOS.PlatformHandle);
	EOS.AccountManager.waitingForCallback = false;
	if ( !data )
	{
		EOSFuncs::logError("Login Callback error: null data");
	}
	else if ( data->ResultCode == EOS_EResult::EOS_Success )
	{
		EOSFuncs::logInfo("Login Callback: success");
		EOS.AccountManager.AccountAuthenticationStatus = EOS_EResult::EOS_Success;

		const int numAccounts = EOS_Auth_GetLoggedInAccountsCount(AuthHandle);
		for ( int accIndex = 0; accIndex < numAccounts; ++accIndex )
		{
			EOS.CurrentUserInfo.epicAccountId = EOSFuncs::Helpers_t::epicIdToString(EOS_Auth_GetLoggedInAccountByIndex(AuthHandle, accIndex));
			EOS_ELoginStatus LoginStatus;
			LoginStatus = EOS_Auth_GetLoginStatus(AuthHandle, data->LocalUserId);

			EOSFuncs::logInfo("Account index: %d Status: %d UserID: %s", accIndex,
				static_cast<int>(LoginStatus), EOS.CurrentUserInfo.epicAccountId.c_str());

			EOS.getUserInfo(EOSFuncs::Helpers_t::epicIdFromString(EOS.CurrentUserInfo.epicAccountId.c_str()),
				UserInfoQueryType::USER_INFO_QUERY_LOCAL, accIndex);
			EOS.initConnectLogin();
		}
		return;
	}
	else if ( data->ResultCode == EOS_EResult::EOS_OperationWillRetry )
	{
		EOSFuncs::logError("Login Callback: retrying");
		return;
	}
	else if ( data->ResultCode == EOS_EResult::EOS_Auth_PinGrantCode )
	{
		EOSFuncs::logError("Login Callback: PIN required");
	}
	else if ( data->ResultCode == EOS_EResult::EOS_Auth_MFARequired )
	{
		EOSFuncs::logError("Login Callback: MFA required");
	}
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
	if ( !data )
	{
		EOSFuncs::logError("Connect Login Callback: null data");
		EOS.CurrentUserInfo.setProductUserIdHandle(nullptr);
		EOS.CurrentUserInfo.bUserLoggedIn = false;
		return;
	}
	if ( data->ResultCode == EOS_EResult::EOS_Success )
	{
		EOS.CurrentUserInfo.setProductUserIdHandle(data->LocalUserId);
		EOS.CurrentUserInfo.bUserLoggedIn = true;
		EOS.SubscribeToConnectionRequests();
		EOSFuncs::logInfo("Connect Login Callback success: %s", EOS.CurrentUserInfo.getProductUserIdStr());
	}
	else
	{
		EOSFuncs::logError("Connect Login Callback: General fail: %d", static_cast<int>(data->ResultCode));
		EOS.CurrentUserInfo.setProductUserIdHandle(nullptr);
		EOS.CurrentUserInfo.bUserLoggedIn = false;

		/*if ( data->ResultCode == EOS_EResult::EOS_InvalidUser )
		{
			EOS_Connect_CreateUserOptions CreateUserOptions;
			CreateUserOptions.ApiVersion = EOS_CONNECT_CREATEUSER_API_LATEST;
			CreateUserOptions.ContinuanceToken = data->ContinuanceToken;

			EOS.ConnectHandle = EOS_Platform_GetConnectInterface(EOS.PlatformHandle);
			EOS_Connect_CreateUser(EOS.ConnectHandle, &CreateUserOptions, nullptr, OnCreateUserCallback);
		}*/
	}
}

void EOS_CALL EOSFuncs::OnCreateUserCallback(const EOS_Connect_CreateUserCallbackInfo* data)
{
	if ( !data )
	{
		EOSFuncs::logError("OnCreateUserCallback: null data");
		return;
	}
	if ( data->ResultCode == EOS_EResult::EOS_Success )
	{
		EOS.CurrentUserInfo.setProductUserIdHandle(data->LocalUserId);
		EOS.CurrentUserInfo.bUserLoggedIn = true;
		EOS.SubscribeToConnectionRequests();
		EOSFuncs::logInfo("OnCreateUserCallback success, new user: %s", EOS.CurrentUserInfo.getProductUserIdStr());
	}
	else
	{
		EOSFuncs::logError("OnCreateUserCallback: General fail: %d", static_cast<int>(data->ResultCode));
	}
}

void EOS_CALL EOSFuncs::FriendsQueryCallback(const EOS_Friends_QueryFriendsCallbackInfo* data)
{
	if ( !data )
	{
		EOSFuncs::logError("FriendsQueryCallback: null data");
		return;
	}
	EOS_HFriends FriendsHandle = EOS_Platform_GetFriendsInterface(EOS.PlatformHandle);
	EOS_Friends_GetFriendsCountOptions FriendsCountOptions;
	FriendsCountOptions.ApiVersion = EOS_FRIENDS_GETFRIENDSCOUNT_API_LATEST;
	FriendsCountOptions.LocalUserId = data->LocalUserId;
	int numFriends = EOS_Friends_GetFriendsCount(FriendsHandle, &FriendsCountOptions);
	EOSFuncs::logInfo("FriendsQueryCallback: Friends num: %d", numFriends);

	EOS.CurrentUserInfo.Friends.clear();

	EOS_Friends_GetFriendAtIndexOptions IndexOptions;
	IndexOptions.ApiVersion = EOS_FRIENDS_GETFRIENDATINDEX_API_LATEST;
	IndexOptions.LocalUserId = data->LocalUserId;
	for ( int i = 0; i < numFriends; ++i )
	{
		IndexOptions.Index = i;
		EOS_EpicAccountId FriendUserId = EOS_Friends_GetFriendAtIndex(FriendsHandle, &IndexOptions);

		if ( EOSFuncs::Helpers_t::epicIdIsValid(FriendUserId) )
		{
			EOS_Friends_GetStatusOptions StatusOptions;
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
	if ( !data )
	{
		EOSFuncs::logError("UserInfoCallback: null data");
		return;
	}
	UserInfoQueryData_t* userInfoQueryData = (static_cast<UserInfoQueryData_t*>(data->ClientData));
	if ( data->ResultCode == EOS_EResult::EOS_Success )
	{
		EOS_UserInfo_CopyUserInfoOptions UserInfoOptions;
		UserInfoOptions.ApiVersion = EOS_USERINFO_COPYUSERINFO_API_LATEST;
		UserInfoOptions.LocalUserId = data->LocalUserId;
		UserInfoOptions.TargetUserId = data->TargetUserId;
		EOS_UserInfo* userInfo; // result is managed by application to free the memory.
		if ( EOS_UserInfo_CopyUserInfo(EOS_Platform_GetUserInfoInterface(EOS.PlatformHandle),
			&UserInfoOptions, &userInfo) == EOS_EResult::EOS_Success )
		{
			if ( userInfoQueryData->queryType == EOS.USER_INFO_QUERY_LOCAL )
			{
				// local user
				EOS.CurrentUserInfo.Name = userInfo->DisplayName;
				EOS.CurrentUserInfo.bUserInfoRequireUpdate = false;
				EOSFuncs::logInfo("UserInfoCallback: Current User Name: %s", userInfo->DisplayName);
			}
			else if ( userInfoQueryData->queryType == EOS.USER_INFO_QUERY_FRIEND )
			{
				if ( EOS.CurrentUserInfo.Friends.empty() )
				{
					EOSFuncs::logInfo("UserInfoCallback: friend info request failed due empty friends list");
				}
				else
				{
					bool foundFriend = false;
					std::string queryTarget = EOSFuncs::Helpers_t::epicIdToString(userInfoQueryData->epicAccountId);
					for ( auto& it : EOS.CurrentUserInfo.Friends )
					{
						if ( it.EpicAccountId.compare(queryTarget) == 0 )
						{
							foundFriend = true;
							it.Name = userInfo->DisplayName;
							it.bUserInfoRequireUpdate = false;
							EOSFuncs::logInfo("UserInfoCallback: found friend username: %s", userInfo->DisplayName);
							break;
						}
					}
					if ( !foundFriend )
					{
						EOSFuncs::logInfo("UserInfoCallback: could not find player in current lobby with account %s", 
							EOSFuncs::Helpers_t::epicIdToString(userInfoQueryData->epicAccountId));
					}
				}
			}
			else if ( userInfoQueryData->queryType == EOS.USER_INFO_QUERY_LOBBY_MEMBER )
			{
				if ( !EOS.CurrentLobbyData.currentLobbyIsValid() || EOS.CurrentLobbyData.playersInLobby.empty() )
				{
					EOSFuncs::logInfo("UserInfoCallback: lobby member request failed due to invalid or no player data in lobby");
				}
				else
				{
					bool foundMember = false;
					std::string queryTarget = EOSFuncs::Helpers_t::epicIdToString(userInfoQueryData->epicAccountId);
					for ( auto& it : EOS.CurrentLobbyData.playersInLobby )
					{
						if ( queryTarget.compare(it.memberEpicAccountId.c_str()) == 0 )
						{
							foundMember = true;
							it.name = userInfo->DisplayName;
							it.bUserInfoRequireUpdate = false;
							EOSFuncs::logInfo("UserInfoCallback: found lobby username: %s", userInfo->DisplayName);
							break;
						}
					}
					if ( !foundMember )
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
	if ( !data )
	{
		EOSFuncs::logError("OnCreateLobbyFinished: null data");
		return;
	}
	else if ( data->ResultCode == EOS_EResult::EOS_Success )
	{
		EOS.CurrentLobbyData.LobbyId = data->LobbyId;

		EOS.CurrentLobbyData.LobbyAttributes.lobbyName = EOS.CurrentUserInfo.Name + "'s lobby";
		strncpy(EOS.currentLobbyName, EOS.CurrentLobbyData.LobbyAttributes.lobbyName.c_str(), 31);

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
	if ( !data )
	{
		EOSFuncs::logError("OnLobbySearchFinished: null data");
		return;
	}
	else if ( data->ResultCode == EOS_EResult::EOS_Success )
	{
		EOS_LobbySearch_GetSearchResultCountOptions SearchResultOptions;
		SearchResultOptions.ApiVersion = EOS_LOBBYSEARCH_GETSEARCHRESULTCOUNT_API_LATEST;
		int NumSearchResults = EOS_LobbySearch_GetSearchResultCount(EOS.LobbySearchResults.CurrentLobbySearch, &SearchResultOptions);
		int* searchOptions = static_cast<int*>(data->ClientData);

		EOS_LobbySearch_CopySearchResultByIndexOptions IndexOptions;
		IndexOptions.ApiVersion = EOS_LOBBYSEARCH_COPYSEARCHRESULTBYINDEX_API_LATEST;
		for ( int i = 0; i < NumSearchResults; ++i )
		{
			LobbyData_t newLobby;
			EOS_HLobbyDetails LobbyDetails = nullptr;
			IndexOptions.LobbyIndex = i;
			EOS_EResult Result = EOS_LobbySearch_CopySearchResultByIndex(EOS.LobbySearchResults.CurrentLobbySearch,
				&IndexOptions, &LobbyDetails);
			if ( Result == EOS_EResult::EOS_Success && LobbyDetails )
			{
				EOS.setLobbyDetailsFromHandle(LobbyDetails, &newLobby);
				EOSFuncs::logInfo("OnLobbySearchFinished: Found lobby: %s, Owner: %s, MaxPlayers: %d", 
					newLobby.LobbyId.c_str(), newLobby.OwnerProductUserId.c_str(), newLobby.MaxPlayers);

				EOS.LobbySearchResults.results.push_back(newLobby);

				if ( searchOptions[EOSFuncs::LobbyParameters_t::JOIN_OPTIONS]
					== static_cast<int>(EOSFuncs::LobbyParameters_t::LOBBY_JOIN_FIRST_SEARCH_RESULT) )
				{
					// set the handle to be used for joining.
					EOS.LobbyParameters.lobbyToJoin = LobbyDetails;
					EOS.joinLobby(&newLobby);
				}
				else if ( searchOptions[EOSFuncs::LobbyParameters_t::JOIN_OPTIONS]
					== static_cast<int>(EOSFuncs::LobbyParameters_t::LOBBY_DONT_JOIN) )
				{
					// we can release this handle.
					EOS_LobbyDetails_Release(LobbyDetails);
				}
				else if ( searchOptions[EOSFuncs::LobbyParameters_t::JOIN_OPTIONS]
					== static_cast<int>(EOSFuncs::LobbyParameters_t::LOBBY_UPDATE_CURRENTLOBBY) )
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

		if ( NumSearchResults == 0 )
		{
			EOSFuncs::logInfo("OnLobbySearchFinished: Found 0 lobbies!");
		}
		EOS.LobbySearchResults.sortResults();
		return;
	}
	else if ( data->ResultCode == EOS_EResult::EOS_NotFound )
	{
		EOSFuncs::logError("OnLobbySearchFinished: Requested lobby no longer exists", static_cast<int>(data->ResultCode));
	}
	else
	{
		EOSFuncs::logError("OnLobbySearchFinished: Callback failure: %d", static_cast<int>(data->ResultCode));
	}

	int* searchOptions = static_cast<int*>(data->ClientData);
	if ( searchOptions[EOSFuncs::LobbyParameters_t::JOIN_OPTIONS]
		== static_cast<int>(EOSFuncs::LobbyParameters_t::LOBBY_JOIN_FIRST_SEARCH_RESULT) )
	{
		// we were trying to join a lobby, set error message.
		EOS.bConnectingToLobbyWindow = false;
		EOS.bConnectingToLobby = false;
		EOS.ConnectingToLobbyStatus = static_cast<int>(data->ResultCode);
	}
}

std::string EOSFuncs::getLobbyJoinFailedConnectString(int result)
{
	char buf[1024] = "";
	switch ( result )
	{
		case EResult_LobbyFailures::LOBBY_GAME_IN_PROGRESS:
			snprintf(buf, 1023, "Failed to join the selected lobby:\n\nGame is currently in progress and not joinable.");
			break;
		case EResult_LobbyFailures::LOBBY_USING_SAVEGAME:
			snprintf(buf, 1023, "Failed to join the selected lobby:\n\nLobby requires a compatible saved game to join.\nNewly created characters cannot join this lobby.");
			break;
		case EResult_LobbyFailures::LOBBY_NOT_USING_SAVEGAME:
			snprintf(buf, 1023, "Failed to join the selected lobby:\n\nLobby is not loading from a saved game.\nCreate a new character to join.");
			break;
		case EResult_LobbyFailures::LOBBY_WRONG_SAVEGAME:
			snprintf(buf, 1023, "Failed to join the selected lobby:\n\nLobby saved game is incompatible with current save.\nEnsure the correct saved game is loaded.");
			break;
		case EOS_EResult::EOS_Canceled:
			snprintf(buf, 1023, "Lobby join cancelled while setting up players.\n\nSafely leaving lobby.");
			break;
		case EOS_EResult::EOS_TimedOut:
			snprintf(buf, 1023, "Failed to join the selected lobby:\n\nTimeout waiting for response from host.");
			break;
		case EResult_LobbyFailures::LOBBY_NO_OWNER:
			snprintf(buf, 1023, "Failed to join the selected lobby:\n\nNo host found for lobby.");
			break;
		case EOS_EResult::EOS_NotFound:
			snprintf(buf, 1023, "Failed to join the selected lobby:\n\nLobby no longer exists.");
			break;
		case EOS_EResult::EOS_Lobby_TooManyPlayers:
			snprintf(buf, 1023, "Failed to join the selected lobby:\n\nLobby is full.");
			break;
		default:
			snprintf(buf, 1023, "Failed to join the selected lobby:\n\nGeneral failure - error code: %d.", result);
			break;
	}
	EOSFuncs::logError(buf);
	return buf;
}

void EOS_CALL EOSFuncs::OnLobbyJoinCallback(const EOS_Lobby_JoinLobbyCallbackInfo* data)
{
	if ( !data )
	{
		EOSFuncs::logError("OnLobbyJoinCallback: null data");
		EOS.bConnectingToLobby = false;
		EOS.bConnectingToLobbyWindow = false;
		EOS.ConnectingToLobbyStatus = static_cast<int>(EOS_EResult::EOS_UnexpectedError);
	}
	else if ( data->ResultCode == EOS_EResult::EOS_Success )
	{
		EOSFuncs::logInfo("OnLobbyJoinCallback: Joined lobby id: %s", data->LobbyId);
		/*if ( static_cast<LobbyData_t*>(data->ClientData) == &EOS.CurrentLobbyData )
		{
			EOS.CurrentLobbyData.LobbyId = data->LobbyId;
		}*/
		EOS.bConnectingToLobby = false;

		if ( EOS.CurrentLobbyData.bDenyLobbyJoinEvent && EOS.CurrentLobbyData.LobbyId.compare(data->LobbyId) == 0 )
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
	if ( !data )
	{
		EOSFuncs::logError("OnLobbyLeaveCallback: null data");
	}
	else if ( data->ResultCode == EOS_EResult::EOS_Success )
	{
		EOSFuncs::logInfo("OnLobbyLeaveCallback: Left lobby id: %s", data->LobbyId);
		if ( EOS.CurrentLobbyData.LobbyId.compare(data->LobbyId) == 0 )
		{
			LobbyLeaveCleanup(EOS.CurrentLobbyData);
		}
		return;
	}
	else if ( data->ResultCode == EOS_EResult::EOS_NotFound )
	{
		EOSFuncs::logInfo("OnLobbyLeaveCallback: Could not find lobby id to leave: %s", data->LobbyId);
		if ( EOS.CurrentLobbyData.LobbyId.compare(data->LobbyId) == 0 )
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
	if ( data )
	{
		std::string SocketName = data->SocketId->SocketName;
		if ( SocketName != "CHAT" )
		{
			EOSFuncs::logError("OnIncomingConnectionRequest: bad socket id: %s", SocketName.c_str());
			return;
		}

		EOS_HP2P P2PHandle = EOS_Platform_GetP2PInterface(EOS.PlatformHandle);
		EOS_P2P_AcceptConnectionOptions Options;
		Options.ApiVersion = EOS_P2P_ACCEPTCONNECTION_API_LATEST;
		Options.LocalUserId = EOS.CurrentUserInfo.getProductUserIdHandle();
		Options.RemoteUserId = data->RemoteUserId;

		EOS_P2P_SocketId SocketId;
		SocketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
		strncpy(SocketId.SocketName, "CHAT", 5);
		Options.SocketId = &SocketId;

		EOS_EResult Result = EOS_P2P_AcceptConnection(P2PHandle, &Options);
		if ( Result != EOS_EResult::EOS_Success )
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
	if ( data )
	{
		if ( EOS.LobbyModificationHandle )
		{
			EOS_LobbyModification_Release(EOS.LobbyModificationHandle);
			EOS.LobbyModificationHandle = nullptr;
		}

		if ( data->ResultCode != EOS_EResult::EOS_Success )
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
	if ( data )
	{
		if ( EOS.LobbyMemberModificationHandle )
		{
			EOS_LobbyModification_Release(EOS.LobbyMemberModificationHandle);
			EOS.LobbyMemberModificationHandle = nullptr;
		}

		if ( data->ResultCode != EOS_EResult::EOS_Success && data->ResultCode != EOS_EResult::EOS_NoChange )
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
	if ( data )
	{
		if ( data->ResultCode == EOS_EResult::EOS_Success )
		{
			EOSFuncs::logInfo("OnQueryAccountMappingsCallback: Success");
			std::vector<EOS_ProductUserId> MappingsReceived;
			for ( const EOS_ProductUserId& productId : EOS.ProductIdsAwaitingAccountMappingCallback )
			{
				EOS_Connect_GetProductUserIdMappingOptions Options;
				Options.ApiVersion = EOS_CONNECT_GETEXTERNALACCOUNTMAPPINGS_API_LATEST;
				Options.AccountIdType = EOS_EExternalAccountType::EOS_EAT_EPIC;
				Options.LocalUserId = EOS.CurrentUserInfo.getProductUserIdHandle();
				Options.TargetProductUserId = productId;

				EOS_HConnect ConnectHandle = EOS_Platform_GetConnectInterface(EOS.PlatformHandle);
				char buffer[EOS_CONNECT_EXTERNAL_ACCOUNT_ID_MAX_LENGTH];
				int bufferSize = EOS_CONNECT_EXTERNAL_ACCOUNT_ID_MAX_LENGTH;
				EOS_EResult Result = EOS_Connect_GetProductUserIdMapping(ConnectHandle, &Options, buffer, &bufferSize);
				if ( Result == EOS_EResult::EOS_Success )
				{
					std::string receivedStr(buffer, bufferSize);
					EOS_EpicAccountId epicAccountId = EOSFuncs::Helpers_t::epicIdFromString(receivedStr.c_str());

					// insert the ids into the global map
					EOS.AccountMappings.insert(std::pair<EOS_ProductUserId, EOS_EpicAccountId>(productId,epicAccountId));

					for ( LobbyData_t::PlayerLobbyData_t& player : EOS.CurrentLobbyData.playersInLobby )
					{
						if ( EOSFuncs::Helpers_t::isMatchingProductIds(productId, EOSFuncs::Helpers_t::productIdFromString(player.memberProductUserId.c_str())) )
						{
							player.memberEpicAccountId = receivedStr;
							EOS.getUserInfo(epicAccountId, EOSFuncs::USER_INFO_QUERY_LOBBY_MEMBER, 0);
							EOSFuncs::logInfo("OnQueryAccountMappingsCallback: product id: %s, epic account id: %s", 
								player.memberProductUserId.c_str(), player.memberEpicAccountId.c_str());
							break;
						}
					}
					MappingsReceived.push_back(productId);
				}
			}

			for ( const EOS_ProductUserId& productId : MappingsReceived )
			{
				EOS.ProductIdsAwaitingAccountMappingCallback.erase(productId);
			}
		}
		else if ( data->ResultCode != EOS_EResult::EOS_OperationWillRetry )
		{
			EOSFuncs::logError("OnQueryAccountMappingsCallback: retrying");
		}
	}
}

void EOS_CALL EOSFuncs::OnMemberUpdateReceived(const EOS_Lobby_LobbyMemberUpdateReceivedCallbackInfo* data)
{
	if ( data )
	{
		if ( EOS.CurrentLobbyData.LobbyId.compare(data->LobbyId) == 0 )
		{
			EOS.CurrentLobbyData.updateLobby();
			EOSFuncs::logInfo("OnMemberUpdateReceived: received user: %s, updating lobby", EOSFuncs::Helpers_t::productIdToString(data->TargetUserId));
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
	if ( data )
	{
		switch ( data->CurrentStatus )
		{
			case EOS_ELobbyMemberStatus::EOS_LMS_CLOSED:
			case EOS_ELobbyMemberStatus::EOS_LMS_DISCONNECTED:
			case EOS_ELobbyMemberStatus::EOS_LMS_KICKED:
			case EOS_ELobbyMemberStatus::EOS_LMS_LEFT:
				if ( EOS.P2PConnectionInfo.isPeerIndexed(data->TargetUserId) )
				{
					EOS.P2PConnectionInfo.assignPeerIndex(data->TargetUserId, -1);
				}

				if ( EOS.CurrentLobbyData.LobbyId.compare(data->LobbyId) == 0 )
				{
					if ( data->CurrentStatus == EOS_ELobbyMemberStatus::EOS_LMS_CLOSED
						|| (data->CurrentStatus == EOS_ELobbyMemberStatus::EOS_LMS_KICKED
							&& (data->TargetUserId == EOS.CurrentUserInfo.getProductUserIdHandle()))
						)
					{
						// if lobby closed or we got kicked, then clear data.
						LobbyLeaveCleanup(EOS.CurrentLobbyData);
					}
					else
					{
						EOS.CurrentLobbyData.updateLobby();
						EOSFuncs::logInfo("OnMemberStatusReceived: received user: %s, event: %d, updating lobby", 
							EOSFuncs::Helpers_t::productIdToString(data->TargetUserId),
							static_cast<int>(data->CurrentStatus));
						return;
					}
				}
				break;
			case EOS_ELobbyMemberStatus::EOS_LMS_JOINED:
			case EOS_ELobbyMemberStatus::EOS_LMS_PROMOTED:
				if ( EOS.CurrentLobbyData.LobbyId.compare(data->LobbyId) == 0 )
				{
					EOS.CurrentLobbyData.updateLobby();
					EOSFuncs::logInfo("OnMemberStatusReceived: received user: %s, event: %d, updating lobby", 
						EOSFuncs::Helpers_t::productIdToString(data->TargetUserId),
						static_cast<int>(data->CurrentStatus));
					return;
				}
				break;
			default:
				break;
		}
		EOSFuncs::logInfo("OnMemberStatusReceived: success, received user: %s | status: %d", 
			EOSFuncs::Helpers_t::productIdToString(data->TargetUserId), static_cast<int>(data->CurrentStatus));
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
	if ( data )
	{
		if ( EOS.CurrentLobbyData.LobbyId.compare(data->LobbyId) == 0 )
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
	if ( data )
	{
		if ( data->ResultCode != EOS_EResult::EOS_Success )
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
	if ( data )
	{
		EOSFuncs::logInfo("ConnectAuthExpirationCallback: connect auth expiring - product id: %s",
			EOSFuncs::Helpers_t::productIdToString(data->LocalUserId));
		EOS.CurrentUserInfo.bUserLoggedIn = false;
		EOS.initConnectLogin();
	}
	else
	{
		EOSFuncs::logError("ConnectAuthExpirationCallback: null data");
	}
}

void EOSFuncs::serialize(void* file) {
	// recommend you start with this because it makes versioning way easier down the road
	int version = 0;
	FileInterface* fileInterface = static_cast<FileInterface*>(file);
	fileInterface->property("version", version);
	fileInterface->property("product", ProductId);
	fileInterface->property("sandbox", SandboxId);
	fileInterface->property("deployment", DeploymentId);
	fileInterface->property("clientcredentials", ClientCredentialsId);
	fileInterface->property("clientcredentialssecret", ClientCredentialsSecret);
	fileInterface->property("credentialhost", CredentialHost);
	fileInterface->property("credentialname", CredentialName);
}

bool EOSFuncs::initPlatform(bool enableLogging)
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
	PlatformOptions.Flags = EOS_PF_DISABLE_OVERLAY;
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

void EOSFuncs::initConnectLogin()
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

void EOSFuncs::readFromFile()
{
	if ( PHYSFS_getRealDir("/data/eos.json") )
	{
		std::string inputPath = PHYSFS_getRealDir("/data/eos.json");
		inputPath.append("/data/eos.json");
		if ( FileHelper::readObject(inputPath.c_str(), *this) )
		{
			EOSFuncs::logInfo("[JSON]: Successfully read json file %s", inputPath.c_str());
		}
	}
}

void EOSFuncs::readFromCmdLineArgs()
{
	for ( auto& arg : CommandLineArgs )
	{
		if ( arg.find("-AUTH_PASSWORD=") != std::string::npos )
		{
			EOSFuncs::logInfo("Launching from store...");
			CredentialName = arg.substr(strlen("-AUTH_PASSWORD="));
		}
		else if ( arg.find("-AUTH_TYPE=exchangecode") != std::string::npos )
		{
			EOS.AccountManager.AuthType = EOS_ELoginCredentialType::EOS_LCT_ExchangeCode;
		}
	}
}

bool EOSFuncs::HandleReceivedMessages(EOS_ProductUserId* remoteIdReturn)
{
	if ( !CurrentUserInfo.isValid() )
	{
		//logError("HandleReceivedMessages: Invalid local user Id: %s", CurrentUserInfo.getProductUserIdStr());
		return false;
	}

	if ( !net_packet )
	{
		return false;
	}

	EOS_HP2P P2PHandle = EOS_Platform_GetP2PInterface(PlatformHandle);

	EOS_P2P_ReceivePacketOptions ReceivePacketOptions;
	ReceivePacketOptions.ApiVersion = EOS_P2P_RECEIVEPACKET_API_LATEST;
	ReceivePacketOptions.LocalUserId = CurrentUserInfo.getProductUserIdHandle();
	ReceivePacketOptions.MaxDataSizeBytes = 512;
	ReceivePacketOptions.RequestedChannel = nullptr;

	EOS_P2P_SocketId SocketId;
	SocketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
	uint8_t Channel = 0;

	Uint32 bytesWritten = 0;
	EOS_EResult result = EOS_P2P_ReceivePacket(P2PHandle, &ReceivePacketOptions, remoteIdReturn, &SocketId, &Channel, net_packet->data, &bytesWritten);
	if ( result == EOS_EResult::EOS_NotFound
		|| result == EOS_EResult::EOS_InvalidAuth
		|| result == EOS_EResult::EOS_InvalidUser )
	{
		//no more packets, just end
		return false;
	}
	else if ( result == EOS_EResult::EOS_Success )
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
	if ( !CurrentUserInfo.isValid() )
	{
		//logError("HandleReceivedMessages: Invalid local user Id: %s", CurrentUserInfo.getProductUserIdStr());
		return false;
	}
	EOS_HP2P P2PHandle = EOS_Platform_GetP2PInterface(PlatformHandle);

	EOS_P2P_ReceivePacketOptions ReceivePacketOptions;
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
	if ( result == EOS_EResult::EOS_NotFound
		|| result == EOS_EResult::EOS_InvalidAuth
		|| result == EOS_EResult::EOS_InvalidUser )
	{
		//no more packets, just end
		return false;
	}
	else if ( result == EOS_EResult::EOS_Success )
	{
		char buffer[512] = "";
		strncpy(buffer, (char*)dummyData, 512 - 1);
		buffer[4] = '\0';
		std::string remoteStr = EOSFuncs::Helpers_t::productIdToString(*remoteIdReturn);
		if ( (int)buffer[3] < '0'
			&& (int)buffer[0] == 0
			&& (int)buffer[1] == 0
			&& (int)buffer[2] == 0 )
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
	if ( !EOSFuncs::Helpers_t::productIdIsValid(RemoteId) )
	{
		logError("SendMessageP2P: Invalid remote Id: %s", EOSFuncs::Helpers_t::productIdToString(RemoteId));
		return;
	}

	if ( !CurrentUserInfo.isValid() )
	{
		logError("SendMessageP2P: Invalid local user Id: %s", CurrentUserInfo.getProductUserIdStr());
		return;
	}

	EOS_HP2P P2PHandle = EOS_Platform_GetP2PInterface(PlatformHandle);

	EOS_P2P_SocketId SocketId;
	SocketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
	strncpy(SocketId.SocketName, "CHAT", 5);

	EOS_P2P_SendPacketOptions SendPacketOptions;
	SendPacketOptions.ApiVersion = EOS_P2P_SENDPACKET_API_LATEST;
	SendPacketOptions.LocalUserId = CurrentUserInfo.getProductUserIdHandle();
	SendPacketOptions.RemoteUserId = RemoteId;
	SendPacketOptions.SocketId = &SocketId;
	SendPacketOptions.bAllowDelayedDelivery = EOS_TRUE;
	SendPacketOptions.Channel = 0;

	SendPacketOptions.DataLengthBytes = len;
	SendPacketOptions.Data = (char*)data;

	EOS_EResult Result = EOS_P2P_SendPacket(P2PHandle, &SendPacketOptions);
	if ( Result != EOS_EResult::EOS_Success )
	{
		logError("SendMessageP2P: error while sending data, code: %d", static_cast<int>(Result));
	}
}

void EOSFuncs::LobbyData_t::setLobbyAttributesFromGame(HostUpdateLobbyTypes updateType)
{
	if ( updateType == LOBBY_UPDATE_MAIN_MENU )
	{
		LobbyAttributes.lobbyName = EOS.currentLobbyName;
		LobbyAttributes.gameVersion = VERSION;
		LobbyAttributes.isLobbyLoadingSavedGame = loadingsavegame;
		LobbyAttributes.serverFlags = svFlags;
		LobbyAttributes.numServerMods = 0;
		std::chrono::system_clock::duration epochDuration = std::chrono::system_clock::now().time_since_epoch();
		LobbyAttributes.lobbyCreationTime = std::chrono::duration_cast<std::chrono::seconds>(epochDuration).count();
	}
	else if ( updateType == LOBBY_UPDATE_DURING_GAME )
	{
		LobbyAttributes.serverFlags = svFlags;
		LobbyAttributes.gameCurrentLevel = currentlevel;
	}
}

void EOSFuncs::LobbyData_t::setBasicCurrentLobbyDataFromInitialJoin(LobbyData_t* lobbyToJoin)
{
	if ( !lobbyToJoin )
	{
		logError("setBasicCurrentLobbyDataFromInitialJoin: invalid lobby passed.");
		return;
	}

	MaxPlayers = lobbyToJoin->MaxPlayers;
	OwnerProductUserId = lobbyToJoin->OwnerProductUserId;
	LobbyId = lobbyToJoin->LobbyId;
	bLobbyHasBasicDetailsRead = true;

	EOS.P2PConnectionInfo.serverProductId = EOSFuncs::Helpers_t::productIdFromString(OwnerProductUserId.c_str());

	EOS.P2PConnectionInfo.peerProductIds.clear();
	for ( PlayerLobbyData_t& player : playersInLobby )
	{
		EOS_ProductUserId productId = EOSFuncs::Helpers_t::productIdFromString(player.memberProductUserId.c_str());
		EOS.P2PConnectionInfo.insertProductIdIntoPeers(productId);
	}
}

bool EOSFuncs::LobbyData_t::currentUserIsOwner()
{
	if ( currentLobbyIsValid() && OwnerProductUserId.compare(EOS.CurrentUserInfo.getProductUserIdStr()) == 0 )
	{
		return true;
	}
	return false;
};

bool EOSFuncs::LobbyData_t::updateLobbyForHost(HostUpdateLobbyTypes updateType)
{
	if ( !EOSFuncs::Helpers_t::isMatchingProductIds(EOSFuncs::Helpers_t::productIdFromString(this->OwnerProductUserId.c_str()), 
		EOS.CurrentUserInfo.getProductUserIdHandle()) )
	{
		EOSFuncs::logError("updateLobby: current user is not lobby owner");
		return false;
	}

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(EOS.PlatformHandle);
	EOS_Lobby_UpdateLobbyModificationOptions ModifyOptions = {};
	ModifyOptions.ApiVersion = EOS_LOBBY_UPDATELOBBYMODIFICATION_API_LATEST;
	ModifyOptions.LobbyId = this->LobbyId.c_str();
	ModifyOptions.LocalUserId = EOS.CurrentUserInfo.getProductUserIdHandle();

	if ( EOS.LobbyModificationHandle )
	{
		EOS_LobbyModification_Release(EOS.LobbyModificationHandle);
		EOS.LobbyModificationHandle = nullptr;
	}
	EOS_HLobbyModification LobbyModification = nullptr;
	EOS_EResult result = EOS_Lobby_UpdateLobbyModification(LobbyHandle, &ModifyOptions, &LobbyModification);
	if ( result != EOS_EResult::EOS_Success )
	{
		EOSFuncs::logError("updateLobby: Could not create lobby modification. Error code: %d", static_cast<int>(result));
		return false;
	}

	EOS.LobbyModificationHandle = LobbyModification;
	setLobbyAttributesFromGame(updateType);

	// build the list of attributes:
	for ( int i = 0; i < EOSFuncs::LobbyData_t::kNumAttributes; ++i )
	{
		EOS_Lobby_AttributeData data;
		data.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
		data.ValueType = EOS_ELobbyAttributeType::EOS_AT_STRING;
		std::pair<std::string, std::string> dataPair = getAttributePair(static_cast<AttributeTypes>(i));
		if ( dataPair.first.compare("empty") == 0 || dataPair.second.compare("empty") == 0 )
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

		EOS_LobbyModification_AddAttributeOptions addAttributeOptions;
		addAttributeOptions.ApiVersion = EOS_LOBBYMODIFICATION_ADDATTRIBUTE_API_LATEST;
		addAttributeOptions.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;
		addAttributeOptions.Attribute = &(data);

		result = EOS_LobbyModification_AddAttribute(LobbyModification, &addAttributeOptions);
		if ( result != EOS_EResult::EOS_Success )
		{
			EOSFuncs::logError("updateLobby: Could not add attribute %s. Error code: %d", addAttributeOptions.Attribute->Value.AsUtf8, static_cast<int>(result));
		}
		else
		{
			//EOSFuncs::logInfo("updateLobby: Added key: %s attribute: %s", AddAttributeOptions.Attribute->Key, AddAttributeOptions.Attribute->Value.AsUtf8);
		}
	}

	// update our client number on the lobby backend
	if ( assignClientnumMemberAttribute(EOS.CurrentUserInfo.getProductUserIdHandle(), 0) )
	{
		modifyLobbyMemberAttributeForCurrentUser();
	}

	//Trigger lobby update
	EOS_Lobby_UpdateLobbyOptions UpdateOptions;
	UpdateOptions.ApiVersion = EOS_LOBBY_UPDATELOBBY_API_LATEST;
	UpdateOptions.LobbyModificationHandle = EOS.LobbyModificationHandle;
	EOS_Lobby_UpdateLobby(LobbyHandle, &UpdateOptions, nullptr, OnLobbyUpdateFinished);

	bLobbyHasFullDetailsRead = true;
	return true;
}

bool EOSFuncs::LobbyData_t::modifyLobbyMemberAttributeForCurrentUser()
{
	if ( !EOS.CurrentUserInfo.isValid() )
	{
		EOSFuncs::logError("modifyLobbyMemberAttributeForCurrentUser: current user is not valid");
		return false;
	}

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(EOS.PlatformHandle);
	EOS_Lobby_UpdateLobbyModificationOptions ModifyOptions;
	ModifyOptions.ApiVersion = EOS_LOBBY_UPDATELOBBYMODIFICATION_API_LATEST;
	ModifyOptions.LobbyId = this->LobbyId.c_str();
	ModifyOptions.LocalUserId = EOS.CurrentUserInfo.getProductUserIdHandle();

	if ( EOS.LobbyMemberModificationHandle )
	{
		EOS_LobbyModification_Release(EOS.LobbyMemberModificationHandle);
		EOS.LobbyMemberModificationHandle = nullptr;
	}
	EOS_HLobbyModification LobbyMemberModification = nullptr;
	EOS_EResult result = EOS_Lobby_UpdateLobbyModification(LobbyHandle, &ModifyOptions, &LobbyMemberModification);
	if ( result != EOS_EResult::EOS_Success )
	{
		EOSFuncs::logError("updateLobby: Could not create lobby modification. Error code: %d", static_cast<int>(result));
		return false;
	}

	EOS.LobbyMemberModificationHandle = LobbyMemberModification;

	// add attributes for current member
	for ( auto& player : playersInLobby )
	{
		EOS_ProductUserId productId = EOSFuncs::Helpers_t::productIdFromString(player.memberProductUserId.c_str());
		if ( productId != EOS.CurrentUserInfo.getProductUserIdHandle() )
		{
			continue;
		}

		EOS_Lobby_AttributeData memberData;
		memberData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
		memberData.ValueType = EOS_ELobbyAttributeType::EOS_AT_INT64;
		memberData.Key = "CLIENTNUM";
		memberData.Value.AsInt64 = player.clientNumber;

		EOS_LobbyModification_AddMemberAttributeOptions addMemberData;
		addMemberData.ApiVersion = EOS_LOBBYMODIFICATION_ADDMEMBERATTRIBUTE_API_LATEST;
		addMemberData.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;
		addMemberData.Attribute = &memberData;

		result = EOS_LobbyModification_AddMemberAttribute(LobbyMemberModification, &addMemberData);
		if ( result != EOS_EResult::EOS_Success )
		{
			EOSFuncs::logError("updateLobby: Could not add member attribute %d. Error code: %d",
				addMemberData.Attribute->Value.AsInt64, static_cast<int>(result));
		}
	}

	//Trigger lobby update
	EOS_Lobby_UpdateLobbyOptions UpdateOptions;
	UpdateOptions.ApiVersion = EOS_LOBBY_UPDATELOBBY_API_LATEST;
	UpdateOptions.LobbyModificationHandle = EOS.LobbyMemberModificationHandle;
	EOS_Lobby_UpdateLobby(LobbyHandle, &UpdateOptions, nullptr, OnLobbyMemberUpdateFinished);

	//bLobbyHasFullDetailsRead = true;
	return true;
}

bool EOSFuncs::LobbyData_t::assignClientnumMemberAttribute(EOS_ProductUserId targetId, int clientNumToSet)
{
	if ( !EOS.CurrentUserInfo.isValid() )
	{
		EOSFuncs::logError("assignClientnumMemberAttribute: current user is not valid");
		return false;
	}

	if ( !currentLobbyIsValid() )
	{
		EOSFuncs::logError("assignClientnumMemberAttribute: current lobby is not valid");
		return false;
	}

	for ( auto& player : playersInLobby )
	{
		EOS_ProductUserId playerId = EOSFuncs::Helpers_t::productIdFromString(player.memberProductUserId.c_str());
		if ( playerId && playerId == targetId )
		{
			player.clientNumber = clientNumToSet;
			return true;
		}
	}
	return false;
}

int EOSFuncs::LobbyData_t::getClientnumMemberAttribute(EOS_ProductUserId targetId)
{
	if ( !EOS.CurrentUserInfo.isValid() )
	{
		EOSFuncs::logError("getClientnumMemberAttribute: current user is not valid");
		return -2;
	}

	if ( !currentLobbyIsValid() )
	{
		EOSFuncs::logError("getClientnumMemberAttribute: current lobby is not valid");
		return -2;
	}

	for ( auto& player : playersInLobby )
	{
		EOS_ProductUserId playerId = EOSFuncs::Helpers_t::productIdFromString(player.memberProductUserId.c_str());
		if ( playerId && playerId == targetId )
		{
			return player.clientNumber;
		}
	}
	return -2;
}

void EOSFuncs::LobbyData_t::getLobbyAttributes(EOS_HLobbyDetails LobbyDetails)
{
	if ( !currentLobbyIsValid() )
	{
		EOSFuncs::logError("getLobbyAttributes: invalid current lobby - no ID set");
		return;
	}

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(EOS.PlatformHandle);

	/*EOS_Lobby_CopyLobbyDetailsHandleOptions CopyHandleOptions;
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

	EOS_LobbyDetails_GetAttributeCountOptions CountOptions;
	CountOptions.ApiVersion = EOS_LOBBYDETAILS_GETATTRIBUTECOUNT_API_LATEST;
	int numAttributes = EOS_LobbyDetails_GetAttributeCount(LobbyDetails, &CountOptions);

	for ( int i = 0; i < numAttributes; ++i )
	{
		EOS_LobbyDetails_CopyAttributeByIndexOptions AttrOptions;
		AttrOptions.ApiVersion = EOS_LOBBYDETAILS_COPYATTRIBUTEBYINDEX_API_LATEST;
		AttrOptions.AttrIndex = i;

		EOS_Lobby_Attribute* attributePtr = nullptr;
		EOS_EResult result = EOS_LobbyDetails_CopyAttributeByIndex(LobbyDetails, &AttrOptions, &attributePtr);
		if ( result == EOS_EResult::EOS_Success && attributePtr->Data )
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

	if ( !EOS.CurrentLobbyData.currentUserIsOwner() )
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
	CreateOptions.MaxLobbyMembers = 2;
	CreateOptions.PermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED;

	EOS_Lobby_CreateLobby(LobbyHandle, &CreateOptions, nullptr, OnCreateLobbyFinished);
	CurrentLobbyData.MaxPlayers = CreateOptions.MaxLobbyMembers;
	CurrentLobbyData.OwnerProductUserId = CurrentUserInfo.getProductUserIdStr();
	strcpy(EOS.currentLobbyName, "Lobby creation in progress...");
}

void EOSFuncs::joinLobby(LobbyData_t* lobby)
{
	if ( !lobby )
	{
		return;
	}
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

	bool errorOnJoin = false;
	if ( CurrentLobbyData.OwnerProductUserId.compare("NULL") == 0 )
	{
		// this is unexpected - perhaps an attempt to join a lobby that was freshly abandoned
		ConnectingToLobbyStatus = EResult_LobbyFailures::LOBBY_NO_OWNER;
		logError("joinLobby: attempting to join a lobby with a NULL owner: %s, aborting.", CurrentLobbyData.LobbyId.c_str());
		errorOnJoin = true;
	}
	else if ( lobby->LobbyAttributes.isLobbyLoadingSavedGame != loadingsavegame )
	{
		// loading save game, but incorrect assertion from client side.
		if ( loadingsavegame == 0 )
		{
			ConnectingToLobbyStatus = EResult_LobbyFailures::LOBBY_USING_SAVEGAME;
		}
		else if ( loadingsavegame > 0 && lobby->LobbyAttributes.isLobbyLoadingSavedGame == 0 )
		{
			ConnectingToLobbyStatus = EResult_LobbyFailures::LOBBY_NOT_USING_SAVEGAME;
		}
		else if ( loadingsavegame > 0 && lobby->LobbyAttributes.isLobbyLoadingSavedGame > 0 )
		{
			ConnectingToLobbyStatus = EResult_LobbyFailures::LOBBY_WRONG_SAVEGAME;
		}
		else
		{
			ConnectingToLobbyStatus = EResult_LobbyFailures::LOBBY_UNHANDLED_ERROR;
		}
		errorOnJoin = true;
	}
	else if ( lobby->LobbyAttributes.gameCurrentLevel >= 0 )
	{
		/*if ( lobby->LobbyAttributes.gameCurrentLevel == 0 )
		{
			if (  )
		}
		else
		{
		}*/
		ConnectingToLobbyStatus = EResult_LobbyFailures::LOBBY_GAME_IN_PROGRESS;
		errorOnJoin = true;
	}

	if ( errorOnJoin )
	{
		bConnectingToLobbyWindow = false;
		bConnectingToLobby = false;

		LobbyParameters.clearLobbyToJoin();
		LobbyLeaveCleanup(EOS.CurrentLobbyData);
		return;
	}

	EOS_Lobby_JoinLobbyOptions JoinOptions;
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

void EOSFuncs::searchLobbies(LobbyParameters_t::LobbySearchOptions searchType,
	LobbyParameters_t::LobbyJoinOptions joinOptions, EOS_LobbyId lobbyIdToSearch)
{
	LobbyHandle = EOS_Platform_GetLobbyInterface(PlatformHandle);
	logInfo("searchLobbies: starting search");
	EOS_Lobby_CreateLobbySearchOptions CreateSearchOptions = {};
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
	LobbySearchResults.resultsSortedForDisplay.clear();

	/*EOS_LobbySearch_SetTargetUserIdOptions SetLobbyOptions = {};
	SetLobbyOptions.ApiVersion = EOS_LOBBYSEARCH_SETLOBBYID_API_LATEST;
	SetLobbyOptions.TargetUserId = CurrentUserInfo.Friends.at(0).UserId;
	Result = EOS_LobbySearch_SetTargetUserId(LobbySearch, &SetLobbyOptions);*/
	EOS_LobbySearch_SetParameterOptions ParamOptions = {};
	ParamOptions.ApiVersion = EOS_LOBBYSEARCH_SETPARAMETER_API_LATEST;
	ParamOptions.ComparisonOp = EOS_EComparisonOp::EOS_CO_NOTANYOF;

	EOS_Lobby_AttributeData AttrData;
	AttrData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
	ParamOptions.Parameter = &AttrData;
	AttrData.Key = "VER";
	AttrData.Value.AsUtf8 = "0.0.0";
	AttrData.ValueType = EOS_ELobbyAttributeType::EOS_AT_STRING;
	EOS_EResult resultParameter = EOS_LobbySearch_SetParameter(LobbySearch, &ParamOptions);

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

void EOSFuncs::LobbyData_t::destroyLobby()
{
	if ( !currentLobbyIsValid() )
	{
		EOSFuncs::logError("destroyLobby: invalid current lobby - no ID set");
		return;
	}

	if ( !currentUserIsOwner() )
	{
		EOSFuncs::logError("destroyLobby: current user is not lobby owner");
		return;
	}

	EOS.LobbyHandle = EOS_Platform_GetLobbyInterface(EOS.PlatformHandle);

	EOS_Lobby_DestroyLobbyOptions DestroyOptions;
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
	if ( !currentLobbyIsValid() )
	{
		return;
	}
	bool doUpdate = false;
	if ( LobbyAttributes.gameCurrentLevel != currentlevel )
	{
		doUpdate = true;
	}
	if ( LobbyAttributes.serverFlags != svFlags )
	{
		doUpdate = true;
	}

	if ( doUpdate )
	{
		updateLobbyForHost(LOBBY_UPDATE_DURING_GAME);
	}
}

void EOSFuncs::LobbyData_t::updateLobby()
{
	if ( !EOS.CurrentUserInfo.isValid() )
	{
		EOSFuncs::logError("updateLobby: invalid current user");
		return;
	}

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(EOS.PlatformHandle);

	EOS_Lobby_CopyLobbyDetailsHandleOptions CopyHandleOptions;
	CopyHandleOptions.ApiVersion = EOS_LOBBY_COPYLOBBYDETAILSHANDLE_API_LATEST;
	CopyHandleOptions.LobbyId = LobbyId.c_str();
	CopyHandleOptions.LocalUserId = EOS.CurrentUserInfo.getProductUserIdHandle();

	EOS_HLobbyDetails LobbyDetailsHandle = nullptr;
	EOS_EResult result = EOS_Lobby_CopyLobbyDetailsHandle(LobbyHandle, &CopyHandleOptions, &LobbyDetailsHandle);
	if ( result != EOS_EResult::EOS_Success )
	{
		EOSFuncs::logError("OnLobbyUpdateReceived: can't get lobby info handle. Error code: %d", static_cast<int>(result));
		return;
	}

	EOS.setLobbyDetailsFromHandle(LobbyDetailsHandle, this);
	EOS_LobbyDetails_Release(LobbyDetailsHandle);
}


void EOSFuncs::LobbyData_t::getLobbyMemberInfo(EOS_HLobbyDetails LobbyDetails)
{
	if ( !currentLobbyIsValid() )
	{
		EOSFuncs::logError("getLobbyMemberInfo: invalid current lobby - no ID set");
		return;
	}

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(EOS.PlatformHandle);
	EOS_LobbyDetails_GetMemberCountOptions MemberCountOptions;
	MemberCountOptions.ApiVersion = EOS_LOBBYDETAILS_GETMEMBERCOUNT_API_LATEST;

	Uint32 numPlayers = EOS_LobbyDetails_GetMemberCount(LobbyDetails, &MemberCountOptions);
	EOSFuncs::logInfo("getLobbyMemberInfo: NumPlayers in lobby: %d", numPlayers);

	// so we don't have to wait for a new callback to retrieve names
	std::unordered_map<EOS_ProductUserId, std::string> previousPlayerNames; 
	for ( auto& player : playersInLobby )
	{
		EOS_ProductUserId id = EOSFuncs::Helpers_t::productIdFromString(player.memberProductUserId.c_str());
		if ( id )
		{
			previousPlayerNames.insert(std::pair<EOS_ProductUserId, std::string>(id, player.name));
		}
	}
	this->playersInLobby.clear();

	EOS_LobbyDetails_GetMemberByIndexOptions MemberByIndexOptions;
	MemberByIndexOptions.ApiVersion = EOS_LOBBYDETAILS_GETMEMBERBYINDEX_API_LATEST;
	for ( int i = 0; i < numPlayers; ++i )
	{
		MemberByIndexOptions.MemberIndex = i;
		EOS_ProductUserId memberId = EOS_LobbyDetails_GetMemberByIndex(LobbyDetails, &MemberByIndexOptions);
		EOSFuncs::logInfo("getLobbyMemberInfo: Lobby Player ID: %s", EOSFuncs::Helpers_t::productIdToString(memberId));

		PlayerLobbyData_t newPlayer;
		newPlayer.memberProductUserId = EOSFuncs::Helpers_t::productIdToString(memberId);
		newPlayer.name = "Pending...";
		auto idMapping = EOS.AccountMappings.find(memberId);
		if ( idMapping != EOS.AccountMappings.end() && idMapping->second != nullptr )
		{
			newPlayer.memberEpicAccountId = EOSFuncs::Helpers_t::epicIdToString(idMapping->second);
		}

		auto previousPlayer = previousPlayerNames.find(memberId);
		if ( previousPlayer != previousPlayerNames.end() )
		{
			// replace "pending..." with the player name we previously knew about.
			newPlayer.name = previousPlayer->second;
		}

		//member attributes
		EOS_LobbyDetails_GetMemberAttributeCountOptions MemberAttributeCountOptions;
		MemberAttributeCountOptions.ApiVersion = EOS_LOBBYDETAILS_GETMEMBERATTRIBUTECOUNT_API_LATEST;
		MemberAttributeCountOptions.TargetUserId = memberId;
		const Uint32 numAttributes = EOS_LobbyDetails_GetMemberAttributeCount(LobbyDetails, &MemberAttributeCountOptions);
		for ( int j = 0; j < numAttributes; ++j )
		{
			EOS_LobbyDetails_CopyMemberAttributeByIndexOptions MemberAttributeCopyOptions;
			MemberAttributeCopyOptions.ApiVersion = EOS_LOBBYDETAILS_COPYMEMBERATTRIBUTEBYINDEX_API_LATEST;
			MemberAttributeCopyOptions.AttrIndex = j;
			MemberAttributeCopyOptions.TargetUserId = memberId;
			EOS_Lobby_Attribute* MemberAttribute = nullptr;
			EOS_EResult result = EOS_LobbyDetails_CopyMemberAttributeByIndex(LobbyDetails, &MemberAttributeCopyOptions, &MemberAttribute);
			if ( result != EOS_EResult::EOS_Success )
			{
				EOSFuncs::logError("getLobbyMemberInfo: can't copy member attribute, error code: %d", 
					static_cast<int>(result));
				continue;
			}

			std::string key = MemberAttribute->Data->Key;
			if ( key.compare("CLIENTNUM") == 0 )
			{
				newPlayer.clientNumber = MemberAttribute->Data->Value.AsInt64;
				EOSFuncs::logInfo("Read clientnum: %d for user: %s", newPlayer.clientNumber, newPlayer.memberProductUserId.c_str());
			}
			EOS_Lobby_Attribute_Release(MemberAttribute);
		}


		this->playersInLobby.push_back(newPlayer);
		this->lobbyMembersQueueToMappingUpdate.push_back(memberId);
	}

	EOS.queryAccountIdFromProductId(this);
}

void EOSFuncs::queryAccountIdFromProductId(LobbyData_t* lobby/*, std::vector<EOS_ProductUserId>& accountsToQuery*/)
{
	if ( !lobby )
	{
		return;
	}
	if ( lobby->lobbyMembersQueueToMappingUpdate.empty() )
	{
		return;
	}
	EOS_Connect_QueryProductUserIdMappingsOptions QueryOptions = {};
	QueryOptions.ApiVersion = EOS_CONNECT_QUERYPRODUCTUSERIDMAPPINGS_API_LATEST;
	QueryOptions.AccountIdType_DEPRECATED = EOS_EExternalAccountType::EOS_EAT_EPIC;
	QueryOptions.LocalUserId = CurrentUserInfo.getProductUserIdHandle();

	QueryOptions.ProductUserIdCount = lobby->lobbyMembersQueueToMappingUpdate.size();
	QueryOptions.ProductUserIds = lobby->lobbyMembersQueueToMappingUpdate.data();

	for ( EOS_ProductUserId& id : lobby->lobbyMembersQueueToMappingUpdate )
	{
		auto find = AccountMappings.find(id);
		if ( find == AccountMappings.end() || (*find).second == nullptr )
		{
			// if the mapping doesn't exist, add to set. otherwise we already know the account id for the given product id
			ProductIdsAwaitingAccountMappingCallback.insert(id);
		}
		else
		{
			// kick off the user info query since we know the data.
			getUserInfo(AccountMappings[id], EOSFuncs::USER_INFO_QUERY_LOBBY_MEMBER, 0);
		}
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
	switch ( type )
	{
		case LOBBY_NAME:
			attributePair.first = "NAME";
			attributePair.second = this->LobbyAttributes.lobbyName;
			break;
		case GAME_VERSION:
			attributePair.first = "VER";
			attributePair.second = this->LobbyAttributes.gameVersion;
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
		case GAME_MODS:
			attributePair.first = "SVNUMMODS";
			attributePair.second = "0";
			break;
		case CREATION_TIME:
			attributePair.first = "CREATETIME";
			attributePair.second = std::to_string(this->LobbyAttributes.lobbyCreationTime);
			break;
		case GAME_CURRENT_LEVEL:
			attributePair.first = "CURRENTLEVEL";
			attributePair.second = std::to_string(this->LobbyAttributes.gameCurrentLevel);
			break;
		default:
			break;
	}
	return attributePair;
}

void EOSFuncs::LobbyData_t::setLobbyAttributesAfterReading(EOS_Lobby_AttributeData* data)
{
	std::string keyName = data->Key;
	if ( keyName.compare("NAME") == 0 )
	{
		this->LobbyAttributes.lobbyName = data->Value.AsUtf8;
	}
	else if ( keyName.compare("VER") == 0 )
	{
		this->LobbyAttributes.gameVersion = data->Value.AsUtf8;
	}
	else if ( keyName.compare("SVFLAGS") == 0 )
	{
		this->LobbyAttributes.serverFlags = std::stoi(data->Value.AsUtf8);
	}
	else if ( keyName.compare("LOADINGSAVEGAME") == 0 )
	{
		this->LobbyAttributes.isLobbyLoadingSavedGame = std::stoul(data->Value.AsUtf8);
	}
	else if ( keyName.compare("SVNUMMODS") == 0 )
	{
		this->LobbyAttributes.numServerMods = std::stoi(data->Value.AsUtf8);
	}
	else if ( keyName.compare("CREATETIME") == 0 )
	{
		this->LobbyAttributes.lobbyCreationTime = std::stoll(data->Value.AsUtf8);
	}
	else if ( keyName.compare("CURRENTLEVEL") == 0 )
	{
		this->LobbyAttributes.gameCurrentLevel = std::stoi(data->Value.AsUtf8);
	}
}

EOS_ProductUserId EOSFuncs::P2PConnectionInfo_t::getPeerIdFromIndex(int index)
{
	if ( index == 0 && multiplayer == CLIENT )
	{
		return serverProductId;
	}

	for ( auto& pair : peerProductIds )
	{
		if ( pair.second == index )
		{
			return pair.first;
		}
	}
	//logError("getPeerIdFromIndex: no peer with index: %d", index);
	return nullptr;
}

int EOSFuncs::P2PConnectionInfo_t::getIndexFromPeerId(EOS_ProductUserId id)
{
	if ( !id )
	{
		return 0;
	}
	for ( auto& pair : peerProductIds )
	{
		if ( pair.first == id )
		{
			return pair.second;
		}
	}
	logError("getPeerIdFromIndex: no peer with id: %s", EOSFuncs::Helpers_t::productIdToString(id));
	return 0;
}

bool EOSFuncs::P2PConnectionInfo_t::isPeerIndexed(EOS_ProductUserId id)
{
	if ( id == serverProductId )
	{
		return true;
	}
	for ( auto& pair : peerProductIds )
	{
		if ( pair.first == id )
		{
			return true;
		}
	}
	return false;
}

bool EOSFuncs::P2PConnectionInfo_t::assignPeerIndex(EOS_ProductUserId id, int index)
{
	for ( auto& pair : peerProductIds )
	{
		if ( pair.first == id )
		{
			pair.second = index;
			return true;
		}
	}
	return false;
}

bool EOSFuncs::P2PConnectionInfo_t::isPeerStillValid(int index)
{
	if ( !getPeerIdFromIndex(index) )
	{
		return false;
	}
	if ( !Helpers_t::productIdIsValid(getPeerIdFromIndex(index)) )
	{
		return false;
	}
	return true;
}

void EOSFuncs::LobbyData_t::SubscribeToLobbyUpdates()
{
	UnsubscribeFromLobbyUpdates();

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(EOS.PlatformHandle);

	EOS_Lobby_AddNotifyLobbyUpdateReceivedOptions UpdateNotifyOptions;
	UpdateNotifyOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYUPDATERECEIVED_API_LATEST;

	LobbyUpdateNotification = EOS_Lobby_AddNotifyLobbyUpdateReceived(LobbyHandle, &UpdateNotifyOptions, nullptr, OnLobbyUpdateReceived);

	EOS_Lobby_AddNotifyLobbyMemberUpdateReceivedOptions MemberUpdateOptions;
	MemberUpdateOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYMEMBERUPDATERECEIVED_API_LATEST;
	LobbyMemberUpdateNotification = EOS_Lobby_AddNotifyLobbyMemberUpdateReceived(LobbyHandle, &MemberUpdateOptions, nullptr, OnMemberUpdateReceived);

	EOS_Lobby_AddNotifyLobbyMemberStatusReceivedOptions MemberStatusOptions;
	MemberStatusOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYMEMBERSTATUSRECEIVED_API_LATEST;
	LobbyMemberStatusNotification = EOS_Lobby_AddNotifyLobbyMemberStatusReceived(LobbyHandle, &MemberStatusOptions, nullptr, OnMemberStatusReceived);

	EOSFuncs::logInfo("SubscribeToLobbyUpdates: %s", this->LobbyId.c_str());
}

void EOSFuncs::LobbyData_t::UnsubscribeFromLobbyUpdates()
{
	if ( LobbyUpdateNotification != EOS_INVALID_NOTIFICATIONID )
	{
		EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(EOS.PlatformHandle);

		EOS_Lobby_RemoveNotifyLobbyUpdateReceived(LobbyHandle, LobbyUpdateNotification);
		LobbyUpdateNotification = EOS_INVALID_NOTIFICATIONID;
	}

	if ( LobbyMemberUpdateNotification != EOS_INVALID_NOTIFICATIONID )
	{
		EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(EOS.PlatformHandle);

		EOS_Lobby_RemoveNotifyLobbyMemberUpdateReceived(LobbyHandle, LobbyMemberUpdateNotification);
		LobbyMemberUpdateNotification = EOS_INVALID_NOTIFICATIONID;
	}

	if ( LobbyMemberStatusNotification != EOS_INVALID_NOTIFICATIONID )
	{
		EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(EOS.PlatformHandle);

		EOS_Lobby_RemoveNotifyLobbyMemberStatusReceived(LobbyHandle, LobbyMemberStatusNotification);
		LobbyMemberStatusNotification = EOS_INVALID_NOTIFICATIONID;
	}

	EOSFuncs::logInfo("UnsubscribeFromLobbyUpdates: %s", this->LobbyId.c_str());
}

void EOSFuncs::showFriendsOverlay()
{
	UIHandle = EOS_Platform_GetUIInterface(PlatformHandle);
	EOS_UI_SetDisplayPreferenceOptions DisplayOptions;
	DisplayOptions.ApiVersion = EOS_UI_SETDISPLAYPREFERENCE_API_LATEST;
	DisplayOptions.NotificationLocation = EOS_UI_ENotificationLocation::EOS_UNL_TopRight;

	EOS_EResult result = EOS_UI_SetDisplayPreference(UIHandle, &DisplayOptions);
	EOSFuncs::logInfo("showFriendsOverlay: result: %d", static_cast<int>(result));

	UIHandle = EOS_Platform_GetUIInterface(PlatformHandle);
	EOS_UI_ShowFriendsOptions Options = {};
	Options.ApiVersion = EOS_UI_SHOWFRIENDS_API_LATEST;
	Options.LocalUserId = EOSFuncs::Helpers_t::epicIdFromString(CurrentUserInfo.epicAccountId.c_str());

	EOS_UI_ShowFriends(UIHandle, &Options, nullptr, ShowFriendsCallback);


}

void EOS_CALL EOSFuncs::ShowFriendsCallback(const EOS_UI_ShowFriendsCallbackInfo* data)
{
	if ( data )
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
	if ( hostname.compare("") == 0 )
	{
		Credentials.Id = nullptr;
	}
	else
	{
		Credentials.Id = hostname.c_str();
	}
	if ( tokenName.compare("") == 0 )
	{
		Credentials.Token = nullptr;
	}
	else
	{
		Credentials.Token = tokenName.c_str();
	}
	Credentials.Type = EOS.AccountManager.AuthType;
	if ( Credentials.Type == EOS_ELoginCredentialType::EOS_LCT_Developer )
	{
		EOSFuncs::logInfo("Connecting to \'%s\'...", hostname.c_str());
	}
	else if ( Credentials.Type == EOS_ELoginCredentialType::EOS_LCT_ExchangeCode )
	{
		EOSFuncs::logInfo("Connecting via exchange token...");
	}

	EOS_Auth_LoginOptions LoginOptions = {};
	LoginOptions.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;
	LoginOptions.ScopeFlags = static_cast<EOS_EAuthScopeFlags>(EOS_EAuthScopeFlags::EOS_AS_BasicProfile | EOS_EAuthScopeFlags::EOS_AS_FriendsList | EOS_EAuthScopeFlags::EOS_AS_Presence);
	LoginOptions.Credentials = &Credentials;

	EOS_Auth_Login(AuthHandle, &LoginOptions, NULL, AuthLoginCompleteCallback);

	AddConnectAuthExpirationNotification();

	EOS.AccountManager.waitingForCallback = true;
	//Uint32 startAuthTicks = SDL_GetTicks();
	//Uint32 currentAuthTicks = startAuthTicks;
	//while ( EOS.AccountManager.AccountAuthenticationStatus == EOS_EResult::EOS_NotConfigured )
	//{
	//	EOS_Platform_Tick(PlatformHandle);
	//	SDL_Delay(50);
	//	currentAuthTicks = SDL_GetTicks();
	//	if ( currentAuthTicks - startAuthTicks >= 30000 ) // 30 second timeout.
	//	{
	//		AccountAuthenticationStatus = EOS_EResult::EOS_InvalidAuth;
	//		logError("initAuth: timeout attempting to log in");
	//		return false;
	//	}
	//}
	/*if ( EOS.AccountManager.AccountAuthenticationStatus == EOS_EResult::EOS_Success )
	{
		return true;
	}
	else
	{
		return false;
	}*/
	return true;
}

void EOSFuncs::LobbySearchResults_t::sortResults()
{
	resultsSortedForDisplay.clear();
	if ( results.empty() )
	{
		return;
	}

	int index = 0;
	for ( auto it = results.begin(); it != results.end(); ++it )
	{
		resultsSortedForDisplay.push_back(std::make_pair(EOSFuncs::LobbyData_t::LobbyAttributes_t((*it).LobbyAttributes), index));
		++index;
	}
	std::sort(resultsSortedForDisplay.begin(), resultsSortedForDisplay.end(), 
		[](const std::pair<EOSFuncs::LobbyData_t::LobbyAttributes_t, int>& lhs, const std::pair<EOSFuncs::LobbyData_t::LobbyAttributes_t, int>& rhs)
		{
			if ( lhs.first.gameCurrentLevel == -1 && rhs.first.gameCurrentLevel == -1 )
			{
				return (lhs.first.lobbyCreationTime > rhs.first.lobbyCreationTime);
			}
			return lhs.first.gameCurrentLevel < rhs.first.gameCurrentLevel;
		}
	);
}

void buttonRetryAuthorisation(button_t* my)
{
	/*if ( EOS.AccountManager.waitingForCallback )
	{
		return;
	}*/
	EOS.AccountManager.AccountAuthenticationStatus = EOS_EResult::EOS_NotConfigured;
	EOS.initAuth();
}

void EOSFuncs::Accounts_t::createLoginDialogue()
{
	if ( initPopupWindow )
	{
		return;
	}
	initPopupWindow = true;
	popupInitTicks = ticks;
	popupCurrentTicks = ticks;

	if ( !loginBanner )
	{
		loginBanner = loadImage("images/system/title.png");
	}

	// close current window
	buttonCloseSubwindow(NULL);
	list_FreeAll(&button_l);
	deleteallbuttons = true;

	// create new window
	subwindow = 1;
	subx1 = xres / 2 - ((loginBanner->w / 2)+ 16);
	subx2 = xres / 2 + ((loginBanner->w / 2) + 16);
	suby1 = yres / 2 - ((loginBanner->h / 2) + 64);
	suby2 = yres / 2 + ((loginBanner->h / 2) + 64);
	strcpy(subtext, "");

	// close button
	button_t* button;
	// retry button
	button = newButton();
	strcpy(button->label, "Retry login");
	button->x = subx1 + 4;
	button->y = suby2 - 24;
	button->sizex = strlen("Retry login") * 12 + 8;
	button->sizey = 20;
	button->visible = 1;
	button->focused = 1;
	button->action = &buttonRetryAuthorisation;

	// qtd button
	button = newButton();
	strcpy(button->label, "Quit to desktop");
	button->sizex = strlen("Quit to desktop") * 12 + 4;
	button->sizey = 20;
	button->x = subx2 - button->sizex - 8;
	button->y = suby2 - 24;
	button->visible = 1;
	button->focused = 1;
	button->action = &buttonQuitConfirm;
}

void EOSFuncs::Accounts_t::drawDialogue()
{
	Uint32 oldTicks = popupCurrentTicks;
	popupCurrentTicks = ticks;

	if ( fadeout )
	{
		return;
	}

	int centerWindowX = subx1 + (subx2 - subx1) / 2;
	if ( loginBanner )
	{
		SDL_Rect pos;
		pos.x = centerWindowX - loginBanner->w / 2;
		pos.y = suby1 + 4;
		pos.w = loginBanner->w;
		pos.h = loginBanner->h;
		drawImage(loginBanner, nullptr, &pos);
	}

	ttfPrintTextFormatted(ttf12, centerWindowX - strlen(language[3936]) * TTF12_WIDTH / 2, suby2 + 8 - TTF12_HEIGHT * 9, language[3936]);

	char messageBuffer[512] = "";
	strcpy(messageBuffer, language[3937]);
	if ( popupCurrentTicks % TICKS_PER_SECOND == 0 && oldTicks != popupCurrentTicks )
	{
		++loadingTicks;
		if ( loadingTicks > 3 )
		{
			loadingTicks = 0;
		}
	}
	switch ( loadingTicks )
	{
		case 0:
			break;
		case 1:
			strcat(messageBuffer, ".");
			break;
		case 2:
			strcat(messageBuffer, "..");
			break;
		case 3:
			strcat(messageBuffer, "...");
			break;
		default:
			break;
	}

	if ( waitingForCallback )
	{
		ttfPrintTextFormatted(ttf12, centerWindowX - strlen(messageBuffer) * TTF12_WIDTH / 2, suby2 + 8 - TTF12_HEIGHT * 8, messageBuffer);
	}
	else
	{
		if ( EOS.AccountManager.AccountAuthenticationStatus == EOS_EResult::EOS_Success )
		{
			ttfPrintTextFormattedColor(ttf12, centerWindowX - strlen(language[3941]) * TTF12_WIDTH / 2, suby2 + 16 - TTF12_HEIGHT * 8,
				SDL_MapRGB(mainsurface->format, 0, 255, 0), language[3941]);
		}
		else if ( EOS.AccountManager.AccountAuthenticationStatus != EOS_EResult::EOS_NotConfigured )
		{
			// general error messages
			if ( !firstTimeSetupCompleted && EOS.AccountManager.AccountAuthenticationStatus == EOS_EResult::EOS_Auth_ExchangeCodeNotFound )
			{
				if ( !loginCriticalErrorOccurred )
				{
					list_FreeAll(&button_l);
					deleteallbuttons = true;

					// qtd button
					button_t* button = newButton();
					strcpy(button->label, "Quit to desktop");
					button->sizex = strlen("Quit to desktop") * 12 + 4;
					button->sizey = 20;
					button->x = subx1 + ((subx2 - subx1) / 2) - (button->sizex / 2);
					button->y = suby2 - 24;
					button->visible = 1;
					button->focused = 1;
					button->action = &buttonQuitConfirm;
				}
				loginCriticalErrorOccurred = true;
				ttfPrintTextFormattedColor(ttf12, centerWindowX - strlen(language[3942]) * TTF12_WIDTH / 2, suby2 + 16 - TTF12_HEIGHT * 8,
					SDL_MapRGB(mainsurface->format, 255, 128, 0), language[3942]);
				ttfPrintTextFormattedColor(ttf12, centerWindowX - strlen(language[3943]) * TTF12_WIDTH / 2, suby2 + 16 - TTF12_HEIGHT * 7,
					SDL_MapRGB(mainsurface->format, 255, 128, 0), language[3943]);
				ttfPrintTextFormattedColor(ttf12, centerWindowX - strlen(language[3944]) * TTF12_WIDTH / 2, suby2 + 16 - TTF12_HEIGHT * 5,
					SDL_MapRGB(mainsurface->format, 255, 255, 0), language[3944]);
			}
			else
			{
				char errorBuf[512] = "";
				snprintf(errorBuf, 512 - 1, language[3941], static_cast<int>(EOS.AccountManager.AccountAuthenticationStatus));
				ttfPrintTextFormattedColor(ttf12, centerWindowX - strlen(language[3941]) * TTF12_WIDTH / 2, suby2 + 16 - TTF12_HEIGHT * 8,
					SDL_MapRGB(mainsurface->format, 255, 0, 0), errorBuf);
			}
		}
	}

	if ( !firstTimeSetupCompleted && EOS.AccountManager.AccountAuthenticationStatus == EOS_EResult::EOS_NotConfigured )
	{
		if ( popupCurrentTicks - popupInitTicks >= TICKS_PER_SECOND * 3 )
		{
			suby1 = yres / 2 - ((loginBanner->h / 2) + 64);
			suby2 = yres / 2 + ((loginBanner->h / 2) + 64);

			ttfPrintTextFormattedColor(ttf12, centerWindowX - strlen(language[3938]) * TTF12_WIDTH / 2, suby2 + 8 - TTF12_HEIGHT * 6, 
				SDL_MapRGB(mainsurface->format, 255, 255, 0), language[3938]);
			ttfPrintTextFormattedColor(ttf12, centerWindowX - strlen(language[3939]) * TTF12_WIDTH / 2, suby2 + 8 - TTF12_HEIGHT * 5,
				SDL_MapRGB(mainsurface->format, 255, 255, 0), language[3939]);
			ttfPrintTextFormattedColor(ttf12, centerWindowX - strlen(language[3940]) * TTF12_WIDTH / 2, suby2 + 16 - TTF12_HEIGHT * 4,
				SDL_MapRGB(mainsurface->format, 255, 255, 0), language[3940]);
		}
	}
}

void EOSFuncs::Accounts_t::handleLogin()
{
	if ( AccountAuthenticationStatus == EOS_EResult::EOS_Success || AccountAuthenticationCompleted == EOS_EResult::EOS_Success )
	{
		firstTimeSetupCompleted = true;
		if ( AccountAuthenticationCompleted != EOS_EResult::EOS_Success )
		{
			if ( popupType == POPUP_FULL )
			{
				// close this popup.
				buttonCloseSubwindow(nullptr);
				list_FreeAll(&button_l);
				deleteallbuttons = true;
			}
			AccountAuthenticationCompleted = EOS_EResult::EOS_Success;
		}
		return;
	}
	AccountAuthenticationCompleted = EOS_EResult::EOS_NotConfigured;

	if ( !initPopupWindow && firstTimeSetupCompleted )
	{
		popupType = POPUP_TOAST;
	}

	if ( popupType == POPUP_FULL )
	{
		if ( !initPopupWindow )
		{
			createLoginDialogue();
		}
	}
	else if ( popupType == POPUP_TOAST )
	{
		if ( !initPopupWindow )
		{
			UIToastNotificationManager.addNotification(UIToastNotificationManager_t::GENERIC_TOAST_IMAGE,
				"Logging in...",
				"",
				"");
			initPopupWindow = true;
		}
		UIToastNotificationManager.drawNotifications();
	}

	drawDialogue();
}
#endif //USE_EOS
