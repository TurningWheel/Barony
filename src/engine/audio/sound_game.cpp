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
#include "../../ui/MainMenu.hpp"
#include "../../mod_tools.hpp"
#include "../../ui/Button.hpp"

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
	if ( snd == 149 || snd == 133 )
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
	else if ( multiplayer == SERVER && vol > 0 )
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
	else if ( multiplayer == SERVER && vol > 0 )
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

	if (multiplayer == SERVER && vol > 0)
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
					if ( (pow(playingPosition.x - position.x, 2) + pow(playingPosition.z - position.z, 2)) <= 2.25 )
					{
						//printlog("Culling sound due to proximity, pos x: %.2f | y: %.2f", position.z, position.x);
						return nullptr;
					}
				}
			}
		}
	}

	if ( soundEnvironment_group && getChannelGroupForSoundIndex(snd) == soundEnvironment_group )
	{
		int numChannels = 0;
		soundEnvironment_group->getNumChannels(&numChannels);
		for ( int i = 0; i < numChannels; ++i )
		{
			FMOD::Channel* c;
			if ( soundEnvironment_group->getChannel(i, &c) == FMOD_RESULT::FMOD_OK )
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
					if ( (pow(playingPosition.x - position.x, 2) + pow(playingPosition.z - position.z, 2)) <= 4.5 )
					{
						//printlog("Culling sound due to proximity, pos x: %.2f | y: %.2f", position.z, position.x);
						return nullptr;
					}
				}
			}
		}
	}

	/*FMOD_OPENSTATE openState;
	unsigned int percentBuffered = 0;
	bool starving = false;
	bool diskbusy = false;
	sounds[snd]->getOpenState(&openState, &percentBuffered, &starving, &diskbusy);
	printlog("Sound: %d state: %d pc: %d starving: %d diskbusy: %d", snd, openState, percentBuffered, starving, diskbusy);*/
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
	if ( snd == 149 || snd == 133 )
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
	else if ( multiplayer == SERVER && vol > 0 )
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

	if (multiplayer == SERVER && vol > 0 )
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

bool shopmusicplaying = false;
bool combatmusicplaying = false;
bool minotaurmusicplaying = false;
bool herxmusicplaying = false;
bool devilmusicplaying = false;
bool olddarkmap = false;
bool sanctummusicplaying = false;

int currenttrack = -1;

void handleLevelMusic()
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
	bool inshop = false;
	if (players[clientnum] && players[clientnum]->entity)
	{
		int x = (int)players[clientnum]->entity->x / 16;
		int y = (int)players[clientnum]->entity->y / 16;
		if ( x >= 0 && x < map.width && y >= 0 && y < map.height )
			if ( shoparea[y + x * map.height] )
			{
				inshop = true;
			}
	}

	bool devilaround = false;
	bool activeminotaur = false;
	bool herxaround = false;
	bool magisteraround = false;
	node_t* node;
	for ( node = map.creatures->first; node != NULL; node = node->next )
	{
		Entity* entity = (Entity*)node->element;
		if ( entity->sprite == 274 )   // herx head
		{
			herxaround = true;
			break;
		}
		else if ( entity->sprite == 304 )     // devil body
		{
			devilaround = true;
			break;
		}
		else if ( entity->sprite == 239 )     // minotaur head
		{
			activeminotaur = true;
			break;
		}
		else if ( entity->sprite == 646 || entity->sprite == 650 )     // magister body
		{
			magisteraround = true;
			break;
		}
	}

	ALboolean playing = true;
	OPENAL_Channel_IsPlaying(music_channel, &playing);

	if ( currenttrack == -1 )
	{
		currenttrack = local_rng.rand();
	}

	if ( (!levelmusicplaying || !playing || olddarkmap != darkmap) 
		&& (!combat || !strcmp(map.name, "Hell Boss")) 
		&& !inshop 
		&& (!activeminotaur || !strcmp(map.name, "Hell Boss")) && !herxaround && !devilaround && !magisteraround )
	{
		if ( !strncmp(map.name, "The Mines", 9) )     // the mines
		{
			if ( !playing )
			{
				currenttrack = 1 + local_rng.rand() % (NUMMINESMUSIC - 1);
			}
			currenttrack = currenttrack % NUMMINESMUSIC;
			if ( currenttrack == 0 )
			{
				currenttrack = 1;
			}
			playMusic(minesmusic[currenttrack], false, true, true);
		}
		else if ( !strncmp(map.name, "The Swamp", 9) )     // the swamp
		{
			if ( !playing )
			{
				currenttrack = 1 + local_rng.rand() % (NUMSWAMPMUSIC - 1);
			}
			currenttrack = currenttrack % NUMSWAMPMUSIC;
			if ( currenttrack == 0 )
			{
				currenttrack = 1;
			}
			playMusic(swampmusic[currenttrack], false, true, true);
		}
		else if ( !strncmp(map.name, "The Labyrinth", 13) )     // the labyrinth
		{
			if ( !playing )
			{
				currenttrack = 1 + local_rng.rand() % (NUMLABYRINTHMUSIC - 1);
			}
			currenttrack = currenttrack % NUMLABYRINTHMUSIC;
			if ( currenttrack == 0 )
			{
				currenttrack = 1;
			}
			playMusic(labyrinthmusic[currenttrack], false, true, true);
		}
		else if ( !strncmp(map.name, "The Ruins", 9) )     // the ruins
		{
			if ( !playing )
			{
				currenttrack = 1 + local_rng.rand() % (NUMRUINSMUSIC - 1);
			}
			currenttrack = currenttrack % NUMRUINSMUSIC;
			if ( currenttrack == 0 )
			{
				currenttrack = 1;
			}
			playMusic(ruinsmusic[currenttrack], false, true, true);
		}
		else if ( !strncmp(map.name, "Underworld", 10) )     // the underworld
		{
			if ( !playing )
			{
				currenttrack = 1 + local_rng.rand() % (NUMUNDERWORLDMUSIC - 1);
			}
			currenttrack = currenttrack % NUMUNDERWORLDMUSIC;
			if ( currenttrack == 0 )
			{
				currenttrack = 1;
			}
			playMusic(underworldmusic[currenttrack], false, true, true);
		}
		else if ( !strcmp(map.name, "Minetown") )     // minetown
		{
			playMusic(minetownmusic, true, true, true);
		}
		else if ( !strcmp(map.name, "The Gnomish Mines") )
		{
			if ( gnomishminesmusic )
			{
				playMusic(gnomishminesmusic, true, true, true);
			}
			else
			{
				playMusic(minetownmusic, true, true, true);
			}
		}
		else if ( !strcmp(map.name, "The Haunted Castle") )
		{
			if ( greatcastlemusic )
			{
				playMusic(greatcastlemusic, true, true, true);
			}
			else
			{
				playMusic(intermissionmusic, true, true, true);
			}
		}
		else if ( !strcmp(map.name, "Sokoban") )
		{
			if ( sokobanmusic )
			{
				playMusic(sokobanmusic, true, true, true);
			}
			else
			{
				playMusic(intermissionmusic, true, true, true);
			}
		}
		else if ( !strcmp(map.name, "Cockatrice Lair") )
		{
			if ( caveslairmusic )
			{
				playMusic(caveslairmusic, true, true, true);
			}
			else
			{
				playMusic(cavesmusic[2], true, true, true);
			}
		}
		else if ( !strcmp(map.name, "Bram's Castle") )
		{
			if ( bramscastlemusic )
			{
				playMusic(bramscastlemusic, true, true, true);
			}
			else
			{
				playMusic(citadelmusic[2], true, true, true);
			}
		}
		else if ( !strcmp(map.name, "The Mystic Library") )     // mystic library
		{
			playMusic(librarymusic, true, true, true);
		}
		else if ( !strcmp(map.name, "The Minotaur Maze") )     // minotaur maze
		{
			playMusic(minotaurmusic[1], true, true, true);
		}
		else if ( !strcmp(map.name, "The Temple") )     // the temple
		{
			playMusic(templemusic, true, true, true);
		}
		else if ( !strcmp(map.name, "Hell Boss") )     // escape theme
		{
			playMusic(escapemusic, true, true, true);
		}
		else if ( !strncmp(map.name, "Hell", 4) )     // hell
		{
			if ( !playing )
			{
				currenttrack = 1 + local_rng.rand() % (NUMHELLMUSIC - 1);
			}
			currenttrack = currenttrack % NUMHELLMUSIC;
			if ( currenttrack == 0 )
			{
				currenttrack = 1;
			}
			playMusic(hellmusic[currenttrack], false, true, true);
		}
		else if ( !strncmp(map.name, "Caves", 5) )
		{
			if ( !playing )
			{
				currenttrack = 1 + local_rng.rand() % (NUMCAVESMUSIC - 1);
			}
			currenttrack = currenttrack % NUMCAVESMUSIC;
			if ( currenttrack == 0 )
			{
				currenttrack = 1;
			}
			playMusic(cavesmusic[currenttrack], false, true, true);
		}
		else if ( !strncmp(map.name, "Citadel", 7) || !strncmp(map.name, "Sanctum", 7) )
		{
			if ( !playing )
			{
				currenttrack = 1 + local_rng.rand() % (NUMCITADELMUSIC - 1);
			}
			currenttrack = currenttrack % NUMCITADELMUSIC;
			if ( currenttrack == 0 )
			{
				currenttrack = 1;
			}
			playMusic(citadelmusic[currenttrack], false, true, true);
		}
		else if ( !strcmp(map.name, "Mages Guild") )
		{
			if ( hamletmusic )
			{
				playMusic(hamletmusic, true, true, true);
			}
			else
			{
				playMusic(minesmusic[4], true, true, true);
			}
		}
		else
		{
			playMusic(intermissionmusic, true, true, true);
		}
		olddarkmap = darkmap;
		levelmusicplaying = true;
		devilmusicplaying = false;
		herxmusicplaying = false;
		minotaurmusicplaying = false;
		combatmusicplaying = false;
		shopmusicplaying = false;
		fadein_increment = default_fadein_increment;
		fadeout_increment = default_fadeout_increment;
	}
	else if ( (!devilmusicplaying || !playing) && devilaround )
	{
		playMusic(devilmusic, true, true, true);
		levelmusicplaying = false;
		devilmusicplaying = true;
		herxmusicplaying = false;
		minotaurmusicplaying = false;
		combatmusicplaying = false;
		shopmusicplaying = false;
		fadein_increment = default_fadein_increment * 2;
		fadeout_increment = default_fadeout_increment * 2;
	}
	else if ( (!herxmusicplaying || !playing) && !devilaround && herxaround )
	{
		playMusic(herxmusic, true, true, true);
		levelmusicplaying = false;
		devilmusicplaying = false;
		herxmusicplaying = true;
		minotaurmusicplaying = false;
		combatmusicplaying = false;
		shopmusicplaying = false;
		fadein_increment = default_fadein_increment * 2;
		fadeout_increment = default_fadeout_increment * 2;
	}
	else if ( (!minotaurmusicplaying || !playing) && !herxaround && activeminotaur && strcmp(map.name, "Hell Boss") )
	{
		playMusic(minotaurmusic[0], true, true, true);
		levelmusicplaying = false;
		devilmusicplaying = false;
		herxmusicplaying = false;
		minotaurmusicplaying = true;
		combatmusicplaying = false;
		shopmusicplaying = false;
		fadein_increment = default_fadein_increment * 5;
		fadeout_increment = default_fadeout_increment * 5;
	}
	else if ( (!sanctummusicplaying || !playing) && magisteraround )
	{
		playMusic(sanctummusic, true, true, true);
		levelmusicplaying = false;
		devilmusicplaying = false;
		herxmusicplaying = false;
		minotaurmusicplaying = false;
		combatmusicplaying = false;
		sanctummusicplaying = true;
		shopmusicplaying = false;
		fadein_increment = default_fadein_increment * 2;
		fadeout_increment = default_fadeout_increment * 2;
	}
	else if ( (!combatmusicplaying || !playing) 
		&& !herxaround 
		&& !activeminotaur 
		&& combat 
		&& strcmp(map.name, "Hell Boss")
		&& strcmp(map.name, "Sanctum") )
	{
		if ( !strncmp(map.name, "The Swamp", 9) || !strncmp(map.name, "The Temple", 10) )   // the swamp
		{
			playMusic(swampmusic[0], true, true, true);
		}
		else if ( !strncmp(map.name, "The Labyrinth", 13) || strstr(map.name, "Minotaur") )     // the labyrinth
		{
			playMusic(labyrinthmusic[0], true, true, true);
		}
		else if ( !strncmp(map.name, "The Ruins", 9) )     // the ruins
		{
			playMusic(ruinsmusic[0], true, true, true);
		}
		else if ( !strncmp(map.name, "Underworld", 10) )     // the underworld
		{
			playMusic(underworldmusic[0], true, true, true);
		}
		else if ( !strncmp(map.name, "Hell", 4) )     // hell
		{
			playMusic(hellmusic[0], true, true, true);
		}
		else if ( !strncmp(map.name, "Caves", 5) || !strcmp(map.name, "Cockatrice Lair") )
		{
			playMusic(cavesmusic[0], true, true, true);
		}
		else if ( !strncmp(map.name, "Citadel", 7) || !strcmp(map.name, "Bram's Castle") )
		{
			playMusic(citadelmusic[0], true, true, true);
		}
		else
		{
			playMusic(minesmusic[0], true, true, true);
		}
		levelmusicplaying = false;
		devilmusicplaying = false;
		herxmusicplaying = false;
		combatmusicplaying = true;
		shopmusicplaying = false;
		minotaurmusicplaying = false;
		fadein_increment = default_fadein_increment * 4;
		fadeout_increment = default_fadeout_increment;
	}
	else if ( (!shopmusicplaying || !playing) && !herxaround && !activeminotaur && !combat && inshop )
	{
		playMusic(shopmusic, true, true, true);
		levelmusicplaying = false;
		devilmusicplaying = false;
		herxmusicplaying = false;
		minotaurmusicplaying = false;
		combatmusicplaying = false;
		shopmusicplaying = true;
		fadein_increment = default_fadein_increment * 4;
		fadeout_increment = default_fadeout_increment;
	}
}


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

#ifdef USE_FMOD
VoiceChat_t VoiceChat;
VoiceChat_t::RingBuffer VoiceChat_t::ringBufferRecord(2048 * 20);
static ConsoleCommand ccmd_voice_init("/voice_init", "", 
	[](int argc, const char* argv[]) {
		VoiceChat.init();
});
static ConsoleCommand ccmd_voice_deinit("/voice_deinit", "",
	[](int argc, const char* argv[]) {
		VoiceChat.deinit();
	});

FMOD_RESULT F_CALLBACK pcmreadcallback(FMOD_SOUND* sound, void* data, unsigned int datalen)
{
	char* bitbuffer = (char*)data;
	if ( !sound ) { return FMOD_OK; }
	void* userData = nullptr;
	fmod_result = ((FMOD::Sound*)sound)->getUserData(&userData);
	if ( FMODErrorCheck() ) { return FMOD_OK; }
	int player = reinterpret_cast<intptr_t>(userData);
	
	if ( !(player >= 0 && player < MAXPLAYERS) )
	{
		return FMOD_OK;
	}

	VoiceChat.PlayerChannels[player].audio_queue_mutex.lock();
    
	auto& audioQueue = VoiceChat.PlayerChannels[player].audioQueue;
	unsigned int bytesRead = std::min(datalen, (unsigned int)audioQueue.size());

	memcpy(bitbuffer, audioQueue.data(), bytesRead);
	audioQueue.erase(audioQueue.begin(), audioQueue.begin() + bytesRead);
	VoiceChat.PlayerChannels[player].totalSamplesRead += bytesRead;
	VoiceChat.PlayerChannels[player].updateLatency();

	VoiceChat.PlayerChannels[player].audio_queue_mutex.unlock();

	for ( unsigned int count = bytesRead; count < datalen; ++count )
	{
		bitbuffer[count] = 0;
	}
	return FMOD_OK;
}

void VoiceChat_t::PlayerChannels_t::updateLatency()
{
	int latency = (int)totalSamplesWritten - (int)totalSamplesRead;
	actualLatency = (int)((0.93f * actualLatency) + (0.03f * latency));

	if ( outputChannel )
	{
		int playbackRate = native_rate;
		if ( actualLatency < (int)(adjustedLatency - driftThreshold) )
		{
			playbackRate = native_rate - (int)(native_rate * (driftCorrectionPercentage / 100.0f));
		}
		else if ( actualLatency > (int)(adjustedLatency + driftThreshold) )
		{
			playbackRate = native_rate + (int)(native_rate * (driftCorrectionPercentage / 100.0f));
		}
		outputChannel->setFrequency(playbackRate);
	}
}

VoiceChat_t::VoicePlayerBarState VoiceChat_t::getVoiceState(const int player)
{
	int voice_no_send = GameplayPreferences_t::getGameConfigValue(GameplayPreferences_t::GOPT_VOICE_NO_SEND);
	int voice_pushtotalk = GameplayPreferences_t::getGameConfigValue(GameplayPreferences_t::GOPT_VOICE_PTT);
	auto result = VOICE_STATE_NONE;
	if ( !(voice_no_send & (1 << player)) )
	{
		result = VOICE_STATE_INERT; //(voice_pushtotalk & (1 << player)) ? VOICE_STATE_INERT : VOICE_STATE_MUTE;
		if ( VoiceChat.PlayerChannels[player].talkingTicks > 0 
			|| VoiceChat.PlayerChannels[player].monitor_output_volume >= 0.05
			|| VoiceChat.PlayerChannels[player].lastAudibleTick > 0 )
		{
			if ( VoiceChat.PlayerChannels[player].monitor_output_volume < 0.05 )
			{
				if ( (voice_pushtotalk & (1 << player)) )
				{
					result = VOICE_STATE_INACTIVE_PTT;
				}
				else
				{
					if ( VoiceChat.PlayerChannels[player].lastAudibleTick > 0 )
					{
						result = VOICE_STATE_INACTIVE_PTT;
					}
					else
					{
						result = VOICE_STATE_INACTIVE;
					}
				}
			}
			else
			{
				result = (ticks % 50) > 25 ? VOICE_STATE_ACTIVE1 : VOICE_STATE_ACTIVE2;
			}
		}
	}
	return result;
}

VoiceChat_t::VoiceChat_t()
{
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		PlayerChannels[i].player = i;
		PlayerChannels[i].audioQueue.reserve(PlayerChannels_t::audioQueueSizeLimit);
	}
}

#ifdef USE_OPUS
VoiceChat_t::OpusAudioCodec_t OpusAudioCodec;
VoiceChat_t::OpusAudioCodec_t::encode_rtn VoiceChat_t::OpusAudioCodec_t::encodeFrame(std::vector<opus_int16>& in)
{
	encode_rtn data;
#ifndef NINTENDO
	if ( !OpusAudioCodec.bInit || !OpusAudioCodec.encoder )
	{
		return data;
	}
	auto t1 = std::chrono::high_resolution_clock::now();
	data.numBytes = opus_encode(OpusAudioCodec.encoder, in.data(), in.size(), data.cbits, encode_rtn::OPUS_MAX_PACKET_SIZE);
	auto t2 = std::chrono::high_resolution_clock::now();
	++OpusAudioCodec.encoded_samples;
	OpusAudioCodec.encoding_time += 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
	if ( data.numBytes > 0 )
	{
		OpusAudioCodec.max_num_bytes_encoded = std::max(OpusAudioCodec.max_num_bytes_encoded, (unsigned int)data.numBytes);
	}
#endif
	return data;
}

int VoiceChat_t::OpusAudioCodec_t::decodeFrame(int which_decoder, encode_rtn& frame_in, std::vector<opus_int16>& out)
{
	if ( !bInit ) { return 0; }

	auto t3 = std::chrono::high_resolution_clock::now();
#ifdef NINTENDO
	int frame_size = nxDecodeFrame(which_decoder, &frame_in, &out);
#else
	if ( !decoder[which_decoder] ) { return 0; }
	int frame_size = opus_decode(decoder[which_decoder], frame_in.cbits, frame_in.numBytes, out.data(), MAX_FRAME_SIZE, 0);
#endif
	auto t4 = std::chrono::high_resolution_clock::now();
	++decoded_samples;
	decoding_time += 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t4 - t3).count();
	return frame_size;
}

void VoiceChat_t::OpusAudioCodec_t::init(int sampleRate, int numChannels)
{
	deinit();

	this->sampleRate = sampleRate;
	this->numChannels = numChannels;
	
#ifdef NINTENDO
	if ( !nxInitOpus(sampleRate, numChannels) )
	{
		deinit();
		return;
	}
#else
	int opus_err;
	encoder = opus_encoder_create(sampleRate, numChannels, OPUS_APPLICATION_VOIP, &opus_err);
	if ( opus_err < 0 )
	{
		logError("opus_encoder_create failed, error: %d (%s)", opus_err, opus_strerror(opus_err));
		deinit();
		return;
	}
	logInfo("encoder created successfully, sample rate: %d, channels: %d", sampleRate, numChannels);

	opus_err = opus_encoder_ctl(encoder, OPUS_SET_BITRATE(BITRATE));
	if ( opus_err < 0 )
	{
		logError("opus_encoder_ctl failed, error: %d (%s)", opus_err, opus_strerror(opus_err));
		deinit();
		return;
	}
	logInfo("opus_encoder_ctl set bitrate to: %d", BITRATE);

	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		decoder[i] = opus_decoder_create(sampleRate, numChannels, &opus_err);
		if ( opus_err < 0 )
		{
			logError("opus_decoder_create failed, error: %d (%s)", opus_err, opus_strerror(opus_err));
			deinit();
			return;
		}
	}
	logInfo("decoder created successfully, sample rate: %d, channels: %d", sampleRate, numChannels);
#endif
	bInit = true;
}
#endif

void VoiceChat_t::init()
{
	if ( !useSystem ) { return; }
	if ( bInit ) { return; }

	nativeChannels = 1;
	nativeRate = BITRATE;

	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		PlayerChannels[i].native_rate = nativeRate;
		PlayerChannels[i].driftThreshold = (nativeRate * PlayerChannels[i].drift_ms) / 1000;       /* The point where we start compensating for drift */
		PlayerChannels[i].desiredLatency = (nativeRate * PlayerChannels[i].playback_latency_ms) / 1000;     /* User specified latency */
		PlayerChannels[i].adjustedLatency = PlayerChannels[i].desiredLatency;	 /* User specified latency adjusted for driver update granularity */
		PlayerChannels[i].actualLatency = PlayerChannels[i].desiredLatency;
	}

	recordingDesiredLatency = (nativeRate * recording_latency_ms) / 1000;
	recordingAdjustedLatency = (nativeRate * recording_latency_ms) / 1000;


	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		PlayerChannels[i].deinit();
	}
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		PlayerChannels[i].setupPlayback();
	}

	if ( !loopbackPacket )
	{
		loopbackPacket = SDLNet_AllocPacket(NET_PACKET_SIZE);
	}

#ifdef USE_OPUS
	OpusAudioCodec.init(nativeRate, nativeChannels);
#endif

	bInit = true;
	logInfo("VoiceChat_t::init()");
}

void VoiceChat_t::setRecordingDevice(int device_index)
{
	deinitRecording(intro);
	recordDeviceIndex = device_index;
}

void VoiceChat_t::deinitRecording(bool resetPushTalkToggle)
{
#ifdef NINTENDO
	return;
#endif
	lastRecordTick = 0;
	recordingLastPos = 0;
	bIsRecording = false;
	bRecordingInit = false;
	fmod_result = fmod_system->recordStop(recordDeviceIndex);
	if ( recordingChannel )
	{
		fmod_result = recordingChannel->stop();
		recordingChannel = nullptr;
	}
	if ( recordingSound )
	{
		recordingSound->release();
		recordingSound = nullptr;
	}
	recordingSamples = 0;
	if ( resetPushTalkToggle )
	{
		voiceToggleTalk = false;
	}
	logInfo("VoiceChat_t::deinitRecording()");
}

void VoiceChat_t::initRecording()
{
	if ( initialized && !loading )
	{
		if ( recordingChannel )
		{
			fmod_result = recordingChannel->stop();
			recordingChannel = nullptr;
		}
		if ( recordingSound )
		{
			recordingSound->release();
			recordingSound = nullptr;
		}

		FMOD_CREATESOUNDEXINFO exinfo = { 0 };
		exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
		exinfo.numchannels = nativeChannels;
		exinfo.format = FMOD_SOUND_FORMAT_PCM16;
		exinfo.defaultfrequency = nativeRate;
		exinfo.length = nativeRate * sizeof(short) * nativeChannels; // 1 second buffer
		fmod_result = fmod_system->createSound(0, FMOD_LOOP_NORMAL | FMOD_OPENUSER, &exinfo, &recordingSound);
		if ( FMODErrorCheck() ) { return; }
		fmod_result = recordingSound->getLength(&recordingSoundLength, FMOD_TIMEUNIT_PCM);
		if ( FMODErrorCheck() ) { return; }

		int numDrivers = 0;
		fmod_result = fmod_system->getRecordNumDrivers(NULL, &numDrivers);
		if ( FMODErrorCheck() || numDrivers == 0 ) { return; }
		//fmod_result = fmod_system->getRecordDriverInfo(recordDeviceIndex, NULL, 0, NULL, &nativeRate, NULL, &nativeChannels, NULL);
		//if ( FMODErrorCheck() ) { return; }
		fmod_result = fmod_system->recordStart(recordDeviceIndex, recordingSound, true);
		if ( FMODErrorCheck() ) { return; }
		bRecordingInit = true;
		logInfo("VoiceChat_t::initRecording()");
	}
}

void VoiceChat_t::PlayerChannels_t::deinit()
{
	audio_queue_mutex.lock();
	if ( outputChannel )
	{
		outputChannel->stop();
		outputChannel = nullptr;
	}
	if ( outputSound )
	{
		outputSound->release();
		outputSound = nullptr;
	}

	if ( VoiceChat.outChannelGroup )
	{
		VoiceChat.outChannelGroup->release();
		VoiceChat.outChannelGroup = nullptr;
	}

	minimumSamplesWritten = -1;

	audio_queue_mutex.unlock();
	logInfo("VoiceChat_t::PlayerChannels_t::deinit()");
}

FMOD_REVERB_PROPERTIES reverbProperties;
void setReverbType(int type)
{
	switch ( type )
	{
	case 0:
		reverbProperties = FMOD_PRESET_OFF;
		break;
	case 1:
		reverbProperties = FMOD_PRESET_GENERIC;
		break;
	case 2:
		reverbProperties = FMOD_PRESET_PADDEDCELL;
		break;
	case 3:
		reverbProperties = FMOD_PRESET_ROOM;
		break;
	case 4:
		reverbProperties = FMOD_PRESET_BATHROOM;
		break;
	case 5:
		reverbProperties = FMOD_PRESET_LIVINGROOM;
		break;
	case 6:
		reverbProperties = FMOD_PRESET_STONEROOM;
		break;
	case 7:
		reverbProperties = FMOD_PRESET_AUDITORIUM;
		break;
	case 8:
		reverbProperties = FMOD_PRESET_CONCERTHALL;
		break;
	case 9:
		reverbProperties = FMOD_PRESET_CAVE;
		break;
	case 10:
		reverbProperties = FMOD_PRESET_ARENA;
		break;
	case 11:
		reverbProperties = FMOD_PRESET_HANGAR;
		break;
	case 12:
		reverbProperties = FMOD_PRESET_CARPETTEDHALLWAY;
		break;
	case 13:
		reverbProperties = FMOD_PRESET_HALLWAY;
		break;
	case 14:
		reverbProperties = FMOD_PRESET_STONECORRIDOR;
		break;
	case 15:
		reverbProperties = FMOD_PRESET_ALLEY;
		break;
	case 16:
		reverbProperties = FMOD_PRESET_FOREST;
		break;
	case 17:
		reverbProperties = FMOD_PRESET_CITY;
		break;
	case 18:
		reverbProperties = FMOD_PRESET_MOUNTAINS;
		break;
	case 19:
		reverbProperties = FMOD_PRESET_QUARRY;
		break;
	case 20:
		reverbProperties = FMOD_PRESET_PLAIN;
		break;
	case 21:
		reverbProperties = FMOD_PRESET_PARKINGLOT;
		break;
	case 22:
		reverbProperties = FMOD_PRESET_SEWERPIPE;
		break;
	case 23:
		reverbProperties = FMOD_PRESET_UNDERWATER;
		break;
	default:
		reverbProperties = FMOD_PRESET_OFF;
		break;
	}
}

void setReverbSettings(FMOD::DSP* dsp_reverb, int type, float wetLevel, float dryLevel)
{
	if ( !dsp_reverb ) { return; }
	setReverbType(type);
	dsp_reverb->setParameterFloat(FMOD_DSP_SFXREVERB_DECAYTIME, reverbProperties.DecayTime);
	dsp_reverb->setParameterFloat(FMOD_DSP_SFXREVERB_EARLYDELAY, reverbProperties.EarlyDelay);
	dsp_reverb->setParameterFloat(FMOD_DSP_SFXREVERB_LATEDELAY, reverbProperties.LateDelay);
	dsp_reverb->setParameterFloat(FMOD_DSP_SFXREVERB_HFREFERENCE, reverbProperties.HFReference);
	dsp_reverb->setParameterFloat(FMOD_DSP_SFXREVERB_HFDECAYRATIO, reverbProperties.HFDecayRatio);
	dsp_reverb->setParameterFloat(FMOD_DSP_SFXREVERB_DIFFUSION, reverbProperties.Diffusion);
	dsp_reverb->setParameterFloat(FMOD_DSP_SFXREVERB_DENSITY, reverbProperties.Density);
	dsp_reverb->setParameterFloat(FMOD_DSP_SFXREVERB_LOWSHELFFREQUENCY, reverbProperties.LowShelfFrequency);
	dsp_reverb->setParameterFloat(FMOD_DSP_SFXREVERB_LOWSHELFGAIN, reverbProperties.LowShelfGain);
	dsp_reverb->setParameterFloat(FMOD_DSP_SFXREVERB_HIGHCUT, reverbProperties.HighCut);
	dsp_reverb->setParameterFloat(FMOD_DSP_SFXREVERB_EARLYLATEMIX, reverbProperties.EarlyLateMix);
	dsp_reverb->setParameterFloat(FMOD_DSP_SFXREVERB_WETLEVEL, reverbProperties.WetLevel + wetLevel);
	dsp_reverb->setParameterFloat(FMOD_DSP_SFXREVERB_DRYLEVEL, 0.f + dryLevel);
	if ( type == 0 )
	{
		dsp_reverb->setBypass(true);
	}
	else
	{
		dsp_reverb->setBypass(false);
	}
}

void setEQSettings(FMOD::DSP* dsp_eq, float low, float mid, float high)
{
	if ( !dsp_eq ) { return; }
	dsp_eq->setParameterFloat(FMOD_DSP_THREE_EQ_LOWGAIN, low);
	dsp_eq->setParameterFloat(FMOD_DSP_THREE_EQ_MIDGAIN, mid);
	dsp_eq->setParameterFloat(FMOD_DSP_THREE_EQ_HIGHGAIN, high);
}

enum VoiceChat_t::DSPOrder : int
{
	DSPORDER_LIMITER = 1,
	DSPORDER_REVERB,
	DSPORDER_EQ,
	DSPORDER_FADER_LOCALGAIN,
	DSPORDER_FADER_CHANNELGAIN,
	DSPORDER_NORMALIZE,
	DSPORDER_END
};

static ConsoleCommand ccmd_voice_reverb("/voice_reverb", "",
	[](int argc, const char* argv[]) {
		if ( argc > 3 )
		{
			static int reverbType = -1;
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( VoiceChat.PlayerChannels[i].outputChannel )
				{
					FMOD::DSP* dsp_reverb = nullptr;
					fmod_result = VoiceChat.PlayerChannels[i].outputChannel->getDSP(VoiceChat_t::DSPORDER_REVERB, &dsp_reverb);
					if ( dsp_reverb )
					{
						FMOD_DSP_TYPE type;
						fmod_result = dsp_reverb->getType(&type);
						if ( type == FMOD_DSP_TYPE_SFXREVERB )
						{
							if ( atoi(argv[1]) < 0 )
							{
								++reverbType;
								if ( reverbType >= 24 )
								{
									reverbType = 0;
								}
								setReverbSettings(dsp_reverb, reverbType, atoi(argv[2]), atoi(argv[3]));
							}
							else
							{
								setReverbSettings(dsp_reverb, atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
							}
						}
					}
				}
			}
		}
	});

static ConsoleCommand ccmd_voice_eq("/voice_eq", "",
	[](int argc, const char* argv[]) {
		if ( argc > 3 )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( VoiceChat.PlayerChannels[i].outputChannel )
				{
					FMOD::DSP* dsp_eq = nullptr;
					fmod_result = VoiceChat.PlayerChannels[i].outputChannel->getDSP(VoiceChat_t::DSPORDER_EQ, &dsp_eq);
					if ( dsp_eq )
					{
						FMOD_DSP_TYPE type;
						fmod_result = dsp_eq->getType(&type);
						if ( type == FMOD_DSP_TYPE_THREE_EQ )
						{
							setEQSettings(dsp_eq, atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
						}
					}
				}
			}
		}
	});

static ConsoleCommand ccmd_voice_limiter_release("/voice_limiter_release", "",
	[](int argc, const char* argv[]) {
		if ( argc > 1 )
		{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( VoiceChat.PlayerChannels[i].outputChannel )
				{
					FMOD::DSP* dsp_limiter = nullptr;
					fmod_result = VoiceChat.PlayerChannels[i].outputChannel->getDSP(VoiceChat_t::DSPORDER_LIMITER, &dsp_limiter);
					if ( dsp_limiter )
					{
						FMOD_DSP_TYPE type;
						fmod_result = dsp_limiter->getType(&type);
						if ( type == FMOD_DSP_TYPE_LIMITER )
						{
							fmod_result = dsp_limiter->setParameterFloat(FMOD_DSP_LIMITER_RELEASETIME, atoi(argv[1]));
						}
					}
				}
			}
		}
	});

//void setPitchShiftSettings(FMOD::DSP* dsp, float pitch)
//{
//	if ( !dsp ) { return; }
//	dsp->setBypass(false);
//	dsp->setParameterFloat(FMOD_DSP_PITCHSHIFT_PITCH, pitch);
//}
//static ConsoleCommand ccmd_voice_pitchshift("/voice_pitchshift", "",
//	[](int argc, const char* argv[]) {
//		if ( argc > 1 )
//		{
//			for ( int i = 0; i < MAXPLAYERS; ++i )
//			{
//				if ( VoiceChat.PlayerChannels[i].outputChannel )
//				{
//					FMOD::DSP* dsp = nullptr;
//					fmod_result = VoiceChat.PlayerChannels[i].outputChannel->getDSP(4, &dsp);
//					if ( dsp )
//					{
//						FMOD_DSP_TYPE type;
//						fmod_result = dsp->getType(&type);
//						if ( type == FMOD_DSP_TYPE_PITCHSHIFT )
//						{
//							setPitchShiftSettings(dsp, atoi(argv[1]));
//						}
//					}
//				}
//			}
//		}
//	});
//void setChorusSettings(FMOD::DSP* dsp, float mix, float rate, float depth)
//{
//	if ( !dsp ) { return; }
//	dsp->setBypass(false);
//	dsp->setParameterFloat(FMOD_DSP_CHORUS_MIX, mix);
//	dsp->setParameterFloat(FMOD_DSP_CHORUS_RATE, rate);
//	dsp->setParameterFloat(FMOD_DSP_CHORUS_DEPTH, depth);
//}
//static ConsoleCommand ccmd_voice_chorus("/voice_chorus", "",
//	[](int argc, const char* argv[]) {
//		if ( argc > 3 )
//		{
//			for ( int i = 0; i < MAXPLAYERS; ++i )
//			{
//				if ( VoiceChat.PlayerChannels[i].outputChannel )
//				{
//					FMOD::DSP* dsp = nullptr;
//					fmod_result = VoiceChat.PlayerChannels[i].outputChannel->getDSP(5, &dsp);
//					if ( dsp )
//					{
//						FMOD_DSP_TYPE type;
//						fmod_result = dsp->getType(&type);
//						if ( type == FMOD_DSP_TYPE_CHORUS )
//						{
//							setChorusSettings(dsp, atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
//						}
//					}
//				}
//			}
//		}
//	});

static ConsoleVariable<Vector4> cvar_voice_reverb_player("/voice_reverb_player", Vector4{ 0.f, 0.f, 0.f, 0.f });
static ConsoleVariable<Vector4> cvar_voice_reverb_ghost("/voice_reverb_ghost", Vector4{ 9.f, -3.f, 0.f, 0.f });
static ConsoleVariable<Vector4> cvar_voice_eq_player("/voice_eq_player", Vector4{ -3.f, 0.f, 0.f, 0.f });
static ConsoleVariable<Vector4> cvar_voice_eq_ghost("/voice_eq_ghost", Vector4{ -6.f, -3.f, 0.f, 0.f });
static FMOD_VECTOR fmod_rolloff_points[6] = {
	{0.f, 1.f, 0.f}, // 0 dist = 1.f vol
	{2.f, 1.f, 0.f},
	{6.f, 0.8f, 0.f},
	{14.f, 0.6f, 0.f},
	{30.f, 0.4f, 0.f},
	{64.f, 0.2f, 0.f}
};
static FMOD_VECTOR fmod_rolloff_points_arena[6] = {
	{0.f, 1.f, 0.f}, // 0 dist = 1.f vol
	{2.f, 1.f, 0.f},
	{6.f, 0.9f, 0.f},
	{14.f, 0.8f, 0.f},
	{30.f, 0.7f, 0.f},
	{64.f, 0.5f, 0.f}
};

static FMOD_VECTOR fmod_rolloff_null_points[4] = {
	{0.f, 1.f, 0.f}, // 0 dist = 1.f vol
	{2.f, 1.f, 0.f},
	{16.f, 0.1f, 0.f},
	{20.f, 0.0f, 0.f}
};

void VoiceChat_t::updateOnMapChange3DRolloff()
{
	bool bossMap = false;
	if ( !strcmp(map.name, "Hell Boss")
		|| !strcmp(map.name, "Sanctum")
		|| !strcmp(map.name, "Boss") )
	{
		bossMap = true;
	}


	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( VoiceChat.PlayerChannels[i].outputChannel )
		{
			if ( VoiceChat.getAudioSettingBool(VoiceChat_t::AudioSettingBool::VOICE_SETTING_USE_CUSTOM_ROLLOFF) )
			{
				fmod_result = VoiceChat.PlayerChannels[i].outputChannel->set3DCustomRolloff(bossMap ? fmod_rolloff_points_arena : fmod_rolloff_points, 6);
			}
			else
			{
				fmod_result = VoiceChat.PlayerChannels[i].outputChannel->set3DCustomRolloff(bossMap ? fmod_rolloff_points_arena : fmod_rolloff_null_points, 4);
			}
		}
	}
}

void VoiceChat_t::PlayerChannels_t::setupPlayback()
{
	if ( no_sound )
	{
		return;
	}
	const int windowSize = desiredLatency * sizeof(short) * VoiceChat.nativeChannels;
	FMOD_CREATESOUNDEXINFO exinfoBuffer = { 0 };
	exinfoBuffer.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
	exinfoBuffer.numchannels = VoiceChat.nativeChannels;
	exinfoBuffer.format = FMOD_SOUND_FORMAT_PCM16;
	exinfoBuffer.defaultfrequency = VoiceChat.nativeRate;
	exinfoBuffer.length = windowSize/* * sizeof(short) * nativeChannels*/;
	exinfoBuffer.pcmreadcallback = pcmreadcallback;
	exinfoBuffer.userdata = (void*)(intptr_t)(player);
	fmod_result = fmod_system->createSound(nullptr, FMOD_LOOP_NORMAL 
		| FMOD_OPENUSER 
		| FMOD_CREATESTREAM 
		| FMOD_3D
		| FMOD_3D_CUSTOMROLLOFF, &exinfoBuffer, &outputSound);

	if ( !VoiceChat.outChannelGroup )
	{
		fmod_result = fmod_system->createChannelGroup(nullptr, &VoiceChat.outChannelGroup);
		if ( VoiceChat.outChannelGroup )
		{
			VoiceChat.outChannelGroup->setVolume(std::max(0.0f, MainMenu::master_volume));
		}
	}

	fmod_result = fmod_system->playSound(outputSound, VoiceChat.outChannelGroup, false, &outputChannel);

	outputChannel->setPriority(64); // default 128

	static ConsoleCommand ccmd_voice_rolloff("/voice_rolloff", "",
		[](int argc, const char* argv[]) {
			if ( argc > 3 )
			{
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( VoiceChat.PlayerChannels[i].outputChannel )
					{
						int index = atoi(argv[1]);
						if ( index >= 0 && index < 6 )
						{
							fmod_rolloff_points[index].x = atoi(argv[2]) / 100.f;
							fmod_rolloff_points[index].y = atoi(argv[3]) / 100.f;
						}
						VoiceChat.PlayerChannels[i].outputChannel->set3DCustomRolloff(fmod_rolloff_points, 6);
					}
				}
			}
			messagePlayer(clientnum, MESSAGE_HINT, "Rolloff values:\n{%.f tiles %.2f vol}\n{%.f tiles %.2f vol}\n{%.f tiles %.2f vol}\n{%.f tiles %.2f vol}\n{%.f tiles %.2f vol}\n{%.f tiles %.2f vol}",
				fmod_rolloff_points[0].x, fmod_rolloff_points[0].y, fmod_rolloff_points[1].x, fmod_rolloff_points[1].y,
				fmod_rolloff_points[2].x, fmod_rolloff_points[2].y, fmod_rolloff_points[3].x, fmod_rolloff_points[3].y,
				fmod_rolloff_points[4].x, fmod_rolloff_points[4].y, fmod_rolloff_points[5].x, fmod_rolloff_points[5].y);
		}
	);
	if ( !VoiceChat.getAudioSettingBool(VoiceChat_t::AudioSettingBool::VOICE_SETTING_USE_CUSTOM_ROLLOFF) )
	{
		fmod_result = outputChannel->set3DCustomRolloff(fmod_rolloff_null_points, 4);
	}
	else
	{
		fmod_result = outputChannel->set3DCustomRolloff(fmod_rolloff_points, 6);
	}

	for ( int i = 1; i < DSPORDER_END; ++i )
	{
		if ( i == DSPORDER_REVERB )
		{
			FMOD::DSP* dsp_reverb = nullptr;
			fmod_result = fmod_system->createDSPByType(FMOD_DSP_TYPE_SFXREVERB, &dsp_reverb);
			if ( dsp_reverb )
			{
				fmod_result = outputChannel->addDSP(i, dsp_reverb);
				setReverbSettings(dsp_reverb, cvar_voice_reverb_player->x, cvar_voice_reverb_player->y, cvar_voice_reverb_player->z);
				if ( i == 1 || (i == DSPORDER_END - 1) )
				{
					dsp_reverb->setMeteringEnabled(true, true);
				}
			}
		}
		else if ( i == DSPORDER_EQ )
		{
			FMOD::DSP* dsp_eq = nullptr;
			fmod_result = fmod_system->createDSPByType(FMOD_DSP_TYPE_THREE_EQ, &dsp_eq);
			if ( dsp_eq )
			{
				fmod_result = outputChannel->addDSP(i, dsp_eq);
				setEQSettings(dsp_eq, cvar_voice_eq_player->x, cvar_voice_eq_player->y, cvar_voice_eq_player->z);
				if ( i == 1 || (i == DSPORDER_END - 1) )
				{
					dsp_eq->setMeteringEnabled(true, true);
				}
			}
		}
		else if ( i == DSPORDER_NORMALIZE )
		{
			FMOD::DSP* dsp_normalize = nullptr;
			fmod_result = fmod_system->createDSPByType(FMOD_DSP_TYPE_NORMALIZE, &dsp_normalize);
			if ( dsp_normalize )
			{
				fmod_result = outputChannel->addDSP(i, dsp_normalize);
				dsp_normalize->setParameterFloat(FMOD_DSP_NORMALIZE_FADETIME, kNormalizeFadeTime);
				dsp_normalize->setMeteringEnabled(true, true);
				if ( i == 1 || (i == DSPORDER_END - 1) )
				{
					dsp_normalize->setMeteringEnabled(true, true);
				}
			}
		}
		else if ( i == DSPORDER_LIMITER )
		{
			FMOD::DSP* dsp_limiter = nullptr;
			fmod_result = fmod_system->createDSPByType(FMOD_DSP_TYPE_LIMITER, &dsp_limiter);
			if ( dsp_limiter )
			{
				fmod_result = outputChannel->addDSP(i, dsp_limiter);
				fmod_result = dsp_limiter->setParameterFloat(FMOD_DSP_LIMITER_MAXIMIZERGAIN, 0.f);
				fmod_result = dsp_limiter->setParameterFloat(FMOD_DSP_LIMITER_CEILING, 0.f);
				fmod_result = dsp_limiter->setParameterFloat(FMOD_DSP_LIMITER_RELEASETIME, 1000.f);
				if ( i == 1 || (i == DSPORDER_END - 1) )
				{
					dsp_limiter->setMeteringEnabled(true, true);
				}
			}
		}
		else if ( i == DSPORDER_FADER_LOCALGAIN )
		{
			FMOD::DSP* dsp_local_fader = nullptr;
			fmod_result = fmod_system->createDSPByType(FMOD_DSP_TYPE_FADER, &dsp_local_fader);
			if ( dsp_local_fader )
			{
				fmod_result = outputChannel->addDSP(i, dsp_local_fader);
				fmod_result = dsp_local_fader->setParameterFloat(FMOD_DSP_FADER_GAIN, std::min(kMaxGain, (std::max(-80.f, (localChannelGain - 100.f)))));
				if ( i == 1 || (i == DSPORDER_END - 1) )
				{
					dsp_local_fader->setMeteringEnabled(true, true);
				}
			}
		}
		else if ( i == DSPORDER_FADER_CHANNELGAIN )
		{
			FMOD::DSP* dsp_fader = nullptr;
			fmod_result = fmod_system->createDSPByType(FMOD_DSP_TYPE_FADER, &dsp_fader);
			if ( dsp_fader )
			{
				fmod_result = outputChannel->addDSP(i, dsp_fader);
				fmod_result = dsp_fader->setParameterFloat(FMOD_DSP_FADER_GAIN, (std::min(kMaxGain, std::max(-80.f, (channelGain - 100.f)))));
				if ( i == 1 || (i == DSPORDER_END - 1) )
				{
					dsp_fader->setMeteringEnabled(true, true);
				}
			}
		}
	}


	/*FMOD::DSP* dsp_misc = nullptr;
	fmod_result = fmod_system->createDSPByType(FMOD_DSP_TYPE_PITCHSHIFT, &dsp_misc);
	if ( dsp_misc )
	{
		outputChannel->addDSP(4, dsp_misc);
		dsp_misc->setBypass(true);
	}

	dsp_misc = nullptr;
	fmod_result = fmod_system->createDSPByType(FMOD_DSP_TYPE_CHORUS, &dsp_misc);
	if ( dsp_misc )
	{
		outputChannel->addDSP(5, dsp_misc);
		dsp_misc->setBypass(true);
	}*/

	logInfo("VoiceChat_t::PlayerChannels_t::setupPlayback()");
}

static ConsoleCommand ccmd_voice_encoding("/voice_encoding", "",
	[](int argc, const char* argv[]) {
		VoiceChat.using_encoding = !VoiceChat.using_encoding;
		messagePlayer(clientnum, MESSAGE_HINT, "Voice encoding set to %d", VoiceChat.using_encoding);
	});

void VoiceChat_t::pushAvailableDatagrams()
{
	if ( using_encoding )
	{
#ifdef USE_OPUS
		static std::vector<char> buffer(FRAME_SIZE * sizeof(short));
		while ( ringBufferRecord.GetReadAvail() >= FRAME_SIZE * sizeof(short) )
		{
			std::fill(buffer.begin(), buffer.end(), 0);
			ringBufferRecord.Read(buffer.data(), FRAME_SIZE * sizeof(short));

			OpusAudioCodec_t::encode_rtn encode_rtn = OpusAudioCodec.encodeFrameSync(buffer, FRAME_SIZE);
			if ( encode_rtn.numBytes < 0 )
			{
				// error
				OpusAudioCodec_t::logError("failed encoding frame, result: %d (%s)", encode_rtn.numBytes, opus_strerror(encode_rtn.numBytes));
			}
			else if ( encode_rtn.numBytes > 0 )
			{
				recordingDatagrams.push_back(std::vector<char>(encode_rtn.numBytes));
				auto& back = recordingDatagrams.back();
				memcpy(back.data(), encode_rtn.cbits, encode_rtn.numBytes);
			}
		}
#endif
	}
	else
	{
		while ( ringBufferRecord.GetReadAvail() )
		{
			recordingDatagrams.push_back(std::vector<char>(std::min(FRAME_SIZE, ringBufferRecord.GetReadAvail())));
			auto& back = recordingDatagrams.back();
			ringBufferRecord.Read(back.data(), back.size());
		}
	}
}

bool VoiceChat_t::mainMenuAudioTabOpen()
{
	if ( MainMenu::main_menu_frame )
	{
		if ( auto frame = MainMenu::main_menu_frame->findFrame("settings") )
		{
			if ( auto settings_subwindow = frame->findFrame("settings_subwindow") )
			{
				if ( auto slider = settings_subwindow->findSlider("setting_master_volume_slider") )
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool VoiceChat_t::getAudioSettingBool(VoiceChat_t::AudioSettingBool option)
{
	AudioSettings_t* setting = &activeSettings;
	if ( MainMenu::main_menu_frame && MainMenu::main_menu_frame->findFrame("settings") )
	{
		setting = &mainmenuSettings;
	}

	if ( !setting ) { return false; }

	switch ( option )
	{
	case AudioSettingBool::VOICE_SETTING_ENABLE_VOICE_INPUT:
		return setting->enable_voice_input;
		break;
	case AudioSettingBool::VOICE_SETTING_ENABLE_VOICE_RECEIVE:
		return setting->enable_voice_receive;
		break;
	case AudioSettingBool::VOICE_SETTING_LOOPBACK_LOCAL_RECORD:
		return setting->loopback_local_record;
		break;
	case AudioSettingBool::VOICE_SETTING_PUSHTOTALK:
		return setting->pushToTalk;
		break;
	case AudioSettingBool::VOICE_SETTING_USE_CUSTOM_ROLLOFF:
		return setting->use_custom_rolloff;
		break;
	default:
		break;
	}
	return false;
}
float VoiceChat_t::getAudioSettingFloat(VoiceChat_t::AudioSettingFloat option)
{
	AudioSettings_t* setting = &activeSettings;
	if ( MainMenu::main_menu_frame && MainMenu::main_menu_frame->findFrame("settings") )
	{
		setting = &mainmenuSettings;
	}

	if ( !setting ) { return 0.f; }

	switch ( option )
	{
	case AudioSettingFloat::VOICE_SETTING_RECORDINGGAIN:
		return setting->recordingGain;
		break;
	case AudioSettingFloat::VOICE_SETTING_VOICE_GLOBAL_VOLUME:
		return setting->voice_global_volume;
		break;
	case AudioSettingFloat::VOICE_SETTING_NORMALIZE_THRESHOLD:
		return setting->recordingNormalizeThreshold;
		break;
	case AudioSettingFloat::VOICE_SETTING_NORMALIZE_AMP:
		return setting->recordingNormalizeAmp;
		break;
	default:
		break;
	}
	return 0.f;
}

const char* VoiceChat_t::getVoiceChatBindingName(int player)
{
	return "Voice Chat";
	//if ( player < 0 || player >= MAXPLAYERS ) { return ""; }

	////bool pauseMenuSlidersAvailable = false;
	////if ( !intro && gamePaused && MainMenu::main_menu_frame )
	////{
	////	auto selectedWidget = MainMenu::main_menu_frame->findSelectedWidget(MainMenu::getMenuOwner());
	////	if ( selectedWidget )
	////	{
	////		if ( !strcmp(selectedWidget->getName(), "pause player status audio") )
	////		{
	////			pauseMenuSlidersAvailable = true;
	////		}
	////		else if ( !strcmp(selectedWidget->getName(), "audio slider") )
	////		{
	////			pauseMenuSlidersAvailable = true;
	////		}
	////		else
	////		{
	////			auto& actions = selectedWidget->getWidgetActions();
	////			auto find = actions.find("MenuPageLeft");
	////			if ( find != actions.end() && find->second == "pause player status audio" )
	////			{
	////				if ( auto pauseAudioSliders = MainMenu::main_menu_frame->findFrame("pause player status audio") )
	////				{
	////					pauseMenuSlidersAvailable = true;
	////				}
	////			}
	////		}
	////	}
	////}

	//const char* voiceBindingName = inputs.bPlayerUsingKeyboardControl(player) && !inputs.hasController(player) ? "Voice Chat" : "Voice Chat Gamepad";
	///*if ( inputs.hasController(clientnum) && (intro || pauseMenuSlidersAvailable) )
	//{
	//	voiceBindingName = "Voice Chat (Pause Menu)";
	//}*/
	//return voiceBindingName;
}

ConsoleVariable<bool> cvar_voice_debug("/voice_debug", false);
void VoiceChat_t::updateRecording()
{
	allowInputs = false;
#ifdef NINTENDO
	return;
#endif
	pushAvailableDatagrams();

	auto& input = Input::inputs[clientnum];
	const char* voiceBindingName = getVoiceChatBindingName(clientnum);

	if ( (multiplayer == SINGLE && !*cvar_voice_debug)
		|| !getAudioSettingBool(AudioSettingBool::VOICE_SETTING_ENABLE_VOICE_INPUT)
		|| (intro && (!net_packet 
			|| (MainMenu::main_menu_frame && MainMenu::main_menu_frame->findFrame("lobby") && !MainMenu::isPlayerSignedIn(clientnum))))
		|| input.input(voiceBindingName).type == Input::binding_t::INVALID )
	{
		// record on the main menu settings tab to view levels
		if ( !(mainMenuAudioTabOpen()) )
		{
			if ( bRecordingInit )
			{
				deinitRecording(intro);
			}
			return;
		}
	}

	if ( initialized && !loading && !bRecordingInit )
	{
		initRecording();
	}

	fmod_result = fmod_system->isRecording(recordDeviceIndex, &bIsRecording);
	if ( (fmod_result != FMOD_ERR_RECORD_DISCONNECTED && fmod_result != FMOD_OK)
		|| !bIsRecording )
	{
		FMODErrorCheck();
		return;
	}

	bool pauseMenuSlidersAvailable = false;
	if ( !intro && gamePaused && MainMenu::main_menu_frame )
	{
		auto selectedWidget = MainMenu::main_menu_frame->findSelectedWidget(MainMenu::getMenuOwner());
		if ( selectedWidget )
		{
			if ( !strcmp(selectedWidget->getName(), "pause player status audio") )
			{
				pauseMenuSlidersAvailable = true;
			}
			else if ( !strcmp(selectedWidget->getName(), "audio slider") )
			{
				pauseMenuSlidersAvailable = true;
			}
			else
			{
				auto& actions = selectedWidget->getWidgetActions();
				auto find = actions.find("MenuPageLeft");
				if ( find != actions.end() && find->second == "pause player status audio" )
				{
					if ( auto pauseAudioSliders = MainMenu::main_menu_frame->findFrame("pause player status audio") )
					{
						pauseMenuSlidersAvailable = true;
					}
				}
			}
		}
	}

	int voice_no_send = GameplayPreferences_t::getGameConfigValue(GameplayPreferences_t::GOPT_VOICE_NO_SEND);
	if ( (players[clientnum]->bControlEnabled || intro)
		&& !players[clientnum]->usingCommand()
		&& !inputstr
		&& !(voice_no_send & (1 << clientnum))
		&& (!gamePaused || (gamePaused && (MainMenu::isCutsceneActive() || movie || pauseMenuSlidersAvailable))) )
	{
		allowInputs = true;
	}

	bool doRecord = false;

	if ( mainMenuAudioTabOpen() )
	{
		if ( getAudioSettingBool(AudioSettingBool::VOICE_SETTING_LOOPBACK_LOCAL_RECORD) )
		{
			doRecord = true;
		}
	}

	if ( lastRecordTick > 0 )
	{
		--lastRecordTick;
	}

	if ( allowInputs )
	{
		if ( lastRecordTick > 0 )
		{
			doRecord = true; // give x ms of letting go of record to continue
		}
		if ( !getAudioSettingBool(AudioSettingBool::VOICE_SETTING_PUSHTOTALK) )
		{
			if ( input.consumeBinaryToggle(voiceBindingName) )
			{
				voiceToggleTalk = !voiceToggleTalk;
				if ( voiceToggleTalk )
				{
					//messagePlayer(clientnum, MESSAGE_INTERACTION, Language::get(6449));
					Player::soundActivate();
					//playSound(710, 64);
					doRecord = true;
				}
				else
				{
					//messagePlayer(clientnum, MESSAGE_INTERACTION, Language::get(6450));
					Player::soundCancel();
					//playSound(711, 64);
				}
			}
		}
		else if ( getAudioSettingBool(AudioSettingBool::VOICE_SETTING_PUSHTOTALK) )
		{
			if ( input.binaryToggle(voiceBindingName) )
			{
				doRecord = true;
				if ( lastRecordTick == 0 )
				{
					//playSound(710, 64);
					//Player::soundActivate();
				}
				lastRecordTick = TICKS_PER_SECOND / 4;
			}
			else
			{
				if ( lastRecordTick == 1 )
				{
					//playSound(711, 64);
				}
			}
		}
	}
	else
	{
		if ( getAudioSettingBool(AudioSettingBool::VOICE_SETTING_PUSHTOTALK) )
		{
			if ( input.binaryToggle(voiceBindingName) )
			{
				input.consumeBinaryToggle(voiceBindingName);
			}
		}
	}

	if ( !getAudioSettingBool(AudioSettingBool::VOICE_SETTING_PUSHTOTALK) )
	{
		if ( voiceToggleTalk )
		{
			doRecord = true;
			lastRecordTick = TICKS_PER_SECOND / 4;
		}
	}

	if ( doRecord )
	{
		PlayerChannels[clientnum].talkingTicks = TICKS_PER_SECOND / 4;
	}

	if ( true )
	{
		unsigned int recordPos = 0;
		fmod_result = fmod_system->getRecordPosition(recordDeviceIndex, &recordPos);
		if ( fmod_result != FMOD_ERR_RECORD_DISCONNECTED )
		{
			if ( FMODErrorCheck() ) { return; }
		}

		unsigned int recordDelta = (recordPos >= recordingLastPos) ? (recordPos - recordingLastPos) : (recordPos + recordingSoundLength - recordingLastPos);
		recordingSamples += recordDelta;

		static unsigned int minRecordDelta = (unsigned int)-1;
		if ( recordDelta && (recordDelta < minRecordDelta) )
		{
			minRecordDelta = recordDelta; /* Smallest driver granularity seen so far */
			recordingAdjustedLatency = (recordDelta <= recordingDesiredLatency) ? recordingDesiredLatency : recordDelta; /* Adjust our latency if driver granularity is high */
		}

		/*
			Delay playback until our desired latency is reached.
		*/
		if ( recordingSamples >= recordingAdjustedLatency && !loading )
		{
			if ( !recordingChannel )
			{
				fmod_result = fmod_system->playSound(recordingSound, 0, false, &recordingChannel);
				if ( FMODErrorCheck() ) { return; }
				//recordingChannel->setVolume(0.f);
				if ( FMODErrorCheck() ) { return; }

				FMOD::DSP* dsp_fader = nullptr;
				fmod_result = recordingChannel->getDSP(0, &dsp_fader);
				if ( dsp_fader )
				{
					dsp_fader->setParameterFloat(FMOD_DSP_FADER_GAIN, -79.9);
				}

				FMOD::DSP* dsp_limiter = nullptr;
				fmod_result = fmod_system->createDSPByType(FMOD_DSP_TYPE_LIMITER, &dsp_limiter);
				if ( dsp_limiter )
				{
					recordingChannel->addDSP(1, dsp_limiter);
					//fmod_result = dsp_limiter->setParameterFloat(FMOD_DSP_LIMITER_MAXIMIZERGAIN, (std::max(0.f, (getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_RECORDINGGAIN) - 100.f))));
					//dsp_limiter->setMeteringEnabled(true, true);
					fmod_result = dsp_limiter->setParameterFloat(FMOD_DSP_LIMITER_MAXIMIZERGAIN, 0.f);
					fmod_result = dsp_limiter->setParameterFloat(FMOD_DSP_LIMITER_CEILING, 0.f);
					fmod_result = dsp_limiter->setParameterFloat(FMOD_DSP_LIMITER_RELEASETIME, 1000.f);
				}

				fmod_result = fmod_system->createDSPByType(FMOD_DSP_TYPE_FADER, &dsp_fader);
				if ( dsp_fader )
				{
					recordingChannel->addDSP(2, dsp_fader);
					dsp_fader->setChannelFormat(0, nativeChannels, FMOD_SPEAKERMODE_MONO);
					fmod_result = dsp_fader->setParameterFloat(FMOD_DSP_FADER_GAIN, std::min(kMaxGain, 
						getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_RECORDINGGAIN) - 100.f));
					dsp_fader->setMeteringEnabled(true, true);
				}

				FMOD::DSP* dsp_normalize = nullptr;
				fmod_result = fmod_system->createDSPByType(FMOD_DSP_TYPE_NORMALIZE, &dsp_normalize);
				if ( dsp_normalize )
				{
					recordingChannel->addDSP(3, dsp_normalize);
					dsp_normalize->setParameterFloat(FMOD_DSP_NORMALIZE_FADETIME, kNormalizeFadeTime);
					if ( getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_NORMALIZE_AMP) < 1.f )
					{
						dsp_normalize->setBypass(true);
					}
					else
					{
						dsp_normalize->setBypass(false);
						fmod_result = dsp_normalize->setParameterFloat(FMOD_DSP_NORMALIZE_MAXAMP, 
							std::min(100.f, std::max(1.f, getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_NORMALIZE_AMP))));
						fmod_result = dsp_normalize->setParameterFloat(FMOD_DSP_NORMALIZE_THRESHOLD,
							std::min(1.f, std::max(0.f, getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_NORMALIZE_THRESHOLD))));
					}
					dsp_normalize->setMeteringEnabled(true, true);
				}
			}
		}

		if ( recordPos != recordingLastPos )
		{
			if ( recordingChannel )
			{
				int num_dsps = 0;
				fmod_result = recordingChannel->getNumDSPs(&num_dsps);
				if ( fmod_result != FMOD_ERR_INVALID_HANDLE )
				{
					for ( int i = 0; i < num_dsps; ++i )
					{
						FMOD::DSP* dsp = nullptr;
						fmod_result = recordingChannel->getDSP(i, &dsp);
						FMOD_DSP_TYPE type;
						dsp->getType(&type);

						bool in, out;
						dsp->getMeteringEnabled(&in, &out);
						if ( type == FMOD_DSP_TYPE::FMOD_DSP_TYPE_FADER )
						{
							if ( i == 0 )
							{
								dsp->setParameterFloat(FMOD_DSP_FADER_GAIN, -79.9);
							}
						}
						if ( type == FMOD_DSP_TYPE_NORMALIZE )
						{
							if ( getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_NORMALIZE_AMP) < 1.f )
							{
								dsp->setBypass(true);
							}
							else
							{
								dsp->setBypass(false);
								fmod_result = dsp->setParameterFloat(FMOD_DSP_NORMALIZE_MAXAMP,
									std::min(kMaxNormalizeAmp, std::max(1.f, getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_NORMALIZE_AMP))));
								fmod_result = dsp->setParameterFloat(FMOD_DSP_NORMALIZE_THRESHOLD,
									std::min(kMaxNormalizeThreshold, std::max(0.f, getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_NORMALIZE_THRESHOLD))));
							}
							if ( in && out )
							{
								FMOD_DSP_METERING_INFO input_metering{};
								FMOD_DSP_METERING_INFO output_metering{};
								fmod_result = dsp->getMeteringInfo(&input_metering, &output_metering);
								loopback_input_volume = std::max(input_metering.peaklevel[0], loopback_input_volume - 0.02f);
							}
						}
						/*if ( type == FMOD_DSP_TYPE_LIMITER )
						{
							fmod_result = dsp->setParameterFloat(FMOD_DSP_LIMITER_MAXIMIZERGAIN, (std::max(0.f, (getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_RECORDINGGAIN) - 100.f))));
						}*/
						if ( type == FMOD_DSP_TYPE::FMOD_DSP_TYPE_FADER && in && out )
						{
							fmod_result = dsp->setParameterFloat(FMOD_DSP_FADER_GAIN, std::min(kMaxGain, 
								std::max(-80.f, getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_RECORDINGGAIN) - 100.f)));
							FMOD_DSP_METERING_INFO input_metering{};
							FMOD_DSP_METERING_INFO output_metering{};
							dsp->getMeteringInfo(&input_metering, &output_metering);
							loopback_output_volume = std::max(output_metering.peaklevel[0], loopback_output_volume - 0.02f);
						}
					}
				}
			}
		}

		if ( recordPos != recordingLastPos )
		{
			int blocklength = (int)recordPos - (int)recordingLastPos;
			if ( blocklength < 0 )
			{
				blocklength += (int)recordingSoundLength;
			}

			void* ptr1 = nullptr;
			unsigned len1 = 0;
			void* ptr2 = nullptr;
			unsigned len2 = 0;

			if ( doRecord )
			{
				recordingSound->lock(recordingLastPos * nativeChannels * sizeof(short),
					blocklength * sizeof(short) * nativeChannels, &ptr1, &ptr2, &len1, &len2);

				if ( len1 > 0 )
				{
					VoiceChat_t::ringBufferRecord.Write((char*)ptr1, len1);
				}
				if ( len2 > 0 )
				{
					VoiceChat_t::ringBufferRecord.Write((char*)ptr2, len2);
				}
				recordingSound->unlock(ptr1, ptr2, len1, len2);
			}
		}
		recordingLastPos = recordPos;
	}

	pushAvailableDatagrams();
}

static ConsoleVariable<bool> cvar_voice_loopback_ingame("/voice_loopback_ingame", false);
void VoiceChat_t::update()
{
	if ( !bInit )
	{
		return;
	}

	updateRecording();

	sendPackets();

	FMOD_VECTOR listener_pos;
	fmod_result = fmod_system->get3DListenerAttributes(0, &listener_pos, nullptr, nullptr, nullptr);
	bool hasListenerPos = fmod_result == FMOD_OK;

	static bool prevCustomRolloff = true;
	if ( prevCustomRolloff != getAudioSettingBool(VOICE_SETTING_USE_CUSTOM_ROLLOFF) )
	{
		updateOnMapChange3DRolloff();
	}
	prevCustomRolloff = getAudioSettingBool(VOICE_SETTING_USE_CUSTOM_ROLLOFF);

	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( PlayerChannels[i].talkingTicks > 0 )
		{
			--PlayerChannels[i].talkingTicks;
		}
		if ( PlayerChannels[i].lastAudibleTick > 0 )
		{
			--PlayerChannels[i].lastAudibleTick;
		}
		if ( PlayerChannels[i].outputChannel )
		{
			bool isVirtual = false;
			fmod_result = PlayerChannels[i].outputChannel->isVirtual(&isVirtual);
			if ( isVirtual )
			{
				PlayerChannels[i].audio_queue_mutex.lock();
				PlayerChannels[i].audioQueue.clear();
				PlayerChannels[i].audio_queue_mutex.unlock();
			}

			float volume = std::min(std::max(0.0f, getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_VOICE_GLOBAL_VOLUME)), 1.f);
			PlayerChannels[i].outputChannel->setVolume(volume);

			if ( (multiplayer == SINGLE && !*cvar_voice_debug) )
			{
				if ( !mainMenuAudioTabOpen() )
				{
					PlayerChannels[i].outputChannel->setVolume(0.f);
				}
			}
			if ( client_disconnected[i] )
			{
				PlayerChannels[i].outputChannel->setVolume(0.f);
			}

			FMOD::DSP* dsp_reverb = nullptr;
			FMOD::DSP* dsp_eq = nullptr;
			{
				fmod_result = PlayerChannels[i].outputChannel->getDSP(DSPORDER_REVERB, &dsp_reverb);
				fmod_result = PlayerChannels[i].outputChannel->getDSP(DSPORDER_EQ, &dsp_eq);
				if ( dsp_eq )
				{
					dsp_eq->setBypass(client_disconnected[i] || (i != clientnum && multiplayer == SINGLE));
				}
			}

			{
				FMOD::DSP* dsp_normalize = nullptr;
				fmod_result = PlayerChannels[i].outputChannel->getDSP(DSPORDER_NORMALIZE, &dsp_normalize);
				if ( dsp_normalize )
				{
					if ( PlayerChannels[i].normalize_amp < 1.f )
					{
						dsp_normalize->setBypass(true);
					}
					else
					{
						dsp_normalize->setBypass(client_disconnected[i] || (i != clientnum && multiplayer == SINGLE));
						fmod_result = dsp_normalize->setParameterFloat(FMOD_DSP_NORMALIZE_MAXAMP, PlayerChannels[i].normalize_amp);
						fmod_result = dsp_normalize->setParameterFloat(FMOD_DSP_NORMALIZE_THRESHOLD, PlayerChannels[i].normalize_threshold);
					}
				}
			}

			{
				FMOD::DSP* dsp_fader_local = nullptr;
				fmod_result = PlayerChannels[i].outputChannel->getDSP(DSPORDER_FADER_LOCALGAIN, &dsp_fader_local);
				if ( dsp_fader_local )
				{
					if ( i == clientnum )
					{
						PlayerChannels[i].localChannelGain = 100.f;
					}
					if ( intro )
					{
						fmod_result = dsp_fader_local->setParameterFloat(FMOD_DSP_FADER_GAIN, 0.f);
					}
					else
					{
						fmod_result = dsp_fader_local->setParameterFloat(FMOD_DSP_FADER_GAIN,
							std::min(kMaxGain, (std::max(-80.f, (PlayerChannels[i].localChannelGain - 100.f)))));
					}
				}
			}

			{
				FMOD::DSP* dsp_limiter_channelGain = nullptr;
				fmod_result = PlayerChannels[i].outputChannel->getDSP(DSPORDER_LIMITER, &dsp_limiter_channelGain);
				if ( dsp_limiter_channelGain )
				{
					/*fmod_result = dsp_limiter_channelGain->setParameterFloat(FMOD_DSP_LIMITER_MAXIMIZERGAIN,
						std::max(0.f, PlayerChannels[i].channelGain - 100.f));*/
				}
			}
			{
				FMOD::DSP* dsp_fader = nullptr;
				fmod_result = PlayerChannels[i].outputChannel->getDSP(DSPORDER_FADER_CHANNELGAIN, &dsp_fader);
				if ( dsp_fader )
				{
					fmod_result = dsp_fader->setParameterFloat(FMOD_DSP_FADER_GAIN, 
						std::min(kMaxGain, (std::max(-80.f, PlayerChannels[i].channelGain - 100.f))));
				}
			}

			if ( i == clientnum 
				&& !((mainMenuAudioTabOpen() && getAudioSettingBool(AudioSettingBool::VOICE_SETTING_LOOPBACK_LOCAL_RECORD))
					|| (getAudioSettingBool(AudioSettingBool::VOICE_SETTING_LOOPBACK_LOCAL_RECORD) && *cvar_voice_loopback_ingame) ) )
			{
				// if clientnum, use local volume when not testing loopback audio
				if ( PlayerChannels[i].talkingTicks > 0 )
				{
					PlayerChannels[i].monitor_input_volume = loopback_input_volume;
					PlayerChannels[i].monitor_output_volume = loopback_output_volume;
				}
				else
				{
					PlayerChannels[i].monitor_input_volume = 0.f;
					PlayerChannels[i].monitor_output_volume = 0.f;
				}
			}
			else
			{
				FMOD_DSP_METERING_INFO input_metering{};
				FMOD_DSP_METERING_INFO output_metering{};
				FMOD::DSP* dsp_end = nullptr;
				PlayerChannels[i].outputChannel->getDSP(DSPORDER_END - 1, &dsp_end);
				FMOD::DSP* dsp_start = nullptr;
				PlayerChannels[i].outputChannel->getDSP(1, &dsp_start);

				if ( dsp_end )
				{
					fmod_result = dsp_end->getMeteringInfo(&input_metering, &output_metering);
					PlayerChannels[i].monitor_input_volume = std::max(input_metering.peaklevel[0], PlayerChannels[i].monitor_input_volume - 0.02f);
				}

				if ( dsp_start )
				{
					fmod_result = dsp_start->getMeteringInfo(&input_metering, &output_metering);
					PlayerChannels[i].monitor_output_volume = std::max(output_metering.peaklevel[0], PlayerChannels[i].monitor_output_volume - 0.02f);
					if ( PlayerChannels[i].monitor_output_volume > 0.05 )
					{
						PlayerChannels[i].lastAudibleTick = TICKS_PER_SECOND / 4;
					}
				}
			}
			//messagePlayer(i, MESSAGE_DEBUG, "In: %.2f Out: %.2f", PlayerChannels[i].monitor_input_volume, PlayerChannels[i].monitor_output_volume);

			FMOD_MODE mode;
			fmod_result = PlayerChannels[i].outputChannel->getMode(&mode);

			static ConsoleVariable<bool> cvar_voice_audibility("/voice_audibility", false);
			if ( *cvar_voice_audibility && i == clientnum )
			{
				float audibility = 0.f;
				PlayerChannels[i].outputChannel->getAudibility(&audibility);
				if ( audibility > 0.0001 )
				{
					messagePlayer(i, MESSAGE_HINT, "Audibility: %.2f", audibility);
				}
			}

			if ( intro || MainMenu::isCutsceneActive() || movie )
			{
				mode &= ~(FMOD_3D);
				mode |= FMOD_2D;
				fmod_result = PlayerChannels[i].outputChannel->setMode(mode);

				setReverbSettings(dsp_reverb, cvar_voice_reverb_player->x, cvar_voice_reverb_player->y, cvar_voice_reverb_player->z);
				setEQSettings(dsp_eq, cvar_voice_eq_player->x, cvar_voice_eq_player->y, cvar_voice_eq_player->z);
			}
			else
			{
				mode &= ~(FMOD_2D);
				mode |= FMOD_3D;
				fmod_result = PlayerChannels[i].outputChannel->setMode(mode);

				constexpr real_t lowpassAmount = 12.0;
				constexpr real_t lowpassMinDist = 2.0;
				if ( Entity* entity = Player::getPlayerInteractEntity(i) )
				{
					FMOD_VECTOR position;
					position.x = (float)(entity->x / (real_t)16.0);
					position.y = (float)(0.0);
					position.z = (float)(entity->y / (real_t)16.0);
					fmod_result = PlayerChannels[i].outputChannel->set3DAttributes(&position, nullptr);

					real_t listenerMeters = 0.0;
					if ( hasListenerPos )
					{
						real_t dx = listener_pos.x - position.x;
						real_t dy = listener_pos.z - position.z;
						listenerMeters = sqrt(dx * dx + dy * dy);
					}

					real_t lowpassAttenuation = 0.0;
					if ( listenerMeters >= lowpassMinDist )
					{
						lowpassAttenuation = std::min(1.0, (listenerMeters - lowpassMinDist) / 32.0);
					}

					if ( entity->behavior == &actDeathGhost )
					{
						setReverbSettings(dsp_reverb, cvar_voice_reverb_ghost->x, cvar_voice_reverb_ghost->y, cvar_voice_reverb_ghost->z);
						setEQSettings(dsp_eq, 
							cvar_voice_eq_ghost->x - lowpassAmount * lowpassAttenuation, 
							cvar_voice_eq_ghost->y, 
							cvar_voice_eq_ghost->z);
					}
					else
					{
						setReverbSettings(dsp_reverb, cvar_voice_reverb_player->x, cvar_voice_reverb_player->y, cvar_voice_reverb_player->z);
						setEQSettings(dsp_eq, 
							cvar_voice_eq_player->x - lowpassAmount * lowpassAttenuation, 
							cvar_voice_eq_player->y, 
							cvar_voice_eq_player->z);
					}
				}
				else
				{
					real_t listenerMeters = 0.0;
					if ( hasListenerPos )
					{
						FMOD_VECTOR position;
						fmod_result = PlayerChannels[i].outputChannel->get3DAttributes(&position, nullptr);
						if ( fmod_result == FMOD_OK )
						{
							real_t dx = listener_pos.x - position.x;
							real_t dy = listener_pos.z - position.z;
							listenerMeters = sqrt(dx * dx + dy * dy);
						}
					}

					real_t lowpassAttenuation = 0.0;
					if ( listenerMeters >= lowpassMinDist )
					{
						lowpassAttenuation = std::min(1.0, (listenerMeters - lowpassMinDist) / 32.0);
					}

					setReverbSettings(dsp_reverb, cvar_voice_reverb_ghost->x, cvar_voice_reverb_ghost->y, cvar_voice_reverb_ghost->z);
					setEQSettings(dsp_eq, 
						cvar_voice_eq_ghost->x - lowpassAmount * lowpassAttenuation, 
						cvar_voice_eq_ghost->y, 
						cvar_voice_eq_ghost->z);
				}
			}

			if ( dsp_reverb )
			{
				if ( client_disconnected[i] || (i != clientnum && multiplayer == SINGLE) )
				{
					dsp_reverb->setBypass(true);
				}
			}
		}
	}
}
void VoiceChat_t::deinit()
{
	if ( loopbackPacket )
	{
		SDLNet_FreePacket(loopbackPacket);
		loopbackPacket = nullptr;
	}

	deinitRecording();
	FMODErrorCheck();

	if ( recordingChannel )
	{
		recordingChannel->stop();
		recordingChannel = nullptr;
	}

	if ( recordingSound )
	{
		recordingSound->release();
		recordingSound = nullptr;
	}
	recordingLastPos = 0;
	recordingSamples = 0;
	bIsRecording = false;
	bRecordingInit = false;

	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		PlayerChannels[i].deinit();
	}

	bInit = false;
	logInfo("VoiceChat_t::deinit()");
}

int VoiceChat_t::packetVoiceDataIdx = 15;
void VoiceChat_t::sendPackets()
{
	UDPpacket* packet = (net_packet) ? net_packet : (getAudioSettingBool(AudioSettingBool::VOICE_SETTING_LOOPBACK_LOCAL_RECORD) ? loopbackPacket : nullptr);
	if ( !packet )
	{
		recordingDatagrams.clear();
		return;
	}

	bool loopback = false;
	bool sendVoice = true;
	if ( mainMenuAudioTabOpen()
		&& getAudioSettingBool(AudioSettingBool::VOICE_SETTING_LOOPBACK_LOCAL_RECORD) )
	{
		loopback = true; // we're testing in the settings menu
		sendVoice = false;
	}
	if ( *cvar_voice_loopback_ingame && getAudioSettingBool(AudioSettingBool::VOICE_SETTING_LOOPBACK_LOCAL_RECORD) ) // debug send to all
	{
		loopback = true;
		sendVoice = true;
	}

	static ConsoleVariable<int> cvar_voice_packet_limit("/voice_packet_limit", 10);
	static ConsoleVariable<int> cvar_voice_all_loopback("/voice_all_loopback", false);
	int voice_no_recv = GameplayPreferences_t::getGameConfigValue(GameplayPreferences_t::GOPT_VOICE_NO_RECV);
	for ( int i = 0; i < *cvar_voice_packet_limit; ++i )
	{
		if ( recordingDatagrams.size() )
		{
			memset(packet->data, 0, NET_PACKET_SIZE);
			strcpy((char*)packet->data, "VOIP");
			SDLNet_Write32(datagramSequence, &packet->data[5]);
			SDLNet_Write16(recordingDatagrams.front().size(), &packet->data[9]);
			Uint8 gain = (std::max(-kMaxGain * 10.f, std::min(kMaxGain * 10.f, (getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_RECORDINGGAIN) - 100.f) * 10.f))) + 120;
			packet->data[11] = gain;
			Uint8 normalizeAmp = std::min(kMaxNormalizeAmp, std::max(0.f, getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_NORMALIZE_AMP)));
			packet->data[12] = normalizeAmp;
			Uint16 normalizeThreshold = 400.f * std::min(kMaxNormalizeThreshold, std::max(0.f, getAudioSettingFloat(AudioSettingFloat::VOICE_SETTING_NORMALIZE_THRESHOLD)));
			SDLNet_Write16(normalizeThreshold, &packet->data[13]);
			packet->data[4] = 0;
			packet->data[4] |= getAudioSettingBool(AudioSettingBool::VOICE_SETTING_PUSHTOTALK) ? (1 << 7) : 0;
			packet->data[4] |= using_encoding ? (1 << 6) : 0;
			memcpy(packet->data + packetVoiceDataIdx, (char*)recordingDatagrams.front().data(), recordingDatagrams.front().size());
			packet->len = packetVoiceDataIdx + recordingDatagrams.front().size();
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( loopback && i == clientnum )
				{
					packet->data[4] &= ~(0xF);
					packet->data[4] |= (Uint8)clientnum;
					receivePacket(packet);
				}
				else if ( loopback && *cvar_voice_all_loopback )
				{
					packet->data[4] &= ~(0xF);
					packet->data[4] |= (Uint8)i;
					receivePacket(packet);
				}
				
				if ( sendVoice )
				{
					if ( i == 0 )
					{
						if ( multiplayer == CLIENT )
						{
							packet->data[4] &= ~(0xF);
							packet->data[4] |= (Uint8)clientnum;
							packet->address.host = net_server.host;
							packet->address.port = net_server.port;
							sendPacket(net_sock, -1, packet, 0);
						}
					}
					else if ( i > 0 )
					{
						if ( !client_disconnected[i] && multiplayer == SERVER )
						{
							if ( !(voice_no_recv & i) )
							{
								packet->data[4] &= ~(0xF);
								packet->data[4] |= (Uint8)clientnum;
								packet->address.host = net_clients[i - 1].host;
								packet->address.port = net_clients[i - 1].port;
								sendPacket(net_sock, -1, packet, i - 1);
							}
						}
					}
				}
			}
			recordingDatagrams.erase(recordingDatagrams.begin());
			++datagramSequence;
		}
		else
		{
			break;
		}
	}
}

#ifdef USE_OPUS
static ConsoleCommand ccmd_voice_opus_stats("/voice_opus_stats", "",
	[](int argc, const char* argv[]) {
		messagePlayer(clientnum, MESSAGE_HINT, "samples encode: %u (%.2fms) | decode: %u (%.2fms) | max frame bytes: %u",
			OpusAudioCodec.encoded_samples, OpusAudioCodec.encoding_time,
			OpusAudioCodec.decoded_samples, OpusAudioCodec.decoding_time,
			OpusAudioCodec.max_num_bytes_encoded);
	});
#endif

void VoiceChat_t::receivePacket(UDPpacket* packet)
{
	if ( !packet )
	{
		return;
	}
	if ( !bInit )
	{
		return;
	}

	int player = (packet->data[4]) & 0xF;
	bool encodedFrame = (packet->data[4] & (1 << 6));
	int voice_no_recv = GameplayPreferences_t::getGameConfigValue(GameplayPreferences_t::GOPT_VOICE_NO_RECV);
	bool localNoRecv = (voice_no_recv & (1 << clientnum)) && (player != clientnum);

	if ( player >= 0 && player < MAXPLAYERS
		&& !localNoRecv
		&& packet->len > packetVoiceDataIdx
		&& (packet->len - packetVoiceDataIdx == SDLNet_Read16(&packet->data[9])) )
	{
		Uint32 sequence = SDLNet_Read32(&packet->data[5]);
		PlayerChannels[player].channelGain = (std::min(kMaxGain, std::max(-kMaxGain, ((float)(packet->data[11]) - 120.f) / 10.f))) + 100.f;
		PlayerChannels[player].normalize_amp = std::max(0.f, std::min(kMaxNormalizeAmp, (float)packet->data[12]));
		PlayerChannels[player].normalize_threshold = std::max(0.f, std::min(kMaxNormalizeThreshold, (float)SDLNet_Read16(&packet->data[13]) / 400.f));
		PlayerChannels[player].talkingTicks = TICKS_PER_SECOND / 4;
		unsigned int readBytes = (packet->len - packetVoiceDataIdx);
		if ( (readBytes % 2 == 0 && !encodedFrame) || encodedFrame ) // should be even numbered unencoded
		{
			if ( encodedFrame )
			{
#ifdef USE_OPUS
				static std::vector<opus_int16> out(OpusAudioCodec_t::MAX_FRAME_SIZE * OpusAudioCodec.numChannels);
				static OpusAudioCodec_t::encode_rtn encodedFrame;
				std::fill(out.begin(), out.end(), 0);

#ifdef NINTENDO
				if ( readBytes + 8 >= OpusAudioCodec_t::encode_rtn::OPUS_MAX_PACKET_SIZE )
				{
					OpusAudioCodec_t::logError("failed decoding frame, frame too large with header, size: %d, limit: %d", readBytes + 8, OpusAudioCodec_t::encode_rtn::OPUS_MAX_PACKET_SIZE);
					return; // no space to decode packet
				}
				// header data (packet size, big endian)
				encodedFrame.cbits[0] = (readBytes >> 24) & 0xFF;
				encodedFrame.cbits[1] = (readBytes >> 16) & 0xFF;
				encodedFrame.cbits[2] = (readBytes >> 8) & 0xFF;
				encodedFrame.cbits[3] = (readBytes >> 0) & 0xFF;
				// header data (zeroes)
				memset(encodedFrame.cbits + 4, 0, sizeof(Uint32));
				// opus data offset by 8 due to header
				memcpy(encodedFrame.cbits + 8, &packet->data[packetVoiceDataIdx], readBytes);
				// length of packet increases
				encodedFrame.numBytes = readBytes + 8;
#else
				memcpy(encodedFrame.cbits, &packet->data[packetVoiceDataIdx], readBytes);
				encodedFrame.numBytes = readBytes;
#endif
				int frame_size = OpusAudioCodec.decodeFrame(player, encodedFrame, out);
				if ( frame_size < 0 )
				{
					// error
					OpusAudioCodec_t::logError("failed decoding frame, result: %d (%s)", frame_size, opus_strerror(frame_size));
				}
				else if ( frame_size % 2 != 0 )
				{
					// error
					OpusAudioCodec_t::logError("failed decoding frame, frame size odd: %d", frame_size);
				}
				else if ( frame_size > 0 )
				{
					PlayerChannels[player].audio_queue_mutex.lock();
					int i = 0;
					for ( i = 0; i < OpusAudioCodec.numChannels * frame_size 
						&& (PlayerChannels[player].audioQueue.size() < PlayerChannels_t::audioQueueSizeLimit - 1); i++ )
					{
						// insert 2x bytes per decoded unit
						PlayerChannels[player].audioQueue.push_back(out[i] & 0xFF);
						PlayerChannels[player].audioQueue.push_back((out[i] >> 8) & 0xFF);
					}
					PlayerChannels[player].audio_queue_mutex.unlock();
					unsigned int samplesWritten = i;
					PlayerChannels[player].totalSamplesWritten += samplesWritten * 2;
					if ( samplesWritten != 0 && (samplesWritten < PlayerChannels[player].minimumSamplesWritten) )
					{
						PlayerChannels[player].minimumSamplesWritten = samplesWritten;
						PlayerChannels[player].adjustedLatency = std::max(samplesWritten, PlayerChannels[player].desiredLatency);
					}
					if ( PlayerChannels[player].audioQueue.size() >= PlayerChannels_t::audioQueueSizeLimit )
					{
						logInfo("warning: VoiceChat_t::receivePacket() audio queue full");
					}
					if ( i % 2 == 1 )
					{
						logError("warning: VoiceChat_t::receivePacket() decoded odd number bytes");
					}
				}
#else
				logError("received encoded frame without Opus encoder configured");
				return;
#endif
			}
			else
			{
				PlayerChannels[player].audio_queue_mutex.lock();
				int i = 0;
				for ( i = 0; i < readBytes && PlayerChannels[player].audioQueue.size() < PlayerChannels_t::audioQueueSizeLimit; ++i )
				{
					PlayerChannels[player].audioQueue.push_back(packet->data[packetVoiceDataIdx + i]);
				}
				PlayerChannels[player].audio_queue_mutex.unlock();
				unsigned int samplesWritten = i;
				PlayerChannels[player].totalSamplesWritten += samplesWritten;
				if ( samplesWritten != 0 && (samplesWritten < PlayerChannels[player].minimumSamplesWritten) )
				{
					PlayerChannels[player].minimumSamplesWritten = samplesWritten;
					PlayerChannels[player].adjustedLatency = std::max(samplesWritten, PlayerChannels[player].desiredLatency);
				}
				if ( PlayerChannels[player].audioQueue.size() >= PlayerChannels_t::audioQueueSizeLimit )
				{
					logInfo("warning: VoiceChat_t::receivePacket() audio queue full");
				}
			}
		}
	}

	if ( multiplayer == SERVER )
	{
		if ( player != clientnum ) // don't forward the server's loopback
		{
			for ( int i = 1; i < MAXPLAYERS; ++i )
			{
				if ( i != player && !client_disconnected[i] && !(voice_no_recv & (1 << i)) )
				{
					// forward this on to other clients
					packet->address.host = net_clients[i - 1].host;
					packet->address.port = net_clients[i - 1].port;
					sendPacket(net_sock, -1, packet, i - 1);
				}
			}
		}
	}
}

VoiceChat_t::RingBuffer::RingBuffer(int sizeBytes)
{
	_data = new char[sizeBytes];
	memset(_data, 0, sizeBytes);
	_size = sizeBytes;
	_readPtr = 0;
	_writePtr = 0;
	_writeBytesAvail = sizeBytes;
}

VoiceChat_t::RingBuffer::~RingBuffer()
{
	delete[] _data;
}

bool VoiceChat_t::RingBuffer::Empty()
{
	memset(_data, 0, _size);
	_readPtr = 0;
	_writePtr = 0;
	_writeBytesAvail = _size;
	return true;
}

int VoiceChat_t::RingBuffer::Read(char* dataPtr, int numBytes)
{
	if ( dataPtr == 0 || numBytes <= 0 || _writeBytesAvail == _size )
	{
		return 0;
	}

	int readBytesAvail = _size - _writeBytesAvail;

	if ( numBytes > readBytesAvail )
	{
		numBytes = readBytesAvail;
	}

	if ( numBytes > _size - _readPtr )
	{
		int len = _size - _readPtr;
		memcpy(dataPtr, _data + _readPtr, len);
		memcpy(dataPtr + len, _data, numBytes - len);
	}
	else
	{
		memcpy(dataPtr, _data + _readPtr, numBytes);
	}

	_readPtr = (_readPtr + numBytes) % _size;
	_writeBytesAvail += numBytes;

	return numBytes;
}

int VoiceChat_t::RingBuffer::Write(char* dataPtr, int numBytes)
{
	if ( dataPtr == 0 || numBytes <= 0 || _writeBytesAvail == 0 )
	{
		return 0;
	}

	if ( numBytes > _writeBytesAvail )
	{
		numBytes = _writeBytesAvail;
	}

	if ( numBytes > _size - _writePtr )
	{
		int len = _size - _writePtr;
		memcpy(_data + _writePtr, dataPtr, len);
		memcpy(_data, dataPtr + len, numBytes - len);
	}
	else
	{
		memcpy(_data + _writePtr, dataPtr, numBytes);
	}

	_writePtr = (_writePtr + numBytes) % _size;
	_writeBytesAvail -= numBytes;

	return numBytes;
}

int VoiceChat_t::RingBuffer::GetSize()
{
	return _size;
}

int VoiceChat_t::RingBuffer::GetWriteAvail()
{
	return _writeBytesAvail;
}

int VoiceChat_t::RingBuffer::GetReadAvail()
{
	return _size - _writeBytesAvail;
}

EnsembleSounds_t ensembleSounds;

FMOD_RESULT F_CALLBACK ensembleExplorationCallback(FMOD_CHANNELCONTROL* channelcontrol,
	FMOD_CHANNELCONTROL_TYPE controltype,
	FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype,
	void* commanddata1,
	void* commanddata2);

FMOD_RESULT F_CALLBACK ensembleCombatCallback(FMOD_CHANNELCONTROL* channelcontrol,
	FMOD_CHANNELCONTROL_TYPE controltype,
	FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype,
	void* commanddata1,
	void* commanddata2);

FMOD_VECTOR ensemble_global_position {0.f, 0.f, 0.f};

void EnsembleSounds_t::playSong()
{
	songTransitionState = TRANSITION_EXPLORE;
	unsigned int dsp_block_len = 0;
	fmod_result = fmod_system->getDSPBufferSize(&dsp_block_len, 0);
	int outputrate = 0;
	fmod_result = fmod_system->getSoftwareFormat(&outputrate, 0, 0);
	FMODErrorCheck();
	unsigned long long clock_start = 0;

	float buffer_ms = (dsp_block_len * 1000.f / (float)outputrate);

	for ( int i = 0; i < NUMENSEMBLEMUSIC; i++ )
	{
		fmod_result = fmod_system->playSound(exploreSound[i], transceiver_group[i], true, &exploreChannel[i]);
		fmod_result = exploreChannel[i]->setCallback(ensembleExplorationCallback);

		fmod_result = fmod_system->playSound(combatSound[i], transceiver_group[i], true, &combatChannel[i]);
		fmod_result = combatChannel[i]->setCallback(ensembleCombatCallback);

		unsigned int songPos = 0;
		FMOD_SYNCPOINT* syncPoint = nullptr;
		fmod_result = exploreSound[i]->getSyncPoint(exploreSongSeek, &syncPoint);
		fmod_result = exploreSound[i]->getSyncPointInfo(syncPoint, nullptr, 0, &songPos, FMOD_TIMEUNIT_PCM);
		fmod_result = exploreChannel[i]->setPosition(songPos, FMOD_TIMEUNIT_PCM);

		unsigned long long clock_now = 0;
		fmod_result = exploreChannel[i]->getDSPClock(0, &clock_now);
		if ( !clock_start )
		{
			clock_start = clock_now;
			clock_start += (dsp_block_len * 5);
		}
		assert(clock_now < clock_start);

		/*if ( !clock_start )
		{
		}
		else
		{
			float freq;
			unsigned int slen = 0;
			fmod_result = sound[count]->getLength(&slen, FMOD_TIMEUNIT_PCM);

			fmod_result = sound[count]->getDefaults(&freq, 0);
			slen = (unsigned int)((float)slen / freq * outputrate);

			clock_start += slen;
		}*/

		exploreChannel[i]->setDelay(clock_start, 0, false);

		unsigned long long fade_delay = (1000.f / buffer_ms) * dsp_block_len;
		fmod_result = exploreChannel[i]->removeFadePoints(0, (unsigned long long)(-1));
		fmod_result = exploreChannel[i]->addFadePoint(clock_start, 0.f);
		fmod_result = exploreChannel[i]->addFadePoint(clock_start + fade_delay, 1.f);

		fmod_result = exploreChannel[i]->setPaused(false);

		transceiver_group[i]->setVolume(0.f);

		fmod_result = exploreChannel[i]->setMode(FMOD_3D_HEADRELATIVE);
		fmod_result = exploreChannel[i]->set3DAttributes(&ensemble_global_position, nullptr);
		fmod_result = combatChannel[i]->setMode(FMOD_3D_HEADRELATIVE);
		fmod_result = combatChannel[i]->set3DAttributes(&ensemble_global_position, nullptr);
	}
}

void EnsembleSounds_t::stopPlaying(bool setCombatDelay)
{
	songTransitionState = TRANSITION_EXPLORE;
	if ( setCombatDelay )
	{
		combatDelay = TICKS_PER_SECOND * 5;
	}
	for ( int i = 0; i < NUMENSEMBLEMUSIC; ++i )
	{
		fmod_result = exploreChannel[i] ? exploreChannel[i]->stop() : FMOD_OK;
		exploreChannel[i] = nullptr;
		fmod_result = combatChannel[i] ? combatChannel[i]->stop() : FMOD_OK;
		combatChannel[i] = nullptr;

		for ( int j = 0; j < NUM_COMBAT_TRANS; ++j )
		{
			fmod_result = combatTransChannel[j][i] ? combatTransChannel[j][i]->stop() : FMOD_OK;
			combatTransChannel[j][i] = nullptr;
		}
		for ( int j = 0; j < NUM_EXPLORE_TRANS; ++j )
		{
			fmod_result = exploreTransChannel[j][i] ? exploreTransChannel[j][i]->stop() : FMOD_OK;
			exploreTransChannel[j][i] = nullptr;
		}
	}
}

void EnsembleSounds_t::deinit()
{
	stopPlaying(true);

	for ( int i = 0; i < NUMENSEMBLEMUSIC; ++i )
	{
		fmod_result = exploreChannel[i] ? exploreChannel[i]->stop() : FMOD_OK;
		exploreChannel[i] = nullptr;
		fmod_result = combatChannel[i] ? combatChannel[i]->stop() : FMOD_OK;
		combatChannel[i] = nullptr;
		fmod_result = exploreSound[i] ? exploreSound[i]->release() : FMOD_OK;
		exploreSound[i] = nullptr;
		fmod_result = combatSound[i] ? combatSound[i]->release() : FMOD_OK;
		combatSound[i] = nullptr;

		for ( int j = 0; j < NUM_COMBAT_TRANS; ++j )
		{
			fmod_result = combatTransChannel[j][i] ? combatTransChannel[j][i]->stop() : FMOD_OK;
			combatTransChannel[j][i] = nullptr;
			fmod_result = combatTransSound[j][i] ? combatTransSound[j][i]->release() : FMOD_OK;
			combatTransSound[j][i] = nullptr;
		}
		for ( int j = 0; j < NUM_EXPLORE_TRANS; ++j )
		{
			fmod_result = exploreTransChannel[j][i] ? exploreTransChannel[j][i]->stop() : FMOD_OK;
			exploreTransChannel[j][i] = nullptr;
			fmod_result = exploreTransSound[j][i] ? exploreTransSound[j][i]->release() : FMOD_OK;
			exploreTransSound[j][i] = nullptr;
		}
	}
}

static ConsoleCommand ccmd_ensemble_transition_mode("/ensemble_transition_mode", "",
	[](int argc, const char* argv[]) {
		if ( argc > 1 )
		{
			ensembleSounds.songTransitionMode = (EnsembleSounds_t::TransitionMode)atoi(argv[1]);
			messagePlayer(clientnum, MESSAGE_HINT, "Set transition mode to: %d", (int)ensembleSounds.songTransitionMode);
		}
});

static ConsoleCommand ccmd_ensemble_transition_state("/ensemble_transition_state", "",
	[](int argc, const char* argv[]) {
		if ( argc > 1 )
		{
			ensembleSounds.songTransitionState = (EnsembleSounds_t::SongTransitionState)atoi(argv[1]);
			messagePlayer(clientnum, MESSAGE_HINT, "Set transition state to: %d", (int)ensembleSounds.songTransitionState);
		}
	});

bool checkSoundReady(FMOD::Sound* sound)
{
	if ( !sound ) { return true; }
	FMOD_OPENSTATE openState = FMOD_OPENSTATE_LOADING;
	unsigned int percentBuffered = 0;
	bool starving = false;
	bool diskBusy = false;
	fmod_result = sound->getOpenState(&openState, &percentBuffered, &starving, &diskBusy);

	if ( fmod_result == FMOD_OK )
	{
		if ( openState == FMOD_OPENSTATE_READY )
		{
			return true;
		}
	}

	return false;
}

void EnsembleSounds_t::setup()
{
	songTransitionState = TRANSITION_EXPLORE;

	if ( no_sound )
	{
		return;
	}

	Uint32 startTick = SDL_GetTicks();

	for ( int i = 0; i < NUMENSEMBLEMUSIC; )
	{
		bool result = checkSoundReady(exploreSound[i]);
		if ( !result )
		{
			i = 0;
			continue;
		}
		++i;
	}

	for ( int i = 0; i < NUMENSEMBLEMUSIC; )
	{
		bool result = checkSoundReady(combatSound[i]);
		if ( !result )
		{
			i = 0;
			continue;
		}
		++i;
	}

	for ( int i = 0; i < NUMENSEMBLEMUSIC; )
	{
		bool result = false;
		for ( int j = 0; j < NUM_COMBAT_TRANS; ++j )
		{
			result = checkSoundReady(combatTransSound[j][i]);
			if ( !result )
			{
				break;
			}
		}

		if ( !result )
		{
			i = 0;
			continue;
		}

		for ( int j = 0; j < NUM_EXPLORE_TRANS; ++j )
		{
			result = checkSoundReady(exploreTransSound[j][i]);
			if ( !result )
			{
				break;
			}
		}

		if ( !result )
		{
			i = 0;
			continue;
		}

		++i;
	}

	Uint32 endTick = SDL_GetTicks();

	printlog("EnsembleSounds_t::setup() in %lums", endTick - startTick);

	for ( int i = 0; i < NUMENSEMBLEMUSIC; ++i )
	{
		fmod_result = exploreChannel[i] ? exploreChannel[i]->stop() : FMOD_OK;
		fmod_result = combatChannel[i] ? combatChannel[i]->stop() : FMOD_OK;
		//fmod_result = exploreSound[i] ? exploreSound[i]->release() : FMOD_OK;
		//fmod_result = combatSound[i] ? combatSound[i]->release() : FMOD_OK;
		for ( int j = 0; j < NUM_COMBAT_TRANS; ++j )
		{
			fmod_result = combatTransChannel[j][i] ? combatTransChannel[j][i]->stop() : FMOD_OK;
			//fmod_result = combatTransSound[j][i] ? combatTransSound[j][i]->release() : FMOD_OK;
		}
		for ( int j = 0; j < NUM_EXPLORE_TRANS; ++j )
		{
			fmod_result = exploreTransChannel[j][i] ? exploreTransChannel[j][i]->stop() : FMOD_OK;
			//fmod_result = exploreTransSound[j][i] ? exploreTransSound[j][i]->release() : FMOD_OK;
		}

		if ( !transceiver_group[i] )
		{
			fmod_result = fmod_system->createChannelGroup(nullptr, &transceiver_group[i]);

			FMOD::DSP* transceiver = nullptr;
			fmod_result = fmod_system->createDSPByType(FMOD_DSP_TYPE_TRANSCEIVER, &transceiver);
			fmod_result = transceiver_group[i]->addDSP(0, transceiver);
			fmod_result = transceiver->setParameterBool(FMOD_DSP_TRANSCEIVER_TRANSMIT, true); // sending signal
			fmod_result = transceiver->setParameterInt(FMOD_DSP_TRANSCEIVER_CHANNEL, 1 + i); // sending on channel x

			music_ensemble_global_send_group->addGroup(transceiver_group[i]);
		}

		if ( exploreSound[i] )
		{
			int syncPoints = 0;
			exploreSound[i]->getNumSyncPoints(&syncPoints);
			for ( int point = 0; point < syncPoints; ++point )
			{
				FMOD_SYNCPOINT* syncpoint = nullptr;
				fmod_result = exploreSound[i]->getSyncPoint(point, &syncpoint);
				if ( syncpoint )
				{
					exploreSound[i]->deleteSyncPoint(syncpoint);
				}
			}

			FMOD_SOUND_TYPE type;
			int chan = 0;
			unsigned int len = 0;
			exploreSound[i]->getLength(&len, FMOD_TIMEUNIT_PCM);
			exploreSound[i]->getFormat(&type, nullptr, &chan, nullptr);
			float freq = 0;
			exploreSound[i]->getDefaults(&freq, nullptr);
			int beatTime = freq * (60 / 120.f); // beats per sample, 120bpm

			int interval = 1;
			int numBeats = 2;
			exploreSoundSyncPointInterval = interval * (beatTime * numBeats);
			unsigned int beat = beatTime * 3 / 4; // starting point from beginning of song
			//exploreSound[i]->setLoopPoints(0, FMOD_TIMEUNIT_PCM, len - beat, FMOD_TIMEUNIT_PCM);
			int numSyncPoints = ((len - beat) / (interval * beatTime * numBeats)) + 1;
			int index = -1;
			if ( i == 0 )
			{
				exploreSyncPoints.clear();
				exploreSyncPointsToSeek.clear();
				exploreSyncPointsUnique.clear();
			}
			int seekBar = -1;
			int oldSeekBar = -1;
			while ( beat < len )
			{
				++index;
				if ( i == 0 )
				{
					if ( index == 0 )
					{
						seekBar = -1; // no action
					}
					else if ( index < 166 )
					{
						seekBar = (index / 8) * 8;
					}
					else if ( index < 310 )
					{
						seekBar = 166 + ((index - 166) / 8) * 8;
					}
					else if ( index == 310 || index == 311 )
					{
						seekBar = 302;
					}
					else if ( index < 360 )
					{
						seekBar = 312 + ((index - 312) / 8) * 8;
					}
					else if ( index == 360 || index == 361 )
					{
						seekBar = 352;
					}
					else if ( index >= 362 )
					{
						seekBar = 362;
					}
					if ( oldSeekBar != seekBar )
					{
						exploreSyncPointsUnique.push_back(seekBar);
					}
					oldSeekBar = seekBar;
				}
				fmod_result = exploreSound[i]->addSyncPoint(beat, FMOD_TIMEUNIT_PCM, nullptr, nullptr);
				if ( i == 0 )
				{
					exploreSyncPoints.push_back(beat);
					exploreSyncPointsToSeek.push_back(seekBar);
				}
				FMODErrorCheck();
				beat += interval * (beatTime * numBeats);
			}

			fmod_result = fmod_system->playSound(exploreSound[i], transceiver_group[i], true, &exploreChannel[i]);
			fmod_result = exploreChannel[i]->setMode(FMOD_3D_HEADRELATIVE);
			fmod_result = exploreChannel[i]->set3DAttributes(&ensemble_global_position, nullptr);
			FMODErrorCheck();
			fmod_result = exploreChannel[i]->setCallback(ensembleExplorationCallback);
		}

		if ( combatSound[i] )
		{
			int syncPoints = 0;
			combatSound[i]->getNumSyncPoints(&syncPoints);
			for ( int point = 0; point < syncPoints; ++point )
			{
				FMOD_SYNCPOINT* syncpoint = nullptr;
				combatSound[i]->getSyncPoint(point, &syncpoint);
				if ( syncpoint )
				{
					combatSound[i]->deleteSyncPoint(syncpoint);
				}
			}

			FMOD_SOUND_TYPE type;
			int chan = 0;
			unsigned int len = 0;
			combatSound[i]->getLength(&len, FMOD_TIMEUNIT_PCM);
			combatSound[i]->getFormat(&type, nullptr, &chan, nullptr);
			float freq = 0;
			combatSound[i]->getDefaults(&freq, nullptr);
			int beatTime = freq * (60 / 90.f) / 8.f; // eighths, 90bpm
			combatBeat = beatTime;
			int interval = 2;
			int numBeats = 7;
			combatSoundSyncPointInterval = interval * (beatTime * numBeats);
			unsigned int beat = beatTime * 0; // starting point from beginning of song
			int numSyncPoints = ((len - beat) / (interval * beatTime * numBeats)) + 1;
			int index = -1;
			if ( i == 0 )
			{
				combatSyncPoints.clear();
			}
			while ( beat < len )
			{
				++index;
				fmod_result = combatSound[i]->addSyncPoint(beat, FMOD_TIMEUNIT_PCM, nullptr, nullptr);
				if ( i == 0 )
				{
					combatSyncPoints.push_back(beat);
				}
				FMODErrorCheck();
				beat += interval * beatTime * numBeats;
			}

			fmod_result = fmod_system->playSound(combatSound[i], transceiver_group[i], true, &combatChannel[i]);
			fmod_result = combatChannel[i]->setMode(FMOD_3D_HEADRELATIVE);
			fmod_result = combatChannel[i]->set3DAttributes(&ensemble_global_position, nullptr);
			FMODErrorCheck();
			combatChannel[i]->setCallback(ensembleCombatCallback);
		}

		for ( int j = 0; j < NUM_EXPLORE_TRANS; ++j )
		{
			if ( !exploreTransSound[j][i] )
			{
				continue;
			}

			int syncPoints = 0;
			exploreTransSound[j][i]->getNumSyncPoints(&syncPoints);
			for ( int point = 0; point < syncPoints; ++point )
			{
				FMOD_SYNCPOINT* syncpoint = nullptr;
				fmod_result = exploreTransSound[j][i]->getSyncPoint(point, &syncpoint);
				if ( syncpoint )
				{
					exploreTransSound[j][i]->deleteSyncPoint(syncpoint);
				}
			}

			FMOD_SOUND_TYPE type;
			int chan = 0;
			unsigned int len = 0;
			exploreTransSound[j][i]->getLength(&len, FMOD_TIMEUNIT_PCM);
			exploreTransSound[j][i]->getFormat(&type, nullptr, &chan, nullptr);
			float freq = 0;
			exploreTransSound[j][i]->getDefaults(&freq, nullptr);
			int beatTime = freq * (60 / 120.f); // beats per sample, 120bpm

			int interval = 1;
			int numBeats = 4;
			unsigned int beat = interval * beatTime * numBeats;
			if ( j == 3 )
			{
				fmod_result = exploreTransSound[j][i]->addSyncPoint(0, FMOD_TIMEUNIT_PCM, nullptr, nullptr);
				fmod_result = exploreTransSound[j][i]->addSyncPoint(8 * beat / 16, FMOD_TIMEUNIT_PCM, nullptr, nullptr);
			}
			else
			{
				fmod_result = exploreTransSound[j][i]->addSyncPoint(4 * beat / 8, FMOD_TIMEUNIT_PCM, nullptr, nullptr);
				fmod_result = exploreTransSound[j][i]->addSyncPoint(beat, FMOD_TIMEUNIT_PCM, nullptr, nullptr);
			}
			FMODErrorCheck();
			//result = fmod_system->playSound(exploreTransSound[j][i], groupTx[i], true, &channelTrans[j][count]);
			//FMODErrorCheck();
		}

		for ( int j = 0; j < NUM_COMBAT_TRANS; ++j )
		{
			if ( !combatTransSound[j][i] )
			{
				continue;
			}

			int syncPoints = 0;
			combatTransSound[j][i]->getNumSyncPoints(&syncPoints);
			for ( int point = 0; point < syncPoints; ++point )
			{
				FMOD_SYNCPOINT* syncpoint = nullptr;
				fmod_result = combatTransSound[j][i]->getSyncPoint(point, &syncpoint);
				if ( syncpoint )
				{
					combatTransSound[j][i]->deleteSyncPoint(syncpoint);
				}
			}

			FMOD_SOUND_TYPE type;
			int chan = 0;
			unsigned int len = 0;
			combatTransSound[j][i]->getLength(&len, FMOD_TIMEUNIT_PCM);
			combatTransSound[j][i]->getFormat(&type, nullptr, &chan, nullptr);
			float freq = 0;
			combatTransSound[j][i]->getDefaults(&freq, nullptr);
			int beatTime = freq * (60 / 90.f); // beats per sample, 120bpm

			int interval = 2;
			if ( j == 3 )
			{
				interval = 1;
			}
			int numBeats = 4;
			unsigned int beat = interval * beatTime * numBeats;
			fmod_result = combatTransSound[j][i]->addSyncPoint(3 * beat / 8, FMOD_TIMEUNIT_PCM, nullptr, nullptr);
			fmod_result = combatTransSound[j][i]->addSyncPoint(beat, FMOD_TIMEUNIT_PCM, nullptr, nullptr);
			FMODErrorCheck();
			//result = fmod_system->playSound(combatTransSound[j][i], groupTx[i], true, &channelCombatTrans[j][count]);
			//FMODErrorCheck();
		}
	}

	ensembleSounds.firstTimeSetup = false;
}

static ConsoleVariable<int> cvar_ensemble_combat_length("/ensemble_combat_length", 30);

FMOD_RESULT F_CALLBACK ensembleCombatCallback(FMOD_CHANNELCONTROL* channelcontrol,
	FMOD_CHANNELCONTROL_TYPE controltype,
	FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype,
	void* commanddata1,
	void* commanddata2)
{
	FMOD_RESULT result;
	if ( callbacktype == FMOD_CHANNELCONTROL_CALLBACK_TYPE::FMOD_CHANNELCONTROL_CALLBACK_SYNCPOINT )
	{
		int currentSyncPoint = reinterpret_cast<intptr_t>(commanddata1);
		FMOD::Channel* chan = reinterpret_cast<FMOD::Channel*>(channelcontrol);
		FMOD::Sound* sound = nullptr;
		chan->getCurrentSound(&sound);
		if ( ensembleSounds.songTransitionState == EnsembleSounds_t::TRANSITION_COMBAT_ENDING )
		{
			if ( sound == ::ensembleSounds.combatSound[0] )
			{
				unsigned int currentPos = 0;
				chan->getPosition(&currentPos, FMOD_TIMEUNIT_PCM);

				FMOD_SYNCPOINT* syncPoint = nullptr;
				sound->getSyncPoint(currentSyncPoint, &syncPoint);
				unsigned int currentOffset = 0;
				result = sound->getSyncPointInfo(syncPoint, nullptr, 0, &currentOffset, FMOD_TIMEUNIT_PCM);

				if ( currentPos >= currentOffset // check our callback is within expected sync range
					&& (currentPos < currentOffset + ensembleSounds.combatSoundSyncPointInterval) )
				{
					int syncInterval = 8 / 2;
					int syncpointNext = currentSyncPoint + syncInterval - currentSyncPoint % (syncInterval);
					/*if ( currentSyncPoint % (syncInterval * 2) >= syncInterval )
					{
						syncpointNext += syncInterval;
					}*/

					bool noTransition = false;
					if ( ensembleSounds.ticksCombatPlaying < *cvar_ensemble_combat_length * TICKS_PER_SECOND )
					{
						noTransition = true;
					}

					if ( ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_FADE
						|| ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_FADE_HALF
						|| noTransition )
					{
						syncpointNext = currentSyncPoint + 1;
					}

					FMOD_SYNCPOINT* syncPoint2 = nullptr;
					int numSyncPoints = 0;
					sound->getNumSyncPoints(&numSyncPoints);
					while ( syncpointNext >= numSyncPoints )
					{
						syncpointNext -= numSyncPoints;
					}

					sound->getSyncPoint(syncpointNext, &syncPoint2);

					unsigned int soundLength = 0;
					sound->getLength(&soundLength, FMOD_TIMEUNIT_PCM);

					unsigned int soundPos = 0;
					chan->getPosition(&soundPos, FMOD_TIMEUNIT_PCM);

					unsigned int offset2 = 0;
					result = sound->getSyncPointInfo(syncPoint2, nullptr, 0, &offset2, FMOD_TIMEUNIT_PCM);

					unsigned int delayLength = 0;
					if ( soundPos > offset2 )
					{
						// wrap ahead
						delayLength = (soundLength - soundPos) + offset2;
					}
					else
					{
						delayLength = offset2 - soundPos;
					}

					ensembleSounds.songTransitionState = EnsembleSounds_t::TRANSITION_COMBAT_ENDED;

					//chan->addFadePoint(clock_now + (unsigned long long)(rate * 2.0), 0.f);
					int combatTransition = 0;
					if ( syncpointNext >= 28 && syncpointNext <= 32 )
					{
						combatTransition = 1;
					}
					if ( ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_FADE_HALF )
					{
						combatTransition = 0;
					}

					float volume = 0.f;
					chan->getVolume(&volume);
					/*if ( ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_FADE_HALF && volume < 0.5f )
					{
						noTransition = true;
						delayLength = ensembleSounds.combatSoundSyncPointInterval;
					}*/
					auto mainChannelDelay = delayLength;

					unsigned int buffersize = 0;
					fmod_system->getDSPBufferSize(&buffersize, nullptr);
					int sampleRate = 1;
					fmod_system->getSoftwareFormat(&sampleRate, nullptr, nullptr);
					float buffer_ms = (buffersize * 1000.f / (float)sampleRate);

					for ( int i = 0; i < NUMENSEMBLEMUSIC; ++i )
					{
						unsigned long long clock_now = 0;
						result = ensembleSounds.combatChannel[i]->getDSPClock(0, &clock_now);
						result = ensembleSounds.combatChannel[i]->setDelay(0, clock_now + (unsigned long long)(mainChannelDelay), false);

						if ( ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_DEFAULT && noTransition )
						{
							result = ensembleSounds.combatChannel[i]->addFadePoint(clock_now, 1.f);
							result = ensembleSounds.combatChannel[i]->addFadePoint(clock_now + (unsigned long long)(mainChannelDelay), 0.f);
						}
						else if ( ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_DEFAULT && !noTransition )
						{
							result = ensembleSounds.combatChannel[i]->addFadePoint(clock_now + (unsigned long long)(mainChannelDelay) - 500.f * buffer_ms, 1.f);
							result = ensembleSounds.combatChannel[i]->addFadePoint(clock_now + (unsigned long long)(mainChannelDelay), 0.f);
						}

						unsigned int transitionLength = 0;
						result = ensembleSounds.combatTransSound[combatTransition][i]->getLength(&transitionLength, FMOD_TIMEUNIT_PCM);
						//result = channelCombatTrans[combatTransition][i]->setPaused(true);

						FMOD_SYNCPOINT* syncPoint4 = nullptr;
						unsigned int offset3 = 0;
						if ( ensembleSounds.songTransitionMode != EnsembleSounds_t::TRANSITION_MODE_FADE && !noTransition )
						{
							result = fmod_system->playSound(ensembleSounds.combatTransSound[combatTransition][i], 
								ensembleSounds.transceiver_group[i], true, &ensembleSounds.combatTransChannel[combatTransition][i]);
							result = ensembleSounds.combatTransChannel[combatTransition][i]->setMode(FMOD_3D_HEADRELATIVE);
							result = ensembleSounds.combatTransChannel[combatTransition][i]->set3DAttributes(&ensemble_global_position, nullptr);

							FMOD_SYNCPOINT* syncPoint3 = nullptr;
							result = ensembleSounds.combatTransSound[combatTransition][i]->getSyncPoint(1, &syncPoint3);
							result = ensembleSounds.combatTransSound[combatTransition][i]->getSyncPointInfo(syncPoint3, nullptr, 
								0, &offset3, FMOD_TIMEUNIT_PCM);

							if ( ensembleSounds.songTransitionMode != EnsembleSounds_t::TRANSITION_MODE_FADE_HALF )
							{
								result = ensembleSounds.combatTransChannel[combatTransition][i]->setPosition(0, FMOD_TIMEUNIT_PCM);
							}
							else if ( ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_FADE_HALF )
							{
								result = ensembleSounds.combatTransSound[combatTransition][i]->getSyncPoint(0, &syncPoint4);
								unsigned int offset4 = 0;
								result = ensembleSounds.combatTransSound[combatTransition][i]->getSyncPointInfo(syncPoint4, nullptr,
									0, &offset4, FMOD_TIMEUNIT_PCM);

								ensembleSounds.combatTransChannel[combatTransition][i]->setPosition(offset4, FMOD_TIMEUNIT_PCM);

								unsigned int beat = ensembleSounds.combatBeat * 64;
								offset3 = offset3 - offset4;
								offset3 -= beat / 4;
								transitionLength -= offset4;
							}

							result = ensembleSounds.combatTransChannel[combatTransition][i]->setDelay(clock_now + (unsigned long long)(delayLength),
								clock_now + (unsigned long long)(delayLength) + transitionLength, true);
							{
								unsigned long long delay = 0;
								result = ensembleSounds.combatTransChannel[combatTransition][i]->getDelay(&delay, nullptr);

								result = ensembleSounds.combatTransChannel[combatTransition][i]->removeFadePoints(0, (unsigned long long)(-1));
								result = ensembleSounds.combatTransChannel[combatTransition][i]->addFadePoint(delay, 0.f);
								unsigned long long fade_delay = (10.f / buffer_ms) * buffersize;
								result = ensembleSounds.combatTransChannel[combatTransition][i]->addFadePoint(delay + fade_delay, 1.f);
							}

							result = ensembleSounds.combatTransChannel[combatTransition][i]->setPaused(false);
						}

						result = ensembleSounds.exploreChannel[i]->setPaused(true);
						result = ensembleSounds.exploreChannel[i]->setPosition(0, FMOD_TIMEUNIT_PCM);

						//ensembleSounds.exploreChannel[i]->setVolume(0.f);
						//ensembleSounds.combatTransChannel[combatTransition][i]->setVolume(0.f);

						// code to seek to position if needed, but have pickup notes at 0:00 currently
						{
							unsigned int songPos = 0;
							result = ensembleSounds.exploreSound[i]->getSyncPoint(ensembleSounds.exploreSongSeek, &syncPoint);
							result = ensembleSounds.exploreSound[i]->getSyncPointInfo(syncPoint, nullptr, 0, &songPos, FMOD_TIMEUNIT_PCM);
							result = ensembleSounds.exploreChannel[i]->setPosition(songPos, FMOD_TIMEUNIT_PCM);
						}

						result = ensembleSounds.exploreChannel[i]->setDelay(clock_now + (unsigned long long)(delayLength + offset3), 0, false);

						{
							unsigned long long delay = 0;
							result = ensembleSounds.exploreChannel[i]->getDelay(&delay, nullptr);

							unsigned long long fade_delay = (1000.f / buffer_ms) * buffersize;

							result = ensembleSounds.exploreChannel[i]->removeFadePoints(0, (unsigned long long)(-1));
							result = ensembleSounds.exploreChannel[i]->addFadePoint(delay, 0.f);
							result = ensembleSounds.exploreChannel[i]->addFadePoint(delay + fade_delay, 1.f);
						}
						result = ensembleSounds.exploreChannel[i]->setPaused(false);
					}
				}
			}
		}

		if ( false && currentSyncPoint % 8 == 0 )
		{
			unsigned int currentOffset = 0;
			FMOD_SYNCPOINT* syncPoint = nullptr;
			result = sound->getSyncPoint(currentSyncPoint, &syncPoint);
			result = sound->getSyncPointInfo(syncPoint, nullptr, 0, &currentOffset, FMOD_TIMEUNIT_PCM);

			currentSyncPoint = ensembleSounds.combatSongSeek * 8;

			unsigned int offset = 0;
			result = sound->getSyncPoint(currentSyncPoint, &syncPoint);
			result = sound->getSyncPointInfo(syncPoint, nullptr, 0, &offset, FMOD_TIMEUNIT_PCM);

			unsigned int currentPos = 0;
			result = chan->getPosition(&currentPos, FMOD_TIMEUNIT_PCM);

			// this goes crazy and calls back every missed sync point, twice for the same setpoint
			if ( currentPos >= currentOffset // check our callback is within expected sync range
				&& (currentPos < currentOffset + ensembleSounds.combatSoundSyncPointInterval) // check our callback is within expected sync range
				&& (abs((int)currentPos - (int)offset) > ((ensembleSounds.combatSoundSyncPointInterval) * 0.9)) ) // check our position is sufficiently far and not a double up
			{
				//currentPos = currentPos % soundCombatSyncPointInterval; // callback interval
				currentPos = currentPos - currentOffset;
				result = chan->setPosition(offset + currentPos, FMOD_TIMEUNIT_PCM);
			}
			//chan->getPosition(&pos, FMOD_TIMEUNIT_MS);
		}
	}

	return FMOD_OK;
}

FMOD_RESULT F_CALLBACK ensembleExplorationCallback(FMOD_CHANNELCONTROL* channelcontrol,
	FMOD_CHANNELCONTROL_TYPE controltype,
	FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype,
	void* commanddata1,
	void* commanddata2)
{
	FMOD_RESULT result = FMOD_OK;
	if ( callbacktype == FMOD_CHANNELCONTROL_CALLBACK_TYPE::FMOD_CHANNELCONTROL_CALLBACK_SYNCPOINT )
	{
		int currentSyncPoint = reinterpret_cast<intptr_t>(commanddata1);
		FMOD::Channel* chan = reinterpret_cast<FMOD::Channel*>(channelcontrol);
		FMOD::Sound* sound = nullptr;
		chan->getCurrentSound(&sound);

		if ( sound == ::ensembleSounds.exploreSound[0] ) // should this be [5] ??
		{
			if ( ensembleSounds.songTransitionState == EnsembleSounds_t::TRANSITION_COMBAT_ENDED )
			{
				ensembleSounds.songTransitionState = EnsembleSounds_t::TRANSITION_EXPLORE;
			}
			if ( ensembleSounds.songTransitionState == EnsembleSounds_t::TRANSITION_COMBAT_START && currentSyncPoint > 0 )
			{
				unsigned int currentPos = 0;
				chan->getPosition(&currentPos, FMOD_TIMEUNIT_PCM);

				FMOD_SYNCPOINT* syncPoint = nullptr;
				sound->getSyncPoint(currentSyncPoint, &syncPoint);
				unsigned int currentOffset = 0;
				result = sound->getSyncPointInfo(syncPoint, nullptr, 0, &currentOffset, FMOD_TIMEUNIT_PCM);

				if ( currentPos >= currentOffset // check our callback is within expected sync range
					&& (currentPos < currentOffset + ensembleSounds.exploreSoundSyncPointInterval) )
				{
					int syncInterval = 8 / 2;
					int syncpointNext = currentSyncPoint + syncInterval - currentSyncPoint % (syncInterval);
					if ( currentSyncPoint % (syncInterval * 2) >= syncInterval )
					{
						syncpointNext += syncInterval;
					}

					if ( ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_FADE
						|| ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_FADE_HALF
						|| ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_DEFAULT )
					{
						syncpointNext = currentSyncPoint + 1;
					}

					FMOD_SYNCPOINT* syncPoint2 = nullptr;
					int numSyncPoints = 0;
					sound->getNumSyncPoints(&numSyncPoints);
					while ( syncpointNext >= numSyncPoints )
					{
						syncpointNext -= numSyncPoints;
					}

					sound->getSyncPoint(syncpointNext, &syncPoint2);

					unsigned int soundLength = 0;
					sound->getLength(&soundLength, FMOD_TIMEUNIT_PCM);

					unsigned int offset2 = 0;
					result = sound->getSyncPointInfo(syncPoint2, nullptr, 0, &offset2, FMOD_TIMEUNIT_PCM);

					unsigned int delayLength = 0;
					if ( currentPos > offset2 )
					{
						// wrap ahead
						delayLength = (soundLength - currentPos) + offset2;
					}
					else
					{
						delayLength = offset2 - currentPos;
					}

					ensembleSounds.songTransitionState = EnsembleSounds_t::TRANSITION_COMBAT;

					//chan->addFadePoint(clock_now + (unsigned long long)(rate * 2.0), 0.f);
					int explorationTransition = 2;
					if ( ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_DEFAULT )
					{
						explorationTransition = 3;
					}
					else if ( syncpointNext == 20 )
					{
						explorationTransition = 1;
					}
					else if ( syncpointNext == 28 )
					{
						explorationTransition = 0;
					}

					float volume = 0.f;
					chan->getVolume(&volume);
					bool noTransition = false;
					if ( ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_FADE_HALF && volume < 0.5f )
					{
						//noTransition = true;
						delayLength = ensembleSounds.exploreSoundSyncPointInterval / 4;
					}
					auto mainChannelDelay = delayLength * 2;
					if ( ensembleSounds.songTransitionMode != EnsembleSounds_t::TRANSITION_MODE_DEFAULT )
					{
						mainChannelDelay = delayLength;
					}

					unsigned int buffersize = 0;
					fmod_system->getDSPBufferSize(&buffersize, nullptr);
					int sampleRate = 1;
					fmod_system->getSoftwareFormat(&sampleRate, nullptr, nullptr);
					float buffer_ms = (buffersize * 1000.f / (float)sampleRate);

					for ( int i = 0; i < NUMENSEMBLEMUSIC; ++i )
					{
						unsigned long long clock_now = 0;
						result = ensembleSounds.exploreChannel[i]->getDSPClock(0, &clock_now);
						result = ensembleSounds.exploreChannel[i]->setDelay(0, clock_now + (unsigned long long)(mainChannelDelay), false);

						{
							result = ensembleSounds.exploreChannel[i]->removeFadePoints(0, (unsigned long long)(-1));
							result = ensembleSounds.exploreChannel[i]->addFadePoint(clock_now, 1.f);
							result = ensembleSounds.exploreChannel[i]->addFadePoint(clock_now + (unsigned long long)(mainChannelDelay), 0.f);
						}

						unsigned int transitionLength = 0;
						result = ensembleSounds.exploreTransSound[explorationTransition][i]->getLength(&transitionLength, FMOD_TIMEUNIT_PCM);
						//result = channelTrans[explorationTransition][i]->setPaused(true);

						FMOD_SYNCPOINT* syncPoint4 = nullptr;
						unsigned int offset3 = 0;
						if ( ensembleSounds.songTransitionMode != EnsembleSounds_t::TRANSITION_MODE_FADE && !noTransition )
						{
							if ( ensembleSounds.exploreTransChannel[explorationTransition][i] )
							{
								ensembleSounds.exploreTransChannel[explorationTransition][i]->stop();
							}
							result = fmod_system->playSound(ensembleSounds.exploreTransSound[explorationTransition][i], 
								ensembleSounds.transceiver_group[i], true, &ensembleSounds.exploreTransChannel[explorationTransition][i]);
							result = ensembleSounds.exploreTransChannel[explorationTransition][i]->setMode(FMOD_3D_HEADRELATIVE);
							result = ensembleSounds.exploreTransChannel[explorationTransition][i]->set3DAttributes(&ensemble_global_position, nullptr);

							FMOD_SYNCPOINT* syncPoint3 = nullptr;
							result = ensembleSounds.exploreTransSound[explorationTransition][i]->getSyncPoint(1, &syncPoint3);
							result = ensembleSounds.exploreTransSound[explorationTransition][i]->getSyncPointInfo(syncPoint3, nullptr, 
								0, &offset3, FMOD_TIMEUNIT_PCM);

							if ( ensembleSounds.songTransitionMode != EnsembleSounds_t::TRANSITION_MODE_FADE_HALF )
							{
								result = ensembleSounds.exploreTransChannel[explorationTransition][i]->setPosition(0, FMOD_TIMEUNIT_PCM);
							}
							else if ( ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_FADE_HALF 
								|| ensembleSounds.songTransitionMode == EnsembleSounds_t::TRANSITION_MODE_DEFAULT )
							{
								result = ensembleSounds.exploreTransSound[explorationTransition][i]->getSyncPoint(0, &syncPoint4);
								unsigned int offset4 = 0;
								result = ensembleSounds.exploreTransSound[explorationTransition][i]->getSyncPointInfo(syncPoint4, nullptr, 
									0, &offset4, FMOD_TIMEUNIT_PCM);

								ensembleSounds.exploreTransChannel[explorationTransition][i]->setPosition(offset4, FMOD_TIMEUNIT_PCM);

								offset3 = offset3 - offset4;
								transitionLength -= offset4;
							}

							result = ensembleSounds.exploreTransChannel[explorationTransition][i]->setDelay(clock_now + (unsigned long long)(delayLength),
								clock_now + (unsigned long long)(delayLength) + transitionLength, true);

							{
								unsigned long long delay = 0;
								result = ensembleSounds.exploreTransChannel[explorationTransition][i]->getDelay(&delay, nullptr);
								result = ensembleSounds.exploreTransChannel[explorationTransition][i]->removeFadePoints(0, (unsigned long long)(-1));
								result = ensembleSounds.exploreTransChannel[explorationTransition][i]->addFadePoint(delay, 0.f);
								unsigned long long fade_delay = (500.f / buffer_ms) * buffersize;
								result = ensembleSounds.exploreTransChannel[explorationTransition][i]->addFadePoint(delay + fade_delay, 1.f);
							}

							result = ensembleSounds.exploreTransChannel[explorationTransition][i]->setPaused(false);
						}

						result = ensembleSounds.combatChannel[i]->setPaused(true);

						unsigned int songPos = 0;
						result = ensembleSounds.combatSound[i]->getSyncPoint(ensembleSounds.combatSongSeek * 8, &syncPoint);
						result = ensembleSounds.combatSound[i]->getSyncPointInfo(syncPoint, nullptr, 0, &songPos, FMOD_TIMEUNIT_PCM);
						result = ensembleSounds.combatChannel[i]->setPosition(songPos, FMOD_TIMEUNIT_PCM);

						//result = ensembleSounds.combatChannel[i]->setVolume(0.f);
						//result = ensembleSounds.exploreTransChannel[explorationTransition][i]->setVolume(0.f);

						result = ensembleSounds.combatChannel[i]->removeFadePoints(0, (unsigned long long)(-1));
						result = ensembleSounds.combatChannel[i]->setDelay(clock_now + (unsigned long long)(delayLength + offset3), 0, false);
						result = ensembleSounds.combatChannel[i]->setPaused(false);
					}
				}
			}
		}

		if ( currentSyncPoint == 0 /*currentSyncPoint % 8 == 0*/ )
		{
			unsigned int currentOffset = 0;
			FMOD_SYNCPOINT* syncPoint = nullptr;
			result = sound->getSyncPoint(currentSyncPoint, &syncPoint);
			result = sound->getSyncPointInfo(syncPoint, nullptr, 0, &currentOffset, FMOD_TIMEUNIT_PCM);

			currentSyncPoint = ensembleSounds.exploreSongSeek;

			unsigned int offset = 0;
			result = sound->getSyncPoint(currentSyncPoint, &syncPoint);
			result = sound->getSyncPointInfo(syncPoint, nullptr, 0, &offset, FMOD_TIMEUNIT_PCM);

			unsigned int currentPos = 0;
			result = chan->getPosition(&currentPos, FMOD_TIMEUNIT_PCM);

			// this goes crazy and calls back every missed sync point, twice for the same setpoint
			if ( currentPos >= currentOffset // check our callback is within expected sync range
				&& (currentPos < currentOffset + ensembleSounds.exploreSoundSyncPointInterval) // check our callback is within expected sync range
				&& (abs((int)currentPos - (int)offset) > ((ensembleSounds.exploreSoundSyncPointInterval) * 0.9)) ) // check our position is sufficiently far and not a double up
			{
				//currentPos = currentPos % soundSyncPointInterval; // callback interval
				currentPos = currentPos - currentOffset;
				result = chan->setPosition(offset + currentPos, FMOD_TIMEUNIT_PCM);
			}
		}
	}

	return FMOD_OK;
}

ConsoleVariable<int> cvar_ensemble_explore_seek("/ensemble_explore_seek", -1);
ConsoleVariable<int> cvar_ensemble_combat_seek("/ensemble_combat_seek", -1);
void EnsembleSounds_t::updatePlayingChannelVolumes()
{
	bool fade_only_mode = (songTransitionMode == TRANSITION_MODE_FADE);
	static ConsoleVariable<float> cvar_ensemble_fade_in("/ensemble_fade_in", 0.02f);
	static ConsoleVariable<float> cvar_ensemble_fade_out("/ensemble_fade_out", 0.01f);
	float fadeoutspeed = fade_only_mode ? 2 * *cvar_ensemble_fade_out : *cvar_ensemble_fade_out;
	float fadeinspeed = *cvar_ensemble_fade_in;

	if ( songTransitionMode == TRANSITION_MODE_FULL )
	{
		fadeoutspeed = 0.f;
		fadeinspeed = 1.f;
	}

	if ( exploreChannel[5] )
	{
		bool playing = false;
		float audibility = 0.f;
		exploreChannel[5]->getAudibility(&audibility);
		exploreChannel[5]->isPlaying(&playing);

		if ( playing && audibility > 0.01 )
		{
			unsigned int pos = 0;
			exploreChannel[5]->getPosition(&pos, FMOD_TIMEUNIT_PCM);
			{
				int lastPoint = -1;
				int index = -1;
				for ( auto& point : exploreSyncPoints )
				{
					++index;
					if ( index == 0 )
					{
						continue;
					}
					if ( point > pos )
					{
						break;
					}
					lastPoint = index;
				}
				if ( lastPoint >= 0 )
				{
					if ( *cvar_ensemble_explore_seek >= 0 )
					{
						lastPoint = *cvar_ensemble_explore_seek;
					}
					if ( lastPoint >= (exploreSyncPointsToSeek.size() - 4) )
					{
						exploreSongSeek = 0; // loop back to beginning
					}
					else
					{
						if ( lastPoint < exploreSyncPointsToSeek.size() )
						{
							exploreSongSeek = exploreSyncPointsToSeek[lastPoint];
						}
						else
						{
							exploreSongSeek = 0; // loop back to beginning, unknown error?
						}
					}
				}
			}
		}
	}

	if ( combatChannel[5] )
	{
		bool playing = false;
		float audibility = 0.f;
		combatChannel[5]->getAudibility(&audibility);
		combatChannel[5]->isPlaying(&playing);
		bool paused = true;
		combatChannel[5]->getPaused(&paused);

		if ( playing && !paused && audibility > 0.0001 )
		{
			unsigned int pos = 0;
			combatChannel[5]->getPosition(&pos, FMOD_TIMEUNIT_PCM);
			{
				int lastPoint = -1;
				int index = -1;
				for ( auto& point : combatSyncPoints )
				{
					++index;
					if ( index % 8 != 0 )
					{
						continue;
					}
					if ( point > pos )
					{
						break;
					}
					lastPoint = index;
				}
				if ( lastPoint >= 0 )
				{
					if ( *cvar_ensemble_combat_seek >= 0 )
					{
						lastPoint = *cvar_ensemble_combat_seek;
					}
					combatSongSeek = lastPoint / 8;
				}
			}
		}
		if ( playing && !paused && audibility > 0.0001 )
		{
			if ( lastUpdateTick != ticks )
			{
				++ticksCombatPlaying;
				lastTickCombatPlaying = ticks;
			}
		}
		else
		{
			ticksCombatPlaying = 0;
		}
	}
	else
	{
		ticksCombatPlaying = 0;
	}

	if ( lastUpdateTick != ticks )
	{
		if ( combat && songTransitionState == TRANSITION_COMBAT_START )
		{
			messagePlayer(clientnum, MESSAGE_DEBUG, "Combat start");
		}
		else if ( !combat && songTransitionState == TRANSITION_COMBAT_ENDING )
		{
			messagePlayer(clientnum, MESSAGE_DEBUG, "Combat end");
		}
		if ( combatDelay > 0 )
		{
			--combatDelay;
		}
	}
	lastUpdateTick = ticks;

	// non-callback volume control if needed
	for ( int i = 0; i < NUMENSEMBLEMUSIC && false; ++i )
	{
		if ( songTransitionState == TRANSITION_EXPLORE || songTransitionState == TRANSITION_COMBAT_ENDED )
		{
			bool playing = false;
			float audibility = 0.f;
			if ( exploreChannel[i] )
			{
				exploreChannel[i]->getAudibility(&audibility);
				exploreChannel[i]->isPlaying(&playing);
				if ( playing )
				{
					unsigned int ms = 0;
					exploreChannel[i]->getPosition(&ms, FMOD_TIMEUNIT_MS);
					if ( ms > 250 /*|| fade_only_mode*/ )
					{
						float volume = 0.f;
						exploreChannel[i]->getVolume(&volume);
						volume = std::min(1.f, volume + fadeinspeed);
						exploreChannel[i]->setVolume(volume);
					}
				}
				else
				{
					exploreChannel[i]->setVolume(0.f);
				}
			}

			playing = false;
			audibility = 0.f;
			if ( combatChannel[i] )
			{
				combatChannel[i]->getAudibility(&audibility);
				combatChannel[i]->isPlaying(&playing);
				if ( playing )
				{
					float volume = 0.f;
					combatChannel[i]->getVolume(&volume);
					volume = std::max(0.f, volume - fadeoutspeed);
					combatChannel[i]->setVolume(volume);
				}
				else
				{
					combatChannel[i]->setVolume(0.f);
				}
			}

			for ( int j = 0; j < 3; ++j )
			{
				if ( !exploreTransChannel[j][i] ) { continue; }
				playing = false;
				audibility = 0.f;
				exploreTransChannel[j][i]->getAudibility(&audibility);
				exploreTransChannel[j][i]->isPlaying(&playing);
				if ( playing )
				{
					float volume = 0.f;
					exploreTransChannel[j][i]->getVolume(&volume);
					volume = std::max(0.f, volume - fadeoutspeed);
					exploreTransChannel[j][i]->setVolume(volume);
				}
				else
				{
					exploreTransChannel[j][i]->setVolume(0.f);
				}
			}

			for ( int j = 0; j < 4; ++j )
			{
				if ( !combatTransChannel[j][i] ) { continue; }
				playing = false;
				audibility = 0.f;
				combatTransChannel[j][i]->getAudibility(&audibility);
				combatTransChannel[j][i]->isPlaying(&playing);
				if ( playing )
				{
					float volume = 0.f;
					combatTransChannel[j][i]->getVolume(&volume);
					volume = std::min(1.f, volume + fadeinspeed);
					combatTransChannel[j][i]->setVolume(volume);
				}
				else
				{
					combatTransChannel[j][i]->setVolume(0.f);
				}
			}
		}
		else if ( songTransitionState == TRANSITION_COMBAT )
		{
			bool playing = false;
			float audibility = 0.f;
			if ( exploreChannel[i] )
			{
				exploreChannel[i]->getAudibility(&audibility);
				exploreChannel[i]->isPlaying(&playing);
				if ( playing )
				{
					float volume = 0.f;
					exploreChannel[i]->getVolume(&volume);
					volume = std::max(0.f, volume - fadeoutspeed);
					exploreChannel[i]->setVolume(volume);
				}
				else
				{
					exploreChannel[i]->setVolume(0.f);
				}
			}

			playing = false;
			audibility = 0.f;
			if ( combatChannel[i] )
			{
				combatChannel[i]->getAudibility(&audibility);
				combatChannel[i]->isPlaying(&playing);
				if ( playing )
				{
					float volume = 0.f;
					combatChannel[i]->getVolume(&volume);
					volume = std::min(1.f, volume + fadeinspeed);
					combatChannel[i]->setVolume(volume);
				}
				else
				{
					combatChannel[i]->setVolume(0.f);
				}
			}

			for ( int j = 0; j < 3; ++j )
			{
				if ( !exploreTransChannel[j][i] ) { continue; }
				playing = false;
				audibility = 0.f;
				exploreTransChannel[j][i]->getAudibility(&audibility);
				exploreTransChannel[j][i]->isPlaying(&playing);
				if ( playing )
				{
					float volume = 0.f;
					exploreTransChannel[j][i]->getVolume(&volume);
					volume = std::min(1.f, volume + fadeinspeed);
					exploreTransChannel[j][i]->setVolume(volume);
				}
				else
				{
					exploreTransChannel[j][i]->setVolume(0.f);
				}
			}

			for ( int j = 0; j < 4; ++j )
			{
				if ( !combatTransChannel[j][i] ) { continue; }
				playing = false;
				audibility = 0.f;
				combatTransChannel[j][i]->getAudibility(&audibility);
				combatTransChannel[j][i]->isPlaying(&playing);
				if ( playing )
				{
					float volume = 0.f;
					combatTransChannel[j][i]->getVolume(&volume);
					volume = std::max(0.f, volume - fadeoutspeed);
					combatTransChannel[j][i]->setVolume(volume);
				}
				else
				{
					combatTransChannel[j][i]->setVolume(0.f);
				}
			}
		}
	}
}
#endif