/*-------------------------------------------------------------------------------

BARONY
File: lobbies.hpp
Desc: header for lobbies.cpp (matchmaking handlers)

Copyright 2013-2020 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once
#include <utility>
#include <vector>

class LobbyHandler_t
{
	enum LobbyServiceType : int
	{
		LOBBY_DISABLE,
		LOBBY_STEAM,
		LOBBY_CROSSPLAY,
		LOBBY_COMBINED
	};
	LobbyServiceType connectionType = LOBBY_DISABLE;
	LobbyServiceType searchType = LOBBY_DISABLE;
	const int kNumSearchResults = 200;
public:
	LobbyHandler_t() :
		lobbyDisplayedSearchResults(kNumSearchResults, std::make_pair(-1, LOBBY_DISABLE))
	{
#if defined STEAMWORKS && !defined USE_EOS
		connectionType = LOBBY_STEAM;
		searchType = LOBBY_STEAM;
#elif !defined STEAMWORKS && defined USE_EOS
		connectionType = LOBBY_CROSSPLAY;
		searchType = LOBBY_CROSSPLAY;
#elif defined STEAMWORKS && defined USE_EOS
		connectionType = LOBBY_STEAM;
		searchType = LOBBY_COMBINED;
#endif
	};

	void handleLobbyListRequests();
	void handleLobbyBrowser();
	void updateSearchResults();
	LobbyServiceType getDisplayedResultLobbyType(int selection);
	Sint32 getDisplayedResultLobbyIndex(int selection);
	std::vector<std::pair<Sint32, LobbyServiceType>> lobbyDisplayedSearchResults;
	Uint32 numLobbyDisplaySearchResults = 0;
	int selectedLobbyInList = -1;
};
extern LobbyHandler_t LobbyHandler;