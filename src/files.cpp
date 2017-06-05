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
#include "main.hpp"
#include "files.hpp"
#include "sound.hpp"
#include "entity.hpp"

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


bool completeDataPath(char *dest, const char * const filename) {
	if (!(filename && filename[0])) {
		return false;
	}

	// Already absolute
	if (filename[0] == '/') {
		strncpy(dest, filename, 1024);
		return true;
	}

	snprintf(dest, 1024, "%s/%s", datadir, filename);
	return true;
}

FILE* openDataFile(const char * const filename, const char * const mode) {
	char path[1024];
	completeDataPath(path, filename);
	FILE * result = fopen(path, mode);
	if (!result) {
		printlog("Could not open '%s': %s", path, strerror(errno));
	}
	return result;
}

DIR* openDataDir(const char * const name) {
	char path[1024];
	completeDataPath(path, name);
	DIR * result = opendir(path);
	if (!result) {
		printlog("Could not open '%s': %s", path, strerror(errno));
	}
	return result;
}


bool dataPathExists(const char * const path) {
	char full_path[1024];
	completeDataPath(full_path, path);
	return access(full_path, F_OK) != -1;
}


/*-------------------------------------------------------------------------------

	loadImage

	Loads the image specified in filename, binds it to an opengl texture name,
	and returns the image as an SDL_Surface

-------------------------------------------------------------------------------*/

SDL_Surface* loadImage(char* filename)
{
	char full_path[1024];
	completeDataPath(full_path, filename);
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

		if ((file = openDataFile(filename, "rb")) == NULL)
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

int loadMap(char* filename2, map_t* destmap, list_t* entlist)
{
	FILE* fp;
	char valid_data[11];
	Uint32 numentities;
	Uint32 c;
	Sint32 x, y;
	Entity* entity;
	Sint32 sprite;
	char filename[256];

	char oldmapname[64];
	strcpy(oldmapname, map.name);

	printlog("LoadMap %s", filename2);

	if (! (filename2 && filename2[0])) {
		printlog("map filename empty or null");
		return -1;
	}

	strcpy(filename, "maps/");
	strcat(filename, filename2);

	// add extension if missing
	if ( strstr(filename, ".lmp") == NULL )
	{
		strcat(filename, ".lmp");
	}

	// load the file!
	if ((fp = openDataFile(filename, "rb")) == NULL)
	{
		printlog("warning: failed to open file '%s' for map loading!\n", filename);
		if ( destmap == &map && game )
		{
			printlog("error: main map failed to load, aborting.\n");
			mainloop = 0;
		}
		return -1;
	}

	fread(valid_data, sizeof(char), strlen("BARONY"), fp);
	if ( strncmp(valid_data, "BARONY", strlen("BARONY")) )
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
	destmap->tiles = (Sint32*) malloc(sizeof(Sint32) * destmap->width * destmap->height * MAPLAYERS);
	fread(destmap->tiles, sizeof(Sint32), destmap->width * destmap->height * MAPLAYERS, fp);
	fread(&numentities, sizeof(Uint32), 1, fp); // number of entities on the map
	for (c = 0; c < numentities; c++)
	{
		fread(&sprite, sizeof(Sint32), 1, fp);
		entity = newEntity(sprite, 0, entlist);
		fread(&x, sizeof(Sint32), 1, fp);
		fread(&y, sizeof(Sint32), 1, fp);
		entity->x = x;
		entity->y = y;
	}
	fclose(fp);

	if ( destmap == &map )
	{
		nummonsters = 0;
		minotaurlevel = 0;

#if defined (HAVE_FMOD) || defined(HAVE_OPENAL)
		if ( strcmp(oldmapname, map.name) )
		{
			levelmusicplaying = false;
		}
#endif

		// create new lightmap
		if (lightmap != NULL)
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
			for ( y = 0; y < 64; y++ )
			{
				minimap[y][x] = 0;
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

	return numentities;
}

/*-------------------------------------------------------------------------------

	saveMap

	Saves a map to the given filename

-------------------------------------------------------------------------------*/

int saveMap(char* filename2)
{
	FILE* fp;
	Uint32 numentities = 0;
	node_t* node;
	Entity* entity;
	char* filename;
	Sint32 x, y;

	if ( filename2 != NULL && strcmp(filename2, "") )
	{
		filename = (char*) malloc(sizeof(char) * 256);
		strcpy(filename, "maps/");
		strcat(filename, filename2);

		if ( strstr(filename, ".lmp") == NULL )
		{
			strcat(filename, ".lmp");
		}
		if ((fp = openDataFile(filename, "wb")) == NULL)
		{
			printlog("warning: failed to open file '%s' for map saving!\n", filename);
			return 1;
		}

		fwrite("BARONY", sizeof(char), strlen("BARONY"), fp); // magic code
		fwrite(map.name, sizeof(char), 32, fp); // map filename
		fwrite(map.author, sizeof(char), 32, fp); // map author
		fwrite(&map.width, sizeof(Uint32), 1, fp); // map width
		fwrite(&map.height, sizeof(Uint32), 1, fp); // map height
		fwrite(map.tiles, sizeof(Sint32), map.width * map.height * MAPLAYERS, fp);
		for (node = map.entities->first; node != NULL; node = node->next)
		{
			numentities++;
		}
		fwrite(&numentities, sizeof(Uint32), 1, fp); // number of entities on the map
		for (node = map.entities->first; node != NULL; node = node->next)
		{
			entity = (Entity*) node->element;
			fwrite(&entity->sprite, sizeof(Sint32), 1, fp);
			x = entity->x;
			y = entity->y;
			fwrite(&x, sizeof(Sint32), 1, fp);
			fwrite(&y, sizeof(Sint32), 1, fp);
		}
		fclose(fp);
		free(filename);
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

list_t* directoryContents(char* directory)
{
	list_t* list = NULL; // list of strings
	char fullPath[1024];
	completeDataPath(fullPath, directory);
	DIR* dir = opendir(fullPath);
	struct dirent* entry = NULL;

	if ( !dir )
	{
		printlog( "[directoryContents()] Failed to open directory \"%s\".\n", directory);
		return NULL;
	}

	list = (list_t*) malloc(sizeof(list_t));
	list->first = NULL;
	list->last = NULL;

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
		if ((cur.st_mode & S_IFMT) == S_IFREG)
		{
			newString(list, 0xFFFFFFFF, entry->d_name);
		}
	}

	closedir(dir);

	return list;
}
