/*-------------------------------------------------------------------------------

	BARONY
	File: messages.cpp
	Desc: implements messages that draw onto the screen and then fade
	away after a while.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "draw.hpp"
#include "messages.hpp"
#include "main.hpp"

Uint32 old_sdl_ticks;
std::list<Message*> notification_messages;

void messageDeconstructor(void* data)
{
	if (data != NULL)
	{
		Message* message = (Message*)data;
		stringDeconstructor((void*)message->text);
		free(data);
	}
}

void addMessage(Uint32 color, char* content, ...)
{
	//Add a message to notification_messages (the list of messages) -- and pop off (and delete) and excess messages.

	//Find out how many lines we need.
	int lines_needed = 0;
	{
		va_list argptr;
		int c, i;
		char str[1024] = { 0 };

		if (content != NULL)
		{
			va_start(argptr, content);
			i = vsnprintf(str, 1023, content, argptr);
			va_end(argptr);
			for (c = 0; c < i; c++)
			{
				if (str[c] == 10 || c == i - 1)   //Line feed
				{
					lines_needed++;
				}
			}
		}
		else
		{
			lines_needed = 0;
		}
	}

	//First check number of lines all messages currently are contributing.
	int line_count = 0;
	for (Message *m : notification_messages)
	{
		line_count += m->text->lines;
	}

	if (MESSAGE_MAX_TOTAL_LINES > 0)
	{
		for (auto last_message = notification_messages.rbegin(); last_message != notification_messages.rend(); last_message++) {
			if (line_count < (MESSAGE_MAX_TOTAL_LINES - lines_needed))
				break;
			line_count -= (*last_message)->text->lines;
			notification_messages.pop_back();
		}
	}
	else
	{
		printlog( "Error, MESSAGE_MAX_TOTAL_LINES is too small (must be 1 or greater!)\n"); //Something done broke.
		exit(1);
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
		va_list argptr;
		int c, i;
		char str[1024] = { 0 };

		if ((new_message->text = (string_t*) malloc(sizeof(string_t))) == NULL)
		{
			printlog( "[addMessage()] Failed to allocate memory for new string!\n" );
			exit(1); //Should it do this?
		}

		new_message->text->color = color;
		new_message->text->lines = 1;
		if (content != NULL)
		{
			//Format the content.
			va_start(argptr, content);
			i = vsnprintf(str, 1023, content, argptr);
			va_end(argptr);

			int additionalCharacters = 0;
			strncpy(str, messageSanitizePercentSign(str, &additionalCharacters).c_str(), ADD_MESSAGE_BUFFER_LENGTH - 1);
			i += additionalCharacters;

			new_message->text->data = (char*) malloc(sizeof(char) * (i + 1));
			if (new_message->text->data == NULL)
			{
				printlog( "Failed to allocate memory for new message's text!\n"); //Yell at user.
				exit(1);
			}
			memset(new_message->text->data, 0, sizeof(char) * (i + 1));
			for (c = 0; c < i; ++c)
			{
				if (str[c] == 10)   //Line feed
				{
					new_message->text->lines++;
				}
			}
			strncpy(new_message->text->data, str, i);

			//Make sure we don't exceed the maximum number of lines permissible.
			if (line_count + new_message->text->lines > MESSAGE_MAX_TOTAL_LINES)
			{
				//Remove lines until we have an okay amount.
				while (line_count + new_message->text->lines > MESSAGE_MAX_TOTAL_LINES)
				{
					for (c = 0; c < i; ++c)
					{
						if (str[c] == 10)   //Line feed
						{
							new_message->text->lines--; //First let it know it's now one less line.
							char* temp = new_message->text->data; //Point to the message's text so we don't lose it.
							new_message->text->data = (char*) malloc(sizeof(char) * ((i + 1) - c));  //Give the message itself new text.
							if (new_message->text->data == NULL)   //Error checking.
							{
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
		}
		else
		{
			new_message->text->data = NULL;
		}
	}
	new_message->time_displayed = 0; //Currently been displayed for 0 seconds.
	new_message->alpha = SDL_ALPHA_OPAQUE; //Set the initial alpha.
	//Initialize these two to NULL values -- cause no neighbors yet!

	notification_messages.push_front(new_message);

	//Set the message location.
	new_message->x = MESSAGE_X_OFFSET;
	new_message->y = MESSAGE_Y_OFFSET - MESSAGE_FONT_SIZE * new_message->text->lines;

	//Update the position of the other messages;
	Message *prev = nullptr;
	for (Message *m : notification_messages)
	{
		m->y = (prev ? prev->y : MESSAGE_Y_OFFSET) - MESSAGE_FONT_SIZE * m -> text->lines;
		prev = m;
	}
}

void updateMessages()
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
	for (auto it = notification_messages.begin(); it != notification_messages.end(); it++)
	{
		Message *msg = *it;

		// Start fading all messages beyond index 10 immediately
		c++;
		if (c > 10 && msg->time_displayed < MESSAGE_PREFADE_TIME)
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
	}
	// Remove all completely faded messages
	for (auto it = notification_messages.begin(); it != notification_messages.end(); it++)
	{
		Message *msg = *it;
		if (msg->alpha <= SDL_ALPHA_TRANSPARENT)
		{
			messageDeconstructor(msg);
			it = notification_messages.erase(it);
		}
	}
}

void drawMessages()
{
	for ( Message *current : notification_messages )
	{
		Uint32 color = current->text->color ^ mainsurface->format->Amask;
		color += std::min<Sint16>(std::max<Sint16>(0, current->alpha), 255) << mainsurface->format->Ashift;
		ttfPrintTextFormattedColor(MESSAGE_FONT, current->x, current->y, color, current->text->data);
	}
}

void deleteAllNotificationMessages()
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
