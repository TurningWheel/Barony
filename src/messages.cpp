/*-------------------------------------------------------------------------------

	BARONY
	File: messages.cpp
	Desc: implements messages that draw onto the screen and then fade
	away after a while.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "draw.hpp"
#include "messages.hpp"
#include "player.hpp"
#include <regex>

const int MESSAGE_LIST_SIZE_CAP = 400;

void messageDeconstructor(void* data)
{
	if (data != NULL)
	{
		Message* message = (Message*)data;
		stringDeconstructor((void*)message->text);
		free(data);
	}
}

//Add a message to notification_messages (the list of messages) -- and pop off (and delete) and excess messages.
void Player::MessageZone_t::addMessage(Uint32 color, const char* content)
{
    if (content == nullptr) {
        return;
    }

	//Find out how many lines we need.
	size_t content_len = 0;
	int lines_needed = 1;
	for (int c = 0; content[c] != '\0'; ++c, ++content_len) {
		if (content[c] == '\n') {
			++lines_needed;
		}
	}

	//First check number of lines all messages currently are contributing.
	int line_count = 0;
	for (Message *m : notification_messages)
	{
		line_count += m->text->lines;
	}

    //Clear space in the message log
    int total_lines = line_count + lines_needed;
	while (!notification_messages.empty() && total_lines > getMaxTotalLines()) {
		Message *msg = notification_messages.back();
		if (msg->text->lines > 1 && total_lines - msg->text->lines < getMaxTotalLines()) {
		    int lines_to_delete = total_lines - getMaxTotalLines();
		    int lines = 1;
		    for (int c = 0; msg->text->data[c] != '\0'; ++c) {
		        if (msg->text->data[c] == '\n') {
		            if (lines == lines_to_delete) {
		                size_t len = strlen(msg->text->data);
		                memmove(msg->text->data, msg->text->data + c, len - c);
		                msg->text->data[c] = '\0';
		                msg->text->lines -= lines_to_delete;
		                total_lines -= lines_to_delete;
		            }
		            ++lines;
		        }
		    }
		    if (msg->text->lines > lines) {
		        // how did this message record more lines than it has?
		        msg->text->lines = lines;
		    }
		} else {
		    total_lines -= msg->text->lines;
		    messageDeconstructor(msg);
		    notification_messages.pop_back();
		}
	}

	//Allocate the new message.
	Message* new_message = NULL;
	if ((new_message = (Message*) malloc(sizeof(Message))) == NULL)
	{
		printlog( "failed to allocate memory for new message!\n"); //Yell at the user.
		exit(1);
	}
	//Assign the message's text.
	{
		if ((new_message->text = (string_t*) malloc(sizeof(string_t))) == NULL)
		{
			printlog( "[addMessage()] Failed to allocate memory for new string!\n" );
			exit(1); //Should it do this?
		}

		new_message->text->color = color;
		new_message->text->lines = 1;

		char str[ADD_MESSAGE_BUFFER_LENGTH] = { '\0' };

		int additionalCharacters = 0;
		strncpy(str, messageSanitizePercentSign(content, &additionalCharacters).c_str(), sizeof(str) - 1);
		int i = content_len + additionalCharacters;

		new_message->text->data = (char*) malloc(sizeof(char) * (i + 1));
		if (new_message->text->data == NULL)
		{
			printlog( "Failed to allocate memory for new message's text!\n"); //Yell at user.
			exit(1);
		}
		memset(new_message->text->data, 0, sizeof(char) * (i + 1));
		new_message->text->lines = lines_needed;
		strncpy(new_message->text->data, str, i);
	}
	new_message->time_displayed = 0; //Currently been displayed for 0 seconds.
	new_message->alpha = SDL_ALPHA_OPAQUE; //Set the initial alpha.
	//Initialize these two to NULL values -- cause no neighbors yet!

	notification_messages.push_front(new_message);

	//Update the position of the other messages;
	Message *prev = nullptr;
	for (Message *m : notification_messages)
	{
		prev = m;
	}
}

static ConsoleVariable<int> cvar_messages_max_lines("/messages_max_lines", 7);

int Player::MessageZone_t::getMaxTotalLines()
{
	return *cvar_messages_max_lines;
}

void Player::MessageZone_t::startMessages()
{
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		players[i]->messageZone.old_sdl_ticks = SDL_GetTicks();
	}
}

void Player::MessageZone_t::updateMessages()
{
	int time_passed = 0;

	if (old_sdl_ticks == 0)
	{
		old_sdl_ticks = SDL_GetTicks();
	}

	time_passed = SDL_GetTicks() - old_sdl_ticks;
	old_sdl_ticks = SDL_GetTicks();

	// limit the number of onscreen messages to reduce spam
	int c = 0;
	int currentline = 0;
	for (auto it = notification_messages.begin(); it != notification_messages.end(); it++)
	{
		Message *msg = *it;

		// Start fading all messages beyond index immediately
		c++;
		if ( currentline >= MESSAGE_MAX_ENTRIES - 2 && msg->time_displayed < MESSAGE_PREFADE_TIME)
		{
			msg->time_displayed = MESSAGE_PREFADE_TIME;
		}

		// Increase timer for message to start fading
		msg->time_displayed += time_passed;

		// Fade it if appropriate
		if (msg->time_displayed >= MESSAGE_PREFADE_TIME)
		{
			msg->alpha -= MESSAGE_FADE_RATE;
		}

		if ( msg->text )
		{
			currentline += msg->text->lines;
		}
		else
		{
			++currentline;
		}
	}
	// Remove all completely faded messages
	for (auto it = notification_messages.begin(); it != notification_messages.end(); )
	{
		Message *msg = *it;
		if (msg->alpha <= SDL_ALPHA_TRANSPARENT)
		{
			messageDeconstructor(msg);
			it = notification_messages.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void Player::MessageZone_t::drawMessages()
{
	return;
	//for ( Message *current : notification_messages )
	//{
	//	Uint32 color = (current->text->color & 0x00ffffff) | ((Uint32)current->alpha << 24);

	//	// highlights in messages.
	//	std::string data = current->text->data;
	//	size_t findHighlight = data.find("[");
	//	bool doHighlight = false;
	//	if ( findHighlight != std::string::npos )
	//	{
	//		std::string highlightedWords = data;
	//		for ( int i = 0; i < highlightedWords.size(); ++i )
	//		{
	//			if ( highlightedWords[i] != '\n' && highlightedWords[i] != '\0' )
	//			{
	//				highlightedWords[i] = ' '; // replace all characters with a space.
	//			}
	//		}
	//		size_t highlightedIndex = 0;
	//		while ( findHighlight != std::string::npos )
	//		{
	//			size_t findHighlightEnd = data.find("]", findHighlight);
	//			if ( findHighlightEnd != std::string::npos )
	//			{
	//				doHighlight = true;
	//				size_t numChars = findHighlightEnd - findHighlight + 1;

	//				// add in the highlighed words.
	//				highlightedWords.replace(findHighlight, numChars, data.substr(findHighlight, numChars));

	//				std::string padding(numChars, ' ');
	//				// replace the words in the old string with spacing.
	//				data.replace(findHighlight, numChars, padding);
	//				findHighlight += numChars;
	//			}
	//			else
	//			{
	//				break;
	//			}
	//			findHighlight = data.find("[", findHighlight);
	//			highlightedIndex = findHighlight;
	//		}
	//		if ( doHighlight )
	//		{
	//			ttfPrintTextFormattedColor(font, current->x, current->y, color, "%s", data.c_str());

	//			Uint32 color = (makeColorRGB(0, 192, 255) & 0x00ffffff) | ((Uint32)current->alpha << 24);
	//			ttfPrintTextFormattedColor(font, current->x, current->y, color, "%s", highlightedWords.c_str());
	//		}
	//	}
	//	if ( !doHighlight )
	//	{
	//		ttfPrintTextFormattedColor(font, current->x, current->y, color, current->text->data);
	//	}
	//}
}

void Player::MessageZone_t::deleteAllNotificationMessages()
{
	std::for_each(notification_messages.begin(), notification_messages.end(), messageDeconstructor);
	notification_messages.clear();
}

std::string messageSanitizePercentSign(std::string src, int* percentSignsFound)
{
	// sanitize input string for print commands.
	std::string commandString = src;
	std::size_t found = commandString.find('%');
	if ( !commandString.empty() )
	{
		while ( found != std::string::npos )
		{
			if ( found + 1 < commandString.size() )
			{
				if ( commandString.at(found + 1) == '%' )
				{
					// this command has already been escaped, no need for more %
					found = commandString.find('%', found + 2);
					continue;
				}
			}
			commandString.insert(found, "%");
			found = commandString.find('%', found + 2);
			if ( percentSignsFound )
			{
				++(*percentSignsFound);
			}
		}
	}
	return commandString;
}
