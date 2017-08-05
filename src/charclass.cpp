/*-------------------------------------------------------------------------------

	BARONY
	File: charclass.cpp
	Desc: Character class definition code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "interface/interface.hpp"
#include "magic/magic.hpp"
#include "items.hpp"
#include "book.hpp"

/*-------------------------------------------------------------------------------

	initClass

	Sets up a character class for the given player

-------------------------------------------------------------------------------*/

void initClass(int player)
{
    // TODO: Dynamic class list loading/saving from/to file
    enum class classes_t
    {
        BARBARIAN,
        WARRIOR,
        HEALER,
        ROGUE,
        WANDERER,
        CLERIC,
        MERCHANT,
        WIZARD,
        ARCANIST,
        JOKER,
        ENUM_LEN
    };

	if ( player == clientnum )
	{
		// TODO: Dedicated gameStartStuff() function. Seriously.
		// (Same for deathStuff() and/or gameEndStuff().
		selected_inventory_slot_x = 0;
		selected_inventory_slot_y = 0;
		current_hotbar = 0;
	}

	// SEX MODIFIER
	// Female; else Male
	if ( stats[player]->sex )
	{
		stats[player]->DEX += 1;
	}
	else
	{
		stats[player]->STR += 1;
	}

    Item* item = nullptr;  // Pointer to created Item
    Item* item2 = nullptr; // Pointer to picked up Item?

	// CLASS LOADOUTS
    // TODO: Remove static_cast<Sint32>() in favor for changing client_classes[] to an array of classes_t for direct comparison
	// Barbarian
	if ( client_classes[player] == static_cast<Sint32>(classes_t::BARBARIAN) )
	{
		// Attributes
		stats[player]->STR += 1;
		stats[player]->CON += 1;
		stats[player]->DEX -= 1;
		stats[player]->INT -= 1;

		stats[player]->MAXHP += 10;
		stats[player]->HP += 10;
		stats[player]->MAXMP -= 10;
		stats[player]->MP -= 10;

		// Skills
		stats[player]->PROFICIENCIES[PRO_SWIMMING] = 25;
		stats[player]->PROFICIENCIES[PRO_SHIELD] = 25;
		stats[player]->PROFICIENCIES[PRO_AXE] = 50;
		stats[player]->PROFICIENCIES[PRO_MACE] = 50;

		// Iron Axe
		item = newItem(IRON_AXE, SERVICABLE, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Wooden Shield
		item = newItem(WOODEN_SHIELD, SERVICABLE, 0, 1, 1, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[2].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Leather Helm
		item = newItem(LEATHER_HELM, WORN, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( player == clientnum )
		{
			// Iron Mace
			item = newItem(IRON_MACE, SERVICABLE, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// Bread
			item = newItem(FOOD_BREAD, SERVICABLE, 0, 2, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Meat
			item = newItem(FOOD_MEAT, SERVICABLE, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Torch
			item = newItem(TOOL_TORCH, SERVICABLE, 0, 2, 0, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[3].item = item2->uid;
			free(item);

			// Booze
			item = newItem(POTION_BOOZE, SERVICABLE, 0, 1, 1, true, NULL);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	// Warrior
	else if ( client_classes[player] == static_cast<Sint32>(classes_t::WARRIOR) )
	{
		// Attributes
		stats[player]->STR += 1;
		stats[player]->DEX += 1;
		stats[player]->CON -= 3;
		stats[player]->INT -= 1;
		stats[player]->PER -= 1;
		stats[player]->CHR += 1;

		stats[player]->MAXMP -= 10;
		stats[player]->MP -= 10;

		// Skills
		stats[player]->PROFICIENCIES[PRO_LEADERSHIP] = 40;
		stats[player]->PROFICIENCIES[PRO_RANGED] = 25;
		stats[player]->PROFICIENCIES[PRO_SWORD] = 50;
		stats[player]->PROFICIENCIES[PRO_POLEARM] = 50;
		stats[player]->PROFICIENCIES[PRO_SHIELD] = 50;

		// Iron Spear
		item = newItem(IRON_SPEAR, SERVICABLE, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Bronze Shield
		item = newItem(BRONZE_SHIELD, SERVICABLE, 0, 1, 1, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[3].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Leather Helm
		item = newItem(LEATHER_HELM, WORN, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Iron Armor
		item = newItem(IRON_BREASTPIECE, SERVICABLE, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Boots
		item = newItem(LEATHER_BOOTS, WORN, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( player == clientnum )
		{
			// Iron Sword
			item = newItem(IRON_SWORD, SERVICABLE, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// Shortbow
			item = newItem(SHORTBOW, EXCELLENT, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			// Bread
			item = newItem(FOOD_BREAD, SERVICABLE, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Fish
			item = newItem(FOOD_FISH, SERVICABLE, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	// Healer
	else if ( client_classes[player] == static_cast<Sint32>(classes_t::HEALER) )
	{
		// Attributes
		stats[player]->CON += 2;
		stats[player]->INT += 1;
		stats[player]->STR -= 1;
		stats[player]->DEX -= 1;

		stats[player]->MAXHP -= 10;
		stats[player]->HP -= 10;
		stats[player]->MAXMP += 10;
		stats[player]->MP += 10;

		// Skills
		stats[player]->PROFICIENCIES[PRO_SPELLCASTING] = 50;
		stats[player]->PROFICIENCIES[PRO_MAGIC] = 25;
		stats[player]->PROFICIENCIES[PRO_SWIMMING] = 25;
		stats[player]->PROFICIENCIES[PRO_POLEARM] = 25;

		// Quarterstaff
		item = newItem(QUARTERSTAFF, SERVICABLE, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Phrygian Hat
		item = newItem(HAT_PHRYGIAN, SERVICABLE, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Cloak (Red, Protection)
		item = newItem(CLOAK, SERVICABLE, 0, 1, 2, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( player == clientnum )
		{
			// Fish
			item = newItem(FOOD_FISH, EXCELLENT, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Extra Healing Potions
			item = newItem(POTION_EXTRAHEALING, EXCELLENT, 0, 3, 6, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			// Cure Ailment Spellbook
			item = newItem(SPELLBOOK_CUREAILMENT, EXCELLENT, 0, 1, 1, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Healing Spellbook
			item = newItem(SPELLBOOK_HEALING, EXCELLENT, 0, 1, 2, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Apples
			item = newItem(FOOD_APPLE, SERVICABLE, 0, 2, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Towels
			item = newItem(TOOL_TOWEL, SERVICABLE, 0, 3, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Magicstaff of Slow
			item = newItem(MAGICSTAFF_SLOW, SERVICABLE, 0, 1, 3, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);
		}
	}
	// Rogue
	else if ( client_classes[player] == static_cast<Sint32>(classes_t::ROGUE) )
	{
		// Attributes
		stats[player]->DEX += 2;
		stats[player]->PER += 2;
		stats[player]->INT -= 1;
		stats[player]->STR -= 1;
		stats[player]->CHR -= 1;

		stats[player]->MAXHP -= 10;
		stats[player]->HP -= 10;
		stats[player]->MAXMP -= 10;
		stats[player]->MP -= 10;

		// Skills
		stats[player]->PROFICIENCIES[PRO_APPRAISAL] = 25;
		stats[player]->PROFICIENCIES[PRO_STEALTH] = 50;
		stats[player]->PROFICIENCIES[PRO_LOCKPICKING] = 50;
		stats[player]->PROFICIENCIES[PRO_RANGED] = 25;
		stats[player]->PROFICIENCIES[PRO_SWORD] = 25;

		// Bronze Sword
		item = newItem(BRONZE_SWORD, SERVICABLE, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Cloak (Green)
		item = newItem(CLOAK, WORN, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Hood (Green)
		item = newItem(HAT_HOOD, WORN, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Gloves
		item = newItem(GLOVES, SERVICABLE, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Leather Breastpiece
		item = newItem(LEATHER_BREASTPIECE, DECREPIT, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( player == clientnum )
		{
			// Shortbow
			item = newItem(SHORTBOW, SERVICABLE, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// Sickness Potion
			item = newItem(POTION_SICKNESS, EXCELLENT, 0, 3, 5, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Lockpicks
			item = newItem(TOOL_LOCKPICK, SERVICABLE, 0, 3, 0, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);
		}
	}
	// Wanderer
	else if ( client_classes[player] == static_cast<Sint32>(classes_t::WANDERER) )
	{
		// Attributes
		stats[player]->CON += 1;
		stats[player]->INT -= 1;
		stats[player]->CHR -= 1;

		stats[player]->MAXHP += 10;
		stats[player]->HP += 10;
		stats[player]->MAXMP -= 10;
		stats[player]->MP -= 10;

		// Skills
		stats[player]->PROFICIENCIES[PRO_STEALTH] = 25;
		stats[player]->PROFICIENCIES[PRO_SWIMMING] = 50;
		stats[player]->PROFICIENCIES[PRO_POLEARM] = 25;
		stats[player]->PROFICIENCIES[PRO_RANGED] = 25;
		stats[player]->PROFICIENCIES[PRO_TRADING] = 25;

		// Quarterstaff
		item = newItem(QUARTERSTAFF, WORN, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Phrygian Hat
		item = newItem(HAT_PHRYGIAN, SERVICABLE, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Cloak
		item = newItem(CLOAK, SERVICABLE, 0, 1, 1, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Boots
		item = newItem(LEATHER_BOOTS, SERVICABLE, 1, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( player == clientnum )
		{
			// Crossbow
			item = newItem(CROSSBOW, WORN, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// Tins
			item = newItem(FOOD_TIN, SERVICABLE, 0, 2, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Bread
			item = newItem(FOOD_BREAD, SERVICABLE, 0, 2, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Fish
			item = newItem(FOOD_FISH, EXCELLENT, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Tin opener
			item = newItem(TOOL_TINOPENER, SERVICABLE, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Towel
			item = newItem(TOOL_TOWEL, SERVICABLE, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Torches
			item = newItem(TOOL_TORCH, SERVICABLE, 0, 2, 0, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			// Lantern
			item = newItem(TOOL_LANTERN, WORN, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[3].item = item2->uid;
			free(item);
		}
	}
	// Cleric
	else if ( client_classes[player] == static_cast<Sint32>(classes_t::CLERIC) )
	{
		// Attributes
		stats[player]->PER += 2;
		stats[player]->CON += 1;
		stats[player]->DEX -= 1;
		stats[player]->CHR -= 1;

		// Skills
		stats[player]->PROFICIENCIES[PRO_MACE] = 25;
		stats[player]->PROFICIENCIES[PRO_SWIMMING] = 25;
		stats[player]->PROFICIENCIES[PRO_MAGIC] = 25;
		stats[player]->PROFICIENCIES[PRO_SPELLCASTING] = 25;
		stats[player]->PROFICIENCIES[PRO_LEADERSHIP] = 20;

		// Iron Mace
		item = newItem(IRON_MACE, SERVICABLE, 1, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Wooden Shield
		item = newItem(WOODEN_SHIELD, WORN, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[1].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Leather Breastpiece
		item = newItem(LEATHER_BREASTPIECE, WORN, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Cloak (Purple)
		item = newItem(CLOAK, SERVICABLE, 0, 1, 3, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Gloves
		item = newItem(BRACERS, WORN, 1, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( player == clientnum )
		{
			// Torch
			item = newItem(TOOL_TORCH, SERVICABLE, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			// Healing Potions
			item = newItem(POTION_HEALING, EXCELLENT, 0, 2, 7, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[3].item = item2->uid;
			free(item);

			// Summon Scrolls
			item = newItem(SCROLL_SUMMON, EXCELLENT, 0, 3, 0, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[4].item = item2->uid;
			free(item);

			// Bread
			item = newItem(FOOD_BREAD, SERVICABLE, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	// Merchant
	else if ( client_classes[player] == static_cast<Sint32>(classes_t::MERCHANT) )
	{
		// Attributes
		stats[player]->CHR += 1;
		stats[player]->PER += 1;
		stats[player]->DEX -= 1;
		stats[player]->INT -= 1;
		stats[player]->GOLD += 500;

		stats[player]->MAXMP -= 10;
		stats[player]->MP -= 10;

		// Skills
		stats[player]->PROFICIENCIES[PRO_AXE] = 25;
		stats[player]->PROFICIENCIES[PRO_LEADERSHIP] = 20;
		stats[player]->PROFICIENCIES[PRO_APPRAISAL] = 50;
		stats[player]->PROFICIENCIES[PRO_TRADING] = 50;

		// Bronze Axe
		item = newItem(BRONZE_AXE, SERVICABLE, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Phrygian Hat
		item = newItem(HAT_PHRYGIAN, SERVICABLE, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Leather Breastpiece
		item = newItem(LEATHER_BREASTPIECE, WORN, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Glasses
		item = newItem(TOOL_GLASSES, SERVICABLE, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( player == clientnum )
		{
			// Pickaxe
			item = newItem(TOOL_PICKAXE, SERVICABLE, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// Gloves
			item = newItem(GLOVES, EXCELLENT, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);

			// Scroll of Remove Curse
			item = newItem(SCROLL_REMOVECURSE, EXCELLENT, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Cheese
			item = newItem(FOOD_CHEESE, SERVICABLE, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Meat
			item = newItem(FOOD_MEAT, EXCELLENT, 0, 2, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	// Wizard
	else if ( client_classes[player] == static_cast<Sint32>(classes_t::WIZARD) )
	{
		// Attributes
		stats[player]->INT += 1;
		stats[player]->PER += 1;
		stats[player]->DEX -= 1;
		stats[player]->CHR -= 1;

		stats[player]->MAXHP -= 10;
		stats[player]->HP -= 10;
		stats[player]->MAXMP += 20;
		stats[player]->MP += 20;

		// Skills
		stats[player]->PROFICIENCIES[PRO_POLEARM] = 25;
		stats[player]->PROFICIENCIES[PRO_SPELLCASTING] = 50;
		stats[player]->PROFICIENCIES[PRO_MAGIC] = 50;

		// Quarterstaff
		item = newItem(QUARTERSTAFF, SERVICABLE, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Wizard Hat
		item = newItem(HAT_WIZARD, SERVICABLE, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Cloak (Purple, Protection)
		item = newItem(CLOAK_PROTECTION, SERVICABLE, 0, 1, 3, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Amulet of Magic Reflection
		item = newItem(AMULET_MAGICREFLECTION, EXCELLENT, 0, 1, 1, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Leather Boots
		item = newItem(LEATHER_BOOTS, SERVICABLE, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( player == clientnum )
		{
			// Magicstaff of Light
			item = newItem(MAGICSTAFF_LIGHT, EXCELLENT, 0, 1, 3, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// Potion of Restore Magic
			item = newItem(POTION_RESTOREMAGIC, EXCELLENT, 0, 1, 1, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Spellbook of Fireball
			item = newItem(SPELLBOOK_FIREBALL, SERVICABLE, 0, 1, 3, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Spellbook of Cold
			item = newItem(SPELLBOOK_COLD, SERVICABLE, 0, 1, 4, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Spellbook of Light
			item = newItem(SPELLBOOK_LIGHT, SERVICABLE, 0, 1, 5, true, NULL);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	// Arcanist
	else if ( client_classes[player] == static_cast<Sint32>(classes_t::ARCANIST) )
	{
		// Attributes
		stats[player]->INT += 1;
		stats[player]->CHR += 1;
		stats[player]->PER -= 1;
		stats[player]->STR -= 1;

		stats[player]->MAXMP += 10;
		stats[player]->MP += 10;

		// Skills
		stats[player]->PROFICIENCIES[PRO_MAGIC] = 25;
		stats[player]->PROFICIENCIES[PRO_SPELLCASTING] = 50;
		stats[player]->PROFICIENCIES[PRO_STEALTH] = 25;
		stats[player]->PROFICIENCIES[PRO_LOCKPICKING] = 25;
		stats[player]->PROFICIENCIES[PRO_SWORD] = 25;
		stats[player]->PROFICIENCIES[PRO_RANGED] = 25;

		// Iron Sword
		item = newItem(IRON_SWORD, SERVICABLE, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Crossbow
		item = newItem(CROSSBOW, SERVICABLE, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[1].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Leather Armor
		item = newItem(LEATHER_BREASTPIECE, SERVICABLE, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Leather Boots
		item = newItem(LEATHER_BOOTS, SERVICABLE, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Cloak (Red)
		item = newItem(CLOAK, WORN, 0, 1, 2, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// Hood (Red)
		item = newItem(HAT_HOOD, WORN, 0, 1, 2, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( player == clientnum )
		{
			// Magicstaff of Opening
			item = newItem(MAGICSTAFF_OPENING, SERVICABLE, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			// Spellbook of Forcebolt
			item = newItem(SPELLBOOK_FORCEBOLT, WORN, 0, 1, 6, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Spellbook of Light
			item = newItem(SPELLBOOK_LIGHT, WORN, 0, 1, 7, true, NULL);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	// Joker
	else if ( client_classes[player] == static_cast<Sint32>(classes_t::JOKER) )
	{
		// Attributes
		stats[player]->INT += 1;
		stats[player]->CHR += 1;
		stats[player]->CON -= 1;
		stats[player]->STR -= 1;
		stats[player]->GOLD += 200;

		// Skills
		stats[player]->PROFICIENCIES[PRO_LOCKPICKING] = 25;
		stats[player]->PROFICIENCIES[PRO_TRADING] = 25;
		stats[player]->PROFICIENCIES[PRO_LEADERSHIP] = 20;
		stats[player]->PROFICIENCIES[PRO_MAGIC] = 25;
		stats[player]->PROFICIENCIES[PRO_SPELLCASTING] = 25;

		// Jester Hat
		item = newItem(HAT_JESTER, SERVICABLE, 0, 1, 0, true, NULL);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( player == clientnum )
		{
			// Slingshot
			item = newItem(SLING, SERVICABLE, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[0].item = item2->uid;
			free(item);

			// Lockpicks
			item = newItem(TOOL_LOCKPICK, EXCELLENT, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// Scroll of Teleportation
			item = newItem(SCROLL_TELEPORTATION, EXCELLENT, 0, 3, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Scroll of Food
			item = newItem(SCROLL_FOOD, EXCELLENT, 0, 1, 0, false, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Ring of Levitation
			item = newItem(RING_LEVITATION, SERVICABLE, 0, 1, 2, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Spellbook of Confuse
			item = newItem(SPELLBOOK_CONFUSE, WORN, 0, 1, 8, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Blindfold
			item = newItem(TOOL_BLINDFOLD, SERVICABLE, 0, 1, 2, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Luckstone
			item = newItem(GEM_LUCK, EXCELLENT, 0, 1, 1, false, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// Fish
			item = newItem(FOOD_FISH, SERVICABLE, 0, 2, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);
		}
	}

	// Move class starting items to the right
	if ( player == clientnum )
	{
		node_t* node;
		for ( node = stats[player]->inventory.first; node != nullptr; node = node->next )
		{
			Item* item = (Item*)node->element;
			item->x = INVENTORY_SIZEX - item->x - 1;
		}
	}

	//spellList = malloc(sizeof(list_t));
	spellList.first = nullptr;
	spellList.last = nullptr;
}
