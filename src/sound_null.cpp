#include "sound.hpp"

Channel *playSoundPlayer(int, Uint32, int) {return NULL;}
Channel *playSoundPos(real_t, real_t, Uint32) {return NULL;}
Channel* playSoundPosLocal(real_t x, real_t y, Uint32 snd, int vol) {return NULL;}
Channel* playSoundEntity(Entity* entity, Uint32 snd, int vol) {return NULL;}
Channel* playSoundEntityLocal(Entity* entity, Uint32 snd, int vol) {return NULL;}
Channel* playSound(Uint32 snd, int vol) {return NULL;}
Channel* playSoundVelocity() {return NULL;}
Uint32 numsounds = 0;
Sound** sounds = NULL;
SoundSystem *soundSystem = NULL;
ChannelGroup *sound_group = NULL;
bool levelmusicplaying;
void ChannelGroup_SetVolume(ChannelGroup*, float) {}
void initSound() {}
void deinitSound() {}
Sound* createSound(SoundSystem*, const char*) {return NULL;}
void Sound_Release(Sound*) {}
