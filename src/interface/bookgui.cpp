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
#include "../prng.hpp"
#include "../mod_tools.hpp"
#include "../ui/MainMenu.hpp"

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
	promptBack->setText(Language::get(4053));
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
	promptNextPage->setText(Language::get(4054));
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
	promptPrevPage->setText(Language::get(4055));
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

	bookFrame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
		players[player.playernum]->camera_virtualy1(),
		players[player.playernum]->camera_virtualWidth(),
		players[player.playernum]->camera_virtualHeight() });

	bool errorOpening = false;
	int bookIndex = getBook(openBookName);
	if ( !allBooks.empty() && allBooks.size() >= bookIndex )
	{
		if ( getBookDefaultNameFromIndex(bookIndex) != openBookName )
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

	const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
	real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - bookFadeInAnimationY)) / 5.0;
	bookFadeInAnimationY += setpointDiffX;
	bookFadeInAnimationY = std::min(1.0, bookFadeInAnimationY);

	auto fade = bookFrame->findImage("fade img");
	Uint8 r, g, b, a;
	getColor(fade->color, &r, &g, &b, &a);
	a = 128 * bookFadeInAnimationY;
	fade->color = makeColor( r, g, b, a);
	fade->pos = SDL_Rect{ 0, 0, bookFrame->getSize().w, bookFrame->getSize().h };

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
		promptBack->setText(Language::get(4053));
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
		promptNext->setText(Language::get(4054));
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
		promptPrev->setText(Language::get(4055));
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

	auto leftColumn = innerFrame->findField("left column text");
	auto rightColumn = innerFrame->findField("right column text");

	auto prevPageBoundary = innerFrame->findFrame("prev page mouse boundary");
	auto nextPageBoundary = innerFrame->findFrame("next page mouse boundary");

	// book gui
	if ( Input::inputs[player.playernum].binaryToggle("MenuLeftClick")
		&& inputs.bPlayerUsingKeyboardControl(player.playernum) )
	{
		//book_t GUI next page button.
		if ( nextPageBoundary->capturesMouse() )
		{
			Input::inputs[player.playernum].consumeBinaryToggle("MenuLeftClick");
			if ( canAdvanceNextPage )
			{
				canAdvanceNextPage = false;
				currentBookPage += 2;
				playSound(83 + local_rng.rand() % 6, 156);
			}
		}
		else if ( prevPageBoundary->capturesMouse() )
		{
			Input::inputs[player.playernum].consumeBinaryToggle("MenuLeftClick");
			if ( canAdvancePrevPage )
			{
				canAdvancePrevPage = false;
				currentBookPage -= 2;
				playSound(83 + local_rng.rand() % 6, 156);
			}
		}
		if ( !innerFrame->capturesMouse() )
		{
			Input::inputs[player.playernum].consumeBinaryToggle("MenuLeftClick");
			Input::inputs[player.playernum].consumeBindingsSharedWithBinding("MenuLeftClick");
			if ( bBookOpen )
			{
				Player::soundCancel();
			}
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
				playSound(83 + local_rng.rand() % 6, 156);
			}
		}
		if ( Input::inputs[player.playernum].binaryToggle("MenuPageLeft") )
		{
			Input::inputs[player.playernum].consumeBinaryToggle("MenuPageLeft");
			if ( canAdvancePrevPage )
			{
				currentBookPage -= 2;
				playSound(83 + local_rng.rand() % 6, 156);
			}
		}
		if ( Input::inputs[player.playernum].binaryToggle("MenuCancel") )
		{
			Input::inputs[player.playernum].consumeBinaryToggle("MenuCancel");
			if ( bBookOpen )
			{
				Player::soundCancel();
			}
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
	if ( getBookDefaultNameFromIndex(index) == "" )
	{
		return;
	}

	for (int c = 0; c < num_banned_books; ++c) {
		const char* banned_book = banned_books[c];
		if ( !spawn_blood && getBookDefaultNameFromIndex(index, false) == banned_book )
		{
			openBook((index + 1) % numbooks, item);
			return;
		}
	}

	player.GUI.previousModule = player.GUI.activeModule;

	players[player.playernum]->openStatusScreen(GUI_MODE_INVENTORY, 
		INVENTORY_MODE_ITEM, player.GUI.MODULE_BOOK_VIEW); // Reset the GUI to the inventory.
	bBookOpen = true;
	openBookName = getBookDefaultNameFromIndex(index);
	openBookItem = item;
	currentBookPage = 0;

	playSound(83 + local_rng.rand() % 6, 156);

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

	Compendium_t::Events_t::eventUpdate(player.playernum, Compendium_t::CPDM_LORE_READ, READABLE_BOOK, 1);
	if ( numbooks > 0 )
	{
		if ( index % numbooks >= 32 )
		{
			Compendium_t::Events_t::eventUpdate(player.playernum, Compendium_t::CPDM_LORE_PERCENT_READ_2, READABLE_BOOK, (1 << ((index % numbooks) - 32)));
		}
		else
		{
			Compendium_t::Events_t::eventUpdate(player.playernum, Compendium_t::CPDM_LORE_PERCENT_READ, READABLE_BOOK, (1 << (index % numbooks)));
		}
	}

	// activate the steam achievement
	if ( list_Size(&booksRead) >= numbooks )
	{
		steamAchievement("BARONY_ACH_WELL_READ");
	}
}

void Player::SignGUI_t::openSign(std::string name, Uint32 uid)
{
	if ( ScriptTextParser.allEntries.find(name) == ScriptTextParser.allEntries.end() )
	{
		closeSignGUI();
		return;
	}

	if ( inputs.hasController(player.playernum) )
	{
		std::string gamepadName = name;
		gamepadName += "_gamepad";
		if ( ScriptTextParser.allEntries.find(gamepadName) != ScriptTextParser.allEntries.end() )
		{
			name = gamepadName;
		}
	}

	player.GUI.previousModule = player.GUI.activeModule;
	player.closeAllGUIs(CloseGUIShootmode::CLOSEGUI_ENABLE_SHOOTMODE, CloseGUIIgnore::CLOSEGUI_CLOSE_ALL);
	player.openStatusScreen(GUI_MODE_SIGN,
		INVENTORY_MODE_ITEM, player.GUI.MODULE_SIGN_VIEW); // Reset the GUI to the inventory.
	bSignOpen = true;
	signName = name;
	signUID = uid;

	if ( Entity* entity = uidToEntity(uid) )
	{
		signWorldCoordX = entity->x;
		signWorldCoordY = entity->y;
	}

	// fix for binding left click to open sign
	{
		Input::inputs[player.playernum].consumeBinaryToggle("MenuLeftClick");
		Input::inputs[player.playernum].consumeBindingsSharedWithBinding("MenuLeftClick");
	}
}

void Player::SignGUI_t::closeSignGUI()
{
	bool wasOpen = bSignOpen;
#ifdef USE_THEORA_VIDEO
	VideoManager[player.playernum].stop();
#endif
	bSignOpen = false;
	signName = "";
	signUID = 0;
	signAnimVideo = 0.0;
	if ( signFrame )
	{
		signFrame->setDisabled(true);
	}
	signWorldCoordX = 0.0;
	signWorldCoordY = 0.0;
	//player.GUI.returnToPreviousActiveModule();
	if ( wasOpen )
	{
		player.closeAllGUIs(CloseGUIShootmode::CLOSEGUI_ENABLE_SHOOTMODE, CloseGUIIgnore::CLOSEGUI_CLOSE_ALL);
	}
}

void Player::SignGUI_t::createSignGUI()
{
	if ( signFrame )
	{
		return;
	}

	char name[32];
	snprintf(name, sizeof(name), "player sign %d", player.playernum);
	Frame* frame = gameUIFrame[player.playernum]->addFrame(name);
	signFrame = frame;
	frame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
		players[player.playernum]->camera_virtualy1(),
		players[player.playernum]->camera_virtualWidth(),
		players[player.playernum]->camera_virtualHeight() });
	frame->setHollow(false);
	frame->setBorder(0);
	frame->setOwner(player.playernum);
	frame->setInheritParentFrameOpacity(false);

	auto fade = frame->addImage(
		SDL_Rect{ 0, 0, frame->getSize().w, frame->getSize().h },
		0, "images/system/white.png", "fade img");
	Uint8 r, g, b, a;
	getColor(fade->color, &r, &g, &b, &a);
	a = 0;
	fade->color = makeColor(r, g, b, a);

	Frame* videoFrame = frame->addFrame("video frame");

	Frame* signBackground = frame->addFrame("sign frame");
	const int width = 366;
	const int promptHeight = 27;
	const int promptWidth = 60;
	const int height = 282;
	signBackground->setSize(SDL_Rect{ frame->getSize().x, frame->getSize().y, width, height + promptHeight * 2 });
	auto bgImg = signBackground->addImage(SDL_Rect{ 0, promptHeight, width, height }, 0xFFFFFFFF,
		"#*images/ui/Signs/UI_Sign_Window_00.png", "sign img");

	std::string promptFont = "fonts/pixel_maz.ttf#32#2";
	auto promptBack = signBackground->addField("prompt back txt", 16);
	promptBack->setSize(SDL_Rect{ bgImg->pos.x + bgImg->pos.w - promptWidth - 16, // lower right corner
		bgImg->pos.y + bgImg->pos.h, promptWidth, promptHeight });
	promptBack->setFont(promptFont.c_str());
	promptBack->setHJustify(Field::justify_t::RIGHT);
	promptBack->setVJustify(Field::justify_t::CENTER);
	promptBack->setText(Language::get(4053));
	promptBack->setColor(makeColor(201, 162, 100, 255));

	auto promptBackImg = signBackground->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF,
		"", "prompt back img");
	promptBackImg->disabled = true;

	std::string signFont = "fonts/pixel_maz_multiline.ttf#16#2";
	for ( int i = 1; i <= 10; ++i )
	{
		char fieldBuf[32] = "";
		char imgBuf[32] = "";
		snprintf(fieldBuf, sizeof(fieldBuf), "text %d", i);
		snprintf(imgBuf, sizeof(imgBuf), "img %d", i);
		Field* signText = signBackground->addField(fieldBuf, 1024);
		signText->setText("");
		const int leftMargin = 16;
		const int pageWidth = width - leftMargin * 2;
		const int pageHeight = 24;
		const int topMargin = 10;
		signText->setSize(SDL_Rect{ leftMargin, bgImg->pos.y + topMargin, pageWidth, pageHeight });
		signText->setFont(signFont.c_str());
		signText->setHJustify(Field::justify_t::LEFT);
		signText->setVJustify(Field::justify_t::TOP);
		signText->setTextColor(makeColor(255, 255, 255, 255));
		signText->setDisabled(true);

		Frame::image_t* signImg = signBackground->addImage(SDL_Rect{ 0, 0, 0, 0 },
			0xFFFFFFFF, "", imgBuf);
		signImg->disabled = true;
	}

	videoFrame->setDisabled(true);
	videoFrame->setSize(SDL_Rect{ 0, 0, signBackground->getSize().w, signBackground->getSize().h });
	auto videoImg = videoFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "", "video bg");
	videoFrame->setInheritParentFrameOpacity(false);
	auto videoEmbed = videoFrame->addFrame("video");
	videoEmbed->setSize(SDL_Rect{ 0, 0, 0, 0 });
#ifdef USE_THEORA_VIDEO
	videoEmbed->setDrawCallback([](const Widget& widget, SDL_Rect rect) {
		if ( widget.getOwner() < 0 )
		{
			return;
		}
		if ( ScriptTextParser.allEntries.find(players[widget.getOwner()]->signGUI.signName)
			!= ScriptTextParser.allEntries.end() )
		{
			auto& signEntry = ScriptTextParser.allEntries[players[widget.getOwner()]->signGUI.signName];
			const Frame* f = static_cast<const Frame*>(&widget);
			VideoManager[widget.getOwner()].drawAsFrameCallback(widget, rect, signEntry.signVideoContent.pos, f->getOpacity() / 100.0);
		}
	});
#endif // USE_THEORA_VIDEO
}

void Player::SignGUI_t::updateSignGUI()
{
	if ( !signFrame )
	{
		createSignGUI();
	}

	bool errorOpening = false;
	if ( signName == "" || ScriptTextParser.allEntries.find(signName) == ScriptTextParser.allEntries.end() )
	{
		errorOpening = true;
	}

	if ( player.GUI.activeModule != player.GUI.MODULE_SIGN_VIEW || player.gui_mode != GUI_MODE_SIGN )
	{
		bSignOpen = false;
	}
	else if ( player.entity )
	{
		real_t dist = sqrt(pow(signWorldCoordX - player.entity->x, 2) + pow(signWorldCoordY - player.entity->y, 2));
		if ( dist > TOUCHRANGE )
		{
			errorOpening = true;
		}
	}

	if ( !bSignOpen || errorOpening || !player.isLocalPlayerAlive() )
	{
		auto innerFrame = signFrame->findFrame("sign frame");
		SDL_Rect pos = innerFrame->getSize();
		signFadeInAnimationY = 0.0;
		pos.y = -pos.h;
		innerFrame->setSize(pos);

		auto fade = signFrame->findImage("fade img");
		Uint8 r, g, b, a;
		getColor(fade->color, &r, &g, &b, &a);
		a = 0;
		fade->color = makeColor(r, g, b, a);

		closeSignGUI();
		return;
	}

	signFrame->setDisabled(false);
	signFrame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
		players[player.playernum]->camera_virtualy1(),
		players[player.playernum]->camera_virtualWidth(),
		players[player.playernum]->camera_virtualHeight() });

	auto innerFrame = signFrame->findFrame("sign frame");
	SDL_Rect bookSize = innerFrame->getSize();
	bookSize.x = signFrame->getSize().w / 2 - bookSize.w / 2;

	const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
	real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - signFadeInAnimationY)) / 5.0;
	signFadeInAnimationY += setpointDiffX;
	signFadeInAnimationY = std::min(1.0, signFadeInAnimationY);

	auto fade = signFrame->findImage("fade img");
	Uint8 r, g, b, a;
	getColor(fade->color, &r, &g, &b, &a);
	a = 128 * signFadeInAnimationY;
	fade->color = makeColor(r, g, b, a);

	int baseY = (signFrame->getSize().h / 2 - bookSize.h / 2);
	bookSize.y = -bookSize.h + signFadeInAnimationY * (baseY + bookSize.h);
	innerFrame->setSize(bookSize);

	bool drawGlyphs = ::inputs.getVirtualMouse(player.playernum)->lastMovementFromController;

	if ( auto promptBack = innerFrame->findField("prompt back txt") )
	{
		promptBack->setDisabled(!drawGlyphs);
		promptBack->setText(Language::get(4053));
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

	// book gui
	if ( Input::inputs[player.playernum].binaryToggle("MenuLeftClick") )
	{
		if ( !innerFrame->capturesMouse() )
		{
			Input::inputs[player.playernum].consumeBinaryToggle("MenuLeftClick");
			Input::inputs[player.playernum].consumeBindingsSharedWithBinding("MenuLeftClick");
			closeSignGUI();
			return;
		}
	}

	if ( player.GUI.activeModule == player.GUI.MODULE_SIGN_VIEW
		&& player.bControlEnabled && !gamePaused
		&& !player.usingCommand() )
	{
		if ( Input::inputs[player.playernum].binaryToggle("MenuCancel") )
		{
			Input::inputs[player.playernum].consumeBinaryToggle("MenuCancel");
			Input::inputs[player.playernum].consumeBindingsSharedWithBinding("MenuCancel");
			closeSignGUI();
			return;
		}
	}

	auto& signEntry = ScriptTextParser.allEntries[signName];

	auto videoFrame = signFrame->findFrame("video frame");
	videoFrame->setDisabled(true);
	if ( signFadeInAnimationY >= 0.99 && signEntry.signVideoContent.path != "" )
	{
#ifdef USE_THEORA_VIDEO
		if ( !VideoManager[player.playernum].isPlaying(signEntry.signVideoContent.path.c_str()) )
		{
			VideoManager[player.playernum].loadfile(signEntry.signVideoContent.path.c_str());
		}
		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
		real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - signAnimVideo)) / 5.0;
		signAnimVideo += setpointDiffX;
		signAnimVideo = std::min(1.0, signAnimVideo);
		videoFrame->setDisabled(false);
		videoFrame->setOpacity(signAnimVideo * 100.0);
		SDL_Rect pos = innerFrame->getSize();
		pos.x = pos.x + signAnimVideo * pos.w;
		videoFrame->setSize(pos);
		auto videoImg = videoFrame->findImage("video bg");
		videoImg->path = signEntry.signVideoContent.bgPath;
		if ( auto imgGet = Image::get(videoImg->path.c_str()) )
		{
			videoImg->pos.w = imgGet->getWidth();
			videoImg->pos.h = imgGet->getHeight();
			videoImg->pos.x = 0;// videoFrame->getSize().w / 2 - videoImg->pos.w / 2;
			videoImg->pos.y = videoFrame->getSize().h / 2 - videoImg->pos.h / 2;
		}
		else
		{
			videoImg->pos.w = pos.w;
			videoImg->pos.h = pos.h;
			videoImg->pos.y = videoFrame->getSize().h / 2 - videoImg->pos.h / 2;
		}
		videoImg->color = makeColor(255, 255, 255, 255 * signAnimVideo);

		Frame* videoEmbed = videoFrame->findFrame("video");
		const int border = signEntry.signVideoContent.imgBorder;
		videoEmbed->setSize(SDL_Rect{ videoImg->pos.x + border, videoImg->pos.y + border, 
			videoImg->pos.w - 2 * border, videoImg->pos.h - 2 * border });
#else
		signAnimVideo = 0.0;
#endif
	}
	else
	{
		signAnimVideo = 0.0;
	}

	std::vector<Field*> allFields;
	std::vector<Frame::image_t*> allImgs;
	for ( int i = 1; i <= 10; ++i )
	{
		char fieldBuf[32] = "";
		char imgBuf[32] = "";
		snprintf(fieldBuf, sizeof(fieldBuf), "text %d", i);
		snprintf(imgBuf, sizeof(imgBuf), "img %d", i);
		auto text = innerFrame->findField(fieldBuf);
		auto img = innerFrame->findImage(imgBuf);
		text->setDisabled(true);
		text->setFont(signEntry.font.c_str());
		text->setHJustify(signEntry.hjustify);
		text->setVJustify(signEntry.vjustify);
		text->setTextColor(signEntry.fontColor);
		text->setOutlineColor(signEntry.fontOutlineColor);
		text->clearWordsToHighlight();
		img->disabled = true;
		allFields.push_back(text);
		allImgs.push_back(img);
	}

	int variableIndex = 0;
	int imgIndex = 0;
	std::string formattedText = "";
	int fontHeight = 24;
	if ( auto fontGet = Font::get(allFields[0]->getFont()) )
	{
		fontHeight = fontGet->height(false) + fontGet->getOutline() * 2;
	}
	int spaceWidth = 10;
	if ( auto textGet = Text::get(" ", allFields[0]->getFont(), 
		allFields[0]->getTextColor(), allFields[0]->getOutlineColor()) )
	{
		spaceWidth = textGet->getWidth();
	}
	int currentY = 30 + signEntry.padTopY;
	const int topMostY = currentY;
	int bottomMostY = topMostY;
	int nextCurrentY = 0;
	for ( int line = 0; line < signEntry.rawText.size(); ++line )
	{
		if ( line >= 10 )
		{
			break;
		}
		std::vector<Frame::image_t*> imgsThisLine;
		Field* txt = allFields[line];
		int imageTopMostY = currentY;
		for ( auto c : signEntry.rawText[line] )
		{
			if ( c == '$' )
			{
				if ( signEntry.variables.size() > variableIndex )
				{
					auto& var = signEntry.variables[variableIndex];
					if ( var.type == ScriptTextParser_t::TEXT )
					{
						formattedText += var.value;
					}
					else if ( var.type == ScriptTextParser_t::GLYPH )
					{
						// set image as binding
						Frame::image_t* img = allImgs[imgIndex];
						imgsThisLine.push_back(img);

						if ( var.value == "Pause Game" && Input::inputs[player.playernum].input("Pause Game").isBindingUsingKeyboard() )
						{
							img->path = Input::inputs[player.playernum].getGlyphPathForInput("Escape");
						}
						else
						{
							img->path = Input::inputs[player.playernum].getGlyphPathForBinding(var.value.c_str());
						}
						if ( auto imgGet = Image::get(img->path.c_str()))
						{
							img->disabled = false;
							img->pos.w = imgGet->getWidth();
							img->pos.h = imgGet->getHeight();
							img->pos.y = currentY + fontHeight / 2 - img->pos.h / 2;
							if ( img->pos.y % 2 == 1 )
							{
								--img->pos.y;
							}
							if ( auto textGet = Text::get(formattedText.c_str(), txt->getFont(), txt->getTextColor(),
								txt->getOutlineColor()) )
							{
								// position just after the text.
								img->pos.x = txt->getSize().x + textGet->getWidth() + (spaceWidth / 2);
								if ( formattedText != "" )
								{
									img->pos.x += signEntry.imageInlineTextAdjustX;
								}
							}
							if ( img->pos.x % 2 == 1 )
							{
								++img->pos.x;
							}
							int numspaces = 1 + (imgGet->getWidth() / spaceWidth);
							while ( numspaces > 0 )
							{
								formattedText += ' '; // add spaces to cover image width
								--numspaces;
							}
							if ( img->pos.y < imageTopMostY )
							{
								// img overhangs the last line, push this one down
								const int diff = (imageTopMostY - img->pos.y);
								currentY += diff;
								img->pos.y += diff;

								// move previous images down
								for ( auto i : imgsThisLine )
								{
									if ( i != img )
									{
										i->pos.y += diff;
									}
								}
							}
							imageTopMostY = std::min(imageTopMostY, img->pos.y);
							if ( img->pos.y + img->pos.h > (currentY + fontHeight) )
							{
								nextCurrentY = std::max(nextCurrentY, (img->pos.y + img->pos.h) - (currentY + fontHeight));
							}
						}
						++imgIndex;
					}
					else if ( var.type == ScriptTextParser_t::IMG )
					{
						// set image
						Frame::image_t* img = allImgs[imgIndex];
						imgsThisLine.push_back(img);
						img->path = var.value;
						if ( auto imgGet = Image::get(img->path.c_str()) )
						{
							img->disabled = false;
							img->pos.w = imgGet->getWidth();
							img->pos.h = imgGet->getHeight();
							if ( var.sizex != 0 )
							{
								img->pos.w = var.sizex;
							}
							if ( var.sizey != 0 )
							{
								img->pos.h = var.sizey;
							}
							img->pos.y = currentY + fontHeight / 2 - img->pos.h / 2;
							if ( img->pos.y % 2 == 1 )
							{
								--img->pos.y;
							}
							if ( auto textGet = Text::get(formattedText.c_str(), txt->getFont(), txt->getTextColor(),
								txt->getOutlineColor()) )
							{
								// position just after the text.
								img->pos.x = txt->getSize().x + textGet->getWidth() + (spaceWidth / 2);
								if ( formattedText != "" )
								{
									img->pos.x += signEntry.imageInlineTextAdjustX;
								}
							}
							if ( img->pos.x % 2 == 1 )
							{
								++img->pos.x;
							}
							int numspaces = 1 + (img->pos.w / spaceWidth);
							while ( numspaces > 0 )
							{
								formattedText += ' '; // add spaces to cover image width
								--numspaces;
							}
							if ( img->pos.y < imageTopMostY )
							{
								// img overhangs the last line, push this one down
								const int diff = (imageTopMostY - img->pos.y);
								currentY += diff;
								img->pos.y += diff;

								// move previous images down
								for ( auto i : imgsThisLine )
								{
									if ( i != img )
									{
										i->pos.y += diff;
									}
								}
							}
							imageTopMostY = std::min(imageTopMostY, img->pos.y);
							if ( img->pos.y + img->pos.h >(currentY + fontHeight) )
							{
								nextCurrentY = std::max(nextCurrentY, (img->pos.y + img->pos.h) - (currentY + fontHeight));
							}
						}
						++imgIndex;
					}
					++variableIndex;
				}
			}
			else
			{
				formattedText += c;
			}
		}
		txt->setText(formattedText.c_str());
		txt->setDisabled(false);
		SDL_Rect pos = txt->getSize();
		pos.y = currentY;
		txt->setSize(pos);
		const int oldCurrentY = currentY;
		currentY += std::max(1, txt->getNumTextLines()) * fontHeight + nextCurrentY + signEntry.padPerLine[line];
		nextCurrentY = 0;

		if ( txt->getHJustify() == Field::justify_t::CENTER )
		{
			// adjust all images based on text width.
			const int textCenterOffset = txt->getTextObject()->getWidth();
			const int textStartX = txt->getSize().w / 2 - (textCenterOffset / 2);
			for ( auto i : imgsThisLine )
			{
				i->pos.x += textStartX;
				if ( i->pos.x % 2 == 1 )
				{
					++i->pos.x;
				}
			}
		}
		if ( formattedText != "" )
		{
			bottomMostY = currentY;
		}
		else
		{
			bottomMostY = oldCurrentY; // empty string, img only don't add buffer for line height.
		}
		formattedText = "";
	}

	int verticalAdjustY = 0;
	if ( signEntry.vjustify == Field::justify_t::CENTER )
	{
		verticalAdjustY = (innerFrame->getSize().h / 2) - ((bottomMostY - topMostY) / 2) - topMostY;
	}

	for ( auto f : allFields )
	{
		SDL_Rect pos = f->getSize();
		pos.y += verticalAdjustY;
		f->setSize(pos);
	}
	for ( auto i : allImgs )
	{
		i->pos.y += verticalAdjustY;
	}
	for ( auto highlight : signEntry.wordHighlights )
	{
		int line = 0;
		while ( highlight >= Field::TEXT_HIGHLIGHT_WORDS_PER_LINE )
		{
			highlight -= Field::TEXT_HIGHLIGHT_WORDS_PER_LINE;
			++line;
		}
		allFields[line]->addWordToHighlight(highlight, signEntry.fontHighlightColor);
	}
	for ( auto highlight : signEntry.wordHighlights2 )
	{
		int line = 0;
		while ( highlight >= Field::TEXT_HIGHLIGHT_WORDS_PER_LINE )
		{
			highlight -= Field::TEXT_HIGHLIGHT_WORDS_PER_LINE;
			++line;
		}
		allFields[line]->addWordToHighlight(highlight, signEntry.fontHighlight2Color);
	}
	return;
}