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
#include "game.hpp"

class Entity; // forward declare
class Stat; // forward declare
struct spell_t; // forward declare

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
	MASK_MARIGOLD,
	KEY_STONE,
	KEY_BONE,
	KEY_BRONZE,
	KEY_IRON,
	KEY_SILVER,
	KEY_GOLD,
	KEY_CRYSTAL,
	KEY_MACHINE,
	TOOL_FOCI_FIRE,
	INSTRUMENT_FLUTE,
	INSTRUMENT_LYRE,
	INSTRUMENT_DRUM,
	INSTRUMENT_LUTE,
	INSTRUMENT_HORN,
	RAPIER,
	AMULET_BURNINGRESIST,
	GREASE_BALL,
	BRANCH_STAFF,
	BRANCH_BOW,
	BRANCH_BOW_INFECTED,
	DUST_BALL,
	BOLAS,
	STEEL_FLAIL,
	FOOD_RATION,
	FOOD_RATION_SPICY,
	FOOD_RATION_SOUR,
	FOOD_RATION_BITTER,
	FOOD_RATION_HEARTY,
	FOOD_RATION_HERBAL,
	FOOD_RATION_SWEET,
	SLOP_BALL,
	TOOL_FRYING_PAN,
	CLEAT_BOOTS,
	BANDIT_BREASTPIECE,
	TUNIC_BLOUSE,
	BONE_BREASTPIECE,
	BLACKIRON_BREASTPIECE,
	SILVER_BREASTPIECE,
	IRON_PAULDRONS,
	QUILTED_GAMBESON,
	ROBE_CULTIST,
	ROBE_HEALER,
	ROBE_MONK,
	ROBE_WIZARD,
	SHAWL,
	CHAIN_HAUBERK,
	BONE_BRACERS,
	BLACKIRON_GAUNTLETS,
	SILVER_GAUNTLETS,
	QUILTED_GLOVES,
	CHAIN_GLOVES,
	BONE_BOOTS,
	BLACKIRON_BOOTS,
	SILVER_BOOTS,
	QUILTED_BOOTS,
	LOAFERS,
	CHAIN_BOOTS,
	SCUTUM,
	BONE_SHIELD,
	BLACKIRON_SHIELD,
	SILVER_SHIELD,
	CLOAK_DENDRITE,
	BONE_HELM,
	BLACKIRON_HELM,
	SILVER_HELM,
	HAT_FELT,
	QUILTED_CAP,
	HOOD_TEAL,
	CHAIN_COIF,
	FOOD_SHROOM,
	FOOD_NUT,
	TOOL_FOCI_SNOW,
	TOOL_FOCI_NEEDLES,
	TOOL_FOCI_ARCS,
	TOOL_FOCI_SAND,
	TOOL_FOCI_DARK_LIFE,
	TOOL_FOCI_DARK_RIFT,
	TOOL_FOCI_DARK_SILENCE,
	TOOL_FOCI_DARK_VENGEANCE,
	TOOL_FOCI_DARK_SUPPRESS,
	TOOL_FOCI_LIGHT_PEACE,
	TOOL_FOCI_LIGHT_JUSTICE,
	TOOL_FOCI_LIGHT_PROVIDENCE,
	TOOL_FOCI_LIGHT_PURITY,
	TOOL_FOCI_LIGHT_SANCTUARY,
	MAGICSTAFF_SCEPTER,
	TOME_SORCERY,
	TOME_MYSTICISM,
	TOME_THAUMATURGY,
	HAT_CIRCLET_SORCERY,
	HAT_CIRCLET_THAUMATURGY,
	TOOL_DUCK,
	SHILLELAGH_MACE,
	CLAYMORE_SWORD,
	ANELACE_SWORD,
	LANCE_SPEAR,
	STEEL_FALSHION,
	STEEL_GREATAXE,
	BLACKIRON_AXE,
	BLACKIRON_CROSSBOW,
	BLACKIRON_DART,
	BLACKIRON_MACE,
	BLACKIRON_SWORD,
	BLACKIRON_TRIDENT,
	BONE_AXE,
	BONE_MACE,
	BONE_SHORTBOW,
	BONE_SPEAR,
	BONE_SWORD,
	BONE_THROWING,
	SILVER_AXE,
	SILVER_GLAIVE,
	SILVER_MACE,
	SILVER_PLUMBATA,
	SILVER_SWORD,
	QUIVER_BONE,
	QUIVER_BLACKIRON,
	GEM_JEWEL,
	SPELLBOOK_METEOR,
	SPELLBOOK_ICE_WAVE,
	SPELLBOOK_GUARD_BODY,
	SPELLBOOK_GUARD_SPIRIT,
	SPELLBOOK_DIVINE_GUARD,
	SPELLBOOK_PROF_NIMBLENESS,
	SPELLBOOK_PROF_GREATER_MIGHT,
	SPELLBOOK_PROF_COUNSEL,
	SPELLBOOK_PROF_STURDINESS,
	SPELLBOOK_BLESS_FOOD,
	SPELLBOOK_PINPOINT,
	SPELLBOOK_DONATION,
	SPELLBOOK_SCRY_ALLIES,
	SPELLBOOK_SCRY_TRAPS,
	SPELLBOOK_SCRY_TREASURES,
	SPELLBOOK_DETECT_ENEMY,
	SPELLBOOK_TURN_UNDEAD,
	SPELLBOOK_HEAL_OTHER,
	SPELLBOOK_BLOOD_WARD,
	SPELLBOOK_DIVINE_ZEAL,
	SPELLBOOK_MAXIMISE,
	SPELLBOOK_MINIMISE,
	SPELLBOOK_INCOHERENCE,
	SPELLBOOK_OVERCHARGE,
	SPELLBOOK_ENVENOM_WEAPON,
	SPELLBOOK_PSYCHIC_SPEAR,
	SPELLBOOK_DEFY_FLESH,
	SPELLBOOK_GREASE_SPRAY,
	SPELLBOOK_BLOOD_WAVES,
	SPELLBOOK_COMMAND,
	SPELLBOOK_METALLURGY,
	SPELLBOOK_FORGE_KEY,
	SPELLBOOK_RESHAPE_WEAPON,
	SPELLBOOK_ALTER_ARROW,
	SPELLBOOK_VOID_CHEST,
	SPELLBOOK_LEAD_BOLT,
	SPELLBOOK_NUMBING_BOLT,
	SPELLBOOK_CURSE_FLESH,
	SPELLBOOK_COWARDICE,
	SPELLBOOK_SEEK_ALLY,
	SPELLBOOK_DEEP_SHADE,
	SPELLBOOK_SPIRIT_WEAPON,
	SPELLBOOK_SPORES,
	SPELLBOOK_WINDGATE,
	SPELLBOOK_TELEKINESIS,
	SPELLBOOK_DISARM,
	SPELLBOOK_ABUNDANCE,
	SPELLBOOK_PRESERVE,
	SPELLBOOK_SABOTAGE,
	SPELLBOOK_MIST_FORM,
	SPELLBOOK_FORCE_SHIELD,
	SPELLBOOK_SPLINTER_GEAR,
	SPELLBOOK_ATTRACT_ITEMS,
	SPELLBOOK_ABSORB_MAGIC,
	SPELLBOOK_TUNNEL,
	SPELLBOOK_NULL_AREA,
	SPELLBOOK_FIRE_SPRITE,
	SPELLBOOK_SPIN,
	SPELLBOOK_CLEANSE_FOOD,
	SPELLBOOK_FLAME_CLOAK,
	SPELLBOOK_LIGHTNING_BOLT,
	SPELLBOOK_DISRUPT_EARTH,
	SPELLBOOK_FIRE_WALL,
	SPELLBOOK_SLAM,
	SPELLBOOK_IGNITE,
	SPELLBOOK_SHATTER_OBJECTS,
	SPELLBOOK_KINETIC_FIELD,
	SPELLBOOK_THORNS,
	SPELLBOOK_MAGICIANS_ARMOR,
	SPELLBOOK_HEAL_MINOR,
	SPELLBOOK_SIGIL,
	SPELLBOOK_SANCTUARY,
	SPELLBOOK_HOLY_BEAM,
	SPELLBOOK_DOMINATE,
		
	//mod addition:
	HARPOON,
	// mod add: firearms
	FLINTLOCK_PISTOL,
	MUSKET,
	// mod add end
	// mod add: Entrench spellbook. TODO: replace placeholder mod artwork.
	SPELLBOOK_ENTRENCH,
	// mod add: offhand scouting utility. TODO: dedicated model/icon in mod assets.
	SPYGLASS = 528,
	// mod add: one generic, instance-configured magic rune.
	MAGIC_RUNE,
	RUNE_HAMMER,
	AMULET_FAMILIAR = 531,
	TINCTURE_STR = 532,
	TINCTURE_DEX,
	TINCTURE_CON,
	TINCTURE_INT,
	TINCTURE_PER,
	TINCTURE_CHA,
	OIL_AETHER,
	OIL_SILVER,
	OIL_TOXINS,
	OIL_BASES,
	OIL_LEAD,
	OIL_MINERAL,
	OIL_BRINE,
	INK_MYSTICISM,
	INK_SORCERY,
	INK_THAUMATURGY,

	INGREDIENT_BAG_SLIME = 548,
	INGREDIENT_BAG_HUMORIN,
	INGREDIENT_BAG_WILDBLOOD,
	INGREDIENT_BAG_ANIMATIC,

	INGREDIENT_GELATIN,
	INGREDIENT_OOZE,
	INGREDIENT_GREASE,
	INGREDIENT_MOLD,

	INGREDIENT_BLOOD,
	INGREDIENT_PHLEGM,
	INGREDIENT_BLACK_BILE,
	INGREDIENT_YELLOW_BILE,

	INGREDIENT_EYE,
	INGREDIENT_HEART,
	INGREDIENT_TOOTH,
	INGREDIENT_LIVER,

	INGREDIENT_SPIRIT,
	INGREDIENT_CORE,
	INGREDIENT_DUST,
	INGREDIENT_FRAGMENTS,

	ITEM_ENUM_MAX
} ItemType;
const int NUMITEMS = ITEM_ENUM_MAX;

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
	SPELL_CAT,
	TOME_SPELL,
	CATEGORY_MAX
} Category;

typedef enum Status
{
	BROKEN,
	DECREPIT,
	WORN,
	SERVICABLE,
	EXCELLENT
} Status;

enum AppliedOil : Uint8
{
	APPLIED_OIL_NONE,
	APPLIED_OIL_AETHER,
	APPLIED_OIL_SILVER,
	APPLIED_OIL_TOXINS,
	APPLIED_OIL_BASES,
	APPLIED_OIL_LEAD,
	APPLIED_OIL_MINERAL,
	APPLIED_OIL_BRINE,
	APPLIED_OIL_MAX
};

bool itemTypeIsTincture(ItemType type);
bool itemTypeIsOil(ItemType type);
bool itemTypeIsInk(ItemType type);
bool itemTypeIsWitchbrewConsumable(ItemType type);

enum WitchIngredientCategory : Sint32
{
	INGREDIENT_CATEGORY_NONE = 0,
	INGREDIENT_CATEGORY_SLIME,
	INGREDIENT_CATEGORY_HUMORIN,
	INGREDIENT_CATEGORY_WILDBLOOD,
	INGREDIENT_CATEGORY_ANIMATIC
};

bool itemTypeIsIngredientBag(ItemType type);
bool itemTypeIsWitchIngredient(ItemType type);
bool itemTypeIsWitchbrewGeneratedItem(ItemType type);
WitchIngredientCategory ingredientCategoryFromBag(ItemType type);
WitchIngredientCategory ingredientCategoryFromItem(ItemType type);
ItemType ingredientBagForCategory(WitchIngredientCategory category);
ItemType ingredientForCategorySlot(WitchIngredientCategory category, int slot);
int ingredientSlotForItem(ItemType type);
WitchIngredientCategory getIngredientCategoryForMonster(Entity* monster);
AppliedOil appliedOilFromItemType(ItemType type);
const char* getOilIconPath(AppliedOil oil);
const char* getTinctureIconPath(ItemType type);
bool itemTypeCanReceiveOil(ItemType type);
bool itemCanReceiveOil(const Item* item);
bool oilMatchesTarget(AppliedOil oil, Entity* target);
void serverSyncAppliedOil(int player, bool shieldSlot);
void rollOilWearOff(Item* source, int ownerPlayer, bool shieldSlot);
void rollProjectileOilWearOff(Entity* projectile);
bool applyTinctureStackToPlayer(ItemType tincture, int player);
bool spellbookIsEligibleForInk(ItemType book);
ItemType getInkSpellbookResult(ItemType ink, ItemType sourceSpellbook);
bool useOilOnItem(Item*& oil, Item* target, int player);
bool useInkOnSpellbook(Item*& ink, Item* book, int player);

static constexpr int STEWING_MAX_EDUCTS = 8;
static constexpr int STEWING_RECIPE_COUNT = 20;
static constexpr int STEWING_RESOURCE_COUNT = 18; // 16 Witch ingredients, Water, Empty Bottle.
static constexpr Uint32 STEWING_GENERATOR_VERSION = 1;

struct StewingEduct
{
	ItemType type = ITEM_ENUM_MAX;
	Uint32 quantity = 0;
};

using StewingSelection = std::array<StewingEduct, STEWING_MAX_EDUCTS>;

struct StewingRecipe
{
	ItemType result = ITEM_ENUM_MAX;
	std::array<StewingEduct, STEWING_MAX_EDUCTS> educts{};
	Uint32 eductCount = 0;
	bool alwaysUnlocked = false;
};

enum StewingMatchType : Uint8
{
	STEWING_MATCH_EMPTY,
	STEWING_MATCH_TOO_SMALL,
	STEWING_MATCH_VALID_RECIPE,
	STEWING_MATCH_INVALID_POISON,
	STEWING_MATCH_INVALID_RESOURCE
};

struct StewingMatch
{
	StewingMatchType type = STEWING_MATCH_EMPTY;
	Sint32 recipeIndex = -1;
	Uint32 multiplier = 0;
	ItemType result = ITEM_ENUM_MAX;
};

// A distinctive appearance prevents Use-brewed oils/inks merging with ordinary bottles.
constexpr Uint32 STEWING_NO_BOTTLE_APPEARANCE = 0x57535455u;

enum StewingOperation : Uint8
{
	STEWING_OPERATION_BREW_INTO,
	STEWING_OPERATION_APPLY,
	STEWING_OPERATION_BREW_FOR_USE // Create a no-bottle oil/ink for the Use target picker.
};

struct StewingTransactionResult
{
	bool success = false;
	StewingSelection remainingSelection{};
	std::array<Uint32, STEWING_RESOURCE_COUNT> resources{};
	std::array<Uint32, STEWING_RESOURCE_COUNT> inventoryConsumed{};
	ItemType newlyUnlockedRecipe = ITEM_ENUM_MAX;
};

const std::array<StewingRecipe, STEWING_RECIPE_COUNT>& getStewingRecipes();
Uint32 getActiveStewingGeneratorVersion();
void setActiveStewingGeneratorVersion(Uint32 version);
Uint32 getStewingRecipeUnlockMask(int player);
void setStewingRecipeUnlockMask(int player, Uint32 mask);
bool playerKnowsStewingRecipe(int player, ItemType result);
bool unlockStewingRecipe(int player, ItemType result);
void unlockAllStewingRecipes(int player);
int getNumKnownStewingRecipes(int player);
ItemType tryDiscoverStewingRecipe(int player,
	const std::array<Uint64, STEWING_RESOURCE_COUNT>& consumedResources);
void tryIncreaseAlchemyFromWitchbrewUse(int player);
bool stewingResultCanApplyImmediately(ItemType result);
bool itemTypeIsStewingSelectable(ItemType type);
int getStewingResourceIndex(ItemType type);
ItemType getStewingResourceType(int index);
int getBottleContribution(ItemType educt);
Uint32 countSelectedStewingResource(const StewingSelection& selection, ItemType type);
Uint32 countSelectedWitchIngredients(const StewingSelection& selection);
StewingMatch evaluateStewingMixture(const StewingSelection& selection);
Uint32 getTotalStewingResource(int player, ItemType resource);
Uint32 getAvailableStewingIngredient(int player, ItemType ingredient,
	const StewingSelection& reserved);
std::array<Uint32, STEWING_RESOURCE_COUNT> getStewingResourceSnapshot(int player);
std::array<Uint32, STEWING_RESOURCE_COUNT> getStewingInventoryResourceSnapshot(int player);
void setServerStewingInventoryMirror(int player,
	const std::array<Uint32, STEWING_RESOURCE_COUNT>& resources);
void clearServerStewingInventoryMirror(int player);
bool serverStewingInventoryMirrorIsValid(int player);
void consumeClientStewingInventoryResources(int player,
	const std::array<Uint32, STEWING_RESOURCE_COUNT>& resources);
bool performStewingTransaction(int player, Entity* familiar, StewingOperation operation,
	const StewingSelection& selection, StewingTransactionResult& result);

// Compact indices stored in MAGIC_RUNE appearance metadata. Deliberately excludes
// rock, luckstone, worthless glass, and jewel.
enum RuneGemType : Uint8
{
	RUNE_GEM_OBSIDIAN,
	RUNE_GEM_JADE,
	RUNE_GEM_FLUORITE,
	RUNE_GEM_AMETHYST,
	RUNE_GEM_GARNET,
	RUNE_GEM_CITRINE,
	RUNE_GEM_OPAL,
	RUNE_GEM_JETSTONE,
	RUNE_GEM_AMBER,
	RUNE_GEM_AQUAMARINE,
	RUNE_GEM_JACINTH,
	RUNE_GEM_EMERALD,
	RUNE_GEM_SAPPHIRE,
	RUNE_GEM_RUBY,
	RUNE_GEM_DIAMOND,
	RUNE_GEM_COUNT,
	RUNE_GEM_INVALID = 0xff
};
RuneGemType runeGemTypeFromItemType(ItemType type);
ItemType itemTypeFromRuneGemType(RuneGemType gem);

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
	// MAGIC_RUNE stores its crafter's final bonus-form PWR as signed fixed point.
	// This deliberately lives outside appearance so the existing Rune durability,
	// resource-factor, and Dominate-capacity encodings retain their precision.
	static constexpr Sint32 RUNE_STORED_PWR_INVALID = (-2147483647 - 1);
	static constexpr real_t RUNE_STORED_PWR_SCALE = 1000000.0;
	static constexpr Sint8 RUNE_CREATOR_INVALID = -1;
	static constexpr Uint32 RUNE_INSTANCE_ID_INVALID = 0;
	static constexpr Uint32 RUNE_CREATOR_IDENTITY_INVALID = 0;

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
	bool spellNotifyIcon = false; // if spell can level you up
	Uint8 itemRequireTradingSkillInShop = 0; // if item hidden in shop view until player has trading req
	bool itemSpecialShopConsumable = false; // if item is extra non-standard inventory consumable
	Sint32 runeStoredPWR = RUNE_STORED_PWR_INVALID;
	Sint8 runeCreatorPlayer = RUNE_CREATOR_INVALID;
	// Network/save-stable identity of this physical Rune. Generic uid remains
	// peer-local and must never be used to identify a Rune across the network.
	Uint32 runeInstanceId = RUNE_INSTANCE_ID_INVALID;
	// Persistent character/session identity paired with runeCreatorPlayer.
	Uint32 runeCreatorIdentity = RUNE_CREATOR_IDENTITY_INVALID;
	AppliedOil appliedOil = APPLIED_OIL_NONE;
	Uint32 ingredientCounts[4] = { 0, 0, 0, 0 };
	// The physical Familiar Amulet owns its Familiar's durable progression.
	// The summoned monster is only a temporary manifestation of this state.
	bool familiarInitialized = false;
	Uint32 familiarInstanceId = 0;
	// Save/network-stable physical identity for readable Wraith lore objects.
	Uint32 wraithLoreInstanceId = 0;
	Sint32 familiarHP = 0;
	Sint32 familiarMAXHP = 0;
	Sint32 familiarMP = 0;
	Sint32 familiarMAXMP = 0;
	Sint32 familiarLVL = 0;
	Sint32 familiarEXP = 0;
	Sint32 familiarSTR = 0;
	Sint32 familiarDEX = 0;
	Sint32 familiarCON = 0;
	Sint32 familiarINT = 0;
	Sint32 familiarPER = 0;
	Sint32 familiarCHR = 0;

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
	bool isMagicRune() const;
	int runeGetSpellID() const;
	void runeSetSpellID(int spellID);
	RuneGemType runeGetGemType() const;
	void runeSetGemType(RuneGemType gem);
	int runeGetMagicSchool() const;
	bool runeIsValid() const;
	bool runeCanCast() const;
	ItemType runeGetVisualGemItemType() const;
	std::string runeGetDisplayName() const;
	real_t runeGetStoredBreakChance() const;
	bool runeSetStoredBreakChance(real_t chance);
	real_t runeGetStoredResourceFactor() const;
	bool runeSetStoredResourceFactor(real_t factor);
	int runeGetDominateCapacity() const;
	bool runeSetDominateCapacity(int capacity);
	bool runeHasStoredPWR() const;
	real_t runeGetStoredPWR() const;
	real_t runeGetStoredPWRDisplayPercent() const;
	Sint32 runeGetStoredPWRRaw() const;
	bool runeSetStoredPWR(real_t pwr);
	void runeSetStoredPWRRaw(Sint32 raw);
	bool runeHasCreator() const;
	int runeGetCreatorPlayer() const;
	void runeSetCreatorPlayer(int player);
	bool runeHasInstanceId() const;
	Uint32 runeGetInstanceId() const;
	void runeSetInstanceId(Uint32 instanceId);
	Uint32 runeEnsureInstanceId();
	Uint32 runeGetCreatorIdentity() const;
	void runeSetCreatorIdentity(Uint32 identity);
	bool runeCreatorMatchesPlayer(int player) const;
	Uint32 familiarEnsureInstanceId();
	Uint32 wraithLoreEnsureInstanceId();

	//General Functions.
	Sint32 weaponGetAttack(const Stat* wielder = nullptr) const; //Returns the tohit of the weapon.
	Sint32 armorGetAC(const Stat* wielder = nullptr) const;
	bool canUnequip(const Stat* wielder = nullptr); //Returns true if the item can be unequipped (not cursed), false if it can't (cursed).
	int buyValue(int player) const;
	int sellValue(int player) const;
	bool usableWhileShapeshifted(const Stat* wielder = nullptr) const;
	char* getScrollLabel() const;
	const char* getTomeLabel() const;
	int getTomeSpellID() const;

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
	bool doesPotionHarmAlliesOnThrown() const;

	Sint32 potionGetEffectHealth(Entity* my, Stat* myStats) const;
	Sint32 potionGetEffectDamage(Entity* my, Stat* myStats) const;
	Sint32 potionGetEffectDurationMinimum(Entity* my, Stat* myStats) const;
	Sint32 potionGetEffectDurationMaximum(Entity* my, Stat* myStats) const;
	Sint32 potionGetEffectDurationRandom(Entity* my, Stat* myStats) const;
	Sint32 potionGetCursedEffectDurationMinimum(Entity* my, Stat* myStats) const;
	Sint32 potionGetCursedEffectDurationMaximum(Entity* my, Stat* myStats) const;
	Sint32 potionGetCursedEffectDurationRandom(Entity* my, Stat* myStats) const;
	static int getBaseFoodSatiation(ItemType type);

	Sint32 getWeight() const;
	Sint32 getGoldValue() const;

	// mod add: Firearm loaded state uses a reserved
	// appearance bit so normal item serialization/networking preserves it.
	bool isFirearm() const;
	bool isUnbreakableFromUse() const;
	static bool firearmAppearanceIsLoaded(ItemType type, Uint32 appearance);
	static Uint32 setFirearmAppearanceLoaded(ItemType type, Uint32 appearance, bool loaded);
	bool firearmIsLoaded() const;
	void setFirearmLoaded(bool loaded);
	// mod add: The Library Musket's persistent lock is separate from an ordinary jam.
	static bool musketAppearanceIsQuestJammed(ItemType type, Uint32 appearance);
	static Uint32 setMusketAppearanceQuestJammed(ItemType type, Uint32 appearance, bool jammed);
	bool musketQuestJammed() const;
	void setMusketQuestJammed(bool jammed);
	ItemType firearmReloadMaterialType() const;
	Sint32 firearmReloadMaterialCost(Sint32 rawTinkering) const;
	Sint32 firearmReloadDuration(Sint32 rawTinkering) const;
	Sint32 firearmJamChancePercent(Sint32 rawTinkering) const;
	Sint32 firearmJamRecoveryScrapCost(Sint32 rawTinkering) const;
	Sint32 firearmReloadTrainingCap() const;
	// mod add end

	void foodTinGetDescriptionIndices(int* a, int* b, int* c) const;
	void foodTinGetDescription(std::string& cookingMethod, std::string& protein, std::string& sides) const;
	int foodGetPukeChance(Stat* eater) const;
	int getLootBagPlayer() const;
	int getLootBagNumItems() const;
	int getDuckPlayer() const;

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
	void applyDuck(Uint32 parentUid, real_t x, real_t y, Entity* hitentity, bool onLevelRespawn);
	bool unableToEquipDueToSwapWeaponTimer(const int player) const;
	bool tinkeringBotIsMaxHealth() const;
	bool isTinkeringItemWithThrownLimit() const;
	static void onItemIdentified(int player, Item* tempItem);
	static void itemFindUniqueAppearance(Item* tempItem, std::unordered_set<Uint32>& appearancesOfSimilarItems);
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
	int gold_value;                  // value per item
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
bool item_PotionGrease(Item*& item, Entity* entity, Entity* usedBy);
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
void item_ScrollMail(Item*& item, int player);
void item_ScrollIdentify(Item*& item, int player);
void item_ScrollLight(Item*& item, int player, bool suppressScrollNarration = false);
void item_ScrollBlank(Item*& item, int player);
void item_ScrollEnchantWeapon(Item*& item, int player);
void item_ScrollEnchantArmor(Item*& item, int player);
void item_ScrollRemoveCurse(Item*& item, int player);
bool item_ScrollFire(Item*& item, int player, bool suppressScrollNarration = false); // return true if exploded into fire.
void item_ScrollFood(Item*& item, int player);
void item_ScrollConjureArrow(Item*& item, int player);
void item_ScrollMagicMapping(Item*& item, int player);
void item_ScrollRepair(Item*& item, int player);
void item_ScrollDestroyArmor(Item*& item, int player);
void item_ScrollTeleportation(Item*& item, int player);
void item_ScrollSummon(Item*& item, int player);
void item_AmuletSexChange(Item*& item, int player);
void item_ToolTowel(Item*& item, int player);
void item_ToolTinOpener(Item*& item, int player);
void item_ToolMirror(Item*& item, int player);
Entity* item_ToolBeartrap(Item*& item, Entity* usedBy);
void item_Food(Item*& item, int player);
void item_FoodTin(Item*& item, int player);
void item_FoodAutomaton(Item*& item, int player);
static constexpr Sint32 GOLEM_CHARGE_MAX = 2000;
static constexpr Sint32 GOLEM_POLARITY_SWITCH_POINT = 5000;
void item_GolemConsume(Item*& item, int player);
void item_Spellbook(Item*& item, int player);
void degradeSpellbookFromLearning(Item*& spellbook, int player);
void item_ToolLootBag(Item*& item, int player);

//General functions.
Item* newItem(ItemType type, Status status, Sint16 beatitude, Sint16 count, Uint32 appearance, bool identified, list_t* inventory);
void copyItemPersistentCustomState(Item& destination, const Item& source);
void copyItemPersistentCustomStateToEntity(Entity& destination, const Item& source);
void copyItemPersistentCustomStateFromEntity(Item& destination, const Entity& source);
bool mergeIngredientBags(Item& destination, const Item& source);
bool ingredientBagIsEmpty(const Item& bag);
Item* findIngredientBagInInventory(list_t& inventory, WitchIngredientCategory category);
Item* mergeIngredientBagIntoInventory(list_t& inventory, const Item& source);
void writeIngredientCountsToPacket(const Item& item, Uint8* data, int offset);
void readIngredientCountsFromPacket(Item& item, const Uint8* data, int packetLength, int offset);
static constexpr int FAMILIAR_PERSISTENT_PACKET_BYTES = 14 * static_cast<int>(sizeof(Uint32));
void writeFamiliarPersistentStateToPacket(const Item& item, Uint8* data, int offset);
void readFamiliarPersistentStateFromPacket(Item& item, const Uint8* data, int packetLength, int offset);
Item* newItemFromExistingItem(const Item& source, Sint16 count, list_t* inventory);
Item* createMagicRune(RuneGemType gem, int spellID, Status status, Sint16 beatitude,
	bool identified, list_t* inventory = nullptr);
int getRuneInscriptionManaCost(Status gemstoneStatus, const spell_t* spell, Entity* caster);
int getRuneInscriptionGoldCost(Status gemstoneStatus, spell_t* spell, int player);
bool activateMagicRune(int player);
bool consumeSustainedSpellResource(Entity* caster, spell_t* spell, int manaCost);
bool runeSpellIsSustained(spell_t* spell);
real_t getRuneDominateStorageEfficiency(const Item& rune);
real_t getRuneCrafterCompetence(const spell_t* spell, Entity* creator);
bool initializeMagicRuneCraftingProfile(Item& rune, spell_t* spell, Entity* creator,
	const Item& sourceSpellbook);
real_t getMagicRuneEffectivePWR(const Item& rune, const Entity* caster);
bool applyMagicRuneEffectivePWR(spell_t& spell, const Item& rune, const Entity* caster);
bool applyMagicRuneCastStress(Uint32 runeInstanceId, int resourceEquivalent,
	int requiredOwner = -1);
Item* findMagicRuneByInstanceId(Uint32 runeInstanceId, int* ownerOut = nullptr,
	int requiredOwner = -1);
bool breakMagicRune(Uint32 runeInstanceId, const char* message = nullptr);
void reserveMagicRuneInstanceId(Uint32 runeInstanceId);
void reserveFamiliarInstanceId(Uint32 familiarInstanceId);
void reserveWraithLoreInstanceId(Uint32 instanceId);
static constexpr int WRAITH_LORE_INSTANCE_PACKET_BYTES = 4;
void writeWraithLoreInstanceIdToPacket(const Item& item, Uint8* data, int offset);
void readWraithLoreInstanceIdFromPacket(Item& item, const Uint8* data,
	int packetLength, int offset);
Uint32 ensureRuneCreatorIdentity(int player);
void reserveRuneCreatorIdentity(Uint32 creatorIdentity);
Item* uidToItem(Uint32 uid);
ItemType itemLevelCurveEntity(Entity& my, Category cat, int minLevel, int maxLevel, BaronyRNG& rng);
bool itemLevelCurvePostProcess(Entity* my, Item* item, BaronyRNG& rng, 
#ifdef EDITOR
	int itemLevel = 0
#else
	int itemLevel = currentlevel
#endif
	, int* lastItemType = nullptr, int* lastItemSpellType = nullptr
);
ItemType itemLevelCurve(Category cat, int minLevel, int maxLevel, BaronyRNG& rng);
Item* newItemFromEntity(const Entity* entity, bool discardUid = false); //Make sure to call free(item). discardUid will free the new items uid if this is for temp purposes
Entity* dropItemMonster(Item* item, Entity* monster, Stat* monsterStats, Sint16 count = 1);
bool inscribeFloorGemWithRuneHammer(int player, Entity* floorItem, int charge);
bool beginRuneHammerInscription(int player, Uint32 targetUid, int charge);
void updateRuneHammerInscription(int player);
bool runeHammerUsesTwoHandedPose(const Stat* stats);
bool runeHammerHasAttachedOffhand(const Stat* stats);
namespace RuneHammerInscriptionAnimation
{
	constexpr int IMPACT_TICK = 12;
	constexpr int END_TICK = 24;
}
Item** itemSlot(Stat* myStats, Item* item);

enum Category itemCategory(const Item* item);
Sint32 itemModel(const Item* item, bool shortModel = false, Entity* creature = nullptr);
Sint32 itemModelFirstperson(const Item* item);
bool updateGolemPolarityFromComposition(int player);
void reverseGolemEquipmentBeatitudes(Stat& playerStats, const Item* excludedItem = nullptr);
void consumeItem(Item*& item, int player); //NOTE: Items have to be unequipped before calling this function on them. NOTE: THIS CAN FREE THE ITEM POINTER. Sets item to nullptr if it does.
bool dropItem(Item* item, int player, const bool notifyMessage = true, const bool dropAll = false); // return true on free'd item
bool playerGreasyDropItem(const int player, Item* const item);
bool playerThrowDuck(const int player, Item* const item, int charge);
void useItem(Item* item, int player, Entity* usedBy = nullptr, bool unequipForDropping = false,
	bool serverCheckUse = false, bool alternateUse = false);
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
	ItemType type, Status status, Sint16 beatitude, int count, Uint32 appearance, bool identified,
	Sint32 runeStoredPWR = Item::RUNE_STORED_PWR_INVALID,
	Sint8 runeCreatorPlayer = Item::RUNE_CREATOR_INVALID,
	Uint32 runeInstanceId = Item::RUNE_INSTANCE_ID_INVALID,
	Uint32 runeCreatorIdentity = Item::RUNE_CREATOR_IDENTITY_INVALID,
	AppliedOil appliedOil = APPLIED_OIL_NONE);
void clientUnequipSlotAndUpdateServer(const int player, EquipItemSendToServerSlot slot, Item* item);
void clientSendAppearanceUpdateToServer(const int player, Item* item, const bool onIdentify);
void clientSendItemTypeUpdateToServer(const int player, Item* item, ItemType prevItemType);
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
Item* itemPickupCraftingResult(int player, Item* item);
bool itemIsEquipped(const Item* item, int player);
bool shouldInvertEquipmentBeatitude(const Stat* wielder);
bool isItemEquippableInShieldSlot(const Item* item);
bool itemIsConsumableByAutomaton(const Item& item);
bool itemIsConsumableByGolem(const Item& item);
int getGolemChargeValue(const Item& item);

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
static const int MAGICSTAFF_SCEPTER_CHARGE_MAX = 101;
static const int TOME_APPEARANCE_MAX = 1024;

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
node_t* itemNodeInInventory(const Stat* myStats, Sint32 itemToFind, Category cat, bool randomSlot = false);
node_t* spellbookNodeInInventory(const Stat* myStats, int spellIDToFind);
node_t* getRangedWeaponItemNodeInInventory(const Stat* myStats, bool includeMagicstaff);
node_t* getMeleeWeaponItemNodeInInventory(const Stat* myStats);
ItemType itemTypeWithinGoldValue(int cat, int minValue, int maxValue, BaronyRNG& rng);
bool itemSpriteIsQuiverThirdPersonModel(int sprite);
bool itemSpriteIsQuiverBaseThirdPersonModel(int sprite);
bool itemSpriteIsFociThirdPersonModel(const int sprite);
bool itemTypeIsQuiver(ItemType type);
bool itemTypeIsFoci(ItemType type);
bool itemTypeIsInstrument(ItemType type);
bool itemTypeIsThrownBall(ItemType type);
real_t rangedAttackGetSpeedModifier(const Stat* myStats);
bool rangedWeaponUseQuiverOnAttack(const Stat* myStats);
bool tryReloadFirearm(Item& firearm, int player); // mod add: firearm scrap reload
void updateFirearmReload(int player); // mod add: timed firearm reload completion
void receiveFirearmReloadResult(int player, ItemType type, Uint32 appearance,
	bool success, Uint32 token); // authoritative remote reload completion
void receiveFirearmReloadStart(int player, ItemType type, Uint32 token);
void receiveFirearmReloadReady(int player, ItemType type, Uint32 token);
void receiveFirearmReloadPayment(int player, ItemType type, Uint32 token, bool success);
void cancelFirearmReload(int player, bool endSession = false);
bool tryJamFirearm(Item& firearm, int player); // mod add: authoritative firearm jam roll
bool tryQuestJamMusket(Item& firearm, int player); // mod add: persistent quest-jam feedback
bool firearmUnjamIsActive(int player); // mod add: block firearm actions during clearing
void playFirearmJamSound(Entity* wielder); // mod add: native Tinkering disassembly feedback
void receiveFirearmJam(int player, ItemType type, Sint32 duration,
	bool retainsLoadedShot, Uint32 token); // server correction/presentation only
void receiveFirearmJamReady(int player, ItemType type, Sint32 duration, Sint32 cost, Uint32 token);
void receiveFirearmJamPayment(int player, ItemType type, Uint32 token, bool paid);
void cancelFirearmJamPayment(int player, bool endSession = false);
void updateFirearmUnjam(int player); // mod add: timed firearm jam clearing
bool unlockLibraryMusketFromGnomeDeath(); // mod add: authoritative one-time quest unlock
void receiveLibraryMusketUnlock(); // mod add: mirror the unlock in local client inventory
Sint32 spoilLoadedFirearmsInInventory(int player, bool giveFeedback); // mod add: water spoils loaded shots
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

enum SpellbookColors
{
	SPELLBOOK_COLOR_THAUM_2,		//"items/images/SpellbookYellow.png",
	SPELLBOOK_COLOR_THAUM_3,		//"items/images/SpellbookWhite.png",
	SPELLBOOK_COLOR_THAUM_1,		//"items/images/SpellbookBlack.png",
	SPELLBOOK_COLOR_MYSTICISM_2,	//"items/images/SpellbookRed.png",
	SPELLBOOK_COLOR_SORCERY_1,		//"items/images/SpellbookBrown.png",
	SPELLBOOK_COLOR_SORCERY_3,		//"items/images/SpellbookOrange.png",
	SPELLBOOK_COLOR_MYSTICISM_1,	//"items/images/SpellbookGreen.png",
	SPELLBOOK_COLOR_SORCERY_2,		//"items/images/SpellbookBlue.png",
	SPELLBOOK_COLOR_MYSTICISM_3		//"items/images/SpellbookPurple.png"
};

int getItemVariationFromSpellbookOrTome(const Item& item);

#ifdef EDITOR
SDL_Surface* itemSprite(Item* const item);
#endif
