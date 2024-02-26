/*-------------------------------------------------------------------------------

	BARONY
	File: music.cpp
	Desc: Contains any music-specific code that would have gone into sound_game.cpp

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../../main.hpp"
#include "../../game.hpp"
#include "sound.hpp"
#include "../../entity.hpp"
#include "../../player.hpp"
#include "../../prng.hpp"
#include "../../files.hpp"
#ifdef NINTENDO
 #include "../../nintendo/music.hpp"
#else
 #include "music_pc.hpp"
#endif

bool loadMusic()
{
#ifdef USE_FMOD
    if ( NUMMINESMUSIC > 0 )
    {
        minesmusic = (FMOD::Sound**)malloc(sizeof(FMOD::Sound*) * NUMMINESMUSIC);
        memset(minesmusic, 0, sizeof(FMOD::Sound*) * NUMMINESMUSIC);
    }
    if ( NUMSWAMPMUSIC > 0 )
    {
        swampmusic = (FMOD::Sound**)malloc(sizeof(FMOD::Sound*) * NUMSWAMPMUSIC);
        memset(swampmusic, 0, sizeof(FMOD::Sound*) * NUMSWAMPMUSIC);
    }
    if ( NUMLABYRINTHMUSIC > 0 )
    {
        labyrinthmusic = (FMOD::Sound**)malloc(sizeof(FMOD::Sound*) * NUMLABYRINTHMUSIC);
        memset(labyrinthmusic, 0, sizeof(FMOD::Sound*) * NUMLABYRINTHMUSIC);
    }
    if ( NUMRUINSMUSIC > 0 )
    {
        ruinsmusic = (FMOD::Sound**)malloc(sizeof(FMOD::Sound*) * NUMRUINSMUSIC);
        memset(ruinsmusic, 0, sizeof(FMOD::Sound*) * NUMRUINSMUSIC);
    }
    if ( NUMUNDERWORLDMUSIC > 0 )
    {
        underworldmusic = (FMOD::Sound**)malloc(sizeof(FMOD::Sound*) * NUMUNDERWORLDMUSIC);
        memset(underworldmusic, 0, sizeof(FMOD::Sound*) * NUMUNDERWORLDMUSIC);
    }
    if ( NUMHELLMUSIC > 0 )
    {
        hellmusic = (FMOD::Sound**)malloc(sizeof(FMOD::Sound*) * NUMHELLMUSIC);
        memset(hellmusic, 0, sizeof(FMOD::Sound*) * NUMHELLMUSIC);
    }
    if ( NUMMINOTAURMUSIC > 0 )
    {
        minotaurmusic = (FMOD::Sound**)malloc(sizeof(FMOD::Sound*) * NUMMINOTAURMUSIC);
        memset(minotaurmusic, 0, sizeof(FMOD::Sound*) * NUMMINOTAURMUSIC);
    }
    if ( NUMCAVESMUSIC > 0 )
    {
        cavesmusic = (FMOD::Sound**)malloc(sizeof(FMOD::Sound*) * NUMCAVESMUSIC);
        memset(cavesmusic, 0, sizeof(FMOD::Sound*) * NUMCAVESMUSIC);
    }
    if ( NUMCITADELMUSIC > 0 )
    {
        citadelmusic = (FMOD::Sound**)malloc(sizeof(FMOD::Sound*) * NUMCITADELMUSIC);
        memset(citadelmusic, 0, sizeof(FMOD::Sound*) * NUMCITADELMUSIC);
    }
    if ( NUMINTROMUSIC > 0 )
    {
        intromusic = (FMOD::Sound**)malloc(sizeof(FMOD::Sound*) * NUMINTROMUSIC);
        memset(intromusic, 0, sizeof(FMOD::Sound*) * NUMINTROMUSIC);
    }
#endif

    bool introMusicChanged;
	physfsReloadMusic(introMusicChanged, true);
    return true;
}

void stopMusic()
{
#ifdef SOUND
    playMusic(nullptr, false, false, false);
#endif
}

#ifdef USE_FMOD
void playMusic(FMOD::Sound* sound, bool loop, bool crossfade, bool resume)
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
	if (!fmod_system)
	{
		printlog("Can't play music.\n");
		return;
	}
	if ( resume && music_channel2 )
	{
		FMOD::Sound* lastmusic = nullptr;
		music_channel2->getCurrentSound(&lastmusic);
		if ( lastmusic == sound )
		{
			FMOD::Channel* tempmusic = music_channel;
			music_channel = music_channel2;
			music_channel2 = tempmusic;
		}
		else
		{
			music_channel2->stop();
			music_channel2 = music_channel;
			music_channel = nullptr;
			if (sound) {
			    fmod_result = fmod_system->playSound(sound, music_group, true, &music_channel);
			}
		}
	}
	else
	{
		music_channel2->stop();
		music_channel2 = music_channel;
		music_channel = nullptr;
		if (sound) {
		    fmod_result = fmod_system->playSound(sound, music_group, true, &music_channel);
		}
	}
	//FMOD_Channel_SetChannelGroup(music_channel, music_group);
	if (crossfade == true)
	{
		//Start at volume 0 to get louder.
		music_channel->setVolume(0.0f); //Start at 0 then pop up.
	}
	else
	{
		music_channel->setVolume(1.0f); // start at max volume
		music_channel2->stop();
	}
	if (loop == true)
	{
		//Loop the channel.
		FMOD_MODE mode;
		music_channel->getMode(&mode);
		fmod_result = music_channel->setMode(FMOD_LOOP_NORMAL);
		FMODErrorCheck();
	}
	music_channel->setPaused(false);
	if (FMODErrorCheck())
	{
		return; // What?
	}
}
#endif

bool shopmusicplaying = false;
bool combatmusicplaying = false;
bool minotaurmusicplaying = false;
bool herxmusicplaying = false;
bool devilmusicplaying = false;
bool olddarkmap = false;
bool sanctummusicplaying = false;

int currenttrack = -1;

#ifdef USE_FMOD
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

	if ( !strcmp(map.name, "Mages Guild") )
	{
		inshop = false; // everywhere is shop!
	}

	bool devilaround = false;
	bool activeminotaur = false;
	bool herxaround = false;
	bool magisteraround = false;
	node_t* node;
	for ( node = map.creatures->first; node != nullptr; node = node->next )
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

	bool playing = true;
	music_channel->isPlaying(&playing);

	if ( currenttrack == -1 )
	{
		currenttrack = local_rng.rand();
	}

	if ( (!levelmusicplaying || !playing || olddarkmap != darkmap) && (!combat || !strcmp(map.name, "Hell Boss")) 
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
		else if ( !strncmp(map.name, "Tutorial ", 9) )
		{
			playMusic(tutorialmusic, true, true, true);
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
#endif
