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
	enum LobbyServiceType : int
	{
		LOBBY_DISABLE,
		LOBBY_STEAM,
		LOBBY_CROSSPLAY
	};
	LobbyServiceType connectionType = LOBBY_DISABLE;
public:
	LobbyHandler_t()
	{
#ifdef STEAMWORKS
		connectionType = LOBBY_STEAM
#elif defined USE_EOS
		connectionType = LOBBY_CROSSPLAY;
#endif // STEAMWORKS
	};

	void handleLobbyListRequests();
};
extern LobbyHandler_t LobbyHandler;