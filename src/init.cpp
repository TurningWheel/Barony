/*-------------------------------------------------------------------------------

	BARONY
	File: init.cpp
	Desc: contains program initialization code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "files.hpp"
#include "sound.hpp"
#include "prng.hpp"
#include "hash.hpp"
#include "init.hpp"
#include "net.hpp"
#include "editor.hpp"
#ifdef STEAMWORKS
#include <steam/steam_api.h>
#include "steam.hpp"
#endif
#include "player.hpp"
#include "items.hpp"
#include "cppfuncs.hpp"

#ifdef HAVE_FMOD
#include "fmod.h"
//#include <fmod_errors.h>
#endif

/*-------------------------------------------------------------------------------

	initApp

	initializes all the application variables and starts the engine

-------------------------------------------------------------------------------*/

#define LOADSTR1 language[741]
#define LOADSTR2 language[742]
#define LOADSTR3 language[743]
#define LOADSTR4 language[744]

#ifdef PANDORA
// Pandora FBO
GLuint fbo_fbo = 0;
GLuint fbo_tex = 0;
GLuint fbo_trn = 0;
GLuint fbo_ren = 0;
#endif

FILE* logfile = nullptr;
bool steam_init = false;

int initApp(char* title, int fullscreen)
{
	char name[128];
	FILE* fp;
	Uint32 x, c;

	// open log file
	if ( !logfile )
	{
		logfile = freopen("log.txt", "wb" /*or "wt"*/, stderr);
	}

	for (c = 0; c < NUM_JOY_STATUS; ++c)
	{
		joystatus[c] = 0;
	}
	for (c = 0; c < NUM_JOY_TRIGGER_STATUS; ++c)
	{
		joy_trigger_status[c] = 0;
	}

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
	map.entities = NULL;
	map.tiles = NULL;

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
#endif

	window_title = title;
	printlog("initializing SDL...\n");
	if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER ) == -1 )
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

#ifdef HAVE_FMOD
	printlog("initializing FMOD...\n");
	fmod_result = FMOD_System_Create(&fmod_system);
	if (FMODErrorCheck())
	{
		printlog("Failed to create FMOD.\n");
		no_sound = true;
	}
	if (!no_sound)
	{
		//FMOD_System_SetOutput(fmod_system, FMOD_OUTPUTTYPE_DSOUND);
		fmod_result = FMOD_System_Init(fmod_system, fmod_maxchannels, FMOD_INIT_NORMAL | FMOD_INIT_3D_RIGHTHANDED, 0);
		if (FMODErrorCheck())
		{
			printlog("Failed to initialize FMOD.\n");
			no_sound = true;
		}
		if (!no_sound)
		{
			fmod_result = FMOD_System_CreateChannelGroup(fmod_system, NULL, &sound_group);
			if (FMODErrorCheck())
			{
				printlog("Failed to create sound channel group.\n");
				no_sound = true;
			}
			if (!no_sound)
			{
				fmod_result = FMOD_System_CreateChannelGroup(fmod_system, NULL, &music_group);
				if (FMODErrorCheck())
				{
					printlog("Failed to create music channel group.\n");
					no_sound = true;
				}
			}
		}
	}
#elif defined HAVE_OPENAL
	if (!no_sound)
	{
		initOPENAL();
	}
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
		SDL_ShowCursor(SDL_FALSE);
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

	// print a loading message
	drawClearBuffers();
	int w, h;
	TTF_SizeUTF8(ttf16, LOADSTR1, &w, &h);
	ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, LOADSTR1);

	GO_SwapBuffers(screen);

	// load sprites
	printlog("loading sprites...\n");
	fp = openDataFile("images/sprites.txt", "r");
	for ( numsprites = 0; !feof(fp); numsprites++ )
	{
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
	}
	fclose(fp);
	if ( numsprites == 0 )
	{
		printlog("failed to identify any sprites in sprites.txt\n");
		return 6;
	}
	sprites = (SDL_Surface**) malloc(sizeof(SDL_Surface*)*numsprites);
	fp = openDataFile("images/sprites.txt", "r");
	for ( c = 0; !feof(fp); c++ )
	{
		fscanf(fp, "%s", name);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
		sprites[c] = loadImage(name);
		if ( sprites[c] == NULL )
		{
			printlog("warning: failed to load '%s' listed at line %d in sprites.txt\n", name, c + 1);
			if ( c == 0 )
			{
				printlog("sprite 0 cannot be NULL!\n");
				fclose(fp);
				return 7;
			}
		}
	}
	fclose(fp);

	// print a loading message
	drawClearBuffers();
	TTF_SizeUTF8(ttf16, LOADSTR2, &w, &h);
	ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, LOADSTR2);

	GO_SwapBuffers(screen);

	// load models
	printlog("loading models...\n");
	fp = openDataFile("models/models.txt", "r");
	for ( nummodels = 0; !feof(fp); nummodels++ )
	{
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
	}
	fclose(fp);
	if ( nummodels == 0 )
	{
		printlog("failed to identify any models in models.txt\n");
		return 11;
	}
	models = (voxel_t**) malloc(sizeof(voxel_t*)*nummodels);
	fp = openDataFile("models/models.txt", "r");
	for ( c = 0; !feof(fp); c++ )
	{
		fscanf(fp, "%s", name);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
		models[c] = loadVoxel(name);
		if ( models[c] == NULL )
		{
			printlog("warning: failed to load '%s' listed at line %d in models.txt\n", name, c + 1);
			if ( c == 0 )
			{
				printlog("model 0 cannot be NULL!\n");
				fclose(fp);
				return 12;
			}
		}
	}
	if ( !softwaremode )
	{
		generatePolyModels(0, nummodels, false);
	}
	fclose(fp);
	// print a loading message
	drawClearBuffers();
	TTF_SizeUTF8(ttf16, LOADSTR3, &w, &h);
	ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, LOADSTR3);

	GO_SwapBuffers(screen);

	// load tiles
	printlog("loading tiles...\n");
	fp = openDataFile("images/tiles.txt", "r");
	for ( numtiles = 0; !feof(fp); numtiles++ )
	{
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
	}
	fclose(fp);
	if ( numtiles == 0 )
	{
		printlog("failed to identify any tiles in tiles.txt\n");
		return 8;
	}
	tiles = (SDL_Surface**) malloc(sizeof(SDL_Surface*)*numtiles);
	animatedtiles = (bool*) malloc(sizeof(bool) * numtiles);
	lavatiles = (bool*) malloc(sizeof(bool) * numtiles);
	swimmingtiles = (bool*)malloc(sizeof(bool) * numtiles);
	fp = openDataFile("images/tiles.txt", "r");
	for ( c = 0; !feof(fp); c++ )
	{
		fscanf(fp, "%s", name);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
		tiles[c] = loadImage(name);
		animatedtiles[c] = false;
		lavatiles[c] = false;
		swimmingtiles[c] = false;
		if ( tiles[c] != NULL )
		{
			for (x = 0; x < strlen(name); x++)
			{
				if ( name[x] >= '0' && name[x] < '9' )
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
				fclose(fp);
				return 9;
			}
		}
	}
	fclose(fp);

	// print a loading message
	drawClearBuffers();
	TTF_SizeUTF8(ttf16, LOADSTR4, &w, &h);
	ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, LOADSTR4);

	GO_SwapBuffers(screen);

	// load sound effects
#ifdef HAVE_FMOD
	printlog("loading sounds...\n");
	fp = openDataFile("sound/sounds.txt", "r");
	for ( numsounds = 0; !feof(fp); numsounds++ )
	{
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
	}
	fclose(fp);
	if ( numsounds == 0 )
	{
		printlog("failed to identify any sounds in sounds.txt\n");
		return 10;
	}
	sounds = (FMOD_SOUND**) malloc(sizeof(FMOD_SOUND*)*numsounds);
	fp = openDataFile("sound/sounds.txt", "r");
	for ( c = 0; !feof(fp); c++ )
	{
		fscanf(fp, "%s", name);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
		//TODO: Might need to malloc the sounds[c]->sound
		fmod_result = FMOD_System_CreateSound(fmod_system, name, (FMOD_MODE)(FMOD_SOFTWARE | FMOD_3D), NULL, &sounds[c]);
		if (FMODErrorCheck())
		{
			printlog("warning: failed to load '%s' listed at line %d in sounds.txt\n", name, c + 1);
		}
		//TODO: set sound volume? Or otherwise handle sound volume.
	}
	fclose(fp);
	FMOD_ChannelGroup_SetVolume(sound_group, sfxvolume / 128.f);
	FMOD_System_Set3DSettings(fmod_system, 1.0, 2.0, 1.0);
#elif defined HAVE_OPENAL
	printlog("loading sounds...\n");
	fp = openDataFile("sound/sounds.txt", "r");
	for ( numsounds = 0; !feof(fp); numsounds++ )
	{
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
	}
	fclose(fp);
	if ( numsounds == 0 )
	{
		printlog("failed to identify any sounds in sounds.txt\n");
		return 10;
	}
	sounds = (OPENAL_BUFFER**) malloc(sizeof(OPENAL_BUFFER*)*numsounds);
	fp = openDataFile("sound/sounds.txt", "r");
	for ( c = 0; !feof(fp); c++ )
	{
		fscanf(fp, "%s", name);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
		//TODO: Might need to malloc the sounds[c]->sound
		OPENAL_CreateSound(name, true, &sounds[c]);
		//TODO: set sound volume? Or otherwise handle sound volume.
	}
	fclose(fp);
	OPENAL_ChannelGroup_SetVolume(sound_group, sfxvolume / 128.f);
	//FMOD_System_Set3DSettings(fmod_system, 1.0, 2.0, 1.0); // This on is hardcoded, I've been lazy here'
#endif

	return 0;
}

/*-------------------------------------------------------------------------------

	loadLanguage

	loads the language file with the given language code in *lang

-------------------------------------------------------------------------------*/

int loadLanguage(char* lang)
{
	char filename[128] = { 0 };
	FILE* fp;
	int c;

	// open log file
	if ( !logfile )
	{
		logfile = freopen("log.txt", "wb" /*or "wt"*/, stderr);
	}

	// compose filename
	snprintf(filename, 127, "lang/%s.txt", lang);

	// check if language file is valid
	if ( !dataPathExists(filename) )
	{
		// language file doesn't exist
		printlog("error: unable to locate language file: '%s'", filename);
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
	if ( !dataPathExists(fontName) )
	{
		strncpy(fontName, "lang/en.ttf", 63);
	}
	if ( !dataPathExists(fontName) )
	{
		printlog("error: default game font 'lang/en.ttf' not found");
		return 1;
	}
	completePath(fontPath, fontName);
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
	if ( (fp = openDataFile(filename, "r")) == NULL )
	{
		printlog("error: unable to load language file: '%s'", filename);
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
	for ( line = 1; !feof(fp); )
	{
		//printlog( "loading line %d...\n", line);
		char data[1024];
		int entry = NUMLANGENTRIES;
		int dummy;

		// read line from file
		int i;
		bool fileEnd = false;
		for ( i = 0; ; i++ )
		{
			data[i] = fgetc(fp);
			if ( feof(fp) )
			{
				fileEnd = true;
				break;
			}

			// blank or comment lines stop reading at a newline
			if ( data[i] == '\n' )
			{
				line++;
				if ( data[0] == '\n' || data[0] == '#' )
				{
					break;
				}
			}
			if ( data[i] == '#' )
			{
				if ( data[0] != '\n' && data[0] != '#' )
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
		if ( data[0] == '\n' || data[0] == '#' )
		{
			continue;
		}

		data[i] = 0;

		// process line
		if ( (entry = atoi(data)) == 0 )
		{
			printlog( "warning: syntax error in '%s':%d\n bad syntax!\n", filename, line);
			continue;
		}
		else if ( entry >= NUMLANGENTRIES || entry < 0 )
		{
			printlog( "warning: syntax error in '%s':%d\n invalid language entry!\n", filename, line);
			continue;
		}
		//printlog( "loading entry %d...\n", entry);
		char entryText[16] = { 0 };
		snprintf(entryText, 15, "%d", entry);
		if ( language[entry][0] )
		{
			printlog( "warning: duplicate entry %d in '%s':%d\n", entry, filename, line);
			free(language[entry]);
		}
		language[entry] = (char*) calloc(strlen((char*)(data + strlen(entryText) + 1)) + 1, sizeof(char));
		strcpy(language[entry], (char*)(data + strlen(entryText) + 1));
	}

	// close file
	fclose(fp);
	printlog( "successfully loaded language file '%s'\n", filename);
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
	FILE *model_cache;
	bool generateAll = start == 0 && end == nummodels;

	quads.first = NULL;
	quads.last = NULL;

	printlog("generating poly models...\n");
	if ( generateAll )
	{
		polymodels = (polymodel_t*) malloc(sizeof(polymodel_t) * nummodels);
		if ( useModelCache && !forceCacheRebuild )
		{
			model_cache = openDataFile("models.cache", "rb");
			if (model_cache) {
				for (size_t model_index = 0; model_index < nummodels; model_index++) {
					polymodel_t *cur = &polymodels[model_index];
					fread(&cur->numfaces, sizeof(cur->numfaces), 1, model_cache);
					cur->faces = (polytriangle_t *) calloc(sizeof(polytriangle_t), cur->numfaces);
					fread(polymodels[model_index].faces, sizeof(polytriangle_t), cur->numfaces, model_cache);
				}
				fclose(model_cache);
				return generateVBOs(start, end);
			}
		}
	}

	for ( c = start; c < end; ++c )
	{
		char loadText[128];
		snprintf(loadText, 127, language[745], c, nummodels);

		// print a loading message
		if ( start == 0 && end == nummodels )
		{
			drawClearBuffers();
			int w, h;
			TTF_SizeUTF8(ttf16, loadText, &w, &h);
			ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, loadText);

			GO_SwapBuffers(screen);
		}
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
	if (useModelCache && (model_cache = openDataFile("models.cache", "wb"))) {
		for (size_t model_index = 0; model_index < nummodels; model_index++) {
			polymodel_t *cur = &polymodels[model_index];
			fwrite(&cur->numfaces, sizeof(cur->numfaces), 1, model_cache);
			fwrite(cur->faces, sizeof(polytriangle_t), cur->numfaces, model_cache);
		}
		fclose(model_cache);
	}

	// now store models into VBOs
	if ( !disablevbos )
	{
		generateVBOs(start, end);
	}
}

/*-------------------------------------------------------------------------------

	generateVBOs

	generates VBOs/VAs from polymodel data

-------------------------------------------------------------------------------*/

void generateVBOs(int start, int end)
{
	int i, c;

	for ( c = start; c < end; ++c )
	{
		/*if( c>0 )
			break;*/
		GLfloat* points = (GLfloat*) malloc(sizeof(GLfloat) * 9 * polymodels[c].numfaces);
		GLfloat* colors = (GLfloat*) malloc(sizeof(GLfloat) * 9 * polymodels[c].numfaces);
		GLfloat* colors_shifted = (GLfloat*) malloc(sizeof(GLfloat) * 9 * polymodels[c].numfaces);
		for ( i = 0; i < polymodels[c].numfaces; i++ )
		{
			points[i * 9] = polymodels[c].faces[i].vertex[0].x;
			colors[i * 9] = polymodels[c].faces[i].r / 255.f;
			colors_shifted[i * 9] = polymodels[c].faces[i].b / 255.f;

			points[i * 9 + 1] = -polymodels[c].faces[i].vertex[0].z;
			colors[i * 9 + 1] = polymodels[c].faces[i].g / 255.f;
			colors_shifted[i * 9 + 1] = polymodels[c].faces[i].r / 255.f;

			points[i * 9 + 2] = polymodels[c].faces[i].vertex[0].y;
			colors[i * 9 + 2] = polymodels[c].faces[i].b / 255.f;
			colors_shifted[i * 9 + 2] = polymodels[c].faces[i].g / 255.f;

			points[i * 9 + 3] = polymodels[c].faces[i].vertex[1].x;
			colors[i * 9 + 3] = polymodels[c].faces[i].r / 255.f;
			colors_shifted[i * 9 + 3] = polymodels[c].faces[i].b / 255.f;

			points[i * 9 + 4] = -polymodels[c].faces[i].vertex[1].z;
			colors[i * 9 + 4] = polymodels[c].faces[i].g / 255.f;
			colors_shifted[i * 9 + 4] = polymodels[c].faces[i].r / 255.f;

			points[i * 9 + 5] = polymodels[c].faces[i].vertex[1].y;
			colors[i * 9 + 5] = polymodels[c].faces[i].b / 255.f;
			colors_shifted[i * 9 + 5] = polymodels[c].faces[i].g / 255.f;

			points[i * 9 + 6] = polymodels[c].faces[i].vertex[2].x;
			colors[i * 9 + 6] = polymodels[c].faces[i].r / 255.f;
			colors_shifted[i * 9 + 6] = polymodels[c].faces[i].b / 255.f;

			points[i * 9 + 7] = -polymodels[c].faces[i].vertex[2].z;
			colors[i * 9 + 7] = polymodels[c].faces[i].g / 255.f;
			colors_shifted[i * 9 + 7] = polymodels[c].faces[i].r / 255.f;

			points[i * 9 + 8] = polymodels[c].faces[i].vertex[2].y;
			colors[i * 9 + 8] = polymodels[c].faces[i].b / 255.f;
			colors_shifted[i * 9 + 8] = polymodels[c].faces[i].g / 255.f;
		}
		SDL_glGenVertexArrays(1, &polymodels[c].va);
		SDL_glGenBuffers(1, &polymodels[c].vbo);
		SDL_glGenBuffers(1, &polymodels[c].colors);
		SDL_glGenBuffers(1, &polymodels[c].colors_shifted);
		SDL_glBindVertexArray(polymodels[c].va);

		// vertex data
		// Well, the generic vertex array are not used, so disabled (making it run on any OpenGL 1.5 hardware)
		SDL_glBindBuffer(GL_ARRAY_BUFFER, polymodels[c].vbo);
		SDL_glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 9 * polymodels[c].numfaces, points, GL_STATIC_DRAW);
		//SDL_glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		//SDL_glEnableVertexAttribArray(0);

		// color data
		SDL_glBindBuffer(GL_ARRAY_BUFFER, polymodels[c].colors);
		SDL_glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 9 * polymodels[c].numfaces, colors, GL_STATIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW
		//SDL_glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glColorPointer(3, GL_FLOAT, 0, 0);
		//SDL_glEnableVertexAttribArray(1);

		// shifted color data
		SDL_glBindBuffer(GL_ARRAY_BUFFER, polymodels[c].colors_shifted);
		SDL_glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 9 * polymodels[c].numfaces, colors_shifted, GL_STATIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW
		//SDL_glVertexAttribPointer((GLuint)2, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glColorPointer(3, GL_FLOAT, 0, 0);
		//SDL_glEnableVertexAttribArray(2);

		free(points);
		free(colors);
		free(colors_shifted);
	}
}

/*-------------------------------------------------------------------------------

	deinitApp

	frees all memory consumed by the application and terminates the engine

-------------------------------------------------------------------------------*/
int deinitApp()
{
	Uint32 c;
#ifdef HAVE_OPENAL
	closeOPENAL();
#endif
	// close engine
	printlog("closing engine...\n");
	printlog("removing engine timer...\n");
	if ( timer )
	{
		SDL_RemoveTimer(timer);
	}
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

	printlog("freeing map data...\n");
	if ( map.entities != NULL )
	{
		list_FreeAll(map.entities);
		free(map.entities);
	}
	list_FreeAll(&light_l);
	if ( map.tiles != NULL )
	{
		free(map.tiles);
	}
	if ( lightmap != NULL )
	{
		free(lightmap);
	}
	if ( vismap != NULL )
	{
		free(vismap);
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
			}
		}
		free(polymodels);
	}

	// free sounds
#ifdef HAVE_FMOD
	printlog("freeing sounds...\n");
	if ( sounds != NULL )
	{
		for ( c = 0; c < numsounds; c++ )
		{
			if (sounds[c] != NULL)
			{
				if (sounds[c] != NULL)
				{
					FMOD_Sound_Release(sounds[c]);    //Free the sound's FMOD sound.
				}
				//free(sounds[c]); //Then free the sound itself.
			}
		}
		free(sounds); //Then free the sound array.
	}
#endif

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
	SDLNet_Quit();
	IMG_Quit();
	//Mix_HaltChannel(-1);
	//Mix_CloseAudio();
#ifdef HAVE_FMOD
	if ( fmod_system )
	{
		FMOD_System_Close(fmod_system);
		FMOD_System_Release(fmod_system);
		fmod_system = NULL;
	}
#endif
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
		SteamAPI_Shutdown();
	}
#endif

	// free currently loaded language if any
	freeLanguages();

	printlog("success\n");
	fclose(logfile);
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
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 1/*3*/ ); //Why GL 3.0? using only fixed pipeline stuff here
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );
	SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
	//SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
	//SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
	//SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 4 );

	printlog("setting display mode to %dx%d...\n", xres, yres);
	Uint32 flags = 0;
#ifdef PANDORA
	fullscreen = true;
#endif
	if ( fullscreen )
	{
		flags |= SDL_WINDOW_FULLSCREEN;
	}
	if ( !game )
	{
		flags |= SDL_WINDOW_RESIZABLE;
	}
	if ( !softwaremode )
	{
		flags |= SDL_WINDOW_OPENGL;
	}
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
	if (splitscreen)
	{
		screen_width *= 2;
	}
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
		if ( fullscreen )
		{
			SDL_SetWindowFullscreen(screen, SDL_WINDOW_FULLSCREEN);
		}
		else
		{
			SDL_SetWindowFullscreen(screen, 0);
		}
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
	if ( SDL_SetWindowBrightness(screen, vidgamma) < 0 )
	{
		printlog("warning: failed to change gamma setting:\n%s\n", SDL_GetError());
		return true;
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

bool changeVideoMode()
{
	printlog("changing video mode.\n");
#ifdef PANDORA
	GO_InitFBO();
#else
	int c;

	// delete old texture names (they're going away anyway)
	glDeleteTextures(MAXTEXTURES, texid);

	// delete vertex data
	if ( !disablevbos )
	{
		for ( c = 0; c < nummodels; c++ )
		{
			SDL_glDeleteBuffers(1, &polymodels[c].vbo);
			SDL_glDeleteBuffers(1, &polymodels[c].colors);
			SDL_glDeleteVertexArrays(1, &polymodels[c].va);
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
		xres = 960;
		yres = 600;
		fullscreen = 0;
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
#endif
	// success
	return true;
}

/*-------------------------------------------------------------------------------

loadItemLists()

loads the global item whitelist/blacklists and level curve.

-------------------------------------------------------------------------------*/

bool loadItemLists()
{
	char filename[128] = { 0 };
	//FILE* fp;
	int c;

	// open log file
	if ( !logfile )
	{
		logfile = freopen("log.txt", "wb" /*or "wt"*/, stderr);
	}

	// compose filename
	strcpy(filename, "items/items_global.txt");
	// check if item list is valid
	if ( !dataPathExists(filename) )
	{
		// file doesn't exist
		printlog("error: unable to locate tile palette file: '%s'", filename);
		return false;
	}

	std::vector<std::string> itemLevels = getLinesFromFile(filename);
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

	printlog("successfully loaded global item list '%s' \n", filename);
	/*for ( c = 0; c < NUMITEMS; ++c )
	{
		printlog("%s level: %d", items[c].name_identified, items[c].level);
	}*/
	return true;
}