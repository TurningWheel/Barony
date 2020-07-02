/*-------------------------------------------------------------------------------

BARONY
File: lobbies.hpp
Desc: header for lobbies.cpp (matchmaking handlers)

Copyright 2013-2020 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

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
	void handleLobbyBrowser();
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
	bool settings_crossplayEnabled = false;

	LobbyServiceType getHostingType()
	{
		return hostingType;
	}
	LobbyServiceType getJoiningType()
	{
		return joiningType;
	}
	LobbyServiceType getP2PType()
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
		LOBBY_USING_SAVEGAME = 50000,
		LOBBY_WRONG_SAVEGAME,
		LOBBY_NOT_USING_SAVEGAME,
		LOBBY_NO_OWNER,
		LOBBY_GAME_IN_PROGRESS,
		LOBBY_UNHANDLED_ERROR,
		LOBBY_JOIN_CANCELLED,
		LOBBY_JOIN_TIMEOUT,
		LOBBY_NOT_FOUND,
		LOBBY_TOO_MANY_PLAYERS
	};
};
extern LobbyHandler_t LobbyHandler;