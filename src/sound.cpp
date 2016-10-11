/*-------------------------------------------------------------------------------

	BARONY
	File: sound.cpp
	Desc: various sound functions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#ifdef HAVE_FMOD
#include <fmod_errors.h>
#endif

#include "main.hpp"
//#include "game.hpp"
#include "sound.hpp"

#ifdef HAVE_FMOD
FMOD_SYSTEM *fmod_system = NULL;

FMOD_RESULT fmod_result;

int fmod_maxchannels = 100;
int fmod_flags;
void* fmod_extdriverdata;

FMOD_SOUND **sounds = NULL;
Uint32 numsounds = 0;
FMOD_SOUND **minesmusic = NULL;
FMOD_SOUND **swampmusic = NULL;
FMOD_SOUND **labyrinthmusic = NULL;
FMOD_SOUND **ruinsmusic = NULL;
FMOD_SOUND **underworldmusic = NULL;
FMOD_SOUND **hellmusic = NULL;
FMOD_SOUND *intromusic = NULL;
FMOD_SOUND *intermissionmusic = NULL;
FMOD_SOUND *minetownmusic = NULL;
FMOD_SOUND *splashmusic = NULL;
FMOD_SOUND *librarymusic = NULL;
FMOD_SOUND *shopmusic = NULL;
FMOD_SOUND *storymusic = NULL;
FMOD_SOUND **minotaurmusic = NULL;
FMOD_SOUND *herxmusic = NULL;
FMOD_SOUND *templemusic = NULL;
FMOD_SOUND *endgamemusic = NULL;
FMOD_SOUND *devilmusic = NULL;
FMOD_SOUND *escapemusic = NULL;
FMOD_SOUND *introductionmusic = NULL;
bool levelmusicplaying=FALSE;

FMOD_CHANNEL *music_channel = NULL;
FMOD_CHANNEL *music_channel2 = NULL;
FMOD_CHANNEL *music_resume = NULL;

FMOD_CHANNELGROUP *sound_group = NULL;
FMOD_CHANNELGROUP *music_group = NULL;

float fadein_increment = 0.002f;
float default_fadein_increment = 0.002f;
float fadeout_increment = 0.005f;
float default_fadeout_increment = 0.005f;

bool FMODErrorCheck() {
	if (no_sound) {
		return FALSE;
	}
	if (fmod_result != FMOD_OK) {
		printlog("[FMOD Error] Error Code (%d): \"%s\"\n", fmod_result, FMOD_ErrorString(fmod_result)); //Report the FMOD error.
		return TRUE;
	}

	return FALSE;
}

void sound_update() {
	if (no_sound) {
		return;
	}
	if (!fmod_system) {
		return;
	}

	FMOD_VECTOR position, forward, up;
	position.x = -camera.y;
	position.y = -camera.z/32;
	position.z = -camera.x;

	/*double cosroll = cos(0);
	double cosyaw = cos(camera.ang);
	double cospitch = cos(camera.vang);
	double sinroll = sin(0);
	double sinyaw = sin(camera.ang);
	double sinpitch = sin(camera.vang);

	double rx = sinroll*sinyaw - cosroll*sinpitch*cosyaw;
	double ry = sinroll*cosyaw + cosroll*sinpitch*sinyaw;
	double rz = cosroll*cospitch;*/

	forward.x = 1*sin(camera.ang);
	forward.y = 0;
	forward.z = 1*cos(camera.ang);
	/*forward.x = rx;
	forward.y = ry;
	forward.z = rz;*/

	/*rx = sinroll*sinyaw - cosroll*cospitch*cosyaw;
	ry = sinroll*cosyaw + cosroll*cospitch*sinyaw;
	rz = cosroll*sinpitch;*/

	up.x = 0;
	up.y = 1;
	up.z = 0;
	/*up.x = rx;
	up.y = ry;
	up.z = rz;*/

	//FMOD_System_Set3DListenerAttributes(fmod_system, 0, &position, &velocity, &forward, &up);
	FMOD_System_Set3DListenerAttributes(fmod_system, 0, &position, 0, &forward, &up);

	//Fade in the currently playing music.
	if (music_channel) {
		FMOD_BOOL playing = FALSE;
		FMOD_Channel_IsPlaying(music_channel, &playing);
		if (playing) {
			float volume = 1.0f;
			FMOD_Channel_GetVolume(music_channel, &volume);

			if (volume < 1.0f) {
				volume += fadein_increment*2;
				if (volume > 1.0f)
					volume = 1.0f;
				FMOD_Channel_SetVolume(music_channel, volume);
			}
		}
	}
	//The following makes crossfading possible. Fade out the last playing music. //TODO: Support for saving music so that it can be resumed (for stuff interrupting like combat music).
	if (music_channel2) {
		FMOD_BOOL playing = FALSE;
		FMOD_Channel_IsPlaying(music_channel2, &playing);
		if (playing) {
			float volume = 0.0f;
			FMOD_Channel_GetVolume(music_channel2, &volume);

			if (volume > 0.0f) {
				//volume -= 0.001f;
				//volume -= 0.005f;
				volume -= fadeout_increment*2;
				if (volume < 0.0f) {
					volume = 0.0f;
				}
				FMOD_Channel_SetVolume(music_channel2, volume);
			}
		}
	}

	FMOD_System_Update(fmod_system);

	//TODO: Mute sound if focus lost.
}
#define SOUND

#endif

