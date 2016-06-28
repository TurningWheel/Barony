/*-------------------------------------------------------------------------------

	BARONY
	File: screenshot.cpp
	Desc: contains takeScreenshot()

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../net.hpp"
#include "interface.hpp"

/*-------------------------------------------------------------------------------

	takeScreenshot
	
	takes a screenshot of the game and saves it in the current directory

-------------------------------------------------------------------------------*/

void takeScreenshot() {
	char filename[1024];
	SDL_Surface *temp, *temp2;
	
	strcpy( filename, "Screenshot " );
	time_t timer;
	char buffer[32];
	struct tm* tm_info;
	time(&timer);
	tm_info = localtime(&timer);
	strftime( buffer, 32, "%Y-%m-%d %H-%M-%S", tm_info );
	strcat( filename, buffer );
	strcat( filename, ".png" );
	
	temp = SDL_CreateRGBSurface(0,xres,yres,32,0,0,0,0);
	SDL_LockSurface(temp);
	glReadPixels(0,0,xres,yres,GL_BGRA,GL_UNSIGNED_BYTE,temp->pixels);
	SDL_UnlockSurface(temp);
	temp2 = flipSurface( temp, FLIP_VERTICAL );
	SDL_FreeSurface( temp );
	temp = SDL_CreateRGBSurface(0,xres,yres,24,0,0,0,0);
	SDL_FillRect(temp,NULL,0);
	SDL_BlitSurface(temp2,NULL,temp,NULL);
	SDL_FreeSurface( temp2 );
	SDL_SavePNG( temp, filename );
	SDL_FreeSurface( temp );
	if( !intro )
		messagePlayer(clientnum,"%s",filename);
	else
		printlog("%s",filename);
}