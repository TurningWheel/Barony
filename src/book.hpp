/*-------------------------------------------------------------------------------

	BARONY
	File: book.hpp
	Desc: declarations and such for readable books

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include "main.hpp"

//TODO: The book name will need to be replaced with books[item->appearnce%numbooks]->name as opposed to item->name in the inventory.
//typedef struct book_t
//{
//	char* text; //The total book's text. Do not render this.
//
//	char* name; //The book's name. Must be created before the pages are created.
//	char* bookgui_render_title; //The name converted so that it's ready to render in the book GUI. Splits it across lines and everything.
//	int bookgui_render_title_numlines; //How many lines the title takes up.
//
//	list_t pages; //The pages in the book. Make a node, point it at this, iterate the whole book and render the text the node gives you access to. This list should be created in init or whenever the book GUI's read area changes. Or maybe it should be created runtime for the book GUI. We'll see. In either case, do something to account for the GUI changing size.
//} book_t;
//void formatTitle(book_t* book); //Prepares the book's title for rendering in the book GUI.
//int getBook(char const * const booktitle); // returns the appearance index number for the book with the given title
//extern book_t** books;


class Book_t
{
public:
	Book_t() {};
	std::string text = "";
	std::string name = "";
	std::vector<std::string> formattedPages;
};
extern std::vector<Book_t> allBooks;

class BookParser_t
{
	static const int MAX_FILE_LENGTH = 192000;
	const int versionJSON = 1;
public:
	bool booksRequireCompiling();
	bool readCompiledBooks();
	void writeCompiledBooks();
	void createBooks(bool forceCacheRebuild);
	void createBook(std::string filename); //Take a book filename and generate all of its pages.
	void deleteBooks();
	std::unordered_map<std::string, std::string> tempBookData;
	void readBooksIntoTemp();
	std::list<std::string> getListOfBooksAfterFiltering();
};
extern BookParser_t bookParser_t;

int getBook(std::string bookTitle);
std::string getBookNameFromIndex(int index);
extern int numbooks;

