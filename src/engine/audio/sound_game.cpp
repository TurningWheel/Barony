/*-------------------------------------------------------------------------------

	BARONY
	File: sound_game.cpp
	Desc: Contains all the code that will cause the editor to crash and burn.
	Quick workaround because I don't want to separate the editor and game
	into two separate projects just because of sound.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../../main.hpp"
#include "../../game.hpp"
#include "../../stat.hpp"
#include "sound.hpp"
#include "../../entity.hpp"
#include "../../net.hpp"
#include "../../player.hpp"
#include "../../ui/GameUI.hpp"

/*-------------------------------------------------------------------------------

	playSoundPlayer

	has the given player play the specified global sound effect. Mostly
	used by the server to instruct clients to play a certain sound.

-------------------------------------------------------------------------------*/
#ifdef USE_FMOD

FMOD::ChannelGroup* getChannelGroupForSoundIndex(Uint32 snd)
{
	if ( snd == 155 || snd == 135 ) // water/lava
	{
		return soundEnvironment_group;
	}
	if ( snd == 149 )
	{
		return soundAmbient_group;
	}
	if ( SkillUpAnimation_t::soundIndexUsedForNotification(snd) )
	{
		return soundNotification_group;
	}
	return sound_group;
}

FMOD::Channel* playSoundPlayer(int player, Uint16 snd, Uint8 vol)
{
	if (no_sound)
	{
		return nullptr;
	}


	if ( player < 0 || player >= MAXPLAYERS )   //Perhaps this can be reprogrammed to remove MAXPLAYERS, and use a pointer to the player instead of an int?
	{
		return nullptr;
	}
	if ( players[player]->isLocalPlayer() )
	{
		return playSound(snd, vol);
	}
	else if ( multiplayer == SERVER )
	{
		if ( client_disconnected[player] || player <= 0 )
		{
			return nullptr;
		}
		memcpy(net_packet->data, "SNDG", 4);
		SDLNet_Write16(snd, &net_packet->data[4]);
		net_packet->data[6] = vol;
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 7;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
		return nullptr;
	}

	return nullptr;
}

FMOD::Channel* playSoundNotificationPlayer(int player, Uint16 snd, Uint8 vol)
{
	if ( no_sound )
	{
		return nullptr;
	}


	if ( player < 0 || player >= MAXPLAYERS )   //Perhaps this can be reprogrammed to remove MAXPLAYERS, and use a pointer to the player instead of an int?
	{
		return nullptr;
	}
	if ( players[player]->isLocalPlayer() )
	{
		return playSoundNotification(snd, vol);
	}
	else if ( multiplayer == SERVER )
	{
		if ( client_disconnected[player] || player <= 0 )
		{
			return nullptr;
		}
		memcpy(net_packet->data, "SNDN", 4);
		SDLNet_Write16(snd, &net_packet->data[4]);
		net_packet->data[6] = vol;
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 7;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
		return nullptr;
	}

	return nullptr;
}

/*-------------------------------------------------------------------------------

	playSoundPos

	plays a sound effect with the given volume at the given
	position; returns the channel that the sound is playing in

-------------------------------------------------------------------------------*/

FMOD::Channel* playSoundPos(real_t x, real_t y, Uint16 snd, Uint8 vol)
{
	auto result = playSoundPosLocal(x, y, snd, vol);

	if (multiplayer == SERVER)
	{
		for (int c = 1; c < MAXPLAYERS; c++)
		{
			if ( client_disconnected[c] == true || players[c]->isLocalPlayer() )
			{
				continue;
			}
			memcpy(net_packet->data, "SNDP", 4);
			SDLNet_Write32(x, &net_packet->data[4]);
			SDLNet_Write32(y, &net_packet->data[8]);
			SDLNet_Write16(snd, &net_packet->data[12]);
			net_packet->data[14] = vol;
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 15;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}

	return result;
}

FMOD::Channel* playSoundPosLocal(real_t x, real_t y, Uint16 snd, Uint8 vol)
{
	if (no_sound)
	{
		return nullptr;
	}

#ifndef SOUND
	return nullptr;
#endif

	FMOD::Channel* channel;

	if (intro)
	{
		return nullptr;
	}
	if (snd < 0 || snd >= numsounds)
	{
		return nullptr;
	}
	if (sounds[snd] == nullptr || vol == 0)
	{
		return nullptr;
	}

	if (!fmod_system)   //For the client.
	{
		return nullptr;
	}

	FMOD_VECTOR position;
	position.x = (float)(x / (real_t)16.0);
	position.y = (float)(0.0);
	position.z = (float)(y / (real_t)16.0);

	if ( soundAmbient_group && getChannelGroupForSoundIndex(snd) == soundAmbient_group )
	{
		int numChannels = 0;
		soundAmbient_group->getNumChannels(&numChannels);
		for ( int i = 0; i < numChannels; ++i )
		{
			FMOD::Channel* c;
			if ( soundAmbient_group->getChannel(i, &c) == FMOD_RESULT::FMOD_OK )
			{
				float audibility = 0.f;
				c->getAudibility(&audibility);
				float volume = 0.f;
				c->getVolume(&volume);
				FMOD_VECTOR playingPosition;
				c->get3DAttributes(&playingPosition, nullptr);
				//printlog("Channel index: %d, audibility: %f, vol: %f, pos x: %.2f | y: %.2f", i, audibility, volume, playingPosition.z, playingPosition.x);
				if ( abs(volume - (vol / 255.f)) < 0.05 )
				{
					if ( sqrt(pow(playingPosition.x - position.x, 2) + pow(playingPosition.z - position.z, 2)) <= 1.5 )
					{
						//printlog("Culling sound due to proximity, pos x: %.2f | y: %.2f", position.z, position.x);
						return nullptr;
					}
				}
			}
		}
	}

	fmod_result = fmod_system->playSound(sounds[snd], getChannelGroupForSoundIndex(snd), true, &channel);
	if (FMODErrorCheck())
	{
		return nullptr;
	}

	channel->setVolume(vol / 255.f);
	channel->set3DAttributes(&position, nullptr);
    channel->setMode(FMOD_3D_WORLDRELATIVE);
	channel->setPaused(false);

	return channel;
}

/*-------------------------------------------------------------------------------

	playSoundEntity

	plays a sound effect with the given volume at the given entity's
	position; returns the channel that the sound is playing in

-------------------------------------------------------------------------------*/

FMOD::Channel* playSoundEntity(Entity* entity, Uint16 snd, Uint8 vol)
{
	if (no_sound)
	{
		return nullptr;
	}

	if ( entity == nullptr )
	{
		return nullptr;
	}
	return playSoundPos(entity->x, entity->y, snd, vol);
}

FMOD::Channel* playSoundEntityLocal(Entity* entity, Uint16 snd, Uint8 vol)
{
	if ( entity == nullptr )
	{
		return nullptr;
	}
	return playSoundPosLocal(entity->x, entity->y, snd, vol);
}

/*-------------------------------------------------------------------------------

	playSound

	plays a sound effect with the given volume and returns the channel that
	the sound is playing in

-------------------------------------------------------------------------------*/

FMOD::Channel* playSound(Uint16 snd, Uint8 vol)
{
	if (no_sound)
	{
		return nullptr;
	}
#ifndef SOUND
	return nullptr;
#endif
	if (!fmod_system || snd < 0 || snd >= numsounds || !getChannelGroupForSoundIndex(snd) )
	{
		return nullptr;
	}
	if (sounds[snd] == nullptr || vol == 0)
	{
		return nullptr;
	}
	FMOD::Channel* channel = nullptr;
	fmod_result = fmod_system->playSound(sounds[snd], getChannelGroupForSoundIndex(snd), true, &channel);
    if (fmod_result == FMOD_OK && channel) {
        //Faux 3D. Set to 0 and then set the channel's mode to be relative  to the player's head to achieve global sound.
        FMOD_VECTOR position;
        position.x = 0.f;
        position.y = 0.f;
        position.z = 0.f;
        
        channel->setVolume(vol / 255.f);
        channel->set3DAttributes(&position, nullptr);
        channel->setMode(FMOD_3D_HEADRELATIVE);
        
        if (FMODErrorCheck())
        {
            return nullptr;
        }
        channel->setPaused(false);
    }
	return channel;
}

FMOD::Channel* playSoundNotification(Uint16 snd, Uint8 vol)
{
	if ( no_sound )
	{
		return nullptr;
	}
#ifndef SOUND
	return nullptr;
#endif
	if ( !fmod_system || snd < 0 || snd >= numsounds || !getChannelGroupForSoundIndex(snd) )
	{
		return nullptr;
	}
	if ( sounds[snd] == nullptr || vol == 0 )
	{
		return nullptr;
	}
	FMOD::Channel* channel;
	fmod_result = fmod_system->playSound(sounds[snd], music_notification_group, true, &channel);
	//Faux 3D. Set to 0 and then set the channel's mode to be relative  to the player's head to achieve global sound.
	FMOD_VECTOR position;
	position.x = 0.f;
	position.y = 0.f;
	position.z = 0.f;

	channel->setVolume(vol / 255.f);
	channel->set3DAttributes(&position, nullptr);
	channel->setMode(FMOD_3D_HEADRELATIVE);

	if ( FMODErrorCheck() )
	{
		return nullptr;
	}
	channel->setPaused(false);
	return channel;
}

#elif defined USE_OPENAL
OPENAL_CHANNELGROUP* getChannelGroupForSoundIndex(Uint32 snd)
{
	if ( snd == 155 || snd == 135 ) // water/lava
	{
		return soundEnvironment_group;
	}
	if ( snd == 149 )
	{
		return soundAmbient_group;
	}
	return sound_group;
}

OPENAL_SOUND* playSoundPlayer(int player, Uint16 snd, Uint8 vol)
{
	if (no_sound)
	{
		return NULL;
	}


	if ( player < 0 || player >= MAXPLAYERS )   //Perhaps this can be reprogrammed to remove MAXPLAYERS, and use a pointer to the player instead of an int?
	{
		return NULL;
	}
	if ( players[player]->isLocalPlayer() )
	{
		return playSound(snd, vol);
	}
	else if ( multiplayer == SERVER )
	{
		if ( client_disconnected[player] || player <= 0 )
		{
			return NULL;
		}
		memcpy(net_packet->data, "SNDG", 4);
		SDLNet_Write16(snd, &net_packet->data[4]);
		net_packet->data[6] = vol;
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 7;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
		return NULL;
	}

	return NULL;
}

OPENAL_SOUND* playSoundNotificationPlayer(int player, Uint16 snd, Uint8 vol)
{
	if (no_sound)
	{
		return NULL;
	}

	if ( player < 0 || player >= MAXPLAYERS )   //Perhaps this can be reprogrammed to remove MAXPLAYERS, and use a pointer to the player instead of an int?
	{
		return nullptr;
	}
	if ( players[player]->isLocalPlayer() )
	{
		return playSoundNotification(snd, vol);
	}
	else if ( multiplayer == SERVER )
	{
		if ( client_disconnected[player] || player <= 0 )
		{
			return nullptr;
		}
		memcpy(net_packet->data, "SNDN", 4);
		SDLNet_Write16(snd, &net_packet->data[4]);
		net_packet->data[6] = vol;
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 7;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
		return nullptr;
	}

	return nullptr;
}

/*-------------------------------------------------------------------------------

	playSoundPos

	plays a sound effect with the given volume at the given
	position; returns the channel that the sound is playing in

-------------------------------------------------------------------------------*/

OPENAL_SOUND* playSoundPos(real_t x, real_t y, Uint16 snd, Uint8 vol)
{
	if (no_sound)
	{
		return NULL;
	}

#ifndef SOUND
	return NULL;
#endif

	OPENAL_SOUND* channel;
	int c;

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

	if (multiplayer == SERVER)
	{
		for (c = 1; c < MAXPLAYERS; c++)
		{
			if ( client_disconnected[c] == true || players[c]->isLocalPlayer() )
			{
				continue;
			}
			memcpy(net_packet->data, "SNDP", 4);
			SDLNet_Write32(x, &net_packet->data[4]);
			SDLNet_Write32(y, &net_packet->data[8]);
			SDLNet_Write16(snd, &net_packet->data[12]);
			net_packet->data[14] = vol;
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 15;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}

	if (!openal_context)   //For the client.
	{
		return NULL;
	}

	channel = OPENAL_CreateChannel(sounds[snd]);
	OPENAL_Channel_SetVolume(channel, vol / 255.f);
	OPENAL_Channel_Set3DAttributes(channel, -y / 16.0, 0, -x / 16.0);
	OPENAL_Channel_SetChannelGroup(channel, getChannelGroupForSoundIndex(snd));
	OPENAL_Channel_Play(channel);

	return channel;
}

OPENAL_SOUND* playSoundPosLocal(real_t x, real_t y, Uint16 snd, Uint8 vol)
{
	if (no_sound)
	{
		return NULL;
	}

#ifndef SOUND
	return NULL;
#endif

	OPENAL_SOUND* channel;

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
	OPENAL_Channel_SetVolume(channel, vol / 255.f);
	OPENAL_Channel_Set3DAttributes(channel, -y / 16.0, 0, -x / 16.0);
	OPENAL_Channel_SetChannelGroup(channel, getChannelGroupForSoundIndex(snd));
	OPENAL_Channel_Play(channel);

	return channel;
}

/*-------------------------------------------------------------------------------

	playSoundEntity

	plays a sound effect with the given volume at the given entity's
	position; returns the channel that the sound is playing in

-------------------------------------------------------------------------------*/

OPENAL_SOUND* playSoundEntity(Entity* entity, Uint16 snd, Uint8 vol)
{
	if (no_sound)
	{
		return NULL;
	}

	if ( entity == NULL )
	{
		return NULL;
	}
	return playSoundPos(entity->x, entity->y, snd, vol);
}

OPENAL_SOUND* playSoundEntityLocal(Entity* entity, Uint16 snd, Uint8 vol)
{
	if ( entity == NULL )
	{
		return NULL;
	}
	return playSoundPosLocal(entity->x, entity->y, snd, vol);
}

/*-------------------------------------------------------------------------------

	playSound

	plays a sound effect with the given volume and returns the channel that
	the sound is playing in

-------------------------------------------------------------------------------*/

OPENAL_SOUND* playSound(Uint16 snd, Uint8 vol)
{
	if (no_sound)
	{
		return NULL;
	}
#ifndef SOUND
	return NULL;
#endif
	if (!openal_context || snd < 0 || snd >= numsounds || !getChannelGroupForSoundIndex(snd) )
	{
		return NULL;
	}
	if (sounds[snd] == NULL || vol == 0)
	{
		return NULL;
	}
	OPENAL_SOUND* channel = OPENAL_CreateChannel(sounds[snd]);
	OPENAL_Channel_SetVolume(channel, vol / 255.f);

	OPENAL_Channel_SetChannelGroup(channel, getChannelGroupForSoundIndex(snd));

	OPENAL_Channel_Play(channel);

	return channel;
}

OPENAL_SOUND* playSoundNotification(Uint16 snd, Uint8 vol)
{
	if (no_sound)
	{
		return NULL;
	}
#ifndef SOUND
	return NULL;
#endif
	if (!openal_context || snd < 0 || snd >= numsounds || !music_notification_group )
	{
		return NULL;
	}
	if (sounds[snd] == NULL || vol == 0)
	{
		return NULL;
	}
	OPENAL_SOUND* channel = OPENAL_CreateChannel(sounds[snd]);
	OPENAL_Channel_SetVolume(channel, vol / 255.f);

	OPENAL_Channel_SetChannelGroup(channel, music_notification_group);

	OPENAL_Channel_Play(channel);

	return channel;
}

void playMusic(OPENAL_BUFFER* sound, bool loop, bool crossfade, bool resume)
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
		OPENAL_BUFFER* lastmusic = NULL;
		OPENAL_GetBuffer(music_channel2, &lastmusic);
		if ( lastmusic == sound )
		{
			OPENAL_SOUND* tempmusic = music_channel;
			music_channel = music_channel2;
			music_channel2 = tempmusic;
		}
		else
		{
			OPENAL_Channel_Stop(music_channel2);
			music_channel2 = music_channel;
			music_channel = OPENAL_CreateChannel(sound);
		}
	}
	else
	{
		OPENAL_Channel_Stop(music_channel2);
		music_channel2 = music_channel;
		music_channel = OPENAL_CreateChannel(sound);
	}
	OPENAL_Channel_SetChannelGroup(music_channel, music_group);
	if (crossfade == true)
	{
		//Start at volume 0 to get louder.
		OPENAL_Channel_SetVolume(music_channel, 0.0f); //Start at 0 then pop up.
	}
	else
	{
		OPENAL_Channel_SetVolume(music_channel, 1.0f);
		OPENAL_Channel_Stop(music_channel2);
		music_channel2 = NULL;
	}
	if (loop == true)
	{
		OPENAL_SetLoop(music_channel, AL_TRUE);
	}
	OPENAL_Channel_Play(music_channel);
}

extern bool shopmusicplaying;
extern bool combatmusicplaying;
extern bool minotaurmusicplaying;
extern bool herxmusicplaying;
extern bool devilmusicplaying;
extern bool olddarkmap;
extern bool sanctummusicplaying;

extern int currenttrack;

#else

/*-------------------------------------------------------------------------------

	playSoundEntity

	plays a sound effect with the given volume at the given entity's
	position; returns the channel that the sound is playing in

-------------------------------------------------------------------------------*/
void* playSound(Uint16 snd, Uint8 vol)
{
	return NULL;
}

void* playSoundPos(real_t x, real_t y, Uint16 snd, Uint8 vol)
{
	int c;

	if (intro || vol == 0)
	{
		return nullptr;
	}

	if (multiplayer == SERVER)
	{
		for (c = 1; c < MAXPLAYERS; c++)
		{
			if ( client_disconnected[c] == true || players[c]->isLocalPlayer() )
			{
				continue;
			}
			memcpy(net_packet->data, "SNDP", 4);
			SDLNet_Write32(x, &net_packet->data[4]);
			SDLNet_Write32(y, &net_packet->data[8]);
			SDLNet_Write16(snd, &net_packet->data[12]);
			net_packet->data[14] = vol;
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 15;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}

	return NULL;
}

void* playSoundPosLocal(real_t x, real_t y, Uint16 snd, Uint8 vol)
{
	return NULL;
}

void* playSoundEntity(Entity* entity, Uint16 snd, Uint8 vol)
{
	if (entity == NULL)
	{
		return NULL;
	}
	return playSoundPos(entity->x, entity->y, snd, vol);
}

void* playSoundEntityLocal(Entity* entity, Uint16 snd, Uint8 vol)
{
	if ( entity == NULL )
	{
		return NULL;
	}
	return playSoundPosLocal(entity->x, entity->y, snd, vol);
}

void* playSoundPlayer(int player, Uint16 snd, Uint8 vol)
{
	int c;

	if ( player < 0 || player >= MAXPLAYERS )   //Perhaps this can be reprogrammed to remove MAXPLAYERS, and use a pointer to the player instead of an int?
	{
		return NULL;
	}
	if ( players[player]->isLocalPlayer() )
	{
		return playSound(snd, vol);
	}
	else if ( multiplayer == SERVER )
	{
		if ( client_disconnected[player] || player <= 0 )
		{
			return NULL;
		}
		memcpy(net_packet->data, "SNDG", 4);
		SDLNet_Write16(snd, &net_packet->data[4]);
		net_packet->data[6] = vol;
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 7;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
		return NULL;
	}

	return NULL;
}

void* playSoundNotification(Uint16 snd, Uint8 vol)
{
	return nullptr;
}

void* playSoundNotificationPlayer(int player, Uint16 snd, Uint8 vol)
{
	if ( no_sound )
	{
		return nullptr;
	}


	if ( player < 0 || player >= MAXPLAYERS )   //Perhaps this can be reprogrammed to remove MAXPLAYERS, and use a pointer to the player instead of an int?
	{
		return nullptr;
	}
	if ( players[player]->isLocalPlayer() )
	{
		return playSoundNotification(snd, vol);
	}
	else if ( multiplayer == SERVER )
	{
		if ( client_disconnected[player] || player <= 0 )
		{
			return nullptr;
		}
		memcpy(net_packet->data, "SNDN", 4);
		SDLNet_Write16(snd, &net_packet->data[4]);
		net_packet->data[6] = vol;
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 7;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
		return nullptr;
	}

	return nullptr;
}
#endif
