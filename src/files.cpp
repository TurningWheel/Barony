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

std::vector<int> gamemods_modelsListModifiedIndexes;

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


bool completePath(char *dest, const char * const filename) {
	if (!(filename && filename[0])) {
		return false;
	}

	// Already absolute
	if (filename[0] == '/') {
		strncpy(dest, filename, 1024);
		return true;
	}

#ifdef WINDOWS
	// Already absolute (drive letter in path)
	if ( filename[1] == ':' ) {
		strncpy(dest, filename, 1024);
		return true;
	}
#endif

	snprintf(dest, 1024, "%s/%s", datadir, filename);
	return true;
}

FILE* openDataFile(const char * const filename, const char * const mode) {
	char path[1024];
	completePath(path, filename);
	FILE * result = fopen(path, mode);
	if (!result) {
		printlog("Could not open '%s': %s", path, strerror(errno));
	}
	return result;
}

DIR* openDataDir(const char * const name) {
	char path[1024];
	completePath(path, name);
	DIR * result = opendir(path);
	if (!result) {
		printlog("Could not open '%s': %s", path, strerror(errno));
	}
	return result;
}


bool dataPathExists(const char * const path) {
	char full_path[1024];
	completePath(full_path, path);
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

int loadMap(const char* filename2, map_t* destmap, list_t* entlist, list_t* creatureList)
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


							fread(&myStats->sex, sizeof(sex_t), 1, fp);
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

int physfsLoadMapFile(int levelToLoad, Uint32 seed, bool useRandSeed)
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
			return loadMap(mapName.c_str(), &map, map.entities, map.creatures);
		}
		else if ( mapType.compare("gen:") == 0 )
		{
			strncpy(tempstr, mapName.c_str(), mapName.length());
			tempstr[mapName.length()] = '\0';
			if ( useRandSeed )
			{
				return generateDungeon(tempstr, rand());
			}
			else
			{
				return generateDungeon(tempstr, seed);
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
		if ( sounds != NULL )
		{
			for ( int c = 0; c < numsounds; c++ )
			{
				if ( sounds[c] != NULL )
				{
#ifdef USE_FMOD
					FMOD_Sound_Release(sounds[c]);    //Free the sound in FMOD
#elif USE_OPENAL
					OPENAL_Sound_Release(sounds[c]); //Free the sound in OPENAL
#endif
				}
			}
		}
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
#elif USE_OPENAL
				if ( !reloadAll )
				{
					OPENAL_Sound_Release(sounds[c]);
				}
				OPENAL_CreateSound(name, true, &sounds[c]);
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
				strncpy(name, tileFile.c_str(), 127);
				tiles[c] = loadImage(name);
				animatedtiles[c] = false;
				lavatiles[c] = false;
				swimmingtiles[c] = false;
				if ( tiles[c] != NULL )
				{
					for ( int x = 0; x < strlen(name); x++ )
					{
						if ( name[x] >= '0' && name[x] < '9' )
						{
							// animated tiles if the tile name ends in a number 0-9.
							animatedtiles[c] = true;
							break;
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
