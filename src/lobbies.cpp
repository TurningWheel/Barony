/*-------------------------------------------------------------------------------

BARONY
File: lobbies.cpp
Desc: contains code for matchmaking handlers

Copyright 2013-2020 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "menu.hpp"
#include "game.hpp"
#include "lobbies.hpp"
#include "eos.hpp"
#include "steam.hpp"

LobbyHandler_t LobbyHandler;

void LobbyHandler_t::handleLobbyListRequests()
{
	if ( !intro || connectionType == LOBBY_DISABLE )
	{
		return;
	}

#if !defined STEAMWORKS && !defined USE_EOS
	return;
#endif


	if ( connectionType == LOBBY_STEAM )
	{
#ifdef STEAMWORKS
		// lobby list request succeeded
		if ( !requestingLobbies && !strcmp(subtext, language[1132]) )
		{
			openSteamLobbyBrowserWindow(NULL);
		}

		// lobby entered
		if ( !connectingToLobby && connectingToLobbyWindow )
		{
			connectingToLobbyWindow = false;
			connectingToLobby = false;

			// close current window
			buttonCloseSubwindow(NULL);
			list_FreeAll(&button_l);
			deleteallbuttons = true;

			// we are assuming here that the lobby join was successful
			// otherwise, the callback would've flipped off the connectingToLobbyWindow and opened an error window

			// get number of lobby members (capped to game limit)

			// record CSteamID of lobby owner (and nobody else)
			int lobbyMembers = SteamMatchmaking()->GetNumLobbyMembers(*static_cast<CSteamID*>(currentLobby));
			if ( steamIDRemote[0] )
			{
				cpp_Free_CSteamID(steamIDRemote[0]);
			}
			steamIDRemote[0] = cpp_SteamMatchmaking_GetLobbyOwner(currentLobby); //TODO: Bugger void pointers!
			int c;
			for ( c = 1; c < MAXPLAYERS; c++ )
			{
				if ( steamIDRemote[c] )
				{
					cpp_Free_CSteamID(steamIDRemote[c]);
					steamIDRemote[c] = NULL;
				}
			}
			for ( c = 1; c < lobbyMembers; ++c )
			{
				steamIDRemote[c] = cpp_SteamMatchmaking_GetLobbyMember(currentLobby, c);
			}
			buttonJoinLobby(NULL);
			// TODO - what if the server never replies? hangs indefinitely.
		}
#endif
	}
	else if ( connectionType == LOBBY_CROSSPLAY )
	{
#ifdef USE_EOS
		// lobby list request succeeded
		if ( !EOS.bRequestingLobbies && !strcmp(subtext, language[1132]) )
		{
			openSteamLobbyBrowserWindow(NULL);
		}

		// lobby entered
		if ( EOS.ConnectingToLobbyStatus != static_cast<int>(EOS_EResult::EOS_Success) )
		{
			// close current window
			buttonCloseSubwindow(NULL);
			list_FreeAll(&button_l);
			deleteallbuttons = true;

			openFailedConnectionWindow(CLIENT);
			strcpy(subtext, EOSFuncs::getLobbyJoinFailedConnectString(EOS.ConnectingToLobbyStatus).c_str());
			EOS.ConnectingToLobbyStatus = static_cast<int>(EOS_EResult::EOS_Success);
		}
		else if ( !EOS.bConnectingToLobby && EOS.bConnectingToLobbyWindow )
		{
			EOS.bConnectingToLobbyWindow = false;
			EOS.bConnectingToLobby = false;

			// close current window
			buttonCloseSubwindow(NULL);
			list_FreeAll(&button_l);
			deleteallbuttons = true;

			// we are assuming here that the lobby join was successful
			// otherwise, the callback would've flipped off the connectingToLobbyWindow and opened an error window
			buttonJoinLobby(NULL);
		}
	}
#endif
}