/*-------------------------------------------------------------------------------

	BARONY
	File: messages.cpp
	Desc: implements messages that draw onto the screen and then fade
	away after a while.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "messages.hpp"
#include "main.hpp"

Uint32 old_sdl_ticks;

/*
 * Removes the last message from the list of messages. Don't you worry yourself about this function, it's only needed in this file.
 * No generic remove message function needed, the only message that needs to be removed is the last one -- when it fades away or gets pushed off.
 * It's always the first to fade away and it's always the one pushed off when there are too many messages.
 */
void removeLastMessage() {
	if (!notification_messages.last) {
		return;
	}

	list_RemoveNode(notification_messages.last);
}

void messageDeconstructor(void* data) {
	if (data != NULL) {
		Message* message = (Message*)data;
		stringDeconstructor((void*)message->text);
		free(data);
	}
}

void addMessage(Uint32 color, char* content, ...) {
	//Add a message to notification_messages (the list of messages) -- and pop off (and delete) and excess messages.

	//Find out how many lines we need.
	int lines_needed = 0;
	{
		va_list argptr;
		int c, i;
		char str[1024] = { 0 };

		if (content != NULL) {
			va_start(argptr, content);
			i = vsnprintf(str, 1023, content, argptr);
			va_end(argptr);
			for (c = 0; c < i; c++) {
				if (str[c] == 10 || c == i - 1) { //Line feed
					lines_needed++;
				}
			}
		} else {
			lines_needed = 0;
		}
	}

	node_t* node = NULL;

	//First check number of lines all messages currently are contributing.
	int line_count = 0;
	if (notification_messages.first) {
		node = notification_messages.first;
		Message* current = (Message* )node->element;
		line_count += current->text->lines;

		while (node->next) {
			node = node->next;
			current = (Message* )node->element;
			line_count += current->text->lines;
		}
	}

	if (MESSAGE_MAX_TOTAL_LINES > 0) {
		while (line_count > (MESSAGE_MAX_TOTAL_LINES - lines_needed) && notification_messages.first) {
			//Too many lines. Remove messages until either there are enough lines or all messages are gone (the latter case occurs when the new message is more lines than the total lines allowed).
			node = notification_messages.first;
			while (node->next) {
				node = node->next;
			}
			//TODO: Instead of just removing it, check to see if it's multiline. If it is, check to see if removing some of its lines, not the entire thing, will give enough space. If so, do that, rather than outright deleting the entire thing.
			Message* last = (Message* )node->element;
			line_count -= last->text->lines;
			removeLastMessage(); //Remove last message, cause there's too many!
		}
	} else {
		printlog( "Error, MESSAGE_MAX_TOTAL_LINES is too small (must be 1 or greater!)\n"); //Something done broke.
		exit(1);
	}

	//Allocate the new message.
	Message* new_message = NULL;
	if ((new_message = (Message*) malloc(sizeof(Message))) == NULL) {
		printlog( "failed to allocate memory for new message!\n"); //Yell at the user.
		exit(1);
	}
	//Assign the message's text.
	{
		va_list argptr;
		int c, i;
		char str[1024] = { 0 };

		if ((new_message->text = (string_t*) malloc(sizeof(string_t))) == NULL) {
			printlog( "[addMessage()] Failed to allocate memory for new string!\n" );
			exit(1); //Should it do this?
		}

		new_message->text->color = color;
		new_message->text->lines = 1;
		if (content != NULL) {
			//Format the content.
			va_start(argptr, content);
			i = vsnprintf(str, 1023, content, argptr);
			va_end(argptr);
			new_message->text->data = (char*) malloc(sizeof(char) * (i + 1));
			if (new_message->text->data == NULL) {
				printlog( "Failed to allocate memory for new message's text!\n"); //Yell at user.
				exit(1);
			}
			memset(new_message->text->data, 0, sizeof(char) * (i + 1));
			for (c = 0; c < i; ++c) {
				if (str[c] == 10) { //Line feed
					new_message->text->lines++;
				}
			}
			strncpy(new_message->text->data, str, i);

			//Make sure we don't exceed the maximum number of lines permissible.
			if (line_count + new_message->text->lines > MESSAGE_MAX_TOTAL_LINES) {
				//Remove lines until we have an okay amount.
				while (line_count + new_message->text->lines > MESSAGE_MAX_TOTAL_LINES) {
					for (c = 0; c < i; ++c) {
						if (str[c] == 10) { //Line feed
							new_message->text->lines--; //First let it know it's now one less line.
							char* temp = new_message->text->data; //Point to the message's text so we don't lose it.
							new_message->text->data = (char*) malloc(sizeof(char) * ((i + 1) - c));  //Give the message itself new text.
							if (new_message->text->data == NULL) { //Error checking.
								printlog( "Failed to allocate memory for new message's text!\n"); //Yell at user.
								exit(1);
							}
							memmove(new_message->text->data, temp + (c - 1), strlen(temp)); //Now copy the text into the message's new allocated memory, sans the first line.
							free(temp); //Free the old memory so we don't memleak.
							break;
						}
					}
				}
			}
		} else {
			new_message->text->data = NULL;
		}
	}
	new_message->time_displayed = 0; //Currently been displayed for 0 seconds.
	new_message->alpha = SDL_ALPHA_OPAQUE; //Set the initial alpha.
	//Initialize these two to NULL values -- cause no neighbors yet!
	new_message->node = NULL;

	//Replace the root message.
	node = list_AddNodeFirst(&notification_messages);
	node->element = new_message;
	node->deconstructor = &messageDeconstructor;
	new_message->node = node;

	//Set the message location.
	new_message->x = MESSAGE_X_OFFSET;
	new_message->y = MESSAGE_Y_OFFSET - MESSAGE_FONT_SIZE * new_message->text->lines;

	//Update the position of the other messages;
	int count = 0;
	Message* current = NULL;
	node = notification_messages.first;
	while (node->next) {
		count++;
		node = node->next;
		current = (Message* )node->element;
		current->y = ((Message*)node->prev->element)->y - MESSAGE_FONT_SIZE * current->text->lines;
	}
}

void updateMessages() {
	int time_passed = 0;

	if (old_sdl_ticks == 0) {
		old_sdl_ticks = SDL_GetTicks();
	}

	time_passed = SDL_GetTicks() - old_sdl_ticks;
	old_sdl_ticks = SDL_GetTicks();
	node_t* node = NULL, *nextnode = NULL;
	Message* current = NULL;

	// limit the number of onscreen messages to reduce spam
	int c = 0;
	node_t* tempNode = notification_messages.last;
	while ( list_Size(&notification_messages) - c > 10 ) {
		current = (Message*)tempNode->element;
		current->time_displayed = MESSAGE_PREFADE_TIME;
		tempNode = tempNode->prev;
		c++;
	}

	for ( node = notification_messages.first; node != NULL; node = nextnode ) {
		nextnode = node->next;
		current = (Message* )node->element;
		if (current->time_displayed < MESSAGE_PREFADE_TIME) {
			if (time_passed > 0) {
				current->time_displayed += time_passed; // less time before fade
			}
		} else {
			if (current->alpha <= SDL_ALPHA_TRANSPARENT) {
				list_RemoveNode(node); // transparent message, deleted message :)
			} else {
				current->alpha -= MESSAGE_FADE_RATE; // fade message
				if ( current->alpha < 0 ) {
					current->alpha = 0;
				}
			}
		}
	}
}

void drawMessages() {
	node_t* node;
	Message* current;

	for ( node = notification_messages.first; node != NULL; node = node->next ) {
		current = (Message*)node->element;

		Uint32 color = current->text->color ^ mainsurface->format->Amask;
		color += std::min<Sint16>(std::max<Sint16>(0, current->alpha), 255) << mainsurface->format->Ashift;
		ttfPrintTextFormattedColor(MESSAGE_FONT, current->x, current->y, color, current->text->data);
	}
}

void deleteAllNotificationMessages() {
	list_FreeAll(&notification_messages);
}

list_t notification_messages;
