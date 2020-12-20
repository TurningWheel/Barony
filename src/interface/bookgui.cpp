/*-------------------------------------------------------------------------------

	BARONY
	File: bookgui.cpp
	Desc: contains book (GUI) related functions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../draw.hpp"
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

void Player::BookGUI_t::updateBookGUI()
{
	if ( !bBookOpen || !bookPageNode || !book )
	{
		bBookOpen = false;
		bookPageNode = nullptr;
		book = nullptr;
		draggingBookGUI = false;
		return;
	}

	SDL_Rect pos;

	//Center the book GUI.
	pos.x = getStartX();
	pos.y = getStartY();
	if (mouseInBounds(player.playernum, getStartX() + getBookWidth() - FLIPMARGIN, getStartX() + getBookWidth(),
		getStartY() + DRAGHEIGHT_BOOK, getStartY() + getBookHeight()) )
	{
		drawImage(book_highlighted_right_img, NULL, &pos);
	}
	else if (mouseInBounds(player.playernum, getStartX(), getStartX() + FLIPMARGIN,
		getStartY() + DRAGHEIGHT_BOOK, getStartY() + getBookHeight()) )
	{
		drawImage(book_highlighted_left_img, NULL, &pos);
	}
	else
	{
		drawImage(bookgui_img, NULL, &pos);
	}

	if ( inputs.bControllerInputPressed(player.playernum, INJOY_MENU_BOOK_NEXT) )
	{
		inputs.controllerClearInput(player.playernum, INJOY_MENU_BOOK_NEXT);
		if ( bookPageNode->next)
		{
			if ( bookPageNode->next->next)
			{
				bookPageNode = bookPageNode->next->next;
				playSound(83 + rand() % 6, 128);
			}
		}
	}

	if ( inputs.bControllerInputPressed(player.playernum, INJOY_MENU_BOOK_PREV) )
	{
		inputs.controllerClearInput(player.playernum, INJOY_MENU_BOOK_PREV);
		if ( bookPageNode->prev)
		{
			if ( bookPageNode->prev->prev)
			{
				bookPageNode = bookPageNode->prev->prev;
				playSound(83 + rand() % 6, 128);
			}
		}
	}

	if ( inputs.bControllerInputPressed(player.playernum, INJOY_MENU_CANCEL) )
	{
		inputs.controllerClearInput(player.playernum, INJOY_MENU_CANCEL);
		closeBookGUI();
	}

	// book gui
	if ( inputs.bMouseLeft(player.playernum) )
	{
		//book_t GUI next page button.
		if ( mouseInBounds(player.playernum, getStartX() + getBookWidth() - FLIPMARGIN, getStartX() + getBookWidth(),
			getStartY() + DRAGHEIGHT_BOOK, getStartY() + getBookHeight()) )
		{
			inputs.mouseClearLeft(player.playernum);
			if ( bookPageNode->next)
			{
				if ( bookPageNode->next->next)
				{
					bookPageNode = bookPageNode->next->next;
					playSound(83 + rand() % 6, 128);
				}
			}
		}
		else if ( mouseInBounds(player.playernum, getStartX(), getStartX() + FLIPMARGIN,
			getStartY() + DRAGHEIGHT_BOOK, getStartY() + getBookHeight()) )
		{
			inputs.mouseClearLeft(player.playernum);
			if ( bookPageNode->prev)
			{
				if ( bookPageNode->prev->prev)
				{
					bookPageNode = bookPageNode->prev->prev;
					playSound(83 + rand() % 6, 128);
				}
			}
		}
		if ( !mouseInBounds(player.playernum, getStartX(), getStartX() + getBookWidth(),
			getStartY(), getStartY() + getBookHeight()) )
		{
			// closing book
			closeBookGUI();
		}

		// 20/12/20 - disabling this for now. unnecessary
		if ( false )
		{
			if ( mouseInBounds(player.playernum, getStartX() + FLIPMARGIN, getStartX() + getBookWidth() - FLIPMARGIN,
				getStartY(), getStartY() + getBookHeight())
				|| mouseInBounds(player.playernum, getStartX(), getStartX() + getBookWidth(),
					getStartY(), getStartY() + DRAGHEIGHT_BOOK) )
			{
				// moving book
				gui_clickdrag[player.playernum] = true;
				draggingBookGUI = true;
				dragoffset_x[player.playernum] = inputs.getMouse(player.playernum, Inputs::MouseInputs::OX) - getStartX();
				dragoffset_y[player.playernum] = inputs.getMouse(player.playernum, Inputs::MouseInputs::OY) - getStartY();
				inputs.mouseClearLeft(player.playernum);
			}
		}
	}

	// render the book's text
	Uint32 color = SDL_MapRGBA(mainsurface->format, 0, 0, 0, 255);
	string_t* pagetext = (string_t*)bookPageNode->element;
	ttfPrintTextColor(BOOK_FONT, getStartX() + 44, getStartY() + 20, color, false, pagetext->data );
	if ( bookPageNode->next != NULL )
	{
		string_t* pagetext = (string_t*)bookPageNode->next->element;
		ttfPrintTextColor(BOOK_FONT, getStartX() + 316, getStartY() + 20, color, false, pagetext->data );
	}

	if ( draggingBookGUI )
	{
		if ( gui_clickdrag[player.playernum] )
		{
			offsetx = (inputs.getMouse(player.playernum, Inputs::MouseInputs::OX) - dragoffset_x[player.playernum]) - (getStartX() - offsetx);
			offsety = (inputs.getMouse(player.playernum, Inputs::MouseInputs::OY) - dragoffset_y[player.playernum]) - (getStartY() - offsety);
			if ( getStartX() <= player.camera_x1() )
			{
				offsetx = player.camera_x1() - (getStartX() - offsetx);
			}
			if ( getStartX() > player.camera_x2() - getBookWidth())
			{
				offsetx = (player.camera_x2() - getBookWidth()) - (getStartX() - offsetx);
			}
			if ( getStartY() <= player.camera_y1() )
			{
				offsety = player.camera_y1() - (getStartY() - offsety);
			}
			if ( getStartY() > player.camera_y2() - getBookHeight())
			{
				offsety = (player.camera_y2() - getBookHeight()) - (getStartY() - offsety);
			}
		}
		else
		{
			draggingBookGUI = false;
		}
	}
}

/*-------------------------------------------------------------------------------

	closeBookGUI

	closes any open book

-------------------------------------------------------------------------------*/

void Player::BookGUI_t::closeBookGUI()
{
	bBookOpen = false;
	/*if (book_page)
		free(book_page);*/
	draggingBookGUI = false;
	book = nullptr;
	openBookItem = nullptr;
}

/*-------------------------------------------------------------------------------

	openBook

	opens the given book

-------------------------------------------------------------------------------*/

void Player::BookGUI_t::openBook(book_t* bookToOpen, Item* item)
{
	if (!bookToOpen || !bookToOpen->pages.first)
	{
		return;
	}

	players[player.playernum]->openStatusScreen(GUI_MODE_INVENTORY, INVENTORY_MODE_ITEM); // Reset the GUI to the inventory.
	bookPageNode = bookToOpen->pages.first;
	bBookOpen = true;
	book = bookToOpen;

	openBookItem = item;

	// add the book to the list of read books
	bool hasreadbook = false;
	node_t* node;
	for ( node = booksRead.first; node != NULL; node = node->next )
	{
		if ( !strcmp(bookToOpen->name, (char*)node->element) )
		{
			hasreadbook = true;
			break;
		}
	}
	if ( !hasreadbook )
	{
		char* bookName = (char*) malloc(sizeof(char) * (strlen(bookToOpen->name) + 1));
		strcpy(bookName, bookToOpen->name);

		node = list_AddNodeFirst(&booksRead);
		node->element = bookName;
		node->size = sizeof(char) * (strlen(bookToOpen->name) + 1);
		node->deconstructor = &defaultDeconstructor;
	}

	// activate the steam achievement
	if ( list_Size(&booksRead) >= numbooks )
	{
		steamAchievement("BARONY_ACH_WELL_READ");
	}
}
