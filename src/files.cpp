/*-------------------------------------------------------------------------------

	BARONY
	File: files.cpp
	Desc: contains code for file i/o

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include <sys/types.h>
#include <dirent.h>
#include "main.hpp"
#include "sound.hpp"
#include "entity.hpp"

/*-------------------------------------------------------------------------------

	glLoadTexture
	
	Binds the given image to an opengl texture name

-------------------------------------------------------------------------------*/

void glLoadTexture(SDL_Surface *image, int texnum) {
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

/*-------------------------------------------------------------------------------

	loadImage
	
	Loads the image specified in filename, binds it to an opengl texture name,
	and returns the image as an SDL_Surface

-------------------------------------------------------------------------------*/

SDL_Surface *loadImage(char *filename) {
	SDL_Surface *originalSurface;

	if( imgref>=MAXTEXTURES ) {
		printlog("critical error! No more room in allsurfaces[], MAXTEXTURES reached.\n");
		printlog("aborting...\n");
		exit(1);
	}
	if( (originalSurface=IMG_Load(filename))==NULL ) {
		printlog("error: failed to load image '%s'\n",filename);
		exit(1); // critical error
		return NULL;
	}

	// translate the original surface to an RGBA surface
	//int w = pow(2, ceil( log(std::max(originalSurface->w,originalSurface->h))/log(2) ) ); // round up to the nearest power of two
	SDL_Surface* newSurface = SDL_CreateRGBSurface(0, originalSurface->w, originalSurface->h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	SDL_BlitSurface(originalSurface, NULL, newSurface, NULL); // blit onto a purely RGBA Surface

	// load the new surface as a GL texture
	allsurfaces[imgref] = newSurface;
	allsurfaces[imgref]->refcount = imgref+1;
	glLoadTexture(allsurfaces[imgref],imgref);

	// free the translated surface
	SDL_FreeSurface(originalSurface);

	imgref++;
	return allsurfaces[imgref-1];
}

/*-------------------------------------------------------------------------------

	loadVoxel
	
	Loads a voxel model from the given filename

-------------------------------------------------------------------------------*/

voxel_t *loadVoxel(char *filename2) {
	char *filename;
	FILE *file;
	voxel_t *model;

	if(filename2!=NULL) {
		if( strstr(filename2,".vox") == NULL ) {
			filename = (char *) malloc(sizeof(char)*256);
			strcpy(filename,filename2);
			strcat(filename,".vox");
		} else {
			filename = (char *) malloc(sizeof(char)*256);
			strcpy(filename,filename2);
		}
		if((file = fopen(filename,"rb"))==NULL) {
			free(filename);
			return NULL;
		}
		model = (voxel_t *) malloc(sizeof(voxel_t));
		model->sizex = 0; fread(&model->sizex,sizeof(Sint32),1,file);
		model->sizey = 0; fread(&model->sizey,sizeof(Sint32),1,file);
		model->sizez = 0; fread(&model->sizez,sizeof(Sint32),1,file);
		model->data = (Uint8 *) malloc(sizeof(Uint8)*model->sizex*model->sizey*model->sizez);
		memset(model->data,0,sizeof(Uint8)*model->sizex*model->sizey*model->sizez);
		fread(model->data,sizeof(Uint8),model->sizex*model->sizey*model->sizez,file);
		fread(&model->palette,sizeof(Uint8),256*3,file);
		int c;
		for( c=0; c<256; c++ ) {
			model->palette[c][0]=model->palette[c][0]<<2;
			model->palette[c][1]=model->palette[c][1]<<2;
			model->palette[c][2]=model->palette[c][2]<<2;
		}
		fclose(file);
		free(filename);
		
		return model;
	} else {
		return NULL;
	}
}

/*-------------------------------------------------------------------------------

	loadMap
	
	Loads a map from the given filename

-------------------------------------------------------------------------------*/

int loadMap(char *filename2, map_t *destmap, list_t *entlist) {
	FILE *fp;
	char valid_data[11];
	Uint32 numentities;
	Uint32 c;
	Sint32 x, y;
	Entity *entity;
	Sint32 sprite;
	char *filename;

	char oldmapname[64];
	strcpy(oldmapname,map.name);
	
	if( filename2 != NULL && strcmp(filename2,"") ) {
		c=0;
		while(1) {
			if(filename2[c]==0)
				break;
			c++;
		}
		filename = (char *) malloc(sizeof(char)*256);
		strcpy(filename,"maps/");
		strcat(filename,filename2);
		
		if( strcmp(filename,"..") && strcmp(filename,".") ) {
			// add extension if missing
			if( strstr(filename,".lmp") == NULL )
				strcat(filename,".lmp");
			
			// load the file!
			if((fp = fopen(filename, "rb")) == NULL) {
				printlog("warning: failed to open file '%s' for map loading!\n",filename);
				if( destmap == &map && game ) {
					printlog("error: main map failed to load, aborting.\n");
					mainloop=0;
				}
				free(filename);
				return -1;
			}
		} else {
			printlog("warning: failed to open file '%s' for map loading!\n",filename);
			if( destmap == &map && game ) {
				printlog("error: main map failed to load, aborting.\n");
				mainloop=0;
			}
			free(filename);
			return -1;
		}
		fread(valid_data, sizeof(char), strlen("BARONY"), fp);
		if( strncmp(valid_data,"BARONY",strlen("BARONY")) ) {
			printlog("warning: file '%s' is an invalid map file.\n",filename);
			fclose(fp);
			if( destmap == &map && game ) {
				printlog("error: main map failed to load, aborting.\n");
				mainloop=0;
			}
			free(filename);
			return -1;
		}
		list_FreeAll(entlist);
		if( destmap == &map ) {
			// remove old lights
			list_FreeAll(&light_l);
		}
		if( destmap->tiles != NULL )
			free(destmap->tiles);
		fread(destmap->name, sizeof(char), 32, fp); // map name
		fread(destmap->author, sizeof(char), 32, fp); // map author
		fread(&destmap->width, sizeof(Uint32), 1, fp); // map width
		fread(&destmap->height, sizeof(Uint32), 1, fp); // map height
		destmap->tiles = (Sint32 *) malloc(sizeof(Sint32)*destmap->width*destmap->height*MAPLAYERS);
		fread(destmap->tiles, sizeof(Sint32), destmap->width*destmap->height*MAPLAYERS, fp);
		fread(&numentities,sizeof(Uint32), 1, fp); // number of entities on the map
		for(c=0; c<numentities; c++) {
			fread(&sprite,sizeof(Sint32), 1, fp);
			entity = newEntity(sprite,0,entlist);
			fread(&x, sizeof(Sint32), 1, fp);
			fread(&y, sizeof(Sint32), 1, fp);
			entity->x = x;
			entity->y = y;
		}
		free(filename);
		fclose(fp);
		
		if( destmap == &map ) {
			nummonsters=0;
			minotaurlevel=0;

#ifdef HAVE_FMOD
			if( strcmp(oldmapname,map.name) )
				levelmusicplaying=FALSE;
#endif
			
			// create new lightmap
			if(lightmap!=NULL)
				free(lightmap);
			lightmap=(int *) malloc(sizeof(Sint32)*destmap->width*destmap->height);
			if( strncmp(map.name, "Hell", 4) ) {
				for(c=0; c<destmap->width*destmap->height; c++ )
					lightmap[c]=0;
			} else {
				for(c=0; c<destmap->width*destmap->height; c++ )
					lightmap[c]=32;
			}
			
			// create a new vismap
			if(vismap!=NULL)
				free(vismap);
			vismap=(bool *) calloc(destmap->width*destmap->height,sizeof(bool));
	
			// reset minimap
			for( x=0; x<64; x++ )
				for( y=0; y<64; y++ )
					minimap[y][x]=0;

			// reset camera
			if( game ) {
				camera.x = -32;
				camera.y = -32;
				camera.z = 0;
				camera.ang = 3*PI/2;
				camera.vang = 0;
			} else {
				camera.x = 2;
				camera.y = 2;
				camera.z = 0;
				camera.ang = 0;
				camera.vang = 0;
			}
			
			// shoparea
			if( shoparea )
				free(shoparea);
			shoparea=(bool *) malloc(sizeof(bool)*destmap->width*destmap->height);
			for( x=0; x<destmap->width; x++ )
				for( y=0; y<destmap->height; y++ )
					shoparea[y+x*destmap->height] = FALSE;
		}
		
		for( c=0; c<512; c++ )
			keystatus[c] = 0;
		
		return numentities;
	} else {
		return -1;
	}
}

/*-------------------------------------------------------------------------------

	saveMap
	
	Saves a map to the given filename

-------------------------------------------------------------------------------*/

int saveMap(char *filename2) {
	FILE *fp;
	Uint32 numentities=0;
	node_t *node;
	Entity *entity;
	char *filename;
	Sint32 x, y;
	
	if( filename2 != NULL && strcmp(filename2,"") ) {
		filename = (char *) malloc(sizeof(char)*256);
		strcpy(filename,"maps/");
		strcat(filename,filename2);
		
		if( strstr(filename,".lmp") == NULL )
			strcat(filename,".lmp");
		if((fp = fopen(filename, "wb")) == NULL) {
			printlog("warning: failed to open file '%s' for map saving!\n",filename);
			return 1;
		}

		fwrite("BARONY", sizeof(char), strlen("BARONY"), fp); // magic code
		fwrite(map.name, sizeof(char), 32, fp); // map filename
		fwrite(map.author, sizeof(char), 32, fp); // map author
		fwrite(&map.width, sizeof(Uint32), 1, fp); // map width
		fwrite(&map.height, sizeof(Uint32), 1, fp); // map height
		fwrite(map.tiles, sizeof(Sint32), map.width*map.height*MAPLAYERS, fp);
		for(node=map.entities->first;node!=NULL;node=node->next)
			numentities++;
		fwrite(&numentities,sizeof(Uint32), 1, fp); // number of entities on the map
		for(node=map.entities->first;node!=NULL;node=node->next) {
			entity = (Entity *) node->element;
			fwrite(&entity->sprite,sizeof(Sint32), 1, fp);
			x = entity->x;
			y = entity->y;
			fwrite(&x, sizeof(Sint32), 1, fp);
			fwrite(&y, sizeof(Sint32), 1, fp);
		}
		fclose(fp);
		free(filename);
		return 0;
	} else
		return 1;
}

/*-------------------------------------------------------------------------------

	readFile

	Simply reads the contents of an entire file into a char array

-------------------------------------------------------------------------------*/

char *readFile(char *filename) {
	char *file_contents=NULL;
	long input_file_size;
	FILE *input_file = fopen(filename, "rb");
	if( input_file ) {
		fseek(input_file, 0, SEEK_END);
		input_file_size = ftell(input_file);
		rewind(input_file);
		file_contents = static_cast<char*>(malloc((input_file_size+1) * sizeof(char)));
		fread(file_contents, sizeof(char), input_file_size, input_file);
		file_contents[input_file_size] = 0;
		fclose(input_file);
	}

	return file_contents;
}

/*-------------------------------------------------------------------------------

	directoryContents

	Stores the contents of a directory in a list

-------------------------------------------------------------------------------*/

list_t *directoryContents(char *directory) {
	list_t *list = NULL; // list of strings
	DIR *dir = NULL;
	struct dirent *entry = NULL;
	dir = opendir(directory);

	if( !dir ) {
		printlog( "[directoryContents()] Failed to open directory \"%s\".\n", directory);
		return NULL;
	}

	list = (list_t *) malloc(sizeof(list_t));
	list->first = NULL; list->last = NULL;

	while ((entry=readdir(dir))!=NULL) {
		strcpy(tempstr, directory);
		strcat(tempstr, entry->d_name);

		DIR *newdir=NULL;
		if( (newdir=opendir(tempstr))==NULL ) {
			newString(list,0xFFFFFFFF,entry->d_name);
		} else {
			closedir(newdir);
		}
	}

	closedir(dir);

	return list;
}
