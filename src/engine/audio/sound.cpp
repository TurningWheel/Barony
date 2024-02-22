/*-------------------------------------------------------------------------------

	BARONY
	File: sound.cpp
	Desc: various sound functions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../../main.hpp"
#include "../../files.hpp"
#include "../../game.hpp"
#include "sound.hpp"
#ifndef EDITOR
#include "../../player.hpp"
#endif

#ifdef USE_FMOD
#include "fmod_errors.h"
#elif defined USE_OPENAL
#ifdef USE_TREMOR
#include <tremor/ivorbisfile.h>
#else
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <vorbis/codec.h>
#endif
#endif

#ifdef USE_FMOD
#elif defined USE_OPENAL
#else
void setGlobalVolume(real_t master, real_t music, real_t gameplay, real_t ambient, real_t environment, real_t notification)
{
	return;
}
void setAudioDevice(const std::string& device) 
{
	return;
}
#endif

#ifdef USE_FMOD

bool FMODErrorCheck()
{
	if (no_sound)
	{
		return false;
	}
	if (fmod_result != FMOD_OK)
	{
		printlog("[FMOD Error] Error Code (%d): \"%s\"\n", fmod_result, FMOD_ErrorString(fmod_result)); //Report the FMOD error.
		return true;
	}

	return false;
}

void setAudioDevice(const std::string& device) {
	int selected_driver = 0;
	int numDrivers = 0;
	fmod_system->getNumDrivers(&numDrivers);
	for (int i = 0; i < numDrivers; ++i) {
		FMOD_GUID guid;
		fmod_result = fmod_system->getDriverInfo(i, nullptr, 0, &guid, nullptr, nullptr, nullptr);

		uint32_t _1; memcpy(&_1, &guid.Data1, sizeof(_1));
		uint64_t _2; memcpy(&_2, &guid.Data4, sizeof(_2));
		char guid_string[25];
		snprintf(guid_string, sizeof(guid_string), FMOD_AUDIO_GUID_FMT, _1, _2);
		if (!selected_driver && device == guid_string) {
			selected_driver = i;
		}
	}
	fmod_system->setDriver(selected_driver);
}

void setGlobalVolume(real_t master, real_t music, real_t gameplay, real_t ambient, real_t environment, real_t notification) {
    master = std::min(std::max(0.0, master), 1.0);
    music = std::min(std::max(0.0, music / 4.0), 1.0); // music volume cut in half because the music is loud...
    gameplay = std::min(std::max(0.0, gameplay), 1.0);
    ambient = std::min(std::max(0.0, ambient), 1.0);
    environment = std::min(std::max(0.0, environment), 1.0);
	notification = std::min(std::max(0.0, notification), 1.0);

	music_group->setVolume(master * music);
	sound_group->setVolume(master * gameplay);
	soundAmbient_group->setVolume(master * ambient);
	soundEnvironment_group->setVolume(master * environment);
	music_notification_group->setVolume(master * notification);
	soundNotification_group->setVolume(master * notification);
}

#ifndef EDITOR
	static ConsoleVariable<float> cvar_sfx_notification_music_fade("/sfx_notification_music_fade", 0.5f);
#endif // !EDITOR

void sound_update(int player, int index, int numplayers)
{
#ifdef DEBUG_EVENT_TIMERS
	auto time1 = std::chrono::high_resolution_clock::now();
	auto time2 = std::chrono::high_resolution_clock::now();
	auto accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
	if ( accum > 5 )
	{
		printlog("Large tick time: [10] %f", accum);
	}
	time1 = std::chrono::high_resolution_clock::now();
#endif

	if (no_sound)
	{
		return;
	}
	if (!fmod_system)
	{
		return;
	}

	FMOD_VECTOR position, forward, up;
	bool playing = false;

	auto& camera = cameras[index];

	position.x = (float)(camera.x);
	position.y = (float)(camera.z / (real_t)32.0);
	position.z = (float)(camera.y);

	/*forward.x = -1.0 * cos(camera.ang) * cos(camera.vang);
	forward.y =  1.0 * sin(camera.vang);
	forward.z = -1.0 * sin(camera.ang) * cos(camera.vang);*/
 
    forward.x = (float)((real_t)1.0 * cos(camera.ang));
    forward.y = 0.f;
    forward.z = (float)((real_t)1.0 * sin(camera.ang));

	/*up.x = -1.0 * cos(camera.ang) * sin(camera.vang);
	up.y =  1.0 * cos(camera.vang);
	up.z = -1.0 * sin(camera.ang) * sin(camera.vang);*/
    up.x = 0.f;
    up.y = 1.f;
    up.z = 0.f;

	//FMOD_System_Set3DListenerAttributes(fmod_system, 0, &position, &velocity, &forward, &up);
	fmod_system->set3DNumListeners(numplayers);

#ifdef DEBUG_EVENT_TIMERS
	time2 = std::chrono::high_resolution_clock::now();
	accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
	if ( accum > 5 )
	{
		printlog("Large tick time: [11] %f", accum);
	}
	time1 = std::chrono::high_resolution_clock::now();
#endif

	fmod_system->set3DListenerAttributes(player, &position, nullptr, &forward, &up);

#ifdef DEBUG_EVENT_TIMERS
	time2 = std::chrono::high_resolution_clock::now();
	accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
	if ( accum > 5 )
	{
		printlog("Large tick time: [12] %f", accum);
	}
	time1 = std::chrono::high_resolution_clock::now();
#endif

	if (player == 0) {
		//Fade in the currently playing music.
		bool notificationPlaying = false;
		if ( music_notification_group )
		{
			music_notification_group->isPlaying(&notificationPlaying);
		}

#ifdef DEBUG_EVENT_TIMERS
		time2 = std::chrono::high_resolution_clock::now();
		accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
		if ( accum > 5 )
		{
			printlog("Large tick time: [13] %f", accum);
		}
		time1 = std::chrono::high_resolution_clock::now();
#endif

		if (music_channel)
		{
			playing = false;
			music_channel->isPlaying(&playing);
			if (playing)
			{
				float volume = 1.0f;
				music_channel->getVolume(&volume);

#ifdef DEBUG_EVENT_TIMERS
				time2 = std::chrono::high_resolution_clock::now();
				accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
				if ( accum > 5 )
				{
					printlog("Large tick time: [14] %f", accum);
				}
				time1 = std::chrono::high_resolution_clock::now();
#endif
#ifdef EDITOR
				if ( volume < 1.0f )
				{
					volume += fadein_increment * 2;
					if ( volume > 1.0f )
					{
						volume = 1.0f;
					}
					music_channel->setVolume(volume);
				}
#else
				if ( notificationPlaying && volume > 0.0f )
				{
					volume -= fadeout_increment * 5;
					if ( volume < *cvar_sfx_notification_music_fade )
					{
						volume = *cvar_sfx_notification_music_fade;
					}
					music_channel->setVolume(volume);
				}
				else if (volume < 1.0f)
				{
					volume += fadein_increment * 2;
					if (volume > 1.0f)
					{
						volume = 1.0f;
					}
					music_channel->setVolume(volume);
				}
#endif
#ifdef DEBUG_EVENT_TIMERS
				time2 = std::chrono::high_resolution_clock::now();
				accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
				if ( accum > 5 )
				{
					printlog("Large tick time: [15] %f", accum);
				}
				time1 = std::chrono::high_resolution_clock::now();
#endif
			}
		}

		//The following makes crossfading possible. Fade out the last playing music. //TODO: Support for saving music so that it can be resumed (for stuff interrupting like combat music).
		if (music_channel2)
		{
			playing = false;

#ifdef DEBUG_EVENT_TIMERS
			time2 = std::chrono::high_resolution_clock::now();
			accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
			if ( accum > 5 )
			{
				printlog("Large tick time: [16] %f", accum);
			}
			time1 = std::chrono::high_resolution_clock::now();
#endif

			music_channel2->isPlaying(&playing);
			if (playing)
			{
				float volume = 0.0f;
				music_channel2->getVolume(&volume);

#ifdef DEBUG_EVENT_TIMERS
				time2 = std::chrono::high_resolution_clock::now();
				accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
				if ( accum > 5 )
				{
					printlog("Large tick time: [17] %f", accum);
				}
				time1 = std::chrono::high_resolution_clock::now();
#endif

				if (volume > 0.0f)
				{
					volume -= fadeout_increment * 2;
					if (volume < 0.0f)
					{
						volume = 0.0f;
					}
					music_channel2->setVolume(volume);
				}

#ifdef DEBUG_EVENT_TIMERS
				time2 = std::chrono::high_resolution_clock::now();
				accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
				if ( accum > 5 )
				{
					printlog("Large tick time: [18] %f", accum);
				}
				time1 = std::chrono::high_resolution_clock::now();
#endif
			}
		}
	}

#ifdef DEBUG_EVENT_TIMERS
	time2 = std::chrono::high_resolution_clock::now();
	accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
	if ( accum > 5 )
	{
		printlog("Large tick time: [19] %f", accum);
	}
	time1 = std::chrono::high_resolution_clock::now();
#endif

	if (player == numplayers - 1) {
		fmod_system->update();
	}

#ifdef DEBUG_EVENT_TIMERS
	time2 = std::chrono::high_resolution_clock::now();
	accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
	if ( accum > 5 )
	{
		printlog("Large tick time: [20] %f", accum);
	}
	time1 = std::chrono::high_resolution_clock::now();
#endif
}

#elif defined USE_OPENAL

struct OPENAL_BUFFER {
	ALuint id;
	bool stream;
	char oggfile[64];
};
struct OPENAL_SOUND {
	ALuint id;
	OPENAL_CHANNELGROUP *group;
	float volume;
	OPENAL_BUFFER *buffer;
	bool active;
	char* oggdata;
	int oggdata_length;
	int ogg_seekoffset;
	OggVorbis_File oggStream;
	vorbis_info* vorbisInfo;
	vorbis_comment* vorbisComment;
	ALuint streambuff[4];
	bool loop;
	bool stream_active;
	int indice;
};

struct OPENAL_CHANNELGROUP {
	float volume;
	int num;
	int cap;
	OPENAL_SOUND **sounds;
};

SDL_mutex *openal_mutex;

static size_t openal_oggread(void* ptr, size_t size, size_t nmemb, void* datasource) {
	OPENAL_SOUND* self = (OPENAL_SOUND*)datasource;

	int bytes = size*nmemb;
	int remain = self->oggdata_length - self->ogg_seekoffset - bytes;
	if(remain < 0) bytes += remain;

	memcpy(ptr, self->oggdata + self->ogg_seekoffset, bytes);
	self->ogg_seekoffset += bytes;

	return bytes;
}

static int openal_oggseek(void* datasource, ogg_int64_t offset, int whence) {
	OPENAL_SOUND* self = (OPENAL_SOUND*)datasource;
	int seek_offset;

	switch(whence) {
	case SEEK_CUR:
		seek_offset = self->ogg_seekoffset + offset;
		break;
	case SEEK_END:
		seek_offset = self->oggdata_length + offset;
		break;
	case SEEK_SET:
		seek_offset = offset;
		break;
	/*default:
		exit(1);*/
	}
	if(seek_offset > self->oggdata_length) return -1;

	self->ogg_seekoffset = seek_offset;
	return 0;
}

static int openal_oggclose(void* datasource) {
	return 0;
}

static long int openal_oggtell(void* datasource) {
	OPENAL_SOUND* self = (OPENAL_SOUND*)datasource;
	return self->ogg_seekoffset;
}

static int openal_oggopen(OPENAL_SOUND *self, const char* oggfile) {
	File *f = openDataFile(oggfile, "rb");
	int err;

	ov_callbacks oggcb = {openal_oggread, openal_oggseek, openal_oggclose, openal_oggtell};

	if(!f) {
		return 0;
	}

	self->ogg_seekoffset = 0;
	self->oggdata_length = f->size();

	self->oggdata = (char*)malloc(self->oggdata_length);
	f->read(self->oggdata, sizeof(char), self->oggdata_length);
	FileIO::close(f);

	if(ov_open_callbacks(self, &self->oggStream, 0, 0, oggcb)) {
		printf("Issues with OGG callbacks\n");
		return 0;
	}

	self->vorbisInfo = ov_info(&self->oggStream, -1);
	self->vorbisComment = ov_comment(&self->oggStream, -1);

	alGenBuffers(4, self->streambuff);
	return 1;
}

static int openal_oggrelease(OPENAL_SOUND *self) {
	alSourceStop(self->id);
	ov_raw_seek(&self->oggStream, 0);
	int queued;
	alGetSourcei(self->id, AL_BUFFERS_QUEUED, &queued);
	while(queued--) {
		ALuint buffer;
		alSourceUnqueueBuffers(self->id, 1, &buffer);
	}
	alDeleteBuffers(4, self->streambuff);
	ov_clear(&self->oggStream);
	free(self->oggdata);
	return 1;
}

static int openal_streamread(OPENAL_SOUND *self, ALuint buffer) {
	#define OGGSIZE 65536
	char pcm[OGGSIZE];
	int size = 0;
	int section;
	int result;


	while (size < OGGSIZE) {
		#ifdef USE_TREMOR
		result = ov_read(&self->oggStream, pcm+size, OGGSIZE -size, &section);
		#else
		result = ov_read(&self->oggStream, pcm+size, OGGSIZE -size, 0, 2, 1, &section);
		#endif
		if(result==0 && self->loop)
			ov_raw_seek(&self->oggStream, 0);

		if(result>0)
			size += result;
		else
			break;
	}

	if(size==0) {
		return 0;
	}
	alBufferData(buffer, 
		(self->vorbisInfo->channels==1)?AL_FORMAT_MONO16:AL_FORMAT_STEREO16, 
		pcm, size, self->vorbisInfo->rate);

	return 1;

	#undef OGGSIZE
}

static int openal_streamupdate(OPENAL_SOUND* self) {
	int processed;
	int active = 1;

	alGetSourcei(self->id, AL_BUFFERS_PROCESSED, &processed);

	while(processed--) {
		ALuint buffer;

		alSourceUnqueueBuffers(self->id, 1, &buffer);

		active = openal_streamread(self, buffer);
		if(active)
			alSourceQueueBuffers(self->id, 1, &buffer);
	}
	self->stream_active = active;

	return active;
}

bool sfxUseDynamicAmbientVolume = true;
bool sfxUseDynamicEnvironmentVolume = true;

ALCcontext *openal_context = nullptr;
ALCdevice  *openal_device = nullptr;

//#define openal_maxchannels 100

OPENAL_BUFFER** sounds = nullptr;
Uint32 numsounds = 0;
OPENAL_BUFFER** minesmusic = NULL;
OPENAL_BUFFER** swampmusic = NULL;
OPENAL_BUFFER** labyrinthmusic = NULL;
OPENAL_BUFFER** ruinsmusic = NULL;
OPENAL_BUFFER** underworldmusic = NULL;
OPENAL_BUFFER** hellmusic = NULL;
OPENAL_BUFFER** intromusic = NULL;
OPENAL_BUFFER* intermissionmusic = NULL;
OPENAL_BUFFER* minetownmusic = NULL;
OPENAL_BUFFER* splashmusic = NULL;
OPENAL_BUFFER* librarymusic = NULL;
OPENAL_BUFFER* shopmusic = NULL;
OPENAL_BUFFER* storymusic = NULL;
OPENAL_BUFFER** minotaurmusic = NULL;
OPENAL_BUFFER* herxmusic = NULL;
OPENAL_BUFFER* templemusic = NULL;
OPENAL_BUFFER* endgamemusic = NULL;
OPENAL_BUFFER* devilmusic = NULL;
OPENAL_BUFFER* escapemusic = NULL;
OPENAL_BUFFER* sanctummusic = NULL;
OPENAL_BUFFER* introductionmusic = NULL;
OPENAL_BUFFER** cavesmusic = NULL;
OPENAL_BUFFER** citadelmusic = NULL;
OPENAL_BUFFER* gnomishminesmusic = NULL;
OPENAL_BUFFER* greatcastlemusic = NULL;
OPENAL_BUFFER* sokobanmusic = NULL;
OPENAL_BUFFER* caveslairmusic = NULL;
OPENAL_BUFFER* bramscastlemusic = NULL;
OPENAL_BUFFER* hamletmusic = NULL;
OPENAL_BUFFER* tutorialmusic = nullptr;
OPENAL_BUFFER* gameovermusic = nullptr;
OPENAL_BUFFER* introstorymusic = nullptr;
bool levelmusicplaying = false;

OPENAL_SOUND* music_channel = nullptr;
OPENAL_SOUND* music_channel2 = nullptr;
OPENAL_SOUND* music_resume = nullptr;

OPENAL_CHANNELGROUP *sound_group = NULL;
OPENAL_CHANNELGROUP *soundAmbient_group = NULL;
OPENAL_CHANNELGROUP *soundEnvironment_group = NULL;
OPENAL_CHANNELGROUP *music_group = NULL;
OPENAL_CHANNELGROUP *music_notification_group = NULL;

float fadein_increment = 0.002f;
float default_fadein_increment = 0.002f;
float fadeout_increment = 0.005f;
float default_fadeout_increment = 0.005f;

#define MAXSOUND 1024
OPENAL_SOUND openal_sounds[MAXSOUND];
int lower_freechannel = 0;
int upper_unfreechannel = 0;

SDL_Thread* openal_soundthread;
bool OpenALSoundON = true;

void OPENAL_RemoveChannelGroup(OPENAL_SOUND *channel, OPENAL_CHANNELGROUP *group);

static void private_OPENAL_Channel_Stop(OPENAL_SOUND* channel) {
	// stop and delete Sound (channel)
	channel->stream_active = false;
	alSourceStop(channel->id);
	if(channel->group)
		OPENAL_RemoveChannelGroup(channel, channel->group);
	if(channel->buffer->stream)
		openal_oggrelease(channel);
	alDeleteSources( 1, &channel->id );
	//free(channel);
	channel->active = false;
}


int OPENAL_ThreadFunction(void* data) {
	(void)data;
	while(OpenALSoundON) {
		SDL_LockMutex(openal_mutex);

		// Updates Stream channel
		for (int i=0; i<upper_unfreechannel; i++) {
			if(openal_sounds[i].active && openal_sounds[i].buffer->stream && openal_sounds[i].stream_active) {
				openal_streamupdate(&openal_sounds[i]);
			}
		}

		// check finished sound to free them, unless it's a streamed channel...
		for (int i=0; i<upper_unfreechannel; i++) {
			if(openal_sounds[i].active && !openal_sounds[i].buffer->stream) {
				ALint state = 0;
				alGetSourcei(openal_sounds[i].id, AL_SOURCE_STATE, &state);
				if(!(state==AL_PLAYING || state==AL_PAUSED || state==AL_INITIAL)) {
					private_OPENAL_Channel_Stop(&openal_sounds[i]);
					if (lower_freechannel > i)
						lower_freechannel = i;
				}
			}
		}
		while ((upper_unfreechannel > 0) && (!openal_sounds[upper_unfreechannel-1].active))
			--upper_unfreechannel;

		SDL_UnlockMutex(openal_mutex);
		
		SDL_Delay(100);
	}
	return 1;
}

int initOPENAL()
{
	static int initialized = 0;
	if(initialized)
		return 1;

	openal_device = alcOpenDevice(NULL); // preferred device
	if(!openal_device)
		return 0;

	openal_context = alcCreateContext(openal_device,NULL);
	if(!openal_context)
		return 0;

	alcMakeContextCurrent(openal_context);

	alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
	alDopplerFactor(2.0f);

	// creates channels groups
	sound_group = (OPENAL_CHANNELGROUP*)malloc(sizeof(OPENAL_CHANNELGROUP));
	soundAmbient_group = (OPENAL_CHANNELGROUP*)malloc(sizeof(OPENAL_CHANNELGROUP));
	soundEnvironment_group = (OPENAL_CHANNELGROUP*)malloc(sizeof(OPENAL_CHANNELGROUP));
	music_group = (OPENAL_CHANNELGROUP*)malloc(sizeof(OPENAL_CHANNELGROUP));
	music_notification_group = (OPENAL_CHANNELGROUP*)malloc(sizeof(OPENAL_CHANNELGROUP));
	memset(sound_group, 0, sizeof(OPENAL_CHANNELGROUP));
	memset(soundAmbient_group, 0, sizeof(OPENAL_CHANNELGROUP));
	memset(soundEnvironment_group, 0, sizeof(OPENAL_CHANNELGROUP));
	memset(music_group, 0, sizeof(OPENAL_CHANNELGROUP));
	memset(music_notification_group, 0, sizeof(OPENAL_CHANNELGROUP));
	sound_group->volume = 1.0f;
	soundAmbient_group->volume = 1.0f;
	soundEnvironment_group->volume = 1.0f;
	music_group->volume = 1.0f;
	music_notification_group->volume = 1.0f;

	memset(openal_sounds, 0, sizeof(openal_sounds));
	lower_freechannel = 0;
	upper_unfreechannel = 0;

	OpenALSoundON = true;
	openal_mutex = SDL_CreateMutex();
	openal_soundthread = SDL_CreateThread(OPENAL_ThreadFunction, "openal", NULL);

	initialized = 1;

#ifdef NINTENDO
	//TODO: Do we also want this on other platforms?
	// print source limit
	ALCint size = -1;
	alcGetIntegerv(openal_device, ALC_MONO_SOURCES, 1, &size);
	printlog("openAL: max mono sources: %d", size);
	size = -1;
	alcGetIntegerv(openal_device, ALC_STEREO_SOURCES, 1, &size);
	printlog("openAL: max stereo sources: %d", size);
#endif // NINTENDO

	return 1;
}

int closeOPENAL()
{
	if(OpenALSoundON) return 0;

	OpenALSoundON = false;
	int i = 0;
	SDL_WaitThread(openal_soundthread, &i);
	if(i!=1) {
		printlog("Warning, unable to stop Openal thread\n");
	}

	if(openal_mutex) {
		SDL_DestroyMutex(openal_mutex);
		openal_mutex = NULL;
	}

	// stop all remaining sound
	for (int i=0; i<upper_unfreechannel; i++) {
		if(openal_sounds[i].active && !openal_sounds[i].buffer->stream) {
			private_OPENAL_Channel_Stop(&openal_sounds[i]);
		}
	}

	alcMakeContextCurrent(NULL);
	alcDestroyContext(openal_context);
	openal_context = NULL;
	alcCloseDevice(openal_device);
	openal_device = NULL;
	initialized = 0;

	return 1;
}


static int get_firstfreechannel()
{
	int i = lower_freechannel;
	while ((i<MAXSOUND) && (openal_sounds[i].active))
		i++;
	if (i<MAXSOUND) {
		return i;
	}
	//no free channels, force free last one :(
	i = MAXSOUND-1;
	// TODO, check if it's a Stream one, then skip it if yes
	while((i>0) && (!openal_sounds[i].buffer->stream))
		--i;

	private_OPENAL_Channel_Stop(&openal_sounds[i]);

	return i;
}

void setGlobalVolume(real_t master, real_t music, real_t gameplay, real_t ambient, real_t environment) {
    master = std::min(std::max(0.0, master), 1.0);
    music = std::min(std::max(0.0, music / 4.0), 1.0); // music volume cut in half because the music is loud...
    gameplay = std::min(std::max(0.0, gameplay), 1.0);
    ambient = std::min(std::max(0.0, ambient), 1.0);
    environment = std::min(std::max(0.0, environment), 1.0);

	OPENAL_ChannelGroup_SetVolume(music_group, master * music);
	OPENAL_ChannelGroup_SetVolume(sound_group, master * gameplay);
	OPENAL_ChannelGroup_SetVolume(soundAmbient_group, master * ambient);
	OPENAL_ChannelGroup_SetVolume(soundEnvironment_group, master * environment);
	OPENAL_ChannelGroup_SetVolume(music_notification_group, master * gameplay);
}

void sound_update(int player, int index, int numplayers)
{
	if (no_sound)
	{
		return;
	}
	if (!openal_device)
	{
		return;
	}

	FMOD_VECTOR position;

	auto& camera = cameras[index];
	if ( splitscreen )
	{
		camera = cameras[0];
	}

	position.x = -camera.y;
	position.y = -camera.z / 32;
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

	float vector[6];
	vector[0] = 1 * sin(camera.ang);
	vector[1] = 0;
	vector[2] = 1 * cos(camera.ang);
	/*forward.x = rx;
	forward.y = ry;
	forward.z = rz;*/

	/*rx = sinroll*sinyaw - cosroll*cospitch*cosyaw;
	ry = sinroll*cosyaw + cosroll*cospitch*sinyaw;
	rz = cosroll*sinpitch;*/

	vector[3] = 0;
	vector[4] = 1;
	vector[5] = 0;
	/*up.x = rx;
	up.y = ry;
	up.z = rz;*/

	alListenerfv(AL_POSITION, (float*)&position);
	alListenerfv(AL_ORIENTATION, vector);
	//FMOD_System_Set3DListenerAttributes(fmod_system, 0, &position, 0, &forward, &up);

	//Fade in the currently playing music.
	if (player == 0) {
		if (music_channel)
		{
			ALint playing = 0;
			alGetSourcei( music_channel->id, AL_SOURCE_STATE, &playing );
			if (playing==AL_PLAYING)
			{
				float volume = music_channel->volume;

				if (volume < 1.0f)
				{
					volume += fadein_increment * 2;
					if (volume > 1.0f)
					{
						volume = 1.0f;
					}
					OPENAL_Channel_SetVolume(music_channel, volume);
				}
			}
		}
		//The following makes crossfading possible. Fade out the last playing music. //TODO: Support for saving music so that it can be resumed (for stuff interrupting like combat music).
		if (music_channel2)
		{
			ALint playing = 0;
			alGetSourcei( music_channel2->id, AL_SOURCE_STATE, &playing );
			if (playing)
			{
				float volume = music_channel2->volume;

				if (volume > 0.0f)
				{
					//volume -= 0.001f;
					//volume -= 0.005f;
					volume -= fadeout_increment * 2;
					if (volume < 0.0f)
					{
						volume = 0.0f;
					}
					OPENAL_Channel_SetVolume(music_channel2, volume);
				} else {
					/*OPENAL_Channel_Stop(music_channel2);
					music_channel2 = NULL;*/
					OPENAL_Channel_Pause(music_channel2);
				}
			}
		}
	}
}

void OPENAL_Channel_SetVolume(OPENAL_SOUND *channel, float f) {
	channel->volume = f;
	if(channel->group)
		f *= channel->group->volume;
	alSourcef(channel->id, AL_GAIN, f);
}

void OPENAL_ChannelGroup_Stop(OPENAL_CHANNELGROUP* group) {
	for (int i = 0; i< group->num; i++) {
		if (group->sounds[i])
			alSourceStop( group->sounds[i]->id );
	}
}

void OPENAL_ChannelGroup_SetVolume(OPENAL_CHANNELGROUP* group, float f) {
	group->volume = f;
	for (int i = 0; i< group->num; i++) {
		if (group->sounds[i])
			alSourcef( group->sounds[i]->id, AL_GAIN, f*group->sounds[i]->volume );
	}
}

void OPENAL_Channel_SetChannelGroup(OPENAL_SOUND *channel, OPENAL_CHANNELGROUP *group) {
	if(group->num==group->cap) {
		group->cap += 8;
		group->sounds = (OPENAL_SOUND**)realloc(group->sounds, group->cap*sizeof(OPENAL_SOUND*));
	}
	alSourcef(channel->id, AL_GAIN, channel->volume * group->volume);
	group->sounds[group->num++] = channel;
	channel->group = group;
}

void OPENAL_RemoveChannelGroup(OPENAL_SOUND *channel, OPENAL_CHANNELGROUP *group) {
	int i = 0;
	while ((i<group->num) && (channel!=group->sounds[i]))
		i++;
	if(i==group->num)
		return;
	memmove(group->sounds+i, group->sounds+i+1, sizeof(OPENAL_SOUND*)*(group->num-(i+1)));
	group->num--;
}

static size_t openal_file_oggread(void* ptr, size_t size, size_t nmemb, void* datasource) {
	File* file = (File*)datasource;
	return file->read(ptr, size, nmemb);
}

static int openal_file_oggseek(void* datasource, ogg_int64_t offset, int whence) {
	File* file = (File*)datasource;
	switch (whence) {
	case SEEK_CUR:
		return file->seek((ptrdiff_t)offset, File::SeekMode::ADD);
	case SEEK_END:
		return file->seek((ptrdiff_t)offset, File::SeekMode::SETEND);
	case SEEK_SET:
		return file->seek((ptrdiff_t)offset, File::SeekMode::SET);
	}
	return 0;
}

static int openal_file_oggclose(void* datasource) {
	return 0;
}

static long int openal_file_oggtell(void* datasource) {
	File* file = (File*)datasource;
	return file->tell();
}

int OPENAL_CreateSound(const char* name, bool b3D, OPENAL_BUFFER **buffer) {
	*buffer = (OPENAL_BUFFER*)malloc(sizeof(OPENAL_BUFFER));
	strncpy((*buffer)->oggfile, name, 64);	// for debugging purpose
	(*buffer)->stream = false;
	File *f = openDataFile(name, "rb");
	if(!f) {
		printlog("Error loading sound %s\n", name);
		return 0;
	}

	ov_callbacks oggcb = { openal_file_oggread, openal_file_oggseek, openal_file_oggclose, openal_file_oggtell };

	vorbis_info * pInfo;
	OggVorbis_File oggFile;
	ov_open_callbacks(f, &oggFile, NULL, 0, oggcb);
	pInfo = ov_info(&oggFile, -1);

	int channels = pInfo->channels;
	int freq = pInfo->rate;
	ov_pcm_seek(&oggFile, 0);
	size_t size = ov_pcm_total(&oggFile, -1) * 2 * (pInfo->channels+1);
	char* data = (char*)malloc(size+size/2);	// safe side
	char* ptr = data;
	int bytes = 0;
	size_t sz = 0;
	do {
		int bitStream;
		#ifdef USE_TREMOR
		bytes = ov_read(&oggFile, ptr, size, &bitStream);
		#else
		bytes = ov_read(&oggFile, ptr, size, 0, 2, 1, &bitStream);
		#endif
		size-=bytes;
		ptr+=bytes;
		sz+=bytes;
	} while(bytes>0);
	char *data2 = data;
	if(b3D && channels==2) {
		// downmixing sound to mono, because 3D sounds NEEDS mono sound
		data2 = (char*)malloc(sz/2);
		int16_t *p1, *p2;
		p1 = (int16_t*)data2;
		p2 = (int16_t*)data;
		sz/=2;
		for(int i=0; i<sz/2; i++) {
			*(p1++) = (p2[0]+p2[1])/2;
			p2+=2;
		}
		channels = 1;
	}

	ov_clear(&oggFile);
	alGenBuffers(1, &(*buffer)->id);
	alBufferData((*buffer)->id, (channels==1)?AL_FORMAT_MONO16:AL_FORMAT_STEREO16, data2, sz, freq);
	if(data2!=data)
		free(data2);
	free(data);
	FileIO::close(f);
	return 1;
}

int OPENAL_CreateStreamSound(const char* name, OPENAL_BUFFER **buffer) {
	*buffer = (OPENAL_BUFFER*)malloc(sizeof(OPENAL_BUFFER));
	(*buffer)->stream = true;
	strcpy((*buffer)->oggfile, name);
	return 1;
}

OPENAL_SOUND* OPENAL_CreateChannel(OPENAL_BUFFER* buffer) {
	//OPENAL_SOUND *channel=(OPENAL_SOUND*)malloc(sizeof(OPENAL_SOUND));

	SDL_LockMutex(openal_mutex);

	int i = get_firstfreechannel();

	if(upper_unfreechannel < (i+1))
		upper_unfreechannel = i+1;
	lower_freechannel = i+1;

	OPENAL_SOUND *channel = &openal_sounds[i];
	alGenSources(1,&channel->id);
	channel->volume = 1.0f;
	channel->group = NULL;
	channel->active = true;
	channel->loop = false;
	channel->buffer = buffer;
	channel->stream_active = false;
	channel->indice = i;

	if(buffer->stream) {
		openal_oggopen(channel, buffer->oggfile);
	} else
		alSourcei(channel->id, AL_BUFFER, buffer->id);
	// default to 2D...
	alSourcei(channel->id,AL_SOURCE_RELATIVE, AL_TRUE);
	alSource3f(channel->id, AL_POSITION, 0, 0, 0);

	SDL_UnlockMutex(openal_mutex);
	return channel;
}

void OPENAL_Channel_IsPlaying(void* channel, ALboolean *playing) {
	ALint state;
	alGetSourcei( ((OPENAL_SOUND*)channel)->id, AL_SOURCE_STATE, &state );
	(*playing) = (state == AL_PLAYING);
}

void OPENAL_Channel_Stop(void* chan) {
	SDL_LockMutex(openal_mutex);

	OPENAL_SOUND* channel = (OPENAL_SOUND*)chan;
	if(channel==NULL || !channel->active) {
		SDL_UnlockMutex(openal_mutex);
		return;
	}

	int i = channel->indice;
	private_OPENAL_Channel_Stop(channel);
	if (lower_freechannel > i)
		lower_freechannel = i;


	SDL_UnlockMutex(openal_mutex);
}

void OPENAL_Channel_Set3DAttributes(OPENAL_SOUND* channel, float x, float y, float z) {

	alSourcei(channel->id,AL_SOURCE_RELATIVE, AL_FALSE);
	alSource3f(channel->id, AL_POSITION, x, y, z);
	alSourcef(channel->id, AL_REFERENCE_DISTANCE, 1.f);	// hardcoding FMOD_System_Set3DSettings(fmod_system, 1.0, 2.0, 1.0);
	alSourcef(channel->id, AL_MAX_DISTANCE, 10.f);		// but this are simply OpenAL default (the 2.0f is used for Dopler only)
}

void OPENAL_Channel_Play(OPENAL_SOUND* channel) {
	SDL_LockMutex(openal_mutex);

	ALint state;
	alGetSourcei( channel->id, AL_SOURCE_STATE, &state );
	if(state != AL_PLAYING && state != AL_PAUSED) {
		if(channel->buffer->stream) {
			int processed;
			int num_buffers = 4;
			int i;
			ALuint trash[256];

			alGetSourcei(channel->id, AL_BUFFERS_PROCESSED, &processed);
			alSourceUnqueueBuffers(channel->id, processed, trash);

			for(i=0; i<4; i++) {
				if(!openal_streamread(channel, channel->streambuff[i])) {
					num_buffers = i;
					break;
				}
			}

			alSourceQueueBuffers(channel->id, num_buffers, channel->streambuff);
			channel->stream_active = true;
		}
	}
	alSourcePlay(channel->id);

	SDL_UnlockMutex(openal_mutex);
}

void OPENAL_Channel_Pause(OPENAL_SOUND* channel) {
	alSourcePause(channel->id);
}

void OPENAL_GetBuffer(OPENAL_SOUND* channel, OPENAL_BUFFER** buffer) {
	(*buffer) = channel->buffer;
}

void OPENAL_SetLoop(OPENAL_SOUND* channel, ALboolean looping) {
	channel->loop = looping;
	if(!channel->buffer->stream)
		alSourcei(channel->id, AL_LOOPING, looping);
}

void OPENAL_Channel_GetPosition(OPENAL_SOUND* channel, unsigned int *position) {
	alGetSourcei(channel->id, AL_BYTE_OFFSET, (GLint*)position);
}

void OPENAL_Sound_GetLength(OPENAL_BUFFER* buffer, unsigned int *length) {
	if(!buffer) return;
	alGetBufferi(buffer->id, AL_SIZE, (GLint*)length);
}

void OPENAL_Sound_Release(OPENAL_BUFFER* buffer) {
	if(!buffer) return;
	if(!buffer->stream)
		alDeleteBuffers( 1, &buffer->id );
	free(buffer);
}

#endif

bool physfsSearchMusicToUpdate_helper_findModifiedMusic(uint32_t numMusic, const char* filenameTemplate)
{
	for ( int c = 0; c < numMusic; c++ )
	{
		snprintf(tempstr, 1000, filenameTemplate, c);
		if ( PHYSFS_getRealDir(tempstr) != nullptr )
		{
			std::string musicDir = PHYSFS_getRealDir(tempstr);
			if ( musicDir.compare("./") != 0 )
			{
				printlog("[PhysFS]: Found modified music in music/ directory, reloading music files...");
				return true;
			}
		}
	}

	return false;
}

bool physfsSearchMusicToUpdate()
{
	if ( no_sound )
	{
		return false;
	}
#ifdef SOUND
	std::vector<std::string> themeMusic;
	themeMusic.push_back("music/introduction.ogg");
	themeMusic.push_back("music/intermission.ogg");
	themeMusic.push_back("music/minetown.ogg");
	themeMusic.push_back("music/splash.ogg");
	themeMusic.push_back("music/library.ogg");
	themeMusic.push_back("music/shop.ogg");
	themeMusic.push_back("music/herxboss.ogg");
	themeMusic.push_back("music/temple.ogg");
	themeMusic.push_back("music/endgame.ogg");
	themeMusic.push_back("music/escape.ogg");
	themeMusic.push_back("music/devil.ogg");
	themeMusic.push_back("music/sanctum.ogg");
	themeMusic.push_back("music/gnomishmines.ogg");
	themeMusic.push_back("music/greatcastle.ogg");
	themeMusic.push_back("music/sokoban.ogg");
	themeMusic.push_back("music/caveslair.ogg");
	themeMusic.push_back("music/bramscastle.ogg");
	themeMusic.push_back("music/hamlet.ogg");
	themeMusic.push_back("music/tutorial.ogg");
	themeMusic.push_back("sound/Death.ogg");
	themeMusic.push_back("sound/ui/StoryMusicV3.ogg");

	for ( std::vector<std::string>::iterator it = themeMusic.begin(); it != themeMusic.end(); ++it )
	{
		std::string filename = *it;
		if ( PHYSFS_getRealDir(filename.c_str()) != nullptr )
		{
			std::string musicDir = PHYSFS_getRealDir(filename.c_str());
			if ( musicDir.compare("./") != 0 )
			{
				printlog("[PhysFS]: Found modified music in music/ directory, reloading music files...");
				return true;
			}
		}
	}

	int c;

	if ( physfsSearchMusicToUpdate_helper_findModifiedMusic(NUMMINESMUSIC, "music/mines%02d.ogg")
		|| physfsSearchMusicToUpdate_helper_findModifiedMusic(NUMSWAMPMUSIC, "music/swamp%02d.ogg")
		|| physfsSearchMusicToUpdate_helper_findModifiedMusic(NUMLABYRINTHMUSIC, "music/labyrinth%02d.ogg")
		|| physfsSearchMusicToUpdate_helper_findModifiedMusic(NUMRUINSMUSIC, "music/ruins%02d.ogg")
		|| physfsSearchMusicToUpdate_helper_findModifiedMusic(NUMUNDERWORLDMUSIC, "music/underworld%02d.ogg")
		|| physfsSearchMusicToUpdate_helper_findModifiedMusic(NUMHELLMUSIC, "music/hell%02d.ogg")
		|| physfsSearchMusicToUpdate_helper_findModifiedMusic(NUMMINOTAURMUSIC, "music/minotaur%02d.ogg")
		|| physfsSearchMusicToUpdate_helper_findModifiedMusic(NUMCAVESMUSIC, "music/caves%02d.ogg")
		|| physfsSearchMusicToUpdate_helper_findModifiedMusic(NUMCITADELMUSIC, "music/citadel%02d.ogg") )
	{
		return true;
	}

	for ( c = 0; c < NUMINTROMUSIC; c++ )
	{
		if ( c == 0 )
		{
			strcpy(tempstr, "music/intro.ogg");
		}
		else
		{
			snprintf(tempstr, 1000, "music/intro%02d.ogg", c);
		}
		if ( PHYSFS_getRealDir(tempstr) != nullptr )
		{
			std::string musicDir = PHYSFS_getRealDir(tempstr);
			if ( musicDir.compare("./") != 0 )
			{
				printlog("[PhysFS]: Found modified music in music/ directory, reloading music files...");
				return true;
			}
		}
	}
#endif // SOUND
	return false;
}

#ifdef USE_FMOD
FMOD_RESULT physfsReloadMusic_helper_reloadMusicArray(uint32_t numMusic, const char* filenameTemplate, FMOD::Sound** musicArray, bool reloadAll)
{
	for ( int c = 0; c < numMusic; c++ )
	{
		snprintf(tempstr, 1000, filenameTemplate, c);
		if ( PHYSFS_getRealDir(tempstr) != nullptr )
		{
			std::string musicDir = PHYSFS_getRealDir(tempstr);
			if ( musicDir.compare("./") != 0 || reloadAll )
			{
				musicDir.append(PHYSFS_getDirSeparator()).append(tempstr);
				printlog("[PhysFS]: Loading music file %s...", tempstr);
				if ( musicArray )
				{
					musicArray[c]->release();
				}
                if ( musicPreload )
                {
                    fmod_result = fmod_system->createSound(musicDir.c_str(), FMOD_2D, nullptr, &musicArray[c]); //TODO: Any other FMOD_MODEs should be used here? FMOD_SOFTWARE -> what now? FMOD_2D? LOOP?
                }
                else
                {
                    fmod_result = fmod_system->createStream(musicDir.c_str(), FMOD_2D, nullptr, &musicArray[c]); //TODO: Any other FMOD_MODEs should be used here? FMOD_SOFTWARE -> what now? FMOD_2D? LOOP?
                }
                if (fmod_result != FMOD_OK)
                {
                    printlog("[PhysFS]: ERROR: Failed reloading music file \"%s\".");
                    return fmod_result;
                }
			}
		}
	}

	return FMOD_OK;
}
#endif

void physfsReloadMusic(bool &introMusicChanged, bool reloadAll) //TODO: This should probably return an error.
{
	if ( no_sound )
	{
		return;
	}
#ifdef SOUND

	std::vector<std::string> themeMusic;
	themeMusic.push_back("music/introduction.ogg");
	themeMusic.push_back("music/intermission.ogg");
	themeMusic.push_back("music/minetown.ogg");
	themeMusic.push_back("music/splash.ogg");
	themeMusic.push_back("music/library.ogg");
	themeMusic.push_back("music/shop.ogg");
	themeMusic.push_back("music/herxboss.ogg");
	themeMusic.push_back("music/temple.ogg");
	themeMusic.push_back("music/endgame.ogg");
	themeMusic.push_back("music/escape.ogg");
	themeMusic.push_back("music/devil.ogg");
	themeMusic.push_back("music/sanctum.ogg");
	themeMusic.push_back("music/gnomishmines.ogg");
	themeMusic.push_back("music/greatcastle.ogg");
	themeMusic.push_back("music/sokoban.ogg");
	themeMusic.push_back("music/caveslair.ogg");
	themeMusic.push_back("music/bramscastle.ogg");
	themeMusic.push_back("music/hamlet.ogg");
	themeMusic.push_back("music/tutorial.ogg");
	themeMusic.push_back("sound/Death.ogg");
	themeMusic.push_back("sound/ui/StoryMusicV3.ogg");

	int index = 0;
#ifdef USE_OPENAL
#define FMOD_System_CreateStream(A, B, C, D, E) OPENAL_CreateStreamSound(B, E) //TODO: If this is still needed, it's probably now broke!
#define FMOD_SOUND OPENAL_BUFFER
#define fmod_system 0
#define FMOD_SOFTWARE 0
#define FMOD_Sound_Release OPENAL_Sound_Release
	int fmod_result;
#endif
	for ( std::vector<std::string>::iterator it = themeMusic.begin(); it != themeMusic.end(); ++it )
	{
		std::string filename = *it;
		if ( PHYSFS_getRealDir(filename.c_str()) != nullptr )
		{
			std::string musicDir = PHYSFS_getRealDir(filename.c_str());
			if ( musicDir.compare("./") != 0 || reloadAll )
			{
				musicDir += PHYSFS_getDirSeparator() + filename;
				printlog("[PhysFS]: Loading music file %s...", filename.c_str());
				switch ( index )
				{
					case 0:
						if ( introductionmusic )
						{
							introductionmusic->release();
						}
                        if ( musicPreload )
                        {
                            fmod_result = fmod_system->createSound(musicDir.c_str(), FMOD_2D, nullptr, &introductionmusic); //TODO: FMOD_SOFTWARE -> what now? FMOD_2D? FMOD_LOOP_NORMAL? More things? Something else?
                        }
                        else
                        {
                            fmod_result = fmod_system->createStream(musicDir.c_str(), FMOD_2D, nullptr, &introductionmusic); //TODO: FMOD_SOFTWARE -> what now? FMOD_2D? FMOD_LOOP_NORMAL? More things? Something else?
                        }
						break;
					case 1:
						if ( intermissionmusic )
						{
							intermissionmusic->release();
						}
                        if ( musicPreload )
                        {
                            fmod_result = fmod_system->createSound(musicDir.c_str(), FMOD_2D, nullptr, &intermissionmusic);
                        }
                        else
                        {
                            fmod_result = fmod_system->createStream(musicDir.c_str(), FMOD_2D, nullptr, &intermissionmusic);
                        }
						break;
					case 2:
						if ( minetownmusic )
						{
							minetownmusic->release();
						}
                        if ( musicPreload )
                        {
                            fmod_result = fmod_system->createSound(musicDir.c_str(), FMOD_2D, nullptr, &minetownmusic);
                        }
                        else
                        {
                            fmod_result = fmod_system->createStream(musicDir.c_str(), FMOD_2D, nullptr, &minetownmusic);
                        }
						break;
					case 3:
						if ( splashmusic )
						{
							splashmusic->release();
						}
                        if ( musicPreload )
                        {
                            fmod_result = fmod_system->createSound(musicDir.c_str(), FMOD_2D, nullptr, &splashmusic);
                        }
                        else
                        {
                            fmod_result = fmod_system->createStream(musicDir.c_str(), FMOD_2D, nullptr, &splashmusic);
                        }
						break;
					case 4:
						if ( librarymusic )
						{
							librarymusic->release();
						}
                        if ( musicPreload )
                        {
                            fmod_result = fmod_system->createSound(musicDir.c_str(), FMOD_2D, nullptr, &librarymusic);
                        }
                        else
                        {
                            fmod_result = fmod_system->createStream(musicDir.c_str(), FMOD_2D, nullptr, &librarymusic);
                        }
						break;
					case 5:
						if ( shopmusic )
						{
							shopmusic->release();
						}
                        if ( musicPreload )
                        {
                            fmod_result = fmod_system->createSound(musicDir.c_str(), FMOD_2D, nullptr, &shopmusic);
                        }
                        else
                        {
                            fmod_result = fmod_system->createStream(musicDir.c_str(), FMOD_2D, nullptr, &shopmusic);
                        }
						break;
					case 6:
						if ( herxmusic )
						{
							herxmusic->release();
						}
                        if ( musicPreload )
                        {
                            fmod_result = fmod_system->createSound(musicDir.c_str(), FMOD_2D, nullptr, &herxmusic);
                        }
                        else
                        {
                            fmod_result = fmod_system->createStream(musicDir.c_str(), FMOD_2D, nullptr, &herxmusic);
                        }
						break;
					case 7:
						if ( templemusic )
						{
							templemusic->release();
						}
                        if ( musicPreload )
                        {
                            fmod_result = fmod_system->createSound(musicDir.c_str(), FMOD_2D, nullptr, &templemusic);
                        }
                        else
                        {
                            fmod_result = fmod_system->createStream(musicDir.c_str(), FMOD_2D, nullptr, &templemusic);
                        }
						break;
					case 8:
						if ( endgamemusic )
						{
							endgamemusic->release();
						}
                        if ( musicPreload )
                        {
                            fmod_result = fmod_system->createSound(musicDir.c_str(), FMOD_2D, nullptr, &endgamemusic);
                        }
                        else
                        {
                            fmod_result = fmod_system->createStream(musicDir.c_str(), FMOD_2D, nullptr, &endgamemusic);
                        }
						break;
					case 9:
						if ( escapemusic )
						{
							escapemusic->release();
						}
                        if ( musicPreload )
                        {
                            fmod_result = fmod_system->createSound(musicDir.c_str(), FMOD_2D, nullptr, &escapemusic);
                        }
                        else
                        {
                            fmod_result = fmod_system->createStream(musicDir.c_str(), FMOD_2D, nullptr, &escapemusic);
                        }
						break;
					case 10:
						if ( devilmusic )
						{
							devilmusic->release();
						}
                        if ( musicPreload )
                        {
                            fmod_result = fmod_system->createSound(musicDir.c_str(), FMOD_2D, nullptr, &devilmusic);
                        }
                        else
                        {
                            fmod_result = fmod_system->createStream(musicDir.c_str(), FMOD_2D, nullptr, &devilmusic);
                        }
						break;
					case 11:
						if ( sanctummusic )
						{
							sanctummusic->release();
                        }
                        if ( musicPreload )
                        {
                            fmod_result = fmod_system->createSound(musicDir.c_str(), FMOD_2D, nullptr, &sanctummusic);
                        }
                        else
                        {
                            fmod_result = fmod_system->createStream(musicDir.c_str(), FMOD_2D, nullptr, &sanctummusic);
                        }
						break;
					case 12:
						if ( gnomishminesmusic )
						{
							gnomishminesmusic->release();
						}
						if ( musicPreload )
						{
							fmod_result = fmod_system->createSound(musicDir.c_str(), FMOD_2D, nullptr, &gnomishminesmusic);
						}
						else
						{
							fmod_result = fmod_system->createStream(musicDir.c_str(), FMOD_2D, nullptr, &gnomishminesmusic);
						}
						break;
					case 13:
						if ( greatcastlemusic )
						{
							greatcastlemusic->release();
						}
						if ( musicPreload )
						{
							fmod_result = fmod_system->createSound(musicDir.c_str(), FMOD_2D, nullptr, &greatcastlemusic);
						}
						else
						{
							fmod_result = fmod_system->createStream(musicDir.c_str(), FMOD_2D, nullptr, &greatcastlemusic);
						}
						break;
					case 14:
						if ( sokobanmusic )
						{
							sokobanmusic->release();
						}
						if ( musicPreload )
						{
							fmod_result = fmod_system->createSound(musicDir.c_str(), FMOD_2D, nullptr, &sokobanmusic);
						}
						else
						{
							fmod_result = fmod_system->createStream(musicDir.c_str(), FMOD_2D, nullptr, &sokobanmusic);
						}
						break;
					case 15:
						if ( caveslairmusic )
						{
							caveslairmusic->release();
						}
						if ( musicPreload )
						{
							fmod_result = fmod_system->createSound(musicDir.c_str(), FMOD_2D, nullptr, &caveslairmusic);
						}
						else
						{
							fmod_result = fmod_system->createStream(musicDir.c_str(), FMOD_2D, nullptr, &caveslairmusic);
						}
						break;
					case 16:
						if ( bramscastlemusic )
						{
							bramscastlemusic->release();
						}
						if ( musicPreload )
						{
							fmod_result = fmod_system->createSound(musicDir.c_str(), FMOD_2D, nullptr, &bramscastlemusic);
						}
						else
						{
							fmod_result = fmod_system->createStream(musicDir.c_str(), FMOD_2D, nullptr, &bramscastlemusic);
						}
						break;
					case 17:
						if ( hamletmusic )
						{
							hamletmusic->release();
						}
						if ( musicPreload )
						{
							fmod_result = fmod_system->createSound(musicDir.c_str(), FMOD_2D, nullptr, &hamletmusic);
						}
						else
						{
							fmod_result = fmod_system->createStream(musicDir.c_str(), FMOD_2D, nullptr, &hamletmusic);
						}
						break;
					case 18:
						if ( tutorialmusic )
						{
							tutorialmusic->release();
						}
						if ( musicPreload )
						{
							fmod_result = fmod_system->createSound(musicDir.c_str(), FMOD_2D, nullptr, &tutorialmusic);
						}
						else
						{
							fmod_result = fmod_system->createStream(musicDir.c_str(), FMOD_2D, nullptr, &tutorialmusic);
						}
						break;
					case 19:
						if ( gameovermusic )
						{
							gameovermusic->release();
						}
						if ( musicPreload )
						{
							fmod_result = fmod_system->createSound(musicDir.c_str(), FMOD_DEFAULT, nullptr, &gameovermusic);
						}
						else
						{
							fmod_result = fmod_system->createStream(musicDir.c_str(), FMOD_DEFAULT, nullptr, &gameovermusic);
						}
						break;
					case 20:
						if ( introstorymusic )
						{
							introstorymusic->release();
						}
						if ( musicPreload )
						{
							fmod_result = fmod_system->createSound(musicDir.c_str(), FMOD_DEFAULT, nullptr, &introstorymusic);
						}
						else
						{
							fmod_result = fmod_system->createStream(musicDir.c_str(), FMOD_DEFAULT, nullptr, &introstorymusic);
						}
						break;
					default:
						break;
				}
				if ( FMODErrorCheck() )
				{
					printlog("[PhysFS]: ERROR: Failed reloading music file \"%s\".", filename.c_str());
					//TODO: Handle error? Abort? Fling pies at people?
				}
			}
		}
		++index;
	}

	int c;
	FMOD::Sound** music = nullptr;

	if (FMOD_OK != (fmod_result = physfsReloadMusic_helper_reloadMusicArray(NUMMINESMUSIC, "music/mines%02d.ogg", minesmusic, reloadAll)) )
	{
		printlog("[PhysFS]: Failed to reload mines music array.");
		//TODO: Handle error? Abort? Fling pies at people?
	}
	if (FMOD_OK != (fmod_result = physfsReloadMusic_helper_reloadMusicArray(NUMSWAMPMUSIC, "music/swamp%02d.ogg", swampmusic, reloadAll)) )
	{
		printlog("[PhysFS]: Failed to reload swamp music array.");
	}
	if (FMOD_OK != (fmod_result = physfsReloadMusic_helper_reloadMusicArray(NUMLABYRINTHMUSIC, "music/labyrinth%02d.ogg", labyrinthmusic, reloadAll)) )
	{
		printlog("[PhysFS]: Failed to reload labyrinth music array.");
	}
	if (FMOD_OK != (fmod_result = physfsReloadMusic_helper_reloadMusicArray(NUMRUINSMUSIC, "music/ruins%02d.ogg", ruinsmusic, reloadAll)) )
	{
		printlog("[PhysFS]: Failed to reload ruins music array.");
	}
	if (FMOD_OK != (fmod_result = physfsReloadMusic_helper_reloadMusicArray(NUMUNDERWORLDMUSIC, "music/underworld%02d.ogg", underworldmusic, reloadAll)) )
	{
		printlog("[PhysFS]: Failed to reload underworld music array.");
	}
	if (FMOD_OK != (fmod_result = physfsReloadMusic_helper_reloadMusicArray(NUMHELLMUSIC, "music/hell%02d.ogg", hellmusic, reloadAll)) )
	{
		printlog("[PhysFS]: Failed to reload hell music array.");
	}
	if (FMOD_OK != (fmod_result = physfsReloadMusic_helper_reloadMusicArray(NUMMINOTAURMUSIC, "music/minotaur%02d.ogg", minotaurmusic, reloadAll)) )
	{
		printlog("[PhysFS]: Failed to reload minotaur music array.");
	}
	if (FMOD_OK != (fmod_result = physfsReloadMusic_helper_reloadMusicArray(NUMCAVESMUSIC, "music/caves%02d.ogg", cavesmusic, reloadAll)) )
	{
		printlog("[PhysFS]: Failed to reload caves music array.");
	}
	if (FMOD_OK != (fmod_result = physfsReloadMusic_helper_reloadMusicArray(NUMCITADELMUSIC, "music/citadel%02d.ogg", citadelmusic, reloadAll)) )
	{
		printlog("[PhysFS]: Failed to reload citadel music array.");
	}

	bool introChanged = false;

	for ( c = 0; c < NUMINTROMUSIC; c++ )
	{
		if ( c == 0 )
		{
			strcpy(tempstr, "music/intro.ogg");
		}
		else
		{
			snprintf(tempstr, 1000, "music/intro%02d.ogg", c);
		}
		if ( PHYSFS_getRealDir(tempstr) != nullptr )
		{
			std::string musicDir = PHYSFS_getRealDir(tempstr);
			if ( musicDir.compare("./") != 0 || reloadAll )
			{
				musicDir.append(PHYSFS_getDirSeparator()).append(tempstr);
				printlog("[PhysFS]: Loading music file %s...", tempstr);
				music = intromusic;
				if ( music )
				{
					music[c]->release();
				}
                if ( musicPreload )
                {
                    fmod_result = fmod_system->createSound(musicDir.c_str(), FMOD_2D, nullptr, &music[c]);
                }
                else
                {
                    fmod_result = fmod_system->createStream(musicDir.c_str(), FMOD_2D, nullptr, &music[c]);
                }
                introChanged = true;
                if (fmod_result != FMOD_OK)
                {
                    printlog("[PhysFS]: ERROR: Failed reloading music file \"%s\".");
                    break; //TODO: Handle the error?
                }
			}
		}
	}

	introMusicChanged = introChanged; // use this variable outside of this function to start playing a new fresh list of tracks in the main menu.
#ifdef USE_OPENAL
#undef FMOD_System_CreateStream
#undef FMOD_SOUND
#undef fmod_system
#undef FMOD_SOFTWARE
#undef FMOD_Sound_Release
#endif

#endif // SOUND
}

void gamemodsUnloadCustomThemeMusic()
{
#ifdef SOUND
#ifdef USE_OPENAL
#define FMOD_Sound_Release OPENAL_Sound_Release
#endif
	// free custom music slots, not used by official music assets.
	if ( gnomishminesmusic )
	{
		gnomishminesmusic->release();
		gnomishminesmusic = nullptr;
	}
	if ( greatcastlemusic )
	{
		greatcastlemusic->release();
		greatcastlemusic = nullptr;
	}
	if ( sokobanmusic )
	{
		sokobanmusic->release();
		sokobanmusic = nullptr;
	}
	if ( caveslairmusic )
	{
		caveslairmusic->release();
		caveslairmusic = nullptr;
	}
	if ( bramscastlemusic )
	{
		bramscastlemusic->release();
		bramscastlemusic = nullptr;
	}
	if ( hamletmusic )
	{
		hamletmusic->release();
		hamletmusic = nullptr;
	}
#ifdef USE_OPENAL
#undef FMOD_Sound_Release
#endif
#endif // !SOUND
}
