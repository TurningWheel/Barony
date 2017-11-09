/*-------------------------------------------------------------------------------

	BARONY
	File: items.hpp
	Desc: contains names and definitions for items

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include "main.hpp"
#include "entity.hpp"

// variables
extern bool weaponSwitch;
extern bool shieldSwitch;

// items
typedef enum ItemType
{
	WOODEN_SHIELD,
	QUARTERSTAFF,
	BRONZE_SWORD,
	BRONZE_MACE,
	BRONZE_AXE,
	BRONZE_SHIELD,
	SLING,
	IRON_SPEAR,
	IRON_SWORD,
	IRON_MACE,
	IRON_AXE,
	IRON_SHIELD,
	SHORTBOW,
	STEEL_HALBERD,
	STEEL_SWORD,
	STEEL_MACE,
	STEEL_AXE,
	STEEL_SHIELD,
	STEEL_SHIELD_RESISTANCE,
	CROSSBOW,
	GLOVES,
	GLOVES_DEXTERITY,
	BRACERS,
	BRACERS_CONSTITUTION,
	GAUNTLETS,
	GAUNTLETS_STRENGTH,
	CLOAK,
	CLOAK_MAGICREFLECTION,
	CLOAK_INVISIBILITY,
	CLOAK_PROTECTION,
	LEATHER_BOOTS,
	LEATHER_BOOTS_SPEED,
	IRON_BOOTS,
	IRON_BOOTS_WATERWALKING,
	STEEL_BOOTS,
	STEEL_BOOTS_LEVITATION,
	STEEL_BOOTS_FEATHER,
	LEATHER_BREASTPIECE,
	IRON_BREASTPIECE,
	STEEL_BREASTPIECE,
	HAT_PHRYGIAN,
	HAT_HOOD,
	HAT_WIZARD,
	HAT_JESTER,
	LEATHER_HELM,
	IRON_HELM,
	STEEL_HELM,
	AMULET_SEXCHANGE,
	AMULET_LIFESAVING,
	AMULET_WATERBREATHING,
	AMULET_MAGICREFLECTION,
	AMULET_STRANGULATION,
	AMULET_POISONRESISTANCE,
	POTION_WATER,
	POTION_BOOZE,
	POTION_JUICE,
	POTION_SICKNESS,
	POTION_CONFUSION,
	POTION_EXTRAHEALING,
	POTION_HEALING,
	POTION_CUREAILMENT,
	POTION_BLINDNESS,
	POTION_RESTOREMAGIC,
	POTION_INVISIBILITY,
	POTION_LEVITATION,
	POTION_SPEED,
	POTION_ACID,
	POTION_PARALYSIS,
	SCROLL_MAIL,
	SCROLL_IDENTIFY,
	SCROLL_LIGHT,
	SCROLL_BLANK,
	SCROLL_ENCHANTWEAPON,
	SCROLL_ENCHANTARMOR,
	SCROLL_REMOVECURSE,
	SCROLL_FIRE,
	SCROLL_FOOD,
	SCROLL_MAGICMAPPING,
	SCROLL_REPAIR,
	SCROLL_DESTROYARMOR,
	SCROLL_TELEPORTATION,
	SCROLL_SUMMON,
	MAGICSTAFF_LIGHT,
	MAGICSTAFF_DIGGING,
	MAGICSTAFF_LOCKING,
	MAGICSTAFF_MAGICMISSILE,
	MAGICSTAFF_OPENING,
	MAGICSTAFF_SLOW,
	MAGICSTAFF_COLD,
	MAGICSTAFF_FIRE,
	MAGICSTAFF_LIGHTNING,
	MAGICSTAFF_SLEEP,
	RING_ADORNMENT,
	RING_SLOWDIGESTION,
	RING_PROTECTION,
	RING_WARNING,
	RING_STRENGTH,
	RING_CONSTITUTION,
	RING_INVISIBILITY,
	RING_MAGICRESISTANCE,
	RING_CONFLICT,
	RING_LEVITATION,
	RING_REGENERATION,
	RING_TELEPORTATION,
	SPELLBOOK_FORCEBOLT,
	SPELLBOOK_MAGICMISSILE,
	SPELLBOOK_COLD,
	SPELLBOOK_FIREBALL,
	SPELLBOOK_LIGHT,
	SPELLBOOK_REMOVECURSE,
	SPELLBOOK_LIGHTNING,
	SPELLBOOK_IDENTIFY,
	SPELLBOOK_MAGICMAPPING,
	SPELLBOOK_SLEEP,
	SPELLBOOK_CONFUSE,
	SPELLBOOK_SLOW,
	SPELLBOOK_OPENING,
	SPELLBOOK_LOCKING,
	SPELLBOOK_LEVITATION,
	SPELLBOOK_INVISIBILITY,
	SPELLBOOK_TELEPORTATION,
	SPELLBOOK_HEALING,
	SPELLBOOK_EXTRAHEALING,
	SPELLBOOK_CUREAILMENT,
	SPELLBOOK_DIG,
	GEM_ROCK,
	GEM_LUCK,
	GEM_GARNET,
	GEM_RUBY,
	GEM_JACINTH,
	GEM_AMBER,
	GEM_CITRINE,
	GEM_JADE,
	GEM_EMERALD,
	GEM_SAPPHIRE,
	GEM_AQUAMARINE,
	GEM_AMETHYST,
	GEM_FLUORITE,
	GEM_OPAL,
	GEM_DIAMOND,
	GEM_JETSTONE,
	GEM_OBSIDIAN,
	GEM_GLASS,
	TOOL_PICKAXE,
	TOOL_TINOPENER,
	TOOL_MIRROR,
	TOOL_LOCKPICK,
	TOOL_SKELETONKEY,
	TOOL_TORCH,
	TOOL_LANTERN,
	TOOL_BLINDFOLD,
	TOOL_TOWEL,
	TOOL_GLASSES,
	TOOL_BEARTRAP,
	FOOD_BREAD,
	FOOD_CREAMPIE,
	FOOD_CHEESE,
	FOOD_APPLE,
	FOOD_MEAT,
	FOOD_FISH,
	FOOD_TIN,
	READABLE_BOOK,
	SPELL_ITEM,
	ARTIFACT_SWORD,
	ARTIFACT_MACE,
	ARTIFACT_SPEAR,
	ARTIFACT_AXE,
	ARTIFACT_BOW,
	ARTIFACT_BREASTPIECE,
	ARTIFACT_HELM,
	ARTIFACT_BOOTS,
	ARTIFACT_CLOAK,
	ARTIFACT_GLOVES,
	CRYSTAL_BREASTPIECE,
	CRYSTAL_HELM,
	CRYSTAL_BOOTS,
	CRYSTAL_SHIELD,
	CRYSTAL_GLOVES,
	VAMPIRE_DOUBLET,
	WIZARD_DOUBLET,
	HEALER_DOUBLET,
	MIRROR_SHIELD,
	BRASS_KNUCKLES,
	IRON_KNUCKLES,
	SPIKED_GAUNTLETS,
	FOOD_TOMALLEY,
	TOOL_CRYSTALSHARD,
	CRYSTAL_SWORD,
	CRYSTAL_SPEAR,
	CRYSTAL_BATTLEAXE,
	CRYSTAL_MACE,
	BRONZE_TOMAHAWK,
	IRON_DAGGER,
	STEEL_CHAKRAM,
	CRYSTAL_SHURIKEN,
	CLOAK_BLACK,
	MAGICSTAFF_STONEBLOOD,
	MAGICSTAFF_BLEED,
	MAGICSTAFF_SUMMON,
	TOOL_BLINDFOLD_FOCUS,
	TOOL_BLINDFOLD_TELEPATHY,
	SPELLBOOK_SUMMON,
	SPELLBOOK_STONEBLOOD,
	SPELLBOOK_BLEED,
	SPELLBOOK_REFLECT_MAGIC,
	SPELLBOOK_ACID_SPRAY,
	SPELLBOOK_STEAL_WEAPON,
	SPELLBOOK_DRAIN_SOUL,
	SPELLBOOK_VAMPIRIC_AURA,
	SPELLBOOK_BLANK_5,
	POTION_EMPTY,
	ARTIFACT_ORB_BLUE,
	ARTIFACT_ORB_RED,
	ARTIFACT_ORB_PURPLE,
	ARTIFACT_ORB_GREEN
} ItemType;
const int NUMITEMS = 215;

//NOTE: If you change this, make sure to update NUMCATEGORIES in game.h to reflect the total number of categories. Not doing that will make bad things happen.
typedef enum Category
{
	WEAPON,
	ARMOR,
	AMULET,
	POTION,
	SCROLL,
	MAGICSTAFF,
	RING,
	SPELLBOOK,
	GEM,
	THROWN,
	TOOL,
	FOOD,
	BOOK,
	SPELL_CAT
} Category;

typedef enum Status
{
	BROKEN,
	DECREPIT,
	WORN,
	SERVICABLE,
	EXCELLENT
} Status;

typedef enum EquipmentType
{
	TYPE_NONE,
	TYPE_HELM,
	TYPE_HAT,
	TYPE_BREASTPIECE,
	TYPE_BOOTS,
	TYPE_SHIELD,
	TYPE_GLOVES,
	TYPE_CLOAK,
	TYPE_RING,
	TYPE_AMULET,
	TYPE_MASK,
	TYPE_SWORD,
	TYPE_AXE,
	TYPE_SPEAR,
	TYPE_MACE,
	TYPE_BOW,
	TYPE_PROJECTILE,
	TYPE_OFFHAND
} EquipmentType;

class SummonProperties
{
	//TODO: Store monster stats.
public:
	SummonProperties();
	~SummonProperties();
};

// inventory item structure
class Item
{
public:
	ItemType type;
	Status status;

	Sint16 beatitude;  // blessedness
	Sint16 count;      // how many of item
	Uint32 appearance; // large random static number
	bool identified;   // if the item is identified or not
	Uint32 uid;        // item uid
	Sint32 x, y;       // slot coordinates in item grid

	// weight, category and other generic info reported by function calls

	node_t* node;

	/*
	 * Gems use this to store information about what sort of creature they contain.
	 */
	//SummonProperties *captured_monster;
	//I wish there was an easy way to do this.
	//As it stands, no item destructor is called , so this would lead to a memory leak.
	//And tracking down every time an item gets deleted and calling an item destructor would be quite a doozey.

	char* description();
	char* getName();

	//General Functions.
	Sint32 weaponGetAttack() const; //Returns the tohit of the weapon.
	Sint32 armorGetAC() const;
	bool canUnequip(); //Returns true if the item can be unequipped (not cursed), false if it can't (cursed).
	int buyValue(int player);
	int sellValue(int player);

	void apply(int player, Entity* entity);

	//Item usage functions.
	void applySkeletonKey(int player, Entity& entity);
	void applyLockpick(int player, Entity& entity);
	void applyOrb(int player, ItemType type, Entity& entity);

	//-----ITEM COMPARISON FUNCTIONS-----
	/*
	 * Returns which weapon hits harder.
	 */
	static bool isThisABetterWeapon(const Item& newWeapon, const Item* weaponAlreadyHave);
	static bool isThisABetterArmor(const Item& newArmor, const Item* armorAlreadyHave); //Also checks shields.

	bool isShield() const;

};
extern Uint32 itemuids;

static const int INVENTORY_SIZEX = 12;
static const int INVENTORY_SIZEY = 3;
#define INVENTORY_SIZE ((INVENTORY_SIZEX)*(INVENTORY_SIZEY))
static const int INVENTORY_SLOTSIZE = 40;
#define INVENTORY_STARTX ((xres)/2-(INVENTORY_SIZEX)*(INVENTORY_SLOTSIZE)/2-inventory_mode_item_img->w/2)
static const int INVENTORY_STARTY = 10;

extern Item* selectedItem;

// item generic
class ItemGeneric
{
public:
	char* name_identified;      // identified item name
	char* name_unidentified;    // unidentified item name
	int index;                  // world model
	int fpindex;                // first person model
	int variations;             // number of model variations
	int weight;                 // weight per item
	int value;                  // value per item
	list_t images;              // item image filenames (inventory)
	list_t surfaces;            // item image surfaces (inventory)
	Category category;          // item category
};
extern ItemGeneric items[NUMITEMS];

//----------Item usage functions----------
void item_PotionWater(Item* item, Entity* entity);
void item_PotionBooze(Item* item, Entity* entity, bool shouldConsumeItem = true);
void item_PotionJuice(Item* item, Entity* entity);
void item_PotionSickness(Item* item, Entity* entity);
void item_PotionConfusion(Item* item, Entity* entity);
void item_PotionCureAilment(Item* item, Entity* entity);
void item_PotionBlindness(Item* item, Entity* entity);
void item_PotionHealing(Item* item, Entity* entity, bool shouldConsumeItem = true);
void item_PotionExtraHealing(Item* item, Entity* entity, bool shouldConsumeItem = true);
void item_PotionRestoreMagic(Item* item, Entity* entity);
void item_PotionInvisibility(Item* item, Entity* entity);
void item_PotionLevitation(Item* item, Entity* entity);
void item_PotionSpeed(Item* item, Entity* entity);
void item_PotionAcid(Item* item, Entity* entity);
void item_PotionParalysis(Item* item, Entity* entity);
void item_ScrollMail(Item* item, int player);
void item_ScrollIdentify(Item* item, int player);
void item_ScrollLight(Item* item, int player);
void item_ScrollBlank(Item* item, int player);
void item_ScrollEnchantWeapon(Item* item, int player);
void item_ScrollEnchantArmor(Item* item, int player);
void item_ScrollRemoveCurse(Item* item, int player);
void item_ScrollFire(Item* item, int player);
void item_ScrollFood(Item* item, int player);
void item_ScrollMagicMapping(Item* item, int player);
void item_ScrollRepair(Item* item, int player);
void item_ScrollDestroyArmor(Item* item, int player);
void item_ScrollTeleportation(Item* item, int player);
void item_ScrollSummon(Item* item, int player);
void item_AmuletSexChange(Item* item, int player);
void item_ToolTowel(Item* item, int player);
void item_ToolTinOpener(Item* item, int player);
void item_ToolMirror(Item* item, int player);
void item_ToolBeartrap(Item* item, int player);
void item_Food(Item* item, int player);
void item_FoodTin(Item* item, int player);
void item_Gem(Item* item, int player);
void item_Spellbook(Item* item, int player);

//General functions.
Item* newItem(ItemType type, Status status, Sint16 beatitude, Sint16 count, Uint32 appearance, bool identified, list_t* inventory);
void addItemToMonsterInventory(Item &item, list_t& inventory);
Item* uidToItem(Uint32 uid);
ItemType itemCurve(Category cat);
Item* newItemFromEntity(Entity* entity); //Make sure to call free(item).
Entity* dropItemMonster(Item* item, Entity* monster, Stat* monsterStats, Sint16 count = 1);
Item** itemSlot(Stat* myStats, Item* item);

enum Category itemCategory(const Item* item);
Sint32 itemModel(Item* item);
Sint32 itemModelFirstperson(Item* item);
SDL_Surface* itemSprite(Item* item);
void consumeItem(Item* item); //NOTE: Items have to be unequipped before calling this function on them.
void dropItem(Item* item, int player);
void useItem(Item* item, int player);
void equipItem(Item* item, Item** slot, int player);
Item* itemPickup(int player, Item* item);
bool itemIsEquipped(const Item* item, int player);

//-----ITEM COMPARISON FUNCS-----
/*
 * Only compares items of the same type.
 */
int itemCompare(const Item* item1, const Item* item2);

/*
 * Returns true if potion is harmful to the player.
 */
bool isPotionBad(const Item& potion);
bool inline isRangedWeapon(const Item& item);
bool inline isMeleeWeapon(const Item& item);

void createCustomInventory(Stat* stats, int itemLimit);
void copyItem(Item* itemToSet, Item* itemToCopy);
bool swapMonsterWeaponWithInventoryItem(Entity* my, Stat* myStats, node_t* inventoryNode, bool moveStack, bool overrideCursed);
bool monsterUnequipSlot(Stat* myStats, Item** slot, Item* itemToUnequip);
bool monsterUnequipSlotFromCategory(Stat* myStats, Item** slot, Category cat);
node_t* itemNodeInInventory(Stat* myStats, ItemType itemToFind, Category cat);
node_t* spellbookNodeInInventory(Stat* myStats, int spellIDToFInd);
node_t* getRangedWeaponItemNodeInInventory(Stat* myStats, bool includeMagicstaff);
node_t* getMeleeWeaponItemNodeInInventory(Stat* myStats);
ItemType itemTypeWithinGoldValue(Category cat, int minValue, int maxValue);

// unique monster item appearance to avoid being dropped on death.
static const int MONSTER_ITEM_UNDROPPABLE_APPEARANCE = 1234567890;
