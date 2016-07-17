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

/*-------------------------------------------------------------------------------

	initClass
	
	Sets up a character class for the given player

-------------------------------------------------------------------------------*/

void initClass(int player) {
	Item *item, *item2;

	// SEX MODIFIER
	// female; else male
	if( stats[player]->sex ) {
		stats[player]->DEX += 1;
	} else {
		stats[player]->STR += 1;
	}

	// CLASS LOADOUTS
	// barbarian
	if( client_classes[player] == 0 ) {
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

		// iron axe
		item = newItem(IRON_AXE,SERVICABLE,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			hotbar[0].item = item2->uid;
			free(item);
		} else {
			useItem(item,player);
		}
		
		// wooden shield
		item = newItem(WOODEN_SHIELD,SERVICABLE,0,1,1,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			hotbar[2].item = item2->uid;
			free(item);
		} else {
			useItem(item,player);
		}
		
		// leather helm
		item = newItem(LEATHER_HELM,WORN,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}
		
		if( player==clientnum ) {
			// iron mace
			item = newItem(IRON_MACE,SERVICABLE,0,1,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			hotbar[1].item = item2->uid;
			free(item);

			// bread
			item = newItem(FOOD_BREAD,SERVICABLE,0,2,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// meat
			item = newItem(FOOD_MEAT,SERVICABLE,0,1,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// torch
			item = newItem(TOOL_TORCH,SERVICABLE,0,2,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			hotbar[3].item = item2->uid;
			free(item);
			
			// booze
			item = newItem(POTION_BOOZE,SERVICABLE,0,1,1,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
		}
	}
	// warrior
	else if( client_classes[player] == 1 ) {
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

		// iron spear
		item = newItem(IRON_SPEAR,SERVICABLE,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			hotbar[0].item = item2->uid;
			free(item);
		} else {
			useItem(item,player);
		}
			
		// bronze shield
		item = newItem(BRONZE_SHIELD,SERVICABLE,0,1,1,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			hotbar[3].item = item2->uid;
			free(item);
		} else {
			useItem(item,player);
		}
		
		// leather helm
		item = newItem(LEATHER_HELM,WORN,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}
		
		// iron armor
		item = newItem(IRON_BREASTPIECE,SERVICABLE,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}
		
		// boots
		item = newItem(LEATHER_BOOTS,WORN,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}

		if( player==clientnum ) {
			// iron sword
			item = newItem(IRON_SWORD,SERVICABLE,0,1,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			hotbar[1].item = item2->uid;
			free(item);
			
			// shortbow
			item = newItem(SHORTBOW,EXCELLENT,0,1,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			hotbar[2].item = item2->uid;
			free(item);
			
			// bread
			item = newItem(FOOD_BREAD,SERVICABLE,0,1,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// fish
			item = newItem(FOOD_FISH,SERVICABLE,0,1,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
		}
	}
	// healer
	else if( client_classes[player] == 2 ) {
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

		// quarterstaff
		item = newItem(QUARTERSTAFF,SERVICABLE,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			hotbar[0].item = item2->uid;
			free(item);
		} else {
			useItem(item,player);
		}
		
		// phrygian hat
		item = newItem(HAT_PHRYGIAN,SERVICABLE,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}
		
		// cloak (red, protection)
		item = newItem(CLOAK,SERVICABLE,0,1,2,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}

		if( player==clientnum ) {
			// fish
			item = newItem(FOOD_FISH,EXCELLENT,0,1,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// extra healing potions
			item = newItem(POTION_EXTRAHEALING,EXCELLENT,0,3,6,TRUE,NULL);
			item2 = itemPickup(player,item);
			hotbar[2].item = item2->uid;
			free(item);
			
			// cure ailment spellbook
			item = newItem(SPELLBOOK_CUREAILMENT,EXCELLENT,0,1,1,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// healing spellbook
			item = newItem(SPELLBOOK_HEALING,EXCELLENT,0,1,2,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// apples
			item = newItem(FOOD_APPLE,SERVICABLE,0,2,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// towels
			item = newItem(TOOL_TOWEL,SERVICABLE,0,3,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// magicstaff of slow
			item = newItem(MAGICSTAFF_SLOW,SERVICABLE,0,1,3,TRUE,NULL);
			item2 = itemPickup(player,item);
			hotbar[1].item = item2->uid;
			free(item);
		}
	}
	// rogue
	else if( client_classes[player] == 3 ) {
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
		stats[player]->PROFICIENCIES[PRO_LOCKPICKING] = 50;
		stats[player]->PROFICIENCIES[PRO_RANGED] = 25;
		stats[player]->PROFICIENCIES[PRO_SWORD] = 25;

		// bronze sword
		item = newItem(BRONZE_SWORD,SERVICABLE,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);	
			hotbar[0].item = item2->uid;
			free(item);
		} else {
			useItem(item,player);
		}
		
		// cloak (green)
		item = newItem(CLOAK,WORN,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}
		
		// hood (green)
		item = newItem(HAT_HOOD,WORN,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}
		
		// gloves
		item = newItem(GLOVES,SERVICABLE,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}
		
		// leather breastpiece
		item = newItem(LEATHER_BREASTPIECE,DECREPIT,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}

		if( player==clientnum ) {
			// shortbow
			item = newItem(SHORTBOW,SERVICABLE,0,1,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			hotbar[1].item = item2->uid;
			free(item);
			
			// sickness potion
			item = newItem(POTION_SICKNESS,EXCELLENT,0,3,5,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// lockpicks
			item = newItem(TOOL_LOCKPICK,SERVICABLE,0,3,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			hotbar[2].item = item2->uid;
			free(item);
		}
	}
	// wanderer
	else if( client_classes[player] == 4 ) {
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

		// quarterstaff
		item = newItem(QUARTERSTAFF,WORN,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			hotbar[0].item = item2->uid;
			free(item);
		} else {
			useItem(item,player);
		}
			
		// phrygian hat
		item = newItem(HAT_PHRYGIAN,SERVICABLE,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}
		
		// cloak
		item = newItem(CLOAK,SERVICABLE,0,1,1,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}
		
		// boots
		item = newItem(LEATHER_BOOTS,SERVICABLE,1,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}

		if( player==clientnum ) {
			// crossbow
			item = newItem(CROSSBOW,WORN,0,1,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			hotbar[1].item = item2->uid;
			free(item);
			
			// tins
			item = newItem(FOOD_TIN,SERVICABLE,0,2,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// bread
			item = newItem(FOOD_BREAD,SERVICABLE,0,2,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// fish
			item = newItem(FOOD_FISH,EXCELLENT,0,1,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// tin opener
			item = newItem(TOOL_TINOPENER,SERVICABLE,0,1,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// towel
			item = newItem(TOOL_TOWEL,SERVICABLE,0,1,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// torches
			item = newItem(TOOL_TORCH,SERVICABLE,0,2,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			hotbar[2].item = item2->uid;
			free(item);
			
			// lantern
			item = newItem(TOOL_LANTERN,WORN,0,1,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			hotbar[3].item = item2->uid;
			free(item);
		}
	}
	// cleric
	else if( client_classes[player] == 5 ) {
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

		// iron mace
		item = newItem(IRON_MACE,SERVICABLE,1,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			hotbar[0].item = item2->uid;
			free(item);
		} else {
			useItem(item,player);
		}
		
		// wooden shield
		item = newItem(WOODEN_SHIELD,WORN,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			hotbar[1].item = item2->uid;
			free(item);
		} else {
			useItem(item,player);
		}
		
		// leather breastpiece
		item = newItem(LEATHER_BREASTPIECE,WORN,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}
		
		// cloak (purple)
		item = newItem(CLOAK,SERVICABLE,0,1,3,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}
		
		// gloves
		item = newItem(BRACERS,WORN,1,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}

		if( player==clientnum ) {
			// torch
			item = newItem(TOOL_TORCH,SERVICABLE,0,1,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			hotbar[2].item = item2->uid;
			free(item);

			// healing potions
			item = newItem(POTION_HEALING,EXCELLENT,0,2,7,TRUE,NULL);
			item2 = itemPickup(player,item);
			hotbar[3].item = item2->uid;
			free(item);

			// summon scrolls
			item = newItem(SCROLL_SUMMON,EXCELLENT,0,3,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			hotbar[4].item = item2->uid;
			free(item);
			
			// bread
			item = newItem(FOOD_BREAD,SERVICABLE,0,1,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
		}
	}
	// merchant
	else if( client_classes[player] == 6 ) {
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

		// bronze axe
		item = newItem(BRONZE_AXE,SERVICABLE,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			hotbar[0].item = item2->uid;
			free(item);
		} else {
			useItem(item,player);
		}
		
		// phrygian hat
		item = newItem(HAT_PHRYGIAN,SERVICABLE,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}
		
		// leather breastpiece
		item = newItem(LEATHER_BREASTPIECE,WORN,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}
			
		// glasses
		item = newItem(TOOL_GLASSES,SERVICABLE,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}

		if( player==clientnum ) {
			// pickaxe
			item = newItem(TOOL_PICKAXE,SERVICABLE,0,1,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			hotbar[1].item = item2->uid;
			free(item);

			// gloves
			item = newItem(GLOVES,EXCELLENT,0,1,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
			
			// scroll of remove curse
			item = newItem(SCROLL_REMOVECURSE,EXCELLENT,0,1,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// cheese
			item = newItem(FOOD_CHEESE,SERVICABLE,0,1,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// meat
			item = newItem(FOOD_MEAT,EXCELLENT,0,2,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
		}
	}
	// wizard
	else if( client_classes[player] == 7 ) {
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

		// quarterstaff
		item = newItem(QUARTERSTAFF,SERVICABLE,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			hotbar[0].item = item2->uid;
			free(item);
		} else {
			useItem(item,player);
		}

		// wizard hat
		item = newItem(HAT_WIZARD,SERVICABLE,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}

		// cloak (purple, protection)
		item = newItem(CLOAK_PROTECTION,SERVICABLE,0,1,3,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}

		// amulet of magic reflection
		item = newItem(AMULET_MAGICREFLECTION,EXCELLENT,0,1,1,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}

		// leather boots
		item = newItem(LEATHER_BOOTS,SERVICABLE,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}

		if( player==clientnum ) {
			// magicstaff of light
			item = newItem(MAGICSTAFF_LIGHT,EXCELLENT,0,1,3,TRUE,NULL);
			item2 = itemPickup(player,item);
			hotbar[1].item = item2->uid;
			free(item);

			// potion of restore magic
			item = newItem(POTION_RESTOREMAGIC,EXCELLENT,0,1,1,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);

			// spellbook of fireball
			item = newItem(SPELLBOOK_FIREBALL,SERVICABLE,0,1,3,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);

			// spellbook of cold
			item = newItem(SPELLBOOK_COLD,SERVICABLE,0,1,4,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// spellbook of light
			item = newItem(SPELLBOOK_LIGHT, SERVICABLE, 0, 1, 5, TRUE, NULL);
			item2 = itemPickup(player,item);
			free(item);
		}
	}
	// arcanist
	else if( client_classes[player] == 8 ) {
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
		item = newItem(IRON_SWORD,SERVICABLE,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			hotbar[0].item = item2->uid;
			free(item);
		} else {
			useItem(item,player);
		}
		
		// crossbow
		item = newItem(CROSSBOW,SERVICABLE,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			hotbar[1].item = item2->uid;
			free(item);
		} else {
			useItem(item,player);
		}
		
		// leather armor
		item = newItem(LEATHER_BREASTPIECE,SERVICABLE,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}
		
		// leather boots
		item = newItem(LEATHER_BOOTS,SERVICABLE,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}
		
		// cloak (red)
		item = newItem(CLOAK,WORN,0,1,2,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}
		
		// hood (red)
		item = newItem(HAT_HOOD,WORN,0,1,2,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}

		if( player==clientnum ) {
			// magicstaff of opening
			item = newItem(MAGICSTAFF_OPENING,SERVICABLE,0,1,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			hotbar[2].item = item2->uid;
			free(item);
			
			// spellbook of forcebolt
			item = newItem(SPELLBOOK_FORCEBOLT,WORN,0,1,6,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// spellbook of light
			item = newItem(SPELLBOOK_LIGHT,WORN,0,1,7,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
		}
	}
	// joker
	else if( client_classes[player] == 9 ) {
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

		// jester hat
		item = newItem(HAT_JESTER,SERVICABLE,0,1,0,TRUE,NULL);
		if( player==clientnum ) {
			item2 = itemPickup(player,item);
			useItem(item2,player);
			free(item);
		} else {
			useItem(item,player);
		}
		
		if( player==clientnum ) {
			// slingshot
			item = newItem(SLING,SERVICABLE,0,1,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			hotbar[0].item = item2->uid;
			free(item);

			// lockpicks
			item = newItem(TOOL_LOCKPICK,EXCELLENT,0,1,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			hotbar[1].item = item2->uid;
			free(item);

			// scroll of teleportation
			item = newItem(SCROLL_TELEPORTATION,EXCELLENT,0,3,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// scroll of food
			item = newItem(SCROLL_FOOD,EXCELLENT,0,1,0,FALSE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// ring of levitation
			item = newItem(RING_LEVITATION,SERVICABLE,0,1,2,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// spellbook of confuse
			item = newItem(SPELLBOOK_CONFUSE,WORN,0,1,8,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// blindfold
			item = newItem(TOOL_BLINDFOLD,SERVICABLE,0,1,2,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// luckstone
			item = newItem(GEM_LUCK,EXCELLENT,0,1,1,FALSE,NULL);
			item2 = itemPickup(player,item);
			free(item);
			
			// fish
			item = newItem(FOOD_FISH,SERVICABLE,0,2,0,TRUE,NULL);
			item2 = itemPickup(player,item);
			free(item);
		}
	}

	// move default items to the right
	if( player==clientnum ) {
		node_t *node;
		for( node=stats[player]->inventory.first; node!=NULL; node=node->next ) {
			Item *item = (Item *)node->element;
			item->x = INVENTORY_SIZEX-item->x-1;
		}
	}

	//spellList = malloc(sizeof(list_t));
	spellList.first = NULL; spellList.last = NULL;
}
