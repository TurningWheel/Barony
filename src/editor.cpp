/*-------------------------------------------------------------------------------

	BARONY
	File: editor.cpp
	Desc: main code for the level editor

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "editor.hpp"
#include "entity.hpp"
#include "files.hpp"

#define EDITOR

//#include "player.hpp"

Entity* selectedEntity = nullptr;
Sint32 mousex = 0, mousey = 0;
Sint32 omousex = 0, omousey = 0;
Sint32 mousexrel = 0, mouseyrel = 0;

bool splitscreen = false; //Unused variable, for game only.

int game = 0;
// function prototypes
Uint32 timerCallback(Uint32 interval, void* param);
void handleEvents(void);
void mainLogic(void);

map_t copymap;

void closeNetworkInterfaces()
{
	//Because Dennis.
}

/*-------------------------------------------------------------------------------

	mainLogic

	handles time dependent procedures

-------------------------------------------------------------------------------*/

view_t camera_vel;

void mainLogic(void)
{
	// messages
	if ( messagetime > 0 )
	{
		messagetime--;
	}

	// basic editing functions are not available under these cases
	if ( subwindow || tilepalette || spritepalette )
	{
		return;
	}

	// scroll camera on minimap
	if ( mousestatus[SDL_BUTTON_LEFT] && toolbox )
	{
		if ( omousex >= xres - 120 && omousex < xres - 8 )
		{
			if ( omousey >= 24 && omousey < 136 )
			{
				camx = ((long)map.width << TEXTUREPOWER) * (real_t)(mousex - xres + 120) / 112 - xres / 2;
				camy = ((long)map.height << TEXTUREPOWER) * (real_t)(mousey - 24) / 112 - yres / 2;
			}
		}
	}

	// basic editor functions
	if ( mode3d == false )
	{
		camx += (keystatus[SDL_SCANCODE_RIGHT] - keystatus[SDL_SCANCODE_LEFT]) * TEXTURESIZE;
		camy += (keystatus[SDL_SCANCODE_DOWN] - keystatus[SDL_SCANCODE_UP]) * TEXTURESIZE;
	}
	else
	{
		// camera velocity
		camera_vel.x += cos(camera.ang) * (keystatus[SDL_SCANCODE_UP] - keystatus[SDL_SCANCODE_DOWN]) * .05;
		camera_vel.y += sin(camera.ang) * (keystatus[SDL_SCANCODE_UP] - keystatus[SDL_SCANCODE_DOWN]) * .05;
		camera_vel.z += (keystatus[SDL_SCANCODE_PAGEDOWN] - keystatus[SDL_SCANCODE_PAGEUP]) * .25;
		camera_vel.ang += (keystatus[SDL_SCANCODE_RIGHT] - keystatus[SDL_SCANCODE_LEFT]) * .04;

		// camera position
		camera.x += camera_vel.x;
		camera.y += camera_vel.y;
		camera.z += camera_vel.z;
		camera.ang += camera_vel.ang;
		while ( camera.ang >= PI * 2 )
		{
			camera.ang -= PI * 2;
		}
		while ( camera.ang < 0 )
		{
			camera.ang += PI * 2;
		}

		// friction
		camera_vel.x *= .65;
		camera_vel.y *= .65;
		camera_vel.z *= .65;
		camera_vel.ang *= .5;
	}
	if ( camx < -xres / 2 )
	{
		camx = -xres / 2;
	}
	if ( camx > ((long)map.width << TEXTUREPOWER) - ((long)xres / 2) )
	{
		camx = ((long)map.width << TEXTUREPOWER) - ((long)xres / 2);
	}
	if ( camy < -yres / 2 )
	{
		camy = -yres / 2;
	}
	if ( camy > ((long)map.height << TEXTUREPOWER) - ((long)yres / 2) )
	{
		camy = ((long)map.height << TEXTUREPOWER) - ((long)yres / 2);
	}

	if (scroll < 0)   // mousewheel up
	{
		drawlayer = std::min(drawlayer + 1, MAPLAYERS - 1);
		scroll = 0;
	}
	if (scroll > 0)   // mousewheel down
	{
		drawlayer = std::max(drawlayer - 1, 0);
		scroll = 0;
	}

	switch ( drawlayer )
	{
		case 0:
			strcpy(layerstatus, "FLOOR");
			break;
		case 1:
			strcpy(layerstatus, "WALLS");
			break;
		case 2:
			strcpy(layerstatus, "CEILING");
			break;
		default:
			strcpy(layerstatus, "UNKNOWN");
			break;
	}
}

/*-------------------------------------------------------------------------------

	handleButtons

	Draws buttons and processes clicks

-------------------------------------------------------------------------------*/

void handleButtons(void)
{
	node_t* node;
	node_t* nextnode;
	button_t* button;
	int w, h;

	// handle buttons
	for ( node = button_l.first; node != NULL; node = nextnode )
	{
		nextnode = node->next;
		button = (button_t*)node->element;
		if ( !subwindow && button->focused )
		{
			list_RemoveNode(button->node);
			continue;
		}
		if ( button->visible == 0 )
		{
			continue;    // invisible buttons are not processed
		}
		w = strlen(button->label) * 8;
		h = 8;
		if ( subwindow && !button->focused )
		{
			// unfocused buttons do not work when a subwindow is active
			drawWindow(button->x, button->y, button->x + button->sizex, button->y + button->sizey);
			printText(font8x8_bmp, button->x + (button->sizex - w) / 2, button->y + (button->sizey - h) / 2, button->label);
		}
		else
		{
			if ( omousex >= button->x && omousex < button->x + button->sizex )
			{
				if ( omousey >= button->y && omousey < button->y + button->sizey )
				{
					if ( button == butFile && menuVisible )
					{
						menuVisible = 1;
					}
					if ( button == butEdit && menuVisible )
					{
						menuVisible = 2;
					}
					if ( button == butView && menuVisible )
					{
						menuVisible = 3;
					}
					if ( button == butMap && menuVisible )
					{
						menuVisible = 4;
					}
					if ( button == butHelp && menuVisible )
					{
						menuVisible = 5;
					}
					if ( mousestatus[SDL_BUTTON_LEFT] )
					{
						button->pressed = true;
					}
				}
			}
			if ( button->pressed )
			{
				if ( omousex >= button->x && omousex < button->x + button->sizex && mousex >= button->x && mousex < button->x + button->sizex )
				{
					if ( omousey >= button->y && omousey < button->y + button->sizey && mousey >= button->y && mousey < button->y + button->sizey )
					{
						drawDepressed(button->x, button->y, button->x + button->sizex, button->y + button->sizey);
						printText(font8x8_bmp, button->x + (button->sizex - w) / 2, button->y + (button->sizey - h) / 2, button->label);
						if ( !mousestatus[SDL_BUTTON_LEFT] )   // releasing the mouse over the button
						{
							button->pressed = false;
							if ( button->action != NULL )
							{
								(*button->action)(button); // run the button's assigned action
								if ( !subwindow && button->focused )
								{
									list_RemoveNode(button->node);
								}
							}
						}
					}
					else
					{
						drawWindow(button->x, button->y, button->x + button->sizex, button->y + button->sizey);
						printText(font8x8_bmp, button->x + (button->sizex - w) / 2, button->y + (button->sizey - h) / 2, button->label);
						if ( !mousestatus[SDL_BUTTON_LEFT] )   // releasing the mouse over nothing
						{
							button->pressed = false;
						}
					}
				}
				else
				{
					drawWindow(button->x, button->y, button->x + button->sizex, button->y + button->sizey);
					printText(font8x8_bmp, button->x + (button->sizex - w) / 2, button->y + (button->sizey - h) / 2, button->label);
					if ( !mousestatus[SDL_BUTTON_LEFT] )   // releasing the mouse over nothing
					{
						button->pressed = false;
					}
				}
			}
			else
			{
				if ( (button != butFile || menuVisible != 1) && (button != butEdit || menuVisible != 2) && (button != butView || menuVisible != 3) && (button != butMap || menuVisible != 4) && (button != butHelp || menuVisible != 5) )
				{
					drawWindow(button->x, button->y, button->x + button->sizex, button->y + button->sizey);
					printText(font8x8_bmp, button->x + (button->sizex - w) / 2, button->y + (button->sizey - h) / 2, button->label);
				}
				else
				{
					drawDepressed(button->x, button->y, button->x + button->sizex, button->y + button->sizey);
					printText(font8x8_bmp, button->x + (button->sizex - w) / 2, button->y + (button->sizey - h) / 2, button->label);
				}
			}
		}
	}
}

/*-------------------------------------------------------------------------------

	handleEvents

	Handles all SDL events; receives input, updates gamestate, etc.

-------------------------------------------------------------------------------*/

void handleEvents(void)
{
	real_t d;
	int j;

	// calculate app rate
	t = clock();
	timesync = t - ot;
	ot = t;

	// calculate fps
	if ( timesync != 0 )
	{
		frameval[cycles & (AVERAGEFRAMES - 1)] = 1.0 / timesync;
	}
	else
	{
		frameval[cycles & (AVERAGEFRAMES - 1)] = 1.0;
	}
	d = frameval[0];
	for (j = 1; j < AVERAGEFRAMES; j++)
	{
		d += frameval[j];
	}
	fps = d / AVERAGEFRAMES * 1000;

	while ( SDL_PollEvent(&event) )   // poll SDL events
	{
		// Global events
		switch ( event.type )
		{
			case SDL_QUIT: // if SDL receives the shutdown signal
				buttonExit(NULL);
				break;
			case SDL_KEYDOWN: // if a key is pressed...
				if ( SDL_IsTextInputActive() )
				{
#ifdef APPLE
					if ( (event.key.keysym.sym == SDLK_DELETE || event.key.keysym.sym == SDLK_BACKSPACE) && strlen(inputstr) > 0 )
					{
						inputstr[strlen(inputstr) - 1] = 0;
						cursorflash = ticks;
					}
#else
					if ( event.key.keysym.sym == SDLK_BACKSPACE && strlen(inputstr) > 0 )
					{
						inputstr[strlen(inputstr) - 1] = 0;
						cursorflash = ticks;
					}
#endif
					else if ( event.key.keysym.sym == SDLK_c && SDL_GetModState()&KMOD_CTRL )
					{
						SDL_SetClipboardText(inputstr);
						cursorflash = ticks;
					}
					else if ( event.key.keysym.sym == SDLK_v && SDL_GetModState()&KMOD_CTRL )
					{
						strncpy(inputstr, SDL_GetClipboardText(), inputlen);
						cursorflash = ticks;
					}
				}
				lastkeypressed = event.key.keysym.sym;
				keystatus[event.key.keysym.scancode] = 1; // set this key's index to 1
				break;
			case SDL_KEYUP: // if a key is unpressed...
				keystatus[event.key.keysym.scancode] = 0; // set this key's index to 0
				break;
			case SDL_TEXTINPUT:
				if ( (event.text.text[0] != 'c' && event.text.text[0] != 'C') || !(SDL_GetModState()&KMOD_CTRL) )
				{
					if ( (event.text.text[0] != 'v' && event.text.text[0] != 'V') || !(SDL_GetModState()&KMOD_CTRL) )
					{
						strncat(inputstr, event.text.text, std::max<size_t>(0, inputlen - strlen(inputstr)));
						cursorflash = ticks;
					}
				}
				break;
			case SDL_MOUSEMOTION: // if the mouse is moved...
				mousex = event.motion.x;
				mousey = event.motion.y;
				mousexrel = event.motion.xrel;
				mouseyrel = event.motion.yrel;
				break;
			case SDL_MOUSEBUTTONDOWN: // if a mouse button is pressed...
				mousestatus[event.button.button] = 1; // set this mouse button to 1
				break;
			case SDL_MOUSEBUTTONUP: // if a mouse button is released...
				mousestatus[event.button.button] = 0; // set this mouse button to 0
				break;
			case SDL_MOUSEWHEEL:
				if ( event.wheel.y > 0 )
				{
					mousestatus[SDL_BUTTON_WHEELUP] = 1;
				}
				else if ( event.wheel.y < 0 )
				{
					mousestatus[SDL_BUTTON_WHEELDOWN] = 1;
				}
				if (mousestatus[4])
				{
					scroll = 1;
				}
				else if (mousestatus[5])
				{
					scroll = -1;
				}
				mousestatus[SDL_BUTTON_WHEELUP] = 0;
				mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
				break;
			case SDL_USEREVENT: // if the game timer elapses
				mainLogic();
				mousexrel = 0;
				mouseyrel = 0;
				break;
			case SDL_WINDOWEVENT: // if the window is resized
				if ( event.window.event == SDL_WINDOWEVENT_RESIZED )
				{
					if (fullscreen || ticks == 0)
					{
						break;
					}
					xres = std::max(event.window.data1, 100);
					yres = std::max(event.window.data2, 75);
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
					if (palette != NULL)
					{
						free(palette);
					}
					palette = (int*) malloc(sizeof(unsigned int) * xres * yres);
					if ( !changeVideoMode() )
					{
						printlog("critical error! Attempting to abort safely...\n");
						mainloop = 0;
					}
				}
				break;
		}
	}
	if (!mousestatus[SDL_BUTTON_LEFT])
	{
		omousex = mousex;
		omousey = mousey;
		ocamx = camx;
		ocamy = camy;
	}
}

/*-------------------------------------------------------------------------------

	timerCallback

	A callback function for the game timer which pushes an SDL event

-------------------------------------------------------------------------------*/

Uint32 timerCallback(Uint32 interval, void* param)
{
	SDL_Event event;
	SDL_UserEvent userevent;

	userevent.type = SDL_USEREVENT;
	userevent.code = 0;
	userevent.data1 = NULL;
	userevent.data2 = NULL;

	event.type = SDL_USEREVENT;
	event.user = userevent;

	ticks++;
	SDL_PushEvent(&event);
	return (interval);
}

/*-------------------------------------------------------------------------------

	editFill

	Fills a region of the map with a certain tile

-------------------------------------------------------------------------------*/

void editFill(int x, int y, int layer, int type)
{
	int repeat = 1;
	int fillspot;

	if ( type == map.tiles[layer + y * MAPLAYERS + x * MAPLAYERS * map.height] )
	{
		return;
	}

	fillspot = map.tiles[layer + y * MAPLAYERS + x * MAPLAYERS * map.height];
	map.tiles[layer + y * MAPLAYERS + x * MAPLAYERS * map.height] = type + numtiles;

	while ( repeat )
	{
		repeat = 0;
		for ( x = 0; x < map.width; x++ )
		{
			for ( y = 0; y < map.height; y++ )
			{
				if ( map.tiles[layer + y * MAPLAYERS + x * MAPLAYERS * map.height] == type + numtiles )
				{
					if ( x < map.width - 1 )
					{
						if ( map.tiles[layer + y * MAPLAYERS + (x + 1)*MAPLAYERS * map.height] == fillspot )
						{
							map.tiles[layer + y * MAPLAYERS + (x + 1)*MAPLAYERS * map.height] = type + numtiles;
							repeat = 1;
						}
					}
					if ( x > 0 )
					{
						if ( map.tiles[layer + y * MAPLAYERS + (x - 1)*MAPLAYERS * map.height] == fillspot )
						{
							map.tiles[layer + y * MAPLAYERS + (x - 1)*MAPLAYERS * map.height] = type + numtiles;
							repeat = 1;
						}
					}
					if ( y < map.height - 1 )
					{
						if ( map.tiles[layer + (y + 1)*MAPLAYERS + x * MAPLAYERS * map.height] == fillspot )
						{
							map.tiles[layer + (y + 1)*MAPLAYERS + x * MAPLAYERS * map.height] = type + numtiles;
							repeat = 1;
						}
					}
					if ( y > 0 )
					{
						if ( map.tiles[layer + (y - 1)*MAPLAYERS + x * MAPLAYERS * map.height] == fillspot )
						{
							map.tiles[layer + (y - 1)*MAPLAYERS + x * MAPLAYERS * map.height] = type + numtiles;
							repeat = 1;
						}
					}
				}
			}
		}
	}

	for ( x = 0; x < map.width; x++ )
	{
		for ( y = 0; y < map.height; y++ )
		{
			if ( map.tiles[layer + y * MAPLAYERS + x * MAPLAYERS * map.height] == type + numtiles )
			{
				map.tiles[layer + y * MAPLAYERS + x * MAPLAYERS * map.height] = type;
			}
		}
	}
}

/*-------------------------------------------------------------------------------

	makeUndo

	adds an undomap to the undolist

-------------------------------------------------------------------------------*/

#define MAXUNDOS 10

node_t* undospot = NULL;
node_t* redospot = NULL;
list_t undolist;
void makeUndo()
{
	node_t* node, *nextnode;

	// eliminate any undo nodes beyond the one we are currently on
	if ( undospot != NULL )
	{
		for ( node = undospot->next; node != NULL; node = nextnode )
		{
			nextnode = node->next;
			list_RemoveNode(node);
		}
	}
	else
	{
		if ( redospot )
		{
			list_FreeAll(&undolist);
		}
	}

	// copy all the current map data
	map_t* undomap = (map_t*) malloc(sizeof(map_t));
	strcpy(undomap->author, map.author);
	strcpy(undomap->name, map.name);
	undomap->width = map.width;
	undomap->height = map.height;
	undomap->tiles = (Sint32*) malloc(sizeof(Sint32) * undomap->width * undomap->height * MAPLAYERS);
	memcpy(undomap->tiles, map.tiles, sizeof(Sint32)*undomap->width * undomap->height * MAPLAYERS);
	undomap->entities = (list_t*) malloc(sizeof(list_t));
	undomap->entities->first = NULL;
	undomap->entities->last = NULL;
	for ( node = map.entities->first; node != NULL; node = node->next )
	{
		Entity* entity = newEntity(((Entity*)node->element)->sprite, 1, undomap->entities);
		entity->x = ((Entity*)node->element)->x;
		entity->y = ((Entity*)node->element)->y;
	}

	// add the new node to the undo list
	node = list_AddNodeLast(&undolist);
	node->element = undomap;
	node->deconstructor = &mapDeconstructor;
	if ( list_Size(&undolist) > MAXUNDOS + 1 )
	{
		list_RemoveNode(undolist.first);
	}
	undospot = node;
	redospot = NULL;
}

void clearUndos()
{
	list_FreeAll(&undolist);
	undospot = NULL;
	redospot = NULL;
}

/*-------------------------------------------------------------------------------

	undo() / redo()

	self explanatory

-------------------------------------------------------------------------------*/

void undo()
{
	node_t* node;

	if ( undospot == NULL )
	{
		return;
	}
	selectedEntity = NULL;
	if ( undospot == undolist.last )
	{
		node_t* tempnode = undospot;
		makeUndo();
		undospot = tempnode;
	}
	free(map.tiles);
	map_t* undomap = (map_t*)undospot->element;
	map.width = undomap->width;
	map.height = undomap->height;
	map.tiles = (Sint32*) malloc(sizeof(Sint32) * map.width * map.height * MAPLAYERS);
	memcpy(map.tiles, undomap->tiles, sizeof(Sint32)*undomap->width * undomap->height * MAPLAYERS);
	list_FreeAll(map.entities);
	for ( node = undomap->entities->first; node != NULL; node = node->next )
	{
		Entity* entity = newEntity(((Entity*)node->element)->sprite, 1, map.entities);
		entity->x = ((Entity*)node->element)->x;
		entity->y = ((Entity*)node->element)->y;
	}
	if ( redospot != NULL )
	{
		redospot = redospot->prev;
	}
	else
	{
		redospot = undospot->next;
	}
	undospot = undospot->prev;
}

void redo()
{
	node_t* node;

	if ( redospot == NULL )
	{
		return;
	}
	selectedEntity = NULL;
	free(map.tiles);
	map_t* undomap = (map_t*)redospot->element;
	map.width = undomap->width;
	map.height = undomap->height;
	map.tiles = (Sint32*) malloc(sizeof(Sint32) * map.width * map.height * MAPLAYERS);
	memcpy(map.tiles, undomap->tiles, sizeof(Sint32)*undomap->width * undomap->height * MAPLAYERS);
	list_FreeAll(map.entities);
	for ( node = undomap->entities->first; node != NULL; node = node->next )
	{
		Entity* entity = newEntity(((Entity*)node->element)->sprite, 1, map.entities);
		entity->x = ((Entity*)node->element)->x;
		entity->y = ((Entity*)node->element)->y;
	}
	if ( undospot != NULL )
	{
		undospot = undospot->next;
	}
	else
	{
		undospot = redospot->prev;
	}
	redospot = redospot->next;
}

void processCommandLine(int argc, char** argv)
{
	int c = 0;
	strcpy(datadir, "./");
	if ( argc > 1 )
	{
		for ( c = 1; c < argc; c++ )
		{
			if ( argv[c] != nullptr )
			{
				if ( !strncmp(argv[c], "-map=", 5) )
				{
					strcpy(maptoload, argv[c] + 5);
					loadingmap = true;
				}
				else if (!strncmp(argv[c], "-datadir=", 9))
				{
					strcpy(datadir, argv[c] + 9);
				}
			}
		}
	}
	printlog("Data path is %s", datadir);
}

/*-------------------------------------------------------------------------------

	main

	Initializes program resources, harbors main loop, and cleans up
	afterwords

-------------------------------------------------------------------------------*/

bool selectingspace = false;
int selectedarea_x1, selectedarea_x2;
int selectedarea_y1, selectedarea_y2;
bool selectedarea = false;
bool pasting = false;

#ifdef APPLE
#include <mach-o/dyld.h> //For _NSGetExecutablePath()
#endif

int main(int argc, char** argv)
{
#ifdef APPLE
	uint32_t buffsize = 4096;
	char binarypath[buffsize];
	int result = _NSGetExecutablePath(binarypath, &buffsize);
	if (result == 0)   //It worked.
	{
		printlog( "Binary path: %s\n", binarypath);
		char* last = strrchr(binarypath, '/');
		*last = '\0';
		char execpath[buffsize];
		strcpy(execpath, binarypath);
		//char* last = strrchr(execpath, '/');
		//strcat(execpath, '/');
		//strcat(execpath, "/../../../");
		printlog( "Chrooting to directory: %s\n", execpath);
		chdir(execpath);
		///Users/ciprian/barony/barony-sdl2-take2/barony.app/Contents/MacOS/barony
		chdir("..");
		chdir("..");
		chdir("..");
		chdir("barony.app/Contents/Resources");
		//chdir("..");
	}
	else
	{
		printlog( "Failed to get binary path. Program may not work correctly!\n");
	}
#endif
	button_t* button;
	node_t* node;
	node_t* nextnode;
	Entity* entity;
	SDL_Rect pos;
	int c;
	int x, y, z;
	int x2, y2;
	//char action[32];
	int oslidery = 0;
	light_t* light = NULL;
	bool savedundo = false;
	smoothlighting = true;

	processCommandLine(argc, argv);

	// load default language file (english)
	if ( loadLanguage("en") )
	{
		exit(1);
	}

	// initialize
	if ( (x = initApp("Barony Editor", fullscreen)) )
	{
		printlog("Critical error: %d\n", x);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Uh oh",
		                         "Barony Editor has encountered a critical error and cannot start.\n\n"
		                         "Please check the log.txt file in the game directory for additional info,\n"
		                         "or contact us through our website at http://www.baronygame.com/ for support.",
		                         screen);
		deinitApp();
		exit(x);
	}
	copymap.tiles = NULL;
	copymap.entities = NULL;
	undolist.first = NULL;
	undolist.last = NULL;

	// load cursors
	cursorArrow = SDL_GetCursor();
	cursorPencil = newCursor(cursor_pencil);
	cursorBrush = newCursor(cursor_brush);
	cursorSelect = cursorArrow;
	cursorFill = newCursor(cursor_fill);

	// instatiate a timer
	timer = SDL_AddTimer(1000 / TICKS_PER_SECOND, timerCallback, NULL);
	srand(time(NULL));

	// create an empty map
	map.width = 32;
	map.height = 24;
	map.entities = (list_t*) malloc(sizeof(list_t));
	map.entities->first = NULL;
	map.entities->last = NULL;
	map.tiles = (int*) malloc(sizeof(int) * map.width * map.height * MAPLAYERS);
	strcpy(map.name, "");
	strcpy(map.author, "");
	for ( z = 0; z < MAPLAYERS; z++ )
	{
		for ( y = 0; y < map.height; y++ )
		{
			for ( x = 0; x < map.width; x++ )
			{
				if (z == OBSTACLELAYER)
				{
					if (x == 0 || y == 0 || x == map.width - 1 || y == map.height - 1)
					{
						map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height] = 2;
					}
					else
					{
						map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height] = 0;
					}
				}
				else
				{
					map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height] = 1;
				}
			}
		}
	}
	vismap = (bool*) malloc(sizeof(bool) * map.width * map.height);
	lightmap = (int*) malloc(sizeof(Sint32) * map.width * map.height);
	for (c = 0; c < map.width * map.height; c++ )
	{
		lightmap[c] = 0;
	}

	// initialize camera position
	camera.x = 4;
	camera.y = 4;
	camera.z = 0;
	camera.ang = 0;
	camera.vang = 0;

	// initialize editor settings
	strcpy(layerstatus, "BACKGROUND");
	palette = (int*) malloc(sizeof(unsigned int) * xres * yres);

	// main interface
	button = butFile = newButton();
	strcpy(button->label, "File");
	button->x = 0;
	button->y = 0;
	button->sizex = 40;
	button->sizey = 16;
	button->action = &buttonFile;

	button = butEdit = newButton();
	strcpy(button->label, "Edit");
	button->x = 40;
	button->y = 0;
	button->sizex = 40;
	button->sizey = 16;
	button->action = &buttonEdit;

	button = butView = newButton();
	strcpy(button->label, "View");
	button->x = 80;
	button->y = 0;
	button->sizex = 40;
	button->sizey = 16;
	button->action = &buttonView;

	button = butMap = newButton();
	strcpy(button->label, "Map");
	button->x = 120;
	button->y = 0;
	button->sizex = 32;
	button->sizey = 16;
	button->action = &buttonMap;

	button = butHelp = newButton();
	strcpy(button->label, "Help");
	button->x = 152;
	button->y = 0;
	button->sizex = 40;
	button->sizey = 16;
	button->action = &buttonHelp;

	button = butX = newButton();
	strcpy(button->label, "X");
	button->x = xres - 16;
	button->y = 0;
	button->sizex = 16;
	button->sizey = 16;
	button->action = &buttonExit;
	button->visible = 0;

	button = but_ = newButton();
	strcpy(button->label, "_");
	button->x = xres - 32;
	button->y = 0;
	button->sizex = 16;
	button->sizey = 16;
	button->action = &buttonIconify;
	button->visible = 0;

	// toolbox
	button = butTilePalette = newButton();
	strcpy(button->label, "Palette ...");
	button->x = xres - 112;
	button->y = 152;
	button->sizex = 96;
	button->sizey = 16;
	button->action = &buttonTilePalette;

	button = butSprite = newButton();
	strcpy(button->label, "Sprite  ...");
	button->x = xres - 112;
	button->y = 168;
	button->sizex = 96;
	button->sizey = 16;
	button->action = &buttonSprite;

	button = butPoint = newButton();
	strcpy(button->label, "Point");
	button->x = xres - 96;
	button->y = 204;
	button->sizex = 64;
	button->sizey = 16;
	button->action = &buttonPoint;

	button = butBrush = newButton();
	strcpy(button->label, "Brush");
	button->x = xres - 96;
	button->y = 220;
	button->sizex = 64;
	button->sizey = 16;
	button->action = &buttonBrush;

	button = butSelect = newButton();
	strcpy(button->label, "Select");
	button->x = xres - 96;
	button->y = 236;
	button->sizex = 64;
	button->sizey = 16;
	button->action = &buttonSelect;

	button = butFill = newButton();
	strcpy(button->label, "Fill");
	button->x = xres - 96;
	button->y = 252;
	button->sizex = 64;
	button->sizey = 16;
	button->action = &buttonFill;

	// file menu
	butNew = button = newButton();
	strcpy(button->label, "New          Ctrl+N");
	button->x = 16;
	button->y = 16;
	button->sizex = 160;
	button->sizey = 16;
	button->action = &buttonNew;
	button->visible = 0;

	butOpen = button = newButton();
	strcpy(button->label, "Open ...     Ctrl+O");
	button->x = 16;
	button->y = 32;
	button->sizex = 160;
	button->sizey = 16;
	button->action = &buttonOpen;
	button->visible = 0;

	butSave = button = newButton();
	strcpy(button->label, "Save         Ctrl+S");
	button->x = 16;
	button->y = 48;
	button->sizex = 160;
	button->sizey = 16;
	button->action = &buttonSave;
	button->visible = 0;

	butSaveAs = button = newButton();
	strcpy(button->label, "Save As ...        ");
	button->x = 16;
	button->y = 64;
	button->sizex = 160;
	button->sizey = 16;
	button->action = &buttonSaveAs;
	button->visible = 0;

	butExit = button = newButton();
	strcpy(button->label, "Exit         Alt+F4");
	button->x = 16;
	button->y = 80;
	button->sizex = 160;
	button->sizey = 16;
	button->action = &buttonExit;
	button->visible = 0;

	// edit menu
	butCut = button = newButton();
	strcpy(button->label, "Cut         Ctrl+X");
	button->x = 56;
	button->y = 16;
	button->sizex = 152;
	button->sizey = 16;
	button->action = &buttonCut;
	button->visible = 0;

	butCopy = button = newButton();
	strcpy(button->label, "Copy        Ctrl+C");
	button->x = 56;
	button->y = 32;
	button->sizex = 152;
	button->sizey = 16;
	button->action = &buttonCopy;
	button->visible = 0;

	butPaste = button = newButton();
	strcpy(button->label, "Paste       Ctrl+V");
	button->x = 56;
	button->y = 48;
	button->sizex = 152;
	button->sizey = 16;
	button->action = &buttonPaste;
	button->visible = 0;

	butDelete = button = newButton();
	strcpy(button->label, "Delete      Del   ");
	button->x = 56;
	button->y = 64;
	button->sizex = 152;
	button->sizey = 16;
	button->action = &buttonDelete;
	button->visible = 0;

	butSelectAll = button = newButton();
	strcpy(button->label, "Select All  Ctrl+A");
	button->x = 56;
	button->y = 80;
	button->sizex = 152;
	button->sizey = 16;
	button->action = &buttonSelectAll;
	button->visible = 0;

	butUndo = button = newButton();
	strcpy(button->label, "Undo        Ctrl+Z");
	button->x = 56;
	button->y = 96;
	button->sizex = 152;
	button->sizey = 16;
	button->action = &buttonUndo;
	button->visible = 0;

	butRedo = button = newButton();
	strcpy(button->label, "Redo        Ctrl+Y");
	button->x = 56;
	button->y = 112;
	button->sizex = 152;
	button->sizey = 16;
	button->action = &buttonRedo;
	button->visible = 0;

	// view menu
	butStatusBar = button = newButton();
	strcpy(button->label, "Statusbar   Ctrl+I");
	button->x = 96;
	button->y = 16;
	button->sizex = 152;
	button->sizey = 16;
	button->action = &buttonStatusBar;
	button->visible = 0;

	butToolbox = button = newButton();
	strcpy(button->label, "Toolbox     Ctrl+T");
	button->x = 96;
	button->y = 32;
	button->sizex = 152;
	button->sizey = 16;
	button->action = &buttonToolbox;
	button->visible = 0;

	butAllLayers = button = newButton();
	strcpy(button->label, "All Layers  Ctrl+L");
	button->x = 96;
	button->y = 48;
	button->sizex = 152;
	button->sizey = 16;
	button->action = &buttonAllLayers;
	button->visible = 0;

	butViewSprites = button = newButton();
	strcpy(button->label, "Sprites     Ctrl+E");
	button->x = 96;
	button->y = 64;
	button->sizex = 152;
	button->sizey = 16;
	button->action = &buttonViewSprites;
	button->visible = 0;

	butGrid = button = newButton();
	strcpy(button->label, "Grid        Ctrl+G");
	button->x = 96;
	button->y = 80;
	button->sizex = 152;
	button->sizey = 16;
	button->action = &buttonGrid;
	button->visible = 0;

	but3DMode = button = newButton();
	strcpy(button->label, "3D Mode     Ctrl+F");
	button->x = 96;
	button->y = 96;
	button->sizex = 152;
	button->sizey = 16;
	button->action = &button3DMode;
	button->visible = 0;

	// map menu
	butAttributes = button = newButton();
	strcpy(button->label, "Attributes ...  Ctrl+M      ");
	button->x = 136;
	button->y = 16;
	button->sizex = 232;
	button->sizey = 16;
	button->action = &buttonAttributes;
	button->visible = 0;

	butClearMap = button = newButton();
	strcpy(button->label, "Clear Map       Ctrl+Shift+N");
	button->x = 136;
	button->y = 32;
	button->sizex = 232;
	button->sizey = 16;
	button->action = &buttonClearMap;
	button->visible = 0;

	// help menu
	butAbout = button = newButton();
	strcpy(button->label, "About  F1");
	button->x = 168;
	button->y = 16;
	button->sizex = 80;
	button->sizey = 16;
	button->action = &buttonAbout;
	button->visible = 0;

	if ( loadingmap )
	{
		if ( loadMap(maptoload, &map, map.entities) == -1 )
		{
			strcat(message, "Failed to open ");
			strcat(message, maptoload);
		}
		else
		{
			strcpy(filename, maptoload);
		}
	}

	// main loop
	printlog( "running main loop.\n");
	while (mainloop)
	{
		// game logic
		handleEvents();

		// move buttons
		/*if( !fullscreen ) {
			butX->visible = 0;
			but_->visible = 0;
		} else {
			butX->visible = 1;
			but_->visible = 1;
			butX->x = xres-16;
			but_->x = xres-32;
		}*/
		butTilePalette->x = xres - 112;
		butSprite->x = xres - 112;
		butPoint->x = xres - 96;
		butBrush->x = xres - 96;
		butSelect->x = xres - 96;
		butFill->x = xres - 96;

		if ( !spritepalette && !tilepalette )
		{
			allowediting = 1;
			if ( (omousex >= xres - 128 && toolbox) || omousey < 16 || (omousey >= yres - 16 && statusbar) || subwindow || menuVisible )
			{
				allowediting = 0;
			}
			if ( mode3d )
			{
				allowediting = 0;
			}
			if ( menuVisible == 1 )
			{
				if ((omousex > 16 + butNew->sizex || omousey > 96 || (omousey < 16 && omousex > 192)) && mousestatus[SDL_BUTTON_LEFT])
				{
					menuVisible = 0;
					menuDisappear = 1;
				}
			}
			else if ( menuVisible == 2 )
			{
				if ((omousex > 56 + butCut->sizex || omousex < 40 || omousey > 128 || (omousey < 16 && omousex > 192)) && mousestatus[SDL_BUTTON_LEFT])
				{
					menuVisible = 0;
					menuDisappear = 1;
				}
			}
			else if ( menuVisible == 3 )
			{
				if ((omousex > 96 + butToolbox->sizex || omousex < 80 || omousey > 112 || (omousey < 16 && omousex > 192)) && mousestatus[SDL_BUTTON_LEFT])
				{
					menuVisible = 0;
					menuDisappear = 1;
				}
			}
			else if ( menuVisible == 4 )
			{
				if ((omousex > 136 + butClearMap->sizex || omousex < 120 || omousey > 48 || (omousey < 16 && omousex > 192)) && mousestatus[SDL_BUTTON_LEFT])
				{
					menuVisible = 0;
					menuDisappear = 1;
				}
			}
			else if ( menuVisible == 5 )
			{
				if ((omousex > 168 + butAbout->sizex || omousex < 152 || omousey > 32 || (omousey < 16 && omousex > 192)) && mousestatus[SDL_BUTTON_LEFT])
				{
					menuVisible = 0;
					menuDisappear = 1;
				}
			}
			if ( !mousestatus[SDL_BUTTON_LEFT] )
			{
				menuDisappear = 0;
			}

			if ( allowediting && !menuDisappear )
			{
				// MAIN LEVEL EDITING
				drawx = (mousex + camx) >> TEXTUREPOWER;
				drawy = (mousey + camy) >> TEXTUREPOWER;
				odrawx = (omousex + ocamx) >> TEXTUREPOWER;
				odrawy = (omousey + ocamy) >> TEXTUREPOWER;

				// set the cursor
				switch ( selectedTool )
				{
					case 0:
						SDL_SetCursor(cursorPencil);
						break;
					case 1:
						SDL_SetCursor(cursorBrush);
						break;
					case 2:
						SDL_SetCursor(cursorSelect);
						break;
					case 3:
						SDL_SetCursor(cursorFill);
						break;
					default:
						SDL_SetCursor(cursorArrow);
						break;
				}

				// move entities
				if ( map.entities->first != NULL && viewsprites && allowediting )
				{
					for ( node = map.entities->first; node != NULL; node = nextnode )
					{
						nextnode = node->next;
						entity = (Entity*)node->element;
						if ( entity == selectedEntity )
						{
							if ( mousestatus[SDL_BUTTON_LEFT] )
							{
								mousestatus[SDL_BUTTON_LEFT] = 0;
								selectedEntity = NULL;
								break;
							}
							else if ( mousestatus[SDL_BUTTON_RIGHT] )
							{
								makeUndo();
								selectedEntity = newEntity(entity->sprite, 0, map.entities);
								selectedEntity->x = entity->x;
								selectedEntity->y = entity->y;
								mousestatus[SDL_BUTTON_RIGHT] = 0;
								break;
							}
							entity->x = (long)(drawx << 4);
							entity->y = (long)(drawy << 4);
						}
						else
						{
							if ( (omousex + camx) >> TEXTUREPOWER == entity->x / 16 && (omousey + camy) >> TEXTUREPOWER == entity->y / 16 )
							{
								if ( mousestatus[SDL_BUTTON_LEFT] )
								{
									// select sprite
									makeUndo();
									selectedEntity = entity;
									mousestatus[SDL_BUTTON_LEFT] = 0;
								}
								else if ( mousestatus[SDL_BUTTON_RIGHT] )
								{
									// duplicate sprite
									makeUndo();
									selectedEntity = newEntity(entity->sprite, 0, map.entities);
									selectedEntity->x = entity->x;
									selectedEntity->y = entity->y;
									mousestatus[SDL_BUTTON_RIGHT] = 0;
								}
							}
						}
					}
				}

				// modify world
				if ( mousestatus[SDL_BUTTON_LEFT] && selectedEntity == NULL )
				{
					if ( allowediting )
					{
						if ( !savedundo )
						{
							savedundo = true;
							makeUndo();
						}
						if ( !pasting )   // not pasting, normal editing mode
						{
							if ( selectedTool == 0 )   // point draw
							{
								if ( drawx >= 0 && drawx < map.width && drawy >= 0 && drawy < map.height )
								{
									map.tiles[drawlayer + drawy * MAPLAYERS + drawx * MAPLAYERS * map.height] = selectedTile;
								}
							}
							else if ( selectedTool == 1 )     // brush tool
							{
								for (x = drawx - 1; x <= drawx + 1; x++)
									for (y = drawy - 1; y <= drawy + 1; y++)
										if ( (x != drawx - 1 || y != drawy - 1) && (x != drawx + 1 || y != drawy - 1) && (x != drawx - 1 || y != drawy + 1) && (x != drawx + 1 || y != drawy + 1) )
											if ( x >= 0 && x < map.width && y >= 0 && y < map.height )
											{
												map.tiles[drawlayer + y * MAPLAYERS + x * MAPLAYERS * map.height] = selectedTile;
											}
							}
							else if ( selectedTool == 2 )     // select tool
							{
								if ( selectingspace == false )
								{
									if ( drawx >= 0 && drawy >= 0 && drawx < map.width && drawy < map.height )
									{
										selectingspace = true;
										selectedarea_x1 = drawx;
										selectedarea_x2 = drawx;
										selectedarea_y1 = drawy;
										selectedarea_y2 = drawy;
										selectedarea = true;
									}
									else
									{
										selectedarea = false;
									}
								}
								else
								{
									if ( drawx < odrawx )
									{
										selectedarea_x1 = std::min<unsigned int>(std::max(0, drawx), map.width - 1); //TODO: Why are int and unsigned int being compared?
										selectedarea_x2 = std::min<unsigned int>(std::max(0, odrawx), map.width - 1); //TODO: Why are int and unsigned int being compared?
									}
									else
									{
										selectedarea_x1 = std::min<unsigned int>(std::max(0, odrawx), map.width - 1); //TODO: Why are int and unsigned int being compared?
										selectedarea_x2 = std::min<unsigned int>(std::max(0, drawx), map.width - 1); //TODO: Why are int and unsigned int being compared?
									}
									if ( drawy < odrawy )
									{
										selectedarea_y1 = std::min<unsigned int>(std::max(0, drawy), map.height - 1); //TODO: Why are int and unsigned int being compared?
										selectedarea_y2 = std::min<unsigned int>(std::max(0, odrawy), map.height - 1); //TODO: Why are int and unsigned int being compared?
									}
									else
									{
										selectedarea_y1 = std::min<unsigned int>(std::max(0, odrawy), map.height - 1); //TODO: Why are int and unsigned int being compared?
										selectedarea_y2 = std::min<unsigned int>(std::max(0, drawy), map.height - 1); //TODO: Why are int and unsigned int being compared?
									}
								}
							}
							else if ( selectedTool == 3 )     // fill tool
							{
								if ( drawx >= 0 && drawx < map.width && drawy >= 0 && drawy < map.height )
								{
									editFill(drawx, drawy, drawlayer, selectedTile);
								}
							}
						}
						else
						{
							// pasting from copymap
							mousestatus[SDL_BUTTON_LEFT] = false;
							for ( x = 0; x < copymap.width; x++ )
							{
								for ( y = 0; y < copymap.height; y++ )
								{
									if ( drawx + x >= 0 && drawx + x < map.width && drawy + y >= 0 && drawy + y < map.height )
									{
										z = copymap.name[0] + y * MAPLAYERS + x * MAPLAYERS * copymap.height;
										if ( copymap.tiles[z] )
										{
											map.tiles[drawlayer + (drawy + y)*MAPLAYERS + (drawx + x)*MAPLAYERS * map.height] = copymap.tiles[z];
										}
									}
								}
							}
							pasting = false;
						}
					}
				}
				else if ( !mousestatus[SDL_BUTTON_LEFT] )
				{
					selectingspace = false;
					savedundo = false;
				}
				if ( mousestatus[SDL_BUTTON_RIGHT] && selectedEntity == NULL )
				{
					if ( selectedTool != 2 )
					{
						if ( drawx >= 0 && drawx < map.width && drawy >= 0 && drawy < map.height )
						{
							selectedTile = map.tiles[drawlayer + drawy * MAPLAYERS + drawx * MAPLAYERS * map.height];
						}
					}
					else
					{
						selectedarea = false;
					}
				}
			}
			else
			{
				SDL_SetCursor(cursorArrow);
			}

			// main drawing
			drawClearBuffers();
			if ( mode3d == false )
			{
				if ( alllayers )
					for (c = 0; c <= drawlayer; c++)
					{
						drawLayer(camx, camy, c, &map);
					}
				else
				{
					drawLayer(camx, camy, drawlayer, &map);
				}
				if ( pasting )
				{
					drawLayer(camx - (drawx << TEXTUREPOWER), camy - (drawy << TEXTUREPOWER), copymap.name[0], &copymap);
				}
				if ( selectedarea )
				{
					pos.x = (selectedarea_x1 << TEXTUREPOWER) - camx;
					pos.y = (selectedarea_y1 << TEXTUREPOWER) - camy;
					pos.w = (selectedarea_x2 - selectedarea_x1 + 1) << TEXTUREPOWER;
					pos.h = (selectedarea_y2 - selectedarea_y1 + 1) << TEXTUREPOWER;
					drawRect(&pos, SDL_MapRGB(mainsurface->format, 255, 255, 255), 127);
				}
				if ( viewsprites )
				{
					drawEntities2D(camx, camy);
				}
				if ( showgrid )
				{
					drawGrid(camx, camy);
				}
			}
			else
			{
				camera.winx = 0;
				camera.winy = 16;
				camera.winw = xres - 128;
				camera.winh = yres - 32;
				light = lightSphere(camera.x, camera.y, 16, 255);
				for ( node = map.entities->first; node != NULL; node = node->next )
				{
					entity = (Entity*)node->element;
					entity->flags[SPRITE] = true; // all entities rendered as SPRITES in the editor
					entity->x += 8;
					entity->y += 8;
				}
				raycast(&camera, REALCOLORS);
				glDrawWorld(&camera, REALCOLORS);
				//drawFloors(&camera);
				drawEntities3D(&camera, REALCOLORS);
				printTextFormatted(font8x8_bmp, 8, yres - 64, "x = %3.3f\ny = %3.3f\nz = %3.3f\nang = %3.3f\nfps = %3.1f", camera.x, camera.y, camera.z, camera.ang, fps);
				list_RemoveNode(light->node);
				for ( node = map.entities->first; node != NULL; node = node->next )
				{
					entity = (Entity*)node->element;
					entity->x -= 8;
					entity->y -= 8;
				}
			}

			// primary interface
			drawWindowFancy(0, 0, xres, 16);
			if ( toolbox )
			{
				if ( statusbar )
				{
					drawWindowFancy(xres - 128, 16, xres, yres - 16);
				}
				else
				{
					drawWindowFancy(xres - 128, 16, xres, yres);
				}
				drawEditormap(camx, camy);

				// draw selected tile / hovering tile
				pos.x = xres - 48;
				pos.y = 320;
				pos.w = 0;
				pos.h = 0;
				if ( selectedTile >= 0 && selectedTile < numtiles )
				{
					if ( tiles[selectedTile] != NULL )
					{
						drawImage(tiles[selectedTile], NULL, &pos);
					}
					else
					{
						drawImage(sprites[0], NULL, &pos);
					}
				}
				else
				{
					drawImage(sprites[0], NULL, &pos);
				}
				pos.x = xres - 48;
				pos.y = 360;
				pos.w = 0;
				pos.h = 0;
				if ( drawx >= 0 && drawx < map.width && drawy >= 0 && drawy < map.height )
				{
					c = map.tiles[drawlayer + drawy * MAPLAYERS + drawx * MAPLAYERS * map.height];
					if ( c >= 0 && c < numtiles )
					{
						if ( tiles[c] != NULL )
						{
							drawImage(tiles[c], NULL, &pos);
						}
						else
						{
							drawImage(sprites[0], NULL, &pos);
						}
					}
					else
					{
						drawImage(sprites[0], NULL, &pos);
					}
				}
				else
				{
					drawImage(sprites[0], NULL, &pos);
				}
				printText(font8x8_bmp, xres - 124, 332, "Selected:");
				printText(font8x8_bmp, xres - 124, 372, "   Above:");

				// print selected tool
				switch ( selectedTool )
				{
					case 0:
						printText(font8x8_bmp, xres - 84, 276, "POINT");
						break;
					case 1:
						printText(font8x8_bmp, xres - 84, 276, "BRUSH");
						break;
					case 2:
						printText(font8x8_bmp, xres - 88, 276, "SELECT");
						break;
					case 3:
						printText(font8x8_bmp, xres - 80, 276, "FILL");
						break;
				}
			}
			if ( statusbar )
			{
				drawWindowFancy(0, yres - 16, xres, yres);
				printTextFormatted(font8x8_bmp, 4, yres - 12, "X: %4d Y: %4d Z: %d %s", drawx, drawy, drawlayer + 1, layerstatus);
				if ( messagetime )
				{
					printText(font8x8_bmp, xres - 384, yres - 12, message);
				}
			}

			// handle main menus
			if ( menuVisible == 1 )
			{
				drawWindowFancy(0, 16, 16, 96);
				butNew->visible = 1;
				butOpen->visible = 1;
				butSave->visible = 1;
				butSaveAs->visible = 1;
				butExit->visible = 1;
			}
			else
			{
				butNew->visible = 0;
				butOpen->visible = 0;
				butSave->visible = 0;
				butSaveAs->visible = 0;
				butExit->visible = 0;
			}
			if ( menuVisible == 2 )
			{
				drawWindowFancy(40, 16, 56, 128);
				butPaste->visible = 1;
				butCut->visible = 1;
				butCopy->visible = 1;
				butDelete->visible = 1;
				butSelectAll->visible = 1;
				butUndo->visible = 1;
				butRedo->visible = 1;
			}
			else
			{
				butPaste->visible = 0;
				butCut->visible = 0;
				butCopy->visible = 0;
				butDelete->visible = 0;
				butSelectAll->visible = 0;
				butUndo->visible = 0;
				butRedo->visible = 0;
			}
			if ( menuVisible == 3 )
			{
				drawWindowFancy(80, 16, 96, 112);
				butToolbox->visible = 1;
				butStatusBar->visible = 1;
				butAllLayers->visible = 1;
				butViewSprites->visible = 1;
				butGrid->visible = 1;
				but3DMode->visible = 1;
				if ( statusbar )
				{
					printText(font8x8_bmp, 84, 20, "x");
				}
				if ( toolbox )
				{
					printText(font8x8_bmp, 84, 36, "x");
				}
				if ( alllayers )
				{
					printText(font8x8_bmp, 84, 52, "x");
				}
				if ( viewsprites )
				{
					printText(font8x8_bmp, 84, 68, "x");
				}
				if ( showgrid )
				{
					printText(font8x8_bmp, 84, 84, "x");
				}
				if ( mode3d )
				{
					printText(font8x8_bmp, 84, 100, "x");
				}
			}
			else
			{
				butToolbox->visible = 0;
				butStatusBar->visible = 0;
				butAllLayers->visible = 0;
				butViewSprites->visible = 0;
				butGrid->visible = 0;
				but3DMode->visible = 0;
			}
			if ( menuVisible == 4 )
			{
				drawWindowFancy(120, 16, 136, 48);
				butAttributes->visible = 1;
				butClearMap->visible = 1;
			}
			else
			{
				butAttributes->visible = 0;
				butClearMap->visible = 0;
			}
			if ( menuVisible == 5 )
			{
				drawWindowFancy(152, 16, 168, 32);
				butAbout->visible = 1;
			}
			else
			{
				butAbout->visible = 0;
			}

			// subwindows
			if ( subwindow )
			{
				drawWindowFancy(subx1, suby1, subx2, suby2);
				if ( subtext != NULL )
				{
					printText(font8x8_bmp, subx1 + 8, suby1 + 8, subtext);
				}

				// open and save windows
				if ( (openwindow || savewindow) && d_names != NULL )
				{
					drawDepressed(subx1 + 4, suby1 + 20, subx2 - 20, suby2 - 52);
					drawDepressed(subx2 - 20, suby1 + 20, subx2 - 4, suby2 - 52);
					slidersize = std::min<int>(((suby2 - 53) - (suby1 + 21)), ((suby2 - 53) - (suby1 + 21)) / ((real_t)d_names_length / 20)); //TODO: Why are int and real_t being compared?
					slidery = std::min(std::max(suby1 + 21, slidery), suby2 - 53 - slidersize);
					drawWindowFancy(subx2 - 19, slidery, subx2 - 5, slidery + slidersize);

					// directory list offset from slider
					y2 = ((real_t)(slidery - suby1 - 20) / ((suby2 - 52) - (suby1 + 20))) * d_names_length;
					if ( scroll )
					{
						slidery -= 8 * scroll;
						slidery = std::min(std::max(suby1 + 21, slidery), suby2 - 53 - slidersize);
						y2 = ((real_t)(slidery - suby1 - 20) / ((suby2 - 52) - (suby1 + 20))) * d_names_length;
						selectedFile = std::min<long unsigned int>(std::max(y2, selectedFile), std::min<long unsigned int>(d_names_length - 1, y2 + 19)); //TODO: Why are long unsigned int and int being compared? TWICE. On the same line.
						strcpy(filename, d_names[selectedFile]);
						inputstr = filename;
						scroll = 0;
					}
					if ( mousestatus[SDL_BUTTON_LEFT] && omousex >= subx2 - 20 && omousex < subx2 - 4 && omousey >= suby1 + 20 && omousey < suby2 - 52 )
					{
						slidery = oslidery + mousey - omousey;
						slidery = std::min(std::max(suby1 + 21, slidery), suby2 - 53 - slidersize);
						y2 = ((real_t)(slidery - suby1 - 20) / ((suby2 - 52) - (suby1 + 20))) * d_names_length;
						mclick = 1;
						selectedFile = std::min<long unsigned int>(std::max(y2, selectedFile), std::min<long unsigned int>(d_names_length - 1, y2 + 19)); //TODO: Why are long unsigned int and int being compared? TWICE. On the same line.
						strcpy(filename, d_names[selectedFile]);
						inputstr = filename;
					}
					else
					{
						oslidery = slidery;
					}

					// select a file
					if ( mousestatus[SDL_BUTTON_LEFT] )
					{
						if ( omousex >= subx1 + 8 && omousex < subx2 - 24 && omousey >= suby1 + 24 && omousey < suby2 - 56 )
						{
							selectedFile = y2 + ((omousey - suby1 - 24) >> 3);
							selectedFile = std::min<long unsigned int>(std::max(y2, selectedFile), std::min<long unsigned int>(d_names_length - 1, y2 + 19)); //TODO: Why are long unsigned int and int being compared? TWICE. On the same line.
							strcpy(filename, d_names[selectedFile]);
							inputstr = filename;
						}
					}
					pos.x = subx1 + 8;
					pos.y = suby1 + 24 + (selectedFile - y2) * 8;
					pos.w = subx2 - subx1 - 32;
					pos.h = 8;
					drawRect(&pos, SDL_MapRGB(mainsurface->format, 64, 64, 64), 255);

					// print all the files within the directory
					x = subx1 + 8;
					y = suby1 + 24;
					c = std::min<long unsigned int>(d_names_length, 20 + y2); //TODO: Why are long unsigned int and int being compared?
					for (z = y2; z < c; z++)
					{
						printText(font8x8_bmp, x, y, d_names[z]);
						y += 8;
					}

					// text box to enter file
					drawDepressed(subx1 + 4, suby2 - 48, subx2 - 68, suby2 - 32);
					printText(font8x8_bmp, subx1 + 8, suby2 - 44, filename);

					// enter filename
					if ( !SDL_IsTextInputActive() )
					{
						SDL_StartTextInput();
						inputstr = filename;
					}
					//strncpy(filename,inputstr,28);
					inputlen = 28;
					if ( (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
					{
						printText(font8x8_bmp, subx1 + 8 + strlen(filename) * 8, suby2 - 44, "\26");
					}
				}

				// new map and attributes windows
				if ( newwindow )
				{
					printText(font8x8_bmp, subx1 + 8, suby1 + 28, "Map name:");
					drawDepressed(subx1 + 4, suby1 + 40, subx2 - 4, suby1 + 56);
					printText(font8x8_bmp, subx1 + 8, suby1 + 44, nametext);
					printText(font8x8_bmp, subx1 + 8, suby1 + 64, "Author name:");
					drawDepressed(subx1 + 4, suby1 + 76, subx2 - 4, suby1 + 92);
					printText(font8x8_bmp, subx1 + 8, suby1 + 80, authortext);
					printText(font8x8_bmp, subx1 + 8, suby2 - 44, "Map width:");
					drawDepressed(subx1 + 104, suby2 - 48, subx1 + 168, suby2 - 32);
					printText(font8x8_bmp, subx1 + 108, suby2 - 44, widthtext);
					printText(font8x8_bmp, subx1 + 8, suby2 - 20, "Map height:");
					drawDepressed(subx1 + 104, suby2 - 24, subx1 + 168, suby2 - 8);
					printText(font8x8_bmp, subx1 + 108, suby2 - 20, heighttext);

					if ( keystatus[SDL_SCANCODE_TAB] )
					{
						keystatus[SDL_SCANCODE_TAB] = 0;
						cursorflash = ticks;
						editproperty++;
						if ( editproperty == 4 )
						{
							editproperty = 0;
						}
						switch ( editproperty )
						{
							case 0:
								inputstr = nametext;
								break;
							case 1:
								inputstr = authortext;
								break;
							case 2:
								inputstr = widthtext;
								break;
							case 3:
								inputstr = heighttext;
								break;
						}
					}

					// select a textbox
					if ( mousestatus[SDL_BUTTON_LEFT] )
					{
						if ( omousex >= subx1 + 4 && omousey >= suby1 + 40 && omousex < subx2 - 4 && omousey < suby1 + 56 )
						{
							inputstr = nametext;
							editproperty = 0;
							cursorflash = ticks;
						}
						if ( omousex >= subx1 + 4 && omousey >= suby1 + 76 && omousex < subx2 - 4 && omousey < suby1 + 92 )
						{
							inputstr = authortext;
							editproperty = 1;
							cursorflash = ticks;
						}
						if ( omousex >= subx1 + 104 && omousey >= suby2 - 48 && omousex < subx1 + 168 && omousey < suby2 - 32 )
						{
							inputstr = widthtext;
							editproperty = 2;
							cursorflash = ticks;
						}
						if ( omousex >= subx1 + 104 && omousey >= suby2 - 24 && omousex < subx1 + 168 && omousey < suby2 - 8 )
						{
							inputstr = heighttext;
							editproperty = 3;
							cursorflash = ticks;
						}
					}

					if ( editproperty == 0 )   // edit map name
					{
						if ( !SDL_IsTextInputActive() )
						{
							SDL_StartTextInput();
							inputstr = nametext;
						}
						//strncpy(nametext,inputstr,31);
						inputlen = 31;
						if ( (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
						{
							printText(font8x8_bmp, subx1 + 8 + strlen(nametext) * 8, suby1 + 44, "\26");
						}
					}
					if ( editproperty == 1 )   // edit author name
					{
						if ( !SDL_IsTextInputActive() )
						{
							SDL_StartTextInput();
							inputstr = authortext;
						}
						//strncpy(authortext,inputstr,31);
						inputlen = 31;
						if ( (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
						{
							printText(font8x8_bmp, subx1 + 8 + strlen(authortext) * 8, suby1 + 80, "\26");
						}
					}
					if ( editproperty == 2 )   // edit map width
					{
						if ( !SDL_IsTextInputActive() )
						{
							SDL_StartTextInput();
							inputstr = widthtext;
						}
						//strncpy(widthtext,inputstr,3);
						inputlen = 3;
						if ( (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
						{
							printText(font8x8_bmp, subx1 + 108 + strlen(widthtext) * 8, suby2 - 44, "\26");
						}
					}
					if ( editproperty == 3 )   // edit map height
					{
						if ( !SDL_IsTextInputActive() )
						{
							SDL_StartTextInput();
							inputstr = heighttext;
						}
						//strncpy(heighttext,inputstr,3);
						inputlen = 3;
						if ( (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
						{
							printText(font8x8_bmp, subx1 + 108 + strlen(heighttext) * 8, suby2 - 20, "\26");
						}
					}
				}
			}
			else
			{
				// handle hotkeys
				if ( keystatus[SDL_SCANCODE_LCTRL] || keystatus[SDL_SCANCODE_RCTRL] )
				{
					if ( keystatus[SDL_SCANCODE_N] && !keystatus[SDL_SCANCODE_LSHIFT] && !keystatus[SDL_SCANCODE_RSHIFT] )
					{
						keystatus[SDL_SCANCODE_N] = 0;
						buttonNew(NULL);
					}
					if ( keystatus[SDL_SCANCODE_S] )
					{
						keystatus[SDL_SCANCODE_S] = 0;
						buttonSave(NULL);
					}
					if ( keystatus[SDL_SCANCODE_O] )
					{
						keystatus[SDL_SCANCODE_O] = 0;
						buttonOpen(NULL);
					}
					if ( keystatus[SDL_SCANCODE_X] )
					{
						keystatus[SDL_SCANCODE_X] = 0;
						buttonCut(NULL);
					}
					if ( keystatus[SDL_SCANCODE_C] )
					{
						keystatus[SDL_SCANCODE_C] = 0;
						buttonCopy(NULL);
					}
					if ( keystatus[SDL_SCANCODE_V] )
					{
						keystatus[SDL_SCANCODE_V] = 0;
						buttonPaste(NULL);
					}
					if ( keystatus[SDL_SCANCODE_A] )
					{
						keystatus[SDL_SCANCODE_A] = 0;
						buttonSelectAll(NULL);
					}
					if ( keystatus[SDL_SCANCODE_Z] )
					{
						keystatus[SDL_SCANCODE_Z] = 0;
						buttonUndo(NULL);
					}
					if ( keystatus[SDL_SCANCODE_Y] )
					{
						keystatus[SDL_SCANCODE_Y] = 0;
						buttonRedo(NULL);
					}
					if ( keystatus[SDL_SCANCODE_G] )
					{
						keystatus[SDL_SCANCODE_G] = 0;
						buttonGrid(NULL);
					}
					if ( keystatus[SDL_SCANCODE_T] )
					{
						keystatus[SDL_SCANCODE_T] = 0;
						buttonToolbox(NULL);
					}
					if ( keystatus[SDL_SCANCODE_E] )
					{
						keystatus[SDL_SCANCODE_E] = 0;
						buttonViewSprites(NULL);
					}
					if ( keystatus[SDL_SCANCODE_L] )
					{
						keystatus[SDL_SCANCODE_L] = 0;
						buttonAllLayers(NULL);
					}
					if ( keystatus[SDL_SCANCODE_I] )
					{
						keystatus[SDL_SCANCODE_I] = 0;
						buttonStatusBar(NULL);
					}
					if ( keystatus[SDL_SCANCODE_F] )
					{
						keystatus[SDL_SCANCODE_F] = 0;
						button3DMode(NULL);
					}
					if ( keystatus[SDL_SCANCODE_M] )
					{
						keystatus[SDL_SCANCODE_M] = 0;
						buttonAttributes(NULL);
					}
					if ( keystatus[SDL_SCANCODE_LSHIFT] || keystatus[SDL_SCANCODE_RSHIFT] )
						if ( keystatus[SDL_SCANCODE_N] )
						{
							keystatus[SDL_SCANCODE_N] = 0;
							buttonClearMap(NULL);
						}
				}
				else
				{
					if ( keystatus[SDL_SCANCODE_S] )
					{
						keystatus[SDL_SCANCODE_S] = 0;
						spritepalette = 1;
					}
					if ( keystatus[SDL_SCANCODE_T] )
					{
						keystatus[SDL_SCANCODE_T] = 0;
						tilepalette = 1;
					}
				}
				if ( keystatus[SDL_SCANCODE_LALT] || keystatus[SDL_SCANCODE_RALT] )
				{
					if ( keystatus[SDL_SCANCODE_F] )
					{
						keystatus[SDL_SCANCODE_F] = 0;
						menuVisible = 1;
					}
					if ( keystatus[SDL_SCANCODE_E] )
					{
						keystatus[SDL_SCANCODE_E] = 0;
						menuVisible = 2;
					}
					if ( keystatus[SDL_SCANCODE_V] )
					{
						keystatus[SDL_SCANCODE_V] = 0;
						menuVisible = 3;
					}
					if ( keystatus[SDL_SCANCODE_M] )
					{
						keystatus[SDL_SCANCODE_M] = 0;
						menuVisible = 4;
					}
					if ( keystatus[SDL_SCANCODE_H] )
					{
						keystatus[SDL_SCANCODE_H] = 0;
						menuVisible = 5;
					}
					if ( keystatus[SDL_SCANCODE_F4] )
					{
						keystatus[SDL_SCANCODE_F4] = 0;
						buttonExit(NULL);
					}
				}
				if ( keystatus[SDL_SCANCODE_DELETE] )
				{
					keystatus[SDL_SCANCODE_DELETE] = 0;
					buttonDelete(NULL);
				}
				if ( keystatus[SDL_SCANCODE_F1] )
				{
					keystatus[SDL_SCANCODE_F1] = 0;
					buttonAbout(NULL);
				}
				if ( keystatus[SDL_SCANCODE_1] )
				{
					keystatus[SDL_SCANCODE_1] = 0;
					selectedTool = 0;
					selectedarea = false;
				}
				if ( keystatus[SDL_SCANCODE_2] )
				{
					keystatus[SDL_SCANCODE_2] = 0;
					selectedTool = 1;
					selectedarea = false;
				}
				if ( keystatus[SDL_SCANCODE_3] )
				{
					keystatus[SDL_SCANCODE_3] = 0;
					selectedTool = 2;
					selectedarea = false;
				}
				if ( keystatus[SDL_SCANCODE_4] )
				{
					keystatus[SDL_SCANCODE_4] = 0;
					selectedTool = 3;
					selectedarea = false;
				}
			}
			// process and draw buttons
			handleButtons();
		}

		if ( spritepalette )
		{
			x = 0;
			y = 0;
			z = 0;
			drawRect( NULL, SDL_MapRGB(mainsurface->format, 0, 0, 0), 255 ); // wipe screen
			for ( c = 0; c < xres * yres; c++ )
			{
				palette[c] = -1;
			}
			for ( c = 0; c < numsprites; c++ )
			{
				if ( sprites[c] != NULL )
				{
					pos.x = x;
					pos.y = y;
					pos.w = sprites[c]->w * 2;
					pos.h = sprites[c]->h * 2;
					drawImageScaled(sprites[c], NULL, &pos);
					for ( x2 = x; x2 < x + sprites[c]->w * 2; x2++ )
					{
						for ( y2 = y; y2 < y + sprites[c]->h * 2; y2++ )
						{
							if ( x2 < xres && y2 < yres )
							{
								palette[y2 + x2 * yres] = c;
							}
						}
					}
					x += sprites[c]->w * 2;
					z = std::max(z, sprites[c]->h * 2);
					if ( c < numsprites - 1 )
					{
						if ( sprites[c + 1] != NULL )
						{
							if ( x + sprites[c + 1]->w * 2 > xres )
							{
								x = 0;
								y += z;
							}
						}
						else
						{
							if ( x + sprites[0]->w * 2 > xres )
							{
								x = 0;
								y += z;
							}
						}
					}
				}
				else
				{
					pos.x = x;
					pos.y = y;
					pos.w = TEXTURESIZE;
					pos.h = TEXTURESIZE;
					drawImageScaled(sprites[0], NULL, &pos);
					x += sprites[0]->w * 2;
					z = std::max(z, sprites[0]->h * 2);
					if ( c < numsprites - 1 )
					{
						if ( sprites[c + 1] != NULL )
						{
							if ( x + sprites[c + 1]->w * 2 > xres )
							{
								x = 0;
								y += z;
							}
						}
						else
						{
							if ( x + sprites[0]->w * 2 > xres )
							{
								x = 0;
								y += z;
							}
						}
					}
				}
			}
			if (mousestatus[SDL_BUTTON_LEFT])
			{
				mclick = 1;
			}
			if (!mousestatus[SDL_BUTTON_LEFT] && mclick)
			{
				// create a new object
				if (palette[mousey + mousex * yres] >= 0)
				{
					entity = newEntity(palette[mousey + mousex * yres], 0, map.entities);
					selectedEntity = entity;
				}

				mclick = 0;
				spritepalette = 0;
			}
			if (keystatus[SDL_SCANCODE_ESCAPE])
			{
				mclick = 0;
				spritepalette = 0;
			}
			/*switch( palette[mousey+mousex*yres] ) {
				case 1:	strcpy(action,"PLAYER"); break;
				case 53:	strcpy(action,"PURPLEGEM"); break;
				case 37:	strcpy(action,"REDGEM"); break;
				case 74:
				case 75:	strcpy(action,"TROLL"); break;
				default:	strcpy(action,"STATIC"); break;
			}*/
			if ( palette[mousey + mousex * yres] >= 0 )
			{
				printTextFormatted(font8x8_bmp, 0, yres - 8, "Sprite index:%5d", palette[mousey + mousex * yres]);
			}
			else
			{
				printText(font8x8_bmp, 0, yres - 8, "Click to cancel");
			}
		}
		if ( tilepalette )
		{
			x = 0;
			y = 0;
			drawRect( NULL, SDL_MapRGB(mainsurface->format, 0, 0, 0), 255 ); // wipe screen
			for ( c = 0; c < xres * yres; c++ )
			{
				palette[c] = -1;
			}
			for ( c = 0; c < numtiles; c++ )
			{
				pos.x = x;
				pos.y = y;
				pos.w = TEXTURESIZE;
				pos.h = TEXTURESIZE;
				if ( tiles[c] != NULL )
				{
					drawImageScaled(tiles[c], NULL, &pos);
					for ( x2 = x; x2 < x + TEXTURESIZE; x2++ )
						for ( y2 = y; y2 < y + TEXTURESIZE; y2++ )
						{
							if ( x2 < xres && y2 < yres )
							{
								palette[y2 + x2 * yres] = c;
							}
						}
					x += TEXTURESIZE;
					if ( c < numtiles - 1 )
					{
						if ( x + TEXTURESIZE > xres )
						{
							x = 0;
							y += TEXTURESIZE;
						}
					}
				}
				else
				{
					drawImageScaled(sprites[0], NULL, &pos);
					x += TEXTURESIZE;
					if ( c < numtiles - 1 )
					{
						if ( x + TEXTURESIZE > xres )
						{
							x = 0;
							y += TEXTURESIZE;
						}
					}
				}
			}
			if (mousestatus[SDL_BUTTON_LEFT])
			{
				mclick = 1;
			}
			if (!mousestatus[SDL_BUTTON_LEFT] && mclick)
			{
				// select the tile under the mouse
				if (palette[mousey + mousex * yres] >= 0)
				{
					selectedTile = palette[mousey + mousex * yres];
				}
				mclick = 0;
				tilepalette = 0;
			}
			if (keystatus[SDL_SCANCODE_ESCAPE])
			{
				mclick = 0;
				tilepalette = 0;
			}
			if ( palette[mousey + mousex * yres] >= 0 )
			{
				printTextFormatted(font8x8_bmp, 0, yres - 8, "Tile index:%5d", palette[mousey + mousex * yres]);
			}
			else
			{
				printText(font8x8_bmp, 0, yres - 8, "Click to cancel");
			}
		}

		// flip screen
		GO_SwapBuffers(screen);
		cycles++;
	}

	// deinit
	SDL_SetCursor(cursorArrow);
	SDL_FreeCursor(cursorPencil);
	SDL_FreeCursor(cursorBrush);
	SDL_FreeCursor(cursorFill);
	if ( palette != NULL )
	{
		free(palette);
	}
	if ( copymap.tiles != NULL )
	{
		free(copymap.tiles);
	}
	list_FreeAll(&undolist);
	return deinitApp();
}
