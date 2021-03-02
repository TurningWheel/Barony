/*-------------------------------------------------------------------------------

	BARONY
	File: init.cpp
	Desc: init, load, exit audio engine stuff.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../../main.hpp"
#include "../../files.hpp"
#include "sound.hpp"
#include "../../player.hpp"

bool initSoundEngine()
{
#ifdef USE_FMOD
	printlog("initializing FMOD...\n");
	fmod_result = FMOD::System_Create(&fmod_system);
	if (FMODErrorCheck())
	{
		printlog("Failed to create FMOD. DISABLING AUDIO.\n");
		no_sound = true;
		return false;
	}

	if (!no_sound)
	{
		fmod_result = fmod_system->init(fmod_maxchannels, FMOD_INIT_NORMAL | FMOD_INIT_3D_RIGHTHANDED, fmod_extraDriverData);
		if (FMODErrorCheck())
		{
			printlog("Failed to initialize FMOD. DISABLING AUDIO.\n");
			no_sound = true;
			return false;
		}

		fmod_result = fmod_system->createChannelGroup(nullptr, &sound_group);
		if (FMODErrorCheck())
		{
			printlog("Failed to create sound channel group. DISABLING AUDIO.\n");
			no_sound = true;
			return false;
		}
		fmod_result = FMOD_System_CreateChannelGroup(fmod_system, NULL, &soundAmbient_group); //TODO: Update these for FMOD Studio.
		if ( FMODErrorCheck() )
		{
			printlog("Failed to create sound ambient channel group.\n");
			no_sound = true;
		}
		fmod_result = FMOD_System_CreateChannelGroup(fmod_system, NULL, &soundEnvironment_group);
		if ( FMODErrorCheck() )
		{
			printlog("Failed to create sound environment channel group.\n");
			no_sound = true;
		}
		
		fmod_result = fmod_system->createChannelGroup(nullptr, &music_group);
		if (FMODErrorCheck())
		{
			printlog("Failed to create music channel group. DISABLING AUDIO.\n");
			no_sound = true;
			return false;
		}
	}
#elif defined USE_OPENAL
	if (!no_sound)
	{
		initOPENAL();
	}
#endif

	return !no_sound; //No double negatives pls
}

int loadSoundResources()
{
	FILE* fp;
	Uint32 c;
	char name[128];

	// load sound effects
	std::string soundsDirectory = PHYSFS_getRealDir("sound/sounds.txt");
	soundsDirectory.append(PHYSFS_getDirSeparator()).append("sound/sounds.txt");
#ifdef USE_FMOD
	printlog("loading sounds...\n");
	fp = openDataFile(soundsDirectory.c_str(), "r");
	for ( numsounds = 0; !fp->eof(); ++numsounds )
	{
		while ( fp->getc() != '\n' )
		{
			if ( fp->eof() )
			{
				break;
			}
		}
	}
	FileIO::close(fp);
	if ( numsounds == 0 )
	{
		printlog("failed to identify any sounds in sounds.txt\n");
		return 10;
	}
	sounds = (FMOD::Sound**) malloc(sizeof(FMOD::Sound*)*numsounds);
	fp = openDataFile(soundsDirectory.c_str(), "r");
	for ( c = 0; !fp->eof(); ++c )
	{
		fp->gets2(name, 128);
		fmod_result = fmod_system->createSound(name, (FMOD_DEFAULT | FMOD_3D), nullptr, &sounds[c]);
		if (FMODErrorCheck())
		{
			printlog("warning: failed to load '%s' listed at line %d in sounds.txt\n", name, c + 1);
		}
	}
	FileIO::close(fp);
	sound_group->setVolume(sfxvolume / 128.f);
	FMOD_ChannelGroup_SetVolume(soundAmbient_group, sfxAmbientVolume / 128.f); //TODO: Update these two for FMOD Studio too.
	FMOD_ChannelGroup_SetVolume(soundEnvironment_group, sfxEnvironmentVolume / 128.f);
	fmod_system->set3DSettings(1.0, 2.0, 1.0);
#elif defined USE_OPENAL
	printlog("loading sounds...\n");
	fp = openDataFile(soundsDirectory.c_str(), "r");
	for ( numsounds = 0; !fp->eof(); numsounds++ )
	{
		while ( fp->getc() != '\n' ) if ( fp->eof() )
			{
				break;
			}
	}
	FileIO::close(fp);
	if ( numsounds == 0 )
	{
		printlog("failed to identify any sounds in sounds.txt\n");
		return 10;
	}
	sounds = (OPENAL_BUFFER**) malloc(sizeof(OPENAL_BUFFER*)*numsounds);
	for (c = 0, fp = openDataFile(soundsDirectory.c_str(), "r"); fp->gets2(name, 128); ++c)
	{
		//TODO: Might need to malloc the sounds[c]->sound
		OPENAL_CreateSound(name, true, &sounds[c]);
		//TODO: set sound volume? Or otherwise handle sound volume.
	}
	FileIO::close(fp);
	OPENAL_ChannelGroup_SetVolume(sound_group, sfxvolume / 128.f);
	OPENAL_ChannelGroup_SetVolume(soundAmbient_group, sfxAmbientVolume / 128.f);
	OPENAL_ChannelGroup_SetVolume(soundEnvironment_group, sfxEnvironmentVolume / 128.f);
	//FMOD_System_Set3DSettings(fmod_system, 1.0, 2.0, 1.0); // This on is hardcoded, I've been lazy here'
#endif // defined USE_OPENAL

	return 0;
}

void freeSoundResources()
{
	uint32 c;
	// free sounds
#ifdef USE_FMOD
	printlog("freeing sounds...\n");
	if ( sounds != nullptr )
	{
		for ( c = 0; c < numsounds && !no_sound; c++ )
		{
			if (sounds[c] != nullptr)
			{
				if (sounds[c] != nullptr)
				{
					sounds[c]->release(); //Free the sound's FMOD sound.
				}
			}
		}
		free(sounds); //Then free the sound array.
	}
#endif
}

void exitSoundEngine()
{
#ifdef USE_FMOD
	if ( fmod_system )
	{
		fmod_system->close();
		fmod_system->release();
		fmod_system = nullptr;
	}
#endif
}
