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
#include "../ui/Image.hpp"
#include "../ui/GameUI.hpp"

void Player::BookGUI_t::createBookGUI()
{
	if ( bookFrame )
	{
		return;
	}

	char name[32];
	snprintf(name, sizeof(name), "player book %d", player.playernum);
	Frame* frame = gameUIFrame[player.playernum]->addFrame(name);
	bookFrame = frame;
	frame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
		players[player.playernum]->camera_virtualy1(),
		players[player.playernum]->camera_virtualWidth(),
		players[player.playernum]->camera_virtualHeight() });
	frame->setHollow(false);
	frame->setBorder(0);
	frame->setOwner(player.playernum);
	frame->setInheritParentFrameOpacity(false);

	auto fade = bookFrame->addImage(
		SDL_Rect{ 0, 0, bookFrame->getSize().w, bookFrame->getSize().h }, 
		0, "images/system/white.png", "fade img");
	Uint8 r, g, b, a;
	getColor(fade->color, &r, &g, &b, &a);
	a = 0;
	fade->color = makeColor( r, g, b, a);

	Frame* bookBackground = frame->addFrame("book frame");
	const int width = 534;
	const int promptHeight = 27;
	const int promptWidth = 60;
	const int height = 306;
	bookBackground->setSize(SDL_Rect{ frame->getSize().x, frame->getSize().y, width, height + promptHeight * 2});
	auto bgImg = bookBackground->addImage(SDL_Rect{ 0, promptHeight, width, height }, 0xFFFFFFFF,
		"images/ui/Books/Book_00.png", "book img");

	auto prevPage = bookBackground->addFrame("prev page mouse boundary");
	prevPage->setSize(SDL_Rect{ 0, bgImg->pos.y, width / 2, height });
	auto nextPage = bookBackground->addFrame("next page mouse boundary");
	nextPage->setSize(SDL_Rect{ width / 2, bgImg->pos.y, width, height });

	std::string promptFont = "fonts/pixel_maz.ttf#32#2";
	auto promptBack = bookBackground->addField("prompt back txt", 16);
	promptBack->setSize(SDL_Rect{ bgImg->pos.x + bgImg->pos.w - promptWidth - 16, // lower right corner
		bgImg->pos.y + bgImg->pos.h, promptWidth, promptHeight });
	promptBack->setFont(promptFont.c_str());
	promptBack->setHJustify(Field::justify_t::RIGHT);
	promptBack->setVJustify(Field::justify_t::CENTER);
	promptBack->setText(language[4053]);
	promptBack->setColor(makeColor(201, 162, 100, 255));

	auto promptBackImg = bookBackground->addImage(SDL_Rect{0, 0, 0, 0}, 0xFFFFFFFF,
		"",	"prompt back img");
	promptBackImg->disabled = true;
	
	auto promptNextPage = bookBackground->addField("prompt next txt", 16);
	promptNextPage->setSize(SDL_Rect{ bgImg->pos.x + bgImg->pos.w - promptWidth - 16, // upper right corner
		0, promptWidth, promptHeight });
	promptNextPage->setFont(promptFont.c_str());
	promptNextPage->setHJustify(Field::justify_t::RIGHT);
	promptNextPage->setVJustify(Field::justify_t::CENTER);
	promptNextPage->setText(language[4054]);
	promptNextPage->setColor(makeColor(201, 162, 100, 255));

	auto promptNextPageImg = bookBackground->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF,
		"", "prompt next img");
	promptNextPageImg->disabled = true;

	auto promptPrevPage = bookBackground->addField("prompt prev txt", 16);
	promptPrevPage->setSize(SDL_Rect{ 16, // upper left corner
		0, promptWidth, promptHeight });
	promptPrevPage->setFont(promptFont.c_str());
	promptPrevPage->setHJustify(Field::justify_t::LEFT);
	promptPrevPage->setVJustify(Field::justify_t::CENTER);
	promptPrevPage->setText(language[4055]);
	promptPrevPage->setColor(makeColor(201, 162, 100, 255));

	auto promptPrevPageImg = bookBackground->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF,
		"", "prompt prev img");
	promptPrevPageImg->disabled = true;

	std::string bookFont = "fonts/pixel_maz.ttf#32";
	Field* bookLeftColumnText = bookBackground->addField("left column text", 1024);
	bookLeftColumnText->setText("Nothing");
	const int pageWidth = BOOK_PAGE_WIDTH;
	const int pageHeight = BOOK_PAGE_HEIGHT + 10;
	const int leftMargin = 30;
	const int topMargin = 10;
	bookLeftColumnText->setSize(SDL_Rect{ leftMargin, bgImg->pos.y + topMargin, pageWidth, pageHeight });
	bookLeftColumnText->setFont(bookFont.c_str());
	bookLeftColumnText->setHJustify(Field::justify_t::LEFT);
	bookLeftColumnText->setVJustify(Field::justify_t::TOP);
	bookLeftColumnText->setColor(makeColor( 0, 0, 0, 255));
	//bookLeftColumnText->setColor(makeColor(201, 162, 100, 255));

	//bookBackground->addImage(bookLeftColumnText->getSize(), 0xFFFFFFFF, "images/system/white.png", "debug img");

	Field* bookRightColumnText = bookBackground->addField("right column text", 1024);
	bookRightColumnText->setText("Nothing");
	bookRightColumnText->setSize(SDL_Rect{ width - pageWidth - leftMargin, bgImg->pos.y + topMargin, pageWidth, pageHeight });
	bookRightColumnText->setFont(bookFont.c_str());
	bookRightColumnText->setHJustify(Field::justify_t::LEFT);
	bookRightColumnText->setVJustify(Field::justify_t::TOP);
	bookRightColumnText->setColor(makeColor( 0, 0, 0, 255));
	//bookRightColumnText->setColor(makeColor( 67, 195, 157, 255));

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

	if ( !inputs.getVirtualMouse(player.playernum)->draw_cursor
		&& player.GUI.activeModule != player.GUI.MODULE_BOOK_VIEW )
	{
		bBookOpen = false;
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
		getColor(fade->color, &r, &g, &b, &a);
		a = 0;
		fade->color = makeColor( r, g, b, a);
		return;
	}

	bookFrame->setDisabled(false);
	bookFrame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
		players[player.playernum]->camera_virtualy1(),
		players[player.playernum]->camera_virtualWidth(),
		players[player.playernum]->camera_virtualHeight() });
	
	auto innerFrame = bookFrame->findFrame("book frame");
	SDL_Rect bookSize = innerFrame->getSize();
	bookSize.x = bookFrame->getSize().w / 2 - bookSize.w / 2;

	const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
	real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - bookFadeInAnimationY)) / 5.0;
	bookFadeInAnimationY += setpointDiffX;
	bookFadeInAnimationY = std::min(1.0, bookFadeInAnimationY);

	auto fade = bookFrame->findImage("fade img");
	Uint8 r, g, b, a;
	getColor(fade->color, &r, &g, &b, &a);
	a = 128 * bookFadeInAnimationY;
	fade->color = makeColor( r, g, b, a);

	int baseY = (bookFrame->getSize().h / 2 - bookSize.h / 2);
	bookSize.y = -bookSize.h + bookFadeInAnimationY * (baseY + bookSize.h);
	innerFrame->setSize(bookSize);

	bool drawGlyphs = ::inputs.getVirtualMouse(player.playernum)->lastMovementFromController;
	int numPages = allBooks[bookIndex].formattedPages.size();
	bool canAdvanceNextPage = (currentBookPage + 2 < numPages);
	bool canAdvancePrevPage = (currentBookPage - 2 >= 0);

	if ( auto promptBack = innerFrame->findField("prompt back txt") )
	{
		promptBack->setDisabled(!drawGlyphs);
		promptBack->setText(language[4053]);
		auto promptImg = innerFrame->findImage("prompt back img");
		promptImg->disabled = !drawGlyphs;
		SDL_Rect glyphPos = promptImg->pos;
		if ( auto textGet = Text::get(promptBack->getText(), promptBack->getFont(),
			makeColor(255, 255, 255, 255), makeColor(0, 0, 0, 255)) )
		{
			SDL_Rect textPos = promptBack->getSize();
			textPos.w = textGet->getWidth();
			textPos.x = bookSize.w - textPos.w - 16;
			promptBack->setSize(textPos);
			glyphPos.x = promptBack->getSize().x + promptBack->getSize().w - textGet->getWidth() - 4;
		}
		promptImg->path = Input::inputs[player.playernum].getGlyphPathForBinding("MenuCancel");
		Image* glyphImage = Image::get(promptImg->path.c_str());
		if ( glyphImage )
		{
			glyphPos.w = glyphImage->getWidth();
			glyphPos.h = glyphImage->getHeight();
			glyphPos.x -= glyphPos.w;
			glyphPos.y = promptBack->getSize().y + promptBack->getSize().h / 2 - glyphPos.h / 2;
			promptImg->pos = glyphPos;
		}
	}
	if ( auto promptNext = innerFrame->findField("prompt next txt") )
	{
		promptNext->setDisabled(!drawGlyphs || !canAdvanceNextPage);
		promptNext->setText(language[4054]);
		auto promptImg = innerFrame->findImage("prompt next img");
		promptImg->disabled = promptNext->isDisabled();
		SDL_Rect glyphPos = promptImg->pos;
		SDL_Rect textPos = promptNext->getSize();
		if ( auto textGet = Text::get(promptNext->getText(), promptNext->getFont(),
			makeColor(255, 255, 255, 255), makeColor(0, 0, 0, 255)) )
		{
			textPos.w = textGet->getWidth();
		}
		promptImg->path = Input::inputs[player.playernum].getGlyphPathForBinding("MenuPageRight");
		Image* glyphImage = Image::get(promptImg->path.c_str());
		if ( glyphImage )
		{
			glyphPos.w = glyphImage->getWidth();
			glyphPos.h = glyphImage->getHeight();
			glyphPos.x = bookSize.w - 16 - glyphPos.w;
			glyphPos.y = promptNext->getSize().y + promptNext->getSize().h / 2 - glyphPos.h / 2;
			promptImg->pos = glyphPos;
		}
		textPos.x = glyphPos.x - 4 - textPos.w;
		promptNext->setSize(textPos);
	}
	if ( auto promptPrev = innerFrame->findField("prompt prev txt") )
	{
		promptPrev->setDisabled(!drawGlyphs || !canAdvancePrevPage);
		promptPrev->setText(language[4055]);
		auto promptImg = innerFrame->findImage("prompt prev img");
		promptImg->disabled = promptPrev->isDisabled();
		SDL_Rect glyphPos = promptImg->pos;
		SDL_Rect textPos = promptPrev->getSize();
		if ( auto textGet = Text::get(promptPrev->getText(), promptPrev->getFont(),
			makeColor(255, 255, 255, 255), makeColor(0, 0, 0, 255)) )
		{
			textPos.w = textGet->getWidth();
		}
		promptImg->path = Input::inputs[player.playernum].getGlyphPathForBinding("MenuPageLeft");
		Image* glyphImage = Image::get(promptImg->path.c_str());
		if ( glyphImage )
		{
			glyphPos.w = glyphImage->getWidth();
			glyphPos.h = glyphImage->getHeight();
			glyphPos.x = 16;
			glyphPos.y = promptPrev->getSize().y + promptPrev->getSize().h / 2 - glyphPos.h / 2;
			promptImg->pos = glyphPos;
		}
		textPos.x = glyphPos.x + glyphPos.w + 4;
		promptPrev->setSize(textPos);
	}

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

	auto leftColumn = innerFrame->findField("left column text");
	auto rightColumn = innerFrame->findField("right column text");

	auto prevPageBoundary = innerFrame->findFrame("prev page mouse boundary");
	auto nextPageBoundary = innerFrame->findFrame("next page mouse boundary");

	// book gui
	if ( Input::inputs[player.playernum].binaryToggle("MenuLeftClick") )
	{
		//book_t GUI next page button.
		if ( nextPageBoundary->capturesMouse() )
		{
			Input::inputs[player.playernum].consumeBinaryToggle("MenuLeftClick");
			if ( canAdvanceNextPage )
			{
				canAdvanceNextPage = false;
				currentBookPage += 2;
				playSound(83 + rand() % 6, 128);
			}
		}
		else if ( prevPageBoundary->capturesMouse() )
		{
			Input::inputs[player.playernum].consumeBinaryToggle("MenuLeftClick");
			if ( canAdvancePrevPage )
			{
				canAdvancePrevPage = false;
				currentBookPage -= 2;
				playSound(83 + rand() % 6, 128);
			}
		}
		if ( !innerFrame->capturesMouse() )
		{
			Input::inputs[player.playernum].consumeBinaryToggle("MenuLeftClick");
			Input::inputs[player.playernum].consumeBindingsSharedWithBinding("MenuLeftClick");
			closeBookGUI();
			return;
		}
	}

	if ( player.GUI.activeModule == player.GUI.MODULE_BOOK_VIEW 
		&& player.bControlEnabled && !gamePaused
		&& !player.usingCommand() )
	{
		if ( Input::inputs[player.playernum].binaryToggle("MenuPageRight") )
		{
			Input::inputs[player.playernum].consumeBinaryToggle("MenuPageRight");
			if ( canAdvanceNextPage )
			{
				currentBookPage += 2;
				playSound(83 + rand() % 6, 128);
			}
		}
		if ( Input::inputs[player.playernum].binaryToggle("MenuPageLeft") )
		{
			Input::inputs[player.playernum].consumeBinaryToggle("MenuPageLeft");
			if ( canAdvancePrevPage )
			{
				currentBookPage -= 2;
				playSound(83 + rand() % 6, 128);
			}
		}
		if ( Input::inputs[player.playernum].binaryToggle("MenuCancel") )
		{
			Input::inputs[player.playernum].consumeBinaryToggle("MenuCancel");
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
	//	if ( auto getText = Text::get(leftColumn->getText(), leftColumn->getFont(),
	//		makeColor(255, 255, 255, 255), makeColor(0, 0, 0, 255)) )
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

	if ( bookFrame )
	{
		bookFrame->setDisabled(true);
	}

	player.GUI.returnToPreviousActiveModule();
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

	player.GUI.previousModule = player.GUI.activeModule;

	players[player.playernum]->openStatusScreen(GUI_MODE_INVENTORY, 
		INVENTORY_MODE_ITEM, player.GUI.MODULE_BOOK_VIEW); // Reset the GUI to the inventory.
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
