/*-------------------------------------------------------------------------------

	BARONY
	File: buttons.cpp
	Desc: contains code for all buttons in the editor

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "editor.hpp"
#include "entity.hpp"
#include "player.hpp"

button_t* butX;
button_t* but_;
button_t* butTilePalette;
button_t* butSprite;
button_t* butPoint;
button_t* butBrush;
button_t* butSelect;
button_t* butFill;
button_t* butFile;
button_t* butNew;
button_t* butOpen;
button_t* butSave;
button_t* butSaveAs;
button_t* butExit;
button_t* butEdit;
button_t* butCut;
button_t* butCopy;
button_t* butPaste;
button_t* butDelete;
button_t* butSelectAll;
button_t* butUndo;
button_t* butRedo;
button_t* butView;
button_t* butToolbox;
button_t* butStatusBar;
button_t* butAllLayers;
button_t* butViewSprites;
button_t* butGrid;
button_t* but3DMode;
button_t* butMap;
button_t* butAttributes;
button_t* butClearMap;
button_t* butHelp;
button_t* butAbout;

// Corner buttons

void buttonExit(button_t* my)
{
	button_t* button;

	// this shouldn't work if a window is already open
	if ( subwindow )
	{
		return;
	}

	menuVisible = 0;
	subwindow = 1;
	subx1 = xres / 2 - 128;
	subx2 = xres / 2 + 128;
	suby1 = yres / 2 - 32;
	suby2 = yres / 2 + 32;
	strcpy(subtext, "Are you sure you want to quit?\nAny unsaved work will be lost.");

	button = newButton();
	strcpy(button->label, "Yes");
	button->x = subx1 + 32;
	button->y = suby2 - 24;
	button->sizex = 32;
	button->sizey = 16;
	button->action = &buttonExitConfirm;
	button->visible = 1;
	button->focused = 1;

	button = newButton();
	strcpy(button->label, "No");
	button->x = subx2 - 56;
	button->y = suby2 - 24;
	button->sizex = 24;
	button->sizey = 16;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
}

void buttonExitConfirm(button_t* my)
{
	mainloop = 0; // gracefully stops the game/editor
}

void buttonIconify(button_t* my)
{
	// aka minimize
	SDL_MinimizeWindow(screen);
}

// Toolbox buttons

void buttonTilePalette(button_t* my)
{
	tilepalette = 1;
}

void buttonSprite(button_t* my)
{
	makeUndo();
	spritepalette = 1;
}

void buttonPoint(button_t* my)
{
	selectedTool = 0;
	selectedarea = false;
}

void buttonBrush(button_t* my)
{
	selectedTool = 1;
	selectedarea = false;
}

void buttonSelect(button_t* my)
{
	selectedTool = 2;
	selectedarea = false;
}

void buttonFill(button_t* my)
{
	selectedTool = 3;
	selectedarea = false;
}

// File menu

void buttonFile(button_t* my)
{
	if ( menuVisible != 1 )
	{
		menuVisible = 1;
	}
	else
	{
		menuVisible = 0;
	}
}

void buttonNew(button_t* my)
{
	button_t* button;

	editproperty = 0;
	inputstr = map.name;
	snprintf(widthtext, 4, "%d", map.width);
	snprintf(heighttext, 4, "%d", map.height);
	strcpy(nametext, map.name);
	strcpy(authortext, map.author);
	cursorflash = ticks;
	menuVisible = 0;
	subwindow = 1;
	newwindow = 1;
	subx1 = xres / 2 - 160;
	subx2 = xres / 2 + 160;
	suby1 = yres / 2 - 80;
	suby2 = yres / 2 + 80;
	strcpy(subtext, "New map:");

	button = newButton();
	strcpy(button->label, "Create");
	button->x = subx2 - 64;
	button->y = suby2 - 48;
	button->sizex = 56;
	button->sizey = 16;
	button->action = &buttonNewConfirm;
	button->visible = 1;
	button->focused = 1;

	button = newButton();
	strcpy(button->label, "Cancel");
	button->x = subx2 - 64;
	button->y = suby2 - 24;
	button->sizex = 56;
	button->sizey = 16;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;

	button = newButton();
	strcpy(button->label, "X");
	button->x = subx2 - 16;
	button->y = suby1;
	button->sizex = 16;
	button->sizey = 16;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
}

void buttonNewConfirm(button_t* my)
{
	int x, y, z, c;
	clearUndos();
	free(map.tiles);
	list_FreeAll(map.entities);
	strcpy(map.name, nametext);
	strcpy(map.author, authortext);
	map.width = atoi(widthtext);
	map.height = atoi(heighttext);
	map.width = std::min(std::max(MINWIDTH, map.width), MAXWIDTH);
	map.height = std::min(std::max(MINHEIGHT, map.height), MAXHEIGHT);
	map.tiles = (int*) malloc(sizeof(int) * MAPLAYERS * map.height * map.width);
	for ( z = 0; z < MAPLAYERS; z++ )
	{
		for ( y = 0; y < map.height; y++ )
		{
			for ( x = 0; x < map.width; x++ )
			{
				if ( z == OBSTACLELAYER )
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
	if ( vismap != NULL )
	{
		free(vismap);
	}
	vismap = (bool*) malloc(sizeof(bool) * map.width * map.height);
	if ( lightmap != NULL )
	{
		free(lightmap);
	}
	lightmap = (int*) malloc(sizeof(Sint32) * map.width * map.height);
	for (c = 0; c < map.width * map.height; c++ )
	{
		lightmap[c] = 0;
	}
	strcpy(message, "                             Created a new map.");
	filename[0] = 0;
	oldfilename[0] = 0;
	messagetime = 60;
	buttonCloseSubwindow(my);
}

void buttonOpen(button_t* my)
{
	button_t* button;
	DIR* dir = NULL;
	struct dirent* ent = NULL;
	unsigned long c = 0;

	inputstr = filename;
	cursorflash = ticks;
	d_names_length = 0;
	menuVisible = 0;
	subwindow = 1;
	openwindow = 1;
	slidery = 0;
	selectedFile = 0;
	subx1 = xres / 2 - 160;
	subx2 = xres / 2 + 160;
	suby1 = yres / 2 - 120;
	suby2 = yres / 2 + 120;
	strcpy(subtext, "Open file:");

	button = newButton();
	strcpy(button->label, " Open ");
	button->x = subx2 - 64;
	button->y = suby2 - 48;
	button->sizex = 56;
	button->sizey = 16;
	button->action = &buttonOpenConfirm;
	button->visible = 1;
	button->focused = 1;

	button = newButton();
	strcpy(button->label, "Cancel");
	button->x = subx2 - 64;
	button->y = suby2 - 24;
	button->sizex = 56;
	button->sizey = 16;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;

	button = newButton();
	strcpy(button->label, "X");
	button->x = subx2 - 16;
	button->y = suby1;
	button->sizex = 16;
	button->sizey = 16;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;

	// file list
	if ( (dir = opendir("maps/")) != NULL )
	{
		while ( (ent = readdir(dir)) != NULL )
		{
			if ( strstr(ent->d_name, ".lmp") != NULL || !strcmp(ent->d_name, "..") || !strcmp(ent->d_name, ".") )
			{
				d_names_length++;
			}
		}
		closedir(dir);
	}
	else
	{
		// could not open directory
		printlog("failed to open map directory for viewing!\n");
		d_names = NULL;
		d_names_length = 0;
		return;
	}
	if ( d_names_length > 0 )
	{
		d_names = (char**) malloc(sizeof(char*)*d_names_length);
		for ( c = 0; c < d_names_length; c++ )
		{
			d_names[c] = (char*) malloc(sizeof(char) * FILENAME_MAX);
		}
		c = 0;
		if ( (dir = opendir("maps/")) != NULL )
		{
			while ( (ent = readdir(dir)) != NULL )
			{
				if ( strstr(ent->d_name, ".lmp") != NULL || !strcmp(ent->d_name, "..") || !strcmp(ent->d_name, ".") )
				{
					strcpy(d_names[c], ent->d_name);
					c++;
				}
			}
			closedir(dir);
		}
	}
	else
	{
		d_names = NULL;
	}
}

void buttonOpenConfirm(button_t* my)
{
	int c, c2;
	clearUndos();
	strcpy(oldfilename, filename);
	strcpy(message, "");
	for ( c = 0; c < 32; c++ )
	{
		if (filename[c] == 0)
		{
			break;
		}
	}
	for ( c2 = 0; c2 < 32 - c; c2++ )
	{
		strcat(message, " ");
	}
	printlog("opening map file '%s'...\n", filename);
	if (loadMap(filename, &map, map.entities) == -1)
	{
		strcat(message, "Failed to open ");
		strcat(message, filename);
	}
	else
	{
		strcat(message, "      Opened '");
		strcat(message, filename);
		strcat(message, "'");
	}
	messagetime = 60; // 60*50 ms = 3000 ms (3 seconds)
	buttonCloseSubwindow(my);
}

void buttonSave(button_t* my)
{
	int c, c2;
	menuVisible = 0;
	strcpy(oldfilename, filename);
	inputstr = filename;
	if (filename[0] == 0)
	{
		buttonSaveAs(my);
	}
	else
	{
		strcpy(message, "");
		for ( c = 0; c < 32; c++ )
		{
			if (filename[c] == 0)
			{
				break;
			}
		}
		for ( c2 = 0; c2 < 32 - c; c2++ )
		{
			strcat(message, " ");
		}
		printlog("saving map file '%s'...\n", filename);
		if (saveMap(filename))
		{
			strcat(message, "Failed to save ");
			strcat(message, filename);
		}
		else
		{
			strcat(message, "       Saved '");
			strcat(message, filename);
			strcat(message, "'");
		}
		messagetime = 60; // 60*50 ms = 3000 ms (3 seconds)
		buttonCloseSubwindow(my);
	}
}

void buttonSaveAs(button_t* my)
{
	button_t* button;
	DIR* dir;
	struct dirent* ent;
	unsigned long c = 0;

	cursorflash = ticks;
	d_names_length = 0;
	menuVisible = 0;
	subwindow = 1;
	savewindow = 1;
	slidery = 0;
	selectedFile = 0;
	subx1 = xres / 2 - 160;
	subx2 = xres / 2 + 160;
	suby1 = yres / 2 - 120;
	suby2 = yres / 2 + 120;
	strcpy(subtext, "Save file:");

	button = newButton();
	strcpy(button->label, " Save ");
	button->x = subx2 - 64;
	button->y = suby2 - 48;
	button->sizex = 56;
	button->sizey = 16;
	button->action = &buttonSave;
	button->visible = 1;
	button->focused = 1;

	button = newButton();
	strcpy(button->label, "Cancel");
	button->x = subx2 - 64;
	button->y = suby2 - 24;
	button->sizex = 56;
	button->sizey = 16;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;

	button = newButton();
	strcpy(button->label, "X");
	button->x = subx2 - 16;
	button->y = suby1;
	button->sizex = 16;
	button->sizey = 16;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;

	// file list
	if ( (dir = opendir("maps/")) != NULL )
	{
		while ( (ent = readdir(dir)) != NULL )
		{
			if ( strstr(ent->d_name, ".lmp") != NULL || !strcmp(ent->d_name, "..") || !strcmp(ent->d_name, ".") )
			{
				d_names_length++;
			}
		}
		closedir(dir);
	}
	else
	{
		// could not open directory
		printlog("failed to open map directory for viewing!\n");
		d_names = NULL;
		d_names_length = 0;
		return;
	}
	if ( d_names_length > 0 )
	{
		d_names = (char**) malloc(sizeof(char*)*d_names_length);
		for ( c = 0; c < d_names_length; c++ )
		{
			d_names[c] = (char*) malloc(sizeof(char) * FILENAME_MAX);
		}
		c = 0;
		if ( (dir = opendir("maps/")) != NULL )
		{
			while ( (ent = readdir(dir)) != NULL )
			{
				if ( strstr(ent->d_name, ".lmp") != NULL || !strcmp(ent->d_name, "..") || !strcmp(ent->d_name, ".") )
				{
					strcpy(d_names[c], ent->d_name);
					c++;
				}
			}
			closedir(dir);
		}
	}
	else
	{
		d_names = NULL;
	}
}

// Edit menu

void buttonEdit(button_t* my)
{
	if ( menuVisible != 2 )
	{
		menuVisible = 2;
	}
	else
	{
		menuVisible = 0;
	}
}

void buttonCut(button_t* my)
{
	menuVisible = 0;
	if ( !selectedarea )
	{
		return;
	}
	buttonCopy(my);
	selectedarea = true;
	buttonDelete(my);
}

void buttonCopy(button_t* my)
{
	menuVisible = 0;
	int x, y;

	// copy the selected tiles
	if (selectedarea && !pasting)
	{
		copymap.width = selectedarea_x2 - selectedarea_x1 + 1;
		copymap.height = selectedarea_y2 - selectedarea_y1 + 1;
		if ( copymap.tiles != NULL )
		{
			free(copymap.tiles);
		}
		copymap.tiles = (Sint32*) malloc(sizeof(Sint32) * copymap.width * copymap.height * MAPLAYERS);
		memset(copymap.tiles, 0, sizeof(Sint32)*copymap.width * copymap.height * MAPLAYERS);
		for ( x = 0; x < copymap.width; x++ )
		{
			for ( y = 0; y < copymap.height; y++ )
			{
				copymap.tiles[drawlayer + y * MAPLAYERS + x * MAPLAYERS * copymap.height] = map.tiles[drawlayer + (y + selectedarea_y1) * MAPLAYERS + (x + selectedarea_x1) * MAPLAYERS * map.height];
			}
		}
		copymap.name[0] = drawlayer;
	}
	selectedarea = false;
}

void buttonPaste(button_t* my)
{
	menuVisible = 0;

	// paste the selected tiles
	if ( copymap.tiles != NULL )
	{
		pasting = true;
		selectedarea = false;
	}
}

void buttonDelete(button_t* my)
{
	menuVisible = 0;
	makeUndo();

	// delete the selected entity, if there is one
	if (selectedEntity != NULL)
	{
		list_RemoveNode(selectedEntity->mynode);
		selectedEntity = NULL;
	}
	if (selectedarea)
	{
		// delete all selected tiles
		int x, y;
		for ( x = selectedarea_x1; x <= selectedarea_x2; x++ )
		{
			for ( y = selectedarea_y1; y <= selectedarea_y2; y++ )
			{
				map.tiles[drawlayer + y * MAPLAYERS + x * MAPLAYERS * map.height] = 0;
			}
		}
		selectedarea = false;
	}
}

void buttonSelectAll(button_t* my)
{
	menuVisible = 0;
	selectedTool = 2;
	selectedarea = true;
	selectingspace = false;
	selectedarea_x1 = 0;
	selectedarea_x2 = map.width - 1;
	selectedarea_y1 = 0;
	selectedarea_y2 = map.height - 1;
}

void buttonUndo(button_t* my)
{
	menuVisible = 0;
	undo();
}

void buttonRedo(button_t* my)
{
	menuVisible = 0;
	redo();
}

// View menu

void buttonView(button_t* my)
{
	if ( menuVisible != 3 )
	{
		menuVisible = 3;
	}
	else
	{
		menuVisible = 0;
	}
}

void buttonToolbox(button_t* my)
{
	toolbox = (toolbox == 0);
	butTilePalette->visible = (butTilePalette->visible == 0);
	butSprite->visible = (butSprite->visible == 0);
	butPoint->visible = (butPoint->visible == 0);
	butBrush->visible = (butBrush->visible == 0);
	butSelect->visible = (butSelect->visible == 0);
	butFill->visible = (butFill->visible == 0);
}

void buttonStatusBar(button_t* my)
{
	statusbar = (statusbar == 0);
}

void buttonAllLayers(button_t* my)
{
	alllayers = (alllayers == 0);
}

void buttonViewSprites(button_t* my)
{
	viewsprites = (viewsprites == 0);
}

void buttonGrid(button_t* my)
{
	showgrid = (showgrid == 0);
}

void button3DMode(button_t* my)
{
	mode3d = (mode3d == false);
}

// Map menu

void buttonMap(button_t* my)
{
	if ( menuVisible != 4 )
	{
		menuVisible = 4;
	}
	else
	{
		menuVisible = 0;
	}
}

void buttonAttributes(button_t* my)
{
	button_t* button;

	editproperty = 0;
	inputstr = map.name;
	snprintf(widthtext, 4, "%d", map.width);
	snprintf(heighttext, 4, "%d", map.height);
	strcpy(nametext, map.name);
	strcpy(authortext, map.author);
	cursorflash = ticks;
	menuVisible = 0;
	subwindow = 1;
	newwindow = 1;
	subx1 = xres / 2 - 160;
	subx2 = xres / 2 + 160;
	suby1 = yres / 2 - 80;
	suby2 = yres / 2 + 80;
	strcpy(subtext, "Map properties:");

	button = newButton();
	strcpy(button->label, "  OK  ");
	button->x = subx2 - 64;
	button->y = suby2 - 48;
	button->sizex = 56;
	button->sizey = 16;
	button->action = &buttonAttributesConfirm;
	button->visible = 1;
	button->focused = 1;

	button = newButton();
	strcpy(button->label, "Cancel");
	button->x = subx2 - 64;
	button->y = suby2 - 24;
	button->sizex = 56;
	button->sizey = 16;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;

	button = newButton();
	strcpy(button->label, "X");
	button->x = subx2 - 16;
	button->y = suby1;
	button->sizex = 16;
	button->sizey = 16;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
}

void buttonAttributesConfirm(button_t* my)
{
	int x, y, z, c;
	map_t mapcopy;
	makeUndo();

	// make a copy of the current map
	mapcopy.width = map.width;
	mapcopy.height = map.height;
	mapcopy.tiles = (int*) malloc(sizeof(int) * MAPLAYERS * mapcopy.width * mapcopy.height);
	for ( z = 0; z < MAPLAYERS; z++ )
	{
		for ( y = 0; y < map.height; y++ )
		{
			for ( x = 0; x < map.width; x++ )
			{
				mapcopy.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height] = map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * mapcopy.height];
			}
		}
	}

	// allocate memory for a new map
	free(map.tiles);
	map.width = atoi(widthtext);
	map.height = atoi(heighttext);
	map.width = std::min(std::max(MINWIDTH, map.width), MAXWIDTH);
	map.height = std::min(std::max(MINHEIGHT, map.height), MAXHEIGHT);
	map.tiles = (int*) malloc(sizeof(int) * MAPLAYERS * map.height * map.width);
	strcpy(map.name, nametext);
	strcpy(map.author, authortext);
	if ( vismap != NULL )
	{
		free(vismap);
	}
	vismap = (bool*) malloc(sizeof(bool) * map.width * map.height);
	if ( lightmap != NULL )
	{
		free(lightmap);
	}
	lightmap = (int*) malloc(sizeof(Sint32) * map.width * map.height);
	for (c = 0; c < map.width * map.height; c++ )
	{
		lightmap[c] = 0;
	}

	// transfer data from the new map to the old map and fill extra space with empty data
	for ( z = 0; z < MAPLAYERS; z++ )
	{
		for ( y = 0; y < map.height; y++ )
		{
			for ( x = 0; x < map.width; x++ )
			{
				if ( x >= mapcopy.width || y >= mapcopy.height )
				{
					map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height] = 0;
				}
				else
				{
					map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height] = mapcopy.tiles[z + y * MAPLAYERS + x * MAPLAYERS * mapcopy.height];
				}
			}
		}
	}
	free(mapcopy.tiles);
	strcpy(message, "                       Modified map attributes.");
	messagetime = 60;
	buttonCloseSubwindow(my);
}

void buttonClearMap(button_t* my)
{
	button_t* button;

	menuVisible = 0;
	subwindow = 1;
	subx1 = xres / 2 - 160;
	subx2 = xres / 2 + 160;
	suby1 = yres / 2 - 56;
	suby2 = yres / 2 + 56;
	strcpy(subtext, "Warning:\n\nThis option will completely erase your\nentire map.\n\nAre you sure you want to continue?\n");

	button = newButton();
	strcpy(button->label, "OK");
	button->x = subx1 + 64;
	button->y = suby2 - 24;
	button->sizex = 24;
	button->sizey = 16;
	button->action = &buttonClearMapConfirm;
	button->visible = 1;
	button->focused = 1;

	button = newButton();
	strcpy(button->label, "Cancel");
	button->x = subx2 - 112;
	button->y = suby2 - 24;
	button->sizex = 56;
	button->sizey = 16;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;

	button = newButton();
	strcpy(button->label, "X");
	button->x = subx2 - 16;
	button->y = suby1;
	button->sizex = 16;
	button->sizey = 16;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
}

void buttonClearMapConfirm(button_t* my)
{
	long x, y, z;
	makeUndo();
	for ( z = 0; z < MAPLAYERS; z++ )
	{
		for ( y = 0; y < map.height; y++ )
		{
			for ( x = 0; x < map.width; x++ )
			{
				map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height] = 0;
			}
		}
	}
	list_FreeAll(map.entities);
	buttonCloseSubwindow(my);
}

// Help menu

void buttonHelp(button_t* my)
{
	if ( menuVisible != 5 )
	{
		menuVisible = 5;
	}
	else
	{
		menuVisible = 0;
	}
}

void buttonAbout(button_t* my)
{
	button_t* button;

	menuVisible = 0;
	subwindow = 1;
	subx1 = xres / 2 - 160;
	subx2 = xres / 2 + 160;
	suby1 = yres / 2 - 56;
	suby2 = yres / 2 + 56;
	strcpy(subtext, "Barony: Map Editor v1.2"
	       "\n\nSee EDITING for full documentation."
	       "\n\nThis software is copyright 2013 (c)"
	       "\nSheridan Rathbun, all rights reserved."
	       "\n\nSee LICENSE for details.\n");

	button = newButton();
	strcpy(button->label, "OK");
	button->x = xres / 2 - 12;
	button->y = suby2 - 24;
	button->sizex = 24;
	button->sizey = 16;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;

	button = newButton();
	strcpy(button->label, "X");
	button->x = subx2 - 16;
	button->y = suby1;
	button->sizex = 16;
	button->sizey = 16;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
}

// Subwindows
void buttonCloseSubwindow(button_t* my)
{
	int c;

	// close window
	selectedEntity = NULL;
	subwindow = 0;
	newwindow = 0;
	openwindow = 0;
	savewindow = 0;
	editproperty = 0;
	if ( d_names != NULL )
	{
		for ( c = 0; c < d_names_length; c++ )
			if ( d_names[c] != NULL )
			{
				free(d_names[c]);
				d_names[c] = NULL;
			}
		free(d_names);
		d_names = NULL;
	}
	strcpy(filename, oldfilename);
}

void buttonSpriteProperties(button_t* my)
{
	if ( selectedEntity != NULL )
	{
		Stat* spriteStats = selectedEntity->getStats();
		if ( spriteStats != nullptr )
		{
			button_t* button;
			editproperty = 0;
			//strcpy(spriteProperties[0], spriteStats->name);
			strcpy(spriteProperties[1], "");
			strcpy(spriteProperties[2], "");
			snprintf(spriteProperties[1], 4, "%d", spriteStats->sex);
			snprintf(spriteProperties[2], 4, "%d", spriteStats->MAXHP);
			inputstr = spriteStats->name;
			//snprintf(widthtext, 4, "%d", map.width);
			//snprintf(heighttext, 4, "%d", map.height);
			//strcpy(nametext, map.name);
			//strcpy(authortext, map.author);
			cursorflash = ticks;
			menuVisible = 0;
			subwindow = 1;
			newwindow = 2;
			subx1 = xres / 2 - 160;
			subx2 = xres / 2 + 160;
			suby1 = yres / 2 - 80;
			suby2 = yres / 2 + 80;
			strcpy(subtext, "Sprite properties:");

			button = newButton();
			strcpy(button->label, "  OK  ");
			button->x = subx2 - 64;
			button->y = suby2 - 48;
			button->sizex = 56;
			button->sizey = 16;
			button->action = &buttonSpritePropertiesConfirm;
			button->visible = 1;
			button->focused = 1;

			button = newButton();
			strcpy(button->label, "Cancel");
			button->x = subx2 - 64;
			button->y = suby2 - 24;
			button->sizex = 56;
			button->sizey = 16;
			button->action = &buttonCloseSpriteSubwindow;
			button->visible = 1;
			button->focused = 1;

			button = newButton();
			strcpy(button->label, "X");
			button->x = subx2 - 16;
			button->y = suby1;
			button->sizex = 16;
			button->sizey = 16;
			button->action = &buttonCloseSpriteSubwindow;
			button->visible = 1;
			button->focused = 1;
		}
	}
}

void buttonSpritePropertiesConfirm(button_t* my)
{
	if ( selectedEntity != NULL )
	{
		Stat* spriteStats = selectedEntity->getStats();
		if ( spriteStats != nullptr )
		{
			spriteStats->sex = (sex_t)atoi(spriteProperties[1]);
			spriteStats->MAXHP = (Sint32)atoi(spriteProperties[2]);
			strcpy(message, "                       Modified sprite properties.");
			messagetime = 60;
			buttonCloseSpriteSubwindow(my);
		}
	}
}

void buttonCloseSpriteSubwindow(button_t* my)
{
	// close window
	selectedEntity = NULL;
	subwindow = 0;
	newwindow = 0;
	editproperty = 0;
	spritepalette = 0;

}