/*-------------------------------------------------------------------------------

	BARONY
	File: updatechestinventory.cpp
	Desc: contains updateChestInventory()

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../draw.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../items.hpp"
#include "../sound.hpp"
#include "../net.hpp"
#include "../player.hpp"
#include "interface.hpp"

Entity* openedChest[MAXPLAYERS] = { nullptr };

void repopulateInvItems(const int player, list_t* chestInventory)
{
	int c = 0;

	//Step 1: Clear.
	for ( c = 0; c < kNumChestItemsToDisplay; ++c )
	{
		invitemschest[player][c] = nullptr;
	}

	node_t* node = nullptr;
	Item* item = nullptr;

	c = 0;

	//Step 2: Loop through inventory till reach part visible in chest GUI and add those items.
	for ( node = chestInventory->first; node != nullptr; node = node->next )
	{
		if ( node->element )
		{
			item = (Item*) node->element;
			if ( item )
			{
				++c;
				if ( c <= chestitemscroll[player] )
				{
					continue;
				}
				invitemschest[player][c - chestitemscroll[player] - 1] = item;
				if ( c > (kNumChestItemsToDisplay - 1) + chestitemscroll[player] )
				{
					break;
				}
			}
		}
	}
}

int numItemsInChest(const int player)
{
	node_t* node = nullptr;

	list_t* chestInventory = nullptr;
	if ( multiplayer == CLIENT )
	{
		chestInventory = &chestInv[player];
	}
	else if (openedChest[player]->children.first && openedChest[player]->children.first->element)
	{
		chestInventory = (list_t*)openedChest[player]->children.first->element;
	}

	int i = 0;

	if ( chestInventory )
	{
		for (node = chestInventory->first; node != nullptr; node = node->next)
		{
			++i;
		}
	}

	return i;
}

const int getChestGUIStartX(const int player)
{
	return ((players[player]->camera_midx() - (inventoryChest_bmp->w / 2)) + chestgui_offset_x[player]);
}
const int getChestGUIStartY(const int player)
{
	return ((players[player]->camera_midy() - (inventoryChest_bmp->h / 2)) + chestgui_offset_y[player]);
}

void warpMouseToSelectedChestSlot(const int player)
{
	int x = getChestGUIStartX(player) + (inventoryoptionChest_bmp->w / 2);
	int y = getChestGUIStartY(player) + 16 + (inventoryoptionChest_bmp->h * selectedChestSlot[player]) + (inventoryoptionChest_bmp->h / 2);


	//SDL_WarpMouseInWindow(screen, x, y);
	Uint32 flags = (Inputs::SET_MOUSE | Inputs::SET_CONTROLLER);
	inputs.warpMouse(player, x, y, flags);
}

/*-------------------------------------------------------------------------------

	updateChestInventory

	Processes and draws everything related to chest inventory

-------------------------------------------------------------------------------*/

inline void drawChestSlots(const int player)
{
	if ( !openedChest[player] )
	{
		return;
	}

	SDL_Rect pos;
	Item* item = nullptr;

	const Sint32 mousex = inputs.getMouse(player, Inputs::X);
	const Sint32 mousey = inputs.getMouse(player, Inputs::Y);
	const Sint32 omousex = inputs.getMouse(player, Inputs::OX);
	const Sint32 omousey = inputs.getMouse(player, Inputs::OY);

	int highlightingSlot = -1;

	if (omousex >= getChestGUIStartX(player) && omousex < getChestGUIStartX(player) + (inventoryChest_bmp->w - 28))
	{
		pos.x = getChestGUIStartX(player) + 12;
		pos.w = 0;
		pos.h = 0;

		int currentY = getChestGUIStartY(player) + 16;
		for ( int i = 0; i < kNumChestItemsToDisplay; ++i, currentY += inventoryoptionChest_bmp->h )
		{
			if ( omousey >= currentY && omousey < currentY + inventoryoptionChest_bmp->h )
			{
				pos.y = currentY;
				item = openedChest[player]->getItemFromChest(invitemschest[player][i], false, true);
				if ( item != nullptr )
				{
					free(item); //Only YOU can prevent memleaks!
					drawImage(inventoryoptionChest_bmp, nullptr, &pos); //Highlight the moused-over slot.
					highlightingSlot = i;

					bool grabbedItem = false;

					if ( (inputs.bMouseLeft(player) || inputs.bControllerInputPressed(player, INJOY_MENU_USE)) )
					{
						inputs.mouseClearLeft(player);
						inputs.controllerClearInput(player, INJOY_MENU_USE);
						item = openedChest[player]->getItemFromChest(invitemschest[player][i], false);
						messagePlayer(player, language[374], item->description());
						itemPickup(player, item);
						playSound(35 + rand() % 3, 64);
						grabbedItem = true;
					}
					else if ( inputs.bMouseRight(player) )
					{
						inputs.mouseClearRight(player);
						item = openedChest[player]->getItemFromChest(invitemschest[player][i], true);
						messagePlayer(player, language[374], item->description());
						itemPickup(player, item); //Grab all of that item from the chest.
						playSound(35 + rand() % 3, 64);
						grabbedItem = true;
					}

					if ( grabbedItem )
					{
						list_t* chestInventory = nullptr;
						if ( multiplayer == CLIENT )
						{
							chestInventory = &chestInv[player];
						}
						else if ( openedChest[player]->children.first && openedChest[player]->children.first->element )
						{
							chestInventory = (list_t*)openedChest[player]->children.first->element;
						}
						repopulateInvItems(player, chestInventory); //Have to regenerate, otherwise the following if check will often end up evaluating to false. //Doesn't work. #blamedennis

						item = openedChest[player]->getItemFromChest(invitemschest[player][i], false, true);
						if ( item )
						{
							free(item);
						}
						else
						{
							//Move cursor if this slot is now empty.
							--highlightingSlot;
							selectedChestSlot[player] = highlightingSlot;
							if ( selectedChestSlot[player] >= 0 )
							{
								warpMouseToSelectedChestSlot(player);
							}
							else
							{
								warpMouseToSelectedInventorySlot(player);
							}
						}
					}
				}
			}
		}
	}

	if ( highlightingSlot >= 0 )
	{
		selectedChestSlot[player] = highlightingSlot;
	}
	else
	{
		selectedChestSlot[player] = -1;
	}
}

void updateChestInventory(const int player)
{
	if ( !openedChest[player] )
	{
		for ( int c = 0; c < kNumChestItemsToDisplay; ++c )
		{
			invitemschest[player][c] = nullptr;
		}
		return;
	}

	SDL_Rect pos;
	node_t* node, *nextnode;
	int y, c;
	int chest_buttonclick = 0;
	Item* item;

	//Chest inventory GUI.

	//Center the chest GUI.
	//pos.x = ((xres - winx) / 2) - (inventory_bmp->w / 2);
	pos.x = getChestGUIStartX(player); //(winx + ((winw / 2) - (inventoryChest_bmp->w / 2)))
	//pos.x = character_bmp->w;
	//pos.y = ((yres - winy) / 2) - (inventory_bmp->h / 2);
	pos.y = getChestGUIStartY(player); //(winy + ((winh - winy) - (inventoryChest_bmp->h / 2)));
	drawImage(inventoryChest_bmp, NULL, &pos);

	const Sint32 omousex = inputs.getMouse(player, Inputs::MouseInputs::OX);
	const Sint32 omousey = inputs.getMouse(player, Inputs::MouseInputs::OY);

	// buttons
	if ( inputs.bMouseLeft(player) && !inputs.getUIInteraction(player)->selectedItem )
	{
		if (openedChest[player])
		{
			//Chest inventory scroll up button.
			if (omousey >= getChestGUIStartX(player) + 16 && omousey < getChestGUIStartY(player) + 52)
			{
				if (omousex >= getChestGUIStartX(player) + (inventoryChest_bmp->w - 28) && omousex < getChestGUIStartX(player) + (inventoryChest_bmp->w - 12))
				{
					chest_buttonclick = 7;
					chestitemscroll[player]--;
					inputs.mouseClearLeft(player);
				}
			}
			//Chest inventory scroll down button.
			else if (omousey >= getChestGUIStartY(player) + 52 && omousey < getChestGUIStartY(player) + 88)
			{
				if (omousex >= getChestGUIStartX(player) + (inventoryChest_bmp->w - 28) && omousex < getChestGUIStartX(player) + (inventoryChest_bmp->w - 12))
				{
					chest_buttonclick = 8;
					chestitemscroll[player]++;
					inputs.mouseClearLeft(player);
				}
			}
			else if (omousey >= getChestGUIStartY(player) && omousey < getChestGUIStartY(player) + 15)
			{
				//Chest inventory close button.
				if (omousex >= getChestGUIStartX(player) + 393 && omousex < getChestGUIStartX(player) + 407)
				{
					chest_buttonclick = 9;
					inputs.mouseClearLeft(player);
				}
				//Chest inventory grab all button.
				if (omousex >= getChestGUIStartX(player) + 376 && omousex < getChestGUIStartX(player) + 391)
				{
					chest_buttonclick = 10;
					inputs.mouseClearLeft(player);
				}
				// 20/12/20 - disabling this for now. unnecessary
				if ( false )
				{
					if (omousex >= getChestGUIStartX(player) && omousex < getChestGUIStartX(player) + 377 && omousey >= getChestGUIStartY(player) && omousey < getChestGUIStartY(player) + 15)
					{
						gui_clickdrag[player] = true;
						dragging_chestGUI[player] = true;
						dragoffset_x[player] = omousex - getChestGUIStartX(player);
						dragoffset_y[player] = omousey - getChestGUIStartY(player);
						inputs.mouseClearLeft(player);
					}
				}
			}
		}
	}

	if ( inputs.bControllerInputPressed(player, INJOY_MENU_CHEST_GRAB_ALL) )   //Gamepad "Y" button grabs all items in chest.
	{
		inputs.controllerClearInput(player, INJOY_MENU_CHEST_GRAB_ALL);
		chest_buttonclick = 10;
	}

	// mousewheel
	if ( omousex >= getChestGUIStartX(player) + 12 && omousex < getChestGUIStartX(player) + (inventoryChest_bmp->w - 28) )
	{
		if ( omousey >= getChestGUIStartY(player) + 16 && omousey < getChestGUIStartY(player) + (inventoryChest_bmp->h - 8) )
		{
			if ( mousestatus[SDL_BUTTON_WHEELDOWN] )
			{
				mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
				chestitemscroll[player]++;
			}
			else if ( mousestatus[SDL_BUTTON_WHEELUP] )
			{
				mousestatus[SDL_BUTTON_WHEELUP] = 0;
				chestitemscroll[player]--;
			}
		}
	}

	if ( dragging_chestGUI )
	{
		if (gui_clickdrag[player] )
		{
			chestgui_offset_x[player] = (omousex - dragoffset_x[player]) - (getChestGUIStartX(player) - chestgui_offset_x[player]);
			chestgui_offset_y[player] = (omousey - dragoffset_y[player]) - (getChestGUIStartY(player) - chestgui_offset_y[player]);
			if ( getChestGUIStartX(player) <= 0)
			{
				chestgui_offset_x[player] = 0 - (getChestGUIStartX(player) - chestgui_offset_x[player]);
			}
			if ( getChestGUIStartX(player) > 0 + xres - inventoryChest_bmp->w)
			{
				chestgui_offset_x[player] = (0 + xres - inventoryChest_bmp->w) - (getChestGUIStartX(player) - chestgui_offset_x[player]);
			}
			if ( getChestGUIStartY(player) <= 0)
			{
				chestgui_offset_y[player] = 0 - (getChestGUIStartY(player) - chestgui_offset_y[player]);
			}
			if ( getChestGUIStartY(player) > 0 + yres - inventoryChest_bmp->h)
			{
				chestgui_offset_y[player] = (0 + yres - inventoryChest_bmp->h) - (getChestGUIStartY(player) - chestgui_offset_y[player]);
			}
		}
		else
		{
			dragging_chestGUI[player] = false;
		}
	}

	list_t* chest_inventory = NULL;
	if ( multiplayer == CLIENT )
	{
		chest_inventory = &chestInv[player];
	}
	else if (openedChest[player]->children.first && openedChest[player]->children.first->element)
	{
		chest_inventory = (list_t*)openedChest[player]->children.first->element;
	}

	if (!chest_inventory)
	{
		messagePlayer(0, "Warning: openedChest[%d] has no inventory. This should not happen.", player);
	}
	else
	{
		//Print the window label signifying this as the chest inventory GUI.
		ttfPrintText(ttf8, (getChestGUIStartX(player) + 2 + ((inventoryChest_bmp->w / 2) - ((TTF8_WIDTH * 15) / 2))), getChestGUIStartY(player) + 4, language[373]);

		//Chest inventory up button.
		if (chest_buttonclick == 7)
		{
			pos.x = getChestGUIStartX(player) + (inventoryChest_bmp->w - 28);
			pos.y = getChestGUIStartY(player) + 16;
			pos.w = 0;
			pos.h = 0;
			drawImage(invup_bmp, NULL, &pos);
		}
		//Chest inventory down button.
		if (chest_buttonclick == 8)
		{
			pos.x = getChestGUIStartX(player) + (inventoryChest_bmp->w - 28);
			pos.y = getChestGUIStartY(player) + 52;
			pos.w = 0;
			pos.h = 0;
			drawImage(invdown_bmp, NULL, &pos);
		}
		//Chest inventory close button.
		if (chest_buttonclick == 9)
		{
			pos.x = getChestGUIStartX(player) + 393;
			pos.y = getChestGUIStartY(player);
			pos.w = 0;
			pos.h = 0;
			drawImage(invclose_bmp, NULL, &pos);
			if (openedChest[player])
			{
				openedChest[player]->closeChest();
				return; //Crashes otherwise, because the rest of this function runs without a chest...
			}
		}
		//Chest inventory grab all items button.
		if (chest_buttonclick == 10)
		{
			pos.x = getChestGUIStartX(player) + 376;
			pos.y = getChestGUIStartY(player);
			pos.w = 0;
			pos.h = 0;
			drawImage(invgraball_bmp, NULL, &pos);
			for (node = chest_inventory->first; node != NULL; node = nextnode)
			{
				nextnode = node->next;
				if (node->element && openedChest[player])
				{
					item = openedChest[player]->getItemFromChest(static_cast<Item* >(node->element), true);
					if ( item != NULL )
					{
						messagePlayer(player, language[374], item->description());
						itemPickup(player, item);
						playSound(35 + rand() % 3, 64);
					}
				}
			}
			repopulateInvItems(player, chest_inventory); // otherwise drawChestSlots will try draw corrupted data
		}

		drawChestSlots(player);

		//Okay, now prepare to render all the items.
		y = getChestGUIStartY(player) + 22;
		c = 0;
		if (chest_inventory)
		{
			c = numItemsInChest(player);
			chestitemscroll[player] = std::max(0, std::min(chestitemscroll[player], c - 4));

			repopulateInvItems(player, chest_inventory);

			c = 0;

			//Actually render the items.
			for (node = chest_inventory->first; node != NULL; node = node->next)
			{
				if (node->element)
				{
					item = (Item*) node->element;
					if (item)
					{
						c++;
						if ( c <= chestitemscroll[player] )
						{
							continue;
						}
						char tempstr[64] = { 0 };
						strncpy(tempstr, item->description(), 46);
						if ( strlen(tempstr) == 46 )
						{
							strcat(tempstr, " ...");
						}
						ttfPrintText(ttf8, getChestGUIStartX(player) + 36, y, tempstr);
						pos.x = getChestGUIStartX(player) + 16;
						pos.y = getChestGUIStartY(player) + 17 + 18 * (c - chestitemscroll[player] - 1);
						pos.w = 16;
						pos.h = 16;
						drawImageScaled(itemSprite(item), NULL, &pos);
						y += 18;
						if (c > (kNumChestItemsToDisplay - 1) + chestitemscroll[player])
						{
							break;
						}
					}
				}
			}
		}
	}
}

void selectChestSlot(const int player, const int slot)
{
	//TODO?: Grab amount (difference between slot and selectedChestSlot)?

	if ( slot < selectedChestSlot[player] )
	{
		//Moving up.

		/*
		 * Possible cases:
		 * * 1) Move cursor up the GUI through different selectedChestSlot.
		 * * 2) Page up through chestitemscroll--
		 * * 3) Scrolling up past top of chest, no chestitemscroll (move back to inventory)
		 */

		if ( selectedChestSlot[player] <= 0 )
		{
			//Covers cases 2 & 3.

			/*
			 * Possible cases:
			 * * A) Hit very top of chest inventory, can't go any further. Return to inventory.
			 * * B) Page up, scrolling through chestitemscroll.
			 */

			if ( chestitemscroll[player] <= 0 )
			{
				//Case 3/A: Return to inventory.
				selectedChestSlot[player] = -1;
			}
			else
			{
				//Case 2/B: Page up through chest inventory.
				--chestitemscroll[player];
			}
		}
		else
		{
			//Covers case 1.

			//Move cursor up the GUI through different selectedChestSlot (--selectedChestSlot).
			--selectedChestSlot[player];
			warpMouseToSelectedChestSlot(player);
		}
	}
	else if ( slot > selectedChestSlot[player] )
	{
		//Moving down.

		/*
		 * Possible cases:
		 * * 1) Moving cursor down through GUI through different selectedChestSlot.
		 * * 2) Scrolling down past bottom of chest through chestitemscroll++
		 * * 3) Scrolling down past bottom of chest, max chest scroll (revoke move -- can't go beyond limit of chest).
		 */

		Item* item = nullptr;

		if ( selectedChestSlot[player] >= (kNumChestItemsToDisplay - 1) )
		{
			//Covers cases 2 & 3.

			/*
			 * Possible cases:
			 * * A) Hit very bottom of chest inventory, can't even scroll any further! Revoke movement.
			 * * B) Page down, scrolling through chestitemscroll.
			 */

			++chestitemscroll[player]; //chestitemscroll is sanitized in updateChestInventory().
		}
		else
		{
			//Covers case 1.
			//Move cursor down through the GUi through different selectedChestSlot (++selectedChestSlot).
			//This is a little bit trickier since must revoke movement if there is no item in the next slot!

			/*
			 * Two possible cases:
			 * * A) Items below this. Advance selectedChestSlot to them.
			 * * B) On last item already. Do nothing (revoke movement).
			 */

			item = openedChest[player]->getItemFromChest(invitemschest[player][selectedChestSlot[player] + 1], false, true);


			if ( item )
			{
				free(item); //Cleanup duty.

				++selectedChestSlot[player];
				warpMouseToSelectedChestSlot(player);
			}
			else
			{
			}
		}
	}
}
