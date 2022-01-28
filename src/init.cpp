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
#include "net.hpp"
#ifndef NINTENDO
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

#include <thread>
#include <future>
#include <chrono>


/*-------------------------------------------------------------------------------

	initApp

	initializes all the application variables and starts the engine

-------------------------------------------------------------------------------*/

#ifdef PANDORA
// Pandora FBO
GLuint fbo_fbo = 0;
GLuint fbo_tex = 0;
GLuint fbo_trn = 0;
GLuint fbo_ren = 0;
#endif

FILE* logfile = nullptr;
bool steam_init = false;

int initApp(char const * const title, int fullscreen)
{
	char name[128];
	File* fp;
	Uint32 x, c;

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
	for ( c = 0; c < HASH_SIZE; c++ )
	{
		ttfTextHash[c].first = NULL;
		ttfTextHash[c].last = NULL;
	}

	// init PHYSFS
#ifndef NINTENDO
	PHYSFS_init("/");
	PHYSFS_permitSymbolicLinks(1);
#endif // NINTENDO

	if ( !PHYSFS_isInit() )
	{
		printlog("[PhysFS]: failed to initialize! Error: %s", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
		return 13;
	}
	else
	{
		printlog("[PhysFS]: successfully initialized, last error: %s", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	}

	if ( !PHYSFS_mount(datadir, NULL, 1) )
	{
		printlog("[PhysFS]: unsuccessfully mounted base %s folder. Error: %s", datadir, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
		return 13;
	}

	if ( PHYSFS_mount(outputdir, NULL, 1) )
	{
		printlog("[PhysFS]: successfully mounted output \"%s\" folder", outputdir);
		if ( PHYSFS_setWriteDir(outputdir) )
		{
		    PHYSFS_mkdir("books");
			PHYSFS_mkdir("savegames");
			//TODO: Will these need special NINTENDO handling?
			PHYSFS_mkdir("crashlogs");
			PHYSFS_mkdir("logfiles");
			PHYSFS_mkdir("data");
			PHYSFS_mkdir("data/custom-monsters");
			PHYSFS_mkdir("data/statues");
			PHYSFS_mkdir("config");
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
				return 13;
			}
#endif // !NINTENDO
		}
	}
	else
	{
		printlog("[PhysFS]: unsuccessfully mounted base %s folder. Error: %s", outputdir, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
		return 13;
	}

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
	// Preloads mod content from a workshop fileID
	//gamemodsWorkshopPreloadMod(YOUR WORKSHOP FILE ID HERE, "YOUR WORKSHOP TITLE HERE");
#endif
#if defined USE_EOS
	EOS.readFromFile();
	EOS.readFromCmdLineArgs();
	if ( EOS.initPlatform(true) == false )
	{
		return 14;
	}
#ifndef STEAMWORKS
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
	EOS.initAuth();
#endif // !STEAMWORKS
#endif

	window_title = title;
	printlog("initializing SDL...\n");
	if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER 
		| SDL_INIT_EVENTS | SDL_INIT_JOYSTICK 
		| SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) == -1 )
	{
		printlog("failed to initialize SDL: %s\n", SDL_GetError());
		return 1;
	}
	//printlog("initializing SDL_mixer. rate: %d format: %d channels: %d buffers: %d\n", audio_rate, audio_format, audio_channels, audio_buffers);
	/*if( Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) ) {
		SDL_Quit();
		printlog("failed to initialize SDL_mixer: %s\n", Mix_GetError());
		return 2;
	}*/

	initSoundEngine(); //Yes, this silently ignores the return value...(which is not good, but not important either)

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
	//SDL_EnableUNICODE(1);
	//SDL_WM_SetCaption(title, 0);
	//SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,SDL_DEFAULT_REPEAT_INTERVAL);

	// get pointers to opengl extensions
#ifdef WINDOWS
	bool noextensions = false;
	if ( !softwaremode )
	{
		if ( (SDL_glGenBuffers = (PFNGLGENBUFFERSPROC)SDL_GL_GetProcAddress("glGenBuffers")) == NULL )
		{
			noextensions = true;
		}
		else if ( (SDL_glBindBuffer = (PFNGLBINDBUFFERPROC)SDL_GL_GetProcAddress("glBindBuffer")) == NULL )
		{
			noextensions = true;
		}
		else if ( (SDL_glBufferData = (PFNGLBUFFERDATAPROC)SDL_GL_GetProcAddress("glBufferData")) == NULL )
		{
			noextensions = true;
		}
		else if ( (SDL_glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteBuffers")) == NULL )
		{
			noextensions = true;
		}
		else if ( (SDL_glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)SDL_GL_GetProcAddress("glGenVertexArrays")) == NULL )
		{
			noextensions = true;
		}
		else if ( (SDL_glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)SDL_GL_GetProcAddress("glBindVertexArray")) == NULL )
		{
			noextensions = true;
		}
		else if ( (SDL_glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)SDL_GL_GetProcAddress("glDeleteVertexArrays")) == NULL )
		{
			noextensions = true;
		}
		else if ( (SDL_glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glGenFramebuffers")) == NULL )
		{
			noextensions = true;
		}
		else if ( (SDL_glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteFramebuffers")) == NULL )
		{
			noextensions = true;
		}
		else if ( (SDL_glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)SDL_GL_GetProcAddress("glBindFramebuffer")) == NULL )
		{
			noextensions = true;
		}
		else if ( (SDL_glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)SDL_GL_GetProcAddress("glFramebufferTexture2D")) == NULL )
		{
			noextensions = true;
		}
		else if ( (SDL_glDrawBuffers = (PFNGLDRAWBUFFERSPROC)SDL_GL_GetProcAddress("glDrawBuffers")) == NULL )
		{
			noextensions = true;
		}
		else if ( (SDL_glBlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC)SDL_GL_GetProcAddress("glBlendFuncSeparate")) == NULL )
		{
			noextensions = true;
		}
/*
// Unused
		else if ( (SDL_glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)SDL_GL_GetProcAddress("glEnableVertexAttribArray")) == NULL )
		{
			noextensions = true;
		}
		else if ( (SDL_glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)SDL_GL_GetProcAddress("glVertexAttribPointer")) == NULL )
		{
			noextensions = true;
		}
*/
	}
	if (softwaremode)
	{
		printlog("notice: using software rendering.\n");
	}
	if ( noextensions )
	{
		printlog("warning: failed to load OpenGL extensions.\nYou may want to update your drivers or your graphics card, as performance will be reduced without these.\n");
		disablevbos = true;
	}
#else
	if (softwaremode)
	{
		printlog("notice: using software rendering.\n");
	}
#endif

	// initialize buffers
	zbuffer = (real_t*) malloc(sizeof(real_t) * xres * yres);
	clickmap = (Entity**) malloc(sizeof(Entity*)*xres * yres);
	texid = (GLuint*) malloc(MAXTEXTURES * sizeof(GLuint));
	//vaoid = (GLuint *) malloc(MAXBUFFERS*sizeof(GLuint));
	//vboid = (GLuint *) malloc(MAXBUFFERS*sizeof(GLuint));
	allsurfaces = (SDL_Surface**) malloc(sizeof(SDL_Surface*)*MAXTEXTURES);
	for ( c = 0; c < MAXTEXTURES; c++ )
	{
		allsurfaces[c] = NULL;
	}
	glGenTextures(MAXTEXTURES, texid);
	//SDL_glGenVertexArrays(MAXBUFFERS, vaoid);
	//SDL_glGenBuffers(MAXBUFFERS, vboid);

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
	if ((fancyWindow_bmp = loadImage("images/system/fancyWindow.png")) == NULL)
	{
		printlog("failed to load fancyWindow.png\n");
		return 5;
	}
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

	// cache language entries
	bool cacheText = false;
	if (cacheText) { //This will never run. Why is this here?
		for (int c = 0; c < NUMLANGENTRIES; ++c) {
			bool foundSpecialChar = false;
			for (int i = 0; language[c][i] != '\0'; ++i) {
				if (language[c][i] == '\\' || language[c][i] == '%') {
					foundSpecialChar = true;
				}
			}
			if (foundSpecialChar) {
				continue;
			}
			ttfPrintText(ttf8, 0, -200, language[c]);
			ttfPrintText(ttf12, 0, -200, language[c]);
			ttfPrintText(ttf16, 0, -200, language[c]);
		}
	}

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
	fp = openDataFile("images/sprites.txt", "r");
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
	fp = openDataFile("images/sprites.txt", "r");
	for ( int c = 0; !fp->eof(); c++ )
	{
		char name[128];
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
	fp = openDataFile(tilesDirectory.c_str(), "r");
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
	fp = openDataFile(tilesDirectory.c_str(), "r");
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

	// asynchronous loading tasks
	std::atomic_bool loading_done {false};
	auto loading_task = std::async(std::launch::async, [&loading_done](){
		File* fp;

		updateLoadingScreen(20);

		// load models
		std::string modelsDirectory = PHYSFS_getRealDir("models/models.txt");
		modelsDirectory.append(PHYSFS_getDirSeparator()).append("models/models.txt");
		printlog("loading models from directory %s...\n", modelsDirectory.c_str());

		fp = openDataFile(modelsDirectory.c_str(), "r");
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
		models = (voxel_t**) malloc(sizeof(voxel_t*)*nummodels);
		fp = openDataFile(modelsDirectory.c_str(), "r");
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
			}
		}
		updateLoadingScreen(30);
		if ( !softwaremode )
		{
			generatePolyModels(0, nummodels, false);
		}
		FileIO::close(fp);
		updateLoadingScreen(60);

		// TODO this function now needs to call updateLoadingScreen() as well.
		int soundStatus = loadSoundResources();
		if ( 0 != soundStatus )
		{
			return soundStatus;
		}

		updateLoadingScreen(90);
		loading_done = true;
		return 0;
	});
	while (!loading_done)
	{
		doLoadingScreen();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	destroyLoadingScreen();

	int result = loading_task.get();
	if (result == 0)
	{
		generateVBOs(0, nummodels);
	}

	//createTestUI();

	return result;
}

/*-------------------------------------------------------------------------------

	loadLanguage

	loads the language file with the given language code in *lang

-------------------------------------------------------------------------------*/

int loadLanguage(char const * const lang)
{
	char filename[128] = { 0 };
	File* fp;
	int c;

	// open log file
	if ( !logfile )
	{
		openLogFile();
	}

	// compose filename
	snprintf(filename, 127, "lang/%s.txt", lang);
	std::string langFilepath;
	if ( PHYSFS_isInit() && PHYSFS_getRealDir(filename) != NULL )
	{
		std::string langRealDir = PHYSFS_getRealDir(filename);
		langFilepath = langRealDir + PHYSFS_getDirSeparator() + filename;
	}
	else
	{
		langFilepath = filename;
	}

	// check if language file is valid
	if ( !dataPathExists(langFilepath.c_str()) )
	{
		// language file doesn't exist
		printlog("error: unable to locate language file: '%s'", langFilepath.c_str());
		return 1;
	}

	// check if we've loaded this language already
	if ( !strcmp(languageCode, lang) )
	{
		printlog("info: language '%s' already loaded", lang);
		return 1;
	}

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
	if ( (fp = openDataFile(langFilepath.c_str(), "r")) == NULL )
	{
		printlog("error: unable to load language file: '%s'", langFilepath.c_str());
		return 1;
	}

	// free currently loaded language if any
	freeLanguages();

	// store the new language code
	strcpy(languageCode, lang);

	// allocate new language strings
	language = (char**) calloc(NUMLANGENTRIES, sizeof(char*));

	// Allocate an emptry string for each possible language entry
	for (c = 0; c < NUMLANGENTRIES; c++)
	{
		language[c] = (char*)calloc(1, sizeof(char));
	}

	// read file
	Uint32 line;
	for ( line = 1; !fp->eof(); )
	{
		//printlog( "loading line %d...\n", line);
		char data[1024];
		int entry = NUMLANGENTRIES;

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
		else if ( entry >= NUMLANGENTRIES || entry < 0 )
		{
			printlog( "warning: syntax error in '%s':%d\n invalid language entry!\n", langFilepath.c_str(), line);
			continue;
		}
		char entryText[16] = { 0 };
		snprintf(entryText, 15, "%d", entry);
		if ( language[entry][0] )
		{
			printlog( "warning: duplicate entry %d in '%s':%d\n", entry, langFilepath.c_str(), line);
			free(language[entry]);
		}
		language[entry] = (char*) calloc(strlen((char*)(data + strlen(entryText) + 1)) + 1, sizeof(char));
		strcpy(language[entry], (char*)(data + strlen(entryText) + 1));
		//printlog("loading entry %d...text: \"%s\"\n", entry, language[entry]);
	}

	// close file
	FileIO::close(fp);
	printlog( "successfully loaded language file '%s'\n", langFilepath.c_str());

	// update item internal language entries.
	for ( int c = 0; c < NUMITEMS; ++c )
	{
		if ( c > SPELLBOOK_DETECT_FOOD )
		{
			int newItems = c - SPELLBOOK_DETECT_FOOD - 1;
			items[c].name_identified = language[3500 + newItems * 2];
			items[c].name_unidentified = language[3501 + newItems * 2];
		}
		else if ( c > ARTIFACT_BOW )
		{
			int newItems = c - ARTIFACT_BOW - 1;
			items[c].name_identified = language[2200 + newItems * 2];
			items[c].name_unidentified = language[2201 + newItems * 2];
		}
		else
		{
			items[c].name_identified = language[1545 + c * 2];
			items[c].name_unidentified = language[1546 + c * 2];
		}
	}
	initMenuOptions();
	return 0;
}

/*-------------------------------------------------------------------------------

	reloadLanguage

	reloads the current language file

-------------------------------------------------------------------------------*/

int reloadLanguage()
{
	char lang[32];

	strcpy(lang, languageCode);
	strcpy(languageCode, "");
	return loadLanguage(lang);
}

/*-------------------------------------------------------------------------------
 *
       freeLanguages

	free languages string resources

--------------------------------------------------------------------------------*/

void freeLanguages()
{
	int c;

	if ( language )
	{
		for ( c = 0; c < NUMLANGENTRIES; c++ )
		{
			char* entry = language[c];
			if ( entry )
			{
				free(entry);
			}
		}
		free(language);
	}
}

/*-------------------------------------------------------------------------------

	generatePolyModels

	processes voxel models and turns them into polygon-based models (surface
	optimized)

-------------------------------------------------------------------------------*/

void generatePolyModels(int start, int end, bool forceCacheRebuild)
{
	Sint32 x, y, z;
	Sint32 c, i;
	Uint32 index, indexdown[3];
	Uint8 newcolor, oldcolor;
	bool buildingquad;
	polyquad_t* quad1, *quad2;
	Uint32 numquads;
	list_t quads;
	File *model_cache;
	bool generateAll = start == 0 && end == nummodels;

	quads.first = NULL;
	quads.last = NULL;

	printlog("generating poly models...\n");
	if ( generateAll )
	{
		polymodels = (polymodel_t*) malloc(sizeof(polymodel_t) * nummodels);
		if ( useModelCache )
		{
            std::string cache_path = std::string(outputdir) + "models.cache";
			model_cache = openDataFile(cache_path.c_str(), "rb");
			if ( model_cache )
			{
				char polymodelsVersionStr[7] = "v0.0.0";
				char modelsCacheHeader[7] = "000000";
				model_cache->read(&modelsCacheHeader, sizeof(char), strlen("BARONY"));

				if ( !strcmp(modelsCacheHeader, "BARONY") )
				{
					// we're using the new polymodels file.
					model_cache->read(&polymodelsVersionStr, sizeof(char), strlen(VERSION));
					printlog("[MODEL CACHE]: Using updated version format %s.", polymodelsVersionStr);
					if ( strncmp(polymodelsVersionStr, VERSION, strlen(VERSION)) )
					{
						// different version.
						forceCacheRebuild = true;
						printlog("[MODEL CACHE]: Detected outdated version number %s - current is %s. Upgrading cache...", polymodelsVersionStr, VERSION);
					}
				}
				else
				{
					printlog("[MODEL CACHE]: Detected legacy cache without embedded version data, upgrading cache to %s...", VERSION);
					model_cache->rewind();
					forceCacheRebuild = true; // upgrade from legacy cache
				}
				if ( !forceCacheRebuild )
				{
					for (size_t model_index = 0; model_index < nummodels; model_index++) {
						updateLoadingScreen(30 + ((real_t)model_index / nummodels) * 30.0);
						polymodel_t *cur = &polymodels[model_index];
						model_cache->read(&cur->numfaces, sizeof(cur->numfaces), 1);
						cur->faces = (polytriangle_t *) calloc(sizeof(polytriangle_t), cur->numfaces);
						model_cache->read(polymodels[model_index].faces, sizeof(polytriangle_t), cur->numfaces);
					}
					FileIO::close(model_cache);
					return;
				}
				else
				{
					FileIO::close(model_cache);
				}
			}
		}
	}

	for ( c = start; c < end; ++c )
	{
		updateLoadingScreen(30 + ((real_t)(c - start) / (end - start)) * 30.0);
		numquads = 0;
		polymodels[c].numfaces = 0;
		voxel_t* model = models[c];
		if ( !model )
		{
			continue;
		}
		indexdown[0] = model->sizez * model->sizey;
		indexdown[1] = model->sizez;
		indexdown[2] = 1;

		// find front faces
		for ( x = models[c]->sizex - 1; x >= 0; x-- )
		{
			for ( z = 0; z < models[c]->sizez; z++ )
			{
				oldcolor = 255;
				buildingquad = false;
				for ( y = 0; y < models[c]->sizey; y++ )
				{
					index = z + y * models[c]->sizez + x * models[c]->sizey * models[c]->sizez;
					newcolor = models[c]->data[index];
					if ( buildingquad == true )
					{
						bool doit = false;
						if ( newcolor != oldcolor )
						{
							doit = true;
						}
						else if ( x < models[c]->sizex - 1 )
							if ( models[c]->data[index + indexdown[0]] >= 0 && models[c]->data[index + indexdown[0]] < 255 )
							{
								doit = true;
							}
						if ( doit )
						{
							// add the last two vertices to the previous quad
							buildingquad = false;

							node_t* currentNode = quads.last;
							quad1 = (polyquad_t*)currentNode->element;
							quad1->vertex[1].x = x - model->sizex / 2.f + 1;
							quad1->vertex[1].y = y - model->sizey / 2.f;
							quad1->vertex[1].z = z - model->sizez / 2.f - 1;
							quad1->vertex[2].x = x - model->sizex / 2.f + 1;
							quad1->vertex[2].y = y - model->sizey / 2.f;
							quad1->vertex[2].z = z - model->sizez / 2.f;

							// optimize quad
							node_t* node;
							for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
							{
								quad2 = (polyquad_t*)node->element;
								if ( quad1->side == quad2->side )
								{
									if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
									{
										if ( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z )
										{
											if ( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z )
											{
												quad2->vertex[2].z++;
												quad2->vertex[3].z++;
												list_RemoveNode(currentNode);
												numquads--;
												polymodels[c].numfaces -= 2;
												break;
											}
										}
									}
								}
							}
						}
					}
					if ( newcolor != oldcolor || !buildingquad )
					{
						if ( newcolor != 255 )
						{
							bool doit = false;
							if ( x == models[c]->sizex - 1 )
							{
								doit = true;
							}
							else if ( models[c]->data[index + indexdown[0]] == 255 )
							{
								doit = true;
							}
							if ( doit )
							{
								// start building a new quad
								buildingquad = true;
								numquads++;
								polymodels[c].numfaces += 2;

								quad1 = (polyquad_t*) calloc(1, sizeof(polyquad_t));
								quad1->side = 0;
								quad1->vertex[0].x = x - model->sizex / 2.f + 1;
								quad1->vertex[0].y = y - model->sizey / 2.f;
								quad1->vertex[0].z = z - model->sizez / 2.f - 1;
								quad1->vertex[3].x = x - model->sizex / 2.f + 1;
								quad1->vertex[3].y = y - model->sizey / 2.f;
								quad1->vertex[3].z = z - model->sizez / 2.f;
								quad1->r = models[c]->palette[models[c]->data[index]][0];
								quad1->g = models[c]->palette[models[c]->data[index]][1];
								quad1->b = models[c]->palette[models[c]->data[index]][2];

								node_t* newNode = list_AddNodeLast(&quads);
								newNode->element = quad1;
								newNode->deconstructor = &defaultDeconstructor;
								newNode->size = sizeof(polyquad_t);
							}
						}
					}
					oldcolor = newcolor;
				}
				if ( buildingquad == true )
				{
					// add the last two vertices to the previous quad
					buildingquad = false;

					node_t* currentNode = quads.last;
					quad1 = (polyquad_t*)currentNode->element;
					quad1->vertex[1].x = x - model->sizex / 2.f + 1;
					quad1->vertex[1].y = y - model->sizey / 2.f;
					quad1->vertex[1].z = z - model->sizez / 2.f - 1;
					quad1->vertex[2].x = x - model->sizex / 2.f + 1;
					quad1->vertex[2].y = y - model->sizey / 2.f;
					quad1->vertex[2].z = z - model->sizez / 2.f;

					// optimize quad
					node_t* node;
					for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
					{
						quad2 = (polyquad_t*)node->element;
						if ( quad1->side == quad2->side )
						{
							if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
							{
								if ( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z )
								{
									if ( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z )
									{
										quad2->vertex[2].z++;
										quad2->vertex[3].z++;
										list_RemoveNode(currentNode);
										numquads--;
										polymodels[c].numfaces -= 2;
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		// find back faces
		for ( x = 0; x < models[c]->sizex; x++ )
		{
			for ( z = 0; z < models[c]->sizez; z++ )
			{
				oldcolor = 255;
				buildingquad = false;
				for ( y = 0; y < models[c]->sizey; y++ )
				{
					index = z + y * models[c]->sizez + x * models[c]->sizey * models[c]->sizez;
					newcolor = models[c]->data[index];
					if ( buildingquad == true )
					{
						bool doit = false;
						if ( newcolor != oldcolor )
						{
							doit = true;
						}
						else if ( x > 0 )
							if ( models[c]->data[index - indexdown[0]] >= 0 && models[c]->data[index - indexdown[0]] < 255 )
							{
								doit = true;
							}
						if ( doit )
						{
							// add the last two vertices to the previous quad
							buildingquad = false;

							node_t* currentNode = quads.last;
							quad1 = (polyquad_t*)currentNode->element;
							quad1->vertex[1].x = x - model->sizex / 2.f;
							quad1->vertex[1].y = y - model->sizey / 2.f;
							quad1->vertex[1].z = z - model->sizez / 2.f;
							quad1->vertex[2].x = x - model->sizex / 2.f;
							quad1->vertex[2].y = y - model->sizey / 2.f;
							quad1->vertex[2].z = z - model->sizez / 2.f - 1;

							// optimize quad
							node_t* node;
							for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
							{
								quad2 = (polyquad_t*)node->element;
								if ( quad1->side == quad2->side )
								{
									if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
									{
										if ( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z )
										{
											if ( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z )
											{
												quad2->vertex[0].z++;
												quad2->vertex[1].z++;
												list_RemoveNode(currentNode);
												numquads--;
												polymodels[c].numfaces -= 2;
												break;
											}
										}
									}
								}
							}
						}
					}
					if ( newcolor != oldcolor || !buildingquad )
					{
						if ( newcolor != 255 )
						{
							bool doit = false;
							if ( x == 0 )
							{
								doit = true;
							}
							else if ( models[c]->data[index - indexdown[0]] == 255 )
							{
								doit = true;
							}
							if ( doit )
							{
								// start building a new quad
								buildingquad = true;
								numquads++;
								polymodels[c].numfaces += 2;

								quad1 = (polyquad_t*) calloc(1, sizeof(polyquad_t));
								quad1->side = 1;
								quad1->vertex[0].x = x - model->sizex / 2.f;
								quad1->vertex[0].y = y - model->sizey / 2.f;
								quad1->vertex[0].z = z - model->sizez / 2.f;
								quad1->vertex[3].x = x - model->sizex / 2.f;
								quad1->vertex[3].y = y - model->sizey / 2.f;
								quad1->vertex[3].z = z - model->sizez / 2.f - 1;
								quad1->r = models[c]->palette[models[c]->data[index]][0];
								quad1->g = models[c]->palette[models[c]->data[index]][1];
								quad1->b = models[c]->palette[models[c]->data[index]][2];

								node_t* newNode = list_AddNodeLast(&quads);
								newNode->element = quad1;
								newNode->deconstructor = &defaultDeconstructor;
								newNode->size = sizeof(polyquad_t);
							}
						}
					}
					oldcolor = newcolor;
				}
				if ( buildingquad == true )
				{
					// add the last two vertices to the previous quad
					buildingquad = false;

					node_t* currentNode = quads.last;
					quad1 = (polyquad_t*)currentNode->element;
					quad1->vertex[1].x = x - model->sizex / 2.f;
					quad1->vertex[1].y = y - model->sizey / 2.f;
					quad1->vertex[1].z = z - model->sizez / 2.f;
					quad1->vertex[2].x = x - model->sizex / 2.f;
					quad1->vertex[2].y = y - model->sizey / 2.f;
					quad1->vertex[2].z = z - model->sizez / 2.f - 1;

					// optimize quad
					node_t* node;
					for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
					{
						quad2 = (polyquad_t*)node->element;
						if ( quad1->side == quad2->side )
						{
							if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
							{
								if ( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z )
								{
									if ( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z )
									{
										quad2->vertex[0].z++;
										quad2->vertex[1].z++;
										list_RemoveNode(currentNode);
										numquads--;
										polymodels[c].numfaces -= 2;
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		// find right faces
		for ( y = models[c]->sizey - 1; y >= 0; y-- )
		{
			for ( z = 0; z < models[c]->sizez; z++ )
			{
				oldcolor = 255;
				buildingquad = false;
				for ( x = 0; x < models[c]->sizex; x++ )
				{
					index = z + y * models[c]->sizez + x * models[c]->sizey * models[c]->sizez;
					newcolor = models[c]->data[index];
					if ( buildingquad == true )
					{
						bool doit = false;
						if ( newcolor != oldcolor )
						{
							doit = true;
						}
						else if ( y < models[c]->sizey - 1 )
							if ( models[c]->data[index + indexdown[1]] >= 0 && models[c]->data[index + indexdown[1]] < 255 )
							{
								doit = true;
							}
						if ( doit )
						{
							// add the last two vertices to the previous quad
							buildingquad = false;

							node_t* currentNode = quads.last;
							quad1 = (polyquad_t*) currentNode->element;
							quad1->vertex[1].x = x - model->sizex / 2.f;
							quad1->vertex[1].y = y - model->sizey / 2.f + 1;
							quad1->vertex[1].z = z - model->sizez / 2.f;
							quad1->vertex[2].x = x - model->sizex / 2.f;
							quad1->vertex[2].y = y - model->sizey / 2.f + 1;
							quad1->vertex[2].z = z - model->sizez / 2.f - 1;

							// optimize quad
							node_t* node;
							for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
							{
								quad2 = (polyquad_t*)node->element;
								if ( quad1->side == quad2->side )
								{
									if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
									{
										if ( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z )
										{
											if ( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z )
											{
												quad2->vertex[0].z++;
												quad2->vertex[1].z++;
												list_RemoveNode(currentNode);
												numquads--;
												polymodels[c].numfaces -= 2;
												break;
											}
										}
									}
								}
							}
						}
					}
					if ( newcolor != oldcolor || !buildingquad )
					{
						if ( newcolor != 255 )
						{
							bool doit = false;
							if ( y == models[c]->sizey - 1 )
							{
								doit = true;
							}
							else if ( models[c]->data[index + indexdown[1]] == 255 )
							{
								doit = true;
							}
							if ( doit )
							{
								// start building a new quad
								buildingquad = true;
								numquads++;
								polymodels[c].numfaces += 2;

								quad1 = (polyquad_t*) calloc(1, sizeof(polyquad_t));
								quad1->side = 2;
								quad1->vertex[0].x = x - model->sizex / 2.f;
								quad1->vertex[0].y = y - model->sizey / 2.f + 1;
								quad1->vertex[0].z = z - model->sizez / 2.f;
								quad1->vertex[3].x = x - model->sizex / 2.f;
								quad1->vertex[3].y = y - model->sizey / 2.f + 1;
								quad1->vertex[3].z = z - model->sizez / 2.f - 1;
								quad1->r = models[c]->palette[models[c]->data[index]][0];
								quad1->g = models[c]->palette[models[c]->data[index]][1];
								quad1->b = models[c]->palette[models[c]->data[index]][2];

								node_t* newNode = list_AddNodeLast(&quads);
								newNode->element = quad1;
								newNode->deconstructor = &defaultDeconstructor;
								newNode->size = sizeof(polyquad_t);
							}
						}
					}
					oldcolor = newcolor;
				}
				if ( buildingquad == true )
				{
					// add the last two vertices to the previous quad
					buildingquad = false;
					node_t* currentNode = quads.last;
					quad1 = (polyquad_t*) currentNode->element;
					quad1->vertex[1].x = x - model->sizex / 2.f;
					quad1->vertex[1].y = y - model->sizey / 2.f + 1;
					quad1->vertex[1].z = z - model->sizez / 2.f;
					quad1->vertex[2].x = x - model->sizex / 2.f;
					quad1->vertex[2].y = y - model->sizey / 2.f + 1;
					quad1->vertex[2].z = z - model->sizez / 2.f - 1;

					// optimize quad
					node_t* node;
					for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
					{
						quad2 = (polyquad_t*)node->element;
						if ( quad1->side == quad2->side )
						{
							if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
							{
								if ( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z )
								{
									if ( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z )
									{
										quad2->vertex[0].z++;
										quad2->vertex[1].z++;
										list_RemoveNode(currentNode);
										numquads--;
										polymodels[c].numfaces -= 2;
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		// find left faces
		for ( y = 0; y < models[c]->sizey; y++ )
		{
			for ( z = 0; z < models[c]->sizez; z++ )
			{
				oldcolor = 255;
				buildingquad = false;
				for ( x = 0; x < models[c]->sizex; x++ )
				{
					index = z + y * models[c]->sizez + x * models[c]->sizey * models[c]->sizez;
					newcolor = models[c]->data[index];
					if ( buildingquad == true )
					{
						bool doit = false;
						if ( newcolor != oldcolor )
						{
							doit = true;
						}
						else if ( y > 0 )
							if ( models[c]->data[index - indexdown[1]] >= 0 && models[c]->data[index - indexdown[1]] < 255 )
							{
								doit = true;
							}
						if ( doit )
						{
							// add the last two vertices to the previous quad
							buildingquad = false;

							node_t* currentNode = quads.last;
							quad1 = (polyquad_t*) currentNode->element;
							quad1->vertex[1].x = x - model->sizex / 2.f;
							quad1->vertex[1].y = y - model->sizey / 2.f;
							quad1->vertex[1].z = z - model->sizez / 2.f - 1;
							quad1->vertex[2].x = x - model->sizex / 2.f;
							quad1->vertex[2].y = y - model->sizey / 2.f;
							quad1->vertex[2].z = z - model->sizez / 2.f;

							// optimize quad
							node_t* node;
							for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
							{
								quad2 = (polyquad_t*)node->element;
								if ( quad1->side == quad2->side )
								{
									if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
									{
										if ( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z )
										{
											if ( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z )
											{
												quad2->vertex[2].z++;
												quad2->vertex[3].z++;
												list_RemoveNode(currentNode);
												numquads--;
												polymodels[c].numfaces -= 2;
												break;
											}
										}
									}
								}
							}
						}
					}
					if ( newcolor != oldcolor || !buildingquad )
					{
						if ( newcolor != 255 )
						{
							bool doit = false;
							if ( y == 0 )
							{
								doit = true;
							}
							else if ( models[c]->data[index - indexdown[1]] == 255 )
							{
								doit = true;
							}
							if ( doit )
							{
								// start building a new quad
								buildingquad = true;
								numquads++;
								polymodels[c].numfaces += 2;

								quad1 = (polyquad_t*) calloc(1, sizeof(polyquad_t));
								quad1->side = 3;
								quad1->vertex[0].x = x - model->sizex / 2.f;
								quad1->vertex[0].y = y - model->sizey / 2.f;
								quad1->vertex[0].z = z - model->sizez / 2.f - 1;
								quad1->vertex[3].x = x - model->sizex / 2.f;
								quad1->vertex[3].y = y - model->sizey / 2.f;
								quad1->vertex[3].z = z - model->sizez / 2.f;
								quad1->r = models[c]->palette[models[c]->data[index]][0];
								quad1->g = models[c]->palette[models[c]->data[index]][1];
								quad1->b = models[c]->palette[models[c]->data[index]][2];

								node_t* newNode = list_AddNodeLast(&quads);
								newNode->element = quad1;
								newNode->deconstructor = &defaultDeconstructor;
								newNode->size = sizeof(polyquad_t);
							}
						}
					}
					oldcolor = newcolor;
				}
				if ( buildingquad == true )
				{
					// add the last two vertices to the previous quad
					buildingquad = false;
					node_t* currentNode = quads.last;
					quad1 = (polyquad_t*) currentNode->element;
					quad1->vertex[1].x = x - model->sizex / 2.f;
					quad1->vertex[1].y = y - model->sizey / 2.f;
					quad1->vertex[1].z = z - model->sizez / 2.f - 1;
					quad1->vertex[2].x = x - model->sizex / 2.f;
					quad1->vertex[2].y = y - model->sizey / 2.f;
					quad1->vertex[2].z = z - model->sizez / 2.f;

					// optimize quad
					node_t* node;
					for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
					{
						quad2 = (polyquad_t*)node->element;
						if ( quad1->side == quad2->side )
						{
							if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
							{
								if ( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z )
								{
									if ( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z )
									{
										quad2->vertex[2].z++;
										quad2->vertex[3].z++;
										list_RemoveNode(currentNode);
										numquads--;
										polymodels[c].numfaces -= 2;
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		// find bottom faces
		for ( z = models[c]->sizez - 1; z >= 0; z-- )
		{
			for ( y = 0; y < models[c]->sizey; y++ )
			{
				oldcolor = 255;
				buildingquad = false;
				for ( x = 0; x < models[c]->sizex; x++ )
				{
					index = z + y * models[c]->sizez + x * models[c]->sizey * models[c]->sizez;
					newcolor = models[c]->data[index];
					if ( buildingquad == true )
					{
						bool doit = false;
						if ( newcolor != oldcolor )
						{
							doit = true;
						}
						else if ( z < models[c]->sizez - 1 )
							if ( models[c]->data[index + indexdown[2]] >= 0 && models[c]->data[index + indexdown[2]] < 255 )
							{
								doit = true;
							}
						if ( doit )
						{
							// add the last two vertices to the previous quad
							buildingquad = false;

							node_t* currentNode = quads.last;
							quad1 = (polyquad_t*) currentNode->element;
							quad1->vertex[1].x = x - model->sizex / 2.f;
							quad1->vertex[1].y = y - model->sizey / 2.f;
							quad1->vertex[1].z = z - model->sizez / 2.f;
							quad1->vertex[2].x = x - model->sizex / 2.f;
							quad1->vertex[2].y = y - model->sizey / 2.f + 1;
							quad1->vertex[2].z = z - model->sizez / 2.f;

							// optimize quad
							node_t* node;
							for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
							{
								quad2 = (polyquad_t*)node->element;
								if ( quad1->side == quad2->side )
								{
									if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
									{
										if ( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z )
										{
											if ( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z )
											{
												quad2->vertex[2].y++;
												quad2->vertex[3].y++;
												list_RemoveNode(currentNode);
												numquads--;
												polymodels[c].numfaces -= 2;
												break;
											}
										}
									}
								}
							}
						}
					}
					if ( newcolor != oldcolor || !buildingquad )
					{
						if ( newcolor != 255 )
						{
							bool doit = false;
							if ( z == models[c]->sizez - 1 )
							{
								doit = true;
							}
							else if ( models[c]->data[index + indexdown[2]] == 255 )
							{
								doit = true;
							}
							if ( doit )
							{
								// start building a new quad
								buildingquad = true;
								numquads++;
								polymodels[c].numfaces += 2;

								quad1 = (polyquad_t*) calloc(1, sizeof(polyquad_t));
								quad1->side = 4;
								quad1->vertex[0].x = x - model->sizex / 2.f;
								quad1->vertex[0].y = y - model->sizey / 2.f;
								quad1->vertex[0].z = z - model->sizez / 2.f;
								quad1->vertex[3].x = x - model->sizex / 2.f;
								quad1->vertex[3].y = y - model->sizey / 2.f + 1;
								quad1->vertex[3].z = z - model->sizez / 2.f;
								quad1->r = models[c]->palette[models[c]->data[index]][0];
								quad1->g = models[c]->palette[models[c]->data[index]][1];
								quad1->b = models[c]->palette[models[c]->data[index]][2];

								node_t* newNode = list_AddNodeLast(&quads);
								newNode->element = quad1;
								newNode->deconstructor = &defaultDeconstructor;
								newNode->size = sizeof(polyquad_t);
							}
						}
					}
					oldcolor = newcolor;
				}
				if ( buildingquad == true )
				{
					// add the last two vertices to the previous quad
					buildingquad = false;

					node_t* currentNode = quads.last;
					quad1 = (polyquad_t*) currentNode->element;
					quad1->vertex[1].x = x - model->sizex / 2.f;
					quad1->vertex[1].y = y - model->sizey / 2.f;
					quad1->vertex[1].z = z - model->sizez / 2.f;
					quad1->vertex[2].x = x - model->sizex / 2.f;
					quad1->vertex[2].y = y - model->sizey / 2.f + 1;
					quad1->vertex[2].z = z - model->sizez / 2.f;

					// optimize quad
					node_t* node;
					for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
					{
						quad2 = (polyquad_t*)node->element;
						if ( quad1->side == quad2->side )
						{
							if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
							{
								if ( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z )
								{
									if ( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z )
									{
										quad2->vertex[2].y++;
										quad2->vertex[3].y++;
										list_RemoveNode(currentNode);
										numquads--;
										polymodels[c].numfaces -= 2;
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		// find top faces
		for ( z = 0; z < models[c]->sizez; z++ )
		{
			for ( y = 0; y < models[c]->sizey; y++ )
			{
				oldcolor = 255;
				buildingquad = false;
				for ( x = 0; x < models[c]->sizex; x++ )
				{
					index = z + y * models[c]->sizez + x * models[c]->sizey * models[c]->sizez;
					newcolor = models[c]->data[index];
					if ( buildingquad == true )
					{
						bool doit = false;
						if ( newcolor != oldcolor )
						{
							doit = true;
						}
						else if ( z > 0 )
							if ( models[c]->data[index - indexdown[2]] >= 0 && models[c]->data[index - indexdown[2]] < 255 )
							{
								doit = true;
							}
						if ( doit )
						{
							// add the last two vertices to the previous quad
							buildingquad = false;

							node_t* currentNode = quads.last;
							quad1 = (polyquad_t*) currentNode->element;
							quad1->vertex[1].x = x - model->sizex / 2.f;
							quad1->vertex[1].y = y - model->sizey / 2.f + 1;
							quad1->vertex[1].z = z - model->sizez / 2.f - 1;
							quad1->vertex[2].x = x - model->sizex / 2.f;
							quad1->vertex[2].y = y - model->sizey / 2.f;
							quad1->vertex[2].z = z - model->sizez / 2.f - 1;

							// optimize quad
							node_t* node;
							for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
							{
								quad2 = (polyquad_t*)node->element;
								if ( quad1->side == quad2->side )
								{
									if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
									{
										if ( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z )
										{
											if ( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z )
											{
												quad2->vertex[0].y++;
												quad2->vertex[1].y++;
												list_RemoveNode(currentNode);
												numquads--;
												polymodels[c].numfaces -= 2;
												break;
											}
										}
									}
								}
							}
						}
					}
					if ( newcolor != oldcolor || !buildingquad )
					{
						if ( newcolor != 255 )
						{
							bool doit = false;
							if ( z == 0 )
							{
								doit = true;
							}
							else if ( models[c]->data[index - indexdown[2]] == 255 )
							{
								doit = true;
							}
							if ( doit )
							{
								// start building a new quad
								buildingquad = true;
								numquads++;
								polymodels[c].numfaces += 2;

								quad1 = (polyquad_t*) calloc(1, sizeof(polyquad_t));
								quad1->side = 5;
								quad1->vertex[0].x = x - model->sizex / 2.f;
								quad1->vertex[0].y = y - model->sizey / 2.f + 1;
								quad1->vertex[0].z = z - model->sizez / 2.f - 1;
								quad1->vertex[3].x = x - model->sizex / 2.f;
								quad1->vertex[3].y = y - model->sizey / 2.f;
								quad1->vertex[3].z = z - model->sizez / 2.f - 1;
								quad1->r = models[c]->palette[models[c]->data[index]][0];
								quad1->g = models[c]->palette[models[c]->data[index]][1];
								quad1->b = models[c]->palette[models[c]->data[index]][2];

								node_t* newNode = list_AddNodeLast(&quads);
								newNode->element = quad1;
								newNode->deconstructor = &defaultDeconstructor;
								newNode->size = sizeof(polyquad_t);
							}
						}
					}
					oldcolor = newcolor;
				}
				if ( buildingquad == true )
				{
					// add the last two vertices to the previous quad
					buildingquad = false;

					node_t* currentNode = quads.last;
					quad1 = (polyquad_t*) currentNode->element;
					quad1->vertex[1].x = x - model->sizex / 2.f;
					quad1->vertex[1].y = y - model->sizey / 2.f + 1;
					quad1->vertex[1].z = z - model->sizez / 2.f - 1;
					quad1->vertex[2].x = x - model->sizex / 2.f;
					quad1->vertex[2].y = y - model->sizey / 2.f;
					quad1->vertex[2].z = z - model->sizez / 2.f - 1;

					// optimize quad
					node_t* node;
					for ( i = 0, node = quads.first; i < numquads - 1; i++, node = node->next )
					{
						quad2 = (polyquad_t*)node->element;
						if ( quad1->side == quad2->side )
						{
							if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
							{
								if ( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z )
								{
									if ( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z )
									{
										quad2->vertex[0].y++;
										quad2->vertex[1].y++;
										list_RemoveNode(currentNode);
										numquads--;
										polymodels[c].numfaces -= 2;
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		// translate quads into triangles
		polymodels[c].faces = (polytriangle_t*) malloc(sizeof(polytriangle_t) * polymodels[c].numfaces);
		for ( i = 0; i < polymodels[c].numfaces; i++ )
		{
			node_t* node = list_Node(&quads, i / 2);
			polyquad_t* quad = (polyquad_t*)node->element;
			polymodels[c].faces[i].r = quad->r;
			polymodels[c].faces[i].g = quad->g;
			polymodels[c].faces[i].b = quad->b;
			if ( i % 2 )
			{
				polymodels[c].faces[i].vertex[0] = quad->vertex[0];
				polymodels[c].faces[i].vertex[1] = quad->vertex[1];
				polymodels[c].faces[i].vertex[2] = quad->vertex[2];
			}
			else
			{
				polymodels[c].faces[i].vertex[0] = quad->vertex[0];
				polymodels[c].faces[i].vertex[1] = quad->vertex[2];
				polymodels[c].faces[i].vertex[2] = quad->vertex[3];
			}
		}

		// free up quads for the next model
		list_FreeAll(&quads);
	}
#ifndef NINTENDO
    std::string cache_path = std::string(outputdir) + "models.cache";
	if (useModelCache && (model_cache = openDataFile(cache_path.c_str(), "wb")))
	{
		char modelCacheHeader[32] = "BARONY";
		strcat(modelCacheHeader, VERSION);
		model_cache->write(&modelCacheHeader, sizeof(char), strlen(modelCacheHeader));
		for (size_t model_index = 0; model_index < nummodels; model_index++)
		{
			polymodel_t *cur = &polymodels[model_index];
			model_cache->write(&cur->numfaces, sizeof(cur->numfaces), 1);
			model_cache->write(cur->faces, sizeof(polytriangle_t), cur->numfaces);
		}
		FileIO::close(model_cache);
	}
#endif
}

/*-------------------------------------------------------------------------------

	generateVBOs

	generates VBOs/VAs from polymodel data

-------------------------------------------------------------------------------*/

void generateVBOs(int start, int end)
{
	int count = end - start;

	std::unique_ptr<GLuint[]> vas(new GLuint[count]);
	SDL_glGenVertexArrays(count, vas.get());

	std::unique_ptr<GLuint[]> vbos(new GLuint[count]);
	SDL_glGenBuffers(count, vbos.get());

	std::unique_ptr<GLuint[]> color_buffers(new GLuint[count]);
	SDL_glGenBuffers(count, color_buffers.get());

	std::unique_ptr<GLuint[]> color_shifted_buffers(new GLuint[count]);
	SDL_glGenBuffers(count, color_shifted_buffers.get());

	std::unique_ptr<GLuint[]> grayscale_color_buffers(new GLuint[count]);
	SDL_glGenBuffers(count, grayscale_color_buffers.get());

	std::unique_ptr<GLuint[]> grayscale_color_shifted_buffers(new GLuint[count]);
	SDL_glGenBuffers(count, grayscale_color_shifted_buffers.get());

	for ( int c = start; c < end; ++c )
	{
		polymodel_t *model = &polymodels[c];
		std::unique_ptr<GLfloat[]> points(new GLfloat[9 * model->numfaces]);
		std::unique_ptr<GLfloat[]> colors(new GLfloat[9 * model->numfaces]);
		std::unique_ptr<GLfloat[]> colors_shifted(new GLfloat[9 * model->numfaces]);
		std::unique_ptr<GLfloat[]> grayscale_colors(new GLfloat[9 * model->numfaces]);
		std::unique_ptr<GLfloat[]> grayscale_colors_shifted(new GLfloat[9 * model->numfaces]);
		for ( int i = 0; i < model->numfaces; i++ )
		{
			const polytriangle_t *face = &model->faces[i];
			for (int vert_index = 0; vert_index < 3; vert_index++)
			{
				int data_index = i * 9 + vert_index * 3;
				const vertex_t *vert = &face->vertex[vert_index];

				points[data_index] = vert->x;
				points[data_index + 1] = -vert->z;
				points[data_index + 2] = vert->y;

				colors[data_index] = face->r / 255.f;
				colors[data_index + 1] = face->g / 255.f;
				colors[data_index + 2] = face->b / 255.f;

				colors_shifted[data_index] = face->b / 255.f;
				colors_shifted[data_index + 1] = face->r / 255.f;
				colors_shifted[data_index + 2] = face->g / 255.f;

				real_t grayscaleFactor = (face->r + face->g + face->b) / 3.0;
				grayscale_colors[data_index] = grayscaleFactor / 255.f;
				grayscale_colors[data_index + 1] = grayscaleFactor / 255.f;
				grayscale_colors[data_index + 2] = grayscaleFactor / 255.f;

				grayscale_colors_shifted[data_index] = grayscaleFactor / 255.f;
				grayscale_colors_shifted[data_index + 1] = grayscaleFactor / 255.f;
				grayscale_colors_shifted[data_index + 2] = grayscaleFactor / 255.f;
			}
		}
		model->va = vas[c - start];
		model->vbo = vbos[c - start];
		model->colors = color_buffers[c - start];
		model->colors_shifted = color_shifted_buffers[c - start];
		model->grayscale_colors = grayscale_color_buffers[c - start];
		model->grayscale_colors_shifted = grayscale_color_shifted_buffers[c - start];
		SDL_glBindVertexArray(model->va);

		// vertex data
		// Well, the generic vertex array are not used, so disabled (making it run on any OpenGL 1.5 hardware)
		SDL_glBindBuffer(GL_ARRAY_BUFFER, model->vbo);
		SDL_glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 9 * model->numfaces, points.get(), GL_STATIC_DRAW);

		// color data
		SDL_glBindBuffer(GL_ARRAY_BUFFER, model->colors);
		SDL_glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 9 * model->numfaces, colors.get(), GL_STATIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW

		// shifted color data
		SDL_glBindBuffer(GL_ARRAY_BUFFER, model->colors_shifted);
		SDL_glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 9 * model->numfaces, colors_shifted.get(), GL_STATIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW

		// grayscale color data
		SDL_glBindBuffer(GL_ARRAY_BUFFER, model->grayscale_colors);
		SDL_glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 9 * model->numfaces, grayscale_colors.get(), GL_STATIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW

		// grayscale shifted color data
		SDL_glBindBuffer(GL_ARRAY_BUFFER, model->grayscale_colors_shifted);
		SDL_glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 9 * model->numfaces, grayscale_colors_shifted.get(), GL_STATIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW
	}
}

/*-------------------------------------------------------------------------------

	deinitApp

	frees all memory consumed by the application and terminates the engine

-------------------------------------------------------------------------------*/
int deinitApp()
{
	Uint32 c;
#ifdef USE_OPENAL
	closeOPENAL();
#endif
	// close engine
	printlog("closing engine...\n");

	printlog("freeing engine resources...\n");
	list_FreeAll(&button_l);
	list_FreeAll(&entitiesdeleted);
	if ( fancyWindow_bmp )
	{
		SDL_FreeSurface(fancyWindow_bmp);
	}
	if ( font8x8_bmp )
	{
		SDL_FreeSurface(font8x8_bmp);
	}
	if ( font12x12_bmp )
	{
		SDL_FreeSurface(font12x12_bmp);
	}
	if ( font16x16_bmp )
	{
		SDL_FreeSurface(font16x16_bmp);
	}
	if ( ttf8 )
	{
		TTF_CloseFont(ttf8);
	}
	if ( ttf12 )
	{
		TTF_CloseFont(ttf12);
	}
	if ( ttf16 )
	{
		TTF_CloseFont(ttf16);
	}

	printlog("freeing ui resources...\n");
	Text::dumpCache();
	Image::dumpCache();
	Font::dumpCache();
	Frame::guiDestroy();

	printlog("freeing map data...\n");
	if ( map.entities != NULL )
	{
		list_FreeAll(map.entities);
		free(map.entities);
	}
	if ( map.creatures != nullptr)
	{
		list_FreeAll(map.creatures); //TODO: Need to call this? Entities are only pointed to by the thing, not owned.
		delete map.creatures;
	}
	if ( map.worldUI != nullptr )
	{
		list_FreeAll(map.worldUI);
		delete map.worldUI;
	}
	list_FreeAll(&light_l);
	if ( map.tiles != nullptr )
	{
		free(map.tiles);
	}
	if ( map.vismap != nullptr )
	{
	    free(map.vismap);
	}
	if ( lightmap != nullptr )
	{
		free(lightmap);
	}
	if ( lightmapSmoothed )
	{
		free(lightmapSmoothed);
	}

	for ( c = 0; c < HASH_SIZE; c++ )
	{
		list_FreeAll(&ttfTextHash[c]);
	}

	// free textures
	printlog("freeing textures...\n");
	if ( tiles != NULL )
	{
		for ( c = 0; c < numtiles; c++ )
		{
			if ( tiles[c] )
			{
				SDL_FreeSurface(tiles[c]);
			}
		}
		free(tiles);
	}
	if ( animatedtiles )
	{
		free(animatedtiles);
		animatedtiles = nullptr;
	}
	if ( lavatiles )
	{
		free(lavatiles);
		lavatiles = nullptr;
	}
	if ( swimmingtiles )
	{
		free(swimmingtiles);
		swimmingtiles = nullptr;
	}

	// free sprites
	printlog("freeing sprites...\n");
	if ( sprites != NULL )
	{
		for ( c = 0; c < numsprites; c++ )
		{
			if ( sprites[c] )
			{
				SDL_FreeSurface(sprites[c]);
			}
		}
		free(sprites);
	}

	// free achievement images
	for (auto& item : achievementImages) 
	{
		SDL_FreeSurface(item.second);
	}
	achievementImages.clear();

	// free models
	printlog("freeing models...\n");
	if ( models != NULL )
	{
		for ( c = 0; c < nummodels; c++ )
		{
			if ( models[c] != NULL )
			{
				if ( models[c]->data )
				{
					free(models[c]->data);
				}
				free(models[c]);
			}
		}
		free(models);
	}
	if ( polymodels != NULL )
	{
		for ( c = 0; c < nummodels; c++ )
		{
			if ( polymodels[c].faces )
			{
				free(polymodels[c].faces);
			}
		}
		if ( !disablevbos )
		{
			for ( c = 0; c < nummodels; c++ )
			{
				if ( polymodels[c].vbo )
				{
					SDL_glDeleteBuffers(1, &polymodels[c].vbo);
				}
				if ( polymodels[c].colors )
				{
					SDL_glDeleteBuffers(1, &polymodels[c].colors);
				}
				if ( polymodels[c].va )
				{
					SDL_glDeleteVertexArrays(1, &polymodels[c].va);
				}
				if ( polymodels[c].colors_shifted )
				{
					SDL_glDeleteBuffers(1, &polymodels[c].colors_shifted);
				}
				if ( polymodels[c].grayscale_colors )
				{
					SDL_glDeleteBuffers(1, &polymodels[c].grayscale_colors);
				}
				if ( polymodels[c].grayscale_colors_shifted )
				{
					SDL_glDeleteBuffers(1, &polymodels[c].grayscale_colors_shifted);
				}
			}
		}
		free(polymodels);
	}

	freeSoundResources();

	// delete opengl buffers
	if ( allsurfaces != NULL )
	{
		free(allsurfaces);
	}
	if ( texid != NULL )
	{
		glDeleteTextures(MAXTEXTURES, texid);
		free(texid);
	}

	// delete opengl buffers
	/*SDL_glDeleteBuffers(MAXBUFFERS,vboid);
	if( vboid != NULL )
		free(vboid);
	SDL_glDeleteVertexArrays(MAXBUFFERS,vaoid);
	if( vaoid != NULL )
		free(vaoid);*/

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
	exitSoundEngine();
	if ( screen )
	{
		SDL_DestroyWindow(screen);
		screen = NULL;
	}
	if ( renderer )
	{
#ifdef APPLE
		SDL_DestroyRenderer(renderer);
#else
		SDL_GL_DeleteContext(renderer);
#endif
		renderer = NULL;
	}
	if ( mainsurface )
	{
		SDL_FreeSurface(mainsurface);
		mainsurface = NULL;
	}
	TTF_Quit();
	SDL_Quit();

	// free video and input buffers
	if ( zbuffer != NULL )
	{
		free(zbuffer);
	}
	if ( clickmap != NULL )
	{
		free(clickmap);
	}

	// shutdown steamworks
#ifdef STEAMWORKS
	if ( steam_init )
	{
		printlog("storing user stats to Steam...\n");
		SteamUserStats()->StoreStats();
		if ( g_SteamLeaderboards )
		{
			delete g_SteamLeaderboards;
		}
		if ( g_SteamWorkshop )
		{
			delete g_SteamWorkshop;
		}
		if ( g_SteamStatistics )
		{
			delete g_SteamStatistics;
		}
		SteamAPI_Shutdown();
	}
#endif


#ifndef NINTENDO
	int numLogFilesToKeepInArchive = 30;
	// archive logfiles.
	char lognamewithTimestamp[128];
	std::time_t timeNow = std::time(nullptr);
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

	// free currently loaded language if any
	freeLanguages();
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

#ifdef PANDORA
void GO_InitFBO()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	if(fbo_fbo) {
		glDeleteFramebuffers(1, &fbo_fbo); fbo_fbo = 0;
		glDeleteRenderbuffers(1, &fbo_ren); fbo_ren = 0;
		if(fbo_trn) {
			glDeleteRenderbuffers(1, &fbo_trn); fbo_trn = 0;
		}
		if(fbo_tex) {
			glDeleteTextures(1, &fbo_tex); fbo_tex = 0;
		}
	}

	// Pandora, create the FBO!
	bool small_fbo=((xres==800) && (yres==480));
	glGenFramebuffers(1, &fbo_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_fbo);
	if(small_fbo) {
		glGenRenderbuffers(1, &fbo_trn);
		glBindRenderbuffer(GL_RENDERBUFFER, fbo_trn);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, 1024, 512);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, fbo_trn);
	} else {
		glGenTextures(1, &fbo_tex);
		glBindTexture(GL_TEXTURE_2D, fbo_tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_tex, 0);
	}
	glGenRenderbuffers(1, &fbo_ren);
	glBindRenderbuffer(GL_RENDERBUFFER, fbo_ren);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, 1024, (small_fbo)?512:1024);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fbo_ren);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	if(!small_fbo)
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_fbo);

}
#endif

/*-------------------------------------------------------------------------------

	initVideo

	Sets the SDL/openGL context using global video variables

-------------------------------------------------------------------------------*/

bool initVideo()
{
#ifdef NINTENDO
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#else
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 1/*3*/ ); //Why GL 3.0? using only fixed pipeline stuff here
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );
	SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
	//SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
	//SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
	//SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 4 );
#endif

	printlog("setting display mode to %dx%d...\n", xres, yres);
	Uint32 flags = 0;
#ifdef PANDORA
	fullscreen = true;
#endif
#ifdef NINTENDO
	flags = SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_OPENGL;
#else
	if ( fullscreen )
	{
		flags |= SDL_WINDOW_FULLSCREEN;
	}
	if ( borderless )
	{
		flags |= SDL_WINDOW_BORDERLESS;
	}
	if ( !game )
	{
		flags |= SDL_WINDOW_RESIZABLE;
	}
	if ( !softwaremode )
	{
		flags |= SDL_WINDOW_OPENGL;
	}
#endif
#ifdef APPLE
	if ( fullscreen )
	{
		flags |= SDL_WINDOW_BORDERLESS;
	}
	SDL_DestroyWindow(screen);
	screen = NULL;
#endif
#ifdef PANDORA
	int screen_width = 800;
#else
	int screen_width = xres;
#endif
	if ( !screen )
	{
#ifdef PANDORA
		if ((screen = SDL_CreateWindow( window_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, 480, flags )) == NULL)
#else
		if ((screen = SDL_CreateWindow( window_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, yres, flags )) == NULL)
#endif
		{
			printlog("failed to set video mode.\n");
			return false;
		}
	}
	else
	{
#ifdef PANDORA
		SDL_SetWindowSize(screen, screen_width, 480);
#else
		SDL_SetWindowSize(screen, screen_width, yres);
#endif
#ifndef NINTENDO
		if ( fullscreen )
		{
			SDL_SetWindowFullscreen(screen, SDL_WINDOW_FULLSCREEN);
		}
		else
		{
			SDL_SetWindowFullscreen(screen, 0);
		}

		if ( borderless )
		{
			SDL_SetWindowBordered(screen, SDL_bool::SDL_FALSE);
		}
		else
		{
			SDL_SetWindowBordered(screen, SDL_bool::SDL_TRUE);
		}
		SDL_SetWindowPosition(screen, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
#endif // !NINTENDO
	}

	if ( !renderer )
	{
#ifdef APPLE
		if ((renderer = SDL_CreateRenderer(screen, -1, 0)) == NULL)
		{
#else
		if ((renderer = SDL_GL_CreateContext(screen)) == NULL)
		{
#endif
			printlog("failed to create SDL renderer. Reason: \"%s\"\n", SDL_GetError());
			printlog("You may need to update your video drivers.\n");
			return false;
		}
	}
#ifndef APPLE
	SDL_GL_MakeCurrent(screen, renderer);
#endif
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	Uint32 rmask = 0xff000000;
	Uint32 gmask = 0x00ff0000;
	Uint32 bmask = 0x0000ff00;
	Uint32 amask = 0x000000ff;
#else
	Uint32 rmask = 0x000000ff;
	Uint32 gmask = 0x0000ff00;
	Uint32 bmask = 0x00ff0000;
	Uint32 amask = 0xff000000;
#endif
	if ((mainsurface = SDL_CreateRGBSurface(0, xres, yres, 32, rmask, gmask, bmask, amask)) == NULL)
	{
		printlog("failed to create main window surface.\n");
		return false;
	}
	if ( !softwaremode )
	{
#ifdef PANDORA
		GO_InitFBO();
#endif
#ifdef NINTENDO
		initNxGL();
#endif
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//glEnableClientState(GL_VERTEX_ARRAY);
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		glClearColor( 0, 0, 0, 0 );
	}

	if ( verticalSync )
	{
		SDL_GL_SetSwapInterval(1);
	}
	else
	{
		SDL_GL_SetSwapInterval(0);
	}
#ifndef NINTENDO
	if ( SDL_SetWindowBrightness(screen, vidgamma) < 0 )
	{
		printlog("warning: failed to change gamma setting:\n%s\n", SDL_GetError());
		return true;
	}
#endif
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
	if (new_xres) {
		xres = new_xres;
	}
	if (new_yres) {
		yres = new_yres;
	}
	printlog("changing video mode (%d x %d).\n", xres, yres);
#ifdef PANDORA
	GO_InitFBO();
#else
	int c;

	// delete old texture names (they're going away anyway)
	glDeleteTextures(MAXTEXTURES, texid);

	// destroy gui fbo
#ifndef EDITOR
	Frame::fboDestroy();
#endif

	// delete vertex data
	if ( !disablevbos )
	{
		for ( c = 0; c < nummodels; c++ )
		{
			SDL_glDeleteBuffers(1, &polymodels[c].vbo);
			SDL_glDeleteBuffers(1, &polymodels[c].colors);
			SDL_glDeleteVertexArrays(1, &polymodels[c].va);
			SDL_glDeleteBuffers(1, &polymodels[c].colors_shifted);
			SDL_glDeleteBuffers(1, &polymodels[c].grayscale_colors);
			SDL_glDeleteBuffers(1, &polymodels[c].grayscale_colors_shifted);
		}
	}

	/*if( screen ) {
		SDL_DestroyWindow(screen);
		screen = NULL;
	}*/
	if ( renderer )
	{
#ifdef APPLE
		SDL_DestroyRenderer(renderer);
#else
		SDL_GL_DeleteContext(renderer);
#endif
		renderer = NULL;
	}
	if ( mainsurface )
	{
		SDL_FreeSurface(mainsurface);
		mainsurface = NULL;
	}

	// set video mode
	int result = initVideo();
	if ( !result )
	{
		xres = 1280;
		yres = 720;
		fullscreen = 0;
		borderless = false;
		printlog("defaulting to safe video mode...\n");
		if ( !initVideo() )
		{
			return false;
		}
	}

	// now reload all textures
	glGenTextures(MAXTEXTURES, texid);
	for ( c = 1; c < imgref; c++ )
	{
		glLoadTexture(allsurfaces[c], c);
	}

	// regenerate vbos
	if ( !disablevbos )
	{
		generateVBOs(0, nummodels);
	}

#ifndef EDITOR
	// dump ui resources, these are all no good too!
	Text::dumpCache();
	Image::dumpCache();
	Font::dumpCache();
#endif // !EDITOR

	// create new frame fbo
#ifndef EDITOR
	Frame::fboInit();
#endif

#endif

	if ( zbuffer != NULL )
	{
		free(zbuffer);
	}
	zbuffer = (real_t*) malloc(sizeof(real_t) * xres * yres);
	if ( clickmap != NULL )
	{
		free(clickmap);
	}
	clickmap = (Entity**) malloc(sizeof(Entity*)*xres * yres);

	// success
	return true;
}

/*-------------------------------------------------------------------------------

loadItemLists()

loads the global item whitelist/blacklists and level curve.

-------------------------------------------------------------------------------*/

bool loadItemLists()
{
	// open log file
	if ( !logfile )
	{
		openLogFile();
	}

	// compose filename
	//char filename[128] = "items/items_global.txt";
	std::string itemsTxtDirectory = PHYSFS_getRealDir("items/items_global.txt");
	itemsTxtDirectory.append(PHYSFS_getDirSeparator()).append("items/items_global.txt");
	// check if item list is valid
	if ( !dataPathExists(itemsTxtDirectory.c_str()) )
	{
		// file doesn't exist
		printlog("error: unable to locate global item list file: '%s'", itemsTxtDirectory.c_str());
		return false;
	}

	std::vector<std::string> itemLevels = getLinesFromDataFile(itemsTxtDirectory);
	std::string line;
	int itemIndex = 0;

	for ( std::vector<std::string>::const_iterator i = itemLevels.begin(); i != itemLevels.end(); ++i ) {
		// process i
		line = *i;
		if ( line[0] == '#' || line[0] == '\n' )
		{
			continue;
		}
		std::size_t found = line.find('#');
		if ( found != std::string::npos )
		{
			char tmp[128];
			std::string sub = line.substr(0, found);
			strncpy(tmp, sub.c_str(), sub.length());
			tmp[sub.length()] = '\0';
			//printlog("%s", tmp);
			items[itemIndex].level = atoi(tmp);
			++itemIndex;
		}
	}

	printlog("successfully loaded global item list '%s' \n", itemsTxtDirectory.c_str());
	/*for ( c = 0; c < NUMITEMS; ++c )
	{
		printlog("%s level: %d", items[c].name_identified, items[c].level);
	}*/
	return true;
}
