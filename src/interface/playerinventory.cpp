/*-------------------------------------------------------------------------------

	BARONY
	File: playerinventory.cpp
	Desc: contains player inventory related functions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../draw.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../items.hpp"
#include "../shops.hpp"
#include "../engine/audio/sound.hpp"
#include "../net.hpp"
#include "../magic/magic.hpp"
#include "../menu.hpp"
#include "../player.hpp"
#include "interface.hpp"
#include "consolecommand.hpp"
#include "../ui/GameUI.hpp"
#include "../ui/Frame.hpp"
#include "../ui/Image.hpp"
#include "../ui/Field.hpp"
#include "../ui/Text.hpp"
#include "../ui/Button.hpp"
#include "../ui/MainMenu.hpp"
#include "../mod_tools.hpp"
#include "../book.hpp"
#ifdef STEAMWORKS
#include <steam/steam_api.h>
#include "../steam.hpp"
#endif

//Prototype helper functions for player inventory helper functions.
bool restrictPaperDollMovement = true;
SDL_Surface* inventory_mode_item_img = NULL;
SDL_Surface* inventory_mode_item_highlighted_img = NULL;
SDL_Surface* inventory_mode_spell_img = NULL;
SDL_Surface* inventory_mode_spell_highlighted_img = NULL;

bool executeItemMenuOption0ForPaperDoll(const int player, Item* item, bool droppingAndUnequipping)
{
	if ( !item )
	{
		return false;
	}

	if ( !players[player]->isLocalPlayer()
		|| !players[player]->paperDoll.enabled
		|| !players[player]->paperDoll.isItemOnDoll(*item) )
	{
		return false;
	}

	if ( itemCategory(item) == SPELL_CAT )
	{
		printlog("Warning: executed paper doll menu on spell");
		return false;
	}

	if ( !droppingAndUnequipping && !players[player]->inventoryUI.bItemInventoryHasFreeSlot() )
	{
		// no backpack space
		messagePlayer(player, MESSAGE_INVENTORY, Language::get(3997), item->getName());
		playSoundPlayer(player, 90, 64);
		return false;
	}

	int oldGUI = players[player]->gui_mode;
	players[player]->gui_mode = GUI_MODE_INVENTORY; // this makes sure we don't try sell the item or something

	if ( droppingAndUnequipping )
	{
		players[player]->inventoryUI.activateItemContextMenuOption(item, ItemContextMenuPrompts::PROMPT_UNEQUIP_FOR_DROP);
	}
	else
	{
		players[player]->inventoryUI.activateItemContextMenuOption(item, ItemContextMenuPrompts::PROMPT_UNEQUIP);
	}

	players[player]->gui_mode = oldGUI;

	players[player]->paperDoll.updateSlots();
	if ( players[player]->paperDoll.isItemOnDoll(*item) )
	{
		// cursed or couldn't unequip
		return false;
	}
	return true;
}

bool executeItemMenuOption0ForInventoryItem(const int player, Item* item) // returns true on equip successful.
{
	if ( !item )
	{
		return false;
	}

	if ( !players[player]->isLocalPlayer()
		|| players[player]->paperDoll.isItemOnDoll(*item) )
	{
		return false;
	}

	if ( itemCategory(item) == SPELL_CAT )
	{
		printlog("Warning: executed inventory 0 menu on spell");
		return false;
	}

	// should always have backpack space?
	//if ( !players[player]->inventoryUI.bItemInventoryHasFreeSlot() )
	//{
	//	// no backpack space
	//	messagePlayer(player, Language::get(3997), item->getName());
	//	return false;
	//}

	int oldGUI = players[player]->gui_mode;
	players[player]->gui_mode = GUI_MODE_INVENTORY; // this makes sure we don't try sell the item or something

	bool isSelectedItem = inputs.getUIInteraction(player)->selectedItem == item;
	players[player]->inventoryUI.activateItemContextMenuOption(item, ItemContextMenuPrompts::PROMPT_UNEQUIP);

	players[player]->gui_mode = oldGUI;

	players[player]->paperDoll.updateSlots();
	if ( isSelectedItem && inputs.getUIInteraction(player)->selectedItem == nullptr )
	{
		return true; // item was consumed
	}
	if ( players[player]->paperDoll.isItemOnDoll(*item) )
	{
		return true;
	}
	// cursed or couldn't unequip existing item
	return false;
}

/*-------------------------------------------------------------------------------

	itemUseString

	Returns a string with the verb cooresponding to the item which is
	to be used

-------------------------------------------------------------------------------*/

const char* itemEquipString(int player, const Item& item)
{
	if ( items[item.type].item_slot == ItemEquippableSlot::NO_EQUIP && !(itemCategory(&item) == SPELL_CAT) )
	{
		return "Invalid";
	}

	if ( itemCategory(&item) == WEAPON )
	{
		if ( itemIsEquipped(&item, player) )
		{
			return Language::get(323);
		}
		else
		{
			return Language::get(324);
		}
	}
	else if ( itemCategory(&item) == ARMOR )
	{
		switch ( item.type )
		{
			case WOODEN_SHIELD:
			case BRONZE_SHIELD:
			case IRON_SHIELD:
			case STEEL_SHIELD:
			case STEEL_SHIELD_RESISTANCE:
			case CRYSTAL_SHIELD:
			case MIRROR_SHIELD:
				if ( itemIsEquipped(&item, player) )
				{
					return Language::get(325);
				}
				else
				{
					return Language::get(326);
				}
			default:
				break;
		}
		if ( itemIsEquipped(&item, player) )
		{
			return Language::get(327);
		}
		else
		{
			return Language::get(328);
		}
	}
	else if ( itemCategory(&item) == AMULET )
	{
		if ( itemIsEquipped(&item, player) )
		{
			return Language::get(327);
		}
		else
		{
			return Language::get(328);
		}
	}
	else if ( itemCategory(&item) == POTION )
	{
		if ( itemIsEquipped(&item, player) )
		{
			return Language::get(323);
		}
		else
		{
			return Language::get(324);
		}
	}
	else if ( itemCategory(&item) == MAGICSTAFF )
	{
		if ( itemIsEquipped(&item, player) )
		{
			return Language::get(323);
		}
		else
		{
			return Language::get(324);
		}
	}
	else if ( itemCategory(&item) == RING )
	{
		if ( itemIsEquipped(&item, player) )
		{
			return Language::get(327);
		}
		else
		{
			return Language::get(331);
		}
	}
	else if ( itemCategory(&item) == SPELLBOOK )
	{
		if ( itemIsEquipped(&item, player) )
		{
			return Language::get(323);
		}
		else
		{
			return Language::get(324);
		}
	}
	else if ( itemCategory(&item) == GEM )
	{
		if ( itemIsEquipped(&item, player) )
		{
			return Language::get(323);
		}
		else
		{
			return Language::get(324);
		}
	}
	else if ( itemCategory(&item) == THROWN )
	{
		if ( itemIsEquipped(&item, player) )
		{
			return Language::get(323);
		}
		else
		{
			return Language::get(324);
		}
	}
	else if ( itemCategory(&item) == TOOL )
	{
		switch ( item.type )
		{
			case TOOL_PICKAXE:
				if ( itemIsEquipped(&item, player) )
				{
					return Language::get(323);
				}
				else
				{
					return Language::get(324);
				}
			case TOOL_LOCKPICK:
			case TOOL_SKELETONKEY:
				if ( itemIsEquipped(&item, player) )
				{
					return Language::get(333);
				}
				else
				{
					return Language::get(334);
				}
			case TOOL_TORCH:
			case TOOL_LANTERN:
			case TOOL_CRYSTALSHARD:
				if ( itemIsEquipped(&item, player) )
				{
					return Language::get(335);
				}
				else
				{
					return Language::get(336);
				}
			case TOOL_BLINDFOLD:
				if ( itemIsEquipped(&item, player) )
				{
					return Language::get(327);
				}
				else
				{
					return Language::get(328);
				}
			case TOOL_GLASSES:
			case MONOCLE:
				if ( itemIsEquipped(&item, player) )
				{
					return Language::get(327);
				}
				else
				{
					return Language::get(331);
				}
			default:
				if ( itemIsEquipped(&item, player) )
				{
					return Language::get(323);
				}
				else
				{
					return Language::get(324);
				}
		}
	}
	else if ( itemCategory(&item) == FOOD )
	{
		if ( itemIsEquipped(&item, player) )
		{
			return Language::get(323);
		}
		else
		{
			return Language::get(324);
		}
	}
	else if ( itemCategory(&item) == SPELL_CAT )
	{
		return Language::get(339);
	}

	return "Invalid";
}

const char* itemUseString(int player, const Item& item)
{
	if ( itemCategory(&item) == POTION )
	{
		return Language::get(329);
	}
	else if ( itemCategory(&item) == SCROLL )
	{
		return Language::get(330);
	}
	else if ( itemCategory(&item) == SPELLBOOK )
	{
		return Language::get(330);
	}
	else if ( itemCategory(&item) == TOOL )
	{
		switch ( item.type )
		{
			case TOOL_TINOPENER:
				return Language::get(1881);
			case TOOL_MIRROR:
				return Language::get(332);
			case TOOL_TOWEL:
				return Language::get(332);
			case TOOL_BEARTRAP:
				return Language::get(337);
			case TOOL_ALEMBIC:
				if ( GenericGUI[player].alchemyGUI.bOpen && GenericGUI[player].alembicItem == &item )
				{
					return Language::get(3341);
				}
				else
				{
					return Language::get(3339);
				}
			case TOOL_METAL_SCRAP:
			case TOOL_MAGIC_SCRAP:
				return Language::get(1881);
				break;
			default:
				break;
		}
	}
	else if ( itemCategory(&item) == FOOD )
	{
		return Language::get(338);
	}
	else if ( itemCategory(&item) == BOOK )
	{
		return Language::get(330);
	}
	return Language::get(332);
}

// deprecated
//char* itemUseStringOld(int player, const Item* item)
//{
//	if ( itemCategory(item) == WEAPON )
//	{
//		if ( itemIsEquipped(item, player) )
//		{
//			return Language::get(323);
//		}
//		else
//		{
//			return Language::get(324);
//		}
//	}
//	else if ( itemCategory(item) == ARMOR )
//	{
//		switch ( item->type )
//		{
//			case WOODEN_SHIELD:
//			case BRONZE_SHIELD:
//			case IRON_SHIELD:
//			case STEEL_SHIELD:
//			case STEEL_SHIELD_RESISTANCE:
//			case CRYSTAL_SHIELD:
//			case MIRROR_SHIELD:
//				if ( itemIsEquipped(item, player) )
//				{
//					return Language::get(325);
//				}
//				else
//				{
//					return Language::get(326);
//				}
//			default:
//				break;
//		}
//		if ( itemIsEquipped(item, player) )
//		{
//			return Language::get(327);
//		}
//		else
//		{
//			return Language::get(328);
//		}
//	}
//	else if ( itemCategory(item) == AMULET )
//	{
//		if ( itemIsEquipped(item, player) )
//		{
//			return Language::get(327);
//		}
//		else
//		{
//			return Language::get(328);
//		}
//	}
//	else if ( itemCategory(item) == POTION )
//	{
//		return Language::get(329);
//	}
//	else if ( itemCategory(item) == SCROLL )
//	{
//		return Language::get(330);
//	}
//	else if ( itemCategory(item) == MAGICSTAFF )
//	{
//		if ( itemIsEquipped(item, player) )
//		{
//			return Language::get(323);
//		}
//		else
//		{
//			return Language::get(324);
//		}
//	}
//	else if ( itemCategory(item) == RING )
//	{
//		if ( itemIsEquipped(item, player) )
//		{
//			return Language::get(327);
//		}
//		else
//		{
//			return Language::get(331);
//		}
//	}
//	else if ( itemCategory(item) == SPELLBOOK )
//	{
//		return Language::get(330);
//	}
//	else if ( itemCategory(item) == GEM )
//	{
//		if ( itemIsEquipped(item, player) )
//		{
//			return Language::get(323);
//		}
//		else
//		{
//			return Language::get(324);
//		}
//	}
//	else if ( itemCategory(item) == THROWN )
//	{
//		if ( itemIsEquipped(item, player) )
//		{
//			return Language::get(323);
//		}
//		else
//		{
//			return Language::get(324);
//		}
//	}
//	else if ( itemCategory(item) == TOOL )
//	{
//		switch ( item->type )
//		{
//			case TOOL_PICKAXE:
//				if ( itemIsEquipped(item, player) )
//				{
//					return Language::get(323);
//				}
//				else
//				{
//					return Language::get(324);
//				}
//			case TOOL_TINOPENER:
//				return Language::get(1881);
//			case TOOL_MIRROR:
//				return Language::get(332);
//			case TOOL_LOCKPICK:
//			case TOOL_SKELETONKEY:
//				if ( itemIsEquipped(item, player) )
//				{
//					return Language::get(333);
//				}
//				else
//				{
//					return Language::get(334);
//				}
//			case TOOL_TORCH:
//			case TOOL_LANTERN:
//			case TOOL_CRYSTALSHARD:
//				if ( itemIsEquipped(item, player) )
//				{
//					return Language::get(335);
//				}
//				else
//				{
//					return Language::get(336);
//				}
//			case TOOL_BLINDFOLD:
//				if ( itemIsEquipped(item, player) )
//				{
//					return Language::get(327);
//				}
//				else
//				{
//					return Language::get(328);
//				}
//			case TOOL_TOWEL:
//				return Language::get(332);
//			case TOOL_GLASSES:
//				if ( itemIsEquipped(item, player) )
//				{
//					return Language::get(327);
//				}
//				else
//				{
//					return Language::get(331);
//				}
//			case TOOL_BEARTRAP:
//				return Language::get(337);
//			case TOOL_ALEMBIC:
//				return Language::get(3339);
//			case TOOL_METAL_SCRAP:
//			case TOOL_MAGIC_SCRAP:
//				return Language::get(1881);
//				break;
//			default:
//				break;
//		}
//	}
//	else if ( itemCategory(item) == FOOD )
//	{
//		return Language::get(338);
//	}
//	else if ( itemCategory(item) == BOOK )
//	{
//		return Language::get(330);
//	}
//	else if ( itemCategory(item) == SPELL_CAT )
//	{
//		return Language::get(339);
//	}
//	return Language::get(332);
//}

Player::PaperDoll_t::PaperDollSlotType getPaperDollSlotFromItemType(Item& item)
{
	auto slotName = items[item.type].item_slot;
	Player::PaperDoll_t::PaperDollSlotType dollSlot = Player::PaperDoll_t::PaperDollSlotType::SLOT_MAX;
	switch ( slotName )
	{
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_WEAPON:
			dollSlot = Player::PaperDoll_t::PaperDollSlotType::SLOT_WEAPON;
			break;
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_SHIELD:
			dollSlot = Player::PaperDoll_t::PaperDollSlotType::SLOT_OFFHAND;
			break;
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_MASK:
			dollSlot = Player::PaperDoll_t::PaperDollSlotType::SLOT_GLASSES;
			break;
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_HELM:
			dollSlot = Player::PaperDoll_t::PaperDollSlotType::SLOT_HELM;
			break;
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_GLOVES:
			dollSlot = Player::PaperDoll_t::PaperDollSlotType::SLOT_GLOVES;
			break;
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_BOOTS:
			dollSlot = Player::PaperDoll_t::PaperDollSlotType::SLOT_BOOTS;
			break;
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_BREASTPLATE:
			dollSlot = Player::PaperDoll_t::PaperDollSlotType::SLOT_BREASTPLATE;
			break;
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_CLOAK:
			dollSlot = Player::PaperDoll_t::PaperDollSlotType::SLOT_CLOAK;
			break;
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_AMULET:
			dollSlot = Player::PaperDoll_t::PaperDollSlotType::SLOT_AMULET;
			break;
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_RING:
			dollSlot = Player::PaperDoll_t::PaperDollSlotType::SLOT_RING;
			break;
		default:
			break;
	}
	return dollSlot;
}

Player::PaperDoll_t::PaperDollSlotType Player::PaperDoll_t::paperDollSlotFromCoordinates(int x, int y) const
{
	auto slot = PaperDollSlotType::SLOT_MAX;
	if ( !enabled )
	{
		// in inventory
		return SLOT_MAX;
	}

	if ( player.inventoryUI.selectedSlotInPaperDoll() )
	{
		const int selectedSlotX = player.inventoryUI.getSelectedSlotX();
		const int selectedSlotY = player.inventoryUI.getSelectedSlotY();

		if ( player.inventoryUI.bCompactView
			&& (x > Player::Inventory_t::DOLL_COLUMN_RIGHT || x < Player::Inventory_t::DOLL_COLUMN_LEFT) )
		{
			return SLOT_MAX; // off onto inventory
		}

		if ( x > selectedSlotX ) // moving right
		{
			if ( x > Player::Inventory_t::DOLL_COLUMN_RIGHT )
			{
				x = Player::Inventory_t::DOLL_COLUMN_LEFT;
			}
		}
		else if ( x < selectedSlotX ) // moving left
		{
			if ( x < Player::Inventory_t::DOLL_COLUMN_LEFT )
			{
				x = Player::Inventory_t::DOLL_COLUMN_RIGHT;
			}
		}
		x = std::max(std::min(x, static_cast<int>(Player::Inventory_t::DOLL_COLUMN_RIGHT)), static_cast<int>(Player::Inventory_t::DOLL_COLUMN_LEFT));

		if ( !player.inventoryUI.bCompactView )
		{
			if ( y < Player::Inventory_t::DOLL_ROW_1 || y > Player::Inventory_t::DOLL_ROW_5 )
			{
				// in inventory
				return SLOT_MAX;
			}
		}
		else
		{
			if ( y > selectedSlotY ) // moving down
			{
				if ( y > Player::Inventory_t::DOLL_ROW_5 )
				{
					y = Player::Inventory_t::DOLL_ROW_1;
				}
			}
			else if ( y < selectedSlotY ) // moving up
			{
				if ( y < Player::Inventory_t::DOLL_ROW_1 )
				{
					y = Player::Inventory_t::DOLL_ROW_5;
				}
			}
		}

		y = std::min(std::max(y, static_cast<int>(Player::Inventory_t::DOLL_ROW_1)), std::min(y, static_cast<int>(Player::Inventory_t::DOLL_ROW_5)));
		if ( y == Player::Inventory_t::DOLL_ROW_5 )
		{
			slot = (x == Player::Inventory_t::DOLL_COLUMN_LEFT ? SLOT_OFFHAND : SLOT_WEAPON);
		}
		else if ( y == Player::Inventory_t::DOLL_ROW_4 )
		{
			slot = (x == Player::Inventory_t::DOLL_COLUMN_LEFT ? SLOT_RING : SLOT_BOOTS);
		}
		else if ( y == Player::Inventory_t::DOLL_ROW_3 )
		{
			slot = (x == Player::Inventory_t::DOLL_COLUMN_LEFT ? SLOT_AMULET : SLOT_GLOVES);
		}
		else if ( y == Player::Inventory_t::DOLL_ROW_2 )
		{
			slot = (x == Player::Inventory_t::DOLL_COLUMN_LEFT ? SLOT_CLOAK : SLOT_BREASTPLATE);
		}
		else if ( y == Player::Inventory_t::DOLL_ROW_1 )
		{
			slot = (x == Player::Inventory_t::DOLL_COLUMN_LEFT ? SLOT_GLASSES : SLOT_HELM);
		}
		return slot;
	}

	if ( player.inventoryUI.bCompactView )
	{
		if ( x < 0 )
		{
			// right half.
			slot = SLOT_HELM;
			if ( y == 0 )
			{
				slot = PaperDollSlotType((int)slot + 0);
			}
			else if ( y == 1 )
			{
				slot = PaperDollSlotType((int)slot + 1);
			}
			else if ( y == 2 )
			{
				slot = PaperDollSlotType((int)slot + 2);
			}
			else if ( y == 3 )
			{
				slot = PaperDollSlotType((int)slot + 3);
			}
			else
			{
				slot = PaperDollSlotType((int)slot + 4);
			}
			return slot;
		}
		else if ( x >= player.inventoryUI.getSizeX() )
		{
			// left half.
			slot = SLOT_GLASSES;
			if ( y == 0 )
			{
				slot = PaperDollSlotType((int)slot + 0);
			}
			else if ( y == 1 )
			{
				slot = PaperDollSlotType((int)slot + 1);
			}
			else if ( y == 2 )
			{
				slot = PaperDollSlotType((int)slot + 2);
			}
			else if ( y == 3 )
			{
				slot = PaperDollSlotType((int)slot + 3);
			}
			else
			{
				slot = PaperDollSlotType((int)slot + 4);
			}
			return slot;
		}
	}
	else
	{
		if ( y < 0 )
		{
			if ( x <= (player.inventoryUI.getSizeX() / 2) )
			{
				// left half.
				slot = SLOT_OFFHAND;
			}
			else
			{
				// right half.
				slot = SLOT_WEAPON;
			}
			return slot;
		}
		else if ( y >= player.inventoryUI.getSizeY() )
		{
			if ( x <= (player.inventoryUI.getSizeX() / 2) )
			{
				// left half.
				slot = SLOT_GLASSES;
			}
			else
			{
				// right half.
				slot = SLOT_HELM;
			}
			return slot;
		}
	}

	// in inventory
	return SLOT_MAX;
}

void Player::PaperDoll_t::getCoordinatesFromSlotType(Player::PaperDoll_t::PaperDollSlotType slot, int& outx, int& outy) const
{
	if ( slot == SLOT_MAX )
	{
		return;
	}
	int x = Player::Inventory_t::DOLL_COLUMN_LEFT;
	int y = Player::Inventory_t::DOLL_ROW_1;
	if ( slot >= SLOT_HELM )
	{
		x = Player::Inventory_t::DOLL_COLUMN_RIGHT;
		y += (static_cast<int>(slot - SLOT_HELM));
		outx = x;
		outy = y;
	}
	else if ( slot >= SLOT_GLASSES )
	{
		y += (static_cast<int>(slot));
		outx = x;
		outy = y;
	}
}

void Player::PaperDoll_t::selectPaperDollCoordinatesFromSlotType(Player::PaperDoll_t::PaperDollSlotType slot) const
{
	if ( slot == SLOT_MAX )
	{
		return;
	}
	int x = Player::Inventory_t::DOLL_COLUMN_LEFT;
	int y = Player::Inventory_t::DOLL_ROW_1;
	if ( slot >= SLOT_HELM )
	{
		x = Player::Inventory_t::DOLL_COLUMN_RIGHT;
		y += (static_cast<int>(slot - SLOT_HELM));
		player.inventoryUI.selectSlot(x, y);
	}
	else if ( slot >= SLOT_GLASSES )
	{
		y += (static_cast<int>(slot));
		player.inventoryUI.selectSlot(x, y);
	}
}

bool moveInPaperDoll(int player, Player::PaperDoll_t::PaperDollSlotType paperDollSlot, int currentx, int currenty, int diffx, int diffy, int& xout, int& yout)
{
	auto& inventoryUI = players[player]->inventoryUI;

	int x = xout;
	int y = yout;

	bool movingFromInventory = currenty >= 0;

	if ( inventoryUI.bCompactView )
	{
		if ( GenericGUI[player].tinkerGUI.bOpen || GenericGUI[player].alchemyGUI.bOpen 
			|| GenericGUI[player].featherGUI.bOpen
			|| GenericGUI[player].assistShrineGUI.bOpen )
		{
			return false;
		}
		if ( players[player]->gui_mode == GUI_MODE_SHOP && players[player]->shopGUI.bOpen )
		{
			return false;
		}
		if ( openedChest[player] && players[player]->inventoryUI.chestGUI.bOpen )
		{
			return false;
		}
		if ( !movingFromInventory ) // if last position was in paper doll
		{
			if ( y > Player::Inventory_t::DOLL_ROW_5 )
			{
				y = Player::Inventory_t::DOLL_ROW_1;
			}
			else if ( y < Player::Inventory_t::DOLL_ROW_1 )
			{
				y = Player::Inventory_t::DOLL_ROW_5;
			}
		}
		else if ( movingFromInventory ) // if last position was in inventory
		{
			if ( y == 0 )
			{
				y = Player::Inventory_t::DOLL_ROW_1;
			}
			else if ( y == 1 )
			{
				y = Player::Inventory_t::DOLL_ROW_2;
			}
			else if ( y == 2 )
			{
				y = Player::Inventory_t::DOLL_ROW_3;
			}
			else if ( y == 3 )
			{
				y = Player::Inventory_t::DOLL_ROW_4;
			}
			else
			{
				y = Player::Inventory_t::DOLL_ROW_5;
			}

			if ( x >= inventoryUI.getSizeX() )
			{
				x = Player::Inventory_t::DOLL_COLUMN_LEFT;
			}
			else if ( x >= 0 )
			{
				x = Player::Inventory_t::DOLL_COLUMN_RIGHT;
			}
		}

	}
	else
	{
		if ( openedChest[player] && players[player]->inventoryUI.chestGUI.bOpen )
		{
			if ( !movingFromInventory ) // if last position was in paper doll
			{
				if ( x > Player::Inventory_t::DOLL_COLUMN_RIGHT )
				{
					xout = players[player]->inventoryUI.getSizeX();
					return false;
				}
				else if ( x < Player::Inventory_t::DOLL_COLUMN_LEFT )
				{
					return false;
				}
			}
		}

		if ( !movingFromInventory ) // if last position was in paper doll
		{
			if ( x > Player::Inventory_t::DOLL_COLUMN_RIGHT )
			{
				x = Player::Inventory_t::DOLL_COLUMN_LEFT;
			}
			else if ( x < Player::Inventory_t::DOLL_COLUMN_LEFT )
			{
				x = Player::Inventory_t::DOLL_COLUMN_RIGHT;
			}
		}
		else if ( movingFromInventory ) // if last position was in inventory
		{
			if ( x > inventoryUI.getSizeX() / 2 )
			{
				x = Player::Inventory_t::DOLL_COLUMN_RIGHT;
			}
			else
			{
				x = Player::Inventory_t::DOLL_COLUMN_LEFT;
			}
		}

		if ( y < Player::Inventory_t::DOLL_ROW_1 )
		{
			y = inventoryUI.getSizeY() - 1;
		}
		else if ( y >= inventoryUI.getSizeY() )
		{
			y = Player::Inventory_t::DOLL_ROW_1;
		}
		else if ( y >= 0 )
		{
			y = Player::Inventory_t::DOLL_ROW_5;
		}
	}

	if ( players[player]->paperDoll.dollSlots[paperDollSlot].item == 0 && y < 0 )
	{
		std::vector<std::pair<Player::PaperDoll_t::PaperDollSlotType, int>> goodspots;
		for ( auto& slot : players[player]->paperDoll.dollSlots )
		{
			if ( slot.slotType == paperDollSlot || slot.item == 0 )
			{
				continue;
			}
			int slotx, sloty;
			players[player]->paperDoll.getCoordinatesFromSlotType(slot.slotType, slotx, sloty);
			int dist = abs(sloty - y);

			if ( players[player]->inventoryUI.bCompactView )
			{
				if ( diffy != 0 )
				{
					if ( slotx != x )
					{
						continue;
					}
					if ( diffy > 0 && sloty < y ) 
					{ 
						// just in case this is the only available slot, add a distance penalty
						dist += 10 * (10 - (y - sloty));
					}
					if ( diffy < 0 && sloty > y ) 
					{ 
						// just in case this is the only available slot, add a distance penalty
						dist += 10 * (10 - (sloty - y));
					}
				}
				if ( diffx != 0 )
				{
					if ( slotx != x ) 
					{ 
						if ( !movingFromInventory )
						{
							continue;
						}
						else if ( movingFromInventory )
						{
							// just in case this is the only available slot, add a distance penalty
							dist += 10;
						}
					}
				}
			}
			else
			{
				if ( diffy != 0 )
				{
					if ( slotx != x ) 
					{ 
						if ( !movingFromInventory )
						{
							continue;
						}
						else if ( movingFromInventory )
						{
							// just in case this is the only available slot, add a distance penalty
							dist += 10;
						}
					}
					if ( diffy > 0 && sloty < y ) { continue; }
					if ( diffy < 0 && sloty > y ) { continue; }
				}
				if ( diffx != 0 )
				{
					if ( slotx != x ) { continue; }
				}
			}
			goodspots.push_back(std::make_pair(slot.slotType, dist));
		}

		// try find next available paper doll slot
		Player::PaperDoll_t::PaperDollSlotType targetSlot = Player::PaperDoll_t::PaperDollSlotType::SLOT_MAX;
		if ( !goodspots.empty() )
		{
			std::sort(goodspots.begin(), goodspots.end(),
				[](const std::pair<Player::PaperDoll_t::PaperDollSlotType, int>& lhs,
					const std::pair<Player::PaperDoll_t::PaperDollSlotType, int>& rhs)
			{
				return (lhs.second < rhs.second); // sort by distance
			});

			/*for ( auto& spot : goodspots )
			{
				messagePlayer(0, "%d | %d", spot.first, spot.second);
			}*/

			targetSlot = goodspots[0].first;
			if ( targetSlot != Player::PaperDoll_t::PaperDollSlotType::SLOT_MAX )
			{
				players[player]->paperDoll.selectPaperDollCoordinatesFromSlotType(targetSlot);
				return true;
			}
		}
		else if ( !movingFromInventory 
			&& ((!inventoryUI.bCompactView && diffy == 0 && diffx != 0) 
				|| (inventoryUI.bCompactView && diffx == 0 && diffy != 0)) )
		{
			// no action, stay on the current slot
			return true;
		}
	}
	else
	{
		players[player]->paperDoll.selectPaperDollCoordinatesFromSlotType(paperDollSlot);
		return true;
	}

	if ( !inventoryUI.bCompactView )
	{
		if ( diffy > 0 )
		{
			yout = 0;
		}
		else if ( diffy < 0 )
		{
			yout = Player::Inventory_t::DOLL_ROW_1 - 1;
		}
	}
	else
	{
		if ( diffx > 0 )
		{
			xout = 0;
		}
		else if ( diffx < 0 )
		{
			xout = inventoryUI.getSizeX() - 1;
		}
	}
	return false;
}
// only called by handleInventoryMovement in player.cpp
void select_chest_slot(int player, int currentx, int currenty, int diffx, int diffy)
{
	int x = currentx + diffx;
	int y = currenty + diffy;

	int lowestItemY = players[player]->inventoryUI.chestGUI.getNumItemsToDisplayVertical() - 1;
	if ( y < 0 )
	{
		y = lowestItemY;
	}
	if ( y > lowestItemY )
	{
		y = 0;
	}

	if ( x < 0 )
	{
		players[player]->inventoryUI.selectSlot(players[player]->inventoryUI.getSizeX() - 1, y);
		players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
		return;
	}
	if ( x >= Player::Inventory_t::MAX_CHEST_X )
	{
		players[player]->inventoryUI.selectSlot(0, y);
		players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
		return;
	}
	players[player]->inventoryUI.selectChestSlot(x, y);
}

// only called by handleInventoryMovement in player.cpp
void select_shop_slot(int player, int currentx, int currenty, int diffx, int diffy)
{
	int x = currentx + diffx;
	int y = currenty + diffy;

	int lowestItemY = players[player]->shopGUI.MAX_SHOP_Y - 1;
	if ( y < 0 )
	{
		y = lowestItemY;
	}
	if ( y > lowestItemY )
	{
		y = 0;
	}

	if ( x < 0 )
	{
		players[player]->inventoryUI.selectSlot(players[player]->inventoryUI.getSizeX() - 1, y);
		players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
		return;
	}
	if ( x >= players[player]->shopGUI.MAX_SHOP_X )
	{
		players[player]->inventoryUI.selectSlot(0, y);
		players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
		return;
	}
	players[player]->shopGUI.selectShopSlot(x, y);
}

// only called by handleInventoryMovement in player.cpp
void select_tinkering_slot(int player, int currentx, int currenty, int diffx, int diffy)
{
	int x = currentx + diffx;
	int y = currenty + diffy;

	auto& tinkerGUI = GenericGUI[player].tinkerGUI;
	int lowestItemY = tinkerGUI.MAX_TINKER_Y - 1;
	if ( y < 0 )
	{
		y = lowestItemY;
	}
	if ( y > lowestItemY )
	{
		y = 0;
	}
	if ( x < 0 )
	{
		x = tinkerGUI.MAX_TINKER_X - 1;
	}
	if ( x >= tinkerGUI.MAX_TINKER_X )
	{
		x = 0;
	}
	tinkerGUI.selectTinkerSlot(x, y);
}

void select_feather_slot(int player, int currentx, int currenty, int diffx, int diffy)
{
	int x = currentx + diffx;
	int y = currenty + diffy;

	auto& featherGUI = GenericGUI[player].featherGUI;

	if ( !featherGUI.bFirstTimeSnapCursor 
		|| !featherGUI.isInteractable )
	{
		return;
	}

	int lowestItemY = featherGUI.MAX_FEATHER_Y - 1;
	if ( y < 0 )
	{
		y = lowestItemY;
	}
	if ( y > lowestItemY )
	{
		y = 0;
	}

	x = 0;
	featherGUI.selectFeatherSlot(x, y);
	featherGUI.scrollToSlot(x, y, false);
}

// only called by handleInventoryMovement in player.cpp
void select_assistshrine_slot(int player, int currentx, int currenty, int diffx, int diffy)
{
	int x = currentx + diffx;
	int y = currenty + diffy;

	auto& assistShrineGUI = GenericGUI[player].assistShrineGUI;

	if ( !assistShrineGUI.bFirstTimeSnapCursor
		|| !assistShrineGUI.isInteractable )
	{
		return;
	}

	int lowestItemY = 0;
	if ( assistShrineGUI.currentView == GenericGUIMenu::AssistShrineGUI_t::ASSIST_SHRINE_VIEW_CLASSES )
	{
		for ( auto& pair : assistShrineGUI.classSlots )
		{
			lowestItemY = std::max(lowestItemY, pair.first / 100);
		}
	}
	else if ( assistShrineGUI.currentView == GenericGUIMenu::AssistShrineGUI_t::ASSIST_SHRINE_VIEW_RACE )
	{
		lowestItemY = std::max(lowestItemY, (int)assistShrineGUI.raceSlots.size() - 1);
	}

	if ( currentx < 0 )
	{
		if ( assistShrineGUI.currentView != GenericGUIMenu::AssistShrineGUI_t::ASSIST_SHRINE_VIEW_ITEMS )
		{
			if ( assistShrineGUI.currentView == GenericGUIMenu::AssistShrineGUI_t::ASSIST_SHRINE_VIEW_RACE )
			{
				assistShrineGUI.selectAssistShrineSlot(assistShrineGUI.ASSIST_RACE_COLUMN, 0);
				assistShrineGUI.scrollToSlot(assistShrineGUI.ASSIST_RACE_COLUMN, 0, false);
			}
			else
			{
				assistShrineGUI.selectAssistShrineSlot(0, 0);
				assistShrineGUI.scrollToSlot(0, 0, false);
			}
			return;
		}
		if ( diffy != 0 )
		{
			y = 0;
		}
		else if ( diffx != 0 )
		{
			y = 0;
			if ( diffx > 0 )
			{
				if ( currentx > assistShrineGUI.ASSIST_SLOT_RING )
				{
					x = std::max(currentx - 1, assistShrineGUI.ASSIST_SLOT_RING);
				}
				else if ( currentx <= assistShrineGUI.ASSIST_SLOT_RING )
				{
					players[player]->inventoryUI.selectSlot(0, 0);
					players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
					return;
				}
			}
			else if ( diffx < 0 )
			{
				if ( currentx < assistShrineGUI.ASSIST_SLOT_CLOAK )
				{
					x = std::min(currentx + 1, assistShrineGUI.ASSIST_SLOT_CLOAK);
				}
				else if ( currentx == assistShrineGUI.ASSIST_SLOT_CLOAK )
				{
					players[player]->inventoryUI.selectSlot(players[player]->inventoryUI.getSizeX() - 1, 0);
					players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
					return;
				}
			}
		}
	}
	else if ( currentx >= 0 )
	{
		if ( assistShrineGUI.currentView == GenericGUIMenu::AssistShrineGUI_t::ASSIST_SHRINE_VIEW_ITEMS )
		{
			assistShrineGUI.selectAssistShrineSlot(assistShrineGUI.ASSIST_SLOT_CLOAK, 0);
			return;
		}
		else if ( assistShrineGUI.currentView == GenericGUIMenu::AssistShrineGUI_t::ASSIST_SHRINE_VIEW_RACE )
		{
			if ( diffx != 0 )
			{
				x = assistShrineGUI.ASSIST_RACE_COLUMN;
				y = std::min((int)assistShrineGUI.raceSlots.size(), std::max(0, y));
			}
			else if ( diffy != 0 )
			{
				if ( diffy > 0 )
				{
					y = currenty + 1;
				}
				else if ( diffy < 0 )
				{
					y = currenty - 1;
				}

				x = assistShrineGUI.ASSIST_RACE_COLUMN;
				y = std::min(std::max(0, y), lowestItemY);
			}
		}
		else if ( assistShrineGUI.currentView == GenericGUIMenu::AssistShrineGUI_t::ASSIST_SHRINE_VIEW_CLASSES )
		{
			if ( diffy != 0 )
			{
				if ( diffy > 0 )
				{
					auto slotFrame = assistShrineGUI.getAssistShrineSlotFrame(x, currenty + 1);
					if ( slotFrame && !slotFrame->isDisabled() )
					{
						y = currenty + 1;
					}
					else
					{
						// no move
						// check slots to the left
						x = currentx;
						bool found = false;
						while ( x > 0 )
						{
							--x;
							auto slotFrame = assistShrineGUI.getAssistShrineSlotFrame(x, currenty + 1);
							if ( slotFrame && !slotFrame->isDisabled() )
							{
								y = currenty + 1;
								found = true;
								break;
							}
						}
						if ( !found )
						{
							x = currentx;
							y = currenty;
						}
					}
				}
				else
				{
					auto slotFrame = assistShrineGUI.getAssistShrineSlotFrame(x, currenty - 1);
					if ( slotFrame && !slotFrame->isDisabled() )
					{
						y = currenty - 1;
					}
					else
					{
						// no move
					}
				}
			}
			else if ( diffx != 0 )
			{
				if ( diffx > 0 )
				{
					auto slotFrame = assistShrineGUI.getAssistShrineSlotFrame(currentx + 1, y);
					if ( slotFrame && !slotFrame->isDisabled() )
					{
						x = currentx + 1;
					}
					else
					{
						// no move
					}
				}
				else
				{
					auto slotFrame = assistShrineGUI.getAssistShrineSlotFrame(currentx - 1, y);
					if ( slotFrame && !slotFrame->isDisabled() )
					{
						x = currentx - 1;
					}
					else
					{
						// no move
					}
				}
			}

			x = std::min(std::max(0, x), assistShrineGUI.MAX_ASSISTSHRINE_X - 1);
			y = std::min(std::max(0, y), lowestItemY);

			Frame* slotFrame = assistShrineGUI.getAssistShrineSlotFrame(x, y);
			if ( !slotFrame || slotFrame->isDisabled() )
			{
				// dest slot not valid, reset to current
				x = std::min(std::max(0, currentx), assistShrineGUI.MAX_ASSISTSHRINE_X - 1);
				y = std::min(std::max(0, currenty), lowestItemY);
				slotFrame = assistShrineGUI.getAssistShrineSlotFrame(x, y);
				if ( !slotFrame || slotFrame->isDisabled() )
				{
					// current slot not valid, reset to 0, 0
					assistShrineGUI.selectAssistShrineSlot(0, 0);
					assistShrineGUI.scrollToSlot(0, 0, false);
					return;
				}
			}
		}
	}
	assistShrineGUI.selectAssistShrineSlot(x, y);
	if ( assistShrineGUI.currentView != assistShrineGUI.ASSIST_SHRINE_VIEW_ITEMS )
	{
		assistShrineGUI.scrollToSlot(x, y, false);
	}
}

// only called by handleInventoryMovement in player.cpp
void select_alchemy_slot(int player, int currentx, int currenty, int diffx, int diffy)
{
	int x = currentx + diffx;
	int y = currenty + diffy;

	auto& alchemyGUI = GenericGUI[player].alchemyGUI;
	int lowestItemY = alchemyGUI.recipes.getNumRecipesToDisplayVertical() - 1;
	if ( !alchemyGUI.recipes.bOpen )
	{
		lowestItemY = 0;
	}

	if ( currentx < 0 )
	{
		if ( diffy != 0 )
		{
			// main alchemy frame
			if ( currentx == alchemyGUI.ALCH_SLOT_RESULT_POTION_X )
			{
				x = alchemyGUI.ALCH_SLOT_BASE_POTION_X;
			}
			else if ( currentx == alchemyGUI.ALCH_SLOT_BASE_POTION_X || currentx == alchemyGUI.ALCH_SLOT_SECONDARY_POTION_X )
			{
				x = alchemyGUI.ALCH_SLOT_RESULT_POTION_X;
			}
			y = 0;
		}
		else if ( diffx != 0 )
		{
			if ( currentx == alchemyGUI.ALCH_SLOT_BASE_POTION_X )
			{
				if ( diffx > 0 )
				{
					x = alchemyGUI.ALCH_SLOT_SECONDARY_POTION_X;
					y = 0;
				}
				else
				{
					if ( alchemyGUI.recipes.bOpen && alchemyGUI.recipes.panelJustifyInverted )
					{
						x = alchemyGUI.MAX_ALCH_X - 1;
						y = 0;
					}
					else
					{
						players[player]->inventoryUI.selectSlot(players[player]->inventoryUI.getSizeX() - 1, 0);
						players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
						return;
					}
				}
			}
			else if ( currentx == alchemyGUI.ALCH_SLOT_SECONDARY_POTION_X )
			{
				if ( diffx > 0 )
				{
					if ( alchemyGUI.recipes.bOpen && !alchemyGUI.recipes.panelJustifyInverted )
					{
						x = 0;
						y = 0;
					}
					else
					{
						players[player]->inventoryUI.selectSlot(0, 0);
						players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
						return;
					}
				}
				else
				{
					x = alchemyGUI.ALCH_SLOT_BASE_POTION_X;
					y = 0;
				}
			}
			else
			{
				if ( diffx > 0 )
				{
					if ( alchemyGUI.recipes.bOpen && !alchemyGUI.recipes.panelJustifyInverted )
					{
						x = 0;
						y = alchemyGUI.MAX_ALCH_Y - 1;
					}
					else
					{
						players[player]->inventoryUI.selectSlot(0, 5);
						players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
						return;
					}
				}
				else
				{
					if ( alchemyGUI.recipes.bOpen && alchemyGUI.recipes.panelJustifyInverted )
					{
						x = alchemyGUI.MAX_ALCH_X - 1;
						y = alchemyGUI.MAX_ALCH_Y - 1;
					}
					else
					{
						players[player]->inventoryUI.selectSlot(players[player]->inventoryUI.getSizeX() - 1, 5);
						players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
						return;
					}
				}
			}
		}
	}
	else if ( currentx >= 0 )
	{
		if ( alchemyGUI.recipes.bOpen )
		{
			if ( diffy != 0 )
			{
				if ( y < 0 )
				{
					y = lowestItemY;
				}
				if ( y > lowestItemY )
				{
					y = 0;
				}
				x = std::min(std::max(0, currentx), alchemyGUI.MAX_ALCH_X - 1);
			}
			else if ( diffx != 0 )
			{
				//if ( diffx > 0 )
				{
					if ( x >= alchemyGUI.MAX_ALCH_X )
					{
						if ( !alchemyGUI.recipes.panelJustifyInverted )
						{
							players[player]->inventoryUI.selectSlot(0, currenty);
							players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
							return;
						}
						else
						{
							if ( y < alchemyGUI.MAX_ALCH_Y - 2 )
							{
								x = alchemyGUI.ALCH_SLOT_BASE_POTION_X;
								y = 0;
							}
							else
							{
								x = alchemyGUI.ALCH_SLOT_RESULT_POTION_X;
								y = 0;
							}
						}
					}
					else if ( x < 0 )
					{
						if ( !alchemyGUI.recipes.panelJustifyInverted )
						{
							if ( y < alchemyGUI.MAX_ALCH_Y - 2 )
							{
								x = alchemyGUI.ALCH_SLOT_SECONDARY_POTION_X;
								y = 0;
							}
							else
							{
								x = alchemyGUI.ALCH_SLOT_RESULT_POTION_X;
								y = 0;
							}
						}
						else
						{
							players[player]->inventoryUI.selectSlot(players[player]->inventoryUI.getSizeX() - 1, currenty);
							players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
							return;
						}
					}
				}
			}
		}
		else
		{
			x = alchemyGUI.ALCH_SLOT_BASE_POTION_X;
			y = 0;
		}
	}
	alchemyGUI.selectAlchemySlot(x, y);
}

// only called by handleInventoryMovement in player.cpp
void select_spell_slot(int player, int currentx, int currenty, int diffx, int diffy)
{
	int x = currentx + diffx;
	int y = currenty + diffy;

	if ( !players[player]->inventoryUI.spellPanel.bFirstTimeSnapCursor )
	{
		return;
	}

	if ( x < 0 ) 
	{ 
		x = Player::Inventory_t::MAX_SPELLS_X - 1; 
	}
	if ( x >= Player::Inventory_t::MAX_SPELLS_X ) 
	{ 
		x = 0;
	}

	int lowestItemY = players[player]->inventoryUI.spellPanel.getNumSpellsToDisplayVertical() - 1;
	for ( node_t* node = stats[player]->inventory.first; node != NULL; node = node->next )
	{
		Item* item = (Item*)node->element;
		if ( !item ) { continue; }
		if ( itemCategory(item) != SPELL_CAT ) { continue; }

		lowestItemY = std::max(lowestItemY, item->y);
	}
	if ( y < 0 ) 
	{ 
		y = lowestItemY;
	}
	if ( y > lowestItemY )
	{ 
		y = 0;
	}
	players[player]->inventoryUI.selectSpell(x, y);
	players[player]->inventoryUI.spellPanel.scrollToSlot(x, y, false);
}

// only called by handleInventoryMovement in player.cpp
void select_inventory_slot(int player, int currentx, int currenty, int diffx, int diffy)
{
	auto& selectedItem = inputs.getUIInteraction(player)->selectedItem;
	auto& inventoryUI = players[player]->inventoryUI;
	auto paperDollSlot = Player::PaperDoll_t::PaperDollSlotType::SLOT_MAX;

	int x = currentx + diffx;
	int y = currenty + diffy;

	if ( !selectedItem && GenericGUI[player].tinkerGUI.isConstructMenuActive()
		&& players[player]->GUI.activeModule != Player::GUI_t::MODULE_TINKERING )
	{
		// prevent inventory moving if tinkering not selected when it should be
		select_tinkering_slot(player, x, y, 0, 0);
		players[player]->GUI.activateModule(Player::GUI_t::MODULE_TINKERING);
		return;
	}
	else if ( !selectedItem && GenericGUI[player].featherGUI.isInscriptionDrawerOpen()
		&& players[player]->GUI.activeModule != Player::GUI_t::MODULE_FEATHER )
	{
		// prevent inventory moving if feather not selected when it should be
		select_feather_slot(player, x, y, 0, 0);
		players[player]->GUI.activateModule(Player::GUI_t::MODULE_FEATHER);
		return;
	}

	if ( selectedItem )
	{
		auto selectedItemDollSlot = getPaperDollSlotFromItemType(*selectedItem);
		if ( selectedItemDollSlot != Player::PaperDoll_t::PaperDollSlotType::SLOT_MAX )
		{
			if ( currenty < 0 ) // moving from doll
			{
				if ( inventoryUI.bCompactView )
				{
					if ( diffx < 0 )
					{
						x = inventoryUI.getSizeX() - 1;
					}
					else if ( diffx > 0 )
					{
						x = 0;
					}
					y = currenty; // y no effect.

					if ( diffx != 0 )
					{
						if ( openedChest[player] && players[player]->inventoryUI.chestGUI.bOpen )
						{
							if ( diffx > 0 )
							{
								x = 0;
							}
							else if ( diffx < 0 )
							{
								x = players[player]->inventoryUI.MAX_CHEST_X - 1;
							}
							select_chest_slot(player, x, 0, 0, 0);
							players[player]->GUI.activateModule(Player::GUI_t::MODULE_CHEST);
							return;
						}

						if ( currenty == Player::Inventory_t::DOLL_ROW_1 )
						{
							y = 0;
						}
						else if ( currenty == Player::Inventory_t::DOLL_ROW_2 )
						{
							y = 1;
						}
						else if ( currenty == Player::Inventory_t::DOLL_ROW_3 )
						{
							y = 2;
						}
						else if ( currenty == Player::Inventory_t::DOLL_ROW_4 )
						{
							y = 3;
						}
						else if ( currenty == Player::Inventory_t::DOLL_ROW_5 )
						{
							y = 4;
						}
						else
						{
							y = 5;
						}
					}
				}
				else
				{
					if ( diffy < 0 )
					{
						y = inventoryUI.getSizeY() - 1;
					}
					else if ( diffy > 0 )
					{
						y = 0;
					}
					else if ( diffx != 0 )
					{
						if ( openedChest[player] && players[player]->inventoryUI.chestGUI.bOpen )
						{
							if ( diffx > 0 )
							{
								x = 0;
							}
							else if ( diffx < 0 )
							{
								x = players[player]->inventoryUI.MAX_CHEST_X - 1;
							}
							select_chest_slot(player, x, 0, 0, 0);
							players[player]->GUI.activateModule(Player::GUI_t::MODULE_CHEST);
							return;
						}
					}
					x = currentx; // x no effect.

					if ( diffy != 0 )
					{
						if ( currentx == Player::Inventory_t::DOLL_COLUMN_LEFT )
						{
							x = 0;
						}
						else
						{
							x = inventoryUI.getSizeX() - 1;
						}
					}
				}

				inventoryUI.selectSlot(x, y);
				return;
			}
			else
			{
				if ( inventoryUI.bCompactView )
				{
					if ( openedChest[player] && players[player]->inventoryUI.chestGUI.bOpen )
					{
						if ( x >= inventoryUI.getSizeX() )
						{
							x = 0;
							if ( y >= players[player]->inventoryUI.MAX_CHEST_Y )
							{
								y = players[player]->inventoryUI.MAX_CHEST_Y - 1;
							}
							else if ( y < 0 )
							{
								y = 0;
							}
							select_chest_slot(player, x, y, 0, 0);
							players[player]->GUI.activateModule(Player::GUI_t::MODULE_CHEST);
							return;
						}
						else if ( x < 0 )
						{
							x = players[player]->inventoryUI.MAX_CHEST_X - 1;
							if ( y >= players[player]->inventoryUI.MAX_CHEST_Y )
							{
								y = players[player]->inventoryUI.MAX_CHEST_Y - 1;
							}
							else if ( y < 0 )
							{
								y = 0;
							}
							select_chest_slot(player, x, y, 0, 0);
							players[player]->GUI.activateModule(Player::GUI_t::MODULE_CHEST);
							return;
						}
					}
					else
					{
						bool skipPaperDollSelection = false;
						if ( GenericGUI[player].tinkerGUI.bOpen 
							|| GenericGUI[player].alchemyGUI.bOpen 
							|| GenericGUI[player].assistShrineGUI.bOpen
							|| GenericGUI[player].featherGUI.bOpen )
						{
							skipPaperDollSelection = true;
						}
						if ( x >= inventoryUI.getSizeX() )
						{
							if ( !skipPaperDollSelection )
							{
								players[player]->paperDoll.selectPaperDollCoordinatesFromSlotType(selectedItemDollSlot);
								return;
							}
						}
						else if ( x < 0 )
						{
							if ( !skipPaperDollSelection )
							{
								players[player]->paperDoll.selectPaperDollCoordinatesFromSlotType(selectedItemDollSlot);
								return;
							}
						}
					}
				}
				else
				{
					bool skipPaperDollSelection = false;
					if ( players[player]->inventoryUI.isItemFromChest(selectedItem) )
					{
						for ( auto& slot : players[player]->paperDoll.dollSlots )
						{
							if ( slot.slotType == selectedItemDollSlot && slot.item != 0 )
							{
								// disallow navigation to taken paper doll slots
								skipPaperDollSelection = true;
								if ( y < 0 )
								{
									y = inventoryUI.getSizeY() - 1;
								}
								if ( y >= inventoryUI.getSizeY() )
								{
									y = 0;
								}
								break;
							}
						}
					}

					if ( !skipPaperDollSelection )
					{
						if ( y >= inventoryUI.getSizeY() )
						{
							players[player]->paperDoll.selectPaperDollCoordinatesFromSlotType(selectedItemDollSlot);
							return;
						}
						else if ( y < 0 )
						{
							players[player]->paperDoll.selectPaperDollCoordinatesFromSlotType(selectedItemDollSlot);
							return;
						}
					}
				}
			}
		}
		else
		{
			if ( y < 0 )
			{
				y = inventoryUI.getSizeY() - 1;
			}
			if ( y >= inventoryUI.getSizeY() )
			{
				y = 0;
			}
		}
	}

	if ( players[player]->paperDoll.enabled )
	{
		paperDollSlot = players[player]->paperDoll.paperDollSlotFromCoordinates(x, y);
	}
	bool doPaperDollMovement = (paperDollSlot != Player::PaperDoll_t::PaperDollSlotType::SLOT_MAX);

	if ( doPaperDollMovement )
	{
		if ( !restrictPaperDollMovement )
		{
			players[player]->paperDoll.selectPaperDollCoordinatesFromSlotType(paperDollSlot);
			return;
		}
		else
		{
			// moveInPaperDoll will reject movement for compact view tinkering, shops, chests etc by returning false
			if ( moveInPaperDoll(player, paperDollSlot, currentx, currenty, diffx, diffy, x, y) )
			{
				return;
			}
		}
	}

	if ( x < 0 )   //Wrap around left boundary.
	{
		if ( openedChest[player] && players[player]->inventoryUI.chestGUI.bOpen )
		{
			if ( y >= players[player]->inventoryUI.MAX_CHEST_Y )
			{
				y = players[player]->inventoryUI.MAX_CHEST_Y - 1;
			}
			else if ( y < 0 )
			{
				y = 0;
			}
			select_chest_slot(player, players[player]->inventoryUI.MAX_CHEST_X - 1, y, 0, 0);
			players[player]->GUI.activateModule(Player::GUI_t::MODULE_CHEST);
			return;
		}
		else if ( !selectedItem && GenericGUI[player].assistShrineGUI.bOpen )
		{
			if ( GenericGUI[player].assistShrineGUI.currentView == GenericGUI[player].assistShrineGUI.ASSIST_SHRINE_VIEW_RACE )
			{
				select_assistshrine_slot(player, GenericGUI[player].assistShrineGUI.ASSIST_RACE_COLUMN, 0, 0, 0);
			}
			else if ( GenericGUI[player].assistShrineGUI.currentView == GenericGUI[player].assistShrineGUI.ASSIST_SHRINE_VIEW_CLASSES )
			{
				select_assistshrine_slot(player, 0, 0, 0, 0);
			}
			else if ( GenericGUI[player].assistShrineGUI.currentView == GenericGUI[player].assistShrineGUI.ASSIST_SHRINE_VIEW_ITEMS )
			{
				select_assistshrine_slot(player, GenericGUI[player].assistShrineGUI.ASSIST_SLOT_RING, 0, 0, 0);
			}
			players[player]->GUI.activateModule(Player::GUI_t::MODULE_ASSISTSHRINE);
			return;
		}
		else if ( !selectedItem && GenericGUI[player].alchemyGUI.bOpen )
		{
			if ( y >= GenericGUI[player].alchemyGUI.MAX_ALCH_Y )
			{
				y = GenericGUI[player].alchemyGUI.MAX_ALCH_Y - 1;
			}
			else if ( y < 0 )
			{
				y = 0;
			}
			if ( GenericGUI[player].alchemyGUI.recipes.bOpen && !GenericGUI[player].alchemyGUI.recipes.panelJustifyInverted )
			{
				select_alchemy_slot(player, GenericGUI[player].alchemyGUI.MAX_ALCH_X - 1, y, 0, 0);
				players[player]->GUI.activateModule(Player::GUI_t::MODULE_ALCHEMY);
				return;
			}
			else
			{
				if ( y >= 4 )
				{
					select_alchemy_slot(player, GenericGUI[player].alchemyGUI.ALCH_SLOT_RESULT_POTION_X, 0, 0, 0);
				}
				else
				{
					select_alchemy_slot(player, GenericGUI[player].alchemyGUI.ALCH_SLOT_SECONDARY_POTION_X, 0, 0, 0);
				}
				players[player]->GUI.activateModule(Player::GUI_t::MODULE_ALCHEMY);
				return;
			}
		}
		else if ( !selectedItem && players[player]->gui_mode == GUI_MODE_SHOP && players[player]->shopGUI.bOpen )
		{
			if ( y >= players[player]->shopGUI.MAX_SHOP_Y )
			{
				y = players[player]->shopGUI.MAX_SHOP_Y - 1;
			}
			else if ( y < 0 )
			{
				y = 0;
			}
			select_shop_slot(player, players[player]->shopGUI.MAX_SHOP_X - 1, y, 0, 0);
			players[player]->GUI.activateModule(Player::GUI_t::MODULE_SHOP);
			return;
		}
		else
		{
			x = inventoryUI.getSizeX() - 1;
		}
	}

	if ( x >= inventoryUI.getSizeX() )   //Wrap around right boundary.
	{
		if ( openedChest[player] && players[player]->inventoryUI.chestGUI.bOpen )
		{
			if ( y >= players[player]->inventoryUI.MAX_CHEST_Y )
			{
				y = players[player]->inventoryUI.MAX_CHEST_Y - 1;
			}
			else if ( y < 0 )
			{
				y = 0;
			}
			select_chest_slot(player, 0, y, 0, 0);
			players[player]->GUI.activateModule(Player::GUI_t::MODULE_CHEST);
			return;
		}
		else if ( !selectedItem && GenericGUI[player].alchemyGUI.bOpen )
		{
			if ( y >= GenericGUI[player].alchemyGUI.MAX_ALCH_Y )
			{
				y = GenericGUI[player].alchemyGUI.MAX_ALCH_Y - 1;
			}
			else if ( y < 0 )
			{
				y = 0;
			}
			if ( GenericGUI[player].alchemyGUI.recipes.bOpen && GenericGUI[player].alchemyGUI.recipes.panelJustifyInverted )
			{
				select_alchemy_slot(player, 0, y, 0, 0);
				players[player]->GUI.activateModule(Player::GUI_t::MODULE_ALCHEMY);
				return;
			}
			else
			{
				if ( y >= 4 )
				{
					select_alchemy_slot(player, GenericGUI[player].alchemyGUI.ALCH_SLOT_RESULT_POTION_X, 0, 0, 0);
				}
				else
				{
					select_alchemy_slot(player, GenericGUI[player].alchemyGUI.ALCH_SLOT_BASE_POTION_X, 0, 0, 0);
				}
				players[player]->GUI.activateModule(Player::GUI_t::MODULE_ALCHEMY);
				return;
			}
		}
		else if ( !selectedItem && GenericGUI[player].assistShrineGUI.bOpen )
		{
			if ( GenericGUI[player].assistShrineGUI.currentView == GenericGUI[player].assistShrineGUI.ASSIST_SHRINE_VIEW_RACE )
			{
				select_assistshrine_slot(player, GenericGUI[player].assistShrineGUI.ASSIST_RACE_COLUMN, 0, 0, 0);
			}
			else if ( GenericGUI[player].assistShrineGUI.currentView == GenericGUI[player].assistShrineGUI.ASSIST_SHRINE_VIEW_CLASSES )
			{
				select_assistshrine_slot(player, 0, 0, 0, 0);
			}
			else if ( GenericGUI[player].assistShrineGUI.currentView == GenericGUI[player].assistShrineGUI.ASSIST_SHRINE_VIEW_ITEMS )
			{
				select_assistshrine_slot(player, GenericGUI[player].assistShrineGUI.ASSIST_SLOT_CLOAK, 0, 0, 0);
			}
			players[player]->GUI.activateModule(Player::GUI_t::MODULE_ASSISTSHRINE);
			return;
		}
		else if ( !selectedItem && players[player]->gui_mode == GUI_MODE_SHOP && players[player]->shopGUI.bOpen )
		{
			if ( y >= players[player]->shopGUI.MAX_SHOP_Y )
			{
				y = players[player]->shopGUI.MAX_SHOP_Y - 1;
			}
			else if ( y < 0 )
			{
				y = 0;
			}
			select_shop_slot(player, 0, y, 0, 0);
			players[player]->GUI.activateModule(Player::GUI_t::MODULE_SHOP);
			return;
		}
		else
		{
			x = 0;
		}
	}

	if ( inventoryUI.selectedSlotInPaperDoll() )
	{
		auto oldSlot = players[player]->paperDoll.paperDollSlotFromCoordinates(inventoryUI.getSelectedSlotX(), inventoryUI.getSelectedSlotY());
		if ( !inventoryUI.bCompactView )
		{
			if ( oldSlot >= Player::PaperDoll_t::PaperDollSlotType::SLOT_HELM )
			{
				// right column
				x = inventoryUI.getSizeX() - 1;
			}
			else
			{
				// left column
				x = 0;
			}
		}
		else if ( inventoryUI.bCompactView )
		{
			if ( diffx < 0 )
			{
				x = inventoryUI.getSizeX() - 1;
			}
			else if ( diffx > 0 )
			{
				x = 0;
			}
		}

		if ( inventoryUI.bCompactView )
		{
			if ( oldSlot >= Player::PaperDoll_t::PaperDollSlotType::SLOT_HELM )
			{
				y = (int)(oldSlot - Player::PaperDoll_t::PaperDollSlotType::SLOT_HELM);
			}
			else
			{
				y = (int)oldSlot;
			}
		}
		else
		{
			if ( y < Player::Inventory_t::DOLL_ROW_1 )
			{
				y = inventoryUI.getSizeY() - 1;
			}
			else if ( y >= Player::Inventory_t::DOLL_ROW_1 && y <= Player::Inventory_t::DOLL_ROW_5 )
			{
				// should not happen, failsafe.
				y = 0;
			}
		}
	}

	auto& hotbar_t = players[player]->hotbar;

	if ( y < 0 )   //Wrap around top to bottom.
	{
		y = inventoryUI.getSizeY() - 1;
	}
	if ( y >= inventoryUI.getSizeY() )   //Hit bottom. Wrap around or go to shop/chest?
	{
		y = 0;
	}

	inventoryUI.selectSlot(x, y);
}

std::string getItemSpritePath(const int player, Item& item)
{
	if ( item.type == SPELL_ITEM )
	{
		return ItemTooltips.getSpellIconPath(player, item, -1);
	}
	else
	{
		node_t* imagePathsNode = nullptr;
		if ( item.type == TOOL_PLAYER_LOOT_BAG )
		{
			if ( colorblind_lobby )
			{
				int playerOwner = item.getLootBagPlayer();
				Uint32 index = 4;
				switch ( playerOwner )
				{
					case 0:
						index = 2;
						break;
					case 1:
						index = 3;
						break;
					case 2:
						index = 1;
						break;
					case 3:
						index = 4;
						break;
					default:
						break;
				}

				imagePathsNode = list_Node(&items[item.type].images, index);
			}
			else
			{
				imagePathsNode = list_Node(&items[item.type].images, item.getLootBagPlayer());
			}
		}
		else
		{
			imagePathsNode = list_Node(&items[item.type].images, item.appearance % items[item.type].variations);
		}
		if ( imagePathsNode )
		{
			string_t* imagePath = static_cast<string_t*>(imagePathsNode->element);
			return imagePath->data;
		}
	}
	return "";
}

Item* takeItemFromChest(int player, Item* item, int amount, Item* addToSpecificInventoryItem, bool forceNewStack, bool bDoPickupMessage)
{
	if ( !openedChest[player] || !item )
	{
		return nullptr;
	}
	list_t* chest_inventory = nullptr;
	if ( multiplayer == CLIENT )
	{
		chest_inventory = &chestInv[player];
	}
	else if ( openedChest[player]->children.first && openedChest[player]->children.first->element )
	{
		chest_inventory = (list_t*)openedChest[player]->children.first->element;
	}

	if ( !chest_inventory || !item->node || item->node->list != chest_inventory )
	{
		// item isn't a part of this chest?
		return nullptr;
	}

	Item* checkExists = openedChest[player]->getItemFromChest(item, amount, true);
	if ( !checkExists )
	{
		// could not get item info
		return nullptr;
	}
	free(checkExists);
	checkExists = nullptr;

	// deletes 'item' from the chest inventory, if remaining qty is 0
	int oldcount = item->count;
	Item* itemCopyToTake = openedChest[player]->getItemFromChest(item, amount); 
	if ( bDoPickupMessage )
	{
		messagePlayer(player, MESSAGE_INVENTORY, Language::get(374), itemCopyToTake->description());
		playSound(35 + local_rng.rand() % 3, 64);
	}
	Item* pickedUp = itemPickup(player, itemCopyToTake, addToSpecificInventoryItem, forceNewStack);
	free(itemCopyToTake);
	itemCopyToTake = nullptr;
	
	return pickedUp;
}

bool dragDropStackChestItems(const int player, Item*& selectedItem, Item* tempItem, int oldx, int oldy)
{
	if ( !openedChest[player] || !selectedItem || !tempItem || selectedItem == tempItem )
	{
		return false;
	}
	list_t* chest_inventory = nullptr;
	if ( multiplayer == CLIENT )
	{
		chest_inventory = &chestInv[player];
	}
	else if ( openedChest[player]->children.first && openedChest[player]->children.first->element )
	{
		chest_inventory = (list_t*)openedChest[player]->children.first->element;
	}

	if ( !chest_inventory || !selectedItem->node || selectedItem->node->list != chest_inventory
		|| !tempItem->node || tempItem->node->list != chest_inventory )
	{
		// item isn't a part of this chest?
		return false;
	}

	bool& toggleclick = inputs.getUIInteraction(player)->toggleclick;
	bool stackedItems = false;
	if ( !itemCompare(selectedItem, tempItem, false) )
	{
		int selectedItemQty = 0;
		int destItemQty = 0;
		auto stackingResult = getItemStackingBehavior(player, selectedItem, tempItem, selectedItemQty, destItemQty);
		if ( stackingResult.resultType == ITEM_ADDED_ENTIRELY_TO_DESTINATION_STACK )
		{
			// items have stacked entirely
			Item* itemCopyToTake = openedChest[player]->getItemFromChest(selectedItem, selectedItem->count);
			if ( itemCopyToTake )
			{
				assert(selectedItem == nullptr);
				if ( Item* chestItem = openedChest[player]->addItemToChest(itemCopyToTake, false, tempItem) )
				{
					assert(chestItem == tempItem);
					assert(chestItem->count == destItemQty);
				}
				else
				{
					messagePlayer(0, MESSAGE_DEBUG, "ERROR STACKING CHEST ITEM");
				}
				free(itemCopyToTake);
				itemCopyToTake = nullptr;
			}
			selectedItem = nullptr;
			inputs.getUIInteraction(player)->selectedItemFromChest = 0;
			toggleclick = false;
			stackedItems = true;
		}
		else if ( stackingResult.resultType == ITEM_ADDED_PARTIALLY_TO_DESTINATION_STACK )
		{
			int qtyToTake = selectedItem->count - selectedItemQty;
			int oldQty = selectedItem->count;
			Item* itemCopyToTake = openedChest[player]->getItemFromChest(selectedItem, qtyToTake);
			assert(selectedItem && selectedItem->count == selectedItemQty);
			assert(itemCopyToTake);

			if ( Item* chestItem = openedChest[player]->addItemToChest(itemCopyToTake, false, tempItem) )
			{
				assert(chestItem != itemCopyToTake);
				selectedItem->x = oldx;
				selectedItem->y = oldy;
				toggleclick = true;
				stackedItems = true;
			}
			else
			{
				messagePlayer(0, MESSAGE_DEBUG, "ERROR STACKING CHEST ITEM");
				if ( selectedItem )
				{
					selectedItem->count = oldQty;
					selectedItem->x = oldx;
					selectedItem->y = oldy;
				}
				selectedItem = nullptr;
				inputs.getUIInteraction(player)->selectedItemFromChest = 0;
				toggleclick = false;
			}
			free(itemCopyToTake);
			itemCopyToTake = nullptr;
		}
		else if ( stackingResult.resultType == ITEM_DESTINATION_STACK_IS_FULL )
		{
			/*selectedItem->x = oldx;
			selectedItem->y = oldy;
			selectedItem = nullptr;
			inputs.getUIInteraction(player)->selectedItemFromChest = 0;
			toggleclick = false;
			stackedItems = true;*/
		}
	}

	return stackedItems;
}

bool dragDropStackInventoryItems(const int player, Item*& selectedItem, Item* tempItem, int oldx, int oldy)
{
	bool& toggleclick = inputs.getUIInteraction(player)->toggleclick;
	bool stackedItems = false;
	if ( !itemCompare(selectedItem, tempItem, false) )
	{
		int selectedItemQty = 0;
		int destItemQty = 0;
		auto stackingResult = getItemStackingBehavior(player, selectedItem, tempItem, selectedItemQty, destItemQty);
		if ( stackingResult.resultType == ITEM_ADDED_ENTIRELY_TO_DESTINATION_STACK )
		{
			// items have stacked entirely
			Item* pickedUp = itemPickup(player, selectedItem, tempItem, false);
			if ( !pickedUp )
			{
				messagePlayer(0, MESSAGE_DEBUG, "ERROR STACKING ITEM");
			}
			if ( selectedItem->node )
			{
				list_RemoveNode(selectedItem->node);
			}
			else
			{
				free(selectedItem);
			}
			selectedItem = nullptr;
			inputs.getUIInteraction(player)->selectedItemFromChest = 0;
			toggleclick = false;
			stackedItems = true;
		}
		else if ( stackingResult.resultType == ITEM_ADDED_PARTIALLY_TO_DESTINATION_STACK )
		{
			int qtyToTake = selectedItem->count - selectedItemQty;
			int oldQty = selectedItem->count;
			selectedItem->count = qtyToTake;
			Item* pickedUp = itemPickup(player, selectedItem, tempItem, false);
			if ( pickedUp )
			{
				selectedItem->count = selectedItemQty;
				selectedItem->x = oldx;
				selectedItem->y = oldy;
				toggleclick = true;
				stackedItems = true;
			}
			else
			{
				messagePlayer(0, MESSAGE_DEBUG, "ERROR STACKING ITEM");
				if ( selectedItem )
				{
					selectedItem->count = oldQty;
					selectedItem->x = oldx;
					selectedItem->y = oldy;
				}
				selectedItem = nullptr;
				inputs.getUIInteraction(player)->selectedItemFromChest = 0;
				toggleclick = false;
			}
		}
		else if ( stackingResult.resultType == ITEM_DESTINATION_STACK_IS_FULL )
		{
			/*selectedItem->x = oldx;
			selectedItem->y = oldy;
			selectedItem = nullptr;
			inputs.getUIInteraction(player)->selectedItemFromChest = 0;
			toggleclick = false;
			stackedItems = true;*/
		}
	}

	return stackedItems;
}

bool releaseInventoryItemIntoAlchemy(const int player)
{
	return false; // disable for now
	Item*& selectedItem = inputs.getUIInteraction(player)->selectedItem;
	if ( !selectedItem )
	{
		return false;
	}
	auto& alchemyGUI = GenericGUI[player].alchemyGUI;
	if ( !alchemyGUI.bOpen || !alchemyGUI.isInteractable )
	{
		return false;
	}

	if ( !(itemCategory(selectedItem) == POTION && selectedItem->type != POTION_EMPTY)
		|| itemIsEquipped(selectedItem, player) || !selectedItem->identified )
	{
		return false;
	}

	if ( auto slotFrame = alchemyGUI.getAlchemySlotFrame(GenericGUIMenu::AlchemyGUI_t::ALCH_SLOT_BASE_POTION_X, 0) )
	{
		if ( slotFrame->capturesMouseInRealtimeCoords() )
		{
			if ( selectedItem->uid != alchemyGUI.potion1Uid )
			{
				alchemyGUI.potion1Uid = selectedItem->uid;
				alchemyGUI.recipes.activateRecipeIndex = -1;
				if ( selectedItem->uid == alchemyGUI.potion2Uid )
				{
					alchemyGUI.potion2Uid = 0;
					alchemyGUI.animPotion2 = 0.0;
				}
			}
			return true;
		}
	}
	if ( auto slotFrame = alchemyGUI.getAlchemySlotFrame(GenericGUIMenu::AlchemyGUI_t::ALCH_SLOT_SECONDARY_POTION_X, 0) )
	{
		if ( slotFrame->capturesMouseInRealtimeCoords() )
		{
			if ( selectedItem->uid != alchemyGUI.potion2Uid )
			{
				alchemyGUI.potion2Uid = selectedItem->uid;
				alchemyGUI.recipes.activateRecipeIndex = -1;
				if ( selectedItem->uid == alchemyGUI.potion1Uid )
				{
					alchemyGUI.potion1Uid = 0;
					alchemyGUI.animPotion1 = 0.0;
				}
			}
			return true;
		}
	}
	return false;
}

void releaseChestItem(const int player)
{
	Item*& selectedItem = inputs.getUIInteraction(player)->selectedItem;
	if ( !selectedItem )
	{
		return;
	}

	int& selectedItemFromHotbar = inputs.getUIInteraction(player)->selectedItemFromHotbar;
	Uint32& selectedItemFromChest = inputs.getUIInteraction(player)->selectedItemFromChest;
	Frame* frame = players[player]->inventoryUI.frame;
	bool& toggleclick = inputs.getUIInteraction(player)->toggleclick;

	const int UNKNOWN_SLOT = -10;
	int slotFrameX = UNKNOWN_SLOT;
	int slotFrameY = UNKNOWN_SLOT;

	bool mouseOverSlot = getSlotFrameXYFromMousePos(player, slotFrameX, slotFrameY, itemCategory(selectedItem) == SPELL_CAT);
	bool mouseInInventory = mouseInsidePlayerInventory(player);
	bool mouseInChest = false;
	if ( !mouseInInventory )
	{
		if ( players[player]->inventoryUI.chestFrame
			&& !players[player]->inventoryUI.chestFrame->isDisabled()
			&& openedChest[player] )
		{
			if ( auto chestSlots = players[player]->inventoryUI.chestFrame->findFrame("chest slots") )
			{
				if ( !chestSlots->isDisabled() && chestSlots->capturesMouse() )
				{
					mouseInChest = true;
				}
			}
		}
	}

	if ( mouseOverSlot && mouseInInventory && slotFrameY > Player::Inventory_t::DOLL_ROW_5 )
	{
		//if ( !players[player]->inventoryUI.bItemInventoryHasFreeSlot() )
		//{
		//	// can't drag off into inventory, no slots available
		//	messagePlayer(player, MESSAGE_INVENTORY, Language::get(727), selectedItem->getName());
		//	selectedItem = nullptr;
		//	inputs.getUIInteraction(player)->selectedItemFromChest = 0;
		//	toggleclick = false;
		//	if ( inputs.bMouseLeft(player) )
		//	{
		//		inputs.mouseClearLeft(player);
		//	}
		//	return;
		//}

		// within inventory
		int oldx = selectedItem->x;
		int oldy = selectedItem->y;
		selectedItem->x = slotFrameX;
		selectedItem->y = slotFrameY;

		Item* swappedItem = nullptr;
		node_t* nextnode = nullptr;
		for ( node_t* node = stats[player]->inventory.first; node != NULL;
			node = nextnode )
		{
			toggleclick = false;
			nextnode = node->next;
			Item* tempItem = (Item*)(node->element);
			if ( tempItem == selectedItem )
			{
				continue;
			}

			if ( tempItem->x == selectedItem->x
				&& tempItem->y == selectedItem->y )
			{
				if ( itemCategory(selectedItem) != SPELL_CAT
					&& itemCategory(tempItem) == SPELL_CAT )
				{
					//It's alright, the item can go here. The item sharing this x is just a spell, but the item being moved isn't a spell.
				}
				else if ( itemCategory(selectedItem) == SPELL_CAT
					&& itemCategory(tempItem) != SPELL_CAT )
				{
					//It's alright, the item can go here. The item sharing this x isn't a spell, but the item being moved is a spell.
				}
				else
				{
					//The player just dropped an item onto another item.
					if ( !itemCompare(selectedItem, tempItem, false) )
					{
						// items are the same, check stacking behavior...
						int selectedItemQty = 0;
						int destItemQty = 0;
						auto stackingResult = getItemStackingBehavior(player, selectedItem, tempItem, selectedItemQty, destItemQty);
						if ( stackingResult.resultType == ITEM_ADDED_ENTIRELY_TO_DESTINATION_STACK )
						{
							// items have stacked entirely
							if ( Item* itemFromChest = takeItemFromChest(player, selectedItem, selectedItem->count, tempItem, false) )
							{
							}
							else
							{
								messagePlayer(0, MESSAGE_DEBUG, "ERROR CHEST");
							}
							selectedItem = nullptr;
							inputs.getUIInteraction(player)->selectedItemFromChest = 0;
							toggleclick = false;
						}
						else if ( stackingResult.resultType == ITEM_ADDED_PARTIALLY_TO_DESTINATION_STACK )
						{
							int qtyToTake = selectedItem->count - selectedItemQty;
							if ( Item* itemFromChest = takeItemFromChest(player, selectedItem, qtyToTake, tempItem, false) )
							{
								swappedItem = itemFromChest; // just to set this to not check for empty slots outside of the inventory loop
								// if any items left over, place the chest item back in original slot and toggleclick
								if ( selectedItem )
								{
									selectedItem->x = oldx;
									selectedItem->y = oldy;
									toggleclick = true;
									inputs.getUIInteraction(player)->selectedItemFromChest = selectedItem->uid;
								}
								else
								{
									selectedItem = nullptr;
									inputs.getUIInteraction(player)->selectedItemFromChest = 0;
									toggleclick = false;
								}
							}
							else
							{
								messagePlayer(0, MESSAGE_DEBUG, "ERROR CHEST");
								// failed to move item or error
								if ( selectedItem )
								{
									selectedItem->x = oldx;
									selectedItem->y = oldy;
								}
								selectedItem = nullptr;
								inputs.getUIInteraction(player)->selectedItemFromChest = 0;
								toggleclick = false;
							}
						}
						else
						{
							// failed to move item or error
							if ( selectedItem )
							{
								selectedItem->x = oldx;
								selectedItem->y = oldy;
							}
							selectedItem = nullptr;
							inputs.getUIInteraction(player)->selectedItemFromChest = 0;
							toggleclick = false;
						}
					}
					else
					{
						// dissimilar items
						if ( Item* itemFromChest = takeItemFromChest(player, selectedItem, selectedItem->count, nullptr, true) )
						{
							swappedItem = itemFromChest;
							if ( tempItem != swappedItem )
							{
								// different item stacks
								Item* chestItem = openedChest[player]->addItemToChestFromInventory(
									player, tempItem, tempItem->count, true, nullptr);

								if ( chestItem )
								{
									chestItem->x = oldx;
									chestItem->y = oldy;

									// toggleclick when swapping items
									selectedItem = chestItem;
									toggleclick = true;
									inputs.getUIInteraction(player)->selectedItemFromChest = selectedItem->uid;
								}
								else
								{
									selectedItem = nullptr;
									inputs.getUIInteraction(player)->selectedItemFromChest = 0;
									toggleclick = false;
								}

								if ( swappedItem )
								{
									swappedItem->x = slotFrameX;
									swappedItem->y = slotFrameY;
								}
							}
							else
							{
								if ( selectedItem )
								{
									selectedItem->x = oldx;
									selectedItem->y = oldy;
								}

								// items have stacked
								selectedItem = nullptr;
								inputs.getUIInteraction(player)->selectedItemFromChest = 0;
								toggleclick = false;
							}
						}
						else
						{
							// failed to take item
							if ( selectedItem )
							{
								selectedItem->x = oldx;
								selectedItem->y = oldy;
							}
							selectedItem = nullptr;
							inputs.getUIInteraction(player)->selectedItemFromChest = 0;
							toggleclick = false;
						}
					}
					break;
				}
			}
		}

		if ( swappedItem && selectedItem )
		{
			players[player]->inventoryUI.selectedItemAnimate.animateX = 0.0;
			players[player]->inventoryUI.selectedItemAnimate.animateY = 0.0;

			if ( bUseSelectedSlotCycleAnimation )
			{
				if ( auto oldDraggingItemImg = frame->findFrame("dragging inventory item old")->findImage("item sprite img") )
				{
					oldDraggingItemImg->path = getItemSpritePath(player, *swappedItem);
				}
			}
		}
		else if ( !swappedItem && selectedItem )
		{
			// placing an item into empty slot
			int selectedItemQty = 0;
			int destItemQty = 0;
			getItemEmptySlotStackingBehavior(player, *selectedItem, selectedItemQty, destItemQty);
			if ( destItemQty > 0 )
			{
				if ( Item* itemFromChest = takeItemFromChest(player, selectedItem, destItemQty, nullptr, true) )
				{
					itemFromChest->x = slotFrameX;
					itemFromChest->y = slotFrameY;
					toggleclick = false;

					// if any items left over, place the chest item back in original slot and toggleclick
					if ( selectedItem )
					{
						selectedItem->x = oldx;
						selectedItem->y = oldy;
						toggleclick = true;
						inputs.getUIInteraction(player)->selectedItemFromChest = selectedItem->uid;
					}
				}
				else
				{
					// failed to move item or error
					messagePlayer(0, MESSAGE_DEBUG, "ERROR CHEST");
					if ( selectedItem )
					{
						selectedItem->x = oldx;
						selectedItem->y = oldy;
					}
					toggleclick = false;
				}
			}
			else
			{
				// failed to move item or error
				messagePlayer(0, MESSAGE_DEBUG, "ERROR CHEST");
				if ( selectedItem )
				{
					selectedItem->x = oldx;
					selectedItem->y = oldy;
				}
				toggleclick = false;
			}
		}

		if ( !toggleclick )
		{
			selectedItem = nullptr;
			inputs.getUIInteraction(player)->selectedItemFromChest = 0;
		}

		playSound(139, 64); // click sound
	}
	else if ( mouseOverSlot && mouseInChest )
	{
		list_t* chest_inventory = nullptr;
		if ( multiplayer == CLIENT )
		{
			chest_inventory = &chestInv[player];
		}
		else if ( openedChest[player]->children.first && openedChest[player]->children.first->element )
		{
			chest_inventory = (list_t*)openedChest[player]->children.first->element;
		}

		if ( chest_inventory )
		{
			// within inventory
			int oldx = selectedItem->x;
			int oldy = selectedItem->y;
			selectedItem->x = slotFrameX;
			selectedItem->y = slotFrameY;

			Item* swappedItem = nullptr;
			node_t* nextnode = nullptr;
			for ( node_t* node = chest_inventory->first; node != NULL;
				node = nextnode )
			{
				toggleclick = false;
				nextnode = node->next;
				Item* tempItem = (Item*)(node->element);
				if ( tempItem == selectedItem )
				{
					continue;
				}

				if ( tempItem->x == selectedItem->x
					&& tempItem->y == selectedItem->y )
				{
					//The player just dropped an item onto another item.

					// drag/drop to stack inventory items
					bool stackedItems = dragDropStackChestItems(player, selectedItem, tempItem, oldx, oldy);
					if ( !stackedItems && selectedItem )
					{
						// if not stacked, then swap as normal
						swappedItem = selectedItem;
						tempItem->x = oldx;
						tempItem->y = oldy;
						selectedItem = tempItem;
						inputs.getUIInteraction(player)->selectedItemFromChest = selectedItem->uid;
						toggleclick = true;
					}
					else if ( stackedItems && selectedItem )
					{
						// stacked partially, swappedItem keeps the animation
						swappedItem = selectedItem;
						inputs.getUIInteraction(player)->selectedItemFromChest = selectedItem->uid;
						toggleclick = true;
					}
					break;
				}
			}

			if ( swappedItem && selectedItem )
			{
				players[player]->inventoryUI.selectedItemAnimate.animateX = 0.0;
				players[player]->inventoryUI.selectedItemAnimate.animateY = 0.0;

				if ( bUseSelectedSlotCycleAnimation )
				{
					if ( auto oldDraggingItemImg = frame->findFrame("dragging inventory item old")->findImage("item sprite img") )
					{
						oldDraggingItemImg->path = getItemSpritePath(player, *swappedItem);
					}
				}
			}

			if ( !toggleclick )
			{
				selectedItem = nullptr;
				inputs.getUIInteraction(player)->selectedItemFromChest = 0;
			}

			playSound(139, 64); // click sound
		}
		else
		{
			// failed to move item
			selectedItem = nullptr;
			inputs.getUIInteraction(player)->selectedItemFromChest = 0;
			toggleclick = false;
		}
	}
	else if ( frame && frame->findFrame("paperdoll slots") && players[player]->inventory_mode == INVENTORY_MODE_ITEM
		&& frame->findFrame("paperdoll slots")->capturesMouseInRealtimeCoords() && itemCategory(selectedItem) != SPELL_CAT )
	{
		// get slot for item type
		// if slot type is invalid, cancel.
		auto slotName = items[selectedItem->type].item_slot;
		Player::PaperDoll_t::PaperDollSlotType dollSlot = getPaperDollSlotFromItemType(*selectedItem);

		if ( slotName == ItemEquippableSlot::NO_EQUIP || dollSlot == Player::PaperDoll_t::PaperDollSlotType::SLOT_MAX )
		{
			// item can't be equipped, no action.
			selectedItem = nullptr;
			inputs.getUIInteraction(player)->selectedItemFromChest = 0;
			toggleclick = false;
		}
		else
		{
			// get coordinates of doll slot
			players[player]->paperDoll.getCoordinatesFromSlotType(dollSlot, slotFrameX, slotFrameY);

			Item* swappedItem = nullptr;
			node_t* nextnode = nullptr;
			for ( node_t* node = stats[player]->inventory.first; node != NULL; node = nextnode )
			{
				nextnode = node->next;
				Item* tempItem = (Item*)(node->element);
				if ( !tempItem ) { continue; }

				if ( players[player]->paperDoll.getSlotForItem(*tempItem) == dollSlot )
				{
					// not allowed to swap items on paper doll - no action.
					selectedItem = nullptr;
					inputs.getUIInteraction(player)->selectedItemFromChest = 0;
					toggleclick = false;
					break;
				}
			}

			if ( selectedItem )
			{
				// free slot - let us try equip. check first if we're not shapeshifted
				bool disableItemUsage = false;
				if ( players[player] && players[player]->entity )
				{
					if ( players[player]->entity->effectShapeshift != NOTHING )
					{
						// shape shifted, disable some items
						if ( !selectedItem->usableWhileShapeshifted(stats[player]) )
						{
							disableItemUsage = true;
						}
						if ( selectedItem->type == FOOD_CREAMPIE )
						{
							disableItemUsage = true;
						}
					}
				}

				if ( selectedItem->status == BROKEN )
				{
					messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(1092), selectedItem->getName()); // don't try equip broken stuff
					playSoundPlayer(player, 90, 64);
					toggleclick = false;
				}
				else if ( !disableItemUsage )
				{
					int oldx = selectedItem->x;
					int oldy = selectedItem->y;

					// placing an item into empty slot
					int selectedItemQty = 0;
					int destItemQty = 0;
					getItemEmptySlotStackingBehavior(player, *selectedItem, selectedItemQty, destItemQty);
					if ( destItemQty > 0 )
					{
						if ( Item* itemFromChest = takeItemFromChest(player, selectedItem, destItemQty, nullptr, true) )
						{
							itemFromChest->x = slotFrameX;
							itemFromChest->y = slotFrameY;
							toggleclick = false;

							// if any items left over, place the chest item back in original slot and toggleclick
							if ( selectedItem )
							{
								selectedItem->x = oldx;
								selectedItem->y = oldy;
								toggleclick = true;
							}
							bool equipped = executeItemMenuOption0ForInventoryItem(player, itemFromChest);
						}
						else
						{
							// failed to move item or error
							messagePlayer(0, MESSAGE_DEBUG, "ERROR CHEST");
							if ( selectedItem )
							{
								selectedItem->x = oldx;
								selectedItem->y = oldy;
							}
							toggleclick = false;
						}
					}
					else
					{
						// failed to move item or error
						messagePlayer(0, MESSAGE_DEBUG, "ERROR CHEST");
						if ( selectedItem )
						{
							selectedItem->x = oldx;
							selectedItem->y = oldy;
						}
						toggleclick = false;
					}
				}
				else
				{
					messagePlayer(player, 
						MESSAGE_INVENTORY | MESSAGE_HINT | MESSAGE_EQUIPMENT, 
						Language::get(3432)); // unable to use in current form message.
					playSoundPlayer(player, 90, 64);
					toggleclick = false;
				}
			}

			if ( swappedItem && selectedItem )
			{
				players[player]->inventoryUI.selectedItemAnimate.animateX = 0.0;
				players[player]->inventoryUI.selectedItemAnimate.animateY = 0.0;

				if ( bUseSelectedSlotCycleAnimation )
				{
					if ( auto oldDraggingItemImg = frame->findFrame("dragging inventory item old")->findImage("item sprite img") )
					{
						oldDraggingItemImg->path = getItemSpritePath(player, *swappedItem);
					}
				}
			}

			if ( !toggleclick )
			{
				selectedItem = nullptr;
				inputs.getUIInteraction(player)->selectedItemFromChest = 0;
			}

			playSound(139, 64); // click sound
		}
	}
	else if ( itemCategory(selectedItem) == SPELL_CAT || mouseInInventory || mouseInChest )
	{
		//Outside inventory. Spells can't be dropped.
		//If mouseInInventory, we dropped onto a slot frame area and the item should return to where it was
		//Catches cases like 1px between slots in backpack.

		selectedItem = nullptr;
		inputs.getUIInteraction(player)->selectedItemFromChest = 0;
		toggleclick = false;
	}
	else
	{
		// outside inventory
		int slotNum = 0;
		hotbar_slot_t* slot = getCurrentHotbarUnderMouse(player, &slotNum);
		if ( slot )
		{
			//Add item to hotbar.
			int oldItemQty = 0;
			int destItemQty = 0;
			ItemStackResult result;
			if ( !itemCompare(selectedItem, uidToItem(slot->item), false) )
			{
				result = getItemStackingBehavior(player, selectedItem, uidToItem(slot->item), oldItemQty, destItemQty);
			}
			else
			{
				result = getItemStackingBehavior(player, selectedItem, nullptr, oldItemQty, destItemQty);
			}
			int amountToPlace = selectedItem->count - oldItemQty;
			Item* newInventoryItem = nullptr;
			if ( amountToPlace > 0 )
			{
				switch ( result.resultType )
				{
					case ITEM_ADDED_PARTIALLY_TO_DESTINATION_STACK:
					case ITEM_ADDED_ENTIRELY_TO_DESTINATION_STACK:
					{
						newInventoryItem = takeItemFromChest(player, selectedItem, amountToPlace, result.itemToStackInto, false);
						break;
					}
					case ITEM_ADDED_WITHOUT_NEEDING_STACK:
					{
						// check for inventory space
						if ( !players[player]->inventoryUI.bItemInventoryHasFreeSlot() )
						{
							// no space
							messagePlayer(player, MESSAGE_INVENTORY, Language::get(727), selectedItem->getName()); // no room
							playSoundPlayer(player, 90, 64);
							break;
						}
						newInventoryItem = takeItemFromChest(player, selectedItem, amountToPlace, nullptr, true);
						break;
					}
					default:
						// error out
						break;
				}
			}

			if ( newInventoryItem )
			{
				slot->item = newInventoryItem->uid;
				// empty out duplicate slots that match this item uid.
				int i = 0;
				for ( auto& s : players[player]->hotbar.slots() )
				{
					if ( i != slotNum && s.item == slot->item )
					{
						s.item = 0;
						s.resetLastItem();
					}
					++i;
				}
				playSound(139, 64); // click sound
			}
			selectedItem = nullptr;
			inputs.getUIInteraction(player)->selectedItemFromChest = 0;
			toggleclick = false;
		}
		else
		{
			selectedItem = nullptr;
			inputs.getUIInteraction(player)->selectedItemFromChest = 0;
			toggleclick = false;
		}
		return;
	}
}

void releaseItem(const int player)
{
	Item*& selectedItem = inputs.getUIInteraction(player)->selectedItem;
	int& selectedItemFromHotbar = inputs.getUIInteraction(player)->selectedItemFromHotbar;
	Uint32& selectedItemFromChest = inputs.getUIInteraction(player)->selectedItemFromChest;

	if ( !selectedItem )
	{
		return;
	}

	node_t* node = nullptr;
	node_t* nextnode = nullptr;

	Frame* frame = players[player]->inventoryUI.frame;

	auto& hotbar = players[player]->hotbar.slots();

	if ( Input::inputs[player].binaryToggle("MenuCancel") )
	{
		if ( selectedItemFromChest > 0 )
		{
			if ( !players[player]->inventoryUI.chestGUI.bOpen )
			{
				if ( !players[player]->inventoryUI.warpMouseToSelectedItem(selectedItem, (Inputs::SET_CONTROLLER)) )
				{
					//messagePlayer(0, "[Debug]: warpMouseToSelectedItem failed");
				}
				players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
				selectedItemFromChest = 0;
			}
			else
			{
				players[player]->inventoryUI.selectChestSlot(selectedItem->x, selectedItem->y);
				players[player]->inventoryUI.chestGUI.scrollToSlot(players[player]->inventoryUI.getSelectedChestX(),
					players[player]->inventoryUI.getSelectedChestY(), false);
				if ( !players[player]->inventoryUI.warpMouseToSelectedChestSlot(selectedItem, (Inputs::SET_CONTROLLER)) )
				{
					//messagePlayer(0, "[Debug]: warpMouseToSelectedSpell failed");
				}
				players[player]->GUI.activateModule(Player::GUI_t::MODULE_CHEST);
				selectedItemFromHotbar = -1;
				selectedItemFromChest = 0;
			}
		}
		else if (selectedItemFromHotbar >= 0 && selectedItemFromHotbar < NUM_HOTBAR_SLOTS)
		{
			//Warp cursor back into hotbar, for gamepad convenience.
			if ( players[player]->hotbar.warpMouseToHotbar(selectedItemFromHotbar, (Inputs::SET_CONTROLLER)) )
			{
				players[player]->hotbar.selectHotbarSlot(selectedItemFromHotbar);
			}
			hotbar[selectedItemFromHotbar].item = selectedItem->uid;
			selectedItemFromHotbar = -1;
			players[player]->GUI.activateModule(Player::GUI_t::MODULE_HOTBAR);
			players[player]->hotbar.updateHotbar(); // simulate the slots rearranging before we try to move the mouse to it.
			warpMouseToSelectedHotbarSlot(player);
		}
		else
		{
			if ( itemCategory(selectedItem) == SPELL_CAT )
			{
				players[player]->inventoryUI.selectSpell(selectedItem->x, selectedItem->y);
				players[player]->inventoryUI.spellPanel.scrollToSlot(players[player]->inventoryUI.getSelectedSpellX(),
					players[player]->inventoryUI.getSelectedSpellY(), false);
				if ( !players[player]->inventoryUI.warpMouseToSelectedSpell(selectedItem, (Inputs::SET_CONTROLLER)) )
				{
					//messagePlayer(0, "[Debug]: warpMouseToSelectedSpell failed");
				}
				players[player]->GUI.activateModule(Player::GUI_t::MODULE_SPELLS);
			}
			else
			{
				if ( !players[player]->inventoryUI.warpMouseToSelectedItem(selectedItem, (Inputs::SET_CONTROLLER)) )
				{
					//messagePlayer(0, "[Debug]: warpMouseToSelectedInventorySlot failed");
				}
				players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
			}
		}
		selectedItemFromHotbar = -1;
		selectedItemFromChest = 0;
		selectedItem = nullptr;
		Input::inputs[player].consumeBinaryToggle("MenuCancel");
		Player::soundCancel();
		return;
	}

	if ( selectedItem && itemCategory(selectedItem) == SPELL_CAT && selectedItem->appearance >= 1000 )
	{
		if ( canUseShapeshiftSpellInCurrentForm(player, *selectedItem) == 0 )
		{
			selectedItem = nullptr;
			inputs.getUIInteraction(player)->selectedItemFromChest = 0;
			return;
		}
	}

	const Sint32 mousex = inputs.getMouse(player, Inputs::X);
	const Sint32 mousey = inputs.getMouse(player, Inputs::Y);

	bool& toggleclick = inputs.getUIInteraction(player)->toggleclick;

	// releasing items
	if ( (!Input::inputs[player].binary("MenuLeftClick") && inputs.bPlayerUsingKeyboardControl(player) && !toggleclick)
		|| (Input::inputs[player].binaryToggle("MenuLeftClick") && inputs.bPlayerUsingKeyboardControl(player) && toggleclick)
		|| ( (Input::inputs[player].binaryToggle(getContextMenuOptionBindingName(player, PROMPT_GRAB).c_str())
			|| Input::inputs[player].binaryToggle("MenuConfirm")) 
			&& toggleclick) )
	{
		Input::inputs[player].consumeBinaryToggle("MenuConfirm");
		Input::inputs[player].consumeBinaryToggle(getContextMenuOptionBindingName(player, PROMPT_GRAB).c_str());
		if ( players[player]->inventoryUI.isItemFromChest(selectedItem) )
		{
			releaseChestItem(player);
			Input::inputs[player].consumeBinaryToggle("MenuLeftClick");
			return;
		}
		else if ( GenericGUI[player].alchemyGUI.bOpen )
		{
			if ( releaseInventoryItemIntoAlchemy(player) )
			{
				selectedItem = nullptr;
				inputs.getUIInteraction(player)->selectedItemFromChest = 0;
				toggleclick = false;
				Input::inputs[player].consumeBinaryToggle("MenuLeftClick");
				return;
			}
		}

		const int inventorySlotSize = players[player]->inventoryUI.getSlotSize();

		if ( selectedItem )
		{
			bool bPaperDollItem = (players[player]->paperDoll.enabled && players[player]->paperDoll.isItemOnDoll(*selectedItem));

			const int UNKNOWN_SLOT = -10;
			int slotFrameX = UNKNOWN_SLOT;
			int slotFrameY = UNKNOWN_SLOT;

			//TODO UI: CLEANUP COMMENTS
			bool mouseOverSlot = getSlotFrameXYFromMousePos(player, slotFrameX, slotFrameY, itemCategory(selectedItem) == SPELL_CAT);
			bool mouseInInventory = mouseInsidePlayerInventory(player);
			bool mouseInChest = false;
			bool mouseInAlchemyBasePotion = false;
			bool mouseInAlchemySecondaryPotion = false;
			bool allowDropItems = players[player]->inventoryUI.guiAllowDropItems(selectedItem);
			if ( !mouseInInventory )
			{
				if ( players[player]->inventoryUI.chestFrame
					&& !players[player]->inventoryUI.chestFrame->isDisabled()
					&& openedChest[player] )
				{
					if ( auto chestSlots = players[player]->inventoryUI.chestFrame->findFrame("chest slots") )
					{
						if ( !chestSlots->isDisabled() && chestSlots->capturesMouse() )
						{
							mouseInChest = true;
						}
					}
					if ( players[player]->inventoryUI.chestFrame->capturesMouse() )
					{
						allowDropItems = false;
					}
				}
				else if ( GenericGUI[player].alchemyGUI.bOpen )
				{
					if ( GenericGUI[player].alchemyGUI.alchFrame
						&& !GenericGUI[player].alchemyGUI.alchFrame->isDisabled() )
					{
						if ( auto baseFrame = GenericGUI[player].alchemyGUI.alchFrame->findFrame("alchemy base") )
						{
							if ( !baseFrame->isDisabled() && baseFrame->capturesMouse() )
							{
								allowDropItems = false;
							}
						}
						if ( auto recipesFrame = GenericGUI[player].alchemyGUI.alchFrame->findFrame("player recipes") )
						{
							if ( !recipesFrame->isDisabled() && recipesFrame->capturesMouse() )
							{
								allowDropItems = false;
							}
						}
					}
				}
			}

			if ( mouseOverSlot && mouseInInventory && slotFrameY > Player::Inventory_t::DOLL_ROW_5 )
			{
				if ( bPaperDollItem )
				{
					if ( !players[player]->inventoryUI.bItemInventoryHasFreeSlot() )
					{
						// can't drag off into inventory, no slots available
						messagePlayer(player, MESSAGE_INVENTORY, Language::get(3997), selectedItem->getName());
						playSoundPlayer(player, 90, 64);
						selectedItem = nullptr;
						inputs.getUIInteraction(player)->selectedItemFromChest = 0;
						toggleclick = false;
						Input::inputs[player].consumeBinaryToggle("MenuLeftClick");
						return;
					}
				}

				// within inventory
				int oldx = selectedItem->x;
				int oldy = selectedItem->y;
				selectedItem->x = slotFrameX; //(mousex - x) / inventorySlotSize;
				selectedItem->y = slotFrameY; //(mousey - y) / inventorySlotSize;

				Item* swappedItem = nullptr;

				for (node = stats[player]->inventory.first; node != NULL;
				        node = nextnode)
				{
					toggleclick = false;
					nextnode = node->next;
					Item* tempItem = (Item*) (node->element);
					if (tempItem == selectedItem)
					{
						continue;
					}

					if (tempItem->x == selectedItem->x
					        && tempItem->y == selectedItem->y)
					{
						if (itemCategory(selectedItem) != SPELL_CAT
						        && itemCategory(tempItem) == SPELL_CAT)
						{
							//It's alright, the item can go here. The item sharing this x is just a spell, but the item being moved isn't a spell.
						}
						else if (itemCategory(selectedItem) == SPELL_CAT
						         && itemCategory(tempItem) != SPELL_CAT)
						{
							//It's alright, the item can go here. The item sharing this x isn't a spell, but the item being moved is a spell.
						}
						else
						{
							//The player just dropped an item onto another item.
							if ( bPaperDollItem )
							{
								int newx = selectedItem->x;
								int newy = selectedItem->y;

								// disabled stacking items from paper doll for now
								//int selectedItemQty = 0;
								//int destItemQty = 0;
								//auto stackingResult = getItemStackingBehavior(player, selectedItem, tempItem, selectedItemQty, destItemQty);
								//if ( stackingResult == ITEM_ADDED_PARTIALLY_TO_DESTINATION_STACK )
								//{
								//	// no need to unequip, just send qty update to server
								//	if ( !selectedItem->unableToEquipDueToSwapWeaponTimer(player) )
								//	{
								//		int oldQty = selectedItem->count;
								//		bool stackedItems = dragDropStackInventoryItems(player, selectedItem, tempItem, oldx, oldy);
								//		if ( selectedItem && oldQty != selectedItem->count )
								//		{
								//			if ( multiplayer == CLIENT && player >= 0 && players[player]->isLocalPlayer() && itemIsEquipped(selectedItem, player) )
								//			{
								//				// if incrementing qty and holding item, then send "equip" for server to update their count of your held item.
								//				Item** slot = itemSlot(stats[player], selectedItem);
								//				if ( slot )
								//				{
								//					if ( slot == &stats[player]->weapon )
								//					{
								//						clientSendEquipUpdateToServer(EQUIP_ITEM_SLOT_WEAPON, EQUIP_ITEM_SUCCESS_UPDATE_QTY, player,
								//							selectedItem->type, selectedItem->status, selectedItem->beatitude, 
								//							selectedItem->count, selectedItem->appearance, selectedItem->identified);
								//					}
								//					else if ( slot == &stats[player]->shield )
								//					{
								//						clientSendEquipUpdateToServer(EQUIP_ITEM_SLOT_SHIELD, EQUIP_ITEM_SUCCESS_UPDATE_QTY, player,
								//							selectedItem->type, selectedItem->status, selectedItem->beatitude, 
								//							selectedItem->count, selectedItem->appearance, selectedItem->identified);
								//					}
								//				}
								//			}
								//		}
								//	}
								//	selectedItem = nullptr;
								//	inputs.getUIInteraction(player)->selectedItemFromChest = 0;
								//	toggleclick = false;
								//	if ( inputs.bMouseLeft(player) )
								//	{
								//		inputs.mouseClearLeft(player);
								//	}
								//	return;
								//}
								//else if ( stackingResult == ITEM_ADDED_ENTIRELY_TO_DESTINATION_STACK )
								//{
								//	// unequip, then add to stack
								//	bool unequipped = executeItemMenuOption0ForPaperDoll(player, selectedItem, true);
								//	if ( !unequipped )
								//	{
								//		// failure to unequip
								//		selectedItem->x = oldx;
								//		selectedItem->y = oldy;

								//		selectedItem = nullptr;
								//		inputs.getUIInteraction(player)->selectedItemFromChest = 0;
								//		toggleclick = false;
								//		if ( inputs.bMouseLeft(player) )
								//		{
								//			inputs.mouseClearLeft(player);
								//		}
								//		return;
								//	}
								//	else
								//	{
								//		bool stackedItems = dragDropStackInventoryItems(player, selectedItem, tempItem, oldx, oldy);
								//		if ( stackedItems )
								//		{
								//			selectedItem = nullptr;
								//			inputs.getUIInteraction(player)->selectedItemFromChest = 0;
								//			toggleclick = false;
								//			if ( inputs.bMouseLeft(player) )
								//			{
								//				inputs.mouseClearLeft(player);
								//			}
								//			return;
								//		}
								//		else if ( selectedItem )
								//		{
								//			selectedItem->x = newx;
								//			selectedItem->y = newy;

								//			if ( selectedItem
								//				&& oldx == Player::PaperDoll_t::ITEM_PAPERDOLL_COORDINATE
								//				&& oldy == Player::PaperDoll_t::ITEM_PAPERDOLL_COORDINATE )
								//			{
								//				// item was on paperdoll, now is unequipped. oldx/y needs to be returning to inventory
								//				oldx = Player::PaperDoll_t::ITEM_RETURN_TO_INVENTORY_COORDINATE;
								//				oldy = Player::PaperDoll_t::ITEM_RETURN_TO_INVENTORY_COORDINATE;
								//			}

								//			swappedItem = selectedItem;
								//			tempItem->x = oldx;
								//			tempItem->y = oldy;
								//			selectedItem = tempItem;
								//			toggleclick = true;
								//		}
								//	}
								//}
								//else
								{
									bool unequipped = executeItemMenuOption0ForPaperDoll(player, selectedItem, false);
									if ( !unequipped )
									{
										// failure to unequip
										selectedItem->x = oldx;
										selectedItem->y = oldy;

										selectedItem = nullptr;
										inputs.getUIInteraction(player)->selectedItemFromChest = 0;
										toggleclick = false;
										Input::inputs[player].consumeBinaryToggle("MenuLeftClick");
										return;
									}
									else
									{
										selectedItem->x = newx;
										selectedItem->y = newy;

										if ( selectedItem
											&& oldx == Player::PaperDoll_t::ITEM_PAPERDOLL_COORDINATE
											&& oldy == Player::PaperDoll_t::ITEM_PAPERDOLL_COORDINATE )
										{
											// item was on paperdoll, now is unequipped. oldx/y needs to be returning to inventory
											oldx = Player::PaperDoll_t::ITEM_RETURN_TO_INVENTORY_COORDINATE;
											oldy = Player::PaperDoll_t::ITEM_RETURN_TO_INVENTORY_COORDINATE;
										}
									}

									swappedItem = selectedItem;
									tempItem->x = oldx;
									tempItem->y = oldy;
									selectedItem = tempItem;
									toggleclick = true;
								}
							}
							else
							{
								// drag/drop to stack inventory items
								bool stackedItems = dragDropStackInventoryItems(player, selectedItem, tempItem, oldx, oldy);
								if ( !stackedItems && selectedItem )
								{
									// if not stacked, then swap as normal
									swappedItem = selectedItem;
									tempItem->x = oldx;
									tempItem->y = oldy;
									selectedItem = tempItem;
									toggleclick = true;
								}
								else if ( stackedItems && selectedItem )
								{
									// stacked partially, swappedItem keeps the animation
									swappedItem = selectedItem;
									toggleclick = true;
								}
							}
							break;
						}
					}
				}

				if ( bPaperDollItem )
				{
					if ( !swappedItem )
					{
						int newx = selectedItem->x;
						int newy = selectedItem->y;

						bool unequipped = executeItemMenuOption0ForPaperDoll(player, selectedItem, false);
						if ( !unequipped )
						{
							// failure to unequip, reset coords
							selectedItem->x = oldx;
							selectedItem->y = oldy;
						}
						else
						{
							selectedItem->x = newx;
							selectedItem->y = newy;
						}
					}
					else if ( swappedItem 
						&& selectedItem 
						&& selectedItem->x == Player::PaperDoll_t::ITEM_RETURN_TO_INVENTORY_COORDINATE )
					{
						// item was on paperdoll, swapped, now is unequipped. 
						// find a new home for the new selectedItem
						autosortInventory(player, true);
					}
				}

				if ( swappedItem && selectedItem )
				{
					players[player]->inventoryUI.selectedItemAnimate.animateX = 0.0;
					players[player]->inventoryUI.selectedItemAnimate.animateY = 0.0;

					if ( bUseSelectedSlotCycleAnimation )
					{
						if ( auto oldDraggingItemImg = frame->findFrame("dragging inventory item old")->findImage("item sprite img") )
						{
							oldDraggingItemImg->path = getItemSpritePath(player, *swappedItem);
						}
					}
				}

				if ( !toggleclick )
				{
					selectedItem = nullptr;
					inputs.getUIInteraction(player)->selectedItemFromChest = 0;
				}

				playSound(139, 64); // click sound
			}
			else if ( mouseOverSlot && mouseInChest && !(itemCategory(selectedItem) == SPELL_CAT) )
			{
				list_t* chest_inventory = nullptr;
				if ( multiplayer == CLIENT )
				{
					chest_inventory = &chestInv[player];
				}
				else if ( openedChest[player]->children.first && openedChest[player]->children.first->element )
				{
					chest_inventory = (list_t*)openedChest[player]->children.first->element;
				}

				if ( chest_inventory )
				{
					// within inventory
					int oldx = selectedItem->x;
					int oldy = selectedItem->y;
					selectedItem->x = slotFrameX;
					selectedItem->y = slotFrameY;

					Item* swappedItem = nullptr;
					node_t* nextnode = nullptr;
					for ( node_t* node = chest_inventory->first; node != NULL;
						node = nextnode )
					{
						toggleclick = false;
						nextnode = node->next;
						Item* tempItem = (Item*)(node->element);
						if ( tempItem == selectedItem )
						{
							continue;
						}

						if ( tempItem->x == selectedItem->x
							&& tempItem->y == selectedItem->y )
						{
							int newx = selectedItem->x;
							int newy = selectedItem->y;

							//The player just dropped an item onto another item.
							if ( bPaperDollItem )
							{
								// cannot drag paper doll item onto another item in chest
								if ( selectedItem )
								{
									selectedItem->x = oldx;
									selectedItem->y = oldy;
								}
								selectedItem = nullptr;
								inputs.getUIInteraction(player)->selectedItemFromChest = 0;
								toggleclick = false;
							}
							else if ( !itemCompare(selectedItem, tempItem, false) )
							{
								// items are the same, check stacking behavior...
								int selectedItemQty = 0;
								int destItemQty = 0;
								auto stackingResult = getItemStackingBehavior(player, selectedItem, tempItem, selectedItemQty, destItemQty);
								if ( stackingResult.resultType == ITEM_ADDED_ENTIRELY_TO_DESTINATION_STACK )
								{
									if ( Item* chestItem = openedChest[player]->addItemToChestFromInventory(
										player, selectedItem, selectedItem->count, false, tempItem) )
									{
										// success adding to chest
										chestItem->x = newx;
										chestItem->y = newy;
									}
									selectedItem = nullptr;
									inputs.getUIInteraction(player)->selectedItemFromChest = 0;
									toggleclick = false;
								}
								else if ( stackingResult.resultType == ITEM_ADDED_PARTIALLY_TO_DESTINATION_STACK )
								{
									int qtyToStash = selectedItem->count - selectedItemQty;
									if ( Item* chestItem = openedChest[player]->addItemToChestFromInventory(
										player, selectedItem, qtyToStash, false, tempItem) )
									{
										// success adding to chest
										chestItem->x = newx;
										chestItem->y = newy;
										if ( selectedItem )
										{
											swappedItem = chestItem;
											selectedItem->x = oldx;
											selectedItem->y = oldy;
											toggleclick = true;
										}
										else
										{
											selectedItem = nullptr;
											inputs.getUIInteraction(player)->selectedItemFromChest = 0;
											toggleclick = false;
										}
									}
									else
									{
										messagePlayer(0, MESSAGE_DEBUG, "ERROR CHEST");
										// failed to move item or error
										if ( selectedItem )
										{
											selectedItem->x = oldx;
											selectedItem->y = oldy;
										}
										selectedItem = nullptr;
										inputs.getUIInteraction(player)->selectedItemFromChest = 0;
										toggleclick = false;
									}
								}
								else
								{
									// failed to move item or error
									selectedItem->x = oldx;
									selectedItem->y = oldy;
									selectedItem = nullptr;
									inputs.getUIInteraction(player)->selectedItemFromChest = 0;
									toggleclick = false;
								}
							}
							else
							{
								// dissimilar items
								if ( Item* chestItem = openedChest[player]->addItemToChestFromInventory(
									player, selectedItem, selectedItem->count, true, nullptr) )
								{
									// success adding to chest
									chestItem->x = newx;
									chestItem->y = newy;

									if ( Item* itemFromChest = takeItemFromChest(player, tempItem, tempItem->count, nullptr, true) )
									{
										itemFromChest->x = oldx;
										itemFromChest->y = oldy;

										// success swapping items
										swappedItem = chestItem;
										selectedItem = itemFromChest;
										inputs.getUIInteraction(player)->selectedItemFromChest = 0;
										toggleclick = true;
									}
									else
									{
										// failed to take item from chest
										selectedItem = nullptr;
										inputs.getUIInteraction(player)->selectedItemFromChest = 0;
										toggleclick = false;
									}
								}
								else
								{
									// failure adding to chest
									if ( selectedItem )
									{
										selectedItem->x = oldx;
										selectedItem->y = oldy;
									}
									selectedItem = nullptr;
									inputs.getUIInteraction(player)->selectedItemFromChest = 0;
									toggleclick = false;
								}
							}
							Input::inputs[player].consumeBinaryToggle("MenuLeftClick");
							if ( swappedItem && selectedItem )
							{
								players[player]->inventoryUI.selectedItemAnimate.animateX = 0.0;
								players[player]->inventoryUI.selectedItemAnimate.animateY = 0.0;

								if ( bUseSelectedSlotCycleAnimation )
								{
									if ( auto oldDraggingItemImg = frame->findFrame("dragging inventory item old")->findImage("item sprite img") )
									{
										oldDraggingItemImg->path = getItemSpritePath(player, *swappedItem);
									}
								}
							}
							return;
						}
					}

					if ( swappedItem && selectedItem )
					{
						players[player]->inventoryUI.selectedItemAnimate.animateX = 0.0;
						players[player]->inventoryUI.selectedItemAnimate.animateY = 0.0;

						if ( bUseSelectedSlotCycleAnimation )
						{
							if ( auto oldDraggingItemImg = frame->findFrame("dragging inventory item old")->findImage("item sprite img") )
							{
								oldDraggingItemImg->path = getItemSpritePath(player, *swappedItem);
							}
						}
					}
					else if ( !swappedItem && selectedItem )
					{
						// placing an item into empty slot
						if ( bPaperDollItem )
						{
							bool unequipped = executeItemMenuOption0ForPaperDoll(player, selectedItem, true); 
							// treat as if we're dropping the item, don't check inventory space for item
							if ( !unequipped )
							{
								// failure to unequip, reset coords
								if ( selectedItem )
								{
									selectedItem->x = oldx;
									selectedItem->y = oldy;
								}
								toggleclick = false;
							}
							else
							{
								if ( Item* chestItem = openedChest[player]->addItemToChestFromInventory(
									player, selectedItem, selectedItem->count, true, nullptr) )
								{
									// success adding to chest
									chestItem->x = slotFrameX;
									chestItem->y = slotFrameY;
									toggleclick = false;
								}
								else
								{
									// failure adding to chest
									if ( selectedItem )
									{
										selectedItem->x = oldx;
										selectedItem->y = oldy;
									}
									toggleclick = false;
								}
							}
						}
						else
						{
							int selectedItemQty = 0;
							int destItemQty = 0;
							getItemEmptySlotStackingBehavior(player, *selectedItem, selectedItemQty, destItemQty);
							if ( destItemQty > 0 )
							{
								if ( Item* chestItem = openedChest[player]->addItemToChestFromInventory(
									player, selectedItem, destItemQty, true, nullptr) )
								{
									// success adding to chest
									chestItem->x = slotFrameX;
									chestItem->y = slotFrameY;
									toggleclick = false;

									// if any items left over, place the chest item back in original slot and toggleclick
									if ( selectedItem )
									{
										selectedItem->x = oldx;
										selectedItem->y = oldy;
										toggleclick = true;
									}
								}
								else
								{
									// failure adding to chest
									if ( selectedItem )
									{
										selectedItem->x = oldx;
										selectedItem->y = oldy;
									}
									toggleclick = false;
								}
							}
							else
							{
								// failure adding to chest
								if ( selectedItem )
								{
									selectedItem->x = oldx;
									selectedItem->y = oldy;
								}
								toggleclick = false;
							}
						}
					}

					if ( !toggleclick )
					{
						selectedItem = nullptr;
						inputs.getUIInteraction(player)->selectedItemFromChest = 0;
					}

					playSound(139, 64); // click sound
				}
				else
				{
					// failed to move item
					selectedItem = nullptr;
					inputs.getUIInteraction(player)->selectedItemFromChest = 0;
					toggleclick = false;
				}
			}
			else if ( frame && frame->findFrame("paperdoll slots") && players[player]->inventory_mode == INVENTORY_MODE_ITEM
				&& frame->findFrame("paperdoll slots")->capturesMouseInRealtimeCoords() )
			{
				// get slot for item type
				// if slot type is invalid, cancel.
				auto slotName = items[selectedItem->type].item_slot;
				Player::PaperDoll_t::PaperDollSlotType dollSlot = getPaperDollSlotFromItemType(*selectedItem);

				if ( bPaperDollItem || slotName == ItemEquippableSlot::NO_EQUIP || dollSlot == Player::PaperDoll_t::PaperDollSlotType::SLOT_MAX )
				{
					// moving paper doll item onto own area - or item can't be equipped, no action.
					selectedItem = nullptr;
					inputs.getUIInteraction(player)->selectedItemFromChest = 0;
					toggleclick = false;
				}
				else
				{
					// get coordinates of doll slot
					players[player]->paperDoll.getCoordinatesFromSlotType(dollSlot, slotFrameX, slotFrameY);

					Item* swappedItem = nullptr;
					toggleclick = false;

					for ( node = stats[player]->inventory.first; node != NULL;
						node = nextnode )
					{
						nextnode = node->next;
						Item* tempItem = (Item*)(node->element); 
						if ( !tempItem ) { continue; }
						if ( tempItem == selectedItem )	{ continue;	}

						if ( players[player]->paperDoll.getSlotForItem(*tempItem) != dollSlot )
						{
							continue;
						}
						swappedItem = tempItem;
						break;
					}

					if ( swappedItem )
					{
						int oldx = selectedItem->x;
						int oldy = selectedItem->y;

						int newx = slotFrameX;
						int newy = slotFrameY;

						// try to equip the selected item
						bool equipped = executeItemMenuOption0ForInventoryItem(player, selectedItem);
						if ( !equipped )
						{
							// failure to equip
							if ( selectedItem )
							{
								selectedItem->x = oldx;
								selectedItem->y = oldy;
								selectedItem = nullptr;
								inputs.getUIInteraction(player)->selectedItemFromChest = 0;
								toggleclick = false;
								Input::inputs[player].consumeBinaryToggle("MenuLeftClick");
								return;
							}
						}

						// success - assign selectedItem to the newly picked up item from the paper doll
						swappedItem->x = oldx;
						swappedItem->y = oldy;
						selectedItem = swappedItem;
						toggleclick = true;
					}
					else
					{
						// free slot - let us try equip.
						bool equipped = executeItemMenuOption0ForInventoryItem(player, selectedItem);
						toggleclick = false;
					}

					if ( swappedItem && selectedItem )
					{
						players[player]->inventoryUI.selectedItemAnimate.animateX = 0.0;
						players[player]->inventoryUI.selectedItemAnimate.animateY = 0.0;

						// unused for now
						if ( bUseSelectedSlotCycleAnimation )
						{
							if ( auto oldDraggingItemImg = frame->findFrame("dragging inventory item old")->findImage("item sprite img") )
							{
								oldDraggingItemImg->path = getItemSpritePath(player, *swappedItem);
							}
						}
					}

					if ( !toggleclick )
					{
						selectedItem = nullptr;
						inputs.getUIInteraction(player)->selectedItemFromChest = 0;
					}

					playSound(139, 64); // click sound
				}
			}
			else if ( itemCategory(selectedItem) == SPELL_CAT || mouseInInventory || !allowDropItems )
			{
				//Outside inventory. Spells can't be dropped.
				//If mouseInInventory, we dropped onto a slot frame area and the item should return to where it was
				//Catches cases like 1px between slots in backpack.
				int slotNum = 0;
				hotbar_slot_t* slot = getCurrentHotbarUnderMouse(player, &slotNum);
				if ( slot && players[player]->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_HOTBAR) )
				{
					//Add spell to hotbar.
					Item* tempItem = uidToItem(slot->item);
					if ( tempItem && tempItem != selectedItem )
					{
						slot->item = selectedItem->uid;
						selectedItem = tempItem;
						toggleclick = true;
					}
					else
					{
						slot->item = selectedItem->uid;
						selectedItem = nullptr;
						inputs.getUIInteraction(player)->selectedItemFromChest = 0;
						toggleclick = false;
					}

					// empty out duplicate slots that match this item uid.
					int i = 0;
					for ( auto& s : players[player]->hotbar.slots() )
					{
						if ( i != slotNum && s.item == slot->item )
						{
							s.item = 0;
							s.resetLastItem();
						}
						++i;
					}
					playSound(139, 64); // click sound
				}
				else
				{
					selectedItem = nullptr;
					inputs.getUIInteraction(player)->selectedItemFromChest = 0;
				}
			}
			else
			{
				// outside inventory
				int slotNum = 0;
				hotbar_slot_t* slot = getCurrentHotbarUnderMouse(player, &slotNum);
				if ( slot )
				{
					if ( !players[player]->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_HOTBAR) )
					{
						selectedItem = nullptr;
						inputs.getUIInteraction(player)->selectedItemFromChest = 0;
					}
					else
					{
						//Add item to hotbar.
						Item* tempItem = uidToItem(slot->item);
						Item* swappedItem = nullptr;
						if ( tempItem && tempItem != selectedItem )
						{
							slot->item = selectedItem->uid;
							swappedItem = selectedItem;
							selectedItem = tempItem;
							toggleclick = true;
						}
						else
						{
							slot->item = selectedItem->uid;
							selectedItem = nullptr;
							inputs.getUIInteraction(player)->selectedItemFromChest = 0;
							toggleclick = false;
						}

						if ( swappedItem && selectedItem )
						{
							players[player]->inventoryUI.selectedItemAnimate.animateX = 0.0;
							players[player]->inventoryUI.selectedItemAnimate.animateY = 0.0;

							// unused for now
							if ( bUseSelectedSlotCycleAnimation )
							{
								if ( auto oldDraggingItemImg = frame->findFrame("dragging inventory item old")->findImage("item sprite img") )
								{
									oldDraggingItemImg->path = getItemSpritePath(player, *swappedItem);
								}
							}
						}

						// empty out duplicate slots that match this item uid.
						int i = 0;
						for ( auto& s : players[player]->hotbar.slots() )
						{
							if ( i != slotNum && s.item == slot->item )
							{
								s.item = 0;
								s.resetLastItem();
							}
							++i;
						}
						playSound(139, 64); // click sound
					}
				}
				else
				{
					if ( bPaperDollItem )
					{
						bool unequipped = executeItemMenuOption0ForPaperDoll(player, selectedItem, true); // unequip item
						if ( !unequipped )
						{
							selectedItem = nullptr;
							inputs.getUIInteraction(player)->selectedItemFromChest = 0;
							toggleclick = false;
						}
						else
						{
							bool droppedAll = false;
							while ( selectedItem && selectedItem->count > 1 )
							{
								droppedAll = dropItem(selectedItem, player);
								if ( droppedAll )
								{
									selectedItem = nullptr;
									inputs.getUIInteraction(player)->selectedItemFromChest = 0;
								}
							}
							if ( !droppedAll )
							{
								dropItem(selectedItem, player);
							}
							selectedItem = nullptr;
							inputs.getUIInteraction(player)->selectedItemFromChest = 0;
							toggleclick = false;
						}
					}
					else
					{
						if ( selectedItem->count > 1 ) // drop item
						{
							if ( dropItem(selectedItem, player) )
							{
								selectedItem = nullptr;
								inputs.getUIInteraction(player)->selectedItemFromChest = 0;
							}
							toggleclick = true;
						}
						else
						{
							dropItem(selectedItem, player);
							selectedItem = nullptr;
							inputs.getUIInteraction(player)->selectedItemFromChest = 0;
							toggleclick = false;
						}
					}
				}
			}
		}
		if ( Input::inputs[player].binaryToggle("MenuLeftClick") )
		{
			Input::inputs[player].consumeBinaryToggle("MenuLeftClick");
		}
	}
}

void Player::Inventory_t::cycleInventoryTab()
{
	if ( player.inventory_mode == INVENTORY_MODE_ITEM 
		|| player.hud.compactLayoutMode != Player::HUD_t::COMPACT_LAYOUT_INVENTORY )
	{
		player.hud.compactLayoutMode = Player::HUD_t::COMPACT_LAYOUT_INVENTORY;
		player.inventory_mode = INVENTORY_MODE_SPELL;
		if ( player.GUI.activeModule == Player::GUI_t::MODULE_INVENTORY
			|| player.GUI.activeModule == Player::GUI_t::MODULE_CHEST
			|| player.GUI.activeModule == Player::GUI_t::MODULE_SHOP )
		{
			player.GUI.activateModule(Player::GUI_t::MODULE_SPELLS);
			if ( auto selectedItem = inputs.getUIInteraction(player.playernum)->selectedItem )
			{
				player.inventoryUI.selectSpell(selectedItem->x, selectedItem->y);
			}
			player.inventoryUI.spellPanel.scrollToSlot(player.inventoryUI.getSelectedSpellX(),
				player.inventoryUI.getSelectedSpellY(), true);
		}
	}
	else
	{
		player.hud.compactLayoutMode = Player::HUD_t::COMPACT_LAYOUT_INVENTORY;
		player.inventory_mode = INVENTORY_MODE_ITEM;
		if ( player.GUI.activeModule == Player::GUI_t::MODULE_SPELLS )
		{
			player.GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
			if ( !inputs.getVirtualMouse(player.playernum)->draw_cursor )
			{
				player.GUI.warpControllerToModule(false);
			}
		}

		if ( player.shopGUI.bOpen )
		{
			// once off - since we usually set the gui_mode to inventory on opening spell panel
			// this makes sure we go back into the shop gui mode after browsing our spells and returning to inventory.
			player.gui_mode = GUI_MODE_SHOP; 
		}
	}
}

/*
 * Because the mouseInBounds() function looks at the omousex for whatever reason.
 * And changing that function to use mousex has, through much empirical study, been proven to be a bad idea. Things will break.
 * So, this function is used instead for one thing and only one thing: the gold borders that follow the mouse through the inventory.
 *
 * Places used so far:
 * * Here.
 * * drawstatus.cpp: drawing the gold borders around the hotbar slot the mouse is hovering over
 */
bool mouseInBoundsRealtimeCoords(int player, int x1, int x2, int y1, int y2)
{
	if ( inputs.getMouse(player, Inputs::Y) >= y1 && inputs.getMouse(player, Inputs::Y) < y2 )
	{
		if ( inputs.getMouse(player, Inputs::X) >= x1 && inputs.getMouse(player, Inputs::X) < x2)
		{
			return true;
		}
	}

	return false;
}

//void drawBlueInventoryBorder(const int player, const Item& item, int x, int y)
//{
//	SDL_Rect pos;
//	pos.x = x + item.x * players[player]->inventoryUI.getSlotSize() + 2;
//	pos.y = y + item.y * players[player]->inventoryUI.getSlotSize() + 1;
//	pos.w = players[player]->inventoryUI.getSlotSize();
//	pos.h = players[player]->inventoryUI.getSlotSize();
//
//	Uint32 color = makeColor( 0, 0, 255, 127);
//	drawBox(&pos, color, 127);
//}

std::string getBindingNameForMissingTooltipPrompts(int index)
{
	if ( index == 1 )
	{
#ifdef NINTENDO
		return "MenuAlt1";
#else
		return "MenuAlt2";
#endif
	}
	else if ( index == 2 )
	{
#ifdef NINTENDO
		return "MenuAlt2";
#else
		return "MenuAlt1";
#endif
	}
	else if ( index == 3 )
	{
#ifdef NINTENDO
		return "MenuConfirm";
#else
		return "MenuCancel";
#endif
	}
	else if ( index == 4 )
	{
#ifdef NINTENDO
		return "MenuCancel";
#else
		return "MenuConfirm";
#endif
	}
	else
	{
		return "";
	}
}

int getContextMenuOptionOrder(const int player, ItemContextMenuPrompts prompt)
{
	std::string bindingName = getContextMenuOptionBindingName(player, prompt);
	if ( bindingName == "MenuAlt1" )
	{
		return 2;
//#ifdef NINTENDO
//		return 1;
//#else
//#endif
	}
	else if ( bindingName == "MenuAlt2" )
	{
		return 1;
//#ifdef NINTENDO
//		return 2;
//#else
//#endif
	}
	else if ( bindingName == "MenuConfirm" )
	{
		return 4;
//#ifdef NINTENDO
//		return 3;
//#else
//#endif
	}
	else if ( bindingName == "MenuCancel" )
	{
		return 3;
//#ifdef NINTENDO
//		return 4;
//#else
//#endif
	}
	return 5;
}

void Player::HUD_t::updateFrameTooltip(Item* item, const int x, const int y, int justify, Frame* parentFrame)
{
    const int player = this->player.playernum;
    if ( !item )
    {
        return;
    }

	Frame* tooltipContainerFrame = nullptr;
	Frame* frameMain = nullptr;
	Frame* frameInventory = nullptr;
	Frame* frameInteract = nullptr;
	Frame* frameTooltipPrompt = nullptr;
	Frame* titleOnlyFrame = nullptr;
	if ( parentFrame != nullptr )
	{
		// hacks for compendium tooltips
		char name[32];
		snprintf(name, sizeof(name), "player tooltip container %d", 0);
		if ( tooltipContainerFrame = parentFrame->findFrame(name) )
		{
			snprintf(name, sizeof(name), "player title only tooltip %d", 0);
			titleOnlyFrame = tooltipContainerFrame->findFrame(name);
			snprintf(name, sizeof(name), "player tooltip %d", 0);
			frameMain = tooltipContainerFrame->findFrame(name);
			snprintf(name, sizeof(name), "player interact %d", 0);
			frameInteract = parentFrame->findFrame(name);
			snprintf(name, sizeof(name), "player item prompt %d", 0);
			frameTooltipPrompt = tooltipContainerFrame->findFrame(name);
		}
	}
	else
	{
		tooltipContainerFrame = this->player.inventoryUI.tooltipContainerFrame;
		frameMain = this->player.inventoryUI.tooltipFrame;
		frameInventory = this->player.inventoryUI.frame;
		frameInteract = this->player.inventoryUI.interactFrame;
		frameTooltipPrompt = this->player.inventoryUI.tooltipPromptFrame;
		titleOnlyFrame = this->player.inventoryUI.titleOnlyTooltipFrame;
		if ( !frameInventory )
		{
			return;
		}
	}

	bool compendiumTooltip = false;
	if ( parentFrame && !strcmp(parentFrame->getName(), "compendium") )
	{
		compendiumTooltip = true;
	}

    if ( tooltipContainerFrame )
    {
		if ( compendiumTooltip )
		{
			tooltipContainerFrame->setSize(SDL_Rect{ 0, 0, parentFrame->getSize().w, parentFrame->getSize().h });
		}
		else
		{
			tooltipContainerFrame->setSize(
			    SDL_Rect{ this->player.camera_virtualx1(),
					this->player.camera_virtualy1(),
					this->player.camera_virtualWidth(),
					this->player.camera_virtualHeight()});
		}
    }
    

	players[player]->inventoryUI.miscTooltipOpacitySetpoint = 0;
	players[player]->inventoryUI.miscTooltipOpacityAnimate = 0.0;
    

    auto& tooltipDisplayedSettings = compendiumTooltip ? this->player.inventoryUI.compendiumItemTooltipDisplay 
		: this->player.inventoryUI.itemTooltipDisplay;
    
    bool bUpdateDisplayedTooltip =
    (!tooltipDisplayedSettings.isItemSameAsCurrent(player, item) || ItemTooltips.itemDebug);
    
    if ( !frameMain )
    {
        return;
    }
    
    if ( !frameInteract )
    {
        return;
    }
    
    if ( !frameTooltipPrompt )
    {
        return;
    }
    
    static const char* bigfont = "fonts/pixelmix.ttf#18";
    
    auto frameAttr = frameMain->findFrame("inventory mouse tooltip attributes frame");
    auto frameDesc = frameMain->findFrame("inventory mouse tooltip description frame");
    auto framePrompt = frameMain->findFrame("inventory mouse tooltip prompt frame");
    auto frameValues = frameMain->findFrame("inventory mouse tooltip value frame");
    
    auto imgTopBackground = frameMain->findImage("tooltip top background");
    auto imgTopBackgroundLeft = frameMain->findImage("tooltip top left");
    auto imgTopBackgroundRight = frameMain->findImage("tooltip top right");
    
    auto imgMiddleBackground = frameMain->findImage("tooltip middle background");
    auto imgMiddleBackgroundLeft = frameMain->findImage("tooltip middle left");
    auto imgMiddleBackgroundRight = frameMain->findImage("tooltip middle right");
    
    auto imgBottomBackground = frameMain->findImage("tooltip bottom background");
    auto imgBottomBackgroundLeft = frameMain->findImage("tooltip bottom left");
    auto imgBottomBackgroundRight = frameMain->findImage("tooltip bottom right");
    
    auto txtPrompt = framePrompt->findField("inventory mouse tooltip prompt");
    char buf[1024] = "";
    
    const int padx = 4;
    const int pady = 4;
    const int imgTopBackgroundDefaultHeight = 28;
    const int imgTopBackground2XHeight = 42;
    
    const bool doTitleOnlyTooltip = players[player]->shootmode && !compendiumTooltip && !(enableDebugKeys && keystatus[SDLK_g]);
    bool doShortTooltip = false;
    if ( players[player]->shootmode && !compendiumTooltip )
    {
        doShortTooltip = true;
        if ( doTitleOnlyTooltip )
        {
            if ( !tooltipDisplayedSettings.displayingTitleOnlyTooltip )
            {
                bUpdateDisplayedTooltip = true;
            }
            tooltipDisplayedSettings.displayingTitleOnlyTooltip = true;
            tooltipDisplayedSettings.displayingShortFormTooltip = false;
        }
        else
        {
            if ( !tooltipDisplayedSettings.displayingShortFormTooltip )
            {
                bUpdateDisplayedTooltip = true;
            }
            tooltipDisplayedSettings.displayingShortFormTooltip = true;
            tooltipDisplayedSettings.displayingTitleOnlyTooltip = false;
        }
    }
    else
    {
        doShortTooltip = false;
        if ( tooltipDisplayedSettings.displayingShortFormTooltip || tooltipDisplayedSettings.displayingTitleOnlyTooltip )
        {
            bUpdateDisplayedTooltip = true;
        }
        tooltipDisplayedSettings.displayingShortFormTooltip = false;
        tooltipDisplayedSettings.displayingTitleOnlyTooltip = false;
    }
    
    bool isItemFromInventory = false;
    if ( item->node && item->node->list == &stats[player]->inventory )
    {
        isItemFromInventory = true;
    }
    
    if ( ItemTooltips.itemDebug && (svFlags & SV_FLAG_CHEATS) )
    {
        if ( keystatus[SDLK_KP_PLUS] )
        {
            keystatus[SDLK_KP_PLUS] = 0;
            item->beatitude += 1;
            /*for ( int i = 0; i < 100; ++i )
             {
             Uint32 r = local_rng.rand() % 4096;
             printlog("%d %d %d", (r >> 8 & 0xF), (r >> 4 & 0xF), (r & 0xF));
             }*/
        }
        if ( keystatus[SDLK_KP_MINUS] )
        {
            keystatus[SDLK_KP_MINUS] = 0;
            item->beatitude -= 1;
        }
        if ( keystatus[SDLK_KP_6] )
        {
            keystatus[SDLK_KP_6] = 0;
            item->status = std::min(EXCELLENT, static_cast<Status>(item->status + 1));
        }
        if ( keystatus[SDLK_KP_4] )
        {
            keystatus[SDLK_KP_4] = 0;
            item->status = std::max(BROKEN, static_cast<Status>(item->status - 1));
        }
        if ( keystatus[SDLK_KP_5] )
        {
            keystatus[SDLK_KP_5] = 0;
            if ( item->type == WOODEN_SHIELD )
            {
                item->type = static_cast<ItemType>(NUMITEMS - 1);
            }
            else
            {
                item->type = static_cast<ItemType>(item->type - 1);
                if ( item->type == SPELL_ITEM )
                {
                    item->type = static_cast<ItemType>(item->type - 1);
                }
            }
        }
        if ( keystatus[SDLK_KP_8] )
        {
            keystatus[SDLK_KP_8] = 0;
            if ( item->type == NUMITEMS - 1 )
            {
                item->type = WOODEN_SHIELD;
            }
            else
            {
                item->type = static_cast<ItemType>(item->type + 1);
                if ( item->type == SPELL_ITEM )
                {
                    item->type = static_cast<ItemType>(item->type + 1);
                }
            }
        }
    }
    
    bool expandBindingPressed = false;
    if ( !players[player]->usingCommand()
        && players[player]->bControlEnabled
        && (!gamePaused || compendiumTooltip)
        && Input::inputs[player].consumeBinaryToggle("Expand Inventory Tooltip") )
    {
        expandBindingPressed = true;
		if ( compendiumTooltip )
		{
			tooltipDisplayedSettings.expanded = !tooltipDisplayedSettings.expanded;
		}
        else if ( !players[player]->shootmode && item->identified && !doTitleOnlyTooltip
            && !doShortTooltip )
        {
            tooltipDisplayedSettings.expanded = !tooltipDisplayedSettings.expanded;
        }
    }
    
    if ( !doTitleOnlyTooltip )
    {
        titleOnlyFrame->setDisabled(true);
    }
    
    auto& appraisal = players[player]->inventoryUI.appraisal;
    
    if ( bUpdateDisplayedTooltip )
    {
        tooltipDisplayedSettings.updateItem(player, item);
        
        std::string tooltipType = ItemTooltips.tmpItems[item->type].tooltip;
        
        if ( ItemTooltips.tooltips.find(tooltipType) == ItemTooltips.tooltips.end() )
        {
            tooltipType = "tooltip_default";
        }
        else if ( !item->identified )
        {
            tooltipType = "tooltip_unidentified";
        }
        auto itemTooltip = ItemTooltips.tooltips[tooltipType];
        
        int textx = 0;
        int texty = 0;
        
        /*if ( item->type == FOOD_TIN )
         {
         std::string cookingMethod;
         std::string protein;
         std::string sides;
         item->foodTinGetDescription(cookingMethod, protein, sides);
         snprintf(buf, sizeof(buf), "%s %s %s%s%s (%+d)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(),
         item->getName(),
         cookingMethod.c_str(), protein.c_str(), sides.c_str(),
         item->beatitude);
         }
         else*/
        
        bool manuallyInsertedNewline = false;
        
        if ( item->type == SPELL_ITEM )
        {
            spell_t* spell = getSpellFromItem(player, item, false);
            if ( !spell )
            {
                snprintf(buf, sizeof(buf), "%s", "Unknown Spell");
            }
            else if ( item && item->appearance >= 1000 )
            {
                // shapeshift spells, append the form name here.
                switch ( spell->ID )
                {
                case SPELL_SPEED:
                case SPELL_DETECT_FOOD:
                    snprintf(buf, sizeof(buf), "%s%s\n(%s)",
                             ItemTooltips.adjectives["spell_prefixes"]["spell_of"].c_str(), spell->getSpellName(),
                             ItemTooltips.spellItems[SPELL_RAT_FORM].name.c_str());
                    manuallyInsertedNewline = true;
                    break;
                case SPELL_POISON:
                case SPELL_SPRAY_WEB:
                    snprintf(buf, sizeof(buf), "%s%s\n(%s)",
                             ItemTooltips.adjectives["spell_prefixes"]["spell_of"].c_str(), spell->getSpellName(),
                             ItemTooltips.spellItems[SPELL_SPIDER_FORM].name.c_str());
                    manuallyInsertedNewline = true;
                    break;
                case SPELL_STRIKE:
                case SPELL_FEAR:
                case SPELL_TROLLS_BLOOD:
                    snprintf(buf, sizeof(buf), "%s%s\n(%s)",
                             ItemTooltips.adjectives["spell_prefixes"]["spell_of"].c_str(), spell->getSpellName(),
                             ItemTooltips.spellItems[SPELL_TROLL_FORM].name.c_str());
                    manuallyInsertedNewline = true;
                    break;
                case SPELL_LIGHTNING:
                case SPELL_CONFUSE:
                case SPELL_AMPLIFY_MAGIC:
                    snprintf(buf, sizeof(buf), "%s%s\n(%s)",
                             ItemTooltips.adjectives["spell_prefixes"]["spell_of"].c_str(), spell->getSpellName(),
                             ItemTooltips.spellItems[SPELL_IMP_FORM].name.c_str());
                    manuallyInsertedNewline = true;
                    break;
                default:
                    snprintf(buf, sizeof(buf), "%s%s",
                             ItemTooltips.adjectives["spell_prefixes"]["spell_of"].c_str(), spell->getSpellName());
                    break;
                }
            }
            else
            {
                snprintf(buf, sizeof(buf), "%s%s",
                         ItemTooltips.adjectives["spell_prefixes"]["spell_of"].c_str(), spell->getSpellName());
            }
        }
        else
        {
            if ( !item->identified )
            {
                if ( itemCategory(item) == BOOK )
                {
                    snprintf(buf, sizeof(buf), "%s %s\n%s (?)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(),
                             Language::get(4214), getBookLocalizedNameFromIndex(item->appearance % numbooks).c_str());
                    manuallyInsertedNewline = true;
                }
                else if ( itemCategory(item) == SCROLL )
                {
                    snprintf(buf, sizeof(buf), "%s %s\n%s %s (?)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(),
                             items[item->type].getUnidentifiedName(), Language::get(4215), item->getScrollLabel());
                    manuallyInsertedNewline = true;
                }
                else
                {
                    snprintf(buf, sizeof(buf), "%s %s (?)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), item->getName());
                }
            }
            else
            {
                if ( item->type == TOOL_SENTRYBOT || item->type == TOOL_SPELLBOT || item->type == TOOL_DUMMYBOT
                    || item->type == TOOL_GYROBOT )
                {
                    int health = 100;
                    if ( !item->tinkeringBotIsMaxHealth() )
                    {
                        health = 25 * (item->appearance % 10);
                        if ( health == 0 && item->status != BROKEN )
                        {
                            health = 5;
                        }
                    }
                    snprintf(buf, sizeof(buf), "%s %s (%d%%)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), item->getName(), health);
                }
                else if ( item->type == ENCHANTED_FEATHER )
                {
                    snprintf(buf, sizeof(buf), "%s %s (%d%%) (%+d)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(),
                             item->getName(), item->appearance % ENCHANTED_FEATHER_MAX_DURABILITY, item->beatitude);
                }
                else if ( itemCategory(item) == BOOK )
                {
                    snprintf(buf, sizeof(buf), "%s %s\n%s (%+d)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(),
                             Language::get(4214), getBookLocalizedNameFromIndex(item->appearance % numbooks).c_str(), item->beatitude); // brand new copy of
                    manuallyInsertedNewline = true;
                }
                else
                {
                    snprintf(buf, sizeof(buf), "%s %s (%+d)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), item->getName(), item->beatitude);
                }
            }
        }
        
        if ( doTitleOnlyTooltip )
        {
            auto titleOnlyTxt = titleOnlyFrame->findField("title only header");
            std::string title = buf;
            if ( itemCategory(item) != BOOK && !(itemCategory(item) == SCROLL && !item->identified) )
            {
                std::replace(title.begin(), title.end(), '\n', ' ');
            }
            titleOnlyTxt->setText(title.c_str());
            bool bigTooltip = false;
            SDL_Rect txtPos = titleOnlyTxt->getSize();
            int padx = 10;
            txtPos.x = padx * 2;
            txtPos.y = pady + 1;
            txtPos.h = 24;
            if ( false /*&& players[player]->messageZone.useBigFont*/ )
            {
                titleOnlyTxt->setFont(players[player]->messageZone.bigfont);
                bigTooltip = true;
                txtPos.h = 44;
                txtPos.y = pady * 2 + 2;
            }
            else
            {
                titleOnlyTxt->setFont(players[player]->messageZone.smallfont);
            }
            if ( auto textGet = Text::get(titleOnlyTxt->getLongestLine().c_str(), titleOnlyTxt->getFont(),
                                          titleOnlyTxt->getTextColor(), titleOnlyTxt->getOutlineColor()) )
            {
                txtPos.w = textGet->getWidth();
                if ( titleOnlyTxt->getNumTextLines() > 1 )
                {
                    txtPos.h = 44;
                    bigTooltip = true;
                }
            }
            titleOnlyTxt->setSize(txtPos);
            SDL_Rect tooltipPos{ x, y, txtPos.w + padx * 4, 0 };
            tooltipPos.x -= tooltipPos.w / 2;
            if ( tooltipPos.x % 2 == 1 )
            {
                ++tooltipPos.x;
                ++tooltipPos.w;
            }
            
            auto tm = titleOnlyFrame->findImage("tooltip top background");
            tm->disabled = true;
            auto tl = titleOnlyFrame->findImage("tooltip top left");
            tl->disabled = true;
            auto tr = titleOnlyFrame->findImage("tooltip top right");
            tr->disabled = true;
            if ( bigTooltip )
            {
                tm->path = "*#images/ui/Inventory/tooltips/Hover_T00_TitleOnly2x.png";
                tl->path = "*#images/ui/Inventory/tooltips/Hover_TL00_TitleOnly2x.png";
                tr->path = "*#images/ui/Inventory/tooltips/Hover_TR00_TitleOnly2x.png";
            }
            else
            {
                tm->path = "*#images/ui/Inventory/tooltips/Hover_T00_TitleOnly.png";
                tl->path = "*#images/ui/Inventory/tooltips/Hover_TL00_TitleOnly.png";
                tr->path = "*#images/ui/Inventory/tooltips/Hover_TR00_TitleOnly.png";
            }
            if ( auto imgGet = Image::get(tm->path.c_str()) )
            {
                tm->pos.w = imgGet->getWidth();
                tm->pos.h = imgGet->getHeight();
            }
            if ( auto imgGet = Image::get(tl->path.c_str()) )
            {
                tl->pos.w = imgGet->getWidth();
                tl->pos.h = imgGet->getHeight();
                tooltipPos.h = tl->pos.h;
            }
            if ( auto imgGet = Image::get(tr->path.c_str()) )
            {
                tr->pos.w = imgGet->getWidth();
                tr->pos.h = imgGet->getHeight();
            }
            static ConsoleVariable<Vector4> cvar_titleOnlyColor("/tooltip_title_only_color", Vector4{ 188, 154, 114, 255 });
            titleOnlyTxt->setTextColor(makeColor(
                                                 cvar_titleOnlyColor->x,
                                                 cvar_titleOnlyColor->y,
                                                 cvar_titleOnlyColor->z,
                                                 cvar_titleOnlyColor->w));
            tl->disabled = false;
            tl->pos.x = 0;
            tm->disabled = false;
            tm->pos.x = tl->pos.x + tl->pos.w;
            tm->pos.w = tooltipPos.w - tl->pos.w - tr->pos.w;
            tr->disabled = false;
            tr->pos.x = tm->pos.x + tm->pos.w;
            titleOnlyFrame->setSize(tooltipPos);
            finalizeFrameTooltip(item, x, y, justify);
            return;
        }
        auto txtHeader = frameMain->findField("inventory mouse tooltip header");
        txtHeader->setText(buf);
        Text* textGet = Text::get(txtHeader->getText(), txtHeader->getFont(),
                                  txtHeader->getTextColor(), txtHeader->getOutlineColor());
        if ( textGet )
        {
            textx = textGet->getWidth();
            texty = textGet->getNumTextLines() * Font::get(txtHeader->getFont())->height();
        }
        
        std::string minWidthKey = "default";
        std::string maxWidthKey = "default";
        std::string headerMaxWidthKey = "default";
        if ( spell_t* spell = getSpellFromItem(player, item, false) )
        {
            if ( itemTooltip.minWidths.find(ItemTooltips.spellItems[spell->ID].internalName) != itemTooltip.minWidths.end() )
            {
                minWidthKey = ItemTooltips.spellItems[spell->ID].internalName;
            }
            if ( itemTooltip.maxWidths.find(ItemTooltips.spellItems[spell->ID].internalName) != itemTooltip.maxWidths.end() )
            {
                maxWidthKey = ItemTooltips.spellItems[spell->ID].internalName;
            }
            if ( itemTooltip.headerMaxWidths.find(ItemTooltips.spellItems[spell->ID].internalName) != itemTooltip.headerMaxWidths.end() )
            {
                headerMaxWidthKey = ItemTooltips.spellItems[spell->ID].internalName;
            }
        }
        else
        {
            if ( itemTooltip.minWidths.find(ItemTooltips.tmpItems[item->type].internalName) != itemTooltip.minWidths.end() )
            {
                minWidthKey = ItemTooltips.tmpItems[item->type].internalName;
            }
            if ( itemTooltip.maxWidths.find(ItemTooltips.tmpItems[item->type].internalName) != itemTooltip.maxWidths.end() )
            {
                maxWidthKey = ItemTooltips.tmpItems[item->type].internalName;
            }
            if ( itemTooltip.headerMaxWidths.find(ItemTooltips.tmpItems[item->type].internalName) != itemTooltip.headerMaxWidths.end() )
            {
                headerMaxWidthKey = ItemTooltips.tmpItems[item->type].internalName;
            }
        }
        
        bool useDefaultHeaderHeight = true;
        if ( manuallyInsertedNewline ||
            (itemTooltip.headerMaxWidths[headerMaxWidthKey] > 0 && textx > itemTooltip.headerMaxWidths[headerMaxWidthKey]) )
        {
            if ( !manuallyInsertedNewline )
            {
                txtHeader->setSize(SDL_Rect{ 0, 0, itemTooltip.headerMaxWidths[headerMaxWidthKey], 0 });
                txtHeader->reflowTextToFit(0);
            }
            
            std::string input = txtHeader->getText();
            size_t offset = 0;
            size_t findChar = 0;
            std::vector<std::string> tokens;
            while ( (findChar = input.find('\n', offset)) != std::string::npos ) {
                tokens.push_back(input.substr(offset, findChar - offset));
                offset = findChar + 1;
            }
            tokens.push_back(input.substr(offset));
            
            int newWidth = 0;
            if ( tokens.size() == 1 )
            {
                // no reflow
            }
            else
            {
                unsigned int newWidth = 0;
                for ( auto& str : tokens )
                {
                    if ( Text* textGet = Text::get(str.c_str(), txtHeader->getFont(),
                                                   txtHeader->getTextColor(), txtHeader->getOutlineColor()) )
                    {
                        newWidth = std::max(newWidth, textGet->getWidth());
                    }
                }
                
                textx = newWidth;
                texty = txtHeader->getNumTextLines() * Font::get(txtHeader->getFont())->height();
                imgTopBackground->pos.h = imgTopBackground2XHeight;
                imgTopBackground->path = "images/ui/Inventory/tooltips/Hover_T00_2x.png";
                imgTopBackgroundLeft->pos.h = imgTopBackground->pos.h;
                imgTopBackgroundLeft->path = "images/ui/Inventory/tooltips/Hover_TL00_2x.png";
                imgTopBackgroundRight->pos.h = imgTopBackground->pos.h;
                imgTopBackgroundRight->path = "images/ui/Inventory/tooltips/Hover_TR00_2x.png";
                useDefaultHeaderHeight = false;
            }
        }
        
        if ( useDefaultHeaderHeight )
        {
            imgTopBackground->pos.h = imgTopBackgroundDefaultHeight;
            imgTopBackground->path = "images/ui/Inventory/tooltips/Hover_T00.png";
            imgTopBackgroundLeft->pos.h = imgTopBackground->pos.h;
            imgTopBackgroundLeft->path = "images/ui/Inventory/tooltips/Hover_TL00.png";
            imgTopBackgroundRight->pos.h = imgTopBackground->pos.h;
            imgTopBackgroundRight->path = "images/ui/Inventory/tooltips/Hover_TR00.png";
        }
        
		auto minWidth = itemTooltip.minWidths[minWidthKey];
		if ( compendiumTooltip )
		{
			minWidth = std::max(minWidth, 242);
		}

        if ( ItemTooltips.itemDebug && (svFlags & SV_FLAG_CHEATS) )
        {
            auto headerBg = frameMain->findImage("inventory mouse tooltip header bg");
            headerBg->pos = SDL_Rect{
                imgTopBackgroundLeft->pos.x + imgTopBackgroundLeft->pos.w + padx,
                0,
                textx,
                1 };
            headerBg->disabled = false;
            auto tooltipMin = frameMain->findImage("inventory mouse tooltip min");
            tooltipMin->pos = SDL_Rect{
                imgTopBackgroundLeft->pos.x + imgTopBackgroundLeft->pos.w + padx,
                2,
				minWidth,
                1 };
            tooltipMin->disabled = false;
            auto tooltipMax = frameMain->findImage("inventory mouse tooltip max");
            tooltipMax->pos = SDL_Rect{
                imgTopBackgroundLeft->pos.x + imgTopBackgroundLeft->pos.w + padx,
                4,
                itemTooltip.maxWidths[maxWidthKey],
                1 };
            tooltipMax->disabled = false;
            auto headerMax = frameMain->findImage("inventory mouse tooltip header max");
            headerMax->pos = SDL_Rect{
                imgTopBackgroundLeft->pos.x + imgTopBackgroundLeft->pos.w + padx,
                6,
                itemTooltip.headerMaxWidths[headerMaxWidthKey],
                1 };
            headerMax->disabled = false;
        }
        
        if ( minWidth > 0 )
        {
            textx = std::max(minWidth, textx);
        }
        if ( itemTooltip.maxWidths[maxWidthKey] > 0 )
        {
            textx = std::min(itemTooltip.maxWidths[maxWidthKey], textx);
        }
        
        if ( ItemTooltips.itemDebug && (svFlags & SV_FLAG_CHEATS) )
        {
            auto headerBg = frameMain->findImage("inventory mouse tooltip header bg new");
            headerBg->pos = SDL_Rect{
                imgTopBackgroundLeft->pos.x + imgTopBackgroundLeft->pos.w + padx,
                imgTopBackground->pos.h,
                textx,
                1 };
            headerBg->disabled = false;
        }
        else
        {
            auto headerBg = frameMain->findImage("inventory mouse tooltip header bg");
            headerBg->disabled = true;
            auto tooltipMin = frameMain->findImage("inventory mouse tooltip min");
            tooltipMin->disabled = true;
            auto tooltipMax = frameMain->findImage("inventory mouse tooltip max");
            tooltipMax->disabled = true;
            auto headerMax = frameMain->findImage("inventory mouse tooltip header max");
            headerMax->disabled = true;
        }
        
        txtHeader->setSize(SDL_Rect{ imgTopBackgroundLeft->pos.x + imgTopBackgroundLeft->pos.w + padx, 1, textx + 3 * padx, imgTopBackground->pos.h});
        txtHeader->setVJustify(Field::justify_t::CENTER);
        int totalHeight = txtHeader->getSize().h;
        
        // get total width of tooltip
        const int tooltipWidth = txtHeader->getSize().w + imgTopBackgroundLeft->pos.w + imgTopBackgroundRight->pos.w;
        tooltipDisplayedSettings.tooltipWidth = tooltipWidth;
        
        // attribute frame size - padded by (imgTopBackgroundLeft->pos.x + (imgTopBackgroundLeft->pos.w / 2) + padx) on either side
        SDL_Rect frameAttrPos{ imgTopBackgroundLeft->pos.x + (imgTopBackgroundLeft->pos.w / 2) + padx, totalHeight, 0, 0 };
        frameAttrPos.w = tooltipWidth - frameAttrPos.x * 2;
        
        auto imgSpellIcon = frameAttr->findImage("inventory mouse tooltip spell image");
        auto imgSpellIconBg = frameAttr->findImage("inventory mouse tooltip spell image bg");
        auto imgPrimaryIcon = frameAttr->findImage("inventory mouse tooltip primary image");
        auto imgSecondaryIcon = frameAttr->findImage("inventory mouse tooltip secondary image");
        auto imgThirdIcon = frameAttr->findImage("inventory mouse tooltip third image");
        auto imgGoldIcon = frameValues->findImage("inventory mouse tooltip gold image");
        auto imgWeightIcon = frameValues->findImage("inventory mouse tooltip weight image");
        auto imgValueBackground = frameValues->findImage("inventory mouse tooltip value background");
        
        imgPrimaryIcon->pos.y = 0;
        imgSecondaryIcon->pos.y = 0;
        imgThirdIcon->pos.y = 0;
        
        imgSpellIcon->disabled = true;
        imgSpellIcon->path = "";
        imgSpellIconBg->disabled = true;
        imgPrimaryIcon->disabled = true;
        imgPrimaryIcon->path = "";
        imgSecondaryIcon->disabled = true;
        imgSecondaryIcon->path = "";
        imgThirdIcon->disabled = true;
        imgThirdIcon->path = "";
        
        if ( tooltipType.find("tooltip_spell_") != std::string::npos )
        {
            imgSpellIcon->disabled = false;
            imgSpellIconBg->disabled = false;
            imgSpellIcon->path = ItemTooltips.getSpellIconPath(player, *item, -1);
            
            static ConsoleVariable<int> cvar_spelltooltipIconX("/spell_tooltip_icon_x", 0);
            static ConsoleVariable<int> cvar_spelltooltipIconY("/spell_tooltip_icon_y", -4);
            imgSpellIcon->pos.x = frameAttrPos.w - imgSpellIcon->pos.w - 2 * padx + *cvar_spelltooltipIconX;
            imgSpellIcon->pos.y = 3 * pady + *cvar_spelltooltipIconY;
            imgSpellIconBg->pos.x = imgSpellIcon->pos.x - 5;
            imgSpellIconBg->pos.y = imgSpellIcon->pos.y - 5;
        }
        
        auto txtPrimaryValue = frameAttr->findField("inventory mouse tooltip primary value");
        auto txtPrimaryValueHighlight = frameAttr->findField("inventory mouse tooltip primary value highlight");
        auto txtPrimaryValuePositive = frameAttr->findField("inventory mouse tooltip primary value positive text");
        auto txtPrimaryValueNegative = frameAttr->findField("inventory mouse tooltip primary value negative text");
        
        auto txtSecondaryValue = frameAttr->findField("inventory mouse tooltip secondary value");
        auto txtSecondaryValueHighlight = frameAttr->findField("inventory mouse tooltip secondary value highlight");
        auto txtSecondaryValuePositive = frameAttr->findField("inventory mouse tooltip secondary value positive text");
        auto txtSecondaryValueNegative = frameAttr->findField("inventory mouse tooltip secondary value negative text");
        
        auto txtThirdValue = frameAttr->findField("inventory mouse tooltip third value");
        auto txtThirdValueHighlight = frameAttr->findField("inventory mouse tooltip third value highlight");
        auto txtThirdValuePositive = frameAttr->findField("inventory mouse tooltip third value positive text");
        auto txtThirdValueNegative = frameAttr->findField("inventory mouse tooltip third value negative text");
        
        auto txtDescription = frameDesc->findField("inventory mouse tooltip description");
        auto txtDescriptionPositive = frameDesc->findField("inventory mouse tooltip description positive text");
        auto txtDescriptionNegative = frameDesc->findField("inventory mouse tooltip description negative text");
        
        {
            static ConsoleVariable<int> cvar_item_tooltip_attr_padding("/item_tooltip_attr_padding", -4);
            txtPrimaryValue->setPaddingPerLine(*cvar_item_tooltip_attr_padding);
            txtPrimaryValueHighlight->setPaddingPerLine(*cvar_item_tooltip_attr_padding);
            txtPrimaryValuePositive->setPaddingPerLine(*cvar_item_tooltip_attr_padding);
            txtPrimaryValueNegative->setPaddingPerLine(*cvar_item_tooltip_attr_padding);
            
            txtSecondaryValue->setPaddingPerLine(*cvar_item_tooltip_attr_padding);
            txtSecondaryValueHighlight->setPaddingPerLine(*cvar_item_tooltip_attr_padding);
            txtSecondaryValuePositive->setPaddingPerLine(*cvar_item_tooltip_attr_padding);
            txtSecondaryValueNegative->setPaddingPerLine(*cvar_item_tooltip_attr_padding);
            
            txtThirdValue->setPaddingPerLine(*cvar_item_tooltip_attr_padding);
            txtThirdValueHighlight->setPaddingPerLine(*cvar_item_tooltip_attr_padding);
            txtThirdValuePositive->setPaddingPerLine(*cvar_item_tooltip_attr_padding);
            txtThirdValueNegative->setPaddingPerLine(*cvar_item_tooltip_attr_padding);
        }

        if ( itemTooltip.icons.size() > 0 )
        {
            int index = 0;
            for ( auto& icon : itemTooltip.icons )
            {
                if ( icon.conditionalAttribute.compare("") != 0 )
                {
                    if ( itemCategory(item) == MAGICSTAFF )
                    {
                        if ( icon.conditionalAttribute.find("magicstaff_") != std::string::npos )
                        {
                            if ( ItemTooltips.tmpItems[item->type].internalName != icon.conditionalAttribute )
                            {
                                continue;
                            }
                            icon.iconPath = ItemTooltips.getSpellIconPath(player, *item, -1);
                        }
                    }
                    else if ( itemCategory(item) == SCROLL )
                    {
                        if ( icon.conditionalAttribute.find("scroll_") != std::string::npos )
                        {
                            if ( ItemTooltips.tmpItems[item->type].internalName != icon.conditionalAttribute )
                            {
                                continue;
                            }
                        }
                    }
                    else if ( item->type == TOOL_SENTRYBOT || item->type == TOOL_SPELLBOT
                             || item->type == TOOL_GYROBOT || item->type == TOOL_DUMMYBOT )
                    {
                        if ( icon.conditionalAttribute == "TINKERBOT_UPGRADEABLE" )
                        {
                            if ( item->status == EXCELLENT || item->status == BROKEN )
                            {
                                continue;
                            }
                        }
                        else if ( icon.conditionalAttribute == "TINKERBOT_BROKEN" )
                        {
                            if ( item->status != BROKEN )
                            {
                                continue;
                            }
                        }
                        else if ( item->status == BROKEN )
                        {
                            continue;
                        }
                        else if ( icon.conditionalAttribute == "TINKERBOT_MAGICATK" )
                        {
                            icon.iconPath = ItemTooltips.getSpellIconPath(player, *item, -1);
                        }
                    }
                    else if ( item->type == SPELL_ITEM )
                    {
                        if ( icon.conditionalAttribute == "SPELL_ICON_EFFECT" )
                        {
                            icon.iconPath = "images/ui/HUD/statusfx/magic_effect.png";
                        }
                        else if ( icon.conditionalAttribute.find("spell_") != std::string::npos )
                        {
                            spell_t* spell = getSpellFromItem(player, item, false);
                            if ( spell && ItemTooltips.spellItems[spell->ID].internalName == icon.conditionalAttribute )
                            {
                                // current spell uses this attribute
                            }
                            else
                            {
                                continue; // no spell, or spellname doesn't match attribute
                            }
                        }
                    }
					else if ( item->type == MASK_MARIGOLD
						&& (icon.conditionalAttribute == "EFF_MARIGOLD1"
							|| icon.conditionalAttribute == "EFF_MARIGOLD1_HUNGER") )
					{
						if ( icon.conditionalAttribute == "EFF_MARIGOLD1" )
						{
							if ( (svFlags & SV_FLAG_HUNGER) )
							{
								continue;
							}
						}
						else if ( icon.conditionalAttribute == "EFF_MARIGOLD1_HUNGER" )
						{
							if ( !(svFlags & SV_FLAG_HUNGER) )
							{
								continue;
							}
						}
					}
                    else if ( itemCategory(item) == SPELLBOOK
                             && icon.conditionalAttribute.find("SPELLBOOK_") != std::string::npos )
                    {
                        spell_t* spell = getSpellFromID(getSpellIDFromSpellbook(item->type));
                        int skillLVL = std::min(100, stats[player]->getModifiedProficiency(PRO_MAGIC) + statGetINT(stats[player], players[player]->entity));
                        bool isGoblin = (stats[player]
                                         && (stats[player]->type == GOBLIN
                                             || (stats[player]->playerRace == RACE_GOBLIN && stats[player]->stat_appearance == 0)));
                        if ( icon.conditionalAttribute == "SPELLBOOK_CAST_BONUS" )
                        {
                            if ( !items[item->type].hasAttribute(icon.conditionalAttribute) )
                            {
                                continue;
                            }
                            if ( spell->ID == SPELL_CUREAILMENT && getSpellbookBonusPercent(players[player]->entity, stats[player], item) < 25 )
                            {
                                continue;
                            }
                        }
                        else if ( icon.conditionalAttribute == "SPELLBOOK_UNLEARNED" )
                        {
							if ( compendiumTooltip && intro ) { continue; }
                            if ( isGoblin || playerLearnedSpellbook(player, item) || (spell && skillLVL >= spell->difficulty) )
                            {
                                continue;
                            }
                        }
                        else if ( icon.conditionalAttribute == "SPELLBOOK_UNLEARNABLE" )
                        {
                            if ( !isGoblin ) { continue; }
							if ( compendiumTooltip && intro ) { continue; }
                            if ( playerLearnedSpellbook(player, item) )
                            {
                                continue;
                            }
                        }
                        else if ( icon.conditionalAttribute == "SPELLBOOK_LEARNABLE" )
                        {
                            if ( isGoblin ) { continue; }
							if ( compendiumTooltip && intro ) { continue; }
                            if ( playerLearnedSpellbook(player, item) || (spell && skillLVL < spell->difficulty) )
                            {
                                continue;
                            }
                        }
                        else if ( icon.conditionalAttribute == "SPELLBOOK_SPELLINFO_UNLEARNED"
                                 || icon.conditionalAttribute == "SPELLBOOK_SPELLINFO_LEARNED" )
                        {
                            if ( icon.conditionalAttribute == "SPELLBOOK_SPELLINFO_LEARNED" )
                            {
                                if ( !(compendiumTooltip && intro) && !playerLearnedSpellbook(player, item) )
                                {
                                    continue;
                                }
                            }
                            else if ( icon.conditionalAttribute == "SPELLBOOK_SPELLINFO_UNLEARNED" )
                            {
								if ( compendiumTooltip && intro ) { continue; }
                                if ( playerLearnedSpellbook(player, item) )
                                {
                                    continue;
                                }
                            }
                            icon.iconPath = ItemTooltips.getSpellIconPath(player, *item, -1);
                        }
                        else if ( !items[item->type].hasAttribute(icon.conditionalAttribute) )
                        {
                            continue;
                        }
                    }
                    else
                    {
                        if ( !items[item->type].hasAttribute(icon.conditionalAttribute) )
                        {
                            continue;
                        }
                    }
                    if ( itemCategory(item) == RING && index > 0 && icon.conditionalAttribute == "AC" )
                    {
                        if ( item->armorGetAC(stats[player]) == 0 )
                        {
                            continue;
                        }
                    }
                }
                
                std::map<int, Uint32> positiveWordIndexes;
                std::map<int, Uint32> negativeWordIndexes;
                std::map<int, Uint32> highlightWordIndexes;
                
                if ( index == 0 )
                {
                    imgPrimaryIcon->disabled = false;
                    imgPrimaryIcon->path = icon.iconPath;
                    
                    std::string iconText = icon.text;
                    ItemTooltips.formatItemIcon(player, tooltipType, *item, iconText, index, icon.conditionalAttribute, parentFrame);
                    
                    if ( tooltipType.find("tooltip_spell_") != std::string::npos && iconText == "" )
                    {
                        imgPrimaryIcon->disabled = true;
                        continue;
                    }
                    
                    std::string bracketText = "";
                    ItemTooltips.stripOutHighlightBracketText(iconText, bracketText);
                    std::string positiveText = "";
                    std::string negativeText = "";
                    ItemTooltips.stripOutPositiveNegativeItemDetails(iconText, positiveText, negativeText);
                    ItemTooltips.getWordIndexesItemDetails(txtPrimaryValue, iconText, bracketText, positiveText, negativeText,
                                                           highlightWordIndexes, positiveWordIndexes, negativeWordIndexes, itemTooltip);
                    
                    txtPrimaryValue->setText(iconText.c_str());
                    txtPrimaryValue->setTextColor(icon.textColor);
                    
                    //txtPrimaryValueHighlight->setText(bracketText.c_str());
                    txtPrimaryValueHighlight->setTextColor(itemTooltip.statusEffectTextColor);
                    
                    //txtPrimaryValuePositive->setText(positiveText.c_str());
                    txtPrimaryValuePositive->setTextColor(itemTooltip.positiveTextColor);
                    
                    //txtPrimaryValueNegative->setText(negativeText.c_str());
                    txtPrimaryValueNegative->setTextColor(itemTooltip.negativeTextColor);
                }
                else if ( index == 1 )
                {
                    imgSecondaryIcon->disabled = false;
                    imgSecondaryIcon->path = icon.iconPath;
                    
                    std::string iconText = icon.text;
                    ItemTooltips.formatItemIcon(player, tooltipType, *item, iconText, index, icon.conditionalAttribute, parentFrame);
                    
                    std::string bracketText = "";
                    ItemTooltips.stripOutHighlightBracketText(iconText, bracketText);
                    std::string positiveText = "";
                    std::string negativeText = "";
                    ItemTooltips.stripOutPositiveNegativeItemDetails(iconText, positiveText, negativeText);
                    ItemTooltips.getWordIndexesItemDetails(txtSecondaryValue, iconText, bracketText, positiveText, negativeText,
                                                           highlightWordIndexes, positiveWordIndexes, negativeWordIndexes, itemTooltip);
                    
                    txtSecondaryValue->setText(iconText.c_str());
                    txtSecondaryValue->setTextColor(icon.textColor);
                    //txtSecondaryValueHighlight->setText(bracketText.c_str());
                    txtSecondaryValueHighlight->setTextColor(itemTooltip.statusEffectTextColor);
                    
                    //txtSecondaryValuePositive->setText(positiveText.c_str());
                    txtSecondaryValuePositive->setTextColor(itemTooltip.positiveTextColor);
                    
                    //txtSecondaryValueNegative->setText(negativeText.c_str());
                    txtSecondaryValueNegative->setTextColor(itemTooltip.negativeTextColor);
                }
                else if ( index == 2 )
                {
                    imgThirdIcon->disabled = false;
                    imgThirdIcon->path = icon.iconPath;
                    
                    std::string iconText = icon.text;
                    ItemTooltips.formatItemIcon(player, tooltipType, *item, iconText, index, icon.conditionalAttribute, parentFrame);
                    
                    std::string bracketText = "";
                    ItemTooltips.stripOutHighlightBracketText(iconText, bracketText);
                    std::string positiveText = "";
                    std::string negativeText = "";
                    ItemTooltips.stripOutPositiveNegativeItemDetails(iconText, positiveText, negativeText);
                    ItemTooltips.getWordIndexesItemDetails(txtThirdValue, iconText, bracketText, positiveText, negativeText,
                                                           highlightWordIndexes, positiveWordIndexes, negativeWordIndexes, itemTooltip);
                    
                    txtThirdValue->setText(iconText.c_str());
                    txtThirdValue->setTextColor(icon.textColor);
                    //txtThirdValueHighlight->setText(bracketText.c_str());
                    txtThirdValueHighlight->setTextColor(itemTooltip.statusEffectTextColor);
                    
                    //txtThirdValuePositive->setText(positiveText.c_str());
                    txtThirdValuePositive->setTextColor(itemTooltip.positiveTextColor);
                    
                    //txtThirdValueNegative->setText(negativeText.c_str());
                    txtThirdValueNegative->setTextColor(itemTooltip.negativeTextColor);
                }
                else
                {
                    break;
                }
                ++index;
            }
        }
        
        auto txtAttributes = frameAttr->findField("inventory mouse tooltip attributes text");
        txtAttributes->setDisabled(true);
        
        std::string descriptionTextString = "";
        if ( !doShortTooltip && (itemTooltip.descriptionText.size() > 0 || descriptionTextString.size() > 0) )
        {
            txtAttributes->setDisabled(false);
            txtAttributes->setColor(itemTooltip.descriptionTextColor);
            int index = 0;
            for ( auto it = itemTooltip.descriptionText.begin(); it != itemTooltip.descriptionText.end(); ++it )
            {
                descriptionTextString += (*it);
                if ( std::next(it) != itemTooltip.descriptionText.end() )
                {
                    descriptionTextString += '\n';
                }
            }
            ItemTooltips.formatItemDescription(player, tooltipType, *item, descriptionTextString);
            txtAttributes->setText(descriptionTextString.c_str());
            
            if ( descriptionTextString == "" )
            {
                txtAttributes->setDisabled(true);
            }
        }
        
        frameDesc->setDisabled(true);

        std::string detailsTextString = "";
        if ( !doShortTooltip && itemTooltip.detailsText.size() > 0 )
        {
            frameDesc->setDisabled(false);
            txtDescription->setTextColor(itemTooltip.detailsTextColor);
            int index = 0;
            
            for ( auto& tag : itemTooltip.detailsTextInsertOrder )
            {
                if ( itemCategory(item) == POTION )
                {
                    if ( tag.compare("potion_on_cursed") == 0 &&
                        (!items[item->type].hasAttribute("POTION_CURSED_SIDE_EFFECT") || item->beatitude >= 0) )
                    {
                        continue;
                    }
                    else if ( tag.compare("potion_on_blessed") == 0 && item->beatitude <= 0 )
                    {
                        continue;
                    }
                    else if ( tag.compare("alchemy_details") == 0 )
                    {
                        if ( detailsTextString.compare("") != 0 ) // non-empty
                        {
                            detailsTextString += '\n'; // add a padding newline
                        }
                    }
                }
                else if ( item->type == TOOL_BEARTRAP )
                {
                    if ( tag.compare("beartrap_degrade_on_use_cursed") == 0 )
                    {
                        if ( item->status == BROKEN || item->beatitude >= 0 ) { continue; }
                    }
                    else if ( tag.compare("beartrap_degrade_on_use") == 0 )
                    {
                        if ( item->status == BROKEN || item->beatitude < 0 )
                        {
                            continue;
                        }
                    }
                    else if ( tag.compare("on_use") == 0 )
                    {
                        if ( item->status == BROKEN )
                        {
                            continue;
                        }
                    }
                }
                else if ( item->type == TOOL_SENTRYBOT || item->type == TOOL_SPELLBOT
                         || item->type == TOOL_GYROBOT || item->type == TOOL_DUMMYBOT )
                {
                    if ( item->status == BROKEN )
                    {
                        continue;
                    }
                    else if ( tag.compare("tinkerbot_atk_initiative") == 0 )
                    {
                        if ( item->status < SERVICABLE )
                        {
                            continue;
                        }
                    }
                    else if ( tag.compare("gyrobot_info_interact") == 0 )
                    {
                        if ( item->status < SERVICABLE )
                        {
                            continue;
                        }
                    }
                }
                else if ( itemCategory(item) == SCROLL )
                {
                    if ( tag.compare("scroll_on_cursed_sideeffect") == 0 )
                    {
                        if ( !items[item->type].hasAttribute("SCROLL_CURSED_SIDE_EFFECT") )
                        {
                            continue;
                        }
                        else if ( item->beatitude >= 0 )
                        {
                            continue;
                        }
                    }
                }
                else if ( itemCategory(item) == FOOD )
                {
                    if ( tag.compare("food_on_cursed_sideeffect") == 0 && item->beatitude >= 0 )
                    {
                        continue;
                    }
                }
                else if ( itemCategory(item) == WEAPON || itemCategory(item) == ARMOR || itemCategory(item) == TOOL
                         || itemCategory(item) == AMULET || itemCategory(item) == RING || itemTypeIsQuiver(item->type)
                         || itemCategory(item) == GEM )
                {
                    if ( tag.compare("weapon_durability") == 0 )
                    {
                        int proficiency = itemCategory(item) == ARMOR ? PRO_UNARMED : getWeaponSkill(item);
                        if ( !(compendiumTooltip && intro) && stats[player]->getModifiedProficiency(proficiency) == SKILL_LEVEL_LEGENDARY )
                        {
                            continue;
                        }
                    }
                    else if ( tag.compare("weapon_legendary_durability") == 0 )
                    {
                        int proficiency = itemCategory(item) == ARMOR ? PRO_UNARMED: getWeaponSkill(item);
                        if ( (compendiumTooltip && intro) || stats[player]->getModifiedProficiency(proficiency) != SKILL_LEVEL_LEGENDARY )
                        {
                            continue;
                        }
                    }
                    else if ( tag.compare("weapon_bonus_exp") == 0 )
                    {
                        if ( !items[item->type].hasAttribute("BONUS_SKILL_EXP") )
                        {
                            continue;
                        }
                    }
                    else if ( tag.compare("equipment_fragile_durability") == 0 )
                    {
                        if ( !items[item->type].hasAttribute("FRAGILE") )
                        {
                            continue;
                        }
                    }
                    else if ( tag.compare("equipment_fragile_durability") == 0 )
                    {
                        if ( !items[item->type].hasAttribute("FRAGILE") )
                        {
                            continue;
                        }
                    }
                    else if ( tag.compare("weapon_ranged_quiver_augment") == 0 )
                    {
                        if ( items[item->type].hasAttribute("RANGED_NO_QUIVER") )
                        {
                            continue;
                        }
                    }
                    else if ( tag.compare("weapon_ranged_rate_of_fire") == 0 )
                    {
                        // skip on 0, 100 or non-existing
                        if ( !items[item->type].hasAttribute("RATE_OF_FIRE") )
                        {
                            continue;
                        }
                        else if ( items[item->type].hasAttribute("RATE_OF_FIRE")
                                 && (items[item->type].attributes["RATE_OF_FIRE"] == 100
                                     || items[item->type].attributes["RATE_OF_FIRE"] == 0) )
                        {
                            continue;
                        }
                    }
                    else if ( tag.compare("weapon_ranged_armor_pierce") == 0 )
                    {
                        if ( !items[item->type].hasAttribute("ARMOR_PIERCE") )
                        {
                            continue;
                        }
                    }
                    else if ( tag.find("EFF_") != std::string::npos )
                    {
                        if ( !items[item->type].hasAttribute(tag) )
                        {
                            continue;
                        }
                    }
                    else if ( tag.compare("equipment_stat_bonus") == 0 )
                    {
                        if ( !items[item->type].hasAttribute("STR")
                            && !items[item->type].hasAttribute("DEX")
                            && !items[item->type].hasAttribute("CON")
                            && !items[item->type].hasAttribute("INT")
                            && !items[item->type].hasAttribute("PER")
                            && !items[item->type].hasAttribute("CHR") )
                        {
                            continue;
                        }
                    }
                    else if ( tag.compare("attributes_text_if_armor_has_stats") == 0
                             || tag.compare("blank_text_if_armor_has_stats") == 0 )
                    {
                        bool skip = true;
                        for ( auto it = ItemTooltips.templates["template_attributes_text_armor_conditional_tags"].begin();
                             it != ItemTooltips.templates["template_attributes_text_armor_conditional_tags"].end(); ++it )
                        {
                            if ( items[item->type].hasAttribute(*it) )
                            {
                                skip = false;
                                break;
                            }
                        }
                        if ( skip ) { continue; } // no attributes found, don't put this block of text in
                    }
                    else if ( tag.compare("equipment_on_cursed_sideeffect") == 0 && item->beatitude >= 0 )
                    {
                        continue;
                    }
                    else if ( tag.compare("armor_on_cursed_sideeffect") == 0
                             && (item->beatitude >= 0 || !items[item->type].hasAttribute("ARMOR_CURSED_SIDEEFFECT")) )
                    {
                        continue;
                    }
                    else if ( tag.compare("ring_on_cursed_sideeffect") == 0
                             && (item->beatitude >= 0 || !items[item->type].hasAttribute("RING_CURSED_SIDEEFFECT")) )
                    {
                        continue;
                    }
                    else if ( tag.compare("on_degraded") == 0 )
                    {
                        if ( items[item->type].hasAttribute("ARTIFACT_STATUS_ATK") )
                        {
                            continue;
                        }
                    }
					else if ( tag.compare("armor_ac_text") == 0
						|| tag.compare("armor_base_ac") == 0
						|| tag.compare("on_bless_or_curse") == 0 )
					{
						if ( !Item::doesItemProvideBeatitudeAC(item->type) )
						{
							continue;
						}
					}
                    else if ( tag.compare("artifact_armor_on_degraded") == 0 )
                    {
                        if ( !items[item->type].hasAttribute("ARTIFACT_STATUS_AC") )
                        {
                            continue;
                        }
                    }
                    else if ( tag.compare("artifact_weapon_on_degraded") == 0 )
                    {
                        if ( !items[item->type].hasAttribute("ARTIFACT_STATUS_ATK") )
                        {
                            continue;
                        }
                    }
                }
                else if ( itemCategory(item) == SPELLBOOK )
                {
                    if ( tag.compare("spellbook_cast_bonus") == 0 )
                    {
                        if ( !items[item->type].hasAttribute("SPELLBOOK_CAST_BONUS") )
                        {
                            continue;
                        }
                    }
                    else if ( tag.compare("spellbook_cast_success") == 0
                             || tag.compare("spellbook_extramana_chance") == 0 )
                    {
						if ( compendiumTooltip && intro )
						{
							continue;
						}
                        bool newbie = isSpellcasterBeginnerFromSpellbook(player, players[player]->entity, stats[player],
                                                                         getSpellFromID(getSpellIDFromSpellbook(item->type)), item);
                        if ( !newbie )
                        {
                            continue;
                        }
                    }
                    else if ( tag.compare("spellbook_magic_requirement") == 0
                             || tag.compare("spellbook_magic_current") == 0
                             || tag.compare("spellbook_unlearned_blank_space") == 0 )
                    {
						if ( compendiumTooltip && intro )
						{
							continue;
						}
                        if ( playerLearnedSpellbook(player, item) )
                        {
                            continue;
                        }
                    }
                }
                else if ( itemCategory(item) == SPELL_CAT )
                {
                    spell_t* spell = getSpellFromItem(player, item, false);
                    if ( tag.compare("spell_damage_bonus") == 0 )
                    {
                        if ( !ItemTooltips.bIsSpellDamageOrHealingType(spell) )
                        {
                            continue;
                        }
                    }
                    else if ( tag.compare("spell_cast_success") == 0
                             || tag.compare("spell_extramana_chance") == 0 )
                    {
						if ( compendiumTooltip && intro )
						{
							continue;
						}
                        bool newbie = isSpellcasterBeginner(player, players[player]->entity);
                        if ( !newbie )
                        {
                            continue;
                        }
                    }
                    else if ( tag.compare("spell_newbie_newline") == 0 )
                    {
						if ( compendiumTooltip && intro )
						{
							continue;
						}
                        bool newbie = isSpellcasterBeginner(player, players[player]->entity);
                        if ( !newbie )
                        {
                            continue;
                        }
                        else if ( detailsTextString.compare("") == 0 )
                        {
                            continue; // don't insert this newline
                        }
                    }
                    else if ( tag.find("attribute_spell_") != std::string::npos )
                    {
                        spell_t* spell = getSpellFromItem(player, item, false);
                        if ( !spell ) { continue; }
                        std::string subs = tag.substr(10, std::string::npos);
                        if ( !spell || ItemTooltips.spellItems[spell->ID].internalName != subs )
                        {
                            continue;
                        }
                    }
                }
                else if ( itemCategory(item) == MAGICSTAFF )
                {
                    if ( tag.compare("magicstaff_degrade_chance") == 0 )
                    {
                        if ( item->type == MAGICSTAFF_CHARM )
                        {
                            continue;
                        }
                    }
                    else if ( tag.compare("magicstaff_charm_degrade_chance") == 0 )
                    {
                        if ( item->type != MAGICSTAFF_CHARM )
                        {
                            continue;
                        }
                    }
                    else if ( tag.find("attribute_spell_") != std::string::npos )
                    {
                        spell_t* spell = nullptr;
                        for ( auto& s : ItemTooltips.spellItems )
                        {
                            if ( s.second.magicstaffId == item->type )
                            {
                                spell = getSpellFromID(s.first);
                                break;
                            }
                        }
                        if ( !spell ) { continue; }
                        std::string subs = tag.substr(10, std::string::npos);
                        if ( !spell || ItemTooltips.spellItems[spell->ID].internalName != subs )
                        {
                            continue;
                        }
                    }
                }
                
                if ( items[item->type].item_slot == ItemEquippableSlot::EQUIPPABLE_IN_SLOT_SHIELD )
                {
                    if ( tag.compare("shield_durability") == 0 )
                    {
                        if ( items[item->type].hasAttribute("EFF_SHIELD_BRITTLE") )
                        {
                            continue;
                        }
                        if ( !(compendiumTooltip && intro) && stats[player]->getModifiedProficiency(PRO_SHIELD) == SKILL_LEVEL_LEGENDARY )
                        {
                            continue;
                        }
                    }
                    else if ( tag.compare("shield_legendary_durability") == 0 )
                    {
                        if ( items[item->type].hasAttribute("EFF_SHIELD_BRITTLE") )
                        {
                            continue;
                        }
                        if ( (compendiumTooltip && intro) || stats[player]->getModifiedProficiency(PRO_SHIELD) != SKILL_LEVEL_LEGENDARY )
                        {
                            continue;
                        }
                    }
                }
                
                std::string tagText = "";
                for ( auto it = itemTooltip.detailsText[tag.c_str()].begin(); it != itemTooltip.detailsText[tag.c_str()].end(); ++it )
                {
                    tagText += (*it);
                    if ( std::next(it) != itemTooltip.detailsText[tag.c_str()].end() )
                    {
                        tagText += '\n';
                    }
                }
                ItemTooltips.formatItemDetails(player, tooltipType, *item, tagText, tag, parentFrame);
                if ( detailsTextString.compare("") != 0 )
                {
                    detailsTextString += '\n';
                }
                detailsTextString += tagText;
            }
			if ( detailsTextString.size() > 2 && detailsTextString[detailsTextString.size() - 1] == '\n' )
			{
				// if ending on a newline, remove it
				detailsTextString[detailsTextString.size() - 1] = ' ';
			}
            txtDescription->setText(detailsTextString.c_str());
        }
        
        if ( detailsTextString == "" )
        {
            frameDesc->setDisabled(true);
        }
        
        const int imgToTextOffset = 0;
        txtPrimaryValue->setDisabled(true);
        txtPrimaryValueHighlight->setDisabled(txtPrimaryValue->isDisabled());
        txtPrimaryValuePositive->setDisabled(txtPrimaryValue->isDisabled());
        txtPrimaryValueNegative->setDisabled(txtPrimaryValue->isDisabled());
        txtPrimaryValue->setDisabled(imgPrimaryIcon->disabled);
        
        auto txtSlotName = frameAttr->findField("inventory mouse tooltip primary value slot name");
        txtSlotName->setDisabled(true);
        if ( items[item->type].item_slot != ItemEquippableSlot::NO_EQUIP )
        {
            txtSlotName->setDisabled(false);
            txtSlotName->setColor(itemTooltip.faintTextColor);
            txtSlotName->setSize(SDL_Rect{padx, pady * 2 + imgToTextOffset + 1, txtHeader->getSize().w, imgPrimaryIcon->pos.h});
            txtSlotName->setText(ItemTooltips.getItemSlotName(items[item->type].item_slot).c_str());
        }
        
        const int iconPadx = 8;
        const int iconTextPadx = 4;
        
        int numPrimaryLines = 0;
        if ( !imgPrimaryIcon->disabled )
        {
            imgPrimaryIcon->pos.x = iconPadx;
            imgPrimaryIcon->pos.y = pady * 2;
            
            if ( !imgSpellIcon->disabled )
            {
                static ConsoleVariable<int> cvar_spelltooltipIconY2("/spell_tooltip_icon_y2", -2);
                imgPrimaryIcon->pos.y = imgSpellIcon->pos.y + (imgSpellIcon->pos.h / 2) - imgPrimaryIcon->pos.h / 2 + *cvar_spelltooltipIconY2;
            }
            
            int iconMultipleLinePadding = 0;
            numPrimaryLines = txtPrimaryValue->getNumTextLines();
            if ( numPrimaryLines > 1 )
            {
                imgPrimaryIcon->pos.y += pady * (std::max(0, numPrimaryLines - 1)); // for each line > 1 add padding
                iconMultipleLinePadding = numPrimaryLines * Font::get(txtPrimaryValue->getFont())->height() - imgPrimaryIcon->pos.h;
            }
            
			const int txtPosPadY = -2;
            SDL_Rect txtPos {
                imgPrimaryIcon->pos.x + imgPrimaryIcon->pos.w + padx + iconTextPadx,
                imgPrimaryIcon->pos.y + imgToTextOffset - iconMultipleLinePadding / 2 + txtPosPadY
				- (std::max(0, numPrimaryLines - 1) * txtPrimaryValue->getPaddingPerLine() / 2),
                txtHeader->getSize().w,
                imgPrimaryIcon->pos.h + iconMultipleLinePadding
            };
            if ( txtPos.y % 2 == 0 )
            {
                ++txtPos.y;
            }
            txtPrimaryValue->setSize(txtPos);
            txtPrimaryValueHighlight->setSize(txtPos);
            txtPrimaryValuePositive->setSize(txtPos);
            txtPrimaryValueNegative->setSize(txtPos);
        }
        
        txtSecondaryValue->setDisabled(true);
        txtSecondaryValueHighlight->setDisabled(txtSecondaryValue->isDisabled());
        txtSecondaryValuePositive->setDisabled(txtSecondaryValue->isDisabled());
        txtSecondaryValueNegative->setDisabled(txtSecondaryValue->isDisabled());
        txtSecondaryValue->setDisabled(imgSecondaryIcon->disabled);
        
        txtThirdValue->setDisabled(true);
        txtThirdValueHighlight->setDisabled(txtThirdValue->isDisabled());
        txtThirdValuePositive->setDisabled(txtThirdValue->isDisabled());
        txtThirdValueNegative->setDisabled(txtThirdValue->isDisabled());
        txtThirdValue->setDisabled(imgThirdIcon->disabled);
        
        int numSecondaryLines = 0;
        if ( !imgSecondaryIcon->disabled )
        {
            imgSecondaryIcon->pos.x = iconPadx;
            if ( imgPrimaryIcon->disabled )
            {
                imgSecondaryIcon->pos.y = pady * 2;
            }
            else
            {
                imgSecondaryIcon->pos.y = pady * 2 + imgPrimaryIcon->pos.y + imgPrimaryIcon->pos.h;
            }
            
            int iconMultipleLinePadding = 0;
            numSecondaryLines = txtSecondaryValue->getNumTextLines();
            if ( numSecondaryLines > 1 )
            {
                imgSecondaryIcon->pos.y += pady * (std::max(0, numSecondaryLines - 1)); // for each line > 1 add padding
                iconMultipleLinePadding = numSecondaryLines * Font::get(txtSecondaryValue->getFont())->height() - imgSecondaryIcon->pos.h;
            }
            
			const int txtPosPadY = -2;
            SDL_Rect txtPos {
                imgSecondaryIcon->pos.x + imgSecondaryIcon->pos.w + padx + iconTextPadx,
                imgSecondaryIcon->pos.y + imgToTextOffset - iconMultipleLinePadding / 2 + txtPosPadY
				- (std::max(0, numSecondaryLines - 1) * txtSecondaryValue->getPaddingPerLine() / 2),
                txtHeader->getSize().w,
                imgSecondaryIcon->pos.h + iconMultipleLinePadding
            };
            if ( txtPos.y % 2 == 0 )
            {
                ++txtPos.y;
            }
            txtSecondaryValue->setSize(txtPos);
            txtSecondaryValueHighlight->setSize(txtPos);
            txtSecondaryValuePositive->setSize(txtPos);
            txtSecondaryValueNegative->setSize(txtPos);
        }
        
        int numThirdLines = 0;
        if ( !imgThirdIcon->disabled )
        {
            imgThirdIcon->pos.x = iconPadx;
            if ( imgPrimaryIcon->disabled )
            {
                imgThirdIcon->pos.y = pady * 2;
            }
            else if ( imgSecondaryIcon->disabled )
            {
                imgThirdIcon->pos.y = pady * 2 + imgPrimaryIcon->pos.y + imgPrimaryIcon->pos.h;
            }
            else
            {
                imgThirdIcon->pos.y = pady * 2 + imgSecondaryIcon->pos.y + imgSecondaryIcon->pos.h;
            }
            
            int iconMultipleLinePadding = 0;
            numThirdLines = txtThirdValue->getNumTextLines();
            if ( numThirdLines > 1 )
            {
                imgThirdIcon->pos.y += pady * (std::max(0, numThirdLines - 1)); // for each line > 1 add padding
                iconMultipleLinePadding = numThirdLines * Font::get(txtThirdValue->getFont())->height() - imgThirdIcon->pos.h;
            }
            
			const int txtPosPadY = -2;
            SDL_Rect txtPos {
                imgThirdIcon->pos.x + imgThirdIcon->pos.w + padx + iconTextPadx,
                imgThirdIcon->pos.y + imgToTextOffset - iconMultipleLinePadding / 2 + txtPosPadY
				- (std::max(0, numThirdLines - 1) * txtThirdValue->getPaddingPerLine() / 2),
                txtHeader->getSize().w,
                imgThirdIcon->pos.h + iconMultipleLinePadding
            };
            if ( txtPos.y % 2 == 0 )
            {
                ++txtPos.y;
            }
            txtThirdValue->setSize(txtPos);
            txtThirdValueHighlight->setSize(txtPos);
            txtThirdValuePositive->setSize(txtPos);
            txtThirdValueNegative->setSize(txtPos);
        }
        
        bool imagesDisabled = true;
        for ( auto image : frameAttr->getImages() )
        {
            if ( !image->disabled )
            {
                imagesDisabled = false;
            }
        }
        // attribute frame size - add height after the icons
        if ( imagesDisabled )
        {
            frameAttrPos.h += 2 * pady; // no icons, just text.
        }
        else
        {
            // original - int iconHeight1 = imgPrimaryIcon->disabled ? 0 : (imgPrimaryIcon->pos.y + std::max(txtPrimaryValue->getSize().h, imgPrimaryIcon->pos.h));
            int iconHeight1 = imgPrimaryIcon->disabled ? 0 :
            (std::max(txtPrimaryValue->getSize().y + txtPrimaryValue->getSize().h
                      + (numPrimaryLines > 1 ? (numPrimaryLines - 1) * (int)(txtPrimaryValue->getPaddingPerLine()) : 0),
                      imgPrimaryIcon->pos.y + imgPrimaryIcon->pos.h));
            int iconHeight2 = imgSecondaryIcon->disabled ? 0 :
            (std::max(txtSecondaryValue->getSize().y + txtSecondaryValue->getSize().h
                      + (numSecondaryLines > 1 ? (numSecondaryLines - 1) * (int)(txtSecondaryValue->getPaddingPerLine()) : 0),
                      imgSecondaryIcon->pos.y + imgSecondaryIcon->pos.h));
            int iconHeight3 = imgThirdIcon->disabled ? 0 :
            (std::max(txtThirdValue->getSize().y + txtThirdValue->getSize().h
                      + (numThirdLines > 1 ? (numThirdLines - 1) * (int)(txtThirdValue->getPaddingPerLine()) : 0),
                      imgThirdIcon->pos.y + imgThirdIcon->pos.h));
            
            if ( !imgSpellIcon->disabled && txtPrimaryValue->getNumTextLines() <= 1 )
            {
                iconHeight1 += 2 * pady; // extra padding for the spell icon
            }
            
            frameAttrPos.h += std::max(std::max(iconHeight1, iconHeight2), iconHeight3);
            frameAttrPos.h += pady;
            
            //if ( txtAttributes->isDisabled() )
            //{
            //    frameAttrPos.h += pady; // no description, add some padding
            //}
        }
        
        if ( !txtAttributes->isDisabled() )
        {
            int charHeight = 13;
            Font::get(txtAttributes->getFont())->sizeText("_", nullptr, &charHeight);
            //messagePlayer(0, "%d", charHeight);
            
            
            int attributesWidth = frameAttrPos.w;
            txtAttributes->setSize(SDL_Rect{ padx * 2, frameAttrPos.h + pady, attributesWidth - padx * 2, frameAttrPos.h - pady });
            if ( tooltipType.find("tooltip_spell_") != std::string::npos )
            {
                // don't reflow
            }
            else
            {
                txtAttributes->reflowTextToFit(-1);
            }
            int currentHeight = frameAttrPos.h;
            frameAttrPos.h += txtAttributes->getNumTextLines() * charHeight + pady * 3;
            txtAttributes->setSize(SDL_Rect{ padx * 2, currentHeight + pady, attributesWidth - padx * 2, frameAttrPos.h - pady });
        }
        
        frameAttr->setSize(frameAttrPos);
        tooltipDisplayedSettings.tooltipAttributeHeight = frameAttrPos.h;
        totalHeight += frameAttrPos.h;
        
        SDL_Rect frameDescPos = frameAttrPos;
        if ( !frameDesc->isDisabled() )
        {
            frameDescPos.y = frameAttrPos.y + frameAttrPos.h;
            frameDescPos.h = pady;
            
            int _pady = pady;
            if ( !imagesDisabled || !txtAttributes->isDisabled() )
            {
                _pady = 2 * pady;
            }
            
            int charHeight = 13;
            Font::get(txtDescription->getFont())->sizeText("_", nullptr, &charHeight);
            
            txtDescription->setSize(SDL_Rect{ padx * 2, _pady /*pady + tmpx*/, frameDescPos.w, frameDescPos.h - pady });
            
            if ( tooltipType == "tooltip_spell_item" || tooltipType == "tooltip_whip" )
            {
                txtDescription->reflowTextToFit(1);
            }
            else
            {
                txtDescription->reflowTextToFit(-1);
            }
            
            std::string detailsPositiveText = "";
            std::string detailsNegativeText = "";
            
            detailsTextString = txtDescription->getText();
            ItemTooltips.stripOutPositiveNegativeItemDetails(detailsTextString, detailsPositiveText, detailsNegativeText);
            
            std::map<int, Uint32> positiveWordIndexes;
            std::map<int, Uint32> negativeWordIndexes;
            std::map<int, Uint32> highlightWordIndexes;
            std::string empty;
            ItemTooltips.getWordIndexesItemDetails(txtDescription, detailsTextString, empty, detailsPositiveText, detailsNegativeText,
                                                   highlightWordIndexes, positiveWordIndexes, negativeWordIndexes, itemTooltip);
            txtDescription->setText(detailsTextString.c_str());
            
            //txtDescriptionPositive->setText(detailsPositiveText.c_str());
            //txtDescriptionPositive->setTextColor(itemTooltip.positiveTextColor);
            //txtDescriptionNegative->setText(detailsNegativeText.c_str());
            //txtDescriptionNegative->setTextColor(itemTooltip.negativeTextColor);
            
            txtDescriptionPositive->setDisabled(true);
            txtDescriptionNegative->setDisabled(true);
            
            frameDescPos.h += txtDescription->getNumTextLines() * charHeight + pady * 3;
            
            txtDescription->setSize(SDL_Rect{ padx * 2, _pady /*pady + tmpx*/, frameDescPos.w, frameDescPos.h - pady});
            txtDescription->setScroll(true);
            
            txtDescriptionPositive->setSize(txtDescription->getSize());
            txtDescriptionNegative->setSize(txtDescription->getSize());
            
            tooltipDisplayedSettings.tooltipDescriptionHeight = frameDescPos.h;
            //frameDescPos.h *= (tooltipDisplayedSettings.expandCurrent);
            
            frameDesc->setSize(frameDescPos);
            totalHeight += frameDescPos.h;
        }
        else
        {
            tooltipDisplayedSettings.tooltipDescriptionHeight = 0;
        }
        
        auto txtGoldValue = frameValues->findField("inventory mouse tooltip gold value");
        auto txtWeightValue = frameValues->findField("inventory mouse tooltip weight value");
        auto txtIdentifiedValue = frameValues->findField("inventory mouse tooltip identified value");
        
        SDL_Rect frameValuesPos = frameDescPos;
        if ( !frameValues->isDisabled() )
        {
            frameValuesPos.y = frameDescPos.y + frameDescPos.h;
            frameValuesPos.h = 0;
            
            char valueBuf[64];
            if ( tooltipType.find("tooltip_spell_") != std::string::npos )
            {
                imgWeightIcon->disabled = true;
                txtWeightValue->setDisabled(true);
                
                std::string spellCost = ItemTooltips.getCostOfSpellString(player, *item);
                txtGoldValue->setText(spellCost.c_str());
                if ( txtGoldValue->getNumTextLines() > 1 )
                {
                    txtWeightValue->setText(spellCost.substr(spellCost.find('\n') + 1).c_str());
                    txtGoldValue->setText(spellCost.substr(0, spellCost.find('\n')).c_str());
                    txtWeightValue->setDisabled(false);
                }
            }
            else
            {
				if ( GenericGUI[player].assistShrineGUI.bOpen && GenericGUI[player].assistShrineGUI.itemIsFromGUI(item) )
				{
					snprintf(valueBuf, sizeof(valueBuf), "%d", GenericGUI[player].assistShrineGUI.getAssistPointFromItem(item));
				}
				else
				{
					if ( !item->identified && itemCategory(item) == GEM )
					{
						snprintf(valueBuf, sizeof(valueBuf), "%d", items[GEM_GLASS].value);
					}
					else
					{
						snprintf(valueBuf, sizeof(valueBuf), "%d", items[item->type].value);
					}
				}
                txtGoldValue->setText(valueBuf);
            }
            txtGoldValue->setDisabled(false);
            
            int charWidth = 8;
            int charHeight = 13;
            const int lowerIconImgToTextOffset = 0;
            
            const std::string goldImagePath = "images/ui/Inventory/tooltips/HUD_Tooltip_Icon_Money_00.png";
            const std::string weightImagePath = "images/ui/Inventory/tooltips/HUD_Tooltip_Icon_WGT_00.png";
            const std::string spellMPCostImagePath = "images/ui/Inventory/tooltips/HUD_Tooltip_Icon_ManaRegen_00.png";
			const std::string assistImagePath = "images/ui/Inventory/tooltips/HUD_Tooltip_Icon_Assistance_00.png";
            
            Font::get(txtGoldValue->getFont())->sizeText("_", &charWidth, &charHeight);
            Font::get(txtWeightValue->getFont())->sizeText("_", &charWidth, &charHeight);
            if ( tooltipType.find("tooltip_spell_") != std::string::npos )
            {
                auto imgMPCost = imgGoldIcon;
                auto imgSustainedMPCost = imgWeightIcon;
                auto txtMPCost = txtGoldValue;
                auto txtSustainedMPCost = txtWeightValue;
                
                auto textGetMPCost = Text::get(txtMPCost->getText(), txtMPCost->getFont(),
                                               txtMPCost->getTextColor(), txtMPCost->getOutlineColor());
                
                imgMPCost->pos.x = frameValuesPos.w - (textGetMPCost->getWidth()) - (imgMPCost->pos.w + padx);
                imgMPCost->pos.y = pady;
                txtMPCost->setSize(SDL_Rect{ imgMPCost->pos.x + imgMPCost->pos.w + padx,
                    imgMPCost->pos.y + lowerIconImgToTextOffset, txtHeader->getSize().w, imgMPCost->pos.h });
                imgMPCost->path = spellMPCostImagePath;
                
                frameValuesPos.h += imgMPCost->disabled ? 0 : (imgMPCost->pos.y + imgMPCost->pos.h);
                frameValuesPos.h += pady;
                
                if ( !txtSustainedMPCost->isDisabled() )
                {
                    // get alignment of longer text for sustained MP cost
                    auto textGetSustainedMPCost = Text::get(txtSustainedMPCost->getText(), txtSustainedMPCost->getFont(),
                                                            txtSustainedMPCost->getTextColor(), txtSustainedMPCost->getOutlineColor());
                    
                    imgSustainedMPCost->pos.x = frameValuesPos.w - (textGetSustainedMPCost->getWidth()) - (imgSustainedMPCost->pos.w + padx);
                    imgSustainedMPCost->pos.y = pady + imgMPCost->pos.h;
                    txtSustainedMPCost->setSize(SDL_Rect{ imgSustainedMPCost->pos.x + imgSustainedMPCost->pos.w + padx,
                        imgSustainedMPCost->pos.y + lowerIconImgToTextOffset, txtHeader->getSize().w, imgSustainedMPCost->pos.h });
                    
                    frameValuesPos.h += imgSustainedMPCost->pos.h;
                    
                    // align left MP cost x to leftmost of sustained cost
                    imgMPCost->pos.x = imgSustainedMPCost->pos.x;
                    txtMPCost->setSize(SDL_Rect{ imgMPCost->pos.x + imgMPCost->pos.w + padx,
                        imgMPCost->pos.y + lowerIconImgToTextOffset, txtHeader->getSize().w, imgMPCost->pos.h });
                    
                    // move the MP cost image between the 2 lines, and a little left.
                    imgMPCost->pos.y += imgMPCost->pos.h / 2;
                    imgMPCost->pos.x -= padx;
                }
            }
            else
            {
                auto textGetGoldValue = Text::get(txtGoldValue->getText(), txtGoldValue->getFont(),
                                                  txtGoldValue->getTextColor(), txtGoldValue->getOutlineColor());
                
                imgGoldIcon->pos.x = frameValuesPos.w - (textGetGoldValue->getWidth()) - (imgGoldIcon->pos.w + padx);
                imgGoldIcon->pos.y = pady;
                txtGoldValue->setSize(SDL_Rect{ imgGoldIcon->pos.x + imgGoldIcon->pos.w + padx,
                    imgGoldIcon->pos.y + lowerIconImgToTextOffset, txtHeader->getSize().w, imgGoldIcon->pos.h });
				if ( GenericGUI[player].assistShrineGUI.bOpen && GenericGUI[player].assistShrineGUI.itemIsFromGUI(item) )
				{
					imgGoldIcon->path = assistImagePath;
				}
				else
				{
					imgGoldIcon->path = goldImagePath;
				}
                
                snprintf(valueBuf, sizeof(valueBuf), "%d", item->getWeight());
                txtWeightValue->setText(valueBuf);
                txtWeightValue->setDisabled(false);
                imgWeightIcon->disabled = false;
                
                auto textGetWeightValue = Text::get(txtWeightValue->getText(), txtWeightValue->getFont(),
                                                    txtWeightValue->getTextColor(), txtWeightValue->getOutlineColor());
                
                imgWeightIcon->pos.x = imgGoldIcon->pos.x - padx - (textGetWeightValue->getWidth()) - (imgWeightIcon->pos.w + padx);
                imgWeightIcon->pos.y = pady;
                txtWeightValue->setSize(SDL_Rect{ imgWeightIcon->pos.x + imgWeightIcon->pos.w + padx,
                    imgWeightIcon->pos.y + lowerIconImgToTextOffset, txtHeader->getSize().w, imgWeightIcon->pos.h });
                imgWeightIcon->path = weightImagePath;
                frameValuesPos.h += imgGoldIcon->disabled ? 0 : (imgGoldIcon->pos.y + imgGoldIcon->pos.h);
                frameValuesPos.h += pady;
            }
            
            frameValues->setSize(frameValuesPos);
            totalHeight += frameValuesPos.h;
            
            txtIdentifiedValue->setSize(SDL_Rect{ padx * 2, imgGoldIcon->pos.y + lowerIconImgToTextOffset,
                frameValuesPos.w - padx, txtGoldValue->getSize().h });
            
            if ( tooltipType.find("tooltip_spell_") != std::string::npos )
            {
                txtIdentifiedValue->setText(ItemTooltips.getSpellTypeString(player, *item).c_str());
            }
            else if ( !item->identified )
            {
                txtIdentifiedValue->setText(ItemTooltips.adjectives["item_identified_status"]["unidentified"].c_str());
            }
            else if ( item->beatitude > 0 )
            {
                txtIdentifiedValue->setText(ItemTooltips.adjectives["item_identified_status"]["blessed"].c_str());
            }
            else if ( item->beatitude < 0 )
            {
                txtIdentifiedValue->setText(ItemTooltips.adjectives["item_identified_status"]["cursed"].c_str());
            }
            else
            {
                txtIdentifiedValue->setText(ItemTooltips.adjectives["item_identified_status"]["uncursed"].c_str());
            }
            
            imgValueBackground->pos = SDL_Rect{0, 0, frameValuesPos.w, frameValuesPos.h };
            imgValueBackground->disabled = true;
        }
        else
        {
            txtGoldValue->setDisabled(true);
            txtWeightValue->setDisabled(true);
        }
        
        // add dividers
        auto divDesc = frameDesc->findImage("inventory mouse tooltip description divider");
        auto divValue = frameValues->findImage("inventory mouse tooltip value divider");
        divDesc->disabled = true;
        divValue->disabled = true;
        if ( (!imagesDisabled || !txtAttributes->isDisabled()) && !frameAttr->isDisabled() && !frameDesc->isDisabled() )
        {
            divDesc->disabled = false;
            divDesc->pos.x = padx;
            divDesc->pos.w = frameDescPos.w - 2 * padx;
        }
        if ( !frameValues->isDisabled() && (!imagesDisabled || !frameDesc->isDisabled() || !txtAttributes->isDisabled()) )
        {
            divValue->disabled = false;
            divValue->pos.x = padx;
            divValue->pos.w = frameValuesPos.w - 2 * padx;
        }
        
        // button prompts
        {
            SDL_Rect framePromptPos = frameValuesPos;
            framePromptPos.y = frameValuesPos.y + frameValuesPos.h;
            framePromptPos.h = imgBottomBackground->pos.h;
            
            if ( (!item->identified
                  && (!isItemFromInventory || (isItemFromInventory && inputs.getVirtualMouse(player)->draw_cursor && appraisal.current_item != item->uid)))
                || doShortTooltip
                || (frameDesc->isDisabled() && item->identified) )
            {
                imgBottomBackground->path = "images/ui/Inventory/tooltips/Hover_B00_NoPrompt.png";
                imgBottomBackgroundLeft->path = "images/ui/Inventory/tooltips/Hover_BL01_NoPrompt.png";
                imgBottomBackgroundRight->path = "images/ui/Inventory/tooltips/Hover_BR01_NoPrompt.png";
                txtPrompt->setDisabled(true);
                totalHeight += 4;
            }
            else
            {
                imgBottomBackground->path = "images/ui/Inventory/tooltips/Hover_B00.png";
                imgBottomBackgroundLeft->path = "images/ui/Inventory/tooltips/Hover_BL01.png";
                imgBottomBackgroundRight->path = "images/ui/Inventory/tooltips/Hover_BR01.png";
                totalHeight += framePromptPos.h;
                txtPrompt->setDisabled(false);
            }
            txtPrompt->setSize(SDL_Rect{ 0, 1, framePromptPos.w, framePromptPos.h });
            
            framePrompt->setSize(framePromptPos);
        }
        
        tooltipDisplayedSettings.tooltipHeight = totalHeight;
    }
    
    // realign stuff based on expanded animation status
    int scrollOverflow = 0;
    auto gradientTop = frameMain->findImage("inventory mouse tooltip fade top");
    auto gradientBottom = frameMain->findImage("inventory mouse tooltip fade bottom");
    auto& frameTooltipScrollSetpoint = tooltipDisplayedSettings.frameTooltipScrollSetpoint;
    auto& frameTooltipScrollAnim = tooltipDisplayedSettings.frameTooltipScrollAnim;
    bool wasScrollable = tooltipDisplayedSettings.scrollable;
    {
        SDL_Rect frameDescPos = frameDesc->getSize();
        frameDescPos.h = tooltipDisplayedSettings.tooltipDescriptionHeight;
        
        static ConsoleVariable<int> cvar_item_tooltip_max_height("/item_tooltip_max_height", 348);
        static ConsoleVariable<int> cvar_item_tooltip_max_height_compact("/item_tooltip_max_height_compact", 264);
        int maxHeight = !players[player]->bUseCompactGUIHeight() ? *cvar_item_tooltip_max_height : *cvar_item_tooltip_max_height_compact;
		if ( compendiumTooltip )
		{
			maxHeight = *cvar_item_tooltip_max_height;
		}
        else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_HOTBAR && players[player]->bUseCompactGUIHeight() )
        {
            maxHeight -= 68;
        }
        
        scrollOverflow = std::max(0, frameAttr->getSize().y + frameAttr->getSize().h + tooltipDisplayedSettings.tooltipDescriptionHeight - maxHeight);
        tooltipDisplayedSettings.scrollable = scrollOverflow > 0 && tooltipDisplayedSettings.expanded;
        frameDescPos.h = std::max(0, frameDescPos.h - scrollOverflow);
        frameDescPos.h *= tooltipDisplayedSettings.expandCurrent;
        int heightDiff = tooltipDisplayedSettings.tooltipDescriptionHeight - frameDescPos.h;
        frameDesc->setSize(frameDescPos);
        
        
        if ( tooltipDisplayedSettings.expanded )
        {
            if ( tooltipDisplayedSettings.scrollable )
            {
                const real_t fpsScale = getFPSScale(60.0);
                if ( inputs.bPlayerUsingKeyboardControl(player) )
                {
                    if ( Input::inputs[player].binaryToggle("MenuMouseWheelUpAlt") )
                    {
                        frameTooltipScrollSetpoint -= 24.0;
                    }
                    else if ( Input::inputs[player].binaryToggle("MenuMouseWheelDownAlt") )
                    {
                        frameTooltipScrollSetpoint += 24.0;
                    }
                }
                if ( Input::inputs[player].binaryToggle("MenuScrollUp") )
                {
                    frameTooltipScrollSetpoint -= 8.0 * fpsScale;
                }
                else if ( Input::inputs[player].binaryToggle("MenuScrollDown") )
                {
                    frameTooltipScrollSetpoint += 8.0 * fpsScale;
                }
            }
            frameTooltipScrollSetpoint = std::max(0.0, frameTooltipScrollSetpoint);
            if ( frameTooltipScrollSetpoint > scrollOverflow )
            {
                frameTooltipScrollSetpoint = scrollOverflow;
            }
            else
            {
                if ( frameTooltipScrollSetpoint == scrollOverflow )
                {
                    tooltipDisplayedSettings.scrolledToMax = scrollOverflow;
                }
                if ( tooltipDisplayedSettings.scrolledToMax != 0
                    && tooltipDisplayedSettings.scrolledToMax != scrollOverflow
                    && frameTooltipScrollSetpoint != scrollOverflow )
                {
                    frameTooltipScrollSetpoint = scrollOverflow; // snap to new value (moving between tooltips)
                    frameTooltipScrollAnim = frameTooltipScrollSetpoint;
                }
                if ( frameTooltipScrollSetpoint < scrollOverflow )
                {
                    tooltipDisplayedSettings.scrolledToMax = 0;
                }
            }
            if ( frameTooltipScrollAnim > scrollOverflow )
            {
                frameTooltipScrollAnim = scrollOverflow;
            }
            
            SDL_Rect actualSize = frameAttr->getActualSize();
            actualSize.h = std::max(frameAttr->getSize().h * 2, tooltipDisplayedSettings.tooltipDescriptionHeight);
            actualSize.y = frameTooltipScrollAnim;
            frameAttr->setActualSize(actualSize);
            frameAttr->setAllowScrollBinds(false);
            frameAttr->setScrollBarsEnabled(false);
            
            if ( abs(frameTooltipScrollSetpoint - frameTooltipScrollAnim) > 0.00001 )
            {
                const real_t fpsScale = getFPSScale(60.0);
                real_t setpointDiff = 0.0;
                if ( frameTooltipScrollSetpoint - frameTooltipScrollAnim > 0.0 )
                {
                    setpointDiff = fpsScale * std::max(3.0, (frameTooltipScrollSetpoint - frameTooltipScrollAnim)) / 3.0;
                }
                else
                {
                    setpointDiff = fpsScale * std::min(-3.0, (frameTooltipScrollSetpoint - frameTooltipScrollAnim)) / 3.0;
                }
                frameTooltipScrollAnim += setpointDiff;
                if ( setpointDiff > 0.0 )
                {
                    frameTooltipScrollAnim = std::min(frameTooltipScrollSetpoint, frameTooltipScrollAnim);
                }
                else
                {
                    frameTooltipScrollAnim = std::max(frameTooltipScrollSetpoint, frameTooltipScrollAnim);
                }
            }
            else
            {
                frameTooltipScrollAnim = frameTooltipScrollSetpoint;
            }
            
            const int scrolledPastAttrFrame = frameAttr->getSize().h - frameAttr->getActualSize().y;
            frameDescPos.y = std::max(frameAttr->getSize().y, frameAttr->getSize().y + frameAttr->getSize().h - frameAttr->getActualSize().y);
            frameDescPos.h += std::min(frameAttr->getActualSize().y, frameAttr->getSize().h);
            if ( scrolledPastAttrFrame < 0 )
            {
                SDL_Rect actualSize = frameDesc->getActualSize();
                actualSize.y = -scrolledPastAttrFrame;
                actualSize.h = tooltipDisplayedSettings.tooltipDescriptionHeight * 2;
                frameDesc->setActualSize(actualSize);
                frameDesc->setAllowScrollBinds(false);
                frameDesc->setScrollBarsEnabled(false);
            }
            else
            {
                SDL_Rect actualSize = frameDesc->getActualSize();
                actualSize.y = 0;
                frameDesc->setActualSize(actualSize);
                frameDesc->setAllowScrollBinds(false);
                frameDesc->setScrollBarsEnabled(false);
            }
            frameDesc->setSize(frameDescPos);
        }
        else
        {
            SDL_Rect frameAttrActualSize = frameAttr->getActualSize();
            frameAttrActualSize.y = 0;
            frameAttrActualSize.h = frameAttr->getSize().h * 2;
            frameAttr->setActualSize(frameAttrActualSize);
            frameAttr->setAllowScrollBinds(false);
            frameAttr->setScrollBarsEnabled(false);
            
            heightDiff += std::min(frameAttrActualSize.y, frameAttr->getSize().h);
            
            SDL_Rect actualSize = frameDesc->getActualSize();
            actualSize.y = 0;
            frameDesc->setActualSize(actualSize);
            frameDesc->setAllowScrollBinds(false);
            frameDesc->setScrollBarsEnabled(false);
            
            frameDescPos.y = std::max(frameAttr->getSize().y, frameAttr->getSize().y + frameAttr->getSize().h - frameAttrActualSize.y);
            frameDesc->setSize(frameDescPos);
            
            frameTooltipScrollSetpoint = 0.0;
            frameTooltipScrollAnim = 0.0;
        }
        
        
        SDL_Rect frameValuesPos = frameValues->getSize();
        if ( !frameDesc->isDisabled() )
        {
            frameValuesPos.y = frameDescPos.y + frameDescPos.h;
            frameValues->setSize(frameValuesPos);
        }
        
        SDL_Rect framePromptPos = framePrompt->getSize();
        framePromptPos.y = frameValuesPos.y + frameValuesPos.h;
        framePrompt->setSize(framePromptPos);
        
        bool doAppraisalPrompt = !item->identified && isItemFromInventory && appraisal.current_item != item->uid && !compendiumTooltip;
        bool doAppraisalProgressPrompt = !item->identified && isItemFromInventory && appraisal.current_item == item->uid && !compendiumTooltip;
        if ( doAppraisalProgressPrompt )
        {
            real_t percent = (((double)(appraisal.timermax - appraisal.timer)) / ((double)appraisal.timermax)) * 100;
            char buf[32];
            snprintf(buf, sizeof(buf), Language::get(4198), percent);
            txtPrompt->setText(buf);
        }
        else if ( doAppraisalPrompt )
        {
            txtPrompt->setText(Language::get(4102));
        }
        else if ( !tooltipDisplayedSettings.expanded )
        {
            if ( itemCategory(item) == SPELL_CAT )
            {
                txtPrompt->setText(Language::get(4103)); // show spell details
            }
            else
            {
                txtPrompt->setText(Language::get(4086)); // show item details
            }
        }
        else
        {
            if ( itemCategory(item) == SPELL_CAT )
            {
                txtPrompt->setText(Language::get(4104)); // hide spell details
            }
            else
            {
                txtPrompt->setText(Language::get(4087)); // hide item details
            }
        }
        
        // get left anchor for tooltip - if we want moving x tooltips, otherwise currently anchor to right of inventory panel
        //auto inventoryBgFrame = frameInventory->findFrame("inventory base");
        //const int tooltipPosX = frameInventory->getSize().x + inventoryBgFrame->getSize().x + inventoryBgFrame->getSize().w + 8;
        SDL_Rect newPos { x, y, tooltipDisplayedSettings.tooltipWidth, tooltipDisplayedSettings.tooltipHeight - heightDiff };
        if ( justify == Player::PANEL_JUSTIFY_RIGHT )
        {
            newPos.x -= newPos.w;
        }
        frameMain->setSize(newPos);
        
        // prompt for expanding tooltip position
        auto promptImg = frameMain->findImage("inventory mouse tooltip prompt img");
        promptImg->disabled = true;
        if ( (!players[player]->shootmode || compendiumTooltip)
            && !txtPrompt->isDisabled()
            && !doAppraisalProgressPrompt
            && (!doAppraisalPrompt || (doAppraisalPrompt && stats[player]->HP > 0)) )
        {
            auto binding = Input::inputs[player].input("Expand Inventory Tooltip");
            bool pressedGlyph = false;
            if ( binding.isBindingUsingGamepad()
                && (binding.padButton == SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSTICK
                    || binding.padButton == SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSTICK) )
            {
                pressedGlyph = true;
            }
            promptImg->path = Input::inputs[player].getGlyphPathForBinding(binding, pressedGlyph);
            if ( auto imgGet = Image::get(promptImg->path.c_str()) )
            {
                unsigned int largestTextWidth = 0;
                std::vector<int> languageIndexes;
                if ( doAppraisalPrompt )
                {
                    languageIndexes.push_back(4102);
                }
                else if ( itemCategory(item) == SPELL_CAT )
                {
                    languageIndexes.push_back(4103);
                    languageIndexes.push_back(4104);
                }
                else
                {
                    languageIndexes.push_back(4086);
                    languageIndexes.push_back(4087);
                }
                
                for ( auto index : languageIndexes )
                {
                    if ( auto textGet = Text::get(Language::get(index), txtPrompt->getFont(),
                                                  txtPrompt->getTextColor(), txtPrompt->getOutlineColor()) )
                    {
                        largestTextWidth = std::max(textGet->getWidth(), largestTextWidth);
                    }
                }
                promptImg->disabled = false;
                promptImg->pos.w = imgGet->getWidth();
                promptImg->pos.h = imgGet->getHeight();
                promptImg->pos.x = framePromptPos.x + framePromptPos.w - 4 - largestTextWidth - promptImg->pos.w;
                promptImg->pos.y = framePromptPos.y;
                if ( promptImg->pos.y + promptImg->pos.h > frameMain->getSize().h )
                {
                    promptImg->pos.y -= (promptImg->pos.y + promptImg->pos.h) - frameMain->getSize().h;
                }
            }
            if ( doAppraisalPrompt && expandBindingPressed )
            {
                players[player]->inventoryUI.activateItemContextMenuOption(item, ItemContextMenuPrompts::PROMPT_APPRAISE);
            }
        }
    }
    
    // position the background elements.
    {
        imgTopBackgroundLeft->pos.x = 0;
        imgTopBackgroundLeft->pos.y = 0;
        imgTopBackgroundRight->pos.x = frameMain->getSize().w - imgTopBackgroundRight->pos.w;
        imgTopBackgroundRight->pos.y = 0;
        
        imgTopBackground->pos.x = imgTopBackgroundLeft->pos.x + imgTopBackgroundLeft->pos.w;
        imgTopBackground->pos.y = 0;
        imgTopBackground->pos.w = imgTopBackgroundRight->pos.x - (imgTopBackground->pos.x);
        
        imgMiddleBackgroundLeft->pos.x = 0;
        imgMiddleBackgroundLeft->pos.y = imgTopBackgroundLeft->pos.h;
        imgMiddleBackgroundRight->pos.x = frameMain->getSize().w - imgTopBackgroundRight->pos.w;
        imgMiddleBackgroundRight->pos.y = imgMiddleBackgroundLeft->pos.y;
        
        imgMiddleBackground->pos.x = imgMiddleBackgroundLeft->pos.x + imgMiddleBackgroundLeft->pos.w;
        imgMiddleBackground->pos.y = imgMiddleBackgroundLeft->pos.y;
        imgMiddleBackground->pos.w = imgMiddleBackgroundRight->pos.x - (imgMiddleBackground->pos.x);
        
        imgBottomBackgroundLeft->pos.x = 0;
        imgBottomBackgroundLeft->pos.y = frameMain->getSize().h - imgBottomBackgroundLeft->pos.h;
        imgBottomBackgroundRight->pos.x = frameMain->getSize().w - imgBottomBackgroundRight->pos.w;
        imgBottomBackgroundRight->pos.y = frameMain->getSize().h - imgBottomBackgroundLeft->pos.h;
        
        imgBottomBackground->pos.x = imgBottomBackgroundLeft->pos.x + imgBottomBackgroundLeft->pos.w;
        imgBottomBackground->pos.y = imgBottomBackgroundLeft->pos.y;
        imgBottomBackground->pos.w = imgBottomBackgroundRight->pos.x - (imgBottomBackground->pos.x);
        
        // resize the middle section height
        imgMiddleBackgroundLeft->pos.h = imgBottomBackground->pos.y - (imgTopBackground->pos.y + imgTopBackground->pos.h);
        imgMiddleBackgroundRight->pos.h = imgMiddleBackgroundLeft->pos.h;
        imgMiddleBackground->pos.h = imgMiddleBackgroundLeft->pos.h;
        
        if ( tooltipDisplayedSettings.expanded && tooltipDisplayedSettings.expandCurrent > 0.1 )
        {
            if ( scrollOverflow > 0 )
            {
                if ( tooltipDisplayedSettings.expandCurrent >= .95 ) // already open, but gradient disabled from previous item
                {
                    if ( frameTooltipScrollSetpoint < scrollOverflow && !wasScrollable )
                    {
                        if ( gradientBottom->disabled ) { gradientBottom->color = makeColor(255, 255, 255, 255); }
                    }
                    if ( frameTooltipScrollSetpoint > 0 && !wasScrollable )
                    {
                        if ( gradientTop->disabled ) { gradientTop->color = makeColor(255, 255, 255, 255); }
                    }
                }
                
                gradientBottom->disabled = false;
                gradientTop->disabled = false;
                
                real_t fpsScale = getFPSScale(144.0);
                if ( frameTooltipScrollSetpoint < scrollOverflow )
                {
                    Uint8 r, g, b, a;
                    getColor(gradientBottom->color, &r, &g, &b, &a);
                    int newAlpha = a;
                    newAlpha += fpsScale * (10); // constant speed
                    newAlpha = std::min(newAlpha, 255);
                    a = newAlpha;
                    gradientBottom->color = makeColor(r, g, b, a);
                }
                else
                {
                    Uint8 r, g, b, a;
                    getColor(gradientBottom->color, &r, &g, &b, &a);
                    int newAlpha = a;
                    newAlpha -= fpsScale * (10); // constant speed
                    newAlpha = std::max(newAlpha, 0);
                    a = newAlpha;
                    gradientBottom->color = makeColor(r, g, b, a);
                    if ( a == 0 )
                    {
                        gradientBottom->disabled = true;
                    }
                }
                
                if ( frameTooltipScrollSetpoint > 0 )
                {
                    Uint8 r, g, b, a;
                    getColor(gradientTop->color, &r, &g, &b, &a);
                    int newAlpha = a;
                    newAlpha += fpsScale * (10); // constant speed
                    newAlpha = std::min(newAlpha, 255);
                    a = newAlpha;
                    gradientTop->color = makeColor(r, g, b, a);
                }
                else
                {
                    Uint8 r, g, b, a;
                    getColor(gradientTop->color, &r, &g, &b, &a);
                    int newAlpha = a;
                    newAlpha -= fpsScale * (10); // constant speed
                    newAlpha = std::max(newAlpha, 0);
                    a = newAlpha;
                    gradientTop->color = makeColor(r, g, b, a);
                    
                    if ( a == 0 )
                    {
                        gradientTop->disabled = true;
                    }
                }
            }
            else
            {
                gradientTop->disabled = true;
                gradientBottom->disabled = true;
            }
        }
        else
        {
            if ( scrollOverflow > 0 )
            {
                real_t fpsScale = getFPSScale(144.0);
                {
                    Uint8 r, g, b, a;
                    getColor(gradientTop->color, &r, &g, &b, &a);
                    int newAlpha = a;
                    newAlpha -= fpsScale * (10); // constant speed
                    newAlpha = std::max(newAlpha, 0);
                    a = newAlpha;
                    gradientTop->color = makeColor(r, g, b, a);
                    
                    if ( a == 0 )
                    {
                        gradientTop->disabled = true;
                    }
                }
                {
                    Uint8 r, g, b, a;
                    getColor(gradientBottom->color, &r, &g, &b, &a);
                    int newAlpha = a;
                    newAlpha -= fpsScale * (10); // constant speed
                    newAlpha = std::max(newAlpha, 0);
                    a = newAlpha;
                    gradientBottom->color = makeColor(r, g, b, a);
                    if ( a == 0 )
                    {
                        gradientBottom->disabled = true;
                    }
                }
            }
            else
            {
                gradientTop->disabled = true;
                gradientBottom->disabled = true;
            }
        }
        if ( gradientBottom->disabled ) { gradientBottom->color = makeColor(255, 255, 255, 0); }
        if ( gradientTop->disabled ) { gradientTop->color = makeColor(255, 255, 255, 0); }
        
        gradientBottom->pos.h = 42;
        gradientBottom->pos.x = imgBottomBackground->pos.x - 10;
        gradientBottom->pos.y = frameValues->getSize().y - gradientBottom->pos.h;
        gradientBottom->pos.w = imgBottomBackground->pos.w + 20;
        
        gradientTop->pos.h = 42;
        gradientTop->pos.x = imgMiddleBackground->pos.x - 10;
        gradientTop->pos.y = imgMiddleBackground->pos.y;
        gradientTop->pos.w = imgMiddleBackground->pos.w + 20;
    }
    
    finalizeFrameTooltip(item, x, y, justify, parentFrame);
}

void Player::HUD_t::finalizeFrameTooltip(Item* item, const int x, const int y, int justify, Frame* parentFrame)
{
    const int player = this->player.playernum;
    
	Frame* tooltipContainerFrame = nullptr;
	Frame* frameMain = nullptr;
	Frame* frameInventory = nullptr;
	Frame* frameInteract = nullptr;
	Frame* frameTooltipPrompt = nullptr;
	Frame* titleOnlyFrame = nullptr;
	bool compendiumTooltip = false;
	if ( parentFrame != nullptr )
	{
		// hacks for compendium tooltips
		char name[32];
		snprintf(name, sizeof(name), "player tooltip container %d", 0);
		tooltipContainerFrame = parentFrame->findFrame(name);
		snprintf(name, sizeof(name), "player title only tooltip %d", 0);
		titleOnlyFrame = tooltipContainerFrame->findFrame(name);
		snprintf(name, sizeof(name), "player tooltip %d", 0);
		frameMain = tooltipContainerFrame->findFrame(name);
		snprintf(name, sizeof(name), "player interact %d", 0);
		frameInteract = parentFrame->findFrame(name);
		snprintf(name, sizeof(name), "player item prompt %d", 0);
		frameTooltipPrompt = tooltipContainerFrame->findFrame(name);

		if ( !strcmp(parentFrame->getName(), "compendium") )
		{
			compendiumTooltip = true;
		}
	}
	else
	{
		tooltipContainerFrame = this->player.inventoryUI.tooltipContainerFrame;
		frameMain = this->player.inventoryUI.tooltipFrame;
		frameInventory = this->player.inventoryUI.frame;
		frameInteract = this->player.inventoryUI.interactFrame;
		frameTooltipPrompt = this->player.inventoryUI.tooltipPromptFrame;
		titleOnlyFrame = this->player.inventoryUI.titleOnlyTooltipFrame;
		if ( !frameInventory )
		{
			return;
		}
	}

	auto& tooltipDisplayedSettings = compendiumTooltip ? this->player.inventoryUI.compendiumItemTooltipDisplay
		: this->player.inventoryUI.itemTooltipDisplay;

    const bool doTitleOnlyTooltip = players[player]->shootmode && !compendiumTooltip && !(enableDebugKeys && keystatus[SDLK_g]);
	if ( !doTitleOnlyTooltip )
	{
		tooltipDisplayedSettings.opacitySetpoint = 0;
		tooltipDisplayedSettings.opacityAnimate = 1.0;
		frameMain->setDisabled(false);
		frameMain->setOpacity(100.0);
	}
	else
	{
		tooltipDisplayedSettings.opacitySetpoint = 0;
		tooltipDisplayedSettings.opacityAnimate = 0.0;
		frameMain->setDisabled(true);
		frameMain->setOpacity(0.0);
	}

	//inputs.getUIInteraction(player)->itemMenuItem = item->uid;
	// quick prompt stuff here.
	bool& itemMenuOpen = inputs.getUIInteraction(player)->itemMenuOpen;
	Item*& selectedItem = inputs.getUIInteraction(player)->selectedItem;
	frameTooltipPrompt->setDisabled(true);
	if ( !*cvar_hideGlyphs
		&& !doTitleOnlyTooltip
		&& !players[player]->GUI.isDropdownActive()
		&& !selectedItem
		&& !inputs.getVirtualMouse(player)->draw_cursor
		&& !compendiumTooltip
		&& !players[player]->shootmode
		&& !(GenericGUI->assistShrineGUI.bOpen && GenericGUI->assistShrineGUI.itemIsFromGUI(item))
		&& !(itemCategory(item) == SPELL_CAT && (players[player]->shopGUI.bOpen || players[player]->inventoryUI.chestGUI.bOpen)) )
	{
		auto options = getContextTooltipOptionsForItem(player, item, players[player]->inventoryUI.useItemDropdownOnGamepad, players[player]->GUI.activeModule == Player::GUI_t::MODULE_HOTBAR);

		std::sort(options.begin(), options.end(), [player](const ItemContextMenuPrompts& lhs, const ItemContextMenuPrompts& rhs) {
			return getContextMenuOptionOrder(player, lhs) < getContextMenuOptionOrder(player, rhs);
		});

		if ( !options.empty() )
		{
			frameTooltipPrompt->setDisabled(false);
			frameTooltipPrompt->setOpacity(frameMain->getOpacity());
			std::vector<std::pair<Frame::image_t*, Field*>> promptFrames;
			
			// disable all the text
			for ( int index = 1; index < 5; ++index )
			{
				char imgName[16];
				char fieldName[16];
				snprintf(imgName, sizeof(imgName), "glyph %d", index);
				snprintf(fieldName, sizeof(imgName), "txt %d", index);
				auto img = frameTooltipPrompt->findImage(imgName);
				auto txt = frameTooltipPrompt->findField(fieldName);
				if ( img && txt )
				{
					if ( players[player]->inventoryUI.useItemDropdownOnGamepad == Player::Inventory_t::GAMEPAD_DROPDOWN_COMPACT )
					{
						img->disabled = true;
					}
					else
					{
						Uint8 r, g, b, a;
						getColor(img->color, &r, &g, &b, &a);
						a = 128;
						img->disabled = false; // unused glyphs will show as partially opaque
						img->color = makeColor( r, g, b, a);
					}
					txt->setDisabled(true);
					promptFrames.push_back(std::make_pair(img, txt));
				}
			}

			std::set<int> promptsAvailable;

			// set the text and button prompts from the available options
			bool dropEnabled = false;
			for ( auto& option : options )
			{
				char imgName[16];
				char fieldName[16];
				const int order = getContextMenuOptionOrder(player, option);
				promptsAvailable.insert(order);

				snprintf(imgName, sizeof(imgName), "glyph %d", order);
				snprintf(fieldName, sizeof(imgName), "txt %d", order);
				auto img = frameTooltipPrompt->findImage(imgName);
				auto txt = frameTooltipPrompt->findField(fieldName);
				img->disabled = false;
				img->color = 0xFFFFFFFF;
				if ( players[player]->inventoryUI.useItemDropdownOnGamepad == Player::Inventory_t::GAMEPAD_DROPDOWN_COMPACT )
				{
					if ( option == ItemContextMenuPrompts::PROMPT_DROP || option == ItemContextMenuPrompts::PROMPT_GRAB )
					{
						if ( option == ItemContextMenuPrompts::PROMPT_DROP )
						{
							dropEnabled = true;
						}
						txt->setDisabled(true);
						img->path = Input::inputs[player].getGlyphPathForBinding(getContextMenuOptionBindingName(player, option).c_str());
						continue;
					}
				}

				img->path = Input::inputs[player].getGlyphPathForBinding(getContextMenuOptionBindingName(player, option).c_str());
				txt->setDisabled(false);
				txt->setText(getContextMenuLangEntry(player, option, *item));
			}

			int alignx1 = xres;

			if ( players[player]->inventoryUI.useItemDropdownOnGamepad == Player::Inventory_t::GAMEPAD_DROPDOWN_COMPACT )
			{
				auto& useGlyph = promptFrames[3].first;
				auto& useTxt = promptFrames[3].second;

				useGlyph->pos = promptFrames[2].first->pos;
				useTxt->setSize(promptFrames[2].second->getSize());
				alignx1 = useGlyph->pos.x;
				auto dropIcon = frameTooltipPrompt->findImage("drop icon");
				dropIcon->disabled = true;
				if ( dropEnabled )
				{
					dropIcon->disabled = false;
					if ( auto imgGet = Image::get(dropIcon->path.c_str()) )
					{
						dropIcon->pos.w = imgGet->getWidth();
						dropIcon->pos.h = imgGet->getWidth();
						dropIcon->pos.x = alignx1 - dropIcon->pos.w - 16;
						dropIcon->pos.y = useGlyph->pos.y + useGlyph->pos.h / 2 - dropIcon->pos.h / 2 - 2;
						if ( dropIcon->pos.y % 2 == 1 )
						{
							++dropIcon->pos.y;
						}

						auto& dropGlyph = promptFrames[0].first;
						dropGlyph->pos = useGlyph->pos;
						dropGlyph->pos.x = dropIcon->pos.x - dropGlyph->pos.w - 4;
						alignx1 = std::min(alignx1, dropGlyph->pos.x);
					}
				}

				auto grabIcon = frameTooltipPrompt->findImage("grab icon");
				grabIcon->disabled = false;
				if ( auto imgGet = Image::get(grabIcon->path.c_str()) )
				{
					grabIcon->pos.w = imgGet->getWidth();
					grabIcon->pos.h = imgGet->getWidth();
					if ( dropEnabled )
					{
						grabIcon->pos.x = alignx1 - grabIcon->pos.w - 6;
					}
					else
					{
						grabIcon->pos.x = alignx1 - grabIcon->pos.w - 16;
					}
					grabIcon->pos.y = useGlyph->pos.y + useGlyph->pos.h / 2 - grabIcon->pos.h / 2 - 2;
					if ( grabIcon->pos.y % 2 == 1 )
					{
						++grabIcon->pos.y;
					}

					auto& grabGlyph = promptFrames[1].first;
					grabGlyph->pos = useGlyph->pos;
					grabGlyph->pos.x = grabIcon->pos.x - grabGlyph->pos.w - 4;
					alignx1 = std::min(alignx1, grabGlyph->pos.x);
				}
			}
			else
			{
				int index = 0;
				for ( auto& pair : promptFrames )
				{
					++index;
					auto& img = pair.first;
					auto& txt = pair.second;
					if ( img->path == "" )
					{
						img->path = Input::inputs[player].getGlyphPathForBinding(getBindingNameForMissingTooltipPrompts(index).c_str());
					}

					if ( !txt->isDisabled() )
					{
						if ( auto textGet = Text::get(txt->getText(), txt->getFont(),
							txt->getTextColor(), txt->getOutlineColor()) )
						{
							if ( txt->getHJustify() == Field::justify_t::RIGHT )
							{
								// find the furthest left text
								const unsigned int textWidth = textGet->getWidth();
								alignx1 = std::min(txt->getSize().x + txt->getSize().w - (int)textWidth, alignx1);
							}
						}
					}
				}
			}

			// shift everything by the further left text as anchor
			for ( auto& pair : promptFrames )
			{
				auto& img = pair.first;
				auto& txt = pair.second;

				int offset = -(alignx1 - 8);
				img->pos.x += offset;
				SDL_Rect txtSize = txt->getSize();
				txtSize.x += offset;
				txt->setSize(txtSize);
			}
			if ( players[player]->inventoryUI.useItemDropdownOnGamepad == Player::Inventory_t::GAMEPAD_DROPDOWN_COMPACT )
			{
				auto dropIcon = frameTooltipPrompt->findImage("drop icon");
				auto grabIcon = frameTooltipPrompt->findImage("grab icon");
				int offset = -(alignx1 - 8);
				dropIcon->pos.x += offset;
				grabIcon->pos.x += offset;
			}

			// find the furthest right text for the width of the frame
			int alignx2 = 0;
			for ( auto& pair : promptFrames )
			{
				auto& img = pair.first;
				auto& txt = pair.second;
				if ( !txt->isDisabled() )
				{
					if ( auto textGet = Text::get(txt->getText(), txt->getFont(),
						txt->getTextColor(), txt->getOutlineColor()) )
					{
						if ( txt->getHJustify() == Field::justify_t::LEFT )
						{
							const unsigned int textWidth = textGet->getWidth();
							alignx2 = std::max(txt->getSize().x + (int)textWidth, alignx2);
							SDL_Rect txtSize = txt->getSize();
							txtSize.w = textWidth;
							txt->setSize(txtSize);
						}
					}
				}
			}

			SDL_Rect promptPos = frameTooltipPrompt->getSize();
			const int width = (alignx2 - 4); // 4px padding
			{
				auto middleCenter = frameTooltipPrompt->findImage("interact middle background");
				middleCenter->pos.w = width;
				auto middleTop = frameTooltipPrompt->findImage("interact middle top background");
				middleTop->pos.w = width;
				auto middleBottom = frameTooltipPrompt->findImage("interact middle bottom background");
				middleBottom->pos.w = width;
				auto middleLeft = frameTooltipPrompt->findImage("interact middle left");
				auto middleRight = frameTooltipPrompt->findImage("interact middle right");
				middleRight->pos.x = middleLeft->pos.x + middleLeft->pos.w + width;

				auto bottomCenter = frameTooltipPrompt->findImage("interact bottom background");
				bottomCenter->pos.w = width + 4;
				auto bottomLeft = frameTooltipPrompt->findImage("interact bottom left");
				auto bottomRight = frameTooltipPrompt->findImage("interact bottom right");
				bottomRight->pos.x = bottomCenter->pos.x + bottomCenter->pos.w;

				promptPos.w = middleRight->pos.x + middleRight->pos.w;

				if ( players[player]->inventoryUI.useItemDropdownOnGamepad == Player::Inventory_t::GAMEPAD_DROPDOWN_COMPACT )
				{
					auto& useGlyph = promptFrames[2].first;
					bottomCenter->pos.y = useGlyph->pos.y + useGlyph->pos.h + 2;
					bottomLeft->pos.y = bottomCenter->pos.y;
					bottomRight->pos.y = bottomCenter->pos.y;

					promptPos.h = bottomCenter->pos.y + bottomCenter->pos.h;

					middleCenter->pos.h = bottomCenter->pos.y - 4;
					middleLeft->pos.h = bottomCenter->pos.y;
					middleRight->pos.h = bottomCenter->pos.y;
					middleBottom->pos.y = bottomCenter->pos.y - 2;
				}
			}

			promptPos.x = frameMain->getSize().x + frameMain->getSize().w - 6 - promptPos.w;
			promptPos.y = frameMain->getSize().y + frameMain->getSize().h - 2;
			frameTooltipPrompt->setSize(promptPos);
		}
	}

	if ( doTitleOnlyTooltip )
	{
		tooltipDisplayedSettings.titleOnlyOpacitySetpoint = 0;
		tooltipDisplayedSettings.titleOnlyOpacityAnimate = 1.0;
		titleOnlyFrame->setDisabled(false);
		titleOnlyFrame->setOpacity(100.0);
	}
	else
	{
		static ConsoleVariable<int> cvar_item_tooltip_lowest_y("/item_tooltip_lowest_y", 90);
		static ConsoleVariable<int> cvar_item_tooltip_lowest_y_compact("/item_tooltip_lowest_y_compact", 0);
		SDL_Rect mainPos = frameMain->getSize();
		int totalHeight = mainPos.y + mainPos.h;
		if ( !frameTooltipPrompt->isDisabled() )
		{
			totalHeight += frameTooltipPrompt->getSize().h - 2;
		}
		int lowestY = 0;
		if ( compendiumTooltip )
		{
			lowestY = tooltipContainerFrame->getSize().h - *cvar_item_tooltip_lowest_y - 38;
		}
		else
		{
			lowestY = players[player]->camera_virtualHeight() -
				(players[player]->bUseCompactGUIHeight() ? *cvar_item_tooltip_lowest_y_compact : *cvar_item_tooltip_lowest_y);
		}
		if ( totalHeight > lowestY )
		{
			mainPos.y -= (totalHeight - lowestY);
			SDL_Rect promptPos = frameTooltipPrompt->getSize();
			promptPos.y -= (totalHeight - lowestY);
			frameTooltipPrompt->setSize(promptPos);
			frameMain->setSize(mainPos);
		}
	}
}

void Player::Inventory_t::setCompactView(bool bCompact)
{
	bCompactView = bCompact;

	if ( bCompactView )
	{
		bool invertJustification = false;
		if ( !player.bUseCompactGUIHeight()
			&& !player.bUseCompactGUIWidth() )
		{
			// fullscreen
			invertJustification = false;
		}
		else if ( !player.bUseCompactGUIWidth() )
		{
			// widescreen
			invertJustification = false;
		}
		else if ( !player.bUseCompactGUIHeight() )
		{
			// tallscreen
			invertJustification = false;
		}
		else if ( player.bUseCompactGUIHeight() && player.bUseCompactGUIWidth() )
		{
			// quadrants
			invertJustification = false;
			if ( player.camera_virtualx1() > 0 ) // right-side
			{
				invertJustification = true;
			}
		}
		if ( keystatus[SDLK_t] && enableDebugKeys )
		{
			invertJustification = !invertJustification;
		}
		inventoryPanelJustify = (invertJustification == false) ? PANEL_JUSTIFY_LEFT : PANEL_JUSTIFY_RIGHT;
		paperDollPanelJustify = (invertJustification == false) ? PANEL_JUSTIFY_RIGHT : PANEL_JUSTIFY_LEFT;
	}
	else
	{
		inventoryPanelJustify = PANEL_JUSTIFY_LEFT;
		paperDollPanelJustify = PANEL_JUSTIFY_LEFT;
	}
}

int Player::Inventory_t::slideOutWidth = 100;
void Player::Inventory_t::resizeAndPositionInventoryElements()
{
	if ( !frame ) { return; }
	auto inventoryBaseImagesFrame = playerInventoryFrames[player.playernum].inventoryBaseImagesFrame;
	auto invSlotsFrame = playerInventoryFrames[player.playernum].invSlotsFrame;
	auto dollSlotsFrame = playerInventoryFrames[player.playernum].dollSlotsFrame;
	auto backpackSlotsFrame = playerInventoryFrames[player.playernum].backpackSlotsFrame;
	SDL_Rect invSlotsPos = invSlotsFrame->getSize();
	SDL_Rect dollSlotsPos = dollSlotsFrame->getSize();
	SDL_Rect backpackSlotsPos = backpackSlotsFrame->getSize();

	auto defaultInvImg = playerInventoryFrames[player.playernum].defaultInvImg;
	if ( !bCompactView && defaultInvImg->disabled )
	{
		if ( auto img = Image::get(defaultInvImg->path.c_str()) )
		{
			defaultInvImg->pos.w = img->getWidth();
			defaultInvImg->pos.h = img->getHeight();
		}
	}
	{
		defaultInvImg->pos.x = 0;
		defaultInvImg->pos.y = 0;
		defaultInvImg->disabled = bCompactView ? true : false;
	}

	if ( !bCompactView )
	{
		SDL_Rect pos = inventoryBaseImagesFrame->getSize();
		pos.w = defaultInvImg->pos.w;
		pos.h = frame->getSize().h;
		inventoryBaseImagesFrame->setSize(pos);
	}
	else
	{
		SDL_Rect pos = inventoryBaseImagesFrame->getSize();
		// consume the entire window to position things on both sides
		pos.w = frame->getSize().w;
		pos.h = frame->getSize().h;
		inventoryBaseImagesFrame->setSize(pos);
	}

	auto compactInvImg = playerInventoryFrames[player.playernum].compactInvImg;
	if ( bCompactView && compactInvImg->disabled )
	{
		if ( auto img = Image::get(compactInvImg->path.c_str()) )
		{
			compactInvImg->pos.w = img->getWidth();
			compactInvImg->pos.h = img->getHeight();
		}
	}
	{
		if ( inventoryPanelJustify == PANEL_JUSTIFY_RIGHT )
		{
			compactInvImg->pos.x = inventoryBaseImagesFrame->getSize().w - compactInvImg->pos.w;
		}
		else if ( inventoryPanelJustify == PANEL_JUSTIFY_LEFT )
		{
			compactInvImg->pos.x = 0;
		}
		compactInvImg->pos.y = 4;
		compactInvImg->disabled = bCompactView ? false : true;
	}
	auto compactCharImg = playerInventoryFrames[player.playernum].compactCharImg;
	if ( bCompactView && compactCharImg->disabled )
	{
		if ( auto img = Image::get(compactCharImg->path.c_str()) )
		{
			compactCharImg->pos.w = img->getWidth();
			compactCharImg->pos.h = img->getHeight();
		}
	}
	{
		if ( paperDollPanelJustify == PANEL_JUSTIFY_RIGHT )
		{
			compactCharImg->pos.x = inventoryBaseImagesFrame->getSize().w - compactCharImg->pos.w;
		}
		else if ( paperDollPanelJustify == PANEL_JUSTIFY_LEFT )
		{
			compactCharImg->pos.x = 0;
		}
		compactCharImg->pos.y = 0;
		compactCharImg->disabled = bCompactView ? false : true;
	}

	if ( !bCompactView )
	{
		invSlotsPos.x = 0;
		invSlotsPos.y = 202;
		invSlotsPos.w = defaultInvImg->pos.w;
		invSlotsPos.h = 242;

		dollSlotsPos.x = 0;
		dollSlotsPos.y = 0;
		dollSlotsPos.w = defaultInvImg->pos.w;
		dollSlotsPos.h = 202;

		backpackSlotsPos.x = invSlotsPos.x;
		backpackSlotsPos.y = invSlotsPos.y + invSlotsPos.h;
	}
	else
	{
		invSlotsPos.w = compactInvImg->pos.w;
		invSlotsPos.h = 242;
		invSlotsPos.x = (inventoryPanelJustify == PANEL_JUSTIFY_LEFT) ? 0 
			: inventoryBaseImagesFrame->getSize().w - invSlotsPos.w;
		invSlotsPos.y = 8;

		dollSlotsPos.w = defaultInvImg->pos.w;
		dollSlotsPos.h = 202;
		dollSlotsPos.x = (paperDollPanelJustify == PANEL_JUSTIFY_LEFT) ? 0 
			: inventoryBaseImagesFrame->getSize().w - dollSlotsPos.w;
		dollSlotsPos.y = 8;

		backpackSlotsPos.x = invSlotsPos.x;
		backpackSlotsPos.y = invSlotsPos.y + invSlotsPos.h;
	}

	if ( bCompactView 
		&& (player.bUseCompactGUIWidth() 
		&& (player.GUI.activeModule == Player::GUI_t::MODULE_HOTBAR
			|| player.GUI.activeModule == Player::GUI_t::MODULE_SHOP))
		|| (player.bUseCompactGUIHeight()
			&& (player.GUI.activeModule == Player::GUI_t::MODULE_STATUS_EFFECTS
				|| player.hud.statusFxFocusedWindowActive)) )
	{
		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
		real_t setpointDiff = fpsScale * std::max(.1, (1.0 - slideOutPercent)) / 2.0;
		slideOutPercent += setpointDiff;
		slideOutPercent = std::min(1.0, slideOutPercent);
	}
	else
	{
		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
		real_t setpointDiff = fpsScale * std::max(.1, (slideOutPercent)) / 2.0;
		slideOutPercent -= setpointDiff;
		slideOutPercent = std::max(0.0, slideOutPercent);
	}

	int hideFrameAmount = 0;
	if ( !bCompactView )
	{
		hideFrameAmount = slideOutWidth * slideOutPercent;
	}
	else
	{
		hideFrameAmount = slideOutWidth * slideOutPercent;
	}
	invSlotsPos.x -= ((inventoryPanelJustify == PANEL_JUSTIFY_LEFT) ? hideFrameAmount : -hideFrameAmount);
	if ( bCompactView )
	{
		int paperDollHideFrameAmount = hideFrameAmount;
		dollSlotsPos.x -= ((paperDollPanelJustify == PANEL_JUSTIFY_LEFT) ? paperDollHideFrameAmount : -paperDollHideFrameAmount);
		compactCharImg->pos.x -= ((paperDollPanelJustify == PANEL_JUSTIFY_LEFT) ? paperDollHideFrameAmount : -paperDollHideFrameAmount);
		if ( animPaperDollHide > 0.01 )
		{
			int chestHideAmount = animPaperDollHide * compactCharImg->pos.w;
			dollSlotsPos.x += ((paperDollPanelJustify == PANEL_JUSTIFY_LEFT) ? -chestHideAmount : chestHideAmount);
			compactCharImg->pos.x += ((paperDollPanelJustify == PANEL_JUSTIFY_LEFT) ? -chestHideAmount : chestHideAmount);
		}
	}
	else
	{
		dollSlotsPos.x -= ((paperDollPanelJustify == PANEL_JUSTIFY_LEFT) ? hideFrameAmount : -hideFrameAmount);
		compactCharImg->pos.x -= ((paperDollPanelJustify == PANEL_JUSTIFY_LEFT) ? hideFrameAmount : -hideFrameAmount);
	}
	backpackSlotsPos.x -= ((inventoryPanelJustify == PANEL_JUSTIFY_LEFT) ? hideFrameAmount : -hideFrameAmount);
	compactInvImg->pos.x -= ((inventoryPanelJustify == PANEL_JUSTIFY_LEFT) ? hideFrameAmount : -hideFrameAmount);
	defaultInvImg->pos.x -= ((inventoryPanelJustify == PANEL_JUSTIFY_LEFT) ? hideFrameAmount : -hideFrameAmount);

	invSlotsFrame->setSize(invSlotsPos);
	dollSlotsFrame->setSize(dollSlotsPos);
	backpackSlotsFrame->setSize(backpackSlotsPos);

	auto characterPreview = playerInventoryFrames[player.playernum].characterPreview;
	auto characterPreviewPos = characterPreview->getSize();
	characterPreviewPos.y = dollSlotsPos.y;
	characterPreviewPos.h = dollSlotsPos.h;
	characterPreviewPos.x = dollSlotsPos.x + getSlotSize() + 8;
	characterPreview->setSize(characterPreviewPos);

	auto flourishFrame = playerInventoryFrames[player.playernum].flourishFrame;
	auto flourishFramePos = flourishFrame->getSize();
	flourishFramePos.x = dollSlotsPos.x + dollSlotsPos.w / 2 - flourishFramePos.w / 2;
	flourishFramePos.y = dollSlotsPos.y + dollSlotsPos.h - flourishFramePos.h + 6;
	flourishFrame->setSize(flourishFramePos);

	auto backpackFrame = playerInventoryFrames[player.playernum].backpackFrame;
	backpackFrame->setDisabled(true);
	backpackSlotsFrame->setDisabled(true);
	if ( getSizeY() > DEFAULT_INVENTORY_SIZEY )
	{
		backpackFrame->setDisabled(false);
		backpackSlotsFrame->setDisabled(false);
	}
	auto backpackFramePos = backpackFrame->getSize();
	if ( bCompactView )
	{
		backpackFramePos.x = compactInvImg->pos.x;
		backpackFramePos.y = compactInvImg->pos.y + compactInvImg->pos.h - 6;
	}
	else
	{
		backpackFramePos.x = defaultInvImg->pos.x;
		backpackFramePos.y = defaultInvImg->pos.y + defaultInvImg->pos.h - 6;
	}
	backpackFrame->setSize(backpackFramePos);

	auto spellFramePos = spellFrame->getSize();
	spellFramePos.y = invSlotsFrame->getSize().y - 4;
	if ( !player.bUseCompactGUIHeight() )
	{
		spellFramePos.y -= spellPanel.heightOffsetWhenNotCompact;
	}
	spellFrame->setSize(spellFramePos);

	// mouse hovering over inventory frames in compact view - if we're on hotbar, refocus the inventory/spells panel
	if ( (player.GUI.activeModule == Player::GUI_t::MODULE_HOTBAR || player.GUI.activeModule == Player::GUI_t::MODULE_SHOP)
		&& inputs.bPlayerUsingKeyboardControl(player.playernum) && bCompactView
		&& cursor.queuedModule != Player::GUI_t::MODULE_HOTBAR )
	{
		if (player.GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_INVENTORY)
			&& !isInteractable )
		{
			if ( (!invSlotsFrame->isDisabled() && invSlotsFrame->capturesMouseInRealtimeCoords()) 
				|| (!dollSlotsFrame->isDisabled() && dollSlotsFrame->capturesMouseInRealtimeCoords())
				|| (!backpackSlotsFrame->isDisabled() && backpackSlotsFrame->capturesMouseInRealtimeCoords())
				|| (!spellFrame->isDisabled() && spellFrame->capturesMouseInRealtimeCoords()) )
			{
				if ( player.inventory_mode == INVENTORY_MODE_SPELL )
				{
					player.GUI.activateModule(Player::GUI_t::MODULE_SPELLS);
				}
				else
				{
					player.GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
				}
			}
		}
	}
}

bool Player::Inventory_t::bIsTooltipDelayed()
{
	 if ( tooltipDelayTick > 0 ) 
	 { 
		 if ( tooltipDelayTick > ticks )
		 {
			 return true;
		 }
		 return ((ticks - tooltipDelayTick) < (TICKS_PER_SECOND / 25)); 
	 }
	 return false;
}

void Player::Inventory_t::openInventory()
{
	bool wasDisabled = true;
	if ( frame )
	{
		wasDisabled = frame->isDisabled();
		frame->setDisabled(false);
	}
	if ( wasDisabled )
	{
		bFirstTimeSnapCursor = false;
		slideOutPercent = 1.0;
		isInteractable = false;

		// consume tooltip prompt buttons e.g so holding 'A' doesn't activate when opening inventory
		auto& input = Input::inputs[player.playernum];
		input.consumeBinaryToggle("MenuConfirm");
		input.consumeBinaryToggle("MenuCancel");
		input.consumeBinaryToggle("MenuAlt1");
		input.consumeBinaryToggle("MenuAlt2");
	}
	player.hotbar.hotbarTooltipLastGameTick = 0;
}
void Player::Inventory_t::closeInventory()
{
	if ( frame )
	{
		frame->setDisabled(true);
	}
	spellPanel.closeSpellPanel();
	chestGUI.closeChest();
	updateItemContextMenu(); // process + close the item context menu
	bFirstTimeSnapCursor = false;
	isInteractable = false;
	tooltipDelayTick = 0;
	itemTooltipDisplay.expanded = false;
	itemTooltipDisplay.expandSetpoint = 0;
	itemTooltipDisplay.expandCurrent = 0;
	itemTooltipDisplay.expandAnimate = 0;
	itemTooltipDisplay.frameTooltipScrollAnim = 0.0;
	itemTooltipDisplay.frameTooltipScrollSetpoint = 0.0;
	itemTooltipDisplay.frameTooltipScrollPrevSetpoint = 0.0;
	itemTooltipDisplay.scrolledToMax = 0;
}

bool Player::Inventory_t::guiAllowDefaultRightClick() const
{
	if ( player.GUI.bModuleAccessibleWithMouse(player.GUI.activeModule) )
	{
		if ( GenericGUI[player.playernum].isGUIOpen() )
		{
			return false;
		}
		return true;
	}
	return false;
}

bool Player::Inventory_t::guiAllowDropItems(Item* itemToDrop) const
{
	if ( player.GUI.bModuleAccessibleWithMouse(player.GUI.activeModule) )
	{
		if ( GenericGUI[player.playernum].alchemyGUI.bOpen )
		{
			if ( itemToDrop && GenericGUI[player.playernum].alembicItem == itemToDrop )
			{
				return false;
			}
			return true;
		}

		if ( player.shopGUI.bOpen 
			|| GenericGUI[player.playernum].isGUIOpen()
			|| player.GUI.isDropdownActive() || player.GUI.isGameoverActive()
			/*|| player.inventoryUI.chestGUI.bOpen*/
			|| !players[player.playernum]->bControlEnabled )
		{
			return false;
		}
		return true;
	}
	return false;
}

bool Player::Inventory_t::paperDollContextMenuActive()
{
	bool inventoryControlActive = player.bControlEnabled
		&& !gamePaused
		&& !player.usingCommand();
	return inventoryControlActive 
		&& !player.shootmode
		&& !inputs.getUIInteraction(player.playernum)->selectedItem
		&& !(player.GUI.isDropdownActive() && player.GUI.dropdownMenu.currentName.find("paper_doll") == std::string::npos)
		&& player.inventory_mode == INVENTORY_MODE_ITEM
		&& animPaperDollHide <= 0.001
		&& isInteractable
		&& slideOutPercent <= 0.001
		&& player.GUI.activeModule != Player::GUI_t::MODULE_PORTRAIT
		&& (inputs.getVirtualMouse(player.playernum)->draw_cursor
			|| (!inputs.getVirtualMouse(player.playernum)->draw_cursor 
				&& (player.GUI.activeModule == Player::GUI_t::MODULE_INVENTORY) ));
}

bool blitInventorySlotFramesToSurf = false;
void Player::Inventory_t::updateInventory()
{
	const int player = this->player.playernum;
	assert(frame);
	assert(tooltipFrame);

	auto& shopGUI = this->player.shopGUI;
	auto& tinkerGUI = GenericGUI[player].tinkerGUI;
	auto& alchemyGUI = GenericGUI[player].alchemyGUI;
	auto& featherGUI = GenericGUI[player].featherGUI;
	auto& itemfxGUI = GenericGUI[player].itemfxGUI;
	auto& assistShrineGUI = GenericGUI[player].assistShrineGUI;

	appraisal.updateAppraisalAnim();

	bool bCompactView = false;
	if ( (keystatus[SDLK_y] && enableDebugKeys) || players[player]->bUseCompactGUIHeight()
		|| players[player]->bUseCompactGUIWidth() )
	{
		bCompactView = true;
	}
	setCompactView(bCompactView);

	bool hideAndExit = false;
	if ( nohud || !players[player]->isLocalPlayer() || players[player]->shootmode
		|| !((players[player]->gui_mode == GUI_MODE_INVENTORY || players[player]->gui_mode == GUI_MODE_SHOP)) )
	{
		hideAndExit = true;
	}
	else if ( bCompactView && players[player]->hud.compactLayoutMode != Player::HUD_t::COMPACT_LAYOUT_INVENTORY )
	{
		hideAndExit = true;
	}

	if ( !this->player.bAlignGUINextToInventoryCompact() &&
		((!chestFrame->isDisabled() && chestGUI.bOpen)
		|| (GenericGUI[player].isGUIOpen() && !(GenericGUI[player].itemfxGUI.bOpen))
		|| shopGUI.bOpen) )
	{
		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
		real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animPaperDollHide)) / 2.0;
		animPaperDollHide += setpointDiffX;
		animPaperDollHide = std::min(1.0, animPaperDollHide);
	}
	else
	{
		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
		real_t setpointDiffX = fpsScale * std::max(.01, (animPaperDollHide)) / 2.0;
		animPaperDollHide -= setpointDiffX;
		animPaperDollHide = std::max(0.0, animPaperDollHide);
	}

	if ( hideAndExit )
	{
		closeInventory();

		// process the slide out animation for other panels (character sheet view)
		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
		real_t setpointDiff = fpsScale * std::max(.01, (slideOutPercent)) / 2.0;
		slideOutPercent -= setpointDiff;
		slideOutPercent = std::max(0.0, slideOutPercent);
		return;
	}

	//if ( keystatus[SDLK_g] )
	//{
	//	keystatus[SDLK_g] = 0;
	//	blitInventorySlotFramesToSurf = !blitInventorySlotFramesToSurf;
	//	if ( slotFrames[0] )
	//	{
	//		if ( !blitInventorySlotFramesToSurf )
	//		{
	//			if ( slotFrames[0]->findParentToBlitTo() )
	//			{
	//				auto parent = slotFrames[0]->getParent();
	//				parent->setBlitChildren(false);
	//			}
	//		}
	//		else if ( blitInventorySlotFramesToSurf )
	//		{
	//			auto parent = slotFrames[0]->getParent();
	//			parent->setBlitChildren(true);
	//		}
	//	}
	//}

	Item*& selectedItem = inputs.getUIInteraction(player)->selectedItem;
	openInventory();
	if ( slideOutPercent <= .0001 )
	{
		if ( !bFirstTimeSnapCursor )
		{
			bFirstTimeSnapCursor = true;
			if ( !inputs.getUIInteraction(player)->selectedItem
				&& players[player]->GUI.activeModule == Player::GUI_t::MODULE_INVENTORY )
			{
				//warpMouseToSelectedItem(nullptr, (Inputs::SET_CONTROLLER));
			}
		}
		if ( !isInteractable )
		{
			tooltipDelayTick = ticks + 4;
		}
		isInteractable = true;
	}
	else
	{
		isInteractable = false;
		tooltipDelayTick = ticks;
	}

	if ( players[player]->inventory_mode == INVENTORY_MODE_SPELL )
	{
		spellPanel.openSpellPanel();
	}
	else
	{
		spellPanel.closeSpellPanel();
	}
	spellPanel.updateSpellPanel();
	chestGUI.updateChest();
	if ( selectedItem )
	{
		if ( itemCategory(selectedItem) == SPELL_CAT && players[player]->inventory_mode != INVENTORY_MODE_SPELL )
		{
			cycleInventoryTab();
		}
		else if ( itemCategory(selectedItem) != SPELL_CAT && players[player]->inventory_mode == INVENTORY_MODE_SPELL )
		{
			cycleInventoryTab();
		}
	}

	{
		// resize/position elements based on compact view or not
		resizeAndPositionInventoryElements();
	}
	bool disableMouseDisablingHotbarFocus = false;
	SDL_Rect mode_pos{-100, -100, 0, 0};
	node_t* node, *nextnode;

	auto& hotbar_t = players[player]->hotbar;
	auto& hotbar = hotbar_t.slots();

	const Sint32 mousex = inputs.getMouse(player, Inputs::X);
	const Sint32 mousey = inputs.getMouse(player, Inputs::Y);
	const Sint32 omousex = inputs.getMouse(player, Inputs::OX);
	const Sint32 omousey = inputs.getMouse(player, Inputs::OY);

	//const int x = getStartX();
	//const int y = getStartY();

	const int inventorySlotSize = getSlotSize();

	bool& toggleclick = inputs.getUIInteraction(player)->toggleclick;
	bool& itemMenuOpen = inputs.getUIInteraction(player)->itemMenuOpen;
	Uint32& itemMenuItem = inputs.getUIInteraction(player)->itemMenuItem;
	int& itemMenuX = inputs.getUIInteraction(player)->itemMenuX;
	int& itemMenuY = inputs.getUIInteraction(player)->itemMenuY;
	int& itemMenuSelected = inputs.getUIInteraction(player)->itemMenuSelected;
	bool& itemMenuFromHotbar = inputs.getUIInteraction(player)->itemMenuFromHotbar;

	bool inventoryControlActive = players[player]->bControlEnabled
		&& !gamePaused
		&& !players[player]->usingCommand()
		&& !players[player]->GUI.isDropdownActive();

	if ( inputs.hasController(player) )
	{
		bool radialMenuOpen = FollowerMenu[player].followerMenuIsOpen() || CalloutMenu[player].calloutMenuIsOpen();
		if ( radialMenuOpen )
		{
			// do nothing?
		}
		else if ( inventoryControlActive
			&& players[player]->GUI.handleInventoryMovement() ) // handleInventoryMovement should be at the end of this check
		{
			if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_INVENTORY )
			{
				warpMouseToSelectedItem(nullptr, (Inputs::SET_CONTROLLER));
			}
			else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_SPELLS )
			{
				warpMouseToSelectedSpell(nullptr, (Inputs::SET_CONTROLLER));
			}
			else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_CHEST )
			{
				warpMouseToSelectedChestSlot(nullptr, (Inputs::SET_CONTROLLER));
			}
			else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_SHOP )
			{
				players[player]->shopGUI.warpMouseToSelectedShopItem(nullptr, (Inputs::SET_CONTROLLER));
			}
			else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_TINKERING )
			{
				tinkerGUI.warpMouseToSelectedTinkerItem(nullptr, (Inputs::SET_CONTROLLER));
			}
			else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_ALCHEMY )
			{
				alchemyGUI.warpMouseToSelectedAlchemyItem(nullptr, (Inputs::SET_CONTROLLER));
			}
			else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_FEATHER )
			{
				featherGUI.warpMouseToSelectedFeatherItem(nullptr, (Inputs::SET_CONTROLLER));
			}
			else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_ASSISTSHRINE )
			{
				assistShrineGUI.warpMouseToSelectedAssistShrineItem(nullptr, (Inputs::SET_CONTROLLER));
			}
			else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_HOTBAR )
			{
				disableMouseDisablingHotbarFocus = true;
			}
		}
	}

	Input& input = Input::inputs[player];
	auto selectedSlotFrame = playerInventoryFrames[player].selectedSlotFrame;
	auto selectedSlotCursor = selectedItemCursorFrame;

	{
		auto autosortFrame = frame->findFrame("autosort frame");
		auto autosortBtn = autosortFrame->findButton("autosort button");
		auto autosortGlyph = autosortFrame->findImage("autosort glyph");
		autosortGlyph->disabled = true;

		SDL_Rect autosortFrameSize = autosortFrame->getSize();
		SDL_Rect portraitSize = playerInventoryFrames[player].characterPreview->getSize();
		static ConsoleVariable<int> cvar_autosortBtnX("/autosort_button_x", 0);
		static ConsoleVariable<int> cvar_autosortBtnY("/autosort_button_y", 0);
		autosortFrameSize.x = portraitSize.x + 2 + *cvar_autosortBtnX;
		autosortFrameSize.y = portraitSize.y + 2 + *cvar_autosortBtnY;
		autosortFrame->setSize(autosortFrameSize);

		autosortBtn->setDisabled(!paperDollContextMenuActive());
		autosortBtn->setInvisible(false);

		if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_PORTRAIT )
		{
			autosortBtn->setInvisible(true);
			if ( inputs.hasController(player) && !inputs.getVirtualMouse(player)->draw_cursor )
			{
				if ( ticks % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
				{
					autosortGlyph->path = input.getGlyphPathForBinding("InventoryCharacterRotateLeft", false);
				}
				else
				{
					autosortGlyph->path = input.getGlyphPathForBinding("InventoryCharacterRotateRight", false);
				}
			}
			else
			{
				if ( ticks % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
				{
					autosortGlyph->path = input.getGlyphPathForBinding("InventoryCharacterRotateLeftMouse", false);
				}
				else
				{
					autosortGlyph->path = input.getGlyphPathForBinding("InventoryCharacterRotateRightMouse", false);
				}
			}
			if ( auto imgGet = Image::get(autosortGlyph->path.c_str()) )
			{
				autosortGlyph->disabled = false;
				autosortGlyph->color = makeColor(255, 255, 255, 255);
				autosortGlyph->pos.w = imgGet->getWidth();
				autosortGlyph->pos.h = imgGet->getHeight();
				autosortGlyph->pos.x = autosortBtn->getSize().x + autosortBtn->getSize().w / 2 - autosortGlyph->pos.w / 2;
				autosortGlyph->pos.y = autosortBtn->getSize().y;
			}
		}
		else if ( !inputs.getVirtualMouse(player)->draw_cursor )
		{
			autosortBtn->setBackground("*#images/ui/Inventory/HUD_Button_AutosortGamepad.png");
			autosortBtn->setSize(SDL_Rect{ autosortFrameSize.w - 42, autosortFrameSize.h - 42, 42, 38 });

			autosortGlyph->path = input.getGlyphPathForBinding("PaperDollContextMenu", true);
			if ( auto imgGet = Image::get(autosortGlyph->path.c_str()) )
			{
				autosortGlyph->disabled = false;
				autosortGlyph->color = makeColor(255, 255, 255, 255);
				if ( autosortBtn->isDisabled() )
				{
					autosortGlyph->color = makeColor(255, 255, 255, 128);
				}
				autosortGlyph->pos.w = imgGet->getWidth();
				autosortGlyph->pos.h = imgGet->getHeight();
				autosortGlyph->pos.x = autosortBtn->getSize().x + 4;
				autosortGlyph->pos.y = autosortBtn->getSize().y;
			}
		}
		else
		{
			autosortBtn->setBackground("*#images/ui/Inventory/HUD_Button_AutosortUnselect.png");
			autosortBtn->setSize(SDL_Rect{ autosortFrameSize.w - 42, autosortFrameSize.h - 42, 42, 38 });
		}

		if ( paperDollContextMenuActive() )
		{
			bool keyboardInputPressed = (Input::inputs[player].binaryToggle("MenuRightClick") && inputs.bPlayerUsingKeyboardControl(player));
			if ( keyboardInputPressed && autosortBtn->isSelected() && autosortBtn->isHighlighted() )
			{
				if ( /*players[player]->bUseCompactGUIHeight()*/true )
				{
					players[player]->GUI.dropdownMenu.open("paper_doll");
					players[player]->GUI.dropdownMenu.dropDownToggleClick = false;
					players[player]->GUI.dropdownMenu.dropDownY = std::max(players[player]->GUI.dropdownMenu.dropDownY, 
						players[player]->camera_virtualy1());
				}
			}
			else if ( input.consumeBinaryToggle("PaperDollContextMenu") )
			{
				players[player]->GUI.dropdownMenu.open("paper_doll");
				players[player]->GUI.dropdownMenu.dropDownToggleClick = !keyboardInputPressed;
				SDL_Rect dropdownPos = players[player]->inventoryUI.frame->findFrame("inventory character preview")->getSize();
				dropdownPos.y += dropdownPos.h;
				if ( auto interactMenuTop = players[player]->GUI.dropdownMenu.dropdownFrame->findImage("interact top background") )
				{
					// 10px is slot half height, move by 1.5 slots, minus the top interact text height
					dropdownPos.y -= (interactMenuTop->pos.h + (3 * 10) + 4);
				}
				if ( players[player]->GUI.dropdownMenu.getDropDownAlignRight("paper_doll") )
				{
					players[player]->GUI.dropdownMenu.dropDownX = dropdownPos.x + dropdownPos.w;
				}
				else
				{
					players[player]->GUI.dropdownMenu.dropDownX = dropdownPos.x;
				}
				players[player]->GUI.dropdownMenu.dropDownX += players[player]->camera_virtualx1();
				players[player]->GUI.dropdownMenu.dropDownY = dropdownPos.y + players[player]->camera_virtualy1();
			}
			if ( input.consumeBinaryToggle("Autosort Inventory") && input.input("Autosort Inventory").isBindingUsingKeyboard() )
			{
				autosortInventory(player);
				//quickStackItems();
				Player::soundActivate();
			}
		}

		if ( (inventoryControlActive && inputs.getVirtualMouse(player)->draw_cursor
			&& players[player]->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_INVENTORY)
			&& autosortBtn->isHighlighted())
			|| (players[player]->GUI.isDropdownActive()
				&& players[player]->GUI.dropdownMenu.currentName == "paper_doll") )
		{
			if ( inputs.getVirtualMouse(player)->draw_cursor )
			{
				players[player]->GUI.setHoveringOverModuleButton(Player::GUI_t::MODULE_INVENTORY);
				if ( players[player]->GUI.activeModule != Player::GUI_t::MODULE_INVENTORY )
				{
					players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
				}
			}
			SDL_Rect pos = autosortBtn->getAbsoluteSize();
			// make sure to adjust absolute size to camera viewport
			pos.x -= players[player]->camera_virtualx1();
			pos.y -= players[player]->camera_virtualy1();

			//selectedSlotCursor->setDisabled(false);
			//updateSelectedSlotAnimation(pos.x - 1, pos.y - 1, pos.w, pos.h, inputs.getVirtualMouse(player)->draw_cursor);

			players[player]->hud.setCursorDisabled(false);
			players[player]->hud.updateCursorAnimation(pos.x - 1, pos.y - 1, pos.w, pos.h,
				true);
			//else if ( playerInventoryFrames[player].characterPreview->capturesMouse()
			//	&& players[player]->GUI.hoveringOverModuleButton() == Player::GUI_t::MODULE_NONE )
			//{
			//	if ( players[player]->GUI.activeModule != Player::GUI_t::MODULE_PORTRAIT )
			//	{
			//		SDL_Rect size = playerInventoryFrames[player].characterPreview->getAbsoluteSize();
			//		players[player]->GUI.activateModule(Player::GUI_t::MODULE_PORTRAIT);

			//		// make sure to adjust absolute size to camera viewport
			//		const int offsetX = 4;
			//		const int offsetY = 6;
			//		size.x += offsetX;
			//		size.y += offsetY;
			//		size.w -= offsetX * 2;
			//		size.h -= offsetY * 2;
			//		size.x -= players[player]->camera_virtualx1();
			//		size.y -= players[player]->camera_virtualy1();

			//		players[player]->hud.updateCursorAnimation(size.x - 1, size.y - 1,
			//			size.w, size.h, inputs.getVirtualMouse(player)->draw_cursor);
			//	}
			//}
		}
	}
	//DebugStats.gui7 = std::chrono::high_resolution_clock::now();
	resetInventorySlotFrames(player);

	auto invSlotsFrame = playerInventoryFrames[player].invSlotsFrame;
	auto dollSlotsFrame = playerInventoryFrames[player].dollSlotsFrame;

	bool tinkerCraftableListOpen = tinkerGUI.isConstructMenuActive();
	bool tinkeringSalvageOrRepairMenuActive = tinkerGUI.isSalvageOrRepairMenuActive();
	bool featherDrawerOpen = featherGUI.isInscriptionDrawerOpen();
	bool featherInscribeOrRepairActive = featherGUI.isInscribeOrRepairActive();
	featherGUI.highlightedSlot = -1;

	if ( true )
	{
		//Highlight (draw a gold border) currently selected inventory slot (for gamepad).
		//Only if item menu is not open

		Frame* slotFrameToHighlight = nullptr;
		int startx = 0;
		int starty = 0;
		int highlightWidth = 0;
		int highlightHeight = 0;

		if ( tinkerCraftableListOpen && players[player]->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_TINKERING) )
		{
			for ( int x = 0; x < tinkerGUI.MAX_TINKER_X; ++x )
			{
				for ( int y = 0; y < tinkerGUI.MAX_TINKER_Y; ++y )
				{
					if ( auto slotFrame = tinkerGUI.getTinkerSlotFrame(x, y) )
					{
						if ( !players[player]->GUI.isDropdownActive() ) // don't update selected slot while item menu open
						{
							if ( tinkerGUI.isInteractable && slotFrame->capturesMouseInRealtimeCoords() )
							{
								tinkerGUI.selectTinkerSlot(x, y);
								if ( inputs.getVirtualMouse(player)->draw_cursor )
								{
									// mouse movement captures the inventory
									players[player]->GUI.activateModule(Player::GUI_t::MODULE_TINKERING);
								}
							}
						}

						if ( x == tinkerGUI.getSelectedTinkerSlotX()
							&& y == tinkerGUI.getSelectedTinkerSlotY()
							&& players[player]->GUI.activeModule == Player::GUI_t::MODULE_TINKERING
							&& tinkerGUI.isInteractable )
						{
							slotFrameToHighlight = slotFrame;
							startx = slotFrame->getAbsoluteSize().x;
							starty = slotFrame->getAbsoluteSize().y;
							startx -= players[player]->camera_virtualx1(); // offset any splitscreen camera positioning.
							starty -= players[player]->camera_virtualy1();
							highlightHeight = slotFrame->getSize().h;
						}
					}
				}
			}
		}

		if ( featherDrawerOpen && players[player]->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_FEATHER) )
		{
			for ( int x = 0; x < featherGUI.MAX_FEATHER_X; ++x )
			{
				for ( int y = 0; y < featherGUI.MAX_FEATHER_Y; ++y )
				{
					if ( auto slotFrame = featherGUI.getFeatherSlotFrame(x, y) )
					{
						if ( !players[player]->GUI.isDropdownActive() ) // don't update selected slot while item menu open
						{
							if ( featherGUI.isSlotVisible(x, y) && featherGUI.isInteractable && slotFrame->capturesMouseInRealtimeCoords() )
							{
								featherGUI.selectFeatherSlot(x, y);
								if ( inputs.getVirtualMouse(player)->draw_cursor )
								{
									// mouse movement captures the inventory
									players[player]->GUI.activateModule(Player::GUI_t::MODULE_FEATHER);
								}
							}
						}

						if ( x == featherGUI.getSelectedFeatherSlotX()
							&& y == featherGUI.getSelectedFeatherSlotY()
							&& players[player]->GUI.activeModule == Player::GUI_t::MODULE_FEATHER
							&& featherGUI.isInteractable
							&& featherGUI.isSlotVisible(x, y) )
						{
							featherGUI.highlightedSlot = y;
							slotFrameToHighlight = slotFrame;
							startx = slotFrame->getAbsoluteSize().x;
							starty = slotFrame->getAbsoluteSize().y;
							startx -= players[player]->camera_virtualx1(); // offset any splitscreen camera positioning.
							starty -= players[player]->camera_virtualy1();
							highlightWidth = slotFrame->getSize().w;
							featherGUI.scrollToSlot(x, y, false);
						}
					}
				}
			}
		}

		if ( shopGUI.bOpen && players[player]->inventory_mode == INVENTORY_MODE_ITEM
			&& players[player]->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_SHOP) )
		{
			for ( int x = 0; x < shopGUI.MAX_SHOP_X; ++x )
			{
				for ( int y = 0; y < shopGUI.MAX_SHOP_Y; ++y )
				{
					if ( auto slotFrame = shopGUI.getShopSlotFrame(x, y) )
					{
						if ( !players[player]->GUI.isDropdownActive() ) // don't update selected slot while item menu open
						{
							if ( shopGUI.isInteractable && slotFrame->capturesMouseInRealtimeCoords() )
							{
								shopGUI.selectShopSlot(x, y);
								if ( inputs.getVirtualMouse(player)->draw_cursor )
								{
									// mouse movement captures the inventory
									players[player]->GUI.activateModule(Player::GUI_t::MODULE_SHOP);
								}
							}
						}

						if ( x == shopGUI.getSelectedShopX()
							&& y == shopGUI.getSelectedShopY()
							&& players[player]->GUI.activeModule == Player::GUI_t::MODULE_SHOP
							&& shopGUI.isInteractable )
						{
							slotFrameToHighlight = slotFrame;
							startx = slotFrame->getAbsoluteSize().x;
							starty = slotFrame->getAbsoluteSize().y;
							startx -= players[player]->camera_virtualx1(); // offset any splitscreen camera positioning.
							starty -= players[player]->camera_virtualy1();
						}
					}
				}
			}
		}

		if ( alchemyGUI.bOpen && players[player]->inventory_mode == INVENTORY_MODE_ITEM
			&& players[player]->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_ALCHEMY) )
		{
			for ( int x = alchemyGUI.ALCH_SLOT_RESULT_POTION_X; x < alchemyGUI.MAX_ALCH_X; ++x )
			{
				for ( int y = 0; y < alchemyGUI.MAX_ALCH_Y; ++y )
				{
					if ( y > 0 && x < 0 )
					{
						continue;
					}
					bool slotVisible = false;
					if ( x < 0 )
					{
						if ( alchemyGUI.animPotion1 < 0.001 && alchemyGUI.animPotion2 < 0.001 )
						{
							slotVisible = true;
						}
					}
					else if ( alchemyGUI.recipes.bOpen && alchemyGUI.recipes.isInteractable )
					{
						slotVisible = alchemyGUI.recipes.isSlotVisible(x, y);
					}

					if ( auto slotFrame = alchemyGUI.getAlchemySlotFrame(x, y) )
					{
						if ( !players[player]->GUI.isDropdownActive() ) // don't update selected slot while item menu open
						{
							if ( slotVisible && alchemyGUI.isInteractable && slotFrame->capturesMouseInRealtimeCoords() )
							{
								alchemyGUI.selectAlchemySlot(x, y);
								if ( inputs.getVirtualMouse(player)->draw_cursor )
								{
									// mouse movement captures the inventory
									players[player]->GUI.activateModule(Player::GUI_t::MODULE_ALCHEMY);
								}
							}
						}

						bool hideCursor = false;
						/*if ( !alchemyGUI.notifications.empty() && alchemyGUI.recipes.bOpen && y == 0 && x >= 0 )
						{
							hideCursor = true;
						}*/
						if ( x == alchemyGUI.getSelectedAlchemySlotX()
							&& y == alchemyGUI.getSelectedAlchemySlotY()
							&& !hideCursor
							&& players[player]->GUI.activeModule == Player::GUI_t::MODULE_ALCHEMY
							&& alchemyGUI.isInteractable
							&& !(x == alchemyGUI.ALCH_SLOT_RESULT_POTION_X && alchemyGUI.animPotionResult > 0.001)
							&& slotVisible )
						{
							slotFrameToHighlight = slotFrame;
							startx = slotFrame->getAbsoluteSize().x;
							starty = slotFrame->getAbsoluteSize().y;
							startx -= players[player]->camera_virtualx1(); // offset any splitscreen camera positioning.
							starty -= players[player]->camera_virtualy1();
						}
					}
				}
			}
		}

		if ( assistShrineGUI.bOpen && players[player]->inventory_mode == INVENTORY_MODE_ITEM
			&& players[player]->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_ASSISTSHRINE) )
		{
			for ( int x = assistShrineGUI.ASSIST_SLOT_RING; x <= assistShrineGUI.MAX_ASSISTSHRINE_X; ++x )
			{
				if ( x == assistShrineGUI.MAX_ASSISTSHRINE_X )
				{
					x = assistShrineGUI.ASSIST_RACE_COLUMN;
				}
				for ( int y = 0; y < assistShrineGUI.MAX_ASSISTSHRINE_Y; ++y )
				{
					if ( y > 0 && x < 0 )
					{
						continue;
					}

					if ( assistShrineGUI.currentView == assistShrineGUI.ASSIST_SHRINE_VIEW_ITEMS )
					{
						if ( x >= 0 )
						{
							continue;
						}
					}

					bool slotVisible = assistShrineGUI.isSlotVisible(x, y);

					if ( auto slotFrame = assistShrineGUI.getAssistShrineSlotFrame(x, y) )
					{
						if ( !players[player]->GUI.isDropdownActive() ) // don't update selected slot while item menu open
						{
							if ( slotVisible && assistShrineGUI.isInteractable && slotFrame->capturesMouseInRealtimeCoords() )
							{
								assistShrineGUI.selectAssistShrineSlot(x, y);
								if ( inputs.getVirtualMouse(player)->draw_cursor )
								{
									// mouse movement captures the inventory
									players[player]->GUI.activateModule(Player::GUI_t::MODULE_ASSISTSHRINE);
								}
							}
						}

						bool hideCursor = false;
						if ( x == assistShrineGUI.getSelectedAssistShrineX()
							&& y == assistShrineGUI.getSelectedAssistShrineY()
							&& !hideCursor
							&& players[player]->GUI.activeModule == Player::GUI_t::MODULE_ASSISTSHRINE
							&& assistShrineGUI.isInteractable
							&& slotVisible )
						{
							slotFrameToHighlight = slotFrame;
							startx = slotFrame->getAbsoluteSize().x;
							starty = slotFrame->getAbsoluteSize().y;
							startx -= players[player]->camera_virtualx1(); // offset any splitscreen camera positioning.
							starty -= players[player]->camera_virtualy1();
							if ( assistShrineGUI.currentView == assistShrineGUI.ASSIST_SHRINE_VIEW_CLASSES
								&& x >= 0 && x < assistShrineGUI.MAX_ASSISTSHRINE_X
								&& y >= 0 && y < assistShrineGUI.MAX_ASSISTSHRINE_Y )
							{
								startx += 1;
								starty += 3;
								highlightWidth = slotFrame->getSize().w - 6;
								highlightHeight = slotFrame->getSize().h - 6;
							}
							else if ( assistShrineGUI.currentView == assistShrineGUI.ASSIST_SHRINE_VIEW_RACE
								&& x == assistShrineGUI.ASSIST_RACE_COLUMN
								&& y >= 0 && y < assistShrineGUI.MAX_ASSISTSHRINE_Y )
							{
								highlightWidth = slotFrame->getSize().w;
								highlightHeight = slotFrame->getSize().h;
							}
						}
					}
				}
			}
		}

		if ( chestGUI.bOpen && players[player]->inventory_mode == INVENTORY_MODE_ITEM
			&& players[player]->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_CHEST) )
		{
			for ( int x = 0; x < MAX_CHEST_X; ++x )
			{
				for ( int y = 0; y < MAX_CHEST_Y; ++y )
				{
					if ( auto slotFrame = getChestSlotFrame(x, y) )
					{
						if ( !players[player]->GUI.isDropdownActive() ) // don't update selected slot while item menu open
						{
							if ( chestGUI.isInteractable && slotFrame->capturesMouseInRealtimeCoords() )
							{
								selectChestSlot(x, y);
								if ( inputs.getVirtualMouse(player)->draw_cursor )
								{
									// mouse movement captures the inventory
									players[player]->GUI.activateModule(Player::GUI_t::MODULE_CHEST);
								}
							}
						}

						if ( x == getSelectedChestX()
							&& y == getSelectedChestY()
							&& players[player]->GUI.activeModule == Player::GUI_t::MODULE_CHEST
							&& chestGUI.isInteractable )
						{
							slotFrameToHighlight = slotFrame;
							startx = slotFrame->getAbsoluteSize().x;
							starty = slotFrame->getAbsoluteSize().y;
							startx -= players[player]->camera_virtualx1(); // offset any splitscreen camera positioning.
							starty -= players[player]->camera_virtualy1();
						}
					}
				}
			}
		}

		if ( players[player]->inventory_mode == INVENTORY_MODE_ITEM
			&& !tinkerCraftableListOpen
			&& !featherDrawerOpen
			&& !(alchemyGUI.bOpen && !alchemyGUI.isInteractable)
			&& players[player]->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_INVENTORY) )
		{
			for ( int x = 0; x < getSizeX(); ++x )
			{
				for ( int y = Player::Inventory_t::DOLL_ROW_1; y < getSizeY(); ++y )
				{
					if ( auto slotFrame = getInventorySlotFrame(x, y) )
					{
						if ( !players[player]->GUI.isDropdownActive() ) // don't update selected slot while item menu open
						{
							if ( isInteractable && slotFrame->capturesMouseInRealtimeCoords() )
							{
								selectSlot(x, y);
								if ( inputs.getVirtualMouse(player)->draw_cursor )
								{
									// mouse movement captures the inventory
									players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
								}
							}
						}

						if ( x == getSelectedSlotX()
							&& y == getSelectedSlotY()
							&& players[player]->GUI.activeModule == Player::GUI_t::MODULE_INVENTORY
							&& isInteractable )
						{
							slotFrameToHighlight = slotFrame;
							startx = slotFrame->getAbsoluteSize().x;
							starty = slotFrame->getAbsoluteSize().y;
							startx -= players[player]->camera_virtualx1(); // offset any splitscreen camera positioning.
							starty -= players[player]->camera_virtualy1();
						}
					}
				}
			}
		}
		else if ( players[player]->inventory_mode == INVENTORY_MODE_SPELL
			&& !tinkerCraftableListOpen
			&& !featherDrawerOpen
			&& players[player]->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_SPELLS) )
		{
			for ( int x = 0; x < MAX_SPELLS_X; ++x )
			{
				for ( int y = 0; y < MAX_SPELLS_Y; ++y )
				{
					if ( auto slotFrame = getSpellSlotFrame(x, y) )
					{
						if ( !players[player]->GUI.isDropdownActive() ) // don't update selected slot while item menu open
						{
							if ( spellPanel.isSlotVisible(x, y) && spellPanel.isInteractable )
							{
								if ( slotFrame->capturesMouseInRealtimeCoords() )
								{
									selectSpell(x, y);
									if ( inputs.getVirtualMouse(player)->draw_cursor )
									{
										// mouse movement captures the spells list
										players[player]->GUI.activateModule(Player::GUI_t::MODULE_SPELLS);
									}
								}
							}
						}

						if ( x == getSelectedSpellX()
							&& y == getSelectedSpellY()
							&& players[player]->GUI.activeModule == Player::GUI_t::MODULE_SPELLS
							&& spellPanel.isInteractable
							&& spellPanel.isSlotVisible(x, y) )
						{
							slotFrameToHighlight = slotFrame;
							startx = slotFrame->getAbsoluteSize().x;
							starty = slotFrame->getAbsoluteSize().y;
							startx -= players[player]->camera_virtualx1(); // offset any splitscreen camera positioning.
							starty -= players[player]->camera_virtualy1();
							spellPanel.scrollToSlot(x, y, false);
						}
					}
				}
			}
		}

		bool highlighted = false;
		if ( slotFrameToHighlight )
		{
			bool paperDollMenuOpen = players[player]->GUI.dropdownMenu.currentName == "paper_doll";
			if ( (players[player]->GUI.isDropdownActive() && !paperDollMenuOpen && !itemMenuFromHotbar) || // if item menu open, then always draw cursor on current item.
				(!selectedItem	// otherwise, if no selected item, and mouse hovering over item
					&& !(players[player]->GUI.isDropdownActive() && itemMenuFromHotbar)
					&& !paperDollMenuOpen
					&& (!inputs.getVirtualMouse(player)->draw_cursor
						|| (inputs.getVirtualMouse(player)->draw_cursor && slotFrameToHighlight->capturesMouse()))) )
			{
				highlighted = true;
				int width = getSlotSize();
				if ( highlightWidth > 0 )
				{
					width = highlightWidth;
				}
				int height = getSlotSize();
				if ( highlightHeight > 0 )
				{
					height = highlightHeight;
				}
				selectedSlotFrame->setSize(SDL_Rect{ startx + 1, starty + 1, selectedSlotFrame->getSize().w, selectedSlotFrame->getSize().h });
				selectedSlotFrame->setDisabled(false);

				selectedSlotCursor->setDisabled(false);
				updateSelectedSlotAnimation(startx, starty, width, height, inputs.getVirtualMouse(player)->draw_cursor);
				//messagePlayer(0, "0: %d, %d", x, y);
			}
		}

		if ( !highlighted )
		{
			featherGUI.highlightedSlot = -1;
		}
	}
	//DebugStats.gui8 = std::chrono::high_resolution_clock::now();
	players[player]->paperDoll.drawSlots();
	// dragging item - highlight slots
	if ( selectedItem && selectedSlotFrame->isDisabled() )
	{
		bool isSpell = itemCategory(selectedItem) == SPELL_CAT;
		bool isFromChest = isItemFromChest(selectedItem);

		Frame* hoveringDollSlotFrame = nullptr;
		int loopStartY = 0;
		if ( inputs.getVirtualMouse(player)->draw_cursor )
		{
			if ( dollSlotsFrame && dollSlotsFrame->capturesMouseInRealtimeCoords() && players[player]->inventory_mode == INVENTORY_MODE_ITEM )
			{
				auto slotName = items[selectedItem->type].item_slot;
				Player::PaperDoll_t::PaperDollSlotType dollSlot = getPaperDollSlotFromItemType(*selectedItem);
				if ( dollSlot != Player::PaperDoll_t::PaperDollSlotType::SLOT_MAX )
				{
					bool drawSelectedSlotOnPaperDoll = true;
					// get coordinates of doll slot
					if ( isFromChest )
					{
						// don't draw on occupied slots - not allowed to drag onto equipped items
						node_t* nextnode = nullptr;
						for ( node_t* node = stats[player]->inventory.first; node != NULL; node = nextnode )
						{
							nextnode = node->next;
							Item* tempItem = (Item*)(node->element);
							if ( !tempItem ) { continue; }
							if ( players[player]->paperDoll.getSlotForItem(*tempItem) == dollSlot )
							{
								drawSelectedSlotOnPaperDoll = false;
								break;
							}
						}
					}

					if ( drawSelectedSlotOnPaperDoll )
					{
						int slotFrameX, slotFrameY;
						players[player]->paperDoll.getCoordinatesFromSlotType(dollSlot, slotFrameX, slotFrameY);
						hoveringDollSlotFrame = getInventorySlotFrame(slotFrameX, slotFrameY);
					}
				}
			}
		}
		else
		{
			loopStartY = DOLL_ROW_1;
		}

		bool mouseInChest = false;
		if ( !isSpell )
		{
			if ( players[player]->inventoryUI.chestFrame
				&& !players[player]->inventoryUI.chestFrame->isDisabled()
				&& openedChest[player] )
			{
				if ( auto chestSlots = playerInventoryFrames[player].chestFrameSlots )
				{
					if ( !chestSlots->isDisabled() && chestSlots->capturesMouseInRealtimeCoords() )
					{
						mouseInChest = true;
					}
				}
			}
		}

		const int maxX = std::max(getSizeX(), MAX_SPELLS_X);
		const int maxY = std::max(getSizeY(), MAX_SPELLS_Y);
		for ( int x = 0; x < maxX; ++x )
		{
			for ( int y = loopStartY; y < maxY; ++y )
			{
				Frame* slotFrame = nullptr;
				if ( !isSpell && !isFromChest && mouseInChest )
				{
					if ( x >= 0 && y >= 0 && x < MAX_CHEST_X && y < MAX_CHEST_Y )
					{
						slotFrame = getChestSlotFrame(x, y);
					}
				}
				else if ( !isSpell && isFromChest && !mouseInChest )
				{
					slotFrame = getInventorySlotFrame(x, y);
				}
				else
				{
					slotFrame = getItemSlotFrame(selectedItem, x, y);
				}
				if ( hoveringDollSlotFrame )
				{
					slotFrame = dollSlotsFrame;
					--y; // one-off check
				}

				if ( !slotFrame )
				{
					continue;
				}

				if ( isSpell 
					&& (!spellPanel.isSlotVisible(x, y)) )
				{
					continue;
				}
				if ( !isSpell && y >= DEFAULT_INVENTORY_SIZEY && (getSizeY() <= DEFAULT_INVENTORY_SIZEY) )
				{
					// backpack slots but invisible, skip em.
					continue;
				}
				if ( itemCategory(selectedItem) == SPELL_CAT && players[player]->inventory_mode == INVENTORY_MODE_ITEM )
				{
					continue;
				}
				else if ( itemCategory(selectedItem) != SPELL_CAT && players[player]->inventory_mode == INVENTORY_MODE_SPELL )
				{
					continue;
				}

				int startx = slotFrame->getAbsoluteSize().x;
				int starty = slotFrame->getAbsoluteSize().y;
				if ( hoveringDollSlotFrame )
				{
					startx = hoveringDollSlotFrame->getAbsoluteSize().x;
					starty = hoveringDollSlotFrame->getAbsoluteSize().y;
					hoveringDollSlotFrame = nullptr;
				}

				startx -= players[player]->camera_virtualx1();  // offset any splitscreen camera positioning.
				starty -= players[player]->camera_virtualy1();

				int selectedSlotX = getSelectedSlotX();
				int selectedSlotY = getSelectedSlotY();
				if ( itemCategory(selectedItem) == SPELL_CAT )
				{
					selectedSlotX = getSelectedSpellX();
					selectedSlotY = getSelectedSpellY();
				}
				else if ( mouseInChest )
				{
					selectedSlotX = getSelectedChestX();
					selectedSlotY = getSelectedChestY();
				}

				if ( inputs.getVirtualMouse(player)->draw_cursor )
				{
					if ( slotFrame->capturesMouse()
						&& slotFrame->capturesMouseInRealtimeCoords() )
					{
						// don't draw yellow border, but draw cursor - we're hovering over our current item
						selectedSlotCursor->setDisabled(false);
						updateSelectedSlotAnimation(startx, starty, getSlotSize(), getSlotSize(), inputs.getVirtualMouse(player)->draw_cursor);
						//messagePlayer(0, "5: %d, %d", x, y);
					}
					else if ( !slotFrame->capturesMouse()
						&& slotFrame->capturesMouseInRealtimeCoords() )
					{
						// if dragging item, use realtime coords to find what's selected.
						selectedSlotFrame->setSize(SDL_Rect{ startx + 1, starty + 1, selectedSlotFrame->getSize().w, selectedSlotFrame->getSize().h });
						selectedSlotFrame->setDisabled(false);

						selectedSlotCursor->setDisabled(false);
						updateSelectedSlotAnimation(startx, starty, getSlotSize(), getSlotSize(), inputs.getVirtualMouse(player)->draw_cursor);
						//messagePlayer(0, "1: %d, %d", x, y);
					}
				}
				else if ( slotFrame->capturesMouse() && x == selectedSlotX && y == selectedSlotY )
				{
					selectedSlotFrame->setSize(SDL_Rect{ startx + 1, starty + 1, selectedSlotFrame->getSize().w, selectedSlotFrame->getSize().h });
					selectedSlotFrame->setDisabled(false);

					selectedSlotCursor->setDisabled(false);
					updateSelectedSlotAnimation(startx, starty, getSlotSize(), getSlotSize(), inputs.getVirtualMouse(player)->draw_cursor);
					//messagePlayer(0, "2: %d, %d", x, y);
				}

				if ( !selectedSlotFrame->isDisabled() )
				{
					break;
				}
			}
			if ( !selectedSlotFrame->isDisabled() )
			{
				break;
			}
		}
	}

	//DebugStats.gui9 = std::chrono::high_resolution_clock::now();
	// draw contents of each slot
	auto oldSelectedSlotFrame = playerInventoryFrames[player].oldSelectedSlotFrame;
	oldSelectedSlotFrame->setDisabled(true);
	bool shopOpen = false;
	if ( shopGUI.bOpen && shopGUI.shopFrame && shopInv[player] && uidToEntity(shopkeeper[player]) )
	{
		shopOpen = true;
		for ( node = shopInv[player]->first; node != NULL; node = nextnode )
		{
			nextnode = node->next;
			Item* item = (Item*)node->element;
			if ( !item ) { continue; }

			if ( shopGUI.buybackView && !item->playerSoldItemToShop )
			{
				continue;
			}
			else if ( !shopGUI.buybackView && item->playerSoldItemToShop )
			{
				continue;
			}

			int itemx = item->x;
			int itemy = item->y;

			if ( itemx >= 0 && itemx < shopGUI.MAX_SHOP_X
				&& itemy >= 0 && itemy < shopGUI.MAX_SHOP_Y
				&& !hideItemFromShopView(*item) )
			{
				if ( auto slotFrame = getItemSlotFrame(item, itemx, itemy) )
				{
					slotFrame->setUserData(&GAMEUI_FRAMEDATA_SHOP_ITEM);
					static ConsoleVariable<bool> cvar_shop_backgrounds("/shopbackgrounds", false);
					if ( *cvar_shop_backgrounds )
					{
						updateSlotFrameFromItem(slotFrame, item);
					}
					else
					{
						bool oldIdentified = item->identified;
						Sint32 oldBeatitude = item->beatitude;
						item->beatitude = 0;
						updateSlotFrameFromItem(slotFrame, item);
						item->beatitude = oldBeatitude;
					}
				}
			}
		}
	}

	if ( chestGUI.bOpen && openedChest[player] )
	{
		list_t* chest_inventory = nullptr;
		if ( multiplayer == CLIENT )
		{
			chest_inventory = &chestInv[player];
		}
		else if ( openedChest[player]->children.first && openedChest[player]->children.first->element )
		{
			chest_inventory = (list_t*)openedChest[player]->children.first->element;
		}

		if ( chest_inventory )
		{
			for ( node = chest_inventory->first; node != NULL; node = nextnode )
			{
				nextnode = node->next;
				Item* item = (Item*)node->element;
				if ( !item ) { continue; }

				if ( item == selectedItem )
				{
					if ( chestGUI.isInteractable )
					{
						//Draw blue border around the slot if it's the currently grabbed item.
						//drawBlueInventoryBorder(player, *item, x, y);
						Frame* slotFrame = nullptr;

						SDL_Rect borderPos{ 0, 0, oldSelectedSlotFrame->getSize().w, oldSelectedSlotFrame->getSize().h };
						if ( slotFrame = getItemSlotFrame(selectedItem, selectedItem->x, selectedItem->y) )
						{
							borderPos.x = slotFrame->getAbsoluteSize().x;
							borderPos.y = slotFrame->getAbsoluteSize().y;
						}
						if ( slotFrame )
						{
							++borderPos.x;
							++borderPos.y;

							oldSelectedSlotFrame->setDisabled(false);
							oldSelectedSlotFrame->setSize(borderPos);

							auto oldSelectedSlotItem = playerInventoryFrames[player].oldSelectedSlotItemImg;
							oldSelectedSlotItem->disabled = false;
							oldSelectedSlotItem->path = getItemSpritePath(player, *item);
						}
					}
					continue;
				}

				int itemx = item->x;
				int itemy = item->y;

				if ( itemx >= 0 && itemx < MAX_CHEST_X
					&& itemy >= 0 && itemy < MAX_CHEST_Y )
				{
					if ( auto slotFrame = getItemSlotFrame(item, itemx, itemy) )
					{
						updateSlotFrameFromItem(slotFrame, item);
					}
				}
			}
		}
	}
	//DebugStats.gui10 = std::chrono::high_resolution_clock::now();
	for ( node = stats[player]->inventory.first; node != NULL; node = nextnode )
	{
		nextnode = node->next;
		Item* item = (Item*)node->element;
		if ( !item ) { continue; }

		if ( item == selectedItem )
		{
			bool interactable = true;
			if ( itemCategory(item) == SPELL_CAT )
			{
				if ( !spellPanel.isItemVisible(item) )
				{
					interactable = false;
				}
			}

			if ( interactable )
			{
				//Draw blue border around the slot if it's the currently grabbed item.
				//drawBlueInventoryBorder(player, *item, x, y);
				Frame* slotFrame = nullptr;

				SDL_Rect borderPos{ 0, 0, oldSelectedSlotFrame->getSize().w, oldSelectedSlotFrame->getSize().h };
				if ( players[player]->paperDoll.getSlotForItem(*item) != Player::PaperDoll_t::SLOT_MAX )
				{
					int slotx, sloty;
					this->player.paperDoll.getCoordinatesFromSlotType(players[player]->paperDoll.getSlotForItem(*item), slotx, sloty);
					if ( slotFrame = getItemSlotFrame(item, slotx, sloty) )
					{
						borderPos.x = dollSlotsFrame->getSize().x + slotFrame->getSize().x;
						borderPos.y = dollSlotsFrame->getSize().y + slotFrame->getSize().y;
					}
				}
				else
				{
					if ( slotFrame = getItemSlotFrame(selectedItem, selectedItem->x, selectedItem->y) )
					{
						borderPos.x = slotFrame->getAbsoluteSize().x;
						borderPos.y = slotFrame->getAbsoluteSize().y;
					}
				}
				if ( slotFrame )
				{
					++borderPos.x;
					++borderPos.y;

					oldSelectedSlotFrame->setDisabled(false);
					oldSelectedSlotFrame->setSize(borderPos);

					auto oldSelectedSlotItem = playerInventoryFrames[player].oldSelectedSlotItemImg;
					oldSelectedSlotItem->disabled = false;
					oldSelectedSlotItem->path = getItemSpritePath(player, *item);
				}
			}
			continue;
		}

		int itemx = item->x;
		int itemy = item->y;

		bool itemOnPaperDoll = false;
		if ( players[player]->paperDoll.enabled && itemIsEquipped(item, player) )
		{
			auto slotType = players[player]->paperDoll.getSlotForItem(*item);
			if ( slotType != Player::PaperDoll_t::SLOT_MAX )
			{
				itemOnPaperDoll = true;
			}
		}

		if ( itemOnPaperDoll )
		{
			players[player]->paperDoll.getCoordinatesFromSlotType(players[player]->paperDoll.getSlotForItem(*item), itemx, itemy);
		}

		if ( isInteractable && item->notifyIcon )
		{
			if ( appraisal.itemsToNotify.find(item->uid) == appraisal.itemsToNotify.end() )
			{
				appraisal.itemsToNotify[item->uid] = Appraisal_t::NOTIFY_ITEM_WAITING_TO_HOVER;
			}
			else
			{
				if ( appraisal.itemsToNotify[item->uid] == Appraisal_t::NOTIFY_ITEM_REMOVE )
				{
					appraisal.itemsToNotify.erase(item->uid);
					item->notifyIcon = false;
				}
			}
		}

		if ( itemCategory(item) == SPELL_CAT )
		{
			if ( auto slotFrame = getItemSlotFrame(item, itemx, itemy) )
			{
				updateSlotFrameFromItem(slotFrame, item);
			}
		}
		else if ( itemx >= 0 && itemx < getSizeX()
			&& itemy >= Player::Inventory_t::PaperDollRows::DOLL_ROW_1 && itemy < getSizeY() )
		{
			if ( auto slotFrame = getInventorySlotFrame(itemx, itemy) )
			{
				slotFrame->setUserData(nullptr);
				if ( shopOpen && !isItemSellableToShop(player, item) )
				{
					updateSlotFrameFromItem(slotFrame, item, true); // force grey backgrounds
				}
				else if ( featherInscribeOrRepairActive )
				{
					auto res = featherGUI.setItemDisplayNameAndPrice(item, true);
					if ( res == GenericGUIMenu::FeatherGUI_t::FEATHER_ACTION_OK )
					{
						updateSlotFrameFromItem(slotFrame, item);
					}
					else
					{
						updateSlotFrameFromItem(slotFrame, item, true); // force grey backgrounds
					}
				}
				else if ( itemfxGUI.isItemEffectMenuActive() )
				{
					auto res = itemfxGUI.setItemDisplayNameAndPrice(item, true);
					if ( res == GenericGUIMenu::ItemEffectGUI_t::ITEMFX_ACTION_OK )
					{
						updateSlotFrameFromItem(slotFrame, item);
					}
					else
					{
						updateSlotFrameFromItem(slotFrame, item, true); // force grey backgrounds
					}
				}
				else if ( tinkerCraftableListOpen || tinkeringSalvageOrRepairMenuActive )
				{
					// grab action status of this item, don't modify using 'true' param
					auto res = tinkerGUI.setItemDisplayNameAndPrice(item, true); 
					bool invalidItem = !(res == GenericGUIMenu::TinkerGUI_t::TINKER_ACTION_OK
						|| res == GenericGUIMenu::TinkerGUI_t::TINKER_ACTION_OK_UPGRADE
						|| res == GenericGUIMenu::TinkerGUI_t::TINKER_ACTION_OK_UNIDENTIFIED_SALVAGE);
					if ( tinkerCraftableListOpen && GenericGUI[player].isNodeTinkeringCraftableItem(item->node) )
					{
						if ( invalidItem )
						{
							updateSlotFrameFromItem(slotFrame, item, true); // force grey backgrounds
						}
						else
						{
							updateSlotFrameFromItem(slotFrame, item);
						}
					}
					else if ( tinkeringSalvageOrRepairMenuActive
						&& GenericGUI[player].tinkeringFilter == GenericGUIMenu::TINKER_FILTER_REPAIRABLE
						&& invalidItem )
					{
						updateSlotFrameFromItem(slotFrame, item, true); // force grey backgrounds
					}
					else if ( tinkeringSalvageOrRepairMenuActive
						&& GenericGUI[player].tinkeringFilter == GenericGUIMenu::TINKER_FILTER_SALVAGEABLE
						&& invalidItem )
					{
						updateSlotFrameFromItem(slotFrame, item, true); // force grey backgrounds
					}
					else
					{
						updateSlotFrameFromItem(slotFrame, item);
					}
				}
				else if ( alchemyGUI.bOpen )
				{
					if ( !(itemCategory(item) == POTION && item->type != POTION_EMPTY /*&& item != GenericGUI[player].alembicItem*/
						&& item->identified && !itemIsEquipped(item, player)) )
					{
						updateSlotFrameFromItem(slotFrame, item, true);
					}
					else if ( alchemyGUI.potionResultUid == item->uid 
						&& alchemyGUI.potionResultUid != alchemyGUI.potion1Uid
						&& alchemyGUI.potionResultUid != alchemyGUI.potion2Uid )
					{
						// set qty to 
						int oldCount = item->count;
						item->count = std::max(0, item->count - alchemyGUI.animPotionResultCount);
						if ( item->count > 0 )
						{
							slotFrame->setUserData(&GAMEUI_FRAMEDATA_ALCHEMY_ITEM);
							updateSlotFrameFromItem(slotFrame, item);
						}
						item->count = oldCount;
					}
					else if ( (alchemyGUI.potion1Uid == item->uid || alchemyGUI.potion2Uid == item->uid) )
					{
						slotFrame->setUserData(&GAMEUI_FRAMEDATA_ALCHEMY_ITEM);
						updateSlotFrameFromItem(slotFrame, item);
					}
					else
					{
						updateSlotFrameFromItem(slotFrame, item, !item->identified);
					}
				}
				else
				{
					updateSlotFrameFromItem(slotFrame, item);
				}
			}
		}
	}

	//DebugStats.gui11 = std::chrono::high_resolution_clock::now();

	// autosort button
	//if ( bNewInventoryLayout )
	//{
	//	// draw halfway down
	//	mode_pos.y += (getSizeY() / 2) * inventorySlotSize;
	//}
	//mode_pos.w = 24;
	//mode_pos.h = 24;
	//bool mouse_in_bounds = mouseInBounds(player, mode_pos.x, mode_pos.x + mode_pos.w, mode_pos.y, mode_pos.y + mode_pos.h);
	//if ( !mouse_in_bounds )
	//{
	//	drawWindow(mode_pos.x, mode_pos.y, mode_pos.x + mode_pos.w, mode_pos.y + mode_pos.h);
	//}
	//else
	//{
	//	drawDepressed(mode_pos.x, mode_pos.y, mode_pos.x + mode_pos.w, mode_pos.y + mode_pos.h);
	//}
	////ttfPrintText(ttf12, mode_pos.x, mode_pos.y + 6, "||");
	//if ( mouse_in_bounds )
	//{
	//	mode_pos.x += 2;
	//	mode_pos.y += 2;
	//	mode_pos.w -= 4;
	//	mode_pos.h -= 4;
	//	drawRect(&mode_pos, makeColorRGB(192, 192, 192), 64);
	//	// tooltip
	//	SDL_Rect src;
	//	src.x = mousex + 16;
	//	src.y = mousey + 8;
	//	src.h = TTF12_HEIGHT + 8;
	//	src.w = longestline(Language::get(2960)) * TTF12_WIDTH + 8;
	//	drawTooltip(&src);
	//	ttfPrintTextFormatted(ttf12, src.x + 4, src.y + 4, Language::get(2960), "DEPRECATED");
	//	if ( inputs.bMouseLeft(player) )
	//	{
	//		inputs.mouseClearLeft(player);
	//		autosortInventory(player);
	//		playSound(139, 64);
	//	}
	//}

	// mouse interactions
	bool noPreviousSelectedItem = (selectedItem == nullptr);

	shopGUI.clearItemDisplayed();
	tinkerGUI.clearItemDisplayed();
	alchemyGUI.clearItemDisplayed();
	itemfxGUI.clearItemDisplayed();
	if ( !featherDrawerOpen || (featherDrawerOpen && GenericGUI[player].scribingBlankScrollTarget == nullptr) )
	{
		featherGUI.clearItemDisplayed();
	}
	else
	{
		featherGUI.currentHoveringInscriptionLabel = "";
		featherGUI.chargeCostMin = 0;
		featherGUI.chargeCostMax = 0;
	}
	
	if ( !selectedItem && featherDrawerOpen )
	{
		list_t* player_inventory = nullptr;
		player_inventory = &GenericGUI[player].scribingTotalItems;
		if ( player_inventory )
		{
			for ( node = player_inventory->first; node != NULL; node = nextnode )
			{
				nextnode = node->next;
				Item* item = (Item*)node->element;
				if ( !item ) { continue; }

				if ( !GenericGUI[player].isNodeScribingCraftableItem(item->node) )
				{
					continue;
				}

				bool mouseOverSlot = false;

				int itemx = item->x;
				int itemy = item->y;

				auto slotFrame = featherGUI.getFeatherSlotFrame(itemx, itemy);
				if ( !slotFrame ) { continue; }

				mouseOverSlot = players[player]->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_FEATHER)
					&& slotFrame->capturesMouse();

				if ( mouseOverSlot && inputs.getVirtualMouse(player)->draw_cursor )
				{
					// mouse movement captures the inventory
					players[player]->GUI.activateModule(Player::GUI_t::MODULE_FEATHER);
				}

				if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_FEATHER
					&& (!featherGUI.isInteractable) )
				{
					// don't do anything while in motion
					break;
				}

				if ( stats[player]->HP <= 0 )
				{
					break;
				}

				if ( mouseOverSlot && players[player]->GUI.bActiveModuleUsesInventory() && featherGUI.isInteractable )
				{
					featherGUI.setItemDisplayNameAndPrice(item, false);
					break;
				}
			}
		}
	}
	if ( !selectedItem && tinkerCraftableListOpen )
	{
		list_t* player_inventory = nullptr;
		player_inventory = &GenericGUI[player].tinkeringTotalItems;
		if ( player_inventory )
		{
			for ( node = player_inventory->first; node != NULL; node = nextnode )
			{
				nextnode = node->next;
				Item* item = (Item*)node->element;
				if ( !item ) { continue; }

				if ( !GenericGUI[player].isNodeTinkeringCraftableItem(item->node) )
				{
					continue;
				}

				bool mouseOverSlot = false;

				int itemx = item->x;
				int itemy = item->y;

				auto slotFrame = getItemSlotFrame(item, itemx, itemy);
				if ( !slotFrame ) { continue; }

				if ( itemCategory(item) == SPELL_CAT
					|| (players[player]->inventory_mode == INVENTORY_MODE_SPELL) )
				{
					continue;    //Skip over this item if not in inventory mode
				}

				mouseOverSlot = players[player]->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_TINKERING)
					&& slotFrame->capturesMouse();

				if ( mouseOverSlot && inputs.getVirtualMouse(player)->draw_cursor )
				{
					// mouse movement captures the inventory
					players[player]->GUI.activateModule(Player::GUI_t::MODULE_TINKERING);
				}

				if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_TINKERING
					&& (!tinkerGUI.isInteractable) )
				{
					// don't do anything while in motion
					break;
				}

				if ( stats[player]->HP <= 0 )
				{
					break;
				}

				if ( mouseOverSlot && players[player]->GUI.bActiveModuleUsesInventory() && tinkerGUI.isInteractable )
				{
					tinkerGUI.setItemDisplayNameAndPrice(item);
					break;
				}
			}
		}
	}
	if ( !selectedItem && shopOpen )
	{
		for ( node = shopInv[player]->first; node != NULL; node = nextnode )
		{
			nextnode = node->next;
			Item* item = (Item*)node->element;
			if ( !item ) { continue; }

			bool mouseOverSlot = false;

			int itemx = item->x;
			int itemy = item->y;

			auto slotFrame = getItemSlotFrame(item, itemx, itemy);
			if ( !slotFrame ) { continue; }

			if ( itemCategory(item) == SPELL_CAT
				|| (players[player]->inventory_mode == INVENTORY_MODE_SPELL) )
			{
				continue;    //Skip over this item if not in inventory mode
			}

			mouseOverSlot = players[player]->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_SHOP)
				&& slotFrame->capturesMouse();

			if ( mouseOverSlot && inputs.getVirtualMouse(player)->draw_cursor )
			{
				// mouse movement captures the inventory
				//players[player]->GUI.activateModule(Player::GUI_t::MODULE_SHOP);
			}

			if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_SHOP
				&& (!shopGUI.isInteractable) )
			{
				// don't do anything while in motion
				break;
			}

			if ( stats[player]->HP <= 0 )
			{
				break;
			}

			if ( hideItemFromShopView(*item) )
			{
				continue;
			}
			if ( shopGUI.buybackView && !item->playerSoldItemToShop )
			{
				continue;
			}
			else if ( !shopGUI.buybackView && item->playerSoldItemToShop )
			{
				continue;
			}

			if ( mouseOverSlot && players[player]->GUI.bActiveModuleUsesInventory() && players[player]->GUI.activeModule == Player::GUI_t::MODULE_SHOP )
			{
				if ( shopGUI.isItemSelectedFromShop(item) )
				{
					shopGUI.setItemDisplayNameAndPrice(item);
				}
				break;
			}
		}
	}
	if ( !selectedItem && chestGUI.bOpen && openedChest[player] && chestFrame )
	{
		list_t* chest_inventory = nullptr;
		if ( multiplayer == CLIENT )
		{
			chest_inventory = &chestInv[player];
		}
		else if ( openedChest[player]->children.first && openedChest[player]->children.first->element )
		{
			chest_inventory = (list_t*)openedChest[player]->children.first->element;
		}

		if ( chest_inventory )
		{
			for ( node = chest_inventory->first; node != NULL; node = nextnode )
			{
				nextnode = node->next;
				Item* item = (Item*)node->element;
				if ( !item )
				{
					continue;
				}

				bool mouseOverSlot = false;

				int itemx = item->x;
				int itemy = item->y;

				auto slotFrame = getItemSlotFrame(item, itemx, itemy);
				if ( !slotFrame ) { continue; }

				if ( (players[player]->inventory_mode == INVENTORY_MODE_ITEM && itemCategory(item) == SPELL_CAT)
					|| (players[player]->inventory_mode == INVENTORY_MODE_SPELL && itemCategory(item) != SPELL_CAT) )
				{
					continue;    //Skip over this item if not in inventory mode
				}

				mouseOverSlot = players[player]->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_CHEST)
					&& slotFrame->capturesMouse();

				if ( mouseOverSlot && inputs.getVirtualMouse(player)->draw_cursor )
				{
					// mouse movement captures the inventory
					players[player]->GUI.activateModule(Player::GUI_t::MODULE_CHEST);
				}

				if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_CHEST
					&& (!chestGUI.isInteractable) )
				{
					// don't do anything while in motion
					break;
				}
				
				if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_CHEST
					&& !chestGUI.isItemVisible(item) )
				{
					// this item is obscured by the chest scroll, ignore.
					continue;
				}

				if ( mouseOverSlot && players[player]->GUI.bActiveModuleUsesInventory() )
				{
					auto chestBgFrame = playerInventoryFrames[player].chestBgFrame;
					int tooltipCoordX = 0;
					PanelJustify_t justify = inventoryPanelJustify;
					if ( bCompactView )
					{
						if ( justify == PANEL_JUSTIFY_LEFT )
						{
							justify = PANEL_JUSTIFY_RIGHT;
						}
						else if ( justify == PANEL_JUSTIFY_RIGHT )
						{
							justify = PANEL_JUSTIFY_LEFT;
						}
						if ( this->player.bAlignGUINextToInventoryCompact() ) // flip justify if next to inventory
						{
							justify = (justify == PANEL_JUSTIFY_RIGHT) ? PANEL_JUSTIFY_LEFT : PANEL_JUSTIFY_RIGHT;
						}
					}
					if ( !bCompactView )
					{
						Frame::image_t* chestBaseImg = playerInventoryFrames[player].chestBaseImg;

						if ( justify == PANEL_JUSTIFY_LEFT )
						{
							tooltipCoordX = chestFrame->getSize().x + chestBgFrame->getSize().x + 8;
							tooltipCoordX += chestBaseImg->pos.w;
						}
						else
						{
							tooltipCoordX = chestFrame->getSize().x + chestBgFrame->getSize().x - 8;
							tooltipCoordX += chestBaseImg->pos.x;
						}
					}
					else
					{
						Frame::image_t* compactImg = playerInventoryFrames[player].chestBaseImg;

						if ( justify == PANEL_JUSTIFY_LEFT )
						{
							tooltipCoordX = chestFrame->getSize().x + chestBgFrame->getSize().x + 8;
							tooltipCoordX += compactImg->pos.w;
						}
						else
						{
							tooltipCoordX = chestFrame->getSize().x + chestBgFrame->getSize().x - 8;
							tooltipCoordX += compactImg->pos.x;
						}
					}
					int tooltipCoordY = slotFrame->getAbsoluteSize().y - players[player]->camera_virtualy1();

					bool tooltipOpen = false;
					if ( inventoryControlActive && !bIsTooltipDelayed() )
					{
						tooltipOpen = true;
						players[player]->hud.updateFrameTooltip(item, tooltipCoordX, tooltipCoordY, justify);
					}

					if ( stats[player]->HP <= 0 )
					{
						break;
					}

					bool tooltipPromptWasDisabled = tooltipPromptFrame->isDisabled();
					if ( useItemDropdownOnGamepad == GamepadDropdownTypes::GAMEPAD_DROPDOWN_FULL && !inputs.getVirtualMouse(player)->draw_cursor )
					{
						tooltipPromptFrame->setDisabled(true);
					}
					if ( ((tooltipOpen && (!tooltipPromptWasDisabled))
						|| bIsTooltipDelayed())
						&& inventoryControlActive 
						&& !selectedItem )
					{
						auto contextTooltipOptions = getContextTooltipOptionsForItem(player, item, useItemDropdownOnGamepad, false);
						bool bindingPressed = false;
						for ( auto& option : contextTooltipOptions )
						{
							if ( Input::inputs[player].binaryToggle(getContextMenuOptionBindingName(player, option).c_str()) )
							{
								bindingPressed = true;

								if ( Input::inputs[player].getPlayerControlType() == Input::PLAYER_CONTROLLED_BY_KEYBOARD )
								{
									// rare bug causes rogue activations on keyboard controls holding space + opening inventory
									// skip this section and consume presses
									break;
								}

								if ( option == ItemContextMenuPrompts::PROMPT_DROP && players[player]->paperDoll.isItemOnDoll(*item) )
								{
									// need to unequip
									players[player]->inventoryUI.activateItemContextMenuOption(item, ItemContextMenuPrompts::PROMPT_UNEQUIP_FOR_DROP);
									players[player]->paperDoll.updateSlots();
									if ( players[player]->paperDoll.isItemOnDoll(*item) )
									{
										// couldn't unequip, no more actions
									}
									else
									{
										// successfully unequipped, let's drop it.
										bool droppedAll = false;
										while ( item && item->count > 1 )
										{
											droppedAll = dropItem(item, player);
											if ( droppedAll )
											{
												item = nullptr;
											}
										}
										if ( !droppedAll )
										{
											dropItem(item, player);
										}
									}
								}
								else
								{
									activateItemContextMenuOption(item, option);
									if ( option == ItemContextMenuPrompts::PROMPT_DROPDOWN )
									{
										if ( !players[player]->GUI.isDropdownActive() )
										{
											players[player]->GUI.dropdownMenu.open("chest_interact");
											players[player]->GUI.dropdownMenu.dropDownToggleClick = true;
											players[player]->GUI.dropdownMenu.dropDownItem = item->uid;
											SDL_Rect dropdownPos = slotFrame->getAbsoluteSize();
											dropdownPos.y += dropdownPos.h / 2;
											if ( auto interactMenuTop = players[player]->GUI.dropdownMenu.dropdownFrame->findImage("interact top background") )
											{
												// 10px is slot half height, move by 0.5 slots, minus the top interact text height
												dropdownPos.y -= (interactMenuTop->pos.h + (1 * 10) + 4);
											}

											if ( players[player]->GUI.dropdownMenu.getDropDownAlignRight("chest_interact") )
											{
												dropdownPos.x += dropdownPos.w - 10;
											}
											else
											{
												dropdownPos.x += 10;
											}
											dropdownPos.x = std::max(dropdownPos.x, players[player]->camera_virtualx1());
											dropdownPos.x = std::min(dropdownPos.x, players[player]->camera_virtualx1() + players[player]->camera_virtualWidth());
											players[player]->GUI.dropdownMenu.dropDownX = dropdownPos.x;
											dropdownPos.y = std::max(dropdownPos.y, players[player]->camera_virtualy1());
											dropdownPos.y = std::min(dropdownPos.y, players[player]->camera_virtualy1() + players[player]->camera_virtualHeight());
											players[player]->GUI.dropdownMenu.dropDownY = dropdownPos.y;
										}
									}
								}
							}
						}
						if ( bindingPressed )
						{
							for ( auto& option : contextTooltipOptions )
							{
								// clear the other bindings just in case.
								Input::inputs[player].consumeBinaryToggle(getContextMenuOptionBindingName(player, option).c_str());
							}
							break;
						}
					}

					// handle clicking
					if ( Input::inputs[player].binaryToggle("MenuLeftClick") && !selectedItem && inventoryControlActive && inputs.bPlayerUsingKeyboardControl(player) )
					{
						inputs.getUIInteraction(player)->selectedItemFromHotbar = -1;
						inputs.getUIInteraction(player)->selectedItemFromChest = item->uid;
						selectedItem = item;
						playSound(139, 64); // click sound

						if ( inputs.getVirtualMouse(player)->draw_cursor )
						{
							cursor.lastUpdateTick = ticks;
						}

						toggleclick = false; //Default reset. Otherwise will break mouse support after using gamepad once to trigger a context menu.
					}
					else if ( Input::inputs[player].binaryToggle("MenuRightClick") && inputs.bPlayerUsingKeyboardControl(player)
						&& inventoryControlActive && !selectedItem )
					{
						// open a drop-down menu of options for "using" the item
						itemMenuOpen = true;
						itemMenuFromHotbar = false;
						itemMenuX = (inputs.getMouse(player, Inputs::X) / (float)xres) * (float)Frame::virtualScreenX + 8;
						itemMenuY = (inputs.getMouse(player, Inputs::Y) / (float)yres) * (float)Frame::virtualScreenY;
						if ( auto interactMenuTop = interactFrame->findImage("interact top background") )
						{
							// 10px is slot half height, minus the top interact text height
							// mouse will be situated halfway in first menu option
							itemMenuY -= (interactMenuTop->pos.h + 10 + 2);
						}
						if ( itemMenuX % 2 == 1 )
						{
							++itemMenuX; // even pixel adjustment
						}
						if ( itemMenuY % 2 == 1 )
						{
							++itemMenuY; // even pixel adjustment
						}
						itemMenuY = std::max(itemMenuY, players[player]->camera_virtualy1());

						bool alignRight = true;
						if ( bCompactView )
						{
							// normal inventory items
							if ( inventoryPanelJustify != PanelJustify_t::PANEL_JUSTIFY_RIGHT )
							{
								alignRight = true;
							}
							else
							{
								alignRight = false;
							}
						}
						if ( !alignRight )
						{
							itemMenuX -= 16;
						}

						if ( auto highlightImage = interactFrame->findImage("interact selected highlight") )
						{
							highlightImage->disabled = true;
						}
						itemMenuSelected = 0;
						itemMenuItem = item->uid;

						toggleclick = false; //Default reset. Otherwise will break mouse support after using gamepad once to trigger a context menu.

						if ( inputs.getVirtualMouse(player)->draw_cursor )
						{
							cursor.lastUpdateTick = ticks;
						}
					}
					break;
				}
			}
		}
	}

	if ( !selectedItem && !tinkerCraftableListOpen && !featherDrawerOpen )
	{
		for ( node = stats[player]->inventory.first; node != NULL; node = nextnode )
		{
			nextnode = node->next;
			Item* item = (Item*)node->element;
			if ( !item )
			{
				continue;
			}

			if ( GenericGUI[player].isGUIOpen() )
			{
				if ( !GenericGUI[player].isNodeFromPlayerInventory(item->node) )
				{
					continue;
				}
			}

			bool mouseOverSlot = false;

			bool itemOnPaperDoll = false;
			if ( players[player]->paperDoll.enabled && itemIsEquipped(item, player) )
			{
				auto slotType = players[player]->paperDoll.getSlotForItem(*item);
				if ( slotType != Player::PaperDoll_t::SLOT_MAX )
				{
					itemOnPaperDoll = true;
				}
			}

			int itemx = item->x;
			int itemy = item->y;
			if ( itemOnPaperDoll )
			{
				players[player]->paperDoll.getCoordinatesFromSlotType(players[player]->paperDoll.getSlotForItem(*item), itemx, itemy);
			}

			auto slotFrame = getItemSlotFrame(item, itemx, itemy);
			if ( !slotFrame ) { continue; }

			if ( (players[player]->inventory_mode == INVENTORY_MODE_ITEM && itemCategory(item) == SPELL_CAT)
				|| (players[player]->inventory_mode == INVENTORY_MODE_SPELL && itemCategory(item) != SPELL_CAT) )
			{
				continue;    //Skip over this item if not in inventory mode
			}

			mouseOverSlot = players[player]->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_INVENTORY)
				&& slotFrame->capturesMouse();

			if ( mouseOverSlot && inputs.getVirtualMouse(player)->draw_cursor && !(players[player]->GUI.isDropdownActive() && itemMenuFromHotbar) )
			{
				// mouse movement captures the inventory
				if ( itemCategory(item) == SPELL_CAT )
				{
					players[player]->GUI.activateModule(Player::GUI_t::MODULE_SPELLS);
				}
				else
				{
					players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
				}
			}

			if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_SPELLS
				&& (!spellPanel.isInteractable) && itemCategory(item) == SPELL_CAT )
			{
				// don't do anything while in motion
				break;
			}
			if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_INVENTORY
				&& !isInteractable && itemCategory(item) != SPELL_CAT )
			{
				// don't do anything while in motion
				break;
			}
			if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_CHEST
				&& !chestGUI.isInteractable )
			{
				// don't do anything while in motion
				break;
			}
			if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_TINKERING
				&& !tinkerGUI.isInteractable )
			{
				// don't do anything while in motion
				break;
			}
			if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_FEATHER
				&& !featherGUI.isInteractable )
			{
				// don't do anything while in motion
				break;
			}
			if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_SHOP
				&& (!shopGUI.isInteractable) )
			{
				// don't do anything while in motion
				break;
			}
			if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_ASSISTSHRINE
				&& (!assistShrineGUI.isInteractable) )
			{
				// don't do anything while in motion
				break;
			}
			if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_SPELLS
				&& itemCategory(item) == SPELL_CAT && !spellPanel.isItemVisible(item) )
			{
				// this item is obscured by the spell panel scroll, ignore.
				continue;
			}

			if ( mouseOverSlot && players[player]->GUI.bActiveModuleUsesInventory() )
			{
				auto inventoryBgFrame = playerInventoryFrames[player].inventoryBgFrame;
				if ( itemCategory(item) == SPELL_CAT )
				{
					inventoryBgFrame = spellFrame;
				}
				int tooltipCoordX = 0;
				PanelJustify_t justify = itemOnPaperDoll ? paperDollPanelJustify : inventoryPanelJustify;
				if ( !bCompactView )
				{
					Frame::image_t* invBaseImg = nullptr;
					if ( itemCategory(item) == SPELL_CAT )
					{
						invBaseImg = playerInventoryFrames[player].spellBaseImg;
					}
					else
					{
						invBaseImg = playerInventoryFrames[player].defaultInvImg;
					}

					if ( justify == PANEL_JUSTIFY_LEFT )
					{
						tooltipCoordX = inventoryBgFrame->getSize().x + 8;
						tooltipCoordX += invBaseImg->pos.w;
					}
					else
					{
						tooltipCoordX = inventoryBgFrame->getSize().x - 8;
						tooltipCoordX += invBaseImg->pos.x;
					}

					if ( alchemyGUI.alchFrame && alchemyGUI.bOpen && alchemyGUI.isInteractable )
					{
						if ( justify == PANEL_JUSTIFY_LEFT )
						{
							tooltipCoordX += alchemyGUI.alchFrame->getSize().w;
						}
						else
						{
							tooltipCoordX -= alchemyGUI.alchFrame->getSize().w;
						}
					}
				}
				else
				{
					Frame::image_t* compactImg = nullptr;
					if ( itemCategory(item) == SPELL_CAT )
					{
						compactImg = playerInventoryFrames[player].spellBaseImg;
					}
					else
					{
						compactImg = itemOnPaperDoll ?
							playerInventoryFrames[player].compactCharImg
							: playerInventoryFrames[player].compactInvImg;
					}

					if ( justify == PANEL_JUSTIFY_LEFT )
					{
						tooltipCoordX = inventoryBgFrame->getSize().x + 8;
						tooltipCoordX += compactImg->pos.w;
					}
					else
					{
						tooltipCoordX = inventoryBgFrame->getSize().x - 8;
						tooltipCoordX += compactImg->pos.x;
					}
				}
				int tooltipCoordY = slotFrame->getAbsoluteSize().y - players[player]->camera_virtualy1();
				if ( !itemOnPaperDoll )
				{
				//	tooltipCoordY += invSlotsFrame->getSize().y;
				}
				else
				{
				//	tooltipCoordY += dollSlotsFrame->getSize().y;
				}

				if ( appraisal.itemsToNotify.find(item->uid) != appraisal.itemsToNotify.end() )
				{
					if ( appraisal.itemsToNotify[item->uid] == Appraisal_t::NOTIFY_ITEM_WAITING_TO_HOVER )
					{
						appraisal.itemsToNotify[item->uid] = Appraisal_t::NOTIFY_ITEM_HOVERED;
					}
				}
				for ( auto& itemNotify : appraisal.itemsToNotify )
				{
					if ( itemNotify.first != item->uid && itemNotify.second == Appraisal_t::NOTIFY_ITEM_HOVERED )
					{
						itemNotify.second = Appraisal_t::NOTIFY_ITEM_REMOVE;
					}
				}

				bool tooltipOpen = false;
				bool sellingItemToShop = false;
				bool tinkerOpen = false;
				bool alchemyOpen = false;
				bool featherOpen = false;
				bool itemfxOpen = false;
				if ( featherInscribeOrRepairActive )
				{
					tooltipOpen = false;
					featherOpen = true;
					if ( !featherDrawerOpen )
					{
						featherGUI.setItemDisplayNameAndPrice(item, false);
					}
				}
				else if ( itemfxGUI.isItemEffectMenuActive() )
				{
					tooltipOpen = false;
					itemfxOpen = true;
					itemfxGUI.setItemDisplayNameAndPrice(item);
				}
				else if ( tinkeringSalvageOrRepairMenuActive )
				{
					tooltipOpen = false;
					tinkerOpen = true;
					tinkerGUI.setItemDisplayNameAndPrice(item);
				}
				else if ( shopGUI.isItemSelectedToSellToShop(item) )
				{
					sellingItemToShop = true;
					if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_INVENTORY )
					{
						shopGUI.setItemDisplayNameAndPrice(item);
					}
				}
				else if ( alchemyGUI.bOpen )
				{
					alchemyOpen = true;
					if ( itemCategory(item) == POTION && item->type != POTION_EMPTY )
					{
						alchemyGUI.setItemDisplayNameAndPrice(item, false, false);
					}
				}
				else
				{
					if ( inventoryControlActive && !bIsTooltipDelayed() )
					{
						tooltipOpen = true;
						players[player]->hud.updateFrameTooltip(item, tooltipCoordX, tooltipCoordY, justify);
					}
				}

				if ( stats[player]->HP <= 0 )
				{
					break;
				}

				bool tooltipPromptWasDisabled = tooltipPromptFrame->isDisabled();
				if ( useItemDropdownOnGamepad == GamepadDropdownTypes::GAMEPAD_DROPDOWN_FULL && !inputs.getVirtualMouse(player)->draw_cursor )
				{
					tooltipPromptFrame->setDisabled(true);
				}
				if ( ((tooltipOpen && (!tooltipPromptWasDisabled))
					|| bIsTooltipDelayed()
					|| sellingItemToShop
					|| tinkerOpen
					|| featherOpen
					|| itemfxOpen
					|| alchemyOpen)
					&& inventoryControlActive
					&& !selectedItem )
				{
					auto contextTooltipOptions = getContextTooltipOptionsForItem(player, item, useItemDropdownOnGamepad, false);
					bool bindingPressed = false;
					for ( auto& option : contextTooltipOptions )
					{
						if ( Input::inputs[player].binaryToggle(getContextMenuOptionBindingName(player, option).c_str()) )
						{
							bindingPressed = true;

							if ( Input::inputs[player].getPlayerControlType() == Input::PLAYER_CONTROLLED_BY_KEYBOARD )
							{
								// rare bug causes rogue activations on keyboard controls holding space + opening inventory
								// skip this section and consume presses
								break;
							}

							if ( option == ItemContextMenuPrompts::PROMPT_DROP && players[player]->paperDoll.isItemOnDoll(*item) )
							{
								// need to unequip
								players[player]->inventoryUI.activateItemContextMenuOption(item, ItemContextMenuPrompts::PROMPT_UNEQUIP_FOR_DROP);
								players[player]->paperDoll.updateSlots();
								if ( players[player]->paperDoll.isItemOnDoll(*item) )
								{
									// couldn't unequip, no more actions
								}
								else
								{
									// successfully unequipped, let's drop it.
									bool droppedAll = false;
									while ( item && item->count > 1 )
									{
										droppedAll = dropItem(item, player);
										if ( droppedAll )
										{
											item = nullptr;
										}
									}
									if ( !droppedAll )
									{
										dropItem(item, player);
									}
								}
							}
							else
							{
								activateItemContextMenuOption(item, option);
								if ( option == ItemContextMenuPrompts::PROMPT_DROPDOWN )
								{
									if ( !players[player]->GUI.isDropdownActive() )
									{
										std::string dropdownName = "item_interact";
										if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_SPELLS && itemCategory(item) == SPELL_CAT )
										{
											dropdownName = "spell_interact";
										}
										players[player]->GUI.dropdownMenu.open(dropdownName);
										players[player]->GUI.dropdownMenu.dropDownToggleClick = true;
										players[player]->GUI.dropdownMenu.dropDownItem = item->uid;
										SDL_Rect dropdownPos = slotFrame->getAbsoluteSize();
										dropdownPos.y += dropdownPos.h / 2;
										if ( auto interactMenuTop = players[player]->GUI.dropdownMenu.dropdownFrame->findImage("interact top background") )
										{
											// 10px is slot half height, move by 0.5 slots, minus the top interact text height
											dropdownPos.y -= (interactMenuTop->pos.h + (1 * 10) + 4);
										}

										if ( players[player]->GUI.dropdownMenu.getDropDownAlignRight(dropdownName) )
										{
											dropdownPos.x += dropdownPos.w - 10;
										}
										else
										{
											dropdownPos.x += 10;
										}
										dropdownPos.x = std::max(dropdownPos.x, players[player]->camera_virtualx1());
										dropdownPos.x = std::min(dropdownPos.x, players[player]->camera_virtualx1() + players[player]->camera_virtualWidth());
										players[player]->GUI.dropdownMenu.dropDownX = dropdownPos.x;
										dropdownPos.y = std::max(dropdownPos.y, players[player]->camera_virtualy1());
										dropdownPos.y = std::min(dropdownPos.y, players[player]->camera_virtualy1() + players[player]->camera_virtualHeight());
										players[player]->GUI.dropdownMenu.dropDownY = dropdownPos.y;
									}
								}
							}
							break;
						}
					}
					if ( bindingPressed )
					{
						for ( auto& option : contextTooltipOptions )
						{
							// clear the other bindings just in case.
							Input::inputs[player].consumeBinaryToggle(getContextMenuOptionBindingName(player, option).c_str());
						}
						break;
					}
				}

				bool disableItemUsage = false;
				if ( item )
				{
					if ( players[player] && players[player]->entity && players[player]->entity->effectShapeshift != NOTHING )
					{
						// shape shifted, disable some items
						if ( !item->usableWhileShapeshifted(stats[player]) )
						{
							disableItemUsage = true;
						}
					}
					if ( client_classes[player] == CLASS_SHAMAN )
					{
						if ( item->type == SPELL_ITEM && !(playerUnlockedShamanSpell(player, item)) )
						{
							disableItemUsage = true;
						}
					}
				}

				// handle clicking
				if ( Input::inputs[player].binaryToggle("MenuLeftClick") && !selectedItem && inventoryControlActive && inputs.bPlayerUsingKeyboardControl(player) )
				{
					if ( (keystatus[SDLK_LSHIFT] || keystatus[SDLK_RSHIFT]) )
					{
						if ( guiAllowDropItems(item) )
						{
							if ( players[player]->paperDoll.isItemOnDoll(*item) )
							{
								// need to unequip
								players[player]->inventoryUI.activateItemContextMenuOption(item, ItemContextMenuPrompts::PROMPT_UNEQUIP_FOR_DROP);
								players[player]->paperDoll.updateSlots();
								if ( players[player]->paperDoll.isItemOnDoll(*item) )
								{
									// couldn't unequip, no more actions
									Input::inputs[player].consumeBinaryToggle("MenuLeftClick");
								}
							}

							if ( Input::inputs[player].binaryToggle("MenuLeftClick") /*keystatus[SDLK_LCTRL] || keystatus[SDLK_RCTRL]*/ )
							{
								// drop all.
								int qty = item->count;
								bool droppedAll = false;
								bool unableToDropAll = false;
								while ( item && item->count > 1 )
								{
									droppedAll = dropItem(item, player);
									if ( droppedAll )
									{
										item = nullptr;
									}
									else if ( item->count == qty ) // couldn't drop
									{
										unableToDropAll = true;
										break;
									}
								}
								if ( !droppedAll && !unableToDropAll )
								{
									if ( dropItem(item, player) ) // Quick item drop
									{
										item = nullptr;
									}
								}
							}
							//else 
							//{
							//	if ( dropItem(item, player) ) // Quick item drop
							//	{
							//		item = nullptr;
							//	}
							//}
						}
					}
					else
					{
						inputs.getUIInteraction(player)->selectedItemFromHotbar = -1;
						inputs.getUIInteraction(player)->selectedItemFromChest = 0;
						selectedItem = item;
						playSound(139, 64); // click sound

						if ( inputs.getVirtualMouse(player)->draw_cursor )
						{
							cursor.lastUpdateTick = ticks;
						}

						toggleclick = false; //Default reset. Otherwise will break mouse support after using gamepad once to trigger a context menu.
					}
				}
				else if ( Input::inputs[player].binaryToggle("MenuRightClick") && inputs.bPlayerUsingKeyboardControl(player)
					&& inventoryControlActive && !selectedItem )
				{
					if ( keystatus[SDLK_LSHIFT] || keystatus[SDLK_RSHIFT] )
					{
						if ( guiAllowDefaultRightClick() )
						{
							// auto-appraise the item
							appraisal.appraiseItem(item);
							Input::inputs[player].consumeBinaryToggle("MenuRightClick");
						}
					}
					else if ( !disableItemUsage
						&& (itemCategory(item) == POTION || itemCategory(item) == SPELLBOOK || item->type == FOOD_CREAMPIE) &&
						(keystatus[SDLK_LALT] || keystatus[SDLK_RALT]) )
					{
						Input::inputs[player].consumeBinaryToggle("MenuRightClick");
						if ( guiAllowDefaultRightClick() )
						{
							// force equip potion/spellbook
							playerTryEquipItemAndUpdateServer(player, item, false);
						}
					}
					else if ( !tinkeringSalvageOrRepairMenuActive && !alchemyOpen && !featherInscribeOrRepairActive && !itemfxOpen )
					{
						// open a drop-down menu of options for "using" the item
						itemMenuOpen = true;
						itemMenuFromHotbar = false;
						itemMenuX = (inputs.getMouse(player, Inputs::X) / (float)xres) * (float)Frame::virtualScreenX + 8;
						itemMenuY = (inputs.getMouse(player, Inputs::Y) / (float)yres) * (float)Frame::virtualScreenY;
						if ( auto interactMenuTop = interactFrame->findImage("interact top background") )
						{
							// 10px is slot half height, minus the top interact text height
							// mouse will be situated halfway in first menu option
							itemMenuY -= (interactMenuTop->pos.h + 10 + 2);
						}
						if ( itemMenuX % 2 == 1 )
						{
							++itemMenuX; // even pixel adjustment
						}
						if ( itemMenuY % 2 == 1 )
						{
							++itemMenuY; // even pixel adjustment
						}
						itemMenuY = std::max(itemMenuY, players[player]->camera_virtualy1());

						bool alignRight = true;
						if ( bCompactView )
						{
							if ( players[player]->paperDoll.isItemOnDoll(*item) )
							{
								if ( paperDollPanelJustify == PanelJustify_t::PANEL_JUSTIFY_RIGHT )
								{
									alignRight = false;
								}
								else
								{
									alignRight = true;
								}
							}
							else if ( itemCategory(item) == SPELL_CAT )
							{
								if ( spellPanel.panelJustify == PanelJustify_t::PANEL_JUSTIFY_RIGHT )
								{
									alignRight = false;
								}
								else
								{
									alignRight = true;
								}
							}
							else
							{
								// normal inventory items
								if ( inventoryPanelJustify == PanelJustify_t::PANEL_JUSTIFY_RIGHT )
								{
									alignRight = false;
								}
								else
								{
									alignRight = true;
								}
							}
						}
						if ( !alignRight )
						{
							itemMenuX -= 16;
						}

						if ( auto highlightImage = interactFrame->findImage("interact selected highlight") )
						{
							highlightImage->disabled = true;
						}
						itemMenuSelected = 0;
						itemMenuItem = item->uid;

						toggleclick = false; //Default reset. Otherwise will break mouse support after using gamepad once to trigger a context menu.

						if ( inputs.getVirtualMouse(player)->draw_cursor )
						{
							cursor.lastUpdateTick = ticks;
						}
					}
				}

				bool numkey_quick_add = playerSettings[multiplayer ? 0 : player].hotbar_numkey_quick_add && inputs.bPlayerUsingKeyboardControl(player);
				if ( item && itemCategory(item) == SPELL_CAT && item->appearance >= 1000 &&
					players[player] && players[player]->entity && players[player]->entity->effectShapeshift )
				{
					if ( canUseShapeshiftSpellInCurrentForm(player, *item) != 1 )
					{
						numkey_quick_add = false;
					}
				}

				if ( numkey_quick_add && inventoryControlActive && item
					&& inputs.bPlayerUsingKeyboardControl(player) )
				{
					int slotNum = -1;
					if ( Input::inputs[player].binaryToggle("Hotbar Slot 1") )
					{
						Input::inputs[player].consumeBinaryToggle("Hotbar Slot 1");
						hotbar[0].item = item->uid;
						slotNum = 0;
					}
					if ( Input::inputs[player].binaryToggle("Hotbar Slot 2") )
					{
						Input::inputs[player].consumeBinaryToggle("Hotbar Slot 2");
						hotbar[1].item = item->uid;
						slotNum = 1;
					}
					if ( Input::inputs[player].binaryToggle("Hotbar Slot 3") )
					{
						Input::inputs[player].consumeBinaryToggle("Hotbar Slot 3");
						hotbar[2].item = item->uid;
						slotNum = 2;
					}
					if ( Input::inputs[player].binaryToggle("Hotbar Slot 4") )
					{
						Input::inputs[player].consumeBinaryToggle("Hotbar Slot 4");
						hotbar[3].item = item->uid;
						slotNum = 3;
					}
					if ( Input::inputs[player].binaryToggle("Hotbar Slot 5") )
					{
						Input::inputs[player].consumeBinaryToggle("Hotbar Slot 5");
						hotbar[4].item = item->uid;
						slotNum = 4;
					}
					if ( Input::inputs[player].binaryToggle("Hotbar Slot 6") )
					{
						Input::inputs[player].consumeBinaryToggle("Hotbar Slot 6");
						hotbar[5].item = item->uid;
						slotNum = 5;
					}
					if ( Input::inputs[player].binaryToggle("Hotbar Slot 7") )
					{
						Input::inputs[player].consumeBinaryToggle("Hotbar Slot 7");
						hotbar[6].item = item->uid;
						slotNum = 6;
					}
					if ( Input::inputs[player].binaryToggle("Hotbar Slot 8") )
					{
						Input::inputs[player].consumeBinaryToggle("Hotbar Slot 8");
						hotbar[7].item = item->uid;
						slotNum = 7;
					}
					if ( Input::inputs[player].binaryToggle("Hotbar Slot 9") )
					{
						Input::inputs[player].consumeBinaryToggle("Hotbar Slot 9");
						hotbar[8].item = item->uid;
						slotNum = 8;
					}
					if ( Input::inputs[player].binaryToggle("Hotbar Slot 10") 
						&& this->player.hotbar.getHotbarSlotFrame(9)
						&& !this->player.hotbar.getHotbarSlotFrame(9)->isDisabled() )
					{
						Input::inputs[player].consumeBinaryToggle("Hotbar Slot 10");
						hotbar[9].item = item->uid;
						slotNum = 9;
					}

					// empty out duplicate slots that match this item uid.
					if ( slotNum >= 0 )
					{
						int i = 0;
						for ( auto& s : players[player]->hotbar.slots() )
						{
							if ( i != slotNum && s.item == item->uid )
							{
								s.item = 0;
								s.resetLastItem();
							}
							++i;
						}
					}
				}
				break;
			}
			else
			{
				if ( appraisal.itemsToNotify.find(item->uid) != appraisal.itemsToNotify.end() )
				{
					if ( players[player]->GUI.bActiveModuleUsesInventory() && players[player]->GUI.activeModule != Player::GUI_t::MODULE_HOTBAR )
					{
						if ( appraisal.itemsToNotify[item->uid] == Appraisal_t::NOTIFY_ITEM_HOVERED )
						{
							appraisal.itemsToNotify[item->uid] = Appraisal_t::NOTIFY_ITEM_REMOVE;
						}
					}
				}
			}
		}
	}

	if ( !noPreviousSelectedItem && stats[player]->HP > 0 )
	{
		// releasing items
		Item* oldSelectedItem = selectedItem;
		Category cat = itemCategory(oldSelectedItem);
		int selectedSlotFrameX = 0;
		int selectedSlotFrameY = 0;
		Frame* oldSlotFrame = nullptr;
		bool foundOldSlot = false;
		
		if ( oldSelectedItem )
		{
			foundOldSlot = getSlotFrameXYFromMousePos(player, selectedSlotFrameX, selectedSlotFrameY, cat == SPELL_CAT);
			if ( foundOldSlot )
			{
				if ( this->player.GUI.activeModule == Player::GUI_t::MODULE_CHEST )
				{
					oldSlotFrame = getChestSlotFrame(selectedSlotFrameX, selectedSlotFrameY);
				}
				else if ( this->player.GUI.activeModule == Player::GUI_t::MODULE_SPELLS )
				{
					oldSlotFrame = getSpellSlotFrame(selectedSlotFrameX, selectedSlotFrameY);
				}
				else
				{
					oldSlotFrame = getInventorySlotFrame(selectedSlotFrameX, selectedSlotFrameY);
				}
			}
		}
		Uint32 oldUid = selectedItem ? selectedItem->uid : 0;
		bool oldQueuedCursor = !(cursor.queuedModule == Player::GUI_t::MODULE_NONE);

		releaseItem(player);

		// re-highlight the selected slots/remove old selected slots
		// otherwise we'll have a frame of drawing nothing
		if ( oldSelectedItem && !selectedItem && frame )
		{
			auto selectedSlotFrame = playerInventoryFrames[player].selectedSlotFrame;
			if ( foundOldSlot )
			{
				if ( auto slotFrame = oldSlotFrame )
				{
					int startx = slotFrame->getAbsoluteSize().x - players[player]->camera_virtualx1();
					int starty = slotFrame->getAbsoluteSize().y - players[player]->camera_virtualy1();

					selectedSlotFrame->setSize(SDL_Rect{ startx + 1, starty + 1,
						selectedSlotFrame->getSize().w, selectedSlotFrame->getSize().h });
					selectedSlotFrame->setDisabled(false);

					selectedSlotCursor->setDisabled(false);
					updateSelectedSlotAnimation(selectedSlotFrame->getSize().x - 1, selectedSlotFrame->getSize().y - 1, 
						getSlotSize(), getSlotSize(), inputs.getVirtualMouse(player)->draw_cursor);
					//messagePlayer(player, "6: %d, %d", selectedSlotFrameX, selectedSlotFrameY);
				}
			}
			else if ( hotbar_t.hotbarFrame )
			{
				if ( auto oldselectedSlotCursor = hotbar_t.hotbarFrame->findFrame("hotbar old item cursor") )
				{
					oldselectedSlotCursor->setDisabled(true);
				}

				if ( cursor.queuedModule == Player::GUI_t::MODULE_NONE && !oldQueuedCursor )
				{
					if ( players[player]->GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_HOTBAR) )
					{
						for ( int c = 0; c < NUM_HOTBAR_SLOTS; ++c )
						{
							auto hotbarSlotFrame = hotbar_t.getHotbarSlotFrame(c);
							if ( hotbarSlotFrame && !hotbarSlotFrame->isDisabled() && hotbarSlotFrame->capturesMouseInRealtimeCoords() )
							{
								players[player]->hotbar.selectHotbarSlot(c);

								selectedSlotCursor->setDisabled(false);

								int startx = hotbarSlotFrame->getAbsoluteSize().x - players[player]->camera_virtualx1();
								int starty = hotbarSlotFrame->getAbsoluteSize().y - players[player]->camera_virtualy1();

								updateSelectedSlotAnimation(startx - 1, starty - 1,
									hotbar_t.getSlotSize() - 2, hotbar_t.getSlotSize() - 2, inputs.getVirtualMouse(player)->draw_cursor);
								//messagePlayer(player, "7: hotbar: %d", c);
								break;
							}
						}
					}
				}
			}

			auto oldSelectedSlotFrame = playerInventoryFrames[player].oldSelectedSlotFrame;
			oldSelectedSlotFrame->setDisabled(true);

			// update the displayed item frames - to catch any changes
			for ( node = stats[player]->inventory.first; node != NULL; node = nextnode )
			{
				nextnode = node->next;
				Item* item = (Item*)node->element;
				if ( !item ) { continue; }

				int itemx = item->x;
				int itemy = item->y;

				bool itemOnPaperDoll = false;
				if ( players[player]->paperDoll.enabled && itemIsEquipped(item, player) )
				{
					auto slotType = players[player]->paperDoll.getSlotForItem(*item);
					if ( slotType != Player::PaperDoll_t::SLOT_MAX )
					{
						itemOnPaperDoll = true;
					}
				}

				if ( itemOnPaperDoll )
				{
					players[player]->paperDoll.getCoordinatesFromSlotType(players[player]->paperDoll.getSlotForItem(*item), itemx, itemy);
				}

				if ( itemCategory(item) == SPELL_CAT )
				{
					if ( auto slotFrame = getItemSlotFrame(item, itemx, itemy) )
					{
						updateSlotFrameFromItem(slotFrame, item);
					}
				}
				else if ( itemx >= 0 && itemx < getSizeX()
					&& itemy >= Player::Inventory_t::PaperDollRows::DOLL_ROW_1 && itemy < getSizeY() )
				{
					if ( itemx >= 2 && itemx <= Player::Inventory_t::PaperDollRows::DOLL_ROW_5 ) { continue; }
					if ( auto slotFrame = getInventorySlotFrame(itemx, itemy) )
					{
						if ( shopOpen && !isItemSellableToShop(player, item) )
						{
							updateSlotFrameFromItem(slotFrame, item, true); // force grey backgrounds
						}
						else if ( featherInscribeOrRepairActive )
						{
							auto res = featherGUI.setItemDisplayNameAndPrice(item, true);
							if ( res == GenericGUIMenu::FeatherGUI_t::FEATHER_ACTION_OK )
							{
								updateSlotFrameFromItem(slotFrame, item);
							}
							else
							{
								updateSlotFrameFromItem(slotFrame, item, true); // force grey backgrounds
							}
						}
						else if ( itemfxGUI.isItemEffectMenuActive() )
						{
							auto res = itemfxGUI.setItemDisplayNameAndPrice(item, true);
							if ( res == GenericGUIMenu::ItemEffectGUI_t::ITEMFX_ACTION_OK )
							{
								updateSlotFrameFromItem(slotFrame, item);
							}
							else
							{
								updateSlotFrameFromItem(slotFrame, item, true); // force grey backgrounds
							}
						}
						else if ( tinkerCraftableListOpen || tinkeringSalvageOrRepairMenuActive )
						{
							auto res = tinkerGUI.setItemDisplayNameAndPrice(item, true);
							bool invalidItem = !(res == GenericGUIMenu::TinkerGUI_t::TINKER_ACTION_OK
								|| res == GenericGUIMenu::TinkerGUI_t::TINKER_ACTION_OK_UPGRADE);
							if ( tinkerCraftableListOpen && GenericGUI[player].isNodeTinkeringCraftableItem(item->node) )
							{
								if ( invalidItem )
								{
									updateSlotFrameFromItem(slotFrame, item, true); // force grey backgrounds
								}
								else
								{
									updateSlotFrameFromItem(slotFrame, item);
								}
							}
							else if ( tinkeringSalvageOrRepairMenuActive
								&& GenericGUI[player].tinkeringFilter == GenericGUIMenu::TINKER_FILTER_REPAIRABLE
								&& invalidItem )
							{
								updateSlotFrameFromItem(slotFrame, item, true); // force grey backgrounds
							}
							else if ( tinkeringSalvageOrRepairMenuActive
								&& GenericGUI[player].tinkeringFilter == GenericGUIMenu::TINKER_FILTER_SALVAGEABLE
								&& invalidItem )
							{
								updateSlotFrameFromItem(slotFrame, item, true); // force grey backgrounds
							}
							else
							{
								updateSlotFrameFromItem(slotFrame, item);
							}
						}
						else if ( alchemyGUI.bOpen )
						{
							if ( !(itemCategory(item) == POTION && item->type != POTION_EMPTY /*&& item != GenericGUI[player].alembicItem*/
								&& item->identified && !itemIsEquipped(item, player)) )
							{
								updateSlotFrameFromItem(slotFrame, item, true);
							}
							else if ( alchemyGUI.potionResultUid == item->uid
								&& alchemyGUI.potionResultUid != alchemyGUI.potion1Uid
								&& alchemyGUI.potionResultUid != alchemyGUI.potion2Uid )
							{
								// set qty to 
								int oldCount = item->count;
								item->count = std::max(0, item->count - alchemyGUI.animPotionResultCount);
								if ( item->count > 0 )
								{
									slotFrame->setUserData(&GAMEUI_FRAMEDATA_ALCHEMY_ITEM);
									updateSlotFrameFromItem(slotFrame, item);
								}
								item->count = oldCount;
							}
							else if ( (alchemyGUI.potion1Uid == item->uid || alchemyGUI.potion2Uid == item->uid) )
							{
								slotFrame->setUserData(&GAMEUI_FRAMEDATA_ALCHEMY_ITEM);
								updateSlotFrameFromItem(slotFrame, item);
							}
							else
							{
								updateSlotFrameFromItem(slotFrame, item, !item->identified);
							}
						}
						else
						{
							updateSlotFrameFromItem(slotFrame, item);
						}
					}
				}
			}

			if ( !selectedItem && chestGUI.bOpen && openedChest[player] && chestFrame )
			{
				list_t* chest_inventory = nullptr;
				if ( multiplayer == CLIENT )
				{
					chest_inventory = &chestInv[player];
				}
				else if ( openedChest[player]->children.first && openedChest[player]->children.first->element )
				{
					chest_inventory = (list_t*)openedChest[player]->children.first->element;
				}

				if ( chest_inventory )
				{
					for ( node = chest_inventory->first; node != NULL; node = nextnode )
					{
						nextnode = node->next;
						Item* item = (Item*)node->element;
						if ( !item ) { continue; }

						int itemx = item->x;
						int itemy = item->y;

						if ( itemx >= 0 && itemx < MAX_CHEST_X
							&& itemy >= 0 && itemy < MAX_CHEST_Y )
						{
							if ( auto slotFrame = getItemSlotFrame(item, itemx, itemy) )
							{
								updateSlotFrameFromItem(slotFrame, item);
							}
						}
					}
				}
			}
		}

		if ( hotbar_t.hotbarFrame ) 
		{
			if ( auto oldSelectedItemFrame = hotbar_t.hotbarFrame->findFrame("hotbar old selected item") )
			{
				if ( !selectedItem )
				{
					// hide the 'old' transparent item frame
					oldSelectedItemFrame->setDisabled(true);
				}
				if ( selectedItem && oldSelectedItem )
				{
					if ( oldUid != 0 && selectedItem->uid != oldUid )
					{
						// if swapped item, then the old item image is outdated, hide it, but still show cursor
						if ( auto oldImg = oldSelectedItemFrame->findImage("hotbar old selected item") )
						{
							oldImg->disabled = true;
						}
					}
				}
			}
		}
	}
	updateItemContextMenu();
}

void Player::PaperDoll_t::warpMouseToMostRecentReturnedInventoryItem()
{
	if ( returningItemsToInventory.empty() || !enabled )
	{
		return;
	}
	Uint32 uid = returningItemsToInventory[returningItemsToInventory.size() - 1]; // get last item and select it's new position
	Item* item = uidToItem(uid);
	if ( item && !isItemOnDoll(*item) )
	{
		if ( item->x >= 0 && item->x < player.inventoryUI.getSizeX()
			&& item->y >= 0 && item->y < player.inventoryUI.getSizeY() )
		{
			player.inventoryUI.selectSlot(item->x, item->y);
			player.inventoryUI.warpMouseToSelectedItem(nullptr, (Inputs::SET_CONTROLLER));
		}
	}
	returningItemsToInventory.clear();
}

void Player::PaperDoll_t::updateSlots()
{
	returningItemsToInventory.clear();
	if ( !stats[player.playernum] || !player.isLocalPlayer() )
	{
		return;
	}

	for ( auto& slot : dollSlots )
	{
		Uint32 prevSlot = slot.item;
		slot.item = 0;
		if ( !enabled )
		{
			continue;
		}

		Item* equippedItem = nullptr;

		switch ( slot.slotType )
		{
			case SLOT_GLASSES:
				if ( equippedItem = stats[player.playernum]->mask )
				{
					slot.item = equippedItem->uid;
				}
				break;
			case SLOT_CLOAK:
				if ( equippedItem = stats[player.playernum]->cloak )
				{
					slot.item = equippedItem->uid;
				}
				break;
			case SLOT_AMULET:
				if ( equippedItem = stats[player.playernum]->amulet )
				{
					slot.item = equippedItem->uid;
				}
				break;
			case SLOT_RING:
				if ( equippedItem = stats[player.playernum]->ring )
				{
					slot.item = equippedItem->uid;
				}
				break;
			case SLOT_OFFHAND:
				if ( equippedItem = stats[player.playernum]->shield )
				{
					slot.item = equippedItem->uid;
				}
				break;
			case SLOT_HELM:
				if ( equippedItem = stats[player.playernum]->helmet )
				{
					slot.item = equippedItem->uid;
				}
				break;
			case SLOT_BREASTPLATE:
				if ( equippedItem = stats[player.playernum]->breastplate )
				{
					slot.item = equippedItem->uid;
				}
				break;
			case SLOT_GLOVES:
				if ( equippedItem = stats[player.playernum]->gloves )
				{
					slot.item = equippedItem->uid;
				}
				break;
			case SLOT_BOOTS:
				if ( equippedItem = stats[player.playernum]->shoes )
				{
					slot.item = equippedItem->uid;
				}
				break;
			case SLOT_WEAPON:
				if ( equippedItem = stats[player.playernum]->weapon )
				{
					slot.item = equippedItem->uid;
				}
				break;
			default:
				break;
		}

		if ( prevSlot != slot.item )
		{
			Item* prevItem = uidToItem(prevSlot);
			if ( prevItem )
			{
				// item is returning to inventory
				prevItem->x = Player::PaperDoll_t::ITEM_RETURN_TO_INVENTORY_COORDINATE;
				prevItem->y = Player::PaperDoll_t::ITEM_RETURN_TO_INVENTORY_COORDINATE;
				returningItemsToInventory.push_back(prevSlot);

			}
		}
	}

	if ( !returningItemsToInventory.empty() )
	{
		autosortInventory(player.playernum, true);
	}
}

std::string getContextMenuOptionBindingName(const int player, const ItemContextMenuPrompts prompt)
{
	switch ( prompt )
	{
		case PROMPT_EQUIP:
		case PROMPT_UNEQUIP:
		case PROMPT_SPELL_EQUIP:
		case PROMPT_UNEQUIP_FOR_DROP:
		case PROMPT_RETRIEVE_CHEST_ALL:
		case PROMPT_STORE_CHEST_ALL:
		case PROMPT_CONSUME_ALTERNATE:
		case PROMPT_INSPECT_ALTERNATE:
			if ( players[player]->inventoryUI.useItemDropdownOnGamepad == Player::Inventory_t::GAMEPAD_DROPDOWN_COMPACT )
			{
				return "MenuConfirm";
			}
			else
			{
				return "MenuAlt2";
			}
		case PROMPT_GRAB:
			return "MenuAlt1";
		case PROMPT_TINKER:
		case PROMPT_INTERACT:
		case PROMPT_INTERACT_SPELLBOOK_HOTBAR:
		case PROMPT_EAT:
		case PROMPT_SPELL_QUICKCAST:
		case PROMPT_CONSUME:
		case PROMPT_SELL:
		case PROMPT_BUY:
		case PROMPT_STORE_CHEST:
		case PROMPT_RETRIEVE_CHEST:
		case PROMPT_INSPECT:
		case PROMPT_DROPDOWN:
			return "MenuConfirm";
		case PROMPT_DROP:
			if ( players[player]->inventoryUI.useItemDropdownOnGamepad == Player::Inventory_t::GAMEPAD_DROPDOWN_COMPACT )
			{
				return "MenuAlt2";
			}
			else
			{
				return "MenuCancel";
			}
		case PROMPT_APPRAISE:
			return "InventoryTooltipPromptAppraise";
		default:
			return "";
	}
}

const char* getContextMenuLangEntry(const int player, const ItemContextMenuPrompts prompt, Item& item)
{
	switch ( prompt )
	{
		case PROMPT_EQUIP:
		case PROMPT_UNEQUIP:
		case PROMPT_SPELL_EQUIP:
			return itemEquipString(player, item);
		case PROMPT_INTERACT:
		case PROMPT_INTERACT_SPELLBOOK_HOTBAR:
		case PROMPT_EAT:
			return itemUseString(player, item);
		case PROMPT_SPELL_QUICKCAST:
			return Language::get(4049);
		case PROMPT_TINKER:
			return Language::get(3670);
		case PROMPT_CLEAR_HOTBAR_SLOT:
			return Language::get(3723);
		case PROMPT_APPRAISE:
			return Language::get(1161);
		case PROMPT_CONSUME:
		case PROMPT_CONSUME_ALTERNATE:
			return Language::get(3487);
		case PROMPT_INSPECT:
		case PROMPT_INSPECT_ALTERNATE:
			return Language::get(1881);
		case PROMPT_SELL:
			return Language::get(345);
			break;
		case PROMPT_BUY:
			break;
		case PROMPT_STORE_CHEST:
			return Language::get(344);
		case PROMPT_RETRIEVE_CHEST:
			return Language::get(4096);
		case PROMPT_RETRIEVE_CHEST_ALL:
			return Language::get(4091);
		case PROMPT_STORE_CHEST_ALL:
			return Language::get(4097);
		case PROMPT_DROP:
			return Language::get(1162);
		case PROMPT_GRAB:
			return Language::get(4050);
		case PROMPT_DROPDOWN:
			return Language::get(4040);
		default:
			return "Invalid";
	}
	return "Invalid";
}

std::vector<ItemContextMenuPrompts> getContextTooltipOptionsForItem(const int player, Item* item, int useDropdownMenu, bool hotbarItem)
{
	if ( stats[player] && stats[player]->HP <= 0 )
	{
		// ded, cant do anything with items
		return std::vector<ItemContextMenuPrompts>();
	}
	std::vector<ItemContextMenuPrompts> options;
	if ( useDropdownMenu == Player::Inventory_t::GAMEPAD_DROPDOWN_FULL )
	{
		options.push_back(ItemContextMenuPrompts::PROMPT_DROPDOWN);
		options.push_back(ItemContextMenuPrompts::PROMPT_GRAB);
	}
	else if ( useDropdownMenu == Player::Inventory_t::GAMEPAD_DROPDOWN_COMPACT )
	{
		options = getContextMenuOptionsForItem(player, item);
		options.push_back(ItemContextMenuPrompts::PROMPT_GRAB); // additional prompt here for non-right click menu
		auto findAppraise = std::find(options.begin(), options.end(), ItemContextMenuPrompts::PROMPT_APPRAISE);
		if ( findAppraise != options.end() )
		{
			options.erase(findAppraise);
		}

		int numPrimaryOptions = 0;
		for ( auto it = options.begin(); it != options.end(); )
		{
			if ( getContextMenuOptionBindingName(player, *it) == "MenuConfirm" )
			{
				++numPrimaryOptions;
			}
			++it;
		}
		if ( numPrimaryOptions >= 2 || hotbarItem )
		{
			for ( auto it = options.begin(); it != options.end(); )
			{
				if ( getContextMenuOptionBindingName(player, *it) == "MenuConfirm" )
				{
					it = options.erase(it);
					continue;
				}
				++it;
			}
			options.push_back(PROMPT_DROPDOWN);
		}
	}
	else
	{
		options = getContextMenuOptionsForItem(player, item);
		options.push_back(ItemContextMenuPrompts::PROMPT_GRAB); // additional prompt here for non-right click menu
		auto findAppraise = std::find(options.begin(), options.end(), ItemContextMenuPrompts::PROMPT_APPRAISE);
		if ( findAppraise != options.end() )
		{
			options.erase(findAppraise);
		}
	}
	return options;
}

std::vector<ItemContextMenuPrompts> getContextMenuOptionsForItem(const int player, Item* item)
{
	std::vector<ItemContextMenuPrompts> options;
	if ( !item )
	{
		return options;
	}

	bool playerOwnedItem = false;
	if ( item->node )
	{
		if ( item->node->list == &stats[player]->inventory )
		{
			playerOwnedItem = true;
		}
	}

	if ( openedChest[player] && itemCategory(item) != SPELL_CAT )
	{
		if ( playerOwnedItem )
		{
			options.push_back(PROMPT_STORE_CHEST);
			options.push_back(PROMPT_STORE_CHEST_ALL);
			options.push_back(PROMPT_DROP);
		}
		else if ( players[player]->inventoryUI.isItemFromChest(item) )
		{
			options.push_back(PROMPT_RETRIEVE_CHEST);
			options.push_back(PROMPT_RETRIEVE_CHEST_ALL);
		}
		return options;
	}

	if ( itemCategory(item) == SPELL_CAT )
	{
		options.push_back(PROMPT_SPELL_EQUIP);
		options.push_back(PROMPT_SPELL_QUICKCAST);
		return options;
	}

	if ( itemCategory(item) == POTION )
	{
		bool is_potion_bad = isPotionBad(*item);
		if ( is_potion_bad )
		{
			options.push_back(PROMPT_EQUIP);
			if ( !itemIsEquipped(item, player) )
			{
				options.push_back(PROMPT_EAT);
			}
			options.push_back(PROMPT_APPRAISE);
			options.push_back(PROMPT_DROP);
		}
		else
		{
			if ( !itemIsEquipped(item, player) )
			{
				options.push_back(PROMPT_EAT);
			}
			options.push_back(PROMPT_EQUIP);
			options.push_back(PROMPT_APPRAISE);
			options.push_back(PROMPT_DROP);
		}
	}
	else if ( item->type == TOOL_ALEMBIC )
	{
		options.push_back(PROMPT_INTERACT);
		options.push_back(PROMPT_APPRAISE);
		options.push_back(PROMPT_DROP);
	}
	else if ( item->type == TOOL_PLAYER_LOOT_BAG )
	{
		options.push_back(PROMPT_INSPECT);
		options.push_back(PROMPT_INTERACT);
		options.push_back(PROMPT_APPRAISE);
		options.push_back(PROMPT_DROP);
	}
	else if ( item->type == TOOL_TINKERING_KIT )
	{
		options.push_back(PROMPT_TINKER);
		options.push_back(PROMPT_EQUIP);
		options.push_back(PROMPT_APPRAISE);
		options.push_back(PROMPT_DROP);
	}
	else if ( item->type == FOOD_CREAMPIE )
	{
		if ( !itemIsEquipped(item, player) )
		{
			if ( stats[player] && stats[player]->type == AUTOMATON && itemIsConsumableByAutomaton(*item) )
			{
				options.push_back(PROMPT_CONSUME);
			}
			else
			{
				options.push_back(PROMPT_EAT);
			}
		}
		options.push_back(PROMPT_EQUIP);
		options.push_back(PROMPT_APPRAISE);
		options.push_back(PROMPT_DROP);
	}
	else if ( stats[player] && stats[player]->type == AUTOMATON && itemIsConsumableByAutomaton(*item) )
	{
		if ( item->type == TOOL_METAL_SCRAP || item->type == TOOL_MAGIC_SCRAP )
		{
			options.push_back(PROMPT_CONSUME_ALTERNATE);
			options.push_back(PROMPT_INSPECT);
			options.push_back(PROMPT_APPRAISE);
			options.push_back(PROMPT_DROP);
		}
		else if ( itemCategory(item) == FOOD )
		{
			if ( !itemIsEquipped(item, player) )
			{
				options.push_back(PROMPT_CONSUME);
			}
			options.push_back(PROMPT_APPRAISE);
			options.push_back(PROMPT_DROP);
		}
		else if ( itemCategory(item) == GEM )
		{
			options.push_back(PROMPT_EQUIP);
			if ( !itemIsEquipped(item, player) )
			{
				options.push_back(PROMPT_CONSUME);
			}
			options.push_back(PROMPT_APPRAISE);
			options.push_back(PROMPT_DROP);
		}
		else
		{
			options.push_back(PROMPT_INTERACT);
			options.push_back(PROMPT_CONSUME_ALTERNATE);
			options.push_back(PROMPT_APPRAISE);
			options.push_back(PROMPT_DROP);
		}
	}
	else if ( itemCategory(item) == SPELLBOOK )
	{
		bool learnedSpell = playerLearnedSpellbook(player, item);
		if ( itemIsEquipped(item, player) )
		{
			learnedSpell = true; // equipped spellbook will unequip on use.
		}
		else if ( stats[player] 
			&& (stats[player]->type == GOBLIN
				|| (stats[player]->playerRace == RACE_GOBLIN && stats[player]->stat_appearance == 0)) )
		{
			// goblinos can't learn spells but always equip books.
			learnedSpell = true; 
		}
		else if ( players[player] && players[player]->entity )
		{
			if ( players[player]->entity->effectShapeshift == CREATURE_IMP )
			{
				learnedSpell = true; // imps can't learn spells but always equip books.
			}
		}
		if ( learnedSpell )
		{
			options.push_back(PROMPT_EQUIP);
			options.push_back(PROMPT_INTERACT);
		}
		else
		{
			options.push_back(PROMPT_INTERACT);
			options.push_back(PROMPT_EQUIP);
		}
		options.push_back(PROMPT_APPRAISE);
		options.push_back(PROMPT_DROP);
	}
	else if ( items[item->type].item_slot != ItemEquippableSlot::NO_EQUIP )
	{
		options.push_back(PROMPT_EQUIP);
		options.push_back(PROMPT_APPRAISE);
		options.push_back(PROMPT_DROP);
	}
	else
	{
		options.push_back(PROMPT_INTERACT);
		options.push_back(PROMPT_APPRAISE);
		options.push_back(PROMPT_DROP);
	}

	bool sellingToShop = false;
	bool tinkerOpen = false;
	bool alembicOpen = false;
	bool featherOpen = false;
	bool itemfxOpen = false;
	if ( players[player]->gui_mode == GUI_MODE_SHOP && itemCategory(item) != SPELL_CAT )
	{
		if ( playerOwnedItem )
		{
			sellingToShop = true;
		}
	}
	else if ( GenericGUI[player].tinkerGUI.bOpen )
	{
		tinkerOpen = true;
	}
	else if ( GenericGUI[player].alchemyGUI.bOpen )
	{
		alembicOpen = true;
	}
	else if ( GenericGUI[player].featherGUI.bOpen )
	{
		featherOpen = true;
	}
	else if ( GenericGUI[player].itemfxGUI.bOpen )
	{
		itemfxOpen = true;
	}

	for ( auto it = options.begin(); it != options.end(); )
	{
		if ( sellingToShop || tinkerOpen || alembicOpen || featherOpen || itemfxOpen )
		{
			if ( getContextMenuOptionBindingName(player, *it) == "MenuConfirm"
				|| getContextMenuOptionBindingName(player, *it) == "MenuCancel" )
			{
				it = options.erase(it);
				continue;
			}
			if ( (tinkerOpen || featherOpen) && *it == PROMPT_DROP ) // no drop items in feather or tinker as they have inventory hacks
			{
				it = options.erase(it);
				continue;
			}
			if ( alembicOpen && getContextMenuOptionBindingName(player, *it) == "MenuAlt2" )
			{
				it = options.erase(it);
				continue;
			}
			if ( featherOpen && getContextMenuOptionBindingName(player, *it) == "MenuAlt2" )
			{
				it = options.erase(it);
				continue;
			}
			if ( *it == PROMPT_CONSUME_ALTERNATE || *it == PROMPT_INSPECT_ALTERNATE )
			{
				it = options.erase(it);
				continue;
			}
		}
		if ( *it == PROMPT_EQUIP )
		{
			if ( itemIsEquipped(item, player) )
			{
				*it = PROMPT_UNEQUIP;
			}
		}
		if ( *it == PROMPT_APPRAISE )
		{
			if ( item->identified )
			{
				it = options.erase(it);
				continue;
			}
		}
		++it;
	}

	if ( sellingToShop )
	{
		options.insert(options.begin(), PROMPT_SELL);
	}

#ifndef NDEBUG
	std::unordered_map<std::string, int> optionsMap;
	for ( auto it = options.begin(); it != options.end(); ++it )
	{
		optionsMap[getContextMenuOptionBindingName(player, *it)] += 1;
	}
	for ( auto& pair : optionsMap )
	{
		if ( players[player]->inventoryUI.useItemDropdownOnGamepad != Player::Inventory_t::GAMEPAD_DROPDOWN_COMPACT )
		{
			assert(pair.second <= 1);
		}
	}
#endif // NDEBUG

	return options;
}

inline bool itemMenuSkipRow1ForShopsAndChests(const int player, const Item& item)
{
	if ( (openedChest[player] || players[player]->gui_mode == GUI_MODE_SHOP)
		&& (itemCategory(&item) == POTION || item.type == TOOL_ALEMBIC || item.type == TOOL_TINKERING_KIT || itemCategory(&item) == SPELLBOOK) )
	{
		return true;
	}
	return false;
}

inline void drawItemMenuOptionAutomaton(const int player, const Item& item, int x, int y, int height, bool is_potion_bad)
{
	return;
	int width = 0;

	//Option 0.
	if ( openedChest[player] )
	{
		//drawOptionStoreInChest(x, y);
	}
	else if ( players[player]->gui_mode == GUI_MODE_SHOP )
	{
		//drawOptionSell(x, y);
	}
	else
	{
		if ( !is_potion_bad )
		{
			if ( !itemIsConsumableByAutomaton(item) || (itemCategory(&item) != FOOD && item.type != TOOL_METAL_SCRAP && item.type != TOOL_MAGIC_SCRAP) )
			{
				//drawOptionUse(player, item, x, y);
			}
			else
			{
				getSizeOfText(ttf12, Language::get(3487), &width, nullptr);
				ttfPrintText(ttf12, x + 50 - width / 2, y + 4, Language::get(3487));
			}
		}
		else
		{
			if ( itemIsEquipped(&item, player) )
			{
				//drawOptionUnwield(x, y);
			}
			else
			{
				//drawOptionWield(x, y);
			}
		}
	}
	y += height;

	//Option 1.
	if ( item.type == TOOL_METAL_SCRAP || item.type == TOOL_MAGIC_SCRAP )
	{
		getSizeOfText(ttf12, Language::get(1881), &width, nullptr);
		ttfPrintText(ttf12, x + 50 - width / 2, y + 4, Language::get(1881));
		y += height;
	}
	else if ( itemCategory(&item) != FOOD )
	{
		getSizeOfText(ttf12, Language::get(3487), &width, nullptr);
		ttfPrintText(ttf12, x + 50 - width / 2, y + 4, Language::get(3487));
		y += height;
	}

	//Option 1.
	//drawOptionAppraise(x, y);
	y += height;

	//Option 2.
	//drawOptionDrop(x, y);
}

// filters out items excluded by auto_hotbar_categories
bool autoAddHotbarFilter(const Item& item)
{
	Category cat = itemCategory(&item);
	for ( int i = 0; i < NUM_HOTBAR_CATEGORIES; ++i )
	{
		if ( auto_hotbar_categories[i] )
		{
			switch ( i )
			{
				case 0: // weapons
					if ( cat == WEAPON )
					{
						return true;
					}
					break;
				case 1: // armor
					if ( cat == ARMOR )
					{
						return true;
					}
					break;
				case 2: // jewelry
					if ( cat == RING || cat == AMULET )
					{
						return true;
					}
					break;
				case 3: // books/spellbooks
					if ( /*cat == BOOK ||*/ cat == SPELLBOOK )
					{
						return true;
					}
					break;
				case 4: // tools
					if ( item.type == POTION_EMPTY )
					{
						return true;
					}
					else if ( cat == TOOL )
					{
						switch ( item.type )
						{
							case ARTIFACT_ORB_BLUE:
							case ARTIFACT_ORB_RED:
							case ARTIFACT_ORB_GREEN:
							case ARTIFACT_ORB_PURPLE:
							case TOOL_LOCKPICK:
							case TOOL_TORCH:
							case TOOL_LANTERN:
							case TOOL_CRYSTALSHARD:
								return true;
								break;
							default:
								break;
						}
					}
					break;
				case 5: // thrown
					if ( cat == THROWN || item.type == GEM_ROCK || itemTypeIsQuiver(item.type)
						|| itemIsThrowableTinkerTool(&item) || item.type == TOOL_BEARTRAP )
					{
						return true;
					}
					break;
				case 6: // gems
					if ( cat == GEM && item.type != GEM_ROCK )
					{
						return true;
					}
					break;
				case 7: // potions
					if ( cat == POTION && item.type != POTION_EMPTY )
					{
						return true;
					}
					break;
				case 8: // scrolls
					if ( cat == SCROLL )
					{
						return true;
					}
					break;
				case 9: // magicstaves
					if ( cat == MAGICSTAFF )
					{
						return true;
					}
					break;
				case 10: // food
					if ( cat == FOOD )
					{
						return true;
					}
					break;
				case 11: // spells
					if ( cat == SPELL_CAT )
					{
						return true;
					}
					break;
				default:
					break;
			}
		}
	}
	return false;
}

void quickStackItems(int player)
{
	for ( node_t* node = stats[player]->inventory.first; node != NULL; node = node->next )
	{
		Item* itemToStack = (Item*)node->element;
		if ( itemToStack && itemToStack->shouldItemStack(player) )
		{
			for ( node_t* node = stats[player]->inventory.first; node != NULL; node = node->next )
			{
				Item* item2 = (Item*)node->element;
				// if items are the same, check to see if they should stack
				if ( item2 && item2 != itemToStack && !itemCompare(itemToStack, item2, false) )
				{
					itemToStack->count += item2->count;
					if ( multiplayer == CLIENT && itemIsEquipped(itemToStack, player) )
					{
						// if incrementing qty and holding item, then send "equip" for server to update their count of your held item.
						clientSendEquipUpdateToServer(EQUIP_ITEM_SLOT_WEAPON, EQUIP_ITEM_SUCCESS_UPDATE_QTY, player,
							itemToStack->type, itemToStack->status,	itemToStack->beatitude, itemToStack->count, itemToStack->appearance, itemToStack->identified);
					}
					if ( item2->node )
					{
						list_RemoveNode(item2->node);
					}
					else
					{
						free(item2);
						item2 = nullptr;
					}
				}
			}
		}
	}
}

void autosortInventory(int player, bool sortPaperDoll)
{
	std::vector<std::pair<int, int>> autosortPairs;
	for ( int i = 0; i < NUM_AUTOSORT_CATEGORIES; ++i )
	{
		autosortPairs.push_back(std::make_pair(autosort_inventory_categories[i], i));
	}

	for ( node_t* node = stats[player]->inventory.first; node != NULL; node = node->next )
	{
		Item* item = (Item*)node->element;
		if ( item )
		{
			if ( sortPaperDoll )
			{
				// don't assign any items
			}
			else if ( (!itemIsEquipped(item, player) || (autosort_inventory_categories[11] != 0 && !players[player]->paperDoll.enabled)) 
				&& itemCategory(item) != SPELL_CAT )
			{
				item->x = -1;
				item->y = 0;
				// move all items away.
			}
		}
	}

	std::sort(autosortPairs.begin(), autosortPairs.end());

	// iterate and sort from highest to lowest priority, 1 to 9
	for ( std::vector<std::pair<int, int>>::reverse_iterator it = autosortPairs.rbegin(); it != autosortPairs.rend(); ++it )
	{
		std::pair<int, int> tmpPair = *it;
		if ( tmpPair.first > 0 )
		{
			//messagePlayer(0, "priority %d, category: %d", tmpPair.first, tmpPair.second);
			bool invertSortDirection = true;
			switch ( tmpPair.second )
			{
				case 0: // weapons
					sortInventoryItemsOfType(player, WEAPON, invertSortDirection);
					break;
				case 1: // armor
					sortInventoryItemsOfType(player, ARMOR, invertSortDirection);
					break;
				case 2: // jewelry
					sortInventoryItemsOfType(player, RING, invertSortDirection);
					sortInventoryItemsOfType(player, AMULET, invertSortDirection);
					break;
				case 3: // books/spellbooks
					sortInventoryItemsOfType(player, SPELLBOOK, invertSortDirection);
					sortInventoryItemsOfType(player, BOOK, invertSortDirection);
					break;
				case 4: // tools
					sortInventoryItemsOfType(player, TOOL, invertSortDirection);
					break;
				case 5: // thrown
					sortInventoryItemsOfType(player, THROWN, invertSortDirection);
					break;
				case 6: // gems
					sortInventoryItemsOfType(player, GEM, invertSortDirection);
					break;
				case 7: // potions
					sortInventoryItemsOfType(player, POTION, invertSortDirection);
					break;
				case 8: // scrolls
					sortInventoryItemsOfType(player, SCROLL, invertSortDirection);
					break;
				case 9: // magicstaves
					sortInventoryItemsOfType(player, MAGICSTAFF, invertSortDirection);
					break;
				case 10: // food
					sortInventoryItemsOfType(player, FOOD, invertSortDirection);
					break;
				case 11: // equipped items
					//sortInventoryItemsOfType(player, -2, invertSortDirection);
					break;
				default:
					break;
			}
		}
	}

	// iterate and sort from lowest to highest priority, -9 to -1
	for ( std::vector<std::pair<int, int>>::iterator it = autosortPairs.begin(); it != autosortPairs.end(); ++it )
	{
		std::pair<int, int> tmpPair = *it;
		if ( tmpPair.first < 0 )
		{
			//messagePlayer(0, "priority %d, category: %d", tmpPair.first, tmpPair.second);
			bool invertSortDirection = false;
			switch ( tmpPair.second )
			{
				case 0: // weapons
					sortInventoryItemsOfType(player, WEAPON, invertSortDirection);
					break;
				case 1: // armor
					sortInventoryItemsOfType(player, ARMOR, invertSortDirection);
					break;
				case 2: // jewelry
					sortInventoryItemsOfType(player, RING, invertSortDirection);
					sortInventoryItemsOfType(player, AMULET, invertSortDirection);
					break;
				case 3: // books/spellbooks
					sortInventoryItemsOfType(player, SPELLBOOK, invertSortDirection);
					sortInventoryItemsOfType(player, BOOK, invertSortDirection);
					break;
				case 4: // tools
					sortInventoryItemsOfType(player, TOOL, invertSortDirection);
					break;
				case 5: // thrown
					sortInventoryItemsOfType(player, THROWN, invertSortDirection);
					break;
				case 6: // gems
					sortInventoryItemsOfType(player, GEM, invertSortDirection);
					break;
				case 7: // potions
					sortInventoryItemsOfType(player, POTION, invertSortDirection);
					break;
				case 8: // scrolls
					sortInventoryItemsOfType(player, SCROLL, invertSortDirection);
					break;
				case 9: // magicstaves
					sortInventoryItemsOfType(player, MAGICSTAFF, invertSortDirection);
					break;
				case 10: // food
					sortInventoryItemsOfType(player, FOOD, invertSortDirection);
					break;
				case 11: // equipped items
					//sortInventoryItemsOfType(player, -2, invertSortDirection);
					break;
				default:
					break;
			}
		}
	}


	sortInventoryItemsOfType(player, -1, true); // clean up the rest of the items.
}

bool Player::Inventory_t::moveItemToFreeInventorySlot(Item* item)
{
	bool notfree = false;
	bool is_spell = itemCategory(item) == SPELL_CAT;
	bool foundaspot = false;

	if ( is_spell )
	{
		int x = 0;
		int y = 0;
		while ( true )
		{
			for ( x = 0; x < Player::Inventory_t::MAX_SPELLS_X; x++ )
			{
				for ( node_t* node = stats[player.playernum]->inventory.first; node != nullptr; node = node->next )
				{
					Item* tempItem = static_cast<Item*>(node->element);
					if ( tempItem == item )
					{
						continue;
					}
					if ( tempItem )
					{
						if ( tempItem->x == x && tempItem->y == y )
						{
							if ( is_spell && itemCategory(tempItem) == SPELL_CAT )
							{
								notfree = true;  //Both spells. Can't fit in the same slot.
							}
							else if ( !is_spell && itemCategory(tempItem) != SPELL_CAT )
							{
								notfree = true;  //Both not spells. Can't fit in the same slot.
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
				return true;
				break;
			}
			y++;
		}
		return false;
	}
	else
	{
		if ( multiplayer != CLIENT )
		{
			if ( stats[player.playernum]->cloak
				&& stats[player.playernum]->cloak->type == CLOAK_BACKPACK 
				&& stats[player.playernum]->cloak->status != BROKEN
				&& (shouldInvertEquipmentBeatitude(stats[player.playernum]) ? abs(stats[player.playernum]->cloak->beatitude) >= 0 : stats[player.playernum]->cloak->beatitude >= 0) )
			{
				setSizeY(DEFAULT_INVENTORY_SIZEY + getPlayerBackpackBonusSizeY());
			}
		}
		else if ( multiplayer == CLIENT )
		{
			if ( !players[player.playernum]->isLocalPlayer() )
			{
				return false;
			}
			if ( stats[player.playernum]->cloak 
				&& stats[player.playernum]->cloak->type == CLOAK_BACKPACK
				&& stats[player.playernum]->cloak->status != BROKEN
				&& (shouldInvertEquipmentBeatitude(stats[player.playernum]) ? abs(stats[player.playernum]->cloak->beatitude) >= 0 : stats[player.playernum]->cloak->beatitude >= 0) )
			{
				setSizeY(DEFAULT_INVENTORY_SIZEY + getPlayerBackpackBonusSizeY());
			}
		}

		int x = 0;
		int y = 0;
		const int sort_y = std::min(getSizeY(), DEFAULT_INVENTORY_SIZEY);
		while ( true )
		{
			for ( y = 0; y < sort_y; y++ )
			{
				for ( node_t* node = stats[player.playernum]->inventory.first; node != nullptr; node = node->next )
				{
					Item* tempItem = static_cast<Item*>(node->element);
					if ( tempItem == item )
					{
						continue;
					}
					if ( tempItem )
					{
						if ( tempItem->x == x && tempItem->y == y )
						{
							if ( is_spell && itemCategory(tempItem) == SPELL_CAT )
							{
								notfree = true;  //Both spells. Can't fit in the same slot.
							}
							else if ( !is_spell && itemCategory(tempItem) != SPELL_CAT )
							{
								notfree = true;  //Both not spells. Can't fit in the same slot.
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

		if ( foundaspot && x < getSizeX() && y < getSizeY() )
		{
			item->x = -1;
			autosortInventory(player.playernum, true);
			return true;
		}
		foundaspot = false;

		// backpack sorting, sort into here as last priority.
		if ( x > getSizeX() - 1 && getSizeY() > DEFAULT_INVENTORY_SIZEY )
		{
			x = 0;
			foundaspot = false;
			notfree = false;
			while ( true )
			{
				for ( y = DEFAULT_INVENTORY_SIZEY; y < getSizeY(); y++ )
				{
					for ( node_t* node = stats[player.playernum]->inventory.first; node != nullptr; node = node->next )
					{
						Item* tempItem = static_cast<Item*>(node->element);
						if ( tempItem == item )
						{
							continue;
						}
						if ( tempItem )
						{
							if ( tempItem->x == x && tempItem->y == y )
							{
								if ( is_spell && itemCategory(tempItem) == SPELL_CAT )
								{
									notfree = true;  //Both spells. Can't fit in the same slot.
								}
								else if ( !is_spell && itemCategory(tempItem) != SPELL_CAT )
								{
									notfree = true;  //Both not spells. Can't fit in the same slot.
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
					if ( x < getSizeX() && y < getSizeY() )
					{
						item->x = -1;
						autosortInventory(player.playernum, true);
						return true;
					}
					else
					{
						foundaspot = false;
						break;
					}
				}
				x++;
			}
		}

		if ( !foundaspot )
		{
			return false;
		}
	}
	return false;
}

void sortInventoryItemsOfType(int player, int categoryInt, bool sortRightToLeft)
{
	node_t* node = nullptr;
	Item* itemBeingSorted = nullptr;
	Category cat = static_cast<Category>(categoryInt);

	for ( node = stats[player]->inventory.first; node != NULL; node = node->next )
	{
		itemBeingSorted = (Item*)node->element;
		if ( itemBeingSorted && (itemBeingSorted->x == -1 || itemBeingSorted->x == Player::PaperDoll_t::ITEM_RETURN_TO_INVENTORY_COORDINATE) )
		{
			if ( itemCategory(itemBeingSorted) == SPELL_CAT )
			{
				continue;
			}
			if ( categoryInt != -1 && categoryInt != -2 && itemCategory(itemBeingSorted) != cat )
			{
				if ( (itemBeingSorted->type == GEM_ROCK && categoryInt == THROWN) )
				{
					// exception for rocks as they are part of the thrown sort category...
				}
				else
				{
					// if item is not in the category specified, continue on.
					continue;
				}
			}
			if ( categoryInt == -2 && !itemIsEquipped(itemBeingSorted, player) )
			{
				continue;
			}
			if ( categoryInt != -2 && itemIsEquipped(itemBeingSorted, player) )
			{
				Item** slot = itemSlot(stats[player], itemBeingSorted);
				if ( slot && (*slot == itemBeingSorted) )
				{
					continue;
				}
			}

			// find a place...
			int x, y;
			bool notfree = false, foundaspot = false;

			bool is_spell = false;
			int inventory_y = std::min(players[player]->inventoryUI.getSizeY(), players[player]->inventoryUI.DEFAULT_INVENTORY_SIZEY); // only sort y values of 2-3, if extra row don't auto sort into it.

			if ( sortRightToLeft )
			{
				x = players[player]->inventoryUI.getSizeX() - 1; // fill rightmost first.
			}
			else
			{
				x = 0; // fill leftmost first.
			}
			while ( 1 )
			{
				for ( y = 0; y < inventory_y; y++ )
				{
					node_t* node2 = nullptr;
					for ( node2 = stats[player]->inventory.first; node2 != nullptr; node2 = node2->next )
					{
						Item* tempItem = (Item*)node2->element;
						if ( tempItem == itemBeingSorted )
						{
							continue;
						}
						if ( tempItem )
						{
							if ( tempItem->x == x && tempItem->y == y )
							{
								if ( is_spell && itemCategory(tempItem) == SPELL_CAT )
								{
									notfree = true;  //Both spells. Can't fit in the same slot.
								}
								else if ( !is_spell && itemCategory(tempItem) != SPELL_CAT )
								{
									notfree = true;  //Both not spells. Can't fit in the same slot.
								}
							}
						}
					}
					if ( notfree )
					{
						notfree = false;
						continue;
					}
					itemBeingSorted->x = x;
					itemBeingSorted->y = y;
					foundaspot = true;
					break;
				}
				if ( foundaspot )
				{
					break;
				}
				if ( sortRightToLeft )
				{
					--x; // fill rightmost first.
				}
				else
				{
					++x; // fill leftmost first.
				}
			}

			// backpack sorting, sort into here as last priority.
			if ( (x < 0 || x > players[player]->inventoryUI.getSizeX() - 1) && players[player]->inventoryUI.getSizeY() > players[player]->inventoryUI.DEFAULT_INVENTORY_SIZEY )
			{
				foundaspot = false;
				notfree = false;
				if ( sortRightToLeft )
				{
					x = players[player]->inventoryUI.getSizeX() - 1; // fill rightmost first.
				}
				else
				{
					x = 0; // fill leftmost first.
				}
				while ( 1 )
				{
					for ( y = players[player]->inventoryUI.DEFAULT_INVENTORY_SIZEY; y < players[player]->inventoryUI.getSizeY(); y++ )
					{
						node_t* node2 = nullptr;
						for ( node2 = stats[player]->inventory.first; node2 != nullptr; node2 = node2->next )
						{
							Item* tempItem = (Item*)node2->element;
							if ( tempItem == itemBeingSorted )
							{
								continue;
							}
							if ( tempItem )
							{
								if ( tempItem->x == x && tempItem->y == y )
								{
									if ( is_spell && itemCategory(tempItem) == SPELL_CAT )
									{
										notfree = true;  //Both spells. Can't fit in the same slot.
									}
									else if ( !is_spell && itemCategory(tempItem) != SPELL_CAT )
									{
										notfree = true;  //Both not spells. Can't fit in the same slot.
									}
								}
							}
						}
						if ( notfree )
						{
							notfree = false;
							continue;
						}
						itemBeingSorted->x = x;
						itemBeingSorted->y = y;
						foundaspot = true;
						break;
					}
					if ( foundaspot )
					{
						break;
					}
					if ( sortRightToLeft )
					{
						--x; // fill rightmost first.
					}
					else
					{
						++x; // fill leftmost first.
					}
				}
			}
		}
	}
}

bool mouseInsidePlayerInventory(const int player)
{
	if ( players[player]->inventoryUI.frame
		&& !players[player]->inventoryUI.frame->isDisabled() )
	{
		if ( auto invSlots = players[player]->inventoryUI.frame->findFrame("inventory slots") )
		{
			if ( !invSlots->isDisabled() && invSlots->capturesMouse() )
			{
				return true;
			}
		}
		if ( auto dollSlots = players[player]->inventoryUI.frame->findFrame("paperdoll slots") )
		{
			if ( !dollSlots->isDisabled() && dollSlots->capturesMouse() )
			{
				return true;
			}
		}
		if ( auto backpackSlots = players[player]->inventoryUI.frame->findFrame("backpack slots") )
		{
			if ( !backpackSlots->isDisabled() && backpackSlots->capturesMouse() )
			{
				return true;
			}
		}
	}
	return false;
}

bool mouseInsidePlayerHotbar(const int player)
{
	if ( players[player]->hotbar.hotbarFrame
		&& !players[player]->hotbar.hotbarFrame->isDisabled() )
	{
		for ( int num = 0; num < NUM_HOTBAR_SLOTS; ++num )
		{
			if ( auto slotFrame = players[player]->hotbar.getHotbarSlotFrame(num) )
			{
				if ( !slotFrame->isDisabled() && slotFrame->capturesMouse() )
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool playerLearnedSpellbook(int player, Item* current_item)
{
	if ( !current_item )
	{
		return false;
	}
	if ( itemCategory(current_item) != SPELLBOOK )
	{
		return false;
	}
	for ( node_t* node = stats[player]->inventory.first; node && current_item->identified; node = node->next )
	{
		Item* item = static_cast<Item*>(node->element);
		if ( !item )
		{
			continue;
		}
		//Search player's inventory for the special spell item.
		if ( itemCategory(item) != SPELL_CAT )
		{
			continue;
		}
		if ( item->appearance >= 1000 )
		{
			// special shaman racial spells, don't count this as being learnt
			continue;
		}
		spell_t *spell = getSpellFromItem(player, item, false); //Do not free or delete this.
		if ( !spell )
		{
			continue;
		}
		if ( current_item->type == getSpellbookFromSpellID(spell->ID) )
		{
			// learned spell, default option is now equip spellbook.
			return true;
		}
	}
	return false;
}
