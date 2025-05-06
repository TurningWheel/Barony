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
#include <mutex>
#include <queue>
#ifdef USE_OPUS
#ifdef NINTENDO
typedef int16_t opus_int16;
#define opus_strerror(x) ""
#else
#include <opus/opus.h>
#endif
#endif

extern Uint32 numsounds;
bool initSoundEngine(); //If it fails to initialize the sound engine, it'll just disable audio.
void exitSoundEngine();
int loadSoundResources(real_t base_load_percent, real_t top_load_percent);
void freeSoundResources();
// all parameters should be in ranges of [0.0 - 1.0]
void setGlobalVolume(real_t master, real_t music, real_t gameplay, real_t ambient, real_t environment, real_t notification);
void setAudioDevice(const std::string& device);
void setRecordDevice(const std::string& device);
bool loadMusic();

#ifdef USE_FMOD

#define SOUND
#define MUSIC

extern FMOD_SPEAKERMODE fmod_speakermode;

extern const char* fmod_speakermode_strings[FMOD_SPEAKERMODE_MAX]; 

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

#define NUMENSEMBLEMUSIC 5
extern FMOD::Sound* music_ensemble_global_sound[NUMENSEMBLEMUSIC];
extern FMOD::Channel* music_ensemble_global_channel[NUMENSEMBLEMUSIC];
extern FMOD::ChannelGroup* music_ensemble_global_send_group;
extern FMOD::ChannelGroup* music_ensemble_global_recv_group;
extern FMOD::ChannelGroup* music_ensemble_local_recv_player[MAXPLAYERS];
extern FMOD::ChannelGroup* music_ensemble_local_recv_group;


/*
 * Checks for FMOD errors. Store return value of all FMOD functions in fmod_result so that this funtion can access it and check for errors.
 * Returns true on error (and prints an error message), false if everything went fine.
 */
bool FMODErrorCheck();

void sound_update(int player, int index, int numplayers);

FMOD::Channel* playSoundPlayer(int player, Uint16 snd, Uint8 vol);
FMOD::Channel* playSoundNotificationPlayer(int player, Uint16 snd, Uint8 vol);
FMOD::Channel* playSoundPos(real_t x, real_t y, Uint16 snd, Uint8 vol);
FMOD::Channel* playSoundPosLocal(real_t x, real_t y, Uint16 snd, Uint8 vol);
FMOD::Channel* playSoundEntity(Entity* entity, Uint16 snd, Uint8 vol);
FMOD::Channel* playSoundEntityLocal(Entity* entity, Uint16 snd, Uint8 vol);
FMOD::Channel* playSound(Uint16 snd, Uint8 vol);
FMOD::Channel* playSoundNotification(Uint16 snd, Uint8 vol);
FMOD::Channel* playSoundVelocity();

void stopMusic();
void playMusic(FMOD::Sound* sound, bool loop, bool crossfade, bool resume); //Automatically crossfades. NOTE: Resets fadein and fadeout increments to the defaults every time it is called. You'll have to change the fadein and fadeout increments AFTER calling this function.

void handleLevelMusic(); //Manages and updates the level music.

extern float fadein_increment, fadeout_increment, default_fadein_increment, default_fadeout_increment, dynamicAmbientVolume, dynamicEnvironmentVolume;
extern bool sfxUseDynamicAmbientVolume, sfxUseDynamicEnvironmentVolume;

#ifndef EDITOR
class VoiceChat_t
{
    static int packetVoiceDataIdx; // index start of voice data in VOIP packet
	int recording_latency_ms = 50;
	int recordDeviceIndex = 0;
	bool bInit = false;
	int nativeRate = 0;
	int nativeChannels = 1;
	FMOD::Sound* recordingSound = nullptr;
	FMOD::Channel* recordingChannel = nullptr;
	unsigned int recordingSoundLength = 0;
	unsigned int recordingSamples = 0;
	unsigned int recordingAdjustedLatency = 0;
	unsigned int recordingDesiredLatency = 0;
	unsigned int recordingLastPos = 0;
	Uint32 lastRecordTick = 0;
	bool bIsRecording = false;
	std::vector<std::vector<char>> recordingDatagrams;
	Uint32 datagramSequence = 0;
	UDPpacket* loopbackPacket = nullptr;
public:
    enum DSPOrder : int;
    bool mainMenuAudioTabOpen();
    bool allowInputs = false;
    static constexpr float kMaxGain = 10.f;
    static constexpr float kNormalizeFadeTime = 100.f;
    static constexpr float kMaxNormalizeAmp = 100.f;
    static constexpr float kMaxNormalizeThreshold = 1.f;
	bool useSystem = false;
	bool bRecordingInit = false;
	float loopback_input_volume = 0.f;
	float loopback_output_volume = 0.f;
    struct AudioSettings_t
    {
        bool loopback_local_record = false;
        float voice_global_volume = 100.f;
#ifdef NINTENDO
        bool enable_voice_input = false;
        bool enable_voice_receive = false;
#else
        bool enable_voice_input = false;
        bool enable_voice_receive = true;
#endif
        float recordingGain = 100.f;
        bool pushToTalk = true;
        bool use_custom_rolloff = true;
        float recordingNormalizeAmp = 20.f;
        float recordingNormalizeThreshold = 1.0f;
    };
    AudioSettings_t mainmenuSettings;
    AudioSettings_t activeSettings;
    enum AudioSettingBool
    {
        VOICE_SETTING_LOOPBACK_LOCAL_RECORD,
        VOICE_SETTING_ENABLE_VOICE_INPUT,
        VOICE_SETTING_ENABLE_VOICE_RECEIVE,
        VOICE_SETTING_PUSHTOTALK,
        VOICE_SETTING_USE_CUSTOM_ROLLOFF
    };
    enum AudioSettingFloat
    {
        VOICE_SETTING_VOICE_GLOBAL_VOLUME,
        VOICE_SETTING_RECORDINGGAIN,
        VOICE_SETTING_NORMALIZE_AMP,
        VOICE_SETTING_NORMALIZE_THRESHOLD
    };
    bool getAudioSettingBool(AudioSettingBool option);
    float getAudioSettingFloat(AudioSettingFloat option);
    void updateOnMapChange3DRolloff();
#ifdef USE_OPUS
	bool using_encoding = true;
#else
    bool using_encoding = false;
#endif
	bool voiceToggleTalk = false;
	FMOD::ChannelGroup* outChannelGroup = nullptr;
	class PlayerChannels_t
	{
	public:
		std::mutex audio_queue_mutex;
		float channelGain = 100.f;
        float localChannelGain = 100.f;
        float normalize_amp = 20.f;
        float normalize_threshold = 0.1f;
		int talkingTicks = 0;
        int lastAudibleTick = 0;
		int player = -1;
		int drift_ms = 10;
		int native_rate = 0;
		int playback_latency_ms = 150;
		float monitor_input_volume = 0.f;
		float monitor_output_volume = 0.f;
		unsigned int minimumSamplesWritten = -1;
		unsigned int driftThreshold = 0;
		float driftCorrectionPercentage = 0.5f;
		static const size_t audioQueueSizeLimit = 48000;
		std::vector<char> audioQueue;
		int totalSamplesRead = 0;
		int totalSamplesWritten = 0;
		void updateLatency();
		FMOD::Sound* outputSound = nullptr;
		FMOD::Channel* outputChannel = nullptr;
		unsigned int desiredLatency = 0;
		unsigned int adjustedLatency = 0;
		int actualLatency = 0;
		void setupPlayback();
		void deinit();
		std::priority_queue<std::pair<int, std::vector<char>>> voiceDatagrams;
	};

	PlayerChannels_t PlayerChannels[MAXPLAYERS];

	VoiceChat_t();

	void setRecordingDevice(int device_index);
	void init();
	void deinitRecording(bool resetPushTalkToggle = true);
	void initRecording();
	void deinit();
	void updateRecording();
    const char* getVoiceChatBindingName(int player);
    void pushAvailableDatagrams();
	void update();
	void receivePacket(UDPpacket* packet);
	void sendPackets();
    enum VoicePlayerBarState
    {
        VOICE_STATE_NONE,
        VOICE_STATE_INERT,
        VOICE_STATE_MUTE,
        VOICE_STATE_INACTIVE,
        VOICE_STATE_INACTIVE_PTT,
        VOICE_STATE_ACTIVE1,
        VOICE_STATE_ACTIVE2
    };
    VoicePlayerBarState getVoiceState(const int player);

    static const int FRAME_SIZE = 480;
    static const int BITRATE = 24000;
    static void logError(const char* str, ...)
    {
        char newstr[1024] = { 0 };
        va_list argptr;

        // format the content
        va_start(argptr, str);
        vsnprintf(newstr, 1023, str, argptr);
        va_end(argptr);
        printlog("[FMOD Voice Error]: %s", newstr);
    }
    static void logInfo(const char* str, ...)
    {
        char newstr[1024] = { 0 };
        va_list argptr;

        // format the content
        va_start(argptr, str);
        vsnprintf(newstr, 1023, str, argptr);
        va_end(argptr);
        printlog("[FMOD Voice Info]: %s", newstr);
    }
	class RingBuffer
	{
	public:
		RingBuffer(int sizeBytes);
		~RingBuffer();
		int Read(char* dataPtr, int numBytes);
		int Write(char* dataPtr, int numBytes);
		bool Empty(void);
		int GetSize();
		int GetWriteAvail();
		int GetReadAvail();
	private:
		char* _data;
		int _size;
		int _readPtr;
		int _writePtr;
		int _writeBytesAvail;
	};
	static RingBuffer ringBufferRecord;
#ifdef USE_OPUS
    class OpusAudioCodec_t
    {
#ifndef NINTENDO
        OpusEncoder* encoder = nullptr;
        OpusDecoder* decoder[MAXPLAYERS] = { nullptr };
#endif
        bool bInit = false;
    public:
        static void logError(const char* str, ...)
        {
            char newstr[1024] = { 0 };
            va_list argptr;

            // format the content
            va_start(argptr, str);
            vsnprintf(newstr, 1023, str, argptr);
            va_end(argptr);
            printlog("[Opus Error]: %s", newstr);
        }
        static void logInfo(const char* str, ...)
        {
            char newstr[1024] = { 0 };
            va_list argptr;

            // format the content
            va_start(argptr, str);
            vsnprintf(newstr, 1023, str, argptr);
            va_end(argptr);
            printlog("[Opus Info]: %s", newstr);
        }
        int numChannels = 0;
        int sampleRate = 0;
        void init(int sampleRate, int numChannels);

        void deinit()
        {
#ifdef NINTENDO
            nxDeinitOpus();
#else
            if ( encoder )
            {
                opus_encoder_destroy(encoder);
                encoder = nullptr;
            }
            for ( int i = 0; i < MAXPLAYERS; ++i )
            {
                if ( decoder[i] )
                {
                    opus_decoder_destroy(decoder[i]);
                    decoder[i] = nullptr;
                }
            }
#endif
            if ( bInit )
            {
                logInfo("OpusAudioCodec_t::deinit()");
            }
            bInit = false;
        }

        struct encode_rtn
        {
            int frame_size = 0;
            static const int OPUS_MAX_PACKET_SIZE = FRAME_SIZE * 2;
            unsigned char cbits[OPUS_MAX_PACKET_SIZE] = {};
            int numBytes = 0;
            int encoder_id = -1;
        };
        static const int MAX_FRAME_SIZE = 6 * FRAME_SIZE;
        unsigned int encoded_samples = 0;
        double encoding_time = 0.0;
        unsigned int decoded_samples;
        double decoding_time = 0.0;
        double encoding_fetch_time = 0.0;
        unsigned int max_num_bytes_encoded = 0;
    private:
        static encode_rtn encodeFrame(std::vector<opus_int16>& in);
    public:
        encode_rtn encodeFrameSync(const std::vector<char>& data, int frame_size)
        {
            std::vector<opus_int16> in(frame_size * numChannels);
            for ( int i = 0; i < numChannels * frame_size; i++ )
                in[i] = (unsigned char)data[2 * i + 1] << 8 | (unsigned char)data[2 * i];
            return OpusAudioCodec_t::encodeFrame(in);
        }
        int decodeFrame(int which_decoder, encode_rtn& frame_in, std::vector<opus_int16>& out);
    };
#endif
};
extern VoiceChat_t VoiceChat;
#ifdef USE_OPUS
extern VoiceChat_t::OpusAudioCodec_t OpusAudioCodec;
#endif
#endif

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
void* playSound(Uint16, Uint8);
void* playSoundPos(real_t x, real_t y, Uint16, Uint8);
void* playSoundPosLocal(real_t, real_t, Uint16, Uint8);
void* playSoundEntity(Entity*, Uint16, Uint8);
void* playSoundEntityLocal(Entity*, Uint16, Uint8);
void* playSoundPlayer(int, Uint16, Uint8);
void* playSoundNotification(Uint16, Uint8);
void* playSoundNotificationPlayer(int, Uint16, Uint8);
#endif
