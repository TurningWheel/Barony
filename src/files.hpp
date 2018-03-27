/*-------------------------------------------------------------------------------

	BARONY
	File: files.hpp
	Desc: prototypes for file.cpp, all file access should be mediated
		  through this interface

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/
#pragma once

#include <list>
#include <string>
#include <vector>
#include <cstdio>
#include <dirent.h>

void glLoadTexture(SDL_Surface* image, int texnum);
SDL_Surface* loadImage(char* filename);
voxel_t* loadVoxel(char* filename2);
int loadMap(const char* filename, map_t* destmap, list_t* entlist, list_t* creatureList);
int loadConfig(char* filename);
int saveMap(char* filename);
char* readFile(char* filename);
std::list<std::string> directoryContents(const char* directory, bool subdirectoryOnly);
FILE *openDataFile(const char *const filename, const char * const mode);
DIR * openDataDir(const char *const);
bool dataPathExists(const char *const);
bool completePath(char *dest, const char * const path);
std::vector<std::string> getLinesFromDataFile(std::string filename);
extern char datadir[PATH_MAX]; //PATH_MAX as defined in main.hpp -- maybe define in Config.hpp?
int loadMainMenuMap(bool blessedAdditionMaps, bool forceVictoryMap);
int physfsLoadMapFile(int levelToLoad, Uint32 seed, bool useRandSeed);
std::vector<std::string> physfsGetFileNamesInDirectory(const char* dir);
std::string physfsFormatMapName(char* levelfilename);
