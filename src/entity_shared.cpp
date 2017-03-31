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


char* spriteEditorName(Sint32 sprite)
{
	char tmpStr[32] = "";
	switch ( sprite )
	{
	case 71:
		strcpy(tmpStr, "DEVIL");
		break;
	case 70:
		strcpy(tmpStr, "GNOME");
		break;
	case 62:
		strcpy(tmpStr, "LICH");
		break;
	case 48:
		strcpy(tmpStr, "SPIDER");
		break;
	case 36:
		strcpy(tmpStr, "GOBLIN");
		break;
	case 35:
		strcpy(tmpStr, "SHOPKEEPER");
		break;
	case 30:
		strcpy(tmpStr, "TROLL");
		break;
	case 27:
		strcpy(tmpStr, "HUMAN");
		break;
	case 10:
		strcpy(tmpStr, "RANDOM (Dependent on Level)");
		break;
	case 83:
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
		break;
	case 95:
		strcpy(tmpStr, "SKELETON");
		break;
	case 96:
		strcpy(tmpStr, "RAT");
		break;
		//monsters
		break;
	case 21:
		strcpy(tmpStr, "CHEST");
		break;
	case 63:
		strcpy(tmpStr, "END PORTAL");
		break;
	case 64:
		strcpy(tmpStr, "SPEAR TRAP");
		break;
	case 65:
		strcpy(tmpStr, "MAGIC TRAP");
		break;
	case 66:
		strcpy(tmpStr, "WALL BUSTER");
		break;
	case 67:
		strcpy(tmpStr, "WALL BUILDER");
		break;
	case 68:
		strcpy(tmpStr, "MAGIC BOW");
		break;
	case 69:
		strcpy(tmpStr, "MAGIC SPEAR");
		break;
	case 59:
		strcpy(tmpStr, "TABLE");
		break;
	case 60:
		strcpy(tmpStr, "CHAIR");
		break;
	case 45:
		strcpy(tmpStr, "PORTAL");
		break;
	case 44:
		strcpy(tmpStr, "BOULDER");
		break;
	case 38:
		strcpy(tmpStr, "BOULDER TRAP");
		break;
	case 39:
		strcpy(tmpStr, "HEADSTONE");
		break;
	case 8:
		strcpy(tmpStr, "ITEM");
		break;
	case 9:
		strcpy(tmpStr, "GOLD");
		break;
	case 11:
		strcpy(tmpStr, "LADDER");
		break;
	case 12:
		strcpy(tmpStr, "FIREPLACE");
		break;
	case 14:
		strcpy(tmpStr, "FOUNTAIN");
		break;
	case 15:
		strcpy(tmpStr, "SINK");
		break;
	case 19:
		strcpy(tmpStr, "GATE (North-South)");
		break;
	case 20:
		strcpy(tmpStr, "GATE (East-West)");
		break;
	case 2:
		strcpy(tmpStr, "DOOR (East-West)");
		break;
	case 3:
		strcpy(tmpStr, "DOOR (North-South)");
		break;
	case 4:
		strcpy(tmpStr, "TORCH (West Wall)");
		break;
	case 5:
		strcpy(tmpStr, "TORCH (North Wall)");
		break;
	case 6:
		strcpy(tmpStr, "TORCH (East Wall)");
		break;
	case 7:
		strcpy(tmpStr, "TORCH (South Wall)");
		break;
	case 1:
		strcpy(tmpStr, "PLAYER START");
		break;
	case 33:
		strcpy(tmpStr, "PRESSURE PLATE");
		break;
	case 34:
		strcpy(tmpStr, "PRESSURE PLATE (Latch On)");
		break;
	case 37:
		strcpy(tmpStr, "MINOTAUR SPAWN TRAP");
		break;
	case 41:
		strcpy(tmpStr, "LAVA");
		break;
	case 43:
		strcpy(tmpStr, "LADDER HOLE");
		break;
	case 46:
		strcpy(tmpStr, "SECRET LADDER");
		break;
	case 72:
		strcpy(tmpStr, "DEVIL TELEPORT LOCATION");
		break;
	case 73:
		strcpy(tmpStr, "DEVIL TELEPORT LOCATION");
		break;
	case 74:
		strcpy(tmpStr, "DEVIL TELEPORT LOCATION");
		break;
	default:
		break;
	}

	return tmpStr;
}

char itemNameStrings[170][32] =
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
	"artifact_bow"
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
		"tool_torch",
		"tool_lantern",
		""
	},
	{
		"NULL",
		"random_item",
		"leather_breastpiece",
		"iron_breastpiece",
		"steel_breastpiece",
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
		""
	}
	
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
				//monsters that don't wear equipment
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

				//monsters with cloak/weapon/shield/boots
				case GNOME:
					equipType = 2;
					break;

				//monsters with cloak/weapon/shield/boots/helm/armor
				case GOBLIN:
				case HUMAN:
				case SKELETON:
				case SHOPKEEPER:
					equipType = 3;
					break;

				default:
					equipType = 0;
					break;
			}

			if ( category == 0 && equipType == 3 ) //HELM
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
			else if ( category >= 5 ) {
				return 1;
			}

			return 0;
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
		}
	}
}