/*-------------------------------------------------------------------------------

	BARONY
	File: bookgui.cpp
	Desc: contains book (GUI) related functions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../items.hpp"
#include "../book.hpp"
#include "../sound.hpp"
#include "../scores.hpp"
#include "../player.hpp"
#include "interface.hpp"

/*-------------------------------------------------------------------------------

	updateBookGUI
	
	Does all book gui processing

-------------------------------------------------------------------------------*/

void updateBookGUI() {
	SDL_Rect pos;

	if (book_open) {
		//Center the book GUI.
		pos.x = BOOK_GUI_X;
		pos.y = BOOK_GUI_Y;
		if (mouseInBounds(BOOK_GUI_X + bookgui_img->w - FLIPMARGIN, BOOK_GUI_X + bookgui_img->w, BOOK_GUI_Y + DRAGHEIGHT_BOOK, BOOK_GUI_Y + bookgui_img->h) )
			drawImage(book_highlighted_right_img, NULL, &pos);
		else if(mouseInBounds(BOOK_GUI_X, BOOK_GUI_X + FLIPMARGIN, BOOK_GUI_Y + DRAGHEIGHT_BOOK, BOOK_GUI_Y + bookgui_img->h) )
			drawImage(book_highlighted_left_img, NULL, &pos);
		else
			drawImage(bookgui_img, NULL, &pos);
	}

	// book gui
	if (book_open && book_page && open_book) {
		if( mousestatus[SDL_BUTTON_LEFT] ) {
			//book_t GUI next page button.
			if( mouseInBounds(BOOK_GUI_X+bookgui_img->w-FLIPMARGIN,BOOK_GUI_X+bookgui_img->w,BOOK_GUI_Y+DRAGHEIGHT_BOOK,BOOK_GUI_Y+bookgui_img->h) ) {
				mousestatus[SDL_BUTTON_LEFT] = 0;
				if (book_page->next) {
					if (book_page->next->next) {
						book_page = book_page->next->next;
						playSound(83+rand()%6,128);
					}
				}
			} else if( mouseInBounds(BOOK_GUI_X,BOOK_GUI_X+FLIPMARGIN,BOOK_GUI_Y+DRAGHEIGHT_BOOK,BOOK_GUI_Y+bookgui_img->h) ) {
				mousestatus[SDL_BUTTON_LEFT] = 0;
				if (book_page->prev) {
					if (book_page->prev->prev) {
						book_page = book_page->prev->prev;
						playSound(83+rand()%6,128);
					}
				}
			}
			if( !mouseInBounds(BOOK_GUI_X,BOOK_GUI_X+bookgui_img->w,BOOK_GUI_Y,BOOK_GUI_Y+bookgui_img->h) ) {
				// closing book
				closeBookGUI();
			}
			if( mouseInBounds(BOOK_GUI_X+FLIPMARGIN,BOOK_GUI_X+bookgui_img->w-FLIPMARGIN,BOOK_GUI_Y,BOOK_GUI_Y+bookgui_img->h) || mouseInBounds(BOOK_GUI_X,BOOK_GUI_X+bookgui_img->w,BOOK_GUI_Y,BOOK_GUI_Y+DRAGHEIGHT_BOOK) ) {
				// moving book
				gui_clickdrag = TRUE;
				dragging_book_GUI = TRUE;
				dragoffset_x = omousex - BOOK_GUI_X;
				dragoffset_y = omousey - BOOK_GUI_Y;
				mousestatus[SDL_BUTTON_LEFT] = 0;
			}
		}
		
		// render the book's text
		Uint32 color = SDL_MapRGBA(mainsurface->format,0,0,0,255);
		string_t *pagetext = (string_t *)book_page->element;
		ttfPrintTextColor(BOOK_FONT, BOOK_GUI_X + 44, BOOK_GUI_Y + 20, color, FALSE, pagetext->data );
		if( book_page->next != NULL ) {
			string_t *pagetext = (string_t *)book_page->next->element;
			ttfPrintTextColor(BOOK_FONT, BOOK_GUI_X + 316, BOOK_GUI_Y + 20, color, FALSE, pagetext->data );
		}
		
		if (dragging_book_GUI) {
			if (gui_clickdrag) {
				bookgui_offset_x = (omousex - dragoffset_x) - (BOOK_GUI_X - bookgui_offset_x);
				bookgui_offset_y = (omousey - dragoffset_y) - (BOOK_GUI_Y - bookgui_offset_y);
				if (BOOK_GUI_X <= camera.winx)
					bookgui_offset_x = camera.winx - (BOOK_GUI_X - bookgui_offset_x);
				if (BOOK_GUI_X > camera.winx + camera.winw - bookgui_img->w)
					bookgui_offset_x = (camera.winx + camera.winw - bookgui_img->w) - (BOOK_GUI_X - bookgui_offset_x);
				if (BOOK_GUI_Y <= camera.winy)
					bookgui_offset_y = camera.winy - (BOOK_GUI_Y - bookgui_offset_y);
				if (BOOK_GUI_Y > camera.winy + camera.winh - bookgui_img->h)
					bookgui_offset_y = (camera.winy + camera.winh - bookgui_img->h) - (BOOK_GUI_Y - bookgui_offset_y);
			} else {
				dragging_book_GUI = FALSE;
			}
		}
	}
}

/*-------------------------------------------------------------------------------

	closeBookGUI

	closes any open book

-------------------------------------------------------------------------------*/

void closeBookGUI() {
	book_open = FALSE;
	/*if (book_page)
		free(book_page);*/
	dragging_book_GUI = FALSE;
	open_book = NULL;
	open_book_item = NULL;
}

/*-------------------------------------------------------------------------------

	openBook
	
	opens the given book

-------------------------------------------------------------------------------*/

void openBook(book_t *book, Item *item) {
	if (!book || !book->pages.first)
		return;

	gui_mode = GUI_MODE_INVENTORY; //Yes, this is pretty much the main GUI screen.
	shootmode = FALSE;
	book_page = book->pages.first;
	book_open = TRUE;
	open_book = book;

	open_book_item = item;

	// add the book to the list of read books
	bool hasreadbook=FALSE;
	node_t *node;
	for( node=booksRead.first; node!=NULL; node=node->next ) {
		if( !strcmp(book->name,(char *)node->element) ) {
			hasreadbook=TRUE;
			break;
		}
	}
	if( !hasreadbook ) {
		char *bookName = (char *) malloc(sizeof(char)*(strlen(book->name)+1));
		strcpy(bookName,book->name);

		node = list_AddNodeFirst(&booksRead);
		node->element = bookName;
		node->size = sizeof(char)*(strlen(book->name)+1);
		node->deconstructor = &defaultDeconstructor;
	}

	// activate the steam achievement
	if( list_Size(&booksRead)>=numbooks )
		steamAchievement("BARONY_ACH_WELL_READ");
}