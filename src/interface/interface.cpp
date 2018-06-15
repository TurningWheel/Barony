/*-------------------------------------------------------------------------------

	BARONY
	File: interface.cpp
	Desc: contains code for game interface

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../files.hpp"
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
#include "../player.hpp"

Uint32 svFlags = 30;
SDL_Surface* backdrop_minotaur_bmp = nullptr;
SDL_Surface* backdrop_blessed_bmp = nullptr;
SDL_Surface* backdrop_cursed_bmp = nullptr;
SDL_Surface* status_bmp = nullptr;
SDL_Surface* character_bmp = nullptr;
SDL_Surface* hunger_bmp = nullptr;
SDL_Surface* minotaur_bmp = nullptr;
int textscroll = 0;
int attributespage = 0;
int proficienciesPage = 0;
Item* invitems[4];
Item* invitemschest[4];
int inventorycategory = 7; // inventory window defaults to wildcard
int itemscroll = 0;
view_t camera_charsheet;
real_t camera_charsheet_offsetyaw = (330) * PI / 180;

SDL_Surface* font12x12_small_bmp = NULL;
SDL_Surface* inventoryChest_bmp = NULL;
SDL_Surface* invclose_bmp = NULL;
SDL_Surface* invgraball_bmp = NULL;
SDL_Surface* button_bmp = NULL, *smallbutton_bmp = NULL, *invup_bmp = NULL, *invdown_bmp = NULL;
bool gui_clickdrag = false;
int dragoffset_x = 0;
int dragoffset_y = 0;

int chestitemscroll = 0;
list_t chestInv;
int chestgui_offset_x = 0;
int chestgui_offset_y = 0;
bool dragging_chestGUI = false;
int selectedChestSlot = -1;

int selected_inventory_slot_x = 0;
int selected_inventory_slot_y = 0;

SDL_Surface* rightsidebar_titlebar_img = NULL;
SDL_Surface* rightsidebar_slot_img = NULL;
SDL_Surface* rightsidebar_slot_highlighted_img = NULL;
SDL_Surface* rightsidebar_slot_grayedout_img = NULL;
int rightsidebar_height = 0;
int appraisal_timer = 0;
int appraisal_timermax = 0;
Uint32 appraisal_item = 0;

SDL_Surface* bookgui_img = NULL;
//SDL_Surface *nextpage_img = NULL;
//SDL_Surface *previouspage_img = NULL;
//SDL_Surface *bookclose_img = NULL;
node_t* book_page = NULL;
int bookgui_offset_x = 0;
int bookgui_offset_y = 0;
bool dragging_book_GUI = false;
bool book_open = false;
book_t* open_book = NULL;
Item* open_book_item = NULL;
//int book_characterspace_x = 0;
//int book_characterspace_y = 0;

SDL_Surface* book_highlighted_left_img = NULL;
SDL_Surface* book_highlighted_right_img = NULL;

int gui_mode = GUI_MODE_NONE;

SDL_Surface* magicspellList_bmp = NULL;
SDL_Surface* spell_list_titlebar_bmp = NULL;
SDL_Surface* spell_list_gui_slot_bmp = NULL;
SDL_Surface* spell_list_gui_slot_highlighted_bmp = NULL;
SDL_Surface* textup_bmp = NULL;
SDL_Surface* textdown_bmp = NULL;
SDL_Surface* attributesleft_bmp = NULL;
SDL_Surface* attributesright_bmp = NULL;
SDL_Surface* attributesleftunclicked_bmp = NULL;
SDL_Surface* attributesrightunclicked_bmp = NULL;
SDL_Surface* inventory_bmp = NULL, *inventoryoption_bmp = NULL, *inventoryoptionChest_bmp = NULL, *equipped_bmp = NULL;
SDL_Surface* itembroken_bmp = nullptr;
//SDL_Surface *category_bmp[NUMCATEGORIES];
SDL_Surface* shopkeeper_bmp = NULL;
SDL_Surface* damage_bmp = NULL;
SDL_Surface *str_bmp64u = NULL;
SDL_Surface *dex_bmp64u = NULL;
SDL_Surface *con_bmp64u = NULL;
SDL_Surface *int_bmp64u = NULL;
SDL_Surface *per_bmp64u = NULL;
SDL_Surface *chr_bmp64u = NULL;
SDL_Surface *str_bmp64 = NULL;
SDL_Surface *dex_bmp64 = NULL;
SDL_Surface *con_bmp64 = NULL;
SDL_Surface *int_bmp64 = NULL;
SDL_Surface *per_bmp64 = NULL;
SDL_Surface *chr_bmp64 = NULL;
SDL_Surface *sidebar_lock_bmp = nullptr;
SDL_Surface *sidebar_unlock_bmp = nullptr;
int spellscroll = 0;
int magicspell_list_offset_x = 0;
int magicspell_list_offset_y = 0;
bool dragging_magicspell_list_GUI = false;
int magic_GUI_state = 0;
SDL_Rect magic_gui_pos;
SDL_Surface* sustained_spell_generic_icon = NULL;

int buttonclick = 0;

bool draw_cursor = true;

hotbar_slot_t hotbar[NUM_HOTBAR_SLOTS];
int current_hotbar = 0;
SDL_Surface* hotbar_img = NULL;
SDL_Surface* hotbar_spell_img = NULL;
bool hotbarHasFocus = false;

list_t damageIndicators;

bool auto_hotbar_new_items = true;
bool auto_hotbar_categories[NUM_HOTBAR_CATEGORIES] = {	true, true, true, true, 
														true, true, true, true,
														true, true, true, true };
int autosort_inventory_categories[NUM_AUTOSORT_CATEGORIES] = {	0, 0, 0, 0,
																0, 0, 0, 0,
																0, 0, 0, 0 };
bool hotbar_numkey_quick_add = false;
bool disable_messages = false;
bool right_click_protect = false;
bool auto_appraise_new_items = false;
bool lock_right_sidebar = false;
bool show_game_timer_always = false;

bool loadInterfaceResources()
{
	//General GUI images.
	font12x12_small_bmp = loadImage("images/system/font12x12_small.png");
	backdrop_minotaur_bmp = loadImage("images/system/backdrop.png");
	backdrop_blessed_bmp = loadImage("images/system/backdrop_blessed.png");
	backdrop_cursed_bmp = loadImage("images/system/backdrop_cursed.png");
	button_bmp = loadImage("images/system/ButtonHighlighted.png");
	smallbutton_bmp = loadImage("images/system/SmallButtonHighlighted.png");
	invup_bmp = loadImage("images/system/InventoryUpHighlighted.png");
	invdown_bmp = loadImage("images/system/InventoryDownHighlighted.png");
	status_bmp = loadImage("images/system/StatusBar.png");
	character_bmp = loadImage("images/system/CharacterSheet.png");
	hunger_bmp = loadImage("images/system/Hunger.png");
	minotaur_bmp = loadImage("images/system/minotaur.png"); // the file "images/system/minotaur.png" doesn't exist in current Data
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
	inventory_bmp = loadImage("images/system/Inventory.png");
	inventoryoption_bmp = loadImage("images/system/InventoryOption.png");
	inventory_mode_item_img = loadImage("images/system/inventory_mode_item.png");
	inventory_mode_item_highlighted_img = loadImage("images/system/inventory_mode_item_highlighted.png");
	inventory_mode_spell_img = loadImage("images/system/inventory_mode_spell.png");
	inventory_mode_spell_highlighted_img = loadImage("images/system/inventory_mode_spell_highlighted.png");
	equipped_bmp = loadImage("images/system/Equipped.png");
	itembroken_bmp = loadImage("images/system/Broken.png");
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
		return false;
	}
	rightsidebar_slot_img = loadImage("images/system/rightSidebarSlot.png");
	if (!rightsidebar_slot_img) {
		printlog( "Failed to load \"images/system/rightSidebarSlot.png\".");
		return false;
	}
	rightsidebar_slot_highlighted_img = loadImage("images/system/rightSidebarSlotHighlighted.png");
	if (!rightsidebar_slot_highlighted_img) {
		printlog( "Failed to load \"images/system/rightSidebarSlotHighlighted.png\".");
		return false;
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

	str_bmp64u = loadImage("images/system/str64u.png");
	dex_bmp64u = loadImage("images/system/dex64u.png");
	con_bmp64u = loadImage("images/system/con64u.png");
	int_bmp64u = loadImage("images/system/int64u.png");
	per_bmp64u = loadImage("images/system/per64u.png");
	chr_bmp64u = loadImage("images/system/chr64u.png");
	str_bmp64 = loadImage("images/system/str64.png");
	dex_bmp64 = loadImage("images/system/dex64.png");
	con_bmp64 = loadImage("images/system/con64.png");
	int_bmp64 = loadImage("images/system/int64.png");
	per_bmp64 = loadImage("images/system/per64.png");
	chr_bmp64 = loadImage("images/system/chr64.png");

	sidebar_lock_bmp = loadImage("images/system/locksidebar.png");
	sidebar_unlock_bmp = loadImage("images/system/unlocksidebar.png");
	hotbar_img = loadImage("images/system/hotbar_slot.png");
	hotbar_spell_img = loadImage("images/system/magic/hotbar_spell.png");
	int i = 0;
	for (i = 0; i < NUM_HOTBAR_SLOTS; ++i)
	{
		hotbar[i].item = 0;
	}

	damageIndicators.first = nullptr;
	damageIndicators.last = nullptr;

	return true;
}

void freeInterfaceResources()
{
	//int c;

	if (font12x12_small_bmp)
	{
		SDL_FreeSurface(font12x12_small_bmp);
	}
	if (backdrop_minotaur_bmp)
	{
		SDL_FreeSurface(backdrop_minotaur_bmp);
	}
	if ( backdrop_blessed_bmp )
	{
		SDL_FreeSurface(backdrop_blessed_bmp);
	}
	if ( backdrop_cursed_bmp )
	{
		SDL_FreeSurface(backdrop_cursed_bmp);
	}
	if (status_bmp)
	{
		SDL_FreeSurface(status_bmp);
	}
	if (character_bmp)
	{
		SDL_FreeSurface(character_bmp);
	}
	if (hunger_bmp)
	{
		SDL_FreeSurface(hunger_bmp);
	}
	if ( minotaur_bmp )
	{
		SDL_FreeSurface(minotaur_bmp);
	}
	//if(textup_bmp)
	//SDL_FreeSurface(textup_bmp);
	//if(textdown_bmp)
	//SDL_FreeSurface(textdown_bmp);
	if (attributesleft_bmp)
	{
		SDL_FreeSurface(attributesleft_bmp);
	}
	if (attributesright_bmp)
	{
		SDL_FreeSurface(attributesright_bmp);
	}
	if (attributesleftunclicked_bmp)
	{
		SDL_FreeSurface(attributesleftunclicked_bmp);
	}
	if (attributesrightunclicked_bmp)
	{
		SDL_FreeSurface(attributesrightunclicked_bmp);
	}
	if (magicspellList_bmp)
	{
		SDL_FreeSurface(magicspellList_bmp);
	}
	if (spell_list_titlebar_bmp)
	{
		SDL_FreeSurface(spell_list_titlebar_bmp);
	}
	if (spell_list_gui_slot_bmp)
	{
		SDL_FreeSurface(spell_list_gui_slot_bmp);
	}
	if (spell_list_gui_slot_highlighted_bmp)
	{
		SDL_FreeSurface(spell_list_gui_slot_highlighted_bmp);
	}
	if (sustained_spell_generic_icon)
	{
		SDL_FreeSurface(sustained_spell_generic_icon);
	}
	if (invup_bmp != NULL)
	{
		SDL_FreeSurface(invup_bmp);
	}
	if (invdown_bmp != NULL)
	{
		SDL_FreeSurface(invdown_bmp);
	}
	if (inventory_bmp != NULL)
	{
		SDL_FreeSurface(inventory_bmp);
	}
	if (inventoryoption_bmp != NULL)
	{
		SDL_FreeSurface(inventoryoption_bmp);
	}
	if (inventory_mode_item_img)
	{
		SDL_FreeSurface(inventory_mode_item_img);
	}
	if (inventory_mode_item_highlighted_img)
	{
		SDL_FreeSurface(inventory_mode_item_highlighted_img);
	}
	if (inventory_mode_spell_img)
	{
		SDL_FreeSurface(inventory_mode_spell_img);
	}
	if (inventory_mode_spell_highlighted_img)
	{
		SDL_FreeSurface(inventory_mode_spell_highlighted_img);
	}
	if (button_bmp != NULL)
	{
		SDL_FreeSurface(button_bmp);
	}
	if (smallbutton_bmp != NULL)
	{
		SDL_FreeSurface(smallbutton_bmp);
	}
	if (equipped_bmp != NULL)
	{
		SDL_FreeSurface(equipped_bmp);
	}
	if ( itembroken_bmp != nullptr )
	{
		SDL_FreeSurface(itembroken_bmp);
	}
	if (inventoryChest_bmp != NULL)
	{
		SDL_FreeSurface(inventoryChest_bmp);
	}
	if (invclose_bmp != NULL)
	{
		SDL_FreeSurface(invclose_bmp);
	}
	if (invgraball_bmp != NULL)
	{
		SDL_FreeSurface(invgraball_bmp);
	}
	if (inventoryoptionChest_bmp != NULL)
	{
		SDL_FreeSurface(inventoryoptionChest_bmp);
	}
	if (shopkeeper_bmp != NULL)
	{
		SDL_FreeSurface(shopkeeper_bmp);
	}
	if (damage_bmp != NULL)
	{
		SDL_FreeSurface(damage_bmp);
	}
	//for( c=0; c<NUMCATEGORIES; c++ )
	//if(category_bmp[c]!=NULL)
	//SDL_FreeSurface(category_bmp[c]);
	if (identifyGUI_img != NULL)
	{
		SDL_FreeSurface(identifyGUI_img);
	}
	/*if (rightsidebar_titlebar_img)
		SDL_FreeSurface(rightsidebar_titlebar_img);
	if (rightsidebar_slot_img)
		SDL_FreeSurface(rightsidebar_slot_img);
	if (rightsidebar_slot_highlighted_img)
		SDL_FreeSurface(rightsidebar_slot_highlighted_img);*/
	if (rightsidebar_slot_grayedout_img)
	{
		SDL_FreeSurface(rightsidebar_slot_grayedout_img);
	}
	if (bookgui_img)
	{
		SDL_FreeSurface(bookgui_img);
	}
	//if (nextpage_img)
	//SDL_FreeSurface(nextpage_img);
	//if (previouspage_img)
	//SDL_FreeSurface(previouspage_img);
	//if (bookclose_img)
	//SDL_FreeSurface(bookclose_img);
	if (book_highlighted_left_img)
	{
		SDL_FreeSurface(book_highlighted_left_img);
	}
	if (book_highlighted_right_img)
	{
		SDL_FreeSurface(book_highlighted_right_img);
	}
	if (hotbar_img)
	{
		SDL_FreeSurface(hotbar_img);
	}
	if (hotbar_spell_img)
	{
		SDL_FreeSurface(hotbar_spell_img);
	}
	if ( str_bmp64u )
	{
		SDL_FreeSurface(str_bmp64u);
	}
	if ( dex_bmp64u )
	{
		SDL_FreeSurface(dex_bmp64u);
	}
	if ( con_bmp64u )
	{
		SDL_FreeSurface(con_bmp64u);
	}
	if ( int_bmp64u )
	{
		SDL_FreeSurface(int_bmp64u);
	}
	if ( per_bmp64u )
	{
		SDL_FreeSurface(per_bmp64u);
	}
	if ( chr_bmp64u )
	{
		SDL_FreeSurface(chr_bmp64u);
	}
	if ( str_bmp64 )
	{
		SDL_FreeSurface(str_bmp64);
	}
	if ( dex_bmp64 )
	{
		SDL_FreeSurface(dex_bmp64);
	}
	if ( con_bmp64 )
	{
		SDL_FreeSurface(con_bmp64);
	}
	if ( int_bmp64 )
	{
		SDL_FreeSurface(int_bmp64);
	}
	if ( per_bmp64 )
	{
		SDL_FreeSurface(per_bmp64);
	}
	if ( chr_bmp64 )
	{
		SDL_FreeSurface(chr_bmp64);
	}
	if ( sidebar_lock_bmp )
	{
		SDL_FreeSurface(sidebar_lock_bmp);
	}
	if ( sidebar_unlock_bmp )
	{
		SDL_FreeSurface(sidebar_unlock_bmp);
	}
	list_FreeAll(&damageIndicators);
}

void defaultImpulses()
{
#ifdef PANDORA
	impulses[IN_FORWARD] = 82;
	impulses[IN_LEFT] = 80;
	impulses[IN_BACK] = 81;
	impulses[IN_RIGHT] = 79;
#else
	impulses[IN_FORWARD] = 26;
	impulses[IN_LEFT] = 4;
	impulses[IN_BACK] = 22;
	impulses[IN_RIGHT] = 7;
#endif
	impulses[IN_TURNL] = 20;
	impulses[IN_TURNR] = 8;
	impulses[IN_UP] = 6;
	impulses[IN_DOWN] = 29;
	impulses[IN_CHAT] = 40;
	impulses[IN_COMMAND] = 56;
	impulses[IN_STATUS] = 43;
#ifdef PANDORA
	impulses[IN_SPELL_LIST] = 75;
	impulses[IN_CAST_SPELL] = 77;
	impulses[IN_DEFEND] = 78;
#else
	impulses[IN_SPELL_LIST] = 16;
	impulses[IN_CAST_SPELL] = 9;
	impulses[IN_DEFEND] = 44;
#endif
	impulses[IN_ATTACK] = 283;
	impulses[IN_USE] = 285;
	impulses[IN_AUTOSORT] = 21;

	joyimpulses[INJOY_STATUS] = 307;
	joyimpulses[INJOY_SPELL_LIST] = SCANCODE_UNASSIGNED_BINDING;
	joyimpulses[INJOY_GAME_CAST_SPELL] = 311;
	joyimpulses[INJOY_GAME_DEFEND] = 299;
	joyimpulses[INJOY_GAME_ATTACK] = 300;
	joyimpulses[INJOY_GAME_USE] = 301;
	joyimpulses[INJOY_PAUSE_MENU] = 305;
	joyimpulses[INJOY_MENU_LEFT_CLICK] = 303;
	joyimpulses[INJOY_DPAD_LEFT] = 314;
	joyimpulses[INJOY_DPAD_RIGHT] = 315;
	joyimpulses[INJOY_DPAD_UP] = 312;
	joyimpulses[INJOY_DPAD_DOWN] = 313;
	joyimpulses[INJOY_MENU_NEXT] = 301;
	joyimpulses[INJOY_GAME_HOTBAR_NEXT] = 315;
	joyimpulses[INJOY_GAME_HOTBAR_PREV] = 314;
	joyimpulses[INJOY_GAME_HOTBAR_ACTIVATE] = 310;
	joyimpulses[INJOY_MENU_CHEST_GRAB_ALL] = 304;
	joyimpulses[INJOY_MENU_CANCEL] = 302;
	joyimpulses[INJOY_MENU_USE] = 301;
	joyimpulses[INJOY_MENU_HOTBAR_CLEAR] = 304;
	joyimpulses[INJOY_MENU_REFRESH_LOBBY] = 304;
	joyimpulses[INJOY_MENU_DONT_LOAD_SAVE] = 304;
	joyimpulses[INJOY_MENU_RANDOM_CHAR] = 304;
	joyimpulses[INJOY_MENU_DROP_ITEM] = 302;
	joyimpulses[INJOY_MENU_CYCLE_SHOP_LEFT] = 310;
	joyimpulses[INJOY_MENU_CYCLE_SHOP_RIGHT] = 311;
	joyimpulses[INJOY_MENU_BOOK_NEXT] = 311;
	joyimpulses[INJOY_MENU_BOOK_PREV] = 310;
	joyimpulses[INJOY_MENU_SETTINGS_NEXT] = 311;
	joyimpulses[INJOY_MENU_SETTINGS_PREV] = 310;
	joyimpulses[INJOY_MENU_INVENTORY_TAB] = 299;
	joyimpulses[INJOY_MENU_MAGIC_TAB] = 300;
	joyimpulses[INJOY_MENU_RANDOM_NAME] = 304;
}

void defaultConfig()
{
#ifdef PANDORA
	consoleCommand("/res 960x600");
	consoleCommand("/gamma 2.000");
	consoleCommand("/smoothlighting");
	consoleCommand("/fullscreen");
#else
	consoleCommand("/res 1280x720");
	consoleCommand("/gamma 1.000");
	consoleCommand("/smoothlighting");
#endif
	consoleCommand("/shaking");
	consoleCommand("/bobbing");
	consoleCommand("/sfxvolume 64");
	consoleCommand("/musvolume 32");
#ifdef PANDORA
	consoleCommand("/mousespeed 105");
	consoleCommand("/svflags 30");
	consoleCommand("/bind 82 IN_FORWARD");
	consoleCommand("/bind 80 IN_LEFT");
	consoleCommand("/bind 81 IN_BACK");
	consoleCommand("/bind 79 IN_RIGHT");
#else
	consoleCommand("/mousespeed 16");
	consoleCommand("/svflags 30");
	consoleCommand("/bind 26 IN_FORWARD");
	consoleCommand("/bind 4 IN_LEFT");
	consoleCommand("/bind 22 IN_BACK");
	consoleCommand("/bind 7 IN_RIGHT");
#endif
	consoleCommand("/bind 20 IN_TURNL");
	consoleCommand("/bind 8 IN_TURNR");
	consoleCommand("/bind 6 IN_UP");
	consoleCommand("/bind 29 IN_DOWN");
	consoleCommand("/bind 40 IN_CHAT");
	consoleCommand("/bind 56 IN_COMMAND");
	consoleCommand("/bind 43 IN_STATUS");
#ifdef PANDORA
	consoleCommand("/bind 75 IN_SPELL_LIST");
	consoleCommand("/bind 77 IN_CAST_SPELL");
	consoleCommand("/bind 78 IN_DEFEND");
#else
	consoleCommand("/bind 16 IN_SPELL_LIST");
	consoleCommand("/bind 9 IN_CAST_SPELL");
	consoleCommand("/bind 44 IN_DEFEND");
#endif
	consoleCommand("/bind 283 IN_ATTACK");
	consoleCommand("/bind 285 IN_USE");
	consoleCommand("/bind 21 IN_AUTOSORT");
	consoleCommand("/joybind 307 INJOY_STATUS");
	consoleCommand("/joybind 399 INJOY_SPELL_LIST"); //SCANCODE_UNASSIGNED_BINDING
	consoleCommand("/joybind 311 INJOY_GAME_CAST_SPELL");
	consoleCommand("/joybind 299 INJOY_GAME_DEFEND");
	consoleCommand("/joybind 300 INJOY_GAME_ATTACK");
	consoleCommand("/joybind 301 INJOY_GAME_USE");
	consoleCommand("/joybind 301 INJOY_MENU_USE");
	consoleCommand("/joybind 305 INJOY_PAUSE_MENU");
	consoleCommand("/joybind 303 INJOY_MENU_LEFT_CLICK");
	consoleCommand("/joybind 314 INJOY_DPAD_LEFT");
	consoleCommand("/joybind 315 INJOY_DPAD_RIGHT");
	consoleCommand("/joybind 312 INJOY_DPAD_UP");
	consoleCommand("/joybind 313 INJOY_DPAD_DOWN");
	consoleCommand("/joybind 301 INJOY_MENU_NEXT");
	consoleCommand("/joybind 315 INJOY_GAME_HOTBAR_NEXT");
	consoleCommand("/joybind 314 INJOY_GAME_HOTBAR_PREV");
	consoleCommand("/joybind 310 INJOY_GAME_HOTBAR_ACTIVATE");
	consoleCommand("/joybind 304 INJOY_MENU_CHEST_GRAB_ALL");
	consoleCommand("/joybind 304 INJOY_MENU_HOTBAR_CLEAR");
	consoleCommand("/joybind 304 INJOY_MENU_REFRESH_LOBBY");
	consoleCommand("/joybind 304 INJOY_MENU_DONT_LOAD_SAVE");
	consoleCommand("/joybind 304 INJOY_MENU_RANDOM_CHAR");
	consoleCommand("/joybind 301 INJOY_MENU_NEXT");
	consoleCommand("/joybind 302 INJOY_MENU_CANCEL");
	consoleCommand("/joybind 302 INJOY_MENU_DROP_ITEM");
	consoleCommand("/joybind 310 INJOY_MENU_CYCLE_SHOP_LEFT");
	consoleCommand("/joybind 311 INJOY_MENU_CYCLE_SHOP_RIGHT");
	consoleCommand("/joybind 311 INJOY_MENU_BOOK_NEXT");
	consoleCommand("/joybind 310 INJOY_MENU_BOOK_PREV");
	consoleCommand("/joybind 311 INJOY_MENU_SETTINGS_NEXT");
	consoleCommand("/joybind 310 INJOY_MENU_SETTINGS_PREV");
	consoleCommand("/joybind 299 INJOY_MENU_INVENTORY_TAB");
	consoleCommand("/joybind 300 INJOY_MENU_MAGIC_TAB");
	consoleCommand("/joybind 304 INJOY_MENU_RANDOM_NAME");
	consoleCommand("/gamepad_deadzone 8000");
	consoleCommand("/gamepad_trigger_deadzone 18000");
	consoleCommand("/gamepad_leftx_sensitivity 1400");
	consoleCommand("/gamepad_lefty_sensitivity 1400");
	consoleCommand("/gamepad_rightx_sensitivity 500");
	consoleCommand("/gamepad_righty_sensitivity 600");
	consoleCommand("/gamepad_menux_sensitivity 1400");
	consoleCommand("/gamepad_menuy_sensitivity 1400");
	consoleCommand("/autoappraisenewitems");
	return;
}

static char impulsenames[NUMIMPULSES][12] =
{
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
	"USE",
	"AUTOSORT"
};

static char joyimpulsenames[NUM_JOY_IMPULSES][30] =
{
	//Bi-functional:
	"STATUS",
	"SPELL_LIST",
	"PAUSE_MENU",
	"DPAD_LEFT",
	"DPAD_RIGHT",
	"DPAD_UP",
	"DPAD_DOWN",

	//Menu exclusive:
	"MENU_LEFT_CLICK",
	"MENU_NEXT",
	"MENU_CANCEL",
	"MENU_SETTINGS_NEXT",
	"MENU_SETTINGS_PREV",
	"MENU_REFRESH_LOBBY",
	"MENU_DONT_LOAD_SAVE",
	"MENU_RANDOM_NAME",
	"MENU_RANDOM_CHAR",
	"MENU_INVENTORY_TAB",
	"MENU_MAGIC_TAB",
	"MENU_USE",
	"MENU_HOTBAR_CLEAR",
	"MENU_DROP_ITEM",
	"MENU_CHEST_GRAB_ALL",
	"MENU_CYCLE_SHOP_LEFT",
	"MENU_CYCLE_SHOP_RIGHT",
	"MENU_BOOK_NEXT",
	"MENU_BOOK_PREV",

	//Game Exclusive:
	"GAME_USE",
	"GAME_DEFEND",
	"GAME_ATTACK",
	"GAME_CAST_SPELL",
	"GAME_HOTBAR_ACTIVATE",
	"GAME_HOTBAR_PREV",
	"GAME_HOTBAR_NEXT"
};

/*-------------------------------------------------------------------------------

	saveCommand

	saves a command to the command history

-------------------------------------------------------------------------------*/

void saveCommand(char* content)
{
	newString(&command_history, 0xFFFFFFFF, content);
}

/*-------------------------------------------------------------------------------

	loadConfig

	Reads the provided config file and executes the commands therein. Return
	value represents number of errors in config file

-------------------------------------------------------------------------------*/

int loadConfig(char* filename)
{
	defaultImpulses(); //So that a config file that's missing impulses can get all them.

	char str[1024];
	FILE* fp;
	bool mallocd = false;

	printlog("Loading config '%s'...\n", filename);

	if ( strstr(filename, ".cfg") == NULL )
	{
		char* filename2 = filename;
		filename = (char*) malloc(sizeof(char) * 256);
		strcpy(filename, filename2);
		mallocd = true;
		strcat(filename, ".cfg");
	}

	// open the config file
	if ( (fp = fopen(filename, "rb")) == NULL )
	{
		printlog("warning: config file '%s' does not exist!\n", filename);
		defaultConfig(); //Set up the game with the default config.
		return 0;
	}

	// read commands from it
	while ( fgets(str, 1024, fp) != NULL )
	{
		if ( str[0] != '#' && str[0] != '\n' && str[0] != '\r' )   // if this line is not white space or a comment
		{
			// execute command
			consoleCommand(str);
		}
	}
	fclose(fp);
	if ( mallocd )
	{
		free(filename);
	}
	return 0;
}

/*-------------------------------------------------------------------------------

	saveConfig

	Opens the provided config file and saves the status of certain variables
	therein

-------------------------------------------------------------------------------*/

int saveConfig(char* filename)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	FILE* fp;
	int c;
	bool mallocd = false;

	printlog("Saving config '%s'...\n", filename);

	if ( strstr(filename, ".cfg") == NULL )
	{
		char* filename2 = filename;
		filename = (char*) malloc(sizeof(char) * 256);
		strcpy(filename, filename2);
		mallocd = true;
		strcat(filename, ".cfg");
	}

	// open the config file
	if ( (fp = fopen(filename, "wb")) == NULL )
	{
		printlog("ERROR: failed to save config file '%s'!\n", filename);
		return 1;
	}

	// write config header
	fprintf(fp, "# %s\n", filename);
	fprintf(fp, "# this file was auto-generated on %d-%02d-%02d at %02d:%02d:%02d\n\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	// write contents of config
	fprintf(fp, "/lang %s\n", languageCode);
	fprintf(fp, "/res %dx%d\n", xres, yres);
	fprintf(fp, "/gamma %3.3f\n", vidgamma);
	fprintf(fp, "/fov %d\n", fov);
	fprintf(fp, "/fps %d\n", fpsLimit);
	fprintf(fp, "/svflags %d\n", svFlags);
	if ( lastname != "" )
	{
		fprintf(fp, "/lastname %s\n", lastname.c_str());
	}
	if ( smoothlighting )
	{
		fprintf(fp, "/smoothlighting\n");
	}
	if ( fullscreen )
	{
		fprintf(fp, "/fullscreen\n");
	}
	if ( shaking )
	{
		fprintf(fp, "/shaking\n");
	}
	if ( bobbing )
	{
		fprintf(fp, "/bobbing\n");
	}
	fprintf(fp, "/sfxvolume %d\n", sfxvolume);
	fprintf(fp, "/musvolume %d\n", musvolume);
	for (c = 0; c < NUMIMPULSES; c++)
	{
		fprintf(fp, "/bind %d IN_%s\n", impulses[c], impulsenames[c]);
	}
	for (c = 0; c < NUM_JOY_IMPULSES; c++)
	{
		fprintf(fp, "/joybind %d INJOY_%s\n", joyimpulses[c], joyimpulsenames[c]);
	}
	fprintf(fp, "/mousespeed %d\n", (int)(mousespeed));
	if ( reversemouse )
	{
		fprintf(fp, "/reversemouse\n");
	}
	if ( smoothmouse )
	{
		fprintf(fp, "/smoothmouse\n");
	}
	if (last_ip[0])
	{
		fprintf(fp, "/ip %s\n", last_ip);
	}
	if (last_port[0])
	{
		fprintf(fp, "/port %s\n", last_port);
	}
	if (!spawn_blood)
	{
		fprintf(fp, "/noblood\n");
	}
	if ( !flickerLights )
	{
		fprintf(fp, "/nolightflicker\n");
	}
	if ( verticalSync )
	{
		fprintf(fp, "/vsync\n");
	}
	if ( minimapPingMute )
	{
		fprintf(fp, "/muteping\n");
	}
	if (colorblind)
	{
		fprintf(fp, "/colorblind\n");
	}
	if (!capture_mouse)
	{
		fprintf(fp, "/nocapturemouse\n");
	}
	if (broadcast)
	{
		fprintf(fp, "/broadcast\n");
	}
	if (nohud)
	{
		fprintf(fp, "/nohud\n");
	}
	if (!auto_hotbar_new_items)
	{
		fprintf(fp, "/disablehotbarnewitems\n");
	}
	for ( c = 0; c < NUM_HOTBAR_CATEGORIES; ++c )
	{
		fprintf(fp, "/hotbarenablecategory %d %d\n", c, auto_hotbar_categories[c]);
	}
	for ( c = 0; c < NUM_AUTOSORT_CATEGORIES; ++c )
	{
		fprintf(fp, "/autosortcategory %d %d\n", c, autosort_inventory_categories[c]);
	}
	if ( hotbar_numkey_quick_add )
	{
		fprintf(fp, "/quickaddtohotbar\n");
	}
	if ( lock_right_sidebar )
	{
		fprintf(fp, "/locksidebar\n");
	}
	if ( show_game_timer_always )
	{
		fprintf(fp, "/showgametimer\n");
	}
	if (disable_messages)
	{
		fprintf(fp, "/disablemessages\n");
	}
	if (right_click_protect)
	{
		fprintf(fp, "/right_click_protect\n");
	}
	if (auto_appraise_new_items)
	{
		fprintf(fp, "/autoappraisenewitems\n");
	}
	if (startfloor)
	{
		fprintf(fp, "/startfloor %d\n", startfloor);
	}
	if (splitscreen)
	{
		fprintf(fp, "/splitscreen\n");
	}
	if ( useModelCache )
	{
		fprintf(fp, "/usemodelcache\n");
	}
	fprintf(fp, "/lastcharacter %d %d %d\n", lastCreatedCharacterSex, lastCreatedCharacterClass, lastCreatedCharacterAppearance);
	fprintf(fp, "/gamepad_deadzone %d\n", gamepad_deadzone);
	fprintf(fp, "/gamepad_trigger_deadzone %d\n", gamepad_trigger_deadzone);
	fprintf(fp, "/gamepad_leftx_sensitivity %d\n", gamepad_leftx_sensitivity);
	fprintf(fp, "/gamepad_lefty_sensitivity %d\n", gamepad_lefty_sensitivity);
	fprintf(fp, "/gamepad_rightx_sensitivity %d\n", gamepad_rightx_sensitivity);
	fprintf(fp, "/gamepad_righty_sensitivity %d\n", gamepad_righty_sensitivity);
	fprintf(fp, "/gamepad_menux_sensitivity %d\n", gamepad_menux_sensitivity);
	fprintf(fp, "/gamepad_menuy_sensitivity %d\n", gamepad_menuy_sensitivity);
	if (gamepad_rightx_invert)
	{
		fprintf(fp, "/gamepad_rightx_invert\n");
	}
	if (gamepad_righty_invert)
	{
		fprintf(fp, "/gamepad_righty_invert\n");
	}
	if (gamepad_leftx_invert)
	{
		fprintf(fp, "/gamepad_leftx_invert\n");
	}
	if (gamepad_lefty_invert)
	{
		fprintf(fp, "/gamepad_lefty_invert\n");
	}
	if (gamepad_menux_invert)
	{
		fprintf(fp, "/gamepad_menux_invert\n");
	}
	if (gamepad_menuy_invert)
	{
		fprintf(fp, "/gamepad_menuy_invert\n");
	}
	fprintf(fp, "/skipintro\n");

	fclose(fp);
	if ( mallocd )
	{
		free(filename);
	}
	return 0;
}

/*-------------------------------------------------------------------------------

	mouseInBounds

	Returns true if the mouse is within the rectangle specified, otherwise
	returns false

-------------------------------------------------------------------------------*/

bool mouseInBounds(int x1, int x2, int y1, int y2)
{
	if (omousey >= y1 && omousey < y2)
		if (omousex >= x1 && omousex < x2)
		{
			return true;
		}

	return false;
}

hotbar_slot_t* getHotbar(int x, int y)
{
	if (x >= STATUS_X && x < STATUS_X + status_bmp->w && y >= STATUS_Y - hotbar_img->h && y < STATUS_Y)
	{
		int relx = x - STATUS_X; //X relative to the start of the hotbar.
		return &hotbar[relx / hotbar_img->w]; //The slot will clearly be the x divided by the width of a slot
	}

	return NULL;
}

/*-------------------------------------------------------------------------------

	getInputName

	Returns the character string from the

-------------------------------------------------------------------------------*/

const char* getInputName(Uint32 scancode)
{
	if ( scancode >= 0 && scancode < 283 )
	{
		return SDL_GetKeyName(SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(scancode)));
	}
	else if ( scancode < 299 )
	{
		switch ( scancode )
		{
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
	}
	else if ( scancode < 301 )     //Game Controller triggers.
	{
		switch ( scancode )
		{
			case 299:
				return "Left Trigger";
			case 300:
				return "Right Trigger";
			default:
				return "Unknown trigger";
		}
	}
	else if ( scancode < 317 )     //Game controller buttons.
	{
		return SDL_GameControllerGetStringForButton(static_cast<SDL_GameControllerButton>(scancode - 301));
	}
	else
	{
		if ( scancode == SCANCODE_UNASSIGNED_BINDING )
		{
			return "Unassigned key";
		}
		return "Unknown key";
	}
}

/*-------------------------------------------------------------------------------

	inputPressed

	Returns non-zero number if the given key/mouse/joystick button is being
	pressed, returns 0 otherwise

-------------------------------------------------------------------------------*/

Sint8 dummy_value = 0; //THIS LINE IS AN UTTER BODGE to stop this function from crashing.

Sint8* inputPressed(Uint32 scancode)
{
	if (scancode >= 0 && scancode < 283)
	{
		// usual (keyboard) scancode range
		return &keystatus[scancode];
	}
	else if (scancode < 299)
	{
		// mouse scancodes
		return &mousestatus[scancode - 282];
	}
	else if (scancode < 301)
	{
		//Analog joystick triggers are mapped to digital status (0 = not pressed, 1 = pressed).
		return &joy_trigger_status[scancode - 299];
	}
	else if (scancode < 318)
	{
		return &joystatus[scancode - 301];
	}
	else
	{
		// bad scancode
		//return nullptr; //This crashes.
		dummy_value = 0;
		return &dummy_value;
		//Not an ideal solution, but...
	}
}

void selectHotbarSlot(int slot)
{
	if (slot < 0)
	{
		slot = NUM_HOTBAR_SLOTS - 1;
	}
	if (slot >= NUM_HOTBAR_SLOTS)
	{
		slot = 0;
	}

	current_hotbar = slot;

	hotbarHasFocus = true;
}

void openStatusScreen(int whichGUIMode, int whichInventoryMode)
{
	shootmode = false;
	gui_mode = whichGUIMode;
	selectedItem = nullptr;
	inventory_mode = whichInventoryMode;
	SDL_SetRelativeMouseMode(SDL_FALSE);
	SDL_WarpMouseInWindow(screen, xres / 2, yres / 2);
	mousex = xres / 2;
	mousey = yres / 2;
	attributespage = 0;
	//proficienciesPage = 0;
}
