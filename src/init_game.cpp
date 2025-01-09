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
#ifdef USE_PLAYFAB
#include "playfab.hpp"
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
#include "ui/MainMenu.hpp"

#include <thread>
#include <future>
#include <chrono>

/*-------------------------------------------------------------------------------

	initGame

	initializes certain game specific resources

-------------------------------------------------------------------------------*/

void initGameDatafiles(bool moddedReload)
{
	for ( int i = 0; i < NUMITEMS && i < (NUM_ITEM_STRINGS - 2); ++i )
	{
		ItemTooltips.itemNameStringToItemID[itemNameStrings[i + 2]] = i;
	}
	ItemTooltips.readItemsFromFile();
	ItemTooltips.readTooltipsFromFile();
	ItemTooltips.readItemLocalizationsFromFile();
	ItemTooltips.readBookLocalizationsFromFile();
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		// set these to something silly clear the tooltip cache match
		players[i]->worldUI.worldTooltipItem.type = WOODEN_SHIELD;
		players[i]->worldUI.worldTooltipItem.count = 99;
	}

	loadHUDSettingsJSON();
	Player::SkillSheet_t::loadSkillSheetJSON();
	Player::CharacterSheet_t::loadCharacterSheetJSON();
	StatusEffectQueue_t::loadStatusEffectsJSON();
	FollowerRadialMenu::loadFollowerJSON();
	CalloutRadialMenu::loadCalloutJSON();
	MonsterData_t::loadMonsterDataJSON();
	ScriptTextParser.readAllScripts();
	ShopkeeperConsumables_t::readFromFile();
	EditorEntityData_t::readFromFile();
	ClassHotbarConfig_t::init();
	MainMenu::RaceDescriptions::readFromFile();
	MainMenu::ClassDescriptions::readFromFile();
	StatueManager.readAllStatues();
	GameModeManager_t::CurrentSession_t::SeededRun_t::readSeedNamesFromFile();
	loadLights();
	for ( int c = 1; c < NUMMONSTERS; ++c )
	{
		EquipmentModelOffsets.readFromFile(monstertypename[c], c);
	}
	setupSpells();
	CompendiumEntries.readMonstersFromFile();
	Compendium_t::Events_t::itemDisplayedEventsList.clear();
	Compendium_t::Events_t::readEventsFromFile();
	CompendiumEntries.readCodexFromFile();
	CompendiumEntries.readWorldFromFile();
	CompendiumEntries.readItemsFromFile();
	CompendiumEntries.readMagicFromFile();
	CompendiumEntries.readMonstersTranslationsFromFile();
	CompendiumEntries.readCodexTranslationsFromFile();
	CompendiumEntries.readWorldTranslationsFromFile();
	CompendiumEntries.readItemsTranslationsFromFile();
	CompendiumEntries.readMagicTranslationsFromFile();
	Compendium_t::AchievementData_t::readContentsLang();
	Compendium_t::Events_t::readEventsTranslations();
	if ( !moddedReload )
	{
		Compendium_t::readUnlocksSaveData();
		Compendium_t::Events_t::loadItemsSaveData();
	}
	CompendiumEntries.readModelLimbsFromFile("monster");
	CompendiumEntries.readModelLimbsFromFile("world");
	CompendiumEntries.readModelLimbsFromFile("codex");
	MainMenu::MainMenuBanners_t::readFromFile();
}

void initGameDatafilesAsync(bool moddedReload)
{
	physfsReloadMonsterLimbFiles();
	GlyphHelper.readFromFile();
#ifndef NINTENDO
	if ( PHYSFS_getRealDir(PLAYERNAMES_MALE_FILE.c_str()) )
	{
		std::string namesDirectory = PHYSFS_getRealDir(PLAYERNAMES_MALE_FILE.c_str());
		namesDirectory.append(PHYSFS_getDirSeparator()).append(PLAYERNAMES_MALE_FILE);
		randomPlayerNamesMale = getLinesFromDataFile(namesDirectory);
	}
	if ( PHYSFS_getRealDir(PLAYERNAMES_FEMALE_FILE.c_str()) )
	{
		std::string namesDirectory = PHYSFS_getRealDir(PLAYERNAMES_FEMALE_FILE.c_str());
		namesDirectory.append(PHYSFS_getDirSeparator()).append(PLAYERNAMES_FEMALE_FILE);
		randomPlayerNamesFemale = getLinesFromDataFile(namesDirectory);
	}
	if ( PHYSFS_getRealDir(NPCNAMES_MALE_FILE.c_str()) )
	{
		std::string namesDirectory = PHYSFS_getRealDir(NPCNAMES_MALE_FILE.c_str());
		namesDirectory.append(PHYSFS_getDirSeparator()).append(NPCNAMES_MALE_FILE);
		randomNPCNamesMale = getLinesFromDataFile(namesDirectory);
	}
	if ( PHYSFS_getRealDir(NPCNAMES_FEMALE_FILE.c_str()) )
	{
		std::string namesDirectory = PHYSFS_getRealDir(NPCNAMES_FEMALE_FILE.c_str());
		namesDirectory.append(PHYSFS_getDirSeparator()).append(NPCNAMES_FEMALE_FILE);
		randomNPCNamesFemale = getLinesFromDataFile(namesDirectory);
	}
#endif
}

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
#ifdef USE_PLAYFAB
	playfabUser.init();
#endif

	initGameControllers();

	// another loading screen!
	updateLoadingScreen(90);
	doLoadingScreen();

	// load item types
	initGameDatafiles(false);

	std::atomic_bool loading_done {false};
	auto loading_task = std::async(std::launch::async, [&loading_done](){
		updateLoadingScreen(92);
		initGameDatafilesAsync(false);
#ifdef NINTENDO
		const auto playerMaleNames = BASE_DATA_DIR + std::string("/") + PLAYERNAMES_MALE_FILE;
		const auto playerFemaleNames = BASE_DATA_DIR + std::string("/") + PLAYERNAMES_FEMALE_FILE;
        const auto npcMaleNames = BASE_DATA_DIR + std::string("/") + NPCNAMES_MALE_FILE;
        const auto npcFemaleNames = BASE_DATA_DIR + std::string("/") + NPCNAMES_FEMALE_FILE;
		randomPlayerNamesMale = getLinesFromDataFile(playerMaleNames);
		randomPlayerNamesFemale = getLinesFromDataFile(playerFemaleNames);
        randomNPCNamesMale = getLinesFromDataFile(npcMaleNames);
        randomNPCNamesFemale = getLinesFromDataFile(npcFemaleNames);
#endif // NINTENDO

		updateLoadingScreen(94);

//#ifdef NINTENDO_DEBUG
		//enabledDLCPack1 = true;
		//enabledDLCPack2 = true;
//#endif

#if defined(USE_EOS) || defined(STEAMWORKS)
#else
#ifndef NINTENDO
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

		removedEntities.first = NULL;
		removedEntities.last = NULL;
		safePacketsSent.first = NULL;
		safePacketsSent.last = NULL;
		for ( int c = 0; c < MAXPLAYERS; c++ )
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
		}
		command_history.first = NULL;
		command_history.last = NULL;
		for ( int c = 0; c < MAXPLAYERS; c++ )
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
			stats[c]->stat_appearance = 0;
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
			initClass(c);
			GenericGUI[c].setPlayer(c);
			FollowerMenu[c].setPlayer(c);
			CalloutMenu[c].setPlayer(c);
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
	updateLoadingScreen(100);
	doLoadingScreen();
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	doLoadingScreen();
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	destroyLoadingScreen();

	int result = loading_task.get();
	if (result == 0)
	{
		bookParser_t.createBooks(false);
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
		if (!loadInterfaceResources())
		{
			printlog("Failed to load interface resources.\n");
			loading_done = true;
			return -1;
		}

		loadAchievementData("/data/achievements.json");
#ifdef LOCAL_ACHIEVEMENTS
		LocalAchievements.readFromFile();
#endif
#ifdef NINTENDO
		nxPostSDLInit();
#endif
	}

	return result;
}

/*-------------------------------------------------------------------------------

	deinitGame

	deinitializes certain game specific resources

-------------------------------------------------------------------------------*/

#include "interface/ui.hpp"

void deinitGame()
{
    // destroy camera framebuffers
    constexpr int numFbs = sizeof(view_t::fb) / sizeof(view_t::fb[0]);
    for (int c = 0; c < MAXPLAYERS; ++c) {
        for (int i = 0; i < numFbs; ++i) {
            cameras[c].fb[i].destroy();
            playerPortraitView[c].fb[i].destroy();
        }
    }
    for (int i = 0; i < numFbs; ++i) {
        menucam.fb[i].destroy();
    }

	// destroy enemy hp bar textures
	EnemyHPDamageBarHandler::dumpCache();

	// send disconnect messages
	if (multiplayer != SINGLE) {
	    if ( multiplayer == CLIENT )
	    {
		    strcpy((char*)net_packet->data, "DISC");
		    net_packet->data[4] = clientnum;
		    net_packet->address.host = net_server.host;
		    net_packet->address.port = net_server.port;
		    net_packet->len = 5;
		    sendPacketSafe(net_sock, -1, net_packet, 0);
		    printlog("disconnected from server.\n");
	    }
	    else if ( multiplayer == SERVER )
	    {
		    for ( int x = 1; x < MAXPLAYERS; x++ )
		    {
			    if ( client_disconnected[x] == true )
			    {
				    continue;
			    }
			    strcpy((char*)net_packet->data, "DISC");
			    net_packet->data[4] = clientnum;
			    net_packet->address.host = net_clients[x - 1].host;
			    net_packet->address.port = net_clients[x - 1].port;
			    net_packet->len = 5;
			    sendPacketSafe(net_sock, -1, net_packet, x - 1);

			    stats[x]->freePlayerEquipment();
			    client_disconnected[x] = true;
		    }
	    }

	    // this short delay makes sure that the disconnect message gets out
	    Uint32 timetoshutdown = SDL_GetTicks();
	    while ( SDL_GetTicks() - timetoshutdown < 200 )
	    {
	        /*if ( multiplayer == CLIENT ) {
		        clientHandleMessages(fpsLimit);
	        } else if ( multiplayer == SERVER ) {
		        serverHandleMessages(fpsLimit);
	        }*/
	        pollNetworkForShutdown();
	    }
	}

	UIToastNotificationManager.term(true);
	Compendium_t::Events_t::writeItemsSaveData();
	Compendium_t::writeUnlocksSaveData();
#ifdef LOCAL_ACHIEVEMENTS
	LocalAchievements_t::writeToFile();
#endif

	saveAllScores(SCORESFILE);
	saveAllScores(SCORESFILE_MULTIPLAYER);
	list_FreeAll(&topscores);
	list_FreeAll(&topscoresMultiplayer);
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		players[i]->messageZone.deleteAllNotificationMessages();
		players[i]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_CLOSE_ALL);
	}
	list_FreeAll(&removedEntities);
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		list_FreeAll(&chestInv[i]);
	}
	freeInterfaceResources();
	bookParser_t.deleteBooks();
	for ( int c = 0; c < MAXPLAYERS; c++ )
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
		for ( int c = 0; c < MAXPLAYERS; ++c )
		{
			list_FreeAll(&channeledSpells[c]);
		}
	}

	for ( int c = 0; c < MAXPLAYERS; c++ )
	{
		list_FreeAll(&players[c]->magic.spellList);
	}
	list_FreeAll(&command_history);

	list_FreeAll(&safePacketsSent);
	for ( int c = 0; c < MAXPLAYERS; c++ )
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
		gameovermusic->release();
		introstorymusic->release();

		for ( int c = 0; c < NUMMINESMUSIC; c++ )
		{
			minesmusic[c]->release();
		}
		if ( minesmusic )
		{
			free(minesmusic);
		}
		for ( int c = 0; c < NUMSWAMPMUSIC; c++ )
		{
			swampmusic[c]->release();
		}
		if ( swampmusic )
		{
			free(swampmusic);
		}
		for ( int c = 0; c < NUMLABYRINTHMUSIC; c++ )
		{
			labyrinthmusic[c]->release();
		}
		if ( labyrinthmusic )
		{
			free(labyrinthmusic);
		}
		for ( int c = 0; c < NUMRUINSMUSIC; c++ )
		{
			ruinsmusic[c]->release();
		}
		if ( ruinsmusic )
		{
			free(ruinsmusic);
		}
		for ( int c = 0; c < NUMUNDERWORLDMUSIC; c++ )
		{
			underworldmusic[c]->release();
		}
		if ( underworldmusic )
		{
			free(underworldmusic);
		}
		for ( int c = 0; c < NUMHELLMUSIC; c++ )
		{
			hellmusic[c]->release();
		}
		if ( hellmusic )
		{
			free(hellmusic);
		}
		for ( int c = 0; c < NUMMINOTAURMUSIC; c++ )
		{
			minotaurmusic[c]->release();
		}
		if ( minotaurmusic )
		{
			free(minotaurmusic);
		}
		for ( int c = 0; c < NUMCAVESMUSIC; c++ )
		{
			cavesmusic[c]->release();
		}
		if ( cavesmusic )
		{
			free(cavesmusic);
		}
		for ( int c = 0; c < NUMCITADELMUSIC; c++ )
		{
			citadelmusic[c]->release();
		}
		if ( citadelmusic )
		{
			free(citadelmusic);
		}
		for ( int c = 0; c < NUMINTROMUSIC; c++ )
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
	for ( int c = 0; c < NUMITEMS; c++ )
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
	for ( int c = 0; c < MAXPLAYERS; c++ )
	{
		if ( steamIDRemote[c] )
		{
			cpp_Free_CSteamID(steamIDRemote[c]);
			steamIDRemote[c] = NULL;
		}
	}
	for ( int c = 0; c < MAX_STEAM_LOBBIES; c++ )
	{
		if ( lobbyIDs[c] )
		{
			cpp_Free_CSteamID(lobbyIDs[c]);
			lobbyIDs[c] = NULL;
		}
	}
#endif
#if defined USE_EOS
	EOS.stop();
	EOS.quit();
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
	if ( CompendiumEntries.compendiumMap.tiles )
	{
		free(CompendiumEntries.compendiumMap.tiles);
	}
	for (int i = 0; i < MAXPLAYERS; ++i)
	{
		delete players[i];
		players[i] = nullptr;
	}

#ifdef USE_THEORA_VIDEO
	VideoManager_t::deinitManager();
#endif
#ifdef USE_IMGUI
	ImGui_t::deinit();
#endif
#ifdef USE_PLAYFAB
	playfabUser.postScoreHandler.deinit();
#endif
}

void loadAchievementData(const char* path) {
	if ( !PHYSFS_getRealDir(path) )
	{
		printlog("[JSON]: Error: Could not find file: %s", path);
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(path);
	inputPath.append(path);

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if (!fp) {
		printlog("[JSON]: Error: Could not find file: %s", path);
		return;
	}

	char buf[120000];
	int count = (int)fp->read(buf, sizeof(buf[0]), sizeof(buf));
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);

	if (!d.HasMember("achievements") || !d["achievements"].IsObject()) {
		printlog("[JSON]: Error: could not parse %s", path);
		return;
	}
	const auto& achievements = d["achievements"].GetObject();

	for (const auto& it : achievements) {
		if (!it.name.IsString()) {
			printlog("[JSON]: Error: could not parse %s", path);
			return;
		}
		auto achName = it.name.GetString();
#ifdef NINTENDO
		if ( !strcmp(achName, "BARONY_ACH_LOCAL_CUSTOMS") )
		{
			continue;
		}
#endif
#ifndef STEAMWORKS
		if ( !strcmp(achName, "BARONY_ACH_CARTOGRAPHER") )
		{
			continue;
		}
#endif
#ifndef USE_PLAYFAB
		if ( !strcmp(achName, "BARONY_ACH_BLOOM_PLANTED") )
		{
			continue;
		}
		if ( !strcmp(achName, "BARONY_ACH_DUNGEONSEED") )
		{
			continue;
		}
		if ( !strcmp(achName, "BARONY_ACH_GROWTH_MINDSET") )
		{
			continue;
		}
		if ( !strcmp(achName, "BARONY_ACH_REAP_SOW") )
		{
			continue;
		}
		if ( !strcmp(achName, "BARONY_ACH_SPROUTS") )
		{
			continue;
		}
#endif
		const auto& ach = it.value.GetObject();
		auto& achData = Compendium_t::achievements[achName];
		if (ach.HasMember("name") && ach["name"].IsString()) {
			achData.name = ach["name"].GetString();
		}
		if (ach.HasMember("description") && ach["description"].IsString()) {
			achData.desc = ach["description"].GetString();
		}
		if (ach.HasMember("hidden") && ach["hidden"].IsBool()) {
			achData.hidden = ach["hidden"].GetBool();
		}
		if ( ach.HasMember("category") )
		{
			achData.category = ach["category"].GetString();
		}
		if ( ach.HasMember("lore_points") )
		{
			achData.lorePoints = ach["lore_points"].GetInt();
		}

		achData.dlcType = Compendium_t::AchievementData_t::ACH_TYPE_NORMAL;
		if ( ach.HasMember("dlc") )
		{
			if ( ach["dlc"].IsString() )
			{
				if ( !strcmp(ach["dlc"].GetString(), "myths_outcasts") )
				{
					achData.dlcType = Compendium_t::AchievementData_t::ACH_TYPE_DLC1;
				}
				else if ( !strcmp(ach["dlc"].GetString(), "legends_pariahs") )
				{
					achData.dlcType = Compendium_t::AchievementData_t::ACH_TYPE_DLC2;
				}
			}
			else if ( ach["dlc"].IsArray() )
			{
				for ( auto it = ach["dlc"].Begin(); it != ach["dlc"].End(); ++it )
				{
					if ( it->IsString() )
					{
						if ( !strcmp(it->GetString(), "myths_outcasts") )
						{
							if ( achData.dlcType == Compendium_t::AchievementData_t::ACH_TYPE_DLC2 )
							{
								achData.dlcType = Compendium_t::AchievementData_t::ACH_TYPE_DLC1_DLC2;
							}
							else
							{
								achData.dlcType = Compendium_t::AchievementData_t::ACH_TYPE_DLC1;
							}
						}
						else if ( !strcmp(it->GetString(), "legends_pariahs") )
						{
							if ( achData.dlcType == Compendium_t::AchievementData_t::ACH_TYPE_DLC1 )
							{
								achData.dlcType = Compendium_t::AchievementData_t::ACH_TYPE_DLC1_DLC2;
							}
							else
							{
								achData.dlcType = Compendium_t::AchievementData_t::ACH_TYPE_DLC2;
							}
						}
					}
				}
			}
		}
	}

	for ( int statNum = 0; statNum < NUM_STEAM_STATISTICS; ++statNum )
	{
		if ( steamStatAchStringsAndMaxVals[statNum].first != "BARONY_ACH_NONE" )
		{
			auto find = Compendium_t::achievements.find(steamStatAchStringsAndMaxVals[statNum].first);
			if ( find != Compendium_t::achievements.end() )
			{
				find->second.achievementProgress = statNum;
			}
		}
	}

	sortAchievementsForDisplay();
}


void sortAchievementsForDisplay()
{
#ifdef STEAMWORKS
	if ( Compendium_t::AchievementData_t::achievementsNeedFirstData )
	{
		if ( SteamUser()->BLoggedOn() )
		{
			Compendium_t::AchievementData_t::achievementsNeedFirstData = false;

			for ( auto& achData : Compendium_t::achievements )
			{
				Uint32 time = 0;
				bool unlocked = false;
				SteamUserStats()->GetAchievementAndUnlockTime(achData.first.c_str(), &achData.second.unlocked, &time);
				if ( achData.second.unlocked )
				{
					achData.second.unlockTime = time;
				}
			}
		}
	}
#endif

	Compendium_t::AchievementData_t::achievementsNeedResort = false;

	// sort achievements list
	Compendium_t::AchievementData_t::achievementNamesSorted.clear();
	std::vector<std::pair<std::string, std::string>> names;
	for ( auto& achData : Compendium_t::achievements )
	{
		names.push_back(std::make_pair(achData.first, achData.second.name));
	}
	Compendium_t::AchievementData_t::Comparator compFunctor =
		[](std::pair<std::string, std::string> lhs, std::pair<std::string, std::string> rhs)
	{
		auto& achData1 = Compendium_t::achievements[lhs.first];
		auto& achData2 = Compendium_t::achievements[rhs.first];

		bool ach1 = achData1.unlocked;
		bool ach2 = achData2.unlocked;
		bool lhsAchIsHidden = achData1.hidden;
		bool rhsAchIsHidden = achData2.hidden;
		if ( !Compendium_t::AchievementData_t::sortAlphabetical )
		{
			if ( ach1 && !ach2 )
			{
				if ( Compendium_t::compendium_sorting_hide_ach_unlocked )
				{
					return false;
				}
				return true;
			}
			else if ( !ach1 && ach2 )
			{
				if ( Compendium_t::compendium_sorting_hide_ach_unlocked )
				{
					return true;
				}
				return false;
			}
			else if ( !ach1 && !ach2 && (lhsAchIsHidden || rhsAchIsHidden) )
			{
				if ( lhsAchIsHidden && rhsAchIsHidden )
				{
					return lhs.second < rhs.second;
				}
				if ( !lhsAchIsHidden )
				{
					return true;
				}
				if ( !rhsAchIsHidden )
				{
					return false;
				}
				return lhs.second < rhs.second;
			}
			else
			{
				if ( !ach1 && !ach2 )
				{
					return lhs.second < rhs.second;
				}
				else
				{
					if ( achData1.unlockTime == achData2.unlockTime )
					{
						return lhs.second < rhs.second;
					}
					else
					{
						return achData1.unlockTime > achData2.unlockTime;
					}
				}
			}
		}
		else
		{
			if ( !ach1 && !ach2 && (lhsAchIsHidden || rhsAchIsHidden) )
			{
				if ( lhsAchIsHidden && rhsAchIsHidden )
				{
					return lhs.second < rhs.second;
				}
				if ( !lhsAchIsHidden )
				{
					return true;
				}
				if ( !rhsAchIsHidden )
				{
					return false;
				}
				return lhs.second < rhs.second;
			}
			else
			{
				if ( ach1 && !ach2 )
				{
					if ( Compendium_t::compendium_sorting_hide_ach_unlocked )
					{
						return false;
					}
					return true;
				}
				else if ( !ach1 && ach2 )
				{
					if ( Compendium_t::compendium_sorting_hide_ach_unlocked )
					{
						return true;
					}
					return false;
				}
				return lhs.second < rhs.second;
			}
		}
	};

	std::set<std::pair<std::string, std::string>, Compendium_t::AchievementData_t::Comparator> sorted(
		names.begin(),
		names.end(),
		compFunctor);
	Compendium_t::AchievementData_t::achievementNamesSorted.swap(sorted);
	Compendium_t::AchievementData_t::achievementCategories.clear();
	for ( auto& entry : Compendium_t::AchievementData_t::achievementNamesSorted )
	{
		auto& achData = Compendium_t::achievements[entry.first];
		Compendium_t::AchievementData_t::achievementCategories[achData.category].push_back(entry);
	}

	Compendium_t::AchievementData_t::achievementsBookDisplay.clear();
	for ( auto& entry : Compendium_t::AchievementData_t::achievementCategories )
	{
		auto& achDisplay = Compendium_t::AchievementData_t::achievementsBookDisplay[entry.first];
		if ( entry.second.size() > 0 )
		{
			achDisplay.pages.push_back(std::vector<std::string>());
		}
		int numEntries = 0;
		bool foundHidden = false;
		for ( auto& name : entry.second )
		{
			auto& achData = Compendium_t::achievements[name.first];
			if ( foundHidden )
			{
				if ( Compendium_t::compendium_sorting_hide_ach_unlocked )
				{
					if ( achData.hidden && !achData.unlocked )
					{
						achDisplay.numHidden++;
						continue;
					}
				}
				else
				{
					if ( achData.hidden && !achData.unlocked )
					{
						achDisplay.numHidden++;
					}
					// hidden, so allow only 1 entry to represent all the hidden ones
					continue;
				}
			}
			++numEntries;
			if ( numEntries > 1 && ((numEntries - 1) % 8 == 0) )
			{
				achDisplay.pages.push_back(std::vector<std::string>());
			}
			auto& list = achDisplay.pages.back();
			list.push_back(name.first);
			if ( achData.hidden && !achData.unlocked )
			{
				foundHidden = true;
				achDisplay.numHidden++;
			}
		}
	}
}
