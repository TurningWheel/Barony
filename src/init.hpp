/*-------------------------------------------------------------------------------

	BARONY
	File: init.hpp
	Desc: prototypes for init.cpp, various setup/teardown functions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/
#pragma once

int initApp(char const * const title, int fullscreen);
int deinitApp();
bool initVideo();
bool changeVideoMode(int new_xres = 0, int new_yres = 0);
bool resizeWindow(int new_xres = 0, int new_yres = 0);
void generatePolyModels(int start, int end, bool forceCacheRebuild);
void generateVBOs(int start, int end);
void reloadModels(int start, int end);
void generateTileTextures();
void destroyTileTextures();
void bindTextureAtlas(int index);
bool mountBaseDataFolders();
bool remountBaseDataFolders();
