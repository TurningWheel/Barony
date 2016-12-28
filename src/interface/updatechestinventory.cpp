/*-------------------------------------------------------------------------------

	BARONY
	File: updatechestinventory.cpp
	Desc: contains updateChestInventory()

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../items.hpp"
#include "../sound.hpp"
#include "../net.hpp"
#include "../player.hpp"
#include "interface.hpp"

void repopulateInvItems(list_t *chestInventory) {
	int c = 0;

	//Step 1: Clear.
	for ( c = 0; c < 4; ++c ) {
		invitemschest[c] = nullptr;
	}

	node_t *node = nullptr;
	Item *item = nullptr;

	c = 0;

	//Step 2: Loop through inventory till reach part visible in chest GUI and add those items.
	for ( node = chestInventory->first; node != nullptr; node = node->next ) {
		if ( node->element ) {
			item = (Item *) node->element;
			if ( item ) {
				++c;
				if ( c <= chestitemscroll ) {
					continue;
				}
				invitemschest[c - chestitemscroll - 1] = item;
				if ( c > 3 + chestitemscroll ) {
					break;
				}
			}
		}
	}
}

int numItemsInChest() {
	node_t* node = nullptr;

	list_t *chestInventory = nullptr;
	if ( clientnum != 0 ) {
		if ( multiplayer != SERVER ) {
			chestInventory = &chestInv;
		}
	} else if (openedChest[clientnum]->children.first && openedChest[clientnum]->children.first->element) {
		chestInventory = (list_t *)openedChest[clientnum]->children.first->element;
	}

	int i = 0;

	for (node = chestInventory->first; node != nullptr; node = node->next) {
		++i;
	}

	return i;
}

void warpMouseToSelectedChestSlot() {
	int x = CHEST_INVENTORY_X + (inventoryoptionChest_bmp->w / 2);
	int y = CHEST_INVENTORY_Y + 16 + (18 * selectedChestSlot) + (inventoryoptionChest_bmp->h / 2);
	SDL_WarpMouseInWindow(screen, x, y);
}

/*-------------------------------------------------------------------------------

	updateChestInventory
	
	Processes and draws everything related to chest inventory

-------------------------------------------------------------------------------*/

inline void drawChestSlots() {
	SDL_Rect pos;
	Item *item = nullptr;

	int highlightingSlot = -1;

	if (omousex >= CHEST_INVENTORY_X && omousex < CHEST_INVENTORY_X + (inventoryChest_bmp->w - 28)) {
		pos.x = CHEST_INVENTORY_X + 12;
		pos.w = 0; pos.h = 0;

		int currentY = CHEST_INVENTORY_Y + 16;
		for ( int i = 0; i < 4; ++i, currentY += inventoryoptionChest_bmp->h ) {
			if ( omousey >= currentY && omousey < currentY + inventoryoptionChest_bmp->h ) {
				pos.y = currentY;
				item = openedChest[clientnum]->getItemFromChest(invitemschest[i], false, true);
				if ( item != nullptr ) {
					free(item); //Only YOU can prevent memleaks!
					drawImage(inventoryoptionChest_bmp, nullptr, &pos); //Highlight the moused-over slot.
					highlightingSlot = i;

					bool grabbedItem = false;

					if ( (mousestatus[SDL_BUTTON_LEFT] || *inputPressed(joyimpulses[INJOY_MENU_USE])) && openedChest[clientnum] ) {
						mousestatus[SDL_BUTTON_LEFT] = 0;
						*inputPressed(joyimpulses[INJOY_MENU_USE]) = 0;
						item = openedChest[clientnum]->getItemFromChest(invitemschest[i], false);
						messagePlayer(clientnum, language[374], item->description());
						itemPickup(clientnum, item);
						playSound(35 + rand()%3, 64);
						grabbedItem = true;
					} else if ( mousestatus[SDL_BUTTON_RIGHT] && openedChest[clientnum] ) {
						mousestatus[SDL_BUTTON_RIGHT] = 0;
						item = openedChest[clientnum]->getItemFromChest(invitemschest[i], true);
						messagePlayer(clientnum, language[374], item->description());
						itemPickup(clientnum, item); //Grab all of that item from the chest.
						playSound(35 + rand()%3, 64);
						grabbedItem = true;
					}

					if ( grabbedItem ) {
						list_t *chestInventory = nullptr;
						if ( clientnum != 0 ) {
							if ( multiplayer != SERVER ) {
								chestInventory = &chestInv;
							}
						} else if ( openedChest[clientnum]->children.first && openedChest[clientnum]->children.first->element ) {
							chestInventory = (list_t *)openedChest[clientnum]->children.first->element;
						}
						repopulateInvItems(chestInventory); //Have to regenerate, otherwise the following if check will often end up evaluating to false. //Doesn't work. #blamedennis

						item = openedChest[clientnum]->getItemFromChest(invitemschest[i], false, true);
						if ( item ) {
							free(item);
						} else {
							//Move cursor if this slot is now empty.
							--highlightingSlot;
							selectedChestSlot = highlightingSlot;
							if ( selectedChestSlot >= 0 ) {
								warpMouseToSelectedChestSlot();
							} else {
								warpMouseToSelectedInventorySlot();
							}
						}
					}
				}
			}
		}
	}

	if ( highlightingSlot >= 0 ) {
		selectedChestSlot = highlightingSlot;
	} else {
		selectedChestSlot = -1;
	}
}

void updateChestInventory() {
	SDL_Rect pos;
	node_t *node, *nextnode;
	int y, c;
	int chest_buttonclick = 0;
	Item *item;

	//Chest inventory GUI.
	if (openedChest[clientnum]) {
		//Center the chest GUI.
		//pos.x = ((xres - winx) / 2) - (inventory_bmp->w / 2);
		pos.x = CHEST_INVENTORY_X; //(winx + ((winw / 2) - (inventoryChest_bmp->w / 2)))
		//pos.x = character_bmp->w;
		//pos.y = ((yres - winy) / 2) - (inventory_bmp->h / 2);
		pos.y = CHEST_INVENTORY_Y; //(winy + ((winh - winy) - (inventoryChest_bmp->h / 2)));
		drawImage(inventoryChest_bmp, NULL, &pos);

		// buttons
		if( mousestatus[SDL_BUTTON_LEFT] && !selectedItem ) {
			if (openedChest[clientnum]) {
				//Chest inventory scroll up button.
				if (omousey >= CHEST_INVENTORY_Y + 16 && omousey < CHEST_INVENTORY_Y + 52) {
					if (omousex >= CHEST_INVENTORY_X + (inventoryChest_bmp->w - 28) && omousex < CHEST_INVENTORY_X + (inventoryChest_bmp->w - 12)) {
						chest_buttonclick = 7;
						chestitemscroll--;
						mousestatus[SDL_BUTTON_LEFT] = 0;
					}
				}
				//Chest inventory scroll down button.
				else if (omousey >= CHEST_INVENTORY_Y + 52 && omousey < CHEST_INVENTORY_Y + 88) {
					if (omousex >= CHEST_INVENTORY_X + (inventoryChest_bmp->w - 28) && omousex < CHEST_INVENTORY_X + (inventoryChest_bmp->w - 12)) {
						chest_buttonclick = 8;
						chestitemscroll++;
						mousestatus[SDL_BUTTON_LEFT] = 0;
					}
				}
				else if (omousey >= CHEST_INVENTORY_Y && omousey < CHEST_INVENTORY_Y + 15) {
					//Chest inventory close button.
					if (omousex >= CHEST_INVENTORY_X + 393 && omousex < CHEST_INVENTORY_X + 407) {
						chest_buttonclick = 9;
						mousestatus[SDL_BUTTON_LEFT] = 0;
					}
					//Chest inventory grab all button.
					if (omousex >= CHEST_INVENTORY_X + 376 && omousex < CHEST_INVENTORY_X + 391) {
						chest_buttonclick = 10;
						mousestatus[SDL_BUTTON_LEFT] = 0;
					}
					if (omousex >= CHEST_INVENTORY_X && omousex < CHEST_INVENTORY_X + 377 && omousey >= CHEST_INVENTORY_Y && omousey < CHEST_INVENTORY_Y + 15) {
						gui_clickdrag = TRUE;
						dragging_chestGUI = TRUE;
						dragoffset_x = omousex - CHEST_INVENTORY_X;
						dragoffset_y = omousey - CHEST_INVENTORY_Y;
						mousestatus[SDL_BUTTON_LEFT] = 0;
					}
				}
			}
		}
		
		if ( *inputPressed(joyimpulses[INJOY_MENU_CHEST_GRAB_ALL]) ) { //Gamepad "Y" button grabs all items in chest.
			*inputPressed(joyimpulses[INJOY_MENU_CHEST_GRAB_ALL]) = 0;
			chest_buttonclick = 10;
		}

		// mousewheel
		if( omousex>=CHEST_INVENTORY_X+12 && omousex<CHEST_INVENTORY_X+(inventoryChest_bmp->w-28) ) {
			if( omousey>=CHEST_INVENTORY_Y+16 && omousey<CHEST_INVENTORY_Y+(inventoryChest_bmp->h-8) ) {
				if( mousestatus[SDL_BUTTON_WHEELDOWN] ) {
					mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
					chestitemscroll++;
				} else if( mousestatus[SDL_BUTTON_WHEELUP] ) {
					mousestatus[SDL_BUTTON_WHEELUP] = 0;
					chestitemscroll--;
				}
			}
		}
		
		if (dragging_chestGUI) {
			if (gui_clickdrag) {
				chestgui_offset_x = (omousex - dragoffset_x) - (CHEST_INVENTORY_X - chestgui_offset_x);
				chestgui_offset_y = (omousey - dragoffset_y) - (CHEST_INVENTORY_Y - chestgui_offset_y);
				if (CHEST_INVENTORY_X <= camera.winx)
					chestgui_offset_x = camera.winx - (CHEST_INVENTORY_X - chestgui_offset_x);
				if (CHEST_INVENTORY_X > camera.winx+camera.winw-inventoryChest_bmp->w)
					chestgui_offset_x = (camera.winx+camera.winw-inventoryChest_bmp->w) - (CHEST_INVENTORY_X - chestgui_offset_x);
				if (CHEST_INVENTORY_Y <= camera.winy)
					chestgui_offset_y = camera.winy - (CHEST_INVENTORY_Y - chestgui_offset_y);
				if (CHEST_INVENTORY_Y > camera.winy+camera.winh-inventoryChest_bmp->h)
					chestgui_offset_y = (camera.winy+camera.winh-inventoryChest_bmp->h) - (CHEST_INVENTORY_Y - chestgui_offset_y);
			} else {
				dragging_chestGUI = FALSE;
			}
		}

		list_t *chest_inventory = NULL;
		if (clientnum != 0) {
			if (multiplayer != SERVER) {
				chest_inventory = &chestInv;
			}
		} else if (openedChest[clientnum]->children.first && openedChest[clientnum]->children.first->element) {
			chest_inventory = (list_t *)openedChest[clientnum]->children.first->element;
		}

		if (!chest_inventory) {
			messagePlayer(0, "Warning: openedChest[%d] has no inventory. This should not happen.", clientnum);
		} else {
			//Print the window label signifying this as the chest inventory GUI.
			ttfPrintText(ttf8, (CHEST_INVENTORY_X + 2 + ((inventoryChest_bmp->w / 2) - ((TTF8_WIDTH * 15) / 2))), CHEST_INVENTORY_Y + 4, language[373]);

			//Chest inventory up button.
			if (chest_buttonclick == 7) {
				pos.x = CHEST_INVENTORY_X + (inventoryChest_bmp->w - 28); pos.y = CHEST_INVENTORY_Y + 16;
				pos.w=0; pos.h=0;
				drawImage(invup_bmp, NULL, &pos);
			}
			//Chest inventory down button.
			if (chest_buttonclick == 8) {
				pos.x = CHEST_INVENTORY_X + (inventoryChest_bmp->w - 28); pos.y = CHEST_INVENTORY_Y + 52;
				pos.w=0; pos.h=0;
				drawImage(invdown_bmp, NULL, &pos);
			}
			//Chest inventory close button.
			if (chest_buttonclick == 9) {
				pos.x = CHEST_INVENTORY_X + 393; pos.y = CHEST_INVENTORY_Y;
				pos.w=0; pos.h=0;
				drawImage(invclose_bmp, NULL, &pos);
				if (openedChest[clientnum])
					openedChest[clientnum]->closeChest();
			}
			//Chest inventory grab all items button.
			if (chest_buttonclick == 10) {
				pos.x = CHEST_INVENTORY_X + 376; pos.y = CHEST_INVENTORY_Y;
				pos.w=0; pos.h=0;
				drawImage(invgraball_bmp, NULL, &pos);
				for (node = chest_inventory->first; node != NULL; node=nextnode) {
					nextnode=node->next;
					if (node->element && openedChest[clientnum]) {
						item = openedChest[clientnum]->getItemFromChest(static_cast<Item* >(node->element), TRUE);
						if( item != NULL ) {
							messagePlayer(clientnum, language[374], item->description());
							itemPickup(clientnum, item);
							playSound(35+rand()%3,64);
						}
					}
				}
			}

			drawChestSlots();

			//Okay, now prepare to render all the items.
			y = CHEST_INVENTORY_Y + 22; c = 0;
			if (chest_inventory) {
				c = numItemsInChest();
				chestitemscroll = std::max(0, std::min(chestitemscroll, c - 4));

				repopulateInvItems(chest_inventory); //This doesn't work. I blame Dennis.

				c = 0;

				//Actually render the items.
				for (node = chest_inventory->first; node != NULL; node = node->next) {
					if (node->element) {
						item = (Item *) node->element;
						if (item) {
							c++;
							if (c <= chestitemscroll)
								continue;
							char tempstr[64] = { 0 };
							strncpy(tempstr,item->description(),46);
							if( strlen(tempstr)==46 )
								strcat(tempstr," ...");
							ttfPrintText(ttf8,CHEST_INVENTORY_X+36,y,tempstr);
							pos.x = CHEST_INVENTORY_X + 16;
							pos.y = CHEST_INVENTORY_Y + 17 + 18 * (c - chestitemscroll - 1);
							pos.w = 16; pos.h = 16;
							drawImageScaled(itemSprite(item), NULL, &pos);
							y += 18;
							if (c > 3 + chestitemscroll)
								break;
						}
					}
				}
			}
		}
	}
}

void selectChestSlot(int slot) {
	//TODO?: Grab amount (difference between slot and selectedChestSlot)?

	if ( slot < selectedChestSlot ) {
		//Moving up.

		/*
		 * Possible cases:
		 * * 1) Move cursor up the GUI through different selectedChestSlot.
		 * * 2) Page up through chestitemscroll--
		 * * 3) Scrolling up past top of chest, no chestitemscroll (move back to inventory)
		 */

		if ( selectedChestSlot <= 0 ) {
			//Covers cases 2 & 3.

			/*
			 * Possible cases:
			 * * A) Hit very top of chest inventory, can't go any further. Return to inventory.
			 * * B) Page up, scrolling through chestitemscroll.
			 */

			if ( chestitemscroll <= 0 ) {
				//Case 3/A: Return to inventory.
				selectedChestSlot = -1;
			} else {
				//Case 2/B: Page up through chest inventory.
				--chestitemscroll;
			}
		} else {
			//Covers case 1.

			//Move cursor up the GUI through different selectedChestSlot (--selectedChestSlot).
			--selectedChestSlot;
			warpMouseToSelectedChestSlot();
		}
	} else if ( slot > selectedChestSlot ) {
		//Moving down.

		/*
		 * Possible cases:
		 * * 1) Moving cursor down through GUI through different selectedChestSlot.
		 * * 2) Scrolling down past bottom of chest through chestitemscroll++
		 * * 3) Scrolling down past bottom of chest, max chest scroll (revoke move -- can't go beyond limit of chest).
		 */

		Item *item = nullptr;

		if ( selectedChestSlot >= 3 ) {
			//Covers cases 2 & 3.

			/*
			 * Possible cases:
			 * * A) Hit very bottom of chest inventory, can't even scroll any further! Revoke movement.
			 * * B) Page down, scrolling through chestitemscroll.
			 */

			int maxChestItemScroll = numItemsInChest() - 4;
			if ( chestitemscroll < maxChestItemScroll ) {
				//Case 2/B: page down (++chestitemscroll).
				++chestitemscroll;
			} else {
				//Case 3/A.
				//Max chest item scroll here. Rebuke movement (do jack diddly squat).
			}
		} else {
			//Covers case 1.
			//Move cursor down through the GUi through different selectedChestSlot (++selectedChestSlot).
			//This is a little bit trickier since must revoke movement if there is no item in the next slot!

			/*
			 * Two possible cases:
			 * * A) Items below this. Advance selectedChestSlot to them.
			 * * B) On last item already. Do nothing (revoke movement).
			 */

			item = openedChest[clientnum]->getItemFromChest(invitemschest[selectedChestSlot + 1], false, true);


			if ( item ) {
				free(item); //Cleanup duty.

				++selectedChestSlot;
				warpMouseToSelectedChestSlot();
			} else {
			}
		}
	}
}
