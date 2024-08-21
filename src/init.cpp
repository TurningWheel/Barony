/*-------------------------------------------------------------------------------

	BARONY
	File: init.cpp
	Desc: contains program initialization code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include <memory>
#include <ctime>
#include <sys/stat.h>
#include <sstream>

#include "main.hpp"
#ifdef NINTENDO
 #include "nintendo/nxplatform.hpp"
#endif // NINTENDO
#include "draw.hpp"
#include "files.hpp"
#include "engine/audio/sound.hpp"
#include "prng.hpp"
#include "hash.hpp"
#include "init.hpp"
#include "light.hpp"
#include "net.hpp"
#ifdef EDITOR
 #include "editor.hpp"
#endif // NINTENDO
#include "menu.hpp"
#ifdef STEAMWORKS
 #include <steam/steam_api.h>
 #include "steam.hpp"
#endif // STEAMWORKS
#ifndef EDITOR
#include "player.hpp"
#endif
#include "items.hpp"
#include "cppfuncs.hpp"
#include "ui/Text.hpp"
#include "ui/Font.hpp"
#include "ui/Image.hpp"
#include "ui/Frame.hpp"
#include "ui/Button.hpp"
#include "ui/LoadingScreen.hpp"
#ifndef EDITOR
#include "mod_tools.hpp"
#include "ui/MainMenu.hpp"
#include "interface/consolecommand.hpp"
static ConsoleVariable<bool> cvar_sdl_disablejoystickrawinput("/sdl_joystick_rawinput_disable", false, "disable SDL rawinput for gamepads (helps SDL_HapticOpen())");
#endif

#include <thread>
#include <future>
#include <chrono>

bool mountBaseDataFolders() {
    if (isCurrentHoliday()) {
        const auto holiday = getCurrentHoliday();
        const auto holiday_dir = holidayThemeDirs[holiday];
        const auto holiday_dir_str = (std::string(datadir) + "/") + holiday_dir;
        if (!PHYSFS_mount(holiday_dir_str.c_str(), NULL, 1)) {
            printlog("[PhysFS]: unsuccessfully mounted holiday %s folder. Error: %s",
                holiday_dir_str.c_str(), PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
            return false;
        }
    }

	if ( !PHYSFS_mount(datadir, NULL, 1) )
	{
		printlog("[PhysFS]: unsuccessfully mounted base %s folder. Error: %s",
            datadir, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
		return false;
	}

	if ( PHYSFS_mount(outputdir, NULL, 1) )
	{
		printlog("[PhysFS]: successfully mounted output \"%s\" folder", outputdir);
		if ( PHYSFS_setWriteDir(outputdir) )
		{
		    PHYSFS_mkdir("books");
			PHYSFS_mkdir("savegames");
			PHYSFS_mkdir("scores");
			PHYSFS_mkdir("scores/processing");
			//TODO: Will these need special NINTENDO handling?
			PHYSFS_mkdir("crashlogs");
			PHYSFS_mkdir("logfiles");
			PHYSFS_mkdir("data");
			PHYSFS_mkdir("data/custom-monsters");
			PHYSFS_mkdir("data/statues");
			PHYSFS_mkdir("data/scripts");
			PHYSFS_mkdir("config");
#ifdef STEAMWORKS
			PHYSFS_mkdir("workshop_cache");
#endif
#ifdef NINTENDO
			PHYSFS_mkdir("mods");
			std::string path = outputdir;
			path.append(PHYSFS_getDirSeparator()).append("mods");
			PHYSFS_setWriteDir(path.c_str()); //Umm...should it really be doing that? First off, it didn't actually create this directory. Second off, what about the rest of the directories it created?
			printlog("[PhysFS]: successfully set write folder %s", path.c_str());
#else // NINTENDO
			if ( PHYSFS_mkdir("mods") )
			{
				std::string path = outputdir;
				path.append(PHYSFS_getDirSeparator()).append("mods");
				PHYSFS_setWriteDir(path.c_str());
				printlog("[PhysFS]: successfully set write folder %s", path.c_str());
			}
			else
			{
				printlog("[PhysFS]: unsuccessfully created mods/ folder. Error: %s", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
				return false;
			}
#endif // !NINTENDO
		}
	}
	else
	{
		printlog("[PhysFS]: unsuccessfully mounted base %s folder. Error: %s", outputdir, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
		return false;
	}
 
    return true;
}

bool remountBaseDataFolders() {
#ifdef EDITOR
    return false; // don't do anything
#else
    // first unmount everything.
    bool success = true;
	char** i;
	for ( i = PHYSFS_getSearchPath(); *i != NULL; i++ ) {
        if ( PHYSFS_unmount(*i) == 0 ) {
            success = false;
            printlog("[%s] unsuccessfully removed from the search path.\n", *i);
        } else {
            printlog("[%s] is removed from the search path.\n", *i);
        }
	}
	PHYSFS_freeList(*i);
 
    // then mount base data folders
    success = mountBaseDataFolders() ? success : false;
    
    // reload files
    Mods::unloadMods(true);
    
    return success;
#endif
}

/*-------------------------------------------------------------------------------

	initApp

	initializes all the application variables and starts the engine

-------------------------------------------------------------------------------*/

FILE* logfile = nullptr;
bool steam_init = false;

int initApp(char const * const title, int fullscreen)
{
	File* fp;

	Uint32 seed;
	local_rng.seedTime();
    local_rng.getSeed(&seed, sizeof(seed));
    net_rng.seedBytes(&seed, sizeof(seed));

	// open log file
	if ( !logfile )
	{
		openLogFile();
	}

	/*for (c = 0; c < NUM_JOY_STATUS; ++c)
	{
		joystatus[c] = 0;
	}
	for (c = 0; c < NUM_JOY_TRIGGER_STATUS; ++c)
	{
		joy_trigger_status[c] = 0;
	}*/

	// init some lists
	button_l.first = NULL;
	button_l.last = NULL;
	light_l.first = NULL;
	light_l.last = NULL;
	entitiesdeleted.first = NULL;
	entitiesdeleted.last = NULL;
	for (int c = 0; c < HASH_SIZE; ++c)
	{
		ttfTextHash[c].first = NULL;
		ttfTextHash[c].last = NULL;
	}

	// init PHYSFS
#ifndef NINTENDO
	PHYSFS_init("/");
	PHYSFS_permitSymbolicLinks(1);
#endif

	if ( !PHYSFS_isInit() )
	{
		printlog("[PhysFS]: failed to initialize! Error: %s",
            PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
		return 13;
	}
	else
	{
		printlog("[PhysFS]: successfully initialized, last error: %s",
            PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	}

    if (!mountBaseDataFolders()) {
        return 13;
    }

	// load default language file (english)
	Language::languageCode = "en";
	if ( Language::reloadLanguage() )
	{
		printlog("Fatal error: failed to load default language file!\n");
		if ( logfile )
		{
			fclose(logfile);
		}
		exit(1);
	}

	// initialize SDL
	window_title = title;
	printlog("initializing SDL...\n");
#ifdef WINDOWS
#ifndef EDITOR
	if ((*cvar_sdl_disablejoystickrawinput) == true)
	{
		SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "0"); // prefer XINPUT devices, helps making SDL_HapticOpen() work on my wireless xbox controllers
		printlog("SDL_HINT_JOYSTICK_RAWINPUT set to 0");
	}
#endif
#endif
    // do this in main() now.
	/*Uint32 init_flags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;
	init_flags |= SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC;
	if (SDL_Init(init_flags) == -1)
	{
		printlog("failed to initialize SDL: %s\n", SDL_GetError());
		return 1;
	}*/

	// init steamworks
#ifdef STEAMWORKS
	SteamAPI_RestartAppIfNecessary(STEAM_APPID);
	if ( !SteamAPI_Init() )
	{
		printlog("error: failed to initialize Steamworks!\n");
		printlog(" make sure your steam client is running before attempting to start again.\n");
		return 1;
	}
	steam_init = true;
	g_SteamLeaderboards = new CSteamLeaderboards();
	g_SteamWorkshop = new CSteamWorkshop();
	g_SteamStatistics = new CSteamStatistics(g_SteamStats, g_SteamAPIGlobalStats, NUM_STEAM_STATISTICS);
    if (xres == 1280 && yres == 720 && SteamUtils()->IsSteamRunningOnSteamDeck()) {
        // default steam deck native resolution
        xres = 1280;
        yres = 800;
    }
#ifdef PANDORA
    if (xres == 1280 && yres == 720) {
        // Pandora native resolution
        xres = 800;
        yres = 480;
    }
#endif
	// Preloads mod content from a workshop fileID
	//gamemodsWorkshopPreloadMod(YOUR WORKSHOP FILE ID HERE, "YOUR WORKSHOP TITLE HERE");
#endif
#if defined USE_EOS
	EOS.readFromFile();
	EOS.readFromCmdLineArgs();
#ifndef NINTENDO
	if ( EOS.initPlatform(true) == false )
	{
		return 14;
	}
#endif
#ifndef STEAMWORKS
#ifndef NINTENDO
#ifdef APPLE
	if ( EOS.CredentialName.compare("") == 0 )
	{
		EOSFuncs::logInfo("Error, attempting to launch outside of store...");
		return 15;
	}
#else
	if ( EOS.appRequiresRestart == EOS_EResult::EOS_Success )
	{
		// restarting app
		EOSFuncs::logInfo("App attempting restart through store...");
		return 15;
	}
#endif
#endif
#ifdef NINTENDO
	EOS.SetNetworkAvailable(nxConnectedToNetwork());
#else
	EOS.initAuth();
#endif
#endif // !STEAMWORKS
#endif
#ifndef EDITOR
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		playerSettings->init(i);
	}
#endif

	// I'm not sure when we'd need the following block.
	// mobile ports????
#if 0
	auto event_filter = [](void* userdata, SDL_Event* e) -> int {
		if (!e) {
			return 0;
		}
		switch (e->type) {
		default: break;
		case SDL_APP_TERMINATING:
			// safe cleanup
			printlog("barony safely shutting down");
			saveConfig("default.cfg");
			MainMenu::settingsMount();
			(void)MainMenu::settingsSave();
			deinitGame();
			deinitApp();
			nxTerm();
			break;
		case SDL_APP_WILLENTERFOREGROUND:
		case SDL_APP_DIDENTERFOREGROUND:
		{
			printlog("barony waking up");
			static const int displays = SDL_GetNumVideoDisplays();
			std::vector<SDL_Rect> displayBounds;
			for (int i = 0; i < displays; i++) {
				displayBounds.push_back(SDL_Rect());
				SDL_GetDisplayBounds(i, &displayBounds.back());
			}
			if (displayBounds.size() > 0) {
				const int x = displayBounds[0].w;
				const int y = displayBounds[0].h;
				printlog("new display size: %d %d", x, y);
				SDL_Event new_event;
				new_event.type = SDL_WINDOWEVENT;
				new_event.window.event = SDL_WINDOWEVENT_RESIZED;
				new_event.window.data1 = x;
				new_event.window.data2 = y;
				new_event.window.timestamp = SDL_GetTicks();
				new_event.window.type = SDL_WINDOWEVENT;
				new_event.window.windowID = screen ? SDL_GetWindowID(screen) : 0;
				SDL_PushEvent(&new_event);
			}
#ifdef USE_EOS
			EOS.SetSleepStatus(false);
#endif
			break;
		}
		case SDL_APP_WILLENTERBACKGROUND:
		case SDL_APP_DIDENTERBACKGROUND:
			printlog("barony going to sleep");
#ifdef USE_EOS
			EOS.SetSleepStatus(true);
#endif
			break;
		case SDL_APP_LOWMEMORY:
			printlog("barony low memory, dumping UI cache");
			Text::dumpCache();
			Image::dumpCache();
			Font::dumpCache();
			break;
		}

		return 0;
		};
	SDL_SetEventFilter(event_filter, nullptr);
#endif

#ifdef NINTENDO
	SDL_GameControllerAddMappingsFromFile(GAME_CONTROLLER_DB_FILEPATH);
#else
	SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");
#endif

	//printlog("initializing SDL_mixer. rate: %d format: %d channels: %d buffers: %d\n", audio_rate, audio_format, audio_channels, audio_buffers);
	/*if( Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) ) {
		SDL_Quit();
		printlog("failed to initialize SDL_mixer: %s\n", Mix_GetError());
		return 2;
	}*/

#ifndef EDITOR
	initSoundEngine(); //Yes, this silently ignores the return value...(which is not good, but not important either)
#endif

	printlog("initializing SDL_net...\n");
	if ( SDLNet_Init() < 0 )
	{
		printlog("failed to initialize SDL_net: %s\n", SDLNet_GetError());
		return 2;
	}
	printlog("initializing SDL_image...\n");
	if ( IMG_Init(IMG_INIT_PNG) != (IMG_INIT_PNG) )
	{
		printlog("failed to initialize SDL_image: %s\n", IMG_GetError());
		return 2;
	}

	// hide cursor for game
	if ( game )
	{
		SDL_ShowCursor(EnableMouseCapture == SDL_FALSE ? SDL_ENABLE : SDL_DISABLE);
	}
	SDL_StopTextInput();

	// initialize video
	if ( !initVideo() )
	{
		return 3;
	}

#ifdef WINDOWS
	char gl_version[64];
	snprintf(gl_version, sizeof(gl_version), "%s", glGetString(GL_VERSION));
	printlog("[OpenGL]: Graphics Vendor: %s | Renderer: %s | Version: %s",
		glGetString(GL_VENDOR),
		glGetString(GL_RENDERER),
		glGetString(GL_VERSION));
	if ( strstr(gl_version, "1.1.0") )
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error",
			"Detected OpenGL version (1.1.0) is not compatible.\n\n"
			"Your video card drivers may be out of date, please check for updates directly from your manufacturer's website (e.g Intel/AMD/NVIDIA).\n"
			"Drivers obtained through Windows Update may not automatically find the correct drivers.\n\n"
			"If you have a computer with more than one video card, make sure you run Barony using the dedicated video card (NVIDIA/AMD).\n\n"
			"For more assistance, send the following diagnostic file output through to one of our support forums:\n"
			"- Use the run command (Windows key + R) and enter \"dxdiag\" and \"Save All Information..\" to a text file\n"
			"- Additionally send the log.txt file from the game installation directory\n\n"
			"http://www.baronygame.com/",
			screen);
	}
#else
	printlog("[OpenGL]: Graphics Vendor: %s | Renderer: %s | Version: %s",
		GL_CHECK_ERR_RET(glGetString(GL_VENDOR)),
        GL_CHECK_ERR_RET(glGetString(GL_RENDERER)),
        GL_CHECK_ERR_RET(glGetString(GL_VERSION)));
#endif

	createCommonDrawResources();
	main_framebuffer.init(xres, yres, GL_NEAREST, GL_NEAREST);
    if (!hdrEnabled) {
        main_framebuffer.bindForWriting();
    }
    GL_CHECK_ERR(glClearColor(0.f, 0.f, 0.f, 1.f));
    GL_CHECK_ERR(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	//SDL_EnableUNICODE(1);
	//SDL_WM_SetCaption(title, 0);
	//SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,SDL_DEFAULT_REPEAT_INTERVAL);

	// initialize buffers
	texid = (GLuint*) malloc(MAXTEXTURES * sizeof(GLuint));
	//vaoid = (GLuint *) malloc(MAXBUFFERS*sizeof(GLuint));
	//vboid = (GLuint *) malloc(MAXBUFFERS*sizeof(GLuint));
	allsurfaces = (SDL_Surface**) malloc(sizeof(SDL_Surface*)*MAXTEXTURES);
	for (int c = 0; c < MAXTEXTURES; ++c)
	{
		allsurfaces[c] = NULL;
	}
    GL_CHECK_ERR(glGenTextures(MAXTEXTURES, texid));

	// load windows icon
#ifndef _MSC_VER
#if defined(WINDOWS) && defined(GCL_HICON)
	HINSTANCE handle = GetModuleHandle(NULL);
	HICON icon = LoadIcon(handle, "id");
	if ( icon != NULL )
	{
		SDL_SysWMinfo wminfo;
		SDL_VERSION( &wminfo.version );
		if ( SDL_GetWindowWMInfo(screen, &wminfo) == SDL_TRUE )
		{
			HWND hwnd = wminfo.info.win.window;
			SetClassLong(hwnd, GCL_HICON, (LONG)icon);
		}
	}
#endif
#endif

	// load resources
	printlog("loading engine resources...\n");
	if ((font8x8_bmp = loadImage("images/system/font8x8.png")) == NULL)
	{
		printlog("failed to load font8x8.png\n");
		return 5;
	}
	if ((font12x12_bmp = loadImage("images/system/font12x12.png")) == NULL)
	{
		printlog("failed to load font12x12.png\n");
		return 5;
	}
	if ((font16x16_bmp = loadImage("images/system/font16x16.png")) == NULL)
	{
		printlog("failed to load font16x16.png\n");
		return 5;
	}

	// init new ui engine
	Frame::guiInit();

	// create player classes
	// TODO/FIXME: why isn't this in initGame? why is it in init.cpp?
#ifndef EDITOR
	for ( int c = 0; c < MAXPLAYERS; c++ )
	{
		players[c] = new Player(c, true);
	}
#endif

	createLoadingScreen(10);
	doLoadingScreen();

	// load sprites
	printlog("loading sprites...\n");
	fp = openDataFile("images/sprites.txt", "rb");
	for ( numsprites = 0; !fp->eof(); numsprites++ )
	{
		while ( fp->getc() != '\n' ) if ( fp->eof() )
		{
			break;
		}
	}
	FileIO::close(fp);
	if ( numsprites == 0 )
	{
		printlog("failed to identify any sprites in sprites.txt\n");
		return 6;
	}
	sprites = (SDL_Surface**) malloc(sizeof(SDL_Surface*)*numsprites);
	fp = openDataFile("images/sprites.txt", "rb");
	for ( int c = 0; !fp->eof(); c++ )
	{
		char name[128] = { '\0' };
		fp->gets2(name, 128);
		sprites[c] = loadImage(name);
		if ( sprites[c] == NULL )
		{
			printlog("warning: failed to load '%s' listed at line %d in sprites.txt\n", name, c + 1);
			if ( c == 0 )
			{
				printlog("sprite 0 cannot be NULL!\n");
				FileIO::close(fp);
				return 7;
			}
		}
	}
	FileIO::close(fp);

	updateLoadingScreen(15);
	doLoadingScreen();

	// load tiles
	std::string tilesDirectory = PHYSFS_getRealDir("images/tiles.txt");
	tilesDirectory.append(PHYSFS_getDirSeparator()).append("images/tiles.txt");
	printlog("loading tiles from directory %s...\n", tilesDirectory.c_str());
	fp = openDataFile(tilesDirectory.c_str(), "rb");
	for ( numtiles = 0; !fp->eof(); numtiles++ )
	{
		while ( fp->getc() != '\n' )
		{
			if ( fp->eof() )
			{
				break;
			}
		}
	}
	FileIO::close(fp);
	if ( numtiles == 0 )
	{
		printlog("failed to identify any tiles in tiles.txt\n");
		return 8;
	}
	tiles = (SDL_Surface**) malloc(sizeof(SDL_Surface*)*numtiles);
	animatedtiles = (bool*) malloc(sizeof(bool) * numtiles);
	lavatiles = (bool*) malloc(sizeof(bool) * numtiles);
	swimmingtiles = (bool*)malloc(sizeof(bool) * numtiles);
	fp = openDataFile(tilesDirectory.c_str(), "rb");
	for ( int c = 0; !fp->eof(); c++ )
	{
		char name[128];
		fp->gets2(name, 128);
		tiles[c] = loadImage(name);
		animatedtiles[c] = false;
		lavatiles[c] = false;
		swimmingtiles[c] = false;
		if ( tiles[c] != NULL )
		{
			for (int x = 0; x < strlen(name); x++)
			{
				if ( name[x] >= '0' && name[x] <= '9' )
				{
					// animated tiles if the tile name ends in a number 0-9.
					animatedtiles[c] = true;
					break;
				}
			}
			if ( strstr(name, "Lava") || strstr(name, "lava") )
			{
				lavatiles[c] = true;
			}
			if ( strstr(name, "Water") || strstr(name, "water") || strstr(name, "swimtile") || strstr(name, "Swimtile") )
			{
				swimmingtiles[c] = true;
			}
		}
		else
		{
			printlog("warning: failed to load '%s' listed at line %d in tiles.txt\n", name, c + 1);
			if ( c == 0 )
			{
				printlog("tile 0 cannot be NULL!\n");
				FileIO::close(fp);
				return 9;
			}
		}
	}
	FileIO::close(fp);
 
    // load animated.txt
	if (!PHYSFS_getRealDir("images/animated.txt")) {
		printlog("error: could not find file: %s", "images/animated.txt");
	} else {
        std::string directory = PHYSFS_getRealDir("images/animated.txt");
        directory.append(PHYSFS_getDirSeparator()).append("images/animated.txt");
        printlog("[PhysFS]: Loading tile animations from directory %s...\n", directory.c_str());
        File* fp = openDataFile(directory.c_str(), "rb");
        if (!fp) {
            printlog("error: could not open file: %s", "images/animated.txt");
        } else {
            for (int c = 0; !fp->eof(); ++c) {
                AnimatedTile animation;
                char line[PATH_MAX];
                fp->gets2(line, PATH_MAX);
                
                // extract animation frames
                constexpr int numIndices = sizeof(animation.indices) / sizeof(animation.indices[0]);
                char *str = line, *end;
                int index = 0;
                do {
                    animation.indices[index] = (int)strtol(str, &end, 10);
                    str = end + 1;
                    ++index;
                } while (end && *end == ' ' && index < numIndices);
                tileAnimations.insert({animation.indices[0], animation});
            }
            FileIO::close(fp);
        }
    }

	// asynchronous loading tasks
	std::atomic_bool loading_done {false};
	auto loading_task = std::async(std::launch::async, [&loading_done](){
		File* fp;

		updateLoadingScreen(20);

		// load models
		std::string modelsDirectory = PHYSFS_getRealDir("models/models.txt");
		modelsDirectory.append(PHYSFS_getDirSeparator()).append("models/models.txt");
		printlog("loading models from directory %s...\n", modelsDirectory.c_str());

		fp = openDataFile(modelsDirectory.c_str(), "rb");
		for ( nummodels = 0; !fp->eof(); nummodels++ )
		{
			while ( fp->getc() != '\n' ) if ( fp->eof() )
				{
					break;
				}
		}
		FileIO::close(fp);
		if ( nummodels == 0 )
		{
			printlog("failed to identify any models in models.txt\n");
			loading_done = true;
			return 11;
		}

#ifdef EDITOR
		modelFileNames.clear();
#endif

		models = (voxel_t**) malloc(sizeof(voxel_t*)*nummodels);
		fp = openDataFile(modelsDirectory.c_str(), "rb");
		for ( int c = 0; !fp->eof(); c++ )
		{
			char name[128];
			fp->gets2(name, 128);
			models[c] = loadVoxel(name);
			if ( models[c] == NULL )
			{
				printlog("warning: failed to load '%s' listed at line %d in models.txt\n", name, c + 1);
				if ( c == 0 )
				{
					printlog("model 0 cannot be NULL!\n");
					FileIO::close(fp);
					loading_done = true;
					return 12;
				}
				else
				{
					printlog("copying model 0 for %d as a fallback\n", c);
					auto model = (voxel_t*)malloc(sizeof(voxel_t));
					model->sizex = models[0]->sizex;
					model->sizey = models[0]->sizey;
					model->sizez = models[0]->sizez;
					const auto size = sizeof(Uint8) * model->sizex * model->sizey * model->sizez;
					model->data = (Uint8*)malloc(size);
					memcpy(model->data, models[0]->data, size);
					memcpy(model->palette, models[0]->palette, sizeof(voxel_t::palette));
					models[c] = model;
				}
			}
#ifdef EDITOR
			std::string filename = name;
			if ( filename.find("models/") != std::string::npos )
			{
				filename = filename.substr(strlen("models/"));
			}
			modelFileNames[c] = filename;
#endif
		}
		updateLoadingScreen(30);
		generatePolyModels(0, nummodels, false);
		FileIO::close(fp);
		updateLoadingScreen(60);

#ifndef EDITOR
		int soundStatus = loadSoundResources(60, 20); // start at 60% loading, progress to 80%
		if ( 0 != soundStatus )
		{
		    loading_done = true;
			return soundStatus;
		}
#endif

		updateLoadingScreen(80);

		loading_done = true;
		return 0;
	});
	while (!loading_done)
	{
		doLoadingScreen();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	int result = loading_task.get();
	if (result == 0)
	{
		generateVBOs(0, nummodels);
        generateTileTextures();
		loadLights();
	}

#ifdef EDITOR
	// Don't destroy the loading screen in the game.
	// It will be used later.
	destroyLoadingScreen();
#endif

	//createTestUI();

	return result;
}

std::map<int, std::string> Language::entries;
std::map<int, std::string> Language::tmpEntries;
std::string Language::languageCode = "";
const char* Language::get(const int line)
{
	if ( line < 0 ) {
		return "";
	}
	return entries[line].c_str();
}
void Language::reset()
{
	entries.clear();
	tmpEntries.clear();
}

/*-------------------------------------------------------------------------------

	loadLanguage

	loads the language file with the given language code in *lang

-------------------------------------------------------------------------------*/

int Language::loadLanguage(char const * const lang, bool forceLoadBaseDirectory)
{
	// open log file
	if ( !logfile )
	{
		openLogFile();
	}

	// compose filename
	char filename[128] = { 0 };
	snprintf(filename, 127, "/lang/%s.txt", lang);
	std::string langFilepath;
	if ( PHYSFS_isInit() && PHYSFS_getRealDir(filename) != NULL && !forceLoadBaseDirectory )
	{
		std::string langRealDir = PHYSFS_getRealDir(filename);
		langFilepath = langRealDir + PHYSFS_getDirSeparator() + filename;
	}
	else
	{
#ifdef NINTENDO
		langFilepath = std::string(BASE_DATA_DIR) + filename;
#else
		langFilepath = std::string(BASE_DATA_DIR) + filename;
#endif
	}

	// check if language file is valid
	if ( !dataPathExists(langFilepath.c_str()) )
	{
		// language file doesn't exist
		printlog("error: unable to locate language file: '%s'", langFilepath.c_str());
		return 1;
	}

	// check if we've loaded this language already
	/*if ( !strcmp(languageCode, lang) )
	{
		printlog("info: language '%s' already loaded", lang);
		return 1;
	}*/

	// init SDL_TTF
	if ( !TTF_WasInit() )
	{
		if ( TTF_Init() == -1 )
		{
			printlog("failed to initialize SDL_ttf.\n");
			return 1;
		}
	}

	// load fonts
	char fontName[64] = { 0 };
	char fontPath[1024];
	snprintf(fontName, 63, "lang/%s.ttf", lang);
	std::string fontFilepath;
	if ( PHYSFS_isInit() && PHYSFS_getRealDir(fontName) != NULL )
	{
		std::string fontRealDir = PHYSFS_getRealDir(fontName);
		fontFilepath = fontRealDir + PHYSFS_getDirSeparator() + fontName;
	}
	else
	{
		fontFilepath = fontName;
	}

	if ( !dataPathExists(fontFilepath.c_str()) )
	{
		strncpy(fontName, "lang/en.ttf", 63);
		if ( PHYSFS_isInit() && PHYSFS_getRealDir(fontName) != NULL )
		{
			std::string fontRealDir = PHYSFS_getRealDir(fontName);
			fontFilepath = fontRealDir + PHYSFS_getDirSeparator() + fontName;
		}
		else
		{
			fontFilepath = fontName;
		}
	}
	if ( !dataPathExists(fontFilepath.c_str()) )
	{
		printlog("error: default game font 'lang/en.ttf' not found");
		return 1;
	}
	completePath(fontPath, fontFilepath.c_str());
	if ( ttf8 )
	{
		TTF_CloseFont(ttf8);
	}
	if ((ttf8 = TTF_OpenFont(fontPath, TTF8_HEIGHT)) == NULL )
	{
		printlog("failed to load size 8 ttf: %s\n", TTF_GetError());
		return 1;
	}
	TTF_SetFontKerning(ttf8, 0);
	TTF_SetFontHinting(ttf8, TTF_HINTING_MONO);
	if ( ttf12 )
	{
		TTF_CloseFont(ttf12);
	}
	if ((ttf12 = TTF_OpenFont(fontPath, TTF12_HEIGHT)) == NULL )
	{
		printlog("failed to load size 12 ttf: %s\n", TTF_GetError());
		return 1;
	}
	TTF_SetFontKerning(ttf12, 0);
	TTF_SetFontHinting(ttf12, TTF_HINTING_MONO);
	if ( ttf16 )
	{
		TTF_CloseFont(ttf16);
	}
	if ((ttf16 = TTF_OpenFont(fontPath, TTF16_HEIGHT)) == NULL )
	{
		printlog("failed to load size 16 ttf: %s\n", TTF_GetError());
		return 1;
	}
	TTF_SetFontKerning(ttf16, 0);
	TTF_SetFontHinting(ttf16, TTF_HINTING_MONO);

	// open language file
	File* fp = FileIO::open(langFilepath.c_str(), "rb");
	if ( !fp )
	{
		printlog("error: unable to load language file: '%s'", langFilepath.c_str());
		return 1;
	}

	// store the new language code
	languageCode = lang;

	tmpEntries.clear();
	if ( forceLoadBaseDirectory )
	{
		entries.clear();
	}

	// read file
	Uint32 line;
	for ( line = 1; !fp->eof(); )
	{
		//printlog( "loading line %d...\n", line);
		char data[1024];
		int entry = 0;

		// read line from file
		int i;
		bool fileEnd = false;
		for ( i = 0; ; i++ )
		{
			data[i] = fp->getc();
			if ( fp->eof() )
			{
				fileEnd = true;
				break;
			}

			// blank or comment lines stop reading at a newline
			if ( data[i] == '\n' )
			{
				line++;
				if (data[0] == '\r' || data[0] == '\n' || data[0] == '#')
				{
					break;
				}
			}
			if (data[i] == '#')
			{
				if (data[0] != '\n' && data[0] != '\r' && data[0] != '#')
				{
					break;
				}
			}
		}
		if ( fileEnd )
		{
			break;
		}

		// skip blank and comment lines
		if ( data[0] == '\r' || data[0] == '\n' || data[0] == '#' )
		{
			continue;
		}

		data[i] = 0;

		// process line
		if ( (entry = atoi(data)) == 0 )
		{
			printlog( "warning: syntax error in '%s':%d\n bad syntax!\n", langFilepath.c_str(), line);
			continue;
		}
		else if ( entry < 0 )
		{
			printlog( "warning: syntax error in '%s':%d\n invalid language entry!\n", langFilepath.c_str(), line);
			continue;
		}
		char entryText[16] = { 0 };
		snprintf(entryText, 15, "%d", entry);
		if ( entries.find(entry) != entries.end() )
		{
			printlog("warning: duplicate entry %d in '%s':%d\n", entry, langFilepath.c_str(), line);
		}
		entries[entry] = (char*)(data + strlen(entryText) + 1);
		//printlog("loading entry %d...text: \"%s\"\n", entry, Language::get(entry));
	}

	// close file
	FileIO::close(fp);
	printlog( "successfully loaded language file '%s'\n", langFilepath.c_str());

	return 0;
}

/*-------------------------------------------------------------------------------

	reloadLanguage

	reloads the current language file

-------------------------------------------------------------------------------*/

int Language::reloadLanguage()
{
	if ( PHYSFS_isInit() && PHYSFS_getRealDir("lang/en.txt") != NULL )
	{
		std::string langRealDir = PHYSFS_getRealDir("lang/en.txt");
		if ( langRealDir != BASE_DATA_DIR )
		{
			loadLanguage("en", true); // force load the base directory first, then modded paths later.
		}
	}
	return loadLanguage(languageCode.c_str(), false);
}

/*-------------------------------------------------------------------------------

	generateVBOs

	generates VBOs/VAs from polymodel data

-------------------------------------------------------------------------------*/

static constexpr int numTileAtlases = sizeof(AnimatedTile::indices) / sizeof(AnimatedTile::indices[0]);
static GLuint tileTextures[numTileAtlases] = { 0 };

#ifndef EDITOR
static ConsoleVariable<int> cvar_tileTextureSize("/tile_texture_size", 32, "the size of a tile texture");
void readTilesJson()
{
#ifdef NINTENDO
	return;
#endif
	if ( !PHYSFS_getRealDir("/data/tiles.json") )
	{
		printlog("[JSON]: Error: Could not find file: data/tiles.json");
		return;
	}

	std::string inputPath = PHYSFS_getRealDir("/data/tiles.json");
	inputPath.append("/data/tiles.json");

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not open json file %s", inputPath.c_str());
		return;
	}


	char buf[1024];
	int count = (int)fp->read(buf, sizeof(buf[0]), sizeof(buf));
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);

	if ( d.HasMember("tile_texture_size") )
	{
		*cvar_tileTextureSize = d["tile_texture_size"].GetInt();
	}
	printlog("[JSON]: Tile texture size is: %d", *cvar_tileTextureSize);
}
#endif

void generateTileTextures() {
    destroyTileTextures();
    
#ifdef EDITOR
    constexpr int size = 32;
#else
	readTilesJson();
    const int size = *cvar_tileTextureSize;
#endif
    
    const int w = size; // width of a tile texture
    const int h = size; // height of a tile texture
    constexpr int dim = 32; // how many tile textures to put in each row and column
    const int max = numtiles < dim * dim ? numtiles : dim * dim;
    
    // create atlases
    GL_CHECK_ERR(glActiveTexture(GL_TEXTURE2));
    GL_CHECK_ERR(glGenTextures(numTileAtlases, tileTextures));
    for (int atlas = 0; atlas < numTileAtlases; ++atlas) {
        GL_CHECK_ERR(glBindTexture(GL_TEXTURE_2D, tileTextures[atlas]));
        GL_CHECK_ERR(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w * dim, h * dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr));
        for (int index = 0; index < max; ++index) {
            SDL_Surface* tile;
            
            // select tile for the atlas (it might be animated)
            auto find = tileAnimations.find(index);
            if (find == tileAnimations.end()) {
                tile = tiles[index];
            } else {
                const auto& animation = find->second;
                const int animIndex = std::clamp(animation.indices[atlas], 0, (int)numtiles - 1);
                tile = tiles[animIndex];
            }
            
            // error checking
            if (!tile) {
                // tile failed to load from tiles directory
                continue;
            }
            if (tile->w != size || tile->h != size || tile->format->BytesPerPixel != 4) {
                // incorrect format
                continue;
            }
            
            // load tile image
            SDL_LockSurface(tile);
            GL_CHECK_ERR(glTexSubImage2D(GL_TEXTURE_2D, 0,
                (index % dim) * w,
                (index / dim) * h,
                w, h, GL_RGBA, GL_UNSIGNED_BYTE, tile->pixels));
            SDL_UnlockSurface(tile);
        }
        GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
        GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
        GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
        GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
    }
    
    // reset active texture to 0 at end
    GL_CHECK_ERR(glActiveTexture(GL_TEXTURE0));
}

void destroyTileTextures() {
    for (int c = 0; c < numTileAtlases; ++c) {
        if (tileTextures[c]) {
            GL_CHECK_ERR(glDeleteTextures(1, &tileTextures[c]));
            tileTextures[c] = 0;
        }
    }
}

void bindTextureAtlas(int index) {
    GL_CHECK_ERR(glBindTexture(GL_TEXTURE_2D, tileTextures[index]));
}

/*-------------------------------------------------------------------------------

	deinitApp

	frees all memory consumed by the application and terminates the engine

-------------------------------------------------------------------------------*/
int deinitApp()
{
#ifdef USE_OPENAL
	closeOPENAL();
#endif
	// close engine
	printlog("closing engine...\n");

	finishStackTraceUnique();

	printlog("freeing engine resources...\n");
	list_FreeAll(&button_l);
	list_FreeAll(&entitiesdeleted);
	if (font8x8_bmp) {
		SDL_FreeSurface(font8x8_bmp);
	}
	if (font12x12_bmp) {
		SDL_FreeSurface(font12x12_bmp);
	}
	if (font16x16_bmp) {
		SDL_FreeSurface(font16x16_bmp);
	}
	if (ttf8) {
		TTF_CloseFont(ttf8);
	}
	if (ttf12) {
		TTF_CloseFont(ttf12);
	}
	if (ttf16) {
		TTF_CloseFont(ttf16);
	}

	printlog("freeing ui resources...\n");
	Text::dumpCache();
	Image::dumpCache();
	Font::dumpCache();
	Frame::guiDestroy();
    destroyTileTextures();

	printlog("freeing map data...\n");
	if (map.entities != nullptr) {
		list_FreeAll(map.entities);
		free(map.entities);
	}
	if (map.creatures != nullptr) {
		list_FreeAll(map.creatures); //TODO: Need to call this? Entities are only pointed to by the thing, not owned.
		delete map.creatures;
	}
	if (map.worldUI != nullptr) {
		list_FreeAll(map.worldUI);
		delete map.worldUI;
	}
	list_FreeAll(&light_l);
	if (map.tiles != nullptr) {
		free(map.tiles);
	}
#ifdef EDITOR
	if (camera.vismap != nullptr) {
		free(camera.vismap);
		camera.vismap = nullptr;
	}
#endif
	if (menucam.vismap != nullptr) {
		free(menucam.vismap);
		menucam.vismap = nullptr;
	}
	for ( int i = 0; i < MAXPLAYERS; ++i ) {
		if ( cameras[i].vismap != nullptr ) {
			free(cameras[i].vismap);
			cameras[i].vismap = nullptr;
		}
	}
	for (int c = 0; c < HASH_SIZE; ++c) {
		list_FreeAll(&ttfTextHash[c]);
	}

	// free textures
	printlog("freeing textures...\n");
	if (tiles != nullptr) {
		for (int c = 0; c < numtiles; ++c) {
			if (tiles[c]) {
				SDL_FreeSurface(tiles[c]);
			}
		}
		free(tiles);
	}
	if (animatedtiles) {
		free(animatedtiles);
		animatedtiles = nullptr;
	}
	if (lavatiles) {
		free(lavatiles);
		lavatiles = nullptr;
	}
	if (swimmingtiles) {
		free(swimmingtiles);
		swimmingtiles = nullptr;
	}

	// free sprites
	printlog("freeing sprites...\n");
	if (sprites != nullptr) {
		for (int c = 0; c < numsprites; ++c) {
			if ( sprites[c] ) {
				SDL_FreeSurface(sprites[c]);
			}
		}
		free(sprites);
	}

	// free models
	printlog("freeing models...\n");
	if (models != nullptr) {
        for (int c = 0; c < nummodels; ++c) {
            if (models[c] != nullptr) {
                if (models[c]->data) {
					free(models[c]->data);
				}
				free(models[c]);
			}
		}
		free(models);
	}
	if (polymodels != nullptr) {
		for (int c = 0; c < nummodels; ++c) {
			if (polymodels[c].faces) {
				free(polymodels[c].faces);
				polymodels[c].faces = nullptr;
			}
		}
		if (!disablevbos) {
            for (int c = 0; c < nummodels; ++c) {
                if (polymodels[c].vao) {
                    GL_CHECK_ERR(glDeleteVertexArrays(1, &polymodels[c].vao));
                }
                if (polymodels[c].positions) {
                    GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].positions));
                }
                if (polymodels[c].colors) {
                    GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].colors));
                }
                if (polymodels[c].normals) {
                    GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].normals));
                }
			}
		}
		free(polymodels);
		polymodels = nullptr;
	}

#ifndef EDITOR
	freeSoundResources();
#endif

	// delete opengl buffers
	if (allsurfaces != nullptr) {
		free(allsurfaces);
	}
    if (texid != nullptr) {
        GL_CHECK_ERR(glDeleteTextures(MAXTEXTURES, texid));
		free(texid);
	}

	// close network interfaces
	closeNetworkInterfaces();

	// shutdown SDL subsystems
	printlog("shutting down SDL and its subsystems...\n");
#ifndef NINTENDO
	SDLNet_Quit();
#endif
	IMG_Quit();
	//Mix_HaltChannel(-1);
	//Mix_CloseAudio();
#ifndef EDITOR
	exitSoundEngine();
#endif
	destroyCommonDrawResources();
	main_framebuffer.destroy();
	if (renderer) {
		SDL_GL_DeleteContext(renderer);
		renderer = NULL;
	}
	if (screen) {
		SDL_DestroyWindow(screen);
		screen = NULL;
	}
	TTF_Quit();
	SDL_Quit();

	// shutdown steamworks
#ifdef STEAMWORKS
	if (steam_init) {
		printlog("storing user stats to Steam...\n");
		SteamUserStats()->StoreStats();
		if (g_SteamLeaderboards) {
			delete g_SteamLeaderboards;
		}
		if (g_SteamWorkshop) {
			delete g_SteamWorkshop;
		}
		if (g_SteamStatistics) {
			delete g_SteamStatistics;
		}
		SteamAPI_Shutdown();
	}
#endif


#ifndef NINTENDO
	int numLogFilesToKeepInArchive = 30;
	// archive logfiles.
	char lognamewithTimestamp[128];
    std::time_t timeNow = getTime();
	struct tm *localTimeNow = nullptr;
	localTimeNow = std::localtime(&timeNow);

	snprintf(lognamewithTimestamp, 127, "log_%4d%02d%02d_%02d%02d%02d.txt", 
		localTimeNow->tm_year + 1900, localTimeNow->tm_mon + 1, localTimeNow->tm_mday, localTimeNow->tm_hour, localTimeNow->tm_min, localTimeNow->tm_sec);

	std::string logarchivePath = outputdir;
	logarchivePath.append(PHYSFS_getDirSeparator()).append("logfiles").append(PHYSFS_getDirSeparator());
	std::string logarchiveFilePath = logarchivePath + lognamewithTimestamp;

	
	// prune any old logfiles if qty >= numLogFilesToKeepInArchive 
	std::vector<std::pair<int, std::string>> sortedLogFiles;
	auto archivedFiles = directoryContents(logarchivePath.c_str(), false, true);
	if ( !archivedFiles.empty() && archivedFiles.size() >= numLogFilesToKeepInArchive )
	{
		// first find the date modified of log files.
		for ( auto file : archivedFiles )
		{
			struct tm *tm = nullptr;
//#ifdef WINDOWS
			std::string filePath = logarchivePath + file;
#ifdef WINDOWS
			struct _stat fileDateModified;
			if ( _stat(filePath.c_str(), &fileDateModified) == 0 )
#else
			struct stat fileDateModified;
			if ( stat(filePath.c_str(), &fileDateModified) == 0 )
#endif
			{
				tm = localtime(&fileDateModified.st_mtime);
			}
//#else
			// UNIX/MAC
			/*struct stat fileDateModified;
			if ( stat(filePath.c_str(), &fileDateModified) == 0 )
			{
			tm = localtime(&fileDateModified.st_mtime);
			}*/
//#endif
			if ( tm )
			{
				int timeDifference = std::difftime(timeNow, mktime(tm));
				sortedLogFiles.push_back(std::make_pair(timeDifference, file));
			}
		}
	}
	std::sort(sortedLogFiles.begin(), sortedLogFiles.end()); // sort most recent to oldest.
	while ( sortedLogFiles.size() >= numLogFilesToKeepInArchive )
	{
		std::string logToRemove = logarchivePath + sortedLogFiles.back().second;
		printlog("notice: Deleting archived log file %s due to number of old log files (%d) exceeds limit of %d.", logToRemove.c_str(), sortedLogFiles.size(), numLogFilesToKeepInArchive);
		if ( access(logToRemove.c_str(), F_OK) != -1 )
		{
			int result = remove(logToRemove.c_str());
			if ( result )
			{
				printlog("warning: failed to delete logfile %s", logToRemove.c_str());
			}
		}
		else
		{
			printlog("warning: could not access logfile %s", logToRemove.c_str());
		}
		sortedLogFiles.pop_back();
	}

	if ( PHYSFS_isInit() )
	{
		PHYSFS_deinit();
		printlog("[PhysFS]: De-initializing...\n");
	}

	printlog("notice: archiving log file as %s...\n", logarchiveFilePath.c_str());
	printlog("success\n");
	if (logfile)
	{
		fclose(logfile);
		logfile = nullptr;
	}

	// copy the log file into the archives.
	char logToArchive[PATH_MAX];
	completePath(logToArchive, "log.txt", outputdir);
#ifdef WINDOWS
	CopyFileA(logToArchive, logarchiveFilePath.c_str(), false);
#elif defined NINTENDO
	// TODO?
#else //LINUX & APPLE
	std::stringstream ss;
	ss << "cp " << logToArchive << " " << logarchiveFilePath.c_str();
	system(ss.str().c_str());
#endif // WINDOWS
#endif //ndef NINTENDO
	return 0;
}

/*-------------------------------------------------------------------------------

	initVideo

	Sets the SDL/openGL context using global video variables

-------------------------------------------------------------------------------*/

static void positionAndLimitWindow(int& x, int& y, int& w, int& h)
{
#ifdef NINTENDO
	// don't do anything on nintendo.
	// SDL_GetDisplayBounds() isn't helpful, because it just returns
	// the size of the current display, which is incorrect when you're
	// trying to switch the display size.
	return;
#else
	static const int displays = SDL_GetNumVideoDisplays();
	std::vector<SDL_Rect> displayBounds;
	for (int i = 0; i < displays; i++) {
		displayBounds.push_back(SDL_Rect());
		SDL_GetDisplayBounds(i, &displayBounds.back());
	}
	if (display_id >= 0 && display_id < displays) {
		auto& bound = displayBounds[display_id];
		if (fullscreen) {
			x = bound.x;
			y = bound.y;
			//w = std::min(bound.w, w);
			//h = std::min(bound.h, h);
		}
		else {
            //w = std::min(bound.w, w);
            //h = std::min(bound.h, h);
			x = bound.x + (bound.w - w) / 2;
			y = bound.y + (bound.h - h) / 2;
		}
	}
#endif
}

bool initVideo()
{
    if (!renderer) {
        // On Apple:
        // * the highest supported compatibility-profile version is 2.1
        // * the highest supported core-profile version is 4.1
        // * the lowest supported core-profile version is 3.2
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		//SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    }

	// desired screen dimensions + position
    int screen_x = 0;
    int screen_y = 0;
	int screen_width = xres;
	int screen_height = yres;

	printlog("attempting to set display mode to %dx%d on device %d...", screen_width, screen_height, display_id);

	/*

	2022-11-16

	Fullscreen modes in SDL2 are absolutely broken right now on all platforms besides Windows. We are experiencing:

	- display server crashes when changing video mode (must be reset in a desktop properties window)
	- severe visual glitches when reverting to windowed mode in fullscreen desktop
	- fullscreen desktop mode only supports native res - you can't downscale
	- window can be placed on the wrong display, and on wayland, it can't be moved at all
	- window size sometimes changes but not actual display mode
	- window position sometimes wrong, mouse stops at wrong place
	- huge black bars (display mode or window size changes, but not contents)
    - on macOS 13 "Ventura" SDL_SetWindowFullscreen() crashes to desktop... lovely

	So, "true" fullscreen mode is cancelled on POSIX devices. Thanks SDL.

	*/

	if ( !screen )
	{
        Uint32 flags = 0;
        flags |= SDL_WINDOW_OPENGL;
        
#ifndef EDITOR
        flags |= SDL_WINDOW_ALLOW_HIGHDPI;
#endif
        
        flags |= SDL_WINDOW_RESIZABLE;
        
#ifdef PANDORA
	    flags |= SDL_WINDOW_FULLSCREEN;
#endif
        
#ifdef NINTENDO
    	flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#else
        if (fullscreen) {
            flags |= SDL_WINDOW_FULLSCREEN;
        }
        if (borderless) {
            flags |= SDL_WINDOW_BORDERLESS;
        }
#endif

		positionAndLimitWindow(screen_x, screen_y, screen_width, screen_height);
        
        if ((screen = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            screen_width, screen_height, flags)) == nullptr)
        {
            printlog("failed to set video mode.\n");
            return false;
        }
        
#ifndef NINTENDO
        // make sure that we actually got the window size we wanted
        SDL_GL_GetDrawableSize(screen, &xres, &yres);
        SDL_DestroyWindow(screen);
        const float factorx = (float)xres / screen_width;
        const float factory = (float)yres / screen_height;
        if ((screen = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            (int)(screen_width / factorx), (int)(screen_height / factory), flags)) == nullptr)
        {
            printlog("failed to set video mode.\n");
            return false;
        }
#endif
	}
	else
	{
	    SDL_SetWindowFullscreen(screen, 0);

		positionAndLimitWindow(screen_x, screen_y, screen_width, screen_height);

		SDL_RestoreWindow(screen); // if the window is maximized, we need to un-maximize it.
		SDL_SetWindowBordered(screen, borderless ? SDL_bool::SDL_FALSE : SDL_bool::SDL_TRUE);
		SDL_SetWindowPosition(screen, screen_x, screen_y);
		SDL_SetWindowSize(screen, screen_width, screen_height);
        SDL_GL_GetDrawableSize(screen, &xres, &yres);
        printlog("set window size to %dx%d", xres, yres);

		if (fullscreen) {
			SDL_DisplayMode mode;
			SDL_GetDesktopDisplayMode(display_id, &mode);
			mode.w = xres;
			mode.h = yres;
			SDL_SetWindowDisplayMode(screen, &mode);
            SDL_SetWindowFullscreen(screen, SDL_WINDOW_FULLSCREEN);
		}
	}

	if ( !renderer )
	{
		if ((renderer = SDL_GL_CreateContext(screen)) == NULL)
		{
			printlog("failed to create GL context. Reason: \"%s\"\n", SDL_GetError());
			printlog("You may need to update your video drivers.\n");
			return false;
		}

#ifdef WINDOWS
		glewInit();
#endif

#ifdef NINTENDO
		initNxGL();
#endif
        
        // do this to fix the window size/position caused by high-dpi scaling
        int w1, w2, h1, h2;
        SDL_GL_GetDrawableSize(screen, &w1, &h1);
        SDL_GetWindowSize(screen, &w2, &h2);
        const float factorX = (float)w1 / w2;
        const float factorY = (float)h1 / h2;
        SDL_SetWindowSize(screen, screen_width / factorX, screen_height / factorY);
        SDL_GL_GetDrawableSize(screen, &xres, &yres);
        printlog("set window size to %dx%d", xres, yres);
        
        // setup opengl
        GL_CHECK_ERR(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
        GL_CHECK_ERR(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
        GL_CHECK_ERR(glEnable(GL_LINE_SMOOTH));
        //GL_CHECK_ERR(glEnable(GL_TEXTURE_2D));
        GL_CHECK_ERR(glEnable(GL_CULL_FACE));
        GL_CHECK_ERR(glCullFace(GL_BACK));
        GL_CHECK_ERR(glDisable(GL_DEPTH_TEST));
        //GL_CHECK_ERR(glDisable(GL_LIGHTING));
        GL_CHECK_ERR(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        GL_CHECK_ERR(glClearColor(0.f, 0.f, 0.f, 0.f));
	}

	if ( verticalSync )
	{
		SDL_GL_SetSwapInterval(1);
	}
	else
	{
		SDL_GL_SetSwapInterval(0);
	}

	printlog("display changed successfully.\n");
	return true;
}

/*-------------------------------------------------------------------------------

	changeVideoMode

	In windows: saves the openGL context, sets the video mode, and restores
	the context
	otherwise: acts as a wrapper for initVideo

-------------------------------------------------------------------------------*/

bool changeVideoMode(int new_xres, int new_yres)
{
    float factorX, factorY;
    {
        int w1, w2, h1, h2;
        SDL_GL_GetDrawableSize(screen, &w1, &h1);
        SDL_GetWindowSize(screen, &w2, &h2);
        factorX = (float)w1 / w2;
        factorY = (float)h1 / h2;
    }
    if (new_xres) {
        xres = std::max(1024, new_xres);
    }
    if (new_yres) {
        yres = std::max(720, new_yres);
    }
    xres /= factorX;
    yres /= factorY;
	printlog("changing video mode (%d x %d).\n", xres, yres);

	// destroy gui fbo
	main_framebuffer.destroy();
	Frame::fboDestroy();

	// set video mode
	int result = initVideo();
	if ( !result )
	{
#if defined(APPLE) && !defined(EDITOR)
        xres = 2560;
        yres = 1440;
#else
        xres = 1280;
		yres = 720;
#endif
		fullscreen = 0;
		borderless = false;
		printlog("defaulting to safe video mode...\n");
		if ( !initVideo() )
		{
			return false;
		}
	}

    // create new framebuffers
	Frame::fboInit();
    if (!hdrEnabled) {
        main_framebuffer.unbindForWriting();
    }
	main_framebuffer.init(xres, yres, GL_NEAREST, GL_NEAREST);
    if (!hdrEnabled) {
        main_framebuffer.bindForWriting();
    }
    GL_CHECK_ERR(glClearColor(0.f, 0.f, 0.f, 1.f));
    GL_CHECK_ERR(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	// success
	return true;
}

bool resizeWindow(int new_xres, int new_yres)
{
    if (!screen || !renderer) {
        return false;
    }
    
	if (new_xres) {
		xres = std::max(100, new_xres);
	}
	if (new_yres) {
		yres = std::max(100, new_yres);
	}

	// destroy fbos
	main_framebuffer.destroy();
	Frame::fboDestroy();

#ifndef EDITOR
    if (!intro) {
        MainMenu::setupSplitscreen();
    }
#endif

	// create new framebuffers
	Frame::fboInit();
    if (!hdrEnabled) {
        main_framebuffer.unbindForWriting();
    }
	main_framebuffer.init(xres, yres, GL_NEAREST, GL_NEAREST);
    if (!hdrEnabled) {
        main_framebuffer.bindForWriting();
    }
    GL_CHECK_ERR(glClearColor(0.f, 0.f, 0.f, 1.f));
    GL_CHECK_ERR(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	// success
	return true;
}
