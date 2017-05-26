/*-------------------------------------------------------------------------------

	BARONY
	File: editor.cpp
	Desc: main code for the level editor

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include "main.hpp"
#include "editor.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "player.hpp"
#include "interface/interface.hpp"

#define EDITOR

//#include "player.hpp"

Entity* selectedEntity = nullptr;
Entity* lastSelectedEntity = nullptr;
Sint32 mousex = 0, mousey = 0;
Sint32 omousex = 0, omousey = 0;
Sint32 mousexrel = 0, mouseyrel = 0;
int itemSelect = 0;
int itemSlotSelected = -1;
char itemName[128];
bool splitscreen = false; //Unused variable, for game only.
real_t prev_x = 0;
real_t prev_y = 0;
bool duplicatedSprite = false;
int game = 0;
// function prototypes
Uint32 timerCallback(Uint32 interval, void* param);
void handleEvents(void);
void mainLogic(void);

map_t copymap;

int errorMessage = 0;
int errorArr[8] =
{
	0, 0, 0, 0, 0, 0, 0, 0
};

char monsterPropertyNames[13][11] = 
{
	"Name:",
	"MAX HP:",
	"HP:",
	"MAX MP:",
	"MP:",
	"LEVEL:",
	"GOLD:",
	"STR:",
	"DEX:",
	"CON:",
	"INT:",
	"PER:",
	"CHR:"
};

char chestPropertyNames[3][40] =
{
	"Orientation: (0-3)",
	"Chest Type: (0-7)",
	"Locked Chance: (0-100%)"
};

char summonTrapPropertyNames[6][44] =
{
	"Monster To Spawn: (-1 to 32)",
	"Quantity Per Spawn: (1-9)",
	"Time Between Spawns: (1-999s)",
	"Amount of Spawn Instances: (1-99)",
	"Requires Power to Disable: (0-1)",
	"Chance to Stop Working Each Spawn: (0-100%)"
};

char itemPropertyNames[5][32] =
{
	"Item ID: (1-255)",
	"Status: (0-5)",
	"Blessing: (-9 to +9)",
	"Quantity: (1-9)",
	"Identified: (0-2)"
};

char monsterItemPropertyNames[6][32] =
{
	"Item ID: (0-255)",
	"Status: (0-5)",
	"Blessing: (-9 to +9)",
	"Quantity: (1-9)",
	"Identified: (0-2)",
	"Chance (1-100)"
};

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

	if ( errorMessage > 0 )
	{
		errorMessage--;
		if ( errorMessage == 0 )
		{
			for ( int i = 0; i < sizeof(errorArr) / sizeof(int); i++ )
				errorArr[i] = 0;
		}
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
					else if ( event.key.keysym.sym == SDLK_BACKQUOTE && strlen(inputstr) > 0 )
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
						if ( event.text.text[0] != '`' )
						{
							strncat(inputstr, event.text.text, std::max<size_t>(0, inputlen - strlen(inputstr)));
							cursorflash = ticks;
						}
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

		setSpriteAttributes(entity, (Entity*)node->element, (Entity*)node->element);
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

		setSpriteAttributes(entity, (Entity*)node->element, (Entity*)node->element);
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

		setSpriteAttributes(entity, (Entity*)node->element, (Entity*)node->element);
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

	Stat* spriteStats = NULL;

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

	butHoverText = button = newButton();
	strcpy(button->label, "Hover Text  Ctrl+H");
	button->x = 96;
	button->y = 112;
	button->sizex = 152;
	button->sizey = 16;
	button->action = &buttonHoverText;
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

	loadItems();

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
				if ((omousex > 96 + butToolbox->sizex || omousex < 80 || omousey > 128 || (omousey < 16 && omousex > 192)) && mousestatus[SDL_BUTTON_LEFT])
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
								if ( newwindow == 0 )
								{
									// if the entity moved from where it was picked up, or if the sprite was right click duplicated, store an undo.
									if ( selectedEntity->x / 16 != prev_x || selectedEntity->y / 16 != prev_y || duplicatedSprite )
									{
										duplicatedSprite = false;
										makeUndo();
									}
									else
									{
									}
								}
								mousestatus[SDL_BUTTON_LEFT] = 0;
								selectedEntity = NULL;
								break;
							}
							else if ( mousestatus[SDL_BUTTON_RIGHT] )
							{
								if ( newwindow == 0 )
								{
									// if previous sprite was duplicated and another right click is registered, store an undo.
									if ( duplicatedSprite )
									{
										makeUndo();
									}
									duplicatedSprite = true;
								}
								selectedEntity = newEntity(entity->sprite, 0, map.entities);
								
								setSpriteAttributes(selectedEntity, entity, lastSelectedEntity);

								lastSelectedEntity = selectedEntity;

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
									selectedEntity = entity;
									lastSelectedEntity = selectedEntity;
									prev_x = entity->x / 16;
									prev_y = entity->y / 16;
									mousestatus[SDL_BUTTON_LEFT] = 0;
									if ( newwindow == 0 && selectedEntity != NULL )
									{
										makeUndo();
									}
								}
								else if ( mousestatus[SDL_BUTTON_RIGHT] )
								{
									// duplicate sprite
									duplicatedSprite = true;
									if ( newwindow == 0 )
									{
										makeUndo();
									}
									selectedEntity = newEntity(entity->sprite, 0, map.entities);
									lastSelectedEntity = selectedEntity;

									setSpriteAttributes(selectedEntity, entity, entity);

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
				drawWindowFancy(80, 16, 96, 128);
				butToolbox->visible = 1;
				butStatusBar->visible = 1;
				butAllLayers->visible = 1;
				butHoverText->visible = 1;
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
				if ( hovertext )
				{
					printText(font8x8_bmp, 84, 116, "x");
				}
			}
			else
			{
				butToolbox->visible = 0;
				butStatusBar->visible = 0;
				butAllLayers->visible = 0;
				butHoverText->visible = 0;
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
				if ( newwindow == 1 )
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
				else if ( newwindow == 2 ) 
				{
					if ( selectedEntity != NULL ) 
					{
						spriteStats = selectedEntity->getStats();
						if ( spriteStats != nullptr )
						{
							int numProperties = sizeof(monsterPropertyNames) / sizeof(monsterPropertyNames[0]); //find number of entries in property list
							const int lenProperties = sizeof(monsterPropertyNames[0]) / sizeof(char); //find length of entry in property list
							int spacing = 20; // px between each item in the list.
							int pad_y1 = suby1 + 28; // 28 px spacing from subwindow start.
							int pad_x1 = subx1 + 8; // 8px spacing from subwindow start.
							int pad_x2 = 64;
							int pad_y2 = suby1 + 28 + 2 * spacing;
							int pad_x3 = 44; //property field width
							int pad_x4;
							Uint32 color = SDL_MapRGB(mainsurface->format, 255, 255, 255);
							char tmpPropertyName[lenProperties] = "";
							for ( int i = 0; i < numProperties; i++ )
							{
								strcpy(tmpPropertyName, monsterPropertyNames[i]);
								pad_y1 = suby1 + 28 + i * spacing;

								// value of 0 is name field, should be longer
								if ( i == 0 )
								{
									drawDepressed(pad_x1 - 4, pad_y1 + 16 - 4, subx2 - 4, pad_y1 + 28);
									// print values on top of boxes
									printTextFormattedColor(font8x8_bmp, pad_x1, pad_y1, color, tmpPropertyName);
									printText(font8x8_bmp, pad_x1, pad_y1 + 16, spriteProperties[i]);
								}
								else
								{
									pad_y1 += spacing;
									if ( i < 7 )
									{
										if ( i < 3 ) //hp
										{
											color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
										}
										else if ( i < 5 ) //mp
										{
											color = SDL_MapRGB(mainsurface->format, 0, 255, 228);
										}
										else if ( i == 5 ) //level
										{
											color = SDL_MapRGB(mainsurface->format, 255, 192, 0);
										}
										else if ( i == 6 ) //gold
										{
											color = SDL_MapRGB(mainsurface->format, 255, 192, 0);
										}
										drawDepressed(pad_x1 + pad_x2 - 4, pad_y1 - 4, pad_x1 + pad_x2 + pad_x3 - 4, pad_y1 + 16 - 4);
										// draw another box side by side, spaced by pad_x3 + 16
										drawDepressed(pad_x1 + pad_x2 - 4 + (pad_x3 + 16), pad_y1 - 4, pad_x1 + pad_x2 + pad_x3 - 4 + (pad_x3 + 16), pad_y1 + 16 - 4);
										// print values on top of boxes
										// print property name
										printTextFormattedColor(font8x8_bmp, pad_x1, pad_y1, color, tmpPropertyName);
										// print dash between boxes
										printTextFormattedColor(font8x8_bmp, pad_x1 + pad_x2 - 4 + (pad_x3 + 4), pad_y1, color, "-");
										// print left text
										printText(font8x8_bmp, pad_x1 + pad_x2, pad_y1, spriteProperties[i]);
										// print right text
										printText(font8x8_bmp, pad_x1 + pad_x2 + (pad_x3 + 16), pad_y1, spriteProperties[i + 12]);
									}
									else
									{
										color = SDL_MapRGB(mainsurface->format, 255, 255, 255);
										pad_y1 += spacing + 10;
										drawDepressed(pad_x1 + pad_x2 - 4, pad_y1 - 4, pad_x1 + pad_x2 + pad_x3 - 4, pad_y1 + 16 - 4);
										// draw another box side by side, spaced by pad_x3 + 16
										drawDepressed(pad_x1 + pad_x2 - 4 + (pad_x3 + 16), pad_y1 - 4, pad_x1 + pad_x2 + pad_x3 - 4 + (pad_x3 + 16), pad_y1 + 16 - 4);
										// print values on top of boxes
										// print property name
										printTextFormattedColor(font8x8_bmp, pad_x1, pad_y1, color, tmpPropertyName);
										// print dash between boxes
										printTextFormattedColor(font8x8_bmp, pad_x1 + pad_x2 - 4 + (pad_x3 + 4), pad_y1, color, "-");
										// print left text
										printText(font8x8_bmp, pad_x1 + pad_x2, pad_y1, spriteProperties[i]);
										// print right text
										printText(font8x8_bmp, pad_x1 + pad_x2 + (pad_x3 + 16), pad_y1, spriteProperties[i + 12]);
									}
								}
							}

							
							// Cycle properties with TAB.
							if ( keystatus[SDL_SCANCODE_TAB] )
							{
								keystatus[SDL_SCANCODE_TAB] = 0;
								cursorflash = ticks;
								editproperty++;
								if ( editproperty == numProperties * 2 - 1 )
								{
									// limit of properties is twice the vertical count, minus 1 for name (every property has a random component)
									editproperty = 0;
								}
								
								inputstr = spriteProperties[editproperty];
							}
							// TODO - add escape and return key functionality
							/*
							if ( keystatus[SDL_SCANCODE_ESCAPE] )
							{
								keystatus[SDL_SCANCODE_ESCAPE] = 0;
								buttonCloseSpriteSubwindow(NULL);
							}
							if ( keystatus[SDL_SCANCODE_RETURN] )
							{
								keystatus[SDL_SCANCODE_RETURN] = 0;
								buttonSpritePropertiesConfirm(NULL);
							}
							*/
							// select a textbox
							if ( mousestatus[SDL_BUTTON_LEFT] )
							{
								for ( int i = 0; i < numProperties; i++ )
								{
									pad_y1 = suby1 + 28 + i * spacing;
									if ( i == 0 )
									{
										if ( omousex >= pad_x1 - 4 && omousey >= pad_y1 + 16 - 4 && omousex < subx2 - 4 && omousey < pad_y1 + 32 - 4 )
										{
											inputstr = spriteProperties[i];
											editproperty = i;
											cursorflash = ticks;
										}
									}
									else
									{
										pad_y1 += spacing;
										if ( i < 7 )
										{
											// check if mouse is in left property box
											if ( omousex >= pad_x1 + pad_x2 - 4 && omousey >= pad_y1 - 4 && omousex < pad_x1 + pad_x2 + pad_x3 - 4 && omousey < pad_y1 + 16 - 4 )
											{
												inputstr = spriteProperties[i];
												editproperty = i;
												cursorflash = ticks;
											}
											// check if mouse is in right property box (offset from above by pad_x3 + 16)
											else if ( omousex >= pad_x1 + pad_x2 - 4 + (pad_x3 + 16) && omousey >= pad_y1 - 4 && omousex < pad_x1 + pad_x2 + pad_x3 - 4 + (pad_x3 + 16) && omousey < pad_y1 + 16 - 4 )
											{
												inputstr = spriteProperties[i + 12];
												editproperty = i + 12;
												cursorflash = ticks;
											}
										}
										else
										{
											pad_y1 += spacing + 10;
											// check if mouse is in left property box
											if ( omousex >= pad_x1 + pad_x2 - 4 && omousey >= pad_y1 - 4 && omousex < pad_x1 + pad_x2 + pad_x3 - 4 && omousey < pad_y1 + 16 - 4 )
											{
												inputstr = spriteProperties[i];
												editproperty = i;
												cursorflash = ticks;
											}
											// check if mouse is in right property box (offset from above by pad_x3 + 16)
											else if ( omousex >= pad_x1 + pad_x2 - 4 + (pad_x3 + 16) && omousey >= pad_y1 - 4 && omousex < pad_x1 + pad_x2 + pad_x3 - 4 + (pad_x3 + 16) && omousey < pad_y1 + 16 - 4 )
											{
												inputstr = spriteProperties[i + 12];
												editproperty = i + 12;
												cursorflash = ticks;
											}
										}
									}
								}

							}

							//items for monster
							pad_y2 = suby1 + 28 + 2 * spacing;
							pad_x3 = 40;
							pad_x4 = subx2 - 112;
							printTextFormattedColor(font8x8_bmp, pad_x4 - 8, pad_y2, color, " Helm");
							
							//pad_y2 += spacing * 2 - 16;
							pad_y2 += spacing * 2;
							printTextFormattedColor(font8x8_bmp, pad_x4 - 8, pad_y2, color, "Amulet");

							//pad_x4 += 64 * 2;
							pad_y2 += spacing * 2;
							printTextFormattedColor(font8x8_bmp, pad_x4 - 4, pad_y2, color, "Armor");

							//pad_x4 -= 64;
							//pad_y2 += spacing * 2 - 16;
							pad_y2 += spacing * 2;
							printTextFormattedColor(font8x8_bmp, pad_x4 - 8 - 4, pad_y2, color, " Boots");

							pad_y2 = suby1 + 28 + 2 * spacing;
							pad_y2 += 16;
							pad_x4 -= 64;
							printTextFormattedColor(font8x8_bmp, pad_x4 - 8 - 4, pad_y2, color, " Cloak");

							pad_x4 += 64 * 2;
							printTextFormattedColor(font8x8_bmp, pad_x4 - 8, pad_y2, color, " Mask");

							pad_x4 -= 64 * 2;
							pad_y2 += spacing * 2;
							printTextFormattedColor(font8x8_bmp, pad_x4 - 8, pad_y2, color, "Weapon");

							pad_x4 += 64 * 2;
							printTextFormattedColor(font8x8_bmp, pad_x4 - 8, pad_y2, color, "Shield");

							pad_x4 -= 64 * 2;
							pad_y2 += spacing * 2;
							printTextFormattedColor(font8x8_bmp, pad_x4 - 8, pad_y2, color, " Ring");

							pad_x4 += 64 * 2;
							printTextFormattedColor(font8x8_bmp, pad_x4 - 8, pad_y2, color, "Gloves");

							pad_x4 -= 64 * 2;

							pad_y2 += 32 + spacing * 2;
							printTextFormattedColor(font8x8_bmp, pad_x4 - 8, pad_y2, color, "Inventory");


							if ( editproperty < numProperties * 2 - 1 )   // edit property values
							{
								// limit of properties is twice the vertical count, minus 1 for name  (every property has a random component)
								if ( !SDL_IsTextInputActive() )
								{
									SDL_StartTextInput();
									inputstr = spriteProperties[0];
								}
								//strncpy(nametext,inputstr,31);

								// value of 0 is the name field, else the input is a number
								if ( editproperty == 0 )
								{
									inputlen = 31;
								}
								else
								{
									inputlen = 4;
								}
								if ( (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
								{
									pad_y1 = suby1 + 28 + editproperty * spacing;

									if ( editproperty == 0 )
									{
										printText(font8x8_bmp, pad_x1 + strlen(spriteProperties[editproperty]) * 8, pad_y1 + 16, "\26");
									}
									else if ( editproperty < 7 )
									{
										pad_y1 += spacing;
										// left box
										printText(font8x8_bmp, pad_x1 + pad_x2 + strlen(spriteProperties[editproperty]) * 8, pad_y1, "\26");
									}
									else if ( editproperty < 13 )
									{
										pad_y1 += spacing;
										pad_y1 += spacing + 10;
										// left box
										printText(font8x8_bmp, pad_x1 + pad_x2 + strlen(spriteProperties[editproperty]) * 8, pad_y1, "\26");
									}
									else if ( editproperty < 19 )
									{
										pad_y1 = suby1 + 28 + (editproperty - 12) * spacing;
										pad_y1 += spacing;
										// right box
										printText(font8x8_bmp, pad_x1 + pad_x2 + (pad_x3 + 20) + strlen(spriteProperties[editproperty]) * 8, pad_y1, "\26");
									}
									else if ( editproperty < 25 )
									{
										pad_y1 = suby1 + 28 + (editproperty - 12) * spacing;
										pad_y1 += spacing;
										pad_y1 += spacing + 10;
										// right box
										printText(font8x8_bmp, pad_x1 + pad_x2 + (pad_x3 + 20) + strlen(spriteProperties[editproperty]) * 8, pad_y1, "\26");
									}
								}
							}
						}
					}
				}
				else if ( newwindow == 3 )
				{
					if ( selectedEntity != NULL )
					{
						int numProperties = sizeof(chestPropertyNames) / sizeof(chestPropertyNames[0]); //find number of entries in property list
						const int lenProperties = sizeof(chestPropertyNames[0]) / sizeof(char); //find length of entry in property list
						int spacing = 36; // 36 px between each item in the list.
						int pad_y1 = suby1 + 28; // 28 px spacing from subwindow start.
						int pad_x1 = subx1 + 8; // 8px spacing from subwindow start.
						int pad_x2 = 64;
						int pad_x3 = pad_x1 + pad_x2 + 8;
						int pad_y2 = 0;
						char tmpPropertyName[lenProperties] = "";
						Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
						Uint32 colorRandom = SDL_MapRGB(mainsurface->format, 0, 168, 255);

						for ( int i = 0; i < numProperties; i++ )
						{
							int propertyInt = atoi(spriteProperties[i]);

							strcpy(tmpPropertyName, chestPropertyNames[i]);
							pad_y1 = suby1 + 28 + i * spacing;
							pad_y2 = suby1 + 44 + i * spacing;
							// box outlines then text
							drawDepressed(pad_x1 - 4, suby1 + 40 + i * spacing, pad_x1 - 4 + pad_x2, suby1 + 56 + i * spacing);
							// print values on top of boxes
							printText(font8x8_bmp, pad_x1, suby1 + 44 + i * spacing, spriteProperties[i]);
							printText(font8x8_bmp, pad_x1, pad_y1, tmpPropertyName);

							if ( errorArr[i] != 1 )
							{
								if ( i == 0 )
								{
									if ( propertyInt > 3 || propertyInt < 0 )
									{
										errorMessage = 60;
										errorArr[i] = 1;
										snprintf(spriteProperties[i], sizeof(spriteProperties[i]), "%d", 1); //reset
									}
									else
									{
										color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
										char tmpStr[32] = "";
										if ( propertyInt == 0 )
										{
											strcpy(tmpStr, "EAST");
										}
										else if ( propertyInt == 1 )
										{
											strcpy(tmpStr, "SOUTH");
										}
										else if ( propertyInt == 2 )
										{
											strcpy(tmpStr, "WEST");
										}
										else if ( propertyInt == 3 )
										{
											strcpy(tmpStr, "NORTH");
										}
										printTextFormattedColor(font8x8_bmp, pad_x3, pad_y2, color, tmpStr);
									}
								} 
								else if ( i == 2 )
								{
									if ( propertyInt > 100 || propertyInt < -1 )
									{
										errorMessage = 60;
										errorArr[i] = 1;
										snprintf(spriteProperties[i], sizeof(spriteProperties[i]), "%d", 1); //reset
									}
									else if ( propertyInt == -1 )
									{
										printTextFormattedColor(font8x8_bmp, pad_x3, pad_y2, colorRandom, "Default 10%");
									}
									else
									{
										color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
										char tmpStr[32] = "";
										strcpy(tmpStr, spriteProperties[i]); //reset
										strcat(tmpStr, " %%");
										printTextFormattedColor(font8x8_bmp, pad_x3, pad_y2, color, tmpStr);
									}
								}
							}

							if ( errorMessage )
							{
								color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
								if ( errorArr[i] == 1 )
								{
									printTextFormattedColor(font8x8_bmp, pad_x3, pad_y2, color, "Invalid ID!");
								}
							}

							pad_x1 = subx1 + 8;
						}

						pad_y1 = suby1 + 28 + 4 * spacing + 18;
						pad_x1 += 18;
						spacing = 18;
						//printText(font8x8_bmp, pad_x1 + 16, pad_y1, "Chest Facing");
						pad_y1 = suby1 + 28 + 7 * spacing;
						printText(font8x8_bmp, pad_x1 + 32, pad_y1, "NORTH(3)");
						pad_y1 = suby1 + 28 + 8 * spacing;
						printText(font8x8_bmp, pad_x1, pad_y1, "WEST(2)");
						printText(font8x8_bmp, pad_x1 + 96 - 16, pad_y1, "EAST(0)");
						pad_y1 = suby1 + 28 + 9 * spacing;
						printText(font8x8_bmp, pad_x1 + 32, pad_y1, "SOUTH(1)");

						spacing = 14;
						pad_y1 = suby1 + 14 + 14 * spacing;
						pad_x1 = subx1 + 8 + 192;
						pad_y1 = suby1 + 14 + 1 * spacing;
						printText(font8x8_bmp, pad_x1, pad_y1, "Chest Types");
						pad_y1 = suby1 + 14 + 2 * spacing;
						printText(font8x8_bmp, pad_x1, pad_y1, "0 - Random");
						pad_y1 = suby1 + 14 + 3 * spacing;
						printText(font8x8_bmp, pad_x1, pad_y1, "1 - Garbage");
						pad_y1 = suby1 + 14 + 4 * spacing;
						printText(font8x8_bmp, pad_x1, pad_y1, "2 - Food");
						pad_y1 = suby1 + 14 + 5 * spacing;
						printText(font8x8_bmp, pad_x1, pad_y1, "3 - Jewelry");
						pad_y1 = suby1 + 14 + 6 * spacing;
						printText(font8x8_bmp, pad_x1, pad_y1, "4 - Equipment");
						pad_y1 = suby1 + 14 + 7 * spacing;
						printText(font8x8_bmp, pad_x1, pad_y1, "5 - Tools");
						pad_y1 = suby1 + 14 + 8 * spacing;
						printText(font8x8_bmp, pad_x1, pad_y1, "6 - Magical");
						pad_y1 = suby1 + 14 + 9 * spacing;
						printText(font8x8_bmp, pad_x1, pad_y1, "7 - Potions");

						pad_x1 = subx1 + 8;
						pad_y1 = suby1 + 28;
						spacing = 36;

						// Cycle properties with TAB.
						if ( keystatus[SDL_SCANCODE_TAB] )
						{
							keystatus[SDL_SCANCODE_TAB] = 0;
							cursorflash = ticks;
							editproperty++;
							if ( editproperty == numProperties )
							{
								editproperty = 0;
							}

							inputstr = spriteProperties[editproperty];
						}
						if ( keystatus[SDL_SCANCODE_ESCAPE] )
						{
							keystatus[SDL_SCANCODE_ESCAPE] = 0;
							buttonCloseSpriteSubwindow(NULL);
						}
						if ( keystatus[SDL_SCANCODE_RETURN] )
						{
							keystatus[SDL_SCANCODE_RETURN] = 0;
							buttonSpritePropertiesConfirm(NULL);
						}
						// select a textbox
						if ( mousestatus[SDL_BUTTON_LEFT] )
						{
							for ( int i = 0; i < numProperties; i++ )
							{
								if ( omousex >= pad_x1 - 4 && omousey >= suby1 + 40 + i * spacing && omousex < pad_x1 - 4 + pad_x2 && omousey < suby1 + 56 + i * spacing )
								{
									inputstr = spriteProperties[i];
									editproperty = i;
									cursorflash = ticks;
								}
								pad_x1 = subx1 + 8;
							}
						}
						if ( editproperty < numProperties )   // edit
						{
							if ( !SDL_IsTextInputActive() )
							{
								SDL_StartTextInput();
								inputstr = spriteProperties[0];
							}
							if ( editproperty == 2 )
							{
								inputlen = 3;
							}
							else
							{
								inputlen = 1;
							}
							if ( (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
							{
								printText(font8x8_bmp, subx1 + 8 + strlen(spriteProperties[editproperty]) * 8, suby1 + 44 + editproperty * spacing, "\26");
							}
						}
					}
				}
				else if ( newwindow == 4 || newwindow == 5 )
				{
					if ( selectedEntity != NULL )
					{
						int numProperties;

						if ( newwindow == 4 )
						{
							numProperties = sizeof(itemPropertyNames) / sizeof(itemPropertyNames[0]); //find number of entries in property list
						}
						else if ( newwindow == 5 )
						{
							numProperties = sizeof(monsterItemPropertyNames) / sizeof(monsterItemPropertyNames[0]); //find number of entries in property list
						}
						const int lenProperties = 32;

						int spacing = 36; // 36 px between each item in the list.
						int verticalOffset = 20;
						int pad_x1 = subx1 + 8 + 96 + 80; // 104 px spacing from subwindow start. handles right side item list
						int pad_y1 = suby1 + 20 + verticalOffset; // 20 px spacing from subwindow start. handles right side item list
						int pad_x2 = 64; // handles right side item list

						int pad_y2 = (suby2 - 52) + verticalOffset - 4; //handles right side item list
						int pad_x3 = subx1 + 8; //handles left side menu
						int pad_y3 = suby1 + 28; // 28 px spacing from subwindow start, handles left side menu
						int pad_x4 = 64; //handles left side menu-end
						int pad_y4; //handles left side menu-end
						int totalNumItems = (sizeof(itemNameStrings) / sizeof(itemNameStrings[0]));
						int editorNumItems = totalNumItems + 1;
						switch ( itemSlotSelected )
						{
							case -1:
								break;
							default:
								if ( itemSlotSelected < 10 )
								{
									editorNumItems = 0;
									for ( int i = 0; i < (sizeof(itemStringsByType[itemSlotSelected]) / sizeof(itemStringsByType[itemSlotSelected][0])); i++ )
									{
										if ( strcmp(itemStringsByType[itemSlotSelected][i], "") == 0) //look for the end of the array
										{
											i = totalNumItems;
										}
										editorNumItems++;
									}
								}
								break;
						}
						int propertyInt = 0;
						char tmpPropertyName[lenProperties] = "";
						Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
						Uint32 colorRandom = SDL_MapRGB(mainsurface->format, 0, 168, 255);
						for ( int i = 0; i < numProperties; i++ )
						{
							if ( newwindow == 4 )
							{
								strcpy(tmpPropertyName, itemPropertyNames[i]);

							}
							else if ( newwindow == 5 )
							{
								strcpy(tmpPropertyName, monsterItemPropertyNames[i]);
							}
							pad_y3 = suby1 + 40 + spacing + i * spacing;
							pad_y4 = suby1 + 44 + 12 + spacing + i * spacing;
							// box outlines then text
							drawDepressed(pad_x3 - 4, pad_y3, pad_x3 - 4 + pad_x2, pad_y4);
							// print values on top of boxes
							printText(font8x8_bmp, pad_x3, pad_y3 - 12, tmpPropertyName);
							printText(font8x8_bmp, pad_x3, pad_y3 + 4, spriteProperties[i]);

							propertyInt = atoi(spriteProperties[i]);

							if ( errorArr[i] != 1 )
							{
								if ( i == 0 )
								{
									if ( newwindow == 4 )
									{
										if ( propertyInt > totalNumItems - 1 || propertyInt < 1 )
										{
											errorMessage = 60;
											errorArr[i] = 1;
											if ( propertyInt < 1 )
											{
												snprintf(spriteProperties[i], sizeof(spriteProperties[i]), "%d", 1);
											}
											else
											{
												snprintf(spriteProperties[i], sizeof(spriteProperties[i]), "%d", editorNumItems - 1);
											}
										}
									}
									else
									{
										if ( propertyInt > totalNumItems - 1 || propertyInt < 0 )
										{
											errorMessage = 60;
											errorArr[i] = 1;
											if ( propertyInt < 0 )
											{
												snprintf(spriteProperties[i], sizeof(spriteProperties[i]), "%d", 0);
											}
											else
											{
												snprintf(spriteProperties[i], sizeof(spriteProperties[i]), "%d", editorNumItems - 1);
											}
										}
									}
								}
								else if ( i == 1 )
								{
									if ( propertyInt == 2 )
									{
										color = SDL_MapRGB(mainsurface->format, 200, 128, 0);
										printTextFormattedColor(font8x8_bmp, pad_x3 + pad_x4 + 8, pad_y3 + 4, color, "Decrepit");
									}
									else if ( propertyInt == 3 )
									{
										color = SDL_MapRGB(mainsurface->format, 255, 255, 0);
										printTextFormattedColor(font8x8_bmp, pad_x3 + pad_x4 + 8, pad_y3 + 4, color, "Worn");
									}
									else if ( propertyInt == 4 )
									{
										color = SDL_MapRGB(mainsurface->format, 128, 200, 0);
										printTextFormattedColor(font8x8_bmp, pad_x3 + pad_x4 + 8, pad_y3 + 4, color, "Servicable");
									}
									else if ( propertyInt == 5 )
									{
										color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
										printTextFormattedColor(font8x8_bmp, pad_x3 + pad_x4 + 8, pad_y3 + 4, color, "Excellent");
									}
									else if ( propertyInt == 1 )
									{
										color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
										printTextFormattedColor(font8x8_bmp, pad_x3 + pad_x4 + 8, pad_y3 + 4, color, "Broken");
									}
									else if ( propertyInt == 0 )
									{
										printTextFormattedColor(font8x8_bmp, pad_x3 + pad_x4 + 8, pad_y3 + 4, colorRandom, "Random");
									}
									else if ( propertyInt < 0 || propertyInt > 5 )
									{
										errorMessage = 60;
										errorArr[i] = 1;
										snprintf(spriteProperties[i], sizeof(spriteProperties[i]), "%d", 0); //reset
									}
								}
								else if ( i == 2 )
								{
									if ( strcmp(spriteProperties[i], "00") == 0 )
									{
										printTextFormattedColor(font8x8_bmp, pad_x3 + pad_x4 + 8, pad_y3 + 4, colorRandom, "Random");
									}
									else if ( propertyInt > 9 || propertyInt < -9 )
									{
										errorMessage = 60;
										errorArr[i] = 1;
										snprintf(spriteProperties[i], sizeof(spriteProperties[i]), "%d", 0); //reset
									}
								}
								else if ( i == 3 )
								{
									if ( propertyInt > 9 || propertyInt < 0 )
									{
										errorMessage = 60;
										errorArr[i] = 1;
										snprintf(spriteProperties[i], sizeof(spriteProperties[i]), "%d", 1); //reset
									}
									else if ( propertyInt == 0 )
									{
										color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
										printTextFormattedColor(font8x8_bmp, pad_x3 + pad_x4 + 8, pad_y3 + 4, color, "Invalid ID!");
									}
								}
								else if ( i == 4 )
								{
									if ( propertyInt == 1 )
									{
										color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
										printTextFormattedColor(font8x8_bmp, pad_x3 + pad_x4 + 8, pad_y3 + 4, color, "Identified");
									}
									else if ( propertyInt == 0 )
									{
										color = SDL_MapRGB(mainsurface->format, 255, 255, 0);
										printTextFormattedColor(font8x8_bmp, pad_x3 + pad_x4 + 8, pad_y3 + 4, color, "Unidentified");
									}
									else if ( propertyInt == 2 )
									{
										printTextFormattedColor(font8x8_bmp, pad_x3 + pad_x4 + 8, pad_y3 + 4, colorRandom, "Random");
									}
									else
									{
										errorMessage = 60;
										errorArr[i] = 1;
										snprintf(spriteProperties[i], sizeof(spriteProperties[i]), "%d", 0);
									}
								}
								else if ( i == 5 )
								{
									if ( propertyInt > 100 || propertyInt < 0 )
									{
										errorMessage = 60;
										errorArr[i] = 1;
										snprintf(spriteProperties[i], sizeof(spriteProperties[i]), "%d", 100); //reset
									}
									else if ( propertyInt == 0 )
									{
										color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
										printTextFormattedColor(font8x8_bmp, pad_x3 + pad_x4 + 8, pad_y3 + 4, color, "Invalid ID!");
									}
									else
									{
										color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
										char tmpStr[32] = "";
										strcpy(tmpStr, spriteProperties[i]); //reset
										strcat(tmpStr, " %%");
										printTextFormattedColor(font8x8_bmp, pad_x3 + pad_x4 + 8, pad_y3 + 4, color, tmpStr);
									}
								}
							}

							if ( errorMessage )
							{
								color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
								if ( errorArr[i] == 1 )
								{
									printTextFormattedColor(font8x8_bmp, pad_x3 + pad_x4 + 8, pad_y3 + 4, color, "Invalid ID!");
								}
							}

						}



						drawDepressed(pad_x1, pad_y1, subx2 - 20, pad_y2);
						drawDepressed(subx2 - 20, pad_y1, subx2 - 4, pad_y2);
						slidersize = std::min<int>(((pad_y2 - 1) - (pad_y1 + 1)), ((pad_y2 - 1) - (pad_y1 + 1)) / ((real_t)(editorNumItems) / 20)); //TODO: Why are int and real_t being compared?
						slidery = std::min(std::max(pad_y1, slidery), pad_y2 - 1 - slidersize);
						drawWindowFancy(subx2 - 19, slidery, subx2 - 5, slidery + slidersize);

						// directory list offset from slider
						y2 = ((real_t)(slidery - (pad_y1)) / (pad_y2 - (pad_y1))) * editorNumItems;
						if ( scroll )
						{
							slidery -= 8 * scroll;
							slidery = std::min(std::max(pad_y1, slidery), pad_y2 - 1 - slidersize);
							y2 = ((real_t)(slidery - (pad_y1)) / ((pad_y2) - (pad_y1))) * editorNumItems;
							itemSelect = std::min<long unsigned int>(std::max(y2, itemSelect), std::min<long unsigned int>(editorNumItems - 1, y2 + 19)); //TODO: Why are long unsigned int and int being compared? TWICE. On the same line.
							scroll = 0;
						}
						if ( mousestatus[SDL_BUTTON_LEFT] && omousex >= subx2 - 20 && omousex < subx2 - 4 && omousey >= (pad_y1) && omousey < pad_y2 )
						{
							slidery = oslidery + mousey - omousey;
							slidery = std::min(std::max(pad_y1, slidery), pad_y2 - 1 - slidersize);
							y2 = ((real_t)(slidery - (pad_y1)) / ((pad_y2) - (pad_y1))) * editorNumItems;
							mclick = 1;
							itemSelect = std::min<long unsigned int>(std::max(y2, itemSelect), std::min<long unsigned int>(editorNumItems - 1, y2 + 19)); //TODO: Why are long unsigned int and int being compared? TWICE. On the same line.
						}
						else
						{
							oslidery = slidery;
						}

						pos.x = pad_x1 ;
						pos.y = pad_y1 + 4 + (itemSelect - y2) * 8;
						pos.w = subx2 - pad_x1 - 24;
						pos.h = 8;
						drawRect(&pos, SDL_MapRGB(mainsurface->format, 64, 64, 64), 255);

						// print all the items
						x = pad_x1;
						y = pad_y1 + 4;
						if ( newwindow == 4 )
						{
							c = std::min<long unsigned int>(editorNumItems, 20 + y2); //TODO: Why are long unsigned int and int being compared?
						}
						else
						{
							c = std::min<long unsigned int>(editorNumItems, 24 + y2); //TODO: Why are long unsigned int and int being compared?
						}
						for ( z = y2; z < c; z++ )
						{
							if ( newwindow == 5 && z == 1 )
							{
								printText(font8x8_bmp, x, y, "default_random");
							}
							else
							{
								switch ( itemSlotSelected )
								{
									case -1:
										printText(font8x8_bmp, x, y, itemNameStrings[z]);
										break;
									default:
										if ( itemSlotSelected < 10 )
										{
											printText(font8x8_bmp, x, y, itemStringsByType[itemSlotSelected][z]);
										}
										else
										{
											printText(font8x8_bmp, x, y, itemNameStrings[z]);
										}
										break;
								}
									
							}
							y += 8;
						}

						// item selection box
						pad_y3 = suby1 + 40;
						pad_y4 = suby1 + 44 + 12;
						// box outlines then text
						drawDepressed(pad_x3 - 4, pad_y3, pad_x3 - 4 + pad_x2 + 112, pad_y4);
						// print values on top of boxes
						printText(font8x8_bmp, pad_x3, pad_y3 - 12, "Item Name");
						printText(font8x8_bmp, pad_x1, pad_y3 - 12, "Click to select item");
						printText(font8x8_bmp, pad_x3, pad_y3 + 4, itemName);
						//drawDepressed(pad_x1, suby2 - 48, subx2 - 4, suby2 - 32);
						//printText(font8x8_bmp, pad_x1, suby2 - 44, itemName);

						// select a file
						if ( mousestatus[SDL_BUTTON_LEFT] )
						{
							if ( omousex >= pad_x1 && omousex < subx2 - 24 && omousey >= pad_y1 + 4 && omousey < pad_y2 - 4 )
							{
								itemSelect = y2 + ((omousey - (pad_y1 + 4)) >> 3);
								if ( newwindow == 4 )
								{
									itemSelect = std::min<long unsigned int>(std::max(y2, itemSelect), std::min<long unsigned int>(editorNumItems - 1, y2 + 19)); //TODO: Why are long unsigned int and int being compared? TWICE. On the same line.
								}
								else
								{
									itemSelect = std::min<long unsigned int>(std::max(y2, itemSelect), std::min<long unsigned int>(editorNumItems - 1, y2 + 23)); //TODO: Why are long unsigned int and int being compared? TWICE. On the same line.
								}
								switch ( itemSlotSelected )
								{
									case -1:
										strcpy(itemName, itemNameStrings[itemSelect]);
										break;
									default:
										if ( itemSlotSelected < 10 )
										{
											strcpy(itemName, itemStringsByType[itemSlotSelected][itemSelect]);
										}
										else
										{
											strcpy(itemName, itemNameStrings[itemSelect]);
										}
										break;
								}
								//inputstr = itemName;
								editorNumItems = (sizeof(itemNameStrings) / sizeof(itemNameStrings[0]));
								for ( z = 0; z < editorNumItems; z++ )
								{
									if ( strcmp(itemName, itemNameStrings[z]) == 0 )
									{
										char tmpStr[5];
										snprintf(tmpStr, sizeof(tmpStr), "%d", z);
										strcpy(spriteProperties[0], tmpStr);
										
										z = editorNumItems;
									}
								}
							}

							for ( int i = 0; i < numProperties; i++ )
							{
								if ( omousex >= pad_x3 - 4 && omousey >= suby1 + 40 + spacing + i * spacing && omousex < pad_x3 - 4 + pad_x4 && omousey < suby1 + 56 + spacing + i * spacing )
								{
									inputstr = spriteProperties[i];
									editproperty = i;
									cursorflash = ticks;
								}
							}
						}

						// Cycle properties with TAB.
						if ( keystatus[SDL_SCANCODE_TAB] )
						{
							keystatus[SDL_SCANCODE_TAB] = 0;
							cursorflash = ticks;
							editproperty++;
							if ( editproperty == numProperties )
							{
								editproperty = 0;
							}

							inputstr = spriteProperties[editproperty];
						}
						if ( keystatus[SDL_SCANCODE_ESCAPE] )
						{
							keystatus[SDL_SCANCODE_ESCAPE] = 0;
							buttonCloseSpriteSubwindow(NULL);
						}
						if ( keystatus[SDL_SCANCODE_RETURN] )
						{
							keystatus[SDL_SCANCODE_RETURN] = 0;
							buttonSpritePropertiesConfirm(NULL);
						}
						
						if ( editproperty < numProperties )   // edit
						{
							if ( !SDL_IsTextInputActive() )
							{
								SDL_StartTextInput();
								inputstr = spriteProperties[0];
							}

							if ( editproperty == 0 )
							{
								inputlen = 3;
								//update the item name when the ID changes.
								if ( newwindow == 5 && atoi(spriteProperties[0]) == 1 )
								{
									strcpy(itemName, "default_random");
								}
								else
								{
									strcpy(itemName, itemNameStrings[atoi(spriteProperties[0])]);
								}
							}
							else if( editproperty == 2 )
							{
								inputlen = 2;
							}
							else if ( editproperty == 5 )
							{
								inputlen = 3;
							}
							else
							{
								inputlen = 1;
							}

							if ( (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
							{
								printText(font8x8_bmp, pad_x3 + strlen(spriteProperties[editproperty]) * 8, suby1 + 44 + spacing + editproperty * spacing, "\26");
							}
						}
						
					}
				}
				else if ( newwindow == 6 )
				{
					if ( selectedEntity != NULL )
					{
						int numProperties = sizeof(summonTrapPropertyNames) / sizeof(summonTrapPropertyNames[0]); //find number of entries in property list
						const int lenProperties = sizeof(summonTrapPropertyNames[0]) / sizeof(char); //find length of entry in property list
						int spacing = 36; // 36 px between each item in the list.
						int pad_y1 = suby1 + 28; // 28 px spacing from subwindow start.
						int pad_x1 = subx1 + 8; // 8px spacing from subwindow start.
						int pad_x2 = 64;
						int pad_x3 = pad_x1 + pad_x2 + 8;
						int pad_y2 = 0;
						char tmpPropertyName[lenProperties] = "";
						Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
						Uint32 color2 = SDL_MapRGB(mainsurface->format, 255, 255, 0);
						Uint32 colorBad = SDL_MapRGB(mainsurface->format, 255, 0, 0);
						Uint32 colorRandom = SDL_MapRGB(mainsurface->format, 0, 168, 255);

						for ( int i = 0; i < numProperties; i++ )
						{
							int propertyInt = atoi(spriteProperties[i]);

							strcpy(tmpPropertyName, summonTrapPropertyNames[i]);
							pad_y1 = suby1 + 28 + i * spacing;
							pad_y2 = suby1 + 44 + i * spacing;
							// box outlines then text
							drawDepressed(pad_x1 - 4, suby1 + 40 + i * spacing, pad_x1 - 4 + pad_x2, suby1 + 56 + i * spacing);
							// print values on top of boxes
							printText(font8x8_bmp, pad_x1, suby1 + 44 + i * spacing, spriteProperties[i]);
							printText(font8x8_bmp, pad_x1, pad_y1, tmpPropertyName);

							if ( errorArr[i] != 1 )
							{
								if ( i == 0 ) //check input for valid entries, correct or notify the user if out of bounds.
								{
									if ( propertyInt > 32 || propertyInt < -1 )
									{
										errorMessage = 60;
										errorArr[i] = 1;
										snprintf(spriteProperties[i], sizeof(spriteProperties[i]), "%d", 0); //reset
									}
									else if ( propertyInt == 0 )
									{
										printTextFormattedColor(font8x8_bmp, pad_x3, pad_y2, colorRandom, "Random monster to match level curve");
									}
									else if ( propertyInt == -1 )
									{
										printTextFormattedColor(font8x8_bmp, pad_x3, pad_y2, colorRandom, "Completely random monster");
									}
									else if ( propertyInt == 6 || propertyInt == 12 || propertyInt == 16 )
									{
										printTextFormattedColor(font8x8_bmp, pad_x3, pad_y2, colorBad, "Error: Unused monster ID, will reset to 0");
									}
									else
									{
										color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
										char tmpStr[32] = "";
										strcpy(tmpStr, monsterEditorNameStrings[atoi(spriteProperties[i])]);
										printTextFormattedColor(font8x8_bmp, pad_x3, pad_y2, color, tmpStr);
									}
								}
								else if ( i == 1 )
								{
									if ( propertyInt > 9 || propertyInt < 0 )
									{
										errorMessage = 60;
										errorArr[i] = 1;
										snprintf(spriteProperties[i], sizeof(spriteProperties[i]), "%d", 1); //reset
									}
									else if ( propertyInt == 0 )
									{
										printTextFormattedColor(font8x8_bmp, pad_x3, pad_y2, colorBad, "Error: Must be > 0");
									}
								}
								else if ( i == 2 )
								{
									if ( propertyInt > 999 || propertyInt < 0 )
									{
										errorMessage = 60;
										errorArr[i] = 1;
										snprintf(spriteProperties[i], sizeof(spriteProperties[i]), "%d", 1); //reset
									}
									else if ( propertyInt == 0 )
									{
										printTextFormattedColor(font8x8_bmp, pad_x3, pad_y2, colorBad, "Error: Must be > 0");
									}
									else
									{
										printTextFormattedColor(font8x8_bmp, pad_x3, pad_y2, color, "%d seconds", atoi(spriteProperties[i]));
									}
								}
								else if ( i == 3 )
								{
									if ( propertyInt > 99 || propertyInt < 0 )
									{
										errorMessage = 60;
										errorArr[i] = 1;
										snprintf(spriteProperties[i], sizeof(spriteProperties[i]), "%d", 1); //reset
									}
									else if ( propertyInt == 0 )
									{
										printTextFormattedColor(font8x8_bmp, pad_x3, pad_y2, colorBad, "Error: Must be > 0");
									}
									else
									{
										printTextFormattedColor(font8x8_bmp, pad_x3, pad_y2, color, "%d instances", atoi(spriteProperties[i]));
									}
								}
								else if ( i == 4 )
								{
									if ( propertyInt > 1 || propertyInt < 0 )
									{
										errorMessage = 60;
										errorArr[i] = 1;
										snprintf(spriteProperties[i], sizeof(spriteProperties[i]), "%d", 0); //reset
									}
									else if ( propertyInt == 0 )
									{
										printTextFormattedColor(font8x8_bmp, pad_x3, pad_y2, color2, "No - power to enable");
									}
									else if ( propertyInt == 1 )
									{
										printTextFormattedColor(font8x8_bmp, pad_x3, pad_y2, color, "Yes - power to disable");
									}
								}
								else if ( i == 5 )
								{
									if ( propertyInt > 100 || propertyInt < 0 )
									{
										errorMessage = 60;
										errorArr[i] = 1;
										snprintf(spriteProperties[i], sizeof(spriteProperties[i]), "%d", 0); //reset
									}
									else
									{
										color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
										char tmpStr[32] = "";
										strcpy(tmpStr, spriteProperties[i]); //reset
										strcat(tmpStr, " %%");
										printTextFormatted(font8x8_bmp, pad_x3, pad_y2, tmpStr);
									}
								}
							}

							if ( errorMessage )
							{
								color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
								if ( errorArr[i] == 1 )
								{
									printTextFormattedColor(font8x8_bmp, pad_x3, pad_y2, color, "Invalid ID!");
								}
							}

							pad_x1 = subx1 + 8;
						}
						// Cycle properties with TAB.
						if ( keystatus[SDL_SCANCODE_TAB] )
						{
							keystatus[SDL_SCANCODE_TAB] = 0;
							cursorflash = ticks;
							editproperty++;
							if ( editproperty == numProperties )
							{
								editproperty = 0;
							}

							inputstr = spriteProperties[editproperty];
						}
						if ( keystatus[SDL_SCANCODE_ESCAPE] )
						{
							keystatus[SDL_SCANCODE_ESCAPE] = 0;
							buttonCloseSpriteSubwindow(NULL);
						}
						if ( keystatus[SDL_SCANCODE_RETURN] )
						{
							keystatus[SDL_SCANCODE_RETURN] = 0;
							buttonSpritePropertiesConfirm(NULL);
						}
						// select a textbox
						if ( mousestatus[SDL_BUTTON_LEFT] )
						{
							for ( int i = 0; i < numProperties; i++ )
							{
								if ( omousex >= pad_x1 - 4 && omousey >= suby1 + 40 + i * spacing && omousex < pad_x1 - 4 + pad_x2 && omousey < suby1 + 56 + i * spacing )
								{
									inputstr = spriteProperties[i];
									editproperty = i;
									cursorflash = ticks;
								}
								pad_x1 = subx1 + 8;
							}
						}
						if ( editproperty < numProperties )   // edit
						{
							if ( !SDL_IsTextInputActive() )
							{
								SDL_StartTextInput();
								inputstr = spriteProperties[0];
							}
							if ( editproperty == 0 || editproperty == 3) //length of text field allowed to enter
							{
								inputlen = 2;
							}
							else if ( editproperty == 2 || editproperty == 5 )
							{
								inputlen = 3;
							}
							else
							{
								inputlen = 1;
							}
							if ( (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
							{
								printText(font8x8_bmp, subx1 + 8 + strlen(spriteProperties[editproperty]) * 8, suby1 + 44 + editproperty * spacing, "\26");
							}
						}
					}
				}
			}
			else
			{
				if ( SDL_IsTextInputActive() )
				{
					SDL_StopTextInput();
				}

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
					if ( keystatus[SDL_SCANCODE_H] )
					{
						keystatus[SDL_SCANCODE_H] = 0;
						buttonHoverText(NULL);
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
				if ( keystatus[SDL_SCANCODE_F2] )
				{
					keystatus[SDL_SCANCODE_F2] = 0;
					makeUndo();
					buttonSpriteProperties(NULL);
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
					lastSelectedEntity = selectedEntity;
					setSpriteAttributes(selectedEntity, nullptr, nullptr);
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

			int numsprites = static_cast<int>(sizeof(spriteEditorNameStrings) / sizeof(spriteEditorNameStrings[0]));

			if ( palette[mousey + mousex * yres] >= 0 && palette[mousey + mousex * yres] <= numsprites )
			{
				printTextFormatted(font8x8_bmp, 0, yres - 8, "Sprite index:%5d", palette[mousey + mousex * yres]);
				printTextFormatted(font8x8_bmp, 0, yres - 16, "%s", spriteEditorNameStrings[palette[mousey + mousex * yres]]);

				char hoverTextString[32] = "";
				snprintf(hoverTextString, 5, "%d: ", palette[mousey + mousex * yres]);
				strcat(hoverTextString, spriteEditorNameStrings[palette[mousey + mousex * yres]]);
				int hoverTextWidth = strlen(hoverTextString);

				if ( mousey - 20 <= 0 )
				{
					if ( mousex + 16 + 8 * hoverTextWidth >= xres )
					{
						// stop text being drawn above y = 0 and past window width (xres)
						drawWindowFancy(mousex - 16 - (8 + 8 * hoverTextWidth), 0, mousex - 16, 16);
						printTextFormatted(font8x8_bmp, mousex - 16 - (4 + 8 * hoverTextWidth), 4, "%s", hoverTextString);
					}
					else
					{
						// stop text being drawn above y = 0 
						drawWindowFancy(mousex + 16, 0, 16 + 8 + mousex + 8 * hoverTextWidth, 16);
						printTextFormatted(font8x8_bmp, mousex + 16 + 4, 4, "%s", hoverTextString);
					}
				}
				else
				{
					if ( mousex + 16 + 8 * hoverTextWidth >= xres )
					{
						// stop text being drawn past window width (xres)
						drawWindowFancy(xres - (8 + 8 * hoverTextWidth), mousey - 20, xres, mousey - 4);
						printTextFormatted(font8x8_bmp, xres - (4 + 8 * hoverTextWidth), mousey - 16, "%s", hoverTextString);
					}
					else
					{
						drawWindowFancy(mousex + 16, mousey - 20, 16 + 8 + mousex + 8 * hoverTextWidth, mousey - 4);
						printTextFormatted(font8x8_bmp, mousex + 16 + 4, mousey - 16, "%s", hoverTextString);
					}
				}
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

			int numtiles = static_cast<int>(sizeof(tileEditorNameStrings) / sizeof(tileEditorNameStrings[0]));

			if ( palette[mousey + mousex * yres] >= 0 && palette[mousey + mousex * yres] <= numtiles)
			{
				printTextFormatted(font8x8_bmp, 0, yres - 8, "Tile index:%5d", palette[mousey + mousex * yres]);
				printTextFormatted(font8x8_bmp, 0, yres - 16, "%s", tileEditorNameStrings[palette[mousey + mousex * yres]]);

				char hoverTextString[32] = "";
				snprintf(hoverTextString, 5, "%d: ", palette[mousey + mousex * yres]);
				strcat(hoverTextString, tileEditorNameStrings[palette[mousey + mousex * yres]]);
				int hoverTextWidth = strlen(hoverTextString);

				if ( mousey - 20 <= 0 )
				{
					if ( mousex + 16 + 8 * hoverTextWidth >= xres )
					{
						// stop text being drawn above y = 0 and past window width (xres)
						drawWindowFancy(mousex - 16 - (8 + 8 * hoverTextWidth), 0, mousex - 16, 16);
						printTextFormatted(font8x8_bmp, mousex - 16 - (4 + 8 * hoverTextWidth), 4, "%s", hoverTextString);
					}
					else
					{
						// stop text being drawn above y = 0 
						drawWindowFancy(mousex + 16, 0, 16 + 8 + mousex + 8 * hoverTextWidth, 16);
						printTextFormatted(font8x8_bmp, mousex + 16 + 4, 4, "%s", hoverTextString);
					}
				}
				else
				{
					if ( mousex + 16 + 8 * hoverTextWidth >= xres )
					{
						// stop text being drawn past window width (xres)
						drawWindowFancy(xres - (8 + 8 * hoverTextWidth), mousey - 20, xres, mousey - 4);
						printTextFormatted(font8x8_bmp, xres - (4 + 8 * hoverTextWidth), mousey - 16, "%s", hoverTextString);
					}
					else
					{
						drawWindowFancy(mousex + 16, mousey - 20, 16 + 8 + mousex + 8 * hoverTextWidth, mousey - 4);
						printTextFormatted(font8x8_bmp, mousex + 16 + 4, mousey - 16, "%s", hoverTextString);
					}
				}
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

