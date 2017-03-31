/*-------------------------------------------------------------------------------

	BARONY
	File: editor.hpp
	Desc: header file for the editor

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

// for file browsing
#include <dirent.h>

static const unsigned int MAXWIDTH = 2000;
static const unsigned int MAXHEIGHT = 2000;
static const unsigned int MINWIDTH = 2;
static const unsigned int MINHEIGHT =  2;

extern int drawlayer, drawx, drawy, odrawx, odrawy;
extern int alllayers;
extern int scroll;
extern char layerstatus[20];
extern int menuVisible;
extern int subwindow;
extern int subx1, subx2, suby1, suby2;
extern char subtext[1024];
extern int toolbox;
extern int statusbar;
extern int viewsprites;
extern int showgrid;
extern int selectedTile;
extern int tilepalette;
extern int spritepalette;
extern int mclick;
extern int selectedTool; // 0: point draw 1: brush 2: select 3: fill
extern int allowediting; // only turned on when the mouse is over paintable screen region
extern int openwindow, savewindow, newwindow;
extern int slidery, slidersize;
extern int menuDisappear;
extern int selectedFile;
extern char** d_names;
extern unsigned long d_names_length;
extern char filename[128];
extern char oldfilename[128];
extern char message[48];
extern int messagetime;
extern char widthtext[4], heighttext[4], nametext[32], authortext[32];
extern int editproperty;
extern SDL_Cursor* cursorArrow, *cursorPencil, *cursorBrush, *cursorSelect, *cursorFill;
extern int* palette;

// button definitions
extern button_t* butX;
extern button_t* but_;
extern button_t* butTilePalette;
extern button_t* butSprite;
extern button_t* butPoint;
extern button_t* butBrush;
extern button_t* butSelect;
extern button_t* butFill;
extern button_t* butFile;
extern button_t* butNew;
extern button_t* butOpen;
extern button_t* butSave;
extern button_t* butSaveAs;
extern button_t* butExit;
extern button_t* butEdit;
extern button_t* butCut;
extern button_t* butCopy;
extern button_t* butPaste;
extern button_t* butDelete;
extern button_t* butSelectAll;
extern button_t* butUndo;
extern button_t* butRedo;
extern button_t* butView;
extern button_t* butToolbox;
extern button_t* butStatusBar;
extern button_t* butAllLayers;
extern button_t* butViewSprites;
extern button_t* butGrid;
extern button_t* butFullscreen;
extern button_t* butMap;
extern button_t* butAttributes;
extern button_t* butClearMap;
extern button_t* butHelp;
extern button_t* butAbout;
extern button_t* but3DMode;

extern int menuVisible;
extern int toolbox;
extern int statusbar;
extern int viewsprites;
extern int alllayers;
extern int showgrid;
extern int selectedTile;
extern int tilepalette;
extern int spritepalette;
extern int selectedTool;
extern int openwindow, savewindow, newwindow;
extern int slidery;
extern int slidersize;
extern int selectedFile;
extern int messagetime;
extern char message[48];
extern Uint32 cursorflash;
extern char widthtext[4], heighttext[4], nametext[32], authortext[32];
extern char spriteProperties[16][128];
extern int editproperty;
extern bool mode3d;
extern bool selectingspace;
extern int selectedarea_x1, selectedarea_x2;
extern int selectedarea_y1, selectedarea_y2;
extern bool selectedarea;
extern bool pasting;
extern map_t copymap;
extern list_t undolist;

// fps
extern bool showfps;
extern real_t t, ot, frameval[AVERAGEFRAMES];
extern Uint32 cycles, pingtime;
extern Uint32 timesync;
extern real_t fps;

extern Sint32 ocamx, ocamy;

// undo/redo funcs
void makeUndo();
void clearUndos();
void undo();
void redo();

// function prototypes for buttons.c:
void buttonExit(button_t* my);
void buttonExitConfirm(button_t* my);
void buttonIconify(button_t* my);
void buttonTilePalette(button_t* my);
void buttonSprite(button_t* my);
void buttonPoint(button_t* my);
void buttonBrush(button_t* my);
void buttonSelect(button_t* my);
void buttonFill(button_t* my);
void buttonFile(button_t* my);
void buttonNewConfirm(button_t* my);
void buttonNew(button_t* my);
void buttonOpenConfirm(button_t* my);
void buttonOpen(button_t* my);
void buttonSaveConfirm(button_t* my);
void buttonSave(button_t* my);
void buttonSaveAs(button_t* my);
void buttonExit(button_t* my);
void buttonEdit(button_t* my);
void buttonCut(button_t* my);
void buttonCopy(button_t* my);
void buttonPaste(button_t* my);
void buttonDelete(button_t* my);
void buttonSelectAll(button_t* my);
void buttonUndo(button_t* my);
void buttonRedo(button_t* my);
void buttonView(button_t* my);
void buttonToolbox(button_t* my);
void buttonStatusBar(button_t* my);
void buttonAllLayers(button_t* my);
void buttonViewSprites(button_t* my);
void buttonGrid(button_t* my);
void button3DMode(button_t* my);
void buttonMap(button_t* my);
void buttonAttributes(button_t* my);
void buttonAttributesConfirm(button_t* my);
void buttonClearMap(button_t* my);
void buttonClearMapConfirm(button_t* my);
void buttonHelp(button_t* my);
void buttonAbout(button_t* my);
void buttonCloseSubwindow(button_t* my);
void buttonSpriteProperties(button_t* my);
void buttonSpritePropertiesConfirm(button_t* my);
void buttonCloseSpriteSubwindow(button_t* my);
void buttonMonsterItems(button_t* my);

extern char itemName[128];
extern int itemSelect;
extern int itemSlotSelected;

#define TICKS_PER_SECOND 50