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
#include "../colors.hpp"
#include "../net.hpp"
#include "../draw.hpp"

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
bool hide_statusbar = false;
bool hide_playertags = false;
bool show_skill_values = false;
real_t uiscale_chatlog = 1.f;
real_t uiscale_playerbars = 1.f;
real_t uiscale_hotbar = 1.f;
real_t uiscale_inventory = 1.f;
bool uiscale_charactersheet = false;
bool uiscale_skillspage = false;

FollowerRadialMenu FollowerMenu;
SDL_Rect interfaceSkillsSheet;
SDL_Rect interfacePartySheet;

std::vector<std::pair<SDL_Surface**, std::string>> systemResourceImages =
{
	std::make_pair(&title_bmp, "images/system/title.png"),
	std::make_pair(&logo_bmp, "images/system/logo.png"),
	std::make_pair(&cursor_bmp, "images/system/cursor.png"),
	std::make_pair(&cross_bmp, "images/system/cross.png"),

	std::make_pair(&fancyWindow_bmp, "images/system/fancyWindow.png"),
	std::make_pair(&font8x8_bmp, "images/system/font8x8.png"),
	std::make_pair(&font12x12_bmp, "images/system/font12x12.png"),
	std::make_pair(&font16x16_bmp, "images/system/font16x16.png"),

	std::make_pair(&font12x12_small_bmp, "images/system/font12x12_small.png"),
	std::make_pair(&backdrop_minotaur_bmp, "images/system/backdrop.png"),
	std::make_pair(&backdrop_blessed_bmp, "images/system/backdrop_blessed.png"),
	std::make_pair(&backdrop_cursed_bmp, "images/system/backdrop_cursed.png"),
	std::make_pair(&button_bmp, "images/system/ButtonHighlighted.png"),
	std::make_pair(&smallbutton_bmp, "images/system/SmallButtonHighlighted.png"),
	std::make_pair(&invup_bmp, "images/system/InventoryUpHighlighted.png"),
	std::make_pair(&invdown_bmp, "images/system/InventoryDownHighlighted.png"),
	std::make_pair(&status_bmp, "images/system/StatusBar.png"),
	std::make_pair(&character_bmp, "images/system/CharacterSheet.png"),
	std::make_pair(&hunger_bmp, "images/system/Hunger.png"),
	std::make_pair(&minotaur_bmp, "images/system/minotaur.png"),
	std::make_pair(&attributesleft_bmp, "images/system/AttributesLeftHighlighted.png"),
	std::make_pair(&attributesright_bmp, "images/system/AttributesRightHighlighted.png"),

	//General GUI images.
	std::make_pair(&attributesleftunclicked_bmp, "images/system/AttributesLeft.png"),
	std::make_pair(&attributesrightunclicked_bmp, "images/system/AttributesRight.png"),
	std::make_pair(&shopkeeper_bmp, "images/system/shopkeeper.png"),
	std::make_pair(&damage_bmp, "images/system/damage.png"),

	//Magic GUI images.
	std::make_pair(&magicspellList_bmp, "images/system/spellList.png"),
	std::make_pair(&spell_list_titlebar_bmp, "images/system/spellListTitlebar.png"),
	std::make_pair(&spell_list_gui_slot_bmp, "images/system/spellListSlot.png"),
	std::make_pair(&spell_list_gui_slot_highlighted_bmp, "images/system/spellListSlotHighlighted.png"),
	std::make_pair(&sustained_spell_generic_icon, "images/system/magic/channeled_spell.png"),

	// inventory GUI images.
	std::make_pair(&inventory_bmp, "images/system/Inventory.png"),
	std::make_pair(&inventoryoption_bmp, "images/system/InventoryOption.png"),
	std::make_pair(&inventory_mode_item_img, "images/system/inventory_mode_item.png"),
	std::make_pair(&inventory_mode_item_highlighted_img, "images/system/inventory_mode_item_highlighted.png"),
	std::make_pair(&inventory_mode_spell_img, "images/system/inventory_mode_spell.png"),
	std::make_pair(&inventory_mode_spell_highlighted_img, "images/system/inventory_mode_spell_highlighted.png"),
	std::make_pair(&equipped_bmp, "images/system/Equipped.png"),
	std::make_pair(&itembroken_bmp, "images/system/Broken.png"),

	//Chest images..
	std::make_pair(&inventoryChest_bmp, "images/system/InventoryChest.png"),
	std::make_pair(&inventoryoptionChest_bmp, "images/system/InventoryOptionChest.png"),
	std::make_pair(&invclose_bmp, "images/system/InventoryCloseHighlighted.png"),
	std::make_pair(&invgraball_bmp, "images/system/InventoryChestGraballHighlighted.png"),

	//Identify GUI images...
	std::make_pair(&identifyGUI_img, "images/system/identifyGUI.png"),
	std::make_pair(&rightsidebar_slot_grayedout_img, "images/system/rightSidebarSlotGrayedOut.png"),
	std::make_pair(&bookgui_img, "images/system/book.png"),
	std::make_pair(&book_highlighted_left_img, "images/system/bookpageleft-highlighted.png"),
	std::make_pair(&book_highlighted_right_img, "images/system/bookpageright-highlighted.png"),

	//Levelup images.
	std::make_pair(&str_bmp64u, "images/system/str64u.png"),
	std::make_pair(&dex_bmp64u, "images/system/dex64u.png"),
	std::make_pair(&con_bmp64u, "images/system/con64u.png"),
	std::make_pair(&int_bmp64u, "images/system/int64u.png"),
	std::make_pair(&per_bmp64u, "images/system/per64u.png"),
	std::make_pair(&chr_bmp64u, "images/system/chr64u.png"),
	std::make_pair(&str_bmp64, "images/system/str64.png"),
	std::make_pair(&dex_bmp64, "images/system/dex64.png"),
	std::make_pair(&con_bmp64, "images/system/con64.png"),
	std::make_pair(&int_bmp64, "images/system/int64.png"),
	std::make_pair(&per_bmp64, "images/system/per64.png"),
	std::make_pair(&chr_bmp64, "images/system/chr64.png"),

	//Misc GUI images.
	std::make_pair(&sidebar_lock_bmp, "images/system/locksidebar.png"),
	std::make_pair(&sidebar_unlock_bmp, "images/system/unlocksidebar.png"),
	std::make_pair(&hotbar_img, "images/system/hotbar_slot.png"),
	std::make_pair(&hotbar_spell_img, "images/system/magic/hotbar_spell.png")
};

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
	impulses[IN_TURNL] = 80;
	impulses[IN_TURNR] = 79;
	impulses[IN_UP] = 82;
	impulses[IN_DOWN] = 81;
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
	impulses[IN_MINIMAPSCALE] = 27;
	impulses[IN_TOGGLECHATLOG] = 15;
	impulses[IN_FOLLOWERMENU] = 6;
	impulses[IN_FOLLOWERMENU_LASTCMD] = 20;
	impulses[IN_FOLLOWERMENU_CYCLENEXT] = 8;

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
	joyimpulses[INJOY_GAME_TOGGLECHATLOG] = SCANCODE_UNASSIGNED_BINDING;
	joyimpulses[INJOY_GAME_MINIMAPSCALE] = SCANCODE_UNASSIGNED_BINDING;
	joyimpulses[INJOY_GAME_FOLLOWERMENU] = SCANCODE_UNASSIGNED_BINDING;
	joyimpulses[INJOY_GAME_FOLLOWERMENU_LASTCMD] = SCANCODE_UNASSIGNED_BINDING;
	joyimpulses[INJOY_GAME_FOLLOWERMENU_CYCLE] = SCANCODE_UNASSIGNED_BINDING;
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
	consoleCommand("/bind 80 IN_TURNL");
	consoleCommand("/bind 79 IN_TURNR");
	consoleCommand("/bind 82 IN_UP");
	consoleCommand("/bind 81 IN_DOWN");
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
	consoleCommand("/bind 27 IN_MINIMAPSCALE");
	consoleCommand("/bind 15 IN_TOGGLECHATLOG");
	consoleCommand("/bind 6 IN_FOLLOWERMENU");
	consoleCommand("/bind 20 IN_FOLLOWERMENU_LASTCMD");
	consoleCommand("/bind 8 IN_FOLLOWERMENU_CYCLENEXT");

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
	consoleCommand("/joybind 399 INJOY_GAME_MINIMAPSCALE"); //SCANCODE_UNASSIGNED_BINDING
	consoleCommand("/joybind 399 INJOY_GAME_TOGGLECHATLOG"); //SCANCODE_UNASSIGNED_BINDING
	consoleCommand("/joybind 399 INJOY_GAME_FOLLOWERMENU"); //SCANCODE_UNASSIGNED_BINDING
	consoleCommand("/joybind 399 INJOY_GAME_FOLLOWERMENU_LASTCMD"); //SCANCODE_UNASSIGNED_BINDING
	consoleCommand("/joybind 399 INJOY_GAME_FOLLOWERMENU_CYCLE"); //SCANCODE_UNASSIGNED_BINDING
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

static char impulsenames[NUMIMPULSES][23] =
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
	"AUTOSORT",
	"MINIMAPSCALE",
	"TOGGLECHATLOG",
	"FOLLOWERMENU_OPEN",
	"FOLLOWERMENU_LASTCMD",
	"FOLLOWERMENU_CYCLENEXT"
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
	"GAME_HOTBAR_NEXT",
	"GAME_MINIMAPSCALE",
	"GAME_TOGGLECHATLOG",
	"GAME_FOLLOWERMENU_OPEN",
	"GAME_FOLLOWERMENU_LASTCMD",
	"GAME_FOLLOWERMENU_CYCLENEXT"
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

	if ( impulses[IN_FOLLOWERMENU_CYCLENEXT] == impulses[IN_TURNR]
		&& impulses[IN_TURNR] == 8 )
	{
		// reset to default arrow key to avoid overlapping keybinds on first launch.
		// due to legacy keybind, now we have useful things to assign to q,e,z,c
		impulses[IN_TURNR] = 79;
		printlog("Legacy keys detected, conflict with IN_FOLLOWERMENU_CYCLENEXT. Automatically rebound IN_TURNR: %d (Right arrow key)\n", impulses[IN_TURNR]);
	}
	if ( impulses[IN_FOLLOWERMENU] == impulses[IN_UP]
		&& impulses[IN_UP] == 6 )
	{
		// reset to default arrow key to avoid overlapping keybinds on first launch.
		// due to legacy keybind, now we have useful things to assign to q,e,z,c
		impulses[IN_UP] = 82;
		printlog("Legacy keys detected, conflict with IN_FOLLOWERMENU_CYCLENEXT. Automatically rebound IN_TURNR: %d (Right arrow key)\n", impulses[IN_TURNR]);
	}
	if ( impulses[IN_FOLLOWERMENU_LASTCMD] == impulses[IN_TURNL]
		&& impulses[IN_TURNL] == 20 )
	{
		// reset to default arrow key to avoid overlapping keybinds on first launch.
		// due to legacy keybind, now we have useful things to assign to q,e,z,c
		impulses[IN_TURNL] = 80;
		printlog("Legacy keys detected, conflict with IN_FOLLOWERMENU_CYCLENEXT. Automatically rebound IN_TURNR: %d (Right arrow key)\n", impulses[IN_TURNR]);
	}

	return 0;
}

int loadDefaultConfig()
{
	char path[PATH_MAX];
	completePath(path, "default.cfg", outputdir);
	return loadConfig(path);
}

/*-------------------------------------------------------------------------------

	saveConfig

	Opens the provided config file and saves the status of certain variables
	therein

-------------------------------------------------------------------------------*/

int saveConfig(char* filename)
{
	char path[PATH_MAX];
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

	completePath(path, filename, outputdir);

	// open the config file
	if ( (fp = fopen(path, "wb")) == NULL )
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
	if ( mute_audio_on_focus_lost )
	{
		fprintf(fp, "/muteaudiofocuslost\n");
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
	fprintf(fp, "/minimaptransparencyfg %d\n", minimapTransparencyForeground);
	fprintf(fp, "/minimaptransparencybg %d\n", minimapTransparencyBackground);
	fprintf(fp, "/minimapscale %d\n", minimapScale);
	fprintf(fp, "/minimapobjectzoom %d\n", minimapObjectZoom);
	if ( uiscale_charactersheet )
	{
		fprintf(fp, "/uiscale_charsheet\n");
	}
	if ( uiscale_skillspage )
	{
		fprintf(fp, "/uiscale_skillsheet\n");
	}
	fprintf(fp, "/uiscale_inv %f\n", uiscale_inventory);
	fprintf(fp, "/uiscale_hotbar %f\n", uiscale_hotbar);
	fprintf(fp, "/uiscale_chatbox %f\n", uiscale_chatlog);
	fprintf(fp, "/uiscale_playerbars %f\n", uiscale_playerbars);
	if ( hide_playertags )
	{
		fprintf(fp, "/hideplayertags\n");
	}
	if ( show_skill_values )
	{
		fprintf(fp, "/showskillvalues\n");
	}
	if ( disableMultithreadedSteamNetworking )
	{
		fprintf(fp, "/disablenetworkmultithreading\n");
	}
	if ( !gamemods_mountedFilepaths.empty() )
	{
		std::vector<std::pair<std::string, std::string>>::iterator it;
		for ( it = gamemods_mountedFilepaths.begin(); it != gamemods_mountedFilepaths.end(); ++it )
		{
			fprintf(fp, "/loadmod dir:%s name:%s", (*it).first.c_str(), (*it).second.c_str());
#ifdef STEAMWORKS
			for ( std::vector<std::pair<std::string, uint64>>::iterator itId = gamemods_workshopLoadedFileIDMap.begin();
				itId != gamemods_workshopLoadedFileIDMap.end(); ++itId )
			{
				if ( itId->first.compare((*it).second) == 0 )
				{
					fprintf(fp, " fileid:%lld", (*itId).second);
				}
			}
#endif // STEAMWORKS
			fprintf(fp, "\n");
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
	if (x >= HOTBAR_START_X && x < HOTBAR_START_X + (10 * hotbar_img->w * uiscale_hotbar) && y >= STATUS_Y - hotbar_img->h * uiscale_hotbar && y < STATUS_Y)
	{
		int relx = x - HOTBAR_START_X; //X relative to the start of the hotbar.
		return &hotbar[static_cast<int>(relx / (hotbar_img->w * uiscale_hotbar))]; //The slot will clearly be the x divided by the width of a slot
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

void FollowerRadialMenu::initFollowerMenuGUICursor(bool openInventory)
{
	if ( openInventory )
	{
		openStatusScreen(GUI_MODE_INVENTORY, INVENTORY_MODE_ITEM);
	}
	omousex = mousex;
	omousey = mousey;
	if ( menuX == -1 )
	{
		menuX = mousex;
	}
	if ( menuY == -1 )
	{
		menuY = mousey;
	}
}

void FollowerRadialMenu::closeFollowerMenuGUI(bool clearRecentEntity)
{
	followerToCommand = nullptr;
	menuX = -1;
	menuY = -1;
	moveToX = -1;
	moveToY = -1;
	if ( clearRecentEntity )
	{
		recentEntity = nullptr;
	}
	menuToggleClick = false;
	holdWheel = false;
	if ( accessedMenuFromPartySheet )
	{
		if ( optionSelected == ALLY_CMD_MOVETO_CONFIRM || optionSelected == ALLY_CMD_ATTACK_CONFIRM )
		{
			initFollowerMenuGUICursor(true);
		}
		accessedMenuFromPartySheet = false;
		if ( optionSelected != ALLY_CMD_CANCEL && optionSelected != -1 )
		{
			mousex = partySheetMouseX;
			mousey = partySheetMouseY;
			SDL_SetRelativeMouseMode(SDL_FALSE);
			SDL_WarpMouseInWindow(screen, mousex, mousey);
		}
	}
	optionSelected = -1;
}

bool FollowerRadialMenu::followerMenuIsOpen()
{
	if ( selectMoveTo || followerToCommand != nullptr )
	{
		return true;
	}
	return false;
}

void FollowerRadialMenu::drawFollowerMenu()
{
	if ( selectMoveTo )
	{
		if ( !followerToCommand )
		{
			selectMoveTo = false;
		}
		return;
	}

	int disableOption = 0;
	bool keepWheelOpen = false;
	if ( followerToCommand )
	{
		if ( players[clientnum] && players[clientnum]->entity
			&& followerToCommand->monsterTarget == players[clientnum]->entity->getUID() )
		{
			shootmode = true;
			CloseIdentifyGUI();
			closeRemoveCurseGUI();
			if ( openedChest[clientnum] )
			{
				openedChest[clientnum]->closeChest();
			}
			gui_mode = GUI_MODE_NONE;
			closeFollowerMenuGUI();
			return;
		}

		Stat* followerStats = followerToCommand->getStats();
		if ( !followerStats )
		{
			return;
		}
		int skillLVL = 0;
		if ( stats[clientnum] )
		{
			if ( optionSelected >= ALLY_CMD_DEFEND && optionSelected < ALLY_CMD_ATTACK_CONFIRM )
			{
				skillLVL = stats[clientnum]->PROFICIENCIES[PRO_LEADERSHIP] + statGetCHR(stats[clientnum]);
				if ( optionSelected == ALLY_CMD_ATTACK_SELECT )
				{
					if ( attackCommandOnly(followerStats->type) )
					{
						// attack only.
						disableOption = FollowerMenu.optionDisabledForCreature(skillLVL, followerStats->type, ALLY_CMD_ATTACK_CONFIRM);
					}
					else
					{
						disableOption = FollowerMenu.optionDisabledForCreature(skillLVL, followerStats->type, optionSelected);
					}
				}
				else
				{
					disableOption = FollowerMenu.optionDisabledForCreature(skillLVL, followerStats->type, optionSelected);
				}
			}
		}
		// process commands if option selected on the wheel.
		if ( (!(*inputPressed(impulses[IN_USE])) && !(*inputPressed(joyimpulses[INJOY_GAME_USE])) && !menuToggleClick && !holdWheel)
			|| ((*inputPressed(impulses[IN_USE]) || *inputPressed(joyimpulses[INJOY_GAME_USE])) && menuToggleClick)
			|| (!(*inputPressed(impulses[IN_FOLLOWERMENU] || !(*inputPressed(joyimpulses[INJOY_GAME_FOLLOWERMENU])) )) && holdWheel && !menuToggleClick)
			|| (*inputPressed(impulses[IN_FOLLOWERMENU_LASTCMD] || *inputPressed(joyimpulses[INJOY_GAME_FOLLOWERMENU_LASTCMD])) && optionPrevious != -1)
			)
		{
			if ( menuToggleClick )
			{
				*inputPressed(impulses[IN_USE]) = 0;
				*inputPressed(joyimpulses[INJOY_GAME_USE]) = 0;
				menuToggleClick = false;
				if ( optionSelected == -1 )
				{
					optionSelected = ALLY_CMD_CANCEL;
				}
			}

			if ( *inputPressed(impulses[IN_FOLLOWERMENU_LASTCMD]) || *inputPressed(joyimpulses[INJOY_GAME_FOLLOWERMENU_LASTCMD]) )
			{
				if ( optionPrevious != -1 )
				{
					if ( optionPrevious == ALLY_CMD_ATTACK_CONFIRM )
					{
						optionPrevious = ALLY_CMD_ATTACK_SELECT;
					}
					else if ( optionPrevious == ALLY_CMD_MOVETO_CONFIRM )
					{
						optionPrevious = ALLY_CMD_MOVETO_SELECT;
					}
					else if ( optionPrevious == ALLY_CMD_FOLLOW || optionPrevious == ALLY_CMD_DEFEND )
					{
						if ( followerToCommand->monsterAllyState == ALLY_STATE_DEFEND || followerToCommand->monsterAllyState == ALLY_STATE_MOVETO )
						{
							optionPrevious = ALLY_CMD_FOLLOW;
						}
						else
						{
							optionPrevious = ALLY_CMD_DEFEND;
						}
					}
					optionSelected = optionPrevious;
				}
			}

			keepWheelOpen = (optionSelected == ALLY_CMD_CLASS_TOGGLE || optionSelected == ALLY_CMD_PICKUP_TOGGLE);
			if ( disableOption != 0 )
			{
				keepWheelOpen = true;
			}

			if ( *inputPressed(impulses[IN_FOLLOWERMENU_LASTCMD]) || *inputPressed(joyimpulses[INJOY_GAME_FOLLOWERMENU_LASTCMD]) )
			{
				if ( keepWheelOpen )
				{
					// need to reset the coordinates of the mouse.
					initFollowerMenuGUICursor();
				}
				*inputPressed(impulses[IN_FOLLOWERMENU_LASTCMD]) = 0;
				*inputPressed(joyimpulses[INJOY_GAME_FOLLOWERMENU_LASTCMD]) = 0;
			}

			if ( optionSelected != -1 )
			{
				holdWheel = false;
				if ( optionSelected != ALLY_CMD_ATTACK_CONFIRM && optionSelected != ALLY_CMD_MOVETO_CONFIRM )
				{
					playSound(139, 64); // click
				}
				else
				{
					playSound(399, 48); // ping
				}
				// return to shootmode and close guis etc. TODO: tidy up interface code into 1 spot?
				if ( !keepWheelOpen )
				{
					if ( !accessedMenuFromPartySheet
						|| optionSelected == ALLY_CMD_MOVETO_SELECT
						|| optionSelected == ALLY_CMD_ATTACK_SELECT
						|| optionSelected == ALLY_CMD_CANCEL )
					{
						shootmode = true;
						CloseIdentifyGUI();
						closeRemoveCurseGUI();
						if ( openedChest[clientnum] )
						{
							openedChest[clientnum]->closeChest();
						}
						gui_mode = GUI_MODE_NONE;
					}
				}

				if ( disableOption == 0
					&& (optionSelected == ALLY_CMD_MOVETO_SELECT || optionSelected == ALLY_CMD_ATTACK_SELECT) )
				{
					// return early, let the player use select command point.
					selectMoveTo = true;
					return;
				}
				else
				{
					if ( disableOption == 0 )
					{
						if ( optionSelected == ALLY_CMD_DEFEND &&
							(followerToCommand->monsterAllyState == ALLY_STATE_DEFEND || followerToCommand->monsterAllyState == ALLY_STATE_MOVETO) )
						{
							optionSelected = ALLY_CMD_FOLLOW;
						}
						if ( multiplayer == CLIENT )
						{
							if ( optionSelected == ALLY_CMD_ATTACK_CONFIRM )
							{
								sendAllyCommandClient(clientnum, followerToCommand->getUID(), optionSelected, 0, 0, followerToCommand->monsterAllyInteractTarget);
							}
							else if ( optionSelected == ALLY_CMD_MOVETO_CONFIRM )
							{
								sendAllyCommandClient(clientnum, followerToCommand->getUID(), optionSelected, moveToX, moveToY);
							}
							else
							{
								sendAllyCommandClient(clientnum, followerToCommand->getUID(), optionSelected, 0, 0);
							}
						}
						else
						{
							followerToCommand->monsterAllySendCommand(optionSelected, moveToX, moveToY, followerToCommand->monsterAllyInteractTarget);
						}
					}

					if ( optionSelected != ALLY_CMD_CANCEL && disableOption == 0 )
					{
						optionPrevious = optionSelected;
					}

					if ( !keepWheelOpen )
					{
						closeFollowerMenuGUI();
					}
					optionSelected = -1; 
				}
			}
			else
			{
				menuToggleClick = true;
			}
		}
	}

	if ( followerToCommand )
	{
		int skillLVL = 0;
		Stat* followerStats = followerToCommand->getStats();
		if ( !followerStats )
		{
			return;
		}
		if ( stats[clientnum] )
		{
			skillLVL = stats[clientnum]->PROFICIENCIES[PRO_LEADERSHIP] + statGetCHR(stats[clientnum]);
		}

		SDL_Rect src;
		src.x = xres / 2;
		src.y = yres / 2;

		int numoptions = 8;
		real_t angleStart = PI / 2 - (PI / numoptions);
		real_t angleMiddle = angleStart + PI / numoptions;
		real_t angleEnd = angleMiddle + PI / numoptions;
		int radius = 140;
		int thickness = 70;
		src.h = radius;
		src.w = radius;
		if ( yres <= 768 )
		{
			radius = 110;
			thickness = 70;
			src.h = 125;
			src.w = 125;
		}
		int highlight = -1;
		int i = 0;

		int width = 0;
		TTF_SizeUTF8(ttf12, language[3036], &width, nullptr);
		if ( yres < 768 )
		{
			ttfPrintText(ttf12, src.x - width / 2, src.y - radius - thickness - 14, language[3036]);
		}
		else
		{
			ttfPrintText(ttf12, src.x - width / 2, src.y - radius - thickness - 24, language[3036]);
		}

		drawImageRing(fancyWindow_bmp, nullptr, radius, thickness, 40, 0, PI * 2, 156);

		for ( i = 0; i < numoptions; ++i )
		{
			// draw borders around ring.
			drawLine(xres / 2 + (radius - thickness) * cos(angleStart), yres / 2 - (radius - thickness) * sin(angleStart),
				xres / 2 + (radius + thickness) * cos(angleStart), yres / 2 - (radius + thickness) * sin(angleStart), uint32ColorGray(*mainsurface), 192);
			drawLine(xres / 2 + (radius - thickness) * cos(angleEnd), yres / 2 - (radius - thickness) * sin(angleEnd),
				xres / 2 + (radius + thickness - 1) * cos(angleEnd), yres / 2 - (radius + thickness - 1) * sin(angleEnd), uint32ColorGray(*mainsurface), 192);

			drawArcInvertedY(xres / 2, yres / 2, radius - thickness, std::round((angleStart * 180) / PI), ((angleEnd * 180) / PI), uint32ColorGray(*mainsurface), 192);
			drawArcInvertedY(xres / 2, yres / 2, (radius + thickness), std::round((angleStart * 180) / PI), ((angleEnd * 180) / PI) + 1, uint32ColorGray(*mainsurface), 192);

			angleStart += 2 * PI / numoptions;
			angleMiddle = angleStart + PI / numoptions;
			angleEnd = angleMiddle + PI / numoptions;
		}

		angleStart = PI / 2 - (PI / numoptions);
		angleMiddle = angleStart + PI / numoptions;
		angleEnd = angleMiddle + PI / numoptions;

		bool mouseInCenterButton = sqrt(pow((omousex - menuX), 2) + pow((omousey - menuY), 2)) < (radius - thickness);

		for ( i = 0; i < numoptions; ++i )
		{
			// see if mouse cursor is within an option.
			if ( highlight == -1 )
			{
				if ( !mouseInCenterButton )
				{
					real_t x1 = menuX + (radius + thickness + 45) * cos(angleEnd);
					real_t y1 = menuY - (radius + thickness + 45) * sin(angleEnd);
					real_t x2 = menuX + 5 * cos(angleMiddle);
					real_t y2 = menuY - 5 * sin(angleMiddle);
					real_t x3 = menuX + (radius + thickness + 45) * cos(angleStart);
					real_t y3 = menuY - (radius + thickness + 45) * sin(angleStart);
					real_t a = ((y2 - y3)*(omousex - x3) + (x3 - x2)*(omousey - y3)) / ((y2 - y3)*(x1 - x3) + (x3 - x2)*(y1 - y3));
					real_t b = ((y3 - y1)*(omousex - x3) + (x1 - x3)*(omousey - y3)) / ((y2 - y3)*(x1 - x3) + (x3 - x2)*(y1 - y3));
					real_t c = 1 - a - b;
					if ( (0 <= a && a <= 1) && (0 <= b && b <= 1) && (0 <= c && c <= 1) )
					{
						//barycentric calc for figuring if mouse point is within triangle.
						highlight = i;
						drawImageRing(fancyWindow_bmp, &src, radius, thickness, (numoptions) * 8, angleStart, angleEnd, 192);

						// draw borders around highlighted item.
						Uint32 borderColor = uint32ColorBaronyBlue(*mainsurface);
						if ( optionDisabledForCreature(skillLVL, followerStats->type, i) != 0 )
						{
							borderColor = uint32ColorOrange(*mainsurface);
						}
						drawLine(xres / 2 + (radius - thickness) * cos(angleStart), yres / 2 - (radius - thickness) * sin(angleStart),
							xres / 2 + (radius + thickness) * cos(angleStart), yres / 2 - (radius + thickness) * sin(angleStart), borderColor, 192);
						drawLine(xres / 2 + (radius - thickness) * cos(angleEnd), yres / 2 - (radius - thickness) * sin(angleEnd),
							xres / 2 + (radius + thickness - 1) * cos(angleEnd), yres / 2 - (radius + thickness - 1) * sin(angleEnd), borderColor, 192);

						drawArcInvertedY(xres / 2, yres / 2, radius - thickness, std::round((angleStart * 180) / PI), ((angleEnd * 180) / PI), borderColor, 192);
						drawArcInvertedY(xres / 2, yres / 2, (radius + thickness), std::round((angleStart * 180) / PI), ((angleEnd * 180) / PI) + 1, borderColor, 192);
					}
				}
			}

			SDL_Rect txt;
			txt.x = src.x + src.w * cos(angleMiddle);
			txt.y = src.y - src.h * sin(angleMiddle);
			txt.w = 0;
			txt.h = 0;
			SDL_Rect img;
			img.x = txt.x - sidebar_unlock_bmp->w / 2;
			img.y = txt.y - sidebar_unlock_bmp->h / 2;
			img.w = sidebar_unlock_bmp->w;
			img.h = sidebar_unlock_bmp->h;

			// draw the text for the menu wheel.

			if ( optionDisabledForCreature(skillLVL, followerStats->type, i) != 0 )
			{
				drawImage(sidebar_unlock_bmp, nullptr, &img); // locked menu options
			}
			else if ( i == ALLY_CMD_DEFEND
				&& (followerToCommand->monsterAllyState == ALLY_STATE_DEFEND || followerToCommand->monsterAllyState == ALLY_STATE_MOVETO) )
			{
				TTF_SizeUTF8(ttf12, language[3037 + i + 8], &width, nullptr);
				ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, language[3037 + i + 8]);
			}
			else
			{
				TTF_SizeUTF8(ttf12, language[3037 + i], &width, nullptr);
				if ( i == ALLY_CMD_CLASS_TOGGLE )
				{
					// draw higher.
					ttfPrintText(ttf12, txt.x - width / 2, txt.y - 12, language[3037 + i]);
					TTF_SizeUTF8(ttf12, language[3053 + followerToCommand->monsterAllyClass], &width, nullptr);
					ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, language[3053 + followerToCommand->monsterAllyClass]);
				}
				else if ( i == ALLY_CMD_PICKUP_TOGGLE )
				{
					// draw higher.
					TTF_SizeUTF8(ttf12, "Pickup", &width, nullptr);
					ttfPrintText(ttf12, txt.x - width / 2, txt.y - 24, language[3037 + i]);
					TTF_SizeUTF8(ttf12, language[3056 + followerToCommand->monsterAllyPickupItems], &width, nullptr);
					ttfPrintText(ttf12, txt.x - width / 2, txt.y + 12, language[3056 + followerToCommand->monsterAllyPickupItems]);
				}
				else if ( i == ALLY_CMD_DROP_EQUIP )
				{
					ttfPrintText(ttf12, txt.x - width / 2, txt.y - 12, language[3037 + i]);
					if ( skillLVL >= SKILL_LEVEL_LEGENDARY )
					{
						TTF_SizeUTF8(ttf12, language[3061], &width, nullptr);
						ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, language[3061]);
					}
					else if ( skillLVL >= SKILL_LEVEL_MASTER )
					{
						TTF_SizeUTF8(ttf12, language[3060], &width, nullptr);
						ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, language[3060]);
					}
					else
					{
						TTF_SizeUTF8(ttf12, language[3059], &width, nullptr);
						ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, language[3059]);
					}
				}
				else if ( i == ALLY_CMD_SPECIAL )
				{
					TTF_SizeUTF8(ttf12, language[3037 + i], &width, nullptr);
					ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, language[3037 + i]);
				}
				else if ( i == ALLY_CMD_ATTACK_SELECT )
				{
					if ( !attackCommandOnly(followerStats->type) )
					{
						if ( optionDisabledForCreature(skillLVL, followerStats->type, ALLY_CMD_ATTACK_CONFIRM) == 0 )
						{
							TTF_SizeUTF8(ttf12, "Interact / ", &width, nullptr);
							ttfPrintText(ttf12, txt.x - width / 2, txt.y - 12, language[3051]);
						}
						else
						{
							TTF_SizeUTF8(ttf12, language[3037 + i], &width, nullptr);
							ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, language[3037 + i]);
						}
					}
					else
					{
						TTF_SizeUTF8(ttf12, language[3104], &width, nullptr); // print just attack if no world interaction.
						ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, language[3104]);
					}
				}
				else if ( i == ALLY_CMD_MOVETO_SELECT )
				{
					TTF_SizeUTF8(ttf12, language[3037 + i], &width, nullptr);
					ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, language[3037 + i]);
				}
				else
				{
					TTF_SizeUTF8(ttf12, language[3037 + i], &width, nullptr);
					ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, language[3037 + i]);
				}
			}

			angleStart += 2 * PI / numoptions;
			angleMiddle = angleStart + PI / numoptions;
			angleEnd = angleMiddle + PI / numoptions;
		}
		// draw center text.
		if ( mouseInCenterButton )
		{
			highlight = -1;
			//drawImageRing(fancyWindow_bmp, nullptr, 35, 35, 40, 0, 2 * PI, 192);
			drawCircle(xres / 2, yres / 2, radius - thickness, uint32ColorBaronyBlue(*mainsurface), 192);
			//TTF_SizeUTF8(ttf12, language[3063], &width, nullptr);
			//ttfPrintText(ttf12, xres / 2 - width / 2, yres / 2 - 8, language[3063]);
		}

		if ( optionSelected == -1 && disableOption == 0 && highlight != -1 )
		{
			// in case optionSelected is cleared, but we're still highlighting text (happens on next frame when clicking on disabled option.)
			if ( highlight == ALLY_CMD_ATTACK_SELECT )
			{
				if ( attackCommandOnly(followerStats->type) )
				{
					// attack only.
					disableOption = FollowerMenu.optionDisabledForCreature(skillLVL, followerStats->type, ALLY_CMD_ATTACK_CONFIRM);
				}
				else
				{
					disableOption = FollowerMenu.optionDisabledForCreature(skillLVL, followerStats->type, highlight);
				}
			}
			else
			{
				disableOption = FollowerMenu.optionDisabledForCreature(skillLVL, followerStats->type, highlight);
			}
		}

		if ( disableOption != 0 )
		{
			SDL_Rect tooltip;
			tooltip.x = omousex + 16;
			tooltip.y = omousey + 16;
			tooltip.w = longestline(language[3062]) * TTF12_WIDTH + 8;
			tooltip.h = TTF12_HEIGHT * 2 + 8;

			if ( disableOption == -2 ) // disabled due to cooldown
			{
				tooltip.h = TTF12_HEIGHT + 8;
				tooltip.w = longestline(language[3092]) * TTF12_WIDTH + 8;
				drawTooltip(&tooltip);
				ttfPrintTextFormattedColor(ttf12, tooltip.x + 4, tooltip.y + 6, uint32ColorOrange(*mainsurface), language[3092]);
			}
			else if ( disableOption == -1 ) // disabled due to creature type
			{
				tooltip.h = TTF12_HEIGHT + 8;
				tooltip.w = longestline(language[3103]) * TTF12_WIDTH + 8;
				if ( followerStats->type < KOBOLD ) //Original monster count
				{
					tooltip.w += strlen(language[90 + followerStats->type]) * TTF12_WIDTH;
					drawTooltip(&tooltip);
					ttfPrintTextFormattedColor(ttf12, tooltip.x + 4, tooltip.y + 6,
						uint32ColorOrange(*mainsurface), language[3103], language[90 + followerStats->type]);
				}
				else if ( followerStats->type >= KOBOLD ) //New monsters
				{
					tooltip.w += strlen(language[2000 + followerStats->type - KOBOLD]) * TTF12_WIDTH;
					drawTooltip(&tooltip);
					ttfPrintTextFormattedColor(ttf12, tooltip.x + 4, tooltip.y + 6, 
						uint32ColorOrange(*mainsurface), language[3103], language[2000 + followerStats->type - KOBOLD]);
				}
			}
			else
			{
				drawTooltip(&tooltip);
				std::string requirement = "";
				std::string current = "";
				if ( highlight >= ALLY_CMD_DEFEND && highlight <= ALLY_CMD_END && highlight != ALLY_CMD_CANCEL )
				{
					switch ( std::min(disableOption, SKILL_LEVEL_LEGENDARY) )
					{
						case 0:
							requirement = language[363];
							break;
						case SKILL_LEVEL_NOVICE:
							requirement = language[364];
							break;
						case SKILL_LEVEL_BASIC:
							requirement = language[365];
							break;
						case SKILL_LEVEL_SKILLED:
							requirement = language[366];
							break;
						case SKILL_LEVEL_EXPERT:
							requirement = language[367];
							break;
						case SKILL_LEVEL_MASTER:
							requirement = language[368];
							break;
						case SKILL_LEVEL_LEGENDARY:
							requirement = language[369];
							break;
						default:
							break;
					}
					requirement.erase(std::remove(requirement.begin(), requirement.end(), ' '), requirement.end()); // trim whitespace

					if ( skillLVL >= SKILL_LEVEL_LEGENDARY )
					{
						current = language[369];
					}
					else if ( skillLVL >= SKILL_LEVEL_MASTER )
					{
						current = language[368];
					}
					else if ( skillLVL >= SKILL_LEVEL_EXPERT )
					{
						current = language[367];
					}
					else if ( skillLVL >= SKILL_LEVEL_SKILLED )
					{
						current = language[366];
					}
					else if ( skillLVL >= SKILL_LEVEL_BASIC )
					{
						current = language[365];
					}
					else if ( skillLVL >= SKILL_LEVEL_NOVICE )
					{
						current = language[364];
					}
					else
					{
						current = language[363];
					}
					current.erase(std::remove(current.begin(), current.end(), ' '), current.end()); // trim whitespace
				}
				ttfPrintTextFormattedColor(ttf12, tooltip.x + 4, tooltip.y + 6, 
					uint32ColorOrange(*mainsurface), language[3062], requirement.c_str(), current.c_str());
			}
		}

		if ( !keepWheelOpen )
		{
			optionSelected = highlight; // don't reselect if we're keeping the wheel open by using a toggle option.
		}
	}
}

int FollowerRadialMenu::numMonstersToDrawInParty()
{
	int players = 0;
	for ( int c = 0; c < MAXPLAYERS; ++c )
	{
		if ( !client_disconnected[c] )
		{
			++players;
		}
	}
	return std::max(2, (maxMonstersToDraw - std::max(0, players - 1) * 2));
}

void FollowerRadialMenu::selectNextFollower()
{
	if ( !stats[clientnum] )
	{
		return;
	}

	int numFollowers = list_Size(&stats[clientnum]->FOLLOWERS);

	if ( numFollowers <= 0 )
	{
		return;
	}

	if ( !recentEntity ) // set first follower to be the selected one.
	{
		node_t* node = stats[clientnum]->FOLLOWERS.first;
		if ( node )
		{
			Entity* follower = uidToEntity(*((Uint32*)node->element));
			if ( follower )
			{
				recentEntity = follower;
				FollowerMenu.sidebarScrollIndex = 0;
				return;
			}
		}
	}
	else if ( numFollowers == 1 )
	{
		// only 1 follower, no work to do.
		FollowerMenu.sidebarScrollIndex = 0;
		return;
	}

	int monstersToDraw = numMonstersToDrawInParty();

	node_t* node2 = nullptr;
	int i = 0;
	for ( node_t* node = stats[clientnum]->FOLLOWERS.first; node != nullptr; node = node->next, ++i)
	{
		Entity* follower = uidToEntity(*((Uint32*)node->element));
		if ( follower == recentEntity )
		{
			if ( node->next != nullptr )
			{
				follower = uidToEntity(*((Uint32*)(node->next)->element));
				if ( follower )
				{
					recentEntity = follower;
					if ( followerToCommand )
					{
						followerToCommand = follower; // if we had the menu open, we're now controlling the new selected follower.
					}
					if ( numFollowers > monstersToDraw )
					{
						if ( monstersToDraw > 1 )
						{
							if ( i < sidebarScrollIndex || i >= sidebarScrollIndex + monstersToDraw )
							{
								sidebarScrollIndex = std::min(i, numFollowers - monstersToDraw - 1);
							}
							if ( i != 0 )
							{
								sidebarScrollIndex = std::min(sidebarScrollIndex + 1, numFollowers - monstersToDraw - 1);
							}
						}
					}
				}
			}
			else
			{
				node2 = stats[clientnum]->FOLLOWERS.first; // loop around to first index.
				follower = uidToEntity(*((Uint32*)(node2)->element));
				if ( follower )
				{
					recentEntity = follower;
					if ( followerToCommand )
					{
						followerToCommand = follower; // if we had the menu open, we're now controlling the new selected follower.
					}
					sidebarScrollIndex = 0;
				}
			}
			if ( recentEntity )
			{
				createParticleFollowerCommand(recentEntity->x, recentEntity->y, 0, 174);
				playSound(139, 64);
			}
			return;
		}
	}
}

void FollowerRadialMenu::updateScrollPartySheet()
{
	if ( !stats[clientnum] )
	{
		return;
	}

	int numFollowers = list_Size(&stats[clientnum]->FOLLOWERS);

	if ( numFollowers <= 0 )
	{
		return;
	}

	int monstersToDraw = numMonstersToDrawInParty();

	if ( !recentEntity ) // set first follower to be the selected one.
	{
		return;
	}
	else if ( numFollowers == 1 || numFollowers <= monstersToDraw )
	{
		// only 1 follower or limit not reached, no work to do.
		FollowerMenu.sidebarScrollIndex = 0;
		return;
	}

	int i = 0;

	for ( node_t* node = stats[clientnum]->FOLLOWERS.first; node != nullptr; node = node->next, ++i )
	{
		Entity* follower = uidToEntity(*((Uint32*)node->element));
		if ( follower == recentEntity )
		{
			if ( monstersToDraw > 1 )
			{
				if ( i < sidebarScrollIndex || i >= sidebarScrollIndex + monstersToDraw )
				{
					sidebarScrollIndex = std::min(i, numFollowers - monstersToDraw - 1);
				}
			}
			break;
		}
	}
}

bool FollowerRadialMenu::allowedInteractEntity(Entity& selectedEntity)
{
	if ( optionSelected != ALLY_CMD_ATTACK_SELECT )
	{
		return false;
	}

	if ( !followerToCommand )
	{
		return false;
	}

	if ( followerToCommand == &selectedEntity )
	{
		return false;
	}

	Stat* followerStats = followerToCommand->getStats();
	if ( !followerStats )
	{
		return false;
	}

	bool interactItems = allowedInteractItems(followerStats->type) || allowedInteractFood(followerStats->type);
	bool interactWorld = allowedInteractWorld(followerStats->type);

	int skillLVL = stats[clientnum]->PROFICIENCIES[PRO_LEADERSHIP] + statGetCHR(stats[clientnum]);
	bool enableAttack = (optionDisabledForCreature(skillLVL, followerStats->type, ALLY_CMD_ATTACK_CONFIRM) == 0);
	
	if ( !interactItems && !interactWorld && enableAttack )
	{
		strcpy(FollowerMenu.interactText, "Attack ");
	}
	else
	{
		strcpy(FollowerMenu.interactText, "Interact with ");
	}
	if ( selectedEntity.behavior == &actTorch && interactWorld )
	{
		strcat(FollowerMenu.interactText, items[TOOL_TORCH].name_identified);
	}
	else if ( (selectedEntity.behavior == &actSwitch || selectedEntity.sprite == 184) && interactWorld )
	{
		strcat(FollowerMenu.interactText, "switch");
	}
	else if ( selectedEntity.behavior == &actItem && interactItems )
	{
		if ( multiplayer != CLIENT )
		{
			if ( selectedEntity.skill[15] == 0 )
			{
				strcat(FollowerMenu.interactText, items[selectedEntity.skill[10]].name_unidentified);
			}
			else
			{
				strcat(FollowerMenu.interactText, items[selectedEntity.skill[10]].name_identified);
			}
		}
		else
		{
			strcat(FollowerMenu.interactText, "item");
		}
	}
	else if ( selectedEntity.behavior == &actMonster && enableAttack )
	{
		strcpy(FollowerMenu.interactText, "Attack ");
		int monsterType = selectedEntity.getMonsterTypeFromSprite();
		if ( monsterType < KOBOLD ) //Original monster count
		{
			strcat(FollowerMenu.interactText, language[90 + monsterType]);
		}
		else if ( monsterType >= KOBOLD ) //New monsters
		{
			strcat(FollowerMenu.interactText, language[2000 + monsterType - KOBOLD]);
		}
	}
	else
	{
		strcpy(FollowerMenu.interactText, "No interactions available");
		return false;
	}
	return true;
}

int FollowerRadialMenu::optionDisabledForCreature(int playerSkillLVL, int monsterType, int option)
{
	int creatureTier = 0;

	switch ( monsterType )
	{
		case HUMAN:
		case RAT:
		case SLIME:
		case SPIDER:
		case SKELETON:
		case SCORPION:
			creatureTier = 0;
			break;
		case GOBLIN:
		case TROLL:
		case GHOUL:
		case GNOME:
		case SCARAB:
		case AUTOMATON:
		case SUCCUBUS:
			creatureTier = 1;
			break;
		case CREATURE_IMP:
		case DEMON:
		case KOBOLD:
		case INCUBUS:
		case INSECTOID:
		case GOATMAN:
			creatureTier = 2;
			break;
		case CRYSTALGOLEM:
		case VAMPIRE:
		case COCKATRICE:
		case SHADOW:
			creatureTier = 3;
			break;
	}

	int requirement = AllyNPCSkillRequirements[option];

	if ( option == ALLY_CMD_SPECIAL
		&& followerToCommand->monsterAllySpecialCooldown != 0 )
	{
		return -2; // disabled due to cooldown.
	}

	switch ( option )
	{
		case ALLY_CMD_MOVEASIDE:
		case ALLY_CMD_CANCEL:
			if ( playerSkillLVL < requirement )
			{
				return requirement; // disabled due to basic skill requirements.
			}
			return 0; // all permitted.
			break;

		case ALLY_CMD_FOLLOW:
		case ALLY_CMD_DEFEND:
			if ( creatureTier > 0 )
			{
				requirement = 20 * creatureTier; // 20, 40, 60.
			}
			if ( playerSkillLVL < requirement )
			{
				return requirement; // disabled due to advanced skill requirements.
			}
			return 0;
			break;

		case ALLY_CMD_MOVETO_SELECT:
		case ALLY_CMD_MOVETO_CONFIRM:
			if ( creatureTier > 0 )
			{
				requirement += 20 * creatureTier; // 40, 60, 80.
			}
			if ( playerSkillLVL < requirement )
			{
				return requirement; // disabled due to advanced skill requirements.
			}
			return 0;
			break;

		case ALLY_CMD_DROP_EQUIP:
			if ( !allowedInteractItems(monsterType) )
			{
				return -1; // disabled due to creature.
			}
			else if ( creatureTier > 0 )
			{
				requirement += 20 * creatureTier; // 60, 80, 100
			}
			if ( playerSkillLVL < requirement )
			{
				return requirement; // disabled due to advanced skill requirements.
			}
			return 0;
			break;

		case ALLY_CMD_ATTACK_SELECT:
			if ( attackCommandOnly(monsterType) )
			{
				// attack only.
				if ( creatureTier == 3 && playerSkillLVL < SKILL_LEVEL_LEGENDARY )
				{
					return SKILL_LEVEL_LEGENDARY; // disabled due to advanced skill requirements.
				}
				else if ( creatureTier == 2 && playerSkillLVL < SKILL_LEVEL_MASTER )
				{
					return SKILL_LEVEL_MASTER; // disabled due to advanced skill requirements.
				}
				else if ( playerSkillLVL < AllyNPCSkillRequirements[ALLY_CMD_ATTACK_CONFIRM] )
				{
					return AllyNPCSkillRequirements[ALLY_CMD_ATTACK_CONFIRM];
				}
				return 0;
			}
			else
			{
				if ( playerSkillLVL < AllyNPCSkillRequirements[ALLY_CMD_ATTACK_SELECT] )
				{
					return AllyNPCSkillRequirements[ALLY_CMD_ATTACK_SELECT];
				}
				return 0;
			}
			break;

		case ALLY_CMD_ATTACK_CONFIRM:
			if ( creatureTier == 3 && playerSkillLVL < SKILL_LEVEL_LEGENDARY )
			{
				return SKILL_LEVEL_LEGENDARY; // disabled due to advanced skill requirements.
			}
			else if ( creatureTier == 2 && playerSkillLVL < SKILL_LEVEL_MASTER )
			{
				return SKILL_LEVEL_MASTER; // disabled due to advanced skill requirements.
			}
			else if ( playerSkillLVL < AllyNPCSkillRequirements[ALLY_CMD_ATTACK_CONFIRM] )
			{
				return AllyNPCSkillRequirements[ALLY_CMD_ATTACK_CONFIRM];
			}
			return 0;
			break;

		case ALLY_CMD_CLASS_TOGGLE:
			if ( !allowedClassToggle(monsterType) )
			{
				return -1; // disabled due to creature.
			}
			if ( playerSkillLVL < requirement )
			{
				return requirement; // disabled due to basic skill requirements.
			}
			return 0;
			break;

		case ALLY_CMD_PICKUP_TOGGLE:
			if ( !allowedItemPickupToggle(monsterType) )
			{
				return -1; // disabled due to creature.
			}
			if ( playerSkillLVL < requirement )
			{
				return requirement; // disabled due to basic skill requirements.
			}
			return 0;
			break;

		case ALLY_CMD_SPECIAL:
			if ( creatureTier == 3 )
			{
				return -1; // disabled due to creature.
			}
			if ( playerSkillLVL < requirement )
			{
				return requirement; // disabled due to basic skill requirements.
			}
			break;
		default:
			if ( playerSkillLVL < requirement )
			{
				return requirement; // disabled due to basic skill requirements.
			}
			break;
	}
	return 0;
}

bool FollowerRadialMenu::allowedClassToggle(int monsterType)
{
	switch ( monsterType )
	{
		case HUMAN:
		case GOBLIN:
		case AUTOMATON:
		case GOATMAN:
			return true;
			break;
		default:
			break;
	}
	return false;
}

bool FollowerRadialMenu::allowedItemPickupToggle(int monsterType)
{
	switch ( monsterType )
	{
		case HUMAN:
		case GOBLIN:
		case AUTOMATON:
		case GOATMAN:
			return true;
			break;
		default:
			break;
	}
	return false;
}

bool FollowerRadialMenu::allowedInteractFood(int monsterType)
{
	switch ( monsterType )
	{
		case HUMAN:
		case GOBLIN:
		case GNOME:
		case KOBOLD:
		case GOATMAN:
		case SLIME:
		case INSECTOID:
		case SPIDER:
		case SCORPION:
		case RAT:
		case TROLL:
		case COCKATRICE:
		case SCARAB:
			return true;
			break;
		default:
			break;
	}
	return false;
}

bool FollowerRadialMenu::allowedInteractWorld(int monsterType)
{
	switch ( monsterType )
	{
		case HUMAN:
		case GOBLIN:
		case AUTOMATON:
		case GNOME:
		case KOBOLD:
		case GOATMAN:
			return true;
			break;
		default:
			break;
	}
	return false;
}

bool FollowerRadialMenu::allowedInteractItems(int monsterType)
{
	switch ( monsterType )
	{
		case HUMAN:
		case GOBLIN:
		case AUTOMATON:
		case GNOME:
		case KOBOLD:
		case GOATMAN:
		case INCUBUS:
		case INSECTOID:
		case SKELETON:
		case VAMPIRE:
		case SLIME:
			return true;
			break;
		default:
			break;
	}
	return false;
}

bool FollowerRadialMenu::attackCommandOnly(int monsterType)
{
	return !(allowedInteractItems(monsterType) || allowedInteractWorld(monsterType) || allowedInteractFood(monsterType));
}
