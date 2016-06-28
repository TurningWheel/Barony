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
#include "interface.hpp"

/*-------------------------------------------------------------------------------

	updateChestInventory
	
	Processes and draws everything related to chest inventory

-------------------------------------------------------------------------------*/

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

			Item *item = NULL;
			if (omousex >= CHEST_INVENTORY_X && omousex < CHEST_INVENTORY_X + (inventoryChest_bmp->w - 28)) {
				pos.x = CHEST_INVENTORY_X + 12;
				pos.w = 0; pos.h = 0;
				if (omousey >= CHEST_INVENTORY_Y + 16 && omousey < CHEST_INVENTORY_Y + 34) { //First inventory slot.
					pos.y = CHEST_INVENTORY_Y + 16;
					drawImage(inventoryoptionChest_bmp, NULL, &pos);
					if (mousestatus[SDL_BUTTON_LEFT] && openedChest[clientnum]) {
						mousestatus[SDL_BUTTON_LEFT]=0;
						item = openedChest[clientnum]->getItemFromChest(invitemschest[0], FALSE);
						if( item != NULL ) {
							messagePlayer(clientnum, language[374], item->description());
							itemPickup(clientnum, item);
							playSound(35+rand()%3,64);
						}
					} else if (mousestatus[SDL_BUTTON_RIGHT] && openedChest[clientnum]) {
						mousestatus[SDL_BUTTON_RIGHT]=0;
						item = openedChest[clientnum]->getItemFromChest(invitemschest[0], TRUE);
						if( item != NULL ) {
							messagePlayer(clientnum, language[374], item->description());
							itemPickup(clientnum, item); //Grab all of that item from the chest.
							playSound(35+rand()%3,64);
						}
					}
				}
				else if (omousey >= CHEST_INVENTORY_Y + 34 && omousey < CHEST_INVENTORY_Y + 52) {
					pos.y = CHEST_INVENTORY_Y + 34;
					drawImage(inventoryoptionChest_bmp, NULL, &pos);
					if (mousestatus[SDL_BUTTON_LEFT] && openedChest[clientnum]) {
						mousestatus[SDL_BUTTON_LEFT]=0;
						item = openedChest[clientnum]->getItemFromChest(invitemschest[1], FALSE);
						if( item != NULL ) {
							messagePlayer(clientnum, language[374], item->description());
							itemPickup(clientnum, item);
							playSound(35+rand()%3,64);
						}
					} else if (mousestatus[SDL_BUTTON_RIGHT] && openedChest[clientnum]) {
						mousestatus[SDL_BUTTON_RIGHT]=0;
						item = openedChest[clientnum]->getItemFromChest(invitemschest[1], TRUE);
						if( item != NULL ) {
							messagePlayer(clientnum, language[374], item->description());
							itemPickup(clientnum, item); //Grab all of that item from the chest.
							playSound(35+rand()%3,64);
						}
					}
				}
				else if (omousey >= CHEST_INVENTORY_Y + 52 && omousey < CHEST_INVENTORY_Y + 70 ) {
					pos.y = CHEST_INVENTORY_Y + 52;
					drawImage(inventoryoptionChest_bmp, NULL, &pos);
					if (mousestatus[SDL_BUTTON_LEFT] && openedChest[clientnum]) {
						mousestatus[SDL_BUTTON_LEFT]=0;
						item = openedChest[clientnum]->getItemFromChest(invitemschest[2], FALSE);
						if( item != NULL ) {
							messagePlayer(clientnum, language[374], item->description());
							itemPickup(clientnum, item);
							playSound(35+rand()%3,64);
						}
					} else if (mousestatus[SDL_BUTTON_RIGHT] && openedChest[clientnum]) {
						mousestatus[SDL_BUTTON_RIGHT]=0;
						item = openedChest[clientnum]->getItemFromChest(invitemschest[2], TRUE);
						if( item != NULL ) {
							messagePlayer(clientnum, language[374], item->description());
							itemPickup(clientnum, item); //Grab all of that item from the chest.
							playSound(35+rand()%3,64);
						}
					}
				}
				else if (omousey >= CHEST_INVENTORY_Y + 70 && omousey < CHEST_INVENTORY_Y + 88) {
					pos.y = CHEST_INVENTORY_Y + 70;
					drawImage(inventoryoptionChest_bmp, NULL, &pos);
					if (mousestatus[SDL_BUTTON_LEFT] && openedChest[clientnum]) {
						mousestatus[SDL_BUTTON_LEFT]=0;
						item = openedChest[clientnum]->getItemFromChest(invitemschest[3], FALSE);
						if( item != NULL ) {
							messagePlayer(clientnum, language[374], item->description());
							itemPickup(clientnum, item);
							playSound(35+rand()%3,64);
						}
					} else if (mousestatus[SDL_BUTTON_RIGHT] && openedChest[clientnum]) {
						mousestatus[SDL_BUTTON_RIGHT]=0;
						item = openedChest[clientnum]->getItemFromChest(invitemschest[3], TRUE);
						if( item != NULL ) {
							messagePlayer(clientnum, language[374], item->description());
							itemPickup(clientnum, item); //Grab all of that item from the chest.
							playSound(35+rand()%3,64);
						}
					}
				}
			}

			//Okay, now prepare to render all the items.
			y = CHEST_INVENTORY_Y + 22; c = 0;
			if (chest_inventory) {
				for (node = chest_inventory->first; node != NULL; node = node->next) {
					c++;
				}
				chestitemscroll = std::max(0, std::min(chestitemscroll, c - 4));
				for (c = 0; c < 4; ++c) {
					invitemschest[c] = NULL;
				}
				c = 0;

				//Actually render the items.
				for (node = chest_inventory->first; node != NULL; node = node->next) {
					if (node->element) {
						item = (Item *) node->element;
						if (item) {
							c++;
							if (c <= chestitemscroll)
								continue;
							invitemschest[c - chestitemscroll - 1] = item;
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