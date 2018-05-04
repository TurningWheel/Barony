/*-------------------------------------------------------------------------------

	BARONY
	File: interface.hpp
	Desc: contains interface related declarations

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include "../main.hpp"
#include "../game.hpp"

class Item;

typedef struct damageIndicator_t
{
	double x, y;  // x and y of the attacker in world coordinates
	double alpha; // alpha value of the indicator
	node_t* node; // node in the damageIndicator list
	Sint32 ticks; // birthtime of the damage indicator
} damageIndicator_t;
extern list_t damageIndicators;

#define STATUS_BAR_Y_OFFSET (status_bmp->h * uiscale_chatlog)
#define INVENTORY_SLOTSIZE (40 * uiscale_inventory)
#define STATUS_X (xres / 2 - status_bmp->w * uiscale_chatlog / 2)
#define STATUS_Y (yres - STATUS_BAR_Y_OFFSET)
#define HOTBAR_START_X ((xres / 2) - (5 * hotbar_img->w * uiscale_hotbar))
extern bool hide_statusbar;
extern real_t uiscale_chatlog;
extern real_t uiscale_playerbars;
extern bool uiscale_charactersheet;
extern bool uiscale_skillspage;
extern real_t uiscale_hotbar;
extern real_t uiscale_inventory;

extern char enemy_name[128];
extern Sint32 enemy_hp, enemy_maxhp;
extern Uint32 enemy_timer;
extern Uint32 enemy_bar_color[MAXPLAYERS];

#ifndef SHOPWINDOW_SIZE
#define SHOPWINDOW_SIZE
#define SHOPWINDOW_SIZEX 576
#define SHOPWINDOW_SIZEY 324
#endif

static const int GUI_MODE_NONE = -1; //GUI closed, ingame & playing.
static const int GUI_MODE_INVENTORY = 0;
static const int GUI_MODE_MAGIC = 1;
static const int GUI_MODE_SHOP = 2;
extern int gui_mode;

/*
 * So that the cursor jumps back to the hotbar instead of the inventory if a picked up hotbar item is canceled.
 * Its value indicates which hotbar slot it's from.
 * -1 means it's not from the hotbar.
 */
extern int selectedItemFromHotbar;

extern SDL_Surface* font12x12_small_bmp;
extern SDL_Surface* backdrop_minotaur_bmp;
extern SDL_Surface* backdrop_blessed_bmp;
extern SDL_Surface* backdrop_cursed_bmp;
extern SDL_Surface* status_bmp;
extern SDL_Surface* character_bmp;
extern SDL_Surface* hunger_bmp;
extern SDL_Surface* minotaur_bmp;
extern SDL_Surface* textup_bmp;
extern SDL_Surface* textdown_bmp;
extern SDL_Surface* attributesleft_bmp, *attributesleftunclicked_bmp;
extern SDL_Surface* attributesright_bmp, *attributesrightunclicked_bmp;
extern SDL_Surface* button_bmp, *smallbutton_bmp, *invup_bmp, *invdown_bmp;
extern SDL_Surface* inventory_bmp, *inventoryoption_bmp, *inventoryoptionChest_bmp, *equipped_bmp;
extern SDL_Surface* itembroken_bmp;
//extern SDL_Surface *category_bmp[NUMCATEGORIES];
extern SDL_Surface* shopkeeper_bmp;
extern SDL_Surface* damage_bmp;
extern int textscroll;
extern int attributespage;
extern int proficienciesPage;
extern Item* invitems[4];
extern Item* invitemschest[4];
extern int inventorycategory;
extern int itemscroll;
extern view_t camera_charsheet;
extern real_t camera_charsheet_offsetyaw;

extern int selected_inventory_slot_x;
extern int selected_inventory_slot_y;

void select_inventory_slot(int x, int y);

extern SDL_Surface* inventoryChest_bmp;
extern SDL_Surface* invclose_bmp;
extern SDL_Surface* invgraball_bmp;
extern int chestitemscroll; //Same as itemscroll, but for the chest inventory GUI.
extern Entity* openedChest[4]; //One for each client. //TODO: Clientside, [0] will always point to something other than NULL when a chest is open and it will be NULL when a chest is closed.
extern list_t chestInv; //This is just for the client, so that it can populate the chest inventory on its end.

extern bool gui_clickdrag; //True as long as an interface element is being dragged.
extern int dragoffset_x;
extern int dragoffset_y;
extern int buttonclick;

extern bool draw_cursor; //True if the gamepad's d-pad has been used to navigate menus and such. //TODO: Off by default on consoles and the like.

// function prototypes
void takeScreenshot();
bool loadInterfaceResources();
void freeInterfaceResources();
void clickDescription(int player, Entity* entity);
void consoleCommand(char* command);
void drawMinimap();
void handleDamageIndicators();
void handleDamageIndicatorTicks();
void drawStatus();
void saveCommand(char* content);
int loadConfig(char* filename);
int saveConfig(char* filename);
int saveDefaultConfig();
void defaultConfig();
void updateChestInventory();
void updateAppraisalItemBox();
void updatePlayerInventory();
void updateShopWindow();
void updateEnemyBar(Entity* source, Entity* target, char* name, Sint32 hp, Sint32 maxhp);
damageIndicator_t* newDamageIndicator(double x, double y);

void selectItemMenuSlot(const Item& item, int entry);
bool autoAddHotbarFilter(const Item& item);
void quickStackItems();
void sortInventoryItemsOfType(int categoryInt, bool sortRightToLeft); // sort inventory items matching category. -1 is everything, -2 is only equipped items.
void autosortInventory();
bool mouseInsidePlayerInventory();
bool mouseInsidePlayerHotbar();
extern Uint32 itemMenuItem;
extern bool itemMenuOpen;
extern int itemMenuSelected;

/*
 * Used for two purposes:
 * * In inventory navigation, if you pick up an item, drops it only if detects a second click, not if the button is released.
 * * In the item context menu, toggles if it should pop open or not.
 * This makes this variable super useful for gamepad support.
 */
extern bool toggleclick;

//Inventory GUI definitions.
static const int INVENTORY_MODE_ITEM = 0;
static const int INVENTORY_MODE_SPELL = 1;
extern SDL_Surface* inventory_mode_item_img;
extern SDL_Surface* inventory_mode_item_highlighted_img;
extern SDL_Surface* inventory_mode_spell_img;
extern SDL_Surface* inventory_mode_spell_highlighted_img;
extern int inventory_mode;

/*
 * Determines how item select (pick up, release) mechanic works.
 *		BEHAVIOR_MOUSE = press left button to pick up, release left button to drop,
 *		BEHAVIOR_GAMEPAD = press mapped button (x by default) to select/"grab" item, press again to drop.
 */
enum selectBehavior_t
{
	BEHAVIOR_MOUSE = 0,
	BEHAVIOR_GAMEPAD = 1,
	ENUM_LEN = 2
} extern itemSelectBehavior;

//Chest GUI definitions.
#define CHEST_INVENTORY_X (((xres / 2) - (inventoryChest_bmp->w / 2)) + chestgui_offset_x)
#define CHEST_INVENTORY_Y (((yres / 2) - (inventoryChest_bmp->h / 2)) + chestgui_offset_y)
extern int chestgui_offset_x;
extern int chestgui_offset_y;
extern bool dragging_chestGUI; //The chest GUI is being dragged.
/*
 * Currently selected chest inventory slot.
 * Same deal as with hotbar & inventory selection (exists for gamepad support).
 * -1 = no selection (e.g. mouse out or no items in the first place).
 */
extern int selectedChestSlot;
void selectChestSlot(int slot);
int numItemsInChest();
void warpMouseToSelectedChestSlot();

void warpMouseToSelectedInventorySlot();

//Magic GUI definitions.
extern SDL_Surface* magicspellList_bmp;
extern SDL_Surface* spell_list_titlebar_bmp;
extern SDL_Surface* spell_list_gui_slot_bmp;
extern SDL_Surface* spell_list_gui_slot_highlighted_bmp;
extern int spellscroll; //Same as itemscroll, but for the spell list GUI.
extern int magicspell_list_offset_x;
extern int magicspell_list_offset_y;
#define MAGICSPELL_LIST_X (((xres / 2) - (magicspellList_bmp->w / 2)) + magicspell_list_offset_x)
#define MAGICSPELL_LIST_Y (((yres / 2) - (magicspellList_bmp->h / 2)) + magicspell_list_offset_y)
extern bool dragging_magicspell_list_GUI; //The magic spell list GUI is being dragged.
/*
 * The state of the magic GUI.
 * 0 = spell list.
 * 1 = spell editor.
 */
extern int magic_GUI_state;
extern SDL_Rect magic_gui_pos; //The position of the magic GUI is stored here.
extern SDL_Surface* sustained_spell_generic_icon; //The goto icon when no other is available.

void renderMagicGUI(int winx, int winy, int winw, int winh);
void updateMagicGUI();
#define SUST_DIR_HORZ 0
#define SUST_DIR_VERT 1
#define SUST_SPELLS_DIRECTION SUST_DIR_VERT //0 = horizontal, 1 = vertical.
//sust_spells_x & sust_spells_y define the top left corner of where the sustained spells icons start drawing.
#define SUST_SPELLS_X 32
#define SUST_SPELLS_Y 32
#define SUST_SPELLS_RIGHT_ALIGN true //If true, overrides settings and makes the sustained spells draw alongside the right edge of the screen, vertically.
void drawSustainedSpells(); //Draws an icon for every sustained spell.

//Identify GUI definitions.
//NOTE: Make sure to always reset identifygui_appraising back to false.
#define IDENTIFY_GUI_X (((xres / 2) - (inventoryChest_bmp->w / 2)) + identifygui_offset_x)
#define IDENTIFY_GUI_Y (((yres / 2) - (inventoryChest_bmp->h / 2)) + identifygui_offset_y)
extern bool identifygui_active;
extern bool identifygui_appraising; //If this is true, the appraisal skill is controlling the identify GUI. If this is false, it originated from an identify spell.
extern int identifygui_offset_x;
extern int identifygui_offset_y;
extern bool dragging_identifyGUI; //The identify GUI is being dragged.
extern int identifyscroll;
static const int NUM_IDENTIFY_GUI_ITEMS = 4;
extern Item* identify_items[NUM_IDENTIFY_GUI_ITEMS];
extern SDL_Surface* identifyGUI_img;

extern int selectedIdentifySlot;

void selectIdentifySlot(int slot);
void warpMouseToSelectedIdentifySlot();

void CloseIdentifyGUI();
void updateIdentifyGUI(); //Updates the identify item GUI.
void identifyGUIIdentify(Item* item); //Identify the given item.
int getAppraisalTime(Item* item); // Return time in ticks needed to appraise an item
void drawSustainedSpells(); //Draws an icon for every sustained spell.

//Remove curse GUI definitions.
#define REMOVECURSE_GUI_X (((xres / 2) - (inventoryChest_bmp->w / 2)) + removecursegui_offset_x)
#define REMOVECURSE_GUI_Y (((yres / 2) - (inventoryChest_bmp->h / 2)) + removecursegui_offset_y)
extern bool removecursegui_active;
extern int removecursegui_offset_x;
extern int removecursegui_offset_y;
extern bool dragging_removecurseGUI; //The remove curse GUI is being dragged.
extern int removecursescroll;
static const int NUM_REMOVE_CURSE_GUI_ITEMS = 4;
extern Item* removecurse_items[NUM_REMOVE_CURSE_GUI_ITEMS];
//extern SDL_Surface *removecurseGUI_img; //Nah, just use the identify GUI's image. It works well enough. No need to double the resources.

void closeRemoveCurseGUI();
void updateRemoveCurseGUI(); //Updates the remove curse GUI.
void removecurseGUIRemoveCurse(Item* item); //Uncurse the given item.

//Gamepad-support related stuff.
extern int selectedRemoveCurseSlot;
void selectRemoveCurseSlot(int slot);
void warpMouseToSelectedRemoveCurseSlot();


/*
 * Returns true if the mouse is in the specified bounds, with x1 and y1 specifying the top left corner, and x2 and y2 specifying the bottom right corner.
 */
bool mouseInBounds(int x1, int x2, int y1, int y2);

void updateCharacterSheet();
void drawPartySheet();
void drawSkillsSheet();

//Right sidebar defines.
#define RIGHTSIDEBAR_X (xres - rightsidebar_titlebar_img->w)
#define RIGHTSIDEBAR_Y 0
//Note: Just using the spell versions of these for now.
extern SDL_Surface* rightsidebar_titlebar_img;
extern SDL_Surface* rightsidebar_slot_img;
extern SDL_Surface* rightsidebar_slot_highlighted_img;
extern SDL_Surface* rightsidebar_slot_grayedout_img;
extern int rightsidebar_height;
extern int appraisal_timer; //There is a delay after the appraisal skill is activated before the item is identified.
extern int appraisal_timermax;
extern Uint32 appraisal_item; //The item being appraised (or rather its uid)

void updateRightSidebar(); //Updates the sidebar on the right side of the screen, the one containing spells, skills, etc.

//------book_t Defines-----
extern SDL_Surface* bookgui_img;
//extern SDL_Surface *nextpage_img;
//extern SDL_Surface *previouspage_img;
//extern SDL_Surface *bookclose_img;
extern SDL_Surface* book_highlighted_left_img; //Draw this when the mouse is over the left half of the book.
extern SDL_Surface* book_highlighted_right_img; //Draw this when the mouse is over the right half of the book.
extern node_t* book_page;
extern int bookgui_offset_x;
extern int bookgui_offset_y;
#define BOOK_GUI_X (((xres / 2) - (bookgui_img->w / 2)) + bookgui_offset_x)
#define BOOK_GUI_Y (((yres / 2) - (bookgui_img->h / 2)) + bookgui_offset_y)
extern bool dragging_book_GUI; //The book GUI is being dragged.
extern bool book_open; //Is there a book open?
struct book_t;
extern struct book_t* open_book;
extern Item* open_book_item; //A pointer to the open book's item, so that the game knows to close the book when the player drops that item.
#define BOOK_FONT ttf12
#define BOOK_FONT_WIDTH TTF12_WIDTH
#define BOOK_FONT_HEIGHT TTF12_HEIGHT
//TODO: Calculate these two automatically based off of the buttons?
#define BOOK_PAGE_WIDTH 248
#define BOOK_PAGE_HEIGHT 256
#define BOOK_TITLE_PADDING 2 //The amount of empty space above and below the book titlename.
//#define BOOK_TITLE_HEIGHT (BOOK_TITLE_FONT_SIZE + BOOK_TITLE_PADDING) //The total y space the book's title takes up. Used for calculating BOOK_DRAWSPACE_Y.
int bookTitleHeight(struct book_t* book); //Returns how much space the book's title will occupy.
//#define BOOK_DRAWSPACE_X 280
//#define BOOK_DRAWSPACE_X (bookgui_img->w - (BOOK_BORDER_THICKNESS * 2))
#define START_OF_BOOKDRAWSPACE_X (BOOK_BORDER_THICKNESS) //This is the amount to add to BOOK_GUI_X to get the render area for the text.
//#define BOOK_DRAWSPACE_Y 180
//#define BOOK_DRAWSPACE_Y (bookgui_img->h - (BOOK_BORDER_THICKNESS * 2) - std::max(previouspage_img->h, nextpage_img->h)) //NOTE: You need to manually add  "- bookTitleHeight(open_book)" wherever you use this define.
#define START_OF_BOOK_DRAWSPACE_Y (BOOK_BORDER_THICKNESS) //This is the amount to add to BOOK_GUI_Y to get the render area for the text. //NOTE: You need to manually add  "+ bookTitleHeight(open_book)" wherever you use this define.
#define FLIPMARGIN 240
#define DRAGHEIGHT_BOOK 32
//extern int book_characterspace_x; //How many characters can fit along the x axis.
//extern int book_characterspace_y; //How many characters can fit along the y axis.
void updateBookGUI();
void closeBookGUI();
void openBook(struct book_t* book, Item* item);

extern Entity* hudweapon; //A pointer to the hudweapon entity.


//------Hotbar Defines-----
/*
 * The hotbar itself is an array.
 * NOTE: If the status bar width is changed, you need to change the slot image too. Make sure the status bar width stays divisible by 10.
 */

//NOTE: Each hotbar slot is "constructed" in loadInterfaceResources() in interface.c. If you add anything, make sure to initialize it there.
typedef struct hotbar_slot_t
{
	/*
	 * This is an item's ID. It just resolves to NULL if an item is no longer valid.
	 */
	Uint32 item;
} hotbar_slot_t;

#define HOTBAR_EMPTY 0
#define HOTBAR_ITEM 1
#define HOTBAR_SPELL 2

static const unsigned NUM_HOTBAR_SLOTS = 10; //NOTE: If you change this, you must dive into drawstatus.c and update the hotbar code. It expects 10.
extern hotbar_slot_t hotbar[NUM_HOTBAR_SLOTS];
extern int current_hotbar; //For use with gamepads and stuff because no hotkeys like a keyboard.

extern SDL_Surface* hotbar_img; //A 64x64 slot.
extern SDL_Surface* hotbar_spell_img; //Drawn when a spell is in the hotbar. TODO: Replace with unique images for every spell. (Or draw this by default if none found?)

//Returns a pointer to a hotbar slot if the specified coordinates are in the area of the hotbar. Used for such things as dragging and dropping items.
hotbar_slot_t* getHotbar(int x, int y);

void selectHotbarSlot(int slot);
extern bool hotbarHasFocus;
void warpMouseToSelectedHotbarSlot();

/*
 * True = automatically place items you pick up in your hotbar.
 * False = don't.
 */
extern bool auto_hotbar_new_items;

extern bool auto_hotbar_categories[NUM_HOTBAR_CATEGORIES]; // true = enable auto add to hotbar. else don't add.

extern int autosort_inventory_categories[NUM_AUTOSORT_CATEGORIES]; // 0 = disable priority sort, fill rightmost first. greater than 0, fill leftmost using value as priority (0 = lowest priority)

extern bool hotbar_numkey_quick_add; // use number keys to add items to hotbar if mouse in inventory panel.

extern bool disable_messages;

extern bool right_click_protect;

extern bool auto_appraise_new_items;

extern bool lock_right_sidebar;

extern bool show_game_timer_always;

const char* getInputName(Uint32 scancode);
Sint8* inputPressed(Uint32 scancode);

//All the code that sets shootmode = false. Display chests, inventory, books, shopkeeper, identify, whatever.
void openStatusScreen(int whichGUIMode, int whichInventoryMode); //TODO: Make all the everything use this. //TODO: Make an accompanying closeStatusScreen() function.

static const int SCANCODE_UNASSIGNED_BINDING = 399;

inline bool hotbarGamepadControlEnabled()
{
	return ( !openedChest[clientnum] && gui_mode != GUI_MODE_SHOP && !identifygui_active && !removecursegui_active );
}

extern SDL_Surface *str_bmp64u;
extern SDL_Surface *dex_bmp64u;
extern SDL_Surface *con_bmp64u;
extern SDL_Surface *int_bmp64u;
extern SDL_Surface *per_bmp64u;
extern SDL_Surface *chr_bmp64u;
extern SDL_Surface *str_bmp64;
extern SDL_Surface *dex_bmp64;
extern SDL_Surface *con_bmp64;
extern SDL_Surface *int_bmp64;
extern SDL_Surface *per_bmp64;
extern SDL_Surface *chr_bmp64;

extern SDL_Surface *sidebar_lock_bmp;
extern SDL_Surface *sidebar_unlock_bmp;

void printStatBonus(TTF_Font* outputFont, Sint32 stat, Sint32 statWithModifiers, int x, int y);
void attackHoverText(Sint32 input[6]);
Sint32 displayAttackPower(Sint32 output[6]);

class MinimapPing
{
public:
	Sint32 tickStart;
	Uint8 player;
	Uint8 x;
	Uint8 y;
	MinimapPing(Sint32 tickStart, Uint8 player, Uint8 x, Uint8 y) :
		tickStart(tickStart),
		player(player),
		x(x),
		y(y) {}
};

extern std::vector<MinimapPing> minimapPings;
void minimapPingAdd(MinimapPing newPing);
extern int minimapPingGimpTimer;

extern std::vector<std::pair<SDL_Surface**, std::string>> systemResourceImages;