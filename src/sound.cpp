/*-------------------------------------------------------------------------------

	BARONY
	File: sound.cpp
	Desc: various sound functions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "files.hpp"
//#include "game.hpp"
#include "sound.hpp"

#ifdef USE_FMOD
#include <fmod_errors.h>
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
FMOD_SYSTEM* fmod_system = NULL;

FMOD_RESULT fmod_result;

int fmod_maxchannels = 100;
int fmod_flags;
void* fmod_extdriverdata;

FMOD_SOUND** sounds = NULL;
Uint32 numsounds = 0;
FMOD_SOUND** minesmusic = NULL;
FMOD_SOUND** swampmusic = NULL;
FMOD_SOUND** labyrinthmusic = NULL;
FMOD_SOUND** ruinsmusic = NULL;
FMOD_SOUND** underworldmusic = NULL;
FMOD_SOUND** hellmusic = NULL;
FMOD_SOUND** intromusic = NULL;
FMOD_SOUND* intermissionmusic = NULL;
FMOD_SOUND* minetownmusic = NULL;
FMOD_SOUND* splashmusic = NULL;
FMOD_SOUND* librarymusic = NULL;
FMOD_SOUND* shopmusic = NULL;
FMOD_SOUND* storymusic = NULL;
FMOD_SOUND** minotaurmusic = NULL;
FMOD_SOUND* herxmusic = NULL;
FMOD_SOUND* templemusic = NULL;
FMOD_SOUND* endgamemusic = NULL;
FMOD_SOUND* devilmusic = NULL;
FMOD_SOUND* escapemusic = NULL;
FMOD_SOUND* sanctummusic = NULL;
FMOD_SOUND* introductionmusic = NULL;
FMOD_SOUND** cavesmusic = NULL;
FMOD_SOUND** citadelmusic = NULL;
bool levelmusicplaying = false;

FMOD_CHANNEL* music_channel = NULL;
FMOD_CHANNEL* music_channel2 = NULL;
FMOD_CHANNEL* music_resume = NULL;

FMOD_CHANNELGROUP* sound_group = NULL;
FMOD_CHANNELGROUP* music_group = NULL;

float fadein_increment = 0.002f;
float default_fadein_increment = 0.002f;
float fadeout_increment = 0.005f;
float default_fadeout_increment = 0.005f;

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

void sound_update()
{
	if (no_sound)
	{
		return;
	}
	if (!fmod_system)
	{
		return;
	}

	FMOD_VECTOR position, forward, up;
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

	forward.x = 1 * sin(camera.ang);
	forward.y = 0;
	forward.z = 1 * cos(camera.ang);
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
	if (music_channel)
	{
		FMOD_BOOL playing = false;
		FMOD_Channel_IsPlaying(music_channel, &playing);
		if (playing)
		{
			float volume = 1.0f;
			FMOD_Channel_GetVolume(music_channel, &volume);

			if (volume < 1.0f)
			{
				volume += fadein_increment * 2;
				if (volume > 1.0f)
				{
					volume = 1.0f;
				}
				FMOD_Channel_SetVolume(music_channel, volume);
			}
		}
	}
	//The following makes crossfading possible. Fade out the last playing music. //TODO: Support for saving music so that it can be resumed (for stuff interrupting like combat music).
	if (music_channel2)
	{
		FMOD_BOOL playing = false;
		FMOD_Channel_IsPlaying(music_channel2, &playing);
		if (playing)
		{
			float volume = 0.0f;
			FMOD_Channel_GetVolume(music_channel2, &volume);

			if (volume > 0.0f)
			{
				//volume -= 0.001f;
				//volume -= 0.005f;
				volume -= fadeout_increment * 2;
				if (volume < 0.0f)
				{
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

#elif defined USE_OPENAL

struct OPENAL_BUFFER {
	ALuint id;
	bool stream;
	char oggfile[256];
};
struct OPENAL_SOUND {
	ALuint id;
	OPENAL_CHANNELGROUP *group;
	float volume;
	OPENAL_BUFFER *buffer;
	bool active;
	char* oggdata;
	int oggdata_lenght;
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
	int remain = self->oggdata_lenght - self->ogg_seekoffset - bytes;
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
		seek_offset = self->oggdata_lenght + offset;
		break;
	case SEEK_SET:
		seek_offset = offset;
		break;
	/*default:
		exit(1);*/
	}
	if(seek_offset > self->oggdata_lenght) return -1;

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
	FILE *f = openDataFile(oggfile, "rb");
	int err;

	ov_callbacks oggcb = {openal_oggread, openal_oggseek, openal_oggclose, openal_oggtell};

	if(!f) {
		return 0;
	}

	self->ogg_seekoffset = 0;
	fseek(f, 0, SEEK_END);
	self->oggdata_lenght = ftell(f);
	fseek(f, 0, SEEK_SET);

	self->oggdata = (char*)malloc(self->oggdata_lenght);
	fread(self->oggdata, sizeof(char), self->oggdata_lenght, f);
	fclose(f);

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

ALCcontext *openal_context = NULL;
ALCdevice  *openal_device = NULL;

//#define openal_maxchannels 100

OPENAL_BUFFER** sounds = NULL;
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
bool levelmusicplaying = false;

OPENAL_SOUND* music_channel = NULL;
OPENAL_SOUND* music_channel2 = NULL;
OPENAL_SOUND* music_resume = NULL;

OPENAL_CHANNELGROUP *sound_group = NULL;
OPENAL_CHANNELGROUP *music_group = NULL;

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
				ALint state = AL_STOPPED;
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
	music_group = (OPENAL_CHANNELGROUP*)malloc(sizeof(OPENAL_CHANNELGROUP));
	memset(sound_group, 0, sizeof(OPENAL_CHANNELGROUP));
	memset(music_group, 0, sizeof(OPENAL_CHANNELGROUP));
	sound_group->volume = 1.0f;
	music_group->volume = 1.0f;

	memset(openal_sounds, 0, sizeof(openal_sounds));
	lower_freechannel = 0;
	upper_unfreechannel = 0;

	OpenALSoundON = true;
	openal_mutex = SDL_CreateMutex();
	openal_soundthread = SDL_CreateThread(OPENAL_ThreadFunction, "openal", NULL);

	initialized = 1;

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

void sound_update()
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

int OPENAL_CreateSound(const char* name, bool b3D, OPENAL_BUFFER **buffer) {
	*buffer = (OPENAL_BUFFER*)malloc(sizeof(OPENAL_BUFFER));
	strcpy((*buffer)->oggfile, name);	// for debugging purpose
	(*buffer)->stream = false;
	FILE *f = openDataFile(name, "rb");
	if(!f) {
		printlog("Error loading sound %s\n", name);
		return 0;
	}
	vorbis_info * pInfo;
	OggVorbis_File oggFile;
	ov_open(f, &oggFile, NULL, 0);
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

#define SOUND

#endif

bool physfsSearchMusicToUpdate()
{
#ifdef SOUND
	std::vector<std::string> themeMusic;
	themeMusic.push_back("music/splash.ogg");
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

	for ( std::vector<std::string>::iterator it = themeMusic.begin(); it != themeMusic.end(); ++it )
	{
		std::string filename = *it;
		if ( PHYSFS_getRealDir(filename.c_str()) != NULL )
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

	for ( c = 0; c < NUMMINESMUSIC; c++ )
	{
		snprintf(tempstr, 1000, "music/mines%02d.ogg", c);
		if ( PHYSFS_getRealDir(tempstr) != NULL )
		{
			std::string musicDir = PHYSFS_getRealDir(tempstr);
			if ( musicDir.compare("./") != 0 )
			{
				printlog("[PhysFS]: Found modified music in music/ directory, reloading music files...");
				return true;
			}
		}
	}
	for ( c = 0; c < NUMSWAMPMUSIC; c++ )
	{
		snprintf(tempstr, 1000, "music/swamp%02d.ogg", c);
		if ( PHYSFS_getRealDir(tempstr) != NULL )
		{
			std::string musicDir = PHYSFS_getRealDir(tempstr);
			if ( musicDir.compare("./") != 0 )
			{
				printlog("[PhysFS]: Found modified music in music/ directory, reloading music files...");
				return true;
			}
		}
	}
	for ( c = 0; c < NUMLABYRINTHMUSIC; c++ )
	{
		snprintf(tempstr, 1000, "music/labyrinth%02d.ogg", c);
		if ( PHYSFS_getRealDir(tempstr) != NULL )
		{
			std::string musicDir = PHYSFS_getRealDir(tempstr);
			if ( musicDir.compare("./") != 0 )
			{
				printlog("[PhysFS]: Found modified music in music/ directory, reloading music files...");
				return true;
			}
		}
	}
	for ( c = 0; c < NUMRUINSMUSIC; c++ )
	{
		snprintf(tempstr, 1000, "music/ruins%02d.ogg", c);
		if ( PHYSFS_getRealDir(tempstr) != NULL )
		{
			std::string musicDir = PHYSFS_getRealDir(tempstr);
			if ( musicDir.compare("./") != 0 )
			{
				printlog("[PhysFS]: Found modified music in music/ directory, reloading music files...");
				return true;
			}
		}
	}
	for ( c = 0; c < NUMUNDERWORLDMUSIC; c++ )
	{
		snprintf(tempstr, 1000, "music/underworld%02d.ogg", c);
		if ( PHYSFS_getRealDir(tempstr) != NULL )
		{
			std::string musicDir = PHYSFS_getRealDir(tempstr);
			if ( musicDir.compare("./") != 0 )
			{
				printlog("[PhysFS]: Found modified music in music/ directory, reloading music files...");
				return true;
			}
		}
	}
	for ( c = 0; c < NUMHELLMUSIC; c++ )
	{
		snprintf(tempstr, 1000, "music/hell%02d.ogg", c);
		if ( PHYSFS_getRealDir(tempstr) != NULL )
		{
			std::string musicDir = PHYSFS_getRealDir(tempstr);
			if ( musicDir.compare("./") != 0 )
			{
				printlog("[PhysFS]: Found modified music in music/ directory, reloading music files...");
				return true;
			}
		}
	}
	for ( c = 0; c < NUMMINOTAURMUSIC; c++ )
	{
		snprintf(tempstr, 1000, "music/minotaur%02d.ogg", c);
		if ( PHYSFS_getRealDir(tempstr) != NULL )
		{
			std::string musicDir = PHYSFS_getRealDir(tempstr);
			if ( musicDir.compare("./") != 0 )
			{
				printlog("[PhysFS]: Found modified music in music/ directory, reloading music files...");
				return true;
			}
		}
	}
	for ( c = 0; c < NUMCAVESMUSIC; c++ )
	{
		snprintf(tempstr, 1000, "music/caves%02d.ogg", c);
		if ( PHYSFS_getRealDir(tempstr) != NULL )
		{
			std::string musicDir = PHYSFS_getRealDir(tempstr);
			if ( musicDir.compare("./") != 0 )
			{
				printlog("[PhysFS]: Found modified music in music/ directory, reloading music files...");
				return true;
			}
		}
	}
	for ( c = 0; c < NUMCITADELMUSIC; c++ )
	{
		snprintf(tempstr, 1000, "music/citadel%02d.ogg", c);
		if ( PHYSFS_getRealDir(tempstr) != NULL )
		{
			std::string musicDir = PHYSFS_getRealDir(tempstr);
			if ( musicDir.compare("./") != 0 )
			{
				printlog("[PhysFS]: Found modified music in music/ directory, reloading music files...");
				return true;
			}
		}
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
		if ( PHYSFS_getRealDir(tempstr) != NULL )
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

void physfsReloadMusic(bool &introMusicChanged)
{
#ifdef SOUND

	std::vector<std::string> themeMusic;
	themeMusic.push_back("music/splash.ogg");
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

	int index = 0;
#ifdef USE_OPENAL
#define FMOD_System_CreateStream(A, B, C, D, E) OPENAL_CreateStreamSound(B, E)
#define FMOD_SOUND OPENAL_BUFFER
#define fmod_system 0
#define FMOD_SOFTWARE 0
#define FMOD_Sound_Release OPENAL_Sound_Release
	int fmod_result;
#endif
	for ( std::vector<std::string>::iterator it = themeMusic.begin(); it != themeMusic.end(); ++it )
	{
		std::string filename = *it;
		if ( PHYSFS_getRealDir(filename.c_str()) != NULL )
		{
			std::string musicDir = PHYSFS_getRealDir(filename.c_str());
			if ( musicDir.compare("./") != 0 )
			{
				musicDir += PHYSFS_getDirSeparator() + filename;
				printlog("[PhysFS]: Reloading music file %s...", filename.c_str());
				FMOD_SOUND* music = NULL;
				switch ( index )
				{
					case 0:
						music = introductionmusic;
						break;
					case 1:
						music = intermissionmusic;
						break;
					case 2:
						music = minetownmusic;
						break;
					case 3:
						music = splashmusic;
						break;
					case 4:
						music = librarymusic;
						break;
					case 5:
						music = shopmusic;
						break;
					case 6:
						music = herxmusic;
						break;
					case 7:
						music = templemusic;
						break;
					case 8:
						music = endgamemusic;
						break;
					case 9:
						music = escapemusic;
						break;
					case 10:
						music = devilmusic;
						break;
					case 11:
						music = sanctummusic;
						break;
					default:
						break;
				}
				if ( music )
				{
					FMOD_Sound_Release(music);
					fmod_result = FMOD_System_CreateStream(fmod_system, musicDir.c_str(), FMOD_SOFTWARE, NULL, &music);
				}
			}
		}
		++index;
	}

	int c;
	FMOD_SOUND** music = NULL;

	for ( c = 0; c < NUMMINESMUSIC; c++ )
	{
		snprintf(tempstr, 1000, "music/mines%02d.ogg", c);
		if ( PHYSFS_getRealDir(tempstr) != NULL )
		{
			std::string musicDir = PHYSFS_getRealDir(tempstr);
			if ( musicDir.compare("./") != 0 )
			{
				musicDir.append(PHYSFS_getDirSeparator()).append(tempstr);
				printlog("[PhysFS]: Reloading music file %s...", tempstr);
				music = minesmusic;
				if ( music )
				{
					FMOD_Sound_Release(music[c]);
					fmod_result = FMOD_System_CreateStream(fmod_system, musicDir.c_str(), FMOD_SOFTWARE, NULL, &music[c]);
				}
			}
		}
	}
	for ( c = 0; c < NUMSWAMPMUSIC; c++ )
	{
		snprintf(tempstr, 1000, "music/swamp%02d.ogg", c);
		if ( PHYSFS_getRealDir(tempstr) != NULL )
		{
			std::string musicDir = PHYSFS_getRealDir(tempstr);
			if ( musicDir.compare("./") != 0 )
			{
				musicDir.append(PHYSFS_getDirSeparator()).append(tempstr);
				printlog("[PhysFS]: Reloading music file %s...", tempstr);
				music = swampmusic;
				if ( music )
				{
					FMOD_Sound_Release(music[c]);
					fmod_result = FMOD_System_CreateStream(fmod_system, musicDir.c_str(), FMOD_SOFTWARE, NULL, &music[c]);
				}
			}
		}
	}
	for ( c = 0; c < NUMLABYRINTHMUSIC; c++ )
	{
		snprintf(tempstr, 1000, "music/labyrinth%02d.ogg", c);
		if ( PHYSFS_getRealDir(tempstr) != NULL )
		{
			std::string musicDir = PHYSFS_getRealDir(tempstr);
			if ( musicDir.compare("./") != 0 )
			{
				musicDir.append(PHYSFS_getDirSeparator()).append(tempstr);
				printlog("[PhysFS]: Reloading music file %s...", tempstr);
				music = labyrinthmusic;
				if ( music )
				{
					FMOD_Sound_Release(music[c]);
					fmod_result = FMOD_System_CreateStream(fmod_system, musicDir.c_str(), FMOD_SOFTWARE, NULL, &music[c]);
				}
			}
		}
	}
	for ( c = 0; c < NUMRUINSMUSIC; c++ )
	{
		snprintf(tempstr, 1000, "music/ruins%02d.ogg", c);
		if ( PHYSFS_getRealDir(tempstr) != NULL )
		{
			std::string musicDir = PHYSFS_getRealDir(tempstr);
			if ( musicDir.compare("./") != 0 )
			{
				musicDir.append(PHYSFS_getDirSeparator()).append(tempstr);
				printlog("[PhysFS]: Reloading music file %s...", tempstr);
				music = ruinsmusic;
				if ( music )
				{
					FMOD_Sound_Release(music[c]);
					fmod_result = FMOD_System_CreateStream(fmod_system, musicDir.c_str(), FMOD_SOFTWARE, NULL, &music[c]);
				}
			}
		}
	}
	for ( c = 0; c < NUMUNDERWORLDMUSIC; c++ )
	{
		snprintf(tempstr, 1000, "music/underworld%02d.ogg", c);
		if ( PHYSFS_getRealDir(tempstr) != NULL )
		{
			std::string musicDir = PHYSFS_getRealDir(tempstr);
			if ( musicDir.compare("./") != 0 )
			{
				musicDir.append(PHYSFS_getDirSeparator()).append(tempstr);
				printlog("[PhysFS]: Reloading music file %s...", tempstr);
				music = underworldmusic;
				if ( music )
				{
					FMOD_Sound_Release(music[c]);
					fmod_result = FMOD_System_CreateStream(fmod_system, musicDir.c_str(), FMOD_SOFTWARE, NULL, &music[c]);
				}
			}
		}
	}
	for ( c = 0; c < NUMHELLMUSIC; c++ )
	{
		snprintf(tempstr, 1000, "music/hell%02d.ogg", c);
		if ( PHYSFS_getRealDir(tempstr) != NULL )
		{
			std::string musicDir = PHYSFS_getRealDir(tempstr);
			if ( musicDir.compare("./") != 0 )
			{
				musicDir.append(PHYSFS_getDirSeparator()).append(tempstr);
				printlog("[PhysFS]: Reloading music file %s...", tempstr);
				music = hellmusic;
				if ( music )
				{
					FMOD_Sound_Release(music[c]);
					fmod_result = FMOD_System_CreateStream(fmod_system, musicDir.c_str(), FMOD_SOFTWARE, NULL, &music[c]);
				}
			}
		}
	}
	for ( c = 0; c < NUMMINOTAURMUSIC; c++ )
	{
		snprintf(tempstr, 1000, "music/minotaur%02d.ogg", c);
		if ( PHYSFS_getRealDir(tempstr) != NULL )
		{
			std::string musicDir = PHYSFS_getRealDir(tempstr);
			if ( musicDir.compare("./") != 0 )
			{
				musicDir.append(PHYSFS_getDirSeparator()).append(tempstr);
				printlog("[PhysFS]: Reloading music file %s...", tempstr);
				music = minotaurmusic;
				if ( music )
				{
					FMOD_Sound_Release(music[c]);
					fmod_result = FMOD_System_CreateStream(fmod_system, musicDir.c_str(), FMOD_SOFTWARE, NULL, &music[c]);
				}
			}
		}
	}
	for ( c = 0; c < NUMCAVESMUSIC; c++ )
	{
		snprintf(tempstr, 1000, "music/caves%02d.ogg", c);
		if ( PHYSFS_getRealDir(tempstr) != NULL )
		{
			std::string musicDir = PHYSFS_getRealDir(tempstr);
			if ( musicDir.compare("./") != 0 )
			{
				musicDir.append(PHYSFS_getDirSeparator()).append(tempstr);
				printlog("[PhysFS]: Reloading music file %s...", tempstr);
				music = cavesmusic;
				if ( music )
				{
					FMOD_Sound_Release(music[c]);
					fmod_result = FMOD_System_CreateStream(fmod_system, musicDir.c_str(), FMOD_SOFTWARE, NULL, &music[c]);
				}
			}
		}
	}
	for ( c = 0; c < NUMCITADELMUSIC; c++ )
	{
		snprintf(tempstr, 1000, "music/citadel%02d.ogg", c);
		if ( PHYSFS_getRealDir(tempstr) != NULL )
		{
			std::string musicDir = PHYSFS_getRealDir(tempstr);
			if ( musicDir.compare("./") != 0 )
			{
				musicDir.append(PHYSFS_getDirSeparator()).append(tempstr);
				printlog("[PhysFS]: Reloading music file %s...", tempstr);
				music = citadelmusic;
				if ( music )
				{
					FMOD_Sound_Release(music[c]);
					fmod_result = FMOD_System_CreateStream(fmod_system, musicDir.c_str(), FMOD_SOFTWARE, NULL, &music[c]);
				}
			}
		}
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
		if ( PHYSFS_getRealDir(tempstr) != NULL )
		{
			std::string musicDir = PHYSFS_getRealDir(tempstr);
			if ( musicDir.compare("./") != 0 )
			{
				musicDir.append(PHYSFS_getDirSeparator()).append(tempstr);
				printlog("[PhysFS]: Reloading music file %s...", tempstr);
				music = intromusic;
				if ( music )
				{
					FMOD_Sound_Release(music[c]);
					fmod_result = FMOD_System_CreateStream(fmod_system, musicDir.c_str(), FMOD_SOFTWARE, NULL, &music[c]);
					introChanged = true;
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