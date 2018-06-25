/*-------------------------------------------------------------------------------

	BARONY
	File: messages.hpp
	Desc: defines stuff for messages that draw onto the screen and then
	fade away after a while.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

//#include "SDL.h"

#include "main.hpp"
#include "interface/interface.hpp"

//Time in seconds before the message starts fading.
#define MESSAGE_PREFADE_TIME 3600
//How fast the alpha value de-increments
#define MESSAGE_FADE_RATE 10
/*
 * Maximum number of messages displayed on screen at once before the oldest message is automatically deleted.
 * Currently calculated as the maximum number of messages from the top of the screen to the status panel.
 */
#define MESSAGE_FONT ttf16
#define MESSAGE_FONT_SIZE TTF_FontHeight(MESSAGE_FONT)
#define MESSAGE_MAX_TOTAL_LINES ((yres - STATUS_BAR_Y_OFFSET) / MESSAGE_FONT_SIZE)
//Number of pixels from the left edge of the screen the messages are.
#define MESSAGE_X_OFFSET 5
//The location the newest message is displayed (in other words, the bottom of the message list -- they're drawn from oldest to newest, top down).
#define MESSAGE_Y_OFFSET (yres-STATUS_BAR_Y_OFFSET-MESSAGE_FONT_SIZE-20-(60 * uiscale_playerbars * uiscale_playerbars))

/*
 * Right, so this is how it's going to work:
 * This is a "class" to emulate a virtual console -- minecraft style. I mean, message log, not console.
 * It draws messages up above the main bar and stuff. It...ya. Minecraft messages pop up, dissapear, you know?
 * This is what that does.
 */

typedef struct Message
{
	string_t* text; //Same size as the message in draw.c. Make sure not to overrun it.

	//Its location (durr).
	int x, y;

	//The time it's been displayed so far.
	int time_displayed;

	//The alpha of the message (SDL > 1.1.5, or whatever version it was, has 255 as SDL_ALPHA_OPAQUE and 0 as ASL_ALPHA_TRANSPARENT).
	/*
	 * Building on that last point, we could probably:
		if (SDL_ALPHA_TRANSPARENT < SDL_ALPHA_OPAQUE)
		{
			alpha--;
		}
		else
		{
			alpha++;
		}
	 * To ensure everything always works right. I guess. Maybe not necessary. Whatever. There are much bigger problems to worry about.
	 */
	Sint16 alpha;
} Message;

/*
 * Adds a message to the list of messages.
 */
void addMessage(Uint32 color, char* content, ...);

/*
 * Updates all the messages; fades them & removes them.
 */
void updateMessages();

/*
 * Draw all the messages.
 */
void drawMessages();


/*
 * Used on program deinitialization.
 */
void deleteAllNotificationMessages();
