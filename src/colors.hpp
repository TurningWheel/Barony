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

constexpr Uint32 uint32ColorPlayer1 = makeColorRGB(255, 212, 64);
constexpr Uint32 uint32ColorPlayer2 = makeColorRGB(64, 255, 64);
constexpr Uint32 uint32ColorPlayer3 = makeColorRGB(255, 64, 64);
constexpr Uint32 uint32ColorPlayer4 = makeColorRGB(255, 160, 255);
constexpr Uint32 uint32ColorPlayerX = makeColorRGB(191, 191, 191);

constexpr Uint32 uint32ColorPlayer1_Ally = makeColorRGB(127, 106, 32);
constexpr Uint32 uint32ColorPlayer2_Ally = makeColorRGB(31, 127, 31);
constexpr Uint32 uint32ColorPlayer3_Ally = makeColorRGB(127, 31, 31);
constexpr Uint32 uint32ColorPlayer4_Ally = makeColorRGB(127, 80, 127);
constexpr Uint32 uint32ColorPlayerX_Ally = makeColorRGB(95, 95, 95);

constexpr Uint32 uint32ColorPlayer1_colorblind = makeColorRGB(255, 64, 64);
constexpr Uint32 uint32ColorPlayer2_colorblind = makeColorRGB(255, 160, 255);
constexpr Uint32 uint32ColorPlayer3_colorblind = makeColorRGB(64, 255, 64);
constexpr Uint32 uint32ColorPlayer4_colorblind = makeColorRGB(255, 255, 255);
constexpr Uint32 uint32ColorPlayerX_colorblind = makeColorRGB(191, 191, 191);

constexpr Uint32 uint32ColorPlayer1_Ally_colorblind = makeColorRGB(127, 31, 31);
constexpr Uint32 uint32ColorPlayer2_Ally_colorblind = makeColorRGB(127, 80, 127);
constexpr Uint32 uint32ColorPlayer3_Ally_colorblind = makeColorRGB(31, 127, 31);
constexpr Uint32 uint32ColorPlayer4_Ally_colorblind = makeColorRGB(128, 128, 128);
constexpr Uint32 uint32ColorPlayerX_Ally_colorblind = makeColorRGB(95, 95, 95);

constexpr Uint32 playerColor(int index, bool colorblind, bool ally) {
	if (ally) {
		if (colorblind) {
			switch (index) {
			default: return uint32ColorPlayerX_Ally_colorblind;
			case 0: return uint32ColorPlayer1_Ally_colorblind;
			case 1: return uint32ColorPlayer2_Ally_colorblind;
			case 2: return uint32ColorPlayer3_Ally_colorblind;
			case 3: return uint32ColorPlayer4_Ally_colorblind;
			}
		}
		else {
			switch (index) {
			default: return uint32ColorPlayerX_Ally;
			case 0: return uint32ColorPlayer1_Ally;
			case 1: return uint32ColorPlayer2_Ally;
			case 2: return uint32ColorPlayer3_Ally;
			case 3: return uint32ColorPlayer4_Ally;
			}
		}
	} else {
		if (colorblind) {
			switch (index) {
			default: return uint32ColorPlayerX_colorblind;
			case 0: return uint32ColorPlayer1_colorblind;
			case 1: return uint32ColorPlayer2_colorblind;
			case 2: return uint32ColorPlayer3_colorblind;
			case 3: return uint32ColorPlayer4_colorblind;
			}
		}
		else {
			switch (index) {
			default: return uint32ColorPlayerX;
			case 0: return uint32ColorPlayer1;
			case 1: return uint32ColorPlayer2;
			case 2: return uint32ColorPlayer3;
			case 3: return uint32ColorPlayer4;
			}
		}
	}
}