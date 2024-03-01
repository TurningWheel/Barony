/*-------------------------------------------------------------------------------

BARONY
File: lobbies.hpp
Desc: header for lobbies.cpp (matchmaking handlers)

Copyright 2013-2020 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include <string>

class LobbyHandler_t
{
	const int kNumSearchResults = 200;
	bool filterShowInProgressLobbies = false;
public:
	LobbyHandler_t() :
		lobbyDisplayedSearchResults(kNumSearchResults, std::make_pair(-1, LOBBY_DISABLE))
	{
#if defined STEAMWORKS && !defined USE_EOS
		searchType = LOBBY_STEAM;
		joiningType = LOBBY_STEAM;
		hostingType = LOBBY_STEAM;
		P2PType = LOBBY_STEAM;
#elif !defined STEAMWORKS && defined USE_EOS
		searchType = LOBBY_CROSSPLAY;
		joiningType = LOBBY_CROSSPLAY;
		hostingType = LOBBY_CROSSPLAY;
		P2PType = LOBBY_CROSSPLAY;
#elif defined STEAMWORKS && defined USE_EOS
		searchType = LOBBY_COMBINED;
		joiningType = LOBBY_STEAM;
		hostingType = LOBBY_STEAM;
		P2PType = LOBBY_STEAM;
#endif
	};

	enum LobbyServiceType : int
	{
		LOBBY_DISABLE,
		LOBBY_STEAM,
		LOBBY_CROSSPLAY,
		LOBBY_COMBINED
	};
	LobbyServiceType hostingType = LOBBY_DISABLE;
	LobbyServiceType joiningType = LOBBY_DISABLE;
	LobbyServiceType searchType = LOBBY_DISABLE;
	LobbyServiceType P2PType = LOBBY_DISABLE;
	void handleLobbyListRequests();
	void updateSearchResults();
	static void filterLobbyButton(button_t* my);
	static void searchLobbyWithFilter(button_t* my);
	void drawLobbyFilters();
	LobbyServiceType getDisplayedResultLobbyType(int selection);
	Sint32 getDisplayedResultLobbyIndex(int selection);
	std::vector<std::pair<Sint32, LobbyServiceType>> lobbyDisplayedSearchResults;
	Uint32 numLobbyDisplaySearchResults = 0;
	int selectedLobbyInList = 0;
	bool showLobbyFilters = false;

	bool crossplayEnabled = false;

	std::string getCurrentRoomKey() const;

	LobbyServiceType getHostingType() const
	{
		return hostingType;
	}
	LobbyServiceType getJoiningType() const
	{
		return joiningType;
	}
	LobbyServiceType getP2PType() const
	{
		return P2PType;
	}
	LobbyServiceType setLobbyJoinTypeOfCurrentSelection()
	{
		if ( getDisplayedResultLobbyType(selectedLobbyInList) != LOBBY_DISABLE )
		{
			joiningType = getDisplayedResultLobbyType(selectedLobbyInList);
		}
		return joiningType;
	}
	void setHostingType(LobbyServiceType type)
	{
		hostingType = type;
	}
	void setLobbyJoinType(LobbyServiceType type)
	{
	    joiningType = type;
	}
	void setP2PType(LobbyServiceType type)
	{
		P2PType = type;
	}
	static void logError(const char* str, ...)
	{
		char newstr[1024] = { 0 };
		va_list argptr;

		// format the content
		va_start(argptr, str);
		vsnprintf(newstr, 1023, str, argptr);
		va_end(argptr);
		printlog("[Lobbies Error]: %s", newstr);
	}
	static std::string getLobbyJoinFailedConnectString(int result);
#ifdef STEAMWORKS
	CSteamID steamLobbyToValidate = {};
	void steamValidateAndJoinLobby(CSteamID& id);
	bool validateSteamLobbyDataOnJoin();
#endif
	enum EResult_LobbyFailures : int
	{
	    LOBBY_NO_ERROR = 1,             // no error (success)
		LOBBY_USING_SAVEGAME = 50000,   // trying to join a savegame lobby with a new character
		LOBBY_WRONG_SAVEGAME,           // trying to join a savegame lobby with the wrong save file
		LOBBY_NOT_USING_SAVEGAME,       // trying to join a newgame lobby with a savegame
		LOBBY_NO_OWNER,                 // no one in lobby (ghost lobby)
		LOBBY_GAME_IN_PROGRESS,         // game is already in progress
		LOBBY_UNHANDLED_ERROR,          // unknown/unhandled error type
		LOBBY_JOIN_CANCELLED,           // cancelled join request
		LOBBY_JOIN_TIMEOUT,             // timeout connecting to server
		LOBBY_NOT_FOUND,                // server no longer exists
		LOBBY_TOO_MANY_PLAYERS,         // server is full
		LOBBY_NOT_ALLOWED,              // server won't allow you in for one reason or another
		LOBBY_YOU_ARE_BANNED,           // can't join lobby because you are banned
        LOBBY_TOO_MANY_JOINS,           // overloaded lobby with join requests
		LOBBY_SAVEGAME_REQUIRES_DLC		// our client does not have DLC detected for their savefile
	};
};
extern LobbyHandler_t LobbyHandler;
