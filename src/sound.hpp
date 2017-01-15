/*-------------------------------------------------------------------------------

	BARONY
	File: sound.hpp
	Desc: Defines sound related stuff.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once


#include <stdio.h>
#ifdef HAVE_FMOD
#include "fmod.h"
#endif


//Pointer to the FMOD system.
#ifdef HAVE_FMOD

#define SOUND
#define MUSIC

extern FMOD_SYSTEM* fmod_system;

extern FMOD_RESULT fmod_result;

extern int fmod_maxchannels;
extern int fmod_flags;
extern void* fmod_extdriverdata;
extern bool levelmusicplaying;

extern bool shopmusicplaying;
extern bool combatmusicplaying;
extern bool minotaurmusicplaying;
extern bool herxmusicplaying;
extern bool devilmusicplaying;
extern bool olddarkmap;

extern FMOD_SOUND** sounds;
extern Uint32 numsounds;
extern FMOD_SOUND** minesmusic;
#define NUMMINESMUSIC 4
extern FMOD_SOUND** swampmusic;
#define NUMSWAMPMUSIC 3
extern FMOD_SOUND** labyrinthmusic;
#define NUMLABYRINTHMUSIC 3
extern FMOD_SOUND** ruinsmusic;
#define NUMRUINSMUSIC 3
extern FMOD_SOUND** underworldmusic;
#define NUMUNDERWORLDMUSIC 3
extern FMOD_SOUND** hellmusic;
#define NUMHELLMUSIC 3
extern FMOD_SOUND* intromusic, *intermissionmusic, *minetownmusic, *splashmusic, *librarymusic, *shopmusic, *storymusic;
extern FMOD_SOUND** minotaurmusic, *herxmusic, *templemusic;
extern FMOD_SOUND* endgamemusic, *escapemusic, *devilmusic;
extern FMOD_SOUND* introductionmusic;
#define NUMMINOTAURMUSIC 2
//TODO: Automatically scan the music folder for a mines subdirectory and use all the music for the mines or something like that. I'd prefer something neat like for that loading music for a level, anyway. And I can just reuse the code I had for ORR.

extern FMOD_CHANNEL* music_channel, *music_channel2, *music_resume; //TODO: List of music, play first one, fade out all the others? Eh, maybe some other day. //music_resume is the music to resume after, say, combat or shops. //TODO: Clear music_resume every biome change. Or otherwise validate it for that level set.

extern FMOD_CHANNELGROUP* sound_group, *music_group;

/*
 * Checks for FMOD errors. Store return value of all FMOD functions in fmod_result so that this funtion can access it and check for errors.
 * Returns TRUE on error (and prints an error message), FALSE if everything went fine.
 */
bool FMODErrorCheck();

//Updates FMOD and whatnot.
void sound_update();

FMOD_CHANNEL* playSoundPlayer(int player, Uint32 snd, int vol);
FMOD_CHANNEL* playSoundPos(double x, double y, Uint32 snd, int vol);
FMOD_CHANNEL* playSoundPosLocal(double x, double y, Uint32 snd, int vol);
FMOD_CHANNEL* playSoundEntity(Entity* entity, Uint32 snd, int vol);
FMOD_CHANNEL* playSoundEntityLocal(Entity* entity, Uint32 snd, int vol);
FMOD_CHANNEL* playSound(Uint32 snd, int vol);
FMOD_CHANNEL* playSoundVelocity(); //TODO: Write.

void playmusic(FMOD_SOUND* sound, bool loop, bool crossfade, bool resume); //Automatically crossfades. NOTE: Resets fadein and fadeout increments to the defualts every time it is called. You'll have to change the fadein and fadeout increments AFTER calling this function.

void handleLevelMusic(); //Manages and updates the level music.

extern float fadein_increment, fadeout_increment, default_fadein_increment, default_fadeout_increment;

#else
void* playSound(Uint32, int);
void* playSoundPos(double x, double y, Uint32, int);
void* playSoundPosLocal(double, double, Uint32, int);
void* playSoundEntity(Entity*, Uint32, int);
void* playSoundEntityLocal(Entity*, Uint32, int);
void* playSoundPlayer(int, Uint32, int);
#endif
