/*-------------------------------------------------------------------------------

	BARONY
	File: draw.cpp
	Desc: contains various definitions for player code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "player.hpp"

Player **players = nullptr;

Entity *selectedEntity = nullptr;
int current_player = 0;
Sint32 mousex=0, mousey=0;
Sint32 omousex=0, omousey=0;
Sint32 mousexrel=0, mouseyrel=0;

Player::Player(int in_playernum, bool in_local_host)
{
	screen = nullptr;
	local_host = in_local_host;
	playernum = in_playernum;
	entity = nullptr;
}

Player::~Player()
{
	if (screen)
	{
		SDL_FreeSurface(screen);
	}

	if (entity)
	{
		delete entity;
	}
}