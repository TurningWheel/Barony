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
typedef struct book_t
{
	char* text; //The total book's text. Do not render this.

	char* name; //The book's name. Must be created before the pages are created.
	char* bookgui_render_title; //The name converted so that it's ready to render in the book GUI. Splits it across lines and everything.
	int bookgui_render_title_numlines; //How many lines the title takes up.

	list_t pages; //The pages in the book. Make a node, point it at this, iterate the whole book and render the text the node gives you access to. This list should be created in init or whenever the book GUI's read area changes. Or maybe it should be created runtime for the book GUI. We'll see. In either case, do something to account for the GUI changing size.
} book_t;

extern book_t** books;
extern int numbooks;
extern list_t* discoveredbooks;

void createBooks();
void createBook(book_t* book); //Take's a book and generates all of its pages.

void formatTitle(book_t* book); //Prepares the book's title for rendering in the book GUI.
int getBook(char* booktitle); // returns the appearance index number for the book with the given title
