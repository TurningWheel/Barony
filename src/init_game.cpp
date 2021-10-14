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
#include "Directory.hpp"
#include "mod_tools.hpp"
#include "ui/LoadingScreen.hpp"
#include "ui/GameUI.hpp"
#include "ui/Text.hpp"

#include <thread>
#include <future>
#include <chrono>

/*-------------------------------------------------------------------------------

	initGame

	initializes certain game specific resources

-------------------------------------------------------------------------------*/

int initGame()
{
	// setup some lists
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
 #ifdef USE_EOS
	cpp_SteamServerClientWrapper_OnRequestEncryptedAppTicket = &steam_OnRequestEncryptedAppTicket;
 #endif //USE_EOS
#endif

	initGameControllers();

	// another loading screen!
	createLoadingScreen(90);
	doLoadingScreen();

	// load achievement images
#ifdef NINTENDO
	Directory achievementsDir("rom:/images/achievements");
#else
	Directory achievementsDir("images/achievements");
#endif
	for (auto& item : achievementsDir.list)
	{
		std::string fullPath = achievementsDir.path + std::string("/") + item;
		char* name = const_cast<char*>(fullPath.c_str()); // <- evil
		achievementImages.emplace(std::make_pair(item, loadImage(name)));
	}

	std::atomic_bool loading_done {false};
	auto loading_task = std::async(std::launch::async, [&loading_done](){
		int c, x;
		char name[32];
		File* fp;

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
			for ( line = 1; !fp->eof(); line++ )
			{
				char data[256];
				int limb = 20;
				int dummy;

				// read line from file
				fp->gets( data, 256 );

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
			FileIO::close(fp);
		}

		updateLoadingScreen(92);

		int newItems = 0;

		// load item types
		printlog( "loading items...\n");
		std::string itemsDirectory = PHYSFS_getRealDir("items/items.txt");
		itemsDirectory.append(PHYSFS_getDirSeparator()).append("items/items.txt");
		fp = openDataFile(itemsDirectory.c_str(), "r");
		for ( c = 0; !fp->eof(); ++c )
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
			items[c].index = fp->geti();
			items[c].fpindex = fp->geti();
			items[c].variations = fp->geti();
			fp->gets2(name, 32);
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
			items[c].weight = fp->geti();
			items[c].value = fp->geti();
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
				while ( (string->data[x] = fp->getc()) != '\n' )
				{
					if ( fp->eof() )
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
		FileIO::close(fp);
		//bookParser_t.createBooks(false);
		setupSpells();

#ifdef NINTENDO
		std::string maleNames, femaleNames;
		maleNames = BASE_DATA_DIR + std::string("/") + PLAYERNAMES_MALE_FILE;
		femaleNames = BASE_DATA_DIR + std::string("/") + PLAYERNAMES_FEMALE_FILE;
		randomPlayerNamesMale = getLinesFromDataFile(maleNames);
		randomPlayerNamesFemale = getLinesFromDataFile(femaleNames);
#else // NINTENDO
		randomPlayerNamesMale = getLinesFromDataFile(PLAYERNAMES_MALE_FILE);
		randomPlayerNamesFemale = getLinesFromDataFile(PLAYERNAMES_FEMALE_FILE);
#endif // !NINTENDO

		loadItemLists();

		ItemTooltips.readItemsFromFile();
		ItemTooltips.readTooltipsFromFile();

		loadHUDSettingsJSON();

		updateLoadingScreen(94);

#if defined(USE_EOS) || defined(STEAMWORKS)
#else
#ifdef NINTENDO
		//#error "No DLC support on SWITCH yet :(" //TODO: Resolve this.
		enabledDLCPack1 = true;
		enabledDLCPack2 = true;
#else // NINTENDO
		if ( PHYSFS_getRealDir("mythsandoutcasts.key") != NULL )
		{
			std::string serial = PHYSFS_getRealDir("mythsandoutcasts.key");
			serial.append(PHYSFS_getDirSeparator()).append("mythsandoutcasts.key");
			// open the serial file
			File* fp = nullptr;
			if ( (fp = FileIO::open(serial.c_str(), "rb")) != NULL )
			{
				char buf[64];
				size_t len = fp->read(&buf, sizeof(char), 32);
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
				FileIO::close(fp);
			}
		}
		if ( PHYSFS_getRealDir("legendsandpariahs.key") != NULL ) //TODO: NX PORT: Update for the Switch?
		{
			std::string serial = PHYSFS_getRealDir("legendsandpariahs.key");
			serial.append(PHYSFS_getDirSeparator()).append("legendsandpariahs.key");
			// open the serial file
			File* fp = nullptr;
			if ( (fp = FileIO::open(serial.c_str(), "rb")) != NULL )
			{
				char buf[64];
				size_t len = fp->read(&buf, sizeof(char), 32);
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
				FileIO::close(fp);
			}
		}
#endif // !NINTENDO
#endif

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
			safePacketsReceivedMap[c].clear();
		}
		topscores.first = NULL;
		topscores.last = NULL;
		topscoresMultiplayer.first = NULL;
		topscoresMultiplayer.last = NULL;
		messages.first = NULL;
		messages.last = NULL;
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			chestInv[i].first = NULL;
			chestInv[i].last = NULL;
			for ( c = 0; c < kNumChestItemsToDisplay; c++ )
			{
				invitemschest[i][c] = NULL;
			}
		}
		command_history.first = NULL;
		command_history.last = NULL;
		for ( c = 0; c < MAXPLAYERS; c++ )
		{
			openedChest[c] = NULL;
		}
		mousex = xres / 2;
		mousey = yres / 2;

		// default player stats
		for ( int c = 0; c < MAXPLAYERS; c++ )
		{
			if ( !players[c] )
			{
				players[c] = new Player(c, true);
			}
			players[c]->init();
			// Stat set to 0 as monster type not needed, values will be filled with default, then overwritten by savegame or the charclass.cpp file
			stats[c] = new Stat(0);
			if ( c > 0 )
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
			if ( c == 0 )
			{
				initClass(c);
			}
			GenericGUI[c].setPlayer(c);
			FollowerMenu[c].setPlayer(c);
			cameras[c].winx = 0;
			cameras[c].winy = 0;
			cameras[c].winw = xres;
			cameras[c].winh = yres;
			cast_animation[c].player = c;
		}
		updateLoadingScreen(96);
		
		if ( !loadMusic() )
		{
			printlog("WARN: loadMusic() from initGame() failed!");
		}

		loadAllScores(SCORESFILE);
		loadAllScores(SCORESFILE_MULTIPLAYER);

		updateLoadingScreen(98);
		loading_done = true;
		return 0;
	});
	while (!loading_done) {
		doLoadingScreen();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	destroyLoadingScreen();

	int result = loading_task.get();
	if (result == 0)
	{
		Text::dumpCache(); // createBooks makes some invalid Text() surfaces, this cleans them up for re-rendering.
		gameModeManager.Tutorial.init();

		for ( int c = 0; c < NUMITEMS; c++ )
		{
			items[c].surfaces.first = NULL;
			items[c].surfaces.last = NULL;
			for ( int x = 0; x < list_Size(&items[c].images); x++ )
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

		// load extraneous game resources
		title_bmp = loadImage("images/system/title.png");
		logo_bmp = loadImage("images/system/logo.png");
		cursor_bmp = loadImage("images/system/cursor.png");
		cross_bmp = loadImage("images/system/cross.png");
		selected_cursor_bmp = loadImage("images/system/selectedcursor.png");
		controllerglyphs1_bmp = loadImage("images/system/glyphsheet_ns.png");
		skillIcons_bmp = loadImage("images/system/skillicons_sheet.png");
		if (!loadInterfaceResources())
		{
			printlog("Failed to load interface resources.\n");
			loading_done = true;
			return -1;
		}
	}

	return result;
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
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		players[i]->messageZone.deleteAllNotificationMessages();
	}
	list_FreeAll(&removedEntities);
	if ( title_bmp != nullptr )
	{
		SDL_FreeSurface(title_bmp);
	}
	if ( logo_bmp != nullptr )
	{
		SDL_FreeSurface(logo_bmp);
	}
	if ( cursor_bmp != nullptr )
	{
		SDL_FreeSurface(cursor_bmp);
	}
	if ( cross_bmp != nullptr )
	{
		SDL_FreeSurface(cross_bmp);
	}
	if ( selected_cursor_bmp != nullptr )
	{
		SDL_FreeSurface(selected_cursor_bmp);
	}
	if ( controllerglyphs1_bmp != nullptr )
	{
		SDL_FreeSurface(controllerglyphs1_bmp);
	}
	if ( skillIcons_bmp != nullptr )
	{
		SDL_FreeSurface(skillIcons_bmp);
	}
	//if(sky_bmp!=NULL)
	//	SDL_FreeSurface(sky_bmp);
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		list_FreeAll(&chestInv[i]);
	}
	freeInterfaceResources();
	bookParser_t.deleteBooks();
	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		players[c]->inventoryUI.appraisal.timer = 0;
		players[c]->inventoryUI.appraisal.current_item = 0;
		list_FreeAll(&stats[c]->inventory);
		list_FreeAll(&stats[c]->FOLLOWERS);
		if ( multiplayer == CLIENT )
		{
			if ( shopInv[c] )
			{
				list_FreeAll(shopInv[c]);
				free(shopInv[c]);
				shopInv[c] = NULL;
			}
		}
	}
	list_FreeAll(map.entities);
	if ( map.creatures )
	{
		list_FreeAll(map.creatures); //TODO: Need to do this?
	}
	if ( map.worldUI )
	{
		list_FreeAll(map.worldUI); //TODO: Need to do this?
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
		for ( c = 0; c < MAXPLAYERS; ++c )
		{
			list_FreeAll(&channeledSpells[c]);
		}
	}

	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		list_FreeAll(&players[c]->magic.spellList);
	}
	list_FreeAll(&command_history);

	list_FreeAll(&safePacketsSent);
	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		safePacketsReceivedMap[c].clear();
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
		tutorialmusic->release();

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
#if defined USE_EOS
	if ( EOS.CurrentLobbyData.currentLobbyIsValid() )
	{
		EOS.leaveLobby();

		Uint32 shutdownTicks = SDL_GetTicks();
		while ( EOS.CurrentLobbyData.bAwaitingLeaveCallback )
		{
#ifdef APPLE
			SDL_Event event;
			while ( SDL_PollEvent(&event) != 0 )
			{
				//Makes Mac work because Apple had to do it different.
			}
#endif
			EOS_Platform_Tick(EOS.PlatformHandle);
			SDL_Delay(50);
			if ( SDL_GetTicks() - shutdownTicks >= 3000 )
			{
				break;
			}
		}
	}
	EOS.AccountManager.deinit();
	EOS.shutdown();
#endif

	//Close game controller
	/*if (game_controller)
	{
		SDL_GameControllerClose(game_controller);
		game_controller = nullptr;
	}*/
	/*if (game_controller)
	{
		delete game_controller;
	}*/

#ifndef NINTENDO
	IRCHandler.disconnect();
#endif // !NINTENDO

	if ( shoparea )
	{
		free(shoparea);
	}

	for (int i = 0; i < MAXPLAYERS; ++i)
	{
		delete players[i];
		players[i] = nullptr;
	}
}
