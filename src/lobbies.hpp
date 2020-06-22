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
public:
	LobbyHandler_t() :
		lobbyDisplayedSearchResults(kNumSearchResults, std::make_pair(-1, LOBBY_DISABLE))
	{
#if defined STEAMWORKS && !defined USE_EOS
		connectionType = LOBBY_STEAM;
		searchType = LOBBY_STEAM;
		joiningType = LOBBY_STEAM;
		hostingType = LOBBY_STEAM;
		P2PType = LOBBY_STEAM;
#elif !defined STEAMWORKS && defined USE_EOS
		connectionType = LOBBY_CROSSPLAY;
		searchType = LOBBY_CROSSPLAY;
		joiningType = LOBBY_CROSSPLAY;
		hostingType = LOBBY_CROSSPLAY;
		P2PType = LOBBY_CROSSPLAY;
#elif defined STEAMWORKS && defined USE_EOS
		connectionType = LOBBY_STEAM;
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
	LobbyServiceType connectionType = LOBBY_DISABLE;
	LobbyServiceType hostingType = LOBBY_DISABLE;
	LobbyServiceType joiningType = LOBBY_DISABLE;
	LobbyServiceType searchType = LOBBY_DISABLE;
	LobbyServiceType P2PType = LOBBY_DISABLE;
	void handleLobbyListRequests();
	void handleLobbyBrowser();
	void updateSearchResults();
	LobbyServiceType getDisplayedResultLobbyType(int selection);
	Sint32 getDisplayedResultLobbyIndex(int selection);
	std::vector<std::pair<Sint32, LobbyServiceType>> lobbyDisplayedSearchResults;
	Uint32 numLobbyDisplaySearchResults = 0;
	int selectedLobbyInList = 0;
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
};
extern LobbyHandler_t LobbyHandler;

//if ( LobbyHandler.getJoiningType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
//{
//}
//else if ( LobbyHandler.getJoiningType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
