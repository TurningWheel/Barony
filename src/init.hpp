/*-------------------------------------------------------------------------------

	BARONY
	File: init.hpp
	Desc: prototypes for init.cpp, various setup/teardown functions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/
#pragma once

int initApp(const char* title, int fullscreen);
int deinitApp();
bool initVideo();
bool changeVideoMode();
void generatePolyModels(int start, int end);
void generateVBOs(int start, int end);
int loadLanguage(const char* lang);
int reloadLanguage();
void freeLanguages();
