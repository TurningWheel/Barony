/*-------------------------------------------------------------------------------

	BARONY
	File: book.cpp
	Desc: implements readable books

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "interface/interface.hpp"
#include "book.hpp"

//#define SPACE_NINJA_NAME "Order of the Space Ninjas"
#define TAB_WIDTH 3
//Any word less than this will just get bumped onto the next line.
#define MIN_LENGTH_TO_SPLIT_WORD (book_characterspace_x / 2)
//#define MIN_LENGTH_TO_SPLIT_WORD_TITLE (characterspace_x / 2) //This only works in the formatTitle() function, since it uses a local variable characterspace_x in there.
#define SPLIT_WORD_IN_TITLE FALSE //Whether or not to split a word in the book's title. If set to false, will only split words if they have to be split. If set to true, will split workds if they're a minimum length of MIN_LENGTH_TO_SPLIT_WORD_TITLE.

book_t **books = NULL;
int numbooks = 0;
list_t *discoveredbooks = NULL;

//book_t *book_space_ninjas = NULL;

int getBook(char *booktitle) {
	int c;
	for( c = 0; c < numbooks; c++ ) {
		if( !strcmp(booktitle, books[c]->name) ) {
			return c;
		}
	}
	return 0;
}

void createBooks() {
	node_t *node;
	string_t *name = NULL;
	bool unsorted;
	int i = 0;

	//TODO: Read the books/ inventory for all *.txt files.
	//TODO: Then create a book for each file there and add it to a books array.
	discoveredbooks = directoryContents("books/");
	if (discoveredbooks) {
		printlog("compiling books...\n");
		if (discoveredbooks->first && discoveredbooks->last) {
			// allocate memory for books
			numbooks = list_Size(discoveredbooks);
			books = (book_t **) malloc(sizeof(book_t *) * numbooks); //Allocate memory for all of the books.

			// sort books alphabetically (bubblesort)
			do {
				unsorted = FALSE;
				for(node = discoveredbooks->first; node != NULL; node = node->next) {
					if( node->next != NULL ) {
						string_t *firststring = (string_t *)node->element;
						string_t *secondstring = (string_t *)node->next->element;
						if( strcmp(firststring->data, secondstring->data) > 0 ) {
							unsorted = TRUE;
							node->element = secondstring;
							node->next->element = firststring;
							firststring->node = node->next;
							secondstring->node = node;
						}
					} else {
						break;
					}
				}
			} while( unsorted );

			// create books
			for(node = discoveredbooks->first, i = 0; node != NULL; node = node->next, ++i) {
				books[i] = (book_t *) malloc(sizeof(book_t));
				books[i]->text = NULL;
				name = (string_t *)node->element;
				books[i]->name = name->data;
				//printlog("compiling book: \"%s\"\n",books[i]->name);
				books[i]->bookgui_render_title = NULL;
				books[i]->bookgui_render_title_numlines = 0;
				books[i]->pages.first = NULL;
				books[i]->pages.last = NULL;
				//formatTitle(books[i]);
				createBook(books[i]);
				books[i]->name[strlen(books[i]->name) - 4] = 0;
			}
		} else {
			printlog( "Warning: discoveredbooks->first and last do not exist. No books in /books/ directory?\n");
		}
	}
}

/*****createBook() helper functions******/

bool isLetter(char character) {
	switch (tolower(character)) {
		case 'a':
			return TRUE;
		case 'b':
			return TRUE;
		case 'c':
			return TRUE;
		case 'd':
			return TRUE;
		case 'e':
			return TRUE;
		case 'f':
			return TRUE;
		case 'g':
			return TRUE;
		case 'h':
			return TRUE;
		case 'i':
			return TRUE;
		case 'j':
			return TRUE;
		case 'k':
			return TRUE;
		case 'l':
			return TRUE;
		case 'm':
			return TRUE;
		case 'n':
			return TRUE;
		case 'o':
			return TRUE;
		case 'p':
			return TRUE;
		case 'q':
			return TRUE;
		case 'r':
			return TRUE;
		case 's':
			return TRUE;
		case 't':
			return TRUE;
		case 'u':
			return TRUE;
		case 'v':
			return TRUE;
		case 'w':
			return TRUE;
		case 'x':
			return TRUE;
		case 'y':
			return TRUE;
		case 'z':
			return TRUE;
		default:
			return FALSE;
	}
	return FALSE;
}

//This is a more powerful version of isLetter that checks if a specified character is part of a word. However, it requires contextual information -- what are the next and previous characters? So pass the entire string to this function and the index in the string of the character being looked up.
bool isCharacterPartOfWord(char *text, int index) {
	if (!text) {
		return FALSE;
	}
	if (index < 0 || index > strlen(text)) {
		return FALSE;
	}

	if (isLetter(text[index])) {
		return TRUE;    //It's a character. Return true because no use continuing this function.
	}

	switch (text[index]) {
		case '\'':
			if (isLetter(text[index - 1]) && isLetter(text[index + 1])) { //An apostrophe needs to be surrounded by letters to qualify as part of a word.
				return TRUE;
			}
			return FALSE;
		default:
			return FALSE;
	}

	return FALSE;
}

int moveToStartOfWord(char *text, int index) {
	if (!text) {
		return index;
	}
	if (index < 0 || index > strlen(text)) {
		return index;
	}

	int i = index;

	//Keep running backwards until it finds a non-letter character (which would be the character before the start of the word).
	for (; i - 1 > 0; --i) {
		if (!isCharacterPartOfWord(text, i - 1)) {
			return i;
		}
	}

	return i;
}

//Returns 0 on error. Returns 0 if index not on a word. Returns length of word otherwise. If nonletter character at the current index, it keeps looking until it finds the start of the next word.
int lengthOfCurrentWord(char *text, int index) {
	if (!text) { //Can't do this without text.
		return 0;
	}
	if (index < 0 || index > strlen(text)) { //Index has to be in bounds.
		return 0;
	}

	int length = 0;
	int i = index;

	if (!isCharacterPartOfWord(text, i)) //The current character is not part of a word.
		//i = moveToNextWord(text, index); //Find the start of the next word.
	{
		return 0;    //Not our problem. Tell it the current word is length 0.
	} else if (i > 0 && isCharacterPartOfWord(text, i)) { //Not at the start of the text array and the previous character is a letter...yikes, not at the start of the word.
		i = moveToStartOfWord(text, i);    //Move to the start of the current word.
	}

	for(; i < strlen(text); ++i) {
		if (isCharacterPartOfWord(text, i)) {
			length++;    //The current character is part of the word.
		} else {
			return length;    //Reached the end of the word. Return the length.
		}
	}

	return length;
}

void createBook(book_t *book) {
	if (!book) {
		return;
	}

	//Load in the text from a file.
	strcpy(tempstr, "books/");
	strcat(tempstr, book->name);
	book->text = readFile(tempstr);
	if (!book->text) {
		printlog( "error opening book \"%s\".\n", tempstr);
		return; //Failed to open the file.
	}

	int book_characterspace_x = BOOK_PAGE_WIDTH / BOOK_FONT_WIDTH;
	int book_characterspace_y = BOOK_PAGE_HEIGHT / BOOK_FONT_HEIGHT;
	int max_characters = book_characterspace_x * book_characterspace_y;

	book->pages.first = NULL;
	book->pages.last = NULL;

	Uint32 color = SDL_MapRGBA(mainsurface->format, 0, 0, 0, 255);
	string_t *string = newString(&book->pages, color, NULL);
	string->data = (char *) malloc(sizeof(char) * (max_characters + 1));
	memset(string->data, 0, sizeof(char) * (max_characters + 1));

	int i; // current character in the book's entire text
	int p = 0; // current character in the page's text
	int x = 0; // number of characters written on the current line
	int y = 0; // number of lines on the page
	bool newline = FALSE;
	bool can_write = TRUE; //If false, it means that the character write to page was interecepted by a - to properly break up a word.
	int tab = 0; //Inserting tab characters.
	char character_to_record  = ' ';
	bool write_out = TRUE;
	//found_word and word_length are used to prevent smaller words from being broken up. When the for loop detects that it has hit the start of a word, it queries for the word's length. If the word's length < MIN_LENGTH_TO_SPLIT_WORD, then it pumps out a newline and then starts the word. word_length_left is there so that it knows how many more characters it has to go through to reach the end of the word.
	bool found_word = FALSE;
	int word_length = 0;
	for (i = 0; book->text[i] != 0; ++i) {
		//So first iterate through and count every line.
		//Line end conditions:
		//1. Encountered a newline character.
		//2. Hit the max amount of characters that can fit horizontally in the book GUI.
		write_out = TRUE;
		newline = FALSE;
		can_write = TRUE;

		//When the max amount of lines that can fit on one page have been hit, create a new page.
		if (tab <= 0 && book->text[i] == '\t') { //Assuming it is a null terminated string.
			//Found a newline. Create a new line.
			tab = TAB_WIDTH;
		}
		if (x + 1 >= book_characterspace_x) {
			//Overflowed the line. Go onto the next line.
			newline = TRUE;
		} else if (tab <= 0 && book->text[i] == '\n') {
			newline = TRUE;
			write_out = FALSE; //Do not want to write out this /n.
		}

		character_to_record = book->text[i];

		if (newline) {
			x = 0; //Reset x since it's back to the beginning of the line.
			if (y + 1 >= book_characterspace_y) {
				//Create the next page. Do not record the character if it's a newline.
				string = newString(&book->pages, color, NULL);
				string->data = (char *) malloc(sizeof(char) * (max_characters + 1));
				memset(string->data, 0, sizeof(char) * (max_characters + 1));
				p = 0;
				y = 0;
			} else {
				//Record the new line.
				string->data[p] = '\n';
				p++;
				y++;
				//No need to record the item in book->text[i] because it was a newline.
			}
			//Bugger the tab away if it hit the end of the line.
			if (tab > 0) {
				if (tab < TAB_WIDTH) {
					i--;    //Do this too? Or i++? I don't know.
				}
				tab = 0;
			}
			// no spaces at the start of a line
			if( character_to_record == ' ' ) {
				continue;
			}
		} else if (tab > 0) {
			character_to_record = ' ';
		}

		if (write_out) {
			//Ok, record the character.

			//First, check if it's found a word.
			if (!found_word && tab <= 0) { //Don't check for a word if already found one (to save processing) and don't bother checking if tab mode is active.
				if (isCharacterPartOfWord(book->text, i)) {
					//The character is part of a word. Record that.
					found_word = TRUE;
					word_length = lengthOfCurrentWord(book->text, i);

					//Now check if the word will fit on the line and if it's okay to split it. If not okay to split it, make a new line.
					if (word_length <= MIN_LENGTH_TO_SPLIT_WORD) {
						//The word doesn't like being split.
						if ((x + 1) + word_length >= book_characterspace_x && word_length < book_characterspace_x) {
							//So make a new line.
							x = 0; //Reset x, since it's writing out a new line and x gets reset when we do that.
							if (y + 1 >= book_characterspace_y) { //Check if it hit the end of the page.
								//It does indeed go off the page. Start a new page.
								string = newString(&book->pages, color, NULL);
								string->data = (char *) malloc(sizeof(char) * (max_characters + 1));
								memset(string->data, 0, sizeof(char) * (max_characters + 1));
								p = 0;
								y = 0;
							} else {
								string->data[p] = '\n';
								p++;
								y++;
							}
						}
					}
				}
			} else if (found_word) {
				if (!isCharacterPartOfWord(book->text, i)) {
					//Reached the end of the word.
					found_word = FALSE;
					word_length = 0;
				}
			}

			if (x + 2 >= book_characterspace_x) {
				if (isCharacterPartOfWord(book->text, i)) {
					if (book->text[i - 1] != ' ') { //First check if the previous character is not a space.
						//If it is not a space (and the like), write out a space since this one is character, the previous one isn't, and the newline is interrupting a word.
						string->data[p] = '-';
						p++;
						x++;
						can_write = FALSE;
						i--;
					} else if (isCharacterPartOfWord(book->text, i + 1)) { //Okay, so the previous character is a space (or otherwise not constituting a word. Check if the next character is not a space (or the like)).
						//Next character is not a space (and the like), write out a space so that the word's start is delayed until a safer point in time.
						string->data[p] = ' ';
						p++;
						x++;
						can_write = FALSE;
						i--;
					}
				}
			}

			if (can_write) {
				(string->data)[p] = character_to_record;
				p++;
				x++;
			} else {
				can_write = TRUE;
			}

			if (tab > 0) {
				if (tab < TAB_WIDTH) {
					i--;    //To make sure it doesn't bugger the character.
				}
				tab--;
			}
		}

		newline = FALSE;
	}
}
