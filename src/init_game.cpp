/*-------------------------------------------------------------------------------

	BARONY
	File: init_game.cpp
	Desc: contains game specific initialization code that shouldn't be
	seen in the editor.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "draw.hpp"
#include "files.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "interface/interface.hpp"
#include "messages.hpp"
#include "book.hpp"
#include "engine/audio/sound.hpp"
#include "shops.hpp"
#include "scores.hpp"
#include "magic/magic.hpp"
#include "monster.hpp"
#include "net.hpp"
#ifdef STEAMWORKS
#include <steam/steam_api.h>
#include "steam.hpp"
#endif
#include "menu.hpp"
#include "paths.hpp"
#include "player.hpp"
#include "cppfuncs.hpp"

/*-------------------------------------------------------------------------------

	initGame

	initializes certain game specific resources

-------------------------------------------------------------------------------*/

#define _LOADSTR1 language[746]
#define _LOADSTR2 language[747]
#define _LOADSTR3 language[748]
#define _LOADSTR4 language[749]

int initGame()
{
	int c, x;
	char name[32];
	FILE* fp;

	// setup some lists
	steamAchievements.first = NULL;
	steamAchievements.last = NULL;
	booksRead.first = NULL;
	booksRead.last = NULL;
	lobbyChatboxMessages.first = NULL;
	lobbyChatboxMessages.last = NULL;

	// steam stuff
#ifdef STEAMWORKS
	cpp_SteamServerWrapper_Instantiate(); //TODO: Remove these wrappers.
	cpp_SteamServerClientWrapper_Instantiate();

	cpp_SteamServerClientWrapper_OnP2PSessionRequest = &steam_OnP2PSessionRequest;
	//cpp_SteamServerClientWrapper_OnGameOverlayActivated = &steam_OnGameOverlayActivated;
	cpp_SteamServerClientWrapper_OnLobbyCreated = &steam_OnLobbyCreated;
	cpp_SteamServerClientWrapper_OnGameJoinRequested = &steam_OnGameJoinRequested;
	cpp_SteamServerClientWrapper_OnLobbyEntered = &steam_OnLobbyEntered;
	cpp_SteamServerClientWrapper_GameServerPingOnServerResponded = &steam_GameServerPingOnServerResponded;
	cpp_SteamServerClientWrapper_OnLobbyMatchListCallback = &steam_OnLobbyMatchListCallback;
	cpp_SteamServerClientWrapper_OnP2PSessionConnectFail = &steam_OnP2PSessionConnectFail;
	cpp_SteamServerClientWrapper_OnLobbyDataUpdate = &steam_OnLobbyDataUpdatedCallback;
#endif

	// print a loading message
	drawClearBuffers();
	int w, h;
	TTF_SizeUTF8(ttf16, _LOADSTR1, &w, &h);
	ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, _LOADSTR1);

	GO_SwapBuffers(screen);

	initGameControllers();

	// load model offsets
	printlog( "loading model offsets...\n");
	for ( c = 1; c < NUMMONSTERS; c++ )
	{
		// initialize all offsets to zero
		for ( x = 0; x < 20; x++ )
		{
			limbs[c][x][0] = 0;
			limbs[c][x][1] = 0;
			limbs[c][x][2] = 0;
		}

		// open file
		char filename[256];
		strcpy(filename, "models/creatures/");
		strcat(filename, monstertypename[c]);
		strcat(filename, "/limbs.txt");
		if ( (fp = openDataFile(filename, "r")) == NULL )
		{
			continue;
		}

		// read file
		int line;
		for ( line = 1; feof(fp) == 0; line++ )
		{
			char data[256];
			int limb = 20;
			int dummy;

			// read line from file
			fgets( data, 256, fp );

			// skip blank and comment lines
			if ( data[0] == '\n' || data[0] == '\r' || data[0] == '#' )
			{
				continue;
			}

			// process line
			if ( sscanf( data, "%d", &limb ) != 1 || limb >= 20 || limb < 0 )
			{
				printlog( "warning: syntax error in '%s':%d\n invalid limb index!\n", filename, line);
				continue;
			}
			if ( sscanf( data, "%d %f %f %f\n", &dummy, &limbs[c][limb][0], &limbs[c][limb][1], &limbs[c][limb][2] ) != 4 )
			{
				printlog( "warning: syntax error in '%s':%d\n invalid limb offsets!\n", filename, line);
				continue;
			}
		}

		// close file
		fclose(fp);
	}

	// print a loading message
	drawClearBuffers();
	TTF_SizeUTF8(ttf16, _LOADSTR2, &w, &h);
	ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, _LOADSTR2);

	GO_SwapBuffers(screen);

	int newItems = 0;

	// load item types
	printlog( "loading items...\n");
	std::string itemsDirectory = PHYSFS_getRealDir("items/items.txt");
	itemsDirectory.append(PHYSFS_getDirSeparator()).append("items/items.txt");
	fp = openDataFile(itemsDirectory.c_str(), "r");
	for ( c = 0; !feof(fp); ++c )
	{
		if ( c > SPELLBOOK_DETECT_FOOD )
		{
			newItems = c - SPELLBOOK_DETECT_FOOD - 1;
			items[c].name_identified = language[3500 + newItems * 2];
			items[c].name_unidentified = language[3501 + newItems * 2];
		}
		else if ( c > ARTIFACT_BOW )
		{
			newItems = c - ARTIFACT_BOW - 1;
			items[c].name_identified = language[2200 + newItems * 2];
			items[c].name_unidentified = language[2201 + newItems * 2];
		}
		else
		{
			items[c].name_identified = language[1545 + c * 2];
			items[c].name_unidentified = language[1546 + c * 2];
		}
		fscanf(fp, "%d", &items[c].index);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
		fscanf(fp, "%d", &items[c].fpindex);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
		fscanf(fp, "%d", &items[c].variations);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
		fscanf(fp, "%s", name);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
		if ( !strcmp(name, "WEAPON") )
		{
			items[c].category = WEAPON;
		}
		else if ( !strcmp(name, "ARMOR") )
		{
			items[c].category = ARMOR;
		}
		else if ( !strcmp(name, "AMULET") )
		{
			items[c].category = AMULET;
		}
		else if ( !strcmp(name, "POTION") )
		{
			items[c].category = POTION;
		}
		else if ( !strcmp(name, "SCROLL") )
		{
			items[c].category = SCROLL;
		}
		else if ( !strcmp(name, "MAGICSTAFF") )
		{
			items[c].category = MAGICSTAFF;
		}
		else if ( !strcmp(name, "RING") )
		{
			items[c].category = RING;
		}
		else if ( !strcmp(name, "SPELLBOOK") )
		{
			items[c].category = SPELLBOOK;
		}
		else if ( !strcmp(name, "TOOL") )
		{
			items[c].category = TOOL;
		}
		else if ( !strcmp(name, "FOOD") )
		{
			items[c].category = FOOD;
		}
		else if ( !strcmp(name, "BOOK") )
		{
			items[c].category = BOOK;
		}
		else if ( !strcmp(name, "THROWN") )
		{
			items[c].category = THROWN;
		}
		else if ( !strcmp(name, "SPELL_CAT") )
		{
			items[c].category = SPELL_CAT;
		}
		else
		{
			items[c].category = GEM;
		}
		fscanf(fp, "%d", &items[c].weight);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
		fscanf(fp, "%d", &items[c].value);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
		items[c].images.first = NULL;
		items[c].images.last = NULL;
		while ( 1 )
		{
			string_t* string = (string_t*) malloc(sizeof(string_t));
			string->data = (char*) malloc(sizeof(char) * 64);
			string->lines = 1;

			node_t* node = list_AddNodeLast(&items[c].images);
			node->element = string;
			node->deconstructor = &stringDeconstructor;
			node->size = sizeof(string_t);
			string->node = node;

			x = 0;
			bool fileend = false;
			while ( (string->data[x] = fgetc(fp)) != '\n' )
			{
				if ( feof(fp) )
				{
					fileend = true;
					break;
				}
				x++;
			}
			if ( x == 0 || fileend )
			{
				list_RemoveNode(node);
				break;
			}
			string->data[x] = 0;
		}
	}
	for ( c = 0; c < NUMITEMS; c++ )
	{
		items[c].surfaces.first = NULL;
		items[c].surfaces.last = NULL;
		for ( x = 0; x < list_Size(&items[c].images); x++ )
		{
			SDL_Surface** surface = (SDL_Surface**) malloc(sizeof(SDL_Surface*));
			node_t* node = list_AddNodeLast(&items[c].surfaces);
			node->element = surface;
			node->deconstructor = &defaultDeconstructor;
			node->size = sizeof(SDL_Surface*);

			node_t* node2 = list_Node(&items[c].images, x);
			string_t* string = (string_t*)node2->element;
			std::string itemImgDir;
			if ( PHYSFS_getRealDir(string->data) != NULL )
			{
				itemImgDir = PHYSFS_getRealDir(string->data);
				itemImgDir.append(PHYSFS_getDirSeparator()).append(string->data);
			}
			else
			{
				itemImgDir = string->data;
			}
			char imgFileChar[256];
			strncpy(imgFileChar, itemImgDir.c_str(), 255);
			*surface = loadImage(imgFileChar);
		}
	}
	fclose(fp);
	createBooks();
	setupSpells();

	randomPlayerNamesMale = getLinesFromDataFile(PLAYERNAMES_MALE_FILE);
	randomPlayerNamesFemale = getLinesFromDataFile(PLAYERNAMES_FEMALE_FILE);
	loadItemLists();

#ifndef STEAMWORKS
	if ( PHYSFS_getRealDir("mythsandoutcasts.key") != NULL )
	{
		std::string serial = PHYSFS_getRealDir("mythsandoutcasts.key");
		serial.append(PHYSFS_getDirSeparator()).append("mythsandoutcasts.key");
		// open the serial file
		FILE* fp = nullptr;
		if ( (fp = fopen(serial.c_str(), "rb")) != NULL )
		{
			char buf[64];
			size_t len = fread(&buf, sizeof(char), 32, fp);
			buf[len] = '\0';
			serial = buf;
			// compute hash
			size_t DLCHash = serialHash(serial);
			if ( DLCHash == 144425 )
			{
				printlog("[LICENSE]: Myths and Outcasts DLC license key found.");
				enabledDLCPack1 = true;
			}
			else
			{
				printlog("[LICENSE]: DLC license key invalid.");
			}
			fclose(fp);
		}
	}
	if ( PHYSFS_getRealDir("legendsandpariahs.key") != NULL )
	{
		std::string serial = PHYSFS_getRealDir("legendsandpariahs.key");
		serial.append(PHYSFS_getDirSeparator()).append("legendsandpariahs.key");
		// open the serial file
		FILE* fp = nullptr;
		if ( (fp = fopen(serial.c_str(), "rb")) != NULL )
		{
			char buf[64];
			size_t len = fread(&buf, sizeof(char), 32, fp);
			buf[len] = '\0';
			serial = buf;
			// compute hash
			size_t DLCHash = serialHash(serial);
			if ( DLCHash == 135398 )
			{
				printlog("[LICENSE]: Legends and Pariahs DLC license key found.");
				enabledDLCPack2 = true;
			}
			else
			{
				printlog("[LICENSE]: DLC license key invalid.");
			}
			fclose(fp);
		}
	}
#endif // !STEAMWORKS

	// print a loading message
	drawClearBuffers();
	TTF_SizeUTF8(ttf16, _LOADSTR3, &w, &h);
	ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, _LOADSTR3);

	GO_SwapBuffers(screen);

#ifdef USE_FMOD
	music_group->setVolume(musvolume / 128.f);
#elif defined USE_OPENAL
	OPENAL_ChannelGroup_SetVolume(music_group, musvolume / 128.f);
#endif
	removedEntities.first = NULL;
	removedEntities.last = NULL;
	safePacketsSent.first = NULL;
	safePacketsSent.last = NULL;
	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		safePacketsReceived[c].first = NULL;
		safePacketsReceived[c].last = NULL;
	}
	topscores.first = NULL;
	topscores.last = NULL;
	topscoresMultiplayer.first = NULL;
	topscoresMultiplayer.last = NULL;
	messages.first = NULL;
	messages.last = NULL;
	chestInv.first = NULL;
	chestInv.last = NULL;
	command_history.first = NULL;
	command_history.last = NULL;
	for ( c = 0; c < 4; c++ )
	{
		invitemschest[c] = NULL;
		openedChest[c] = NULL;
	}
	mousex = xres / 2;
	mousey = yres / 2;

	players = new Player*[MAXPLAYERS];
	// default player stats
	for (c = 0; c < MAXPLAYERS; c++)
	{
		players[c] = new Player();
		// Stat set to 0 as monster type not needed, values will be filled with default, then overwritten by savegame or the charclass.cpp file
		stats[c] = new Stat(0);
		if (c > 0)
		{
			client_disconnected[c] = true;
		}
		players[c]->entity = nullptr;
		stats[c]->sex = static_cast<sex_t>(0);
		stats[c]->appearance = 0;
		strcpy(stats[c]->name, "");
		stats[c]->type = HUMAN;
		stats[c]->playerRace = RACE_HUMAN;
		stats[c]->FOLLOWERS.first = nullptr;
		stats[c]->FOLLOWERS.last = nullptr;
		stats[c]->inventory.first = nullptr;
		stats[c]->inventory.last = nullptr;
		stats[c]->clearStats();
		entitiesToDelete[c].first = nullptr;
		entitiesToDelete[c].last = nullptr;
		if (c == 0)
		{
			initClass(c);
		}
	}

	if ( !loadMusic() )
	{
		printlog("WARN: loadMusic() from initGame() failed!");
	}

	// print a loading message
	drawClearBuffers();
	TTF_SizeUTF8(ttf16, _LOADSTR4, &w, &h);
	ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, _LOADSTR4);

	GO_SwapBuffers(screen);

	// load extraneous game resources
	title_bmp = loadImage("images/system/title.png");
	logo_bmp = loadImage("images/system/logo.png");
	cursor_bmp = loadImage("images/system/cursor.png");
	cross_bmp = loadImage("images/system/cross.png");

	loadAllScores(SCORESFILE);
	loadAllScores(SCORESFILE_MULTIPLAYER);
	if (!loadInterfaceResources())
	{
		printlog("Failed to load interface resources.\n");
		return -1;
	}

	return 0;
}

/*-------------------------------------------------------------------------------

	deinitGame

	deinitializes certain game specific resources

-------------------------------------------------------------------------------*/

void deinitGame()
{
	int c, x;

	// send disconnect messages
	if ( multiplayer == CLIENT )
	{
		strcpy((char*)net_packet->data, "DISCONNECT");
		net_packet->data[10] = clientnum;
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 11;
		sendPacketSafe(net_sock, -1, net_packet, 0);
		printlog("disconnected from server.\n");
	}
	else if ( multiplayer == SERVER )
	{
		for ( x = 1; x < MAXPLAYERS; x++ )
		{
			if ( client_disconnected[x] == true )
			{
				continue;
			}
			strcpy((char*)net_packet->data, "DISCONNECT");
			net_packet->data[10] = clientnum;
			net_packet->address.host = net_clients[x - 1].host;
			net_packet->address.port = net_clients[x - 1].port;
			net_packet->len = 11;
			sendPacketSafe(net_sock, -1, net_packet, x - 1);

			stats[x]->freePlayerEquipment();
			client_disconnected[x] = true;
		}
	}

	// this short delay makes sure that the disconnect message gets out
	Uint32 timetoshutdown = SDL_GetTicks();
	while ( SDL_GetTicks() - timetoshutdown < 500 )
	{
		// handle network messages
		if ( multiplayer == CLIENT )
		{
			clientHandleMessages(fpsLimit);
		}
		else if ( multiplayer == SERVER )
		{
			serverHandleMessages(fpsLimit);
		}
		if ( !(SDL_GetTicks() % 25) && multiplayer )
		{
			int j = 0;
			node_t* node, *nextnode;
			for ( node = safePacketsSent.first; node != NULL; node = nextnode )
			{
				nextnode = node->next;

				packetsend_t* packet = (packetsend_t*)node->element;
				sendPacket(packet->sock, packet->channel, packet->packet, packet->hostnum);
				packet->tries++;
				if ( packet->tries >= MAXTRIES )
				{
					list_RemoveNode(node);
				}
				j++;
				if ( j >= MAXDELETES )
				{
					break;
				}
			}
		}
	}

	saveAllScores(SCORESFILE);
	saveAllScores(SCORESFILE_MULTIPLAYER);
	list_FreeAll(&topscores);
	list_FreeAll(&topscoresMultiplayer);
	deleteAllNotificationMessages();
	list_FreeAll(&removedEntities);
	if ( title_bmp != NULL )
	{
		SDL_FreeSurface(title_bmp);
	}
	if ( logo_bmp != NULL )
	{
		SDL_FreeSurface(logo_bmp);
	}
	if ( cursor_bmp != NULL )
	{
		SDL_FreeSurface(cursor_bmp);
	}
	if ( cross_bmp != NULL )
	{
		SDL_FreeSurface(cross_bmp);
	}
	//if(sky_bmp!=NULL)
	//	SDL_FreeSurface(sky_bmp);
	list_FreeAll(&chestInv);
	freeInterfaceResources();
	if ( books )
	{
		for ( c = 0; c < numbooks; c++ )
		{
			if ( books[c] )
			{
				if ( books[c]->name )
				{
					free(books[c]->name);
				}
				if ( books[c]->text )
				{
					free(books[c]->text);
				}
				if ( books[c]->bookgui_render_title )
				{
					free(books[c]->bookgui_render_title);
				}
				list_FreeAll(&books[c]->pages);
				free(books[c]);
			}
		}
		free(books);
	}
	appraisal_timer = 0;
	appraisal_item = 0;
	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		list_FreeAll(&stats[c]->inventory);
	}
	if ( multiplayer == CLIENT )
	{
		if ( shopInv )
		{
			list_FreeAll(shopInv);
			free(shopInv);
			shopInv = NULL;
		}
	}
	list_FreeAll(map.entities);
	if ( map.creatures )
	{
		list_FreeAll(map.creatures); //TODO: Need to do this?
	}
	list_FreeAll(&messages);
	if ( multiplayer == SINGLE )
	{
		list_FreeAll(&channeledSpells[0]);
	}
	else if ( multiplayer == CLIENT )
	{
		list_FreeAll(&channeledSpells[clientnum]);
	}
	else if ( multiplayer == SERVER )
	{
		for ( c = 0; c < numplayers; ++c )
		{
			list_FreeAll(&channeledSpells[c]);
		}
	}
	list_FreeAll(&spellList);
	list_FreeAll(&command_history);

	list_FreeAll(&safePacketsSent);
	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		list_FreeAll(&safePacketsReceived[c]);
	}
#ifdef SOUND
#ifdef USE_OPENAL //TODO: OpenAL is now all of the broken...
#define FMOD_Channel_Stop OPENAL_Channel_Stop
#define FMOD_Sound_Release OPENAL_Sound_Release
#endif
	if ( !no_sound )
	{
		music_channel->stop();
		music_channel2->stop();
		introductionmusic->release();
		intermissionmusic->release();
		minetownmusic->release();
		splashmusic->release();
		librarymusic->release();
		shopmusic->release();
		herxmusic->release();
		templemusic->release();
		endgamemusic->release();
		escapemusic->release();
		devilmusic->release();
		sanctummusic->release();
		gnomishminesmusic->release();
		greatcastlemusic->release();
		sokobanmusic->release();
		caveslairmusic->release();
		bramscastlemusic->release();
		hamletmusic->release();
		for ( c = 0; c < NUMMINESMUSIC; c++ )
		{
			minesmusic[c]->release();
		}
		if ( minesmusic )
		{
			free(minesmusic);
		}
		for ( c = 0; c < NUMSWAMPMUSIC; c++ )
		{
			swampmusic[c]->release();
		}
		if ( swampmusic )
		{
			free(swampmusic);
		}
		for ( c = 0; c < NUMLABYRINTHMUSIC; c++ )
		{
			labyrinthmusic[c]->release();
		}
		if ( labyrinthmusic )
		{
			free(labyrinthmusic);
		}
		for ( c = 0; c < NUMRUINSMUSIC; c++ )
		{
			ruinsmusic[c]->release();
		}
		if ( ruinsmusic )
		{
			free(ruinsmusic);
		}
		for ( c = 0; c < NUMUNDERWORLDMUSIC; c++ )
		{
			underworldmusic[c]->release();
		}
		if ( underworldmusic )
		{
			free(underworldmusic);
		}
		for ( c = 0; c < NUMHELLMUSIC; c++ )
		{
			hellmusic[c]->release();
		}
		if ( hellmusic )
		{
			free(hellmusic);
		}
		for ( c = 0; c < NUMMINOTAURMUSIC; c++ )
		{
			minotaurmusic[c]->release();
		}
		if ( minotaurmusic )
		{
			free(minotaurmusic);
		}
		for ( c = 0; c < NUMCAVESMUSIC; c++ )
		{
			cavesmusic[c]->release();
		}
		if ( cavesmusic )
		{
			free(cavesmusic);
		}
		for ( c = 0; c < NUMCITADELMUSIC; c++ )
		{
			citadelmusic[c]->release();
		}
		if ( citadelmusic )
		{
			free(citadelmusic);
		}
		for ( c = 0; c < NUMINTROMUSIC; c++ )
		{
			intromusic[c]->release();
		}
		if ( intromusic )
		{
			free(intromusic);
		}
	}
#ifdef USE_OPENAL
#undef FMOD_Channel_Stop
#undef FMOD_Sound_Release
#endif
#endif

	// free items
	printlog( "freeing item data...\n");
	for ( c = 0; c < NUMITEMS; c++ )
	{
		list_FreeAll(&items[c].images);
		node_t* node, *nextnode;
		for ( node = items[c].surfaces.first; node != NULL; node = nextnode )
		{
			nextnode = node->next;
			SDL_Surface** surface = (SDL_Surface**)node->element;
			if ( surface )
				if ( *surface )
				{
					SDL_FreeSurface(*surface);
				}
		}
		list_FreeAll(&items[c].surfaces);
	}

	freeSpells();

	// pathmaps
	if ( pathMapGrounded )
	{
		free(pathMapGrounded);
	}
	pathMapGrounded = NULL;
	if ( pathMapFlying )
	{
		free(pathMapFlying);
	}
	pathMapFlying = NULL;

	// clear steam achievement list
	list_FreeAll(&steamAchievements);
	list_FreeAll(&booksRead);

	// clear lobby chatbox data
	list_FreeAll(&lobbyChatboxMessages);

	// steam stuff
#ifdef STEAMWORKS
	cpp_SteamServerWrapper_Destroy();
	cpp_SteamServerClientWrapper_Destroy();
	if ( currentLobby )
	{
		SteamMatchmaking()->LeaveLobby(*static_cast<CSteamID*>(currentLobby));
		cpp_Free_CSteamID(currentLobby); //TODO: Remove these bodges.
		currentLobby = NULL;
	}
	if ( lobbyToConnectTo )
	{
		cpp_Free_CSteamID(lobbyToConnectTo);
		lobbyToConnectTo = NULL;
	}
	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		if ( steamIDRemote[c] )
		{
			cpp_Free_CSteamID(steamIDRemote[c]);
			steamIDRemote[c] = NULL;
		}
	}
	for ( c = 0; c < MAX_STEAM_LOBBIES; c++ )
	{
		if ( lobbyIDs[c] )
		{
			cpp_Free_CSteamID(lobbyIDs[c]);
			lobbyIDs[c] = NULL;
		}
	}
#endif

	//Close game controller
	/*if (game_controller)
	{
		SDL_GameControllerClose(game_controller);
		game_controller = nullptr;
	}*/
	if (game_controller)
	{
		delete game_controller;
	}

	if ( shoparea )
	{
		free(shoparea);
	}

	for (int i = 0; i < MAXPLAYERS; ++i)
	{
		delete players[i];
	}
	delete[] players;
}
