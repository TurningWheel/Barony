/*-------------------------------------------------------------------------------

	BARONY
	File: colors.hpp
	Desc: I can see the rainbow.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

/*
 * SDL_Color colors.
 */
static const SDL_Color sdlColorWhite = { 255, 255, 255, 255 };


/*
 * The following functions generate the corresponding color Uint32s from the given surface.
 */
inline Uint32 uint32ColorWhite(const SDL_Surface& surface)
{
	return SDL_MapRGB(surface.format, 255, 255, 255);
}

inline Uint32 uint32ColorGray(const SDL_Surface& surface)
{
	return SDL_MapRGB(surface.format, 127, 127, 127);
}
