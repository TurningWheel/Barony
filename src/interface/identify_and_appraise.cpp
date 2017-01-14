/*-------------------------------------------------------------------------------

	BARONY
	File: identify_and_appraise.cpp
	Desc: contains identify and appraisal related (GUI) code.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../items.hpp"
#include "../net.hpp"
#include "../player.hpp"
#include "interface.hpp"

//Identify GUI definitions.
bool identifygui_active = FALSE;
bool identifygui_appraising = FALSE;
int identifygui_offset_x = 0;
int identifygui_offset_y = 0;
bool dragging_identifyGUI = FALSE;
int identifyscroll = 0;
Item *identify_items[NUM_IDENTIFY_GUI_ITEMS];
SDL_Surface *identifyGUI_img;

int selectedIdentifySlot = -1;

void rebuildIdentifyGUIInventory() {
	list_t *identify_inventory = &stats[clientnum]->inventory;
	node_t* node = nullptr;
	Item* item = nullptr;
	int c = 0;

	if (identify_inventory) {
		//Count the number of items in the identify GUI "inventory".
		for (node = identify_inventory->first; node != NULL; node = node->next) {
			item = (Item *) node->element;
			if (item && !item->identified) {
				c++;
			}
		}
		identifyscroll = std::max(0, std::min(identifyscroll, c - 4));
		for (c = 0; c < 4; ++c) {
			identify_items[c] = NULL;
		}
		c = 0;

		//Assign the visible items to the GUI slots.
		for (node = identify_inventory->first; node != NULL; node = node->next) {
			if (node->element) {
				item = (Item *) node->element;
				if (item && !item->identified) { //Skip over all identified items.
					c++;
					if (c <= identifyscroll) {
						continue;
					}
					identify_items[c - identifyscroll - 1] = item;
					if (c > 3 + identifyscroll) {
						break;
					}
				}
			}
		}
	}
}

void updateIdentifyGUI() {
	//if (openedChest[clientnum])
	//	return; //Cannot have the identify and chest GUIs open at the same time.

	SDL_Rect pos;
	node_t *node;
	int y, c;

	//Identify GUI.
	if (identifygui_active) {
		//Center the identify GUI.
		pos.x = IDENTIFY_GUI_X;
		pos.y = IDENTIFY_GUI_Y;
		drawImage(identifyGUI_img, NULL, &pos);

		//Buttons
		if( mousestatus[SDL_BUTTON_LEFT] ) {
			//Identify GUI scroll up button.
			if (omousey >= IDENTIFY_GUI_Y + 16 && omousey < IDENTIFY_GUI_Y + 52) {
				if (omousex >= IDENTIFY_GUI_X + (identifyGUI_img->w - 28) && omousex < IDENTIFY_GUI_X + (identifyGUI_img->w - 12)) {
					buttonclick = 7;
					identifyscroll--;
					mousestatus[SDL_BUTTON_LEFT] = 0;
				}
			}
			//Identify GUI scroll down button.
			else if (omousey >= IDENTIFY_GUI_Y + 52 && omousey < IDENTIFY_GUI_Y + 88) {
				if (omousex >= IDENTIFY_GUI_X + (identifyGUI_img->w - 28) && omousex < IDENTIFY_GUI_X + (identifyGUI_img->w - 12)) {
					buttonclick = 8;
					identifyscroll++;
					mousestatus[SDL_BUTTON_LEFT] = 0;
				}
			} else if (omousey >= IDENTIFY_GUI_Y && omousey < IDENTIFY_GUI_Y + 15) {
				//Identify GUI close button.
				if (omousex >= IDENTIFY_GUI_X + 393 && omousex < IDENTIFY_GUI_X + 407) {
					buttonclick = 9;
					mousestatus[SDL_BUTTON_LEFT] = 0;
				}
				if (omousex >= IDENTIFY_GUI_X && omousex < IDENTIFY_GUI_X + 377 && omousey >= IDENTIFY_GUI_Y && omousey < IDENTIFY_GUI_Y + 15) {
					gui_clickdrag = TRUE;
					dragging_identifyGUI = TRUE;
					dragoffset_x = omousex - IDENTIFY_GUI_X;
					dragoffset_y = omousey - IDENTIFY_GUI_Y;
					mousestatus[SDL_BUTTON_LEFT] = 0;
				}
			}
		}

		// mousewheel
		if( omousex>=IDENTIFY_GUI_X+12 && omousex<IDENTIFY_GUI_X+(identifyGUI_img->w-28) ) {
			if( omousey>=IDENTIFY_GUI_Y+16 && omousey<IDENTIFY_GUI_Y+(identifyGUI_img->h-8) ) {
				if( mousestatus[SDL_BUTTON_WHEELDOWN] ) {
					mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
					identifyscroll++;
				} else if( mousestatus[SDL_BUTTON_WHEELUP] ) {
					mousestatus[SDL_BUTTON_WHEELUP] = 0;
					identifyscroll--;
				}
			}
		}

		if (dragging_identifyGUI) {
			if (gui_clickdrag) {
				identifygui_offset_x = (omousex - dragoffset_x) - (IDENTIFY_GUI_X - identifygui_offset_x);
				identifygui_offset_y = (omousey - dragoffset_y) - (IDENTIFY_GUI_Y - identifygui_offset_y);
				if (IDENTIFY_GUI_X <= camera.winx) {
					identifygui_offset_x = camera.winx - (IDENTIFY_GUI_X - identifygui_offset_x);
				}
				if (IDENTIFY_GUI_X > camera.winx + camera.winw - identifyGUI_img->w) {
					identifygui_offset_x = (camera.winx + camera.winw - identifyGUI_img->w) - (IDENTIFY_GUI_X - identifygui_offset_x);
				}
				if (IDENTIFY_GUI_Y <= camera.winy) {
					identifygui_offset_y = camera.winy - (IDENTIFY_GUI_Y - identifygui_offset_y);
				}
				if (IDENTIFY_GUI_Y > camera.winy + camera.winh - identifyGUI_img->h) {
					identifygui_offset_y = (camera.winy + camera.winh - identifyGUI_img->h) - (IDENTIFY_GUI_Y - identifygui_offset_y);
				}
			} else {
				dragging_identifyGUI = FALSE;
			}
		}

		list_t *identify_inventory = &stats[clientnum]->inventory;

		if (!identify_inventory) {
			messagePlayer(0, "Warning: stats[%d].inventory is not a valid list. This should not happen.", clientnum);
		} else {
			//Print the window label signifying this as the identify GUI.
			//char *window_name = (char*)malloc(sizeof(char));
			//strcpy(window_name, "Identify Item");
			char *window_name;
			if (identifygui_appraising) {
				window_name = language[317];
			} else {
				window_name = language[318];
			}
			ttfPrintText(ttf8, (IDENTIFY_GUI_X + 2 + ((identifyGUI_img->w / 2) - ((TTF8_WIDTH * longestline(window_name)) / 2))), IDENTIFY_GUI_Y + 4, window_name);

			//Identify GUI up button.
			if (buttonclick == 7) {
				pos.x = IDENTIFY_GUI_X + (identifyGUI_img->w - 28);
				pos.y = IDENTIFY_GUI_Y + 16;
				pos.w = 0;
				pos.h = 0;
				drawImage(invup_bmp, NULL, &pos);
			}
			//Identify GUI down button.
			if (buttonclick == 8) {
				pos.x = IDENTIFY_GUI_X + (identifyGUI_img->w - 28);
				pos.y = IDENTIFY_GUI_Y + 52;
				pos.w = 0;
				pos.h = 0;
				drawImage(invdown_bmp, NULL, &pos);
			}
			//Identify GUI close button.
			if (buttonclick == 9) {
				pos.x = IDENTIFY_GUI_X + 393;
				pos.y = IDENTIFY_GUI_Y;
				pos.w = 0;
				pos.h = 0;
				drawImage(invclose_bmp, NULL, &pos);
				identifygui_active = FALSE;
				identifygui_appraising = FALSE;

				//Cleanup identify GUI gamecontroller code here.
				selectedIdentifySlot = -1;
			}

			Item *item = NULL;

			bool selectingSlot = false;
			SDL_Rect slotPos;
			slotPos.x = IDENTIFY_GUI_X;
			slotPos.w = inventoryoptionChest_bmp->w;
			slotPos.y = IDENTIFY_GUI_Y + 16;
			slotPos.h = inventoryoptionChest_bmp->h;
			for ( int i = 0; i < NUM_IDENTIFY_GUI_ITEMS; ++i, slotPos.y += slotPos.h ) {
				pos.x = slotPos.x + 12;
				pos.w = 0;
				pos.h = 0;

				if ( omousey >= slotPos.y && omousey < slotPos.y + slotPos.h && identify_items[i] ) {
					pos.y = slotPos.y;
					drawImage(inventoryoptionChest_bmp, nullptr, &pos);
					selectedIdentifySlot = i;
					selectingSlot = true;
					if ( mousestatus[SDL_BUTTON_LEFT] || *inputPressed(joyimpulses[INJOY_MENU_USE]) ) {
						*inputPressed(joyimpulses[INJOY_MENU_USE]) = 0;
						mousestatus[SDL_BUTTON_LEFT] = 0;
						identifyGUIIdentify(identify_items[i]);

						rebuildIdentifyGUIInventory();
						if ( identify_items[i] == nullptr ) {
							if ( identify_items[0] == nullptr ) {
								//Go back to inventory.
								selectedIdentifySlot = -1;
								warpMouseToSelectedInventorySlot();
							} else {
								//Move up one slot.
								--selectedIdentifySlot;
								warpMouseToSelectedIdentifySlot();
							}
						}
					}
				}
			}

			if ( !selectingSlot ) {
				selectedIdentifySlot = -1;
			}

			//Okay, now prepare to render all the items.
			y = IDENTIFY_GUI_Y + 22;
			c = 0;
			if (identify_inventory) {
				rebuildIdentifyGUIInventory();

				//Actually render the items.
				c = 0;
				for (node = identify_inventory->first; node != NULL; node = node->next) {
					if (node->element) {
						item = (Item *) node->element;
						if (item && !item->identified) { //Skip over all identified items.
							c++;
							if (c <= identifyscroll) {
								continue;
							}
							char tempstr[64] = { 0 };
							strncpy(tempstr,item->description(),46);
							if( strlen(tempstr)==46 ) {
								strcat(tempstr," ...");
							}
							ttfPrintText(ttf8,IDENTIFY_GUI_X+36,y,tempstr);
							pos.x = IDENTIFY_GUI_X + 16;
							pos.y = IDENTIFY_GUI_Y + 17 + 18 * (c - identifyscroll - 1);
							pos.w = 16;
							pos.h = 16;
							drawImageScaled(itemSprite(item), NULL, &pos);
							y += 18;
							if (c > 3 + identifyscroll) {
								break;
							}
						}
					}
				}
			}
		}
	}
} //updateIdentifyGUI()

void identifyGUIIdentify(Item *item) {
	if (!item) {
		return;
	}
	if (item->identified) {
		messagePlayer(clientnum, language[319],item->getName());
		return;
	}

	if (!identifygui_appraising) {
		item->identified = TRUE;
		messagePlayer(clientnum, language[320], item->description());
		if (appraisal_timer > 0 && appraisal_item && appraisal_item == item->uid) {
			appraisal_timer = 0;
			appraisal_item = 0;
		}
	} else {
		//Appraising.
		messagePlayer(clientnum, language[321], item->description());

		//Tick the timer in act player.
		//Once the timer hits zero, roll to see if the item is identified.
		//If it is identified, identify it and print out a message for the player.

		identifygui_appraising = FALSE;
		appraisal_timer = getAppraisalTime(item);
		appraisal_timermax = appraisal_timer;
		appraisal_item = item->uid;
		//printlog( "DEBUGGING: Appraisal timer = %i.\n", appraisal_timer);
	}
	identifygui_active = FALSE;

	//Cleanup identify GUI gamecontroller code here.
	selectedIdentifySlot = -1;
}

int getAppraisalTime(Item *item) {
	int appraisal_time;
	if( item->type!=GEM_GLASS ) {
		appraisal_time = (items[item->type].value * 60) / (stats[clientnum]->PROFICIENCIES[PRO_APPRAISAL] + 1);    // time in ticks until item is appraised
	} else {
		appraisal_time = (1000 * 60) / (stats[clientnum]->PROFICIENCIES[PRO_APPRAISAL] + 1);    // time in ticks until item is appraised+-
	}
	appraisal_time = std::min(std::max(1,appraisal_time),36000);
	return appraisal_time;
}

inline Item* getItemInfoFromIdentifyGUI(int slot) {
	if ( slot >= 4 ) {
		return nullptr; //Out of bounds,
	}

	return identify_items[slot];
}

void selectIdentifySlot(int slot) {
	if ( slot < selectedIdentifySlot ) {
		//Moving up.

		/*
		 * Possible cases:
		 * * 1) Move cursor up the GUI through different selectedIdentifySlot.
		 * * 2) Page up through identifyscroll--
		 * * 3) Scrolling up past top of Identify GUI, no identifyscroll (move back to inventory)
		 */

		if ( selectedIdentifySlot <= 0 ) {
			//Covers cases 2 & 3.

			/*
			 * Possible cases:
			 * * A) Hit very top of Identify "inventory", can't go any further. Return to inventory.
			 * * B) Page up, scrolling through identifyscroll.
			 */

			if ( identifyscroll <= 0 ) {
				//Case 3/A: Return to inventory.
				selectedIdentifySlot = -1;
			} else {
				//Case 2/B: Page up through Identify "inventory".
				--identifyscroll;
			}
		} else {
			//Covers case 1.

			//Move cursor up the GUI through different selectedIdentifySlot (--selectedIdentifySlot).
			--selectedIdentifySlot;
			warpMouseToSelectedIdentifySlot();
		}
	} else if ( slot > selectedIdentifySlot ) {
		//Moving down.

		/*
		 * Possible cases:
		 * * 1) Moving cursor down through GUI through different selectedIdentifySlot.
		 * * 2) Scrolling down past bottom of Identify GUI through identifyscroll++
		 * * 3) Scrolling down past bottom of Identify GUI, max Identify scroll (revoke move -- can't go beyond limit of Identify GUI).
		 */

		if ( selectedIdentifySlot >= NUM_IDENTIFY_GUI_ITEMS - 1 ) {
			//Covers cases 2 & 3.
			++identifyscroll; //identifyscroll is automatically sanitized in updateIdentifyGUI().
		} else {
			//Covers case 1.
			//Move cursor down through the GUi through different selectedIdentifySlot (++selectedIdentifySlot).
			//This is a little bit trickier since must revoke movement if there is no item in the next slot!

			/*
			 * Two possible cases:
			 * * A) Items below this. Advance selectedIdentifySlot to them.
			 * * B) On last item already. Do nothing (revoke movement).
			 */

			Item *item = getItemInfoFromIdentifyGUI(selectedIdentifySlot + 1);

			if ( item ) {
				++selectedIdentifySlot;
				warpMouseToSelectedIdentifySlot();
			} else {
				//No more items. Stop.
			}
		}
	}
}

void warpMouseToSelectedIdentifySlot() {
	SDL_Rect slotPos;
	slotPos.x = IDENTIFY_GUI_X;
	slotPos.w = inventoryoptionChest_bmp->w;
	slotPos.h = inventoryoptionChest_bmp->h;
	slotPos.y = IDENTIFY_GUI_Y + 16 + (slotPos.h * selectedIdentifySlot);

	SDL_WarpMouseInWindow(screen, slotPos.x + (slotPos.w / 2), slotPos.y + (slotPos.h / 2));
}
