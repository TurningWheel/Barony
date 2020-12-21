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

namespace
{

constexpr int32_t TAB_WIDTH = 3;

}

//#define SPACE_NINJA_NAME "Order of the Space Ninjas"
//Any word less than this will just get bumped onto the next line.
#define MIN_LENGTH_TO_SPLIT_WORD (book_characterspace_x / 2)
//#define MIN_LENGTH_TO_SPLIT_WORD_TITLE (characterspace_x / 2) //This only works in the formatTitle() function, since it uses a local variable characterspace_x in there.
//#define SPLIT_WORD_IN_TITLE false //Whether or not to split a word in the book's title. If set to false, will only split words if they have to be split. If set to true, will split words if they're a minimum length of MIN_LENGTH_TO_SPLIT_WORD_TITLE.;

book_t** books = nullptr;
int numbooks = 0;

//book_t *book_space_ninjas = NULL;

int getBook(char const * const booktitle)
{
	for ( int c = 0; c < numbooks; c++ )
	{
		if ( !strcmp(booktitle, books[c]->name) )
		{
			return c;
		}
	}
	return 0;
}

//Local helper function to make getting the list of books cross-platform easier.
std::list<std::string> getListOfBooks()
{
	std::list<std::string> books;
#ifdef NINTENDO
	File* fp = FileIO::open(NINTENDO_LIST_OF_BOOKS_FILEPATH, "r");
	for (char buf[64]; fp->gets2(buf, 64);)
	{
		books.push_back(std::string(buf));
	}
	FileIO::close(fp);
#else
	books = physfsGetFileNamesInDirectory("books/");
#endif
	return books;
}

void createBooks()
{
	int i = 0;

	//TODO: Read the books/ inventory for all *.txt files.
	//TODO: Then create a book for each file there and add it to a books array.
	//auto discoveredbooks = directoryContents("books/", false, true);
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
			char buf[65536];
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

	if (!discoveredbooks.empty())
	{
		printlog("compiling books...\n");

		int numSkipBooks = 0;
		if ( foundIgnoreBookFile )
		{
			for ( auto& filename : discoveredbooks )
			{
				if ( filename.compare("ignored_books.json") == 0 )
				{
					++numSkipBooks;
					printlog("skipping book %s due to filename\n", filename.c_str());
				}
				else if ( ignoredBooks.find(filename) != ignoredBooks.end() )
				{
					++numSkipBooks;
					printlog("'skipping book %s due to 'ignored_books.json'\n", filename.c_str());
				}
			}
		}

		// Allocate memory for books
		numbooks = discoveredbooks.size() - numSkipBooks;
		books = static_cast<book_t**>(malloc(sizeof(book_t*) * numbooks));

		// sort books alphabetically
		discoveredbooks.sort();

		// create books
		for ( const auto& filename : discoveredbooks )
		{
			if ( ignoredBooks.find(filename) != ignoredBooks.end() )
			{
				continue;
			}
			books[i] = static_cast<book_t*>(malloc(sizeof(book_t)));
			books[i]->text = nullptr;
			books[i]->name = strdup(filename.c_str());
			printlog("compiling book: \"%s\"\n",books[i]->name);
			books[i]->bookgui_render_title = nullptr;
			books[i]->bookgui_render_title_numlines = 0;
			books[i]->pages.first = nullptr;
			books[i]->pages.last = nullptr;
			//formatTitle(books[i]);
			createBook(books[i]);
			books[i]->name[strlen(books[i]->name) - 4] = 0;
			++i;
		}
	}
	else
	{
		printlog( "Warning: discoveredbooks is empty. No books in /books/ directory?\n");
	}
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

void createBook(book_t* const book)
{
	if (!book)
	{
		return;
	}

	//Load in the text from a file.
	std::string tempstr = "books/";
	tempstr.append(book->name);
	if ( PHYSFS_getRealDir(tempstr.c_str()) != nullptr )
	{
		std::string path = PHYSFS_getRealDir(tempstr.c_str());
		path.append(PHYSFS_getDirSeparator());
		tempstr = path + tempstr;
	}
	char bookChar[256];
	strncpy(bookChar, tempstr.c_str(), 255);
	book->text = readFile(bookChar);
	if (!book->text)
	{
		printlog( "error opening book \"%s\".\n", tempstr.c_str());
		return; //Failed to open the file.
	}

	const int book_characterspace_x = Player::BookGUI_t::BOOK_PAGE_WIDTH / BOOK_FONT_WIDTH;
	const int book_characterspace_y = Player::BookGUI_t::BOOK_PAGE_HEIGHT / BOOK_FONT_HEIGHT;
	const int max_characters = book_characterspace_x * book_characterspace_y;

	book->pages.first = nullptr;
	book->pages.last = nullptr;

	const Uint32 color = SDL_MapRGBA(mainsurface->format, 0, 0, 0, 255);
	string_t* string = newString(&book->pages, color, nullptr);
	string->data = static_cast<char*>(malloc(sizeof(char) * (max_characters + 1)));
	memset(string->data, 0, sizeof(char) * (max_characters + 1));

	int p = 0; // current character in the page's text
	int x = 0; // number of characters written on the current line
	int y = 0; // number of lines on the page
	bool newline = false;
	bool can_write = true; //If false, it means that the character write to page was interecepted by a - to properly break up a word.
	int tab = 0; //Inserting tab characters.
	char character_to_record  = ' ';
	bool write_out = true;
	//found_word and word_length are used to prevent smaller words from being broken up. When the for loop detects that it has hit the start of a word, it queries for the word's length. If the word's length < MIN_LENGTH_TO_SPLIT_WORD, then it pumps out a newline and then starts the word. word_length_left is there so that it knows how many more characters it has to go through to reach the end of the word.
	bool found_word = false;
	int word_length = 0;
	for (int i = 0; book->text[i] != 0; ++i)
	{
		//So first iterate through and count every line.
		//Line end conditions:
		//1. Encountered a newline character.
		//2. Hit the max amount of characters that can fit horizontally in the book GUI.
		write_out = true;
		newline = false;
		can_write = true;

		//When the max amount of lines that can fit on one page have been hit, create a new page.
		if (tab <= 0 && book->text[i] == '\t')   //Assuming it is a null terminated string.
		{
			//Found a newline. Create a new line.
			tab = TAB_WIDTH;
		}
		if (x + 1 >= book_characterspace_x)
		{
			//Overflowed the line. Go onto the next line.
			newline = true;
		}
		else if (tab <= 0 && book->text[i] == '\n')
		{
			newline = true;
			write_out = false; //Do not want to write out this /n.
		}

		character_to_record = book->text[i];

		if (newline)
		{
			x = 0; //Reset x since it's back to the beginning of the line.
			if (y + 1 >= book_characterspace_y)
			{
				//Create the next page. Do not record the character if it's a newline.
				string = newString(&book->pages, color, nullptr);
				string->data = static_cast<char*>(malloc(sizeof(char) * (max_characters + 1)));
				memset(string->data, 0, sizeof(char) * (max_characters + 1));
				p = 0;
				y = 0;
			}
			else
			{
				//Record the new line.
				string->data[p] = '\n';
				p++;
				y++;
				//No need to record the item in book->text[i] because it was a newline.
			}
			//Bugger the tab away if it hit the end of the line.
			if (tab > 0)
			{
				if (tab < TAB_WIDTH)
				{
					i--;    //Do this too? Or i++? I don't know.
				}
				tab = 0;
			}
			// no spaces at the start of a line
			if ( character_to_record == ' ' )
			{
				continue;
			}
		}
		else if (tab > 0)
		{
			character_to_record = ' ';
		}

		if (write_out)
		{
			//Ok, record the character.

			//First, check if it's found a word.
			if (!found_word && tab <= 0)   //Don't check for a word if already found one (to save processing) and don't bother checking if tab mode is active.
			{
				if (isCharacterPartOfWord(book->text, i))
				{
					//The character is part of a word. Record that.
					found_word = true;
					word_length = lengthOfCurrentWord(book->text, i);

					//Now check if the word will fit on the line and if it's okay to split it. If not okay to split it, make a new line.
					if (word_length <= MIN_LENGTH_TO_SPLIT_WORD)
					{
						//The word doesn't like being split.
						if ((x + 1) + word_length >= book_characterspace_x && word_length < book_characterspace_x)
						{
							//So make a new line.
							x = 0; //Reset x, since it's writing out a new line and x gets reset when we do that.
							if (y + 1 >= book_characterspace_y)   //Check if it hit the end of the page.
							{
								//It does indeed go off the page. Start a new page.
								string = newString(&book->pages, color, nullptr);
								string->data = static_cast<char*>(malloc(sizeof(char) * (max_characters + 1)));
								memset(string->data, 0, sizeof(char) * (max_characters + 1));
								p = 0;
								y = 0;
							}
							else
							{
								string->data[p] = '\n';
								p++;
								y++;
							}
						}
					}
				}
			}
			else if (found_word)
			{
				if (!isCharacterPartOfWord(book->text, i))
				{
					//Reached the end of the word.
					found_word = false;
					word_length = 0;
				}
			}

			if (x + 2 >= book_characterspace_x)
			{
				if (isCharacterPartOfWord(book->text, i))
				{
					if (book->text[i - 1] != ' ')   //First check if the previous character is not a space.
					{
						//If it is not a space (and the like), write out a space since this one is character, the previous one isn't, and the newline is interrupting a word.
						string->data[p] = '-';
						p++;
						x++;
						can_write = false;
						i--;
					}
					else if (isCharacterPartOfWord(book->text, i + 1))     //Okay, so the previous character is a space (or otherwise not constituting a word. Check if the next character is not a space (or the like)).
					{
						//Next character is not a space (and the like), write out a space so that the word's start is delayed until a safer point in time.
						string->data[p] = ' ';
						p++;
						x++;
						can_write = false;
						i--;
					}
				}
			}

			if (can_write)
			{
				(string->data)[p] = character_to_record;
				p++;
				x++;
			}
			else
			{
				can_write = true;
			}

			if (tab > 0)
			{
				if (tab < TAB_WIDTH)
				{
					i--;    //To make sure it doesn't bugger the character.
				}
				tab--;
			}
		}

		newline = false;
	}
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
		// clear the previous book memory..
		if ( books )
		{
			for ( int c = 0; c < numbooks; c++ )
			{
				if ( books[c] )
				{
					if ( books[c]->name )
					{
						free(books[c]->name);
					}
					if ( books[c]->text )
					{
						free(books[c]->text);
					}
					if ( books[c]->bookgui_render_title )
					{
						free(books[c]->bookgui_render_title);
					}
					list_FreeAll(&books[c]->pages);
					free(books[c]);
				}
			}
			free(books);
		}
		createBooks();
	}
}
