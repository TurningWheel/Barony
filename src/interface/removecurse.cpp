/*-------------------------------------------------------------------------------

	BARONY
	File: removecurse.cpp
	Desc: GUI code for the remove curse spell.

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

void updateRemoveCurseGUI() {
	SDL_Rect pos;
	node_t *node;
	int y, c;

	//Remove Curse GUI.
	if (removecursegui_active) {
		//Center the remove curse GUI.
		pos.x = REMOVECURSE_GUI_X;
		pos.y = REMOVECURSE_GUI_Y;
		drawImage(identifyGUI_img, NULL, &pos);

		//Buttons
		if( mousestatus[SDL_BUTTON_LEFT] ) {
			//Remove Curse GUI scroll up button.
			if (omousey >= REMOVECURSE_GUI_Y + 16 && omousey < REMOVECURSE_GUI_Y + 52) {
				if (omousex >= REMOVECURSE_GUI_X + (identifyGUI_img->w - 28) && omousex < REMOVECURSE_GUI_X + (identifyGUI_img->w - 12)) {
					buttonclick = 7;
					removecursescroll--;
					mousestatus[SDL_BUTTON_LEFT] = 0;
				}
			}
			//Remove Curse GUI scroll down button.
			else if (omousey >= REMOVECURSE_GUI_Y + 52 && omousey < REMOVECURSE_GUI_Y + 88) {
				if (omousex >= REMOVECURSE_GUI_X + (identifyGUI_img->w - 28) && omousex < REMOVECURSE_GUI_X + (identifyGUI_img->w - 12)) {
					buttonclick = 8;
					removecursescroll++;
					mousestatus[SDL_BUTTON_LEFT] = 0;
				}
			} else if (omousey >= REMOVECURSE_GUI_Y && omousey < REMOVECURSE_GUI_Y + 15) {
				//Remove Curse GUI close button.
				if (omousex >= REMOVECURSE_GUI_X + 393 && omousex < REMOVECURSE_GUI_X + 407) {
					buttonclick = 9;
					mousestatus[SDL_BUTTON_LEFT] = 0;
				}
				if (omousex >= REMOVECURSE_GUI_X && omousex < REMOVECURSE_GUI_X + 377 && omousey >= REMOVECURSE_GUI_Y && omousey < REMOVECURSE_GUI_Y + 15) {
					gui_clickdrag = TRUE;
					dragging_removecurseGUI = TRUE;
					dragoffset_x = omousex - REMOVECURSE_GUI_X;
					dragoffset_y = omousey - REMOVECURSE_GUI_Y;
					mousestatus[SDL_BUTTON_LEFT] = 0;
				}
			}
		}

		// mousewheel
		if( omousex>=REMOVECURSE_GUI_X+12 && omousex<REMOVECURSE_GUI_X+(identifyGUI_img->w-28) ) {
			if( omousey>=REMOVECURSE_GUI_Y+16 && omousey<REMOVECURSE_GUI_Y+(identifyGUI_img->h-8) ) {
				if( mousestatus[SDL_BUTTON_WHEELDOWN] ) {
					mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
					removecursescroll++;
				} else if( mousestatus[SDL_BUTTON_WHEELUP] ) {
					mousestatus[SDL_BUTTON_WHEELUP] = 0;
					removecursescroll--;
				}
			}
		}

		if (dragging_removecurseGUI) {
			if (gui_clickdrag) {
				removecursegui_offset_x = (omousex - dragoffset_x) - (REMOVECURSE_GUI_X - removecursegui_offset_x);
				removecursegui_offset_y = (omousey - dragoffset_y) - (REMOVECURSE_GUI_Y - removecursegui_offset_y);
				if (REMOVECURSE_GUI_X <= camera.winx) {
					removecursegui_offset_x = camera.winx - (REMOVECURSE_GUI_X - removecursegui_offset_x);
				}
				if (REMOVECURSE_GUI_X > camera.winx + camera.winw - identifyGUI_img->w) {
					removecursegui_offset_x = (camera.winx + camera.winw - identifyGUI_img->w) - (REMOVECURSE_GUI_X - removecursegui_offset_x);
				}
				if (REMOVECURSE_GUI_Y <= camera.winy) {
					removecursegui_offset_y = camera.winy - (REMOVECURSE_GUI_Y - removecursegui_offset_y);
				}
				if (REMOVECURSE_GUI_Y > camera.winy + camera.winh - identifyGUI_img->h) {
					removecursegui_offset_y = (camera.winy + camera.winh - identifyGUI_img->h) - (REMOVECURSE_GUI_Y - removecursegui_offset_y);
				}
			} else {
				dragging_removecurseGUI = FALSE;
			}
		}

		list_t *removecurse_inventory = &stats[clientnum]->inventory;

		if (!removecurse_inventory) {
			messagePlayer(0, "Warning: stats[%d].inventory is not a valid list. This should not happen.", clientnum);
		} else {
			//Print the window label signifying this as the remove curse GUI.
			char *window_name;
			window_name = language[346];
			ttfPrintText(ttf8, (REMOVECURSE_GUI_X + 2 + ((identifyGUI_img->w / 2) - ((TTF8_WIDTH * longestline(window_name)) / 2))), REMOVECURSE_GUI_Y + 4, window_name);

			//Remove Curse GUI up button.
			if (buttonclick == 7) {
				pos.x = REMOVECURSE_GUI_X + (identifyGUI_img->w - 28);
				pos.y = REMOVECURSE_GUI_Y + 16;
				pos.w = 0;
				pos.h = 0;
				drawImage(invup_bmp, NULL, &pos);
			}
			//Remove Curse GUI down button.
			if (buttonclick == 8) {
				pos.x = REMOVECURSE_GUI_X + (identifyGUI_img->w - 28);
				pos.y = REMOVECURSE_GUI_Y + 52;
				pos.w = 0;
				pos.h = 0;
				drawImage(invdown_bmp, NULL, &pos);
			}
			//Remove Curse GUI close button.
			if (buttonclick == 9) {
				pos.x = REMOVECURSE_GUI_X + 393;
				pos.y = REMOVECURSE_GUI_Y;
				pos.w = 0;
				pos.h = 0;
				drawImage(invclose_bmp, NULL, &pos);
				removecursegui_active = FALSE;
			}

			Item *item = NULL;
			if (omousex >= REMOVECURSE_GUI_X && omousex < REMOVECURSE_GUI_X + (identifyGUI_img->w - 28)) {
				pos.x = REMOVECURSE_GUI_X + 12;
				pos.w = 0;
				pos.h = 0;
				if (omousey >= REMOVECURSE_GUI_Y + 16 && omousey < REMOVECURSE_GUI_Y + 34) { //First inventory slot.
					pos.y = REMOVECURSE_GUI_Y + 16;
					drawImage(inventoryoptionChest_bmp, NULL, &pos);
					if (mousestatus[SDL_BUTTON_LEFT]) {
						mousestatus[SDL_BUTTON_LEFT] = 0;
						removecurseGUIRemoveCurse(removecurse_items[0]);
					}
				} else if (omousey >= REMOVECURSE_GUI_Y + 34 && omousey < REMOVECURSE_GUI_Y + 52) {
					pos.y = REMOVECURSE_GUI_Y + 34;
					drawImage(inventoryoptionChest_bmp, NULL, &pos);
					if (mousestatus[SDL_BUTTON_LEFT]) {
						mousestatus[SDL_BUTTON_LEFT]=0;
						removecurseGUIRemoveCurse(removecurse_items[1]);
					}
				} else if (omousey >= REMOVECURSE_GUI_Y + 52 && omousey < REMOVECURSE_GUI_Y + 70 ) {
					pos.y = REMOVECURSE_GUI_Y + 52;
					drawImage(inventoryoptionChest_bmp, NULL, &pos);
					if( mousestatus[SDL_BUTTON_LEFT] ) {
						mousestatus[SDL_BUTTON_LEFT]=0;
						removecurseGUIRemoveCurse(removecurse_items[2]);
					}
				} else if (omousey >= REMOVECURSE_GUI_Y + 70 && omousey < REMOVECURSE_GUI_Y + 88) {
					pos.y = REMOVECURSE_GUI_Y + 70;
					drawImage(inventoryoptionChest_bmp, NULL, &pos);
					if( mousestatus[SDL_BUTTON_LEFT] ) {
						mousestatus[SDL_BUTTON_LEFT]=0;
						removecurseGUIRemoveCurse(removecurse_items[3]);
					}
				}
			}

			//Okay, now prepare to render all the items.
			y = REMOVECURSE_GUI_Y + 22;
			c = 0;
			if (removecurse_inventory) {
				for (node = removecurse_inventory->first; node != NULL; node = node->next) {
					item = (Item *) node->element;
					if (item && item->identified && item->beatitude < 0) {
						c++;
					}
				}
				removecursescroll = std::max(0, std::min(removecursescroll, c - 4));
				for (c = 0; c < 4; ++c) {
					removecurse_items[c] = NULL;
				}
				c = 0;

				//Actually render the items.
				for (node = removecurse_inventory->first; node != NULL; node = node->next) {
					if (node->element) {
						item = (Item *) node->element;
						if (item && item->identified && item->beatitude < 0) { //Skip over all unidentified or uncursed items.
							c++;
							if (c <= removecursescroll) {
								continue;
							}
							removecurse_items[c - removecursescroll - 1] = item;
							char tempstr[64] = { 0 };
							strncpy(tempstr,item->description(),46);
							if( strlen(tempstr)==46 ) {
								strcat(tempstr," ...");
							}
							ttfPrintText(ttf8,REMOVECURSE_GUI_X+36,y,tempstr);
							pos.x = REMOVECURSE_GUI_X + 16;
							pos.y = REMOVECURSE_GUI_Y + 17 + 18 * (c - removecursescroll - 1);
							pos.w = 16;
							pos.h = 16;
							drawImageScaled(itemSprite(item), NULL, &pos);
							y += 18;
							if (c > 3 + removecursescroll) {
								break;
							}
						}
					}
				}
			}
		}
	}
} //updateRemoveCurseGUI()

void removecurseGUIRemoveCurse(Item *item) {
	if (!item) {
		return;
	}
	if (item->beatitude >= 0) {
		messagePlayer(clientnum, language[347],item->getName());
		return;
	}

	item->beatitude = 0; //0 = uncursed. > 0 = blessed.
	messagePlayer(clientnum, language[348], item->description());
	removecursegui_active = FALSE;
	if( multiplayer==CLIENT && itemIsEquipped(item,clientnum) ) {
		// the client needs to inform the server that their equipment was uncursed.
		int armornum = 0;
		if( item == stats[clientnum]->helmet ) {
			armornum = 0;
		} else if( item == stats[clientnum]->breastplate ) {
			armornum = 1;
		} else if( item == stats[clientnum]->gloves ) {
			armornum = 2;
		} else if( item == stats[clientnum]->shoes ) {
			armornum = 3;
		} else if( item == stats[clientnum]->shield ) {
			armornum = 4;
		} else if( item == stats[clientnum]->weapon ) {
			armornum = 5;
		} else if( item == stats[clientnum]->cloak ) {
			armornum = 6;
		} else if( item == stats[clientnum]->amulet ) {
			armornum = 7;
		} else if( item == stats[clientnum]->ring ) {
			armornum = 8;
		} else if( item == stats[clientnum]->mask ) {
			armornum = 9;
		}
		strcpy((char *)net_packet->data,"RCUR");
		net_packet->data[4] = clientnum;
		net_packet->data[5] = armornum;
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 6;
		sendPacketSafe(net_sock, -1, net_packet, 0);
	}
}