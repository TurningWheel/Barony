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

/*-------------------------------------------------------------------------------

	updateShopWindow
	
	Draws and processes everything related to the shop window

-------------------------------------------------------------------------------*/

void updateShopWindow() {
	SDL_Rect pos;
	node_t *node;
	int c;
	
	if( multiplayer != CLIENT ) {
		Entity *entity = uidToEntity(shopkeeper);
		if (entity)
		{
			Stat *stats = entity->getStats();
			shopkeepername = stats->name;
		}
	}
	
	// draw window
	int x1 = xres/2-SHOPWINDOW_SIZEX/2, x2 = xres/2+SHOPWINDOW_SIZEX/2;
	int y1 = yres/2-SHOPWINDOW_SIZEY/2, y2 = yres/2+SHOPWINDOW_SIZEY/2;
	drawWindowFancy( x1, y1, x2, y2 );
	
	// clicking
	int x = x1+(x2-x1)/2-inventory_bmp->w/2;
	int y = y1+16+160;
	pos.x = x; pos.y = y;
	drawImage(inventory_bmp, NULL, &pos);
	if( mousestatus[SDL_BUTTON_LEFT] ) {
		if( omousey>=y && omousey<y+16 ) {
			if( omousex>=x+12 && omousex < x + inventory_bmp->w - 12 ) {
				mousestatus[SDL_BUTTON_LEFT] = 0;
				shopinventorycategory=(omousex-x-12)/button_bmp->w;
			}
		}
		else if( omousey>=y+16 && omousey<y+52 ) {
			if( omousex>=x+inventory_bmp->w-28 && omousex<x+inventory_bmp->w-12 ) {
				mousestatus[SDL_BUTTON_LEFT] = 0;
				buttonclick=10;
				shopitemscroll--;
			}
		}
		else if( omousey>=y+52 && omousey<y+88 ) {
			if( omousex>=x+inventory_bmp->w-28 && omousex<x+inventory_bmp->w-12 ) {
				mousestatus[SDL_BUTTON_LEFT] = 0;
				buttonclick=11;
				shopitemscroll++;
			}
		}
	}
	
	// mousewheel
	if( omousex>=x+12 && omousex<x+inventory_bmp->w-28 ) {
		if( omousey>=16 && omousey<y+inventory_bmp->h-8 ) {
			if( mousestatus[SDL_BUTTON_WHEELDOWN] ) {
				mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
				shopitemscroll++;
			} else if( mousestatus[SDL_BUTTON_WHEELUP] ) {
				mousestatus[SDL_BUTTON_WHEELUP] = 0;
				shopitemscroll--;
			}
		}
	}

	// inventory up button
	if( buttonclick==10 ) {
		pos.x=x+inventory_bmp->w-28; pos.y=y+16;
		pos.w=0; pos.h=0;
		drawImage(invup_bmp, NULL, &pos);
	}
	// inventory down button
	if( buttonclick==11 ) {
		pos.x=x+inventory_bmp->w-28; pos.y=y+52;
		pos.w=0; pos.h=0;
		drawImage(invdown_bmp, NULL, &pos);
	}
	pos.x=x+12+button_bmp->w*shopinventorycategory; pos.y=y;
	pos.w=0; pos.h=0;
	if( shopinventorycategory <= 6 )
		drawImage(button_bmp, NULL, &pos);
	else
		drawImage(smallbutton_bmp, NULL, &pos);

	// inventory category labels
	ttfPrintText(ttf8,x+14,y+4,language[349]);
	ttfPrintText(ttf8,x+14+60,y+4,language[350]);
	ttfPrintText(ttf8,x+14+120,y+4,language[351]);
	ttfPrintText(ttf8,x+14+180,y+4,language[352]);
	ttfPrintText(ttf8,x+14+240,y+4,language[353]);
	ttfPrintText(ttf8,x+14+300,y+4,language[354]);
	ttfPrintText(ttf8,x+14+360,y+4,language[355]);
	ttfPrintText(ttf8,x+12+424,y+4,language[356]);

	// buying
	if (stats[clientnum]->HP > 0 && players[clientnum] != nullptr && players[clientnum]->entity != nullptr)
	{
		if (omousex >= x + 12 && omousex < x + inventory_bmp->w - 28)
		{
			pos.x = x + 12;
			pos.w = 0; pos.h = 0;
			if (omousey >= y + 16 && omousey < y + 34)
			{
				pos.y = y + 16;
				drawImage(inventoryoption_bmp, nullptr, &pos);
				if (mousestatus[SDL_BUTTON_LEFT])
				{
					if (stats[clientnum]->HP > 0 && players[clientnum] != nullptr && players[clientnum]->entity != nullptr)
					{
						buyItemFromShop(shopinvitems[0]);
					}
					mousestatus[SDL_BUTTON_LEFT] = 0;
				}
			}
			else if (omousey >= y + 34 && omousey < y + 52)
			{
				pos.y = y + 34;
				drawImage(inventoryoption_bmp, nullptr, &pos);
				if (mousestatus[SDL_BUTTON_LEFT])
				{
					if (stats[clientnum]->HP > 0 && players[clientnum] != nullptr && players[clientnum]->entity != nullptr)
					{
						buyItemFromShop(shopinvitems[1]);
					}
					mousestatus[SDL_BUTTON_LEFT] = 0;
				}
			}
			else if (omousey >= y + 52 && omousey < y + 70)
			{
				pos.y = y + 52;
				drawImage(inventoryoption_bmp, nullptr, &pos);
				if (mousestatus[SDL_BUTTON_LEFT])
				{
					if (stats[clientnum]->HP > 0 && players[clientnum] != nullptr && players[clientnum]->entity != nullptr)
					{
						buyItemFromShop(shopinvitems[2]);
					}
					mousestatus[SDL_BUTTON_LEFT] = 0;
				}
			}
			else if (omousey >= y + 70 && omousey < y + 88)
			{
				pos.y = y + 70;
				drawImage(inventoryoption_bmp, nullptr, &pos);
				if (mousestatus[SDL_BUTTON_LEFT])
				{
					if (stats[clientnum]->HP > 0 && players[clientnum] != nullptr && players[clientnum]->entity != nullptr)
					{
						buyItemFromShop(shopinvitems[3]);
					}
					mousestatus[SDL_BUTTON_LEFT] = 0;
				}
			}
		}
	}

	int y3=y+22; c=0;
	for( node=shopInv->first; node!=NULL; node=node->next ) {
		Item *item = (Item *) node->element;
		if( shopinventorycategory==0 && itemCategory(item)!=WEAPON )
			continue;
		else if( shopinventorycategory==1 && itemCategory(item)!=ARMOR )
			continue;
		else if( shopinventorycategory==2 && itemCategory(item)!=AMULET && itemCategory(item)!=RING )
			continue;
		else if( shopinventorycategory==3 && itemCategory(item)!=SPELLBOOK && itemCategory(item)!=MAGICSTAFF && itemCategory(item)!=SCROLL )
			continue;
		else if( shopinventorycategory==4 && itemCategory(item)!=GEM )
			continue;
		else if( shopinventorycategory==5 && itemCategory(item)!=FOOD && itemCategory(item)!=POTION )
			continue;
		else if( shopinventorycategory==6 && itemCategory(item)!=TOOL && itemCategory(item)!=BOOK )
			continue;
		c++;
	}
	shopitemscroll=std::max(0,std::min(shopitemscroll,c-4));
	for( c=0; c<4; c++ )
		shopinvitems[c]=NULL;
	c=0;
	for( node=shopInv->first; node!=NULL; node=node->next ) {
		Item *item = (Item *) node->element;
		if (item) {
			if( shopinventorycategory==0 && itemCategory(item)!=WEAPON )
				continue;
			else if( shopinventorycategory==1 && itemCategory(item)!=ARMOR )
				continue;
			else if( shopinventorycategory==2 && itemCategory(item)!=AMULET && itemCategory(item)!=RING )
				continue;
			else if( shopinventorycategory==3 && itemCategory(item)!=SPELLBOOK && itemCategory(item)!=MAGICSTAFF && itemCategory(item)!=SCROLL )
				continue;
			else if( shopinventorycategory==4 && itemCategory(item)!=GEM )
				continue;
			else if( shopinventorycategory==5 && itemCategory(item)!=FOOD && itemCategory(item)!=POTION )
				continue;
			else if( shopinventorycategory==6 && itemCategory(item)!=TOOL && itemCategory(item)!=BOOK )
				continue;
			c++;
			if( c<=shopitemscroll )
				continue;
			shopinvitems[c-shopitemscroll-1]=item;
			char tempstr[64] = { 0 };
			strncpy(tempstr,item->description(),42);
			if( strlen(tempstr)==42 )
				strcat(tempstr," ...");
			ttfPrintText(ttf8,x+12+36,y3,tempstr);
			ttfPrintTextFormatted(ttf8,x+12+348,y3,"%7dG",item->buyValue(clientnum));
			pos.x=x+12+16;
			pos.y=y+17+18*(c-shopitemscroll-1);
			pos.w=16; pos.h=16;
			drawImageScaled(itemSprite(item), NULL, &pos);
			y3+=18;
			if( c>3+shopitemscroll )
				break;
		}
	}
	
	// draw money count
	ttfPrintTextFormatted( ttf16, x1+16, y2-32, language[357], stats[clientnum]->GOLD );
	
	// chitchat
	if( (ticks-shoptimer)%600==0 ) {
		shopspeech = language[216+rand()%NUMCHITCHAT];
		shoptimer--;
	}
	
	// draw speech
	if (sellitem)
		ttfPrintTextFormatted( ttf16, x1+16+160+16, y1+32, shopspeech, sellitem->sellValue(clientnum) );
	else
		ttfPrintTextFormatted( ttf16, x1+16+160+16, y1+32, shopspeech, 0 );
	if( !strcmp(shopspeech,language[194]) || !strcmp(shopspeech,language[195]) || !strcmp(shopspeech,language[196]) ) {
		ttfPrintTextFormatted( ttf16, x1+16+160+16, y1+64, language[358], shopkeepername, language[184+shopkeepertype] );
	}
	
	// draw black box for shopkeeper
	pos.x = x1+16;
	pos.y = y1+16;
	pos.w = 160;
	pos.h = 160;
	drawRect(&pos,0,255);
	
	// draw shopkeeper
	if( uidToEntity(shopkeeper) ) {
		Entity *entity = uidToEntity(shopkeeper);
		if( !entity->flags[INVISIBLE] ) {
			pos.x = x1+16;
			pos.y = y1+16;
			pos.w = 160;
			pos.h = 160;
			drawImage(shopkeeper_bmp, NULL, &pos);
		} else {
			pos.x = x1+16;
			pos.y = y1+16;
			pos.w = 160;
			pos.h = 160;
			drawRect(&pos,0,255);
		}
	}
}