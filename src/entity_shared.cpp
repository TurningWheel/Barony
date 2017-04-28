/*-------------------------------------------------------------------------------

BARONY
File: entity_shared.cpp
Desc: functions to be shared between editor.exe and barony.exe

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/
#pragma once

#include "entity.hpp"


int checkSpriteType(Sint32 sprite)
{
	switch ( sprite )
	{
	case 71:
	case 70:
	case 62:
	case 48:
	case 36:
	case 35:
	case 30:
	case 27:
	case 10:
	case 83:
	case 84:
	case 85:
	case 86:
	case 87:
	case 88:
	case 89:
	case 90:
	case 91:
	case 92:
	case 93:
	case 94:
	case 95:
	case 96:
	case 75:
	case 76:
	case 77:
	// to test case 37
	case 37:
	case 78:
	case 79:
	case 80:
	case 81:
	case 82:
	
		//monsters
		return 1;
		break;
	case 21:
		//chest
		return 2;
		break;
	case 8:
		//items
		return 3;
		break;
	default:
		return 0;
		break;
	}

	return 0;
}

char itemNameStrings[190][32] =
{
	"NULL",
	"random_item",
	"wooden_shield",
	"quarterstaff",
	"bronze_sword",
	"bronze_mace",
	"bronze_axe",
	"bronze_shield",
	"sling",
	"iron_spear",
	"iron_sword",
	"iron_mace",
	"iron_axe",
	"iron_shield",
	"shortbow",
	"steel_halberd",
	"steel_sword",
	"steel_mace",
	"steel_axe",
	"steel_shield",
	"steel_shield_resistance",
	"crossbow",
	"gloves",
	"gloves_dexterity",
	"bracers",
	"bracers_constitution",
	"gauntlets",
	"gauntlets_strength",
	"cloak",
	"cloak_magicreflection",
	"cloak_invisibility",
	"cloak_protection",
	"leather_boots",
	"leather_boots_speed",
	"iron_boots",
	"iron_boots_waterwalking",
	"steel_boots",
	"steel_boots_levitation",
	"steel_boots_feather",
	"leather_breastpiece",
	"iron_breastpiece",
	"steel_breastpiece",
	"hat_phrygian",
	"hat_hood",
	"hat_wizard",
	"hat_jester",
	"leather_helm",
	"iron_helm",
	"steel_helm",
	"amulet_sexchange",
	"amulet_lifesaving",
	"amulet_waterbreathing",
	"amulet_magicreflection",
	"amulet_strangulation",
	"amulet_poisonresistance",
	"potion_water",
	"potion_booze",
	"potion_juice",
	"potion_sickness",
	"potion_confusion",
	"potion_extrahealing",
	"potion_healing",
	"potion_cureailment",
	"potion_blindness",
	"potion_restoremagic",
	"potion_invisibility",
	"potion_levitation",
	"potion_speed",
	"potion_acid",
	"potion_paralysis",
	"scroll_mail",
	"scroll_identify",
	"scroll_light",
	"scroll_blank",
	"scroll_enchantweapon",
	"scroll_enchantarmor",
	"scroll_removecurse",
	"scroll_fire",
	"scroll_food",
	"scroll_magicmapping",
	"scroll_repair",
	"scroll_destroyarmor",
	"scroll_teleportation",
	"scroll_summon",
	"magicstaff_light",
	"magicstaff_digging",
	"magicstaff_locking",
	"magicstaff_magicmissile",
	"magicstaff_opening",
	"magicstaff_slow",
	"magicstaff_cold",
	"magicstaff_fire",
	"magicstaff_lightning",
	"magicstaff_sleep",
	"ring_adornment",
	"ring_slowdigestion",
	"ring_protection",
	"ring_warning",
	"ring_strength",
	"ring_constitution",
	"ring_invisibility",
	"ring_magicresistance",
	"ring_conflict",
	"ring_levitation",
	"ring_regeneration",
	"ring_teleportation",
	"spellbook_forcebolt",
	"spellbook_magicmissile",
	"spellbook_cold",
	"spellbook_fireball",
	"spellbook_light",
	"spellbook_removecurse",
	"spellbook_lightning",
	"spellbook_identify",
	"spellbook_magicmapping",
	"spellbook_sleep",
	"spellbook_confuse",
	"spellbook_slow",
	"spellbook_opening",
	"spellbook_locking",
	"spellbook_levitation",
	"spellbook_invisibility",
	"spellbook_teleportation",
	"spellbook_healing",
	"spellbook_extrahealing",
	"spellbook_cureailment",
	"spellbook_dig",
	"gem_rock",
	"gem_luck",
	"gem_garnet",
	"gem_ruby",
	"gem_jacinth",
	"gem_amber",
	"gem_citrine",
	"gem_jade",
	"gem_emerald",
	"gem_sapphire",
	"gem_aquamarine",
	"gem_amethyst",
	"gem_fluorite",
	"gem_opal",
	"gem_diamond",
	"gem_jetstone",
	"gem_obsidian",
	"gem_glass",
	"tool_pickaxe",
	"tool_tinopener",
	"tool_mirror",
	"tool_lockpick",
	"tool_skeletonkey",
	"tool_torch",
	"tool_lantern",
	"tool_blindfold",
	"tool_towel",
	"tool_glasses",
	"tool_beartrap",
	"food_bread",
	"food_creampie",
	"food_cheese",
	"food_apple",
	"food_meat",
	"food_fish",
	"food_tin",
	"readable_book",
	"spell_item",
	"artifact_sword",
	"artifact_mace",
	"artifact_spear",
	"artifact_axe",
	"artifact_bow",
	"artifact_breastpiece",
	"artifact_helm",
	"artifact_boots",
	"artifact_cloak",
	"artifact_gloves",
	"crystal_breastpiece",
	"crystal_helm",
	"crystal_boots",
	"crystal_shield",
	"crystal_gloves",
	"vampire_doublet",
	"wizard_doublet",
	"healer_doublet",
	"mirror_shield",
	"brass_knuckles",
	"iron_knuckles",
	"spiked_gauntlets",
	"food_tomalley",
	"tool_crystalshard",
	""
};


char itemStringsByType[10][55][32] =
{
	{
		"NULL",
		"random_item",
		"hat_phrygian",
		"hat_hood",
		"hat_wizard",
		"hat_jester",
		"leather_helm",
		"iron_helm",
		"steel_helm",
		"crystal_helm",
		"artifact_helm"
		""
	},
	{
		"NULL",
		"random_item",
		"quarterstaff",
		"bronze_sword",
		"bronze_mace",
		"bronze_axe",
		"sling",
		"iron_spear",
		"iron_sword",
		"iron_mace",
		"iron_axe",
		"shortbow",
		"steel_halberd",
		"steel_sword",
		"steel_mace",
		"steel_axe",
		"crossbow",
		"magicstaff_light",
		"magicstaff_digging",
		"magicstaff_locking",
		"magicstaff_magicmissile",
		"magicstaff_opening",
		"magicstaff_slow",
		"magicstaff_cold",
		"magicstaff_fire",
		"magicstaff_lightning",
		"magicstaff_sleep",
		"spellbook_forcebolt",
		"spellbook_magicmissile",
		"spellbook_cold",
		"spellbook_fireball",
		"spellbook_light",
		"spellbook_removecurse",
		"spellbook_lightning",
		"spellbook_identify",
		"spellbook_magicmapping",
		"spellbook_sleep",
		"spellbook_confuse",
		"spellbook_slow",
		"spellbook_opening",
		"spellbook_locking",
		"spellbook_levitation",
		"spellbook_invisibility",
		"spellbook_teleportation",
		"spellbook_healing",
		"spellbook_extrahealing",
		"spellbook_cureailment",
		"spellbook_dig",
		"tool_pickaxe",
		"artifact_sword",
		"artifact_mace",
		"artifact_spear",
		"artifact_axe",
		"artifact_bow",
		""
	},
	{
		"NULL",
		"random_item",
		"wooden_shield",
		"bronze_shield",
		"iron_shield",
		"steel_shield",
		"steel_shield_resistance",
		"crystal_shield",
		"mirror_shield",
		"tool_torch",
		"tool_lantern",
		"tool_crystalshard",
		""
	},
	{
		"NULL",
		"random_item",
		"leather_breastpiece",
		"iron_breastpiece",
		"steel_breastpiece",
		"crystal_breastpiece",
		"artifact_breastpiece",
		"vampire_doublet",
		"wizard_doublet",
		"healer_doublet",
		""
	},
	{
		"NULL",
		"random_item",
		"leather_boots",
		"leather_boots_speed",
		"iron_boots",
		"iron_boots_waterwalking",
		"steel_boots",
		"steel_boots_levitation",
		"steel_boots_feather",
		"crystal_boots",
		"artifact_boots",
		""
	},
	{
		"NULL",
		"random_item",
		"ring_adornment",
		"ring_slowdigestion",
		"ring_protection",
		"ring_warning",
		"ring_strength",
		"ring_constitution",
		"ring_invisibility",
		"ring_magicresistance",
		"ring_conflict",
		"ring_levitation",
		"ring_regeneration",
		"ring_teleportation",
		""
	},
	{
		"NULL",
		"random_item",
		"amulet_sexchange",
		"amulet_lifesaving",
		"amulet_waterbreathing",
		"amulet_magicreflection",
		"amulet_strangulation",
		"amulet_poisonresistance",
		""
	},
	{
		"NULL",
		"random_item",
		"cloak",
		"cloak_magicreflection",
		"cloak_invisibility",
		"cloak_protection",
		"artifact_cloak",
		""
	},
	{
		"NULL",
		"random_item",
		"tool_blindfold",
		"tool_glasses",
		""
	},
	{
		"NULL",
		"random_item",
		"gloves",
		"gloves_dexterity",
		"bracers",
		"bracers_constitution",
		"gauntlets",
		"gauntlets_strength",
		"crystal_gloves",
		"artifact_gloves",
		"brass_knuckles",
		"iron_knuckles",
		"spiked_gauntlets"
		""
	}
	
};

char spriteEditorNameStrings[96][28] = 
{
	"NULL",	
	"PLAYER START",
	"DOOR (East-West)",
	"DOOR (North-South)",
	"TORCH (West Wall)",
	"TORCH (North Wall)",
	"TORCH (East Wall)",
	"TORCH (South Wall)",
	"ITEM",
	"GOLD",
	"RANDOM (Dependent on Level)",
	"LADDER",
	"FIREPLACE",
	"Flame Sprite (Not Used)",
	"FOUNTAIN",
	"SINK",
	"Flame Sprite (Not Used)",
	"Lever",
	"Wire",
	"GATE (North-South)",
	"GATE (East-West)",
	"CHEST",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"HUMAN",
	"NOT USED",
	"NOT USED",
	"TROLL",
	"NOT USED",
	"ARROW TRAP",
	"PRESSURE PLATE",
	"PRESSURE PLATE (Latch On)",
	"SHOPKEEPER",
	"GOBLIN",
	"MINOTAUR SPAWN TRAP",
	"BOULDER TRAP",
	"HEADSTONE",
	"NULL",
	"LAVA",
	"NOT USED",
	"LADDER HOLE",
	"BOULDER",
	"PORTAL",
	"SECRET LADDER",
	"NOT USED",
	"SPIDER",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"NOT USED",
	"TABLE",
	"CHAIR",
	"DIAMOND PICKAXE",
	"LICH",
	"END PORTAL",
	"SPEAR TRAP",
	"MAGIC TRAP",
	"WALL BUSTER",
	"WALL BUILDER",
	"MAGIC BOW",
	"MAGIC SPEAR",
	"GNOME",
	"DEVIL",
	"DEVIL TELEPORT LOCATION",
	"DEVIL TELEPORT LOCATION",
	"DEVIL TELEPORT LOCATION",
	"DEMON",
	"IMP",
	"MINOTAUR",
	"SCORPION",
	"SLIME",
	"SUCCUBUS",
	"RAT",
	"GHOUL",
	"KOBOLD",
	"SCARAB",
	"CRYSTALGOLEM",
	"INCUBUS",
	"VAMPIRE",
	"SHADOW",
	"COCKATRICE",
	"INSECTOID",
	"GOATMAN",
	"AUTOMATON",
	"LICH ICE",
	"LICH FIRE",
	"SKELETON",
};

int canWearEquip(Entity* entity, int category)
{
	Stat* stats;
	int equipType = 0;
	int type;
	if ( entity != NULL )
	{
		stats = entity->getStats();
		if ( stats != NULL )
		{
			type = stats->type;

			switch ( type )
			{
				//monsters that don't wear equipment (only rings/amulets)
				case DEVIL:
				case SPIDER:
				case TROLL:
				case RAT:
				case SLIME:
				case SUCCUBUS:
				case SCORPION:
				case MINOTAUR:
				case GHOUL:
					equipType = 0;
					break;

				//monsters with weapons only (incl. spellbooks)
				case LICH:
				case CREATURE_IMP:
				case DEMON:
					equipType = 1;
					break;

				//monsters with cloak/weapon/shield/boots/mask/gloves (no helm)
				case GNOME:
					equipType = 2;
					break;

				//monsters with cloak/weapon/shield/boots/helm/armor/mask/gloves
				case GOBLIN:
				case HUMAN:
				case VAMPIRE:
				case SKELETON:
				case SHOPKEEPER:
					equipType = 3;
					break;

				default:
					equipType = 0;
					break;
			}
		}
	}

	if ( category == 0 && equipType >= 3 ) //HELM
	{
		return 1;
	}
	else if ( category == 1 && equipType >= 1 ) //WEAPON
	{
		return 1;
	}
	else if ( category == 2 && equipType >= 2 ) //SHIELD
	{
		return 1;
	}
	else if ( category == 3 && equipType >= 3 ) //ARMOR
	{
		return 1;
	}
	else if ( category == 4 && equipType >= 2 ) //BOOTS
	{
		return 1;
	}
	else if ( category == 5 || category == 6 ) { //RINGS/AMULETS WORN BY ALL
		return 1;
	}
	else if ( (category >= 7 && category <= 9) && equipType >= 2 ) //CLOAK/MASK/GLOVES
	{
		return 1;
	}
	else
	{
		return 0;
	}

	// TODO
	/*case 83:
	strcpy(tmpStr, "KOBOLD");
	break;
	case 84:
	strcpy(tmpStr, "SCARAB");
	break;
	case 85:
	strcpy(tmpStr, "CRYSTALGOLEM");
	break;
	case 86:
	strcpy(tmpStr, "INCUBUS");
	break;
	case 87:
	strcpy(tmpStr, "VAMPIRE");
	break;
	case 88:
	strcpy(tmpStr, "SHADOW");
	break;
	case 89:
	strcpy(tmpStr, "COCKATRICE");
	break;
	case 90:
	strcpy(tmpStr, "INSECTOID");
	break;
	case 91:
	strcpy(tmpStr, "GOATMAN");
	break;
	case 92:
	strcpy(tmpStr, "AUTOMATON");
	break;
	case 93:
	strcpy(tmpStr, "LICH ICE");
	break;
	case 94:
	strcpy(tmpStr, "LICH FIRE");
	break;*/

	return 0;
}