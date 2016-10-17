/*-------------------------------------------------------------------------------

	BARONY
	File: sound_game.cpp
	Desc: Contains all the code that will cause the editor to crash and burn.
	Quick workaround because I don't want to separate the editor and game
	into two separate projects just because of sound.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "sound.hpp"
#include "entity.hpp"
#include "net.hpp"
#include "player.hpp"

/*-------------------------------------------------------------------------------

	playSoundPlayer
	
	has the given player play the specified global sound effect. Mostly
	used by the server to instruct clients to play a certain sound.

-------------------------------------------------------------------------------*/

#ifdef HAVE_FMOD
FMOD_CHANNEL* playSoundPlayer(int player, Uint32 snd, int vol) {
	if (no_sound) {
		return NULL;
	}


	if( player<0 || player>=MAXPLAYERS ) //Perhaps this can be reprogrammed to remove MAXPLAYERS, and use a pointer to the player instead of an int?
		return NULL;
	if( player==clientnum ) {
		return playSound(snd,vol);
	} else if( multiplayer==SERVER ) {
		if( client_disconnected[player] )
			return NULL;
		strcpy((char *)net_packet->data,"SNDG");
		SDLNet_Write32(snd,&net_packet->data[4]);
		SDLNet_Write32((Uint32)vol,&net_packet->data[8]);
		net_packet->address.host = net_clients[player-1].host;
		net_packet->address.port = net_clients[player-1].port;
		net_packet->len = 12;
		sendPacketSafe(net_sock, -1, net_packet, player-1);
		return NULL;
	}

	return NULL;
}

/*-------------------------------------------------------------------------------

	playSoundPos
	
	plays a sound effect with the given volume at the given
	position; returns the channel that the sound is playing in

-------------------------------------------------------------------------------*/

FMOD_CHANNEL* playSoundPos(double x, double y, Uint32 snd, int vol) {
	if (no_sound) {
		return NULL;
	}

	#ifndef SOUND
	return NULL;
	#endif

	FMOD_CHANNEL *channel;
	int c;

	if (intro)
		return NULL;
	if (snd < 0 || snd >= numsounds)
		return NULL;
	if (sounds[snd] == NULL || vol == 0)
		return NULL;

	if(multiplayer==SERVER) {
		for(c=1; c<MAXPLAYERS; c++) {
			if( client_disconnected[c]==TRUE )
				continue;
			strcpy((char *)net_packet->data,"SNDP");
			SDLNet_Write32(x,&net_packet->data[4]);
			SDLNet_Write32(y,&net_packet->data[8]);
			SDLNet_Write32(snd,&net_packet->data[12]);
			SDLNet_Write32((Uint32)vol,&net_packet->data[16]);
			net_packet->address.host = net_clients[c-1].host;
			net_packet->address.port = net_clients[c-1].port;
			net_packet->len = 20;
			sendPacketSafe(net_sock, -1, net_packet, c-1);
		}
	}

	if (!fmod_system) //For the client.
		return NULL;

	fmod_result = FMOD_System_PlaySound(fmod_system, FMOD_CHANNEL_FREE, sounds[snd], TRUE, &channel);
	if (FMODErrorCheck()) {
		return NULL;
	}
	FMOD_Channel_SetVolume(channel, vol/128.f);
	FMOD_VECTOR position;
	position.x = -y / 16; //Left/right.
	position.y = 0; //Up/down. //Should be z, but that's not passed. Ignore? Ignoring. Useful for sounds in the floor and ceiling though.
	position.z = -x / 16; //Forward/backward.
	FMOD_Channel_Set3DAttributes(channel, &position, NULL);
	FMOD_Channel_SetChannelGroup(channel, sound_group);
	FMOD_Channel_SetPaused(channel, FALSE);

	return channel;
}

FMOD_CHANNEL* playSoundPosLocal(double x, double y, Uint32 snd, int vol) {
	if (no_sound) {
		return NULL;
	}

	#ifndef SOUND
	return NULL;
	#endif

	FMOD_CHANNEL *channel;

	if (intro)
		return NULL;
	if (snd < 0 || snd >= numsounds)
		return NULL;
	if (sounds[snd] == NULL || vol == 0)
		return NULL;

	if (!fmod_system) //For the client.
		return NULL;

	fmod_result = FMOD_System_PlaySound(fmod_system, FMOD_CHANNEL_FREE, sounds[snd], TRUE, &channel);
	if (FMODErrorCheck()) {
		return NULL;
	}
	FMOD_Channel_SetVolume(channel, vol/128.f);
	FMOD_VECTOR position;
	position.x = -y / 16; //Left/right.
	position.y = 0; //Up/down. //Should be z, but that's not passed. Ignore? Ignoring. Useful for sounds in the floor and ceiling though.
	position.z = -x / 16; //Forward/backward.
	FMOD_Channel_Set3DAttributes(channel, &position, NULL);
	FMOD_Channel_SetChannelGroup(channel, sound_group);
	FMOD_Channel_SetPaused(channel, FALSE);

	return channel;
}

/*-------------------------------------------------------------------------------

	playSoundEntity
	
	plays a sound effect with the given volume at the given entity's
	position; returns the channel that the sound is playing in

-------------------------------------------------------------------------------*/

FMOD_CHANNEL* playSoundEntity(Entity *entity, Uint32 snd, int vol) {
	if (no_sound) {
		return NULL;
	}

	if( entity==NULL )
		return NULL;
	return playSoundPos(entity->x, entity->y, snd, vol);
}

FMOD_CHANNEL* playSoundEntityLocal(Entity *entity, Uint32 snd, int vol) {
	if( entity==NULL )
		return NULL;
	return playSoundPosLocal(entity->x, entity->y, snd, vol);
}

/*-------------------------------------------------------------------------------

	playSound
	
	plays a sound effect with the given volume and returns the channel that
	the sound is playing in

-------------------------------------------------------------------------------*/

FMOD_CHANNEL* playSound(Uint32 snd, int vol) {
	if (no_sound) {
		return NULL;
	}
	#ifndef SOUND
	return NULL;
	#endif
	if (!fmod_system || snd < 0 || snd >= numsounds || !sound_group)
		return NULL;
	if (sounds[snd] == NULL || vol == 0)
		return NULL;
	FMOD_CHANNEL *channel;
	fmod_result = FMOD_System_PlaySound(fmod_system, FMOD_CHANNEL_FREE, sounds[snd], TRUE, &channel);
	//Faux 3D. Set to 0 and then set the channel's mode to be relative  to the player's head to achieve global sound.
	FMOD_VECTOR position;
	position.x = 0;
	position.y = 0;
	position.z = 0;
	FMOD_Channel_Set3DAttributes(channel, &position, NULL);
	FMOD_Channel_SetChannelGroup(channel, sound_group);
	FMOD_Channel_SetVolume(channel, vol/128.f);
	FMOD_Channel_SetMode(channel, FMOD_3D_HEADRELATIVE);
	if (FMODErrorCheck()) {
		return NULL;
	}
	FMOD_Channel_SetPaused(channel, FALSE);
	return channel;
}

void playmusic(FMOD_SOUND *sound, bool loop, bool crossfade, bool resume) {
	if (no_sound) {
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
	if (!fmod_system || !sound) {
		printlog("Can't play music.\n");
		return;
	}
	if( resume && music_channel2 ) {
		FMOD_SOUND *lastmusic = NULL;
		FMOD_Channel_GetCurrentSound(music_channel2,&lastmusic);
		if( lastmusic==sound ) {
			FMOD_CHANNEL *tempmusic = music_channel;
			music_channel = music_channel2;
			music_channel2 = tempmusic;
		} else {
			FMOD_Channel_Stop(music_channel2);
			music_channel2 = music_channel;
			music_channel = NULL;
			fmod_result = FMOD_System_PlaySound(fmod_system, FMOD_CHANNEL_FREE, sound, TRUE, &music_channel);
		}
	} else {
		FMOD_Channel_Stop(music_channel2);
		music_channel2 = music_channel;
		music_channel = NULL;
		fmod_result = FMOD_System_PlaySound(fmod_system, FMOD_CHANNEL_FREE, sound, TRUE, &music_channel);
	}
	FMOD_Channel_SetChannelGroup(music_channel, music_group);
	if (crossfade == TRUE) {
		//Start at volume 0 to get louder.
		FMOD_Channel_SetVolume(music_channel, 0.0f); //Start at 0 then pop up.
	} else {
		FMOD_Channel_Stop(music_channel2);
	}
	if (loop == TRUE) {
		//Loop the channel.
		FMOD_MODE mode;
		FMOD_Channel_GetMode(music_channel, &mode);
		//fmod_result = FMOD_Channel_SetMode(music_channel, (FMOD_MODE)(mode | FMOD_LOOP_NORMAL));
		fmod_result = FMOD_Channel_SetMode(music_channel, FMOD_LOOP_NORMAL);
		FMODErrorCheck();
	}
	FMOD_Channel_SetPaused(music_channel, FALSE);
	if (FMODErrorCheck()) {
		return;
	}
}

bool shopmusicplaying=FALSE;
bool combatmusicplaying=FALSE;
bool minotaurmusicplaying=FALSE;
bool herxmusicplaying=FALSE;
bool devilmusicplaying=FALSE;
bool olddarkmap=FALSE;

int currenttrack=-1;

void handleLevelMusic() {
	if (no_sound) {
		return;
	}
	#ifndef SOUND
		return;
	#endif
	#ifndef MUSIC
		return;
	#endif
	bool inshop=FALSE;
	if (players[clientnum] && players[clientnum]->entity)
	{
		int x = (int)players[clientnum]->entity->x/16;
		int y = (int)players[clientnum]->entity->y/16;
		if( x>=0 && x<map.width && y>=0 && y<map.height )
			if( shoparea[y+x*map.height] )
				inshop=TRUE;
	}

	bool devilaround=FALSE;
	bool activeminotaur=FALSE;
	bool herxaround=FALSE;
	node_t *node;
	for( node=map.entities->first; node!=NULL; node=node->next ) {
		Entity *entity = (Entity *)node->element;
		if( entity->sprite==274 ) { // herx head
			herxaround=TRUE;
			break;
		} else if( entity->sprite==304 ) { // devil body
			devilaround=TRUE;
			break;
		} else if( entity->sprite==237 ) { // minotaur head
			activeminotaur=TRUE;
			break;
		}
	}
	
	FMOD_BOOL playing=TRUE;
	FMOD_Channel_IsPlaying(music_channel,&playing);

	if( currenttrack==-1 )
		currenttrack = rand();

	if( (!levelmusicplaying || !playing || olddarkmap!=darkmap) && (!combat || !strcmp(map.name,"Hell Boss")) && !inshop && (!activeminotaur || !strcmp(map.name,"Hell Boss")) && !herxaround && !devilaround ) {
		if( darkmap ) {
			playmusic(intermissionmusic, TRUE, TRUE, TRUE);
		} else if( !strncmp(map.name,"The Mines",9) ) { // the mines
			if( !playing ) {
				currenttrack = 1+rand()%(NUMMINESMUSIC-1);
			}
			currenttrack = currenttrack%NUMMINESMUSIC;
			if( currenttrack==0 )
				currenttrack=1;
			playmusic(minesmusic[currenttrack], FALSE, TRUE, TRUE);
		} else if( !strncmp(map.name,"The Swamp",9) ) { // the swamp
			if( !playing ) {
				currenttrack = 1+rand()%(NUMSWAMPMUSIC-1);
			}
			currenttrack = currenttrack%NUMSWAMPMUSIC;
			if( currenttrack==0 )
				currenttrack=1;
			playmusic(swampmusic[currenttrack], FALSE, TRUE, TRUE);
		} else if( !strncmp(map.name,"The Labyrinth",13) ) { // the labyrinth
			if( !playing ) {
				currenttrack = 1+rand()%(NUMLABYRINTHMUSIC-1);
			}
			currenttrack = currenttrack%NUMLABYRINTHMUSIC;
			if( currenttrack==0 )
				currenttrack=1;
			playmusic(labyrinthmusic[currenttrack], FALSE, TRUE, TRUE);
		} else if( !strncmp(map.name,"The Ruins",9) ) { // the ruins
			if( !playing ) {
				currenttrack = 1+rand()%(NUMRUINSMUSIC-1);
			}
			currenttrack = currenttrack%NUMRUINSMUSIC;
			if( currenttrack==0 )
				currenttrack=1;
			playmusic(ruinsmusic[currenttrack], FALSE, TRUE, TRUE);
		} else if( !strncmp(map.name,"Underworld",10) ) { // the underworld
			if( !playing ) {
				currenttrack = 1+rand()%(NUMUNDERWORLDMUSIC-1);
			}
			currenttrack = currenttrack%NUMUNDERWORLDMUSIC;
			if( currenttrack==0 )
				currenttrack=1;
			playmusic(underworldmusic[currenttrack], FALSE, TRUE, TRUE);
		} else if( !strcmp(map.name,"Minetown") || !strcmp(map.name,"The Gnomish Mines") ) { // minetown & gnomish mines
			playmusic(minetownmusic, TRUE, TRUE, TRUE);
		} else if( !strcmp(map.name,"The Mystic Library") ) { // mystic library
			playmusic(librarymusic, TRUE, TRUE, TRUE);
		} else if( !strcmp(map.name,"The Minotaur Maze") ) { // minotaur maze
			playmusic(minotaurmusic[1], TRUE, TRUE, TRUE);
		} else if( !strcmp(map.name,"The Temple") ) { // the temple
			playmusic(templemusic, TRUE, TRUE, TRUE);
		} else if( !strcmp(map.name,"Hell Boss") ) { // escape theme
			playmusic(escapemusic, TRUE, TRUE, TRUE);
		} else if( !strncmp(map.name,"Hell",4) ) { // hell
			if( !playing ) {
				currenttrack = 1+rand()%(NUMHELLMUSIC-1);
			}
			currenttrack = currenttrack%NUMHELLMUSIC;
			if( currenttrack==0 )
				currenttrack=1;
			playmusic(hellmusic[currenttrack], FALSE, TRUE, TRUE);
		} else {
			playmusic(intermissionmusic, TRUE, TRUE, TRUE);
		}
		olddarkmap = darkmap;
		levelmusicplaying=TRUE;
		devilmusicplaying=FALSE;
		herxmusicplaying=FALSE;
		minotaurmusicplaying=FALSE;
		combatmusicplaying=FALSE;
		shopmusicplaying=FALSE;
		fadein_increment = default_fadein_increment;
		fadeout_increment = default_fadeout_increment;
	} else if( (!devilmusicplaying || !playing) && devilaround ) {
		playmusic(devilmusic, TRUE, TRUE, TRUE);
		levelmusicplaying=FALSE;
		devilmusicplaying=TRUE;
		herxmusicplaying=FALSE;
		minotaurmusicplaying=FALSE;
		combatmusicplaying=FALSE;
		shopmusicplaying=FALSE;
		fadein_increment = default_fadein_increment*2;
		fadeout_increment = default_fadeout_increment*2;
	} else if( (!herxmusicplaying || !playing) && !devilaround && herxaround ) {
		playmusic(herxmusic, TRUE, TRUE, TRUE);
		levelmusicplaying=FALSE;
		devilmusicplaying=FALSE;
		herxmusicplaying=TRUE;
		minotaurmusicplaying=FALSE;
		combatmusicplaying=FALSE;
		shopmusicplaying=FALSE;
		fadein_increment = default_fadein_increment*2;
		fadeout_increment = default_fadeout_increment*2;
	} else if( (!minotaurmusicplaying || !playing) && !herxaround && activeminotaur && strcmp(map.name,"Hell Boss") ) {
		playmusic(minotaurmusic[0], TRUE, TRUE, TRUE);
		levelmusicplaying=FALSE;
		devilmusicplaying=FALSE;
		herxmusicplaying=FALSE;
		minotaurmusicplaying=TRUE;
		combatmusicplaying=FALSE;
		shopmusicplaying=FALSE;
		fadein_increment = default_fadein_increment*5;
		fadeout_increment = default_fadeout_increment*5;
	} else if( (!combatmusicplaying || !playing) && !herxaround && !activeminotaur && combat && strcmp(map.name,"Hell Boss") ) {
		if( !strncmp(map.name,"The Swamp",9) || !strncmp(map.name,"The Temple",10) ) { // the swamp
			playmusic(swampmusic[0], TRUE, TRUE, TRUE);
		} else if( !strncmp(map.name,"The Labyrinth",13) || strstr(map.name,"Minotaur") ) { // the labyrinth
			playmusic(labyrinthmusic[0], TRUE, TRUE, TRUE);
		} else if( !strncmp(map.name,"The Ruins",9) ) { // the ruins
			playmusic(ruinsmusic[0], TRUE, TRUE, TRUE);
		} else if( !strncmp(map.name,"Underworld",10) ) { // the underworld
			playmusic(underworldmusic[0], TRUE, TRUE, TRUE);
		} else if( !strncmp(map.name,"Hell",4) ) { // hell
			playmusic(hellmusic[0], TRUE, TRUE, TRUE);
		} else {
			playmusic(minesmusic[0], TRUE, TRUE, TRUE);
		}
		levelmusicplaying=FALSE;
		devilmusicplaying=FALSE;
		herxmusicplaying=FALSE;
		combatmusicplaying=TRUE;
		shopmusicplaying=FALSE;
		minotaurmusicplaying=FALSE;
		fadein_increment = default_fadein_increment*4;
		fadeout_increment = default_fadeout_increment;
	} else if( (!shopmusicplaying || !playing) && !herxaround && !activeminotaur && !combat && inshop ) {
		playmusic(shopmusic, TRUE, TRUE, TRUE);
		levelmusicplaying=FALSE;
		devilmusicplaying=FALSE;
		herxmusicplaying=FALSE;
		minotaurmusicplaying=FALSE;
		combatmusicplaying=FALSE;
		shopmusicplaying=TRUE;
		fadein_increment = default_fadein_increment*4;
		fadeout_increment = default_fadeout_increment;
	}
}

#else

/*-------------------------------------------------------------------------------

	playSoundEntity
	
	plays a sound effect with the given volume at the given entity's
	position; returns the channel that the sound is playing in

-------------------------------------------------------------------------------*/
void* playSound(Uint32 snd, int vol) {
	return NULL;
}

void* playSoundPos(double x, double y, Uint32 snd, int vol) {
	int c;

	if (intro || vol == 0)
		return nullptr;

	if(multiplayer==SERVER) {
		for(c=1; c<MAXPLAYERS; c++) {
			if( client_disconnected[c]==TRUE )
				continue;
			strcpy((char *)net_packet->data,"SNDP");
			SDLNet_Write32(x,&net_packet->data[4]);
			SDLNet_Write32(y,&net_packet->data[8]);
			SDLNet_Write32(snd,&net_packet->data[12]);
			SDLNet_Write32((Uint32)vol,&net_packet->data[16]);
			net_packet->address.host = net_clients[c-1].host;
			net_packet->address.port = net_clients[c-1].port;
			net_packet->len = 20;
			sendPacketSafe(net_sock, -1, net_packet, c-1);
		}
	}

	return NULL;
}

void* playSoundPosLocal(double x, double y, Uint32 snd, int vol) {
	return NULL;
}

void* playSoundEntity(Entity *entity, Uint32 snd, int vol) {
	if (entity == NULL) return NULL;
	return playSoundPos(entity->x, entity->y, snd, vol);
}

void* playSoundEntityLocal(Entity *entity, Uint32 snd, int vol) {
	if( entity==NULL )
		return NULL;
	return playSoundPosLocal(entity->x, entity->y, snd, vol);
}

void* playSoundPlayer(int player, Uint32 snd, int vol) {
	int c;

	if( player<0 || player>=MAXPLAYERS ) //Perhaps this can be reprogrammed to remove MAXPLAYERS, and use a pointer to the player instead of an int?
		return NULL;
	if( player==clientnum ) {
		return playSound(snd,vol);
	} else if( multiplayer==SERVER ) {
		if( client_disconnected[player] )
			return NULL;
		strcpy((char *)net_packet->data,"SNDG");
		SDLNet_Write32(snd,&net_packet->data[4]);
		SDLNet_Write32((Uint32)vol,&net_packet->data[8]);
		net_packet->address.host = net_clients[player-1].host;
		net_packet->address.port = net_clients[player-1].port;
		net_packet->len = 12;
		sendPacketSafe(net_sock, -1, net_packet, player-1);
		return NULL;
	}

	return NULL;
}

#endif
