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
#include "../engine/audio/sound.hpp"
#include "../scores.hpp"
#include "../player.hpp"
#include "interface.hpp"
#include "../ui/Field.hpp"
#include "../ui/Text.hpp"

void Player::BookGUI_t::createBookGUI()
{
	if ( bookFrame )
	{
		return;
	}

	char name[32];
	snprintf(name, sizeof(name), "player book %d", player.playernum);
	Frame* frame = gui->addFrame(name);
	bookFrame = frame;
	frame->setSize(SDL_Rect{ players[player.playernum]->camera_x1(),
		players[player.playernum]->camera_y1(),
		players[player.playernum]->camera_width(),
		players[player.playernum]->camera_height() });
	frame->setActualSize(frame->getSize());
	frame->setHollow(true);
	frame->setBorder(0);
	frame->setOwner(player.playernum);
	frame->setInheritParentFrameOpacity(false);

	auto fade = bookFrame->addImage(
		SDL_Rect{ 0, 0, bookFrame->getSize().w, bookFrame->getSize().h }, 
		0, "images/system/white.png", "fade img");
	Uint8 r, g, b, a;
	SDL_GetRGBA(fade->color, mainsurface->format, &r, &g, &b, &a);
	a = 0;
	fade->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);

	Frame* bookBackground = frame->addFrame("book frame");
	const int width = 534;
	const int height = 306;
	bookBackground->setSize(SDL_Rect{ frame->getSize().x, frame->getSize().y, width, height });
	bookBackground->addImage(SDL_Rect{ 0, 0, width, height }, 0xFFFFFFFF,
		"images/ui/Books/Book_00.png", "book img");

	auto prevPage = bookBackground->addFrame("prev page mouse boundary");
	prevPage->setSize(SDL_Rect{ 0, 0, width / 2, height });
	auto nextPage = bookBackground->addFrame("next page mouse boundary");
	nextPage->setSize(SDL_Rect{ width / 2, 0, width, height });

	std::string bookFont = "fonts/pixel_maz.ttf#16";
	Field* bookLeftColumnText = bookBackground->addField("left column text", 1024);
	bookLeftColumnText->setText("Nothing");
	const int pageWidth = BOOK_PAGE_WIDTH;
	const int pageHeight = BOOK_PAGE_HEIGHT + 10;
	const int leftMargin = 30;
	const int topMargin = 10;
	bookLeftColumnText->setSize(SDL_Rect{ leftMargin, topMargin, pageWidth, pageHeight });
	bookLeftColumnText->setFont(bookFont.c_str());
	bookLeftColumnText->setHJustify(Field::justify_t::LEFT);
	bookLeftColumnText->setVJustify(Field::justify_t::TOP);
	bookLeftColumnText->setColor(SDL_MapRGBA(mainsurface->format, 0, 0, 0, 255));
	//bookLeftColumnText->setColor(SDL_MapRGBA(mainsurface->format, 67, 195, 157, 255));

	//bookBackground->addImage(bookLeftColumnText->getSize(), 0xFFFFFFFF, "images/system/white.png", "debug img");

	Field* bookRightColumnText = bookBackground->addField("right column text", 1024);
	bookRightColumnText->setText("Nothing");
	bookRightColumnText->setSize(SDL_Rect{ width - pageWidth - leftMargin, topMargin, pageWidth, pageHeight });
	bookRightColumnText->setFont(bookFont.c_str());
	bookRightColumnText->setHJustify(Field::justify_t::LEFT);
	bookRightColumnText->setVJustify(Field::justify_t::TOP);
	bookRightColumnText->setColor(SDL_MapRGBA(mainsurface->format, 0, 0, 0, 255));
	//bookRightColumnText->setColor(SDL_MapRGBA(mainsurface->format, 67, 195, 157, 255));

	//bookBackground->addImage(bookRightColumnText->getSize(), 0xFFFF00FF, "images/system/white.png", "debug img");
}

/*-------------------------------------------------------------------------------

	updateBookGUI

	Does all book gui processing

-------------------------------------------------------------------------------*/

void Player::BookGUI_t::updateBookGUI()
{
	if ( !bookFrame )
	{
		createBookGUI();
	}

	bool errorOpening = false;
	int bookIndex = getBook(openBookName);
	if ( !allBooks.empty() && allBooks.size() >= bookIndex )
	{
		if ( getBookNameFromIndex(bookIndex) != openBookName )
		{
			errorOpening = true;
		}
	}

	if ( !bBookOpen || allBooks.empty() || errorOpening )
	{
		bookFrame->setDisabled(true);
		bBookOpen = false;
		openBookName = "";

		auto innerFrame = bookFrame->findFrame("book frame");
		SDL_Rect pos = innerFrame->getSize();
		bookFadeInAnimationY = 0.0;
		pos.y = -pos.h;
		innerFrame->setSize(pos);

		auto fade = bookFrame->findImage("fade img");
		Uint8 r, g, b, a;
		SDL_GetRGBA(fade->color, mainsurface->format, &r, &g, &b, &a);
		a = 0;
		fade->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
		return;
	}

	bookFrame->setDisabled(false);
	bookFrame->setSize(SDL_Rect{ players[player.playernum]->camera_x1(),
		players[player.playernum]->camera_y1(),
		Frame::virtualScreenX,
		Frame::virtualScreenY });
	
	auto innerFrame = bookFrame->findFrame("book frame");
	SDL_Rect bookSize = innerFrame->getSize();
	bookSize.x = bookFrame->getSize().x + bookFrame->getSize().w / 2 - bookSize.w / 2;

	const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
	real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - bookFadeInAnimationY)) / 5.0;
	bookFadeInAnimationY += setpointDiffX;
	bookFadeInAnimationY = std::min(1.0, bookFadeInAnimationY);

	auto fade = bookFrame->findImage("fade img");
	Uint8 r, g, b, a;
	SDL_GetRGBA(fade->color, mainsurface->format, &r, &g, &b, &a);
	a = 128 * bookFadeInAnimationY;
	fade->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);

	int baseY = (bookFrame->getSize().y + bookFrame->getSize().h / 2 - bookSize.h / 2);
	bookSize.y = -baseY + 2 * bookFadeInAnimationY * (baseY);
	innerFrame->setSize(bookSize);

	//Center the book GUI.
	SDL_Rect pos;
	pos.x = getStartX();
	pos.y = getStartY();
	/*if (mouseInBounds(player.playernum, getStartX() + getBookWidth() - FLIPMARGIN, getStartX() + getBookWidth(),
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
	}*/

	int numPages = allBooks[bookIndex].formattedPages.size();

	if ( inputs.bControllerInputPressed(player.playernum, INJOY_MENU_BOOK_NEXT) )
	{
		inputs.controllerClearInput(player.playernum, INJOY_MENU_BOOK_NEXT);
		if ( currentBookPage + 2 < numPages )
		{
			currentBookPage += 2;
			playSound(83 + rand() % 6, 128);
		}
	}

	if ( inputs.bControllerInputPressed(player.playernum, INJOY_MENU_BOOK_PREV) )
	{
		inputs.controllerClearInput(player.playernum, INJOY_MENU_BOOK_PREV);
		if ( currentBookPage - 2 >= 0 )
		{
			currentBookPage -= 2;
			playSound(83 + rand() % 6, 128);
		}
	}

	if ( inputs.bControllerInputPressed(player.playernum, INJOY_MENU_CANCEL) )
	{
		inputs.controllerClearInput(player.playernum, INJOY_MENU_CANCEL);
		closeBookGUI();
	}

	auto leftColumn = innerFrame->findField("left column text");
	auto rightColumn = innerFrame->findField("right column text");

	auto prevPageBoundary = innerFrame->findFrame("prev page mouse boundary");
	auto nextPageBoundary = innerFrame->findFrame("next page mouse boundary");

	// book gui
	if ( inputs.bMouseLeft(player.playernum) )
	{
		//book_t GUI next page button.
		if ( nextPageBoundary->capturesMouse() )
		{
			inputs.mouseClearLeft(player.playernum);
			if ( currentBookPage + 2 < numPages )
			{
				currentBookPage += 2;
				playSound(83 + rand() % 6, 128);
			}
		}
		else if ( prevPageBoundary->capturesMouse() )
		{
			inputs.mouseClearLeft(player.playernum);
			if ( currentBookPage - 2 >= 0 )
			{
				currentBookPage -= 2;
				playSound(83 + rand() % 6, 128);
			}
		}
		if ( !innerFrame->capturesMouse() )
		{
			inputs.mouseClearLeft(player.playernum);
			// closing book
			closeBookGUI();
			return;
		}
	}

	if ( currentBookPage < numPages )
	{
		leftColumn->setText(allBooks[bookIndex].formattedPages[currentBookPage].c_str());
	}
	else
	{
		leftColumn->setText("");
	}
	if ( currentBookPage + 1 < numPages )
	{
		rightColumn->setText(allBooks[bookIndex].formattedPages[currentBookPage + 1].c_str());
	}
	else
	{
		rightColumn->setText("");
	}

	//for ( auto& book : allBooks )
	//{
	//	if ( book.name == openBookName )
	//	{
	//		if ( book.formattedPages.size() > 2 )
	//		{
	//			//Text::dumpCache();
	//			leftColumn->setText(newbook.formattedPages[0].c_str());
	//			//auto textGet = Text::get(leftColumn->getText(), leftColumn->getFont());
	//			rightColumn->setText(newbook.formattedPages[1].c_str());
	//		}
	//		break;
	//	}
	//}

	return;

	//std::string pageText = "";
	//if ( book && book->text )
	//{
	//	for ( int c = 0; book->text[c] != '\0'; ++c )
	//	{
	//		if ( book->text[c] == '\r' )
	//		{
	//			continue;
	//		}
	//		if ( book->text[c] == '\t' )
	//		{
	//			for ( int insertTabs = 3; insertTabs > 0; --insertTabs )
	//			{
	//				pageText += ' ';
	//			}
	//		}
	//		else
	//		{
	//			pageText += book->text[c];
	//		}
	//	}
	//}

	////string_t* pagetext = (string_t*)bookPageNode->element;
	//leftColumn->setText(pageText.c_str());
	//leftColumn->reflowTextToFit(0);


	//int len = strlen(leftColumn->getText());
	//char* reflowedText = (char*)malloc(len + 1);
	//memcpy(reflowedText, leftColumn->getText(), sizeof(char) * (len + 1));
	//reflowedText[len] = '\0';

	//pageText = "";
	//bool firstIteration = true;
	//std::vector<std::string> pages;
	//char* nexttoken = nullptr;
	//char* token = reflowedText;
	//do {
	//	nexttoken = Field::tokenize(token, "\n");
	//	if ( !pageText.empty() || (!strcmp(token, "") && !firstIteration) )
	//	{
	//		pageText.push_back('\n');
	//	}
	//	firstIteration = false;
	//	pageText += token;
	//	leftColumn->setText(pageText.c_str());
	//	if ( auto getText = Text::get(leftColumn->getText(), leftColumn->getFont()) )
	//	{
	//		int textHeight = getText->getHeight();
	//		if ( textHeight > leftColumn->getSize().h )
	//		{
	//			// exceeds size, move to next page.
	//			pages.push_back(pageText);
	//			pageText = "";
	//		}
	//	}
	//} while ( (token = nexttoken) != NULL );
	//pages.push_back(pageText);

	//if ( ticks % 250 > 120 )
	//{
	//	leftColumn->setText(pages[0].c_str());
	//	rightColumn->setText(pages[1].c_str());
	//}
	//else
	//{
	//	//leftColumn->setText(pages[2].c_str());
	//	rightColumn->setText("");
	//}
	//free(reflowedText);
	/*
	//int result = leftColumn->getLastLineThatFitsWithinHeight();
	if ( bookPageNode->next != NULL )
	{
		string_t* pagetext = (string_t*)bookPageNode->next->element;
	}
	rightColumn->setText(pages[1].c_str());*/
}

/*-------------------------------------------------------------------------------

	closeBookGUI

	closes any open book

-------------------------------------------------------------------------------*/

void Player::BookGUI_t::closeBookGUI()
{
	bBookOpen = false;
	openBookName = "";
	openBookItem = nullptr;
}

/*-------------------------------------------------------------------------------

	openBook

	opens the given book

-------------------------------------------------------------------------------*/

void Player::BookGUI_t::openBook(int index, Item* item)
{
	if ( getBookNameFromIndex(index) == "" )
	{
		return;
	}

	players[player.playernum]->openStatusScreen(GUI_MODE_INVENTORY, INVENTORY_MODE_ITEM); // Reset the GUI to the inventory.
	bBookOpen = true;
	openBookName = getBookNameFromIndex(index);
	openBookItem = item;
	currentBookPage = 0;

	// add the book to the list of read books
	bool hasreadbook = false;
	node_t* node;
	for ( node = booksRead.first; node != NULL; node = node->next )
	{
		if ( !strcmp(openBookName.c_str(), (char*)node->element) )
		{
			hasreadbook = true;
			break;
		}
	}
	if ( !hasreadbook )
	{
		char* bookName = (char*) malloc(sizeof(char) * (strlen(openBookName.c_str()) + 1));
		strcpy(bookName, openBookName.c_str());

		node = list_AddNodeFirst(&booksRead);
		node->element = bookName;
		node->size = sizeof(char) * (strlen(openBookName.c_str()) + 1);
		node->deconstructor = &defaultDeconstructor;
	}

	// activate the steam achievement
	if ( list_Size(&booksRead) >= numbooks )
	{
		steamAchievement("BARONY_ACH_WELL_READ");
	}
}
