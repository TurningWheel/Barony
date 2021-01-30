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
#include "../scores.hpp"
#include "../scrolls.hpp"
#include "../lobbies.hpp"

Uint32 svFlags = 30;
Uint32 settings_svFlags = svFlags;
SDL_Surface* backdrop_minotaur_bmp = nullptr;
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
Item* invitemschest[MAXPLAYERS][kNumChestItemsToDisplay];
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
int chestitemscroll[MAXPLAYERS] = { 0 };
int chestgui_offset_x[MAXPLAYERS] = { 0 };
int chestgui_offset_y[MAXPLAYERS] = { 0 };
bool dragging_chestGUI[MAXPLAYERS] = { 0 };
int selectedChestSlot[MAXPLAYERS];

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
bool auto_appraise_new_items = false;
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

EnemyHPDamageBarHandler enemyHPDamageBarHandler[MAXPLAYERS];
FollowerRadialMenu FollowerMenu[MAXPLAYERS];
GenericGUIMenu GenericGUI[MAXPLAYERS];

std::vector<std::pair<SDL_Surface**, std::string>> systemResourceImages =
{
	std::make_pair(&title_bmp, "images/system/title.png"),
	std::make_pair(&logo_bmp, "images/system/logo.png"),
	std::make_pair(&cursor_bmp, "images/system/cursor.png"),
	std::make_pair(&cross_bmp, "images/system/cross.png"),
	std::make_pair(&selected_cursor_bmp, "images/system/selectedcursor.png"),
	std::make_pair(&controllerglyphs1_bmp, "images/system/glyphsheet_ns.png"),
	std::make_pair(&skillIcons_bmp, "images/system/skillicons_sheet.png"),

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
	impulses[IN_HOTBAR_SCROLL_LEFT] = 286;
	impulses[IN_HOTBAR_SCROLL_RIGHT] = 287;
	impulses[IN_HOTBAR_SCROLL_SELECT] = 284;

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
	consoleCommand("/sfxambientvolume 64");
	consoleCommand("/sfxenvironmentvolume 64");
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
	consoleCommand("/bind 286 IN_HOTBAR_SCROLL_LEFT");
	consoleCommand("/bind 287 IN_HOTBAR_SCROLL_RIGHT");
	consoleCommand("/bind 284 IN_HOTBAR_SCROLL_SELECT");

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
#ifdef NINTENDO
	nxDefaultConfig();
#endif
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
		printlog("Legacy keys detected, conflict with IN_FOLLOWERMENU_CYCLENEXT. Automatically rebound IN_UP: %d (Right arrow key)\n", impulses[IN_UP]);
	}
	if ( impulses[IN_FOLLOWERMENU_LASTCMD] == impulses[IN_TURNL]
		&& impulses[IN_TURNL] == 20 )
	{
		// reset to default arrow key to avoid overlapping keybinds on first launch.
		// due to legacy keybind, now we have useful things to assign to q,e,z,c
		impulses[IN_TURNL] = 80;
		printlog("Legacy keys detected, conflict with IN_FOLLOWERMENU_CYCLENEXT. Automatically rebound IN_TURNL: %d (Right arrow key)\n", impulses[IN_TURNL]);
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

int saveConfig(char const * const _filename)
{
	char path[PATH_MAX];
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	File* fp;
	int c;
	char *filename = strdup(_filename);

	printlog("Saving config '%s'...\n", filename);

	if ( strstr(filename, ".cfg") == NULL )
	{
		filename = (char*) realloc(filename, sizeof(char) * (strlen(filename) + 5));
		strcat(filename, ".cfg");
	}

	completePath(path, filename, outputdir);

	// open the config file
	if ( (fp = FileIO::open(path, "wb")) == NULL )
	{
		printlog("ERROR: failed to save config file '%s'!\n", filename);
		free(filename);
		return 1;
	}

	// write config header
	fp->printf("# %s\n", filename);
	fp->printf("# this file was auto-generated on %d-%02d-%02d at %02d:%02d:%02d\n\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	// write contents of config
	fp->printf("/lang %s\n", languageCode);
	fp->printf("/res %dx%d\n", xres, yres);
	fp->printf("/gamma %3.3f\n", vidgamma);
	fp->printf("/fov %d\n", fov);
	fp->printf("/fps %d\n", fpsLimit);
	fp->printf("/svflags %d\n", svFlags);
	if ( lastname != "" )
	{
		fp->printf("/lastname %s\n", lastname.c_str());
	}
	if ( smoothlighting )
	{
		fp->printf("/smoothlighting\n");
	}
	if ( fullscreen )
	{
		fp->printf("/fullscreen\n");
	}
	if ( borderless )
	{
		fp->printf("/borderless\n");
	}
	if ( shaking )
	{
		fp->printf("/shaking\n");
	}
	if ( bobbing )
	{
		fp->printf("/bobbing\n");
	}
	fp->printf("/sfxvolume %d\n", sfxvolume);
	fp->printf("/sfxambientvolume %d\n", sfxAmbientVolume);
	fp->printf("/sfxenvironmentvolume %d\n", sfxEnvironmentVolume);
	fp->printf("/musvolume %d\n", musvolume);
	for (c = 0; c < NUMIMPULSES; c++)
	{
		fp->printf("/bind %d IN_%s\n", impulses[c], impulsenames[c]);
	}
	for (c = 0; c < NUM_JOY_IMPULSES; c++)
	{
		fp->printf("/joybind %d INJOY_%s\n", joyimpulses[c], joyimpulsenames[c]);
	}
	fp->printf("/mousespeed %d\n", (int)(mousespeed));
	if ( reversemouse )
	{
		fp->printf("/reversemouse\n");
	}
	if ( smoothmouse )
	{
		fp->printf("/smoothmouse\n");
	}
	if ( disablemouserotationlimit )
	{
		fp->printf("/disablemouserotationlimit\n");
	}
	if (last_ip[0])
	{
		fp->printf("/ip %s\n", last_ip);
	}
	if (last_port[0])
	{
		fp->printf("/port %s\n", last_port);
	}
	if (!spawn_blood)
	{
		fp->printf("/noblood\n");
	}
	if ( !flickerLights )
	{
		fp->printf("/nolightflicker\n");
	}
	if ( verticalSync )
	{
		fp->printf("/vsync\n");
	}
	if ( !showStatusEffectIcons )
	{
		fp->printf("/hidestatusicons\n");
	}
	if ( minimapPingMute )
	{
		fp->printf("/muteping\n");
	}
	if ( mute_audio_on_focus_lost )
	{
		fp->printf("/muteaudiofocuslost\n");
	}
	if ( mute_player_monster_sounds )
	{
		fp->printf("/muteplayermonstersounds\n");
	}
	if (colorblind)
	{
		fp->printf("/colorblind\n");
	}
	if (!capture_mouse)
	{
		fp->printf("/nocapturemouse\n");
	}
	if (broadcast)
	{
		fp->printf("/broadcast\n");
	}
	if (nohud)
	{
		fp->printf("/nohud\n");
	}
	if (!auto_hotbar_new_items)
	{
		fp->printf("/disablehotbarnewitems\n");
	}
	for ( c = 0; c < NUM_HOTBAR_CATEGORIES; ++c )
	{
		fp->printf("/hotbarenablecategory %d %d\n", c, auto_hotbar_categories[c]);
	}
	for ( c = 0; c < NUM_AUTOSORT_CATEGORIES; ++c )
	{
		fp->printf("/autosortcategory %d %d\n", c, autosort_inventory_categories[c]);
	}
	if ( hotbar_numkey_quick_add )
	{
		fp->printf("/quickaddtohotbar\n");
	}
	if ( players[clientnum]->characterSheet.lock_right_sidebar )
	{
		fp->printf("/locksidebar\n");
	}
	if ( show_game_timer_always )
	{
		fp->printf("/showgametimer\n");
	}
	if (disable_messages)
	{
		fp->printf("/disablemessages\n");
	}
	if (right_click_protect)
	{
		fp->printf("/right_click_protect\n");
	}
	if (auto_appraise_new_items)
	{
		fp->printf("/autoappraisenewitems\n");
	}
	if (startfloor)
	{
		fp->printf("/startfloor %d\n", startfloor);
	}
	/*if (splitscreen)
	{
		fp->printf("/splitscreen\n");
	}*/
	if ( useModelCache )
	{
		fp->printf("/usemodelcache\n");
	}
	fp->printf("/lastcharacter %d %d %d %d\n", lastCreatedCharacterSex, lastCreatedCharacterClass, lastCreatedCharacterAppearance, lastCreatedCharacterRace);
	fp->printf("/gamepad_deadzone %d\n", gamepad_deadzone);
	fp->printf("/gamepad_trigger_deadzone %d\n", gamepad_trigger_deadzone);
	fp->printf("/gamepad_leftx_sensitivity %d\n", gamepad_leftx_sensitivity);
	fp->printf("/gamepad_lefty_sensitivity %d\n", gamepad_lefty_sensitivity);
	fp->printf("/gamepad_rightx_sensitivity %d\n", gamepad_rightx_sensitivity);
	fp->printf("/gamepad_righty_sensitivity %d\n", gamepad_righty_sensitivity);
	fp->printf("/gamepad_menux_sensitivity %d\n", gamepad_menux_sensitivity);
	fp->printf("/gamepad_menuy_sensitivity %d\n", gamepad_menuy_sensitivity);
	if (gamepad_rightx_invert)
	{
		fp->printf("/gamepad_rightx_invert\n");
	}
	if (gamepad_righty_invert)
	{
		fp->printf("/gamepad_righty_invert\n");
	}
	if (gamepad_leftx_invert)
	{
		fp->printf("/gamepad_leftx_invert\n");
	}
	if (gamepad_lefty_invert)
	{
		fp->printf("/gamepad_lefty_invert\n");
	}
	if (gamepad_menux_invert)
	{
		fp->printf("/gamepad_menux_invert\n");
	}
	if (gamepad_menuy_invert)
	{
		fp->printf("/gamepad_menuy_invert\n");
	}
	fp->printf("/skipintro\n");
	fp->printf("/minimaptransparencyfg %d\n", minimapTransparencyForeground);
	fp->printf("/minimaptransparencybg %d\n", minimapTransparencyBackground);
	fp->printf("/minimapscale %d\n", minimapScale);
	fp->printf("/minimapobjectzoom %d\n", minimapObjectZoom);
	if ( uiscale_charactersheet )
	{
		fp->printf("/uiscale_charsheet\n");
	}
	if ( uiscale_skillspage )
	{
		fp->printf("/uiscale_skillsheet\n");
	}
	fp->printf("/uiscale_inv %f\n", uiscale_inventory);
	fp->printf("/uiscale_hotbar %f\n", uiscale_hotbar);
	fp->printf("/uiscale_chatbox %f\n", uiscale_chatlog);
	fp->printf("/uiscale_playerbars %f\n", uiscale_playerbars);
	if ( hide_playertags )
	{
		fp->printf("/hideplayertags\n");
	}
	if ( hide_statusbar )
	{
		fp->printf("/hidestatusbar\n");
	}
	if ( show_skill_values )
	{
		fp->printf("/showskillvalues\n");
	}
	if ( disableMultithreadedSteamNetworking )
	{
		fp->printf("/disablenetworkmultithreading\n");
	}
	if ( disableFPSLimitOnNetworkMessages )
	{
		fp->printf("/disablenetcodefpslimit\n");
	}
#ifdef USE_EOS
	if ( LobbyHandler.crossplayEnabled )
	{
		fp->printf("/crossplay\n");
	}
#endif // USE_EOS

	if ( !gamemods_mountedFilepaths.empty() )
	{
		std::vector<std::pair<std::string, std::string>>::iterator it;
		for ( it = gamemods_mountedFilepaths.begin(); it != gamemods_mountedFilepaths.end(); ++it )
		{
			fp->printf("/loadmod dir:%s name:%s", (*it).first.c_str(), (*it).second.c_str());
#ifdef STEAMWORKS
			for ( std::vector<std::pair<std::string, uint64>>::iterator itId = gamemods_workshopLoadedFileIDMap.begin();
				itId != gamemods_workshopLoadedFileIDMap.end(); ++itId )
			{
				if ( itId->first.compare((*it).second) == 0 )
				{
					fp->printf(" fileid:%lld", (*itId).second);
				}
			}
#endif // STEAMWORKS
			fp->printf("\n");
		}
	}

	FileIO::close(fp);
	free(filename);
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

hotbar_slot_t* getHotbar(int player, int x, int y, int* outSlotNum)
{
	if ( players[player]->hotbar.useHotbarFaceMenu )
	{
		for ( Uint32 num = 0; num < NUM_HOTBAR_SLOTS; ++num )
		{
			auto& slotRect = players[player]->hotbar.faceButtonPositions[num];
			if ( x >= slotRect.x && x < (slotRect.x + slotRect.w)
				&& y >= slotRect.y && y < (slotRect.y + slotRect.h) )
			{
				if ( outSlotNum )
				{
					*outSlotNum = num;
				}
				return &players[player]->hotbar.slots()[num];
			}
		}
	}
	else if ( x >= players[player]->hotbar.getStartX() 
		&& x < players[player]->hotbar.getStartX() + (NUM_HOTBAR_SLOTS * players[player]->hotbar.getSlotSize())
		&& y >= players[player]->statusBarUI.getStartY() - players[player]->hotbar.getSlotSize()
		&& y < players[player]->statusBarUI.getStartY() )
	{
		int relx = x - players[player]->hotbar.getStartX(); //X relative to the start of the hotbar.
		int slot = std::max(0, std::min(relx / (players[player]->hotbar.getSlotSize()), static_cast<int>(NUM_HOTBAR_SLOTS - 1))); // bounds check
		if ( outSlotNum )
		{
			*outSlotNum = slot;
		}
		return &players[player]->hotbar.slots()[slot]; //The slot will clearly be the x divided by the width of a slot
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

void Player::openStatusScreen(const int whichGUIMode, const int whichInventoryMode)
{
	if ( !inputs.bPlayerIsControllable(playernum) )
	{
		return;
	}

	shootmode = false;

	if ( whichGUIMode != GUI_MODE_NONE && whichGUIMode != GUI_MODE_FOLLOWERMENU )
	{
		FollowerMenu[playernum].closeFollowerMenuGUI();
	}

	bool warpMouseToInventorySlot = false;
	if ( inputs.hasController(playernum) 
		&& gui_mode == GUI_MODE_NONE && whichGUIMode != GUI_MODE_NONE
		&& !FollowerMenu[playernum].followerToCommand )
	{
		warpMouseToInventorySlot = true;
	}

	gui_mode = whichGUIMode;
	inputs.getUIInteraction(playernum)->selectedItem = nullptr;
	inventory_mode = whichInventoryMode;

	Uint32 flags = (Inputs::SET_MOUSE | Inputs::SET_CONTROLLER | Inputs::UNSET_RELATIVE_MOUSE);

	bool warped = false;
	if ( warpMouseToInventorySlot )
	{
		// hide cursor, select an inventory slot and disable hotbar focus.
		if ( auto vmouse = inputs.getVirtualMouse(playernum) )
		{
			if ( vmouse->lastMovementFromController )
			{
				vmouse->draw_cursor = false;
				hotbar.hotbarHasFocus = false;
				warped = true;
				if ( inventoryUI.selectedSlotInPaperDoll() )
				{
					int x = inventoryUI.getSelectedSlotX();
					int y = inventoryUI.getSelectedSlotY();
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
				warpMouseToSelectedInventorySlot(playernum);
			}
		}
	}

	if ( !warped )
	{
		inputs.warpMouse(playernum, camera_x1() + (camera_width() / 2), camera_y1() + (camera_height() / 2), flags);
	}

	//if ( inputs.bPlayerUsingKeyboardControl(playernum) )
	//{
	//	SDL_SetRelativeMouseMode(SDL_FALSE);
	//	//SDL_WarpMouseInWindow(screen, camera_x1() + (camera_width() / 2), camera_y1() + (camera_height() / 2));
	//	mousex = camera_x1() + (camera_width() / 2);
	//	mousey = camera_y1() + (camera_height() / 2);
	//	omousex = mousex;
	//	omousey = mousey;

	//	if ( inputs.hasController(playernum) )
	//	{
	//		const auto& mouse = inputs.getVirtualMouse(playernum);
	//		mouse->x = mousex;
	//		mouse->y = mousey;
	//		mouse->ox = omousex;
	//		mouse->oy = omousey;
	//	}
	//}
	//else if ( inputs.hasController(playernum) )
	//{
	//	const auto& mouse = inputs.getVirtualMouse(playernum);
	//	mouse->x = camera_x1() + (camera_width() / 2);
	//	mouse->y = camera_y1() + (camera_height() / 2);
	//	mouse->ox = mouse->x;
	//	mouse->oy = mouse->y;
	//}
	
	players[playernum]->characterSheet.attributespage = 0;
	//proficienciesPage = 0;
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

	if ( whatToClose != CLOSEGUI_DONT_CLOSE_SHOP && shopkeeper[playernum] != 0 )
	{
		if ( multiplayer != CLIENT )
		{
			Entity* entity = uidToEntity(shopkeeper[playernum]);
			if ( entity )
			{
				entity->skill[0] = 0;
				if ( uidToEntity(entity->skill[1]) )
				{
					monsterMoveAside(entity, uidToEntity(entity->skill[1]));
				}
				entity->skill[1] = 0;
			}
		}
		else
		{
			// inform server that we're done talking to shopkeeper
			strcpy((char*)net_packet->data, "SHPC");
			SDLNet_Write32((Uint32)shopkeeper[playernum], &net_packet->data[4]);
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 8;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
		closeShop(playernum);
	}
	gui_mode = GUI_MODE_NONE;
	if ( shootmodeAction == CLOSEGUI_ENABLE_SHOOTMODE )
	{
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
		if ( stats[gui_player] && players[gui_player] && players[gui_player]->entity )
		{
			if ( (*inputPressed(impulses[IN_FOLLOWERMENU_LASTCMD]) || inputs.bControllerInputPressed(gui_player, INJOY_GAME_FOLLOWERMENU_LASTCMD)) && optionPrevious != -1 )
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
		if ( (!(*inputPressedForPlayer(gui_player, impulses[IN_USE])) && !(inputs.bControllerInputPressed(gui_player, INJOY_GAME_USE)) && !inputs.bControllerInputPressed(gui_player, INJOY_GAME_FOLLOWERMENU) && !menuToggleClick && !holdWheel)
			|| ((*inputPressedForPlayer(gui_player, impulses[IN_USE]) || inputs.bControllerInputPressed(gui_player, INJOY_GAME_USE) || inputs.bControllerInputPressed(gui_player, INJOY_GAME_FOLLOWERMENU)) && menuToggleClick)
			|| (!(*inputPressedForPlayer(gui_player, impulses[IN_FOLLOWERMENU])) && holdWheel && !menuToggleClick)
			|| ((*inputPressedForPlayer(gui_player, impulses[IN_FOLLOWERMENU_LASTCMD]) || inputs.bControllerInputPressed(gui_player, INJOY_GAME_FOLLOWERMENU_LASTCMD)) && optionPrevious != -1)
			)
		{
			if ( menuToggleClick )
			{
				*inputPressedForPlayer(gui_player, impulses[IN_USE]) = 0;
				inputs.controllerClearInput(gui_player, INJOY_GAME_USE);
				inputs.controllerClearInput(gui_player, INJOY_GAME_FOLLOWERMENU);
				menuToggleClick = false;
				if ( optionSelected == -1 )
				{
					optionSelected = ALLY_CMD_CANCEL;
				}
			}

			bool usingLastCmd = false;
			if ( *inputPressedForPlayer(gui_player, impulses[IN_FOLLOWERMENU_LASTCMD]) || inputs.bControllerInputPressed(gui_player, INJOY_GAME_FOLLOWERMENU_LASTCMD) )
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

			if ( *inputPressedForPlayer(gui_player, impulses[IN_FOLLOWERMENU_LASTCMD]) || inputs.bControllerInputPressed(gui_player, INJOY_GAME_FOLLOWERMENU_LASTCMD) )
			{
				if ( keepWheelOpen )
				{
					// need to reset the coordinates of the mouse.
					initfollowerMenuGUICursor(false);
				}
				*inputPressedForPlayer(gui_player, impulses[IN_FOLLOWERMENU_LASTCMD]) = 0;
				inputs.controllerClearInput(gui_player, INJOY_GAME_FOLLOWERMENU_LASTCMD);
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
						if ( followerStats->type < KOBOLD ) // Original monster count
						{
							if ( disableOption < 0 )
							{
								messagePlayer(gui_player, language[3640], language[90 + followerStats->type]);
							}
							else if ( tinkeringFollower )
							{
								messagePlayer(gui_player, language[3639], language[90 + followerStats->type]);
							}
							else
							{
								messagePlayer(gui_player, language[3638], language[90 + followerStats->type]);
							}
						}
						else if ( followerStats->type >= KOBOLD ) //New monsters
						{
							if ( disableOption < 0 )
							{
								messagePlayer(gui_player, language[3640], language[2000 + (followerStats->type - KOBOLD)]);
							}
							else if ( tinkeringFollower )
							{
								messagePlayer(gui_player, language[3639], language[2000 + (followerStats->type - KOBOLD)]);
							}
							else
							{
								messagePlayer(gui_player, language[3638], language[2000 + (followerStats->type - KOBOLD)]);
							}
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
				centerx + (radius + thickness) * cos(angleStart), centery - (radius + thickness) * sin(angleStart), uint32ColorGray(*mainsurface), 192);
			drawLine(centerx + (radius - thickness) * cos(angleEnd), centery - (radius - thickness) * sin(angleEnd),
				centerx + (radius + thickness - 1) * cos(angleEnd), centery - (radius + thickness - 1) * sin(angleEnd), uint32ColorGray(*mainsurface), 192);

			drawArcInvertedY(centerx, centery, radius - thickness, std::round((angleStart * 180) / PI), ((angleEnd * 180) / PI), uint32ColorGray(*mainsurface), 192);
			drawArcInvertedY(centerx, centery, (radius + thickness), std::round((angleStart * 180) / PI), ((angleEnd * 180) / PI) + 1, uint32ColorGray(*mainsurface), 192);

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
						Uint32 borderColor = uint32ColorBaronyBlue(*mainsurface);
						if ( optionDisabledForCreature(skillLVL, followerStats->type, i) != 0 )
						{
							borderColor = uint32ColorOrange(*mainsurface);
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
			drawCircle(centerx, centery, radius - thickness, uint32ColorBaronyBlue(*mainsurface), 192);
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
			else if ( disableOption == -3 ) // disabled due to tinkerbot quality
			{
				tooltip.h = TTF12_HEIGHT + 8;
				tooltip.w = longestline(language[3673]) * TTF12_WIDTH + 8;
				drawTooltip(&tooltip);
				if ( followerStats->type >= KOBOLD ) //New monsters
				{
					tooltip.w += strlen(language[2000 + followerStats->type - KOBOLD]) * TTF12_WIDTH;
					drawTooltip(&tooltip);
					ttfPrintTextFormattedColor(ttf12, tooltip.x + 4, tooltip.y + 6,
						uint32ColorOrange(*mainsurface), language[3673], language[2000 + followerStats->type - KOBOLD]);
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
					uint32ColorOrange(*mainsurface), lowSkillLVLTooltip, requirement.c_str(), current.c_str());
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
			if ( monsterType < KOBOLD ) //Original monster count
			{
				strcat(interactText, language[90 + monsterType]);
			}
			else if ( monsterType >= KOBOLD ) //New monsters
			{
				strcat(interactText, language[2000 + monsterType - KOBOLD]);
			}
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
			if ( !experimentingAlchemy )
			{
				messagePlayer(gui_player, language[3338]);
			}
			else
			{
				messagePlayer(gui_player, language[3343]);
			}
			closeGUI();
			return;
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

		gui_starty = (players[gui_player]->camera_midx() - (inventoryChest_bmp->w / 2)) + offsetx;
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
			windowX1 -= 20;
			windowX2 += 20;
			windowY1 -= 40;
			windowY2 += 40;
			drawWindowFancy(windowX1, windowY1, windowX2, windowY2);
			int numMetalScrap = 0;
			int numMagicScrap = 0;
			if ( !tinkeringMetalScrap.empty() )
			{
				for ( auto uid : tinkeringMetalScrap )
				{
					if ( uidToItem(uid) )
					{
						numMetalScrap += (uidToItem(uid))->count;
					}
				}
			}
			if ( !tinkeringMagicScrap.empty() )
			{
				for ( auto uid : tinkeringMagicScrap )
				{
					if ( uidToItem(uid) )
					{
						numMagicScrap += (uidToItem(uid))->count;
					}
				}
			}

			// title
			ttfPrintTextFormatted(ttf12, windowX1 + 16, windowY1 + 8,
				language[3690]);
			char kitStatusText[64] = "";
			if ( tinkeringKitItem )
			{
				snprintf(kitStatusText, 63, language[3691], language[3691 + std::max(1, static_cast<int>(tinkeringKitItem->status))]);
			}
			ttfPrintTextFormatted(ttf12, windowX2 - 16 - (strlen(kitStatusText) + 1) * TTF12_WIDTH, windowY2 - TTF12_HEIGHT - 8,
				kitStatusText);

			ttfPrintTextFormatted(ttf12, windowX1 + 16, windowY2 - TTF12_HEIGHT - 8,
				language[3647], numMetalScrap, numMagicScrap);
			SDL_Rect smallIcon;
			smallIcon.x = windowX1 + 16 + (strlen(language[3647]) - 5) * TTF12_WIDTH;
			smallIcon.y = windowY2 - TTF12_HEIGHT - 12;
			smallIcon.h = 16;
			smallIcon.w = 16;
			node_t* imageNode = items[TOOL_METAL_SCRAP].surfaces.first;
			if ( imageNode )
			{
				drawImageScaled(*((SDL_Surface**)imageNode->element), NULL, &smallIcon);
			}
			smallIcon.x += TTF12_WIDTH * 6;
			imageNode = items[TOOL_MAGIC_SCRAP].surfaces.first;
			if ( imageNode )
			{
				drawImageScaled(*((SDL_Surface**)imageNode->element), NULL, &smallIcon);
			}

			// draw filter labels.
			int txtWidth = 0;
			int txtHeight = 0;
			int charWidth = 0;
			TTF_Font* font = ttf8;
			getSizeOfText(font, "a", &charWidth, nullptr); // get 1 character width.
			int textstartx = pos.x + 2 * charWidth + 4;

			SDL_Rect highlightBtn;
			// Craft
			getSizeOfText(ttf8, language[3644], &txtWidth, &txtHeight);
			highlightBtn.x = textstartx;
			highlightBtn.y = pos.y + (12 - txtHeight);
			highlightBtn.w = txtWidth + 2 * charWidth + 4;
			highlightBtn.h = txtHeight + 4;
			if ( (inputs.bMouseLeft(gui_player) || inputs.bControllerInputPressed(gui_player, INJOY_MENU_USE))
				&& mouseInBounds(gui_player, highlightBtn.x, highlightBtn.x + highlightBtn.w, highlightBtn.y, highlightBtn.y + highlightBtn.h) )
			{
				tinkeringFilter = TINKER_FILTER_CRAFTABLE;
				inputs.controllerClearInput(gui_player, INJOY_MENU_USE);
				inputs.mouseClearLeft(gui_player);
			}
			if ( tinkeringFilter == TINKER_FILTER_CRAFTABLE )
			{
				drawImageScaled(button_bmp, NULL, &highlightBtn);
			}
			ttfPrintText(font, highlightBtn.x + 4 + charWidth, pos.y - (8 - txtHeight), language[3644]);

			// Salvage
			getSizeOfText(font, language[3645], &txtWidth, &txtHeight);
			highlightBtn.x += highlightBtn.w;
			highlightBtn.y = pos.y + (12 - txtHeight);
			highlightBtn.w = txtWidth + 2 * charWidth + 4;
			highlightBtn.h = txtHeight + 4;
			if ( (inputs.bMouseLeft(gui_player) || inputs.bControllerInputPressed(gui_player, INJOY_MENU_USE))
				&& mouseInBounds(gui_player, highlightBtn.x, highlightBtn.x + highlightBtn.w, highlightBtn.y, highlightBtn.y + highlightBtn.h) )
			{
				tinkeringFilter = TINKER_FILTER_SALVAGEABLE;
				inputs.controllerClearInput(gui_player, INJOY_MENU_USE);
				inputs.mouseClearLeft(gui_player);
			}
			if ( tinkeringFilter == TINKER_FILTER_SALVAGEABLE )
			{
				drawImageScaled(button_bmp, NULL, &highlightBtn);
			}
			ttfPrintText(font, highlightBtn.x + 4 + charWidth, pos.y - (8 - txtHeight), language[3645]);

			// Repair
			getSizeOfText(font, language[3646], &txtWidth, &txtHeight);
			highlightBtn.x += highlightBtn.w;
			highlightBtn.y = pos.y + (12 - txtHeight);
			highlightBtn.w = txtWidth + 2 * charWidth + 4;
			highlightBtn.h = txtHeight + 4;
			if ( (inputs.bMouseLeft(gui_player) || inputs.bControllerInputPressed(gui_player, INJOY_MENU_USE))
				&& mouseInBounds(gui_player, highlightBtn.x, highlightBtn.x + highlightBtn.w, highlightBtn.y, highlightBtn.y + highlightBtn.h) )
			{
				tinkeringFilter = TINKER_FILTER_REPAIRABLE;
				inputs.controllerClearInput(gui_player, INJOY_MENU_USE);
				inputs.mouseClearLeft(gui_player);
			}
			if ( tinkeringFilter == TINKER_FILTER_REPAIRABLE )
			{
				drawImageScaled(button_bmp, NULL, &highlightBtn);
			}
			ttfPrintText(font, highlightBtn.x + 4 + charWidth, pos.y - (8 - txtHeight), language[3646]);

			// Filter include all (*)
			getSizeOfText(font, language[356], &txtWidth, &txtHeight);
			highlightBtn.x += highlightBtn.w;
			highlightBtn.y = pos.y + (12 - txtHeight);
			highlightBtn.w = 2 * charWidth + 4;
			highlightBtn.h = txtHeight + 4;
			if ( (inputs.bMouseLeft(gui_player) || inputs.bControllerInputPressed(gui_player, INJOY_MENU_USE))
				&& mouseInBounds(gui_player, highlightBtn.x, highlightBtn.x + highlightBtn.w, highlightBtn.y, highlightBtn.y + highlightBtn.h) )
			{
				tinkeringFilter = TINKER_FILTER_ALL;
				inputs.controllerClearInput(gui_player, INJOY_MENU_USE);
				inputs.mouseClearLeft(gui_player);
			}
			if ( tinkeringFilter == TINKER_FILTER_ALL )
			{
				drawImageScaled(smallbutton_bmp, NULL, &highlightBtn);
			}
			ttfPrintText(font, highlightBtn.x + (highlightBtn.w - txtWidth) / 2, pos.y - (8 - txtHeight), language[356]);
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
				ttfPrintTextFormattedColor(ttf12, windowX2 - 16 - 11 * TTF12_WIDTH, windowY2 - TTF12_HEIGHT - 8, uint32ColorRed(*mainsurface),
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
			messagePlayer(0, "Warning: stats[%d].inventory is not a valid list. This should not happen.", gui_player);
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
				if ( !basePotion )
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
				}
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
								warpMouseToSelectedInventorySlot(gui_player);
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

			//Okay, now prepare to render all the items.
			y = gui_startx + 22;
			c = 0;
			if ( player_inventory )
			{
				rebuildGUIInventory();

				//Actually render the items.
				c = 0;
				for ( node = player_inventory->first; node != NULL; node = node->next )
				{
					if ( node->element )
					{
						item = (Item*)node->element;
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
							Uint32 color = uint32ColorWhite(*mainsurface);
							if ( guiType == GUI_TYPE_TINKERING )
							{
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
										color = uint32ColorGray(*mainsurface);
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
											color = uint32ColorGray(*mainsurface); // can't upgrade since it's higher status than we can craft.
										}
										else if ( !tinkeringPlayerCanAffordRepair(item) )
										{
											color = uint32ColorGray(*mainsurface); // can't upgrade since no materials
										}
										strcpy(tempstr, language[3684]); // upgrade
										strncat(tempstr, item->description(), 46 - strlen(language[3684]));
									}
									else
									{
										if ( tinkeringPlayerHasSkillLVLToCraft(item) == -1 && itemCategory(item) == TOOL )
										{
											color = uint32ColorGray(*mainsurface); // can't repair since no we can't craft it.
										}
										else if ( !tinkeringPlayerCanAffordRepair(item) )
										{
											color = uint32ColorGray(*mainsurface); // can't repair since no materials
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
									messagePlayer(0, "%d", item->type);
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
		messagePlayer(gui_player, language[347], item->getName());
		return;
	}

	item->beatitude = 0; //0 = uncursed. > 0 = blessed.
	messagePlayer(gui_player, language[348], item->description());

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
		messagePlayer(gui_player, language[319], item->getName());
		return;
	}

	item->identified = true;
	messagePlayer(gui_player, language[320], item->description());
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
		messagePlayer(gui_player, language[3287], item->getName());
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
		messagePlayer(gui_player, language[3730], item->getName());
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
		messagePlayer(gui_player, language[872], item->getName());
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
	SDL_WarpMouseInWindow(screen, slotPos.x + (slotPos.w / 2), slotPos.y + (slotPos.h / 2));
}

void GenericGUIMenu::initGUIControllerCode()
{
	if ( itemsDisplayed[0] )
	{
		selectedSlot = 0;
		this->warpMouseToSelectedSlot();
	}
	else
	{
		selectedSlot = -1;
	}
}

void GenericGUIMenu::openGUI(int type, int scrollBeatitude, int scrollType)
{
	this->closeGUI();

	players[gui_player]->shootmode = false;
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
	this->initGUIControllerCode();
}

void GenericGUIMenu::openGUI(int type, bool experimenting, Item* itemOpenedWith)
{
	this->closeGUI();
	players[gui_player]->shootmode = false;
	players[gui_player]->openStatusScreen(GUI_MODE_INVENTORY, INVENTORY_MODE_ITEM); // Reset the GUI to the inventory.
	guiActive = true;
	alembicItem = itemOpenedWith;
	experimentingAlchemy = experimenting;
	guiType = static_cast<GUICurrentType>(type);

	gui_starty = (players[gui_player]->camera_midx() - (inventoryChest_bmp->w / 2)) + offsetx;
	gui_startx = (players[gui_player]->camera_midy() - (inventoryChest_bmp->h / 2)) + offsety;

	FollowerMenu[gui_player].closeFollowerMenuGUI();

	if ( openedChest[gui_player] )
	{
		openedChest[gui_player]->closeChest();
	}
	rebuildGUIInventory();
	this->initGUIControllerCode();
}

void GenericGUIMenu::openGUI(int type, Item* itemOpenedWith)
{
	this->closeGUI();
	players[gui_player]->shootmode = false;
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
	this->initGUIControllerCode();
}

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
					messagePlayer(gui_player, language[3337]);
				}
				else
				{
					messagePlayer(gui_player, language[3342]);
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
			if ( tinkeringIsItemRepairable(item, gui_player) && tinkeringFilter == TINKER_FILTER_REPAIRABLE )
			{
				tinkeringRepairItem(item);
			}
			else
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

	if ( players[gui_player] && players[gui_player]->entity )
	{
		if ( players[gui_player]->entity->isBlind() )
		{
			messagePlayer(gui_player, language[892]);
			closeGUI();
			return false; // I can't see!
		}
	}


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
				if ( clientLearnedAlchemyIngredients.find(item->type) != clientLearnedAlchemyIngredients.end() )
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
				if ( clientLearnedAlchemyIngredients.find(item->type) != clientLearnedAlchemyIngredients.end() )
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

void GenericGUIMenu::alchemyCombinePotions()
{
	if ( !basePotion || !secondaryPotion )
	{
		return;
	}

	bool tryDuplicatePotion = false;
	ItemType result = POTION_SICKNESS;
	bool randomResult = false;
	bool explodeSelf = false;

	switch ( basePotion->type )
	{
		case POTION_WATER:
			if ( secondaryPotion->type == POTION_ACID )
			{
				explodeSelf = true;
			}
			else
			{
				tryDuplicatePotion = true;
			}
			break;
		case POTION_BOOZE:
			switch ( secondaryPotion->type )
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
					randomResult = true;
					break;
				default:
					break;
			}
			break;
		case POTION_JUICE:
			switch ( secondaryPotion->type )
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
					randomResult = true;
					break;
				default:
					break;
			}
			break;
		case POTION_ACID:
			switch ( secondaryPotion->type )
			{
				case POTION_WATER:
					explodeSelf = true; // oh no. don't do that.
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
					randomResult = true;
					break;
				default:
					explodeSelf = true;
					break;
			}
			break;
		case POTION_INVISIBILITY:
			switch ( secondaryPotion->type )
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
					randomResult = true;
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
		switch ( secondaryPotion->type )
		{
			case POTION_WATER:
				if ( basePotion->type == POTION_ACID )
				{
					explodeSelf = true;
				}
				else
				{
					tryDuplicatePotion = true;
				}
				break;
			case POTION_BOOZE:
				switch ( basePotion->type )
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
						randomResult = true;
						break;
					default:
						break;
				}
				break;
			case POTION_JUICE:
				switch ( basePotion->type )
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
						randomResult = true;
						break;
					default:
						break;
				}
				break;
			case POTION_ACID:
				switch ( basePotion->type )
				{
					case POTION_WATER:
						explodeSelf = true; // oh no. don't do that.
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
						randomResult = true;
						break;
					default:
						explodeSelf = true;
						break;
				}
				break;
			case POTION_INVISIBILITY:
				switch ( basePotion->type )
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
						randomResult = true;
						break;
					default:
						break;
				}
				break;
			default:
				break;
		}
	}

	if ( basePotion->type == POTION_POLYMORPH || secondaryPotion->type == POTION_POLYMORPH )
	{
		randomResult = true;
	}

	int skillLVL = 0;
	if ( stats[gui_player] )
	{
		skillLVL = stats[gui_player]->PROFICIENCIES[PRO_ALCHEMY] / 20; // 0 to 5;
	}

	Status status = SERVICABLE;
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
		messagePlayerColor(gui_player, uint32ColorWhite(*mainsurface), language[3332],
			items[basePotion->type].name_identified, items[secondaryPotion->type].name_identified);
	}
	else if ( basePotion->identified )
	{
		messagePlayerColor(gui_player, uint32ColorWhite(*mainsurface), language[3334],
			items[basePotion->type].name_identified);
	}
	else if ( secondaryPotion->identified )
	{
		messagePlayerColor(gui_player, uint32ColorWhite(*mainsurface), language[3333],
			items[secondaryPotion->type].name_identified);
	}
	else
	{
		messagePlayerColor(gui_player, uint32ColorWhite(*mainsurface), language[3335]);
	}

	if ( !explodeSelf && result != POTION_SICKNESS && !tryDuplicatePotion )
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

	int appearance = 0;
	int blessing = std::min(static_cast<int>(std::min(basePotion->beatitude, secondaryPotion->beatitude)), 0);
	if ( basePotion->type == secondaryPotion->type )
	{
		// same potion, keep the second potion only.
		result = secondaryPotion->type;
		blessing = secondaryPotion->beatitude;
		appearance = secondaryPotion->appearance;
	}

	bool knewBothBaseIngredients = false;
	if ( clientLearnedAlchemyIngredients.find(basePotion->type) != clientLearnedAlchemyIngredients.end() )
	{
		if ( clientLearnedAlchemyIngredients.find(secondaryPotion->type) != clientLearnedAlchemyIngredients.end() )
		{
			// knew about both combinations.
			if ( !tryDuplicatePotion && !explodeSelf && result != POTION_SICKNESS )
			{
				if ( skillLVL >= 3 )
				{
					knewBothBaseIngredients = true; // auto ID the new potion.
				}
			}
		}
	}

	Item* duplicatedPotion = nullptr;

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
		consumeItem(basePotion, gui_player);
		consumeItem(secondaryPotion, gui_player);
		if ( rand() % 100 < (50 + skillLVL * 5) ) // 50 - 75% chance
		{
			Item* emptyBottle = newItem(POTION_EMPTY, SERVICABLE, 0, 1, 0, true, nullptr);
			itemPickup(gui_player, emptyBottle);
			messagePlayer(gui_player, language[3351], items[POTION_EMPTY].name_identified);
			free(emptyBottle);
		}
	}

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
		degradeAlembic = false;
	}

	if ( degradeAlembic && alembicItem )
	{
		alembicItem->status = static_cast<Status>(alembicItem->status - 1);
		if ( alembicItem->status > BROKEN )
		{
			messagePlayer(gui_player, language[681], alembicItem->getName());
		}
		else
		{
			messagePlayer(gui_player, language[2351], alembicItem->getName());
			playSoundPlayer(gui_player, 162, 64);
			consumeItem(alembicItem, gui_player);
			alembicItem = nullptr;
		}
	}

	if ( explodeSelf && players[gui_player] && players[gui_player]->entity )
	{
		// hurt.
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
		}
		closeGUI();
		return;
	}

	for ( auto it = potionStandardAppearanceMap.begin(); it != potionStandardAppearanceMap.end(); ++it )
	{
		if ( (*it).first == result )
		{
			if ( appearance == 0 )
			{
				appearance = (*it).second;
			}
			bool raiseSkill = true;
			if ( result == POTION_SICKNESS )
			{
				appearance = 0 + rand() % 3;
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

			Item* newPotion = newItem(result, status, blessing, 1, appearance, knewBothBaseIngredients, nullptr);
			if ( tryDuplicatePotion )
			{
				if ( result == POTION_WATER && !duplicateSucceed )
				{
					messagePlayer(gui_player, language[3356]);
					newPotion->identified = true;
				}
				else
				{
					if ( duplicatedPotion )
					{
						newPotion->appearance = duplicatedPotion->appearance;
						newPotion->beatitude = duplicatedPotion->beatitude;
						newPotion->identified = duplicatedPotion->identified;
						newPotion->status = duplicatedPotion->status;
					}
					messagePlayer(gui_player, language[3352], newPotion->description());
				}
			}
			else
			{
				messagePlayer(gui_player, language[3352], newPotion->description());
				steamStatisticUpdate(STEAM_STAT_IN_THE_MIX, STEAM_STAT_INT, 1);
			}
			itemPickup(gui_player, newPotion);
			free(newPotion);
			if ( players[gui_player] && players[gui_player]->entity )
			{
				playSoundEntityLocal(players[gui_player]->entity, 401, 64);
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
	closeGUI();
}

bool GenericGUIMenu::alchemyLearnRecipe(int type, bool increaseskill, bool notify)
{
	int index = 0;
	for ( auto it = potionStandardAppearanceMap.begin(); it != potionStandardAppearanceMap.end(); ++it )
	{
		// loop through to get the index number to insert into gameStatistics[STATISTICS_ALCHEMY_RECIPES]
		if ( (*it).first == type )
		{
			if ( clientLearnedAlchemyIngredients.find(type) == clientLearnedAlchemyIngredients.end() )
			{
				// new recipe!
				clientLearnedAlchemyIngredients.insert(type);
				Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
				if ( notify )
				{
					if ( isItemBaseIngredient(type) )
					{
						messagePlayerColor(gui_player, color, language[3346], items[type].name_identified);
					}
					else if ( isItemSecondaryIngredient(type) )
					{
						messagePlayerColor(gui_player, color, language[3349], items[type].name_identified);
					}
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
	items.push_back(newItem(TOOL_FREEZE_BOMB, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items.push_back(newItem(TOOL_SLEEP_BOMB, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items.push_back(newItem(TOOL_TELEPORT_BOMB, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items.push_back(newItem(TOOL_DUMMYBOT, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items.push_back(newItem(TOOL_DECOY, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items.push_back(newItem(TOOL_GYROBOT, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items.push_back(newItem(TOOL_SENTRYBOT, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items.push_back(newItem(TOOL_SPELLBOT, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items.push_back(newItem(TOOL_BEARTRAP, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items.push_back(newItem(CLOAK_BACKPACK, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items.push_back(newItem(TOOL_ALEMBIC, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items.push_back(newItem(TOOL_LOCKPICK, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items.push_back(newItem(TOOL_GLASSES, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items.push_back(newItem(TOOL_LANTERN, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items.push_back(newItem(POTION_EMPTY, SERVICABLE, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
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
		playSound(90, 64);
		messagePlayer(gui_player, language[3652], items[item->type].name_identified);
		return false;
	}
	if ( !tinkeringPlayerCanAffordCraft(item) )
	{
		playSound(90, 64);
		messagePlayer(gui_player, language[3648], items[item->type].name_identified);
		return false;
	}

	Item* crafted = tinkeringCraftItemAndConsumeMaterials(item);
	if ( crafted )
	{
		Item* pickedUp = itemPickup(gui_player, crafted);
		messagePlayer(gui_player, language[3668], crafted->description());
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
		messagePlayer(player, language[3669]);
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
			if ( bonusMetalScrap > 0 )
			{
				Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
				messagePlayerColor(player, color, language[3665], metal, items[pickedUp->type].name_identified);
			}
			else
			{
				messagePlayer(player, language[3665], metal, items[pickedUp->type].name_identified);
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
			if ( bonusMagicScrap > 0 )
			{
				Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
				messagePlayerColor(player, color, language[3665], magic, items[pickedUp->type].name_identified);
			}
			else
			{
				messagePlayer(player, language[3665], magic, items[pickedUp->type].name_identified);
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
				else if ( rand() % 20 == 0 )
				{
					messagePlayer(player, language[3666]); // nothing left to learn from salvaging.
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
				else if ( rand() % 20 == 0 )
				{
					messagePlayer(player, language[3666]); // nothing left to learn from salvaging.
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

		if ( players[player] && players[player]->entity )
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
						messagePlayer(gui_player, language[3667], items[item->type].name_identified);
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
					int metalSalvage = 0;
					int magicSalvage = 0;
					tinkeringGetItemValue(item, &metalSalvage, &magicSalvage);
					*metal = metalSalvage * 8;
					*magic = magicSalvage * 8;
					int blessingOrCurse = abs(item->beatitude);
					*magic += blessingOrCurse * 4;
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
			messagePlayer(gui_player, language[681], toDegrade->getName());
		}
		else
		{
			messagePlayer(gui_player, language[662], toDegrade->getName());
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
					playSound(90, 64);
					messagePlayer(gui_player, language[3685], items[item->type].name_identified);
					return false;
				}
				else if ( !tinkeringPlayerCanAffordRepair(item) )
				{
					playSound(90, 64);
					messagePlayer(gui_player, language[3687], items[item->type].name_identified);
					return false;
				}
				
				Status newStatus = DECREPIT;
				Status maxStatus = static_cast<Status>(tinkeringUpgradeMaxStatus(item));

				if ( maxStatus <= item->status )
				{
					playSound(90, 64);
					messagePlayer(gui_player, language[3685], items[item->type].name_identified);
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
					messagePlayer(gui_player, language[3683], items[item->type].name_identified);
					consumeItem(item, gui_player);
					return true;
				}
			}
			else
			{
				int craftRequirement = tinkeringPlayerHasSkillLVLToCraft(item);
				if ( craftRequirement == -1 ) // can't craft, can't repair!
				{
					playSound(90, 64);
					messagePlayer(gui_player, language[3688], items[item->type].name_identified);
					return false;
				}
				else if ( !tinkeringPlayerCanAffordRepair(item) )
				{
					playSound(90, 64);
					messagePlayer(gui_player, language[3686], items[item->type].name_identified);
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
					messagePlayer(gui_player, language[3682], items[item->type].name_identified);
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
				playSound(90, 64);
				messagePlayer(gui_player, language[3688], items[item->type].name_identified);
				return false;
			}
			if ( !tinkeringPlayerCanAffordRepair(item) )
			{
				playSound(90, 64);
				messagePlayer(gui_player, language[3686], items[item->type].name_identified);
				return false;
			}

			if ( tinkeringConsumeMaterialsForRepair(item, false) )
			{
				int repairedStatus = std::min(static_cast<Status>(item->status + 1), EXCELLENT);
				bool isEquipped = itemIsEquipped(item, gui_player);
				item->status = static_cast<Status>(repairedStatus);
				messagePlayer(gui_player, language[872], item->getName());
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
			messagePlayer(gui_player, language[3727], toDegrade->getName());
			scribingToolItem = nullptr;
			return usageCost;
		}
		else
		{
			if ( durability > 25 && (toDegrade->appearance % ENCHANTED_FEATHER_MAX_DURABILITY) <= 25 )
			{
				// notify we're at less than 25%.
				messagePlayer(gui_player, language[3729], toDegrade->getName());
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
			messagePlayer(gui_player, language[3727], toDegrade->getName());
			scribingToolItem = nullptr;
			return usageCost;
		}
		else
		{
			messagePlayer(gui_player, language[3728], toDegrade->getName());
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
			//messagePlayerColor(gui_player, uint32ColorGreen(*mainsurface), language[3724]);
			int oldcount = pickedUp->count;
			pickedUp->count = 1;
			messagePlayerColor(gui_player, uint32ColorGreen(*mainsurface), language[3724], pickedUp->description());
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
		messagePlayer(gui_player, language[3725]);
		messagePlayer(gui_player, language[872], item->getName());
		if ( !isEquipped )
		{
			Item* repairedItem = newItem(item->type, item->status, item->beatitude, 1, item->appearance, true, nullptr);
			if ( repairedItem )
			{
				itemPickup(gui_player, repairedItem);
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

void EnemyHPDamageBarHandler::displayCurrentHPBar(const int player)
{
	if ( HPBars.empty() )
	{
		return;
	}
	Uint32 mostRecentTicks = 0;
	auto mostRecentEntry = HPBars.end();
	auto highPriorityMostRecentEntry = HPBars.end();
	bool foundHighPriorityEntry = false;
	for ( auto it = HPBars.begin(); it != HPBars.end(); )
	{
		if ( ticks - (*it).second.enemy_timer >= k_maxTickLifetime )
		{
			it = HPBars.erase(it); // no need to show this bar, delete it
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
			}
			else
			{
				(*it).second.shouldDisplay = false;
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

		auto& HPDetails = (*mostRecentEntry).second;
		HPDetails.enemy_hp = std::max(0, HPDetails.enemy_hp);

		const int barWidth = 512;
		SDL_Rect pos;
		pos.w = barWidth;
		pos.h = 38;
		//pos.y = players[player]->camera_y2() - 224;
		if ( players[player]->hotbar.useHotbarFaceMenu )
		{
			// anchor to the topmost position, including button glyphs
			pos.y = players[player]->hotbar.faceButtonTopYPosition - pos.h - 8;
		}
		else
		{
			pos.y = players[player]->hotbar.hotbarBox.y - pos.h - 8;
		}
		if ( players[player]->isLocalPlayer() && players[player]->camera_width() < yres )
		{
			if ( yres < 900 )
			{
				pos.w *= 0.5;
			}
			else if ( yres < 1080 )
			{
				pos.w *= 0.8;
			}
		}
		pos.x = players[player]->camera_midx() - (pos.w / 2);

		// bar
		drawTooltip(&pos);

		pos.w = pos.w - 6;
		pos.x = pos.x + 3;
		pos.y = pos.y + 3;
		pos.h = pos.h - 6;
		drawRect(&pos, SDL_MapRGB(mainsurface->format, 16, 0, 0), 255);

		if ( HPDetails.enemy_oldhp > HPDetails.enemy_hp )
		{
			int timeDiff = ticks - HPDetails.enemy_timer;
			if ( timeDiff > 30 || HPDetails.enemy_hp == 0 )
			{
				// delay 30 ticks before background hp drop animation, or if health 0 start immediately.
				// we want to complete animation with x ticks to go
				int depletionTicks = (80 - timeDiff);
				int healthDiff = HPDetails.enemy_oldhp - HPDetails.enemy_hp;

				// scale duration to FPS - tested @ 144hz
				real_t fpsScale = (144.f / std::max(1U, fpsLimit));
				HPDetails.depletionAnimationPercent -= fpsScale * (std::max((healthDiff) / std::max(depletionTicks, 1), 1) / 100.0);
				HPDetails.enemy_oldhp = HPDetails.depletionAnimationPercent * HPDetails.enemy_maxhp; // this follows the animation
			}
			else
			{
				HPDetails.depletionAnimationPercent =
					HPDetails.enemy_oldhp / static_cast<real_t>(HPDetails.enemy_maxhp);
			}
			int tmpw = pos.w;
			pos.w = pos.w * HPDetails.depletionAnimationPercent;
			if ( HPDetails.enemy_bar_color > 0 )
			{
				drawRect(&pos, HPDetails.enemy_bar_color, 128);
			}
			else
			{
				drawRect(&pos, SDL_MapRGB(mainsurface->format, 128, 0, 0), 128);
			}
			pos.w = tmpw;
		}
		if ( HPDetails.enemy_hp > 0 )
		{
			int tmpw = pos.w;
			pos.w = pos.w * ((double)HPDetails.enemy_hp / HPDetails.enemy_maxhp);
			drawRect(&pos, SDL_MapRGB(mainsurface->format, 128, 0, 0), 255);
			if ( HPDetails.enemy_bar_color > 0 )
			{
				drawRect(&pos, HPDetails.enemy_bar_color, 224);
			}
			pos.w = tmpw;
		}

		// name
		int x = players[player]->camera_midx() - longestline(HPDetails.enemy_name) * TTF12_WIDTH / 2 + 2;
		int y = pos.y + 16 - TTF12_HEIGHT / 2 + 2;
		ttfPrintText(ttf12, x, y, HPDetails.enemy_name);
	}
}

void EnemyHPDamageBarHandler::addEnemyToList(Sint32 HP, Sint32 maxHP, Sint32 oldHP, Uint32 color, Uint32 uid, char* name, bool isLowPriority)
{
	auto find = HPBars.find(uid);
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
	}
	else
	{
		HPBars.insert(std::make_pair(uid, EnemyHPDetails(HP, maxHP, oldHP, color, name, isLowPriority)));
	}
}

SDL_Rect getRectForSkillIcon(const int skill)
{
	SDL_Rect defaultRect{ 0, 0, 0, 0 };

	int glyphHeight = 32;
	int glyphWidth = 32;
	const int glyphSpacing = 32;

	switch ( skill )
	{
		case PRO_LOCKPICKING:
			return SDL_Rect{ 1 * glyphSpacing, 3 * glyphSpacing, glyphWidth, glyphHeight };
		case PRO_STEALTH:
			return SDL_Rect{ 1 * glyphSpacing, 2 * glyphSpacing, glyphWidth, glyphHeight };
		case PRO_TRADING:
			return SDL_Rect{ 2 * glyphSpacing, 3 * glyphSpacing, glyphWidth, glyphHeight };
		case PRO_APPRAISAL:
			return SDL_Rect{ 1 * glyphSpacing, 0 * glyphSpacing, glyphWidth, glyphHeight };
		case PRO_SWIMMING:
			return SDL_Rect{ 2 * glyphSpacing, 2 * glyphSpacing, glyphWidth, glyphHeight };
		case PRO_LEADERSHIP:
			return SDL_Rect{ 1 * glyphSpacing, 1 * glyphSpacing, glyphWidth, glyphHeight };
		case PRO_SPELLCASTING:
			return SDL_Rect{ 0 * glyphSpacing, 1 * glyphSpacing, glyphWidth, glyphHeight };
		case PRO_MAGIC:
			return SDL_Rect{ 3 * glyphSpacing, 1 * glyphSpacing, glyphWidth, glyphHeight };
		case PRO_RANGED:
			return SDL_Rect{ 3 * glyphSpacing, 2 * glyphSpacing, glyphWidth, glyphHeight };
		case PRO_SWORD:
			return SDL_Rect{ 0 * glyphSpacing, 3 * glyphSpacing, glyphWidth, glyphHeight };
		case PRO_MACE:
			return SDL_Rect{ 2 * glyphSpacing, 1 * glyphSpacing, glyphWidth, glyphHeight };
		case PRO_AXE:
			return SDL_Rect{ 2 * glyphSpacing, 0 * glyphSpacing, glyphWidth, glyphHeight };
		case PRO_POLEARM:
			return SDL_Rect{ 0 * glyphSpacing, 2 * glyphSpacing, glyphWidth, glyphHeight };
		case PRO_UNARMED:
			return SDL_Rect{ 3 * glyphSpacing, 3 * glyphSpacing, glyphWidth, glyphHeight };
		case PRO_SHIELD:
			return SDL_Rect{ 3 * glyphSpacing, 0 * glyphSpacing, glyphWidth, glyphHeight };
		case PRO_ALCHEMY:
			return SDL_Rect{ 0 * glyphSpacing, 0 * glyphSpacing, glyphWidth, glyphHeight };
		default:
			break;
	}

	return defaultRect;
}