/*-------------------------------------------------------------------------------

	BARONY
	File: sound.hpp
	Desc: Defines sound related stuff.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#define FMOD_AUDIO_GUID_FMT "%.8x%.16llx"

#include <stdio.h>
#ifdef USE_FMOD
#include <fmod.hpp>
#endif
#ifdef USE_OPENAL
#ifdef APPLE
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif
#endif

extern Uint32 numsounds;

//Pointer to the FMOD system.
#ifdef USE_FMOD

#define SOUND
#define MUSIC

extern FMOD::System* fmod_system;

extern FMOD_RESULT fmod_result;

extern int fmod_maxchannels;
extern int fmod_flags;
extern void* fmod_extraDriverData;
extern bool levelmusicplaying;

extern bool shopmusicplaying;
extern bool combatmusicplaying;
extern bool minotaurmusicplaying;
extern bool herxmusicplaying;
extern bool devilmusicplaying;
extern bool olddarkmap;

extern FMOD::Sound** sounds;
extern FMOD::Sound** minesmusic;
#define NUMMINESMUSIC 5
extern FMOD::Sound** swampmusic;
#define NUMSWAMPMUSIC 4
extern FMOD::Sound** labyrinthmusic;
#define NUMLABYRINTHMUSIC 3
extern FMOD::Sound** ruinsmusic;
#define NUMRUINSMUSIC 3
extern FMOD::Sound** underworldmusic;
#define NUMUNDERWORLDMUSIC 3
extern FMOD::Sound** hellmusic;
#define NUMHELLMUSIC 3
extern FMOD::Sound** intromusic, *intermissionmusic, *minetownmusic, *splashmusic, *librarymusic, *shopmusic, *storymusic;
extern FMOD::Sound** minotaurmusic, *herxmusic, *templemusic;
extern FMOD::Sound* endgamemusic, *escapemusic, *devilmusic, *sanctummusic, *tutorialmusic, *introstorymusic, *gameovermusic;
extern FMOD::Sound* introductionmusic;
#define NUMMINOTAURMUSIC 2
extern FMOD::Sound** cavesmusic;
extern FMOD::Sound** citadelmusic;
extern FMOD::Sound* gnomishminesmusic;
extern FMOD::Sound* greatcastlemusic;
extern FMOD::Sound* sokobanmusic;
extern FMOD::Sound* caveslairmusic;
extern FMOD::Sound* bramscastlemusic;
extern FMOD::Sound* hamletmusic;
#define NUMCAVESMUSIC 3
#define NUMCITADELMUSIC 3
#define NUMINTROMUSIC 3
//TODO: Automatically scan the music folder for a mines subdirectory and use all the music for the mines or something like that. I'd prefer something neat like for that loading music for a level, anyway. And I can just reuse the code I had for ORR.

extern FMOD::Channel* music_channel, *music_channel2, *music_resume; //TODO: List of music, play first one, fade out all the others? Eh, maybe some other day. //music_resume is the music to resume after, say, combat or shops. //TODO: Clear music_resume every biome change. Or otherwise validate it for that level set.

extern FMOD::ChannelGroup* sound_group, *music_group;
extern FMOD::ChannelGroup* soundAmbient_group, *soundEnvironment_group, *music_notification_group, *soundNotification_group;

/*
 * Checks for FMOD errors. Store return value of all FMOD functions in fmod_result so that this funtion can access it and check for errors.
 * Returns true on error (and prints an error message), false if everything went fine.
 */
bool FMODErrorCheck();

void sound_update(int player, int index, int numplayers);
bool initSoundEngine(); //If it fails to initialize the sound engine, it'll just disable audio.
void exitSoundEngine();
int loadSoundResources(real_t base_load_percent, real_t top_load_percent);
void freeSoundResources();

FMOD::Channel* playSoundPlayer(int player, Uint16 snd, Uint8 vol);
FMOD::Channel* playSoundNotificationPlayer(int player, Uint16 snd, Uint8 vol);
FMOD::Channel* playSoundPos(real_t x, real_t y, Uint16 snd, Uint8 vol);
FMOD::Channel* playSoundPosLocal(real_t x, real_t y, Uint16 snd, Uint8 vol);
FMOD::Channel* playSoundEntity(Entity* entity, Uint16 snd, Uint8 vol);
FMOD::Channel* playSoundEntityLocal(Entity* entity, Uint16 snd, Uint8 vol);
FMOD::Channel* playSound(Uint16 snd, Uint8 vol);
FMOD::Channel* playSoundNotification(Uint16 snd, Uint8 vol);
FMOD::Channel* playSoundVelocity();

// all parameters should be in ranges of [0.0 - 1.0]
void setAudioDevice(const std::string& device);
void setGlobalVolume(real_t master, real_t music, real_t gameplay, real_t ambient, real_t environment, real_t notification);

bool loadMusic();
void stopMusic();
void playMusic(FMOD::Sound* sound, bool loop, bool crossfade, bool resume); //Automatically crossfades. NOTE: Resets fadein and fadeout increments to the defaults every time it is called. You'll have to change the fadein and fadeout increments AFTER calling this function.

void handleLevelMusic(); //Manages and updates the level music.

extern float fadein_increment, fadeout_increment, default_fadein_increment, default_fadeout_increment, dynamicAmbientVolume, dynamicEnvironmentVolume;
extern bool sfxUseDynamicAmbientVolume, sfxUseDynamicEnvironmentVolume;

#elif defined USE_OPENAL

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
extern bool sfxUseDynamicAmbientVolume, sfxUseDynamicEnvironmentVolume;

struct OPENAL_CHANNELGROUP;

struct OPENAL_BUFFER;
struct OPENAL_SOUND;

struct FMOD_VECTOR {
	float x,y,z;
};

extern OPENAL_BUFFER** sounds;
extern OPENAL_BUFFER** minesmusic;
#define NUMMINESMUSIC 5
extern OPENAL_BUFFER** swampmusic;
#define NUMSWAMPMUSIC 4
extern OPENAL_BUFFER** labyrinthmusic;
#define NUMLABYRINTHMUSIC 3
extern OPENAL_BUFFER** ruinsmusic;
#define NUMRUINSMUSIC 3
extern OPENAL_BUFFER** underworldmusic;
#define NUMUNDERWORLDMUSIC 3
extern OPENAL_BUFFER** hellmusic;
#define NUMHELLMUSIC 3
extern OPENAL_BUFFER** intromusic, *intermissionmusic, *minetownmusic, *splashmusic, *librarymusic, *shopmusic, *storymusic;
extern OPENAL_BUFFER** minotaurmusic, *herxmusic, *templemusic;
extern OPENAL_BUFFER* endgamemusic, *escapemusic, *devilmusic, *sanctummusic, *tutorialmusic, *introstorymusic, *gameovermusic;
extern OPENAL_BUFFER* introductionmusic;
#define NUMMINOTAURMUSIC 2
extern OPENAL_BUFFER** cavesmusic;
extern OPENAL_BUFFER** citadelmusic;
extern OPENAL_BUFFER* gnomishminesmusic;
extern OPENAL_BUFFER* greatcastlemusic;
extern OPENAL_BUFFER* sokobanmusic;
extern OPENAL_BUFFER* caveslairmusic;
extern OPENAL_BUFFER* bramscastlemusic;
extern OPENAL_BUFFER* hamletmusic;
#define NUMCAVESMUSIC 3
#define NUMCITADELMUSIC 3
#define NUMINTROMUSIC 3
//TODO: Automatically scan the music folder for a mines subdirectory and use all the music for the mines or something like that. I'd prefer something neat like for that loading music for a level, anyway. And I can just reuse the code I had for ORR.

extern OPENAL_SOUND* music_channel, *music_channel2, *music_resume; //TODO: List of music, play first one, fade out all the others? Eh, maybe some other day. //music_resume is the music to resume after, say, combat or shops. //TODO: Clear music_resume every biome change. Or otherwise validate it for that level set.
extern OPENAL_CHANNELGROUP *sound_group, *music_group;
extern OPENAL_CHANNELGROUP *soundAmbient_group, *soundEnvironment_group, *music_notification_group;

int initOPENAL();
int closeOPENAL();

void sound_update(int player, int index, int numplayers);

OPENAL_SOUND* playSoundPlayer(int player, Uint16 snd, Uint8 vol);
OPENAL_SOUND* playSoundPos(real_t x, real_t y, Uint16 snd, Uint8 vol);
OPENAL_SOUND* playSoundPosLocal(real_t x, real_t y, Uint16 snd, Uint8 vol);
OPENAL_SOUND* playSoundEntity(Entity* entity, Uint16 snd, Uint8 vol);
OPENAL_SOUND* playSoundEntityLocal(Entity* entity, Uint16 snd, Uint8 vol);
OPENAL_SOUND* playSound(Uint16 snd, Uint8 vol);
OPENAL_SOUND* playSoundVelocity(); //TODO: Write.

void playmusic(OPENAL_BUFFER* sound, bool loop, bool crossfade, bool resume); //Automatically crossfades. NOTE: Resets fadein and fadeout increments to the defaults every time it is called. You'll have to change the fadein and fadeout increments AFTER calling this function.

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
