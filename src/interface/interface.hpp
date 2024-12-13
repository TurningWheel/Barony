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
#include "../draw.hpp"
#include "../ui/Frame.hpp"
#include "../items.hpp"

class Item;

struct DamageIndicatorHandler_t
{
	struct DamageIndicator_t
	{
		int player = -1;
		real_t x = 0.0; // x and y of the attacker in world coordinates
		real_t y = 0.0;
		real_t alpha = 0.0;
		Uint32 ticks = 0;
		Uint32 animateTicks = 0;
		Uint32 uid = 0;

		int size = 0;
		int w = 0;
		int h = 0;
		Uint32 flashTicks = 0;
		Uint32 flashProcessedOnTick = 0;
		int flashAnimState = -1;
		bool hitDealtDamage = false;
		bool expired = false;
		DamageIndicator_t(const int _player)
		{
			player = _player;
		}
		void process();
	};
	void update();
	void insert(const int player, const real_t _x, const real_t _y, const bool damaged);
	std::vector<DamageIndicator_t> indicators[MAXPLAYERS];
};
extern DamageIndicatorHandler_t DamageIndicatorHandler;

extern bool hide_statusbar;
extern real_t uiscale_chatlog;
extern real_t uiscale_playerbars;
extern bool uiscale_charactersheet;
extern bool uiscale_skillspage;
extern real_t uiscale_hotbar;
extern real_t uiscale_inventory;

#include "../entity.hpp"

enum DamageGib {
	DMG_DEFAULT,
	DMG_WEAKER,
	DMG_WEAKEST,
	DMG_STRONGER,
	DMG_STRONGEST,
	DMG_FIRE,
	DMG_BLEED,
	DMG_POISON,
	DMG_HEAL,
	DMG_MISS,
	DMG_TODO
};
enum DamageGibDisplayType {
	DMG_GIB_NUMBER,
	DMG_GIB_MISS,
	DMG_GIB_SPRITE
};
class EnemyHPDamageBarHandler
{
public:
	static bool bDamageGibTypesEnabled;
	static int maxTickLifetime;
	static int maxTickFurnitureLifetime;
	static int shortDistanceHPBarFadeTicks;
	static real_t shortDistanceHPBarFadeDistance;
	static bool bEnemyBarSimpleBlit;
	static std::map<int, std::vector<int>> damageGibAnimCurves;
	static void dumpCache();
	static std::vector<std::pair<real_t, int>>widthHealthBreakpointsMonsters;
	static std::vector<std::pair<real_t, int>>widthHealthBreakpointsFurniture;
	enum HPBarType {
		BAR_TYPE_CREATURE,
		BAR_TYPE_FURNITURE
	};

	struct BarAnimator_t
	{
		real_t foregroundValue = 0.0;
		real_t backgroundValue = 0.0;
		real_t previousSetpoint = 0.0;
		Sint32 setpoint = 0;
		Uint32 animateTicks = 0;
		Sint32 damageTaken = -1;
		real_t widthMultiplier = 1.0;
		real_t maxValue = 0.0;
		real_t currentOpacity = 0.0;
		real_t fadeOut = 100.0;
		real_t fadeIn = 100.0;
		real_t skullOpacities[4];
		real_t damageFrameOpacity = 100.0;
		BarAnimator_t() { 
			for ( int i = 0; i < 4; ++i )
			{
				skullOpacities[i] = 100.0;
			}
		}
	};
	struct EnemyHPDetails
	{
		HPBarType barType = BAR_TYPE_CREATURE;
		BarAnimator_t animator;
		std::string enemy_name = "";
		Sint32 enemy_hp = 0;
		Sint32 enemy_maxhp = 0;
		Sint32 enemy_oldhp = 0;
		Uint32 enemy_timer = 0;
		Uint32 enemy_uid = 0;
		Uint32 enemy_statusEffects1 = 0;
		Uint32 enemy_statusEffects2 = 0;
		Uint32 enemy_statusEffectsLowDuration1 = 0;
		Uint32 enemy_statusEffectsLowDuration2 = 0;
		bool lowPriorityTick = false;
		bool shouldDisplay = true;
		bool hasDistanceCheck = false;
		bool displayOnHUD = false;
		bool expired = false;
		real_t depletionAnimationPercent = 100.0;
		EnemyHPDetails() {};
		EnemyHPDetails(Uint32 uid, Sint32 HP, Sint32 maxHP, Sint32 oldHP, const char* name, bool isLowPriority)
		{
			if ( Entity* entity = uidToEntity(uid) )
			{
				if ( entity->behavior != &actMonster && entity->behavior != actPlayer )
				{
					barType = BAR_TYPE_FURNITURE;
				}
				else if ( entity->isInertMimic() )
				{
					barType = BAR_TYPE_FURNITURE;
				}
			}
			enemy_uid = uid;
			enemy_hp = HP;
			enemy_maxhp = maxHP;
			enemy_oldhp = oldHP;
			depletionAnimationPercent =
				enemy_oldhp / static_cast<real_t>(enemy_maxhp);
			enemy_timer = ticks;
			lowPriorityTick = isLowPriority;
			shouldDisplay = true;
			enemy_name = name;
		}
		~EnemyHPDetails();

		real_t worldX = 0.0;
		real_t worldY = 0.0;
		real_t worldZ = 0.0;
		real_t screenDistance = 0.0;
		TempTexture* worldTexture = nullptr;
		SDL_Surface* worldSurfaceSprite = nullptr;
		SDL_Surface* worldSurfaceSpriteStatusEffects = nullptr;
		SDL_Surface* blitEnemyBarStatusEffects(const int player);
		SDL_Surface* blitEnemyBar(const int player, SDL_Surface* statusEffectSprite);
		void updateWorldCoordinates();
	};

	std::unordered_map<Uint32, EnemyHPDetails> HPBars;
	EnemyHPDetails* addEnemyToList(Sint32 HP, Sint32 maxHP, Sint32 oldHP, Uint32 uid, const char* name, bool isLowPriority, DamageGib gibDmgType);
	void displayCurrentHPBar(const int player);
	void cullExpiredHPBars();
	EnemyHPDetails* getMostRecentHPBar(int index = 0);
	Uint32 lastEnemyUid = 0;
};
extern EnemyHPDamageBarHandler enemyHPDamageBarHandler[MAXPLAYERS];

#ifndef SHOPWINDOW_SIZE
#define SHOPWINDOW_SIZE
#define SHOPWINDOW_SIZEX 576
#define SHOPWINDOW_SIZEY 324
#endif

static const int GUI_MODE_NONE = -1; //GUI closed, ingame & playing.
static const int GUI_MODE_INVENTORY = 0;
static const int GUI_MODE_MAGIC = 1;
static const int GUI_MODE_SHOP = 2;
static const int GUI_MODE_FOLLOWERMENU = 3;
static const int GUI_MODE_SIGN = 4;
static const int GUI_MODE_CALLOUT = 5;

extern int textscroll;
extern int inventorycategory;
extern int itemscroll;
extern view_t camera_charsheet;
extern real_t camera_charsheet_offsetyaw;

void select_inventory_slot(int player, int currentx, int currenty, int diffx, int diffy);
void select_spell_slot(int player, int currentx, int currenty, int diffx, int diffy);
void select_chest_slot(int player, int currentx, int currenty, int diffx, int diffy);
void select_shop_slot(int player, int currentx, int currenty, int diffx, int diffy);
void select_tinkering_slot(int player, int currentx, int currenty, int diffx, int diffy);
void select_alchemy_slot(int player, int currentx, int currenty, int diffx, int diffy);
void select_feather_slot(int player, int currentx, int currenty, int diffx, int diffy);
void select_assistshrine_slot(int player, int currentx, int currenty, int diffx, int diffy);

extern Entity* openedChest[MAXPLAYERS]; //One for each client. //TODO: Clientside, [0] will always point to something other than NULL when a chest is open and it will be NULL when a chest is closed.
extern list_t chestInv[MAXPLAYERS]; //This is just for the client, so that it can populate the chest inventory on its end.

//extern bool gui_clickdrag[MAXPLAYERS]; //True as long as an interface element is being dragged.
//extern int dragoffset_x[MAXPLAYERS];
//extern int dragoffset_y[MAXPLAYERS];
//extern int buttonclick;

// function prototypes
void takeScreenshot(const char* output_path = nullptr);
bool loadInterfaceResources();
void freeInterfaceResources();
void clickDescription(const int player, Entity* entity);
void consoleCommand(char const * const command);
void drawMinimap(const int player, SDL_Rect rect, bool drawingSharedMap);
struct MinimapHighlight_t
{
	Uint32 ticks = 0;
};
extern std::map<int, MinimapHighlight_t> minimapHighlights;
void handleDamageIndicatorTicks();
void drawStatus(const int player);
void drawStatusNew(const int player);
void saveCommand(char* content);
int loadConfig(char* filename);
int saveConfig(char const * const filename);
void defaultConfig();
void updateChestInventory(const int player);
Item* takeItemFromChest(int player, Item* item, int amount, Item* addToSpecificInventoryItem, bool forceNewStack, bool bDoPickupMessage = true);
void updateShopWindow(const int player);
bool getShopFreeSlot(const int player, list_t* shopInventory, Item* itemToSell, int& xout, int& yout, Item*& itemToStackInto);

void updateEnemyBar(Entity* source, Entity* target, const char* name, Sint32 hp, Sint32 maxhp, 
	bool lowPriorityTick, DamageGib gibType);

bool autoAddHotbarFilter(const Item& item);
void quickStackItems(const int player);
void sortInventoryItemsOfType(const int player, int categoryInt, bool sortRightToLeft); // sort inventory items matching category. -1 is everything, -2 is only equipped items.
void autosortInventory(const int player, bool sortPaperDoll = false);
bool mouseInsidePlayerInventory(const int player);
bool mouseInsidePlayerHotbar(const int player);
bool playerLearnedSpellbook(const int player, Item* current_item);

/*
 * Used for two purposes:
 * * In inventory navigation, if you pick up an item, drops it only if detects a second click, not if the button is released.
 * * In the item context menu, toggles if it should pop open or not.
 * This makes this variable super useful for gamepad support.
 */

//Inventory GUI definitions.
static const int INVENTORY_MODE_ITEM = 0;
static const int INVENTORY_MODE_SPELL = 1;
extern bool restrictPaperDollMovement;

//Chest GUI definitions.
int numItemsInChest(const int player);

//Magic GUI definitions.
//extern SDL_Surface* magicspellList_bmp;
//extern SDL_Surface* spell_list_titlebar_bmp;
//extern SDL_Surface* spell_list_gui_slot_bmp;
//extern SDL_Surface* spell_list_gui_slot_highlighted_bmp;
//extern int spellscroll; //Same as itemscroll, but for the spell list GUI.
//extern int magicspell_list_offset_x;
//extern int magicspell_list_offset_y;
//#define MAGICSPELL_LIST_X (((xres / 2) - (magicspellList_bmp->w / 2)) + magicspell_list_offset_x)
//#define MAGICSPELL_LIST_Y (((yres / 2) - (magicspellList_bmp->h / 2)) + magicspell_list_offset_y)
//extern bool dragging_magicspell_list_GUI; //The magic spell list GUI is being dragged.
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

void drawSustainedSpells(const int player); //Draws an icon for every sustained spell.

enum GUICurrentType
{
	GUI_TYPE_NONE,
	GUI_TYPE_ALCHEMY,
	GUI_TYPE_TINKERING,
	GUI_TYPE_SCRIBING,
	GUI_TYPE_ITEMFX,
	GUI_TYPE_ASSIST
};

// Generic GUI Stuff (repair/alchemy)
class GenericGUIMenu
{
	int gui_player = 0;
	GUICurrentType guiType;
public:
	static const int kNumShownItems = 4;
	bool guiActive;

	// Alchemy
	Item* basePotion;
	Item* secondaryPotion;
	Item* alembicItem;
	bool experimentingAlchemy;
	
	// Misc item/spell effects
	Item* itemEffectScrollItem;
	bool itemEffectUsingSpell;
	bool itemEffectUsingSpellbook;
	int itemEffectItemType;
	int itemEffectItemBeatitude;

	// Tinkering
	enum TinkeringFilter
	{
		TINKER_FILTER_ALL,
		TINKER_FILTER_CRAFTABLE,
		TINKER_FILTER_SALVAGEABLE,
		TINKER_FILTER_REPAIRABLE
	};
	Item* tinkeringKitItem;
	list_t tinkeringTotalItems;
	node_t* tinkeringTotalLastCraftableNode;
	TinkeringFilter tinkeringFilter;
	std::unordered_set<Uint32> tinkeringMetalScrap;
	std::unordered_set<Uint32> tinkeringMagicScrap;
	Item* tinkeringAutoSalvageKitItem;
	Item* tinkeringAutoSalvageThisItem;
	Uint32 tinkeringSfxLastTicks = 0;
	bool tinkeringBulkSalvage = false;
	Sint32 tinkeringBulkSalvageMetalScrap = 0;
	Sint32 tinkeringBulkSalvageMagicScrap = 0;

	// Scribing
	Item* scribingToolItem;
	list_t scribingTotalItems;
	node_t* scribingTotalLastCraftableNode;
	Item* scribingBlankScrollTarget;
	enum ScribingFilter
	{
		SCRIBING_FILTER_CRAFTABLE,
		SCRIBING_FILTER_REPAIRABLE
	};
	ScribingFilter scribingFilter;

	GenericGUIMenu() :
		guiActive(false),
		basePotion(nullptr),
		secondaryPotion(nullptr),
		alembicItem(nullptr),
		experimentingAlchemy(false),
		itemEffectScrollItem(nullptr),
		itemEffectUsingSpell(false),
		itemEffectUsingSpellbook(false),
		itemEffectItemType(0),
		itemEffectItemBeatitude(0),
		tinkeringKitItem(nullptr),
		tinkeringTotalLastCraftableNode(nullptr),
		tinkeringFilter(TINKER_FILTER_CRAFTABLE),
		tinkeringAutoSalvageKitItem(nullptr),
		tinkeringAutoSalvageThisItem(nullptr),
		scribingFilter(SCRIBING_FILTER_CRAFTABLE),
		scribingToolItem(nullptr),
		scribingTotalLastCraftableNode(nullptr),
		scribingBlankScrollTarget(nullptr),
		scribingLastUsageAmount(0),
		scribingLastUsageDisplayTimer(0),
		tinkerGUI(*this),
		alchemyGUI(*this),
		featherGUI(*this),
		itemfxGUI(*this),
		assistShrineGUI(*this)
	{
		tinkeringTotalItems.first = nullptr;
		tinkeringTotalItems.last = nullptr;
		scribingTotalItems.first = nullptr;
		scribingTotalItems.last = nullptr;
	};

	void setPlayer(const int p) { gui_player = p; }
	const int getPlayer() { return gui_player;  }
	void closeGUI();
	void openGUI(int type, Item* effectItem, int effectBeatitude, int effectItemType, int usingSpellID);
	void openGUI(int type, bool experimenting, Item* itemOpenedWith);
	void openGUI(int type, Item* itemOpenedWith);
	void openGUI(int type, Entity* interactable);
	void updateGUI();
	void rebuildGUIInventory();
	bool shouldDisplayItemInGUI(Item* item);
	bool executeOnItemClick(Item* item);

	// repair menu funcs
	void repairItem(Item* item);
	bool isItemRepairable(const Item* item, int repairScroll);

	//remove curse
	bool isItemRemoveCursable(const Item* item);
	void uncurseItem(Item* item);

	//identify
	bool isItemIdentifiable(const Item* item);
	void identifyItem(Item* item);

	//enchant
	bool isItemEnchantWeaponable(const Item* item);
	bool isItemEnchantArmorable(const Item* item);
	void enchantItem(Item* item);

	//alchemy menu funcs
	bool isItemMixable(const Item* item);
	void alchemyCombinePotions();
	bool alchemyLearnRecipe(int type, bool increaseskill, bool notify = true);
	bool isItemBaseIngredient(int type);
	bool isItemSecondaryIngredient(int type);
	void alchemyLearnRecipeOnLevelUp(int skill);

	// tinkering menu foncs
	bool tinkeringSalvageItem(Item* item, bool outsideInventory, int player);
	bool tinkeringCraftItem(Item* item);
	void tinkeringCreateCraftableItemList();
	void tinkeringFreeLists();
	bool isItemSalvageable(const Item* item, int player);
	static bool tinkeringGetItemValue(const Item* item, int* metal, int* magic);
	static bool tinkeringGetCraftingCost(const Item* item, int* metal, int* magic);
	bool tinkeringPlayerCanAffordCraft(const Item* item);
	Item* tinkeringCraftItemAndConsumeMaterials(const Item* item);
	int tinkeringPlayerHasSkillLVLToCraft(const Item* item);
	bool tinkeringKitDegradeOnUse(int player);
	Item* tinkeringKitFindInInventory();
	bool tinkeringKitRollIfShouldBreak();
	bool tinkeringGetRepairCost(Item* item, int* metal, int* magic);
	bool tinkeringIsItemRepairable(Item* item, int player);
	bool tinkeringIsItemUpgradeable(const Item* item);
	bool tinkeringRepairItem(Item* item);
	int tinkeringUpgradeMaxStatus(Item* item);
	bool tinkeringConsumeMaterialsForRepair(Item* item, bool upgradingItem);
	bool tinkeringPlayerCanAffordRepair(Item* item);
	int tinkeringRepairGeneralItemSkillRequirement(Item* item);
	bool tinkeringPlayerHasMaterialsInventory(int metal, int magic);
	Uint32 tinkeringRetrieveLeastScrapStack(int type);
	int tinkeringCountScrapTotal(int type);

	void scribingCreateCraftableItemList();
	void scribingFreeLists();
	int scribingToolDegradeOnUse(Item* itemUsedWith);
	Item* scribingToolFindInInventory();
	bool scribingWriteItem(Item* item);
	int scribingLastUsageAmount;
	int scribingLastUsageDisplayTimer;
	void scribingGetChargeCost(Item* itemUsedWith, int& outChargeCostMin, int& outChargeCostMax);

	inline bool isGUIOpen()
	{
		return guiActive;
	};
	inline bool isNodeTinkeringCraftableItem(node_t* node)
	{
		if ( !node )
		{
			return false;
		}
		return (node->list == &tinkeringTotalItems);
	};
	inline bool isNodeScribingCraftableItem(node_t* node)
	{
		if ( !node )
		{
			return false;
		}
		return (node->list == &scribingTotalItems);
	};
	inline bool isItemUsedForCurrentGUI(const Item& item)
	{
		if ( &item == scribingToolItem || &item == tinkeringKitItem || &item == alembicItem
			|| &item == scribingBlankScrollTarget
			|| &item == basePotion || &item == secondaryPotion || &item == itemEffectScrollItem )
		{
			return true;
		}
		return false;
	}
	inline void clearCurrentGUIFromItem(const Item& item)
	{
		if ( &item == scribingToolItem )
		{
			scribingToolItem = nullptr;
		}
		if ( &item == scribingBlankScrollTarget )
		{
			scribingBlankScrollTarget = nullptr;
		}
		if ( &item == tinkeringKitItem )
		{
			tinkeringKitItem = nullptr;
		}
		if ( &item == alembicItem )
		{
			alembicItem = nullptr;
		}
		if ( &item == basePotion )
		{
			basePotion = nullptr;
		}
		if ( &item == secondaryPotion )
		{
			secondaryPotion = nullptr;
		}
		if ( &item == itemEffectScrollItem )
		{
			itemEffectScrollItem = nullptr;
		}
	}
	bool isNodeFromPlayerInventory(node_t* node);

	struct TinkerGUI_t
	{
		GenericGUIMenu& parentGUI;
		TinkerGUI_t(GenericGUIMenu& g) :
			parentGUI(g) 
		{}

		Frame* tinkerFrame = nullptr;
		real_t animx = 0.0;
		bool isInteractable = true;
		bool bOpen = false;
		bool bFirstTimeSnapCursor = false;
		void openTinkerMenu();
		void closeTinkerMenu();
		void updateTinkerMenu();
		void createTinkerMenu();
		bool tinkerGUIHasBeenCreated() const;
		bool isConstructMenuActive() const;
		bool isSalvageOrRepairMenuActive() const;
		Sint32 metalScrapPrice = 0;
		Sint32 magicScrapPrice = 0;
		std::string itemDesc = "";
		int itemType = -1;
		int itemRequirement = -1;
		enum TinkerActions_t : int 
		{
			TINKER_ACTION_NONE,
			TINKER_ACTION_OK,
			TINKER_ACTION_OK_UPGRADE,
			TINKER_ACTION_INVALID_ITEM,
			TINKER_ACTION_INVALID_ROBOT_TO_SALVAGE,
			TINKER_ACTION_NO_MATERIALS,
			TINKER_ACTION_NO_MATERIALS_UPGRADE,
			TINKER_ACTION_NO_SKILL_LVL,
			TINKER_ACTION_NO_SKILL_LVL_UPGRADE,
			TINKER_ACTION_ITEM_FULLY_REPAIRED,
			TINKER_ACTION_ITEM_FULLY_UPGRADED,
			TINKER_ACTION_ROBOT_BROKEN,
			TINKER_ACTION_MUST_BE_UNEQUIPPED,
			TINKER_ACTION_ALREADY_USING_THIS_TINKERING_KIT,
			TINKER_ACTION_OK_UNIDENTIFIED_SALVAGE,
			TINKER_ACTION_NOT_IDENTIFIED_YET,
			TINKER_ACTION_KIT_NEEDS_REPAIRS
		};
		TinkerActions_t itemActionType = TINKER_ACTION_NONE;
		bool itemRequiresTitleReflow = true;
		real_t animDrawer = 0.0;
		real_t animTooltip = 0.0;
		Uint32 animTooltipTicks = 0;
		real_t animFilter = 0.0;
		real_t animPrompt = 0.0;
		Uint32 animPromptTicks = 0;
		bool animPromptMoveLeft = false;
		real_t animInvalidAction = 0.0;
		Uint32 animInvalidActionTicks = 0;
		bool drawerJustifyInverted = false;
		enum InvalidActionFeedback_t : int
		{
			INVALID_ACTION_NONE,
			INVALID_ACTION_SHAKE_PROMPT,
			INVALID_ACTION_SHAKE_METAL_SCRAP,
			INVALID_ACTION_SHAKE_MAGIC_SCRAP,
			INVALID_ACTION_SHAKE_ALL_SCRAP
		};
		InvalidActionFeedback_t invalidActionType = INVALID_ACTION_NONE;
		Sint32 playerCurrentMetalScrap = 0;
		Sint32 playerCurrentMagicScrap = 0;
		Sint32 playerChangeMetalScrap = 0;
		Sint32 playerChangeMagicScrap = 0;
		real_t animScrap = 0.0;
		Uint32 animScrapStartTicks = 0;

		int selectedTinkerSlotX = -1;
		int selectedTinkerSlotY = -1;
		static const int MAX_TINKER_X;
		static const int MAX_TINKER_Y;
		std::unordered_map<int, Frame*> tinkerSlotFrames;
		bool isTinkerConstructItemSelected(Item* item);
		bool isSalvageOrRepairItemSelected(Item* item);
		void selectTinkerSlot(const int x, const int y);
		const int getSelectedTinkerSlotX() const { return selectedTinkerSlotX; }
		const int getSelectedTinkerSlotY() const { return selectedTinkerSlotY; }
		Frame* getTinkerSlotFrame(int x, int y) const;
		TinkerActions_t setItemDisplayNameAndPrice(Item* item, bool checkResultOnly = false);
		bool warpMouseToSelectedTinkerItem(Item* snapToItem, Uint32 flags);
		void clearItemDisplayed();
		void updateTinkerScrapHeld(void* metalHeldText, void* magicHeldText, int realMetalScrap, int realMagicScrap);

		static int heightOffsetWhenNotCompact;
	};
	TinkerGUI_t tinkerGUI;

	struct ItemEffectGUI_t
	{
		GenericGUIMenu& parentGUI;
		ItemEffectGUI_t(GenericGUIMenu& g) :
			parentGUI(g)
		{}

		Frame* itemEffectFrame = nullptr;
		real_t animx = 0.0;
		bool isInteractable = true;
		bool bOpen = false;
		bool bFirstTimeSnapCursor = false;
		enum ItemEffectModes : int
		{
			ITEMFX_MODE_NONE,
			ITEMFX_MODE_SCROLL_REPAIR,
			ITEMFX_MODE_SCROLL_CHARGING,
			ITEMFX_MODE_SCROLL_IDENTIFY,
			ITEMFX_MODE_SCROLL_REMOVECURSE,
			ITEMFX_MODE_SPELL_IDENTIFY,
			ITEMFX_MODE_SPELL_REMOVECURSE,
			ITEMFX_MODE_SCROLL_ENCHANT_WEAPON,
			ITEMFX_MODE_SCROLL_ENCHANT_ARMOR
		};
		void openItemEffectMenu(ItemEffectModes mode);
		ItemEffectModes currentMode = ITEMFX_MODE_NONE;
		void closeItemEffectMenu();
		void updateItemEffectMenu();
		void createItemEffectMenu();
		bool isItemSelectedToEffect(Item* item);
		bool isItemEffectMenuActive() const;
		bool ItemEffectHasBeenCreated() const;
		std::string itemDesc = "";
		int itemType = -1;
		int itemRequirement = -1;
		enum ItemEffectActions_t : int
		{
			ITEMFX_ACTION_NONE,
			ITEMFX_ACTION_OK,
			ITEMFX_ACTION_INVALID_ITEM,
			ITEMFX_ACTION_ITEM_FULLY_REPAIRED,
			ITEMFX_ACTION_ITEM_FULLY_CHARGED,
			ITEMFX_ACTION_ITEM_IDENTIFIED,
			ITEMFX_ACTION_MUST_BE_UNEQUIPPED,
			ITEMFX_ACTION_NOT_IDENTIFIED_YET,
			ITEMFX_ACTION_NOT_CURSED
		};
		ItemEffectActions_t itemActionType = ITEMFX_ACTION_NONE;
		bool itemRequiresTitleReflow = true;
		real_t animTooltip = 0.0;
		Uint32 animTooltipTicks = 0;
		real_t animFilter = 0.0;
		real_t animPrompt = 0.0;
		Uint32 animPromptTicks = 0;
		bool animPromptMoveLeft = false;
		real_t animInvalidAction = 0.0;
		Uint32 animInvalidActionTicks = 0;
		bool panelJustifyInverted = false;
		enum InvalidActionFeedback_t : int
		{
			INVALID_ACTION_NONE,
			INVALID_ACTION_SHAKE_PROMPT
		};
		InvalidActionFeedback_t invalidActionType = INVALID_ACTION_NONE;

		ItemEffectActions_t setItemDisplayNameAndPrice(Item* item, bool checkResultOnly = false);
		void clearItemDisplayed();

		static int heightOffsetWhenNotCompact;
	};
	ItemEffectGUI_t itemfxGUI;

	struct AssistShrineGUI_t
	{
		GenericGUIMenu& parentGUI;
		AssistShrineGUI_t(GenericGUIMenu& g) :
			parentGUI(g)
		{
#ifndef EDITOR
			resetItems();
#endif
		}

		static constexpr int ASSIST_SLOT_CLOAK = -1;
		static constexpr int ASSIST_SLOT_MASK = -2;
		static constexpr int ASSIST_SLOT_AMULET = -3;
		static constexpr int ASSIST_SLOT_RING = -4;
		static constexpr int ASSIST_RACE_COLUMN = 10;
		static constexpr int ASSIST_CHAR_NAME = -10;
		static constexpr int ASSIST_CHAR_RACE = -11;
		static constexpr int ASSIST_CHAR_CLASS = -12;

		static constexpr int achievementDisabledLimit = 10;

		Item itemCloak;
		Item itemMask;
		Item itemAmulet;
		Item itemRing;

		void resetItems();
		void onGameStart();
		void resetSavedCharacterChanges();
		void onMainMenuEnd();
		bool hasItemsToClaim();
		bool raceHasChanged();
		bool classHasChanged();
		bool claimItems(bool* isEquipped);

		enum AssistShrineView_t : int
		{
			ASSIST_SHRINE_VIEW_ITEMS,
			ASSIST_SHRINE_VIEW_CLASSES,
			ASSIST_SHRINE_VIEW_RACE
		};

		AssistShrineView_t currentView = ASSIST_SHRINE_VIEW_ITEMS;
		std::map<int, int> classSlots;
		std::vector<int> raceSlots;
		int selectedClass = -1;
		int selectedRace = -1;
		int selectedSex = -1;
		int selectedAppearance = -1;
		int selectedDisableAbilities = -1;
		int savedClass = -1;
		int savedRace = -1;
		int savedSex = -1;
		int savedAppearance = -1;
		void changeCurrentView(AssistShrineView_t view);
		void updateClassSlots();
		void updateRaceSlots();
		bool receivedCharacterChangeOK = false;
		void onCharacterChange();
		static void serverUpdateStatFlagsForClients();
		real_t animx = 0.0;
		real_t animFilter = 0.0;
		real_t animPrompt = 0.0;
		real_t animTooltip = 0.0;
		real_t animAssistValueFade = 0.0;
		Uint32 animPromptTicks = 0;
		Uint32 animTooltipTicks = 0;
		real_t animClassRaceTooltipOpacity = 0.0;
		Uint32 animClassRaceTooltipTicks = 0;
		std::set<ItemType> claimedItems;
		bool isInteractable = true;
		bool bOpen = false;
		bool bFirstTimeSnapCursor = false;
		Frame* assistShrineFrame = nullptr;
		Uint32 shrineUID = 0;

		void openAssistShrine(Entity* shrine);
		void closeAssistShrine();
		void updateAssistShrine();
		void createAssistShrine();
		bool assistShrineGUIHasBeenCreated() const;
		std::unordered_map<int, Frame*> assistShrineSlotFrames;
		void selectAssistShrineSlot(const int x, const int y);
		int getAssistPointsSaved();
		int getAssistPointsPreview();
		int getAssistPointFromItem(Item* item);

		int selectedAssistShrineSlotX = -1;
		int selectedAssistShrineSlotY = -1;
		int currentScrollRow1 = 0;
		int currentScrollRow2 = 0;
		real_t scrollPercent1 = 0.0;
		real_t scrollInertia1 = 0.0;
		int scrollSetpoint1 = 0;
		real_t scrollAnimateX1 = 0.0;
		real_t scrollPercent2 = 0.0;
		real_t scrollInertia2 = 0.0;
		int scrollSetpoint2 = 0;
		real_t scrollAnimateX2 = 0.0;
		static const int kNumClassesToDisplayVertical;
		static const int kNumRacesToDisplayVertical;
		static const int kClassSlotHeight;
		static const int kRaceSlotHeight;
		static const int kRaceSlotWidth;
		static const int MAX_ASSISTSHRINE_X;
		static const int MAX_ASSISTSHRINE_Y;
		const int getSelectedAssistShrineX() const { return selectedAssistShrineSlotX; }
		const int getSelectedAssistShrineY() const { return selectedAssistShrineSlotY; }
		Frame* getAssistShrineSlotFrame(int x, int y) const;
		//void setItemDisplayNameAndPrice(Item* item, bool isTooltipForResultPotion, bool isTooltipForRecipe);
		bool warpMouseToSelectedAssistShrineItem(Item* snapToItem, Uint32 flags);
		bool itemIsFromGUI(Item* item);
		bool isSlotVisible(int x, int y) const;
		void scrollToSlot(int x, int y, bool instantly);
		static int heightOffsetWhenNotCompact;

		struct AssistNotification_t
		{
			std::string img = "";
			std::string title = "";
			std::string body = "";
			enum NotificationTypes
			{
				NOTIF_DEFAULT,
				NOTIF_SEND_REQ,
				NOTIF_CHARACTER_CHANGE_OK,
				NOTIF_CLASS_RESET
			};
			Uint32 lifetime = 3 * TICKS_PER_SECOND;
			NotificationTypes notificationType = NOTIF_DEFAULT;
			real_t animx = 0.0;
			int state = 0;
			AssistNotification_t(std::string _title, std::string _body, std::string _img, NotificationTypes _notifType)
			{
				title = _title;
				img = _img;
				body = _body;
				notificationType = _notifType;
			}
		};
		std::vector<std::pair<Uint32, AssistNotification_t>> notifications;
		AssistNotification_t* addNotification(std::string _title, std::string _body, std::string _img, AssistNotification_t::NotificationTypes _notifType);

		enum AssistItemActions_t
		{
			ASSIST_ITEM_NONE,
			ASSIST_ITEM_ACTIVATE,
			ASSIST_ITEM_DEACTIVATE,
			ASSIST_ITEM_CLAIMED,
			ASSIST_ITEM_NOTHING_TO_CLAIM,
			ASSIST_ITEM_FLAG_DISABLED,
			ASSIST_CLASS_OK,
			ASSIST_RACE_OK,
		};
		AssistItemActions_t itemActionType = ASSIST_ITEM_NONE;
		real_t animInvalidAction = 0.0;
		Uint32 animInvalidActionTicks = 0;
		enum InvalidActionFeedback_t : int
		{
			INVALID_ACTION_NONE,
			INVALID_ACTION_SHAKE_PROMPT
		};
		InvalidActionFeedback_t invalidActionType = INVALID_ACTION_NONE;
		int itemType = -1;
		AssistItemActions_t setItemDisplayNameAndPrice(Item* item, bool checkResultOnly = false);
		void clearItemDisplayed();
	};
	AssistShrineGUI_t assistShrineGUI;

	struct FeatherGUI_t
	{
		GenericGUIMenu& parentGUI;
		FeatherGUI_t(GenericGUIMenu& g) :
			parentGUI(g)
		{}

		Frame* featherFrame = nullptr;
		real_t animx = 0.0;
		bool isInteractable = true;
		bool bOpen = false;
		bool bFirstTimeSnapCursor = false;
		void openFeatherMenu();
		void closeFeatherMenu();
		void updateFeatherMenu();
		void createFeatherMenu();
		bool featherGUIHasBeenCreated() const;
		std::string itemDesc = "";
		int itemType = -1;
		enum FeatherActions_t : int
		{
			FEATHER_ACTION_NONE,
			FEATHER_ACTION_OK,
			FEATHER_ACTION_INVALID_ITEM,
			FEATHER_ACTION_NO_BLANK_SCROLL,
			FEATHER_ACTION_NO_BLANK_SCROLL_UNKNOWN_HIGHLIGHT,
			FEATHER_ACTION_FULLY_REPAIRED,
			FEATHER_ACTION_UNIDENTIFIED,
			FEATHER_ACTION_CANT_AFFORD,
			FEATHER_ACTION_MAY_SUCCEED,
			FEATHER_ACTION_OK_AND_DESTROY,
			FEATHER_ACTION_OK_UNKNOWN_SCROLL
		};
		FeatherActions_t itemActionType = FEATHER_ACTION_NONE;
		bool itemRequiresTitleReflow = true;
		real_t animDrawer = 0.0;
		real_t animTooltip = 0.0;
		Uint32 animTooltipTicks = 0;
		real_t animFilter = 0.0;
		real_t animPrompt = 0.0;
		Uint32 animPromptTicks = 0;
		bool animPromptMoveLeft = false;
		real_t animInvalidAction = 0.0;
		Uint32 animInvalidActionTicks = 0;
		bool bDrawerOpen = false;
		Uint32 inscribeSuccessTicks = 0;
		std::string inscribeSuccessName = "";
		bool drawerJustifyInverted = false;

		real_t scrollPercent = 0.0;
		real_t scrollInertia = 0.0;
		int scrollSetpoint = 0;
		real_t scrollAnimateX = 0.0;
		int currentScrollRow = 0;

		int chargeCostMin = 0;
		int chargeCostMax = 0;
		std::string currentHoveringInscriptionLabel = "";
		Sint32 currentFeatherCharge = 0;
		Sint32 changeFeatherCharge = 0;
		real_t animCharge = 0.0;
		real_t animQtyChange = 0.0;
		Uint32 animChargeStartTicks = 0;
		int highlightedSlot = -1;
		struct DiscoveryAnim_t
		{
			Uint32 startTicks = 0;
			Uint32 processedOnTick = 0;
			std::string name = "";
			DiscoveryAnim_t() :
				startTicks(ticks),
				processedOnTick(0)
			{}
		};
		std::unordered_map<std::string, DiscoveryAnim_t> labelDiscoveries;

		enum InvalidActionFeedback_t : int
		{
			INVALID_ACTION_NONE,
			INVALID_ACTION_SHAKE_PROMPT,
			INVALID_ACTION_NO_CHARGE
		};
		InvalidActionFeedback_t invalidActionType = INVALID_ACTION_NONE;

		int selectedFeatherSlotX = -1;
		int selectedFeatherSlotY = -1;
		static const int MAX_FEATHER_X;
		static const int MAX_FEATHER_Y;
		std::unordered_map<int, Frame*> featherSlotFrames;
		void selectFeatherSlot(const int x, const int y);
		const int getSelectedFeatherSlotX() const { return selectedFeatherSlotX; }
		const int getSelectedFeatherSlotY() const { return selectedFeatherSlotY; }
		Frame* getFeatherSlotFrame(int x, int y) const;
		FeatherActions_t setItemDisplayNameAndPrice(Item* item, bool checkResultOnly);
		bool warpMouseToSelectedFeatherItem(Item* snapToItem, Uint32 flags);
		bool isInscriptionDrawerItemSelected(Item* item);
		bool isItemSelectedToRepairOrInscribe(Item* item);
		bool isInscriptionDrawerOpen() const;
		bool isInscribeOrRepairActive() const;
		void clearItemDisplayed();
		static int heightOffsetWhenNotCompact;
		void scrollToSlot(int x, int y, bool instantly);
		bool isSlotVisible(int x, int y) const;
		bool isItemVisible(Item* item) const;
		const int kNumInscriptionsToDisplayVertical = 5;
		int getNumInscriptionsToDisplayVertical() const;
		void updateFeatherCharge(void* featherChargeText, void* featherChangeChargeText, int currentCharge);
		enum SortTypes_t : int
		{
			SORT_SCROLL_DEFAULT,
			SORT_SCROLL_DISCOVERED,
			SORT_SCROLL_UNKNOWN
		};
		SortTypes_t sortType = SORT_SCROLL_DEFAULT;
		void sortScrolls();
		void updateScrolls();
		std::unordered_map<std::string, std::pair<int, bool>> scrolls;
		std::vector<std::pair<std::string, std::pair<int, bool>>> sortedScrolls;
		bool scrollListRequiresSorting = false;
		void changeSortingType(SortTypes_t newType);
		bool scrollSortFunc(const std::pair<std::string, std::pair<int, bool>>& lhs,
			const std::pair<std::string, std::pair<int, bool>>& rhs);
	};
	FeatherGUI_t featherGUI;

	struct AlchemyGUI_t
	{
		GenericGUIMenu& parentGUI;
		Frame* recipesFrame = nullptr;
		static const int ALCH_SLOT_SECONDARY_POTION_X = -1;
		static const int ALCH_SLOT_BASE_POTION_X = -2;
		static const int ALCH_SLOT_RESULT_POTION_X = -3;
		static const int ALCH_SLOT_RECIPE_PREVIEW_POTION1_X = -4;
		static const int ALCH_SLOT_RECIPE_PREVIEW_POTION2_X = -5;
		struct AlchemyRecipes_t
		{
			AlchemyGUI_t& alchemy;

			bool justifyLeft = true;
			real_t animx = 0.0;
			real_t scrollPercent = 0.0;
			real_t scrollInertia = 0.0;
			int scrollSetpoint = 0;
			real_t scrollAnimateX = 0.0;
			bool isInteractable = true;
			bool bOpen = false;
			bool bFirstTimeSnapCursor = false;
			int currentScrollRow = 0;
			bool panelJustifyInverted = false;

			const int kNumRecipesToDisplayVertical = 6;
			int getNumRecipesToDisplayVertical() const;
			void openRecipePanel();
			void closeRecipePanel();
			void updateRecipePanel();
			void scrollToSlot(int x, int y, bool instantly);
			bool isSlotVisible(int x, int y) const;
			bool isItemVisible(Item* item) const;

			int activateRecipeIndex = -1;

			struct RecipeEntry_t
			{
				Item resultItem;
				Item dummyPotion1;
				Item dummyPotion2;
				int x = 0;
				int y = 0;
				Uint32 basePotionUid = 0;
				Uint32 secondaryPotionUid = 0;
				RecipeEntry_t()
				{
					dummyPotion1.appearance = 0;
					dummyPotion1.type = POTION_EMPTY;
					dummyPotion1.node = nullptr;
					dummyPotion1.status = SERVICABLE;
					dummyPotion1.beatitude = 0;
					dummyPotion1.count = 1;
					dummyPotion1.appearance = 0;
					dummyPotion1.identified = true;
					dummyPotion1.uid = 0;
					dummyPotion1.isDroppable = false;
					dummyPotion1.x = 0;
					dummyPotion1.y = 0;

					dummyPotion2.appearance = 0;
					dummyPotion2.type = POTION_EMPTY;
					dummyPotion2.node = nullptr;
					dummyPotion2.status = SERVICABLE;
					dummyPotion2.beatitude = 0;
					dummyPotion2.count = 1;
					dummyPotion2.appearance = 0;
					dummyPotion2.identified = true;
					dummyPotion2.uid = 0;
					dummyPotion2.isDroppable = false;
					dummyPotion2.x = 0;
					dummyPotion2.y = 0;

					resultItem.appearance = 0;
					resultItem.type = POTION_EMPTY;
					resultItem.node = nullptr;
					resultItem.status = SERVICABLE;
					resultItem.beatitude = 0;
					resultItem.count = 1;
					resultItem.appearance = 0;
					resultItem.identified = true;
					resultItem.uid = 0;
					resultItem.isDroppable = false;
					resultItem.x = 0;
					resultItem.y = 0;
				}
			};
			std::vector<RecipeEntry_t> recipeList;
			std::unordered_map<int, Frame::image_t*> stones;

			AlchemyRecipes_t(AlchemyGUI_t& a) :
				alchemy(a) 
			{}
		} recipes;

		Item alchemyResultPotion;
		Item emptyBottleCount;
		AlchemyGUI_t(GenericGUIMenu& g) :
			parentGUI(g),
			recipes(*this)
		{
			alchemyResultPotion.appearance = 0;
			alchemyResultPotion.type = POTION_EMPTY;
			alchemyResultPotion.node = nullptr;
			alchemyResultPotion.status = SERVICABLE;
			alchemyResultPotion.beatitude = 0;
			alchemyResultPotion.count = 1;
			alchemyResultPotion.appearance = 0;
			alchemyResultPotion.identified = false;
			alchemyResultPotion.uid = 0;
			alchemyResultPotion.isDroppable = false;
			alchemyResultPotion.x = ALCH_SLOT_RESULT_POTION_X;
			alchemyResultPotion.y = 0;

			emptyBottleCount.appearance = 0;
			emptyBottleCount.type = POTION_EMPTY;
			emptyBottleCount.node = nullptr;
			emptyBottleCount.status = SERVICABLE;
			emptyBottleCount.beatitude = 0;
			emptyBottleCount.count = 0;
			emptyBottleCount.appearance = 0;
			emptyBottleCount.identified = true;
			emptyBottleCount.uid = 0;
			emptyBottleCount.isDroppable = false;
			emptyBottleCount.x = 0;
			emptyBottleCount.y = 0;
		}
		enum AlchemyActions_t : int
		{
			ALCHEMY_ACTION_NONE,
			ALCHEMY_ACTION_OK,
			ALCHEMY_ACTION_INVALID_ITEM,
			ALCHEMY_ACTION_UNIDENTIFIED_POTION
		};
		AlchemyActions_t itemActionType = ALCHEMY_ACTION_NONE;
		enum AlchemyView_t : int
		{
			ALCHEMY_VIEW_BREW,
			ALCHEMY_VIEW_RECIPES
		};
		struct AlchNotification_t
		{
			std::string img = "";
			std::string title = "";
			std::string body = "";
			real_t animx = 0.0;
			int state = 0;
			AlchNotification_t(std::string _title, std::string _body, std::string _img)
			{
				title = _title;
				img = _img;
				body = _body;
			}
		};
		std::vector<std::pair<Uint32, AlchNotification_t>> notifications;
		AlchemyView_t currentView = ALCHEMY_VIEW_BREW;
		Frame* alchFrame = nullptr;
		real_t animx = 0.0;
		real_t animTooltip = 0.0;
		Uint32 animTooltipTicks = 0;
		real_t animPotion1 = 0.0;
		int animPotion1StartX = 0;
		int animPotion1StartY = 0;
		int animPotion1DestX = 0;
		int animPotion1DestY = 0;
		Uint32 potion1Uid = 0;
		real_t animPotion2 = 0.0;
		int animPotion2StartX = 0;
		int animPotion2StartY = 0;
		int animPotion2DestX = 0;
		int animPotion2DestY = 0;
		Uint32 potion2Uid = 0;
		real_t animPotionResult = 0.0;
		int animPotionResultStartX = 0;
		int animPotionResultStartY = 0;
		int animPotionResultDestX = 0;
		int animPotionResultDestY = 0;
		Uint32 potionResultUid = 0;
		int animPotionResultCount = 1;
		Uint32 animRandomPotionTicks = 0;
		Uint32 animRandomPotionUpdatedThisTick = 0;
		int animRandomPotionVariation = 0;
		Uint32 animRecipeAutoAddToSlot1Uid = 0;
		Uint32 animRecipeAutoAddToSlot2Uid = 0;
		bool isInteractable = true;
		bool bOpen = false;
		bool bFirstTimeSnapCursor = false;
		void openAlchemyMenu();
		void closeAlchemyMenu();
		void updateAlchemyMenu();
		void createAlchemyMenu();
		bool alchemyGUIHasBeenCreated() const;
		//bool isConstructMenuActive() const;
		//bool isSalvageOrRepairMenuActive() const;
		std::string itemDesc = "";
		int itemType = -1;
		bool itemRequiresTitleReflow = true;
		bool itemTooltipForRecipe = false;
		int selectedAlchemySlotX = -1;
		int selectedAlchemySlotY = -1;
		static const int MAX_ALCH_X;
		static const int MAX_ALCH_Y;
		std::unordered_map<int, Frame*> alchemySlotFrames;
		//bool isTinkerConstructItemSelected(Item* item);
		//bool isSalvageOrRepairItemSelected(Item* item);
		void selectAlchemySlot(const int x, const int y);
		const int getSelectedAlchemySlotX() const { return selectedAlchemySlotX; }
		const int getSelectedAlchemySlotY() const { return selectedAlchemySlotY; }
		Frame* getAlchemySlotFrame(int x, int y) const;
		void setItemDisplayNameAndPrice(Item* item, bool isTooltipForResultPotion, bool isTooltipForRecipe);
		bool warpMouseToSelectedAlchemyItem(Item* snapToItem, Uint32 flags);
		void clearItemDisplayed();
		static int heightOffsetWhenNotCompact;
	};
	AlchemyGUI_t alchemyGUI;
};
extern GenericGUIMenu GenericGUI[MAXPLAYERS];

/*
 * Returns true if the mouse is in the specified bounds, with x1 and y1 specifying the top left corner, and x2 and y2 specifying the bottom right corner.
 */
bool mouseInBounds(const int player, int x1, int x2, int y1, int y2);

//Right sidebar defines.
//#define RIGHTSIDEBAR_X (xres - rightsidebar_titlebar_img->w)
//#define RIGHTSIDEBAR_Y 0
//Note: Just using the spell versions of these for now.
//extern SDL_Surface* rightsidebar_titlebar_img;
//extern SDL_Surface* rightsidebar_slot_img;
//extern SDL_Surface* rightsidebar_slot_highlighted_img;
//extern SDL_Surface* rightsidebar_slot_grayedout_img;
//extern int rightsidebar_height;

void updateRightSidebar(); //Updates the sidebar on the right side of the screen, the one containing spells, skills, etc.

//------book_t Defines-----
//extern SDL_Surface* bookgui_img;
//extern SDL_Surface *nextpage_img;
//extern SDL_Surface *previouspage_img;
//extern SDL_Surface *bookclose_img;
//extern SDL_Surface* book_highlighted_left_img; //Draw this when the mouse is over the left half of the book.
//extern SDL_Surface* book_highlighted_right_img; //Draw this when the mouse is over the right half of the book.
class BookParser_t;

//------Hotbar Defines-----
/*
 * The hotbar itself is an array.
 * NOTE: If the status bar width is changed, you need to change the slot image too. Make sure the status bar width stays divisible by 10.
 */
//extern SDL_Surface* hotbar_img; //A 64x64 slot.
//extern SDL_Surface* hotbar_spell_img; //Drawn when a spell is in the hotbar. TODO: Replace with unique images for every spell. (Or draw this by default if none found?)

//NOTE: Each hotbar slot is "constructed" in loadInterfaceResources() in interface.c. If you add anything, make sure to initialize it there.
typedef struct hotbar_slot_t
{
	/*
	* This is an item's ID. It just resolves to NULL if an item is no longer valid.
	*/
	Uint32 item = 0;
	Item lastItem;
	int lastCategory = -1;
	bool matchesExactLastItem(int player, Item* item);
	void resetLastItem();
	hotbar_slot_t()
	{
		resetLastItem();
	}
	void storeLastItem(Item* item);
} hotbar_slot_t;


// Returns a pointer to a hotbar slot if the mouse is over a hotbar slot
// Used for such things as dragging and dropping items. Uses realtime (mousex/mousey) coords as may be dragging
hotbar_slot_t* getCurrentHotbarUnderMouse(int player, int* outSlotNum = nullptr);

bool warpMouseToSelectedHotbarSlot(const int player);

/*
 * True = automatically place items you pick up in your hotbar.
 * False = don't.
 */
extern bool auto_hotbar_new_items;

extern bool auto_hotbar_categories[NUM_HOTBAR_CATEGORIES]; // true = enable auto add to hotbar. else don't add.

extern int autosort_inventory_categories[NUM_AUTOSORT_CATEGORIES]; // 0 = disable priority sort, fill rightmost first. greater than 0, fill leftmost using value as priority (0 = lowest priority)

extern bool disable_messages;

extern bool right_click_protect;

extern bool auto_appraise_new_items;

extern bool show_game_timer_always;

extern bool hide_playertags;

extern bool show_skill_values;

const char* getInputName(Uint32 scancode);
Sint8* inputPressed(Uint32 scancode);
Sint8* inputPressedForPlayer(int player, Uint32 scancode);

enum CloseGUIShootmode : int
{
	DONT_CHANGE_SHOOTMODE,
	CLOSEGUI_ENABLE_SHOOTMODE
};
enum CloseGUIIgnore : int
{
	CLOSEGUI_CLOSE_ALL,
	CLOSEGUI_DONT_CLOSE_FOLLOWERGUI,
	CLOSEGUI_DONT_CLOSE_CHEST,
	CLOSEGUI_DONT_CLOSE_SHOP,
	CLOSEGUI_DONT_CLOSE_INVENTORY,
	CLOSEGUI_DONT_CLOSE_CALLOUTGUI
};

static const int SCANCODE_UNASSIGNED_BINDING = 399;

const bool hotbarGamepadControlEnabled(const int player);

struct AttackHoverText_t
{
	enum HoverTypes
	{
		ATK_HOVER_TYPE_DEFAULT,
		ATK_HOVER_TYPE_UNARMED,
		ATK_HOVER_TYPE_RANGED,
		ATK_HOVER_TYPE_THROWN,
		ATK_HOVER_TYPE_THROWN_POTION,
		ATK_HOVER_TYPE_THROWN_GEM,
		ATK_HOVER_TYPE_MELEE_WEAPON,
		ATK_HOVER_TYPE_WHIP,
		ATK_HOVER_TYPE_MAGICSTAFF,
		ATK_HOVER_TYPE_TOOL,
		ATK_HOVER_TYPE_PICKAXE,
		ATK_HOVER_TYPE_TOOL_TRAP
	};
	HoverTypes hoverType = ATK_HOVER_TYPE_DEFAULT;
	Sint32 totalAttack = 0;
	Sint32 weaponBonus = 0;
	Sint32 mainAttributeBonus = 0;
	Sint32 secondaryAttributeBonus = 0;
	Sint32 proficiencyBonus = 0;
	real_t proficiencyVariance = 0.0;
	Sint32 attackMinRange = 0;
	Sint32 attackMaxRange = 0;
	Sint32 equipmentAndEffectBonus = 0;
	int proficiency = -1;
};
Sint32 displayAttackPower(const int player, AttackHoverText_t& output);

class MinimapPing
{
public:
	Uint32 tickStart;
	Uint8 player;
	Uint8 x;
	Uint8 y;
	bool radiusPing;
	enum PingType : Uint8
	{
		PING_DEFAULT,
		PING_DEATH_MARKER
	};
	PingType pingType = PING_DEFAULT;
	MinimapPing(Uint32 tickStart, Uint8 player, Uint8 x, Uint8 y) :
		tickStart(tickStart),
		player(player),
		x(x),
		y(y),
		radiusPing(false),
		pingType(PING_DEFAULT) {}

	MinimapPing(Uint32 tickStart, Uint8 player, Uint8 x, Uint8 y, bool radiusPing) :
		tickStart(tickStart),
		player(player),
		x(x),
		y(y),
		radiusPing(radiusPing),
		pingType(PING_DEFAULT) {}

	MinimapPing(Uint32 tickStart, Uint8 player, Uint8 x, Uint8 y, bool radiusPing, PingType pingType) :
		tickStart(tickStart),
		player(player),
		x(x),
		y(y),
		radiusPing(radiusPing),
		pingType(pingType) {}
};

extern std::vector<MinimapPing> minimapPings[MAXPLAYERS];
void minimapPingAdd(const int srcPlayer, const int destPlayer, MinimapPing newPing);
extern int minimapPingGimpTimer[MAXPLAYERS];
extern SDL_Rect minimaps[MAXPLAYERS];

extern std::vector<std::pair<SDL_Surface**, std::string>> systemResourceImages;

class FollowerRadialMenu
{
public:
	Entity* followerToCommand;
	Entity* recentEntity;
	Entity* entityToInteractWith;
	int menuX; // starting mouse coordinates that are the center of the circle.
	int menuY; // starting mouse coordinates that are the center of the circle.
	int optionSelected; // current moused over option.
	int optionPrevious; // previously selected option.
	bool selectMoveTo; // player is choosing a point or target to interact with.
	int moveToX; // x position for follower to move to.
	int moveToY; // y position for follower to move to.
	bool menuToggleClick; // user pressed menu key but did not select option before letting go. keeps the menu open without input.
	bool holdWheel; // user pressed quick menu for last follower.
	char interactText[128]; // user moused over object while selecting interact object.
	bool accessedMenuFromPartySheet; // right click from party sheet will warp mouse back after a selection.
	int partySheetMouseX; // store mouse x cooord for accessedMenuFromPartySheet warp.
	int partySheetMouseY; // store mouse y cooord for accessedMenuFromPartySheet warp.
	int sidebarScrollIndex; // entries scrolled in the sidebar list if overflowed with followers.
	int maxMonstersToDraw;
	int gui_player = 0;
	Frame* followerFrame;

	FollowerRadialMenu() :
		followerFrame(nullptr),
		followerToCommand(nullptr),
		recentEntity(nullptr),
		entityToInteractWith(nullptr),
		menuX(-1),
		menuY(-1),
		optionSelected(-1),
		optionPrevious(-1),
		selectMoveTo(false),
		moveToX(-1),
		moveToY(-1),
		menuToggleClick(false),
		holdWheel(false),
		accessedMenuFromPartySheet(false),
		partySheetMouseX(-1),
		partySheetMouseY(-1),
		sidebarScrollIndex(0),
		maxMonstersToDraw(5)
	{
		memset(interactText, 0, 128);
	}

	void createFollowerMenuGUI();
	bool followerGUIHasBeenCreated() const;
	static void loadFollowerJSON();
	enum PanelDirections : int
	{
		NORTH,
		NORTHWEST,
		WEST,
		SOUTHWEST,
		SOUTH,
		SOUTHEAST,
		EAST,
		NORTHEAST,
		PANEL_DIRECTION_END
	};
	struct PanelEntry
	{
		int x = 0;
		int y = 0;
		std::string path = "";
		std::string path_locked = "";
		std::string path_hover = "";
		std::string path_locked_hover = "";
		int icon_offsetx = 0;
		int icon_offsety = 0;
	};
	static std::vector<PanelEntry> panelEntries;
	static std::vector<PanelEntry> panelEntriesAlternate;
	struct IconEntry
	{
		std::string name = "";
		int id = -1;
		std::string path = "";
		std::string path_hover = "";
		std::string path_active = "";
		std::string path_active_hover = "";
		int icon_offsetx = 0;
		int icon_offsety = 0;
		std::map<std::string, std::pair<std::string, std::set<int>>> text_map;
	};
	static std::map<std::string, IconEntry> iconEntries;
	static int followerWheelRadius;
	static int followerWheelButtonThickness;
	static int followerWheelFrameOffsetX;
	static int followerWheelFrameOffsetY;
	static int followerWheelInnerCircleRadiusOffset;
	static int followerWheelInnerCircleRadiusOffsetAlternate;

	real_t animTitle = 0.0;
	real_t animWheel = 0.0;
	Uint32 openedThisTick = 0;
	real_t animInvalidAction = 0.0;
	Uint32 animInvalidActionTicks = 0;

	bool followerMenuIsOpen();
	void drawFollowerMenu();
	void initfollowerMenuGUICursor(bool openInventory);
	void closeFollowerMenuGUI(bool clearRecentEntity = false);
	void selectNextFollower();
	int numMonstersToDrawInParty();
	void updateScrollPartySheet();
	bool allowedInteractEntity(Entity& selectedEntity, bool updateInteractText = true);
	int optionDisabledForCreature(int playerSkillLVL, int monsterType, int option, Entity* follower);
	bool allowedClassToggle(int monsterType);
	bool allowedItemPickupToggle(int monsterType);
	static bool allowedInteractFood(int monsterType);
	static bool allowedInteractWorld(int monsterType);
	bool allowedInteractItems(int monsterType);
	bool attackCommandOnly(int monsterType);
	void monsterGyroBotConvertCommand(int* option);
	bool monsterGyroBotOnlyCommand(int option);
	bool monsterGyroBotDisallowedCommands(int option);
	bool isTinkeringFollower(int type);
	void setPlayer(const int p) { gui_player = p; }
	const int getPlayer() const { return gui_player; }
};
extern FollowerRadialMenu FollowerMenu[MAXPLAYERS];

struct CalloutRadialMenu
{
	int menuX; // starting mouse coordinates that are the center of the circle.
	int menuY; // starting mouse coordinates that are the center of the circle.
	int optionSelected; // current moused over option.
	bool selectMoveTo; // player is choosing a point or target to interact with.
	int moveToX; // x position for follower to move to.
	int moveToY; // y position for follower to move to.
	bool menuToggleClick; // user pressed menu key but did not select option before letting go. keeps the menu open without input.
	bool holdWheel; // user pressed quick menu for last follower.
	char interactText[128]; // user moused over object while selecting interact object.
	int maxMonstersToDraw;
	int gui_player = 0;
	Frame* calloutFrame = nullptr;
	Frame* calloutPingFrame = nullptr;
	bool bOpen = false;
	Uint32 lockOnEntityUid = 0;

	CalloutRadialMenu() :
		calloutFrame(nullptr),
		calloutPingFrame(nullptr),
		menuX(-1),
		menuY(-1),
		optionSelected(-1),
		selectMoveTo(false),
		moveToX(-1),
		moveToY(-1),
		menuToggleClick(false),
		holdWheel(false),
		bOpen(false)
	{
		memset(interactText, 0, 128);
	}

	void createCalloutMenuGUI();
	bool calloutGUIHasBeenCreated() const;
	static void loadCalloutJSON();
	enum PanelDirections : int
	{
		NORTH,
		NORTHWEST,
		WEST,
		SOUTHWEST,
		SOUTH,
		SOUTHEAST,
		EAST,
		NORTHEAST,
		PANEL_DIRECTION_END
	};
	struct PanelEntry
	{
		int x = 0;
		int y = 0;
		std::string path = "";
		std::string path_hover = "";
		int icon_offsetx = 0;
		int icon_offsety = 0;
	};
	static std::vector<PanelEntry> panelEntries;
	struct IconEntry
	{
		std::string name = "";
		int id = -1;
		std::string path = "";
		std::string path_hover = "";
		std::string path_active = "";
		std::string path_active_hover = "";
		int icon_offsetx = 0;
		int icon_offsety = 0;
		struct IconEntryText_t
		{
			std::string bannerText = "";
			std::set<int> bannerHighlights;
			std::string worldMsgSays = "";
			std::string worldMsg = "";
			std::string worldMsgEmote = "";
			std::string worldMsgEmoteYou = "";
			std::string worldMsgEmoteToYou = "";
			std::string worldIconTag = "";
			std::string worldIconTagMini = "";
		};
		std::map<std::string, IconEntryText_t> text_map;
	};
	static std::map<std::string, IconEntry> iconEntries;
	struct WorldIconEntry_t
	{
		std::string pathDefault = "";
		std::string pathPlayer1 = "";
		std::string pathPlayer2 = "";
		std::string pathPlayer3 = "";
		std::string pathPlayer4 = "";
		std::string pathPlayerX = "";
		int id = 0;
		std::string& getPlayerIconPath(const int playernum);
	};
	static std::map<std::string, WorldIconEntry_t> worldIconEntries;
	static std::map<std::string, std::string> helpDescriptors;
	static std::map<int, std::string> worldIconIDToEntryKey;
	static int followerWheelRadius;
	static int followerWheelButtonThickness;
	static int followerWheelFrameOffsetX;
	static int followerWheelFrameOffsetY;
	static int followerWheelInnerCircleRadiusOffset;
	static int followerWheelInnerCircleRadiusOffsetAlternate;
	static int CALLOUT_SFX_NEUTRAL;
	static int CALLOUT_SFX_NEGATIVE;
	static int CALLOUT_SFX_POSITIVE;

	enum CalloutCommand : int
	{
		CALLOUT_CMD_LOOK,
		CALLOUT_CMD_HELP,
		CALLOUT_CMD_NEGATIVE,
		CALLOUT_CMD_SOUTHWEST,
		CALLOUT_CMD_SOUTH,
		CALLOUT_CMD_SOUTHEAST,
		CALLOUT_CMD_AFFIRMATIVE,
		CALLOUT_CMD_MOVE,
		CALLOUT_CMD_CANCEL,
		CALLOUT_CMD_SELECT,
		CALLOUT_CMD_END,
		CALLOUT_CMD_THANKS
	};
	int getPlayerForDirectPlayerCmd(const int player, const CalloutCommand cmd);
	enum CalloutType : int
	{
		CALLOUT_TYPE_NO_TARGET,
		CALLOUT_TYPE_NPC,
		CALLOUT_TYPE_PLAYER,
		CALLOUT_TYPE_BOULDER,
		CALLOUT_TYPE_TRAP,
		CALLOUT_TYPE_GENERIC_INTERACTABLE,
		CALLOUT_TYPE_CHEST,
		CALLOUT_TYPE_ITEM,
		CALLOUT_TYPE_SWITCH,
		CALLOUT_TYPE_SWITCH_ON,
		CALLOUT_TYPE_SWITCH_OFF,
		CALLOUT_TYPE_SHRINE,
		CALLOUT_TYPE_EXIT,
		CALLOUT_TYPE_SECRET_EXIT,
		CALLOUT_TYPE_SECRET_ENTRANCE,
		CALLOUT_TYPE_GOLD,
		CALLOUT_TYPE_FOUNTAIN,
		CALLOUT_TYPE_SINK,
		CALLOUT_TYPE_NPC_ENEMY,
		CALLOUT_TYPE_NPC_PLAYERALLY,
		CALLOUT_TYPE_TELEPORTER_LADDER_UP,
		CALLOUT_TYPE_TELEPORTER_LADDER_DOWN,
		CALLOUT_TYPE_TELEPORTER_PORTAL,
		CALLOUT_TYPE_BOMB_TRAP,
		CALLOUT_TYPE_COLLIDER_BREAKABLE,
		CALLOUT_TYPE_BELL,
		CALLOUT_TYPE_DAEDALUS,
		CALLOUT_TYPE_ASSIST_SHRINE
		/*,CALLOUT_TYPE_PEDESTAL*/
	};
	enum CalloutHelpFlags : int
	{
		CALLOUT_HELP_FOOD_HUNGRY =		0b1,
		CALLOUT_HELP_BLOOD_HUNGRY =		0b10,
		CALLOUT_HELP_FOOD_WEAK =		0b100,
		CALLOUT_HELP_BLOOD_WEAK =		0b1000,
		CALLOUT_HELP_FOOD_STARVING =	0b10000,
		CALLOUT_HELP_BLOOD_STARVING =	0b100000,
		CALLOUT_HELP_STEAM_CRITICAL =	0b1000000,
		CALLOUT_HELP_HP_LOW =			0b10000000,
		CALLOUT_HELP_HP_CRITICAL =		0b100000000,
		CALLOUT_HELP_NEGATIVE_FX =		0b1000000000,
	};
	Uint32 clientCalloutHelpFlags = 0;
	struct CalloutParticle_t
	{
		real_t x = 0.0;
		real_t y = 0.0;
		real_t z = 0.0;
		Uint32 entityUid = 0;
		Uint32 ticks = 0;
		Uint32 lifetime = 1;
		Uint32 creationTick = 0;
		CalloutCommand cmd = CALLOUT_CMD_END;
		CalloutType type = CALLOUT_TYPE_NO_TARGET;
		bool expired = false;
		bool lockOnScreen[MAXPLAYERS];
		int playerColor = -1;
		int tagID = -1;
		int tagSmallID = -1;
		int animateState = 0;
		int animateStateInit = 0;
		real_t scale = 1.0;
		real_t animateX = 0.0;
		real_t animateScaleForPlayerView[MAXPLAYERS];
		real_t animateBounce = 0.0;
		real_t animateY = 0.0;
		bool noUpdate = false;
		bool selfCallout = false;
		bool doMessage = true;
		Uint32 messageSentTick = 0;
		bool big[MAXPLAYERS];
		void animate();
		void init(const int player);
		CalloutParticle_t() = default;
		CalloutParticle_t(const int player, real_t _x, real_t _y, real_t _z, Uint32 _uid, CalloutCommand _cmd) :
			x(_x),
			y(_y),
			z(_z),
			entityUid(_uid),
			cmd(_cmd)
		{
			init(player);
		};
		static Uint32 kParticleLifetime;
	};
	std::map<Uint32, CalloutParticle_t> callouts;

	real_t animTitle = 0.0;
	real_t animWheel = 0.0;
	Uint32 openedThisTick = 0;
	real_t animInvalidAction = 0.0;
	Uint32 animInvalidActionTicks = 0;
	Uint32 updatedThisTick = 0;

	bool calloutMenuIsOpen();
	void drawCalloutMenu();
	void initCalloutMenuGUICursor(bool openInventory);
	void closeCalloutMenuGUI();
	bool allowedInteractEntity(Entity& selectedEntity, bool updateInteractText = true);
	bool createParticleCallout(real_t x, real_t y, real_t z, Uint32 uid, CalloutCommand _cmd = CALLOUT_CMD_LOOK); // if true, send message
	bool createParticleCallout(Entity* entity, CalloutCommand _cmd, Uint32 overrideUID = 0); // if true, send message
	enum SetCalloutTextTypes : int {
		SET_CALLOUT_BANNER_TEXT,
		SET_CALLOUT_WORLD_TEXT,
		SET_CALLOUT_ICON_KEY
	};
	std::string setCalloutText(Field* field, const char* iconName, Uint32 color, CalloutCommand cmd, SetCalloutTextTypes setType, const int targetPlayer);
	std::string getCalloutMessage(const IconEntry::IconEntryText_t& text_map, const char* object, const int targetPlayer);
	void sendCalloutText(CalloutCommand cmd);
	static std::string getCalloutKeyForCommand(CalloutCommand cmd);
	static CalloutType getCalloutTypeForUid(const int player, Uint32 uid);
	static CalloutType getCalloutTypeForEntity(const int player, Entity* parent);
	static void drawCallouts(const int playernum);
	/*void selectNextFollower();
	int numMonstersToDrawInParty();
	void updateScrollPartySheet();
	int optionDisabledForCreature(int playerSkillLVL, int monsterType, int option, Entity* follower);
	bool allowedClassToggle(int monsterType);
	bool allowedItemPickupToggle(int monsterType);
	static bool allowedInteractFood(int monsterType);
	static bool allowedInteractWorld(int monsterType);
	bool allowedInteractItems(int monsterType);
	bool attackCommandOnly(int monsterType);
	void monsterGyroBotConvertCommand(int* option);
	bool monsterGyroBotOnlyCommand(int option);
	bool monsterGyroBotDisallowedCommands(int option);
	bool isTinkeringFollower(int type);*/
	void setPlayer(const int p) { gui_player = p; }
	const int getPlayer() const { return gui_player; }
	static bool uidMatchesPlayer(const int playernum, const Uint32 uid);
	static Uint32 getPlayerUid(const int playernum);
	static bool calloutMenuEnabledForGamemode();
	void update();
};
extern CalloutRadialMenu CalloutMenu[MAXPLAYERS];

std::string getItemSpritePath(const int player, Item& item);

enum ItemContextMenuPrompts {
	PROMPT_EQUIP,
	PROMPT_UNEQUIP,
	PROMPT_SPELL_EQUIP,
	PROMPT_SPELL_QUICKCAST,
	PROMPT_APPRAISE,
	PROMPT_DROPDOWN,
	PROMPT_INTERACT,
	PROMPT_INTERACT_SPELLBOOK_HOTBAR,
	PROMPT_EAT,
	PROMPT_CONSUME,
	PROMPT_CONSUME_ALTERNATE,
	PROMPT_INSPECT,
	PROMPT_INSPECT_ALTERNATE,
	PROMPT_SELL,
	PROMPT_BUY,
	PROMPT_STORE_CHEST,
	PROMPT_RETRIEVE_CHEST,
	PROMPT_RETRIEVE_CHEST_ALL,
	PROMPT_STORE_CHEST_ALL,
	PROMPT_DROP,
	PROMPT_TINKER,
	PROMPT_GRAB,
	PROMPT_UNEQUIP_FOR_DROP,
	PROMPT_CLEAR_HOTBAR_SLOT
};

std::vector<ItemContextMenuPrompts> getContextMenuOptionsForItem(const int player, Item* item);
std::vector<ItemContextMenuPrompts> getContextTooltipOptionsForItem(const int player, Item* item, int useDropdownMenu, bool hotbarItem);
const char* getContextMenuLangEntry(const int player, const ItemContextMenuPrompts prompt, Item& item);
std::string getContextMenuOptionBindingName(const int player, const ItemContextMenuPrompts prompt);
void cleanupMinimapTextures();
