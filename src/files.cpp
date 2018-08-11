/*-------------------------------------------------------------------------------

	BARONY
	File: files.cpp
	Desc: contains code for file i/o

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <fstream>
#include <list>
#include <string>

#include "main.hpp"
#include "files.hpp"
#include "sound.hpp"
#include "entity.hpp"
#include "book.hpp"
#include "menu.hpp"
#include "items.hpp"
#include "interface/interface.hpp"

std::vector<int> gamemods_modelsListModifiedIndexes;
std::vector<std::pair<SDL_Surface**, std::string>> systemResourceImagesToReload;
std::unordered_map<std::string, int> mapHashes = 
{
	{ "start.lmp", 97295 },
	{ "mine.lmp", 85780 },
	{ "mine00.lmp", 12895 },
	{ "mine01.lmp", 11830 },
	{ "mine02.lmp", 6249 },
	{ "mine03.lmp", 6098 },
	{ "mine04.lmp", 7120 },
	{ "mine05.lmp", 8076 },
	{ "mine06.lmp", 18001 },
	{ "mine07.lmp", 19587 },
	{ "mine08.lmp", 5241 },
	{ "mine09.lmp", 18992 },
	{ "mine10.lmp", 24825 },
	{ "mine11.lmp", 13415 },
	{ "mine12.lmp", 12253 },
	{ "mine13.lmp", 10261 },
	{ "mine14.lmp", 15502 },
	{ "mine15.lmp", 9930 },
	{ "mine16.lmp", 13337 },
	{ "mine17.lmp", 11360 },
	{ "mine18.lmp", 10841 },
	{ "mine19.lmp", 4695 },
	{ "mine20.lmp", 5250 },
	{ "mine21.lmp", 6005 },
	{ "mine22.lmp", 5940 },
	{ "mine23.lmp", 6022 },
	{ "mine24.lmp", 8775 },
	{ "mine25.lmp", 2509 },
	{ "mine26.lmp", 1852 },
	{ "mine27.lmp", 1909 },
	{ "mine28.lmp", 1966 },
	{ "mine29.lmp", 2189 },
	{ "mine30.lmp", 36103 },
	{ "mine31.lmp", 20880 },
	{ "mine32.lmp", 10381 },
	{ "minetoswamp.lmp", 35867 },
	{ "minesecret.lmp", 10316 },
	{ "gnomishmines.lmp", 483571 },
	{ "minetown.lmp", 1401309 },
	{ "swamp.lmp", 13542615 },
	{ "swamp00.lmp", 14498 },
	{ "swamp01.lmp", 6780 },
	{ "swamp02.lmp", 27059 },
	{ "swamp03.lmp", 17148 },
	{ "swamp04.lmp", 26450 },
	{ "swamp05.lmp", 13262 },
	{ "swamp06.lmp", 16320 },
	{ "swamp07.lmp", 100108 },
	{ "swamp08.lmp", 8690 },
	{ "swamp09.lmp", 7540 },
	{ "swamp10.lmp", 19205 },
	{ "swamp11.lmp", 52707 },
	{ "swamp12.lmp", 88446 },
	{ "swamp13.lmp", 12936 },
	{ "swamp14.lmp", 14215 },
	{ "swamp15.lmp", 1092 },
	{ "swamp16.lmp", 3977 },
	{ "swamp17.lmp", 4683 },
	{ "swamp18.lmp", 1524 },
	{ "swamp19.lmp", 6366 },
	{ "swamp20.lmp", 8541 },
	{ "swamp21.lmp", 9036 },
	{ "swamp22.lmp", 15810 },
	{ "swamp23.lmp", 151549 },
	{ "swamp24.lmp", 16155 },
	{ "swamp25.lmp", 9710 },
	{ "swamp26.lmp", 41369 },
	{ "swamp27.lmp", 49462 },
	{ "swamp28.lmp", 1223 },
	{ "swamp29.lmp", 1136 },
	{ "swamp30.lmp", 43685 },
	{ "swamp31.lmp", 19612 },
	{ "swamp32.lmp", 13342 },
	{ "swamptolabyrinth.lmp", 63062 },
	{ "swampsecret.lmp", 11316 },
	{ "temple.lmp", 1756474 },
	{ "greatcastle.lmp", 551758 },
	{ "labyrinth.lmp", 219311 },
	{ "labyrinth00.lmp", 12194 },
	{ "labyrinth01.lmp", 4079 },
	{ "labyrinth02.lmp", 472 },
	{ "labyrinth03.lmp", 24615 },
	{ "labyrinth04.lmp", 8421 },
	{ "labyrinth05.lmp", 5520 },
	{ "labyrinth06.lmp", 4296 },
	{ "labyrinth07.lmp", 4297 },
	{ "labyrinth08.lmp", 4389 },
	{ "labyrinth09.lmp", 9494 },
	{ "labyrinth10.lmp", 10304 },
	{ "labyrinth11.lmp", 96656 },
	{ "labyrinth12.lmp", 39775 },
	{ "labyrinth13.lmp", 9162 },
	{ "labyrinth14.lmp", 40743 },
	{ "labyrinth15.lmp", 38253 },
	{ "labyrinth16.lmp", 13093 },
	{ "labyrinth17.lmp", 10923 },
	{ "labyrinth18.lmp", 4565 },
	{ "labyrinth19.lmp", 6714 },
	{ "labyrinth20.lmp", 10117 },
	{ "labyrinth21.lmp", 6830 },
	{ "labyrinth22.lmp", 5829 },
	{ "labyrinth23.lmp", 13042 },
	{ "labyrinth24.lmp", 14902 },
	{ "labyrinth25.lmp", 6399 },
	{ "labyrinth26.lmp", 1913 },
	{ "labyrinth27.lmp", 1916 },
	{ "labyrinth28.lmp", 1755 },
	{ "labyrinth29.lmp", 5155 },
	{ "labyrinth30.lmp", 163483 },
	{ "labyrinth31.lmp", 28195 },
	{ "labyrinth32.lmp", 23002 },
	{ "labyrinthtoruins.lmp", 36433 },
	{ "labyrinthsecret.lmp", 26508 },
	{ "sokoban.lmp", 137092 },
	{ "minotaur.lmp", 484073 },
	{ "ruins.lmp", 11472 },
	{ "ruins00.lmp", 6373 },
	{ "ruins01.lmp", 2301 },
	{ "ruins02.lmp", 2295 },
	{ "ruins03.lmp", 18465 },
	{ "ruins04.lmp", 11113 },
	{ "ruins05.lmp", 16194 },
	{ "ruins06.lmp", 1890 },
	{ "ruins07.lmp", 2486 },
	{ "ruins08.lmp", 4682 },
	{ "ruins09.lmp", 4704 },
	{ "ruins10.lmp", 10987 },
	{ "ruins11.lmp", 13490 },
	{ "ruins12.lmp", 6662 },
	{ "ruins13.lmp", 26335 },
	{ "ruins14.lmp", 766 },
	{ "ruins15.lmp", 772 },
	{ "ruins16.lmp", 4681 },
	{ "ruins17.lmp", 1848 },
	{ "ruins18.lmp", 9066 },
	{ "ruins19.lmp", 17940 },
	{ "ruins20.lmp", 40374 },
	{ "ruins21.lmp", 10664 },
	{ "ruins22.lmp", 11523 },
	{ "ruins23.lmp", 88 },
	{ "ruins24.lmp", 39421 },
	{ "ruins25.lmp", 290 },
	{ "ruins26.lmp", 72992 },
	{ "ruins27.lmp", 1430 },
	{ "ruins28.lmp", 244 },
	{ "ruins29.lmp", 13673 },
	{ "ruins30.lmp", 39050 },
	{ "ruins31.lmp", 46626 },
	{ "ruins32.lmp", 32243 },
	{ "ruinssecret.lmp", 66694 },
	{ "mysticlibrary.lmp", 55745 },
	{ "boss.lmp", 588024 },
	{ "underworld.lmp", 187740 },
	{ "underworld00.lmp", 20100 },
	{ "underworld01.lmp", 31871 },
	{ "underworld02.lmp", 9820 },
	{ "underworld03.lmp", 1687 },
	{ "underworld04.lmp", 5941 },
	{ "underworld05.lmp", 8743 },
	{ "underworld06.lmp", 11131 },
	{ "underworld07.lmp", 14844 },
	{ "underworld08.lmp", 11533 },
	{ "underworld09.lmp", 40231 },
	{ "underworld10.lmp", 218726 },
	{ "underworld11.lmp", 43090 },
	{ "underworld12.lmp", 13684 },
	{ "underworld13.lmp", 13664 },
	{ "underworld14.lmp", 43872 },
	{ "underworld15.lmp", 486983 },
	{ "underworld16.lmp", 2122 },
	{ "underworld17.lmp", 33907 },
	{ "underworld18.lmp", 48642 },
	{ "underworld19.lmp", 75679 },
	{ "underworld20.lmp", 89314 },
	{ "underworld21.lmp", 79585 },
	{ "underworld22.lmp", 76361 },
	{ "underworld23.lmp", 6463 },
	{ "underworld24.lmp", 9343 },
	{ "underworld25.lmp", 88248 },
	{ "underworld26.lmp", 3298 },
	{ "underworld27.lmp", 56635 },
	{ "underworld28.lmp", 12961 },
	{ "underworld29.lmp", 12473 },
	{ "hell.lmp", 145792 },
	{ "hell00.lmp", 43314 },
	{ "hell01.lmp", 15404 },
	{ "hell02.lmp", 17388 },
	{ "hell03.lmp", 23065 },
	{ "hell04.lmp", 2222 },
	{ "hell05.lmp", 3790 },
	{ "hell06.lmp", 2270 },
	{ "hell07.lmp", 3790 },
	{ "hell08.lmp", 10844 },
	{ "hell09.lmp", 11219 },
	{ "hell10.lmp", 19144 },
	{ "hell11.lmp", 4066 },
	{ "hell12.lmp", 20327 },
	{ "hell13.lmp", 3600 },
	{ "hell14.lmp", 28891 },
	{ "hell15.lmp", 99289 },
	{ "hell16.lmp", 357141 },
	{ "hell17.lmp", 14157 },
	{ "hell18.lmp", 14717 },
	{ "hell19.lmp", 5471 },
	{ "hell20.lmp", 15568 },
	{ "hell21.lmp", 8218 },
	{ "hell22.lmp", 14855 },
	{ "hell23.lmp", 8685 },
	{ "hell24.lmp", 61443 },
	{ "hell25.lmp", 53518 },
	{ "hell26.lmp", 4116 },
	{ "hell27.lmp", 2333 },
	{ "hell28.lmp", 1329 },
	{ "hell29.lmp", 36832 },
	{ "hellboss.lmp", 1115424 },
	{ "hamlet.lmp", 7100363 },
	{ "caves.lmp", 1065461 },
	{ "caves00.lmp", 70935 },
	{ "caves01.lmp", 13350 },
	{ "caves02.lmp", 6995 },
	{ "caves03.lmp", 11883 },
	{ "caves04.lmp", 15294 },
	{ "caves05.lmp", 10359 },
	{ "caves06.lmp", 8376 },
	{ "caves07.lmp", 9198 },
	{ "caves08.lmp", 6873 },
	{ "caves09.lmp", 102879 },
	{ "caves10.lmp", 28899 },
	{ "caves11.lmp", 35066 },
	{ "caves12.lmp", 39802 },
	{ "caves13.lmp", 45478 },
	{ "caves14.lmp", 37757 },
	{ "caves15.lmp", 26887 },
	{ "caves16.lmp", 233992 },
	{ "caves17.lmp", 31734 },
	{ "caves18.lmp", 36806 },
	{ "caves19.lmp", 25878 },
	{ "caves20.lmp", 117281 },
	{ "caves21.lmp", 16169 },
	{ "caves22.lmp", 26212 },
	{ "caves23.lmp", 35367 },
	{ "caves24.lmp", 80159 },
	{ "caves25.lmp", 19691 },
	{ "caves26.lmp", 41694 },
	{ "caves27.lmp", 10622 },
	{ "caves28.lmp", 8712 },
	{ "caves01a.lmp", 5 },
	{ "caves01b.lmp", 898 },
	{ "caves01c.lmp", 1238 },
	{ "caves01d.lmp", 709 },
	{ "caves01e.lmp", 5 },
	{ "caves01f.lmp", 955 },
	{ "caves09a.lmp", 2836 },
	{ "caves09b.lmp", 6462 },
	{ "caves09c.lmp", 8822 },
	{ "caves09d.lmp", 9722 },
	{ "caves09e.lmp", 6727 },
	{ "caves13a.lmp", 1855 },
	{ "caves13b.lmp", 1678 },
	{ "caves13c.lmp", 6637 },
	{ "caves13d.lmp", 3017 },
	{ "caves13e.lmp", 2892 },
	{ "caves24a.lmp", 159828 },
	{ "caves24b.lmp", 170175 },
	{ "caves24c.lmp", 169732 },
	{ "caves24d.lmp", 391561 },
	{ "cavestocitadel.lmp", 215724 },
	{ "caveslair.lmp", 4872675 },
	{ "citadel.lmp", 729069 },
	{ "citadel00.lmp", 23997 },
	{ "citadel01.lmp", 30094 },
	{ "citadel02.lmp", 20605 },
	{ "citadel03.lmp", 23292 },
	{ "citadel04.lmp", 23377 },
	{ "citadel05.lmp", 24208 },
	{ "citadel06.lmp", 22342 },
	{ "citadel07.lmp", 24158 },
	{ "citadel08.lmp", 23917 },
	{ "citadel09.lmp", 31320 },
	{ "citadel10.lmp", 31323 },
	{ "citadel11.lmp", 31330 },
	{ "citadel12.lmp", 31330 },
	{ "citadel13.lmp", 62357 },
	{ "citadel14.lmp", 121487 },
	{ "citadel15.lmp", 62368 },
	{ "citadel16.lmp", 55383 },
	{ "citadel17.lmp", 111086 },
	{ "citadel01a.lmp", 2025 },
	{ "citadel01b.lmp", 2224 },
	{ "citadel01c.lmp", 2025 },
	{ "citadel01d.lmp", 2377 },
	{ "citadel01e.lmp", 582 },
	{ "citadel01f.lmp", 204 },
	{ "citadel01g.lmp", 6 },
	{ "citadel02a.lmp", 4 },
	{ "citadel02b.lmp", 4 },
	{ "citadel02c.lmp", 4 },
	{ "citadel02d.lmp", 4 },
	{ "citadel02e.lmp", 4 },
	{ "citadel02f.lmp", 4 },
	{ "citadel02g.lmp", 4 },
	{ "citadel09a.lmp", 4930 },
	{ "citadel09b.lmp", 6181 },
	{ "citadel09c.lmp", 5270 },
	{ "citadel10a.lmp", 4930 },
	{ "citadel10b.lmp", 8900 },
	{ "citadel10c.lmp", 6072 },
	{ "citadel11a.lmp", 4930 },
	{ "citadel11b.lmp", 7722 },
	{ "citadel11c.lmp", 4860 },
	{ "citadel11d.lmp", 4179 },
	{ "citadel12a.lmp", 4930 },
	{ "citadel12b.lmp", 5874 },
	{ "citadel12c.lmp", 6072 },
	{ "citadel13a.lmp", 21969 },
	{ "citadel13b.lmp", 12526 },
	{ "citadel14a.lmp", 171089 },
	{ "citadel14b.lmp", 42457 },
	{ "citadel14c.lmp", 43306 },
	{ "citadel15a.lmp", 9038 },
	{ "citadel15b.lmp", 12010 },
	{ "citadel16a.lmp", 11376 },
	{ "citadel16b.lmp", 6314 },
	{ "citadel17a.lmp", 7279 },
	{ "citadel17b.lmp", 1729 },
	{ "citadel17c.lmp", 4197 },
	{ "citadel17d.lmp", 8339 },
	{ "bramscastle.lmp", 2995692 },
	{ "sanctum.lmp", 5630316 },
	{ "shop00.lmp", 7106 },
	{ "shop02.lmp", 11805 },
	{ "shop01.lmp", 11521 },
	{ "shop03.lmp", 11396 },
	{ "shopcitadel.lmp", 23033 },
	{  "warpzone.lmp", 3133088  }
};

/*-------------------------------------------------------------------------------

	glLoadTexture

	Binds the given image to an opengl texture name

-------------------------------------------------------------------------------*/

void glLoadTexture(SDL_Surface* image, int texnum)
{
	SDL_LockSurface(image);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texid[texnum]);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	//#ifdef APPLE
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image->w, image->h, 0, GL_BGRA_EXT, GL_UNSIGNED_INT_8_8_8_8_REV, image->pixels);
	//#else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->w, image->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels);
	//#endif
	SDL_UnlockSurface(image);
}


bool completePath(char *dest, const char * const filename, const char *base) {
	if (!(filename && filename[0])) {
		return false;
	}

	// Already absolute
	if (filename[0] == '/') {
		strncpy(dest, filename, PATH_MAX);
		return true;
	}

#ifdef WINDOWS
	// Already absolute (drive letter in path)
	if ( filename[1] == ':' ) {
		strncpy(dest, filename, PATH_MAX);
		return true;
	}
#endif

	snprintf(dest, PATH_MAX, "%s/%s", base, filename);
	return true;
}

FILE* openDataFile(const char * const filename, const char * const mode) {
	char path[PATH_MAX];
	completePath(path, filename);
	FILE * result = fopen(path, mode);
	if (!result) {
		printlog("Could not open '%s': %s", path, strerror(errno));
	}
	return result;
}

DIR* openDataDir(const char * const name) {
	char path[PATH_MAX];
	completePath(path, name);
	DIR * result = opendir(path);
	if (!result) {
		printlog("Could not open '%s': %s", path, strerror(errno));
	}
	return result;
}


bool dataPathExists(const char * const path) {
	char full_path[PATH_MAX];
	completePath(full_path, path);
	return access(full_path, F_OK) != -1;
}

void openLogFile() {
	char path[PATH_MAX];
	completePath(path, "log.txt", outputdir);

	logfile = freopen(path, "wb" /*or "wt"*/, stderr);
}


/*-------------------------------------------------------------------------------

	loadImage

	Loads the image specified in filename, binds it to an opengl texture name,
	and returns the image as an SDL_Surface

-------------------------------------------------------------------------------*/

SDL_Surface* loadImage(char* filename)
{
	char full_path[PATH_MAX];
	completePath(full_path, filename);
	SDL_Surface* originalSurface;

	if ( imgref >= MAXTEXTURES )
	{
		printlog("critical error! No more room in allsurfaces[], MAXTEXTURES reached.\n");
		printlog("aborting...\n");
		exit(1);
	}
	if ( (originalSurface = IMG_Load(full_path)) == NULL )
	{
		printlog("error: failed to load image '%s'\n", full_path);
		exit(1); // critical error
		return NULL;
	}

	// translate the original surface to an RGBA surface
	//int w = pow(2, ceil( log(std::max(originalSurface->w,originalSurface->h))/log(2) ) ); // round up to the nearest power of two
	SDL_Surface* newSurface = SDL_CreateRGBSurface(0, originalSurface->w, originalSurface->h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	SDL_BlitSurface(originalSurface, NULL, newSurface, NULL); // blit onto a purely RGBA Surface

	// load the new surface as a GL texture
	allsurfaces[imgref] = newSurface;
	allsurfaces[imgref]->refcount = imgref + 1;
	glLoadTexture(allsurfaces[imgref], imgref);

	// free the translated surface
	SDL_FreeSurface(originalSurface);

	imgref++;
	return allsurfaces[imgref - 1];
}

/*-------------------------------------------------------------------------------

	loadVoxel

	Loads a voxel model from the given filename

-------------------------------------------------------------------------------*/

voxel_t* loadVoxel(char* filename)
{
	//char filename2[1024];
	FILE* file;
	voxel_t* model;

	if (filename != NULL)
	{
		//bool has_ext = strstr(filename, ".vox") == NULL;
		//snprintf(filename2, 1024, "%s%s", filename, has_ext ? "" : ".vox");
		std::string filenamePath = PHYSFS_getRealDir(filename);
		filenamePath.append(PHYSFS_getDirSeparator()).append(filename);

		if ((file = openDataFile(filenamePath.c_str(), "rb")) == NULL)
		{
			return NULL;
		}
		model = (voxel_t*) malloc(sizeof(voxel_t));
		model->sizex = 0;
		fread(&model->sizex, sizeof(Sint32), 1, file);
		model->sizey = 0;
		fread(&model->sizey, sizeof(Sint32), 1, file);
		model->sizez = 0;
		fread(&model->sizez, sizeof(Sint32), 1, file);
		model->data = (Uint8*) malloc(sizeof(Uint8) * model->sizex * model->sizey * model->sizez);
		memset(model->data, 0, sizeof(Uint8)*model->sizex * model->sizey * model->sizez);
		fread(model->data, sizeof(Uint8), model->sizex * model->sizey * model->sizez, file);
		fread(&model->palette, sizeof(Uint8), 256 * 3, file);
		int c;
		for ( c = 0; c < 256; c++ )
		{
			model->palette[c][0] = model->palette[c][0] << 2;
			model->palette[c][1] = model->palette[c][1] << 2;
			model->palette[c][2] = model->palette[c][2] << 2;
		}
		fclose(file);

		return model;
	}
	else
	{
		return NULL;
	}
}

/*-------------------------------------------------------------------------------

	loadMap

	Loads a map from the given filename

-------------------------------------------------------------------------------*/

int loadMap(const char* filename2, map_t* destmap, list_t* entlist, list_t* creatureList, int *checkMapHash)
{
	FILE* fp;
	char valid_data[16];
	Uint32 numentities;
	Uint32 c;
	Sint32 x, y;
	Entity* entity;
	Sint32 sprite;
	Stat* myStats;
	Stat* dummyStats;
	sex_t s;
	int editorVersion = 0;
	char filename[1024];
	int mapHashData = 0;
	if ( checkMapHash )
	{
		*checkMapHash = 0;
	}

	char oldmapname[64];
	strcpy(oldmapname, map.name);

	printlog("LoadMap %s", filename2);

	if (! (filename2 && filename2[0]))
	{
		printlog("map filename empty or null");
		return -1;
	}

	if ( !PHYSFS_isInit() )
	{
		strcpy(filename, "maps/");
		strcat(filename, filename2);
	}
	else
	{
		strcpy(filename, filename2);
	}


	// add extension if missing
	if ( strstr(filename, ".lmp") == nullptr )
	{
		strcat(filename, ".lmp");
	}

	// load the file!
	if ((fp = openDataFile(filename, "rb")) == nullptr)
	{
		printlog("warning: failed to open file '%s' for map loading!\n", filename);
		if ( destmap == &map && game )
		{
			printlog("error: main map failed to load, aborting.\n");
			mainloop = 0;
		}
		return -1;
	}

	// read map version number
	fread(valid_data, sizeof(char), strlen("BARONY LMPV2.0"), fp);
	if ( strncmp(valid_data, "BARONY LMPV2.4", strlen("BARONY LMPV2.0")) == 0 )
	{
		// V2.4 version of editor - boulder trap properties
		editorVersion = 24;
	}
	else if ( strncmp(valid_data, "BARONY LMPV2.3", strlen("BARONY LMPV2.0")) == 0 )
	{
		// V2.3 version of editor - map flags
		editorVersion = 23;
	}
	else if ( strncmp(valid_data, "BARONY LMPV2.2", strlen("BARONY LMPV2.0")) == 0 )
	{
		// V2.2 version of editor - submaps
		editorVersion = 22;
	}
	else if ( strncmp(valid_data, "BARONY LMPV2.1", strlen("BARONY LMPV2.0")) == 0 )
	{
		// V2.1 version of editor - skybox
		editorVersion = 21;
	}
	else if ( strncmp(valid_data, "BARONY LMPV2.0", strlen("BARONY LMPV2.0")) == 0 )
	{
		// V2.0 version of editor
		editorVersion = 2;
	}
	else
	{
		rewind(fp);
		fread(valid_data, sizeof(char), strlen("BARONY"), fp);
		if ( strncmp(valid_data, "BARONY", strlen("BARONY")) == 0 )
		{
			// V1.0 version of editor
			editorVersion = 1;
		}
		else
		{
			printlog("warning: file '%s' is an invalid map file.\n", filename);
			fclose(fp);
			if ( destmap == &map && game )
			{
				printlog("error: main map failed to load, aborting.\n");
				mainloop = 0;
			}
			return -1;
		}
	}

	list_FreeAll(entlist);
	if ( destmap == &map )
	{
		// remove old lights
		list_FreeAll(&light_l);
	}
	if ( destmap->tiles != NULL )
	{
		free(destmap->tiles);
	}
	fread(destmap->name, sizeof(char), 32, fp); // map name
	fread(destmap->author, sizeof(char), 32, fp); // map author
	fread(&destmap->width, sizeof(Uint32), 1, fp); // map width
	fread(&destmap->height, sizeof(Uint32), 1, fp); // map height

	mapHashData += destmap->width + destmap->height;

	// map skybox
	if ( editorVersion == 1 || editorVersion == 2 )
	{
		if ( strncmp(destmap->name, "Hell", 4) == 0 )
		{
			destmap->skybox = 77;
		}
		else
		{
			destmap->skybox = 0;
		}
	}
	else
	{
		fread(&destmap->skybox, sizeof(Uint32), 1, fp); // map skybox
	}

	// misc map flags
	if ( editorVersion == 1 || editorVersion == 2 || editorVersion == 21 || editorVersion == 22 )
	{
		for ( c = 0; c < MAPFLAGS; c++ )
		{
			destmap->flags[c] = 0;
		}
	}
	else
	{
		fread(destmap->flags, sizeof(Sint32), MAPFLAGS, fp); // map flags
	}
	destmap->tiles = (Sint32*) malloc(sizeof(Sint32) * destmap->width * destmap->height * MAPLAYERS);
	fread(destmap->tiles, sizeof(Sint32), destmap->width * destmap->height * MAPLAYERS, fp);
	fread(&numentities, sizeof(Uint32), 1, fp); // number of entities on the map

	for ( c = 0; c < destmap->width * destmap->height * MAPLAYERS; ++c )
	{
		mapHashData += destmap->tiles[c];
	}

	for (c = 0; c < numentities; c++)
	{
		fread(&sprite, sizeof(Sint32), 1, fp);
		entity = newEntity(sprite, 0, entlist, nullptr); //TODO: Figure out when we need to assign an entity to the global monster list. And do it!
		switch( editorVersion )
		{	case 1:
				// V1.0 of editor version
			switch ( checkSpriteType(sprite) )
			{
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
					setSpriteAttributes(entity, nullptr, nullptr);
					break;
				default:
					break;
			}
				break;
			case 2:
			case 21:
			case 22:
			case 23:
			case 24:
				// V2.0+ of editor version
				switch ( checkSpriteType(sprite) )
				{
					case 1:
						if ( multiplayer != CLIENT )
						{
							// need to give the entity its list stuff.
							// create an empty first node for traversal purposes
							node_t* node2 = list_AddNodeFirst(&entity->children);
							node2->element = NULL;
							node2->deconstructor = &emptyDeconstructor;

							myStats = new Stat(entity->sprite);
							node2 = list_AddNodeLast(&entity->children);
							node2->element = myStats;
							//					node2->deconstructor = &myStats->~Stat;
							node2->size = sizeof(myStats);

							sex_t dummyVar = MALE; 
							// we don't actually embed the sex from the editor
							// advance the fp since we read in 0 always.
							// otherwise it would overwrite the value of a handplaced succubus or a certain icey lich.
							// certainly were a lot of male adventurers locked in cells...
							fread(&dummyVar, sizeof(sex_t), 1, fp);
							fread(&myStats->name, sizeof(char[128]), 1, fp);
							fread(&myStats->HP, sizeof(Sint32), 1, fp);
							fread(&myStats->MAXHP, sizeof(Sint32), 1, fp);
							fread(&myStats->OLDHP, sizeof(Sint32), 1, fp);
							fread(&myStats->MP, sizeof(Sint32), 1, fp);
							fread(&myStats->MAXMP, sizeof(Sint32), 1, fp);
							fread(&myStats->STR, sizeof(Sint32), 1, fp);
							fread(&myStats->DEX, sizeof(Sint32), 1, fp);
							fread(&myStats->CON, sizeof(Sint32), 1, fp);
							fread(&myStats->INT, sizeof(Sint32), 1, fp);
							fread(&myStats->PER, sizeof(Sint32), 1, fp);
							fread(&myStats->CHR, sizeof(Sint32), 1, fp);
							fread(&myStats->LVL, sizeof(Sint32), 1, fp);
							fread(&myStats->GOLD, sizeof(Sint32), 1, fp);

							fread(&myStats->RANDOM_MAXHP, sizeof(Sint32), 1, fp);
							fread(&myStats->RANDOM_HP, sizeof(Sint32), 1, fp);
							fread(&myStats->RANDOM_MAXMP, sizeof(Sint32), 1, fp);
							fread(&myStats->RANDOM_MP, sizeof(Sint32), 1, fp);
							fread(&myStats->RANDOM_STR, sizeof(Sint32), 1, fp);
							fread(&myStats->RANDOM_CON, sizeof(Sint32), 1, fp);
							fread(&myStats->RANDOM_DEX, sizeof(Sint32), 1, fp);
							fread(&myStats->RANDOM_INT, sizeof(Sint32), 1, fp);
							fread(&myStats->RANDOM_PER, sizeof(Sint32), 1, fp);
							fread(&myStats->RANDOM_CHR, sizeof(Sint32), 1, fp);
							fread(&myStats->RANDOM_LVL, sizeof(Sint32), 1, fp);
							fread(&myStats->RANDOM_GOLD, sizeof(Sint32), 1, fp);

							if ( editorVersion >= 22 )
							{
								fread(&myStats->EDITOR_ITEMS, sizeof(Sint32), ITEM_SLOT_NUM, fp);
							}
							else
							{
								// read old map formats
								fread(&myStats->EDITOR_ITEMS, sizeof(Sint32), 96, fp);
							}
							fread(&myStats->MISC_FLAGS, sizeof(Sint32), 32, fp);
						}
						//Read dummy values to move fp for the client
						else
						{
							dummyStats = new Stat(entity->sprite);
							fread(&dummyStats->sex, sizeof(sex_t), 1, fp);
							fread(&dummyStats->name, sizeof(char[128]), 1, fp);
							fread(&dummyStats->HP, sizeof(Sint32), 1, fp);
							fread(&dummyStats->MAXHP, sizeof(Sint32), 1, fp);
							fread(&dummyStats->OLDHP, sizeof(Sint32), 1, fp);
							fread(&dummyStats->MP, sizeof(Sint32), 1, fp);
							fread(&dummyStats->MAXMP, sizeof(Sint32), 1, fp);
							fread(&dummyStats->STR, sizeof(Sint32), 1, fp);
							fread(&dummyStats->DEX, sizeof(Sint32), 1, fp);
							fread(&dummyStats->CON, sizeof(Sint32), 1, fp);
							fread(&dummyStats->INT, sizeof(Sint32), 1, fp);
							fread(&dummyStats->PER, sizeof(Sint32), 1, fp);
							fread(&dummyStats->CHR, sizeof(Sint32), 1, fp);
							fread(&dummyStats->LVL, sizeof(Sint32), 1, fp);
							fread(&dummyStats->GOLD, sizeof(Sint32), 1, fp);

							fread(&dummyStats->RANDOM_MAXHP, sizeof(Sint32), 1, fp);
							fread(&dummyStats->RANDOM_HP, sizeof(Sint32), 1, fp);
							fread(&dummyStats->RANDOM_MAXMP, sizeof(Sint32), 1, fp);
							fread(&dummyStats->RANDOM_MP, sizeof(Sint32), 1, fp);
							fread(&dummyStats->RANDOM_STR, sizeof(Sint32), 1, fp);
							fread(&dummyStats->RANDOM_CON, sizeof(Sint32), 1, fp);
							fread(&dummyStats->RANDOM_DEX, sizeof(Sint32), 1, fp);
							fread(&dummyStats->RANDOM_INT, sizeof(Sint32), 1, fp);
							fread(&dummyStats->RANDOM_PER, sizeof(Sint32), 1, fp);
							fread(&dummyStats->RANDOM_CHR, sizeof(Sint32), 1, fp);
							fread(&dummyStats->RANDOM_LVL, sizeof(Sint32), 1, fp);
							fread(&dummyStats->RANDOM_GOLD, sizeof(Sint32), 1, fp);

							if ( editorVersion >= 22 )
							{
								fread(&dummyStats->EDITOR_ITEMS, sizeof(Sint32), ITEM_SLOT_NUM, fp);
							}
							else
							{
								fread(&dummyStats->EDITOR_ITEMS, sizeof(Sint32), 96, fp);
							}
							fread(&dummyStats->MISC_FLAGS, sizeof(Sint32), 32, fp);
						}
						break;
					case 2:
						fread(&entity->yaw, sizeof(real_t), 1, fp);
						fread(&entity->skill[9], sizeof(Sint32), 1, fp);
						fread(&entity->chestLocked, sizeof(Sint32), 1, fp);
						break;
					case 3:
						fread(&entity->skill[10], sizeof(Sint32), 1, fp);
						fread(&entity->skill[11], sizeof(Sint32), 1, fp);
						fread(&entity->skill[12], sizeof(Sint32), 1, fp);
						fread(&entity->skill[13], sizeof(Sint32), 1, fp);
						fread(&entity->skill[15], sizeof(Sint32), 1, fp);
						if ( editorVersion >= 22 )
						{
							fread(&entity->skill[16], sizeof(Sint32), 1, fp);
						}
						break;
					case 4:
						fread(&entity->skill[0], sizeof(Sint32), 1, fp);
						fread(&entity->skill[1], sizeof(Sint32), 1, fp);
						fread(&entity->skill[2], sizeof(Sint32), 1, fp);
						fread(&entity->skill[3], sizeof(Sint32), 1, fp);
						fread(&entity->skill[4], sizeof(Sint32), 1, fp);
						fread(&entity->skill[5], sizeof(Sint32), 1, fp);
						break;
					case 5:
						fread(&entity->yaw, sizeof(real_t), 1, fp);
						fread(&entity->crystalNumElectricityNodes, sizeof(Sint32), 1, fp);
						fread(&entity->crystalTurnReverse, sizeof(Sint32), 1, fp);
						fread(&entity->crystalSpellToActivate, sizeof(Sint32), 1, fp);
						break;
					case 6:
						fread(&entity->leverTimerTicks, sizeof(Sint32), 1, fp);
						break;
					case 7:
						if ( editorVersion >= 24 )
						{
							fread(&entity->boulderTrapRefireAmount, sizeof(Sint32), 1, fp);
							fread(&entity->boulderTrapRefireDelay, sizeof(Sint32), 1, fp);
							fread(&entity->boulderTrapPreDelay, sizeof(Sint32), 1, fp);
						}
						else
						{
							setSpriteAttributes(entity, nullptr, nullptr);
						}
						break;
					case 8:
						fread(&entity->pedestalOrbType, sizeof(Sint32), 1, fp);
						fread(&entity->pedestalHasOrb, sizeof(Sint32), 1, fp);
						fread(&entity->pedestalInvertedPower, sizeof(Sint32), 1, fp);
						fread(&entity->pedestalInGround, sizeof(Sint32), 1, fp);
						fread(&entity->pedestalLockOrb, sizeof(Sint32), 1, fp);
						break;
					case 9:
						fread(&entity->teleporterX, sizeof(Sint32), 1, fp);
						fread(&entity->teleporterY, sizeof(Sint32), 1, fp);
						fread(&entity->teleporterType, sizeof(Sint32), 1, fp);
						break;
					case 10:
						fread(&entity->ceilingTileModel, sizeof(Sint32), 1, fp);
						break;
					case 11:
						fread(&entity->spellTrapType, sizeof(Sint32), 1, fp);
						fread(&entity->spellTrapRefire, sizeof(Sint32), 1, fp);
						fread(&entity->spellTrapLatchPower, sizeof(Sint32), 1, fp);
						fread(&entity->spellTrapFloorTile, sizeof(Sint32), 1, fp);
						fread(&entity->spellTrapRefireRate, sizeof(Sint32), 1, fp);
						break;
					case 12:
						fread(&entity->furnitureDir, sizeof(Sint32), 1, fp);
						break;
					case 13:
						fread(&entity->floorDecorationModel, sizeof(Sint32), 1, fp);
						fread(&entity->floorDecorationRotation, sizeof(Sint32), 1, fp);
						fread(&entity->floorDecorationHeightOffset, sizeof(Sint32), 1, fp);
						break;
					case 14:
						fread(&entity->soundSourceToPlay, sizeof(Sint32), 1, fp);
						fread(&entity->soundSourceVolume, sizeof(Sint32), 1, fp);
						fread(&entity->soundSourceLatchOn, sizeof(Sint32), 1, fp);
						fread(&entity->soundSourceDelay, sizeof(Sint32), 1, fp);
						fread(&entity->soundSourceOrigin, sizeof(Sint32), 1, fp);
						break;
					case 15:
						fread(&entity->lightSourceAlwaysOn, sizeof(Sint32), 1, fp);
						fread(&entity->lightSourceBrightness, sizeof(Sint32), 1, fp);
						fread(&entity->lightSourceInvertPower, sizeof(Sint32), 1, fp);
						fread(&entity->lightSourceLatchOn, sizeof(Sint32), 1, fp);
						fread(&entity->lightSourceRadius, sizeof(Sint32), 1, fp);
						fread(&entity->lightSourceFlicker, sizeof(Sint32), 1, fp);
						fread(&entity->lightSourceDelay, sizeof(Sint32), 1, fp);
						break;
					case 16:
					{
						fread(&entity->textSourceColorRGB, sizeof(Sint32), 1, fp);
						fread(&entity->textSourceVariables4W, sizeof(Sint32), 1, fp);
						fread(&entity->textSourceDelay, sizeof(Sint32), 1, fp);
						fread(&entity->textSource3, sizeof(Sint32), 1, fp);
						for ( int i = 4; i < 60; ++i )
						{
							fread(&entity->skill[i], sizeof(Sint32), 1, fp);
						}
						break;
					}
					case 17:
						fread(&entity->signalInputDirection, sizeof(Sint32), 1, fp);
						fread(&entity->signalActivateDelay, sizeof(Sint32), 1, fp);
						fread(&entity->signalTimerInterval, sizeof(Sint32), 1, fp);
						fread(&entity->signalTimerRepeatCount, sizeof(Sint32), 1, fp);
						fread(&entity->signalTimerLatchInput, sizeof(Sint32), 1, fp);
						break;
					default:
						break;
				}
				break;
			default:
				break;
		}
		if ( entity->behavior == actMonster || entity->behavior == actPlayer )
		{
			entity->addToCreatureList(creatureList);
		}

		fread(&x, sizeof(Sint32), 1, fp);
		fread(&y, sizeof(Sint32), 1, fp);
		entity->x = x;
		entity->y = y;
		mapHashData += (sprite * c);
	}

	fclose(fp);

	if ( destmap == &map )
	{
		nummonsters = 0;
		minotaurlevel = 0;

#if defined (USE_FMOD) || defined(USE_OPENAL)
		if ( strcmp(oldmapname, map.name) )
		{
			levelmusicplaying = false;
		}
#endif

		// create new lightmap
		if ( lightmap != NULL )
		{
			free(lightmap);
		}

		lightmap = (int*) malloc(sizeof(Sint32) * destmap->width * destmap->height);
		if ( strncmp(map.name, "Hell", 4) )
		{
			for (c = 0; c < destmap->width * destmap->height; c++ )
			{
				lightmap[c] = 0;
			}
		}
		else
		{
			for (c = 0; c < destmap->width * destmap->height; c++ )
			{
				lightmap[c] = 32;
			}
		}


		// create a new vismap
		if (vismap != NULL)
		{
			free(vismap);
		}
		vismap = (bool*) calloc(destmap->width * destmap->height, sizeof(bool));

		// reset minimap
		for ( x = 0; x < 64; x++ )
		{
			for ( y = 0; y < 64; y++ )
			{
				minimap[y][x] = 0;
			}
		}

		// reset camera
		if ( game )
		{
			camera.x = -32;
			camera.y = -32;
			camera.z = 0;
			camera.ang = 3 * PI / 2;
			camera.vang = 0;
		}
		else
		{
			camera.x = 2;
			camera.y = 2;
			camera.z = 0;
			camera.ang = 0;
			camera.vang = 0;
		}

		// shoparea
		if ( shoparea )
		{
			free(shoparea);
		}
		shoparea = (bool*) malloc(sizeof(bool) * destmap->width * destmap->height);
		for ( x = 0; x < destmap->width; x++ )
			for ( y = 0; y < destmap->height; y++ )
			{
				shoparea[y + x * destmap->height] = false;
			}
	}

	for ( c = 0; c < 512; c++ )
	{
		keystatus[c] = 0;
	}


	if ( checkMapHash != nullptr )
	{
		std::string mapShortName = filename2;
		size_t found = mapShortName.rfind("/");
		if ( found != std::string::npos )
		{
			mapShortName = mapShortName.substr(found + 1);
			std::unordered_map<std::string, int>::iterator it;
			it = mapHashes.find(mapShortName);
			if ( it != mapHashes.end() && (*it).second == mapHashData )
			{
				//printlog("MAP HASH SUCCESS");
				*checkMapHash = 1;
			}
			else
			{
				printlog("Notice: Unable to verify map %s hash %d.", filename2, mapHashData);
				*checkMapHash = 0;
			}
		}
		else
		{
			size_t found2 = mapShortName.rfind("\\");
			if ( found2 != std::string::npos )
			{
				mapShortName = mapShortName.substr(found2 + 1);
				std::unordered_map<std::string, int>::iterator it;
				it = mapHashes.find(mapShortName);
				if ( it != mapHashes.end() && (*it).second == mapHashData )
				{
					//printlog("MAP HASH SUCCESS");
					*checkMapHash = 1;
				}
				else
				{
					printlog("Notice: Unable to verify map %s hash %d.", filename2, mapHashData);
					*checkMapHash = 0;
				}
			}
		}
	}

	return numentities;
}

/*-------------------------------------------------------------------------------

	saveMap

	Saves a map to the given filename

-------------------------------------------------------------------------------*/

int saveMap(const char* filename2)
{
	FILE* fp;
	Uint32 numentities = 0;
	node_t* node;
	Entity* entity;
	char filename[256];
	Sint32 x, y;
	Stat* myStats;

	if ( filename2 != NULL && strcmp(filename2, "") )
	{
		if ( !PHYSFS_isInit() )
		{
			strcpy(filename, "maps/");
			strcat(filename, filename2);
		}
		else
		{
			strcpy(filename, filename2);
		}

		if ( strstr(filename, ".lmp") == NULL )
		{
			strcat(filename, ".lmp");
		}

		if ((fp = openDataFile(filename, "wb")) == NULL)
		{
			printlog("warning: failed to open file '%s' for map saving!\n", filename);
			return 1;
		}

		fwrite("BARONY LMPV2.4", sizeof(char), strlen("BARONY LMPV2.0"), fp); // magic code
		fwrite(map.name, sizeof(char), 32, fp); // map filename
		fwrite(map.author, sizeof(char), 32, fp); // map author
		fwrite(&map.width, sizeof(Uint32), 1, fp); // map width
		fwrite(&map.height, sizeof(Uint32), 1, fp); // map height
		fwrite(&map.skybox, sizeof(Uint32), 1, fp); // map skybox
		fwrite(map.flags, sizeof(Sint32), MAPFLAGS, fp); // map flags
		fwrite(map.tiles, sizeof(Sint32), map.width * map.height * MAPLAYERS, fp);
		for (node = map.entities->first; node != nullptr; node = node->next)
		{
			++numentities;
		}
		fwrite(&numentities, sizeof(Uint32), 1, fp); // number of entities on the map
		for (node = map.entities->first; node != nullptr; node = node->next)
		{
			entity = (Entity*) node->element;
			fwrite(&entity->sprite, sizeof(Sint32), 1, fp);

			switch ( checkSpriteType(entity->sprite) )
			{
				case 1:
					// monsters
					myStats = entity->getStats();
					fwrite(&myStats->sex, sizeof(sex_t), 1, fp);
					fwrite(&myStats->name, sizeof(char[128]), 1, fp);
					fwrite(&myStats->HP, sizeof(Sint32), 1, fp);
					fwrite(&myStats->MAXHP, sizeof(Sint32), 1, fp);
					fwrite(&myStats->OLDHP, sizeof(Sint32), 1, fp);
					fwrite(&myStats->MP, sizeof(Sint32), 1, fp);
					fwrite(&myStats->MAXMP, sizeof(Sint32), 1, fp);
					fwrite(&myStats->STR, sizeof(Sint32), 1, fp);
					fwrite(&myStats->DEX, sizeof(Sint32), 1, fp);
					fwrite(&myStats->CON, sizeof(Sint32), 1, fp);
					fwrite(&myStats->INT, sizeof(Sint32), 1, fp);
					fwrite(&myStats->PER, sizeof(Sint32), 1, fp);
					fwrite(&myStats->CHR, sizeof(Sint32), 1, fp);
					fwrite(&myStats->LVL, sizeof(Sint32), 1, fp);
					fwrite(&myStats->GOLD, sizeof(Sint32), 1, fp);

					fwrite(&myStats->RANDOM_MAXHP, sizeof(Sint32), 1, fp);
					fwrite(&myStats->RANDOM_HP, sizeof(Sint32), 1, fp);
					fwrite(&myStats->RANDOM_MAXMP, sizeof(Sint32), 1, fp);
					fwrite(&myStats->RANDOM_MP, sizeof(Sint32), 1, fp);
					fwrite(&myStats->RANDOM_STR, sizeof(Sint32), 1, fp);
					fwrite(&myStats->RANDOM_CON, sizeof(Sint32), 1, fp);
					fwrite(&myStats->RANDOM_DEX, sizeof(Sint32), 1, fp);
					fwrite(&myStats->RANDOM_INT, sizeof(Sint32), 1, fp);
					fwrite(&myStats->RANDOM_PER, sizeof(Sint32), 1, fp);
					fwrite(&myStats->RANDOM_CHR, sizeof(Sint32), 1, fp);
					fwrite(&myStats->RANDOM_LVL, sizeof(Sint32), 1, fp);
					fwrite(&myStats->RANDOM_GOLD, sizeof(Sint32), 1, fp);

					fwrite(&myStats->EDITOR_ITEMS, sizeof(Sint32), ITEM_SLOT_NUM, fp);
					fwrite(&myStats->MISC_FLAGS, sizeof(Sint32), 32, fp);
					break;
				case 2:
					// chests
					fwrite(&entity->yaw, sizeof(real_t), 1, fp);
					fwrite(&entity->skill[9], sizeof(Sint32), 1, fp);
					fwrite(&entity->chestLocked, sizeof(Sint32), 1, fp);
					break;
				case 3:
					// items
					fwrite(&entity->skill[10], sizeof(Sint32), 1, fp);
					fwrite(&entity->skill[11], sizeof(Sint32), 1, fp);
					fwrite(&entity->skill[12], sizeof(Sint32), 1, fp);
					fwrite(&entity->skill[13], sizeof(Sint32), 1, fp);
					fwrite(&entity->skill[15], sizeof(Sint32), 1, fp);
					fwrite(&entity->skill[16], sizeof(Sint32), 1, fp);
					break;
				case 4:
					fwrite(&entity->skill[0], sizeof(Sint32), 1, fp);
					fwrite(&entity->skill[1], sizeof(Sint32), 1, fp);
					fwrite(&entity->skill[2], sizeof(Sint32), 1, fp);
					fwrite(&entity->skill[3], sizeof(Sint32), 1, fp);
					fwrite(&entity->skill[4], sizeof(Sint32), 1, fp);
					fwrite(&entity->skill[5], sizeof(Sint32), 1, fp);
					break;
				case 5:
					fwrite(&entity->yaw, sizeof(real_t), 1, fp);
					fwrite(&entity->crystalNumElectricityNodes, sizeof(Sint32), 1, fp);
					fwrite(&entity->crystalTurnReverse, sizeof(Sint32), 1, fp);
					fwrite(&entity->crystalSpellToActivate, sizeof(Sint32), 1, fp);
					break;
				case 6:
					fwrite(&entity->leverTimerTicks, sizeof(Sint32), 1, fp);
					break;
				case 7:
					fwrite(&entity->boulderTrapRefireAmount, sizeof(Sint32), 1, fp);
					fwrite(&entity->boulderTrapRefireDelay, sizeof(Sint32), 1, fp);
					fwrite(&entity->boulderTrapPreDelay, sizeof(Sint32), 1, fp);
					break;
				case 8:
					fwrite(&entity->pedestalOrbType, sizeof(Sint32), 1, fp);
					fwrite(&entity->pedestalHasOrb, sizeof(Sint32), 1, fp);
					fwrite(&entity->pedestalInvertedPower, sizeof(Sint32), 1, fp);
					fwrite(&entity->pedestalInGround, sizeof(Sint32), 1, fp);
					fwrite(&entity->pedestalLockOrb, sizeof(Sint32), 1, fp);
					break;
				case 9:
					fwrite(&entity->teleporterX, sizeof(Sint32), 1, fp);
					fwrite(&entity->teleporterY, sizeof(Sint32), 1, fp);
					fwrite(&entity->teleporterType, sizeof(Sint32), 1, fp);
					break;
				case 10:
					fwrite(&entity->ceilingTileModel, sizeof(Sint32), 1, fp);
					break;
				case 11:
					fwrite(&entity->spellTrapType, sizeof(Sint32), 1, fp);
					fwrite(&entity->spellTrapRefire, sizeof(Sint32), 1, fp);
					fwrite(&entity->spellTrapLatchPower, sizeof(Sint32), 1, fp);
					fwrite(&entity->spellTrapFloorTile, sizeof(Sint32), 1, fp);
					fwrite(&entity->spellTrapRefireRate, sizeof(Sint32), 1, fp);
					break;
				case 12:
					fwrite(&entity->furnitureDir, sizeof(Sint32), 1, fp);
					break;
				case 13:
					fwrite(&entity->floorDecorationModel, sizeof(Sint32), 1, fp);
					fwrite(&entity->floorDecorationRotation, sizeof(Sint32), 1, fp);
					fwrite(&entity->floorDecorationHeightOffset, sizeof(Sint32), 1, fp);
					break;
				case 14:
					fwrite(&entity->soundSourceToPlay, sizeof(Sint32), 1, fp);
					fwrite(&entity->soundSourceVolume, sizeof(Sint32), 1, fp);
					fwrite(&entity->soundSourceLatchOn, sizeof(Sint32), 1, fp);
					fwrite(&entity->soundSourceDelay, sizeof(Sint32), 1, fp);
					fwrite(&entity->soundSourceOrigin, sizeof(Sint32), 1, fp);
					break;
				case 15:
					fwrite(&entity->lightSourceAlwaysOn, sizeof(Sint32), 1, fp);
					fwrite(&entity->lightSourceBrightness, sizeof(Sint32), 1, fp);
					fwrite(&entity->lightSourceInvertPower, sizeof(Sint32), 1, fp);
					fwrite(&entity->lightSourceLatchOn, sizeof(Sint32), 1, fp);
					fwrite(&entity->lightSourceRadius, sizeof(Sint32), 1, fp);
					fwrite(&entity->lightSourceFlicker, sizeof(Sint32), 1, fp);
					fwrite(&entity->lightSourceDelay, sizeof(Sint32), 1, fp);
					break;
				case 16:
				{
					fwrite(&entity->textSourceColorRGB, sizeof(Sint32), 1, fp);
					fwrite(&entity->textSourceVariables4W, sizeof(Sint32), 1, fp);
					fwrite(&entity->textSourceDelay, sizeof(Sint32), 1, fp);
					fwrite(&entity->textSource3, sizeof(Sint32), 1, fp);
					for ( int i = 4; i < 60; ++i )
					{
						fwrite(&entity->skill[i], sizeof(Sint32), 1, fp);
					}
					break;
				}
				case 17:
					fwrite(&entity->signalInputDirection, sizeof(Sint32), 1, fp);
					fwrite(&entity->signalActivateDelay, sizeof(Sint32), 1, fp);
					fwrite(&entity->signalTimerInterval, sizeof(Sint32), 1, fp);
					fwrite(&entity->signalTimerRepeatCount, sizeof(Sint32), 1, fp);
					fwrite(&entity->signalTimerLatchInput, sizeof(Sint32), 1, fp);
					break;
				default:
					break;
			}

			x = entity->x;
			y = entity->y;
			fwrite(&x, sizeof(Sint32), 1, fp);
			fwrite(&y, sizeof(Sint32), 1, fp);
		}
		fclose(fp);
		return 0;
	}
	else
	{
		return 1;
	}
}

/*-------------------------------------------------------------------------------

	readFile

	Simply reads the contents of an entire file into a char array

-------------------------------------------------------------------------------*/

char* readFile(char* filename)
{
	char* file_contents = NULL;
	long input_file_size;
	FILE* input_file = openDataFile(filename, "rb");
	if (!input_file) {
		printlog("Open failed: %s", strerror(errno));
		goto out_input_file;
	}

	if (fseek(input_file, 0, SEEK_END) != 0) {
		printlog("Seek failed");
		goto out_input_file;
	}

	if ((input_file_size = ftell(input_file)) == -1) {
		printlog("ftell failed");
		goto out_input_file;
	}

	if (input_file_size > (1<<30)) {
		printlog("Unreasonable size: %ld", input_file_size);
		goto out_input_file;
	}
	
	rewind(input_file);
	file_contents = static_cast<char*>(malloc((input_file_size + 1) * sizeof(char)));
	fread(file_contents, sizeof(char), input_file_size, input_file);
	file_contents[input_file_size] = 0;

out_input_file:
	fclose(input_file);
	return file_contents;
}

/*-------------------------------------------------------------------------------

	directoryContents

	Stores the contents of a directory in a list

-------------------------------------------------------------------------------*/

std::list<std::string> directoryContents(const char* directory, bool includeSubdirectory, bool includeFiles)
{
	std::list<std::string> list;
	char fullPath[1024];
	completePath(fullPath, directory);
	DIR* dir = opendir(fullPath);
	struct dirent* entry = NULL;

	if ( !dir )
	{
		printlog( "[directoryContents()] Failed to open directory \"%s\".\n", directory);
		return list;
	}

	struct stat cur;
	char curPath[1024];
	while ((entry = readdir(dir)) != NULL)
	{
		strcpy(curPath, fullPath);
		strcat(curPath, entry->d_name);

		if (stat(curPath, &cur) != 0)
		{
			continue;
		}
		if ( includeFiles )
		{
			if ((cur.st_mode & S_IFMT) == S_IFREG)
			{
				list.push_back(entry->d_name);
			}
		}
		if ( includeSubdirectory )
		{
			if ((cur.st_mode & S_IFMT) == S_IFDIR)
			{
				list.push_back(entry->d_name);
			}
		}
	}

	closedir(dir);

	return list;
}

std::vector<std::string> getLinesFromDataFile(std::string filename)
{
	std::vector<std::string> lines;
	std::string filepath(datadir);
	filepath += "/";
	filepath += filename;
	std::ifstream file(filepath);
	if ( !file )
	{
		std::ifstream file(filename); // check absolute path.
		if ( !file)
		{
			printlog("Error: Failed to open file \"%s\"", filename.c_str());
			return lines;
		}
		else
		{
			std::string line;
			while ( std::getline(file, line) )
			{
				if ( !line.empty() )
				{
					lines.push_back(line);
				}
			}
			file.close();
		}
	}
	else
	{
		std::string line;
		while ( std::getline(file, line) )
		{
			if ( !line.empty() )
			{
				lines.push_back(line);
			}
		}
		file.close();
	}

	return lines;
}

int physfsLoadMapFile(int levelToLoad, Uint32 seed, bool useRandSeed, int* checkMapHash)
{
	std::string mapsDirectory; // store the full file path here.
	if ( !secretlevel )
	{
		mapsDirectory = PHYSFS_getRealDir(LEVELSFILE);
		mapsDirectory.append(PHYSFS_getDirSeparator()).append(LEVELSFILE);
	}
	else
	{
		mapsDirectory = PHYSFS_getRealDir(SECRETLEVELSFILE);
		mapsDirectory.append(PHYSFS_getDirSeparator()).append(SECRETLEVELSFILE);
	}
	printlog("Maps directory: %s", mapsDirectory.c_str());
	std::vector<std::string> levelsList = getLinesFromDataFile(mapsDirectory);
	std::string line = levelsList.front();
	int levelsCounted = 0;
	if ( levelToLoad > 0 ) // if level == 0, then load up the first map.
	{
		for ( std::vector<std::string>::const_iterator i = levelsList.begin(); i != levelsList.end() && levelsCounted <= levelToLoad; ++i )
		{
			// process i, iterate through all the map levels until currentlevel.
			line = *i;
			if ( line[0] == '\n' )
			{
				continue;
			}
			++levelsCounted;
		}
	}
	std::size_t found = line.find(' ');
	char tempstr[1024];
	if ( found != std::string::npos )
	{
		std::string mapType = line.substr(0, found);
		std::string mapName;
		mapName = line.substr(found + 1, line.find('\n'));
		std::size_t carriageReturn = mapName.find('\r');
		if ( carriageReturn != std::string::npos )
		{
			mapName.erase(carriageReturn);
			printlog("%s", mapName.c_str());
		}
		if ( mapType.compare("map:") == 0 )
		{
			strncpy(tempstr, mapName.c_str(), mapName.length());
			tempstr[mapName.length()] = '\0';
			mapName = physfsFormatMapName(tempstr);
			if ( checkMapHash )
			{
				return loadMap(mapName.c_str(), &map, map.entities, map.creatures, checkMapHash);
			}
			else
			{
				return loadMap(mapName.c_str(), &map, map.entities, map.creatures);
			}
		}
		else if ( mapType.compare("gen:") == 0 )
		{
			std::size_t secretChanceFound = mapName.find(" secret%: ");
			std::size_t darkmapChanceFound = mapName.find(" darkmap%: ");
			std::size_t minotaurChanceFound = mapName.find(" minotaur%: ");
			std::string parameterStr = "";
			std::tuple<int, int, int> mapParameters = std::make_tuple(-1, -1, -1);
			if ( secretChanceFound != std::string::npos )
			{
				// found a percentage for secret levels to spawn.
				parameterStr = mapName.substr(secretChanceFound + strlen(" secret%: "));
				parameterStr = parameterStr.substr(0, parameterStr.find_first_of(" \0"));
				std::get<LEVELPARAM_CHANCE_SECRET>(mapParameters) = std::stoi(parameterStr);
				if ( std::get<LEVELPARAM_CHANCE_SECRET>(mapParameters) < 0 || std::get<LEVELPARAM_CHANCE_SECRET>(mapParameters) > 100 )
				{
					std::get<LEVELPARAM_CHANCE_SECRET>(mapParameters) = -1;
				}
			}
			if ( darkmapChanceFound != std::string::npos )
			{
				// found a percentage for secret levels to spawn.
				parameterStr = mapName.substr(darkmapChanceFound + strlen(" darkmap%: "));
				parameterStr = parameterStr.substr(0, parameterStr.find_first_of(" \0"));
				std::get<LEVELPARAM_CHANCE_DARKNESS>(mapParameters) = std::stoi(parameterStr);
				if ( std::get<LEVELPARAM_CHANCE_DARKNESS>(mapParameters) < 0 || std::get<LEVELPARAM_CHANCE_DARKNESS>(mapParameters) > 100 )
				{
					std::get<LEVELPARAM_CHANCE_DARKNESS>(mapParameters) = -1;
				}
			}
			if ( minotaurChanceFound != std::string::npos )
			{
				// found a percentage for secret levels to spawn.
				parameterStr = mapName.substr(minotaurChanceFound + strlen(" minotaur%: "));
				parameterStr = parameterStr.substr(0, parameterStr.find_first_of(" \0"));
				std::get<LEVELPARAM_CHANCE_MINOTAUR>(mapParameters) = std::stoi(parameterStr);
				if ( std::get<LEVELPARAM_CHANCE_MINOTAUR>(mapParameters) < 0 || std::get<LEVELPARAM_CHANCE_MINOTAUR>(mapParameters) > 100 )
				{
					std::get<LEVELPARAM_CHANCE_MINOTAUR>(mapParameters) = -1;
				}
			}
			mapName = mapName.substr(0, mapName.find_first_of(" \0"));

			strncpy(tempstr, mapName.c_str(), mapName.length());
			tempstr[mapName.length()] = '\0';
			if ( useRandSeed )
			{
				return generateDungeon(tempstr, rand(), mapParameters);
			}
			else
			{
				return generateDungeon(tempstr, seed, mapParameters);
			}
		}
		//printlog("%s", mapName.c_str());
	}
	return 0;
}

std::list<std::string> physfsGetFileNamesInDirectory(const char* dir)
{
	std::list<std::string> filenames;
	char **rc = PHYSFS_enumerateFiles(dir);
	if ( *rc == NULL )
	{
		printlog("[PhysFS]: Error: Failed to enumerate filenames in directory '%s'", dir);
		return filenames;
	}
	char **i;
	char buf[1024];
	std::string file;
	for ( i = rc; *i != NULL; i++ )
	{
		file = *i;
		//printlog(" * We've got [%s].\n", file.c_str());
		filenames.push_back(file);
	}
	PHYSFS_freeList(rc);
	return filenames;
}

std::string physfsFormatMapName(char* levelfilename)
{
	std::string fullMapPath;
	std::string mapFileName = "maps/";
	mapFileName.append(levelfilename);
	if ( mapFileName.find(".lmp") == std::string::npos )
	{
		mapFileName.append(".lmp");
	}
	//printlog("format map name: %s", mapFileName.c_str());
	if ( PHYSFS_getRealDir(mapFileName.c_str()) != NULL )
	{
		//printlog("format map name: %s", mapFileName.c_str());
		fullMapPath = PHYSFS_getRealDir(mapFileName.c_str());
		fullMapPath.append(PHYSFS_getDirSeparator()).append(mapFileName);
	}
	return fullMapPath;
}

bool physfsSearchModelsToUpdate()
{
	std::string modelsDirectory = PHYSFS_getRealDir("models/models.txt");
	modelsDirectory.append(PHYSFS_getDirSeparator()).append("models/models.txt");
	FILE* fp = openDataFile(modelsDirectory.c_str(), "r");
	char name[128];

	for ( int c = 0; !feof(fp); c++ )
	{
		fscanf(fp, "%s", name);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
		{
			break;
		}

		if ( PHYSFS_getRealDir(name) != NULL )
		{
			std::string modelRealDir = PHYSFS_getRealDir(name);
			if ( modelRealDir.compare("./") != 0 )
			{
				fclose(fp);
				return true;
			}
		}
	}
	fclose(fp);
	return false;
}

bool physfsModelIndexUpdate(int &start, int &end, bool freePreviousModels)
{
	std::string modelsDirectory = PHYSFS_getRealDir("models/models.txt");
	char modelName[128];
	int startnum = 1;
	int endnum = nummodels;
	modelsDirectory.append(PHYSFS_getDirSeparator()).append("models/models.txt");
	FILE *fp = openDataFile(modelsDirectory.c_str(), "r");
	for ( int c = 0; !feof(fp); c++ )
	{
		fscanf(fp, "%s", modelName);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
		{
			break;
		}

		bool modelHasBeenModified = false;
		// has this model index been modified?
		std::vector<int>::iterator it = gamemods_modelsListModifiedIndexes.end();
		if ( !gamemods_modelsListModifiedIndexes.empty() )
		{
			it = std::find(gamemods_modelsListModifiedIndexes.begin(), 
				gamemods_modelsListModifiedIndexes.end(), c);
			if ( it != gamemods_modelsListModifiedIndexes.end() )
			{
				modelHasBeenModified = true; // found the model in the vector.
			}
		}

		std::string modelPath = PHYSFS_getRealDir(modelName);
		if ( modelHasBeenModified || modelPath.compare("./") != 0 )
		{
			if ( !modelHasBeenModified )
			{
				// add this model index to say we've modified it as the base dir is not default.
				gamemods_modelsListModifiedIndexes.push_back(c);
			}
			else
			{
				if ( modelPath.compare("./") == 0 )
				{
					// model returned to base directory, remove from the modified index list.
					gamemods_modelsListModifiedIndexes.erase(it);
				}
			}

			if ( models[c] != NULL )
			{
				if ( models[c]->data )
				{
					free(models[c]->data);
				}
				free(models[c]);
			}
			models[c] = loadVoxel(modelName);

			// this index is not found in the normal models folder.
			// store the lowest found model number inside startnum.
			if ( startnum == 1 || c < startnum )
			{
				startnum = c;
			}

			// store the higher end model num in endnum.
			if ( endnum == nummodels )
			{
				endnum = c + 1;
			}
			else if ( c + 1 > endnum )
			{
				endnum = c + 1;
			}
		}
	}
	if ( startnum == endnum )
	{
		endnum = std::min(static_cast<int>(nummodels), endnum + 1); // if both indices are the same, then models won't load.
	}
	printlog("[PhysFS]: Models file not in default directory... reloading models from index %d to %d\n", startnum, endnum);
	start = startnum;
	end = endnum;

	// now free polymodels as we'll be loading them up later.
	if ( freePreviousModels )
	{
		for ( int c = start; c < end; ++c )
		{
			if ( polymodels[c].faces )
			{
				free(polymodels[c].faces);
			}
			if ( polymodels[c].vbo )
			{
				SDL_glDeleteBuffers(1, &polymodels[c].vbo);
			}
			if ( polymodels[c].colors )
			{
				SDL_glDeleteBuffers(1, &polymodels[c].colors);
			}
			if ( polymodels[c].va )
			{
				SDL_glDeleteVertexArrays(1, &polymodels[c].va);
			}
			if ( polymodels[c].colors_shifted )
			{
				SDL_glDeleteBuffers(1, &polymodels[c].colors_shifted);
			}
		}
	}

	fclose(fp);
	return true;
}

bool physfsSearchSoundsToUpdate()
{
	std::string soundsDirectory = PHYSFS_getRealDir("sound/sounds.txt");
	if ( soundsDirectory.compare("./") != 0 )
	{
		return true;
	}
	soundsDirectory.append(PHYSFS_getDirSeparator()).append("sound/sounds.txt");
	FILE* fp = openDataFile(soundsDirectory.c_str(), "r");
	char name[128];

	for ( int c = 0; !feof(fp); c++ )
	{
		fscanf(fp, "%s", name);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
		{
			break;
		}

		if ( PHYSFS_getRealDir(name) != NULL )
		{
			std::string soundRealDir = PHYSFS_getRealDir(name);
			if ( soundRealDir.compare("./") != 0 )
			{
				fclose(fp);
				return true;
			}
		}
	}
	fclose(fp);
	return false;
}

void physfsReloadSounds(bool reloadAll)
{
	std::string soundsDirectory = PHYSFS_getRealDir("sound/sounds.txt");
	soundsDirectory.append(PHYSFS_getDirSeparator()).append("sound/sounds.txt");
	FILE* fp = openDataFile(soundsDirectory.c_str(), "r");
	char name[128];

	printlog("freeing sounds and loading modded sounds...\n");
	if ( reloadAll )
	{
#ifdef SOUND
		if ( sounds != NULL )
		{
			for ( int c = 0; c < numsounds; c++ )
			{
				if ( sounds[c] != NULL )
				{
#ifdef USE_FMOD
					FMOD_Sound_Release(sounds[c]);    //Free the sound in FMOD
#endif
#ifdef USE_OPENAL
					OPENAL_Sound_Release(sounds[c]); //Free the sound in OPENAL
#endif
				}
			}
		}
#endif
	}

	for ( int c = 0; !feof(fp); c++ )
	{
		fscanf(fp, "%s", name);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
		{
			break;
		}
		
		if ( PHYSFS_getRealDir(name) != NULL )
		{
			std::string soundRealDir = PHYSFS_getRealDir(name);
			if ( reloadAll || soundRealDir.compare("./") != 0 )
			{
				std::string soundFile = soundRealDir;
				soundFile.append(PHYSFS_getDirSeparator()).append(name);
#ifdef USE_FMOD
				if ( !reloadAll )
				{
					FMOD_Sound_Release(sounds[c]);
				}
				fmod_result = FMOD_System_CreateSound(fmod_system, soundFile.c_str(), (FMOD_MODE)(FMOD_SOFTWARE | FMOD_3D), NULL, &sounds[c]);
				if ( FMODErrorCheck() )
				{
					printlog("warning: failed to load '%s' listed at line %d in sounds.txt\n", name, c + 1);
				}
#endif
#ifdef USE_OPENAL
				if ( !reloadAll )
				{
					OPENAL_Sound_Release(sounds[c]);
				}
				OPENAL_CreateSound(soundFile.c_str(), true, &sounds[c]);
#endif 
			}
		}
	}
	fclose(fp);
}

bool physfsSearchTilesToUpdate()
{
	std::string tilesDirectory = PHYSFS_getRealDir("images/tiles.txt");
	tilesDirectory.append(PHYSFS_getDirSeparator()).append("images/tiles.txt");
	FILE* fp = openDataFile(tilesDirectory.c_str(), "r");
	char name[128];

	for ( int c = 0; !feof(fp); c++ )
	{
		fscanf(fp, "%s", name);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
		{
			break;
		}

		if ( PHYSFS_getRealDir(name) != NULL )
		{
			std::string tileRealDir = PHYSFS_getRealDir(name);
			if ( tileRealDir.compare("./") != 0 )
			{
				fclose(fp);
				printlog("[PhysFS]: Found modified tile in tiles/ directory, reloading all tiles...");
				return true;
			}
		}
	}
	fclose(fp);
	return false;
}

void physfsReloadTiles(bool reloadAll)
{
	std::string tilesDirectory = PHYSFS_getRealDir("images/tiles.txt");
	tilesDirectory.append(PHYSFS_getDirSeparator()).append("images/tiles.txt");
	printlog("[PhysFS]: Loading tiles from directory %s...\n", tilesDirectory.c_str());
	FILE* fp = openDataFile(tilesDirectory.c_str(), "r");
	char name[128];

	for ( int c = 0; !feof(fp); c++ )
	{
		fscanf(fp, "%s", name);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
		{
			break;
		}
		if ( PHYSFS_getRealDir(name) != NULL )
		{
			std::string tileRealDir = PHYSFS_getRealDir(name);
			if ( reloadAll || tileRealDir.compare("./") != 0 )
			{
				std::string tileFile = tileRealDir;
				tileFile.append(PHYSFS_getDirSeparator()).append(name);
				if ( tiles[c] )
				{
					SDL_FreeSurface(tiles[c]);
				}
				char fullname[128];
				strncpy(fullname, tileFile.c_str(), 127);
				tiles[c] = loadImage(fullname);
				animatedtiles[c] = false;
				lavatiles[c] = false;
				swimmingtiles[c] = false;
				if ( tiles[c] != NULL )
				{
					size_t found = tileFile.find(".png");
					if ( found != string::npos && found != 0 )
					{
						if ( tileFile.at(found - 1) >= '0' && tileFile.at(found - 1) <= '9' )
						{
							// animated tiles if the tile name ends in a number 0-9.
							animatedtiles[c] = true;
						}
					}
					if ( strstr(name, "Lava") || strstr(name, "lava") )
					{
						lavatiles[c] = true;
					}
					if ( strstr(name, "Water") || strstr(name, "water") || strstr(name, "swimtile") || strstr(name, "Swimtile") )
					{
						swimmingtiles[c] = true;
					}
				}
				else
				{
					printlog("warning: failed to load '%s' listed at line %d in %s\n", name, c + 1, tilesDirectory.c_str());
					if ( c == 0 )
					{
						printlog("tile 0 cannot be NULL!\n");
						fclose(fp);
						return;
					}
				}
			}
		}
	}
	fclose(fp);
}

bool physfsIsMapLevelListModded()
{
	std::string mapsDirectory = PHYSFS_getRealDir(LEVELSFILE);
	if ( mapsDirectory.compare("./") != 0 )
	{
		return true;
	}
	mapsDirectory.append(PHYSFS_getDirSeparator()).append(LEVELSFILE);

	std::vector<std::string> levelsList = getLinesFromDataFile(mapsDirectory);
	if ( levelsList.empty() )
	{
		return false;
	}
	std::string line = levelsList.front();
	int levelsCounted = 0;
	for ( std::vector<std::string>::const_iterator i = levelsList.begin(); i != levelsList.end(); ++i )
	{
		// process i, iterate through all the map levels until currentlevel.
		line = *i;
		if ( line[0] == '\n' )
		{
			continue;
		}
		std::size_t found = line.find(' ');
		char tempstr[1024];
		if ( found != std::string::npos )
		{
			std::string mapType = line.substr(0, found);
			std::string mapName;
			mapName = line.substr(found + 1, line.find('\n'));
			std::size_t carriageReturn = mapName.find('\r');
			if ( carriageReturn != std::string::npos )
			{
				mapName.erase(carriageReturn);
			}
			mapName = mapName.substr(0, mapName.find_first_of(" \0"));
			mapName = "maps/" + mapName + ".lmp";
			//printlog("%s", mapName.c_str());
			if ( PHYSFS_getRealDir(mapName.c_str()) != NULL )
			{
				mapsDirectory = PHYSFS_getRealDir(mapName.c_str());
				if ( mapsDirectory.compare("./") != 0 )
				{
					return true;
				}
			}
		}
		++levelsCounted;
	}

	mapsDirectory = PHYSFS_getRealDir(SECRETLEVELSFILE);
	if ( mapsDirectory.compare("./") != 0 )
	{
		return true;
	}
	mapsDirectory.append(PHYSFS_getDirSeparator()).append(SECRETLEVELSFILE);

	levelsList = getLinesFromDataFile(mapsDirectory);
	if ( levelsList.empty() )
	{
		return false;
	}
	line = levelsList.front();
	levelsCounted = 0;
	for ( std::vector<std::string>::const_iterator i = levelsList.begin(); i != levelsList.end(); ++i )
	{
		// process i, iterate through all the map levels until currentlevel.
		line = *i;
		if ( line[0] == '\n' )
		{
			continue;
		}
		std::size_t found = line.find(' ');
		char tempstr[1024];
		if ( found != std::string::npos )
		{
			std::string mapType = line.substr(0, found);
			std::string mapName;
			mapName = line.substr(found + 1, line.find('\n'));
			std::size_t carriageReturn = mapName.find('\r');
			if ( carriageReturn != std::string::npos )
			{
				mapName.erase(carriageReturn);
			}
			mapName = mapName.substr(0, mapName.find_first_of(" \0"));
			mapName = "maps/" + mapName + ".lmp";
			//printlog("%s", mapName.c_str());
			if ( PHYSFS_getRealDir(mapName.c_str()) != NULL )
			{
				mapsDirectory = PHYSFS_getRealDir(mapName.c_str());
				if ( mapsDirectory.compare("./") != 0 )
				{
					return true;
				}
			}
		}
		++levelsCounted;
	}
	return false;
}

bool physfsSearchItemSpritesToUpdate()
{
	for ( int c = 0; c < NUMITEMS; ++c )
	{
		for ( int x = 0; x < list_Size(&items[c].images); x++ )
		{
			node_t* node = list_Node(&items[c].images, x);
			string_t* string = (string_t*)node->element;
			std::string itemImgDir;
			if ( PHYSFS_getRealDir(string->data) != NULL )
			{
				itemImgDir = PHYSFS_getRealDir(string->data);
				if ( itemImgDir.compare("./") != 0 )
				{
					printlog("[PhysFS]: Found modified item sprite in items/items.txt file, reloading all item sprites...");
					return true;
				}
			}
		}
	}
	return false;
}

void physfsReloadItemSprites(bool reloadAll)
{
	for ( int c = 0; c < NUMITEMS; ++c )
	{
		bool reloadImg = reloadAll;
		if ( !reloadAll )
		{
			for ( int x = 0; x < list_Size(&items[c].images); x++ )
			{
				node_t* node = list_Node(&items[c].images, x);
				string_t* string = (string_t*)node->element;
				std::string itemImgDir;
				if ( PHYSFS_getRealDir(string->data) != NULL )
				{
					itemImgDir = PHYSFS_getRealDir(string->data);
					if ( itemImgDir.compare("./") != 0 )
					{
						reloadImg = true;
					}
				}
			}
		}
		if ( reloadImg )
		{
			// free the image data.
			//list_FreeAll(&items[c].images);
			node_t* node, *nextnode;
			for ( node = items[c].surfaces.first; node != NULL; node = nextnode )
			{
				nextnode = node->next;
				SDL_Surface** surface = (SDL_Surface**)node->element;
				if ( surface )
				{
					if ( *surface )
					{
						SDL_FreeSurface(*surface);
					}
				}
			}
			list_FreeAll(&items[c].surfaces);

			// now reload the image data.
			for ( int x = 0; x < list_Size(&items[c].images); x++ )
			{
				SDL_Surface** surface = (SDL_Surface**)malloc(sizeof(SDL_Surface*));
				node_t* node = list_AddNodeLast(&items[c].surfaces);
				node->element = surface;
				node->deconstructor = &defaultDeconstructor;
				node->size = sizeof(SDL_Surface*);

				node_t* node2 = list_Node(&items[c].images, x);
				string_t* string = (string_t*)node2->element;
				std::string itemImgDir;
				if ( PHYSFS_getRealDir(string->data) != NULL )
				{
					itemImgDir = PHYSFS_getRealDir(string->data);
					itemImgDir.append(PHYSFS_getDirSeparator()).append(string->data);
				}
				else
				{
					itemImgDir = string->data;
				}
				char imgFileChar[256];
				strncpy(imgFileChar, itemImgDir.c_str(), 255);
				*surface = loadImage(imgFileChar);
			}
		}
	}
}

bool physfsSearchItemsTxtToUpdate()
{
	std::string itemsTxtDirectory = PHYSFS_getRealDir("items/items.txt");
	if ( itemsTxtDirectory.compare("./") != 0 )
	{
		printlog("[PhysFS]: Found modified items/items.txt file, reloading all item information...");
		return true;
	}
	return false;
}

bool physfsSearchItemsGlobalTxtToUpdate()
{
	std::string itemsTxtDirectory = PHYSFS_getRealDir("items/items_global.txt");
	if ( itemsTxtDirectory.compare("./") != 0 )
	{
		printlog("[PhysFS]: Found modified items/items_global.txt file, reloading item spawn levels...");
		return true;
	}
	return false;
}

void physfsReloadItemsTxt()
{
	std::string itemsTxtDirectory = PHYSFS_getRealDir("items/items.txt");
	itemsTxtDirectory.append(PHYSFS_getDirSeparator()).append("items/items.txt");
	FILE* fp = openDataFile(itemsTxtDirectory.c_str(), "r");
	char buffer[128];

	for ( int c = 0; !feof(fp) && c < NUMITEMS; ++c )
	{
		//if ( c > ARTIFACT_BOW )
		//{
		//	int newItems = c - ARTIFACT_BOW - 1;
		//	items[c].name_identified = language[2200 + newItems * 2];
		//	items[c].name_unidentified = language[2201 + newItems * 2];
		//}
		//else
		//{
		//	items[c].name_identified = language[1545 + c * 2];
		//	items[c].name_unidentified = language[1546 + c * 2];
		//}
		fscanf(fp, "%d", &items[c].index);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
		{
			break;
		}
		fscanf(fp, "%d", &items[c].fpindex);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
		{
			break;
		}
		fscanf(fp, "%d", &items[c].variations);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
		{
			break;
		}
		fscanf(fp, "%s", buffer);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
		{
			break;
		}
		if ( !strcmp(buffer, "WEAPON") )
		{
			items[c].category = WEAPON;
		}
		else if ( !strcmp(buffer, "ARMOR") )
		{
			items[c].category = ARMOR;
		}
		else if ( !strcmp(buffer, "AMULET") )
		{
			items[c].category = AMULET;
		}
		else if ( !strcmp(buffer, "POTION") )
		{
			items[c].category = POTION;
		}
		else if ( !strcmp(buffer, "SCROLL") )
		{
			items[c].category = SCROLL;
		}
		else if ( !strcmp(buffer, "MAGICSTAFF") )
		{
			items[c].category = MAGICSTAFF;
		}
		else if ( !strcmp(buffer, "RING") )
		{
			items[c].category = RING;
		}
		else if ( !strcmp(buffer, "SPELLBOOK") )
		{
			items[c].category = SPELLBOOK;
		}
		else if ( !strcmp(buffer, "TOOL") )
		{
			items[c].category = TOOL;
		}
		else if ( !strcmp(buffer, "FOOD") )
		{
			items[c].category = FOOD;
		}
		else if ( !strcmp(buffer, "BOOK") )
		{
			items[c].category = BOOK;
		}
		else if ( !strcmp(buffer, "THROWN") )
		{
			items[c].category = THROWN;
		}
		else if ( !strcmp(buffer, "SPELL_CAT") )
		{
			items[c].category = SPELL_CAT;
		}
		else
		{
			items[c].category = GEM;
		}
		fscanf(fp, "%d", &items[c].weight);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
		{
			break;
		}
		fscanf(fp, "%d", &items[c].value);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
		{
			break;
		}

		list_FreeAll(&items[c].images);

		while ( 1 )
		{
			string_t* string = (string_t*)malloc(sizeof(string_t));
			string->data = (char*)malloc(sizeof(char) * 64);
			string->lines = 1;

			node_t* node = list_AddNodeLast(&items[c].images);
			node->element = string;
			node->deconstructor = &stringDeconstructor;
			node->size = sizeof(string_t);
			string->node = node;

			int x = 0;
			bool fileend = false;
			while ( (string->data[x] = fgetc(fp)) != '\n' )
			{
				if ( feof(fp) )
				{
					fileend = true;
					break;
				}
				x++;
			}
			if ( x == 0 || fileend )
			{
				list_RemoveNode(node);
				break;
			}
			string->data[x] = 0;
		}
	}

	fclose(fp);
}

bool physfsSearchMonsterLimbFilesToUpdate()
{
	bool requiresUpdate = false;
	for ( int c = 1; c < NUMMONSTERS; c++ )
	{
		char filename[256];
		strcpy(filename, "models/creatures/");
		strcat(filename, monstertypename[c]);
		strcat(filename, "/limbs.txt");
		if ( PHYSFS_getRealDir(filename) == NULL ) // some monsters don't have limbs.
		{
			continue;
		}
		std::string limbsDir = PHYSFS_getRealDir(filename);
		if ( limbsDir.compare("./") != 0 )
		{
			printlog("[PhysFS]: Found modified limbs.txt file for monster %s, reloading all limb information...", monstertypename[c]);
			requiresUpdate = true;
		}
	}
	return requiresUpdate;
}

void physfsReloadMonsterLimbFiles()
{
	int x;
	FILE* fp;
	for ( int c = 1; c < NUMMONSTERS; c++ )
	{
		// initialize all offsets to zero
		for ( x = 0; x < 20; x++ )
		{
			limbs[c][x][0] = 0;
			limbs[c][x][1] = 0;
			limbs[c][x][2] = 0;
		}

		// open file
		char filename[256];
		strcpy(filename, "models/creatures/");
		strcat(filename, monstertypename[c]);
		strcat(filename, "/limbs.txt");
		if ( PHYSFS_getRealDir(filename) == NULL ) // some monsters don't have limbs
		{
			continue;
		}
		std::string limbsDir = PHYSFS_getRealDir(filename);
		limbsDir.append(PHYSFS_getDirSeparator()).append(filename);
		if ( (fp = openDataFile(limbsDir.c_str(), "r")) == NULL )
		{
			continue;
		}

		// read file
		int line;
		for ( line = 1; feof(fp) == 0; line++ )
		{
			char data[256];
			int limb = 20;
			int dummy;

			// read line from file
			fgets(data, 256, fp);

			// skip blank and comment lines
			if ( data[0] == '\n' || data[0] == '\r' || data[0] == '#' )
			{
				continue;
			}

			// process line
			if ( sscanf(data, "%d", &limb) != 1 || limb >= 20 || limb < 0 )
			{
				printlog("warning: syntax error in '%s':%d\n invalid limb index!\n", limbsDir.c_str(), line);
				continue;
			}
			if ( sscanf(data, "%d %f %f %f\n", &dummy, &limbs[c][limb][0], &limbs[c][limb][1], &limbs[c][limb][2]) != 4 )
			{
				printlog("warning: syntax error in '%s':%d\n invalid limb offsets!\n", limbsDir.c_str(), line);
				continue;
			}
		}
		// close file
		fclose(fp);
	}
}

bool physfsSearchSystemImagesToUpdate()
{
	bool requireReload = false;
	systemResourceImagesToReload.clear();

	for ( std::vector<std::pair<SDL_Surface**, std::string>>::const_iterator it = systemResourceImages.begin(); it != systemResourceImages.end(); ++it )
	{
		std::pair<SDL_Surface**, std::string> line = *it;
		std::string imgFile = line.second;
		if ( PHYSFS_getRealDir(imgFile.c_str()) != NULL)
		{
			std::string imgDir = PHYSFS_getRealDir(imgFile.c_str());
			if ( imgDir.compare("./") != 0 )
			{
				printlog("[PhysFS]: Found modified %s file, reloading system image...", imgFile.c_str());
				requireReload = true;
				systemResourceImagesToReload.push_back(line);
			}
		}
	}
	return requireReload;
}

void physfsReloadSystemImages()
{
	if ( !systemResourceImagesToReload.empty() )
	{
		for ( std::vector<std::pair<SDL_Surface**, std::string>>::const_iterator it = systemResourceImagesToReload.begin(); it != systemResourceImagesToReload.end(); ++it )
		{
			std::pair<SDL_Surface**, std::string> line = *it;
			if ( *(line.first) ) // SDL_Surface* pointer exists
			{
				// load a new image, getting the VFS system location.
				std::string filepath = PHYSFS_getRealDir(line.second.c_str());
				filepath.append(PHYSFS_getDirSeparator()).append(line.second);

				char filepathChar[1024];
				strncpy(filepathChar, filepath.c_str(), 1023);

				SDL_FreeSurface(*(line.first));
				*(line.first) = loadImage(filepathChar);

				if ( !(*(line.first)))
				{
					printlog("[PhysFS]: Error: Failed to reload %s!", filepath.c_str());
				}
			}
		}
	}
}
