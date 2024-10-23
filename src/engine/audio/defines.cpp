/*-------------------------------------------------------------------------------

	BARONY
	File: defines.cpp
	Desc: defines extern'd sound variables and stuff. This should really all be part of a class.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../../main.hpp"
#include "sound.hpp"

Uint32 numsounds = 0;

#ifdef USE_FMOD
FMOD::System* fmod_system = nullptr;

FMOD_RESULT fmod_result;

int fmod_maxchannels = 256;
int fmod_flags;

FMOD::Sound** sounds = nullptr;
FMOD::Sound** minesmusic = nullptr;
FMOD::Sound** swampmusic = nullptr;
FMOD::Sound** labyrinthmusic = nullptr;
FMOD::Sound** ruinsmusic = nullptr;
FMOD::Sound** underworldmusic = nullptr;
FMOD::Sound** hellmusic = nullptr;
FMOD::Sound** intromusic = nullptr;
FMOD::Sound* intermissionmusic = nullptr;
FMOD::Sound* minetownmusic = nullptr;
FMOD::Sound* splashmusic = nullptr;
FMOD::Sound* librarymusic = nullptr;
FMOD::Sound* shopmusic = nullptr;
FMOD::Sound* storymusic = nullptr;
FMOD::Sound** minotaurmusic = nullptr;
FMOD::Sound* herxmusic = nullptr;
FMOD::Sound* templemusic = nullptr;
FMOD::Sound* endgamemusic = nullptr;
FMOD::Sound* devilmusic = nullptr;
FMOD::Sound* escapemusic = nullptr;
FMOD::Sound* sanctummusic = nullptr;
FMOD::Sound* introductionmusic = nullptr;
FMOD::Sound** cavesmusic = nullptr;
FMOD::Sound** citadelmusic = nullptr;
FMOD::Sound* gnomishminesmusic = nullptr;
FMOD::Sound* greatcastlemusic = nullptr;
FMOD::Sound* sokobanmusic = nullptr;
FMOD::Sound* caveslairmusic = nullptr;
FMOD::Sound* bramscastlemusic = nullptr;
FMOD::Sound* hamletmusic = nullptr;
FMOD::Sound* tutorialmusic = nullptr;
FMOD::Sound* gameovermusic = nullptr;
FMOD::Sound* introstorymusic = nullptr;
bool levelmusicplaying = false;

FMOD::Channel* music_channel = nullptr;
FMOD::Channel* music_channel2 = nullptr;
FMOD::Channel* music_resume = nullptr;

FMOD::ChannelGroup* sound_group = nullptr;
FMOD::ChannelGroup* soundAmbient_group = nullptr;
FMOD::ChannelGroup* soundEnvironment_group = nullptr;
FMOD::ChannelGroup* soundNotification_group = nullptr;
FMOD::ChannelGroup* music_group = nullptr;
FMOD::ChannelGroup* music_notification_group = nullptr;

float fadein_increment = 0.002f;
float default_fadein_increment = 0.002f;
float fadeout_increment = 0.005f;
float default_fadeout_increment = 0.005f;
float dynamicAmbientVolume = 1.f;
float dynamicEnvironmentVolume = 1.f;
bool sfxUseDynamicAmbientVolume = true;
bool sfxUseDynamicEnvironmentVolume = true;

void* fmod_extraDriverData = nullptr;
#endif //USE_FMOD
