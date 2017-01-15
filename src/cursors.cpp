/*-------------------------------------------------------------------------------

	BARONY
	File: cursors.cpp
	Desc: contains definitions and init code for various mouse cursors

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"

char* cursor_pencil[] = {
	// width height num_colors chars_per_pixel
	"    32    32        3            1",
	// colors
	"X c #000000",
	". c #ffffff",
	"  c None",
	// pixels
	"XXXXXX                          ",
	"X....XX                         ",
	"X...X..X                        ",
	"X..X....X                       ",
	"X.X......X                      ",
	"XX........X                     ",
	" X.........X                    ",
	"  X.........X                   ",
	"   X.........X                  ",
	"    X.........X                 ",
	"     X.......X.X                ",
	"      X.....X..XX               ",
	"       X...X..X..X              ",
	"        X.X..X...X              ",
	"         X..X....X              ",
	"          XX....X               ",
	"           X...X                ",
	"            XXX                 ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"0,0"
};

char* cursor_brush[] = {
	// width height num_colors chars_per_pixel
	"    32    32        3            1",
	// colors
	"X c #000000",
	". c #ffffff",
	"  c None",
	// pixels
	" XX                             ",
	" XX                             ",
	"X.X                             ",
	"X..X                            ",
	"X...XX                          ",
	"X.....XX                        ",
	"X......X                        ",
	" X....X.X                       ",
	" X...X..XX                      ",
	"  XXX..X..X                     ",
	"    XXX....X                    ",
	"     X......X                   ",
	"      X......X                  ",
	"       X......X                 ",
	"        X......X                ",
	"         X.....X                ",
	"          X.....X               ",
	"           XX....X              ",
	"             X....X             ",
	"              X....X            ",
	"               X...X            ",
	"                X..X            ",
	"                 XX             ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"1,0"
};

char* cursor_fill[] = {
	// width height num_colors chars_per_pixel
	"    32    32        3            1",
	// colors
	"X c #000000",
	". c #ffffff",
	"  c None",
	// pixels
	"          XXX                   ",
	"         XXXXXXXXXXX            ",
	"        XXXXXXXXXXXXXX          ",
	"       XXXX..XXX..XXXX          ",
	"      XXXX....XXXXXXX           ",
	"  XXXXXXXXXXXXXXXX              ",
	"XXXXXXXXXXXX....XXX             ",
	"XXXXXXX..........XXX            ",
	"XXXXXX...XXX......XXX           ",
	"XXXXXXX.XXXXX.....XXX           ",
	"XXXXXXXXXXXXXX....XX            ",
	"XXXXX XXXXXXX....XXX            ",
	" XXXX  XXXXX....XXX             ",
	"  XXX   XXX...XXXX              ",
	"  XXX    XXX.XXXX               ",
	"   XX     XXXXXX                ",
	"   XX      XXXX                 ",
	"   X        X                   ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"3,17"
};

SDL_Cursor* newCursor(char* image[]) {
	int i, row, col;
	Uint8 data[128];
	Uint8 mask[128];
	int hot_x, hot_y;

	i = -1;
	for ( row = 0; row < 32; ++row ) {
		for ( col = 0; col < 32; ++col ) {
			if ( col % 8 ) {
				data[i] <<= 1;
				mask[i] <<= 1;
			} else {
				++i;
				data[i] = mask[i] = 0;
			}
			switch (image[4 + row][col]) {
				case '.':
					data[i] |= 0x01;
					mask[i] |= 0x01;
					break;
				case 'X':
					mask[i] |= 0x01;
					break;
				case ' ':
					break;
			}
		}
	}
	sscanf(image[4 + row], "%d,%d", &hot_x, &hot_y);
	return SDL_CreateCursor(data, mask, 32, 32, hot_x, hot_y);
}