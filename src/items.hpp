/*-------------------------------------------------------------------------------

	BARONY
	File: items.hpp
	Desc: contains names and definitions for items

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include "main.hpp"
#include "prng.hpp"

class Entity; // forward declare
class Stat; // forward declare

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
	SPELLBOOK_CHARM_MONSTER,
	POTION_EMPTY,
	ARTIFACT_ORB_BLUE,
	ARTIFACT_ORB_RED,
	ARTIFACT_ORB_PURPLE,
	ARTIFACT_ORB_GREEN,
	TUNIC,
	HAT_FEZ,
	MAGICSTAFF_CHARM,
	POTION_POLYMORPH,
	FOOD_BLOOD,
	CLOAK_BACKPACK,
	TOOL_ALEMBIC,
	POTION_FIRESTORM,
	POTION_ICESTORM,
	POTION_THUNDERSTORM,
	POTION_STRENGTH,
	SUEDE_BOOTS,
	SUEDE_GLOVES,
	CLOAK_SILVER,
	HAT_HOOD_SILVER,
	HAT_HOOD_RED,
	SILVER_DOUBLET,
	SPELLBOOK_REVERT_FORM,
	SPELLBOOK_RAT_FORM,
	SPELLBOOK_SPIDER_FORM,
	SPELLBOOK_TROLL_FORM,
	SPELLBOOK_IMP_FORM,
	SPELLBOOK_SPRAY_WEB,
	SPELLBOOK_POISON,
	SPELLBOOK_SPEED,
	SPELLBOOK_FEAR,
	SPELLBOOK_STRIKE,
	SPELLBOOK_DETECT_FOOD,
	SPELLBOOK_WEAKNESS,
	MASK_SHAMAN,
	SPELLBOOK_AMPLIFY_MAGIC,
	SPELLBOOK_SHADOW_TAG,
	SPELLBOOK_TELEPULL,
	SPELLBOOK_DEMON_ILLU,
	SPELLBOOK_TROLLS_BLOOD,
	SPELLBOOK_SALVAGE,
	TOOL_WHIP,
	SPELLBOOK_FLUTTER,
	SPELLBOOK_DASH,
	SPELLBOOK_SELF_POLYMORPH,
	SPELLBOOK_9,
	SPELLBOOK_10,
	MAGICSTAFF_POISON,
	TOOL_METAL_SCRAP,
	TOOL_MAGIC_SCRAP,
	TOOL_TINKERING_KIT,
	TOOL_SENTRYBOT,
	TOOL_DETONATOR_CHARGE,
	TOOL_BOMB,
	TOOL_SLEEP_BOMB,
	TOOL_FREEZE_BOMB,
	TOOL_TELEPORT_BOMB,
	TOOL_GYROBOT,
	TOOL_SPELLBOT,
	TOOL_DECOY,
	TOOL_DUMMYBOT,
	MACHINIST_APRON,
	ENCHANTED_FEATHER,
	PUNISHER_HOOD,
	SCROLL_CHARGING,
	QUIVER_SILVER,
	QUIVER_PIERCE,
	QUIVER_LIGHTWEIGHT,
	QUIVER_FIRE,
	QUIVER_KNOCKBACK,
	QUIVER_CRYSTAL,
	QUIVER_HUNTING,
	LONGBOW,
	COMPOUND_BOW,
	HEAVY_CROSSBOW,
	BOOMERANG,
	SCROLL_CONJUREARROW,
	MONOCLE,
	TOOL_PLAYER_LOOT_BAG,
	MASK_BANDIT,
	MASK_EYEPATCH,
	MASK_MASQUERADE,
	MASK_MOUTH_ROSE,
	MASK_GOLDEN,
	MASK_SPOOKY,
	MASK_TECH_GOGGLES,
	MASK_HAZARD_GOGGLES,
	MASK_PHANTOM,
	MASK_PIPE,
	MASK_GRASS_SPRIG,
	MASK_PLAGUE,
	MASK_MOUTHKNIFE,
	HAT_SILKEN_BOW,
	HAT_PLUMED_CAP,
	HAT_BYCOCKET,
	HAT_TOPHAT,
	HAT_BANDANA,
	HAT_CIRCLET,
	HAT_CROWN,
	HAT_LAURELS,
	HAT_TURBAN,
	HAT_CROWNED_HELM,
	HAT_WARM,
	HAT_WOLF_HOOD,
	HAT_BEAR_HOOD,
	HAT_STAG_HOOD,
	HAT_BUNNY_HOOD,
	HAT_BOUNTYHUNTER,
	HAT_MITER,
	HAT_HEADDRESS,
	HAT_CHEF,
	HELM_MINING,
	MASK_STEEL_VISOR,
	MASK_CRYSTAL_VISOR,
	MASK_ARTIFACT_VISOR,
	HAT_CIRCLET_WISDOM,
	HAT_HOOD_APPRENTICE,
	HAT_HOOD_ASSASSIN,
	HAT_HOOD_WHISPERS,
	RING_RESOLVE,
	CLOAK_GUARDIAN,
	MASK_MARIGOLD
} ItemType;
const int NUMITEMS = 332;

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
	~SummonProperties() noexcept;

	SummonProperties(const SummonProperties& other) = default;
	SummonProperties(SummonProperties&& other) noexcept = default;
	SummonProperties& operator=(const SummonProperties& other) = default;
	SummonProperties& operator=(SummonProperties&& other) noexcept = default;

protected:

private:

};

enum ItemEquippableSlot : int
{
	EQUIPPABLE_IN_SLOT_WEAPON,
	EQUIPPABLE_IN_SLOT_SHIELD,
	EQUIPPABLE_IN_SLOT_MASK,
	EQUIPPABLE_IN_SLOT_HELM,
	EQUIPPABLE_IN_SLOT_GLOVES,
	EQUIPPABLE_IN_SLOT_BOOTS,
	EQUIPPABLE_IN_SLOT_BREASTPLATE,
	EQUIPPABLE_IN_SLOT_CLOAK,
	EQUIPPABLE_IN_SLOT_AMULET,
	EQUIPPABLE_IN_SLOT_RING,
	NO_EQUIP
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
	Uint32 ownerUid;   // original owner
	Uint32 interactNPCUid; // if NPC is interacting with item
	bool forcedPickupByPlayer; // player used interact on NPC with item on floor
	bool isDroppable; // if item should drop on death
	bool playerSoldItemToShop = false; // if item was sold to a shopkeeper
	bool itemHiddenFromShop = false; // if item needs to be hidden in shop view
	bool notifyIcon = false; // if item draws exclamation as a 'new' untouched item
	Uint8 itemRequireTradingSkillInShop = 0; // if item hidden in shop view until player has trading req
	bool itemSpecialShopConsumable = false; // if item is extra non-standard inventory consumable

	// weight, category and other generic info reported by function calls

	node_t* node = nullptr;

	/*
	 * Gems use this to store information about what sort of creature they contain.
	 */
	//SummonProperties *captured_monster;
	//I wish there was an easy way to do this.
	//As it stands, no item destructor is called , so this would lead to a memory leak.
	//And tracking down every time an item gets deleted and calling an item destructor would be quite a doozey.

	char* description() const;
	char* getName() const;

	//General Functions.
	Sint32 weaponGetAttack(const Stat* wielder = nullptr) const; //Returns the tohit of the weapon.
	Sint32 armorGetAC(const Stat* wielder = nullptr) const;
	bool canUnequip(const Stat* wielder = nullptr); //Returns true if the item can be unequipped (not cursed), false if it can't (cursed).
	int buyValue(int player) const;
	int sellValue(int player) const;
	bool usableWhileShapeshifted(const Stat* wielder = nullptr) const;
	char* getScrollLabel() const;

	void apply(int player, Entity* entity);
	void applyLockpickToWall(int player, int x, int y) const;

	//Item usage functions.
	void applySkeletonKey(int player, Entity& entity);
	void applyLockpick(int player, Entity& entity);
	void applyOrb(int player, ItemType type, Entity& entity);
	void applyEmptyPotion(int player, Entity& entity);
	//-----ITEM COMPARISON FUNCTIONS-----
	/*
	 * Returns which weapon hits harder.
	 */
	static bool isThisABetterWeapon(const Item& newWeapon, const Item* weaponAlreadyHave);
	static bool isThisABetterArmor(const Item& newArmor, const Item* armorAlreadyHave); //Also checks shields.
	bool shouldItemStack(int player, bool ignoreStackLimit = false) const;
	bool shouldItemStackInShop(bool ignoreStackLimit = false);
	int getMaxStackLimit(int player) const;

	bool isShield() const;
	static bool doesItemProvideBeatitudeAC(ItemType type);
	bool doesItemProvidePassiveShieldBonus() const;
	bool doesPotionHarmAlliesOnThrown() const;

	Sint32 potionGetEffectHealth(Entity* my, Stat* myStats) const;
	Sint32 potionGetEffectDamage(Entity* my, Stat* myStats) const;
	Sint32 potionGetEffectDurationMinimum(Entity* my, Stat* myStats) const;
	Sint32 potionGetEffectDurationMaximum(Entity* my, Stat* myStats) const;
	Sint32 potionGetEffectDurationRandom(Entity* my, Stat* myStats) const;
	Sint32 potionGetCursedEffectDurationMinimum(Entity* my, Stat* myStats) const;
	Sint32 potionGetCursedEffectDurationMaximum(Entity* my, Stat* myStats) const;
	Sint32 potionGetCursedEffectDurationRandom(Entity* my, Stat* myStats) const;

	Sint32 getWeight() const;

	void foodTinGetDescriptionIndices(int* a, int* b, int* c) const;
	void foodTinGetDescription(std::string& cookingMethod, std::string& protein, std::string& sides) const;
	int foodGetPukeChance(Stat* eater) const;
	int getLootBagPlayer() const;
	int getLootBagNumItems() const;

	enum ItemBombPlacement : int
	{
		BOMB_FLOOR,
		BOMB_WALL,
		BOMB_CHEST,
		BOMB_DOOR,
		BOMB_COLLIDER
	};
	enum ItemBombFacingDirection : int
	{
		BOMB_UP,
		BOMB_NORTH,
		BOMB_EAST,
		BOMB_SOUTH,
		BOMB_WEST
	};
	enum ItemBombTriggerType : int
	{
		BOMB_TRIGGER_ENEMIES,
		BOMB_TELEPORT_RECEIVER,
		BOMB_TRIGGER_ALL
	};
	void applyBomb(Entity* parent, ItemType type, ItemBombPlacement placement, ItemBombFacingDirection dir, Entity* thrown, Entity* onEntity);
	void applyTinkeringCreation(Entity* parent, Entity* thrown);
	bool unableToEquipDueToSwapWeaponTimer(const int player) const;
	bool tinkeringBotIsMaxHealth() const;
	bool isTinkeringItemWithThrownLimit() const;
};
extern Uint32 itemuids;

// item generic
class ItemGeneric
{
	std::string item_name_identified;      // identified item name
	std::string item_name_unidentified;    // unidentified item name
public:
	int index;                  // world model
	int indexShort;				// short mob world model
	int fpindex;                // first person model
	int variations;             // number of model variations
	int weight;                 // weight per item
	int value;                  // value per item
	list_t images;              // item image filenames (inventory)
	list_t surfaces;            // item image surfaces (inventory)
	Category category;          // item category
	int level;					// item level for random generation
	// equip slot that item can go in
	ItemEquippableSlot item_slot = ItemEquippableSlot::NO_EQUIP;
	std::map<std::string, Sint32> attributes;
	std::string tooltip = "tooltip_default";

	const char* getIdentifiedName() const { return item_name_identified.c_str(); }
	const char* getUnidentifiedName() const { return item_name_unidentified.c_str(); }
	void setIdentifiedName(std::string name) { item_name_identified = name; }
	void setUnidentifiedName(std::string name) { item_name_unidentified = name; }
	bool hasAttribute(std::string attribute)
	{
		if ( attributes.size() > 0 )
		{
			if ( attributes.find(attribute) != attributes.end() )
			{
				return true;
			}
			return false;
		}
		else
		{
			return false;
		}
	}
};
extern ItemGeneric items[NUMITEMS];

//----------Item usage functions----------
bool item_PotionWater(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionBooze(Item*& item, Entity* entity, Entity* usedBy, bool shouldConsumeItem = true);
bool item_PotionJuice(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionSickness(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionConfusion(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionCureAilment(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionBlindness(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionHealing(Item*& item, Entity* entity, Entity* usedBy, bool shouldConsumeItem = true);
bool item_PotionExtraHealing(Item*& item, Entity* entity, Entity* usedBy, bool shouldConsumeItem = true);
bool item_PotionRestoreMagic(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionInvisibility(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionLevitation(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionSpeed(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionStrength(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionAcid(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionUnstableStorm(Item*& item, Entity* entity, Entity* usedBy, Entity* thrownPotion);
bool item_PotionParalysis(Item*& item, Entity* entity, Entity* usedBy);
Entity* item_PotionPolymorph(Item*& item, Entity* entity, Entity* usedBy);
void item_ScrollMail(Item* item, int player);
void item_ScrollIdentify(Item* item, int player);
void item_ScrollLight(Item* item, int player);
void item_ScrollBlank(Item* item, int player);
void item_ScrollEnchantWeapon(Item* item, int player);
void item_ScrollEnchantArmor(Item* item, int player);
void item_ScrollRemoveCurse(Item* item, int player);
bool item_ScrollFire(Item* item, int player); // return true if exploded into fire.
void item_ScrollFood(Item* item, int player);
void item_ScrollConjureArrow(Item* item, int player);
void item_ScrollMagicMapping(Item* item, int player);
void item_ScrollRepair(Item* item, int player);
void item_ScrollDestroyArmor(Item* item, int player);
void item_ScrollTeleportation(Item* item, int player);
void item_ScrollSummon(Item* item, int player);
void item_AmuletSexChange(Item* item, int player);
void item_ToolTowel(Item*& item, int player);
void item_ToolTinOpener(Item* item, int player);
void item_ToolMirror(Item*& item, int player);
Entity* item_ToolBeartrap(Item*& item, Entity* usedBy);
void item_Food(Item*& item, int player);
void item_FoodTin(Item*& item, int player);
void item_FoodAutomaton(Item*& item, int player);
void item_Gem(Item* item, int player);
void item_Spellbook(Item*& item, int player);
void item_ToolLootBag(Item*& item, int player);

//General functions.
Item* newItem(ItemType type, Status status, Sint16 beatitude, Sint16 count, Uint32 appearance, bool identified, list_t* inventory);
Item* uidToItem(Uint32 uid);
ItemType itemLevelCurveEntity(Entity& my, Category cat, int minLevel, int maxLevel, BaronyRNG& rng);
ItemType itemLevelCurve(Category cat, int minLevel, int maxLevel, BaronyRNG& rng);
Item* newItemFromEntity(const Entity* entity, bool discardUid = false); //Make sure to call free(item). discardUid will free the new items uid if this is for temp purposes
Entity* dropItemMonster(Item* item, Entity* monster, Stat* monsterStats, Sint16 count = 1);
Item** itemSlot(Stat* myStats, Item* item);

enum Category itemCategory(const Item* item);
Sint32 itemModel(const Item* item, bool shortModel = false);
Sint32 itemModelFirstperson(const Item* item);
SDL_Surface* itemSprite(Item* item);
void consumeItem(Item*& item, int player); //NOTE: Items have to be unequipped before calling this function on them. NOTE: THIS CAN FREE THE ITEM POINTER. Sets item to nullptr if it does.
bool dropItem(Item* item, int player, const bool notifyMessage = true, const bool dropAll = false); // return true on free'd item
bool playerGreasyDropItem(const int player, Item* const item);
void useItem(Item* item, int player, Entity* usedBy = nullptr, bool unequipForDropping = false);
enum EquipItemResult : int
{
	EQUIP_ITEM_FAIL_CANT_UNEQUIP,
	EQUIP_ITEM_SUCCESS_NEWITEM,
	EQUIP_ITEM_SUCCESS_UPDATE_QTY,
	EQUIP_ITEM_SUCCESS_UNEQUIP
};
enum EquipItemSendToServerSlot : int
{
	EQUIP_ITEM_SLOT_WEAPON,
	EQUIP_ITEM_SLOT_SHIELD,
	EQUIP_ITEM_SLOT_MASK,
	EQUIP_ITEM_SLOT_HELM,
	EQUIP_ITEM_SLOT_GLOVES,
	EQUIP_ITEM_SLOT_BOOTS,
	EQUIP_ITEM_SLOT_BREASTPLATE,
	EQUIP_ITEM_SLOT_CLOAK,
	EQUIP_ITEM_SLOT_AMULET,
	EQUIP_ITEM_SLOT_RING
};
void playerTryEquipItemAndUpdateServer(const int player, Item* item, bool checkInventorySpaceForPaperDoll);
void clientSendEquipUpdateToServer(EquipItemSendToServerSlot slot, EquipItemResult equipType, int player,
	ItemType type, Status status, Sint16 beatitude, int count, Uint32 appearance, bool identified);
void clientUnequipSlotAndUpdateServer(const int player, EquipItemSendToServerSlot slot, Item* item);
void clientSendAppearanceUpdateToServer(const int player, Item* item, const bool onIdentify);
EquipItemResult equipItem(Item* item, Item** slot, int player, bool checkInventorySpaceForPaperDoll);
enum ItemStackResults : int
{
	ITEM_STACKING_ERROR,
	ITEM_DESTINATION_NOT_SAME_ITEM,
	ITEM_DESTINATION_STACK_IS_FULL,
	ITEM_ADDED_ENTIRELY_TO_DESTINATION_STACK,
	ITEM_ADDED_PARTIALLY_TO_DESTINATION_STACK,
	ITEM_ADDED_WITHOUT_NEEDING_STACK
};
struct ItemStackResult
{
	ItemStackResults resultType = ITEM_STACKING_ERROR;
	Item* itemToStackInto = nullptr;
};
// checks inventory order for stacking items (the first item in the list that is stackable will be returned)
ItemStackResult getItemStackingBehavior(const int player, Item* itemToCheck, Item* itemDestinationStack, int& newQtyForCheckedItem, int& newQtyForDestItem);
// checks chest inventory order for dropping all items into (the first item in the list that is stackable will be returned)
ItemStackResult getItemStackingBehaviorIntoChest(const int player, Item* itemToCheck, Item* itemDestinationStack, int& newQtyForCheckedItem, int& newQtyForDestItem);
void getItemEmptySlotStackingBehavior(const int player, Item& itemToCheck, int& newQtyForCheckedItem, int& newQtyForDestItem);
Item* itemPickup(int player, Item* item, Item* addToSpecificInventoryItem = nullptr, bool forceNewStack = false);
bool itemIsEquipped(const Item* item, int player);
bool shouldInvertEquipmentBeatitude(const Stat* wielder);
bool isItemEquippableInShieldSlot(const Item* item);
bool itemIsConsumableByAutomaton(const Item& item);

extern const real_t potionDamageSkillMultipliers[6];
extern const real_t thrownDamageSkillMultipliers[6];
extern Uint32 enchantedFeatherScrollSeed;
extern std::vector<int> enchantedFeatherScrollsShuffled;
static const std::vector<int> enchantedFeatherScrollsFixedList =
{
	SCROLL_BLANK,
	SCROLL_MAIL,
	SCROLL_DESTROYARMOR,
	SCROLL_DESTROYARMOR,
	SCROLL_DESTROYARMOR,
	SCROLL_FIRE,
	SCROLL_FIRE,
	SCROLL_FIRE,
	SCROLL_LIGHT,
	SCROLL_LIGHT,
	SCROLL_SUMMON,
	SCROLL_SUMMON,
	SCROLL_IDENTIFY,
	SCROLL_IDENTIFY,
	SCROLL_REMOVECURSE,
	SCROLL_CONJUREARROW,
	SCROLL_FOOD,
	SCROLL_FOOD,
	SCROLL_TELEPORTATION,
	SCROLL_TELEPORTATION,
	SCROLL_CHARGING,
	SCROLL_REPAIR,
	SCROLL_MAGICMAPPING,
	SCROLL_ENCHANTWEAPON,
	SCROLL_ENCHANTARMOR
};
static const int ENCHANTED_FEATHER_MAX_DURABILITY = 101;
static const int QUIVER_MAX_AMMO_QTY = 51;
static const int SCRAP_MAX_STACK_QTY = 101;
static const int THROWN_GEM_MAX_STACK_QTY = 9;

//-----ITEM COMPARISON FUNCS-----
/*
 * Only compares items of the same type.
 */
int itemCompare(const Item* item1, const Item* item2, bool checkAppearance, bool comparisonUsedForStacking = true);

/*
 * Returns true if potion is harmful to the player.
 */
bool isPotionBad(const Item& potion);
bool isRangedWeapon(const Item& item);
bool isRangedWeapon(const ItemType type);
bool isMeleeWeapon(const Item& item);
bool itemIsThrowableTinkerTool(const Item* item);

void createCustomInventory(Stat* stats, int itemLimit, BaronyRNG& rng);
void copyItem(Item* itemToSet, const Item* itemToCopy);
bool swapMonsterWeaponWithInventoryItem(Entity* my, Stat* myStats, node_t* inventoryNode, bool moveStack, bool overrideCursed);
bool monsterUnequipSlot(Stat* myStats, Item** slot, Item* itemToUnequip);
bool monsterUnequipSlotFromCategory(Stat* myStats, Item** slot, Category cat);
node_t* itemNodeInInventory(const Stat* myStats, Sint32 itemToFind, Category cat);
node_t* spellbookNodeInInventory(const Stat* myStats, int spellIDToFind);
node_t* getRangedWeaponItemNodeInInventory(const Stat* myStats, bool includeMagicstaff);
node_t* getMeleeWeaponItemNodeInInventory(const Stat* myStats);
ItemType itemTypeWithinGoldValue(int cat, int minValue, int maxValue, BaronyRNG& rng);
bool itemSpriteIsQuiverThirdPersonModel(int sprite);
bool itemSpriteIsQuiverBaseThirdPersonModel(int sprite);
bool itemTypeIsQuiver(ItemType type);
real_t rangedAttackGetSpeedModifier(const Stat* myStats);
bool rangedWeaponUseQuiverOnAttack(const Stat* myStats);
real_t getArtifactWeaponEffectChance(ItemType type, Stat& wielder, real_t* effectAmount);
void updateHungerMessages(Entity* my, Stat* myStats, Item* eaten);
bool playerCanSpawnMoreTinkeringBots(const Stat* myStats);
int maximumTinkeringBotsCanBeDeployed(const Stat* myStats);
extern bool overrideTinkeringLimit;
extern int decoyBoxRange;

// unique monster item appearance to avoid being dropped on death.
static const int MONSTER_ITEM_UNDROPPABLE_APPEARANCE = 1234567890;
static const int ITEM_TINKERING_APPEARANCE = 987654320;
static const int ITEM_GENERATED_QUIVER_APPEARANCE = 1122334455;
