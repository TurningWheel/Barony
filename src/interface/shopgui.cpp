/*-------------------------------------------------------------------------------

	BARONY
	File: shopgui.cpp
	Desc: contains shop (GUI) related functions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../draw.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../items.hpp"
#include "../shops.hpp"
#include "../player.hpp"
#include "interface.hpp"
#include "../colors.hpp"
#include "../ui/Field.hpp"
#include "../mod_tools.hpp"
#include "../ui/GameUI.hpp"
#include "../ui/Image.hpp"
#include "../ui/Button.hpp"

#include <assert.h>

bool hideItemFromShopView(Item& item)
{
	if ( item.type == ARTIFACT_ORB_GREEN || item.type == ARTIFACT_ORB_RED || item.type == ARTIFACT_ORB_BLUE )
	{
		return true;
	}
	if ( item.itemHiddenFromShop )
	{
		return true;
	}
	return false;
}

void rebuildShopInventory(const int player)
{
	bool mysteriousShopkeeper = (shopkeepertype[player] == 10);
	bool mysteriousShopkeeperGreenOrb = false;
	bool mysteriousShopkeeperBlueOrb = false;
	bool mysteriousShopkeeperRedOrb = false;
	if ( mysteriousShopkeeper )
	{
		for ( node_t* node = shopInv[player]->first; node != NULL; node = node->next )
		{
			Item* item = (Item*)node->element;
			if ( item )
			{
				if ( item->type == ARTIFACT_ORB_BLUE )
				{
					mysteriousShopkeeperBlueOrb = true;
				}
				else if ( item->type == ARTIFACT_ORB_RED )
				{
					mysteriousShopkeeperRedOrb = true;
				}
				else if ( item->type == ARTIFACT_ORB_GREEN )
				{
					mysteriousShopkeeperGreenOrb = true;
				}
			}
		}
	}

	//Count number of items.
	int c = 0;
	node_t* node = nullptr;
	for ( node = shopInv[player]->first; node != NULL; node = node->next )
	{
		Item* item = (Item*) node->element;
		if ( item )
		{
			if ( mysteriousShopkeeper )
			{
				if ( !mysteriousShopkeeperBlueOrb
					&& shopkeeperMysteriousItems[ARTIFACT_ORB_BLUE].find(item->type) != shopkeeperMysteriousItems[ARTIFACT_ORB_BLUE].end() )
				{
					continue;
				}
				if ( !mysteriousShopkeeperGreenOrb
					&& shopkeeperMysteriousItems[ARTIFACT_ORB_GREEN].find(item->type) != shopkeeperMysteriousItems[ARTIFACT_ORB_GREEN].end() )
				{
					continue;
				}
				if ( !mysteriousShopkeeperRedOrb
					&& shopkeeperMysteriousItems[ARTIFACT_ORB_RED].find(item->type) != shopkeeperMysteriousItems[ARTIFACT_ORB_RED].end() )
				{
					continue;
				}
			}
			if ( hideItemFromShopView(*item) )
			{
				continue;
			}
			if ( shopinventorycategory[player] == 0 && itemCategory(item) != WEAPON && itemCategory(item) != THROWN )
			{
				continue;
			}
			else if ( shopinventorycategory[player] == 1 && itemCategory(item) != ARMOR )
			{
				continue;
			}
			else if ( shopinventorycategory[player] == 2 && itemCategory(item) != AMULET && itemCategory(item) != RING )
			{
				continue;
			}
			else if ( shopinventorycategory[player] == 3 && itemCategory(item) != SPELLBOOK && itemCategory(item) != MAGICSTAFF && itemCategory(item) != SCROLL )
			{
				continue;
			}
			else if ( shopinventorycategory[player] == 4 && itemCategory(item) != GEM )
			{
				continue;
			}
			else if ( shopinventorycategory[player] == 5 && itemCategory(item) != FOOD && itemCategory(item) != POTION )
			{
				continue;
			}
			else if ( shopinventorycategory[player] == 6 && itemCategory(item) != TOOL && itemCategory(item) != BOOK )
			{
				continue;
			}
		}
		c++;
	}
	//Sanitize item scroll.
	shopitemscroll[player] = std::max(0, std::min(shopitemscroll[player], c - NUM_SHOP_GUI_SLOTS));
	//Clear out currently displayed items.
	for ( c = 0; c < 4; c++ )
	{
		shopinvitems[player][c] = NULL;
	}

	//Display the items.
	c = 0;
	for ( node = shopInv[player]->first; node != NULL; node = node->next )
	{
		Item* item = (Item*) node->element;
		if (item)
		{
			if ( mysteriousShopkeeper )
			{
				if ( !mysteriousShopkeeperBlueOrb
					&& shopkeeperMysteriousItems[ARTIFACT_ORB_BLUE].find(item->type) != shopkeeperMysteriousItems[ARTIFACT_ORB_BLUE].end() )
				{
					continue;
				}
				if ( !mysteriousShopkeeperGreenOrb
					&& shopkeeperMysteriousItems[ARTIFACT_ORB_GREEN].find(item->type) != shopkeeperMysteriousItems[ARTIFACT_ORB_GREEN].end() )
				{
					continue;
				}
				if ( !mysteriousShopkeeperRedOrb
					&& shopkeeperMysteriousItems[ARTIFACT_ORB_RED].find(item->type) != shopkeeperMysteriousItems[ARTIFACT_ORB_RED].end() )
				{
					continue;
				}
			}
			if ( hideItemFromShopView(*item) )
			{
				continue;
			}
			if ( shopinventorycategory[player] == 0 && itemCategory(item) != WEAPON && itemCategory(item) != THROWN )
			{
				continue;
			}
			else if ( shopinventorycategory[player] == 1 && itemCategory(item) != ARMOR )
			{
				continue;
			}
			else if ( shopinventorycategory[player] == 2 && itemCategory(item) != AMULET && itemCategory(item) != RING )
			{
				continue;
			}
			else if ( shopinventorycategory[player] == 3 && itemCategory(item) != SPELLBOOK && itemCategory(item) != MAGICSTAFF && itemCategory(item) != SCROLL )
			{
				continue;
			}
			else if ( shopinventorycategory[player] == 4 && itemCategory(item) != GEM )
			{
				continue;
			}
			else if ( shopinventorycategory[player] == 5 && itemCategory(item) != FOOD && itemCategory(item) != POTION )
			{
				continue;
			}
			else if ( shopinventorycategory[player] == 6 && itemCategory(item) != TOOL && itemCategory(item) != BOOK )
			{
				continue;
			}
			c++;
			if ( c <= shopitemscroll[player] )
			{
				continue;
			}
			shopinvitems[player][c - shopitemscroll[player] - 1] = item;
			if ( c > 3 + shopitemscroll[player] )
			{
				break;
			}
		}
	}
}

struct Area
{
public:
	int x1, x2, y1, y2;

	Area(int inX1 = 0, int inX2 = 0, int inY1 = 0, int inY2 = 0)
	{
		x1 = inX1;
		x2 = inX2;
		y1 = inY1;
		y2 = inY2;
	}
};

inline void checkBuyItem(const int player)
{
	if ( player < 0 || (stats[player]->HP <= 0) || (players[player] == nullptr) || (players[player]->entity == nullptr) )
	{
		return;
	}

	//const Sint32 mousex = inputs.getMouse(player, Inputs::X);
	//const Sint32 mousey = inputs.getMouse(player, Inputs::Y);
	//const Sint32 omousex = inputs.getMouse(player, Inputs::OX);
	//const Sint32 omousey = inputs.getMouse(player, Inputs::OY);
	//const Sint32 mousexrel = inputs.getMouse(player, Inputs::XREL);
	//const Sint32 mouseyrel = inputs.getMouse(player, Inputs::YREL);

	//Window bounds.
	Area shopWindow(xres / 2 - SHOPWINDOW_SIZEX / 2, xres / 2 + SHOPWINDOW_SIZEX / 2, yres / 2 - SHOPWINDOW_SIZEY / 2, yres / 2 + SHOPWINDOW_SIZEY / 2);

	//Location of inventory slots.
	Area slotAreaBounds;
	slotAreaBounds.x1 = shopWindow.x1 + (shopWindow.x2 - shopWindow.x1) / 2 - inventory_bmp->w / 2 + 12;
	slotAreaBounds.x2 = slotAreaBounds.x1 + inventoryoption_bmp->w; //Original code: inventory_bmp->w - 28
	slotAreaBounds.y1 = shopWindow.y1 + 16 + 160 + 16;
	slotAreaBounds.y2 = slotAreaBounds.y1 + (inventoryoption_bmp->h * NUM_SHOP_GUI_SLOTS);

	SDL_Rect slotPos;
	slotPos.x = slotAreaBounds.x1;
	slotPos.w = inventoryoption_bmp->w;
	slotPos.y = slotAreaBounds.y1;
	slotPos.h = inventoryoption_bmp->h;

	if ( (omousex < slotPos.x) || (omousex >= slotPos.x + slotPos.w) )
	{
		selectedShopSlot[player] = -1;
		return;
	}

	bool selectingSlot = false;

	for (int i = 0; i < NUM_SHOP_GUI_SLOTS; ++i, slotPos.y += slotPos.h)
	{
		if ( omousey >= slotPos.y && omousey < slotPos.y + slotPos.h )
		{
			//If moused over, highlight slot & check for mouse click or gamepad buy button.
			selectedShopSlot[player] = i;
			selectingSlot = true;
			drawImage(inventoryoption_bmp, nullptr, &slotPos);
			if ( inputs.bMouseLeft(player) || inputs.bControllerInputPressed(player, INJOY_MENU_USE) )
			{
				inputs.mouseClearLeft(player);
				inputs.controllerClearInput(player, INJOY_MENU_USE);
				bool consumedEntireStack = false;
				buyItemFromShop(player, shopinvitems[player][i], consumedEntireStack);
				//Check if no more items after this slot & deal accordingly.
				rebuildShopInventory(player);
				if ( shopinvitems[player][i] == nullptr )
				{
					if ( shopinvitems[player][0] == nullptr )
					{
						//Go back to inventory.
						selectedShopSlot[player] = -1;
						players[player]->inventoryUI.warpMouseToSelectedItem(nullptr, (Inputs::SET_CONTROLLER));
					}
					else
					{
						//Move up one slot.
						--selectedShopSlot[player];
						warpMouseToSelectedShopSlot(player);
					}
				}
			}
		}
	}

	if ( !selectingSlot )
	{
		selectedShopSlot[player] = -1;
	}
}

void warpMouseToSelectedShopSlot(const int player)
{
	Area shopWindow(xres / 2 - SHOPWINDOW_SIZEX / 2, xres / 2 + SHOPWINDOW_SIZEX / 2, yres / 2 - SHOPWINDOW_SIZEY / 2, yres / 2 + SHOPWINDOW_SIZEY / 2);

	Area slotAreaBounds;
	slotAreaBounds.x1 = shopWindow.x1 + (shopWindow.x2 - shopWindow.x1) / 2 - inventory_bmp->w / 2 + 12;
	slotAreaBounds.x2 = slotAreaBounds.x1 + inventoryoption_bmp->w; //Original code: inventory_bmp->w - 28
	slotAreaBounds.y1 = shopWindow.y1 + 16 + 160 + 16;
	slotAreaBounds.y2 = slotAreaBounds.y1 + (inventoryoption_bmp->h * NUM_SHOP_GUI_SLOTS);

	SDL_Rect slotPos;
	slotPos.x = slotAreaBounds.x1;
	slotPos.w = inventoryoption_bmp->w;
	slotPos.h = inventoryoption_bmp->h;
	slotPos.y = slotAreaBounds.y1 + (inventoryoption_bmp->h * selectedShopSlot[player]);

	SDL_WarpMouseInWindow(screen, slotPos.x + (slotPos.w / 2), slotPos.y + (slotPos.h / 2));
}

bool getShopFreeSlot(const int player, list_t* shopInventory, Item* itemToSell, int& xout, int& yout, Item*& itemToStackInto)
{
	xout = Player::ShopGUI_t::MAX_SHOP_X;
	yout = Player::ShopGUI_t::MAX_SHOP_Y;

	list_t* shopkeeperInv = nullptr;
	if ( player >= 0 )
	{
		shopkeeperInv = shopInv[player];
	}
	else
	{
		shopkeeperInv = shopInventory;
	}
	if ( !itemToSell || !shopkeeperInv )
	{
		return false;
	}

	bool lookForStackableItem = itemToSell->shouldItemStackInShop();
	if ( lookForStackableItem )
	{
		if ( itemTypeIsQuiver(itemToSell->type) )
		{
			lookForStackableItem = false; // don't stack, as we always buy the whole stack
		}
	}
	if ( lookForStackableItem )
	{
		for ( node_t* node = shopkeeperInv->first; node != NULL; node = node->next )
		{
			Item* item = (Item*)node->element;
			if ( item )
			{
				if ( hideItemFromShopView(*item) )
				{
					continue;
				}
				if ( !item->playerSoldItemToShop )
				{
					continue;
				}
				if ( !itemCompare(item, itemToSell, false) )
				{
					xout = item->x;
					yout = item->y;
					itemToStackInto = item;
					return true;
				}
			}
		}
	}

	std::unordered_set<int> takenSlots;
	for ( node_t* node = shopkeeperInv->first; node != NULL; node = node->next )
	{
		Item* item = (Item*)node->element;
		if ( item )
		{
			if ( hideItemFromShopView(*item) )
			{
				continue;
			}
			if ( !item->playerSoldItemToShop )
			{
				continue;
			}
			int key = item->x + 100 * item->y;
			if ( item->x >= 0 && item->x < Player::ShopGUI_t::MAX_SHOP_X
				&& item->y >= 0 && item->y < Player::ShopGUI_t::MAX_SHOP_Y )
			{
				takenSlots.insert(key);
			}
		}
	}

	for ( int y = 0; y < Player::ShopGUI_t::MAX_SHOP_Y; ++y )
	{
		for ( int x = 0; x < Player::ShopGUI_t::MAX_SHOP_X; ++x )
		{
			int key = x + 100 * y;
			if ( takenSlots.find(key) == takenSlots.end() )
			{
				xout = x;
				yout = y;
				return true;
			}
		}
	}
	return false;
}

/*-------------------------------------------------------------------------------

	updateShopWindow

	Draws and processes everything related to the shop window

-------------------------------------------------------------------------------*/

void updateShopWindow(const int player)
{
	SDL_Rect pos;
	node_t* node;
	int c;

	if ( player < 0 )
	{
		return;
	}
	if ( !uidToEntity(shopkeeper[player]) )
	{
		closeShop(player);
		return;
	}

	if ( multiplayer != CLIENT && players[player]->isLocalPlayer() )
	{
		Entity* entity = uidToEntity(shopkeeper[player]);
		if (entity)
		{
			Stat* stats = entity->getStats();
			shopkeepername[player] = stats->name;
		}
	}

	bool mysteriousShopkeeper = (shopkeepertype[player] == 10);
	bool mysteriousShopkeeperGreenOrb = false;
	bool mysteriousShopkeeperBlueOrb = false;
	bool mysteriousShopkeeperRedOrb = false;
	if ( mysteriousShopkeeper )
	{
		for ( node_t* node = shopInv[player]->first; node != NULL; node = node->next )
		{
			Item* item = (Item*)node->element;
			if ( item )
			{
				if ( item->type == ARTIFACT_ORB_BLUE )
				{
					mysteriousShopkeeperBlueOrb = true;
					item->itemHiddenFromShop = true;
				}
				else if ( item->type == ARTIFACT_ORB_RED )
				{
					mysteriousShopkeeperRedOrb = true;
					item->itemHiddenFromShop = true;
				}
				else if ( item->type == ARTIFACT_ORB_GREEN )
				{
					mysteriousShopkeeperGreenOrb = true;
					item->itemHiddenFromShop = true;
				}
			}
		}
		for ( node = shopInv[player]->first; node != NULL; node = node->next )
		{
			Item* item = (Item*)node->element;
			if ( item )
			{
				if ( shopkeeperMysteriousItems[ARTIFACT_ORB_BLUE].find(item->type) != shopkeeperMysteriousItems[ARTIFACT_ORB_BLUE].end() )
				{
					item->itemHiddenFromShop = !mysteriousShopkeeperBlueOrb;
					continue;
				}
				if ( shopkeeperMysteriousItems[ARTIFACT_ORB_GREEN].find(item->type) != shopkeeperMysteriousItems[ARTIFACT_ORB_GREEN].end() )
				{
					item->itemHiddenFromShop = !mysteriousShopkeeperGreenOrb;
					continue;
				}
				if ( shopkeeperMysteriousItems[ARTIFACT_ORB_RED].find(item->type) != shopkeeperMysteriousItems[ARTIFACT_ORB_RED].end() )
				{
					item->itemHiddenFromShop = !mysteriousShopkeeperRedOrb;
					continue;
				}
			}
		}
	}

	// draw window
	int x1 = xres / 2 + SHOPWINDOW_SIZEX / 4;
	int x2 = xres / 2 + SHOPWINDOW_SIZEX / 4 + SHOPWINDOW_SIZEX;
	int y1 = yres / 2 - SHOPWINDOW_SIZEY / 2;
	int y2 = yres / 2 + SHOPWINDOW_SIZEY / 2;
	drawWindowFancy( x1, y1, x2, y2 );

	// clicking
	int x = x1 + (x2 - x1) / 2 - inventory_bmp->w / 2;
	int y = y1 + 16 + 160;
	pos.x = x;
	pos.y = y;
	drawImage(inventory_bmp, NULL, &pos);
	if ( mousestatus[SDL_BUTTON_LEFT] )
	{
		if ( omousey >= y && omousey < y + 16 )
		{
			if ( omousex >= x + 12 && omousex < x + inventory_bmp->w - 12 )
			{
				mousestatus[SDL_BUTTON_LEFT] = 0;
				shopinventorycategory[player] = (omousex - x - 12) / button_bmp->w;
			}
		}
		else if ( omousey >= y + 16 && omousey < y + 52 )
		{
			if ( omousex >= x + inventory_bmp->w - 28 && omousex < x + inventory_bmp->w - 12 )
			{
				mousestatus[SDL_BUTTON_LEFT] = 0;
				buttonclick = 10;
				shopitemscroll[player]--;
			}
		}
		else if ( omousey >= y + 52 && omousey < y + 88 )
		{
			if ( omousex >= x + inventory_bmp->w - 28 && omousex < x + inventory_bmp->w - 12 )
			{
				mousestatus[SDL_BUTTON_LEFT] = 0;
				buttonclick = 11;
				shopitemscroll[player]++;
			}
		}
	}

	// mousewheel
	if ( omousex >= x + 12 && omousex < x + inventory_bmp->w - 28 )
	{
		if ( omousey >= 16 && omousey < y + inventory_bmp->h - 8 )
		{
			if ( mousestatus[SDL_BUTTON_WHEELDOWN] )
			{
				mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
				shopitemscroll[player]++;
			}
			else if ( mousestatus[SDL_BUTTON_WHEELUP] )
			{
				mousestatus[SDL_BUTTON_WHEELUP] = 0;
				shopitemscroll[player]--;
			}
		}
	}

	// inventory up button
	if ( buttonclick == 10 )
	{
		pos.x = x + inventory_bmp->w - 28;
		pos.y = y + 16;
		pos.w = 0;
		pos.h = 0;
		drawImage(invup_bmp, NULL, &pos);
	}
	// inventory down button
	if ( buttonclick == 11 )
	{
		pos.x = x + inventory_bmp->w - 28;
		pos.y = y + 52;
		pos.w = 0;
		pos.h = 0;
		drawImage(invdown_bmp, NULL, &pos);
	}
	pos.x = x + 12 + button_bmp->w * shopinventorycategory[player];
	pos.y = y;
	pos.w = 0;
	pos.h = 0;
	if ( shopinventorycategory[player] <= 6 )
	{
		drawImage(button_bmp, NULL, &pos);
	}
	else
	{
		drawImage(smallbutton_bmp, NULL, &pos);
	}

	// inventory category labels
	ttfPrintText(ttf8, x + 14, y + 4, language[349]);
	ttfPrintText(ttf8, x + 14 + 60, y + 4, language[350]);
	ttfPrintText(ttf8, x + 14 + 120, y + 4, language[351]);
	ttfPrintText(ttf8, x + 14 + 180, y + 4, language[352]);
	ttfPrintText(ttf8, x + 14 + 240, y + 4, language[353]);
	ttfPrintText(ttf8, x + 14 + 300, y + 4, language[354]);
	ttfPrintText(ttf8, x + 14 + 360, y + 4, language[355]);
	ttfPrintText(ttf8, x + 12 + 424, y + 4, language[356]);

	// buying
	checkBuyItem(player);


	rebuildShopInventory(player);

	int y3 = y + 22;

	if ( mysteriousShopkeeper )
	{
		for ( node = shopInv[player]->first; node != NULL; node = node->next )
		{
			Item* item = (Item*)node->element;
			if ( item )
			{
				if ( item->type == ARTIFACT_ORB_BLUE )
				{
					mysteriousShopkeeperBlueOrb = true;
				}
				else if ( item->type == ARTIFACT_ORB_RED )
				{
					mysteriousShopkeeperRedOrb = true;
				}
				else if ( item->type == ARTIFACT_ORB_GREEN )
				{
					mysteriousShopkeeperGreenOrb = true;
				}
			}
		}
	}

	c = 0;
	for ( node = shopInv[player]->first; node != NULL; node = node->next )
	{
		Item* item = (Item*) node->element;
		if (item)
		{
			if ( mysteriousShopkeeper )
			{
				if ( !mysteriousShopkeeperBlueOrb
					&& shopkeeperMysteriousItems[ARTIFACT_ORB_BLUE].find(item->type) != shopkeeperMysteriousItems[ARTIFACT_ORB_BLUE].end() )
				{
					continue;
				}
				if ( !mysteriousShopkeeperGreenOrb 
					&& shopkeeperMysteriousItems[ARTIFACT_ORB_GREEN].find(item->type) != shopkeeperMysteriousItems[ARTIFACT_ORB_GREEN].end() )
				{
					continue;
				}
				if ( !mysteriousShopkeeperRedOrb
					&& shopkeeperMysteriousItems[ARTIFACT_ORB_RED].find(item->type) != shopkeeperMysteriousItems[ARTIFACT_ORB_RED].end() )
				{
					continue;
				}
			}
			if ( hideItemFromShopView(*item) )
			{
				continue;
			}
			if ( shopinventorycategory[player] == 0 && itemCategory(item) != WEAPON && itemCategory(item) != THROWN )
			{
				continue;
			}
			else if ( shopinventorycategory[player] == 1 && itemCategory(item) != ARMOR )
			{
				continue;
			}
			else if ( shopinventorycategory[player] == 2 && itemCategory(item) != AMULET && itemCategory(item) != RING )
			{
				continue;
			}
			else if ( shopinventorycategory[player] == 3 && itemCategory(item) != SPELLBOOK && itemCategory(item) != MAGICSTAFF && itemCategory(item) != SCROLL )
			{
				continue;
			}
			else if ( shopinventorycategory[player] == 4 && itemCategory(item) != GEM )
			{
				continue;
			}
			else if ( shopinventorycategory[player] == 5 && itemCategory(item) != FOOD && itemCategory(item) != POTION )
			{
				continue;
			}
			else if ( shopinventorycategory[player] == 6 && itemCategory(item) != TOOL && itemCategory(item) != BOOK )
			{
				continue;
			}
			c++;
			if ( c <= shopitemscroll[player] )
			{
				continue;
			}
			char tempstr[64] = { 0 };
			strncpy(tempstr, item->description(), 42);
			if ( strlen(tempstr) == 42 )
			{
				strcat(tempstr, " ...");
			}
			Uint32 color = uint32ColorWhite;
			if ( item->beatitude > 0 && stats[player]->PROFICIENCIES[PRO_APPRAISAL] >= SKILL_LEVEL_EXPERT )
			{
				color = uint32ColorGreen;
			}
			ttfPrintTextColor(ttf8, x + 12 + 36, y3, color, true, tempstr);
			ttfPrintTextFormatted(ttf8, x + 12 + 348, y3, "%7dG", item->buyValue(player));

			if ( mysteriousShopkeeper )
			{
				pos.x = x + 12 + (348);
				pos.y = y + 17 + 18 * (c - shopitemscroll[player] - 1);
				pos.w = 16;
				pos.h = 16;

				for ( auto orbCategories : shopkeeperMysteriousItems )
				{
					if ( orbCategories.second.find(item->type) != orbCategories.second.end() )
					{
						node_t* tmpNode = items[orbCategories.first].surfaces.first;
						if ( tmpNode )
						{
							drawImageScaled(*(SDL_Surface**)(tmpNode->element), NULL, &pos);
						}
						break;
					}
				}
			}

			pos.x = x + 12 + 16;
			pos.y = y + 17 + 18 * (c - shopitemscroll[player] - 1);
			pos.w = 16;
			pos.h = 16;
			drawImageScaled(itemSprite(item), NULL, &pos);
			y3 += 18;
			if ( c > 3 + shopitemscroll[player] )
			{
				break;
			}
		}
	}

	// draw money count
	ttfPrintTextFormatted( ttf16, x1 + 16, y2 - 32, language[357], stats[player]->GOLD );

	// chitchat
	if ( (ticks - shoptimer[player]) % 600 == 0 && !mysteriousShopkeeper )
	{
		shopspeech[player] = language[216 + rand() % NUMCHITCHAT];
		shoptimer[player]--;
	}

	// draw speech
	char buf[1024];
	if ( sellitem[clientnum] && !strcmp(shopspeech[player], language[215]) )
	{
		// "I would sell that for %d gold"
		ttfPrintTextFormatted(ttf16, x1 + 16 + 160 + 16, y1 + 32, shopspeech[player], sellitem[player]->sellValue(player));
		snprintf(buf, sizeof(buf), shopspeech[player], sellitem[player]->sellValue(player));
		if ( players[player]->shopGUI.chatStrFull != buf )
		{
			players[player]->shopGUI.chatTicks = ticks;
			players[player]->shopGUI.chatStrFull = buf;
			players[player]->shopGUI.chatStringLength = 0;
		}
	}
	else
	{
		if ( !strcmp(shopspeech[player], language[194]) // greetings
			|| !strcmp(shopspeech[player], language[195]) 
			|| !strcmp(shopspeech[player], language[196]) )
		{
			if ( mysteriousShopkeeper )
			{
				shopspeech[player] = language[3893 + rand() % 3];
				if ( players[player]->shopGUI.chatStrFull != shopspeech[player] )
				{
					players[player]->shopGUI.chatTicks = ticks;
					players[player]->shopGUI.chatStrFull = shopspeech[player];
					players[player]->shopGUI.chatStringLength = 0;
				}
			}
			else
			{
				char shopnamebuf[1024];
				ttfPrintTextFormatted( ttf16, x1 + 16 + 160 + 16, y1 + 64, language[358], shopkeepername[player], language[184 + shopkeepertype[player]] );
				snprintf(shopnamebuf, sizeof(shopnamebuf), language[358], shopkeepername[player], language[184 + shopkeepertype[player]]);

				snprintf(buf, sizeof(buf), "%s\n%s", shopspeech[player], shopnamebuf); // greetings, welcome to %s's shop
				if ( players[player]->shopGUI.chatStrFull != buf )
				{
					players[player]->shopGUI.chatTicks = ticks;
					players[player]->shopGUI.chatStrFull = buf;
					players[player]->shopGUI.chatStringLength = 0;
				}
			}
		}
		else
		{
			char buf[1024];
			ttfPrintTextFormatted(ttf16, x1 + 16 + 160 + 16, y1 + 32, shopspeech[player], 0);
			snprintf(buf, sizeof(buf), shopspeech[player], 0);
			if ( players[player]->shopGUI.chatStrFull != buf )
			{
				players[player]->shopGUI.chatTicks = ticks;
				players[player]->shopGUI.chatStrFull = buf;
				players[player]->shopGUI.chatStringLength = 0;
			}
		}
	}
}

inline Item* getItemInfoFromShop(const int player, int slot)
{
	if ( slot >= 4 )
	{
		return nullptr; //Out of bounds,
	}

	return shopinvitems[player][slot];
}

void selectShopSlot(const int player, int slot)
{
	if ( slot < selectedShopSlot[player] )
	{
		//Moving up.

		/*
		 * Possible cases:
		 * * 1) Move cursor up the GUI through different selectedShopSlot.
		 * * 2) Page up through shopitemscroll--
		 * * 3) Scrolling up past top of shop, no shopitemscroll (move back to inventory)
		 */

		if ( selectedShopSlot[player] <= 0 )
		{
			//Covers cases 2 & 3.

			/*
			 * Possible cases:
			 * * A) Hit very top of shop inventory, can't go any further. Return to inventory.
			 * * B) Page up, scrolling through shopitemscroll.
			 */

			if ( shopitemscroll[player] <= 0 )
			{
				//Case 3/A: Return to inventory.
				selectedShopSlot[player] = -1;
			}
			else
			{
				//Case 2/B: Page up through shop inventory.
				--shopitemscroll[player];
			}
		}
		else
		{
			//Covers case 1.

			//Move cursor up the GUI through different selectedShopSlot (--selectedShopSlot).
			--selectedShopSlot[player];
			warpMouseToSelectedShopSlot(player);
		}
	}
	else if ( slot > selectedShopSlot[player] )
	{
		//Moving down.

		/*
		 * Possible cases:
		 * * 1) Moving cursor down through GUI through different selectedShopSlot.
		 * * 2) Scrolling down past bottom of shop through shopitemscroll++
		 * * 3) Scrolling down past bottom of shop, max shop scroll (revoke move -- can't go beyond limit of shop).
		 */

		if ( selectedShopSlot[player] >= NUM_SHOP_GUI_SLOTS - 1 )
		{
			//Covers cases 2 & 3.
			++shopitemscroll[player]; //Shopitemscroll is automatically sanitized in updateShopWindow().
		}
		else
		{
			//Covers case 1.
			//Move cursor down through the GUi through different selectedShopSlot (++selectedShopSlot).
			//This is a little bit trickier since must revoke movement if there is no item in the next slot!

			/*
			 * Two possible cases:
			 * * A) Items below this. Advance selectedShopSlot to them.
			 * * B) On last item already. Do nothing (revoke movement).
			 */

			Item* item = getItemInfoFromShop(player, selectedShopSlot[player] + 1);

			if ( item )
			{
				++selectedShopSlot[player];
				warpMouseToSelectedShopSlot(player);
			}
			else
			{
				//No more items. Stop.
			}
		}
	}
}

void cycleShopCategories(const int player, int direction)
{
	if ( direction < 0 )
	{
		//Cycle left.
		--shopinventorycategory[player];
	}
	else if ( direction > 0 )
	{
		//Cycle right.
		++shopinventorycategory[player];
	}

	if ( shopinventorycategory[player] < 0 )
	{
		shopinventorycategory[player] = NUM_SHOP_CATEGORIES - 1;
	}
	else if ( shopinventorycategory[player] >= NUM_SHOP_CATEGORIES )
	{
		shopinventorycategory[player] = 0;
	}
}

void shopChangeGoldEvent(const int player, Sint32 amount)
{
	if ( !players[player]->isLocalPlayer() )
	{
		return;
	}

	bool addedToCurrentTotal = false;
	bool isAnimatingValue = ((ticks - players[player]->shopGUI.animGoldStartTicks) > TICKS_PER_SECOND / 2);
	if ( amount < 0 )
	{
		if ( players[player]->shopGUI.playerChangeGold < 0 
			&& !isAnimatingValue 
			&& abs(amount) > 0 )
		{
			addedToCurrentTotal = true;
			if ( stats[player]->GOLD + amount < 0 )
			{
				players[player]->shopGUI.playerChangeGold -= stats[player]->GOLD;
			}
			else
			{
				players[player]->shopGUI.playerChangeGold += amount;
			}
		}
		else
		{
			if ( stats[player]->GOLD + amount < 0 )
			{
				players[player]->shopGUI.playerChangeGold = -stats[player]->GOLD;
			}
			else
			{
				players[player]->shopGUI.playerChangeGold = amount;
			}
		}
	}
	else
	{
		if ( players[player]->shopGUI.playerChangeGold > 0 
			&& !isAnimatingValue
			&& abs(amount) > 0 )
		{
			addedToCurrentTotal = true;
			players[player]->shopGUI.playerChangeGold += amount;
		}
		else
		{
			players[player]->shopGUI.playerChangeGold = amount;
		}
	}
	players[player]->shopGUI.animGoldStartTicks = ticks;
	players[player]->shopGUI.animGold = 0.0;
	if ( !addedToCurrentTotal )
	{
		players[player]->shopGUI.playerCurrentGold = stats[player]->GOLD;
	}
}

void Player::ShopGUI_t::openShop()
{
	if ( shopFrame )
	{
		bool wasDisabled = shopFrame->isDisabled();
		shopFrame->setDisabled(false);
		if ( wasDisabled )
		{
			animx = 0.0;
			animTooltip = 0.0;
			isInteractable = false;
			bFirstTimeSnapCursor = false;
		}
		if ( getSelectedShopX() < 0 || getSelectedShopX() >= MAX_SHOP_X
			|| getSelectedShopY() < 0 || getSelectedShopY() >= MAX_SHOP_Y )
		{
			selectShopSlot(0, 0);
		}
		player.hud.compactLayoutMode = Player::HUD_t::COMPACT_LAYOUT_INVENTORY;
		player.inventory_mode = INVENTORY_MODE_ITEM;
		player.gui_mode = GUI_MODE_SHOP;
		bOpen = true;
	}
	if ( inputs.getUIInteraction(player.playernum)->selectedItem )
	{
		inputs.getUIInteraction(player.playernum)->selectedItem = nullptr;
		inputs.getUIInteraction(player.playernum)->toggleclick = false;
	}
	inputs.getUIInteraction(player.playernum)->selectedItemFromChest = 0;
	clearItemDisplayed();
	shopChangeGoldEvent(player.playernum, 0);
	buybackView = false;
}

void Player::ShopGUI_t::selectShopSlot(const int x, const int y)
{
	selectedShopSlotX = x;
	selectedShopSlotY = y;
}

void Player::ShopGUI_t::closeShop()
{
	if ( shopFrame )
	{
		shopFrame->setDisabled(true);
	}
	animx = 0.0;
	animTooltip = 0.0;
	isInteractable = false;
	bool wasOpen = bOpen;
	bOpen = false;
	bFirstTimeSnapCursor = false;
	if ( wasOpen )
	{
		if ( inputs.getUIInteraction(player.playernum)->selectedItem )
		{
			inputs.getUIInteraction(player.playernum)->selectedItem = nullptr;
			inputs.getUIInteraction(player.playernum)->toggleclick = false;
		}
		inputs.getUIInteraction(player.playernum)->selectedItemFromChest = 0;
	}
	if ( players[player.playernum]->GUI.activeModule == Player::GUI_t::MODULE_SHOP
		&& !players[player.playernum]->shootmode )
	{
		// reset to inventory mode if still hanging in shop GUI
		players[player.playernum]->hud.compactLayoutMode = Player::HUD_t::COMPACT_LAYOUT_INVENTORY;
		players[player.playernum]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
		if ( !inputs.getVirtualMouse(player.playernum)->draw_cursor )
		{
			players[player.playernum]->GUI.warpControllerToModule(false);
		}
	}
	if ( !players[player.playernum]->shootmode )
	{
		if ( players[player.playernum]->gui_mode == GUI_MODE_SHOP )
		{
			players[player.playernum]->gui_mode = GUI_MODE_INVENTORY;
		}
	}

	if ( auto bgFrame = shopFrame->findFrame("shop base") )
	{
		if ( auto discountFrame = bgFrame->findFrame("discount frame") )
		{
			if ( auto discountValue = discountFrame->findField("discount") )
			{
				discountValue->setText("");
			}
		}
	}
	clearItemDisplayed();
	itemRequiresTitleReflow = true;
	shopChangeGoldEvent(player.playernum, 0);
	buybackView = false;
	chatStrFull = "";
	chatStringLength = 0;
	chatTicks = 0;
}

bool Player::ShopGUI_t::isShopSelected()
{
	if ( !bOpen )
	{
		return false;
	}

	if ( player.GUI.activeModule == Player::GUI_t::MODULE_SHOP )
	{
		if ( selectedShopSlotX >= 0 && selectedShopSlotX < MAX_SHOP_X
			&& selectedShopSlotY >= 0 && selectedShopSlotY < MAX_SHOP_Y )
		{
			return true;
		}
	}

	return false;
}

int Player::ShopGUI_t::heightOffsetWhenNotCompact = 172;

void updatePlayerGold(const int player)
{
	auto shopFrame = players[player]->shopGUI.shopFrame;
	if ( !shopFrame )
	{
		return;
	}

	auto bgFrame = shopFrame->findFrame("shop base");
	assert(bgFrame);
	auto currentGoldText = bgFrame->findField("current gold");
	auto changeGoldText = bgFrame->findField("change gold");

	SDL_Rect changeGoldPos = changeGoldText->getSize();
	const int changePosAnimHeight = 10;
	changeGoldPos.y = bgFrame->getSize().h - 92 - 16;

	real_t& animGold = players[player]->shopGUI.animGold;
	bool pauseChangeGoldAnim = false;

	real_t& animNoDeal = players[player]->shopGUI.animNoDeal;
	Uint32& animNoDealTicks = players[player]->shopGUI.animNoDealTicks;

	if ( players[player]->shopGUI.playerChangeGold != 0 )
	{
		if ( ((ticks - players[player]->shopGUI.animGoldStartTicks) > TICKS_PER_SECOND / 2) )
		{
			const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.1, (animGold)) / 10.0;
			animGold -= setpointDiffX;
			animGold = std::max(0.0, animGold);

			if ( animGold <= 0.0001 )
			{
				players[player]->shopGUI.playerChangeGold = 0;
			}
		}
		else
		{
			pauseChangeGoldAnim = true;

			const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animGold)) / 10.0;
			animGold += setpointDiffX;
			animGold = std::min(1.0, animGold);

			changeGoldPos.y += changePosAnimHeight;
			changeGoldPos.y -= animGold * changePosAnimHeight;
		}
	}

	changeGoldText->setSize(changeGoldPos);

	{ 
		// constant decay for animation
		const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
		real_t setpointDiffX = fpsScale * 1.0 / 25.0;
		animNoDeal -= setpointDiffX;
		animNoDeal = std::max(0.0, animNoDeal);

		SDL_Rect currentGoldTextPos = currentGoldText->getSize();
		currentGoldTextPos.x = bgFrame->getSize().w - 80 - 18;
		currentGoldTextPos.x += -2 + 2 * (cos(animNoDeal * 4 * PI));
		currentGoldText->setSize(currentGoldTextPos);
		if ( animNoDeal > 0.001 || (ticks - animNoDealTicks) < TICKS_PER_SECOND * .8 )
		{
			currentGoldText->setColor(makeColor(215, 38, 61, 255)); // red
		}
		else
		{
			currentGoldText->setColor(makeColor(201, 162, 100, 255));
		}
	}

	if ( keystatus[SDL_SCANCODE_G] )
	{
		if ( keystatus[SDL_SCANCODE_LSHIFT] )
		{
			shopChangeGoldEvent(player, -(50 + rand() % 50));
			if ( keystatus[SDL_SCANCODE_RSHIFT] )
			{
				shopChangeGoldEvent(player, 0);
			}
		}
		else if ( keystatus[SDL_SCANCODE_RSHIFT] )
		{
			shopspeech[clientnum] = language[217];
		}
		else
		{
			shopChangeGoldEvent(player, 50 + rand() % 50);
		}
		keystatus[SDL_SCANCODE_G] = 0;
	}

	bool showChangedGold = false;
	if ( players[player]->shopGUI.playerChangeGold != 0 )
	{
		Sint32 displayedChangeGold = animGold * players[player]->shopGUI.playerChangeGold;
		if ( pauseChangeGoldAnim )
		{
			displayedChangeGold = players[player]->shopGUI.playerChangeGold;
		}
		if ( abs(displayedChangeGold) > 0 )
		{
			showChangedGold = true;
			changeGoldText->setDisabled(false);
			std::string s = "+";
			if ( players[player]->shopGUI.playerChangeGold < 0 )
			{
				s = "";
			}
			s += std::to_string(displayedChangeGold);
			changeGoldText->setText(s.c_str());
			Sint32 displayedCurrentGold = players[player]->shopGUI.playerCurrentGold 
				+ (players[player]->shopGUI.playerChangeGold - displayedChangeGold);
			currentGoldText->setText(std::to_string(displayedCurrentGold).c_str());
		}
	}
	
	if ( !showChangedGold )
	{
		Sint32 displayedChangeGold = 0;
		changeGoldText->setDisabled(true);
		changeGoldText->setText(std::to_string(displayedChangeGold).c_str());
		currentGoldText->setText(std::to_string(stats[player]->GOLD).c_str());
	}
}

void updateShopGUIChatter(const int player)
{
	auto shopFrame = players[player]->shopGUI.shopFrame;
	if ( !shopFrame )
	{
		return;
	}

	auto bgFrame = shopFrame->findFrame("shop base");
	assert(bgFrame);
	auto chatWindow = bgFrame->findFrame("chatter");
	assert(chatWindow);

	auto tl = chatWindow->findImage("top left img");
	auto tm = chatWindow->findImage("top img");
	auto tr = chatWindow->findImage("top right img");

	auto ml = chatWindow->findImage("middle left img");
	auto mm1 = chatWindow->findImage("middle 1 img");
	auto mm2 = chatWindow->findImage("middle 2 img");
	auto mr = chatWindow->findImage("middle right img");

	auto bl = chatWindow->findImage("bottom left img");
	auto bm = chatWindow->findImage("bottom img");
	auto br = chatWindow->findImage("bottom right img");

	auto pointer = chatWindow->findImage("pointer img");

	auto shopkeeperImg = bgFrame->findImage("shopkeeper img");
	if ( shopkeepertype[player] == 10 )
	{
		shopkeeperImg->path = "images/ui/Shop/shopkeeper2.png";
	}
	else
	{
		shopkeeperImg->path = "images/ui/Shop/shopkeeper.png";
	}

	int width = 288;
	int height = 200;
	const int pointerHeightAddition = 10;
	SDL_Rect chatPos{ bgFrame->getSize().w - 12 - width, 88, width, height + pointerHeightAddition };

	if ( !players[player]->shopGUI.bOpen )
	{
		players[player]->shopGUI.chatStrFull = "";
		players[player]->shopGUI.chatStringLength = 0;
		players[player]->shopGUI.chatTicks = 0;
		chatWindow->setDisabled(true);
		return;
	}

	if ( players[player]->shopGUI.chatStrFull.size() > 1 )
	{
		chatWindow->setDisabled(false);
		auto chatText = chatWindow->findField("chat body");
		if ( players[player]->shopGUI.chatTicks - ticks > 1 )
		{
			players[player]->shopGUI.chatTicks = ticks;

			SDL_Rect textPos{ 22, 18 + pointerHeightAddition, 0, 0 };
			textPos.w = chatPos.w - textPos.x - 14 - 8;
			textPos.h = chatPos.h - 14 - textPos.y;
			chatText->setSize(textPos);

			size_t& currentLen = players[player]->shopGUI.chatStringLength;
			size_t fullLen = players[player]->shopGUI.chatStrFull.size();

			if ( currentLen < fullLen )
			{
				chatText->setText(players[player]->shopGUI.chatStrFull.substr(0U, currentLen + 1).c_str());
				++currentLen;
				if ( auto textGet = chatText->getTextObject() )
				{
					if ( textGet->getWidth() >= textPos.w )
					{
						chatText->reflowTextToFit(0);
					}
				}
			}
			else if ( currentLen > fullLen )
			{
				currentLen = fullLen;
			}

			if ( Font* actualFont = Font::get(chatText->getFont()) )
			{
				textPos.h = std::min(textPos.h, chatText->getNumTextLines() * actualFont->height(true) + 12);
				chatText->setSize(textPos);
			}
		}

		chatPos.h = std::min(chatPos.h, (chatText->getSize().y + chatText->getSize().h));
		chatWindow->setSize(chatPos);

		const int mainTextAreaY = pointerHeightAddition;
		pointer->pos.y = 0;
		pointer->pos.x = chatPos.w - 96;

		tl->pos.x = 0;
		tl->pos.y = mainTextAreaY;
		tr->pos.x = width - tr->pos.w;
		tr->pos.y = tl->pos.y + 6;
		tm->pos.x = tl->pos.x + tl->pos.w;
		tm->pos.y = tl->pos.y + 6;
		tm->pos.w = tr->pos.x - (tl->pos.x + tl->pos.w);

		bl->pos.x = tl->pos.x + 6;
		bl->pos.y = chatPos.h - bl->pos.h;
		br->pos.x = width - br->pos.w;
		br->pos.y = chatPos.h - br->pos.h;

		ml->pos.x = tl->pos.x + 6;
		ml->pos.y = tl->pos.y + tl->pos.h;
		ml->pos.h = bl->pos.y - (tl->pos.y + tl->pos.h);

		mr->pos.x = width - mr->pos.w;
		mr->pos.y = tr->pos.y + tr->pos.h;
		mr->pos.h = br->pos.y - (mr->pos.y);

		bm->pos.x = bl->pos.x + bl->pos.w;
		bm->pos.y = br->pos.y + br->pos.h - bm->pos.h;
		bm->pos.w = tm->pos.w;

		mm1->pos.x = tm->pos.x;
		mm1->pos.y = tm->pos.y + tm->pos.h;
		mm1->pos.h = bm->pos.y - mm1->pos.y;
		mm1->pos.w = tm->pos.w;

		mm2->pos.x = ml->pos.x + ml->pos.w;
		mm2->pos.y = ml->pos.y;
		mm2->pos.h = ml->pos.h;
		mm2->pos.w = mr->pos.x - (ml->pos.x + ml->pos.w);
	}
	else
	{
		chatWindow->setDisabled(true);
	}
}

void Player::ShopGUI_t::clearItemDisplayed()
{
	itemPrice = -1;
}

void Player::ShopGUI_t::setItemDisplayNameAndPrice(Item* item)
{
	if ( !item || item->type == SPELL_ITEM )
	{
		clearItemDisplayed();
	}
	auto bgFrame = shopFrame->findFrame("shop base");
	Field* buyOrSellPrompt = nullptr;
	Frame::image_t* orbImg = nullptr;
	if ( bgFrame )
	{
		auto buyTooltipFrame = bgFrame->findFrame("buy tooltip frame");
		orbImg = buyTooltipFrame->findImage("orb img");
		orbImg->disabled = true;
		buyOrSellPrompt = buyTooltipFrame->findField("buy prompt txt");
	}
	if ( isItemFromShop(item) )
	{
		itemPrice = item->buyValue(player.playernum);
		char buf[1024];
		if ( !item->identified )
		{
			snprintf(buf, sizeof(buf), "%s %s (?)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), item->getName());
		}
		else
		{
			snprintf(buf, sizeof(buf), "%s %s (%+d)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), item->getName(), item->beatitude);
		}
		if ( itemDesc != buf )
		{
			itemRequiresTitleReflow = true;
		}
		itemDesc = buf;
		if ( buyOrSellPrompt )
		{
			buyOrSellPrompt->setText(language[4113]); // 'buy'
		}
		if ( shopkeepertype[player.playernum] == 10 )
		{
			for ( auto orbCategories : shopkeeperMysteriousItems )
			{
				if ( orbCategories.second.find(item->type) != orbCategories.second.end() )
				{
					ItemType oldType = item->type;
					item->type = (ItemType)orbCategories.first;
					orbImg->path = getItemSpritePath(player.playernum, *item);
					orbImg->disabled = false;
					item->type = oldType;
					break;
				}
			}
		}
	}
	else
	{
		itemPrice = item->sellValue(player.playernum);
		char buf[1024];
		if ( !item->identified )
		{
			snprintf(buf, sizeof(buf), "%s %s (?)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), item->getName());
		}
		else
		{
			snprintf(buf, sizeof(buf), "%s %s (%+d)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), item->getName(), item->beatitude);
		}
		if ( itemDesc != buf )
		{
			itemRequiresTitleReflow = true;
		}
		itemDesc = buf;
		if ( buyOrSellPrompt )
		{
			buyOrSellPrompt->setText(language[4114]); // 'sell'
		}
	}

	if ( bgFrame )
	{
		auto itemSlotFrame = bgFrame->findFrame("item slot frame");
		int oldQty = item->count;
		if ( !itemTypeIsQuiver(item->type) )
		{
			item->count = 1;
		}
		updateSlotFrameFromItem(itemSlotFrame, item);
		item->count = oldQty;
	}
}

void Player::ShopGUI_t::updateShop()
{
	DebugTimers.addTimePoint("shop", "1");
	updateShopWindow(player.playernum);

	if ( !shopFrame )
	{
		return;
	}
	shopFrame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
		players[player.playernum]->camera_virtualy1(),
		players[player.playernum]->camera_virtualWidth(),
		players[player.playernum]->camera_virtualHeight() });

	if ( !shopFrame->isDisabled() && bOpen )
	{
		const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
		real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animx)) / 2.0;
		animx += setpointDiffX;
		animx = std::min(1.0, animx);
		if ( animx >= .9999 )
		{
			if ( !bFirstTimeSnapCursor )
			{
				bFirstTimeSnapCursor = true;
				if ( !inputs.getUIInteraction(player.playernum)->selectedItem
					&& player.GUI.activeModule == Player::GUI_t::MODULE_SHOP )
				{
					warpMouseToSelectedShopItem(nullptr, (Inputs::SET_CONTROLLER));
				}
			}
			isInteractable = true;
		}
	}
	else
	{
		animx = 0.0;
		isInteractable = false;
	}

	auto shopFramePos = shopFrame->getSize();
	DebugTimers.addTimePoint("shop", "2");
	if ( player.inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_LEFT )
	{
		if ( !player.inventoryUI.bCompactView )
		{
			const int fullWidth = shopFramePos.w + 210; // inventory width 210
			shopFramePos.x = -shopFramePos.w + animx * fullWidth;
			if ( player.bUseCompactGUIWidth() )
			{
				if ( player.inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				shopFramePos.x -= player.inventoryUI.slideOutWidth * player.inventoryUI.slideOutPercent;
			}
		}
		else
		{
			shopFramePos.x = player.camera_virtualWidth() - animx * shopFramePos.w;
			if ( player.bUseCompactGUIWidth() )
			{
				if ( player.inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				shopFramePos.x -= -player.inventoryUI.slideOutWidth * player.inventoryUI.slideOutPercent;
			}
		}
	}
	else if ( player.inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_RIGHT )
	{
		if ( !player.inventoryUI.bCompactView )
		{
			shopFramePos.x = player.camera_virtualWidth() - animx * shopFramePos.w * 2;
			if ( player.bUseCompactGUIWidth() )
			{
				if ( player.inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				shopFramePos.x -= -player.inventoryUI.slideOutWidth * player.inventoryUI.slideOutPercent;
			}
		}
		else
		{
			shopFramePos.x = -shopFramePos.w + animx * shopFramePos.w;
			if ( player.bUseCompactGUIWidth() )
			{
				if ( player.inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				shopFramePos.x -= player.inventoryUI.slideOutWidth * player.inventoryUI.slideOutPercent;
			}
		}
	}

	if ( !player.bUseCompactGUIHeight() )
	{
		shopFramePos.y = heightOffsetWhenNotCompact;
	}
	else
	{
		shopFramePos.y = 0;
	}

	shopFrame->setSize(shopFramePos);

	bool purchaseItemAction = false;
	bool usingGamepad = (inputs.hasController(player.playernum) && !inputs.getVirtualMouse(player.playernum)->draw_cursor);

	if ( !player.isLocalPlayerAlive()
		|| (player.gui_mode != GUI_MODE_SHOP && player.inventory_mode != INVENTORY_MODE_SPELL) 
		// cycleInventoryTab() sets proper gui mode after spell list viewing with INVENTORY_MODE_SPELL
		// this allows us to browse spells while shop is open.
		|| stats[player.playernum]->HP <= 0
		|| !player.entity
		|| player.shootmode )
	{
		::closeShop(player.playernum);
		return;
	}
	DebugTimers.addTimePoint("shop", "3");
	if ( bOpen && isInteractable && shopInv[player.playernum] )
	{
		if ( !inputs.getUIInteraction(player.playernum)->selectedItem
			&& !player.GUI.isDropdownActive()
			&& player.GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_SHOP)
			&& !player.inventoryUI.chestGUI.bOpen
			&& !player.inventoryUI.spellPanel.bOpen )
		{
			if ( Input::inputs[player.playernum].binaryToggle("MenuCancel") )
			{
				Input::inputs[player.playernum].consumeBinaryToggle("MenuCancel");
				::closeShop(player.playernum);
				return;
			}
			else
			{
				if ( usingGamepad && Input::inputs[player.playernum].binaryToggle("MenuConfirm") )
				{
					purchaseItemAction = true;
					Input::inputs[player.playernum].consumeBinaryToggle("MenuConfirm");
				}
				else if ( !usingGamepad && Input::inputs[player.playernum].binaryToggle("MenuRightClick") )
				{
					purchaseItemAction = true;
					Input::inputs[player.playernum].consumeBinaryToggle("MenuRightClick");
				}
				else if ( usingGamepad && Input::inputs[player.playernum].binaryToggle("MenuPageRightAlt") )
				{
					toggleShopBuybackView(player.playernum);
					Input::inputs[player.playernum].consumeBinaryToggle("MenuPageRightAlt");
				}
			}
		}
	}

	if ( purchaseItemAction && itemPrice >= 0 && player.GUI.activeModule == Player::GUI_t::MODULE_SHOP )
	{
		for ( node_t* node = shopInv[player.playernum]->first; node != NULL; node = node->next )
		{
			Item* item = (Item*)node->element;
			if ( item )
			{
				if ( hideItemFromShopView(*item) )
				{
					continue;
				}
				if ( buybackView && !item->playerSoldItemToShop )
				{
					continue;
				}
				else if ( !buybackView && item->playerSoldItemToShop )
				{
					continue;
				}
				if ( !hideItemFromShopView(*item)
					&& item->x == getSelectedShopX()
					&& item->y == getSelectedShopY()
					&& getSelectedShopX() >= 0 && getSelectedShopX() < MAX_SHOP_X
					&& getSelectedShopY() >= 0 && getSelectedShopX() < MAX_SHOP_Y )
				{
					int oldQty = item->count;
					bool consumedEntireStack = false;
					if ( buyItemFromShop(player.playernum, item, consumedEntireStack) )
					{
						if ( consumedEntireStack )
						{
							animTooltipTicks = 0;
							animTooltip = 0.0;
							clearItemDisplayed();
						}
					}
					break;
				}
			}
		}
	}

	int buybackItems = 0;
	if ( bOpen && shopInv[player.playernum] )
	{
		for ( node_t* node = shopInv[player.playernum]->first; node != NULL; node = node->next )
		{
			Item* item = (Item*)node->element;
			if ( item )
			{
				if ( hideItemFromShopView(*item) )
				{
					continue;
				}
				if ( item->playerSoldItemToShop )
				{
					++buybackItems;
				}
			}
		}
	}
	DebugTimers.addTimePoint("shop", "4");
	auto bgFrame = shopFrame->findFrame("shop base");
	assert(bgFrame);

	auto bgGrid = bgFrame->findImage("shop grid img");
	if ( buybackView )
	{
		bgGrid->path = "images/ui/Shop/Shop_ItemSlots_Buyback_Areas03.png";
	}
	else
	{
		bgGrid->path = "images/ui/Shop/Shop_ItemSlots_Areas03.png";
	}

	auto buyTooltipFrame = bgFrame->findFrame("buy tooltip frame");
	buyTooltipFrame->setDisabled(false);

	auto displayItemName = buyTooltipFrame->findField("item display name");
	auto displayItemValue = buyTooltipFrame->findField("item display value");
	auto itemTooltipImg = buyTooltipFrame->findImage("tooltip img");
	itemTooltipImg->disabled = false;
	auto itemGoldImg = buyTooltipFrame->findImage("gold img");
	auto itemSlotFrame = buyTooltipFrame->findFrame("item slot frame");
	auto buyPromptText = buyTooltipFrame->findField("buy prompt txt");
	auto buyPromptGlyphFrame = buyTooltipFrame->findFrame("buy prompt frame");
	auto buyPromptGlyph = buyPromptGlyphFrame->findImage("buy prompt glyph");
	auto itemBgImg = buyTooltipFrame->findImage("item bg img");
	itemBgImg->pos.y = itemTooltipImg->pos.y + itemTooltipImg->pos.h / 2 - itemBgImg->pos.h / 2 - 1;
	if ( animTooltip <= 0.0001 )
	{
		displayItemName->setDisabled(true);
		displayItemValue->setDisabled(true);
		itemGoldImg->disabled = true;
		itemSlotFrame->setDisabled(true);
		buyPromptText->setDisabled(true);
		buyPromptGlyph->disabled = true;
		buyPromptGlyphFrame->setDisabled(true);
	}

	bool drawGlyphs = !::inputs.getVirtualMouse(player.playernum)->draw_cursor;
	auto closeBtn = bgFrame->findButton("close shop button");
	auto buybackBtn = bgFrame->findButton("buyback button");
	auto buybackPromptTxt = bgFrame->findField("buyback txt");
	auto buybackPromptGlyph = bgFrame->findImage("buyback glyph");
	auto closePromptTxt = bgFrame->findField("close shop prompt");
	auto closePromptGlyph = bgFrame->findImage("close shop glyph");
	DebugTimers.addTimePoint("shop", "5");
	char buybackText[32];
	if ( buybackItems > 0 )
	{
		snprintf(buybackText, sizeof(buybackText), "%s (%d)", language[4120], buybackItems);
	}
	else
	{
		snprintf(buybackText, sizeof(buybackText), "%s", language[4120]);
	}
	if ( !drawGlyphs )
	{
		closeBtn->setInvisible(false);
		buybackBtn->setInvisible(false);
		buybackBtn->setDisabled(!isInteractable);
		if ( buybackView )
		{
			buybackBtn->setText(language[4124]);
		}
		else
		{
			buybackBtn->setText(buybackText);
		}
		closeBtn->setDisabled(!isInteractable);

		closePromptGlyph->disabled = true;
		closePromptTxt->setDisabled(true);
		buybackPromptGlyph->disabled = true;
		buybackPromptTxt->setDisabled(true);
	}
	else
	{
		closeBtn->setInvisible(true);
		buybackBtn->setInvisible(true);
		buybackBtn->setDisabled(true);
		closeBtn->setDisabled(true);

		closePromptTxt->setDisabled(false);
		closePromptTxt->setText(language[4121]);
		buybackPromptTxt->setDisabled(false);
		if ( buybackView )
		{
			buybackPromptTxt->setText(language[4124]);
		}
		else
		{
			buybackPromptTxt->setText(buybackText);
		}

		closePromptGlyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("MenuCancel");
		closePromptGlyph->disabled = true;
		if ( auto imgGet = Image::get(closePromptGlyph->path.c_str()) )
		{
			closePromptGlyph->pos.w = imgGet->getWidth();
			closePromptGlyph->pos.h = imgGet->getHeight();
			closePromptGlyph->disabled = false;
		}
		closePromptGlyph->pos.x = closePromptTxt->getSize().x - 4 - closePromptGlyph->pos.w;
		closePromptGlyph->pos.y = closePromptTxt->getSize().y 
			+ closePromptTxt->getSize().h / 2 - closePromptGlyph->pos.h / 2;

		buybackPromptGlyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("MenuPageRightAlt");
		buybackPromptGlyph->disabled = true;
		if ( auto imgGet = Image::get(buybackPromptGlyph->path.c_str()) )
		{
			buybackPromptGlyph->pos.w = imgGet->getWidth();
			buybackPromptGlyph->pos.h = imgGet->getHeight();
			buybackPromptGlyph->disabled = false;
		}
		buybackPromptGlyph->pos.x = buybackPromptTxt->getSize().x - 4 - buybackPromptGlyph->pos.w;
		buybackPromptGlyph->pos.y = buybackPromptTxt->getSize().y 
			+ buybackPromptTxt->getSize().h / 2 - buybackPromptGlyph->pos.h / 2;
		if ( buybackPromptGlyph->pos.y + buybackPromptGlyph->pos.h >= closePromptGlyph->pos.y )
		{
			// touching below glyph, move this up
			buybackPromptGlyph->pos.y -= 2;
		}
		if ( buybackPromptGlyph->pos.y % 2 == 1 ) // odd numbered pixel, move up
		{
			buybackPromptGlyph->pos.y -= 1;
		}
	}

	DebugTimers.addTimePoint("shop", "6");
	if ( itemPrice >= 0 && itemDesc.size() > 1 )
	{
		buyTooltipFrame->setDisabled(false);
		displayItemName->setDisabled(false);
		displayItemValue->setDisabled(false);
		itemTooltipImg->disabled = false;
		itemGoldImg->disabled = false;
		itemSlotFrame->setDisabled(false);

		if ( isInteractable )
		{
			const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animTooltip)) / 2.0;
			animTooltip += setpointDiffX;
			animTooltip = std::min(1.0, animTooltip);
			animTooltipTicks = ticks;
		}

		SDL_Rect buyTooltipFramePos = buyTooltipFrame->getSize();
		buyTooltipFramePos.x = 4;

		SDL_Rect namePos{ 76, 0, 208, 24 };
		displayItemName->setSize(namePos);
		if ( itemRequiresTitleReflow )
		{
			displayItemName->setText(itemDesc.c_str());
			displayItemName->reflowTextToFit(0);
			if ( displayItemName->getNumTextLines() > 2 )
			{
				// more than 2 lines, append ...
				std::string copiedName = displayItemName->getText();
				auto lastNewline = copiedName.find_last_of('\n');
				copiedName = copiedName.substr(0U, lastNewline);
				copiedName += "...";
				displayItemName->setText(copiedName.c_str());
				displayItemName->reflowTextToFit(0);
				if ( displayItemName->getNumTextLines() > 2 )
				{
					// ... doesn't fit, replace last 3 characters with ...
					copiedName = copiedName.substr(0U, copiedName.size() - 6);
					copiedName += "...";
					displayItemName->setText(copiedName.c_str());
					displayItemName->reflowTextToFit(0);
				}
			}
			itemRequiresTitleReflow = false;
		}
		namePos.h = displayItemName->getNumTextLines() * 24;

		itemBgImg->pos.x = 10;
		if ( displayItemName->getNumTextLines() > 1 )
		{
			itemTooltipImg->path = "images/ui/Shop/Shop_Tooltip_3Row_00.png";
			itemTooltipImg->pos.h = 86;
			buyTooltipFramePos.h = 86;
			itemTooltipImg->pos.y = 0;

			namePos.y = buyTooltipFramePos.h - 77;
		}
		else
		{
			itemTooltipImg->path = "images/ui/Shop/Shop_Tooltip_2Row_00.png";
			itemTooltipImg->pos.h = 66;
			buyTooltipFramePos.h = 66;
			itemTooltipImg->pos.y = 0;

			namePos.y = buyTooltipFramePos.h - 57;
		}
		displayItemName->setSize(namePos);

		itemBgImg->pos.y = itemTooltipImg->pos.y + itemTooltipImg->pos.h / 2 - itemBgImg->pos.h / 2 - 1;
		SDL_Rect slotFramePos = itemSlotFrame->getSize();
		slotFramePos.x = itemBgImg->pos.x + itemBgImg->pos.w / 2 - slotFramePos.w / 2 - 1;
		slotFramePos.y = itemBgImg->pos.y + itemBgImg->pos.h / 2 - slotFramePos.h / 2 - 1;
		itemSlotFrame->setSize(slotFramePos);

		char priceFormat[64];
		snprintf(priceFormat, sizeof(priceFormat), "%d gold", itemPrice);

		SDL_Rect valuePos = namePos;
		valuePos.x = 102;
		valuePos.y = buyTooltipFramePos.h - 31;
		valuePos.h = 24;
		displayItemValue->setSize(valuePos);
		displayItemValue->setText(priceFormat);

		Uint32 negativeColor = makeColor(215, 38, 61, 255);
		displayItemValue->setColor(makeColor(201, 162, 100, 255));
		if ( itemPrice > 0 && itemPrice > stats[player.playernum]->GOLD )
		{
			if ( !strcmp(buyPromptText->getText(), language[4113]) ) // buy prompt
			{
				displayItemValue->setColor(negativeColor);
			}
		}

		itemGoldImg->pos.x = 76;
		itemGoldImg->pos.y = buyTooltipFramePos.h - 36;

		buyTooltipFramePos.y = bgFrame->getSize().h - (buyTooltipFramePos.h + 4) * animTooltip;
		buyTooltipFrame->setSize(buyTooltipFramePos);

		buyPromptText->setSize(SDL_Rect{ buyTooltipFramePos.w - 60, valuePos.y, 52, 24 });
		buyPromptText->setDisabled(false);

		if ( inputs.hasController(player.playernum) && !inputs.getVirtualMouse(player.playernum)->draw_cursor )
		{
			buyPromptGlyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("MenuConfirm");
		}
		else
		{
			buyPromptGlyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("MenuRightClick");
		}
		buyPromptGlyph->disabled = true;
		if ( auto imgGet = Image::get(buyPromptGlyph->path.c_str()) )
		{
			buyPromptGlyph->pos.w = imgGet->getWidth();
			buyPromptGlyph->pos.h = imgGet->getHeight();
			buyPromptGlyph->disabled = false;
		}
		buyPromptGlyphFrame->setDisabled(buyPromptGlyph->disabled);
		buyPromptGlyph->pos.x = buyPromptText->getSize().x - 8 - buyPromptGlyph->pos.w;
		buyPromptGlyph->pos.y = buyPromptText->getSize().y + buyPromptText->getSize().h / 2 - buyPromptGlyph->pos.h / 2;
		if ( buyPromptGlyph->pos.y % 2 == 1 )
		{
			buyPromptGlyph->pos.y -= 1;
		}
		buyPromptGlyphFrame->setSize(buyPromptGlyph->pos);
		buyPromptGlyph->pos.x = 0;
		buyPromptGlyph->pos.y = 0;

		/*SDL_Rect buyPromptGlyphFramePos = buyPromptGlyphFrame->getSize();
		int maxPromptHeight = buyTooltipFramePos.h - 6; // if we want to prevent glyph spilling over too much on border
		if ( buyPromptGlyphFramePos.y + buyPromptGlyphFramePos.h > (maxPromptHeight) )
		{
			buyPromptGlyphFramePos.h -= (buyPromptGlyphFramePos.y + buyPromptGlyphFramePos.h) - (maxPromptHeight);
			buyPromptGlyphFrame->setSize(buyPromptGlyphFramePos);
		}*/
	}
	else
	{
		if ( ticks - animTooltipTicks > TICKS_PER_SECOND / 3
			|| animTooltip < 0.9999 )
		{
			const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.01, (animTooltip)) / 2.0;
			animTooltip -= setpointDiffX;
			animTooltip = std::max(0.0, animTooltip);
		}

		SDL_Rect buyTooltipFramePos = buyTooltipFrame->getSize();
		buyTooltipFramePos.y = bgFrame->getSize().h - (buyTooltipFramePos.h + 4) * animTooltip;
		buyTooltipFrame->setSize(buyTooltipFramePos);
	}
	DebugTimers.addTimePoint("shop", "7");
	auto orbImg = buyTooltipFrame->findImage("orb img");
	orbImg->pos.y = buyTooltipFrame->getSize().h - 28;

	// discount values
	auto discountFrame = bgFrame->findFrame("discount frame");
	auto discountLabelText = bgFrame->findField("discount label");
	auto discountValue = discountFrame->findField("discount");
	real_t shopModifier = 0.0;

	discountFrame->setDisabled(false);
	discountLabelText->setDisabled(false);
	//discountValue->setDisabled(true);
	if ( animTooltip > 0.0001 )
	{
		discountValue->setDisabled(false);
	}

	if ( (itemPrice >= 0 && itemDesc.size() > 1) || !strcmp(discountValue->getText(), "") )
	{
		if ( player.GUI.activeModule == Player::GUI_t::MODULE_SHOP ) // buy prompt
		{
			discountLabelText->setText(language[4122]);
			shopModifier = 1 / ((50 + stats[player.playernum]->PROFICIENCIES[PRO_TRADING]) / 150.f); // buy value
			shopModifier /= 1.f + statGetCHR(stats[player.playernum], players[player.playernum]->entity) / 20.f;
			shopModifier = std::max(1.0, shopModifier);
			shopModifier = shopModifier * 100.0 - 100.0;
			char buf[32];
			snprintf(buf, sizeof(buf), "%+.f%%", shopModifier);
			discountValue->setText(buf);
		}
		else
		{
			discountLabelText->setText(language[4123]);
			shopModifier = (50 + stats[player.playernum]->PROFICIENCIES[PRO_TRADING]) / 150.f; // sell value
			shopModifier *= 1.f + statGetCHR(stats[player.playernum], players[player.playernum]->entity) / 20.f;
			shopModifier = std::min(1.0, shopModifier);
			shopModifier = shopModifier * 100.0 - 100.0;
			char buf[32];
			if ( shopModifier < 0.0 )
			{
				snprintf(buf, sizeof(buf), "%+.f%%", shopModifier);
			}
			else
			{
				snprintf(buf, sizeof(buf), "%.f%%", shopModifier);
			}
			discountValue->setText(buf);
		}
	}
	DebugTimers.addTimePoint("shop", "8");
	updatePlayerGold(player.playernum);
	DebugTimers.addTimePoint("shop", "9");
	updateShopGUIChatter(player.playernum);
	DebugTimers.addTimePoint("shop", "10");
}

const bool Player::ShopGUI_t::isItemFromShop(Item* item) const
{
	if ( !item || !bOpen )
	{
		return false;
	}

	Entity* shopkeeperEntity = uidToEntity(shopkeeper[player.playernum]);
	if ( !shopkeeperEntity )
	{
		return false;
	}

	if ( item->node && item->node->list && shopInv[player.playernum] && item->node->list == shopInv[player.playernum] )
	{
		return true;
	}
	return false;
}

const bool Player::ShopGUI_t::isItemSelectedFromShop(Item* item) const
{
	if ( !item || !bOpen )
	{
		return false;
	}

	Entity* shopkeeperEntity = uidToEntity(shopkeeper[player.playernum]);
	if ( !shopkeeperEntity )
	{
		return false;
	}

	if ( item->type == SPELL_ITEM )
	{
		return false;
	}

	if ( item->x == getSelectedShopX()
		&& item->y == getSelectedShopY()
		&& isInteractable
		&& isItemFromShop(item) )
	{
		return true;
	}
	return false;
}

const bool Player::ShopGUI_t::isItemSelectedToSellToShop(Item* item) const
{
	if ( !item || !bOpen )
	{
		return false;
	}

	Entity* shopkeeperEntity = uidToEntity(shopkeeper[player.playernum]);
	if ( !shopkeeperEntity )
	{
		return false;
	}

	if ( item->type == SPELL_ITEM )
	{
		return false;
	}

	if ( !isItemFromShop(item) && player.gui_mode == GUI_MODE_SHOP )
	{
		return true;
	}
	return false;
}