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
#ifdef USE_EOS
#include "eos.hpp"
#endif
#ifdef STEAMWORKS
#include "steam.hpp"
#endif
#include "draw.hpp"
#include "player.hpp"
#include "scores.hpp"
#include "interface/interface.hpp"
#include "colors.hpp"
#include "net.hpp"

LobbyHandler_t LobbyHandler;

std::string LobbyHandler_t::getCurrentRoomKey() const
{
    if (multiplayer != CLIENT && multiplayer != SERVER) {
        return "";
    }
    const LobbyServiceType type = multiplayer == SERVER ?
        getHostingType() : getJoiningType();
    if (type == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
#ifdef STEAMWORKS
        char roomkey[16];
        snprintf(roomkey, sizeof(roomkey), "s%s", getRoomCode());
        for (auto ptr = roomkey; *ptr != '\0'; ++ptr) {
            *ptr = (char)toupper((int)(*ptr));
        }
        return std::string(roomkey);
#endif // STEAMWORKS
    }
    else if (type == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
#ifdef USE_EOS
        char roomkey[16];
        snprintf(roomkey, sizeof(roomkey), "e%s",
            EOS.CurrentLobbyData.LobbyAttributes.gameJoinKey.c_str());
        for (auto ptr = roomkey; *ptr != '\0'; ++ptr) {
            *ptr = (char)toupper((int)(*ptr));
        }
        return std::string(roomkey);
#endif // USE_EOS
    }
    return "";
}

std::string LobbyHandler_t::getLobbyJoinFailedConnectString(int result)
{
	char buf[1024] = "";
	switch ( result )
	{
		case EResult_LobbyFailures::LOBBY_GAME_IN_PROGRESS:
			snprintf(buf, 1023, "Unable to join lobby:\nGame in progress not joinable.");
			break;
		case EResult_LobbyFailures::LOBBY_USING_SAVEGAME:
			snprintf(buf, 1023, "Unable to join lobby:\n%s", Language::get(1381));
			break;
		case EResult_LobbyFailures::LOBBY_NOT_USING_SAVEGAME:
			snprintf(buf, 1023, "Unable to join lobby:\nOnly new characters allowed.");
			break;
		case EResult_LobbyFailures::LOBBY_WRONG_SAVEGAME:
			snprintf(buf, 1023, "Unable to join lobby:\nIncompatible save game.");
			break;
		case EResult_LobbyFailures::LOBBY_JOIN_CANCELLED:
			snprintf(buf, 1023, "Lobby join cancelled.\nSafely leaving lobby.");
			break;
		case EResult_LobbyFailures::LOBBY_NO_OWNER:
			snprintf(buf, 1023, "Unable to join lobby:\nLobby has no host.");
			break;
		case EResult_LobbyFailures::LOBBY_NOT_FOUND:
			snprintf(buf, 1023, "Unable to join lobby:\nLobby no longer exists.");
			break;
		case EResult_LobbyFailures::LOBBY_TOO_MANY_PLAYERS:
			snprintf(buf, 1023, "Unable to join lobby:\nLobby is full.");
			break;
		case EResult_LobbyFailures::LOBBY_SAVEGAME_REQUIRES_DLC:
			snprintf(buf, 1023, "Unable to join lobby:\n%s", Language::get(6100));
			break;
#ifdef USE_EOS
#ifdef STEAMWORKS
		case static_cast<int>(EOS_EResult::EOS_InvalidUser):
			snprintf(buf, 1023, "Unable to join lobby:\nCrossplay not enabled.");
			break;
#else
		case static_cast<int>(EOS_EResult::EOS_InvalidUser):
			snprintf(buf, 1023, "Unable to join lobby:\nNot connected to Epic Online.");
			break;
#endif
		case static_cast<int>(EOS_EResult::EOS_NoChange) :
			snprintf(buf, 1023, "Unable to join lobby:\nNo match found.");
			break;
#ifdef STEAMWORKS
		case static_cast<int>(k_EResultNoMatch) :
			snprintf(buf, 1023, "Unable to join lobby:\nNo match found.");
			break;
#endif
		case static_cast<int>(EOS_EResult::EOS_NotFound):
			snprintf(buf, 1023, "Unable to join lobby:\nLobby no longer exists.");
			break;
		case static_cast<int>(EOS_EResult::EOS_Lobby_TooManyPlayers):
			snprintf(buf, 1023, "Unable to join lobby:\nLobby is full.");
			break;
#endif
		case EResult_LobbyFailures::LOBBY_JOIN_TIMEOUT:
#ifndef NINTENDO
			snprintf(buf, 1023, "Unable to join lobby:\nTimeout waiting for host.");
#else
			nxErrorPrompt(
				"Unable to join lobby. Timeout waiting for host.",
				"Unable to join lobby.\n\nTimeout waiting for host.\n\nPlease try again later.",
				result);
			snprintf(buf, 1023, "Unable to join lobby.");
#endif
			break;
		default:
#ifndef NINTENDO
			snprintf(buf, 1023, "Unable to join lobby:\nError code: %d.", result);
#else
			nxErrorPrompt(
				"Unable to join lobby. Invalid game version.",
				"Unable to join lobby. Invalid game version.\n\n"
				"Please check that your game version is up-to-date.\n\n"
				"If the error persists, please try again later.",
				result);
			snprintf(buf, 1023, "Unable to join lobby.");
#endif
			break;
	}
	printlog("[Lobbies Error]: %s", buf);
	return buf;
}

#ifdef STEAMWORKS
bool LobbyHandler_t::validateSteamLobbyDataOnJoin()
{
	bool errorOnJoin = false;
	const char* lsgStr = SteamMatchmaking()->GetLobbyData(steamLobbyToValidate, "loadingsavegame");
	const char* lukStr = SteamMatchmaking()->GetLobbyData(steamLobbyToValidate, "lobbyuniquekey");
	if ( lsgStr )
	{
		Uint32 lsg = atoi(lsgStr);
		Uint32 luk = lukStr ? atoi(lukStr) : 0;
		if ( lsg != loadingsavegame || luk != loadinglobbykey )
		{
			// loading save game, but incorrect assertion from client side.
			if ( loadingsavegame == 0 )
			{
				// try reload from your other savefiles since this didn't match the default savegameIndex.
				bool foundSave = false;
				int checkDLC = VALID_OK_CHARACTER;
				for ( int c = 0; c < SAVE_GAMES_MAX; ++c ) {
					auto info = getSaveGameInfo(false, c);
					if ( info.game_version != -1 ) {
						if ( info.player_num < info.players.size() )
						{
							checkDLC = info.players[info.player_num].isCharacterValidFromDLC();
							if ( checkDLC != VALID_OK_CHARACTER )
							{
								foundSave = false;
								break;
							}
						}
						if ( info.gamekey == lsg && info.lobbykey == luk ) {
							savegameCurrentFileIndex = c;
							foundSave = true;
							break;
						}
					}
				}

				if ( checkDLC != VALID_OK_CHARACTER )
				{
					connectingToLobbyStatus = LobbyHandler_t::EResult_LobbyFailures::LOBBY_SAVEGAME_REQUIRES_DLC;
					errorOnJoin = true;
				}
				else if ( foundSave )
				{
					loadingsavegame = lsg;
					loadinglobbykey = luk;
					auto info = getSaveGameInfo(false, savegameCurrentFileIndex);
					for ( int c = 0; c < MAXPLAYERS; ++c ) {
						if ( info.players_connected[c] ) {
							loadGame(c, info);
						}
					}
				}
				else
				{
					connectingToLobbyStatus = LobbyHandler_t::EResult_LobbyFailures::LOBBY_USING_SAVEGAME;
					errorOnJoin = true;
				}
			}
			else if ( loadingsavegame > 0 && lsg == 0 )
			{
				connectingToLobbyStatus = LobbyHandler_t::EResult_LobbyFailures::LOBBY_NOT_USING_SAVEGAME;
				errorOnJoin = true;
			}
			else if ( (loadingsavegame > 0 && lsg > 0) || (loadinglobbykey > 0 && luk > 0) )
			{
				connectingToLobbyStatus = LobbyHandler_t::EResult_LobbyFailures::LOBBY_WRONG_SAVEGAME;
				errorOnJoin = true;
			}
			else
			{
				connectingToLobbyStatus = LobbyHandler_t::EResult_LobbyFailures::LOBBY_UNHANDLED_ERROR;
				errorOnJoin = true;
			}
		}
	}

	if ( !errorOnJoin )
	{
		const int numMembers = SteamMatchmaking()->GetNumLobbyMembers(steamLobbyToValidate);
		if ( numMembers == 0 )
		{
			connectingToLobbyStatus = LobbyHandler_t::EResult_LobbyFailures::LOBBY_NO_OWNER;
			errorOnJoin = true;
		}
	}

	if ( errorOnJoin )
	{
		connectingToLobbyWindow = false;
		connectingToLobby = false;
		multiplayer = SINGLE;
	}

	return !errorOnJoin;
}
#endif

void LobbyHandler_t::handleLobbyListRequests()
{
	if ( !intro || joiningType == LOBBY_DISABLE )
	{
		return;
	}

#if !defined STEAMWORKS && !defined USE_EOS
	return;
#endif
	if ( joiningType == LOBBY_STEAM )
	{
#ifdef STEAMWORKS
		// lobby entered
		if ( connectingToLobbyStatus != EResult::k_EResultOK )
		{
			// close current window
			buttonCloseSubwindow(NULL);
			list_FreeAll(&button_l);
			deleteallbuttons = true;
			
			openFailedConnectionWindow(CLIENT);
			strcpy(subtext, LobbyHandler_t::getLobbyJoinFailedConnectString(static_cast<int>(connectingToLobbyStatus)).c_str());
			connectingToLobbyStatus = EResult::k_EResultOK;
		}
		else if ( !connectingToLobby && connectingToLobbyWindow )
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
		}
#endif
	}
	else if ( joiningType == LOBBY_CROSSPLAY )
	{
#ifdef USE_EOS
		// lobby entered
		if ( EOS.ConnectingToLobbyStatus != static_cast<int>(EOS_EResult::EOS_Success) )
		{
			// close current window
			buttonCloseSubwindow(NULL);
			list_FreeAll(&button_l);
			deleteallbuttons = true;

			openFailedConnectionWindow(CLIENT);
			strcpy(subtext, LobbyHandler_t::getLobbyJoinFailedConnectString(EOS.ConnectingToLobbyStatus).c_str());
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
#endif
	}


	// lobby list request succeeded?
	if ( !strcmp(subtext, Language::get(1132)) )
	{
		bool hasLobbyListRequestReturned = false;
		switch ( searchType )
		{
			case LOBBY_STEAM:
#ifdef STEAMWORKS
				hasLobbyListRequestReturned = !requestingLobbies;
#endif
				break;
			case LOBBY_CROSSPLAY:
#ifdef USE_EOS
				hasLobbyListRequestReturned = !EOS.bRequestingLobbies;
#endif // USE_EOS
				break;
			case LOBBY_COMBINED:
#if defined STEAMWORKS && defined USE_EOS
				hasLobbyListRequestReturned = !EOS.bRequestingLobbies && !requestingLobbies;
#endif
				break;
			default:
				break;
		}
		if ( hasLobbyListRequestReturned )
		{
			openSteamLobbyBrowserWindow(NULL);
		}
	}
}

void LobbyHandler_t::updateSearchResults()
{
	numLobbyDisplaySearchResults = 0;

#if !defined STEAMWORKS && !defined USE_EOS
	for ( auto& result : lobbyDisplayedSearchResults )
	{
		result.first = -1;
		result.second = LOBBY_DISABLE;
	}
	return;
#endif

	if ( searchType == LOBBY_STEAM )
	{
#ifdef STEAMWORKS
		Uint32 steamLobbyIndex = 0;
		for ( auto& result : lobbyDisplayedSearchResults )
		{
			if ( steamLobbyIndex >= MAX_STEAM_LOBBIES || steamLobbyIndex >= numSteamLobbies )
			{
				result.first = -1;
				result.second = LOBBY_DISABLE;
				continue;
			}
			if ( lobbyIDs[steamLobbyIndex] )
			{
				result.first = steamLobbyIndex;
				result.second = LOBBY_STEAM;
				++numLobbyDisplaySearchResults;
			}
			++steamLobbyIndex;
		}
#endif
	}
	else if ( searchType == LOBBY_CROSSPLAY )
	{
#ifdef USE_EOS
		Uint32 eosLobbyIndex = 0;
		const Uint32 numEOSLobbies = EOS.LobbySearchResults.resultsSortedForDisplay.size();
		for ( auto& result : lobbyDisplayedSearchResults )
		{
			if ( eosLobbyIndex >= numEOSLobbies )
			{
				result.first = -1;
				result.second = LOBBY_DISABLE;
				continue;
			}
			result.first = eosLobbyIndex;
			result.second = LOBBY_CROSSPLAY;
			++numLobbyDisplaySearchResults;
			++eosLobbyIndex;
		}
#endif // USE_EOS
	}
	else if ( searchType == LOBBY_COMBINED )
	{
#if defined STEAMWORKS && defined USE_EOS
		Uint32 steamLobbyIndex = 0;
		Uint32 eosLobbyIndex = 0;
		const Uint32 numEOSLobbies = EOS.LobbySearchResults.resultsSortedForDisplay.size();
		for ( auto& result : lobbyDisplayedSearchResults )
		{
			if ( steamLobbyIndex >= MAX_STEAM_LOBBIES || steamLobbyIndex >= numSteamLobbies )
			{
				if ( eosLobbyIndex >= numEOSLobbies )
				{
					result.first = -1;
					result.second = LOBBY_DISABLE;
					continue;
				}
				else
				{
					result.first = eosLobbyIndex;
					result.second = LOBBY_CROSSPLAY;
					++numLobbyDisplaySearchResults;
					++eosLobbyIndex;
				}
			}
			else
			{
				if ( lobbyIDs[steamLobbyIndex] )
				{
					result.first = steamLobbyIndex;
					result.second = LOBBY_STEAM;
					++numLobbyDisplaySearchResults;
				}
			}
			++steamLobbyIndex;
		}
#endif
	}
}

LobbyHandler_t::LobbyServiceType LobbyHandler_t::getDisplayedResultLobbyType(int selection)
{
	if ( selection < 0 || selection >= kNumSearchResults )
	{
		return LOBBY_DISABLE;
	}
	return lobbyDisplayedSearchResults.at(selection).second;
}
Sint32 LobbyHandler_t::getDisplayedResultLobbyIndex(int selection)
{
	if ( selection < 0 || selection >= kNumSearchResults )
	{
		return -1;
	}
	return lobbyDisplayedSearchResults.at(selection).first;
}

#ifdef STEAMWORKS
void LobbyHandler_t::steamValidateAndJoinLobby(CSteamID& id)
{
	steamLobbyToValidate.Set(id.GetAccountID(), id.GetEUniverse(), id.GetEAccountType());
	steamLobbyToValidate.SetAccountInstance(id.GetUnAccountInstance());
	if ( !SteamMatchmaking()->RequestLobbyData(steamLobbyToValidate) )
	{
		steamLobbyToValidate.SetAccountID(0);
	}
}
#endif

void LobbyHandler_t::filterLobbyButton(button_t* my)
{
	LobbyHandler.showLobbyFilters = !LobbyHandler.showLobbyFilters;
}

void LobbyHandler_t::searchLobbyWithFilter(button_t* my)
{
#ifdef USE_EOS
	EOS.LobbySearchResults.showLobbiesInProgress = LobbyHandler.filterShowInProgressLobbies;

	if ( strcmp(EOS.lobbySearchByCode, "") != 0 )
	{
		EOS.LobbySearchResults.useLobbyCode = true;
		strcpy(EOS.LobbySearchResults.lobbyLastSearchByCode, EOS.lobbySearchByCode);
	}
	else
	{
		strcpy(EOS.LobbySearchResults.lobbyLastSearchByCode, "");
	}
	openSteamLobbyWaitWindow(nullptr);
	EOS.LobbySearchResults.useLobbyCode = false;
	EOS.LobbySearchResults.showLobbiesInProgress = false;

	EOS.LobbySearchResults.lastResultWasFiltered = true;
	LobbyHandler.showLobbyFilters = false;
#endif
}

void LobbyHandler_t::drawLobbyFilters()
{
#ifdef USE_EOS
	button_t* buttonFilterSearch = nullptr;
	button_t* buttonRefresh = nullptr;
	for ( node_t* node = button_l.first; node != NULL; node = node->next )
	{
		if ( node->element == NULL )
		{
			continue;
		}
		button_t* button = (button_t*)node->element;
		if ( button )
		{
			if ( !buttonFilterSearch && !strcmp(button->label, Language::get(3953)) )
			{
				buttonFilterSearch = button;
			}
			if ( !buttonRefresh && button->action == &buttonSteamLobbyBrowserJoinGame )
			{
				buttonRefresh = button;
			}
		}
	}

	if ( !subwindow || !showLobbyFilters )
	{
		if ( buttonFilterSearch )
		{
			buttonFilterSearch->visible = 0;
			buttonFilterSearch->focused = 0;
		}
		if ( buttonRefresh )
		{
			buttonRefresh->key = SDLK_RETURN;
		}
		return;
	}

	SDL_Rect text;
	text.x = subx2 + 4 + 8;
	text.y = suby2 - (TTF12_HEIGHT + 2) * 8;

	SDL_Rect pos;
	pos.x = subx2 + 4;
	pos.w = TTF12_WIDTH * 24; 
	pos.y = text.y - 8;
	pos.h = suby2 - pos.y;
	drawWindowFancy(pos.x, pos.y, pos.x + pos.w, pos.y + pos.h);

	if ( buttonRefresh )
	{
		buttonRefresh->key = 0;
	}
	if ( buttonFilterSearch )
	{
		buttonFilterSearch->x = pos.x + 8 + 4;
		buttonFilterSearch->y = suby2 - 28;
		buttonFilterSearch->sizex = strlen(Language::get(3953)) * 12 + 8;
		buttonFilterSearch->sizey = 20;
		buttonFilterSearch->visible = 1;
		buttonFilterSearch->focused = 1;
		buttonFilterSearch->key = SDLK_RETURN;
		strcpy(buttonFilterSearch->label, Language::get(3953));
		buttonFilterSearch->action = &LobbyHandler.searchLobbyWithFilter;
	}

	// lobby code search
	ttfPrintTextFormatted(ttf12, text.x, text.y, Language::get(3954));
	text.y += (TTF12_HEIGHT + 2) * 1;
	drawDepressed(text.x, text.y, text.x + (TTF12_WIDTH * 6), text.y + TTF12_HEIGHT + 4);
	ttfPrintTextFormatted(ttf12, text.x + 2, text.y + 4, "%s", EOS.lobbySearchByCode);
	if ( (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
	{
		ttfPrintTextFormatted(ttf12, text.x + 4 + strlen(EOS.lobbySearchByCode) * TTF12_WIDTH, text.y + 4, "_");
	}

	text.y += (TTF12_HEIGHT + 2) * 2;

	// show in-progress lobbies
	ttfPrintTextFormatted(ttf12, text.x, text.y, Language::get(3955), filterShowInProgressLobbies ? 'x' : ' ');
	if ( inputs.bMouseLeft(clientnum) )
	{
		if ( mouseInBounds(clientnum, text.x + strlen("crossplay lobbies: ") * TTF12_WIDTH, text.x + strlen("crossplay lobbies: [x]") * TTF12_WIDTH,
			text.y + TTF12_HEIGHT, text.y + TTF12_HEIGHT * 2) )
		{
			filterShowInProgressLobbies = !filterShowInProgressLobbies;
			inputs.mouseClearLeft(clientnum);
		}
	}
#endif
}
