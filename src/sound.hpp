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
#ifdef HAVE_OPENAL
#ifdef APPLE
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif
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
extern FMOD_SOUND** cavesmusic;
extern FMOD_SOUND** citadelmusic;
#define NUMCAVESMUSIC 3
#define NUMCITADELMUSIC 2
//TODO: Automatically scan the music folder for a mines subdirectory and use all the music for the mines or something like that. I'd prefer something neat like for that loading music for a level, anyway. And I can just reuse the code I had for ORR.

extern FMOD_CHANNEL* music_channel, *music_channel2, *music_resume; //TODO: List of music, play first one, fade out all the others? Eh, maybe some other day. //music_resume is the music to resume after, say, combat or shops. //TODO: Clear music_resume every biome change. Or otherwise validate it for that level set.

extern FMOD_CHANNELGROUP* sound_group, *music_group;

/*
 * Checks for FMOD errors. Store return value of all FMOD functions in fmod_result so that this funtion can access it and check for errors.
 * Returns true on error (and prints an error message), false if everything went fine.
 */
bool FMODErrorCheck();

//Updates FMOD and whatnot.
void sound_update();

FMOD_CHANNEL* playSoundPlayer(int player, Uint32 snd, int vol);
FMOD_CHANNEL* playSoundPos(real_t x, real_t y, Uint32 snd, int vol);
FMOD_CHANNEL* playSoundPosLocal(real_t x, real_t y, Uint32 snd, int vol);
FMOD_CHANNEL* playSoundEntity(Entity* entity, Uint32 snd, int vol);
FMOD_CHANNEL* playSoundEntityLocal(Entity* entity, Uint32 snd, int vol);
FMOD_CHANNEL* playSound(Uint32 snd, int vol);
FMOD_CHANNEL* playSoundVelocity(); //TODO: Write.

void playmusic(FMOD_SOUND* sound, bool loop, bool crossfade, bool resume); //Automatically crossfades. NOTE: Resets fadein and fadeout increments to the defualts every time it is called. You'll have to change the fadein and fadeout increments AFTER calling this function.

void handleLevelMusic(); //Manages and updates the level music.

extern float fadein_increment, fadeout_increment, default_fadein_increment, default_fadeout_increment;

#elif defined HAVE_OPENAL

#define SOUND
#define MUSIC

extern ALCcontext *openal_context;
extern ALCdevice  *openal_device;

extern int openal_maxchannels;

extern bool levelmusicplaying;

extern bool shopmusicplaying;
extern bool combatmusicplaying;
extern bool minotaurmusicplaying;
extern bool herxmusicplaying;
extern bool devilmusicplaying;
extern bool olddarkmap;

struct OPENAL_CHANNELGROUP;

struct OPENAL_BUFFER;
struct OPENAL_SOUND;

struct FMOD_VECTOR {
	float x,y,z;
};

extern OPENAL_BUFFER** sounds;
extern Uint32 numsounds;
extern OPENAL_BUFFER** minesmusic;
#define NUMMINESMUSIC 4
extern OPENAL_BUFFER** swampmusic;
#define NUMSWAMPMUSIC 3
extern OPENAL_BUFFER** labyrinthmusic;
#define NUMLABYRINTHMUSIC 3
extern OPENAL_BUFFER** ruinsmusic;
#define NUMRUINSMUSIC 3
extern OPENAL_BUFFER** underworldmusic;
#define NUMUNDERWORLDMUSIC 3
extern OPENAL_BUFFER** hellmusic;
#define NUMHELLMUSIC 3
extern OPENAL_BUFFER* intromusic, *intermissionmusic, *minetownmusic, *splashmusic, *librarymusic, *shopmusic, *storymusic;
extern OPENAL_BUFFER** minotaurmusic, *herxmusic, *templemusic;
extern OPENAL_BUFFER* endgamemusic, *escapemusic, *devilmusic;
extern OPENAL_BUFFER* introductionmusic;
#define NUMMINOTAURMUSIC 2
extern OPENAL_BUFFER** cavesmusic;
extern OPENAL_BUFFER** citadelmusic;
#define NUMCAVESMUSIC 3
#define NUMCITADELMUSIC 2
//TODO: Automatically scan the music folder for a mines subdirectory and use all the music for the mines or something like that. I'd prefer something neat like for that loading music for a level, anyway. And I can just reuse the code I had for ORR.

extern OPENAL_SOUND* music_channel, *music_channel2, *music_resume; //TODO: List of music, play first one, fade out all the others? Eh, maybe some other day. //music_resume is the music to resume after, say, combat or shops. //TODO: Clear music_resume every biome change. Or otherwise validate it for that level set.
extern OPENAL_CHANNELGROUP *sound_group, *music_group;

int initOPENAL();
int closeOPENAL();

//Updates OpenAL and whatnot (dummy function)
void sound_update();

OPENAL_SOUND* playSoundPlayer(int player, Uint32 snd, int vol);
OPENAL_SOUND* playSoundPos(real_t x, real_t y, Uint32 snd, int vol);
OPENAL_SOUND* playSoundPosLocal(real_t x, real_t y, Uint32 snd, int vol);
OPENAL_SOUND* playSoundEntity(Entity* entity, Uint32 snd, int vol);
OPENAL_SOUND* playSoundEntityLocal(Entity* entity, Uint32 snd, int vol);
OPENAL_SOUND* playSound(Uint32 snd, int vol);
OPENAL_SOUND* playSoundVelocity(); //TODO: Write.

void playmusic(OPENAL_BUFFER* sound, bool loop, bool crossfade, bool resume); //Automatically crossfades. NOTE: Resets fadein and fadeout increments to the defualts every time it is called. You'll have to change the fadein and fadeout increments AFTER calling this function.

void handleLevelMusic(); //Manages and updates the level music.

int OPENAL_CreateSound(const char* name, bool b3D, OPENAL_BUFFER **buffer);
int OPENAL_CreateStreamSound(const char* name, OPENAL_BUFFER **buffer);

void OPENAL_ChannelGroup_Stop(OPENAL_CHANNELGROUP* group);
void OPENAL_ChannelGroup_SetVolume(OPENAL_CHANNELGROUP* group, float f);
void OPENAL_Channel_SetChannelGroup(OPENAL_SOUND *channel, OPENAL_CHANNELGROUP *group);
void OPENAL_Channel_SetVolume(OPENAL_SOUND *channel, float f);
void OPENAL_Channel_Stop(void* channel);
void OPENAL_Channel_Pause(OPENAL_SOUND* channel);
void OPENAL_Channel_IsPlaying(void* channel, ALboolean *playing);
OPENAL_SOUND* OPENAL_CreateChannel(OPENAL_BUFFER* buffer);
void OPENAL_Channel_Set3DAttributes(OPENAL_SOUND* channel, float x, float y, float z);
void OPENAL_Channel_Play(OPENAL_SOUND* channel);
void OPENAL_GetBuffer(OPENAL_SOUND* channel, OPENAL_BUFFER** buffer);
void OPENAL_SetLoop(OPENAL_SOUND* channel, ALboolean looping);
void OPENAL_Channel_GetPosition(OPENAL_SOUND* channel, unsigned int *position);
void OPENAL_Sound_GetLength(OPENAL_BUFFER* buffer, unsigned int *length);
void OPENAL_Sound_Release(OPENAL_BUFFER* buffer);

extern float fadein_increment, fadeout_increment, default_fadein_increment, default_fadeout_increment;
#else
void* playSound(Uint32, int);
void* playSoundPos(real_t x, real_t y, Uint32, int);
void* playSoundPosLocal(real_t, real_t, Uint32, int);
void* playSoundEntity(Entity*, Uint32, int);
void* playSoundEntityLocal(Entity*, Uint32, int);
void* playSoundPlayer(int, Uint32, int);
#endif
