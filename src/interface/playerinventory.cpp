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
#include "../ui/GameUI.hpp"
#include "../ui/Frame.hpp"
#include "../ui/Image.hpp"
#include "../ui/Field.hpp"
#include "../ui/Text.hpp"
#include "../mod_tools.hpp"
#ifdef STEAMWORKS
#include <steam/steam_api.h>
#include "../steam.hpp"
#endif

//Prototype helper functions for player inventory helper functions.
void itemContextMenu(const int player);
bool restrictPaperDollMovement = true;
SDL_Surface* inventory_mode_item_img = NULL;
SDL_Surface* inventory_mode_item_highlighted_img = NULL;
SDL_Surface* inventory_mode_spell_img = NULL;
SDL_Surface* inventory_mode_spell_highlighted_img = NULL;

void executeItemMenuOption0(const int player, Item* item, bool is_potion_bad, bool learnedSpell);
bool executeItemMenuOption0ForPaperDoll(const int player, Item* item)
{
	//TODO UI: VERIFY
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

	if ( !players[player]->inventoryUI.bItemInventoryHasFreeSlot() )
	{
		// no backpack space
		messagePlayer(player, language[3997], item->getName());
		return false;
	}

	players[player]->gui_mode = GUI_MODE_INVENTORY;
	openedChest[player] = nullptr;

	players[player]->inventoryUI.activateItemContextMenuOption(item, ItemContextMenuPrompts::PROMPT_UNEQUIP);
	//executeItemMenuOption0(player, item, isBadPotion, learnedSpell);

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
	//TODO UI: VERIFY
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
	//	messagePlayer(player, language[3997], item->getName());
	//	return false;
	//}

	Entity* oldChest = openedChest[player];
	int oldGUI = players[player]->gui_mode;

	//executeItemMenuOption0(player, item, isBadPotion, learnedSpell);
	players[player]->inventoryUI.activateItemContextMenuOption(item, ItemContextMenuPrompts::PROMPT_UNEQUIP);

	players[player]->paperDoll.updateSlots();
	if ( players[player]->paperDoll.isItemOnDoll(*item) )
	{
		return true;
	}
	// cursed or couldn't unequip existing item
	return false;
}

void warpMouseToSelectedInventorySlot(const int player)
{
	if ( players[player]->inventoryUI.warpMouseToSelectedItem(nullptr, (Inputs::SET_CONTROLLER)) )
	{
		return;
	}

	// TODO UI: REMOVE DEBUG AND CLEAN UP
	messagePlayer(0, "[Debug]: warpMouseToSelectedInventorySlot failed");

	//int xres = players[player]->camera_width();
	//int yres = players[player]->camera_height();
	//int x = players[player]->inventoryUI.getSelectedSlotPositionX(nullptr);
	//int y = players[player]->inventoryUI.getSelectedSlotPositionY(nullptr);
	//
	//Uint32 flags = (Inputs::SET_MOUSE | Inputs::SET_CONTROLLER);
	//inputs.warpMouse(player, x, y, flags);

	//SDL_WarpMouseInWindow(screen, 
	//	INVENTORY_STARTX + (selected_inventory_slot_x * INVENTORY_SLOTSIZE) + (INVENTORY_SLOTSIZE / 2), 
	//	INVENTORY_STARTY + (selected_inventory_slot_y * INVENTORY_SLOTSIZE) + (INVENTORY_SLOTSIZE / 2));
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
			return language[323];
		}
		else
		{
			return language[324];
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
					return language[325];
				}
				else
				{
					return language[326];
				}
			default:
				break;
		}
		if ( itemIsEquipped(&item, player) )
		{
			return language[327];
		}
		else
		{
			return language[328];
		}
	}
	else if ( itemCategory(&item) == AMULET )
	{
		if ( itemIsEquipped(&item, player) )
		{
			return language[327];
		}
		else
		{
			return language[328];
		}
	}
	else if ( itemCategory(&item) == POTION )
	{
		if ( itemIsEquipped(&item, player) )
		{
			return language[323];
		}
		else
		{
			return language[324];
		}
	}
	else if ( itemCategory(&item) == MAGICSTAFF )
	{
		if ( itemIsEquipped(&item, player) )
		{
			return language[323];
		}
		else
		{
			return language[324];
		}
	}
	else if ( itemCategory(&item) == RING )
	{
		if ( itemIsEquipped(&item, player) )
		{
			return language[327];
		}
		else
		{
			return language[331];
		}
	}
	else if ( itemCategory(&item) == SPELLBOOK )
	{
		if ( itemIsEquipped(&item, player) )
		{
			return language[323];
		}
		else
		{
			return language[324];
		}
	}
	else if ( itemCategory(&item) == GEM )
	{
		if ( itemIsEquipped(&item, player) )
		{
			return language[323];
		}
		else
		{
			return language[324];
		}
	}
	else if ( itemCategory(&item) == THROWN )
	{
		if ( itemIsEquipped(&item, player) )
		{
			return language[323];
		}
		else
		{
			return language[324];
		}
	}
	else if ( itemCategory(&item) == TOOL )
	{
		switch ( item.type )
		{
			case TOOL_PICKAXE:
				if ( itemIsEquipped(&item, player) )
				{
					return language[323];
				}
				else
				{
					return language[324];
				}
			case TOOL_LOCKPICK:
			case TOOL_SKELETONKEY:
				if ( itemIsEquipped(&item, player) )
				{
					return language[333];
				}
				else
				{
					return language[334];
				}
			case TOOL_TORCH:
			case TOOL_LANTERN:
			case TOOL_CRYSTALSHARD:
				if ( itemIsEquipped(&item, player) )
				{
					return language[335];
				}
				else
				{
					return language[336];
				}
			case TOOL_BLINDFOLD:
				if ( itemIsEquipped(&item, player) )
				{
					return language[327];
				}
				else
				{
					return language[328];
				}
			case TOOL_GLASSES:
				if ( itemIsEquipped(&item, player) )
				{
					return language[327];
				}
				else
				{
					return language[331];
				}
			default:
				if ( itemIsEquipped(&item, player) )
				{
					return language[323];
				}
				else
				{
					return language[324];
				}
		}
	}
	else if ( itemCategory(&item) == FOOD )
	{
		if ( itemIsEquipped(&item, player) )
		{
			return language[323];
		}
		else
		{
			return language[324];
		}
	}
	else if ( itemCategory(&item) == SPELL_CAT )
	{
		return language[339];
	}

	return "Invalid";
}

const char* itemUseString(int player, const Item& item)
{
	if ( itemCategory(&item) == POTION )
	{
		return language[329];
	}
	else if ( itemCategory(&item) == SCROLL )
	{
		return language[330];
	}
	else if ( itemCategory(&item) == SPELLBOOK )
	{
		return language[330];
	}
	else if ( itemCategory(&item) == TOOL )
	{
		switch ( item.type )
		{
			case TOOL_TINOPENER:
				return language[1881];
			case TOOL_MIRROR:
				return language[332];
			case TOOL_TOWEL:
				return language[332];
			case TOOL_BEARTRAP:
				return language[337];
			case TOOL_ALEMBIC:
				return language[3339];
			case TOOL_METAL_SCRAP:
			case TOOL_MAGIC_SCRAP:
				return language[1881];
				break;
			default:
				break;
		}
	}
	else if ( itemCategory(&item) == FOOD )
	{
		return language[338];
	}
	else if ( itemCategory(&item) == BOOK )
	{
		return language[330];
	}
	return language[332];
}

// deprecated
//char* itemUseStringOld(int player, const Item* item)
//{
//	if ( itemCategory(item) == WEAPON )
//	{
//		if ( itemIsEquipped(item, player) )
//		{
//			return language[323];
//		}
//		else
//		{
//			return language[324];
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
//					return language[325];
//				}
//				else
//				{
//					return language[326];
//				}
//			default:
//				break;
//		}
//		if ( itemIsEquipped(item, player) )
//		{
//			return language[327];
//		}
//		else
//		{
//			return language[328];
//		}
//	}
//	else if ( itemCategory(item) == AMULET )
//	{
//		if ( itemIsEquipped(item, player) )
//		{
//			return language[327];
//		}
//		else
//		{
//			return language[328];
//		}
//	}
//	else if ( itemCategory(item) == POTION )
//	{
//		return language[329];
//	}
//	else if ( itemCategory(item) == SCROLL )
//	{
//		return language[330];
//	}
//	else if ( itemCategory(item) == MAGICSTAFF )
//	{
//		if ( itemIsEquipped(item, player) )
//		{
//			return language[323];
//		}
//		else
//		{
//			return language[324];
//		}
//	}
//	else if ( itemCategory(item) == RING )
//	{
//		if ( itemIsEquipped(item, player) )
//		{
//			return language[327];
//		}
//		else
//		{
//			return language[331];
//		}
//	}
//	else if ( itemCategory(item) == SPELLBOOK )
//	{
//		return language[330];
//	}
//	else if ( itemCategory(item) == GEM )
//	{
//		if ( itemIsEquipped(item, player) )
//		{
//			return language[323];
//		}
//		else
//		{
//			return language[324];
//		}
//	}
//	else if ( itemCategory(item) == THROWN )
//	{
//		if ( itemIsEquipped(item, player) )
//		{
//			return language[323];
//		}
//		else
//		{
//			return language[324];
//		}
//	}
//	else if ( itemCategory(item) == TOOL )
//	{
//		switch ( item->type )
//		{
//			case TOOL_PICKAXE:
//				if ( itemIsEquipped(item, player) )
//				{
//					return language[323];
//				}
//				else
//				{
//					return language[324];
//				}
//			case TOOL_TINOPENER:
//				return language[1881];
//			case TOOL_MIRROR:
//				return language[332];
//			case TOOL_LOCKPICK:
//			case TOOL_SKELETONKEY:
//				if ( itemIsEquipped(item, player) )
//				{
//					return language[333];
//				}
//				else
//				{
//					return language[334];
//				}
//			case TOOL_TORCH:
//			case TOOL_LANTERN:
//			case TOOL_CRYSTALSHARD:
//				if ( itemIsEquipped(item, player) )
//				{
//					return language[335];
//				}
//				else
//				{
//					return language[336];
//				}
//			case TOOL_BLINDFOLD:
//				if ( itemIsEquipped(item, player) )
//				{
//					return language[327];
//				}
//				else
//				{
//					return language[328];
//				}
//			case TOOL_TOWEL:
//				return language[332];
//			case TOOL_GLASSES:
//				if ( itemIsEquipped(item, player) )
//				{
//					return language[327];
//				}
//				else
//				{
//					return language[331];
//				}
//			case TOOL_BEARTRAP:
//				return language[337];
//			case TOOL_ALEMBIC:
//				return language[3339];
//			case TOOL_METAL_SCRAP:
//			case TOOL_MAGIC_SCRAP:
//				return language[1881];
//				break;
//			default:
//				break;
//		}
//	}
//	else if ( itemCategory(item) == FOOD )
//	{
//		return language[338];
//	}
//	else if ( itemCategory(item) == BOOK )
//	{
//		return language[330];
//	}
//	else if ( itemCategory(item) == SPELL_CAT )
//	{
//		return language[339];
//	}
//	return language[332];
//}

/*-------------------------------------------------------------------------------

	updateAppraisalItemBox

	draws the current item being appraised

-------------------------------------------------------------------------------*/

//TODO UI: PORT
void updateAppraisalItemBox(const int player)
{
	SDL_Rect pos;
	Item* item;

	int x = 0; //players[player]->inventoryUI.getStartX();
	int y = 0; //players[player]->inventoryUI.getStartY();

	Player::Inventory_t::Appraisal_t& appraisal_t = players[player]->inventoryUI.appraisal;

	// appraisal item box
	if ( (item = uidToItem(appraisal_t.current_item)) != NULL
		&& appraisal_t.timer > 0 )
	{
		if ( !players[player]->shootmode )
		{
			pos.x = x + 16;
			pos.y = y + players[player]->inventoryUI.getSizeY() * players[player]->inventoryUI.getSlotSize() + 16;
		}
		else
		{
			pos.x = players[player]->camera_x1() + 16;
			pos.y = players[player]->camera_y1() + 16;
		}
		int w1, w2;
		getSizeOfText(ttf12, language[340], &w1, NULL);
		getSizeOfText(ttf12, item->getName(), &w2, NULL);
		w2 += 48;
		pos.w = std::max(w1, w2) + 8;
		pos.h = 68;
		drawTooltip(&pos);

		char tempstr[64] = { 0 };
		snprintf(tempstr, 63, language[341], 
			(((double)(appraisal_t.timermax - appraisal_t.timer)) / ((double)appraisal_t.timermax)) * 100);
		ttfPrintText( ttf12, pos.x + 8, pos.y + 8, tempstr );
		if ( !players[player]->shootmode )
		{
			pos.x = x + 24;
			pos.y = y + players[player]->inventoryUI.getSizeY() * players[player]->inventoryUI.getSlotSize() + 16 + 24;
		}
		else
		{
			pos.x = players[player]->camera_x1() + 24;
			pos.y = players[player]->camera_y1() + 16 + 24;
		}
		ttfPrintText( ttf12, pos.x + 40, pos.y + 8, item->getName() );
		pos.w = 32;
		pos.h = 32;
		drawImageScaled(itemSprite(item), NULL, &pos);
	}
}

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

void drawItemTooltip(const int player, Item* item, SDL_Rect& src)
{
	if ( !item )
	{
		return;
	}

	std::string tooltipString = "";
	char tooltipBuffer[1024];

	src.w = std::max(13, longestline(item->description())) * TTF12_WIDTH + 8;
	src.h = TTF12_HEIGHT * 4 + 8;
	char spellEffectText[256] = "";
	if ( item->identified )
	{
		bool learnedSpellbook = false;
		if ( itemCategory(item) == SPELLBOOK )
		{
			learnedSpellbook = playerLearnedSpellbook(player, item);
			if ( !learnedSpellbook && stats[player] && players[player] && players[player]->entity )
			{
				// spellbook tooltip shows if you have the magic requirement as well (for goblins)
				int skillLVL = stats[player]->PROFICIENCIES[PRO_MAGIC] + statGetINT(stats[player], players[player]->entity);
				spell_t* spell = getSpellFromID(getSpellIDFromSpellbook(item->type));
				if ( spell && skillLVL >= spell->difficulty )
				{
					learnedSpellbook = true;
				}
			}
		}

		if ( itemCategory(item) == WEAPON || itemCategory(item) == ARMOR || itemCategory(item) == THROWN
			|| itemTypeIsQuiver(item->type) )
		{
			src.h += TTF12_HEIGHT;
		}
		else if ( itemCategory(item) == SCROLL && item->identified )
		{
			src.h += TTF12_HEIGHT;
			src.w = std::max((2 + longestline(language[3862]) + longestline(item->getScrollLabel())) * TTF12_WIDTH + 8, src.w);
		}
		else if ( itemCategory(item) == SPELLBOOK && learnedSpellbook )
		{
			int height = 1;
			char effectType[32] = "";
			int spellID = getSpellIDFromSpellbook(item->type);
			int damage = drawSpellTooltip(player, getSpellFromID(spellID), item, nullptr);
			real_t dummy = 0.f;
			getSpellEffectString(spellID, spellEffectText, effectType, damage, &height, &dummy);
			int width = longestline(spellEffectText) * TTF12_WIDTH + 8;
			if ( width > src.w )
			{
				src.w = width;
			}
			src.h += height * TTF12_HEIGHT;
		}
		else if ( item->type == TOOL_GYROBOT || item->type == TOOL_DUMMYBOT
			|| item->type == TOOL_SENTRYBOT || item->type == TOOL_SPELLBOT
			|| (item->type == ENCHANTED_FEATHER && item->identified) )
		{
			src.w += 7 * TTF12_WIDTH;
		}
	}
	int furthestX = players[player]->camera_x2();
	if ( players[player]->characterSheet.proficienciesPage == 0 )
	{
		if ( src.y < players[player]->characterSheet.skillsSheetBox.y + players[player]->characterSheet.skillsSheetBox.h )
		{
			furthestX = players[player]->camera_x2() - players[player]->characterSheet.skillsSheetBox.w;
		}
	}
	else
	{
		if ( src.y < players[player]->characterSheet.partySheetBox.y + players[player]->characterSheet.partySheetBox.h )
		{
			furthestX = players[player]->camera_x2() - players[player]->characterSheet.partySheetBox.w;
		}
	}
	if ( src.x + src.w + 16 > furthestX ) // overflow right side of screen
	{
		src.x -= (src.w + 32);
	}

	drawTooltip(&src);

	Uint32 color = 0xFFFFFFFF;
	if ( !item->identified )
	{
		color = SDL_MapRGB(mainsurface->format, 255, 255, 0);
		ttfPrintTextFormattedColor(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT, color, language[309]);
		tooltipString += language[309];
		tooltipString += "\r\n";
	}
	else
	{
		if ( item->beatitude < 0 )
		{
			//Red if cursed
			color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
			ttfPrintTextFormattedColor(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT, color, language[310]);
			tooltipString += language[310];
			tooltipString += "\r\n";
		}
		else if ( item->beatitude == 0 )
		{
			//White if normal item.
			color = 0xFFFFFFFF;
			ttfPrintTextFormattedColor(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT, color, language[311]);
			tooltipString += language[311];
			tooltipString += "\r\n";
		}
		else
		{
			//Green if blessed.
			if ( colorblind )
			{
				color = SDL_MapRGB(mainsurface->format, 100, 245, 255); //Light blue if colorblind
			}
			else
			{
				color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
			}

			ttfPrintTextFormattedColor(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT, color, language[312]);
			tooltipString += language[312];
			tooltipString += "\r\n";
		}
	}
	if ( item->beatitude == 0 || !item->identified )
	{
		color = 0xFFFFFFFF;
	}

	if ( item->type == TOOL_GYROBOT || item->type == TOOL_DUMMYBOT
		|| item->type == TOOL_SENTRYBOT || item->type == TOOL_SPELLBOT )
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
		ttfPrintTextFormattedColor(ttf12, src.x + 4, src.y + 4, color, "%s (%d%%)", item->description(), health);

		snprintf(tooltipBuffer, sizeof(tooltipBuffer), "%s (%d%%)", item->description(), health);
		tooltipString += tooltipBuffer;
		tooltipString += "\r\n";
	}
	else if ( item->type == ENCHANTED_FEATHER && item->identified )
	{
		ttfPrintTextFormattedColor(ttf12, src.x + 4, src.y + 4, color, "%s (%d%%)", item->description(), item->appearance % ENCHANTED_FEATHER_MAX_DURABILITY);

		snprintf(tooltipBuffer, sizeof(tooltipBuffer), "%s (%d%%)", item->description(), item->appearance % ENCHANTED_FEATHER_MAX_DURABILITY);
		tooltipString += tooltipBuffer;
		tooltipString += "\r\n";
	}
	else
	{
		ttfPrintTextFormattedColor(ttf12, src.x + 4, src.y + 4, color, "%s", item->description());

		tooltipString += item->description();
		tooltipString += "\r\n";
	}
	int itemWeight = item->getWeight();
	ttfPrintTextFormatted(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT * 2, language[313], itemWeight);
	snprintf(tooltipBuffer, sizeof(tooltipBuffer), language[313], itemWeight);
	tooltipString += tooltipBuffer;
	tooltipString += "\r\n";

	ttfPrintTextFormatted(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT * 3, language[314], item->sellValue(player));
	snprintf(tooltipBuffer, sizeof(tooltipBuffer), language[314], item->sellValue(player));
	tooltipString += tooltipBuffer;
	tooltipString += "\r\n";

	if ( strcmp(spellEffectText, "") )
	{
		ttfPrintTextFormattedColor(ttf12, src.x + 4, src.y + 4 + TTF12_HEIGHT * 4, SDL_MapRGB(mainsurface->format, 0, 255, 255), spellEffectText);
		tooltipString += spellEffectText;
		tooltipString += "\r\n";
	}

	if ( item->identified )
	{
		if ( itemCategory(item) == WEAPON || itemCategory(item) == THROWN
			|| itemTypeIsQuiver(item->type) )
		{
			Monster tmpRace = stats[player]->type;
			if ( stats[player]->type == TROLL
				|| stats[player]->type == RAT
				|| stats[player]->type == SPIDER
				|| stats[player]->type == CREATURE_IMP )
			{
				// these monsters have 0 bonus from weapons, but want the tooltip to say the normal amount.
				stats[player]->type = HUMAN;
			}

			if ( item->weaponGetAttack(stats[player]) >= 0 )
			{
				color = SDL_MapRGB(mainsurface->format, 0, 255, 255);
			}
			else
			{
				color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
			}
			if ( stats[player]->type != tmpRace )
			{
				color = SDL_MapRGB(mainsurface->format, 127, 127, 127); // grey out the text if monster doesn't benefit.
			}

			ttfPrintTextFormattedColor(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT * 4, color, language[315], item->weaponGetAttack(stats[player]));

			snprintf(tooltipBuffer, sizeof(tooltipBuffer), language[315], item->weaponGetAttack(stats[player]));
			tooltipString += tooltipBuffer;
			tooltipString += "\r\n";

			stats[player]->type = tmpRace;
		}
		else if ( itemCategory(item) == ARMOR )
		{
			Monster tmpRace = stats[player]->type;
			if ( stats[player]->type == TROLL
				|| stats[player]->type == RAT
				|| stats[player]->type == SPIDER
				|| stats[player]->type == CREATURE_IMP )
			{
				// these monsters have 0 bonus from weapons, but want the tooltip to say the normal amount.
				stats[player]->type = HUMAN;
			}

			if ( item->armorGetAC(stats[player]) >= 0 )
			{
				color = SDL_MapRGB(mainsurface->format, 0, 255, 255);
			}
			else
			{
				color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
			}
			if ( stats[player]->type != tmpRace )
			{
				color = SDL_MapRGB(mainsurface->format, 127, 127, 127); // grey out the text if monster doesn't benefit.
			}

			ttfPrintTextFormattedColor(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT * 4, color, language[316], item->armorGetAC(stats[player]));
			snprintf(tooltipBuffer, sizeof(tooltipBuffer), language[316], item->armorGetAC(stats[player]));
			tooltipString += tooltipBuffer;
			tooltipString += "\r\n";

			stats[player]->type = tmpRace;
		}
		else if ( itemCategory(item) == SCROLL )
		{
			color = SDL_MapRGB(mainsurface->format, 0, 255, 255);
			ttfPrintTextFormattedColor(ttf12, src.x + 4 + TTF12_WIDTH, src.y + 4 + TTF12_HEIGHT * 4, color, "%s%s", language[3862], item->getScrollLabel());
			snprintf(tooltipBuffer, sizeof(tooltipBuffer), "%s%s", language[3862], item->getScrollLabel());
			tooltipString += tooltipBuffer;
			tooltipString += "\r\n";
		}
	}
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

		if ( y < Player::Inventory_t::DOLL_ROW_1 || y > Player::Inventory_t::DOLL_ROW_5 )
		{
			// in inventory
			return SLOT_MAX;
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
		else if ( !movingFromInventory && diffy == 0 && diffx != 0 )
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

	if ( diffy > 0 )
	{
		yout = 0;
	}
	else if ( diffy < 0 )
	{
		yout = Player::Inventory_t::DOLL_ROW_1 - 1;
	}
	return false;
}

// only called by handleInventoryMovement in player.cpp
void select_inventory_slot(int player, int currentx, int currenty, int diffx, int diffy)
{
	auto& selectedItem = inputs.getUIInteraction(player)->selectedItem;
	auto& inventoryUI = players[player]->inventoryUI;
	auto paperDollSlot = Player::PaperDoll_t::PaperDollSlotType::SLOT_MAX;

	int x = currentx + diffx;
	int y = currenty + diffy;

	if ( selectedItem )
	{
		auto selectedItemDollSlot = getPaperDollSlotFromItemType(*selectedItem);
		if ( selectedItemDollSlot != Player::PaperDoll_t::PaperDollSlotType::SLOT_MAX )
		{
			if ( currenty < 0 ) // moving from doll
			{
				if ( diffy < 0 )
				{
					y = inventoryUI.getSizeY() - 1;
				}
				else if ( diffy > 0 )
				{
					y = 0;
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
				inventoryUI.selectSlot(x, y);
				return;
			}
			else
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
			if ( moveInPaperDoll(player, paperDollSlot, currentx, currenty, diffx, diffy, x, y) )
			{
				return;
			}
		}
	}

	if ( x < 0 )   //Wrap around left boundary.
	{
		x = inventoryUI.getSizeX() - 1;
	}

	if ( x >= inventoryUI.getSizeX() )   //Wrap around right boundary.
	{
		x = 0;
	}

	if ( inventoryUI.selectedSlotInPaperDoll() )
	{
		auto oldSlot = players[player]->paperDoll.paperDollSlotFromCoordinates(inventoryUI.getSelectedSlotX(), inventoryUI.getSelectedSlotY());
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

	bool warpInv = true;
	auto& hotbar_t = players[player]->hotbar;

	if ( y < 0 )   //Wrap around top to bottom.
	{
		y = inventoryUI.getSizeY() - 1;
	}
	if ( y >= inventoryUI.getSizeY() )   //Hit bottom. Wrap around or go to shop/chest?
	{
		if ( openedChest[player] )
		{
			//Do not want to wrap around if opened chest or shop.
			warpInv = false;
			y = inventoryUI.getSizeY() - 1; //Keeps the selected slot within the inventory, to warp back to later.

			if ( numItemsInChest(player) > 0 )   //If chest even has an item...
			{
				//Then warp cursor to chest.
				selectedChestSlot[player] = 0; //Warp to first chest slot.
				int warpX = getChestGUIStartX(player) + (inventoryoptionChest_bmp->w / 2);
				int warpY = getChestGUIStartY(player) + (inventoryoptionChest_bmp->h / 2)  + 16;
				//SDL_WarpMouseInWindow(screen, warpX, warpY);
				Uint32 flags = (Inputs::SET_MOUSE | Inputs::SET_CONTROLLER);
				inputs.warpMouse(player, warpX, warpY, flags);
			}
		}
		else if ( players[player]->gui_mode == GUI_MODE_SHOP )
		{
			warpInv = false;
			y = inventoryUI.getSizeY() - 1; //Keeps the selected slot within the inventory, to warp back to later.

			//Warp into shop inventory if shopkeep has any items.
			if ( shopinvitems[player][0] )
			{
				selectedShopSlot[player] = 0;
				warpMouseToSelectedShopSlot(player);
			}
		}
		else if ( GenericGUI[player].isGUIOpen() )
		{
			warpInv = false;
			y = inventoryUI.getSizeY() - 1;

			//Warp into GUI "inventory"...if there is anything there.
			if ( GenericGUI[player].itemsDisplayed[0] )
			{
				GenericGUI[player].selectedSlot = 0;
				GenericGUI[player].warpMouseToSelectedSlot();
			}
		}

		if ( warpInv )   //Wrap around to top.
		{
			y = 0;
		}
	}

	inventoryUI.selectSlot(x, y);
}

std::string getItemSpritePath(const int player, Item& item)
{
	if ( item.type == SPELL_ITEM )
	{
		return ItemTooltips.getSpellIconPath(player, item);
	}
	else
	{
		node_t* imagePathsNode = list_Node(&items[item.type].images, item.appearance % items[item.type].variations);
		if ( imagePathsNode )
		{
			string_t* imagePath = static_cast<string_t*>(imagePathsNode->element);
			return imagePath->data;
		}
	}
	return "";
}

/*-------------------------------------------------------------------------------

	updatePlayerInventory

	Draws and processes everything related to the player's inventory window

-------------------------------------------------------------------------------*/

/*void releaseItem(int x, int y) {
	node_t* node = nullptr;
	node_t* nextnode = nullptr;

	/*
	 * So, here is what must happen:
	 * * If mouse behavior mode, toggle drop!
	 * * If gamepad behavior mode, toggle release if x key pressed.
	 * * * However, keep in mind that you want a mouse click to trigger drop, just in case potato. You know, controller dying or summat. Don't wanna jam game.
	 */

void releaseItem(const int player) //TODO: This function uses toggleclick. Conflict with inventory context menu?
{
	Item*& selectedItem = inputs.getUIInteraction(player)->selectedItem;
	int& selectedItemFromHotbar = inputs.getUIInteraction(player)->selectedItemFromHotbar;

	if ( !selectedItem )
	{
		return;
	}

	node_t* node = nullptr;
	node_t* nextnode = nullptr;

	char name[32];
	snprintf(name, sizeof(name), "player inventory %d", player);
	Frame* frame = gui->findFrame(name);

	auto& hotbar = players[player]->hotbar.slots();

	if ( Input::inputs[player].binaryToggle(getContextMenuOptionBindingName(PROMPT_DROP).c_str()) )
	{
		//TODO UI: VERIFY
		if (selectedItemFromHotbar >= 0 && selectedItemFromHotbar < NUM_HOTBAR_SLOTS)
		{
			//Warp cursor back into hotbar, for gamepad convenience.
			if ( players[player]->hotbar.warpMouseToHotbar(selectedItemFromHotbar, (Inputs::SET_CONTROLLER)) )
			{
				players[player]->hotbar.selectHotbarSlot(selectedItemFromHotbar);
			}
			hotbar[selectedItemFromHotbar].item = selectedItem->uid;
			selectedItemFromHotbar = -1;
			players[player]->GUI.activateModule(Player::GUI_t::MODULE_HOTBAR);
		}
		else
		{
			if ( !players[player]->inventoryUI.warpMouseToSelectedItem(selectedItem, (Inputs::SET_CONTROLLER)) )
			{
				//messagePlayer(0, "[Debug]: warpMouseToSelectedInventorySlot failed");
				// TODO UI: REMOVE DEBUG AND CLEAN UP
				//Warp cursor back into inventory, for gamepad convenience.
				//int newx = players[player]->inventoryUI.getSelectedSlotPositionX(selectedItem);
				//int newy = players[player]->inventoryUI.getSelectedSlotPositionY(selectedItem);
				//
				////SDL_WarpMouseInWindow(screen, newx, newy);
				//Uint32 flags = (Inputs::SET_MOUSE | Inputs::SET_CONTROLLER);
				//inputs.warpMouse(player, newx, newy, flags);
			}
			players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
		}

		selectedItem = nullptr;
		Input::inputs[player].consumeBinaryToggle(getContextMenuOptionBindingName(PROMPT_DROP).c_str());
		return;
	}

	//TODO: Do proper refactoring.
	if ( selectedItem && itemCategory(selectedItem) == SPELL_CAT && selectedItem->appearance >= 1000 )
	{
		if ( canUseShapeshiftSpellInCurrentForm(player, *selectedItem) == 0 )
		{
			selectedItem = nullptr;
			return;
		}
	}

	const Sint32 mousex = inputs.getMouse(player, Inputs::X);
	const Sint32 mousey = inputs.getMouse(player, Inputs::Y);

	bool& toggleclick = inputs.getUIInteraction(player)->toggleclick;

	// releasing items
	if ( (!inputs.bMouseLeft(player) && !toggleclick)
		|| (inputs.bMouseLeft(player) && toggleclick)
		|| ( Input::inputs[player].binaryToggle(getContextMenuOptionBindingName(PROMPT_GRAB).c_str()) && toggleclick) )
	{
		Input::inputs[player].consumeBinaryToggle(getContextMenuOptionBindingName(PROMPT_GRAB).c_str());
		if (openedChest[player] && itemCategory(selectedItem) != SPELL_CAT)
		{
			if (mousex >= getChestGUIStartX(player) && mousey >= getChestGUIStartY(player)
			        && mousex < getChestGUIStartX(player) + inventoryChest_bmp->w
			        && mousey < getChestGUIStartY(player) + inventoryChest_bmp->h)
			{
				if (selectedItem->count > 1)
				{
					openedChest[player]->addItemToChestFromInventory(
						player, selectedItem, false);
					toggleclick = true;
				}
				else
				{
					openedChest[player]->addItemToChestFromInventory(
						player, selectedItem, false);
					selectedItem = nullptr;
					toggleclick = false;
				}
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
			bool mouseOverSlot = getSlotFrameXYFromMousePos(player, slotFrameX, slotFrameY);
			if ( mouseOverSlot && slotFrameY > Player::Inventory_t::DOLL_ROW_5 )
			{
				if ( bPaperDollItem )
				{
					if ( !players[player]->inventoryUI.bItemInventoryHasFreeSlot() )
					{
						// can't drag off into inventory, no slots available
						messagePlayer(player, language[3997], selectedItem->getName());
						selectedItem = nullptr;
						toggleclick = false;
						if ( inputs.bMouseLeft(player) )
						{
							inputs.mouseClearLeft(player);
						}
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
					nextnode = node->next;
					Item* tempItem = (Item*) (node->element);
					if (tempItem == selectedItem)
					{
						continue;
					}

					toggleclick = false;
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

								bool unequipped = executeItemMenuOption0ForPaperDoll(player, selectedItem);
								if ( !unequipped )
								{
									// failure to unequip
									selectedItem->x = oldx;
									selectedItem->y = oldy;

									selectedItem = nullptr;
									toggleclick = false;
									if ( inputs.bMouseLeft(player) )
									{
										inputs.mouseClearLeft(player);
									}
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
							}

							swappedItem = selectedItem;

							tempItem->x = oldx;
							tempItem->y = oldy;
							selectedItem = tempItem;
							toggleclick = true;
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

						bool unequipped = executeItemMenuOption0ForPaperDoll(player, selectedItem);
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
				}

				playSound(139, 64); // click sound
			}
			else if (itemCategory(selectedItem) == SPELL_CAT)
			{
				//Outside inventory. Spells can't be dropped.
				int slotNum = 0;
				hotbar_slot_t* slot = getCurrentHotbarUnderMouse(player, &slotNum);
				if (slot)
				{
					//Add spell to hotbar.
					Item* tempItem = uidToItem(slot->item);
					if (tempItem)
					{
						slot->item = selectedItem->uid;
						selectedItem = tempItem;
						toggleclick = true;
					}
					else
					{
						slot->item = selectedItem->uid;
						selectedItem = NULL;
						toggleclick = false;
					}
					// empty out duplicate slots that match this item uid.
					int i = 0;
					for ( auto& s : players[player]->hotbar.slots() )
					{
						if ( i != slotNum && s.item == slot->item )
						{
							s.item = 0;
						}
						++i;
					}
					playSound(139, 64); // click sound
				}
				else
				{
					selectedItem = NULL;
				}
			}
			else if ( frame && frame->findFrame("paperdoll slots") && frame->findFrame("paperdoll slots")->capturesMouseInRealtimeCoords() )
			{
				// get slot for item type
				// if slot type is invalid, cancel.
				auto slotName = items[selectedItem->type].item_slot;
				Player::PaperDoll_t::PaperDollSlotType dollSlot = getPaperDollSlotFromItemType(*selectedItem);

				if ( bPaperDollItem || slotName == ItemEquippableSlot::NO_EQUIP || dollSlot == Player::PaperDoll_t::PaperDollSlotType::SLOT_MAX )
				{
					// moving paper doll item onto own area - or item can't be equipped, no action.
					selectedItem = NULL;
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
							selectedItem->x = oldx;
							selectedItem->y = oldy;
							selectedItem = nullptr;
							toggleclick = false;
							if ( inputs.bMouseLeft(player) )
							{
								inputs.mouseClearLeft(player);
							}
							return;
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
					}

					playSound(139, 64); // click sound
				}
			}
			else
			{
				// outside inventory
				int slotNum = 0;
				hotbar_slot_t* slot = getCurrentHotbarUnderMouse(player, &slotNum);
				if (slot)
				{
					//Add item to hotbar.
					Item* tempItem = uidToItem(slot->item);
					if (tempItem && tempItem != selectedItem)
					{
						slot->item = selectedItem->uid;
						selectedItem = tempItem;
						toggleclick = true;
					}
					else
					{
						slot->item = selectedItem->uid;
						selectedItem = NULL;
						toggleclick = false;
					}

					// empty out duplicate slots that match this item uid.
					int i = 0;
					for ( auto& s : players[player]->hotbar.slots() )
					{
						if ( i != slotNum && s.item == slot->item )
						{
							s.item = 0;
						}
						++i;
					}
					playSound(139, 64); // click sound
				}
				else
				{
					if ( bPaperDollItem )
					{
						bool unequipped = executeItemMenuOption0ForPaperDoll(player, selectedItem); // unequip item
						if ( !unequipped )
						{
							selectedItem = NULL;
							toggleclick = false;
						}
					}
					else
					{
						if ( selectedItem->count > 1 ) // drop item
						{
							if ( dropItem(selectedItem, player) )
							{
								selectedItem = NULL;
							}
							toggleclick = true;
						}
						else
						{
							dropItem(selectedItem, player);
							selectedItem = NULL;
							toggleclick = false;
						}
					}
				}
			}
		}
		if ( inputs.bMouseLeft(player) )
		{
			inputs.mouseClearLeft(player);
		}
	}
}

void cycleInventoryTab(const int player)
{
	if ( inputs.getUIInteraction(player)->selectedItem )
	{
		return;
	}
	if ( players[player]->inventory_mode == INVENTORY_MODE_ITEM)
	{
		players[player]->inventory_mode = INVENTORY_MODE_SPELL;
	}
	else
	{
		//inventory_mode == INVENTORY_MODE_SPELL
		players[player]->inventory_mode = INVENTORY_MODE_ITEM;
	}
}

/*
 * Because the mouseInBounds() function looks at the omousex for whatever reason.
 * And changing that function to use mousex has, through much empirical study, been proven to be a bad idea. Things will break.
 * So, this function is used instead for one thing and only one thing: the gold borders that follow the mouse through the inventory.
 *
 * Places used so far:
 * * Here. In updatePlayerInventory(), where it draws the gold borders around the inventory tile the mouse is hovering over.
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

//TODO UI: VERIFY BLUE BORDER FOR PAPERDOLL
void drawBlueInventoryBorder(const int player, const Item& item, int x, int y)
{
	SDL_Rect pos;
	pos.x = x + item.x * players[player]->inventoryUI.getSlotSize() + 2;
	pos.y = y + item.y * players[player]->inventoryUI.getSlotSize() + 1;
	pos.w = players[player]->inventoryUI.getSlotSize();
	pos.h = players[player]->inventoryUI.getSlotSize();

	Uint32 color = SDL_MapRGBA(mainsurface->format, 0, 0, 255, 127);
	drawBox(&pos, color, 127);
}

int getContextMenuOptionOrder(ItemContextMenuPrompts prompt)
{
	std::string bindingName = getContextMenuOptionBindingName(prompt);
	if ( bindingName == "MenuAlt1" )
	{
#ifdef NINTENDO
		return 1;
#else
		return 2;
#endif
	}
	else if ( bindingName == "MenuAlt2" )
	{
#ifdef NINTENDO
		return 2;
#else
		return 1;
#endif
	}
	else if ( bindingName == "MenuConfirm" )
	{
#ifdef NINTENDO
		return 3;
#else
		return 4;
#endif
	}
	else if ( bindingName == "MenuCancel" )
	{
#ifdef NINTENDO
		return 4;
#else
		return 3;
#endif
	}
	return 5;
}

void Player::HUD_t::updateFrameTooltip(Item* item, const int x, const int y)
{
	const int player = this->player.playernum;

	if ( !item )
	{
		return;
	}

	auto& tooltipDisplayedSettings = this->player.inventoryUI.itemTooltipDisplay;

	bool bUpdateDisplayedTooltip = 
		(!tooltipDisplayedSettings.isItemSameAsCurrent(player, item) || ItemTooltips.itemDebug);

	auto frameMain = this->player.inventoryUI.tooltipFrame;
	if ( !frameMain )
	{
		return;
	}

	auto frameInventory = this->player.inventoryUI.frame;
	if ( !frameInventory )
	{
		return;
	}

	auto frameInteract = this->player.inventoryUI.interactFrame;
	if ( !frameInteract )
	{
		return;
	}

	auto frameTooltipPrompt = this->player.inventoryUI.tooltipPromptFrame;
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

	auto imgSpellIcon = frameAttr->findImage("inventory mouse tooltip spell image");
	auto imgSpellIconBg = frameAttr->findImage("inventory mouse tooltip spell image bg");
	auto imgPrimaryIcon = frameAttr->findImage("inventory mouse tooltip primary image");
	auto imgSecondaryIcon = frameAttr->findImage("inventory mouse tooltip secondary image");
	auto imgThirdIcon = frameAttr->findImage("inventory mouse tooltip third image");
	auto imgGoldIcon = frameValues->findImage("inventory mouse tooltip gold image");
	auto imgWeightIcon = frameValues->findImage("inventory mouse tooltip weight image");
	auto imgValueBackground = frameValues->findImage("inventory mouse tooltip value background");

	auto txtHeader = frameMain->findField("inventory mouse tooltip header");
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

	auto txtAttributes = frameAttr->findField("inventory mouse tooltip attributes text");
	auto txtDescription = frameDesc->findField("inventory mouse tooltip description");
	auto txtDescriptionPositive = frameDesc->findField("inventory mouse tooltip description positive text");
	auto txtDescriptionNegative = frameDesc->findField("inventory mouse tooltip description negative text");

	auto txtPrompt = framePrompt->findField("inventory mouse tooltip prompt");
	auto txtGoldValue = frameValues->findField("inventory mouse tooltip gold value");
	auto txtWeightValue = frameValues->findField("inventory mouse tooltip weight value");
	auto txtIdentifiedValue = frameValues->findField("inventory mouse tooltip identified value");

	char buf[1024] = "";

	const int padx = 4;
	const int pady = 4;
	const int imgTopBackgroundDefaultHeight = 28;
	const int imgTopBackground2XHeight = 42;

	if ( ItemTooltips.itemDebug )
	{
		if ( keystatus[SDL_SCANCODE_KP_PLUS] )
		{
			keystatus[SDL_SCANCODE_KP_PLUS] = 0;
			item->beatitude += 1;
			/*for ( int i = 0; i < 100; ++i )
			{
				Uint32 r = rand() % 4096;
				printlog("%d %d %d", (r >> 8 & 0xF), (r >> 4 & 0xF), (r & 0xF));
			}*/
		}
		if ( keystatus[SDL_SCANCODE_KP_MINUS] )
		{
			keystatus[SDL_SCANCODE_KP_MINUS] = 0;
			item->beatitude -= 1;
		}
		if ( keystatus[SDL_SCANCODE_KP_6] )
		{
			keystatus[SDL_SCANCODE_KP_6] = 0;
			item->status = std::min(EXCELLENT, static_cast<Status>(item->status + 1));
		}
		if ( keystatus[SDL_SCANCODE_KP_4] )
		{
			keystatus[SDL_SCANCODE_KP_4] = 0;
			item->status = std::max(BROKEN, static_cast<Status>(item->status - 1));
		}
		if ( keystatus[SDL_SCANCODE_KP_5] )
		{
			keystatus[SDL_SCANCODE_KP_5] = 0;
			if ( item->type == WOODEN_SHIELD )
			{
				item->type = static_cast<ItemType>(NUMITEMS - 1);
			}
			else
			{
				item->type = static_cast<ItemType>(item->type - 1);
			}
		}
		if ( keystatus[SDL_SCANCODE_KP_8] )
		{
			keystatus[SDL_SCANCODE_KP_8] = 0;
			if ( item->type == NUMITEMS - 1 )
			{
				item->type = WOODEN_SHIELD;
			}
			else
			{
				item->type = static_cast<ItemType>(item->type + 1);
			}
		}
	}

	if ( keystatus[SDL_SCANCODE_KP_1] )
	{
		keystatus[SDL_SCANCODE_KP_1] = 0;
		tooltipDisplayedSettings.expanded = !tooltipDisplayedSettings.expanded;
	}

	if ( bUpdateDisplayedTooltip )
	{
		tooltipDisplayedSettings.updateItem(player, item);

		std::string tooltipType = ItemTooltips.tmpItems[item->type].tooltip;

		if ( ItemTooltips.tooltips.find(tooltipType) == ItemTooltips.tooltips.end() )
		{
			tooltipType = "tooltip_default";
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
		if ( item->type == SPELL_ITEM )
		{
			spell_t* spell = getSpellFromItem(player, item);
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
						snprintf(buf, sizeof(buf), "%s%s%s",
							ItemTooltips.adjectives["spell_prefixes"]["spell_of"].c_str(), spell->name, language[3408]);
						break;
					case SPELL_POISON:
					case SPELL_SPRAY_WEB:
						snprintf(buf, sizeof(buf), "%s%s%s",
							ItemTooltips.adjectives["spell_prefixes"]["spell_of"].c_str(), spell->name, language[3409]);
						break;
					case SPELL_STRIKE:
					case SPELL_FEAR:
					case SPELL_TROLLS_BLOOD:
						snprintf(buf, sizeof(buf), "%s%s%s",
							ItemTooltips.adjectives["spell_prefixes"]["spell_of"].c_str(), spell->name, language[3410]);
						break;
					case SPELL_LIGHTNING:
					case SPELL_CONFUSE:
					case SPELL_AMPLIFY_MAGIC:
						snprintf(buf, sizeof(buf), "%s%s%s",
							ItemTooltips.adjectives["spell_prefixes"]["spell_of"].c_str(), spell->name, language[3411]);
						break;
					default:
						snprintf(buf, sizeof(buf), "%s%s",
							ItemTooltips.adjectives["spell_prefixes"]["spell_of"].c_str(), spell->name);
						break;
				}
			}
			else
			{
				snprintf(buf, sizeof(buf), "%s%s",
					ItemTooltips.adjectives["spell_prefixes"]["spell_of"].c_str(), spell->name);
			}
		}
		else
		{
			snprintf(buf, sizeof(buf), "%s %s (%+d)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), item->getName(), item->beatitude);
		}
		txtHeader->setText(buf);
		Text* textGet = Text::get(txtHeader->getText(), txtHeader->getFont(),
			txtHeader->getTextColor(), txtHeader->getOutlineColor());
		if ( textGet )
		{
			textx = textGet->getWidth();
			texty = textGet->getHeight() * txtHeader->getNumTextLines();
		}

		std::string minWidthKey = "default";
		std::string maxWidthKey = "default";
		std::string headerMaxWidthKey = "default";
		if ( spell_t* spell = getSpellFromItem(player, item) )
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
			if ( itemTooltip.minWidths.find(ItemTooltips.tmpItems[item->type].itemName) != itemTooltip.minWidths.end() )
			{
				minWidthKey = ItemTooltips.tmpItems[item->type].itemName;
			}
			if ( itemTooltip.maxWidths.find(ItemTooltips.tmpItems[item->type].itemName) != itemTooltip.maxWidths.end() )
			{
				maxWidthKey = ItemTooltips.tmpItems[item->type].itemName;
			}
			if ( itemTooltip.headerMaxWidths.find(ItemTooltips.tmpItems[item->type].itemName) != itemTooltip.headerMaxWidths.end() )
			{
				headerMaxWidthKey = ItemTooltips.tmpItems[item->type].itemName;
			}
		}

		bool useDefaultHeaderHeight = true;
		if ( itemTooltip.headerMaxWidths[headerMaxWidthKey] > 0 && textx > itemTooltip.headerMaxWidths[headerMaxWidthKey] )
		{
			txtHeader->setSize(SDL_Rect{ 0, 0, itemTooltip.headerMaxWidths[headerMaxWidthKey], 0 });
			txtHeader->reflowTextToFit(0);

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

				if ( Text* textGet = Text::get(txtHeader->getText(), txtHeader->getFont(),
					txtHeader->getTextColor(), txtHeader->getOutlineColor()) )
				{
					textx = newWidth;
					texty = textGet->getHeight() * txtHeader->getNumTextLines();
					imgTopBackground->pos.h = imgTopBackground2XHeight;
					imgTopBackground->path = "images/ui/Inventory/tooltips/Hover_T00_2x.png";
					imgTopBackgroundLeft->pos.h = imgTopBackground->pos.h;
					imgTopBackgroundLeft->path = "images/ui/Inventory/tooltips/Hover_TL00_2x.png";
					imgTopBackgroundRight->pos.h = imgTopBackground->pos.h;
					imgTopBackgroundRight->path = "images/ui/Inventory/tooltips/Hover_TR00_2x.png";
					useDefaultHeaderHeight = false;
				}
			}

			//if ( Text* textGet = Text::get(txtHeader->getText(), txtHeader->getFont(),
			//	  makeColor(255, 255, 255, 255), makeColor(0, 0, 0, 255)) )
			//{

			//}
			//if ( textGet )
			//{
			//	if ( textGet->getWidth() == textx )
			//	{
			//		// no reflow
			//	}
			//	else
			//	{
			//		textx = textGet->getWidth();
			//		texty = textGet->getHeight();
			//		imgTopBackground->pos.h = imgTopBackground2XHeight;
			//		imgTopBackground->path = "images/ui/Inventory/tooltips/Hover_T00_2x.png";
			//		imgTopBackgroundLeft->pos.h = imgTopBackground->pos.h;
			//		imgTopBackgroundLeft->path = "images/ui/Inventory/tooltips/Hover_TL00_2x.png";
			//		imgTopBackgroundRight->pos.h = imgTopBackground->pos.h;
			//		imgTopBackgroundRight->path = "images/ui/Inventory/tooltips/Hover_TR00_2x.png";
			//		useDefaultHeaderHeight = false;
			//	}

			//}
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

		if ( ItemTooltips.itemDebug )
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
				itemTooltip.minWidths[minWidthKey],
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

		if ( itemTooltip.minWidths[minWidthKey] > 0 )
		{
			textx = std::max(itemTooltip.minWidths[minWidthKey], textx);
		}
		if ( itemTooltip.maxWidths[maxWidthKey] > 0 )
		{
			textx = std::min(itemTooltip.maxWidths[maxWidthKey], textx);
		}

		if ( ItemTooltips.itemDebug )
		{
			auto headerBg = frameMain->findImage("inventory mouse tooltip header bg new");
			headerBg->pos = SDL_Rect{
				imgTopBackgroundLeft->pos.x + imgTopBackgroundLeft->pos.w + padx,
				imgTopBackground->pos.h,
				textx,
				1 };
			headerBg->disabled = false;
		}
	
		txtHeader->setSize(SDL_Rect{ imgTopBackgroundLeft->pos.x + imgTopBackgroundLeft->pos.w + padx, 0, textx + 3 * padx, imgTopBackground->pos.h});
		txtHeader->setVJustify(Field::justify_t::CENTER);
		int totalHeight = txtHeader->getSize().h;

		// get total width of tooltip
		const int tooltipWidth = txtHeader->getSize().w + imgTopBackgroundLeft->pos.w + imgTopBackgroundRight->pos.w;
		tooltipDisplayedSettings.tooltipWidth = tooltipWidth;

		// attribute frame size - padded by (imgTopBackgroundLeft->pos.x + (imgTopBackgroundLeft->pos.w / 2) + padx) on either side
		SDL_Rect frameAttrPos{ imgTopBackgroundLeft->pos.x + (imgTopBackgroundLeft->pos.w / 2) + padx, totalHeight, 0, 0 };
		frameAttrPos.w = tooltipWidth - frameAttrPos.x * 2;

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
			imgSpellIcon->path = ItemTooltips.getSpellIconPath(player, *item);
			imgSpellIcon->pos.x = frameAttrPos.w - imgSpellIcon->pos.w - 2 * padx;
			imgSpellIcon->pos.y = 3 * pady;
			imgSpellIconBg->pos.x = imgSpellIcon->pos.x - 6;
			imgSpellIconBg->pos.y = imgSpellIcon->pos.y - 6;
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
							if ( ItemTooltips.tmpItems[item->type].itemName != icon.conditionalAttribute )
							{
								continue;
							}
							icon.iconPath = ItemTooltips.getSpellIconPath(player, *item);
						}
					}
					else if ( itemCategory(item) == SCROLL )
					{
						if ( icon.conditionalAttribute.find("scroll_") != std::string::npos )
						{
							if ( ItemTooltips.tmpItems[item->type].itemName != icon.conditionalAttribute )
							{
								continue;
							}
						}
					}
					else if ( item->type == SPELL_ITEM )
					{
						if ( icon.conditionalAttribute == "SPELL_ICON_EFFECT" )
						{
							icon.iconPath = "images/ui/HUD/HUD_CharSheet_RES_00.png";
						}
						else if ( icon.conditionalAttribute.find("spell_") != std::string::npos )
						{
							spell_t* spell = getSpellFromItem(player, item);
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
					else if ( itemCategory(item) == SPELLBOOK 
						&& icon.conditionalAttribute.find("SPELLBOOK_") != std::string::npos )
					{
						spell_t* spell = getSpellFromID(getSpellIDFromSpellbook(item->type));
						int skillLVL = std::min(100, stats[player]->PROFICIENCIES[PRO_MAGIC] + statGetINT(stats[player], players[player]->entity));
						bool isGoblin = (stats[player]
							&& (stats[player]->type == GOBLIN
								|| (stats[player]->playerRace == RACE_GOBLIN && stats[player]->appearance == 0)));
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
							if ( isGoblin || playerLearnedSpellbook(player, item) || (spell && skillLVL >= spell->difficulty) )
							{
								continue;
							}
						}
						else if ( icon.conditionalAttribute == "SPELLBOOK_UNLEARNABLE" )
						{
							if ( !isGoblin ) { continue; }
							if ( playerLearnedSpellbook(player, item) )
							{
								continue;
							}
						}
						else if ( icon.conditionalAttribute == "SPELLBOOK_LEARNABLE" )
						{
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
								if ( !playerLearnedSpellbook(player, item) )
								{
									continue;
								}
							}
							else if ( icon.conditionalAttribute == "SPELLBOOK_SPELLINFO_UNLEARNED" )
							{
								if ( playerLearnedSpellbook(player, item) )
								{
									continue;
								}
							}
							icon.iconPath = ItemTooltips.getSpellIconPath(player, *item);
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

				if ( index == 0 )
				{
					imgPrimaryIcon->disabled = false;
					imgPrimaryIcon->path = icon.iconPath;

					std::string iconText = icon.text;
					ItemTooltips.formatItemIcon(player, tooltipType, *item, iconText, index, icon.conditionalAttribute);

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

					txtPrimaryValue->setText(iconText.c_str());
					txtPrimaryValue->setColor(icon.textColor);
					txtPrimaryValueHighlight->setText(bracketText.c_str());
					txtPrimaryValueHighlight->setColor(itemTooltip.statusEffectTextColor);

					txtPrimaryValuePositive->setText(positiveText.c_str());
					txtPrimaryValuePositive->setColor(itemTooltip.positiveTextColor);

					txtPrimaryValueNegative->setText(negativeText.c_str());
					txtPrimaryValueNegative->setColor(itemTooltip.negativeTextColor);
				}
				else if ( index == 1 )
				{
					imgSecondaryIcon->disabled = false;
					imgSecondaryIcon->path = icon.iconPath;

					std::string iconText = icon.text;
					ItemTooltips.formatItemIcon(player, tooltipType, *item, iconText, index, icon.conditionalAttribute);

					std::string bracketText = "";
					ItemTooltips.stripOutHighlightBracketText(iconText, bracketText);
					std::string positiveText = "";
					std::string negativeText = "";
					ItemTooltips.stripOutPositiveNegativeItemDetails(iconText, positiveText, negativeText);

					txtSecondaryValue->setText(iconText.c_str());
					txtSecondaryValue->setColor(icon.textColor);
					txtSecondaryValueHighlight->setText(bracketText.c_str());
					txtSecondaryValueHighlight->setColor(itemTooltip.statusEffectTextColor);

					txtSecondaryValuePositive->setText(positiveText.c_str());
					txtSecondaryValuePositive->setColor(itemTooltip.positiveTextColor);

					txtSecondaryValueNegative->setText(negativeText.c_str());
					txtSecondaryValueNegative->setColor(itemTooltip.negativeTextColor);
				}
				else if ( index == 2 )
				{
					imgThirdIcon->disabled = false;
					imgThirdIcon->path = icon.iconPath;

					std::string iconText = icon.text;
					ItemTooltips.formatItemIcon(player, tooltipType, *item, iconText, index, icon.conditionalAttribute);

					std::string bracketText = "";
					ItemTooltips.stripOutHighlightBracketText(iconText, bracketText);
					std::string positiveText = "";
					std::string negativeText = "";
					ItemTooltips.stripOutPositiveNegativeItemDetails(iconText, positiveText, negativeText);

					txtThirdValue->setText(iconText.c_str());
					txtThirdValue->setColor(icon.textColor);
					txtThirdValueHighlight->setText(bracketText.c_str());
					txtThirdValueHighlight->setColor(itemTooltip.statusEffectTextColor);

					txtThirdValuePositive->setText(positiveText.c_str());
					txtThirdValuePositive->setColor(itemTooltip.positiveTextColor);

					txtThirdValueNegative->setText(negativeText.c_str());
					txtThirdValueNegative->setColor(itemTooltip.negativeTextColor);
				}
				else
				{
					break;
				}
				++index;
			}
		}

		txtAttributes->setDisabled(true);

		std::string descriptionTextString = "";
		if ( itemTooltip.descriptionText.size() > 0 || descriptionTextString.size() > 0 )
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
		if ( itemTooltip.detailsText.size() > 0 )
		{
			frameDesc->setDisabled(false);
			txtDescription->setColor(itemTooltip.detailsTextColor);
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
						if ( stats[player]->PROFICIENCIES[proficiency] == SKILL_LEVEL_LEGENDARY )
						{
							continue;
						}
					}
					else if ( tag.compare("weapon_legendary_durability") == 0 )
					{
						int proficiency = itemCategory(item) == ARMOR ? PRO_UNARMED: getWeaponSkill(item);
						if ( stats[player]->PROFICIENCIES[proficiency] != SKILL_LEVEL_LEGENDARY )
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
						if ( playerLearnedSpellbook(player, item) )
						{
							continue;
						}
					}
				}
				else if ( itemCategory(item) == SPELL_CAT )
				{
					spell_t* spell = getSpellFromItem(player, item);
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
						bool newbie = isSpellcasterBeginner(player, players[player]->entity);
						if ( !newbie )
						{
							continue;
						}
					}
					else if ( tag.compare("spell_newbie_newline") == 0 )
					{
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
						spell_t* spell = getSpellFromItem(player, item);
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
						if ( stats[player]->PROFICIENCIES[PRO_SHIELD] == SKILL_LEVEL_LEGENDARY )
						{
							continue;
						}
					}
					else if ( tag.compare("shield_legendary_durability") == 0 )
					{
						if ( stats[player]->PROFICIENCIES[PRO_SHIELD] != SKILL_LEVEL_LEGENDARY )
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
				ItemTooltips.formatItemDetails(player, tooltipType, *item, tagText, tag);
				if ( detailsTextString.compare("") != 0 )
				{
					detailsTextString += '\n';
				}
				detailsTextString += tagText;
			}
			txtDescription->setText(detailsTextString.c_str());
		}

		if ( detailsTextString == "" )
		{
			frameDesc->setDisabled(true);
		}

		const int imgToTextOffset = 0;
		txtPrimaryValue->setDisabled(imgPrimaryIcon->disabled);
		txtPrimaryValueHighlight->setDisabled(txtPrimaryValue->isDisabled());
		txtPrimaryValuePositive->setDisabled(txtPrimaryValue->isDisabled());
		txtPrimaryValueNegative->setDisabled(txtPrimaryValue->isDisabled());

		auto txtSlotName = frameAttr->findField("inventory mouse tooltip primary value slot name");
		txtSlotName->setDisabled(true);
		if ( items[item->type].item_slot != ItemEquippableSlot::NO_EQUIP )
		{
			txtSlotName->setDisabled(false);
			txtSlotName->setColor(itemTooltip.faintTextColor);
			txtSlotName->setSize(SDL_Rect{padx, pady * 2 + imgToTextOffset, txtHeader->getSize().w, imgPrimaryIcon->pos.h});
			txtSlotName->setText(ItemTooltips.getItemSlotName(items[item->type].item_slot).c_str());
		}

		const int iconPadx = 8;
		const int iconTextPadx = 4;

		if ( !imgPrimaryIcon->disabled )
		{
			imgPrimaryIcon->pos.x = iconPadx;
			imgPrimaryIcon->pos.y = pady * 2;

			if ( !imgSpellIcon->disabled )
			{
				imgPrimaryIcon->pos.y = imgSpellIcon->pos.y + (imgSpellIcon->pos.w / 2) - imgPrimaryIcon->pos.h / 2;
			}

			int iconMultipleLinePadding = 0;
			int numLines = txtPrimaryValue->getNumTextLines();
			if ( numLines > 1 )
			{
				auto textGet = Text::get(txtPrimaryValue->getText(), txtPrimaryValue->getFont(),
					txtPrimaryValue->getTextColor(), txtPrimaryValue->getOutlineColor());
				if ( textGet )
				{
					imgPrimaryIcon->pos.y += pady * (std::max(0, numLines - 1)); // for each line > 1 add padding
					iconMultipleLinePadding = textGet->getHeight() * numLines - imgPrimaryIcon->pos.h;
				}
			}

			txtPrimaryValue->setSize(SDL_Rect{ 
				imgPrimaryIcon->pos.x + imgPrimaryIcon->pos.w + padx + iconTextPadx,
				imgPrimaryIcon->pos.y + imgToTextOffset - iconMultipleLinePadding / 2,
				txtHeader->getSize().w, 
				imgPrimaryIcon->pos.h + iconMultipleLinePadding
			});
			txtPrimaryValueHighlight->setSize(txtPrimaryValue->getSize());
			txtPrimaryValuePositive->setSize(txtPrimaryValue->getSize());
			txtPrimaryValueNegative->setSize(txtPrimaryValue->getSize());
		}
	
		txtSecondaryValue->setDisabled(imgSecondaryIcon->disabled);
		txtSecondaryValueHighlight->setDisabled(txtSecondaryValue->isDisabled());
		txtSecondaryValuePositive->setDisabled(txtSecondaryValue->isDisabled());
		txtSecondaryValueNegative->setDisabled(txtSecondaryValue->isDisabled());
		txtThirdValue->setDisabled(imgThirdIcon->disabled);
		txtThirdValueHighlight->setDisabled(txtThirdValue->isDisabled());
		txtThirdValuePositive->setDisabled(txtThirdValue->isDisabled());
		txtThirdValueNegative->setDisabled(txtThirdValue->isDisabled());
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
			int numLines = txtSecondaryValue->getNumTextLines();
			if ( numLines > 1 )
			{
				auto textGet = Text::get(txtSecondaryValue->getText(), txtSecondaryValue->getFont(),
					txtSecondaryValue->getTextColor(), txtSecondaryValue->getOutlineColor());
				if ( textGet )
				{
					imgSecondaryIcon->pos.y += pady * (std::max(0, numLines - 1)); // for each line > 1 add padding
					iconMultipleLinePadding = textGet->getHeight() * numLines - imgSecondaryIcon->pos.h;
				}
			}

			txtSecondaryValue->setSize(SDL_Rect{ 
				imgSecondaryIcon->pos.x + imgSecondaryIcon->pos.w + padx + iconTextPadx,
				imgSecondaryIcon->pos.y + imgToTextOffset - iconMultipleLinePadding / 2, 
				txtHeader->getSize().w, 
				imgSecondaryIcon->pos.h + iconMultipleLinePadding 
			});
			txtSecondaryValueHighlight->setSize(txtSecondaryValue->getSize());
			txtSecondaryValuePositive->setSize(txtSecondaryValue->getSize());
			txtSecondaryValueNegative->setSize(txtSecondaryValue->getSize());
		}
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
			int numLines = txtThirdValue->getNumTextLines();
			if ( numLines > 1 )
			{
				auto textGet = Text::get(txtThirdValue->getText(), txtThirdValue->getFont(),
					txtThirdValue->getTextColor(), txtThirdValue->getOutlineColor());
				if ( textGet )
				{
					imgThirdIcon->pos.y += pady * (std::max(0, numLines - 1)); // for each line > 1 add padding
					iconMultipleLinePadding = textGet->getHeight() * numLines - imgThirdIcon->pos.h;
				}
			}

			txtThirdValue->setSize(SDL_Rect{ 
				imgThirdIcon->pos.x + imgThirdIcon->pos.w + padx + iconTextPadx,
				imgThirdIcon->pos.y + imgToTextOffset - iconMultipleLinePadding / 2,
				txtHeader->getSize().w, 
				imgThirdIcon->pos.h + iconMultipleLinePadding });
			txtThirdValueHighlight->setSize(txtThirdValue->getSize());
			txtThirdValuePositive->setSize(txtThirdValue->getSize());
			txtThirdValueNegative->setSize(txtThirdValue->getSize());
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
			int iconHeight1 = imgPrimaryIcon->disabled ? 0 : (std::max(txtPrimaryValue->getSize().y + txtPrimaryValue->getSize().h, 
				imgPrimaryIcon->pos.y + imgPrimaryIcon->pos.h));
			int iconHeight2 = imgSecondaryIcon->disabled ? 0 : (std::max(txtSecondaryValue->getSize().y + txtSecondaryValue->getSize().h,
				imgSecondaryIcon->pos.y + imgSecondaryIcon->pos.h));
			int iconHeight3 = imgThirdIcon->disabled ? 0 : (std::max(txtThirdValue->getSize().y + txtThirdValue->getSize().h,
				imgThirdIcon->pos.y + imgThirdIcon->pos.h));

			if ( !imgSpellIcon->disabled && txtPrimaryValue->getNumTextLines() <= 1 )
			{
				iconHeight1 += 2 * pady; // extra padding for the spell icon
			}

			frameAttrPos.h += std::max(std::max(iconHeight1, iconHeight2), iconHeight3);
			frameAttrPos.h += pady;

			if ( txtAttributes->isDisabled() )
			{
				//frameAttrPos.h += pady; // no description, add some padding
			}
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
			//messagePlayer(0, "%d", charHeight);

			/*auto textGet = Text::get(txtDescription->getText(), txtDescription->getFont(),
				makeColor(255, 255, 255, 255), makeColor(0, 0, 0, 255));
			if ( textGet )
			{
				auto tmp = textGet->getWidth();
				auto tmp2 = textGet->getHeight();
			}*/
		
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
			txtDescription->setText(detailsTextString.c_str());

			txtDescriptionPositive->setText(detailsPositiveText.c_str());
			txtDescriptionPositive->setColor(itemTooltip.positiveTextColor);
			txtDescriptionNegative->setText(detailsNegativeText.c_str());
			txtDescriptionNegative->setColor(itemTooltip.negativeTextColor);

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
				snprintf(valueBuf, sizeof(valueBuf), "%d", item->sellValue(player));
				txtGoldValue->setText(valueBuf);
			}
			txtGoldValue->setDisabled(false);

			int charWidth = 8;
			int charHeight = 13;
			const int lowerIconImgToTextOffset = 0;

			const std::string goldImagePath = "images/ui/Inventory/tooltips/HUD_Tooltip_Icon_Money_00.png";
			const std::string weightImagePath = "images/ui/Inventory/tooltips/HUD_Tooltip_Icon_WGT_00.png";
			const std::string spellMPCostImagePath = "images/ui/Inventory/tooltips/HUD_Tooltip_Icon_ManaRegen_00.png";

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
				imgGoldIcon->path = goldImagePath;

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
			totalHeight += framePromptPos.h;

			txtPrompt->setText("View item details");
			txtPrompt->setSize(SDL_Rect{ 0, 0, framePromptPos.w, framePromptPos.h });

			framePrompt->setSize(framePromptPos);
		}

		tooltipDisplayedSettings.tooltipHeight = totalHeight;
	}

	// realign stuff based on expanded animation status
	{
		SDL_Rect frameDescPos = frameDesc->getSize();
		frameDescPos.h = tooltipDisplayedSettings.tooltipDescriptionHeight;
		frameDescPos.h *= tooltipDisplayedSettings.expandCurrent;
		frameDesc->setSize(frameDescPos);

		const int heightDiff = tooltipDisplayedSettings.tooltipDescriptionHeight - frameDescPos.h;
		
		SDL_Rect frameValuesPos = frameValues->getSize();
		if ( !frameDesc->isDisabled() )
		{
			frameValuesPos.y = frameDescPos.y + frameDescPos.h;
			frameValues->setSize(frameValuesPos);
		}

		SDL_Rect framePromptPos = framePrompt->getSize();
		framePromptPos.y = frameValuesPos.y + frameValuesPos.h;
		framePrompt->setSize(framePromptPos);
		if ( tooltipDisplayedSettings.expanded )
		{
			txtPrompt->setText("Hide item details");
		}
		else
		{
			txtPrompt->setText("View item details");
		}

		// get left anchor for tooltip - if we want moving x tooltips, otherwise currently anchor to right of inventory panel
		//auto inventoryBgFrame = frameInventory->findFrame("inventory base");
		//const int tooltipPosX = frameInventory->getSize().x + inventoryBgFrame->getSize().x + inventoryBgFrame->getSize().w + 8;

		frameMain->setSize(SDL_Rect{ x, y, tooltipDisplayedSettings.tooltipWidth, tooltipDisplayedSettings.tooltipHeight - heightDiff });
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
	}

	tooltipDisplayedSettings.opacitySetpoint = 0;
	tooltipDisplayedSettings.opacityAnimate = 1.0;
	frameMain->setDisabled(false);
	frameMain->setOpacity(100.0);

	//inputs.getUIInteraction(player)->itemMenuItem = item->uid;
	// quick prompt stuff here.
	bool& itemMenuOpen = inputs.getUIInteraction(player)->itemMenuOpen;
	Item*& selectedItem = inputs.getUIInteraction(player)->selectedItem;
	frameTooltipPrompt->setDisabled(true);
	if ( !itemMenuOpen && !selectedItem && !inputs.getVirtualMouse(player)->draw_cursor )
	{
		auto options = getContextTooltipOptionsForItem(player, item);

		std::sort(options.begin(), options.end(), [](const ItemContextMenuPrompts& lhs, const ItemContextMenuPrompts& rhs) {
			return getContextMenuOptionOrder(lhs) < getContextMenuOptionOrder(rhs);
		});

		if ( !options.empty() )
		{
			frameTooltipPrompt->setDisabled(false);
			frameTooltipPrompt->setOpacity(frameMain->getOpacity());
			std::vector<std::pair<Frame::image_t*, Field*>> promptFrames;
			
			// disable all the text and prompts
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
					Uint8 r, g, b, a;
					SDL_GetRGBA(img->color, mainsurface->format, &r, &g, &b, &a);
					a = 128;
					img->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
					txt->setDisabled(true);
					promptFrames.push_back(std::make_pair(img, txt));
				}
			}

			// set the text and button prompts from the available options
			for ( auto& option : options )
			{
				char imgName[16];
				char fieldName[16];
				const int order = getContextMenuOptionOrder(option);

				snprintf(imgName, sizeof(imgName), "glyph %d", order);
				snprintf(fieldName, sizeof(imgName), "txt %d", order);
				auto img = frameTooltipPrompt->findImage(imgName);
				auto txt = frameTooltipPrompt->findField(fieldName);
				img->disabled = false;
				Uint8 r, g, b, a;
				SDL_GetRGBA(img->color, mainsurface->format, &r, &g, &b, &a);
				a = 255;
				img->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
				txt->setDisabled(false);
				img->path = Input::inputs[player].getGlyphPathForInput(getContextMenuOptionBindingName(option).c_str());
				txt->setText(getContextMenuLangEntry(player, option, *item));
			}

			int alignx1 = xres;
			for ( auto& pair : promptFrames )
			{
				auto& img = pair.first;
				auto& txt = pair.second;
				if ( !txt->isDisabled() )
				{
					if ( auto textGet = Text::get(txt->getText(), txt->getFont(),
						txt->getTextColor(), txt->getOutlineColor()) )
					{
						if ( txt->getHJustify() == Field::justify_t::RIGHT )
						{
							// find the furthest left text
							alignx1 = std::min(txt->getSize().x + txt->getSize().w - (int)textGet->getWidth(), alignx1);
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
							alignx2 = std::max(txt->getSize().x + (int)textGet->getWidth(), alignx2);
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
			}

			promptPos.x = frameMain->getSize().x + frameMain->getSize().w - 6 - promptPos.w;
			promptPos.y = frameMain->getSize().y + frameMain->getSize().h - 2;
			frameTooltipPrompt->setSize(promptPos);
		}
	}
}

void updatePlayerInventory(const int player)
{
	bool disableMouseDisablingHotbarFocus = false;
	SDL_Rect pos, mode_pos;
	node_t* node, *nextnode;

	auto& hotbar_t = players[player]->hotbar;
	auto& hotbar = hotbar_t.slots();

	const Sint32 mousex = inputs.getMouse(player, Inputs::X);
	const Sint32 mousey = inputs.getMouse(player, Inputs::Y);
	const Sint32 omousex = inputs.getMouse(player, Inputs::OX);
	const Sint32 omousey = inputs.getMouse(player, Inputs::OY);

	//const int x = players[player]->inventoryUI.getStartX();
	//const int y = players[player]->inventoryUI.getStartY();

	const int inventorySlotSize = players[player]->inventoryUI.getSlotSize();

	// draw translucent box
	//pos.x = x;
	//pos.y = y;
	pos.w = players[player]->inventoryUI.getSizeX() * inventorySlotSize;
	pos.h = players[player]->inventoryUI.getSizeY() * inventorySlotSize;

	char framename[32];
	snprintf(framename, sizeof(framename), "player inventory %d", player);
	Frame* frame = gui->findFrame(framename);
	if ( !frame )
	{
		drawRect(&pos, 0, 224);
	}

	bool& toggleclick = inputs.getUIInteraction(player)->toggleclick;
	bool& itemMenuOpen = inputs.getUIInteraction(player)->itemMenuOpen;
	Uint32& itemMenuItem = inputs.getUIInteraction(player)->itemMenuItem;
	int& itemMenuX = inputs.getUIInteraction(player)->itemMenuX;
	int& itemMenuY = inputs.getUIInteraction(player)->itemMenuY;
	int& itemMenuSelected = inputs.getUIInteraction(player)->itemMenuSelected;
	Item*& selectedItem = inputs.getUIInteraction(player)->selectedItem;

	if ( inputs.hasController(player) )
	{
		bool radialMenuOpen = FollowerMenu[player].followerMenuIsOpen();
		if ( players[player]->gui_mode == GUI_MODE_SHOP )
		{
			if ( inputs.bControllerInputPressed(player, INJOY_MENU_CYCLE_SHOP_LEFT) )
			{
				inputs.controllerClearInput(player, INJOY_MENU_CYCLE_SHOP_LEFT);
				cycleShopCategories(player, -1);
			}
			if ( inputs.bControllerInputPressed(player, INJOY_MENU_CYCLE_SHOP_RIGHT) )
			{
				inputs.controllerClearInput(player, INJOY_MENU_CYCLE_SHOP_RIGHT);
				cycleShopCategories(player, 1);
			}
		}
		
		if ( radialMenuOpen )
		{
			// do nothing?
		}
		else if ( selectedChestSlot[player] < 0 && selectedShopSlot[player] < 0 
			&& !itemMenuOpen && GenericGUI[player].selectedSlot < 0
			&& players[player]->GUI.handleInventoryMovement() ) // handleInventoryMovement should be at the end of this check
		{
			if ( selectedChestSlot[player] < 0 && selectedShopSlot[player] < 0
				&& GenericGUI[player].selectedSlot < 0 ) //This second check prevents the extra mouse warp.
			{
				if ( !hotbar_t.hotbarHasFocus )
				{
					warpMouseToSelectedInventorySlot(player);
				}
				else
				{
					disableMouseDisablingHotbarFocus = true;
				}
			}
		}
		else if ( selectedChestSlot[player] >= 0 && !itemMenuOpen && inputs.getController(player)->handleChestMovement(player) )
		{
			if ( selectedChestSlot[player] < 0 )
			{
				//Move out of chest. Warp cursor back to selected inventory slot.
				warpMouseToSelectedInventorySlot(player);
			}
		}
		else if ( selectedShopSlot[player] >= 0 && !itemMenuOpen && inputs.getController(player)->handleShopMovement(player) )
		{
			if ( selectedShopSlot[player] < 0 )
			{
				warpMouseToSelectedInventorySlot(player);
			}
		}
		else if ( GenericGUI[player].selectedSlot >= 0 && !itemMenuOpen && inputs.getController(player)->handleRepairGUIMovement(player) )
		{
			if ( GenericGUI[player].selectedSlot < 0 )
			{
				warpMouseToSelectedInventorySlot(player);
			}
		}

		if ( inputs.bControllerInputPressed(player, INJOY_MENU_INVENTORY_TAB) )
		{
			inputs.controllerClearInput(player, INJOY_MENU_INVENTORY_TAB);
			cycleInventoryTab(player);
		}

		if ( lastkeypressed == 300 )
		{
			lastkeypressed = 0;
		}

		if ( inputs.bControllerInputPressed(player, INJOY_MENU_MAGIC_TAB) )
		{
			inputs.controllerClearInput(player, INJOY_MENU_MAGIC_TAB);
			cycleInventoryTab(player);
		}
	}

	if ( !command 
		&& (*inputPressedForPlayer(player, impulses[IN_AUTOSORT]) 
			|| (inputs.bControllerInputPressed(player, INJOY_MENU_CHEST_GRAB_ALL) && !openedChest[player])
			)
		)
	{
		autosortInventory(player);
		//quickStackItems();
		*inputPressedForPlayer(player, impulses[IN_AUTOSORT]) = 0;
		inputs.controllerClearInput(player, INJOY_MENU_CHEST_GRAB_ALL);
		playSound(139, 64);
	}

	// draw grid
	//pos.x = x;
	//pos.y = y;
	pos.w = players[player]->inventoryUI.getSizeX() * inventorySlotSize;
	pos.h = players[player]->inventoryUI.getSizeY() * inventorySlotSize;

	if ( !frame )
	{
		drawLine(pos.x, pos.y, pos.x, pos.y + pos.h, SDL_MapRGB(mainsurface->format, 150, 150, 150), 255);
		drawLine(pos.x, pos.y, pos.x + pos.w, pos.y, SDL_MapRGB(mainsurface->format, 150, 150, 150), 255);
		for ( int x = 0; x <= players[player]->inventoryUI.getSizeX(); x++ )
		{
			drawLine(pos.x + x * inventorySlotSize, pos.y, pos.x + x * inventorySlotSize, pos.y + pos.h, SDL_MapRGB(mainsurface->format, 150, 150, 150), 255);
		}
		for ( int y = 0; y <= players[player]->inventoryUI.getSizeY(); y++ )
		{
			drawLine(pos.x, pos.y + y * inventorySlotSize, pos.x + pos.w, pos.y + y * inventorySlotSize, SDL_MapRGB(mainsurface->format, 150, 150, 150), 255);
		}
	}

	if ( frame )
	{
		resetInventorySlotFrames(player);
	}

	if ( !itemMenuOpen 
		&& selectedChestSlot[player] < 0 && selectedShopSlot[player] < 0
		&& GenericGUI[player].selectedSlot < 0 )
	{
		//Highlight (draw a gold border) currently selected inventory slot (for gamepad).
		//Only if item menu is not open, no chest slot is selected, no shop slot is selected.

		if ( frame )
		{
			if ( frame->findFrame("inventory selected item") )
			{
				auto selectedSlotFrame = frame->findFrame("inventory selected item");
				selectedSlotFrame->setDisabled(true);
				auto oldSelectedSlotFrame = frame->findFrame("inventory old selected item");
				oldSelectedSlotFrame->setDisabled(true);

				for ( int x = 0; x < players[player]->inventoryUI.getSizeX(); ++x )
				{
					for ( int y = 0; y < players[player]->inventoryUI.getSizeY(); ++y )
					{
						char slotname[32] = "";
						snprintf(slotname, sizeof(slotname), "slot %d %d", x, y);
						auto slotFrame = frame->findFrame(slotname);
						if ( slotFrame )
						{
							if ( slotFrame->capturesMouse() )
							{
								players[player]->inventoryUI.selectSlot(x, y);
								if ( hotbar_t.hotbarHasFocus && !disableMouseDisablingHotbarFocus )
								{
									hotbar_t.hotbarHasFocus = false; //Utter bodge to fix hotbar nav on OS X.
								}
							}

							auto invSlotsFrame = frame->findFrame("inventory slots");
							int startx = invSlotsFrame->getSize().x + slotFrame->getSize().x;
							int starty = invSlotsFrame->getSize().y + slotFrame->getSize().y;

							if ( x == players[player]->inventoryUI.getSelectedSlotX()
								&& y == players[player]->inventoryUI.getSelectedSlotY()
								&& !hotbar_t.hotbarHasFocus )
							{
								if ( !selectedItem )
								{
									selectedSlotFrame->setSize(SDL_Rect{ startx + 1, starty + 1, selectedSlotFrame->getSize().w, selectedSlotFrame->getSize().h });
									selectedSlotFrame->setDisabled(false);
								}
								else
								{
									oldSelectedSlotFrame->setSize(SDL_Rect{ startx + 1, starty + 1, oldSelectedSlotFrame->getSize().w, oldSelectedSlotFrame->getSize().h });
									oldSelectedSlotFrame->setDisabled(false);
								}
							}
						}
					}
				}
			}
		}
		else
		{
			pos.w = inventorySlotSize;
			pos.h = inventorySlotSize;
			for (int x = 0; x < players[player]->inventoryUI.getSizeX(); ++x)
			{
				for (int y = 0; y < players[player]->inventoryUI.getSizeY(); ++y)
				{
					//pos.x = players[player]->inventoryUI.getStartX() + x * inventorySlotSize;
					//pos.y = players[player]->inventoryUI.getStartY() + y * inventorySlotSize;

					//Cursor moved over this slot, highlight it.
					if (mouseInBoundsRealtimeCoords(player, pos.x, pos.x + pos.w, pos.y, pos.y + pos.h))
					{
						players[player]->inventoryUI.selectSlot(x, y);
						if ( hotbar_t.hotbarHasFocus && !disableMouseDisablingHotbarFocus )
						{
							hotbar_t.hotbarHasFocus = false; //Utter bodge to fix hotbar nav on OS X.
						}
					}

					if ( x == players[player]->inventoryUI.getSelectedSlotX() 
						&& y == players[player]->inventoryUI.getSelectedSlotY()
						&& !hotbar_t.hotbarHasFocus )
					{
						Uint32 color = SDL_MapRGBA(mainsurface->format, 255, 255, 0, 127);
						drawBox(&pos, color, 127);
					}
				}
			}
		}
	}

	players[player]->paperDoll.drawSlots();

	// dragging item - highlight slots
	if ( frame )
	{
		if ( auto selectedSlotFrame = frame->findFrame("inventory selected item") )
		{
			for ( int x = 0; x < players[player]->inventoryUI.getSizeX(); ++x )
			{
				for ( int y = Player::Inventory_t::DOLL_ROW_1; y < players[player]->inventoryUI.getSizeY(); ++y )
				{
					char slotname[32] = "";
					snprintf(slotname, sizeof(slotname), "slot %d %d", x, y);
					auto slotFrame = frame->findFrame(slotname);
					if ( !slotFrame )
					{
						continue;
					}

					auto invSlotsFrame = frame->findFrame("inventory slots");
					int startx = invSlotsFrame->getSize().x + slotFrame->getSize().x;
					int starty = invSlotsFrame->getSize().y + slotFrame->getSize().y;
					if ( y >= Player::Inventory_t::DOLL_ROW_1 && y <= Player::Inventory_t::DOLL_ROW_5 )
					{
						startx = slotFrame->getSize().x;
						starty = slotFrame->getSize().y;
					}

					if ( !slotFrame->capturesMouse()
						&& slotFrame->capturesMouseInRealtimeCoords()
						&& selectedSlotFrame->isDisabled() )
					{
						// if dragging item, use realtime coords to find what's selected.
						selectedSlotFrame->setSize(SDL_Rect{ startx + 1, starty + 1, selectedSlotFrame->getSize().w, selectedSlotFrame->getSize().h });
						selectedSlotFrame->setDisabled(false);
					}

				}
			}
		}
	}
	
	// draw contents of each slot
	for ( node = stats[player]->inventory.first; node != NULL; node = nextnode )
	{
		nextnode = node->next;
		Item* item = (Item*)node->element;
		if ( !item ) { continue; }


		if ( frame )
		{
			if ( item == selectedItem
				|| (players[player]->inventory_mode == INVENTORY_MODE_ITEM && itemCategory(item) == SPELL_CAT)
				|| (players[player]->inventory_mode == INVENTORY_MODE_SPELL && itemCategory(item) != SPELL_CAT) )
			{
				//Item is selected, or, item is a spell but it's item inventory mode, or, item is an item but it's spell inventory mode...(this filters out items)
				if ( !(players[player]->inventory_mode == INVENTORY_MODE_ITEM && itemCategory(item) == SPELL_CAT)
					|| (players[player]->inventory_mode == INVENTORY_MODE_SPELL && itemCategory(item) != SPELL_CAT) )
				{
					if ( item == selectedItem )
					{
						//Draw blue border around the slot if it's the currently grabbed item.
						//drawBlueInventoryBorder(player, *item, x, y);
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
			
			if ( itemx >= 0 && itemx < players[player]->inventoryUI.getSizeX()
				&& itemy >= Player::Inventory_t::PaperDollRows::DOLL_ROW_1 && itemy < players[player]->inventoryUI.getSizeY() )
			{
				char slotname[32] = "";
				snprintf(slotname, sizeof(slotname), "slot %d %d", itemx, itemy);
				auto slotFrame = frame->findFrame(slotname);

				updateSlotFrameFromItem(slotFrame, item);
			}
		}
		else
		{
			if ( item == selectedItem
				|| (players[player]->inventory_mode == INVENTORY_MODE_ITEM && itemCategory(item) == SPELL_CAT)
				|| (players[player]->inventory_mode == INVENTORY_MODE_SPELL && itemCategory(item) != SPELL_CAT) )
			{
				//Item is selected, or, item is a spell but it's item inventory mode, or, item is an item but it's spell inventory mode...(this filters out items)
				if ( !(players[player]->inventory_mode == INVENTORY_MODE_ITEM && itemCategory(item) == SPELL_CAT)
					|| (players[player]->inventory_mode == INVENTORY_MODE_SPELL && itemCategory(item) != SPELL_CAT) )
				{
					if ( item == selectedItem )
					{
						//Draw blue border around the slot if it's the currently grabbed item.
						drawBlueInventoryBorder(player, *item, 0, 0/*x, y*/);
					}
				}
				continue;
			}

			int itemDrawnSlotSize = inventorySlotSize;
			int itemCoordX = /*x +*/ item->x * itemDrawnSlotSize;
			int itemCoordY = /*y +*/ item->y * itemDrawnSlotSize;

			bool itemOnPaperDoll = false;
			if ( players[player]->paperDoll.enabled && itemIsEquipped(item, player) )
			{
				auto slotType = players[player]->paperDoll.getSlotForItem(*item);
				if ( slotType != Player::PaperDoll_t::SLOT_MAX )
				{
					itemOnPaperDoll = true;
					auto& paperDollSlot = players[player]->paperDoll.dollSlots[slotType];
					//itemCoordX = paperDollSlot.pos.x;
					//itemCoordY = paperDollSlot.pos.y;
					//itemDrawnSlotSize = paperDollSlot.pos.w;
				}
			}

			pos.x = itemCoordX + 2;
			if ( itemOnPaperDoll )
			{
				pos.x -= 1; // outline here is thinner
			}

			pos.y = itemCoordY + 1;
			pos.w = (itemDrawnSlotSize)-2;
			pos.h = (itemDrawnSlotSize)-2;

			if ( !item->identified )
			{
				// give it a yellow background if it is unidentified
				drawRect(&pos, SDL_MapRGB(mainsurface->format, 128, 128, 0), 125); //31875
			}
			else if ( item->beatitude < 0 )
			{
				// give it a red background if cursed
				drawRect(&pos, SDL_MapRGB(mainsurface->format, 128, 0, 0), 125);
			}
			else if ( item->beatitude > 0 )
			{
				// give it a green background if blessed (light blue if colorblind mode)
				if ( colorblind )
				{
					drawRect(&pos, SDL_MapRGB(mainsurface->format, 100, 245, 255), 65);
				}
				else
				{
					drawRect(&pos, SDL_MapRGB(mainsurface->format, 0, 255, 0), 65);
				}
			}
			if ( item->status == BROKEN )
			{
				drawRect(&pos, SDL_MapRGB(mainsurface->format, 160, 160, 160), 64);
			}

			if ( itemMenuOpen && item == uidToItem(itemMenuItem) )
			{
				//Draw blue border around the slot if it's the currently context menu'd item.
				drawBlueInventoryBorder(player, *item, 0, 0/*x, y*/);
			}

			// draw item
			real_t itemUIScale = uiscale_inventory;
			if ( itemOnPaperDoll )
			{
				itemUIScale = 1.0;
			}

			pos.x = itemCoordX + 4 * itemUIScale;
			pos.y = itemCoordY + 4 * itemUIScale;
			if ( !itemOnPaperDoll )
			{
				pos.w = (32) * itemUIScale;
				pos.h = (32) * itemUIScale;
			}
			else
			{
				pos.w = (itemDrawnSlotSize * 0.8) * itemUIScale;
				pos.h = (itemDrawnSlotSize * 0.8) * itemUIScale;
			}

			if ( itemSprite(item) )
			{
				drawImageScaled(itemSprite(item), NULL, &pos);
			}

			bool greyedOut = false;
			if ( players[player] && players[player]->entity && players[player]->entity->effectShapeshift != NOTHING )
			{
				// shape shifted, disable some items
				if ( !item->usableWhileShapeshifted(stats[player]) )
				{
					SDL_Rect greyBox;
					greyBox.x = itemCoordX + 2;
					greyBox.y = itemCoordY + 1;
					greyBox.w = (itemDrawnSlotSize)-2;
					greyBox.h = (itemDrawnSlotSize)-2;
					drawRect(&greyBox, SDL_MapRGB(mainsurface->format, 64, 64, 64), 144);
					greyedOut = true;
				}
			}
			if ( !greyedOut && client_classes[player] == CLASS_SHAMAN
				&& item->type == SPELL_ITEM && !(playerUnlockedShamanSpell(player, item)) )
			{
				SDL_Rect greyBox;
				greyBox.x = itemCoordX + 2;
				greyBox.y = itemCoordY + 1;
				greyBox.w = (itemDrawnSlotSize)-2;
				greyBox.h = (itemDrawnSlotSize)-2;
				drawRect(&greyBox, SDL_MapRGB(mainsurface->format, 64, 64, 64), 144);
				greyedOut = true;
			}

			// item count
			if ( item->count > 1 )
			{
				if ( itemUIScale < 1.5 )
				{
					printTextFormatted(font8x8_bmp, pos.x + pos.w - 8 * itemUIScale, pos.y + pos.h - 8 * itemUIScale, "%d", item->count);
				}
				else
				{
					printTextFormatted(font12x12_bmp, pos.x + pos.w - 12, pos.y + pos.h - 12, "%d", item->count);
				}
			}

			// item equipped
			if ( itemCategory(item) != SPELL_CAT )
			{
				if ( itemIsEquipped(item, player) )
				{
					if ( !itemOnPaperDoll )
					{
						pos.x = itemCoordX + 2;
						pos.y = itemCoordY + itemDrawnSlotSize - 18;
						pos.w = 16;
						pos.h = 16;
						drawImage(equipped_bmp, NULL, &pos);
					}
				}
				else if ( item->status == BROKEN )
				{
					pos.x = itemCoordX + 2;
					pos.y = itemCoordY + itemDrawnSlotSize - 18;
					pos.w = 16;
					pos.h = 16;
					drawImage(itembroken_bmp, NULL, &pos);
				}
			}
			else
			{
				spell_t* spell = getSpellFromItem(player, item);
				if ( players[player]->magic.selectedSpell() == spell
					&& (players[player]->magic.selected_spell_last_appearance == item->appearance || players[player]->magic.selected_spell_last_appearance == -1) )
				{
					pos.x = itemCoordX + 2;
					pos.y = itemCoordY + itemDrawnSlotSize - 18;
					pos.w = 16;
					pos.h = 16;
					drawImage(equipped_bmp, NULL, &pos);
				}
			}
		}
	}
	// autosort button
	//mode_pos.x = players[player]->inventoryUI.getStartX() + players[player]->inventoryUI.getSizeX() * inventorySlotSize + inventory_mode_item_img->w * uiscale_inventory + 2;
	//mode_pos.y = players[player]->inventoryUI.getStartY();
	if ( players[player]->inventoryUI.bNewInventoryLayout )
	{
		// draw halfway down
		mode_pos.y += (players[player]->inventoryUI.getSizeY() / 2) * inventorySlotSize;
	}
	mode_pos.w = 24;
	mode_pos.h = 24;
	bool mouse_in_bounds = mouseInBounds(player, mode_pos.x, mode_pos.x + mode_pos.w, mode_pos.y, mode_pos.y + mode_pos.h);
	if ( !mouse_in_bounds )
	{
		drawWindow(mode_pos.x, mode_pos.y, mode_pos.x + mode_pos.w, mode_pos.y + mode_pos.h);
	}
	else
	{
		drawDepressed(mode_pos.x, mode_pos.y, mode_pos.x + mode_pos.w, mode_pos.y + mode_pos.h);
	}
	ttfPrintText(ttf12, mode_pos.x, mode_pos.y + 6, "||");
	if ( mouse_in_bounds )
	{
		mode_pos.x += 2;
		mode_pos.y += 2;
		mode_pos.w -= 4;
		mode_pos.h -= 4;
		drawRect(&mode_pos, SDL_MapRGB(mainsurface->format, 192, 192, 192), 64);
		// tooltip
		SDL_Rect src;
		src.x = mousex + 16;
		src.y = mousey + 8;
		src.h = TTF12_HEIGHT + 8;
		src.w = longestline(language[2960]) * TTF12_WIDTH + 8;
		drawTooltip(&src);
		ttfPrintTextFormatted(ttf12, src.x + 4, src.y + 4, language[2960], getInputName(impulses[IN_AUTOSORT]));
		if ( inputs.bMouseLeft(player) )
		{
			inputs.mouseClearLeft(player);
			autosortInventory(player);
			playSound(139, 64);
		}
	}
	// do inventory mode buttons
	//mode_pos.x = players[player]->inventoryUI.getStartX() + players[player]->inventoryUI.getSizeX() * inventorySlotSize + 1;
	//mode_pos.y = players[player]->inventoryUI.getStartY() + inventory_mode_spell_img->h * uiscale_inventory;
	if ( players[player]->inventoryUI.bNewInventoryLayout )
	{
		// draw halfway down
		mode_pos.y += (players[player]->inventoryUI.getSizeY() / 2) * inventorySlotSize;
	}
	mode_pos.w = inventory_mode_spell_img->w * uiscale_inventory;
	mode_pos.h = inventory_mode_spell_img->h * uiscale_inventory + 1;
	mouse_in_bounds = mouseInBounds(player, mode_pos.x, mode_pos.x + mode_pos.w,
		mode_pos.y, mode_pos.y + mode_pos.h);
	if (mouse_in_bounds)
	{
		drawImageScaled(inventory_mode_spell_highlighted_img, NULL, &mode_pos);

		// tooltip
		SDL_Rect src;
		src.x = mousex + 16;
		src.y = mousey + 8;
		src.h = TTF12_HEIGHT + 8;
		src.w = longestline(language[342]) * TTF12_WIDTH + 8;
		drawTooltip(&src);
		ttfPrintText(ttf12, src.x + 4, src.y + 4, language[342]);

		if ( inputs.bMouseLeft(player) )
		{
			inputs.mouseClearLeft(player);
			players[player]->inventory_mode = INVENTORY_MODE_SPELL;
			playSound(139, 64);
		}
	}
	else
	{
		drawImageScaled(inventory_mode_spell_img, NULL, &mode_pos);
	}
	//mode_pos.x = players[player]->inventoryUI.getStartX() + players[player]->inventoryUI.getSizeX() * inventorySlotSize + 1;
	//mode_pos.y = players[player]->inventoryUI.getStartY() - 1;
	if ( players[player]->inventoryUI.bNewInventoryLayout )
	{
		// draw halfway down
		mode_pos.y += (players[player]->inventoryUI.getSizeY() / 2) * inventorySlotSize;
	}
	mode_pos.w = inventory_mode_item_img->w * uiscale_inventory;
	mode_pos.h = inventory_mode_item_img->h * uiscale_inventory + 2;
	mouse_in_bounds = mouseInBounds(player, mode_pos.x, mode_pos.x + mode_pos.w,
		mode_pos.y, mode_pos.y + mode_pos.h);
	if (mouse_in_bounds)
	{
		drawImageScaled(inventory_mode_item_highlighted_img, NULL, &mode_pos);

		// tooltip
		SDL_Rect src;
		src.x = mousex + 16;
		src.y = mousey + 8;
		src.h = TTF12_HEIGHT + 8;
		src.w = longestline(language[343]) * TTF12_WIDTH + 8;
		drawTooltip(&src);
		ttfPrintText(ttf12, src.x + 4, src.y + 4, language[343]);

		if ( inputs.bMouseLeft(player) )
		{
			inputs.mouseClearLeft(player);
			players[player]->inventory_mode = INVENTORY_MODE_ITEM;
			playSound(139, 64);
		}
	}
	else
	{
		drawImageScaled(inventory_mode_item_img, NULL, &mode_pos);
	}

	// mouse interactions
	if ( !selectedItem )
	{
		for ( node = stats[player]->inventory.first; node != NULL; node = nextnode )
		{
			nextnode = node->next;
			Item* item = (Item*)node->element;  //I don't like that there's not a check that either are null.

			if (item)
			{
				int itemCoordX = 0;
				int itemCoordY = 0;

				bool mouseOverSlot = false;
				int itemDrawnSlotSize = inventorySlotSize;
				itemCoordX = /*x +*/ item->x * itemDrawnSlotSize;
				itemCoordY = /*y +*/ item->y * itemDrawnSlotSize;

				bool itemOnPaperDoll = false;
				if ( players[player]->paperDoll.enabled && itemIsEquipped(item, player) )
				{
					auto slotType = players[player]->paperDoll.getSlotForItem(*item);
					if ( slotType != Player::PaperDoll_t::SLOT_MAX )
					{
						itemOnPaperDoll = true;
						auto& paperDollSlot = players[player]->paperDoll.dollSlots[slotType];
						//itemCoordX = paperDollSlot.pos.x;
						//itemCoordY = paperDollSlot.pos.y;
						//itemDrawnSlotSize = paperDollSlot.pos.w;
					}
				}

				pos.x = itemCoordX + 4;
				pos.y = itemCoordY + 4;
				pos.w = itemDrawnSlotSize - 8;
				pos.h = itemDrawnSlotSize - 8;

				mouseOverSlot = (omousex >= pos.x && omousey >= pos.y && omousex < pos.x + pos.w && omousey < pos.y + pos.h);

				if ( mouseOverSlot )
				{
					// tooltip
					if ((players[player]->inventory_mode == INVENTORY_MODE_ITEM && itemCategory(item) == SPELL_CAT) 
						|| (players[player]->inventory_mode == INVENTORY_MODE_SPELL && itemCategory(item) != SPELL_CAT))
					{
						continue;    //Skip over this items since the filter is blocking it (eg spell in normal inventory or vice versa).
					}
					if ( !itemMenuOpen )
					{
						SDL_Rect src;
						src.x = mousex + 16;
						src.y = mousey + 8;
						if (itemCategory(item) == SPELL_CAT)
						{
							if ( frame )
							{
								players[player]->hud.updateFrameTooltip(item, itemCoordX, itemCoordY);
							}
							spell_t* spell = getSpellFromItem(player, item);
							drawSpellTooltip(player, spell, item, nullptr);
						}
						else
						{
							if ( frame )
							{
								players[player]->hud.updateFrameTooltip(item, itemCoordX, itemCoordY);
							}
							else
							{
								drawItemTooltip(player, item, src);
							}
						}
					}

					if ( stats[player]->HP <= 0 )
					{
						break;
					}

					if ( inputs.bControllerInputPressed(player, INJOY_MENU_DROP_ITEM)
						&& !itemMenuOpen && !selectedItem && selectedChestSlot[player] < 0
						&& selectedShopSlot[player] < 0
						&& GenericGUI[player].selectedSlot < 0 )
					{
						inputs.controllerClearInput(player, INJOY_MENU_DROP_ITEM);
						if ( dropItem(item, player) )
						{
							item = nullptr;
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
					if ( (inputs.bMouseLeft(player)
						|| (inputs.bControllerInputPressed(player, INJOY_MENU_LEFT_CLICK)
							&& selectedChestSlot[player] < 0 && selectedShopSlot[player] < 0
							&& GenericGUI[player].selectedSlot < 0))
						&& !selectedItem && !itemMenuOpen )
					{
						if ( !(inputs.bControllerInputPressed(player, INJOY_MENU_LEFT_CLICK)) && (keystatus[SDL_SCANCODE_LSHIFT] || keystatus[SDL_SCANCODE_RSHIFT]) )
						{
							if ( dropItem(item, player) ) // Quick item drop
							{
								item = nullptr;
							}
						}
						else
						{
							selectedItem = item;
							playSound(139, 64); // click sound

							toggleclick = false; //Default reset. Otherwise will break mouse support after using gamepad once to trigger a context menu.

							if ( inputs.bControllerInputPressed(player, INJOY_MENU_LEFT_CLICK) )
							{
								inputs.controllerClearInput(player, INJOY_MENU_LEFT_CLICK);
								toggleclick = true;
								inputs.mouseClearLeft(player);
								//TODO: Change the mouse cursor to THE HAND.
							}
						}
					}
					else if ( (inputs.bMouseRight(player)
						|| (inputs.bControllerInputPressed(player, INJOY_MENU_USE)
							&& selectedChestSlot[player] < 0 && selectedShopSlot[player] < 0
							&& GenericGUI[player].selectedSlot < 0))
						&& !itemMenuOpen && !selectedItem )
					{
						if ( (keystatus[SDL_SCANCODE_LSHIFT] || keystatus[SDL_SCANCODE_RSHIFT]) && !(inputs.bControllerInputPressed(player, INJOY_MENU_USE) && selectedChestSlot[player] < 0) ) //TODO: selected shop slot, identify, remove curse?
						{
							// auto-appraise the item
							players[player]->inventoryUI.appraisal.appraiseItem(item);
							inputs.mouseClearRight(player);
						}
						else if ( !disableItemUsage && (itemCategory(item) == POTION || itemCategory(item) == SPELLBOOK || item->type == FOOD_CREAMPIE) &&
							(keystatus[SDL_SCANCODE_LALT] || keystatus[SDL_SCANCODE_RALT]) 
							&& !(inputs.bControllerInputPressed(player, INJOY_MENU_USE)) )
						{
							inputs.mouseClearRight(player);
							// force equip potion/spellbook
							playerTryEquipItemAndUpdateServer(player, item, false);
						}
						else
						{
							// open a drop-down menu of options for "using" the item
							itemMenuOpen = true;
							itemMenuX = mousex + 8;
							itemMenuY = mousey;
							itemMenuSelected = 0;
							itemMenuItem = item->uid;

							toggleclick = false; //Default reset. Otherwise will break mouse support after using gamepad once to trigger a context menu.

							if ( inputs.bControllerInputPressed(player, INJOY_MENU_USE) )
							{
								inputs.controllerClearInput(player, INJOY_MENU_USE);
								toggleclick = true;
							}
						}
					}

					bool numkey_quick_add = hotbar_numkey_quick_add;
					if ( item && itemCategory(item) == SPELL_CAT && item->appearance >= 1000 &&
						players[player] && players[player]->entity && players[player]->entity->effectShapeshift )
					{
						if ( canUseShapeshiftSpellInCurrentForm(player, *item) != 1 )
						{
							numkey_quick_add = false;
						}
					}

					if ( numkey_quick_add && !command )
					{
						if ( keystatus[SDL_SCANCODE_1] )
						{
							keystatus[SDL_SCANCODE_1] = 0;
							hotbar[0].item = item->uid;
						}
						if ( keystatus[SDL_SCANCODE_2] )
						{
							keystatus[SDL_SCANCODE_2] = 0;
							hotbar[1].item = item->uid;
						}
						if ( keystatus[SDL_SCANCODE_3] )
						{
							keystatus[SDL_SCANCODE_3] = 0;
							hotbar[2].item = item->uid;
						}
						if ( keystatus[SDL_SCANCODE_4] )
						{
							keystatus[SDL_SCANCODE_4] = 0;
							hotbar[3].item = item->uid;
						}
						if ( keystatus[SDL_SCANCODE_5] )
						{
							keystatus[SDL_SCANCODE_5] = 0;
							hotbar[4].item = item->uid;
						}
						if ( keystatus[SDL_SCANCODE_6] )
						{
							keystatus[SDL_SCANCODE_6] = 0;
							hotbar[5].item = item->uid;
						}
						if ( keystatus[SDL_SCANCODE_7] )
						{
							keystatus[SDL_SCANCODE_7] = 0;
							hotbar[6].item = item->uid;
						}
						if ( keystatus[SDL_SCANCODE_8] )
						{
							keystatus[SDL_SCANCODE_8] = 0;
							hotbar[7].item = item->uid;
						}
						if ( keystatus[SDL_SCANCODE_9] )
						{
							keystatus[SDL_SCANCODE_9] = 0;
							hotbar[8].item = item->uid;
						}
						if ( keystatus[SDL_SCANCODE_0] )
						{
							keystatus[SDL_SCANCODE_0] = 0;
							hotbar[9].item = item->uid;
						}
					}
					break;
				}
			}
		}
	}
	else if ( stats[player]->HP > 0 )
	{
		// releasing items
		releaseItem(player);
	}

	itemContextMenu(player);
}

void Player::Inventory_t::updateInventory()
{
	const int player = this->player.playernum;
	assert(frame);
	assert(tooltipFrame);

	if ( nohud || !players[player]->isLocalPlayer() || players[player]->shootmode
		|| !(players[player]->gui_mode == GUI_MODE_INVENTORY || players[player]->gui_mode == GUI_MODE_SHOP) )
	{
		// hide
		frame->setDisabled(true);
		updateItemContextMenu(); // process + close the item context menu
		return;
	}

	frame->setDisabled(false);

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
	Item*& selectedItem = inputs.getUIInteraction(player)->selectedItem;

	if ( inputs.hasController(player) )
	{
		bool radialMenuOpen = FollowerMenu[player].followerMenuIsOpen();
		if ( players[player]->gui_mode == GUI_MODE_SHOP )
		{
			if ( inputs.bControllerInputPressed(player, INJOY_MENU_CYCLE_SHOP_LEFT) )
			{
				inputs.controllerClearInput(player, INJOY_MENU_CYCLE_SHOP_LEFT);
				cycleShopCategories(player, -1);
			}
			if ( inputs.bControllerInputPressed(player, INJOY_MENU_CYCLE_SHOP_RIGHT) )
			{
				inputs.controllerClearInput(player, INJOY_MENU_CYCLE_SHOP_RIGHT);
				cycleShopCategories(player, 1);
			}
		}

		if ( radialMenuOpen )
		{
			// do nothing?
		}
		else if ( selectedChestSlot[player] < 0 && selectedShopSlot[player] < 0
			&& !itemMenuOpen && GenericGUI[player].selectedSlot < 0
			&& players[player]->GUI.handleInventoryMovement() ) // handleInventoryMovement should be at the end of this check
		{
			if ( selectedChestSlot[player] < 0 && selectedShopSlot[player] < 0
				&& GenericGUI[player].selectedSlot < 0 ) //This second check prevents the extra mouse warp.
			{
				if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_INVENTORY )
				{
					warpMouseToSelectedInventorySlot(player);
				}
				else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_HOTBAR )
				{
					disableMouseDisablingHotbarFocus = true;
				}
			}
		}
		else if ( selectedChestSlot[player] >= 0 && !itemMenuOpen && inputs.getController(player)->handleChestMovement(player) )
		{
			if ( selectedChestSlot[player] < 0 )
			{
				//Move out of chest. Warp cursor back to selected inventory slot.
				warpMouseToSelectedInventorySlot(player);
			}
		}
		else if ( selectedShopSlot[player] >= 0 && !itemMenuOpen && inputs.getController(player)->handleShopMovement(player) )
		{
			if ( selectedShopSlot[player] < 0 )
			{
				warpMouseToSelectedInventorySlot(player);
			}
		}
		else if ( GenericGUI[player].selectedSlot >= 0 && !itemMenuOpen && inputs.getController(player)->handleRepairGUIMovement(player) )
		{
			if ( GenericGUI[player].selectedSlot < 0 )
			{
				warpMouseToSelectedInventorySlot(player);
			}
		}

		if ( inputs.bControllerInputPressed(player, INJOY_MENU_INVENTORY_TAB) )
		{
			inputs.controllerClearInput(player, INJOY_MENU_INVENTORY_TAB);
			cycleInventoryTab(player);
		}

		if ( lastkeypressed == 300 )
		{
			lastkeypressed = 0;
		}

		if ( inputs.bControllerInputPressed(player, INJOY_MENU_MAGIC_TAB) )
		{
			inputs.controllerClearInput(player, INJOY_MENU_MAGIC_TAB);
			cycleInventoryTab(player);
		}
	}

	if ( !command
		&& (*inputPressedForPlayer(player, impulses[IN_AUTOSORT])
			|| (inputs.bControllerInputPressed(player, INJOY_MENU_CHEST_GRAB_ALL) && !openedChest[player])
			)
		)
	{
		autosortInventory(player);
		//quickStackItems();
		*inputPressedForPlayer(player, impulses[IN_AUTOSORT]) = 0;
		inputs.controllerClearInput(player, INJOY_MENU_CHEST_GRAB_ALL);
		playSound(139, 64);
	}

	resetInventorySlotFrames(player);


	auto invSlotsFrame = frame->findFrame("inventory slots");
	auto dollSlotsFrame = frame->findFrame("paperdoll slots");

	auto selectedSlotFrame = frame->findFrame("inventory selected item");
	auto selectedSlotCursor = selectedItemCursorFrame;

	if ( selectedChestSlot[player] < 0 && selectedShopSlot[player] < 0
		&& GenericGUI[player].selectedSlot < 0 )
	{
		//Highlight (draw a gold border) currently selected inventory slot (for gamepad).
		//Only if item menu is not open, no chest slot is selected, no shop slot is selected.

		for ( int x = 0; x < getSizeX(); ++x )
		{
			for ( int y = Player::Inventory_t::DOLL_ROW_1; y < getSizeY(); ++y )
			{
				if ( auto slotFrame = getInventorySlotFrame(x, y) )
				{
					if ( !itemMenuOpen ) // don't update selected slot while item menu open
					{
						if ( slotFrame->capturesMouseInRealtimeCoords() )
						{
							selectSlot(x, y);
							if ( inputs.getVirtualMouse(player)->draw_cursor )
							{
								// mouse movement captures the inventory
								players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
							}
							/*if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_HOTBAR && !disableMouseDisablingHotbarFocus )
							{
								players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
							}*/
						}
					}

					int startx = slotFrame->getAbsoluteSize().x;
					int starty = slotFrame->getAbsoluteSize().y;
					startx -= players[player]->camera_virtualx1(); // offset any splitscreen camera positioning.
					starty -= players[player]->camera_virtualy1();

					if ( x == getSelectedSlotX()
						&& y == getSelectedSlotY()
						&& players[player]->GUI.activeModule == Player::GUI_t::MODULE_INVENTORY )
					{
						if ( itemMenuOpen || // if item menu open, then always draw cursor on current item.
							(!selectedItem	// otherwise, if no selected item, and mouse hovering over item
								&& (!inputs.getVirtualMouse(player)->draw_cursor 
								|| (inputs.getVirtualMouse(player)->draw_cursor && slotFrame->capturesMouse()))) )
						{
							selectedSlotFrame->setSize(SDL_Rect{ startx + 1, starty + 1, selectedSlotFrame->getSize().w, selectedSlotFrame->getSize().h });
							selectedSlotFrame->setDisabled(false);

							selectedSlotCursor->setDisabled(false);
							updateSelectedSlotAnimation(startx, starty, getSlotSize(), getSlotSize(), inputs.getVirtualMouse(player)->draw_cursor);
							//messagePlayer(0, "0: %d, %d", x, y);
						}
					}
				}
			}
		}
	}

	players[player]->paperDoll.drawSlots();

	// dragging item - highlight slots
	if ( selectedItem && selectedSlotFrame->isDisabled() )
	{
		Frame* hoveringDollSlotFrame = nullptr;
		int loopStartY = 0;
		if ( inputs.getVirtualMouse(player)->draw_cursor )
		{
			if ( dollSlotsFrame && dollSlotsFrame->capturesMouseInRealtimeCoords() )
			{
				auto slotName = items[selectedItem->type].item_slot;
				Player::PaperDoll_t::PaperDollSlotType dollSlot = getPaperDollSlotFromItemType(*selectedItem);
				if ( dollSlot != Player::PaperDoll_t::PaperDollSlotType::SLOT_MAX )
				{
					// get coordinates of doll slot
					int slotFrameX, slotFrameY;
					players[player]->paperDoll.getCoordinatesFromSlotType(dollSlot, slotFrameX, slotFrameY);
					hoveringDollSlotFrame = getInventorySlotFrame(slotFrameX, slotFrameY);
				}
			}
		}
		else
		{
			loopStartY = DOLL_ROW_1;
		}

		for ( int x = 0; x < getSizeX(); ++x )
		{
			for ( int y = loopStartY; y < getSizeY(); ++y )
			{
				auto slotFrame = getInventorySlotFrame(x, y);
				if ( hoveringDollSlotFrame )
				{
					slotFrame = dollSlotsFrame;
					--y; // one-off check
				}

				if ( !slotFrame )
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
				else if ( slotFrame->capturesMouse() && x == getSelectedSlotX() && y == getSelectedSlotY() )
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

	// draw contents of each slot
	auto oldSelectedSlotFrame = frame->findFrame("inventory old selected item");
	oldSelectedSlotFrame->setDisabled(true);
	for ( node = stats[player]->inventory.first; node != NULL; node = nextnode )
	{
		nextnode = node->next;
		Item* item = (Item*)node->element;
		if ( !item ) { continue; }

		if ( item == selectedItem
			|| (players[player]->inventory_mode == INVENTORY_MODE_ITEM && itemCategory(item) == SPELL_CAT)
			|| (players[player]->inventory_mode == INVENTORY_MODE_SPELL && itemCategory(item) != SPELL_CAT) )
		{
			//Item is selected, or, item is a spell but it's item inventory mode, or, item is an item but it's spell inventory mode...(this filters out items)
			if ( !(players[player]->inventory_mode == INVENTORY_MODE_ITEM && itemCategory(item) == SPELL_CAT)
				|| (players[player]->inventory_mode == INVENTORY_MODE_SPELL && itemCategory(item) != SPELL_CAT) )
			{
				if ( item == selectedItem )
				{
					//Draw blue border around the slot if it's the currently grabbed item.
					//drawBlueInventoryBorder(player, *item, x, y);
					Frame* slotFrame = nullptr;

					SDL_Rect borderPos{ 0, 0, oldSelectedSlotFrame->getSize().w, oldSelectedSlotFrame->getSize().h };
					if ( players[player]->paperDoll.getSlotForItem(*item) != Player::PaperDoll_t::SLOT_MAX )
					{
						int slotx, sloty;
						this->player.paperDoll.getCoordinatesFromSlotType(players[player]->paperDoll.getSlotForItem(*item), slotx, sloty);
						if ( slotFrame = getInventorySlotFrame(slotx, sloty) )
						{
							borderPos.x = dollSlotsFrame->getSize().x + slotFrame->getSize().x;
							borderPos.y = dollSlotsFrame->getSize().y + slotFrame->getSize().y;
						}
					}
					else
					{
						if ( slotFrame = getInventorySlotFrame(selectedItem->x, selectedItem->y) )
						{
							borderPos.x = invSlotsFrame->getSize().x + slotFrame->getSize().x;
							borderPos.y = invSlotsFrame->getSize().y + slotFrame->getSize().y;
						}
					}
					if ( slotFrame )
					{
						++borderPos.x;
						++borderPos.y;

						oldSelectedSlotFrame->setDisabled(false);
						oldSelectedSlotFrame->setSize(borderPos);

						auto oldSelectedSlotItem = oldSelectedSlotFrame->findImage("inventory old selected item");
						oldSelectedSlotItem->disabled = false;
						oldSelectedSlotItem->path = getItemSpritePath(player, *item);
					}
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

		if ( itemx >= 0 && itemx < getSizeX()
			&& itemy >= Player::Inventory_t::PaperDollRows::DOLL_ROW_1 && itemy < getSizeY() )
		{
			if ( auto slotFrame = getInventorySlotFrame(itemx, itemy) )
			{
				updateSlotFrameFromItem(slotFrame, item);
			}
		}
	}

	// autosort button
	//mode_pos.x = getStartX() + getSizeX() * inventorySlotSize + inventory_mode_item_img->w * uiscale_inventory + 2;
	//mode_pos.y = getStartY();
	if ( bNewInventoryLayout )
	{
		// draw halfway down
		mode_pos.y += (getSizeY() / 2) * inventorySlotSize;
	}
	mode_pos.w = 24;
	mode_pos.h = 24;
	bool mouse_in_bounds = mouseInBounds(player, mode_pos.x, mode_pos.x + mode_pos.w, mode_pos.y, mode_pos.y + mode_pos.h);
	if ( !mouse_in_bounds )
	{
		drawWindow(mode_pos.x, mode_pos.y, mode_pos.x + mode_pos.w, mode_pos.y + mode_pos.h);
	}
	else
	{
		drawDepressed(mode_pos.x, mode_pos.y, mode_pos.x + mode_pos.w, mode_pos.y + mode_pos.h);
	}
	ttfPrintText(ttf12, mode_pos.x, mode_pos.y + 6, "||");
	if ( mouse_in_bounds )
	{
		mode_pos.x += 2;
		mode_pos.y += 2;
		mode_pos.w -= 4;
		mode_pos.h -= 4;
		drawRect(&mode_pos, SDL_MapRGB(mainsurface->format, 192, 192, 192), 64);
		// tooltip
		SDL_Rect src;
		src.x = mousex + 16;
		src.y = mousey + 8;
		src.h = TTF12_HEIGHT + 8;
		src.w = longestline(language[2960]) * TTF12_WIDTH + 8;
		drawTooltip(&src);
		ttfPrintTextFormatted(ttf12, src.x + 4, src.y + 4, language[2960], getInputName(impulses[IN_AUTOSORT]));
		if ( inputs.bMouseLeft(player) )
		{
			inputs.mouseClearLeft(player);
			autosortInventory(player);
			playSound(139, 64);
		}
	}
	// do inventory mode buttons
	//mode_pos.x = getStartX() + getSizeX() * inventorySlotSize + 1;
	//mode_pos.y = getStartY() + inventory_mode_spell_img->h * uiscale_inventory;
	if ( bNewInventoryLayout )
	{
		// draw halfway down
		mode_pos.y += (getSizeY() / 2) * inventorySlotSize;
	}
	mode_pos.w = inventory_mode_spell_img->w * uiscale_inventory;
	mode_pos.h = inventory_mode_spell_img->h * uiscale_inventory + 1;
	mouse_in_bounds = mouseInBounds(player, mode_pos.x, mode_pos.x + mode_pos.w,
		mode_pos.y, mode_pos.y + mode_pos.h);
	if ( mouse_in_bounds )
	{
		drawImageScaled(inventory_mode_spell_highlighted_img, NULL, &mode_pos);

		// tooltip
		SDL_Rect src;
		src.x = mousex + 16;
		src.y = mousey + 8;
		src.h = TTF12_HEIGHT + 8;
		src.w = longestline(language[342]) * TTF12_WIDTH + 8;
		drawTooltip(&src);
		ttfPrintText(ttf12, src.x + 4, src.y + 4, language[342]);

		if ( inputs.bMouseLeft(player) )
		{
			inputs.mouseClearLeft(player);
			players[player]->inventory_mode = INVENTORY_MODE_SPELL;
			playSound(139, 64);
		}
	}
	else
	{
		drawImageScaled(inventory_mode_spell_img, NULL, &mode_pos);
	}
	//mode_pos.x = getStartX() + getSizeX() * inventorySlotSize + 1;
	//mode_pos.y = getStartY() - 1;
	if ( bNewInventoryLayout )
	{
		// draw halfway down
		mode_pos.y += (getSizeY() / 2) * inventorySlotSize;
	}
	mode_pos.w = inventory_mode_item_img->w * uiscale_inventory;
	mode_pos.h = inventory_mode_item_img->h * uiscale_inventory + 2;
	mouse_in_bounds = mouseInBounds(player, mode_pos.x, mode_pos.x + mode_pos.w,
		mode_pos.y, mode_pos.y + mode_pos.h);
	if ( mouse_in_bounds )
	{
		drawImageScaled(inventory_mode_item_highlighted_img, NULL, &mode_pos);

		// tooltip
		SDL_Rect src;
		src.x = mousex + 16;
		src.y = mousey + 8;
		src.h = TTF12_HEIGHT + 8;
		src.w = longestline(language[343]) * TTF12_WIDTH + 8;
		drawTooltip(&src);
		ttfPrintText(ttf12, src.x + 4, src.y + 4, language[343]);

		if ( inputs.bMouseLeft(player) )
		{
			inputs.mouseClearLeft(player);
			players[player]->inventory_mode = INVENTORY_MODE_ITEM;
			playSound(139, 64);
		}
	}
	else
	{
		drawImageScaled(inventory_mode_item_img, NULL, &mode_pos);
	}

	// mouse interactions
	if ( !selectedItem )
	{
		for ( node = stats[player]->inventory.first; node != NULL; node = nextnode )
		{
			nextnode = node->next;
			Item* item = (Item*)node->element;
			if ( !item )
			{
				continue;
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

			auto slotFrame = getInventorySlotFrame(itemx, itemy);
			if ( !slotFrame ) { continue; }
			mouseOverSlot = slotFrame->capturesMouse();

			if ( mouseOverSlot && inputs.getVirtualMouse(player)->draw_cursor )
			{
				// mouse movement captures the inventory
				players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
			}

			if ( mouseOverSlot && players[player]->GUI.bActiveModuleUsesInventory() )
			{
				auto inventoryBgFrame = frame->findFrame("inventory base");
				int tooltipCoordX = frame->getSize().x + inventoryBgFrame->getSize().x + inventoryBgFrame->getSize().w + 8;
				int tooltipCoordY = frame->getSize().y + slotFrame->getSize().y;
				if ( !itemOnPaperDoll )
				{
					tooltipCoordY += frame->findFrame("inventory slots")->getSize().y;
				}

				// tooltip
				if ( (players[player]->inventory_mode == INVENTORY_MODE_ITEM && itemCategory(item) == SPELL_CAT)
					|| (players[player]->inventory_mode == INVENTORY_MODE_SPELL && itemCategory(item) != SPELL_CAT) )
				{
					continue;    //Skip over this items since the filter is blocking it (eg spell in normal inventory or vice versa).
				}

				bool tooltipOpen = false;
				if ( !itemMenuOpen )
				{
					tooltipOpen = true;
					players[player]->hud.updateFrameTooltip(item, tooltipCoordX, tooltipCoordY);
				}

				if ( stats[player]->HP <= 0 )
				{
					break;
				}

				if ( tooltipOpen && !tooltipPromptFrame->isDisabled() 
					&& !itemMenuOpen && !selectedItem && selectedChestSlot[player] < 0
					&& selectedShopSlot[player] < 0
					&& GenericGUI[player].selectedSlot < 0 )
				{
					auto contextTooltipOptions = getContextTooltipOptionsForItem(player, item);
					bool bindingPressed = false;
					for ( auto& option : contextTooltipOptions )
					{
						if ( Input::inputs[player].binaryToggle(getContextMenuOptionBindingName(option).c_str()) )
						{
							bindingPressed = true;
							activateItemContextMenuOption(item, option);
						}
					}
					if ( bindingPressed )
					{
						for ( auto& option : contextTooltipOptions )
						{
							// clear the other bindings just in case.
							Input::inputs[player].consumeBinaryToggle(getContextMenuOptionBindingName(option).c_str());
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
				if ( inputs.bMouseLeft(player) && !selectedItem && !itemMenuOpen )
				{
					if ( keystatus[SDL_SCANCODE_LSHIFT] || keystatus[SDL_SCANCODE_RSHIFT] )
					{
						if ( dropItem(item, player) ) // Quick item drop
						{
							item = nullptr;
						}
					}
					else
					{
						inputs.getUIInteraction(player)->selectedItemFromHotbar = -1;
						selectedItem = item;
						playSound(139, 64); // click sound

						if ( inputs.getVirtualMouse(player)->draw_cursor )
						{
							cursor.lastUpdateTick = ticks;
						}

						toggleclick = false; //Default reset. Otherwise will break mouse support after using gamepad once to trigger a context menu.
					}
				}
				else if ( inputs.bMouseRight(player)
					&& !itemMenuOpen && !selectedItem )
				{
					if ( (keystatus[SDL_SCANCODE_LSHIFT] || keystatus[SDL_SCANCODE_RSHIFT]) && selectedChestSlot[player] < 0 ) //TODO: selected shop slot, identify, remove curse?
					{
						// auto-appraise the item
						appraisal.appraiseItem(item);
						inputs.mouseClearRight(player);
					}
					else if ( !disableItemUsage && (itemCategory(item) == POTION || itemCategory(item) == SPELLBOOK || item->type == FOOD_CREAMPIE) &&
						(keystatus[SDL_SCANCODE_LALT] || keystatus[SDL_SCANCODE_RALT]) )
					{
						inputs.mouseClearRight(player);
						// force equip potion/spellbook
						playerTryEquipItemAndUpdateServer(player, item, false);
					}
					else
					{
						// open a drop-down menu of options for "using" the item
						itemMenuOpen = true;
						itemMenuX = (inputs.getMouse(player, Inputs::X) / (float)xres) * (float)Frame::virtualScreenX + 8;
						itemMenuY = (inputs.getMouse(player, Inputs::Y) / (float)yres) * (float)Frame::virtualScreenY;
						if ( auto interactMenuTop = interactFrame->findImage("interact top background") )
						{
							// 10px is slot half height, minus the top interact text height
							// mouse will be situated halfway in first menu option
							itemMenuY -= (interactMenuTop->pos.h + 10 + 2);
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

				bool numkey_quick_add = hotbar_numkey_quick_add;
				if ( item && itemCategory(item) == SPELL_CAT && item->appearance >= 1000 &&
					players[player] && players[player]->entity && players[player]->entity->effectShapeshift )
				{
					if ( canUseShapeshiftSpellInCurrentForm(player, *item) != 1 )
					{
						numkey_quick_add = false;
					}
				}

				if ( numkey_quick_add && !command && item )
				{
					int slotNum = -1;
					if ( Input::inputs[player].binaryToggle("HotbarSlot1") )
					{
						Input::inputs[player].consumeBinaryToggle("HotbarSlot1");
						hotbar[0].item = item->uid;
						slotNum = 0;
					}
					if ( Input::inputs[player].binaryToggle("HotbarSlot2") )
					{
						Input::inputs[player].consumeBinaryToggle("HotbarSlot2");
						hotbar[1].item = item->uid;
						slotNum = 1;
					}
					if ( Input::inputs[player].binaryToggle("HotbarSlot3") )
					{
						Input::inputs[player].consumeBinaryToggle("HotbarSlot3");
						hotbar[2].item = item->uid;
						slotNum = 2;
					}
					if ( Input::inputs[player].binaryToggle("HotbarSlot4") )
					{
						Input::inputs[player].consumeBinaryToggle("HotbarSlot4");
						hotbar[3].item = item->uid;
						slotNum = 3;
					}
					if ( Input::inputs[player].binaryToggle("HotbarSlot5") )
					{
						Input::inputs[player].consumeBinaryToggle("HotbarSlot5");
						hotbar[4].item = item->uid;
						slotNum = 4;
					}
					if ( Input::inputs[player].binaryToggle("HotbarSlot6") )
					{
						Input::inputs[player].consumeBinaryToggle("HotbarSlot6");
						hotbar[5].item = item->uid;
						slotNum = 5;
					}
					if ( Input::inputs[player].binaryToggle("HotbarSlot7") )
					{
						Input::inputs[player].consumeBinaryToggle("HotbarSlot7");
						hotbar[6].item = item->uid;
						slotNum = 6;
					}
					if ( Input::inputs[player].binaryToggle("HotbarSlot8") )
					{
						Input::inputs[player].consumeBinaryToggle("HotbarSlot8");
						hotbar[7].item = item->uid;
						slotNum = 7;
					}
					if ( Input::inputs[player].binaryToggle("HotbarSlot9") )
					{
						Input::inputs[player].consumeBinaryToggle("HotbarSlot9");
						hotbar[8].item = item->uid;
						slotNum = 8;
					}
					if ( Input::inputs[player].binaryToggle("HotbarSlot10") 
						&& this->player.hotbar.getHotbarSlotFrame(9)
						&& !this->player.hotbar.getHotbarSlotFrame(9)->isDisabled() )
					{
						Input::inputs[player].consumeBinaryToggle("HotbarSlot10");
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
							}
							++i;
						}
					}
				}
				break;
			}
		}
	}
	else if ( stats[player]->HP > 0 )
	{
		// releasing items
		bool oldSelectedItem = selectedItem != nullptr;
		Uint32 oldUid = selectedItem ? selectedItem->uid : 0;

		releaseItem(player);

		// re-highlight the selected slots/remove old selected slots
		// otherwise we'll have a frame of drawing nothing
		if ( oldSelectedItem && !selectedItem && frame )
		{
			auto selectedSlotFrame = frame->findFrame("inventory selected item");
			int selectedSlotFrameX = 0;
			int selectedSlotFrameY = 0;
			if ( getSlotFrameXYFromMousePos(player, selectedSlotFrameX, selectedSlotFrameY) )
			{
				if ( auto slotFrame = getInventorySlotFrame(selectedSlotFrameX, selectedSlotFrameY) )
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
							hotbar_t.getSlotSize(), hotbar_t.getSlotSize(), inputs.getVirtualMouse(player)->draw_cursor);
						//messagePlayer(player, "7: hotbar: %d", c);
						break;
					}
				}
			}

			auto oldSelectedSlotFrame = frame->findFrame("inventory old selected item");
			oldSelectedSlotFrame->setDisabled(true);

			// update the displayed item frames - to catch any changes
			for ( node = stats[player]->inventory.first; node != NULL; node = nextnode )
			{
				nextnode = node->next;
				Item* item = (Item*)node->element;
				if ( !item ) { continue; }

				if ( (players[player]->inventory_mode == INVENTORY_MODE_ITEM && itemCategory(item) == SPELL_CAT)
					|| (players[player]->inventory_mode == INVENTORY_MODE_SPELL && itemCategory(item) != SPELL_CAT) )
				{
					// don't update hidden items
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

				if ( itemx >= 0 && itemx < getSizeX()
					&& itemy >= Player::Inventory_t::PaperDollRows::DOLL_ROW_1 && itemy < getSizeY() )
				{
					if ( auto slotFrame = getInventorySlotFrame(itemx, itemy) )
					{
						updateSlotFrameFromItem(slotFrame, item);
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
	itemContextMenu(player);
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
			warpMouseToSelectedInventorySlot(player.playernum);
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

std::string getContextMenuOptionBindingName(const ItemContextMenuPrompts prompt)
{
	switch ( prompt )
	{
		case PROMPT_EQUIP:
		case PROMPT_UNEQUIP:
		case PROMPT_SPELL_EQUIP:
			return "MenuAlt2";
		case PROMPT_GRAB:
			return "MenuAlt1";
		case PROMPT_TINKER:
		case PROMPT_INTERACT:
		case PROMPT_EAT:
		case PROMPT_SPELL_QUICKCAST:
		case PROMPT_CONSUME:
		case PROMPT_SELL:
		case PROMPT_BUY:
		case PROMPT_STORE_CHEST:
		case PROMPT_RETRIEVE_CHEST:
		case PROMPT_INSPECT:
			return "MenuConfirm";
		case PROMPT_DROP:
			return "MenuCancel";
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
		case PROMPT_EAT:
			return itemUseString(player, item);
		case PROMPT_SPELL_QUICKCAST:
			return language[4049];
		case PROMPT_TINKER:
			return language[3670];
			break;
		case PROMPT_APPRAISE:
			return language[1161];
		case PROMPT_CONSUME:
			return language[3487];
		case PROMPT_INSPECT:
			return language[1881];
		case PROMPT_SELL:
			return language[345];
			break;
		case PROMPT_BUY:
			break;
		case PROMPT_STORE_CHEST:
			return language[344];
		case PROMPT_RETRIEVE_CHEST:
			break;
		case PROMPT_DROP:
			return language[1162];
		case PROMPT_GRAB:
			return language[4050];
		default:
			return "Invalid";
	}
	return "Invalid";
}

std::vector<ItemContextMenuPrompts> getContextTooltipOptionsForItem(const int player, Item* item)
{
	auto options = getContextMenuOptionsForItem(player, item);
	options.push_back(ItemContextMenuPrompts::PROMPT_GRAB); // additional prompt here for non-right click menu
	auto findAppraise = std::find(options.begin(), options.end(), ItemContextMenuPrompts::PROMPT_APPRAISE);
	if ( findAppraise != options.end() )
	{
		options.erase(findAppraise);
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

	if ( openedChest[player] )
	{
		if ( playerOwnedItem )
		{
			options.push_back(PROMPT_STORE_CHEST);
		}
		else
		{
			options.push_back(PROMPT_RETRIEVE_CHEST);
		}
		return options;
	}
	if ( players[player]->gui_mode == GUI_MODE_SHOP )
	{
		if ( playerOwnedItem )
		{
			options.push_back(PROMPT_SELL);
		}
		else
		{
			options.push_back(PROMPT_BUY);
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
			options.push_back(PROMPT_EAT);
			options.push_back(PROMPT_APPRAISE);
			options.push_back(PROMPT_DROP);
		}
		else
		{
			options.push_back(PROMPT_EAT);
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
	else if ( item->type == TOOL_TINKERING_KIT )
	{
		options.push_back(PROMPT_EQUIP);
		options.push_back(PROMPT_TINKER);
		options.push_back(PROMPT_APPRAISE);
		options.push_back(PROMPT_DROP);
	}
	else if ( stats[player] && stats[player]->type == AUTOMATON && itemIsConsumableByAutomaton(*item) )
	{
		if ( item->type == TOOL_METAL_SCRAP || item->type == TOOL_MAGIC_SCRAP )
		{
			options.push_back(PROMPT_CONSUME);
			options.push_back(PROMPT_INSPECT);
			options.push_back(PROMPT_APPRAISE);
			options.push_back(PROMPT_DROP);
		}
		else if ( itemCategory(item) == FOOD )
		{
			options.push_back(PROMPT_CONSUME);
			options.push_back(PROMPT_APPRAISE);
			options.push_back(PROMPT_DROP);
		}
		else if ( itemCategory(item) == GEM )
		{
			options.push_back(PROMPT_EQUIP);
			options.push_back(PROMPT_CONSUME);
			options.push_back(PROMPT_APPRAISE);
			options.push_back(PROMPT_DROP);
		}
		else
		{
			options.push_back(PROMPT_INTERACT);
			options.push_back(PROMPT_CONSUME);
			options.push_back(PROMPT_APPRAISE);
			options.push_back(PROMPT_DROP);
		}
	}
	else if ( item->type == FOOD_CREAMPIE )
	{
		options.push_back(PROMPT_EAT);
		options.push_back(PROMPT_EQUIP);
		options.push_back(PROMPT_APPRAISE);
		options.push_back(PROMPT_DROP);
	}
	else if ( itemCategory(item) == SPELLBOOK )
	{
		bool learnedSpell = playerLearnedSpellbook(player, item);
		if ( itemIsEquipped(item, player) )
		{
			learnedSpell = true; // equipped spellbook will unequip on use.
		}
		else if ( stats[player] && stats[player]->type == GOBLIN )
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

	for ( auto it = options.begin(); it != options.end(); )
	{
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

/*
 * Helper function to drawItemMenuSlots. Draws the empty window for an individual item context menu slot.
 */
inline void drawItemMenuSlot(int x, int y, int width, int height, bool selected = false)
{
	if (selected)
	{
		drawDepressed(x, y, x + width, y + height);
	}
	else
	{
		drawWindow(x, y, x + width, y + height);
	}
}

/*
 * Helper function to itemContextMenu(). Draws the context menu slots.
 */
inline void drawItemMenuSlots(const int player, const Item& item, int slot_width, int slot_height)
{
	int& itemMenuX = inputs.getUIInteraction(player)->itemMenuX;
	int& itemMenuY = inputs.getUIInteraction(player)->itemMenuY;
	int& itemMenuSelected = inputs.getUIInteraction(player)->itemMenuSelected;

	//Draw the action select boxes. "Appraise", "Use, "Equip", etc.
	int current_x = itemMenuX + 300;
	int current_y = itemMenuY + 300;
	drawItemMenuSlot(current_x, current_y, slot_width, slot_height, itemMenuSelected == 0); //Option 0 => Store in chest, sell, use.
	if (itemCategory(&item) != SPELL_CAT)
	{
		if ( itemMenuSkipRow1ForShopsAndChests(player, item) )
		{
			current_y += slot_height;
			drawItemMenuSlot(current_x, current_y, slot_width, slot_height, itemMenuSelected == 2); //Option 1 => appraise

			current_y += slot_height;
			drawItemMenuSlot(current_x, current_y, slot_width, slot_height, itemMenuSelected == 3); //Option 2 => drop
			return; // only draw 3 lines.
		}

		current_y += slot_height;
		drawItemMenuSlot(current_x, current_y, slot_width, slot_height, itemMenuSelected == 1); //Option 1 => wield, unwield, use, appraise

		current_y += slot_height;
		drawItemMenuSlot(current_x, current_y, slot_width, slot_height, itemMenuSelected == 2); //Option 2 => appraise, drop

		if ( stats[player] && stats[player]->type == AUTOMATON && itemIsConsumableByAutomaton(item)
			&& itemCategory(&item) != FOOD )
		{
			current_y += slot_height;
			drawItemMenuSlot(current_x, current_y, slot_width, slot_height, itemMenuSelected == 3); //Option 3 => drop
		}
		if (itemCategory(&item) == POTION || item.type == TOOL_ALEMBIC || item.type == TOOL_TINKERING_KIT )
		{
			current_y += slot_height;
			drawItemMenuSlot(current_x, current_y, slot_width, slot_height, itemMenuSelected == 3); //Option 3 => drop
		}
		else if ( itemCategory(&item) == SPELLBOOK )
		{
			current_y += slot_height;
			drawItemMenuSlot(current_x, current_y, slot_width, slot_height, itemMenuSelected == 3); //Option 3 => drop
		}
		else if ( item.type == FOOD_CREAMPIE )
		{
			current_y += slot_height;
			drawItemMenuSlot(current_x, current_y, slot_width, slot_height, itemMenuSelected == 3); //Option 3 => drop
		}
	}
}

/*
 * drawOptionX() - renders the specified option in the given item option menu slot.
 */
inline void drawOptionStoreInChest(int x, int y)
{
	int width = 0;
	getSizeOfText(ttf12, language[344], &width, nullptr);
	ttfPrintText(ttf12, x + 50 - width / 2, y + 4, language[344]);
}

inline void drawOptionSell(int x, int y)
{
	int width = 0;
	getSizeOfText(ttf12, language[345], &width, nullptr);
	ttfPrintText(ttf12, x + 50 - width / 2, y + 4, language[345]);
}

inline void drawOptionUse(const int player, const Item& item, int x, int y)
{
	int width = 0;
	ttfPrintTextFormatted(ttf12, x + 50 - strlen(itemUseString(player, item)) * TTF12_WIDTH / 2, y + 4, "%s", itemUseString(player, item));
}

inline void drawOptionUnwield(int x, int y)
{
	int width = 0;
	getSizeOfText(ttf12, language[323], &width, nullptr);
	ttfPrintText(ttf12, x + 50 - width / 2, y + 4, language[323]);
}

inline void drawOptionWield(int x, int y)
{
	int width = 0;
	getSizeOfText(ttf12, language[324], &width, nullptr);
	ttfPrintText(ttf12, x + 50 - width / 2, y + 4, language[324]);
}

inline void drawOptionAppraise(int x, int y)
{
	int width = 0;
	getSizeOfText(ttf12, language[1161], &width, nullptr);
	ttfPrintText(ttf12, x + 50 - width / 2, y + 4, language[1161]);
}

inline void drawOptionDrop(int x, int y)
{
	int width = 0;
	getSizeOfText(ttf12, language[1162], &width, nullptr);
	ttfPrintText(ttf12, x + 50 - width / 2, y + 4, language[1162]);
}

/*
 * Helper function to itemContextMenu(). Draws a spell's options.
 */
inline void drawItemMenuOptionSpell(const int player, const Item& item, int x, int y)
{
	if (itemCategory(&item) != SPELL_CAT)
	{
		return;
	}

	int width = 0;

	//Option 0.
	drawOptionUse(player, item, x, y);
}

/*
 * Helper function to itemContextMenu(). Draws a potion's options.
 */
inline void drawItemMenuOptionPotion(const int player, const Item& item, int x, int y, int height, bool is_potion_bad = false)
{
	if (itemCategory(&item) != POTION && item.type != TOOL_ALEMBIC && item.type != TOOL_TINKERING_KIT )
	{
		return;
	}

	int width = 0;

	//Option 0.
	if (openedChest[player])
	{
		drawOptionStoreInChest(x, y);
	}
	else if ( players[player]->gui_mode == GUI_MODE_SHOP)
	{
		drawOptionSell(x, y);
	}
	else
	{
		if ( item.type == TOOL_TINKERING_KIT )
		{
			if ( itemIsEquipped(&item, player) )
			{
				drawOptionUnwield(x, y);
			}
			else
			{
				drawOptionWield(x, y);
			}
		}
		else if (!is_potion_bad)
		{
			drawOptionUse(player, item, x, y);
		}
		else
		{
			if (itemIsEquipped(&item, player))
			{
				drawOptionUnwield(x, y);
			}
			else
			{
				drawOptionWield(x, y);
			}
		}
	}
	y += height;

	//Option 1.
	if ( itemMenuSkipRow1ForShopsAndChests(player, item) )
	{
		// skip this row.
	}
	else
	{
		if (!is_potion_bad)
		{
			if ( item.type == TOOL_ALEMBIC )
			{
				getSizeOfText(ttf12, language[3341], &width, nullptr);
				ttfPrintText(ttf12, x + 50 - width / 2, y + 4, language[3341]);
			}
			else if ( item.type == TOOL_TINKERING_KIT )
			{
				getSizeOfText(ttf12, language[3670], &width, nullptr);
				ttfPrintText(ttf12, x + 50 - width / 2, y + 4, language[3670]);
			}
			else if (itemIsEquipped(&item, player))
			{
				drawOptionUnwield(x, y);
			}
			else
			{
				drawOptionWield(x, y);
			}
		}
		else
		{
			drawOptionUse(player, item, x, y);
		}
		y += height;
	}

	//Option 1.
	drawOptionAppraise(x, y);
	y += height;

	//Option 2.
	drawOptionDrop(x, y);
}

inline void drawItemMenuOptionAutomaton(const int player, const Item& item, int x, int y, int height, bool is_potion_bad)
{
	int width = 0;

	//Option 0.
	if ( openedChest[player] )
	{
		drawOptionStoreInChest(x, y);
	}
	else if ( players[player]->gui_mode == GUI_MODE_SHOP )
	{
		drawOptionSell(x, y);
	}
	else
	{
		if ( !is_potion_bad )
		{
			if ( !itemIsConsumableByAutomaton(item) || (itemCategory(&item) != FOOD && item.type != TOOL_METAL_SCRAP && item.type != TOOL_MAGIC_SCRAP) )
			{
				drawOptionUse(player, item, x, y);
			}
			else
			{
				getSizeOfText(ttf12, language[3487], &width, nullptr);
				ttfPrintText(ttf12, x + 50 - width / 2, y + 4, language[3487]);
			}
		}
		else
		{
			if ( itemIsEquipped(&item, player) )
			{
				drawOptionUnwield(x, y);
			}
			else
			{
				drawOptionWield(x, y);
			}
		}
	}
	y += height;

	//Option 1.
	if ( item.type == TOOL_METAL_SCRAP || item.type == TOOL_MAGIC_SCRAP )
	{
		getSizeOfText(ttf12, language[1881], &width, nullptr);
		ttfPrintText(ttf12, x + 50 - width / 2, y + 4, language[1881]);
		y += height;
	}
	else if ( itemCategory(&item) != FOOD )
	{
		getSizeOfText(ttf12, language[3487], &width, nullptr);
		ttfPrintText(ttf12, x + 50 - width / 2, y + 4, language[3487]);
		y += height;
	}

	//Option 1.
	drawOptionAppraise(x, y);
	y += height;

	//Option 2.
	drawOptionDrop(x, y);
}

/*
 * Helper function to itemContextMenu(). Draws all other items's options.
 */
inline void drawItemMenuOptionGeneric(const int player, const Item& item, int x, int y, int height)
{
	if (itemCategory(&item) == SPELL_CAT || itemCategory(&item) == POTION)
	{
		return;
	}

	int width = 0;

	//Option 0.
	if (openedChest[player])
	{
		drawOptionStoreInChest(x, y);
	}
	else if ( players[player]->gui_mode == GUI_MODE_SHOP)
	{
		drawOptionSell(x, y);
	}
	else
	{
		drawOptionUse(player, item, x, y);
	}
	y += height;

	//Option 1
	drawOptionAppraise(x, y);
	y += height;

	//Option 2
	drawOptionDrop(x, y);
}

inline void drawItemMenuOptionSpellbook(const int player, const Item& item, int x, int y, int height, bool learnedSpell)
{
	int width = 0;

	//Option 0.
	if ( openedChest[player] )
	{
		drawOptionStoreInChest(x, y);
	}
	else if ( players[player]->gui_mode == GUI_MODE_SHOP )
	{
		drawOptionSell(x, y);
	}
	else
	{
		if ( learnedSpell )
		{
			if ( itemIsEquipped(&item, player) )
			{
				drawOptionUnwield(x, y);
			}
			else
			{
				drawOptionWield(x, y);
			}
		}
		else
		{
			drawOptionUse(player, item, x, y);
		}
	}
	y += height;

	//Option 1
	if ( itemMenuSkipRow1ForShopsAndChests(player, item) )
	{
		// skip this row.
	}
	else
	{
		if ( learnedSpell )
		{
			drawOptionUse(player, item, x, y);
		}
		else
		{
			if ( itemIsEquipped(&item, player) )
			{
				drawOptionUnwield(x, y);
			}
			else
			{
				drawOptionWield(x, y);
			}
		}
		y += height;
	}

	//Option 2
	drawOptionAppraise(x, y);
	y += height;

	//Option 3
	drawOptionDrop(x, y);
}

inline void drawItemMenuOptionUsableAndWieldable(const int player, const Item& item, int x, int y, int height)
{
	int width = 0;

	//Option 0.
	if ( openedChest[player] )
	{
		drawOptionStoreInChest(x, y);
	}
	else if ( players[player]->gui_mode == GUI_MODE_SHOP )
	{
		drawOptionSell(x, y);
	}
	else
	{
		drawOptionUse(player, item, x, y);
	}
	y += height;

	//Option 1
	if ( itemIsEquipped(&item, player) )
	{
		drawOptionUnwield(x, y);
	}
	else
	{
		drawOptionWield(x, y);
	}
	y += height;

	//Option 2
	drawOptionAppraise(x, y);
	y += height;

	//Option 3
	drawOptionDrop(x, y);
}

/*
 * Helper function to itemContextMenu(). Changes the currently selected slot based on the mouse cursor's position.
 */
inline void selectItemMenuSlot(const int player, const Item& item, int x, int y, int slot_width, int slot_height)
{
	return;
	int& itemMenuX = inputs.getUIInteraction(player)->itemMenuX;
	int& itemMenuY = inputs.getUIInteraction(player)->itemMenuY;
	int& itemMenuSelected = inputs.getUIInteraction(player)->itemMenuSelected;

	int current_x = itemMenuX;
	int current_y = itemMenuY;

	const Sint32 mousex = inputs.getMouse(player, Inputs::X);
	const Sint32 mousey = inputs.getMouse(player, Inputs::Y);

	if (mousey < current_y - slot_height)   //Check if out of bounds above.
	{
		itemMenuSelected = -1; //For canceling out.
	}
	if (mousey >= current_y - 2 && mousey < current_y + slot_height)
	{
		itemMenuSelected = 0;
	}
	if (itemCategory(&item) != SPELL_CAT)
	{
		if ( itemMenuSkipRow1ForShopsAndChests(player, item) )
		{
			current_y += slot_height;
			if ( mousey >= current_y && mousey < current_y + slot_height )
			{
				itemMenuSelected = 2;
			}
			current_y += slot_height;
			if ( mousey >= current_y && mousey < current_y + slot_height )
			{
				itemMenuSelected = 3;
			}
		}
		else
		{
			current_y += slot_height;
			if (mousey >= current_y && mousey < current_y + slot_height)
			{
				itemMenuSelected = 1;
			}
			current_y += slot_height;
			if (mousey >= current_y && mousey < current_y + slot_height)
			{
				itemMenuSelected = 2;
			}
			current_y += slot_height;
			if ( itemCategory(&item) == POTION || itemCategory(&item) == SPELLBOOK || item.type == FOOD_CREAMPIE
				|| (stats[player] && stats[player]->type == AUTOMATON && itemIsConsumableByAutomaton(item)
					&& itemCategory(&item) != FOOD) )
			{
				if (mousey >= current_y && mousey < current_y + slot_height)
				{
					itemMenuSelected = 3;
				}
				current_y += slot_height;
			}
		}
	}

	if (mousey >= current_y + slot_height)   //Check if out of bounds below.
	{
		itemMenuSelected = -1; //For canceling out.
	}

	if (mousex >= current_x + slot_width)   //Check if out of bounds to the right.
	{
		itemMenuSelected = -1; //For canceling out.
	}
	if ( mousex < itemMenuX - 10 || (mousex < itemMenuX && right_click_protect) )   //Check if out of bounds to the left.
	{
		itemMenuSelected = -1; //For canceling out.
	}
}

/*
 * execteItemMenuOptionX() -  Helper function to itemContextMenu(). Executes the specified menu option for the item.
 */
void executeItemMenuOption0(const int player, Item* item, bool is_potion_bad, bool learnedSpell)
{
	if (!item)
	{
		return;
	}

	bool disableItemUsage = false;
	if ( players[player] && players[player]->entity )
	{
		if ( players[player]->entity->effectShapeshift != NOTHING )
		{
			// shape shifted, disable some items
			if ( !item->usableWhileShapeshifted(stats[player]) )
			{
				disableItemUsage = true;
			}
		}
		else
		{
			if ( itemCategory(item) == SPELL_CAT && item->appearance >= 1000 )
			{
				if ( canUseShapeshiftSpellInCurrentForm(player, *item) != 1 )
				{
					disableItemUsage = true;
				}
			}
		}
	}

	if ( client_classes[player] == CLASS_SHAMAN )
	{
		if ( item->type == SPELL_ITEM && !(playerUnlockedShamanSpell(player, item)) )
		{
			disableItemUsage = true;
		}
	}

	if (openedChest[player] && itemCategory(item) != SPELL_CAT)
	{
		//Option 0 = store in chest.
		openedChest[player]->addItemToChestFromInventory(player, item, false);
	}
	else if ( players[player]->gui_mode == GUI_MODE_SHOP && itemCategory(item) != SPELL_CAT)
	{
		//Option 0 = sell.
		sellItemToShop(player, item);
	}
	else
	{
		if (!is_potion_bad && !learnedSpell)
		{
			//Option 0 = use.
			if ( !(isItemEquippableInShieldSlot(item) && cast_animation[player].active_spellbook) )
			{
				if ( !disableItemUsage )
				{
					if ( stats[player] && stats[player]->type == AUTOMATON
						&& (item->type == TOOL_METAL_SCRAP || item->type == TOOL_MAGIC_SCRAP) )
					{
						// consume item
						if ( multiplayer == CLIENT )
						{
							strcpy((char*)net_packet->data, "FODA");
							SDLNet_Write32((Uint32)item->type, &net_packet->data[4]);
							SDLNet_Write32((Uint32)item->status, &net_packet->data[8]);
							SDLNet_Write32((Uint32)item->beatitude, &net_packet->data[12]);
							SDLNet_Write32((Uint32)item->count, &net_packet->data[16]);
							SDLNet_Write32((Uint32)item->appearance, &net_packet->data[20]);
							net_packet->data[24] = item->identified;
							net_packet->data[25] = player;
							net_packet->address.host = net_server.host;
							net_packet->address.port = net_server.port;
							net_packet->len = 26;
							sendPacketSafe(net_sock, -1, net_packet, 0);
						}
						item_FoodAutomaton(item, player);
					}
					else
					{
						useItem(item, player);
					}
				}
				else
				{
					if ( client_classes[player] == CLASS_SHAMAN && item->type == SPELL_ITEM )
					{
						messagePlayer(player, language[3488]); // unable to use with current level.
					}
					else
					{
						messagePlayer(player, language[3432]); // unable to use in current form message.
					}
				}
			}
		}
		else
		{
			if ( !disableItemUsage )
			{
				//Option 0 = equip.
				playerTryEquipItemAndUpdateServer(player, item, true);
			}
			else
			{
				if ( client_classes[player] == CLASS_SHAMAN && item->type == SPELL_ITEM )
				{
					messagePlayer(player, language[3488]); // unable to use with current level.
				}
				else
				{
					messagePlayer(player, language[3432]); // unable to use in current form message.
				}
			}
		}
	}
}

inline void executeItemMenuOption1(const int player, Item* item, bool is_potion_bad, bool learnedSpell)
{
	if (!item || itemCategory(item) == SPELL_CAT)
	{
		return;
	}

	bool disableItemUsage = false;
	if ( players[player] && players[player]->entity && players[player]->entity->effectShapeshift != NOTHING )
	{
		// shape shifted, disable some items
		if ( !item->usableWhileShapeshifted(stats[player]) )
		{
			disableItemUsage = true;
		}
		if ( item->type == FOOD_CREAMPIE )
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

	if ( stats[player] && stats[player]->type == AUTOMATON && itemIsConsumableByAutomaton(*item)
		&& itemCategory(item) != FOOD )
	{
		if ( item->type == TOOL_METAL_SCRAP || item->type == TOOL_MAGIC_SCRAP )
		{
			useItem(item, player);
		}
		else
		{
			// consume item
			if ( multiplayer == CLIENT )
			{
				strcpy((char*)net_packet->data, "FODA");
				SDLNet_Write32((Uint32)item->type, &net_packet->data[4]);
				SDLNet_Write32((Uint32)item->status, &net_packet->data[8]);
				SDLNet_Write32((Uint32)item->beatitude, &net_packet->data[12]);
				SDLNet_Write32((Uint32)item->count, &net_packet->data[16]);
				SDLNet_Write32((Uint32)item->appearance, &net_packet->data[20]);
				net_packet->data[24] = item->identified;
				net_packet->data[25] = player;
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 26;
				sendPacketSafe(net_sock, -1, net_packet, 0);
			}
			item_FoodAutomaton(item, player);
		}
	}
	else if ( item->type == TOOL_ALEMBIC )
	{
		// experimenting!
		if ( !disableItemUsage )
		{
			GenericGUI[player].openGUI(GUI_TYPE_ALCHEMY, true, item);
		}
		else
		{
			messagePlayer(player, language[3432]); // unable to use in current form message.
		}
	}
	else if ( item->type == TOOL_TINKERING_KIT )
	{
		if ( !disableItemUsage )
		{
			GenericGUI[player].openGUI(GUI_TYPE_TINKERING, item);
		}
		else
		{
			messagePlayer(player, language[3432]); // unable to use in current form message.
		}
	}
	else if (itemCategory(item) != POTION && itemCategory(item) != SPELLBOOK && item->type != FOOD_CREAMPIE)
	{
		//Option 1 = appraise.
		players[player]->inventoryUI.appraisal.appraiseItem(item);
	}
	else
	{
		if ( !disableItemUsage )
		{
			if (!is_potion_bad && !learnedSpell)
			{
				//Option 1 = equip.
				playerTryEquipItemAndUpdateServer(player, item, true);
			}
			else
			{
				//Option 1 = drink/use/whatever.
				if ( !(isItemEquippableInShieldSlot(item) && cast_animation[player].active_spellbook) )
				{
					useItem(item, player);
				}
			}
		}
		else
		{
			if ( client_classes[player] == CLASS_SHAMAN && item->type == SPELL_ITEM )
			{
				messagePlayer(player, language[3488]); // unable to use with current level.
			}
			else
			{
				messagePlayer(player, language[3432]); // unable to use in current form message.
			}
		}
	}
}

inline void executeItemMenuOption2(const int player, Item* item)
{
	if (!item || itemCategory(item) == SPELL_CAT)
	{
		return;
	}

	if ( stats[player] && stats[player]->type == AUTOMATON && itemIsConsumableByAutomaton(*item) && itemCategory(item) != FOOD )
	{
		//Option 2 = appraise.
		players[player]->inventoryUI.appraisal.appraiseItem(item);
	}
	else if ( itemCategory(item) != POTION && item->type != TOOL_ALEMBIC 
		&& itemCategory(item) != SPELLBOOK && item->type != TOOL_TINKERING_KIT
		&& item->type != FOOD_CREAMPIE )
	{
		//Option 2 = drop.
		dropItem(item, player);
	}
	else
	{
		//Option 2 = appraise.
		players[player]->inventoryUI.appraisal.appraiseItem(item);
	}
}

inline void executeItemMenuOption3(const int player, Item* item)
{
	if ( !item )
	{
		return;
	}
	if ( stats[player] && stats[player]->type == AUTOMATON && itemIsConsumableByAutomaton(*item) && itemCategory(item) != FOOD )
	{
		//Option 3 = drop (automaton has 4 options on consumable items).
		dropItem(item, player);
		return;
	}
	if ( itemCategory(item) != POTION && item->type != TOOL_ALEMBIC 
		&& itemCategory(item) != SPELLBOOK && item->type != TOOL_TINKERING_KIT
		&& item->type != FOOD_CREAMPIE )
	{
		return;
	}

	//Option 3 = drop (only spellbooks/potions have option 3).
	dropItem(item, player);
}

void itemContextMenu(const int player)
{
	return;
	bool& toggleclick = inputs.getUIInteraction(player)->toggleclick;
	bool& itemMenuOpen = inputs.getUIInteraction(player)->itemMenuOpen;
	Uint32& itemMenuItem = inputs.getUIInteraction(player)->itemMenuItem;
	int itemMenuX = inputs.getUIInteraction(player)->itemMenuX + 300;
	int itemMenuY = inputs.getUIInteraction(player)->itemMenuY + 300;
	int& itemMenuSelected = inputs.getUIInteraction(player)->itemMenuSelected;
	const int inventorySlotSize = players[player]->inventoryUI.getSlotSize();

	if (!itemMenuOpen)
	{
		return;
	}

	if ( inputs.bControllerInputPressed(player, INJOY_MENU_CANCEL) )
	{
		inputs.controllerClearInput(player, INJOY_MENU_CANCEL);
		itemMenuOpen = false;
		//Warp cursor back into inventory, for gamepad convenience.

		if ( players[player]->inventoryUI.warpMouseToSelectedItem(nullptr, (Inputs::SET_CONTROLLER)) )
		{
			return;
		}

		messagePlayer(0, "[Debug]: warpMouseToSelectedInventorySlot failed");
		// TODO UI: REMOVE DEBUG AND CLEAN UP
		//int newx = players[player]->inventoryUI.getSelectedSlotPositionX(uidToItem(itemMenuItem));
		//int newy = players[player]->inventoryUI.getSelectedSlotPositionY(uidToItem(itemMenuItem));
		////SDL_WarpMouseInWindow(screen, newx, newy);
		//Uint32 flags = (Inputs::SET_MOUSE | Inputs::SET_CONTROLLER);
		//inputs.warpMouse(player, newx, newy, flags);
		return;
	}

	//Item *item = uidToItem(itemMenuItem);

	Item* current_item = uidToItem(itemMenuItem);
	if (!current_item)
	{
		itemMenuOpen = false;
		return;
	}

	bool is_potion_bad = isPotionBad(*current_item);

	const int slot_width = 100;
	const int slot_height = 20;

	if ( inputs.hasController(player) && inputs.getController(player)->handleItemContextMenu(player, *current_item) )
	{
		//SDL_WarpMouseInWindow(screen, itemMenuX + (slot_width / 2), itemMenuY + (itemMenuSelected * slot_height) + (slot_height / 2));
		Uint32 flags = (Inputs::SET_MOUSE | Inputs::SET_CONTROLLER);
		inputs.warpMouse(player, itemMenuX + (slot_width / 2), itemMenuY + (itemMenuSelected * slot_height) + (slot_height / 2), flags);
	}

	drawItemMenuSlots(player, *current_item, slot_width, slot_height);
	bool learnedSpell = false;

	if (itemCategory(current_item) == SPELL_CAT)
	{
		drawItemMenuOptionSpell(player, *current_item, itemMenuX, itemMenuY);
	}
	else
	{
		if ( stats[player] && stats[player]->type == AUTOMATON && itemIsConsumableByAutomaton(*current_item)
			&& current_item->type != FOOD_CREAMPIE )
		{
			drawItemMenuOptionAutomaton(player, *current_item, itemMenuX, itemMenuY, slot_height, is_potion_bad);
		}
		else if ( current_item->type == TOOL_ALEMBIC || current_item->type == TOOL_TINKERING_KIT )
		{
			drawItemMenuOptionPotion(player, *current_item, itemMenuX, itemMenuY, slot_height, false);
		}
		else if (itemCategory(current_item) == POTION || current_item->type == TOOL_ALEMBIC)
		{
			drawItemMenuOptionPotion(player, *current_item, itemMenuX, itemMenuY, slot_height, is_potion_bad);
		}
		else if ( itemCategory(current_item) == SPELLBOOK )
		{
			learnedSpell = playerLearnedSpellbook(player, current_item);
			if ( itemIsEquipped(current_item, player) )
			{
				learnedSpell = true; // equipped spellbook will unequip on use.
			}
			else if ( players[player] && players[player]->entity )
			{
				if ( players[player]->entity->effectShapeshift == CREATURE_IMP )
				{
					learnedSpell = true; // imps can't learn spells but always equip books.
				}
				else if ( stats[player] && stats[player]->type == GOBLIN )
				{
					learnedSpell = true; // goblinos can't learn spells but always equip books.
				}
			}

			drawItemMenuOptionSpellbook(player, *current_item, itemMenuX, itemMenuY, slot_height, learnedSpell);
		}
		else if ( current_item->type == FOOD_CREAMPIE )
		{
			drawItemMenuOptionUsableAndWieldable(player, *current_item, itemMenuX, itemMenuY, slot_height);
		}
		else
		{
			drawItemMenuOptionGeneric(player, *current_item, itemMenuX, itemMenuY, slot_height); //Every other item besides potions and spells.
		}
	}

	selectItemMenuSlot(player, *current_item, itemMenuX, itemMenuY, slot_width, slot_height);
	return;
	bool activateSelection = false;
	if (!inputs.bMouseRight(player) && !toggleclick)
	{
		activateSelection = true;
	}
	else if ( inputs.bControllerInputPressed(player, INJOY_MENU_USE) )
	{
		inputs.controllerClearInput(player, INJOY_MENU_USE);
		activateSelection = true;

		if ( !players[player]->inventoryUI.warpMouseToSelectedItem(nullptr, (Inputs::SET_CONTROLLER)) )
		{
			// TODO UI: REMOVE DEBUG AND CLEAN UP
			messagePlayer(0, "[Debug]: warpMouseToSelectedInventorySlot failed");
			////Warp cursor back into inventory, for gamepad convenience.
			//int newx = players[player]->inventoryUI.getSelectedSlotPositionX(nullptr);
			//int newy = players[player]->inventoryUI.getSelectedSlotPositionY(nullptr);
			////SDL_WarpMouseInWindow(screen, newx, newy);
			//Uint32 flags = (Inputs::SET_MOUSE | Inputs::SET_CONTROLLER);
			//inputs.warpMouse(player, newx, newy, flags);
		}

	}

	bool itemWasOnPaperDoll = players[player]->paperDoll.isItemOnDoll(*current_item);

	if (activateSelection)
	{
		switch (itemMenuSelected)
		{
			case 0:
				executeItemMenuOption0(player, current_item, is_potion_bad, learnedSpell);
				break;
			case 1:
				executeItemMenuOption1(player, current_item, is_potion_bad, learnedSpell);
				break;
			case 2:
				executeItemMenuOption2(player, current_item);
				break;
			case 3:
				executeItemMenuOption3(player, current_item);
				break;
			default:
				break;
		}

		if ( itemWasOnPaperDoll )
		{
			players[player]->paperDoll.warpMouseToMostRecentReturnedInventoryItem();
		}

		//Close the menu.
		itemMenuOpen = false;
		itemMenuItem = 0;
	}
}

int numItemMenuSlots(const Item& item)
{
	int numSlots = 1; //Option 0 => store in chest, sell, use.

	if (itemCategory(&item) != SPELL_CAT)
	{
		numSlots += 2; //Option 1 => wield, unwield, use, appraise. & Option 2 => appraise, drop

		if (itemCategory(&item) == POTION)
		{
			numSlots += 1; //Option 3 => drop.
		}
		else if ( itemCategory(&item) == SPELLBOOK )
		{
			numSlots += 1; //Can read/equip spellbooks
		}
		else if ( item.type == FOOD_CREAMPIE )
		{
			numSlots += 1; //Can equip
		}
	}

	return numSlots;
}

//Used by the gamepad, primarily. Dpad buttons changes selection.
void selectItemMenuSlot(const int player, const Item& item, int entry)
{
	int& itemMenuSelected = inputs.getUIInteraction(player)->itemMenuSelected;
	if (entry >= numItemMenuSlots(item))
	{
		entry = 0;
	}
	if (entry < 0)
	{
		entry = numItemMenuSlots(item) - 1;
	}

	itemMenuSelected = entry;
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
					if ( cat == BOOK || cat == SPELLBOOK )
					{
						return true;
					}
					break;
				case 4: // tools
					if ( cat == TOOL )
					{
						return true;
					}
					break;
				case 5: // thrown
					if ( cat == THROWN || item.type == GEM_ROCK )
					{
						return true;
					}
					break;
				case 6: // gems
					if ( cat == GEM )
					{
						return true;
					}
					break;
				case 7: // potions
					if ( cat == POTION )
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
					sortInventoryItemsOfType(player, -2, invertSortDirection);
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
					sortInventoryItemsOfType(player, -2, invertSortDirection);
					break;
				default:
					break;
			}
		}
	}


	sortInventoryItemsOfType(player, -1, true); // clean up the rest of the items.
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
				continue;
			}

			// find a place...
			int x, y;
			bool notfree = false, foundaspot = false;

			bool is_spell = false;
			int inventory_y = std::min(std::max(players[player]->inventoryUI.getSizeY(), 2), players[player]->inventoryUI.DEFAULT_INVENTORY_SIZEY); // only sort y values of 2-3, if extra row don't auto sort into it.
			if ( itemCategory(itemBeingSorted) == SPELL_CAT )
			{
				is_spell = true;
				inventory_y = std::min(inventory_y, players[player]->inventoryUI.DEFAULT_INVENTORY_SIZEY);
			}

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
	}
	return false;
	// TODO UI: CLEAN UP / VERIFY
	//SDL_Rect pos;
	//pos.x = players[player]->inventoryUI.getStartX();
	//pos.y = players[player]->inventoryUI.getStartY();
	//pos.w = players[player]->inventoryUI.getSizeX() * players[player]->inventoryUI.getSlotSize();
	//pos.h = players[player]->inventoryUI.getSizeY() * players[player]->inventoryUI.getSlotSize();
	//return mouseInBounds(player, pos.x, pos.x + pos.w, pos.y, pos.y + pos.h);
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
	// TODO UI: CLEAN UP / VERIFY
	//SDL_Rect pos;
	//pos.x = players[player]->hotbar.getStartX();
	//pos.y = players[player]->statusBarUI.getStartY() - hotbar_img->h * uiscale_hotbar;
	//pos.w = NUM_HOTBAR_SLOTS * hotbar_img->w * uiscale_hotbar;
	//pos.h = hotbar_img->h * uiscale_hotbar;
	//return mouseInBounds(player, pos.x, pos.x + pos.w, pos.y, pos.y + pos.h);
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
		spell_t *spell = getSpellFromItem(player, item); //Do not free or delete this.
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