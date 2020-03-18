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
	
	fmod_result = fmod_system->createChannelGroup(nullptr, &music_group);
	if (FMODErrorCheck())
	{
		printlog("Failed to create music channel group. DISABLING AUDIO.\n");
		no_sound = true;
		return false;
	}
#elif defined USE_OPENAL
	if (!no_sound)
	{
		initOPENAL();
	}
#endif

	return !no_sound;
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
	for ( numsounds = 0; !feof(fp); numsounds++ )
	{
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
		{
			break;
		}
	}
	fclose(fp);
	if ( numsounds == 0 )
	{
		printlog("failed to identify any sounds in sounds.txt\n");
		return 10;
	}
	sounds = (FMOD::Sound**) malloc(sizeof(FMOD::Sound*)*numsounds);
	fp = openDataFile(soundsDirectory.c_str(), "r");
	for ( c = 0; !feof(fp); c++ )
	{
		fscanf(fp, "%s", name);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
		{
			break;
		}
		fmod_result = fmod_system->createSound(name, (FMOD_DEFAULT | FMOD_3D), nullptr, &sounds[c]);
		if (FMODErrorCheck())
		{
			printlog("warning: failed to load '%s' listed at line %d in sounds.txt\n", name, c + 1);
		}
	}
	fclose(fp);
	sound_group->setVolume(sfxvolume / 128.f);
	fmod_system->set3DSettings(1.0, 2.0, 1.0);
#elif defined USE_OPENAL
	printlog("loading sounds...\n");
	fp = openDataFile(soundsDirectory.c_str(), "r");
	for ( numsounds = 0; !feof(fp); numsounds++ )
	{
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
	}
	fclose(fp);
	if ( numsounds == 0 )
	{
		printlog("failed to identify any sounds in sounds.txt\n");
		return 10;
	}
	sounds = (OPENAL_BUFFER**) malloc(sizeof(OPENAL_BUFFER*)*numsounds);
	fp = openDataFile(soundsDirectory.c_str(), "r");
	for ( c = 0; !feof(fp); c++ )
	{
		fscanf(fp, "%s", name);
		while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
		//TODO: Might need to malloc the sounds[c]->sound
		OPENAL_CreateSound(name, true, &sounds[c]);
		//TODO: set sound volume? Or otherwise handle sound volume.
	}
	fclose(fp);
	OPENAL_ChannelGroup_SetVolume(sound_group, sfxvolume / 128.f);
	//FMOD_System_Set3DSettings(fmod_system, 1.0, 2.0, 1.0); // This on is hardcoded, I've been lazy here'
#endif

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
