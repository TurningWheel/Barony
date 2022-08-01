/*-------------------------------------------------------------------------------

	BARONY
	File: colors.hpp
	Desc: I can see the rainbow.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include "draw.hpp"

/*
 * SDL_Color colors.
 */
constexpr SDL_Color sdlColorWhite = { 255, 255, 255, 255 };

/*
 * 32-bit color defines
 */
constexpr Uint32 uint32ColorBlack = makeColorRGB(0, 0, 0);
constexpr Uint32 uint32ColorWhite = makeColorRGB(255, 255, 255);
constexpr Uint32 uint32ColorGray = makeColorRGB(127, 127, 127);
constexpr Uint32 uint32ColorBlue = makeColor(0, 92, 255, 255);
constexpr Uint32 uint32ColorLightBlue = makeColor(0, 255, 255, 255);
constexpr Uint32 uint32ColorBaronyBlue = makeColor(0, 192, 255, 255); //Dodger Blue. Apparently.
constexpr Uint32 uint32ColorRed = makeColor(255, 0, 0, 255);
constexpr Uint32 uint32ColorGreen = makeColor(0, 255, 0, 255);
constexpr Uint32 uint32ColorOrange = makeColor(255, 128, 0, 255);
constexpr Uint32 uint32ColorYellow = makeColor(255, 255, 0, 255);

constexpr Uint32 uint32ColorPlayer1 = makeColorRGB(64, 255, 64);
constexpr Uint32 uint32ColorPlayer2 = makeColorRGB(64, 64, 255);
constexpr Uint32 uint32ColorPlayer3 = makeColorRGB(255, 64, 64);
constexpr Uint32 uint32ColorPlayer4 = makeColorRGB(255, 160, 255);
constexpr Uint32 uint32ColorPlayerX = makeColorRGB(191, 191, 191);

constexpr Uint32 uint32ColorPlayer1_Ally = makeColorRGB(31, 127, 31);
constexpr Uint32 uint32ColorPlayer2_Ally = makeColorRGB(31, 31, 127);
constexpr Uint32 uint32ColorPlayer3_Ally = makeColorRGB(127, 31, 31);
constexpr Uint32 uint32ColorPlayer4_Ally = makeColorRGB(127, 80, 127);
constexpr Uint32 uint32ColorPlayerX_Ally = makeColorRGB(95, 95, 95);