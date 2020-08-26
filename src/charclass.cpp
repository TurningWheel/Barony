/*-------------------------------------------------------------------------------

	BARONY
	File: charclass.cpp
	Desc: character class definition code

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
#include "net.hpp"
#include "charclass.hpp"

/*-------------------------------------------------------------------------------

	initClass

	Sets up a character class for the given player

-------------------------------------------------------------------------------*/

void initClass(const int player)
{
	Item* item = nullptr;
	Item* item2 = nullptr;
	if ( player == clientnum)
	{
		//TODO: Dedicated gameStartStuff() function. Seriously.
		//(same for deathStuff() and/or gameEndStuff().
		selected_inventory_slot_x = 0;
		selected_inventory_slot_y = 0;
		current_hotbar = 0;

		for ( Uint32 i = 0; i < NUM_HOTBAR_SLOTS; ++i )
		{
			hotbar[i].item = 0;
		}
		magicBoomerangHotbarSlot = -1;
	}

	bool curseItems = false;
	if ( (stats[player]->playerRace == RACE_SUCCUBUS || stats[player]->playerRace == RACE_INCUBUS)
		&& stats[player]->appearance == 0 )
	{
		curseItems = true;
	}

	// SEX MODIFIER
	// female; else male
	if ( stats[player]->sex )
	{
		stats[player]->DEX += 1;
	}
	else
	{
		stats[player]->STR += 1;
	}

	stats[player]->type = HUMAN;

	//spellList = malloc(sizeof(list_t));
	spellList.first = nullptr;
	spellList.last = nullptr;


	// CLASS LOADOUTS
	// barbarian
	if ( client_classes[player] == CLASS_BARBARIAN )
	{
		// attributes
		stats[player]->STR += 1;
		stats[player]->CON += 1;
		stats[player]->DEX -= 1;
		stats[player]->INT -= 1;

		stats[player]->MAXHP += 10;
		stats[player]->HP += 10;
		stats[player]->MAXMP -= 10;
		stats[player]->MP -= 10;

		// skills
		stats[player]->PROFICIENCIES[PRO_SWIMMING] = 25;
		stats[player]->PROFICIENCIES[PRO_SHIELD] = 25;
		stats[player]->PROFICIENCIES[PRO_AXE] = 50;
		stats[player]->PROFICIENCIES[PRO_MACE] = 50;
		stats[player]->PROFICIENCIES[PRO_UNARMED] = 20;
		stats[player]->PROFICIENCIES[PRO_ALCHEMY] = 10;

		// iron axe
		item = newItem(IRON_AXE, SERVICABLE, 0, 1, 0, true, nullptr);
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

		// wooden shield
		item = newItem(WOODEN_SHIELD, SERVICABLE, 0, 1, 1, true, nullptr);
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

		// leather helm
		item = newItem(LEATHER_HELM, WORN, 0, 1, 0, true, nullptr);
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
			// iron mace
			item = newItem(IRON_MACE, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// bread
			item = newItem(FOOD_BREAD, SERVICABLE, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// meat
			item = newItem(FOOD_MEAT, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// torch
			item = newItem(TOOL_TORCH, SERVICABLE, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[3].item = item2->uid;
			free(item);

			// booze
			item = newItem(POTION_BOOZE, SERVICABLE, 0, 1, 2, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// tomahawk
			item = newItem(BRONZE_TOMAHAWK, DECREPIT, 0, 2, 1, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[4].item = item2->uid;
			free(item);
		}
	}
	// warrior
	else if ( client_classes[player] == CLASS_WARRIOR )
	{
		// attributes
		stats[player]->STR += 1;
		stats[player]->DEX += 1;
		stats[player]->CON -= 3;
		stats[player]->INT -= 1;
		stats[player]->PER -= 1;
		stats[player]->CHR += 1;

		stats[player]->MAXMP -= 10;
		stats[player]->MP -= 10;

		// skills
		stats[player]->PROFICIENCIES[PRO_LEADERSHIP] = 40;
		stats[player]->PROFICIENCIES[PRO_RANGED] = 25;
		stats[player]->PROFICIENCIES[PRO_SWORD] = 50;
		stats[player]->PROFICIENCIES[PRO_POLEARM] = 50;
		stats[player]->PROFICIENCIES[PRO_SHIELD] = 50;
		stats[player]->PROFICIENCIES[PRO_UNARMED] = 10;

		// iron spear
		item = newItem(IRON_SPEAR, SERVICABLE, 0, 1, 0, true, nullptr);
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

		// bronze shield
		item = newItem(BRONZE_SHIELD, SERVICABLE, 0, 1, 1, true, nullptr);
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

		// leather helm
		item = newItem(LEATHER_HELM, WORN, 0, 1, 0, true, nullptr);
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

		// iron armor
		item = newItem(IRON_BREASTPIECE, SERVICABLE, 0, 1, 0, true, nullptr);
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

		// boots
		item = newItem(LEATHER_BOOTS, WORN, 0, 1, 0, true, nullptr);
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
			// iron sword
			item = newItem(IRON_SWORD, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// shortbow
			item = newItem(SHORTBOW, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			// bread
			item = newItem(FOOD_BREAD, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// fish
			item = newItem(FOOD_FISH, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	// healer
	else if ( client_classes[player] == CLASS_HEALER )
	{
		// attributes
		stats[player]->CON += 2;
		stats[player]->INT += 1;
		stats[player]->STR -= 1;
		stats[player]->DEX -= 1;

		stats[player]->MAXHP -= 10;
		stats[player]->HP -= 10;
		stats[player]->MAXMP += 10;
		stats[player]->MP += 10;

		// skills
		stats[player]->PROFICIENCIES[PRO_SPELLCASTING] = 50;
		stats[player]->PROFICIENCIES[PRO_MAGIC] = 25;
		stats[player]->PROFICIENCIES[PRO_SWIMMING] = 25;
		stats[player]->PROFICIENCIES[PRO_POLEARM] = 25;
		stats[player]->PROFICIENCIES[PRO_ALCHEMY] = 30;

		// quarterstaff
		item = newItem(QUARTERSTAFF, SERVICABLE, 0, 1, 0, true, nullptr);
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

		// phrygian hat
		item = newItem(HAT_PHRYGIAN, SERVICABLE, 0, 1, 0, true, nullptr);
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

		// healer doublet
		item = newItem(HEALER_DOUBLET, SERVICABLE, 0, 1, 2, true, nullptr);
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
		
		// cloak (red, protection)
		item = newItem(CLOAK, SERVICABLE, 0, 1, 2, true, nullptr);
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
			// fish
			item = newItem(FOOD_FISH, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// extra healing potions
			item = newItem(POTION_EXTRAHEALING, EXCELLENT, 0, 3, 6, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			// cure ailment spellbook
			item = newItem(SPELLBOOK_CUREAILMENT, EXCELLENT, 0, 1, 1, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[8].item = item2->uid;
			free(item);

			// healing spellbook
			item = newItem(SPELLBOOK_HEALING, EXCELLENT, 0, 1, 2, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[9].item = item2->uid;
			free(item);

			// apples
			item = newItem(FOOD_APPLE, SERVICABLE, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// towels
			item = newItem(TOOL_TOWEL, SERVICABLE, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// magicstaff of slow
			item = newItem(MAGICSTAFF_SLOW, SERVICABLE, 0, 1, 3, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);
		}
	}
	// rogue
	else if ( client_classes[player] == CLASS_ROGUE )
	{
		// attributes
		stats[player]->DEX += 2;
		stats[player]->PER += 2;
		stats[player]->INT -= 1;
		stats[player]->STR -= 1;
		stats[player]->CHR -= 1;

		stats[player]->MAXHP -= 10;
		stats[player]->HP -= 10;
		stats[player]->MAXMP -= 10;
		stats[player]->MP -= 10;

		// skills
		stats[player]->PROFICIENCIES[PRO_APPRAISAL] = 25;
		stats[player]->PROFICIENCIES[PRO_STEALTH] = 50;
		stats[player]->PROFICIENCIES[PRO_LOCKPICKING] = 40;
		stats[player]->PROFICIENCIES[PRO_RANGED] = 25;
		stats[player]->PROFICIENCIES[PRO_SWORD] = 25;
		stats[player]->PROFICIENCIES[PRO_ALCHEMY] = 20;

		// bronze sword
		item = newItem(BRONZE_SWORD, SERVICABLE, 0, 1, 0, true, nullptr);
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

		// cloak (green)
		item = newItem(CLOAK, WORN, 0, 1, 0, true, nullptr);
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

		// hood (green)
		item = newItem(HAT_HOOD, WORN, 0, 1, 0, true, nullptr);
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

		// gloves
		item = newItem(GLOVES, SERVICABLE, 0, 1, 0, true, nullptr);
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

		// leather breastpiece
		item = newItem(LEATHER_BREASTPIECE, DECREPIT, 0, 1, 0, true, nullptr);
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
			// shortbow
			item = newItem(SHORTBOW, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// sickness potion
			item = newItem(POTION_SICKNESS, EXCELLENT, 0, 3, 5, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// lockpicks
			item = newItem(TOOL_LOCKPICK, SERVICABLE, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);
		}
	}
	// wanderer
	else if ( client_classes[player] == CLASS_WANDERER )
	{
		// attributes
		stats[player]->CON += 1;
		stats[player]->INT -= 1;
		stats[player]->CHR -= 1;

		stats[player]->MAXHP += 10;
		stats[player]->HP += 10;
		stats[player]->MAXMP -= 10;
		stats[player]->MP -= 10;

		// skills
		stats[player]->PROFICIENCIES[PRO_STEALTH] = 25;
		stats[player]->PROFICIENCIES[PRO_SWIMMING] = 50;
		stats[player]->PROFICIENCIES[PRO_POLEARM] = 25;
		stats[player]->PROFICIENCIES[PRO_RANGED] = 25;
		stats[player]->PROFICIENCIES[PRO_TRADING] = 25;
		stats[player]->PROFICIENCIES[PRO_UNARMED] = 10;
		stats[player]->PROFICIENCIES[PRO_ALCHEMY] = 30;

		// quarterstaff
		item = newItem(QUARTERSTAFF, WORN, 0, 1, 0, true, nullptr);
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

		// brown hood
		item = newItem(HAT_HOOD, SERVICABLE, 0, 1, 1, true, nullptr);
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

		// cloak
		item = newItem(CLOAK, SERVICABLE, 0, 1, 1, true, nullptr);
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

		// boots
		item = newItem(LEATHER_BOOTS, SERVICABLE, curseItems ? -1 : 1, 1, 0, true, nullptr);
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
			// crossbow
			item = newItem(CROSSBOW, WORN, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// tins
			item = newItem(FOOD_TIN, SERVICABLE, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// bread
			item = newItem(FOOD_BREAD, SERVICABLE, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// fish
			item = newItem(FOOD_FISH, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// tin opener
			item = newItem(TOOL_TINOPENER, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// towel
			item = newItem(TOOL_TOWEL, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// torches
			item = newItem(TOOL_TORCH, SERVICABLE, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			// lantern
			item = newItem(TOOL_LANTERN, WORN, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[3].item = item2->uid;
			free(item);
		}
	}
	// cleric
	else if ( client_classes[player] == CLASS_CLERIC )
	{
		// attributes
		stats[player]->PER += 2;
		stats[player]->CON += 1;
		stats[player]->DEX -= 1;
		stats[player]->CHR -= 1;

		// skills
		stats[player]->PROFICIENCIES[PRO_MACE] = 25;
		stats[player]->PROFICIENCIES[PRO_SWIMMING] = 25;
		stats[player]->PROFICIENCIES[PRO_MAGIC] = 25;
		stats[player]->PROFICIENCIES[PRO_SPELLCASTING] = 25;
		stats[player]->PROFICIENCIES[PRO_LEADERSHIP] = 20;
		stats[player]->PROFICIENCIES[PRO_ALCHEMY] = 20;

		// iron mace
		item = newItem(IRON_MACE, SERVICABLE, curseItems ? -1 : 1, 1, 0, true, nullptr);
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

		// wooden shield
		item = newItem(WOODEN_SHIELD, WORN, 0, 1, 0, true, nullptr);
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

		// leather breastpiece
		item = newItem(LEATHER_BREASTPIECE, WORN, 0, 1, 0, true, nullptr);
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

		// cloak (purple)
		item = newItem(CLOAK, SERVICABLE, 0, 1, 3, true, nullptr);
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

		// gloves
		item = newItem(BRACERS, WORN, curseItems ? -1 : 1, 1, 0, true, nullptr);
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
			// torch
			item = newItem(TOOL_TORCH, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			// healing potions
			item = newItem(POTION_HEALING, EXCELLENT, 0, 2, 7, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[3].item = item2->uid;
			free(item);

			// summon scrolls
			item = newItem(SCROLL_SUMMON, EXCELLENT, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[4].item = item2->uid;
			free(item);

			// bread
			item = newItem(FOOD_BREAD, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	// merchant
	else if ( client_classes[player] == CLASS_MERCHANT )
	{
		// attributes
		stats[player]->CHR += 1;
		stats[player]->PER += 1;
		stats[player]->DEX -= 1;
		stats[player]->INT -= 1;
		stats[player]->GOLD += 500;

		stats[player]->MAXMP -= 10;
		stats[player]->MP -= 10;

		// skills
		stats[player]->PROFICIENCIES[PRO_AXE] = 25;
		stats[player]->PROFICIENCIES[PRO_LEADERSHIP] = 20;
		stats[player]->PROFICIENCIES[PRO_APPRAISAL] = 50;
		stats[player]->PROFICIENCIES[PRO_TRADING] = 50;
		stats[player]->PROFICIENCIES[PRO_ALCHEMY] = 10;

		// bronze axe
		item = newItem(BRONZE_AXE, SERVICABLE, 0, 1, 0, true, nullptr);
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

		// phrygian hat
		item = newItem(HAT_PHRYGIAN, SERVICABLE, 0, 1, 0, true, nullptr);
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

		// leather breastpiece
		item = newItem(LEATHER_BREASTPIECE, WORN, 0, 1, 0, true, nullptr);
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

		// glasses
		item = newItem(TOOL_GLASSES, SERVICABLE, 0, 1, 0, true, nullptr);
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
			// pickaxe
			item = newItem(TOOL_PICKAXE, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// gloves
			item = newItem(GLOVES, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);

			// scroll of remove curse
			item = newItem(SCROLL_REMOVECURSE, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// cheese
			item = newItem(FOOD_CHEESE, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// meat
			item = newItem(FOOD_MEAT, EXCELLENT, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	// wizard
	else if ( client_classes[player] == CLASS_WIZARD )
	{
		// attributes
		stats[player]->INT += 1;
		stats[player]->PER += 1;
		stats[player]->DEX -= 1;
		stats[player]->CHR -= 1;

		stats[player]->MAXHP -= 10;
		stats[player]->HP -= 10;
		stats[player]->MAXMP += 20;
		stats[player]->MP += 20;

		// skills
		stats[player]->PROFICIENCIES[PRO_POLEARM] = 25;
		stats[player]->PROFICIENCIES[PRO_SPELLCASTING] = 50;
		stats[player]->PROFICIENCIES[PRO_MAGIC] = 50;
		stats[player]->PROFICIENCIES[PRO_ALCHEMY] = 10;

		// quarterstaff
		item = newItem(QUARTERSTAFF, SERVICABLE, 0, 1, 0, true, nullptr);
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

		// wizard hat
		item = newItem(HAT_WIZARD, SERVICABLE, 0, 1, 0, true, nullptr);
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

		// wizard doublet
		item = newItem(WIZARD_DOUBLET, SERVICABLE, 0, 1, 0, true, nullptr);
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

		// cloak (purple, protection)
		item = newItem(CLOAK_PROTECTION, SERVICABLE, 0, 1, 3, true, nullptr);
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

		// amulet of magic reflection
		item = newItem(AMULET_MAGICREFLECTION, EXCELLENT, 0, 1, 1, true, nullptr);
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

		// leather boots
		item = newItem(LEATHER_BOOTS, SERVICABLE, 0, 1, 0, true, nullptr);
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
			// magicstaff of light
			item = newItem(MAGICSTAFF_LIGHT, EXCELLENT, 0, 1, 3, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// potion of restore magic
			item = newItem(POTION_RESTOREMAGIC, EXCELLENT, 0, 1, 1, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// spellbook of fireball
			item = newItem(SPELLBOOK_FIREBALL, SERVICABLE, 0, 1, 3, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[7].item = item2->uid;
			free(item);

			// spellbook of cold
			item = newItem(SPELLBOOK_COLD, SERVICABLE, 0, 1, 4, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[8].item = item2->uid;
			free(item);

			// spellbook of light
			item = newItem(SPELLBOOK_LIGHT, SERVICABLE, 0, 1, 5, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[9].item = item2->uid;
			free(item);
		}
	}
	// arcanist
	else if ( client_classes[player] == CLASS_ARCANIST )
	{
		// attributes
		stats[player]->INT += 1;
		stats[player]->CHR += 1;
		stats[player]->PER -= 1;
		stats[player]->STR -= 1;

		stats[player]->MAXMP += 10;
		stats[player]->MP += 10;

		// skills
		stats[player]->PROFICIENCIES[PRO_MAGIC] = 25;
		stats[player]->PROFICIENCIES[PRO_SPELLCASTING] = 50;
		stats[player]->PROFICIENCIES[PRO_STEALTH] = 25;
		stats[player]->PROFICIENCIES[PRO_LOCKPICKING] = 25;
		stats[player]->PROFICIENCIES[PRO_SWORD] = 25;
		stats[player]->PROFICIENCIES[PRO_RANGED] = 25;

		// iron sword
		item = newItem(IRON_SWORD, SERVICABLE, 0, 1, 0, true, nullptr);
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

		// crossbow
		item = newItem(CROSSBOW, SERVICABLE, 0, 1, 0, true, nullptr);
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

		// leather armor
		item = newItem(LEATHER_BREASTPIECE, SERVICABLE, 0, 1, 0, true, nullptr);
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

		// leather boots
		item = newItem(LEATHER_BOOTS, SERVICABLE, 0, 1, 0, true, nullptr);
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

		// cloak (purple)
		item = newItem(CLOAK, WORN, 0, 1, 3, true, nullptr);
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

		// hood (purple)
		item = newItem(HAT_HOOD, WORN, 0, 1, 3, true, nullptr);
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
			// magicstaff of opening
			item = newItem(MAGICSTAFF_OPENING, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			// spellbook of forcebolt
			item = newItem(SPELLBOOK_FORCEBOLT, WORN, 0, 1, 6, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[8].item = item2->uid;
			free(item);

			// spellbook of light
			item = newItem(SPELLBOOK_LIGHT, WORN, 0, 1, 7, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[9].item = item2->uid;
			free(item);
		}
	}
	// joker
	else if ( client_classes[player] == CLASS_JOKER )
	{
		// attributes
		stats[player]->INT += 1;
		stats[player]->CHR += 1;
		stats[player]->CON -= 1;
		stats[player]->STR -= 1;
		stats[player]->GOLD += 200;

		// skills
		stats[player]->PROFICIENCIES[PRO_LOCKPICKING] = 25;
		stats[player]->PROFICIENCIES[PRO_TRADING] = 25;
		stats[player]->PROFICIENCIES[PRO_LEADERSHIP] = 20;
		stats[player]->PROFICIENCIES[PRO_MAGIC] = 25;
		stats[player]->PROFICIENCIES[PRO_SPELLCASTING] = 25;
		stats[player]->PROFICIENCIES[PRO_ALCHEMY] = 10;

		// jester hat
		item = newItem(HAT_JESTER, SERVICABLE, 0, 1, 0, true, nullptr);
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
			// slingshot
			item = newItem(SLING, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[0].item = item2->uid;
			free(item);

			// lockpicks
			item = newItem(TOOL_LOCKPICK, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// scroll of teleportation
			item = newItem(SCROLL_TELEPORTATION, EXCELLENT, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// scroll of food
			item = newItem(SCROLL_FOOD, EXCELLENT, 0, 1, 0, false, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// ring of levitation
			item = newItem(RING_LEVITATION, SERVICABLE, 0, 1, 2, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// spellbook of confuse
			item = newItem(SPELLBOOK_CONFUSE, WORN, 0, 1, 8, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[9].item = item2->uid;
			free(item);

			// blindfold
			item = newItem(TOOL_BLINDFOLD, SERVICABLE, 0, 1, 2, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// luckstone
			item = newItem(GEM_LUCK, EXCELLENT, 0, 1, 1, false, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// fish
			item = newItem(FOOD_FISH, SERVICABLE, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	// sexton
	else if ( client_classes[player] == CLASS_SEXTON )
	{
		// attributes
		stats[player]->STR -= 1;
		stats[player]->DEX += 1;
		stats[player]->CON -= 1;
		stats[player]->INT += 1;

		stats[player]->MAXMP += 5;
		stats[player]->MP += 5;

		// skills
		stats[player]->PROFICIENCIES[PRO_MACE] = 10;
		stats[player]->PROFICIENCIES[PRO_SHIELD] = 10;
		stats[player]->PROFICIENCIES[PRO_STEALTH] = 40;
		stats[player]->PROFICIENCIES[PRO_SPELLCASTING] = 40;
		stats[player]->PROFICIENCIES[PRO_MAGIC] = 40;
		stats[player]->PROFICIENCIES[PRO_RANGED] = 20;
		stats[player]->PROFICIENCIES[PRO_ALCHEMY] = 20;

		// bronze mace
		item = newItem(BRONZE_MACE, SERVICABLE, 0, 1, 0, true, nullptr);
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

		// shard
		item = newItem(TOOL_CRYSTALSHARD, SERVICABLE, 0, 2, 0, true, nullptr);
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

		// leather breastpiece
		item = newItem(LEATHER_BREASTPIECE, WORN, 0, 1, 0, true, nullptr);
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

		// leather boots
		item = newItem(LEATHER_BOOTS, SERVICABLE, 0, 1, 0, true, nullptr);
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

		// purple hood
		item = newItem(HAT_FEZ, SERVICABLE, 0, 1, 0, true, nullptr);
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
			// chakram
			item = newItem(STEEL_CHAKRAM, SERVICABLE, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// bread
			item = newItem(FOOD_BREAD, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// bread
			item = newItem(FOOD_TOMALLEY, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// books
			item = newItem(SPELLBOOK_SLEEP, WORN, 0, 1, 7, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[7].item = item2->uid;
			free(item);

			item = newItem(SPELLBOOK_OPENING, WORN, 0, 1, 6, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[8].item = item2->uid;
			free(item);

			item = newItem(SPELLBOOK_LOCKING, WORN, 0, 1, 6, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[9].item = item2->uid;
			free(item);
		}
	}
	// ninja
	else if ( client_classes[player] == CLASS_NINJA )
	{
		// attributes
		stats[player]->STR -= 1;
		stats[player]->DEX += 2;
		stats[player]->CON -= 1;
		stats[player]->INT -= 2;

		stats[player]->MAXHP += 5;
		stats[player]->HP += 5;

		// skills
		stats[player]->PROFICIENCIES[PRO_STEALTH] = 60;
		stats[player]->PROFICIENCIES[PRO_SWORD] = 60;
		stats[player]->PROFICIENCIES[PRO_RANGED] = 40;

		// katana
		item = newItem(CRYSTAL_SWORD, DECREPIT, 0, 1, 0, true, nullptr);
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

		// black hood
		item = newItem(HAT_HOOD, SERVICABLE, 0, 1, 2, true, nullptr);
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

		// tunic
		item = newItem(TUNIC, SERVICABLE, 0, 1, 1, true, nullptr);
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

		// leather boots
		item = newItem(LEATHER_BOOTS_SPEED, SERVICABLE, 0, 1, 0, true, nullptr);
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

		// black cloak
		item = newItem(CLOAK_BLACK, EXCELLENT, 0, 1, 0, true, nullptr);
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
			// daggers
			item = newItem(IRON_DAGGER, WORN, 0, 5, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// bear trap
			item = newItem(TOOL_BEARTRAP, SERVICABLE, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);
			
			// paralyze potion
			item = newItem(POTION_PARALYSIS, SERVICABLE, 0, 1, 1, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[3].item = item2->uid;
			free(item);

			// invis potion
			item = newItem(POTION_INVISIBILITY, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// teleport scroll
			item = newItem(SCROLL_TELEPORTATION, SERVICABLE, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	// monk
	else if ( client_classes[player] == CLASS_MONK )
	{
		// attributes
		stats[player]->STR += 1;
		stats[player]->CON += 2;
		stats[player]->PER -= -1;
		stats[player]->CHR -= -1;

		stats[player]->MAXHP += 10;
		stats[player]->HP += 10;

		// skills
		stats[player]->PROFICIENCIES[PRO_SHIELD] = 40;
		stats[player]->PROFICIENCIES[PRO_SPELLCASTING] = 20;
		stats[player]->PROFICIENCIES[PRO_LEADERSHIP] = 10;
		stats[player]->PROFICIENCIES[PRO_POLEARM] = 10;
		stats[player]->PROFICIENCIES[PRO_MAGIC] = 10;
		stats[player]->PROFICIENCIES[PRO_RANGED] = 10;
		stats[player]->PROFICIENCIES[PRO_UNARMED] = 50;
		stats[player]->PROFICIENCIES[PRO_ALCHEMY] = 20;

		// knuckles
		item = newItem(BRASS_KNUCKLES, EXCELLENT, 0, 1, 0, true, nullptr);
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

		// tunic
		item = newItem(TUNIC, EXCELLENT, 0, 1, 0, true, nullptr);
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

		// ring slow digestion
		item = newItem(RING_SLOWDIGESTION, SERVICABLE, 0, 1, 0, true, nullptr);
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
			// light book
			item = newItem(SPELLBOOK_LIGHT, WORN, 0, 1, 7, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[9].item = item2->uid;
			free(item);
		}
	}
	// start DLC
	else if ( client_classes[player] == CLASS_CONJURER )
	{
		// attributes
		stats[player]->INT += 1;
		stats[player]->CON += 2;
		stats[player]->DEX -= 1;
		stats[player]->PER -= 2;

		stats[player]->MAXHP -= 0;
		stats[player]->HP -= 0;
		stats[player]->MAXMP += 15;
		stats[player]->MP += 15;

		// skills
		stats[player]->PROFICIENCIES[PRO_MAGIC] = 40;
		stats[player]->PROFICIENCIES[PRO_SPELLCASTING] = 40;
		stats[player]->PROFICIENCIES[PRO_STEALTH] = 20;
		stats[player]->PROFICIENCIES[PRO_RANGED] = 20;
		stats[player]->PROFICIENCIES[PRO_LEADERSHIP] = 40;
		stats[player]->PROFICIENCIES[PRO_ALCHEMY] = 20;

		// weapon
		item = newItem(MAGICSTAFF_LIGHTNING, EXCELLENT, 0, 1, 0, true, nullptr);
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

		item = newItem(TOOL_LANTERN, EXCELLENT, 0, 1, 0, true, nullptr);
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

		// red hood
		item = newItem(HAT_HOOD_RED, SERVICABLE, 0, 1, 1, true, nullptr);
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

		// red cloak
		item = newItem(CLOAK, SERVICABLE, 0, 1, 2, true, nullptr);
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
			// slow book
			item = newItem(SPELLBOOK_SLOW, WORN, 0, 1, 8, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[9].item = item2->uid;
			free(item);

			// restore magic
			item = newItem(POTION_RESTOREMAGIC, EXCELLENT, 0, 1, 1, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);
		}
	}
	else if ( client_classes[player] == CLASS_ACCURSED )
	{
		// attributes
		stats[player]->INT += 10;
		stats[player]->STR -= 2;
		stats[player]->CON -= 2;
		stats[player]->DEX -= 3;
		stats[player]->PER -= 1;

		stats[player]->MAXHP -= 0;
		stats[player]->HP -= 0;
		stats[player]->MAXMP += 10;
		stats[player]->MP += 10;

		// skills
		stats[player]->PROFICIENCIES[PRO_MAGIC] = 70;
		stats[player]->PROFICIENCIES[PRO_SPELLCASTING] = 40;
		stats[player]->PROFICIENCIES[PRO_STEALTH] = 40;
		stats[player]->PROFICIENCIES[PRO_APPRAISAL] = 20;
		stats[player]->PROFICIENCIES[PRO_UNARMED] = 40;

		// doublet
		item = newItem(SILVER_DOUBLET, EXCELLENT, 0, 1, 0, true, nullptr);
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

		// gloves
		item = newItem(SUEDE_GLOVES, SERVICABLE, 0, 1, 0, true, nullptr);
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

		// boots
		item = newItem(SUEDE_BOOTS, SERVICABLE, 0, 1, 0, true, nullptr);
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
			// invis book
			item = newItem(SPELLBOOK_INVISIBILITY, WORN, 0, 1, 2, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[9].item = item2->uid;
			free(item);

			// blood
			item = newItem(FOOD_BLOOD, EXCELLENT, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[0].item = item2->uid;
			free(item);

			// restore magic
			item = newItem(POTION_RESTOREMAGIC, EXCELLENT, 0, 2, 1, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);
		}
	}
	else if ( client_classes[player] == CLASS_MESMER )
	{
		// attributes
		stats[player]->STR -= 2;
		stats[player]->CON -= 3;
		stats[player]->INT += 2;
		stats[player]->DEX -= 2;
		stats[player]->PER += 2;
		stats[player]->CHR += 4;

		stats[player]->MAXHP -= 5;
		stats[player]->HP -= 5;
		stats[player]->MAXMP += 10;
		stats[player]->MP += 10;

		// skills
		stats[player]->PROFICIENCIES[PRO_MAGIC] = 60;
		stats[player]->PROFICIENCIES[PRO_SPELLCASTING] = 40;
		stats[player]->PROFICIENCIES[PRO_POLEARM] = 20;
		stats[player]->PROFICIENCIES[PRO_LEADERSHIP] = 60;

		// ring
		item = newItem(RING_PROTECTION, EXCELLENT, curseItems ? -2 : 2, 1, 0, true, nullptr);
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

		// hood silver
		item = newItem(HAT_HOOD_SILVER, SERVICABLE, 0, 1, 0, true, nullptr);
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

		// cloak silver
		item = newItem(CLOAK_SILVER, SERVICABLE, 0, 1, 0, true, nullptr);
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

		// weapon
		item = newItem(MAGICSTAFF_CHARM, EXCELLENT, curseItems ? -1 : 1, 1, 0, true, nullptr);
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

		if ( player == clientnum )
		{
			// spear
			item = newItem(IRON_SPEAR, SERVICABLE, curseItems ? -1 : 1, 1, 1, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// restore magic
			item = newItem(POTION_RESTOREMAGIC, EXCELLENT, 0, 2, 1, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			// confusion
			item = newItem(POTION_CONFUSION, EXCELLENT, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[3].item = item2->uid;
			free(item);

			// cold spellbook
			item = newItem(SPELLBOOK_COLD, SERVICABLE, 0, 1, 4, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[8].item = item2->uid;
			free(item);

			// charm monster spellbook
			item = newItem(SPELLBOOK_CHARM_MONSTER, DECREPIT, 0, 1, 8, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[9].item = item2->uid;
			free(item);
		}
	}
	else if ( client_classes[player] == CLASS_BREWER )
	{
		// attributes
		stats[player]->STR += -2;
		stats[player]->DEX += 1;
		stats[player]->CON -= 2;
		stats[player]->INT -= 2;
		stats[player]->PER += 1;
		stats[player]->CHR += 1;

		stats[player]->MAXHP += 10;
		stats[player]->HP += 10;
		stats[player]->MAXMP -= 10;
		stats[player]->MP -= 10;

		stats[player]->GOLD = 100;

		// skills
		/*stats[player]->PROFICIENCIES[PRO_MACE] = 60;
		stats[player]->PROFICIENCIES[PRO_SHIELD] = 40;*/
		stats[player]->PROFICIENCIES[PRO_AXE] = 10;
		stats[player]->PROFICIENCIES[PRO_UNARMED] = 25;
		stats[player]->PROFICIENCIES[PRO_TRADING] = 10;
		stats[player]->PROFICIENCIES[PRO_APPRAISAL] = 10;
		stats[player]->PROFICIENCIES[PRO_LEADERSHIP] = 25;
		stats[player]->PROFICIENCIES[PRO_ALCHEMY] = 50;

		// booze
		item = newItem(IRON_AXE, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			hotbar[0].item = item2->uid;
			free(item);
		}

		// empty bottles
		item = newItem(POTION_EMPTY, SERVICABLE, 0, 3, 0, true, nullptr);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			equipItem(item2, &stats[player]->weapon, player);
			hotbar[1].item = item2->uid;
			free(item);
		}
		else
		{
			equipItem(item, &stats[player]->weapon, player);
		}

		// boots
		item = newItem(IRON_BOOTS, WORN, 0, 1, 0, true, nullptr);
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

		// backpack
		item = newItem(CLOAK_BACKPACK, SERVICABLE, 0, 1, 0, true, nullptr);
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
			//// weapon
			//item = newItem(IRON_MACE, SERVICABLE, 0, 1, 0, true, NULL);
			//item2 = itemPickup(player, item);
			//hotbar[1].item = item2->uid;
			//free(item);

			// blindness
			item = newItem(POTION_ACID, SERVICABLE, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[5].item = item2->uid;
			free(item);

			// booze
			item = newItem(POTION_BOOZE, SERVICABLE, 0, 2, 2, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[6].item = item2->uid;
			free(item);

			// juice
			item = newItem(POTION_JUICE, SERVICABLE, 0, 2, 3, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[7].item = item2->uid;
			free(item);
			
			// alembic
			item = newItem(TOOL_ALEMBIC, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[8].item = item2->uid;
			free(item);

			// polymorph
			item = newItem(POTION_POLYMORPH, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// blindness
			item = newItem(POTION_BLINDNESS, SERVICABLE, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// speed
			item = newItem(POTION_SPEED, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// bread
			item = newItem(FOOD_BREAD, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(READABLE_BOOK, DECREPIT, 0, 1, getBook("Bottle Book"), true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[9].item = item2->uid;
			free(item);
		}
	}
	else if ( client_classes[player] == CLASS_SHAMAN )
	{
		// attributes
		stats[player]->STR -= 1;
		stats[player]->INT += 2;
		stats[player]->PER += 1;
		stats[player]->CHR += 1;

		stats[player]->MAXHP += 5;
		stats[player]->HP += 5;

		stats[player]->MAXMP += 10;
		stats[player]->MP += 10;

		// skills
		stats[player]->PROFICIENCIES[PRO_SPELLCASTING] = 40;
		stats[player]->PROFICIENCIES[PRO_MAGIC] = 40;
		stats[player]->PROFICIENCIES[PRO_UNARMED] = 10;
		stats[player]->PROFICIENCIES[PRO_ALCHEMY] = 10;
		stats[player]->PROFICIENCIES[PRO_STEALTH] = 10;
		/*stats[player]->PROFICIENCIES[PRO_SHIELD] = 40;
		stats[player]->PROFICIENCIES[PRO_LEADERSHIP] = 10;
		stats[player]->PROFICIENCIES[PRO_POLEARM] = 10;
		stats[player]->PROFICIENCIES[PRO_RANGED] = 10;*/

		item = newItem(MAGICSTAFF_POISON, EXCELLENT, 0, 1, 0, true, nullptr);
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

		item = newItem(MASK_SHAMAN, SERVICABLE, 0, 1, 0, true, nullptr);
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

		//// ring slow digestion
		//item = newItem(RING_SLOWDIGESTION, SERVICABLE, 0, 1, 0, true, NULL);
		//if ( player == clientnum )
		//{
		//	item2 = itemPickup(player, item);
		//	useItem(item2, player);
		//	free(item);
		//}
		//else
		//{
		//	useItem(item, player);
		//}

		if ( player == clientnum )
		{
			item = newItem(BRONZE_SWORD, WORN, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			item = newItem(GEM_ROCK, WORN, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			item = newItem(ENCHANTED_FEATHER, EXCELLENT, 0, 1, ENCHANTED_FEATHER_MAX_DURABILITY - 1, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[3].item = item2->uid;
			free(item);
		}
	}
	else if ( client_classes[player] == CLASS_PUNISHER )
	{
		// attributes
		stats[player]->STR -= 1;
		stats[player]->DEX += 1;
		stats[player]->CON -= 1;
		stats[player]->INT -= 1;

		/*stats[player]->MAXHP += 5;
		stats[player]->HP += 5;

		stats[player]->MAXMP += 10;
		stats[player]->MP += 10;*/

		// skills
		stats[player]->PROFICIENCIES[PRO_SPELLCASTING] = 40;
		stats[player]->PROFICIENCIES[PRO_MAGIC] = 20;
		stats[player]->PROFICIENCIES[PRO_RANGED] = 25;
		stats[player]->PROFICIENCIES[PRO_AXE] = 25;
		/*stats[player]->PROFICIENCIES[PRO_SHIELD] = 40;
		stats[player]->PROFICIENCIES[PRO_LEADERSHIP] = 10;
		stats[player]->PROFICIENCIES[PRO_POLEARM] = 10;
		stats[player]->PROFICIENCIES[PRO_UNARMED] = 50;
		stats[player]->PROFICIENCIES[PRO_ALCHEMY] = 20;*/

		item = newItem(TOOL_WHIP, EXCELLENT, 0, 1, 0, true, nullptr);
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

		item = newItem(PUNISHER_HOOD, SERVICABLE, 0, 1, 1, true, nullptr);
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

		//// ring slow digestion
		//item = newItem(RING_SLOWDIGESTION, SERVICABLE, 0, 1, 0, true, NULL);
		//if ( player == clientnum )
		//{
		//	item2 = itemPickup(player, item);
		//	useItem(item2, player);
		//	free(item);
		//}
		//else
		//{
		//	useItem(item, player);
		//}

		if ( player == clientnum )
		{
			item = newItem(CRYSTAL_BATTLEAXE, DECREPIT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			item = newItem(FOOD_MEAT, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(RING_CONFLICT, WORN, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);
		}
	}
	else if ( client_classes[player] == CLASS_HUNTER )
	{
		// attributes
		stats[player]->STR -= 3;
		stats[player]->DEX += 1;
		stats[player]->PER += 3;
		stats[player]->INT -= 3;
		stats[player]->CON -= 1;

		stats[player]->MAXHP -= 10;
		stats[player]->HP -= 10;
		stats[player]->MAXMP -= 10;
		stats[player]->MP -= 10;

		// skills
		stats[player]->PROFICIENCIES[PRO_SPELLCASTING] = 10;
		stats[player]->PROFICIENCIES[PRO_APPRAISAL] = 20;
		stats[player]->PROFICIENCIES[PRO_STEALTH] = 25;
		stats[player]->PROFICIENCIES[PRO_SWIMMING] = 50;
		stats[player]->PROFICIENCIES[PRO_RANGED] = 50;
		stats[player]->PROFICIENCIES[PRO_LOCKPICKING] = 10;

		item = newItem(LONGBOW, EXCELLENT, 0, 1, 0, true, nullptr);
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

		item = newItem(AMULET_POISONRESISTANCE, SERVICABLE, 0, 1, 0, true, nullptr);
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

		item = newItem(BRACERS, WORN, 0, 1, 0, true, nullptr);
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

		item = newItem(LEATHER_BOOTS, EXCELLENT, 0, 1, 0, true, nullptr);
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
			item = newItem(BOOMERANG, DECREPIT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			item = newItem(QUIVER_SILVER, EXCELLENT, 0, 40, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			item = newItem(QUIVER_LIGHTWEIGHT, EXCELLENT, 0, 40, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[3].item = item2->uid;
			free(item);

			item = newItem(QUIVER_HUNTING, EXCELLENT, 0, 20, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[4].item = item2->uid;
			free(item);

			item = newItem(SCROLL_CONJUREARROW, EXCELLENT, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(FOOD_MEAT, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(FOOD_FISH, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(TOOL_BLINDFOLD_TELEPATHY, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[5].item = item2->uid;
			free(item);

			item = newItem(POTION_SPEED, EXCELLENT, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[6].item = item2->uid;
			free(item);

			// TO DELETE **********
			//item = newItem(ARTIFACT_BOW, EXCELLENT, 0, 1, 0, true, NULL);
			//item2 = itemPickup(player, item);
			//free(item);

			//item = newItem(COMPOUND_BOW, EXCELLENT, 0, 1, 0, true, NULL);
			//item2 = itemPickup(player, item);
			//free(item);

			//item = newItem(SLING, EXCELLENT, 0, 1, 0, true, NULL);
			//item2 = itemPickup(player, item);
			//free(item);

			//item = newItem(SHORTBOW, EXCELLENT, 0, 1, 0, true, NULL);
			//item2 = itemPickup(player, item);
			//free(item);

			//item = newItem(CROSSBOW, EXCELLENT, 0, 1, 0, true, NULL);
			//item2 = itemPickup(player, item);
			//free(item);

			//for ( int i = QUIVER_SILVER; i <= QUIVER_HUNTING; ++i )
			//{
			//	item = newItem(static_cast<ItemType>(i), EXCELLENT, 1, 48, 0, true, NULL);
			//	item2 = itemPickup(player, item);
			//	free(item);
			//}

			// TO DELETE **********
		}
	}
	else if ( client_classes[player] == CLASS_MACHINIST )
	{
		// attributes
		stats[player]->STR -= 2;
		//stats[player]->DEX -= 2;
		stats[player]->CON -= 3;
		stats[player]->INT += 1;
		stats[player]->PER += 0;

		stats[player]->MAXHP -= 5;
		stats[player]->HP -= 5;

		stats[player]->MAXMP -= 10;
		stats[player]->MP -= 10;

		// skills
		stats[player]->PROFICIENCIES[PRO_LOCKPICKING] = 40;
		stats[player]->PROFICIENCIES[PRO_RANGED] = 10;
		stats[player]->PROFICIENCIES[PRO_ALCHEMY] = 10;
		stats[player]->PROFICIENCIES[PRO_TRADING] = 10;

		item = newItem(CROSSBOW, EXCELLENT, 0, 1, 0, true, nullptr);
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

		item = newItem(TOOL_TINKERING_KIT, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( player == clientnum )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[5].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(MACHINIST_APRON, EXCELLENT, 0, 1, 0, true, nullptr);
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
			item = newItem(TOOL_BEARTRAP, EXCELLENT, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			item = newItem(TOOL_DUMMYBOT, DECREPIT, 0, 1, ITEM_TINKERING_APPEARANCE, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			item = newItem(TOOL_SENTRYBOT, DECREPIT, 0, 1, ITEM_TINKERING_APPEARANCE, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[3].item = item2->uid;
			free(item);

			item = newItem(TOOL_SLEEP_BOMB, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[4].item = item2->uid;
			free(item);

			item = newItem(TOOL_LOCKPICK, EXCELLENT, 0, 4, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[6].item = item2->uid;
			free(item);

			if ( stats[player]->playerRace != RACE_AUTOMATON )
			{
				item = newItem(FOOD_APPLE, EXCELLENT, 0, 2, 0, true, nullptr);
				item2 = itemPickup(player, item);
				free(item);

				item = newItem(FOOD_CHEESE, EXCELLENT, 0, 2, 0, true, nullptr);
				item2 = itemPickup(player, item);
				free(item);
			}
			else
			{
				item = newItem(SCROLL_FIRE, SERVICABLE, 0, 1, 0, true, nullptr);
				item2 = itemPickup(player, item);
				free(item);
			}

			item = newItem(TOOL_METAL_SCRAP, DECREPIT, 0, 16, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(TOOL_MAGIC_SCRAP, DECREPIT, 0, 8, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(POTION_EMPTY, SERVICABLE, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);
		}
	}

	stats[player]->OLDHP = stats[player]->HP;

	if ( stats[player]->appearance == 0 && stats[player]->playerRace == RACE_GOATMAN )
	{
		stats[player]->EFFECTS[EFF_ASLEEP] = true;
		stats[player]->EFFECTS_TIMERS[EFF_ASLEEP] = -1;
		if ( player == clientnum )
		{
			// extra booze for hangover :)
			item = newItem(POTION_BOOZE, SERVICABLE, 0, 1, 2, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	else
	{
		stats[player]->EFFECTS[EFF_ASLEEP] = false;
		stats[player]->EFFECTS_TIMERS[EFF_ASLEEP] = 0;
	}

	if ( stats[player]->appearance == 0 && stats[player]->playerRace == RACE_AUTOMATON )
	{
		//stats[player]->HUNGER = 150;
	}

	if ( stats[player]->appearance == 0 
		&& client_classes[player] <= CLASS_MONK 
		&& stats[player]->playerRace != RACE_HUMAN )
	{
		if ( player == clientnum )
		{
			// bonus polymorph potions
			item = newItem(POTION_POLYMORPH, SERVICABLE, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	if ( stats[player]->appearance == 0 
		&& client_classes[player] >= CLASS_CONJURER 
		&& client_classes[player] <= CLASS_HUNTER 
		&& stats[player]->playerRace != RACE_HUMAN )
	{
		if ( player == clientnum )
		{
			// bonus polymorph potions
			item = newItem(POTION_POLYMORPH, SERVICABLE, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);
		}
	}

	if ( svFlags & SV_FLAG_LIFESAVING )
	{
		item = newItem(AMULET_LIFESAVING, WORN, 0, 1, 0, true, nullptr);
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
	}
	if ( player == clientnum )
	{
		if ( stats[player]->playerRace == RACE_VAMPIRE && stats[player]->appearance == 0 )
		{
			addSpell(SPELL_LEVITATION, player, true);
			addSpell(SPELL_BLEED, player, true);
		}
		else if ( stats[player]->playerRace == RACE_SUCCUBUS && stats[player]->appearance == 0 )
		{
			addSpell(SPELL_TELEPORTATION, player, true);
			addSpell(SPELL_SELF_POLYMORPH, player, true);
		}
		else if ( stats[player]->playerRace == RACE_INSECTOID && stats[player]->appearance == 0 )
		{
			addSpell(SPELL_FLUTTER, player, true);
			addSpell(SPELL_DASH, player, true);
			addSpell(SPELL_ACID_SPRAY, player, true);
		}
		else if ( stats[player]->playerRace == RACE_INCUBUS && stats[player]->appearance == 0 )
		{
			addSpell(SPELL_TELEPORTATION, player, true);
			addSpell(SPELL_SHADOW_TAG, player, true);
		}
		else if ( stats[player]->playerRace == RACE_AUTOMATON && stats[player]->appearance == 0 )
		{
			addSpell(SPELL_SALVAGE, player, true);
		}

		if ( stats[player]->PROFICIENCIES[PRO_ALCHEMY] >= 0 )
		{
			bool learned = false;
			if ( stats[player]->PROFICIENCIES[PRO_ALCHEMY] >= 0 )
			{
				ItemType potion = POTION_WATER;
				learned = GenericGUI.alchemyLearnRecipe(potion, false, false);
			}
			if ( stats[player]->PROFICIENCIES[PRO_ALCHEMY] >= 20 )
			{
				ItemType potion = POTION_JUICE;
				learned = GenericGUI.alchemyLearnRecipe(potion, false, false);
				potion = POTION_BOOZE;
				learned = GenericGUI.alchemyLearnRecipe(potion, false, false);
			}
			if ( stats[player]->PROFICIENCIES[PRO_ALCHEMY] >= 40 )
			{
				ItemType potion = POTION_ACID;
				learned = GenericGUI.alchemyLearnRecipe(potion, false, false);
			}
			if ( stats[player]->PROFICIENCIES[PRO_ALCHEMY] >= 60 )
			{
				ItemType potion = POTION_INVISIBILITY;
				learned = GenericGUI.alchemyLearnRecipe(potion, false, false);
				potion = POTION_POLYMORPH;
				learned = GenericGUI.alchemyLearnRecipe(potion, false, false);
			}
		}

		if ( client_classes[clientnum] == CLASS_SHAMAN )
		{
			addSpell(SPELL_RAT_FORM, player, true);
			addSpell(SPELL_SPIDER_FORM, player, true);
			addSpell(SPELL_TROLL_FORM, player, true);
			addSpell(SPELL_IMP_FORM, player, true);
			addSpell(SPELL_REVERT_FORM, player, true);

			addSpell(SPELL_DETECT_FOOD, player, true);
			addSpell(SPELL_SPEED, player, true);
			addSpell(SPELL_POISON, player, true);
			addSpell(SPELL_SPRAY_WEB, player, true);
			addSpell(SPELL_STRIKE, player, true);
			addSpell(SPELL_FEAR, player, true);
			addSpell(SPELL_LIGHTNING, player, true);
			addSpell(SPELL_CONFUSE, player, true);
			addSpell(SPELL_TROLLS_BLOOD, player, true);
			addSpell(SPELL_AMPLIFY_MAGIC, player, true);
		}
		else if ( client_classes[clientnum] == CLASS_PUNISHER )
		{
			addSpell(SPELL_TELEPULL, player, true);
			addSpell(SPELL_DEMON_ILLUSION, player, true);
		}
		else if ( client_classes[clientnum] == CLASS_CONJURER )
		{
			addSpell(SPELL_SUMMON, player, true);
		}

		//printlog("spell size: %d", list_Size(&spellList));
		// move default items to the right
		for ( node_t* node = stats[player]->inventory.first; node != nullptr; node = node->next )
		{
			Item* item = static_cast<Item*>(node->element);
			if ( item )
			{
				item->x = INVENTORY_SIZEX - item->x - 1;
				if ( item->type == SPELL_ITEM )
				{
					bool skipSpellRearrange = false;
					spell_t* spell = getSpellFromItem(item);
					if ( spell && client_classes[clientnum] == CLASS_SHAMAN )
					{
						// don't add shapeshift spells to hotbar.
						switch ( spell->ID )
						{
							case SPELL_SPEED:
							case SPELL_POISON:
							case SPELL_SPRAY_WEB:
							case SPELL_STRIKE:
							case SPELL_FEAR:
							case SPELL_LIGHTNING:
							case SPELL_CONFUSE:
							case SPELL_DETECT_FOOD:
							case SPELL_TROLLS_BLOOD:
							case SPELL_AMPLIFY_MAGIC:
								item->appearance += 1000;
								item->y -= 100;
								skipSpellRearrange = true;
								break;
							default:
								break;
						}
					}
					if ( skipSpellRearrange )
					{
						continue;
					}
					for ( Uint32 i = 0; i < NUM_HOTBAR_SLOTS; ++i )
					{
						if ( hotbar[i].item == 0 )
						{
							//printlog("%d %s", i, item->getName());
							hotbar[i].item = item->uid;
							break;
						}
					}
				}
			}
		}
	}
	//stats[clientnum]->printStats();
	//PlayerCharacterClassManager playerCharacterClassManager(stats[player], client_classes[player]);
	//playerCharacterClassManager.writeToFile();
	//playerCharacterClassManager.readFromFile();
}

void initShapeshiftHotbar()
{
	Uint32 spellRevertUid = 0;
	std::vector<Uint32> monsterSpells;

	if ( stats[clientnum]->type == HUMAN )
	{
		return;
	}

	swapHotbarOnShapeshift = stats[clientnum]->type;
	hotbar_slot_t* newHotbar = hotbar_alternate[HOTBAR_DEFAULT]; // the monster's special hotbar.
	spell_t* newSpell = selected_spell_alternate[HOTBAR_DEFAULT];
	bool shapeshiftHotbarInit = false;
	if ( swapHotbarOnShapeshift > 0 )
	{
		if ( swapHotbarOnShapeshift == RAT )
		{
			newHotbar = hotbar_alternate[HOTBAR_RAT];
			newSpell = selected_spell_alternate[HOTBAR_RAT];
			shapeshiftHotbarInit = hotbarShapeshiftInit[HOTBAR_RAT];
			hotbarShapeshiftInit[HOTBAR_RAT] = true;
		}
		else if ( swapHotbarOnShapeshift == SPIDER )
		{
			newHotbar = hotbar_alternate[HOTBAR_SPIDER];
			newSpell = selected_spell_alternate[HOTBAR_SPIDER];
			shapeshiftHotbarInit = hotbarShapeshiftInit[HOTBAR_SPIDER];
			hotbarShapeshiftInit[HOTBAR_SPIDER] = true;
		}
		else if ( swapHotbarOnShapeshift == TROLL )
		{
			newHotbar = hotbar_alternate[HOTBAR_TROLL];
			newSpell = selected_spell_alternate[HOTBAR_TROLL];
			shapeshiftHotbarInit = hotbarShapeshiftInit[HOTBAR_TROLL];
			hotbarShapeshiftInit[HOTBAR_TROLL] = true;
		}
		else if ( swapHotbarOnShapeshift == CREATURE_IMP )
		{
			newHotbar = hotbar_alternate[HOTBAR_IMP];
			newSpell = selected_spell_alternate[HOTBAR_IMP];
			shapeshiftHotbarInit = hotbarShapeshiftInit[HOTBAR_IMP];
			hotbarShapeshiftInit[HOTBAR_IMP] = true;
		}
	}

	for ( Uint32 slotIndex = 0; slotIndex < NUM_HOTBAR_SLOTS; ++slotIndex )
	{
		hotbar_alternate[HOTBAR_DEFAULT][slotIndex].item = hotbar[slotIndex].item; // store our current hotbar.
		hotbar[slotIndex].item = newHotbar[slotIndex].item; // load from the monster's hotbar.
	}

	// find "shapeshift" only spells, add em to view.
	for ( node_t* node = stats[clientnum]->inventory.first; node != nullptr; node = node->next )
	{
		Item* item = static_cast<Item*>(node->element);
		if ( item && item->type == SPELL_ITEM )
		{
			spell_t* spell = getSpellFromItem(item);
			if ( spell )
			{
				if ( spell->ID == SPELL_REVERT_FORM )
				{
					spellRevertUid = item->uid;
					selected_spell_alternate[HOTBAR_DEFAULT] = selected_spell;
					if ( !newSpell )
					{
						selected_spell = spell; // revert form add to spell equipped.
						selected_spell_last_appearance = selected_spell->ID;

					}
					else
					{
						selected_spell = newSpell;
						selected_spell_last_appearance = selected_spell->ID;
					}
				}
				else if ( item->appearance >= 1000 )
				{
					if ( canUseShapeshiftSpellInCurrentForm(*item) == 1 )
					{
						monsterSpells.push_back(item->uid);
						item->y += 100;
					}

					if ( item->y >= 0 )
					{
						int x = 0;
						bool notfree = false;
						bool foundaspot = false;
						const bool tooManySpells = (list_Size(&spellList) >= INVENTORY_SIZEX * 3);
						int numRows = INVENTORY_SIZEY;
						if ( tooManySpells && gui_mode == GUI_MODE_INVENTORY && inventory_mode == INVENTORY_MODE_SPELL )
						{
							numRows = 4 + ((list_Size(&spellList) - (INVENTORY_SIZEX * 3)) / INVENTORY_SIZEX);
						}
						while ( true )
						{
							for ( int y = 0; y < numRows; y++ )
							{
								for ( node_t* node2 = stats[clientnum]->inventory.first; node2 != nullptr; node2 = node2->next )
								{
									Item* tempItem = static_cast<Item*>(node2->element);
									if ( tempItem == item )
									{
										continue;
									}
									if ( tempItem )
									{
										if ( tempItem->x == x && tempItem->y == y )
										{
											if ( itemCategory(tempItem) == SPELL_CAT )
											{
												notfree = true;  //Both spells. Can't fit in the same slot.
											}
										}
									}
								}
								if ( notfree )
								{
									notfree = false;
									continue;
								}
								item->x = x;
								item->y = y;
								foundaspot = true;
								break;
							}
							if ( foundaspot )
							{
								break;
							}
							x++;
						}
					}
					else
					{
						for ( Uint32 i = 0; i < NUM_HOTBAR_SLOTS; ++i )
						{
							if ( hotbar[i].item == item->uid )
							{
								hotbar[i].item = 0;
							}
						}
					}
				}
			}
		}
	}

	for ( Uint32 i = 0; i < NUM_HOTBAR_SLOTS; ++i )
	{
		if ( hotbar[i].item == 0 )
		{
			continue;
		}
		if ( hotbar[i].item == spellRevertUid )
		{
			spellRevertUid = 0;
		}
		for ( auto& monsterSpell : monsterSpells )
		{
			if ( monsterSpell == hotbar[i].item )
			{
				monsterSpell = 0;
			}
		}
	}

	int i = 0;
	for ( auto it = monsterSpells.begin(); it != monsterSpells.end() && !shapeshiftHotbarInit; ++it )
	{
		if ( *it != 0 )
		{
			hotbar[i].item = *it;
			++i;
		}
	}
	if ( spellRevertUid && !shapeshiftHotbarInit )
	{
		hotbar[4].item = spellRevertUid; // place revert form.
	}
}

void deinitShapeshiftHotbar()
{
	Uint32 swapItem = 0;
	hotbar_slot_t* newHotbar = hotbar_alternate[HOTBAR_DEFAULT]; // the monster's special hotbar.
	spell_t* newSpell = selected_spell_alternate[HOTBAR_DEFAULT];
	if ( swapHotbarOnShapeshift > 0 )
	{
		if ( swapHotbarOnShapeshift == RAT )
		{
			newHotbar = hotbar_alternate[HOTBAR_RAT];
			newSpell = selected_spell_alternate[HOTBAR_RAT];
		}
		else if ( swapHotbarOnShapeshift == SPIDER )
		{
			newHotbar = hotbar_alternate[HOTBAR_SPIDER];
			newSpell = selected_spell_alternate[HOTBAR_SPIDER];
		}
		else if ( swapHotbarOnShapeshift == TROLL )
		{
			newHotbar = hotbar_alternate[HOTBAR_TROLL];
			newSpell = selected_spell_alternate[HOTBAR_TROLL];
		}
		else if ( swapHotbarOnShapeshift == CREATURE_IMP )
		{
			newHotbar = hotbar_alternate[HOTBAR_IMP];
			newSpell = selected_spell_alternate[HOTBAR_IMP];
		}
	}
	for ( Uint32 slotIndex = 0; slotIndex < NUM_HOTBAR_SLOTS; ++slotIndex )
	{
		swapItem = hotbar[slotIndex].item;
		hotbar[slotIndex].item = hotbar_alternate[HOTBAR_DEFAULT][slotIndex].item; // swap back to default loadout
		newHotbar[slotIndex].item = swapItem;

		// double check for shapeshift spells and remove them.
		Item* item = uidToItem(hotbar[slotIndex].item);
		if ( item && itemCategory(item) == SPELL_CAT && item->appearance >= 1000 )
		{
			if ( canUseShapeshiftSpellInCurrentForm(*item) != 1 ) // not allowed to use spell.
			{
				hotbar[slotIndex].item = 0;
				hotbar_alternate[HOTBAR_DEFAULT][slotIndex].item = 0;
			}
		}
	}
	swapHotbarOnShapeshift = 0;
	newSpell = selected_spell;
	selected_spell = selected_spell_alternate[HOTBAR_DEFAULT];
	if ( selected_spell )
	{
		selected_spell_last_appearance = selected_spell->ID;
	}
	else
	{
		selected_spell_last_appearance = -1;
	}

	for ( node_t* node = stats[clientnum]->inventory.first; node != nullptr; node = node->next )
	{
		Item* item = static_cast<Item*>(node->element);
		if ( item )
		{
			if ( item->type == SPELL_ITEM && item->appearance >= 1000 )
			{
				spell_t* spell = getSpellFromItem(item);
				if ( spell && client_classes[clientnum] == CLASS_SHAMAN )
				{
					// move shapeshift spells out of inventory. 
					// if somehow the spell got added to your selected spell then remove it.
					switch ( spell->ID )
					{
						case SPELL_SPEED:
						case SPELL_POISON:
						case SPELL_SPRAY_WEB:
						case SPELL_STRIKE:
						case SPELL_FEAR:
						case SPELL_LIGHTNING:
						case SPELL_CONFUSE:
						case SPELL_DETECT_FOOD:
						case SPELL_TROLLS_BLOOD:
						case SPELL_AMPLIFY_MAGIC:
							if ( item->y >= 0 )
							{
								item->y -= 100;
							}
							if ( selected_spell == spell )
							{
								selected_spell = nullptr;
								selected_spell_last_appearance = -1;
							}
							if ( selected_spell_alternate[HOTBAR_DEFAULT] == spell )
							{
								selected_spell_alternate[HOTBAR_DEFAULT] = nullptr;
							}
							break;
						default:
							break;
					}
				}
			}
		}
	}
}

bool playerUnlockedShamanSpell(const int player, Item* const item)
{
	if ( player < 0 && player >= MAXPLAYERS )
	{
		return false;
	}

	if ( !stats[player] || !item || item->type != SPELL_ITEM )
	{
		return false;
	}

	spell_t* spell = getSpellFromItem(item);
	int levelRequirement = 0;
	if ( spell && client_classes[player] == CLASS_SHAMAN )
	{
		if ( item->appearance >= 1000 )
		{
			switch ( spell->ID )
			{
				case SPELL_DETECT_FOOD:
					levelRequirement = 0;
					break;
				case SPELL_SPRAY_WEB:
				case SPELL_SPEED:
					levelRequirement = 3;
					break;
				case SPELL_POISON:
				case SPELL_STRIKE:
					levelRequirement = 6;
					break;
				case SPELL_TROLLS_BLOOD:
				case SPELL_LIGHTNING:
				case SPELL_CONFUSE:
					levelRequirement = 12;
					break;
				case SPELL_FEAR:
				case SPELL_AMPLIFY_MAGIC:
					levelRequirement = 15;
					break;
				default:
					return true;
			}
		}
		else
		{
			switch ( spell->ID )
			{
				case SPELL_RAT_FORM:
					levelRequirement = 0;
					break;
				case SPELL_SPIDER_FORM:
					levelRequirement = 3;
					break;
				case SPELL_TROLL_FORM:
					levelRequirement = 6;
					break;
				case SPELL_IMP_FORM:
					levelRequirement = 12;
					break;
				default:
					return true;
			}
		}
	}

	if ( stats[player]->LVL >= levelRequirement )
	{
		return true;
	}
	return false;
}