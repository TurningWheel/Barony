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
	    char filename2[PATH_MAX];
	    strcpy( filename2, "Screenshot " );
	    time_t timer;
	    char buffer[32];
	    struct tm* tm_info;
	    time(&timer);
	    tm_info = localtime(&timer);
	    strftime( buffer, 32, "%Y-%m-%d %H-%M-%S", tm_info );
	    strcat( filename2, buffer );
	    strcat( filename2, ".png" );
        (void)completePath(filename, filename2, outputdir);
	}

	temp = SDL_CreateRGBSurface(0, xres, yres, 32, 0, 0, 0, 0);
	SDL_LockSurface(temp);
	glReadPixels(0, 0, xres, yres, GL_BGRA, GL_UNSIGNED_BYTE, temp->pixels);
	SDL_UnlockSurface(temp);
	temp2 = flipSurface( temp, FLIP_VERTICAL );
	SDL_FreeSurface( temp );
	temp = SDL_CreateRGBSurface(0, xres, yres, 24, 0, 0, 0, 0);
	SDL_FillRect(temp, NULL, 0);
	SDL_BlitSurface(temp2, NULL, temp, NULL);
	SDL_FreeSurface( temp2 );
	//TODO Nintendo needs to be able to do this!
	//Otherwise there are no screenshots for save games!
#ifndef NINTENDO
	SDL_SavePNG( temp, filename );
#endif
	SDL_FreeSurface( temp );
	if ( !intro && !output_path )
	{
		messagePlayer(clientnum, "%s", filename);
	}
	else
	{
		printlog("%s", filename);
	}
}
