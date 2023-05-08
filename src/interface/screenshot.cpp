/*-------------------------------------------------------------------------------

	BARONY
	File: screenshot.cpp
	Desc: contains takeScreenshot()

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../draw.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../net.hpp"
#include "../files.hpp"
#include "interface.hpp"

#ifdef NINTENDO
#include "../nintendo/baronynx.hpp"
#endif

/*-------------------------------------------------------------------------------

	takeScreenshot

	takes a screenshot of the game and saves it in the current directory

-------------------------------------------------------------------------------*/

void takeScreenshot(const char* output_path)
{
	char filename[PATH_MAX];
	SDL_Surface* temp, *temp2;

    if (output_path) {
        (void)completePath(filename, output_path, outputdir);
    } else {
        char buffer[32];
	    char filename2[PATH_MAX];
	    strcpy(filename2, "Screenshot ");
        getTimeAndDateFormatted(getTime(), buffer, sizeof(buffer));
	    strcat(filename2, buffer);
	    strcat(filename2, ".png");
        (void)completePath(filename, filename2, outputdir);
	}

	temp = SDL_CreateRGBSurface(0, xres, yres, 32, 0, 0, 0, 0);
	SDL_LockSurface(temp);
    GL_CHECK_ERR(glReadPixels(0, 0, xres, yres, GL_BGRA, GL_UNSIGNED_BYTE, temp->pixels));
	SDL_UnlockSurface(temp);
	temp2 = flipSurface( temp, FLIP_VERTICAL );
	SDL_FreeSurface( temp );
	temp = SDL_CreateRGBSurface(0, xres, yres, 24, 0, 0, 0, 0);
	SDL_FillRect(temp, NULL, 0);
	SDL_BlitSurface(temp2, NULL, temp, NULL);
	SDL_FreeSurface( temp2 );
	SDL_SavePNG( temp, filename );
	SDL_FreeSurface( temp );
	if ( !intro && !output_path )
	{
		messagePlayer(clientnum, MESSAGE_MISC, "%s", filename);
	}
	else
	{
		printlog("%s", filename);
	}
}
