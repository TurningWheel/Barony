/*-------------------------------------------------------------------------------

	BARONY
	File: sound_openal.cpp
	Desc: OpenAL-based implementation of the sound interface.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "Config.hpp"
#ifdef APPLE
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else /* APPLE */
#include <AL/al.h>
#include <AL/alc.h>
#endif

#ifdef USE_TREMOR
#include <tremor/ivorbisfile.h>
#else /* USE_TREMOR */
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <vorbis/codec.h>
#endif /* USE_TREMOR */

#include "sound.hpp"

#include "main.hpp"
#include "entity.hpp"
#include "files.hpp"
#include "game.hpp"
#include "player.hpp"
#include "sound.hpp"

struct Sound {
	ALuint id;
	bool stream;
	char oggfile[256];
};
struct Channel {
	ALuint id;
	ChannelGroup *group;
	float volume;
	Sound *buffer;
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

struct ChannelGroup {
	float volume;
	int num;
	int cap;
	Channel **sounds;
};

struct FMOD_VECTOR {
	float x, y, z;
};

SDL_mutex *openal_mutex;

static size_t openal_oggread(void* ptr, size_t size, size_t nmemb, void* datasource) {
	Channel* self = (Channel*)datasource;

	int bytes = size*nmemb;
	int remain = self->oggdata_lenght - self->ogg_seekoffset - bytes;
	if(remain < 0) bytes += remain;

	memcpy(ptr, self->oggdata + self->ogg_seekoffset, bytes);
	self->ogg_seekoffset += bytes;

	return bytes;
}

static int openal_oggseek(void* datasource, ogg_int64_t offset, int whence) {
	Channel* self = (Channel*)datasource;
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
	Channel* self = (Channel*)datasource;
	return self->ogg_seekoffset;
}

static int openal_oggopen(Channel *self, const char* oggfile) {
	FILE *f = openDataFile(oggfile, "rb");

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

static int openal_oggrelease(Channel *self) {
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

static int openal_streamread(Channel *self, ALuint buffer) {
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

static int openal_streamupdate(Channel* self) {
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

Sound** sounds = NULL;
Uint32 numsounds = 0;
Sound** minesmusic = NULL;
Sound** swampmusic = NULL;
Sound** labyrinthmusic = NULL;
Sound** ruinsmusic = NULL;
Sound** underworldmusic = NULL;
Sound** hellmusic = NULL;
Sound* intromusic = NULL;
Sound* intermissionmusic = NULL;
Sound* minetownmusic = NULL;
Sound* splashmusic = NULL;
Sound* librarymusic = NULL;
Sound* shopmusic = NULL;
Sound* storymusic = NULL;
Sound** minotaurmusic = NULL;
Sound* herxmusic = NULL;
Sound* templemusic = NULL;
Sound* endgamemusic = NULL;
Sound* devilmusic = NULL;
Sound* escapemusic = NULL;
Sound* introductionmusic = NULL;
bool levelmusicplaying = false;

Channel* music_channel = NULL;
Channel* music_channel2 = NULL;
Channel* music_resume = NULL;

ChannelGroup *sound_group = NULL;
ChannelGroup *music_group = NULL;

float fadein_increment = 0.002f;
float default_fadein_increment = 0.002f;
float fadeout_increment = 0.005f;
float default_fadeout_increment = 0.005f;
SoundSystem *soundSystem = NULL;

#define MAXSOUND 1024
Channel openal_sounds[MAXSOUND];
int lower_freechannel = 0;
int upper_unfreechannel = 0;

SDL_Thread* openal_soundthread;
bool OpenALSoundON = true;

void OPENAL_RemoveChannelGroup(Channel *channel, ChannelGroup *group);

static void private_Channel_Stop(Channel* channel) {
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
					private_Channel_Stop(&openal_sounds[i]);
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

void initSound()
{
	static int initialized = 0;
	if(initialized)
		return;

	openal_device = alcOpenDevice(NULL); // preferred device
	if(!openal_device)
		return;

	openal_context = alcCreateContext(openal_device,NULL);
	if(!openal_context)
		return;

	alcMakeContextCurrent(openal_context);

	alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
	alDopplerFactor(2.0f);

	// creates channels groups
	sound_group = (ChannelGroup*)malloc(sizeof(ChannelGroup));
	music_group = (ChannelGroup*)malloc(sizeof(ChannelGroup));
	memset(sound_group, 0, sizeof(ChannelGroup));
	memset(music_group, 0, sizeof(ChannelGroup));
	sound_group->volume = 1.0f;
	music_group->volume = 1.0f;

	memset(openal_sounds, 0, sizeof(openal_sounds));
	lower_freechannel = 0;
	upper_unfreechannel = 0;

	OpenALSoundON = true;
	openal_mutex = SDL_CreateMutex();
	openal_soundthread = SDL_CreateThread(OPENAL_ThreadFunction, "openal", NULL);

	initialized = 1;
}

void deinitSound()
{
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
			private_Channel_Stop(&openal_sounds[i]);
		}
	}

	alcMakeContextCurrent(NULL);
	alcDestroyContext(openal_context);
	openal_context = NULL;
	alcCloseDevice(openal_device);
	openal_device = NULL;
	initialized = 0;
}

void Channel_SetVolume(Channel *channel, float f) {
	channel->volume = f;
	if(channel->group)
		f *= channel->group->volume;
	alSourcef(channel->id, AL_GAIN, f);
}

void ChannelGroup_Stop(ChannelGroup* group) {
	for (int i = 0; i< group->num; i++) {
		if (group->sounds[i])
			alSourceStop( group->sounds[i]->id );
	}
}

void ChannelGroup_SetVolume(ChannelGroup* group, float f) {
	group->volume = f;
	for (int i = 0; i< group->num; i++) {
		if (group->sounds[i])
			alSourcef( group->sounds[i]->id, AL_GAIN, f*group->sounds[i]->volume );
	}
}

bool Channel_IsPlaying(Channel* channel) {
	ALint state;
	alGetSourcei( channel->id, AL_SOURCE_STATE, &state );
	return (state == AL_PLAYING);
}

void Channel_Stop(Channel* channel) {
	SDL_LockMutex(openal_mutex);

	if(channel==NULL || !channel->active) {
		SDL_UnlockMutex(openal_mutex);
		return;
	}

	int i = channel->indice;
	private_Channel_Stop(channel);
	if (lower_freechannel > i)
		lower_freechannel = i;


	SDL_UnlockMutex(openal_mutex);
}

void Channel_Set3DAttributes(Channel* channel, float x, float y, float z) {

	alSourcei(channel->id,AL_SOURCE_RELATIVE, AL_FALSE);
	alSource3f(channel->id, AL_POSITION, x, y, z);
	alSourcef(channel->id, AL_REFERENCE_DISTANCE, 1.f);	// hardcoding FMOD_System_Set3DSettings(fmod_system, 1.0, 2.0, 1.0);
	alSourcef(channel->id, AL_MAX_DISTANCE, 10.f);		// but this are simply OpenAL default (the 2.0f is used for Dopler only)
}

void Channel_Play(Channel* channel) {
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

void Channel_Pause(Channel* channel) {
	alSourcePause(channel->id);
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

	private_Channel_Stop(&openal_sounds[i]);

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
				Channel_SetVolume(music_channel, volume);
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
				Channel_SetVolume(music_channel2, volume);
			} else {
				/*Channel_Stop(music_channel2);
				music_channel2 = NULL;*/
				Channel_Pause(music_channel2);
			}
		}
	}
}

void Channel_SetChannelGroup(Channel *channel, ChannelGroup *group) {
	if(group->num==group->cap) {
		group->cap += 8;
		group->sounds = (Channel**)realloc(group->sounds, group->cap*sizeof(Channel*));
	}
	alSourcef(channel->id, AL_GAIN, channel->volume * group->volume);
	group->sounds[group->num++] = channel;
	channel->group = group;
}

void OPENAL_RemoveChannelGroup(Channel *channel, ChannelGroup *group) {
	int i = 0;
	while ((i<group->num) && (channel!=group->sounds[i]))
		i++;
	if(i==group->num)
		return;
	memcpy(group->sounds+i, group->sounds+i+1, sizeof(Channel*)*(group->num-(i+1)));
	group->num--;
}

Sound* createSound(SoundSystem *sys, const char* name) {
	Sound* result = (Sound*) malloc(sizeof(Sound));
	strcpy(result->oggfile, name);	// for debugging purpose
	result->stream = false;
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
	if(channels==2) {
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
	alGenBuffers(1, &result->id);
	alBufferData(result->id, (channels==1)?AL_FORMAT_MONO16:AL_FORMAT_STEREO16, data2, sz, freq);
	if(data2!=data)
		free(data2);
	free(data);
	return result;
}

Sound* CreateMusic(const char* name) {
	Sound *buffer = (Sound*)malloc(sizeof(Sound));
	buffer->stream = true;
	strcpy(buffer->oggfile, name);
	return buffer;
}

Channel* OPENAL_CreateChannel(Sound* buffer) {
	//Channel *channel=(Channel*)malloc(sizeof(Channel));

	SDL_LockMutex(openal_mutex);

	int i = get_firstfreechannel();

	if(upper_unfreechannel < (i+1))
		upper_unfreechannel = i+1;
	lower_freechannel = i+1;

	Channel *channel = &openal_sounds[i];
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

void OPENAL_GetBuffer(Channel* channel, Sound** buffer) {
	(*buffer) = channel->buffer;
}

void OPENAL_SetLoop(Channel* channel, ALboolean looping) {
	channel->loop = looping;
	if(!channel->buffer->stream)
		alSourcei(channel->id, AL_LOOPING, looping);
}

unsigned int Channel_GetPosition(Channel* channel) {
	ALint position;
	alGetSourcei(channel->id, AL_BYTE_OFFSET, &position);
	return position;
}

unsigned int Sound_GetLength(Sound* buffer) {
	if(!buffer)
		return 0;
	ALint length;
	alGetBufferi(buffer->id, AL_SIZE, &length);
	return length;
}

void Sound_Release(Sound* buffer) {
	if(!buffer) return;
	if(!buffer->stream)
		alDeleteBuffers( 1, &buffer->id );
	free(buffer);
}

Channel* playSoundPosLocal(real_t x, real_t y, Uint32 snd, int vol)
{
	if (no_sound)
	{
		return NULL;
	}

#ifndef SOUND
	return NULL;
#endif

	Channel* channel;

	if (intro)
	{
		return NULL;
	}
	if (snd < 0 || snd >= numsounds)
	{
		return NULL;
	}
	if (sounds[snd] == NULL || vol == 0)
	{
		return NULL;
	}

	if (!openal_context)   //For the client.
	{
		return NULL;
	}

	channel = OPENAL_CreateChannel(sounds[snd]);
	Channel_SetVolume(channel, vol / 128.f);
	Channel_Set3DAttributes(channel, -y / 16.0, 0, -x / 16.0);
	Channel_SetChannelGroup(channel, sound_group);
	Channel_Play(channel);

	return channel;
}

/*-------------------------------------------------------------------------------

	playSound

	plays a sound effect with the given volume and returns the channel that
	the sound is playing in

-------------------------------------------------------------------------------*/

Channel* playSound(Uint32 snd, int vol)
{
	if (no_sound)
	{
		return NULL;
	}
#ifndef SOUND
	return NULL;
#endif
	if (!openal_context || snd < 0 || snd >= numsounds || !sound_group)
	{
		return NULL;
	}
	if (sounds[snd] == NULL || vol == 0)
	{
		return NULL;
	}
	Channel* channel = OPENAL_CreateChannel(sounds[snd]);
	Channel_SetVolume(channel, vol / 128.f);

	Channel_SetChannelGroup(channel, sound_group);

	Channel_Play(channel);

	return channel;
}

void playmusic(Sound* sound, bool loop, bool crossfade, bool resume)
{
	if (no_sound)
	{
		return;
	}
#ifndef SOUND
	return;
#endif
#ifndef MUSIC
	return;
#endif
	fadein_increment = default_fadein_increment;
	fadeout_increment = default_fadeout_increment;
	if (!openal_context || !sound)
	{
		printlog("Can't play music.\n");
		return;
	}
	if ( resume && music_channel2 )
	{
		Sound* lastmusic = NULL;
		OPENAL_GetBuffer(music_channel2, &lastmusic);
		if ( lastmusic == sound )
		{
			Channel* tempmusic = music_channel;
			music_channel = music_channel2;
			music_channel2 = tempmusic;
		}
		else
		{
			Channel_Stop(music_channel2);
			music_channel2 = music_channel;
			music_channel = OPENAL_CreateChannel(sound);
		}
	}
	else
	{
		Channel_Stop(music_channel2);
		music_channel2 = music_channel;
		music_channel = OPENAL_CreateChannel(sound);
	}
	Channel_SetChannelGroup(music_channel, music_group);
	if (crossfade == true)
	{
		//Start at volume 0 to get louder.
		Channel_SetVolume(music_channel, 0.0f); //Start at 0 then pop up.
	}
	else
	{
		Channel_SetVolume(music_channel, 1.0f);
		Channel_Stop(music_channel2);
		music_channel2 = NULL;
	}
	if (loop == true)
	{
		OPENAL_SetLoop(music_channel, AL_TRUE);
	}
	Channel_Play(music_channel);
}

