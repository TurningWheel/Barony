/*-------------------------------------------------------------------------------

	BARONY
	File: init.cpp
	Desc: contains program initialization code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "sound.hpp"
#include "prng.hpp"
#include "hash.hpp"
#include "net.hpp"
#ifdef STEAMWORKS
#include <steam/steam_api.h>
#include "steam.hpp"
#endif
#include "player.hpp"

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

FILE *logfile = nullptr;
bool steam_init = FALSE;

int initApp(char *title, int fullscreen) {
	char name[128];
	FILE *fp;
	Uint32 x, c;

	// open log file
	if( !logfile )
		logfile = freopen("log.txt", "wb" /*or "wt"*/, stderr);

	for (c = 0; c < NUM_JOY_STATUS; ++c) {
		joystatus[c] = 0;
	}
	for (c = 0; c < NUM_JOY_TRIGGER_STATUS; ++c) {
		joy_trigger_status[c] = 0;
	}

	// init some lists
	button_l.first=NULL;
	button_l.last=NULL;
	light_l.first=NULL;
	light_l.last=NULL;
	entitiesdeleted.first=NULL;
	entitiesdeleted.last=NULL;
	for( c=0; c<HASH_SIZE; c++ ) {
		ttfTextHash[c].first = NULL;
		ttfTextHash[c].last = NULL;
	}
	map.entities = NULL;
	map.tiles = NULL;

	// init steamworks
#ifdef STEAMWORKS
	SteamAPI_RestartAppIfNecessary(STEAM_APPID);
	if( !SteamAPI_Init() ) {
		printlog("error: failed to initialize Steamworks!\n");
		printlog(" make sure your steam client is running before attempting to start again.\n");
		return 1;
	}
	steam_init = TRUE;
#endif

	window_title = title;
	printlog("initializing SDL...\n");
	if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER ) == -1 ) {
		printlog("failed to initialize SDL: %s\n",SDL_GetError());
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
	if (FMODErrorCheck()) {
		printlog("Failed to create FMOD.\n");
		no_sound = TRUE;
	}
	if (!no_sound) {
		//FMOD_System_SetOutput(fmod_system, FMOD_OUTPUTTYPE_DSOUND);
		fmod_result = FMOD_System_Init(fmod_system, fmod_maxchannels, FMOD_INIT_NORMAL | FMOD_INIT_3D_RIGHTHANDED, 0);
		if (FMODErrorCheck()) {
			printlog("Failed to initialize FMOD.\n");
			no_sound = TRUE;
		}
		if (!no_sound) {
			fmod_result = FMOD_System_CreateChannelGroup(fmod_system, NULL, &sound_group);
			if (FMODErrorCheck()) {
				printlog("Failed to create sound channel group.\n");
				no_sound = TRUE;
			}
			if (!no_sound) {
				fmod_result = FMOD_System_CreateChannelGroup(fmod_system, NULL, &music_group);
				if (FMODErrorCheck()) {
					printlog("Failed to create music channel group.\n");
					no_sound = TRUE;
				}
			}
		}
	}
#endif
	printlog("initializing SDL_net...\n");
	if( SDLNet_Init() < 0 ) {
		printlog("failed to initialize SDL_net: %s\n", SDLNet_GetError());
		return 2;
	}
	printlog("initializing SDL_image...\n");
	if( IMG_Init(IMG_INIT_PNG) != (IMG_INIT_PNG) ) {
		printlog("failed to initialize SDL_image: %s\n", IMG_GetError());
		return 2;
	}

	// hide cursor for game
	if( game )
		SDL_ShowCursor(SDL_FALSE);
	SDL_StopTextInput();

	// initialize video
	if( !initVideo() ) {
		return 3;
	}
	//SDL_EnableUNICODE(1);
	//SDL_WM_SetCaption(title, 0);
	//SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,SDL_DEFAULT_REPEAT_INTERVAL);

	// get pointers to opengl extensions
#ifdef WINDOWS
	bool noextensions=FALSE;
	if( !softwaremode ) {
		if( (SDL_glGenBuffers=(PFNGLGENBUFFERSPROC)SDL_GL_GetProcAddress("glGenBuffers"))==NULL )
			noextensions=TRUE;
		else if( (SDL_glBindBuffer=(PFNGLBINDBUFFERPROC)SDL_GL_GetProcAddress("glBindBuffer"))==NULL )
			noextensions=TRUE;
		else if( (SDL_glBufferData=(PFNGLBUFFERDATAPROC)SDL_GL_GetProcAddress("glBufferData"))==NULL )
			noextensions=TRUE;
		else if( (SDL_glDeleteBuffers=(PFNGLDELETEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteBuffers"))==NULL )
			noextensions=TRUE;
		else if( (SDL_glGenVertexArrays=(PFNGLGENVERTEXARRAYSPROC)SDL_GL_GetProcAddress("glGenVertexArrays"))==NULL )
			noextensions=TRUE;
		else if( (SDL_glBindVertexArray=(PFNGLBINDVERTEXARRAYPROC)SDL_GL_GetProcAddress("glBindVertexArray"))==NULL )
			noextensions=TRUE;
		else if( (SDL_glDeleteVertexArrays=(PFNGLDELETEVERTEXARRAYSPROC)SDL_GL_GetProcAddress("glDeleteVertexArrays"))==NULL )
			noextensions=TRUE;
		else if( (SDL_glEnableVertexAttribArray=(PFNGLENABLEVERTEXATTRIBARRAYPROC)SDL_GL_GetProcAddress("glEnableVertexAttribArray"))==NULL )
			noextensions=TRUE;
		else if( (SDL_glVertexAttribPointer=(PFNGLVERTEXATTRIBPOINTERPROC)SDL_GL_GetProcAddress("glVertexAttribPointer"))==NULL )
			noextensions=TRUE;
	}
	if (softwaremode)
		printlog("notice: using software rendering.\n");
	if( noextensions ) {
		printlog("warning: failed to load OpenGL extensions.\nYou may want to update your drivers or your graphics card, as performance will be reduced without these.\n");
		disablevbos=TRUE;
	}
#else
	if (softwaremode)
		printlog("notice: using software rendering.\n");
#endif

	// initialize buffers
	zbuffer=(double *) malloc(sizeof(double)*xres*yres);
	clickmap=(Entity **) malloc(sizeof(Entity *)*xres*yres);
	texid = (GLuint *) malloc(MAXTEXTURES*sizeof(GLuint));
	//vaoid = (GLuint *) malloc(MAXBUFFERS*sizeof(GLuint));
	//vboid = (GLuint *) malloc(MAXBUFFERS*sizeof(GLuint));
	allsurfaces = (SDL_Surface **) malloc(sizeof(SDL_Surface *)*MAXTEXTURES);
	for( c=0; c<MAXTEXTURES; c++ )
		allsurfaces[c]=NULL;
	glGenTextures(MAXTEXTURES,texid);
	//SDL_glGenVertexArrays(MAXBUFFERS, vaoid);
	//SDL_glGenBuffers(MAXBUFFERS, vboid);

	// load windows icon
#ifndef _MSC_VER
#if defined(WINDOWS) && defined(GCL_HICON)
	HINSTANCE handle = GetModuleHandle(NULL);
	HICON icon = LoadIcon(handle, "id");
	if( icon != NULL ) {
		SDL_SysWMinfo wminfo;
		SDL_VERSION( &wminfo.version );
		if( SDL_GetWindowWMInfo(screen,&wminfo)==SDL_TRUE ) {
			HWND hwnd = wminfo.info.win.window;
			SetClassLong(hwnd, GCL_HICON, (LONG)icon);
		}
	}
#endif
#endif

	// load resources
	printlog("loading engine resources...\n");
	if((fancyWindow_bmp=loadImage("images/system/fancyWindow.png")) == NULL) {
		printlog("failed to load fancyWindow.png\n");
		return 5;
	}
	if((font8x8_bmp=loadImage("images/system/font8x8.png")) == NULL) {
		printlog("failed to load font8x8.png\n");
		return 5;
	}
	if((font12x12_bmp=loadImage("images/system/font12x12.png")) == NULL) {
		printlog("failed to load font12x12.png\n");
		return 5;
	}
	if((font16x16_bmp=loadImage("images/system/font16x16.png")) == NULL) {
		printlog("failed to load font16x16.png\n");
		return 5;
	}

	// print a loading message
	drawClearBuffers();
	int w, h;
	TTF_SizeUTF8(ttf16,LOADSTR1,&w,&h);
	ttfPrintText(ttf16,(xres-w)/2,(yres-h)/2,LOADSTR1);
#ifdef APPLE
	SDL_RenderPresent(renderer);
#else
	SDL_GL_SwapWindow(screen);
#endif

	// load sprites
	printlog("loading sprites...\n");
	fp = fopen("images/sprites.txt","r");
	for( numsprites=0; !feof(fp); numsprites++ ) {
		while( fgetc(fp) != '\n' ) if( feof(fp) ) break;
	}
	fclose(fp);
	if( numsprites==0 ) {
		printlog("failed to identify any sprites in sprites.txt\n");
		return 6;
	}
	sprites = (SDL_Surface **) malloc(sizeof(SDL_Surface *)*numsprites);
	fp = fopen("images/sprites.txt","r");
	for( c=0; !feof(fp); c++ ) {
		fscanf(fp,"%s",name);
		while( fgetc(fp) != '\n' ) if( feof(fp) ) break;
		sprites[c] = loadImage(name);
		if( sprites[c] == NULL ) {
			printlog("warning: failed to load '%s' listed at line %d in sprites.txt\n",name,c+1);
			if( c==0 ) {
				printlog("sprite 0 cannot be NULL!\n");
				return 7;
			}
		}
	}

	// print a loading message
	drawClearBuffers();
	TTF_SizeUTF8(ttf16,LOADSTR2,&w,&h);
	ttfPrintText(ttf16,(xres-w)/2,(yres-h)/2,LOADSTR2);
#ifdef APPLE
	SDL_RenderPresent(renderer);
#else
	SDL_GL_SwapWindow(screen);
#endif

	// load models
	printlog("loading models...\n");
	fp = fopen("models/models.txt","r");
	for( nummodels=0; !feof(fp); nummodels++ ) {
		while( fgetc(fp) != '\n' ) if( feof(fp) ) break;
	}
	fclose(fp);
	if( nummodels==0 ) {
		printlog("failed to identify any models in models.txt\n");
		return 11;
	}
	models = (voxel_t **) malloc(sizeof(voxel_t *)*nummodels);
	fp = fopen("models/models.txt","r");
	for( c=0; !feof(fp); c++ ) {
		fscanf(fp,"%s",name);
		while( fgetc(fp) != '\n' ) if( feof(fp) ) break;
		models[c] = loadVoxel(name);
		if( models[c] == NULL ) {
			printlog("warning: failed to load '%s' listed at line %d in models.txt\n",name,c+1);
			if( c==0 ) {
				printlog("model 0 cannot be NULL!\n");
				return 12;
			}
		}
	}
	if( !softwaremode )
		generatePolyModels();

	// print a loading message
	drawClearBuffers();
	TTF_SizeUTF8(ttf16,LOADSTR3,&w,&h);
	ttfPrintText(ttf16,(xres-w)/2,(yres-h)/2,LOADSTR3);
#ifdef APPLE
	SDL_RenderPresent(renderer);
#else
	SDL_GL_SwapWindow(screen);
#endif

	// load tiles
	printlog("loading tiles...\n");
	fp = fopen("images/tiles.txt","r");
	for( numtiles=0; !feof(fp); numtiles++ ) {
		while( fgetc(fp) != '\n' ) if( feof(fp) ) break;
	}
	fclose(fp);
	if( numtiles==0 ) {
		printlog("failed to identify any tiles in tiles.txt\n");
		return 8;
	}
	tiles = (SDL_Surface **) malloc(sizeof(SDL_Surface *)*numtiles);
	animatedtiles = (bool *) malloc(sizeof(bool)*numtiles);
	lavatiles = (bool *) malloc(sizeof(bool)*numtiles);
	fp = fopen("images/tiles.txt","r");
	for( c=0; !feof(fp); c++ ) {
		fscanf(fp,"%s",name);
		while( fgetc(fp) != '\n' ) if( feof(fp) ) break;
		tiles[c] = loadImage(name);
		animatedtiles[c] = FALSE;
		lavatiles[c] = FALSE;
		if( tiles[c] != NULL ) {
			for(x=0; x<strlen(name); x++) {
				if( name[x]>=48 && name[x]<58 ) {
					animatedtiles[c]=TRUE;
					break;
				}
			}
			if( strstr(name,"Lava") || strstr(name,"lava") ) {
				lavatiles[c]=TRUE;
			}
		} else {
			printlog("warning: failed to load '%s' listed at line %d in tiles.txt\n",name,c+1);
			if( c==0 ) {
				printlog("tile 0 cannot be NULL!\n");
				return 9;
			}
		}
	}

	// print a loading message
	drawClearBuffers();
	TTF_SizeUTF8(ttf16,LOADSTR4,&w,&h);
	ttfPrintText(ttf16,(xres-w)/2,(yres-h)/2,LOADSTR4);
#ifdef APPLE
	SDL_RenderPresent(renderer);
#else
	SDL_GL_SwapWindow(screen);
#endif

	// load sound effects
#ifdef HAVE_FMOD
	printlog("loading sounds...\n");
	fp = fopen("sound/sounds.txt","r");
	for( numsounds=0; !feof(fp); numsounds++ ) {
		while( fgetc(fp) != '\n' ) if( feof(fp) ) break;
	}
	fclose(fp);
	if( numsounds==0 ) {
		printlog("failed to identify any sounds in sounds.txt\n");
		return 10;
	}
	sounds = (FMOD_SOUND **) malloc(sizeof(FMOD_SOUND *)*numsounds);
	fp = fopen("sound/sounds.txt","r");
	for( c=0; !feof(fp); c++ ) {
		fscanf(fp,"%s",name);
		while( fgetc(fp) != '\n' ) if( feof(fp) ) break;
		//TODO: Might need to malloc the sounds[c]->sound
		fmod_result = FMOD_System_CreateSound(fmod_system, name, (FMOD_MODE)(FMOD_SOFTWARE | FMOD_3D), NULL, &sounds[c]);
		if (FMODErrorCheck()) {
			printlog("warning: failed to load '%s' listed at line %d in sounds.txt\n",name,c+1);
		}
		//TODO: set sound volume? Or otherwise handle sound volume.
	}
	fclose(fp);
	FMOD_ChannelGroup_SetVolume(sound_group, sfxvolume / 128.f);
	FMOD_System_Set3DSettings(fmod_system, 1.0, 2.0, 1.0);
#endif

	return 0;
}

/*-------------------------------------------------------------------------------

	loadLanguage

	loads the language file with the given language code in *lang

-------------------------------------------------------------------------------*/

int loadLanguage(char *lang) {
	char filename[128] = { 0 };
	FILE *fp;
	int c;

	// open log file
	if( !logfile )
		logfile = freopen("log.txt", "wb" /*or "wt"*/, stderr);

	// compose filename
	snprintf(filename,127,"lang/%s.txt",lang);

	// check if language file is valid
	if( access( filename, F_OK ) == -1 ) {
		// language file doesn't exist
		printlog("error: unable to locate language file: '%s'",filename);
		return 1;
	}

	// check if we've loaded this language already
	if( !strcmp(languageCode,lang) ) {
		printlog("info: language '%s' already loaded",lang);
		return 1;
	}

	// init SDL_TTF
	if( !TTF_WasInit() ) {
		if( TTF_Init() == -1 ) {
			printlog("failed to initialize SDL_ttf.\n");
			return 1;
		}
	}

	// load fonts
	char fontName[64] = { 0 };
	snprintf(fontName,63,"lang/%s.ttf",lang);
	if( access(fontName, F_OK)==-1 ) {
		snprintf(fontName,63,"lang/en.ttf");
	}
	if( access(fontName, F_OK)==-1 ) {
		printlog("error: default game font 'lang/en.ttf' not found");
		return 1;
	}
	if( ttf8 )
		TTF_CloseFont(ttf8);
	if((ttf8=TTF_OpenFont(fontName,TTF8_HEIGHT)) == NULL ) {
		printlog("failed to load size 8 ttf: %s\n",TTF_GetError());
		return 1;
	}
	TTF_SetFontKerning(ttf8, 0);
	TTF_SetFontHinting(ttf8, TTF_HINTING_MONO);
	if( ttf12 )
		TTF_CloseFont(ttf12);
	if((ttf12=TTF_OpenFont(fontName,TTF12_HEIGHT)) == NULL ) {
		printlog("failed to load size 12 ttf: %s\n",TTF_GetError());
		return 1;
	}
	TTF_SetFontKerning(ttf12, 0);
	TTF_SetFontHinting(ttf12, TTF_HINTING_MONO);
	if( ttf16 )
		TTF_CloseFont(ttf16);
	if((ttf16=TTF_OpenFont(fontName,TTF16_HEIGHT)) == NULL ) {
		printlog("failed to load size 16 ttf: %s\n",TTF_GetError());
		return 1;
	}
	TTF_SetFontKerning(ttf16, 0);
	TTF_SetFontHinting(ttf16, TTF_HINTING_MONO);

	// open language file
	if( (fp=fopen(filename,"r"))==NULL ) {
		printlog("error: unable to load language file: '%s'",filename);
		return 1;
	}

	// free currently loaded language if any
	if( language ) {
		for( c=0; c<NUMLANGENTRIES; c++ ) {
			char *entry = language[c];
			if( entry ) {
				free(entry);
			}
		}
		free(language);
	}

	// store the new language code
	strcpy(languageCode,lang);

	// allocate new language strings
	language = (char **) calloc(NUMLANGENTRIES,sizeof(char *));

	// Allocate an emptry string for each possible language entry
	for (c = 0; c < NUMLANGENTRIES; c++) {
		language[c] = (char *)calloc(1, sizeof(char));
	}

	// read file
	Uint32 line;
	for( line=1; !feof(fp); ) {
		//printlog( "loading line %d...\n", line);
		char data[1024];
		int entry=NUMLANGENTRIES;
		int dummy;

		// read line from file
		int i;
		bool fileEnd=FALSE;
		for( i=0; ; i++ ) {
			data[i] = fgetc(fp);
			if( feof(fp) ) {
				fileEnd=TRUE;
				break;
			}

			// blank or comment lines stop reading at a newline
			if( data[i] == '\n' ) {
				line++;
				if( data[0] == '\n' || data[0] == '#' ) {
					break;
				}
			}
			if( data[i] == '#' ) {
				if( data[0] != '\n' && data[0] != '#' ) {
					break;
				}
			}
		}
		if( fileEnd )
			break;

		// skip blank and comment lines
		if( data[0] == '\n' || data[0] == '#' )
			continue;

		data[i] = 0;

		// process line
		if( (entry=atoi(data)) == 0 ) {
			printlog( "warning: syntax error in '%s':%d\n bad syntax!\n",filename,line);
			continue;
		} else if( entry>=NUMLANGENTRIES || entry<0 ) {
			printlog( "warning: syntax error in '%s':%d\n invalid language entry!\n",filename,line);
			continue;
		}
		//printlog( "loading entry %d...\n", entry);
		char entryText[16] = { 0 };
		snprintf(entryText,15,"%d",entry);
		if( language[entry][0] ) {
			printlog( "warning: duplicate entry %d in '%s':%d\n",entry,filename,line);
			free(language[entry]);
		}
		language[entry] = (char *) calloc(strlen((char *)(data+strlen(entryText)+1))+1,sizeof(char));
		strcpy(language[entry],(char *)(data+strlen(entryText)+1));
	}

	// close file
	fclose(fp);
	printlog( "successfully loaded language file '%s'\n",filename);
	return 0;
}

/*-------------------------------------------------------------------------------

	reloadLanguage

	reloads the current language file

-------------------------------------------------------------------------------*/

int reloadLanguage() {
	char lang[32];

	strcpy(lang,languageCode);
	strcpy(languageCode,"");
	return loadLanguage(lang);
}

/*-------------------------------------------------------------------------------

	generatePolyModels

	processes voxel models and turns them into polygon-based models (surface
	optimized)

-------------------------------------------------------------------------------*/

void generatePolyModels() {
	Sint32 x, y, z;
	Sint32 c, i;
	Uint32 index, indexdown[3];
	Uint8 newcolor, oldcolor;
	bool buildingquad;
	polyquad_t *quad1, *quad2;
	Uint32 numquads;
	list_t quads;

	quads.first = NULL;
	quads.last = NULL;

	printlog("generating poly models...\n");
	polymodels = (polymodel_t *) malloc(sizeof(polymodel_t)*nummodels);
	for( c=0; c<nummodels; ++c ) {
		char loadText[128];
		snprintf(loadText,127,language[745],c,nummodels);

		// print a loading message
		drawClearBuffers();
		int w, h;
		TTF_SizeUTF8(ttf16,loadText,&w,&h);
		ttfPrintText(ttf16,(xres-w)/2,(yres-h)/2,loadText);
#ifdef APPLE
		SDL_RenderPresent(renderer);
#else
		SDL_GL_SwapWindow(screen);
#endif

		numquads=0;
		polymodels[c].numfaces = 0;
		voxel_t *model = models[c];
		if( !model )
			continue;
		indexdown[0] = model->sizez*model->sizey;
		indexdown[1] = model->sizez;
		indexdown[2] = 1;

		// find front faces
		for( x=models[c]->sizex-1; x>=0; x-- ) {
			for( z=0; z<models[c]->sizez; z++ ) {
				oldcolor=255;
				buildingquad=FALSE;
				for( y=0; y<models[c]->sizey; y++ ) {
					index = z+y*models[c]->sizez+x*models[c]->sizey*models[c]->sizez;
					newcolor = models[c]->data[index];
					if( buildingquad==TRUE ) {
						bool doit=FALSE;
						if( newcolor != oldcolor )
							doit=TRUE;
						else if( x<models[c]->sizex-1 )
							if( models[c]->data[index+indexdown[0]]>=0 && models[c]->data[index+indexdown[0]]<255 )
								doit=TRUE;
						if( doit ) {
							// add the last two vertices to the previous quad
							buildingquad=FALSE;

							node_t *currentNode = quads.last;
							quad1 = (polyquad_t *)currentNode->element;
							quad1->vertex[1].x = x-model->sizex/2.f+1;
							quad1->vertex[1].y = y-model->sizey/2.f;
							quad1->vertex[1].z = z-model->sizez/2.f-1;
							quad1->vertex[2].x = x-model->sizex/2.f+1;
							quad1->vertex[2].y = y-model->sizey/2.f;
							quad1->vertex[2].z = z-model->sizez/2.f;

							// optimize quad
							node_t *node;
							for( i=0, node=quads.first; i<numquads-1; i++, node=node->next ) {
								quad2 = (polyquad_t *)node->element;
								if( quad1->side == quad2->side ) {
									if( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b ) {
										if( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z ) {
											if( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z ) {
												quad2->vertex[2].z++;
												quad2->vertex[3].z++;
												list_RemoveNode(currentNode);
												numquads--;
												polymodels[c].numfaces-=2;
												break;
											}
										}
									}
								}
							}
						}
					}
					if( newcolor != oldcolor || !buildingquad ) {
						if( newcolor != 255 ) {
							bool doit=FALSE;
							if( x==models[c]->sizex-1 )
								doit=TRUE;
							else if( models[c]->data[index+indexdown[0]]==255 )
								doit=TRUE;
							if( doit ) {
								// start building a new quad
								buildingquad=TRUE;
								numquads++;
								polymodels[c].numfaces+=2;

								quad1 = (polyquad_t *) calloc(1,sizeof(polyquad_t));
								quad1->side = 0;
								quad1->vertex[0].x = x-model->sizex/2.f+1;
								quad1->vertex[0].y = y-model->sizey/2.f;
								quad1->vertex[0].z = z-model->sizez/2.f-1;
								quad1->vertex[3].x = x-model->sizex/2.f+1;
								quad1->vertex[3].y = y-model->sizey/2.f;
								quad1->vertex[3].z = z-model->sizez/2.f;
								quad1->r = models[c]->palette[models[c]->data[index]][0];
								quad1->g = models[c]->palette[models[c]->data[index]][1];
								quad1->b = models[c]->palette[models[c]->data[index]][2];

								node_t *newNode = list_AddNodeLast(&quads);
								newNode->element = quad1;
								newNode->deconstructor = &defaultDeconstructor;
								newNode->size = sizeof(polyquad_t);
							}
						}
					}
					oldcolor=newcolor;
				}
				if( buildingquad==TRUE ) {
					// add the last two vertices to the previous quad
					buildingquad=FALSE;

					node_t *currentNode = quads.last;
					quad1 = (polyquad_t *)currentNode->element;
					quad1->vertex[1].x = x-model->sizex/2.f+1;
					quad1->vertex[1].y = y-model->sizey/2.f;
					quad1->vertex[1].z = z-model->sizez/2.f-1;
					quad1->vertex[2].x = x-model->sizex/2.f+1;
					quad1->vertex[2].y = y-model->sizey/2.f;
					quad1->vertex[2].z = z-model->sizez/2.f;

					// optimize quad
					node_t *node;
					for( i=0, node=quads.first; i<numquads-1; i++, node=node->next ) {
						quad2 = (polyquad_t *)node->element;
						if( quad1->side == quad2->side ) {
							if( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b ) {
								if( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z ) {
									if( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z ) {
										quad2->vertex[2].z++;
										quad2->vertex[3].z++;
										list_RemoveNode(currentNode);
										numquads--;
										polymodels[c].numfaces-=2;
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
		for( x=0; x<models[c]->sizex; x++ ) {
			for( z=0; z<models[c]->sizez; z++ ) {
				oldcolor=255;
				buildingquad=FALSE;
				for( y=0; y<models[c]->sizey; y++ ) {
					index = z+y*models[c]->sizez+x*models[c]->sizey*models[c]->sizez;
					newcolor = models[c]->data[index];
					if( buildingquad==TRUE ) {
						bool doit=FALSE;
						if( newcolor != oldcolor )
							doit=TRUE;
						else if( x>0 )
							if( models[c]->data[index-indexdown[0]]>=0 && models[c]->data[index-indexdown[0]]<255 )
								doit=TRUE;
						if( doit ) {
							// add the last two vertices to the previous quad
							buildingquad=FALSE;

							node_t *currentNode = quads.last;
							quad1 = (polyquad_t *)currentNode->element;
							quad1->vertex[1].x = x-model->sizex/2.f;
							quad1->vertex[1].y = y-model->sizey/2.f;
							quad1->vertex[1].z = z-model->sizez/2.f;
							quad1->vertex[2].x = x-model->sizex/2.f;
							quad1->vertex[2].y = y-model->sizey/2.f;
							quad1->vertex[2].z = z-model->sizez/2.f-1;

							// optimize quad
							node_t *node;
							for( i=0, node=quads.first; i<numquads-1; i++, node=node->next ) {
								quad2 = (polyquad_t *)node->element;
								if( quad1->side == quad2->side ) {
									if( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b ) {
										if( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z ) {
											if( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z ) {
												quad2->vertex[0].z++;
												quad2->vertex[1].z++;
												list_RemoveNode(currentNode);
												numquads--;
												polymodels[c].numfaces-=2;
												break;
											}
										}
									}
								}
							}
						}
					}
					if( newcolor != oldcolor || !buildingquad ) {
						if( newcolor != 255 ) {
							bool doit=FALSE;
							if( x==0 )
								doit=TRUE;
							else if( models[c]->data[index-indexdown[0]]==255 )
								doit=TRUE;
							if( doit ) {
								// start building a new quad
								buildingquad=TRUE;
								numquads++;
								polymodels[c].numfaces+=2;

								quad1 = (polyquad_t *) calloc(1,sizeof(polyquad_t));
								quad1->side = 1;
								quad1->vertex[0].x = x-model->sizex/2.f;
								quad1->vertex[0].y = y-model->sizey/2.f;
								quad1->vertex[0].z = z-model->sizez/2.f;
								quad1->vertex[3].x = x-model->sizex/2.f;
								quad1->vertex[3].y = y-model->sizey/2.f;
								quad1->vertex[3].z = z-model->sizez/2.f-1;
								quad1->r = models[c]->palette[models[c]->data[index]][0];
								quad1->g = models[c]->palette[models[c]->data[index]][1];
								quad1->b = models[c]->palette[models[c]->data[index]][2];

								node_t *newNode = list_AddNodeLast(&quads);
								newNode->element = quad1;
								newNode->deconstructor = &defaultDeconstructor;
								newNode->size = sizeof(polyquad_t);
							}
						}
					}
					oldcolor=newcolor;
				}
				if( buildingquad==TRUE ) {
					// add the last two vertices to the previous quad
					buildingquad=FALSE;

					node_t *currentNode = quads.last;
					quad1 = (polyquad_t *)currentNode->element;
					quad1->vertex[1].x = x-model->sizex/2.f;
					quad1->vertex[1].y = y-model->sizey/2.f;
					quad1->vertex[1].z = z-model->sizez/2.f;
					quad1->vertex[2].x = x-model->sizex/2.f;
					quad1->vertex[2].y = y-model->sizey/2.f;
					quad1->vertex[2].z = z-model->sizez/2.f-1;

					// optimize quad
					node_t *node;
					for( i=0, node=quads.first; i<numquads-1; i++, node=node->next ) {
						quad2 = (polyquad_t *)node->element;
						if( quad1->side == quad2->side ) {
							if( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b ) {
								if( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z ) {
									if( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z ) {
										quad2->vertex[0].z++;
										quad2->vertex[1].z++;
										list_RemoveNode(currentNode);
										numquads--;
										polymodels[c].numfaces-=2;
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
		for( y=models[c]->sizey-1; y>=0; y-- ) {
			for( z=0; z<models[c]->sizez; z++ ) {
				oldcolor=255;
				buildingquad=FALSE;
				for( x=0; x<models[c]->sizex; x++ ) {
					index = z+y*models[c]->sizez+x*models[c]->sizey*models[c]->sizez;
					newcolor = models[c]->data[index];
					if( buildingquad==TRUE ) {
						bool doit=FALSE;
						if( newcolor != oldcolor )
							doit=TRUE;
						else if( y<models[c]->sizey-1 )
							if( models[c]->data[index+indexdown[1]]>=0 && models[c]->data[index+indexdown[1]]<255 )
								doit=TRUE;
						if( doit ) {
							// add the last two vertices to the previous quad
							buildingquad=FALSE;

							node_t *currentNode = quads.last;
							quad1 = (polyquad_t *) currentNode->element;
							quad1->vertex[1].x = x-model->sizex/2.f;
							quad1->vertex[1].y = y-model->sizey/2.f+1;
							quad1->vertex[1].z = z-model->sizez/2.f;
							quad1->vertex[2].x = x-model->sizex/2.f;
							quad1->vertex[2].y = y-model->sizey/2.f+1;
							quad1->vertex[2].z = z-model->sizez/2.f-1;

							// optimize quad
							node_t *node;
							for( i=0, node=quads.first; i<numquads-1; i++, node=node->next ) {
								quad2 = (polyquad_t *)node->element;
								if( quad1->side == quad2->side ) {
									if( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b ) {
										if( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z ) {
											if( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z ) {
												quad2->vertex[0].z++;
												quad2->vertex[1].z++;
												list_RemoveNode(currentNode);
												numquads--;
												polymodels[c].numfaces-=2;
												break;
											}
										}
									}
								}
							}
						}
					}
					if( newcolor != oldcolor || !buildingquad ) {
						if( newcolor != 255 ) {
							bool doit=FALSE;
							if( y==models[c]->sizey-1 )
								doit=TRUE;
							else if( models[c]->data[index+indexdown[1]]==255 )
								doit=TRUE;
							if( doit ) {
								// start building a new quad
								buildingquad=TRUE;
								numquads++;
								polymodels[c].numfaces+=2;

								quad1 = (polyquad_t *) calloc(1,sizeof(polyquad_t));
								quad1->side = 2;
								quad1->vertex[0].x = x-model->sizex/2.f;
								quad1->vertex[0].y = y-model->sizey/2.f+1;
								quad1->vertex[0].z = z-model->sizez/2.f;
								quad1->vertex[3].x = x-model->sizex/2.f;
								quad1->vertex[3].y = y-model->sizey/2.f+1;
								quad1->vertex[3].z = z-model->sizez/2.f-1;
								quad1->r = models[c]->palette[models[c]->data[index]][0];
								quad1->g = models[c]->palette[models[c]->data[index]][1];
								quad1->b = models[c]->palette[models[c]->data[index]][2];

								node_t *newNode = list_AddNodeLast(&quads);
								newNode->element = quad1;
								newNode->deconstructor = &defaultDeconstructor;
								newNode->size = sizeof(polyquad_t);
							}
						}
					}
					oldcolor=newcolor;
				}
				if( buildingquad==TRUE ) {
					// add the last two vertices to the previous quad
					buildingquad=FALSE;
					node_t *currentNode = quads.last;
					quad1 = (polyquad_t *) currentNode->element;
					quad1->vertex[1].x = x-model->sizex/2.f;
					quad1->vertex[1].y = y-model->sizey/2.f+1;
					quad1->vertex[1].z = z-model->sizez/2.f;
					quad1->vertex[2].x = x-model->sizex/2.f;
					quad1->vertex[2].y = y-model->sizey/2.f+1;
					quad1->vertex[2].z = z-model->sizez/2.f-1;

					// optimize quad
					node_t *node;
					for( i=0, node=quads.first; i<numquads-1; i++, node=node->next ) {
						quad2 = (polyquad_t *)node->element;
						if( quad1->side == quad2->side ) {
							if( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b ) {
								if( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z ) {
									if( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z ) {
										quad2->vertex[0].z++;
										quad2->vertex[1].z++;
										list_RemoveNode(currentNode);
										numquads--;
										polymodels[c].numfaces-=2;
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
		for( y=0; y<models[c]->sizey; y++ ) {
			for( z=0; z<models[c]->sizez; z++ ) {
				oldcolor=255;
				buildingquad=FALSE;
				for( x=0; x<models[c]->sizex; x++ ) {
					index = z+y*models[c]->sizez+x*models[c]->sizey*models[c]->sizez;
					newcolor = models[c]->data[index];
					if( buildingquad==TRUE ) {
						bool doit=FALSE;
						if( newcolor != oldcolor )
							doit=TRUE;
						else if( y>0 )
							if( models[c]->data[index-indexdown[1]]>=0 && models[c]->data[index-indexdown[1]]<255 )
								doit=TRUE;
						if( doit ) {
							// add the last two vertices to the previous quad
							buildingquad=FALSE;

							node_t *currentNode = quads.last;
							quad1 = (polyquad_t *) currentNode->element;
							quad1->vertex[1].x = x-model->sizex/2.f;
							quad1->vertex[1].y = y-model->sizey/2.f;
							quad1->vertex[1].z = z-model->sizez/2.f-1;
							quad1->vertex[2].x = x-model->sizex/2.f;
							quad1->vertex[2].y = y-model->sizey/2.f;
							quad1->vertex[2].z = z-model->sizez/2.f;

							// optimize quad
							node_t *node;
							for( i=0, node=quads.first; i<numquads-1; i++, node=node->next ) {
								quad2 = (polyquad_t *)node->element;
								if( quad1->side == quad2->side ) {
									if( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b ) {
										if( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z ) {
											if( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z ) {
												quad2->vertex[2].z++;
												quad2->vertex[3].z++;
												list_RemoveNode(currentNode);
												numquads--;
												polymodels[c].numfaces-=2;
												break;
											}
										}
									}
								}
							}
						}
					}
					if( newcolor != oldcolor || !buildingquad ) {
						if( newcolor != 255 ) {
							bool doit=FALSE;
							if( y==0 )
								doit=TRUE;
							else if( models[c]->data[index-indexdown[1]]==255 )
								doit=TRUE;
							if( doit ) {
								// start building a new quad
								buildingquad=TRUE;
								numquads++;
								polymodels[c].numfaces+=2;

								quad1 = (polyquad_t *) calloc(1,sizeof(polyquad_t));
								quad1->side = 3;
								quad1->vertex[0].x = x-model->sizex/2.f;
								quad1->vertex[0].y = y-model->sizey/2.f;
								quad1->vertex[0].z = z-model->sizez/2.f-1;
								quad1->vertex[3].x = x-model->sizex/2.f;
								quad1->vertex[3].y = y-model->sizey/2.f;
								quad1->vertex[3].z = z-model->sizez/2.f;
								quad1->r = models[c]->palette[models[c]->data[index]][0];
								quad1->g = models[c]->palette[models[c]->data[index]][1];
								quad1->b = models[c]->palette[models[c]->data[index]][2];

								node_t *newNode = list_AddNodeLast(&quads);
								newNode->element = quad1;
								newNode->deconstructor = &defaultDeconstructor;
								newNode->size = sizeof(polyquad_t);
							}
						}
					}
					oldcolor=newcolor;
				}
				if( buildingquad==TRUE ) {
					// add the last two vertices to the previous quad
					buildingquad=FALSE;
					node_t *currentNode = quads.last;
					quad1 = (polyquad_t *) currentNode->element;
					quad1->vertex[1].x = x-model->sizex/2.f;
					quad1->vertex[1].y = y-model->sizey/2.f;
					quad1->vertex[1].z = z-model->sizez/2.f-1;
					quad1->vertex[2].x = x-model->sizex/2.f;
					quad1->vertex[2].y = y-model->sizey/2.f;
					quad1->vertex[2].z = z-model->sizez/2.f;

					// optimize quad
					node_t *node;
					for( i=0, node=quads.first; i<numquads-1; i++, node=node->next ) {
						quad2 = (polyquad_t *)node->element;
						if( quad1->side == quad2->side ) {
							if( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b ) {
								if( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z ) {
									if( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z ) {
										quad2->vertex[2].z++;
										quad2->vertex[3].z++;
										list_RemoveNode(currentNode);
										numquads--;
										polymodels[c].numfaces-=2;
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
		for( z=models[c]->sizez-1; z>=0; z-- ) {
			for( y=0; y<models[c]->sizey; y++ ) {
				oldcolor=255;
				buildingquad=FALSE;
				for( x=0; x<models[c]->sizex; x++ ) {
					index = z+y*models[c]->sizez+x*models[c]->sizey*models[c]->sizez;
					newcolor = models[c]->data[index];
					if( buildingquad==TRUE ) {
						bool doit=FALSE;
						if( newcolor != oldcolor )
							doit=TRUE;
						else if( z<models[c]->sizez-1 )
							if( models[c]->data[index+indexdown[2]]>=0 && models[c]->data[index+indexdown[2]]<255 )
								doit=TRUE;
						if( doit ) {
							// add the last two vertices to the previous quad
							buildingquad=FALSE;

							node_t *currentNode = quads.last;
							quad1 = (polyquad_t *) currentNode->element;
							quad1->vertex[1].x = x-model->sizex/2.f;
							quad1->vertex[1].y = y-model->sizey/2.f;
							quad1->vertex[1].z = z-model->sizez/2.f;
							quad1->vertex[2].x = x-model->sizex/2.f;
							quad1->vertex[2].y = y-model->sizey/2.f+1;
							quad1->vertex[2].z = z-model->sizez/2.f;

							// optimize quad
							node_t *node;
							for( i=0, node=quads.first; i<numquads-1; i++, node=node->next ) {
								quad2 = (polyquad_t *)node->element;
								if( quad1->side == quad2->side ) {
									if( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b ) {
										if( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z ) {
											if( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z ) {
												quad2->vertex[2].y++;
												quad2->vertex[3].y++;
												list_RemoveNode(currentNode);
												numquads--;
												polymodels[c].numfaces-=2;
												break;
											}
										}
									}
								}
							}
						}
					}
					if( newcolor != oldcolor || !buildingquad ) {
						if( newcolor != 255 ) {
							bool doit=FALSE;
							if( z==models[c]->sizez-1 )
								doit=TRUE;
							else if( models[c]->data[index+indexdown[2]]==255 )
								doit=TRUE;
							if( doit ) {
								// start building a new quad
								buildingquad=TRUE;
								numquads++;
								polymodels[c].numfaces+=2;

								quad1 = (polyquad_t *) calloc(1,sizeof(polyquad_t));
								quad1->side = 4;
								quad1->vertex[0].x = x-model->sizex/2.f;
								quad1->vertex[0].y = y-model->sizey/2.f;
								quad1->vertex[0].z = z-model->sizez/2.f;
								quad1->vertex[3].x = x-model->sizex/2.f;
								quad1->vertex[3].y = y-model->sizey/2.f+1;
								quad1->vertex[3].z = z-model->sizez/2.f;
								quad1->r = models[c]->palette[models[c]->data[index]][0];
								quad1->g = models[c]->palette[models[c]->data[index]][1];
								quad1->b = models[c]->palette[models[c]->data[index]][2];

								node_t *newNode = list_AddNodeLast(&quads);
								newNode->element = quad1;
								newNode->deconstructor = &defaultDeconstructor;
								newNode->size = sizeof(polyquad_t);
							}
						}
					}
					oldcolor=newcolor;
				}
				if( buildingquad==TRUE ) {
					// add the last two vertices to the previous quad
					buildingquad=FALSE;

					node_t *currentNode = quads.last;
					quad1 = (polyquad_t *) currentNode->element;
					quad1->vertex[1].x = x-model->sizex/2.f;
					quad1->vertex[1].y = y-model->sizey/2.f;
					quad1->vertex[1].z = z-model->sizez/2.f;
					quad1->vertex[2].x = x-model->sizex/2.f;
					quad1->vertex[2].y = y-model->sizey/2.f+1;
					quad1->vertex[2].z = z-model->sizez/2.f;

					// optimize quad
					node_t *node;
					for( i=0, node=quads.first; i<numquads-1; i++, node=node->next ) {
						quad2 = (polyquad_t *)node->element;
						if( quad1->side == quad2->side ) {
							if( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b ) {
								if( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z ) {
									if( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z ) {
										quad2->vertex[2].y++;
										quad2->vertex[3].y++;
										list_RemoveNode(currentNode);
										numquads--;
										polymodels[c].numfaces-=2;
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
		for( z=0; z<models[c]->sizez; z++ ) {
			for( y=0; y<models[c]->sizey; y++ ) {
				oldcolor=255;
				buildingquad=FALSE;
				for( x=0; x<models[c]->sizex; x++ ) {
					index = z+y*models[c]->sizez+x*models[c]->sizey*models[c]->sizez;
					newcolor = models[c]->data[index];
					if( buildingquad==TRUE ) {
						bool doit=FALSE;
						if( newcolor != oldcolor )
							doit=TRUE;
						else if( z>0 )
							if( models[c]->data[index-indexdown[2]]>=0 && models[c]->data[index-indexdown[2]]<255 )
								doit=TRUE;
						if( doit ) {
							// add the last two vertices to the previous quad
							buildingquad=FALSE;

							node_t *currentNode = quads.last;
							quad1 = (polyquad_t *) currentNode->element;
							quad1->vertex[1].x = x-model->sizex/2.f;
							quad1->vertex[1].y = y-model->sizey/2.f+1;
							quad1->vertex[1].z = z-model->sizez/2.f-1;
							quad1->vertex[2].x = x-model->sizex/2.f;
							quad1->vertex[2].y = y-model->sizey/2.f;
							quad1->vertex[2].z = z-model->sizez/2.f-1;

							// optimize quad
							node_t *node;
							for( i=0, node=quads.first; i<numquads-1; i++, node=node->next ) {
								quad2 = (polyquad_t *)node->element;
								if( quad1->side == quad2->side ) {
									if( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b ) {
										if( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z ) {
											if( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z ) {
												quad2->vertex[0].y++;
												quad2->vertex[1].y++;
												list_RemoveNode(currentNode);
												numquads--;
												polymodels[c].numfaces-=2;
												break;
											}
										}
									}
								}
							}
						}
					}
					if( newcolor != oldcolor || !buildingquad ) {
						if( newcolor != 255 ) {
							bool doit=FALSE;
							if( z==0 )
								doit=TRUE;
							else if( models[c]->data[index-indexdown[2]]==255 )
								doit=TRUE;
							if( doit ) {
								// start building a new quad
								buildingquad=TRUE;
								numquads++;
								polymodels[c].numfaces+=2;

								quad1 = (polyquad_t *) calloc(1,sizeof(polyquad_t));
								quad1->side = 5;
								quad1->vertex[0].x = x-model->sizex/2.f;
								quad1->vertex[0].y = y-model->sizey/2.f+1;
								quad1->vertex[0].z = z-model->sizez/2.f-1;
								quad1->vertex[3].x = x-model->sizex/2.f;
								quad1->vertex[3].y = y-model->sizey/2.f;
								quad1->vertex[3].z = z-model->sizez/2.f-1;
								quad1->r = models[c]->palette[models[c]->data[index]][0];
								quad1->g = models[c]->palette[models[c]->data[index]][1];
								quad1->b = models[c]->palette[models[c]->data[index]][2];

								node_t *newNode = list_AddNodeLast(&quads);
								newNode->element = quad1;
								newNode->deconstructor = &defaultDeconstructor;
								newNode->size = sizeof(polyquad_t);
							}
						}
					}
					oldcolor=newcolor;
				}
				if( buildingquad==TRUE ) {
					// add the last two vertices to the previous quad
					buildingquad=FALSE;

					node_t *currentNode = quads.last;
					quad1 = (polyquad_t *) currentNode->element;
					quad1->vertex[1].x = x-model->sizex/2.f;
					quad1->vertex[1].y = y-model->sizey/2.f+1;
					quad1->vertex[1].z = z-model->sizez/2.f-1;
					quad1->vertex[2].x = x-model->sizex/2.f;
					quad1->vertex[2].y = y-model->sizey/2.f;
					quad1->vertex[2].z = z-model->sizez/2.f-1;

					// optimize quad
					node_t *node;
					for( i=0, node=quads.first; i<numquads-1; i++, node=node->next ) {
						quad2 = (polyquad_t *)node->element;
						if( quad1->side == quad2->side ) {
							if( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b ) {
								if( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z ) {
									if( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z ) {
										quad2->vertex[0].y++;
										quad2->vertex[1].y++;
										list_RemoveNode(currentNode);
										numquads--;
										polymodels[c].numfaces-=2;
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
		polymodels[c].faces = (polytriangle_t *) malloc(sizeof(polytriangle_t)*polymodels[c].numfaces);
		for( i=0; i<polymodels[c].numfaces; i++ ) {
			node_t *node = list_Node(&quads,i/2);
			polyquad_t *quad = (polyquad_t *)node->element;
			polymodels[c].faces[i].r = quad->r;
			polymodels[c].faces[i].g = quad->g;
			polymodels[c].faces[i].b = quad->b;
			if( i%2 ) {
				polymodels[c].faces[i].vertex[0] = quad->vertex[0];
				polymodels[c].faces[i].vertex[1] = quad->vertex[1];
				polymodels[c].faces[i].vertex[2] = quad->vertex[2];
			} else {
				polymodels[c].faces[i].vertex[0] = quad->vertex[0];
				polymodels[c].faces[i].vertex[1] = quad->vertex[2];
				polymodels[c].faces[i].vertex[2] = quad->vertex[3];
			}
		}

		// free up quads for the next model
		list_FreeAll(&quads);
	}

	// now store models into VBOs
	if( !disablevbos )
		generateVBOs();
}

/*-------------------------------------------------------------------------------

	generateVBOs

	generates VBOs/VAs from polymodel data

-------------------------------------------------------------------------------*/

void generateVBOs() {
	int i, c;

	for( c=0; c<nummodels; ++c ) {
		/*if( c>0 )
			break;*/
		GLfloat *points = (GLfloat *) malloc(sizeof(GLfloat)*9*polymodels[c].numfaces);
		GLfloat *colors = (GLfloat *) malloc(sizeof(GLfloat)*9*polymodels[c].numfaces);
		GLfloat *colors_shifted = (GLfloat *) malloc(sizeof(GLfloat)*9*polymodels[c].numfaces);
		for( i=0; i<polymodels[c].numfaces; i++ ) {
			points[i*9] = polymodels[c].faces[i].vertex[0].x;
			colors[i*9] = polymodels[c].faces[i].r/255.f;
			colors_shifted[i*9] = polymodels[c].faces[i].b/255.f;

			points[i*9+1] = -polymodels[c].faces[i].vertex[0].z;
			colors[i*9+1] = polymodels[c].faces[i].g/255.f;
			colors_shifted[i*9+1] = polymodels[c].faces[i].r/255.f;

			points[i*9+2] = polymodels[c].faces[i].vertex[0].y;
			colors[i*9+2] = polymodels[c].faces[i].b/255.f;
			colors_shifted[i*9+2] = polymodels[c].faces[i].g/255.f;

			points[i*9+3] = polymodels[c].faces[i].vertex[1].x;
			colors[i*9+3] = polymodels[c].faces[i].r/255.f;
			colors_shifted[i*9+3] = polymodels[c].faces[i].b/255.f;

			points[i*9+4] = -polymodels[c].faces[i].vertex[1].z;
			colors[i*9+4] = polymodels[c].faces[i].g/255.f;
			colors_shifted[i*9+4] = polymodels[c].faces[i].r/255.f;

			points[i*9+5] = polymodels[c].faces[i].vertex[1].y;
			colors[i*9+5] = polymodels[c].faces[i].b/255.f;
			colors_shifted[i*9+5] = polymodels[c].faces[i].g/255.f;

			points[i*9+6] = polymodels[c].faces[i].vertex[2].x;
			colors[i*9+6] = polymodels[c].faces[i].r/255.f;
			colors_shifted[i*9+6] = polymodels[c].faces[i].b/255.f;

			points[i*9+7] = -polymodels[c].faces[i].vertex[2].z;
			colors[i*9+7] = polymodels[c].faces[i].g/255.f;
			colors_shifted[i*9+7] = polymodels[c].faces[i].r/255.f;

			points[i*9+8] = polymodels[c].faces[i].vertex[2].y;
			colors[i*9+8] = polymodels[c].faces[i].b/255.f;
			colors_shifted[i*9+8] = polymodels[c].faces[i].g/255.f;
		}
		SDL_glGenVertexArrays(1, &polymodels[c].va);
		SDL_glGenBuffers(1, &polymodels[c].vbo);
		SDL_glGenBuffers(1, &polymodels[c].colors);
		SDL_glGenBuffers(1, &polymodels[c].colors_shifted);
		SDL_glBindVertexArray(polymodels[c].va);

		// vertex data
		SDL_glBindBuffer(GL_ARRAY_BUFFER, polymodels[c].vbo);
		SDL_glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*9*polymodels[c].numfaces, points, GL_STATIC_DRAW);
		SDL_glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		SDL_glEnableVertexAttribArray(0);

		// color data
		SDL_glBindBuffer(GL_ARRAY_BUFFER, polymodels[c].colors);
		SDL_glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*9*polymodels[c].numfaces, colors, GL_STATIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW
		SDL_glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glColorPointer(3, GL_FLOAT, 0, 0);
		SDL_glEnableVertexAttribArray(1);

		// shifted color data
		SDL_glBindBuffer(GL_ARRAY_BUFFER, polymodels[c].colors_shifted);
		SDL_glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*9*polymodels[c].numfaces, colors_shifted, GL_STATIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW
		SDL_glVertexAttribPointer((GLuint)2, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glColorPointer(3, GL_FLOAT, 0, 0);
		SDL_glEnableVertexAttribArray(2);

		free(points);
		free(colors);
		free(colors_shifted);
	}
}

/*-------------------------------------------------------------------------------

	deinitApp

	frees all memory consumed by the application and terminates the engine

-------------------------------------------------------------------------------*/

int deinitApp() {
	Uint32 c;

	// close engine
	printlog("closing engine...\n");
	printlog("removing engine timer...\n");
	if( timer )
		SDL_RemoveTimer(timer);
	printlog("freeing engine resources...\n");
	list_FreeAll(&button_l);
	list_FreeAll(&entitiesdeleted);
	if( fancyWindow_bmp )
		SDL_FreeSurface(fancyWindow_bmp);
	if( font8x8_bmp )
		SDL_FreeSurface(font8x8_bmp);
	if( font12x12_bmp )
		SDL_FreeSurface(font12x12_bmp);
	if( font16x16_bmp )
		SDL_FreeSurface(font16x16_bmp);
	if( ttf8 )
		TTF_CloseFont(ttf8);
	if( ttf12 )
		TTF_CloseFont(ttf12);
	if( ttf16 )
		TTF_CloseFont(ttf16);

	printlog("freeing map data...\n");
	if( map.entities != NULL ) {
		list_FreeAll(map.entities);
		free(map.entities);
	}
	list_FreeAll(&light_l);
	if( map.tiles != NULL )
		free(map.tiles);
	if( lightmap != NULL )
		free(lightmap);
	if( vismap != NULL )
		free(vismap);

	for( c=0; c<HASH_SIZE; c++ ) {
		list_FreeAll(&ttfTextHash[c]);
	}

	// free textures
	printlog("freeing textures...\n");
	if( tiles != NULL ) {
		for( c=0; c<numtiles; c++ ) {
			if( tiles[c] ) {
				SDL_FreeSurface(tiles[c]);
			}
		}
		free(tiles);
	}
	if( animatedtiles ) {
		free(animatedtiles);
		animatedtiles = NULL;
	}
	if( lavatiles ) {
		free(lavatiles);
		lavatiles = NULL;
	}

	// free sprites
	printlog("freeing sprites...\n");
	if( sprites != NULL ) {
		for( c=0; c<numsprites; c++ ) {
			if( sprites[c] ) {
				SDL_FreeSurface(sprites[c]);
			}
		}
		free(sprites);
	}

	// free models
	printlog("freeing models...\n");
	if( models != NULL ) {
		for( c=0; c<nummodels; c++ ) {
			if( models[c] != NULL ) {
				if( models[c]->data )
					free(models[c]->data);
				free(models[c]);
			}
		}
		free(models);
	}
	if( polymodels != NULL ) {
		for( c=0; c<nummodels; c++ ) {
			if( polymodels[c].faces ) {
				free(polymodels[c].faces);
			}
		}
		if( !disablevbos ) {
			for( c=0; c<nummodels; c++ ) {
				if( polymodels[c].vbo )
					SDL_glDeleteBuffers(1, &polymodels[c].vbo);
				if( polymodels[c].colors )
					SDL_glDeleteBuffers(1, &polymodels[c].colors);
				if( polymodels[c].va )
					SDL_glDeleteVertexArrays(1, &polymodels[c].va);
			}
		}
		free(polymodels);
	}

	// free sounds
#ifdef HAVE_FMOD
	printlog("freeing sounds...\n");
	if( sounds != NULL ) {
		for( c=0; c<numsounds; c++ ) {
			if(sounds[c] != NULL) {
				if (sounds[c] != NULL)
					FMOD_Sound_Release(sounds[c]); //Free the sound's FMOD sound.
				//free(sounds[c]); //Then free the sound itself.
			}
		}
		free(sounds); //Then free the sound array.
	}
#endif

	// delete opengl buffers
	if( allsurfaces != NULL )
		free(allsurfaces);
	if( texid != NULL ) {
		glDeleteTextures(MAXTEXTURES,texid);
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
	if( fmod_system ) {
		FMOD_System_Close(fmod_system);
		FMOD_System_Release(fmod_system);
		fmod_system = NULL;
	}
#endif
	if( screen ) {
		SDL_DestroyWindow(screen);
		screen = NULL;
	}
	if( renderer ) {
#ifdef APPLE
		SDL_DestroyRenderer(renderer);
#else
		SDL_GL_DeleteContext(renderer);
#endif
		renderer = NULL;
	}
	if( mainsurface ) {
		SDL_FreeSurface(mainsurface);
		mainsurface = NULL;
	}
	TTF_Quit();
	SDL_Quit();

	// free video and input buffers
	if( zbuffer != NULL )
		free(zbuffer);
	if( clickmap != NULL )
		free(clickmap);

	// shutdown steamworks
#ifdef STEAMWORKS
	if( steam_init ) {
		printlog("storing user stats to Steam...\n");
		SteamUserStats()->StoreStats();
		SteamAPI_Shutdown();
	}
#endif

	// free currently loaded language if any
	if( language ) {
		for( c=0; c<NUMLANGENTRIES; c++ ) {
			char *entry = language[c];
			if( entry ) {
				free(entry);
			}
		}
		free(language);
	}

	printlog("success\n");
	fclose(logfile);
	return 0;
}

/*-------------------------------------------------------------------------------

	initVideo

	Sets the SDL/openGL context using global video variables

-------------------------------------------------------------------------------*/

bool initVideo() {
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );
	SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
	//SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
	//SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 4 );

	printlog("setting display mode to %dx%d...\n",xres,yres);
	Uint32 flags = 0;
	if( fullscreen )
		flags |= SDL_WINDOW_FULLSCREEN;
	if( !game )
		flags |= SDL_WINDOW_RESIZABLE;
	if( !softwaremode )
		flags |= SDL_WINDOW_OPENGL;
#ifdef APPLE
	if( fullscreen ) {
		flags |= SDL_WINDOW_BORDERLESS;
	}
	SDL_DestroyWindow(screen);
	screen = NULL;
#endif
	int screen_width = xres;
	if (splitscreen)
		screen_width *= 2;
	if( !screen ) {
		if((screen=SDL_CreateWindow( window_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, yres, flags )) == NULL) {
			printlog("failed to set video mode.\n");
			return FALSE;
		}
	} else {
		SDL_SetWindowSize(screen,screen_width,yres);
		if( fullscreen )
			SDL_SetWindowFullscreen(screen,SDL_WINDOW_FULLSCREEN);
		else
			SDL_SetWindowFullscreen(screen,0);
	}
	if( !renderer ) {
#ifdef APPLE
		if((renderer=SDL_CreateRenderer(screen,-1,0)) == NULL) {
#else
		if((renderer=SDL_GL_CreateContext(screen)) == NULL) {
#endif
			printlog("failed to create SDL renderer. Reason: \"%s\"\n", SDL_GetError());
			printlog("You may need to update your video drivers.\n");
			return FALSE;
		}
	}
#ifndef APPLE
	SDL_GL_MakeCurrent(screen,renderer);
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
	if((mainsurface=SDL_CreateRGBSurface(0,xres,yres,32,rmask,gmask,bmask,amask)) == NULL) {
		printlog("failed to create main window surface.\n");
		return FALSE;
	}
	if( !softwaremode ) {
		glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		//glEnableClientState(GL_VERTEX_ARRAY);
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		glClearColor( 0, 0, 0, 0 );
	}
	if( SDL_SetWindowBrightness(screen,vidgamma)<0 ) {
		printlog("warning: failed to change gamma setting:\n%s\n",SDL_GetError());
		return FALSE;
	}
	printlog("display changed successfully.\n");
	return TRUE;
}

/*-------------------------------------------------------------------------------

	changeVideoMode

	In windows: saves the openGL context, sets the video mode, and restores
	the context
	otherwise: acts as a wrapper for initVideo

-------------------------------------------------------------------------------*/

bool changeVideoMode() {
	printlog("changing video mode.\n");
	int c;

	// delete old texture names (they're going away anyway)
	glDeleteTextures(MAXTEXTURES,texid);

	// delete vertex data
	if( !disablevbos ) {
		for( c=0; c<nummodels; c++ ) {
			SDL_glDeleteBuffers(1, &polymodels[c].vbo);
			SDL_glDeleteBuffers(1, &polymodels[c].colors);
			SDL_glDeleteVertexArrays(1, &polymodels[c].va);
		}
	}

	/*if( screen ) {
		SDL_DestroyWindow(screen);
		screen = NULL;
	}*/
	if( renderer ) {
#ifdef APPLE
		SDL_DestroyRenderer(renderer);
#else
		SDL_GL_DeleteContext(renderer);
#endif
		renderer = NULL;
	}
	if( mainsurface ) {
		SDL_FreeSurface(mainsurface);
		mainsurface = NULL;
	}

	// set video mode
	int result = initVideo();
	if( !result ) {
		xres = 960;
		yres = 600;
		fullscreen = 0;
		printlog("defaulting to safe video mode...\n");
		if( !initVideo() )
			return FALSE;
	}

	// now reload all textures
	glGenTextures(MAXTEXTURES,texid);
	for( c=1; c<imgref; c++ ) {
		glLoadTexture(allsurfaces[c],c);
	}

	// regenerate vbos
	if( !disablevbos )
		generateVBOs();

	// success
	return TRUE;
}
