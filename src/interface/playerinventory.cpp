/*-------------------------------------------------------------------------------

	BARONY
	File: playerinventory.cpp
	Desc: contains player inventory related functions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../items.hpp"
#include "../shops.hpp"
#include "../sound.hpp"
#include "../net.hpp"
#include "../magic/magic.hpp"
#include "../menu.hpp"
#include "../player.hpp"
#include "interface.hpp"


SDL_Surface *inventory_mode_item_img = NULL;
SDL_Surface *inventory_mode_item_highlighted_img = NULL;
SDL_Surface *inventory_mode_spell_img = NULL;
SDL_Surface *inventory_mode_spell_highlighted_img = NULL;
int inventory_mode = INVENTORY_MODE_ITEM;

/*-------------------------------------------------------------------------------

	itemUseString
	
	Returns a string with the verb cooresponding to the item which is
	to be used

-------------------------------------------------------------------------------*/

char *itemUseString(Item *item) {
	if( itemCategory(item)==WEAPON ) {
		if( itemIsEquipped(item,clientnum) )
			return language[323];
		else
			return language[324];
	}
	else if( itemCategory(item)==ARMOR ) {
		switch( item->type ) {
			case WOODEN_SHIELD:
			case BRONZE_SHIELD:
			case IRON_SHIELD:
			case STEEL_SHIELD:
			case STEEL_SHIELD_RESISTANCE:
			if( itemIsEquipped(item,clientnum) )
				return language[325];
			else
				return language[326];
			default:
				break;
		}
		if( itemIsEquipped(item,clientnum) )
			return language[327];
		else
			return language[328];
	}
	else if( itemCategory(item)==AMULET ) {
		if( itemIsEquipped(item,clientnum) )
			return language[327];
		else
			return language[328];
	}
	else if( itemCategory(item)==POTION ) {
		return language[329];
	}
	else if( itemCategory(item)==SCROLL ) {
		return language[330];
	}
	else if( itemCategory(item)==MAGICSTAFF ) {
		if( itemIsEquipped(item,clientnum) )
			return language[323];
		else
			return language[324];
	}
	else if( itemCategory(item)==RING ) {
		if( itemIsEquipped(item,clientnum) )
			return language[327];
		else
			return language[331];
	}
	else if( itemCategory(item)==SPELLBOOK ) {
		return language[330];
	}
	else if( itemCategory(item)==GEM ) {
		if( itemIsEquipped(item,clientnum) )
			return language[323];
		else
			return language[324];
	}
	else if( itemCategory(item)==TOOL ) {
		switch( item->type ) {
			case TOOL_PICKAXE:
				if( itemIsEquipped(item,clientnum) )
					return language[323];
				else
					return language[324];
			case TOOL_TINOPENER:
				return language[1881];
			case TOOL_MIRROR:
				return language[332];
			case TOOL_LOCKPICK:
			case TOOL_SKELETONKEY:
				if( itemIsEquipped(item,clientnum) )
					return language[333];
				else
					return language[334];
			case TOOL_TORCH:
			case TOOL_LANTERN:
				if( itemIsEquipped(item,clientnum) )
					return language[335];
				else
					return language[336];
			case TOOL_BLINDFOLD:
				if( itemIsEquipped(item,clientnum) )
					return language[327];
				else
					return language[328];
			case TOOL_TOWEL:
				return language[332];
			case TOOL_GLASSES:
				if( itemIsEquipped(item,clientnum) )
					return language[327];
				else
					return language[331];
			case TOOL_BEARTRAP:
				return language[337];
			default:
				break;
		}
	}
	else if( itemCategory(item)==FOOD ) {
		return language[338];
	}
	else if( itemCategory(item)==BOOK ) {
		return language[330];
	}
	else if( itemCategory(item)==SPELL_CAT ) {
		return language[339];
	}
	return language[332];
}

/*-------------------------------------------------------------------------------

	updateAppraisalItemBox

	draws the current item being appraised

-------------------------------------------------------------------------------*/

void updateAppraisalItemBox() {
	SDL_Rect pos;
	Item *item;
	int x, y;

	x = INVENTORY_STARTX;
	y = INVENTORY_STARTY;

	// appraisal item box
	if( (item=uidToItem(appraisal_item))!=NULL && appraisal_timer>0 ) {
		if( !shootmode ) {
			pos.x = x+16;
			pos.y = y+INVENTORY_SIZEY*INVENTORY_SLOTSIZE+16;
		} else {
			pos.x = 16;
			pos.y = 16;
		}
		int w1, w2;
		TTF_SizeUTF8(ttf12,language[340],&w1,NULL);
		TTF_SizeUTF8(ttf12,item->getName(),&w2,NULL); w2 += 48;
		pos.w = std::max(w1,w2)+8;
		pos.h = 68;
		drawTooltip(&pos);

		char tempstr[64] = { 0 };
		snprintf(tempstr, 63, language[341], (((double)(appraisal_timermax-appraisal_timer))/((double)appraisal_timermax))*100);
		ttfPrintText( ttf12, pos.x+8, pos.y+8, tempstr );
		if( !shootmode ) {
			pos.x = x+24;
			pos.y = y+INVENTORY_SIZEY*INVENTORY_SLOTSIZE+16+24;
		} else {
			pos.x = 24;
			pos.y = 16+24;
		}
		ttfPrintText( ttf12, pos.x+40, pos.y+8, item->getName() );
		pos.w = 32;
		pos.h = 32;
		drawImageScaled(itemSprite(item),NULL,&pos);
	}
}

/*-------------------------------------------------------------------------------

	updatePlayerInventory
	
	Draws and processes everything related to the player's inventory window

-------------------------------------------------------------------------------*/

Item *selectedItem=NULL;
bool toggleclick=FALSE;

bool itemMenuOpen=FALSE;
int itemMenuX=0;
int itemMenuY=0;
int itemMenuSelected=0;
Uint32 itemMenuItem=0;

void updatePlayerInventory() {
	SDL_Rect pos, mode_pos;
	node_t *node, *nextnode;
	int x, y;

	x = INVENTORY_STARTX;
	y = INVENTORY_STARTY;

	// draw translucent box
	pos.x = x; pos.y = y;
	pos.w = INVENTORY_SIZEX*INVENTORY_SLOTSIZE; pos.h = INVENTORY_SIZEY*INVENTORY_SLOTSIZE;
	drawRect(&pos,0,224);

	// draw grid
	pos.x = x; pos.y = y;
	pos.w = INVENTORY_SIZEX*INVENTORY_SLOTSIZE; pos.h = INVENTORY_SIZEY*INVENTORY_SLOTSIZE;
	drawLine(pos.x,pos.y,pos.x,pos.y+pos.h,SDL_MapRGB(mainsurface->format,150,150,150),255);
	drawLine(pos.x,pos.y,pos.x+pos.w,pos.y,SDL_MapRGB(mainsurface->format,150,150,150),255);
	for( x=0; x<=INVENTORY_SIZEX; x++ ) {
		drawLine(pos.x+x*INVENTORY_SLOTSIZE,pos.y,pos.x+x*INVENTORY_SLOTSIZE,pos.y+pos.h,SDL_MapRGB(mainsurface->format,150,150,150),255);
	}
	for( y=0; y<=INVENTORY_SIZEY; y++ ) {
		drawLine(pos.x,pos.y+y*INVENTORY_SLOTSIZE,pos.x+pos.w,pos.y+y*INVENTORY_SLOTSIZE,SDL_MapRGB(mainsurface->format,150,150,150),255);
	}

	//Highlight currently selected inventory slot (for gamepad).
	pos.w = INVENTORY_SLOTSIZE; pos.h = INVENTORY_SLOTSIZE;
	for (x = 0; x < INVENTORY_SIZEX; x++)
	{
		for (y = 0; y < INVENTORY_SIZEY; y++)
		{
			if (x == selected_inventory_slot_x && y == selected_inventory_slot_y)
			{
				pos.x = INVENTORY_STARTX + x*INVENTORY_SLOTSIZE;
				pos.y = INVENTORY_STARTY + y*INVENTORY_SLOTSIZE;
				Uint32 color = SDL_MapRGBA(mainsurface->format, 255, 255, 0, 127);
				drawBox(&pos, color, 127);
			}
		}
	}

	// draw contents of each slot
	x = INVENTORY_STARTX;
	y = INVENTORY_STARTY;
	for( node=stats[clientnum]->inventory.first; node!=NULL; node=nextnode ) {
		nextnode = node->next;
		Item *item = (Item *)node->element;

		if( item==selectedItem || (inventory_mode == INVENTORY_MODE_ITEM && itemCategory(item) == SPELL_CAT) || (inventory_mode == INVENTORY_MODE_SPELL && itemCategory(item) != SPELL_CAT))
			//Item is selected, or, item is a spell but it's item inventory mode, or, item is an item but it's spell inventory mode...(this filters out items)
			continue;

		// give it a yellow background if it is unidentified
		if (!item->identified) {
			pos.x = x + item->x*INVENTORY_SLOTSIZE + 2; pos.y = y + item->y*INVENTORY_SLOTSIZE + 1;
			pos.w = 38; pos.h = 38;
			drawRect(&pos, 31875, 125);
		} else if (item->beatitude < 0) { // give it a red background if cursed
			pos.x = x + item->x*INVENTORY_SLOTSIZE + 2; pos.y = y + item->y*INVENTORY_SLOTSIZE + 1;
			pos.w = 38; pos.h = 38;
			drawRect(&pos, 125, 125);
		} else if (item->beatitude > 0) { // give it a green background if blessed
			pos.x = x + item->x*INVENTORY_SLOTSIZE + 2; pos.y = y + item->y*INVENTORY_SLOTSIZE + 1;
			pos.w = 38; pos.h = 38;
			drawRect(&pos, 65280, 65);
		}

		// draw item
		pos.x = x+item->x*INVENTORY_SLOTSIZE+4; pos.y = y+item->y*INVENTORY_SLOTSIZE+4;
		pos.w = 32; pos.h = 32;
		if( itemSprite(item) )
			drawImageScaled(itemSprite(item), NULL, &pos);

		// item count
		if( item->count>1 )
			printTextFormatted(font8x8_bmp,pos.x+24,pos.y+24,"%d",item->count);

		// item equipped
		if( itemCategory(item)!=SPELL_CAT ) {
			if( itemIsEquipped(item,clientnum) ) {
				pos.x = x+item->x*INVENTORY_SLOTSIZE+2; pos.y = y+item->y*INVENTORY_SLOTSIZE+22;
				pos.w = 16; pos.h = 16;
				drawImage(equipped_bmp, NULL, &pos);
			}
		} else {
			spell_t *spell = getSpellFromItem(item);
			if( selected_spell == spell ) {
				pos.x = x+item->x*INVENTORY_SLOTSIZE+2; pos.y = y+item->y*INVENTORY_SLOTSIZE+22;
				pos.w = 16; pos.h = 16;
				drawImage(equipped_bmp, NULL, &pos);
			}
		}
	}

	// do inventory mode buttons
	mode_pos.x = x + INVENTORY_SIZEX*INVENTORY_SLOTSIZE;
	mode_pos.y = y+60;
	mode_pos.w = 0; mode_pos.h = 0;
	bool mouse_in_bounds = mouseInBounds(mode_pos.x, mode_pos.x + inventory_mode_spell_img->w, mode_pos.y, mode_pos.y + inventory_mode_spell_img->h);
	if (mouse_in_bounds) {
		drawImage(inventory_mode_spell_highlighted_img, NULL, &mode_pos);

		// tooltip
		SDL_Rect src;
		src.x = mousex+16; src.y = mousey+8;
		src.h = TTF12_HEIGHT+8;
		src.w = longestline(language[342])*TTF12_WIDTH+8;
		drawTooltip(&src);
		ttfPrintText(ttf12, src.x+4, src.y+4, language[342]);

		if (mousestatus[SDL_BUTTON_LEFT]) {
			mousestatus[SDL_BUTTON_LEFT] = 0;
			inventory_mode = INVENTORY_MODE_SPELL;
		}
	} else {
		drawImage(inventory_mode_spell_img, NULL, &mode_pos);
	}
	mode_pos.x = x + INVENTORY_SIZEX*INVENTORY_SLOTSIZE;
	mode_pos.y = y;
	mode_pos.w = 0; mode_pos.h = 0;
	mouse_in_bounds = mouseInBounds(mode_pos.x, mode_pos.x + inventory_mode_item_img->w, mode_pos.y, mode_pos.y + inventory_mode_item_img->h);
	if (mouse_in_bounds) {
		drawImage(inventory_mode_item_highlighted_img, NULL, &mode_pos);

		// tooltip
		SDL_Rect src;
		src.x = mousex+16; src.y = mousey+8;
		src.h = TTF12_HEIGHT+8;
		src.w = longestline(language[343])*TTF12_WIDTH+8;
		drawTooltip(&src);
		ttfPrintText(ttf12, src.x+4, src.y+4, language[343]);

		if (mousestatus[SDL_BUTTON_LEFT]) {
			mousestatus[SDL_BUTTON_LEFT] = 0;
			inventory_mode = INVENTORY_MODE_ITEM;
		}
	} else {
		drawImage(inventory_mode_item_img, NULL, &mode_pos);
	}

	// mouse interactions
	if( !selectedItem ) {
		for( node=stats[clientnum]->inventory.first; node!=NULL; node=nextnode ) {
			nextnode = node->next;
			Item *item = (Item *)node->element; //I don't like that there's not a check that either are null.

			if (item) {
				pos.x = x+item->x*INVENTORY_SLOTSIZE+4; pos.y = y+item->y*INVENTORY_SLOTSIZE+4;
				pos.w = 32; pos.h = 32;

				if( omousex>=pos.x && omousey>=pos.y && omousex<pos.x+pos.w && omousey<pos.y+pos.h ) {
					// tooltip
					if ((inventory_mode == INVENTORY_MODE_ITEM && itemCategory(item) == SPELL_CAT) || (inventory_mode == INVENTORY_MODE_SPELL && itemCategory(item) != SPELL_CAT))
						continue; //Skip over this items since the filter is blocking it (eg spell in normal inventory or vice versa).
					if( !itemMenuOpen ) {
						SDL_Rect src;
						src.x = mousex+16; src.y = mousey+8;
						if (itemCategory(item) == SPELL_CAT) {
							spell_t *spell = getSpellFromItem(item);
							if (spell) {
								char tempstr[32];
								snprintf(tempstr, 31, language[308], getCostOfSpell(spell));
								src.w = std::max(longestline(spell->name),longestline(tempstr))*TTF12_WIDTH+8;
								src.h = TTF12_HEIGHT*2+8;
								drawTooltip(&src);
								ttfPrintTextFormatted( ttf12, src.x+4, src.y+4, "%s\n%s", spell->name, tempstr);
							} else {
								src.w = longestline("Error: Spell doesn't exist!")*TTF12_WIDTH+8;
								src.h = TTF12_HEIGHT+8;
								drawTooltip(&src);
								ttfPrintText( ttf12, src.x+4, src.y+4, "Error: Spell doesn't exist!");
							}
						} else {
							src.w = std::max(13,longestline(item->description()))*TTF12_WIDTH+8;
							src.h = TTF12_HEIGHT*4+8;
							if( item->identified )
								if( itemCategory(item)==WEAPON || itemCategory(item)==ARMOR )
									src.h+=TTF12_HEIGHT;
							drawTooltip(&src);

							Uint32 color=0xFFFFFFFF;
							if( !item->identified ) {
								color = SDL_MapRGB(mainsurface->format,255,255,0);
								ttfPrintTextFormattedColor( ttf12, src.x+4+TTF12_WIDTH, src.y+4+TTF12_HEIGHT, color, language[309] );
							} else {
								if( item->beatitude<0 ) {
									color = SDL_MapRGB(mainsurface->format,255,0,0);
									ttfPrintTextFormattedColor( ttf12, src.x+4+TTF12_WIDTH, src.y+4+TTF12_HEIGHT, color, language[310] );
								} else if( item->beatitude==0 ) {
									color=0xFFFFFFFF;
									ttfPrintTextFormattedColor( ttf12, src.x+4+TTF12_WIDTH, src.y+4+TTF12_HEIGHT, color, language[311] );
								} else {
									color = SDL_MapRGB(mainsurface->format,0,255,0);
									ttfPrintTextFormattedColor( ttf12, src.x+4+TTF12_WIDTH, src.y+4+TTF12_HEIGHT, color, language[312] );
								}
							}
							if( item->beatitude==0 || !item->identified )
								color=0xFFFFFFFF;
							ttfPrintTextFormattedColor( ttf12, src.x+4, src.y+4, color, "%s", item->description());
							ttfPrintTextFormatted( ttf12, src.x+4+TTF12_WIDTH, src.y+4+TTF12_HEIGHT*2, language[313], items[item->type].weight*item->count);
							ttfPrintTextFormatted( ttf12, src.x+4+TTF12_WIDTH, src.y+4+TTF12_HEIGHT*3, language[314], item->sellValue(clientnum));

							if( item->identified ) {
								if( itemCategory(item)==WEAPON ) {
									if( item->weaponGetAttack()>=0 ) {
										color = SDL_MapRGB(mainsurface->format,0,255,255);
									} else {
										color = SDL_MapRGB(mainsurface->format,255,0,0);
									}
									ttfPrintTextFormattedColor( ttf12, src.x+4+TTF12_WIDTH, src.y+4+TTF12_HEIGHT*4, color, language[315], item->weaponGetAttack());
								} else if( itemCategory(item)==ARMOR ) {
									if( item->armorGetAC()>=0 ) {
										color = SDL_MapRGB(mainsurface->format,0,255,255);
									} else {
										color = SDL_MapRGB(mainsurface->format,255,0,0);
									}
									ttfPrintTextFormattedColor( ttf12, src.x+4+TTF12_WIDTH, src.y+4+TTF12_HEIGHT*4, color, language[316], item->armorGetAC());
								}
							}
						}
					}

					if( stats[clientnum]->HP<=0 )
						break;

					// handle clicking
					if( mousestatus[SDL_BUTTON_LEFT] && !selectedItem && !itemMenuOpen ) {
						selectedItem=item;
						playSound(139,64); // click sound
					} else if( mousestatus[SDL_BUTTON_RIGHT] && !itemMenuOpen && !selectedItem ) {
						if( keystatus[SDL_SCANCODE_LSHIFT] || keystatus[SDL_SCANCODE_RSHIFT] ) {
							// auto-appraise the item
							identifygui_active = FALSE;
							identifygui_appraising = TRUE;
							identifyGUIIdentify(item);
							mousestatus[SDL_BUTTON_RIGHT]=0;
						} else {
							// open a drop-down menu of options for "using" the item
							itemMenuOpen=TRUE;
							itemMenuX=mousex+8;
							itemMenuY=mousey;
							itemMenuSelected=0;
							itemMenuItem=item->uid;
						}
					}
					break;
				}
			}
		}
	} else if( stats[clientnum]->HP>0 ) {
		// releasing items
		if( (!mousestatus[SDL_BUTTON_LEFT] && !toggleclick) || (mousestatus[SDL_BUTTON_LEFT] && toggleclick) ) {
			if( openedChest[clientnum] && itemCategory(selectedItem) != SPELL_CAT ) {
				if( mousex>=CHEST_INVENTORY_X && mousey>=CHEST_INVENTORY_Y && mousex<CHEST_INVENTORY_X+inventoryChest_bmp->w && mousey<CHEST_INVENTORY_Y+inventoryChest_bmp->h ) {
					if( selectedItem->count>1 ) {
						openedChest[clientnum]->addItemToChestFromInventory(clientnum, selectedItem, FALSE);
						toggleclick=TRUE;
					} else {
						openedChest[clientnum]->addItemToChestFromInventory(clientnum, selectedItem, FALSE);
						selectedItem=NULL;
						toggleclick=FALSE;
					}
				}
			}
			if( selectedItem ) {
				if( mousex>=x && mousey>=y && mousex<x+INVENTORY_SIZEX*INVENTORY_SLOTSIZE && mousey<y+INVENTORY_SIZEY*INVENTORY_SLOTSIZE ) {
					// within inventory
					int oldx = selectedItem->x;
					int oldy = selectedItem->y;
					selectedItem->x = (mousex-x)/INVENTORY_SLOTSIZE;
					selectedItem->y = (mousey-y)/INVENTORY_SLOTSIZE;
					for( node=stats[clientnum]->inventory.first; node!=NULL; node=nextnode ) {
						nextnode = node->next;
						Item *tempItem = (Item *)node->element;
						if( tempItem == selectedItem )
							continue;

						toggleclick=FALSE;
						if( tempItem->x==selectedItem->x && tempItem->y==selectedItem->y ) {
							if (itemCategory(selectedItem)!=SPELL_CAT && itemCategory(tempItem) == SPELL_CAT) {
								//It's alright, the item can go here. The item sharing this x is just a spell, but the item being moved isn't a spell.
							} else if (itemCategory(selectedItem)==SPELL_CAT && itemCategory(tempItem) != SPELL_CAT) {
								//It's alright, the item can go here. The item sharing this x isn't a spell, but the item being moved is a spell.
							} else {
								//The player just dropped an item onto another item.
								tempItem->x = oldx;
								tempItem->y = oldy;
								selectedItem=tempItem;
								toggleclick=TRUE;
								break;
							}
						}
					}
					if( !toggleclick )
						selectedItem=NULL;
					playSound(139,64); // click sound
				} else if (itemCategory(selectedItem) == SPELL_CAT) {
					//Outside inventory. Spells can't be dropped.
					hotbar_slot_t *slot = getHotbar(mousex, mousey);
					if (slot) {
						//Add spell to hotbar.
						Item *tempItem = uidToItem(slot->item);
						if( tempItem ) {
							slot->item = selectedItem->uid;
							selectedItem = tempItem;
							toggleclick = TRUE;
						} else {
							slot->item = selectedItem->uid;
							selectedItem = NULL;
							toggleclick=FALSE;
						}
						playSound(139,64); // click sound
					} else {
						selectedItem = NULL;
					}
				} else {
					// outside inventory
					hotbar_slot_t *slot = getHotbar(mousex, mousey);
					if (slot) {
						//Add item to hotbar.
						Item *tempItem = uidToItem(slot->item);
						if( tempItem ) {
							slot->item = selectedItem->uid;
							selectedItem = tempItem;
							toggleclick = TRUE;
						} else {
							slot->item = selectedItem->uid;
							selectedItem = NULL;
							toggleclick=FALSE;
						}
						playSound(139,64); // click sound
					} else {
						if( selectedItem->count>1 ) {
							dropItem(selectedItem,clientnum);
							toggleclick=TRUE;
						} else {
							dropItem(selectedItem,clientnum);
							selectedItem=NULL;
							toggleclick=FALSE;
						}
					}
				}
			}
			if( mousestatus[SDL_BUTTON_LEFT] )
				mousestatus[SDL_BUTTON_LEFT] = 0;
		}
	}

	// item use menu
	if( itemMenuOpen ) {
		bool badpotion=FALSE;
		Item *currentItem = uidToItem(itemMenuItem);
		if( !currentItem )
			return;
		if( itemCategory(currentItem) == POTION && currentItem->identified ) {
			if( currentItem->type==POTION_SICKNESS || currentItem->type==POTION_CONFUSION || currentItem->type==POTION_BLINDNESS || currentItem->type==POTION_ACID || currentItem->type==POTION_PARALYSIS )
				badpotion=TRUE;
		}

		if( itemMenuSelected==0 )
			drawDepressed(itemMenuX,itemMenuY,itemMenuX+100,itemMenuY+20);
		else
			drawWindow(itemMenuX,itemMenuY,itemMenuX+100,itemMenuY+20);
		if (itemCategory(currentItem) != SPELL_CAT) {
			if( itemMenuSelected==1)
				drawDepressed(itemMenuX,itemMenuY+20,itemMenuX+100,itemMenuY+40);
			else
				drawWindow(itemMenuX,itemMenuY+20,itemMenuX+100,itemMenuY+40);
			if( itemMenuSelected==2 )
				drawDepressed(itemMenuX,itemMenuY+40,itemMenuX+100,itemMenuY+60);
			else
				drawWindow(itemMenuX,itemMenuY+40,itemMenuX+100,itemMenuY+60);
			if( itemCategory(currentItem)==POTION ) {
				if( itemMenuSelected==3 )
					drawDepressed(itemMenuX,itemMenuY+60,itemMenuX+100,itemMenuY+80);
				else
					drawWindow(itemMenuX,itemMenuY+60,itemMenuX+100,itemMenuY+80);
			}
		}

		if (itemCategory(currentItem) != SPELL_CAT) {
			if (openedChest[clientnum]) {
				int w;
				TTF_SizeUTF8(ttf12,language[344],&w,NULL);
				ttfPrintText(ttf12, itemMenuX+50-w/2, itemMenuY+4, language[344]);
			} else if( gui_mode == GUI_MODE_SHOP) {
				int w;
				TTF_SizeUTF8(ttf12,language[345],&w,NULL);
				ttfPrintText(ttf12, itemMenuX+50-w/2, itemMenuY+4, language[345]);
			} else {
				if( !badpotion ) {
					ttfPrintTextFormatted(ttf12, itemMenuX+50-strlen(itemUseString(currentItem))*TTF12_WIDTH/2, itemMenuY+4, "%s", itemUseString(currentItem));
				} else {
					if( itemIsEquipped(currentItem,clientnum) ) {
						int w;
						TTF_SizeUTF8(ttf12,language[323],&w,NULL);
						ttfPrintText(ttf12, itemMenuX+50-w/2, itemMenuY+4, language[323]);
					} else {
						int w;
						TTF_SizeUTF8(ttf12,language[324],&w,NULL);
						ttfPrintText(ttf12, itemMenuX+50-w/2, itemMenuY+4, language[324]);
					}
				}
			}
			if( itemCategory(currentItem) != POTION ) {
				int w;
				TTF_SizeUTF8(ttf12,language[1161],&w,NULL);
				ttfPrintText(ttf12, itemMenuX+50-w/2, itemMenuY+24, language[1161]);
				TTF_SizeUTF8(ttf12,language[1162],&w,NULL);
				ttfPrintText(ttf12, itemMenuX+50-w/2, itemMenuY+44, language[1162]);
			} else {
				if( !badpotion ) {
					if( itemIsEquipped(currentItem,clientnum) ) {
						int w;
						TTF_SizeUTF8(ttf12,language[323],&w,NULL);
						ttfPrintText(ttf12, itemMenuX+50-w/2, itemMenuY+24, language[323]);
					} else {
						int w;
						TTF_SizeUTF8(ttf12,language[324],&w,NULL);
						ttfPrintText(ttf12, itemMenuX+50-w/2, itemMenuY+24, language[324]);
					}
				} else {
					ttfPrintTextFormatted(ttf12, itemMenuX+50-strlen(itemUseString(currentItem))*TTF12_WIDTH/2, itemMenuY+24, "%s", itemUseString(currentItem));
				}
				int w;
				TTF_SizeUTF8(ttf12,language[1161],&w,NULL);
				ttfPrintText(ttf12, itemMenuX+50-w/2, itemMenuY+44, language[1161]);
				TTF_SizeUTF8(ttf12,language[1162],&w,NULL);
				ttfPrintText(ttf12, itemMenuX+50-w/2, itemMenuY+64, language[1162]);
			}
		} else {
			//Tis a spell.
			ttfPrintText(ttf12, itemMenuX+50-strlen(itemUseString(currentItem))*TTF12_WIDTH/2, itemMenuY+4, itemUseString(currentItem));
		}
		if( mousey<itemMenuY-20) {
			itemMenuSelected=-1; // for cancelling out
		}
		else if( mousey>=itemMenuY-2 && mousey<itemMenuY+20 ) {
			itemMenuSelected=0;
		}
		else if( mousey>=itemMenuY+20 && mousey<itemMenuY+40 ) {
			itemMenuSelected=1;
		}
		else if( mousey>=itemMenuY+40 && mousey<itemMenuY+60 ) {
			itemMenuSelected=2;
		}
		else if( itemCategory(currentItem)==POTION && mousey>=itemMenuY+60 && mousey<itemMenuY+80 ) {
			itemMenuSelected=3;
		}
		else {
			itemMenuSelected=-1; // for cancelling out
		}
		if( mousex>=itemMenuX+100 )
			itemMenuSelected=-1; // for cancelling out
		if( mousex<itemMenuX-10 || mousex<itemMenuX && settings_right_click_protect)
			itemMenuSelected=-1; // for cancelling out
		if( !mousestatus[SDL_BUTTON_RIGHT] ) {
			if( itemMenuSelected==0) {
				if (openedChest[clientnum] && itemCategory(currentItem) != SPELL_CAT) {
					openedChest[clientnum]->addItemToChestFromInventory(clientnum, currentItem, FALSE);
					currentItem = uidToItem(itemMenuItem);
				} else if( gui_mode == GUI_MODE_SHOP && itemCategory(currentItem) != SPELL_CAT ) {
					sellItemToShop(currentItem);
					currentItem = uidToItem(itemMenuItem);
				} else {
					if( !badpotion ) {
						useItem(currentItem,clientnum);
						currentItem = uidToItem(itemMenuItem);
					} else {
						if( multiplayer==CLIENT ) {
							strcpy((char *)net_packet->data,"EQUI");
							SDLNet_Write32((Uint32)currentItem->type,&net_packet->data[4]);
							SDLNet_Write32((Uint32)currentItem->status,&net_packet->data[8]);
							SDLNet_Write32((Uint32)currentItem->beatitude,&net_packet->data[12]);
							SDLNet_Write32((Uint32)currentItem->count,&net_packet->data[16]);
							SDLNet_Write32((Uint32)currentItem->appearance,&net_packet->data[20]);
							net_packet->data[24] = currentItem->identified;
							net_packet->data[25] = clientnum;
							net_packet->address.host = net_server.host;
							net_packet->address.port = net_server.port;
							net_packet->len = 26;
							sendPacketSafe(net_sock, -1, net_packet, 0);
						}
						equipItem(currentItem,&stats[clientnum]->weapon,clientnum);
						currentItem = uidToItem(itemMenuItem);
					}
				}
				itemMenuItem=0;
			}
			if( (currentItem=uidToItem(itemMenuItem))!=NULL && itemCategory(currentItem) != SPELL_CAT ) {
				if( itemCategory(currentItem) != POTION ) {
					if( itemMenuSelected==1 ) {
						identifygui_active = FALSE;
						identifygui_appraising = TRUE;
						identifyGUIIdentify(currentItem);
						currentItem = uidToItem(itemMenuItem);
					} else if( itemMenuSelected==2 ) {
						dropItem(currentItem,clientnum);
						currentItem = uidToItem(itemMenuItem);
					}
				} else {
					if( itemMenuSelected==1 ) {
						if( !badpotion ) {
							if( multiplayer==CLIENT ) {
								strcpy((char *)net_packet->data,"EQUI");
								SDLNet_Write32((Uint32)currentItem->type,&net_packet->data[4]);
								SDLNet_Write32((Uint32)currentItem->status,&net_packet->data[8]);
								SDLNet_Write32((Uint32)currentItem->beatitude,&net_packet->data[12]);
								SDLNet_Write32((Uint32)currentItem->count,&net_packet->data[16]);
								SDLNet_Write32((Uint32)currentItem->appearance,&net_packet->data[20]);
								net_packet->data[24] = currentItem->identified;
								net_packet->data[25] = clientnum;
								net_packet->address.host = net_server.host;
								net_packet->address.port = net_server.port;
								net_packet->len = 26;
								sendPacketSafe(net_sock, -1, net_packet, 0);
							}
							equipItem(currentItem,&stats[clientnum]->weapon,clientnum);
							currentItem = uidToItem(itemMenuItem);
						} else {
							useItem(currentItem,clientnum);
							currentItem = uidToItem(itemMenuItem);
						}
					} else if( itemMenuSelected==2 ) {
						identifygui_active = FALSE;
						identifygui_appraising = TRUE;
						identifyGUIIdentify(currentItem);
						currentItem = uidToItem(itemMenuItem);
					} else if( itemMenuSelected==3 ) {
						dropItem(currentItem,clientnum);
						currentItem = uidToItem(itemMenuItem);
					}
					itemMenuItem=0;
				}
			}
			itemMenuOpen=FALSE;
		}
	}
}