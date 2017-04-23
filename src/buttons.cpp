/*-------------------------------------------------------------------------------

	BARONY
	File: buttons.cpp
	Desc: contains code for all buttons in the editor

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

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
button_t* butMonsterHelm;
button_t* butMonsterWeapon;
button_t* butMonsterShield;
button_t* butMonsterArmor;
button_t* butMonsterRing;
button_t* butMonsterAmulet;
button_t* butMonsterBoots;
button_t* butMonsterGloves;
button_t* butMonsterItem1;
button_t* butMonsterItem2;
button_t* butMonsterItem3;
button_t* butMonsterItem4;
button_t* butMonsterItem5;
button_t* butMonsterItem6;
button_t* butMonsterCloak;
button_t* butMonsterMask;
button_t* butMonsterOK;
button_t* butMonsterX;
button_t* butMonsterCancel;
button_t* butMonsterItemOK;
button_t* butMonsterItemX;
button_t* butMonsterItemCancel;
button_t* butItemOK;
button_t* butItemCancel;
button_t* butItemX;

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
		lastSelectedEntity = NULL;
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
	button_t* button;
	int c = 0;
	Stat* tmpSpriteStats = NULL;
	int spriteType = 0;
	int spacing = 20;
	int pad_y2;
	int pad_x3;
	int pad_x4;
	char tmpStr[32] = "";
	int itemIndex = 0;

	if ( selectedEntity == NULL && lastSelectedEntity != NULL )
	{
		if ( checkSpriteType(lastSelectedEntity->sprite) != 0 )
		{
			selectedEntity = lastSelectedEntity;
		}
		else
		{
			strcpy(message, "No properties available for previous sprite.");
			messagetime = 60;
		}
	}

	if ( selectedEntity != NULL )
	{
		editproperty = 0;
		for ( int i = 0; i < (sizeof(spriteProperties) / sizeof(spriteProperties[0])); i++ )
		{
			strcpy(spriteProperties[i], "");
		}

		spriteType = checkSpriteType(selectedEntity->sprite);
		switch ( spriteType )
		{
		case 1: //monsters
			tmpSpriteStats = selectedEntity->getStats();
			if ( tmpSpriteStats != nullptr )
			{
				copyMonsterStatToPropertyStrings(tmpSpriteStats);
				inputstr = spriteProperties[0];
				initMonsterPropertiesWindow();
			}
			tmpSpriteStats = NULL;
			break;
		case 2: //chests
			snprintf(spriteProperties[0], 4, "%d", (int)selectedEntity->yaw);
			snprintf(spriteProperties[1], 4, "%d", selectedEntity->skill[9]);
			inputstr = spriteProperties[0];
			cursorflash = ticks;
			menuVisible = 0;
			subwindow = 1;
			newwindow = 3;
			subx1 = xres / 2 - 160;
			subx2 = xres / 2 + 160;
			suby1 = yres / 2 - 105;
			suby2 = yres / 2 + 105;
			strcpy(subtext, "Sprite Properties:");
			break;
		case 3: //items
			itemSelect = 1;
			snprintf(spriteProperties[0], 4, "%d", (int)selectedEntity->skill[10]); //ID
			snprintf(spriteProperties[1], 4, "%d", (int)selectedEntity->skill[11]); //status
			if ( (int)selectedEntity->skill[12] == 10 )
			{
				strcpy(spriteProperties[2], "00"); //bless random
			}
			else
			{
				snprintf(spriteProperties[2], 4, "%d", (int)selectedEntity->skill[12]); //bless
			}
			snprintf(spriteProperties[3], 4, "%d", (int)selectedEntity->skill[13]); //count
			snprintf(spriteProperties[4], 4, "%d", (int)selectedEntity->skill[15]); //identified
			inputstr = spriteProperties[0];
			cursorflash = ticks;
			menuVisible = 0;
			subwindow = 1;
			newwindow = 4;
			slidery = 0;
			subx1 = xres / 2 - 200;
			subx2 = xres / 2 + 200;
			suby1 = yres / 2 - 122;
			suby2 = yres / 2 + 122;
			strcpy(subtext, "Item Properties:");
			break;
		default:
			strcpy(message, "No properties available for current sprite.");
			messagetime = 60;
			break;
		}

		//remaining buttons
		switch ( spriteType )
		{
			case 1: //monsters
				tmpSpriteStats = selectedEntity->getStats();
				if ( tmpSpriteStats != nullptr )
				{

					butMonsterOK = newButton();
					strcpy(butMonsterOK->label, "  OK  ");
					butMonsterOK->x = subx2 - 64;
					butMonsterOK->y = suby2 - 48;
					butMonsterOK->sizex = 56;
					butMonsterOK->sizey = 16;
					butMonsterOK->action = &buttonSpritePropertiesConfirm;
					butMonsterOK->visible = 1;
					butMonsterOK->focused = 1;

					butMonsterCancel = newButton();
					strcpy(butMonsterCancel->label, "Cancel");
					butMonsterCancel->x = subx2 - 64;
					butMonsterCancel->y = suby2 - 24;
					butMonsterCancel->sizex = 56;
					butMonsterCancel->sizey = 16;
					butMonsterCancel->action = &buttonCloseSpriteSubwindow;
					butMonsterCancel->visible = 1;
					butMonsterCancel->focused = 1;

					butMonsterX = newButton();
					strcpy(butMonsterX->label, "X");
					butMonsterX->x = subx2 - 16;
					butMonsterX->y = suby1;
					butMonsterX->sizex = 16;
					butMonsterX->sizey = 16;
					butMonsterX->action = &buttonCloseSpriteSubwindow;
					butMonsterX->visible = 1;
					butMonsterX->focused = 1;

					pad_y2 = suby1 + 28 + 2 * spacing;
					pad_x3 = 40;
					pad_x4 = subx2 - 112;
					itemIndex = 0;
					if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 0 )
					{
						strcpy(tmpStr, "NULL");
					}
					else if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 1 )
					{
						strcpy(tmpStr, "RAND");
					}
					else
					{
						snprintf(tmpStr, 4, "%d", tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6]);
					}

					if ( canWearEquip(selectedEntity, itemIndex) )
					{
						butMonsterHelm = newButton();
						strcpy(butMonsterHelm->label, tmpStr);
						butMonsterHelm->x = pad_x4 - 10;
						butMonsterHelm->y = pad_y2 + spacing - 4;
						butMonsterHelm->sizex = pad_x4 + pad_x3 - (pad_x4 - 10);
						butMonsterHelm->sizey = 16;
						butMonsterHelm->action = &buttonMonsterItems;
						butMonsterHelm->visible = 1;
						butMonsterHelm->focused = 1;
					}

					pad_y2 += spacing * 2;
					itemIndex = 6;
					if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 0 )
					{
						strcpy(tmpStr, "NULL");
					}
					else if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 1 )
					{
						strcpy(tmpStr, "RAND");
					}
					else
					{
						snprintf(tmpStr, 4, "%d", tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6]);
					}
					if ( canWearEquip(selectedEntity, itemIndex) )
					{
						butMonsterAmulet = newButton();
						strcpy(butMonsterAmulet->label, tmpStr);
						butMonsterAmulet->x = pad_x4 - 10;
						butMonsterAmulet->y = pad_y2 + spacing - 4;
						butMonsterAmulet->sizex = pad_x4 + pad_x3 - (pad_x4 - 10);
						butMonsterAmulet->sizey = 16;
						butMonsterAmulet->action = &buttonMonsterItems;
						butMonsterAmulet->visible = 1;
						butMonsterAmulet->focused = 1;
					}

					pad_y2 += spacing * 2;
					itemIndex = 3;
					if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 0 )
					{
						strcpy(tmpStr, "NULL");
					}
					else if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 1 )
					{
						strcpy(tmpStr, "RAND");
					}
					else
					{
						snprintf(tmpStr, 4, "%d", tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6]);
					}
					if ( canWearEquip(selectedEntity, itemIndex) )
					{
						butMonsterArmor = newButton();
						strcpy(butMonsterArmor->label, tmpStr);
						butMonsterArmor->x = pad_x4 - 10;
						butMonsterArmor->y = pad_y2 + spacing - 4;
						butMonsterArmor->sizex = pad_x4 + pad_x3 - (pad_x4 - 10);
						butMonsterArmor->sizey = 16;
						butMonsterArmor->action = &buttonMonsterItems;
						butMonsterArmor->visible = 1;
						butMonsterArmor->focused = 1;
					}

					pad_y2 += spacing * 2;
					itemIndex = 4;
					if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 0 )
					{
						strcpy(tmpStr, "NULL");
					}
					else if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 1 )
					{
						strcpy(tmpStr, "RAND");
					}
					else
					{
						snprintf(tmpStr, 4, "%d", tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6]);
					}
					if ( canWearEquip(selectedEntity, itemIndex) )
					{
						butMonsterBoots = newButton();
						strcpy(butMonsterBoots->label, tmpStr);
						butMonsterBoots->x = pad_x4 - 10;
						butMonsterBoots->y = pad_y2 + spacing - 4;
						butMonsterBoots->sizex = pad_x4 + pad_x3 - (pad_x4 - 10);
						butMonsterBoots->sizey = 16;
						butMonsterBoots->action = &buttonMonsterItems;
						butMonsterBoots->visible = 1;
						butMonsterBoots->focused = 1;
					}

					pad_y2 = suby1 + 28 + 2 * spacing; //reset y coord
					pad_y2 += 16;
					pad_x4 -= 64;
					itemIndex = 7;
					if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 0 )
					{
						strcpy(tmpStr, "NULL");
					}
					else if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 1 )
					{
						strcpy(tmpStr, "RAND");
					}
					else
					{
						snprintf(tmpStr, 4, "%d", tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6]);
					}

					if ( canWearEquip(selectedEntity, itemIndex) )
					{
						butMonsterCloak = newButton();
						strcpy(butMonsterCloak->label, tmpStr);
						butMonsterCloak->x = pad_x4 - 10;
						butMonsterCloak->y = pad_y2 + spacing - 4;
						butMonsterCloak->sizex = pad_x4 + pad_x3 - (pad_x4 - 10);
						butMonsterCloak->sizey = 16;
						butMonsterCloak->action = &buttonMonsterItems;
						butMonsterCloak->visible = 1;
						butMonsterCloak->focused = 1;
					}
					
					pad_x4 += 64 * 2;
					itemIndex = 8;

					if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 0 )
					{
						strcpy(tmpStr, "NULL");
					}
					else if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 1 )
					{
						strcpy(tmpStr, "RAND");
					}
					else
					{
						snprintf(tmpStr, 4, "%d", tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6]);
					}

					if ( canWearEquip(selectedEntity, itemIndex) )
					{
						butMonsterMask = newButton();
						strcpy(butMonsterMask->label, tmpStr);
						butMonsterMask->x = pad_x4 - 10;
						butMonsterMask->y = pad_y2 + spacing - 4;
						butMonsterMask->sizex = pad_x4 + pad_x3 - (pad_x4 - 10);
						butMonsterMask->sizey = 16;
						butMonsterMask->action = &buttonMonsterItems;
						butMonsterMask->visible = 1;
						butMonsterMask->focused = 1;
					}

					pad_y2 += spacing * 2;
					itemIndex = 2;
					if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 0 )
					{
						strcpy(tmpStr, "NULL");
					}
					else if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 1 )
					{
						strcpy(tmpStr, "RAND");
					}
					else
					{
						snprintf(tmpStr, 4, "%d", tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6]);
					}
					if ( canWearEquip(selectedEntity, itemIndex) )
					{
						butMonsterShield = newButton();
						strcpy(butMonsterShield->label, tmpStr);
						butMonsterShield->x = pad_x4 - 10;
						butMonsterShield->y = pad_y2 + spacing - 4;
						butMonsterShield->sizex = pad_x4 + pad_x3 - (pad_x4 - 10);
						butMonsterShield->sizey = 16;
						butMonsterShield->action = &buttonMonsterItems;
						butMonsterShield->visible = 1;
						butMonsterShield->focused = 1;
					}

					pad_x4 -= 64 * 2;
					itemIndex = 1;
					if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 0 )
					{
						strcpy(tmpStr, "NULL");
					}
					else if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 1 )
					{
						strcpy(tmpStr, "RAND");
					}
					else
					{
						snprintf(tmpStr, 4, "%d", tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6]);
					}
					if ( canWearEquip(selectedEntity, itemIndex) )
					{
						butMonsterWeapon = newButton();
						strcpy(butMonsterWeapon->label, tmpStr);
						butMonsterWeapon->x = pad_x4 - 10;
						butMonsterWeapon->y = pad_y2 + spacing - 4;
						butMonsterWeapon->sizex = pad_x4 + pad_x3 - (pad_x4 - 10);
						butMonsterWeapon->sizey = 16;
						butMonsterWeapon->action = &buttonMonsterItems;
						butMonsterWeapon->visible = 1;
						butMonsterWeapon->focused = 1;
					}

					pad_y2 += spacing * 2;
					itemIndex = 5;
					if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 0 )
					{
						strcpy(tmpStr, "NULL");
					}
					else if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 1 )
					{
						strcpy(tmpStr, "RAND");
					}
					else
					{
						snprintf(tmpStr, 4, "%d", tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6]);
					}

					if ( canWearEquip(selectedEntity, itemIndex) )
					{
						butMonsterRing = newButton();
						strcpy(butMonsterRing->label, tmpStr);
						butMonsterRing->x = pad_x4 - 10;
						butMonsterRing->y = pad_y2 + spacing - 4;
						butMonsterRing->sizex = pad_x4 + pad_x3 - (pad_x4 - 10);
						butMonsterRing->sizey = 16;
						butMonsterRing->action = &buttonMonsterItems;
						butMonsterRing->visible = 1;
						butMonsterRing->focused = 1;
					}
					pad_x4 += 64 * 2;
					itemIndex = 9;
					if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 0 )
					{
						strcpy(tmpStr, "NULL");
					}
					else if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 1 )
					{
						strcpy(tmpStr, "RAND");
					}
					else
					{
						snprintf(tmpStr, 4, "%d", tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6]);
					}
					if ( canWearEquip(selectedEntity, itemIndex) )
					{
						butMonsterGloves = newButton();
						strcpy(butMonsterGloves->label, tmpStr);
						butMonsterGloves->x = pad_x4 - 10;
						butMonsterGloves->y = pad_y2 + spacing - 4;
						butMonsterGloves->sizex = pad_x4 + pad_x3 - (pad_x4 - 10);
						butMonsterGloves->sizey = 16;
						butMonsterGloves->action = &buttonMonsterItems;
						butMonsterGloves->visible = 1;
						butMonsterGloves->focused = 1;
					}
					
					pad_y2 += 32 + spacing * 2;
					itemIndex = 12;
					if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 0 )
					{
						strcpy(tmpStr, "NULL");
					}
					else if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 1 )
					{
						strcpy(tmpStr, "RAND");
					}
					else
					{
						snprintf(tmpStr, 4, "%d", tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6]);
					}
					butMonsterItem3 = newButton();
					strcpy(butMonsterItem3->label, tmpStr);
					butMonsterItem3->x = pad_x4 - 10;
					butMonsterItem3->y = pad_y2 + spacing - 4;
					butMonsterItem3->sizex = pad_x4 + pad_x3 - (pad_x4 - 10);
					butMonsterItem3->sizey = 16;
					butMonsterItem3->action = &buttonMonsterItems;
					butMonsterItem3->visible = 1;
					butMonsterItem3->focused = 1;

					itemIndex = 15;
					if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 0 )
					{
						strcpy(tmpStr, "NULL");
					}
					else if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 1 )
					{
						strcpy(tmpStr, "RAND");
					}
					else
					{
						snprintf(tmpStr, 4, "%d", tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6]);
					}
					butMonsterItem6 = newButton();
					strcpy(butMonsterItem6->label, tmpStr);
					butMonsterItem6->x = pad_x4 - 10;
					butMonsterItem6->y = pad_y2 + 2 * spacing - 4;
					butMonsterItem6->sizex = pad_x4 + pad_x3 - (pad_x4 - 10);
					butMonsterItem6->sizey = 16;
					butMonsterItem6->action = &buttonMonsterItems;
					butMonsterItem6->visible = 1;
					butMonsterItem6->focused = 1;

					pad_x4 -= 64;
					itemIndex = 11;
					if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 0 )
					{
						strcpy(tmpStr, "NULL");
					}
					else if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 1 )
					{
						strcpy(tmpStr, "RAND");
					}
					else
					{
						snprintf(tmpStr, 4, "%d", tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6]);
					}
					butMonsterItem2 = newButton();
					strcpy(butMonsterItem2->label, tmpStr);
					butMonsterItem2->x = pad_x4 - 10;
					butMonsterItem2->y = pad_y2 + spacing - 4;
					butMonsterItem2->sizex = pad_x4 + pad_x3 - (pad_x4 - 10);
					butMonsterItem2->sizey = 16;
					butMonsterItem2->action = &buttonMonsterItems;
					butMonsterItem2->visible = 1;
					butMonsterItem2->focused = 1;

					itemIndex = 14;
					if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 0 )
					{
						strcpy(tmpStr, "NULL");
					}
					else if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 1 )
					{
						strcpy(tmpStr, "RAND");
					}
					else
					{
						snprintf(tmpStr, 4, "%d", tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6]);
					}
					butMonsterItem5 = newButton();
					strcpy(butMonsterItem5->label, tmpStr);
					butMonsterItem5->x = pad_x4 - 10;
					butMonsterItem5->y = pad_y2 + 2 * spacing - 4;
					butMonsterItem5->sizex = pad_x4 + pad_x3 - (pad_x4 - 10);
					butMonsterItem5->sizey = 16;
					butMonsterItem5->action = &buttonMonsterItems;
					butMonsterItem5->visible = 1;
					butMonsterItem5->focused = 1;

					pad_x4 -= 64;
					itemIndex = 10;
					if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 0 )
					{
						strcpy(tmpStr, "NULL");
					}
					else if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 1 )
					{
						strcpy(tmpStr, "RAND");
					}
					else
					{
						snprintf(tmpStr, 4, "%d", tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6]);
					}
					butMonsterItem1 = newButton();
					strcpy(butMonsterItem1->label, tmpStr);
					butMonsterItem1->x = pad_x4 - 10;
					butMonsterItem1->y = pad_y2 + spacing - 4;
					butMonsterItem1->sizex = pad_x4 + pad_x3 - (pad_x4 - 10);
					butMonsterItem1->sizey = 16;
					butMonsterItem1->action = &buttonMonsterItems;
					butMonsterItem1->visible = 1;
					butMonsterItem1->focused = 1;

					itemIndex = 13;
					if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 0 )
					{
						strcpy(tmpStr, "NULL");
					} 
					else if ( tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6] == 1 )
					{
						strcpy(tmpStr, "RAND");
					}
					else
					{
						snprintf(tmpStr, 4, "%d", tmpSpriteStats->EDITOR_ITEMS[itemIndex * 6]);
					}
					butMonsterItem4 = newButton();
					strcpy(butMonsterItem4->label, tmpStr);
					butMonsterItem4->x = pad_x4 - 10;
					butMonsterItem4->y = pad_y2 + 2 * spacing - 4;
					butMonsterItem4->sizex = pad_x4 + pad_x3 - (pad_x4 - 10);
					butMonsterItem4->sizey = 16;
					butMonsterItem4->action = &buttonMonsterItems;
					butMonsterItem4->visible = 1;
					butMonsterItem4->focused = 1;
				}
				break;
			case 3: //items
				butItemOK = newButton();
				strcpy(butItemOK->label, "  OK  ");
				butItemOK->x = subx2 - 128;
				butItemOK->y = suby2 - 24;
				butItemOK->sizex = 56;
				butItemOK->sizey = 16;
				butItemOK->action = &buttonSpritePropertiesConfirm;
				butItemOK->visible = 1;
				butItemOK->focused = 1;

				butItemCancel = newButton();
				strcpy(butItemCancel->label, "Cancel");
				butItemCancel->x = subx2 - 64;
				butItemCancel->y = suby2 - 24;
				butItemCancel->sizex = 56;
				butItemCancel->sizey = 16;
				butItemCancel->action = &buttonCloseSpriteSubwindow;
				butItemCancel->visible = 1;
				butItemCancel->focused = 1;

				butItemX = newButton();
				strcpy(butItemX->label, "X");
				butItemX->x = subx2 - 16;
				butItemX->y = suby1;
				butItemX->sizex = 16;
				butItemX->sizey = 16;
				butItemX->action = &buttonCloseSpriteSubwindow;
				butItemX->visible = 1;
				butItemX->focused = 1;
				break;
			default:
				button = newButton();
				strcpy(button->label, "  OK  ");
				button->x = subx2 - 64;
				button->y = suby2 - 48;
				button->sizex = 56;
				button->sizey = 16;
				button->action = &buttonSpritePropertiesConfirm;
				button->visible = 1;
				button->focused = 1;
				break;
		}

		if ( spriteType != 1 && spriteType != 3 )
		{
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
	Stat* tmpSpriteStats = NULL;
	button_t* button = NULL;
	if ( selectedEntity != NULL )
	{
		int spriteType = checkSpriteType(selectedEntity->sprite);
		switch (spriteType)
		{
			case 1: //monsters
				tmpSpriteStats = selectedEntity->getStats();
				if ( tmpSpriteStats != nullptr )
				{
					if ( my == butMonsterItemOK )
					{
						tmpSpriteStats->EDITOR_ITEMS[(itemSlotSelected) * 6] = (Sint32)atoi(spriteProperties[0]);
						tmpSpriteStats->EDITOR_ITEMS[(itemSlotSelected) * 6 + 1] = (Sint32)atoi(spriteProperties[1]);
						if ( strcmp(spriteProperties[2], "00") == 0 )
						{
							selectedEntity->skill[12] = 10; //bless random
						}
						else
						{
							tmpSpriteStats->EDITOR_ITEMS[(itemSlotSelected) * 6 + 2] = (Sint32)atoi(spriteProperties[2]); //bless
						}
						if ( strcmp(spriteProperties[3], "0") == 0 )
						{
							tmpSpriteStats->EDITOR_ITEMS[(itemSlotSelected) * 6 + 2] = 1; //reset quantity to 1
						}
						if ( strcmp(spriteProperties[3], "0") == 0 )
						{
							tmpSpriteStats->EDITOR_ITEMS[(itemSlotSelected) * 6 + 3] = 1; //reset quantity to 1
						}
						else
						{
							tmpSpriteStats->EDITOR_ITEMS[(itemSlotSelected) * 6 + 3] = (Sint32)atoi(spriteProperties[3]); //quantity
						}
						tmpSpriteStats->EDITOR_ITEMS[(itemSlotSelected) * 6 + 4] = (Sint32)atoi(spriteProperties[4]);
						tmpSpriteStats->EDITOR_ITEMS[(itemSlotSelected) * 6 + 5] = (Sint32)atoi(spriteProperties[5]);
						newwindow = 2;
						
						/*button = newButton();
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

						button = newButton();
						strcpy(button->label, "  OK  ");
						button->x = subx2 - 64;
						button->y = suby2 - 48;
						button->sizex = 56;
						button->sizey = 16;
						button->action = &buttonSpritePropertiesConfirm;
						button->visible = 1;
						button->focused = 1;*/

						//butItemOK->visible = 0;
						//butItemCancel->visible = 0;
						//butItemX->visible = 0;
						if ( butMonsterItemOK != NULL )
						{
							butMonsterItemOK->visible = 0;
						}
						if ( butMonsterItemX != NULL )
						{
							butMonsterItemX->visible = 0;
						}
						if ( butMonsterItemCancel != NULL )
						{
							butMonsterItemCancel->visible = 0;
						}
					}
					else
					{
						strcpy(tmpSpriteStats->name, spriteProperties[0]);
						tmpSpriteStats->MAXHP = (Sint32)atoi(spriteProperties[1]);
						tmpSpriteStats->HP = (Sint32)atoi(spriteProperties[2]);
						tmpSpriteStats->MAXMP = (Sint32)atoi(spriteProperties[3]);
						tmpSpriteStats->MP = (Sint32)atoi(spriteProperties[4]);
						tmpSpriteStats->LVL = (Sint32)atoi(spriteProperties[5]);
						tmpSpriteStats->GOLD = (Sint32)atoi(spriteProperties[6]);
						tmpSpriteStats->STR = (Sint32)atoi(spriteProperties[7]);
						tmpSpriteStats->DEX = (Sint32)atoi(spriteProperties[8]);
						tmpSpriteStats->CON = (Sint32)atoi(spriteProperties[9]);
						tmpSpriteStats->INT = (Sint32)atoi(spriteProperties[10]);
						tmpSpriteStats->PER = (Sint32)atoi(spriteProperties[11]);
						tmpSpriteStats->CHR = (Sint32)atoi(spriteProperties[12]);

						tmpSpriteStats->RANDOM_MAXHP = (Sint32)atoi(spriteProperties[13]) - tmpSpriteStats->MAXHP;
						if ( tmpSpriteStats->RANDOM_MAXHP < 0 )
						{
							tmpSpriteStats->RANDOM_MAXHP = 0;
						}
						tmpSpriteStats->RANDOM_HP = (Sint32)atoi(spriteProperties[14]) - tmpSpriteStats->HP;
						if ( tmpSpriteStats->RANDOM_HP < 0 )
						{
							tmpSpriteStats->RANDOM_HP = 0;
						}
						tmpSpriteStats->RANDOM_MAXMP = (Sint32)atoi(spriteProperties[15]) - tmpSpriteStats->MAXMP;
						if ( tmpSpriteStats->RANDOM_MAXMP < 0 )
						{
							tmpSpriteStats->RANDOM_MAXMP = 0;
						}
						tmpSpriteStats->RANDOM_MP = (Sint32)atoi(spriteProperties[16]) - tmpSpriteStats->MP;
						if ( tmpSpriteStats->RANDOM_MP < 0 )
						{
							tmpSpriteStats->RANDOM_MP = 0;
						}
						tmpSpriteStats->RANDOM_LVL = (Sint32)atoi(spriteProperties[17]) - tmpSpriteStats->LVL;
						if ( tmpSpriteStats->RANDOM_LVL < 0 )
						{
							tmpSpriteStats->RANDOM_LVL = 0;
						}
						tmpSpriteStats->RANDOM_GOLD = (Sint32)atoi(spriteProperties[18]) - tmpSpriteStats->GOLD;
						if ( tmpSpriteStats->RANDOM_GOLD < 0 )
						{
							tmpSpriteStats->RANDOM_GOLD = 0;
						}
						tmpSpriteStats->RANDOM_STR = (Sint32)atoi(spriteProperties[19]) - tmpSpriteStats->STR;
						if ( tmpSpriteStats->RANDOM_STR < 0 )
						{
							tmpSpriteStats->RANDOM_STR = 0;
						}
						tmpSpriteStats->RANDOM_DEX = (Sint32)atoi(spriteProperties[20]) - tmpSpriteStats->DEX;
						if ( tmpSpriteStats->RANDOM_DEX < 0 )
						{
							tmpSpriteStats->RANDOM_DEX = 0;
						}
						tmpSpriteStats->RANDOM_CON = (Sint32)atoi(spriteProperties[21]) - tmpSpriteStats->CON;
						if ( tmpSpriteStats->RANDOM_CON < 0 )
						{
							tmpSpriteStats->RANDOM_CON = 0;
						}
						tmpSpriteStats->RANDOM_INT = (Sint32)atoi(spriteProperties[22]) - tmpSpriteStats->INT;
						if ( tmpSpriteStats->RANDOM_INT < 0 )
						{
							tmpSpriteStats->RANDOM_INT = 0;
						}
						tmpSpriteStats->RANDOM_PER = (Sint32)atoi(spriteProperties[23]) - tmpSpriteStats->PER;
						if ( tmpSpriteStats->RANDOM_PER < 0 )
						{
							tmpSpriteStats->RANDOM_PER = 0;
						}
						tmpSpriteStats->RANDOM_CHR = (Sint32)atoi(spriteProperties[24]) - tmpSpriteStats->CHR;
						if ( tmpSpriteStats->RANDOM_CHR < 0 )
						{
							tmpSpriteStats->RANDOM_CHR = 0;
						}
					}
				}
				break;
			case 2: //chest
				selectedEntity->yaw = (real_t)atoi(spriteProperties[0]);
				selectedEntity->skill[9] = (Sint32)atoi(spriteProperties[1]);
				break;
			case 3: //items
				selectedEntity->skill[10] = (Sint32)atoi(spriteProperties[0]); //id
				selectedEntity->skill[11] = (Sint32)atoi(spriteProperties[1]); //status
				if ( strcmp(spriteProperties[2], "00") == 0 )
				{
					selectedEntity->skill[12] = 10; //bless random
				}
				else
				{
					selectedEntity->skill[12] = (Sint32)atoi(spriteProperties[2]); //bless
				}
				if ( strcmp(spriteProperties[3], "0") == 0 )
				{
					selectedEntity->skill[13] = 1; //reset quantity to 1
				}
				else
				{
					selectedEntity->skill[13] = (Sint32)atoi(spriteProperties[3]); //quantity
				}
				selectedEntity->skill[15] = (Sint32)atoi(spriteProperties[4]); //identified
				break;
			default:
				break;
		}
		strcpy(message, "                 Modified sprite properties.");
		messagetime = 60;
	}

	if ( my == butMonsterItemOK && tmpSpriteStats != NULL )
	{
		copyMonsterStatToPropertyStrings(tmpSpriteStats);
		inputstr = spriteProperties[0];
		initMonsterPropertiesWindow();

		buttonSpriteProperties(my);
		itemSlotSelected = -1;
	}
	else
	{
		buttonCloseSpriteSubwindow(my);
	}
}

void buttonCloseSpriteSubwindow(button_t* my)
{
	Stat* tmpSpriteStats = NULL;
	// close window
	if ( my == butMonsterItemCancel || my == butMonsterItemX )
	{
		if ( selectedEntity != NULL )
		{
			tmpSpriteStats = selectedEntity->getStats();
		}
		if ( tmpSpriteStats != NULL )
		{
			copyMonsterStatToPropertyStrings(tmpSpriteStats);
			inputstr = spriteProperties[0];
			initMonsterPropertiesWindow();

			buttonSpriteProperties(my);
			itemSlotSelected = -1;
			if ( butMonsterItemOK != NULL )
			{
				butMonsterItemOK->visible = 0;
			}
			if ( butMonsterItemX != NULL )
			{
				butMonsterItemX->visible = 0;
			}
			if ( butMonsterItemCancel != NULL )
			{
				butMonsterItemCancel->visible = 0;
			}
		}
	}
	else {
		selectedEntity = NULL;
		newwindow = 0;
		subwindow = 0;
		editproperty = 0;
		spritepalette = 0;
	}
}

void buttonMonsterItems(button_t* my)
{
	int spacing = 20;
	int pad_y2 = suby1 + 28 + 2 * spacing;
	int pad_x3 = 40;
	int pad_x4 = subx2 - 112;
	char tmpStr[32] = "";
	button_t* button = NULL;

	itemSelect = 0;

	inputstr = spriteProperties[0];
	cursorflash = ticks;
	menuVisible = 0;
	subwindow = 1;
	slidery = 0;
	subx1 = xres / 2 - 200;
	subx2 = xres / 2 + 200;
	suby1 = yres / 2 - 140;
	suby2 = yres / 2 + 140;
	strcpy(subtext, "Monster Item Properties:");

	Stat* tmpSpriteStats = selectedEntity->getStats();


	if ( my == butMonsterHelm )
	{
		itemSlotSelected = 0;
	}
	else if ( my == butMonsterWeapon )
	{
		itemSlotSelected = 1;
	}
	else if ( my == butMonsterShield )
	{
		itemSlotSelected = 2;
	}
	else if ( my == butMonsterArmor )
	{
		itemSlotSelected = 3;
	}
	else if ( my == butMonsterBoots )
	{
		itemSlotSelected = 4;
	}
	else if ( my == butMonsterRing )
	{
		itemSlotSelected = 5;
	}
	else if ( my == butMonsterAmulet )
	{
		itemSlotSelected = 6;
	}
	else if ( my == butMonsterCloak )
	{
		itemSlotSelected = 7;
	}
	else if ( my == butMonsterMask )
	{
		itemSlotSelected = 8;
	}
	else if ( my == butMonsterGloves )
	{
		itemSlotSelected = 9;
	}
	else if ( my == butMonsterItem1 )
	{
		itemSlotSelected = 10;
	}
	else if ( my == butMonsterItem2 )
	{
		itemSlotSelected = 11;
	}
	else if ( my == butMonsterItem3 )
	{
		itemSlotSelected = 12;
	}
	else if ( my == butMonsterItem4 )
	{
		itemSlotSelected = 13;
	}
	else if ( my == butMonsterItem5 )
	{
		itemSlotSelected = 14;
	}
	else if ( my == butMonsterItem6 )
	{
		itemSlotSelected = 15;
	}
	else
	{
		itemSlotSelected = -1;
	}

	newwindow = 5;


	if ( butMonsterHelm != NULL )
	{
		butMonsterHelm->visible = 0;
	}
	if ( butMonsterWeapon != NULL )
	{
		butMonsterWeapon->visible = 0;
	}
	if ( butMonsterShield != NULL )
	{
		butMonsterShield->visible = 0;
	}
	if ( butMonsterArmor != NULL )
	{
		butMonsterArmor->visible = 0;
	}
	if ( butMonsterRing != NULL )
	{
		butMonsterRing->visible = 0;
	}
	if ( butMonsterAmulet != NULL )
	{
		butMonsterAmulet->visible = 0;
	}
	if ( butMonsterBoots != NULL )
	{
		butMonsterBoots->visible = 0;
	}
	if ( butMonsterCloak != NULL )
	{
		butMonsterCloak->visible = 0;
	}
	if ( butMonsterMask != NULL )
	{
		butMonsterMask->visible = 0;
	}
	if ( butMonsterGloves != NULL )
	{
		butMonsterGloves->visible = 0;
	}
	if ( butMonsterItem1 != NULL )
	{
		butMonsterItem1->visible = 0;
	}
	if ( butMonsterItem2 != NULL )
	{
		butMonsterItem2->visible = 0;
	}
	if ( butMonsterItem3 != NULL )
	{
		butMonsterItem3->visible = 0;
	}
	if ( butMonsterItem4 != NULL )
	{
		butMonsterItem4->visible = 0;
	}
	if ( butMonsterItem5 != NULL )
	{
		butMonsterItem5->visible = 0;
	}
	if ( butMonsterItem6 != NULL )
	{
		butMonsterItem6->visible = 0;
	}
	if ( butMonsterOK != NULL )
	{
		butMonsterOK->visible = 0;
	}
	if ( butMonsterCancel != NULL )
	{
		butMonsterCancel->visible = 0;
	}
	if ( butMonsterX != NULL )
	{
		butMonsterX->visible = 0;
	}
	snprintf(spriteProperties[0], 5, "%d", tmpSpriteStats->EDITOR_ITEMS[itemSlotSelected * 6 + 0]);
	snprintf(spriteProperties[1], 5, "%d", tmpSpriteStats->EDITOR_ITEMS[itemSlotSelected * 6 + 1]);
	if ( (int)tmpSpriteStats->EDITOR_ITEMS[itemSlotSelected * 6 + 2] == 10 )
	{
		strcpy(spriteProperties[2], "00"); //bless random
	}
	else
	{
		snprintf(spriteProperties[2], 4, "%d", (int)tmpSpriteStats->EDITOR_ITEMS[itemSlotSelected * 6 + 2]); //bless
	}
	snprintf(spriteProperties[3], 5, "%d", tmpSpriteStats->EDITOR_ITEMS[itemSlotSelected * 6 + 3]);
	snprintf(spriteProperties[4], 5, "%d", tmpSpriteStats->EDITOR_ITEMS[itemSlotSelected * 6 + 4]);
	snprintf(spriteProperties[5], 5, "%d", tmpSpriteStats->EDITOR_ITEMS[itemSlotSelected * 6 + 5]);

	butMonsterItemOK = newButton();
	strcpy(butMonsterItemOK->label, "  OK  ");
	butMonsterItemOK->x = subx2 - 128;
	butMonsterItemOK->y = suby2 - 24;
	butMonsterItemOK->sizex = 56;
	butMonsterItemOK->sizey = 16;
	butMonsterItemOK->action = &buttonSpritePropertiesConfirm;
	butMonsterItemOK->visible = 1;
	butMonsterItemOK->focused = 1;

	butMonsterItemCancel = newButton();
	strcpy(butMonsterItemCancel->label, "Cancel");
	butMonsterItemCancel->x = subx2 - 64;
	butMonsterItemCancel->y = suby2 - 24;
	butMonsterItemCancel->sizex = 56;
	butMonsterItemCancel->sizey = 16;
	butMonsterItemCancel->action = &buttonCloseSpriteSubwindow;
	butMonsterItemCancel->visible = 1;
	butMonsterItemCancel->focused = 1;

	butMonsterItemX = newButton();
	strcpy(butMonsterItemX->label, "X");
	butMonsterItemX->x = subx2 - 16;
	butMonsterItemX->y = suby1;
	butMonsterItemX->sizex = 16;
	butMonsterItemX->sizey = 16;
	butMonsterItemX->action = &buttonCloseSpriteSubwindow;
	butMonsterItemX->visible = 1;
	butMonsterItemX->focused = 1;
}

void initMonsterPropertiesWindow() {
	cursorflash = ticks;
	menuVisible = 0;
	subwindow = 1;
	newwindow = 2;
	subx1 = xres / 2 - 200;
	subx2 = xres / 2 + 200;
	suby1 = yres / 2 - 180;
	suby2 = yres / 2 + 180;
	strcpy(subtext, "Sprite properties: ");
	strcat(subtext, spriteEditorName(selectedEntity->sprite));
}

void copyMonsterStatToPropertyStrings(Stat* tmpSpriteStats)
{
	if ( tmpSpriteStats != NULL )
	{
		strcpy(spriteProperties[0], tmpSpriteStats->name);
		snprintf(spriteProperties[1], 5, "%d", tmpSpriteStats->MAXHP);
		snprintf(spriteProperties[2], 5, "%d", tmpSpriteStats->HP);
		snprintf(spriteProperties[3], 5, "%d", tmpSpriteStats->MAXMP);
		snprintf(spriteProperties[4], 5, "%d", tmpSpriteStats->MP);
		snprintf(spriteProperties[5], 4, "%d", tmpSpriteStats->LVL);
		snprintf(spriteProperties[6], 4, "%d", tmpSpriteStats->GOLD);
		snprintf(spriteProperties[7], 4, "%d", tmpSpriteStats->STR);
		snprintf(spriteProperties[8], 4, "%d", tmpSpriteStats->DEX);
		snprintf(spriteProperties[9], 4, "%d", tmpSpriteStats->CON);
		snprintf(spriteProperties[10], 4, "%d", tmpSpriteStats->INT);
		snprintf(spriteProperties[11], 4, "%d", tmpSpriteStats->PER);
		snprintf(spriteProperties[12], 4, "%d", tmpSpriteStats->CHR);
		snprintf(spriteProperties[13], 5, "%d", tmpSpriteStats->RANDOM_MAXHP + tmpSpriteStats->MAXHP);
		snprintf(spriteProperties[14], 5, "%d", tmpSpriteStats->RANDOM_HP + tmpSpriteStats->HP);
		snprintf(spriteProperties[15], 5, "%d", tmpSpriteStats->RANDOM_MAXMP + tmpSpriteStats->MAXMP);
		snprintf(spriteProperties[16], 5, "%d", tmpSpriteStats->RANDOM_MP + tmpSpriteStats->MP);
		snprintf(spriteProperties[17], 4, "%d", tmpSpriteStats->RANDOM_LVL + tmpSpriteStats->LVL);
		snprintf(spriteProperties[18], 4, "%d", tmpSpriteStats->RANDOM_GOLD + tmpSpriteStats->GOLD);
		snprintf(spriteProperties[19], 4, "%d", tmpSpriteStats->RANDOM_STR + tmpSpriteStats->STR);
		snprintf(spriteProperties[20], 4, "%d", tmpSpriteStats->RANDOM_DEX + tmpSpriteStats->DEX);
		snprintf(spriteProperties[21], 4, "%d", tmpSpriteStats->RANDOM_CON + tmpSpriteStats->CON);
		snprintf(spriteProperties[22], 4, "%d", tmpSpriteStats->RANDOM_INT + tmpSpriteStats->INT);
		snprintf(spriteProperties[23], 4, "%d", tmpSpriteStats->RANDOM_PER + tmpSpriteStats->PER);
		snprintf(spriteProperties[24], 4, "%d", tmpSpriteStats->RANDOM_CHR + tmpSpriteStats->CHR);
	}
	return;
}
