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
#include "rapidjson/filereadstream.h"
#include "player.hpp"
#include "ui/Text.hpp"
#include "ui/Field.hpp"
#include "mod_tools.hpp"

namespace
{

constexpr int32_t TAB_WIDTH = 3;

}

//#define SPACE_NINJA_NAME "Order of the Space Ninjas"
//Any word less than this will just get bumped onto the next line.
#define MIN_LENGTH_TO_SPLIT_WORD (book_characterspace_x / 2)
//#define MIN_LENGTH_TO_SPLIT_WORD_TITLE (characterspace_x / 2) //This only works in the formatTitle() function, since it uses a local variable characterspace_x in there.
//#define SPLIT_WORD_IN_TITLE false //Whether or not to split a word in the book's title. If set to false, will only split words if they have to be split. If set to true, will split words if they're a minimum length of MIN_LENGTH_TO_SPLIT_WORD_TITLE.;

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

bool BookParser_t::readCompiledBooks()
{
	std::string compiledBooksPath = "books/compiled_books.json";
	if ( PHYSFS_getRealDir(compiledBooksPath.c_str()) != NULL )
	{
		std::string path = PHYSFS_getRealDir(compiledBooksPath.c_str());
		path.append(PHYSFS_getDirSeparator());
		compiledBooksPath = path + compiledBooksPath;
		File* fp = FileIO::open(compiledBooksPath.c_str(), "rb");
		if ( fp )
		{
			char buf[MAX_FILE_LENGTH];
			int count = fp->read(buf, sizeof(buf[0]), sizeof(buf));
			buf[count] = '\0';
			rapidjson::StringStream is(buf);
			FileIO::close(fp);

			rapidjson::Document d;
			d.ParseStream(is);
			if ( !d.HasMember("version") || !d.HasMember("books") )
			{
				printlog("[JSON]: Could not read member 'version' or 'books', possible invalid syntax.");
				return false;
			}

			int numBooksRead = 0;
			for ( rapidjson::Value::ConstMemberIterator book_itr = d["books"].MemberBegin();
				book_itr != d["books"].MemberEnd(); ++book_itr )
			{
				allBooks.push_back(Book_t());
				auto& newBook = allBooks[allBooks.size() - 1];
				newBook.default_name = book_itr->name.GetString();
				for ( rapidjson::Value::ConstValueIterator page_itr = book_itr->value["pages"].Begin();
					page_itr != book_itr->value["pages"].End(); ++page_itr )
				{
					newBook.formattedPages.push_back(page_itr->GetString());
				}
				++numBooksRead;
			}
			printlog("[Books]: Read %d precompiled books successfully.", numBooksRead);
			return true;
		}
	}
	return false;
}

bool BookParser_t::booksRequireCompiling()
{
	readBooksIntoTemp();

	std::string compiledBooksPath = "books/compiled_books.json";
	if ( PHYSFS_getRealDir(compiledBooksPath.c_str()) != NULL )
	{
		std::string path = PHYSFS_getRealDir(compiledBooksPath.c_str());
		path.append(PHYSFS_getDirSeparator());
		compiledBooksPath = path + compiledBooksPath;
		File* fp = FileIO::open(compiledBooksPath.c_str(), "rb");
		if ( fp )
		{
			char buf[MAX_FILE_LENGTH];
			int count = fp->read(buf, sizeof(buf[0]), sizeof(buf));
			buf[count] = '\0';
			rapidjson::StringStream is(buf);
			FileIO::close(fp);

			rapidjson::Document d;
			d.ParseStream(is);
			if ( !d.HasMember("version") || !d.HasMember("books") )
			{
				printlog("[JSON]: Could not read member 'version' or 'books', possible invalid syntax.");
				return false;
			}

			if ( d["books"].MemberCount() != tempBookData.size() )
			{
				printlog("[Books]: Compiled Books Check - Found %d compiled books, but %d in directory, recompiling...", d["books"].MemberCount(), tempBookData.size());
				return true;
			}

			for ( rapidjson::Value::ConstMemberIterator book_itr = d["books"].MemberBegin();
				book_itr != d["books"].MemberEnd(); ++book_itr )
			{
				std::string bookName = book_itr->name.GetString();
				std::string rawText = book_itr->value["raw_text"].GetString();

				if ( tempBookData.find(bookName) == tempBookData.end() )
				{
					printlog("[Books]: Compiled Books Check - Book title: \"%s\" not found in compiled books, recompiling...", bookName.c_str());
					return true;
				}
				else
				{
					if ( tempBookData[bookName] != rawText )
					{
						printlog("[Books]: Compiled Books Check - Book text: \"%s\" does not match in compiled books, recompiling...", bookName.c_str());
						return true;
					}
				}
			}
		}
	}
	return false;
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

void BookParser_t::readBooksIntoTemp()
{
	tempBookData.clear();

	std::list<std::string> discoveredbooks = getListOfBooksAfterFiltering();

	if ( !discoveredbooks.empty() )
	{
		printlog("[Books]: Read %d books into temporary storage...", discoveredbooks.size());
		// read books
		for ( const auto& filename : discoveredbooks )
		{
			//printlog("reading book: \"%s\"\n", filename.c_str());
			std::string filenameNoExtension = filename;
			auto findExtension = filename.find(".txt");
			if ( findExtension != std::string::npos )
			{
				filenameNoExtension = filename.substr(0, findExtension);
			}
			tempBookData.insert(std::make_pair(filenameNoExtension, ""));
			auto& entry = tempBookData[filenameNoExtension];

			//Load in the text from a file.
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
			entry = readFile(bookChar);
		}
	}
	else
	{
		printlog("[Books]: Warning - no books were discovered.");
	}
}

void BookParser_t::createBooks(bool forceCacheRebuild)
{
	deleteBooks(); // empty the old books array

	if ( !forceCacheRebuild && !booksRequireCompiling() )
	{
		if ( readCompiledBooks() )
		{
			numbooks = allBooks.size();
			return;
		}
		printlog("[Books]: Error - Failed to read pre-compiled books... recompiling.");
	}

	std::list<std::string> discoveredbooks = getListOfBooksAfterFiltering();
	// create books
	for ( const auto& filename : discoveredbooks )
	{
		printlog("[Books]: Compiling book: \"%s\"\n", filename.c_str());
		createBook(filename);
	}
	
	if ( discoveredbooks.empty() )
	{
		printlog( "[Books]: Warning: discoveredbooks is empty. No books in /books/ directory?\n");
	}

	numbooks = allBooks.size();

	writeCompiledBooks();
}

void BookParser_t::writeCompiledBooks()
{
	std::string inputPath = outputdir;
	inputPath.append(PHYSFS_getDirSeparator());
	std::string fileName = "books/compiled_books.json";
	inputPath.append(fileName);

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	rapidjson::Document d;
	if ( !fp )
	{
		printlog("[JSON]: Could not locate json file %s, creating new file.", inputPath.c_str());
		d.SetObject();
		CustomHelpers::addMemberToRoot(d, "version", rapidjson::Value(versionJSON));
	}
	else
	{
		char buf[MAX_FILE_LENGTH];
		int count = fp->read(buf, sizeof(buf[0]), sizeof(buf));
		buf[count] = '\0';
		rapidjson::StringStream is(buf);
		FileIO::close(fp);
		d.ParseStream(is);

		if ( !d.HasMember("version") )
		{
			printlog("[JSON]: Could not read member 'version', possible invalid syntax.");
			printlog("[Books]: Error: Failed to compile books into file: '%s'", inputPath.c_str());
			return;
		}
	}

	if ( d.HasMember("books") )
	{
		d.EraseMember("books");
	}
	rapidjson::Value booksObj(rapidjson::kObjectType);
	CustomHelpers::addMemberToRoot(d, "books", booksObj);

	for ( auto& book : allBooks )
	{
		if ( !d["books"].HasMember(book.default_name.c_str()) )
		{
			rapidjson::Value bookObj(rapidjson::kObjectType);
			CustomHelpers::addMemberToSubkey(d, "books", book.default_name.c_str(), bookObj);

			rapidjson::Value rawTextKey("raw_text", d.GetAllocator());
			rapidjson::Value rawTextVal(book.text.c_str(), d.GetAllocator());
			d["books"][book.default_name.c_str()].AddMember(rawTextKey, rawTextVal, d.GetAllocator());

			rapidjson::Value pagesKey("pages", d.GetAllocator());
			rapidjson::Value pagesArrayVal(rapidjson::kArrayType);
			d["books"][book.default_name.c_str()].AddMember(pagesKey, pagesArrayVal, d.GetAllocator());
			for ( auto& page : book.formattedPages )
			{
				rapidjson::Value pageVal;
				pageVal.SetString(page.c_str(), d.GetAllocator());
				d["books"][book.default_name.c_str()]["pages"].PushBack(pageVal, d.GetAllocator());
			}
		}
		else
		{
			d["books"][book.default_name.c_str()]["raw_text"].SetString(book.text.c_str(), d.GetAllocator());
			d["books"][book.default_name.c_str()]["pages"].Clear();
			for ( auto& page : book.formattedPages )
			{
				rapidjson::Value pageVal;
				pageVal.SetString(page.c_str(), d.GetAllocator());
				d["books"][book.default_name.c_str()]["pages"].PushBack(pageVal, d.GetAllocator());
			}
		}
	}

	fp = FileIO::open(inputPath.c_str(), "wb");
	if ( !fp )
	{
		printlog("[Books]: Error: Failed to compile books into file: '%s'", inputPath.c_str());
		return;
	}
	rapidjson::StringBuffer os;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
	d.Accept(writer);
	fp->write(os.GetString(), sizeof(char), os.GetSize());
	FileIO::close(fp);
	
	printlog("[Books]: Successfully compiled books into file: '%s'", inputPath.c_str());
}

/*****createBook() helper functions******/

bool isLetter(const char character)
{
  return (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z');
}

//This is a more powerful version of isLetter that checks if a specified character is part of a word. However, it requires contextual information -- what are the next and previous characters? So pass the entire string to this function and the index in the string of the character being looked up.
bool isCharacterPartOfWord(char* const text, const int index)
{
	if (!text)
	{
		return false;
	}
	if (index < 0 || index > strlen(text))
	{
		return false;
	}

	if (isLetter(text[index]))
	{
		return true;    //It's a character. Return true because no use continuing this function.
	}

	switch (text[index])
	{
		case '\'':
			if (isLetter(text[index - 1]) && isLetter(text[index + 1]))   //An apostrophe needs to be surrounded by letters to qualify as part of a word.
			{
				return true;
			}
			return false;
		default:
			return false;
	}
}

int moveToStartOfWord(char* const text, const int index)
{
	if (!text)
	{
		return index;
	}
	if (index < 0 || index > strlen(text))
	{
		return index;
	}

	int i = index;

	//Keep running backwards until it finds a non-letter character (which would be the character before the start of the word).
	for (; i - 1 > 0; --i)
	{
		if (!isCharacterPartOfWord(text, i - 1))
		{
			return i;
		}
	}

	return i;
}

//Returns 0 on error. Returns 0 if index not on a word. Returns length of word otherwise. If nonletter character at the current index, it keeps looking until it finds the start of the next word.
int lengthOfCurrentWord(char* const text, const int index)
{
	if (!text)   //Can't do this without text.
	{
		return 0;
	}
	if (index < 0 || index > strlen(text))   //Index has to be in bounds.
	{
		return 0;
	}

	int length = 0;
	int i = index;

	if (!isCharacterPartOfWord(text, i)) //The current character is not part of a word.
		//i = moveToNextWord(text, index); //Find the start of the next word.
	{
		return 0;    //Not our problem. Tell it the current word is length 0.
	}

	if (i > 0 && isCharacterPartOfWord(text, i))     //Not at the start of the text array and the previous character is a letter...yikes, not at the start of the word.
	{
		i = moveToStartOfWord(text, i);    //Move to the start of the current word.
	}

	for (; i < strlen(text); ++i)
	{
		if (isCharacterPartOfWord(text, i))
		{
			length++;    //The current character is part of the word.
		}
		else
		{
			return length;    //Reached the end of the word. Return the length.
		}
	}

	return length;
}

void BookParser_t::createBook(std::string filename)
{
	//Load in the text from a file.
	std::string tempstr = "books/";
	tempstr.append(filename);
	if ( PHYSFS_getRealDir(tempstr.c_str()) != nullptr )
	{
		std::string path = PHYSFS_getRealDir(tempstr.c_str());
		path.append(PHYSFS_getDirSeparator());
		tempstr = path + tempstr;
	}

	allBooks.push_back(Book_t());
	auto& newBook = allBooks[allBooks.size() - 1];

	char bookChar[PATH_MAX];
	strncpy(bookChar, tempstr.c_str(), PATH_MAX - 1);
	newBook.text = readFile(bookChar);
	if ( newBook.text == "" )
	{
		printlog( "error opening book \"%s\".\n", tempstr.c_str());
		return; //Failed to open the file.
	}

	newBook.default_name = filename;
	auto findTxt = newBook.default_name.find(".txt");
	if ( findTxt != std::string::npos )
	{
		newBook.default_name = newBook.default_name.substr(0, findTxt);
	}
	//newBook.rawBookText = book->text;
	
	std::string pageText = "";
	for ( auto& character : newBook.text )
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
			newBook.formattedPages.push_back(pageText);
			pageText = "";
		}
	} while ( (token = nexttoken) != NULL );
	newBook.formattedPages.push_back(pageText);

	if ( tmpField )
	{
		delete tmpField;
		tmpField = nullptr;
	}
	if ( reflowedText )
	{
		free(reflowedText);
		reflowedText = nullptr;
	}
	return;

	//const Uint32 color = makeColor( 0, 0, 0, 255);
	//strncpy(string->data, pageText.c_str(), 120);
	//int len = strlen(pageText.c_str());
	//string->data[120] = '\0';
	//printlog(string->data);
	//return;
	////for ( int c = 0; reflowedText[c] != '\0'; ++c )
	////{
	////	string->data[c] = reflowedText[c];
	////}
	//char* nexttoken = nullptr;
	//char* token = reflowedText;
	//do {
	//	nexttoken = Field::tokenize(token, "\n");
	//	if ( !pageText.empty() )
	//	{
	//		pageText.push_back('\n');
	//	}
	//	if ( token )
	//	{
	//		for ( int c = 0; token[c] != '\0'; ++c )
	//		{
	//			if ( token[c] == '\r' )
	//			{
	//				continue;
	//			}
	//			if ( token[c] == '\t' )
	//			{
	//				for ( int insertTabs = TAB_WIDTH; insertTabs > 0; --insertTabs )
	//				{
	//					pageText += ' ';
	//				}
	//			}
	//			else
	//			{
	//				pageText += token[c];
	//			}
	//		}
	//	}
	//	tmpField.setText(pageText.c_str());
	//	if ( auto getText = Text::get(tmpField.getText(), tmpField.getFont(),
	//		makeColor(255, 255, 255, 255), makeColor(0, 0, 0, 255)) )
	//	{
	//		if ( getText->getHeight() > tmpField.getSize().h - tmpField.getSize().y )
	//		{
	//			// exceeds size, move to next page.
	//			for ( int p = 0; p < pageText.size(); ++p )
	//			{
	//				string->data[p] = pageText[p];
	//			}
	//			//strcpy(string->data, pageText.c_str());
	//			int len = strlen(pageText.c_str());
	//			string->data[len] = '\0';

	//			string = newString(&book->pages, color, nullptr);
	//			string->data = static_cast<char*>(malloc(sizeof(char) * (max_characters + 1)));
	//			memset(string->data, 0, sizeof(char) * (max_characters + 1));

	//			pageText = "";
	//		}
	//	}

	//} while ( (token = nexttoken) != NULL );
	//len = strlen(pageText.c_str());
	//for ( int p = 0; p < pageText.size(); ++p )
	//{
	//	string->data[p] = pageText[p];
	//}
	////strcpy(string->data, pageText.c_str());
	//string->data[len] = '\0';
	//free(reflowedText);
	//return;

	//string_t* string = newString(&book->pages, color, nullptr);
	//string->data = static_cast<char*>(malloc(sizeof(char) * (max_characters + 1)));
	//memset(string->data, 0, sizeof(char) * (max_characters + 1));

	//int p = 0; // current character in the page's text
	//int x = 0; // number of characters written on the current line
	//int y = 0; // number of lines on the page
	//bool newline = false;
	//bool can_write = true; //If false, it means that the character write to page was interecepted by a - to properly break up a word.
	//int tab = 0; //Inserting tab characters.
	//char character_to_record  = ' ';
	//bool write_out = true;
	////found_word and word_length are used to prevent smaller words from being broken up. When the for loop detects that it has hit the start of a word, it queries for the word's length. If the word's length < MIN_LENGTH_TO_SPLIT_WORD, then it pumps out a newline and then starts the word. word_length_left is there so that it knows how many more characters it has to go through to reach the end of the word.
	//bool found_word = false;
	//int word_length = 0;
	//for (int i = 0; book->text[i] != 0; ++i)
	//{
	//	//So first iterate through and count every line.
	//	//Line end conditions:
	//	//1. Encountered a newline character.
	//	//2. Hit the max amount of characters that can fit horizontally in the book GUI.
	//	write_out = true;
	//	newline = false;
	//	can_write = true;

	//	//When the max amount of lines that can fit on one page have been hit, create a new page.
	//	if (tab <= 0 && book->text[i] == '\t')   //Assuming it is a null terminated string.
	//	{
	//		//Found a newline. Create a new line.
	//		tab = TAB_WIDTH;
	//	}
	//	if (x + 1 >= book_characterspace_x)
	//	{
	//		//Overflowed the line. Go onto the next line.
	//		newline = true;
	//	}
	//	else if (tab <= 0 && book->text[i] == '\n')
	//	{
	//		newline = true;
	//		write_out = false; //Do not want to write out this /n.
	//	}

	//	character_to_record = book->text[i];

	//	if (newline)
	//	{
	//		x = 0; //Reset x since it's back to the beginning of the line.
	//		if (y + 1 >= book_characterspace_y)
	//		{
	//			//Create the next page. Do not record the character if it's a newline.
	//			string = newString(&book->pages, color, nullptr);
	//			string->data = static_cast<char*>(malloc(sizeof(char) * (max_characters + 1)));
	//			memset(string->data, 0, sizeof(char) * (max_characters + 1));
	//			p = 0;
	//			y = 0;
	//		}
	//		else
	//		{
	//			//Record the new line.
	//			string->data[p] = '\n';
	//			p++;
	//			y++;
	//			//No need to record the item in book->text[i] because it was a newline.
	//		}
	//		//Bugger the tab away if it hit the end of the line.
	//		if (tab > 0)
	//		{
	//			if (tab < TAB_WIDTH)
	//			{
	//				i--;    //Do this too? Or i++? I don't know.
	//			}
	//			tab = 0;
	//		}
	//		// no spaces at the start of a line
	//		if ( character_to_record == ' ' )
	//		{
	//			continue;
	//		}
	//	}
	//	else if (tab > 0)
	//	{
	//		character_to_record = ' ';
	//	}

	//	if (write_out)
	//	{
	//		//Ok, record the character.

	//		//First, check if it's found a word.
	//		if (!found_word && tab <= 0)   //Don't check for a word if already found one (to save processing) and don't bother checking if tab mode is active.
	//		{
	//			if (isCharacterPartOfWord(book->text, i))
	//			{
	//				//The character is part of a word. Record that.
	//				found_word = true;
	//				word_length = lengthOfCurrentWord(book->text, i);

	//				//Now check if the word will fit on the line and if it's okay to split it. If not okay to split it, make a new line.
	//				if (word_length <= MIN_LENGTH_TO_SPLIT_WORD)
	//				{
	//					//The word doesn't like being split.
	//					if ((x + 1) + word_length >= book_characterspace_x && word_length < book_characterspace_x)
	//					{
	//						//So make a new line.
	//						x = 0; //Reset x, since it's writing out a new line and x gets reset when we do that.
	//						if (y + 1 >= book_characterspace_y)   //Check if it hit the end of the page.
	//						{
	//							//It does indeed go off the page. Start a new page.
	//							string = newString(&book->pages, color, nullptr);
	//							string->data = static_cast<char*>(malloc(sizeof(char) * (max_characters + 1)));
	//							memset(string->data, 0, sizeof(char) * (max_characters + 1));
	//							p = 0;
	//							y = 0;
	//						}
	//						else
	//						{
	//							string->data[p] = '\n';
	//							p++;
	//							y++;
	//						}
	//					}
	//				}
	//			}
	//		}
	//		else if (found_word)
	//		{
	//			if (!isCharacterPartOfWord(book->text, i))
	//			{
	//				//Reached the end of the word.
	//				found_word = false;
	//				word_length = 0;
	//			}
	//		}

	//		if (x + 2 >= book_characterspace_x)
	//		{
	//			if (isCharacterPartOfWord(book->text, i))
	//			{
	//				if (book->text[i - 1] != ' ')   //First check if the previous character is not a space.
	//				{
	//					//If it is not a space (and the like), write out a space since this one is character, the previous one isn't, and the newline is interrupting a word.
	//					string->data[p] = '-';
	//					p++;
	//					x++;
	//					can_write = false;
	//					i--;
	//				}
	//				else if (isCharacterPartOfWord(book->text, i + 1))     //Okay, so the previous character is a space (or otherwise not constituting a word. Check if the next character is not a space (or the like)).
	//				{
	//					//Next character is not a space (and the like), write out a space so that the word's start is delayed until a safer point in time.
	//					string->data[p] = ' ';
	//					p++;
	//					x++;
	//					can_write = false;
	//					i--;
	//				}
	//			}
	//		}

	//		if (can_write)
	//		{
	//			(string->data)[p] = character_to_record;
	//			p++;
	//			x++;
	//		}
	//		else
	//		{
	//			can_write = true;
	//		}

	//		if (tab > 0)
	//		{
	//			if (tab < TAB_WIDTH)
	//			{
	//				i--;    //To make sure it doesn't bugger the character.
	//			}
	//			tab--;
	//		}
	//	}

	//	newline = false;
	//}
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
