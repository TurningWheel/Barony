/*-------------------------------------------------------------------------------

	BARONY
	File: init_audio.cpp
	Desc: init, load, exit audio engine stuff.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../../main.hpp"
#include "../../files.hpp"
#include "../../ui/LoadingScreen.hpp"
#include "sound.hpp"

#ifndef EDITOR
#include "../../ui/MainMenu.hpp"
#endif

bool initSoundEngine()
{
#ifdef USE_FMOD
	printlog("[FMOD]: initializing FMOD...\n");
	fmod_result = FMOD::System_Create(&fmod_system);
	if (FMODErrorCheck())
	{
		printlog("[FMOD]: Failed to create FMOD. DISABLING AUDIO.\n");
		no_sound = true;
		return false;
	}

	if (!no_sound)
	{
		fmod_result = fmod_system->init(fmod_maxchannels, FMOD_INIT_NORMAL | FMOD_INIT_3D_RIGHTHANDED, fmod_extraDriverData);
		if (FMODErrorCheck())
		{
			printlog("[FMOD]: Failed to initialize FMOD. DISABLING AUDIO.\n");
			no_sound = true;
			return false;
		}

		int selected_driver = 0;
		int numDrivers = 0;
		fmod_system->getNumDrivers(&numDrivers);
		for ( int i = 0; i < numDrivers; ++i )
		{
			constexpr int driverNameLen = 64;
			char driverName[driverNameLen] = "";
			FMOD_GUID guid;
			fmod_result = fmod_system->getDriverInfo(i, driverName, driverNameLen, &guid, nullptr, nullptr, nullptr);
			if ( FMODErrorCheck() )
			{
				printlog("[FMOD]: Failed to read audio device index: %d", i);
			}
			else
			{
				printlog("[FMOD]: Audio device found: %d %s | %08x %04x %04x", i, driverName, guid.Data1, guid.Data2, guid.Data3);
			}

#ifndef EDITOR
			uint32_t _1; memcpy(&_1, &guid.Data1, sizeof(_1));
			uint64_t _2; memcpy(&_2, &guid.Data4, sizeof(_2));
			char guid_string[25];
			snprintf(guid_string, sizeof(guid_string), FMOD_AUDIO_GUID_FMT, _1, _2);
			if (!selected_driver && MainMenu::current_audio_device == guid_string)
			{
				selected_driver = i;
			}
#endif
		}

		fmod_system->setDriver(selected_driver);
		fmod_system->getDriver(&selected_driver);
		printlog("[FMOD]: Current audio device: %d", selected_driver);

		fmod_result = fmod_system->createChannelGroup(nullptr, &sound_group);
		if (FMODErrorCheck())
		{
			printlog("[FMOD]: Failed to create sound channel group. DISABLING AUDIO.\n");
			no_sound = true;
			return false;
		}
		fmod_result = fmod_system->createChannelGroup(nullptr, &soundAmbient_group);
		if ( FMODErrorCheck() )
		{
			printlog("F[FMOD]: ailed to create sound ambient channel group.\n");
			no_sound = true;
		}
		fmod_result = fmod_system->createChannelGroup(nullptr, &soundEnvironment_group);
		if ( FMODErrorCheck() )
		{
			printlog("[FMOD]: Failed to create sound environment channel group.\n");
			no_sound = true;
		}
		fmod_result = fmod_system->createChannelGroup(nullptr, &music_notification_group);
		if ( FMODErrorCheck() )
		{
			printlog("[FMOD]: Failed to create notification channel group.\n");
			no_sound = true;
		}
		fmod_result = fmod_system->createChannelGroup(nullptr, &soundNotification_group);
		if ( FMODErrorCheck() )
		{
			printlog("[FMOD]: Failed to create sound notification channel group.\n");
			no_sound = true;
		}
		fmod_result = fmod_system->createChannelGroup(nullptr, &music_group);
		if (FMODErrorCheck())
		{
			printlog("[FMOD]: Failed to create music channel group. DISABLING AUDIO.\n");
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

#ifndef EDITOR
	// saves your ears getting blasted if the game starts without window focus.
	setGlobalVolume(0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
#endif

	return !no_sound; //No double negatives pls
}

int loadSoundResources(real_t base_load_percent, real_t top_load_percent)
{
	File* fp;
	Uint32 c;
	char name[128];

	if ( !PHYSFS_getRealDir("sound/sounds.txt") )
	{
		printlog("error: could not find file: %s", "sound/sounds.txt");
		return 10;
	}

	// load sound effects
	std::string soundsDirectory = PHYSFS_getRealDir("sound/sounds.txt");
	soundsDirectory.append(PHYSFS_getDirSeparator()).append("sound/sounds.txt");
	printlog("loading sounds...\n");
	fp = openDataFile(soundsDirectory.c_str(), "rb");
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
#ifdef USE_FMOD
	sounds = (FMOD::Sound**) malloc(sizeof(FMOD::Sound*)*numsounds);
	fp = openDataFile(soundsDirectory.c_str(), "rb");
	char full_path[PATH_MAX];
	for ( c = 0; !fp->eof(); ++c )
	{
		fp->gets2(name, 128);
		completePath(full_path, name);
		fmod_result = fmod_system->createSound(full_path, (FMOD_DEFAULT | FMOD_3D), nullptr, &sounds[c]);
		if (FMODErrorCheck())
		{
			printlog("warning: failed to load '%s' listed at line %d in sounds.txt\n", full_path, c + 1);
		}
		updateLoadingScreen(base_load_percent + (top_load_percent * c) / numsounds);
	}
	FileIO::close(fp);
	fmod_system->set3DSettings(1.0, 2.0, 1.0);
#elif defined USE_OPENAL
	sounds = (OPENAL_BUFFER**) malloc(sizeof(OPENAL_BUFFER*)*numsounds);
	for (c = 0, fp = openDataFile(soundsDirectory.c_str(), "rb"); fp->gets2(name, 128); ++c)
	{
		//TODO: Might need to malloc the sounds[c]->sound
		OPENAL_CreateSound(name, true, &sounds[c]);
		//TODO: set sound volume? Or otherwise handle sound volume.
		updateLoadingScreen(base_load_percent + (top_load_percent * c) / numsounds);
	}
	FileIO::close(fp);
	//FMOD_System_Set3DSettings(fmod_system, 1.0, 2.0, 1.0); // This on is hardcoded, I've been lazy here'
#endif // defined USE_OPENAL

	return 0;
}

void freeSoundResources()
{
	uint32_t c;
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
// no idea why this causes the game to hang for me.
// someone else investigate? -skrathbun
#ifndef LINUX
		fmod_system->close();
		fmod_system->release();
#endif
		fmod_system = nullptr;
	}
#endif
}
