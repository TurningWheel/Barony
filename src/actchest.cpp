/*-------------------------------------------------------------------------------

	BARONY
	File: actchest.cpp
	Desc: implements all chest related code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "interface/interface.hpp"
#include "items.hpp"
#include "engine/audio/sound.hpp"
#include "net.hpp"
#include "player.hpp"
#include "prng.hpp"
#include "mod_tools.hpp"

/*
 * Chest theme ideas:
"random"
"empty / tiny amount of worthless garbage"
"food"
"treasures, jewelry, gems"
"weapons / armor"
"tools"
"spellbooks / scrolls"
"magicstaffs"
"potions"
 */

//chest->children->first is the chest's inventory.

void actChest(Entity* my)
{
	if ( !my )
	{
		return;
	}

	my->actChest();
}

void createChestInventory(Entity* my, int chestType)
{
	if ( !my )
	{
		return;
	}
	if ( my->behavior != &::actChest && my->behavior != &actMonster )
	{
		return;
	}
	auto& rng = my->entity_rng ? *my->entity_rng : local_rng;

	list_t* inventory = nullptr;
	if ( my->behavior == &actMonster )
	{
		if ( Stat* myStats = my->getStats() )
		{
			inventory = &myStats->inventory;
		}
	}
	else
	{
		node_t* node = NULL;
		node = list_AddNodeFirst(&my->children);
		node->element = malloc(sizeof(list_t)); //Allocate memory for the inventory list.
		node->deconstructor = &listDeconstructor;
		inventory = (list_t*)node->element;
		inventory->first = NULL;
		inventory->last = NULL;
	}

	if ( !inventory )
	{
		return;
	}

	int itemcount = 0;
	int i = 0;

	int chesttype = 0;

	if ( chestType > 0 ) //If chest spawned by editor sprite, manually set the chest content category. Otherwise this value should be 0 (random).
	{
		chesttype = chestType; //Value between 0 and 7.
	}
	else
	{
		if ( strcmp(map.name, "The Mystic Library") )
		{
			chesttype = rng.rand() % 8;
			if ( chesttype == 1 )
			{
				if ( currentlevel > 10 )
				{
					// re-roll the garbage chest.
					while ( chesttype == 1 )
					{
						chesttype = rng.rand() % 8;
					}
				}
				else
				{
					// re-roll the garbage chest 50% chance
					if ( rng.rand() % 2 == 0 )
					{
						chesttype = rng.rand() % 8;
					}
				}
			}
		}
		else
		{
			chesttype = 6; // magic chest			
		}
	}

	int minimumQuality = 0;
	if ( currentlevel >= 32 )
	{
		minimumQuality = 10;
	}
	else if ( currentlevel >= 18 )
	{
		minimumQuality = 5;
	}

	if ( my->chestHasVampireBook && my->behavior == &::actChest )
	{
		newItem(SPELLBOOK_VAMPIRIC_AURA, EXCELLENT, 0, 1, rng.rand(), true, inventory);
	}

	switch ( chesttype )   //Note that all of this needs to be properly balanced over time.
	{
		//TODO: Make all applicable item additions work on a category based search?
	case 0:
		//Completely random.
		itemcount = (rng.rand() % 5) + 1;
		for ( i = 0; i < itemcount; ++i )
		{
			//And add the current entity to it.
			//int itemnum = rng.rand() % NUMITEMS;
			//while (itemnum == SPELL_ITEM || (items[itemnum].level == -1) || items[itemnum].level > currentlevel + 5 )
			//{
			//	//messagePlayer(0, "Skipping item %d, level %d", itemnum, items[itemnum].level);
			//	itemnum = rng.rand() % NUMITEMS;    //Keep trying until you don't get a spell or invalid item.
			//}
			//newItem(static_cast<ItemType>(itemnum), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			int cat = rng.rand() % (NUMCATEGORIES - 1); // exclude spell_cat
			Item* currentItem = newItem(itemLevelCurve(static_cast<Category>(cat), 0, currentlevel + 5, rng), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			if ( currentItem )
			{
				if ( currentItem->type >= BRONZE_TOMAHAWK && currentItem->type <= CRYSTAL_SHURIKEN )
				{
					// thrown weapons always fixed status. (tomahawk = decrepit, shuriken = excellent)
					currentItem->status = std::min(static_cast<Status>(DECREPIT + (currentItem->type - BRONZE_TOMAHAWK)), EXCELLENT);
				}
			}
		}
		break;
	case 1:
		//Garbage chest
		if ( rng.rand() % 2 )
		{
			//Empty.
		}
		else
		{
			//Some worthless garbage. Like a rock. //TODO: Sometimes spawn item 139, worthless piece of glass. Maybe go a step further and have a random amount of items, say 1 - 5, and they can be either rock or the worthless piece of glass or any other garbage.
			itemcount = (rng.rand() % 3) + 1;
			int itemStatus = WORN + rng.rand() % 3;
			for ( i = 0; i < itemcount; ++i )
			{
				if ( rng.rand() % 20 == 0 )
				{
					newItem(MASK_MOUTH_ROSE, static_cast<Status>(itemStatus), -1 + rng.rand() % 3, 1, rng.rand(), false, inventory);
				}
				else
				{
					newItem(GEM_ROCK, static_cast<Status>(itemStatus), 0, 1, rng.rand(), false, inventory);
				}
			}
		}
		break;
	case 2:
		//Food.
		//Items 152 - 158 are all food.
		itemcount = (rng.rand() % 5) + 1;
		for ( i = 0; i < itemcount; ++i )
		{
			//newItem(static_cast<ItemType>(FOOD_BREAD + (rng.rand() % 7)), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			newItem(itemLevelCurve(FOOD, 0, currentlevel + 5, rng), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
		}
		if ( rng.rand() % 10 == 0 )
		{
			newItem(HAT_CHEF, static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
		}
		else if ( rng.rand() % 20 == 0 )
		{
			newItem(MASK_GRASS_SPRIG, static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
		}
		break;
	case 3:
		//Treasures, jewelry, gems 'n stuff.
		itemcount = (rng.rand() % 5) + 1;
		for ( i = 0; i < itemcount; ++i )
		{
			if ( rng.rand() % 4 )
			{
				newItem(static_cast<ItemType>(GEM_GARNET + rng.rand() % 15), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			}
			else
			{
				newItem(GEM_GLASS, static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			}
		}
		//Random chance to spawn a ring or an amulet or some other jewelry.
		if ( rng.rand() % 2 )
		{
			if ( rng.rand() % 2 )
			{
				//Spawn a ring.
				//newItem(static_cast<ItemType>(RING_ADORNMENT + rng.rand() % 12), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
				newItem(itemLevelCurve(RING, 0, currentlevel + 5, rng), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			}
			else
			{
				//Spawn an amulet.
				//newItem(static_cast<ItemType>(AMULET_SEXCHANGE + rng.rand() % 6), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
				newItem(itemLevelCurve(AMULET, 0, currentlevel + 5, rng), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			}
		}
		break;
	case 4:
		//Weapons, armor, stuff.
		//Further break this down into either spawning only weapon(s), only armor(s), or a combo, like a set.

		switch ( rng.rand() % 3 )   //TODO: Note, switch to rng.rand()%4 if/when case 3 is implemented.
		{
		case 0:
			//Only a weapon. Items 0 - 16.
		{
			//int item = rng.rand() % 18;
			////Since the weapons are not a continuous set, check to see if the weapon is part of the continuous set. If it is not, move on to the next block. In this case, there's only one weapon that is not part of the continous set: the crossbow.
			//if (item < 16)
			//	//Almost every weapon.
			//{
			//	newItem(static_cast<ItemType>(rng.rand() % 17), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			//}
			//else
			//	//Crossbow.
			//{
			//	newItem(CROSSBOW, static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			//}
			newItem(itemLevelCurve(WEAPON, minimumQuality, currentlevel + 5, rng), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
		}
		break;
		case 1:
			//Only a piece of armor.
		{
			/*
			 * 0 - 1 are the steel shields, items 17 and 18.
			 * 2 - 5 are the gauntlets, items 20 - 23.
			 * 6 - 15 are the boots & shirts (as in, breastplates and all variants), items 28 - 37.
			 * 16 - 19 are the hats & helmets, items 40 - 43
			 */
			 //int item = rng.rand() % 15;
			 //if (item <= 1)
			 //	//Steel shields. Items 17 & 18.
			 //{
			 //	newItem(static_cast<ItemType>(17 + rng.rand() % 2), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			 //}
			 //else if (item <= 5)
			 //	//Gauntlets. Items 20 - 23.
			 //{
			 //	if ( rng.rand() % 3 > 0 )
			 //	{
			 //		newItem(static_cast<ItemType>(20 + rng.rand() % 4), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			 //	}
			 //	else
			 //	{
			 //		// new gauntlets
			 //		newItem(static_cast<ItemType>(BRASS_KNUCKLES + rng.rand() % 3), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			 //	}
			 //}
			 //else if (item <= 10)
			 //	//Hats & helmets. Items 40 - 43.
			 //{
			 //	newItem(static_cast<ItemType>(40 + rng.rand() % 4), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			 //}
			 //else if (item <= 15)
			 //	//Boots & shirts. Items 28 - 37.
			 //{
			 //	newItem(static_cast<ItemType>(28 + rng.rand() % 10), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			 //}
			newItem(itemLevelCurve(ARMOR, minimumQuality, currentlevel + 5, rng), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
		}
		break;
		case 2:
			//A weapon and an armor, chance of thrown.
		{
			//int item = rng.rand() % 18;
			////Since the weapons are not a continuous set, check to see if the weapon is part of the continuous set. If it is not, move on to the next block. In this case, there's only one weapon that is not part of the continous set: the crossbow.
			//if (item < 16)
			//	//Almost every weapon.
			//{
			//	newItem(static_cast<ItemType>(rng.rand() % 17), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			//}
			//else
			//	//Crossbow.
			//{
			//	newItem(static_cast<ItemType>(19), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			//}

			///*
			// * 0 - 1 are the steel shields, items 17 and 18.
			// * 2 - 5 are the gauntlets, items 20 - 23.
			// * 6 - 15 are the boots & shirts (as in, breastplates and all variants), items 28 - 37.
			// * 16 - 19 are the hats & helmets, items 40 - 43
			// */
			//item = rng.rand() % 20;
			//if (item <= 1)
			//	//Steel shields. Items 17 & 18.
			//{
			//	newItem(static_cast<ItemType>(17 + rng.rand() % 2), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			//}
			//else if (item <= 5)
			//	//Gauntlets. Items 20 - 23.
			//{
			//	if ( rng.rand() % 3 > 0 )
			//	{
			//		newItem(static_cast<ItemType>(20 + rng.rand() % 4), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			//	}
			//	else
			//	{
			//		// new gauntlets
			//		newItem(static_cast<ItemType>(BRASS_KNUCKLES + rng.rand() % 3), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			//	}
			//}
			//else if (item <= 10)
			//	//Hats & helmets. Items 40 - 43.
			//{
			//	newItem(static_cast<ItemType>(40 + rng.rand() % 4), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			//}
			//else if (item <= 15)
			//	//Boots & shirts. Items 28 - 37.
			//{
			//	newItem(static_cast<ItemType>(28 + rng.rand() % 10), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			//}

			newItem(itemLevelCurve(WEAPON, minimumQuality, currentlevel + 5, rng), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			newItem(itemLevelCurve(ARMOR, minimumQuality, currentlevel + 5, rng), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);

			// try for thrown items.
			itemcount = 0 + rng.rand() % 2;
			for ( i = 0; i < itemcount; ++i )
			{
				Item* thrown = newItem(itemLevelCurve(THROWN, minimumQuality, currentlevel, rng), WORN, 0, 3 + rng.rand() % 3, rng.rand(), false, inventory);
				if ( thrown )
				{
					if ( thrown->type >= BRONZE_TOMAHAWK && thrown->type <= CRYSTAL_SHURIKEN )
					{
						// thrown weapons always fixed status. (tomahawk = decrepit, shuriken = excellent)
						thrown->status = std::min(static_cast<Status>(DECREPIT + (thrown->type - BRONZE_TOMAHAWK)), EXCELLENT);
					}
				}
			}
		}
		break;
		case 3:
			//TODO: Rarer. Getting a full set of armor + a weapon.
			break;
		}
		break;
	case 5:
	{
		//Tools.
		Status durability = static_cast<Status>(WORN + rng.rand() % 3);
		switch ( rng.rand() % 3 )
		{
		case 0:
			itemcount = rng.rand() % 3;
			for ( i = 0; i < itemcount; ++i )
			{
				newItem(TOOL_BEARTRAP, durability, 0, 1 + rng.rand() % 3, rng.rand(), false, inventory);
			}
			// fall through
		case 1:
			itemcount = 1 + rng.rand() % 2;
			for ( i = 0; i < itemcount; ++i )
			{
				newItem(static_cast<ItemType>(TOOL_PICKAXE + rng.rand() % 12), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			}
			if ( rng.rand() % 20 == 0 )
			{
				newItem(CLOAK_BACKPACK, durability, 0, 1, rng.rand(), false, inventory);
			}
			if ( rng.rand() % 20 == 0 )
			{
				newItem(TOOL_TINKERING_KIT, DECREPIT, 0, 1, rng.rand(), false, inventory);
				newItem(TOOL_METAL_SCRAP, DECREPIT, 0, 10 + rng.rand() % 11, 0, true, inventory);
				newItem(TOOL_MAGIC_SCRAP, DECREPIT, 0, 10 + rng.rand() % 11, 0, true, inventory);
			}
			else if ( rng.rand() % 20 == 0 )
			{
				newItem(MASK_MOUTHKNIFE, durability, 0, 1, rng.rand(), false, inventory);
			}
			break;
		case 2:
			itemcount = 1 + rng.rand() % 2;
			for ( i = 0; i < itemcount; ++i )
			{
				Item* thrown = newItem(itemLevelCurve(THROWN, minimumQuality, currentlevel, rng), WORN, 0, 3 + rng.rand() % 3, rng.rand(), false, inventory);
				if ( thrown )
				{
					if ( thrown->type >= BRONZE_TOMAHAWK && thrown->type <= CRYSTAL_SHURIKEN )
					{
						// thrown weapons always fixed status. (tomahawk = decrepit, shuriken = excellent)
						thrown->status = std::min(static_cast<Status>(DECREPIT + (thrown->type - BRONZE_TOMAHAWK)), EXCELLENT);
					}
				}
			}
			break;
		default:
			break;
		}
		break;
	}
	case 6:
		//Magic chest.
		//So first choose what kind of magic chest it is.
	{
		/*
		 * Types:
		 * * Scroll chest. Has some scrolls in it ( 3 - 5).
		 * * Book chest. Basically a small library. 1-3 books.
		 * * Staff chest. Staff or 2.
		 * * Wizard's chest, which will contain 1-2 scrolls, a magic book, a staff, and either a wizard/magician/whatever implement of some sort or a piece of armor.
		 */
		int magic_type = rng.rand() % 4;

		switch ( magic_type )
		{
		case 0:
			//Have 3-5 scrolls.
			itemcount = 3 + (rng.rand() % 3);
			for ( i = 0; i < itemcount; ++i )
			{
				//newItem(static_cast<ItemType>(SCROLL_IDENTIFY + rng.rand() % 12), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
				newItem(itemLevelCurve(SCROLL, 0, currentlevel + 5, rng), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			}
			if ( rng.rand() % 10 == 0 )
			{
				if ( rng.rand() % 5 == 0 )
				{
					newItem(ENCHANTED_FEATHER, EXCELLENT, 0, 1, ENCHANTED_FEATHER_MAX_DURABILITY - 1, false, inventory);
				}
				else
				{
					newItem(ENCHANTED_FEATHER, SERVICABLE, 0, 1, (3 * (ENCHANTED_FEATHER_MAX_DURABILITY - 1)) / 4, false, inventory);
				}
				if ( rng.rand() % 2 == 0 )
				{
					newItem(SCROLL_BLANK, static_cast<Status>(WORN + rng.rand() % 3), 0, 1 + rng.rand() % 3, rng.rand(), false, inventory);
				}
			}
			break;
		case 1:
			//Have 1-3 books.
			itemcount = 1 + (rng.rand() % 3);
			for ( i = 0; i < itemcount; ++i )
			{
				//newItem(static_cast<ItemType>(SPELLBOOK_FORCEBOLT + rng.rand() % 22), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
				newItem(itemLevelCurve(SPELLBOOK, 0, currentlevel + 6, rng), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			}
			break;
		case 2:
			//A staff.
			//newItem(static_cast<ItemType>(MAGICSTAFF_LIGHT + rng.rand() % 10), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			newItem(itemLevelCurve(MAGICSTAFF, 0, currentlevel + 5, rng), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			break;
		case 3:
			//So spawn several items at once. A wizard's chest!

			//First the scrolls (1 - 2).
			itemcount = 1 + rng.rand() % 2;
			for ( i = 0; i < itemcount; ++i )
			{
				//newItem(static_cast<ItemType>(SCROLL_IDENTIFY + rng.rand() % 12), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
				newItem(itemLevelCurve(SCROLL, 0, currentlevel + 5, rng), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			}

			//newItem(static_cast<ItemType>(SPELLBOOK_FORCEBOLT + rng.rand() % 22), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			newItem(itemLevelCurve(SPELLBOOK, 0, currentlevel + 6, rng), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			//newItem(static_cast<ItemType>(MAGICSTAFF_LIGHT + rng.rand() % 10), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			newItem(itemLevelCurve(MAGICSTAFF, 0, currentlevel + 5, rng), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			switch ( rng.rand() % 9 )
			{
			case 0:
				//A cloak. Item 24.
				newItem(CLOAK, static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
				break;
			case 1:
				//A cloak of magic resistance. Item 25.
				newItem(CLOAK_MAGICREFLECTION, static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
				break;
			case 2:
				//A cloak of invisibility. Item 26.
				newItem(CLOAK_INVISIBILITY, static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
				break;
			case 3:
				//A cloak of protection. Item 27.
				newItem(CLOAK_PROTECTION, static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
				break;
			case 4:
				//A phyregian's hat/fez hat. Item 38.
				if ( rng.rand() % 5 == 0 )
				{
					newItem(HAT_FEZ, static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
				}
				else
				{
					newItem(HAT_PHRYGIAN, static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
				}
				break;
			case 5:
				//A wizard's hat. Item 39.
				newItem(HAT_WIZARD, static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
				break;
			case 6:
				newItem(ENCHANTED_FEATHER, EXCELLENT, 0, 1, ENCHANTED_FEATHER_MAX_DURABILITY - 1, false, inventory);
				if ( rng.rand() % 2 == 0 )
				{
					newItem(SCROLL_BLANK, static_cast<Status>(WORN + rng.rand() % 3), 0, 1 + rng.rand() % 3, rng.rand(), false, inventory);
				}
				break;
			case 7:
				newItem(HAT_MITER, static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
				break;
			case 8:
				newItem(HAT_HEADDRESS, static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
				break;
			default:
				break;
			}
			break;
		}
	}
	break;
	case 7:
		//Potions.
		//Items 50 - 64 are potions.
		itemcount = (rng.rand() % 3) + 1;
		for ( i = 0; i < itemcount; ++i )
		{
			//newItem(static_cast<ItemType>(POTION_WATER + (rng.rand() % 15)), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
			newItem(itemLevelCurve(POTION, 0, currentlevel + 7, rng), static_cast<Status>(WORN + rng.rand() % 3), 0, 1, rng.rand(), false, inventory);
		}
		if ( rng.rand() % 8 == 0 )
		{
			newItem(TOOL_ALEMBIC, static_cast<Status>(WORN + rng.rand() % 3), -1 + rng.rand() % 3, 1, rng.rand(), false, inventory);
			newItem(POTION_EMPTY, SERVICABLE, 0, 2 + rng.rand() % 3, 0, true, inventory);
		}
		else if ( rng.rand() % 16 == 0 )
		{
			newItem(TOOL_ALEMBIC, static_cast<Status>(WORN + rng.rand() % 3), -1 + rng.rand() % 3, 1, rng.rand(), false, inventory);
		}
		if ( rng.rand() % 4 == 0 )
		{
			newItem(POTION_EMPTY, SERVICABLE, 0, 1 + rng.rand() % 3, 0, true, inventory);
		}
		if ( rng.rand() % 10 == 0 )
		{
			newItem(MASK_HAZARD_GOGGLES, static_cast<Status>(WORN + rng.rand() % 3), 
				0, 1, rng.rand(), false, inventory);
		}
		else if ( rng.rand() % 20 == 0 )
		{
			newItem(MASK_PLAGUE, static_cast<Status>(WORN + rng.rand() % 3),
				0, 1, rng.rand(), false, inventory);
		}
		break;
	case 8:
		break;
	default:
		//Default case. Should never be reached.
		newItem(static_cast<ItemType>(0), BROKEN, 0, 1, rng.rand(), false, inventory);
		printlog("warning: default cause in chest init theme type reached. This should never happen.");
		break;
	}

	if ( my->behavior == &::actChest )
	{
		// sort items into slots
		int slotx = 0;
		int sloty = 0;
		node_t* nextnode;
		for ( node_t* node = inventory->first; node != NULL; node = nextnode )
		{
			nextnode = node->next;
			Item* item = (Item*)node->element;
			if ( !item ) { continue; }

			item->x = slotx;
			item->y = sloty;
			++slotx;
			if ( slotx >= Player::Inventory_t::MAX_CHEST_X )
			{
				slotx = 0;
				++sloty;
			}
		}
	}
}

void Entity::actChest()
{
#ifdef USE_FMOD
	if ( chestAmbience == 0 )
	{
		chestAmbience--;
		stopEntitySound();
		entity_sound = playSoundEntityLocal(this, 149, 32);
	}
	if ( entity_sound )
	{
		bool playing = false;
		entity_sound->isPlaying(&playing);
		if ( !playing )
		{
			entity_sound = nullptr;
		}
	}
#else
	chestAmbience--;
	if ( chestAmbience <= 0 )
	{
		chestAmbience = TICKS_PER_SECOND * 30;
		playSoundEntityLocal(this, 149, 32);
	}
#endif

	if ( ticks == 1 )
	{
		this->createWorldUITooltip();
	}

	if ( multiplayer == CLIENT )
	{
		if ( chestHasVampireBook )
		{
			spawnAmbientParticles(40, 600, 20 + local_rng.rand() % 30, 0.5, true);
			spawnAmbientParticles(40, 600, 20 + local_rng.rand() % 30, 0.5, true);
		}
		return;
	}

	if (!chestInit)
	{
		auto& rng = entity_rng ? *entity_rng : local_rng;
		chestInit = 1;
		chestHealth = 90 + rng.rand() % 20;
		chestMaxHealth = chestHealth;
		chestOldHealth = chestHealth;
		chestPreventLockpickCapstoneExploit = 1;
		chestLockpickHealth = 40;
		int roll = 0;

		if ( chestLocked == -1 )
		{
			roll = rng.rand() % 10;
			if ( roll == 0 )   // 10% chance //TODO: This should be weighted, depending on chest type.
			{
				chestLocked = 1;
				chestPreventLockpickCapstoneExploit = 0;
			}
			else
			{
				chestLocked = 0;
			}
			//messagePlayer(0, "Chest rolled: %d, locked: %d", roll, chestLocked); //debug print
		}
		else  if ( chestLocked >= 0 )
		{
			roll = rng.rand() % 100;
			if ( roll < chestLocked )
			{
				chestLocked = 1;
				chestPreventLockpickCapstoneExploit = 0;
			}
			else
			{
				chestLocked = 0;
			}

			//messagePlayer(0, "Chest rolled: %d, locked: %d", roll, chestLocked); //debug print
		}
	}

	list_t* inventory = static_cast<list_t* >(children.first->element);
	node_t* node = NULL;
	Item* item = NULL;

	chestOldHealth = chestHealth;

	if ( chestHealth <= 0 )
	{
		auto& rng = entity_rng ? *entity_rng : local_rng;

		// the chest busts open, drops some items randomly, then destroys itself.
		node_t* nextnode;
		for ( node = inventory->first; node != NULL; node = nextnode )
		{
			nextnode = node->next;
			item = (Item*)node->element;
			if ( rng.rand() % 2 == 0 )
			{
				dropItemMonster(item, this, NULL);
			}
		}

		// wood chunk particles
		int c;
		for ( c = 0; c < 10; c++ )
		{
			Entity* entity = spawnGib(this);
			entity->flags[INVISIBLE] = false;
			entity->sprite = 187; // Splinter.vox
			entity->x = floor(x / 16) * 16 + 8;
			entity->y = floor(y / 16) * 16 + 8;
			entity->z = -7 + local_rng.rand() % 14;
			entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
			entity->pitch = (local_rng.rand() % 360) * PI / 180.0;
			entity->roll = (local_rng.rand() % 360) * PI / 180.0;
			entity->vel_x = cos(entity->yaw) * (0.5 + (local_rng.rand() % 100) / 100.f);
			entity->vel_y = sin(entity->yaw) * (0.5 + (local_rng.rand() % 100) / 100.f);
			entity->vel_z = -.25;
			entity->fskill[3] = 0.04;
			serverSpawnGibForClient(entity);
		}
		playSoundEntity(this, 177, 64);

		if ( chestStatus == 1 )
		{
			messagePlayer(chestOpener, MESSAGE_WORLD, Language::get(671)); // "The chest is smashed into pieces!" only notify if chest is currently open.
		}

		this->closeChest();

		// remove chest entities
		Entity* parentEntity = uidToEntity(parent);
		if ( parentEntity )
		{
			list_RemoveNode(parentEntity->mynode);    // remove lid
		}
		list_RemoveNode(mynode); // remove me
		return;
	}
	else
	{
		if ( multiplayer != CLIENT && chestHasVampireBook )
		{
			node = inventory->first;
			if ( node )
			{
				item = (Item*)node->element;
				if ( item )
				{
					if ( item->type == SPELLBOOK_VAMPIRIC_AURA )
					{
						spawnAmbientParticles(40, 600, 20 + local_rng.rand() % 30, 0.5, true);
					}
					else
					{
						chestHasVampireBook = 0;
						serverUpdateEntitySkill(this, 11);
					}
				}
			}
		}
		if ( chestHasVampireBook )
		{
			spawnAmbientParticles(40, 600, 20 + local_rng.rand() % 30, 0.5, true);
		}
	}

	if ( chestStatus == 1 )
	{
		if ( players[chestOpener] && players[chestOpener]->entity )
		{
			unsigned int distance = sqrt(pow(x - players[chestOpener]->entity->x, 2) + pow(y - players[chestOpener]->entity->y, 2));
			if (distance > TOUCHRANGE)
			{
				closeChest();
			}
		}
		else
		{
			closeChest();
		}
	}

	//Using the chest (TODO: Monsters using it?).
	int chestclicked = -1;
	for (int i = 0; i < MAXPLAYERS; ++i)
	{
		if ( selectedEntity[i] == this || client_selected[i] == this )
		{
			if (inrange[i])
			{
				chestclicked = i;
			}
		}
	}
	if ( chestLidClicked )
	{
		chestclicked = chestLidClicked - 1;
		chestLidClicked = 0;
	}
	if ( chestclicked >= 0 )
	{
		if ( !chestLocked && !openedChest[chestclicked] )
		{
			if ( !chestStatus )
			{
				messagePlayer(chestclicked, MESSAGE_INTERACTION, Language::get(459));
				openedChest[chestclicked] = this;

				Compendium_t::Events_t::eventUpdateWorld(chestclicked, Compendium_t::CPDM_CHESTS_OPENED, "chest", 1);

				chestOpener = chestclicked;
				if ( !players[chestclicked]->isLocalPlayer() && multiplayer == SERVER)
				{
					//Send all of the items to the client.
					strcpy((char*)net_packet->data, "CHST");  //Chest.
					SDLNet_Write32((Uint32)getUID(), &net_packet->data[4]); //Give the client the UID.
					net_packet->address.host = net_clients[chestclicked - 1].host;
					net_packet->address.port = net_clients[chestclicked - 1].port;
					net_packet->len = 8;
					sendPacketSafe(net_sock, -1, net_packet, chestclicked - 1);
					for (node = inventory->first; node != NULL; node = node->next)
					{
						item = (Item*) node->element;
						strcpy((char*)net_packet->data, "CITM");  //Chest item.
						SDLNet_Write32((Uint32)item->type, &net_packet->data[4]);
						SDLNet_Write32((Uint32)item->status, &net_packet->data[8]);
						SDLNet_Write32((Uint32)item->beatitude, &net_packet->data[12]);
						SDLNet_Write32((Uint32)item->count, &net_packet->data[16]);
						SDLNet_Write32((Uint32)item->appearance, &net_packet->data[20]);
						net_packet->data[24] = item->identified;
						net_packet->data[25] = 1; //forceNewStack ? 1 : 0;
						net_packet->data[26] = (Sint8)item->x;
						net_packet->data[27] = (Sint8)item->y;
						net_packet->address.host = net_clients[chestclicked - 1].host;
						net_packet->address.port = net_clients[chestclicked - 1].port;
						net_packet->len = 28;
						sendPacketSafe(net_sock, -1, net_packet, chestclicked - 1);
					}
				}
				else
				{
					players[chestclicked]->openStatusScreen(GUI_MODE_INVENTORY, INVENTORY_MODE_ITEM); // Reset the GUI to the inventory.
					players[chestclicked]->GUI.activateModule(Player::GUI_t::MODULE_CHEST);
					players[chestclicked]->inventoryUI.chestGUI.openChest();
				}
				chestStatus = 1; //Toggle chest open/closed.
			}
			else
			{
				messagePlayer(chestclicked, MESSAGE_INTERACTION, Language::get(460)); // slam the chest shut
				if ( !players[chestOpener]->isLocalPlayer() )
				{
					strcpy((char*)net_packet->data, "CCLS");  //Chest close.
					net_packet->address.host = net_clients[chestOpener - 1].host;
					net_packet->address.port = net_clients[chestOpener - 1].port;
					net_packet->len = 4;
					sendPacketSafe(net_sock, -1, net_packet, chestOpener - 1);
				}
				if (chestOpener != chestclicked)
				{
					messagePlayer(chestOpener, MESSAGE_HINT, Language::get(461));
				}
				closeChestServer();
			}
		}
		else if ( !chestLocked && chestStatus && openedChest[chestclicked] && chestOpener == chestclicked )
		{
			messagePlayer(chestclicked, MESSAGE_INTERACTION, Language::get(460)); // slam the chest shut
			if ( !players[chestOpener]->isLocalPlayer() )
			{
				strcpy((char*)net_packet->data, "CCLS");  //Chest close.
				net_packet->address.host = net_clients[chestOpener - 1].host;
				net_packet->address.port = net_clients[chestOpener - 1].port;
				net_packet->len = 4;
				sendPacketSafe(net_sock, -1, net_packet, chestOpener - 1);
			}
			closeChestServer();
		}
		else if ( chestLocked )
		{
			messagePlayer(chestclicked, MESSAGE_INTERACTION, Language::get(462));
			playSoundEntity(this, 152, 64);
		}
	}
}

void actChestLid(Entity* my)
{
	int i;

	Entity* parent = uidToEntity(my->parent);
	if ( !parent )
	{
		list_RemoveNode(my->mynode);
		return;
	}

	if ( multiplayer != CLIENT )
	{
		my->skill[1] = parent->skill[1];
		if ( multiplayer == SERVER )
		{
			if ( my->skill[3] != my->skill[1] )
			{
				my->skill[3] = my->skill[1];
				serverUpdateEntitySkill(my, 1);
			}
		}

		for (i = 0; i < MAXPLAYERS; ++i)
		{
			if ( selectedEntity[i] == my || client_selected[i] == my )
			{
				if (inrange[i])
				{
					parent->skill[6] = i + 1;
				}
			}
		}
	}

	if ( my->skill[1] )
	{
		// chest is open
		if ( !my->skill[0] )
		{
			my->skill[0] = 1;
			if ( multiplayer != CLIENT )
			{
				playSoundEntity(my, 21, 64);
			}
			my->fskill[0] = 0.25;
		}
		if ( my->pitch > -PI / 2 )
		{
			my->pitch -= my->fskill[0];
			my->fskill[0] -= 0.02;
			if ( my->pitch <= -PI / 2 )
			{
				my->pitch = -PI / 2;
				my->fskill[0] = 0;
			}
		}
	}
	else
	{
		// chest is closed
		if ( my->skill[0] )
		{
			my->skill[0] = 0;
			if ( multiplayer != CLIENT )
			{
				playSoundEntity(my, 22, 64);
			}
			my->fskill[0] = 0.025;
		}
		if ( my->pitch < 0 )
		{
			my->pitch += my->fskill[0];
			my->fskill[0] += 0.025;
			if ( my->pitch >= 0 )
			{
				my->pitch = 0;
				my->fskill[0] = 0;
			}
		}
	}
}

int getChestOpenerFromEntity(const Entity& chest)
{
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( openedChest[i] == &chest )
		{
			return i;
		}
	}
	return clientnum;
}

void Entity::closeChest()
{
	int player = getChestOpenerFromEntity(*this);

	if ( players[player]->isLocalPlayer() && multiplayer == CLIENT)
	{
		//If client, tell server the chest got closed.
		if (openedChest[player] != NULL)
		{
			//Message server.
			if ( chestHealth > 0 )
			{
				messagePlayer(player, MESSAGE_INTERACTION, Language::get(460));
			}
			
			strcpy( (char*)net_packet->data, "CCLS" );
			net_packet->data[4] = player;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 5;
			sendPacketSafe(net_sock, -1, net_packet, 0);

			closeChestClientside(player);
			return;
		}
	}

	if (chestStatus)
	{
		chestStatus = 0;

		if ( chestHealth > 0 )
		{
			messagePlayer(player, MESSAGE_INTERACTION, Language::get(460));
		}

		openedChest[chestOpener] = nullptr;
		if ( !players[chestOpener]->isLocalPlayer() && multiplayer == SERVER)
		{
			//Tell the client that the chest got closed.
			strcpy((char*)net_packet->data, "CCLS");  //Chest close.
			net_packet->address.host = net_clients[chestOpener - 1].host;
			net_packet->address.port = net_clients[chestOpener - 1].port;
			net_packet->len = 4;
			sendPacketSafe(net_sock, -1, net_packet, chestOpener - 1);
		}
		else
		{
			//Reset chest-gamepad related stuff here.
			players[chestOpener]->inventoryUI.chestGUI.closeChest();
		}
	}
}

void Entity::closeChestServer()
{
	if (chestStatus)
	{
		chestStatus = 0;
		openedChest[chestOpener] = NULL;
		players[chestOpener]->inventoryUI.chestGUI.closeChest();
	}
}

Item* Entity::addItemToChest(Item* item, bool forceNewStack, Item* specificDestinationStack)
{
	if (!item)
	{
		return nullptr;
	}
	int player = getChestOpenerFromEntity(*this);
	if ( player < 0 || player >= MAXPLAYERS )
	{
		return nullptr;
	}
	if ( players[player]->isLocalPlayer() && multiplayer == CLIENT )
	{
		//Tell the server.
		strcpy( (char*)net_packet->data, "CITM" );
		net_packet->data[4] = player;
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		SDLNet_Write32((Uint32)item->type, &net_packet->data[5]);
		SDLNet_Write32((Uint32)item->status, &net_packet->data[9]);
		SDLNet_Write32((Uint32)item->beatitude, &net_packet->data[13]);
		SDLNet_Write32((Uint32)item->count, &net_packet->data[17]);
		SDLNet_Write32((Uint32)item->appearance, &net_packet->data[21]);
		net_packet->data[25] = item->identified;
		net_packet->data[26] = forceNewStack ? 1 : 0;
		net_packet->len = 27;
		sendPacketSafe(net_sock, -1, net_packet, 0);

		return addItemToChestClientside(player, item, forceNewStack, specificDestinationStack);
	}

	Item* item2 = NULL;

	//Add the item to the chest's inventory.
	list_t* inventory = static_cast<list_t* >(children.first->element);

	node_t* t_node = NULL;
	if ( !forceNewStack )
	{
		//If item's already in the chest, add it to a pre-existing stack.
		for (t_node = inventory->first; t_node != NULL; t_node = t_node->next)
		{
			item2 = (Item*) t_node->element;
			if ( !specificDestinationStack )
			{
				if (!itemCompare(item, item2, false) )
				{
					item2->count += item->count;
					return item2;
				}
			}
			else
			{
				if ( specificDestinationStack == item2 )
				{
					item2->count += item->count;
					return item2;
				}
			}
		}
	}

	item->node = list_AddNodeLast(inventory);
	item->node->element = item;
	item->node->deconstructor = &defaultDeconstructor;

	if ( !players[chestOpener]->isLocalPlayer() && multiplayer == SERVER )
	{
		strcpy((char*)net_packet->data, "CITM");
		SDLNet_Write32((Uint32)item->type, &net_packet->data[4]);
		SDLNet_Write32((Uint32)item->status, &net_packet->data[8]);
		SDLNet_Write32((Uint32)item->beatitude, &net_packet->data[12]);
		SDLNet_Write32((Uint32)item->count, &net_packet->data[16]);
		SDLNet_Write32((Uint32)item->appearance, &net_packet->data[20]);
		net_packet->data[24] = item->identified;
		net_packet->data[25] = forceNewStack ? 1 : 0;
		net_packet->data[26] = (Sint8)item->x;
		net_packet->data[27] = (Sint8)item->y;
		net_packet->address.host = net_clients[chestOpener - 1].host;
		net_packet->address.port = net_clients[chestOpener - 1].port;
		net_packet->len = 28;
		sendPacketSafe(net_sock, -1, net_packet, chestOpener - 1);
	}
	return item;
}

Item* Entity::addItemToChestFromInventory(int player, Item* item, int amount, bool forceNewStack, Item* specificDestinationStack)
{
	if (!item || !players[player] || !players[player]->entity)
	{
		return nullptr;
	}

	if (itemCategory(item) == SPELL_CAT)
	{
		return nullptr;
	}

	if ( amount <= 0 )
	{
		amount = item->count;
	}
	else
	{
		amount = std::min((Sint16)amount, item->count);
	}

	bool isEquipped = itemIsEquipped(item, player);

	if ( isEquipped )
	{
		if ( !item->canUnequip(stats[player]) )
		{
			if ( shouldInvertEquipmentBeatitude(stats[player]) && item->beatitude > 0 )
			{
				messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(3218));
			}
			else
			{
				messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(1087));
			}
			playSoundPlayer(player, 90, 64);
			if ( !item->identified )
			{
				if ( players[player]->isLocalPlayer() )
				{
					Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_APPRAISED, item->type, 1);
				}
			}
			item->identified = true;
			return nullptr;
		}
	}
	playSoundPlayer(player, 47 + local_rng.rand() % 3, 64);

	Item* newitem = newItem(item->type, item->status, item->beatitude, amount, item->appearance, item->identified, nullptr);
	Item** slot = itemSlot(stats[player], item);
	if ( multiplayer == CLIENT )
	{
		// tell the server to unequip.
		if ( slot != nullptr )
		{
			if ( slot == &stats[player]->weapon )
			{
				playerTryEquipItemAndUpdateServer(player, item, false);
			}
			else if ( slot == &stats[player]->shield && itemCategory(newitem) == SPELLBOOK )
			{
				playerTryEquipItemAndUpdateServer(player, item, false);
			}
			else
			{
				if ( slot == &stats[player]->helmet )
				{
					clientUnequipSlotAndUpdateServer(player, EQUIP_ITEM_SLOT_HELM, item);
				}
				else if ( slot == &stats[player]->breastplate )
				{
					clientUnequipSlotAndUpdateServer(player, EQUIP_ITEM_SLOT_BREASTPLATE, item);
				}
				else if ( slot == &stats[player]->gloves )
				{
					clientUnequipSlotAndUpdateServer(player, EQUIP_ITEM_SLOT_GLOVES, item);
				}
				else if ( slot == &stats[player]->shoes )
				{
					clientUnequipSlotAndUpdateServer(player, EQUIP_ITEM_SLOT_BOOTS, item);
				}
				else if ( slot == &stats[player]->shield )
				{
					clientUnequipSlotAndUpdateServer(player, EQUIP_ITEM_SLOT_SHIELD, item);
				}
				else if ( slot == &stats[player]->cloak )
				{
					clientUnequipSlotAndUpdateServer(player, EQUIP_ITEM_SLOT_CLOAK, item);
				}
				else if ( slot == &stats[player]->amulet )
				{
					clientUnequipSlotAndUpdateServer(player, EQUIP_ITEM_SLOT_AMULET, item);
				}
				else if ( slot == &stats[player]->ring )
				{
					clientUnequipSlotAndUpdateServer(player, EQUIP_ITEM_SLOT_RING, item);
				}
				else if ( slot == &stats[player]->mask )
				{
					clientUnequipSlotAndUpdateServer(player, EQUIP_ITEM_SLOT_MASK, item);
				}
			}
		}
	}

	// unequip the item
	if ( item->count - (Sint16)amount <= 0 )
	{
		if ( slot != NULL )
		{
			*slot = NULL;
		}
	}
	if ( item->node != NULL )
	{
		if ( item->node->list == &stats[player]->inventory )
		{
			if ( (Sint16)amount != item->count )
			{
				item->count = item->count - (Sint16)amount;
				if ( item->count <= 0 )
				{
					list_RemoveNode(item->node);
				}
			}
			else
			{
				list_RemoveNode(item->node);
			}
		}
	}
	else
	{
		item->count = item->count - (Sint16)amount;
		if ( item->count <= 0 )
		{
			free(item);
		}
	}

	if ( newitem->count > 1 )
	{
		messagePlayer(player, MESSAGE_INVENTORY, Language::get(4197), newitem->count, newitem->getName());
	}
	else
	{
		messagePlayer(player, MESSAGE_INVENTORY, Language::get(463), newitem->getName());
	}

	return addItemToChest(newitem, forceNewStack, specificDestinationStack);
}

Item* Entity::getItemFromChest(Item* item, int amount, bool getInfoOnly)
{
	/*
	 * getInfoOnly just returns a copy of the item at the slot, it does not actually grab the item.
	 * Note that the returned memory will need to be freed.
	 */
	
	Item* newitem = NULL;

	if ( item == NULL )
	{
		return NULL;
	}

	if ( amount <= 0 )
	{
		amount = item->count;
	}
	else
	{
		amount = std::min((Sint16)amount, item->count);
	}

	int player = getChestOpenerFromEntity(*this);

	if ( players[player]->isLocalPlayer() && multiplayer == CLIENT )
	{
		if (!item || !item->node)
		{
			return NULL;
		}

		newitem = newItem(item->type, item->status, item->beatitude, 1, item->appearance, item->identified, nullptr);

		//Tell the server.
		if ( !getInfoOnly )
		{
			strcpy( (char*)net_packet->data, "RCIT" );  //Have the server remove the item from the chest).
			net_packet->data[4] = player;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			SDLNet_Write32((Uint32)item->type, &net_packet->data[5]);
			SDLNet_Write32((Uint32)item->status, &net_packet->data[9]);
			SDLNet_Write32((Uint32)item->beatitude, &net_packet->data[13]);
			Sint16 count = (Sint16)amount;
			SDLNet_Write32((Uint32)count, &net_packet->data[17]);
			SDLNet_Write32((Uint32)item->appearance, &net_packet->data[21]);
			net_packet->data[25] = item->identified;
			net_packet->len = 26;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
	}
	else
	{
		if ( !item )
		{
			return NULL;
		}
		if ( !item->node )
		{
			return NULL;
		}
		if ( item->node->list != children.first->element )
		{
			return NULL;
		}

		newitem = newItem(item->type, item->status, item->beatitude, 1, item->appearance, item->identified, nullptr);
	}

	if ( getInfoOnly )
	{
		itemuids--;
	}

	//Grab only x items from the chest.
	newitem->count = amount;
	if (!getInfoOnly )
	{
		item->count -= amount;
		if ( item->count <= 0 )
		{
			list_RemoveNode(item->node);
		}
	}
	return newitem;
}

void closeChestClientside(const int player)
{
	if ( player < 0 )
	{
		return;
	}
	if (!openedChest[player])
	{
		return;
	}

	if ( multiplayer == CLIENT && players[player]->isLocalPlayer() )
	{
		//Only called for the client.
		list_FreeAll(&chestInv[player]);

		openedChest[player] = NULL;

		//Reset chest-gamepad related stuff here.
		players[player]->inventoryUI.chestGUI.closeChest();
	}
}

Item* addItemToChestClientside(const int player, Item* item, bool forceNewStack, Item* specificDestinationStack)
{
	if (openedChest[player])
	{
		//messagePlayer(player, "Recieved item.");

		//If there's an open chests, add an item to it.
		//TODO: Add item to the chest.

		Item* item2 = NULL;
		node_t* node = NULL;

		if ( !forceNewStack )
		{
			for (node = chestInv[player].first; node != NULL; node = node->next)
			{
				item2 = (Item*) node->element;
				if ( !specificDestinationStack )
				{
					if ( !itemCompare(item, item2, false) )
					{
						item2->count += item->count;
						return item2;
					}
				}
				else
				{
					if ( specificDestinationStack == item2 )
					{
						item2->count += item->count;
						return item2;
					}
				}
			}
		}

		item->node = list_AddNodeLast(&chestInv[player]);
		item->node->element = item;
		item->node->deconstructor = &defaultDeconstructor;
		return item;
	}
	//TODO: Else: Ruh-roh, error!
	return nullptr;
}

Item* Entity::addItemToChestServer(Item* item, bool forceNewStack, Item* specificDestinationStack)
{
	if (!item)
	{
		return nullptr;
	}

	Item* item2 = NULL;
	node_t* t_node = NULL;

	//Add the item to the chest's inventory.
	list_t* inventory = static_cast<list_t* >(children.first->element);

	if (!inventory)
	{
		return nullptr;
	}

	if ( !forceNewStack )
	{
		//If item's already in the chest, add it to a pre-existing stack.
		for (t_node = inventory->first; t_node != NULL; t_node = t_node->next)
		{
			item2 = (Item*) t_node->element;
			if ( !specificDestinationStack )
			{
				if ( !itemCompare(item, item2, false) )
				{
					item2->count += item->count;
					return item2;
				}
			}
			else
			{
				if ( specificDestinationStack == item2 )
				{
					item2->count += item->count;
					return item2;
				}
			}
		}
	}

	item->node = list_AddNodeLast(inventory);
	item->node->element = item;
	item->node->deconstructor = &defaultDeconstructor;
	return item;
}

bool Entity::removeItemFromChestServer(Item* item, int count)
{
	if (!item)
	{
		return false;
	}

	Item* item2 = NULL;
	node_t* t_node = NULL;

	list_t* inventory = static_cast<list_t* >(children.first->element);
	if (!inventory)
	{
		return false;
	}

	node_t* nextnode = nullptr;
	bool removedItems = false;
	for ( t_node = inventory->first; t_node != NULL; t_node = nextnode )
	{
		nextnode = t_node->next;
		item2 = (Item*) t_node->element;
		if (!item2  || !item2->node || item2->node->list != children.first->element)
		{
			return false;
		}
		if (!itemCompare(item, item2, false, false))
		{
			if (count < item2->count)
			{
				//Grab only one item from the chest.
				int oldcount = item2->count;
				item2->count = oldcount - count;
				if ( item2->count <= 0 )
				{
					list_RemoveNode(item2->node);
				}
			}
			else if ( count == item2->count )
			{
				//Grab all items from the chest.
				list_RemoveNode(item2->node);
			}
			else if ( count > item2->count )
			{
				count -= item2->count;
				//Grab items that we can, and then look for more. 
				list_RemoveNode(item2->node);
				removedItems = true;
				continue;
			}
			return true;
		}
	}
	return removedItems;
}

void Entity::unlockChest()
{
	chestLocked = 0;
	chestPreventLockpickCapstoneExploit = 1;
}

void Entity::lockChest()
{
	chestLocked = 1;
}

void Entity::chestHandleDamageMagic(int damage, Entity &magicProjectile, Entity *caster)
{
	if ( behavior == &actMonster )
	{
		Stat* stats = getStats();
		bool oldHP = stats ? stats->HP : 0;
		modHP(-damage); // do the damage

		if ( stats && stats->HP <= 0 )
		{
			if ( caster && caster->behavior == &actPlayer )
			{
				if ( magicProjectile.behavior == &actBomb )
				{
					messagePlayer(caster->skill[2], MESSAGE_COMBAT, Language::get(3617), items[magicProjectile.skill[21]].getIdentifiedName(), Language::get(675));
				}
				else
				{
					messagePlayer(caster->skill[2], MESSAGE_COMBAT, Language::get(2520));
				}
				if ( oldHP > 0 )
				{
					messagePlayerMonsterEvent(caster->skill[2], makeColorRGB(0, 255, 0),
						*stats, Language::get(692), Language::get(692), MSG_COMBAT);
				}	
			}
			if ( caster && oldHP > 0 )
			{
				awardXP(caster, true, true);
			}
		}
		else
		{
			if ( caster && caster->behavior == &actPlayer )
			{
				if ( magicProjectile.behavior == &actBomb )
				{
					messagePlayer(caster->skill[2], MESSAGE_COMBAT_BASIC, Language::get(3618), items[magicProjectile.skill[21]].getIdentifiedName(), Language::get(675));
				}
				else
				{
					messagePlayer(caster->skill[2], MESSAGE_COMBAT_BASIC, Language::get(378), Language::get(675));
				}
			}
			if ( caster && isInertMimic() )
			{
				disturbMimic(caster, true, true);
			}
		}

		if ( stats )
		{
			// update enemy bar for attacker
			updateEnemyBar(caster, this, Language::get(675), stats->HP, stats->MAXHP, false,
				DamageGib::DMG_DEFAULT);
		}
	}
	else
	{
		chestHealth -= damage; //Decrease chest health.
		if ( caster )
		{
			if ( caster->behavior == &actPlayer )
			{
				if ( chestHealth <= 0 )
				{
					if ( magicProjectile.behavior == &actBomb )
					{
						messagePlayer(caster->skill[2], MESSAGE_COMBAT, Language::get(3617), items[magicProjectile.skill[21]].getIdentifiedName(), Language::get(675));
					}
					else
					{
						messagePlayer(caster->skill[2], MESSAGE_COMBAT, Language::get(2520));
					}
					Compendium_t::Events_t::eventUpdateWorld(caster->skill[2], Compendium_t::CPDM_CHESTS_DESTROYED, "chest", 1);
				}
				else
				{
					if ( magicProjectile.behavior == &actBomb )
					{
						messagePlayer(caster->skill[2], MESSAGE_COMBAT_BASIC, Language::get(3618), items[magicProjectile.skill[21]].getIdentifiedName(), Language::get(675));
					}
					else
					{
						messagePlayer(caster->skill[2], MESSAGE_COMBAT_BASIC, Language::get(378), Language::get(675));
					}
				}
			}
			updateEnemyBar(caster, this, Language::get(675), chestHealth, chestMaxHealth,
				false, DamageGib::DMG_DEFAULT);
		}
	}
	playSoundEntity(this, 28, 128);
}
