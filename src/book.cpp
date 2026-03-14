/*-------------------------------------------------------------------------------

	BARONY
	File: book.cpp
	Desc: implements readable books

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "files.hpp"
#include "game.hpp"
#include "interface/interface.hpp"
#include "book.hpp"
#include "rapidjson/document.h"
#include "player.hpp"
#include "ui/Text.hpp"
#include "ui/Field.hpp"
#include "mod_tools.hpp"

std::vector<Book_t> allBooks;
BookParser_t bookParser_t;
int numbooks = 0;

//book_t *book_space_ninjas = NULL;

int getBook(std::string bookTitle)
{
	int index = 0;
	for ( auto& book : allBooks )
	{
		if ( book.default_name == bookTitle )
		{
			return index;
		}
		++index;
	}
	return 0;
}

std::string getBookDefaultNameFromIndex(int index, bool censored)
{
	if (allBooks.empty() || index < 0 || index >= allBooks.size()) {
		return "";
	}
	if (!spawn_blood && censored) {
		for (int c = 0; c < num_banned_books; ++c) {
			auto banned_book = banned_books[c];
			if (allBooks[index].default_name == banned_book) {
				return getBookDefaultNameFromIndex((index + 1) % allBooks.size(), censored);
			}
		}
	}
	return allBooks[index].default_name;
}

std::string getBookLocalizedNameFromIndex(int index, bool censored)
{
	if ( allBooks.empty() || index < 0 || index >= allBooks.size() ) {
		return "";
	}
	if ( !spawn_blood && censored ) {
		for ( int c = 0; c < num_banned_books; ++c ) {
			auto banned_book = banned_books[c];
			if ( allBooks[index].default_name == banned_book ) {
				return getBookLocalizedNameFromIndex((index + 1) % allBooks.size(), censored);
			}
		}
	}
	return ItemTooltips.bookNameLocalizations[allBooks[index].default_name];
}

//Local helper function to make getting the list of books cross-platform easier.
std::list<std::string> getListOfBooks()
{
	std::list<std::string> books;
	books = physfsGetFileNamesInDirectory("books/");
	for ( auto it = books.begin(); it != books.end(); )
	{
		if ( (*it).find(".txt") == std::string::npos)
		{
			it = books.erase(it);
		}
		else
		{
			++it;
		}
	}
	return books;
}

void BookParser_t::deleteBooks()
{
	allBooks.clear();
	numbooks = 0;
}


std::list<std::string> BookParser_t::getListOfBooksAfterFiltering()
{
	std::list<std::string> discoveredbooks = getListOfBooks();

#ifndef NINTENDO
	//TODO: We will need to enable this on NINTENDO if we want mod support. Realistically, that just means adding JSON support and making ignoreBooksPath a static const definition in a header somewhere. (2 headers, actually: define it once for PC, definite it differently for Switch) ...we'll probably also need to update thet PHYSFS_getRealDir() call as well, something akin to the getListOfBooks() function. I.e. NINTENDO ROM reading or whatever.
	std::string ignoreBooksPath = "books/ignored_books.json";
	std::unordered_set<std::string> ignoredBooks;
	bool foundIgnoreBookFile = false;
	if ( PHYSFS_getRealDir(ignoreBooksPath.c_str()) != NULL )
	{
		foundIgnoreBookFile = true;
		ignoredBooks.insert("ignored_books.json");
		std::string path = PHYSFS_getRealDir(ignoreBooksPath.c_str());
		path.append(PHYSFS_getDirSeparator());
		ignoreBooksPath = path + ignoreBooksPath;

		File* fp = FileIO::open(ignoreBooksPath.c_str(), "rb");
		if ( fp )
		{
			char buf[MAX_FILE_LENGTH];
			int count = fp->read(buf, sizeof(buf[0]), sizeof(buf));
			buf[count] = '\0';
			rapidjson::StringStream is(buf);
			FileIO::close(fp);

			rapidjson::Document d;
			d.ParseStream(is);
			if ( !d.HasMember("ignored_books") )
			{
				printlog("[JSON]: Could not read member 'ignored_books', possible invalid syntax.");
			}
			else
			{
				for ( rapidjson::Value::ConstValueIterator itr = d["ignored_books"].Begin(); itr != d["ignored_books"].End(); ++itr )
				{
					ignoredBooks.insert(itr->GetString());
				}
			}
		}
	}
#endif //ifndef NINTENDO

	if ( !discoveredbooks.empty() )
	{
		printlog("[Books]: Loading books...\n");

		int numSkipBooks = 0;
#ifndef NINTENDO
		if ( foundIgnoreBookFile )
		{
			for ( auto& filename : discoveredbooks )
			{
				if ( filename.find(".txt") == std::string::npos )
				{
					++numSkipBooks;
					printlog("[Books]: Skipping book '%s' due to filename\n", filename.c_str());
				}
				else if ( ignoredBooks.find(filename) != ignoredBooks.end() )
				{
					++numSkipBooks;
					printlog("[Books]: Skipping book '%s' due to 'ignored_books.json'\n", filename.c_str());
				}
			}
		}
#endif
		// sort books alphabetically
		discoveredbooks.sort();

		std::list<std::string> filteredBooks;
		// read books
		for ( const auto& filename : discoveredbooks )
		{
#ifndef NINTENDO
			if ( ignoredBooks.find(filename) != ignoredBooks.end() )
			{
				continue;
			}
#endif
			filteredBooks.push_back(filename);
		}
		return filteredBooks;
	}
	return discoveredbooks;
}

void BookParser_t::createBooks(bool forceCacheRebuild)
{
	deleteBooks(); // empty the old books array

	std::list<std::string> discoveredbooks = getListOfBooksAfterFiltering();
	for ( const auto& filename : discoveredbooks )
	{
		allBooks.push_back(Book_t());
		auto& book = allBooks.back();
		book.default_name = filename;
		auto ext = book.default_name.find(".txt");
		if ( ext != std::string::npos )
		{
			book.default_name = book.default_name.substr(0, ext);
		}
	}

	numbooks = allBooks.size();

	if ( numbooks == 0 )
	{
		printlog("[Books]: Warning: no books discovered in /books/ directory.\n");
	}
	else
	{
		printlog("[Books]: Discovered %d books.\n", numbooks);
	}
}

void BookParser_t::compileBook(int index)
{
	if ( index < 0 || index >= (int)allBooks.size() )
	{
		return;
	}
	auto& book = allBooks[index];
	if ( !book.formattedPages.empty() )
	{
		return; // already compiled
	}

	// Reconstruct the .txt filename from the stored default_name
	std::string filename = book.default_name + ".txt";
	std::string bookPath = "books/";
	bookPath.append(filename);
	if ( PHYSFS_getRealDir(bookPath.c_str()) != nullptr )
	{
		std::string path = PHYSFS_getRealDir(bookPath.c_str());
		path.append(PHYSFS_getDirSeparator());
		bookPath = path + bookPath;
	}

	char bookChar[PATH_MAX];
	strncpy(bookChar, bookPath.c_str(), PATH_MAX - 1);
	char* tmp = readFile(bookChar);
	if ( tmp )
	{
		book.text = tmp;
	}
	free(tmp);
	if ( book.text.empty() )
	{
		printlog("error opening book \"%s\".\n", bookPath.c_str());
		return;
	}

	// Normalize: strip \r, expand \t to 3 spaces
	std::string pageText;
	for ( auto& character : book.text )
	{
		if ( character == '\r' )
		{
			continue;
		}
		if ( character == '\t' )
		{
			for ( int insertTabs = 3; insertTabs > 0; --insertTabs )
			{
				pageText += ' ';
			}
		}
		else
		{
			pageText += character;
		}
	}

	const int MAX_PARSE_CHARACTERS = 65535;
	Field* tmpField = new Field(MAX_PARSE_CHARACTERS);
	tmpField->setSize(SDL_Rect{ 0, 0, Player::BookGUI_t::BOOK_PAGE_WIDTH, Player::BookGUI_t::BOOK_PAGE_HEIGHT });
	tmpField->setFont("fonts/pixel_maz.ttf#32");
	tmpField->setText(pageText.c_str());
	tmpField->reflowTextToFit(0, false);

	int len = strlen(tmpField->getText());
	char* reflowedText = (char*)malloc(len + 1);
	memcpy(reflowedText, tmpField->getText(), sizeof(char) * (len + 1));
	reflowedText[len] = '\0';

	pageText = "";
	bool firstIteration = true;
	char* nexttoken = nullptr;
	char* token = reflowedText;
	do {
		nexttoken = Field::tokenize(token, "\n");
		if ( !pageText.empty() || (!strcmp(token, "") && !firstIteration) )
		{
			pageText.push_back('\n');
		}
		firstIteration = false;
		pageText += token;
		tmpField->setText(pageText.c_str());
		int textHeight = tmpField->getNumTextLines() * Font::get(tmpField->getFont())->height();
		if ( textHeight > tmpField->getSize().h )
		{
			// exceeds size, move to next page.
			book.formattedPages.push_back(pageText);
			pageText = "";
		}
	} while ( (token = nexttoken) != NULL );
	book.formattedPages.push_back(pageText);

	delete tmpField;
	free(reflowedText);
}


bool physfsSearchBooksToUpdate()
{
	std::list<std::string> booklist = getListOfBooks();
	if ( !booklist.empty() )
	{
		for ( auto& bookTitle : booklist )
		{
			std::string bookFilename = "books/" + bookTitle;
			if ( PHYSFS_getRealDir(bookFilename.c_str()) != nullptr )
			{
				std::string bookDir = PHYSFS_getRealDir(bookFilename.c_str());
				if ( bookDir.compare("./") != 0 )
				{
					// found a book not belonging in the base path.
					printlog("[PhysFS]: Found modified book in books/ directory, reloading all books...");
					return true;
				}
			}
		}
	}
	return false;
}

void physfsReloadBooks()
{
	const std::list<std::string> booklist = getListOfBooks();
	if ( !booklist.empty() )
	{
		bookParser_t.createBooks(true);
	}
}
