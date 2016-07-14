/*-------------------------------------------------------------------------------

	BARONY
	File: interface.cpp
	Desc: contains code for game interface

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../messages.hpp"
#include "../entity.hpp"
#include "../magic/magic.hpp"
#include "interface.hpp"
#include "../monster.hpp"
#include "../items.hpp"
#include "../book.hpp"
#include "../sound.hpp"
#include "../shops.hpp"
#include "../menu.hpp"

Uint32 svFlags = 30;
SDL_Surface *backdrop_bmp = NULL;
SDL_Surface *status_bmp = NULL;
SDL_Surface *character_bmp = NULL;
SDL_Surface *hunger_bmp = NULL;
int textscroll = 0;
int attributespage = 0;
Item *invitems[4];
Item *invitemschest[4];
int inventorycategory=7; // inventory window defaults to wildcard
int itemscroll=0;
view_t camera_charsheet;

SDL_Surface *font12x12_small_bmp = NULL;
SDL_Surface *inventoryChest_bmp = NULL;
SDL_Surface *invclose_bmp = NULL;
SDL_Surface *invgraball_bmp = NULL;
SDL_Surface *button_bmp = NULL, *smallbutton_bmp = NULL, *invup_bmp = NULL, *invdown_bmp = NULL;
bool gui_clickdrag = FALSE;
int dragoffset_x = 0;
int dragoffset_y = 0;
int chestitemscroll = 0;
list_t chestInv;
int chestgui_offset_x = 0;
int chestgui_offset_y = 0;
bool dragging_chestGUI = FALSE;

//Identify GUI definitions.
bool identifygui_active = FALSE;
bool identifygui_appraising = FALSE;
int identifygui_offset_x = 0;
int identifygui_offset_y = 0;
bool dragging_identifyGUI = FALSE;
int identifyscroll = 0;
Item *identify_items[4];
SDL_Surface *identifyGUI_img;

//Remove curse GUI definitions.
bool removecursegui_active = FALSE;
bool removecursegui_appraising = FALSE;
int removecursegui_offset_x = 0;
int removecursegui_offset_y = 0;
bool dragging_removecurseGUI = FALSE;
int removecursescroll = 0;
Item *removecurse_items[4];
SDL_Surface *removecurseGUI_img;

SDL_Surface *rightsidebar_titlebar_img = NULL;
SDL_Surface *rightsidebar_slot_img = NULL;
SDL_Surface *rightsidebar_slot_highlighted_img = NULL;
SDL_Surface *rightsidebar_slot_grayedout_img = NULL;
int rightsidebar_height = 0;
int appraisal_timer = 0;
int appraisal_timermax = 0;
Uint32 appraisal_item = 0;

SDL_Surface *bookgui_img = NULL;
//SDL_Surface *nextpage_img = NULL;
//SDL_Surface *previouspage_img = NULL;
//SDL_Surface *bookclose_img = NULL;
node_t *book_page = NULL;
int bookgui_offset_x = 0;
int bookgui_offset_y = 0;
bool dragging_book_GUI = FALSE;
bool book_open = FALSE;
book_t *open_book = NULL;
Item *open_book_item = NULL;
//int book_characterspace_x = 0;
//int book_characterspace_y = 0;

SDL_Surface *book_highlighted_left_img = NULL;
SDL_Surface *book_highlighted_right_img = NULL;

int gui_mode = GUI_MODE_INVENTORY;

SDL_Surface *magicspellList_bmp = NULL;
SDL_Surface *spell_list_titlebar_bmp = NULL;
SDL_Surface *spell_list_gui_slot_bmp = NULL;
SDL_Surface *spell_list_gui_slot_highlighted_bmp = NULL;
SDL_Surface *textup_bmp = NULL;
SDL_Surface *textdown_bmp = NULL;
SDL_Surface *attributesleft_bmp = NULL;
SDL_Surface *attributesright_bmp = NULL;
SDL_Surface *attributesleftunclicked_bmp = NULL;
SDL_Surface *attributesrightunclicked_bmp = NULL;
SDL_Surface *inventory_bmp = NULL, *inventoryoption_bmp = NULL, *inventoryoptionChest_bmp = NULL, *equipped_bmp = NULL;
//SDL_Surface *category_bmp[NUMCATEGORIES];
SDL_Surface *shopkeeper_bmp = NULL;
SDL_Surface *damage_bmp = NULL;
int spellscroll = 0;
int magicspell_list_offset_x = 0;
int magicspell_list_offset_y = 0;
bool dragging_magicspell_list_GUI = FALSE;
int magic_GUI_state = 0;
SDL_Rect magic_gui_pos;
SDL_Surface *sustained_spell_generic_icon = NULL;

int buttonclick = 0;

hotbar_slot_t hotbar[NUM_HOTBAR_SLOTS];
SDL_Surface *hotbar_img = NULL;
SDL_Surface *hotbar_spell_img = NULL;
list_t damageIndicators;

bool auto_hotbar_new_items = TRUE;
bool disable_messages = FALSE;

bool loadInterfaceResources() {
	//General GUI images.
	font12x12_small_bmp=loadImage("images/system/font12x12_small.png");
	backdrop_bmp=loadImage("images/system/backdrop.png");
	button_bmp=loadImage("images/system/ButtonHighlighted.png");
	smallbutton_bmp=loadImage("images/system/SmallButtonHighlighted.png");
	invup_bmp=loadImage("images/system/InventoryUpHighlighted.png");
	invdown_bmp=loadImage("images/system/InventoryDownHighlighted.png");
	status_bmp = loadImage("images/system/StatusBar.png");
	character_bmp = loadImage("images/system/CharacterSheet.png");
	hunger_bmp = loadImage("images/system/Hunger.png");
	//textup_bmp = loadImage("images/system/TextBoxUpHighlighted.png");
	//textdown_bmp = loadImage("images/system/TextBoxDownHighlighted.png");
	attributesleft_bmp = loadImage("images/system/AttributesLeftHighlighted.png");
	attributesright_bmp = loadImage("images/system/AttributesRightHighlighted.png");
	attributesleftunclicked_bmp = loadImage("images/system/AttributesLeft.png");
	attributesrightunclicked_bmp = loadImage("images/system/AttributesRight.png");
	shopkeeper_bmp = loadImage("images/system/shopkeeper.png");
	damage_bmp = loadImage("images/system/damage.png");

	//Magic GUI images.
	magicspellList_bmp = loadImage("images/system/spellList.png");
	spell_list_titlebar_bmp = loadImage("images/system/spellListTitlebar.png");
	spell_list_gui_slot_bmp = loadImage("images/system/spellListSlot.png");
	spell_list_gui_slot_highlighted_bmp = loadImage("images/system/spellListSlotHighlighted.png");
	sustained_spell_generic_icon = loadImage("images/system/magic/channeled_spell.png");
	inventory_bmp=loadImage("images/system/Inventory.png");
	inventoryoption_bmp=loadImage("images/system/InventoryOption.png");
	inventory_mode_item_img = loadImage("images/system/inventory_mode_item.png");
	inventory_mode_item_highlighted_img = loadImage("images/system/inventory_mode_item_highlighted.png");
	inventory_mode_spell_img = loadImage("images/system/inventory_mode_spell.png");
	inventory_mode_spell_highlighted_img = loadImage("images/system/inventory_mode_spell_highlighted.png");
	equipped_bmp=loadImage("images/system/Equipped.png");
	//sky_bmp=scaleSurface(loadImage("images/system/sky.png"), 1280*(xres/320.0),468*(xres/320.0));
	/*category_bmp[0]=loadImage("images/system/Weapon.png");
	category_bmp[1]=loadImage("images/system/Armor.png");
	category_bmp[2]=loadImage("images/system/Amulet.png");
	category_bmp[3]=loadImage("images/system/Potion.png");
	category_bmp[4]=loadImage("images/system/Scroll.png");
	category_bmp[5]=loadImage("images/system/Magicstaff.png");
	category_bmp[6]=loadImage("images/system/Ring.png");
	category_bmp[7]=loadImage("images/system/Spellbook.png");
	category_bmp[8]=loadImage("images/system/Gem.png");
	category_bmp[9]=loadImage("images/system/Tool.png");
	category_bmp[10]=loadImage("images/system/Food.png");
	category_bmp[11]=loadImage("images/system/Spellbook.png");*/
	
	//Chest images..
	inventoryChest_bmp = loadImage("images/system/InventoryChest.png");
	inventoryoptionChest_bmp = loadImage("images/system/InventoryOptionChest.png");
	invclose_bmp = loadImage("images/system/InventoryCloseHighlighted.png");
	invgraball_bmp = loadImage("images/system/InventoryChestGraballHighlighted.png");

	//Identify GUI images...
	identifyGUI_img = loadImage("images/system/identifyGUI.png");

	/*rightsidebar_titlebar_img = loadImage("images/system/rightSidebarTitlebar.png");
	if (!rightsidebar_titlebar_img) {
		printlog( "Failed to load \"images/system/rightSidebarTitlebar.png\".");
		return FALSE;
	}
	rightsidebar_slot_img = loadImage("images/system/rightSidebarSlot.png");
	if (!rightsidebar_slot_img) {
		printlog( "Failed to load \"images/system/rightSidebarSlot.png\".");
		return FALSE;
	}
	rightsidebar_slot_highlighted_img = loadImage("images/system/rightSidebarSlotHighlighted.png");
	if (!rightsidebar_slot_highlighted_img) {
		printlog( "Failed to load \"images/system/rightSidebarSlotHighlighted.png\".");
		return FALSE;
	}*/
	rightsidebar_titlebar_img = spell_list_titlebar_bmp;
	rightsidebar_slot_img = spell_list_gui_slot_bmp;
	rightsidebar_slot_highlighted_img = spell_list_gui_slot_highlighted_bmp;
	rightsidebar_slot_grayedout_img = loadImage("images/system/rightSidebarSlotGrayedOut.png");

	bookgui_img = loadImage("images/system/book.png");
	//nextpage_img = loadImage("images/system/nextpage.png");
	//previouspage_img = loadImage("images/system/previouspage.png");
	//bookclose_img = loadImage("images/system/bookclose.png");

	book_highlighted_left_img = loadImage("images/system/bookpageleft-highlighted.png");
	book_highlighted_right_img = loadImage("images/system/bookpageright-highlighted.png");

	hotbar_img = loadImage("images/system/hotbar_slot.png");
	hotbar_spell_img = loadImage("images/system/magic/hotbar_spell.png");
	int i = 0;
	for (i = 0; i < NUM_HOTBAR_SLOTS; ++i) {
		hotbar[i].item = 0;
	}

	damageIndicators.first = NULL;
	damageIndicators.last = NULL;

	return TRUE;
}

void freeInterfaceResources() {
	//int c;

	if(font12x12_small_bmp)
		SDL_FreeSurface(font12x12_small_bmp);
	if(backdrop_bmp)
		SDL_FreeSurface(backdrop_bmp);
	if(status_bmp)
		SDL_FreeSurface(status_bmp);
	if(character_bmp)
		SDL_FreeSurface(character_bmp);
	if(hunger_bmp)
		SDL_FreeSurface(hunger_bmp);
	//if(textup_bmp)
		//SDL_FreeSurface(textup_bmp);
	//if(textdown_bmp)
		//SDL_FreeSurface(textdown_bmp);
	if(attributesleft_bmp)
		SDL_FreeSurface(attributesleft_bmp);
	if(attributesright_bmp)
		SDL_FreeSurface(attributesright_bmp);
	if(attributesleftunclicked_bmp)
		SDL_FreeSurface(attributesleftunclicked_bmp);
	if(attributesrightunclicked_bmp)
		SDL_FreeSurface(attributesrightunclicked_bmp);
	if(magicspellList_bmp)
		SDL_FreeSurface(magicspellList_bmp);
	if(spell_list_titlebar_bmp)
		SDL_FreeSurface(spell_list_titlebar_bmp);
	if(spell_list_gui_slot_bmp)
		SDL_FreeSurface(spell_list_gui_slot_bmp);
	if(spell_list_gui_slot_highlighted_bmp)
		SDL_FreeSurface(spell_list_gui_slot_highlighted_bmp);
	if (sustained_spell_generic_icon)
		SDL_FreeSurface(sustained_spell_generic_icon);
	if(invup_bmp!=NULL)
		SDL_FreeSurface(invup_bmp);
	if(invdown_bmp!=NULL)
		SDL_FreeSurface(invdown_bmp);
	if(inventory_bmp!=NULL)
		SDL_FreeSurface(inventory_bmp);
	if(inventoryoption_bmp!=NULL)
		SDL_FreeSurface(inventoryoption_bmp);
	if (inventory_mode_item_img)
		SDL_FreeSurface(inventory_mode_item_img);
	if (inventory_mode_item_highlighted_img)
		SDL_FreeSurface(inventory_mode_item_highlighted_img);
	if (inventory_mode_spell_img)
		SDL_FreeSurface(inventory_mode_spell_img);
	if (inventory_mode_spell_highlighted_img)
		SDL_FreeSurface(inventory_mode_spell_highlighted_img);
	if(button_bmp!=NULL)
		SDL_FreeSurface(button_bmp);
	if(smallbutton_bmp!=NULL)
		SDL_FreeSurface(smallbutton_bmp);
	if(equipped_bmp!=NULL)
		SDL_FreeSurface(equipped_bmp);
	if(inventoryChest_bmp != NULL)
		SDL_FreeSurface(inventoryChest_bmp);
	if(invclose_bmp != NULL)
		SDL_FreeSurface(invclose_bmp);
	if(invgraball_bmp != NULL)
		SDL_FreeSurface(invgraball_bmp);
	if(inventoryoptionChest_bmp != NULL)
		SDL_FreeSurface(inventoryoptionChest_bmp);
	if(shopkeeper_bmp != NULL)
		SDL_FreeSurface(shopkeeper_bmp);
	if(damage_bmp != NULL)
		SDL_FreeSurface(damage_bmp);
	//for( c=0; c<NUMCATEGORIES; c++ )
		//if(category_bmp[c]!=NULL)
			//SDL_FreeSurface(category_bmp[c]);
	if(identifyGUI_img != NULL)
		SDL_FreeSurface(identifyGUI_img);
	/*if (rightsidebar_titlebar_img)
		SDL_FreeSurface(rightsidebar_titlebar_img);
	if (rightsidebar_slot_img)
		SDL_FreeSurface(rightsidebar_slot_img);
	if (rightsidebar_slot_highlighted_img)
		SDL_FreeSurface(rightsidebar_slot_highlighted_img);*/
	if (rightsidebar_slot_grayedout_img)
		SDL_FreeSurface(rightsidebar_slot_grayedout_img);
	if (bookgui_img)
		SDL_FreeSurface(bookgui_img);
	//if (nextpage_img)
		//SDL_FreeSurface(nextpage_img);
	//if (previouspage_img)
		//SDL_FreeSurface(previouspage_img);
	//if (bookclose_img)
		//SDL_FreeSurface(bookclose_img);
	if (book_highlighted_left_img)
		SDL_FreeSurface(book_highlighted_left_img);
	if (book_highlighted_right_img)
		SDL_FreeSurface(book_highlighted_right_img);
	if (hotbar_img)
		SDL_FreeSurface(hotbar_img);
	if (hotbar_spell_img)
		SDL_FreeSurface(hotbar_spell_img);
	list_FreeAll(&damageIndicators);
}

void defaultConfig() {
	consoleCommand("/res 1280x720");
	consoleCommand("/gamma 1.000");
	consoleCommand("/smoothlighting");
	consoleCommand("/shaking");
	consoleCommand("/bobbing");
	consoleCommand("/sfxvolume 64");
	consoleCommand("/musvolume 32");
	consoleCommand("/mousespeed 16");
	consoleCommand("/svflags 30");
	consoleCommand("/bind 26 IN_FORWARD");
	consoleCommand("/bind 4 IN_LEFT");
	consoleCommand("/bind 22 IN_BACK");
	consoleCommand("/bind 7 IN_RIGHT");
	consoleCommand("/bind 20 IN_TURNL");
	consoleCommand("/bind 8 IN_TURNR");
	consoleCommand("/bind 6 IN_UP");
	consoleCommand("/bind 29 IN_DOWN");
	consoleCommand("/bind 40 IN_CHAT");
	consoleCommand("/bind 56 IN_COMMAND");
	consoleCommand("/bind 43 IN_STATUS");
	consoleCommand("/bind 16 IN_SPELL_LIST");
	consoleCommand("/bind 9 IN_CAST_SPELL");
	consoleCommand("/bind 44 IN_DEFEND");
	consoleCommand("/bind 283 IN_ATTACK");
	consoleCommand("/bind 285 IN_USE");
	return;
}

/*-------------------------------------------------------------------------------

	saveCommand
	
	saves a command to the command history

-------------------------------------------------------------------------------*/

void saveCommand(char *content) {
	newString(&command_history,0xFFFFFFFF,content);
}

/*-------------------------------------------------------------------------------

	loadConfig
	
	Reads the provided config file and executes the commands therein. Return
	value represents number of errors in config file

-------------------------------------------------------------------------------*/

int loadConfig(char *filename) {
	char str[1024];
	FILE *fp;
	bool mallocd = FALSE;

	printlog("Loading config '%s'...\n",filename);
	
	if( strstr(filename,".cfg") == NULL ) {
		char *filename2 = filename;
		filename = (char *) malloc(sizeof(char)*256);
		strcpy(filename,filename2);
		mallocd = TRUE;
		strcat(filename,".cfg");
	}
	
	// open the config file
	if( (fp = fopen(filename,"rb")) == NULL ) {
		printlog("warning: config file '%s' does not exist!\n", filename);
		defaultConfig(); //Set up the game with the default config.
		return 0;
	}
	
	// read commands from it
	while( fgets(str,1024,fp) != NULL ) {
		if( str[0] != '#' && str[0]!='\n' && str[0]!='\r' ) { // if this line is not white space or a comment
			// execute command
			consoleCommand(str);
		}
	}
	fclose(fp);
	if( mallocd )
		free(filename);
	return 0;
}

static char impulsenames[NUMIMPULSES][12] = {
	"FORWARD",
	"LEFT",
	"BACK",
	"RIGHT",
	"TURNL",
	"TURNR",
	"UP",
	"DOWN",
	"CHAT",
	"COMMAND",
	"STATUS",
	"SPELL_LIST",
	"CAST_SPELL",
	"DEFEND",
	"ATTACK",
	"USE"
};

/*-------------------------------------------------------------------------------

	saveConfig
	
	Opens the provided config file and saves the status of certain variables
	therein

-------------------------------------------------------------------------------*/

int saveConfig(char *filename) {
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	FILE *fp;
	int c;
	bool mallocd = FALSE;

	printlog("Saving config '%s'...\n",filename);
	
	if( strstr(filename,".cfg") == NULL ) {
		char *filename2 = filename;
		filename = (char *) malloc(sizeof(char)*256);
		strcpy(filename,filename2);
		mallocd = TRUE;
		strcat(filename,".cfg");
	}
	
	// open the config file
	if( (fp = fopen(filename,"wb")) == NULL ) {
		printlog("ERROR: failed to save config file '%s'!\n", filename);
		return 1;
	}
	
	// write config header
	fprintf(fp,"# %s\n",filename);
	fprintf(fp,"# this file was auto-generated on %d-%02d-%02d at %02d:%02d:%02d\n\n",tm.tm_year + 1900,tm.tm_mon + 1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);
	
	// write contents of config
	fprintf(fp,"/lang %s\n",languageCode);
	fprintf(fp,"/res %dx%d\n",xres,yres);
	fprintf(fp,"/gamma %3.3f\n",vidgamma);
	fprintf(fp,"/fov %d\n",fov);
	fprintf(fp,"/svflags %d\n",svFlags);
	if( smoothlighting )
		fprintf(fp,"/smoothlighting\n");
	if( fullscreen )
		fprintf(fp,"/fullscreen\n");
	if( shaking )
		fprintf(fp,"/shaking\n");
	if( bobbing )
		fprintf(fp,"/bobbing\n");
	fprintf(fp,"/sfxvolume %d\n",sfxvolume);
	fprintf(fp,"/musvolume %d\n",musvolume);
	for( c=0; c<NUMIMPULSES; c++ )
		fprintf(fp,"/bind %d IN_%s\n",impulses[c],impulsenames[c]);
	fprintf(fp,"/mousespeed %d\n",(int)(mousespeed));
	if( reversemouse )
		fprintf(fp,"/reversemouse\n");
	if( smoothmouse )
		fprintf(fp,"/smoothmouse\n");
	if (last_ip[0]) {
		fprintf(fp, "/ip %s\n", last_ip);
	}
	if (last_port[0]) {
		fprintf(fp, "/port %s\n", last_port);
	}
	if (!spawn_blood) {
		fprintf(fp, "/noblood\n");
	}
	if (colorblind) {
		fprintf(fp, "/colorblind\n");
	}
	if (right_click_protect) {
		fprintf(fp, "/right_click_protect\n");
	}
	if (!capture_mouse) {
		fprintf(fp, "/nocapturemouse\n");
	}
	if (broadcast) {
		fprintf(fp, "/broadcast\n");
	}
	if (nohud) {
		fprintf(fp, "/nohud\n");
	}
	if (!auto_hotbar_new_items)
	{
		fprintf(fp, "/disablehotbarnewitems\n");
	}
	if (disable_messages)
	{
		fprintf(fp, "/disablemessages\n");
	}
	fprintf(fp, "/skipintro\n");
	
	fclose(fp);
	if( mallocd )
		free(filename);
	return 0;
}

/*-------------------------------------------------------------------------------

	mouseInBounds
	
	Returns true if the mouse is within the rectangle specified, otherwise
	returns false

-------------------------------------------------------------------------------*/

bool mouseInBounds(int x1, int x2, int y1, int y2) {
	if (omousey >= y1 && omousey < y2)
		if (omousex >= x1 && omousex < x2)
			return TRUE;

	return FALSE;
}

hotbar_slot_t *getHotbar(int x, int y) {
	if (x >= STATUS_X && x < STATUS_X + status_bmp->w && y >= STATUS_Y - hotbar_img->h && y < STATUS_Y) {
		int relx = x - STATUS_X; //X relative to the start of the hotbar.
		return &hotbar[relx / hotbar_img->w]; //The slot will clearly be the x divided by the width of a slot
	}

	return NULL;
}

/*-------------------------------------------------------------------------------

	getInputName
	
	Returns the character string from the 

-------------------------------------------------------------------------------*/

const char *getInputName(Uint32 scancode) {
	if( scancode>=0 && scancode<283 ) {
		return SDL_GetKeyName(SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(scancode)));
	} else if( scancode<299 ) {
		switch( scancode ) {
			case 283:
				return "Mouse 0";
			case 284:
				return "Mouse 1";
			case 285:
				return "Mouse 2";
			case 286:
				return "Mouse 3";
			case 287:
				return "Wheel up";
			case 288:
				return "Wheel down";
			case 289:
				return "Mouse 6";
			case 290:
				return "Mouse 7";
			case 291:
				return "Mouse 8";
			case 292:
				return "Mouse 9";
			case 293:
				return "Mouse 10";
			case 294:
				return "Mouse 11";
			case 295:
				return "Mouse 12";
			case 296:
				return "Mouse 13";
			case 297:
				return "Mouse 14";
			case 298:
				return "Mouse 15";
			default:
				return "Unknown key";
		}
	} else if( scancode<315 ) {
		switch( scancode ) {
			case 299:
				return "Button 0";
			case 300:
				return "Button 1";
			case 301:
				return "Button 2";
			case 302:
				return "Button 3";
			case 303:
				return "Button 4";
			case 304:
				return "Button 5";
			case 305:
				return "Button 6";
			case 306:
				return "Button 7";
			case 307:
				return "Button 8";
			case 308:
				return "Button 9";
			case 309:
				return "Button 10";
			case 310:
				return "Button 11";
			case 311:
				return "Button 12";
			case 312:
				return "Button 13";
			case 313:
				return "Button 14";
			case 314:
				return "Button 15";
			default:
				return "Unknown key";
		}
	} else {
		return "Unknown key";
	}
}

/*-------------------------------------------------------------------------------

	inputPressed

	Returns non-zero number if the given key/mouse/joystick button is being
	pressed, returns 0 otherwise

-------------------------------------------------------------------------------*/

Sint8 *inputPressed(Uint32 scancode) {
	if( scancode>=0 && scancode<283 ) {
		// usual (keyboard) scancode range
		return &keystatus[scancode];
	} else if( scancode<299 ) {
		// mouse scancodes
		return &mousestatus[scancode-282];
	} else if( scancode<315 ) {
		// joysticks are currently unimplemented
		return NULL;
	} else {
		// bad scancode
		return NULL;
	}
}