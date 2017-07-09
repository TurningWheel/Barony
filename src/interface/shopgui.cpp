/*-------------------------------------------------------------------------------

	BARONY
	File: shopgui.cpp
	Desc: contains shop (GUI) related functions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../items.hpp"
#include "../shops.hpp"
#include "../player.hpp"
#include "interface.hpp"

void rebuildShopInventory()
{
	//Count number of items.
	int c = 0;
	node_t* node = nullptr;
	for ( node = shopInv->first; node != NULL; node = node->next )
	{
		Item* item = (Item*) node->element;
		if ( shopinventorycategory == 0 && itemCategory(item) != WEAPON && itemCategory(item) != THROWN )
		{
			continue;
		}
		else if ( shopinventorycategory == 1 && itemCategory(item) != ARMOR )
		{
			continue;
		}
		else if ( shopinventorycategory == 2 && itemCategory(item) != AMULET && itemCategory(item) != RING )
		{
			continue;
		}
		else if ( shopinventorycategory == 3 && itemCategory(item) != SPELLBOOK && itemCategory(item) != MAGICSTAFF && itemCategory(item) != SCROLL )
		{
			continue;
		}
		else if ( shopinventorycategory == 4 && itemCategory(item) != GEM )
		{
			continue;
		}
		else if ( shopinventorycategory == 5 && itemCategory(item) != FOOD && itemCategory(item) != POTION )
		{
			continue;
		}
		else if ( shopinventorycategory == 6 && itemCategory(item) != TOOL && itemCategory(item) != BOOK )
		{
			continue;
		}
		c++;
	}
	//Sanitize item scroll.
	shopitemscroll = std::max(0, std::min(shopitemscroll, c - 4));
	//Clear out currently displayed items.
	for ( c = 0; c < 4; c++ )
	{
		shopinvitems[c] = NULL;
	}
	//Display the items.
	c = 0;
	for ( node = shopInv->first; node != NULL; node = node->next )
	{
		Item* item = (Item*) node->element;
		if (item)
		{
			if ( shopinventorycategory == 0 && itemCategory(item) != WEAPON && itemCategory(item) != THROWN )
			{
				continue;
			}
			else if ( shopinventorycategory == 1 && itemCategory(item) != ARMOR )
			{
				continue;
			}
			else if ( shopinventorycategory == 2 && itemCategory(item) != AMULET && itemCategory(item) != RING )
			{
				continue;
			}
			else if ( shopinventorycategory == 3 && itemCategory(item) != SPELLBOOK && itemCategory(item) != MAGICSTAFF && itemCategory(item) != SCROLL )
			{
				continue;
			}
			else if ( shopinventorycategory == 4 && itemCategory(item) != GEM )
			{
				continue;
			}
			else if ( shopinventorycategory == 5 && itemCategory(item) != FOOD && itemCategory(item) != POTION )
			{
				continue;
			}
			else if ( shopinventorycategory == 6 && itemCategory(item) != TOOL && itemCategory(item) != BOOK )
			{
				continue;
			}
			c++;
			if ( c <= shopitemscroll )
			{
				continue;
			}
			shopinvitems[c - shopitemscroll - 1] = item;
			if ( c > 3 + shopitemscroll )
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

inline void checkBuyItem()
{
	if ( (stats[clientnum]->HP <= 0) || (players[clientnum] == nullptr) || (players[clientnum]->entity == nullptr) )
	{
		return;
	}

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
		selectedShopSlot = -1;
		return;
	}

	bool selectingSlot = false;

	for (int i = 0; i < NUM_SHOP_GUI_SLOTS; ++i, slotPos.y += slotPos.h)
	{
		if ( omousey >= slotPos.y && omousey < slotPos.y + slotPos.h )
		{
			//If moused over, highlight slot & check for mouse click or gamepad buy button.
			selectedShopSlot = i;
			selectingSlot = true;
			drawImage(inventoryoption_bmp, nullptr, &slotPos);
			if ( mousestatus[SDL_BUTTON_LEFT] || *inputPressed(joyimpulses[INJOY_MENU_USE]) )
			{
				mousestatus[SDL_BUTTON_LEFT] = 0;
				*inputPressed(joyimpulses[INJOY_MENU_USE]) = 0;
				buyItemFromShop(shopinvitems[i]);
				//Check if no more items after this slot & deal accordingly.
				rebuildShopInventory();
				if ( shopinvitems[i] == nullptr )
				{
					if ( shopinvitems[0] == nullptr )
					{
						//Go back to inventory.
						selectedShopSlot = -1;
						warpMouseToSelectedInventorySlot();
					}
					else
					{
						//Move up one slot.
						--selectedShopSlot;
						warpMouseToSelectedShopSlot();
					}
				}
			}
		}
	}

	if ( !selectingSlot )
	{
		selectedShopSlot = -1;
	}
}

void warpMouseToSelectedShopSlot()
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
	slotPos.y = slotAreaBounds.y1 + (inventoryoption_bmp->h * selectedShopSlot);

	SDL_WarpMouseInWindow(screen, slotPos.x + (slotPos.w / 2), slotPos.y + (slotPos.h / 2));
}

/*-------------------------------------------------------------------------------

	updateShopWindow

	Draws and processes everything related to the shop window

-------------------------------------------------------------------------------*/

void updateShopWindow()
{
	SDL_Rect pos;
	node_t* node;
	int c;

	if ( multiplayer != CLIENT )
	{
		Entity* entity = uidToEntity(shopkeeper);
		if (entity)
		{
			Stat* stats = entity->getStats();
			shopkeepername = stats->name;
		}
	}

	// draw window
	int x1 = xres / 2 - SHOPWINDOW_SIZEX / 2, x2 = xres / 2 + SHOPWINDOW_SIZEX / 2;
	int y1 = yres / 2 - SHOPWINDOW_SIZEY / 2, y2 = yres / 2 + SHOPWINDOW_SIZEY / 2;
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
				shopinventorycategory = (omousex - x - 12) / button_bmp->w;
			}
		}
		else if ( omousey >= y + 16 && omousey < y + 52 )
		{
			if ( omousex >= x + inventory_bmp->w - 28 && omousex < x + inventory_bmp->w - 12 )
			{
				mousestatus[SDL_BUTTON_LEFT] = 0;
				buttonclick = 10;
				shopitemscroll--;
			}
		}
		else if ( omousey >= y + 52 && omousey < y + 88 )
		{
			if ( omousex >= x + inventory_bmp->w - 28 && omousex < x + inventory_bmp->w - 12 )
			{
				mousestatus[SDL_BUTTON_LEFT] = 0;
				buttonclick = 11;
				shopitemscroll++;
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
				shopitemscroll++;
			}
			else if ( mousestatus[SDL_BUTTON_WHEELUP] )
			{
				mousestatus[SDL_BUTTON_WHEELUP] = 0;
				shopitemscroll--;
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
	pos.x = x + 12 + button_bmp->w * shopinventorycategory;
	pos.y = y;
	pos.w = 0;
	pos.h = 0;
	if ( shopinventorycategory <= 6 )
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
	checkBuyItem();


	rebuildShopInventory();

	int y3 = y + 22;
	c = 0;
	for ( node = shopInv->first; node != NULL; node = node->next )
	{
		Item* item = (Item*) node->element;
		if (item)
		{
			if ( shopinventorycategory == 0 && itemCategory(item) != WEAPON && itemCategory(item) != THROWN )
			{
				continue;
			}
			else if ( shopinventorycategory == 1 && itemCategory(item) != ARMOR )
			{
				continue;
			}
			else if ( shopinventorycategory == 2 && itemCategory(item) != AMULET && itemCategory(item) != RING )
			{
				continue;
			}
			else if ( shopinventorycategory == 3 && itemCategory(item) != SPELLBOOK && itemCategory(item) != MAGICSTAFF && itemCategory(item) != SCROLL )
			{
				continue;
			}
			else if ( shopinventorycategory == 4 && itemCategory(item) != GEM )
			{
				continue;
			}
			else if ( shopinventorycategory == 5 && itemCategory(item) != FOOD && itemCategory(item) != POTION )
			{
				continue;
			}
			else if ( shopinventorycategory == 6 && itemCategory(item) != TOOL && itemCategory(item) != BOOK )
			{
				continue;
			}
			c++;
			if ( c <= shopitemscroll )
			{
				continue;
			}
			char tempstr[64] = { 0 };
			strncpy(tempstr, item->description(), 42);
			if ( strlen(tempstr) == 42 )
			{
				strcat(tempstr, " ...");
			}
			ttfPrintText(ttf8, x + 12 + 36, y3, tempstr);
			ttfPrintTextFormatted(ttf8, x + 12 + 348, y3, "%7dG", item->buyValue(clientnum));
			pos.x = x + 12 + 16;
			pos.y = y + 17 + 18 * (c - shopitemscroll - 1);
			pos.w = 16;
			pos.h = 16;
			drawImageScaled(itemSprite(item), NULL, &pos);
			y3 += 18;
			if ( c > 3 + shopitemscroll )
			{
				break;
			}
		}
	}

	// draw money count
	ttfPrintTextFormatted( ttf16, x1 + 16, y2 - 32, language[357], stats[clientnum]->GOLD );

	// chitchat
	if ( (ticks - shoptimer) % 600 == 0 )
	{
		shopspeech = language[216 + rand() % NUMCHITCHAT];
		shoptimer--;
	}

	// draw speech
	if (sellitem)
	{
		ttfPrintTextFormatted( ttf16, x1 + 16 + 160 + 16, y1 + 32, shopspeech, sellitem->sellValue(clientnum) );
	}
	else
	{
		ttfPrintTextFormatted( ttf16, x1 + 16 + 160 + 16, y1 + 32, shopspeech, 0 );
	}
	if ( !strcmp(shopspeech, language[194]) || !strcmp(shopspeech, language[195]) || !strcmp(shopspeech, language[196]) )
	{
		ttfPrintTextFormatted( ttf16, x1 + 16 + 160 + 16, y1 + 64, language[358], shopkeepername, language[184 + shopkeepertype] );
	}

	// draw black box for shopkeeper
	pos.x = x1 + 16;
	pos.y = y1 + 16;
	pos.w = 160;
	pos.h = 160;
	drawRect(&pos, 0, 255);

	// draw shopkeeper
	if ( uidToEntity(shopkeeper) )
	{
		Entity* entity = uidToEntity(shopkeeper);
		if ( !entity->flags[INVISIBLE] ) //TODO: isInvisible()?
		{
			pos.x = x1 + 16;
			pos.y = y1 + 16;
			pos.w = 160;
			pos.h = 160;
			drawImage(shopkeeper_bmp, NULL, &pos);
		}
		else
		{
			pos.x = x1 + 16;
			pos.y = y1 + 16;
			pos.w = 160;
			pos.h = 160;
			drawRect(&pos, 0, 255);
		}
	}
}

/*void warpMouseToselectedShopSlot() {
	//Shop window boundaries.
	int x1 = xres/2 - SHOPWINDOW_SIZEX/2, x2 = xres/2 + SHOPWINDOW_SIZEX/2;
	int y1 = yres/2 - SHOPWINDOW_SIZEY/2, y2 = yres/2 + SHOPWINDOW_SIZEY/2;

	//Calculate x that will be halfway in slot.
	int x = x1 + (x2 - x1)/2 - inventory_bmp->w/2;
	int y = y1 + 16 + 160 + (inventoryoption_bmp->h * selectedShopSlot) + (inventoryoption_bmp->h / 2);
	SDL_WarpMouseInWindow(screen, x, y);
}*/

inline Item* getItemInfoFromShop(int slot)
{
	if ( slot >= 4 )
	{
		return nullptr; //Out of bounds,
	}

	return shopinvitems[slot];
}

void selectShopSlot(int slot)
{
	if ( slot < selectedShopSlot )
	{
		//Moving up.

		/*
		 * Possible cases:
		 * * 1) Move cursor up the GUI through different selectedShopSlot.
		 * * 2) Page up through shopitemscroll--
		 * * 3) Scrolling up past top of shop, no shopitemscroll (move back to inventory)
		 */

		if ( selectedShopSlot <= 0 )
		{
			//Covers cases 2 & 3.

			/*
			 * Possible cases:
			 * * A) Hit very top of shop inventory, can't go any further. Return to inventory.
			 * * B) Page up, scrolling through shopitemscroll.
			 */

			if ( shopitemscroll <= 0 )
			{
				//Case 3/A: Return to inventory.
				selectedShopSlot = -1;
			}
			else
			{
				//Case 2/B: Page up through shop inventory.
				--shopitemscroll;
			}
		}
		else
		{
			//Covers case 1.

			//Move cursor up the GUI through different selectedShopSlot (--selectedShopSlot).
			--selectedShopSlot;
			warpMouseToSelectedShopSlot();
		}
	}
	else if ( slot > selectedShopSlot )
	{
		//Moving down.

		/*
		 * Possible cases:
		 * * 1) Moving cursor down through GUI through different selectedShopSlot.
		 * * 2) Scrolling down past bottom of shop through shopitemscroll++
		 * * 3) Scrolling down past bottom of shop, max shop scroll (revoke move -- can't go beyond limit of shop).
		 */

		if ( selectedShopSlot >= NUM_SHOP_GUI_SLOTS - 1 )
		{
			//Covers cases 2 & 3.
			++shopitemscroll; //Shopitemscroll is automatically sanitized in updateShopWindow().
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

			Item* item = getItemInfoFromShop(selectedShopSlot + 1);

			if ( item )
			{
				++selectedShopSlot;
				warpMouseToSelectedShopSlot();
			}
			else
			{
				//No more items. Stop.
			}
		}
	}
}

void cycleShopCategories(int direction)
{
	if ( direction < 0 )
	{
		//Cycle left.
		--shopinventorycategory;
	}
	else if ( direction > 0 )
	{
		//Cycle right.
		++shopinventorycategory;
	}

	if ( shopinventorycategory < 0 )
	{
		shopinventorycategory = NUM_SHOP_CATEGORIES - 1;
	}
	else if ( shopinventorycategory >= NUM_SHOP_CATEGORIES )
	{
		shopinventorycategory = 0;
	}
}
