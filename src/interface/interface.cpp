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
#include "../engine/audio/sound.hpp"
#include "../shops.hpp"
#include "../menu.hpp"
#include "../player.hpp"
#include "../colors.hpp"
#include "../net.hpp"
#include "../draw.hpp"
#include "../scores.hpp"
#include "../scrolls.hpp"
#include "../lobbies.hpp"
#include "../ui/GameUI.hpp"
#include "../ui/MainMenu.hpp"
#include "../json.hpp"
#include "../mod_tools.hpp"
#include "../ui/Field.hpp"
#include "../ui/Image.hpp"
#include "../ui/Button.hpp"
#include "../ui/Slider.hpp"

Uint32 svFlags = 30;
Uint32 settings_svFlags = svFlags;
SDL_Surface* backdrop_blessed_bmp = nullptr;
SDL_Surface* backdrop_cursed_bmp = nullptr;
SDL_Surface* status_bmp = nullptr;
SDL_Surface* character_bmp = nullptr;
SDL_Surface* hunger_bmp = nullptr;
SDL_Surface* hunger_blood_bmp = nullptr;
SDL_Surface* hunger_boiler_bmp = nullptr;
SDL_Surface* hunger_boiler_hotflame_bmp = nullptr;
SDL_Surface* hunger_boiler_flame_bmp = nullptr;
SDL_Surface* minotaur_bmp = nullptr;
int textscroll = 0;
int inventorycategory = 7; // inventory window defaults to wildcard
int itemscroll = 0;
view_t camera_charsheet;
real_t camera_charsheet_offsetyaw = (330) * PI / 180;

SDL_Surface* font12x12_small_bmp = NULL;
SDL_Surface* inventoryChest_bmp = NULL;
SDL_Surface* invclose_bmp = NULL;
SDL_Surface* invgraball_bmp = NULL;
SDL_Surface* button_bmp = NULL, *smallbutton_bmp = NULL, *invup_bmp = NULL, *invdown_bmp = NULL;
bool gui_clickdrag[MAXPLAYERS] = { false };
int dragoffset_x[MAXPLAYERS] = { 0 };
int dragoffset_y[MAXPLAYERS] = { 0 };

list_t chestInv[MAXPLAYERS];

SDL_Surface* rightsidebar_titlebar_img = NULL;
SDL_Surface* rightsidebar_slot_img = NULL;
SDL_Surface* rightsidebar_slot_highlighted_img = NULL;
SDL_Surface* rightsidebar_slot_grayedout_img = NULL;
int rightsidebar_height = 0;

SDL_Surface* bookgui_img = NULL;
//SDL_Surface *nextpage_img = NULL;
//SDL_Surface *previouspage_img = NULL;
//SDL_Surface *bookclose_img = NULL;
//node_t* book_page = NULL;
//int bookgui_offset_x = 0;
//int bookgui_offset_y = 0;
//bool dragging_book_GUI = false;
//bool book_open = false;
//book_t* open_book = NULL;
//Item* open_book_item = NULL;
//int book_characterspace_x = 0;
//int book_characterspace_y = 0;

SDL_Surface* book_highlighted_left_img = NULL;
SDL_Surface* book_highlighted_right_img = NULL;

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
SDL_Surface* shopkeeper2_bmp = NULL;
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
SDL_Surface *effect_drunk_bmp = nullptr;
SDL_Surface *effect_polymorph_bmp = nullptr;
SDL_Surface *effect_hungover_bmp = nullptr;
int spellscroll = 0;
int magicspell_list_offset_x = 0;
int magicspell_list_offset_y = 0;
bool dragging_magicspell_list_GUI = false;
int magic_GUI_state = 0;
SDL_Rect magic_gui_pos;
SDL_Surface* sustained_spell_generic_icon = NULL;

int buttonclick = 0;

SDL_Surface* hotbar_img = NULL;
SDL_Surface* hotbar_spell_img = NULL;

list_t damageIndicators[MAXPLAYERS];

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
bool auto_appraise_new_items = true;
bool show_game_timer_always = false;
bool hide_statusbar = true;
bool hide_playertags = false;
bool show_skill_values = false;
real_t uiscale_chatlog = 1.f;
real_t uiscale_playerbars = 1.f;
real_t uiscale_hotbar = 1.f;
real_t uiscale_inventory = 1.f;
bool uiscale_charactersheet = false;
bool uiscale_skillspage = false;

EnemyHPDamageBarHandler enemyHPDamageBarHandler[MAXPLAYERS];
FollowerRadialMenu FollowerMenu[MAXPLAYERS];
GenericGUIMenu GenericGUI[MAXPLAYERS];

int EnemyHPDamageBarHandler::maxTickLifetime = 120;
int EnemyHPDamageBarHandler::maxTickFurnitureLifetime = 60;
int EnemyHPDamageBarHandler::shortDistanceHPBarFadeTicks = TICKS_PER_SECOND / 2;
real_t EnemyHPDamageBarHandler::shortDistanceHPBarFadeDistance = 1.0;
std::vector<std::pair<real_t, int>> EnemyHPDamageBarHandler::widthHealthBreakpointsMonsters;
std::vector<std::pair<real_t, int>> EnemyHPDamageBarHandler::widthHealthBreakpointsFurniture;

std::vector<std::pair<SDL_Surface**, std::string>> systemResourceImages =
{
	std::make_pair(&title_bmp, "images/system/title.png"),
	std::make_pair(&logo_bmp, "images/system/logo.png"),
	std::make_pair(&cursor_bmp, "images/system/cursor.png"),
	std::make_pair(&cross_bmp, "images/system/cross.png"),
	std::make_pair(&selected_cursor_bmp, "images/system/selectedcursor.png"),

	std::make_pair(&fancyWindow_bmp, "images/system/fancyWindow.png"),
	std::make_pair(&font8x8_bmp, "images/system/font8x8.png"),
	std::make_pair(&font12x12_bmp, "images/system/font12x12.png"),
	std::make_pair(&font16x16_bmp, "images/system/font16x16.png"),

	std::make_pair(&font12x12_small_bmp, "images/system/font12x12_small.png"),
	std::make_pair(&backdrop_blessed_bmp, "images/system/backdrop_blessed.png"),
	std::make_pair(&backdrop_cursed_bmp, "images/system/backdrop_cursed.png"),
	std::make_pair(&button_bmp, "images/system/ButtonHighlighted.png"),
	std::make_pair(&smallbutton_bmp, "images/system/SmallButtonHighlighted.png"),
	std::make_pair(&invup_bmp, "images/system/InventoryUpHighlighted.png"),
	std::make_pair(&invdown_bmp, "images/system/InventoryDownHighlighted.png"),
	std::make_pair(&status_bmp, "images/system/StatusBar.png"),
	std::make_pair(&character_bmp, "images/system/CharacterSheet.png"),
	std::make_pair(&hunger_bmp, "images/system/Hunger.png"),
	std::make_pair(&hunger_blood_bmp, "images/system/Hunger_blood.png"),
	std::make_pair(&hunger_boiler_bmp, "images/system/Hunger_boiler.png"),
	std::make_pair(&hunger_boiler_hotflame_bmp, "images/system/Hunger_boiler_hotfire.png"),
	std::make_pair(&hunger_boiler_flame_bmp, "images/system/Hunger_boiler_fire.png"),
	std::make_pair(&minotaur_bmp, "images/system/minotaur.png"),
	std::make_pair(&attributesleft_bmp, "images/system/AttributesLeftHighlighted.png"),
	std::make_pair(&attributesright_bmp, "images/system/AttributesRightHighlighted.png"),

	//General GUI images.
	std::make_pair(&attributesleftunclicked_bmp, "images/system/AttributesLeft.png"),
	std::make_pair(&attributesrightunclicked_bmp, "images/system/AttributesRight.png"),
	std::make_pair(&shopkeeper_bmp, "images/system/shopkeeper.png"),
	std::make_pair(&shopkeeper2_bmp, "images/system/shopkeeper2.png"),
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
	std::make_pair(&hotbar_spell_img, "images/system/magic/hotbar_spell.png"),

	//Misc effect images.
	std::make_pair(&effect_drunk_bmp, "images/system/drunk.png"),
	std::make_pair(&effect_polymorph_bmp, "images/system/polymorph.png"),
	std::make_pair(&effect_hungover_bmp, "images/system/hungover.png")
};

bool loadInterfaceResources()
{
	//General GUI images.
	font12x12_small_bmp = loadImage("images/system/font12x12_small.png");
	backdrop_blessed_bmp = loadImage("images/system/backdrop_blessed.png");
	backdrop_cursed_bmp = loadImage("images/system/backdrop_cursed.png");
	button_bmp = loadImage("images/system/ButtonHighlighted.png");
	smallbutton_bmp = loadImage("images/system/SmallButtonHighlighted.png");
	invup_bmp = loadImage("images/system/InventoryUpHighlighted.png");
	invdown_bmp = loadImage("images/system/InventoryDownHighlighted.png");
	status_bmp = loadImage("images/system/StatusBar.png");
	character_bmp = loadImage("images/system/CharacterSheet.png");
	hunger_bmp = loadImage("images/system/Hunger.png");
	hunger_blood_bmp = loadImage("images/system/Hunger_blood.png");
	hunger_boiler_bmp = loadImage("images/system/Hunger_boiler.png");
	hunger_boiler_hotflame_bmp = loadImage("images/system/Hunger_boiler_hotfire.png");
	hunger_boiler_flame_bmp = loadImage("images/system/Hunger_boiler_fire.png");
	minotaur_bmp = loadImage("images/system/minotaur.png"); // the file "images/system/minotaur.png" doesn't exist in current Data
	//textup_bmp = loadImage("images/system/TextBoxUpHighlighted.png");
	//textdown_bmp = loadImage("images/system/TextBoxDownHighlighted.png");
	attributesleft_bmp = loadImage("images/system/AttributesLeftHighlighted.png");
	attributesright_bmp = loadImage("images/system/AttributesRightHighlighted.png");
	attributesleftunclicked_bmp = loadImage("images/system/AttributesLeft.png");
	attributesrightunclicked_bmp = loadImage("images/system/AttributesRight.png");
	shopkeeper_bmp = loadImage("images/system/shopkeeper.png");
	shopkeeper2_bmp = loadImage("images/system/shopkeeper2.png");
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

	effect_drunk_bmp = loadImage("images/system/drunk.png");
	effect_polymorph_bmp = loadImage("images/system/polymorph.png");
	effect_hungover_bmp = loadImage("images/system/hungover.png");

	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		damageIndicators[i].first = nullptr;
		damageIndicators[i].last = nullptr;
	}

	return true;
}

void freeInterfaceResources()
{
	//int c;

	if (font12x12_small_bmp)
	{
		SDL_FreeSurface(font12x12_small_bmp);
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
	if ( hunger_blood_bmp )
	{
		SDL_FreeSurface(hunger_blood_bmp);
	}
	if ( hunger_boiler_bmp )
	{
		SDL_FreeSurface(hunger_boiler_bmp);
	}
	if ( hunger_boiler_hotflame_bmp )
	{
		SDL_FreeSurface(hunger_boiler_hotflame_bmp);
	}
	if ( hunger_boiler_flame_bmp )
	{
		SDL_FreeSurface(hunger_boiler_flame_bmp);
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
	if ( shopkeeper2_bmp != NULL )
	{
		SDL_FreeSurface(shopkeeper2_bmp);
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
	if ( effect_drunk_bmp )
	{
		SDL_FreeSurface(effect_drunk_bmp);
	}
	if ( effect_polymorph_bmp )
	{
		SDL_FreeSurface(effect_polymorph_bmp);
	}
	if ( effect_hungover_bmp )
	{
		SDL_FreeSurface(effect_hungover_bmp);
	}
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		list_FreeAll(&damageIndicators[i]);
	}
}

void defaultImpulses()
{
    // deprecated
}

void defaultConfig()
{
    // deprecated
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
	"FOLLOWERMENU_CYCLENEXT",
	"HOTBAR_SCROLL_LEFT",
	"HOTBAR_SCROLL_RIGHT",
	"HOTBAR_SCROLL_SELECT"
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

static auto genericgui_deselect_fn = [](Widget& widget) {
	if ( widget.isSelected() )
	{
		if ( !inputs.getVirtualMouse(widget.getOwner())->draw_cursor )
		{
			widget.deselect();
		}
	}
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
	char str[1024];
	File* fp;
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
	if ( (fp = FileIO::open(filename, "rb")) == NULL )
	{
		printlog("warning: config file '%s' does not exist!\n", filename);
		defaultConfig(); //Set up the game with the default config.
		return 0;
	}

	// read commands from it
	while ( fp->gets(str, 1024) != NULL )
	{
		if ( str[0] != '#' && str[0] != '\n' && str[0] != '\r' )   // if this line is not white space or a comment
		{
			// execute command
			consoleCommand(str);
		}
	}
	FileIO::close(fp);
	if ( mallocd )
	{
		free(filename);
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

	DEPRECATED

-------------------------------------------------------------------------------*/

int saveConfig(char const * const _filename)
{
	// as of 2021-11-16 saveConfig() is now deprecated.
	// The game will still load .cfg files the same as before,
	// it simply no longer auto-generates them.
	// game settings are saved in config/config.json.
	return 0;
}

/*-------------------------------------------------------------------------------

	mouseInBounds

	Returns true if the mouse is within the rectangle specified, otherwise
	returns false

-------------------------------------------------------------------------------*/

bool mouseInBounds(const int player, int x1, int x2, int y1, int y2)
{
	if ( inputs.getMouse(player, Inputs::OY) >= y1 && inputs.getMouse(player, Inputs::OY) < y2 )
	{
		if ( inputs.getMouse(player, Inputs::OX) >= x1 && inputs.getMouse(player, Inputs::OX) < x2)
		{
			return true;
		}
	}

	return false;
}

hotbar_slot_t* getCurrentHotbarUnderMouse(int player, int* outSlotNum)
{
	if ( players[player]->hotbar.hotbarFrame )
	{
		for ( Uint32 num = 0; num < NUM_HOTBAR_SLOTS; ++num )
		{
			if ( auto hotbarSlotFrame = players[player]->hotbar.getHotbarSlotFrame(num) )
			{
				if ( hotbarSlotFrame->capturesMouseInRealtimeCoords() )
				{
					if ( outSlotNum )
					{
						*outSlotNum = num;
					}
					return &players[player]->hotbar.slots()[num];
				}
			}
		}
		return nullptr;
	}
	return nullptr;
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
				return "Left Click";
			case 284:
				return "Middle Click";
			case 285:
				return "Right Click";
			case 286:
				return "Wheel up";
			case 287:
				return "Wheel down";
			case 288:
				return "Mouse 4";
			case 289:
				return "Mouse 5";
			case 290:
				return "Mouse 6";
			case 291:
				return "Mouse 7";
			case 292:
				return "Mouse 8";
			case 293:
				return "Mouse 11";
			case 294:
				return "Mouse 12";
			case 295:
				return "Mouse 13";
			case 296:
				return "Mouse 14";
			case 297:
				return "Mouse 15";
			case 298:
				return "Mouse 16";
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
		//return &joy_trigger_status[scancode - 299];
		// WIP SPLITSCREEN - DEPRECATE THIS
		dummy_value = 0;
		return &dummy_value;
	}
	else if (scancode < 318)
	{
		//return &joystatus[scancode - 301];
		// WIP SPLITSCREEN - DEPRECATE THIS
		dummy_value = 0;
		return &dummy_value;
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

Sint8* inputPressedForPlayer(int player, Uint32 scancode)
{
	if ( splitscreen )
	{
		// WIP SPLITSCREEN - keyboard only send for local player
		if ( !inputs.bPlayerUsingKeyboardControl(player) )
		{
			dummy_value = 0;
			return &dummy_value;
		}
	}

	if ( scancode >= 0 && scancode < 283 )
	{
		// usual (keyboard) scancode range
		return &keystatus[scancode];
	}
	else if ( scancode < 299 )
	{
		// mouse scancodes
		return &mousestatus[scancode - 282];
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

void Player::GUI_t::setHoveringOverModuleButton(Player::GUI_t::GUIModules moduleOfButton)
{
	if ( !inputs.getVirtualMouse(player.playernum)->draw_cursor )
	{
		hoveringButtonModule = MODULE_NONE;
		return;
	}
	hoveringButtonModule = moduleOfButton;
}
void Player::GUI_t::clearHoveringOverModuleButton()
{
	hoveringButtonModule = MODULE_NONE;
}
Player::GUI_t::GUIModules Player::GUI_t::hoveringOverModuleButton()
{
	return hoveringButtonModule;
}

bool Player::GUI_t::bActiveModuleHasNoCursor()
{
	if ( hoveringOverModuleButton() != MODULE_NONE )
	{
		return false;
	}
	switch ( activeModule )
	{
		case MODULE_BOOK_VIEW:
		case MODULE_SKILLS_LIST:
		case MODULE_NONE:
			return true;
		default:
			break;
	}
	return false;
}

bool Player::GUI_t::bActiveModuleUsesInventory()
{
	if ( hoveringOverModuleButton() != MODULE_NONE )
	{
		return false;
	}
	switch ( activeModule )
	{
		case MODULE_INVENTORY:
		case MODULE_HOTBAR:
		case MODULE_SHOP:
		case MODULE_CHEST:
		case MODULE_REMOVECURSE:
		case MODULE_IDENTIFY:
		case MODULE_TINKERING:
		case MODULE_SPELLS:
		case MODULE_ALCHEMY:
			return true;
		default:
			break;
	}
	return false;
}

bool Player::GUI_t::warpControllerToModule(bool moveCursorInstantly)
{
	bool warped = false;
	if ( auto vmouse = inputs.getVirtualMouse(player.playernum) )
	{
		if ( !vmouse->lastMovementFromController )
		{
			return false;
		}
		vmouse->draw_cursor = false;
	}

	if ( activeModule == MODULE_INVENTORY )
	{
		auto& inventoryUI = player.inventoryUI;
		Item* selectedItem = inputs.getUIInteraction(player.playernum)->selectedItem;
		if ( selectedItem && !player.inventoryUI.isItemFromChest(selectedItem) )
		{
			// we're holding an item, move to the selected item's slot
			auto slot = player.paperDoll.getSlotForItem(*selectedItem);
			if ( slot != Player::PaperDoll_t::PaperDollSlotType::SLOT_MAX )
			{
				int x, y;
				player.paperDoll.getCoordinatesFromSlotType(slot, x, y);
				inventoryUI.selectSlot(x, y);
			}
			else
			{
				// not equipped, move to it's inventory area
				if ( selectedItem->x >= 0 && selectedItem->x < inventoryUI.getSizeX()
					&& selectedItem->y >= 0 && selectedItem->y < inventoryUI.getSizeY() )
				{
					inventoryUI.selectSlot(selectedItem->x, selectedItem->y);
				}
			}
		}
		if ( inventoryUI.warpMouseToSelectedItem(nullptr, (Inputs::SET_CONTROLLER))
			&& inventoryUI.cursor.queuedModule == Player::GUI_t::MODULE_NONE )
		{
			if ( auto slot = inventoryUI.getInventorySlotFrame(inventoryUI.getSelectedSlotX(), inventoryUI.getSelectedSlotY()) )
			{
				SDL_Rect pos = slot->getAbsoluteSize();
				pos.x -= player.camera_virtualx1();
				pos.y -= player.camera_virtualy1();
				inventoryUI.updateSelectedSlotAnimation(pos.x, pos.y,
					inventoryUI.getSlotSize(), inventoryUI.getSlotSize(), moveCursorInstantly);
			}
		}
		return true;
	}
	else if ( activeModule == MODULE_SPELLS )
	{
		auto& inventoryUI = player.inventoryUI;
		if ( inventoryUI.warpMouseToSelectedSpell(nullptr, (Inputs::SET_CONTROLLER))
			&& inventoryUI.cursor.queuedModule == Player::GUI_t::MODULE_NONE )
		{
			if ( auto slot = inventoryUI.getSpellSlotFrame(inventoryUI.getSelectedSpellX(), inventoryUI.getSelectedSpellY()) )
			{
				SDL_Rect pos = slot->getAbsoluteSize();
				pos.x -= player.camera_virtualx1();
				pos.y -= player.camera_virtualy1();
				inventoryUI.updateSelectedSlotAnimation(pos.x, pos.y,
					inventoryUI.getSlotSize(), inventoryUI.getSlotSize(), moveCursorInstantly);
			}
		}
		return true;
	}
	else if ( activeModule == MODULE_CHEST )
	{
		auto& inventoryUI = player.inventoryUI;
		if ( inventoryUI.warpMouseToSelectedChestSlot(nullptr, (Inputs::SET_CONTROLLER))
			&& inventoryUI.cursor.queuedModule == Player::GUI_t::MODULE_NONE )
		{
			if ( auto slot = inventoryUI.getChestSlotFrame(inventoryUI.getSelectedChestX(), inventoryUI.getSelectedChestY()) )
			{
				SDL_Rect pos = slot->getAbsoluteSize();
				pos.x -= player.camera_virtualx1();
				pos.y -= player.camera_virtualy1();
				inventoryUI.updateSelectedSlotAnimation(pos.x, pos.y,
					inventoryUI.getSlotSize(), inventoryUI.getSlotSize(), moveCursorInstantly);
			}
		}
		return true;
	}
	else if ( activeModule == MODULE_SHOP )
	{
		auto& shopUI = player.shopGUI;
		auto& inventoryUI = player.inventoryUI;
		if ( shopUI.warpMouseToSelectedShopItem(nullptr, (Inputs::SET_CONTROLLER))
			&& inventoryUI.cursor.queuedModule == Player::GUI_t::MODULE_NONE )
		{
			if ( auto slot = shopUI.getShopSlotFrame(shopUI.getSelectedShopX(), shopUI.getSelectedShopY()) )
			{
				SDL_Rect pos = slot->getAbsoluteSize();
				pos.x -= player.camera_virtualx1();
				pos.y -= player.camera_virtualy1();
				inventoryUI.updateSelectedSlotAnimation(pos.x, pos.y,
					inventoryUI.getSlotSize(), inventoryUI.getSlotSize(), moveCursorInstantly);
			}
		}
		return true;
	}
	else if ( activeModule == MODULE_ALCHEMY )
	{
		auto& alchemyGUI = GenericGUI[player.playernum].alchemyGUI;
		auto& inventoryUI = player.inventoryUI;
		if ( alchemyGUI.warpMouseToSelectedAlchemyItem(nullptr, (Inputs::SET_CONTROLLER))
			&& inventoryUI.cursor.queuedModule == Player::GUI_t::MODULE_NONE )
		{
			if ( auto slot = alchemyGUI.getAlchemySlotFrame(alchemyGUI.getSelectedAlchemySlotX(), alchemyGUI.getSelectedAlchemySlotY()) )
			{
				SDL_Rect pos = slot->getAbsoluteSize();
				pos.x -= player.camera_virtualx1();
				pos.y -= player.camera_virtualy1();
				inventoryUI.updateSelectedSlotAnimation(pos.x, pos.y,
					inventoryUI.getSlotSize(), inventoryUI.getSlotSize(), moveCursorInstantly);
			}
		}
		return true;
	}
	else if ( activeModule == MODULE_TINKERING )
	{
		auto& tinkerGUI = GenericGUI[player.playernum].tinkerGUI;
		auto& inventoryUI = player.inventoryUI;
		if ( tinkerGUI.warpMouseToSelectedTinkerItem(nullptr, (Inputs::SET_CONTROLLER))
			&& inventoryUI.cursor.queuedModule == Player::GUI_t::MODULE_NONE )
		{
			if ( auto slot = tinkerGUI.getTinkerSlotFrame(tinkerGUI.getSelectedTinkerSlotX(), tinkerGUI.getSelectedTinkerSlotY()) )
			{
				SDL_Rect pos = slot->getAbsoluteSize();
				pos.x -= player.camera_virtualx1();
				pos.y -= player.camera_virtualy1();
				inventoryUI.updateSelectedSlotAnimation(pos.x, pos.y,
					inventoryUI.getSlotSize(), inventoryUI.getSlotSize(), moveCursorInstantly);
			}
		}
		return true;
	}
	else if ( activeModule == MODULE_HOTBAR )
	{
		warpMouseToSelectedHotbarSlot(player.playernum);
		return true;
	}
	else if ( activeModule == MODULE_CHARACTERSHEET )
	{
		const bool updateCursor = true;
		const bool usingMouse = false;
		player.characterSheet.selectElement(player.characterSheet.selectedElement, usingMouse, updateCursor);
		return true;
	}
	return warped;
}

void Player::GUI_t::activateModule(Player::GUI_t::GUIModules module)
{
	GUIModules oldModule = activeModule;
	activeModule = module;

	if ( oldModule != activeModule )
	{
		Frame* hudCursor = nullptr;
		if ( player.hud.cursorFrame )
		{
			hudCursor = player.hud.cursorFrame->findFrame("hud cursor");
		}
		if ( hudCursor && player.inventoryUI.selectedItemCursorFrame )
		{
			if ( (oldModule == MODULE_INVENTORY 
					|| oldModule == MODULE_HOTBAR 
					|| oldModule == MODULE_SPELLS
					|| oldModule == MODULE_CHEST 
					|| oldModule == MODULE_SHOP
					|| oldModule == MODULE_ALCHEMY
					|| oldModule == MODULE_TINKERING)
				&& !(activeModule == MODULE_INVENTORY 
					|| activeModule == MODULE_HOTBAR 
					|| activeModule == MODULE_SPELLS
					|| activeModule == MODULE_CHEST
					|| activeModule == MODULE_SHOP
					|| activeModule == MODULE_ALCHEMY
					|| activeModule == MODULE_TINKERING)
				&& !bActiveModuleHasNoCursor()
				&& hoveringOverModuleButton() == MODULE_NONE )
			{
				SDL_Rect size = player.inventoryUI.selectedItemCursorFrame->getSize();
				if ( !player.inventoryUI.selectedItemCursorFrame->isDisabled() )
				{
					size.x += player.hud.cursor.cursorToSlotOffset;
					size.y += player.hud.cursor.cursorToSlotOffset;
					size.w -= (2 * (player.hud.cursor.cursorToSlotOffset) - 1);
					size.h -= (2 * (player.hud.cursor.cursorToSlotOffset) - 1);
					player.hud.updateCursorAnimation(size.x, size.y, size.w, size.h, true);
				}
			}
			else if ( ((activeModule == MODULE_INVENTORY 
				|| activeModule == MODULE_HOTBAR 
				|| activeModule == MODULE_SPELLS
				|| activeModule == MODULE_CHEST
				|| activeModule == MODULE_SHOP
				|| activeModule == MODULE_ALCHEMY
				|| activeModule == MODULE_TINKERING)
				&& !(oldModule == MODULE_INVENTORY 
					|| oldModule == MODULE_HOTBAR 
					|| oldModule == MODULE_SPELLS
					|| oldModule == MODULE_CHEST
					|| oldModule == MODULE_SHOP
					|| oldModule == MODULE_ALCHEMY
					|| oldModule == MODULE_TINKERING))
				|| hoveringOverModuleButton() != MODULE_NONE )
			{
				SDL_Rect size = hudCursor->getSize();
				if ( !player.hud.cursorFrame->isDisabled() )
				{
					player.inventoryUI.updateSelectedSlotAnimation(size.x, size.y, size.w, size.h, true);
				}
			}
		}
	}
}

void Player::openStatusScreen(const int whichGUIMode, const int whichInventoryMode, const int whichModule)
{
	if ( !inputs.bPlayerIsControllable(playernum) )
	{
		return;
	}

	bool oldShootmode = shootmode;
	shootmode = false;

	if ( whichGUIMode != GUI_MODE_NONE && whichGUIMode != GUI_MODE_FOLLOWERMENU )
	{
		FollowerMenu[playernum].closeFollowerMenuGUI();
	}
	GenericGUI[playernum].closeGUI();

	int oldgui = gui_mode;
	gui_mode = whichGUIMode;
	if ( oldgui == GUI_MODE_NONE && whichGUIMode == GUI_MODE_INVENTORY )
	{
		this->hud.compactLayoutMode = HUD_t::COMPACT_LAYOUT_INVENTORY;
	}

	if ( oldShootmode )
	{
		if ( hud.cursorFrame )
		{
			// center the hud cursor - there is a first time warping of the inventory cursor but we can leave that in
			auto hudCursor = hud.cursorFrame->findFrame("hud cursor");
			hud.updateCursorAnimation((camera_virtualWidth() / 2), (camera_virtualHeight() / 2),
				hudCursor->getSize().w, hudCursor->getSize().h, true);

			// uncomment to always warp the inventory cursor from center of screen
			//SDL_Rect size = hudCursor->getSize();
			//inventoryUI.updateSelectedSlotAnimation(size.x, size.y, size.w, size.h, true);
		}
	}

	int oldmodule = GUI.activeModule;
	GUI.activateModule((GUI_t::GUIModules)whichModule);
	inputs.getUIInteraction(playernum)->selectedItem = nullptr;
	inputs.getUIInteraction(playernum)->selectedItemFromChest = 0;
	inputs.getUIInteraction(playernum)->toggleclick = false;
	GUI.closeDropdowns();

	inventory_mode = whichInventoryMode;

	Uint32 flags = (Inputs::SET_MOUSE | Inputs::SET_CONTROLLER | Inputs::UNSET_RELATIVE_MOUSE);

	bool warped = false;
	bool warpMouseToInventorySlot = false;
	if ( inputs.hasController(playernum)
		&& ((oldgui == GUI_MODE_NONE && whichGUIMode != GUI_MODE_NONE) 
			|| (oldmodule != GUI.activeModule && GUI.activeModule == GUI_t::MODULE_INVENTORY))
		&& !FollowerMenu[playernum].followerToCommand )
	{
		warpMouseToInventorySlot = true;
	}
	if ( warpMouseToInventorySlot )
	{
		// hide cursor, select an inventory slot and disable hotbar focus.
		int x = inventoryUI.getSelectedSlotX();
		int y = inventoryUI.getSelectedSlotY();
		if ( inventoryUI.selectedSlotInPaperDoll() )
		{
			if ( x == Inventory_t::DOLL_COLUMN_LEFT )
			{
				x = 0; // warp top left
			}
			else
			{
				x = inventoryUI.getSizeX() - 1; // warp top right
			}
			y = 0;
			inventoryUI.selectSlot(x, y);
		}
		if ( GUI.warpControllerToModule(true) )
		{
			warped = true;
		}
	}

	if ( !warped && oldShootmode )
	{
		inputs.warpMouse(playernum, camera_x1() + (camera_width() / 2), camera_y1() + (camera_height() / 2), flags);
	}
}

void Player::closeAllGUIs(CloseGUIShootmode shootmodeAction, CloseGUIIgnore whatToClose)
{
	GenericGUI[playernum].closeGUI();
	if ( whatToClose != CLOSEGUI_DONT_CLOSE_FOLLOWERGUI )
	{
		FollowerMenu[playernum].closeFollowerMenuGUI();
	}
	if ( whatToClose != CLOSEGUI_DONT_CLOSE_CHEST )
	{
		if ( openedChest[playernum] )
		{
			openedChest[playernum]->closeChest();
		}
	}

	if ( whatToClose != CLOSEGUI_DONT_CLOSE_SHOP )
	{
		closeShop(playernum);
	}
	gui_mode = GUI_MODE_NONE;
	GUI.activateModule(GUI_t::MODULE_NONE);

	inventoryUI.closeInventory();
	skillSheet.closeSkillSheet();
	bookGUI.closeBookGUI();

	if ( shootmodeAction == CLOSEGUI_ENABLE_SHOOTMODE )
	{
		inputs.getUIInteraction(playernum)->selectedItem = nullptr;
		inputs.getUIInteraction(playernum)->selectedItemFromChest = 0;
		inputs.getUIInteraction(playernum)->toggleclick = false;
		GUI.closeDropdowns();
		shootmode = true;
	}
}

void FollowerRadialMenu::initfollowerMenuGUICursor(bool openInventory)
{
	if ( openInventory )
	{
		//players[gui_player]->openStatusScreen(GUI_MODE_INVENTORY, INVENTORY_MODE_ITEM);
		players[gui_player]->openStatusScreen(GUI_MODE_FOLLOWERMENU, INVENTORY_MODE_ITEM);
	}

	//const Sint32 mousex = inputs.getMouse(player, Inputs::X);
	//const Sint32 mousey = inputs.getMouse(player, Inputs::Y);
	//const Sint32 omousex = inputs.getMouse(player, Inputs::OX);
	//const Sint32 omousey = inputs.getMouse(player, Inputs::OY);

	inputs.setMouse(gui_player, Inputs::OX, inputs.getMouse(gui_player, Inputs::X));
	inputs.setMouse(gui_player, Inputs::OY, inputs.getMouse(gui_player, Inputs::Y));

	//omousex = mousex;
	//omousey = mousey;
	if ( menuX == -1 )
	{
		menuX = inputs.getMouse(gui_player, Inputs::X);
	}
	if ( menuY == -1 )
	{
		menuY = inputs.getMouse(gui_player, Inputs::Y);
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
			initfollowerMenuGUICursor(true);
		}
		accessedMenuFromPartySheet = false;
		if ( optionSelected != ALLY_CMD_CANCEL && optionSelected != -1 )
		{
			//inputs.setMouse(player, Inputs::X, partySheetMouseX);
			//inputs.setMouse(player, Inputs::Y, partySheetMouseY);
			//mousex = partySheetMouseX;
			//mousey = partySheetMouseY;
			//SDL_SetRelativeMouseMode(SDL_FALSE);
			//SDL_WarpMouseInWindow(screen, mousex, mousey);

			// to verify for splitscreen
			Uint32 flags = (Inputs::SET_MOUSE | Inputs::SET_CONTROLLER | Inputs::UNSET_RELATIVE_MOUSE);
			inputs.warpMouse(gui_player, partySheetMouseX, partySheetMouseY, flags);

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

	Sint32 omousex = inputs.getMouse(gui_player, Inputs::OX);
	Sint32 omousey = inputs.getMouse(gui_player, Inputs::OY);

	Input& input = Input::inputs[gui_player];

	if ( followerToCommand )
	{
		if ( players[gui_player] && players[gui_player]->entity
			&& followerToCommand->monsterTarget == players[gui_player]->entity->getUID() )
		{
			players[gui_player]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_DONT_CLOSE_FOLLOWERGUI);
			return;
		}

		Stat* followerStats = followerToCommand->getStats();
		if ( !followerStats )
		{
			return;
		}
		bool tinkeringFollower = isTinkeringFollower(followerStats->type);
		int skillLVL = 0;
		if ( stats[gui_player] && players[gui_player] && players[gui_player]->entity
			&& players[gui_player]->bControlEnabled && !gamePaused
			&& !players[gui_player]->usingCommand() )
		{
			if ( input.binaryToggle("Command NPC") && optionPrevious != -1 )
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
			if ( optionSelected >= ALLY_CMD_DEFEND && optionSelected < ALLY_CMD_END && optionSelected != ALLY_CMD_ATTACK_CONFIRM )
			{
				skillLVL = stats[gui_player]->PROFICIENCIES[PRO_LEADERSHIP] + statGetCHR(stats[gui_player], players[gui_player]->entity);
				if ( tinkeringFollower )
				{
					skillLVL = stats[gui_player]->PROFICIENCIES[PRO_LOCKPICKING] + statGetPER(stats[gui_player], players[gui_player]->entity);
				}
				if ( followerToCommand->monsterAllySummonRank != 0 )
				{
					skillLVL = SKILL_LEVEL_LEGENDARY;
				}
				if ( optionSelected == ALLY_CMD_ATTACK_SELECT )
				{
					if ( attackCommandOnly(followerStats->type) )
					{
						// attack only.
						disableOption = optionDisabledForCreature(skillLVL, followerStats->type, ALLY_CMD_ATTACK_CONFIRM);
					}
					else
					{
						disableOption = optionDisabledForCreature(skillLVL, followerStats->type, optionSelected);
					}
				}
				else
				{
					disableOption = optionDisabledForCreature(skillLVL, followerStats->type, optionSelected);
				}
			}
		}
		// process commands if option selected on the wheel.
		if ( !(players[gui_player]->bControlEnabled && !gamePaused && !players[gui_player]->usingCommand()) )
		{
			// no action
		}
		else if ( (!input.binaryToggle("Use") && !input.binaryToggle("Show NPC Commands") && !menuToggleClick && !holdWheel)
			|| ((input.binaryToggle("Use") || input.binaryToggle("Show NPC Commands")) && menuToggleClick)
			|| (!input.binaryToggle("Show NPC Commands") && holdWheel && !menuToggleClick)
			|| (input.binaryToggle("Command NPC") && optionPrevious != -1)
			)
		{
			if ( menuToggleClick )
			{
			    input.consumeBinaryToggle("Use");
			    input.consumeBinaryToggle("Show NPC Commands");
				menuToggleClick = false;
				if ( optionSelected == -1 )
				{
					optionSelected = ALLY_CMD_CANCEL;
				}
			}

			bool usingLastCmd = false;
			if ( input.binaryToggle("Command NPC") )
			{
				usingLastCmd = true;
			}

			if ( followerStats->type == GYROBOT )
			{
				monsterGyroBotConvertCommand(&optionSelected);
			}
			else if ( followerStats->type == DUMMYBOT || followerStats->type == SENTRYBOT || followerStats->type == SPELLBOT )
			{
				if ( optionSelected == ALLY_CMD_SPECIAL )
				{
					optionSelected = ALLY_CMD_DUMMYBOT_RETURN;
				}
			}
			else if ( followerToCommand->monsterAllySummonRank != 0 && optionSelected == ALLY_CMD_CLASS_TOGGLE )
			{
				optionSelected = ALLY_CMD_RETURN_SOUL;
			}

			keepWheelOpen = (optionSelected == ALLY_CMD_CLASS_TOGGLE 
				|| optionSelected == ALLY_CMD_PICKUP_TOGGLE
				|| optionSelected == ALLY_CMD_GYRO_LIGHT_TOGGLE
				|| optionSelected == ALLY_CMD_GYRO_DETECT_TOGGLE);
			if ( disableOption != 0 && !usingLastCmd )
			{
				keepWheelOpen = true;
			}

			if ( input.binaryToggle("Command NPC") )
			{
				if ( keepWheelOpen )
				{
					// need to reset the coordinates of the mouse.
					initfollowerMenuGUICursor(false);
				}
				input.consumeBinaryToggle("Command NPC");
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
						players[gui_player]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_DONT_CLOSE_FOLLOWERGUI);
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
								sendAllyCommandClient(gui_player, followerToCommand->getUID(), optionSelected, 0, 0, followerToCommand->monsterAllyInteractTarget);
							}
							else if ( optionSelected == ALLY_CMD_MOVETO_CONFIRM )
							{
								sendAllyCommandClient(gui_player, followerToCommand->getUID(), optionSelected, moveToX, moveToY);
							}
							else
							{
								sendAllyCommandClient(gui_player, followerToCommand->getUID(), optionSelected, 0, 0);
							}
						}
						else
						{
							followerToCommand->monsterAllySendCommand(optionSelected, moveToX, moveToY, followerToCommand->monsterAllyInteractTarget);
						}
					}
					else if ( usingLastCmd )
					{
						// tell player current monster can't do what you asked (e.g using last command & swapping between monsters with different requirements)
						if ( disableOption < 0 )
						{
							messagePlayer(gui_player, MESSAGE_MISC, language[3640], getMonsterLocalizedName(followerStats->type).c_str());
						}
						else if ( tinkeringFollower )
						{
							messagePlayer(gui_player, MESSAGE_MISC, language[3639], getMonsterLocalizedName(followerStats->type).c_str());
						}
						else
						{
							messagePlayer(gui_player, MESSAGE_MISC, language[3638], getMonsterLocalizedName(followerStats->type).c_str());
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
		bool tinkeringFollower = isTinkeringFollower(followerStats->type);
		if ( stats[gui_player] && players[gui_player] && players[gui_player]->entity )
		{
			skillLVL = stats[gui_player]->PROFICIENCIES[PRO_LEADERSHIP] + statGetCHR(stats[gui_player], players[gui_player]->entity);
			if ( tinkeringFollower )
			{
				skillLVL = stats[gui_player]->PROFICIENCIES[PRO_LOCKPICKING] + statGetPER(stats[gui_player], players[gui_player]->entity);
			}
			else if ( followerToCommand->monsterAllySummonRank != 0 )
			{
				skillLVL = SKILL_LEVEL_LEGENDARY;
			}
		}

		const int centerx = players[gui_player]->camera_midx();
		const int centery = players[gui_player]->camera_midy();

		SDL_Rect src;
		src.x = centerx;
		src.y = centery;

		int numoptions = 8;
		real_t angleStart = PI / 2 - (PI / numoptions);
		real_t angleMiddle = angleStart + PI / numoptions;
		real_t angleEnd = angleMiddle + PI / numoptions;
		int radius = 140;
		int thickness = 70;
		src.h = radius;
		src.w = radius;
		if ( players[gui_player]->camera_height() <= 768 )
		{
			radius = 110;
			thickness = 70;
			src.h = 125;
			src.w = 125;
		}
		int highlight = -1;
		int i = 0;

		int width = 0;
		getSizeOfText(ttf12, language[3036], &width, nullptr);
		if ( players[gui_player]->camera_height() < 768 )
		{
			ttfPrintText(ttf12, src.x - width / 2, src.y - radius - thickness - 14, language[3036]);
		}
		else
		{
			ttfPrintText(ttf12, src.x - width / 2, src.y - radius - thickness - 24, language[3036]);
		}

		bool mouseInCenterButton = sqrt(pow((omousex - menuX), 2) + pow((omousey - menuY), 2)) < (radius - thickness);

		if ( inputs.hasController(gui_player) )
		{
			auto controller = inputs.getController(gui_player);
			if ( controller )
			{
				GameController::DpadDirection dir = controller->dpadDirToggle();
				if ( dir != GameController::DpadDirection::INVALID )
				{
					controller->consumeDpadDirToggle();
					highlight = dir;
					real_t angleMiddleForOption = PI / 2 + dir * (2 * PI / numoptions);
					omousex = centerx + (radius + thickness) / 2 * cos(angleMiddleForOption);
					omousey = centery + (radius + thickness) / 2 * sin(angleMiddleForOption);
					inputs.setMouse(gui_player, Inputs::OX, omousex);
					inputs.setMouse(gui_player, Inputs::OY, omousey);
					inputs.setMouse(gui_player, Inputs::X, omousex);
					inputs.setMouse(gui_player, Inputs::Y, omousey);
				}
				/*
				real_t magnitude = sqrt(pow(controller->getRightYPercent(), 2) + pow(controller->getRightXPercent(), 2));
				if ( magnitude > 1 )
				{
					real_t stickAngle = atan2(controller->getRightYPercent(), controller->getRightXPercent());
					while ( stickAngle >= (2 * PI + (PI / 2 - (PI / numoptions))) )
					{
						stickAngle -= PI * 2;
					}
					while ( stickAngle < (PI / 2 - (PI / numoptions)))
					{
						stickAngle += PI * 2;
					}
					real_t currentCursorAngle = atan2(omousey - menuY, omousex - menuX);
					while ( currentCursorAngle >= (2 * PI + (PI / 2 - (PI / numoptions))) )
					{
						currentCursorAngle -= PI * 2;
					}
					while ( currentCursorAngle < (PI / 2 - (PI / numoptions)) )
					{
						currentCursorAngle += PI * 2;
					}

					angleStart = PI / 2 - (PI / numoptions);
					angleMiddle = angleStart + PI / numoptions;
					angleEnd = angleMiddle + PI / numoptions;

					int newOption = -1;
					int currentOption = -1;
					for ( i = 0; i < numoptions; ++i )
					{
						if ( (stickAngle >= angleStart && stickAngle < angleEnd) )
						{
							newOption = i;
						}
						if ( (currentCursorAngle >= angleStart && stickAngle < angleEnd) )
						{
							currentOption = (mouseInCenterButton ? -1 : i); // disregard if mouse in center
						}
						angleStart += 2 * PI / numoptions;
						angleMiddle = angleStart + PI / numoptions;
						angleEnd = angleMiddle + PI / numoptions;
					}

					real_t angleMiddleForOption = PI / 2 + newOption * (2 * PI / numoptions);
					if ( mouseInCenterButton && newOption >= 0 )
					{
						omousex = centerx + (radius + thickness) / 2 * cos(angleMiddleForOption);
						omousey = centery + (radius + thickness) / 2 * sin(angleMiddleForOption);
					}
					else
					{
						switch ( newOption )
						{
							case UP: // up
							case UPLEFT:
							case UPRIGHT:
								if ( currentOption == DOWN || currentOption == DOWNLEFT || currentOption == DOWNRIGHT )
								{
									newOption = -1;
								}
								break;
							case LEFT:
								break;
							case DOWNLEFT:
							case DOWN:
							case DOWNRIGHT:
								if ( currentOption == UP || currentOption == UPLEFT || currentOption == UPRIGHT )
								{
									newOption = -1;
								}
								break;
							case RIGHT:
								break;
							default:
								break;
						}
						if ( newOption != -1 )
						{
							angleMiddleForOption = PI / 2 + newOption * (2 * PI / numoptions);
							omousex = centerx + (radius + thickness) / 2 * cos(angleMiddleForOption);
							omousey = centery + (radius + thickness) / 2 * sin(angleMiddleForOption);
						}
						else
						{
							omousex = centerx;
							omousey = centery;
						}
					}
				}
				else
				{
					//omousex = centerx;
					//omousey = centery;
				}
				inputs.setMouse(gui_player, Inputs::OX, omousex);
				inputs.setMouse(gui_player, Inputs::OY, omousey);
				inputs.setMouse(gui_player, Inputs::X, omousex);
				inputs.setMouse(gui_player, Inputs::Y, omousey);
				*/
			}
		}

		angleStart = PI / 2 - (PI / numoptions);
		angleMiddle = angleStart + PI / numoptions;
		angleEnd = angleMiddle + PI / numoptions;

		drawImageRing(fancyWindow_bmp, &src, radius, thickness, 40, 0, PI * 2, 156);

		for ( i = 0; i < numoptions; ++i )
		{
			// draw borders around ring.
			drawLine(centerx + (radius - thickness) * cos(angleStart), centery - (radius - thickness) * sin(angleStart),
				centerx + (radius + thickness) * cos(angleStart), centery - (radius + thickness) * sin(angleStart), uint32ColorGray, 192);
			drawLine(centerx + (radius - thickness) * cos(angleEnd), centery - (radius - thickness) * sin(angleEnd),
				centerx + (radius + thickness - 1) * cos(angleEnd), centery - (radius + thickness - 1) * sin(angleEnd), uint32ColorGray, 192);

			drawArcInvertedY(centerx, centery, radius - thickness, std::round((angleStart * 180) / PI), ((angleEnd * 180) / PI), uint32ColorGray, 192);
			drawArcInvertedY(centerx, centery, (radius + thickness), std::round((angleStart * 180) / PI), ((angleEnd * 180) / PI) + 1, uint32ColorGray, 192);

			angleStart += 2 * PI / numoptions;
			angleMiddle = angleStart + PI / numoptions;
			angleEnd = angleMiddle + PI / numoptions;
		}

		angleStart = PI / 2 - (PI / numoptions);
		angleMiddle = angleStart + PI / numoptions;
		angleEnd = angleMiddle + PI / numoptions;

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
						Uint32 borderColor = uint32ColorBaronyBlue;
						if ( optionDisabledForCreature(skillLVL, followerStats->type, i) != 0 )
						{
							borderColor = uint32ColorOrange;
						}
						drawLine(centerx + (radius - thickness) * cos(angleStart), centery - (radius - thickness) * sin(angleStart),
							centerx + (radius + thickness) * cos(angleStart), centery - (radius + thickness) * sin(angleStart), borderColor, 192);
						drawLine(centerx + (radius - thickness) * cos(angleEnd), centery - (radius - thickness) * sin(angleEnd),
							centerx + (radius + thickness - 1) * cos(angleEnd), centery - (radius + thickness - 1) * sin(angleEnd), borderColor, 192);

						drawArcInvertedY(centerx, centery, radius - thickness, std::round((angleStart * 180) / PI), ((angleEnd * 180) / PI), borderColor, 192);
						drawArcInvertedY(centerx, centery, (radius + thickness), std::round((angleStart * 180) / PI), ((angleEnd * 180) / PI) + 1, borderColor, 192);
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
				if ( followerStats->type == SENTRYBOT || followerStats->type == SPELLBOT )
				{
					getSizeOfText(ttf12, language[3675], &width, nullptr);
					ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, language[3675]);
				}
				else
				{
					getSizeOfText(ttf12, language[3037 + i + 8], &width, nullptr);
					ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, language[3037 + i + 8]);
				}
			}
			else
			{
				getSizeOfText(ttf12, language[3037 + i], &width, nullptr);
				if ( i == ALLY_CMD_DEFEND 
					&& followerToCommand->monsterAllyState == ALLY_STATE_DEFAULT
					&& (followerStats->type == SENTRYBOT || followerStats->type == SPELLBOT) )
				{
					getSizeOfText(ttf12, language[3674], &width, nullptr);
					ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, language[3674]);
				}
				else if ( i == ALLY_CMD_CLASS_TOGGLE )
				{
					if ( followerStats->type == GYROBOT )
					{
						// draw higher.
						getSizeOfText(ttf12, language[3619], &width, nullptr);
						ttfPrintText(ttf12, txt.x - width / 2, txt.y - 12, language[3619]);
						getSizeOfText(ttf12, language[3620 + followerToCommand->monsterAllyClass], &width, nullptr);
						ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, language[3620 + followerToCommand->monsterAllyClass]);
					}
					else if ( followerToCommand && followerToCommand->monsterAllySummonRank != 0 )
					{
						getSizeOfText(ttf12, "Relinquish ", &width, nullptr);
						ttfPrintText(ttf12, txt.x - width / 2, txt.y - 12, language[3196]);
					}
					else
					{
						// draw higher.
						ttfPrintText(ttf12, txt.x - width / 2, txt.y - 12, language[3037 + i]);
						getSizeOfText(ttf12, language[3053 + followerToCommand->monsterAllyClass], &width, nullptr);
						ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, language[3053 + followerToCommand->monsterAllyClass]);
					}
				}
				else if ( i == ALLY_CMD_PICKUP_TOGGLE )
				{
					if ( followerStats->type == GYROBOT )
					{
						if ( followerToCommand->monsterAllyPickupItems == ALLY_GYRO_DETECT_ITEMS_METAL
							|| followerToCommand->monsterAllyPickupItems == ALLY_GYRO_DETECT_ITEMS_MAGIC
							|| followerToCommand->monsterAllyPickupItems == ALLY_GYRO_DETECT_ITEMS_VALUABLE )
						{
							getSizeOfText(ttf12, "Detect", &width, nullptr);
							ttfPrintText(ttf12, txt.x - width / 2, txt.y - 24, language[3636]);
							getSizeOfText(ttf12, language[3624 + followerToCommand->monsterAllyPickupItems], &width, nullptr);
							ttfPrintText(ttf12, txt.x - width / 2, txt.y + 12, language[3624 + followerToCommand->monsterAllyPickupItems]);
						}
						else
						{
							getSizeOfText(ttf12, language[3623], &width, nullptr);
							ttfPrintText(ttf12, txt.x - width / 2, txt.y - 12, language[3623]);
							getSizeOfText(ttf12, language[3624 + followerToCommand->monsterAllyPickupItems], &width, nullptr);
							ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, language[3624 + followerToCommand->monsterAllyPickupItems]);
						}
					}
					else
					{
						// draw higher.
						getSizeOfText(ttf12, "Pickup", &width, nullptr);
						ttfPrintText(ttf12, txt.x - width / 2, txt.y - 24, language[3037 + i]);
						getSizeOfText(ttf12, language[3056 + followerToCommand->monsterAllyPickupItems], &width, nullptr);
						ttfPrintText(ttf12, txt.x - width / 2, txt.y + 12, language[3056 + followerToCommand->monsterAllyPickupItems]);
					}
				}
				else if ( i == ALLY_CMD_DROP_EQUIP )
				{
					if ( followerStats->type == GYROBOT )
					{
						getSizeOfText(ttf12, language[3633], &width, nullptr);
						ttfPrintText(ttf12, txt.x - width / 2, txt.y - 12, language[3633]);
						getSizeOfText(ttf12, language[3634], &width, nullptr);
						ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, language[3634]);
					}
					else
					{
						ttfPrintText(ttf12, txt.x - width / 2, txt.y - 12, language[3037 + i]);
						if ( skillLVL >= SKILL_LEVEL_LEGENDARY )
						{
							getSizeOfText(ttf12, language[3061], &width, nullptr);
							ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, language[3061]);
						}
						else if ( skillLVL >= SKILL_LEVEL_MASTER )
						{
							getSizeOfText(ttf12, language[3060], &width, nullptr);
							ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, language[3060]);
						}
						else
						{
							getSizeOfText(ttf12, language[3059], &width, nullptr);
							ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, language[3059]);
						}
					}
				}
				else if ( i == ALLY_CMD_SPECIAL )
				{
					if ( followerStats->type == GYROBOT )
					{
						getSizeOfText(ttf12, "Return &", &width, nullptr);
						ttfPrintText(ttf12, txt.x - width / 2, txt.y - 12, language[3635]);
					}
					else if ( followerStats->type == DUMMYBOT )
					{
						getSizeOfText(ttf12, language[3641], &width, nullptr);
						ttfPrintText(ttf12, txt.x - width / 2, txt.y - 12, language[3641]);
						getSizeOfText(ttf12, language[3642], &width, nullptr);
						ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, language[3642]);
					}
					else if ( followerStats->type == SENTRYBOT || followerStats->type == SPELLBOT )
					{
						getSizeOfText(ttf12, language[3649], &width, nullptr);
						ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, language[3649]);
					}
					else
					{
						getSizeOfText(ttf12, language[3037 + i], &width, nullptr);
						ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, language[3037 + i]);
					}
				}
				else if ( i == ALLY_CMD_ATTACK_SELECT )
				{
					if ( !attackCommandOnly(followerStats->type) )
					{
						if ( optionDisabledForCreature(skillLVL, followerStats->type, ALLY_CMD_ATTACK_CONFIRM) == 0 )
						{
							getSizeOfText(ttf12, "Interact / ", &width, nullptr);
							ttfPrintText(ttf12, txt.x - width / 2, txt.y - 12, language[3051]);
						}
						else
						{
							getSizeOfText(ttf12, language[3037 + i], &width, nullptr);
							ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, language[3037 + i]);
						}
					}
					else
					{
						getSizeOfText(ttf12, language[3104], &width, nullptr); // print just attack if no world interaction.
						ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, language[3104]);
					}
				}
				else if ( i == ALLY_CMD_MOVETO_SELECT )
				{
					if ( followerStats->type == SENTRYBOT || followerStats->type == SPELLBOT )
					{
						getSizeOfText(ttf12, language[3650], &width, nullptr);
						ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, language[3650]);
					}
					else
					{
						getSizeOfText(ttf12, language[3037 + i], &width, nullptr);
						ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, language[3037 + i]);
					}
				}
				else
				{
					getSizeOfText(ttf12, language[3037 + i], &width, nullptr);
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
			drawCircle(centerx, centery, radius - thickness, uint32ColorBaronyBlue, 192);
			//getSizeOfText(ttf12, language[3063], &width, nullptr);
			//ttfPrintText(ttf12, centerx - width / 2, centery - 8, language[3063]);
		}

		if ( optionSelected == -1 && disableOption == 0 && highlight != -1 )
		{
			// in case optionSelected is cleared, but we're still highlighting text (happens on next frame when clicking on disabled option.)
			if ( highlight == ALLY_CMD_ATTACK_SELECT )
			{
				if ( attackCommandOnly(followerStats->type) )
				{
					// attack only.
					disableOption = optionDisabledForCreature(skillLVL, followerStats->type, ALLY_CMD_ATTACK_CONFIRM);
				}
				else
				{
					disableOption = optionDisabledForCreature(skillLVL, followerStats->type, highlight);
				}
			}
			else
			{
				disableOption = optionDisabledForCreature(skillLVL, followerStats->type, highlight);
			}
		}

		if ( disableOption != 0 )
		{
			SDL_Rect tooltip;
			tooltip.x = omousex + 16;
			tooltip.y = omousey + 16;
			char* lowSkillLVLTooltip = language[3062];
			if ( tinkeringFollower )
			{
				lowSkillLVLTooltip = language[3672];
			}
			tooltip.w = longestline(lowSkillLVLTooltip) * TTF12_WIDTH + 8;
			tooltip.h = TTF12_HEIGHT * 2 + 8;

			if ( disableOption == -2 ) // disabled due to cooldown
			{
				tooltip.h = TTF12_HEIGHT + 8;
				tooltip.w = longestline(language[3092]) * TTF12_WIDTH + 8;
				drawTooltip(&tooltip);
				ttfPrintTextFormattedColor(ttf12, tooltip.x + 4, tooltip.y + 6, uint32ColorOrange, language[3092]);
			}
			else if ( disableOption == -1 ) // disabled due to creature type
			{
				tooltip.h = TTF12_HEIGHT + 8;
				tooltip.w = longestline(language[3103]) * TTF12_WIDTH + 8;
				tooltip.w += strlen(getMonsterLocalizedName(followerStats->type).c_str()) * TTF12_WIDTH;
				drawTooltip(&tooltip);
				ttfPrintTextFormattedColor(ttf12, tooltip.x + 4, tooltip.y + 6,
					uint32ColorOrange, language[3103], getMonsterLocalizedName(followerStats->type).c_str());
			}
			else if ( disableOption == -3 ) // disabled due to tinkerbot quality
			{
				tooltip.h = TTF12_HEIGHT + 8;
				tooltip.w = longestline(language[3673]) * TTF12_WIDTH + 8;
				drawTooltip(&tooltip);
				tooltip.w += strlen(getMonsterLocalizedName(followerStats->type).c_str()) * TTF12_WIDTH;
				drawTooltip(&tooltip);
				ttfPrintTextFormattedColor(ttf12, tooltip.x + 4, tooltip.y + 6,
					uint32ColorOrange, language[3673], getMonsterLocalizedName(followerStats->type).c_str());
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
					uint32ColorOrange, lowSkillLVLTooltip, requirement.c_str(), current.c_str());
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
	if ( !stats[gui_player] )
	{
		return;
	}

	int numFollowers = list_Size(&stats[gui_player]->FOLLOWERS);

	if ( numFollowers <= 0 )
	{
		return;
	}

	if ( !recentEntity ) // set first follower to be the selected one.
	{
		node_t* node = stats[gui_player]->FOLLOWERS.first;
		if ( node )
		{
			Entity* follower = uidToEntity(*((Uint32*)node->element));
			if ( follower )
			{
				recentEntity = follower;
				sidebarScrollIndex = 0;
				return;
			}
		}
	}
	else if ( numFollowers == 1 )
	{
		// only 1 follower, no work to do.
		sidebarScrollIndex = 0;
		return;
	}

	int monstersToDraw = numMonstersToDrawInParty();

	node_t* node2 = nullptr;
	int i = 0;
	for ( node_t* node = stats[gui_player]->FOLLOWERS.first; node != nullptr; node = node->next, ++i)
	{
		Entity* follower = nullptr;
		if ( (Uint32*)node->element )
		{
			follower = uidToEntity(*((Uint32*)node->element));
		}
		if ( follower && follower == recentEntity )
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
				node2 = stats[gui_player]->FOLLOWERS.first; // loop around to first index.
				follower = nullptr;
				if ( (Uint32*)node2->element )
				{
					follower = uidToEntity(*((Uint32*)node2->element));
				}
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
	if ( !stats[gui_player] )
	{
		return;
	}

	int numFollowers = list_Size(&stats[gui_player]->FOLLOWERS);

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
		sidebarScrollIndex = 0;
		return;
	}

	int i = 0;

	for ( node_t* node = stats[gui_player]->FOLLOWERS.first; node != nullptr; node = node->next, ++i )
	{
		Entity* follower = nullptr;
		if ( (Uint32*)node->element )
		{
			follower = uidToEntity(*((Uint32*)node->element));
		}
		if ( follower && follower == recentEntity )
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

bool FollowerRadialMenu::isTinkeringFollower(int type)
{
	if ( type == GYROBOT || type == SENTRYBOT
		|| type == SPELLBOT || type == DUMMYBOT )
	{
		return true;
	}
	return false;
}

bool FollowerRadialMenu::allowedInteractEntity(Entity& selectedEntity, bool updateInteractText)
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
	if ( !players[gui_player] || !players[gui_player]->entity )
	{
		return false;
	}

	bool interactItems = allowedInteractItems(followerStats->type) || allowedInteractFood(followerStats->type);
	bool interactWorld = allowedInteractWorld(followerStats->type);
	bool tinkeringFollower = isTinkeringFollower(followerStats->type);
	int skillLVL = stats[gui_player]->PROFICIENCIES[PRO_LEADERSHIP] + statGetCHR(stats[gui_player], players[gui_player]->entity);
	if ( tinkeringFollower )
	{
		skillLVL = stats[gui_player]->PROFICIENCIES[PRO_LOCKPICKING] + statGetPER(stats[gui_player], players[gui_player]->entity);
	}
	if ( followerToCommand->monsterAllySummonRank != 0 )
	{
		skillLVL = SKILL_LEVEL_LEGENDARY;
	}

	bool enableAttack = (optionDisabledForCreature(skillLVL, followerStats->type, ALLY_CMD_ATTACK_CONFIRM) == 0);
	
	if ( !interactItems && !interactWorld && enableAttack )
	{
		if ( updateInteractText )
		{
			strcpy(interactText, language[4043]); // "Attack "
		}
	}
	else
	{
		if ( updateInteractText )
		{
			strcpy(interactText, language[4014]); // "Interact with "
		}
	}
	if ( selectedEntity.behavior == &actTorch && interactWorld )
	{
		if ( updateInteractText )
		{
			strcat(interactText, items[TOOL_TORCH].name_identified);
		}
	}
	else if ( (selectedEntity.behavior == &actSwitch || selectedEntity.sprite == 184) && interactWorld )
	{
		if ( updateInteractText )
		{
			strcat(interactText, language[4044]); // "switch"
		}
	}
	else if ( selectedEntity.behavior == &actBomb && interactWorld && followerStats->type == GYROBOT )
	{
		if ( updateInteractText )
		{
			strcpy(interactText, language[3093]);
			strcat(interactText, language[4045]); // "trap"
		}
	}
	else if ( selectedEntity.behavior == &actItem && interactItems )
	{
		if ( updateInteractText )
		{
			if ( multiplayer != CLIENT )
			{
				if ( selectedEntity.skill[15] == 0 )
				{
					strcat(interactText, items[selectedEntity.skill[10]].name_unidentified);
				}
				else
				{
					strcat(interactText, items[selectedEntity.skill[10]].name_identified);
				}
			}
			else
			{
				strcat(interactText, language[4046]); // "item"
			}
		}
	}
	else if ( selectedEntity.behavior == &actMonster && enableAttack )
	{
		if ( updateInteractText )
		{
			strcpy(interactText, language[4043]); // "Attack "
			int monsterType = selectedEntity.getMonsterTypeFromSprite();
			strcat(interactText, getMonsterLocalizedName((Monster)monsterType).c_str());
		}
	}
	else
	{
		if ( updateInteractText )
		{
			strcpy(interactText, language[4047]); // "No interactions available"
		}
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
		case GYROBOT:
		case SENTRYBOT:
		case SPELLBOT:
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
		default:
			break;
	}

	Stat* followerStats = nullptr;
	if ( followerToCommand )
	{
		followerStats = followerToCommand->getStats();
	}

	int requirement = AllyNPCSkillRequirements[option];

	if ( monsterType == GYROBOT )
	{
		monsterGyroBotConvertCommand(&option);
		if ( followerStats )
		{
			if ( option == ALLY_CMD_GYRO_DETECT_TOGGLE )
			{
				if ( playerSkillLVL < SKILL_LEVEL_BASIC )
				{
					return SKILL_LEVEL_BASIC;
				}
				else if ( followerStats->LVL < 5 )
				{
					return -3;
				}
				else
				{
					return 0;
				}
			}
			else if ( option == ALLY_CMD_GYRO_LIGHT_TOGGLE )
			{
				if ( playerSkillLVL < SKILL_LEVEL_BASIC )
				{
					return SKILL_LEVEL_BASIC;
				}
				if ( followerStats->LVL < 5 )
				{
					return -3;
				}
			}
			else if ( option == ALLY_CMD_MOVETO_CONFIRM || option == ALLY_CMD_MOVETO_SELECT )
			{
				if ( playerSkillLVL < SKILL_LEVEL_BASIC )
				{
					return SKILL_LEVEL_BASIC;
				}
				if ( followerStats->LVL < 5 )
				{
					return -3;
				}
			}
			else if ( option == ALLY_CMD_ATTACK_SELECT || option == ALLY_CMD_ATTACK_CONFIRM 
				|| option == ALLY_CMD_DROP_EQUIP )
			{
				if ( playerSkillLVL < SKILL_LEVEL_SKILLED )
				{
					return SKILL_LEVEL_SKILLED;
				}
				if ( followerStats->LVL < 10 )
				{
					return -3;
				}
			}
		}

	}
	else
	{
		if ( monsterGyroBotOnlyCommand(option) )
		{
			return -1; // disabled due to monster.
		}
	}

	if ( monsterType == DUMMYBOT )
	{
		if ( option != ALLY_CMD_SPECIAL && option != ALLY_CMD_DUMMYBOT_RETURN )
		{
			return -1; // disabled due to monster.
		}
		else
		{
			option = ALLY_CMD_DUMMYBOT_RETURN;
		}
	}
	else if ( monsterType == SENTRYBOT || monsterType == SPELLBOT )
	{
		if ( option != ALLY_CMD_SPECIAL && option != ALLY_CMD_DUMMYBOT_RETURN )
		{
			if ( option != ALLY_CMD_MOVETO_CONFIRM && option != ALLY_CMD_MOVETO_SELECT
				&&  option != ALLY_CMD_ATTACK_SELECT && option != ALLY_CMD_ATTACK_CONFIRM
				&&  option != ALLY_CMD_CANCEL && option != ALLY_CMD_DEFEND && option != ALLY_CMD_FOLLOW )
			{
				return -1; // disabled due to monster.
			}
			else
			{
				if ( followerStats && (option != ALLY_CMD_CANCEL && option != ALLY_CMD_DEFEND && option != ALLY_CMD_FOLLOW ) )
				{
					if ( followerStats->LVL < 5 )
					{
						return -3; // disable all due to LVL
					}
					else if ( followerStats->LVL < 10 )
					{
						if ( option == ALLY_CMD_ATTACK_SELECT || option == ALLY_CMD_ATTACK_CONFIRM )
						{
							return -3; // disabled attack commands due to LVL
						}
					}
				}
			}
		}
		else
		{
			option = ALLY_CMD_DUMMYBOT_RETURN;
		}
	}

	if ( option == ALLY_CMD_SPECIAL
		&& followerToCommand->monsterAllySpecialCooldown != 0 )
	{
		return -2; // disabled due to cooldown.
	}

	switch ( option )
	{
		case ALLY_CMD_MOVEASIDE:
			if ( monsterType == SENTRYBOT || monsterType == SPELLBOT )
			{
				return -1; // can't use.
			}
		case ALLY_CMD_CANCEL:
			if ( playerSkillLVL < requirement )
			{
				return requirement; // disabled due to basic skill requirements.
			}
			return 0; // all permitted.
			break;

		case ALLY_CMD_FOLLOW:
		case ALLY_CMD_DEFEND:
			if ( monsterType == SENTRYBOT || monsterType == SPELLBOT )
			{
				return 0;
			}
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
			if ( followerToCommand && followerToCommand->monsterAllySummonRank != 0 )
			{
				return -1;
			}
			if ( !allowedInteractItems(monsterType) )
			{
				return -1; // disabled due to creature.
			}
			else if ( monsterType == GYROBOT )
			{
				requirement = SKILL_LEVEL_SKILLED;
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
				if ( monsterType == GYROBOT )
				{
					if ( playerSkillLVL < SKILL_LEVEL_SKILLED )
					{
						return SKILL_LEVEL_SKILLED;
					}
				}
				if ( playerSkillLVL < AllyNPCSkillRequirements[ALLY_CMD_ATTACK_SELECT] )
				{
					return AllyNPCSkillRequirements[ALLY_CMD_ATTACK_SELECT];
				}
				return 0;
			}
			break;

		case ALLY_CMD_ATTACK_CONFIRM:
			if ( monsterType == GYROBOT )
			{
				return -1; // disabled due to creature.
				break;
			}
			else if ( creatureTier == 3 && playerSkillLVL < SKILL_LEVEL_LEGENDARY )
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
			if ( followerToCommand && followerToCommand->monsterAllySummonRank != 0 )
			{
				return 0;
			}
			if ( monsterType == GYROBOT )
			{
				return 0;
			}
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
			if ( monsterType == GYROBOT )
			{
				return 0;
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
		case ALLY_CMD_DUMMYBOT_RETURN:
			if ( monsterType != DUMMYBOT && monsterType != SENTRYBOT && monsterType != SPELLBOT )
			{
				return -1;
			}
			else
			{
				return 0;
			}
			break;
		case ALLY_CMD_GYRO_DETECT_TOGGLE:
			if ( playerSkillLVL < requirement )
			{
				return requirement; // disabled due to basic skill requirements.
			}
			break;
		case ALLY_CMD_GYRO_LIGHT_TOGGLE:
			if ( playerSkillLVL < requirement )
			{
				return requirement; // disabled due to basic skill requirements.
			}
			break;
		case ALLY_CMD_GYRO_RETURN:
			if ( monsterType == GYROBOT )
			{
				return 0;
			}
			else
			{
				return -1;
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
		case SKELETON:
		case GYROBOT:
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
		case GYROBOT:
			if ( followerToCommand && followerToCommand->monsterAllySummonRank != 0 )
			{
				return false;
			}
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

void FollowerRadialMenu::monsterGyroBotConvertCommand(int* option)
{
	if ( !option )
	{
		return;
	}
	switch ( *option )
	{
		case ALLY_CMD_PICKUP_TOGGLE:
			*option = ALLY_CMD_GYRO_DETECT_TOGGLE;
			break;
		case ALLY_CMD_SPECIAL:
		case ALLY_CMD_RETURN_SOUL:
			*option = ALLY_CMD_GYRO_RETURN;
			break;
		case ALLY_CMD_CLASS_TOGGLE:
			*option = ALLY_CMD_GYRO_LIGHT_TOGGLE;
			break;
		default:
			break;
	}
}


bool FollowerRadialMenu::monsterGyroBotOnlyCommand(int option)
{
	switch ( option )
	{
		case ALLY_CMD_GYRO_DEPLOY:
		case ALLY_CMD_GYRO_PATROL:
		case ALLY_CMD_GYRO_LIGHT_TOGGLE:
		case ALLY_CMD_GYRO_RETURN:
		case ALLY_CMD_GYRO_DETECT_TOGGLE:
			return true;
			break;
		default:
			break;
	}
	return false;
}

bool FollowerRadialMenu::monsterGyroBotDisallowedCommands(int option)
{
	switch ( option )
	{
		case ALLY_CMD_CLASS_TOGGLE:
		case ALLY_CMD_PICKUP_TOGGLE:
			return true;
			break;
		default:
			break;
	}
	return false;
}

bool GenericGUIMenu::isItemRemoveCursable(const Item* item)
{
	if ( !item )
	{
		return false;
	}
	if ( !item->identified )
	{
		return false;
	}
	if ( item->identified && item->beatitude < 0 )
	{
		return true;
	}
	return false;
}

bool GenericGUIMenu::isItemIdentifiable(const Item* item)
{
	if ( !item )
	{
		return false;
	}
	if ( item->identified )
	{
		return false;
	}
	return true;
}

bool GenericGUIMenu::isItemRepairable(const Item* item, int repairScroll)
{
	if ( !item )
	{
		return false;
	}
	if ( !item->identified )
	{
		return false;
	}
	Category cat = itemCategory(item);
	if ( repairScroll == SCROLL_CHARGING )
	{
		if ( item->type == ENCHANTED_FEATHER )
		{
			if ( item->appearance % ENCHANTED_FEATHER_MAX_DURABILITY < 100 )
			{
				return true;
			}
			return false;
		}
		else if ( cat == MAGICSTAFF )
		{
			if ( item->status == EXCELLENT )
			{
				return false;
			}
			return true;
		}

		return false;
	}
	else if ( repairScroll == SCROLL_REPAIR )
	{
		if ( item->status == EXCELLENT )
		{
			return false;
		}
	}
	switch ( cat )
	{
		case WEAPON:
			return true;
		case ARMOR:
			return true;
		case MAGICSTAFF:
			return false;
		case THROWN:
			if ( item->type == BOOMERANG )
			{
				return true;
			}
			return false;
		case TOOL:
			switch ( item->type )
			{
				case TOOL_TOWEL:
				case TOOL_MIRROR:
				case TOOL_SKELETONKEY:
				case TOOL_TINOPENER:
				case TOOL_METAL_SCRAP:
				case TOOL_MAGIC_SCRAP:
				case TOOL_TINKERING_KIT:
				case TOOL_SENTRYBOT:
				case TOOL_DETONATOR_CHARGE:
				case TOOL_BOMB:
				case TOOL_SLEEP_BOMB:
				case TOOL_FREEZE_BOMB:
				case TOOL_TELEPORT_BOMB:
				case TOOL_GYROBOT:
				case TOOL_SPELLBOT:
				case TOOL_DECOY:
				case TOOL_DUMMYBOT:
				case ENCHANTED_FEATHER:
					return false;
					break;
				default:
					if ( itemTypeIsQuiver(item->type) )
					{
						return false;
					}
					return true;
					break;
			}
			break;
		default:
			return false;
	}
}

// Generic GUI Code
void GenericGUIMenu::rebuildGUIInventory()
{
	list_t* player_inventory = &stats[gui_player]->inventory;
	node_t* node = nullptr;
	Item* item = nullptr;
	int c = 0;

	if ( guiType == GUI_TYPE_TINKERING )
	{
		player_inventory = &tinkeringTotalItems;
		tinkeringTotalLastCraftableNode = tinkeringTotalItems.last;
		if ( tinkeringTotalLastCraftableNode )
		{
			tinkeringTotalLastCraftableNode->next = stats[gui_player]->inventory.first;
		}
	}
	else if ( guiType == GUI_TYPE_SCRIBING )
	{
		player_inventory = &scribingTotalItems;
		scribingTotalLastCraftableNode = scribingTotalItems.last;
		if ( scribingTotalLastCraftableNode )
		{
			scribingTotalLastCraftableNode->next = stats[gui_player]->inventory.first;
		}
	}

	if ( player_inventory )
	{
		//Count the number of items in the GUI "inventory".
		for ( node = player_inventory->first; node != nullptr; node = node->next )
		{
			item = (Item*)node->element;
			if ( item )
			{
				if ( shouldDisplayItemInGUI(item) )
				{
					++c;
				}
				if ( guiType == GUI_TYPE_TINKERING )
				{
					if ( item->node && item->node->list == &stats[gui_player]->inventory )
					{
						if ( item->type == TOOL_METAL_SCRAP )
						{
							tinkeringMetalScrap.insert(item->uid);
						}
						else if ( item->type == TOOL_MAGIC_SCRAP )
						{
							tinkeringMagicScrap.insert(item->uid);
						}
					}
				}
			}
		}
		if ( c == 0 && guiType == GUI_TYPE_ALCHEMY )
		{
			// did not find mixable item... close GUI
			//if ( !experimentingAlchemy )
			//{
			//	messagePlayer(gui_player, MESSAGE_MISC, language[3338]);
			//}
			//else
			//{
			//	messagePlayer(gui_player, MESSAGE_MISC | MESSAGE_INVENTORY, language[3343]);
			//}
			//closeGUI();
			//return;
		}
		scroll = std::max(0, std::min(scroll, c - kNumShownItems));
		for ( c = 0; c < kNumShownItems; ++c )
		{
			itemsDisplayed[c] = nullptr;
		}
		c = 0;

		//Assign the visible items to the GUI slots.
		for ( node = player_inventory->first; node != nullptr; node = node->next )
		{
			if ( node->element )
			{
				item = (Item*)node->element;
				if ( shouldDisplayItemInGUI(item) ) //Skip over all non-applicable items.
				{
					++c;
					if ( c <= scroll )
					{
						continue;
					}
					itemsDisplayed[c - scroll - 1] = item;
					if ( c > 3 + scroll )
					{
						break;
					}
				}
			}
		}

		if ( guiType == GUI_TYPE_TINKERING && tinkeringFilter == TinkeringFilter::TINKER_FILTER_CRAFTABLE )
		{
			for ( node = player_inventory->first; node != nullptr; node = node->next )
			{
				if ( node->element )
				{
					item = (Item*)node->element;
					if ( isNodeTinkeringCraftableItem(node)
						&& item->x >= 0 && item->x < TinkerGUI_t::MAX_TINKER_X
						&& item->y >= 0 && item->y < TinkerGUI_t::MAX_TINKER_Y )
					{
						if ( auto slotFrame = tinkerGUI.getTinkerSlotFrame(item->x, item->y) )
						{
							bool unusable = !tinkeringPlayerCanAffordCraft(item) || (tinkeringPlayerHasSkillLVLToCraft(item) == -1);
							updateSlotFrameFromItem(slotFrame, item, unusable);
						}
					}
				}
			}
		}
	}
}


void GenericGUIMenu::updateGUI()
{
	SDL_Rect pos;
	node_t* node;
	int y, c;

	const Sint32 mousex = inputs.getMouse(gui_player, Inputs::X);
	const Sint32 mousey = inputs.getMouse(gui_player, Inputs::Y);
	const Sint32 omousex = inputs.getMouse(gui_player, Inputs::OX);
	const Sint32 omousey = inputs.getMouse(gui_player, Inputs::OY);

	//Generic GUI.
	if ( guiActive )
	{
		auto& player = players[gui_player];
		if ( !player->isLocalPlayerAlive()
			|| stats[gui_player]->HP <= 0
			|| player->shootmode )
		{
			closeGUI();
		}
		if ( guiType == GUI_TYPE_ALCHEMY )
		{
			if ( !alembicItem )
			{
				closeGUI();
				return;
			}
			if ( !alembicItem->node )
			{
				closeGUI();
				return;
			}
			if ( alembicItem->node->list != &stats[gui_player]->inventory )
			{
				// dropped out of inventory or something.
				closeGUI();
				return;
			}
		}
		else if ( guiType == GUI_TYPE_TINKERING )
		{
			if ( !tinkeringKitItem )
			{
				closeGUI();
				return;
			}
			if ( !tinkeringKitItem->node )
			{
				closeGUI();
				return;
			}
			if ( tinkeringKitItem->node->list != &stats[gui_player]->inventory )
			{
				// dropped out of inventory or something.
				closeGUI();
				return;
			}
		}
		else if ( guiType == GUI_TYPE_SCRIBING )
		{
			if ( !scribingToolItem )
			{
				closeGUI();
				return;
			}
			if ( !scribingToolItem->node )
			{
				closeGUI();
				return;
			}
			if ( scribingToolItem->node->list != &stats[gui_player]->inventory )
			{
				// dropped out of inventory or something.
				closeGUI();
				return;
			}
			if ( scribingBlankScrollTarget && scribingFilter != SCRIBING_FILTER_CRAFTABLE )
			{
				scribingBlankScrollTarget = nullptr;
			}
			if ( scribingLastUsageDisplayTimer > 0 )
			{
				if ( ticks % 2 == 0 )
				{
					--scribingLastUsageDisplayTimer;
				}
			}
			else
			{
				scribingLastUsageDisplayTimer = 0;
				scribingLastUsageAmount = 0;
			}
		}

		gui_starty = (players[gui_player]->camera_midx() + (inventoryChest_bmp->w / 2)) + offsetx;
		gui_startx = (players[gui_player]->camera_midy() - (inventoryChest_bmp->h / 2)) + offsety;

		//Center the GUI.
		pos.x = gui_starty;
		pos.y = gui_startx;

		windowX1 = gui_starty;
		windowX2 = gui_starty + identifyGUI_img->w;

		windowY1 = gui_startx;
		windowY2 = gui_startx + identifyGUI_img->h;
		if ( guiType == GUI_TYPE_TINKERING )
		{
			//windowX1 -= 20;
			//windowX2 += 20;
			//windowY1 -= 40;
			//windowY2 += 40;
			//drawWindowFancy(windowX1, windowY1, windowX2, windowY2);
			//int numMetalScrap = 0;
			//int numMagicScrap = 0;
			//if ( !tinkeringMetalScrap.empty() )
			//{
			//	for ( auto uid : tinkeringMetalScrap )
			//	{
			//		if ( uidToItem(uid) )
			//		{
			//			numMetalScrap += (uidToItem(uid))->count;
			//		}
			//	}
			//}
			//if ( !tinkeringMagicScrap.empty() )
			//{
			//	for ( auto uid : tinkeringMagicScrap )
			//	{
			//		if ( uidToItem(uid) )
			//		{
			//			numMagicScrap += (uidToItem(uid))->count;
			//		}
			//	}
			//}

			//// title
			//ttfPrintTextFormatted(ttf12, windowX1 + 16, windowY1 + 8,
			//	language[3690]);
			//char kitStatusText[64] = "";
			//if ( tinkeringKitItem )
			//{
			//	snprintf(kitStatusText, 63, language[3691], language[3691 + std::max(1, static_cast<int>(tinkeringKitItem->status))]);
			//}
			//ttfPrintTextFormatted(ttf12, windowX2 - 16 - (strlen(kitStatusText) + 1) * TTF12_WIDTH, windowY2 - TTF12_HEIGHT - 8,
			//	kitStatusText);

			//ttfPrintTextFormatted(ttf12, windowX1 + 16, windowY2 - TTF12_HEIGHT - 8,
			//	language[3647], numMetalScrap, numMagicScrap);
			//SDL_Rect smallIcon;
			//smallIcon.x = windowX1 + 16 + (strlen(language[3647]) - 5) * TTF12_WIDTH;
			//smallIcon.y = windowY2 - TTF12_HEIGHT - 12;
			//smallIcon.h = 16;
			//smallIcon.w = 16;
			//node_t* imageNode = items[TOOL_METAL_SCRAP].surfaces.first;
			//if ( imageNode )
			//{
			//	drawImageScaled(*((SDL_Surface**)imageNode->element), NULL, &smallIcon);
			//}
			//smallIcon.x += TTF12_WIDTH * 6;
			//imageNode = items[TOOL_MAGIC_SCRAP].surfaces.first;
			//if ( imageNode )
			//{
			//	drawImageScaled(*((SDL_Surface**)imageNode->element), NULL, &smallIcon);
			//}

			//// draw filter labels.
			//int txtWidth = 0;
			//int txtHeight = 0;
			//int charWidth = 0;
			//TTF_Font* font = ttf8;
			//getSizeOfText(font, "a", &charWidth, nullptr); // get 1 character width.
			//int textstartx = pos.x + 2 * charWidth + 4;

			//SDL_Rect highlightBtn;
			//// Craft
			//getSizeOfText(ttf8, language[3644], &txtWidth, &txtHeight);
			//highlightBtn.x = textstartx;
			//highlightBtn.y = pos.y + (12 - txtHeight);
			//highlightBtn.w = txtWidth + 2 * charWidth + 4;
			//highlightBtn.h = txtHeight + 4;
			//if ( (inputs.bMouseLeft(gui_player) || inputs.bControllerInputPressed(gui_player, INJOY_MENU_USE))
			//	&& mouseInBounds(gui_player, highlightBtn.x, highlightBtn.x + highlightBtn.w, highlightBtn.y, highlightBtn.y + highlightBtn.h) )
			//{
			//	tinkeringFilter = TINKER_FILTER_CRAFTABLE;
			//	inputs.controllerClearInput(gui_player, INJOY_MENU_USE);
			//	inputs.mouseClearLeft(gui_player);
			//}
			//if ( tinkeringFilter == TINKER_FILTER_CRAFTABLE )
			//{
			//	drawImageScaled(button_bmp, NULL, &highlightBtn);
			//}
			//ttfPrintText(font, highlightBtn.x + 4 + charWidth, pos.y - (8 - txtHeight), language[3644]);

			//// Salvage
			//getSizeOfText(font, language[3645], &txtWidth, &txtHeight);
			//highlightBtn.x += highlightBtn.w;
			//highlightBtn.y = pos.y + (12 - txtHeight);
			//highlightBtn.w = txtWidth + 2 * charWidth + 4;
			//highlightBtn.h = txtHeight + 4;
			//if ( (inputs.bMouseLeft(gui_player) || inputs.bControllerInputPressed(gui_player, INJOY_MENU_USE))
			//	&& mouseInBounds(gui_player, highlightBtn.x, highlightBtn.x + highlightBtn.w, highlightBtn.y, highlightBtn.y + highlightBtn.h) )
			//{
			//	tinkeringFilter = TINKER_FILTER_SALVAGEABLE;
			//	inputs.controllerClearInput(gui_player, INJOY_MENU_USE);
			//	inputs.mouseClearLeft(gui_player);
			//}
			//if ( tinkeringFilter == TINKER_FILTER_SALVAGEABLE )
			//{
			//	drawImageScaled(button_bmp, NULL, &highlightBtn);
			//}
			//ttfPrintText(font, highlightBtn.x + 4 + charWidth, pos.y - (8 - txtHeight), language[3645]);

			//// Repair
			//getSizeOfText(font, language[3646], &txtWidth, &txtHeight);
			//highlightBtn.x += highlightBtn.w;
			//highlightBtn.y = pos.y + (12 - txtHeight);
			//highlightBtn.w = txtWidth + 2 * charWidth + 4;
			//highlightBtn.h = txtHeight + 4;
			//if ( (inputs.bMouseLeft(gui_player) || inputs.bControllerInputPressed(gui_player, INJOY_MENU_USE))
			//	&& mouseInBounds(gui_player, highlightBtn.x, highlightBtn.x + highlightBtn.w, highlightBtn.y, highlightBtn.y + highlightBtn.h) )
			//{
			//	tinkeringFilter = TINKER_FILTER_REPAIRABLE;
			//	inputs.controllerClearInput(gui_player, INJOY_MENU_USE);
			//	inputs.mouseClearLeft(gui_player);
			//}
			//if ( tinkeringFilter == TINKER_FILTER_REPAIRABLE )
			//{
			//	drawImageScaled(button_bmp, NULL, &highlightBtn);
			//}
			//ttfPrintText(font, highlightBtn.x + 4 + charWidth, pos.y - (8 - txtHeight), language[3646]);

			//// Filter include all (*)
			//getSizeOfText(font, language[356], &txtWidth, &txtHeight);
			//highlightBtn.x += highlightBtn.w;
			//highlightBtn.y = pos.y + (12 - txtHeight);
			//highlightBtn.w = 2 * charWidth + 4;
			//highlightBtn.h = txtHeight + 4;
			//if ( (inputs.bMouseLeft(gui_player) || inputs.bControllerInputPressed(gui_player, INJOY_MENU_USE))
			//	&& mouseInBounds(gui_player, highlightBtn.x, highlightBtn.x + highlightBtn.w, highlightBtn.y, highlightBtn.y + highlightBtn.h) )
			//{
			//	tinkeringFilter = TINKER_FILTER_ALL;
			//	inputs.controllerClearInput(gui_player, INJOY_MENU_USE);
			//	inputs.mouseClearLeft(gui_player);
			//}
			//if ( tinkeringFilter == TINKER_FILTER_ALL )
			//{
			//	drawImageScaled(smallbutton_bmp, NULL, &highlightBtn);
			//}
			//ttfPrintText(font, highlightBtn.x + (highlightBtn.w - txtWidth) / 2, pos.y - (8 - txtHeight), language[356]);
		}
		else if ( guiType == GUI_TYPE_SCRIBING )
		{
			windowX1 -= 20;
			windowX2 += 20;
			windowY1 -= 40;
			windowY2 += 40;
			drawWindowFancy(windowX1, windowY1, windowX2, windowY2);

			// title
			ttfPrintTextFormatted(ttf12, windowX1 + 16, windowY1 + 8,
				language[3716]);
			char toolStatusText[64] = "";
			if ( scribingToolItem && scribingToolItem->identified )
			{
				snprintf(toolStatusText, 63, language[3717], scribingToolItem->appearance % ENCHANTED_FEATHER_MAX_DURABILITY);
			}
			ttfPrintTextFormatted(ttf12, windowX2 - 16 - (strlen(toolStatusText) + 1) * TTF12_WIDTH, windowY2 - TTF12_HEIGHT - 8,
				toolStatusText);
			/*if ( scribingLastUsageDisplayTimer > 0 )
			{
				ttfPrintTextFormattedColor(ttf12, windowX2 - 16 - 11 * TTF12_WIDTH, windowY2 - TTF12_HEIGHT - 8, uint32ColorRed,
						"(%3d)", -scribingLastUsageAmount);
			}*/

			if ( scribingFilter == SCRIBING_FILTER_CRAFTABLE )
			{
				if ( scribingBlankScrollTarget )
				{
					snprintf(tempstr, 1024, language[3722], scribingBlankScrollTarget->beatitude, items[SCROLL_BLANK].name_identified);
					ttfPrintTextFormatted(ttf12, windowX1 + 16, windowY2 - 2 * TTF12_HEIGHT - 8, tempstr);

					SDL_Rect smallIcon;
					smallIcon.x = windowX1 + 16 + (longestline(tempstr) - 5) * TTF12_WIDTH;
					smallIcon.y = windowY2 - TTF12_HEIGHT - 12 - 4;
					smallIcon.h = 16;
					smallIcon.w = 16;
					node_t* imageNode = items[SCROLL_BLANK].surfaces.first;
					if ( imageNode )
					{
						drawImageScaled(*((SDL_Surface**)imageNode->element), NULL, &smallIcon);
					}
					smallIcon.x += smallIcon.w + 4;
					smallIcon.w = longestline(language[3723]) * TTF12_WIDTH + 8;
					smallIcon.y -= 2;
					smallIcon.h += 2;
					if ( mouseInBounds(gui_player, smallIcon.x, smallIcon.x + smallIcon.w, smallIcon.y, smallIcon.y + smallIcon.h) )
					{
						drawDepressed(smallIcon.x, smallIcon.y, smallIcon.x + smallIcon.w, smallIcon.y + smallIcon.h);
						if ( (inputs.bMouseLeft(gui_player) || inputs.bControllerInputPressed(gui_player, INJOY_MENU_USE)) )
						{
							inputs.controllerClearInput(gui_player, INJOY_MENU_USE);
							inputs.mouseClearLeft(gui_player);
							scribingBlankScrollTarget = nullptr;
						}
					}
					else
					{
						drawWindow(smallIcon.x, smallIcon.y, smallIcon.x + smallIcon.w, smallIcon.y + smallIcon.h);
					}
					ttfPrintTextFormatted(ttf12, smallIcon.x + 6, windowY2 - 2 * TTF12_HEIGHT + 2, language[3723]);
				}
				else
				{
					ttfPrintTextFormatted(ttf12, windowX1 + 16, windowY2 - 2 * TTF12_HEIGHT - 8,
						language[3720]);
				}
			}
			else if ( scribingFilter == SCRIBING_FILTER_REPAIRABLE )
			{
				ttfPrintTextFormatted(ttf12, windowX1 + 16, windowY2 - 2 * TTF12_HEIGHT - 8,
					language[3726]);
			}

			// draw filter labels.
			int txtWidth = 0;
			int txtHeight = 0;
			int charWidth = 0;
			TTF_Font* font = ttf8;
			getSizeOfText(font, "a", &charWidth, nullptr); // get 1 character width.
			int textstartx = pos.x + 2 * charWidth + 4;

			SDL_Rect highlightBtn;
			// Inscribe
			getSizeOfText(ttf8, language[3718], &txtWidth, &txtHeight);
			highlightBtn.x = textstartx;
			highlightBtn.y = pos.y + (12 - txtHeight);
			highlightBtn.w = txtWidth + 2 * charWidth + 4;
			highlightBtn.h = txtHeight + 4;
			if ( (inputs.bMouseLeft(gui_player) || inputs.bControllerInputPressed(gui_player, INJOY_MENU_USE))
				&& mouseInBounds(gui_player, highlightBtn.x, highlightBtn.x + highlightBtn.w, highlightBtn.y, highlightBtn.y + highlightBtn.h) )
			{
				scribingFilter = SCRIBING_FILTER_CRAFTABLE;
				inputs.controllerClearInput(gui_player, INJOY_MENU_USE);
				inputs.mouseClearLeft(gui_player);
			}
			if ( scribingFilter == SCRIBING_FILTER_CRAFTABLE )
			{
				drawImageScaled(button_bmp, NULL, &highlightBtn);
			}
			ttfPrintText(font, highlightBtn.x + 4 + charWidth, pos.y - (8 - txtHeight), language[3718]);

			// Repair
			getSizeOfText(font, language[3719], &txtWidth, &txtHeight);
			highlightBtn.x += highlightBtn.w;
			highlightBtn.y = pos.y + (12 - txtHeight);
			highlightBtn.w = txtWidth + 2 * charWidth + 4;
			highlightBtn.h = txtHeight + 4;
			if ( (inputs.bMouseLeft(gui_player) || inputs.bControllerInputPressed(gui_player, INJOY_MENU_USE))
				&& mouseInBounds(gui_player, highlightBtn.x, highlightBtn.x + highlightBtn.w, highlightBtn.y, highlightBtn.y + highlightBtn.h) )
			{
				scribingFilter = SCRIBING_FILTER_REPAIRABLE;
				inputs.controllerClearInput(gui_player, INJOY_MENU_USE);
				inputs.mouseClearLeft(gui_player);
			}
			if ( scribingFilter == SCRIBING_FILTER_REPAIRABLE )
			{
				drawImageScaled(button_bmp, NULL, &highlightBtn);
			}
			ttfPrintText(font, highlightBtn.x + 4 + charWidth, pos.y - (8 - txtHeight), language[3719]);
		}

		if ( guiType != GUI_TYPE_TINKERING && guiType != GUI_TYPE_ALCHEMY ) // gradually remove all this for all windows once upgraded
		{
			drawImage(identifyGUI_img, NULL, &pos);

			//Buttons
			if ( inputs.bMouseLeft(gui_player) )
			{
				//GUI scroll up button.
				if ( omousey >= gui_startx + 16 && omousey < gui_startx + 52 )
				{
					if ( omousex >= gui_starty + (identifyGUI_img->w - 28) && omousex < gui_starty + (identifyGUI_img->w - 12) )
					{
						buttonclick = 7;
						scroll--;
						inputs.mouseClearLeft(gui_player);
					}
				}
				//GUI scroll down button.
				else if ( omousey >= gui_startx + 52 && omousey < gui_startx + 88 )
				{
					if ( omousex >= gui_starty + (identifyGUI_img->w - 28) && omousex < gui_starty + (identifyGUI_img->w - 12) )
					{
						buttonclick = 8;
						scroll++;
						inputs.mouseClearLeft(gui_player);
					}
				}
				else if ( omousey >= gui_startx && omousey < gui_startx + 15 )
				{
					//GUI close button.
					if ( omousex >= gui_starty + 393 && omousex < gui_starty + 407 )
					{
						buttonclick = 9;
						inputs.mouseClearLeft(gui_player);
					}

					// 20/12/20 - disabling this for now. unnecessary
					if ( false )
					{
						if ( omousex >= gui_starty && omousex < gui_starty + 377 && omousey >= gui_startx && omousey < gui_startx + 15 )
						{
							gui_clickdrag[gui_player] = true;
							draggingGUI = true;
							dragoffset_x[gui_player] = omousex - gui_starty;
							dragoffset_y[gui_player] = omousey - gui_startx;
							inputs.mouseClearLeft(gui_player);
						}
					}
				}
			}

			// mousewheel
			if ( omousex >= gui_starty + 12 && omousex < gui_starty + (identifyGUI_img->w - 28) )
			{
				if ( omousey >= gui_startx + 16 && omousey < gui_startx + (identifyGUI_img->h - 8) )
				{
					if ( mousestatus[SDL_BUTTON_WHEELDOWN] )
					{
						mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
						scroll++;
					}
					else if ( mousestatus[SDL_BUTTON_WHEELUP] )
					{
						mousestatus[SDL_BUTTON_WHEELUP] = 0;
						scroll--;
					}
				}
			}

			if ( draggingGUI )
			{
				if ( gui_clickdrag[gui_player] )
				{
					offsetx = (omousex - dragoffset_x[gui_player]) - (gui_starty - offsetx);
					offsety = (omousey - dragoffset_y[gui_player]) - (gui_startx - offsety);
					if ( gui_starty <= 0 )
					{
						offsetx = 0 - (gui_starty - offsetx);
					}
					if ( gui_starty > 0 + xres - identifyGUI_img->w )
					{
						offsetx = (0 + xres - identifyGUI_img->w) - (gui_starty - offsetx);
					}
					if ( gui_startx <= 0 )
					{
						offsety = 0 - (gui_startx - offsety);
					}
					if ( gui_startx > 0 + players[gui_player]->camera_y2() - identifyGUI_img->h )
					{
						offsety = (0 + players[gui_player]->camera_y2() - identifyGUI_img->h) - (gui_startx - offsety);
					}
				}
				else
				{
					draggingGUI = false;
				}
			}
		}

		list_t* player_inventory = &stats[gui_player]->inventory;
		if ( guiType == GUI_TYPE_TINKERING )
		{
			player_inventory = &tinkeringTotalItems;
			tinkeringTotalLastCraftableNode = tinkeringTotalItems.last;
			if ( tinkeringTotalLastCraftableNode )
			{
				tinkeringTotalLastCraftableNode->next = stats[gui_player]->inventory.first;
			}
		}
		else if ( guiType == GUI_TYPE_SCRIBING )
		{
			player_inventory = &scribingTotalItems;
			scribingTotalLastCraftableNode = scribingTotalItems.last;
			if ( scribingTotalLastCraftableNode )
			{
				scribingTotalLastCraftableNode->next = stats[gui_player]->inventory.first;
			}
		}

		if ( !player_inventory )
		{
			messagePlayer(0, MESSAGE_DEBUG, "Warning: stats[%d].inventory is not a valid list. This should not happen.", gui_player);
		}
		else
		{
			//Print the window label signifying this GUI.
			char* window_name;
			if ( guiType == GUI_TYPE_REPAIR )
			{
				if ( repairItemType == SCROLL_REPAIR )
				{
					window_name = language[3286];
				}
				else if ( repairItemType == SCROLL_CHARGING )
				{
					window_name = language[3732];
				}
				ttfPrintText(ttf8, (gui_starty + 2 + ((identifyGUI_img->w / 2) - ((TTF8_WIDTH * longestline(window_name)) / 2))), gui_startx + 4, window_name);
			}
			else if ( guiType == GUI_TYPE_ALCHEMY )
			{
				/*if ( !basePotion )
				{
					if ( !experimentingAlchemy )
					{
						window_name = language[3328];
					}
					else
					{
						window_name = language[3344];
					}
					ttfPrintText(ttf8, (gui_starty + 2 + ((identifyGUI_img->w / 2) - ((TTF8_WIDTH * longestline(window_name)) / 2))), gui_startx + 4, window_name);
				}
				else
				{
					if ( !experimentingAlchemy )
					{
						window_name = language[3329];
					}
					else
					{
						window_name = language[3345];
					}
					ttfPrintText(ttf8, (gui_starty + 2 + ((identifyGUI_img->w / 2) - ((TTF8_WIDTH * longestline(window_name)) / 2))), 
						gui_startx + 4 - TTF8_HEIGHT - 4, window_name);
					int count = basePotion->count;
					basePotion->count = 1;
					char *description = basePotion->description();
					basePotion->count = count;
					ttfPrintText(ttf8, (gui_starty + 2 + ((identifyGUI_img->w / 2) - ((TTF8_WIDTH * longestline(description)) / 2))),
						gui_startx + 4, description);
				}*/
			}
			else if ( guiType == GUI_TYPE_REMOVECURSE )
			{
				window_name = language[346];
				ttfPrintText(ttf8, (gui_starty + 2 + ((identifyGUI_img->w / 2) - ((TTF8_WIDTH * longestline(window_name)) / 2))), gui_startx + 4, window_name);
			}
			else if ( guiType == GUI_TYPE_IDENTIFY )
			{
				window_name = language[318];
				ttfPrintText(ttf8, (gui_starty + 2 + ((identifyGUI_img->w / 2) - ((TTF8_WIDTH * longestline(window_name)) / 2))), gui_startx + 4, window_name);
			}

			if ( guiType != GUI_TYPE_TINKERING && guiType != GUI_TYPE_ALCHEMY )
			{
				//GUI up button.
				if ( buttonclick == 7 )
				{
					pos.x = gui_starty + (identifyGUI_img->w - 28);
					pos.y = gui_startx + 16;
					pos.w = 0;
					pos.h = 0;
					drawImage(invup_bmp, NULL, &pos);
				}
				//GUI down button.
				if ( buttonclick == 8 )
				{
					pos.x = gui_starty + (identifyGUI_img->w - 28);
					pos.y = gui_startx + 52;
					pos.w = 0;
					pos.h = 0;
					drawImage(invdown_bmp, NULL, &pos);
				}
				//GUI close button.
				if ( buttonclick == 9 )
				{
					pos.x = gui_starty + 393;
					pos.y = gui_startx;
					pos.w = 0;
					pos.h = 0;
					drawImage(invclose_bmp, NULL, &pos);
					closeGUI();
				}

				Item *item = nullptr;

				bool selectingSlot = false;
				SDL_Rect slotPos;
				slotPos.x = gui_starty + 12;
				slotPos.w = inventoryoptionChest_bmp->w;
				slotPos.y = gui_startx + 16;
				slotPos.h = inventoryoptionChest_bmp->h;
				bool mouseWithinBoundaryX = (mousex >= slotPos.x && mousex < slotPos.x + slotPos.w);

				for ( int i = 0; i < kNumShownItems; ++i, slotPos.y += slotPos.h )
				{
					pos.x = slotPos.x;
					pos.w = 0;
					pos.h = 0;


					if ( mouseWithinBoundaryX && omousey >= slotPos.y && omousey < slotPos.y + slotPos.h && itemsDisplayed[i] )
					{
						pos.y = slotPos.y;
						drawImage(inventoryoptionChest_bmp, nullptr, &pos);
						selectedSlot = i;
						selectingSlot = true;
						if ( (inputs.bMouseLeft(gui_player) || inputs.bControllerInputPressed(gui_player, INJOY_MENU_USE)) )
						{
							inputs.controllerClearInput(gui_player, INJOY_MENU_USE);
							inputs.mouseClearLeft(gui_player);

							bool result = executeOnItemClick(itemsDisplayed[i]);
							GUICurrentType oldType = guiType;
							rebuildGUIInventory();

							if ( oldType == GUI_TYPE_ALCHEMY && !guiActive )
							{
								// do nothing
							}
							else if ( itemsDisplayed[i] == nullptr )
							{
								if ( itemsDisplayed[0] == nullptr )
								{
									//Go back to inventory.
									selectedSlot = -1;
									players[gui_player]->inventoryUI.warpMouseToSelectedItem(nullptr, (Inputs::SET_CONTROLLER));
								}
								else
								{
									//Move up one slot.
									--selectedSlot;
									warpMouseToSelectedSlot();
								}
							}
						}
					}
				}

				if ( !selectingSlot )
				{
					selectedSlot = -1;
				}
			}

			//Okay, now prepare to render all the items.
			y = gui_startx + 22;
			c = 0;
			if ( player_inventory && guiType != GUI_TYPE_ALCHEMY )
			{
				rebuildGUIInventory();

				std::unordered_map<ItemType, int> itemCounts;
				if ( guiType == GUI_TYPE_TINKERING && tinkeringFilter == TINKER_FILTER_CRAFTABLE )
				{
					for ( node = stats[gui_player]->inventory.first; node != NULL; node = node->next )
					{
						if ( node->element )
						{
							Item* item = (Item*)node->element;
							itemCounts[item->type] += item->count;
						}
					}
					for ( node = player_inventory->first; node != NULL; node = node->next )
					{
						if ( node->element )
						{
							Item* item = (Item*)node->element;
							if ( isNodeTinkeringCraftableItem(item->node) )
							{
								// make the displayed items reflect how many you are carrying.
								item->count = 0;
								if ( itemCounts.find(item->type) != itemCounts.end() )
								{
									item->count = itemCounts[item->type];
								}
							}
							else
							{
								// stop once we reach normal inventory.
								break;
							}
						}
					}
				}
				//Actually render the items.
				c = 0;
				for ( node = player_inventory->first; node != NULL; node = node->next )
				{
					if ( node->element )
					{
						Item* item = (Item*)node->element;
						bool displayItem = shouldDisplayItemInGUI(item);
						if ( displayItem )   //Skip over all non-used items
						{
							c++;
							if ( c <= scroll )
							{
								continue;
							}
							char tempstr[256] = { 0 };
							int showTinkeringBotHealthPercentage = false;
							Uint32 color = uint32ColorWhite;
							if ( guiType == GUI_TYPE_TINKERING )
							{
								break;
								if ( isNodeTinkeringCraftableItem(item->node) )
								{
									// if anything, these should be doing
									// strncpy(tempstr, language[N], TEMPSTR_LEN - <extra space needed>)
									// not strlen(language[N]). there is zero safety conferred from this
									// anti-pattern. different story with memcpy(), but strcpy() is not
									// memcpy().
									strcpy(tempstr, language[3644]); // craft
									strncat(tempstr, item->description(), 46 - strlen(language[3644]));
									if ( !tinkeringPlayerCanAffordCraft(item) || (tinkeringPlayerHasSkillLVLToCraft(item) == -1) )
									{
										color = uint32ColorGray;
									}
								}
								else if ( isItemSalvageable(item, gui_player) && tinkeringFilter != TINKER_FILTER_REPAIRABLE )
								{
									strcpy(tempstr, language[3645]); // salvage
									strncat(tempstr, item->description(), 46 - strlen(language[3645]));
								}
								else if ( tinkeringIsItemRepairable(item, gui_player) )
								{
									if ( tinkeringIsItemUpgradeable(item) )
									{
										if ( tinkeringUpgradeMaxStatus(item) <= item->status )
										{
											color = uint32ColorGray; // can't upgrade since it's higher status than we can craft.
										}
										else if ( !tinkeringPlayerCanAffordRepair(item) )
										{
											color = uint32ColorGray; // can't upgrade since no materials
										}
										strcpy(tempstr, language[3684]); // upgrade
										strncat(tempstr, item->description(), 46 - strlen(language[3684]));
									}
									else
									{
										if ( tinkeringPlayerHasSkillLVLToCraft(item) == -1 && itemCategory(item) == TOOL )
										{
											color = uint32ColorGray; // can't repair since no we can't craft it.
										}
										else if ( !tinkeringPlayerCanAffordRepair(item) )
										{
											color = uint32ColorGray; // can't repair since no materials
										}
										strcpy(tempstr, language[3646]); // repair
										strncat(tempstr, item->description(), 46 - strlen(language[3646]));
									}
									if ( item->type == TOOL_SENTRYBOT || item->type == TOOL_DUMMYBOT || item->type == TOOL_SPELLBOT )
									{
										showTinkeringBotHealthPercentage = true;
									}
								}
								else
								{
									messagePlayer(clientnum, MESSAGE_DEBUG, "%d", item->type);
									strncat(tempstr, "invalid item", 13);
								}
							}
							else if ( guiType == GUI_TYPE_SCRIBING )
							{
								if ( isNodeScribingCraftableItem(item->node) )
								{
									snprintf(tempstr, sizeof(tempstr), language[3721], item->getScrollLabel());
								}
								else
								{
									if ( scribingFilter == SCRIBING_FILTER_REPAIRABLE )
									{
										strcpy(tempstr, language[3719]); // repair
										strncat(tempstr, item->description(), 46 - strlen(language[3718]));
									}
									else
									{
										strcpy(tempstr, language[3718]); // inscribe
										int oldcount = item->count;
										item->count = 1;
										strncat(tempstr, item->description(), 46 - strlen(language[3718]));
										item->count = oldcount;
									}
								}
							}
							else
							{
								strncpy(tempstr, item->description(), 46);
							}

							if ( showTinkeringBotHealthPercentage )
							{
								int health = 100;
								if ( item->appearance >= 0 && item->appearance <= 4 )
								{
									health = 25 * item->appearance;
									if ( health == 0 && item->status != BROKEN )
									{
										health = 5;
									}
								}
								char healthstr[32] = "";
								snprintf(healthstr, 16, " (%d%%)", health);
								strncat(tempstr, healthstr, 46 - strlen(tempstr) - strlen(healthstr));
							}
							else if ( item->type == ENCHANTED_FEATHER && item->identified )
							{
								char healthstr[32] = "";
								snprintf(healthstr, 16, " (%d%%)", item->appearance % ENCHANTED_FEATHER_MAX_DURABILITY);
								strncat(tempstr, healthstr, 46 - strlen(tempstr) - strlen(healthstr));
							}
							

							if ( strlen(tempstr) >= 46 )
							{
								strcat(tempstr, " ...");
							}
							ttfPrintTextColor(ttf8, gui_starty + 36, y, color, true, tempstr);
							pos.x = gui_starty + 16;
							pos.y = gui_startx + 17 + 18 * (c - scroll - 1);
							pos.w = 16;
							pos.h = 16;
							drawImageScaled(itemSprite(item), NULL, &pos);
							if ( guiType == GUI_TYPE_TINKERING )
							{
								int metal = 0;
								int magic = 0;
								if ( isNodeTinkeringCraftableItem(item->node) )
								{
									tinkeringGetCraftingCost(item, &metal, &magic);
								}
								else if ( isItemSalvageable(item, gui_player) && tinkeringFilter != TINKER_FILTER_REPAIRABLE )
								{
									tinkeringGetItemValue(item, &metal, &magic);
								}
								else if ( tinkeringIsItemRepairable(item, gui_player) )
								{
									tinkeringGetRepairCost(item, &metal, &magic);
								}
								pos.x = windowX2 - 20 - TTF8_WIDTH * 12;
								if ( !item->identified )
								{
									ttfPrintTextFormattedColor(ttf8, windowX2 - 24 - TTF8_WIDTH * 15, y, color, "  ?    ?");
								}
								else
								{
									ttfPrintTextFormattedColor(ttf8, windowX2 - 24 - TTF8_WIDTH * 15, y, color, "%3d  %3d", metal, magic);
								}
								node_t* imageNode = items[TOOL_METAL_SCRAP].surfaces.first;
								if ( imageNode )
								{
									drawImageScaled(*((SDL_Surface**)imageNode->element), NULL, &pos);
								}
								pos.x += TTF12_WIDTH * 4;
								imageNode = items[TOOL_MAGIC_SCRAP].surfaces.first;
								if ( imageNode )
								{
									drawImageScaled(*((SDL_Surface**)imageNode->element), NULL, &pos);
								}
							}
							y += 18;
							if ( c > 3 + scroll )
							{
								break;
							}
						}
					}
				}
			}
		}
	}
}

bool GenericGUIMenu::shouldDisplayItemInGUI(Item* item)
{
	if ( !item )
	{
		return false;
	}
	if ( guiType == GUI_TYPE_REPAIR )
	{
		return isItemRepairable(item, repairItemType);
	}
	else if ( guiType == GUI_TYPE_ALCHEMY )
	{
		return isItemMixable(item);
	}
	else if ( guiType == GUI_TYPE_TINKERING )
	{
		if ( isNodeTinkeringCraftableItem(item->node) )
		{
			if ( tinkeringFilter == TINKER_FILTER_ALL || tinkeringFilter == TINKER_FILTER_CRAFTABLE )
			{
				return true;
			}
		}
		else if ( tinkeringIsItemRepairable(item, gui_player) && tinkeringFilter == TINKER_FILTER_REPAIRABLE )
		{
			return true;
		}
		else if ( isItemSalvageable(item, gui_player) )
		{
			if ( tinkeringFilter == TINKER_FILTER_ALL || tinkeringFilter == TINKER_FILTER_SALVAGEABLE )
			{
				return true;
			}
		}
	}
	else if ( guiType == GUI_TYPE_SCRIBING )
	{
		if ( scribingFilter != SCRIBING_FILTER_REPAIRABLE )
		{
			if ( isNodeScribingCraftableItem(item->node) )
			{
				if ( scribingBlankScrollTarget )
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			else if ( !scribingBlankScrollTarget && item->identified && item->type == SCROLL_BLANK )
			{
				return true;
			}
		}
		else if ( scribingFilter != SCRIBING_FILTER_CRAFTABLE )
		{
			if ( itemCategory(item) == SPELLBOOK && item->identified && item->status < EXCELLENT )
			{
				return true;
			}
		}
	}
	else if ( guiType == GUI_TYPE_REMOVECURSE )
	{
		return isItemRemoveCursable(item);
	}
	else if ( guiType == GUI_TYPE_IDENTIFY )
	{
		return isItemIdentifiable(item);
	}
	return false;
}

void GenericGUIMenu::uncurseItem(Item* item)
{
	if ( !item )
	{
		return;
	}
	if ( !shouldDisplayItemInGUI(item) )
	{
		messagePlayer(gui_player, MESSAGE_MISC, language[347], item->getName());
		return;
	}

	item->beatitude = 0; //0 = uncursed. > 0 = blessed.
	messagePlayer(gui_player, MESSAGE_MISC, language[348], item->description());

	closeGUI();
	if ( multiplayer == CLIENT && itemIsEquipped(item, gui_player) )
	{
		// the client needs to inform the server that their equipment was uncursed.
		int armornum = 0;
		if ( item == stats[gui_player]->helmet )
		{
			armornum = 0;
		}
		else if ( item == stats[gui_player]->breastplate )
		{
			armornum = 1;
		}
		else if ( item == stats[gui_player]->gloves )
		{
			armornum = 2;
		}
		else if ( item == stats[gui_player]->shoes )
		{
			armornum = 3;
		}
		else if ( item == stats[gui_player]->shield )
		{
			armornum = 4;
		}
		else if ( item == stats[gui_player]->weapon )
		{
			armornum = 5;
		}
		else if ( item == stats[gui_player]->cloak )
		{
			armornum = 6;
		}
		else if ( item == stats[gui_player]->amulet )
		{
			armornum = 7;
		}
		else if ( item == stats[gui_player]->ring )
		{
			armornum = 8;
		}
		else if ( item == stats[gui_player]->mask )
		{
			armornum = 9;
		}
		strcpy((char*)net_packet->data, "RCUR");
		net_packet->data[4] = gui_player;
		net_packet->data[5] = armornum;
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 6;
		sendPacketSafe(net_sock, -1, net_packet, 0);
	}
}

void GenericGUIMenu::identifyItem(Item* item)
{
	if ( !item )
	{
		return;
	}
	if ( !shouldDisplayItemInGUI(item) )
	{
		messagePlayer(gui_player, MESSAGE_MISC, language[319], item->getName());
		return;
	}

	item->identified = true;
	messagePlayer(gui_player, MESSAGE_MISC, language[320], item->description());
	closeGUI();
}

void GenericGUIMenu::repairItem(Item* item)
{
	if ( !item )
	{
		return;
	}
	if ( !shouldDisplayItemInGUI(item) )
	{
		messagePlayer(gui_player, MESSAGE_MISC, language[3287], item->getName());
		return;
	}

	bool isEquipped = itemIsEquipped(item, gui_player);

	if ( repairItemType == SCROLL_CHARGING )
	{
		if ( itemCategory(item) == MAGICSTAFF )
		{
			if ( item->status == BROKEN )
			{
				if ( usingScrollBeatitude > 0 )
				{
					item->status = EXCELLENT;
				}
				else
				{
					item->status = WORN;
				}
			}
			else
			{
				item->status = EXCELLENT;
			}
		}
		else if ( item->type == ENCHANTED_FEATHER )
		{
			int durability = item->appearance % ENCHANTED_FEATHER_MAX_DURABILITY;
			int repairAmount = 100 - durability;
			if ( repairAmount > (ENCHANTED_FEATHER_MAX_DURABILITY / 2) )
			{
				if ( usingScrollBeatitude == 0 )
				{
					repairAmount = ENCHANTED_FEATHER_MAX_DURABILITY / 2;
				}
			}
			item->appearance += repairAmount;
			item->status = EXCELLENT;
		}
		messagePlayer(gui_player, MESSAGE_MISC, language[3730], item->getName());
	}
	else
	{
		if ( item->status == BROKEN )
		{
			item->status = DECREPIT;
		}
		else
		{
			if ( (item->type >= ARTIFACT_SWORD && item->type <= ARTIFACT_GLOVES) || item->type == BOOMERANG )
			{
				item->status = static_cast<Status>(std::min(item->status + 1, static_cast<int>(EXCELLENT)));
			}
			else
			{
				item->status = static_cast<Status>(std::min(item->status + 2 + usingScrollBeatitude, static_cast<int>(EXCELLENT)));
			}
		}
		messagePlayer(gui_player, MESSAGE_MISC, language[872], item->getName());
	}
	closeGUI();
	if ( multiplayer == CLIENT && isEquipped )
	{
		// the client needs to inform the server that their equipment was repaired.
		int armornum = 0;
		if ( item == stats[gui_player]->weapon )
		{
			armornum = 0;
		}
		else if ( item == stats[gui_player]->helmet )
		{
			armornum = 1;
		}
		else if ( item == stats[gui_player]->breastplate )
		{
			armornum = 2;
		}
		else if ( item == stats[gui_player]->gloves )
		{
			armornum = 3;
		}
		else if ( item == stats[gui_player]->shoes )
		{
			armornum = 4;
		}
		else if ( item == stats[gui_player]->shield )
		{
			armornum = 5;
		}
		else if ( item == stats[gui_player]->cloak )
		{
			armornum = 6;
		}
		else if ( item == stats[gui_player]->mask )
		{
			armornum = 7;
		}
		strcpy((char*)net_packet->data, "REPA");
		net_packet->data[4] = gui_player;
		net_packet->data[5] = armornum;
		net_packet->data[6] = item->status;
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 7;
		sendPacketSafe(net_sock, -1, net_packet, 0);
	}
}

void GenericGUIMenu::closeGUI()
{
	bool wasOpen = guiActive;
	tinkeringFreeLists();
	scribingFreeLists();
	guiActive = false;
	selectedSlot = -1;
	guiType = GUI_TYPE_NONE;
	basePotion = nullptr;
	secondaryPotion = nullptr;
	alembicItem = nullptr;
	repairItemType = 0;
	removeCurseUsingSpell = false;
	identifyUsingSpell = false;
	if ( tinkerGUI.bOpen )
	{
		tinkerGUI.closeTinkerMenu();
	}
	if ( alchemyGUI.bOpen )
	{
		alchemyGUI.closeAlchemyMenu();
	}
	if ( wasOpen )
	{
		players[gui_player]->inventoryUI.tooltipDelayTick = ticks + TICKS_PER_SECOND / 10;
	}
}

inline Item* GenericGUIMenu::getItemInfo(int slot)
{
	if ( slot >= kNumShownItems )
	{
		return nullptr; //Out of bounds,
	}

	return itemsDisplayed[slot];
}

void GenericGUIMenu::selectSlot(int slot)
{
	if ( slot < selectedSlot )
	{
		//Moving up.

		/*
		* Possible cases:
		* * 1) Move cursor up the GUI through different selectedSlot.
		* * 2) Page up through scroll--
		* * 3) Scrolling up past top of GUI, no scroll (move back to inventory)
		*/

		if ( selectedSlot <= 0 )
		{
			//Covers cases 2 & 3.

			/*
			* Possible cases:
			* * A) Hit very top of "inventory", can't go any further. Return to inventory.
			* * B) Page up, scrolling through scroll.
			*/

			if ( scroll <= 0 )
			{
				//Case 3/A: Return to inventory.
				selectedSlot = -1;
			}
			else
			{
				//Case 2/B: Page up through "inventory".
				--scroll;
			}
		}
		else
		{
			//Covers case 1.

			//Move cursor up the GUI through different selectedSlot (--selectedSlot).
			--selectedSlot;
			warpMouseToSelectedSlot();
		}
	}
	else if ( slot > selectedSlot )
	{
		//Moving down.

		/*
		* Possible cases:
		* * 1) Moving cursor down through GUI through different selectedSlot.
		* * 2) Scrolling down past bottom of GUI through scroll++
		* * 3) Scrolling down past bottom of GUI, max scroll (revoke move -- can't go beyond limit of GUI).
		*/

		if ( selectedSlot >= kNumShownItems - 1 )
		{
			//Covers cases 2 & 3.
			++scroll; //scroll is automatically sanitized in updateGUI().
		}
		else
		{
			//Covers case 1.
			//Move cursor down through the GUI through different selectedSlot (++selectedSlot).
			//This is a little bit trickier since must revoke movement if there is no item in the next slot!

			/*
			* Two possible cases:
			* * A) Items below this. Advance selectedSlot to them.
			* * B) On last item already. Do nothing (revoke movement).
			*/

			Item* item = getItemInfo(selectedSlot + 1);

			if ( item )
			{
				++selectedSlot;
				warpMouseToSelectedSlot();
			}
			else
			{
				//No more items. Stop.
			}
		}
	}
}

void GenericGUIMenu::warpMouseToSelectedSlot()
{
	SDL_Rect slotPos;
	slotPos.x = gui_starty;
	slotPos.w = inventoryoptionChest_bmp->w;
	slotPos.h = inventoryoptionChest_bmp->h;
	slotPos.y = gui_startx + 16 + (slotPos.h * selectedSlot);

	// to verify for splitscreen
	//SDL_WarpMouseInWindow(screen, slotPos.x + (slotPos.w / 2), slotPos.y + (slotPos.h / 2));
}

void GenericGUIMenu::openGUI(int type, int scrollBeatitude, int scrollType)
{
	this->closeGUI();

	players[gui_player]->openStatusScreen(GUI_MODE_INVENTORY, INVENTORY_MODE_ITEM); // Reset the GUI to the inventory.

	guiActive = true;
	usingScrollBeatitude = scrollBeatitude;
	repairItemType = scrollType;
	guiType = static_cast<GUICurrentType>(type);
	gui_starty = (players[gui_player]->camera_midx() - (inventoryChest_bmp->w / 2)) + offsetx;
	gui_startx = (players[gui_player]->camera_midy() - (inventoryChest_bmp->h / 2)) + offsety;

	FollowerMenu[gui_player].closeFollowerMenuGUI();

	if ( openedChest[gui_player] )
	{
		openedChest[gui_player]->closeChest();
	}
	rebuildGUIInventory();
}

void GenericGUIMenu::openGUI(int type, bool experimenting, Item* itemOpenedWith)
{
	this->closeGUI();
	if ( players[gui_player] && players[gui_player]->entity )
	{
		if ( players[gui_player]->entity->isBlind() )
		{
			messagePlayer(gui_player, MESSAGE_MISC, language[892]);
			return; // I can't see!
		}
	}

	if ( players[gui_player]->inventoryUI.bCompactView )
	{
		// if compact view, then we don't want the inventory slot being selected
		if ( players[gui_player]->inventoryUI.getSelectedSlotY() < 0 )
		{
			players[gui_player]->inventoryUI.selectSlot(0, 0);
		}
	}
	players[gui_player]->openStatusScreen(GUI_MODE_INVENTORY, INVENTORY_MODE_ITEM); // Reset the GUI to the inventory.
	guiActive = true;
	alembicItem = itemOpenedWith;
	experimentingAlchemy = experimenting;
	guiType = static_cast<GUICurrentType>(type);
	//players[gui_player]->GUI.activateModule(Player::GUI_t::MODULE_ALCHEMY);
	alchemyGUI.openAlchemyMenu();

	gui_starty = (players[gui_player]->camera_midx() - (inventoryChest_bmp->w / 2)) + offsetx;
	gui_startx = 360 + (players[gui_player]->camera_midy() - (inventoryChest_bmp->h / 2)) + offsety;

	FollowerMenu[gui_player].closeFollowerMenuGUI();

	if ( openedChest[gui_player] )
	{
		openedChest[gui_player]->closeChest();
	}
	rebuildGUIInventory();
}

void GenericGUIMenu::openGUI(int type, Item* itemOpenedWith)
{
	this->closeGUI();
	if ( players[gui_player] && players[gui_player]->entity )
	{
		if ( players[gui_player]->entity->isBlind() )
		{
			messagePlayer(gui_player, MESSAGE_MISC, language[892]);
			return; // I can't see!
		}
	}

	if ( players[gui_player]->inventoryUI.bCompactView )
	{
		// if compact view, then we don't want the inventory slot being selected
		// e.g opening a tinkering kit from your hand slot
		if ( players[gui_player]->inventoryUI.getSelectedSlotY() < 0 )
		{
			players[gui_player]->inventoryUI.selectSlot(0, 0);
		}
	}
	players[gui_player]->openStatusScreen(GUI_MODE_INVENTORY, INVENTORY_MODE_ITEM); // Reset the GUI to the inventory.
	guiActive = true;
	guiType = static_cast<GUICurrentType>(type);

	gui_starty = (players[gui_player]->camera_midx() - (inventoryChest_bmp->w / 2)) + offsetx;
	gui_startx = (players[gui_player]->camera_midy() - (inventoryChest_bmp->h / 2)) + offsety;

	// build the craftables list.
	if ( guiType == GUI_TYPE_TINKERING )
	{
		tinkeringKitItem = itemOpenedWith;
		tinkeringCreateCraftableItemList();
		if ( tinkeringFilter == TINKER_FILTER_CRAFTABLE )
		{
			players[gui_player]->GUI.activateModule(Player::GUI_t::MODULE_TINKERING);
		}
		tinkerGUI.openTinkerMenu();
	}
	else if ( guiType == GUI_TYPE_SCRIBING )
	{
		scribingToolItem = itemOpenedWith;
		scribingCreateCraftableItemList();
	}
	else if ( guiType == GUI_TYPE_REMOVECURSE )
	{
		if ( !itemOpenedWith )
		{
			removeCurseUsingSpell = true;
		}
	}
	else if ( guiType == GUI_TYPE_IDENTIFY )
	{
		if ( !itemOpenedWith )
		{
			identifyUsingSpell = true;
		}
	}

	FollowerMenu[gui_player].closeFollowerMenuGUI();

	if ( openedChest[gui_player] )
	{
		openedChest[gui_player]->closeChest();
	}
	rebuildGUIInventory();
}

bool hideRecipeFromList(int type);

bool GenericGUIMenu::executeOnItemClick(Item* item)
{
	if ( !item )
	{
		return false;
	}

	if ( guiType == GUI_TYPE_REPAIR )
	{
		repairItem(item);
		return true;
	}
	else if ( guiType == GUI_TYPE_REMOVECURSE )
	{
		uncurseItem(item);
		return true;
	}
	else if ( guiType == GUI_TYPE_IDENTIFY )
	{
		identifyItem(item);
		return true;
	}
	else if ( guiType == GUI_TYPE_ALCHEMY )
	{
		if ( !basePotion )
		{
			if ( isItemMixable(item) )
			{
				basePotion = item;
				// check if secondary potion available.
				list_t* player_inventory = &stats[gui_player]->inventory;
				for ( node_t* node = player_inventory->first; node != nullptr; node = node->next )
				{
					if ( node->element )
					{
						Item* checkItem = (Item*)node->element;
						if ( checkItem && isItemMixable(checkItem) )
						{
							return true;
						}
					}
				}
				// did not find mixable item... close GUI
				if ( !experimentingAlchemy )
				{
					messagePlayer(gui_player, MESSAGE_MISC, language[3337]);
				}
				else
				{
					messagePlayer(gui_player, MESSAGE_MISC, language[3342]);
				}
				closeGUI();
				return false;
			}
		}
		else if ( !secondaryPotion )
		{
			if ( isItemMixable(item) )
			{
				secondaryPotion = item;
				if ( secondaryPotion && basePotion )
				{
					alchemyCombinePotions();
					basePotion = nullptr;
					secondaryPotion = nullptr;
				}
			}
		}
		return true;
	}
	else if ( guiType == GUI_TYPE_TINKERING )
	{
		if ( isNodeTinkeringCraftableItem(item->node) )
		{
			tinkeringCraftItem(item);
		}
		else if ( isNodeFromPlayerInventory(item->node) )
		{
			if ( tinkeringFilter == TINKER_FILTER_REPAIRABLE )
			{
				if ( tinkeringIsItemRepairable(item, gui_player) )
				{
					tinkeringRepairItem(item);
				}
			}
			else if ( tinkeringFilter == TINKER_FILTER_SALVAGEABLE )
			{
				tinkeringSalvageItem(item, false, gui_player);
			}
		}
		return true;
	}
	else if ( guiType == GUI_TYPE_SCRIBING )
	{
		if ( isNodeScribingCraftableItem(item->node) )
		{
			scribingWriteItem(item);
		}
		else if ( isNodeFromPlayerInventory(item->node) )
		{
			if ( item->identified && item->type == SCROLL_BLANK )
			{
				scribingBlankScrollTarget = item;
			}
			else if ( item->identified && itemCategory(item) == SPELLBOOK )
			{
				scribingWriteItem(item);
			}
		}
		return true;
	}

	return false;
}

bool GenericGUIMenu::isItemMixable(const Item* item)
{
	if ( !item )
	{
		return false;
	}

	if ( itemCategory(item) != POTION )
	{
		return false;
	}
	if ( item->type == POTION_EMPTY )
	{
		return false;
	}

	//if ( players[gui_player] && players[gui_player]->entity )
	//{
	//	if ( players[gui_player]->entity->isBlind() )
	//	{
	//		messagePlayer(gui_player, MESSAGE_MISC, language[892]);
	//		closeGUI();
	//		return false; // I can't see!
	//	}
	//}


	if ( !experimentingAlchemy && !item->identified )
	{
		if ( !basePotion )
		{
			return false;
		}
		else if ( item != basePotion )
		{
			return true;
		}
		return false;
	}

	if ( itemIsEquipped(item, gui_player) )
	{
		return false; // don't want to deal with client/server desync problems here.
	}

	//int skillLVL = 0;
	//if ( stats[gui_player] )
	//{
	//	skillLVL = stats[gui_player]->PROFICIENCIES[PRO_ALCHEMY] / 20; // 0 to 5;
	//}

	if ( experimentingAlchemy )
	{
		if ( !basePotion )
		{
			return true;
		}
		else if ( !secondaryPotion )
		{
			if ( item != basePotion )
			{
				return true;
			}
		}
	}

	if ( !basePotion )
	{
		// we're selecting the first potion.
		switch ( item->type )
		{
			case POTION_WATER:
			case POTION_POLYMORPH:
			case POTION_BOOZE:
			case POTION_JUICE:
			case POTION_ACID:
			case POTION_INVISIBILITY:
				if ( clientLearnedAlchemyIngredients[gui_player].find(item->type) 
					!= clientLearnedAlchemyIngredients[gui_player].end() )
				{
					return true;
				}
				else
				{
					return false;
				}
				break;
			default:
				return false;
				break;
		}
	}

	if ( !secondaryPotion )
	{
		if ( item == basePotion )
		{
			return false;
		}

		// we're selecting the second potion.
		switch ( item->type )
		{
			case POTION_WATER:
			case POTION_SICKNESS:
			case POTION_CONFUSION:
			case POTION_CUREAILMENT:
			case POTION_BLINDNESS:
			case POTION_RESTOREMAGIC:
			case POTION_SPEED:
			case POTION_POLYMORPH:
				if ( clientLearnedAlchemyIngredients[gui_player].find(item->type) 
					!= clientLearnedAlchemyIngredients[gui_player].end() )
				{
					return true;
				}
				else
				{
					return false;
				}
				break;
			default:
				return false;
				break;
		}
	}

	return false;
}

ItemType alchemyMixResult(ItemType potion1, ItemType potion2, 
	bool& outRandomResult, bool& outTryDuplicatePotion, bool& outSamePotion, bool& outExplodeSelf)
{
	outTryDuplicatePotion = false;
	ItemType result = POTION_SICKNESS;
	outRandomResult = false;
	outExplodeSelf = false;

	switch ( potion1 )
	{
		case POTION_WATER:
			if ( potion2 == POTION_ACID )
			{
				outExplodeSelf = true;
			}
			else
			{
				outTryDuplicatePotion = true;
			}
			break;
		case POTION_BOOZE:
			switch ( potion2 )
			{
				case POTION_SICKNESS:
					result = POTION_CONFUSION;
					break;
				case POTION_CONFUSION:
					result = POTION_ACID;
					break;
				case POTION_CUREAILMENT:
					result = POTION_SPEED;
					break;
				case POTION_BLINDNESS:
					result = POTION_STRENGTH;
					break;
				case POTION_RESTOREMAGIC:
					result = POTION_BLINDNESS;
					break;
				case POTION_SPEED:
					result = POTION_PARALYSIS;
					break;
				case POTION_POLYMORPH:
					outRandomResult = true;
					break;
				default:
					break;
			}
			break;
		case POTION_JUICE:
			switch ( potion2 )
			{
				case POTION_SICKNESS:
					result = POTION_BOOZE;
					break;
				case POTION_CONFUSION:
					result = POTION_BOOZE;
					break;
				case POTION_CUREAILMENT:
					result = POTION_RESTOREMAGIC;
					break;
				case POTION_BLINDNESS:
					result = POTION_CUREAILMENT;
					break;
				case POTION_RESTOREMAGIC:
					result = POTION_HEALING;
					break;
				case POTION_SPEED:
					result = POTION_INVISIBILITY;
					break;
				case POTION_POLYMORPH:
					outRandomResult = true;
					break;
				default:
					break;
			}
			break;
		case POTION_ACID:
			switch ( potion2 )
			{
				case POTION_WATER:
					outExplodeSelf = true; // oh no. don't do that.
					break;
				case POTION_SICKNESS:
					result = POTION_FIRESTORM;
					break;
				case POTION_CONFUSION:
					result = POTION_JUICE;
					break;
				case POTION_CUREAILMENT:
					result = POTION_FIRESTORM;
					break;
				case POTION_BLINDNESS:
					result = POTION_ICESTORM;
					break;
				case POTION_RESTOREMAGIC:
					result = POTION_ICESTORM;
					break;
				case POTION_SPEED:
					result = POTION_THUNDERSTORM;
					break;
				case POTION_POLYMORPH:
					outRandomResult = true;
					break;
				default:
					outExplodeSelf = true;
					break;
			}
			break;
		case POTION_INVISIBILITY:
			switch ( potion2 )
			{
				case POTION_SICKNESS:
					result = POTION_BLINDNESS;
					break;
				case POTION_CONFUSION:
					result = POTION_PARALYSIS;
					break;
				case POTION_CUREAILMENT:
					result = POTION_LEVITATION;
					break;
				case POTION_BLINDNESS:
					result = POTION_POLYMORPH;
					break;
				case POTION_RESTOREMAGIC:
					result = POTION_EXTRAHEALING;
					break;
				case POTION_SPEED:
					result = POTION_RESTOREMAGIC;
					break;
				case POTION_POLYMORPH:
					outRandomResult = true;
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}

	if ( result == POTION_SICKNESS ) // didn't get a result, try flip the potion order
	{
		switch ( potion2 )
		{
			case POTION_WATER:
				if ( potion1 == POTION_ACID )
				{
					outExplodeSelf = true;
				}
				else
				{
					outTryDuplicatePotion = true;
				}
				break;
			case POTION_BOOZE:
				switch ( potion1 )
				{
					case POTION_SICKNESS:
						result = POTION_CONFUSION;
						break;
					case POTION_CONFUSION:
						result = POTION_ACID;
						break;
					case POTION_CUREAILMENT:
						result = POTION_SPEED;
						break;
					case POTION_BLINDNESS:
						result = POTION_STRENGTH;
						break;
					case POTION_RESTOREMAGIC:
						result = POTION_BLINDNESS;
						break;
					case POTION_SPEED:
						result = POTION_PARALYSIS;
						break;
					case POTION_POLYMORPH:
						outRandomResult = true;
						break;
					default:
						break;
				}
				break;
			case POTION_JUICE:
				switch ( potion1 )
				{
					case POTION_SICKNESS:
						result = POTION_BOOZE;
						break;
					case POTION_CONFUSION:
						result = POTION_BOOZE;
						break;
					case POTION_CUREAILMENT:
						result = POTION_RESTOREMAGIC;
						break;
					case POTION_BLINDNESS:
						result = POTION_CUREAILMENT;
						break;
					case POTION_RESTOREMAGIC:
						result = POTION_HEALING;
						break;
					case POTION_SPEED:
						result = POTION_INVISIBILITY;
						break;
					case POTION_POLYMORPH:
						outRandomResult = true;
						break;
					default:
						break;
				}
				break;
			case POTION_ACID:
				switch ( potion1 )
				{
					case POTION_WATER:
						outExplodeSelf = true; // oh no. don't do that.
						break;
					case POTION_SICKNESS:
						result = POTION_FIRESTORM;
						break;
					case POTION_CONFUSION:
						result = POTION_JUICE;
						break;
					case POTION_CUREAILMENT:
						result = POTION_FIRESTORM;
						break;
					case POTION_BLINDNESS:
						result = POTION_ICESTORM;
						break;
					case POTION_RESTOREMAGIC:
						result = POTION_ICESTORM;
						break;
					case POTION_SPEED:
						result = POTION_THUNDERSTORM;
						break;
					case POTION_POLYMORPH:
						outRandomResult = true;
						break;
					default:
						outExplodeSelf = true;
						break;
				}
				break;
			case POTION_INVISIBILITY:
				switch ( potion1 )
				{
					case POTION_SICKNESS:
						result = POTION_BLINDNESS;
						break;
					case POTION_CONFUSION:
						result = POTION_PARALYSIS;
						break;
					case POTION_CUREAILMENT:
						result = POTION_LEVITATION;
						break;
					case POTION_BLINDNESS:
						result = POTION_POLYMORPH;
						break;
					case POTION_RESTOREMAGIC:
						result = POTION_EXTRAHEALING;
						break;
					case POTION_SPEED:
						result = POTION_RESTOREMAGIC;
						break;
					case POTION_POLYMORPH:
						outRandomResult = true;
						break;
					default:
						break;
				}
				break;
			default:
				break;
		}
	}

	if ( potion1 == potion2 )
	{
		outExplodeSelf = false;
		outRandomResult = false;
		outSamePotion = true;
		outTryDuplicatePotion = false;
		return potion1;
	}
	if ( outExplodeSelf )
	{
		return TOOL_BOMB;
	}
	if ( (potion1 == POTION_POLYMORPH && potion2 != POTION_POLYMORPH) 
		|| (potion1 != POTION_POLYMORPH && potion2 == POTION_POLYMORPH) )
	{
		outRandomResult = true;
	}
	return result;
}

bool alchemyAddRecipe(int player, int basePotion, int secondaryPotion, int result, bool ignorePotionTypes = false)
{
	if ( basePotion == POTION_WATER || secondaryPotion == POTION_WATER
		|| basePotion == secondaryPotion || basePotion == POTION_POLYMORPH
		|| secondaryPotion == POTION_POLYMORPH )
	{
		if ( !ignorePotionTypes )
		{
			return false;
		}
	}
	if ( GenericGUI[player].isItemBaseIngredient(secondaryPotion) && GenericGUI[player].isItemSecondaryIngredient(basePotion) )
	{
		// keep the orders consistent, base then secondary
		int swapBasePotion = basePotion;
		basePotion = secondaryPotion;
		secondaryPotion = swapBasePotion;
	}
	bool found = false;
	for ( auto& entry : clientLearnedAlchemyRecipes[player] )
	{
		if ( entry.first == result
			&& ((entry.second.first == basePotion && entry.second.second == secondaryPotion)
				|| (entry.second.first == secondaryPotion && entry.second.second == basePotion)) )
		{
			return false; // already exists
		}
	}
	if ( !found )
	{
		clientLearnedAlchemyRecipes[player].push_back(std::make_pair(result, std::make_pair(basePotion, secondaryPotion)));
		if ( !hideRecipeFromList(result) )
		{
			std::string itemName = items[result].name_identified;
			size_t pos = std::string::npos;
			for ( auto& potionName : Player::SkillSheet_t::skillSheetData.potionNamesToFilter )
			{
				if ( (pos = itemName.find(potionName)) != std::string::npos )
				{
					itemName.erase(pos, potionName.length());
				}
			}
			capitalizeString(itemName);
			std::string iconPath = "";
			node_t* imagePathsNode = nullptr;
			for ( auto it = potionStandardAppearanceMap.begin(); it != potionStandardAppearanceMap.end(); ++it )
			{
				// loop through to get the standard appearance
				if ( (*it).first == result )
				{
					Uint32 index = (*it).second % items[result].variations;
					imagePathsNode = list_Node(&items[result].images, index);
					if ( imagePathsNode )
					{
						string_t* imagePath = static_cast<string_t*>(imagePathsNode->element);
						iconPath = imagePath->data;
					}
				}
			}
			GenericGUI[player].alchemyGUI.notifications.push_back(std::make_pair(ticks,
				GenericGUIMenu::AlchemyGUI_t::AlchNotification_t(language[4179], itemName, iconPath)));
			messagePlayerColor(player, MESSAGE_PROGRESSION, makeColorRGB(0, 255, 0), language[4182], items[result].name_identified);
		}
		return true;
	}
	return false;
}

void GenericGUIMenu::alchemyCombinePotions()
{
	if ( !basePotion || !secondaryPotion )
	{
		return;
	}

	bool tryDuplicatePotion = false;
	bool randomResult = false;
	bool explodeSelf = false;
	bool samePotion = false;
	const ItemType basePotionType = basePotion->type;
	const ItemType secondaryPotionType = secondaryPotion->type;
	ItemType result = alchemyMixResult(basePotion->type, secondaryPotion->type, randomResult, tryDuplicatePotion, samePotion, explodeSelf);

	//switch ( basePotion->type )
	//{
	//	case POTION_WATER:
	//		if ( secondaryPotion->type == POTION_ACID )
	//		{
	//			explodeSelf = true;
	//		}
	//		else
	//		{
	//			tryDuplicatePotion = true;
	//		}
	//		break;
	//	case POTION_BOOZE:
	//		switch ( secondaryPotion->type )
	//		{
	//			case POTION_SICKNESS:
	//				result = POTION_CONFUSION;
	//				break;
	//			case POTION_CONFUSION:
	//				result = POTION_ACID;
	//				break;
	//			case POTION_CUREAILMENT:
	//				result = POTION_SPEED;
	//				break;
	//			case POTION_BLINDNESS:
	//				result = POTION_STRENGTH;
	//				break;
	//			case POTION_RESTOREMAGIC:
	//				result = POTION_BLINDNESS;
	//				break;
	//			case POTION_SPEED:
	//				result = POTION_PARALYSIS;
	//				break;
	//			case POTION_POLYMORPH:
	//				randomResult = true;
	//				break;
	//			default:
	//				break;
	//		}
	//		break;
	//	case POTION_JUICE:
	//		switch ( secondaryPotion->type )
	//		{
	//			case POTION_SICKNESS:
	//				result = POTION_BOOZE;
	//				break;
	//			case POTION_CONFUSION:
	//				result = POTION_BOOZE;
	//				break;
	//			case POTION_CUREAILMENT:
	//				result = POTION_RESTOREMAGIC;
	//				break;
	//			case POTION_BLINDNESS:
	//				result = POTION_CUREAILMENT;
	//				break;
	//			case POTION_RESTOREMAGIC:
	//				result = POTION_HEALING;
	//				break;
	//			case POTION_SPEED:
	//				result = POTION_INVISIBILITY;
	//				break;
	//			case POTION_POLYMORPH:
	//				randomResult = true;
	//				break;
	//			default:
	//				break;
	//		}
	//		break;
	//	case POTION_ACID:
	//		switch ( secondaryPotion->type )
	//		{
	//			case POTION_WATER:
	//				explodeSelf = true; // oh no. don't do that.
	//				break;
	//			case POTION_SICKNESS:
	//				result = POTION_FIRESTORM;
	//				break;
	//			case POTION_CONFUSION:
	//				result = POTION_JUICE;
	//				break;
	//			case POTION_CUREAILMENT:
	//				result = POTION_FIRESTORM;
	//				break;
	//			case POTION_BLINDNESS:
	//				result = POTION_ICESTORM;
	//				break;
	//			case POTION_RESTOREMAGIC:
	//				result = POTION_ICESTORM;
	//				break;
	//			case POTION_SPEED:
	//				result = POTION_THUNDERSTORM;
	//				break;
	//			case POTION_POLYMORPH:
	//				randomResult = true;
	//				break;
	//			default:
	//				explodeSelf = true;
	//				break;
	//		}
	//		break;
	//	case POTION_INVISIBILITY:
	//		switch ( secondaryPotion->type )
	//		{
	//			case POTION_SICKNESS:
	//				result = POTION_BLINDNESS;
	//				break;
	//			case POTION_CONFUSION:
	//				result = POTION_PARALYSIS;
	//				break;
	//			case POTION_CUREAILMENT:
	//				result = POTION_LEVITATION;
	//				break;
	//			case POTION_BLINDNESS:
	//				result = POTION_POLYMORPH;
	//				break;
	//			case POTION_RESTOREMAGIC:
	//				result = POTION_EXTRAHEALING;
	//				break;
	//			case POTION_SPEED:
	//				result = POTION_RESTOREMAGIC;
	//				break;
	//			case POTION_POLYMORPH:
	//				randomResult = true;
	//				break;
	//			default:
	//				break;
	//		}
	//		break;
	//	default:
	//		break;
	//}

	//if ( result == POTION_SICKNESS ) // didn't get a result, try flip the potion order
	//{
	//	switch ( secondaryPotion->type )
	//	{
	//		case POTION_WATER:
	//			if ( basePotion->type == POTION_ACID )
	//			{
	//				explodeSelf = true;
	//			}
	//			else
	//			{
	//				tryDuplicatePotion = true;
	//			}
	//			break;
	//		case POTION_BOOZE:
	//			switch ( basePotion->type )
	//			{
	//				case POTION_SICKNESS:
	//					result = POTION_CONFUSION;
	//					break;
	//				case POTION_CONFUSION:
	//					result = POTION_ACID;
	//					break;
	//				case POTION_CUREAILMENT:
	//					result = POTION_SPEED;
	//					break;
	//				case POTION_BLINDNESS:
	//					result = POTION_STRENGTH;
	//					break;
	//				case POTION_RESTOREMAGIC:
	//					result = POTION_BLINDNESS;
	//					break;
	//				case POTION_SPEED:
	//					result = POTION_PARALYSIS;
	//					break;
	//				case POTION_POLYMORPH:
	//					randomResult = true;
	//					break;
	//				default:
	//					break;
	//			}
	//			break;
	//		case POTION_JUICE:
	//			switch ( basePotion->type )
	//			{
	//				case POTION_SICKNESS:
	//					result = POTION_BOOZE;
	//					break;
	//				case POTION_CONFUSION:
	//					result = POTION_BOOZE;
	//					break;
	//				case POTION_CUREAILMENT:
	//					result = POTION_RESTOREMAGIC;
	//					break;
	//				case POTION_BLINDNESS:
	//					result = POTION_CUREAILMENT;
	//					break;
	//				case POTION_RESTOREMAGIC:
	//					result = POTION_HEALING;
	//					break;
	//				case POTION_SPEED:
	//					result = POTION_INVISIBILITY;
	//					break;
	//				case POTION_POLYMORPH:
	//					randomResult = true;
	//					break;
	//				default:
	//					break;
	//			}
	//			break;
	//		case POTION_ACID:
	//			switch ( basePotion->type )
	//			{
	//				case POTION_WATER:
	//					explodeSelf = true; // oh no. don't do that.
	//					break;
	//				case POTION_SICKNESS:
	//					result = POTION_FIRESTORM;
	//					break;
	//				case POTION_CONFUSION:
	//					result = POTION_JUICE;
	//					break;
	//				case POTION_CUREAILMENT:
	//					result = POTION_FIRESTORM;
	//					break;
	//				case POTION_BLINDNESS:
	//					result = POTION_ICESTORM;
	//					break;
	//				case POTION_RESTOREMAGIC:
	//					result = POTION_ICESTORM;
	//					break;
	//				case POTION_SPEED:
	//					result = POTION_THUNDERSTORM;
	//					break;
	//				case POTION_POLYMORPH:
	//					randomResult = true;
	//					break;
	//				default:
	//					explodeSelf = true;
	//					break;
	//			}
	//			break;
	//		case POTION_INVISIBILITY:
	//			switch ( basePotion->type )
	//			{
	//				case POTION_SICKNESS:
	//					result = POTION_BLINDNESS;
	//					break;
	//				case POTION_CONFUSION:
	//					result = POTION_PARALYSIS;
	//					break;
	//				case POTION_CUREAILMENT:
	//					result = POTION_LEVITATION;
	//					break;
	//				case POTION_BLINDNESS:
	//					result = POTION_POLYMORPH;
	//					break;
	//				case POTION_RESTOREMAGIC:
	//					result = POTION_EXTRAHEALING;
	//					break;
	//				case POTION_SPEED:
	//					result = POTION_RESTOREMAGIC;
	//					break;
	//				case POTION_POLYMORPH:
	//					randomResult = true;
	//					break;
	//				default:
	//					break;
	//			}
	//			break;
	//		default:
	//			break;
	//	}
	//}

	//if ( basePotion->type == POTION_POLYMORPH || secondaryPotion->type == POTION_POLYMORPH )
	//{
	//	randomResult = true;
	//}

	int skillLVL = 0;
	if ( stats[gui_player] )
	{
		skillLVL = stats[gui_player]->PROFICIENCIES[PRO_ALCHEMY] / 20; // 0 to 5;
	}

	Status status = EXCELLENT;
	bool duplicateSucceed = false;
	if ( tryDuplicatePotion && !explodeSelf && !randomResult )
	{
		// do duplicate.
		if ( rand() % 100 < (50 + skillLVL * 10) ) // 50 - 100% chance
		{
			duplicateSucceed = true;
			if ( basePotion->type == POTION_WATER )
			{
				result = secondaryPotion->type;
				status = secondaryPotion->status;
			}
			else if ( secondaryPotion->type == POTION_WATER )
			{
				result = basePotion->type;
				status = basePotion->status;
			}
			else
			{
				result = POTION_WATER;
			}
		}
		else
		{
			result = POTION_WATER;
			if ( basePotion->type == POTION_WATER )
			{
				status = basePotion->status;
			}
			else if ( secondaryPotion->type == POTION_WATER )
			{
				status = secondaryPotion->status;
			}
		}
	}

	if ( randomResult )
	{
		std::vector<int> potionChances =
		{
			0,	//POTION_WATER,
			1,	//POTION_BOOZE,
			1,	//POTION_JUICE,
			1,	//POTION_SICKNESS,
			1,	//POTION_CONFUSION,
			1,	//POTION_EXTRAHEALING,
			1,	//POTION_HEALING,
			1,	//POTION_CUREAILMENT,
			1,	//POTION_BLINDNESS,
			1,	//POTION_RESTOREMAGIC,
			1,	//POTION_INVISIBILITY,
			1,	//POTION_LEVITATION,
			1,	//POTION_SPEED,
			1,	//POTION_ACID,
			1,	//POTION_PARALYSIS,
			0,	//POTION_POLYMORPH
			0,	//POTION_FIRESTORM
			0,	//POTION_ICESTORM
			0	//POTION_THUNDERSTORM
		};
		std::discrete_distribution<> potionDistribution(potionChances.begin(), potionChances.end());
		auto generatedPotion = potionStandardAppearanceMap.at(potionDistribution(fountainSeed));
		result = static_cast<ItemType>(generatedPotion.first);
	}

	if ( basePotion->identified && secondaryPotion->identified )
	{
		messagePlayerColor(gui_player, MESSAGE_INVENTORY, uint32ColorWhite, language[3332],
			items[basePotion->type].name_identified, items[secondaryPotion->type].name_identified);
	}
	else if ( basePotion->identified )
	{
		messagePlayerColor(gui_player, MESSAGE_INVENTORY, uint32ColorWhite, language[3334],
			items[basePotion->type].name_identified);
	}
	else if ( secondaryPotion->identified )
	{
		messagePlayerColor(gui_player, MESSAGE_INVENTORY, uint32ColorWhite, language[3333],
			items[secondaryPotion->type].name_identified);
	}
	else
	{
		messagePlayerColor(gui_player, MESSAGE_INVENTORY, uint32ColorWhite, language[3335]);
	}

	if ( !explodeSelf && result != POTION_SICKNESS && !tryDuplicatePotion && !samePotion )
	{
		if ( !(alchemyLearnRecipe(basePotion->type, true)) )
		{
			if ( secondaryPotion->identified )
			{
				alchemyLearnRecipe(secondaryPotion->type, true);
			}
		}
	}

	bool degradeAlembic = false;
	if ( explodeSelf )
	{
		degradeAlembic = true;
	}
	else
	{
		if ( basePotion->type == POTION_ACID || secondaryPotion->type == POTION_ACID )
		{
			if ( rand() % 5 == 0 )
			{
				degradeAlembic = true;
			}
		}
		else
		{
			if ( rand() % 20 == 0 )
			{
				degradeAlembic = true;
			}
		}
	}

	int appearance = -1;
	int blessing = 0;
	if ( basePotion->beatitude > 0 && secondaryPotion->beatitude > 0 )
	{
		blessing = std::min(basePotion->beatitude, secondaryPotion->beatitude); // take least blessed
	}
	else if ( basePotion->beatitude < 0 && secondaryPotion->beatitude < 0 )
	{
		blessing = std::min(basePotion->beatitude, secondaryPotion->beatitude); // take most cursed
	}
	else if ( (basePotion->beatitude < 0 && secondaryPotion->beatitude > 0)
		|| (secondaryPotion->beatitude < 0 && basePotion->beatitude > 0) )
	{
		blessing = 0;
	}
	else if ( basePotion->beatitude < 0 && secondaryPotion->beatitude == 0 )
	{
		blessing = basePotion->beatitude; // curse the result
	}
	else if ( basePotion->beatitude == 0 && secondaryPotion->beatitude < 0 )
	{
		blessing = secondaryPotion->beatitude; // curse the result
	}
	else if ( basePotion->beatitude > 0 && secondaryPotion->beatitude == 0 )
	{
		blessing = 0; // negate the blessing
	}
	else if ( basePotion->beatitude == 0 && secondaryPotion->beatitude > 0 )
	{
		blessing = 0; // negate the blessing
	}
	if ( samePotion )
	{
		// same potion, keep the first potion only.
		result = basePotion->type;
		if ( basePotion->beatitude == secondaryPotion->beatitude )
		{
			blessing = basePotion->beatitude;
		}
		appearance = basePotion->appearance;
		status = basePotion->status;
	}
	else if ( tryDuplicatePotion && !duplicateSucceed )
	{
		if ( basePotion->type == POTION_WATER )
		{
			blessing = basePotion->beatitude;
		}
		else if ( secondaryPotion->type == POTION_WATER )
		{
			blessing = secondaryPotion->beatitude;
		}
	}

	bool knewBothBaseIngredients = true; // always auto ID?
	//if ( clientLearnedAlchemyIngredients[gui_player].find(basePotion->type) 
	//	!= clientLearnedAlchemyIngredients[gui_player].end() )
	//{
	//	if ( clientLearnedAlchemyIngredients[gui_player].find(secondaryPotion->type)
	//		!= clientLearnedAlchemyIngredients[gui_player].end() )
	//	{
	//		// knew about both combinations.
	//		if ( !tryDuplicatePotion && !explodeSelf && result != POTION_SICKNESS )
	//		{
	//			if ( skillLVL >= 3 )
	//			{
	//				knewBothBaseIngredients = true; // auto ID the new potion.
	//			}
	//		}
	//	}
	//}

	Item* duplicatedPotion = nullptr;
	bool emptyBottle = false;
	if ( duplicateSucceed )
	{
		if ( basePotion->type == POTION_WATER )
		{
			consumeItem(basePotion, gui_player);
			duplicatedPotion = secondaryPotion;
		}
		else if ( secondaryPotion->type == POTION_WATER )
		{
			consumeItem(secondaryPotion, gui_player);
			duplicatedPotion = basePotion;
		}
	}
	else
	{
		if ( tryDuplicatePotion )
		{
			if ( basePotion->type != POTION_WATER )
			{
				consumeItem(basePotion, gui_player);
			}
			else if ( secondaryPotion->type != POTION_WATER )
			{
				consumeItem(secondaryPotion, gui_player);
			}
		}
		else
		{
			if ( !samePotion )
			{
				consumeItem(basePotion, gui_player);
				consumeItem(secondaryPotion, gui_player);
			}
		}
		if ( rand() % 100 < (50 + skillLVL * 5) ) // 50 - 75% chance
		{
			emptyBottle = true;
		}
	}

	// calc blessings
	if ( !tryDuplicatePotion && !samePotion )
	{
		if ( alembicItem )
		{
			if ( alembicItem->beatitude >= 1 )
			{
				blessing = 1;
			}
			else if ( alembicItem->beatitude <= -1 )
			{
				blessing = alembicItem->beatitude;
			}
		}

		if ( skillCapstoneUnlocked(gui_player, PRO_ALCHEMY) )
		{
			blessing = 2;
			if ( alembicItem )
			{
				if ( alembicItem->beatitude <= -1 )
				{
					blessing = std::min(-2, (int)alembicItem->beatitude);
				}
				else
				{
					blessing = std::max(2, (int)alembicItem->beatitude);
				}
			}
			degradeAlembic = false;
		}
	}

	if ( degradeAlembic && alembicItem )
	{
		alembicItem->status = static_cast<Status>(alembicItem->status - 1);
		if ( alembicItem->status > BROKEN )
		{
			messagePlayer(gui_player, MESSAGE_MISC, language[681], alembicItem->getName());
		}
		else
		{
			messagePlayer(gui_player, MESSAGE_MISC, language[2351], alembicItem->getName());
			playSoundPlayer(gui_player, 162, 64);
			consumeItem(alembicItem, gui_player);
			alembicItem = nullptr;
		}
	}

	if ( explodeSelf && players[gui_player] && players[gui_player]->entity )
	{
		// hurt.
		alchemyAddRecipe(gui_player, basePotionType, secondaryPotionType, TOOL_BOMB, true);
		if ( multiplayer == CLIENT )
		{
			strcpy((char*)net_packet->data, "BOOM");
			net_packet->data[4] = gui_player;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 5;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
		else
		{
			spawnMagicTower(nullptr, players[gui_player]->entity->x, players[gui_player]->entity->y, SPELL_FIREBALL, nullptr);
			players[gui_player]->entity->setObituary(language[3350]);
		    stats[gui_player]->killer = KilledBy::FAILED_ALCHEMY;
		}
		closeGUI();
		return;
	}

	for ( auto it = potionStandardAppearanceMap.begin(); it != potionStandardAppearanceMap.end(); ++it )
	{
		if ( (*it).first == result )
		{
			if ( appearance == -1 )
			{
				appearance = (*it).second;
			}
			bool raiseSkill = true;
			if ( result == POTION_SICKNESS )
			{
				appearance = 0 + rand() % items[POTION_SICKNESS].variations;
				if ( rand() % 10 > 0 )
				{
					raiseSkill = false;
				}
			}
			else if ( result == POTION_WATER )
			{
				raiseSkill = false;
			}
			else if ( duplicateSucceed )
			{
				if ( rand() % 10 > 0 )
				{
					raiseSkill = false;
				}
			}

			int count = 1;
			if ( samePotion )
			{
				count = 2;
			}
			appearance = std::max(0, appearance);
			Item* newPotion = newItem(result, status, blessing, count, appearance, knewBothBaseIngredients, nullptr);
			if ( tryDuplicatePotion )
			{
				if ( result == POTION_WATER && !duplicateSucceed )
				{
					messagePlayer(gui_player, MESSAGE_MISC, language[3356]);
				}
				else if ( newPotion )
				{
					if ( duplicatedPotion )
					{
						newPotion->appearance = duplicatedPotion->appearance;
						newPotion->beatitude = duplicatedPotion->beatitude;
						newPotion->identified = duplicatedPotion->identified;
						newPotion->status = duplicatedPotion->status;
					}
					messagePlayer(gui_player, MESSAGE_MISC, language[3352], newPotion->description());
				}
			}
			else
			{
				messagePlayer(gui_player, MESSAGE_MISC, language[3352], newPotion->description());
				steamStatisticUpdate(STEAM_STAT_IN_THE_MIX, STEAM_STAT_INT, 1);
			}

			if ( newPotion )
			{
				Item* pickedUp = itemPickup(gui_player, newPotion);
				if ( samePotion )
				{
					if ( pickedUp != basePotion && pickedUp != secondaryPotion
						&& pickedUp && pickedUp->count > 2 )
					{
						// if we consume one of these slots fully, new potion takes its place
						if ( basePotion->count == 1 ) 
						{
							pickedUp->x = basePotion->x;
							pickedUp->y = basePotion->y;
						}
						else if ( secondaryPotion->count == 1 )
						{
							pickedUp->x = secondaryPotion->x;
							pickedUp->y = secondaryPotion->y;
						}
					}
					consumeItem(basePotion, gui_player);
					consumeItem(secondaryPotion, gui_player);
				}
				if ( pickedUp )
				{
					alchemyGUI.potionResultUid = pickedUp->uid;
					alchemyGUI.animPotionResultCount = alchemyGUI.alchemyResultPotion.count;
					if ( alchemyGUI.animPotionResultCount == 2 && tryDuplicatePotion && !duplicateSucceed )
					{
						alchemyGUI.animPotionResultCount = 1;
					}
					if ( !tryDuplicatePotion && !randomResult )
					{
						alchemyAddRecipe(gui_player, basePotionType, secondaryPotionType, pickedUp->type);
					}
					if ( tryDuplicatePotion )
					{
						if ( result == POTION_WATER && !duplicateSucceed )
						{
							if ( basePotionType == POTION_WATER )
							{
								consumeItem(basePotion, gui_player);
							}
							else if ( secondaryPotionType == POTION_WATER )
							{
								consumeItem(secondaryPotion, gui_player);
							}
						}
					}
				}
				free(newPotion);
				newPotion = nullptr;
			}
			if ( players[gui_player] && players[gui_player]->entity )
			{
				playSoundEntityLocal(players[gui_player]->entity, 401, 64);
			}
			if ( samePotion )
			{
				raiseSkill = false;
			}
			if ( emptyBottle )
			{
				Item* emptyBottle = newItem(POTION_EMPTY, SERVICABLE, 0, 1, 0, true, nullptr);
				itemPickup(gui_player, emptyBottle);
				messagePlayer(gui_player, MESSAGE_MISC, language[3351], items[POTION_EMPTY].name_identified);
				free(emptyBottle);
			}
			if ( raiseSkill && rand() % 2 == 0 )
			{
				if ( multiplayer == CLIENT )
				{
					// request level up
					strcpy((char*)net_packet->data, "CSKL");
					net_packet->data[4] = gui_player;
					net_packet->data[5] = PRO_ALCHEMY;
					net_packet->address.host = net_server.host;
					net_packet->address.port = net_server.port;
					net_packet->len = 6;
					sendPacketSafe(net_sock, -1, net_packet, 0);
				}
				else
				{
					if ( players[gui_player] && players[gui_player]->entity )
					{
						players[gui_player]->entity->increaseSkill(PRO_ALCHEMY);
					}
				}
			}
			break;
		}
	}

	//basePotion = nullptr;
	//secondaryPotion = nullptr;
	//closeGUI();
}

bool GenericGUIMenu::alchemyLearnRecipe(int type, bool increaseskill, bool notify)
{
	int index = 0;
	for ( auto it = potionStandardAppearanceMap.begin(); it != potionStandardAppearanceMap.end(); ++it )
	{
		// loop through to get the index number to insert into gameStatistics[STATISTICS_ALCHEMY_RECIPES]
		if ( (*it).first == type )
		{
			if ( clientLearnedAlchemyIngredients[gui_player].find(type) 
				== clientLearnedAlchemyIngredients[gui_player].end() )
			{
				// new recipe!
				clientLearnedAlchemyIngredients[gui_player].insert(type);
				Uint32 color = makeColorRGB(0, 255, 0);
				if ( notify )
				{
					if ( isItemBaseIngredient(type) )
					{
						messagePlayerColor(gui_player, MESSAGE_PROGRESSION, color, language[3346], items[type].name_identified);
					}
					else if ( isItemSecondaryIngredient(type) )
					{
						messagePlayerColor(gui_player, MESSAGE_PROGRESSION, color, language[3349], items[type].name_identified);
					}
					std::string itemName = items[type].name_identified;
					size_t pos = std::string::npos;
					for ( auto& potionName : Player::SkillSheet_t::skillSheetData.potionNamesToFilter )
					{
						if ( (pos = itemName.find(potionName)) != std::string::npos )
						{
							itemName.erase(pos, potionName.length());
						}
					}
					capitalizeString(itemName);
					std::string iconPath = "";
					node_t* imagePathsNode = nullptr;
					Uint32 index = (*it).second % items[type].variations;
					imagePathsNode = list_Node(&items[type].images, index);
					if ( imagePathsNode )
					{
						string_t* imagePath = static_cast<string_t*>(imagePathsNode->element);
						iconPath = imagePath->data;
					}
					alchemyGUI.notifications.push_back(std::make_pair(ticks, 
						AlchemyGUI_t::AlchNotification_t(language[4180], itemName, iconPath)));
				}
				if ( increaseskill )
				{
					if ( multiplayer == CLIENT )
					{
						// request level up
						strcpy((char*)net_packet->data, "CSKL");
						net_packet->data[4] = gui_player;
						net_packet->data[5] = PRO_ALCHEMY;
						net_packet->address.host = net_server.host;
						net_packet->address.port = net_server.port;
						net_packet->len = 6;
						sendPacketSafe(net_sock, -1, net_packet, 0);
					}
					else
					{
						if ( players[gui_player] && players[gui_player]->entity )
						{
							players[gui_player]->entity->increaseSkill(PRO_ALCHEMY);
						}
					}
				}
				gameStatistics[STATISTICS_ALCHEMY_RECIPES] |= (1 << index);
				return true;
			}
			else
			{
				// store the potion index into here for game saves, just in case we don't have it set the element in anyway.
				gameStatistics[STATISTICS_ALCHEMY_RECIPES] |= (1 << index); 
				if ( increaseskill && rand() % 6 == 0 )
				{
					if ( multiplayer == CLIENT )
					{
						// request level up
						strcpy((char*)net_packet->data, "CSKL");
						net_packet->data[4] = gui_player;
						net_packet->data[5] = PRO_ALCHEMY;
						net_packet->address.host = net_server.host;
						net_packet->address.port = net_server.port;
						net_packet->len = 6;
						sendPacketSafe(net_sock, -1, net_packet, 0);
					}
					else
					{
						if ( players[gui_player] && players[gui_player]->entity )
						{
							players[gui_player]->entity->increaseSkill(PRO_ALCHEMY);
						}
					}
				}
			}
			break;
		}
		++index;
	}
	return false;
}

bool GenericGUIMenu::isItemBaseIngredient(int type)
{
	switch ( type )
	{
		case POTION_WATER:
		case POTION_POLYMORPH:
		case POTION_BOOZE:
		case POTION_JUICE:
		case POTION_ACID:
		case POTION_INVISIBILITY:
			return true;
		default:
			return false;
			break;
	}
	return false;
}

bool GenericGUIMenu::isItemSecondaryIngredient(int type)
{
	switch ( type )
	{
		case POTION_WATER:
		case POTION_SICKNESS:
		case POTION_CONFUSION:
		case POTION_CUREAILMENT:
		case POTION_BLINDNESS:
		case POTION_RESTOREMAGIC:
		case POTION_SPEED:
		case POTION_POLYMORPH:
			return true;
		default:
			return false;
			break;
	}
	return false;
}

void GenericGUIMenu::alchemyLearnRecipeOnLevelUp(int skill)
{
	bool learned = false;
	if ( skill > 0 )
	{
		ItemType potion = POTION_WATER;
		learned = alchemyLearnRecipe(potion, false);
	}
	
	if ( skill == 20 )
	{
		ItemType potion = POTION_JUICE;
		learned = alchemyLearnRecipe(potion, false);
		potion = POTION_BOOZE;
		learned = alchemyLearnRecipe(potion, false);
	}
	else if ( skill == 40 )
	{
		ItemType potion = POTION_ACID;
		learned = alchemyLearnRecipe(potion, false);
	}
	else if ( skill == 60 )
	{
		ItemType potion = POTION_INVISIBILITY;
		learned = alchemyLearnRecipe(potion, false);
		potion = POTION_POLYMORPH;
		learned = alchemyLearnRecipe(potion, false);
	}

	if ( !learned && skill % 5 == 0 )
	{
		ItemType potion = itemLevelCurve(POTION, 0, currentlevel);
		alchemyLearnRecipe(potion, false);
	}
}

void GenericGUIMenu::tinkeringCreateCraftableItemList()
{
	tinkeringFreeLists();
	/*Item* tempItem = newItem(TOOL_BOMB, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, nullptr);
	if ( tinkeringPlayerCanAffordCraft(tempItem) )
	{
	}*/
	std::vector<Item*> items;
	items.push_back(newItem(TOOL_BOMB, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 0;
	items[items.size() - 1]->y = 0;
	items.push_back(newItem(TOOL_FREEZE_BOMB, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 1;
	items[items.size() - 1]->y = 0;
	items.push_back(newItem(TOOL_SLEEP_BOMB, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 2;
	items[items.size() - 1]->y = 0;
	items.push_back(newItem(TOOL_TELEPORT_BOMB, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 3;
	items[items.size() - 1]->y = 0;
	items.push_back(newItem(TOOL_DUMMYBOT, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 4;
	items[items.size() - 1]->y = 0;
	items.push_back(newItem(TOOL_DECOY, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 0;
	items[items.size() - 1]->y = 1;
	items.push_back(newItem(TOOL_GYROBOT, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 1;
	items[items.size() - 1]->y = 1;
	items.push_back(newItem(TOOL_SENTRYBOT, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 2;
	items[items.size() - 1]->y = 1;
	items.push_back(newItem(TOOL_SPELLBOT, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 3;
	items[items.size() - 1]->y = 1;
	items.push_back(newItem(TOOL_BEARTRAP, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 4;
	items[items.size() - 1]->y = 1;
	items.push_back(newItem(CLOAK_BACKPACK, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 0;
	items[items.size() - 1]->y = 2;
	items.push_back(newItem(TOOL_ALEMBIC, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 1;
	items[items.size() - 1]->y = 2;
	items.push_back(newItem(TOOL_LOCKPICK, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 2;
	items[items.size() - 1]->y = 2;
	items.push_back(newItem(TOOL_GLASSES, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 3;
	items[items.size() - 1]->y = 2;
	items.push_back(newItem(TOOL_LANTERN, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 4;
	items[items.size() - 1]->y = 2;
	items.push_back(newItem(POTION_EMPTY, SERVICABLE, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = -1;
	items[items.size() - 1]->y = -1;
	for ( auto it = items.begin(); it != items.end(); ++it )
	{
		Item* item = *it;
		if ( item )
		{
			int skillLVL = 0;
			int requiredSkill = tinkeringPlayerHasSkillLVLToCraft(item);
			if ( stats[gui_player] && players[gui_player] )
			{
				skillLVL = (stats[gui_player]->PROFICIENCIES[PRO_LOCKPICKING] + statGetPER(stats[gui_player], players[gui_player]->entity)) / 20; // 0 to 5
				skillLVL = std::min(skillLVL, 5);
			}
			if ( item->type == TOOL_DUMMYBOT || item->type == TOOL_SENTRYBOT
				|| item->type == TOOL_SPELLBOT || item->type == TOOL_GYROBOT )
			{
				if ( skillLVL >= 5 ) // maximum
				{
					item->status = EXCELLENT;
				}
				else if ( requiredSkill >= skillLVL || requiredSkill == -1 )
				{
					item->status = DECREPIT;
				}
				else
				{
					if ( skillLVL - requiredSkill == 1 )
					{
						item->status = WORN;
					}
					else if ( skillLVL - requiredSkill == 2 )
					{
						item->status = SERVICABLE;
					}
					else if ( skillLVL - requiredSkill >= 3 )
					{
						item->status = EXCELLENT;
					}
				}
			}
		}
	}
	//newItem(TOOL_BEARTRAP, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems);
	//newItem(TOOL_DETONATOR_CHARGE, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems);

	//messagePlayer(gui_player, "asserting craftable num items: %d", list_Size(&tinkeringTotalItems));
	if ( stats[gui_player] )
	{
		// make the last node jump to the player's actual items, 
		// so consuming the items in this list will actually update the player's inventory.
		node_t* tinkeringTotalLastCraftableNode = tinkeringTotalItems.last;
		if ( tinkeringTotalLastCraftableNode )
		{
			tinkeringTotalLastCraftableNode->next = stats[gui_player]->inventory.first;
		}
		//messagePlayer(gui_player, "asserting total list size: %d", list_Size(&tinkeringTotalItems));
	}
}

void GenericGUIMenu::tinkeringFreeLists()
{
	node_t* nextnode = nullptr;
	int itemcnt = 0;

	// totalItems is a unique list, contains unique craftable data, 
	// as well as a pointer to continue to the player's inventory
	for ( node_t* node = tinkeringTotalItems.first; node; node = nextnode )
	{
		nextnode = node->next;
		if ( node->list == &tinkeringTotalItems )
		{
			list_RemoveNode(node);
			++itemcnt;
		}
		else if ( node->list == &stats[gui_player]->inventory )
		{
			//messagePlayer(gui_player, "reached inventory after clearing %d items", itemcnt);
			break;
		}
	}
	tinkeringMetalScrap.clear();
	tinkeringMagicScrap.clear();
	tinkeringTotalItems.first = nullptr;
	tinkeringTotalItems.last = nullptr;
	tinkeringTotalLastCraftableNode = nullptr;
}

bool GenericGUIMenu::tinkeringCraftItem(Item* item)
{
	if ( !item )
	{
		return false;
	}

	// add checks/consuming of items here.
	if ( tinkeringPlayerHasSkillLVLToCraft(item) == -1 )
	{
		//playSound(90, 64);
		messagePlayer(gui_player, MESSAGE_MISC, language[3652], items[item->type].name_identified);
		return false;
	}
	if ( !tinkeringPlayerCanAffordCraft(item) )
	{
		//playSound(90, 64);
		messagePlayer(gui_player, MESSAGE_MISC, language[3648], items[item->type].name_identified);
		return false;
	}

	Item* crafted = tinkeringCraftItemAndConsumeMaterials(item);
	if ( crafted )
	{
		Item* pickedUp = itemPickup(gui_player, crafted);
		messagePlayer(gui_player, MESSAGE_MISC, language[3668], crafted->description());
		free(crafted);
		return true;
	}
	return false;
}

bool GenericGUIMenu::tinkeringSalvageItem(Item* item, bool outsideInventory, int player)
{
	if ( !item )
	{
		return false;
	}

	if ( !outsideInventory && itemIsEquipped(item, player) )
	{
		tinkeringBulkSalvage = false;
		messagePlayer(player, MESSAGE_MISC, language[3669]);
		return false; // don't want to deal with client/server desync problems here.
	}

	// add checks/consuming of items here.
	int metal = 0;
	int magic = 0;
	tinkeringGetItemValue(item, &metal, &magic);
	bool didCraft = false;
	int skillLVL = 0;
	int bonusMetalScrap = 0;
	int bonusMagicScrap = 0;
	if ( stats[player] && players[player] && item->status > BROKEN )
	{
   		skillLVL = (stats[player]->PROFICIENCIES[PRO_LOCKPICKING] + statGetPER(stats[player], players[player]->entity)) / 20;
		skillLVL = std::min(skillLVL, 5);
		switch ( skillLVL )
		{
			case 5:
				bonusMetalScrap = (1 + ((rand() % 2 == 0) ? 1 : 0)) * metal; // 2x or 50% 3x extra scrap
				bonusMagicScrap = (1 + ((rand() % 2 == 0) ? 1 : 0)) * magic; // 2x or 50% 3x extra scrap
				break;
			case 4:
				bonusMetalScrap = (1 + ((rand() % 4 == 0) ? 1 : 0)) * metal; // 2x or 25% 3x extra scrap
				bonusMagicScrap = (1 + ((rand() % 4 == 0) ? 1 : 0)) * magic; // 2x or 25% 3x extra scrap
				break;
			case 3:
				bonusMetalScrap = ((rand() % 2 == 0) ? 1 : 0) * metal; // 50% 2x scrap value
				bonusMagicScrap = ((rand() % 2 == 0) ? 1 : 0) * magic; // 50% 2x scrap value
				break;
			case 2:
				bonusMetalScrap = ((rand() % 4 == 0) ? 1 : 0) * metal; // 25% 2x scrap value
				bonusMagicScrap = ((rand() % 4 == 0) ? 1 : 0) * magic; // 25% 2x scrap value
				break;
			case 1:
				bonusMetalScrap = ((rand() % 8 == 0) ? 1 : 0) * metal; // 12.5% 2x scrap value
				bonusMagicScrap = ((rand() % 8 == 0) ? 1 : 0) * magic; // 12.5% 2x scrap value
				break;
			default:
				break;
		}
		int metalCraftCost = 0;
		int magicCraftCost = 0;
		tinkeringGetCraftingCost(item, &metalCraftCost, &magicCraftCost);
		// prevent bonus scrap being more than the amount required to create.
		if ( metalCraftCost > 0 )
		{
			if ( metal > 0 && bonusMetalScrap > 0 )
			{
				if ( (metal + bonusMetalScrap) > metalCraftCost )
				{
					bonusMetalScrap = std::max(0, (metalCraftCost - metal));
				}
			}
		}
		if ( magicCraftCost > 0 )
		{
			if ( magic > 0 && bonusMagicScrap > 0 )
			{
				if ( (magic + bonusMagicScrap) > magicCraftCost )
				{
					bonusMagicScrap = std::max(0, (magicCraftCost - magic));
				}
			}
		}
	}
	if ( metal > 0 )
	{
		metal += bonusMetalScrap;
	}
	if ( magic > 0 )
	{
		magic += bonusMagicScrap;
	}
	if ( metal > 0 )
	{
		Item* crafted = newItem(TOOL_METAL_SCRAP, DECREPIT, 0, metal, 0, true, nullptr);
		if ( crafted )
		{
			Item* pickedUp = itemPickup(player, crafted);
			if ( !tinkeringBulkSalvage )
			{
				if ( bonusMetalScrap > 0 )
				{
					Uint32 color = makeColorRGB(0, 255, 0);
					messagePlayerColor(player, MESSAGE_INVENTORY, color, language[3665], metal + tinkeringBulkSalvageMetalScrap, items[pickedUp->type].name_identified);
				}
				else
				{
					messagePlayer(player, MESSAGE_MISC, language[3665], metal + tinkeringBulkSalvageMetalScrap, items[pickedUp->type].name_identified);
				}
			}
			else
			{
				tinkeringBulkSalvageMetalScrap += metal;
			}
			free(crafted); // if player != clientnum, then crafted == pickedUp
			didCraft = true;
		}
	}
	if ( magic > 0 )
	{
		Item* crafted = newItem(TOOL_MAGIC_SCRAP, DECREPIT, 0, magic, 0, true, nullptr);
		if ( crafted )
		{
			Item* pickedUp = itemPickup(player, crafted);
			if ( !tinkeringBulkSalvage )
			{
				if ( bonusMagicScrap > 0 )
				{
					Uint32 color = makeColorRGB(0, 255, 0);
					messagePlayerColor(player, MESSAGE_INVENTORY, color, language[3665], magic + tinkeringBulkSalvageMagicScrap, items[pickedUp->type].name_identified);
				}
				else
				{
					messagePlayer(player, MESSAGE_MISC, language[3665], magic + tinkeringBulkSalvageMagicScrap, items[pickedUp->type].name_identified);
				}
			}
			else
			{
				tinkeringBulkSalvageMagicScrap += magic;
			}
			free(crafted); // if player != clientnum, then crafted == pickedUp
			didCraft = true;
		}
	}

	bool increaseSkill = false;
	if ( stats[player] && didCraft )
	{
		if ( metal >= 4 || magic >= 4 )
		{
			if ( rand() % 2 == 0 ) // 50%
			{
				if ( stats[player]->PROFICIENCIES[PRO_LOCKPICKING] < SKILL_LEVEL_EXPERT )
				{
					increaseSkill = true;
				}
				else if ( rand() % 20 == 0 && !tinkeringBulkSalvage )
				{
					messagePlayer(player, MESSAGE_MISC, language[3666]); // nothing left to learn from salvaging.
				}
			}
		}
		else if ( metal >= 2 || magic >= 2 )
		{
			if ( rand() % 5 == 0 ) // 20%
			{
				if ( stats[player]->PROFICIENCIES[PRO_LOCKPICKING] < SKILL_LEVEL_EXPERT )
				{
					increaseSkill = true;
				}
				else if ( rand() % 20 == 0 && !tinkeringBulkSalvage )
				{
					messagePlayer(player, MESSAGE_MISC, language[3666]); // nothing left to learn from salvaging.
				}
			}
		}

		if ( item->type == TOOL_TORCH || item->type == TOOL_CRYSTALSHARD )
		{
			achievementObserver.playerAchievements[player].torchererScrap += (magic + metal);
		}
		else if ( item->type == TOOL_SENTRYBOT || item->type == TOOL_SPELLBOT
			|| item->type == TOOL_GYROBOT || item->type == TOOL_DUMMYBOT 
			|| item->type == TOOL_DETONATOR_CHARGE )
		{
			if ( item->status == BROKEN )
			{
				achievementObserver.playerAchievements[player].fixerUpper += 1;
			}
		}

		achievementObserver.playerAchievements[player].superShredder += (magic + metal);

		if ( players[player] && players[player]->entity && !tinkeringBulkSalvage )
		{
			if ( (ticks - tinkeringSfxLastTicks) > 200 && ((metal >= 4 || magic >= 4) || rand() % 5 == 0) )
			{
				tinkeringSfxLastTicks = ticks;
				playSoundEntity(players[player]->entity, 421 + (rand() % 2) * 3, 64);
			}
			else
			{
				if ( rand() % 4 == 0 )
				{
					playSoundEntity(players[player]->entity, 35 + rand() % 3, 64);
				}
				else
				{
					playSoundEntity(players[player]->entity, 462 + rand() % 2, 64);
				}
			}
		}
	}

	if ( increaseSkill )
	{
		if ( player != gui_player ) // server initiated craft for client.
		{
			if ( players[player] && players[player]->entity )
			{
				players[player]->entity->increaseSkill(PRO_LOCKPICKING);
			}
		}
		else if ( players[player]->isLocalPlayer() ) // client/server initiated craft for self.
		{
			if ( multiplayer == CLIENT )
			{
				// request level up
				strcpy((char*)net_packet->data, "CSKL");
				net_packet->data[4] = player;
				net_packet->data[5] = PRO_LOCKPICKING;
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 6;
				sendPacketSafe(net_sock, -1, net_packet, 0);
			}
			else
			{
				if ( players[player] && players[player]->entity )
				{
					players[player]->entity->increaseSkill(PRO_LOCKPICKING);
				}
			}
		}
	}

	if ( !outsideInventory && didCraft )
	{
		consumeItem(item, player);
	}
	if ( !didCraft )
	{
		tinkeringBulkSalvage = false;
	}
	return true;
}

bool GenericGUIMenu::isNodeFromPlayerInventory(node_t* node)
{
	if ( stats[gui_player] && node )
	{
		return (node->list == &stats[gui_player]->inventory);
	}
	return false;
}

bool GenericGUIMenu::isItemSalvageable(const Item* item, int player)
{
	if ( !item )
	{
		return false;
	}
	/*if ( itemIsConsumableByAutomaton(*item) )
	{
		return false;
	}*/
	if ( player == gui_player && isNodeFromPlayerInventory(item->node) && itemIsEquipped(item, gui_player) )
	{
		return false;
	}
	if ( item == tinkeringKitItem )
	{
		return false;
	}
	if ( item->type == TOOL_METAL_SCRAP || item->type == TOOL_MAGIC_SCRAP )
	{
		return false;
	}

	int metal = 0;
	int magic = 0;
	if ( tinkeringGetItemValue(item, &metal, &magic) )
	{
		return true;
	}
	return false;
}

bool GenericGUIMenu::tinkeringIsItemRepairable(Item* item, int player)
{
	if ( !item )
	{
		return false;
	}
	/*if ( player == gui_player && itemIsEquipped(item, gui_player) )
	{
		return false;
	}*/
	/*if ( item == tinkeringKitItem )
	{
		return false;
	}*/

	int metal = 0;
	int magic = 0;
	if ( tinkeringGetRepairCost(item, &metal, &magic) )
	{
		return true;
	}
	return false;
}

bool GenericGUIMenu::tinkeringPlayerCanAffordRepair(Item* item)
{
	if ( !item )
	{
		return false;
	}
	int metal = 0;
	int magic = 0;
	tinkeringGetRepairCost(item, &metal, &magic);
	if ( metal == 0 && magic == 0 )
	{
		return false;
	}

	if ( tinkeringPlayerHasMaterialsInventory(metal, magic) )
	{
		return true;
	}

	return false;
}

bool GenericGUIMenu::tinkeringPlayerCanAffordCraft(const Item* item)
{
	if ( !item )
	{
		return false;
	}
	int metal = 0;
	int magic = 0;
	tinkeringGetCraftingCost(item, &metal, &magic);
	if ( metal == 0 && magic == 0 )
	{
		return false;
	}

	if ( tinkeringPlayerHasMaterialsInventory(metal, magic) )
	{
		return true;
	}
	
	return false;
}

Item* GenericGUIMenu::tinkeringCraftItemAndConsumeMaterials(const Item* item)
{
	if ( !item )
	{
		return nullptr;
	}
	int metal = 0;
	int magic = 0;
	tinkeringGetCraftingCost(item, &metal, &magic);
	if ( metal == 0 && magic == 0 )
	{
		return nullptr;
	}
	if ( tinkeringPlayerHasMaterialsInventory(metal, magic) )
	{
		bool increaseSkill = false;
		if ( stats[gui_player] )
		{
			if ( metal > 4 || magic > 4 )
			{
				if ( rand() % 10 == 0 )
				{
					increaseSkill = true;
				}
			}
			else
			{
				if ( metal > 2 || magic > 2 )
				{
					if ( rand() % 20 == 0 )
					{
						increaseSkill = true;
					}
				}
				else
				{
					if ( stats[gui_player]->PROFICIENCIES[PRO_LOCKPICKING] < SKILL_LEVEL_BASIC )
					{
						if ( rand() % 10 == 0 )
						{
							increaseSkill = true;
						}
					}
					else if ( rand() % 20 == 0 )
					{
						messagePlayer(gui_player, MESSAGE_MISC, language[3667], items[item->type].name_identified);
					}
				}
			}
		}
			
		if ( increaseSkill )
		{
			if ( multiplayer == CLIENT )
			{
				// request level up
				strcpy((char*)net_packet->data, "CSKL");
				net_packet->data[4] = gui_player;
				net_packet->data[5] = PRO_LOCKPICKING;
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 6;
				sendPacketSafe(net_sock, -1, net_packet, 0);
			}
			else
			{
				if ( players[gui_player] && players[gui_player]->entity )
				{
					players[gui_player]->entity->increaseSkill(PRO_LOCKPICKING);
				}
			}
		}

		if ( (ticks - tinkeringSfxLastTicks) > 100 )
		{
			tinkeringSfxLastTicks = ticks;
			if ( itemIsThrowableTinkerTool(item) )
			{
				playSoundEntity(players[gui_player]->entity, 459 + (rand() % 3), 92);
			}
			else
			{
				if ( rand() % 3 == 0 )
				{
					playSoundEntity(players[gui_player]->entity, 422 + (rand() % 2), 92);
				}
				else
				{
					playSoundEntity(players[gui_player]->entity, 35 + rand() % 3, 64);
				}
			}
		}
		else
		{
			playSoundEntity(players[gui_player]->entity, 35 + rand() % 3, 64);
		}

		for ( int c = 0; c < metal; ++c )
		{
			Item* item = uidToItem(tinkeringRetrieveLeastScrapStack(TOOL_METAL_SCRAP));
			if ( item )
			{
				consumeItem(item, gui_player);
			}
		}
		for ( int c = 0; c < magic; ++c )
		{
			Item* item = uidToItem(tinkeringRetrieveLeastScrapStack(TOOL_MAGIC_SCRAP));
			if ( item )
			{
				consumeItem(item, gui_player);
			}
		}
		if ( tinkeringKitRollIfShouldBreak() )
		{
			tinkeringKitDegradeOnUse(gui_player);
		}
		return newItem(item->type, item->status, item->beatitude, 1, ITEM_TINKERING_APPEARANCE, true, nullptr);
	}
	return nullptr;
}

Uint32 GenericGUIMenu::tinkeringRetrieveLeastScrapStack(int type)
{
	if ( type == TOOL_METAL_SCRAP )
	{
		if ( !tinkeringMetalScrap.empty() )
		{
			int lowestCount = 9999;
			Uint32 lowestUid = 0;
			for ( auto it : tinkeringMetalScrap )
			{
				Item* item = uidToItem(it);
				if ( item && item->count > 0 && item->count < lowestCount )
				{
					lowestCount = item->count;
					lowestUid = it;
				}
			}
			return lowestUid;
		}
	}
	else if ( type == TOOL_MAGIC_SCRAP )
	{
		if ( !tinkeringMagicScrap.empty() )
		{
			int lowestCount = 9999;
			Uint32 lowestUid = 0;
			for ( auto it : tinkeringMagicScrap )
			{
				Item* item = uidToItem(it);
				if ( item && item->count > 0 && item->count < lowestCount )
				{
					lowestCount = item->count;
					lowestUid = it;
				}
			}
			return lowestUid;
		}
	}
	return 0;
}

int GenericGUIMenu::tinkeringCountScrapTotal(int type)
{
	int count = 0;
	if ( type == TOOL_METAL_SCRAP )
	{
		if ( !tinkeringMetalScrap.empty() )
		{
			for ( auto it : tinkeringMetalScrap )
			{
				Item* item = uidToItem(it);
				if ( item )
				{
					count += item->count;
				}
			}
		}
	}
	else if ( type == TOOL_MAGIC_SCRAP )
	{
		if ( !tinkeringMagicScrap.empty() )
		{
			for ( auto it : tinkeringMagicScrap )
			{
				Item* item = uidToItem(it);
				if ( item )
				{
					count += item->count;
				}
			}
		}
	}
	return count;
}

bool GenericGUIMenu::tinkeringPlayerHasMaterialsInventory(int metal, int magic)
{
	bool hasMaterials = false;
	if ( metal > 0 && magic > 0 )
	{
		if ( tinkeringCountScrapTotal(TOOL_METAL_SCRAP) >= metal && tinkeringCountScrapTotal(TOOL_MAGIC_SCRAP) >= magic )
		{
			hasMaterials = true;
		}
		else
		{
			hasMaterials = false;
		}
	}
	else if ( metal > 0 )
	{
		if ( tinkeringCountScrapTotal(TOOL_METAL_SCRAP) >= metal )
		{
			hasMaterials = true;
		}
		else
		{
			hasMaterials = false;
		}
	}
	else if ( magic > 0 )
	{
		if ( tinkeringCountScrapTotal(TOOL_MAGIC_SCRAP) >= magic )
		{
			hasMaterials = true;
		}
		else
		{
			hasMaterials = false;
		}
	}
	return hasMaterials;
}

bool GenericGUIMenu::tinkeringGetCraftingCost(const Item* item, int* metal, int* magic)
{
	if ( !item || !metal || !magic )
	{
		return false;
	}

	switch ( item->type )
	{
		case TOOL_BOMB:
		case TOOL_FREEZE_BOMB:
			*metal = 8;
			*magic = 12;
			break;
		case TOOL_SLEEP_BOMB:
		case TOOL_TELEPORT_BOMB:
			*metal = 4;
			*magic = 8;
			break;
		case CLOAK_BACKPACK:
			*metal = 20;
			*magic = 4;
			break;
		case TOOL_DUMMYBOT:
			*metal = 8;
			*magic = 4;
			break;
		case TOOL_GYROBOT:
			*metal = 16;
			*magic = 12;
			break;
		case TOOL_SENTRYBOT:
			*metal = 16;
			*magic = 8;
			break;
		case TOOL_SPELLBOT:
			*metal = 8;
			*magic = 16;
			break;
		case TOOL_ALEMBIC:
			*metal = 16;
			*magic = 16;
			break;
		case TOOL_DECOY:
			*metal = 8;
			*magic = 1;
			break;
		case TOOL_BEARTRAP:
			*metal = 12;
			*magic = 0;
			break;
		case TOOL_LOCKPICK:
			*metal = 2;
			*magic = 0;
			break;
		case TOOL_GLASSES:
		case TOOL_LANTERN:
			*metal = 8;
			*magic = 4;
			break;
		case POTION_EMPTY:
			*metal = 2;
			*magic = 2;
			break;
		default:
			*metal = 0;
			*magic = 0;
			return false;
			break;
	}

	return true;
}

bool GenericGUIMenu::tinkeringGetItemValue(const Item* item, int* metal, int* magic)
{
	if ( !item || !metal || !magic )
	{
		return false;
	}

	switch ( item->type )
	{
		case WOODEN_SHIELD:
		case QUARTERSTAFF:
		case BRONZE_SWORD:
		case BRONZE_MACE:
		case BRONZE_AXE:
		case BRONZE_SHIELD:
		case SLING:
		case GLOVES:
		case CLOAK:
		case LEATHER_BOOTS:
		case HAT_PHRYGIAN:
		case HAT_HOOD:
		case LEATHER_HELM:
		case TOOL_TINOPENER:
		case TOOL_MIRROR:
		case TOOL_TORCH:
		case TOOL_BLINDFOLD:
		case TOOL_TOWEL:
		case FOOD_TIN:
		case WIZARD_DOUBLET:
		case HEALER_DOUBLET:
		case BRASS_KNUCKLES:
		case BRONZE_TOMAHAWK:
		case CLOAK_BLACK:
		case POTION_EMPTY:
		case TUNIC:
		case SUEDE_BOOTS:
		case SUEDE_GLOVES:
		case HAT_HOOD_RED:
		case HAT_HOOD_SILVER:
		case SILVER_DOUBLET:
		case CLOAK_SILVER:
		case TOOL_LOCKPICK:
			*metal = 1;
			*magic = 0;
			break;

		case CLOAK_MAGICREFLECTION:
		case CLOAK_PROTECTION:
		case HAT_WIZARD:
		case HAT_JESTER:
		case AMULET_WATERBREATHING:
		case AMULET_STRANGULATION:
		case AMULET_POISONRESISTANCE:
		case MAGICSTAFF_LIGHT:
		case MAGICSTAFF_LOCKING:
		case MAGICSTAFF_SLOW:
		case RING_ADORNMENT:
		case RING_PROTECTION:
		case RING_TELEPORTATION:
		case GEM_GARNET:
		case GEM_JADE:
		case GEM_JETSTONE:
		case GEM_OBSIDIAN:
		case GEM_GLASS:
		case TOOL_CRYSTALSHARD:
		case HAT_FEZ:
			*metal = 1;
			*magic = 1;
			break;

		case GLOVES_DEXTERITY:
		case LEATHER_BOOTS_SPEED:
		case AMULET_SEXCHANGE:
		case MAGICSTAFF_OPENING:
		case MAGICSTAFF_COLD:
		case MAGICSTAFF_FIRE:
		case MAGICSTAFF_LIGHTNING:
		case MAGICSTAFF_SLEEP:
		case MAGICSTAFF_POISON:
		case RING_STRENGTH:
		case RING_CONSTITUTION:
		case RING_CONFLICT:
		case GEM_AMBER:
		case GEM_EMERALD:
		case GEM_AMETHYST:
		case GEM_FLUORITE:
			*metal = 1;
			*magic = 2;
			break;

		case AMULET_MAGICREFLECTION:
		case MAGICSTAFF_DIGGING:
		case MAGICSTAFF_MAGICMISSILE:
		case RING_WARNING:
		case RING_MAGICRESISTANCE:
		case GEM_RUBY:
		case GEM_JACINTH:
		case GEM_CITRINE:
		case GEM_SAPPHIRE:
		case GEM_AQUAMARINE:
		case GEM_OPAL:
		case TOOL_BLINDFOLD_FOCUS:
			*metal = 1;
			*magic = 3;
			break;

		case CLOAK_INVISIBILITY:
		case AMULET_LIFESAVING:
		case RING_SLOWDIGESTION:
		case RING_INVISIBILITY:
		case RING_LEVITATION:
		case RING_REGENERATION:
		case GEM_DIAMOND:
		case TOOL_SKELETONKEY:
		case VAMPIRE_DOUBLET:
		case MAGICSTAFF_CHARM:
		case MAGICSTAFF_BLEED:
		case MAGICSTAFF_STONEBLOOD:
		case MAGICSTAFF_SUMMON:
		case MASK_SHAMAN:
			*metal = 1;
			*magic = 4;
			break;

		case SCROLL_LIGHT:
		case SCROLL_FIRE:
		case SCROLL_MAGICMAPPING:
		case SCROLL_REPAIR:
		case SCROLL_DESTROYARMOR:
		case SCROLL_TELEPORTATION:
			*metal = 0;
			*magic = 2;
			break;

		case SCROLL_IDENTIFY:
		case SCROLL_REMOVECURSE:
		case SCROLL_FOOD:
		case SCROLL_SUMMON:
		case SPELLBOOK_FORCEBOLT:
		case SPELLBOOK_LIGHT:
		case SPELLBOOK_SLOW:
		case SPELLBOOK_LOCKING:
		case SPELLBOOK_TELEPORTATION:
		case SPELLBOOK_REVERT_FORM:
		case SPELLBOOK_RAT_FORM:
		case SPELLBOOK_SPRAY_WEB:
		case SPELLBOOK_POISON:
		case SPELLBOOK_SPEED:
		case SPELLBOOK_DETECT_FOOD:
		case SPELLBOOK_SHADOW_TAG:
		case SPELLBOOK_SALVAGE:
		case SPELLBOOK_DASH:
		case SPELLBOOK_9:
		case SPELLBOOK_10:
			*metal = 0;
			*magic = 4;
			break;

		case TOOL_BLINDFOLD_TELEPATHY:
			*metal = 1;
			*magic = 6;
			break;

		case SCROLL_ENCHANTWEAPON:
		case SCROLL_ENCHANTARMOR:
		case SPELLBOOK_COLD:
		case SPELLBOOK_FIREBALL:
		case SPELLBOOK_REMOVECURSE:
		case SPELLBOOK_LIGHTNING:
		case SPELLBOOK_IDENTIFY:
		case SPELLBOOK_MAGICMAPPING:
		case SPELLBOOK_SLEEP:
		case SPELLBOOK_CONFUSE:
		case SPELLBOOK_OPENING:
		case SPELLBOOK_HEALING:
		case SPELLBOOK_CUREAILMENT:
		case SPELLBOOK_ACID_SPRAY:
		case SPELLBOOK_CHARM_MONSTER:
		case SPELLBOOK_SPIDER_FORM:
		case SPELLBOOK_TROLL_FORM:
		case SPELLBOOK_FEAR:
		case SPELLBOOK_STRIKE:
		case SPELLBOOK_TELEPULL:
		case SPELLBOOK_FLUTTER:
		case SCROLL_CHARGING:
		case SCROLL_CONJUREARROW:
			*metal = 0;
			*magic = 6;
			break;

		case SPELLBOOK_MAGICMISSILE:
		case SPELLBOOK_LEVITATION:
		case SPELLBOOK_INVISIBILITY:
		case SPELLBOOK_EXTRAHEALING:
		case SPELLBOOK_DIG:
		case SPELLBOOK_SUMMON:
		case SPELLBOOK_BLEED:
		case SPELLBOOK_REFLECT_MAGIC:
		case SPELLBOOK_STONEBLOOD:
		case SPELLBOOK_STEAL_WEAPON:
		case SPELLBOOK_DRAIN_SOUL:
		case SPELLBOOK_VAMPIRIC_AURA:
		case SPELLBOOK_IMP_FORM:
		case SPELLBOOK_TROLLS_BLOOD:
		case SPELLBOOK_WEAKNESS:
		case SPELLBOOK_AMPLIFY_MAGIC:
		case SPELLBOOK_DEMON_ILLU:
		case SPELLBOOK_SELF_POLYMORPH:
		case GEM_LUCK:
		case ENCHANTED_FEATHER:
			*metal = 0;
			*magic = 8;
			break;

		case IRON_SPEAR:
		case IRON_SWORD:
		case IRON_MACE:
		case IRON_AXE:
		case IRON_SHIELD:
		case SHORTBOW:
		case BRACERS:
		case IRON_BOOTS:
		case LEATHER_BREASTPIECE:
		case IRON_HELM:
		case TOOL_PICKAXE:
		case TOOL_LANTERN:
		case TOOL_GLASSES:
		case IRON_KNUCKLES:
		case TOOL_BEARTRAP:
		case IRON_DAGGER:
			*metal = 2;
			*magic = 0;
			break;

		case BRACERS_CONSTITUTION:
		case TOOL_ALEMBIC:
		case PUNISHER_HOOD:
			*metal = 2;
			*magic = 2;
			break;

		case IRON_BOOTS_WATERWALKING:
			*metal = 2;
			*magic = 3;
			break;

		case MIRROR_SHIELD:
			*metal = 2;
			*magic = 4;
			break;

		case STEEL_HALBERD:
		case STEEL_SWORD:
		case STEEL_MACE:
		case STEEL_AXE:
		case STEEL_SHIELD:
		case CROSSBOW:
		case GAUNTLETS:
		case STEEL_BOOTS:
		case IRON_BREASTPIECE:
		case STEEL_HELM:
		case SPIKED_GAUNTLETS:
		case STEEL_CHAKRAM:
		case TOOL_WHIP:
		case MACHINIST_APRON:
		case LONGBOW:
			*metal = 3;
			*magic = 0;
			break;

		case GAUNTLETS_STRENGTH:
			*metal = 3;
			*magic = 2;
			break;

		case STEEL_SHIELD_RESISTANCE:
			*metal = 3;
			*magic = 4;
			break;

		case STEEL_BREASTPIECE:
		case CRYSTAL_SHURIKEN:
		case HEAVY_CROSSBOW:
			*metal = 4;
			*magic = 0;
			break;

		case CRYSTAL_HELM:
		case CRYSTAL_BOOTS:
		case CRYSTAL_SHIELD:
		case CRYSTAL_GLOVES:
		case CRYSTAL_SWORD:
		case CRYSTAL_SPEAR:
		case CRYSTAL_BATTLEAXE:
		case CRYSTAL_MACE:
		case CLOAK_BACKPACK:
		case COMPOUND_BOW:
			*metal = 4;
			*magic = 2;
			break;

		case STEEL_BOOTS_FEATHER:
			*metal = 4;
			*magic = 3;
			break;

		case STEEL_BOOTS_LEVITATION:
			*metal = 4;
			*magic = 4;
			break;

		case ARTIFACT_BOW:
		case BOOMERANG:
			*metal = 4;
			*magic = 16;
			break;
		case ARTIFACT_CLOAK:
			*metal = 4;
			*magic = 24;
			break;

		case CRYSTAL_BREASTPIECE:
			*metal = 8;
			*magic = 2;
			break;

		case ARTIFACT_SWORD:
		case ARTIFACT_MACE:
		case ARTIFACT_SPEAR:
		case ARTIFACT_AXE:
		case ARTIFACT_HELM:
		case ARTIFACT_BOOTS:
		case ARTIFACT_GLOVES:
			*metal = 8;
			*magic = 16;
			break;

		case ARTIFACT_BREASTPIECE:
			*metal = 16;
			*magic = 16;
			break;

		case TOOL_SENTRYBOT:
		case TOOL_SPELLBOT:
		case TOOL_DUMMYBOT:
		case TOOL_GYROBOT:
			if ( item->status == BROKEN )
			{
				tinkeringGetCraftingCost(item, &(*metal), &(*magic));
				*metal /= 2;
				*magic /= 2;
			}
			else
			{
				*metal = 0;
				*magic = 0;
			}
			break;
		case TOOL_DECOY:
			*metal = 2;
			*magic = 0;
			break;
		case TOOL_DETONATOR_CHARGE:
			*metal = 2;
			*magic = 4;
			break;
		default:
			*metal = 0;
			*magic = 0;
			break;
	}


	if ( *metal > 0 || *magic > 0 )
	{
		if ( item->beatitude != 0 )
		{
			*magic += (abs(item->beatitude) * 1);
		}
		return true;
	}
	return false;
}

void getGeneralItemRepairCostWithoutRequirements(const int player, Item* item, int& metal, int& magic)
{
	int metalSalvage = 0;
	int magicSalvage = 0;
	GenericGUI[player].tinkeringGetItemValue(item, &metalSalvage, &magicSalvage);
	metal = metalSalvage * 8;
	magic = magicSalvage * 8;
	int blessingOrCurse = abs(item->beatitude);
	magic += blessingOrCurse * 4;
}

bool GenericGUIMenu::tinkeringGetRepairCost(Item* item, int* metal, int* magic)
{
	if ( !item || !metal || !magic )
	{
		return false;
	}

	switch ( item->type )
	{
		case TOOL_SENTRYBOT:
		case TOOL_SPELLBOT:
		case TOOL_DUMMYBOT:
		case TOOL_GYROBOT:
			if ( item->status != BROKEN )
			{
				tinkeringGetCraftingCost(item, &(*metal), &(*magic));
				if ( *metal > 0 )
				{
					*metal = std::max(1, (*metal) / 4);
				}
				if ( *magic > 0 )
				{
					*magic = std::max(0, (*magic) / 4);
				}

				if ( item->tinkeringBotIsMaxHealth() && item->status == EXCELLENT )
				{
					// can't repair/upgrade.
					*metal = 0;
					*magic = 0;
				}
			}
			else
			{
				*metal = 0;
				*magic = 0;
			}
			break;
		case TOOL_TINKERING_KIT:
			if ( item->status < EXCELLENT )
			{
				*metal = 16;
				*magic = 0;
			}
			else
			{
				*metal = 0;
				*magic = 0;
			}
			break;
		default:
			*metal = 0;
			*magic = 0;
			if ( item->status < EXCELLENT )
			{
				int requirement = tinkeringRepairGeneralItemSkillRequirement(item);
				if ( requirement >= 0 && stats[gui_player]
					&& ((stats[gui_player]->PROFICIENCIES[PRO_LOCKPICKING] + statGetPER(stats[gui_player], players[gui_player]->entity)) >= requirement) )
				{
					getGeneralItemRepairCostWithoutRequirements(gui_player, item, *metal, *magic);
				}
			}
			break;
	}
	// clamp repair cost limits to 99 since GUI overlaps 3 digits...
	*metal = std::min(99, *metal);
	*magic = std::min(99, *magic);

	if ( *metal > 0 || *magic > 0 )
	{
		return true;
	}

	return false;
}

int GenericGUIMenu::tinkeringRepairGeneralItemSkillRequirement(Item* item)
{
	if ( !item )
	{
		return -1;
	}
	if ( itemCategory(item) != WEAPON && itemCategory(item) != ARMOR )
	{
		if ( item->type == BOOMERANG )
		{
			// exception, allowed to repair.
		}
		else
		{
			return -1;
		}
	}
	int metal = 0;
	int magic = 0;
	int requirement = 0;
	int blessing = item->beatitude;
	item->beatitude = 0;
	tinkeringGetItemValue(item, &metal, &magic);
	item->beatitude = blessing;

	if ( metal == 0 && magic == 0 )
	{
		return -1;
	}
	if ( magic > 0 || metal >= 3 )
	{
		requirement = SKILL_LEVEL_LEGENDARY;
	}
	else if ( metal >= 2 )
	{
		requirement = SKILL_LEVEL_MASTER;
	}
	else if ( metal >= 1 )
	{
		requirement = SKILL_LEVEL_EXPERT;
	}

	if ( requirement > 0 )
	{
		if ( stats[gui_player] && stats[gui_player]->type == AUTOMATON )
		{
			requirement -= 20;
		}
		return requirement;
	}
	return -1;
}

bool GenericGUIMenu::tinkeringIsItemUpgradeable(const Item* item)
{
	if ( !item )
	{
		return false;
	}
	switch ( item->type )
	{
		case TOOL_SENTRYBOT:
		case TOOL_SPELLBOT:
		case TOOL_DUMMYBOT:
		case TOOL_GYROBOT:
			if ( item->tinkeringBotIsMaxHealth() && (tinkeringPlayerHasSkillLVLToCraft(item) != -1) )
			{
				return true;
			}
			break;
		default:
			break;
	}
	return false;
}


int GenericGUIMenu::tinkeringPlayerHasSkillLVLToCraft(const Item* item)
{
	if ( !item || !stats[gui_player] )
	{
		return -1;
	}
	int skillLVL = 0;
	if ( stats[gui_player] && players[gui_player] )
	{
		skillLVL = (stats[gui_player]->PROFICIENCIES[PRO_LOCKPICKING] + statGetPER(stats[gui_player], players[gui_player]->entity)) / 20; // 0 to 5
	}

	switch ( item->type )
	{
		case TOOL_LOCKPICK:
		case TOOL_GLASSES:
		case POTION_EMPTY:
		case TOOL_DECOY:
			if ( stats[gui_player]->PROFICIENCIES[PRO_LOCKPICKING] + statGetPER(stats[gui_player], players[gui_player]->entity) >= 10 ) // 10 requirement
			{
				return 0;
			}
			break;
		case TOOL_BEARTRAP:
		case TOOL_GYROBOT:
		case TOOL_SLEEP_BOMB:
		case TOOL_FREEZE_BOMB:
		case TOOL_DUMMYBOT:
		case TOOL_LANTERN:
			if ( skillLVL >= 1 ) // 20 requirement
			{
				return 1;
			}
			break;
		case TOOL_BOMB:
		case TOOL_TELEPORT_BOMB:
		case TOOL_SENTRYBOT:
			if ( skillLVL >= 2 ) // 40 requirement
			{
				return 2;
			}
			break;
		case TOOL_SPELLBOT:
		case TOOL_ALEMBIC:
			if ( skillLVL >= 3 ) // 60 requirement
			{
				return 3;
			}
			break;
		case CLOAK_BACKPACK:
			if ( skillLVL >= 4 ) // 80 requirement
			{
				return 4;
			}
			break;
		case TOOL_TINKERING_KIT:
			return 0;
			break;
		default:
			break;
	}
	return -1;
}

bool GenericGUIMenu::tinkeringKitDegradeOnUse(int player)
{
	if ( players[player]->isLocalPlayer() )
	{
		Item* toDegrade = tinkeringKitItem;
		if ( !toDegrade && players[player]->isLocalPlayer() )
		{
			// look for tinkeringKit in inventory.
			toDegrade = tinkeringKitFindInInventory();
			if ( !toDegrade )
			{
				return false;
			}
		}

		bool isEquipped = itemIsEquipped(toDegrade, gui_player);

		toDegrade->status = std::max(BROKEN, static_cast<Status>(toDegrade->status - 1));
		if ( toDegrade->status > BROKEN )
		{
			messagePlayer(gui_player, MESSAGE_MISC, language[681], toDegrade->getName());
		}
		else
		{
			messagePlayer(gui_player, MESSAGE_MISC, language[662], toDegrade->getName());
			if ( players[gui_player] && players[gui_player]->entity )
			{
				playSoundEntityLocal(players[gui_player]->entity, 76, 64);
			}
			tinkeringKitItem = nullptr;
		}
		if ( multiplayer == CLIENT && isEquipped )
		{
			// the client needs to inform the server that their equipment was damaged.
			int armornum = 5;
			strcpy((char*)net_packet->data, "REPA");
			net_packet->data[4] = gui_player;
			net_packet->data[5] = armornum;
			net_packet->data[6] = toDegrade->status;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 7;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
		return true;
	}
	return false;
}

Item* GenericGUIMenu::tinkeringKitFindInInventory()
{
	if ( tinkeringKitItem )
	{
		return tinkeringKitItem;
	}
	else
	{
		for ( node_t* invnode = stats[gui_player]->inventory.first; invnode != NULL; invnode = invnode->next )
		{
			Item* tinkerItem = (Item*)invnode->element;
			if ( tinkerItem && tinkerItem->type == TOOL_TINKERING_KIT && tinkerItem->status > BROKEN )
			{
				return tinkerItem;
			}
		}
	}
	return nullptr;
}

bool GenericGUIMenu::tinkeringKitRollIfShouldBreak()
{
	if ( rand() % 20 == 10 )
	{
		return true;
	}
	return false;
}

bool GenericGUIMenu::tinkeringRepairItem(Item* item)
{
	if ( !item )
	{
		return false;
	}
	//if ( itemIsEquipped(item, gui_player) && item->type != TOOL_TINKERING_KIT )
	//{
	//	messagePlayer(gui_player, language[3681]);
	//	return false; // don't want to deal with client/server desync problems here.
	//}

	if ( stats[gui_player] && players[gui_player] )
	{
		if ( item->type == TOOL_SENTRYBOT || item->type == TOOL_SPELLBOT || item->type == TOOL_DUMMYBOT || item->type == TOOL_GYROBOT )
		{
			if ( item->tinkeringBotIsMaxHealth() )
			{
				// try upgrade item?
				int craftRequirement = tinkeringPlayerHasSkillLVLToCraft(item);
				if ( craftRequirement == -1 ) // can't craft, can't upgrade!
				{
					//playSound(90, 64);
					messagePlayer(gui_player, MESSAGE_MISC, language[3685], items[item->type].name_identified);
					return false;
				}
				else if ( !tinkeringPlayerCanAffordRepair(item) )
				{
					//playSound(90, 64);
					messagePlayer(gui_player, MESSAGE_MISC, language[3687], items[item->type].name_identified);
					return false;
				}
				
				Status newStatus = DECREPIT;
				Status maxStatus = static_cast<Status>(tinkeringUpgradeMaxStatus(item));

				if ( maxStatus <= item->status )
				{
					//playSound(90, 64);
					messagePlayer(gui_player, MESSAGE_MISC, language[3685], items[item->type].name_identified);
					return false;
				}

				if ( tinkeringConsumeMaterialsForRepair(item, true) )
				{
					newStatus = std::min(static_cast<Status>(item->status + 1), maxStatus);
					Item* upgradedItem = newItem(item->type, newStatus, item->beatitude, 1, ITEM_TINKERING_APPEARANCE, true, nullptr);
					if ( upgradedItem )
					{
						achievementObserver.playerAchievements[gui_player].fixerUpper += 1;
						Item* pickedUp = itemPickup(gui_player, upgradedItem);
						if ( pickedUp && item->count == 1 )
						{
							// item* will be consumed, so pickedUp can take the inventory slot of it.
							pickedUp->x = item->x;
							pickedUp->y = item->y;
							for ( auto& hotbarSlot : players[gui_player]->hotbar.slots() )
							{
								if ( hotbarSlot.item == item->uid )
								{
									hotbarSlot.item = pickedUp->uid;
								}
								else if ( hotbarSlot.item == pickedUp->uid )
								{
									// this was auto placed by itemPickup just above, undo it.
									hotbarSlot.item = 0;
								}
							}
						}
						free(upgradedItem);
					}
					messagePlayer(gui_player, MESSAGE_MISC, language[3683], items[item->type].name_identified);
					consumeItem(item, gui_player);
					return true;
				}
			}
			else
			{
				int craftRequirement = tinkeringPlayerHasSkillLVLToCraft(item);
				if ( craftRequirement == -1 ) // can't craft, can't repair!
				{
					//playSound(90, 64);
					messagePlayer(gui_player, MESSAGE_MISC, language[3688], items[item->type].name_identified);
					return false;
				}
				else if ( !tinkeringPlayerCanAffordRepair(item) )
				{
					//playSound(90, 64);
					messagePlayer(gui_player, MESSAGE_MISC, language[3686], items[item->type].name_identified);
					return false;
				}

				if ( tinkeringConsumeMaterialsForRepair(item, false) )
				{
					Uint32 repairedAppearance = std::min((item->appearance % 10) + 1, static_cast<Uint32>(4));
					if ( repairedAppearance == 4 )
					{
						repairedAppearance = ITEM_TINKERING_APPEARANCE;
					}
					Item* repairedItem = newItem(item->type, item->status, item->beatitude, 1, repairedAppearance, true, nullptr);
					if ( repairedItem )
					{
						achievementObserver.playerAchievements[gui_player].fixerUpper += 1;
						Item* pickedUp = itemPickup(gui_player, repairedItem);
						if ( pickedUp && item->count == 1 )
						{
							// item* will be consumed, so pickedUp can take the inventory slot of it.
							pickedUp->x = item->x;
							pickedUp->y = item->y;
							for ( auto& hotbarSlot : players[gui_player]->hotbar.slots() )
							{
								if ( hotbarSlot.item == item->uid )
								{
									hotbarSlot.item = pickedUp->uid;
								}
								else if ( hotbarSlot.item == pickedUp->uid )
								{
									// this was auto placed by itemPickup just above, undo it.
									hotbarSlot.item = 0;
								}
							}
						}
						free(repairedItem);
					}
					messagePlayer(gui_player, MESSAGE_MISC, language[3682], items[item->type].name_identified);
					consumeItem(item, gui_player);
					return true;
				}
			}
		}
		else
		{
			// normal items.
			int craftRequirement = tinkeringPlayerHasSkillLVLToCraft(item);
			if ( craftRequirement == -1 && itemCategory(item) == TOOL ) // can't craft, can't repair!
			{
				//playSound(90, 64);
				messagePlayer(gui_player, MESSAGE_MISC, language[3688], items[item->type].name_identified);
				return false;
			}
			if ( !tinkeringPlayerCanAffordRepair(item) )
			{
				//playSound(90, 64);
				messagePlayer(gui_player, MESSAGE_MISC, language[3686], items[item->type].name_identified);
				return false;
			}

			if ( tinkeringConsumeMaterialsForRepair(item, false) )
			{
				int repairedStatus = std::min(static_cast<Status>(item->status + 1), EXCELLENT);
				bool isEquipped = itemIsEquipped(item, gui_player);
				item->status = static_cast<Status>(repairedStatus);
				messagePlayer(gui_player, MESSAGE_MISC, language[872], item->getName());
				bool replaceTinkeringKit = false;
				if ( item == tinkeringKitItem )
				{
					replaceTinkeringKit = true;
				}
				if ( !isEquipped )
				{
					Item* repairedItem = newItem(item->type, item->status, item->beatitude, 1, item->appearance, true, nullptr);
					if ( repairedItem )
					{
						Item* pickedUp = itemPickup(gui_player, repairedItem);
						if ( pickedUp && item->count == 1 )
						{
							// item* will be consumed, so pickedUp can take the inventory slot of it.
							pickedUp->x = item->x;
							pickedUp->y = item->y;
							for ( auto& hotbarSlot : players[gui_player]->hotbar.slots() )
							{
								if ( hotbarSlot.item == item->uid )
								{
									hotbarSlot.item = pickedUp->uid;
								}
								else if ( hotbarSlot.item == pickedUp->uid )
								{
									// this was auto placed by itemPickup just above, undo it.
									hotbarSlot.item = 0;
								}
							}
							if ( replaceTinkeringKit )
							{
								tinkeringKitItem = pickedUp;
							}
						}
						free(repairedItem);
					}
					consumeItem(item, gui_player);
				}
				else if ( multiplayer == CLIENT && isEquipped )
				{
					// the client needs to inform the server that their equipment was repaired.
					int armornum = 0;
					if ( item == stats[gui_player]->weapon )
					{
						armornum = 0;
					}
					else if ( item == stats[gui_player]->helmet )
					{
						armornum = 1;
					}
					else if ( item == stats[gui_player]->breastplate )
					{
						armornum = 2;
					}
					else if ( item == stats[gui_player]->gloves )
					{
						armornum = 3;
					}
					else if ( item == stats[gui_player]->shoes )
					{
						armornum = 4;
					}
					else if ( item == stats[gui_player]->shield )
					{
						armornum = 5;
					}
					else if ( item == stats[gui_player]->cloak )
					{
						armornum = 6;
					}
					else if ( item == stats[gui_player]->mask )
					{
						armornum = 7;
					}

					strcpy((char*)net_packet->data, "REPA");
					net_packet->data[4] = gui_player;
					net_packet->data[5] = armornum;
					net_packet->data[6] = item->status;
					net_packet->address.host = net_server.host;
					net_packet->address.port = net_server.port;
					net_packet->len = 7;
					sendPacketSafe(net_sock, -1, net_packet, 0);
				}
				return true;
			}
		}
	}
	return false;
}

int GenericGUIMenu::tinkeringUpgradeMaxStatus(Item* item)
{
	if ( !item )
	{
		return BROKEN;
	}
	int skillLVL = 0;
	if ( stats[gui_player] && players[gui_player] )
	{
		skillLVL = (stats[gui_player]->PROFICIENCIES[PRO_LOCKPICKING] + statGetPER(stats[gui_player], players[gui_player]->entity)) / 20; // 0 to 5
		int craftRequirement = tinkeringPlayerHasSkillLVLToCraft(item);
		if ( skillLVL >= 5 )
		{
			return EXCELLENT;
		}
		else
		{
			if ( skillLVL - craftRequirement == 1 )
			{
				return WORN;
			}
			else if ( skillLVL - craftRequirement == 2 )
			{
				return SERVICABLE;
			}
			else if ( skillLVL - craftRequirement >= 3 )
			{
				return EXCELLENT;
			}
		}
	}
	return BROKEN;
}

bool GenericGUIMenu::tinkeringConsumeMaterialsForRepair(Item* item, bool upgradingItem)
{
	if ( !item )
	{
		return false;
	}
	int metal = 0;
	int magic = 0;
	tinkeringGetRepairCost(item, &metal, &magic);
	if ( metal == 0 && magic == 0 )
	{
		return false;
	}
	if ( tinkeringPlayerHasMaterialsInventory(metal, magic) )
	{
		bool increaseSkill = false;
		if ( stats[gui_player] )
		{
			if ( !upgradingItem )
			{
				if ( rand() % 40 == 0 )
				{
					increaseSkill = true;
				}
			}
			else
			{
				if ( rand() % 10 == 0 )
				{
					increaseSkill = true;
				}
			}
		}

		if ( increaseSkill )
		{
			if ( multiplayer == CLIENT )
			{
				// request level up
				strcpy((char*)net_packet->data, "CSKL");
				net_packet->data[4] = gui_player;
				net_packet->data[5] = PRO_LOCKPICKING;
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 6;
				sendPacketSafe(net_sock, -1, net_packet, 0);
			}
			else
			{
				if ( players[gui_player] && players[gui_player]->entity )
				{
					players[gui_player]->entity->increaseSkill(PRO_LOCKPICKING);
				}
			}
		}

		for ( int c = 0; c < metal; ++c )
		{
			Item* item = uidToItem(tinkeringRetrieveLeastScrapStack(TOOL_METAL_SCRAP));
			if ( item )
			{
				consumeItem(item, gui_player);
			}
		}
		for ( int c = 0; c < magic; ++c )
		{
			Item* item = uidToItem(tinkeringRetrieveLeastScrapStack(TOOL_MAGIC_SCRAP));
			if ( item )
			{
				consumeItem(item, gui_player);
			}
		}
		if ( tinkeringKitRollIfShouldBreak() && item != tinkeringKitItem )
		{
			tinkeringKitDegradeOnUse(gui_player);
		}
		return true;
	}
	return false;
}

void GenericGUIMenu::scribingCreateCraftableItemList()
{
	scribingFreeLists();
	std::vector<Item*> items;
	std::unordered_map<int, int> scrollAppearanceMap;
	for ( int i = 0; i < (NUMLABELS) && i < enchantedFeatherScrollsShuffled.size(); ++i )
	{
		int itemType = enchantedFeatherScrollsShuffled.at(i);
		if ( itemType != SCROLL_BLANK )
		{
			auto find = scrollAppearanceMap.find(itemType);
			if ( find != scrollAppearanceMap.end() )
			{
				// found ItemType in map.
				scrollAppearanceMap[itemType] = (*find).second + 1;
			}
			else
			{
				// new element.
				scrollAppearanceMap.insert({ itemType, 0 });
			}
			items.push_back(newItem(static_cast<ItemType>(itemType),
				EXCELLENT, 0, 1, scrollAppearanceMap[itemType], false, &scribingTotalItems));
		}
	}

	if ( stats[gui_player] )
	{
		// make the last node jump to the player's actual items, 
		// so consuming the items in this list will actually update the player's inventory.
		node_t* scribingTotalLastCraftableNode = scribingTotalItems.last;
		if ( scribingTotalLastCraftableNode )
		{
			scribingTotalLastCraftableNode->next = stats[gui_player]->inventory.first;
		}
	}
}

void GenericGUIMenu::scribingFreeLists()
{
	node_t* nextnode = nullptr;
	int itemcnt = 0;

	// totalItems is a unique list, contains unique craftable data, 
	// as well as a pointer to continue to the player's inventory
	for ( node_t* node = scribingTotalItems.first; node; node = nextnode )
	{
		nextnode = node->next;
		if ( node->list == &scribingTotalItems )
		{
			list_RemoveNode(node);
			++itemcnt;
		}
		else if ( node->list == &stats[gui_player]->inventory )
		{
			//messagePlayer(gui_player, "reached inventory after clearing %d items", itemcnt);
			break;
		}
	}
	scribingTotalItems.first = nullptr;
	scribingTotalItems.last = nullptr;
	scribingTotalLastCraftableNode = nullptr;
	scribingBlankScrollTarget = nullptr;
	scribingLastUsageDisplayTimer = 0;
	scribingLastUsageAmount = 0;
}

int GenericGUIMenu::scribingToolDegradeOnUse(Item* itemUsedWith)
{
	if ( !itemUsedWith )
	{
		return -1;
	}
	Item* toDegrade = scribingToolItem;
	if ( !toDegrade )
	{
		// look for scribing tool in inventory.
		toDegrade = scribingToolFindInInventory();
		if ( !toDegrade )
		{
			return -1;
		}
	}

	int durability = toDegrade->appearance % ENCHANTED_FEATHER_MAX_DURABILITY;
	int usageCost = 0;
	if ( itemCategory(itemUsedWith) == SCROLL )
	{
		switch ( itemUsedWith->type )
		{
			case SCROLL_MAIL:
				usageCost = 2;
				break;
			case SCROLL_DESTROYARMOR:
			case SCROLL_FIRE:
			case SCROLL_LIGHT:
				usageCost = 4;
				break;
				break;
			case SCROLL_SUMMON:
			case SCROLL_IDENTIFY:
			case SCROLL_REMOVECURSE:
				usageCost = 6;
				break;
			case SCROLL_FOOD:
			case SCROLL_TELEPORTATION:
				usageCost = 8;
				break;
			case SCROLL_REPAIR:
			case SCROLL_MAGICMAPPING:
				usageCost = 12;
				break;
			case SCROLL_ENCHANTWEAPON:
			case SCROLL_ENCHANTARMOR:
				usageCost = 16;
				break;
			default:
				usageCost = 8;
				break;
		}
	}
	else if ( itemCategory(itemUsedWith) == SPELLBOOK )
	{
		usageCost = 16;
	}
	int randomValue = 0;
	if ( stats[gui_player] )
	{
		int skillLVL = 0;
		if ( stats[gui_player] && players[gui_player] )
		{
			skillLVL = (stats[gui_player]->PROFICIENCIES[PRO_MAGIC] + statGetINT(stats[gui_player], players[gui_player]->entity)) / 20; // 0 to 5
		}
		if ( toDegrade->beatitude > 0 )
		{
			skillLVL = 5; // blessed feather.
		}
		else if ( toDegrade->beatitude < 0 )
		{
			skillLVL = 0; // cursed feather.
		}

		if ( skillLVL >= 4 )
		{
			randomValue = rand() % 5;
			usageCost = std::max(2, usageCost - randomValue);
		}
		else if ( skillLVL < 2 )
		{
			randomValue = rand() % 7;
			usageCost += randomValue;
		}
		else if ( skillLVL == 2 )
		{
			randomValue = rand() % 5;
			usageCost += randomValue;
		}
		else if ( skillLVL == 3 )
		{
			randomValue = rand() % 3;
			usageCost += randomValue;
		}
	}

	if ( durability - usageCost < 0 )
	{
		toDegrade->status = BROKEN;
		toDegrade->appearance = 0;
	}
	else
	{
		scribingLastUsageDisplayTimer = 200;
		scribingLastUsageAmount = usageCost;
		toDegrade->appearance -= usageCost;
		if ( toDegrade->appearance % ENCHANTED_FEATHER_MAX_DURABILITY == 0 )
		{
			toDegrade->status = BROKEN;
			messagePlayer(gui_player, MESSAGE_EQUIPMENT, language[3727], toDegrade->getName());
			scribingToolItem = nullptr;
			return usageCost;
		}
		else
		{
			if ( durability > 25 && (toDegrade->appearance % ENCHANTED_FEATHER_MAX_DURABILITY) <= 25 )
			{
				// notify we're at less than 25%.
				messagePlayer(gui_player, MESSAGE_EQUIPMENT, language[3729], toDegrade->getName());
			}
		}
	}
	if ( toDegrade->status > BROKEN )
	{
		//messagePlayer(gui_player, language[681], toDegrade->getName());
		return usageCost;
	}
	else
	{
		if ( (usageCost / 2) < durability && itemCategory(itemUsedWith) == SCROLL )
		{
			// if scroll cost is a little more than the durability, then let it succeed.
			messagePlayer(gui_player, MESSAGE_EQUIPMENT, language[3727], toDegrade->getName());
			scribingToolItem = nullptr;
			return usageCost;
		}
		else
		{
			messagePlayer(gui_player, MESSAGE_EQUIPMENT, language[3728], toDegrade->getName());
			scribingToolItem = nullptr;
			return 0;
		}
	}
	return -1;
}

Item* GenericGUIMenu::scribingToolFindInInventory()
{
	if ( scribingToolItem )
	{
		return scribingToolItem;
	}
	else
	{
		for ( node_t* invnode = stats[gui_player]->inventory.first; invnode != NULL; invnode = invnode->next )
		{
			Item* scribeItem = (Item*)invnode->element;
			if ( scribeItem && scribeItem->type == ENCHANTED_FEATHER && scribeItem->status > BROKEN )
			{
				return scribeItem;
			}
		}
	}
	return nullptr;
}

bool GenericGUIMenu::scribingWriteItem(Item* item)
{
	if ( !item )
	{
		return false;
	}

	if ( itemCategory(item) == SCROLL )
	{
		if ( !scribingBlankScrollTarget )
		{
			return false;
		}

		int result = scribingToolDegradeOnUse(item);
		if ( result == 0 )
		{
			// item broken before completion.
			return false;
		}
		else if ( result < 0 )
		{
			// failure - invalid variables.
			return false;
		}

		bool increaseSkill = false;
		if ( stats[gui_player] )
		{
			if ( rand() % 5 == 0 )
			{
				increaseSkill = true;
			}
		}

		if ( increaseSkill )
		{
			if ( multiplayer == CLIENT )
			{
				// request level up
				strcpy((char*)net_packet->data, "CSKL");
				net_packet->data[4] = gui_player;
				net_packet->data[5] = PRO_MAGIC;
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 6;
				sendPacketSafe(net_sock, -1, net_packet, 0);
			}
			else
			{
				if ( players[gui_player] && players[gui_player]->entity )
				{
					players[gui_player]->entity->increaseSkill(PRO_MAGIC);
				}
			}
		}

		Item* crafted = newItem(item->type, scribingBlankScrollTarget->status, 
			scribingBlankScrollTarget->beatitude, 1, item->appearance, false, nullptr);
		if ( crafted )
		{
			if ( crafted->type == SCROLL_MAIL )
			{
				// mail uses the appearance to generate the text, so randomise it here.
				crafted->appearance = rand(); 
			}
			Item* pickedUp = itemPickup(gui_player, crafted);
			//messagePlayerColor(gui_player, uint32ColorGreen, language[3724]);
			int oldcount = pickedUp->count;
			pickedUp->count = 1;
			messagePlayerColor(gui_player, MESSAGE_INVENTORY, uint32ColorGreen, language[3724], pickedUp->description());
			pickedUp->count = oldcount;
			consumeItem(scribingBlankScrollTarget, gui_player);
			//scribingBlankScrollTarget = nullptr;
			if ( client_classes[gui_player] == CLASS_SHAMAN )
			{
				steamStatisticUpdate(STEAM_STAT_ROLL_THE_BONES, STEAM_STAT_INT, 1);
			}
			free(crafted);
			return true;
		}
	}
	else if ( itemCategory(item) == SPELLBOOK )
	{
		if ( item->status == EXCELLENT )
		{
			return false;
		}
		int result = scribingToolDegradeOnUse(item);
		if ( result == 0 )
		{
			// item broken before completion.
			return false;
		}
		else if ( result < 0 )
		{
			// failure - invalid variables.
			return false;
		}

		bool increaseSkill = false;
		if ( stats[gui_player] )
		{
			if ( rand() % 10 == 0 )
			{
				increaseSkill = true;
			}
		}

		if ( increaseSkill )
		{
			if ( multiplayer == CLIENT )
			{
				// request level up
				strcpy((char*)net_packet->data, "CSKL");
				net_packet->data[4] = gui_player;
				net_packet->data[5] = PRO_MAGIC;
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 6;
				sendPacketSafe(net_sock, -1, net_packet, 0);
			}
			else
			{
				if ( players[gui_player] && players[gui_player]->entity )
				{
					players[gui_player]->entity->increaseSkill(PRO_MAGIC);
				}
			}
		}
		int repairedStatus = std::min(static_cast<Status>(item->status + 1), EXCELLENT);
		bool isEquipped = itemIsEquipped(item, gui_player);
		item->status = static_cast<Status>(repairedStatus);
		messagePlayer(gui_player, MESSAGE_MISC, language[3725]);
		messagePlayer(gui_player, MESSAGE_INVENTORY, language[872], item->getName());
		if ( !isEquipped )
		{
			Item* repairedItem = newItem(item->type, item->status, item->beatitude, 1, item->appearance, true, nullptr);
			if ( repairedItem )
			{
				Item* pickedUp = itemPickup(gui_player, repairedItem);
				if ( pickedUp && item->count == 1 )
				{
					// item* will be consumed, so pickedUp can take the inventory slot of it.
					pickedUp->x = item->x;
					pickedUp->y = item->y;
					for ( auto& hotbarSlot : players[gui_player]->hotbar.slots() )
					{
						if ( hotbarSlot.item == item->uid )
						{
							hotbarSlot.item = pickedUp->uid;
						}
						else if ( hotbarSlot.item == pickedUp->uid )
						{
							// this was auto placed by itemPickup just above, undo it.
							hotbarSlot.item = 0;
						}
					}
				}
				free(repairedItem);
			}
			consumeItem(item, gui_player);
		}
		else if ( multiplayer == CLIENT && isEquipped )
		{
			// the client needs to inform the server that their equipment was repaired.
			int armornum = 0;
			if ( stats[gui_player] && item == stats[gui_player]->shield )
			{
				armornum = 5;
			}

			strcpy((char*)net_packet->data, "REPA");
			net_packet->data[4] = gui_player;
			net_packet->data[5] = armornum;
			net_packet->data[6] = item->status;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 7;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
		return true;
	}
	return false;
}

void EnemyHPDamageBarHandler::cullExpiredHPBars()
{
	for ( auto it = HPBars.begin(); it != HPBars.end(); )
	{
		int tickLifetime = EnemyHPDamageBarHandler::maxTickLifetime;
		if ( (*it).second.barType == BAR_TYPE_FURNITURE )
		{
			tickLifetime = EnemyHPDamageBarHandler::maxTickFurnitureLifetime;
		}
		if ( ticks - (*it).second.enemy_timer >= tickLifetime )
		{
			(*it).second.expired = true;
			if ( (*it).second.animator.fadeOut <= 0.01 )
			{
				it = HPBars.erase(it); // no need to show this bar, delete it
			}
			else
			{
				++it;
			}
		}
		else
		{
			++it;
		}
	}
}

EnemyHPDamageBarHandler::EnemyHPDetails* EnemyHPDamageBarHandler::getMostRecentHPBar(int index)
{
	if ( HPBars.empty() )
	{
		lastEnemyUid = 0;
	}

	Uint32 mostRecentTicks = 0;
	auto mostRecentEntry = HPBars.end();
	auto highPriorityMostRecentEntry = HPBars.end();
	bool foundHighPriorityEntry = false;
	for ( auto it = HPBars.begin(); it != HPBars.end(); )
	{
		if ( false /*ticks - (*it).second.enemy_timer >= k_maxTickLifetime * 120*/ )
		{
			++it;
		}
		else
		{
			if ( (*it).second.enemy_timer > mostRecentTicks && (*it).second.shouldDisplay )
			{
				if ( mostRecentEntry != HPBars.end() )
				{
					// previous most recent tick should not display until updated by high priority.
					// since we've found a new one to display.
					(*mostRecentEntry).second.shouldDisplay = false;
				}
				if ( !(*it).second.lowPriorityTick )
				{
					// this is a normal priority damage update (not burn/poison etc)
					// if a newer tick is low priority, then defer to this one.
					highPriorityMostRecentEntry = it;
					foundHighPriorityEntry = true;
				}
				mostRecentEntry = it;
				mostRecentTicks = (*it).second.enemy_timer;
				//queuedBars.push(std::make_pair(mostRecentTicks, mostRecentEntry->first));
			}
			++it;
		}
	}

	if ( mostRecentTicks > 0 )
	{
		if ( !foundHighPriorityEntry )
		{
			// all low priority, just display the last added.
		}
		else
		{
			if ( (mostRecentEntry != highPriorityMostRecentEntry) && foundHighPriorityEntry )
			{
				// the most recent was low priority, so defer to the most recent high priority.
				mostRecentEntry = highPriorityMostRecentEntry;
			}
		}

		return &(*mostRecentEntry).second;
	}
	return nullptr;
}

void EnemyHPDamageBarHandler::displayCurrentHPBar(const int player)
{
	// deprecated
	//if ( HPBars.empty() )
	//{
	//	return;
	//}
	//Uint32 mostRecentTicks = 0;
	//auto mostRecentEntry = HPBars.end();
	//auto highPriorityMostRecentEntry = HPBars.end();
	//bool foundHighPriorityEntry = false;
	//for ( auto it = HPBars.begin(); it != HPBars.end(); )
	//{
	//	if ( ticks - (*it).second.enemy_timer >= EnemyHPDamageBarHandler::maxTickLifetime )
	//	{
	//		it = HPBars.erase(it); // no need to show this bar, delete it
	//	}
	//	else
	//	{
	//		if ( (*it).second.enemy_timer > mostRecentTicks && (*it).second.shouldDisplay )
	//		{
	//			if ( mostRecentEntry != HPBars.end() )
	//			{
	//				// previous most recent tick should not display until updated by high priority.
	//				// since we've found a new one to display.
	//				(*mostRecentEntry).second.shouldDisplay = false;
	//			}
	//			if ( !(*it).second.lowPriorityTick )
	//			{
	//				// this is a normal priority damage update (not burn/poison etc)
	//				// if a newer tick is low priority, then defer to this one.
	//				highPriorityMostRecentEntry = it;
	//				foundHighPriorityEntry = true;
	//			}
	//			mostRecentEntry = it;
	//			mostRecentTicks = (*it).second.enemy_timer;
	//		}
	//		else
	//		{
	//			(*it).second.shouldDisplay = false;
	//		}
	//		++it;
	//	}
	//}
	//if ( mostRecentTicks > 0 )
	//{
	//	if ( !foundHighPriorityEntry )
	//	{
	//		// all low priority, just display the last added.
	//	}
	//	else
	//	{
	//		if ( (mostRecentEntry != highPriorityMostRecentEntry) && foundHighPriorityEntry )
	//		{
	//			// the most recent was low priority, so defer to the most recent high priority.
	//			mostRecentEntry = highPriorityMostRecentEntry;
	//		}
	//	}

	//	auto& HPDetails = (*mostRecentEntry).second;
	//	HPDetails.enemy_hp = std::max(0, HPDetails.enemy_hp);

	//	const int barWidth = 512;
	//	SDL_Rect pos;
	//	pos.w = barWidth;
	//	pos.h = 38;
	//	//pos.y = players[player]->camera_y2() - 224;
	//	if ( players[player]->hotbar.useHotbarFaceMenu )
	//	{
	//		// anchor to the topmost position, including button glyphs
	//		pos.y = players[player]->hotbar.faceButtonTopYPosition - pos.h - 8;
	//	}
	//	else
	//	{
	//		pos.y = players[player]->hotbar.hotbarBox.y - pos.h - 8;
	//	}
	//	if ( players[player]->isLocalPlayer() && players[player]->camera_width() < yres )
	//	{
	//		if ( yres < 900 )
	//		{
	//			pos.w *= 0.5;
	//		}
	//		else if ( yres < 1080 )
	//		{
	//			pos.w *= 0.8;
	//		}
	//	}
	//	pos.x = players[player]->camera_midx() - (pos.w / 2);

	//	// bar
	//	drawTooltip(&pos);

	//	pos.w = pos.w - 6;
	//	pos.x = pos.x + 3;
	//	pos.y = pos.y + 3;
	//	pos.h = pos.h - 6;
	//	drawRect(&pos, makeColorRGB(16, 0, 0), 255);

	//	if ( HPDetails.enemy_oldhp > HPDetails.enemy_hp )
	//	{
	//		int timeDiff = ticks - HPDetails.enemy_timer;
	//		if ( timeDiff > 30 || HPDetails.enemy_hp == 0 )
	//		{
	//			// delay 30 ticks before background hp drop animation, or if health 0 start immediately.
	//			// we want to complete animation with x ticks to go
	//			int depletionTicks = (80 - timeDiff);
	//			int healthDiff = HPDetails.enemy_oldhp - HPDetails.enemy_hp;

	//			// scale duration to FPS - tested @ 144hz
	//			real_t fpsScale = (144.f / std::max(1U, fpsLimit));
	//			HPDetails.depletionAnimationPercent -= fpsScale * (std::max((healthDiff) / std::max(depletionTicks, 1), 1) / 100.0);
	//			HPDetails.enemy_oldhp = HPDetails.depletionAnimationPercent * HPDetails.enemy_maxhp; // this follows the animation
	//		}
	//		else
	//		{
	//			HPDetails.depletionAnimationPercent =
	//				HPDetails.enemy_oldhp / static_cast<real_t>(HPDetails.enemy_maxhp);
	//		}
	//		int tmpw = pos.w;
	//		pos.w = pos.w * HPDetails.depletionAnimationPercent;
	//		if ( HPDetails.enemy_bar_color > 0 )
	//		{
	//			drawRect(&pos, HPDetails.enemy_bar_color, 128);
	//		}
	//		else
	//		{
	//			drawRect(&pos, makeColorRGB(128, 0, 0), 128);
	//		}
	//		pos.w = tmpw;
	//	}
	//	if ( HPDetails.enemy_hp > 0 )
	//	{
	//		int tmpw = pos.w;
	//		pos.w = pos.w * ((double)HPDetails.enemy_hp / HPDetails.enemy_maxhp);
	//		drawRect(&pos, makeColorRGB(128, 0, 0), 255);
	//		if ( HPDetails.enemy_bar_color > 0 )
	//		{
	//			drawRect(&pos, HPDetails.enemy_bar_color, 224);
	//		}
	//		pos.w = tmpw;
	//	}

	//	// name
	//	int x = players[player]->camera_midx() - longestline(HPDetails.enemy_name.c_str()) * TTF12_WIDTH / 2 + 2;
	//	int y = pos.y + 16 - TTF12_HEIGHT / 2 + 2;
	//	ttfPrintText(ttf12, x, y, HPDetails.enemy_name.c_str());
	//}
}

void EnemyHPDamageBarHandler::EnemyHPDetails::updateWorldCoordinates()
{
	Entity* entity = uidToEntity(enemy_uid);
	if ( entity )
	{
		if ( TimerExperiments::bUseTimerInterpolation && entity->bUseRenderInterpolation )
		{
			worldX = entity->lerpRenderState.x.position * 16.0;
			worldY = entity->lerpRenderState.y.position * 16.0;
			worldZ = entity->lerpRenderState.z.position + enemyBarSettings.getHeightOffset(entity);
		}
		else
		{
			worldX = entity->x;
			worldY = entity->y;
			worldZ = entity->z + enemyBarSettings.getHeightOffset(entity);
		}
		if ( entity->behavior == &actDoor && entity->flags[PASSABLE] )
		{
			if ( entity->doorStartAng == 0 )
			{
				worldY -= 5;
			}
			else
			{
				worldX -= 5;
			}
		}
		screenDistance = enemyBarSettings.getScreenDistanceOffset(entity);
	}
}

void EnemyHPDamageBarHandler::addEnemyToList(Sint32 HP, Sint32 maxHP, Sint32 oldHP, Uint32 color, Uint32 uid, const char* name, bool isLowPriority)
{
	auto find = HPBars.find(uid);
	EnemyHPDetails* details = nullptr;
	if ( find != HPBars.end() )
	{
		// uid exists in list.
		(*find).second.enemy_hp = HP;
		(*find).second.enemy_maxhp = maxHP;
		(*find).second.enemy_bar_color = color;
		(*find).second.lowPriorityTick = isLowPriority;
		if ( !isLowPriority )
		{
			(*find).second.shouldDisplay = true;
		}
		(*find).second.enemy_timer = ticks;
		details = &(*find).second; 
		details->animator.previousSetpoint = details->animator.setpoint;
		details->displayOnHUD = false;
		details->hasDistanceCheck = false;
		details->expired = false;
	}
	else
	{
		HPBars.insert(std::make_pair(uid, EnemyHPDetails(uid, HP, maxHP, oldHP, color, name, isLowPriority)));
		auto find = HPBars.find(uid);
		details = &(*find).second;
		details->animator.previousSetpoint = details->enemy_oldhp;
		details->animator.backgroundValue = details->enemy_oldhp;
	}

	details->animator.maxValue = details->enemy_maxhp;
	details->animator.setpoint = details->enemy_hp;
	details->animator.foregroundValue = details->animator.setpoint;
	details->animator.animateTicks = ticks;
	details->animator.damageTaken = std::max(-1, oldHP - HP);

	Entity* entity = uidToEntity(uid);
	spawnDamageGib(entity, details->animator.damageTaken);
	lastEnemyUid = uid;

	if ( entity )
	{
		details->updateWorldCoordinates();
	}
	if ( entity && (entity->behavior == &actPlayer || entity->behavior == &actMonster) )
	{
		if ( Stat* stat = entity->getStats() )
		{
			details->enemy_statusEffects1 = 0;
			details->enemy_statusEffects2 = 0;
			details->enemy_statusEffectsLowDuration1 = 0;
			details->enemy_statusEffectsLowDuration2 = 0;
			for ( int i = 0; i < NUMEFFECTS; ++i )
			{
				if ( stat->EFFECTS[i] )
				{
					if ( i < 32 )
					{
						details->enemy_statusEffects1 |= (1 << i);
						if ( stat->EFFECTS_TIMERS[i] > 0 && stat->EFFECTS_TIMERS[i] < 5 * TICKS_PER_SECOND )
						{
							details->enemy_statusEffectsLowDuration1 |= (1 << i);
						}
					}
					else if ( i < 64 )
					{
						details->enemy_statusEffects2 |= (1 << (i - 32));
						if ( stat->EFFECTS_TIMERS[i] > 0 && stat->EFFECTS_TIMERS[i] < 5 * TICKS_PER_SECOND )
						{
							details->enemy_statusEffectsLowDuration2 |= (1 << i);
						}
					}
				}
			}
		}
	}
}

const int GenericGUIMenu::TinkerGUI_t::MAX_TINKER_X = 5;
const int GenericGUIMenu::TinkerGUI_t::MAX_TINKER_Y = 3;

void GenericGUIMenu::TinkerGUI_t::openTinkerMenu()
{
	const int playernum = parentGUI.getPlayer();
	auto player = players[playernum];

	if ( tinkerFrame )
	{
		bool wasDisabled = tinkerFrame->isDisabled();
		tinkerFrame->setDisabled(false);
		if ( wasDisabled )
		{
			animx = 0.0;
			animTooltip = 0.0;
			animPrompt = 0.0;
			animFilter = 0.0;
			isInteractable = false;
			bFirstTimeSnapCursor = false;
		}
		if ( getSelectedTinkerSlotX () < 0 || getSelectedTinkerSlotX() >= MAX_TINKER_X
			|| getSelectedTinkerSlotY() < 0 || getSelectedTinkerSlotY() >= MAX_TINKER_Y )
		{
			selectTinkerSlot(0, 0);
		}
		player->hud.compactLayoutMode = Player::HUD_t::COMPACT_LAYOUT_INVENTORY;
		player->inventory_mode = INVENTORY_MODE_ITEM;
		bOpen = true;
	}
	if ( inputs.getUIInteraction(playernum)->selectedItem )
	{
		inputs.getUIInteraction(playernum)->selectedItem = nullptr;
		inputs.getUIInteraction(playernum)->toggleclick = false;
	}
	inputs.getUIInteraction(playernum)->selectedItemFromChest = 0;
	clearItemDisplayed();
}

void GenericGUIMenu::TinkerGUI_t::closeTinkerMenu()
{
	const int playernum = parentGUI.getPlayer();
	auto& player = *players[playernum];

	if ( tinkerFrame )
	{
		tinkerFrame->setDisabled(true);
	}
	animx = 0.0;
	animTooltip = 0.0;
	animPrompt = 0.0;
	animFilter = 0.0;
	animInvalidAction = 0.0;
	animInvalidActionTicks = 0;
	invalidActionType = INVALID_ACTION_NONE;
	isInteractable = false;
	bool wasOpen = bOpen;
	bOpen = false;
	bFirstTimeSnapCursor = false;
	if ( wasOpen )
	{
		if ( inputs.getUIInteraction(playernum)->selectedItem )
		{
			inputs.getUIInteraction(playernum)->selectedItem = nullptr;
			inputs.getUIInteraction(playernum)->toggleclick = false;
		}
		inputs.getUIInteraction(playernum)->selectedItemFromChest = 0;
	}
	if ( players[playernum]->GUI.activeModule == Player::GUI_t::MODULE_TINKERING
		&& !players[playernum]->shootmode )
	{
		// reset to inventory mode if still hanging in tinker GUI
		players[playernum]->hud.compactLayoutMode = Player::HUD_t::COMPACT_LAYOUT_INVENTORY;
		players[playernum]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
		if ( !inputs.getVirtualMouse(playernum)->draw_cursor )
		{
			players[playernum]->GUI.warpControllerToModule(false);
		}
	}
	clearItemDisplayed();
	itemRequiresTitleReflow = true;
	if ( tinkerFrame )
	{
		for ( auto f : tinkerFrame->getFrames() )
		{
			f->removeSelf();
		}
		tinkerSlotFrames.clear();
	}
}

void onTinkerChangeTabAction(const int playernum, bool changingToNewTab = true)
{
	auto& tinkerGUI = GenericGUI[playernum].tinkerGUI;
	tinkerGUI.isInteractable = false;
	tinkerGUI.bFirstTimeSnapCursor = false;
	if ( GenericGUI[playernum].tinkeringFilter != GenericGUIMenu::TINKER_FILTER_CRAFTABLE )
	{
		if ( players[playernum]->GUI.activeModule == Player::GUI_t::MODULE_TINKERING )
		{
			// reset to inventory mode if still hanging in tinker GUI
			players[playernum]->hud.compactLayoutMode = Player::HUD_t::COMPACT_LAYOUT_INVENTORY;
			players[playernum]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
			if ( !inputs.getVirtualMouse(playernum)->draw_cursor )
			{
				players[playernum]->GUI.warpControllerToModule(false);
			}
		}
	}
	else
	{
		players[playernum]->hud.compactLayoutMode = Player::HUD_t::COMPACT_LAYOUT_INVENTORY;
		players[playernum]->GUI.activateModule(Player::GUI_t::MODULE_TINKERING);
		if ( !inputs.getVirtualMouse(playernum)->draw_cursor )
		{
			players[playernum]->GUI.warpControllerToModule(false);
		}
	}
	tinkerGUI.clearItemDisplayed();
	tinkerGUI.itemRequiresTitleReflow = true;
	if ( changingToNewTab )
	{
		tinkerGUI.animPrompt = 1.0;
		tinkerGUI.animPromptTicks = ticks;
	}
}

int GenericGUIMenu::TinkerGUI_t::heightOffsetWhenNotCompact = 200;
const int tinkerBaseWidth = 334;

bool GenericGUIMenu::TinkerGUI_t::tinkerGUIHasBeenCreated() const
{
	if ( tinkerFrame )
	{
		if ( !tinkerFrame->getFrames().empty() )
		{
			for ( auto f : tinkerFrame->getFrames() )
			{
				if ( !f->isToBeDeleted() )
				{
					return true;
				}
			}
			return false;
		}
		else
		{
			return false;
		}
	}
	return false;
}

bool GenericGUIMenu::TinkerGUI_t::isConstructMenuActive() const
{
	if ( !parentGUI.isGUIOpen() || !tinkerGUIHasBeenCreated() )
	{
		return false;
	}
	if ( bOpen && parentGUI.guiType == GUICurrentType::GUI_TYPE_TINKERING
		&& parentGUI.tinkeringFilter == TinkeringFilter::TINKER_FILTER_CRAFTABLE )
	{
		return true;
	}
	return false;
}

bool GenericGUIMenu::TinkerGUI_t::isSalvageOrRepairMenuActive() const
{
	if ( !parentGUI.isGUIOpen() || !tinkerGUIHasBeenCreated() )
	{
		return false;
	}
	if ( bOpen && parentGUI.guiType == GUICurrentType::GUI_TYPE_TINKERING
		&& (parentGUI.tinkeringFilter == TinkeringFilter::TINKER_FILTER_REPAIRABLE
			|| parentGUI.tinkeringFilter == TinkeringFilter::TINKER_FILTER_SALVAGEABLE) )
	{
		return true;
	}
	return false;
}

void tinkerScrapChangeEvent(const int player, int metalAmount, int magicAmount, int realMetalScrap, int realMagicScrap)
{
	auto& tinkerGUI = GenericGUI[player].tinkerGUI;
	{
		bool addedToCurrentTotal = false;
		bool isAnimatingValue = true || ((ticks - tinkerGUI.animScrapStartTicks) > TICKS_PER_SECOND / 2);
		if ( metalAmount < 0 )
		{
			if ( tinkerGUI.playerChangeMetalScrap < 0
				&& !isAnimatingValue
				&& abs(metalAmount) > 0 )
			{
				addedToCurrentTotal = true;
				if ( realMetalScrap + metalAmount < 0 )
				{
					tinkerGUI.playerChangeMetalScrap -= realMetalScrap;
				}
				else
				{
					tinkerGUI.playerChangeMetalScrap += metalAmount;
				}
			}
			else
			{
				if ( realMetalScrap + metalAmount < 0 )
				{
					tinkerGUI.playerChangeMetalScrap = -realMetalScrap;
				}
				else
				{
					tinkerGUI.playerChangeMetalScrap = metalAmount;
				}
			}
		}
		else
		{
			if ( tinkerGUI.playerChangeMetalScrap > 0
				&& !isAnimatingValue
				&& abs(metalAmount) > 0 )
			{
				addedToCurrentTotal = true;
				tinkerGUI.playerChangeMetalScrap += metalAmount;
			}
			else
			{
				tinkerGUI.playerChangeMetalScrap = metalAmount;
			}
		}
		//tinkerGUI.animScrapStartTicks = ticks;
		tinkerGUI.animScrap = 1.0;
		if ( !addedToCurrentTotal )
		{
			tinkerGUI.playerCurrentMetalScrap = realMetalScrap;
		}
	}
	{
		bool addedToCurrentTotal = false;
		bool isAnimatingValue = true || ((ticks - tinkerGUI.animScrapStartTicks) > TICKS_PER_SECOND / 2);
		if ( magicAmount < 0 )
		{
			if ( tinkerGUI.playerChangeMagicScrap < 0
				&& !isAnimatingValue
				&& abs(magicAmount) > 0 )
			{
				addedToCurrentTotal = true;
				if ( realMagicScrap + magicAmount < 0 )
				{
					tinkerGUI.playerChangeMagicScrap -= realMagicScrap;
				}
				else
				{
					tinkerGUI.playerChangeMagicScrap += magicAmount;
				}
			}
			else
			{
				if ( realMagicScrap + magicAmount < 0 )
				{
					tinkerGUI.playerChangeMagicScrap = -realMagicScrap;
				}
				else
				{
					tinkerGUI.playerChangeMagicScrap = magicAmount;
				}
			}
		}
		else
		{
			if ( tinkerGUI.playerChangeMagicScrap > 0
				&& !isAnimatingValue
				&& abs(magicAmount) > 0 )
			{
				addedToCurrentTotal = true;
				tinkerGUI.playerChangeMagicScrap += magicAmount;
			}
			else
			{
				tinkerGUI.playerChangeMagicScrap = magicAmount;
			}
		}
		//tinkerGUI.animScrapStartTicks = ticks;
		tinkerGUI.animScrap = 1.0;
		if ( !addedToCurrentTotal )
		{
			tinkerGUI.playerCurrentMagicScrap = realMagicScrap;
		}
	}
}

void GenericGUIMenu::TinkerGUI_t::updateTinkerScrapHeld(void* metalHeldText, void* magicHeldText, int realMetalScrap, int realMagicScrap)
{
	Field* metalField = static_cast<Field*>(metalHeldText);
	Field* magicField = static_cast<Field*>(magicHeldText);

	bool pauseChangeGoldAnim = false;
	if ( playerChangeMetalScrap != 0 || playerChangeMagicScrap != 0 )
	{
		if ( true || ((ticks - animScrapStartTicks) > TICKS_PER_SECOND / 2) )
		{
			const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.1, (animScrap)) / 10.0;
			animScrap -= setpointDiffX;
			animScrap = std::max(0.0, animScrap);

			if ( animScrap <= 0.0001 )
			{
				playerChangeMetalScrap = 0;
				playerChangeMagicScrap = 0;
			}
		}
		else
		{
			pauseChangeGoldAnim = true;

			const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animScrap)) / 10.0;
			animScrap += setpointDiffX;
			animScrap = std::min(1.0, animScrap);
		}
	}

	{
		bool pauseChangeGoldAnim = false;
		bool showChangedMetalScrap = false;
		if ( playerChangeMetalScrap != 0 )
		{
			int displayedChangeMetalScrap = animScrap * playerChangeMetalScrap;
			if ( pauseChangeGoldAnim )
			{
				displayedChangeMetalScrap = playerChangeMetalScrap;
			}
			if ( abs(displayedChangeMetalScrap) > 0 )
			{
				showChangedMetalScrap = true;
				//changeGoldText->setDisabled(false);
				//std::string s = "+";
				//if ( playerChangeMetalScrap < 0 )
				//{
				//	s = "";
				//}
				//s += std::to_string(displayedChangeMetalScrap);
				//changeGoldText->setText(s.c_str());
				int displayedCurrentMetalScrap = playerCurrentMetalScrap
					+ (playerChangeMetalScrap - displayedChangeMetalScrap);
				metalField->setText(std::to_string(displayedCurrentMetalScrap).c_str());
			}
		}

		if ( !showChangedMetalScrap )
		{
			int displayedChangeMetalScrap = 0;
			//changeGoldText->setDisabled(true);
			//changeGoldText->setText(std::to_string(displayedChangeMetalScrap).c_str());
			metalField->setText(std::to_string(realMetalScrap).c_str());
		}
	}

	{
		bool showChangedMagicScrap = false;
		if ( playerChangeMagicScrap != 0 )
		{
			int displayedChangeMagicScrap = animScrap * playerChangeMagicScrap;
			if ( pauseChangeGoldAnim )
			{
				displayedChangeMagicScrap = playerChangeMagicScrap;
			}
			if ( abs(displayedChangeMagicScrap) > 0 )
			{
				showChangedMagicScrap = true;
				//changeGoldText->setDisabled(false);
				//std::string s = "+";
				//if ( playerChangeMagicScrap < 0 )
				//{
				//	s = "";
				//}
				//s += std::to_string(displayedChangeMagicScrap);
				//changeGoldText->setText(s.c_str());
				int displayedCurrentMagicScrap = playerCurrentMagicScrap
					+ (playerChangeMagicScrap - displayedChangeMagicScrap);
				magicField->setText(std::to_string(displayedCurrentMagicScrap).c_str());
			}
		}

		if ( !showChangedMagicScrap )
		{
			int displayedChangeMagicScrap = 0;
			//changeGoldText->setDisabled(true);
			//changeGoldText->setText(std::to_string(displayedChangeMagicScrap).c_str());
			magicField->setText(std::to_string(realMagicScrap).c_str());
		}
	}
}

void buttonTinkerUpdateSelectorOnHighlight(const int player, Button* button)
{
	if ( button->isHighlighted() )
	{
		players[player]->GUI.setHoveringOverModuleButton(Player::GUI_t::MODULE_TINKERING);
		if ( players[player]->GUI.activeModule != Player::GUI_t::MODULE_TINKERING )
		{
			players[player]->GUI.activateModule(Player::GUI_t::MODULE_TINKERING);
		}
		SDL_Rect pos = button->getAbsoluteSize();
		// make sure to adjust absolute size to camera viewport
		pos.x -= players[player]->camera_virtualx1();
		pos.y -= players[player]->camera_virtualy1();
		players[player]->hud.setCursorDisabled(false);
		players[player]->hud.updateCursorAnimation(pos.x - 1, pos.y - 1, pos.w, pos.h, inputs.getVirtualMouse(player)->draw_cursor);
	}
}

void GenericGUIMenu::TinkerGUI_t::updateTinkerMenu()
{
	const int playernum = parentGUI.getPlayer();
	auto player = players[playernum];

	if ( !player->isLocalPlayer() )
	{
		closeTinkerMenu();
		return;
	}

	if ( !tinkerFrame )
	{
		return;
	}

	tinkerFrame->setSize(SDL_Rect{ players[playernum]->camera_virtualx1(),
		players[playernum]->camera_virtualy1(),
		tinkerBaseWidth,
		players[playernum]->camera_virtualHeight() });

	bool bConstructDrawerOpen = isConstructMenuActive();

	if ( !tinkerFrame->isDisabled() && bOpen )
	{
		if ( !tinkerGUIHasBeenCreated() )
		{
			createTinkerMenu();
		}

		const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
		real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animx)) / 2.0;
		animx += setpointDiffX;
		animx = std::min(1.0, animx);
		bool mainPanelReady = false;
		if ( animx >= .9999 )
		{
			if ( !bConstructDrawerOpen )
			{
				if ( !bFirstTimeSnapCursor )
				{
					bFirstTimeSnapCursor = true;
					if ( !inputs.getUIInteraction(playernum)->selectedItem
						&& player->GUI.activeModule == Player::GUI_t::MODULE_TINKERING )
					{
						warpMouseToSelectedTinkerItem(nullptr, (Inputs::SET_CONTROLLER));
					}
				}
				isInteractable = true;
			}
			mainPanelReady = true;
		}

		if ( bConstructDrawerOpen && mainPanelReady )
		{
			real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animDrawer)) / 3.0;
			animDrawer += setpointDiffX;
			animDrawer = std::min(1.0, animDrawer);
			if ( animDrawer >= .9999 )
			{
				if ( !bFirstTimeSnapCursor )
				{
					bFirstTimeSnapCursor = true;
					if ( !inputs.getUIInteraction(playernum)->selectedItem
						&& player->GUI.activeModule == Player::GUI_t::MODULE_TINKERING )
					{
						warpMouseToSelectedTinkerItem(nullptr, (Inputs::SET_CONTROLLER));
					}
				}
				isInteractable = true;
			}
			else
			{
				isInteractable = false;
			}
		}
		else
		{
			real_t setpointDiffX = fpsScale * std::max(.01, (animDrawer)) / 2.0;
			animDrawer -= setpointDiffX;
			animDrawer = std::max(0.0, animDrawer);
		}
	}
	else
	{
		animDrawer = 0.0;
		animx = 0.0;
		animTooltip = 0.0;
		isInteractable = false;
	}

	bool usingConstructMenu = parentGUI.tinkeringFilter == GenericGUIMenu::TINKER_FILTER_CRAFTABLE;
	if ( !usingConstructMenu )
	{
		const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
		real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animFilter)) / 2.0;
		animFilter += setpointDiffX;
		animFilter = std::min(1.0, animFilter);
	}
	else
	{
		const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
		real_t setpointDiffX = fpsScale * std::max(.01, (animFilter)) / 2.0;
		animFilter -= setpointDiffX;
		animFilter = std::max(0.0, animFilter);
	}

	auto tinkerFramePos = tinkerFrame->getSize();
	if ( player->inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_LEFT )
	{
		if ( !player->inventoryUI.bCompactView )
		{
			const int fullWidth = tinkerFramePos.w + 210 + (40 * (1.0 - animFilter)); // inventory width 210
			tinkerFramePos.x = -tinkerFramePos.w + animx * fullWidth;
			if ( player->bUseCompactGUIWidth() )
			{
				if ( player->inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				tinkerFramePos.x -= player->inventoryUI.slideOutWidth * player->inventoryUI.slideOutPercent;
			}
		}
		else
		{
			tinkerFramePos.x = player->camera_virtualWidth() - animx * tinkerFramePos.w;
			if ( player->bUseCompactGUIWidth() )
			{
				if ( player->inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				tinkerFramePos.x -= -player->inventoryUI.slideOutWidth * player->inventoryUI.slideOutPercent;
			}
		}
	}
	else if ( player->inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_RIGHT )
	{
		if ( !player->inventoryUI.bCompactView )
		{
			const int fullWidth = tinkerFramePos.w + 210 + (40 * (1.0 - animFilter)); // inventory width 210
			tinkerFramePos.x = player->camera_virtualWidth() - animx * fullWidth * 2;
			if ( player->bUseCompactGUIWidth() )
			{
				if ( player->inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				tinkerFramePos.x -= -player->inventoryUI.slideOutWidth * player->inventoryUI.slideOutPercent;
			}
		}
		else
		{
			tinkerFramePos.x = -tinkerFramePos.w + animx * tinkerFramePos.w;
			if ( player->bUseCompactGUIWidth() )
			{
				if ( player->inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				tinkerFramePos.x -= player->inventoryUI.slideOutWidth * player->inventoryUI.slideOutPercent;
			}
		}
	}

	if ( !player->bUseCompactGUIHeight() )
	{
		tinkerFramePos.y = heightOffsetWhenNotCompact;
	}
	else
	{
		tinkerFramePos.y = 0;
	}

	if ( !tinkerGUIHasBeenCreated() )
	{
		return;
	}

	auto drawerFrame = tinkerFrame->findFrame("tinker drawer");
	drawerFrame->setDisabled(true);
	auto baseFrame = tinkerFrame->findFrame("tinker base");
	baseFrame->setDisabled(false);

	tinkerFrame->setSize(tinkerFramePos);

	SDL_Rect baseFramePos = baseFrame->getSize();
	baseFramePos.x = 0;
	baseFramePos.w = tinkerBaseWidth;
	baseFrame->setSize(baseFramePos);

	{
		drawerFrame->setDisabled(!bConstructDrawerOpen);
		SDL_Rect drawerFramePos = drawerFrame->getSize();
		const int widthDifference = animDrawer * (drawerFramePos.w - 8);
		drawerFramePos.x = 0;
		drawerFramePos.y = 20;
		drawerFrame->setSize(drawerFramePos);

		tinkerFramePos.x -= widthDifference;
		int adjustx = 0;
		if ( tinkerFramePos.x < 0 )
		{
			adjustx = -tinkerFramePos.x; // to not slide off-frame
			tinkerFramePos.x += adjustx;
		}
		tinkerFramePos.w += (widthDifference);
		tinkerFramePos.h = std::max(drawerFramePos.y + drawerFramePos.h, baseFramePos.y + baseFramePos.h);
		tinkerFrame->setSize(tinkerFramePos);

		baseFramePos.x = tinkerFramePos.w - baseFramePos.w;
		baseFrame->setSize(baseFramePos);
	}

	if ( !bOpen )
	{
		return;
	}

	for ( int x = 0; x < MAX_TINKER_X; ++x )
	{
		for ( int y = 0; y < MAX_TINKER_Y; ++y )
		{
			if ( auto slotFrame = getTinkerSlotFrame(x, y) )
			{
				slotFrame->setDisabled(true);
			}
		}
	}

	if ( !parentGUI.isGUIOpen()
		|| parentGUI.guiType != GUICurrentType::GUI_TYPE_TINKERING
		|| !stats[playernum]
		|| stats[playernum]->HP <= 0
		|| !player->entity
		|| player->shootmode )
	{
		closeTinkerMenu();
		return;
	}

	if ( player->entity && player->entity->isBlind() )
	{
		messagePlayer(playernum, MESSAGE_MISC, language[4159]);
		parentGUI.closeGUI();
		return; // I can't see!
	}

	// tinker kit status
	{
		auto tinkerKitTitle = baseFrame->findField("tinker kit title");
		auto tinkerKitStatus = baseFrame->findField("tinker kit status");
		if ( auto item = parentGUI.tinkeringKitFindInInventory() )
		{
			char buf[128];
			if ( !item->identified )
			{
				snprintf(buf, sizeof(buf), "%s (?)", item->getName());
			}
			else
			{
				snprintf(buf, sizeof(buf), "%s (%+d)", item->getName(), item->beatitude);
			}
			std::string titleStr = buf;
			if ( !titleStr.empty() )
			{
				if ( titleStr[0] >= 'a' && titleStr[0] <= 'z' )
				{
					titleStr[0] = (char)toupper((int)titleStr[0]);
				}
				size_t found = titleStr.find(' ');
				while ( found != std::string::npos )
				{
					auto& c = titleStr[std::min(found + 1, titleStr.size() - 1)];
					if ( c >= 'a' && c <= 'z' )
					{
						c = (char)toupper((int)c);
					}
					found = titleStr.find(' ', found + 1);
				}
				tinkerKitTitle->setText(titleStr.c_str());
			}
			else
			{
				tinkerKitTitle->setText(buf);
			}
			tinkerKitStatus->setText(ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str());
			if ( item->status <= DECREPIT )
			{
				tinkerKitStatus->setTextColor(hudColors.characterSheetRed);
			}
			else
			{
				tinkerKitStatus->setTextColor(hudColors.characterSheetLightNeutral);
			}
		}
		else
		{
			tinkerKitTitle->setText("");
			tinkerKitStatus->setText("");
		}

		SDL_Rect textPos{ 0, 17, baseFrame->getSize().w, 24 };
		tinkerKitTitle->setSize(textPos);
		textPos.y += 20;
		tinkerKitStatus->setSize(textPos);
	}

	if ( itemActionType == TINKER_ACTION_OK	|| itemActionType == TINKER_ACTION_OK_UPGRADE
		|| itemActionType == TINKER_ACTION_OK_UNIDENTIFIED_SALVAGE )
	{
		animInvalidAction = 0.0;
		animInvalidActionTicks = 0;
	}
	else 
	{
		// shaking feedback for invalid action
		// constant decay for animation
		const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
		real_t setpointDiffX = fpsScale * 1.0 / 25.0;
		animInvalidAction -= setpointDiffX;
		animInvalidAction = std::max(0.0, animInvalidAction);
	}
	bool bInvalidActionAnimating = false;
	if ( animInvalidAction > 0.001 || (ticks - animInvalidActionTicks) < TICKS_PER_SECOND * .8 )
	{
		bInvalidActionAnimating = true;
	}
	bool bAnimScrapStarted = animScrap >= 1.0;
	// held qtys
	int heldMetalScrap = parentGUI.tinkeringCountScrapTotal(TOOL_METAL_SCRAP);
	int heldMagicScrap = parentGUI.tinkeringCountScrapTotal(TOOL_MAGIC_SCRAP);
	auto metalHeldText = baseFrame->findField("held metal txt");
	auto magicHeldText = baseFrame->findField("held magic txt");
	{
		auto heldScrapText = baseFrame->findField("held scrap label");
		heldScrapText->setDisabled(false);

		auto heldScrapBg = baseFrame->findImage("held scrap img");
		heldScrapBg->pos.x = baseFrame->getSize().w - 18 - heldScrapBg->pos.w;
		heldScrapBg->pos.y = baseFrame->getSize().h - 48 - 38;

		SDL_Rect metalPos{ heldScrapBg->pos.x, heldScrapBg->pos.y + 9, 74, 24 };
		SDL_Rect magicPos{ heldScrapBg->pos.x, heldScrapBg->pos.y + 9, 166, 24 };
		metalHeldText->setSize(metalPos);
		metalHeldText->setColor(hudColors.characterSheetLightNeutral);
		magicHeldText->setSize(magicPos);
		magicHeldText->setColor(hudColors.characterSheetLightNeutral);

		updateTinkerScrapHeld(metalHeldText, magicHeldText, heldMetalScrap, heldMagicScrap);

		SDL_Rect heldScrapTxtPos = heldScrapText->getSize();
		heldScrapTxtPos.w = heldScrapBg->pos.x - 4;
		heldScrapTxtPos.x = 0;
		heldScrapTxtPos.y = metalPos.y;
		heldScrapTxtPos.h = 24;
		heldScrapText->setSize(heldScrapTxtPos);
		heldScrapText->setText(language[4131]);
		if ( invalidActionType == INVALID_ACTION_SHAKE_METAL_SCRAP
			|| invalidActionType == INVALID_ACTION_SHAKE_ALL_SCRAP )
		{
			metalPos.x += -2 + 2 * (cos(animInvalidAction * 4 * PI));
			metalHeldText->setSize(metalPos);
			if ( bInvalidActionAnimating )
			{
				metalHeldText->setColor(hudColors.characterSheetRed); // red
			}
		}
		if ( invalidActionType == INVALID_ACTION_SHAKE_MAGIC_SCRAP
			|| invalidActionType == INVALID_ACTION_SHAKE_ALL_SCRAP )
		{
			magicPos.x += -2 + 2 * (cos(animInvalidAction * 4 * PI));
			magicHeldText->setSize(magicPos);
			if ( bInvalidActionAnimating )
			{
				magicHeldText->setColor(hudColors.characterSheetRed); // red
			}
		}
	}

	bool usingGamepad = inputs.hasController(playernum) && !inputs.getVirtualMouse(playernum)->draw_cursor;

	int filterLeftSideX = 0;
	int filterStartY = 0;
	int filterRightSideX = 0;
	{
		// filters
		Button* filterBtn = baseFrame->findButton("filter salvage btn");
		filterBtn->setDisabled(true);
		if ( inputs.getVirtualMouse(playernum)->draw_cursor )
		{
			filterBtn->setDisabled(!isInteractable);
			if ( isInteractable )
			{
				buttonTinkerUpdateSelectorOnHighlight(playernum, filterBtn);
			}
		}
		else if ( filterBtn->isSelected() )
		{
			filterBtn->deselect();
		}
		filterBtn->setColor(makeColor(255, 255, 255, 0));
		filterBtn->setText(language[3645]);
		{
			SDL_Rect btnPos{ 126, 264, 82, 26 };
			filterBtn->setSize(btnPos);
		}
		if ( parentGUI.tinkeringFilter == GenericGUIMenu::TINKER_FILTER_SALVAGEABLE )
		{
			filterBtn->setColor(makeColor(255, 255, 255, 32));
		}
		filterBtn->setHighlightColor(filterBtn->getColor());

		filterBtn = baseFrame->findButton("filter craft btn");
		filterBtn->setDisabled(true);
		if ( inputs.getVirtualMouse(playernum)->draw_cursor )
		{
			filterBtn->setDisabled(!isInteractable);
			if ( isInteractable )
			{
				buttonTinkerUpdateSelectorOnHighlight(playernum, filterBtn);
			}
		}
		else if ( filterBtn->isSelected() )
		{
			filterBtn->deselect();
		}
		filterBtn->setColor(makeColor(255, 255, 255, 0));
		filterBtn->setText(language[3644]);
		{
			SDL_Rect btnPos{ 50, 264, 70, 26 };
			filterBtn->setSize(btnPos);
			filterLeftSideX = btnPos.x;
			filterStartY = btnPos.y;
		}
		if ( parentGUI.tinkeringFilter == GenericGUIMenu::TINKER_FILTER_CRAFTABLE )
		{
			filterBtn->setColor(makeColor(255, 255, 255, 32));
		}
		filterBtn->setHighlightColor(filterBtn->getColor());

		filterBtn = baseFrame->findButton("filter repair btn");
		filterBtn->setDisabled(true);
		if ( inputs.getVirtualMouse(playernum)->draw_cursor )
		{
			filterBtn->setDisabled(!isInteractable);
			if ( isInteractable )
			{
				buttonTinkerUpdateSelectorOnHighlight(playernum, filterBtn);
			}
		}
		else if ( filterBtn->isSelected() )
		{
			filterBtn->deselect();
		}
		filterBtn->setColor(makeColor(255, 255, 255, 0));
		filterBtn->setText(language[3646]);
		{
			SDL_Rect btnPos{ 214, 264, 70, 26 };
			filterBtn->setSize(btnPos);
			filterRightSideX = btnPos.x + btnPos.w;
		}
		if ( parentGUI.tinkeringFilter == GenericGUIMenu::TINKER_FILTER_REPAIRABLE )
		{
			filterBtn->setColor(makeColor(255, 255, 255, 32));
		}
		filterBtn->setHighlightColor(filterBtn->getColor());

		// close btn
		auto closeBtn = baseFrame->findButton("close tinker button");
		auto closeGlyph = baseFrame->findImage("close tinker glyph");
		closeBtn->setDisabled(true);
		closeGlyph->disabled = true;
		if ( inputs.getVirtualMouse(playernum)->draw_cursor )
		{
			closeBtn->setDisabled(!isInteractable);
			if ( isInteractable )
			{
				buttonTinkerUpdateSelectorOnHighlight(playernum, closeBtn);
			}
		}
		else if ( closeBtn->isSelected() )
		{
			closeBtn->deselect();
		}
		if ( closeBtn->isDisabled() && usingGamepad )
		{
			closeGlyph->path = Input::inputs[playernum].getGlyphPathForBinding("MenuCancel");
			if ( auto imgGet = Image::get(closeGlyph->path.c_str()) )
			{
				closeGlyph->pos.w = imgGet->getWidth();
				closeGlyph->pos.h = imgGet->getHeight();
				closeGlyph->disabled = false;
			}
			closeGlyph->pos.x = closeBtn->getSize().x + closeBtn->getSize().w / 2 - closeGlyph->pos.w / 2;
			if ( closeGlyph->pos.x % 2 == 1 )
			{
				++closeGlyph->pos.x;
			}
			closeGlyph->pos.y = closeBtn->getSize().y + closeBtn->getSize().h - 4;
		}
	}

	auto itemDisplayTooltip = baseFrame->findFrame("tinker display tooltip");
	itemDisplayTooltip->setDisabled(false);

	auto actionPromptTxt = baseFrame->findField("action prompt txt");
	actionPromptTxt->setDisabled(false);
	auto actionPromptImg = baseFrame->findImage("action prompt glyph");
	auto actionModifierImg = baseFrame->findImage("action modifier glyph");

	int skillLVL = (stats[playernum]->PROFICIENCIES[PRO_LOCKPICKING] + statGetPER(stats[playernum], players[playernum]->entity));
	Uint32 negativeColor = hudColors.characterSheetRed;
	Uint32 neutralColor = hudColors.characterSheetLightNeutral;
	Uint32 positiveColor = hudColors.characterSheetGreen;
	Uint32 defaultPromptColor = makeColor(255, 255, 255, 255);

	auto displayItemName = itemDisplayTooltip->findField("item display name");
	auto displayItemTextImg = itemDisplayTooltip->findImage("item text img");
	auto itemSlotBg = itemDisplayTooltip->findImage("item bg img");
	auto metalText = itemDisplayTooltip->findField("item metal value");
	auto magicText = itemDisplayTooltip->findField("item magic value");
	itemSlotBg->pos.x = 12;
	itemSlotBg->pos.y = 12;
	const int displayItemTextImgBaseX = itemSlotBg->pos.x + itemSlotBg->pos.w;
	displayItemTextImg->pos.x = displayItemTextImgBaseX;
	displayItemTextImg->pos.y = itemSlotBg->pos.y + itemSlotBg->pos.h / 2 - displayItemTextImg->pos.h / 2;
	SDL_Rect displayItemNamePos{ displayItemTextImg->pos.x + 6, displayItemTextImg->pos.y - 4, 208, 24 };
	displayItemNamePos.h = 50;
	displayItemName->setSize(displayItemNamePos);
	SDL_Rect actionPromptTxtPos{ 0, 197, baseFrame->getSize().w - 18 - 8, 24 };
	actionPromptTxt->setSize(actionPromptTxtPos);

	SDL_Rect tooltipPos = itemDisplayTooltip->getSize();
	tooltipPos.w = 298;
	tooltipPos.h = baseFrame->getSize().h - 100;
	tooltipPos.y = 88;
	tooltipPos.x = 18 - (tooltipPos.w + 18) * (0.0/*1.0 - animTooltip*/);
	itemDisplayTooltip->setSize(tooltipPos);

	auto itemIncrementText = baseFrame->findField("item increment txt");
	{
		if ( animScrap > 0.01 )
		{
			itemIncrementText->setDisabled(false);
			auto pos = itemIncrementText->getSize();
			pos.y = tooltipPos.y + itemSlotBg->pos.y + 16 - ((1.0 - animScrap) * 32);
			itemIncrementText->setSize(pos);
			SDL_Color color;
			getColor(itemIncrementText->getColor(), &color.r, &color.g, &color.b, &color.a);
			if ( animScrap < .2 )
			{
				itemIncrementText->setColor(makeColor(color.r, color.g, color.b, 255 * (animScrap / .2)));
			}
			else
			{
				itemIncrementText->setColor(makeColor(color.r, color.g, color.b, 255));
			}
		}
		else
		{
			itemIncrementText->setSize(SDL_Rect{ tooltipPos.x + itemSlotBg->pos.x + 24, tooltipPos.y + itemSlotBg->pos.y + 16, 72, 24});
			itemIncrementText->setDisabled(true);
			itemIncrementText->setText("");
		}
	}

	auto costBg = itemDisplayTooltip->findImage("item cost img");
	auto costScrapText = itemDisplayTooltip->findField("item cost label");
	{
		costBg->pos.x = displayItemTextImgBaseX + displayItemTextImg->pos.w - costBg->pos.w;
		costBg->pos.y = displayItemTextImg->pos.y + displayItemTextImg->pos.h;
		SDL_Rect metalPos{ costBg->pos.x + 12, costBg->pos.y + 9, 50, 24 };
		SDL_Rect magicPos{ costBg->pos.x + 84, costBg->pos.y + 9, 50, 24 };
		metalText->setSize(metalPos);
		magicText->setSize(magicPos);
		SDL_Rect costScrapTxtPos = costScrapText->getSize();
		costScrapTxtPos.w = costBg->pos.x - 4;
		costScrapTxtPos.x = 0;
		costScrapTxtPos.y = metalPos.y;
		costScrapTxtPos.h = 24;
		costScrapText->setSize(costScrapTxtPos);
	}

	auto itemSlotFrame = itemDisplayTooltip->findFrame("item slot frame");
	bool modifierPressed = false;
	if ( usingGamepad && Input::inputs[playernum].binary("MenuPageLeftAlt") )
	{
		modifierPressed = true;
	}
	else if ( inputs.bPlayerUsingKeyboardControl(playernum)
		&& (keystatus[SDL_SCANCODE_LSHIFT] || keystatus[SDL_SCANCODE_RSHIFT]) )
	{
		modifierPressed = true;
	}

	if ( itemActionType != TINKER_ACTION_NONE && itemDesc.size() > 1 )
	{
		if ( isInteractable )
		{
			//const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
			//real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animTooltip)) / 2.0;
			//animTooltip += setpointDiffX;
			//animTooltip = std::min(1.0, animTooltip);
			animTooltip = 1.0;
			animTooltipTicks = ticks;
		}

		itemDisplayTooltip->setDisabled(false);

		bool isTinkeringBot = (itemType == TOOL_SENTRYBOT
			|| itemType == TOOL_SPELLBOT
			|| itemType == TOOL_DUMMYBOT
			|| itemType == TOOL_GYROBOT);

		{
			// prompt + glyph
			actionPromptTxt->setDisabled(false);
			if ( itemActionType == TINKER_ACTION_OK || itemActionType == TINKER_ACTION_OK_UPGRADE
				|| itemActionType == TINKER_ACTION_OK_UNIDENTIFIED_SALVAGE )
			{
				if ( usingGamepad )
				{
					actionPromptImg->path = Input::inputs[playernum].getGlyphPathForBinding("MenuConfirm");
					if ( modifierPressed )
					{
						actionModifierImg->path = Input::inputs[playernum].getGlyphPathForBinding("MenuPageLeftAlt");
					}
				}
				else if ( !usingGamepad )
				{
					actionPromptImg->path = Input::inputs[playernum].getGlyphPathForBinding("MenuRightClick");
					if ( modifierPressed )
					{
						actionModifierImg->path = GlyphHelper.getGlyphPath(SDL_SCANCODE_LSHIFT, false);
					}
				}
				if ( auto imgGet = Image::get(actionPromptImg->path.c_str()) )
				{
					actionPromptImg->pos.w = imgGet->getWidth();
					actionPromptImg->pos.h = imgGet->getHeight();
					actionPromptImg->disabled = false;
					if ( modifierPressed && parentGUI.tinkeringFilter == TINKER_FILTER_SALVAGEABLE )
					{
						if ( auto imgGet2 = Image::get(actionModifierImg->path.c_str()) )
						{
							actionModifierImg->pos.w = imgGet2->getWidth();
							actionModifierImg->pos.h = imgGet2->getHeight();
							actionModifierImg->disabled = false;
						}
					}
					else
					{
						actionModifierImg->disabled = true;
					}
				}
				if ( parentGUI.tinkeringFilter == GenericGUIMenu::TINKER_FILTER_CRAFTABLE )
				{
					actionPromptTxt->setText(language[3644]);
				}
				else if ( parentGUI.tinkeringFilter == GenericGUIMenu::TINKER_FILTER_REPAIRABLE )
				{
					if ( itemActionType == TINKER_ACTION_OK_UPGRADE )
					{
						actionPromptTxt->setText(language[3684]);
					}
					else
					{
						actionPromptTxt->setText(language[3646]);
					}
				}
				else if ( parentGUI.tinkeringFilter == GenericGUIMenu::TINKER_FILTER_SALVAGEABLE )
				{
					if ( modifierPressed )
					{
						actionPromptTxt->setText(language[4154]);
					}
					else
					{
						actionPromptTxt->setText(language[3645]);
					}
				}
				else
				{
					actionPromptTxt->setText("");
				}
				actionPromptTxt->setColor(defaultPromptColor);
			}
			else
			{
				actionPromptTxt->setText("");
				actionPromptImg->disabled = true;
				actionModifierImg->disabled = true;
				switch ( itemActionType )
				{
					case TINKER_ACTION_INVALID_ITEM:
						if ( parentGUI.tinkeringFilter == TINKER_FILTER_REPAIRABLE )
						{
							actionPromptTxt->setText(language[4138]);
						}
						else if ( parentGUI.tinkeringFilter == TINKER_FILTER_SALVAGEABLE )
						{
							actionPromptTxt->setText(language[4137]);
						}
						break;
					case TINKER_ACTION_INVALID_ROBOT_TO_SALVAGE:
						actionPromptTxt->setText(language[4148]);
						break;
					case TINKER_ACTION_NO_MATERIALS_UPGRADE:
						if ( parentGUI.tinkeringFilter == TINKER_FILTER_REPAIRABLE )
						{
							actionPromptTxt->setText(language[4142]);
						}
						break;
					case TINKER_ACTION_NO_MATERIALS:
						if ( parentGUI.tinkeringFilter == TINKER_FILTER_REPAIRABLE )
						{
							actionPromptTxt->setText(language[4141]);
						}
						else if ( parentGUI.tinkeringFilter == TINKER_FILTER_SALVAGEABLE )
						{
							actionPromptTxt->setText(language[4137]);
						}
						else if ( parentGUI.tinkeringFilter == TINKER_FILTER_CRAFTABLE )
						{
							actionPromptTxt->setText(language[4140]);
						}
						break;
					case TINKER_ACTION_NO_SKILL_LVL:
						if ( parentGUI.tinkeringFilter == TINKER_FILTER_CRAFTABLE )
						{
							char buf[128];
							snprintf(buf, sizeof(buf), language[4147], skillLVL, itemRequirement);
							actionPromptTxt->setText(buf);
						}
						else
						{
							char buf[128];
							snprintf(buf, sizeof(buf), language[4144], skillLVL, itemRequirement);
							actionPromptTxt->setText(buf);
						}
						break;
					case TINKER_ACTION_NO_SKILL_LVL_UPGRADE:
						char buf[128];
						snprintf(buf, sizeof(buf), language[4145], skillLVL, itemRequirement);
						actionPromptTxt->setText(buf);
						break;
					case TINKER_ACTION_ITEM_FULLY_REPAIRED:
						actionPromptTxt->setText(language[4136]);
						break;
					case TINKER_ACTION_ITEM_FULLY_UPGRADED:
						actionPromptTxt->setText(language[4139]);
						break;
					case TINKER_ACTION_ROBOT_BROKEN:
						actionPromptTxt->setText(language[4143]);
						break;
					case TINKER_ACTION_MUST_BE_UNEQUIPPED:
						actionPromptTxt->setText(language[4132]);
						break;
					case TINKER_ACTION_ALREADY_USING_THIS_TINKERING_KIT:
						actionPromptTxt->setText(language[4146]);
						break;
					case TINKER_ACTION_KIT_NEEDS_REPAIRS:
						actionPromptTxt->setText(language[4152]);
						break;
					case TINKER_ACTION_NOT_IDENTIFIED_YET:
						actionPromptTxt->setText(language[4153]);
						break;
					default:
						actionPromptTxt->setText("-");
						break;
				}
				actionPromptTxt->setColor(negativeColor);
			}
			if ( auto textGet = actionPromptTxt->getTextObject() )
			{
				actionPromptImg->pos.x = actionPromptTxtPos.x + actionPromptTxtPos.w 
					- textGet->getWidth() - 8 - actionPromptImg->pos.w;
				actionPromptImg->pos.y = actionPromptTxtPos.y + actionPromptTxtPos.h / 2 - actionPromptImg->pos.h / 2;
				if ( actionPromptImg->pos.y % 2 == 1 )
				{
					actionPromptImg->pos.y -= 1;
				}
				actionModifierImg->pos.x = actionPromptImg->pos.x - 4 - actionModifierImg->pos.w;
				actionModifierImg->pos.y = actionPromptTxtPos.y + actionPromptTxtPos.h / 2 - actionModifierImg->pos.h / 2;
				if ( actionModifierImg->pos.y % 2 == 1 )
				{
					actionModifierImg->pos.y -= 1;
				}
			}
		}


		{
			// item slot + frame
			SDL_Rect slotFramePos = itemSlotFrame->getSize();
			slotFramePos.x = itemSlotBg->pos.x + itemSlotBg->pos.w / 2 - slotFramePos.w / 2 - 1;
			slotFramePos.y = itemSlotBg->pos.y + itemSlotBg->pos.h / 2 - slotFramePos.h / 2 - 1;
			itemSlotFrame->setSize(slotFramePos);
		}

		{
			// item name + text bg
			displayItemName->setVJustify(Field::justify_t::CENTER);
			if ( itemRequiresTitleReflow )
			{
				displayItemName->setText(itemDesc.c_str());
				displayItemName->reflowTextToFit(0);
				if ( displayItemName->getNumTextLines() > 2 )
				{
					// more than 2 lines, append ...
					std::string copiedName = displayItemName->getText();
					auto lastNewline = copiedName.find_last_of('\n');
					copiedName = copiedName.substr(0U, lastNewline);
					copiedName += "...";
					displayItemName->setText(copiedName.c_str());
					displayItemName->reflowTextToFit(0);
					if ( displayItemName->getNumTextLines() > 2 )
					{
						// ... doesn't fit, replace last 3 characters with ...
						copiedName = copiedName.substr(0U, copiedName.size() - 6);
						copiedName += "...";
						displayItemName->setText(copiedName.c_str());
						displayItemName->reflowTextToFit(0);
					}
				}
				itemRequiresTitleReflow = false;
			}
		}

		{
			// scrap cost img + texts
			metalText->setColor(neutralColor);
			magicText->setColor(neutralColor);
			if ( itemActionType == TINKER_ACTION_INVALID_ITEM
				|| (metalScrapPrice == 0 && magicScrapPrice == 0) )
			{
				metalText->setText("-");
				magicText->setText("-");
			}
			else
			{
				if ( metalScrapPrice <= 0 )
				{
					metalText->setText("");
				}
				else
				{
					metalText->setText(std::to_string(metalScrapPrice).c_str());
				}
				if ( magicScrapPrice <= 0 )
				{
					magicText->setText("");
				}
				else
				{
					magicText->setText(std::to_string(magicScrapPrice).c_str());
				}
				if ( parentGUI.tinkeringFilter == TINKER_FILTER_CRAFTABLE
					|| parentGUI.tinkeringFilter == TINKER_FILTER_REPAIRABLE )
				{
					if ( itemActionType == TINKER_ACTION_NOT_IDENTIFIED_YET )
					{
						metalText->setText("?");
						magicText->setText("?");
					}
					//if ( metalScrapPrice > heldMetalScrap )
					//{
					//	metalText->setColor(negativeColor);
					//	//metalHeldText->setColor(negativeColor);
					//}
					//if ( magicScrapPrice > heldMagicScrap )
					//{
					//	magicText->setColor(negativeColor);
					//	//magicHeldText->setColor(negativeColor);
					//}
				}
				else if ( parentGUI.tinkeringFilter == TINKER_FILTER_SALVAGEABLE )
				{
					if ( itemActionType == TINKER_ACTION_OK_UNIDENTIFIED_SALVAGE )
					{
						metalText->setText("+?");
						magicText->setText("+?");
					}
					else
					{
						if ( metalScrapPrice > 0 )
						{
							char buf[32];
							snprintf(buf, sizeof(buf), "%+d", metalScrapPrice);
							metalText->setText(buf);
							//metalText->setColor(positiveColor);
						}
						if ( magicScrapPrice > 0 )
						{
							char buf[32];
							snprintf(buf, sizeof(buf), "%+d", magicScrapPrice);
							magicText->setText(buf);
							//magicText->setColor(positiveColor);
						}
					}
				}
			}
		}
	}
	else
	{
		if ( (!usingGamepad && (ticks - animTooltipTicks > TICKS_PER_SECOND / 3))
			|| (usingGamepad && !bConstructDrawerOpen)
			|| animTooltip < 0.9999 )
		{
			const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.01, (animTooltip)) / 2.0;
			animTooltip -= setpointDiffX;
			animTooltip = std::max(0.0, animTooltip);
		}
	}

	if ( parentGUI.tinkeringFilter == TINKER_FILTER_CRAFTABLE )
	{
		costScrapText->setText(language[4130]);
	}
	else if ( parentGUI.tinkeringFilter == TINKER_FILTER_REPAIRABLE )
	{
		bool activeTooltip = (itemActionType != TINKER_ACTION_NONE && itemDesc.size() > 1);
		std::string currentText = costScrapText->getText();
		if ( activeTooltip || (!activeTooltip
			&& currentText != language[4135]
			&& currentText != language[4134]) ) // if inactive tooltip, don't quickly change between upgrade/repair
		{
			if ( itemType == TOOL_SENTRYBOT
				|| itemType == TOOL_SPELLBOT
				|| itemType == TOOL_DUMMYBOT
				|| itemType == TOOL_GYROBOT )
			{
				costScrapText->setText(language[4135]);
			}
			else
			{
				costScrapText->setText(language[4134]);
			}
		}
	}
	else if ( parentGUI.tinkeringFilter == TINKER_FILTER_SALVAGEABLE )
	{
		costScrapText->setText(language[4133]);
	}

	auto actionPromptUnselectedTxt = baseFrame->findField("action prompt unselected txt");
	auto actionPromptCoverLeftImg = baseFrame->findImage("action prompt lcover");
	auto actionPromptCoverRightImg = baseFrame->findImage("action prompt rcover");
	actionPromptCoverLeftImg->pos.x = 0;
	actionPromptCoverRightImg->pos.x = baseFrame->getSize().w - actionPromptCoverLeftImg->pos.w;
	actionPromptCoverLeftImg->pos.y = 60;
	actionPromptCoverRightImg->pos.y = 60;

	{
		actionPromptUnselectedTxt->setDisabled(false);
		actionPromptUnselectedTxt->setColor(makeColor(224, 224, 224, 255));
		if ( parentGUI.tinkeringFilter == TINKER_FILTER_CRAFTABLE )
		{
			actionPromptUnselectedTxt->setText(language[4149]);
		}
		else if ( parentGUI.tinkeringFilter == TINKER_FILTER_SALVAGEABLE )
		{
			actionPromptUnselectedTxt->setText(language[4150]);
		}
		else if ( parentGUI.tinkeringFilter == TINKER_FILTER_REPAIRABLE )
		{
			actionPromptUnselectedTxt->setText(language[4151]);
		}

		{
			SDL_Rect pos = actionPromptTxt->getSize();
			pos.x = 26;
			pos.w -= 26;
			if ( animPromptMoveLeft )
			{
				pos.x -= actionPromptUnselectedTxt->getSize().w * animPrompt;
			}
			else
			{
				pos.x += actionPromptUnselectedTxt->getSize().w * animPrompt;
			}
			pos.y = 63;
			actionPromptUnselectedTxt->setSize(pos);
		}

		if ( ticks - animPromptTicks > TICKS_PER_SECOND / 10 )
		{
			const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.01, (animPrompt)) / 2.0;
			animPrompt -= setpointDiffX;
			animPrompt = std::max(0.0, animPrompt);
		}
		SDL_Color color;
		getColor(actionPromptUnselectedTxt->getColor(), &color.r, &color.g, &color.b, &color.a);
		color.a = (Uint8)(255 * (pow(1.0 - animPrompt, 2)));
		actionPromptUnselectedTxt->setColor(makeColor(color.r, color.g, color.b, color.a));
	}

	{
		SDL_Color color;
		getColor(actionPromptTxt->getColor(), &color.r, &color.g, &color.b, &color.a);
		color.a = (Uint8)(255 * animTooltip);
		actionPromptImg->color = makeColor(255, 255, 255, color.a);
		actionModifierImg->color = actionPromptImg->color;
		actionPromptTxt->setColor(makeColor(color.r, color.g, color.b, color.a));
		if ( invalidActionType == INVALID_ACTION_SHAKE_PROMPT )
		{
			SDL_Rect pos = actionPromptTxt->getSize();
			pos.x += -4 + 4 * (cos(animInvalidAction * 4 * PI));
			actionPromptTxt->setSize(pos);
		}
	}
	{
		SDL_Color color;
		getColor(displayItemName->getColor(), &color.r, &color.g, &color.b, &color.a);
		color.a = (Uint8)(255 * animTooltip);
		displayItemName->setColor(makeColor(color.r, color.g, color.b, color.a));
	}
	{
		SDL_Color color;
		getColor(metalText->getColor(), &color.r, &color.g, &color.b, &color.a);
		color.a = (Uint8)(255 * animTooltip);
		metalText->setColor(makeColor(color.r, color.g, color.b, color.a));
	}
	{
		SDL_Color color;
		getColor(magicText->getColor(), &color.r, &color.g, &color.b, &color.a);
		color.a = (Uint8)(255 * animTooltip);
		magicText->setColor(makeColor(color.r, color.g, color.b, color.a));
	}
	//itemDisplayTooltip->setOpacity(100.0 * animTooltip);
	itemSlotFrame->setOpacity(100.0 * animTooltip);

	auto filterNavLeft = baseFrame->findImage("filter nav left");
	filterNavLeft->disabled = true;
	auto filterNavRight = baseFrame->findImage("filter nav right");
	filterNavRight->disabled = true;

	if ( usingGamepad )
	{
		filterNavLeft->path = Input::inputs[playernum].getGlyphPathForBinding("MenuPageLeft");
		if ( auto imgGet = Image::get(filterNavLeft->path.c_str()) )
		{
			filterNavLeft->pos.w = imgGet->getWidth();
			filterNavLeft->pos.h = imgGet->getHeight();
			filterNavLeft->disabled = false;
			filterNavLeft->pos.x = filterLeftSideX - filterNavLeft->pos.w - 6;
			filterNavLeft->pos.y = filterStartY + 10;
		}
		filterNavRight->path = Input::inputs[playernum].getGlyphPathForBinding("MenuPageRight");
		if ( auto imgGet = Image::get(filterNavRight->path.c_str()) )
		{
			filterNavRight->pos.w = imgGet->getWidth();
			filterNavRight->pos.h = imgGet->getHeight();
			filterNavRight->disabled = false;
			filterNavRight->pos.x = filterRightSideX + 6;
			filterNavRight->pos.y = filterStartY + 10;
		}
	}

	bool activateSelection = false;
	if ( isInteractable )
	{
		if ( !inputs.getUIInteraction(playernum)->selectedItem
			&& !player->GUI.isDropdownActive()
			&& player->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_TINKERING)
			&& player->bControlEnabled && !gamePaused
			&& !player->usingCommand() )
		{
			if ( Input::inputs[playernum].binaryToggle("MenuCancel") )
			{
				Input::inputs[playernum].consumeBinaryToggle("MenuCancel");
				parentGUI.closeGUI();
				return;
			}
			else
			{
				if ( usingGamepad && Input::inputs[playernum].binaryToggle("MenuConfirm") )
				{
					activateSelection = true;
					Input::inputs[playernum].consumeBinaryToggle("MenuConfirm");
				}
				else if ( !usingGamepad && Input::inputs[playernum].binaryToggle("MenuRightClick") )
				{
					activateSelection = true;
					Input::inputs[playernum].consumeBinaryToggle("MenuRightClick");
				}
				else if ( usingGamepad && Input::inputs[playernum].binaryToggle("MenuPageRight") )
				{
					if ( parentGUI.tinkeringFilter == TINKER_FILTER_CRAFTABLE )
					{
						parentGUI.tinkeringFilter = TINKER_FILTER_SALVAGEABLE;
						onTinkerChangeTabAction(playernum);
						animPromptMoveLeft = false;
					}
					else if ( parentGUI.tinkeringFilter == TINKER_FILTER_SALVAGEABLE )
					{
						parentGUI.tinkeringFilter = TINKER_FILTER_REPAIRABLE;
						onTinkerChangeTabAction(playernum);
						animPromptMoveLeft = false;
					}
					else if ( parentGUI.tinkeringFilter == TINKER_FILTER_REPAIRABLE )
					{
						//parentGUI.tinkeringFilter = TINKER_FILTER_CRAFTABLE;
					}
					else
					{
						parentGUI.tinkeringFilter = TINKER_FILTER_CRAFTABLE;
					}
					Input::inputs[playernum].consumeBinaryToggle("MenuPageRight");
				}
				else if ( usingGamepad && Input::inputs[playernum].binaryToggle("MenuPageLeft") )
				{
					if ( parentGUI.tinkeringFilter == TINKER_FILTER_CRAFTABLE )
					{
						//parentGUI.tinkeringFilter = TINKER_FILTER_REPAIRABLE;
					}
					else if ( parentGUI.tinkeringFilter == TINKER_FILTER_SALVAGEABLE )
					{
						parentGUI.tinkeringFilter = TINKER_FILTER_CRAFTABLE;
						onTinkerChangeTabAction(playernum);
						animPromptMoveLeft = true;
					}
					else if ( parentGUI.tinkeringFilter == TINKER_FILTER_REPAIRABLE )
					{
						parentGUI.tinkeringFilter = TINKER_FILTER_SALVAGEABLE;
						onTinkerChangeTabAction(playernum);
						animPromptMoveLeft = true;
					}
					else
					{
						parentGUI.tinkeringFilter = TINKER_FILTER_CRAFTABLE;
					}
					Input::inputs[playernum].consumeBinaryToggle("MenuPageLeft");
				}
			}
		}
	}

	if ( activateSelection && players[playernum] && players[playernum]->entity )
	{
		node_t* nextnode = nullptr;
		list_t* player_inventory = &parentGUI.tinkeringTotalItems;
		bool foundItem = false;
		bool checkConsumedItem = false;
		parentGUI.tinkeringBulkSalvage = false;
		parentGUI.tinkeringBulkSalvageMetalScrap = 0;
		parentGUI.tinkeringBulkSalvageMagicScrap = 0;
		int amountSalvaged = 1;
		bool actionOK = itemActionType == TINKER_ACTION_OK
			|| itemActionType == TINKER_ACTION_OK_UPGRADE
			|| itemActionType == TINKER_ACTION_OK_UNIDENTIFIED_SALVAGE;
		if ( player_inventory )
		{
			for ( node_t* node = player_inventory->first; node != NULL; node = nextnode )
			{
				nextnode = node->next;
				if ( node->element )
				{
					Item* item = (Item*)node->element;
					if ( parentGUI.tinkeringFilter == TINKER_FILTER_CRAFTABLE )
					{
						if ( isTinkerConstructItemSelected(item) )
						{
							foundItem = true;
							if ( parentGUI.tinkeringKitItem && parentGUI.tinkeringKitItem->status > BROKEN
								&& actionOK )
							{
								parentGUI.executeOnItemClick(item);
							}
							break;
						}
					}
					else if ( parentGUI.tinkeringFilter == TINKER_FILTER_SALVAGEABLE
						|| parentGUI.tinkeringFilter == TINKER_FILTER_REPAIRABLE )
					{
						if ( isSalvageOrRepairItemSelected(item) )
						{
							foundItem = true;
							if ( actionOK )
							{
								if ( modifierPressed && parentGUI.tinkeringFilter == TINKER_FILTER_SALVAGEABLE
									&& item->count > 1 )
								{
									int numActions = item->count;
									amountSalvaged = 0;
									parentGUI.tinkeringBulkSalvage = true;
									bool finalAction = false;
									for ( int i = 0; i < numActions
										&& parentGUI.tinkeringBulkSalvage
										&& parentGUI.tinkeringKitItem
										&& parentGUI.tinkeringKitItem->status > BROKEN; ++i )
									{
										if ( i == numActions - 1 )
										{
											finalAction = true;
											parentGUI.tinkeringBulkSalvage = false;
											checkConsumedItem = true;
										}
										assert(item->count >= 1);
										parentGUI.executeOnItemClick(item);
										if ( parentGUI.tinkeringBulkSalvage || finalAction )
										{
											++amountSalvaged;
										}
									}
									parentGUI.tinkeringBulkSalvage = false;
								}
								else
								{
									if ( parentGUI.tinkeringFilter == TINKER_FILTER_SALVAGEABLE )
									{
										if ( item->count == 1 )
										{
											checkConsumedItem = true;
										}
									}
									if ( (parentGUI.tinkeringKitItem && parentGUI.tinkeringKitItem->status > BROKEN)
										|| (parentGUI.tinkeringKitItem == item && parentGUI.tinkeringFilter == TINKER_FILTER_REPAIRABLE))
									{
										parentGUI.executeOnItemClick(item);
									}
								}
							}
							break;
						}
					}
				}
			}
		}
		if ( foundItem )
		{
			parentGUI.rebuildGUIInventory();

			animInvalidAction = 0.0;
			animInvalidActionTicks = 0;
			invalidActionType = INVALID_ACTION_NONE;
			switch ( itemActionType )
			{
				case TINKER_ACTION_NO_MATERIALS:
				case TINKER_ACTION_NO_MATERIALS_UPGRADE:
					if ( metalScrapPrice > 0
						&& metalScrapPrice > heldMetalScrap
						&& magicScrapPrice > 0
						&& magicScrapPrice > heldMagicScrap )
					{
						invalidActionType = INVALID_ACTION_SHAKE_ALL_SCRAP;
					}
					else if ( metalScrapPrice > 0
						&& metalScrapPrice > heldMetalScrap )
					{
						invalidActionType = INVALID_ACTION_SHAKE_METAL_SCRAP;
					}
					else if ( magicScrapPrice > 0
						&& magicScrapPrice > heldMagicScrap )
					{
						invalidActionType = INVALID_ACTION_SHAKE_MAGIC_SCRAP;
					}
					animInvalidAction = 1.0;
					animInvalidActionTicks = ticks;
					// play bad feedback sfx
					playSound(90, 64);
					break;
				case TINKER_ACTION_INVALID_ITEM:
				case TINKER_ACTION_INVALID_ROBOT_TO_SALVAGE:
				case TINKER_ACTION_NO_SKILL_LVL:
				case TINKER_ACTION_NO_SKILL_LVL_UPGRADE:
				case TINKER_ACTION_ITEM_FULLY_REPAIRED:
				case TINKER_ACTION_ITEM_FULLY_UPGRADED:
				case TINKER_ACTION_ROBOT_BROKEN:
				case TINKER_ACTION_MUST_BE_UNEQUIPPED:
				case TINKER_ACTION_ALREADY_USING_THIS_TINKERING_KIT:
				case TINKER_ACTION_KIT_NEEDS_REPAIRS:
				case TINKER_ACTION_NOT_IDENTIFIED_YET:
					invalidActionType = INVALID_ACTION_SHAKE_PROMPT;
					animInvalidAction = 1.0;
					animInvalidActionTicks = ticks;
					// play bad feedback sfx
					playSound(90, 64);
					break;
				case TINKER_ACTION_OK:
				case TINKER_ACTION_OK_UPGRADE:
				case TINKER_ACTION_OK_UNIDENTIFIED_SALVAGE:
				{
					int newMetalTotal = parentGUI.tinkeringCountScrapTotal(TOOL_METAL_SCRAP);
					int newMagicTotal = parentGUI.tinkeringCountScrapTotal(TOOL_MAGIC_SCRAP);
					int diffMetal = heldMetalScrap - newMetalTotal;
					int diffMagic = heldMagicScrap - newMagicTotal;
					tinkerScrapChangeEvent(parentGUI.getPlayer(), -diffMetal, -diffMagic, heldMetalScrap, heldMagicScrap);
					if ( parentGUI.tinkeringFilter == TINKER_FILTER_SALVAGEABLE && checkConsumedItem )
					{
						// immediately fade the tooltip on mouse control
						animTooltipTicks = 0;
					}
					if ( diffMetal != 0 || diffMagic != 0 )
					{
						if ( parentGUI.tinkeringFilter == TINKER_FILTER_SALVAGEABLE )
						{
							itemIncrementText->setText(std::to_string(-amountSalvaged).c_str());
						}
						else if ( parentGUI.tinkeringFilter == TINKER_FILTER_REPAIRABLE )
						{
							itemIncrementText->setText("  +");
						}
						else if ( parentGUI.tinkeringFilter == TINKER_FILTER_CRAFTABLE )
						{
							itemIncrementText->setText("+1");
						}
					}
					break;
				}
				default:
					break;
			}
		}
	}
}

void GenericGUIMenu::TinkerGUI_t::createTinkerMenu()
{
	const int player = parentGUI.getPlayer();
	if ( !gui || !tinkerFrame || !players[player]->inventoryUI.frame )
	{
		return;
	}
	if ( tinkerGUIHasBeenCreated() )
	{
		return;
	}

	SDL_Rect basePos{ 0, 0, tinkerBaseWidth, 304 };
	{
		auto drawerFrame = tinkerFrame->addFrame("tinker drawer");
		SDL_Rect drawerPos{ 0, 0, 258, 228 };
		drawerFrame->setSize(drawerPos);
		drawerFrame->setHollow(true);
		auto bg = drawerFrame->addImage(drawerPos,
			makeColor(255, 255, 255, 255),
			"images/ui/Tinkering/Tinker_Construct_Drawer_00.png", "tinker drawer img");
		drawerFrame->setDisabled(true);

		const int inventorySlotSize = players[player]->inventoryUI.getSlotSize();

		tinkerSlotFrames.clear();

		const int baseSlotOffsetX = 0;
		const int baseSlotOffsetY = 0;

		SDL_Rect tinkerSlotsPos{ 0, 38, 258, 150 };
		{
			const auto drawerSlotsFrame = drawerFrame->addFrame("drawer slots");
			drawerSlotsFrame->setSize(tinkerSlotsPos);
			drawerSlotsFrame->setHollow(true);

			auto gridImg = drawerSlotsFrame->addImage(SDL_Rect{ 0, 0, tinkerSlotsPos.w, tinkerSlotsPos.h },
			makeColor(255, 255, 255, 255), "images/ui/Tinkering/Tinker_Construct_DrawerSlots_00.png", "grid img");

			SDL_Rect currentSlotPos{ baseSlotOffsetX, baseSlotOffsetY, inventorySlotSize, inventorySlotSize };
			const int maxTinkerX = MAX_TINKER_X;
			const int maxTinkerY = MAX_TINKER_Y;

			const int slotInnerWidth = inventorySlotSize - 2;
			std::vector<std::pair<int, int>> slotCoords =
			{
				std::make_pair(28 + (slotInnerWidth + 2) * 0, 0),
				std::make_pair(28 + (slotInnerWidth + 2) * 1, 0),
				std::make_pair(28 + (slotInnerWidth + 2) * 2, 0),
				std::make_pair(28 + (slotInnerWidth + 2) * 3, 0),
				std::make_pair(28 + (slotInnerWidth + 2) * 4, 0),

				std::make_pair(22, 48),
				std::make_pair(22 + (slotInnerWidth + 6), 48),
				std::make_pair(22 + (slotInnerWidth + 6) + (slotInnerWidth + 4) * 1, 48),
				std::make_pair(22 + (slotInnerWidth + 6) + (slotInnerWidth + 4) * 2, 48),
				std::make_pair(22 + (slotInnerWidth + 6) * 2 + (slotInnerWidth + 4) * 2, 48),

				std::make_pair(14, 110),
				std::make_pair(14 + (slotInnerWidth + 10), 110),
				std::make_pair(14 + (slotInnerWidth + 10) + (slotInnerWidth + 8) * 1, 110),
				std::make_pair(14 + (slotInnerWidth + 10) + (slotInnerWidth + 8) * 2, 110),
				std::make_pair(14 + (slotInnerWidth + 10) * 2 + (slotInnerWidth + 8) * 2, 110)
			};
			auto slotCoordsIt = slotCoords.begin();
			for ( int y = 0; y < maxTinkerY; ++y )
			{
				for ( int x = 0; x < maxTinkerX; ++x )
				{
					currentSlotPos.x = slotCoordsIt->first;
					currentSlotPos.y = slotCoordsIt->second;
					char slotname[32] = "";
					snprintf(slotname, sizeof(slotname), "tinker %d %d", x, y);

					auto slotFrame = drawerSlotsFrame->addFrame(slotname);
					tinkerSlotFrames[x + y * 100] = slotFrame;
					SDL_Rect slotPos{ currentSlotPos.x, currentSlotPos.y, inventorySlotSize, inventorySlotSize };
					slotFrame->setSize(slotPos);

					createPlayerInventorySlotFrameElements(slotFrame);
					if ( slotCoordsIt != slotCoords.end() )
					{
						++slotCoordsIt;
					}
					//slotFrame->addImage(SDL_Rect{ 0, 0, inventorySlotSize, inventorySlotSize }, 0xFFFFFFFF,
					//	"images/system/white.png", "tmp");
					slotFrame->setDisabled(true);
				}
			}
		}
	}

	{
		auto bgFrame = tinkerFrame->addFrame("tinker base");
		bgFrame->setSize(basePos);
		bgFrame->setHollow(true);
		bgFrame->setDisabled(true);
		auto bg = bgFrame->addImage(SDL_Rect{ 0, 0, basePos.w, basePos.h },
			makeColor(255, 255, 255, 255),
			"images/ui/Tinkering/Tinker_Construct_Base_00.png", "tinker base img");

		auto headerFont = "fonts/pixel_maz_multiline.ttf#16#2";
		auto tinkerKitTitle = bgFrame->addField("tinker kit title", 128);
		tinkerKitTitle->setFont(headerFont);
		tinkerKitTitle->setText("");
		tinkerKitTitle->setHJustify(Field::justify_t::CENTER);
		tinkerKitTitle->setVJustify(Field::justify_t::TOP);
		tinkerKitTitle->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tinkerKitTitle->setTextColor(hudColors.characterSheetLightNeutral);
		tinkerKitTitle->setOutlineColor(makeColor(29, 16, 11, 255));
		auto tinkerKitStatus = bgFrame->addField("tinker kit status", 128);
		tinkerKitStatus->setFont(headerFont);
		tinkerKitStatus->setText("");
		tinkerKitStatus->setHJustify(Field::justify_t::CENTER);
		tinkerKitStatus->setVJustify(Field::justify_t::TOP);
		tinkerKitStatus->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tinkerKitStatus->setTextColor(hudColors.characterSheetLightNeutral);
		tinkerKitStatus->setOutlineColor(makeColor(29, 16, 11, 255));

		auto itemFont = "fonts/pixel_maz_multiline.ttf#16#2";
		auto itemDisplayTooltip = bgFrame->addFrame("tinker display tooltip");
		itemDisplayTooltip->setSize(SDL_Rect{ 0, 0, 298, 108 });
		itemDisplayTooltip->setHollow(true);
		itemDisplayTooltip->setInheritParentFrameOpacity(false);
		{
			auto tooltipBg = itemDisplayTooltip->addImage(SDL_Rect{ 0, 0, 298, 108 },
				0xFFFFFFFF, "images/ui/Tinkering/Tinker_Tooltip_00.png", "tooltip img");

			auto itemNameText = itemDisplayTooltip->addField("item display name", 1024);
			itemNameText->setFont(itemFont);
			itemNameText->setText("");
			itemNameText->setHJustify(Field::justify_t::LEFT);
			itemNameText->setVJustify(Field::justify_t::TOP);
			itemNameText->setSize(SDL_Rect{ 0, 0, 0, 0 });
			itemNameText->setColor(hudColors.characterSheetLightNeutral);

			auto itemDisplayTextBg = itemDisplayTooltip->addImage(SDL_Rect{ 0, 0, 220, 42 },
				0xFFFFFFFF, "images/ui/Tinkering/Tinker_LabelName_2Row_00.png", "item text img");

			auto itemCostBg = itemDisplayTooltip->addImage(SDL_Rect{ 0, 0, 144, 34 },
				0xFFFFFFFF, "images/ui/Tinkering/Tinker_CostBacking_00.png", "item cost img");

			auto metalText = itemDisplayTooltip->addField("item metal value", 32);
			metalText->setFont(itemFont);
			metalText->setText("");
			metalText->setHJustify(Field::justify_t::RIGHT);
			metalText->setVJustify(Field::justify_t::TOP);
			metalText->setSize(SDL_Rect{ 0, 0, 0, 0 });
			metalText->setColor(hudColors.characterSheetLightNeutral);
			auto magicText = itemDisplayTooltip->addField("item magic value", 32);
			magicText->setFont(itemFont);
			magicText->setText("");
			magicText->setHJustify(Field::justify_t::RIGHT);
			magicText->setVJustify(Field::justify_t::TOP);
			magicText->setSize(SDL_Rect{ 0, 0, 0, 0 });
			magicText->setColor(hudColors.characterSheetLightNeutral);

			auto costScrapText = itemDisplayTooltip->addField("item cost label", 64);
			costScrapText->setFont(itemFont);
			costScrapText->setText("");
			costScrapText->setHJustify(Field::justify_t::RIGHT);
			costScrapText->setVJustify(Field::justify_t::TOP);
			costScrapText->setSize(SDL_Rect{ 0, 0, 90, 0 });
			costScrapText->setColor(hudColors.characterSheetLightNeutral);

			auto itemBgImg = itemDisplayTooltip->addImage(SDL_Rect{ 0, 0, 54, 54 }, 0xFFFFFFFF,
				"*images/ui/Tinkering/Tinker_ItemBGSurround_00.png", "item bg img");

			auto slotFrame = itemDisplayTooltip->addFrame("item slot frame");
			SDL_Rect slotPos{ 0, 0, players[player]->inventoryUI.getSlotSize(), players[player]->inventoryUI.getSlotSize() };
			slotFrame->setSize(slotPos);
			slotFrame->setDisabled(true);
			slotFrame->setInheritParentFrameOpacity(false);
			createPlayerInventorySlotFrameElements(slotFrame);
		}

		{
			auto closeBtn = bgFrame->addButton("close tinker button");
			SDL_Rect closeBtnPos{ basePos.w - 8 - 26, 8, 26, 26 };
			closeBtn->setSize(closeBtnPos);
			closeBtn->setColor(makeColor(255, 255, 255, 255));
			closeBtn->setHighlightColor(makeColor(255, 255, 255, 255));
			closeBtn->setText("X");
			closeBtn->setFont(itemFont);
			closeBtn->setHideGlyphs(true);
			closeBtn->setHideKeyboardGlyphs(true);
			closeBtn->setHideSelectors(true);
			closeBtn->setMenuConfirmControlType(0);
			closeBtn->setBackground("images/ui/Tinkering/Button_X_00.png");
			closeBtn->setBackgroundHighlighted("images/ui/Tinkering/Button_XHigh_00.png");
			closeBtn->setBackgroundActivated("images/ui/Tinkering/Button_XPress_00.png");
			closeBtn->setTextHighlightColor(makeColor(201, 162, 100, 255));
			closeBtn->setCallback([](Button& button) {
				GenericGUI[button.getOwner()].closeGUI();
			});
			closeBtn->setTickCallback([](Widget& widget)
			{
				genericgui_deselect_fn(widget);
			});

			auto closeGlyph = bgFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
				0xFFFFFFFF, "", "close tinker glyph");
			closeGlyph->disabled = true;
			closeGlyph->ontop = true;
		}

		{
			auto actionPromptTxt = bgFrame->addField("action prompt txt", 64);
			actionPromptTxt->setFont(itemFont);
			actionPromptTxt->setText("");
			actionPromptTxt->setHJustify(Field::justify_t::RIGHT);
			actionPromptTxt->setVJustify(Field::justify_t::TOP);
			actionPromptTxt->setSize(SDL_Rect{ 0, 0, 0, 0 });
			actionPromptTxt->setColor(makeColor(255, 255, 255, 255));

			auto actionPromptGlyph = bgFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
				0xFFFFFFFF, "", "action prompt glyph");
			actionPromptGlyph->ontop = true;

			auto actionModifierGlyph = bgFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
				0xFFFFFFFF, "", "action modifier glyph");
			actionModifierGlyph->ontop = true;
			actionModifierGlyph->disabled = true;
		}

		{
			auto actionPromptUnselectedTxt = bgFrame->addField("action prompt unselected txt", 64);
			actionPromptUnselectedTxt->setFont(itemFont);
			actionPromptUnselectedTxt->setText("");
			actionPromptUnselectedTxt->setHJustify(Field::justify_t::CENTER);
			actionPromptUnselectedTxt->setVJustify(Field::justify_t::TOP);
			actionPromptUnselectedTxt->setSize(SDL_Rect{ 0, 0, 0, 0 });
			actionPromptUnselectedTxt->setColor(makeColor(255, 255, 255, 255));
			actionPromptUnselectedTxt->setDisabled(true);

			auto actionPromptCoverLeftImg = bgFrame->addImage(SDL_Rect{ 0, 60, 56, 26 },
				0xFFFFFFFF, "images/ui/Tinkering/Tinker_PromptCoverLeft_00.png", "action prompt lcover");
			actionPromptCoverLeftImg->ontop = true;
			auto actionPromptCoverRightImg = bgFrame->addImage(SDL_Rect{ bg->pos.w - 56, 60, 56, 26 },
				0xFFFFFFFF, "images/ui/Tinkering/Tinker_PromptCoverRight_00.png", "action prompt rcover");
			actionPromptCoverRightImg->ontop = true;
		}

		{
			auto itemIncrementText = bgFrame->addField("item increment txt", 64);
			itemIncrementText->setFont(itemFont);
			itemIncrementText->setText("");
			itemIncrementText->setHJustify(Field::justify_t::TOP);
			itemIncrementText->setVJustify(Field::justify_t::LEFT);
			itemIncrementText->setSize(SDL_Rect{ 0, 0, 0, 0 });
			itemIncrementText->setColor(hudColors.characterSheetLightNeutral);
			itemIncrementText->setDisabled(true);
			itemIncrementText->setOntop(true);
		}

		{
			// filter labels
			Button* filterBtn = bgFrame->addButton("filter salvage btn");
			filterBtn->setColor(makeColor(255, 255, 255, 0));
			filterBtn->setHighlightColor(makeColor(255, 255, 255, 0));
			filterBtn->setText("Salvage");
			filterBtn->setFont(itemFont);
			filterBtn->setTextHighlightColor(makeColor(201, 162, 100, 255));
			filterBtn->setHideGlyphs(true);
			filterBtn->setHideKeyboardGlyphs(true);
			filterBtn->setHideSelectors(true);
			filterBtn->setMenuConfirmControlType(0);
			filterBtn->setCallback([](Button& button) {
				auto oldTab = GenericGUI[button.getOwner()].tinkeringFilter;
				bool changeToDifferentTab = oldTab != GenericGUIMenu::TINKER_FILTER_SALVAGEABLE;
				GenericGUI[button.getOwner()].tinkeringFilter = GenericGUIMenu::TINKER_FILTER_SALVAGEABLE;
				onTinkerChangeTabAction(button.getOwner(), changeToDifferentTab);
				if ( oldTab == GenericGUIMenu::TINKER_FILTER_CRAFTABLE )
				{
					GenericGUI[button.getOwner()].tinkerGUI.animPromptMoveLeft = false;
				}
				else
				{
					GenericGUI[button.getOwner()].tinkerGUI.animPromptMoveLeft = true;
				}
			});
			filterBtn->setTickCallback([](Widget& widget)
			{
				genericgui_deselect_fn(widget);
			});

			filterBtn = bgFrame->addButton("filter craft btn");
			filterBtn->setColor(makeColor(255, 255, 255, 0));
			filterBtn->setHighlightColor(makeColor(255, 255, 255, 0));
			filterBtn->setText("Craft");
			filterBtn->setFont(itemFont);
			filterBtn->setTextHighlightColor(makeColor(201, 162, 100, 255));
			filterBtn->setHideGlyphs(true);
			filterBtn->setHideKeyboardGlyphs(true);
			filterBtn->setHideSelectors(true);
			filterBtn->setMenuConfirmControlType(0);
			filterBtn->setCallback([](Button& button) {
				auto oldTab = GenericGUI[button.getOwner()].tinkeringFilter;
				bool changeToDifferentTab = oldTab != GenericGUIMenu::TINKER_FILTER_CRAFTABLE;
				GenericGUI[button.getOwner()].tinkeringFilter = GenericGUIMenu::TINKER_FILTER_CRAFTABLE;
				onTinkerChangeTabAction(button.getOwner(), changeToDifferentTab);
				GenericGUI[button.getOwner()].tinkerGUI.animPromptMoveLeft = true;
			});
			filterBtn->setTickCallback([](Widget& widget)
			{
				genericgui_deselect_fn(widget);
			});

			filterBtn = bgFrame->addButton("filter repair btn");
			filterBtn->setColor(makeColor(255, 255, 255, 0));
			filterBtn->setHighlightColor(makeColor(255, 255, 255, 0));
			filterBtn->setText("Repair");
			filterBtn->setFont(itemFont);
			filterBtn->setTextHighlightColor(makeColor(201, 162, 100, 255));
			filterBtn->setHideGlyphs(true);
			filterBtn->setHideKeyboardGlyphs(true);
			filterBtn->setHideSelectors(true);
			filterBtn->setMenuConfirmControlType(0);
			filterBtn->setCallback([](Button& button) {
				auto oldTab = GenericGUI[button.getOwner()].tinkeringFilter;
				bool changeToDifferentTab = oldTab != GenericGUIMenu::TINKER_FILTER_REPAIRABLE;
				GenericGUI[button.getOwner()].tinkeringFilter = GenericGUIMenu::TINKER_FILTER_REPAIRABLE;
				onTinkerChangeTabAction(button.getOwner(), changeToDifferentTab);
				GenericGUI[button.getOwner()].tinkerGUI.animPromptMoveLeft = false;
			});
			filterBtn->setTickCallback([](Widget& widget)
			{
				genericgui_deselect_fn(widget);
			});

			auto filterNavLeft = bgFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
				0xFFFFFFFF, "", "filter nav left");
			filterNavLeft->disabled = true;
			auto filterNavRight = bgFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
				0xFFFFFFFF, "", "filter nav right");
			filterNavRight->disabled = true;
		}

		{
			auto heldScrapBg = bgFrame->addImage(SDL_Rect{ 0, 0, 176, 34 },
				0xFFFFFFFF, "images/ui/Tinkering/Tinker_ScrapBacking_00.png", "held scrap img");

			auto heldScrapText = bgFrame->addField("held scrap label", 64);
			heldScrapText->setFont(itemFont);
			heldScrapText->setText("");
			heldScrapText->setHJustify(Field::justify_t::RIGHT);
			heldScrapText->setVJustify(Field::justify_t::TOP);
			heldScrapText->setSize(SDL_Rect{ 0, 0, 90, 0 });
			heldScrapText->setColor(hudColors.characterSheetLightNeutral);

			auto metalHeldText = bgFrame->addField("held metal txt", 32);
			metalHeldText->setFont(itemFont);
			metalHeldText->setText("");
			metalHeldText->setHJustify(Field::justify_t::RIGHT);
			metalHeldText->setVJustify(Field::justify_t::TOP);
			metalHeldText->setSize(SDL_Rect{ 0, 0, 0, 0 });
			metalHeldText->setColor(hudColors.characterSheetLightNeutral);
			auto magicHeldText = bgFrame->addField("held magic txt", 32);
			magicHeldText->setFont(itemFont);
			magicHeldText->setText("");
			magicHeldText->setHJustify(Field::justify_t::RIGHT);
			magicHeldText->setVJustify(Field::justify_t::TOP);
			magicHeldText->setSize(SDL_Rect{ 0, 0, 0, 0 });
			magicHeldText->setColor(hudColors.characterSheetLightNeutral);
		}
	}
}

bool GenericGUIMenu::TinkerGUI_t::isTinkerConstructItemSelected(Item* item)
{
	if ( !item || itemCategory(item) == SPELL_CAT )
	{
		return false;
	}

	if ( !isConstructMenuActive() || !parentGUI.isNodeTinkeringCraftableItem(item->node) )
	{
		return false;
	}

	if ( players[parentGUI.getPlayer()]->GUI.activeModule == Player::GUI_t::MODULE_TINKERING )
	{
		if ( selectedTinkerSlotX >= 0 && selectedTinkerSlotX < MAX_TINKER_X
			&& selectedTinkerSlotY >= 0 && selectedTinkerSlotY < MAX_TINKER_Y
			&& item->x == selectedTinkerSlotX && item->y == selectedTinkerSlotY )
		{
			if ( auto slotFrame = getTinkerSlotFrame(item->x, item->y) )
			{
				return slotFrame->capturesMouse();
			}
			else
			{
				return false;
			}
		}
	}

	return false;
}

bool GenericGUIMenu::TinkerGUI_t::isSalvageOrRepairItemSelected(Item* item)
{
	if ( !item || itemCategory(item) == SPELL_CAT )
	{
		return false;
	}

	if ( !isSalvageOrRepairMenuActive() || !parentGUI.isNodeFromPlayerInventory(item->node) )
	{
		return false;
	}

	if ( players[parentGUI.getPlayer()]->GUI.activeModule == Player::GUI_t::MODULE_INVENTORY )
	{
		auto& inventoryUI = players[parentGUI.getPlayer()]->inventoryUI;
		auto& paperDoll = players[parentGUI.getPlayer()]->paperDoll;
		if ( item->y < 0 && paperDoll.getSlotForItem(*item) != Player::PaperDoll_t::SLOT_MAX )
		{
			int slotx, sloty;
			paperDoll.getCoordinatesFromSlotType(paperDoll.getSlotForItem(*item), slotx, sloty);
			if ( slotx == inventoryUI.getSelectedSlotX() && sloty == inventoryUI.getSelectedSlotY() )
			{
				if ( auto slotFrame = inventoryUI.getInventorySlotFrame(slotx, sloty) )
				{
					return slotFrame->capturesMouse();
				}
				else
				{
					return false;
				}
			}
		}
		else
		{
			if ( inventoryUI.getSelectedSlotX() >= 0 
				&& inventoryUI.getSelectedSlotX() < inventoryUI.getSizeX()
				&& inventoryUI.getSelectedSlotY() >= Player::Inventory_t::PaperDollRows::DOLL_ROW_1 
				&& inventoryUI.getSelectedSlotY() < inventoryUI.getSizeY()
				&& item->x == inventoryUI.getSelectedSlotX() && item->y == inventoryUI.getSelectedSlotY() )
			{
				if ( auto slotFrame = inventoryUI.getInventorySlotFrame(item->x, item->y) )
				{
					return slotFrame->capturesMouse();
				}
				else
				{
					return false;
				}
			}
		}
	}

	return false;
}

void GenericGUIMenu::TinkerGUI_t::selectTinkerSlot(const int x, const int y)
{
	selectedTinkerSlotX = x;
	selectedTinkerSlotY = y;
}

Frame* GenericGUIMenu::TinkerGUI_t::getTinkerSlotFrame(int x, int y) const
{
	if ( tinkerFrame )
	{
		int key = x + y * 100;
		if ( tinkerSlotFrames.find(key) != tinkerSlotFrames.end() )
		{
			return tinkerSlotFrames.at(key);
		}
		//assert(tinkerSlotFrames.find(key) == tinkerSlotFrames.end());
	}
	return nullptr;
}

GenericGUIMenu::TinkerGUI_t::TinkerActions_t GenericGUIMenu::TinkerGUI_t::setItemDisplayNameAndPrice(Item* item, bool checkStatusOnly)
{
	if ( !checkStatusOnly )
	{
		if ( !item || item->type == SPELL_ITEM )
		{
			clearItemDisplayed();
		}

		char buf[1024];
		if ( !item->identified )
		{
			snprintf(buf, sizeof(buf), "%s %s (?)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), item->getName());
		}
		else
		{
			if ( (item->type == TOOL_SENTRYBOT || item->type == TOOL_DUMMYBOT || item->type == TOOL_SPELLBOT
				|| item->type == TOOL_GYROBOT) )
			{
				int health = 100;
				if ( !item->tinkeringBotIsMaxHealth() )
				{
					health = 25 * (item->appearance % 10);
					if ( health == 0 && item->status != BROKEN )
					{
						health = 5;
					}
				}
				snprintf(buf, sizeof(buf), "%s %s (%d%%)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), item->getName(), health);
			}
			else
			{
				snprintf(buf, sizeof(buf), "%s %s (%+d)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), item->getName(), item->beatitude);
			}
		}
		if ( itemDesc != buf )
		{
			itemRequiresTitleReflow = true;
		}
		itemDesc = buf;
		itemActionType = TINKER_ACTION_NONE;
		itemType = item->type;
		itemRequirement = -1;
	}

	auto actionResult = TINKER_ACTION_NONE;

	bool isTinkeringBot = (item->type == TOOL_SENTRYBOT
		|| item->type == TOOL_SPELLBOT
		|| item->type == TOOL_DUMMYBOT
		|| item->type == TOOL_GYROBOT);


	const int player = parentGUI.getPlayer();
	bool tinkeringKitNeedsRepairs = parentGUI.tinkeringKitItem && parentGUI.tinkeringKitItem->status == BROKEN;

	if ( parentGUI.isNodeTinkeringCraftableItem(item->node) && parentGUI.tinkeringFilter == TINKER_FILTER_CRAFTABLE )
	{
		tinkeringGetCraftingCost(item, &metalScrapPrice, &magicScrapPrice);
		if ( tinkeringKitNeedsRepairs )
		{
			actionResult = TINKER_ACTION_KIT_NEEDS_REPAIRS;
		}
		else if ( parentGUI.tinkeringPlayerHasSkillLVLToCraft(item) == -1 )
		{
			actionResult = TINKER_ACTION_NO_SKILL_LVL;

			if ( !checkStatusOnly )
			{
				int oldTinkering = stats[player]->PROFICIENCIES[PRO_LOCKPICKING];
				stats[player]->PROFICIENCIES[PRO_LOCKPICKING] = 100;
				Sint32 oldPER = stats[player]->PER;
				stats[player]->PER += -statGetPER(stats[player], players[player]->entity);
				itemRequirement = parentGUI.tinkeringPlayerHasSkillLVLToCraft(item) * 20; // manually hack this to max to get requirement
				stats[player]->PER = oldPER;
				stats[player]->PROFICIENCIES[PRO_LOCKPICKING] = oldTinkering;
			}
		}
		else if ( !parentGUI.tinkeringPlayerCanAffordCraft(item) )
		{
			actionResult = TINKER_ACTION_NO_MATERIALS;
		}
		else
		{
			actionResult = TINKER_ACTION_OK;
		}
	}
	else if ( parentGUI.tinkeringFilter == TINKER_FILTER_SALVAGEABLE )
	{
		bool hasValue = false;
		if ( !checkStatusOnly )
		{
			hasValue = tinkeringGetItemValue(item, &metalScrapPrice, &magicScrapPrice);
		}
		else
		{
			int dummy1 = 0;
			int dummy2 = 0;
			hasValue = tinkeringGetItemValue(item, &dummy1, &dummy2);
		}

		if ( tinkeringKitNeedsRepairs )
		{
			actionResult = TINKER_ACTION_KIT_NEEDS_REPAIRS;
		}
		else if ( hasValue && item == parentGUI.tinkeringKitItem )
		{
			actionResult = TINKER_ACTION_ALREADY_USING_THIS_TINKERING_KIT;
		}
		else if ( hasValue && parentGUI.isNodeFromPlayerInventory(item->node) && itemIsEquipped(item, player) )
		{
			actionResult = TINKER_ACTION_MUST_BE_UNEQUIPPED;
		}
		else if ( !parentGUI.isItemSalvageable(item, player) )
		{
			if ( isTinkeringBot || itemIsThrowableTinkerTool(item) )
			{
				actionResult = TINKER_ACTION_INVALID_ROBOT_TO_SALVAGE;
			}
			else
			{
				actionResult = TINKER_ACTION_INVALID_ITEM;
			}
		}
		else
		{
			if ( item->identified )
			{
				actionResult = TINKER_ACTION_OK;
			}
			else
			{
				actionResult = TINKER_ACTION_OK_UNIDENTIFIED_SALVAGE;
			}
		}
	}
	else if ( parentGUI.tinkeringFilter == TINKER_FILTER_REPAIRABLE )
	{
		bool repairable = false;
		if ( !checkStatusOnly )
		{
			repairable = parentGUI.tinkeringGetRepairCost(item, &metalScrapPrice, &magicScrapPrice);
			if ( !isTinkeringBot )
			{
				int metalTmp = 0;
				int magicTmp = 0;
				getGeneralItemRepairCostWithoutRequirements(player, item, metalTmp, magicTmp); // manually grab price
				if ( metalTmp > 0 || magicTmp > 0 )
				{
					metalScrapPrice = metalTmp;
					magicScrapPrice = magicTmp;
				}
			}
		}
		else
		{
			int dummy1 = 0;
			int dummy2 = 0;
			repairable = parentGUI.tinkeringGetRepairCost(item, &dummy1, &dummy2);
		}
		int requirement = parentGUI.tinkeringRepairGeneralItemSkillRequirement(item);

		int skillLVL = stats[player]->PROFICIENCIES[PRO_LOCKPICKING] 
			+ statGetPER(stats[player], players[player]->entity);
		if ( !item->identified )
		{
			actionResult = TINKER_ACTION_NOT_IDENTIFIED_YET;
		}
		else if ( tinkeringKitNeedsRepairs && item != parentGUI.tinkeringKitItem )
		{
			actionResult = TINKER_ACTION_KIT_NEEDS_REPAIRS;
		}
		else if ( isTinkeringBot && item->status == BROKEN )
		{
			actionResult = TINKER_ACTION_ROBOT_BROKEN;
		}
		else if ( isTinkeringBot && (item->tinkeringBotIsMaxHealth() && item->status == EXCELLENT) )
		{
			actionResult = TINKER_ACTION_ITEM_FULLY_UPGRADED;
		}
		else if ( item->type != TOOL_TINKERING_KIT && !isTinkeringBot && requirement < 0 )
		{
			actionResult = TINKER_ACTION_INVALID_ITEM;
		}
		else if ( !isTinkeringBot && item->status >= EXCELLENT )
		{
			actionResult = TINKER_ACTION_ITEM_FULLY_REPAIRED;
		}
		else if ( item->type != TOOL_TINKERING_KIT && !isTinkeringBot
			&& skillLVL < requirement )
		{
			actionResult = TINKER_ACTION_NO_SKILL_LVL;
			if ( !checkStatusOnly )
			{
				itemRequirement = requirement;
			}
		}
		else if ( isTinkeringBot && !item->tinkeringBotIsMaxHealth()
			&& (parentGUI.tinkeringPlayerHasSkillLVLToCraft(item) == -1 /*|| parentGUI.tinkeringUpgradeMaxStatus(item) <= item->status)*/) )
		{
			actionResult = TINKER_ACTION_NO_SKILL_LVL;

			if ( !checkStatusOnly )
			{
				// can't craft, figure out base requirement
				int oldTinkering = stats[player]->PROFICIENCIES[PRO_LOCKPICKING];
				stats[player]->PROFICIENCIES[PRO_LOCKPICKING] = 100;
				Sint32 oldPER = stats[player]->PER;
				stats[player]->PER += -statGetPER(stats[player], players[player]->entity);
				itemRequirement = parentGUI.tinkeringPlayerHasSkillLVLToCraft(item) * 20; // manually hack this to max to get requirement
				stats[player]->PER = oldPER;
				stats[player]->PROFICIENCIES[PRO_LOCKPICKING] = oldTinkering;
			}
		}
		else if ( isTinkeringBot && item->tinkeringBotIsMaxHealth()
			&& (parentGUI.tinkeringPlayerHasSkillLVLToCraft(item) == -1 || parentGUI.tinkeringUpgradeMaxStatus(item) <= item->status) )
		{
			actionResult = TINKER_ACTION_NO_SKILL_LVL_UPGRADE;
			if ( !checkStatusOnly )
			{
				int craftRequirement = parentGUI.tinkeringPlayerHasSkillLVLToCraft(item);
				if ( craftRequirement >= 0 )
				{
					// can craft, figure out how much needed to upgrade
					int diff = 0;
					switch ( item->status )
					{
						// intentional fall-through
						case EXCELLENT:
							diff += 20;
						case SERVICABLE:
							diff += 20;
						case WORN:
							diff += 20;
						case DECREPIT:
							diff += 20;
						default:
							break;
					}
					itemRequirement = std::min(100, craftRequirement * 20 + diff);
				}
				else
				{
					int oldTinkering = stats[player]->PROFICIENCIES[PRO_LOCKPICKING];
					stats[player]->PROFICIENCIES[PRO_LOCKPICKING] = 100;
					Sint32 oldPER = stats[player]->PER;
					stats[player]->PER += -statGetPER(stats[player], players[player]->entity);
					itemRequirement = parentGUI.tinkeringPlayerHasSkillLVLToCraft(item) * 20; // manually hack this to max to get requirement
					stats[player]->PER = oldPER;
					stats[player]->PROFICIENCIES[PRO_LOCKPICKING] = oldTinkering;
				}
			}
		}
		else if ( !parentGUI.tinkeringPlayerCanAffordRepair(item) )
		{
			if ( isTinkeringBot && item->tinkeringBotIsMaxHealth() )
			{
				actionResult = TINKER_ACTION_NO_MATERIALS_UPGRADE;
			}
			else
			{
				actionResult = TINKER_ACTION_NO_MATERIALS;
			}
		}
		else if ( repairable )
		{
			if ( isTinkeringBot && item->tinkeringBotIsMaxHealth() )
			{
				actionResult = TINKER_ACTION_OK_UPGRADE;
			}
			else
			{
				actionResult = TINKER_ACTION_OK;
			}
		}
		else
		{
			actionResult = TINKER_ACTION_INVALID_ITEM; // catch-all
		}
	}

	if ( !checkStatusOnly )
	{
		itemActionType = actionResult;
		if ( tinkerFrame )
		{
			if ( auto baseFrame = tinkerFrame->findFrame("tinker base") )
			{
				if ( auto itemTooltipFrame = baseFrame->findFrame("tinker display tooltip") )
				{
					auto itemSlotFrame = itemTooltipFrame->findFrame("item slot frame");
					int oldQty = item->count;
					if ( parentGUI.isNodeTinkeringCraftableItem(item->node) && parentGUI.tinkeringFilter == TINKER_FILTER_CRAFTABLE )
					{
						//item->count = 1;
					}
					updateSlotFrameFromItem(itemSlotFrame, item);
					item->count = oldQty;
				}
			}
		}
	}
	return actionResult;
}

bool GenericGUIMenu::TinkerGUI_t::warpMouseToSelectedTinkerItem(Item* snapToItem, Uint32 flags)
{
	if ( tinkerGUIHasBeenCreated() )
	{
		int x = getSelectedTinkerSlotX();
		int y = getSelectedTinkerSlotY();
		if ( snapToItem )
		{
			x = snapToItem->x;
			y = snapToItem->y;
		}

		if ( auto slot = getTinkerSlotFrame(x, y) )
		{
			int playernum = parentGUI.getPlayer();
			auto player = players[playernum];
			if ( !isInteractable )
			{
				//messagePlayer(0, "[Debug]: select item queued");
				player->inventoryUI.cursor.queuedModule = Player::GUI_t::MODULE_TINKERING;
				player->inventoryUI.cursor.queuedFrameToWarpTo = slot;
				return false;
			}
			else
			{
				//messagePlayer(0, "[Debug]: select item warped");
				player->inventoryUI.cursor.queuedModule = Player::GUI_t::MODULE_NONE;
				player->inventoryUI.cursor.queuedFrameToWarpTo = nullptr;
				slot->warpMouseToFrame(playernum, flags);
			}
			return true;
		}
	}
	return false;
}

void GenericGUIMenu::TinkerGUI_t::clearItemDisplayed()
{
	metalScrapPrice = -1;
	magicScrapPrice = -1;
	itemActionType = TINKER_ACTION_NONE;
	itemType = -1;
	itemRequirement = -1;
}

const int GenericGUIMenu::AlchemyGUI_t::MAX_ALCH_X = 4;
const int GenericGUIMenu::AlchemyGUI_t::MAX_ALCH_Y = 6;

void GenericGUIMenu::AlchemyGUI_t::openAlchemyMenu()
{
	const int playernum = parentGUI.getPlayer();
	auto player = players[playernum];

	if ( alchFrame )
	{
		bool wasDisabled = alchFrame->isDisabled();
		alchFrame->setDisabled(false);
		if ( wasDisabled )
		{
			notifications.clear();
			animx = 0.0;
			animTooltip = 0.0;
			isInteractable = false;
			bFirstTimeSnapCursor = false;
		}
		selectAlchemySlot(ALCH_SLOT_BASE_POTION_X, 0);
		player->hud.compactLayoutMode = Player::HUD_t::COMPACT_LAYOUT_INVENTORY;
		player->inventory_mode = INVENTORY_MODE_ITEM;
		bOpen = true;
		currentView = ALCHEMY_VIEW_BREW;
	}
	if ( inputs.getUIInteraction(playernum)->selectedItem )
	{
		inputs.getUIInteraction(playernum)->selectedItem = nullptr;
		inputs.getUIInteraction(playernum)->toggleclick = false;
	}
	inputs.getUIInteraction(playernum)->selectedItemFromChest = 0;
	clearItemDisplayed();
}

void GenericGUIMenu::AlchemyGUI_t::closeAlchemyMenu()
{
	const int playernum = parentGUI.getPlayer();
	auto& player = *players[playernum];

	if ( alchFrame )
	{
		alchFrame->setDisabled(true);
	}
	animx = 0.0;
	animTooltip = 0.0;

	animPotion1 = 0.0;
	potion1Uid = 0;
	animPotion2 = 0.0;
	potion2Uid = 0;
	alchemyResultPotion.type = POTION_EMPTY;
	potionResultUid = 0;
	animPotionResult = 0.0;
	animRecipeAutoAddToSlot1Uid = 0;
	animRecipeAutoAddToSlot2Uid = 0;

	isInteractable = false;
	bool wasOpen = bOpen;
	bOpen = false;
	bFirstTimeSnapCursor = false;
	if ( wasOpen )
	{
		if ( inputs.getUIInteraction(playernum)->selectedItem )
		{
			inputs.getUIInteraction(playernum)->selectedItem = nullptr;
			inputs.getUIInteraction(playernum)->toggleclick = false;
		}
		inputs.getUIInteraction(playernum)->selectedItemFromChest = 0;
	}
	if ( players[playernum]->GUI.activeModule == Player::GUI_t::MODULE_ALCHEMY
		&& !players[playernum]->shootmode )
	{
		// reset to inventory mode if still hanging in alchemy GUI
		players[playernum]->hud.compactLayoutMode = Player::HUD_t::COMPACT_LAYOUT_INVENTORY;
		players[playernum]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
		if ( !inputs.getVirtualMouse(playernum)->draw_cursor )
		{
			players[playernum]->GUI.warpControllerToModule(false);
		}
	}
	clearItemDisplayed();
	itemRequiresTitleReflow = true;
	if ( alchFrame )
	{
		for ( auto f : alchFrame->getFrames() )
		{
			f->removeSelf();
		}
		alchemySlotFrames.clear();
	}
	recipes.closeRecipePanel();
	recipesFrame = nullptr;
	notifications.clear();
	recipes.stones.clear();
}

int GenericGUIMenu::AlchemyGUI_t::heightOffsetWhenNotCompact = 150;
const int alchemyBaseWidth = 206;

bool GenericGUIMenu::AlchemyGUI_t::alchemyGUIHasBeenCreated() const
{
	if ( alchFrame )
	{
		if ( !alchFrame->getFrames().empty() )
		{
			for ( auto f : alchFrame->getFrames() )
			{
				if ( !f->isToBeDeleted() )
				{
					return true;
				}
			}
			return false;
		}
		else
		{
			return false;
		}
	}
	return false;
}

bool hideRecipeFromList(int type)
{
	if ( type == TOOL_BOMB || type == POTION_SICKNESS )
	{
		return true;
	}
	return false;
}

void buttonAlchemyUpdateSelectorOnHighlight(const int player, Button* button)
{
	if ( button->isHighlighted() )
	{
		players[player]->GUI.setHoveringOverModuleButton(Player::GUI_t::MODULE_ALCHEMY);
		if ( players[player]->GUI.activeModule != Player::GUI_t::MODULE_ALCHEMY )
		{
			players[player]->GUI.activateModule(Player::GUI_t::MODULE_ALCHEMY);
		}
		SDL_Rect pos = button->getAbsoluteSize();
		// make sure to adjust absolute size to camera viewport
		pos.x -= players[player]->camera_virtualx1();
		pos.y -= players[player]->camera_virtualy1();
		players[player]->hud.setCursorDisabled(false);
		players[player]->hud.updateCursorAnimation(pos.x - 1, pos.y - 1, pos.w, pos.h, inputs.getVirtualMouse(player)->draw_cursor);
	}
}

bool playerKnowsRecipe(const int player, ItemType basePotion, ItemType secondaryPotion, ItemType result)
{
	for ( auto& entry : clientLearnedAlchemyRecipes[player] )
	{
		if ( entry.first == result
			&& ((entry.second.first == basePotion && entry.second.second == secondaryPotion)
				|| (entry.second.first == secondaryPotion && entry.second.second == basePotion)) )
		{
			return true;
		}
	}
	return false;
}

void getInventoryItemAlchemyAnimSlotPos(Frame* slotFrame, Player* player, int itemx, int itemy, int& outPosX, int& outPosY, int yOffset)
{
	outPosX = slotFrame->getSize().x + slotFrame->getParent()->getSize().x;
	outPosY = slotFrame->getSize().y + (player->inventoryUI.bCompactView ? 8 : 0) + yOffset;
	if ( itemy >= player->inventoryUI.DEFAULT_INVENTORY_SIZEY )
	{
		// backpack slots, add another offset.
		if ( auto invSlotsFrame = player->inventoryUI.frame->findFrame("inventory slots") )
		{
			outPosY += invSlotsFrame->getSize().h;
		}
	}
}

void buildRecipeList(const int player);

void GenericGUIMenu::AlchemyGUI_t::updateAlchemyMenu()
{
	const int playernum = parentGUI.getPlayer();
	auto player = players[playernum];

	if ( !player->isLocalPlayer() )
	{
		closeAlchemyMenu();
		return;
	}

	if ( !alchFrame )
	{
		return;
	}

	alchFrame->setSize(SDL_Rect{ players[playernum]->camera_virtualx1(),
		players[playernum]->camera_virtualy1(),
		alchemyBaseWidth,
		players[playernum]->camera_virtualHeight() });

	if ( !alchFrame->isDisabled() && bOpen )
	{
		if ( !alchemyGUIHasBeenCreated() )
		{
			createAlchemyMenu();
		}

		const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
		real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animx)) / 2.0;
		animx += setpointDiffX;
		animx = std::min(1.0, animx);
		if ( animx >= .9999 )
		{
			if ( !bFirstTimeSnapCursor )
			{
				bFirstTimeSnapCursor = true;
				if ( !inputs.getUIInteraction(playernum)->selectedItem
					&& player->GUI.activeModule == Player::GUI_t::MODULE_ALCHEMY )
				{
					//warpMouseToSelectedAlchemyItem(nullptr, (Inputs::SET_CONTROLLER));
				}
			}
			isInteractable = true;
		}
	}
	else
	{
		animx = 0.0;
		animTooltip = 0.0;
		isInteractable = false;
	}

	auto alchFramePos = alchFrame->getSize();
	if ( player->inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_LEFT )
	{
		if ( !player->inventoryUI.bCompactView )
		{
			const int fullWidth = alchFramePos.w + 210; // inventory width 210
			alchFramePos.x = -alchFramePos.w + animx * fullWidth;
			if ( player->bUseCompactGUIWidth() )
			{
				if ( player->inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				alchFramePos.x -= player->inventoryUI.slideOutWidth * player->inventoryUI.slideOutPercent;
			}
		}
		else
		{
			alchFramePos.x = player->camera_virtualWidth() - animx * alchFramePos.w;
			if ( player->bUseCompactGUIWidth() )
			{
				if ( player->inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				alchFramePos.x -= -player->inventoryUI.slideOutWidth * player->inventoryUI.slideOutPercent;
			}
		}
	}
	else if ( player->inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_RIGHT )
	{
		if ( !player->inventoryUI.bCompactView )
		{
			const int fullWidth = alchFramePos.w + 210; // inventory width 210
			alchFramePos.x = player->camera_virtualWidth() - animx * fullWidth * 2;
			if ( player->bUseCompactGUIWidth() )
			{
				if ( player->inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				alchFramePos.x -= -player->inventoryUI.slideOutWidth * player->inventoryUI.slideOutPercent;
			}
		}
		else
		{
			alchFramePos.x = -alchFramePos.w + animx * alchFramePos.w;
			if ( player->bUseCompactGUIWidth() )
			{
				if ( player->inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				alchFramePos.x -= player->inventoryUI.slideOutWidth * player->inventoryUI.slideOutPercent;
				alchFramePos.w = player->camera_virtualWidth();
			}
		}
	}

	int potionAnimOffsetY = 0; // all animations tested at heightOffsetWhenNotCompact = 200, so needs offset
	if ( !player->bUseCompactGUIHeight() )
	{
		alchFramePos.y = heightOffsetWhenNotCompact;
		potionAnimOffsetY = 200 - heightOffsetWhenNotCompact;
	}
	else
	{
		alchFramePos.y = 0;
	}

	if ( !alchemyGUIHasBeenCreated() )
	{
		return;
	}

	auto baseFrame = alchFrame->findFrame("alchemy base");
	baseFrame->setDisabled(false);

	alchFrame->setSize(alchFramePos);

	SDL_Rect baseFramePos = baseFrame->getSize();
	baseFramePos.x = 0;
	baseFramePos.w = alchemyBaseWidth;
	baseFrame->setSize(baseFramePos);

	alchFramePos.h = baseFramePos.y + baseFramePos.h;
	if ( animx >= .9999 )
	{
		baseFramePos.x += alchFramePos.x;
		alchFramePos.w += alchFramePos.x;
		alchFramePos.w = std::min(player->camera_virtualWidth(), alchFramePos.w);
		alchFramePos.x = 0;
		baseFrame->setSize(baseFramePos);
	}
	alchFrame->setSize(alchFramePos);
	/*if ( !alchFrame->findImage("tmp") )
	{
		alchFrame->addImage(SDL_Rect{ 0, 0, alchFramePos.w, alchFramePos.h }, 0xFFFFFFFF,
			"images/system/white.png", "tmp");
	}
	else
	{
		alchFrame->findImage("tmp")->pos = SDL_Rect{ 0, 0, alchFramePos.w, alchFramePos.h };
	}*/

	if ( keystatus[SDL_SCANCODE_J] && enableDebugKeys )
	{
		if ( keystatus[SDL_SCANCODE_LSHIFT] )
		{
			std::vector<ItemType> potions;
			for ( int i = 0; i < NUMITEMS; ++i )
			{
				if ( items[i].category == POTION && i != POTION_EMPTY )
				{
					potions.push_back((ItemType)i);
				}
			}
			for ( auto& potion1 : potions )
			{
				for ( auto& potion2 : potions )
				{
					bool tryDuplicatePotion = false;
					bool randomResult = false;
					bool explodeSelf = false;
					bool samePotion = false;
					ItemType res = alchemyMixResult(potion1, potion2, randomResult, tryDuplicatePotion, samePotion, explodeSelf);
					if ( explodeSelf )
					{
						alchemyAddRecipe(playernum, potion1, potion2, TOOL_BOMB, true);
					}
					else if ( !tryDuplicatePotion && !randomResult )
					{
						alchemyAddRecipe(playernum, potion1, potion2, res);
					}
				}
			}
		}
		else
		{
			if ( recipes.bOpen )
			{
				recipes.closeRecipePanel();
			}
			else
			{
				recipes.openRecipePanel();
			}
		}
		keystatus[SDL_SCANCODE_J] = 0;
	}
	if ( keystatus[SDL_SCANCODE_H] && enableDebugKeys )
	{
		keystatus[SDL_SCANCODE_H] = 0;
		if ( keystatus[SDL_SCANCODE_LSHIFT] )
		{
			for ( int i = 0; i < NUMITEMS; ++i )
			{
				if ( items[i].category == POTION && i != POTION_EMPTY )
				{
					clientLearnedAlchemyIngredients[playernum].insert(i);
				}
			}
		}
		else if ( keystatus[SDL_SCANCODE_LCTRL] )
		{
			consoleCommand("/gimmepotions2");
		}
		else
		{
			consoleCommand("/gimmepotions 5");
		}
	}

	if ( bOpen )
	{
		for ( int x = 0; x < MAX_ALCH_X; ++x )
		{
			for ( int y = 0; y < MAX_ALCH_Y; ++y )
			{
				if ( auto slotFrame = getAlchemySlotFrame(x, y) )
				{
					slotFrame->setDisabled(true);
				}
			}
		}
	}

	recipes.updateRecipePanel();
	if ( animx >= 0.999 )
	{
		SDL_Rect recipePos = recipesFrame->getSize();
		if ( !player->inventoryUI.bCompactView )
		{
			if ( player->inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_LEFT )
			{
				alchFramePos.w += recipePos.w;
				alchFrame->setSize(alchFramePos);
			}
		}
	}
	if ( auto emptyBottleFrame = baseFrame->findFrame("empty bottles") )
	{
		emptyBottleFrame->setUserData(&GAMEUI_FRAMEDATA_ALCHEMY_RECIPE_ENTRY);
		updateSlotFrameFromItem(emptyBottleFrame, &emptyBottleCount);
	}

	animPotion1DestX = baseFrame->getSize().x + 36;
	animPotion1DestY = baseFrame->getSize().y + 128;
	animPotion2DestX = baseFrame->getSize().x + 128;
	animPotion2DestY = baseFrame->getSize().y + 128;
	animPotionResultStartX = baseFrame->getSize().x + 74;
	animPotionResultStartY = baseFrame->getSize().y + 286;

	Frame* recipePreview1Frame = getAlchemySlotFrame(ALCH_SLOT_RECIPE_PREVIEW_POTION1_X, 0);
	{
		auto recipePreview1Pos = recipePreview1Frame->getSize();
		recipePreview1Pos.x = animPotion1DestX;
		recipePreview1Pos.y = animPotion1DestY;
		recipePreview1Frame->setSize(recipePreview1Pos);
		recipePreview1Frame->setDisabled(true);
	}
	Frame* recipePreview2Frame = getAlchemySlotFrame(ALCH_SLOT_RECIPE_PREVIEW_POTION2_X, 0);
	{
		auto recipePreview2Pos = recipePreview2Frame->getSize();
		recipePreview2Pos.x = animPotion2DestX;
		recipePreview2Pos.y = animPotion2DestY;
		recipePreview2Frame->setSize(recipePreview2Pos);
		recipePreview2Frame->setDisabled(true);
	}

	{
		const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
		real_t setpointDiffX = fpsScale * std::max(.05, (animPotionResult)) / 3.0;
		animPotionResult -= setpointDiffX;
		animPotionResult = std::max(0.0, animPotionResult);
	}

	auto animPotion1Frame = getAlchemySlotFrame(ALCH_SLOT_BASE_POTION_X, 0);
	animPotion1Frame->setDisabled(true);
	Item* potion1Item = nullptr;
	if ( animRecipeAutoAddToSlot1Uid != 0 && animPotionResult < 0.001 )
	{
		if ( Item* item = uidToItem(animRecipeAutoAddToSlot1Uid) )
		{
			animPotion1 = 1.0;
			getInventoryItemAlchemyAnimSlotPos(player->inventoryUI.getInventorySlotFrame(item->x, item->y), player, item->x, item->y, animPotion1StartX, animPotion1StartY, potionAnimOffsetY);
			potion1Uid = animRecipeAutoAddToSlot1Uid;
		}
		animRecipeAutoAddToSlot1Uid = 0;
	}
	{
		const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
		real_t setpointDiffX = fpsScale * std::max(.05, (animPotion1)) / 3.0;
		animPotion1 -= setpointDiffX;
		animPotion1 = std::max(0.0, animPotion1);
	}
	if ( potion1Uid != 0 )
	{
		if ( potion1Item = uidToItem(potion1Uid) )
		{
			if ( !potion1Item->identified || itemIsEquipped(potion1Item, playernum) )
			{
				potion1Item = nullptr; // if this got unidentified somehow, remove it
				potion1Uid = 0;
				animPotion1 = 0.0;
				alchemyResultPotion.type = POTION_EMPTY;
				potionResultUid = 0;
				animPotionResult = 0.0;
				animRecipeAutoAddToSlot1Uid = 0;
				animRecipeAutoAddToSlot2Uid = 0;
			}
			else
			{
				animPotion1Frame->setDisabled(false);
				if ( animPotion1 < 0.001 )
				{
					animPotion1Frame->setUserData(nullptr);
					if ( animRecipeAutoAddToSlot2Uid != 0 && animPotionResult < 0.001 )
					{
						if ( Item* item = uidToItem(animRecipeAutoAddToSlot2Uid) )
						{
							animPotion2 = 1.0;
							getInventoryItemAlchemyAnimSlotPos(player->inventoryUI.getInventorySlotFrame(item->x, item->y), player, item->x, item->y, animPotion2StartX, animPotion2StartY, potionAnimOffsetY);
							potion2Uid = animRecipeAutoAddToSlot2Uid;
						}
						animRecipeAutoAddToSlot1Uid = 0;
						animRecipeAutoAddToSlot2Uid = 0;
					}
				}
				else
				{
					animPotion1Frame->setUserData(&GAMEUI_FRAMEDATA_ANIMATING_ITEM);
				}
				if ( potionResultUid == potion1Uid && animPotionResult > 0.001 ) // we're animating to this potion
				{
					animPotion1Frame->setUserData(&GAMEUI_FRAMEDATA_ALCHEMY_ITEM);
					int oldCount = potion1Item->count;
					potion1Item->count = std::max(0, potion1Item->count - animPotionResultCount);
					updateSlotFrameFromItem(animPotion1Frame, potion1Item);
					potion1Item->count = oldCount;
				}
				else
				{
					updateSlotFrameFromItem(animPotion1Frame, potion1Item);
				}
			}
		}
	}
	auto animPotion1Pos = animPotion1Frame->getSize();
	animPotion1Pos.x = animPotion1StartX + (1.0 - animPotion1) * (animPotion1DestX - animPotion1StartX);
	animPotion1Pos.y = animPotion1StartY + (1.0 - animPotion1) * (animPotion1DestY - animPotion1StartY);
	animPotion1Frame->setSize(animPotion1Pos);

	auto animPotion2Frame = getAlchemySlotFrame(ALCH_SLOT_SECONDARY_POTION_X, 0);
	{
		const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
		real_t setpointDiffX = fpsScale * std::max(.05, (animPotion2)) / 3.0;
		animPotion2 -= setpointDiffX;
		animPotion2 = std::max(0.0, animPotion2);
	}
	animPotion2Frame->setDisabled(true);
	Item* potion2Item = nullptr;
	if ( potion2Uid != 0 )
	{
		if ( potion2Item = uidToItem(potion2Uid) )
		{
			if ( !potion2Item->identified || itemIsEquipped(potion2Item, playernum) )
			{
				potion2Item = nullptr; // if this got unidentified somehow, remove it
				potion2Uid = 0;
				animPotion2 = 0.0;
				alchemyResultPotion.type = POTION_EMPTY;
				potionResultUid = 0;
				animPotionResult = 0.0;
				animRecipeAutoAddToSlot1Uid = 0;
				animRecipeAutoAddToSlot2Uid = 0;
			}
			else
			{
				animPotion2Frame->setDisabled(false);
				if ( animPotion2 < 0.001 )
				{
					animPotion2Frame->setUserData(nullptr);
				}
				else
				{
					animPotion2Frame->setUserData(&GAMEUI_FRAMEDATA_ANIMATING_ITEM);
				}
				if ( potionResultUid == potion2Uid && animPotionResult > 0.001 ) // we're animating to this potion
				{
					animPotion2Frame->setUserData(&GAMEUI_FRAMEDATA_ALCHEMY_ITEM);
					int oldCount = potion2Item->count;
					potion2Item->count = std::max(0, potion2Item->count - animPotionResultCount);
					updateSlotFrameFromItem(animPotion2Frame, potion2Item);
					potion2Item->count = oldCount;
				}
				else
				{
					updateSlotFrameFromItem(animPotion2Frame, potion2Item);
				}
			}
		}
	}
	auto animPotion2Pos = animPotion2Frame->getSize();
	animPotion2Pos.x = animPotion2StartX + (1.0 - animPotion2) * (animPotion2DestX - animPotion2StartX);
	animPotion2Pos.y = animPotion2StartY + (1.0 - animPotion2) * (animPotion2DestY - animPotion2StartY);
	animPotion2Frame->setSize(animPotion2Pos);

	auto potionResultFrame = getAlchemySlotFrame(ALCH_SLOT_RESULT_POTION_X, 0);
	potionResultFrame->setDisabled(true);

	{
		if ( animPotionResult < 0.001 )
		{
			if ( auto item = uidToItem(potionResultUid) )
			{
				if ( auto slotFrame = player->inventoryUI.getInventorySlotFrame(item->x, item->y) )
				{
					// one-off update to prevent a blank frame
					if ( potionResultUid != potion1Uid && potionResultUid != potion2Uid )
					{
						slotFrame->setUserData(nullptr);
						updateSlotFrameFromItem(slotFrame, item);
					}
				}
			}
			potionResultUid = 0;
		}
	}
	auto animPotionResultPos = potionResultFrame->getSize();
	if ( potionResultUid == 0 )
	{
		animPotionResultPos.x = animPotionResultStartX;
		animPotionResultPos.y = animPotionResultStartY;
	}
	else
	{
		animPotionResultPos.x = animPotionResultStartX + (1.0 - animPotionResult) * (animPotionResultDestX - animPotionResultStartX);
		animPotionResultPos.y = animPotionResultStartY + (1.0 - animPotionResult) * (animPotionResultDestY - animPotionResultStartY);
	}
	potionResultFrame->setSize(animPotionResultPos);
	potionResultFrame->setOpacity(100.0);
	if ( potionResultUid != 0 )
	{
		if ( auto item = uidToItem(potionResultUid) )
		{
			potionResultFrame->setDisabled(false);
			int oldCount = item->count;
			item->count = 1;
			potionResultFrame->setUserData(&GAMEUI_FRAMEDATA_ANIMATING_ITEM);
			updateSlotFrameFromItem(potionResultFrame, item);
			if ( animPotionResult < .25 && itemIsEquipped(item, playernum) )
			{
				potionResultFrame->setOpacity(100.0 * (animPotionResult / .25));
			}
			item->count = oldCount;
		}
	}
	else if ( alchemyResultPotion.type != POTION_EMPTY && potion1Uid != 0 && potion2Uid != 0 )
	{
		potionResultFrame->setDisabled(false);
		potionResultFrame->setUserData(nullptr);
		updateSlotFrameFromItem(potionResultFrame, &alchemyResultPotion);
		if ( alchemyResultPotion.type == TOOL_BOMB )
		{
			auto spriteImageFrame = potionResultFrame->findFrame("item sprite frame");
			auto spriteImage = spriteImageFrame->findImage("item sprite img");
			spriteImage->path = "*#images/ui/Alchemy/fireball.png";
		}
	}

	if ( !bOpen )
	{
		return;
	}

	if ( !parentGUI.isGUIOpen()
		|| parentGUI.guiType != GUICurrentType::GUI_TYPE_ALCHEMY
		|| !stats[playernum]
		|| stats[playernum]->HP <= 0
		|| !player->entity
		|| player->shootmode )
	{
		closeAlchemyMenu();
		return;
	}

	if ( player->entity && player->entity->isBlind() )
	{
		messagePlayer(playernum, MESSAGE_MISC, language[4159]);
		parentGUI.closeGUI();
		return; // I can't see!
	}

	if ( keystatus[SDL_SCANCODE_B] && enableDebugKeys )
	{
		keystatus[SDL_SCANCODE_B] = 0;
		notifications.push_back(std::make_pair(ticks, AlchNotification_t("Wow a title!", "This is a body", "items/images/Alembic.png")));
	}
	auto notificationFrame = alchFrame->findFrame("notification");
	notificationFrame->setDisabled(true);
	if ( !notifications.empty() )
	{
		auto& n = notifications.front();
		SDL_Rect notifPos = notificationFrame->getSize();
		if ( (player->inventoryUI.inventoryPanelJustify == Player::PanelJustify_t::PANEL_JUSTIFY_LEFT
			&& !player->inventoryUI.bCompactView)
			|| (player->inventoryUI.inventoryPanelJustify == Player::PanelJustify_t::PANEL_JUSTIFY_RIGHT
				&& player->inventoryUI.bCompactView) )
		{
			notifPos.x = baseFrame->getSize().x + baseFrame->getSize().w - notifPos.w * (1.0 - n.second.animx);
		}
		else
		{
			notifPos.x = baseFrame->getSize().x - notifPos.w * (n.second.animx);
		}
		notifPos.y = 4;
		notificationFrame->setSize(notifPos);
		notificationFrame->setOpacity(100.0 * n.second.animx);
		notificationFrame->setDisabled(false);

		auto notifTitle = notificationFrame->findField("notif title");
		notifTitle->setText(n.second.title.c_str());
		auto notifBody = notificationFrame->findField("notif body");
		notifBody->setText(n.second.body.c_str());
		auto notifIcon = notificationFrame->findImage("notif icon");
		notifIcon->path = n.second.img;

		if ( n.second.state == 0 )
		{
			const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - n.second.animx)) / 2.0;
			n.second.animx += setpointDiffX;
			n.second.animx = std::min(1.0, n.second.animx);
			if ( n.second.animx >= 0.999 )
			{
				n.second.state = 1;
				n.first = ticks;
			}
		}
		else if ( n.second.state == 1 )
		{
			if ( ticks - n.first > TICKS_PER_SECOND * 3 )
			{
				n.second.state = 2;
			}
		}
		else if ( n.second.state == 2 )
		{
			const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.05, (n.second.animx)) / 3.0;
			n.second.animx -= setpointDiffX;
			n.second.animx = std::max(0.0, n.second.animx);
			if ( n.second.animx <= 0.001 )
			{
				n.second.state = 3;
				notifications.erase(notifications.begin());
			}
		}
	}

	// alembic status
	{
		auto alembicTitle = baseFrame->findField("alchemy alembic title");
		auto alembicStatus = baseFrame->findField("alchemy alembic status");
		if ( auto item = parentGUI.alembicItem )
		{
			char buf[128];
			if ( !item->identified )
			{
				snprintf(buf, sizeof(buf), "%s (?)", item->getName());
			}
			else
			{
				snprintf(buf, sizeof(buf), "%s (%+d)", item->getName(), item->beatitude);
			}
			std::string titleStr = buf;
			if ( !titleStr.empty() )
			{
				if ( titleStr[0] >= 'a' && titleStr[0] <= 'z' )
				{
					titleStr[0] = (char)toupper((int)titleStr[0]);
				}
				size_t found = titleStr.find(' ');
				while ( found != std::string::npos )
				{
					auto& c = titleStr[std::min(found + 1, titleStr.size() - 1)];
					if ( c >= 'a' && c <= 'z' )
					{
						c = (char)toupper((int)c);
					}
					found = titleStr.find(' ', found + 1);
				}
				alembicTitle->setText(titleStr.c_str());
			}
			else
			{
				alembicTitle->setText(buf);
			}
			alembicStatus->setText(ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str());
			if ( item->status <= DECREPIT )
			{
				alembicStatus->setTextColor(hudColors.characterSheetRed);
			}
			else
			{
				alembicStatus->setTextColor(hudColors.characterSheetLightNeutral);
			}
		}
		else
		{
			alembicTitle->setText("");
			alembicStatus->setText("");
		}

		SDL_Rect textPos{ 0, 21, baseFrame->getSize().w, 24 };
		alembicTitle->setSize(textPos);
		textPos.y += 18;
		alembicStatus->setSize(textPos);
	}

	bool usingGamepad = inputs.hasController(playernum) && !inputs.getVirtualMouse(playernum)->draw_cursor;
	auto recipeBtn = baseFrame->findButton("recipe button");
	auto recipeGlyph = baseFrame->findImage("recipe glyph");
	{
		// close btn
		auto closeBtn = baseFrame->findButton("close alchemy button");
		auto closeGlyph = baseFrame->findImage("close alchemy glyph");
		closeBtn->setDisabled(true);
		closeGlyph->disabled = true;
		if ( inputs.getVirtualMouse(playernum)->draw_cursor )
		{
			closeBtn->setDisabled(!isInteractable);
			if ( isInteractable )
			{
				buttonAlchemyUpdateSelectorOnHighlight(playernum, closeBtn);
			}
		}
		else if ( closeBtn->isSelected() )
		{
			closeBtn->deselect();
		}
		if ( closeBtn->isDisabled() && usingGamepad && potion1Uid == 0 && potion2Uid == 0 )
		{
			closeGlyph->path = Input::inputs[playernum].getGlyphPathForBinding("MenuCancel");
			if ( auto imgGet = Image::get(closeGlyph->path.c_str()) )
			{
				closeGlyph->pos.w = imgGet->getWidth();
				closeGlyph->pos.h = imgGet->getHeight();
				closeGlyph->disabled = false;
			}
			closeGlyph->pos.x = closeBtn->getSize().x + closeBtn->getSize().w / 2 - closeGlyph->pos.w / 2;
			if ( closeGlyph->pos.x % 2 == 1 )
			{
				++closeGlyph->pos.x;
			}
			closeGlyph->pos.y = closeBtn->getSize().y + closeBtn->getSize().h - 4;
		}

		recipeBtn->setDisabled(true);
		recipeGlyph->disabled = true;
		if ( recipes.bOpen )
		{
			recipeBtn->setText(language[4171]);
		}
		else
		{
			recipeBtn->setText(language[4170]);
		}
		if ( inputs.getVirtualMouse(playernum)->draw_cursor )
		{
			recipeBtn->setDisabled(!isInteractable);
			if ( isInteractable )
			{
				buttonAlchemyUpdateSelectorOnHighlight(playernum, recipeBtn);
			}
		}
		else if ( recipeBtn->isSelected() )
		{
			recipeBtn->deselect();
		}
		if ( recipeBtn->isDisabled() && usingGamepad )
		{
			recipeGlyph->path = Input::inputs[playernum].getGlyphPathForBinding("MenuPageRightAlt");
			if ( auto imgGet = Image::get(recipeGlyph->path.c_str()) )
			{
				recipeGlyph->pos.w = imgGet->getWidth();
				recipeGlyph->pos.h = imgGet->getHeight();
				recipeGlyph->disabled = false;
			}
			recipeGlyph->pos.x = recipeBtn->getSize().x + recipeBtn->getSize().w / 2 - recipeGlyph->pos.w / 2;
			if ( recipeGlyph->pos.x % 2 == 1 )
			{
				++recipeGlyph->pos.x;
			}
			recipeGlyph->pos.y = recipeBtn->getSize().y + recipeBtn->getSize().h - 8;
		}
	}

	int skillLVL = (stats[playernum]->PROFICIENCIES[PRO_ALCHEMY] + statGetINT(stats[playernum], players[playernum]->entity));
	Uint32 negativeColor = hudColors.characterSheetRed;
	Uint32 neutralColor = hudColors.characterSheetLightNeutral;
	Uint32 positiveColor = hudColors.characterSheetGreen;
	Uint32 secondaryPositiveColor = hudColors.characterSheetHighlightText;
	Uint32 defaultPromptColor = makeColor(255, 255, 255, 255);

	auto itemDisplayTooltip = baseFrame->findFrame("alchemy display tooltip");
	itemDisplayTooltip->setDisabled(false);
	auto displayItemName = itemDisplayTooltip->findField("item display name");
	auto displayItemTextImg = itemDisplayTooltip->findImage("item text img");
	const int displayItemTextImgBaseX = 0;
	displayItemTextImg->pos.x = displayItemTextImgBaseX;
	displayItemTextImg->pos.y = 0;
	SDL_Rect displayItemNamePos{ displayItemTextImg->pos.x + 8, displayItemTextImg->pos.y - 4, 170, displayItemName->getSize().h };
	displayItemName->setSize(displayItemNamePos);

	SDL_Rect tooltipPos = itemDisplayTooltip->getSize();
	tooltipPos.w = 186;
	tooltipPos.h = baseFrame->getSize().h - 100;
	tooltipPos.y = 186;
	tooltipPos.x = 10;// 18 - (tooltipPos.w + 18) * (1.0 - animTooltip);
	itemDisplayTooltip->setSize(tooltipPos);
	//auto itemIncrementText = baseFrame->findField("item increment txt");
	{
		/*if ( animScrap > 0.01 )
		{
			itemIncrementText->setDisabled(false);
			auto pos = itemIncrementText->getSize();
			pos.y = tooltipPos.y + itemSlotBg->pos.y + 16 - ((1.0 - animScrap) * 32);
			itemIncrementText->setSize(pos);
			SDL_Color color;
			getColor(itemIncrementText->getColor(), &color.r, &color.g, &color.b, &color.a);
			if ( animScrap < .2 )
			{
				itemIncrementText->setColor(makeColor(color.r, color.g, color.b, 255 * (animScrap / .2)));
			}
			else
			{
				itemIncrementText->setColor(makeColor(color.r, color.g, color.b, 255));
			}
		}
		else
		{
			itemIncrementText->setSize(SDL_Rect{ tooltipPos.x + itemSlotBg->pos.x + 24, tooltipPos.y + itemSlotBg->pos.y + 16, 72, 24 });
			itemIncrementText->setDisabled(true);
			itemIncrementText->setText("");
		}*/
	}

	// calculate resultant potion
	{
		if ( potion1Item && potion2Item )
		{
			bool tryDuplicatePotion = false;
			bool randomResult = false;
			bool explodeSelf = false;
			bool samePotion = false;
			ItemType res = alchemyMixResult(potion1Item->type, potion2Item->type, randomResult, tryDuplicatePotion, samePotion, explodeSelf);
			Status status = EXCELLENT;
			alchemyResultPotion.identified = false;
			alchemyResultPotion.type = POTION_EMPTY;
			if ( samePotion || tryDuplicatePotion )
			{
				alchemyResultPotion.count = 2;
			}
			else
			{
				alchemyResultPotion.count = 1;
			}
			int appearance = -1;
			int blessing = 0;
			if ( potion1Item->beatitude > 0 && potion2Item->beatitude > 0 )
			{
				blessing = std::min(potion1Item->beatitude, potion2Item->beatitude); // take least blessed
			}
			else if ( potion1Item->beatitude < 0 && potion2Item->beatitude < 0 )
			{
				blessing = std::min(potion1Item->beatitude, potion2Item->beatitude); // take most cursed
			}
			else if ( (potion1Item->beatitude < 0 && potion2Item->beatitude > 0)
				|| (potion2Item->beatitude < 0 && potion1Item->beatitude > 0) )
			{
				blessing = 0;
			}
			else if ( potion1Item->beatitude < 0 && potion2Item->beatitude == 0 )
			{
				blessing = potion1Item->beatitude; // curse the result
			}
			else if ( potion1Item->beatitude == 0 && potion2Item->beatitude < 0 )
			{
				blessing = potion2Item->beatitude; // curse the result
			}
			else if ( potion1Item->beatitude > 0 && potion2Item->beatitude == 0 )
			{
				blessing = 0; // negate the blessing
			}
			else if ( potion1Item->beatitude == 0 && potion2Item->beatitude > 0 )
			{
				blessing = 0; // negate the blessing
			}
			if ( samePotion )
			{
				// same potion, keep the first potion only.
				res = potion1Item->type;
				if ( potion1Item->beatitude == potion2Item->beatitude )
				{
					blessing = potion1Item->beatitude;
				}
				appearance = potion1Item->appearance;
				status = potion1Item->status;
			}

			if ( tryDuplicatePotion && !explodeSelf && !randomResult )
			{
				// duplicate chance
				if ( potion1Item->type == POTION_WATER )
				{
					res = potion2Item->type;
					status = potion2Item->status;
					blessing = potion2Item->beatitude;
					appearance = potion2Item->appearance;
				}
				else if ( potion2Item->type == POTION_WATER )
				{
					res = potion1Item->type;
					status = potion1Item->status;
					blessing = potion1Item->beatitude;
					appearance = potion1Item->appearance;
				}
			}
			else
			{
				tryDuplicatePotion = false;
			}
			alchemyResultPotion.status = status;
			if ( !(potion1Item->identified && potion2Item->identified) || randomResult )
			{
				alchemyResultPotion.identified = false;
			}
			else
			{
				alchemyResultPotion.identified = tryDuplicatePotion || samePotion ||
					playerKnowsRecipe(playernum, potion1Item->type, potion2Item->type, res);
			}
			bool doRandomAppearances = false;
			if ( alchemyResultPotion.identified )
			{
				alchemyResultPotion.type = res;
				if ( res == POTION_SICKNESS )
				{
					doRandomAppearances = true;
				}
				else
				{
					animRandomPotionTicks = 0;
				}
			}
			else
			{
				alchemyResultPotion.type = POTION_BOOZE;
				doRandomAppearances = true;
			}

			if ( doRandomAppearances )
			{
				if ( animRandomPotionTicks == 0 )
				{
					animRandomPotionTicks = ticks;
					animRandomPotionUpdatedThisTick = ticks;
				}
				else if ( (ticks != animRandomPotionTicks) && (ticks - animRandomPotionTicks) % TICKS_PER_SECOND == 0 )
				{
					if ( animRandomPotionUpdatedThisTick != ticks )
					{
						++animRandomPotionVariation;
						animRandomPotionUpdatedThisTick = ticks;
					}
					if ( animRandomPotionVariation >= items[alchemyResultPotion.type].variations )
					{
						animRandomPotionVariation = 0;
					}
				}
			}

			// blessings
			if ( !tryDuplicatePotion && !samePotion )
			{
				if ( parentGUI.alembicItem )
				{
					if ( parentGUI.alembicItem->beatitude >= 1 )
					{
						blessing = 1;
					}
					else if ( parentGUI.alembicItem->beatitude <= -1 )
					{
						blessing = parentGUI.alembicItem->beatitude;
					}
				}
				if ( skillCapstoneUnlocked(playernum, PRO_ALCHEMY) )
				{
					blessing = 2;
					if ( parentGUI.alembicItem )
					{
						if ( parentGUI.alembicItem->beatitude <= -1 )
						{
							blessing = std::min(-2, (int)parentGUI.alembicItem->beatitude);
						}
						else
						{
							blessing = std::max(2, (int)parentGUI.alembicItem->beatitude);
						}
					}
				}
			}

			if ( explodeSelf && alchemyResultPotion.identified )
			{
				alchemyResultPotion.beatitude = 0;
			}
			else
			{
				alchemyResultPotion.beatitude = blessing;
			}

			// appearances
			for ( auto it = potionStandardAppearanceMap.begin(); it != potionStandardAppearanceMap.end(); ++it )
			{
				if ( (*it).first == alchemyResultPotion.type )
				{
					if ( appearance == -1 )
					{
						appearance = (*it).second;
					}
				}
			}
			alchemyResultPotion.appearance = std::max(0, appearance);
			if ( doRandomAppearances )
			{
				alchemyResultPotion.appearance = animRandomPotionVariation;
			}
		}
		else
		{
			animRandomPotionTicks = 0;
		}
	}

	bool modifierPressed = false;
	if ( usingGamepad && Input::inputs[playernum].binary("MenuPageLeftAlt") )
	{
		modifierPressed = true;
	}
	else if ( inputs.bPlayerUsingKeyboardControl(playernum)
		&& (keystatus[SDL_SCANCODE_LSHIFT] || keystatus[SDL_SCANCODE_RSHIFT]) )
	{
		modifierPressed = true;
	}

	auto brewBtn = baseFrame->findButton("brew button");
	auto brewGlyph = baseFrame->findImage("brew glyph");
	brewBtn->setDisabled(true);
	brewGlyph->disabled = true;
	if ( inputs.getVirtualMouse(playernum)->draw_cursor )
	{
		brewBtn->setText(language[4175]);
		if ( (potion1Uid != 0 || potion2Uid != 0) && isInteractable )
		{
			brewBtn->setDisabled(false);
			brewBtn->setTextColor(makeColor(255, 255, 255, 255));
			buttonAlchemyUpdateSelectorOnHighlight(playernum, brewBtn);
		}
		else
		{
			brewBtn->setTextColor(hudColors.characterSheetFaintText);
		}
	}
	else if ( brewBtn->isSelected() )
	{
		brewBtn->deselect();
	}
	if ( usingGamepad && !inputs.getVirtualMouse(playernum)->draw_cursor )
	{
		if ( potionResultFrame->isDisabled() )
		{
			brewBtn->setTextColor(hudColors.characterSheetFaintText);
		}
		else
		{
			brewBtn->setTextColor(makeColor(255, 255, 255, 255));
		}
		brewBtn->setText(language[4178]);
		brewBtn->setDisabled(true);
		if ( !potionResultFrame->isDisabled() )
		{
			brewGlyph->path = Input::inputs[playernum].getGlyphPathForBinding("MenuAlt2");
			if ( auto imgGet = Image::get(brewGlyph->path.c_str()) )
			{
				brewGlyph->pos.w = imgGet->getWidth();
				brewGlyph->pos.h = imgGet->getHeight();
				brewGlyph->disabled = false;
			}
			brewGlyph->pos.x = brewBtn->getSize().x + brewBtn->getSize().w - 16;
			if ( brewGlyph->pos.x % 2 == 1 )
			{
				++brewGlyph->pos.x;
			}
			brewGlyph->pos.y = brewBtn->getSize().y + brewBtn->getSize().h - 16;
		}
	}

	bool inventoryControlActive = player->bControlEnabled
		&& !gamePaused
		&& !player->usingCommand()
		&& !player->GUI.isDropdownActive();

	if ( isInteractable && !inputs.getUIInteraction(playernum)->selectedItem
		&& inventoryControlActive
		&& player->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_ALCHEMY)
		&& player->GUI.bActiveModuleUsesInventory()
		&& player->GUI.activeModule == Player::GUI_t::MODULE_ALCHEMY )
	{
		if ( getSelectedAlchemySlotX() >= 0 && getSelectedAlchemySlotX() < MAX_ALCH_X
			&& getSelectedAlchemySlotY() >= 0 && getSelectedAlchemySlotY() < MAX_ALCH_Y
			&& recipes.bOpen && recipes.isInteractable )
		{
			if ( auto slotFrame = getAlchemySlotFrame(getSelectedAlchemySlotX(), getSelectedAlchemySlotY()) )
			{
				if ( !slotFrame->isDisabled() && slotFrame->capturesMouse() )
				{
					int index = 0;
					for ( auto& entry : recipes.recipeList )
					{
						if ( entry.x == getSelectedAlchemySlotX() && entry.y == getSelectedAlchemySlotY()
							&& !hideRecipeFromList(entry.resultItem.type) )
						{
							setItemDisplayNameAndPrice(&entry.resultItem, true, true);
							if ( recipes.activateRecipeIndex >= 0 )
							{
								break; // don't update slots when recipe active
							}
							if ( auto slot = getAlchemySlotFrame(ALCH_SLOT_RECIPE_PREVIEW_POTION1_X, 0) )
							{
								slot->setUserData(&GAMEUI_FRAMEDATA_ALCHEMY_RECIPE_SLOT);
								updateSlotFrameFromItem(slot, &entry.dummyPotion1);
								if ( !animPotion1Frame->isDisabled() )
								{
									if ( Item* item = uidToItem(potion1Uid) )
									{
										if ( item->type != entry.dummyPotion1.type || recipes.activateRecipeIndex == -1 )
										{
											animPotion1Frame->setDisabled(true);
										}
									}
								}
							}
							if ( auto slot = getAlchemySlotFrame(ALCH_SLOT_RECIPE_PREVIEW_POTION2_X, 0) )
							{
								slot->setUserData(&GAMEUI_FRAMEDATA_ALCHEMY_RECIPE_SLOT);
								updateSlotFrameFromItem(slot, &entry.dummyPotion2);
								if ( !animPotion2Frame->isDisabled() )
								{
									if ( Item* item = uidToItem(potion2Uid) )
									{
										if ( item->type != entry.dummyPotion2.type || recipes.activateRecipeIndex == -1 )
										{
											animPotion2Frame->setDisabled(true);
										}
									}
								}
							}
							if ( auto slot = getAlchemySlotFrame(ALCH_SLOT_RESULT_POTION_X, 0) )
							{
								slot->setUserData(&GAMEUI_FRAMEDATA_ALCHEMY_RECIPE_SLOT);
								int oldCount = entry.resultItem.count;
								entry.resultItem.count = 1;
								updateSlotFrameFromItem(slot, &entry.resultItem);
								entry.resultItem.count = oldCount;
							}
							break;
						}
						++index;
					}
				}
			}
		}
		else if ( getSelectedAlchemySlotX() >= ALCH_SLOT_RESULT_POTION_X && getSelectedAlchemySlotX() < 0
			&& getSelectedAlchemySlotY() == 0 )
		{
			if ( auto slotFrame = getAlchemySlotFrame(getSelectedAlchemySlotX(), getSelectedAlchemySlotY()) )
			{
				if ( !slotFrame->isDisabled() && slotFrame->capturesMouse() )
				{
					switch ( getSelectedAlchemySlotX() )
					{
						case ALCH_SLOT_BASE_POTION_X:
							setItemDisplayNameAndPrice(potion1Item, false, false);
							break;
						case ALCH_SLOT_SECONDARY_POTION_X:
							setItemDisplayNameAndPrice(potion2Item, false, false);
							break;
						case ALCH_SLOT_RESULT_POTION_X:
							setItemDisplayNameAndPrice(&alchemyResultPotion, true, false);
							break;
						default:
							break;
					}
				}
			}
		}
	}

	auto activateSelectionGlyph = alchFrame->findImage("activate glyph");
	auto activateSelectionPrompt = alchFrame->findField("activate prompt");
	if ( !strcmp(activateSelectionPrompt->getText(), "") )
	{
		activateSelectionGlyph->disabled = true;
		activateSelectionPrompt->setDisabled(true);
	}

	if ( itemType != -1 && itemDesc.size() > 1 )
	{
		if ( isInteractable )
		{
			//const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
			//real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animTooltip)) / 2.0;
			//animTooltip += setpointDiffX;
			//animTooltip = std::min(1.0, animTooltip);
			animTooltip = 1.0;
			animTooltipTicks = ticks;
		}

		itemDisplayTooltip->setDisabled(false);

		{
			// item name + text bg
			displayItemName->setVJustify(Field::justify_t::CENTER);
			displayItemName->setHJustify(Field::justify_t::CENTER);
			if ( itemRequiresTitleReflow )
			{
				displayItemName->setText(itemDesc.c_str());
				displayItemName->reflowTextToFit(0);

				if ( displayItemName->getNumTextLines() > 2 || true )
				{
					auto pos = displayItemName->getSize();
					pos.h = 74;
					displayItemName->setSize(pos);
					displayItemTextImg->path = "*#images/ui/Alchemy/Alchemy_LabelName_3Row_00.png";
					displayItemTextImg->pos.h = 64;
				}
				else
				{
					auto pos = displayItemName->getSize();
					pos.h = 50;
					displayItemName->setSize(pos);
					displayItemTextImg->path = "*#images/ui/Alchemy/Alchemy_LabelName_2Row_00.png";
					displayItemTextImg->pos.h = 42;
				}
				if ( displayItemName->getNumTextLines() > 3 )
				{
					// more than 2 lines, append ...
					std::string copiedName = displayItemName->getText();
					auto lastNewline = copiedName.find_last_of('\n');
					copiedName = copiedName.substr(0U, lastNewline);
					copiedName += "...";
					displayItemName->setText(copiedName.c_str());
					displayItemName->reflowTextToFit(0);
					if ( displayItemName->getNumTextLines() > 3 )
					{
						// ... doesn't fit, replace last 3 characters with ...
						copiedName = copiedName.substr(0U, copiedName.size() - 6);
						copiedName += "...";
						displayItemName->setText(copiedName.c_str());
						displayItemName->reflowTextToFit(0);
					}
				}

				// do highlights
				displayItemName->clearWordsToHighlight();
				std::string str = displayItemName->getText();
				if ( str == language[4167] )
				{
					displayItemName->setTextColor(hudColors.characterSheetRed);
				}
				else
				{
					displayItemName->setTextColor(hudColors.characterSheetLightNeutral);
				}
				int wordIndex = 0;
				bool prevCharWasWordSeparator = false;
				int numLines = 0;
				for ( size_t c = 0; c < str.size(); ++c )
				{
					if ( str[c] == '\n' )
					{
						wordIndex = -1;
						++numLines;
						prevCharWasWordSeparator = true;
						continue;
					}

					if ( prevCharWasWordSeparator && !charIsWordSeparator(str[c]) )
					{
						++wordIndex;
						if ( str[c] == '[' )
						{
							std::string toCompare = str.substr(str.find('[', c));
							Uint32 color = 0;
							if ( toCompare == language[4155] )
							{
								// unknown
								color = hudColors.characterSheetLightNeutral;
							}
							else if ( toCompare == language[4162]
								|| toCompare == language[4164] || toCompare == language[4166] )
							{
								color = makeColor(54, 144, 171, 255);
							}
							else if ( toCompare == language[4156] )
							{
								// base pot
								color = hudColors.characterSheetGreen;
							}
							else if ( toCompare == language[4163] )
							{
								// duplication chance
								color = hudColors.characterSheetGreen;
							}
							else if ( toCompare == language[4157] )
							{
								// secondary pot
								color = hudColors.characterSheetHighlightText;
							}
							else if ( toCompare == language[4158] )
							{
								color = hudColors.characterSheetFaintText;
							}
							else if ( toCompare == language[4160] || toCompare == language[4165] || toCompare == language[4168] )
							{
								color = hudColors.characterSheetRed;
							}
							displayItemName->addWordToHighlight(wordIndex + numLines * Field::TEXT_HIGHLIGHT_WORDS_PER_LINE,
								color);
							displayItemName->addWordToHighlight((wordIndex + 1) + numLines * Field::TEXT_HIGHLIGHT_WORDS_PER_LINE,
								color);
							displayItemName->addWordToHighlight((wordIndex + 2) + numLines * Field::TEXT_HIGHLIGHT_WORDS_PER_LINE,
								color);
							displayItemName->addWordToHighlight((wordIndex + 3) + numLines * Field::TEXT_HIGHLIGHT_WORDS_PER_LINE,
								color);
						}
						prevCharWasWordSeparator = false;
						if ( !(c + 1 < str.size() && charIsWordSeparator(str[c + 1])) )
						{
							continue;
						}
					}

					if ( charIsWordSeparator(str[c]) )
					{
						prevCharWasWordSeparator = true;
					}
					else
					{
						prevCharWasWordSeparator = false;
					}
				}
				itemRequiresTitleReflow = false;
			}
		}

		if ( itemActionType == ALCHEMY_ACTION_OK && strcmp(activateSelectionPrompt->getText(), "") )
		{
			if ( usingGamepad )
			{
				activateSelectionGlyph->path = Input::inputs[playernum].getGlyphPathForBinding("MenuConfirm");
			}
			else
			{
				activateSelectionGlyph->path = Input::inputs[playernum].getGlyphPathForBinding("MenuRightClick");
			}
			if ( auto imgGet = Image::get(activateSelectionGlyph->path.c_str()) )
			{
				activateSelectionGlyph->pos.w = imgGet->getWidth();
				activateSelectionGlyph->pos.h = imgGet->getHeight();
			}

			SDL_Rect pos{ 0, 0, activateSelectionGlyph->pos.w, activateSelectionGlyph->pos.h };
			pos.x += baseFrame->getSize().x + itemDisplayTooltip->getSize().x;
			pos.y += baseFrame->getSize().y + itemDisplayTooltip->getSize().y;
			pos.x += displayItemTextImg->pos.x + displayItemTextImg->pos.w / 2;
			pos.y += displayItemTextImg->pos.y + displayItemTextImg->pos.h;
			pos.y += 4;

			auto activateSelectionPromptPos = SDL_Rect{ pos.x, pos.y + 1, baseFrame->getSize().w, 24 };
			if ( auto textGet = activateSelectionPrompt->getTextObject() )
			{
				activateSelectionPromptPos.x -= textGet->getWidth() / 2;
				activateSelectionPromptPos.x += (8 + pos.w) / 2;
				pos.x = activateSelectionPromptPos.x - 8 - pos.w;
				pos.y += activateSelectionPrompt->getSize().h / 2;
				pos.y -= pos.h / 2;
			}
			activateSelectionPrompt->setSize(activateSelectionPromptPos);
			if ( pos.x % 2 == 1 )
			{
				++pos.x;
			}
			if ( pos.y % 2 == 1 )
			{
				--pos.y;
			}
			activateSelectionGlyph->pos = pos;
			activateSelectionGlyph->disabled = false;
			activateSelectionPrompt->setDisabled(false);
		}
	}
	else
	{
		if ( (!usingGamepad && (ticks - animTooltipTicks > TICKS_PER_SECOND / 3))
			|| (usingGamepad)
			|| animTooltip < 0.9999 )
		{
			const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.01, (animTooltip)) / 2.0;
			animTooltip -= setpointDiffX;
			animTooltip = std::max(0.0, animTooltip);
		}
	}

	{
		SDL_Color color;
		getColor(displayItemTextImg->color, &color.r, &color.g, &color.b, &color.a);
		color.a = (Uint8)(192 * animTooltip);
		displayItemTextImg->color = (makeColor(color.r, color.g, color.b, color.a));
	}

	{
		SDL_Color color;
		getColor(displayItemName->getColor(), &color.r, &color.g, &color.b, &color.a);
		color.a = (Uint8)(255 * animTooltip);
		displayItemName->setColor(makeColor(color.r, color.g, color.b, color.a));
	}

	{
		SDL_Color color;
		getColor(activateSelectionPrompt->getColor(), &color.r, &color.g, &color.b, &color.a);
		color.a = (Uint8)(255 * animTooltip);
		activateSelectionPrompt->setColor(makeColor(color.r, color.g, color.b, color.a));
	}

	{
		SDL_Color color;
		getColor(activateSelectionGlyph->color, &color.r, &color.g, &color.b, &color.a);
		color.a = (Uint8)(255 * animTooltip);
		activateSelectionGlyph->color = (makeColor(color.r, color.g, color.b, color.a));
	}

	bool tryBrew = false;
	bool activateSelection = false;
	if ( isInteractable )
	{
		if ( !inputs.getUIInteraction(playernum)->selectedItem
			&& !player->GUI.isDropdownActive()
			&& (player->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_ALCHEMY)
				|| player->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_INVENTORY))
			&& player->bControlEnabled && !gamePaused
			&& !player->usingCommand() )
		{
			
			if ( Input::inputs[playernum].binaryToggle("MenuCancel") )
			{
				Input::inputs[playernum].consumeBinaryToggle("MenuCancel");
				if ( potion1Uid == 0 && potion2Uid == 0 && recipes.activateRecipeIndex == -1 )
				{
					parentGUI.closeGUI();
					return;
				}
				else if ( recipes.activateRecipeIndex >= 0 )
				{
					if ( animPotionResult < 0.001 )
					{
						if ( recipesFrame )
						{
							if ( auto baseFrame = recipesFrame->findFrame("recipe base") )
							{
								if ( auto recipesBtn = baseFrame->findButton("clear recipe") )
								{
									if ( !recipesBtn->isInvisible() )
									{
										recipesBtn->activate();
									}
								}
							}
						}
					}
				}
				else if ( animPotionResult < 0.001 )
				{
					if ( potion2Uid != 0 )
					{
						potion2Uid = 0;
						animPotion2 = 0.0;
						//animPotion2Frame->setDisabled(true);
						animRecipeAutoAddToSlot1Uid = 0;
						animRecipeAutoAddToSlot2Uid = 0;

						recipes.activateRecipeIndex = -1;
						animRandomPotionTicks = 0;
					}
					else if ( potion1Uid != 0 )
					{
						potion1Uid = 0;
						animPotion1 = 0.0;
						//animPotion1Frame->setDisabled(true);
						animRecipeAutoAddToSlot1Uid = 0;
						animRecipeAutoAddToSlot2Uid = 0;

						recipes.activateRecipeIndex = -1;
						animRandomPotionTicks = 0;
					}
					playSound(139, 64); // click sound
				}
			}
			else if ( Input::inputs[playernum].binaryToggle("MenuPageRightAlt") || Input::inputs[playernum].binaryToggle("MenuPageLeftAlt") )
			{
				Input::inputs[playernum].consumeBinaryToggle("MenuPageLeftAlt");
				Input::inputs[playernum].consumeBinaryToggle("MenuPageRightAlt");
				recipeBtn->activate();
				return;
			}
			else if ( Input::inputs[playernum].binaryToggle("MenuAlt2") )
			{
				if ( !brewGlyph->disabled )
				{
					activateSelection = true;
					tryBrew = true;
					Input::inputs[playernum].consumeBinaryToggle("MenuAlt2");
				}
			}
			else
			{
				if ( usingGamepad && Input::inputs[playernum].binaryToggle("MenuConfirm") )
				{
					activateSelection = true;
					Input::inputs[playernum].consumeBinaryToggle("MenuConfirm");
				}
				else if ( !usingGamepad && Input::inputs[playernum].binaryToggle("MenuRightClick") )
				{
					activateSelection = true;
					Input::inputs[playernum].consumeBinaryToggle("MenuRightClick");
				}
			}
		}
	}

	if ( activateSelection && players[playernum] && players[playernum]->entity
		&& animPotionResult < 0.001 )
	{
		parentGUI.basePotion = nullptr;
		parentGUI.secondaryPotion = nullptr;
		if ( itemActionType != ALCHEMY_ACTION_OK && !tryBrew && itemActionType != ALCHEMY_ACTION_NONE )
		{
			playSound(90, 64);
		}
		if ( player->GUI.activeModule == Player::GUI_t::MODULE_ALCHEMY
			&& getSelectedAlchemySlotX() >= 0 && getSelectedAlchemySlotX() < MAX_ALCH_X
			&& getSelectedAlchemySlotY() >= 0 && getSelectedAlchemySlotX() < MAX_ALCH_Y
			&& !tryBrew
			&& recipes.bOpen
			/*&& currentView == ALCHEMY_VIEW_RECIPES*/
			&& itemActionType == ALCHEMY_ACTION_OK )
		{
			if ( recipes.isInteractable 
				&& animRecipeAutoAddToSlot1Uid == 0
				&& animRecipeAutoAddToSlot2Uid == 0 )
			{
				int index = 0;
				for ( auto& entry : recipes.recipeList )
				{
					if ( entry.x == getSelectedAlchemySlotX() && entry.y == getSelectedAlchemySlotY()
						&& !hideRecipeFromList(entry.resultItem.type) )
					{
						if ( recipes.activateRecipeIndex == index )
						{
							recipes.activateRecipeIndex = -1; // clear selection on re-select

							potion1Uid = 0;
							animPotion1 = 0.0;
							//animPotion1Frame->setDisabled(true);
							potion2Uid = 0;
							animPotion2 = 0.0;
							//animPotion2Frame->setDisabled(true);

							animRecipeAutoAddToSlot1Uid = 0;
							animRecipeAutoAddToSlot2Uid = 0;
							playSound(139, 64); // click sound
						}
						else if ( animRecipeAutoAddToSlot1Uid != 0 || animRecipeAutoAddToSlot2Uid != 0
							|| animPotion1 > 0.001 || animPotion2 > 0.001 )
						{
							// do nothing
						}
						else if ( entry.basePotionUid > 0 && entry.secondaryPotionUid > 0 )
						{
							if ( entry.basePotionUid == potion1Uid && entry.secondaryPotionUid == potion2Uid )
							{
								// potions already in their slots
								recipes.activateRecipeIndex = index;
							}
							else
							{
								potion1Uid = 0;
								animPotion1 = 0.0;
								animPotion1Frame->setDisabled(true);
								potion2Uid = 0;
								animPotion2 = 0.0;
								animPotion2Frame->setDisabled(true);

								recipes.activateRecipeIndex = index;

								animRecipeAutoAddToSlot1Uid = entry.basePotionUid;
								animRecipeAutoAddToSlot2Uid = entry.secondaryPotionUid;
								alchemyResultPotion.type = POTION_EMPTY;
							}
							playSound(139, 64); // click sound
						}
						break;
					}
					++index;
				}
			}
		}
		else if ( (player->GUI.activeModule == Player::GUI_t::MODULE_ALCHEMY || (tryBrew && player->GUI.activeModule == Player::GUI_t::MODULE_INVENTORY) )
			&& (itemActionType == ALCHEMY_ACTION_OK || tryBrew)
			&& animRecipeAutoAddToSlot1Uid == 0
			&& animRecipeAutoAddToSlot2Uid == 0 )
		{
			bool tryRecipeAutofill = false;
			ItemType oldPotion1Type = WOODEN_SHIELD;
			ItemType oldPotion2Type = WOODEN_SHIELD;
			if ( (getSelectedAlchemySlotX() >= ALCH_SLOT_RESULT_POTION_X && getSelectedAlchemySlotX() < 0
				&& getSelectedAlchemySlotY() == 0) || tryBrew )
			{
				if ( !tryBrew && getSelectedAlchemySlotX() == ALCH_SLOT_BASE_POTION_X )
				{
					potion1Uid = 0;
					animPotion1 = 0.0;
					animPotion1Frame->setDisabled(true);
					recipes.activateRecipeIndex = -1; // clear active recipe
					playSound(139, 64); // click sound
				}
				else if ( !tryBrew && getSelectedAlchemySlotX() == ALCH_SLOT_SECONDARY_POTION_X )
				{
					potion2Uid = 0;
					animPotion2 = 0.0;
					animPotion2Frame->setDisabled(true);
					recipes.activateRecipeIndex = -1; // clear active recipe
					playSound(139, 64); // click sound

				}
				else if ( tryBrew || getSelectedAlchemySlotX() == ALCH_SLOT_RESULT_POTION_X )
				{
					// brew it
					if ( potion1Item && potion2Item && !potionResultFrame->isDisabled()
						&& alchemyResultPotion.type != POTION_EMPTY
						&& potionResultUid == 0 )
					{
						parentGUI.basePotion = potion1Item;
						parentGUI.secondaryPotion = potion2Item;
						int oldCount1 = potion1Item->count;
						int oldCount2 = potion2Item->count;
						oldPotion1Type = potion1Item->type;
						oldPotion2Type = potion2Item->type;
						potionResultUid = 0;
						parentGUI.alchemyCombinePotions();
						if ( parentGUI.basePotion == nullptr )
						{
							potion1Uid = 0;
							animPotion1 = 0.0;
							animPotion1Frame->setDisabled(true);
							alchemyResultPotion.type = POTION_EMPTY;
							tryRecipeAutofill = recipes.activateRecipeIndex >= 0;
						}
						if ( parentGUI.secondaryPotion == nullptr )
						{
							potion2Uid = 0;
							animPotion2 = 0.0;
							animPotion2Frame->setDisabled(true);
							alchemyResultPotion.type = POTION_EMPTY;
							tryRecipeAutofill = recipes.activateRecipeIndex >= 0;
						}
						if ( potionResultUid != 0 )
						{
							if ( auto item = uidToItem(potionResultUid) )
							{
								auto slotType = player->paperDoll.getSlotForItem(*item);
								if ( potionResultUid == potion1Uid )
								{
									animPotionResultDestX = animPotion1DestX;
									animPotionResultDestY = animPotion1DestY;
									animPotionResult = 1.0;
								}
								else if ( potionResultUid == potion2Uid )
								{
									animPotionResultDestX = animPotion2DestX;
									animPotionResultDestY = animPotion2DestY;
									animPotionResult = 1.0;
								}
								else if ( slotType != Player::PaperDoll_t::SLOT_MAX ) // on paper doll
								{
									animPotionResultDestX = animPotionResultStartX;
									animPotionResultDestY = animPotionResultStartY - player->inventoryUI.getSlotSize();
									animPotionResult = 1.0;
								}
								else if ( auto slotFrame = player->inventoryUI.getInventorySlotFrame(item->x, item->y) )
								{
									getInventoryItemAlchemyAnimSlotPos(slotFrame, player, item->x, item->y, animPotionResultDestX, animPotionResultDestY, potionAnimOffsetY);
									animPotionResultDestY += 2;
									animPotionResultDestY += (player->inventoryUI.bCompactView ? -2 : 0);
									animPotionResult = 1.0;
								}
							}
						}
						if ( animPotionResult < .999 )
						{
							potionResultUid = 0;
						}
					}
					parentGUI.basePotion = nullptr;
					parentGUI.secondaryPotion = nullptr;
				}
			}
			if ( tryRecipeAutofill )
			{
				buildRecipeList(playernum);
				int index = 0;
				bool autofilled = false;
				for ( auto& entry : recipes.recipeList )
				{
					if ( !hideRecipeFromList(entry.resultItem.type) )
					{
						if ( entry.dummyPotion1.type == oldPotion1Type && entry.dummyPotion2.type == oldPotion2Type )
						{
							if ( entry.basePotionUid == 0 || entry.secondaryPotionUid == 0 )
							{
								// no auto fill available
								break;
							}
							if ( entry.basePotionUid > 0 && potion1Uid == 0 )
							{
								animRecipeAutoAddToSlot1Uid = entry.basePotionUid;
								autofilled = true;
							}
							if ( entry.secondaryPotionUid > 0 && potion2Uid == 0 )
							{
								animRecipeAutoAddToSlot2Uid = entry.secondaryPotionUid;
								autofilled = true;
							}
							break;
						}
					}
					++index;
				}
				if ( !autofilled )
				{
					recipes.activateRecipeIndex = -1;
				}
			}
		}
		else if ( player->GUI.activeModule == Player::GUI_t::MODULE_INVENTORY
			&& itemActionType == ALCHEMY_ACTION_OK
			&& !tryBrew
			&& animRecipeAutoAddToSlot1Uid == 0
			&& animRecipeAutoAddToSlot2Uid == 0 )
		{
			if ( auto slotFrame = player->inventoryUI.getInventorySlotFrame(player->inventoryUI.getSelectedSlotX(),
				player->inventoryUI.getSelectedSlotY()) )
			{
				for ( node_t* node = stats[playernum]->inventory.first; node != NULL; node = node->next )
				{
					Item* item = (Item*)node->element;
					if ( !item )
					{
						continue;
					}
					if ( itemCategory(item) == SPELL_CAT )
					{
						continue;
					}

					if ( item->x == player->inventoryUI.getSelectedSlotX()
						&& item->y == player->inventoryUI.getSelectedSlotY()
						&& item->x >= 0 && item->x < player->inventoryUI.getPlayerItemInventoryX()
						&& item->y >= 0 && item->y < player->inventoryUI.getPlayerItemInventoryY() )
					{
						if ( potion1Uid == item->uid )
						{
							potion1Uid = 0;
							animPotion1 = 0.0;
							animPotion1Frame->setDisabled(true);
						}
						else if ( potion2Uid == item->uid )
						{
							potion2Uid = 0;
							animPotion2 = 0.0;
							animPotion2Frame->setDisabled(true);
						}
						else if ( potion1Uid == 0 )
						{
							if ( !parentGUI.isItemMixable(item) )
							{
								continue;
							}
							animPotion1 = 1.0;
							getInventoryItemAlchemyAnimSlotPos(slotFrame, player, item->x, item->y, animPotion1StartX, animPotion1StartY, potionAnimOffsetY);
							potion1Uid = item->uid;
							//alchemyResultPotion.type = POTION_EMPTY;
						}
						else
						{
							if ( !parentGUI.isItemMixable(item) )
							{
								continue;
							}
							animPotion2 = 1.0;
							getInventoryItemAlchemyAnimSlotPos(slotFrame, player, item->x, item->y, animPotion2StartX, animPotion2StartY, potionAnimOffsetY);
							potion2Uid = item->uid;
							//alchemyResultPotion.type = POTION_EMPTY;
						}
						recipes.activateRecipeIndex = -1;
						animRandomPotionTicks = 0;
						playSound(139, 64); // click sound
						break;
					}
				}
			}
		}
	}
}

const int kRecipeListHeight = 250;
const int kRecipeListGridY = 0;
const int kRecipeHeaderHeight = 46;
const int kRecipeFooterHeight = 32;
const int kRecipeGridImgHeight = 240;

void GenericGUIMenu::AlchemyGUI_t::createAlchemyMenu()
{
	const int player = parentGUI.getPlayer();
	if ( !gui || !alchFrame || !players[player]->inventoryUI.frame )
	{
		return;
	}
	if ( alchemyGUIHasBeenCreated() )
	{
		return;
	}

	SDL_Rect basePos{ 0, 0, alchemyBaseWidth, 350 };
	alchemySlotFrames.clear();
	const int recipeWidth = 196;
	Frame* frame = alchFrame->addFrame("player recipes");
	recipesFrame = frame;
	frame->setSize(SDL_Rect{ 0,
		16,
		recipeWidth,
		kRecipeListHeight });
	frame->setHollow(true);
	frame->setBorder(0);
	frame->setOwner(player);
	frame->setInheritParentFrameOpacity(false);
	frame->setDisabled(true);
	const int inventorySlotSize = players[player]->inventoryUI.getSlotSize();

	{
		int numGrids = (MAX_ALCH_Y / recipes.kNumRecipesToDisplayVertical) + 1;
		const int baseSlotOffsetX = 0;
		const int baseSlotOffsetY = 0;
		const int baseGridOffsetY = kRecipeListGridY;

		SDL_Rect recipeSlotsPos{ 6, 0, recipeWidth, kRecipeListHeight };
		auto recipeSlotsFrame = recipesFrame->addFrame("alchemy slots");
		recipeSlotsFrame->setSize(recipeSlotsPos);
		recipeSlotsFrame->setActualSize(SDL_Rect{ 0, 0, recipeSlotsPos.w, (kRecipeGridImgHeight + 2) * numGrids });
		recipeSlotsFrame->setHollow(true);
		recipeSlotsFrame->setAllowScrollBinds(false);
		recipeSlotsFrame->setScrollBarsEnabled(false);

		auto gridImg = recipeSlotsFrame->addImage(SDL_Rect{ baseSlotOffsetX, baseGridOffsetY, 162, (kRecipeGridImgHeight + 2) * numGrids },
			0xFFFFFFFF, "*images/ui/Alchemy/Alchemy_ScrollGrid.png", "grid img");
		gridImg->tiled = true;

		SDL_Rect currentSlotPos{ baseSlotOffsetX, baseSlotOffsetY, inventorySlotSize, inventorySlotSize };
		const int maxRecipesX = MAX_ALCH_X;
		const int maxRecipesY = MAX_ALCH_Y;

		auto cursorFrame = recipeSlotsFrame->addFrame("active recipe");
		cursorFrame->setSize(SDL_Rect{ 0, 0, inventorySlotSize, inventorySlotSize });
		cursorFrame->setDisabled(true);
		auto color = makeColor(177, 72, 3, 255);
		cursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/SelectorGrey_TL.png", "cursor topleft");
		cursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/SelectorGrey_TR.png", "cursor topright");
		cursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/SelectorGrey_BL.png", "cursor bottomleft");
		cursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/SelectorGrey_BR.png", "cursor bottomright");

		for ( int x = 0; x < maxRecipesX; ++x )
		{
			currentSlotPos.x = baseSlotOffsetX + (x * inventorySlotSize);
			for ( int y = 0; y < maxRecipesY; ++y )
			{
				currentSlotPos.y = baseSlotOffsetY + (y * inventorySlotSize);

				char slotname[32] = "";
				snprintf(slotname, sizeof(slotname), "recipe %d %d", x, y);

				auto slotFrame = recipeSlotsFrame->addFrame(slotname);
				alchemySlotFrames[x + y * 100] = slotFrame;
				SDL_Rect slotPos{ currentSlotPos.x, currentSlotPos.y, inventorySlotSize, inventorySlotSize };
				slotFrame->setSize(slotPos);

				createPlayerInventorySlotFrameElements(slotFrame);
			}
		}
	}

	auto bgImgFrame = recipesFrame->addFrame("recipe img frame");
	bgImgFrame->setSize(SDL_Rect{ 0, 0, recipeWidth, 328 });
	bgImgFrame->setHollow(true);
	auto bg = bgImgFrame->addImage(SDL_Rect{ 0, 0, recipeWidth, 328 },
		makeColor(255, 255, 255, 255),
		"*#images/ui/Alchemy/Alchemy_Recipes_00.png", "recipe base img");

	{
		auto bgFrame = recipesFrame->addFrame("recipe base");
		bgFrame->setSize(SDL_Rect{ 0, 0, recipeWidth, kRecipeListHeight });
		bgFrame->setHollow(false);

		auto title1 = bgFrame->addField("recipe title 1", 64);
		title1->setFont("fonts/pixel_maz_multiline.ttf#16#2");
		title1->setText("");
		title1->setHJustify(Field::justify_t::LEFT);
		title1->setVJustify(Field::justify_t::TOP);
		title1->setSize(SDL_Rect{ 0, 0, 0, 24 });
		title1->setDisabled(true);

		auto clearRecipeBtn = bgFrame->addButton("clear recipe");
		clearRecipeBtn->setSize(SDL_Rect{ 0, 0, 172, 26 });
		clearRecipeBtn->setColor(makeColor(255, 255, 255, 255));
		clearRecipeBtn->setHighlightColor(makeColor(255, 255, 255, 255));
		clearRecipeBtn->setText(language[4169]);
		clearRecipeBtn->setFont("fonts/pixel_maz_multiline.ttf#16#2");
		clearRecipeBtn->setHideGlyphs(true);
		clearRecipeBtn->setHideKeyboardGlyphs(true);
		clearRecipeBtn->setHideSelectors(true);
		clearRecipeBtn->setBackground("*#images/ui/Alchemy/Alchemy_RecipeClear_Button_00.png");
		clearRecipeBtn->setColor(makeColor(255, 255, 255, 255));
		clearRecipeBtn->setHighlightColor(makeColor(255, 255, 255, 255));
		clearRecipeBtn->setMenuConfirmControlType(0);
		clearRecipeBtn->setBackgroundHighlighted("*#images/ui/Alchemy/Alchemy_RecipeClear_ButtonHigh_00.png");
		clearRecipeBtn->setBackgroundActivated("*#images/ui/Alchemy/Alchemy_RecipeClear_ButtonPress_00.png");
		clearRecipeBtn->setTextHighlightColor(makeColor(201, 162, 100, 255));
		clearRecipeBtn->setCallback([](Button& button) {
			int player = button.getOwner();
			auto& alchemyGUI = GenericGUI[player].alchemyGUI;
			alchemyGUI.recipes.activateRecipeIndex = -1;
			alchemyGUI.potion1Uid = 0;
			alchemyGUI.animPotion1 = 0.0;
			alchemyGUI.potion2Uid = 0;
			alchemyGUI.animPotion2 = 0.0;
			alchemyGUI.alchemyResultPotion.type = POTION_EMPTY;
			alchemyGUI.potionResultUid = 0;
			alchemyGUI.animPotionResult = 0.0;
			alchemyGUI.animRecipeAutoAddToSlot1Uid = 0;
			alchemyGUI.animRecipeAutoAddToSlot2Uid = 0;
			playSound(139, 64); // click sound
		});
		clearRecipeBtn->setTickCallback([](Widget& widget)
		{
			genericgui_deselect_fn(widget);
		});
		clearRecipeBtn->setDisabled(true);

		auto clearRecipeGlyph = bgFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
			0xFFFFFFFF, "", "clear recipe glyph");
		clearRecipeGlyph->disabled = true;
		clearRecipeGlyph->ontop = true;

		auto slider = bgFrame->addSlider("recipe slider");
		slider->setBorder(16);
		slider->setMinValue(0);
		slider->setMaxValue(100);
		slider->setValue(0);
		SDL_Rect sliderPos{ recipeWidth - 26, 8, 20, kRecipeListHeight - 2 * 8 };
		slider->setRailSize(sliderPos);
		slider->setHandleSize(SDL_Rect{ 0, 0, 20, 28 });
		slider->setOrientation(Slider::SLIDER_VERTICAL);
		//slider->setCallback(callback);
		slider->setColor(makeColor(255, 255, 255, 255));
		slider->setHighlightColor(makeColor(255, 255, 255, 255));
		slider->setHandleImage("*#images/ui/Sliders/HUD_Magic_Slider_Emerald_01.png");
		slider->setRailImage("*#images/ui/Sliders/HUD_Slider_Blank.png");
		slider->setHideGlyphs(true);
		slider->setDisabled(true);
		slider->setHideKeyboardGlyphs(true);
		slider->setHideSelectors(true);
		slider->setMenuConfirmControlType(0);
	}

	{
		auto notificationFrame = alchFrame->addFrame("notification");
		notificationFrame->setHollow(true);
		notificationFrame->setBorder(0);
		notificationFrame->setInheritParentFrameOpacity(false);
		notificationFrame->setDisabled(true);
		notificationFrame->setSize(SDL_Rect{ 0, 0, 180, 56 });

		auto notifBg = notificationFrame->addImage(SDL_Rect{ 0, 0, 180, 56 }, 0xFFFFFFFF,
			"*#images/ui/Alchemy/Alchemy_Notification_00.png", "notif bg");

		auto notifIcon = notificationFrame->addImage(SDL_Rect{ 8, 56 / 2 - players[player]->inventoryUI.getItemSpriteSize() / 2,
			players[player]->inventoryUI.getItemSpriteSize(),
			players[player]->inventoryUI.getItemSpriteSize() }, 0xFFFFFFFF,
			"", "notif icon");

		auto title = notificationFrame->addField("notif title", 128);
		title->setFont("fonts/pixel_maz_multiline.ttf#16#2");
		title->setText("New Title Unlocked!");
		title->setHJustify(Field::justify_t::LEFT);
		title->setVJustify(Field::justify_t::TOP);
		title->setSize(SDL_Rect{ notifIcon->pos.x + notifIcon->pos.w, 8, notificationFrame->getSize().w, 24 });
		title->setColor(makeColor(255, 255, 0, 255));

		auto body = notificationFrame->addField("notif body", 128);
		body->setFont("fonts/pixel_maz_multiline.ttf#16#2");
		body->setText("Blah Blah Blah!");
		body->setHJustify(Field::justify_t::LEFT);
		body->setVJustify(Field::justify_t::TOP);
		body->setSize(SDL_Rect{ notifIcon->pos.x + notifIcon->pos.w, 8 + 18, notificationFrame->getSize().w, 24 });
		body->setColor(makeColor(255, 255, 255, 255));
	}

	{
		auto bgFrame = alchFrame->addFrame("alchemy base");
		bgFrame->setSize(basePos);
		bgFrame->setHollow(true);
		bgFrame->setDisabled(true);
		auto bg = bgFrame->addImage(SDL_Rect{ 0, 0, basePos.w, basePos.h },
			makeColor(255, 255, 255, 255),
			"*#images/ui/Alchemy/Alchemy_Base_01.png", "alchemy base img");

		auto headerFont = "fonts/pixel_maz_multiline.ttf#16#2";
		auto alembicTitle = bgFrame->addField("alchemy alembic title", 128);
		alembicTitle->setFont(headerFont);
		alembicTitle->setText("");
		alembicTitle->setHJustify(Field::justify_t::CENTER);
		alembicTitle->setVJustify(Field::justify_t::TOP);
		alembicTitle->setSize(SDL_Rect{ 0, 0, 0, 0 });
		alembicTitle->setTextColor(hudColors.characterSheetLightNeutral);
		alembicTitle->setOutlineColor(makeColor(29, 16, 11, 255));
		auto alembicStatus = bgFrame->addField("alchemy alembic status", 128);
		alembicStatus->setFont(headerFont);
		alembicStatus->setText("");
		alembicStatus->setHJustify(Field::justify_t::CENTER);
		alembicStatus->setVJustify(Field::justify_t::TOP);
		alembicStatus->setSize(SDL_Rect{ 0, 0, 0, 0 });
		alembicStatus->setTextColor(hudColors.characterSheetLightNeutral);
		alembicStatus->setOutlineColor(makeColor(29, 16, 11, 255));

		auto itemFont = "fonts/pixel_maz_multiline.ttf#16#2";
		{
			auto itemDisplayTooltip = bgFrame->addFrame("alchemy display tooltip");
			itemDisplayTooltip->setSize(SDL_Rect{ 0, 0, 186, 108 });
			itemDisplayTooltip->setHollow(true);
			itemDisplayTooltip->setInheritParentFrameOpacity(false);
			{
				auto itemNameText = itemDisplayTooltip->addField("item display name", 1024);
				itemNameText->setFont(itemFont);
				itemNameText->setText("");
				itemNameText->setHJustify(Field::justify_t::LEFT);
				itemNameText->setVJustify(Field::justify_t::TOP);
				itemNameText->setSize(SDL_Rect{ 0, 0, 0, 0 });
				itemNameText->setTextColor(hudColors.characterSheetLightNeutral);

				auto itemDisplayTextBg = itemDisplayTooltip->addImage(SDL_Rect{ 0, 0, 186, 42 },
					0xFFFFFFFF, "*#images/ui/Alchemy/Alchemy_LabelName_2Row_00.png", "item text img");
			}
		}
		{
			auto recipeBtn = bgFrame->addButton("recipe button");
			SDL_Rect recipeBtnPos{ 12, 72, 182, 40 };
			recipeBtn->setSize(recipeBtnPos);
			recipeBtn->setColor(makeColor(255, 255, 255, 255));
			recipeBtn->setHighlightColor(makeColor(255, 255, 255, 255));
			recipeBtn->setText(language[4170]);
			recipeBtn->setFont(itemFont);
			recipeBtn->setHideGlyphs(true);
			recipeBtn->setHideKeyboardGlyphs(true);
			recipeBtn->setHideSelectors(true);
			recipeBtn->setMenuConfirmControlType(0);
			recipeBtn->setBackground("*#images/ui/Alchemy/Alchemy_Recipe_Button_00.png");
			recipeBtn->setColor(makeColor(255, 255, 255, 255));
			recipeBtn->setHighlightColor(makeColor(255, 255, 255, 255));
			recipeBtn->setBackgroundHighlighted("*#images/ui/Alchemy/Alchemy_Recipe_ButtonHigh_00.png");
			recipeBtn->setBackgroundActivated("*#images/ui/Alchemy/Alchemy_Recipe_ButtonPress_00.png");
			//recipeBtn->setTextHighlightColor(makeColor(201, 162, 100, 255));
			recipeBtn->setCallback([](Button& button) {
				int player = button.getOwner();
				if ( GenericGUI[player].alchemyGUI.bOpen )
				{
					if ( GenericGUI[player].alchemyGUI.recipes.bOpen )
					{
						GenericGUI[player].alchemyGUI.recipes.closeRecipePanel();
					}
					else
					{
						GenericGUI[player].alchemyGUI.recipes.openRecipePanel();
					}
					playSound(139, 64); // click sound
				}
			});
			recipeBtn->setTickCallback([](Widget& widget)
			{
				genericgui_deselect_fn(widget);
			});

			auto openRecipesGlyph = bgFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
				0xFFFFFFFF, "", "recipe glyph");
			openRecipesGlyph->disabled = true;
			openRecipesGlyph->ontop = true;

			auto closeBtn = bgFrame->addButton("close alchemy button");
			SDL_Rect closeBtnPos{ basePos.w - 0 - 26, 0, 26, 26 };
			closeBtn->setSize(closeBtnPos);
			closeBtn->setColor(makeColor(255, 255, 255, 255));
			closeBtn->setHighlightColor(makeColor(255, 255, 255, 255));
			closeBtn->setText("X");
			closeBtn->setFont(itemFont);
			closeBtn->setHideGlyphs(true);
			closeBtn->setHideKeyboardGlyphs(true);
			closeBtn->setHideSelectors(true);
			closeBtn->setMenuConfirmControlType(0);
			closeBtn->setBackground("*#images/ui/Alchemy/Button_X_00.png");
			closeBtn->setBackgroundHighlighted("*#images/ui/Alchemy/Button_XHigh_00.png");
			closeBtn->setBackgroundActivated("*#images/ui/Alchemy/Button_XPress_00.png");
			closeBtn->setTextHighlightColor(makeColor(201, 162, 100, 255));
			closeBtn->setCallback([](Button& button) {
				GenericGUI[button.getOwner()].closeGUI();
			});
			closeBtn->setTickCallback([](Widget& widget)
			{
				genericgui_deselect_fn(widget);
			});

			auto closeGlyph = bgFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
				0xFFFFFFFF, "", "close alchemy glyph");
			closeGlyph->disabled = true;
			closeGlyph->ontop = true;

			auto brewBtn = bgFrame->addButton("brew button");
			SDL_Rect brewPos{ basePos.w - 18 - 64, basePos.h - 20 - 46, 64, 46 };
			brewBtn->setSize(brewPos);
			brewBtn->setColor(makeColor(255, 255, 255, 255));
			brewBtn->setHighlightColor(makeColor(255, 255, 255, 255));
			brewBtn->setText("Brew");
			brewBtn->setFont(itemFont);
			brewBtn->setHideGlyphs(true);
			brewBtn->setHideKeyboardGlyphs(true);
			brewBtn->setHideSelectors(true);
			brewBtn->setMenuConfirmControlType(0);
			brewBtn->setBackground("*#images/ui/Alchemy/Alchemy_ButtonBrew_Base_00.png");
			brewBtn->setBackgroundHighlighted("*#images/ui/Alchemy/Alchemy_ButtonBrew_High_00.png");
			brewBtn->setBackgroundActivated("*#images/ui/Alchemy/Alchemy_ButtonBrew_Press_00.png");
			brewBtn->setTextHighlightColor(makeColor(201, 162, 100, 255));
			brewBtn->setCallback([](Button& button) {
				int player = button.getOwner();
				auto& alchemyGUI = GenericGUI[player].alchemyGUI;
				alchemyGUI.recipes.activateRecipeIndex = -1;
				alchemyGUI.potion1Uid = 0;
				alchemyGUI.animPotion1 = 0.0;
				alchemyGUI.potion2Uid = 0;
				alchemyGUI.animPotion2 = 0.0;
				alchemyGUI.alchemyResultPotion.type = POTION_EMPTY;
				alchemyGUI.potionResultUid = 0;
				alchemyGUI.animPotionResult = 0.0;
				alchemyGUI.animRecipeAutoAddToSlot1Uid = 0;
				alchemyGUI.animRecipeAutoAddToSlot2Uid = 0;
			});
			brewBtn->setTickCallback([](Widget& widget)
			{
				genericgui_deselect_fn(widget);
			});

			auto brewGlyph = bgFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
				0xFFFFFFFF, "", "brew glyph");
			brewGlyph->disabled = true;
			brewGlyph->ontop = true;
		}

		{
			Frame* slotFrame = alchFrame->addFrame("potion recipe 1 frame");
			SDL_Rect slotPos{ 0, 0, players[player]->inventoryUI.getSlotSize(), players[player]->inventoryUI.getSlotSize() };
			slotFrame->setSize(slotPos);
			slotFrame->setDisabled(true);
			slotFrame->setInheritParentFrameOpacity(false);
			createPlayerInventorySlotFrameElements(slotFrame);
			alchemySlotFrames[ALCH_SLOT_RECIPE_PREVIEW_POTION1_X + 0 * 100] = slotFrame;
			slotFrame = alchFrame->addFrame("potion recipe 2 frame");
			slotFrame->setSize(slotPos);
			slotFrame->setDisabled(true);
			slotFrame->setInheritParentFrameOpacity(false);
			createPlayerInventorySlotFrameElements(slotFrame);
			alchemySlotFrames[ALCH_SLOT_RECIPE_PREVIEW_POTION2_X + 0 * 100] = slotFrame;
		}

		{
			Frame* slotFrame = alchFrame->addFrame("potion 1 frame");
			SDL_Rect slotPos{ 0, 0, players[player]->inventoryUI.getSlotSize(), players[player]->inventoryUI.getSlotSize() };
			slotFrame->setSize(slotPos);
			slotFrame->setDisabled(true);
			slotFrame->setInheritParentFrameOpacity(false);
			createPlayerInventorySlotFrameElements(slotFrame);
			alchemySlotFrames[ALCH_SLOT_BASE_POTION_X + 0 * 100] = slotFrame;
			slotFrame = alchFrame->addFrame("potion 2 frame");
			slotFrame->setSize(slotPos);
			slotFrame->setDisabled(true);
			slotFrame->setInheritParentFrameOpacity(false);
			createPlayerInventorySlotFrameElements(slotFrame);
			alchemySlotFrames[ALCH_SLOT_SECONDARY_POTION_X + 0 * 100] = slotFrame;
			slotFrame = alchFrame->addFrame("potion result frame");
			slotFrame->setSize(slotPos);
			slotFrame->setDisabled(true);
			slotFrame->setInheritParentFrameOpacity(false);
			createPlayerInventorySlotFrameElements(slotFrame);
			alchemySlotFrames[ALCH_SLOT_RESULT_POTION_X + 0 * 100] = slotFrame;
		}

		auto emptyBottleFrame = bgFrame->addFrame("empty bottles");
		SDL_Rect slotPos{ 20, 264 + 30, inventorySlotSize, inventorySlotSize };
		emptyBottleFrame->setSize(slotPos);
		emptyBottleFrame->setDisabled(true);
		createPlayerInventorySlotFrameElements(emptyBottleFrame);
	}

	auto activateSelectionGlyph = alchFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
		0xFFFFFFFF, "", "activate glyph");
	activateSelectionGlyph->disabled = true;
	activateSelectionGlyph->ontop = true;
	auto activateSelectionPrompt = alchFrame->addField("activate prompt", 64);
	activateSelectionPrompt->setFont("fonts/pixel_maz_multiline.ttf#16#2");
	activateSelectionPrompt->setText("");
	activateSelectionPrompt->setHJustify(Field::justify_t::LEFT);
	activateSelectionPrompt->setVJustify(Field::justify_t::TOP);
	activateSelectionPrompt->setSize(SDL_Rect{ 0, 0, 0, 0 });
	activateSelectionPrompt->setColor(makeColor(255, 255, 255, 255));
	activateSelectionPrompt->setDisabled(true);
	activateSelectionPrompt->setOntop(true);
}

void GenericGUIMenu::AlchemyGUI_t::selectAlchemySlot(const int x, const int y)
{
	selectedAlchemySlotX = x;
	selectedAlchemySlotY = y;
}

Frame* GenericGUIMenu::AlchemyGUI_t::getAlchemySlotFrame(int x, int y) const
{
	if ( alchFrame )
	{
		int key = x + y * 100;
		if ( alchemySlotFrames.find(key) != alchemySlotFrames.end() )
		{
			return alchemySlotFrames.at(key);
		}
		//assert(alchemySlotFrames.find(key) == alchemySlotFrames.end());
	}
	return nullptr;
}

void GenericGUIMenu::AlchemyGUI_t::setItemDisplayNameAndPrice(Item* item, bool isTooltipForResultPotion, bool isTooltipForRecipe)
{
	itemActionType = ALCHEMY_ACTION_NONE;
	if ( !item || item->type == SPELL_ITEM )
	{
		clearItemDisplayed();
	}
	if ( !item )
	{
		return;
	}

	itemTooltipForRecipe = isTooltipForRecipe;

	char buf[1024];
	if ( !item->identified )
	{
		if ( isTooltipForResultPotion )
		{
			snprintf(buf, sizeof(buf), "%s (?)", language[4161]);
		}
		else
		{
			snprintf(buf, sizeof(buf), "%s %s (?)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), item->getName());
		}
	}
	else if ( item->type == TOOL_BOMB && isTooltipForResultPotion )
	{
		snprintf(buf, sizeof(buf), "%s", language[4167]);
	}
	else
	{
		snprintf(buf, sizeof(buf), "%s %s (%+d)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), item->getName(), item->beatitude);
	}

	auto activateSelectionPrompt = alchFrame->findField("activate prompt");
	activateSelectionPrompt->setText("");

	int player = parentGUI.getPlayer();
	bool isSameResult = false;
	bool isDuplicationResult = false;
	if ( itemCategory(item) == POTION && item->type != POTION_EMPTY )
	{
		bool isEquipped = itemIsEquipped(item, player);
		if ( (!item->identified || isEquipped) && !isTooltipForResultPotion && !isTooltipForRecipe )
		{
			itemActionType = ALCHEMY_ACTION_UNIDENTIFIED_POTION;
		}
		else
		{
			itemActionType = ALCHEMY_ACTION_OK;
		}
		auto find = clientLearnedAlchemyIngredients[player].find(item->type);
		Item* basePotion = nullptr;
		Item* secondaryPotion = nullptr;
		bool isRandomResult = false;
		bool recipeMissingMaterials = false;
		if ( isTooltipForRecipe )
		{
			recipeMissingMaterials = true;
			itemActionType = ALCHEMY_ACTION_UNIDENTIFIED_POTION;
			for ( auto& entry : recipes.recipeList )
			{
				if ( &entry.resultItem == item )
				{
					if ( entry.basePotionUid != 0 && entry.secondaryPotionUid != 0 )
					{
						recipeMissingMaterials = false;
						itemActionType = ALCHEMY_ACTION_OK;
					}
					break;
				}
			}
		}
		else if ( isTooltipForResultPotion )
		{
			basePotion = uidToItem(potion1Uid);
			secondaryPotion = uidToItem(potion2Uid);
			if ( basePotion && secondaryPotion )
			{
				if ( basePotion->identified && secondaryPotion->identified 
					&& basePotion->type == secondaryPotion->type )
				{
					isSameResult = true;
				}
				else if ( (basePotion->identified && basePotion->type == POTION_WATER
					&& secondaryPotion->identified && secondaryPotion->type != POTION_WATER)
					|| (basePotion->identified && basePotion->type != POTION_WATER
						&& secondaryPotion->identified && secondaryPotion->type == POTION_WATER) )
				{
					isDuplicationResult = true;
				}
				else if ( basePotion->identified && basePotion->type == POTION_POLYMORPH
					|| secondaryPotion->identified && secondaryPotion->type == POTION_POLYMORPH )
				{
					isRandomResult = true;
				}
			}
		}
		if ( recipeMissingMaterials )
		{
			strcat(buf, "\n");
			strcat(buf, language[4168]);
		}
		else if ( isEquipped )
		{
			strcat(buf, "\n");
			strcat(buf, language[4165]);
		}
		else if ( isSameResult )
		{
			strcat(buf, "\n");
			strcat(buf, language[4166]);
		}
		else if ( isRandomResult )
		{
			strcat(buf, "\n");
			strcat(buf, language[4164]);
		}
		else if ( isDuplicationResult )
		{
			int skillLVL = 0;
			if ( stats[parentGUI.getPlayer()] )
			{
				skillLVL = stats[parentGUI.getPlayer()]->PROFICIENCIES[PRO_ALCHEMY] / 20; // 0 to 5;
			}
			snprintf(buf, sizeof(buf), "%s\n%d%%", language[4163], 50 + skillLVL * 10);
		}
		else if ( item->identified && find != clientLearnedAlchemyIngredients[player].end() )
		{
			if ( parentGUI.isItemBaseIngredient(item->type) )
			{
				strcat(buf, "\n");
				strcat(buf, language[4156]);
			}
			else if ( parentGUI.isItemSecondaryIngredient(item->type) )
			{
				strcat(buf, "\n");
				strcat(buf, language[4157]);
			}
			else
			{
				strcat(buf, "\n");
				strcat(buf, language[4158]);
			}
		}
		else
		{
			if ( !item->identified )
			{
				if ( isTooltipForResultPotion )
				{
					strcat(buf, "\n");
					strcat(buf, language[4162]);
				}
				else
				{
					strcat(buf, "\n");
					strcat(buf, language[4160]);
				}
			}
			else
			{
				strcat(buf, "\n");
				strcat(buf, language[4155]);
			}
		}
	}
	else
	{
		if ( item->type == TOOL_BOMB && isTooltipForResultPotion )
		{
			itemActionType = ALCHEMY_ACTION_OK; // this is fine :)
		}
		else
		{
			itemActionType = ALCHEMY_ACTION_INVALID_ITEM;
		}
	}
	if ( itemDesc != buf )
	{
		itemRequiresTitleReflow = true;
	}
	itemDesc = buf;
	itemType = item->type;
	if ( itemActionType == ALCHEMY_ACTION_OK )
	{
		if ( isTooltipForRecipe )
		{
			int index = 0;
			activateSelectionPrompt->setText(language[4174]);
			if ( recipes.activateRecipeIndex >= 0 )
			{
				/*if ( !inputs.getVirtualMouse(player)->draw_cursor )
				{
					for ( auto& entry : recipes.recipeList )
					{
						if ( &entry.resultItem == item )
						{
							if ( recipes.activateRecipeIndex == index )
							{
								activateSelectionPrompt->setText(language[4175]);
							}
							break;
						}
						++index;
					}
				}
				else*/
				{
					activateSelectionPrompt->setText("");
				}
			}
			else
			{
				activateSelectionPrompt->setText(language[4174]);
			}
		}
		else if ( !isTooltipForResultPotion )
		{
			if ( item->uid == potion1Uid || item->uid == potion2Uid )
			{
				activateSelectionPrompt->setText(language[4173]);
			}
			else
			{
				activateSelectionPrompt->setText(language[4172]);
			}
		}
		else if ( isTooltipForResultPotion )
		{
			bool usingGamepad = inputs.hasController(player) && !inputs.getVirtualMouse(player)->draw_cursor;
			if ( !usingGamepad )
			{
				if ( isSameResult )
				{
					activateSelectionPrompt->setText(language[4177]);
				}
				else if ( isDuplicationResult )
				{
					activateSelectionPrompt->setText(language[4178]);
				}
				else
				{
					activateSelectionPrompt->setText(language[4176]);
				}
			}
		}
	}
	return;
}

bool GenericGUIMenu::AlchemyGUI_t::warpMouseToSelectedAlchemyItem(Item* snapToItem, Uint32 flags)
{
	if ( alchemyGUIHasBeenCreated() )
	{
		int x = getSelectedAlchemySlotX();
		int y = getSelectedAlchemySlotY();
		if ( snapToItem )
		{
			x = snapToItem->x;
			y = snapToItem->y;
		}

		if ( auto slot = getAlchemySlotFrame(x, y) )
		{
			int playernum = parentGUI.getPlayer();
			auto player = players[playernum];
			if ( !isInteractable )
			{
				//messagePlayer(0, "[Debug]: select item queued");
				player->inventoryUI.cursor.queuedModule = Player::GUI_t::MODULE_ALCHEMY;
				player->inventoryUI.cursor.queuedFrameToWarpTo = slot;
				return false;
			}
			else
			{
				//messagePlayer(0, "[Debug]: select item warped");
				player->inventoryUI.cursor.queuedModule = Player::GUI_t::MODULE_NONE;
				player->inventoryUI.cursor.queuedFrameToWarpTo = nullptr;
				slot->warpMouseToFrame(playernum, flags);
			}
			return true;
		}
	}
	return false;
}

void GenericGUIMenu::AlchemyGUI_t::clearItemDisplayed()
{
	itemType = -1;
	itemActionType = ALCHEMY_ACTION_NONE;
	itemTooltipForRecipe = false;
}

int GenericGUIMenu::AlchemyGUI_t::AlchemyRecipes_t::getNumRecipesToDisplayVertical() const
{
	return kNumRecipesToDisplayVertical;
}
void GenericGUIMenu::AlchemyGUI_t::AlchemyRecipes_t::openRecipePanel()
{
	if ( !alchemy.recipesFrame )
	{
		return;
	}
	alchemy.currentView = ALCHEMY_VIEW_RECIPES;
	bool wasDisabled = alchemy.recipesFrame->isDisabled();
	alchemy.recipesFrame->setDisabled(false);
	if ( wasDisabled )
	{
		animx = 0.0;
		isInteractable = false;
		currentScrollRow = 0;
		scrollPercent = 0.0;
		scrollInertia = 0.0;
		bFirstTimeSnapCursor = false;
	}
	alchemy.animRecipeAutoAddToSlot1Uid = 0;
	alchemy.animRecipeAutoAddToSlot2Uid = 0;
	activateRecipeIndex = -1;
	bOpen = true;
}
void GenericGUIMenu::AlchemyGUI_t::AlchemyRecipes_t::closeRecipePanel()
{
	if ( alchemy.recipesFrame )
	{
		alchemy.recipesFrame->setDisabled(true);
	}
	alchemy.currentView = ALCHEMY_VIEW_BREW;
	animx = 0.0;
	isInteractable = false;
	currentScrollRow = 0;
	scrollPercent = 0.0;
	scrollInertia = 0.0;
	scrollAnimateX = scrollSetpoint;
	bool wasOpen = bOpen;
	bOpen = false;
	bFirstTimeSnapCursor = false;
	recipeList.clear();
	activateRecipeIndex = -1;
	alchemy.animRecipeAutoAddToSlot1Uid = 0;
	alchemy.animRecipeAutoAddToSlot2Uid = 0;
	panelJustifyInverted = false;
	if ( wasOpen && alchemy.getSelectedAlchemySlotX() >= 0 )
	{
		alchemy.selectAlchemySlot(alchemy.ALCH_SLOT_BASE_POTION_X, 0);
		if ( alchemy.bOpen )
		{
			alchemy.warpMouseToSelectedAlchemyItem(nullptr, (Inputs::SET_CONTROLLER));
		}
	}
}

void buildRecipeList(const int player)
{
	auto& alchemy = GenericGUI[player].alchemyGUI;
	auto& recipes = alchemy.recipes;

	int entryx = 0;
	int entryy = 0;

	auto oldSize = recipes.recipeList.size();
	recipes.recipeList.clear();

	std::unordered_map<int, std::pair<std::vector<Item*>, int>> inventoryPotions;
	bool hasEmptyBottle = false;
	for ( node_t* node = stats[player]->inventory.first; node; node = node->next )
	{
		Item* item = (Item*)node->element;
		if ( !item ) { continue; }
		if ( itemCategory(item) == POTION )
		{
			if ( item->type == POTION_EMPTY )
			{
				hasEmptyBottle = true;
			}
			if ( !item->identified )
			{
				continue;
			}
			if ( inventoryPotions.find(item->type) == inventoryPotions.end() )
			{
				inventoryPotions[item->type].second = 0;
			}
			if ( !itemIsEquipped(item, player) )
			{
				inventoryPotions[item->type].first.push_back(item);
			}
			inventoryPotions[item->type].second += item->count;
		}
	}

	alchemy.emptyBottleCount.count = hasEmptyBottle ? inventoryPotions[POTION_EMPTY].second : 0;

	for ( auto& entry : clientLearnedAlchemyRecipes[player] )
	{
		bool hideRecipe = hideRecipeFromList(entry.first);
		Item* basePotion = nullptr;
		Item* secondaryPotion = nullptr;
		if ( !hideRecipe && inventoryPotions.find(entry.second.first) != inventoryPotions.end() )
		{
			for ( auto& item : inventoryPotions[entry.second.first].first )
			{
				if ( !basePotion )
				{
					basePotion = item;
					continue;
				}
				if ( basePotion->beatitude == 0 && item->beatitude != 0 )
				{
					continue; // prefer unblessed, skip
				}
				if ( basePotion->beatitude != 0 && item->beatitude == 0 )
				{
					basePotion = item; // if we're blessed, then take unblessed
					continue;
				}
				if ( basePotion->count > item->count )
				{
					basePotion = item; // if we're greater count, then take lower count
					continue;
				}
				if ( basePotion->count < item->count )
				{
					continue; // ignore higher count items
				}
				if ( basePotion->status > item->status )
				{
					basePotion = item; // take lower status items
					continue;
				}
				if ( basePotion->beatitude != 0 && item->beatitude != 0 )
				{
					if ( basePotion->beatitude > 0 && item->beatitude < 0 )
					{
						continue; // prefer blessed, skip
					}
					if ( basePotion->beatitude < 0 && item->beatitude > 0 )
					{
						basePotion = item; // prefer blessed, take this one
						continue;
					}
					if ( basePotion->beatitude > 0 )
					{
						if ( basePotion->beatitude > item->beatitude )
						{
							basePotion = item; // prefer +1 instead of +2
							continue;
						}
					}
					else if ( basePotion->beatitude < 0 )
					{
						if ( basePotion->beatitude < item->beatitude )
						{
							basePotion = item; // prefer -1 instead of -2
							continue;
						}
					}
				}
			}
		}

		if ( !hideRecipe && inventoryPotions.find(entry.second.second) != inventoryPotions.end() )
		{
			for ( auto& item : inventoryPotions[entry.second.second].first )
			{
				if ( !secondaryPotion )
				{
					secondaryPotion = item;
					continue;
				}
				if ( secondaryPotion->beatitude == 0 && item->beatitude != 0 )
				{
					continue; // prefer unblessed, skip
				}
				if ( secondaryPotion->beatitude != 0 && item->beatitude == 0 )
				{
					secondaryPotion = item; // if we're blessed, then take unblessed
					continue;
				}
				if ( secondaryPotion->count > item->count )
				{
					secondaryPotion = item; // if we're greater count, then take lower count
					continue;
				}
				if ( secondaryPotion->count < item->count )
				{
					continue; // ignore higher count items
				}
				if ( secondaryPotion->status > item->status )
				{
					secondaryPotion = item; // take lower status items
					continue;
				}
				if ( secondaryPotion->beatitude != 0 && item->beatitude != 0 )
				{
					if ( secondaryPotion->beatitude > 0 && item->beatitude < 0 )
					{
						continue; // prefer blessed, skip
					}
					if ( secondaryPotion->beatitude < 0 && item->beatitude > 0 )
					{
						secondaryPotion = item; // prefer blessed, take this one
						continue;
					}
					if ( secondaryPotion->beatitude > 0 )
					{
						if ( secondaryPotion->beatitude > item->beatitude )
						{
							secondaryPotion = item; // prefer +1 instead of +2
							continue;
						}
					}
					else if ( secondaryPotion->beatitude < 0 )
					{
						if ( secondaryPotion->beatitude < item->beatitude )
						{
							secondaryPotion = item; // prefer -1 instead of -2
							continue;
						}
					}
				}
			}
		}

		if ( !hideRecipe )
		{
			recipes.recipeList.push_back(GenericGUIMenu::AlchemyGUI_t::AlchemyRecipes_t::RecipeEntry_t());
			auto& recipe = recipes.recipeList.at(recipes.recipeList.size() - 1);
			recipe.resultItem.type = (ItemType)entry.first;
			recipe.resultItem.count = 0;
			if ( inventoryPotions.find(recipe.resultItem.type) != inventoryPotions.end() )
			{
				recipe.resultItem.count += inventoryPotions[recipe.resultItem.type].second;
			}
			recipe.resultItem.identified = true;
			recipe.resultItem.status = EXCELLENT;
			recipe.resultItem.appearance = 0;
			for ( auto it = potionStandardAppearanceMap.begin(); it != potionStandardAppearanceMap.end(); ++it )
			{
				if ( (*it).first == recipe.resultItem.type )
				{
					recipe.resultItem.appearance = (*it).second;
					break;
				}
			}
			recipe.basePotionUid = 0;
			recipe.dummyPotion1.count = 1;
			recipe.dummyPotion1.beatitude = 0;
			recipe.dummyPotion1.identified = true;
			if ( basePotion )
			{
				recipe.basePotionUid = basePotion->uid;
				copyItem(&recipe.dummyPotion1, basePotion);
			}
			else
			{
				recipe.dummyPotion1.type = (ItemType)entry.second.first;
				for ( auto it = potionStandardAppearanceMap.begin(); it != potionStandardAppearanceMap.end(); ++it )
				{
					if ( (*it).first == recipe.dummyPotion1.type )
					{
						recipe.dummyPotion1.appearance = (*it).second;
						break;
					}
				}
				recipe.dummyPotion1.status = BROKEN;
			}
			recipe.dummyPotion1.uid = 0;
			recipe.dummyPotion1.x = 0;
			recipe.dummyPotion1.y = 0;

			recipe.secondaryPotionUid = 0;
			recipe.dummyPotion2.count = 1;
			recipe.dummyPotion2.beatitude = 0;
			recipe.dummyPotion2.identified = true;
			if ( secondaryPotion )
			{
				recipe.secondaryPotionUid = secondaryPotion->uid;
				copyItem(&recipe.dummyPotion2, secondaryPotion);
			}
			else
			{
				recipe.dummyPotion2.type = (ItemType)entry.second.second;
				for ( auto it = potionStandardAppearanceMap.begin(); it != potionStandardAppearanceMap.end(); ++it )
				{
					if ( (*it).first == recipe.dummyPotion2.type )
					{
						recipe.dummyPotion2.appearance = (*it).second;
						break;
					}
				}
				recipe.dummyPotion2.status = BROKEN;
			}
			recipe.dummyPotion2.uid = 0;
			recipe.dummyPotion2.x = 0;
			recipe.dummyPotion2.y = 0;
			if ( !hideRecipeFromList(recipe.resultItem.type) )
			{
				recipe.x = entryx;
				recipe.y = entryy;
				++entryx;
				if ( entryx >= alchemy.MAX_ALCH_X )
				{
					entryx = 0;
					++entryy;
				}
			}
			else
			{
				recipe.x = entryx;
				recipe.y = entryy;
			}
		}
	}
	if ( oldSize != recipes.recipeList.size() )
	{
		recipes.activateRecipeIndex = -1;
	}
}

void GenericGUIMenu::AlchemyGUI_t::AlchemyRecipes_t::updateRecipePanel()
{
	Frame* recipeFrame = alchemy.recipesFrame;
	if ( !recipeFrame ) { return; }

	const int player = alchemy.parentGUI.getPlayer();
	const int slotSize = players[player]->inventoryUI.getSlotSize();
	if ( !recipeFrame->isDisabled() && bOpen )
	{
		const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
		real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animx)) / 2.0;
		animx += setpointDiffX;
		animx = std::min(1.0, animx);
		if ( animx >= .9999 )
		{
			if ( !bFirstTimeSnapCursor )
			{
				bFirstTimeSnapCursor = true;
				if ( !inputs.getUIInteraction(player)->selectedItem
					&& players[player]->GUI.activeModule == Player::GUI_t::MODULE_ALCHEMY )
				{
					//player.inventoryUI.warpMouseToSelectedSpell(nullptr, (Inputs::SET_CONTROLLER));
				}
			}
			isInteractable = true;
		}
	}
	else
	{
		animx = 0.0;
		isInteractable = false;
		scrollInertia = 0.0;
	}

	auto baseFrame = recipeFrame->findFrame("recipe base");
	auto bacBackgroundImgFrame = recipeFrame->findFrame("recipe img frame");
	auto baseBackgroundImg = bacBackgroundImgFrame->findImage("recipe base img");
	baseBackgroundImg->path = "*#images/ui/Alchemy/Alchemy_Recipes_00.png";
	bool reversed = false;
	auto recipeFramePos = recipeFrame->getSize();
	static ConsoleVariable<int> cvar_alchemy_recipeOffsetX("/alch_recipe_offsetx", 8);
	static ConsoleVariable<int> cvar_alchemy_recipeOffsetY("/alch_recipe_offsety", 14);
	const int frameOffsetX = *cvar_alchemy_recipeOffsetX;
	const int frameOffsetY = *cvar_alchemy_recipeOffsetY;
	if ( players[player]->inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_LEFT )
	{
		auto baseFrame = alchemy.alchFrame->findFrame("alchemy base");
		if ( players[player]->bUseCompactGUIWidth() )
		{
			recipeFramePos.x = baseFrame->getSize().x - animx * recipeFramePos.w + frameOffsetX;
			baseBackgroundImg->path = "*#images/ui/Alchemy/Alchemy_RecipesReverse_00.png";
			reversed = true;
		}
		else
		{
			recipeFramePos.x = baseFrame->getSize().x + baseFrame->getSize().w + animx * recipeFramePos.w - recipeFramePos.w - frameOffsetX;
		}
	}
	else
	{
		auto baseFrame = alchemy.alchFrame->findFrame("alchemy base");
		if ( players[player]->bUseCompactGUIWidth() )
		{
			recipeFramePos.x = baseFrame->getSize().x + baseFrame->getSize().w + animx * recipeFramePos.w - recipeFramePos.w - frameOffsetX;
		}
		else
		{
			recipeFramePos.x = baseFrame->getSize().x - animx * recipeFramePos.w + frameOffsetX;
			baseBackgroundImg->path = "*#images/ui/Alchemy/Alchemy_RecipesReverse_00.png";
			reversed = true;
		}
	}
	panelJustifyInverted = reversed;
	recipeFramePos.y = frameOffsetY;
	recipeFrame->setSize(recipeFramePos);

	auto slider = baseFrame->findSlider("recipe slider");
	auto recipeSlotsFrame = recipeFrame->findFrame("alchemy slots");
	auto clearRecipeBtn = baseFrame->findButton("clear recipe");
	auto clearRecipeGlyph = baseFrame->findImage("clear recipe glyph");
	// handle height changing..
	{
		int frameHeight = kRecipeListHeight;
		int totalFrameHeightChange = kRecipeHeaderHeight;
		recipeFramePos.h = frameHeight + totalFrameHeightChange + kRecipeFooterHeight;
		recipeFrame->setSize(recipeFramePos);

		auto baseBackgroundImgPos = bacBackgroundImgFrame->getSize();
		baseBackgroundImgPos.y = 0;
		bacBackgroundImgFrame->setSize(baseBackgroundImgPos);

		SDL_Rect recipeBasePos = baseFrame->getSize();
		recipeBasePos.h = recipeFramePos.h;
		baseFrame->setSize(recipeBasePos);

		int numGrids = (MAX_ALCH_Y / getNumRecipesToDisplayVertical()) + 1;
		auto gridImg = recipeSlotsFrame->findImage("grid img");

		SDL_Rect recipeSlotsFramePos = recipeSlotsFrame->getSize();

		int heightChange = 0;
		if ( getNumRecipesToDisplayVertical() < kNumRecipesToDisplayVertical )
		{
			heightChange = slotSize * (kNumRecipesToDisplayVertical - getNumRecipesToDisplayVertical());
		}
		recipeSlotsFramePos.y = 4 + heightChange + totalFrameHeightChange;
		recipeSlotsFramePos.x = 8 + (reversed ? 18 : 0);
		recipeSlotsFramePos.h = (kRecipeGridImgHeight + 2) - heightChange;

		SDL_Rect recipeSlotsFrameActualPos { recipeSlotsFrame->getActualSize().x,
			recipeSlotsFrame->getActualSize().y,
			recipeSlotsFrame->getActualSize().w,
			(recipeSlotsFramePos.h) * numGrids };
		recipeSlotsFrame->setActualSize(recipeSlotsFrameActualPos);
		recipeSlotsFrame->setScrollBarsEnabled(false);
		recipeSlotsFrame->setSize(recipeSlotsFramePos);
		gridImg->pos.y = kRecipeListGridY;
		gridImg->pos.h = (recipeSlotsFramePos.h) * numGrids;

		clearRecipeBtn->setPos(recipeFramePos.w / 2 - clearRecipeBtn->getSize().w / 2,
			recipeSlotsFramePos.y + recipeSlotsFramePos.h + 2);

		SDL_Rect sliderPos = slider->getRailSize();
		sliderPos.y = 8 + heightChange + totalFrameHeightChange;
		sliderPos.h = (kRecipeListHeight - 2 * 8);
		slider->setRailSize(sliderPos);
	}

	int lowestItemY = getNumRecipesToDisplayVertical() - 1;
	int entryx = 0;
	int entryy = 0;

	buildRecipeList(player);

	auto title1 = baseFrame->findField("recipe title 1");
	char titleBuf[64] = "";
	snprintf(titleBuf, sizeof(titleBuf), language[4181], recipeList.size());
	title1->setText(titleBuf);
	title1->setTextColor(hudColors.characterSheetLightNeutral);
	title1->setOutlineColor(makeColor(29, 16, 11, 255));
	if ( auto textGet = title1->getTextObject() )
	{
		SDL_Rect pos = title1->getSize();
		pos.w = textGet->getWidth();
		pos.x = baseBackgroundImg->pos.w / 2 - pos.w / 2;
		if ( pos.x % 2 == 1 )
		{
			++pos.x;
		}
		pos.y = 18;
		title1->setSize(pos);
		title1->setDisabled(false);
	}

	if ( !recipeList.empty() && false ) // disable scrolling
	{
		for ( auto it = recipeList.rbegin(); it != recipeList.rend(); ++it )
		{
			if ( !hideRecipeFromList((*it).resultItem.type) )
			{
				lowestItemY = std::max(lowestItemY, (*it).y);
				break;
			}
		}
	}
	int index = 0;

	auto cursor = recipeSlotsFrame->findFrame("active recipe");
	cursor->setDisabled(true);

	for ( int x = 0; x < MAX_ALCH_X; ++x )
	{
		for ( int y = 0; y < MAX_ALCH_Y; ++y )
		{
			Frame::image_t* stone = nullptr;
			if ( stones.find(x + 100 * y) == stones.end() )
			{
				char stoneImgName[32] = "";
				snprintf(stoneImgName, sizeof(stoneImgName), "stone %d %d", x, y);
				stone = recipeSlotsFrame->addImage(SDL_Rect{ 0, 0, 38, 40 },
					0xFFFFFFFF, "images/ui/Alchemy/Alchemy_Icon_RecipeTileBG_00.png", stoneImgName);
				stones[x + 100 * y] = stone;
			}
			else
			{
				stone = stones[x + 100 * y];
			}
			stone->ontop = true;
			stone->disabled = false;
			stone->pos.x = (x * slotSize) + 2;
			stone->pos.y = (y * slotSize);
		}
	}

	for ( auto& entry : recipeList )
	{
		if ( hideRecipeFromList(entry.resultItem.type) )
		{
			++index;
			continue;
		}
		if ( auto slotFrame = alchemy.getAlchemySlotFrame(entry.x, entry.y) )
		{
			slotFrame->setDisabled(true);
			slotFrame->setUserData(&GAMEUI_FRAMEDATA_ALCHEMY_RECIPE_ENTRY);
			if ( entry.basePotionUid > 0 && entry.secondaryPotionUid > 0 )
			{
				if ( uidToItem(entry.basePotionUid) && uidToItem(entry.secondaryPotionUid) )
				{
					updateSlotFrameFromItem(slotFrame, &entry.resultItem);
				}
			}
			else
			{
				if ( index == activateRecipeIndex )
				{
					activateRecipeIndex = -1; // no potions on hand
				}
			}
			if ( slotFrame->isDisabled() )
			{
				updateSlotFrameFromItem(slotFrame, &entry.resultItem, true); // dim result, no potions on hand
			}
			if ( stones.find(entry.x + 100 * entry.y) != stones.end() )
			{
				stones[entry.x + 100 * entry.y]->disabled = true;
			}
			if ( index == activateRecipeIndex )
			{
				cursor->setDisabled(false);
				SDL_Rect cursorPos = slotFrame->getSize();
				cursorPos.x += 1;
				cursorPos.y += 1;
				const int cursorOffset = players[player]->inventoryUI.cursor.cursorToSlotOffset;
				cursorPos.x -= cursorOffset;
				cursorPos.y -= cursorOffset;
				cursorPos.w += cursorOffset * 2;
				cursorPos.h += cursorOffset * 2;
				cursor->setSize(cursorPos);
				const int offset = 8;
				if ( auto tl = cursor->findImage("cursor topleft") )
				{
					tl->pos = SDL_Rect{ offset, offset, tl->pos.w, tl->pos.h };
				}
				if ( auto tr = cursor->findImage("cursor topright") )
				{
					tr->pos = SDL_Rect{ -offset + cursorPos.w - tr->pos.w, offset, tr->pos.w, tr->pos.h };
				}
				if ( auto bl = cursor->findImage("cursor bottomleft") )
				{
					bl->pos = SDL_Rect{ offset, -offset + cursorPos.h - bl->pos.h, bl->pos.w, bl->pos.h };
				}
				if ( auto br = cursor->findImage("cursor bottomright") )
				{
					br->pos = SDL_Rect{ -offset + cursorPos.w - br->pos.w, -offset + cursorPos.h - br->pos.h, br->pos.w, br->pos.h };
				}
			}
		}
		++index;
	}

	int scrollAmount = std::max((lowestItemY + 1) - (getNumRecipesToDisplayVertical()), 0) * slotSize;
	if ( scrollAmount == 0 )
	{
		slider->setDisabled(true);
	}
	else
	{
		//slider->setDisabled(false);
	}

	currentScrollRow = scrollSetpoint / slotSize;

	if ( bOpen && isInteractable )
	{
		// do sliders
		if ( !slider->isDisabled() )
		{
			if ( !inputs.getUIInteraction(player)->selectedItem
				&& players[player]->GUI.activeModule == Player::GUI_t::MODULE_ALCHEMY )
			{
				if ( inputs.bPlayerUsingKeyboardControl(player) )
				{
					if ( Input::mouseButtons[Input::MOUSE_WHEEL_DOWN] )
					{
						Input::mouseButtons[Input::MOUSE_WHEEL_DOWN] = 0;
						scrollSetpoint = std::max(scrollSetpoint + slotSize, 0);
					}
					if ( Input::mouseButtons[Input::MOUSE_WHEEL_UP] )
					{
						Input::mouseButtons[Input::MOUSE_WHEEL_UP] = 0;
						scrollSetpoint = std::max(scrollSetpoint - slotSize, 0);
					}
				}
				if ( Input::inputs[player].analogToggle("MenuScrollDown") )
				{
					Input::inputs[player].consumeAnalogToggle("MenuScrollDown");
					scrollSetpoint = std::max(scrollSetpoint + slotSize, 0);
				}
				else if ( Input::inputs[player].analogToggle("MenuScrollUp") )
				{
					Input::inputs[player].consumeAnalogToggle("MenuScrollUp");
					scrollSetpoint = std::max(scrollSetpoint - slotSize, 0);
				}
			}
		}

		scrollSetpoint = std::min(scrollSetpoint, scrollAmount);
		currentScrollRow = scrollSetpoint / slotSize;

		if ( abs(scrollSetpoint - scrollAnimateX) > 0.00001 )
		{
			isInteractable = false;
			const real_t fpsScale = (60.f / std::max(1U, fpsLimit));
			real_t setpointDiff = 0.0;
			if ( scrollSetpoint - scrollAnimateX > 0.0 )
			{
				setpointDiff = fpsScale * std::max(3.0, (scrollSetpoint - scrollAnimateX)) / 3.0;
			}
			else
			{
				setpointDiff = fpsScale * std::min(-3.0, (scrollSetpoint - scrollAnimateX)) / 3.0;
			}
			scrollAnimateX += setpointDiff;
			if ( setpointDiff > 0.0 )
			{
				scrollAnimateX = std::min((real_t)scrollSetpoint, scrollAnimateX);
			}
			else
			{
				scrollAnimateX = std::max((real_t)scrollSetpoint, scrollAnimateX);
			}
		}
		else
		{
			scrollAnimateX = scrollSetpoint;
		}
	}

	if ( scrollAmount > 0 )
	{
		slider->setValue((scrollAnimateX / scrollAmount) * 100.0);
	}
	else
	{
		slider->setValue(0.0);
	}

	SDL_Rect actualSize = recipeSlotsFrame->getActualSize();
	actualSize.y = scrollAnimateX;
	recipeSlotsFrame->setActualSize(actualSize);

	bool usingGamepad = inputs.hasController(player) && !inputs.getVirtualMouse(player)->draw_cursor;

	clearRecipeBtn->setDisabled(true);
	clearRecipeBtn->setInvisible(true);
	clearRecipeGlyph->disabled = true;
	if ( activateRecipeIndex >= 0 && bOpen )
	{
		clearRecipeBtn->setInvisible(false);

		if ( inputs.getVirtualMouse(player)->draw_cursor )
		{
			clearRecipeBtn->setDisabled(!isInteractable);
			if ( isInteractable )
			{
				buttonAlchemyUpdateSelectorOnHighlight(player, clearRecipeBtn);
			}
		}
		else if ( clearRecipeBtn->isSelected() )
		{
			clearRecipeBtn->deselect();
		}
		if ( clearRecipeBtn->isDisabled() && usingGamepad )
		{
			clearRecipeGlyph->path = Input::inputs[player].getGlyphPathForBinding("MenuCancel");
			if ( auto imgGet = Image::get(clearRecipeGlyph->path.c_str()) )
			{
				clearRecipeGlyph->pos.w = imgGet->getWidth();
				clearRecipeGlyph->pos.h = imgGet->getHeight();
				clearRecipeGlyph->disabled = false;
			}
			clearRecipeGlyph->pos.x = clearRecipeBtn->getSize().x + clearRecipeBtn->getSize().w - 16;
			if ( clearRecipeGlyph->pos.x % 2 == 1 )
			{
				++clearRecipeGlyph->pos.x;
			}
			clearRecipeGlyph->pos.y = clearRecipeBtn->getSize().y + clearRecipeBtn->getSize().h / 2 - clearRecipeGlyph->pos.w / 2;
			if ( clearRecipeGlyph->pos.y % 2 == 1 )
			{
				++clearRecipeGlyph->pos.y;
			}
		}
	}
}

void GenericGUIMenu::AlchemyGUI_t::AlchemyRecipes_t::scrollToSlot(int x, int y, bool instantly)
{
	int lowerY = currentScrollRow;
	int upperY = currentScrollRow + getNumRecipesToDisplayVertical() - 1;

	if ( y >= lowerY && y <= upperY )
	{
		// no work to do.
		return;
	}
	int player = alchemy.parentGUI.getPlayer();
	int lowestItemY = getNumRecipesToDisplayVertical() - 1;
	const int slotSize = players[player]->inventoryUI.getSlotSize();
	if ( !recipeList.empty() )
	{
		for ( auto it = recipeList.rbegin(); it != recipeList.rend(); ++it )
		{
			if ( !hideRecipeFromList((*it).resultItem.type) )
			{
				lowestItemY = std::max(lowestItemY, (*it).y);
				break;
			}
		}
	}
	int maxScroll = std::max((lowestItemY + 1) - (getNumRecipesToDisplayVertical()), 0) * slotSize;

	int scrollAmount = 0;
	if ( y < lowerY )
	{
		scrollAmount = (y) * slotSize;
		//scrollAmount += scrollSetpoint;
	}
	else if ( y > upperY )
	{
		scrollAmount = (y - upperY) * slotSize;
		scrollAmount += scrollSetpoint;
	}
	scrollAmount = std::min(scrollAmount, maxScroll);

	scrollSetpoint = scrollAmount;
	if ( instantly )
	{
		scrollAnimateX = scrollSetpoint;
	}
	currentScrollRow = scrollSetpoint / slotSize;
	if ( abs(scrollSetpoint - scrollAnimateX) > 0.00001 )
	{
		isInteractable = false;
	}
}

bool GenericGUIMenu::AlchemyGUI_t::AlchemyRecipes_t::isSlotVisible(int x, int y) const
{
	if ( alchemy.recipesFrame )
	{
		if ( alchemy.recipesFrame->isDisabled() )
		{
			return false;
		}
	}
	int lowerY = currentScrollRow;
	int upperY = currentScrollRow + getNumRecipesToDisplayVertical() - 1;

	if ( y >= lowerY && y <= upperY )
	{
		return true;
	}
	return false;
}

bool GenericGUIMenu::AlchemyGUI_t::AlchemyRecipes_t::isItemVisible(Item* item) const
{
	if ( !item ) { return false; }
	return isSlotVisible(item->x, item->y);
}