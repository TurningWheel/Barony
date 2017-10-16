/*-------------------------------------------------------------------------------

	BARONY
	File: files.hpp
	Desc: prototypes for file.cpp, all file access should be mediated
		  through this interface

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/
#pragma once
#include <string>
#include <vector>

void glLoadTexture(SDL_Surface* image, int texnum);
SDL_Surface* loadImage(const char* filename);
voxel_t* loadVoxel(char* filename2);
int loadMap(const char* filename, map_t* destmap, list_t* entlist);
int loadConfig(const char* filename);
int saveMap(const char* filename);
char* readFile(const char* filename);
std::list<std::string> directoryContents(const char* directory);
FILE *openDataFile(const char *const filename, const char * const mode);
FILE *openUserFile(const char *const filename, const char * const mode);
void setUserDir(const char * const dir);
void setDataDir(const char * const dir);
DIR * openDataDir(const char *const);
bool dataPathExists(const char *const);
SDL_RWops * openDataFileSDL(const char * filename, const char * mode);
std::vector<std::string> getLinesFromDataFile(std::string path);
