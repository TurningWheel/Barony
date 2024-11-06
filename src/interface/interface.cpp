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
#include "../collision.hpp"

Uint32 svFlags = 30;
Uint32 settings_svFlags = svFlags;
view_t camera_charsheet;
real_t camera_charsheet_offsetyaw = (330) * PI / 180;

bool keepInventoryGlobal = false;


list_t chestInv[MAXPLAYERS];

//SDL_Surface* font12x12_small_bmp = NULL;
//bool gui_clickdrag[MAXPLAYERS] = { false };
//int dragoffset_x[MAXPLAYERS] = { 0 };
//int dragoffset_y[MAXPLAYERS] = { 0 };
//int textscroll = 0;
//int inventorycategory = 7; // inventory window defaults to wildcard
//int itemscroll = 0;
//SDL_Surface* inventoryChest_bmp = NULL;
//SDL_Surface* invclose_bmp = NULL;
//SDL_Surface* invgraball_bmp = NULL;
//SDL_Surface* button_bmp = NULL, *smallbutton_bmp = NULL, *invup_bmp = NULL, *invdown_bmp = NULL;
//SDL_Surface* backdrop_blessed_bmp = nullptr;
//SDL_Surface* backdrop_cursed_bmp = nullptr;
//SDL_Surface* status_bmp = nullptr;
//SDL_Surface* character_bmp = nullptr;
//SDL_Surface* hunger_bmp = nullptr;
//SDL_Surface* hunger_blood_bmp = nullptr;
//SDL_Surface* hunger_boiler_bmp = nullptr;
//SDL_Surface* hunger_boiler_hotflame_bmp = nullptr;
//SDL_Surface* hunger_boiler_flame_bmp = nullptr;
//SDL_Surface* minotaur_bmp = nullptr;
//SDL_Surface* rightsidebar_titlebar_img = NULL;
//SDL_Surface* rightsidebar_slot_img = NULL;
//SDL_Surface* rightsidebar_slot_highlighted_img = NULL;
//SDL_Surface* rightsidebar_slot_grayedout_img = NULL;
//int rightsidebar_height = 0;

//SDL_Surface* bookgui_img = NULL;
//SDL_Surface* book_highlighted_left_img = NULL;
//SDL_Surface* book_highlighted_right_img = NULL;

//SDL_Surface* magicspellList_bmp = NULL;
//SDL_Surface* spell_list_titlebar_bmp = NULL;
//SDL_Surface* spell_list_gui_slot_bmp = NULL;
//SDL_Surface* spell_list_gui_slot_highlighted_bmp = NULL;
//SDL_Surface* textup_bmp = NULL;
//SDL_Surface* textdown_bmp = NULL;
//SDL_Surface* attributesleft_bmp = NULL;
//SDL_Surface* attributesright_bmp = NULL;
//SDL_Surface* attributesleftunclicked_bmp = NULL;
//SDL_Surface* attributesrightunclicked_bmp = NULL;
//SDL_Surface* inventory_bmp = NULL, *inventoryoption_bmp = NULL, *inventoryoptionChest_bmp = NULL, *equipped_bmp = NULL;
//SDL_Surface* itembroken_bmp = nullptr;
//SDL_Surface *category_bmp[NUMCATEGORIES];
//SDL_Surface* shopkeeper_bmp = NULL;
//SDL_Surface* shopkeeper2_bmp = NULL;
//SDL_Surface* damage_bmp = NULL;
//SDL_Surface *str_bmp64u = NULL;
//SDL_Surface *dex_bmp64u = NULL;
//SDL_Surface *con_bmp64u = NULL;
//SDL_Surface *int_bmp64u = NULL;
//SDL_Surface *per_bmp64u = NULL;
//SDL_Surface *chr_bmp64u = NULL;
//SDL_Surface *str_bmp64 = NULL;
//SDL_Surface *dex_bmp64 = NULL;
//SDL_Surface *con_bmp64 = NULL;
//SDL_Surface *int_bmp64 = NULL;
//SDL_Surface *per_bmp64 = NULL;
//SDL_Surface *chr_bmp64 = NULL;
//SDL_Surface *sidebar_lock_bmp = nullptr;
//SDL_Surface *sidebar_unlock_bmp = nullptr;
//SDL_Surface *effect_drunk_bmp = nullptr;
//SDL_Surface *effect_polymorph_bmp = nullptr;
//SDL_Surface *effect_hungover_bmp = nullptr;
//int spellscroll = 0;
//int magicspell_list_offset_x = 0;
//int magicspell_list_offset_y = 0;
//bool dragging_magicspell_list_GUI = false;
//int magic_GUI_state = 0;
//SDL_Rect magic_gui_pos;
//SDL_Surface* sustained_spell_generic_icon = NULL;
//SDL_Surface* hotbar_img = NULL;
//SDL_Surface* hotbar_spell_img = NULL;

int buttonclick = 0;


bool auto_hotbar_new_items = true;
bool auto_hotbar_categories[NUM_HOTBAR_CATEGORIES] = {	true, true, true, true, 
														true, true, true, true,
														true, true, true, true };
int autosort_inventory_categories[NUM_AUTOSORT_CATEGORIES] = {	0, 0, 0, 0,
																0, 0, 0, 0,
																0, 0, 0, 0 };
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
const int inscriptionSlotHeight = 40;

DamageIndicatorHandler_t DamageIndicatorHandler;
EnemyHPDamageBarHandler enemyHPDamageBarHandler[MAXPLAYERS];
FollowerRadialMenu FollowerMenu[MAXPLAYERS];
CalloutRadialMenu CalloutMenu[MAXPLAYERS];
GenericGUIMenu GenericGUI[MAXPLAYERS];

bool EnemyHPDamageBarHandler::bDamageGibTypesEnabled = true;
std::map<int, std::vector<int>> EnemyHPDamageBarHandler::damageGibAnimCurves;
int EnemyHPDamageBarHandler::maxTickLifetime = 120;
int EnemyHPDamageBarHandler::maxTickFurnitureLifetime = 60;
int EnemyHPDamageBarHandler::shortDistanceHPBarFadeTicks = TICKS_PER_SECOND / 2;
real_t EnemyHPDamageBarHandler::shortDistanceHPBarFadeDistance = 1.0;
std::vector<std::pair<real_t, int>> EnemyHPDamageBarHandler::widthHealthBreakpointsMonsters;
std::vector<std::pair<real_t, int>> EnemyHPDamageBarHandler::widthHealthBreakpointsFurniture;

std::vector<std::pair<SDL_Surface**, std::string>> systemResourceImages =
{
	std::make_pair(&font8x8_bmp, "images/system/font8x8.png"),
	std::make_pair(&font12x12_bmp, "images/system/font12x12.png"),
	std::make_pair(&font16x16_bmp, "images/system/font16x16.png"),

	//std::make_pair(&font12x12_small_bmp, "images/system/font12x12_small.png"),
	//std::make_pair(&backdrop_blessed_bmp, "images/system/backdrop_blessed.png"),
	//std::make_pair(&backdrop_cursed_bmp, "images/system/backdrop_cursed.png"),
	//std::make_pair(&button_bmp, "images/system/ButtonHighlighted.png"),
	//std::make_pair(&smallbutton_bmp, "images/system/SmallButtonHighlighted.png"),
	//std::make_pair(&invup_bmp, "images/system/InventoryUpHighlighted.png"),
	//std::make_pair(&invdown_bmp, "images/system/InventoryDownHighlighted.png"),
	//std::make_pair(&status_bmp, "images/system/StatusBar.png"),
	//std::make_pair(&character_bmp, "images/system/CharacterSheet.png"),
	//std::make_pair(&hunger_bmp, "images/system/Hunger.png"),
	//std::make_pair(&hunger_blood_bmp, "images/system/Hunger_blood.png"),
	//std::make_pair(&hunger_boiler_bmp, "images/system/Hunger_boiler.png"),
	//std::make_pair(&hunger_boiler_hotflame_bmp, "images/system/Hunger_boiler_hotfire.png"),
	//std::make_pair(&hunger_boiler_flame_bmp, "images/system/Hunger_boiler_fire.png"),
	//std::make_pair(&minotaur_bmp, "images/system/minotaur.png"),
	//std::make_pair(&attributesleft_bmp, "images/system/AttributesLeftHighlighted.png"),
	//std::make_pair(&attributesright_bmp, "images/system/AttributesRightHighlighted.png"),

	//General GUI images.
	//std::make_pair(&attributesleftunclicked_bmp, "images/system/AttributesLeft.png"),
	//std::make_pair(&attributesrightunclicked_bmp, "images/system/AttributesRight.png"),
	//std::make_pair(&shopkeeper_bmp, "images/system/shopkeeper.png"),
	//std::make_pair(&shopkeeper2_bmp, "images/system/shopkeeper2.png"),
	//std::make_pair(&damage_bmp, "images/system/damage.png"),

	//Magic GUI images.
	//std::make_pair(&magicspellList_bmp, "images/system/spellList.png"),
	//std::make_pair(&spell_list_titlebar_bmp, "images/system/spellListTitlebar.png"),
	//std::make_pair(&spell_list_gui_slot_bmp, "images/system/spellListSlot.png"),
	//std::make_pair(&spell_list_gui_slot_highlighted_bmp, "images/system/spellListSlotHighlighted.png"),
	//std::make_pair(&sustained_spell_generic_icon, "images/system/magic/channeled_spell.png"),

	// inventory GUI images.
	//std::make_pair(&inventory_bmp, "images/system/Inventory.png"),
	//std::make_pair(&inventoryoption_bmp, "images/system/InventoryOption.png"),
	//std::make_pair(&inventory_mode_item_img, "images/system/inventory_mode_item.png"),
	//std::make_pair(&inventory_mode_item_highlighted_img, "images/system/inventory_mode_item_highlighted.png"),
	//std::make_pair(&inventory_mode_spell_img, "images/system/inventory_mode_spell.png"),
	//std::make_pair(&inventory_mode_spell_highlighted_img, "images/system/inventory_mode_spell_highlighted.png"),
	//std::make_pair(&equipped_bmp, "images/system/Equipped.png"),
	//std::make_pair(&itembroken_bmp, "images/system/Broken.png"),

	//Chest images..
	//std::make_pair(&inventoryChest_bmp, "images/system/InventoryChest.png"),
	//std::make_pair(&inventoryoptionChest_bmp, "images/system/InventoryOptionChest.png"),
	//std::make_pair(&invclose_bmp, "images/system/InventoryCloseHighlighted.png"),
	//std::make_pair(&invgraball_bmp, "images/system/InventoryChestGraballHighlighted.png"),

	//Identify GUI images...
	//std::make_pair(&identifyGUI_img, "images/system/identifyGUI.png"),
	//std::make_pair(&rightsidebar_slot_grayedout_img, "images/system/rightSidebarSlotGrayedOut.png"),
	//std::make_pair(&bookgui_img, "images/system/book.png"),
	//std::make_pair(&book_highlighted_left_img, "images/system/bookpageleft-highlighted.png"),
	//std::make_pair(&book_highlighted_right_img, "images/system/bookpageright-highlighted.png"),

	//Levelup images.
	//std::make_pair(&str_bmp64u, "images/system/str64u.png"),
	//std::make_pair(&dex_bmp64u, "images/system/dex64u.png"),
	//std::make_pair(&con_bmp64u, "images/system/con64u.png"),
	//std::make_pair(&int_bmp64u, "images/system/int64u.png"),
	//std::make_pair(&per_bmp64u, "images/system/per64u.png"),
	//std::make_pair(&chr_bmp64u, "images/system/chr64u.png"),
	//std::make_pair(&str_bmp64, "images/system/str64.png"),
	//std::make_pair(&dex_bmp64, "images/system/dex64.png"),
	//std::make_pair(&con_bmp64, "images/system/con64.png"),
	//std::make_pair(&int_bmp64, "images/system/int64.png"),
	//std::make_pair(&per_bmp64, "images/system/per64.png"),
	//std::make_pair(&chr_bmp64, "images/system/chr64.png"),

	//Misc GUI images.
	//std::make_pair(&sidebar_lock_bmp, "images/system/locksidebar.png"),
	//std::make_pair(&sidebar_unlock_bmp, "images/system/unlocksidebar.png"),
	//std::make_pair(&hotbar_img, "images/system/hotbar_slot.png"),
	//std::make_pair(&hotbar_spell_img, "images/system/magic/hotbar_spell.png"),

	//Misc effect images.
	//std::make_pair(&effect_drunk_bmp, "images/system/drunk.png"),
	//std::make_pair(&effect_polymorph_bmp, "images/system/polymorph.png"),
	//std::make_pair(&effect_hungover_bmp, "images/system/hungover.png")
};

bool loadInterfaceResources()
{
	//General GUI images.
	//font12x12_small_bmp = loadImage("images/system/font12x12_small.png");
	//backdrop_blessed_bmp = loadImage("images/system/backdrop_blessed.png");
	//backdrop_cursed_bmp = loadImage("images/system/backdrop_cursed.png");
	//button_bmp = loadImage("images/system/ButtonHighlighted.png");
	//smallbutton_bmp = loadImage("images/system/SmallButtonHighlighted.png");
	//invup_bmp = loadImage("images/system/InventoryUpHighlighted.png");
	//invdown_bmp = loadImage("images/system/InventoryDownHighlighted.png");
	//status_bmp = loadImage("images/system/StatusBar.png");
	//character_bmp = loadImage("images/system/CharacterSheet.png");
	//hunger_bmp = loadImage("images/system/Hunger.png");
	//hunger_blood_bmp = loadImage("images/system/Hunger_blood.png");
	//hunger_boiler_bmp = loadImage("images/system/Hunger_boiler.png");
	//hunger_boiler_hotflame_bmp = loadImage("images/system/Hunger_boiler_hotfire.png");
	//hunger_boiler_flame_bmp = loadImage("images/system/Hunger_boiler_fire.png");
	//minotaur_bmp = loadImage("images/system/minotaur.png"); // the file "images/system/minotaur.png" doesn't exist in current Data
	//textup_bmp = loadImage("images/system/TextBoxUpHighlighted.png");
	//textdown_bmp = loadImage("images/system/TextBoxDownHighlighted.png");
	//attributesleft_bmp = loadImage("images/system/AttributesLeftHighlighted.png");
	//attributesright_bmp = loadImage("images/system/AttributesRightHighlighted.png");
	//attributesleftunclicked_bmp = loadImage("images/system/AttributesLeft.png");
	//attributesrightunclicked_bmp = loadImage("images/system/AttributesRight.png");
	//shopkeeper_bmp = loadImage("images/system/shopkeeper.png");
	//shopkeeper2_bmp = loadImage("images/system/shopkeeper2.png");
	//damage_bmp = loadImage("images/system/damage.png");

	//Magic GUI images.
	//magicspellList_bmp = loadImage("images/system/spellList.png");
	//spell_list_titlebar_bmp = loadImage("images/system/spellListTitlebar.png");
	//spell_list_gui_slot_bmp = loadImage("images/system/spellListSlot.png");
	//spell_list_gui_slot_highlighted_bmp = loadImage("images/system/spellListSlotHighlighted.png");
	//sustained_spell_generic_icon = loadImage("images/system/magic/channeled_spell.png");
	//inventory_bmp = loadImage("images/system/Inventory.png");
	//inventoryoption_bmp = loadImage("images/system/InventoryOption.png");
	//inventory_mode_item_img = loadImage("images/system/inventory_mode_item.png");
	//inventory_mode_item_highlighted_img = loadImage("images/system/inventory_mode_item_highlighted.png");
	//inventory_mode_spell_img = loadImage("images/system/inventory_mode_spell.png");
	//inventory_mode_spell_highlighted_img = loadImage("images/system/inventory_mode_spell_highlighted.png");
	//equipped_bmp = loadImage("images/system/Equipped.png");
	//itembroken_bmp = loadImage("images/system/Broken.png");
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
	//inventoryChest_bmp = loadImage("images/system/InventoryChest.png");
	//inventoryoptionChest_bmp = loadImage("images/system/InventoryOptionChest.png");
	//invclose_bmp = loadImage("images/system/InventoryCloseHighlighted.png");
	//invgraball_bmp = loadImage("images/system/InventoryChestGraballHighlighted.png");

	//Identify GUI images...
	//identifyGUI_img = loadImage("images/system/identifyGUI.png");

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
	//rightsidebar_titlebar_img = spell_list_titlebar_bmp;
	//rightsidebar_slot_img = spell_list_gui_slot_bmp;
	//rightsidebar_slot_highlighted_img = spell_list_gui_slot_highlighted_bmp;
	//rightsidebar_slot_grayedout_img = loadImage("images/system/rightSidebarSlotGrayedOut.png");

	//bookgui_img = loadImage("images/system/book.png");
	//nextpage_img = loadImage("images/system/nextpage.png");
	//previouspage_img = loadImage("images/system/previouspage.png");
	//bookclose_img = loadImage("images/system/bookclose.png");

	//book_highlighted_left_img = loadImage("images/system/bookpageleft-highlighted.png");
	//book_highlighted_right_img = loadImage("images/system/bookpageright-highlighted.png");

	//str_bmp64u = loadImage("images/system/str64u.png");
	//dex_bmp64u = loadImage("images/system/dex64u.png");
	//con_bmp64u = loadImage("images/system/con64u.png");
	//int_bmp64u = loadImage("images/system/int64u.png");
	//per_bmp64u = loadImage("images/system/per64u.png");
	//chr_bmp64u = loadImage("images/system/chr64u.png");
	//str_bmp64 = loadImage("images/system/str64.png");
	//dex_bmp64 = loadImage("images/system/dex64.png");
	//con_bmp64 = loadImage("images/system/con64.png");
	//int_bmp64 = loadImage("images/system/int64.png");
	//per_bmp64 = loadImage("images/system/per64.png");
	//chr_bmp64 = loadImage("images/system/chr64.png");

	//sidebar_lock_bmp = loadImage("images/system/locksidebar.png");
	//sidebar_unlock_bmp = loadImage("images/system/unlocksidebar.png");
	//hotbar_img = loadImage("images/system/hotbar_slot.png");
	//hotbar_spell_img = loadImage("images/system/magic/hotbar_spell.png");

	//effect_drunk_bmp = loadImage("images/system/drunk.png");
	//effect_polymorph_bmp = loadImage("images/system/polymorph.png");
	//effect_hungover_bmp = loadImage("images/system/hungover.png");

	return true;
}

void freeInterfaceResources()
{
	//int c;

	/*if (font12x12_small_bmp)
	{
		SDL_FreeSurface(font12x12_small_bmp);
	}*/
	//if ( backdrop_blessed_bmp )
	//{
	//	SDL_FreeSurface(backdrop_blessed_bmp);
	//}
	//if ( backdrop_cursed_bmp )
	//{
	//	SDL_FreeSurface(backdrop_cursed_bmp);
	//}
	//if (status_bmp)
	//{
	//	SDL_FreeSurface(status_bmp);
	//}
	//if (character_bmp)
	//{
	//	SDL_FreeSurface(character_bmp);
	//}
	//if (hunger_bmp)
	//{
	//	SDL_FreeSurface(hunger_bmp);
	//}
	//if ( hunger_blood_bmp )
	//{
	//	SDL_FreeSurface(hunger_blood_bmp);
	//}
	//if ( hunger_boiler_bmp )
	//{
	//	SDL_FreeSurface(hunger_boiler_bmp);
	//}
	//if ( hunger_boiler_hotflame_bmp )
	//{
	//	SDL_FreeSurface(hunger_boiler_hotflame_bmp);
	//}
	//if ( hunger_boiler_flame_bmp )
	//{
	//	SDL_FreeSurface(hunger_boiler_flame_bmp);
	//}
	//if ( minotaur_bmp )
	//{
	//	SDL_FreeSurface(minotaur_bmp);
	//}
	////if(textup_bmp)
	////SDL_FreeSurface(textup_bmp);
	////if(textdown_bmp)
	////SDL_FreeSurface(textdown_bmp);
	//if (attributesleft_bmp)
	//{
	//	SDL_FreeSurface(attributesleft_bmp);
	//}
	//if (attributesright_bmp)
	//{
	//	SDL_FreeSurface(attributesright_bmp);
	//}
	//if (attributesleftunclicked_bmp)
	//{
	//	SDL_FreeSurface(attributesleftunclicked_bmp);
	//}
	//if (attributesrightunclicked_bmp)
	//{
	//	SDL_FreeSurface(attributesrightunclicked_bmp);
	//}
	//if (magicspellList_bmp)
	//{
	//	SDL_FreeSurface(magicspellList_bmp);
	//}
	//if (spell_list_titlebar_bmp)
	//{
	//	SDL_FreeSurface(spell_list_titlebar_bmp);
	//}
	//if (spell_list_gui_slot_bmp)
	//{
	//	SDL_FreeSurface(spell_list_gui_slot_bmp);
	//}
	//if (spell_list_gui_slot_highlighted_bmp)
	//{
	//	SDL_FreeSurface(spell_list_gui_slot_highlighted_bmp);
	//}
	//if (sustained_spell_generic_icon)
	//{
	//	SDL_FreeSurface(sustained_spell_generic_icon);
	//}
	//if (invup_bmp != NULL)
	//{
	//	SDL_FreeSurface(invup_bmp);
	//}
	//if (invdown_bmp != NULL)
	//{
	//	SDL_FreeSurface(invdown_bmp);
	//}
	//if (inventory_bmp != NULL)
	//{
	//	SDL_FreeSurface(inventory_bmp);
	//}
	//if (inventoryoption_bmp != NULL)
	//{
	//	SDL_FreeSurface(inventoryoption_bmp);
	//}
	//if (inventory_mode_item_img)
	//{
	//	SDL_FreeSurface(inventory_mode_item_img);
	//}
	//if (inventory_mode_item_highlighted_img)
	//{
	//	SDL_FreeSurface(inventory_mode_item_highlighted_img);
	//}
	//if (inventory_mode_spell_img)
	//{
	//	SDL_FreeSurface(inventory_mode_spell_img);
	//}
	//if (inventory_mode_spell_highlighted_img)
	//{
	//	SDL_FreeSurface(inventory_mode_spell_highlighted_img);
	//}
	//if (button_bmp != NULL)
	//{
	//	SDL_FreeSurface(button_bmp);
	//}
	//if (smallbutton_bmp != NULL)
	//{
	//	SDL_FreeSurface(smallbutton_bmp);
	//}
	//if (equipped_bmp != NULL)
	//{
	//	SDL_FreeSurface(equipped_bmp);
	//}
	//if ( itembroken_bmp != nullptr )
	//{
	//	SDL_FreeSurface(itembroken_bmp);
	//}
	//if (inventoryChest_bmp != NULL)
	//{
	//	SDL_FreeSurface(inventoryChest_bmp);
	//}
	//if (invclose_bmp != NULL)
	//{
	//	SDL_FreeSurface(invclose_bmp);
	//}
	//if (invgraball_bmp != NULL)
	//{
	//	SDL_FreeSurface(invgraball_bmp);
	//}
	//if (inventoryoptionChest_bmp != NULL)
	//{
	//	SDL_FreeSurface(inventoryoptionChest_bmp);
	//}
	//if (shopkeeper_bmp != NULL)
	//{
	//	SDL_FreeSurface(shopkeeper_bmp);
	//}
	//if ( shopkeeper2_bmp != NULL )
	//{
	//	SDL_FreeSurface(shopkeeper2_bmp);
	//}
	//if (damage_bmp != NULL)
	//{
	//	SDL_FreeSurface(damage_bmp);
	//}
	//for( c=0; c<NUMCATEGORIES; c++ )
	//if(category_bmp[c]!=NULL)
	//SDL_FreeSurface(category_bmp[c]);
	/*if (identifyGUI_img != NULL)
	{
		SDL_FreeSurface(identifyGUI_img);
	}*/
	/*if (rightsidebar_titlebar_img)
		SDL_FreeSurface(rightsidebar_titlebar_img);
	if (rightsidebar_slot_img)
		SDL_FreeSurface(rightsidebar_slot_img);
	if (rightsidebar_slot_highlighted_img)
		SDL_FreeSurface(rightsidebar_slot_highlighted_img);*/
	/*if (rightsidebar_slot_grayedout_img)
	{
		SDL_FreeSurface(rightsidebar_slot_grayedout_img);
	}
	if (bookgui_img)
	{
		SDL_FreeSurface(bookgui_img);
	}*/
	//if (nextpage_img)
	//SDL_FreeSurface(nextpage_img);
	//if (previouspage_img)
	//SDL_FreeSurface(previouspage_img);
	//if (bookclose_img)
	//SDL_FreeSurface(bookclose_img);
	/*if (book_highlighted_left_img)
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
	}*/
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

static void genericgui_deselect_fn(Widget& widget) {
	if ( widget.isSelected() )
	{
		if ( !inputs.getVirtualMouse(widget.getOwner())->draw_cursor )
		{
			widget.deselect();
		}
	}
}

/*-------------------------------------------------------------------------------

	saveCommand

	saves a command to the command history

-------------------------------------------------------------------------------*/

void saveCommand(char* content)
{
	newString(&command_history, 0xFFFFFFFF, ticks, -1, content);
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
		printlog("note: config file '%s' does not exist!\n", filename);
		defaultConfig(); //Set up the game with the default config.
		return 0;
	}

	// read commands from it
	while ( fp->gets2(str, 1024) != NULL )
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
	// deprecated
    return nullptr;
}

Sint8* inputPressedForPlayer(int player, Uint32 scancode)
{
    // deprecated
    return nullptr;
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
		case MODULE_SIGN_VIEW:
		case MODULE_SKILLS_LIST:
		case MODULE_LOG:
		case MODULE_MAP:
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
			if ( dropdownMenu.bOpen 
				&& (dropdownMenu.currentName.find("paper_doll") != std::string::npos) )
			{
				return false;
			}
			return true;
		case MODULE_HOTBAR:
		case MODULE_SHOP:
		case MODULE_CHEST:
		case MODULE_REMOVECURSE:
		case MODULE_IDENTIFY:
		case MODULE_TINKERING:
		case MODULE_FEATHER:
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
	else if ( activeModule == MODULE_FEATHER )
	{
		auto& featherGUI = GenericGUI[player.playernum].featherGUI;
		auto& inventoryUI = player.inventoryUI;
		if ( featherGUI.warpMouseToSelectedFeatherItem(nullptr, (Inputs::SET_CONTROLLER))
			&& inventoryUI.cursor.queuedModule == Player::GUI_t::MODULE_NONE )
		{
			if ( auto slot = featherGUI.getFeatherSlotFrame(featherGUI.getSelectedFeatherSlotX(), featherGUI.getSelectedFeatherSlotY()) )
			{
				SDL_Rect pos = slot->getAbsoluteSize();
				pos.x -= player.camera_virtualx1();
				pos.y -= player.camera_virtualy1();
				inventoryUI.updateSelectedSlotAnimation(pos.x, pos.y,
					inventoryUI.getSlotSize(), inscriptionSlotHeight, moveCursorInstantly);
			}
		}
		return true;
	}
	else if ( activeModule == MODULE_HOTBAR )
	{
		auto& inventoryUI = player.inventoryUI;
		if ( warpMouseToSelectedHotbarSlot(player.playernum)
			&& inventoryUI.cursor.queuedModule == Player::GUI_t::MODULE_NONE )
		{
			if ( auto hotbarSlotFrame = player.hotbar.getHotbarSlotFrame(player.hotbar.current_hotbar) )
			{
				SDL_Rect pos = hotbarSlotFrame->getAbsoluteSize();
				pos.x -= player.camera_virtualx1();
				pos.y -= player.camera_virtualy1();
				inventoryUI.updateSelectedSlotAnimation(pos.x, pos.y,
					inventoryUI.getSlotSize(), inventoryUI.getSlotSize(), moveCursorInstantly);
			}
		}
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
					|| oldModule == MODULE_TINKERING
					|| oldModule == MODULE_FEATHER)
				&& !(activeModule == MODULE_INVENTORY 
					|| activeModule == MODULE_HOTBAR 
					|| activeModule == MODULE_SPELLS
					|| activeModule == MODULE_CHEST
					|| activeModule == MODULE_SHOP
					|| activeModule == MODULE_ALCHEMY
					|| activeModule == MODULE_TINKERING
					|| activeModule == MODULE_FEATHER)
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
				|| activeModule == MODULE_TINKERING
				|| activeModule == MODULE_FEATHER)
				&& !(oldModule == MODULE_INVENTORY 
					|| oldModule == MODULE_HOTBAR 
					|| oldModule == MODULE_SPELLS
					|| oldModule == MODULE_CHEST
					|| oldModule == MODULE_SHOP
					|| oldModule == MODULE_ALCHEMY
					|| oldModule == MODULE_TINKERING
					|| oldModule == MODULE_FEATHER))
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
	if ( whichGUIMode != GUI_MODE_NONE && whichGUIMode != GUI_MODE_CALLOUT )
	{
		CalloutMenu[playernum].closeCalloutMenuGUI();
	}
	GenericGUI[playernum].closeGUI();
	if ( minimap.mapWindow )
	{
		minimap.mapWindow->removeSelf();
		minimap.mapWindow = nullptr;
	}
	if ( messageZone.logWindow )
	{
		messageZone.logWindow->removeSelf();
		messageZone.logWindow = nullptr;
	}

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
		&& !FollowerMenu[playernum].followerToCommand
		&& !CalloutMenu[playernum].bOpen )
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
	bool oldShootmode = shootmode;
	GenericGUI[playernum].closeGUI();
	if ( whatToClose != CLOSEGUI_DONT_CLOSE_FOLLOWERGUI )
	{
		FollowerMenu[playernum].closeFollowerMenuGUI();
	}
	if ( whatToClose != CLOSEGUI_DONT_CLOSE_CALLOUTGUI )
	{
		CalloutMenu[playernum].closeCalloutMenuGUI();
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
	if ( whatToClose != CLOSEGUI_DONT_CLOSE_INVENTORY )
	{
		inventoryUI.closeInventory();
	}
	skillSheet.closeSkillSheet();
	bookGUI.closeBookGUI();
	if ( signGUI.bSignOpen )
	{
		signGUI.closeSignGUI();
	}

	if ( minimap.mapWindow )
	{
		minimap.mapWindow->removeSelf();
		minimap.mapWindow = nullptr;
	}
	if ( messageZone.logWindow )
	{
		messageZone.logWindow->removeSelf();
		messageZone.logWindow = nullptr;
	}
	hud.closeStatusFxWindow();

	if ( shootmodeAction == CLOSEGUI_ENABLE_SHOOTMODE )
	{
		inputs.getUIInteraction(playernum)->selectedItem = nullptr;
		inputs.getUIInteraction(playernum)->selectedItemFromChest = 0;
		inputs.getUIInteraction(playernum)->toggleclick = false;
		GUI.closeDropdowns();
		shootmode = true;
	}
	else if ( shootmodeAction == DONT_CHANGE_SHOOTMODE )
	{
		shootmode = oldShootmode; // just in case any previous actions modified shootmode
	}
	if (gameUIFrame[playernum]) {
	    gameUIFrame[playernum]->deselect();
	}
}

void FollowerRadialMenu::initfollowerMenuGUICursor(bool openInventory)
{
	bool oldshootmode = players[gui_player]->shootmode;
	if ( openInventory )
	{
		//players[gui_player]->openStatusScreen(GUI_MODE_INVENTORY, INVENTORY_MODE_ITEM);
		players[gui_player]->closeAllGUIs(DONT_CHANGE_SHOOTMODE, CLOSEGUI_DONT_CLOSE_FOLLOWERGUI);
		players[gui_player]->openStatusScreen(GUI_MODE_FOLLOWERMENU, INVENTORY_MODE_ITEM);
	}

	if ( !oldshootmode )
	{
		Uint32 flags = (Inputs::SET_MOUSE | Inputs::SET_CONTROLLER | Inputs::UNSET_RELATIVE_MOUSE);
		inputs.warpMouse(gui_player, 
			players[gui_player]->camera_x1() + (players[gui_player]->camera_width() / 2),
			players[gui_player]->camera_y1() + (players[gui_player]->camera_height() / 2), flags);
	}

	inputs.setMouse(gui_player, Inputs::OX, inputs.getMouse(gui_player, Inputs::X));
	inputs.setMouse(gui_player, Inputs::OY, inputs.getMouse(gui_player, Inputs::Y));
	
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
	if ( followerFrame )
	{
		followerFrame->setDisabled(true);
		for ( auto f : followerFrame->getFrames() )
		{
			f->removeSelf();
		}
	}
	animTitle = 0.0;
	animWheel = 0.0;
	openedThisTick = 0;
	animInvalidAction = 0.0;
	animInvalidActionTicks = 0;
}

bool FollowerRadialMenu::followerMenuIsOpen()
{
	if ( selectMoveTo || followerToCommand != nullptr )
	{
		return true;
	}
	return false;
}

std::vector<FollowerRadialMenu::PanelEntry> FollowerRadialMenu::panelEntries;
std::vector<FollowerRadialMenu::PanelEntry> FollowerRadialMenu::panelEntriesAlternate;
std::map<std::string, FollowerRadialMenu::IconEntry> FollowerRadialMenu::iconEntries;
int FollowerRadialMenu::followerWheelButtonThickness = 70;
int FollowerRadialMenu::followerWheelRadius = 140;
int FollowerRadialMenu::followerWheelFrameOffsetX = 0;
int FollowerRadialMenu::followerWheelFrameOffsetY = 0;
int FollowerRadialMenu::followerWheelInnerCircleRadiusOffset = 0;
int FollowerRadialMenu::followerWheelInnerCircleRadiusOffsetAlternate = 0;
Uint32 followerTitleColor = 0xFFFFFFFF;
Uint32 followerTitleHighlightColor = 0xFFFFFFFF;
Uint32 followerBannerTextColor = 0xFFFFFFFF;
Uint32 followerBannerTextHighlightColor = 0xFFFFFFFF;
void FollowerRadialMenu::loadFollowerJSON()
{
	if ( !PHYSFS_getRealDir("/data/follower_wheel.json") )
	{
		printlog("[JSON]: Error: Could not find file: data/follower_wheel.json");
	}
	else
	{
		std::string inputPath = PHYSFS_getRealDir("/data/follower_wheel.json");
		inputPath.append("/data/follower_wheel.json");

		File* fp = FileIO::open(inputPath.c_str(), "rb");
		if ( !fp )
		{
			printlog("[JSON]: Error: Could not open json file %s", inputPath.c_str());
		}
		else
		{
			char buf[65536];
			int count = fp->read(buf, sizeof(buf[0]), sizeof(buf));
			buf[count] = '\0';
			rapidjson::StringStream is(buf);
			FileIO::close(fp);
			rapidjson::Document d;
			d.ParseStream(is);
			if ( !d.HasMember("version") )
			{
				printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			}
			else
			{
				if ( d.HasMember("panel_center_x_offset") )
				{
					FollowerRadialMenu::followerWheelFrameOffsetX = d["panel_center_x_offset"].GetInt();
				}
				if ( d.HasMember("panel_center_y_offset") )
				{
					FollowerRadialMenu::followerWheelFrameOffsetY = d["panel_center_y_offset"].GetInt();
				}
				if ( d.HasMember("panel_radius") )
				{
					FollowerRadialMenu::followerWheelRadius = d["panel_radius"].GetInt();
				}
				if ( d.HasMember("panel_button_thickness") )
				{
					FollowerRadialMenu::followerWheelButtonThickness = d["panel_button_thickness"].GetInt();
				}
				if ( d.HasMember("panel_inner_circle_radius_offset") )
				{
					FollowerRadialMenu::followerWheelInnerCircleRadiusOffset = d["panel_inner_circle_radius_offset"].GetInt();
				}
				if ( d.HasMember("panel_inner_circle_radius_offset_alternate") )
				{
					FollowerRadialMenu::followerWheelInnerCircleRadiusOffsetAlternate = d["panel_inner_circle_radius_offset_alternate"].GetInt();
				}
				if ( d.HasMember("colors") )
				{
					if ( d["colors"].HasMember("banner_default") )
					{
						followerBannerTextColor = makeColor(
							d["colors"]["banner_default"]["r"].GetInt(),
							d["colors"]["banner_default"]["g"].GetInt(),
							d["colors"]["banner_default"]["b"].GetInt(),
							d["colors"]["banner_default"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("banner_highlight_default") )
					{
						followerBannerTextHighlightColor = makeColor(
							d["colors"]["banner_highlight_default"]["r"].GetInt(),
							d["colors"]["banner_highlight_default"]["g"].GetInt(),
							d["colors"]["banner_highlight_default"]["b"].GetInt(),
							d["colors"]["banner_highlight_default"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("title") )
					{
						followerTitleColor = makeColor(
							d["colors"]["title"]["r"].GetInt(),
							d["colors"]["title"]["g"].GetInt(),
							d["colors"]["title"]["b"].GetInt(),
							d["colors"]["title"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("title_creature_highlight") )
					{
						followerTitleHighlightColor = makeColor(
							d["colors"]["title_creature_highlight"]["r"].GetInt(),
							d["colors"]["title_creature_highlight"]["g"].GetInt(),
							d["colors"]["title_creature_highlight"]["b"].GetInt(),
							d["colors"]["title_creature_highlight"]["a"].GetInt());
					}
				}
				if ( d.HasMember("panels") )
				{
					FollowerRadialMenu::panelEntries.clear();
					for ( rapidjson::Value::ConstValueIterator itr = d["panels"].Begin();
						itr != d["panels"].End(); ++itr )
					{
						FollowerRadialMenu::panelEntries.push_back(FollowerRadialMenu::PanelEntry());
						auto& entry = FollowerRadialMenu::panelEntries[FollowerRadialMenu::panelEntries.size() - 1];
						if ( (*itr).HasMember("x") )
						{
							entry.x = (*itr)["x"].GetInt();
						}
						if ( (*itr).HasMember("y") )
						{
							entry.y = (*itr)["y"].GetInt();
						}
						if ( (*itr).HasMember("path") )
						{
							entry.path = (*itr)["path"].GetString();
						}
						if ( (*itr).HasMember("path_locked") )
						{
							entry.path_locked = (*itr)["path_locked"].GetString();
						}
						if ( (*itr).HasMember("path_hover") )
						{
							entry.path_hover = (*itr)["path_hover"].GetString();
						}
						if ( (*itr).HasMember("path_locked_hover") )
						{
							entry.path_locked_hover = (*itr)["path_locked_hover"].GetString();
						}
						if ( (*itr).HasMember("icon_offset_x") )
						{
							entry.icon_offsetx = (*itr)["icon_offset_x"].GetInt();
						}
						if ( (*itr).HasMember("icon_offset_y") )
						{
							entry.icon_offsety = (*itr)["icon_offset_y"].GetInt();
						}
					}
				}
				if ( d.HasMember("panels_alternate") )
				{
					FollowerRadialMenu::panelEntriesAlternate.clear();
					for ( rapidjson::Value::ConstValueIterator itr = d["panels_alternate"].Begin();
						itr != d["panels_alternate"].End(); ++itr )
					{
						FollowerRadialMenu::panelEntriesAlternate.push_back(FollowerRadialMenu::PanelEntry());
						auto& entry = FollowerRadialMenu::panelEntriesAlternate[FollowerRadialMenu::panelEntriesAlternate.size() - 1];
						if ( (*itr).HasMember("x") )
						{
							entry.x = (*itr)["x"].GetInt();
						}
						if ( (*itr).HasMember("y") )
						{
							entry.y = (*itr)["y"].GetInt();
						}
						if ( (*itr).HasMember("path") )
						{
							entry.path = (*itr)["path"].GetString();
						}
						if ( (*itr).HasMember("path_locked") )
						{
							entry.path_locked = (*itr)["path_locked"].GetString();
						}
						if ( (*itr).HasMember("path_hover") )
						{
							entry.path_hover = (*itr)["path_hover"].GetString();
						}
						if ( (*itr).HasMember("path_locked_hover") )
						{
							entry.path_locked_hover = (*itr)["path_locked_hover"].GetString();
						}
						if ( (*itr).HasMember("icon_offset_x") )
						{
							entry.icon_offsetx = (*itr)["icon_offset_x"].GetInt();
						}
						if ( (*itr).HasMember("icon_offset_y") )
						{
							entry.icon_offsety = (*itr)["icon_offset_y"].GetInt();
						}
					}
				}
				if ( d.HasMember("icons") )
				{
					FollowerRadialMenu::iconEntries.clear();
					for ( rapidjson::Value::ConstValueIterator itr = d["icons"].Begin();
						itr != d["icons"].End(); ++itr )
					{
						std::string actionName = "";
						if ( (*itr).HasMember("action") )
						{
							actionName = (*itr)["action"].GetString();
						}
						if ( actionName == "" )
						{
							continue;
						}
						FollowerRadialMenu::iconEntries[actionName] = FollowerRadialMenu::IconEntry();
						FollowerRadialMenu::iconEntries[actionName].name = actionName;
						if ( (*itr).HasMember("id") )
						{
							FollowerRadialMenu::iconEntries[actionName].id = (*itr)["id"].GetInt();
						}
						if ( (*itr).HasMember("path") )
						{
							FollowerRadialMenu::iconEntries[actionName].path = (*itr)["path"].GetString();
						}
						if ( (*itr).HasMember("path_active") )
						{
							FollowerRadialMenu::iconEntries[actionName].path_active = (*itr)["path_active"].GetString();
						}
						if ( (*itr).HasMember("path_hover") )
						{
							FollowerRadialMenu::iconEntries[actionName].path_hover = (*itr)["path_hover"].GetString();
						}
						if ( (*itr).HasMember("path_active_hover") )
						{
							FollowerRadialMenu::iconEntries[actionName].path_active_hover = (*itr)["path_active_hover"].GetString();
						}
						if ( (*itr).HasMember("text_maps") )
						{
							for ( rapidjson::Value::ConstValueIterator itr2 = (*itr)["text_maps"].Begin();
								itr2 != (*itr)["text_maps"].End(); ++itr2 )
							{
								for ( rapidjson::Value::ConstMemberIterator itr3 = itr2->MemberBegin();
									itr3 != itr2->MemberEnd(); ++itr3 )
								{
									std::string mapKey = itr3->name.GetString();
									std::string mapText = itr3->value["text"].GetString();
									std::set<int> mapHighlights;
									for ( rapidjson::Value::ConstValueIterator highlightItr = itr3->value["word_highlights"].Begin();
										highlightItr != itr3->value["word_highlights"].End(); ++highlightItr )
									{
										mapHighlights.insert(highlightItr->GetInt());
									}
									FollowerRadialMenu::iconEntries[actionName].text_map[mapKey] = std::make_pair(mapText, mapHighlights);
								}
							}
						}
					}
				}
				printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
			}
		}
	}
}

bool FollowerRadialMenu::followerGUIHasBeenCreated() const
{
	if ( followerFrame )
	{
		if ( !followerFrame->getFrames().empty() )
		{
			for ( auto f : followerFrame->getFrames() )
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

void FollowerRadialMenu::createFollowerMenuGUI()
{
	const int player = getPlayer();
	if ( !gui || !followerFrame )
	{
		return;
	}
	if ( followerGUIHasBeenCreated() )
	{
		return;
	}

	const int midx = followerFrame->getSize().w / 2;
	const int midy = followerFrame->getSize().h / 2;

	auto bgFrame = followerFrame->addFrame("wheel base");
	bgFrame->setSize(SDL_Rect{0, 0, followerFrame->getSize().w, followerFrame->getSize().h});
	bgFrame->setHollow(false);
	bgFrame->setDisabled(false);
	bgFrame->setInheritParentFrameOpacity(false);
	bgFrame->setOpacity(0.0);

	const char* font = "fonts/pixel_maz_multiline.ttf#16#2";

	int panelIndex = 0;
	for ( auto& entry : panelEntries )
	{
		if ( panelIndex < PANEL_DIRECTION_END )
		{
			SDL_Rect pos{ entry.x + midx, entry.y + midy, 0, 0 };
			char buf[32] = "";
			snprintf(buf, sizeof(buf), "panel %d", panelIndex);
			Frame::image_t* img = bgFrame->addImage(pos, 0xFFFFFFFF, entry.path.c_str(), buf);
			if ( auto imgGet = Image::get(img->path.c_str()) )
			{
				img->pos.w = imgGet->getWidth();
				img->pos.h = imgGet->getHeight();
			}
		}
		++panelIndex;
	}

	panelIndex = 0;
	for ( auto& entry : panelEntries )
	{
		if ( panelIndex < PANEL_DIRECTION_END )
		{
			SDL_Rect pos{ entry.x + midx, entry.y + midy, 0, 0 };
			char buf[32] = "";
			snprintf(buf, sizeof(buf), "icon %d", panelIndex);
			Frame::image_t* imgIcon = bgFrame->addImage(pos, 0xFFFFFFFF, "", buf);
			imgIcon->disabled = true;
		}
		++panelIndex;
	}

	{
		// do center panel
		auto& entry = panelEntries[panelEntries.size() - 1];
		SDL_Rect pos{ entry.x + midx, entry.y + midy, 0, 0 };
		char buf[32] = "";
		snprintf(buf, sizeof(buf), "panel %d", PANEL_DIRECTION_END);
		Frame::image_t* img = bgFrame->addImage(pos, 0xFFFFFFFF, entry.path.c_str(), buf);
		if ( auto imgGet = Image::get(img->path.c_str()) )
		{
			img->pos.w = imgGet->getWidth();
			img->pos.h = imgGet->getHeight();
		}
	}

	auto bannerFrame = followerFrame->addFrame("banner frame");
	bannerFrame->setSize(SDL_Rect{ 0, 0, 0, 40 });
	bannerFrame->setHollow(false);
	bannerFrame->setDisabled(false);
	bannerFrame->setInheritParentFrameOpacity(false);
	bannerFrame->addImage(SDL_Rect{ 0, 0, 42, 40 }, 0xFFFFFFFF, "#*images/ui/FollowerWheel/banner-cmd_l.png", "banner left");
	bannerFrame->addImage(SDL_Rect{ 0, 0, 42, 40 }, 0xFFFFFFFF, "#*images/ui/FollowerWheel/banner-cmd_r.png", "banner right");
	bannerFrame->addImage(SDL_Rect{ 0, 12, 0, 28 }, 0xFFFFFFFF, "*images/ui/FollowerWheel/banner-cmd_c.png", "banner center");
	auto bannerText = bannerFrame->addField("banner txt", 128);
	bannerText->setFont(font);
	bannerText->setText("");
	bannerText->setHJustify(Field::justify_t::LEFT);
	bannerText->setVJustify(Field::justify_t::TOP);
	bannerText->setSize(SDL_Rect{ 0, 0, 0, 24 });
	bannerText->setTextColor(followerBannerTextColor);
	bannerText->setOutlineColor(makeColor(29, 16, 11, 255));
	auto bannerGlyph = bannerFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "", "banner glyph");
	bannerGlyph->disabled = true;
	auto bannerGlyph2 = bannerFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "", "banner modifier glyph");
	bannerGlyph2->disabled = true;

	auto wheelTitleText = bgFrame->addField("wheel title", 128);
	wheelTitleText->setFont(font);
	wheelTitleText->setText("");
	wheelTitleText->setHJustify(Field::justify_t::LEFT);
	wheelTitleText->setVJustify(Field::justify_t::TOP);
	wheelTitleText->setSize(SDL_Rect{ 0, 0, 240, 24 });
	wheelTitleText->setTextColor(followerTitleColor);
	wheelTitleText->setOutlineColor(makeColor(29, 16, 11, 255));

	auto wheelSkillImg = bannerFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "", "skill img");
	wheelSkillImg->disabled = true;
	auto wheelStatImg = bannerFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "", "stat img");
	wheelStatImg->disabled = true;
}

void setFollowerBannerTextFormatted(const int player, Field* field, Uint32 color, std::set<int>& highlights, char const * const text, ...)
{
	if ( !field ) { return; }

	char buf[256] = "";
	va_list argptr;
	va_start(argptr, text);
	vsnprintf(buf, sizeof(buf), text, argptr);
	va_end(argptr);

	field->setText(buf);
	field->clearWordsToHighlight();
	for ( auto v : highlights )
	{
		field->addWordToHighlight(v, color);
	}
}

void setFollowerBannerText(const int player, Field* field, const char* iconName, const char* textKey, Uint32 color)
{
	if ( !field ) { return; }
	if ( FollowerMenu[player].iconEntries.find(iconName) == FollowerMenu[player].iconEntries.end() )
	{
		return;
	}
	auto& textMap = FollowerMenu[player].iconEntries[iconName].text_map[textKey];
	field->setText(textMap.first.c_str());
	field->clearWordsToHighlight();
	for ( auto v : textMap.second )
	{
		field->addWordToHighlight(v, color);
	}
}

std::vector<FollowerRadialMenu::PanelEntry>& getPanelEntriesForFollower(bool isTinkeringCreation)
{
	if ( isTinkeringCreation )
	{
		return FollowerRadialMenu::panelEntries;
	}
	else
	{
		return FollowerRadialMenu::panelEntriesAlternate;
	}
}

bool commandCanBeSentToAll(int optionSelected)
{
	if ( optionSelected != ALLY_CMD_MOVETO_CONFIRM
		&& optionSelected != ALLY_CMD_ATTACK_CONFIRM
		&& optionSelected != ALLY_CMD_FOLLOW
		&& optionSelected != ALLY_CMD_DEFEND )
	{
		return false;
	}
	return true;
}

std::vector<Entity*> getAllOtherFollowersForSendAllCommand(const int gui_player, Entity* followerToCommand, Monster followerType, int optionSelected)
{
	std::vector<Entity*> vec;
	if ( !followerToCommand )
	{
		return vec;
	}
	if ( !commandCanBeSentToAll(optionSelected) )
	{
		return vec;
	}

	if ( optionSelected == ALLY_CMD_ATTACK_CONFIRM )
	{
		// only send commands if we're trying to attack
		Entity* target = uidToEntity(followerToCommand->monsterAllyInteractTarget);
		if ( target )
		{
			if ( target->behavior != &actMonster && target->behavior != &actPlayer )
			{
				return vec;
			}
		}
		else
		{
			return vec;
		}
	}

	for ( node_t* node = stats[gui_player]->FOLLOWERS.first; node != nullptr; node = node->next )
	{
		Entity* follower2 = nullptr;
		if ( (Uint32*)node->element )
		{
			if ( follower2 = uidToEntity(*((Uint32*)node->element)) )
			{
				if ( follower2 == followerToCommand || !follower2->getStats() )
				{
					continue;
				}
				auto follower2Type = follower2->getStats()->type;

				if ( optionSelected == ALLY_CMD_MOVETO_CONFIRM || optionSelected == ALLY_CMD_FOLLOW 
					|| optionSelected == ALLY_CMD_DEFEND )
				{
					if ( (followerType == SENTRYBOT || followerType == SPELLBOT)
						&& (follower2Type != SENTRYBOT && follower2Type != SPELLBOT) )
					{
						continue; // bots have unique moveto cmd
					}
					if ( followerType != SENTRYBOT && followerType != SPELLBOT
						&& (follower2Type == SENTRYBOT || follower2Type == SPELLBOT) )
					{
						continue; // bots have unique moveto cmd
					}
				}

				if ( optionSelected == ALLY_CMD_FOLLOW )
				{
					if ( !(follower2->monsterAllyState == ALLY_STATE_DEFEND || follower2->monsterAllyState == ALLY_STATE_MOVETO) )
					{
						continue;
					}
					if ( (followerType == GYROBOT && follower2Type != GYROBOT)
						|| (followerType != GYROBOT && follower2Type == GYROBOT) )
					{
						continue; // gyrobot must issue follow/wait separately
					}
				}
				else if ( optionSelected == ALLY_CMD_DEFEND )
				{
					if ( follower2->monsterAllyState == ALLY_STATE_DEFEND || follower2->monsterAllyState == ALLY_STATE_MOVETO )
					{
						continue;
					}
					if ( (followerType == GYROBOT && follower2Type != GYROBOT)
						|| (followerType != GYROBOT && follower2Type == GYROBOT) )
					{
						continue; // gyrobot must issue follow/wait separately
					}
				}
				
				int skillLVL2 = stats[gui_player]->getModifiedProficiency(PRO_LEADERSHIP) + statGetCHR(stats[gui_player], players[gui_player]->entity);
				if ( FollowerMenu[gui_player].isTinkeringFollower(follower2Type) )
				{
					skillLVL2 = stats[gui_player]->getModifiedProficiency(PRO_LOCKPICKING) + statGetPER(stats[gui_player], players[gui_player]->entity);
				}
				if ( follower2->monsterAllySummonRank != 0 )
				{
					skillLVL2 = SKILL_LEVEL_LEGENDARY;
				}
				int disableOption2 = FollowerMenu[gui_player].optionDisabledForCreature(skillLVL2, follower2Type, optionSelected, follower2);
				if ( disableOption2 == 0 )
				{
					vec.push_back(follower2);
				}
			}
		}
	}
	return vec;
}

void FollowerRadialMenu::drawFollowerMenu()
{
	auto player = players[gui_player];
	if ( !player->isLocalPlayer() )
	{
		closeFollowerMenuGUI();
		return;
	}

	Input& input = Input::inputs[gui_player];

	if ( selectMoveTo )
	{
		if ( input.binaryToggle("MenuCancel") )
		{
			input.consumeBinaryToggle("MenuCancel");
			input.consumeBindingsSharedWithBinding("MenuCancel");
			closeFollowerMenuGUI();
			Player::soundCancel();
		}
		if ( !followerToCommand )
		{
			selectMoveTo = false;
			closeFollowerMenuGUI();
		}
		else if ( followerFrame )
		{
			followerFrame->setDisabled(true);
		}
		return;
	}

	if ( !followerFrame )
	{
		return;
	}

	followerFrame->setSize(SDL_Rect{ players[gui_player]->camera_virtualx1(),
		players[gui_player]->camera_virtualy1(),
		players[gui_player]->camera_virtualWidth(),
		players[gui_player]->camera_virtualHeight() });

	int disableOption = 0;
	bool keepWheelOpen = false;

	Sint32 omousex = inputs.getMouse(gui_player, Inputs::OX);
	Sint32 omousey = inputs.getMouse(gui_player, Inputs::OY);

	std::map<int, Frame::image_t*> panelImages;
	std::map<int, Frame::image_t*> panelIcons;
	Frame* bannerFrame = nullptr;
	Field* bannerTxt = nullptr;
	Frame::image_t* bannerImgLeft = nullptr;
	Frame::image_t* bannerImgRight = nullptr;
	Frame::image_t* bannerImgCenter = nullptr;
	Uint32 textHighlightColor = followerBannerTextHighlightColor;
	bool tinkeringFollower = false;

	if ( recentEntity )
	{
		if ( recentEntity->monsterAllyIndex != gui_player ) // our ally left our service by charm or otherwise
		{
			recentEntity = nullptr;
			if ( followerToCommand == recentEntity )
			{
				closeFollowerMenuGUI();
				players[gui_player]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_CLOSE_ALL);
				return;
			}
		}
	}

	if ( !followerToCommand && (!followerFrame->isDisabled() || players[gui_player]->gui_mode == GUI_MODE_FOLLOWERMENU) )
	{
		closeFollowerMenuGUI();
		players[gui_player]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_CLOSE_ALL);
		return;
	}
	if ( followerMenuIsOpen() && input.binaryToggle("MenuCancel") )
	{
		input.consumeBinaryToggle("MenuCancel");
		input.consumeBindingsSharedWithBinding("MenuCancel");
		closeFollowerMenuGUI();
		players[gui_player]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_CLOSE_ALL);
		Player::soundCancel();
		return;
	}

	//if ( ticks % 50 == 0 )
	//{
	//	consoleCommand("/loadfollowerwheel");
	//}

	bool modifierPressed = false;
	bool modifierActiveForOption = false;
	if ( input.binary("Defend") )
	{
		modifierPressed = true;
	}

	if ( followerToCommand )
	{
		{
			const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
			real_t setpointDiff = fpsScale * std::max(.1, (1.0 - animTitle)) / 2.5;
			animTitle += setpointDiff;
			animTitle = std::min(1.0, animTitle);

			real_t setpointDiff2 = fpsScale * std::max(.01, (1.0 - animWheel)) / 2.0;
			animWheel += setpointDiff2;
			animWheel = std::min(1.0, animWheel);

			// shaking feedback for invalid action
			// constant decay for animation
			real_t setpointDiffX = fpsScale * 1.0 / 25.0;
			animInvalidAction -= setpointDiffX;
			animInvalidAction = std::max(0.0, animInvalidAction);
		}

		if ( players[gui_player] && players[gui_player]->entity
			&& followerToCommand->monsterTarget == players[gui_player]->entity->getUID() )
		{
			players[gui_player]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_DONT_CLOSE_FOLLOWERGUI);
			return;
		}

		Stat* followerStats = followerToCommand->getStats();
		if ( !followerStats )
		{
			closeFollowerMenuGUI();
			return;
		}
		tinkeringFollower = isTinkeringFollower(followerStats->type);

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
				skillLVL = stats[gui_player]->getModifiedProficiency(PRO_LEADERSHIP) + statGetCHR(stats[gui_player], players[gui_player]->entity);
				if ( tinkeringFollower )
				{
					skillLVL = stats[gui_player]->getModifiedProficiency(PRO_LOCKPICKING) + statGetPER(stats[gui_player], players[gui_player]->entity);
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
						disableOption = optionDisabledForCreature(skillLVL, followerStats->type, ALLY_CMD_ATTACK_CONFIRM, followerToCommand);
					}
					else
					{
						disableOption = optionDisabledForCreature(skillLVL, followerStats->type, optionSelected, followerToCommand);
					}
				}
				else
				{
					disableOption = optionDisabledForCreature(skillLVL, followerStats->type, optionSelected, followerToCommand);
				}
			}
		}

		bool menuConfirmOnGamepad = input.input("MenuConfirm").isBindingUsingGamepad();
		bool menuLeftClickOnKeyboard = input.input("MenuLeftClick").isBindingUsingKeyboard() && !inputs.hasController(gui_player);

		// process commands if option selected on the wheel.
		if ( !(players[gui_player]->bControlEnabled && !gamePaused && !players[gui_player]->usingCommand()) )
		{
			// no action
		}
		else if ( (!menuToggleClick && !holdWheel
					&& !input.binaryToggle("Use") 
					&& !input.binaryToggle("Show NPC Commands") 
					&& !(input.binaryToggle("MenuConfirm") && menuConfirmOnGamepad)
					&& !(input.binaryToggle("MenuLeftClick") && menuLeftClickOnKeyboard) )
			|| (menuToggleClick && (input.binaryToggle("Use") || input.binaryToggle("Show NPC Commands")) )
			|| ( (input.binaryToggle("MenuConfirm") && menuConfirmOnGamepad)
				|| (input.binaryToggle("MenuLeftClick") && menuLeftClickOnKeyboard)
				|| (input.binaryToggle("Use") && holdWheel) )
			|| (!input.binaryToggle("Show NPC Commands") && holdWheel && !menuToggleClick)
			|| (input.binaryToggle("Command NPC") && optionPrevious != -1)
			)
		{
			bool usingLastCmd = false;
			//bool usingShowCmdRelease = (!input.binaryToggle("Show NPC Commands") && holdWheel && !menuToggleClick);
			if ( input.binaryToggle("Command NPC") )
			{
				usingLastCmd = true;
			}

			if ( menuToggleClick )
			{
				menuToggleClick = false;
				if ( optionSelected == -1 )
				{
					optionSelected = ALLY_CMD_CANCEL;
				}
			}

			input.consumeBinaryToggle("Use");
			input.consumeBinaryToggle("MenuConfirm");
			input.consumeBinaryToggle("MenuLeftClick");
			input.consumeBinaryToggle("Show NPC Commands");
			input.consumeBindingsSharedWithBinding("Use");
			input.consumeBindingsSharedWithBinding("MenuConfirm");
			input.consumeBindingsSharedWithBinding("MenuLeftClick");
			input.consumeBindingsSharedWithBinding("Show NPC Commands");

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

			bool sfxPlayed = false;
			if ( disableOption != 0 )
			{
				animInvalidAction = 1.0;
				animInvalidActionTicks = ticks;
				//if ( !usingShowCmdRelease )
				//{
				//	// play bad feedback sfx
				//}
				playSound(90, 64);
				sfxPlayed = true;
			}

			if ( usingLastCmd )
			{
				if ( keepWheelOpen )
				{
					// need to reset the coordinates of the mouse.
					if ( players[gui_player]->gui_mode != GUI_MODE_FOLLOWERMENU )
					{
						initfollowerMenuGUICursor(true); // set gui_mode to follower menu
					}
					else
					{
						initfollowerMenuGUICursor(false);
					}
				}
				input.consumeBinaryToggle("Command NPC");
			}

			if ( optionSelected != -1 )
			{
				holdWheel = false;
				// return to shootmode and close guis etc. TODO: tidy up interface code into 1 spot?
				if ( !keepWheelOpen )
				{
					if ( !accessedMenuFromPartySheet
						|| optionSelected == ALLY_CMD_MOVETO_SELECT
						|| optionSelected == ALLY_CMD_ATTACK_SELECT
						|| optionSelected == ALLY_CMD_CANCEL )
					{
						players[gui_player]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_DONT_CLOSE_FOLLOWERGUI);
						if ( !followerToCommand )
						{
							return;
						}
					}
				}

				if ( optionSelected != ALLY_CMD_ATTACK_CONFIRM && optionSelected != ALLY_CMD_MOVETO_CONFIRM )
				{
					if ( !sfxPlayed && optionSelected != ALLY_CMD_CANCEL )
					{
						playSound(139, 64); // click
						sfxPlayed = true;
					}
				}
				else
				{
					playSound(399, 48); // ping
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
						if ( modifierPressed )
						{
							if ( !usingLastCmd )
							{
								if ( stats[gui_player]->shield && itemCategory(stats[gui_player]->shield) == SPELLBOOK )
								{
									input.consumeBinaryToggle("Defend"); // don't try cast when menu closes.
								}
							}
						}

						if ( optionSelected == ALLY_CMD_DEFEND &&
							(followerToCommand->monsterAllyState == ALLY_STATE_DEFEND || followerToCommand->monsterAllyState == ALLY_STATE_MOVETO) )
						{
							optionSelected = ALLY_CMD_FOLLOW;
						}
						if ( multiplayer == CLIENT )
						{
							if ( optionSelected == ALLY_CMD_ATTACK_CONFIRM )
							{
								Uint32 olduid = followerToCommand->monsterAllyInteractTarget;
								sendAllyCommandClient(gui_player, followerToCommand->getUID(), optionSelected, 0, 0, followerToCommand->monsterAllyInteractTarget);
								Uint32 newuid = followerToCommand->monsterAllyInteractTarget;
								if ( modifierPressed )
								{
									followerToCommand->monsterAllyInteractTarget = olduid;
									auto repeatCommandToFollowers = getAllOtherFollowersForSendAllCommand(gui_player, followerToCommand, followerStats->type, optionSelected);
									for ( auto f : repeatCommandToFollowers )
									{
										f->monsterAllyInteractTarget = olduid;
										sendAllyCommandClient(gui_player, f->getUID(), optionSelected, 0, 0, f->monsterAllyInteractTarget);
									}
									followerToCommand->monsterAllyInteractTarget = newuid;
								}
							}
							else if ( optionSelected == ALLY_CMD_MOVETO_CONFIRM )
							{
								sendAllyCommandClient(gui_player, followerToCommand->getUID(), optionSelected, moveToX, moveToY);
								if ( modifierPressed )
								{
									auto repeatCommandToFollowers = getAllOtherFollowersForSendAllCommand(gui_player, followerToCommand, followerStats->type, optionSelected);
									for ( auto f : repeatCommandToFollowers )
									{
										sendAllyCommandClient(gui_player, f->getUID(), optionSelected, moveToX, moveToY);
									}
								}
							}
							else
							{
								sendAllyCommandClient(gui_player, followerToCommand->getUID(), optionSelected, 0, 0);
								if ( modifierPressed )
								{
									auto repeatCommandToFollowers = getAllOtherFollowersForSendAllCommand(gui_player, followerToCommand, followerStats->type, optionSelected);
									for ( auto f : repeatCommandToFollowers )
									{
										sendAllyCommandClient(gui_player, f->getUID(), optionSelected, 0, 0);
									}
								}
							}
						}
						else
						{
							Uint32 olduid = followerToCommand->monsterAllyInteractTarget;
							followerToCommand->monsterAllySendCommand(optionSelected, moveToX, moveToY, followerToCommand->monsterAllyInteractTarget);
							Uint32 newuid = followerToCommand->monsterAllyInteractTarget;
							if ( modifierPressed )
							{
								followerToCommand->monsterAllyInteractTarget = olduid;
								auto repeatCommandToFollowers = getAllOtherFollowersForSendAllCommand(gui_player, followerToCommand, followerStats->type, optionSelected);
								for ( auto f : repeatCommandToFollowers )
								{
									f->monsterAllyInteractTarget = olduid;
									f->monsterAllySendCommand(optionSelected, moveToX, moveToY, f->monsterAllyInteractTarget);
								}
								followerToCommand->monsterAllyInteractTarget = newuid;
							}
						}
					}
					else if ( usingLastCmd )
					{
						// tell player current monster can't do what you asked (e.g using last command & swapping between monsters with different requirements)
						if ( disableOption < 0 )
						{
							messagePlayer(gui_player, MESSAGE_MISC, Language::get(3640), getMonsterLocalizedName(followerStats->type).c_str());
						}
						else if ( tinkeringFollower )
						{
							messagePlayer(gui_player, MESSAGE_MISC, Language::get(3639), getMonsterLocalizedName(followerStats->type).c_str());
						}
						else
						{
							messagePlayer(gui_player, MESSAGE_MISC, Language::get(3638), getMonsterLocalizedName(followerStats->type).c_str());
						}
					}

					if ( optionSelected != ALLY_CMD_CANCEL && disableOption == 0 )
					{
						if ( optionSelected == ALLY_CMD_CLASS_TOGGLE || optionSelected == ALLY_CMD_PICKUP_TOGGLE )
						{
							optionPrevious = -1;
						}
						else
						{
							optionPrevious = optionSelected;
						}
					}
					else if ( optionSelected == ALLY_CMD_CANCEL && !sfxPlayed )
					{
						Player::soundCancel();
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

	static ConsoleVariable<bool> cvar_showoldwheel("/followerwheel_old_enable", false);

	if ( followerToCommand )
	{
		if ( !followerGUIHasBeenCreated() )
		{
			createFollowerMenuGUI();
		}
		followerFrame->setDisabled(false);

		auto bgFrame = followerFrame->findFrame("wheel base");
		bgFrame->setOpacity(100.0 * animWheel);
		bannerFrame = followerFrame->findFrame("banner frame");
		bannerImgLeft = bannerFrame->findImage("banner left");
		bannerImgRight = bannerFrame->findImage("banner right");
		bannerImgCenter = bannerFrame->findImage("banner center");
		bannerTxt = bannerFrame->findField("banner txt");
		bannerTxt->setText("");

		int direction = NORTH;
		const int midx = followerFrame->getSize().w / 2;
		const int midy = followerFrame->getSize().h / 2;
		for ( auto img : bgFrame->getImages() )
		{
			if ( direction < PANEL_DIRECTION_END )
			{
				panelImages[direction] = img;
				img->pos.x = getPanelEntriesForFollower(tinkeringFollower)[direction].x + midx + FollowerRadialMenu::followerWheelFrameOffsetX;
				img->pos.y = getPanelEntriesForFollower(tinkeringFollower)[direction].y + midy + FollowerRadialMenu::followerWheelFrameOffsetY;
				img->path = getPanelEntriesForFollower(tinkeringFollower)[direction].path;
			}
			else if ( direction < 2 * PANEL_DIRECTION_END )
			{
				img->disabled = true;
				img->path = "";
				int direction2 = direction - PANEL_DIRECTION_END;
				panelIcons[direction2] = img;
				panelIcons[direction2]->pos.x = panelImages[direction2]->pos.x + getPanelEntriesForFollower(tinkeringFollower)[direction2].icon_offsetx;
				panelIcons[direction2]->pos.y = panelImages[direction2]->pos.y + getPanelEntriesForFollower(tinkeringFollower)[direction2].icon_offsety;
			}
			else if ( direction == 2 * PANEL_DIRECTION_END ) // center img
			{
				panelImages[PANEL_DIRECTION_END] = img;
				img->pos.x = getPanelEntriesForFollower(tinkeringFollower)[PANEL_DIRECTION_END].x + midx + FollowerRadialMenu::followerWheelFrameOffsetX;
				img->pos.y = getPanelEntriesForFollower(tinkeringFollower)[PANEL_DIRECTION_END].y + midy + FollowerRadialMenu::followerWheelFrameOffsetY;
				img->path = getPanelEntriesForFollower(tinkeringFollower)[PANEL_DIRECTION_END].path;
			}
			++direction;
		}

		int skillLVL = 0;
		Stat* followerStats = followerToCommand->getStats();
		if ( !followerStats )
		{
			return;
		}
		bool tinkeringFollower = isTinkeringFollower(followerStats->type);
		if ( stats[gui_player] && players[gui_player] && players[gui_player]->entity )
		{
			skillLVL = stats[gui_player]->getModifiedProficiency(PRO_LEADERSHIP) + statGetCHR(stats[gui_player], players[gui_player]->entity);
			if ( tinkeringFollower )
			{
				skillLVL = stats[gui_player]->getModifiedProficiency(PRO_LOCKPICKING) + statGetPER(stats[gui_player], players[gui_player]->entity);
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

		radius = FollowerRadialMenu::followerWheelRadius;
		thickness = FollowerRadialMenu::followerWheelButtonThickness;
		real_t menuScale = yres / (real_t)Frame::virtualScreenY;
		radius *= menuScale;
		thickness *= menuScale;
		int centerButtonHighlightOffset = tinkeringFollower 
			? FollowerRadialMenu::followerWheelInnerCircleRadiusOffset 
			: FollowerRadialMenu::followerWheelInnerCircleRadiusOffsetAlternate;

		int highlight = -1;
		int i = 0;

		int width = 0;
		if ( *cvar_showoldwheel )
		{
			getSizeOfText(ttf12, Language::get(3036), &width, nullptr);
			if ( players[gui_player]->camera_height() < 768 )
			{
				ttfPrintText(ttf12, src.x - width / 2, src.y - radius - thickness - 14, Language::get(3036));
			}
			else
			{
				ttfPrintText(ttf12, src.x - width / 2, src.y - radius - thickness - 24, Language::get(3036));
			}
		}

		if ( inputs.hasController(gui_player) )
		{
			auto controller = inputs.getController(gui_player);
			if ( controller )
			{
				GameController::DpadDirection dir = controller->dpadDirToggle();
				if ( dir != GameController::DpadDirection::INVALID )
				{
					if ( !controller->virtualDpad.consumed )
					{
						Player::soundMovement();
					}
					controller->consumeDpadDirToggle();
					switch ( dir )
					{
						case GameController::DpadDirection::UP:
							highlight = 0;
							break;
						case GameController::DpadDirection::UPLEFT:
							highlight = 1;
							break;
						case GameController::DpadDirection::LEFT:
							highlight = 2;
							break;
						case GameController::DpadDirection::DOWNLEFT:
							highlight = 3;
							break;
						case GameController::DpadDirection::DOWN:
							highlight = 4;
							break;
						case GameController::DpadDirection::DOWNRIGHT:
							highlight = 5;
							break;
						case GameController::DpadDirection::RIGHT:
							highlight = 6;
							break;
						case GameController::DpadDirection::UPRIGHT:
							highlight = 7;
							break;
						default:
							break;
					}
					real_t angleMiddleForOption = PI / 2 + dir * (2 * PI / numoptions);
					omousex = centerx + (radius + thickness) * .75 * cos(angleMiddleForOption);
					omousey = centery + (radius + thickness) * .75 * sin(angleMiddleForOption);
					inputs.setMouse(gui_player, Inputs::OX, omousex);
					inputs.setMouse(gui_player, Inputs::OY, omousey);
					inputs.setMouse(gui_player, Inputs::X, omousex);
					inputs.setMouse(gui_player, Inputs::Y, omousey);

					if ( highlight != -1 )
					{
						inputs.getVirtualMouse(gui_player)->draw_cursor = false;
					}
				}
			}
		}

		bool mouseInCenterButton = sqrt(pow((omousex - menuX), 2) + pow((omousey - menuY), 2)) < (radius - thickness);

		angleStart = PI / 2 - (PI / numoptions);
		angleMiddle = angleStart + PI / numoptions;
		angleEnd = angleMiddle + PI / numoptions;

		if ( *cvar_showoldwheel )
		{
			//drawImageRing(fancyWindow_bmp, &src, radius, thickness, 40, 0, PI * 2, 156);

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
		}

		angleStart = PI / 2 - (PI / numoptions);
		angleMiddle = angleStart + PI / numoptions;
		angleEnd = angleMiddle + PI / numoptions;

		const real_t mouseDetectionAdjust = PI / 128;
		for ( i = 0; i < numoptions; ++i )
		{
			// see if mouse cursor is within an option.
			if ( highlight == -1 )
			{
				if ( !mouseInCenterButton )
				{
					real_t x1 = menuX + (radius + thickness + 45) * cos(angleEnd + mouseDetectionAdjust);
					real_t y1 = menuY - (radius + thickness + 45) * sin(angleEnd + mouseDetectionAdjust);
					real_t x2 = menuX + 5 * cos(angleMiddle);
					real_t y2 = menuY - 5 * sin(angleMiddle);
					real_t x3 = menuX + (radius + thickness + 45) * cos(angleStart - mouseDetectionAdjust);
					real_t y3 = menuY - (radius + thickness + 45) * sin(angleStart - mouseDetectionAdjust);
					real_t a = ((y2 - y3)*(omousex - x3) + (x3 - x2)*(omousey - y3)) / ((y2 - y3)*(x1 - x3) + (x3 - x2)*(y1 - y3));
					real_t b = ((y3 - y1)*(omousex - x3) + (x1 - x3)*(omousey - y3)) / ((y2 - y3)*(x1 - x3) + (x3 - x2)*(y1 - y3));
					real_t c = 1 - a - b;
					if ( (0 <= a && a <= 1) && (0 <= b && b <= 1) && (0 <= c && c <= 1) )
					{
						//barycentric calc for figuring if mouse point is within triangle.
						highlight = i;
						if ( *cvar_showoldwheel )
						{
							//drawImageRing(fancyWindow_bmp, &src, radius, thickness, (numoptions) * 8, angleStart, angleEnd, 192);

							// draw borders around highlighted item.
							Uint32 borderColor = uint32ColorBaronyBlue;
							if ( optionDisabledForCreature(skillLVL, followerStats->type, i, followerToCommand) != 0 )
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
				if ( !inputs.hasController(gui_player) )
				{
					if ( highlight != -1 && optionSelected != highlight && optionSelected != -1 )
					{
						Player::soundMovement();
					}
				}
			}

			SDL_Rect txt;
			txt.x = src.x + src.w * cos(angleMiddle);
			txt.y = src.y - src.h * sin(angleMiddle);
			txt.w = 0;
			txt.h = 0;

			// draw the text for the menu wheel.

			bool lockedOption = false;
			if ( optionDisabledForCreature(skillLVL, followerStats->type, i, followerToCommand) != 0 )
			{
				if ( *cvar_showoldwheel )
				{
					//SDL_Rect img;
					//img.x = txt.x - sidebar_unlock_bmp->w / 2;
					//img.y = txt.y - sidebar_unlock_bmp->h / 2;
					//img.w = sidebar_unlock_bmp->w;
					//img.h = sidebar_unlock_bmp->h;
					//drawImage(sidebar_unlock_bmp, nullptr, &img); // locked menu options
				}
				lockedOption = true;
			}
			else if ( i == ALLY_CMD_DEFEND
				&& (followerToCommand->monsterAllyState == ALLY_STATE_DEFEND || followerToCommand->monsterAllyState == ALLY_STATE_MOVETO) )
			{
				if ( followerStats->type == SENTRYBOT || followerStats->type == SPELLBOT )
				{
					getSizeOfText(ttf12, Language::get(3675), &width, nullptr);
					(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, Language::get(3675)) : SDL_Rect{};
					if ( i == highlight )
					{
						panelIcons[i]->path = iconEntries["tinker_aim_look"].path_active_hover;
						if ( modifierPressed && commandCanBeSentToAll(i) )
						{
							modifierActiveForOption = true;
							setFollowerBannerText(gui_player, bannerTxt, "tinker_aim_look", "free_look_all", textHighlightColor);
						}
						else
						{
							setFollowerBannerText(gui_player, bannerTxt, "tinker_aim_look", "free_look", textHighlightColor);
						}
					}
					else
					{
						panelIcons[i]->path = iconEntries["tinker_aim_look"].path_active;
					}
				}
				else
				{
					getSizeOfText(ttf12, Language::get(3037 + i + 8), &width, nullptr);
					(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, Language::get(3037 + i + 8)) : SDL_Rect{};
					// "follow"
					if ( i == highlight )
					{
						if ( tinkeringFollower )
						{
							panelIcons[i]->path = iconEntries["tinker_wait"].path_active_hover;
							if ( modifierPressed && commandCanBeSentToAll(i) )
							{
								modifierActiveForOption = true;
								setFollowerBannerText(gui_player, bannerTxt, "tinker_wait", "follow_all", textHighlightColor);
							}
							else
							{
								setFollowerBannerText(gui_player, bannerTxt, "tinker_wait", "follow", textHighlightColor);
							}
						}
						else
						{
							panelIcons[i]->path = iconEntries["leader_wait"].path_active_hover;
							if ( modifierPressed && commandCanBeSentToAll(i) )
							{
								modifierActiveForOption = true;
								setFollowerBannerText(gui_player, bannerTxt, "leader_wait", "follow_all", textHighlightColor);
							}
							else
							{
								setFollowerBannerText(gui_player, bannerTxt, "leader_wait", "follow", textHighlightColor);
							}
						}
					}
					else
					{
						if ( tinkeringFollower )
						{
							panelIcons[i]->path = iconEntries["tinker_wait"].path_active;
						}
						else
						{
							panelIcons[i]->path = iconEntries["leader_wait"].path_active;
						}
					}
				}
			}
			else
			{
				getSizeOfText(ttf12, Language::get(3037 + i), &width, nullptr);
				if ( i == ALLY_CMD_DEFEND 
					&& followerToCommand->monsterAllyState == ALLY_STATE_DEFAULT
					&& (followerStats->type == SENTRYBOT || followerStats->type == SPELLBOT) )
				{
					getSizeOfText(ttf12, Language::get(3674), &width, nullptr);
					(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, Language::get(3674)) : SDL_Rect{};
					if ( i == highlight )
					{
						panelIcons[i]->path = iconEntries["tinker_aim_look"].path_hover;
						if ( modifierPressed && commandCanBeSentToAll(i) )
						{
							modifierActiveForOption = true;
							setFollowerBannerText(gui_player, bannerTxt, "tinker_aim_look", "hold_aim_all", textHighlightColor);
						}
						else
						{
							setFollowerBannerText(gui_player, bannerTxt, "tinker_aim_look", "hold_aim", textHighlightColor);
						}
					}
					else
					{
						panelIcons[i]->path = iconEntries["tinker_aim_look"].path;
					}
				}
				else if ( i == ALLY_CMD_CLASS_TOGGLE )
				{
					if ( followerStats->type == GYROBOT )
					{
						// draw higher.
						getSizeOfText(ttf12, Language::get(3619), &width, nullptr);
						(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y - 12, Language::get(3619)) : SDL_Rect{};
						getSizeOfText(ttf12, Language::get(3620 + followerToCommand->monsterAllyClass), &width, nullptr);
						(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, Language::get(3620 + followerToCommand->monsterAllyClass)) : SDL_Rect{};
						switch ( followerToCommand->monsterAllyClass )
						{
							case ALLY_GYRO_LIGHT_FAINT:
								if ( i == highlight )
								{
									panelIcons[i]->path = iconEntries["tinker_light_faint"].path_active_hover;
									setFollowerBannerText(gui_player, bannerTxt, "tinker_light_faint", "default", textHighlightColor);
								}
								else
								{
									panelIcons[i]->path = iconEntries["tinker_light_faint"].path_active;
								}
								break;
							case ALLY_GYRO_LIGHT_BRIGHT:
								if ( i == highlight )
								{
									panelIcons[i]->path = iconEntries["tinker_light_bright"].path_active_hover;
									setFollowerBannerText(gui_player, bannerTxt, "tinker_light_bright", "default", textHighlightColor);
								}
								else
								{
									panelIcons[i]->path = iconEntries["tinker_light_bright"].path_active;
								}
								break;
							case ALLY_GYRO_LIGHT_NONE:
								if ( i == highlight )
								{
									panelIcons[i]->path = iconEntries["tinker_light_none"].path_active_hover;
									setFollowerBannerText(gui_player, bannerTxt, "tinker_light_none", "default", textHighlightColor);
								}
								else
								{
									panelIcons[i]->path = iconEntries["tinker_light_none"].path_active;
								}
							default:
								break;
						}
					}
					else if ( followerToCommand && followerToCommand->monsterAllySummonRank != 0 )
					{
						getSizeOfText(ttf12, "Relinquish ", &width, nullptr);
						(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y - 12, Language::get(3196)) : SDL_Rect{};
						if ( i == highlight )
						{
							panelIcons[i]->path = iconEntries["leader_relinquish_soul"].path_hover;
							setFollowerBannerText(gui_player, bannerTxt, "leader_relinquish_soul", "default", textHighlightColor);
						}
						else
						{
							panelIcons[i]->path = iconEntries["leader_relinquish_soul"].path;
						}
					}
					else
					{
						// draw higher.
						(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y - 12, Language::get(3037 + i)) : SDL_Rect{};
						getSizeOfText(ttf12, Language::get(3053 + followerToCommand->monsterAllyClass), &width, nullptr);
						(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, Language::get(3053 + followerToCommand->monsterAllyClass)) : SDL_Rect{};
						switch ( followerToCommand->monsterAllyClass )
						{
							case ALLY_CLASS_MELEE:
								if ( i == highlight )
								{
									panelIcons[i]->path = iconEntries["leader_class_melee"].path_active_hover;
									setFollowerBannerText(gui_player, bannerTxt, "leader_class_melee", "default", textHighlightColor);
								}
								else
								{
									panelIcons[i]->path = iconEntries["leader_class_melee"].path_active;
								}
								break;
							case ALLY_CLASS_RANGED:
								if ( i == highlight )
								{
									panelIcons[i]->path = iconEntries["leader_class_ranged"].path_active_hover;
									setFollowerBannerText(gui_player, bannerTxt, "leader_class_ranged", "default", textHighlightColor);
								}
								else
								{
									panelIcons[i]->path = iconEntries["leader_class_ranged"].path_active;
								}
								break;
							case ALLY_CLASS_MIXED:
							default:
								if ( i == highlight )
								{
									panelIcons[i]->path = iconEntries["leader_class_mixed"].path_active_hover;
									setFollowerBannerText(gui_player, bannerTxt, "leader_class_mixed", "default", textHighlightColor);
								}
								else
								{
									panelIcons[i]->path = iconEntries["leader_class_mixed"].path_active;
								}
								break;
						}
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
							(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y - 24, Language::get(3636)) : SDL_Rect{};
							getSizeOfText(ttf12, Language::get(3624 + followerToCommand->monsterAllyPickupItems), &width, nullptr);
							(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y + 12, Language::get(3624 + followerToCommand->monsterAllyPickupItems)) : SDL_Rect{};
						}
						else
						{
							getSizeOfText(ttf12, Language::get(3623), &width, nullptr);
							(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y - 12, Language::get(3623)) : SDL_Rect{};
							getSizeOfText(ttf12, Language::get(3624 + followerToCommand->monsterAllyPickupItems), &width, nullptr);
							(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, Language::get(3624 + followerToCommand->monsterAllyPickupItems)) : SDL_Rect{};
						}
						switch ( followerToCommand->monsterAllyPickupItems )
						{
							case ALLY_GYRO_DETECT_ITEMS_METAL:
								if ( i == highlight )
								{
									panelIcons[i]->path = iconEntries["tinker_detect_metal"].path_active_hover;
									setFollowerBannerText(gui_player, bannerTxt, "tinker_detect_metal", "default", textHighlightColor);
								}
								else
								{
									panelIcons[i]->path = iconEntries["tinker_detect_metal"].path_active;
								}
								break;
							case ALLY_GYRO_DETECT_ITEMS_MAGIC:
								if ( i == highlight )
								{
									panelIcons[i]->path = iconEntries["tinker_detect_magic"].path_active_hover;
									setFollowerBannerText(gui_player, bannerTxt, "tinker_detect_magic", "default", textHighlightColor);
								}
								else
								{
									panelIcons[i]->path = iconEntries["tinker_detect_magic"].path_active;
								}
								break;
							case ALLY_GYRO_DETECT_ITEMS_VALUABLE:
								if ( i == highlight )
								{
									panelIcons[i]->path = iconEntries["tinker_detect_valuable"].path_active_hover;
									setFollowerBannerText(gui_player, bannerTxt, "tinker_detect_valuable", "default", textHighlightColor);
								}
								else
								{
									panelIcons[i]->path = iconEntries["tinker_detect_valuable"].path_active;
								}
								break;
							case ALLY_GYRO_DETECT_MONSTERS:
								if ( i == highlight )
								{
									panelIcons[i]->path = iconEntries["tinker_detect_monsters"].path_active_hover;
									setFollowerBannerText(gui_player, bannerTxt, "tinker_detect_monsters", "default", textHighlightColor);
								}
								else
								{
									panelIcons[i]->path = iconEntries["tinker_detect_monsters"].path_active;
								}
								break;
							case ALLY_GYRO_DETECT_EXITS:
								if ( i == highlight )
								{
									panelIcons[i]->path = iconEntries["tinker_detect_exits"].path_active_hover;
									setFollowerBannerText(gui_player, bannerTxt, "tinker_detect_exits", "default", textHighlightColor);
								}
								else
								{
									panelIcons[i]->path = iconEntries["tinker_detect_exits"].path_active;
								}
								break;
							case ALLY_GYRO_DETECT_TRAPS:
								if ( i == highlight )
								{
									panelIcons[i]->path = iconEntries["tinker_detect_traps"].path_active_hover;
									setFollowerBannerText(gui_player, bannerTxt, "tinker_detect_traps", "default", textHighlightColor);
								}
								else
								{
									panelIcons[i]->path = iconEntries["tinker_detect_traps"].path_active;
								}
								break;
							case ALLY_GYRO_DETECT_NONE:
							default:
								if ( i == highlight )
								{
									panelIcons[i]->path = iconEntries["tinker_detect_off"].path_hover;
									setFollowerBannerText(gui_player, bannerTxt, "tinker_detect_off", "default", textHighlightColor);
								}
								else
								{
									panelIcons[i]->path = iconEntries["tinker_detect_off"].path;
								}
								break;
						}
					}
					else
					{
						// draw higher.
						getSizeOfText(ttf12, "Pickup", &width, nullptr);
						(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y - 24, Language::get(3037 + i)) : SDL_Rect{};
						getSizeOfText(ttf12, Language::get(3056 + followerToCommand->monsterAllyPickupItems), &width, nullptr);
						(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y + 12, Language::get(3056 + followerToCommand->monsterAllyPickupItems)) : SDL_Rect{};
						switch ( followerToCommand->monsterAllyPickupItems )
						{
							case ALLY_PICKUP_ALL:
								if ( i == highlight )
								{
									panelIcons[i]->path = iconEntries["leader_pickup_all"].path_active_hover;
									setFollowerBannerText(gui_player, bannerTxt, "leader_pickup_all", "default", textHighlightColor);
								}
								else
								{
									panelIcons[i]->path = iconEntries["leader_pickup_all"].path_active;
								}
								break;
							case ALLY_PICKUP_NONPLAYER:
								if ( i == highlight )
								{
									panelIcons[i]->path = iconEntries["leader_pickup_unowned"].path_active_hover;
									setFollowerBannerText(gui_player, bannerTxt, "leader_pickup_unowned", "default", textHighlightColor);
								}
								else
								{
									panelIcons[i]->path = iconEntries["leader_pickup_unowned"].path_active;
								}
								break;
							case ALLY_PICKUP_NONE:
							default:
								if ( i == highlight )
								{
									panelIcons[i]->path = iconEntries["leader_pickup_unowned"].path_hover;
									setFollowerBannerText(gui_player, bannerTxt, "leader_pickup_unowned", "leader_pickup_none", textHighlightColor);
								}
								else
								{
									panelIcons[i]->path = iconEntries["leader_pickup_unowned"].path;
								}
								break;
						}
					}
				}
				else if ( i == ALLY_CMD_DROP_EQUIP )
				{
					if ( followerStats->type == GYROBOT )
					{
						getSizeOfText(ttf12, Language::get(3633), &width, nullptr);
						(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y - 12, Language::get(3633)) : SDL_Rect{};
						getSizeOfText(ttf12, Language::get(3634), &width, nullptr);
						(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, Language::get(3634)) : SDL_Rect{};
						if ( i == highlight )
						{
							panelIcons[i]->path = iconEntries["tinker_drop"].path_hover;
							setFollowerBannerText(gui_player, bannerTxt, "tinker_drop", "default", textHighlightColor);
						}
						else
						{
							panelIcons[i]->path = iconEntries["tinker_drop"].path;
						}
					}
					else
					{
						(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y - 12, Language::get(3037 + i)) : SDL_Rect{};
						if ( skillLVL >= SKILL_LEVEL_LEGENDARY )
						{
							getSizeOfText(ttf12, Language::get(3061), &width, nullptr);
							(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, Language::get(3061)) : SDL_Rect{};
							if ( i == highlight )
							{
								setFollowerBannerText(gui_player, bannerTxt, "leader_drop", "leader_drop_all", textHighlightColor);
							}
						}
						else if ( skillLVL >= SKILL_LEVEL_MASTER )
						{
							getSizeOfText(ttf12, Language::get(3060), &width, nullptr);
							(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, Language::get(3060)) : SDL_Rect{};
							if ( i == highlight )
							{
								setFollowerBannerText(gui_player, bannerTxt, "leader_drop", "leader_drop_equipment", textHighlightColor);
							}
						}
						else
						{
							getSizeOfText(ttf12, Language::get(3059), &width, nullptr);
							(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, Language::get(3059)) : SDL_Rect{};
							if ( i == highlight )
							{
								setFollowerBannerText(gui_player, bannerTxt, "leader_drop", "leader_drop_weapon", textHighlightColor);
							}
						}
						if ( i == highlight )
						{
							panelIcons[i]->path = iconEntries["leader_drop"].path_hover;
						}
						else
						{
							panelIcons[i]->path = iconEntries["leader_drop"].path;
						}
					}
				}
				else if ( i == ALLY_CMD_SPECIAL )
				{
					if ( followerStats->type == GYROBOT )
					{
						getSizeOfText(ttf12, "Return &", &width, nullptr);
						(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y - 12, Language::get(3635)) : SDL_Rect{};
						if ( i == highlight )
						{
							panelIcons[i]->path = iconEntries["tinker_return_and_land"].path_hover;
							setFollowerBannerText(gui_player, bannerTxt, "tinker_return_and_land", "default", textHighlightColor);
						}
						else
						{
							panelIcons[i]->path = iconEntries["tinker_return_and_land"].path;
						}
					}
					else if ( followerStats->type == DUMMYBOT )
					{
						getSizeOfText(ttf12, Language::get(3641), &width, nullptr);
						(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y - 12, Language::get(3641)) : SDL_Rect{};
						getSizeOfText(ttf12, Language::get(3642), &width, nullptr);
						(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, Language::get(3642)) : SDL_Rect{};
						if ( i == highlight )
						{
							panelIcons[i]->path = iconEntries["tinker_deactivate"].path_hover;
							setFollowerBannerText(gui_player, bannerTxt, "tinker_deactivate", "dummybot_deactivate", textHighlightColor);
						}
						else
						{
							panelIcons[i]->path = iconEntries["tinker_deactivate"].path;
						}
					}
					else if ( followerStats->type == SENTRYBOT || followerStats->type == SPELLBOT )
					{
						getSizeOfText(ttf12, Language::get(3649), &width, nullptr);
						(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, Language::get(3649)) : SDL_Rect{};
						if ( i == highlight )
						{
							panelIcons[i]->path = iconEntries["tinker_deactivate"].path_hover;
							setFollowerBannerText(gui_player, bannerTxt, "tinker_deactivate", "default", textHighlightColor);
						}
						else
						{
							panelIcons[i]->path = iconEntries["tinker_deactivate"].path;
						}
					}
					else
					{
						getSizeOfText(ttf12, Language::get(3037 + i), &width, nullptr);
						(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, Language::get(3037 + i)) : SDL_Rect{};
						// rest
						if ( i == highlight )
						{
							panelIcons[i]->path = iconEntries["leader_rest"].path_hover;
							setFollowerBannerText(gui_player, bannerTxt, "leader_rest", "default", textHighlightColor);
						}
						else
						{
							panelIcons[i]->path = iconEntries["leader_rest"].path;
						}
					}
				}
				else if ( i == ALLY_CMD_ATTACK_SELECT )
				{
					if ( !attackCommandOnly(followerStats->type) )
					{
						if ( optionDisabledForCreature(skillLVL, followerStats->type, ALLY_CMD_ATTACK_CONFIRM, followerToCommand) == 0 )
						{
							getSizeOfText(ttf12, "Interact / ", &width, nullptr);
							(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y - 12, Language::get(3051)) : SDL_Rect{};
							if ( i == highlight )
							{
								panelIcons[i]->path = iconEntries["leader_attack_or_interact"].path_hover;
								setFollowerBannerText(gui_player, bannerTxt, "leader_attack_or_interact", "default", textHighlightColor);
							}
							else
							{
								panelIcons[i]->path = iconEntries["leader_attack_or_interact"].path;
							}
						}
						else
						{
							getSizeOfText(ttf12, Language::get(3037 + i), &width, nullptr);
							(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y + 4, Language::get(3037 + i)) : SDL_Rect{};
							if ( tinkeringFollower )
							{
								if ( i == highlight )
								{
									panelIcons[i]->path = iconEntries["tinker_interact"].path_hover;
									setFollowerBannerText(gui_player, bannerTxt, "tinker_interact", "default", textHighlightColor);
								}
								else
								{
									panelIcons[i]->path = iconEntries["tinker_interact"].path;
								}
							}
							else
							{
								if ( i == highlight )
								{
									panelIcons[i]->path = iconEntries["leader_interact"].path_hover;
									setFollowerBannerText(gui_player, bannerTxt, "leader_interact", "default", textHighlightColor);
								}
								else
								{
									panelIcons[i]->path = iconEntries["leader_interact"].path;
								}
							}
						}
					}
					else
					{
						getSizeOfText(ttf12, Language::get(3104), &width, nullptr); // print just attack if no world interaction.
						(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, Language::get(3104)) : SDL_Rect{};
						if ( tinkeringFollower )
						{
							if ( i == highlight )
							{
								panelIcons[i]->path = iconEntries["tinker_attack"].path_hover;
								setFollowerBannerText(gui_player, bannerTxt, "tinker_attack", "default", textHighlightColor);
							}
							else
							{
								panelIcons[i]->path = iconEntries["tinker_attack"].path;
							}
						}
						else
						{
							if ( i == highlight )
							{
								panelIcons[i]->path = iconEntries["leader_attack"].path_hover;
								setFollowerBannerText(gui_player, bannerTxt, "leader_attack", "default", textHighlightColor);
							}
							else
							{
								panelIcons[i]->path = iconEntries["leader_attack"].path;
							}
						}
					}
				}
				else if ( i == ALLY_CMD_MOVETO_SELECT )
				{
					if ( followerStats->type == SENTRYBOT || followerStats->type == SPELLBOT )
					{
						getSizeOfText(ttf12, Language::get(3650), &width, nullptr);
						(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, Language::get(3650)) : SDL_Rect{};
						if ( i == highlight )
						{
							panelIcons[i]->path = iconEntries["tinker_lookat"].path_hover;
							setFollowerBannerText(gui_player, bannerTxt, "tinker_lookat", "default", textHighlightColor);
						}
						else
						{
							panelIcons[i]->path = iconEntries["tinker_lookat"].path;
						}
					}
					else
					{
						getSizeOfText(ttf12, Language::get(3037 + i), &width, nullptr);
						(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, Language::get(3037 + i)) : SDL_Rect{};
						if ( tinkeringFollower )
						{
							if ( i == highlight )
							{
								panelIcons[i]->path = iconEntries["tinker_moveto"].path_hover;
								setFollowerBannerText(gui_player, bannerTxt, "tinker_moveto", "default", textHighlightColor);
							}
							else
							{
								panelIcons[i]->path = iconEntries["tinker_moveto"].path;
							}
						}
						else
						{
							if ( i == highlight )
							{
								panelIcons[i]->path = iconEntries["leader_moveto"].path_hover;
								setFollowerBannerText(gui_player, bannerTxt, "leader_moveto", "default", textHighlightColor);
							}
							else
							{
								panelIcons[i]->path = iconEntries["leader_moveto"].path;
							}
						}
					}
				}
				else
				{
					getSizeOfText(ttf12, Language::get(3037 + i), &width, nullptr);
					(*cvar_showoldwheel) ? ttfPrintText(ttf12, txt.x - width / 2, txt.y - 4, Language::get(3037 + i)) : SDL_Rect{};
					if ( i == ALLY_CMD_DEFEND )
					{
						// "wait"
						if ( tinkeringFollower )
						{
							if ( i == highlight )
							{
								panelIcons[i]->path = iconEntries["tinker_wait"].path_hover;
								if ( modifierPressed && commandCanBeSentToAll(i) )
								{
									modifierActiveForOption = true;
									setFollowerBannerText(gui_player, bannerTxt, "tinker_wait", "wait_here_all", textHighlightColor);
								}
								else
								{
									setFollowerBannerText(gui_player, bannerTxt, "tinker_wait", "wait_here", textHighlightColor);
								}
							}
							else
							{
								panelIcons[i]->path = iconEntries["tinker_wait"].path;
							}
						}
						else
						{
							if ( i == highlight )
							{
								panelIcons[i]->path = iconEntries["leader_wait"].path_hover;
								if ( modifierPressed && commandCanBeSentToAll(i) )
								{
									modifierActiveForOption = true;
									setFollowerBannerText(gui_player, bannerTxt, "leader_wait", "wait_here_all", textHighlightColor);
								}
								else
								{
									setFollowerBannerText(gui_player, bannerTxt, "leader_wait", "wait_here", textHighlightColor);
								}
							}
							else
							{
								panelIcons[i]->path = iconEntries["leader_wait"].path;
							}
						}
					}
					else if ( i == ALLY_CMD_MOVEASIDE )
					{
						if ( tinkeringFollower )
						{
							if ( i == highlight )
							{
								panelIcons[i]->path = iconEntries["tinker_moveaside"].path_hover;
								setFollowerBannerText(gui_player, bannerTxt, "tinker_moveaside", "default", textHighlightColor);
							}
							else
							{
								panelIcons[i]->path = iconEntries["tinker_moveaside"].path;
							}
						}
						else
						{
							if ( i == highlight )
							{
								panelIcons[i]->path = iconEntries["leader_moveaside"].path_hover;
								setFollowerBannerText(gui_player, bannerTxt, "leader_moveaside", "default", textHighlightColor);
							}
							else
							{
								panelIcons[i]->path = iconEntries["leader_moveaside"].path;
							}
						}
					}
				}
			}

			if ( lockedOption )
			{
				if ( highlight == i && !mouseInCenterButton )
				{
					panelImages[i]->path = getPanelEntriesForFollower(tinkeringFollower)[i].path_locked_hover;
				}
				else
				{
					panelImages[i]->path = getPanelEntriesForFollower(tinkeringFollower)[i].path_locked;
				}
			}
			else if ( highlight == i && !mouseInCenterButton )
			{
				panelImages[i]->path = getPanelEntriesForFollower(tinkeringFollower)[i].path_hover;
			}

			if ( !lockedOption && panelIcons[i]->path != "" )
			{
				if ( auto imgGet = Image::get(panelIcons[i]->path.c_str()) )
				{
					panelIcons[i]->disabled = false;
					panelIcons[i]->pos.w = imgGet->getWidth();
					panelIcons[i]->pos.h = imgGet->getHeight();
					panelIcons[i]->pos.x -= panelIcons[i]->pos.w / 2;
					panelIcons[i]->pos.y -= panelIcons[i]->pos.h / 2;
				}
			}

			angleStart += 2 * PI / numoptions;
			angleMiddle = angleStart + PI / numoptions;
			angleEnd = angleMiddle + PI / numoptions;
		}

		// draw center text.
		if ( mouseInCenterButton )
		{
			bool mouseInCenterHighlightArea = sqrt(pow((omousex - menuX), 2) + pow((omousey - menuY), 2)) < (radius - thickness + centerButtonHighlightOffset);
			if ( mouseInCenterHighlightArea )
			{
				panelImages[PANEL_DIRECTION_END]->path = getPanelEntriesForFollower(tinkeringFollower)[PANEL_DIRECTION_END].path_hover;
			}

			highlight = -1;
			//drawImageRing(fancyWindow_bmp, nullptr, 35, 35, 40, 0, 2 * PI, 192);
			if ( *cvar_showoldwheel )
			{
				drawCircle(centerx, centery, radius - thickness, uint32ColorBaronyBlue, 192);
			}
			//getSizeOfText(ttf12, Language::get(3063), &width, nullptr);
			//ttfPrintText(ttf12, centerx - width / 2, centery - 8, Language::get(3063));
		}

		if ( optionSelected == -1 && disableOption == 0 && highlight != -1 )
		{
			// in case optionSelected is cleared, but we're still highlighting text (happens on next frame when clicking on disabled option.)
			if ( highlight == ALLY_CMD_ATTACK_SELECT )
			{
				if ( attackCommandOnly(followerStats->type) )
				{
					// attack only.
					disableOption = optionDisabledForCreature(skillLVL, followerStats->type, ALLY_CMD_ATTACK_CONFIRM, followerToCommand);
				}
				else
				{
					disableOption = optionDisabledForCreature(skillLVL, followerStats->type, highlight, followerToCommand);
				}
			}
			else
			{
				disableOption = optionDisabledForCreature(skillLVL, followerStats->type, highlight, followerToCommand);
			}
		}

		if ( highlight == -1 )
		{
			setFollowerBannerText(gui_player, bannerTxt, "cancel", "default", hudColors.characterSheetRed);
		}

		bool disableActionGlyph = false;
		bool missingSkillLevel = false;
		if ( disableOption != 0 )
		{
			disableActionGlyph = true;

			if ( disableOption == -2 ) // disabled due to cooldown
			{
				setFollowerBannerText(gui_player, bannerTxt, "invalid_action", "rest_cooldown", hudColors.characterSheetRed);
			}
			else if ( disableOption == -1 ) // disabled due to creature type
			{
				auto& textMap = FollowerMenu[gui_player].iconEntries["invalid_action"].text_map["command_unavailable"];
				setFollowerBannerTextFormatted(gui_player, bannerTxt, hudColors.characterSheetRed,
					textMap.second, textMap.first.c_str(),
					getMonsterLocalizedName(followerStats->type).c_str());
			}
			else if ( disableOption == -3 ) // disabled due to tinkerbot quality
			{
				auto& textMap = FollowerMenu[gui_player].iconEntries["invalid_action"].text_map["tinker_quality_low"];
				setFollowerBannerTextFormatted(gui_player, bannerTxt, hudColors.characterSheetRed,
					textMap.second, textMap.first.c_str(),
					getMonsterLocalizedName(followerStats->type).c_str());
			}
			else
			{
				std::string requirement = "";
				std::string current = "";
				int requirementVal = 0;
				int currentVal = 0;
				if ( highlight >= ALLY_CMD_DEFEND && highlight <= ALLY_CMD_END && highlight != ALLY_CMD_CANCEL )
				{
					switch ( std::min(disableOption, SKILL_LEVEL_LEGENDARY) )
					{
						case 0:
							requirement = Language::get(363);
							requirementVal = 0;
							break;
						case SKILL_LEVEL_NOVICE:
							requirement = Language::get(364);
							requirementVal = SKILL_LEVEL_NOVICE;
							break;
						case SKILL_LEVEL_BASIC:
							requirement = Language::get(365);
							requirementVal = SKILL_LEVEL_BASIC;
							break;
						case SKILL_LEVEL_SKILLED:
							requirement = Language::get(366);
							requirementVal = SKILL_LEVEL_SKILLED;
							break;
						case SKILL_LEVEL_EXPERT:
							requirement = Language::get(367);
							requirementVal = SKILL_LEVEL_EXPERT;
							break;
						case SKILL_LEVEL_MASTER:
							requirement = Language::get(368);
							requirementVal = SKILL_LEVEL_MASTER;
							break;
						case SKILL_LEVEL_LEGENDARY:
							requirement = Language::get(369);
							requirementVal = SKILL_LEVEL_LEGENDARY;
							break;
						default:
							break;
					}
					requirement.erase(std::remove(requirement.begin(), requirement.end(), ' '), requirement.end()); // trim whitespace

					if ( skillLVL >= SKILL_LEVEL_LEGENDARY )
					{
						current = Language::get(369);
					}
					else if ( skillLVL >= SKILL_LEVEL_MASTER )
					{
						current = Language::get(368);
					}
					else if ( skillLVL >= SKILL_LEVEL_EXPERT )
					{
						current = Language::get(367);
					}
					else if ( skillLVL >= SKILL_LEVEL_SKILLED )
					{
						current = Language::get(366);
					}
					else if ( skillLVL >= SKILL_LEVEL_BASIC )
					{
						current = Language::get(365);
					}
					else if ( skillLVL >= SKILL_LEVEL_NOVICE )
					{
						current = Language::get(364);
					}
					else
					{
						current = Language::get(363);
					}
					current.erase(std::remove(current.begin(), current.end(), ' '), current.end()); // trim whitespace
					currentVal = skillLVL;
				}

				if ( tinkeringFollower )
				{
					auto& textMap = FollowerMenu[gui_player].iconEntries["invalid_action"].text_map["skill_missing_tinker"];
					setFollowerBannerTextFormatted(gui_player, bannerTxt, hudColors.characterSheetRed,
						textMap.second, textMap.first.c_str(),
						currentVal, requirementVal);
				}
				else
				{
					auto& textMap = FollowerMenu[gui_player].iconEntries["invalid_action"].text_map["skill_missing_leader"];
					setFollowerBannerTextFormatted(gui_player, bannerTxt, hudColors.characterSheetRed,
						textMap.second,	textMap.first.c_str(),
						currentVal, requirementVal);
				}
				missingSkillLevel = true;
			}
		}

		auto wheelSkillImg = bannerFrame->findImage("skill img");
		wheelSkillImg->disabled = true;
		auto wheelStatImg = bannerFrame->findImage("stat img");
		wheelStatImg->disabled = true;
		for ( auto& skill : Player::SkillSheet_t::skillSheetData.skillEntries )
		{
			if ( (tinkeringFollower && skill.skillId == PRO_LOCKPICKING)
				|| (!tinkeringFollower && skill.skillId == PRO_LEADERSHIP) )
			{
				if ( skillCapstoneUnlocked(gui_player, skill.skillId) )
				{
					wheelSkillImg->path = skill.skillIconPathLegend;
				}
				else
				{
					wheelSkillImg->path = skill.skillIconPath;
				}
				wheelStatImg->path = skill.statIconPath;
				if ( auto imgGet = Image::get(wheelSkillImg->path.c_str()) )
				{
					wheelSkillImg->pos.w = imgGet->getWidth();
					wheelSkillImg->pos.h = imgGet->getHeight();
				}
				if ( auto imgGet = Image::get(wheelStatImg->path.c_str()) )
				{
					wheelStatImg->pos.w = imgGet->getWidth();
					wheelStatImg->pos.h = imgGet->getHeight();
				}
				break;
			}
		}

		bannerFrame->setDisabled(false);
		if ( auto textGet = bannerTxt->getTextObject() )
		{
			SDL_Rect txtPos = bannerTxt->getSize();
			if ( !strcmp(bannerTxt->getText(), "") && txtPos.w == 0 )
			{
				txtPos.w = 82;
			}
			else if ( strcmp(bannerTxt->getText(), "") )
			{
				txtPos.w = textGet->getWidth();
			}

			auto bannerGlyph = bannerFrame->findImage("banner glyph");
			bannerGlyph->disabled = true;
			if ( inputs.hasController(gui_player) )
			{
				bannerGlyph->path = Input::inputs[gui_player].getGlyphPathForBinding("MenuConfirm");
			}
			else
			{
				bannerGlyph->path = Input::inputs[gui_player].getGlyphPathForBinding("MenuLeftClick");
			}
			//bannerGlyph->path = Input::inputs[gui_player].getGlyphPathForBinding("Use");
			auto bannerGlyphModifier = bannerFrame->findImage("banner modifier glyph");
			bannerGlyphModifier->disabled = true;
			bannerGlyphModifier->path = Input::inputs[gui_player].getGlyphPathForBinding("Defend");
			if ( auto imgGet = Image::get(bannerGlyph->path.c_str()) )
			{
				bannerGlyph->pos.w = imgGet->getWidth();
				bannerGlyph->pos.h = imgGet->getHeight();
				bannerGlyph->disabled = disableActionGlyph || !strcmp(bannerTxt->getText(), "");
			}
			if ( auto imgGet = Image::get(bannerGlyphModifier->path.c_str()) )
			{
				bannerGlyphModifier->pos.w = imgGet->getWidth();
				bannerGlyphModifier->pos.h = imgGet->getHeight();
				bannerGlyphModifier->disabled = bannerGlyph->disabled || !modifierActiveForOption;
			}

			if ( !bannerGlyph->disabled )
			{
				animInvalidAction = 0.0;
			}

			bannerImgCenter->pos.w = txtPos.w + 16 
				+ (bannerGlyph->disabled ? 0 : ((bannerGlyph->pos.w + 8) / 2))
				+ (bannerGlyphModifier->disabled ? 0 : (bannerGlyphModifier->pos.w + 2));
			int missingSkillLevelIconWidth = 0;
			if ( missingSkillLevel )
			{
				missingSkillLevelIconWidth = wheelStatImg->pos.w + wheelSkillImg->pos.w + 8;
			}
			bannerImgCenter->pos.w += missingSkillLevelIconWidth / 2;
			const int totalWidth = bannerImgLeft->pos.w + bannerImgRight->pos.w + bannerImgCenter->pos.w;

			const int midx = followerFrame->getSize().w / 2;
			const int midy = followerFrame->getSize().h / 2;

			SDL_Rect bannerSize = bannerFrame->getSize();
			bannerSize.w = totalWidth;
			bannerSize.x = midx - (totalWidth / 2);
			bannerSize.y = midy + FollowerRadialMenu::followerWheelRadius + FollowerRadialMenu::followerWheelButtonThickness + 4;
			if ( players[gui_player]->bUseCompactGUIHeight() )
			{
				bannerSize.y -= 16;
			}
			//bannerSize.y += 32 * (1.0 - animTitle);
			bannerFrame->setSize(bannerSize);
			bannerFrame->setOpacity(100.0 * animTitle);
			bannerImgLeft->pos.x = 0;
			bannerImgCenter->pos.x = bannerImgLeft->pos.x + bannerImgLeft->pos.w;
			bannerImgRight->pos.x = bannerImgCenter->pos.x + bannerImgCenter->pos.w;

			txtPos.x = bannerImgCenter->pos.x + (bannerImgCenter->pos.w / 2) - (txtPos.w / 2);
			txtPos.x += bannerGlyph->disabled ? 0 : ((bannerGlyph->pos.w + 8) / 2);
			txtPos.x += bannerGlyphModifier->disabled ? 0 : ((bannerGlyphModifier->pos.w + 0) / 2);
			if ( missingSkillLevel )
			{
				txtPos.x -= (missingSkillLevelIconWidth / 2) - 4;
			}
			if ( txtPos.x % 2 == 1 )
			{
				++txtPos.x;
			}
			if ( animInvalidAction > 0.01 )
			{
				txtPos.x += -2 + 2 * (cos(animInvalidAction * 4 * PI));
			}
			txtPos.y = 17;
			bannerTxt->setSize(txtPos);

			if ( missingSkillLevel )
			{
				wheelSkillImg->pos.x = txtPos.x + txtPos.w;
				wheelSkillImg->pos.y = txtPos.y - 3;
				wheelSkillImg->disabled = false;

				wheelStatImg->pos.x = wheelSkillImg->pos.x + wheelSkillImg->pos.w;
				wheelStatImg->pos.y = wheelSkillImg->pos.y;
				wheelStatImg->disabled = false;
			}

			bannerGlyph->pos.x = txtPos.x - bannerGlyph->pos.w - 8;
			if ( bannerGlyph->pos.x % 2 == 1 )
			{
				++bannerGlyph->pos.x;
			}
			bannerGlyph->pos.y = txtPos.y + txtPos.h / 2 - bannerGlyph->pos.h / 2;
			if ( bannerGlyph->pos.y % 2 == 1 )
			{
				bannerGlyph->pos.y -= 1;
			}
			bannerSize.h = std::max(40, bannerGlyph->pos.y + bannerGlyph->pos.h);
			if ( !bannerGlyphModifier->disabled )
			{
				bannerGlyphModifier->pos.x = txtPos.x - bannerGlyphModifier->pos.w - 8;
				bannerGlyph->pos.x = bannerGlyphModifier->pos.x - bannerGlyph->pos.w - 2;

				if ( bannerGlyphModifier->pos.x % 2 == 1 )
				{
					++bannerGlyphModifier->pos.x;
				}
				bannerGlyphModifier->pos.y = txtPos.y + txtPos.h / 2 - bannerGlyphModifier->pos.h / 2;
				if ( bannerGlyphModifier->pos.y % 2 == 1 )
				{
					bannerGlyphModifier->pos.y -= 1;
				}
				bannerSize.h = std::max(bannerSize.h, bannerGlyphModifier->pos.y + bannerGlyphModifier->pos.h);
			}
			bannerFrame->setSize(bannerSize);

			auto wheelTitleText = bgFrame->findField("wheel title");
			if ( followerStats )
			{
				char buf[128] = "";
				int spaces = 0;
				int spaces2 = 0;
				for ( int c = 0; c <= strlen(Language::get(4200)); ++c )
				{
					if ( Language::get(4200)[c] == '\0' )
					{
						break;
					}
					if ( Language::get(4200)[c] == ' ' )
					{
						++spaces;
					}
				}
				if ( strcmp(followerStats->name, "") && strcmp(followerStats->name, "nothing") )
				{
					snprintf(buf, sizeof(buf), Language::get(4200), followerStats->name);
				}
				else
				{
					snprintf(buf, sizeof(buf), Language::get(4200), getMonsterLocalizedName(followerStats->type).c_str());
				}

				for ( int c = 0; c <= strlen(buf); ++c )
				{
					if ( buf[c] == '\0' )
					{
						break;
					}
					if ( buf[c] == ' ' )
					{
						++spaces2;
					}
				}
				wheelTitleText->setText(buf);
				wheelTitleText->clearWordsToHighlight();
				int wordIndex = 1;
				while ( spaces2 >= spaces ) // every additional space means +1 word to highlight for the monster's name
				{
					wheelTitleText->addWordToHighlight(wordIndex, followerTitleHighlightColor);
					--spaces2;
					++wordIndex;
				}
			}
			SDL_Rect titlePos = wheelTitleText->getSize();
			if ( auto textGet2 = wheelTitleText->getTextObject() )
			{
				titlePos.w = textGet2->getWidth();
				titlePos.x = bannerSize.x + bannerSize.w / 2 - (titlePos.w / 2);
				if ( titlePos.x % 2 == 1 )
				{
					++titlePos.x;
				}
				titlePos.y = midy - FollowerRadialMenu::followerWheelRadius - FollowerRadialMenu::followerWheelButtonThickness - 24;
				titlePos.y -= 32 * (1.0 - animTitle);
				++titlePos.y; // add 1 to be even pixeled
				wheelTitleText->setSize(titlePos);
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
				createParticleFollowerCommand(recentEntity->x, recentEntity->y, 0, FOLLOWER_SELECTED_PARTICLE,
					recentEntity->getUID());
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
	int skillLVL = stats[gui_player]->getModifiedProficiency(PRO_LEADERSHIP) + statGetCHR(stats[gui_player], players[gui_player]->entity);
	if ( tinkeringFollower )
	{
		skillLVL = stats[gui_player]->getModifiedProficiency(PRO_LOCKPICKING) + statGetPER(stats[gui_player], players[gui_player]->entity);
	}
	if ( followerToCommand->monsterAllySummonRank != 0 )
	{
		skillLVL = SKILL_LEVEL_LEGENDARY;
	}

	bool enableAttack = (optionDisabledForCreature(skillLVL, followerStats->type, ALLY_CMD_ATTACK_CONFIRM, followerToCommand) == 0);
	
	if ( !interactItems && !interactWorld && enableAttack )
	{
		if ( updateInteractText )
		{
			if ( Input::inputs[gui_player].binary("Defend") )
			{
				strcpy(interactText, Language::get(4201)); //"(ALL) "
				strcat(interactText, Language::get(4043)); // "Attack "
			}
			else
			{
				strcpy(interactText, Language::get(4043)); // "Attack "
			}
		}
	}
	else
	{
		if ( updateInteractText )
		{
			strcpy(interactText, Language::get(4014)); // "Interact with "
		}
	}
	if ( selectedEntity.behavior == &actTorch && interactWorld )
	{
		if ( updateInteractText )
		{
			strcat(interactText, items[TOOL_TORCH].getIdentifiedName());
		}
	}
	else if ( (selectedEntity.behavior == &actSwitch || selectedEntity.sprite == 184) && interactWorld )
	{
		if ( updateInteractText )
		{
			strcat(interactText, Language::get(4044)); // "switch"
		}
	}
	else if ( (selectedEntity.behavior == &actTeleportShrine ) && (interactWorld || interactItems || enableAttack) && followerStats->type != GYROBOT )
	{
		if ( updateInteractText )
		{
			if ( !interactItems && !interactWorld && enableAttack )
			{
				strcpy(interactText, Language::get(4014)); // "Interact with "
			}
			strcat(interactText, Language::get(4309)); // "shrine"
		}
	}
	else if ( (selectedEntity.behavior == &actTeleporter) && interactWorld )
	{
		if ( updateInteractText )
		{
			switch ( selectedEntity.teleporterType )
			{
				case 0:
				case 1:
					strcat(interactText, Language::get(4310)); // "ladder"
					break;
				case 2:
					strcat(interactText, Language::get(4311)); // "portal"
					break;
				default:
					break;
			}
		}
	}
	else if ( selectedEntity.behavior == &actBomb && interactWorld && followerStats->type == GYROBOT )
	{
		if ( updateInteractText )
		{
			strcpy(interactText, Language::get(3093));
			strcat(interactText, Language::get(4045)); // "trap"
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
					strcat(interactText, items[selectedEntity.skill[10]].getUnidentifiedName());
				}
				else
				{
					strcat(interactText, items[selectedEntity.skill[10]].getIdentifiedName());
				}
			}
			else
			{
				strcat(interactText, Language::get(4046)); // "item"
			}
		}
	}
	else if ( selectedEntity.behavior == &actMonster 
		&& enableAttack && selectedEntity.getMonsterTypeFromSprite() != GYROBOT
		&& !selectedEntity.isInertMimic() )
	{
		if ( updateInteractText )
		{
			if ( Input::inputs[gui_player].binary("Defend") )
			{
				strcpy(interactText, Language::get(4201)); //"(ALL) "
				strcat(interactText, Language::get(4043)); // "Attack "
			}
			else
			{
				strcpy(interactText, Language::get(4043)); // "Attack "
			}
			int monsterType = selectedEntity.getMonsterTypeFromSprite();
			strcat(interactText, getMonsterLocalizedName((Monster)monsterType).c_str());
		}
	}
	else
	{
		if ( updateInteractText )
		{
			strcpy(interactText, Language::get(4047)); // "No interactions available"
		}
		return false;
	}
	return true;
}

int FollowerRadialMenu::optionDisabledForCreature(int playerSkillLVL, int monsterType, int option, Entity* follower)
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
		case BUGBEAR:
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
	if ( follower )
	{
		followerStats = follower->getStats();
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
		&& follower->monsterAllySpecialCooldown != 0 )
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
			if ( follower && follower->monsterAllySummonRank != 0 )
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
			if ( follower && follower->monsterAllySummonRank != 0 )
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

bool GenericGUIMenu::isItemEnchantArmorable(const Item* item)
{
	if ( !item )
	{
		return false;
	}
	if ( !item->identified )
	{
		return false;
	}

	if ( items[item->type].item_slot != NO_EQUIP )
	{
		if ( items[item->type].item_slot != EQUIPPABLE_IN_SLOT_AMULET
			&& items[item->type].item_slot != EQUIPPABLE_IN_SLOT_RING
			&& items[item->type].item_slot != EQUIPPABLE_IN_SLOT_WEAPON )
		{
			return true;
		}
	}

	return false;
}

bool GenericGUIMenu::isItemEnchantWeaponable(const Item* item)
{
	if ( !item )
	{
		return false;
	}
	if ( !item->identified )
	{
		return false;
	}

	if ( items[item->type].item_slot == EQUIPPABLE_IN_SLOT_WEAPON )
	{
		return true;
		/*if ( itemCategory(item) == WEAPON || itemCategory(item) == THROWN )
		{
		}*/
	}
	else if ( item->type == BRASS_KNUCKLES
		|| item->type == IRON_KNUCKLES
		|| item->type == SPIKED_GAUNTLETS )
	{
		return true;
	}

	return false;
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
				//if ( shouldDisplayItemInGUI(item) )
				//{
				//	++c;
				//}
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
			//	messagePlayer(gui_player, MESSAGE_MISC, Language::get(3338));
			//}
			//else
			//{
			//	messagePlayer(gui_player, MESSAGE_MISC | MESSAGE_INVENTORY, Language::get(3343));
			//}
			//closeGUI();
			//return;
		}
		/*scroll = std::max(0, std::min(scroll, c - kNumShownItems));
		for ( c = 0; c < kNumShownItems; ++c )
		{
			itemsDisplayed[c] = nullptr;
		}
		c = 0;*/

		//Assign the visible items to the GUI slots.
		//for ( node = player_inventory->first; node != nullptr; node = node->next )
		//{
		//	if ( node->element )
		//	{
		//		item = (Item*)node->element;
		//		if ( shouldDisplayItemInGUI(item) ) //Skip over all non-applicable items.
		//		{
		//			++c;
		//			if ( c <= scroll )
		//			{
		//				continue;
		//			}
		//			itemsDisplayed[c - scroll - 1] = item;
		//			if ( c > 3 + scroll )
		//			{
		//				break;
		//			}
		//		}
		//	}
		//}

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

		/*if ( guiType == GUI_TYPE_SCRIBING && featherGUI.isInscriptionDrawerOpen() )
		{
			for ( node = player_inventory->first; node != nullptr; node = node->next )
			{
				if ( node->element )
				{
					item = (Item*)node->element;
					if ( isNodeScribingCraftableItem(node)
						&& item->x >= 0 && item->x < FeatherGUI_t::MAX_FEATHER_X
						&& item->y >= 0 && item->y < FeatherGUI_t::MAX_FEATHER_Y )
					{
						if ( auto slotFrame = featherGUI.getFeatherSlotFrame(item->x, item->y) )
						{
							updateSlotFrameFromItem(slotFrame->findFrame("item slot frame"), item);
						}
					}
				}
			}
		}*/
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
		else if ( guiType == GUI_TYPE_ITEMFX )
		{
			if ( !itemEffectScrollItem && !itemEffectUsingSpell && !itemEffectUsingSpellbook )
			{
				closeGUI();
				return;
			}
			if ( itemEffectScrollItem )
			{
				if ( !itemEffectScrollItem->node )
				{
					closeGUI();
					return;
				}
				else if ( itemEffectScrollItem->node->list != &stats[gui_player]->inventory )
				{
					// dropped out of inventory or something.
					closeGUI();
					return;
				}
			}
		}

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
			//	Language::get(3690));
			//char kitStatusText[64] = "";
			//if ( tinkeringKitItem )
			//{
			//	snprintf(kitStatusText, 63, Language::get(3691), Language::get(3691 + std::max(1, static_cast<int>(tinkeringKitItem->status))));
			//}
			//ttfPrintTextFormatted(ttf12, windowX2 - 16 - (strlen(kitStatusText) + 1) * TTF12_WIDTH, windowY2 - TTF12_HEIGHT - 8,
			//	kitStatusText);

			//ttfPrintTextFormatted(ttf12, windowX1 + 16, windowY2 - TTF12_HEIGHT - 8,
			//	Language::get(3647), numMetalScrap, numMagicScrap);
			//SDL_Rect smallIcon;
			//smallIcon.x = windowX1 + 16 + (strlen(Language::get(3647)) - 5) * TTF12_WIDTH;
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
			//getSizeOfText(ttf8, Language::get(3644), &txtWidth, &txtHeight);
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
			//ttfPrintText(font, highlightBtn.x + 4 + charWidth, pos.y - (8 - txtHeight), Language::get(3644));

			//// Salvage
			//getSizeOfText(font, Language::get(3645), &txtWidth, &txtHeight);
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
			//ttfPrintText(font, highlightBtn.x + 4 + charWidth, pos.y - (8 - txtHeight), Language::get(3645));

			//// Repair
			//getSizeOfText(font, Language::get(3646), &txtWidth, &txtHeight);
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
			//ttfPrintText(font, highlightBtn.x + 4 + charWidth, pos.y - (8 - txtHeight), Language::get(3646));

			//// Filter include all (*)
			//getSizeOfText(font, Language::get(356), &txtWidth, &txtHeight);
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
			//ttfPrintText(font, highlightBtn.x + (highlightBtn.w - txtWidth) / 2, pos.y - (8 - txtHeight), Language::get(356));
		}
		else if ( guiType == GUI_TYPE_SCRIBING )
		{
			//windowX1 -= 20;
			//windowX2 += 20;
			//windowY1 -= 40;
			//windowY2 += 40;
			//drawWindowFancy(windowX1, windowY1, windowX2, windowY2);

			//// title
			//ttfPrintTextFormatted(ttf12, windowX1 + 16, windowY1 + 8,
			//	Language::get(3716));
			//char toolStatusText[64] = "";
			//if ( scribingToolItem && scribingToolItem->identified )
			//{
			//	snprintf(toolStatusText, 63, Language::get(3717), scribingToolItem->appearance % ENCHANTED_FEATHER_MAX_DURABILITY);
			//}
			//ttfPrintTextFormatted(ttf12, windowX2 - 16 - (strlen(toolStatusText) + 1) * TTF12_WIDTH, windowY2 - TTF12_HEIGHT - 8,
			//	toolStatusText);
			///*if ( scribingLastUsageDisplayTimer > 0 )
			//{
			//	ttfPrintTextFormattedColor(ttf12, windowX2 - 16 - 11 * TTF12_WIDTH, windowY2 - TTF12_HEIGHT - 8, uint32ColorRed,
			//			"(%3d)", -scribingLastUsageAmount);
			//}*/

			//if ( scribingFilter == SCRIBING_FILTER_CRAFTABLE )
			//{
			//	if ( scribingBlankScrollTarget )
			//	{
			//		snprintf(tempstr, 1024, Language::get(3722), scribingBlankScrollTarget->beatitude, items[SCROLL_BLANK].name_identified);
			//		ttfPrintTextFormatted(ttf12, windowX1 + 16, windowY2 - 2 * TTF12_HEIGHT - 8, tempstr);

			//		SDL_Rect smallIcon;
			//		smallIcon.x = windowX1 + 16 + (longestline(tempstr) - 5) * TTF12_WIDTH;
			//		smallIcon.y = windowY2 - TTF12_HEIGHT - 12 - 4;
			//		smallIcon.h = 16;
			//		smallIcon.w = 16;
			//		node_t* imageNode = items[SCROLL_BLANK].surfaces.first;
			//		if ( imageNode )
			//		{
			//			drawImageScaled(*((SDL_Surface**)imageNode->element), NULL, &smallIcon);
			//		}
			//		smallIcon.x += smallIcon.w + 4;
			//		smallIcon.w = longestline(Language::get(3723)) * TTF12_WIDTH + 8;
			//		smallIcon.y -= 2;
			//		smallIcon.h += 2;
			//		if ( mouseInBounds(gui_player, smallIcon.x, smallIcon.x + smallIcon.w, smallIcon.y, smallIcon.y + smallIcon.h) )
			//		{
			//			drawDepressed(smallIcon.x, smallIcon.y, smallIcon.x + smallIcon.w, smallIcon.y + smallIcon.h);
			//			if ( (inputs.bMouseLeft(gui_player) || inputs.bControllerInputPressed(gui_player, INJOY_MENU_USE)) )
			//			{
			//				inputs.controllerClearInput(gui_player, INJOY_MENU_USE);
			//				inputs.mouseClearLeft(gui_player);
			//				scribingBlankScrollTarget = nullptr;
			//			}
			//		}
			//		else
			//		{
			//			drawWindow(smallIcon.x, smallIcon.y, smallIcon.x + smallIcon.w, smallIcon.y + smallIcon.h);
			//		}
			//		ttfPrintTextFormatted(ttf12, smallIcon.x + 6, windowY2 - 2 * TTF12_HEIGHT + 2, Language::get(3723));
			//	}
			//	else
			//	{
			//		ttfPrintTextFormatted(ttf12, windowX1 + 16, windowY2 - 2 * TTF12_HEIGHT - 8,
			//			Language::get(3720));
			//	}
			//}
			//else if ( scribingFilter == SCRIBING_FILTER_REPAIRABLE )
			//{
			//	ttfPrintTextFormatted(ttf12, windowX1 + 16, windowY2 - 2 * TTF12_HEIGHT - 8,
			//		Language::get(3726));
			//}

			//// draw filter labels.
			//int txtWidth = 0;
			//int txtHeight = 0;
			//int charWidth = 0;
			//TTF_Font* font = ttf8;
			//getSizeOfText(font, "a", &charWidth, nullptr); // get 1 character width.
			//int textstartx = pos.x + 2 * charWidth + 4;

			//SDL_Rect highlightBtn;
			//// Inscribe
			//getSizeOfText(ttf8, Language::get(3718), &txtWidth, &txtHeight);
			//highlightBtn.x = textstartx;
			//highlightBtn.y = pos.y + (12 - txtHeight);
			//highlightBtn.w = txtWidth + 2 * charWidth + 4;
			//highlightBtn.h = txtHeight + 4;
			//if ( (inputs.bMouseLeft(gui_player) || inputs.bControllerInputPressed(gui_player, INJOY_MENU_USE))
			//	&& mouseInBounds(gui_player, highlightBtn.x, highlightBtn.x + highlightBtn.w, highlightBtn.y, highlightBtn.y + highlightBtn.h) )
			//{
			//	scribingFilter = SCRIBING_FILTER_CRAFTABLE;
			//	inputs.controllerClearInput(gui_player, INJOY_MENU_USE);
			//	inputs.mouseClearLeft(gui_player);
			//}
			//if ( scribingFilter == SCRIBING_FILTER_CRAFTABLE )
			//{
			//	drawImageScaled(button_bmp, NULL, &highlightBtn);
			//}
			//ttfPrintText(font, highlightBtn.x + 4 + charWidth, pos.y - (8 - txtHeight), Language::get(3718));

			//// Repair
			//getSizeOfText(font, Language::get(3719), &txtWidth, &txtHeight);
			//highlightBtn.x += highlightBtn.w;
			//highlightBtn.y = pos.y + (12 - txtHeight);
			//highlightBtn.w = txtWidth + 2 * charWidth + 4;
			//highlightBtn.h = txtHeight + 4;
			//if ( (inputs.bMouseLeft(gui_player) || inputs.bControllerInputPressed(gui_player, INJOY_MENU_USE))
			//	&& mouseInBounds(gui_player, highlightBtn.x, highlightBtn.x + highlightBtn.w, highlightBtn.y, highlightBtn.y + highlightBtn.h) )
			//{
			//	scribingFilter = SCRIBING_FILTER_REPAIRABLE;
			//	inputs.controllerClearInput(gui_player, INJOY_MENU_USE);
			//	inputs.mouseClearLeft(gui_player);
			//}
			//if ( scribingFilter == SCRIBING_FILTER_REPAIRABLE )
			//{
			//	drawImageScaled(button_bmp, NULL, &highlightBtn);
			//}
			//ttfPrintText(font, highlightBtn.x + 4 + charWidth, pos.y - (8 - txtHeight), Language::get(3719));
		}

		if ( guiType != GUI_TYPE_TINKERING && guiType != GUI_TYPE_ALCHEMY
			&& guiType != GUI_TYPE_SCRIBING ) // gradually remove all this for all windows once upgraded
		{
			//drawImage(identifyGUI_img, NULL, &pos);

			////Buttons
			//if ( inputs.bMouseLeft(gui_player) )
			//{
			//	//GUI scroll up button.
			//	if ( omousey >= gui_startx + 16 && omousey < gui_startx + 52 )
			//	{
			//		if ( omousex >= gui_starty + (identifyGUI_img->w - 28) && omousex < gui_starty + (identifyGUI_img->w - 12) )
			//		{
			//			buttonclick = 7;
			//			scroll--;
			//			inputs.mouseClearLeft(gui_player);
			//		}
			//	}
			//	//GUI scroll down button.
			//	else if ( omousey >= gui_startx + 52 && omousey < gui_startx + 88 )
			//	{
			//		if ( omousex >= gui_starty + (identifyGUI_img->w - 28) && omousex < gui_starty + (identifyGUI_img->w - 12) )
			//		{
			//			buttonclick = 8;
			//			scroll++;
			//			inputs.mouseClearLeft(gui_player);
			//		}
			//	}
			//	else if ( omousey >= gui_startx && omousey < gui_startx + 15 )
			//	{
			//		//GUI close button.
			//		if ( omousex >= gui_starty + 393 && omousex < gui_starty + 407 )
			//		{
			//			buttonclick = 9;
			//			inputs.mouseClearLeft(gui_player);
			//		}

			//		// 20/12/20 - disabling this for now. unnecessary
			//		if ( false )
			//		{
			//			if ( omousex >= gui_starty && omousex < gui_starty + 377 && omousey >= gui_startx && omousey < gui_startx + 15 )
			//			{
			//				gui_clickdrag[gui_player] = true;
			//				draggingGUI = true;
			//				dragoffset_x[gui_player] = omousex - gui_starty;
			//				dragoffset_y[gui_player] = omousey - gui_startx;
			//				inputs.mouseClearLeft(gui_player);
			//			}
			//		}
			//	}
			//}

			//// mousewheel
			//if ( omousex >= gui_starty + 12 && omousex < gui_starty + (identifyGUI_img->w - 28) )
			//{
			//	if ( omousey >= gui_startx + 16 && omousey < gui_startx + (identifyGUI_img->h - 8) )
			//	{
			//		if ( mousestatus[SDL_BUTTON_WHEELDOWN] )
			//		{
			//			mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
			//			scroll++;
			//		}
			//		else if ( mousestatus[SDL_BUTTON_WHEELUP] )
			//		{
			//			mousestatus[SDL_BUTTON_WHEELUP] = 0;
			//			scroll--;
			//		}
			//	}
			//}

			//if ( draggingGUI )
			//{
			//	if ( gui_clickdrag[gui_player] )
			//	{
			//		offsetx = (omousex - dragoffset_x[gui_player]) - (gui_starty - offsetx);
			//		offsety = (omousey - dragoffset_y[gui_player]) - (gui_startx - offsety);
			//		if ( gui_starty <= 0 )
			//		{
			//			offsetx = 0 - (gui_starty - offsetx);
			//		}
			//		if ( gui_starty > 0 + xres - identifyGUI_img->w )
			//		{
			//			offsetx = (0 + xres - identifyGUI_img->w) - (gui_starty - offsetx);
			//		}
			//		if ( gui_startx <= 0 )
			//		{
			//			offsety = 0 - (gui_startx - offsety);
			//		}
			//		if ( gui_startx > 0 + players[gui_player]->camera_y2() - identifyGUI_img->h )
			//		{
			//			offsety = (0 + players[gui_player]->camera_y2() - identifyGUI_img->h) - (gui_startx - offsety);
			//		}
			//	}
			//	else
			//	{
			//		draggingGUI = false;
			//	}
			//}
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

		//else
		//{
		//	//Print the window label signifying this GUI.
		//	char* window_name;
		//	/*if ( guiType == GUI_TYPE_REPAIR )
		//	{
		//		if ( itemEffectItemType == SCROLL_REPAIR )
		//		{
		//			window_name = Language::get(3286);
		//		}
		//		else if ( itemEffectItemType == SCROLL_CHARGING )
		//		{
		//			window_name = Language::get(3732);
		//		}
		//		ttfPrintText(ttf8, (gui_starty + 2 + ((identifyGUI_img->w / 2) - ((TTF8_WIDTH * longestline(window_name)) / 2))), gui_startx + 4, window_name);
		//	}
		//	else */
		//	if ( guiType == GUI_TYPE_ALCHEMY )
		//	{
		//		/*if ( !basePotion )
		//		{
		//			if ( !experimentingAlchemy )
		//			{
		//				window_name = Language::get(3328);
		//			}
		//			else
		//			{
		//				window_name = Language::get(3344);
		//			}
		//			ttfPrintText(ttf8, (gui_starty + 2 + ((identifyGUI_img->w / 2) - ((TTF8_WIDTH * longestline(window_name)) / 2))), gui_startx + 4, window_name);
		//		}
		//		else
		//		{
		//			if ( !experimentingAlchemy )
		//			{
		//				window_name = Language::get(3329);
		//			}
		//			else
		//			{
		//				window_name = Language::get(3345);
		//			}
		//			ttfPrintText(ttf8, (gui_starty + 2 + ((identifyGUI_img->w / 2) - ((TTF8_WIDTH * longestline(window_name)) / 2))), 
		//				gui_startx + 4 - TTF8_HEIGHT - 4, window_name);
		//			int count = basePotion->count;
		//			basePotion->count = 1;
		//			char *description = basePotion->description();
		//			basePotion->count = count;
		//			ttfPrintText(ttf8, (gui_starty + 2 + ((identifyGUI_img->w / 2) - ((TTF8_WIDTH * longestline(description)) / 2))),
		//				gui_startx + 4, description);
		//		}*/
		//	}
		//	/*else if ( guiType == GUI_TYPE_REMOVECURSE )
		//	{
		//		window_name = Language::get(346);
		//		ttfPrintText(ttf8, (gui_starty + 2 + ((identifyGUI_img->w / 2) - ((TTF8_WIDTH * longestline(window_name)) / 2))), gui_startx + 4, window_name);
		//	}
		//	else if ( guiType == GUI_TYPE_IDENTIFY )
		//	{
		//		window_name = Language::get(318);
		//		ttfPrintText(ttf8, (gui_starty + 2 + ((identifyGUI_img->w / 2) - ((TTF8_WIDTH * longestline(window_name)) / 2))), gui_startx + 4, window_name);
		//	}*/

		//	if ( guiType != GUI_TYPE_TINKERING 
		//		&& guiType != GUI_TYPE_ALCHEMY
		//		&& guiType != GUI_TYPE_SCRIBING )
		//	{
		//		//GUI up button.
		//		if ( buttonclick == 7 )
		//		{
		//			pos.x = gui_starty + (identifyGUI_img->w - 28);
		//			pos.y = gui_startx + 16;
		//			pos.w = 0;
		//			pos.h = 0;
		//			drawImage(invup_bmp, NULL, &pos);
		//		}
		//		//GUI down button.
		//		if ( buttonclick == 8 )
		//		{
		//			pos.x = gui_starty + (identifyGUI_img->w - 28);
		//			pos.y = gui_startx + 52;
		//			pos.w = 0;
		//			pos.h = 0;
		//			drawImage(invdown_bmp, NULL, &pos);
		//		}
		//		//GUI close button.
		//		if ( buttonclick == 9 )
		//		{
		//			pos.x = gui_starty + 393;
		//			pos.y = gui_startx;
		//			pos.w = 0;
		//			pos.h = 0;
		//			drawImage(invclose_bmp, NULL, &pos);
		//			closeGUI();
		//		}

		//		Item *item = nullptr;

		//		bool selectingSlot = false;
		//		SDL_Rect slotPos;
		//		slotPos.x = gui_starty + 12;
		//		slotPos.w = inventoryoptionChest_bmp->w;
		//		slotPos.y = gui_startx + 16;
		//		slotPos.h = inventoryoptionChest_bmp->h;
		//		bool mouseWithinBoundaryX = (mousex >= slotPos.x && mousex < slotPos.x + slotPos.w);

		//		for ( int i = 0; i < kNumShownItems; ++i, slotPos.y += slotPos.h )
		//		{
		//			pos.x = slotPos.x;
		//			pos.w = 0;
		//			pos.h = 0;


		//			if ( mouseWithinBoundaryX && omousey >= slotPos.y && omousey < slotPos.y + slotPos.h && itemsDisplayed[i] )
		//			{
		//				pos.y = slotPos.y;
		//				drawImage(inventoryoptionChest_bmp, nullptr, &pos);
		//				selectedSlot = i;
		//				selectingSlot = true;
		//				if ( (inputs.bMouseLeft(gui_player) || inputs.bControllerInputPressed(gui_player, INJOY_MENU_USE)) )
		//				{
		//					inputs.controllerClearInput(gui_player, INJOY_MENU_USE);
		//					inputs.mouseClearLeft(gui_player);

		//					bool result = executeOnItemClick(itemsDisplayed[i]);
		//					GUICurrentType oldType = guiType;
		//					rebuildGUIInventory();

		//					if ( oldType == GUI_TYPE_ALCHEMY && !guiActive )
		//					{
		//						// do nothing
		//					}
		//					else if ( itemsDisplayed[i] == nullptr )
		//					{
		//						if ( itemsDisplayed[0] == nullptr )
		//						{
		//							//Go back to inventory.
		//							selectedSlot = -1;
		//							players[gui_player]->inventoryUI.warpMouseToSelectedItem(nullptr, (Inputs::SET_CONTROLLER));
		//						}
		//						else
		//						{
		//							//Move up one slot.
		//							--selectedSlot;
		//							warpMouseToSelectedSlot();
		//						}
		//					}
		//				}
		//			}
		//		}

		//		if ( !selectingSlot )
		//		{
		//			selectedSlot = -1;
		//		}
		//	}

		//	//Okay, now prepare to render all the items.
		//	y = gui_startx + 22;
		//	c = 0;
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

		//		//Actually render the items.
		//		c = 0;
		//		for ( node = player_inventory->first; node != NULL; node = node->next )
		//		{
		//			if ( node->element )
		//			{
		//				Item* item = (Item*)node->element;
		//				bool displayItem = shouldDisplayItemInGUI(item);
		//				if ( displayItem )   //Skip over all non-used items
		//				{
		//					c++;
		//					if ( c <= scroll )
		//					{
		//						continue;
		//					}
		//					char tempstr[256] = { 0 };
		//					int showTinkeringBotHealthPercentage = false;
		//					Uint32 color = uint32ColorWhite;
		//					if ( guiType == GUI_TYPE_TINKERING )
		//					{
		//						break;
		//						if ( isNodeTinkeringCraftableItem(item->node) )
		//						{
		//							// if anything, these should be doing
		//							// strncpy(tempstr, Language::get(N), TEMPSTR_LEN - <extra space needed>)
		//							// not strlen(Language::get(N)). there is zero safety conferred from this
		//							// anti-pattern. different story with memcpy(), but strcpy() is not
		//							// memcpy().
		//							strcpy(tempstr, Language::get(3644)); // craft
		//							strncat(tempstr, item->description(), 46 - strlen(Language::get(3644)));
		//							if ( !tinkeringPlayerCanAffordCraft(item) || (tinkeringPlayerHasSkillLVLToCraft(item) == -1) )
		//							{
		//								color = uint32ColorGray;
		//							}
		//						}
		//						else if ( isItemSalvageable(item, gui_player) && tinkeringFilter != TINKER_FILTER_REPAIRABLE )
		//						{
		//							strcpy(tempstr, Language::get(3645)); // salvage
		//							strncat(tempstr, item->description(), 46 - strlen(Language::get(3645)));
		//						}
		//						else if ( tinkeringIsItemRepairable(item, gui_player) )
		//						{
		//							if ( tinkeringIsItemUpgradeable(item) )
		//							{
		//								if ( tinkeringUpgradeMaxStatus(item) <= item->status )
		//								{
		//									color = uint32ColorGray; // can't upgrade since it's higher status than we can craft.
		//								}
		//								else if ( !tinkeringPlayerCanAffordRepair(item) )
		//								{
		//									color = uint32ColorGray; // can't upgrade since no materials
		//								}
		//								strcpy(tempstr, Language::get(3684)); // upgrade
		//								strncat(tempstr, item->description(), 46 - strlen(Language::get(3684)));
		//							}
		//							else
		//							{
		//								if ( tinkeringPlayerHasSkillLVLToCraft(item) == -1 && itemCategory(item) == TOOL )
		//								{
		//									color = uint32ColorGray; // can't repair since no we can't craft it.
		//								}
		//								else if ( !tinkeringPlayerCanAffordRepair(item) )
		//								{
		//									color = uint32ColorGray; // can't repair since no materials
		//								}
		//								strcpy(tempstr, Language::get(3646)); // repair
		//								strncat(tempstr, item->description(), 46 - strlen(Language::get(3646)));
		//							}
		//							if ( item->type == TOOL_SENTRYBOT || item->type == TOOL_DUMMYBOT || item->type == TOOL_SPELLBOT )
		//							{
		//								showTinkeringBotHealthPercentage = true;
		//							}
		//						}
		//						else
		//						{
		//							messagePlayer(clientnum, MESSAGE_DEBUG, "%d", item->type);
		//							strncat(tempstr, "invalid item", 13);
		//						}
		//					}
		//					else if ( guiType == GUI_TYPE_SCRIBING )
		//					{
		//						break;
		//						if ( isNodeScribingCraftableItem(item->node) )
		//						{
		//							snprintf(tempstr, sizeof(tempstr), Language::get(3721), item->getScrollLabel());
		//						}
		//						else
		//						{
		//							if ( scribingFilter == SCRIBING_FILTER_REPAIRABLE )
		//							{
		//								strcpy(tempstr, Language::get(3719)); // repair
		//								strncat(tempstr, item->description(), 46 - strlen(Language::get(3718)));
		//							}
		//							else
		//							{
		//								strcpy(tempstr, Language::get(3718)); // inscribe
		//								int oldcount = item->count;
		//								item->count = 1;
		//								strncat(tempstr, item->description(), 46 - strlen(Language::get(3718)));
		//								item->count = oldcount;
		//							}
		//						}
		//					}
		//					else
		//					{
		//						strncpy(tempstr, item->description(), 46);
		//					}

		//					if ( showTinkeringBotHealthPercentage )
		//					{
		//						int health = 100;
		//						if ( item->appearance >= 0 && item->appearance <= 4 )
		//						{
		//							health = 25 * item->appearance;
		//							if ( health == 0 && item->status != BROKEN )
		//							{
		//								health = 5;
		//							}
		//						}
		//						char healthstr[32] = "";
		//						snprintf(healthstr, 16, " (%d%%)", health);
		//						strncat(tempstr, healthstr, 46 - strlen(tempstr) - strlen(healthstr));
		//					}
		//					else if ( item->type == ENCHANTED_FEATHER && item->identified )
		//					{
		//						char healthstr[32] = "";
		//						snprintf(healthstr, 16, " (%d%%)", item->appearance % ENCHANTED_FEATHER_MAX_DURABILITY);
		//						strncat(tempstr, healthstr, 46 - strlen(tempstr) - strlen(healthstr));
		//					}
		//					

		//					if ( strlen(tempstr) >= 46 )
		//					{
		//						strcat(tempstr, " ...");
		//					}
		//					ttfPrintTextColor(ttf8, gui_starty + 36, y, color, true, tempstr);
		//					pos.x = gui_starty + 16;
		//					pos.y = gui_startx + 17 + 18 * (c - scroll - 1);
		//					pos.w = 16;
		//					pos.h = 16;
		//					drawImageScaled(itemSprite(item), NULL, &pos);
		//					if ( guiType == GUI_TYPE_TINKERING )
		//					{
		//						int metal = 0;
		//						int magic = 0;
		//						if ( isNodeTinkeringCraftableItem(item->node) )
		//						{
		//							tinkeringGetCraftingCost(item, &metal, &magic);
		//						}
		//						else if ( isItemSalvageable(item, gui_player) && tinkeringFilter != TINKER_FILTER_REPAIRABLE )
		//						{
		//							tinkeringGetItemValue(item, &metal, &magic);
		//						}
		//						else if ( tinkeringIsItemRepairable(item, gui_player) )
		//						{
		//							tinkeringGetRepairCost(item, &metal, &magic);
		//						}
		//						pos.x = windowX2 - 20 - TTF8_WIDTH * 12;
		//						if ( !item->identified )
		//						{
		//							ttfPrintTextFormattedColor(ttf8, windowX2 - 24 - TTF8_WIDTH * 15, y, color, "  ?    ?");
		//						}
		//						else
		//						{
		//							ttfPrintTextFormattedColor(ttf8, windowX2 - 24 - TTF8_WIDTH * 15, y, color, "%3d  %3d", metal, magic);
		//						}
		//						node_t* imageNode = items[TOOL_METAL_SCRAP].surfaces.first;
		//						if ( imageNode )
		//						{
		//							drawImageScaled(*((SDL_Surface**)imageNode->element), NULL, &pos);
		//						}
		//						pos.x += TTF12_WIDTH * 4;
		//						imageNode = items[TOOL_MAGIC_SCRAP].surfaces.first;
		//						if ( imageNode )
		//						{
		//							drawImageScaled(*((SDL_Surface**)imageNode->element), NULL, &pos);
		//						}
		//					}
		//					y += 18;
		//					if ( c > 3 + scroll )
		//					{
		//						break;
		//					}
		//				}
		//			}
		//		}
		//	}
		}
	}
}

bool GenericGUIMenu::shouldDisplayItemInGUI(Item* item)
{
	if ( !item )
	{
		return false;
	}
	if ( guiType == GUI_TYPE_ITEMFX )
	{
		if ( itemfxGUI.currentMode == ItemEffectGUI_t::ITEMFX_MODE_SCROLL_IDENTIFY
			|| itemfxGUI.currentMode == ItemEffectGUI_t::ITEMFX_MODE_SPELL_IDENTIFY )
		{
			return isItemIdentifiable(item);
		}
		else if ( itemfxGUI.currentMode == ItemEffectGUI_t::ITEMFX_MODE_SCROLL_ENCHANT_ARMOR )
		{
			return isItemEnchantArmorable(item);
		}
		else if ( itemfxGUI.currentMode == ItemEffectGUI_t::ITEMFX_MODE_SCROLL_ENCHANT_WEAPON )
		{
			return isItemEnchantWeaponable(item);
		}
		else if ( itemfxGUI.currentMode == ItemEffectGUI_t::ITEMFX_MODE_SCROLL_REPAIR
			|| itemfxGUI.currentMode == ItemEffectGUI_t::ITEMFX_MODE_SCROLL_CHARGING )
		{
			return isItemRepairable(item, itemEffectItemType);
		}
		else if ( itemfxGUI.currentMode == ItemEffectGUI_t::ITEMFX_MODE_SCROLL_REMOVECURSE 
			|| itemfxGUI.currentMode == ItemEffectGUI_t::ITEMFX_MODE_SPELL_REMOVECURSE )
		{
			return isItemRemoveCursable(item);
		}
		return false;
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
		messagePlayer(gui_player, MESSAGE_MISC, Language::get(347), item->getName());
		return;
	}

	item->beatitude = 0; //0 = uncursed. > 0 = blessed.
	messagePlayer(gui_player, MESSAGE_MISC, Language::get(348), item->description());

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
		messagePlayer(gui_player, MESSAGE_MISC, Language::get(319), item->getName());
		return;
	}

	if ( gui_player >= 0 && gui_player < MAXPLAYERS && players[gui_player]->isLocalPlayer() )
	{
		if ( !item->identified )
		{
			Compendium_t::Events_t::eventUpdate(gui_player, Compendium_t::CPDM_APPRAISED, item->type, 1);
		}
	}
	item->identified = true;
	messagePlayer(gui_player, MESSAGE_MISC, Language::get(320), item->description());
	closeGUI();
}

void GenericGUIMenu::enchantItem(Item* item)
{
	if ( !item )
	{
		return;
	}
	if ( !shouldDisplayItemInGUI(item) )
	{
		messagePlayer(gui_player, MESSAGE_MISC, Language::get(6307), item->getName());
		return;
	}

	if ( itemEffectItemBeatitude >= 0 )
	{
		if ( gui_player >= 0 && gui_player < MAXPLAYERS && players[gui_player]->isLocalPlayer() )
		{
			Compendium_t::Events_t::eventUpdate(gui_player, Compendium_t::CPDM_BLESSED_TOTAL, item->type, 1);
		}
		item->beatitude += 1 + itemEffectItemBeatitude;
		if ( itemEffectItemBeatitude == 0 )
		{
			messagePlayer(gui_player, MESSAGE_HINT, Language::get(859), item->getName()); // glows blue
		}
		else if ( itemEffectItemBeatitude >= 0 )
		{
			messagePlayer(gui_player, MESSAGE_HINT, Language::get(860), item->getName()); // glows violently blue
		}

		if ( multiplayer == CLIENT )
		{
			Item** slot = itemSlot(stats[gui_player], item);
			int armornum = -1;
			if ( slot )
			{
				if ( slot == &stats[gui_player]->weapon )
				{
					armornum = 0;
				}
				else if ( slot == &stats[gui_player]->helmet )
				{
					armornum = 1;
				}
				else if ( slot == &stats[gui_player]->breastplate )
				{
					armornum = 2;
				}
				else if ( slot == &stats[gui_player]->gloves )
				{
					armornum = 3;
				}
				else if ( slot == &stats[gui_player]->shoes )
				{
					armornum = 4;
				}
				else if ( slot == &stats[gui_player]->shield )
				{
					armornum = 5;
				}
				else if ( slot == &stats[gui_player]->cloak )
				{
					armornum = 6;
				}
				else if ( slot == &stats[gui_player]->mask )
				{
					armornum = 7;
				}
				else if ( slot == &stats[gui_player]->mask )
				{
					armornum = 7;
				}
			}
			if ( armornum >= 0 )
			{
				strcpy((char*)net_packet->data, "BEAT");
				net_packet->data[4] = gui_player;
				net_packet->data[5] = armornum;
				net_packet->data[6] = item->beatitude + 100;
				SDLNet_Write16((Sint16)item->type, &net_packet->data[7]);
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 9;
				sendPacketSafe(net_sock, -1, net_packet, 0);
				//messagePlayer(player, "sent server: %d, %d, %d", net_packet->data[4], net_packet->data[5], net_packet->data[6]);
			}
		}
	}
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
		messagePlayer(gui_player, MESSAGE_MISC, Language::get(3287), item->getName());
		return;
	}

	bool isEquipped = itemIsEquipped(item, gui_player);

	if ( itemEffectItemType == SCROLL_CHARGING )
	{
		if ( itemCategory(item) == MAGICSTAFF )
		{
			Compendium_t::Events_t::eventUpdate(gui_player, Compendium_t::CPDM_MAGICSTAFF_RECHARGED, item->type, 1);
			if ( item->status == BROKEN )
			{
				if ( itemEffectItemBeatitude > 0 )
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
				if ( itemEffectItemBeatitude == 0 )
				{
					repairAmount = ENCHANTED_FEATHER_MAX_DURABILITY / 2;
				}
			}
			item->appearance += repairAmount;
			item->status = EXCELLENT;
		}
		messagePlayer(gui_player, MESSAGE_MISC, Language::get(3730), item->getName());
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
				item->status = static_cast<Status>(std::min(item->status + 2 + itemEffectItemBeatitude, static_cast<int>(EXCELLENT)));
			}
		}
		Compendium_t::Events_t::eventUpdate(gui_player, Compendium_t::CPDM_REPAIRS, item->type, 1);
		messagePlayer(gui_player, MESSAGE_MISC, Language::get(872), item->getName());
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
	guiType = GUI_TYPE_NONE;
	basePotion = nullptr;
	secondaryPotion = nullptr;
	alembicItem = nullptr;
	itemEffectUsingSpell = false;
	itemEffectUsingSpellbook = false;
	itemEffectScrollItem = nullptr;
	itemEffectItemType = 0;
	itemEffectItemBeatitude = 0;
	itemfxGUI.currentMode = ItemEffectGUI_t::ITEMFX_MODE_NONE;
	if ( tinkerGUI.bOpen )
	{
		tinkerGUI.closeTinkerMenu();
	}
	if ( alchemyGUI.bOpen )
	{
		alchemyGUI.closeAlchemyMenu();
	}
	if ( featherGUI.bOpen )
	{
		featherGUI.closeFeatherMenu();
	}
	if ( itemfxGUI.bOpen )
	{
		itemfxGUI.closeItemEffectMenu();
	}
	if ( wasOpen )
	{
		players[gui_player]->inventoryUI.tooltipDelayTick = ticks + TICKS_PER_SECOND / 10;
	}
}

//inline Item* GenericGUIMenu::getItemInfo(int slot)
//{
//	if ( slot >= kNumShownItems )
//	{
//		return nullptr; //Out of bounds,
//	}
//
//	return itemsDisplayed[slot];
//}

//void GenericGUIMenu::selectSlot(int slot)
//{
//	if ( slot < selectedSlot )
//	{
//		//Moving up.
//
//		/*
//		* Possible cases:
//		* * 1) Move cursor up the GUI through different selectedSlot.
//		* * 2) Page up through scroll--
//		* * 3) Scrolling up past top of GUI, no scroll (move back to inventory)
//		*/
//
//		if ( selectedSlot <= 0 )
//		{
//			//Covers cases 2 & 3.
//
//			/*
//			* Possible cases:
//			* * A) Hit very top of "inventory", can't go any further. Return to inventory.
//			* * B) Page up, scrolling through scroll.
//			*/
//
//			if ( scroll <= 0 )
//			{
//				//Case 3/A: Return to inventory.
//				//selectedSlot = -1;
//			}
//			else
//			{
//				//Case 2/B: Page up through "inventory".
//				--scroll;
//			}
//		}
//		else
//		{
//			//Covers case 1.
//
//			//Move cursor up the GUI through different selectedSlot (--selectedSlot).
//			--selectedSlot;
//			warpMouseToSelectedSlot();
//		}
//	}
//	else if ( slot > selectedSlot )
//	{
//		//Moving down.
//
//		/*
//		* Possible cases:
//		* * 1) Moving cursor down through GUI through different selectedSlot.
//		* * 2) Scrolling down past bottom of GUI through scroll++
//		* * 3) Scrolling down past bottom of GUI, max scroll (revoke move -- can't go beyond limit of GUI).
//		*/
//
//		if ( selectedSlot >= kNumShownItems - 1 )
//		{
//			//Covers cases 2 & 3.
//			++scroll; //scroll is automatically sanitized in updateGUI().
//		}
//		else
//		{
//			//Covers case 1.
//			//Move cursor down through the GUI through different selectedSlot (++selectedSlot).
//			//This is a little bit trickier since must revoke movement if there is no item in the next slot!
//
//			/*
//			* Two possible cases:
//			* * A) Items below this. Advance selectedSlot to them.
//			* * B) On last item already. Do nothing (revoke movement).
//			*/
//
//			Item* item = getItemInfo(selectedSlot + 1);
//
//			if ( item )
//			{
//				++selectedSlot;
//				warpMouseToSelectedSlot();
//			}
//			else
//			{
//				//No more items. Stop.
//			}
//		}
//	}
//}

//void GenericGUIMenu::warpMouseToSelectedSlot()
//{
//	SDL_Rect slotPos;
//	slotPos.x = gui_starty;
//	slotPos.w = inventoryoptionChest_bmp->w;
//	slotPos.h = inventoryoptionChest_bmp->h;
//	slotPos.y = gui_startx + 16 + (slotPos.h * selectedSlot);
//
//	// to verify for splitscreen
//	//SDL_WarpMouseInWindow(screen, slotPos.x + (slotPos.w / 2), slotPos.y + (slotPos.h / 2));
//}

void GenericGUIMenu::openGUI(int type, Item* effectItem, int effectBeatitude, int effectItemType, int usingSpellID)
{
	this->closeGUI();

	if ( !effectItem && usingSpellID != SPELL_NONE && effectItemType == SPELL_ITEM )
	{
		for ( node_t* node = stats[gui_player]->inventory.first; node; node = node->next )
		{
			Item* item = static_cast<Item*>(node->element);
			if ( !item )
			{
				continue;
			}
			//Search player's inventory for the special spell item.
			if ( itemCategory(item) != SPELL_CAT )
			{
				continue;
			}
			spell_t* spell = getSpellFromItem(gui_player, item, false);
			if ( spell && spell->ID == usingSpellID )
			{
				effectItem = item;
				break;
			}
		}
		if ( !effectItem )
		{
			return;
		}
	}

	if ( players[gui_player] && players[gui_player]->entity )
	{
		if ( players[gui_player]->entity->isBlind() )
		{
			messagePlayer(gui_player, MESSAGE_MISC, Language::get(892));
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
	itemEffectItemBeatitude = effectBeatitude;
	itemEffectItemType = effectItemType;
	guiType = static_cast<GUICurrentType>(type);
	if ( type == GUICurrentType::GUI_TYPE_ITEMFX )
	{
		if ( items[effectItemType].category == SPELLBOOK )
		{
			itemEffectUsingSpellbook = true;
		}
		else if ( itemEffectItemType == SPELL_ITEM )
		{
			itemEffectUsingSpell = true;
		}
		itemEffectScrollItem = effectItem;
		if ( itemEffectItemType == SCROLL_IDENTIFY )
		{
			itemfxGUI.currentMode = ItemEffectGUI_t::ITEMFX_MODE_SCROLL_IDENTIFY;
		}
		else if ( itemEffectItemType == SCROLL_ENCHANTWEAPON )
		{
			itemfxGUI.currentMode = ItemEffectGUI_t::ITEMFX_MODE_SCROLL_ENCHANT_WEAPON;
		}
		else if ( itemEffectItemType == SCROLL_ENCHANTARMOR )
		{
			itemfxGUI.currentMode = ItemEffectGUI_t::ITEMFX_MODE_SCROLL_ENCHANT_ARMOR;
		}
		else if ( usingSpellID == SPELL_IDENTIFY )
		{
			itemfxGUI.currentMode = ItemEffectGUI_t::ITEMFX_MODE_SPELL_IDENTIFY;
		}
		else if ( itemEffectItemType == SCROLL_CHARGING )
		{
			itemfxGUI.currentMode = ItemEffectGUI_t::ITEMFX_MODE_SCROLL_CHARGING;
		}
		else if ( itemEffectItemType == SCROLL_REMOVECURSE )
		{
			itemfxGUI.currentMode = ItemEffectGUI_t::ITEMFX_MODE_SCROLL_REMOVECURSE;
		}
		else if ( usingSpellID == SPELL_REMOVECURSE )
		{
			itemfxGUI.currentMode = ItemEffectGUI_t::ITEMFX_MODE_SPELL_REMOVECURSE;
		}
		else if ( itemEffectItemType == SCROLL_REPAIR )
		{
			itemfxGUI.currentMode = ItemEffectGUI_t::ITEMFX_MODE_SCROLL_REPAIR;
		}
		if ( itemfxGUI.currentMode == ItemEffectGUI_t::ITEMFX_MODE_NONE )
		{
			this->closeGUI();
			return;
		}
		itemfxGUI.openItemEffectMenu(itemfxGUI.currentMode);
	}

	FollowerMenu[gui_player].closeFollowerMenuGUI();
	CalloutMenu[gui_player].closeCalloutMenuGUI();

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
			messagePlayer(gui_player, MESSAGE_MISC, Language::get(892));
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

	FollowerMenu[gui_player].closeFollowerMenuGUI();
	CalloutMenu[gui_player].closeCalloutMenuGUI();

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
			messagePlayer(gui_player, MESSAGE_MISC, Language::get(892));
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
		featherGUI.openFeatherMenu();
	}

	FollowerMenu[gui_player].closeFollowerMenuGUI();
	CalloutMenu[gui_player].closeCalloutMenuGUI();

	if ( openedChest[gui_player] )
	{
		openedChest[gui_player]->closeChest();
	}
	rebuildGUIInventory();
}

bool hideRecipeFromList(int type);
void onFeatherChangeTabAction(const int playernum, bool changingToNewTab = true);

bool GenericGUIMenu::executeOnItemClick(Item* item)
{
	if ( !item )
	{
		return false;
	}

	if ( guiType == GUI_TYPE_ITEMFX )
	{
		if ( itemfxGUI.currentMode == ItemEffectGUI_t::ITEMFX_MODE_SCROLL_REPAIR
			|| itemfxGUI.currentMode == ItemEffectGUI_t::ITEMFX_MODE_SCROLL_CHARGING )
		{
			if ( itemEffectScrollItem && itemCategory(itemEffectScrollItem) == SCROLL )
			{
				messagePlayer(gui_player, MESSAGE_INVENTORY, Language::get(848)); // as you read the scroll it disappears...
				consumeItem(itemEffectScrollItem, gui_player);
			}
			repairItem(item);
			return true;
		}
		else if ( itemfxGUI.currentMode == ItemEffectGUI_t::ITEMFX_MODE_SCROLL_REMOVECURSE
			|| itemfxGUI.currentMode == ItemEffectGUI_t::ITEMFX_MODE_SPELL_REMOVECURSE )
		{
			if ( itemEffectScrollItem && itemCategory(itemEffectScrollItem) == SCROLL )
			{
				messagePlayer(gui_player, MESSAGE_INVENTORY, Language::get(848)); // as you read the scroll it disappears...
				consumeItem(itemEffectScrollItem, gui_player);
			}
			uncurseItem(item);
			return true;
		}
		else if ( itemfxGUI.currentMode == ItemEffectGUI_t::ITEMFX_MODE_SCROLL_IDENTIFY
			|| itemfxGUI.currentMode == ItemEffectGUI_t::ITEMFX_MODE_SPELL_IDENTIFY )
		{
			if ( itemEffectScrollItem && itemCategory(itemEffectScrollItem) == SCROLL )
			{
				messagePlayer(gui_player, MESSAGE_INVENTORY, Language::get(848)); // as you read the scroll it disappears...
				consumeItem(itemEffectScrollItem, gui_player);
			}
			identifyItem(item);
			return true;
		}
		else if ( itemfxGUI.currentMode == ItemEffectGUI_t::ITEMFX_MODE_SCROLL_ENCHANT_WEAPON
			|| itemfxGUI.currentMode == ItemEffectGUI_t::ITEMFX_MODE_SCROLL_ENCHANT_ARMOR )
		{
			if ( itemEffectScrollItem && itemCategory(itemEffectScrollItem) == SCROLL )
			{
				messagePlayer(gui_player, MESSAGE_INVENTORY, Language::get(848)); // as you read the scroll it disappears...
				consumeItem(itemEffectScrollItem, gui_player);
			}
			enchantItem(item);
			return true;
		}
		return false;
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
					messagePlayer(gui_player, MESSAGE_MISC, Language::get(3337));
				}
				else
				{
					messagePlayer(gui_player, MESSAGE_MISC, Language::get(3342));
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
					if ( tinkeringRepairItem(item) )
					{
						Compendium_t::Events_t::eventUpdate(gui_player, Compendium_t::CPDM_TINKERKIT_REPAIRS, TOOL_TINKERING_KIT, 1);
					}
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
				if ( !featherGUI.bDrawerOpen )
				{
					Player::soundModuleNavigation();
				}
				featherGUI.bDrawerOpen = true;
				featherGUI.scrollListRequiresSorting = true;
				onFeatherChangeTabAction(gui_player, false);
				featherGUI.animPrompt = 1.0;
				featherGUI.animPromptTicks = ticks;
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
	//		messagePlayer(gui_player, MESSAGE_MISC, Language::get(892));
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
		outTryDuplicatePotion = false;
		outSamePotion = false;
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
			std::string itemName = items[result].getIdentifiedName();
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
				GenericGUIMenu::AlchemyGUI_t::AlchNotification_t(Language::get(4179), itemName, iconPath)));
			messagePlayerColor(player, MESSAGE_PROGRESSION, makeColorRGB(0, 255, 0), Language::get(4182), items[result].getIdentifiedName());
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
		skillLVL = stats[gui_player]->getModifiedProficiency(PRO_ALCHEMY) / 20; // 0 to 5;
	}

	Status status = EXCELLENT;
	bool duplicateSucceed = false;
	if ( tryDuplicatePotion && !explodeSelf && !randomResult )
	{
		// do duplicate.
		if ( local_rng.rand() % 100 < (50 + skillLVL * 10) ) // 50 - 100% chance
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
		std::vector<unsigned int> potionChances =
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
		auto generatedPotion = potionStandardAppearanceMap.at(
            local_rng.discrete(potionChances.data(), potionChances.size()));
		result = static_cast<ItemType>(generatedPotion.first);
	}

	if ( basePotion->identified && secondaryPotion->identified )
	{
		messagePlayerColor(gui_player, MESSAGE_INVENTORY, uint32ColorWhite, Language::get(3332),
			items[basePotion->type].getIdentifiedName(), items[secondaryPotion->type].getIdentifiedName());
	}
	else if ( basePotion->identified )
	{
		messagePlayerColor(gui_player, MESSAGE_INVENTORY, uint32ColorWhite, Language::get(3334),
			items[basePotion->type].getIdentifiedName());
	}
	else if ( secondaryPotion->identified )
	{
		messagePlayerColor(gui_player, MESSAGE_INVENTORY, uint32ColorWhite, Language::get(3333),
			items[secondaryPotion->type].getIdentifiedName());
	}
	else
	{
		messagePlayerColor(gui_player, MESSAGE_INVENTORY, uint32ColorWhite, Language::get(3335));
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
		if ( (basePotion->type == POTION_ACID || secondaryPotion->type == POTION_ACID) && !samePotion )
		{
			if ( local_rng.rand() % 10 == 0 )
			{
				degradeAlembic = true;
			}
		}
		else
		{
			if ( local_rng.rand() % 40 == 0 )
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

		if ( !samePotion )
		{
			if ( local_rng.rand() % 100 < (50 + skillLVL * 5) ) // 50 - 75% chance
			{
				emptyBottle = true;
			}
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
		}
	}

	if ( skillCapstoneUnlocked(gui_player, PRO_ALCHEMY) )
	{
		degradeAlembic = false;
	}

	if ( degradeAlembic && alembicItem )
	{
		alembicItem->status = static_cast<Status>(alembicItem->status - 1);
		if ( alembicItem->status > BROKEN )
		{
			messagePlayer(gui_player, MESSAGE_MISC, Language::get(681), alembicItem->getName());
		}
		else
		{
			messagePlayer(gui_player, MESSAGE_MISC, Language::get(2351), alembicItem->getName());
			playSoundPlayer(gui_player, 162, 64);
			consumeItem(alembicItem, gui_player);
			alembicItem = nullptr;
		}
	}

	if ( explodeSelf && players[gui_player] && players[gui_player]->entity )
	{
		Compendium_t::Events_t::eventUpdate(gui_player, Compendium_t::CPDM_ALEMBIC_EXPLOSIONS, TOOL_ALEMBIC, 1);

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
			bool protection = false;
			if ( stats[gui_player]->mask && stats[gui_player]->mask->type == MASK_HAZARD_GOGGLES )
			{
				bool shapeshifted = false;
				if ( stats[gui_player]->type != HUMAN )
				{
					if ( players[gui_player]->entity->effectShapeshift != NOTHING )
					{
						shapeshifted = true;
					}
				}
				if ( !shapeshifted )
				{
					protection = true;
					messagePlayerColor(gui_player, MESSAGE_STATUS, makeColorRGB(0, 255, 0), Language::get(6089));
				}
			}
			spawnMagicTower(protection ? players[gui_player]->entity : nullptr, 
				players[gui_player]->entity->x, players[gui_player]->entity->y, SPELL_FIREBALL, nullptr);
			players[gui_player]->entity->setObituary(Language::get(3350));
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
				if ( !samePotion )
				{
					appearance = 0 + local_rng.rand() % items[POTION_SICKNESS].variations;
				}
				if ( local_rng.rand() % 10 > 0 )
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
				if ( local_rng.rand() % 10 > 0 )
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
					messagePlayer(gui_player, MESSAGE_MISC, Language::get(3356));
					Compendium_t::Events_t::eventUpdate(gui_player, Compendium_t::CPDM_ALEMBIC_DUPLICATION_FAIL, TOOL_ALEMBIC, 1);
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
					messagePlayer(gui_player, MESSAGE_MISC, Language::get(3352), newPotion->description());
					if ( duplicateSucceed )
					{
						Compendium_t::Events_t::eventUpdate(gui_player, Compendium_t::CPDM_ALEMBIC_DUPLICATED, TOOL_ALEMBIC, 1);
					}
				}
			}
			else
			{
				messagePlayer(gui_player, MESSAGE_MISC, Language::get(3352), newPotion->description());
				steamStatisticUpdate(STEAM_STAT_IN_THE_MIX, STEAM_STAT_INT, 1);
				if ( samePotion )
				{
					Compendium_t::Events_t::eventUpdate(gui_player, Compendium_t::CPDM_ALEMBIC_DECANTED, TOOL_ALEMBIC, 1);
				}
				else
				{
					Compendium_t::Events_t::eventUpdate(gui_player, Compendium_t::CPDM_ALEMBIC_BREWED, TOOL_ALEMBIC, 1);

					if ( result != POTION_SICKNESS && result != POTION_WATER )
					{
						achievementObserver.updatePlayerAchievement(gui_player, // clientnum intentional for to include splitscreen
							AchievementObserver::BARONY_ACH_BY_THE_BOOK,
							AchievementObserver::BY_THE_BOOK_BREW);
					}
				}
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
				messagePlayer(gui_player, MESSAGE_MISC, Language::get(3351), items[POTION_EMPTY].getIdentifiedName());
				Compendium_t::Events_t::eventUpdate(gui_player, Compendium_t::CPDM_BOTTLE_FROM_BREWING, POTION_EMPTY, 1);
				free(emptyBottle);
			}
			if ( raiseSkill && local_rng.rand() % 2 == 0 )
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
						messagePlayerColor(gui_player, MESSAGE_PROGRESSION, color, Language::get(3346), items[type].getIdentifiedName());
					}
					else if ( isItemSecondaryIngredient(type) )
					{
						messagePlayerColor(gui_player, MESSAGE_PROGRESSION, color, Language::get(3349), items[type].getIdentifiedName());
					}
					std::string itemName = items[type].getIdentifiedName();
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
						AlchemyGUI_t::AlchNotification_t(Language::get(4180), itemName, iconPath)));
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
				if ( increaseskill && local_rng.rand() % 6 == 0 )
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
		ItemType potion = itemLevelCurve(POTION, 0, currentlevel, local_rng);
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
	items.push_back(newItem(TOOL_DECOY, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 0;
	items[items.size() - 1]->y = 0;
	items.push_back(newItem(TOOL_LOCKPICK, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 1;
	items[items.size() - 1]->y = 0;
	items.push_back(newItem(TOOL_GLASSES, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 2;
	items[items.size() - 1]->y = 0;
	items.push_back(newItem(POTION_EMPTY, SERVICABLE, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 3;
	items[items.size() - 1]->y = 0;
	items.push_back(newItem(TOOL_LANTERN, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 4;
	items[items.size() - 1]->y = 0;

	items.push_back(newItem(TOOL_FREEZE_BOMB, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 0;
	items[items.size() - 1]->y = 1;
	items.push_back(newItem(TOOL_SLEEP_BOMB, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 1;
	items[items.size() - 1]->y = 1;
	items.push_back(newItem(TOOL_DUMMYBOT, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 2;
	items[items.size() - 1]->y = 1;
	items.push_back(newItem(TOOL_GYROBOT, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 3;
	items[items.size() - 1]->y = 1;
	items.push_back(newItem(TOOL_BEARTRAP, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 4;
	items[items.size() - 1]->y = 1;

	items.push_back(newItem(TOOL_BOMB, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 0;
	items[items.size() - 1]->y = 2;
	items.push_back(newItem(TOOL_TELEPORT_BOMB, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 1;
	items[items.size() - 1]->y = 2;
	items.push_back(newItem(TOOL_SENTRYBOT, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 2;
	items[items.size() - 1]->y = 2;
	items.push_back(newItem(TOOL_SPELLBOT, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 3;
	items[items.size() - 1]->y = 2;
	items.push_back(newItem(MASK_TECH_GOGGLES, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 4;
	items[items.size() - 1]->y = 2;
	items.push_back(newItem(CLOAK_BACKPACK, EXCELLENT, 0, 1, ITEM_TINKERING_APPEARANCE, true, &tinkeringTotalItems));
	items[items.size() - 1]->x = 0;
	items[items.size() - 1]->y = 3;
	for ( auto it = items.begin(); it != items.end(); ++it )
	{
		Item* item = *it;
		if ( item )
		{
			int skillLVL = 0;
			int requiredSkill = tinkeringPlayerHasSkillLVLToCraft(item);
			if ( stats[gui_player] && players[gui_player] )
			{
				skillLVL = (stats[gui_player]->getModifiedProficiency(PRO_LOCKPICKING) + statGetPER(stats[gui_player], players[gui_player]->entity)) / 20; // 0 to 5
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
		messagePlayer(gui_player, MESSAGE_MISC, Language::get(3652), items[item->type].getIdentifiedName());
		return false;
	}
	if ( !tinkeringPlayerCanAffordCraft(item) )
	{
		//playSound(90, 64);
		messagePlayer(gui_player, MESSAGE_MISC, Language::get(3648), items[item->type].getIdentifiedName());
		return false;
	}

	Item* crafted = tinkeringCraftItemAndConsumeMaterials(item);
	if ( crafted )
	{
		Item* pickedUp = itemPickup(gui_player, crafted);
		messagePlayer(gui_player, MESSAGE_MISC, Language::get(3668), crafted->description());
		Compendium_t::Events_t::eventUpdate(gui_player, Compendium_t::CPDM_TINKERKIT_CRAFTS, TOOL_TINKERING_KIT, 1);
		Compendium_t::Events_t::eventUpdate(gui_player, Compendium_t::CPDM_GADGET_CRAFTED, crafted->type, 1);
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
		messagePlayer(player, MESSAGE_MISC, Language::get(3669));
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
   		skillLVL = (stats[player]->getModifiedProficiency(PRO_LOCKPICKING) + statGetPER(stats[player], players[player]->entity)) / 20;
		skillLVL = std::min(skillLVL, 5);
		switch ( skillLVL )
		{
			case 5:
				bonusMetalScrap = (1 + ((local_rng.rand() % 2 == 0) ? 1 : 0)) * metal; // 2x or 50% 3x extra scrap
				bonusMagicScrap = (1 + ((local_rng.rand() % 2 == 0) ? 1 : 0)) * magic; // 2x or 50% 3x extra scrap
				break;
			case 4:
				bonusMetalScrap = (1 + ((local_rng.rand() % 4 == 0) ? 1 : 0)) * metal; // 2x or 25% 3x extra scrap
				bonusMagicScrap = (1 + ((local_rng.rand() % 4 == 0) ? 1 : 0)) * magic; // 2x or 25% 3x extra scrap
				break;
			case 3:
				bonusMetalScrap = ((local_rng.rand() % 2 == 0) ? 1 : 0) * metal; // 50% 2x scrap value
				bonusMagicScrap = ((local_rng.rand() % 2 == 0) ? 1 : 0) * magic; // 50% 2x scrap value
				break;
			case 2:
				bonusMetalScrap = ((local_rng.rand() % 4 == 0) ? 1 : 0) * metal; // 25% 2x scrap value
				bonusMagicScrap = ((local_rng.rand() % 4 == 0) ? 1 : 0) * magic; // 25% 2x scrap value
				break;
			case 1:
				bonusMetalScrap = ((local_rng.rand() % 8 == 0) ? 1 : 0) * metal; // 12.5% 2x scrap value
				bonusMagicScrap = ((local_rng.rand() % 8 == 0) ? 1 : 0) * magic; // 12.5% 2x scrap value
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
	if ( outsideInventory )
	{
		if ( item->count > 1 )
		{
			metal *= item->count;
			magic *= item->count;
		}
		tinkeringBulkSalvage = false;
		tinkeringBulkSalvageMetalScrap = 0;
		tinkeringBulkSalvageMagicScrap = 0;
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
					messagePlayerColor(player, MESSAGE_INVENTORY, color, Language::get(3665), metal + tinkeringBulkSalvageMetalScrap, items[pickedUp->type].getIdentifiedName());
					if ( players[player]->isLocalPlayer() )
					{
						Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_TINKERKIT_METAL_SCRAPPED, TOOL_TINKERING_KIT, metal + tinkeringBulkSalvageMetalScrap);
						if ( item && item->type == TOOL_DETONATOR_CHARGE )
						{
							Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_DETONATOR_SCRAPPED_METAL, TOOL_DETONATOR_CHARGE, metal + tinkeringBulkSalvageMetalScrap);
						}
					}
				}
				else
				{
					messagePlayer(player, MESSAGE_MISC, Language::get(3665), metal + tinkeringBulkSalvageMetalScrap, items[pickedUp->type].getIdentifiedName());
					if ( players[player]->isLocalPlayer() )
					{
						Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_TINKERKIT_METAL_SCRAPPED, TOOL_TINKERING_KIT, metal + tinkeringBulkSalvageMetalScrap);
						if ( item && item->type == TOOL_DETONATOR_CHARGE )
						{
							Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_DETONATOR_SCRAPPED_METAL, TOOL_DETONATOR_CHARGE, metal + tinkeringBulkSalvageMetalScrap);
						}
					}
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
					messagePlayerColor(player, MESSAGE_INVENTORY, color, Language::get(3665), magic + tinkeringBulkSalvageMagicScrap, items[pickedUp->type].getIdentifiedName());
					if ( players[player]->isLocalPlayer() )
					{
						Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_TINKERKIT_MAGIC_SCRAPPED, TOOL_TINKERING_KIT, magic + tinkeringBulkSalvageMagicScrap);
						if ( item && item->type == TOOL_DETONATOR_CHARGE )
						{
							Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_DETONATOR_SCRAPPED_MAGIC, TOOL_DETONATOR_CHARGE, magic + tinkeringBulkSalvageMagicScrap);
						}
					}
				}
				else
				{
					messagePlayer(player, MESSAGE_MISC, Language::get(3665), magic + tinkeringBulkSalvageMagicScrap, items[pickedUp->type].getIdentifiedName());
					if ( players[player]->isLocalPlayer() )
					{
						Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_TINKERKIT_MAGIC_SCRAPPED, TOOL_TINKERING_KIT, magic + tinkeringBulkSalvageMagicScrap);
						if ( item && item->type == TOOL_DETONATOR_CHARGE )
						{
							Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_DETONATOR_SCRAPPED_MAGIC, TOOL_DETONATOR_CHARGE, magic + tinkeringBulkSalvageMagicScrap);
						}
					}
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
			if ( local_rng.rand() % 2 == 0 ) // 50%
			{
				if ( stats[player]->getProficiency(PRO_LOCKPICKING) < SKILL_LEVEL_EXPERT )
				{
					increaseSkill = true;
				}
				else if ( local_rng.rand() % 20 == 0 && !tinkeringBulkSalvage )
				{
					messagePlayer(player, MESSAGE_MISC, Language::get(3666)); // nothing left to learn from salvaging.
				}
			}
		}
		else if ( metal >= 2 || magic >= 2 )
		{
			if ( local_rng.rand() % 5 == 0 ) // 20%
			{
				if ( stats[player]->getProficiency(PRO_LOCKPICKING) < SKILL_LEVEL_EXPERT )
				{
					increaseSkill = true;
				}
				else if ( local_rng.rand() % 20 == 0 && !tinkeringBulkSalvage )
				{
					messagePlayer(player, MESSAGE_MISC, Language::get(3666)); // nothing left to learn from salvaging.
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
			if ( (ticks - tinkeringSfxLastTicks) > 200 && ((metal >= 4 || magic >= 4) || local_rng.rand() % 5 == 0) )
			{
				tinkeringSfxLastTicks = ticks;
				playSoundEntity(players[player]->entity, 421 + (local_rng.rand() % 2) * 3, 64);
			}
			else
			{
				if ( local_rng.rand() % 4 == 0 )
				{
					playSoundEntity(players[player]->entity, 35 + local_rng.rand() % 3, 64);
				}
				else
				{
					playSoundEntity(players[player]->entity, 462 + local_rng.rand() % 2, 64);
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

	if ( players[player]->isLocalPlayer() && didCraft )
	{
		Compendium_t::Events_t::eventUpdate(gui_player, Compendium_t::CPDM_TINKERKIT_SALVAGED, TOOL_TINKERING_KIT, 1);
		if ( item && item->type == TOOL_DETONATOR_CHARGE )
		{
			Compendium_t::Events_t::eventUpdate(gui_player, Compendium_t::CPDM_DETONATOR_SCRAPPED, TOOL_DETONATOR_CHARGE, 1);
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
				if ( local_rng.rand() % 10 == 0 )
				{
					increaseSkill = true;
				}
			}
			else
			{
				if ( metal > 2 || magic > 2 )
				{
					if ( local_rng.rand() % 20 == 0 )
					{
						increaseSkill = true;
					}
				}
				else
				{
					if ( stats[gui_player]->getProficiency(PRO_LOCKPICKING) < SKILL_LEVEL_BASIC )
					{
						if ( local_rng.rand() % 10 == 0 )
						{
							increaseSkill = true;
						}
					}
					else if ( local_rng.rand() % 20 == 0 )
					{
						messagePlayer(gui_player, MESSAGE_MISC, Language::get(3667), items[item->type].getIdentifiedName());
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
				playSoundEntity(players[gui_player]->entity, 459 + (local_rng.rand() % 3), 92);
			}
			else
			{
				if ( local_rng.rand() % 3 == 0 )
				{
					playSoundEntity(players[gui_player]->entity, 422 + (local_rng.rand() % 2), 92);
				}
				else
				{
					playSoundEntity(players[gui_player]->entity, 35 + local_rng.rand() % 3, 64);
				}
			}
		}
		else
		{
			playSoundEntity(players[gui_player]->entity, 35 + local_rng.rand() % 3, 64);
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
		case MASK_TECH_GOGGLES:
			*metal = 10;
			*magic = 40;
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
		case MASK_EYEPATCH:
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
		case HAT_SILKEN_BOW:
		case HAT_BANDANA:
		case HAT_CHEF:
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
		case MASK_PIPE:
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
		case HAT_CIRCLET:
		case HAT_LAURELS:
		case HAT_HOOD_APPRENTICE:
		case HAT_HOOD_WHISPERS:
		case HAT_HOOD_ASSASSIN:
			*metal = 1;
			*magic = 4;
			break;

		case MASK_MOUTH_ROSE:
		case MASK_GRASS_SPRIG:
			*metal = 0;
			*magic = 1;
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

		case HAT_CIRCLET_WISDOM:
			*metal = 1;
			*magic = 12;
			break;
		case TOOL_BLINDFOLD_TELEPATHY:
		case HAT_MITER:
		case HAT_HEADDRESS:
			*metal = 1;
			*magic = 8;
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
		case MONOCLE:
		case MASK_BANDIT:
			*metal = 2;
			*magic = 0;
			break;

		case MASK_MOUTHKNIFE:
		case HAT_PLUMED_CAP:
		case HAT_BYCOCKET:
		case MASK_STEEL_VISOR:
			*metal = 2;
			*magic = 1;
			break;

		case BRACERS_CONSTITUTION:
		case TOOL_ALEMBIC:
		case PUNISHER_HOOD:
		case MASK_MASQUERADE:
		case MASK_PLAGUE:
		case HAT_BOUNTYHUNTER:
			*metal = 2;
			*magic = 2;
			break;

		case IRON_BOOTS_WATERWALKING:
		case MASK_SPOOKY:
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
		case MASK_HAZARD_GOGGLES:
			*metal = 3;
			*magic = 0;
			break;

		case HAT_WARM:
		case HAT_WOLF_HOOD:
		case HAT_BEAR_HOOD:
		case HAT_STAG_HOOD:
		case HAT_BUNNY_HOOD:
		case HAT_TOPHAT:
			*metal = 3;
			*magic = 1;
			break;

		case GAUNTLETS_STRENGTH:
		case HELM_MINING:
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
		case HAT_CROWN:
		case HAT_CROWNED_HELM:
		case MASK_CRYSTAL_VISOR:
			*metal = 4;
			*magic = 4;
			break;

		case MASK_PHANTOM:
		case HAT_TURBAN:
			*metal = 2;
			*magic = 6;
			break;

		case MASK_TECH_GOGGLES:
			*metal = 6;
			*magic = 6;
			break;

		case MASK_GOLDEN:
			*metal = 4;
			*magic = 8;
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
		case MASK_ARTIFACT_VISOR:
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

	metal = std::min(99, metal);
	magic = std::min(99, magic);
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
					&& ((stats[gui_player]->getModifiedProficiency(PRO_LOCKPICKING) + statGetPER(stats[gui_player], players[gui_player]->entity)) >= requirement) )
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

const int TINKER_MIN_ITEM_SKILL_REQ = 10;

int GenericGUIMenu::tinkeringPlayerHasSkillLVLToCraft(const Item* item)
{
	if ( !item || !stats[gui_player] )
	{
		return -1;
	}
	int skillLVL = 0;
	if ( stats[gui_player] && players[gui_player] )
	{
		skillLVL = (stats[gui_player]->getModifiedProficiency(PRO_LOCKPICKING) + statGetPER(stats[gui_player], players[gui_player]->entity)) / 20; // 0 to 5
	}

	switch ( item->type )
	{
		case TOOL_LOCKPICK:
		case TOOL_GLASSES:
		case POTION_EMPTY:
		case TOOL_DECOY:
			if ( stats[gui_player]->getModifiedProficiency(PRO_LOCKPICKING) + statGetPER(stats[gui_player], players[gui_player]->entity) >= TINKER_MIN_ITEM_SKILL_REQ ) // 10 requirement
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
		case MASK_TECH_GOGGLES:
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
			messagePlayer(gui_player, MESSAGE_MISC, Language::get(681), toDegrade->getName());
		}
		else
		{
			messagePlayer(gui_player, MESSAGE_MISC, Language::get(662), toDegrade->getName());
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
	if ( local_rng.rand() % 20 == 10 )
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
	//	messagePlayer(gui_player, Language::get(3681));
	//	return false; // don't want to deal with client/server desync problems here.
	//}

	if ( stats[gui_player] && players[gui_player] )
	{
		bool isEquipped = itemIsEquipped(item, gui_player);

		if ( item->type == TOOL_SENTRYBOT || item->type == TOOL_SPELLBOT || item->type == TOOL_DUMMYBOT || item->type == TOOL_GYROBOT )
		{
			if ( item->tinkeringBotIsMaxHealth() )
			{
				// try upgrade item?
				int craftRequirement = tinkeringPlayerHasSkillLVLToCraft(item);
				if ( craftRequirement == -1 ) // can't craft, can't upgrade!
				{
					//playSound(90, 64);
					messagePlayer(gui_player, MESSAGE_MISC, Language::get(3685), items[item->type].getIdentifiedName());
					return false;
				}
				else if ( !tinkeringPlayerCanAffordRepair(item) )
				{
					//playSound(90, 64);
					messagePlayer(gui_player, MESSAGE_MISC, Language::get(3687), items[item->type].getIdentifiedName());
					return false;
				}
				
				Status newStatus = DECREPIT;
				Status maxStatus = static_cast<Status>(tinkeringUpgradeMaxStatus(item));

				if ( maxStatus <= item->status )
				{
					//playSound(90, 64);
					messagePlayer(gui_player, MESSAGE_MISC, Language::get(3685), items[item->type].getIdentifiedName());
					return false;
				}

				if ( tinkeringConsumeMaterialsForRepair(item, true) )
				{
					newStatus = std::min(static_cast<Status>(item->status + 1), maxStatus);
					if ( !isEquipped )
					{
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
										hotbarSlot.resetLastItem();
									}
								}
							}
							free(upgradedItem);
						}
					}
					else
					{
						item->status = newStatus;
						item->appearance = ITEM_TINKERING_APPEARANCE;
					}

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

						strcpy((char*)net_packet->data, "REPT");
						net_packet->data[4] = gui_player;
						net_packet->data[5] = armornum;
						net_packet->data[6] = item->status;
						SDLNet_Write32((Uint32)item->appearance, &net_packet->data[7]);
						net_packet->address.host = net_server.host;
						net_packet->address.port = net_server.port;
						net_packet->len = 11;
						sendPacketSafe(net_sock, -1, net_packet, 0);
					}
					messagePlayer(gui_player, MESSAGE_MISC, Language::get(3683), items[item->type].getIdentifiedName());
					if ( !isEquipped )
					{
						consumeItem(item, gui_player);
					}
					return true;
				}
			}
			else
			{
				int craftRequirement = tinkeringPlayerHasSkillLVLToCraft(item);
				if ( craftRequirement == -1 ) // can't craft, can't repair!
				{
					//playSound(90, 64);
					messagePlayer(gui_player, MESSAGE_MISC, Language::get(3688), items[item->type].getIdentifiedName());
					return false;
				}
				else if ( !tinkeringPlayerCanAffordRepair(item) )
				{
					//playSound(90, 64);
					messagePlayer(gui_player, MESSAGE_MISC, Language::get(3686), items[item->type].getIdentifiedName());
					return false;
				}

				if ( tinkeringConsumeMaterialsForRepair(item, false) )
				{
					Uint32 repairedAppearance = std::min((item->appearance % 10) + 1, static_cast<Uint32>(4));
					if ( repairedAppearance == 4 )
					{
						repairedAppearance = ITEM_TINKERING_APPEARANCE;
					}
					if ( !isEquipped )
					{
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
										hotbarSlot.resetLastItem();
									}
								}
							}
							free(repairedItem);
						}
					}
					else
					{
						item->appearance = repairedAppearance;
					}

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

						strcpy((char*)net_packet->data, "REPT");
						net_packet->data[4] = gui_player;
						net_packet->data[5] = armornum;
						net_packet->data[6] = item->status;
						SDLNet_Write32((Uint32)item->appearance, &net_packet->data[7]);
						net_packet->address.host = net_server.host;
						net_packet->address.port = net_server.port;
						net_packet->len = 11;
						sendPacketSafe(net_sock, -1, net_packet, 0);
					}
					messagePlayer(gui_player, MESSAGE_MISC, Language::get(3682), items[item->type].getIdentifiedName());
					if ( !isEquipped )
					{
						consumeItem(item, gui_player);
					}
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
				messagePlayer(gui_player, MESSAGE_MISC, Language::get(3688), items[item->type].getIdentifiedName());
				return false;
			}
			if ( !tinkeringPlayerCanAffordRepair(item) )
			{
				//playSound(90, 64);
				messagePlayer(gui_player, MESSAGE_MISC, Language::get(3686), items[item->type].getIdentifiedName());
				return false;
			}

			if ( tinkeringConsumeMaterialsForRepair(item, false) )
			{
				Compendium_t::Events_t::eventUpdate(gui_player, Compendium_t::CPDM_REPAIRS, item->type, 1);

				int repairedStatus = std::min(static_cast<Status>(item->status + 1), EXCELLENT);
				item->status = static_cast<Status>(repairedStatus);
				messagePlayer(gui_player, MESSAGE_MISC, Language::get(872), item->getName());
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
									hotbarSlot.resetLastItem();
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
		skillLVL = (stats[gui_player]->getModifiedProficiency(PRO_LOCKPICKING) + statGetPER(stats[gui_player], players[gui_player]->entity)) / 20; // 0 to 5
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
				if ( local_rng.rand() % 40 == 0 )
				{
					increaseSkill = true;
				}
			}
			else
			{
				if ( local_rng.rand() % 10 == 0 )
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
	int itemIndexY = 0;
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
			Item* item = items[items.size() - 1];
			item->x = 0;
			item->y = itemIndexY;
			++itemIndexY;
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
	int usageCostMin = 0;
	int usageCostMax = 0;
	scribingGetChargeCost(itemUsedWith, usageCostMin, usageCostMax);
	int usageCost = usageCostMin;
	int randomValue = usageCostMax - usageCostMin;
	if ( randomValue > 0 )
	{
		usageCost += local_rng.rand() % (randomValue + 1);
	}

	if ( durability - usageCost < 0 )
	{
		Compendium_t::Events_t::eventUpdate(gui_player, Compendium_t::CPDM_FEATHER_CHARGE_USED, ENCHANTED_FEATHER, durability);
		toDegrade->status = BROKEN;
		toDegrade->appearance = 0;
	}
	else
	{
		scribingLastUsageDisplayTimer = 200;
		scribingLastUsageAmount = usageCost;
		Compendium_t::Events_t::eventUpdate(gui_player, Compendium_t::CPDM_FEATHER_CHARGE_USED, ENCHANTED_FEATHER, scribingLastUsageAmount);
		toDegrade->appearance -= usageCost;
		if ( toDegrade->appearance % ENCHANTED_FEATHER_MAX_DURABILITY == 0 )
		{
			toDegrade->status = BROKEN;
			messagePlayer(gui_player, MESSAGE_EQUIPMENT, Language::get(3727), toDegrade->getName());
			scribingToolItem = nullptr;
			return usageCost;
		}
		else
		{
			if ( durability > 25 && (toDegrade->appearance % ENCHANTED_FEATHER_MAX_DURABILITY) <= 25 )
			{
				// notify we're at less than 25%.
				messagePlayer(gui_player, MESSAGE_EQUIPMENT, Language::get(3729), toDegrade->getName());
			}
		}
	}
	if ( toDegrade->status > BROKEN )
	{
		//messagePlayer(gui_player, Language::get(681), toDegrade->getName());
		return usageCost;
	}
	else
	{
		if ( (usageCostMin / 2) < durability && itemCategory(itemUsedWith) == SCROLL )
		{
			// if scroll cost is a little more than the durability, then let it succeed.
			messagePlayer(gui_player, MESSAGE_EQUIPMENT, Language::get(3727), toDegrade->getName());
			scribingToolItem = nullptr;
			return usageCost;
		}
		else
		{
			messagePlayer(gui_player, MESSAGE_EQUIPMENT, Language::get(3728), toDegrade->getName());
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

void GenericGUIMenu::scribingGetChargeCost(Item* itemUsedWith, int& outChargeCostMin, int& outChargeCostMax)
{
	outChargeCostMin = 0;
	outChargeCostMax = 0;
	if ( !itemUsedWith || !scribingToolItem )
	{
		return;
	}

	if ( itemCategory(itemUsedWith) == SPELLBOOK )
	{
		outChargeCostMin = 16;
	}
	else if ( itemCategory(itemUsedWith) == SCROLL )
	{
		switch ( itemUsedWith->type )
		{
			case SCROLL_MAIL:
				outChargeCostMin = 2;
				break;
			case SCROLL_DESTROYARMOR:
			case SCROLL_FIRE:
			case SCROLL_LIGHT:
				outChargeCostMin = 4;
				break;
				break;
			case SCROLL_SUMMON:
			case SCROLL_IDENTIFY:
			case SCROLL_REMOVECURSE:
				outChargeCostMin = 6;
				break;
			case SCROLL_FOOD:
			case SCROLL_TELEPORTATION:
				outChargeCostMin = 8;
				break;
			case SCROLL_REPAIR:
			case SCROLL_MAGICMAPPING:
				outChargeCostMin = 12;
				break;
			case SCROLL_ENCHANTWEAPON:
			case SCROLL_ENCHANTARMOR:
				outChargeCostMin = 16;
				break;
			default:
				outChargeCostMin = 8;
				break;
		}
	}

	outChargeCostMax = outChargeCostMin;
	int randomValue = 0;
	if ( stats[gui_player] )
	{
		int skillLVL = 0;
		if ( stats[gui_player] && players[gui_player] )
		{
			skillLVL = (stats[gui_player]->getModifiedProficiency(PRO_MAGIC) + statGetINT(stats[gui_player], players[gui_player]->entity)) / 20; // 0 to 5
		}
		if ( scribingToolItem->beatitude > 0 )
		{
			skillLVL = 5; // blessed feather.
		}
		else if ( scribingToolItem->beatitude < 0 )
		{
			skillLVL = 0; // cursed feather.
		}

		if ( skillLVL >= 4 )
		{
			outChargeCostMin = std::max(2, outChargeCostMin - 4);
			outChargeCostMax = std::max(2, outChargeCostMax);
		}
		else if ( skillLVL < 2 )
		{
			outChargeCostMax += 6;
		}
		else if ( skillLVL == 2 )
		{
			outChargeCostMax += 4;
		}
		else if ( skillLVL == 3 )
		{
			outChargeCostMax += 2;
		}
	}
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
			if ( local_rng.rand() % 5 == 0 )
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
		std::string label = item->getScrollLabel();
		Item* crafted = newItem(item->type, scribingBlankScrollTarget->status, 
			scribingBlankScrollTarget->beatitude, 1, item->appearance, true, nullptr);
		if ( crafted )
		{
			if ( crafted->type == SCROLL_MAIL )
			{
				// mail uses the appearance to generate the text, so randomise it here.
				crafted->appearance = local_rng.rand();
			}
			Item* pickedUp = itemPickup(gui_player, crafted);
			//messagePlayerColor(gui_player, uint32ColorGreen, Language::get(3724));
			int oldcount = pickedUp->count;
			pickedUp->count = 1;
			messagePlayerColor(gui_player, MESSAGE_INVENTORY, uint32ColorGreen, Language::get(3724), pickedUp->description());
			pickedUp->count = oldcount;
			consumeItem(scribingBlankScrollTarget, gui_player);
			featherGUI.inscribeSuccessName = label;
			featherGUI.inscribeSuccessTicks = ticks;
			if ( client_classes[gui_player] == CLASS_SHAMAN )
			{
				steamStatisticUpdate(STEAM_STAT_ROLL_THE_BONES, STEAM_STAT_INT, 1);
			}
			Compendium_t::Events_t::eventUpdate(gui_player, Compendium_t::CPDM_FEATHER_ENSCRIBED, ENCHANTED_FEATHER, 1);
			for ( int i = 0; i < NUMLABELS; ++i )
			{
				if ( label == scroll_label[i] )
				{
					if ( clientLearnedScrollLabels[gui_player].find(i) == clientLearnedScrollLabels[gui_player].end() )
					{
						featherGUI.labelDiscoveries[label] = FeatherGUI_t::DiscoveryAnim_t();
					}
					clientLearnedScrollLabels[gui_player].insert(i);
					break;
				}
			}

			free(crafted);

			playSound(546, 92);
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
			if ( local_rng.rand() % 10 == 0 )
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

		Compendium_t::Events_t::eventUpdate(gui_player, Compendium_t::CPDM_FEATHER_SPELLBOOKS, ENCHANTED_FEATHER, 1);

		int repairedStatus = std::min(static_cast<Status>(item->status + 1), EXCELLENT);
		bool isEquipped = itemIsEquipped(item, gui_player);
		item->status = static_cast<Status>(repairedStatus);
		messagePlayer(gui_player, MESSAGE_MISC, Language::get(3725));
		messagePlayer(gui_player, MESSAGE_INVENTORY, Language::get(872), item->getName());
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
							hotbarSlot.resetLastItem();
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
		playSound(546, 92);
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
	//			real_t fpsScale = getFPSScale(144.0);
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
			if ( entity->behavior == &actMonster && entity->getMonsterTypeFromSprite() == BAT_SMALL )
			{
				if ( entity->bodyparts.size() > 0 )
				{
					worldZ += entity->bodyparts[0]->lerpRenderState.z.position;
				}
			}
		}
		else
		{
			worldX = entity->x;
			worldY = entity->y;
			worldZ = entity->z + enemyBarSettings.getHeightOffset(entity);
			if ( entity->behavior == &actMonster && entity->getMonsterTypeFromSprite() == BAT_SMALL )
			{
				if ( entity->bodyparts.size() > 0 )
				{
					worldZ += entity->bodyparts[0]->z;
				}
			}
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
		if ( entity->behavior == &actMonster 
			&& entity->monsterAttack == MONSTER_POSE_MAGIC_WINDUP2
			&& entity->getMonsterTypeFromSprite() == SLIME )
		{
			worldZ += entity->focalz / 2;
		}
		else if ( entity->behavior == &actMonster && entity->getMonsterTypeFromSprite() == MIMIC )
		{
			if ( entity->isInertMimic() )
			{
				enemy_name = Language::get(675);
			}
			else
			{
				enemy_name = getMonsterLocalizedName(MIMIC);
			}

			if ( entity->bodyparts.size() > 0 )
			{
				auto limb = entity->bodyparts[0];
				worldZ += (limb->z - entity->z) / 2; // offset to trunk animation
			}
		}
		screenDistance = enemyBarSettings.getScreenDistanceOffset(entity);
	}
}

EnemyHPDamageBarHandler::EnemyHPDetails* EnemyHPDamageBarHandler::addEnemyToList(Sint32 HP, Sint32 maxHP, Sint32 oldHP, Uint32 uid, const char* name, bool isLowPriority, DamageGib gibDmgType)
{
	auto find = HPBars.find(uid);
	EnemyHPDetails* details = nullptr;
	if ( find != HPBars.end() )
	{
		// uid exists in list.
		(*find).second.enemy_hp = HP;
		(*find).second.enemy_maxhp = maxHP;
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
		HPBars.insert(std::make_pair(uid, EnemyHPDetails(uid, HP, maxHP, oldHP, name, isLowPriority)));
		auto find = HPBars.find(uid);
		details = &(*find).second;
		details->animator.previousSetpoint = details->enemy_oldhp;
		details->animator.backgroundValue = details->enemy_oldhp;
	}

	details->animator.maxValue = details->enemy_maxhp;
	details->animator.setpoint = details->enemy_hp;
	details->animator.foregroundValue = details->animator.setpoint;
	details->animator.animateTicks = ticks;
	details->animator.damageTaken = std::max(-details->enemy_maxhp, oldHP - HP); // IDK if this needs a lower limit for healing

	Entity* entity = uidToEntity(uid);
	spawnDamageGib(entity, details->animator.damageTaken, gibDmgType);
	lastEnemyUid = uid;

	if ( entity )
	{
		details->updateWorldCoordinates();
	}

	details->enemy_statusEffects1 = 0;
	details->enemy_statusEffects2 = 0;
	details->enemy_statusEffectsLowDuration1 = 0;
	details->enemy_statusEffectsLowDuration2 = 0;

	if ( entity && (entity->behavior == &actPlayer || entity->behavior == &actMonster) && multiplayer != CLIENT )
	{
		if ( Stat* stat = entity->getStats() )
		{
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
	return details;
}

const int GenericGUIMenu::TinkerGUI_t::MAX_TINKER_X = 5;
const int GenericGUIMenu::TinkerGUI_t::MAX_TINKER_Y = 4;

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

int GenericGUIMenu::TinkerGUI_t::heightOffsetWhenNotCompact = 190;
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

	bool pauseChangeScrapAnim = false;
	if ( playerChangeMetalScrap != 0 || playerChangeMagicScrap != 0 )
	{
		if ( true || ((ticks - animScrapStartTicks) > TICKS_PER_SECOND / 2) )
		{
			const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
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
			pauseChangeScrapAnim = true;

			const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animScrap)) / 10.0;
			animScrap += setpointDiffX;
			animScrap = std::min(1.0, animScrap);
		}
	}

	{
		bool pauseChangeScrapAnim = false;
		bool showChangedMetalScrap = false;
		if ( playerChangeMetalScrap != 0 )
		{
			int displayedChangeMetalScrap = animScrap * playerChangeMetalScrap;
			if ( pauseChangeScrapAnim )
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
			if ( pauseChangeScrapAnim )
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
	if ( !bConstructDrawerOpen )
	{
		drawerJustifyInverted = false;
	}

	if ( !tinkerFrame->isDisabled() && bOpen )
	{
		if ( !tinkerGUIHasBeenCreated() )
		{
			createTinkerMenu();
		}

		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
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
		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
		real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animFilter)) / 2.0;
		animFilter += setpointDiffX;
		animFilter = std::min(1.0, animFilter);
	}
	else
	{
		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
		real_t setpointDiffX = fpsScale * std::max(.01, (animFilter)) / 2.0;
		animFilter -= setpointDiffX;
		animFilter = std::max(0.0, animFilter);
	}

	bool reversed = false;
	auto tinkerFramePos = tinkerFrame->getSize();
	if ( player->inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_LEFT )
	{
		if ( !player->inventoryUI.bCompactView )
		{
			const int fullWidth = tinkerFramePos.w + 210; // inventory width 210
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
			if ( player->bAlignGUINextToInventoryCompact() )
			{
				const int fullWidth = tinkerFramePos.w + 210; // inventory width 210
				tinkerFramePos.x = -tinkerFramePos.w + animx * fullWidth;
			}
			else
			{
				tinkerFramePos.x = player->camera_virtualWidth() - animx * tinkerFramePos.w;
			}
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
		reversed = true;
		if ( !player->inventoryUI.bCompactView )
		{
			const int fullWidth = tinkerFramePos.w + 210; // inventory width 210
			tinkerFramePos.x = player->camera_virtualWidth() - animx * fullWidth;
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
			if ( player->bAlignGUINextToInventoryCompact() )
			{
				const int fullWidth = tinkerFramePos.w + 210; // inventory width 210
				tinkerFramePos.x = player->camera_virtualWidth() - animx * fullWidth;
			}
			else
			{
				tinkerFramePos.x = -tinkerFramePos.w + animx * tinkerFramePos.w;
			}
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
	drawerJustifyInverted = reversed;

	if ( !player->bUseCompactGUIHeight() && !player->bUseCompactGUIWidth() )
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
		const int widthDifference = animDrawer * (drawerFramePos.w);
		if ( drawerJustifyInverted )
		{
			drawerFramePos.x = baseFramePos.x + baseFramePos.w + animDrawer * (drawerFramePos.w) - (drawerFramePos.w);
		}
		else
		{
			drawerFramePos.x = 0;
		}
		drawerFramePos.y = 18;
		drawerFrame->setSize(drawerFramePos);

		if ( !drawerJustifyInverted )
		{
			tinkerFramePos.x -= widthDifference;
		}
		int adjustx = 0;
		if ( tinkerFramePos.x < 0 )
		{
			adjustx = -tinkerFramePos.x; // to not slide off-frame
			tinkerFramePos.x += adjustx;
		}
		tinkerFramePos.w += (widthDifference);
		tinkerFramePos.h = std::max(drawerFramePos.y + drawerFramePos.h, baseFramePos.y + baseFramePos.h);
		tinkerFrame->setSize(tinkerFramePos);
		if ( !drawerJustifyInverted )
		{
			baseFramePos.x = tinkerFramePos.w - baseFramePos.w;
		}
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
		messagePlayer(playernum, MESSAGE_MISC, Language::get(4159));
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

		SDL_Rect textPos{ 0, 27, baseFrame->getSize().w, 24 };
		tinkerKitTitle->setSize(textPos);
		textPos.y += 20;
		tinkerKitStatus->setSize(textPos);

		auto skillIcon = baseFrame->findImage("tinker skill img");
		for ( auto& skill : Player::SkillSheet_t::skillSheetData.skillEntries )
		{
			if ( skill.skillId == PRO_LOCKPICKING )
			{
				if ( skillCapstoneUnlocked(playernum, skill.skillId) )
				{
					skillIcon->path = skill.skillIconPathLegend;
				}
				else
				{
					skillIcon->path = skill.skillIconPath;
				}
				skillIcon->disabled = false;
				break;
			}
		}
	}

	// drawer title
	if ( bConstructDrawerOpen )
	{
		auto blueprintsTitle = drawerFrame->findField("blueprints title");
		blueprintsTitle->setText(Language::get(4199));
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
		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
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
		heldScrapBg->pos.y = baseFrame->getSize().h - 48 - 32;

		SDL_Rect metalPos{ heldScrapBg->pos.x - 4, heldScrapBg->pos.y + 9, 74, 24 };
		SDL_Rect magicPos{ heldScrapBg->pos.x - 24, heldScrapBg->pos.y + 9, 166, 24 };
		metalHeldText->setSize(metalPos);
		metalHeldText->setColor(hudColors.characterSheetLightNeutral);
		magicHeldText->setSize(magicPos);
		magicHeldText->setColor(hudColors.characterSheetLightNeutral);

		updateTinkerScrapHeld(metalHeldText, magicHeldText, heldMetalScrap, heldMagicScrap);

		SDL_Rect heldScrapTxtPos = heldScrapText->getSize();
		heldScrapTxtPos.w = heldScrapBg->pos.x + 2;
		heldScrapTxtPos.x = 0;
		heldScrapTxtPos.y = metalPos.y;
		heldScrapTxtPos.h = 24;
		heldScrapText->setSize(heldScrapTxtPos);
		heldScrapText->setText(Language::get(4131));
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

		Uint32 inactiveColor = hudColors.characterSheetDarker1Neutral;
		Uint32 activeColor = hudColors.characterSheetOffWhiteText;
		Uint32 highlightColor = hudColors.characterSheetLighter1Neutral;

		filterBtn->setColor(makeColor(255, 255, 255, 255));
		filterBtn->setBackground("*#images/ui/Tinkering/Tinker_Filter_Center_00.png");
		Field* filterTxt = baseFrame->findField("filter salvage txt");
		filterTxt->setDisabled(false);
		filterTxt->setText(Language::get(3645));
		filterTxt->setColor(inactiveColor);
		if ( false && filterBtn->isHighlighted() )
		{
			filterTxt->setColor(highlightColor);
		}
		else if ( parentGUI.tinkeringFilter == GenericGUIMenu::TINKER_FILTER_SALVAGEABLE )
		{
			filterTxt->setColor(activeColor);
		}
		{
			SDL_Rect btnPos{ 116, 274, 94, 36 };
			filterBtn->setSize(btnPos);
			SDL_Rect txtPos = btnPos;
			txtPos.y += 5;
			txtPos.h = 24;
			filterTxt->setSize(txtPos);
		}
		if ( parentGUI.tinkeringFilter == GenericGUIMenu::TINKER_FILTER_SALVAGEABLE )
		{
			filterBtn->setColor(makeColor(255, 255, 255, 255));
			filterBtn->setBackground("*#images/ui/Tinkering/Tinker_Filter_CenterHigh_00.png");
			SDL_Rect txtPos = filterTxt->getSize();
			txtPos.y += 2;
			filterTxt->setSize(txtPos);
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
		filterBtn->setColor(makeColor(255, 255, 255, 255));
		filterBtn->setBackground("*#images/ui/Tinkering/Tinker_Filter_Left_00.png");
		filterTxt = baseFrame->findField("filter craft txt");
		filterTxt->setDisabled(false);
		filterTxt->setText(Language::get(3644));

		filterTxt->setColor(inactiveColor);
		if ( false && filterBtn->isHighlighted() )
		{
			filterTxt->setColor(highlightColor);
		}
		else if ( parentGUI.tinkeringFilter == GenericGUIMenu::TINKER_FILTER_CRAFTABLE )
		{
			filterTxt->setColor(activeColor);
		}
		{
			SDL_Rect btnPos{ 44, 274, 74, 36 };
			filterBtn->setSize(btnPos);
			SDL_Rect txtPos = btnPos;
			txtPos.y += 5;
			txtPos.h = 24;
			filterTxt->setSize(txtPos);
			filterLeftSideX = btnPos.x;
			filterStartY = btnPos.y;
		}
		if ( parentGUI.tinkeringFilter == GenericGUIMenu::TINKER_FILTER_CRAFTABLE )
		{
			filterBtn->setColor(makeColor(255, 255, 255, 255));
			filterBtn->setBackground("*#images/ui/Tinkering/Tinker_Filter_LeftHigh_00.png");
			SDL_Rect txtPos = filterTxt->getSize();
			txtPos.y += 2;
			filterTxt->setSize(txtPos);
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
		filterBtn->setColor(makeColor(255, 255, 255, 255));
		filterBtn->setBackground("*#images/ui/Tinkering/Tinker_Filter_Right_00.png");
		filterTxt = baseFrame->findField("filter repair txt");
		filterTxt->setDisabled(false);
		filterTxt->setText(Language::get(3646));
		filterTxt->setColor(inactiveColor);
		if ( false && filterBtn->isHighlighted() )
		{
			filterTxt->setColor(highlightColor);
		}
		else if ( parentGUI.tinkeringFilter == GenericGUIMenu::TINKER_FILTER_REPAIRABLE )
		{
			filterTxt->setColor(activeColor);
		}
		{
			SDL_Rect btnPos{ 208, 274, 82, 36 };
			filterBtn->setSize(btnPos);
			SDL_Rect txtPos = btnPos;
			txtPos.y += 5;
			txtPos.h = 24;
			filterTxt->setSize(txtPos);
			filterRightSideX = btnPos.x + btnPos.w;
		}
		if ( parentGUI.tinkeringFilter == GenericGUIMenu::TINKER_FILTER_REPAIRABLE )
		{
			filterBtn->setColor(makeColor(255, 255, 255, 255));
			filterBtn->setBackground("*#images/ui/Tinkering/Tinker_Filter_RightHigh_00.png");
			SDL_Rect txtPos = filterTxt->getSize();
			txtPos.y += 2;
			filterTxt->setSize(txtPos);
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

	int skillLVL = (stats[playernum]->getModifiedProficiency(PRO_LOCKPICKING) + statGetPER(stats[playernum], players[playernum]->entity));
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
	static ConsoleVariable<int> cvar_tinkerPromptY("/tinker_action_prompt_y", -2);
	SDL_Rect actionPromptTxtPos{ 0, 211 + *cvar_tinkerPromptY, baseFrame->getSize().w - 18 - 8, 24 };
	actionPromptTxt->setSize(actionPromptTxtPos);

	SDL_Rect tooltipPos = itemDisplayTooltip->getSize();
	tooltipPos.w = 298;
	tooltipPos.h = baseFrame->getSize().h - 100;
	tooltipPos.y = 98;
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
		costBg->pos.y = displayItemTextImg->pos.y + displayItemTextImg->pos.h + 4;
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
		&& (keystatus[SDLK_LSHIFT] || keystatus[SDLK_RSHIFT]) )
	{
		modifierPressed = true;
	}

	if ( itemActionType != TINKER_ACTION_NONE && itemDesc.size() > 1 )
	{
		if ( isInteractable )
		{
			//const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
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
						actionModifierImg->path = GlyphHelper.getGlyphPath(SDLK_LSHIFT, false);
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
					actionPromptTxt->setText(Language::get(3644));
				}
				else if ( parentGUI.tinkeringFilter == GenericGUIMenu::TINKER_FILTER_REPAIRABLE )
				{
					if ( itemActionType == TINKER_ACTION_OK_UPGRADE )
					{
						actionPromptTxt->setText(Language::get(3684));
					}
					else
					{
						actionPromptTxt->setText(Language::get(3646));
					}
				}
				else if ( parentGUI.tinkeringFilter == GenericGUIMenu::TINKER_FILTER_SALVAGEABLE )
				{
					if ( modifierPressed )
					{
						actionPromptTxt->setText(Language::get(4154));
					}
					else
					{
						actionPromptTxt->setText(Language::get(3645));
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
							actionPromptTxt->setText(Language::get(4138));
						}
						else if ( parentGUI.tinkeringFilter == TINKER_FILTER_SALVAGEABLE )
						{
							actionPromptTxt->setText(Language::get(4137));
						}
						break;
					case TINKER_ACTION_INVALID_ROBOT_TO_SALVAGE:
						actionPromptTxt->setText(Language::get(4148));
						break;
					case TINKER_ACTION_NO_MATERIALS_UPGRADE:
						if ( parentGUI.tinkeringFilter == TINKER_FILTER_REPAIRABLE )
						{
							actionPromptTxt->setText(Language::get(4142));
						}
						break;
					case TINKER_ACTION_NO_MATERIALS:
						if ( parentGUI.tinkeringFilter == TINKER_FILTER_REPAIRABLE )
						{
							actionPromptTxt->setText(Language::get(4141));
						}
						else if ( parentGUI.tinkeringFilter == TINKER_FILTER_SALVAGEABLE )
						{
							actionPromptTxt->setText(Language::get(4137));
						}
						else if ( parentGUI.tinkeringFilter == TINKER_FILTER_CRAFTABLE )
						{
							actionPromptTxt->setText(Language::get(4140));
						}
						break;
					case TINKER_ACTION_NO_SKILL_LVL:
						if ( parentGUI.tinkeringFilter == TINKER_FILTER_CRAFTABLE )
						{
							char buf[128];
							snprintf(buf, sizeof(buf), Language::get(4147), skillLVL, itemRequirement);
							actionPromptTxt->setText(buf);
						}
						else
						{
							char buf[128];
							snprintf(buf, sizeof(buf), Language::get(4144), skillLVL, itemRequirement);
							actionPromptTxt->setText(buf);
						}
						break;
					case TINKER_ACTION_NO_SKILL_LVL_UPGRADE:
						char buf[128];
						snprintf(buf, sizeof(buf), Language::get(4145), skillLVL, itemRequirement);
						actionPromptTxt->setText(buf);
						break;
					case TINKER_ACTION_ITEM_FULLY_REPAIRED:
						actionPromptTxt->setText(Language::get(4136));
						break;
					case TINKER_ACTION_ITEM_FULLY_UPGRADED:
						actionPromptTxt->setText(Language::get(4139));
						break;
					case TINKER_ACTION_ROBOT_BROKEN:
						actionPromptTxt->setText(Language::get(4143));
						break;
					case TINKER_ACTION_MUST_BE_UNEQUIPPED:
						actionPromptTxt->setText(Language::get(4132));
						break;
					case TINKER_ACTION_ALREADY_USING_THIS_TINKERING_KIT:
						actionPromptTxt->setText(Language::get(4146));
						break;
					case TINKER_ACTION_KIT_NEEDS_REPAIRS:
						actionPromptTxt->setText(Language::get(4152));
						break;
					case TINKER_ACTION_NOT_IDENTIFIED_YET:
						actionPromptTxt->setText(Language::get(4153));
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
			|| (usingGamepad)
			|| animTooltip < 0.9999 )
		{
			const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.01, (animTooltip)) / 2.0;
			animTooltip -= setpointDiffX;
			animTooltip = std::max(0.0, animTooltip);
		}
	}

	if ( parentGUI.tinkeringFilter == TINKER_FILTER_CRAFTABLE )
	{
		costScrapText->setText(Language::get(4130));
	}
	else if ( parentGUI.tinkeringFilter == TINKER_FILTER_REPAIRABLE )
	{
		bool activeTooltip = (itemActionType != TINKER_ACTION_NONE && itemDesc.size() > 1);
		std::string currentText = costScrapText->getText();
		if ( activeTooltip || (!activeTooltip
			&& currentText != Language::get(4135)
			&& currentText != Language::get(4134)) ) // if inactive tooltip, don't quickly change between upgrade/repair
		{
			if ( itemType == TOOL_SENTRYBOT
				|| itemType == TOOL_SPELLBOT
				|| itemType == TOOL_DUMMYBOT
				|| itemType == TOOL_GYROBOT )
			{
				costScrapText->setText(Language::get(4135));
			}
			else
			{
				costScrapText->setText(Language::get(4134));
			}
		}
	}
	else if ( parentGUI.tinkeringFilter == TINKER_FILTER_SALVAGEABLE )
	{
		costScrapText->setText(Language::get(4133));
	}

	auto actionPromptUnselectedTxt = baseFrame->findField("action prompt unselected txt");
	auto actionPromptCoverLeftImg = baseFrame->findImage("action prompt lcover");
	auto actionPromptCoverRightImg = baseFrame->findImage("action prompt rcover");
	actionPromptCoverLeftImg->pos.x = 0;
	actionPromptCoverRightImg->pos.x = baseFrame->getSize().w - actionPromptCoverLeftImg->pos.w;
	actionPromptCoverLeftImg->pos.y = 70;
	actionPromptCoverRightImg->pos.y = 70;

	{
		actionPromptUnselectedTxt->setDisabled(false);
		actionPromptUnselectedTxt->setColor(makeColor(224, 224, 224, 255));
		if ( parentGUI.tinkeringFilter == TINKER_FILTER_CRAFTABLE )
		{
			actionPromptUnselectedTxt->setText(Language::get(4149));
		}
		else if ( parentGUI.tinkeringFilter == TINKER_FILTER_SALVAGEABLE )
		{
			actionPromptUnselectedTxt->setText(Language::get(4150));
		}
		else if ( parentGUI.tinkeringFilter == TINKER_FILTER_REPAIRABLE )
		{
			actionPromptUnselectedTxt->setText(Language::get(4151));
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
			pos.y = 73;
			actionPromptUnselectedTxt->setSize(pos);
		}

		if ( ticks - animPromptTicks > TICKS_PER_SECOND / 10 )
		{
			const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
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
	auto filterNavLeftArrow = baseFrame->findImage("filter nav left arrow");
	filterNavLeftArrow->disabled = true;
	auto filterNavRightArrow = baseFrame->findImage("filter nav right arrow");
	filterNavRightArrow->disabled = true;

	static ConsoleVariable<int> cvar_tinkNavGlyphX("/tinker_glyph_nav_x", 8);
	static ConsoleVariable<int> cvar_tinkNavGlyphY("/tinker_glyph_nav_y", 30);

	if ( usingGamepad )
	{
		filterNavLeft->path = Input::inputs[playernum].getGlyphPathForBinding("MenuPageLeft");
		if ( auto imgGet = Image::get(filterNavLeft->path.c_str()) )
		{
			filterNavLeft->pos.w = imgGet->getWidth();
			filterNavLeft->pos.h = imgGet->getHeight();
			filterNavLeft->disabled = false;

			filterNavLeftArrow->disabled = false;
			filterNavLeftArrow->pos.x = 12;
			filterNavLeftArrow->pos.y = filterStartY - 6;

			filterNavLeft->pos.x = filterNavLeftArrow->pos.x - 10 + *cvar_tinkNavGlyphX;
			filterNavLeft->pos.y = filterNavLeftArrow->pos.y + filterNavLeftArrow->pos.h - 2;
			filterNavLeft->pos.y -= filterNavLeft->pos.h;
			filterNavLeft->pos.y -= *cvar_tinkNavGlyphY;
		}

		filterNavRight->path = Input::inputs[playernum].getGlyphPathForBinding("MenuPageRight");
		if ( auto imgGet = Image::get(filterNavRight->path.c_str()) )
		{
			filterNavRight->pos.w = imgGet->getWidth();
			filterNavRight->pos.h = imgGet->getHeight();
			filterNavRight->disabled = false;

			filterNavRightArrow->disabled = false;
			filterNavRightArrow->pos.x = filterRightSideX + 2;
			filterNavRightArrow->pos.y = filterStartY - 6;

			filterNavRight->pos.x = filterNavRightArrow->pos.x + filterNavRightArrow->pos.w + 10;
			filterNavRight->pos.x -= filterNavRight->pos.w + *cvar_tinkNavGlyphX;
			filterNavRight->pos.y = filterNavRightArrow->pos.y + filterNavRightArrow->pos.h - 2;
			filterNavRight->pos.y -= filterNavRight->pos.h;
			filterNavRight->pos.y -= *cvar_tinkNavGlyphY;
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
				Player::soundCancel();
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
					Player::soundModuleNavigation();
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
					Player::soundModuleNavigation();
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
							parentGUI.tinkeringBulkSalvageMetalScrap = 0;
							parentGUI.tinkeringBulkSalvageMagicScrap = 0;
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

const int kTinkerCraftSlotHeightOffset = 6; // slightly taller than normal slots

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

	SDL_Rect basePos{ 0, 0, tinkerBaseWidth, 312 };
	{
		auto drawerFrame = tinkerFrame->addFrame("tinker drawer");
		SDL_Rect drawerPos{ 0, 0, 210, 256 };
		drawerFrame->setSize(drawerPos);
		drawerFrame->setHollow(false);
		auto bg = drawerFrame->addImage(drawerPos,
			makeColor(255, 255, 255, 255),
			"*images/ui/Tinkering/Tinker_Construct_Drawer_01.png", "tinker drawer img");
		drawerFrame->setDisabled(true);

		auto headerFont = "fonts/pixel_maz_multiline.ttf#16#2";
		auto blueprintsTitle = drawerFrame->addField("blueprints title", 128);
		blueprintsTitle->setFont(headerFont);
		blueprintsTitle->setText(Language::get(4199));
		blueprintsTitle->setHJustify(Field::justify_t::CENTER);
		blueprintsTitle->setVJustify(Field::justify_t::TOP);
		blueprintsTitle->setSize(SDL_Rect{ 60, 1, 90, 24 });
		blueprintsTitle->setTextColor(hudColors.characterSheetLightNeutral);
		blueprintsTitle->setOutlineColor(makeColor(29, 16, 11, 255));

		const int inventorySlotSize = players[player]->inventoryUI.getSlotSize();

		tinkerSlotFrames.clear();

		const int baseSlotOffsetX = 0;
		const int baseSlotOffsetY = 0;

		SDL_Rect tinkerSlotsPos{ 0, 0, 210, 256 };
		{
			const auto drawerSlotsFrame = drawerFrame->addFrame("drawer slots");
			drawerSlotsFrame->setSize(tinkerSlotsPos);
			drawerSlotsFrame->setHollow(true);

			/*auto gridImg = drawerSlotsFrame->addImage(SDL_Rect{ 0, 0, tinkerSlotsPos.w, tinkerSlotsPos.h },
			makeColor(255, 255, 255, 255), "*images/ui/Tinkering/Tinker_Construct_DrawerSlots_01.png", "grid img");
			gridImg->disabled = true;*/

			SDL_Rect currentSlotPos{ baseSlotOffsetX, baseSlotOffsetY, inventorySlotSize, inventorySlotSize };
			const int maxTinkerX = MAX_TINKER_X;
			const int maxTinkerY = MAX_TINKER_Y;

			const int slotInnerWidth = inventorySlotSize - 2;
			std::vector<std::pair<int, int>> slotCoords =
			{
				std::make_pair(4 + (slotInnerWidth + 2) * 0, 26),
				std::make_pair(4 + (slotInnerWidth + 2) * 1, 26),
				std::make_pair(4 + (slotInnerWidth + 2) * 2, 26),
				std::make_pair(4 + (slotInnerWidth + 2) * 3, 26),
				std::make_pair(4 + (slotInnerWidth + 2) * 4, 26),

				std::make_pair(4 + (slotInnerWidth + 2) * 0, 76),
				std::make_pair(4 + (slotInnerWidth + 2) * 1, 76),
				std::make_pair(4 + (slotInnerWidth + 2) * 2, 76),
				std::make_pair(4 + (slotInnerWidth + 2) * 3, 76),
				std::make_pair(4 + (slotInnerWidth + 2) * 4, 76),

				std::make_pair(4 + (slotInnerWidth + 2) * 0, 144),
				std::make_pair(4 + (slotInnerWidth + 2) * 1, 144),
				std::make_pair(4 + (slotInnerWidth + 2) * 2, 144),
				std::make_pair(4 + (slotInnerWidth + 2) * 3, 144),
				std::make_pair(4 + (slotInnerWidth + 2) * 4, 144),

				std::make_pair(4 + (slotInnerWidth + 2) * 0, 194),
				std::make_pair(4 + (slotInnerWidth + 2) * 1, 194),
				std::make_pair(4 + (slotInnerWidth + 2) * 2, 194),
				std::make_pair(4 + (slotInnerWidth + 2) * 3, 194),
				std::make_pair(4 + (slotInnerWidth + 2) * 4, 194)
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
					SDL_Rect slotPos{ currentSlotPos.x, currentSlotPos.y - kTinkerCraftSlotHeightOffset, 
						inventorySlotSize, inventorySlotSize + kTinkerCraftSlotHeightOffset };
					if ( x == 0 )
					{
						slotPos.x += 2; // left/right columns have slightly thinner width
						slotPos.w -= 2;
					}
					if ( x == maxTinkerX - 1 )
					{
						slotPos.w -= 2;
					}
					slotFrame->setSize(slotPos);

					createPlayerInventorySlotFrameElements(slotFrame);
					if ( auto itemSprite = slotFrame->findFrame("item sprite frame") )
					{
						SDL_Rect pos = itemSprite->getSize();
						pos.y += 6;
						itemSprite->setSize(pos);
					}
					if ( slotCoordsIt != slotCoords.end() )
					{
						++slotCoordsIt;
					}
					slotFrame->setDisabled(true);
				}
			}
		}
	}

	{
		auto bgFrame = tinkerFrame->addFrame("tinker base");
		bgFrame->setSize(basePos);
		bgFrame->setHollow(false);
		bgFrame->setDisabled(true);
		auto bg = bgFrame->addImage(SDL_Rect{ 0, 0, basePos.w, basePos.h },
			makeColor(255, 255, 255, 255),
			"*images/ui/Tinkering/Tinker_Construct_Base_00.png", "tinker base img");

		auto skillIcon = bgFrame->addImage(SDL_Rect{ 270, 36, 24, 24 },
			makeColor(255, 255, 255, 255),
			"", "tinker skill img");
		skillIcon->disabled = true;

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
			auto itemNameText = itemDisplayTooltip->addField("item display name", 1024);
			itemNameText->setFont(itemFont);
			itemNameText->setText("");
			itemNameText->setHJustify(Field::justify_t::LEFT);
			itemNameText->setVJustify(Field::justify_t::TOP);
			itemNameText->setSize(SDL_Rect{ 0, 0, 0, 0 });
			itemNameText->setColor(hudColors.characterSheetLightNeutral);

			auto itemDisplayTextBg = itemDisplayTooltip->addImage(SDL_Rect{ 0, 0, 220, 42 },
				0xFFFFFFFF, "*images/ui/Tinkering/Tinker_LabelName_2Row_00.png", "item text img");

			auto itemCostBg = itemDisplayTooltip->addImage(SDL_Rect{ 0, 0, 144, 34 },
				0xFFFFFFFF, "*images/ui/Tinkering/Tinker_CostBacking_00.png", "item cost img");

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
			SDL_Rect closeBtnPos{ basePos.w - 4 - 26, 14, 26, 26 };
			closeBtn->setSize(closeBtnPos);
			closeBtn->setColor(makeColor(255, 255, 255, 255));
			closeBtn->setHighlightColor(makeColor(255, 255, 255, 255));
			closeBtn->setText("X");
			closeBtn->setFont(itemFont);
			closeBtn->setHideGlyphs(true);
			closeBtn->setHideKeyboardGlyphs(true);
			closeBtn->setHideSelectors(true);
			closeBtn->setMenuConfirmControlType(0);
			closeBtn->setBackground("*images/ui/Tinkering/Button_X_00.png");
			closeBtn->setBackgroundHighlighted("*images/ui/Tinkering/Button_XHigh_00.png");
			closeBtn->setBackgroundActivated("*images/ui/Tinkering/Button_XPress_00.png");
			closeBtn->setTextHighlightColor(makeColor(201, 162, 100, 255));
			closeBtn->setCallback([](Button& button) {
				GenericGUI[button.getOwner()].closeGUI();
				Player::soundCancel();
			});
			closeBtn->setTickCallback(genericgui_deselect_fn);

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

			auto actionPromptCoverLeftImg = bgFrame->addImage(SDL_Rect{ 0, 70, 56, 26 },
				0xFFFFFFFF, "*images/ui/Tinkering/Tinker_PromptCoverLeft_00.png", "action prompt lcover");
			actionPromptCoverLeftImg->ontop = true;
			auto actionPromptCoverRightImg = bgFrame->addImage(SDL_Rect{ bg->pos.w - 56, 70, 56, 26 },
				0xFFFFFFFF, "*images/ui/Tinkering/Tinker_PromptCoverRight_00.png", "action prompt rcover");
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
			filterBtn->setText("");
			filterBtn->setFont(itemFont);
			filterBtn->setTextHighlightColor(makeColor(201, 162, 100, 255));
			filterBtn->setBackground("*#images/ui/Tinkering/Tinker_Filter_Center_00.png");
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
				if ( changeToDifferentTab )
				{
					Player::soundModuleNavigation();
				}
			});
			filterBtn->setTickCallback(genericgui_deselect_fn);

			Field* filterTxt = bgFrame->addField("filter salvage txt", 64);
			filterTxt->setFont(itemFont);
			filterTxt->setText("Salvage");
			filterTxt->setHJustify(Field::justify_t::CENTER);
			filterTxt->setVJustify(Field::justify_t::TOP);
			filterTxt->setSize(SDL_Rect{ 0, 0, 0, 0 });
			filterTxt->setColor(hudColors.characterSheetLightNeutral);
			filterTxt->setDisabled(true);
			filterTxt->setOntop(true);

			filterBtn = bgFrame->addButton("filter craft btn");
			filterBtn->setColor(makeColor(255, 255, 255, 0));
			filterBtn->setHighlightColor(makeColor(255, 255, 255, 0));
			filterBtn->setText("");
			filterBtn->setFont(itemFont);
			filterBtn->setTextHighlightColor(makeColor(201, 162, 100, 255));
			filterBtn->setBackground("*#images/ui/Tinkering/Tinker_Filter_Left_00.png");
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
				if ( changeToDifferentTab )
				{
					Player::soundModuleNavigation();
				}
			});
			filterBtn->setTickCallback(genericgui_deselect_fn);

			filterTxt = bgFrame->addField("filter craft txt", 64);
			filterTxt->setFont(itemFont);
			filterTxt->setText("Craft");
			filterTxt->setHJustify(Field::justify_t::CENTER);
			filterTxt->setVJustify(Field::justify_t::TOP);
			filterTxt->setSize(SDL_Rect{ 0, 0, 0, 0 });
			filterTxt->setColor(hudColors.characterSheetLightNeutral);
			filterTxt->setDisabled(true);
			filterTxt->setOntop(true);

			filterBtn = bgFrame->addButton("filter repair btn");
			filterBtn->setColor(makeColor(255, 255, 255, 0));
			filterBtn->setHighlightColor(makeColor(255, 255, 255, 0));
			filterBtn->setText("");
			filterBtn->setFont(itemFont);
			filterBtn->setTextHighlightColor(makeColor(201, 162, 100, 255));
			filterBtn->setBackground("*#images/ui/Tinkering/Tinker_Filter_Right_00.png");
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
				if ( changeToDifferentTab )
				{
					Player::soundModuleNavigation();
				}
			});
			filterBtn->setTickCallback(genericgui_deselect_fn);

			filterTxt = bgFrame->addField("filter repair txt", 64);
			filterTxt->setFont(itemFont);
			filterTxt->setText("Repair");
			filterTxt->setHJustify(Field::justify_t::CENTER);
			filterTxt->setVJustify(Field::justify_t::TOP);
			filterTxt->setSize(SDL_Rect{ 0, 0, 0, 0 });
			filterTxt->setColor(hudColors.characterSheetLightNeutral);
			filterTxt->setDisabled(true);
			filterTxt->setOntop(true);

			auto filterNavLeftArrow = bgFrame->addImage(SDL_Rect{ 0, 0, 30, 44 },
				0xFFFFFFFF, "*#images/ui/Tinkering/Tinker_Button_ArrowL_00.png", "filter nav left arrow");
			filterNavLeftArrow->disabled = true;
			auto filterNavRightArrow = bgFrame->addImage(SDL_Rect{ 0, 0, 30, 44 },
				0xFFFFFFFF, "*#images/ui/Tinkering/Tinker_Button_ArrowR_00.png", "filter nav right arrow");
			filterNavRightArrow->disabled = true;

			auto filterNavLeft = bgFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
				0xFFFFFFFF, "", "filter nav left");
			filterNavLeft->disabled = true;
			auto filterNavRight = bgFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
				0xFFFFFFFF, "", "filter nav right");
			filterNavRight->disabled = true;
		}

		{
			auto heldScrapBg = bgFrame->addImage(SDL_Rect{ 0, 0, 176, 36 },
				0xFFFFFFFF, "*images/ui/Tinkering/Tinker_ScrapBacking_00.png", "held scrap img");

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
			else if ( item->type == ENCHANTED_FEATHER && item->identified )
			{
				snprintf(buf, sizeof(buf), "%s %s (%d%%) (%+d)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), 
					item->getName(), item->appearance % ENCHANTED_FEATHER_MAX_DURABILITY, item->beatitude);
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
				int oldTinkering = stats[player]->getProficiency(PRO_LOCKPICKING);
				stats[player]->setProficiencyUnsafe(PRO_LOCKPICKING, 999);
				Sint32 oldPER = stats[player]->PER;
				stats[player]->PER += -statGetPER(stats[player], players[player]->entity);
				itemRequirement = parentGUI.tinkeringPlayerHasSkillLVLToCraft(item) * 20; // manually hack this to max to get requirement
				if ( itemRequirement == 0 )
				{
					itemRequirement = TINKER_MIN_ITEM_SKILL_REQ;
				}
				stats[player]->PER = oldPER;
				stats[player]->setProficiency(PRO_LOCKPICKING, oldTinkering);
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
			if ( !isTinkeringBot && item->type != TOOL_TINKERING_KIT )
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

		int skillLVL = stats[player]->getModifiedProficiency(PRO_LOCKPICKING) 
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
				int oldTinkering = stats[player]->getProficiency(PRO_LOCKPICKING);
				stats[player]->setProficiencyUnsafe(PRO_LOCKPICKING, 999);
				Sint32 oldPER = stats[player]->PER;
				stats[player]->PER += -statGetPER(stats[player], players[player]->entity);
				itemRequirement = parentGUI.tinkeringPlayerHasSkillLVLToCraft(item) * 20; // manually hack this to max to get requirement
				if ( itemRequirement == 0 )
				{
					itemRequirement = TINKER_MIN_ITEM_SKILL_REQ;
				}
				stats[player]->PER = oldPER;
				stats[player]->setProficiency(PRO_LOCKPICKING, oldTinkering);
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
					if ( craftRequirement == 0 )
					{
						craftRequirement = TINKER_MIN_ITEM_SKILL_REQ;
					}
					else
					{
						craftRequirement *= 20;
					}
					itemRequirement = std::min(100, craftRequirement + diff);
				}
				else
				{
					int oldTinkering = stats[player]->getProficiency(PRO_LOCKPICKING);
					stats[player]->setProficiencyUnsafe(PRO_LOCKPICKING, 999);
					Sint32 oldPER = stats[player]->PER;
					stats[player]->PER += -statGetPER(stats[player], players[player]->entity);
					itemRequirement = parentGUI.tinkeringPlayerHasSkillLVLToCraft(item) * 20; // manually hack this to max to get requirement
					if ( itemRequirement == 0 )
					{
						itemRequirement = TINKER_MIN_ITEM_SKILL_REQ;
					}
					stats[player]->PER = oldPER;
					stats[player]->setProficiency(PRO_LOCKPICKING, oldTinkering);
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

		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
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
			if ( player->bAlignGUINextToInventoryCompact() )
			{
				const int fullWidth = alchFramePos.w + 210; // inventory width 210
				alchFramePos.x = -alchFramePos.w + animx * fullWidth;
			}
			else
			{
				alchFramePos.x = player->camera_virtualWidth() - animx * alchFramePos.w;
			}
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
			alchFramePos.x = player->camera_virtualWidth() - animx * fullWidth;
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
			if ( player->bAlignGUINextToInventoryCompact() )
			{
				const int fullWidth = alchFramePos.w + 210; // inventory width 210
				alchFramePos.x = player->camera_virtualWidth() - animx * fullWidth;
			}
			else
			{
				alchFramePos.x = -alchFramePos.w + animx * alchFramePos.w;
			}
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
	if ( !player->bUseCompactGUIHeight() && !player->bUseCompactGUIWidth() )
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

	/*if ( keystatus[SDLK_j] && enableDebugKeys )
	{
		if ( keystatus[SDLK_LSHIFT] )
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
		keystatus[SDLK_j] = 0;
	}*/
	/*if ( keystatus[SDLK_H] && enableDebugKeys )
	{
		keystatus[SDLK_H] = 0;
		if ( keystatus[SDLK_LSHIFT] )
		{
			for ( int i = 0; i < NUMITEMS; ++i )
			{
				if ( items[i].category == POTION && i != POTION_EMPTY )
				{
					clientLearnedAlchemyIngredients[playernum].insert(i);
				}
			}
		}
		else if ( keystatus[SDLK_LCTRL] )
		{
			consoleCommand("/gimmepotions2");
		}
		else
		{
			consoleCommand("/gimmepotions 5");
		}
	}*/

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
		else
		{
			if ( player->inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_LEFT
				&& player->bAlignGUINextToInventoryCompact() )
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
		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
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
		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
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
		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
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
		messagePlayer(playernum, MESSAGE_MISC, Language::get(4159));
		parentGUI.closeGUI();
		return; // I can't see!
	}

	/*if ( keystatus[SDLK_B] && enableDebugKeys )
	{
		keystatus[SDLK_B] = 0;
		notifications.push_back(std::make_pair(ticks, AlchNotification_t("Wow a title!", "This is a body", "items/images/Alembic.png")));
	}*/
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
			const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
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
			const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
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
			recipeBtn->setText(Language::get(4171));
		}
		else
		{
			recipeBtn->setText(Language::get(4170));
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

	int skillLVL = (stats[playernum]->getModifiedProficiency(PRO_ALCHEMY) + statGetINT(stats[playernum], players[playernum]->entity));
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
				if ( res == POTION_SICKNESS && !samePotion )
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
		&& (keystatus[SDLK_LSHIFT] || keystatus[SDLK_RSHIFT]) )
	{
		modifierPressed = true;
	}

	auto brewBtn = baseFrame->findButton("brew button");
	auto brewGlyph = baseFrame->findImage("brew glyph");
	brewBtn->setDisabled(true);
	brewGlyph->disabled = true;
	if ( inputs.getVirtualMouse(playernum)->draw_cursor )
	{
		brewBtn->setText(Language::get(4175));
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
		brewBtn->setText(Language::get(4178));
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
			//const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
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
				if ( str == Language::get(4167) )
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
							if ( toCompare == Language::get(4155) )
							{
								// unknown
								color = hudColors.characterSheetLightNeutral;
							}
							else if ( toCompare == Language::get(4162)
								|| toCompare == Language::get(4164) || toCompare == Language::get(4166) )
							{
								color = makeColor(54, 144, 171, 255);
							}
							else if ( toCompare == Language::get(4156) )
							{
								// base pot
								color = hudColors.characterSheetGreen;
							}
							else if ( toCompare == Language::get(4163) )
							{
								// duplication chance
								color = hudColors.characterSheetGreen;
							}
							else if ( toCompare == Language::get(4157) )
							{
								// secondary pot
								color = hudColors.characterSheetHighlightText;
							}
							else if ( toCompare == Language::get(4158) )
							{
								color = hudColors.characterSheetFaintText;
							}
							else if ( toCompare == Language::get(4160) || toCompare == Language::get(4165) || toCompare == Language::get(4168) )
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
			const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
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
					Player::soundCancel();
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
					Player::soundCancel();
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
					Player::soundCancel();
				}
				else if ( !tryBrew && getSelectedAlchemySlotX() == ALCH_SLOT_SECONDARY_POTION_X )
				{
					potion2Uid = 0;
					animPotion2 = 0.0;
					animPotion2Frame->setDisabled(true);
					recipes.activateRecipeIndex = -1; // clear active recipe
					Player::soundCancel();

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
							Player::soundCancel();
						}
						else if ( potion2Uid == item->uid )
						{
							potion2Uid = 0;
							animPotion2 = 0.0;
							animPotion2Frame->setDisabled(true);
							Player::soundCancel();
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
							playSound(139, 64); // click sound
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
							playSound(139, 64); // click sound
						}
						recipes.activateRecipeIndex = -1;
						animRandomPotionTicks = 0;
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
	frame->setHollow(false);
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
	bgImgFrame->setHollow(false);
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
		clearRecipeBtn->setText(Language::get(4169));
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
			Player::soundCancel();
		});
		clearRecipeBtn->setTickCallback(genericgui_deselect_fn);
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
		notificationFrame->setHollow(false);
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
		bgFrame->setHollow(false);
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
			recipeBtn->setText(Language::get(4170));
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
						Player::soundCancel();
					}
					else
					{
						GenericGUI[player].alchemyGUI.recipes.openRecipePanel();
						Player::soundActivate();
					}
				}
			});
			recipeBtn->setTickCallback(genericgui_deselect_fn);

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
				Player::soundCancel();
			});
			closeBtn->setTickCallback(genericgui_deselect_fn);

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
				Player::soundCancel();
			});
			brewBtn->setTickCallback(genericgui_deselect_fn);

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
			snprintf(buf, sizeof(buf), "%s (?)", Language::get(4161));
		}
		else
		{
			snprintf(buf, sizeof(buf), "%s %s (?)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), item->getName());
		}
	}
	else if ( item->type == TOOL_BOMB && isTooltipForResultPotion )
	{
		snprintf(buf, sizeof(buf), "%s", Language::get(4167));
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
				else if ( basePotion->identified && basePotion->type == POTION_POLYMORPH
					|| secondaryPotion->identified && secondaryPotion->type == POTION_POLYMORPH )
				{
					isRandomResult = true;
				}
				else if ( (basePotion->identified && basePotion->type == POTION_WATER
					&& secondaryPotion->identified && secondaryPotion->type != POTION_WATER)
					|| (basePotion->identified && basePotion->type != POTION_WATER
						&& secondaryPotion->identified && secondaryPotion->type == POTION_WATER) )
				{
					isDuplicationResult = true;
				}
			}
		}
		if ( recipeMissingMaterials )
		{
			strcat(buf, "\n");
			strcat(buf, Language::get(4168));
		}
		else if ( isEquipped )
		{
			strcat(buf, "\n");
			strcat(buf, Language::get(4165));
		}
		else if ( isSameResult )
		{
			strcat(buf, "\n");
			strcat(buf, Language::get(4166));
		}
		else if ( isRandomResult )
		{
			strcat(buf, "\n");
			strcat(buf, Language::get(4164));
		}
		else if ( isDuplicationResult )
		{
			int skillLVL = 0;
			if ( stats[parentGUI.getPlayer()] )
			{
				skillLVL = stats[parentGUI.getPlayer()]->getModifiedProficiency(PRO_ALCHEMY) / 20; // 0 to 5;
			}
			snprintf(buf, sizeof(buf), "%s\n%d%%", Language::get(4163), 50 + skillLVL * 10);
		}
		else if ( item->identified && find != clientLearnedAlchemyIngredients[player].end() )
		{
			if ( parentGUI.isItemBaseIngredient(item->type) )
			{
				strcat(buf, "\n");
				strcat(buf, Language::get(4156));
			}
			else if ( parentGUI.isItemSecondaryIngredient(item->type) )
			{
				strcat(buf, "\n");
				strcat(buf, Language::get(4157));
			}
			else
			{
				strcat(buf, "\n");
				strcat(buf, Language::get(4158));
			}
		}
		else
		{
			if ( !item->identified )
			{
				if ( isTooltipForResultPotion )
				{
					strcat(buf, "\n");
					strcat(buf, Language::get(4162));
				}
				else
				{
					strcat(buf, "\n");
					strcat(buf, Language::get(4160));
				}
			}
			else
			{
				strcat(buf, "\n");
				strcat(buf, Language::get(4155));
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
			activateSelectionPrompt->setText(Language::get(4174));
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
								activateSelectionPrompt->setText(Language::get(4175));
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
				activateSelectionPrompt->setText(Language::get(4174));
			}
		}
		else if ( !isTooltipForResultPotion )
		{
			if ( item->uid == potion1Uid || item->uid == potion2Uid )
			{
				activateSelectionPrompt->setText(Language::get(4173));
			}
			else
			{
				activateSelectionPrompt->setText(Language::get(4172));
			}
		}
		else if ( isTooltipForResultPotion )
		{
			bool usingGamepad = inputs.hasController(player) && !inputs.getVirtualMouse(player)->draw_cursor;
			if ( !usingGamepad )
			{
				if ( isSameResult )
				{
					activateSelectionPrompt->setText(Language::get(4177));
				}
				else if ( isDuplicationResult )
				{
					activateSelectionPrompt->setText(Language::get(4178));
				}
				else
				{
					activateSelectionPrompt->setText(Language::get(4176));
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
		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
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
	snprintf(titleBuf, sizeof(titleBuf), Language::get(4181), recipeList.size());
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
					0xFFFFFFFF, "*images/ui/Alchemy/Alchemy_Icon_RecipeTileBG_00.png", stoneImgName);
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
				auto& input = Input::inputs[player];
				if ( inputs.bPlayerUsingKeyboardControl(player) )
				{
					if ( input.binaryToggle("MenuMouseWheelDown") )
					{
						scrollSetpoint = std::max(scrollSetpoint + slotSize, 0);
					}
					if ( input.binaryToggle("MenuMouseWheelUp") )
					{
						scrollSetpoint = std::max(scrollSetpoint - slotSize, 0);
					}
				}
				if ( input.consumeBinaryToggle("MenuScrollDown") )
				{
					scrollSetpoint = std::max(scrollSetpoint + slotSize, 0);
				}
				else if ( input.consumeBinaryToggle("MenuScrollUp") )
				{
					scrollSetpoint = std::max(scrollSetpoint - slotSize, 0);
				}
			}
		}

		scrollSetpoint = std::min(scrollSetpoint, scrollAmount);
		currentScrollRow = scrollSetpoint / slotSize;

		if ( abs(scrollSetpoint - scrollAnimateX) > 0.00001 )
		{
			isInteractable = false;
			const real_t fpsScale = getFPSScale(60.0);
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

const int GenericGUIMenu::FeatherGUI_t::MAX_FEATHER_X = 1;
const int GenericGUIMenu::FeatherGUI_t::MAX_FEATHER_Y = NUMLABELS - 1;

bool GenericGUIMenu::FeatherGUI_t::scrollSortFunc(const std::pair<std::string, std::pair<int, bool>>& lhs, 
	const std::pair<std::string, std::pair<int, bool>>& rhs)
{
	int lhsVal = lhs.second.second == true ? 1 : 0; // second.second is identified status, convert to int
	int rhsVal = rhs.second.second == true ? 1 : 0; // second.second is identified status, convert to int
	if ( sortType == SORT_SCROLL_UNKNOWN )
	{
		return lhsVal < rhsVal;
	}
	else if ( sortType == SORT_SCROLL_DISCOVERED )
	{
		return lhsVal > rhsVal;
	}
	return lhsVal < rhsVal;
}

void featherChangeChargeEvent(const int player, int chargeAmount, int realCharge)
{
	auto& featherGUI = GenericGUI[player].featherGUI;
	{
		bool addedToCurrentTotal = false;
		bool isAnimatingValue = ((ticks - featherGUI.animChargeStartTicks) > TICKS_PER_SECOND);
		if ( chargeAmount < 0 )
		{
			if ( featherGUI.changeFeatherCharge < 0
				&& !isAnimatingValue
				&& abs(chargeAmount) > 0 )
			{
				addedToCurrentTotal = true;
				if ( realCharge + chargeAmount < 0 )
				{
					featherGUI.changeFeatherCharge -= realCharge;
				}
				else
				{
					featherGUI.changeFeatherCharge += chargeAmount;
				}
			}
			else
			{
				if ( realCharge + chargeAmount < 0 )
				{
					featherGUI.changeFeatherCharge = -realCharge;
				}
				else
				{
					featherGUI.changeFeatherCharge = chargeAmount;
				}
			}
		}
		else
		{
			if ( featherGUI.changeFeatherCharge > 0
				&& !isAnimatingValue
				&& abs(chargeAmount) > 0 )
			{
				addedToCurrentTotal = true;
				featherGUI.changeFeatherCharge += chargeAmount;
			}
			else
			{
				featherGUI.changeFeatherCharge = chargeAmount;
			}
		}
		featherGUI.animChargeStartTicks = ticks;
		featherGUI.animCharge = 0.0;
		featherGUI.animQtyChange = 1.0;
		if ( !addedToCurrentTotal )
		{
			featherGUI.currentFeatherCharge = realCharge;
		}
	}
}

void GenericGUIMenu::FeatherGUI_t::updateFeatherCharge(void* featherChargeText, void* featherChangeChargeText, int currentCharge)
{
	Field* featherChargeField = static_cast<Field*>(featherChargeText);
	Field* featherChangeChargeField = static_cast<Field*>(featherChangeChargeText);

	bool pauseChangeChargeAnim = false;
	if ( changeFeatherCharge != 0 )
	{
		if ( ((ticks - animChargeStartTicks) > TICKS_PER_SECOND) 
			&& (inscribeSuccessTicks == 0 || (ticks - inscribeSuccessTicks) >= 1.5 * TICKS_PER_SECOND) )
		{
			const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.1, (animCharge)) / 10.0;
			animCharge -= setpointDiffX;
			animCharge = std::max(0.0, animCharge);

			if ( animCharge <= 0.0001 )
			{
				changeFeatherCharge = 0;
			}
		}
		else
		{
			pauseChangeChargeAnim = true;

			const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animCharge)) / 10.0;
			animCharge += setpointDiffX;
			animCharge = std::min(1.0, animCharge);
		}
	}

	{
		bool showChangedCharge = false;
		if ( changeFeatherCharge != 0 )
		{
			int displayedChangeCharge = animCharge * changeFeatherCharge;
			if ( pauseChangeChargeAnim )
			{
				displayedChangeCharge = changeFeatherCharge;
			}
			if ( abs(displayedChangeCharge) > 0 )
			{
				showChangedCharge = true;
				featherChangeChargeField->setDisabled(false);
				std::string s = "+";
				if ( changeFeatherCharge < 0 )
				{
					s = "";
				}
				s += std::to_string(displayedChangeCharge);
				featherChangeChargeField->setText(s.c_str());
				int displayedCurrentCharge = currentFeatherCharge
					+ (changeFeatherCharge - displayedChangeCharge);
				char buf[32] = "";
				snprintf(buf, sizeof(buf), "%d%%", displayedCurrentCharge);
				featherChargeField->setText(buf);
			}
		}

		if ( !showChangedCharge )
		{
			int displayedChangeCharge = 0;
			featherChangeChargeField->setDisabled(true);
			featherChangeChargeField->setText(std::to_string(displayedChangeCharge).c_str());
			char buf[32] = "";
			snprintf(buf, sizeof(buf), "%d%%", currentCharge);
			featherChargeField->setText(buf);
		}
	}
}

void GenericGUIMenu::FeatherGUI_t::sortScrolls()
{
	if ( sortedScrolls.empty() )
	{
		scrollListRequiresSorting = true;
	}

	if ( !scrollListRequiresSorting )
	{
		for ( auto& entry : sortedScrolls )
		{
			if ( scrolls.find(entry.first) != scrolls.end() )
			{
				entry.second = scrolls[entry.first];
			}
		}
		return;
	}

	scrollListRequiresSorting = false;
	sortedScrolls.clear();
	for ( int i = 0; i < NUMLABELS; ++i )
	{
		auto find = scrolls.find(scroll_label[i]);
		if ( find != scrolls.end() )
		{
			sortedScrolls.push_back(std::make_pair((*find).first, (*find).second));
		}
	}
	if ( sortType != SORT_SCROLL_DEFAULT )
	{
		std::sort(sortedScrolls.begin(), sortedScrolls.end(), [this](const std::pair<std::string, std::pair<int, bool>>& lhs,
			const std::pair<std::string, std::pair<int, bool>>& rhs) {
			return this->scrollSortFunc(lhs, rhs);
		});
	}
}

void GenericGUIMenu::FeatherGUI_t::changeSortingType(GenericGUIMenu::FeatherGUI_t::SortTypes_t newType)
{
	if ( sortType != newType )
	{
		scrollListRequiresSorting = true;
	}
	sortType = newType;
}

void GenericGUIMenu::FeatherGUI_t::updateScrolls()
{
	scrolls.clear();
	/*if ( keystatus[SDLK_J] && enableDebugKeys )
	{
		keystatus[SDLK_J] = 0;
		if ( keystatus[SDLK_LCTRL] )
		{
			clientLearnedScrollLabels[parentGUI.getPlayer()].clear();
			for ( int i = 0; i < NUMLABELS; ++i )
			{
				clientLearnedScrollLabels[parentGUI.getPlayer()].insert(i);
			}
		}
		else if ( sortType == SORT_SCROLL_DEFAULT )
		{
			changeSortingType(SORT_SCROLL_DISCOVERED);
		}
		else if ( sortType == SORT_SCROLL_DISCOVERED )
		{
			changeSortingType(SORT_SCROLL_UNKNOWN);
		}
		else if ( sortType == SORT_SCROLL_UNKNOWN )
		{
			changeSortingType(SORT_SCROLL_DEFAULT);
		}
	}*/
	for ( node_t* node = parentGUI.scribingTotalItems.first; node; node = node->next )
	{
		if ( node->list == &stats[parentGUI.getPlayer()]->inventory )
		{
			break;
		}
		if ( parentGUI.isNodeScribingCraftableItem(node) )
		{
			Item* item = (Item*)node->element;
			if ( item )
			{
				std::string label = item->getScrollLabel();
				assert(scrolls.find(label) == scrolls.end());

				bool identified = false;
				for ( int i = 0; i < NUMLABELS; ++i )
				{
					if ( label == scroll_label[i] )
					{
						for ( auto& s : clientLearnedScrollLabels[parentGUI.getPlayer()] )
						{
							if ( s == i )
							{
								// we know this scroll's index
								identified = true;
								break;
							}
						}
						break;
					}
				}
				scrolls[label] = std::make_pair(item->type, identified);
			}
		}
	}

	sortScrolls();

	int index = 0;
	for ( auto& scroll : sortedScrolls )
	{
		Item* scrollItem = nullptr;
		for ( node_t* node = parentGUI.scribingTotalItems.first; node; node = node->next )
		{
			if ( node->list == &stats[parentGUI.getPlayer()]->inventory )
			{
				break;
			}
			if ( node->element )
			{
				Item* item = (Item*)node->element;
				if ( item && scroll.first == item->getScrollLabel() )
				{
					item->x = 0;
					item->y = index;
					scrollItem = item;
					break;
				}
			}
		}
		if ( auto frame = getFeatherSlotFrame(0, index) )
		{
			auto titleTxt = frame->findField("title");
			titleTxt->setText(scroll.first.c_str());
			titleTxt->setColor(hudColors.characterSheetLightNeutral);
			auto result = FEATHER_ACTION_NONE;
			if ( scrollItem && scroll.second.second )
			{
				result = setItemDisplayNameAndPrice(scrollItem, true);
				if ( result == FEATHER_ACTION_CANT_AFFORD )
				{
					titleTxt->setColor(hudColors.characterSheetFaintText);
				}
			}
			auto bodyTxt = frame->findField("body");
			bodyTxt->setText("???");
			bodyTxt->setColor(hudColors.characterSheetOffWhiteText);
			if ( scroll.second.second )
			{
				std::string scrollShortName = items[scroll.second.first].getIdentifiedName();
				if ( scrollShortName.find(ItemTooltips.adjectives["scroll_prefixes"]["scroll_of"]) != std::string::npos )
				{
					scrollShortName = scrollShortName.substr(ItemTooltips.adjectives["scroll_prefixes"]["scroll_of"].size());
				}
				if ( scrollShortName.find(ItemTooltips.adjectives["scroll_prefixes"]["piece_of"]) != std::string::npos )
				{
					scrollShortName = scrollShortName.substr(ItemTooltips.adjectives["scroll_prefixes"]["piece_of"].size());
				}
				camelCaseString(scrollShortName);
				bodyTxt->setColor(hudColors.characterSheetOffWhiteText);

				if ( labelDiscoveries.find(scroll.first) != labelDiscoveries.end() )
				{
					bool deleted = false;
					auto& discovery = labelDiscoveries[scroll.first];
					if ( discovery.processedOnTick != ticks && ticks % 2 == 0 )
					{
						real_t percent = ((ticks - discovery.startTicks) / (TICKS_PER_SECOND / 10)) + 1;
						percent /= 10.0;
						percent = std::min(percent, 1.0);
						size_t numChars = std::min(size_t(scrollShortName.size() * percent), scrollShortName.size());
						discovery.name = scrollShortName.substr(0, numChars);
						for ( auto sz = numChars; sz < scrollShortName.size(); ++sz )
						{
							discovery.name += 'A' + (rand() % 26);
						}
						if ( percent >= 0.999 )
						{
							deleted = true;
							labelDiscoveries.erase(scroll.first);
						}
						discovery.processedOnTick = ticks;
					}
					if ( !deleted )
					{
						bodyTxt->setText(discovery.name.c_str());
					}
					else
					{
						bodyTxt->setText(scrollShortName.c_str());
					}
				}
				else
				{
					bodyTxt->setText(scrollShortName.c_str());
				}
			}
			if ( result == FEATHER_ACTION_CANT_AFFORD )
			{
				bodyTxt->setColor(hudColors.characterSheetFaintText);
			}
			auto bg = frame->findImage("bg");
			switch ( index % 5 )
			{
				case 0: bg->path = "*#images/ui/Feather/Feather_ListUnselected_00.png"; break;
				case 1: bg->path = "*#images/ui/Feather/Feather_ListUnselected_01.png"; break;
				case 2: bg->path = "*#images/ui/Feather/Feather_ListUnselected_02.png"; break;
				case 3: bg->path = "*#images/ui/Feather/Feather_ListUnselected_03.png"; break;
				case 4: bg->path = "*#images/ui/Feather/Feather_ListUnselected_04.png"; break;
				default: bg->path = "*#images/ui/Feather/Feather_ListUnselected_00.png"; break;
			}
			/*if ( scrollItem && isInteractable && highlightedSlot == index )
			{
				if ( getSelectedFeatherSlotX() == scrollItem->x
					&& getSelectedFeatherSlotY() == scrollItem->y )
				{
					bg->path = "*#images/ui/Feather/Feather_ListSelected_00.png";
				}
			}*/
		}
		++index;
	}
}

void GenericGUIMenu::FeatherGUI_t::openFeatherMenu()
{
	const int playernum = parentGUI.getPlayer();
	auto player = players[playernum];

	if ( featherFrame )
	{
		bool wasDisabled = featherFrame->isDisabled();
		featherFrame->setDisabled(false);
		if ( wasDisabled )
		{
			animx = 0.0;
			animTooltip = 0.0;
			animPrompt = 0.0;
			animFilter = 0.0;
			isInteractable = false;
			bFirstTimeSnapCursor = false;
		}
		if ( getSelectedFeatherSlotX() < 0 || getSelectedFeatherSlotX() >= MAX_FEATHER_X
			|| getSelectedFeatherSlotY() < 0 || getSelectedFeatherSlotY() >= MAX_FEATHER_Y )
		{
			selectFeatherSlot(0, 0);
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
	scrolls.clear();
	sortedScrolls.clear();
	scrollListRequiresSorting = true;
}

void GenericGUIMenu::FeatherGUI_t::closeFeatherMenu()
{
	const int playernum = parentGUI.getPlayer();
	auto& player = *players[playernum];

	if ( featherFrame )
	{
		featherFrame->setDisabled(true);
	}
	animx = 0.0;
	animTooltip = 0.0;
	animPrompt = 0.0;
	animFilter = 0.0;
	animInvalidAction = 0.0;
	animInvalidActionTicks = 0;
	animCharge = 0.0;
	animChargeStartTicks = 0;
	animQtyChange = 0.0;
	animPromptTicks = 0;
	animDrawer = 0.0;
	invalidActionType = INVALID_ACTION_NONE;
	isInteractable = false;
	bool wasOpen = bOpen;
	bOpen = false;
	bFirstTimeSnapCursor = false;
	highlightedSlot = -1;
	bDrawerOpen = false;
	labelDiscoveries.clear();
	inscribeSuccessTicks = 0;
	inscribeSuccessName = "";
	if ( wasOpen )
	{
		if ( inputs.getUIInteraction(playernum)->selectedItem )
		{
			inputs.getUIInteraction(playernum)->selectedItem = nullptr;
			inputs.getUIInteraction(playernum)->toggleclick = false;
		}
		inputs.getUIInteraction(playernum)->selectedItemFromChest = 0;
	}
	if ( players[playernum]->GUI.activeModule == Player::GUI_t::MODULE_FEATHER
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
	if ( featherFrame )
	{
		for ( auto f : featherFrame->getFrames() )
		{
			f->removeSelf();
		}
		featherSlotFrames.clear();
	}
	scrolls.clear();
	sortedScrolls.clear();
	scrollListRequiresSorting = true;
}

const int featherBaseWidth = 334;
const int featherBaseHeight = 358;
const int featherDrawerWidth = 214;
int GenericGUIMenu::FeatherGUI_t::heightOffsetWhenNotCompact = 150;

void onFeatherChangeTabAction(const int playernum, bool changingToNewTab)
{
	auto& featherGUI = GenericGUI[playernum].featherGUI;
	featherGUI.isInteractable = false;
	featherGUI.bFirstTimeSnapCursor = false;

	if ( !featherGUI.isInscriptionDrawerOpen() )
	{
		if ( players[playernum]->GUI.activeModule == Player::GUI_t::MODULE_FEATHER )
		{
			// reset to inventory mode if still hanging in feather GUI
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
		players[playernum]->GUI.activateModule(Player::GUI_t::MODULE_FEATHER);
		if ( !inputs.getVirtualMouse(playernum)->draw_cursor )
		{
			players[playernum]->GUI.warpControllerToModule(false);
		}
	}
	if ( changingToNewTab )
	{
		featherGUI.bDrawerOpen = false;
		featherGUI.clearItemDisplayed();
		featherGUI.itemRequiresTitleReflow = true;
		featherGUI.animPrompt = 1.0;
		featherGUI.animPromptTicks = ticks;
	}
}

void buttonFeatherUpdateSelectorOnHighlight(const int player, Button* button)
{
	if ( button->isHighlighted() )
	{
		players[player]->GUI.setHoveringOverModuleButton(Player::GUI_t::MODULE_FEATHER);
		if ( players[player]->GUI.activeModule != Player::GUI_t::MODULE_FEATHER )
		{
			players[player]->GUI.activateModule(Player::GUI_t::MODULE_FEATHER);
		}
		SDL_Rect pos = button->getAbsoluteSize();
		// make sure to adjust absolute size to camera viewport
		pos.x -= players[player]->camera_virtualx1();
		pos.y -= players[player]->camera_virtualy1();
		players[player]->hud.setCursorDisabled(false);
		players[player]->hud.updateCursorAnimation(pos.x - 1, pos.y - 1, pos.w, pos.h, inputs.getVirtualMouse(player)->draw_cursor);
	}
}

void sliderFeatherUpdateSelectorOnHighlight(const int player, Slider* slider)
{
	if ( slider->isHighlighted() )
	{
		players[player]->GUI.setHoveringOverModuleButton(Player::GUI_t::MODULE_FEATHER);
		if ( players[player]->GUI.activeModule != Player::GUI_t::MODULE_FEATHER )
		{
			players[player]->GUI.activateModule(Player::GUI_t::MODULE_FEATHER);
		}
		SDL_Rect pos = slider->getAbsoluteSize();
		// make sure to adjust absolute size to camera viewport
		pos.x -= players[player]->camera_virtualx1();
		pos.y -= players[player]->camera_virtualy1();
		players[player]->hud.setCursorDisabled(false);
		players[player]->hud.updateCursorAnimation(pos.x - 1, pos.y - 1, pos.w, pos.h, inputs.getVirtualMouse(player)->draw_cursor);
	}
}

void GenericGUIMenu::FeatherGUI_t::updateFeatherMenu()
{
	const int playernum = parentGUI.getPlayer();
	auto player = players[playernum];

	if ( !player->isLocalPlayer() )
	{
		closeFeatherMenu();
		return;
	}

	if ( !featherFrame )
	{
		return;
	}

	featherFrame->setSize(SDL_Rect{ players[playernum]->camera_virtualx1(),
		players[playernum]->camera_virtualy1(),
		featherBaseWidth,
		players[playernum]->camera_virtualHeight() });

	bool bFeatherDrawerOpen = isInscriptionDrawerOpen();
	if ( !bFeatherDrawerOpen )
	{
		drawerJustifyInverted = false;
	}

	if ( !featherFrame->isDisabled() && bOpen )
	{
		if ( !featherGUIHasBeenCreated() )
		{
			createFeatherMenu();
		}

		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
		real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animx)) / 2.0;
		animx += setpointDiffX;
		animx = std::min(1.0, animx);
		bool mainPanelReady = false;
		if ( animx >= .9999 )
		{
			if ( !bFeatherDrawerOpen )
			{
				isInteractable = true;
				bFirstTimeSnapCursor = false;
			}
			mainPanelReady = true;
		}

		if ( bFeatherDrawerOpen && mainPanelReady )
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
						&& player->GUI.activeModule == Player::GUI_t::MODULE_FEATHER )
					{
						warpMouseToSelectedFeatherItem(nullptr, (Inputs::SET_CONTROLLER));
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

	bool usingScribingMenu = parentGUI.scribingFilter == GenericGUIMenu::SCRIBING_FILTER_CRAFTABLE
		&& bDrawerOpen;
	if ( !usingScribingMenu )
	{
		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
		real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animFilter)) / 2.0;
		animFilter += setpointDiffX;
		animFilter = std::min(1.0, animFilter);
	}
	else
	{
		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
		real_t setpointDiffX = fpsScale * std::max(.01, (animFilter)) / 2.0;
		animFilter -= setpointDiffX;
		animFilter = std::max(0.0, animFilter);
	}

	bool reversed = false;
	auto featherFramePos = featherFrame->getSize();
	if ( player->inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_LEFT )
	{
		if ( !player->inventoryUI.bCompactView )
		{
			const int fullWidth = featherFramePos.w + 210/* + (40 * (1.0 - animFilter))*/; // inventory width 210
			featherFramePos.x = -featherFramePos.w + animx * fullWidth;
			if ( player->bUseCompactGUIWidth() )
			{
				if ( player->inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				featherFramePos.x -= player->inventoryUI.slideOutWidth * player->inventoryUI.slideOutPercent;
			}
		}
		else
		{
			if ( player->bAlignGUINextToInventoryCompact() )
			{
				const int fullWidth = featherFramePos.w + 210/* + (40 * (1.0 - animFilter))*/; // inventory width 210
				featherFramePos.x = -featherFramePos.w + animx * fullWidth;
			}
			else
			{
				featherFramePos.x = player->camera_virtualWidth() - animx * featherFramePos.w;
			}
			if ( player->bUseCompactGUIWidth() )
			{
				if ( player->inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				featherFramePos.x -= -player->inventoryUI.slideOutWidth * player->inventoryUI.slideOutPercent;
			}
		}
	}
	else if ( player->inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_RIGHT )
	{
		reversed = true;
		if ( !player->inventoryUI.bCompactView )
		{
			const int fullWidth = featherFramePos.w + 210 /*+ (40 * (1.0 - animFilter))*/; // inventory width 210
			featherFramePos.x = player->camera_virtualWidth() - animx * fullWidth;
			if ( player->bUseCompactGUIWidth() )
			{
				if ( player->inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				featherFramePos.x -= -player->inventoryUI.slideOutWidth * player->inventoryUI.slideOutPercent;
			}
		}
		else
		{
			if ( player->bAlignGUINextToInventoryCompact() )
			{
				const int fullWidth = featherFramePos.w + 210 /*+ (40 * (1.0 - animFilter))*/; // inventory width 210
				featherFramePos.x = player->camera_virtualWidth() - animx * fullWidth;
			}
			else
			{
				featherFramePos.x = -featherFramePos.w + animx * featherFramePos.w;
			}
			if ( player->bUseCompactGUIWidth() )
			{
				if ( player->inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				featherFramePos.x -= player->inventoryUI.slideOutWidth * player->inventoryUI.slideOutPercent;
			}
		}
	}
	drawerJustifyInverted = reversed;

	int heightOffsetCompact = 0;
	if ( !player->bUseCompactGUIHeight() && !player->bUseCompactGUIWidth() )
	{
		featherFramePos.y = heightOffsetWhenNotCompact;
	}
	else
	{
		featherFramePos.y = 0;
		heightOffsetCompact = -40;
	}

	if ( !featherGUIHasBeenCreated() )
	{
		return;
	}

	auto drawerFrame = featherFrame->findFrame("feather drawer");
	drawerFrame->setDisabled(true);
	auto baseFrame = featherFrame->findFrame("feather base");
	baseFrame->setDisabled(false);

	featherFrame->setSize(featherFramePos);

	SDL_Rect baseFramePos = baseFrame->getSize();
	baseFramePos.x = 0;
	baseFramePos.w = featherBaseWidth;
	baseFrame->setSize(baseFramePos);

	auto baseBg = baseFrame->findImage("feather base img");
	baseBg->pos.y = heightOffsetCompact;

	{
		drawerFrame->setDisabled(!bFeatherDrawerOpen);
		SDL_Rect drawerFramePos = drawerFrame->getSize();
		const int widthDifference = animDrawer * (drawerFramePos.w - (drawerJustifyInverted ? 0 : 6)/* - 8*/);
		if ( drawerJustifyInverted )
		{
			drawerFramePos.x = baseFramePos.x + baseFramePos.w + animDrawer * (drawerFramePos.w - 4) - (drawerFramePos.w);
		}
		else
		{
			drawerFramePos.x = 0;
		}

		if ( !player->bUseCompactGUIHeight() && !player->bUseCompactGUIWidth() )
		{
			drawerFramePos.y = 54;
		}
		else
		{
			drawerFramePos.y = 14;
		}
		drawerFrame->setSize(drawerFramePos);

		if ( !drawerJustifyInverted )
		{
			featherFramePos.x -= widthDifference;
		}
		int adjustx = 0;
		if ( featherFramePos.x < 0 )
		{
			adjustx = -featherFramePos.x; // to not slide off-frame
			featherFramePos.x += adjustx;
		}
		featherFramePos.w += (widthDifference);
		featherFramePos.h = std::max(drawerFramePos.y + drawerFramePos.h, baseFramePos.y + baseFramePos.h);
		featherFrame->setSize(featherFramePos);
		if ( !drawerJustifyInverted )
		{
			baseFramePos.x = featherFramePos.w - baseFramePos.w;
		}
		baseFrame->setSize(baseFramePos);
	}

	if ( !bOpen )
	{
		return;
	}

	if ( !parentGUI.isGUIOpen()
		|| parentGUI.guiType != GUICurrentType::GUI_TYPE_SCRIBING
		|| !stats[playernum]
		|| stats[playernum]->HP <= 0
		|| !player->entity
		|| player->shootmode )
	{
		closeFeatherMenu();
		return;
	}

	if ( player->entity && player->entity->isBlind() )
	{
		messagePlayer(playernum, MESSAGE_MISC, Language::get(4159));
		parentGUI.closeGUI();
		return; // I can't see!
	}

	// feather status
	{
		auto featherTitle = baseFrame->findField("feather title");
		auto featherStatus = baseFrame->findField("feather status");
		if ( auto item = parentGUI.scribingToolFindInInventory() )
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
				featherTitle->setText(titleStr.c_str());
			}
			else
			{
				featherTitle->setText(buf);
			}
			featherStatus->setText(ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str());
			if ( item->status <= DECREPIT )
			{
				featherStatus->setTextColor(hudColors.characterSheetRed);
			}
			else
			{
				featherStatus->setTextColor(hudColors.characterSheetLightNeutral);
			}
		}
		else
		{
			featherTitle->setText("");
			featherStatus->setText("");
		}

		SDL_Rect textPos{ 0, 67 + heightOffsetCompact, baseFrame->getSize().w, 24 };
		featherTitle->setSize(textPos);
		textPos.y += 20;
		featherStatus->setSize(textPos);

		auto skillIcon = baseFrame->findImage("feather skill img");
		skillIcon->pos.y = 76 + heightOffsetCompact;
		for ( auto& skill : Player::SkillSheet_t::skillSheetData.skillEntries )
		{
			if ( skill.skillId == PRO_MAGIC )
			{
				if ( skillCapstoneUnlocked(playernum, skill.skillId) )
				{
					skillIcon->path = skill.skillIconPathLegend;
				}
				else
				{
					skillIcon->path = skill.skillIconPath;
				}
				skillIcon->disabled = false;
				break;
			}
		}
	}

	bool itemActionOK = itemActionType == FEATHER_ACTION_OK 
		|| itemActionType == FEATHER_ACTION_MAY_SUCCEED
		|| itemActionType == FEATHER_ACTION_OK_AND_DESTROY
		|| itemActionType == FEATHER_ACTION_OK_UNKNOWN_SCROLL;
	if ( itemActionOK )
	{
		animInvalidAction = 0.0;
		animInvalidActionTicks = 0;
	}
	else
	{
		// shaking feedback for invalid action
		// constant decay for animation
		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
		real_t setpointDiffX = fpsScale * 1.0 / 25.0;
		animInvalidAction -= setpointDiffX;
		animInvalidAction = std::max(0.0, animInvalidAction);
	}
	bool bInvalidActionAnimating = false;
	if ( animInvalidAction > 0.001 || (ticks - animInvalidActionTicks) < TICKS_PER_SECOND * .8 )
	{
		bInvalidActionAnimating = true;
	}

	{
		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
		real_t setpointDiffX = fpsScale * std::max(.1, (animQtyChange)) / 10.0;
		animQtyChange -= setpointDiffX;
		animQtyChange = std::max(0.0, animQtyChange);
	}

	// held qtys
	int currentCharge = 0;
	if ( parentGUI.scribingToolItem )
	{
		currentCharge = parentGUI.scribingToolItem->appearance % ENCHANTED_FEATHER_MAX_DURABILITY;
	}
	auto currentChargeText = baseFrame->findField("current charge txt");
	auto changeChargeText = baseFrame->findField("change charge txt");
	{
		auto currentChargeLabel = baseFrame->findField("current charge label");
		currentChargeLabel->setDisabled(false);

		auto currentChargeBg = baseFrame->findImage("current charge img");
		currentChargeBg->pos.x = baseFrame->getSize().w - 32 - currentChargeBg->pos.w;
		currentChargeBg->pos.y = baseFrame->getSize().h - 48 - 38 + heightOffsetCompact;

		SDL_Rect currentChargeTextPos{ currentChargeBg->pos.x, currentChargeBg->pos.y + 9, 112, 24 };
		currentChargeText->setSize(currentChargeTextPos);
		currentChargeText->setColor(hudColors.characterSheetLightNeutral);

		changeChargeText->setText("");
		SDL_Rect changeChargeTextPos = currentChargeTextPos;
		changeChargeTextPos.x -= 44;
		changeChargeText->setSize(changeChargeTextPos);

		updateFeatherCharge(currentChargeText, changeChargeText, currentCharge);

		SDL_Rect currentChargeTxtPos = currentChargeLabel->getSize();
		currentChargeTxtPos.w = currentChargeBg->pos.x + 4;
		currentChargeTxtPos.x = 0;
		currentChargeTxtPos.y = currentChargeTextPos.y;
		currentChargeTxtPos.h = 24;
		currentChargeLabel->setSize(currentChargeTxtPos);
		currentChargeLabel->setText(Language::get(4192));
		if ( invalidActionType == INVALID_ACTION_NO_CHARGE )
		{
			currentChargeTextPos.x += -2 + 2 * (cos(animInvalidAction * 4 * PI));
			currentChargeText->setSize(currentChargeTextPos);
			if ( bInvalidActionAnimating )
			{
				currentChargeText->setColor(hudColors.characterSheetRed); // red
			}
		}
	}

	bool usingGamepad = inputs.hasController(playernum) && !inputs.getVirtualMouse(playernum)->draw_cursor;
	auto bgImg = drawerFrame->findImage("feather drawer img");
	if ( usingGamepad )
	{
		bgImg->path = "*images/ui/Feather/Feather_Drawer_00.png";
	}
	else
	{
		bgImg->path = "*images/ui/Feather/Feather_Drawer_TallScroll_00.png";
	}

	int filterLeftSideX = 0;
	int filterStartY = 0;
	int filterRightSideX = 0;
	{
		// filters
		Button* filterBtn = baseFrame->findButton("filter inscribe btn");
		filterBtn->setDisabled(true);
		if ( inputs.getVirtualMouse(playernum)->draw_cursor )
		{
			filterBtn->setDisabled(!isInteractable);
			if ( isInteractable )
			{
				buttonFeatherUpdateSelectorOnHighlight(playernum, filterBtn);
			}
		}
		else if ( filterBtn->isSelected() )
		{
			filterBtn->deselect();
		}

		Uint32 inactiveColor = hudColors.characterSheetDarker1Neutral;
		Uint32 activeColor = hudColors.characterSheetOffWhiteText;
		Uint32 highlightColor = hudColors.characterSheetLighter1Neutral;

		filterBtn->setColor(makeColor(255, 255, 255, 255));
		filterBtn->setBackground("*#images/ui/Feather/Feather_Tab_Inscribe_Unselected_01.png");
		Field* filterTxt = baseFrame->findField("filter inscribe txt");
		filterTxt->setDisabled(false);
		filterTxt->setText(Language::get(3718));
		filterTxt->setColor(inactiveColor);
		if ( false && filterBtn->isHighlighted() )
		{
			filterTxt->setColor(highlightColor);
		}
		else if ( parentGUI.scribingFilter == GenericGUIMenu::SCRIBING_FILTER_CRAFTABLE )
		{
			filterTxt->setColor(activeColor);
		}
		{
			SDL_Rect btnPos{ 38, 314 + heightOffsetCompact, 98, 36 };
			filterBtn->setSize(btnPos);
			SDL_Rect txtPos = btnPos;
			txtPos.y += 5;
			txtPos.h = 24;
			filterTxt->setSize(txtPos);
			filterLeftSideX = btnPos.x;
			filterStartY = btnPos.y;
		}
		if ( parentGUI.scribingFilter == GenericGUIMenu::SCRIBING_FILTER_CRAFTABLE )
		{
			filterBtn->setColor(makeColor(255, 255, 255, 255));
			filterBtn->setBackground("*#images/ui/Feather/Feather_Tab_Inscribe_Selected_01.png");
			SDL_Rect txtPos = filterTxt->getSize();
			txtPos.y += 2;
			filterTxt->setSize(txtPos);
		}
		filterBtn->setHighlightColor(filterBtn->getColor());

		filterBtn = baseFrame->findButton("filter repair btn");
		filterBtn->setDisabled(true);
		if ( inputs.getVirtualMouse(playernum)->draw_cursor )
		{
			filterBtn->setDisabled(!isInteractable);
			if ( isInteractable )
			{
				buttonFeatherUpdateSelectorOnHighlight(playernum, filterBtn);
			}
		}
		else if ( filterBtn->isSelected() )
		{
			filterBtn->deselect();
		}
		filterBtn->setColor(makeColor(255, 255, 255, 255));
		filterBtn->setBackground("*#images/ui/Feather/Feather_Tab_Repair_Unselected_01.png");
		filterTxt = baseFrame->findField("filter repair txt");
		filterTxt->setDisabled(false);
		filterTxt->setText(Language::get(3719));
		filterTxt->setColor(inactiveColor);
		if ( false && filterBtn->isHighlighted() )
		{
			filterTxt->setColor(highlightColor);
		}
		else if ( parentGUI.scribingFilter == GenericGUIMenu::SCRIBING_FILTER_REPAIRABLE )
		{
			filterTxt->setColor(activeColor);
		}
		{
			SDL_Rect btnPos{ 198, 314 + heightOffsetCompact, 90, 36 };
			filterBtn->setSize(btnPos);
			SDL_Rect txtPos = btnPos;
			txtPos.y += 5;
			txtPos.h = 24;
			filterTxt->setSize(txtPos);
			filterRightSideX = btnPos.x + btnPos.w;
		}
		if ( parentGUI.scribingFilter == GenericGUIMenu::SCRIBING_FILTER_REPAIRABLE )
		{
			filterBtn->setColor(makeColor(255, 255, 255, 255));
			filterBtn->setBackground("*#images/ui/Feather/Feather_Tab_Repair_Selected_01.png");
			SDL_Rect txtPos = filterTxt->getSize();
			txtPos.y += 2;
			filterTxt->setSize(txtPos);
		}
		filterBtn->setHighlightColor(filterBtn->getColor());

		// close btn
		auto closeBtn = baseFrame->findButton("close feather button");
		SDL_Rect closeBtnPos = closeBtn->getSize();
		closeBtnPos.y = 54 + heightOffsetCompact;
		closeBtn->setSize(closeBtnPos);
		auto closeGlyph = baseFrame->findImage("close feather glyph");
		closeBtn->setDisabled(true);
		closeBtn->setInvisible(false);
		if ( bDrawerOpen && usingGamepad )
		{
			//closeBtn->setInvisible(true);
		}
		closeGlyph->disabled = true;
		if ( inputs.getVirtualMouse(playernum)->draw_cursor )
		{
			closeBtn->setDisabled(!isInteractable);
			if ( isInteractable )
			{
				buttonFeatherUpdateSelectorOnHighlight(playernum, closeBtn);
			}
		}
		else if ( closeBtn->isSelected() )
		{
			closeBtn->deselect();
		}
		if ( closeBtn->isDisabled() && usingGamepad && !bDrawerOpen )
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

	auto itemDisplayTooltip = baseFrame->findFrame("feather display tooltip");
	itemDisplayTooltip->setDisabled(false);

	auto actionPromptTxt = baseFrame->findField("action prompt txt");
	actionPromptTxt->setDisabled(false);
	auto actionPromptImg = baseFrame->findImage("action prompt glyph");
	auto actionModifierImg = baseFrame->findImage("action modifier glyph");

	Uint32 negativeColor = hudColors.characterSheetRed;
	Uint32 neutralColor = hudColors.characterSheetLightNeutral;
	Uint32 positiveColor = hudColors.characterSheetGreen;
	Uint32 defaultPromptColor = makeColor(255, 255, 255, 255);

	auto displayItemName = itemDisplayTooltip->findField("item display name");
	auto displayItemTextImg = itemDisplayTooltip->findImage("item text img");
	auto itemSlotBg = itemDisplayTooltip->findImage("item bg img");
	auto minChargeText = itemDisplayTooltip->findField("item min value");
	auto maxChargeText = itemDisplayTooltip->findField("item max value");
	itemSlotBg->pos.x = 12;
	itemSlotBg->pos.y = 12;
	const int displayItemTextImgBaseX = itemSlotBg->pos.x + itemSlotBg->pos.w;
	displayItemTextImg->pos.x = displayItemTextImgBaseX;
	displayItemTextImg->pos.y = itemSlotBg->pos.y + itemSlotBg->pos.h / 2 - displayItemTextImg->pos.h / 2;
	SDL_Rect displayItemNamePos{ displayItemTextImg->pos.x + 6, displayItemTextImg->pos.y - 4, 208, 24 };
	displayItemNamePos.h = 50;
	displayItemName->setSize(displayItemNamePos);
	static ConsoleVariable<int> cvar_featherPromptY("/feather_action_prompt_y", -2);
	SDL_Rect actionPromptTxtPos{ 26, 251 + *cvar_featherPromptY + heightOffsetCompact, baseFrame->getSize().w - (26 * 2), 24 };
	actionPromptTxt->setSize(actionPromptTxtPos);

	SDL_Rect tooltipPos = itemDisplayTooltip->getSize();
	tooltipPos.w = 298;
	tooltipPos.h = baseFrame->getSize().h - 100;
	tooltipPos.y = 138 + heightOffsetCompact;
	tooltipPos.x = 18 - (tooltipPos.w + 18) * (0.0/*1.0 - animTooltip*/);
	itemDisplayTooltip->setSize(tooltipPos);

	updateScrolls();

	auto costBg = itemDisplayTooltip->findImage("item cost img");
	auto costLabel = itemDisplayTooltip->findField("item cost label");
	{
		costBg->pos.x = displayItemTextImgBaseX + displayItemTextImg->pos.w - costBg->pos.w;
		costBg->pos.y = displayItemTextImg->pos.y + displayItemTextImg->pos.h + 4;
		SDL_Rect minChargePos{ costBg->pos.x + 28, costBg->pos.y + 9, 66, 24 };
		SDL_Rect maxChargePos{ costBg->pos.x + 84, costBg->pos.y + 9, 50, 24 };
		minChargeText->setSize(minChargePos);
		maxChargeText->setSize(maxChargePos);

		SDL_Rect costLabelTxtPos = costLabel->getSize();
		costLabelTxtPos.w = costBg->pos.x - 4;
		costLabelTxtPos.x = 0;
		costLabelTxtPos.y = minChargePos.y;
		costLabelTxtPos.h = 24;
		costLabel->setSize(costLabelTxtPos);
	}

	auto itemIncrementText = baseFrame->findField("item increment txt");
	{
		if ( animQtyChange > 0.01 )
		{
			itemIncrementText->setDisabled(false);
			auto pos = itemIncrementText->getSize();
			pos.y = tooltipPos.y + itemSlotBg->pos.y + 16 - ((1.0 - animQtyChange) * 32);
			itemIncrementText->setSize(pos);
			SDL_Color color;
			getColor(itemIncrementText->getColor(), &color.r, &color.g, &color.b, &color.a);
			if ( animQtyChange < .2 )
			{
				itemIncrementText->setColor(makeColor(color.r, color.g, color.b, 255 * (animQtyChange / .2)));
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
		}
	}

	auto itemSlotFrame = itemDisplayTooltip->findFrame("item slot frame");
	bool modifierPressed = false;
	if ( usingGamepad && Input::inputs[playernum].binary("MenuPageLeftAlt") )
	{
		modifierPressed = true;
	}
	else if ( inputs.bPlayerUsingKeyboardControl(playernum)
		&& (keystatus[SDLK_LSHIFT] || keystatus[SDLK_RSHIFT]) )
	{
		modifierPressed = true;
	}

	real_t inscribeSuccessFeedbackPercent = 0.0;
	bool inscribeSuccessFeedbackActive = false;

	if ( itemActionType != FEATHER_ACTION_NONE && itemDesc.size() > 1 )
	{
		if ( isInteractable )
		{
			//const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
			//real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animTooltip)) / 2.0;
			//animTooltip += setpointDiffX;
			//animTooltip = std::min(1.0, animTooltip);
			animTooltip = 1.0;
			animTooltipTicks = ticks;
		}

		itemDisplayTooltip->setDisabled(false);

		{
			// prompt + glyph
			actionPromptTxt->setDisabled(false);
			actionPromptTxt->setHJustify(Field::justify_t::RIGHT);
			if ( inscribeSuccessName != "" && inscribeSuccessTicks > 0 && ((ticks - inscribeSuccessTicks) < 2.5 * TICKS_PER_SECOND) )
			{
				//actionPromptTxt->setHJustify(Field::justify_t::LEFT);
				inscribeSuccessFeedbackActive = true;
				inscribeSuccessFeedbackPercent = 1.0;
				auto tickDiff = ticks - inscribeSuccessTicks;
				if ( (tickDiff) < (TICKS_PER_SECOND / 4) )
				{
					inscribeSuccessFeedbackPercent = 1.0 - ((TICKS_PER_SECOND / 4) - tickDiff) / (real_t)(TICKS_PER_SECOND / 4);
				}
				else if ( tickDiff > (2 * TICKS_PER_SECOND) )
				{
					inscribeSuccessFeedbackPercent = 1.0 - (tickDiff - (2 * TICKS_PER_SECOND)) / (real_t)(TICKS_PER_SECOND / 2);
				}
				char buf[128] = "";
				int index = 0;
				for ( auto& scroll : sortedScrolls )
				{
					if ( scroll.first == inscribeSuccessName )
					{
						if ( auto frame = getFeatherSlotFrame(0, index) )
						{
							auto bodyTxt = frame->findField("body");
							snprintf(buf, sizeof(buf), Language::get(4191), bodyTxt->getText());
						}
						break;
					}
					++index;
				}
				actionPromptTxt->setText(buf);
				actionPromptTxt->setColor(makeColor(236, 175, 28, 255));
				actionPromptImg->disabled = true;
				actionModifierImg->disabled = true;
			}
			else if ( itemActionOK && isInteractable )
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
						actionModifierImg->path = GlyphHelper.getGlyphPath(SDLK_LSHIFT, false);
					}
				}
				if ( auto imgGet = Image::get(actionPromptImg->path.c_str()) )
				{
					actionPromptImg->pos.w = imgGet->getWidth();
					actionPromptImg->pos.h = imgGet->getHeight();
					actionPromptImg->disabled = false;
					/*if ( modifierPressed && parentGUI.scribingFilter == SCRIBING_FILTER_CRAFTABLE )
					{
						if ( auto imgGet2 = Image::get(actionModifierImg->path.c_str()) )
						{
							actionModifierImg->pos.w = imgGet2->getWidth();
							actionModifierImg->pos.h = imgGet2->getHeight();
							actionModifierImg->disabled = false;
						}
					}
					else*/
					{
						actionModifierImg->disabled = true;
					}
				}
				if ( parentGUI.scribingFilter == GenericGUIMenu::SCRIBING_FILTER_CRAFTABLE )
				{
					if ( !parentGUI.scribingBlankScrollTarget )
					{
						actionPromptTxt->setText(Language::get(4185));
					}
					else
					{
						if ( currentHoveringInscriptionLabel == "" )
						{
							actionPromptTxt->setText("");
							actionPromptImg->disabled = true;
							actionModifierImg->disabled = true;
						}
						else
						{
							char buf[128];
							if ( itemActionType == FEATHER_ACTION_MAY_SUCCEED )
							{
								snprintf(buf, sizeof(buf), Language::get(4188));
							}
							else if ( itemActionType == FEATHER_ACTION_OK_AND_DESTROY )
							{
								snprintf(buf, sizeof(buf), Language::get(4189));
							}
							else
							{
								snprintf(buf, sizeof(buf), Language::get(4184));
							}
							actionPromptTxt->setText(buf);
						}
					}
				}
				else if ( parentGUI.scribingFilter == GenericGUIMenu::SCRIBING_FILTER_REPAIRABLE )
				{
					actionPromptTxt->setText(Language::get(3646));
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
				if ( !(bDrawerOpen 
					&& parentGUI.scribingFilter == GenericGUIMenu::SCRIBING_FILTER_CRAFTABLE
					&& currentHoveringInscriptionLabel == "")
					&& isInteractable )
				{
					switch ( itemActionType )
					{
						case FEATHER_ACTION_FULLY_REPAIRED:
							actionPromptTxt->setText(Language::get(4136));
							break;
						case FEATHER_ACTION_UNIDENTIFIED:
							actionPromptTxt->setText(Language::get(4153));
							break;
						case FEATHER_ACTION_CANT_AFFORD:
							actionPromptTxt->setText(Language::get(4186));
							break;
						case FEATHER_ACTION_NO_BLANK_SCROLL:
						case FEATHER_ACTION_NO_BLANK_SCROLL_UNKNOWN_HIGHLIGHT:
							actionPromptTxt->setText(Language::get(4190));
							break;
						default:
							actionPromptTxt->setText("-");
							break;
					}
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
			// charge costs
			minChargeText->setColor(neutralColor);
			maxChargeText->setColor(neutralColor);
			maxChargeText->setText("");
			if ( itemActionType == FEATHER_ACTION_NO_BLANK_SCROLL_UNKNOWN_HIGHLIGHT )
			{
				minChargeText->setText("?");
				//maxChargeText->setText("?");
			}
			else if ( (!itemActionOK && itemActionType != FEATHER_ACTION_CANT_AFFORD && itemActionType != FEATHER_ACTION_NO_BLANK_SCROLL)
				|| (chargeCostMin == 0 && chargeCostMax == 0))
			{
				minChargeText->setText("-");
				//maxChargeText->setText("-");
			}
			else if ( itemActionOK && itemActionType == FEATHER_ACTION_OK_UNKNOWN_SCROLL )
			{
				minChargeText->setText("?");
				//maxChargeText->setText("?");
			}
			else if ( itemActionOK || itemActionType == FEATHER_ACTION_CANT_AFFORD
				|| itemActionType == FEATHER_ACTION_NO_BLANK_SCROLL )
			{
				char buf[32];
				if ( chargeCostMin == chargeCostMax )
				{
					snprintf(buf, sizeof(buf), "%d%%", chargeCostMin);
				}
				else
				{
					snprintf(buf, sizeof(buf), "%d-%d%%", chargeCostMin, chargeCostMax);
				}
				minChargeText->setText(buf);
			}
		}
	}
	else
	{
		if ( (!usingGamepad && (ticks - animTooltipTicks > TICKS_PER_SECOND / 3))
			|| (usingGamepad && !bFeatherDrawerOpen)
			|| animTooltip < 0.9999 )
		{
			const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.01, (animTooltip)) / 2.0;
			animTooltip -= setpointDiffX;
			animTooltip = std::max(0.0, animTooltip);
		}
	}

	if ( parentGUI.scribingFilter == SCRIBING_FILTER_CRAFTABLE
		|| parentGUI.scribingFilter == SCRIBING_FILTER_REPAIRABLE )
	{
		costLabel->setText(Language::get(4183));
	}

	auto actionPromptUnselectedTxt = baseFrame->findField("action prompt unselected txt");
	auto actionPromptCoverLeftImg = baseFrame->findImage("action prompt lcover");
	auto actionPromptCoverRightImg = baseFrame->findImage("action prompt rcover");
	actionPromptCoverLeftImg->pos.x = 0;
	actionPromptCoverRightImg->pos.x = baseFrame->getSize().w - actionPromptCoverLeftImg->pos.w;
	actionPromptCoverLeftImg->pos.y = 110 + heightOffsetCompact;
	actionPromptCoverRightImg->pos.y = 110 + heightOffsetCompact;

	{
		actionPromptUnselectedTxt->setDisabled(false);
		actionPromptUnselectedTxt->setColor(makeColor(224, 224, 224, 255));
		if ( parentGUI.scribingFilter == SCRIBING_FILTER_CRAFTABLE )
		{
			if ( bDrawerOpen )
			{
				actionPromptUnselectedTxt->setText(Language::get(4187));
			}
			else
			{
				actionPromptUnselectedTxt->setText(Language::get(3720));
			}
		}
		else if ( parentGUI.scribingFilter == SCRIBING_FILTER_REPAIRABLE )
		{
			actionPromptUnselectedTxt->setText(Language::get(3726));
		}

		{
			SDL_Rect pos{ 26, 113 + heightOffsetCompact, baseFrame->getSize().w - 52, 24 };
			if ( animPromptMoveLeft )
			{
				pos.x -= actionPromptUnselectedTxt->getSize().w * animPrompt;
			}
			else
			{
				pos.x += actionPromptUnselectedTxt->getSize().w * animPrompt;
			}
			actionPromptUnselectedTxt->setSize(pos);
		}

		if ( ticks - animPromptTicks > TICKS_PER_SECOND / 10 )
		{
			const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
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
		if ( inscribeSuccessFeedbackActive )
		{
			color.a *= inscribeSuccessFeedbackPercent;
		}
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
		getColor(minChargeText->getColor(), &color.r, &color.g, &color.b, &color.a);
		color.a = (Uint8)(255 * animTooltip);
		minChargeText->setColor(makeColor(color.r, color.g, color.b, color.a));
	}
	{
		SDL_Color color;
		getColor(maxChargeText->getColor(), &color.r, &color.g, &color.b, &color.a);
		color.a = (Uint8)(255 * animTooltip);
		maxChargeText->setColor(makeColor(color.r, color.g, color.b, color.a));
	}

	//itemDisplayTooltip->setOpacity(100.0 * animTooltip);
	itemSlotFrame->setOpacity(100.0 * animTooltip);

	auto filterNavLeft = baseFrame->findImage("filter nav left");
	filterNavLeft->disabled = true;
	auto filterNavRight = baseFrame->findImage("filter nav right");
	filterNavRight->disabled = true;
	auto filterNavLeftArrow = baseFrame->findImage("filter nav left arrow");
	filterNavLeftArrow->disabled = true;
	auto filterNavRightArrow = baseFrame->findImage("filter nav right arrow");
	filterNavRightArrow->disabled = true;

	static ConsoleVariable<int> cvar_featherNavGlyphX("/feather_glyph_nav_x", 8);
	static ConsoleVariable<int> cvar_featherNavGlyphY("/feather_glyph_nav_y", 30);

	if ( usingGamepad )
	{
		filterNavLeft->path = Input::inputs[playernum].getGlyphPathForBinding("MenuPageLeft");
		if ( auto imgGet = Image::get(filterNavLeft->path.c_str()) )
		{
			filterNavLeft->pos.w = imgGet->getWidth();
			filterNavLeft->pos.h = imgGet->getHeight();
			filterNavLeft->disabled = false;

			filterNavLeftArrow->disabled = false;
			filterNavLeftArrow->pos.x = 6;
			filterNavLeftArrow->pos.y = filterStartY - 6;

			filterNavLeft->pos.x = filterNavLeftArrow->pos.x - 10 + *cvar_featherNavGlyphX;
			filterNavLeft->pos.y = filterNavLeftArrow->pos.y + filterNavLeftArrow->pos.h - 2;
			filterNavLeft->pos.y -= filterNavLeft->pos.h;
			filterNavLeft->pos.y -= *cvar_featherNavGlyphY;
		}
		filterNavRight->path = Input::inputs[playernum].getGlyphPathForBinding("MenuPageRight");
		if ( auto imgGet = Image::get(filterNavRight->path.c_str()) )
		{
			filterNavRight->pos.w = imgGet->getWidth();
			filterNavRight->pos.h = imgGet->getHeight();
			filterNavRight->disabled = false;

			filterNavRightArrow->disabled = false;
			filterNavRightArrow->pos.x = filterRightSideX + 2;
			filterNavRightArrow->pos.y = filterStartY - 6;

			filterNavRight->pos.x = filterNavRightArrow->pos.x + filterNavRightArrow->pos.w + 10;
			filterNavRight->pos.x -= filterNavRight->pos.w + *cvar_featherNavGlyphX;
			filterNavRight->pos.y = filterNavRightArrow->pos.y + filterNavRightArrow->pos.h - 2;
			filterNavRight->pos.y -= filterNavRight->pos.h;
			filterNavRight->pos.y -= *cvar_featherNavGlyphY;
		}
	}

	bool activateSelection = false;
	if ( isInteractable )
	{
		if ( !inputs.getUIInteraction(playernum)->selectedItem
			&& !player->GUI.isDropdownActive()
			&& player->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_FEATHER)
			&& player->bControlEnabled && !gamePaused
			&& !player->usingCommand() )
		{
			if ( Input::inputs[playernum].binaryToggle("MenuCancel") )
			{
				Input::inputs[playernum].consumeBinaryToggle("MenuCancel");
				if ( bDrawerOpen )
				{
					parentGUI.scribingBlankScrollTarget = nullptr;
					bDrawerOpen = false;
					onFeatherChangeTabAction(playernum, true);
					Player::soundCancel();
				}
				else
				{
					parentGUI.closeGUI();
					Player::soundCancel();
					return;
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
				else if ( usingGamepad && Input::inputs[playernum].binaryToggle("MenuPageRight") )
				{
					if ( parentGUI.scribingFilter == SCRIBING_FILTER_CRAFTABLE )
					{
						parentGUI.scribingFilter = SCRIBING_FILTER_REPAIRABLE;
						onFeatherChangeTabAction(playernum);
						animPromptMoveLeft = false;
					}
					Input::inputs[playernum].consumeBinaryToggle("MenuPageRight");
					Player::soundModuleNavigation();
				}
				else if ( usingGamepad && Input::inputs[playernum].binaryToggle("MenuPageLeft") )
				{
					if ( parentGUI.scribingFilter == SCRIBING_FILTER_REPAIRABLE )
					{
						parentGUI.scribingFilter = SCRIBING_FILTER_CRAFTABLE;
						onFeatherChangeTabAction(playernum);
						animPromptMoveLeft = true;
					}
					Input::inputs[playernum].consumeBinaryToggle("MenuPageLeft");
					Player::soundModuleNavigation();
				}
				else if ( usingGamepad && Input::inputs[playernum].binaryToggle("MenuAlt2") )
				{
					Input::inputs[playernum].consumeBinaryToggle("MenuAlt2");
					if ( sortType == SortTypes_t::SORT_SCROLL_DEFAULT )
					{
						changeSortingType(SortTypes_t::SORT_SCROLL_DISCOVERED);
					}
					else if ( sortType == SortTypes_t::SORT_SCROLL_DISCOVERED )
					{
						changeSortingType(SortTypes_t::SORT_SCROLL_UNKNOWN);
					}
					else
					{
						changeSortingType(SortTypes_t::SORT_SCROLL_DEFAULT);
					}
					Player::soundActivate();
				}
			}
		}
	}

	if ( activateSelection && players[playernum] && players[playernum]->entity )
	{
		node_t* nextnode = nullptr;
		list_t* player_inventory = &parentGUI.scribingTotalItems;
		bool foundItem = false;
		bool inscribingBlankScroll = false;
		bool repairingSpellbook = false;
		inscribeSuccessTicks = 0;
		inscribeSuccessName = "";
		if ( player_inventory )
		{
			for ( node_t* node = player_inventory->first; node != NULL; node = nextnode )
			{
				nextnode = node->next;
				if ( node->element )
				{
					Item* item = (Item*)node->element;
					if ( isInscriptionDrawerItemSelected(item) )
					{
						foundItem = true;
						if ( itemActionOK && parentGUI.scribingBlankScrollTarget )
						{
							parentGUI.executeOnItemClick(item);
						}
						inscribingBlankScroll = true;
						break;
					}
					else if ( isInscribeOrRepairActive() && isItemSelectedToRepairOrInscribe(item) )
					{
						foundItem = true;
						if ( itemCategory(item) == SPELLBOOK )
						{
							repairingSpellbook = true;
						}
						if ( itemActionOK )
						{
							parentGUI.executeOnItemClick(item);
						}
						break;
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

			if ( !itemActionOK )
			{
				if ( itemActionType == FEATHER_ACTION_CANT_AFFORD )
				{
					invalidActionType = INVALID_ACTION_NO_CHARGE;
				}
				else
				{
					invalidActionType = INVALID_ACTION_SHAKE_PROMPT;
				}
				animInvalidAction = 1.0;
				animInvalidActionTicks = ticks;
				// play bad feedback sfx
				playSound(90, 64);
			}
			else
			{
				if ( inscribingBlankScroll )
				{
					if ( !parentGUI.scribingBlankScrollTarget )
					{
						// immediately fade the tooltip on mouse control
						animTooltipTicks = 0;
					}
				}
				int newCharge = 0;
				if ( parentGUI.scribingToolItem )
				{
					newCharge = parentGUI.scribingToolItem->appearance % ENCHANTED_FEATHER_MAX_DURABILITY;
				}
				int diffCharge = currentCharge - newCharge;
				featherChangeChargeEvent(parentGUI.getPlayer(), -diffCharge, currentCharge);
				if ( diffCharge != 0 )
				{
					if ( inscribingBlankScroll )
					{
						itemIncrementText->setText("-1");
					}
					else if ( repairingSpellbook )
					{
						itemIncrementText->setText("  +");
					}
				}
			}
		}
	}

	{
		auto sortTxt = drawerFrame->findField("sort txt");
		auto sortBtn = drawerFrame->findButton("sort btn");
		auto sortGlyph = drawerFrame->findImage("sort glyph");
		if ( sortType == SortTypes_t::SORT_SCROLL_DEFAULT )
		{
			sortBtn->setText(Language::get(4194));
		}
		else if ( sortType == SortTypes_t::SORT_SCROLL_DISCOVERED )
		{
			sortBtn->setText(Language::get(4195));
		}
		else if ( sortType == SortTypes_t::SORT_SCROLL_UNKNOWN )
		{
			sortBtn->setText(Language::get(4196));
		}
		sortTxt->setText(Language::get(4193));
		sortGlyph->disabled = true;
		sortBtn->setDisabled(true);
		if ( inputs.getVirtualMouse(playernum)->draw_cursor )
		{
			sortBtn->setDisabled(!isInteractable);
			if ( isInteractable )
			{
				buttonFeatherUpdateSelectorOnHighlight(playernum, sortBtn);
			}
		}
		else if ( sortBtn->isSelected() )
		{
			sortBtn->deselect();
		}
		if ( sortBtn->isDisabled() && usingGamepad && bDrawerOpen )
		{
			sortGlyph->path = Input::inputs[playernum].getGlyphPathForBinding("MenuAlt2");
			if ( auto imgGet = Image::get(sortGlyph->path.c_str()) )
			{
				sortGlyph->pos.w = imgGet->getWidth();
				sortGlyph->pos.h = imgGet->getHeight();
				sortGlyph->disabled = false;
			}
			sortGlyph->pos.x = sortBtn->getSize().x + sortBtn->getSize().w - 4;
			if ( sortGlyph->pos.x % 2 == 1 )
			{
				++sortGlyph->pos.x;
			}
			sortGlyph->pos.y = sortBtn->getSize().y + sortBtn->getSize().h / 2 - sortGlyph->pos.h / 2;
			if ( sortGlyph->pos.y % 2 == 1 )
			{
				++sortGlyph->pos.y;
			}
		}

		// close btn
		auto closeBtn = drawerFrame->findButton("close drawer button");
		auto closeGlyph = drawerFrame->findImage("close drawer glyph");
		closeBtn->setDisabled(true);
		closeGlyph->disabled = true;
		if ( inputs.getVirtualMouse(playernum)->draw_cursor )
		{
			closeBtn->setDisabled(!isInteractable || !bDrawerOpen);
			if ( isInteractable && bDrawerOpen )
			{
				buttonFeatherUpdateSelectorOnHighlight(playernum, closeBtn);
			}
		}
		else if ( closeBtn->isSelected() )
		{
			closeBtn->deselect();
		}
		if ( closeBtn->isDisabled() && usingGamepad && bDrawerOpen )
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

	auto slider = drawerFrame->findSlider("feather slider");
	auto drawerSlotsFrame = drawerFrame->findFrame("drawer slots");
	// handle height changing..
	{
		int numGrids = (MAX_FEATHER_Y / getNumInscriptionsToDisplayVertical()) + 1;
		auto gridImg = drawerSlotsFrame->findImage("grid img");

		SDL_Rect drawerSlotsFramePos = drawerSlotsFrame->getSize();
		drawerSlotsFramePos.h = (200 + 2);

		SDL_Rect drawerSlotsFrameActualPos{ drawerSlotsFrame->getActualSize().x,
			drawerSlotsFrame->getActualSize().y,
			drawerSlotsFrame->getActualSize().w,
			(drawerSlotsFramePos.h) * numGrids };
		drawerSlotsFrame->setActualSize(drawerSlotsFrameActualPos);
		drawerSlotsFrame->setScrollBarsEnabled(false);
		drawerSlotsFrame->setSize(drawerSlotsFramePos);
		gridImg->pos.y = 0;
		gridImg->pos.h = (drawerSlotsFramePos.h) * numGrids;
	}

	int scrollAmount = std::max((MAX_FEATHER_Y) - (getNumInscriptionsToDisplayVertical()), 0) * inscriptionSlotHeight;
	if ( scrollAmount == 0 )
	{
		slider->setDisabled(true);
	}
	else
	{
		slider->setDisabled(false);
	}

	SDL_Rect sliderPos = slider->getRailSize();
	auto sliderCapTop = drawerFrame->findImage("feather slider top");
	if ( usingGamepad )
	{
		sliderPos.y = 50;
		sliderPos.h = bgImg->pos.h - 44 - 50;
	}
	else
	{
		sliderPos.y = 50 - 18;
		sliderPos.h = bgImg->pos.h - 44 - 50 + 18;
	}
	sliderCapTop->pos.y = sliderPos.y;
	slider->setRailSize(sliderPos);


	currentScrollRow = scrollSetpoint / inscriptionSlotHeight;

	if ( bOpen && isInteractable )
	{
		// do sliders
		if ( !slider->isDisabled() && !(abs(scrollSetpoint - scrollAnimateX) > 0.00001 && usingGamepad) )
		{
			if ( !inputs.getUIInteraction(playernum)->selectedItem
				&& players[playernum]->GUI.activeModule == Player::GUI_t::MODULE_FEATHER )
			{
				auto& input = Input::inputs[playernum];
				if ( inputs.bPlayerUsingKeyboardControl(playernum) )
				{
					if ( input.binaryToggle("MenuMouseWheelDown") )
					{
						scrollSetpoint = std::max(scrollSetpoint + inscriptionSlotHeight, 0);
					}
					if ( input.binaryToggle("MenuMouseWheelUp") )
					{
						scrollSetpoint = std::max(scrollSetpoint - inscriptionSlotHeight, 0);
					}
				}
				if ( input.binaryToggle("MenuScrollDown") )
				{
					scrollSetpoint = std::max(scrollSetpoint + inscriptionSlotHeight * kNumInscriptionsToDisplayVertical, 0);
					if ( player->inventoryUI.cursor.queuedModule == Player::GUI_t::MODULE_FEATHER )
					{
						player->inventoryUI.cursor.queuedModule = Player::GUI_t::MODULE_NONE;
					}
				}
				else if ( input.binaryToggle("MenuScrollUp") )
				{
					scrollSetpoint = std::max(scrollSetpoint - inscriptionSlotHeight * kNumInscriptionsToDisplayVertical, 0);
					if ( player->inventoryUI.cursor.queuedModule == Player::GUI_t::MODULE_FEATHER )
					{
						player->inventoryUI.cursor.queuedModule = Player::GUI_t::MODULE_NONE;
					}
				}
			}
		}

		scrollSetpoint = std::min(scrollSetpoint, scrollAmount);
		currentScrollRow = scrollSetpoint / inscriptionSlotHeight;

		if ( abs(scrollSetpoint - scrollAnimateX) > 0.00001 )
		{
			isInteractable = false;
			const real_t fpsScale = getFPSScale(60.0);
			real_t setpointDiff = 0.0;

			// slightly faster on gamepad
			static ConsoleVariable<float> cvar_feather_slider_speed("/feather_slider_speed", 1.f);
			const real_t factor = (3.0 * (*cvar_feather_slider_speed + (usingGamepad ? -.25f : 0.f)));
			if ( scrollSetpoint - scrollAnimateX > 0.0 )
			{
				setpointDiff = fpsScale * std::max(3.0, (scrollSetpoint - scrollAnimateX)) / (factor);
			}
			else
			{
				setpointDiff = fpsScale * std::min(-3.0, (scrollSetpoint - scrollAnimateX)) / (factor);
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
		if ( !slider->isDisabled() && !usingGamepad )
		{
			sliderFeatherUpdateSelectorOnHighlight(playernum, slider);
		}
		if ( slider->isCurrentlyPressed() )
		{
			auto val = slider->getValue() / 100.0;
			int animX = val * scrollAmount;
			animX /= inscriptionSlotHeight;
			animX *= inscriptionSlotHeight;

			scrollSetpoint = animX;
			scrollSetpoint = std::min(scrollSetpoint, scrollAmount);
		}
		else
		{
			slider->setValue((scrollAnimateX / scrollAmount) * 100.0);
		}
	}
	else
	{
		slider->setValue(0.0);
	}

	SDL_Rect actualSize = drawerSlotsFrame->getActualSize();
	actualSize.y = scrollAnimateX;
	drawerSlotsFrame->setActualSize(actualSize);
}

void GenericGUIMenu::FeatherGUI_t::createFeatherMenu()
{
	const int player = parentGUI.getPlayer();
	if ( !gui || !featherFrame || !players[player]->inventoryUI.frame )
	{
		return;
	}
	if ( featherGUIHasBeenCreated() )
	{
		return;
	}

	SDL_Rect basePos{ 0, 0, featherBaseWidth, featherBaseHeight };
	{
		auto drawerFrame = featherFrame->addFrame("feather drawer");
		SDL_Rect drawerPos{ 0, 0, featherDrawerWidth, 260 };
		drawerFrame->setSize(drawerPos);
		drawerPos.h -= 10; // empty area
		drawerFrame->setHollow(false);

		auto bg = drawerFrame->addImage(drawerPos,
			makeColor(255, 255, 255, 255),
			"*images/ui/Feather/Feather_Drawer_00.png", "feather drawer img");
		drawerFrame->setDisabled(true);

		featherSlotFrames.clear();

		const int baseSlotOffsetX = 0;
		const int baseSlotOffsetY = 0;

		SDL_Rect featherSlotsPos{ 6, 6, featherDrawerWidth, 240 };
		{
			int numGrids = (MAX_FEATHER_Y / kNumInscriptionsToDisplayVertical) + 1;

			const auto drawerSlotsFrame = drawerFrame->addFrame("drawer slots");
			drawerSlotsFrame->setSize(featherSlotsPos);
			drawerSlotsFrame->setActualSize(SDL_Rect{ 0, 0, featherSlotsPos.w, (200 + 2) * numGrids });
			drawerSlotsFrame->setHollow(true);
			drawerSlotsFrame->setAllowScrollBinds(false);
			drawerSlotsFrame->setScrollBarsEnabled(false);

			auto gridImg = drawerSlotsFrame->addImage(SDL_Rect{ 0, 0, 174, 200 },
				makeColor(255, 255, 255, 255), "*images/ui/Feather/Feather_ScrollGrid_00.png", "grid img");
			gridImg->tiled = true;

			const int inventorySlotSize = players[player]->inventoryUI.getSlotSize();
			SDL_Rect currentSlotPos{ baseSlotOffsetX, baseSlotOffsetY, inventorySlotSize, inscriptionSlotHeight };
			for ( int x = 0; x < MAX_FEATHER_X; ++x )
			{
				currentSlotPos.x = baseSlotOffsetX;
				for ( int y = 0; y < MAX_FEATHER_Y; ++y )
				{
					currentSlotPos.y = baseSlotOffsetY + (y * inscriptionSlotHeight);

					char slotname[32] = "";
					snprintf(slotname, sizeof(slotname), "feather %d %d", x, y);
					auto listEntry = drawerSlotsFrame->addFrame(slotname);
					listEntry->setSize(SDL_Rect{ currentSlotPos.x, currentSlotPos.y, gridImg->pos.w, inscriptionSlotHeight });
					featherSlotFrames[x + y * 100] = listEntry;

					auto title = listEntry->addField("title", 128);
					title->setFont("fonts/pixel_maz_multiline.ttf#16#2");
					title->setText("");
					title->setHJustify(Field::justify_t::LEFT);
					title->setVJustify(Field::justify_t::TOP);
					title->setSize(SDL_Rect{ 8, 4, listEntry->getSize().w, 24 });
					title->setColor(hudColors.characterSheetNeutral);

					auto body = listEntry->addField("body", 128);
					body->setFont("fonts/pixel_maz_multiline.ttf#16#2");
					body->setText("");
					body->setHJustify(Field::justify_t::LEFT);
					body->setVJustify(Field::justify_t::TOP);
					body->setSize(SDL_Rect{ 8, 18, listEntry->getSize().w, 24 });
					body->setColor(hudColors.characterSheetFaintText);

					auto bg = listEntry->addImage(listEntry->getSize(), 0xFFFFFFFF,
						"*#images/ui/Feather/Feather_ListUnselected_00.png", "bg");
					bg->pos.x = 0;
					bg->pos.y = 0;
				}
			}
		}

		auto slider = drawerFrame->addSlider("feather slider");
		slider->setBorder(16);
		slider->setMinValue(0);
		slider->setMaxValue(100);
		slider->setValue(0);
		SDL_Rect sliderPos{ featherDrawerWidth - 28, 50, 20, drawerPos.h - 44 - 50 };
		slider->setRailSize(sliderPos);
		slider->setHandleSize(SDL_Rect{ 0, 0, 20, 28 });
		slider->setOrientation(Slider::SLIDER_VERTICAL);
		//slider->setCallback(callback);
		slider->setColor(makeColor(255, 255, 255, 255));
		slider->setHighlightColor(makeColor(255, 255, 255, 255));
		slider->setHandleImage("*#images/ui/Sliders/HUD_Magic_Slider_Blue_01.png");
		slider->setRailImage("*#images/ui/Sliders/HUD_Slider_Blank.png");
		slider->setHideGlyphs(true);
		slider->setHideKeyboardGlyphs(true);
		slider->setHideSelectors(true);
		slider->setMenuConfirmControlType(0);

		auto sliderCapTop = drawerFrame->addImage(SDL_Rect{ sliderPos.x + 2, sliderPos.y, 16, 16 },
			0xFFFFFFFF, "*#images/ui/Sliders/HUD_Magic_Slider_SettingTop_01.png", "feather slider top");
		sliderCapTop->ontop = true;

		auto sliderCapBot = drawerFrame->addImage(SDL_Rect{ sliderPos.x + 2, sliderPos.y + sliderPos.h - 16, 16, 16 },
			0xFFFFFFFF, "*#images/ui/Sliders/HUD_Magic_Slider_SettingBot_01.png", "feather slider bot");
		sliderCapBot->ontop = true;

		{


			auto sortTxt = drawerFrame->addField("sort txt", 128);
			sortTxt->setFont("fonts/pixel_maz_multiline.ttf#16#2");
			sortTxt->setText("");
			sortTxt->setHJustify(Field::justify_t::RIGHT);
			sortTxt->setVJustify(Field::justify_t::TOP);
			sortTxt->setSize(SDL_Rect{ 0, drawerPos.h - 33, 70, 24 });
			sortTxt->setColor(hudColors.characterSheetNeutral);

			auto sortBtn = drawerFrame->addButton("sort btn");
			SDL_Rect sortBtnPos{ drawerPos.w - 26 - 108, drawerPos.h - 40, 112, 34 };
			sortBtn->setSize(sortBtnPos);
			sortBtn->setColor(makeColor(255, 255, 255, 255));
			sortBtn->setHighlightColor(makeColor(255, 255, 255, 255));
			sortBtn->setText("");
			sortBtn->setFont("fonts/pixel_maz_multiline.ttf#16#2");
			sortBtn->setHideGlyphs(true);
			sortBtn->setHideKeyboardGlyphs(true);
			sortBtn->setHideSelectors(true);
			sortBtn->setMenuConfirmControlType(0);
			sortBtn->setBackground("*images/ui/Feather/Feather_Sort_Button_00.png");
			sortBtn->setBackgroundHighlighted("*images/ui/Feather/Feather_Sort_ButtonHigh_00.png");
			sortBtn->setBackgroundActivated("*images/ui/Feather/Feather_Sort_ButtonPress_00.png");
			sortBtn->setTextHighlightColor(makeColor(201, 162, 100, 255));
			sortBtn->setCallback([](Button& button) {
				if ( GenericGUI[button.getOwner()].featherGUI.sortType == SortTypes_t::SORT_SCROLL_DEFAULT )
				{
					GenericGUI[button.getOwner()].featherGUI.changeSortingType(SortTypes_t::SORT_SCROLL_DISCOVERED);
				}
				else if ( GenericGUI[button.getOwner()].featherGUI.sortType == SortTypes_t::SORT_SCROLL_DISCOVERED )
				{
					GenericGUI[button.getOwner()].featherGUI.changeSortingType(SortTypes_t::SORT_SCROLL_UNKNOWN);
				}
				else
				{
					GenericGUI[button.getOwner()].featherGUI.changeSortingType(SortTypes_t::SORT_SCROLL_DEFAULT);
				}
				Player::soundActivate();
			});
			sortBtn->setTickCallback(genericgui_deselect_fn);

			auto closeBtn = drawerFrame->addButton("close drawer button");
			SDL_Rect closeBtnPos{ drawerPos.w - 32, 4, 26, 26 };
			closeBtn->setSize(closeBtnPos);
			closeBtn->setColor(makeColor(255, 255, 255, 255));
			closeBtn->setHighlightColor(makeColor(255, 255, 255, 255));
			closeBtn->setText("X");
			closeBtn->setFont("fonts/pixel_maz_multiline.ttf#16#2");
			closeBtn->setHideGlyphs(true);
			closeBtn->setHideKeyboardGlyphs(true);
			closeBtn->setHideSelectors(true);
			closeBtn->setMenuConfirmControlType(0);
			closeBtn->setBackground("*images/ui/Feather/Button_X_00.png");
			closeBtn->setBackgroundHighlighted("*images/ui/Feather/Button_XHigh_00.png");
			closeBtn->setBackgroundActivated("*images/ui/Feather/Button_XPress_00.png");
			closeBtn->setTextHighlightColor(makeColor(201, 162, 100, 255));
			closeBtn->setCallback([](Button& button) {
				GenericGUI[button.getOwner()].scribingBlankScrollTarget = nullptr;
				GenericGUI[button.getOwner()].featherGUI.bDrawerOpen = false;
				onFeatherChangeTabAction(button.getOwner(), true);
				Player::soundCancel();
			});
			closeBtn->setTickCallback(genericgui_deselect_fn);

			auto sortGlyph = drawerFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
				0xFFFFFFFF, "", "sort glyph");
			sortGlyph->disabled = true;
			sortGlyph->ontop = true;

			auto closeGlyph = drawerFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
				0xFFFFFFFF, "", "close drawer glyph");
			closeGlyph->disabled = true;
			closeGlyph->ontop = true;
		}
	}

	{
		auto bgFrame = featherFrame->addFrame("feather base");
		bgFrame->setSize(basePos);
		bgFrame->setHollow(false);
		bgFrame->setDisabled(true);
		auto bg = bgFrame->addImage(SDL_Rect{ 0, 0, basePos.w, basePos.h },
			makeColor(255, 255, 255, 255),
			"*images/ui/Feather/Feather_Base_00.png", "feather base img");

		auto skillIcon = bgFrame->addImage(SDL_Rect{270, 76, 24, 24},
			makeColor(255, 255, 255, 255),
			"", "feather skill img");
		skillIcon->disabled = true;

		auto headerFont = "fonts/pixel_maz_multiline.ttf#16#2";
		auto featherTitle = bgFrame->addField("feather title", 128);
		featherTitle->setFont(headerFont);
		featherTitle->setText("");
		featherTitle->setHJustify(Field::justify_t::CENTER);
		featherTitle->setVJustify(Field::justify_t::TOP);
		featherTitle->setSize(SDL_Rect{ 0, 0, 0, 0 });
		featherTitle->setTextColor(hudColors.characterSheetLightNeutral);
		featherTitle->setOutlineColor(makeColor(29, 16, 11, 255));
		auto featherStatus = bgFrame->addField("feather status", 128);
		featherStatus->setFont(headerFont);
		featherStatus->setText("");
		featherStatus->setHJustify(Field::justify_t::CENTER);
		featherStatus->setVJustify(Field::justify_t::TOP);
		featherStatus->setSize(SDL_Rect{ 0, 0, 0, 0 });
		featherStatus->setTextColor(hudColors.characterSheetLightNeutral);
		featherStatus->setOutlineColor(makeColor(29, 16, 11, 255));

		auto itemFont = "fonts/pixel_maz_multiline.ttf#16#2";
		auto itemDisplayTooltip = bgFrame->addFrame("feather display tooltip");
		itemDisplayTooltip->setSize(SDL_Rect{ 0, 0, 298, 110 });
		itemDisplayTooltip->setHollow(true);
		itemDisplayTooltip->setInheritParentFrameOpacity(false);
		{
			auto tooltipBg = itemDisplayTooltip->addImage(SDL_Rect{ 0, 0, 298, 110 },
				0xFFFFFFFF, "*images/ui/Feather/Feather_Tooltip_00.png", "tooltip img");

			auto itemNameText = itemDisplayTooltip->addField("item display name", 1024);
			itemNameText->setFont(itemFont);
			itemNameText->setText("");
			itemNameText->setHJustify(Field::justify_t::LEFT);
			itemNameText->setVJustify(Field::justify_t::TOP);
			itemNameText->setSize(SDL_Rect{ 0, 0, 0, 0 });
			itemNameText->setColor(hudColors.characterSheetLightNeutral);

			auto itemDisplayTextBg = itemDisplayTooltip->addImage(SDL_Rect{ 0, 0, 220, 42 },
				0xFFFFFFFF, "*images/ui/Feather/Feather_LabelName_2Row_00.png", "item text img");

			auto itemCostBg = itemDisplayTooltip->addImage(SDL_Rect{ 0, 0, 104, 34 },
				0xFFFFFFFF, "*images/ui/Feather/Feather_CostBacking_00.png", "item cost img");

			auto minChargeText = itemDisplayTooltip->addField("item min value", 32);
			minChargeText->setFont(itemFont);
			minChargeText->setText("");
			minChargeText->setHJustify(Field::justify_t::RIGHT);
			minChargeText->setVJustify(Field::justify_t::TOP);
			minChargeText->setSize(SDL_Rect{ 0, 0, 0, 0 });
			minChargeText->setColor(hudColors.characterSheetLightNeutral);
			auto maxChargeText = itemDisplayTooltip->addField("item max value", 32);
			maxChargeText->setFont(itemFont);
			maxChargeText->setText("");
			maxChargeText->setHJustify(Field::justify_t::RIGHT);
			maxChargeText->setVJustify(Field::justify_t::TOP);
			maxChargeText->setSize(SDL_Rect{ 0, 0, 0, 0 });
			maxChargeText->setColor(hudColors.characterSheetLightNeutral);

			auto costLabel = itemDisplayTooltip->addField("item cost label", 64);
			costLabel->setFont(itemFont);
			costLabel->setText("");
			costLabel->setHJustify(Field::justify_t::RIGHT);
			costLabel->setVJustify(Field::justify_t::TOP);
			costLabel->setSize(SDL_Rect{ 0, 0, 90, 0 });
			costLabel->setColor(hudColors.characterSheetLightNeutral);

			auto itemBgImg = itemDisplayTooltip->addImage(SDL_Rect{ 0, 0, 54, 54 }, 0xFFFFFFFF,
				"*images/ui/Feather/Feather_ItemBGSurround_00.png", "item bg img");

			auto slotFrame = itemDisplayTooltip->addFrame("item slot frame");
			SDL_Rect slotPos{ 0, 0, players[player]->inventoryUI.getSlotSize(), players[player]->inventoryUI.getSlotSize() };
			slotFrame->setSize(slotPos);
			slotFrame->setDisabled(true);
			slotFrame->setInheritParentFrameOpacity(false);
			createPlayerInventorySlotFrameElements(slotFrame);
		}

		{
			auto closeBtn = bgFrame->addButton("close feather button");
			SDL_Rect closeBtnPos{ basePos.w - 0 - 30, 54, 26, 26 };
			closeBtn->setSize(closeBtnPos);
			closeBtn->setColor(makeColor(255, 255, 255, 255));
			closeBtn->setHighlightColor(makeColor(255, 255, 255, 255));
			closeBtn->setText("X");
			closeBtn->setFont(itemFont);
			closeBtn->setHideGlyphs(true);
			closeBtn->setHideKeyboardGlyphs(true);
			closeBtn->setHideSelectors(true);
			closeBtn->setMenuConfirmControlType(0);
			closeBtn->setBackground("*images/ui/Feather/Button_X_00.png");
			closeBtn->setBackgroundHighlighted("*images/ui/Feather/Button_XHigh_00.png");
			closeBtn->setBackgroundActivated("*images/ui/Feather/Button_XPress_00.png");
			closeBtn->setTextHighlightColor(makeColor(201, 162, 100, 255));
			closeBtn->setCallback([](Button& button) {
				GenericGUI[button.getOwner()].closeGUI();
				Player::soundCancel();
			});
			closeBtn->setTickCallback(genericgui_deselect_fn);

			auto closeGlyph = bgFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
				0xFFFFFFFF, "", "close feather glyph");
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

			auto actionPromptCoverLeftImg = bgFrame->addImage(SDL_Rect{ 0, 60, 28, 26 },
				0xFFFFFFFF, "*images/ui/Feather/Feather_PromptCoverLeft_00.png", "action prompt lcover");
			actionPromptCoverLeftImg->ontop = true;
			auto actionPromptCoverRightImg = bgFrame->addImage(SDL_Rect{ bg->pos.w - 28, 60, 28, 26 },
				0xFFFFFFFF, "*images/ui/Feather/Feather_PromptCoverRight_00.png", "action prompt rcover");
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
			Button* filterBtn = bgFrame->addButton("filter inscribe btn");
			filterBtn->setColor(makeColor(255, 255, 255, 0));
			filterBtn->setHighlightColor(makeColor(255, 255, 255, 0));
			filterBtn->setText("");
			filterBtn->setFont(itemFont);
			filterBtn->setTextHighlightColor(makeColor(201, 162, 100, 255));
			filterBtn->setHideGlyphs(true);
			filterBtn->setHideKeyboardGlyphs(true);
			filterBtn->setHideSelectors(true);
			filterBtn->setMenuConfirmControlType(0);
			filterBtn->setCallback([](Button& button) {
				auto oldTab = GenericGUI[button.getOwner()].scribingFilter;
				bool changeToDifferentTab = oldTab != GenericGUIMenu::SCRIBING_FILTER_CRAFTABLE;
				GenericGUI[button.getOwner()].scribingFilter = GenericGUIMenu::SCRIBING_FILTER_CRAFTABLE;
				onFeatherChangeTabAction(button.getOwner(), changeToDifferentTab);
				GenericGUI[button.getOwner()].featherGUI.animPromptMoveLeft = true;
				if ( changeToDifferentTab )
				{
					Player::soundModuleNavigation();
				}
			});
			filterBtn->setTickCallback(genericgui_deselect_fn);

			Field* filterTxt = bgFrame->addField("filter inscribe txt", 64);
			filterTxt->setFont(itemFont);
			filterTxt->setText("Inscribe");
			filterTxt->setHJustify(Field::justify_t::CENTER);
			filterTxt->setVJustify(Field::justify_t::TOP);
			filterTxt->setSize(SDL_Rect{ 0, 0, 0, 0 });
			filterTxt->setColor(hudColors.characterSheetLightNeutral);
			filterTxt->setDisabled(true);
			filterTxt->setOntop(true);

			filterBtn = bgFrame->addButton("filter repair btn");
			filterBtn->setColor(makeColor(255, 255, 255, 0));
			filterBtn->setHighlightColor(makeColor(255, 255, 255, 0));
			filterBtn->setText("");
			filterBtn->setFont(itemFont);
			filterBtn->setTextHighlightColor(makeColor(201, 162, 100, 255));
			filterBtn->setHideGlyphs(true);
			filterBtn->setHideKeyboardGlyphs(true);
			filterBtn->setHideSelectors(true);
			filterBtn->setMenuConfirmControlType(0);
			filterBtn->setCallback([](Button& button) {
				auto oldTab = GenericGUI[button.getOwner()].scribingFilter;
				bool changeToDifferentTab = oldTab != GenericGUIMenu::SCRIBING_FILTER_REPAIRABLE;
				GenericGUI[button.getOwner()].scribingFilter = GenericGUIMenu::SCRIBING_FILTER_REPAIRABLE;
				onFeatherChangeTabAction(button.getOwner(), changeToDifferentTab);
				GenericGUI[button.getOwner()].featherGUI.animPromptMoveLeft = false;
				if ( changeToDifferentTab )
				{
					Player::soundModuleNavigation();
				}
			});
			filterBtn->setTickCallback(genericgui_deselect_fn);

			filterTxt = bgFrame->addField("filter repair txt", 64);
			filterTxt->setFont(itemFont);
			filterTxt->setText("Repair");
			filterTxt->setHJustify(Field::justify_t::CENTER);
			filterTxt->setVJustify(Field::justify_t::TOP);
			filterTxt->setSize(SDL_Rect{ 0, 0, 0, 0 });
			filterTxt->setColor(hudColors.characterSheetLightNeutral);
			filterTxt->setDisabled(true);
			filterTxt->setOntop(true);

			auto filterNavLeftArrow = bgFrame->addImage(SDL_Rect{ 0, 0, 30, 44 },
				0xFFFFFFFF, "*#images/ui/Feather/Feather_Button_ArrowL_01.png", "filter nav left arrow");
			filterNavLeftArrow->disabled = true;
			auto filterNavRightArrow = bgFrame->addImage(SDL_Rect{ 0, 0, 30, 44 },
				0xFFFFFFFF, "*#images/ui/Feather/Feather_Button_ArrowR_01.png", "filter nav right arrow");
			filterNavRightArrow->disabled = true;

			auto filterNavLeft = bgFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
				0xFFFFFFFF, "", "filter nav left");
			filterNavLeft->disabled = true;
			auto filterNavRight = bgFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
				0xFFFFFFFF, "", "filter nav right");
			filterNavRight->disabled = true;
		}

		{
			auto currentChargeBg = bgFrame->addImage(SDL_Rect{ 0, 0, 122, 34 },
				0xFFFFFFFF, "*images/ui/Feather/Feather_ChargeBacking_00.png", "current charge img");

			auto currentChargeLabel = bgFrame->addField("current charge label", 64);
			currentChargeLabel->setFont(itemFont);
			currentChargeLabel->setText("");
			currentChargeLabel->setHJustify(Field::justify_t::RIGHT);
			currentChargeLabel->setVJustify(Field::justify_t::TOP);
			currentChargeLabel->setSize(SDL_Rect{ 0, 0, 90, 0 });
			currentChargeLabel->setColor(hudColors.characterSheetLightNeutral);

			auto currentChargeText = bgFrame->addField("current charge txt", 32);
			currentChargeText->setFont(itemFont);
			currentChargeText->setText("");
			currentChargeText->setHJustify(Field::justify_t::RIGHT);
			currentChargeText->setVJustify(Field::justify_t::TOP);
			currentChargeText->setSize(SDL_Rect{ 0, 0, 0, 0 });
			currentChargeText->setColor(hudColors.characterSheetLightNeutral);

			auto changeChargeText = bgFrame->addField("change charge txt", 32);
			changeChargeText->setFont(itemFont);
			changeChargeText->setText("");
			changeChargeText->setHJustify(Field::justify_t::RIGHT);
			changeChargeText->setVJustify(Field::justify_t::TOP);
			changeChargeText->setSize(SDL_Rect{ 0, 0, 0, 0 });
			changeChargeText->setColor(hudColors.characterSheetRed);
		}
	}
}

bool GenericGUIMenu::FeatherGUI_t::featherGUIHasBeenCreated() const
{
	if ( featherFrame )
	{
		if ( !featherFrame->getFrames().empty() )
		{
			for ( auto f : featherFrame->getFrames() )
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

void GenericGUIMenu::FeatherGUI_t::selectFeatherSlot(const int x, const int y)
{
	selectedFeatherSlotX = x;
	selectedFeatherSlotY = y;
}

Frame* GenericGUIMenu::FeatherGUI_t::getFeatherSlotFrame(int x, int y) const
{
	if ( featherFrame )
	{
		int key = x + y * 100;
		if ( featherSlotFrames.find(key) != featherSlotFrames.end() )
		{
			return featherSlotFrames.at(key);
		}
		//assert(alchemySlotFrames.find(key) == alchemySlotFrames.end());
	}
	return nullptr;
}

GenericGUIMenu::FeatherGUI_t::FeatherActions_t GenericGUIMenu::FeatherGUI_t::setItemDisplayNameAndPrice(Item* item, bool checkResultOnly)
{
	auto result = FEATHER_ACTION_NONE;
	int featherCharge = 0;
	if ( parentGUI.scribingToolItem )
	{
		featherCharge = parentGUI.scribingToolItem->appearance % ENCHANTED_FEATHER_MAX_DURABILITY;
	}
	bool isItemInscriptionNode = parentGUI.isNodeScribingCraftableItem(item->node);
	if ( !checkResultOnly )
	{
		chargeCostMin = 0;
		chargeCostMax = 0;
	}
	if ( item )
	{
		if ( parentGUI.scribingFilter == SCRIBING_FILTER_CRAFTABLE )
		{
			if ( isItemInscriptionNode )
			{
				std::string label = item->getScrollLabel();
				if ( !checkResultOnly )
				{
					currentHoveringInscriptionLabel = label;
				}
				bool identified = false;
				for ( int i = 0; i < NUMLABELS; ++i )
				{
					if ( label == scroll_label[i] )
					{
						for ( auto& s : clientLearnedScrollLabels[parentGUI.getPlayer()] )
						{
							if ( s == i )
							{
								// we know this scroll's index
								identified = true;
								break;
							}
						}
						break;
					}
				}
				int chargeMin = 0;
				int chargeMax = 0;
				parentGUI.scribingGetChargeCost(item, chargeMin, chargeMax);
				if ( bDrawerOpen && !parentGUI.scribingBlankScrollTarget )
				{
					if ( !checkResultOnly )
					{
						chargeCostMin = chargeMin;
						chargeCostMax = chargeMax;
					}
					if ( !identified )
					{
						result = FEATHER_ACTION_NO_BLANK_SCROLL_UNKNOWN_HIGHLIGHT;
					}
					else
					{
						result = FEATHER_ACTION_NO_BLANK_SCROLL;
					}
				}
				else if ( !identified )
				{
					if ( !checkResultOnly )
					{
						chargeCostMin = chargeMin;
						chargeCostMax = chargeMax;
					}
					result = FEATHER_ACTION_OK_UNKNOWN_SCROLL;
				}
				else if ( featherCharge >= chargeMin )
				{
					if ( !checkResultOnly )
					{
						chargeCostMin = chargeMin;
						chargeCostMax = chargeMax;
					}

					if ( featherCharge == chargeMin )
					{
						result = FEATHER_ACTION_OK_AND_DESTROY;
					}
					else if ( featherCharge < chargeMax )
					{
						result = FEATHER_ACTION_MAY_SUCCEED;
					}
					else
					{
						result = FEATHER_ACTION_OK;
					}
				}
				else if ( (chargeMin / 2) < featherCharge )
				{
					if ( !checkResultOnly )
					{
						chargeCostMin = chargeMin;
						chargeCostMax = chargeMax;
					}
					result = FEATHER_ACTION_OK_AND_DESTROY;
				}
				else
				{
					if ( !checkResultOnly )
					{
						chargeCostMin = chargeMin;
						chargeCostMax = chargeMax;
					}
					result = FEATHER_ACTION_CANT_AFFORD;
				}
			}
			else
			{
				if ( itemCategory(item) == SCROLL && !item->identified )
				{
					result = FEATHER_ACTION_UNIDENTIFIED;
				}
				else if ( item->identified && item->type == SCROLL_BLANK )
				{
					result = FEATHER_ACTION_OK;
				}
			}
		}
		else if ( parentGUI.scribingFilter == SCRIBING_FILTER_REPAIRABLE && itemCategory(item) == SPELLBOOK )
		{
			if ( !item->identified )
			{
				result = FEATHER_ACTION_UNIDENTIFIED;
			}
			else if ( item->status >= EXCELLENT )
			{
				result = FEATHER_ACTION_FULLY_REPAIRED;
			}
			else
			{
				int chargeMin = 0;
				int chargeMax = 0;
				parentGUI.scribingGetChargeCost(item, chargeMin, chargeMax);
				if ( featherCharge >= chargeMin )
				{
					if ( !checkResultOnly )
					{
						chargeCostMin = chargeMin;
						chargeCostMax = chargeMax;
					}
					if ( featherCharge == chargeMin )
					{
						result = FEATHER_ACTION_OK_AND_DESTROY;
					}
					else if ( featherCharge < chargeMax )
					{
						result = FEATHER_ACTION_MAY_SUCCEED;
					}
					else
					{
						result = FEATHER_ACTION_OK;
					}
				}
				else
				{
					if ( !checkResultOnly )
					{
						chargeCostMin = chargeMin;
						chargeCostMax = chargeMax;
					}
					result = FEATHER_ACTION_CANT_AFFORD;
				}
			}
		}
	}

	if ( !checkResultOnly )
	{
		Item* displayItem = item;
		if ( isItemInscriptionNode )
		{
			displayItem = parentGUI.scribingBlankScrollTarget;
			if ( !parentGUI.scribingBlankScrollTarget )
			{
				itemDesc = "  ";
				itemRequiresTitleReflow = true;
				if ( featherFrame )
				{
					if ( auto baseFrame = featherFrame->findFrame("feather base") )
					{
						if ( auto itemTooltipFrame = baseFrame->findFrame("feather display tooltip") )
						{
							auto itemSlotFrame = itemTooltipFrame->findFrame("item slot frame");
							itemSlotFrame->setDisabled(true);
						}
					}
				}
			}
		}
		if ( result != FEATHER_ACTION_NONE && displayItem )
		{
			char buf[1024];
			if ( !displayItem->identified )
			{
				snprintf(buf, sizeof(buf), "%s %s (?)", ItemTooltips.getItemStatusAdjective(displayItem->type, displayItem->status).c_str(), displayItem->getName());
			}
			else
			{
				snprintf(buf, sizeof(buf), "%s %s (%+d)", ItemTooltips.getItemStatusAdjective(displayItem->type, displayItem->status).c_str(), displayItem->getName(), displayItem->beatitude);
			}
			if ( itemDesc != buf )
			{
				itemRequiresTitleReflow = true;
			}
			itemDesc = buf;
			itemType = displayItem->type;

			if ( featherFrame )
			{
				if ( auto baseFrame = featherFrame->findFrame("feather base") )
				{
					if ( auto itemTooltipFrame = baseFrame->findFrame("feather display tooltip") )
					{
						auto itemSlotFrame = itemTooltipFrame->findFrame("item slot frame");
						updateSlotFrameFromItem(itemSlotFrame, displayItem);
					}
				}
			}
		}
		itemActionType = result;
	}
	return result;
}

bool GenericGUIMenu::FeatherGUI_t::warpMouseToSelectedFeatherItem(Item* snapToItem, Uint32 flags)
{
	if ( featherGUIHasBeenCreated() )
	{
		int x = getSelectedFeatherSlotX();
		int y = getSelectedFeatherSlotY();
		if ( snapToItem )
		{
			x = snapToItem->x;
			y = snapToItem->y;
		}

		if ( isInteractable )
		{
			if ( abs(scrollAnimateX - scrollSetpoint) > 0.00001 )
			{
				int diff = (scrollAnimateX - scrollSetpoint) / inscriptionSlotHeight;
				y += diff; // if we have a scroll in the works, then manipulate y to pretend where we'd be ahead of time.
			}
		}

		if ( auto slot = getFeatherSlotFrame(x, y) )
		{
			int playernum = parentGUI.getPlayer();
			auto player = players[playernum];
			if ( !isInteractable )
			{
				//messagePlayer(0, "[Debug]: select item queued");
				player->inventoryUI.cursor.queuedModule = Player::GUI_t::MODULE_FEATHER;
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

void GenericGUIMenu::FeatherGUI_t::clearItemDisplayed()
{
	itemType = -1;
	itemActionType = FEATHER_ACTION_NONE;
	chargeCostMin = 0;
	chargeCostMax = 0;
	currentHoveringInscriptionLabel = "";
}

int GenericGUIMenu::FeatherGUI_t::getNumInscriptionsToDisplayVertical() const
{
	return kNumInscriptionsToDisplayVertical;
}

void GenericGUIMenu::FeatherGUI_t::scrollToSlot(int x, int y, bool instantly)
{
	int lowerY = currentScrollRow;
	int upperY = currentScrollRow + getNumInscriptionsToDisplayVertical() - 1;

	if ( y >= lowerY && y <= upperY )
	{
		// no work to do.
		return;
	}
	int player = parentGUI.getPlayer();
	int lowestItemY = MAX_FEATHER_Y - 1;
	const int slotSize = inscriptionSlotHeight;
	int maxScroll = std::max((lowestItemY + 1) - (getNumInscriptionsToDisplayVertical()), 0) * slotSize;

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

bool GenericGUIMenu::FeatherGUI_t::isSlotVisible(int x, int y) const
{
	if ( featherFrame )
	{
		if ( featherFrame->isDisabled() )
		{
			return false;
		}
	}
	int lowerY = currentScrollRow;
	int upperY = currentScrollRow + getNumInscriptionsToDisplayVertical() - 1;

	if ( y >= lowerY && y <= upperY )
	{
		return true;
	}
	return false;
}

bool GenericGUIMenu::FeatherGUI_t::isItemVisible(Item* item) const
{
	if ( !item ) { return false; }
	return isSlotVisible(item->x, item->y);
}

bool GenericGUIMenu::FeatherGUI_t::isInscriptionDrawerItemSelected(Item* item)
{
	if ( !item || itemCategory(item) == SPELL_CAT )
	{
		return false;
	}

	if ( !isInscriptionDrawerOpen() || !parentGUI.isNodeScribingCraftableItem(item->node) || !isItemVisible(item) )
	{
		return false;
	}

	if ( players[parentGUI.getPlayer()]->GUI.activeModule == Player::GUI_t::MODULE_FEATHER )
	{
		if ( selectedFeatherSlotX >= 0 && selectedFeatherSlotX < MAX_FEATHER_X
			&& selectedFeatherSlotY >= 0 && selectedFeatherSlotY < MAX_FEATHER_Y
			&& item->x == selectedFeatherSlotX && item->y == selectedFeatherSlotY )
		{
			if ( auto slotFrame = getFeatherSlotFrame(item->x, item->y) )
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

bool GenericGUIMenu::FeatherGUI_t::isItemSelectedToRepairOrInscribe(Item* item)
{
	if ( !item || itemCategory(item) == SPELL_CAT )
	{
		return false;
	}

	if ( isInscriptionDrawerOpen() || !parentGUI.isNodeFromPlayerInventory(item->node) )
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

bool GenericGUIMenu::FeatherGUI_t::isInscriptionDrawerOpen() const
{
	if ( !parentGUI.isGUIOpen() || !featherGUIHasBeenCreated() )
	{
		return false;
	}
	if ( bOpen && parentGUI.guiType == GUICurrentType::GUI_TYPE_SCRIBING
		&& parentGUI.scribingFilter == ScribingFilter::SCRIBING_FILTER_CRAFTABLE
		&& bDrawerOpen )
	{
		return true;
	}
	return false;
}

bool GenericGUIMenu::FeatherGUI_t::isInscribeOrRepairActive() const
{
	if ( !parentGUI.isGUIOpen() || !featherGUIHasBeenCreated() )
	{
		return false;
	}
	if ( bOpen && parentGUI.guiType == GUICurrentType::GUI_TYPE_SCRIBING
		&& (parentGUI.scribingFilter == ScribingFilter::SCRIBING_FILTER_CRAFTABLE
			|| parentGUI.scribingFilter == ScribingFilter::SCRIBING_FILTER_REPAIRABLE) )
	{
		return true;
	}
	return false;
}

void GenericGUIMenu::ItemEffectGUI_t::clearItemDisplayed()
{
	itemType = -1;
	itemActionType = ITEMFX_ACTION_NONE;
}

GenericGUIMenu::ItemEffectGUI_t::ItemEffectActions_t GenericGUIMenu::ItemEffectGUI_t::setItemDisplayNameAndPrice(Item* item, bool checkResultOnly)
{
	auto result = ITEMFX_ACTION_NONE;

	if ( item )
	{
		if ( currentMode == ITEMFX_MODE_SCROLL_IDENTIFY 
			|| currentMode == ITEMFX_MODE_SPELL_IDENTIFY )
		{
			if ( itemCategory(item) == SPELL_CAT )
			{
				result = ITEMFX_ACTION_INVALID_ITEM;
			}
			else if ( item->identified )
			{
				//result = ITEMFX_ACTION_ITEM_IDENTIFIED;
			}
			else
			{
				result = ITEMFX_ACTION_OK;
			}
		}
		else if ( currentMode == ITEMFX_MODE_SCROLL_REMOVECURSE
			|| currentMode == ITEMFX_MODE_SPELL_REMOVECURSE )
		{
			if ( itemCategory(item) == SPELL_CAT )
			{
				result = ITEMFX_ACTION_INVALID_ITEM;
			}
			else if ( !item->identified )
			{
				result = ITEMFX_ACTION_NOT_IDENTIFIED_YET;
			}
			else if ( item->beatitude >= 0 )
			{
				//result = ITEMFX_ACTION_NOT_CURSED;
			}
			else if ( item->beatitude < 0 )
			{
				result = ITEMFX_ACTION_OK;
			}
		}
		else if ( currentMode == ITEMFX_MODE_SCROLL_CHARGING )
		{
			if ( itemCategory(item) == SPELL_CAT )
			{
				result = ITEMFX_ACTION_INVALID_ITEM;
			}
			else if ( item->type == ENCHANTED_FEATHER )
			{
				if ( item->appearance % ENCHANTED_FEATHER_MAX_DURABILITY < 100 )
				{
					result = ITEMFX_ACTION_OK;
				}
				else
				{
					result = ITEMFX_ACTION_ITEM_FULLY_CHARGED;
				}
			}
			else if ( itemCategory(item) == MAGICSTAFF )
			{
				if ( item->status == EXCELLENT )
				{
					result = ITEMFX_ACTION_ITEM_FULLY_CHARGED;
				}
				else
				{
					result = ITEMFX_ACTION_OK;
				}
			}
			else
			{
				result = ITEMFX_ACTION_INVALID_ITEM;
			}
		}
		else if ( currentMode == ITEMFX_MODE_SCROLL_ENCHANT_ARMOR )
		{
			if ( itemCategory(item) == SPELL_CAT )
			{
				result = ITEMFX_ACTION_INVALID_ITEM;
			}
			else if ( !item->identified )
			{
				result = ITEMFX_ACTION_NOT_IDENTIFIED_YET;
			}
			else
			{
				if ( parentGUI.isItemEnchantArmorable(item) )
				{
					result = ITEMFX_ACTION_OK;
				}
				else
				{
					result = ITEMFX_ACTION_INVALID_ITEM;
				}
			}
		}
		else if ( currentMode == ITEMFX_MODE_SCROLL_ENCHANT_WEAPON )
		{
			if ( itemCategory(item) == SPELL_CAT )
			{
				result = ITEMFX_ACTION_INVALID_ITEM;
			}
			else if ( !item->identified )
			{
				result = ITEMFX_ACTION_NOT_IDENTIFIED_YET;
			}
			else
			{
				if ( parentGUI.isItemEnchantWeaponable(item) )
				{
					result = ITEMFX_ACTION_OK;
				}
				else
				{
					result = ITEMFX_ACTION_INVALID_ITEM;
				}
			}
		}
		else if ( currentMode == ITEMFX_MODE_SCROLL_REPAIR )
		{
			if ( itemCategory(item) == SPELL_CAT )
			{
				result = ITEMFX_ACTION_INVALID_ITEM;
			}
			else if ( !item->identified )
			{
				result = ITEMFX_ACTION_NOT_IDENTIFIED_YET;
			}
			else 
			{
				switch ( itemCategory(item) )
				{
					case WEAPON:
						result = ITEMFX_ACTION_OK;
						break;
					case ARMOR:
						result = ITEMFX_ACTION_OK;
						break;
					case MAGICSTAFF:
						result = ITEMFX_ACTION_INVALID_ITEM;
						break;
					case THROWN:
						if ( item->type == BOOMERANG )
						{
							result = ITEMFX_ACTION_OK;
						}
						else
						{
							result = ITEMFX_ACTION_INVALID_ITEM;
						}
						break;
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
								result = ITEMFX_ACTION_INVALID_ITEM;
								break;
							default:
								if ( itemTypeIsQuiver(item->type) )
								{
									result = ITEMFX_ACTION_INVALID_ITEM;
								}
								else
								{
									result = ITEMFX_ACTION_OK;
								}
								break;
						}
						break;
					default:
						result = ITEMFX_ACTION_INVALID_ITEM;
						break;
				}

				if ( result == ITEMFX_ACTION_OK )
				{
					if ( item->status == EXCELLENT )
					{
						result = ITEMFX_ACTION_ITEM_FULLY_REPAIRED;
					}
				}
			}
		}

		if ( result == ITEMFX_ACTION_INVALID_ITEM || item == parentGUI.itemEffectScrollItem )
		{
			result = ITEMFX_ACTION_NONE;
		}
	}

	if ( !checkResultOnly )
	{
		if ( result != ITEMFX_ACTION_NONE && item )
		{
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
				else if ( item->type == ENCHANTED_FEATHER && item->identified )
				{
					snprintf(buf, sizeof(buf), "%s %s %d%% (%+d)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(),
						item->getName(), item->appearance % ENCHANTED_FEATHER_MAX_DURABILITY, item->beatitude);
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
			itemType = item->type;

			if ( itemEffectFrame )
			{
				if ( auto baseFrame = itemEffectFrame->findFrame("itemfx base") )
				{
					if ( auto itemTooltipFrame = baseFrame->findFrame("itemfx display tooltip") )
					{
						auto itemSlotFrame = itemTooltipFrame->findFrame("item slot frame");
						updateSlotFrameFromItem(itemSlotFrame, item);
					}
				}
			}
		}
		itemActionType = result;
	}
	return result;
}

bool GenericGUIMenu::ItemEffectGUI_t::isItemSelectedToEffect(Item* item)
{
	if ( !item || itemCategory(item) == SPELL_CAT )
	{
		return false;
	}

	if ( !parentGUI.isNodeFromPlayerInventory(item->node) )
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

void GenericGUIMenu::ItemEffectGUI_t::openItemEffectMenu(GenericGUIMenu::ItemEffectGUI_t::ItemEffectModes mode)
{
	const int playernum = parentGUI.getPlayer();
	auto player = players[playernum];

	currentMode = mode;
	if ( itemEffectFrame )
	{
		bool wasDisabled = itemEffectFrame->isDisabled();
		itemEffectFrame->setDisabled(false);
		if ( wasDisabled )
		{
			animx = 0.0;
			animTooltip = 0.0;
			animPrompt = 0.0;
			animFilter = 0.0;
			isInteractable = false;
			bFirstTimeSnapCursor = false;
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

void GenericGUIMenu::ItemEffectGUI_t::closeItemEffectMenu()
{
	const int playernum = parentGUI.getPlayer();
	auto& player = *players[playernum];

	if ( itemEffectFrame )
	{
		itemEffectFrame->setDisabled(true);
	}
	animx = 0.0;
	animTooltip = 0.0;
	animPrompt = 0.0;
	animFilter = 0.0;
	animInvalidAction = 0.0;
	animInvalidActionTicks = 0;
	panelJustifyInverted = false;
	currentMode = ITEMFX_MODE_NONE;
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
	if ( players[playernum]->GUI.activeModule == Player::GUI_t::MODULE_ITEMEFFECTGUI
		&& !players[playernum]->shootmode )
	{
		// reset to inventory mode if still hanging in itemeffect GUI
		players[playernum]->hud.compactLayoutMode = Player::HUD_t::COMPACT_LAYOUT_INVENTORY;
		players[playernum]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
		if ( !inputs.getVirtualMouse(playernum)->draw_cursor )
		{
			players[playernum]->GUI.warpControllerToModule(false);
		}
	}
	clearItemDisplayed();
	itemRequiresTitleReflow = true;
	if ( itemEffectFrame )
	{
		for ( auto f : itemEffectFrame->getFrames() )
		{
			f->removeSelf();
		}
	}
}

int GenericGUIMenu::ItemEffectGUI_t::heightOffsetWhenNotCompact = 170;
const int itemEffectBaseWidth = 334;

bool GenericGUIMenu::ItemEffectGUI_t::ItemEffectHasBeenCreated() const
{
	if ( itemEffectFrame )
	{
		if ( !itemEffectFrame->getFrames().empty() )
		{
			for ( auto f : itemEffectFrame->getFrames() )
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

void GenericGUIMenu::ItemEffectGUI_t::updateItemEffectMenu()
{
	const int playernum = parentGUI.getPlayer();
	auto player = players[playernum];

	if ( !player->isLocalPlayer() )
	{
		closeItemEffectMenu();
		return;
	}

	if ( !itemEffectFrame )
	{
		return;
	}

	itemEffectFrame->setSize(SDL_Rect{ players[playernum]->camera_virtualx1(),
		players[playernum]->camera_virtualy1(),
		tinkerBaseWidth,
		players[playernum]->camera_virtualHeight() });

	if ( !itemEffectFrame->isDisabled() && bOpen )
	{
		if ( player->inventoryUI.getSelectedSlotY() < 0 && player->inventoryUI.bCompactView )
		{
			if ( !panelJustifyInverted )
			{
				animx = 0.0; // reinit animation
			}
			panelJustifyInverted = true;
		}
		else
		{
			if ( panelJustifyInverted )
			{
				animx = 0.0; // reinit animation
			}
			panelJustifyInverted = false;
		}

		if ( !ItemEffectHasBeenCreated() )
		{
			createItemEffectMenu();
		}

		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
		real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animx)) / 2.0;
		animx += setpointDiffX;
		animx = std::min(1.0, animx);
		if ( animx >= .9999 )
		{
			if ( !bFirstTimeSnapCursor )
			{
				bFirstTimeSnapCursor = true;
				//if ( !inputs.getUIInteraction(playernum)->selectedItem
				//	&& player->GUI.activeModule == Player::GUI_t::MODULE_TINKERING )
				//{
				//	warpMouseToSelectedTinkerItem(nullptr, (Inputs::SET_CONTROLLER));
				//}
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

	{
		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
		real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animFilter)) / 2.0;
		animFilter += setpointDiffX;
		animFilter = std::min(1.0, animFilter);
	}

	auto itemFxFramePos = itemEffectFrame->getSize();
	if ( (player->inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_LEFT && !panelJustifyInverted) 
		|| (player->inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_RIGHT && panelJustifyInverted) )
	{
		if ( !player->inventoryUI.bCompactView )
		{
			const int fullWidth = itemFxFramePos.w + 210; // inventory width 210
			itemFxFramePos.x = -itemFxFramePos.w + animx * fullWidth;
			if ( player->bUseCompactGUIWidth() )
			{
				if ( player->inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				itemFxFramePos.x -= player->inventoryUI.slideOutWidth * player->inventoryUI.slideOutPercent;
			}
		}
		else
		{
			if ( player->bAlignGUINextToInventoryCompact() )
			{
				const int fullWidth = itemFxFramePos.w + 210; // inventory width 210
				itemFxFramePos.x = -itemFxFramePos.w + animx * fullWidth;
			}
			else
			{
				itemFxFramePos.x = player->camera_virtualWidth() - animx * itemFxFramePos.w;
			}
			if ( player->bUseCompactGUIWidth() )
			{
				if ( player->inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				itemFxFramePos.x -= -player->inventoryUI.slideOutWidth * player->inventoryUI.slideOutPercent;
			}
		}
	}
	else if ( (player->inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_RIGHT && !panelJustifyInverted) 
		|| (player->inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_LEFT && panelJustifyInverted) )
	{
		if ( !player->inventoryUI.bCompactView )
		{
			const int fullWidth = itemFxFramePos.w + 210; // inventory width 210
			itemFxFramePos.x = player->camera_virtualWidth() - animx * fullWidth;
			if ( player->bUseCompactGUIWidth() )
			{
				if ( player->inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				itemFxFramePos.x -= -player->inventoryUI.slideOutWidth * player->inventoryUI.slideOutPercent;
			}
		}
		else
		{
			if ( player->bAlignGUINextToInventoryCompact() )
			{
				const int fullWidth = itemFxFramePos.w + 210; // inventory width 210
				itemFxFramePos.x = player->camera_virtualWidth() - animx * fullWidth;
			}
			else
			{
				itemFxFramePos.x = -itemFxFramePos.w + animx * itemFxFramePos.w;
			}
			if ( player->bUseCompactGUIWidth() )
			{
				if ( player->inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				itemFxFramePos.x -= player->inventoryUI.slideOutWidth * player->inventoryUI.slideOutPercent;
			}
		}
	}

	int heightOffsetCompact = 0;
	if ( !player->bUseCompactGUIHeight() && !player->bUseCompactGUIWidth() )
	{
		itemFxFramePos.y = heightOffsetWhenNotCompact;
	}
	else
	{
		itemFxFramePos.y = 0;
		heightOffsetCompact = -20;
	}

	if ( !ItemEffectHasBeenCreated() )
	{
		return;
	}

	auto baseFrame = itemEffectFrame->findFrame("itemfx base");
	baseFrame->setDisabled(false);

	itemEffectFrame->setSize(itemFxFramePos);

	SDL_Rect baseFramePos = baseFrame->getSize();
	baseFramePos.x = 0;
	baseFramePos.w = itemEffectBaseWidth;
	baseFrame->setSize(baseFramePos);

	auto baseBg = baseFrame->findImage("itemfx base img");
	baseBg->pos.y = heightOffsetCompact;

	{
		itemFxFramePos.h = baseFramePos.y + baseFramePos.h;
		itemEffectFrame->setSize(itemFxFramePos);

		baseFramePos.x = itemFxFramePos.w - baseFramePos.w;
		baseFrame->setSize(baseFramePos);
	}

	if ( !bOpen )
	{
		return;
	}

	if ( !parentGUI.isGUIOpen()
		|| parentGUI.guiType != GUICurrentType::GUI_TYPE_ITEMFX
		|| !stats[playernum]
		|| stats[playernum]->HP <= 0
		|| !player->entity
		|| player->shootmode )
	{
		closeItemEffectMenu();
		return;
	}

	if ( player->entity && player->entity->isBlind() )
	{
		messagePlayer(playernum, MESSAGE_MISC, Language::get(4159));
		parentGUI.closeGUI();
		return; // I can't see!
	}

	// item effect status
	{
		auto itemFxTitle = baseFrame->findField("itemfx title");
		auto itemFxStatus = baseFrame->findField("itemfx status");
		if ( parentGUI.itemEffectScrollItem || (!parentGUI.itemEffectScrollItem && parentGUI.itemEffectUsingSpellbook) )
		{
			Item* item = parentGUI.itemEffectScrollItem;
			bool isSpell = false;
			spell_t* spell = nullptr;
			int spellID = SPELL_NONE;
			char buf[128];
			if ( item && item->type == SPELL_ITEM )
			{
				isSpell = true;
				spell = getSpellFromItem(parentGUI.getPlayer(), item, false);
				if ( spell )
				{
					spellID = spell->ID;
				}
			}
			else if ( !parentGUI.itemEffectScrollItem && parentGUI.itemEffectUsingSpellbook )
			{
				isSpell = true;
				spellID = getSpellIDFromSpellbook(parentGUI.itemEffectItemType);
				if ( spellID != SPELL_NONE )
				{
					spell = getSpellFromID(spellID);
				}
			}
			
			if ( isSpell )
			{
				std::string prefix = ItemTooltips.adjectives["spell_prefixes"]["spell_of"].c_str();
				camelCaseString(prefix);
				itemFxTitle->setText(prefix.c_str());

				std::string statusStr = "";
				if ( spell )
				{
					statusStr = spell->getSpellName();
				}
				if ( !statusStr.empty() )
				{
					camelCaseString(statusStr);
					itemFxStatus->setText(statusStr.c_str());
				}
			}
			else if ( item )
			{
				if ( !item->identified )
				{
					std::string prefix = ItemTooltips.adjectives["scroll_prefixes"]["unknown_scroll"].c_str();
					snprintf(buf, sizeof(buf), "%s (?)", prefix.c_str());
				}
				else
				{
					std::string prefix = ItemTooltips.adjectives["scroll_prefixes"]["scroll_of"].c_str();
					snprintf(buf, sizeof(buf), "%s", prefix.c_str());
				}
				std::string titleStr = buf;
				if ( !titleStr.empty() )
				{
					camelCaseString(titleStr);
					itemFxTitle->setText(titleStr.c_str());
				}
				else
				{
					itemFxTitle->setText(buf);
				}

				if ( item->identified )
				{
					std::string scrollShortName = items[item->type].getIdentifiedName();
					if ( scrollShortName.find(ItemTooltips.adjectives["scroll_prefixes"]["scroll_of"]) != std::string::npos )
					{
						scrollShortName = scrollShortName.substr(ItemTooltips.adjectives["scroll_prefixes"]["scroll_of"].size());
					}
					if ( scrollShortName.find(ItemTooltips.adjectives["scroll_prefixes"]["piece_of"]) != std::string::npos )
					{
						scrollShortName = scrollShortName.substr(ItemTooltips.adjectives["scroll_prefixes"]["piece_of"].size());
					}
					camelCaseString(scrollShortName);
					snprintf(buf, sizeof(buf), "%s (%+d)", scrollShortName.c_str(), item->beatitude);
					itemFxStatus->setText(buf);
				}
				else
				{
					itemFxStatus->setText("");
				}
			}

			itemFxStatus->setTextColor(hudColors.characterSheetLightNeutral);
		}
		else
		{
			itemFxTitle->setText("");
			itemFxStatus->setText("");
		}

		SDL_Rect textPos{ 0, 47 + heightOffsetCompact, baseFrame->getSize().w, 24 };
		itemFxTitle->setSize(textPos);
		textPos.y += 20;
		itemFxStatus->setSize(textPos);

		auto skillIcon = baseFrame->findImage("itemfx skill img");
		skillIcon->pos.y = 56 + heightOffsetCompact;
		for ( auto& skill : Player::SkillSheet_t::skillSheetData.skillEntries )
		{
			if ( skill.skillId == PRO_MAGIC )
			{
				if ( skillCapstoneUnlocked(playernum, skill.skillId) )
				{
					skillIcon->path = skill.skillIconPathLegend;
				}
				else
				{
					skillIcon->path = skill.skillIconPath;
				}
				skillIcon->disabled = false;
				break;
			}
		}

		auto itemIcon = baseFrame->findImage("itemfx item img");
		itemIcon->disabled = true;
		if ( parentGUI.itemEffectUsingSpell || parentGUI.itemEffectUsingSpellbook )
		{
			if ( parentGUI.itemEffectUsingSpell )
			{
				if ( parentGUI.itemEffectScrollItem && parentGUI.itemEffectScrollItem->type == SPELL_ITEM )
				{
					if ( spell_t* spell = getSpellFromItem(parentGUI.gui_player, parentGUI.itemEffectScrollItem, false) )
					{
						if ( node_t* spellImageNode = ItemTooltips.getSpellNodeFromSpellID(spell->ID) )
						{
							string_t* string = (string_t*)spellImageNode->element;
							if ( string )
							{
								itemIcon->path = "*";
								itemIcon->path += string->data;
								if ( auto imgGet = Image::get(itemIcon->path.c_str()) )
								{
									itemIcon->pos.w = imgGet->getWidth();
									itemIcon->pos.h = imgGet->getHeight();
									itemIcon->disabled = false;
									itemIcon->pos.x = 48 - itemIcon->pos.w / 2;
									itemIcon->pos.y = 68 + heightOffsetCompact - itemIcon->pos.h / 2;
									if ( itemIcon->pos.x % 2 == 1 )
									{
										++itemIcon->pos.x;
									}
									if ( itemIcon->pos.y % 2 == 1 )
									{
										++itemIcon->pos.y;
									}
								}
							}
						}
					}
				}
			}
			else if ( parentGUI.itemEffectUsingSpellbook && items[parentGUI.itemEffectItemType].category == SPELLBOOK )
			{
				if ( node_t* spellImageNode = 
					ItemTooltips.getSpellNodeFromSpellID(getSpellIDFromSpellbook(static_cast<ItemType>(parentGUI.itemEffectItemType))) )
				{
					string_t* string = (string_t*)spellImageNode->element;
					if ( string )
					{
						itemIcon->path = "*";
						itemIcon->path += string->data;
						if ( auto imgGet = Image::get(itemIcon->path.c_str()) )
						{
							itemIcon->pos.w = imgGet->getWidth();
							itemIcon->pos.h = imgGet->getHeight();
							itemIcon->disabled = false;
							itemIcon->pos.x = 48 - itemIcon->pos.w / 2;
							itemIcon->pos.y = 68 + heightOffsetCompact - itemIcon->pos.h / 2;
							if ( itemIcon->pos.x % 2 == 1 )
							{
								++itemIcon->pos.x;
							}
							if ( itemIcon->pos.y % 2 == 1 )
							{
								++itemIcon->pos.y;
							}
						}
					}
				}
			}
		}
		else
		{
			itemIcon->path = "*images/ui/ScrollSpells/Scroll_Icon_Scroll_00.png";
			if ( auto imgGet = Image::get(itemIcon->path.c_str()) )
			{
				itemIcon->pos.w = imgGet->getWidth();
				itemIcon->pos.h = imgGet->getHeight();
				itemIcon->disabled = false;
				itemIcon->pos.x = 48 - itemIcon->pos.w / 2;
				itemIcon->pos.y = 68 + heightOffsetCompact - itemIcon->pos.h / 2;
				if ( itemIcon->pos.x % 2 == 1 )
				{
					++itemIcon->pos.x;
				}
				if ( itemIcon->pos.y % 2 == 1 )
				{
					++itemIcon->pos.y;
				}
			}
		}
	}

	if ( itemActionType == ITEMFX_ACTION_OK )
	{
		animInvalidAction = 0.0;
		animInvalidActionTicks = 0;
	}
	else
	{
		// shaking feedback for invalid action
		// constant decay for animation
		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
		real_t setpointDiffX = fpsScale * 1.0 / 25.0;
		animInvalidAction -= setpointDiffX;
		animInvalidAction = std::max(0.0, animInvalidAction);
	}
	bool bInvalidActionAnimating = false;
	if ( animInvalidAction > 0.001 || (ticks - animInvalidActionTicks) < TICKS_PER_SECOND * .8 )
	{
		bInvalidActionAnimating = true;
	}

	bool usingGamepad = inputs.hasController(playernum) && !inputs.getVirtualMouse(playernum)->draw_cursor;

	{
		// close btn
		auto closeBtn = baseFrame->findButton("close itemfx button");
		SDL_Rect closeBtnPos = closeBtn->getSize();
		closeBtnPos.y = 34 + heightOffsetCompact;
		closeBtn->setSize(closeBtnPos);
		auto closeGlyph = baseFrame->findImage("close itemfx glyph");
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

	auto itemDisplayTooltip = baseFrame->findFrame("itemfx display tooltip");
	itemDisplayTooltip->setDisabled(false);

	auto actionPromptTxt = baseFrame->findField("action prompt txt");
	actionPromptTxt->setDisabled(false);
	auto actionPromptImg = baseFrame->findImage("action prompt glyph");
	//auto actionModifierImg = baseFrame->findImage("action modifier glyph");

	Uint32 negativeColor = hudColors.characterSheetRed;
	Uint32 neutralColor = hudColors.characterSheetLightNeutral;
	Uint32 positiveColor = hudColors.characterSheetGreen;
	Uint32 defaultPromptColor = makeColor(255, 255, 255, 255);

	auto displayItemName = itemDisplayTooltip->findField("item display name");
	auto displayItemTextImg = itemDisplayTooltip->findImage("item text img");
	auto itemSlotBg = itemDisplayTooltip->findImage("item bg img");
	itemSlotBg->pos.x = 12;
	itemSlotBg->pos.y = 12;
	const int displayItemTextImgBaseX = itemSlotBg->pos.x + itemSlotBg->pos.w;
	displayItemTextImg->pos.x = displayItemTextImgBaseX;
	displayItemTextImg->pos.y = itemSlotBg->pos.y + itemSlotBg->pos.h / 2 - displayItemTextImg->pos.h / 2;
	SDL_Rect displayItemNamePos{ displayItemTextImg->pos.x + 6, displayItemTextImg->pos.y - 4, 208, 24 };
	displayItemNamePos.h = 50;
	displayItemName->setSize(displayItemNamePos);
	static ConsoleVariable<int> cvar_itemfxPromptY("/itemfx_action_prompt_y", -2);
	SDL_Rect actionPromptTxtPos{ 0, 205 + *cvar_itemfxPromptY + heightOffsetCompact, baseFrame->getSize().w - 18 - 8, 24 };
	actionPromptTxt->setSize(actionPromptTxtPos);

	SDL_Rect tooltipPos = itemDisplayTooltip->getSize();
	tooltipPos.w = 298;
	tooltipPos.h = baseFrame->getSize().h - 100;
	tooltipPos.y = 118 + heightOffsetCompact;
	tooltipPos.x = 18 - (tooltipPos.w + 18) * (0.0/*1.0 - animTooltip*/);
	itemDisplayTooltip->setSize(tooltipPos);

	auto itemSlotFrame = itemDisplayTooltip->findFrame("item slot frame");
	bool modifierPressed = false;
	if ( usingGamepad && Input::inputs[playernum].binary("MenuPageLeftAlt") )
	{
		modifierPressed = true;
	}
	else if ( inputs.bPlayerUsingKeyboardControl(playernum)
		&& (keystatus[SDLK_LSHIFT] || keystatus[SDLK_RSHIFT]) )
	{
		modifierPressed = true;
	}

	if ( itemActionType != ITEMFX_ACTION_NONE && itemDesc.size() > 1 )
	{
		if ( isInteractable )
		{
			//const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
			//real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animTooltip)) / 2.0;
			//animTooltip += setpointDiffX;
			//animTooltip = std::min(1.0, animTooltip);
			animTooltip = 1.0;
			animTooltipTicks = ticks;
		}

		itemDisplayTooltip->setDisabled(false);

		{
			// prompt + glyph
			actionPromptTxt->setDisabled(false);
			if ( itemActionType == ITEMFX_ACTION_OK && isInteractable )
			{
				if ( usingGamepad )
				{
					actionPromptImg->path = Input::inputs[playernum].getGlyphPathForBinding("MenuConfirm");
					if ( modifierPressed )
					{
						//actionModifierImg->path = Input::inputs[playernum].getGlyphPathForBinding("MenuPageLeftAlt");
					}
				}
				else if ( !usingGamepad )
				{
					actionPromptImg->path = Input::inputs[playernum].getGlyphPathForBinding("MenuRightClick");
					if ( modifierPressed )
					{
						//actionModifierImg->path = GlyphHelper.getGlyphPath(SDLK_LSHIFT, false);
					}
				}
				if ( auto imgGet = Image::get(actionPromptImg->path.c_str()) )
				{
					actionPromptImg->pos.w = imgGet->getWidth();
					actionPromptImg->pos.h = imgGet->getHeight();
					actionPromptImg->disabled = false;
					/*if ( modifierPressed && parentGUI.tinkeringFilter == TINKER_FILTER_SALVAGEABLE )
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
					}*/
				}
				switch ( currentMode )
				{
					case ITEMFX_MODE_NONE:
						actionPromptTxt->setText("");
						break;
					case ITEMFX_MODE_SCROLL_REPAIR:
						actionPromptTxt->setText(Language::get(4202));
						break;
					case ITEMFX_MODE_SCROLL_CHARGING:
						actionPromptTxt->setText(Language::get(4206));
						break;
					case ITEMFX_MODE_SCROLL_IDENTIFY:
						actionPromptTxt->setText(Language::get(4208));
						break;
					case ITEMFX_MODE_SCROLL_ENCHANT_ARMOR:
						actionPromptTxt->setText(Language::get(6305));
						break;
					case ITEMFX_MODE_SCROLL_ENCHANT_WEAPON:
						actionPromptTxt->setText(Language::get(6304));
						break;
					case ITEMFX_MODE_SPELL_IDENTIFY:
						actionPromptTxt->setText(Language::get(4208));
						break;
					case ITEMFX_MODE_SCROLL_REMOVECURSE:
						actionPromptTxt->setText(Language::get(4204));
						break;
					case ITEMFX_MODE_SPELL_REMOVECURSE:
						actionPromptTxt->setText(Language::get(4204));
						break;
					default:
						actionPromptTxt->setText("");
						break;
				}
				actionPromptTxt->setColor(defaultPromptColor);
			}
			else
			{
				actionPromptTxt->setText("");
				actionPromptImg->disabled = true;
				//actionModifierImg->disabled = true;
				if ( isInteractable )
				{
					switch ( itemActionType )
					{
						case ITEMFX_ACTION_INVALID_ITEM:
							actionPromptTxt->setText(Language::get(4210));
							break;
						case ITEMFX_ACTION_ITEM_FULLY_REPAIRED:
							actionPromptTxt->setText(Language::get(4136));
							break;
						case ITEMFX_ACTION_ITEM_FULLY_CHARGED:
							actionPromptTxt->setText(Language::get(4211));
							break;
						case ITEMFX_ACTION_ITEM_IDENTIFIED:
							actionPromptTxt->setText(Language::get(4212));
							break;
						case ITEMFX_ACTION_MUST_BE_UNEQUIPPED:
							actionPromptTxt->setText(Language::get(4136));
							break;
						case ITEMFX_ACTION_NOT_IDENTIFIED_YET:
							actionPromptTxt->setText(Language::get(4153));
							break;
						case ITEMFX_ACTION_NOT_CURSED:
							actionPromptTxt->setText(Language::get(4213));
							break;
						default:
							actionPromptTxt->setText("-");
							break;
					}
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
				//actionModifierImg->pos.x = actionPromptImg->pos.x - 4 - actionModifierImg->pos.w;
				//actionModifierImg->pos.y = actionPromptTxtPos.y + actionPromptTxtPos.h / 2 - actionModifierImg->pos.h / 2;
				//if ( actionModifierImg->pos.y % 2 == 1 )
				//{
				//	actionModifierImg->pos.y -= 1;
				//}
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
	}
	else
	{
		if ( (!usingGamepad && (ticks - animTooltipTicks > TICKS_PER_SECOND / 3))
			|| (usingGamepad)
			|| animTooltip < 0.9999 )
		{
			const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.01, (animTooltip)) / 2.0;
			animTooltip -= setpointDiffX;
			animTooltip = std::max(0.0, animTooltip);
		}
	}

	auto actionPromptUnselectedTxt = baseFrame->findField("action prompt unselected txt");
	auto actionPromptCoverLeftImg = baseFrame->findImage("action prompt lcover");
	auto actionPromptCoverRightImg = baseFrame->findImage("action prompt rcover");
	actionPromptCoverLeftImg->pos.x = 0;
	actionPromptCoverRightImg->pos.x = baseFrame->getSize().w - actionPromptCoverLeftImg->pos.w;
	actionPromptCoverLeftImg->pos.y = 90 + heightOffsetCompact;
	actionPromptCoverRightImg->pos.y = 90 + heightOffsetCompact;

	{
		actionPromptUnselectedTxt->setDisabled(false);
		actionPromptUnselectedTxt->setColor(makeColor(224, 224, 224, 255));
		switch ( currentMode )
		{
			case ITEMFX_MODE_NONE:
				actionPromptUnselectedTxt->setText("");
				break;
			case ITEMFX_MODE_SCROLL_REPAIR:
				actionPromptUnselectedTxt->setText(Language::get(4151));
				break;
			case ITEMFX_MODE_SCROLL_CHARGING:
				actionPromptUnselectedTxt->setText(Language::get(4207));
				break;
			case ITEMFX_MODE_SCROLL_IDENTIFY:
				actionPromptUnselectedTxt->setText(Language::get(4209));
				break;
			case ITEMFX_MODE_SCROLL_ENCHANT_ARMOR:
				actionPromptUnselectedTxt->setText(Language::get(6306));
				break;
			case ITEMFX_MODE_SCROLL_ENCHANT_WEAPON:
				actionPromptUnselectedTxt->setText(Language::get(6306));
				break;
			case ITEMFX_MODE_SCROLL_REMOVECURSE:
				actionPromptUnselectedTxt->setText(Language::get(4205));
				break;
			case ITEMFX_MODE_SPELL_IDENTIFY:
				actionPromptUnselectedTxt->setText(Language::get(4209));
				break;
			case ITEMFX_MODE_SPELL_REMOVECURSE:
				actionPromptUnselectedTxt->setText(Language::get(4205));
				break;
			default:
				actionPromptUnselectedTxt->setText("");
				break;
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
			pos.y = 93 + heightOffsetCompact;
			actionPromptUnselectedTxt->setSize(pos);
		}

		if ( ticks - animPromptTicks > TICKS_PER_SECOND / 10 )
		{
			const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
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
		//actionModifierImg->color = actionPromptImg->color;
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

	//itemDisplayTooltip->setOpacity(100.0 * animTooltip);
	itemSlotFrame->setOpacity(100.0 * animTooltip);

	bool activateSelection = false;
	if ( isInteractable )
	{
		if ( !inputs.getUIInteraction(playernum)->selectedItem
			&& !player->GUI.isDropdownActive()
			&& player->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_ITEMEFFECTGUI)
			&& player->bControlEnabled && !gamePaused
			&& !player->usingCommand() )
		{
			if ( Input::inputs[playernum].binaryToggle("MenuCancel") )
			{
				Input::inputs[playernum].consumeBinaryToggle("MenuCancel");
				parentGUI.closeGUI();
				Player::soundCancel();
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
			}
		}
	}

	if ( activateSelection && players[playernum] && players[playernum]->entity )
	{
		node_t* nextnode = nullptr;
		list_t* player_inventory = &stats[playernum]->inventory;
		bool foundItem = false;
		bool checkConsumedItem = false;
		bool itemActionOK = itemActionType == ITEMFX_ACTION_OK;
		if ( player_inventory )
		{
			for ( node_t* node = player_inventory->first; node != NULL; node = nextnode )
			{
				nextnode = node->next;
				if ( node->element )
				{
					Item* item = (Item*)node->element;
					if ( isItemSelectedToEffect(item) )
					{
						foundItem = true;
						if ( itemCategory(item) == SPELLBOOK )
						{
							//repairingSpellbook = true;
						}
						if ( itemActionOK )
						{
							parentGUI.executeOnItemClick(item);
						}
						break;
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
			
			if ( !itemActionOK )
			{
				invalidActionType = INVALID_ACTION_SHAKE_PROMPT;
				animInvalidAction = 1.0;
				animInvalidActionTicks = ticks;
				// play bad feedback sfx
				playSound(90, 64);
			}
			else
			{
				if ( !parentGUI.itemEffectScrollItem )
				{
					// immediately fade the tooltip on mouse control
					animTooltipTicks = 0;
				}
			}
		}
	}
}

bool GenericGUIMenu::ItemEffectGUI_t::isItemEffectMenuActive() const
{
	if ( !parentGUI.isGUIOpen() || !ItemEffectHasBeenCreated() )
	{
		return false;
	}
	if ( bOpen && parentGUI.guiType == GUICurrentType::GUI_TYPE_ITEMFX )
	{
		return true;
	}
	return false;
}

void GenericGUIMenu::ItemEffectGUI_t::createItemEffectMenu()
{
	const int player = parentGUI.getPlayer();
	if ( !gui || !itemEffectFrame || !players[player]->inventoryUI.frame )
	{
		return;
	}
	if ( ItemEffectHasBeenCreated() )
	{
		return;
	}

	SDL_Rect basePos{ 0, 0, itemEffectBaseWidth, 242 };
	{
		auto bgFrame = itemEffectFrame->addFrame("itemfx base");
		bgFrame->setSize(basePos);
		bgFrame->setHollow(false);
		bgFrame->setDisabled(true);
		auto bg = bgFrame->addImage(SDL_Rect{ 0, 0, basePos.w, basePos.h },
			makeColor(255, 255, 255, 255),
			"*images/ui/ScrollSpells/Scroll_Window_00.png", "itemfx base img");

		auto skillIcon = bgFrame->addImage(SDL_Rect{ 270, 36, 24, 24 },
			makeColor(255, 255, 255, 255),
			"", "itemfx skill img");
		skillIcon->disabled = true;

		auto itemIcon = bgFrame->addImage(SDL_Rect{ 270, 36, 24, 24 },
			makeColor(255, 255, 255, 255),
			"", "itemfx item img");
		itemIcon->disabled = true;

		auto headerFont = "fonts/pixel_maz_multiline.ttf#16#2";
		auto itemFxTitle = bgFrame->addField("itemfx title", 128);
		itemFxTitle->setFont(headerFont);
		itemFxTitle->setText("");
		itemFxTitle->setHJustify(Field::justify_t::CENTER);
		itemFxTitle->setVJustify(Field::justify_t::TOP);
		itemFxTitle->setSize(SDL_Rect{ 0, 0, 0, 0 });
		itemFxTitle->setTextColor(hudColors.characterSheetLightNeutral);
		itemFxTitle->setOutlineColor(makeColor(29, 16, 11, 255));
		auto itemFxStatus = bgFrame->addField("itemfx status", 128);
		itemFxStatus->setFont(headerFont);
		itemFxStatus->setText("");
		itemFxStatus->setHJustify(Field::justify_t::CENTER);
		itemFxStatus->setVJustify(Field::justify_t::TOP);
		itemFxStatus->setSize(SDL_Rect{ 0, 0, 0, 0 });
		itemFxStatus->setTextColor(hudColors.characterSheetLightNeutral);
		itemFxStatus->setOutlineColor(makeColor(29, 16, 11, 255));

		auto itemFont = "fonts/pixel_maz_multiline.ttf#16#2";
		auto itemDisplayTooltip = bgFrame->addFrame("itemfx display tooltip");
		itemDisplayTooltip->setSize(SDL_Rect{ 0, 0, 298, 108 });
		itemDisplayTooltip->setHollow(true);
		itemDisplayTooltip->setInheritParentFrameOpacity(false);
		{
			auto itemNameText = itemDisplayTooltip->addField("item display name", 1024);
			itemNameText->setFont(itemFont);
			itemNameText->setText("");
			itemNameText->setHJustify(Field::justify_t::LEFT);
			itemNameText->setVJustify(Field::justify_t::TOP);
			itemNameText->setSize(SDL_Rect{ 0, 0, 0, 0 });
			itemNameText->setColor(hudColors.characterSheetLightNeutral);

			auto itemDisplayTextBg = itemDisplayTooltip->addImage(SDL_Rect{ 0, 0, 220, 42 },
				0xFFFFFFFF, "*images/ui/ScrollSpells/Scroll_LabelName_2Row_00.png", "item text img");

			auto itemBgImg = itemDisplayTooltip->addImage(SDL_Rect{ 0, 0, 54, 54 }, 0xFFFFFFFF,
				"*images/ui/ScrollSpells/Scroll_ItemBGSurround_00.png", "item bg img");

			auto slotFrame = itemDisplayTooltip->addFrame("item slot frame");
			SDL_Rect slotPos{ 0, 0, players[player]->inventoryUI.getSlotSize(), players[player]->inventoryUI.getSlotSize() };
			slotFrame->setSize(slotPos);
			slotFrame->setDisabled(true);
			slotFrame->setInheritParentFrameOpacity(false);
			createPlayerInventorySlotFrameElements(slotFrame);
		}

		{
			auto closeBtn = bgFrame->addButton("close itemfx button");
			SDL_Rect closeBtnPos{ basePos.w - 4 - 26, 14, 26, 26 };
			closeBtn->setSize(closeBtnPos);
			closeBtn->setColor(makeColor(255, 255, 255, 255));
			closeBtn->setHighlightColor(makeColor(255, 255, 255, 255));
			closeBtn->setText("X");
			closeBtn->setFont(itemFont);
			closeBtn->setHideGlyphs(true);
			closeBtn->setHideKeyboardGlyphs(true);
			closeBtn->setHideSelectors(true);
			closeBtn->setMenuConfirmControlType(0);
			closeBtn->setBackground("*images/ui/ScrollSpells/Button_X_00.png");
			closeBtn->setBackgroundHighlighted("*images/ui/ScrollSpells/Button_XHigh_00.png");
			closeBtn->setBackgroundActivated("*images/ui/ScrollSpells/Button_XPress_00.png");
			closeBtn->setTextHighlightColor(makeColor(201, 162, 100, 255));
			closeBtn->setCallback([](Button& button) {
				GenericGUI[button.getOwner()].closeGUI();
				Player::soundCancel();
			});
			closeBtn->setTickCallback(genericgui_deselect_fn);

			auto closeGlyph = bgFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
				0xFFFFFFFF, "", "close itemfx glyph");
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

			auto actionPromptCoverLeftImg = bgFrame->addImage(SDL_Rect{ 0, 70, 56, 26 },
				0xFFFFFFFF, "*images/ui/ScrollSpells/Scroll_PromptCoverLeft_00.png", "action prompt lcover");
			actionPromptCoverLeftImg->ontop = true;
			auto actionPromptCoverRightImg = bgFrame->addImage(SDL_Rect{ bg->pos.w - 56, 70, 56, 26 },
				0xFFFFFFFF, "*images/ui/ScrollSpells/Scroll_PromptCoverRight_00.png", "action prompt rcover");
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
	}
}

void CalloutRadialMenu::loadCalloutJSON()
{
	if ( !PHYSFS_getRealDir("/data/callout_wheel.json") )
	{
		printlog("[JSON]: Error: Could not find file: data/callout_wheel.json");
	}
	else
	{
		std::string inputPath = PHYSFS_getRealDir("/data/callout_wheel.json");
		inputPath.append("/data/callout_wheel.json");

		File* fp = FileIO::open(inputPath.c_str(), "rb");
		if ( !fp )
		{
			printlog("[JSON]: Error: Could not open json file %s", inputPath.c_str());
		}
		else
		{
			char buf[65536];
			int count = fp->read(buf, sizeof(buf[0]), sizeof(buf));
			buf[count] = '\0';
			rapidjson::StringStream is(buf);
			FileIO::close(fp);
			rapidjson::Document d;
			d.ParseStream(is);
			if ( !d.HasMember("version") )
			{
				printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			}
			else
			{
				if ( d.HasMember("panel_center_x_offset") )
				{
					CalloutRadialMenu::followerWheelFrameOffsetX = d["panel_center_x_offset"].GetInt();
				}
				if ( d.HasMember("panel_center_y_offset") )
				{
					CalloutRadialMenu::followerWheelFrameOffsetY = d["panel_center_y_offset"].GetInt();
				}
				if ( d.HasMember("panel_radius") )
				{
					CalloutRadialMenu::followerWheelRadius = d["panel_radius"].GetInt();
				}
				if ( d.HasMember("panel_button_thickness") )
				{
					CalloutRadialMenu::followerWheelButtonThickness = d["panel_button_thickness"].GetInt();
				}
				if ( d.HasMember("panel_inner_circle_radius_offset") )
				{
					CalloutRadialMenu::followerWheelInnerCircleRadiusOffset = d["panel_inner_circle_radius_offset"].GetInt();
				}
				if ( d.HasMember("colors") )
				{
					if ( d["colors"].HasMember("banner_default") )
					{
						followerBannerTextColor = makeColor(
							d["colors"]["banner_default"]["r"].GetInt(),
							d["colors"]["banner_default"]["g"].GetInt(),
							d["colors"]["banner_default"]["b"].GetInt(),
							d["colors"]["banner_default"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("banner_highlight_default") )
					{
						followerBannerTextHighlightColor = makeColor(
							d["colors"]["banner_highlight_default"]["r"].GetInt(),
							d["colors"]["banner_highlight_default"]["g"].GetInt(),
							d["colors"]["banner_highlight_default"]["b"].GetInt(),
							d["colors"]["banner_highlight_default"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("title") )
					{
						followerTitleColor = makeColor(
							d["colors"]["title"]["r"].GetInt(),
							d["colors"]["title"]["g"].GetInt(),
							d["colors"]["title"]["b"].GetInt(),
							d["colors"]["title"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("title_creature_highlight") )
					{
						followerTitleHighlightColor = makeColor(
							d["colors"]["title_creature_highlight"]["r"].GetInt(),
							d["colors"]["title_creature_highlight"]["g"].GetInt(),
							d["colors"]["title_creature_highlight"]["b"].GetInt(),
							d["colors"]["title_creature_highlight"]["a"].GetInt());
					}
				}
				if ( d.HasMember("panels") )
				{
					CalloutRadialMenu::panelEntries.clear();
					for ( rapidjson::Value::ConstValueIterator itr = d["panels"].Begin();
						itr != d["panels"].End(); ++itr )
					{
						CalloutRadialMenu::panelEntries.push_back(CalloutRadialMenu::PanelEntry());
						auto& entry = CalloutRadialMenu::panelEntries[CalloutRadialMenu::panelEntries.size() - 1];
						if ( (*itr).HasMember("x") )
						{
							entry.x = (*itr)["x"].GetInt();
						}
						if ( (*itr).HasMember("y") )
						{
							entry.y = (*itr)["y"].GetInt();
						}
						if ( (*itr).HasMember("path") )
						{
							entry.path = (*itr)["path"].GetString();
						}
						if ( (*itr).HasMember("path_hover") )
						{
							entry.path_hover = (*itr)["path_hover"].GetString();
						}
						if ( (*itr).HasMember("icon_offset_x") )
						{
							entry.icon_offsetx = (*itr)["icon_offset_x"].GetInt();
						}
						if ( (*itr).HasMember("icon_offset_y") )
						{
							entry.icon_offsety = (*itr)["icon_offset_y"].GetInt();
						}
					}
				}
				if ( d.HasMember("help_strings") )
				{
					CalloutRadialMenu::helpDescriptors.clear();
					for ( rapidjson::Value::ConstMemberIterator itr = d["help_strings"].MemberBegin();
						itr != d["help_strings"].MemberEnd(); ++itr )
					{
						CalloutRadialMenu::helpDescriptors[itr->name.GetString()] = itr->value.GetString();
					}
				}
				if ( d.HasMember("world_icons") )
				{
					CalloutRadialMenu::worldIconEntries.clear();
					CalloutRadialMenu::worldIconIDToEntryKey.clear();
					int id = 0;
					for ( rapidjson::Value::ConstMemberIterator itr = d["world_icons"].MemberBegin();
						itr != d["world_icons"].MemberEnd(); ++itr )
					{
						std::string key = (*itr).name.GetString();
						auto& entry = worldIconEntries[key];

						std::string basePath = "*images/ui/CalloutWheel/WorldIcons/";
						entry.pathDefault = basePath + (*itr).value["default"].GetString();
						entry.pathPlayer1 = basePath + (*itr).value["0"].GetString();
						entry.pathPlayer2 = basePath + (*itr).value["1"].GetString();
						entry.pathPlayer3 = basePath + (*itr).value["2"].GetString();
						entry.pathPlayer4 = basePath + (*itr).value["3"].GetString();
						entry.pathPlayerX = basePath + (*itr).value["4"].GetString();
						entry.id = id;
						CalloutRadialMenu::worldIconIDToEntryKey[id] = key;
						++id;
						/*if ( auto img = Image::get(entry.pathDefault.c_str()) )
						{
						}
						if ( auto img = Image::get(entry.pathPlayer1.c_str()) )
						{
						}
						if ( auto img = Image::get(entry.pathPlayer2.c_str()) )
						{
						}
						if ( auto img = Image::get(entry.pathPlayer3.c_str()) )
						{
						}
						if ( auto img = Image::get(entry.pathPlayer4.c_str()) )
						{
						}
						if ( auto img = Image::get(entry.pathPlayerX.c_str()) )
						{
						}*/
					}
				}
				if ( d.HasMember("icons") )
				{
					CalloutRadialMenu::iconEntries.clear();
					for ( rapidjson::Value::ConstValueIterator itr = d["icons"].Begin();
						itr != d["icons"].End(); ++itr )
					{
						std::string actionName = "";
						if ( (*itr).HasMember("action") )
						{
							actionName = (*itr)["action"].GetString();
						}
						if ( actionName == "" )
						{
							continue;
						}
						CalloutRadialMenu::iconEntries[actionName] = CalloutRadialMenu::IconEntry();
						CalloutRadialMenu::iconEntries[actionName].name = actionName;
						if ( (*itr).HasMember("id") )
						{
							CalloutRadialMenu::iconEntries[actionName].id = (*itr)["id"].GetInt();
						}
						if ( (*itr).HasMember("path") )
						{
							CalloutRadialMenu::iconEntries[actionName].path = (*itr)["path"].GetString();
						}
						if ( (*itr).HasMember("path_active") )
						{
							CalloutRadialMenu::iconEntries[actionName].path_active = (*itr)["path_active"].GetString();
						}
						if ( (*itr).HasMember("path_hover") )
						{
							CalloutRadialMenu::iconEntries[actionName].path_hover = (*itr)["path_hover"].GetString();
						}
						if ( (*itr).HasMember("path_active_hover") )
						{
							CalloutRadialMenu::iconEntries[actionName].path_active_hover = (*itr)["path_active_hover"].GetString();
						}
						if ( (*itr).HasMember("text_maps") )
						{
							for ( rapidjson::Value::ConstValueIterator itr2 = (*itr)["text_maps"].Begin();
								itr2 != (*itr)["text_maps"].End(); ++itr2 )
							{
								for ( rapidjson::Value::ConstMemberIterator itr3 = itr2->MemberBegin();
									itr3 != itr2->MemberEnd(); ++itr3 )
								{
									std::string mapKey = itr3->name.GetString();
									std::string mapText = itr3->value["text"].GetString();
									std::set<int> mapHighlights;
									for ( rapidjson::Value::ConstValueIterator highlightItr = itr3->value["word_highlights"].Begin();
										highlightItr != itr3->value["word_highlights"].End(); ++highlightItr )
									{
										mapHighlights.insert(highlightItr->GetInt());
									}
									std::string worldMsg = "";
									std::string worldMsgSays = "";
									std::string worldMsgEmote = "";
									std::string worldMsgEmoteYou = "";
									std::string worldMsgEmoteToYou = "";
									std::string worldIcon = "";
									std::string worldIconMini = "";
									if ( itr3->value.HasMember("msg") )
									{
										worldMsg = itr3->value["msg"].GetString();
									}
									if ( itr3->value.HasMember("msg_says") )
									{
										worldMsgSays = itr3->value["msg_says"].GetString();
									}
									if ( itr3->value.HasMember("msg_emote") )
									{
										worldMsgEmote = itr3->value["msg_emote"].GetString();
									}
									if ( itr3->value.HasMember("msg_emote_you") )
									{
										worldMsgEmoteYou = itr3->value["msg_emote_you"].GetString();
									}
									if ( itr3->value.HasMember("msg_emote_to_you") )
									{
										worldMsgEmoteToYou = itr3->value["msg_emote_to_you"].GetString();
									}
									if ( itr3->value.HasMember("world_icon") )
									{
										worldIcon = itr3->value["world_icon"].GetString();
									}
									if ( itr3->value.HasMember("world_icon_small") )
									{
										worldIconMini = itr3->value["world_icon_small"].GetString();
									}
									CalloutRadialMenu::iconEntries[actionName].text_map[mapKey] = CalloutRadialMenu::IconEntry::IconEntryText_t();
									auto& entry = CalloutRadialMenu::iconEntries[actionName].text_map[mapKey];
									entry.bannerText = mapText;
									entry.bannerHighlights = mapHighlights;
									entry.worldMsg = worldMsg;
									entry.worldMsgSays = worldMsgSays;
									entry.worldMsgEmote = worldMsgEmote;
									entry.worldMsgEmoteYou = worldMsgEmoteYou;
									entry.worldMsgEmoteToYou = worldMsgEmoteToYou;
									entry.worldIconTag = worldIcon;
									entry.worldIconTagMini = worldIconMini;

									if ( worldIcon != "" )
									{
										assert(worldIconEntries.find(worldIcon) != worldIconEntries.end());
									}
									if ( worldIconMini != "" )
									{
										assert(worldIconEntries.find(worldIconMini) != worldIconEntries.end());
									}
								}
							}
						}
					}
				}
				printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
			}
		}
	}
}

void setCalloutBannerTextFormatted(const int player, Field* field, Uint32 color, std::set<int>& highlights, char const* const text, ...)
{
	if ( !field ) { return; }

	char buf[256] = "";
	va_list argptr;
	va_start(argptr, text);
	vsnprintf(buf, sizeof(buf), text, argptr);
	va_end(argptr);

	field->setText(buf);
	field->clearWordsToHighlight();
	for ( auto v : highlights )
	{
		field->addWordToHighlight(v, color);
	}
}

void setCalloutBannerTextUnformatted(const int player, Field* field, const char* iconName, const char* textKey, Uint32 color)
{
	if ( !field ) { return; }
	if ( CalloutMenu[player].iconEntries.find(iconName) == CalloutMenu[player].iconEntries.end() )
	{
		return;
	}
	auto& textMap = CalloutMenu[player].iconEntries[iconName].text_map[textKey];
	field->setText(textMap.bannerText.c_str());
	field->clearWordsToHighlight();
	for ( auto v : textMap.bannerHighlights )
	{
		field->addWordToHighlight(v, color);
	}
}

std::string CalloutRadialMenu::getCalloutMessage(const IconEntry::IconEntryText_t& text_map, const char* object, const int targetPlayer)
{
	if ( text_map.worldMsgEmote != "" )
	{
		char buf[512] = "";
		if ( object )
		{
			if ( targetPlayer >= 0 && getPlayer() == targetPlayer && text_map.worldMsgEmoteYou != "" )
			{
				// messaging the player that owns the callout "you gesture to"
				snprintf(buf, sizeof(buf), text_map.worldMsgEmoteYou.c_str(), object);
			}
			else
			{
				char shortname[32];
				stringCopy(shortname, stats[getPlayer()]->name, sizeof(shortname), 22);
				std::string nameStr = shortname;
				nameStr = messageSanitizePercentSign(nameStr, nullptr);
				snprintf(buf, sizeof(buf), text_map.worldMsgEmote.c_str(), nameStr.c_str(), object);
			}
		}
		else
		{
			if ( targetPlayer >= 0 && getPlayer() == targetPlayer && text_map.worldMsgEmoteYou != "" )
			{
				// messaging the player that owns the callout "you gesture to"
				snprintf(buf, sizeof(buf), text_map.worldMsgEmoteYou.c_str());
			}
			else
			{
				char shortname[32];
				stringCopy(shortname, stats[getPlayer()]->name, sizeof(shortname), 22);
				std::string nameStr = shortname;
				nameStr = messageSanitizePercentSign(nameStr, nullptr);
				snprintf(buf, sizeof(buf), text_map.worldMsgEmote.c_str(), nameStr.c_str());
			}
		}
		return buf;
	}
	else if ( text_map.worldMsgSays != "" )
	{
		char buf[512] = "";
		if ( object )
		{
			if ( targetPlayer >= 0 && getPlayer() == targetPlayer )
			{
				// messaging the player that owns the callout "you say:"
				snprintf(buf, sizeof(buf), text_map.worldMsgSays.c_str(), Language::get(739), object);
			}
			else
			{
				char shortname[32];
				stringCopy(shortname, stats[getPlayer()]->name, sizeof(shortname), 22);
				std::string playerSays = shortname;
				playerSays = messageSanitizePercentSign(playerSays, nullptr);
				playerSays += ": ";
				snprintf(buf, sizeof(buf), text_map.worldMsgSays.c_str(), playerSays.c_str(), object);
			}
		}
		else
		{
			if ( targetPlayer >= 0 && getPlayer() == targetPlayer )
			{
				// messaging the player that owns the callout "you say:"
				snprintf(buf, sizeof(buf), text_map.worldMsgSays.c_str(), Language::get(739));
			}
			else
			{
				char shortname[32];
				stringCopy(shortname, stats[getPlayer()]->name, sizeof(shortname), 22);
				std::string playerSays = shortname;
				playerSays = messageSanitizePercentSign(playerSays, nullptr);
				playerSays += ": ";
				snprintf(buf, sizeof(buf), text_map.worldMsgSays.c_str(), playerSays.c_str());
			}
		}
		return buf;
	}
	else
	{
		char buf[512] = "";
		if ( object )
		{
			snprintf(buf, sizeof(buf), text_map.worldMsg.c_str(), object);
			return buf;
		}
		else
		{
			return text_map.worldMsg;
		}
	}
}

std::string CalloutRadialMenu::setCalloutText(Field* field, const char* iconName, Uint32 color,
	CalloutRadialMenu::CalloutCommand cmd, SetCalloutTextTypes setType, const int targetPlayer)
{
	if ( !field && setType == SET_CALLOUT_BANNER_TEXT ) { return ""; }
	auto findIcon = CalloutRadialMenu::iconEntries.find(iconName);
	if ( findIcon == CalloutRadialMenu::iconEntries.end() )
	{
		return "";
	}
	std::string key = "default";
	Entity* entity = uidToEntity(lockOnEntityUid);
	const int player = getPlayer();

	Entity* playerEntity = Player::getPlayerInteractEntity(player);

	if ( players[player]->isLocalPlayer() )
	{
		if ( lockOnEntityUid == 0 )
		{
			if ( cmd == CALLOUT_CMD_AFFIRMATIVE 
				|| cmd == CALLOUT_CMD_NEGATIVE )
			{
				entity = playerEntity;
			}
			else if ( cmd == CALLOUT_CMD_HELP )
			{
				entity = playerEntity;
			}
		}

		if ( cmd == CALLOUT_CMD_SOUTH
			|| cmd == CALLOUT_CMD_SOUTHWEST
			|| cmd == CALLOUT_CMD_SOUTHEAST )
		{
			int toPlayer = getPlayerForDirectPlayerCmd(getPlayer(), cmd);
			Entity* toPlayerEntity = Player::getPlayerInteractEntity(toPlayer);
			if ( toPlayer >= 0 && toPlayer < MAXPLAYERS
				&& toPlayerEntity && !client_disconnected[toPlayer] )
			{
				entity = toPlayerEntity;
			}
		}
	}

	if ( cmd == CALLOUT_CMD_MOVE )
	{
		if ( setType == SET_CALLOUT_BANNER_TEXT )
		{
			setCalloutBannerTextUnformatted(player, field, iconName, "default", color);
		}
		else if ( setType == SET_CALLOUT_ICON_KEY )
		{
			return "default";
		}
		else
		{
			return getCalloutMessage(findIcon->second.text_map["default"], nullptr, targetPlayer);
		}
		return "";
	}
	else if ( cmd == CALLOUT_CMD_SOUTH
		|| cmd == CALLOUT_CMD_SOUTHWEST
		|| cmd == CALLOUT_CMD_SOUTHEAST )
	{
		std::string targetPlayerName = "";
		
		int toPlayer = getPlayerForDirectPlayerCmd(getPlayer(), cmd);
		key = "player_wave";


		if ( toPlayer < 0 || toPlayer >= MAXPLAYERS 
			|| client_disconnected[toPlayer] )
		{
			key = "unavailable";
		}
		else
		{
			Entity* toPlayerEntity = Player::getPlayerInteractEntity(toPlayer);
			if ( !toPlayerEntity )
			{
				key = "unavailable";
			}
		}

		if ( setType == SET_CALLOUT_ICON_KEY )
		{
			return key;
		}

		if ( key != "unavailable" )
		{
			char shortname[32];
			stringCopy(shortname, stats[toPlayer]->name, sizeof(shortname), 22);
			targetPlayerName = shortname;
			targetPlayerName = messageSanitizePercentSign(targetPlayerName, nullptr);
		}

		auto& textMap = findIcon->second.text_map[key];
		auto highlights = textMap.bannerHighlights;
		if ( highlights.size() > 0 )
		{
			int indexStart = 0;
			for ( auto highlight : highlights )
			{
				indexStart = std::max(highlight, indexStart);
			}
			for ( auto c : targetPlayerName )
			{
				if ( c == ' ' )
				{
					highlights.insert(indexStart + 1);
					++indexStart;
				}
			}
		}
		if ( setType == SET_CALLOUT_BANNER_TEXT )
		{
			if ( key == "unavailable" )
			{
				color = hudColors.characterSheetRed;
			}
			setCalloutBannerTextFormatted(player, field, color, highlights, textMap.bannerText.c_str(), targetPlayerName.c_str());
		}
		else
		{
			if ( entity && entity->skill[2] == targetPlayer )
			{
				char shortname[32];
				stringCopy(shortname, stats[getPlayer()]->name, sizeof(shortname), 22);
				std::string nameStr = shortname;
				nameStr = messageSanitizePercentSign(nameStr, nullptr);
				char buf[128];
				snprintf(buf, sizeof(buf), textMap.worldMsgEmoteToYou.c_str(), nameStr.c_str());
				return buf;
			}
			else
			{
				return getCalloutMessage(textMap, targetPlayerName.c_str(), targetPlayer);
			}
		}
		return "";
	}

	auto calloutType = getCalloutTypeForEntity(player, entity);
	auto& text_map = findIcon->second.text_map;
	switch ( calloutType )
	{
	case CALLOUT_TYPE_NO_TARGET:
		key = "location";
		break;
	case CALLOUT_TYPE_PLAYER:
		if ( cmd == CALLOUT_CMD_HELP && entity && (entity->behavior == &actPlayer || entity->behavior == &actDeathGhost)
			&& player >= 0 && player < MAXPLAYERS && entity == playerEntity )
		{
			if ( players[player]->isLocalPlayer() )
			{
				clientCalloutHelpFlags = 0;
				auto& fx = StatusEffectQueue[player];
				EntityHungerIntervals hunger = EntityHungerIntervals::HUNGER_INTERVAL_OVERSATIATED;
				bool hungerBlood = false;
				for ( auto& eff : fx.effectQueue )
				{
					if ( eff.effect == StatusEffectQueue_t::kEffectBread || eff.effect == StatusEffectQueue_t::kEffectBloodHunger )
					{
						if ( eff.effect == StatusEffectQueue_t::kEffectBloodHunger )
						{
							hungerBlood = true;
						}
						if ( eff.customVariable <= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_STARVING) )
						{
							clientCalloutHelpFlags |= hungerBlood ? CALLOUT_HELP_BLOOD_STARVING : CALLOUT_HELP_FOOD_STARVING;
						}
						else if ( eff.customVariable <= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_WEAK) )
						{
							clientCalloutHelpFlags |= hungerBlood ? CALLOUT_HELP_BLOOD_WEAK : CALLOUT_HELP_FOOD_WEAK;
						}
						else if ( eff.customVariable <= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_HUNGRY) )
						{
							clientCalloutHelpFlags |= hungerBlood ? CALLOUT_HELP_BLOOD_HUNGRY : CALLOUT_HELP_FOOD_HUNGRY;
						}
					}
					else if ( eff.effect == StatusEffectQueue_t::kEffectAutomatonHunger )
					{
						if ( eff.customVariable <= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_AUTOMATON_CRITICAL) )
						{
							clientCalloutHelpFlags |= CALLOUT_HELP_STEAM_CRITICAL;
						}
					}
					else if ( eff.effect >= 0 && eff.effect < NUMEFFECTS )
					{
						if ( stats[player]->statusEffectRemovedByCureAilment(eff.effect, players[player]->entity) )
						{
							clientCalloutHelpFlags |= CALLOUT_HELP_NEGATIVE_FX;
						}
					}
				}
				bool hpLow = stats[player]->HP < (3 * stats[player]->MAXHP / 5);
				bool hpCritical = stats[player]->HP < (1 * stats[player]->MAXHP / 4);
				clientCalloutHelpFlags |= hpLow ? CALLOUT_HELP_HP_LOW : 0;
				clientCalloutHelpFlags |= hpCritical ? CALLOUT_HELP_HP_CRITICAL : 0;
			}

			if ( clientCalloutHelpFlags )
			{
				key = "help_all_conditions";
				if ( setType == SET_CALLOUT_ICON_KEY )
				{
					if ( stats[player]->HP == 0 || !players[player]->entity )
					{
						key = "help_deceased";
					}
					else if ( clientCalloutHelpFlags & CALLOUT_HELP_BLOOD_STARVING )
					{
						key = "condition_blood_starving";
					}
					else if ( clientCalloutHelpFlags & CALLOUT_HELP_FOOD_STARVING )
					{
						key = "condition_food_starving";
					}
					else if ( (clientCalloutHelpFlags & CALLOUT_HELP_STEAM_CRITICAL) && (svFlags & SV_FLAG_HUNGER) )
					{
						key = "condition_steam_empty";
					}
					else if ( clientCalloutHelpFlags & CALLOUT_HELP_BLOOD_WEAK )
					{
						key = "condition_blood_weak";
					}
					else if ( clientCalloutHelpFlags & CALLOUT_HELP_FOOD_WEAK )
					{
						key = "condition_food_weak";
					}
					else if ( clientCalloutHelpFlags & CALLOUT_HELP_BLOOD_HUNGRY )
					{
						key = "condition_blood_hungry";
					}
					else if ( clientCalloutHelpFlags & CALLOUT_HELP_FOOD_HUNGRY )
					{
						key = "condition_food_hungry";
					}
					else if ( clientCalloutHelpFlags & CALLOUT_HELP_HP_CRITICAL )
					{
						key = "condition_heal_urgent";
					}
					else if ( clientCalloutHelpFlags & CALLOUT_HELP_HP_LOW )
					{
						key = "condition_heal";
					}
					else if ( (clientCalloutHelpFlags & CALLOUT_HELP_STEAM_CRITICAL) && !(svFlags & SV_FLAG_HUNGER) )
					{
						key = "condition_steam_empty";
					}
					else if ( clientCalloutHelpFlags & CALLOUT_HELP_NEGATIVE_FX )
					{
						key = "condition_cure_ailment";
					}
					return key;
				}

				if ( stats[player]->HP == 0 || !players[player]->entity )
				{
					key = "help_deceased";
				}

				auto& textMap = text_map[key];
				auto highlights = textMap.bannerHighlights;

				std::string helpText = "";

				// hunger stats
				{
					if ( clientCalloutHelpFlags & CALLOUT_HELP_BLOOD_STARVING )
					{
						helpText += helpDescriptors["starving_blood"];
					}
					else if ( clientCalloutHelpFlags & CALLOUT_HELP_FOOD_STARVING )
					{
						helpText += helpDescriptors["starving"];
					}
					else if ( (clientCalloutHelpFlags & CALLOUT_HELP_STEAM_CRITICAL) && (svFlags & SV_FLAG_HUNGER) )
					{
						helpText += helpDescriptors["empty_steam"];
					}
					else if ( clientCalloutHelpFlags & CALLOUT_HELP_BLOOD_WEAK )
					{
						helpText += helpDescriptors["very_hungry_blood"];
					}
					else if ( clientCalloutHelpFlags & CALLOUT_HELP_FOOD_WEAK )
					{
						helpText += helpDescriptors["very_hungry"];
					}
					else if ( clientCalloutHelpFlags & CALLOUT_HELP_BLOOD_HUNGRY )
					{
						helpText += helpDescriptors["hungry_blood"];
					}
					else if ( clientCalloutHelpFlags & CALLOUT_HELP_FOOD_HUNGRY )
					{
						helpText += helpDescriptors["hungry"];
					}
					else if ( (clientCalloutHelpFlags & CALLOUT_HELP_STEAM_CRITICAL) && !(svFlags & SV_FLAG_HUNGER) )
					{
						helpText += helpDescriptors["empty_steam"];
					}
				}

				// health stats
				{
					if ( clientCalloutHelpFlags & CALLOUT_HELP_HP_CRITICAL )
					{
						if ( helpText.size() > 1 )
						{
							helpText += helpDescriptors["separator"];
						}
						helpText += helpDescriptors["healing_urgent"];
					}
					else if ( clientCalloutHelpFlags & CALLOUT_HELP_HP_LOW )
					{
						if ( helpText.size() > 1 )
						{
							helpText += helpDescriptors["separator"];
						}
						helpText += helpDescriptors["healing"];
					}
				}

				// effects
				if ( clientCalloutHelpFlags & CALLOUT_HELP_NEGATIVE_FX )
				{
					int numEffectsAdded = 0;
					for ( int i = 0; i < NUMEFFECTS; ++i )
					{
						if ( numEffectsAdded >= 3 )
						{
							break;
						}
						if ( stats[player]->EFFECTS[i] 
							&& (stats[player]->statusEffectRemovedByCureAilment(i, players[player]->entity)
								|| i == EFF_WITHDRAWAL && stats[player]->EFFECTS_TIMERS[EFF_WITHDRAWAL] == -2) )
						{
							if ( StatusEffectQueue_t::StatusEffectDefinitions_t::allEffects[i].name == "" )
							{
								continue;
							}
							if ( helpText.size() > 1 )
							{
								helpText += helpDescriptors["separator"];
							}
							helpText += StatusEffectQueue_t::StatusEffectDefinitions_t::allEffects[i].name;
							++numEffectsAdded;
						}
					}
				}

				if ( stats[player]->HP == 0 || !players[player]->entity )
				{
					helpText = helpDescriptors["ghost"];
				}

				if ( setType == SET_CALLOUT_BANNER_TEXT )
				{
					setCalloutBannerTextFormatted(player, field, color, highlights, textMap.bannerText.c_str(), helpText.c_str());
				}
				else
				{
					return getCalloutMessage(textMap, helpText.c_str(), targetPlayer);
				}
				return "";
			}
		}
		else if ( cmd == CALLOUT_CMD_LOOK )
		{
			std::string targetPlayerName = "";
			if ( entity && (entity->behavior == &actPlayer || entity->behavior == &actDeathGhost) )
			{
				char shortname[32];
				stringCopy(shortname, stats[entity->skill[2]]->name, sizeof(shortname), 22);
				targetPlayerName = shortname;
				targetPlayerName = messageSanitizePercentSign(targetPlayerName, nullptr);
			}

			key = "player_wave";
			if ( setType == SET_CALLOUT_ICON_KEY )
			{
				return key;
			}

			auto& textMap = text_map[key];
			auto highlights = textMap.bannerHighlights;
			if ( highlights.size() > 0 )
			{
				int indexStart = 0;
				for ( auto highlight : highlights )
				{
					indexStart = std::max(highlight, indexStart);
				}
				for ( auto c : targetPlayerName )
				{
					if ( c == ' ' )
					{
						highlights.insert(indexStart + 1);
						++indexStart;
					}
				}
			}
			if ( setType == SET_CALLOUT_BANNER_TEXT )
			{
				setCalloutBannerTextFormatted(player, field, color, highlights, textMap.bannerText.c_str(), targetPlayerName.c_str());
			}
			else
			{
				if ( entity && entity->skill[2] == targetPlayer )
				{
					char shortname[32];
					stringCopy(shortname, stats[getPlayer()]->name, sizeof(shortname), 22);
					std::string nameStr = shortname;
					nameStr = messageSanitizePercentSign(nameStr, nullptr);
					char buf[128];
					snprintf(buf, sizeof(buf), textMap.worldMsgEmoteToYou.c_str(), nameStr.c_str());
					return buf;
				}
				else
				{
					return getCalloutMessage(textMap, targetPlayerName.c_str(), targetPlayer);
				}
			}
			return "";
		}
		else if ( (cmd == CALLOUT_CMD_AFFIRMATIVE && !(lockOnEntityUid == 0 || (entity && playerEntity == entity)))
			|| cmd == CALLOUT_CMD_THANKS )
		{
			std::string targetPlayerName = "";
			if ( entity && (entity->behavior == &actPlayer || entity->behavior == &actDeathGhost) )
			{
				char shortname[32];
				stringCopy(shortname, stats[entity->skill[2]]->name, sizeof(shortname), 22);
				targetPlayerName = shortname;
				targetPlayerName = messageSanitizePercentSign(targetPlayerName, nullptr);
			}

			if ( cmd == CALLOUT_CMD_THANKS )
			{
				key = "default";
			}
			else
			{
				key = "player_thanks";
			}
			if ( setType == SET_CALLOUT_ICON_KEY )
			{
				return key;
			}

			auto& textMap = text_map[key];
			auto highlights = textMap.bannerHighlights;
			if ( highlights.size() > 0 )
			{
				int indexStart = 0;
				for ( auto highlight : highlights )
				{
					indexStart = std::max(highlight, indexStart);
				}
				for ( auto c : targetPlayerName )
				{
					if ( c == ' ' )
					{
						highlights.insert(indexStart + 1);
						++indexStart;
					}
				}
			}
			if ( setType == SET_CALLOUT_BANNER_TEXT )
			{
				setCalloutBannerTextFormatted(player, field, color, highlights, textMap.bannerText.c_str(), targetPlayerName.c_str());
			}
			else
			{
				if ( entity && entity->skill[2] == targetPlayer )
				{
					char shortname[32];
					stringCopy(shortname, stats[getPlayer()]->name, sizeof(shortname), 22);
					std::string nameStr = shortname;
					nameStr = messageSanitizePercentSign(nameStr, nullptr);
					char buf[128];
					snprintf(buf, sizeof(buf), textMap.worldMsgEmoteToYou.c_str(), nameStr.c_str());
					return buf;
				}
				else
				{
					return getCalloutMessage(textMap, targetPlayerName.c_str(), targetPlayer);
				}
			}
			return "";
		}
		break;
	case CALLOUT_TYPE_NPC:
	case CALLOUT_TYPE_NPC_ENEMY:
	case CALLOUT_TYPE_NPC_PLAYERALLY:
		if ( entity->behavior == &actMonster )
		{
			int monsterType = entity->getMonsterTypeFromSprite();
			if ( monsterType >= NOTHING && monsterType < NUMMONSTERS )
			{
				std::string monsterName = getMonsterLocalizedName((Monster)monsterType);
				bool namedNPC = false;
				if ( multiplayer != CLIENT && monsterType != SHOPKEEPER )
				{
					if ( Stat* stats = entity->getStats() )
					{
						if ( monsterNameIsGeneric(*stats) )
						{
							monsterName = stats->name;
						}
						else if ( strcmp(stats->name, "") )
						{
							monsterName = stats->name;
							namedNPC = true;
						}
					}
				}

				std::string key = "npc";
				if ( calloutType == CALLOUT_TYPE_NPC_PLAYERALLY )
				{
					key = "npc_ally";
				}
				else if ( calloutType == CALLOUT_TYPE_NPC_ENEMY )
				{
					key = "npc_enemy";
				}

				if ( namedNPC
					&& text_map.find(std::string(key + "_named")) != text_map.end() )
				{
					key += "_named";
				}
				else if ( stringStartsWithVowel(monsterName)
					&& text_map.find(std::string(key + "_an")) != text_map.end() )
				{
					key += "_an";
				}

				if ( text_map.find(key) == text_map.end() )
				{
					key = "default";
				}

				if ( setType == SET_CALLOUT_ICON_KEY )
				{
					return key;
				}
				auto& textMap = text_map[key];
				auto highlights = textMap.bannerHighlights;
				if ( highlights.size() > 0 )
				{
					int indexStart = 0;
					for ( auto highlight : highlights )
					{
						indexStart = std::max(highlight, indexStart);
					}
					for ( auto c : monsterName )
					{
						if ( c == ' ' )
						{
							highlights.insert(indexStart + 1);
							++indexStart;
						}
					}
				}
				if ( setType == SET_CALLOUT_BANNER_TEXT )
				{
					setCalloutBannerTextFormatted(player, field, color, highlights, textMap.bannerText.c_str(), monsterName.c_str());
				}
				else
				{
					return getCalloutMessage(textMap, monsterName.c_str(), targetPlayer);
				}
				return "";
			}
		}
		break;
	case CALLOUT_TYPE_SWITCH_ON:
		key = "switch";
		if ( text_map.find(std::string(key + "_on")) != text_map.end() )
		{
			key += "_on";
		}
		break;
	case CALLOUT_TYPE_SWITCH_OFF:
		key = "switch";
		if ( text_map.find(std::string(key + "_off")) != text_map.end() )
		{
			key += "_off";
		}
		break;
	case CALLOUT_TYPE_SWITCH:
		key = "switch";
		break;
	case CALLOUT_TYPE_CHEST:
		key = "chest";
		break;
	case CALLOUT_TYPE_ITEM:
	{
		std::string itemName;
		if ( entity && (multiplayer != CLIENT || (multiplayer == CLIENT && entity->itemReceivedDetailsFromServer == 1)) )
		{
			if ( Item* item = newItemFromEntity(entity, true) )
			{
				if ( item->type >= WOODEN_SHIELD && item->type < NUMITEMS )
				{
					char buf[256];
					bool manuallyInsertedNewline = false;
					if ( !item->identified )
					{
						if ( itemCategory(item) == BOOK )
						{
							if ( setType == SET_CALLOUT_BANNER_TEXT )
							{
								snprintf(buf, sizeof(buf), "\"%s\" (?)", getBookLocalizedNameFromIndex(item->appearance% numbooks).c_str());
							}
							else
							{
								snprintf(buf, sizeof(buf), "%s %s\n\"%s\" (?)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(),
									Language::get(4214), getBookLocalizedNameFromIndex(item->appearance % numbooks).c_str());
								manuallyInsertedNewline = true;
							}
						}
						else if ( itemCategory(item) == SCROLL )
						{
							if ( setType == SET_CALLOUT_BANNER_TEXT )
							{
								snprintf(buf, sizeof(buf), "%s %s %s (?)",
									items[item->type].getUnidentifiedName(), Language::get(4215), item->getScrollLabel());
							}
							else
							{
								snprintf(buf, sizeof(buf), "%s %s\n%s %s (?)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(),
									items[item->type].getUnidentifiedName(), Language::get(4215), item->getScrollLabel());
								manuallyInsertedNewline = true;
							}
						}
						else
						{
							std::string name = item->getName();
							if ( setType == SET_CALLOUT_WORLD_TEXT && (name.find(' ') != std::string::npos) )
							{
								snprintf(buf, sizeof(buf), "%s\n%s (?)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), name.c_str());
							}
							else
							{
								snprintf(buf, sizeof(buf), "%s %s (?)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), name.c_str());
							}
						}
					}
					else
					{
						if ( item->type == TOOL_SENTRYBOT || item->type == TOOL_SPELLBOT || item->type == TOOL_DUMMYBOT
							|| item->type == TOOL_GYROBOT )
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
							std::string name = item->getName();
							if ( setType == SET_CALLOUT_WORLD_TEXT && (name.find(' ') != std::string::npos) )
							{
								std::vector<size_t> spaces;
								size_t find = name.find(' ');
								while ( find != std::string::npos )
								{
									spaces.push_back(find);
									find = name.find(' ', find + 1);
								}
								if ( spaces.size() == 1 )
								{
									snprintf(buf, sizeof(buf), "%s\n%s",
										ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), name.c_str());
								}
								else
								{
									size_t split = (spaces.size() / 2);
									if ( spaces.size() > split )
									{
										name.at(spaces[split]) = '\n';
									}
									snprintf(buf, sizeof(buf), "%s %s",
										ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), name.c_str());
								}
							}
							else
							{
								snprintf(buf, sizeof(buf), "%s %s", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), 
									name.c_str());
							}
						}
						else if ( itemCategory(item) == BOOK )
						{
							if ( setType == SET_CALLOUT_BANNER_TEXT )
							{
								snprintf(buf, sizeof(buf), "\"%s\" (%+d)", getBookLocalizedNameFromIndex(item->appearance % numbooks).c_str(),
									item->beatitude);
							}
							else
							{
								snprintf(buf, sizeof(buf), "%s %s\n\"%s\" (%+d)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(),
									Language::get(4214), getBookLocalizedNameFromIndex(item->appearance % numbooks).c_str(), item->beatitude); // brand new copy of
								manuallyInsertedNewline = true;
							}
						}
						else
						{
							std::string name = item->getName();
							if ( setType == SET_CALLOUT_WORLD_TEXT && (name.find(' ') != std::string::npos) )
							{
								std::vector<size_t> spaces;
								size_t find = name.find(' ');
								while ( find != std::string::npos )
								{
									spaces.push_back(find);
									find = name.find(' ', find + 1);
								}
								if ( spaces.size() == 1 )
								{
									snprintf(buf, sizeof(buf), "%s\n%s (%+d)", 
										ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), name.c_str(), item->beatitude);
								}
								else
								{
									size_t split = (spaces.size() / 2);
									if ( spaces.size() > split )
									{
										name.at(spaces[split]) = '\n';
									}
									snprintf(buf, sizeof(buf), "%s %s (%+d)",
										ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), name.c_str(), item->beatitude);
								}
							}
							else
							{
								snprintf(buf, sizeof(buf), "%s %s (%+d)", 
									ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), name.c_str(), item->beatitude);
							}
						}
					}
					itemName = buf;
					if ( itemName.size() > 0 && itemName[0] >= 'A' && itemName[0] <= 'Z' )
					{
						itemName[0] -= 'A' - 'a';
					}
				}
				else
				{
					itemName = Language::get(3634);
				}
				free(item);
			}
		}
		else
		{
			itemName = Language::get(3634);
		}
		std::string key = "item";
		if ( stringStartsWithVowel(itemName) && text_map.find(std::string(key + "_an")) != text_map.end() )
		{
			key += "_an";
		}

		if ( text_map.find(key) == text_map.end() )
		{
			key = "default";
		}
		if ( setType == SET_CALLOUT_ICON_KEY )
		{
			return key;
		}
		auto& textMap = text_map[key];
		auto highlights = textMap.bannerHighlights;
		if ( highlights.size() > 0 )
		{
			int indexStart = 0;
			for ( auto highlight : highlights )
			{
				indexStart = std::max(highlight, indexStart);
			}
			for ( auto c : itemName )
			{
				if ( c == ' ' )
				{
					highlights.insert(indexStart + 1);
					++indexStart;
				}
			}
		}
		if ( setType == SET_CALLOUT_BANNER_TEXT )
		{
			setCalloutBannerTextFormatted(player, field, color, highlights, textMap.bannerText.c_str(), itemName.c_str());
		}
		else
		{
			return getCalloutMessage(textMap, itemName.c_str(), targetPlayer);
		}
		return "";
	}
	case CALLOUT_TYPE_BOULDER:
		key = "boulder";
		break;
	case CALLOUT_TYPE_TRAP:
	{
		key = "trap";
		if ( text_map.find(key) == text_map.end() )
		{
			key = "default";
		}
		if ( setType == SET_CALLOUT_ICON_KEY )
		{
			return key;
		}
		std::string trapName = Language::get(4362);
		if ( entity )
		{
			if ( entity->behavior == &actBoulderTrapHole )
			{
				trapName = Language::get(4349);
			}
			else if ( entity->behavior == &actArrowTrap )
			{
				trapName = Language::get(4351);
			}
			else if ( entity->behavior == &actMagicTrap )
			{
				trapName = Language::get(4352);
			}
			else if ( entity->behavior == &actMagicTrapCeiling )
			{
				trapName = Language::get(4352);
			}
			else if ( entity->behavior == &actSpearTrap )
			{
				trapName = Language::get(4350);
			}
		}
		auto& textMap = text_map[key];
		auto highlights = textMap.bannerHighlights;
		if ( highlights.size() > 0 )
		{
			int indexStart = 0;
			for ( auto highlight : highlights )
			{
				indexStart = std::max(highlight, indexStart);
			}
			for ( auto c : trapName )
			{
				if ( c == ' ' )
				{
					highlights.insert(indexStart + 1);
					++indexStart;
				}
			}
		}
		if ( setType == SET_CALLOUT_BANNER_TEXT )
		{
			setCalloutBannerTextFormatted(player, field, color, highlights,
				textMap.bannerText.c_str(), trapName.c_str());
		}
		else
		{
			return getCalloutMessage(textMap, trapName.c_str(), targetPlayer);
		}
		return "";
	}
	case CALLOUT_TYPE_GENERIC_INTERACTABLE:
	{
		key = "generic_interactable";
		if ( text_map.find(key) == text_map.end() )
		{
			key = "default";
		}
		if ( setType == SET_CALLOUT_ICON_KEY )
		{
			return key;
		}
		std::string objectName = Language::get(4366);
		if ( entity )
		{
			if ( entity->behavior == &actSink )
			{
				objectName = Language::get(4354);
			}
			else if ( entity->behavior == &actHeadstone )
			{
				objectName = Language::get(4357);
			}
			else if ( entity->behavior == &actCampfire )
			{
				objectName = Language::get(4365);
			}
			else if ( entity->behavior == &actPowerCrystal )
			{
				objectName = Language::get(4356);
			}
			else if ( entity->behavior == &actPedestalBase )
			{
				objectName = Language::get(4364);
			}
			else if ( entity->behavior == &actFloorDecoration && entity->sprite == 991 )
			{
				objectName = Language::get(4363);
			}
		}
		auto& textMap = text_map[key];
		if ( setType == SET_CALLOUT_BANNER_TEXT )
		{
			setCalloutBannerTextFormatted(player, field, color, textMap.bannerHighlights,
				textMap.bannerText.c_str(), objectName.c_str());
		}
		else
		{
			return getCalloutMessage(textMap, objectName.c_str(), targetPlayer);
		}
		return "";
	}
	case CALLOUT_TYPE_SHRINE:
		key = "shrine";
		break;
	case CALLOUT_TYPE_BELL:
		key = "bell";
		break;
	case CALLOUT_TYPE_DAEDALUS:
		key = "daedalus";
		break;
	case CALLOUT_TYPE_EXIT:
		key = "exit";
		break;
	case CALLOUT_TYPE_SECRET_EXIT:
		key = "secret_exit";
		break;
	case CALLOUT_TYPE_SECRET_ENTRANCE:
		key = "secret_entrance";
		break;
	case CALLOUT_TYPE_GOLD:
		key = "gold";
		break;
	case CALLOUT_TYPE_FOUNTAIN:
		key = "fountain";
		break;
	case CALLOUT_TYPE_SINK:
		key = "sink";
		break;
	case CALLOUT_TYPE_TELEPORTER_LADDER_UP:
		key = "teleporter";
		if ( text_map.find(std::string(key + "_up")) != text_map.end() )
		{
			key += "_up";
		}
		break;
	case CALLOUT_TYPE_TELEPORTER_LADDER_DOWN:
		key = "teleporter";
		if ( text_map.find(std::string(key + "_down")) != text_map.end() )
		{
			key += "_down";
		}
		break;
	case CALLOUT_TYPE_TELEPORTER_PORTAL:
		key = "teleporter";
		if ( text_map.find(std::string(key + "_portal")) != text_map.end() )
		{
			key += "_portal";
		}
		break;
	case CALLOUT_TYPE_BOMB_TRAP:
	{
		key = "bomb";
		if ( setType == SET_CALLOUT_ICON_KEY )
		{
			return key;
		}
		std::string trapName = Language::get(4362);
		if ( entity )
		{
			auto highlights = text_map[key].bannerHighlights;
			if ( entity->behavior == &actBomb )
			{
				if ( entity->skill[21] >= WOODEN_SHIELD && entity->skill[21] < NUMITEMS )
				{
					trapName = items[entity->skill[21]].getIdentifiedName();
					if ( highlights.size() > 0 )
					{
						highlights.insert(*highlights.begin() + 1);
					}
				}
			}
			else if ( entity->behavior == &actBeartrap )
			{
				trapName = items[TOOL_BEARTRAP].getIdentifiedName();
			}
			auto& textMap = text_map[key];
			if ( setType == SET_CALLOUT_BANNER_TEXT )
			{
				setCalloutBannerTextFormatted(player, field, color, highlights,
					textMap.bannerText.c_str(), trapName.c_str());
			}
			else
			{
				return getCalloutMessage(textMap, trapName.c_str(), targetPlayer);
			}
		}
		return "";
	}
	case CALLOUT_TYPE_COLLIDER_BREAKABLE:
	{
		key = "collider";
		if ( setType == SET_CALLOUT_ICON_KEY )
		{
			return key;
		}
		if ( entity )
		{
			auto highlights = text_map[key].bannerHighlights;
			std::string objectName = Language::get(entity->getColliderLangName());

			if ( highlights.size() > 0 )
			{
				int indexStart = *highlights.begin();
				for ( auto c : objectName )
				{
					if ( c == ' ' )
					{
						highlights.insert(indexStart + 1);
						++indexStart;
					}
				}
			}
			auto& textMap = text_map[key];
			if ( setType == SET_CALLOUT_BANNER_TEXT )
			{
				setCalloutBannerTextFormatted(player, field, color, highlights,
					textMap.bannerText.c_str(), objectName.c_str());
			}

			else
			{
				return getCalloutMessage(textMap, objectName.c_str(), targetPlayer);
			}
		}
		return "";
		break;
	}
		default:
			break;
	}

	if ( text_map.find(key) == text_map.end() )
	{
		if ( setType == SET_CALLOUT_BANNER_TEXT )
		{
			setCalloutBannerTextUnformatted(player, field, iconName, "default", color);
		}
		else if ( setType == SET_CALLOUT_ICON_KEY )
		{
			return "default";
		}
		else
		{
			return getCalloutMessage(text_map["default"], nullptr, targetPlayer);
		}
		return "";
	}
	if ( setType == SET_CALLOUT_BANNER_TEXT )
	{
		setCalloutBannerTextUnformatted(player, field, iconName, key.c_str(), color);
	}
	else if ( setType == SET_CALLOUT_ICON_KEY )
	{
		return key;
	}
	else
	{
		return getCalloutMessage(text_map[key], nullptr, targetPlayer);
	}
	return "";
}

void CalloutRadialMenu::initCalloutMenuGUICursor(bool openInventory)
{
	bool oldshootmode = players[gui_player]->shootmode;
	if ( openInventory )
	{
		//players[gui_player]->openStatusScreen(GUI_MODE_INVENTORY, INVENTORY_MODE_ITEM);
		players[gui_player]->closeAllGUIs(DONT_CHANGE_SHOOTMODE, CLOSEGUI_DONT_CLOSE_CALLOUTGUI);
		players[gui_player]->openStatusScreen(GUI_MODE_CALLOUT, INVENTORY_MODE_ITEM);
	}

	if ( !oldshootmode )
	{
		Uint32 flags = (Inputs::SET_MOUSE | Inputs::SET_CONTROLLER | Inputs::UNSET_RELATIVE_MOUSE);
		inputs.warpMouse(gui_player,
			players[gui_player]->camera_x1() + (players[gui_player]->camera_width() / 2),
			players[gui_player]->camera_y1() + (players[gui_player]->camera_height() / 2), flags);
	}

	inputs.setMouse(gui_player, Inputs::OX, inputs.getMouse(gui_player, Inputs::X));
	inputs.setMouse(gui_player, Inputs::OY, inputs.getMouse(gui_player, Inputs::Y));

	if ( menuX == -1 )
	{
		menuX = inputs.getMouse(gui_player, Inputs::X);
	}
	if ( menuY == -1 )
	{
		menuY = inputs.getMouse(gui_player, Inputs::Y);
	}
}

bool CalloutRadialMenu::calloutGUIHasBeenCreated() const
{
	if ( calloutFrame )
	{
		if ( !calloutFrame->getFrames().empty() )
		{
			for ( auto f : calloutFrame->getFrames() )
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

void CalloutRadialMenu::createCalloutMenuGUI()
{
	const int player = getPlayer();
	if ( !gui || !calloutFrame )
	{
		return;
	}
	if ( calloutGUIHasBeenCreated() )
	{
		return;
	}

	const int midx = calloutFrame->getSize().w / 2;
	const int midy = calloutFrame->getSize().h / 2;

	auto bgFrame = calloutFrame->addFrame("wheel base");
	bgFrame->setSize(SDL_Rect{ 0, 0, calloutFrame->getSize().w, calloutFrame->getSize().h });
	bgFrame->setHollow(false);
	bgFrame->setDisabled(false);
	bgFrame->setInheritParentFrameOpacity(false);
	bgFrame->setOpacity(0.0);

	const char* font = "fonts/pixel_maz_multiline.ttf#16#2";

	int panelIndex = 0;
	for ( auto& entry : panelEntries )
	{
		if ( panelIndex < PANEL_DIRECTION_END )
		{
			SDL_Rect pos{ entry.x + midx, entry.y + midy, 0, 0 };
			char buf[32] = "";
			snprintf(buf, sizeof(buf), "panel %d", panelIndex);
			Frame::image_t* img = bgFrame->addImage(pos, 0xFFFFFFFF, entry.path.c_str(), buf);
			if ( auto imgGet = Image::get(img->path.c_str()) )
			{
				img->pos.w = imgGet->getWidth();
				img->pos.h = imgGet->getHeight();
			}
			img->ontop = true;
		}
		++panelIndex;
	}

	panelIndex = 0;
	for ( auto& entry : panelEntries )
	{
		if ( panelIndex < PANEL_DIRECTION_END )
		{
			SDL_Rect pos{ entry.x + midx, entry.y + midy, 0, 0 };
			char buf[32] = "";
			snprintf(buf, sizeof(buf), "icon %d", panelIndex);
			Frame::image_t* imgIcon = bgFrame->addImage(pos, 0xFFFFFFFF, "", buf);
			imgIcon->disabled = true;
			imgIcon->ontop = true;
		}
		++panelIndex;
	}

	{
		// do center panel
		auto& entry = panelEntries[panelEntries.size() - 1];
		SDL_Rect pos{ entry.x + midx, entry.y + midy, 0, 0 };
		char buf[32] = "";
		snprintf(buf, sizeof(buf), "panel %d", PANEL_DIRECTION_END);
		Frame::image_t* img = bgFrame->addImage(pos, 0xFFFFFFFF, entry.path.c_str(), buf);
		if ( auto imgGet = Image::get(img->path.c_str()) )
		{
			img->pos.w = imgGet->getWidth();
			img->pos.h = imgGet->getHeight();
		}
	}

	auto bannerFrame = calloutFrame->addFrame("banner frame");
	bannerFrame->setSize(SDL_Rect{ 0, 0, 0, 40 });
	bannerFrame->setHollow(false);
	bannerFrame->setDisabled(false);
	bannerFrame->setInheritParentFrameOpacity(false);
	bannerFrame->addImage(SDL_Rect{ 0, 0, 42, 40 }, 0xFFFFFFFF, "#*images/ui/FollowerWheel/banner-cmd_l.png", "banner left");
	bannerFrame->addImage(SDL_Rect{ 0, 0, 42, 40 }, 0xFFFFFFFF, "#*images/ui/FollowerWheel/banner-cmd_r.png", "banner right");
	bannerFrame->addImage(SDL_Rect{ 0, 12, 0, 28 }, 0xFFFFFFFF, "*images/ui/FollowerWheel/banner-cmd_c.png", "banner center");
	auto bannerText = bannerFrame->addField("banner txt", 128);
	bannerText->setFont(font);
	bannerText->setText("");
	bannerText->setHJustify(Field::justify_t::LEFT);
	bannerText->setVJustify(Field::justify_t::TOP);
	bannerText->setSize(SDL_Rect{ 0, 0, 0, 24 });
	bannerText->setTextColor(followerBannerTextColor);
	bannerText->setOutlineColor(makeColor(29, 16, 11, 255));
	auto bannerGlyph = bannerFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "", "banner glyph");
	bannerGlyph->disabled = true;
	auto bannerGlyph2 = bannerFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "", "banner modifier glyph");
	bannerGlyph2->disabled = true;

	auto wheelTitleText = bgFrame->addField("wheel title", 128);
	wheelTitleText->setFont(font);
	wheelTitleText->setText("");
	wheelTitleText->setHJustify(Field::justify_t::LEFT);
	wheelTitleText->setVJustify(Field::justify_t::TOP);
	wheelTitleText->setSize(SDL_Rect{ 0, 0, 240, 24 });
	wheelTitleText->setTextColor(followerTitleColor);
	wheelTitleText->setOutlineColor(makeColor(29, 16, 11, 255));

	auto wheelSkillImg = bannerFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "", "skill img");
	wheelSkillImg->disabled = true;
	auto wheelStatImg = bannerFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "", "stat img");
	wheelStatImg->disabled = true;
}

bool CalloutRadialMenu::calloutMenuIsOpen()
{
	if ( selectMoveTo || bOpen )
	{
		return true;
	}
	return false;
}

std::vector<CalloutRadialMenu::PanelEntry> CalloutRadialMenu::panelEntries;
std::map<std::string, CalloutRadialMenu::IconEntry> CalloutRadialMenu::iconEntries;
std::map<std::string, CalloutRadialMenu::WorldIconEntry_t> CalloutRadialMenu::worldIconEntries;
std::map<std::string, std::string> CalloutRadialMenu::helpDescriptors;
std::map<int, std::string> CalloutRadialMenu::worldIconIDToEntryKey;
int CalloutRadialMenu::followerWheelButtonThickness = 70;
int CalloutRadialMenu::followerWheelRadius = 140;
int CalloutRadialMenu::followerWheelFrameOffsetX = 0;
int CalloutRadialMenu::followerWheelFrameOffsetY = 0;
int CalloutRadialMenu::followerWheelInnerCircleRadiusOffset = 0;
int CalloutRadialMenu::followerWheelInnerCircleRadiusOffsetAlternate = 0;
Uint32 CalloutRadialMenu::CalloutParticle_t::kParticleLifetime = TICKS_PER_SECOND * 5;

std::vector<CalloutRadialMenu::PanelEntry>& getPanelEntriesForCallout()
{
	return CalloutRadialMenu::panelEntries;
}

CalloutRadialMenu::CalloutType CalloutRadialMenu::getCalloutTypeForEntity(const int player, Entity* parent)
{
	if ( !parent )
	{
		return CALLOUT_TYPE_NO_TARGET;
	}
	CalloutType type = CALLOUT_TYPE_GENERIC_INTERACTABLE;

	if ( parent->behavior == &actPlayer || parent->behavior == &actDeathGhost )
	{
		type = CALLOUT_TYPE_PLAYER;
	}
	else if ( parent->behavior == &actItem )
	{
		type = CALLOUT_TYPE_ITEM;
	}
	else if ( parent->behavior == &actGoldBag )
	{
		type = CALLOUT_TYPE_GOLD;
	}
	else if ( parent->behavior == &actFountain )
	{
		type = CALLOUT_TYPE_FOUNTAIN;
	}
	else if ( parent->behavior == &actSink )
	{
		type = CALLOUT_TYPE_SINK;
	}
	else if ( parent->behavior == &actChestLid || parent->behavior == &actChest || parent->isInertMimic() )
	{
		type = CALLOUT_TYPE_CHEST;
	}
	/*else if ( parent->behavior == &actTorch )
	{
	}*/
	/*else if ( parent->behavior == &actCrystalShard )
	{
	}*/
	else if ( parent->behavior == &actHeadstone )
	{
		type = CALLOUT_TYPE_GENERIC_INTERACTABLE;
	}
	else if ( parent->behavior == &actColliderDecoration && parent->isDamageableCollider() )
	{
		type = CALLOUT_TYPE_COLLIDER_BREAKABLE;
	}
	else if ( parent->behavior == &actMonster )
	{
		int monsterType = parent->getMonsterTypeFromSprite();
		bool enemies = false;
		if ( players[player]->entity )
		{
			if ( multiplayer != CLIENT && parent->checkEnemy(players[player]->entity) )
			{
				enemies = true;
			}
			else if ( multiplayer == CLIENT
				&& !parent->monsterAllyGetPlayerLeader()
				&& !monsterally[monsterType][stats[player]->type] )
			{
				enemies = true;
			}
		}
		else
		{
			if ( !parent->monsterAllyGetPlayerLeader()
				&& !monsterally[monsterType][stats[player]->type] )
			{
				enemies = true;
			}
		}

		type = CALLOUT_TYPE_NPC;
		if ( monsterType == SHOPKEEPER )
		{
			type = CALLOUT_TYPE_NPC;
		}
		else if ( enemies )
		{
			type = CALLOUT_TYPE_NPC_ENEMY;
		}
		else if ( parent->monsterAllyGetPlayerLeader() )
		{
			type = CALLOUT_TYPE_NPC_PLAYERALLY;
		}
	}
	/*else if ( parent->behavior == &actGate )
	{
	}*/
	else if ( parent->behavior == &actSwitch || parent->behavior == &actSwitchWithTimer )
	{
		if ( parent->skill[0] == 1 )
		{
			type = CALLOUT_TYPE_SWITCH_ON;
		}
		else
		{
			type = CALLOUT_TYPE_SWITCH_OFF;
		}
	}
	else if ( parent->behavior == &actPowerCrystal )
	{
		type = CALLOUT_TYPE_GENERIC_INTERACTABLE;
	}
	else if ( parent->behavior == &actPedestalBase )
	{
		type = CALLOUT_TYPE_GENERIC_INTERACTABLE;
	}
	else if ( parent->behavior == &actCampfire )
	{
		type = CALLOUT_TYPE_GENERIC_INTERACTABLE;
	}
	else if ( parent->behavior == &actBoulderTrapHole )
	{
		type = CALLOUT_TYPE_TRAP;
	}
	else if ( parent->behavior == &actFloorDecoration && parent->sprite == 991 )
	{
		type = CALLOUT_TYPE_GENERIC_INTERACTABLE;
	}
	else if ( parent->behavior == &actBoulder )
	{
		type = CALLOUT_TYPE_BOULDER;
	}
	else if ( parent->behavior == &actLadder )
	{
		if ( secretlevel && parent->skill[3] == 1 ) // secret ladder
		{
			type = CALLOUT_TYPE_SECRET_EXIT;
		}
		else if ( !secretlevel && parent->skill[3] == 1 ) // secret ladder
		{
			type = CALLOUT_TYPE_SECRET_ENTRANCE;
		}
		else
		{
			type = CALLOUT_TYPE_EXIT;
		}
	}
	else if ( parent->behavior == &actPortal )
	{
		if ( parent->skill[3] == 0 ) // secret entrance portal
		{
			if ( secretlevel )
			{
				type = CALLOUT_TYPE_SECRET_EXIT;
			}
			else
			{
				type = CALLOUT_TYPE_SECRET_ENTRANCE;
			}
		}
		else
		{
			if ( !strcmp(map.name, "Hell") )
			{
				type = CALLOUT_TYPE_EXIT;
			}
			else if ( !strcmp(map.name, "Mages Guild") )
			{
				type = CALLOUT_TYPE_EXIT;
			}
			else
			{
				type = CALLOUT_TYPE_EXIT;
			}
		}
	}
	else if ( parent->behavior == &::actMidGamePortal )
	{
		type = CALLOUT_TYPE_EXIT;
	}
	else if ( parent->behavior == &actCustomPortal )
	{
		if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
		{
			type = CALLOUT_TYPE_EXIT;
		}
		else
		{
			if ( parent->portalCustomSpriteAnimationFrames > 0 )
			{
				type = CALLOUT_TYPE_EXIT;
			}
			else
			{
				type = CALLOUT_TYPE_EXIT;
			}
		}
	}
	else if ( parent->behavior == &::actExpansionEndGamePortal
		|| parent->behavior == &actWinningPortal )
		{
			type = CALLOUT_TYPE_EXIT;
	}
	else if ( parent->behavior == &actTeleporter )
	{
		if ( parent->teleporterType == 2 ) // portal
		{
			type = CALLOUT_TYPE_TELEPORTER_PORTAL;
		}
		else if ( parent->teleporterType == 1 ) // down ladder
		{
			type = CALLOUT_TYPE_TELEPORTER_LADDER_DOWN;
		}
		else if ( parent->teleporterType == 0 ) // up ladder
		{
			type = CALLOUT_TYPE_TELEPORTER_LADDER_UP;
		}
	}
	else if ( parent->behavior == &::actTeleportShrine /*|| parent->behavior == &::actSpellShrine*/ )
	{
		type = CALLOUT_TYPE_SHRINE;
	}
	else if ( parent->behavior == &::actDaedalusShrine )
	{
		type = CALLOUT_TYPE_DAEDALUS;
	}
	else if ( parent->behavior == &actBell )
	{
		type = CALLOUT_TYPE_BELL;
	}
	else if ( parent->behavior == &actBomb || parent->behavior == &actBeartrap )
	{
		type = CALLOUT_TYPE_BOMB_TRAP;
	}
	return type;
}

CalloutRadialMenu::CalloutType CalloutRadialMenu::getCalloutTypeForUid(const int player, Uint32 uid)
{
	Entity* parent = uidToEntity(uid);
	if ( !parent )
	{
		return CALLOUT_TYPE_NO_TARGET;
	}

	return CalloutRadialMenu::getCalloutTypeForEntity(player, parent);
}

static ConsoleVariable<bool> cvar_callout_debug("/callout_debug", false);
bool CalloutRadialMenu::calloutMenuEnabledForGamemode()
{
	if ( *cvar_callout_debug )
	{
		return true;
	}
	if ( multiplayer != SINGLE || (multiplayer == SINGLE && splitscreen) )
	{
		return true;
	}
	return false;
}

bool CalloutRadialMenu::uidMatchesPlayer(const int playernum, const Uint32 uid)
{
	if ( uid == 0 ) { return false; }
	if ( achievementObserver.playerUids[playernum] == uid )
	{
		return true;
	}
	else if ( players[playernum]->ghost.uid == uid )
	{
		return true;
	}
	return false;
}

Uint32 CalloutRadialMenu::getPlayerUid(const int playernum)
{
	if ( players[playernum]->ghost.isActive() )
	{
		return players[playernum]->ghost.uid;
	}
	else
	{
		return achievementObserver.playerUids[playernum];
	}
}

void CalloutRadialMenu::CalloutParticle_t::init(const int player)
{
	creationTick = ::ticks;
	messageSentTick = ::ticks;
	lifetime = kParticleLifetime;
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( i == player && uidMatchesPlayer(player, entityUid) && players[i]->isLocalPlayer() )
		{
			lockOnScreen[i] = true;
		}
		else
		{
			lockOnScreen[i] = true;
		}
		big[i] = true;
		animateScaleForPlayerView[i] = 0.0;
	}
	Entity* parent = uidToEntity(entityUid);

	if ( !parent )
	{
		return;
	}

	z = parent->z - 4;

	type = CalloutRadialMenu::getCalloutTypeForEntity(player, parent);
}

void CalloutRadialMenu::closeCalloutMenuGUI()
{
	if ( calloutMenuIsOpen() )
	{
		players[gui_player]->worldUI.reset();
	}
	bOpen = false;
	lockOnEntityUid = 0;
	selectMoveTo = false;
	menuX = -1;
	menuY = -1;
	moveToX = -1;
	moveToY = -1;
	menuToggleClick = false;
	holdWheel = false;
	optionSelected = -1;
	if ( calloutFrame )
	{
		calloutFrame->setDisabled(true);
		for ( auto f : calloutFrame->getFrames() )
		{
			f->removeSelf();
		}
	}
	animTitle = 0.0;
	animWheel = 0.0;
	openedThisTick = 0;
	animInvalidAction = 0.0;
	animInvalidActionTicks = 0;
}

std::string& CalloutRadialMenu::WorldIconEntry_t::getPlayerIconPath(const int playernum)
{
	if ( colorblind_lobby )
	{
		switch ( playernum )
		{
		case 0:
			return pathPlayer3;
		case 1:
			return pathPlayer4;
		case 2:
			return pathPlayer2;
		case 3:
			return pathPlayerX;
		default:
			return pathPlayerX;
			break;
		}
	}
	else
	{
		switch ( playernum )
		{
		case 0:
			return pathPlayer1;
		case 1:
			return pathPlayer2;
		case 2:
			return pathPlayer3;
		case 3:
			return pathPlayer4;
		default:
			return pathPlayerX;
			break;
		}
	}
}

void CalloutRadialMenu::drawCallouts(const int playernum)
{
	auto& pingFrame = CalloutMenu[playernum].calloutPingFrame;
	if ( !pingFrame )
	{
		pingFrame = gameUIFrame[playernum]->addFrame("callout pings");
		pingFrame->setHollow(true);
		pingFrame->setDisabled(true);
		pingFrame->setInheritParentFrameOpacity(false);
		pingFrame->setBorder(0);
		pingFrame->setOwner(playernum);
	}

	if ( players[playernum]->hud.hudFrame )
	{
		pingFrame->setDisabled(players[playernum]->hud.hudFrame->isDisabled());
	}

	pingFrame->setSize(SDL_Rect{ players[playernum]->camera_virtualx1(),
		players[playernum]->camera_virtualy1(),
		players[playernum]->camera_virtualWidth(),
		players[playernum]->camera_virtualHeight() });

	struct CalloutToDraw_t
	{
		Uint32 creationTick = 0;
		real_t dist = 0.0;
		std::string imgPath = "";
		Uint32 color = 0;
		SDL_Rect pos;
		CalloutToDraw_t(std::string _imgPath, Uint32 _color, SDL_Rect _pos, Uint32 _creationTick, real_t _dist)
		{
			imgPath = _imgPath;
			color = _color;
			pos = _pos;
			creationTick = _creationTick;
			dist = _dist;
		}
	};

	auto compFunc = [](CalloutToDraw_t& lhs, CalloutToDraw_t& rhs)
	{
		return lhs.dist < rhs.dist;
	};
	auto compFunc2 = [](CalloutToDraw_t& lhs, CalloutToDraw_t& rhs)
	{
		return lhs.creationTick < rhs.creationTick;
	};
	std::priority_queue<CalloutToDraw_t, std::vector<CalloutToDraw_t>, decltype(compFunc)> priorityQueue(compFunc);
	std::priority_queue<CalloutToDraw_t, std::vector<CalloutToDraw_t>, decltype(compFunc2)> priorityQueueSelf(compFunc2);

	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		for ( auto& callout : CalloutMenu[i].callouts )
		{
			bool selfCallout = false;
			if ( uidMatchesPlayer(playernum, callout.second.entityUid) )
			{
				if ( i == playernum && players[i]->entity && players[i]->entity->skill[3] == 1 )
				{
					// debug/thirdperson cam.
				}
				else
				{
					selfCallout = true;
					//continue; // don't draw self callouts
				}
			}

			auto* iconPaths = &CalloutRadialMenu::worldIconEntries[CalloutRadialMenu::worldIconIDToEntryKey[callout.second.tagID]];
			auto& iconPathsMini = CalloutRadialMenu::worldIconEntries[CalloutRadialMenu::worldIconIDToEntryKey[callout.second.tagSmallID]];
			if ( selfCallout )
			{
				std::string checkTag = CalloutRadialMenu::worldIconIDToEntryKey[callout.second.tagID] + "_display_self";
				if ( CalloutRadialMenu::worldIconEntries.find(checkTag) != CalloutRadialMenu::worldIconEntries.end() )
				{
					iconPaths = &CalloutRadialMenu::worldIconEntries[checkTag];
				}
			}

			std::string iconPath = "";
			std::string iconPathMini = "";
			int playerColor = i;
			if ( callout.second.cmd == CALLOUT_CMD_SOUTH
				|| callout.second.cmd == CALLOUT_CMD_SOUTHWEST
				|| callout.second.cmd == CALLOUT_CMD_SOUTHEAST )
			{
				playerColor = CalloutMenu[i].getPlayerForDirectPlayerCmd(i, callout.second.cmd);
				if ( playernum == playerColor )
				{
					iconPaths = &CalloutRadialMenu::worldIconEntries["tag_btn_player_wave_to_me"];
				}
			}

			iconPath = iconPaths->getPlayerIconPath(playerColor);
			iconPathMini = iconPathsMini.getPlayerIconPath(playerColor);

			if ( iconPath == "" )
			{
				continue;
			}

			vec4_t v;
			mat4x4_t m, t;

			auto camera = &cameras[playernum];
			auto& player = players[playernum];

			const int offset = 40;
			int leftOfWindow = player->camera_virtualx1() + offset;
			int rightOfWindow = player->camera_virtualx1() + player->camera_virtualWidth() - offset;
			int topOfWindow = player->camera_virtualy1() + offset;
			int bottomOfWindow = player->camera_virtualy2() - offset;

			mat4x4_t id;
			vec4_t world{ (float)callout.second.x * 2.f, -(float)callout.second.z * 2.f, (float)callout.second.y * 2.f, 1.f };
			if ( selfCallout )
			{
				world.x = 32.0 * camera->x + 32.0 * cos(camera->ang);
				world.z = 32.0 * camera->y + 32.0 * sin(camera->ang);
			}
			vec4_t window2{ (float)0, (float)0,
				(float)player->camera_virtualWidth(), (float)player->camera_virtualHeight() };
			SDL_Rect dest{ 0, 0, 0, 0 };
			if ( callout.second.lockOnScreen[playernum] )
			{
				auto screen_position = project_clipped2(&world, &id, &camera->projview, &window2);
				dest = SDL_Rect{ player->camera_virtualx1() + (int)screen_position.clipped_coords.x,
					player->camera_virtualy1() + Frame::virtualScreenY - (Frame::virtualScreenY - player->camera_virtualHeight()) - (int)screen_position.clipped_coords.y,
				14, 22 };
				if ( !screen_position.isBehind
					&& (screen_position.direction == ClipResult::Direction::Front
						|| screen_position.direction == ClipResult::Direction::Invalid) )
				{
					callout.second.lockOnScreen[playernum] = false;
				}

				dest.x = std::min(rightOfWindow, std::max(leftOfWindow, dest.x));
				dest.y = std::min(bottomOfWindow, std::max(topOfWindow, dest.y));
				real_t tangent = atan2(camera->y * 32.0 - world.z, camera->x * 32.0 - world.x);
				real_t camang = camera->ang;
				while ( tangent >= 2 * PI )
				{
					tangent -= PI * 2;
				}
				while ( tangent < 0 )
				{
					tangent += PI * 2;
				}
				while ( camang >= 2 * PI )
				{
					camang -= PI * 2;
				}
				while ( camang < 0 )
				{
					camang += PI * 2;
				}
				real_t result = tangent - camang;
				while ( result >= PI )
				{
					result -= PI * 2;
				}
				while ( result < -PI )
				{
					result += PI * 2;
				}
				//messagePlayer(player->playernum, MESSAGE_DEBUG, "%f", ((PI - abs(abs(tangent - camang) - PI)) * 2));
				//messagePlayer(player->playernum, MESSAGE_DEBUG, "%f", result);
				if ( abs(result) < 0.0001 )
				{
					// really small angle due to camera fluctuations, affix to left side to prevent flicker
					dest.x = leftOfWindow;
				}
				else if ( result >= 0.0 && result < PI / 2 )
				{
					dest.x = leftOfWindow;
				}
				else if ( result < 0.0 && result > -PI / 2 )
				{
					dest.x = rightOfWindow;
				}

				if ( abs(result) < (3 * PI / 4) )
				{
					real_t mult = std::min(1.0, ((3 * PI / 4) - abs(result)) / (PI / 2));
					dest.y += ((player->camera_virtualy1() + (player->camera_virtualHeight() / 2)) - dest.y) * mult;
				}
			}
			else
			{
				auto screen_position = project(&world, &id, &camera->projview, &window2);
				if ( screen_position.z >= 1.0 || screen_position.z < 0.0 )
				{
					continue;
				}
				dest = SDL_Rect{ player->camera_virtualx1() + (int)screen_position.x,
					player->camera_virtualy1() + Frame::virtualScreenY - (Frame::virtualScreenY - player->camera_virtualHeight()) -
					(int)screen_position.y,
				14, 22 };
			}

			real_t lifePercent = callout.second.ticks / (real_t)callout.second.lifetime;
			if ( selfCallout )
			{
				// fade early for the self callout player, but not others in splitscreen
				lifePercent = callout.second.ticks / (real_t)((TICKS_PER_SECOND * 4) / 5);
			}
			else
			{
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( uidMatchesPlayer(i, callout.second.entityUid) )
					{
						if ( callout.second.cmd == CALLOUT_CMD_AFFIRMATIVE
							|| callout.second.cmd == CALLOUT_CMD_THANKS
							|| callout.second.cmd == CALLOUT_CMD_NEGATIVE
							|| callout.second.cmd == CALLOUT_CMD_LOOK
							|| callout.second.cmd == CALLOUT_CMD_SOUTH 
							|| callout.second.cmd == CALLOUT_CMD_SOUTHWEST 
							|| callout.second.cmd == CALLOUT_CMD_SOUTHEAST
							|| (callout.second.cmd == CALLOUT_CMD_HELP && players[i]->ghost.isActive()) )
						{
							// fade early for simple thumbs up/down for players
							lifePercent = callout.second.ticks / (real_t)((TICKS_PER_SECOND * 4) / 5);
						}
						break;
					}
				}
			}
			Uint32 alpha = 255;
			if ( lifePercent >= 0.8 )
			{
				alpha -= std::min((Uint32)255, (Uint32)(255 * (lifePercent - 0.8) / 0.2));
			}
			Uint32 color = makeColor(255, 255, 255, alpha);
			SDL_Rect iconPos = dest;

			bool drawMini = false;

			Entity* playerEntity = Player::getPlayerInteractEntity(playernum);
			if ( playerEntity && playerEntity->bodyparts.size() > 0 )
			{
				auto bodypart = playerEntity->bodyparts[0];
				real_t tempx = bodypart->x;
				real_t tempy = bodypart->y;
				bodypart->x = callout.second.x;
				bodypart->y = callout.second.y;

				real_t tangent = atan2(camera->y * 16.0 - bodypart->y, camera->x * 16.0 - bodypart->x);
				Entity* ohitentity = hit.entity;

				bool oldPassable = playerEntity->flags[PASSABLE];
				if ( playerEntity->behavior == &actDeathGhost )
				{
					playerEntity->flags[PASSABLE] = false; // hack to make ghosts linetraceable
				}
				lineTraceTarget(bodypart, bodypart->x, bodypart->y, tangent, 256, 0, false, playerEntity);
				playerEntity->flags[PASSABLE] = oldPassable;
				
				if ( hit.entity != playerEntity )
				{
					// no line of sight through walls
					drawMini = true;
				}
				hit.entity = ohitentity;
				bodypart->x = tempx;
				bodypart->y = tempy;
			}

			callout.second.big[playernum] = !drawMini;
			drawMini = false;

			real_t dist = pow(camera->y * 32.0 - world.z, 2) + pow(camera->x * 32.0 - world.x, 2);

			if ( !drawMini )
			{
				if ( auto image = Image::get(iconPath.c_str()) )
				{
					real_t scale = callout.second.scale - callout.second.animateScaleForPlayerView[playernum] * .5;
					iconPos.w = image->getWidth() * scale;
					iconPos.h = image->getHeight() * scale;
					const int heightOffset = image->getHeight() - iconPos.h;

					if ( selfCallout )
					{
						real_t y = iconPos.y - players[playernum]->camera_virtualy1();
						iconPos.y = players[playernum]->camera_virtualHeight() / 4;
						real_t factor = players[playernum]->camera_virtualHeight() / (real_t)Frame::virtualScreenY;
						iconPos.y += factor * 16.0 * (y - iconPos.y) 
							/ (real_t)players[playernum]->camera_virtualHeight();
						iconPos.y += players[playernum]->camera_virtualy1();
					}

					iconPos.x -= iconPos.w / 2;
					iconPos.y -= iconPos.h + heightOffset / 2;
					iconPos.y -= (iconPos.h / 4) * callout.second.animateBounce;
					/*image->drawColor(nullptr, iconPos,
						SDL_Rect{ 0, 0, Frame::virtualScreenX, Frame::virtualScreenY }, color);*/

					iconPos.x -= players[playernum]->camera_virtualx1();
					iconPos.y -= players[playernum]->camera_virtualy1();

					priorityQueue.push(CalloutToDraw_t(iconPath, color, iconPos, callout.second.creationTick, dist));
				}
			}
			else
			{
				if ( auto image = Image::get(iconPathMini.c_str()) )
				{
					dest.w = image->getWidth();
					dest.h = image->getHeight();
					dest.x -= dest.w / 2;
					dest.y -= dest.h;
					image->drawColor(nullptr, dest,
						SDL_Rect{ 0, 0, Frame::virtualScreenX, Frame::virtualScreenY }, color);
				}
			}
		}
	}

	auto& images = pingFrame->getImages();
	while ( images.size() > priorityQueue.size() )
	{
		images.erase(images.begin());
	}
	while ( images.size() < priorityQueue.size() )
	{
		pingFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0, "", "img");
	}

	size_t index = 0;
	for ( auto img : images )
	{
		img->disabled = true;
	}
	while ( !priorityQueue.empty() )
	{
		auto& top = priorityQueue.top();
		if ( index < images.size() )
		{
			auto img = images[index];
			img->color = top.color;
			img->path = top.imgPath;
			img->pos = top.pos;
			img->disabled = false;
		}
		priorityQueue.pop();
		++index;
	}
}

void CalloutRadialMenu::CalloutParticle_t::animate()
{
	static ConsoleVariable<float> cvar_calloutanimspeed("/calloutanimspeed", 0.3);
	static ConsoleVariable<float> cvar_calloutbouncespeed("/calloutbouncespeed", 0.9);
	static ConsoleVariable<int> cvar_calloutbouncestate("/calloutbouncestate", 0);
	real_t animspeed = 5.0 * *cvar_calloutanimspeed;
	if ( animateState == 0 )
	{
		if ( animateStateInit == 0 )
		{
			animateStateInit = 1;
			animateBounce = 0.0;
		}

		scale = 0.25 + 1.25 * animateX;
		if ( animateX >= 1.0 )
		{
			++animateState;
			animateX = 0.0;
		}
		animspeed *= 2.0;
	}
	else if ( animateState == 1 )
	{
		if ( animateStateInit == 1 )
		{
			animateStateInit = 2;
		}

		scale = 1.5 - .75 * animateX;
		if ( animateX >= 1.0 )
		{
			++animateState;
			animateX = 0.0;
		}
		animspeed *= 2.0;
	}
	else if ( animateState == 2 )
	{
		if ( animateStateInit == 2 )
		{
			animateStateInit = 3;
		}

		scale = 0.75 + 0.25 * animateX;
		if ( animateX >= 1.0 )
		{
			++animateState;
		}
		animspeed *= 1.5;
	}
	//else if ( animateState == 4 )
	//{
	//	if ( animateStateInit == 4 )
	//	{
	//		animateStateInit = 5;
	//	}

	//	scale = 1.5 - 0.5 * animateX;
	//	if ( animateX >= 1.0 )
	//	{
	//		++animateState;
	//		animateX = 0.0;
	//	}
	//	animspeed /= 2.0;
	//}
	else
	{
		scale = 0.5 + 0.5 * animateX;
	}

	const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
	if ( animateState <= 2 )
	{
		real_t setpointDiffX = fpsScale * std::max(.1, (1.0 - animateX)) / (animspeed);
		animateX += setpointDiffX;
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			animateScaleForPlayerView[i] = 0.0;
		}
	}
	else
	{
		animateX = 1.0;
		/*if ( big )
		{
			real_t setpointDiffX = fpsScale * std::max(.1, (1.0 - animateX)) / (animspeed);
			animateX += setpointDiffX;
		}
		else
		{
			real_t setpointDiffX = fpsScale * std::max(.1, (animateX)) / (animspeed);
			animateX -= setpointDiffX;
		}*/
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( !big[i] )
			{
				real_t setpointDiffX = fpsScale * std::max(.1, (1.0 - animateScaleForPlayerView[i])) / (animspeed);
				animateScaleForPlayerView[i] += setpointDiffX;
			}
			else
			{
				real_t setpointDiffX = fpsScale * std::max(.1, (animateScaleForPlayerView[i])) / (animspeed);
				animateScaleForPlayerView[i] -= setpointDiffX;
			}
			animateScaleForPlayerView[i] = std::max(0.0, std::min(1.0, animateScaleForPlayerView[i]));
		}
	}
	animateX = std::max(0.0, std::min(1.0, animateX));

	animateBounce = sin(animateY * PI * 2);
	if ( animateState >= *cvar_calloutbouncestate )
	{
		animateY += fpsScale * 0.05 * *cvar_calloutbouncespeed;
		animateY = std::min(1.0, animateY);
	}
}

void CalloutRadialMenu::update()
{
	for ( auto& c : callouts )
	{
		auto& callout = c.second;
		Entity* entity = uidToEntity(callout.entityUid);
		callout.animate();
		if ( entity )
		{
			if ( TimerExperiments::bUseTimerInterpolation && entity->bUseRenderInterpolation )
			{
				callout.x = entity->lerpRenderState.x.position * 16.0;
				callout.y = entity->lerpRenderState.y.position * 16.0;
				callout.z = entity->lerpRenderState.z.position + enemyBarSettings.getHeightOffset(entity);
				callout.z -= 4;
				if ( entity->behavior == &actMonster && entity->getMonsterTypeFromSprite() == BAT_SMALL )
				{
					if ( entity->bodyparts.size() > 0 )
					{
						callout.z += entity->bodyparts[0]->lerpRenderState.z.position;
					}
				}
			}
			else
			{
				callout.x = entity->x;
				callout.y = entity->y;
				callout.z = entity->z + enemyBarSettings.getHeightOffset(entity);
				callout.z -= 4;
				if ( entity->behavior == &actMonster && entity->getMonsterTypeFromSprite() == BAT_SMALL )
				{
					if ( entity->bodyparts.size() > 0 )
					{
						callout.z += entity->bodyparts[0]->z;
					}
				}
			}
		}
		else if ( callout.entityUid != 0 )
		{
			callout.expired = true;
		}
	}


	if ( updatedThisTick == 0 || ticks != updatedThisTick )
	{
		updatedThisTick = ticks;
		for ( auto it = callouts.begin(); it != callouts.end(); ++it )
		{
			it->second.ticks++;
			if ( it->second.ticks >= it->second.lifetime )
			{
				it->second.expired = true;
			}
		}
	}

	for ( auto it = callouts.begin(); it != callouts.end(); )
	{
		if ( it->second.expired )
		{
			it = callouts.erase(it);
			continue;
		}
		else
		{
			++it;
		}
	}
}

int CalloutRadialMenu::CALLOUT_SFX_NEUTRAL = 605;
int CalloutRadialMenu::CALLOUT_SFX_NEGATIVE = 607;
int CalloutRadialMenu::CALLOUT_SFX_POSITIVE = 606;
static ConsoleVariable<int> cvar_callout_sfx_vol("/callout_sfx_vol", 128);

bool CalloutRadialMenu::createParticleCallout(Entity* entity, CalloutRadialMenu::CalloutCommand _cmd, Uint32 overrideUID)
{
	if ( !entity ) { return false; }
	if ( _cmd == CALLOUT_CMD_CANCEL ) { return false; }

	Uint32 existingMessageSent = 0;
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( CalloutMenu[i].callouts.find(entity->getUID()) != CalloutMenu[i].callouts.end() )
		{
			auto& existingCallout = CalloutMenu[i].callouts[entity->getUID()];
			if ( i == getPlayer() && existingCallout.cmd == _cmd )
			{
				existingMessageSent = existingCallout.messageSentTick;
			}
			CalloutMenu[i].callouts.erase(entity->getUID()); // delete other players pings on this object
		}
	}

	auto& callout = callouts[entity->getUID()];
	real_t x = entity->x;
	real_t y = entity->y;
	if ( TimerExperiments::bUseTimerInterpolation && entity->bUseRenderInterpolation )
	{
		x = entity->lerpRenderState.x.position * 16.0;
		y = entity->lerpRenderState.y.position * 16.0;
	}
	callout = CalloutRadialMenu::CalloutParticle_t(getPlayer(), x, y, entity->z, entity->getUID(), _cmd);
	if ( existingMessageSent > 0 && multiplayer != CLIENT )
	{
		if ( (callout.messageSentTick - existingMessageSent) < (TICKS_PER_SECOND * 3.5) )
		{
			callout.doMessage = false;
			callout.messageSentTick = existingMessageSent;
		}
	}

	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		while ( CalloutMenu[i].callouts.size() > 3 )
		{
			Uint32 earliestTick = ::ticks;
			auto itToDelete = CalloutMenu[i].callouts.end();
			for ( auto it = CalloutMenu[i].callouts.begin(); it != CalloutMenu[i].callouts.end(); ++it )
			{
				if ( it->second.creationTick < earliestTick )
				{
					earliestTick = it->second.creationTick;
					itToDelete = it;
				}
			}
			if ( itToDelete == CalloutMenu[i].callouts.end() )
			{
				break;
			}
			else
			{
				CalloutMenu[i].callouts.erase(itToDelete);
			}
		}
	}

	std::string calloutTypeKey = getCalloutKeyForCommand(_cmd);
	Uint32 oldTarget = lockOnEntityUid;
	if ( overrideUID != 0 )
	{
		lockOnEntityUid = overrideUID;
	}
	else
	{
		lockOnEntityUid = entity->getUID();
	}
	std::string key = setCalloutText(nullptr, calloutTypeKey.c_str(), 0, _cmd, SET_CALLOUT_ICON_KEY, -1);
	lockOnEntityUid = oldTarget;

	callout.tagID = worldIconEntries[iconEntries[calloutTypeKey].text_map[key].worldIconTag].id;
	callout.tagSmallID = worldIconEntries[iconEntries[calloutTypeKey].text_map[key].worldIconTagMini].id;

	if ( callout.cmd == CALLOUT_CMD_AFFIRMATIVE || callout.cmd == CALLOUT_CMD_THANKS )
	{
		playSound(CALLOUT_SFX_POSITIVE, *cvar_callout_sfx_vol);
	}
	else if ( callout.cmd == CALLOUT_CMD_NEGATIVE )
	{
		playSound(CALLOUT_SFX_NEGATIVE, *cvar_callout_sfx_vol);
	}
	else
	{
		playSound(CALLOUT_SFX_NEUTRAL, *cvar_callout_sfx_vol);
	}

	if ( multiplayer == SERVER )
	{
		for ( int i = 1; i < MAXPLAYERS; ++i )
		{
			if ( i == getPlayer() ) { continue; } // don't send clients their own callout
			if ( players[i]->isLocalPlayer() ) { continue; }
			if ( client_disconnected[i] ) { continue; }

			strcpy((char*)net_packet->data, "CALL");
			net_packet->data[4] = getPlayer();
			SDLNet_Write32(entity->getUID(), &net_packet->data[5]);
			net_packet->data[9] = (Uint8)_cmd;
			SDLNet_Write32(clientCalloutHelpFlags, &net_packet->data[10]);
			net_packet->len = 14;
			net_packet->address.host = net_clients[i - 1].host;
			net_packet->address.port = net_clients[i - 1].port;
			sendPacketSafe(net_sock, -1, net_packet, i - 1);
		}
	}

	if ( players[getPlayer()]->ghost.isActive() )
	{
		players[getPlayer()]->ghost.createBounceAnimate();
		if ( players[getPlayer()]->isLocalPlayer() )
		{
			Compendium_t::Events_t::eventUpdateMonster(getPlayer(), Compendium_t::CPDM_GHOST_PINGS, players[getPlayer()]->ghost.my, 1);
		}
	}

	return callout.doMessage;
}
bool CalloutRadialMenu::createParticleCallout(real_t x, real_t y, real_t z, Uint32 uid, CalloutRadialMenu::CalloutCommand _cmd)
{
	if ( _cmd == CALLOUT_CMD_CANCEL ) { return false; }

	//for ( int i = 0; i < MAXPLAYERS; ++i )
	//{
	//	if ( CalloutMenu[i].callouts.find(uid) != CalloutMenu[i].callouts.end() )
	//	{
	//		CalloutMenu[i].callouts.erase(uid);
	//	}
	//}
	Uint32 existingMessageSent = 0;
	if ( _cmd == CALLOUT_CMD_MOVE && callouts.find(uid) != callouts.end() )
	{
		auto& existingCallout = callouts[uid];
		if ( existingCallout.cmd == _cmd )
		{
			existingMessageSent = existingCallout.messageSentTick;
		}
	}
	auto& callout = callouts[uid];

	callout = CalloutRadialMenu::CalloutParticle_t(getPlayer(), x, y, z, uid, _cmd);
	if ( existingMessageSent > 0 && multiplayer != CLIENT )
	{
		if ( (callout.messageSentTick - existingMessageSent) < (TICKS_PER_SECOND * 3.5) )
		{
			callout.doMessage = false;
			callout.messageSentTick = existingMessageSent;
		}
	}

	if ( callout.cmd == CALLOUT_CMD_AFFIRMATIVE || callout.cmd == CALLOUT_CMD_THANKS )
	{
		playSound(CALLOUT_SFX_POSITIVE, *cvar_callout_sfx_vol);
	}
	else if ( callout.cmd == CALLOUT_CMD_NEGATIVE )
	{
		playSound(CALLOUT_SFX_NEGATIVE, *cvar_callout_sfx_vol);
	}
	else
	{
		playSound(CALLOUT_SFX_NEUTRAL, *cvar_callout_sfx_vol);
	}

	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		while ( CalloutMenu[i].callouts.size() > 3 )
		{
			Uint32 earliestTick = ::ticks;
			auto itToDelete = CalloutMenu[i].callouts.end();
			for ( auto it = CalloutMenu[i].callouts.begin(); it != CalloutMenu[i].callouts.end(); ++it )
			{
				if ( it->second.creationTick < earliestTick )
				{
					earliestTick = it->second.creationTick;
					itToDelete = it;
				}
			}
			if ( itToDelete == CalloutMenu[i].callouts.end() )
			{
				break;
			}
			else
			{
				CalloutMenu[i].callouts.erase(itToDelete);
			}
		}
	}

	std::string calloutTypeKey = getCalloutKeyForCommand(_cmd);
	Uint32 oldTarget = lockOnEntityUid;
	lockOnEntityUid = uid;
	std::string key = setCalloutText(nullptr, calloutTypeKey.c_str(), 0, _cmd, SET_CALLOUT_ICON_KEY, -1);
	lockOnEntityUid = oldTarget;

	callout.tagID = worldIconEntries[iconEntries[calloutTypeKey].text_map[key].worldIconTag].id;
	callout.tagSmallID = worldIconEntries[iconEntries[calloutTypeKey].text_map[key].worldIconTagMini].id;
	if ( uid == 0 && callout.cmd == CALLOUT_CMD_LOOK && multiplayer != CLIENT )
	{
		callout.doMessage = false;
	}

	if ( multiplayer == SERVER )
	{
		for ( int i = 1; i < MAXPLAYERS; ++i )
		{
			if ( i == getPlayer() ) { continue; } // don't send clients their own callout
			if ( players[i]->isLocalPlayer() ) { continue; }
			if ( client_disconnected[i] ) { continue; }

			strcpy((char*)net_packet->data, "CALL");
			net_packet->data[4] = getPlayer();
			SDLNet_Write32(uid, &net_packet->data[5]);
			net_packet->data[9] = (Uint8)_cmd;
			SDLNet_Write32(clientCalloutHelpFlags, &net_packet->data[10]);
			net_packet->len = 14;
			if ( uid == 0 )
			{
				Uint16 _x = std::min<Uint16>(std::max<int>(0.0, x / 16), map.width - 1);
				Uint16 _y = std::min<Uint16>(std::max<int>(0.0, y / 16), map.height - 1);
				SDLNet_Write16(_x, &net_packet->data[14]);
				SDLNet_Write16(_y, &net_packet->data[16]);
				net_packet->len = 18;
			}
			net_packet->address.host = net_clients[i - 1].host;
			net_packet->address.port = net_clients[i - 1].port;
			sendPacketSafe(net_sock, -1, net_packet, i - 1);
		}
	}

	if ( players[getPlayer()]->ghost.isActive() )
	{
		players[getPlayer()]->ghost.createBounceAnimate();
		if ( players[getPlayer()]->isLocalPlayer() )
		{
			Compendium_t::Events_t::eventUpdateMonster(getPlayer(), Compendium_t::CPDM_GHOST_PINGS, players[getPlayer()]->ghost.my, 1);
		}
	}

	return callout.doMessage;
}

void CalloutRadialMenu::sendCalloutText(CalloutRadialMenu::CalloutCommand cmd)
{
	if ( cmd == CALLOUT_CMD_CANCEL || cmd == CALLOUT_CMD_END )
	{
		return;
	}
	if ( multiplayer == CLIENT )
	{
		strcpy((char*)net_packet->data, "CALL");
		net_packet->data[4] = getPlayer();
		SDLNet_Write32(lockOnEntityUid, &net_packet->data[5]);
		net_packet->data[9] = (Uint8)cmd;
		SDLNet_Write32(clientCalloutHelpFlags, &net_packet->data[10]);
		net_packet->len = 14;
		if ( lockOnEntityUid == 0 )
		{
			Uint16 _x = std::min<Uint16>(std::max<int>(0.0, moveToX / 16), map.width - 1);
			Uint16 _y = std::min<Uint16>(std::max<int>(0.0, moveToY / 16), map.height - 1);
			SDLNet_Write16(_x, &net_packet->data[14]);
			SDLNet_Write16(_y, &net_packet->data[16]);
			net_packet->len = 18;
		}
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		sendPacketSafe(net_sock, -1, net_packet, 0);
	}
	else
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			std::string text = setCalloutText(nullptr, getCalloutKeyForCommand(cmd).c_str(), 0, cmd, SET_CALLOUT_WORLD_TEXT, i);
			if ( text != "" )
			{
				messagePlayerColor(i, MESSAGE_INTERACTION, playerColor(getPlayer(), colorblind_lobby, false),
					text.c_str());
			}
		}
	}
}

std::string CalloutRadialMenu::getCalloutKeyForCommand(CalloutRadialMenu::CalloutCommand cmd)
{
	if ( cmd == CALLOUT_CMD_LOOK )
	{
		return "look_at";
	}
	else if ( cmd == CALLOUT_CMD_HELP )
	{
		return "help";
	}
	else if ( cmd == CALLOUT_CMD_AFFIRMATIVE )
	{
		return "affirmative";
	}
	else if ( cmd == CALLOUT_CMD_THANKS )
	{
		return "thanks";
	}
	else if ( cmd == CALLOUT_CMD_NEGATIVE )
	{
		return "negative";
	}
	else if ( cmd == CALLOUT_CMD_MOVE )
	{
		return "move";
	}
	else if ( cmd == CALLOUT_CMD_SOUTH )
	{
		return "player_wave_1";
	}
	else if ( cmd == CALLOUT_CMD_SOUTHWEST )
	{
		return "player_wave_2";
	}
	else if ( cmd == CALLOUT_CMD_SOUTHEAST )
	{
		return "player_wave_3";
	}
	return "";
}

int CalloutRadialMenu::getPlayerForDirectPlayerCmd(const int player, const CalloutRadialMenu::CalloutCommand cmd)
{
	if ( cmd == CALLOUT_CMD_SOUTH )
	{
		if ( player == 0 )
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else if ( cmd == CALLOUT_CMD_SOUTHWEST )
	{
		if ( player == 0 )
		{
			return 2;
		}
		else
		{
			return player == 1 ? 2 : 1;
		}
	}
	else if ( cmd == CALLOUT_CMD_SOUTHEAST )
	{
		if ( player == 0 || player == 1 )
		{
			return 3;
		}
		else
		{
			return player == 2 ? 3 : 2;
		}
	}
	return -1;
}

void CalloutRadialMenu::drawCalloutMenu()
{
	auto player = players[gui_player];
	if ( !player->isLocalPlayer() )
	{
		closeCalloutMenuGUI();
		return;
	}

	Input& input = Input::inputs[gui_player];

	bool allowMenuCancel = true;
	if ( input.input("Call Out").input
		== input.input("MenuCancel").input )
	{
		allowMenuCancel = false;
	}

	if ( selectMoveTo )
	{
		if ( input.binaryToggle("MenuCancel") )
		{
			input.consumeBinaryToggle("MenuCancel");
			if ( allowMenuCancel )
			{
				input.consumeBindingsSharedWithBinding("MenuCancel");
				closeCalloutMenuGUI();
				Player::soundCancel();
			}
		}
		if ( calloutFrame )
		{
			calloutFrame->setDisabled(true);
		}
		return;
	}

	if ( !calloutFrame )
	{
		return;
	}

	calloutFrame->setSize(SDL_Rect{ players[gui_player]->camera_virtualx1(),
		players[gui_player]->camera_virtualy1(),
		players[gui_player]->camera_virtualWidth(),
		players[gui_player]->camera_virtualHeight() });

	int disableOption = 0;
	bool keepWheelOpen = false;

	Sint32 omousex = inputs.getMouse(gui_player, Inputs::OX);
	Sint32 omousey = inputs.getMouse(gui_player, Inputs::OY);

	std::map<int, Frame::image_t*> panelImages;
	std::map<int, Frame::image_t*> panelIcons;
	Frame* bannerFrame = nullptr;
	Field* bannerTxt = nullptr;
	Frame::image_t* bannerImgLeft = nullptr;
	Frame::image_t* bannerImgRight = nullptr;
	Frame::image_t* bannerImgCenter = nullptr;
	Uint32 textHighlightColor = followerBannerTextHighlightColor;

	if ( !bOpen && (!calloutFrame->isDisabled() || players[gui_player]->gui_mode == GUI_MODE_CALLOUT) )
	{
		closeCalloutMenuGUI();
		players[gui_player]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_CLOSE_ALL);
		return;
	}
	if ( calloutMenuIsOpen() && input.binaryToggle("MenuCancel") )
	{
		input.consumeBinaryToggle("MenuCancel");
		if ( allowMenuCancel || (optionSelected != -1 && optionSelected != CALLOUT_CMD_END && optionSelected != CALLOUT_CMD_SELECT) )
		{
			input.consumeBindingsSharedWithBinding("MenuCancel");
			closeCalloutMenuGUI();
			players[gui_player]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_CLOSE_ALL);
			Player::soundCancel();
			return;
		}
	}

	//if ( ticks % 50 == 0 )
	//{
	//	consoleCommand("/loadfollowerwheel");
	//}

	bool modifierPressed = false;
	bool modifierActiveForOption = false;
	if ( input.binary("Defend") )
	{
		modifierPressed = true;
	}

	if ( bOpen )
	{
		{
			const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
			real_t setpointDiff = fpsScale * std::max(.1, (1.0 - animTitle)) / 2.5;
			animTitle += setpointDiff;
			animTitle = std::min(1.0, animTitle);

			real_t setpointDiff2 = fpsScale * std::max(.01, (1.0 - animWheel)) / 2.0;
			animWheel += setpointDiff2;
			animWheel = std::min(1.0, animWheel);

			// shaking feedback for invalid action
			// constant decay for animation
			real_t setpointDiffX = fpsScale * 1.0 / 25.0;
			animInvalidAction -= setpointDiffX;
			animInvalidAction = std::max(0.0, animInvalidAction);
		}

		if ( optionSelected == CALLOUT_CMD_SOUTH
			|| optionSelected == CALLOUT_CMD_SOUTHEAST
			|| optionSelected == CALLOUT_CMD_SOUTHWEST )
		{
			int targetPlayer = getPlayerForDirectPlayerCmd(getPlayer(), (CalloutCommand)optionSelected);
			if ( targetPlayer < 0 || client_disconnected[targetPlayer] || (!Player::getPlayerInteractEntity(targetPlayer)) )
			{
				disableOption = true;
			}
			else
			{
				disableOption = false;
			}
		}
		else
		{
			disableOption = false;
		}

		bool menuConfirmOnGamepad = input.input("MenuConfirm").isBindingUsingGamepad();
		if ( menuConfirmOnGamepad )
		{
			if ( input.input("Call Out").input
				== input.input("MenuConfirm").input )
			{
				menuConfirmOnGamepad = false;
			}
		}
		bool menuLeftClickOnKeyboard = input.input("MenuLeftClick").isBindingUsingKeyboard() && !inputs.hasController(gui_player);

		// process commands if option selected on the wheel.
		if ( !(players[gui_player]->bControlEnabled && !gamePaused && !players[gui_player]->usingCommand()) )
		{
			// no action
		}
		else if ( (!menuToggleClick && !holdWheel
			&& !input.binaryToggle("Use")
			&& !input.binaryToggle("Call Out")
			&& !(input.binaryToggle("MenuConfirm") && menuConfirmOnGamepad)
			&& !(input.binaryToggle("MenuLeftClick") && menuLeftClickOnKeyboard))
			|| (menuToggleClick && (input.binaryToggle("Use") || input.binaryToggle("Call Out")))
			|| ((input.binaryToggle("MenuConfirm") && menuConfirmOnGamepad)
				|| (input.binaryToggle("MenuLeftClick") && menuLeftClickOnKeyboard)
				|| (input.binaryToggle("Use") && holdWheel))
			|| (!input.binaryToggle("Call Out") && holdWheel && !menuToggleClick)
			)
		{
			//bool usingShowCmdRelease = (!input.binaryToggle("Call Out") && holdWheel && !menuToggleClick);

			if ( menuToggleClick )
			{
				menuToggleClick = false;
				if ( optionSelected == -1 )
				{
					optionSelected = CALLOUT_CMD_CANCEL;
				}
			}

			input.consumeBinaryToggle("Use");
			input.consumeBinaryToggle("MenuConfirm");
			input.consumeBinaryToggle("MenuLeftClick");
			input.consumeBinaryToggle("Call Out");
			input.consumeBindingsSharedWithBinding("Use");
			input.consumeBindingsSharedWithBinding("MenuConfirm");
			input.consumeBindingsSharedWithBinding("MenuLeftClick");
			input.consumeBindingsSharedWithBinding("Call Out");

			if ( disableOption != 0 )
			{
				keepWheelOpen = true;
			}

			bool sfxPlayed = false;
			if ( disableOption != 0 )
			{
				animInvalidAction = 1.0;
				animInvalidActionTicks = ticks;
				//if ( !usingShowCmdRelease )
				//{
				//	// play bad feedback sfx
				//}
				playSound(90, 64);
				sfxPlayed = true;
			}

			if ( optionSelected != -1 )
			{
				holdWheel = false;
				if ( optionSelected != CALLOUT_CMD_SELECT )
				{
					if ( !sfxPlayed && optionSelected != CALLOUT_CMD_CANCEL )
					{
						//playSound(139, 64); // click
						sfxPlayed = true;
					}
				}
				else
				{
					playSound(399, 48); // ping
				}
				// return to shootmode and close guis etc. TODO: tidy up interface code into 1 spot?
				if ( !keepWheelOpen )
				{
					if ( optionSelected == CALLOUT_CMD_CANCEL )
					{
						players[gui_player]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_DONT_CLOSE_CALLOUTGUI);
					}
				}

				if ( disableOption == 0 )
				{
					if ( modifierPressed )
					{
						if ( stats[gui_player]->shield && itemCategory(stats[gui_player]->shield) == SPELLBOOK )
						{
							input.consumeBinaryToggle("Defend"); // don't try cast when menu closes.
						}
					}

					if ( lockOnEntityUid == 0 )
					{
						if ( (CalloutCommand)optionSelected == CALLOUT_CMD_AFFIRMATIVE
							|| (CalloutCommand)optionSelected == CALLOUT_CMD_NEGATIVE )
						{
							lockOnEntityUid = getPlayerUid(gui_player);
						}
						else if ( (CalloutCommand)optionSelected == CALLOUT_CMD_HELP )
						{
							lockOnEntityUid = getPlayerUid(gui_player);
						}
					}

					if ( (CalloutCommand)optionSelected == CALLOUT_CMD_SOUTH
						|| (CalloutCommand)optionSelected == CALLOUT_CMD_SOUTHWEST
						|| (CalloutCommand)optionSelected == CALLOUT_CMD_SOUTHEAST )
					{
						int toPlayer = getPlayerForDirectPlayerCmd(getPlayer(), (CalloutCommand)optionSelected);
						if ( toPlayer >= 0 )
						{
							lockOnEntityUid = getPlayerUid(toPlayer);
						}
					}

					if ( lockOnEntityUid )
					{
						if ( Entity* target = uidToEntity(lockOnEntityUid) )
						{
							Uint32 overrideUID = 0;
							if ( (target->behavior == &actPlayer || target->behavior == &actDeathGhost)
								&& target->skill[2] != getPlayer() )
							{
								if ( (CalloutCommand)optionSelected == CALLOUT_CMD_HELP )
								{
									lockOnEntityUid = getPlayerUid(getPlayer());
									target = uidToEntity(lockOnEntityUid);
								}
								else if ( (CalloutCommand)optionSelected == CALLOUT_CMD_AFFIRMATIVE )
								{
									target = Player::getPlayerInteractEntity(getPlayer());
									overrideUID = lockOnEntityUid;
									optionSelected = CALLOUT_CMD_THANKS;
								}
								else if ( (CalloutCommand)optionSelected == CALLOUT_CMD_LOOK
									|| (CalloutCommand)optionSelected == CALLOUT_CMD_NEGATIVE )
								{
									target = Player::getPlayerInteractEntity(getPlayer());
								}
								else if ( (CalloutCommand)optionSelected == CALLOUT_CMD_SOUTH
									|| (CalloutCommand)optionSelected == CALLOUT_CMD_SOUTHWEST
									|| (CalloutCommand)optionSelected == CALLOUT_CMD_SOUTHEAST )
								{
									int toPlayer = getPlayerForDirectPlayerCmd(getPlayer(), (CalloutCommand)optionSelected);
									if ( toPlayer >= 0 )
									{
										target = Player::getPlayerInteractEntity(getPlayer());
									}
								}
							}

							if ( createParticleCallout(target, (CalloutCommand)optionSelected, overrideUID) )
							{
								sendCalloutText((CalloutCommand)optionSelected);
							}
						}
					}
					else
					{
						if ( createParticleCallout((real_t)moveToX, (real_t)moveToY, -4, 0, (CalloutCommand)optionSelected) )
						{
							sendCalloutText((CalloutCommand)optionSelected);
						}
					}
				}

				if ( optionSelected == CALLOUT_CMD_CANCEL && !sfxPlayed )
				{
					Player::soundCancel();
				}

				if ( !keepWheelOpen )
				{
					closeCalloutMenuGUI();
					players[gui_player]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_CLOSE_ALL);
					return;
				}
				optionSelected = -1;
			}
			else
			{
				menuToggleClick = true;
			}
		}
	}

	if ( bOpen )
	{
		if ( !calloutGUIHasBeenCreated() )
		{
			createCalloutMenuGUI();
		}
		calloutFrame->setDisabled(false);

		auto bgFrame = calloutFrame->findFrame("wheel base");
		bgFrame->setOpacity(100.0 * animWheel);
		bannerFrame = calloutFrame->findFrame("banner frame");
		bannerImgLeft = bannerFrame->findImage("banner left");
		bannerImgRight = bannerFrame->findImage("banner right");
		bannerImgCenter = bannerFrame->findImage("banner center");
		bannerTxt = bannerFrame->findField("banner txt");
		bannerTxt->setText("");

		int direction = NORTH;
		const int midx = calloutFrame->getSize().w / 2;
		const int midy = calloutFrame->getSize().h / 2;
		for ( auto img : bgFrame->getImages() )
		{
			if ( direction < PANEL_DIRECTION_END )
			{
				panelImages[direction] = img;
				img->pos.x = getPanelEntriesForCallout()[direction].x + midx + CalloutRadialMenu::followerWheelFrameOffsetX;
				img->pos.y = getPanelEntriesForCallout()[direction].y + midy + CalloutRadialMenu::followerWheelFrameOffsetY;
				img->path = getPanelEntriesForCallout()[direction].path;
			}
			else if ( direction < 2 * PANEL_DIRECTION_END )
			{
				img->disabled = true;
				img->path = "";
				int direction2 = direction - PANEL_DIRECTION_END;
				panelIcons[direction2] = img;
				panelIcons[direction2]->pos.x = panelImages[direction2]->pos.x + getPanelEntriesForCallout()[direction2].icon_offsetx;
				panelIcons[direction2]->pos.y = panelImages[direction2]->pos.y + getPanelEntriesForCallout()[direction2].icon_offsety;
			}
			else if ( direction == 2 * PANEL_DIRECTION_END ) // center img
			{
				panelImages[PANEL_DIRECTION_END] = img;
				img->pos.x = getPanelEntriesForCallout()[PANEL_DIRECTION_END].x + midx + CalloutRadialMenu::followerWheelFrameOffsetX;
				img->pos.y = getPanelEntriesForCallout()[PANEL_DIRECTION_END].y + midy + CalloutRadialMenu::followerWheelFrameOffsetY;
				img->path = getPanelEntriesForCallout()[PANEL_DIRECTION_END].path;
			}
			++direction;
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

		radius = CalloutRadialMenu::followerWheelRadius;
		thickness = CalloutRadialMenu::followerWheelButtonThickness;
		real_t menuScale = yres / (real_t)Frame::virtualScreenY;
		radius *= menuScale;
		thickness *= menuScale;
		int centerButtonHighlightOffset = CalloutRadialMenu::followerWheelInnerCircleRadiusOffset;

		int highlight = -1;
		int i = 0;

		if ( inputs.hasController(gui_player) )
		{
			auto controller = inputs.getController(gui_player);
			if ( controller )
			{
				GameController::DpadDirection dir = controller->dpadDirToggle();
				if ( dir != GameController::DpadDirection::INVALID )
				{
					if ( !controller->virtualDpad.consumed )
					{
						Player::soundMovement();
					}
					controller->consumeDpadDirToggle();
					switch ( dir )
					{
					case GameController::DpadDirection::UP:
						highlight = 0;
						break;
					case GameController::DpadDirection::UPLEFT:
						highlight = 1;
						break;
					case GameController::DpadDirection::LEFT:
						highlight = 2;
						break;
					case GameController::DpadDirection::DOWNLEFT:
						highlight = 3;
						break;
					case GameController::DpadDirection::DOWN:
						highlight = 4;
						break;
					case GameController::DpadDirection::DOWNRIGHT:
						highlight = 5;
						break;
					case GameController::DpadDirection::RIGHT:
						highlight = 6;
						break;
					case GameController::DpadDirection::UPRIGHT:
						highlight = 7;
						break;
					default:
						break;
					}
					real_t angleMiddleForOption = PI / 2 + dir * (2 * PI / numoptions);
					omousex = centerx + (radius + thickness) * .75 * cos(angleMiddleForOption);
					omousey = centery + (radius + thickness) * .75 * sin(angleMiddleForOption);
					inputs.setMouse(gui_player, Inputs::OX, omousex);
					inputs.setMouse(gui_player, Inputs::OY, omousey);
					inputs.setMouse(gui_player, Inputs::X, omousex);
					inputs.setMouse(gui_player, Inputs::Y, omousey);

					if ( highlight != -1 )
					{
						inputs.getVirtualMouse(gui_player)->draw_cursor = false;
					}
				}
			}
		}

		bool mouseInCenterButton = sqrt(pow((omousex - menuX), 2) + pow((omousey - menuY), 2)) < (radius - thickness);

		angleStart = PI / 2 - (PI / numoptions);
		angleMiddle = angleStart + PI / numoptions;
		angleEnd = angleMiddle + PI / numoptions;

		const real_t mouseDetectionAdjust = PI / 128;
		for ( i = 0; i < numoptions; ++i )
		{
			// see if mouse cursor is within an option.
			if ( highlight == -1 )
			{
				if ( !mouseInCenterButton )
				{
					real_t x1 = menuX + (radius + thickness + 45) * cos(angleEnd + mouseDetectionAdjust);
					real_t y1 = menuY - (radius + thickness + 45) * sin(angleEnd + mouseDetectionAdjust);
					real_t x2 = menuX + 5 * cos(angleMiddle);
					real_t y2 = menuY - 5 * sin(angleMiddle);
					real_t x3 = menuX + (radius + thickness + 45) * cos(angleStart - mouseDetectionAdjust);
					real_t y3 = menuY - (radius + thickness + 45) * sin(angleStart - mouseDetectionAdjust);
					real_t a = ((y2 - y3) * (omousex - x3) + (x3 - x2) * (omousey - y3)) / ((y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3));
					real_t b = ((y3 - y1) * (omousex - x3) + (x1 - x3) * (omousey - y3)) / ((y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3));
					real_t c = 1 - a - b;
					if ( (0 <= a && a <= 1) && (0 <= b && b <= 1) && (0 <= c && c <= 1) )
					{
						//barycentric calc for figuring if mouse point is within triangle.
						highlight = i;
					}
				}
				if ( !inputs.hasController(gui_player) )
				{
					if ( highlight != -1 && optionSelected != highlight && optionSelected != -1 )
					{
						Player::soundMovement();
					}
				}
			}

			SDL_Rect txt;
			txt.x = src.x + src.w * cos(angleMiddle);
			txt.y = src.y - src.h * sin(angleMiddle);
			txt.w = 0;
			txt.h = 0;

			// draw the text for the menu wheel.

			bool lockedOption = false;
			panelIcons[i]->color = makeColor(255, 255, 255, 255);
			{
				/*if ( i == ALLY_CMD_ATTACK_SELECT )
				{
					if ( i == highlight )
					{
						panelIcons[i]->path = iconEntries["leader_attack"].path_hover;
						setCalloutBannerText(gui_player, bannerTxt, "leader_attack", "default", textHighlightColor);
					}
					else
					{
						panelIcons[i]->path = iconEntries["leader_attack"].path;
					}
				}
				else if ( i == ALLY_CMD_MOVETO_SELECT )
				{
					if ( i == highlight )
					{
						panelIcons[i]->path = iconEntries["leader_moveto"].path_hover;
						setCalloutBannerText(gui_player, bannerTxt, "leader_moveto", "default", textHighlightColor);
					}
					else
					{
						panelIcons[i]->path = iconEntries["leader_moveto"].path;
					}
				}
				else*/
				{
					if ( i == CALLOUT_CMD_LOOK )
					{
						if ( i == highlight )
						{
							panelIcons[i]->path = iconEntries["look_at"].path_hover;
							setCalloutText(bannerTxt, "look_at", textHighlightColor, (CalloutCommand)i, SET_CALLOUT_BANNER_TEXT, -1);
						}
						else
						{
							panelIcons[i]->path = iconEntries["look_at"].path;
						}
					}
					else if ( i == CALLOUT_CMD_HELP )
					{
						if ( i == highlight )
						{
							panelIcons[i]->path = iconEntries["help"].path_hover;
							setCalloutText(bannerTxt, "help", textHighlightColor, (CalloutCommand)i, SET_CALLOUT_BANNER_TEXT, -1);
						}
						else
						{
							panelIcons[i]->path = iconEntries["help"].path;
						}
					}
					else if ( i == CALLOUT_CMD_AFFIRMATIVE )
					{
						if ( i == highlight )
						{
							panelIcons[i]->path = iconEntries["affirmative"].path_hover;
							setCalloutText(bannerTxt, "affirmative", textHighlightColor, (CalloutCommand)i, SET_CALLOUT_BANNER_TEXT, -1);
						}
						else
						{
							panelIcons[i]->path = iconEntries["affirmative"].path;
						}
					}
					else if ( i == CALLOUT_CMD_NEGATIVE )
					{
						if ( i == highlight )
						{
							panelIcons[i]->path = iconEntries["negative"].path_hover;
							setCalloutText(bannerTxt, "negative", textHighlightColor, (CalloutCommand)i, SET_CALLOUT_BANNER_TEXT, -1);
						}
						else
						{
							panelIcons[i]->path = iconEntries["negative"].path;
						}
					}
					else if ( i == CALLOUT_CMD_MOVE )
					{
						if ( i == highlight )
						{
							panelIcons[i]->path = iconEntries["move"].path_hover;
							setCalloutText(bannerTxt, "move", textHighlightColor, (CalloutCommand)i, SET_CALLOUT_BANNER_TEXT, -1);
						}
						else
						{
							panelIcons[i]->path = iconEntries["move"].path;
						}
					}
					else if ( i == CALLOUT_CMD_SOUTH )
					{
						int targetPlayer = getPlayerForDirectPlayerCmd(getPlayer(), (CalloutCommand)i);
						if ( targetPlayer < 0 || client_disconnected[targetPlayer] || !Player::getPlayerInteractEntity(targetPlayer) )
						{
							lockedOption = true;
						}
						if ( i == highlight )
						{
							setCalloutText(bannerTxt, "player_wave_1", textHighlightColor, (CalloutCommand)i, SET_CALLOUT_BANNER_TEXT, -1);
						}

						std::string key = (i == highlight) ? "tag_btn_player_wave_hover" : "tag_btn_player_wave";
						if ( lockedOption )
						{
							panelIcons[i]->color = makeColor(255, 255, 255, 64);
						}
						
						{
							panelIcons[i]->path = worldIconEntries[key].getPlayerIconPath(targetPlayer);
						}
					}
					else if ( i == CALLOUT_CMD_SOUTHWEST )
					{
						int targetPlayer = getPlayerForDirectPlayerCmd(getPlayer(), (CalloutCommand)i);
						if ( targetPlayer < 0 || client_disconnected[targetPlayer] || !Player::getPlayerInteractEntity(targetPlayer) )
						{
							lockedOption = true;
						}
						if ( i == highlight )
						{
							setCalloutText(bannerTxt, "player_wave_2", textHighlightColor, (CalloutCommand)i, SET_CALLOUT_BANNER_TEXT, -1);
						}
						
						std::string key = (i == highlight) ? "tag_btn_player_wave_hover" : "tag_btn_player_wave";
						if ( lockedOption )
						{
							panelIcons[i]->color = makeColor(255, 255, 255, 64);
						}
						
						{
							panelIcons[i]->path = worldIconEntries[key].getPlayerIconPath(targetPlayer);
						}
					}
					else if ( i == CALLOUT_CMD_SOUTHEAST )
					{
						int targetPlayer = getPlayerForDirectPlayerCmd(getPlayer(), (CalloutCommand)i);
						if ( targetPlayer < 0 || client_disconnected[targetPlayer] || !Player::getPlayerInteractEntity(targetPlayer) )
						{
							lockedOption = true;
						}
						if ( i == highlight )
						{
							setCalloutText(bannerTxt, "player_wave_3", textHighlightColor, (CalloutCommand)i, SET_CALLOUT_BANNER_TEXT, -1);
						}
						
						std::string key = (i == highlight) ? "tag_btn_player_wave_hover" : "tag_btn_player_wave";
						if ( lockedOption )
						{
							panelIcons[i]->color = makeColor(255, 255, 255, 64);
						}
						
						{
							panelIcons[i]->path = worldIconEntries[key].getPlayerIconPath(targetPlayer);
						}
					}
				}
			}

			if ( lockedOption )
			{
				if ( highlight == i && !mouseInCenterButton )
				{
					panelImages[i]->path = getPanelEntriesForCallout()[i].path_hover;
				}
				else
				{
					panelImages[i]->path = getPanelEntriesForCallout()[i].path;
				}
			}
			else if ( highlight == i && !mouseInCenterButton )
			{
				panelImages[i]->path = getPanelEntriesForCallout()[i].path_hover;
			}


			if ( /*!lockedOption &&*/ panelIcons[i]->path != "" )
			{
				if ( auto imgGet = Image::get(panelIcons[i]->path.c_str()) )
				{
					panelIcons[i]->disabled = false;
					panelIcons[i]->pos.w = imgGet->getWidth();
					panelIcons[i]->pos.h = imgGet->getHeight();
					panelIcons[i]->pos.x -= panelIcons[i]->pos.w / 2;
					panelIcons[i]->pos.y -= panelIcons[i]->pos.h / 2;
				}
			}

			angleStart += 2 * PI / numoptions;
			angleMiddle = angleStart + PI / numoptions;
			angleEnd = angleMiddle + PI / numoptions;
		}

		// draw center text.
		if ( mouseInCenterButton )
		{
			bool mouseInCenterHighlightArea = sqrt(pow((omousex - menuX), 2) + pow((omousey - menuY), 2)) < (radius - thickness + centerButtonHighlightOffset);
			if ( mouseInCenterHighlightArea )
			{
				panelImages[PANEL_DIRECTION_END]->path = getPanelEntriesForCallout()[PANEL_DIRECTION_END].path_hover;
			}

			highlight = -1;
		}

		if ( optionSelected == -1 && disableOption == 0 && highlight != -1 )
		{
			// in case optionSelected is cleared, but we're still highlighting text (happens on next frame when clicking on disabled option.)
			if ( highlight == CALLOUT_CMD_SOUTH
				|| highlight == CALLOUT_CMD_SOUTHEAST
				|| highlight == CALLOUT_CMD_SOUTHWEST )
			{
				int targetPlayer = getPlayerForDirectPlayerCmd(getPlayer(), (CalloutCommand)highlight);
				if ( targetPlayer < 0 || client_disconnected[targetPlayer] || !Player::getPlayerInteractEntity(targetPlayer) )
				{
					disableOption = true;
				}
				else
				{
					disableOption = false;
				}
			}
			else
			{
				disableOption = false;
			}
		}

		if ( highlight == -1 )
		{
			setCalloutBannerTextUnformatted(gui_player, bannerTxt, "cancel", "default", hudColors.characterSheetRed);
		}

		bool disableActionGlyph = false;
		bool missingSkillLevel = false;
		if ( disableOption != 0 )
		{
			disableActionGlyph = true;
		}
		//	if ( disableOption == -2 ) // disabled due to cooldown
		//	{
		//		setCalloutBannerText(gui_player, bannerTxt, "invalid_action", "rest_cooldown", hudColors.characterSheetRed);
		//	}
		//	else if ( disableOption == -1 ) // disabled due to creature type
		//	{
		//		auto& textMap = FollowerMenu[gui_player].iconEntries["invalid_action"].text_map["command_unavailable"];
		//		setCalloutBannerTextFormatted(gui_player, bannerTxt, hudColors.characterSheetRed,
		//			textMap.second, textMap.first.c_str(),
		//			getMonsterLocalizedName(HUMAN).c_str());
		//	}
		//	else if ( disableOption == -3 ) // disabled due to tinkerbot quality
		//	{
		//		auto& textMap = FollowerMenu[gui_player].iconEntries["invalid_action"].text_map["tinker_quality_low"];
		//		setCalloutBannerTextFormatted(gui_player, bannerTxt, hudColors.characterSheetRed,
		//			textMap.second, textMap.first.c_str(),
		//			getMonsterLocalizedName(HUMAN).c_str());
		//	}
		//	else
		//	{
		//		std::string requirement = "";
		//		std::string current = "";
		//		int requirementVal = 0;
		//		int currentVal = 0;
		//		if ( highlight >= ALLY_CMD_DEFEND && highlight <= ALLY_CMD_END && highlight != CALLOUT_CMD_CANCEL )
		//		{
		//			switch ( std::min(disableOption, SKILL_LEVEL_LEGENDARY) )
		//			{
		//			case 0:
		//				requirement = Language::get(363);
		//				requirementVal = 0;
		//				break;
		//			case SKILL_LEVEL_NOVICE:
		//				requirement = Language::get(364);
		//				requirementVal = SKILL_LEVEL_NOVICE;
		//				break;
		//			case SKILL_LEVEL_BASIC:
		//				requirement = Language::get(365);
		//				requirementVal = SKILL_LEVEL_BASIC;
		//				break;
		//			case SKILL_LEVEL_SKILLED:
		//				requirement = Language::get(366);
		//				requirementVal = SKILL_LEVEL_SKILLED;
		//				break;
		//			case SKILL_LEVEL_EXPERT:
		//				requirement = Language::get(367);
		//				requirementVal = SKILL_LEVEL_EXPERT;
		//				break;
		//			case SKILL_LEVEL_MASTER:
		//				requirement = Language::get(368);
		//				requirementVal = SKILL_LEVEL_MASTER;
		//				break;
		//			case SKILL_LEVEL_LEGENDARY:
		//				requirement = Language::get(369);
		//				requirementVal = SKILL_LEVEL_LEGENDARY;
		//				break;
		//			default:
		//				break;
		//			}
		//			requirement.erase(std::remove(requirement.begin(), requirement.end(), ' '), requirement.end()); // trim whitespace

		//			current = Language::get(363);
		//			current.erase(std::remove(current.begin(), current.end(), ' '), current.end()); // trim whitespace
		//			currentVal = 0;
		//		}

		//		auto& textMap = FollowerMenu[gui_player].iconEntries["invalid_action"].text_map["skill_missing_leader"];
		//		setFollowerBannerTextFormatted(gui_player, bannerTxt, hudColors.characterSheetRed,
		//			textMap.second, textMap.first.c_str(),
		//			currentVal, requirementVal);
		//		missingSkillLevel = true;
		//	}
		//}

		auto wheelSkillImg = bannerFrame->findImage("skill img");
		wheelSkillImg->disabled = true;
		auto wheelStatImg = bannerFrame->findImage("stat img");
		wheelStatImg->disabled = true;

		bannerFrame->setDisabled(false);
		if ( auto textGet = bannerTxt->getTextObject() )
		{
			SDL_Rect txtPos = bannerTxt->getSize();
			if ( !strcmp(bannerTxt->getText(), "") && txtPos.w == 0 )
			{
				txtPos.w = 82;
			}
			else if ( strcmp(bannerTxt->getText(), "") )
			{
				txtPos.w = textGet->getWidth();
			}

			auto bannerGlyph = bannerFrame->findImage("banner glyph");
			bannerGlyph->disabled = true;
			if ( inputs.hasController(gui_player) )
			{
				bannerGlyph->path = Input::inputs[gui_player].getGlyphPathForBinding("MenuConfirm");
			}
			else
			{
				bannerGlyph->path = Input::inputs[gui_player].getGlyphPathForBinding("MenuLeftClick");
			}
			//bannerGlyph->path = Input::inputs[gui_player].getGlyphPathForBinding("Use");
			auto bannerGlyphModifier = bannerFrame->findImage("banner modifier glyph");
			bannerGlyphModifier->disabled = true;
			bannerGlyphModifier->path = Input::inputs[gui_player].getGlyphPathForBinding("Defend");
			if ( auto imgGet = Image::get(bannerGlyph->path.c_str()) )
			{
				bannerGlyph->pos.w = imgGet->getWidth();
				bannerGlyph->pos.h = imgGet->getHeight();
				bannerGlyph->disabled = disableActionGlyph || !strcmp(bannerTxt->getText(), "");
			}
			if ( auto imgGet = Image::get(bannerGlyphModifier->path.c_str()) )
			{
				bannerGlyphModifier->pos.w = imgGet->getWidth();
				bannerGlyphModifier->pos.h = imgGet->getHeight();
				bannerGlyphModifier->disabled = bannerGlyph->disabled || !modifierActiveForOption;
			}

			if ( !bannerGlyph->disabled )
			{
				animInvalidAction = 0.0;
			}

			bannerImgCenter->pos.w = txtPos.w + 16
				+ (bannerGlyph->disabled ? 0 : ((bannerGlyph->pos.w + 8) / 2))
				+ (bannerGlyphModifier->disabled ? 0 : (bannerGlyphModifier->pos.w + 2));
			int missingSkillLevelIconWidth = 0;
			if ( missingSkillLevel )
			{
				missingSkillLevelIconWidth = wheelStatImg->pos.w + wheelSkillImg->pos.w + 8;
			}
			bannerImgCenter->pos.w += missingSkillLevelIconWidth / 2;
			const int totalWidth = bannerImgLeft->pos.w + bannerImgRight->pos.w + bannerImgCenter->pos.w;

			const int midx = calloutFrame->getSize().w / 2;
			const int midy = calloutFrame->getSize().h / 2;

			SDL_Rect bannerSize = bannerFrame->getSize();
			bannerSize.w = totalWidth;
			bannerSize.x = midx - (totalWidth / 2);
			bannerSize.y = midy + CalloutRadialMenu::followerWheelRadius + CalloutRadialMenu::followerWheelButtonThickness + 4;
			if ( players[gui_player]->bUseCompactGUIHeight() )
			{
				bannerSize.y -= 16;
			}
			//bannerSize.y += 32 * (1.0 - animTitle);
			bannerFrame->setSize(bannerSize);
			bannerFrame->setOpacity(100.0 * animTitle);
			bannerImgLeft->pos.x = 0;
			bannerImgCenter->pos.x = bannerImgLeft->pos.x + bannerImgLeft->pos.w;
			bannerImgRight->pos.x = bannerImgCenter->pos.x + bannerImgCenter->pos.w;

			txtPos.x = bannerImgCenter->pos.x + (bannerImgCenter->pos.w / 2) - (txtPos.w / 2);
			txtPos.x += bannerGlyph->disabled ? 0 : ((bannerGlyph->pos.w + 8) / 2);
			txtPos.x += bannerGlyphModifier->disabled ? 0 : ((bannerGlyphModifier->pos.w + 0) / 2);
			if ( missingSkillLevel )
			{
				txtPos.x -= (missingSkillLevelIconWidth / 2) - 4;
			}
			if ( txtPos.x % 2 == 1 )
			{
				++txtPos.x;
			}
			if ( animInvalidAction > 0.01 )
			{
				txtPos.x += -2 + 2 * (cos(animInvalidAction * 4 * PI));
			}
			txtPos.y = 17;
			bannerTxt->setSize(txtPos);

			if ( missingSkillLevel )
			{
				wheelSkillImg->pos.x = txtPos.x + txtPos.w;
				wheelSkillImg->pos.y = txtPos.y - 3;
				wheelSkillImg->disabled = false;

				wheelStatImg->pos.x = wheelSkillImg->pos.x + wheelSkillImg->pos.w;
				wheelStatImg->pos.y = wheelSkillImg->pos.y;
				wheelStatImg->disabled = false;
			}

			bannerGlyph->pos.x = txtPos.x - bannerGlyph->pos.w - 8;
			if ( bannerGlyph->pos.x % 2 == 1 )
			{
				++bannerGlyph->pos.x;
			}
			bannerGlyph->pos.y = txtPos.y + txtPos.h / 2 - bannerGlyph->pos.h / 2;
			if ( bannerGlyph->pos.y % 2 == 1 )
			{
				bannerGlyph->pos.y -= 1;
			}
			bannerSize.h = std::max(40, bannerGlyph->pos.y + bannerGlyph->pos.h);
			if ( !bannerGlyphModifier->disabled )
			{
				bannerGlyphModifier->pos.x = txtPos.x - bannerGlyphModifier->pos.w - 8;
				bannerGlyph->pos.x = bannerGlyphModifier->pos.x - bannerGlyph->pos.w - 2;

				if ( bannerGlyphModifier->pos.x % 2 == 1 )
				{
					++bannerGlyphModifier->pos.x;
				}
				bannerGlyphModifier->pos.y = txtPos.y + txtPos.h / 2 - bannerGlyphModifier->pos.h / 2;
				if ( bannerGlyphModifier->pos.y % 2 == 1 )
				{
					bannerGlyphModifier->pos.y -= 1;
				}
				bannerSize.h = std::max(bannerSize.h, bannerGlyphModifier->pos.y + bannerGlyphModifier->pos.h);
			}
			bannerFrame->setSize(bannerSize);

			auto wheelTitleText = bgFrame->findField("wheel title");
			if ( !strcmp(wheelTitleText->getText(), "") )
			{
				char buf[128] = "";
				int spaces = 0;
				int spaces2 = 0;

				if ( Entity* target = uidToEntity(lockOnEntityUid) )
				{
					bool allowed = allowedInteractEntity(*target, true);
					if ( allowed )
					{
						snprintf(buf, sizeof(buf), "%s...", interactText);
					}
					else
					{
						snprintf(buf, sizeof(buf), Language::get(4348));
					}
					spaces = 1;
				}
				else
				{
					snprintf(buf, sizeof(buf), Language::get(4348));
					spaces = 1;
				}

				for ( int c = 0; c <= strlen(buf); ++c )
				{
					if ( buf[c] == '\0' )
					{
						break;
					}
					if ( buf[c] == ' ' )
					{
						++spaces2;
					}
				}
				wheelTitleText->setText(buf);
				wheelTitleText->clearWordsToHighlight();
				int wordIndex = 2;
				while ( spaces2 >= spaces ) // every additional space means +1 word to highlight for the monster's name
				{
					wheelTitleText->addWordToHighlight(wordIndex, followerTitleHighlightColor);
					--spaces2;
					++wordIndex;
				}
			}
			SDL_Rect titlePos = wheelTitleText->getSize();
			if ( auto textGet2 = wheelTitleText->getTextObject() )
			{
				titlePos.w = textGet2->getWidth();
				titlePos.x = bannerSize.x + bannerSize.w / 2 - (titlePos.w / 2);
				if ( titlePos.x % 2 == 1 )
				{
					++titlePos.x;
				}
				titlePos.y = midy - CalloutRadialMenu::followerWheelRadius - CalloutRadialMenu::followerWheelButtonThickness - 24;
				titlePos.y -= 32 * (1.0 - animTitle);
				++titlePos.y; // add 1 to be even pixeled
				wheelTitleText->setSize(titlePos);
			}
		}

		if ( !keepWheelOpen )
		{
			optionSelected = highlight; // don't reselect if we're keeping the wheel open by using a toggle option.
		}
	}
}

bool CalloutRadialMenu::allowedInteractEntity(Entity& selectedEntity, bool updateInteractText)
{
	if ( optionSelected != CALLOUT_CMD_SELECT )
	{
		return false;
	}

	if ( !players[gui_player] || !Player::getPlayerInteractEntity(gui_player) )
	{
		return false;
	}

	bool interactItems = true; //allowedInteractItems(followerStats->type) || allowedInteractFood(followerStats->type);
	bool interactWorld = true; //allowedInteractWorld(followerStats->type);
	bool enableAttack = true;

	if ( updateInteractText )
	{
		strcpy(interactText, Language::get(4347)); // "Callout "
	}

	/*if ( selectedEntity.behavior == &actTorch && interactWorld )
	{
		if ( updateInteractText )
		{
			strcat(interactText, items[TOOL_TORCH].getIdentifiedName());
		}
	}*/
	if ( (selectedEntity.behavior == &actSwitch || 
		selectedEntity.behavior == &actSwitchWithTimer ||
		selectedEntity.sprite == 184) && interactWorld )
	{
		if ( updateInteractText )
		{
			strcat(interactText, Language::get(4044)); // "switch"
		}
	}
	else if ( (selectedEntity.behavior == &actTeleportShrine) && (interactWorld || interactItems || enableAttack) )
	{
		if ( updateInteractText )
		{
			strcat(interactText, Language::get(4309)); // "shrine"
		}
	}
	else if ( (selectedEntity.behavior == &::actDaedalusShrine) && interactWorld )
	{
		if ( updateInteractText )
		{
			strcat(interactText, Language::get(6261)); // "shrine"
		}
	}
	else if ( (selectedEntity.behavior == &actTeleporter) && interactWorld )
	{
		if ( updateInteractText )
		{
			switch ( selectedEntity.teleporterType )
			{
			case 0:
			case 1:
				strcat(interactText, Language::get(4310)); // "ladder"
				break;
			case 2:
				strcat(interactText, Language::get(4311)); // "portal"
				break;
			default:
				break;
			}
		}
	}
	else if ( (selectedEntity.behavior == &actBell) && interactWorld )
	{
		if ( updateInteractText )
		{
			strcat(interactText, Language::get(6270)); // "bell"
		}
	}
	else if ( selectedEntity.behavior == &actLadder )
	{
		if ( updateInteractText )
		{
			if ( secretlevel && selectedEntity.skill[3] == 1 ) // secret ladder
			{
				strcat(interactText, Language::get(4360)); // "secret exit"
			}
			else if ( !secretlevel && selectedEntity.skill[3] == 1 ) // secret ladder
			{
				strcat(interactText, Language::get(4359)); // "secret entrance" 
			}
			else
			{
				strcat(interactText, Language::get(4361)); // "level exit" 
			}
		}
	}
	else if ( selectedEntity.behavior == &actPortal )
	{
		if ( updateInteractText )
		{
			if ( selectedEntity.skill[3] == 0 ) // secret entrance portal
			{
				if ( secretlevel )
				{
					strcat(interactText, Language::get(4360)); // "secret exit" 
				}
				else
				{
					strcat(interactText, Language::get(4359)); // "secret entrance" 
				}
			}
			else
			{
				if ( !strcmp(map.name, "Hell") )
				{
					strcat(interactText, Language::get(4361)); // "level exit" 
				}
				else if ( !strcmp(map.name, "Mages Guild") )
				{
					strcat(interactText, Language::get(4361)); // "level exit"
				}
				else
				{
					strcat(interactText, Language::get(4361)); // "level exit"
				}
			}
		}
	}
	else if ( selectedEntity.behavior == &::actMidGamePortal )
	{
		if ( updateInteractText )
		{
			strcat(interactText, Language::get(4311)); // "portal"
		}
	}
	else if ( selectedEntity.behavior == &actCustomPortal )
	{
		if ( updateInteractText )
		{
			if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
			{
				strcat(interactText, Language::get(4361)); // "level exit"
			}
			else
			{
				if ( selectedEntity.portalCustomSpriteAnimationFrames > 0 )
				{
					strcat(interactText, Language::get(4361)); // "level exit"
				}
				else
				{
					strcat(interactText, Language::get(4361)); // "level exit"
				}
			}
		}
	}
	else if ( selectedEntity.behavior == &::actExpansionEndGamePortal
		|| selectedEntity.behavior == &actWinningPortal )
	{
		if ( updateInteractText )
		{
			strcat(interactText, Language::get(4311)); // "portal"
		}
	}
	else if ( selectedEntity.behavior == &actBomb || selectedEntity.behavior == &actBeartrap )
	{
		if ( updateInteractText )
		{
			if ( selectedEntity.behavior == &actBomb )
			{
				if ( selectedEntity.skill[21] >= WOODEN_SHIELD && selectedEntity.skill[21] < NUMITEMS )
				{
					strcat(interactText, items[selectedEntity.skill[21]].getIdentifiedName());
				}
			}
			else if ( selectedEntity.behavior == &actBeartrap )
			{
				strcat(interactText, items[TOOL_BEARTRAP].getIdentifiedName());
			}
		}
	}
	else if ( selectedEntity.behavior == &actBoulderTrapHole
		&& interactWorld )
	{
		if ( updateInteractText )
		{
			strcat(interactText, Language::get(4349)); // "trap"
		}
	}
	else if ( selectedEntity.behavior == &actBoulder
		&& interactWorld )
		{
			if ( updateInteractText )
			{
				strcat(interactText, Language::get(4358)); // "boulder"
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
					strcat(interactText, items[selectedEntity.skill[10]].getUnidentifiedName());
				}
				else
				{
					strcat(interactText, items[selectedEntity.skill[10]].getIdentifiedName());
				}
			}
			else
			{
				strcat(interactText, Language::get(4046)); // "item"
			}
		}
	}
	else if ( selectedEntity.behavior == &actGoldBag )
	{
		if ( updateInteractText )
		{
			strcat(interactText, Language::get(4353)); // "gold"
		}
	}
	else if ( selectedEntity.behavior == &actFountain )
	{
		if ( updateInteractText )
		{
			strcat(interactText, Language::get(4355)); // "fountain"
		}
	}
	else if ( selectedEntity.behavior == &actSink )
	{
		if ( updateInteractText )
		{
			strcat(interactText, Language::get(4354)); // "sink"
		}
	}
	else if ( selectedEntity.behavior == &actHeadstone )
	{
		if ( updateInteractText )
		{
			strcat(interactText, Language::get(4357)); // "grave"
		}
	}
	else if ( selectedEntity.behavior == &actCampfire )
	{
		if ( updateInteractText )
		{
			strcat(interactText, Language::get(4365)); // "campfire"
		}
	}
	else if ( selectedEntity.behavior == &actPowerCrystal || selectedEntity.behavior == &actPowerCrystalBase )
	{
		if ( updateInteractText )
		{
			strcat(interactText, Language::get(4356)); // "crystal"
		}
	}
	else if ( selectedEntity.behavior == &actChestLid || selectedEntity.behavior == &actChest
		|| selectedEntity.isInertMimic() )
	{
		if ( updateInteractText )
		{
			strcat(interactText, Language::get(675)); // "chest"
		}
	}
	else if ( selectedEntity.behavior == &actMonster && enableAttack && selectedEntity.getMonsterTypeFromSprite() != GYROBOT )
	{
		if ( updateInteractText )
		{
			int monsterType = selectedEntity.getMonsterTypeFromSprite();
			strcat(interactText, getMonsterLocalizedName((Monster)monsterType).c_str());
		}
	}
	else if ( selectedEntity.behavior == &actPlayer || selectedEntity.behavior == &actDeathGhost )
	{
		if ( updateInteractText )
		{
			int playernum = selectedEntity.skill[2];
			if ( playernum >= 0 && playernum < MAXPLAYERS )
			{
				char shortname[32];
				stringCopy(shortname, stats[playernum]->name, sizeof(shortname), 22);
				std::string nameStr = shortname;
				nameStr = messageSanitizePercentSign(nameStr, nullptr);
				strcat(interactText, nameStr.c_str());
			}
		}
	}
	else if ( selectedEntity.behavior == &actColliderDecoration && selectedEntity.isDamageableCollider() )
	{
		if ( updateInteractText )
		{
			strcat(interactText, Language::get(selectedEntity.getColliderLangName()));
		}
	}
	else if ( selectedEntity.behavior == &actFloorDecoration && selectedEntity.sprite == 991 )
	{
		if ( updateInteractText )
		{
			strcat(interactText, Language::get(4363)); // "sign"
		}
	}
	else if ( selectedEntity.behavior == &actPedestalBase )
	{
		if ( updateInteractText )
		{
			strcat(interactText, Language::get(4364));
		}
	}
	else
	{
		if ( updateInteractText )
		{
			strcpy(interactText, "");
		}
		return false;
	}
	return true;
}