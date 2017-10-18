/*-------------------------------------------------------------------------------

	BARONY
	File: sound.hpp
	Desc: Defines sound related stuff.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once


#include <stdio.h>

#include "main.hpp"

#define SOUND
#define MUSIC

struct Sound;
struct Channel;
struct ChannelGroup;

extern bool levelmusicplaying;
extern bool shopmusicplaying;
extern bool combatmusicplaying;
extern bool minotaurmusicplaying;
extern bool herxmusicplaying;
extern bool devilmusicplaying;
extern bool olddarkmap;

extern float fadein_increment, fadeout_increment, default_fadein_increment, default_fadeout_increment;

extern Sound** sounds;
extern Uint32 numsounds;
extern Sound** minesmusic;
#define NUMMINESMUSIC 4
extern Sound** swampmusic;
#define NUMSWAMPMUSIC 3
extern Sound** labyrinthmusic;
#define NUMLABYRINTHMUSIC 3
extern Sound** ruinsmusic;
#define NUMRUINSMUSIC 3
extern Sound** underworldmusic;
#define NUMUNDERWORLDMUSIC 3
extern Sound** hellmusic;
#define NUMHELLMUSIC 3
extern Sound* intromusic, *intermissionmusic, *minetownmusic, *splashmusic, *librarymusic, *shopmusic, *storymusic;
extern Sound** minotaurmusic, *herxmusic, *templemusic;
extern Sound* endgamemusic, *escapemusic, *devilmusic;
extern Sound* introductionmusic;
#define NUMMINOTAURMUSIC 2
//TODO: Automatically scan the music folder for a mines subdirectory and use all the music for the mines or something like that. I'd prefer something neat like for that loading music for a level, anyway. And I can just reuse the code I had for ORR.

extern ChannelGroup *sound_group, *music_group;

Channel* playSoundPlayer(int player, Uint32 snd, int vol);
Channel* playSoundPos(real_t x, real_t y, Uint32 snd, int vol);
Channel* playSoundPosLocal(real_t x, real_t y, Uint32 snd, int vol);
Channel* playSoundEntity(Entity* entity, Uint32 snd, int vol);
Channel* playSoundEntityLocal(Entity* entity, Uint32 snd, int vol);
Channel* playSound(Uint32 snd, int vol);
Channel* playSoundVelocity(); //TODO: Write.

void ChannelGroup_SetVolume(ChannelGroup*, float);
void ChannelGroup_Stop(ChannelGroup*);
void Channel_Stop(Channel*);
unsigned int Channel_GetPosition(Channel*);
unsigned int Sound_GetLength(Sound*);
bool Channel_IsPlaying(Channel*);
Sound* CreateMusic(const char* name);

void initSound();
void deinitSound();
void sound_update();
Sound* createSound(const char* name);
void playmusic(Sound* sound, bool loop, bool crossfade, bool resume); //Automatically crossfades. NOTE: Resets fadein and fadeout increments to the defualts every time it is called. You'll have to change the fadein and fadeout increments AFTER calling this function.
void handleLevelMusic();

extern Channel *music_channel, *music_channel2, *music_resume;
void Sound_Release(Sound*);
