#include "sound.hpp"

Channel *playSoundPos(real_t, real_t, Uint32) {return NULL;}
Channel* playSoundPosLocal(real_t x, real_t y, Uint32 snd, int vol) {return NULL;}
Channel* playSound(Uint32 snd, int vol) {return NULL;}
Channel* playSoundVelocity() {return NULL;}
Sound* CreateMusic(const char* name) {return NULL;}
void playmusic(Sound* sound, bool loop, bool crossfade, bool resume) {}
unsigned int Channel_GetPosition(Channel*) {return 0;}
void ChannelGroup_Stop(ChannelGroup*) {}
bool Channel_IsPlaying(Channel*) {return false;}
void Channel_Stop(Channel*) {}
unsigned int Sound_GetLength(Sound*) {return 0;}
void sound_update() {}
void ChannelGroup_SetVolume(ChannelGroup*, float) {}
void initSound() {}
void deinitSound() {}
Sound* createSound(const char*) {return NULL;}
void Sound_Release(Sound*) {}
