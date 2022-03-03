/*-------------------------------------------------------------------------------

	BARONY
	File: game.cpp
	Desc: contains main game code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "draw.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "messages.hpp"
#include "entity.hpp"
#include "files.hpp"
#include "menu.hpp"
#include "classdescriptions.hpp"
#include "interface/interface.hpp"
#include "magic/magic.hpp"
#include "engine/audio/sound.hpp"
#include "items.hpp"
#include "init.hpp"
#include "shops.hpp"
#include "monster.hpp"
#include "scores.hpp"
#include "menu.hpp"
#include "net.hpp"
#ifdef STEAMWORKS
#include <steam/steam_api.h>
#include "steam.hpp"
#endif
#include "prng.hpp"
#include "collision.hpp"
#include "paths.hpp"
#include "player.hpp"
#include "mod_tools.hpp"
#include "lobbies.hpp"
#include "interface/ui.hpp"
#include "ui/GameUI.hpp"
#include "ui/MainMenu.hpp"
#include <limits>
#include "ui/Frame.hpp"
#include "ui/Field.hpp"
#include "input.hpp"
#include "ui/Image.hpp"
#include "ui/MainMenu.hpp"

#include "UnicodeDecoder.h"

#ifdef LINUX
//Sigsegv catching stuff.
#include <signal.h>
#include <string.h>
#include <execinfo.h>
#include <sys/stat.h>

static SDL_bool SDL_MouseModeBeforeSignal = SDL_FALSE;
static int SDL_MouseShowBeforeSignal = SDL_ENABLE;

static void stop_sigaction(int signal, siginfo_t* si, void* arg)
{
    SDL_MouseModeBeforeSignal = SDL_GetRelativeMouseMode();
    SDL_MouseShowBeforeSignal = SDL_ShowCursor(SDL_QUERY);
	SDL_SetRelativeMouseMode(SDL_FALSE);
	SDL_ShowCursor(SDL_ENABLE);
}

static void continue_sigaction(int signal, siginfo_t* si, void* arg)
{
	SDL_SetRelativeMouseMode(SDL_MouseModeBeforeSignal);
	SDL_ShowCursor(SDL_MouseShowBeforeSignal);
}

const unsigned STACK_SIZE = 10;

static void segfault_sigaction(int signal, siginfo_t* si, void* arg)
{
	printf("Caught segfault at address %p\n", si->si_addr);

	printlog("Caught segfault at address %p\n", si->si_addr);

	//Dump the stack.
	void* array[STACK_SIZE];
	size_t size;

	size = backtrace(array, STACK_SIZE);

	printlog("Signal %d (dumping stack):\n", signal);
	backtrace_symbols_fd(array, size, STDERR_FILENO);

	SDL_SetRelativeMouseMode(SDL_FALSE); //Uncapture mouse.

	exit(0);
}

#endif

#ifdef APPLE

#include <sys/stat.h>

#endif

#ifdef BSD

#include <sys/types.h>
#include <sys/sysctl.h>

#endif

#ifdef HAIKU

#include <FindDirectory.h>

#endif

#ifdef WINDOWS
void make_minidump(EXCEPTION_POINTERS* e)
{
	auto hDbgHelp = LoadLibraryA("dbghelp");
	if ( hDbgHelp == nullptr )
	{
		return;
	}
	auto pMiniDumpWriteDump = (decltype(&MiniDumpWriteDump))GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
	if ( pMiniDumpWriteDump == nullptr )
	{
		return;
	}

	char name[PATH_MAX];
	{
		strcpy(name, "barony_crash");
		auto nameEnd = name + strlen("barony_crash");
		SYSTEMTIME t;
		GetLocalTime(&t);
		wsprintfA(nameEnd,
			"_%4d%02d%02d_%02d%02d%02d.dmp",
			t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
	}
	std::string crashlogDir = outputdir;
	crashlogDir.append(PHYSFS_getDirSeparator()).append("crashlogs");
	if ( access(crashlogDir.c_str(), F_OK) == -1 )
	{
		// crashlog folder does not exist to write to.
		printlog("Error accessing crashlogs folder, cannot create crash dump file.");
		return;
	}

	// make a new crash folder.
	char newCrashlogFolder[PATH_MAX] = "";
	strncpy(newCrashlogFolder, name, strlen(name) - strlen(".dmp")); // folder name is the dmp file without .dmp
	if ( PHYSFS_setWriteDir(crashlogDir.c_str()) ) // write to the crashlogs/ directory
	{
		if ( PHYSFS_mkdir(newCrashlogFolder) ) // make the folder to hold the .dmp file
		{
			// the full path of the .dmp file to create.
			crashlogDir.append(PHYSFS_getDirSeparator()).append(newCrashlogFolder).append(PHYSFS_getDirSeparator());
		}
		else
		{
			printlog("[PhysFS]: unsuccessfully created %s folder. Error code: %d", newCrashlogFolder, PHYSFS_getLastErrorCode());
			return;
		}
	}
	else
	{
		printlog("[PhysFS]: unsuccessfully mounted base %s folder. Error code: %d", outputdir, PHYSFS_getLastErrorCode());
		return;
	}

	std::string crashDumpFile = crashlogDir + name;

	auto hFile = CreateFileA(crashDumpFile.c_str(), GENERIC_ALL, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if ( hFile == INVALID_HANDLE_VALUE )
	{
		printlog("Error in file handle for %s, cannot create crash dump file.", crashDumpFile.c_str());
		return;
	}

	MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
	exceptionInfo.ThreadId = GetCurrentThreadId();
	exceptionInfo.ExceptionPointers = e;
	exceptionInfo.ClientPointers = FALSE;

	auto dumped = pMiniDumpWriteDump(
		GetCurrentProcess(),
		GetCurrentProcessId(),
		hFile,
		MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory),
		e ? &exceptionInfo : nullptr,
		nullptr,
		nullptr);

	CloseHandle(hFile);

	printlog("CRITICAL ERROR: Barony has encountered a crash. Submit the crashlog folder in a .zip to the developers: %s", crashlogDir.c_str());
	if ( logfile )
	{
		fclose(logfile);
	}

	// now copy the logfile into the crash folder.
	char logfilePath[PATH_MAX];
	completePath(logfilePath, "log.txt", outputdir);
	std::string crashLogFile = crashlogDir + "log.txt";
	CopyFileA(logfilePath, crashLogFile.c_str(), false);
	return;
}

LONG CALLBACK unhandled_handler(EXCEPTION_POINTERS* e)
{
	make_minidump(e);
	return EXCEPTION_CONTINUE_SEARCH;
}
#endif

std::vector<std::string> randomPlayerNamesMale;
std::vector<std::string> randomPlayerNamesFemale;
std::vector<std::string> physFSFilesInDirectory;
TileEntityListHandler TileEntityList;
// recommended for valgrind debugging:
// res of 480x270
// /nohud
// undefine SOUND, MUSIC (see sound.h)
int game = 1;
Uint32 uniqueGameKey = 0;
DebugStatsClass DebugStats;
Uint32 networkTickrate = 0;
bool gameloopFreezeEntities = false;
Uint32 serverSchedulePlayerHealthUpdate = 0;
Uint32 serverLastPlayerHealthUpdate = 0;
Frame* cursorFrame = nullptr;
bool arachnophobia_filter = false;

Uint32 messagesEnabled = 0xffffffff; // all enabled

TimerExperiments::time_point TimerExperiments::timepoint{};
TimerExperiments::time_point TimerExperiments::currentTime = Clock::now();
TimerExperiments::duration TimerExperiments::accumulator = std::chrono::milliseconds{ 0 };
std::chrono::duration<long long, std::ratio<1, 60>> TimerExperiments::dt = std::chrono::duration<long long, std::ratio<1, 60>>{ 1 };
TimerExperiments::EntityStates TimerExperiments::cameraPreviousState[MAXPLAYERS];
TimerExperiments::EntityStates TimerExperiments::cameraCurrentState[MAXPLAYERS];
TimerExperiments::EntityStates TimerExperiments::cameraRenderState[MAXPLAYERS];
bool TimerExperiments::bUseTimerInterpolation = true;
bool TimerExperiments::bIsInit = false;
bool TimerExperiments::bDebug = false;
real_t TimerExperiments::lerpFactor = 30.0;
void TimerExperiments::integrate(TimerExperiments::State& state,
	std::chrono::time_point<Clock, std::chrono::duration<double>>,
	std::chrono::duration<double> dt)
{
	//state.velocity += state.acceleration * dt / std::chrono::seconds{ 1 };
	state.position += state.velocity * dt / std::chrono::seconds{ 1 };
};

void TimerExperiments::updateEntityInterpolationPosition(Entity* entity)
{
	if ( !TimerExperiments::bUseTimerInterpolation ) { return; }
	if ( !entity ) { return; }

	entity->bUseRenderInterpolation = true;
	if ( entity->behavior == &actHudWeapon
		|| entity->behavior == &actHudShield
		|| entity->behavior == &actHudArm
		|| entity->behavior == &actHudAdditional
		|| entity->behavior == &actHudArrowModel
		|| entity->behavior == &actLeftHandMagic
		|| entity->behavior == &actRightHandMagic
		|| entity->behavior == &actDoor )
	{
		entity->bUseRenderInterpolation = false;
	}
	if ( !entity->bUseRenderInterpolation )
	{
		return;
	}

	if ( entity->bNeedsRenderPositionInit )
	{
		entity->bNeedsRenderPositionInit = false;

		entity->lerpCurrentState.x.position = entity->x / 16.0;
		entity->lerpCurrentState.y.position = entity->y / 16.0;
		entity->lerpCurrentState.z.position = entity->z;
		entity->lerpCurrentState.pitch.position = entity->pitch;
		entity->lerpCurrentState.yaw.position = entity->yaw;
		entity->lerpCurrentState.roll.position = entity->roll;
		entity->lerpCurrentState.resetMovement();
		entity->lerpPreviousState = entity->lerpCurrentState;
		entity->lerpRenderState = entity->lerpCurrentState;
		return;
	}

	entity->lerpCurrentState.x.velocity = TimerExperiments::lerpFactor * (entity->x / 16.0 - entity->lerpCurrentState.x.position);
	entity->lerpCurrentState.y.velocity = TimerExperiments::lerpFactor * (entity->y / 16.0 - entity->lerpCurrentState.y.position);
	entity->lerpCurrentState.z.velocity = TimerExperiments::lerpFactor * (entity->z - entity->lerpCurrentState.z.position);
	real_t diff = entity->yaw - entity->lerpCurrentState.yaw.position;
	if ( diff >= PI )
	{
		diff -= 2 * PI;
	}
	else if ( diff < -PI )
	{
		diff += 2 * PI;
	}
	entity->lerpCurrentState.yaw.velocity = TimerExperiments::lerpFactor * (diff);

	diff = entity->pitch - entity->lerpCurrentState.pitch.position;
	if ( diff >= PI )
	{
		diff -= 2 * PI;
	}
	else if ( diff < -PI )
	{
		diff += 2 * PI;
	}
	entity->lerpCurrentState.pitch.velocity = TimerExperiments::lerpFactor * (diff);

	diff = entity->roll - entity->lerpCurrentState.roll.position;
	if ( diff >= PI )
	{
		diff -= 2 * PI;
	}
	else if ( diff < -PI )
	{
		diff += 2 * PI;
	}
	entity->lerpCurrentState.roll.velocity = TimerExperiments::lerpFactor * (diff);
}

void TimerExperiments::renderCameras(view_t& camera, int player)
{
	if ( !bUseTimerInterpolation )
	{
		return;
	}

	if ( players[player]->entity )
	{
		if ( !(players[player]->entity->skill[3] == 1) ) // skill[3] is debug cam
		{
			if ( bDebug )
			{
				printTextFormatted(font8x8_bmp, 8, 32, "Timer debug is ON");
				real_t diff = camera.ang - players[player]->entity->lerpRenderState.yaw.position;
				while ( diff >= PI )
				{
					diff -= 2 * PI;
				}
				while ( diff < -PI )
				{
					diff += 2 * PI;
				}
				real_t curStateYaw = players[player]->entity->lerpCurrentState.yaw.position;
				real_t prevStateYaw = players[player]->entity->lerpPreviousState.yaw.position;
				if ( abs(diff) > PI / 8 )
				{
					messagePlayer(0, MESSAGE_DEBUG, "new: %.4f old: %.4f | current: %.4f | prev: %.4f",
						players[player]->entity->lerpRenderState.yaw.position, camera.ang, curStateYaw, prevStateYaw);
				}
				printTextFormatted(font8x8_bmp, 8, 20, "new: %.4f old: %.4f | current: %.4f | prev: %.4f",
					players[player]->entity->lerpRenderState.yaw.position, camera.ang, curStateYaw, prevStateYaw);
			}
			if ( bDebug && keystatus[SDL_SCANCODE_I] )
			{
				camera.x = players[player]->entity->x / 16.0;
				camera.y = players[player]->entity->y / 16.0;
				camera.ang = players[player]->entity->yaw;
				camera.vang = players[player]->entity->pitch;

				camera.z = TimerExperiments::cameraRenderState[player].z.position; // this uses PLAYER_CAMERAZ_ACCEL, not entity Z
			}
			else
			{
				camera.x = players[player]->entity->lerpRenderState.x.position;
				camera.y = players[player]->entity->lerpRenderState.y.position;
				camera.ang = players[player]->entity->lerpRenderState.yaw.position;
				camera.vang = players[player]->entity->lerpRenderState.pitch.position;

				camera.z = TimerExperiments::cameraRenderState[player].z.position; // this uses PLAYER_CAMERAZ_ACCEL, not entity Z
			}
		}
	}
	else
	{
		camera.x = TimerExperiments::cameraRenderState[player].x.position;
		camera.y = TimerExperiments::cameraRenderState[player].y.position;
		camera.ang = TimerExperiments::cameraRenderState[player].yaw.position;
		camera.vang = TimerExperiments::cameraRenderState[player].pitch.position;
		camera.z = TimerExperiments::cameraRenderState[player].z.position; // this uses PLAYER_CAMERAZ_ACCEL, not entity Z
	}
	return;
	//if ( players[player]->entity )
	//{
	//	// store original x/y by game logic
	//	playerBodypartOffsets[player].entity_ox = players[player]->entity->x;
	//	playerBodypartOffsets[player].entity_oy = players[player]->entity->y;

	//	players[player]->entity->lerp_ox = players[player]->entity->x;
	//	players[player]->entity->lerp_oy = players[player]->entity->y;

	//	// set to interpolated position
	//	players[player]->entity->x = TimerExperiments::cameraRenderState[player].x.position * 16.0;
	//	players[player]->entity->y = TimerExperiments::cameraRenderState[player].y.position * 16.0;

	//	// adjust bodyparts for this interpolation too, ignore static HUD elements
	//	playerBodypartOffsets[player].limb_newx = players[player]->entity->x - playerBodypartOffsets[player].entity_ox;
	//	playerBodypartOffsets[player].limb_newy = players[player]->entity->y - playerBodypartOffsets[player].entity_oy;
	//	for ( Entity *bodypart : players[player]->entity->bodyparts )
	//	{
	//		if ( players[player]->isLocalPlayer() )
	//		{
	//			if ( bodypart->behavior == &actHudAdditional
	//				|| bodypart->behavior == &actHudWeapon
	//				|| bodypart->behavior == &actHudArm
	//				|| bodypart->behavior == &actHudArrowModel
	//				|| bodypart->behavior == &actHudShield
	//				|| bodypart->behavior == &actLeftHandMagic
	//				|| bodypart->behavior == &actRightHandMagic )
	//			{
	//				continue;
	//			}
	//		}
	//		bodypart->x += playerBodypartOffsets[player].limb_newx;
	//		bodypart->y += playerBodypartOffsets[player].limb_newy;
	//	}
	//}
}

void TimerExperiments::postRenderRestore(view_t& camera, int player)
{
	// unused for now?
	if ( !players[player]->entity || !bUseTimerInterpolation )
	{
		reset();
		return;
	}

	players[player]->entity->x = players[player]->entity->lerp_ox;
	players[player]->entity->y = players[player]->entity->lerp_oy;
	return;
	/*players[player]->entity->x = playerBodypartOffsets[player].entity_ox;
	players[player]->entity->y = playerBodypartOffsets[player].entity_oy;
	for ( Entity *bodypart : players[player]->entity->bodyparts )
	{
		if ( players[player]->isLocalPlayer() )
		{
			if ( bodypart->behavior == &actHudAdditional
				|| bodypart->behavior == &actHudWeapon
				|| bodypart->behavior == &actHudArm
				|| bodypart->behavior == &actHudArrowModel
				|| bodypart->behavior == &actHudShield
				|| bodypart->behavior == &actLeftHandMagic
				|| bodypart->behavior == &actRightHandMagic )
			{
				continue;
			}
		}
		bodypart->x -= playerBodypartOffsets[player].limb_newx;
		bodypart->y -= playerBodypartOffsets[player].limb_newy;
	}*/
}

void TimerExperiments::State::resetMovement()
{
	velocity = 0.0;
	acceleration = 0.0;
}

void TimerExperiments::State::resetPosition()
{
	position = 0.0;
}

void TimerExperiments::State::normalize(real_t min, real_t max)
{
	while ( position >= max )
	{
		position -= 2 * PI;
	}
	while ( position < min )
	{
		position += 2 * PI;
	}
}

void TimerExperiments::EntityStates::resetMovement()
{
	x.resetMovement();
	y.resetMovement();
	z.resetMovement();
	pitch.resetMovement();
	yaw.resetMovement();
	roll.resetMovement();
}

void TimerExperiments::EntityStates::resetPosition()
{
	x.resetPosition();
	y.resetPosition();
	z.resetPosition();
	pitch.resetPosition();
	yaw.resetPosition();
	roll.resetPosition();
}

void TimerExperiments::reset()
{

}

void TimerExperiments::updateClocks()
{
	if ( !bUseTimerInterpolation )
	{
		bIsInit = false;
		return;
	}

	if ( !bIsInit )
	{
		reset();
		bIsInit = true;
	}

	time_point newTime = Clock::now();
	auto frameTime = newTime - currentTime;
	if ( frameTime > std::chrono::milliseconds{ 250 } )
		frameTime = std::chrono::milliseconds{ 250 };
	currentTime = newTime;
	accumulator += frameTime;

	std::vector<Entity*> entitiesToInterpolate;
	for ( node_t* node = map.entities->first; node != nullptr; node = node->next )
	{
		Entity* entity = (Entity*)node->element;
		if ( entity->bUseRenderInterpolation )
		{
			entitiesToInterpolate.push_back(entity);
		}
	}

	while ( accumulator >= dt )
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			cameraPreviousState[i] = cameraCurrentState[i];
			integrate(cameraCurrentState[i].x, timepoint, dt);
			integrate(cameraCurrentState[i].y, timepoint, dt);
			integrate(cameraCurrentState[i].z, timepoint, dt);
			integrate(cameraCurrentState[i].yaw, timepoint, dt);
			integrate(cameraCurrentState[i].pitch, timepoint, dt);
			integrate(cameraCurrentState[i].roll, timepoint, dt);
		}
		for ( auto& entity : entitiesToInterpolate )
		{
			entity->lerpPreviousState = entity->lerpCurrentState;
			integrate(entity->lerpCurrentState.x, timepoint, dt);
			integrate(entity->lerpCurrentState.y, timepoint, dt);
			integrate(entity->lerpCurrentState.z, timepoint, dt);
			integrate(entity->lerpCurrentState.yaw, timepoint, dt);
			integrate(entity->lerpCurrentState.pitch, timepoint, dt);
			integrate(entity->lerpCurrentState.roll, timepoint, dt);
		}
		timepoint += dt;
		accumulator -= dt;
	}

	double alpha = std::chrono::duration<double>{ accumulator } / dt;
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		cameraRenderState[i] = cameraCurrentState[i] * alpha + cameraPreviousState[i] * (1 - alpha);
		// make sure these are limited to prevent large jumps
		cameraCurrentState[i].yaw.normalize(0, 2 * PI);
		cameraCurrentState[i].roll.normalize(0, 2 * PI);
		cameraCurrentState[i].pitch.normalize(0, 2 * PI);

		cameraRenderState[i].yaw.position = 
			lerpAngle(cameraPreviousState[i].yaw.position, cameraCurrentState[i].yaw.position, alpha);
		cameraRenderState[i].roll.position = 
			lerpAngle(cameraPreviousState[i].roll.position, cameraCurrentState[i].roll.position, alpha);
		cameraRenderState[i].pitch.position = 
			lerpAngle(cameraPreviousState[i].pitch.position, cameraCurrentState[i].pitch.position, alpha);

		cameraRenderState[i].yaw.normalize(0, 2 * PI);
		cameraRenderState[i].roll.normalize(0, 2 * PI);
		cameraRenderState[i].pitch.normalize(0, 2 * PI);

		// original angle lerp code
		//real_t a1 = cameraPreviousState[i].yaw.position;
		//real_t a2 = cameraCurrentState[i].yaw.position;
		//real_t adiff = a2 - a1;
		//cameraRenderState[i].yaw.position = a1 + alpha * (fmod(3 * PI + fmod(adiff, 2 * PI), 2 * PI) - PI);
	}
	for ( auto& entity : entitiesToInterpolate )
	{
		entity->lerpRenderState = entity->lerpCurrentState * alpha + entity->lerpPreviousState * (1 - alpha);
		// make sure these are limited to prevent large jumps
		entity->lerpCurrentState.yaw.normalize(0, 2 * PI);
		entity->lerpCurrentState.roll.normalize(0, 2 * PI);
		entity->lerpCurrentState.pitch.normalize(0, 2 * PI);

		entity->lerpRenderState.yaw.position = 
			lerpAngle(entity->lerpPreviousState.yaw.position, entity->lerpCurrentState.yaw.position, alpha);
		entity->lerpRenderState.roll.position = 
			lerpAngle(entity->lerpPreviousState.roll.position, entity->lerpCurrentState.roll.position, alpha);
		entity->lerpRenderState.pitch.position = 
			lerpAngle(entity->lerpPreviousState.pitch.position, entity->lerpCurrentState.pitch.position, alpha);

		entity->lerpRenderState.yaw.normalize(0, 2 * PI);
		entity->lerpRenderState.roll.normalize(0, 2 * PI);
		entity->lerpRenderState.pitch.normalize(0, 2 * PI);
	}
}

real_t TimerExperiments::lerpAngle(real_t angle1, real_t angle2, real_t alpha)
{
	real_t adiff = angle2 - angle1;
	return angle1 + alpha * (fmod(3 * PI + fmod(adiff, 2 * PI), 2 * PI) - PI);
}

std::string TimerExperiments::render(State state)
{
	using namespace std::chrono;
	static auto t = time_point_cast<seconds>(Clock::now());
	static int frame_count = 0;
	static int frame_rate = 0;
	auto pt = t;
	t = time_point_cast<seconds>(Clock::now());
	++frame_count;
	if ( t != pt )
	{
		frame_rate = frame_count;
		frame_count = 0;
	}

	char output[256] = "";
	snprintf(output, sizeof(output), "Frame rate is %d frames per second. Position = %.4f", frame_rate, state.position);
	/*if ( abs(state.velocity) > 0.01 )
	{
		printlog("FPS: %d | Velocity: %.4f | Position: %.4f", frame_rate, state.velocity, state.position);
	}*/
	return output;
}

/*-------------------------------------------------------------------------------

	gameLogic

	Updates the gamestate; moves actors, primarily

-------------------------------------------------------------------------------*/

void gameLogic(void)
{
	Uint32 x;
	node_t* node, *nextnode, *node2;
	Entity* entity;
	int c = 0;
	Uint32 i = 0, j;
	deleteent_t* deleteent;
	bool entitydeletedself;

	int auto_appraise_lowest_time[MAXPLAYERS];
	Item* auto_appraise_target[MAXPLAYERS];
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		auto_appraise_target[i] = nullptr;
		auto_appraise_lowest_time[i] = std::numeric_limits<int>::max();
	}

	if ( creditstage > 0 )
	{
		credittime++;
	}
	if ( intromoviestage > 0 )
	{
		intromovietime++;
	}
	if ( firstendmoviestage > 0 )
	{
		firstendmovietime++;
	}
	if ( secondendmoviestage > 0 )
	{
		secondendmovietime++;
	}
	if ( thirdendmoviestage > 0 )
	{
		thirdendmovietime++;
	}
	if ( fourthendmoviestage > 0 )
	{
		fourthendmovietime++;
	}
	for ( int i = 0; i < 8; ++i )
	{
		if ( DLCendmovieStageAndTime[i][MOVIE_STAGE] > 0 )
		{
			DLCendmovieStageAndTime[i][MOVIE_TIME]++;
		}
	}

	DebugStats.eventsT1 = std::chrono::high_resolution_clock::now();

#ifdef SOUND
	// sound_update(); //Update FMOD and whatnot.
#endif

	// camera shaking
	for (int c = 0; c < MAXPLAYERS; ++c) 
	{
		if ( !splitscreen && c != clientnum )
		{
			continue;
		}

		auto& camera_shakex = cameravars[c].shakex;
		auto& camera_shakey = cameravars[c].shakey;
		auto& camera_shakex2 = cameravars[c].shakex2;
		auto& camera_shakey2 = cameravars[c].shakey2;
		if ( shaking )
		{
			camera_shakex2 = (camera_shakex2 + camera_shakex) * .8;
			camera_shakey2 = (camera_shakey2 + camera_shakey) * .9;
			if ( camera_shakex2 > 0 )
			{
				if ( camera_shakex2 < .02 && camera_shakex >= -.01 )
				{
					camera_shakex2 = 0;
					camera_shakex = 0;
				}
				else
				{
					camera_shakex -= .01;
				}
			}
			else if ( camera_shakex2 < 0 )
			{
				if ( camera_shakex2 > -.02 && camera_shakex <= .01 )
				{
					camera_shakex2 = 0;
					camera_shakex = 0;
				}
				else
				{
					camera_shakex += .01;
				}
			}
			if ( camera_shakey2 > 0 )
			{
				camera_shakey -= 1;
			}
			else if ( camera_shakey2 < 0 )
			{
				camera_shakey += 1;
			}
		}
		else
		{
			camera_shakex = 0;
			camera_shakey = 0;
			camera_shakex2 = 0;
			camera_shakey2 = 0;
		}
	}

	// drunkenness
	if ( !intro )
	{
		if ( stats[clientnum]->EFFECTS[EFF_DRUNK] )
		{
			// goat/drunkards no spin!
			if ( stats[clientnum]->type == GOATMAN )
			{
				// return to normal.
				if ( drunkextend > 0 )
				{
					drunkextend -= .005;
					if ( drunkextend < 0 )
					{
						drunkextend = 0;
					}
				}
			}
			else
			{
				if ( drunkextend < 0.5 )
				{
					drunkextend += .005;
					if ( drunkextend > 0.5 )
					{
						drunkextend = 0.5;
					}
				}
			}
		}
		else
		{
			if ( stats[clientnum]->EFFECTS[EFF_WITHDRAWAL] || stats[clientnum]->EFFECTS[EFF_DISORIENTED] )
			{
				// special widthdrawal shakes
				if ( drunkextend < 0.2 )
				{
					drunkextend += .005;
					if ( drunkextend > 0.2 )
					{
						drunkextend = 0.2;
					}
				}
			}
			else
			{
				// return to normal.
				if ( drunkextend > 0 )
				{
					drunkextend -= .005;
					if ( drunkextend < 0 )
					{
						drunkextend = 0;
					}
				}
			}
		}
	}

	// fading in/out
	if ( fadeout == true )
	{
		fadealpha = std::min(fadealpha + 10, 255);
		if ( fadealpha == 255 )
		{
			fadefinished = true;
		}
		if ( multiplayer == SERVER && introstage == 3 )
		{
			// machinegun this message to clients to make sure they get it!
			for ( c = 1; c < MAXPLAYERS; c++ )
			{
				if ( client_disconnected[c] || players[c]->isLocalPlayer() )
				{
					continue;
				}
				strcpy((char*)net_packet->data, "STRT");
				SDLNet_Write32(svFlags, &net_packet->data[4]);
				SDLNet_Write32(uniqueGameKey, &net_packet->data[8]);
				if ( loadingsavegame == 0 )
				{
					net_packet->data[12] = 0;
				}
				else
				{
					net_packet->data[12] = 1;
				}
				net_packet->address.host = net_clients[c - 1].host;
				net_packet->address.port = net_clients[c - 1].port;
				net_packet->len = 13;
				sendPacket(net_sock, -1, net_packet, c - 1);
			}
		}
	}
	else
	{
		fadealpha = std::max(0, fadealpha - 10);
	}

	// handle safe packets
	if ( !(ticks % 4) )
	{
		j = 0;
		for ( node = safePacketsSent.first; node != nullptr; node = nextnode )
		{
			nextnode = node->next;

			packetsend_t* packet = (packetsend_t*)node->element;
			//printlog("Packet resend: %d", packet->hostnum);
			sendPacket(packet->sock, packet->channel, packet->packet, packet->hostnum, true);
			packet->tries++;
			if ( packet->tries >= MAXTRIES )
			{
				list_RemoveNode(node);
			}
			j++;
			if ( j >= MAXDELETES )
			{
				break;
			}
		}
	}

	// spawn flame particles on burning objects
	if ( !gamePaused || (multiplayer && !client_disconnected[0]) )
	{
		for ( node = map.entities->first; node != nullptr; node = node->next )
		{
			entity = (Entity*)node->element;
			if ( entity->flags[BURNING] )
			{
				if ( !entity->flags[BURNABLE] )
				{
					entity->flags[BURNING] = false;
					continue;
				}
	            if ( flickerLights || entity->ticks % TICKS_PER_SECOND == 1 )
	            {
				    j = 1 + rand() % 4;
				    for ( c = 0; c < j; ++c )
				    {
					    Entity* flame = spawnFlame(entity, SPRITE_FLAME);
					    flame->x += rand() % (entity->sizex * 2 + 1) - entity->sizex;
					    flame->y += rand() % (entity->sizey * 2 + 1) - entity->sizey;
					    flame->z += rand() % 5 - 2;
				    }
				}
			}
		}
	}

	// damage indicator timers
	handleDamageIndicatorTicks();

	if ( intro == true )
	{
		// rotate gear
		gearrot += 1;
		if ( gearrot >= 360 )
		{
			gearrot -= 360;
		}
		gearsize -= std::max<double>(2, gearsize / 20);
		if ( gearsize < 70 )
		{
			gearsize = 70;
			logoalpha += 2;
		}

		// animate tiles
		if ( ticks % 10 == 0 && !gamePaused )
		{
			int x, y, z;
			for ( x = 0; x < map.width; x++ )
			{
				for ( y = 0; y < map.height; y++ )
				{
					for ( z = 0; z < MAPLAYERS; z++ )
					{
						if ( animatedtiles[map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height]] )
						{
							map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height]--;
							if ( !animatedtiles[map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height]] )
							{
								int tile = map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height];
								do
								{
									tile++;
								}
								while ( animatedtiles[tile] );
								map.tiles[z + y * MAPLAYERS + x * MAPLAYERS * map.height] = tile - 1;
							}
						}
					}
				}
			}
		}

		// execute entity behaviors
		c = multiplayer;
		x = clientnum;
		multiplayer = SINGLE;
		clientnum = 0;
		for ( node = map.entities->first; node != nullptr; node = nextnode )
		{
			nextnode = node->next;
			entity = (Entity*)node->element;
			if ( entity && !entity->ranbehavior )
			{
				entity->ticks++;
				if ( entity->behavior != nullptr )
				{
					(*entity->behavior)(entity);
					if ( entitiesdeleted.first != nullptr )
					{
						entitydeletedself = false;
						for ( node2 = entitiesdeleted.first; node2 != nullptr; node2 = node2->next )
						{
							if ( entity == (Entity*)node2->element )
							{
								entitydeletedself = true;
								break;
							}
						}
						if ( entitydeletedself == false )
						{
							entity->ranbehavior = true;
						}
						nextnode = map.entities->first;
						list_FreeAll(&entitiesdeleted);
					}
					else
					{
						entity->ranbehavior = true;
						nextnode = node->next;
					}
				}
			}
		}
		for ( node = map.entities->first; node != nullptr; node = node->next )
		{
			entity = (Entity*)node->element;
			entity->ranbehavior = false;
		}
		multiplayer = c;
		clientnum = x;
	}
	else
	{
		if ( multiplayer == SERVER )
		{
			if ( ticks % 4 == 0 )
			{
				// continue informing clients of entities they need to delete
				for ( i = 1; i < MAXPLAYERS; i++ )
				{
					if ( players[i]->isLocalPlayer() )
					{
						continue;
					}
					j = 0;
					for ( node = entitiesToDelete[i].first; node != NULL; node = nextnode )
					{
						nextnode = node->next;

						// send the delete entity command to the client
						strcpy((char*)net_packet->data, "ENTD");
						deleteent = (deleteent_t*)node->element;
						SDLNet_Write32(deleteent->uid, &net_packet->data[4]);
						net_packet->address.host = net_clients[i - 1].host;
						net_packet->address.port = net_clients[i - 1].port;
						net_packet->len = 8;
						sendPacket(net_sock, -1, net_packet, i - 1);

						// quit reminding clients after a certain number of attempts
						deleteent->tries++;
						if ( deleteent->tries >= MAXTRIES )
						{
							list_RemoveNode(node);
						}
						j++;
						if ( j >= MAXDELETES )
						{
							break;
						}
					}
				}
			}
		}
		DebugStats.eventsT2 = std::chrono::high_resolution_clock::now();
		if ( multiplayer != CLIENT )   // server/singleplayer code
		{
			for ( c = 0; c < MAXPLAYERS; c++ )
			{
				if ( assailantTimer[c] > 0 )
				{
					--assailantTimer[c];
					//messagePlayer(0, "music cd: %d", assailantTimer[c]);
				}
				if ( assailant[c] == true && assailantTimer[c] <= 0 )
				{
					assailant[c] = false;
					assailantTimer[c] = 0;
				}
			}

			if ( !directConnect && LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
			{
#ifdef USE_EOS
				if ( multiplayer == SERVER && ticks % TICKS_PER_SECOND == 0 )
				{
					EOS.CurrentLobbyData.updateLobbyDuringGameLoop();
				}
#endif // USE_EOS
			}


			// animate tiles
			if ( !gamePaused )
			{
				int x, y, z;
				for ( x = 0; x < map.width; x++ )
				{
					for ( y = 0; y < map.height; y++ )
					{
						for ( z = 0; z < MAPLAYERS; z++ )
						{
							int index = z + y * MAPLAYERS + x * MAPLAYERS * map.height;
							if ( animatedtiles[map.tiles[index]] )
							{
								if ( ticks % 10 == 0 )
								{
									map.tiles[index]--;
									if ( !animatedtiles[map.tiles[index]] )
									{
										do
										{
											map.tiles[index]++;
										}
										while ( animatedtiles[map.tiles[index]] );
										map.tiles[index]--;
									}
								}
								if ( z == 0 )
								{
									// water and lava noises
									if ( ticks % (TICKS_PER_SECOND * 4) == (y + x * map.height) % (TICKS_PER_SECOND * 4) && rand() % 3 == 0 )
									{
										if ( lavatiles[map.tiles[index]] )
										{
											// bubbling lava
											playSoundPosLocal( x * 16 + 8, y * 16 + 8, 155, 100 );
										}
										else if ( swimmingtiles[map.tiles[index]] )
										{
											// running water
											playSoundPosLocal( x * 16 + 8, y * 16 + 8, 135, 32 );
										}
									}

									// lava bubbles
									if ( lavatiles[map.tiles[index]] && !gameloopFreezeEntities )
									{
										if ( ticks % 40 == (y + x * map.height) % 40 && rand() % 3 == 0 )
										{
											int c, j = 1 + rand() % 2;
											for ( c = 0; c < j; ++c )
											{
												Entity* entity = newEntity(42, 1, map.entities, nullptr); //Gib entity.
												entity->behavior = &actGib;
												entity->x = x * 16 + rand() % 16;
												entity->y = y * 16 + rand() % 16;
												entity->z = 7.5;
												entity->flags[PASSABLE] = true;
												entity->flags[SPRITE] = true;
												entity->flags[NOUPDATE] = true;
												entity->flags[UPDATENEEDED] = false;
												entity->flags[UNCLICKABLE] = true;
												entity->sizex = 2;
												entity->sizey = 2;
												entity->fskill[3] = 0.01;
												double vel = (rand() % 10) / 20.f;
												entity->vel_x = vel * cos(entity->yaw);
												entity->vel_y = vel * sin(entity->yaw);
												entity->vel_z = -.15 - (rand() % 15) / 100.f;
												entity->yaw = (rand() % 360) * PI / 180.0;
												entity->pitch = (rand() % 360) * PI / 180.0;
												entity->roll = (rand() % 360) * PI / 180.0;
												if ( multiplayer != CLIENT )
												{
													--entity_uids;
												}
												entity->setUID(-3);
											}
										}
									}
								}
							}
						}
					}
				}
			}

			// periodic steam achievement check
			if ( ticks % TICKS_PER_SECOND == 0 )
			{
				for ( c = 0; c < MAXPLAYERS; c++ )
				{
					if ( client_disconnected[c] )
					{
						continue;
					}
					int followerCount = list_Size(&stats[c]->FOLLOWERS);
					if ( followerCount >= 3 )
					{
						steamAchievementClient(c, "BARONY_ACH_NATURAL_BORN_LEADER");
					}
					if ( stats[c]->GOLD >= 10000 )
					{
						steamAchievementClient(c, "BARONY_ACH_FILTHY_RICH");
					}
					if ( stats[c]->GOLD >= 100000 )
					{
						steamAchievementClient(c, "BARONY_ACH_GILDED");
					}

					if ( stats[c]->helmet && stats[c]->helmet->type == ARTIFACT_HELM
						&& stats[c]->breastplate && stats[c]->breastplate->type == ARTIFACT_BREASTPIECE
						&& stats[c]->gloves && stats[c]->gloves->type == ARTIFACT_GLOVES
						&& stats[c]->cloak && stats[c]->cloak->type == ARTIFACT_CLOAK
						&& stats[c]->shoes && stats[c]->shoes->type == ARTIFACT_BOOTS )
					{
						steamAchievementClient(c, "BARONY_ACH_GIFTS_ETERNALS");
					}

					if ( stats[c]->type == SKELETON 
						&& stats[c]->weapon && stats[c]->weapon->type == ARTIFACT_AXE
						&& stats[c]->cloak && stats[c]->cloak->type == CLOAK_PROTECTION
						&& !stats[c]->gloves && !stats[c]->helmet && !stats[c]->shoes
						&& !stats[c]->breastplate && !stats[c]->mask && !stats[c]->ring
						&& !stats[c]->amulet && !stats[c]->shield )
					{
						// nothing but an axe and a cloak.
						steamAchievementClient(c, "BARONY_ACH_COMEDIAN");
					}

					if ( stats[c]->EFFECTS[EFF_SHRINE_RED_BUFF]
						&& stats[c]->EFFECTS[EFF_SHRINE_GREEN_BUFF]
						&& stats[c]->EFFECTS[EFF_SHRINE_BLUE_BUFF] )
					{
						steamAchievementClient(c, "BARONY_ACH_WELL_PREPARED");
					}

					if ( achievementStatusRhythmOfTheKnight[c] )
					{
						steamAchievementClient(c, "BARONY_ACH_RHYTHM_OF_THE_KNIGHT");
					}
					if ( achievementStatusThankTheTank[c] )
					{
						steamAchievementClient(c, "BARONY_ACH_THANK_THE_TANK");
					}

					int bodyguards = 0;
					int squadGhouls = 0;
					int badRomance = 0;
					int familyReunion = 0;
					int machineHead = 0;

					if ( followerCount >= 4 )
					{
						achievementObserver.playerAchievements[c].caughtInAMoshTargets.clear();
					}
					for ( node = stats[c]->FOLLOWERS.first; node != nullptr; node = node->next )
					{
						Entity* follower = nullptr;
						if ( (Uint32*)node->element )
						{
							follower = uidToEntity(*((Uint32*)node->element));
						}
						if ( follower )
						{
							Stat* followerStats = follower->getStats();
							if ( followerStats )
							{
								if ( followerStats->type == CRYSTALGOLEM )
								{
									++bodyguards;
								}
								if ( followerStats->type == GHOUL && stats[c]->type == SKELETON )
								{
									++squadGhouls;
								}
								if ( stats[c]->type == SUCCUBUS && (followerStats->type == SUCCUBUS || followerStats->type == INCUBUS) )
								{
									++badRomance;
								}
								if ( stats[c]->type == GOATMAN && followerStats->type == GOATMAN )
								{
									++familyReunion;
								}
								if ( followerStats->type == GYROBOT || followerStats->type == SENTRYBOT || followerStats->type == SPELLBOT )
								{
									machineHead |= (followerStats->type == GYROBOT);
									machineHead |= (followerStats->type == SENTRYBOT) << 1;
									machineHead |= (followerStats->type == SPELLBOT) << 2;

									if ( followerCount >= 4 && !(achievementObserver.playerAchievements[c].caughtInAMosh) )
									{
										if ( follower->monsterTarget != 0 
											&& (follower->monsterState == MONSTER_STATE_ATTACK || follower->monsterState == MONSTER_STATE_HUNT) &&
											(followerStats->type == SENTRYBOT || followerStats->type == SPELLBOT) )
										{
											auto it = achievementObserver.playerAchievements[c].caughtInAMoshTargets.find(follower->monsterTarget);
											if ( it != achievementObserver.playerAchievements[c].caughtInAMoshTargets.end() )
											{
												// key exists.
												achievementObserver.playerAchievements[c].caughtInAMoshTargets[follower->monsterTarget] += 1; // increase value
												if ( achievementObserver.playerAchievements[c].caughtInAMoshTargets[follower->monsterTarget] >= 4 )
												{
													achievementObserver.awardAchievement(c, AchievementObserver::BARONY_ACH_CAUGHT_IN_A_MOSH);
													achievementObserver.playerAchievements[c].caughtInAMosh = true;
												}
											}
											else
											{
												achievementObserver.playerAchievements[c].caughtInAMoshTargets.insert(std::make_pair(static_cast<Uint32>(follower->monsterTarget), 1));
											}
										}
									}
								}
							}
						}
					}
					if ( bodyguards >= 2 )
					{
						steamAchievementClient(c, "BARONY_ACH_BODYGUARDS");
					}
					if ( squadGhouls >= 4 )
					{
						steamAchievementClient(c, "BARONY_ACH_SQUAD_GHOULS");
					}
					if ( badRomance >= 2 )
					{
						steamAchievementClient(c, "BARONY_ACH_BAD_ROMANCE");
					}
					if ( familyReunion >= 3 )
					{
						steamAchievementClient(c, "BARONY_ACH_FAMILY_REUNION");
					}
					if ( machineHead == 7 )
					{
						steamAchievementClient(c, "BARONY_ACH_MACHINE_HEAD");
					}
					if ( achievementObserver.playerAchievements[c].ticksSpentOverclocked >= 250 )
					{
						Uint32 increase = achievementObserver.playerAchievements[c].ticksSpentOverclocked / TICKS_PER_SECOND;
						steamStatisticUpdateClient(c, STEAM_STAT_OVERCLOCKED, STEAM_STAT_INT, increase);

						// add the leftover sub-second ticks result back into the score.
						achievementObserver.playerAchievements[c].ticksSpentOverclocked = increase % TICKS_PER_SECOND;
					}
				}
				updateGameplayStatisticsInMainLoop();
			}

			updatePlayerConductsInMainLoop();

			//if( TICKS_PER_SECOND )
			//generatePathMaps();
			bool debugMonsterTimer = false && !gamePaused;
			if ( debugMonsterTimer )
			{
				printlog("loop start");
			}
			real_t accum = 0.0;
			DebugStats.eventsT3 = std::chrono::high_resolution_clock::now();

			// run world UI entities
			for ( node = map.worldUI->first; node != nullptr; node = nextnode )
			{
				nextnode = node->next;
				entity = (Entity*)node->element;
				if ( entity && !entity->ranbehavior )
				{
					if ( !gamePaused || (multiplayer && !client_disconnected[0]) )
					{
						++entity->ticks;
					}
					if ( entity->behavior != nullptr )
					{
						if ( !gamePaused || (multiplayer && !client_disconnected[0]) )
						{
							(*entity->behavior)(entity);
						}
						if ( entitiesdeleted.first != nullptr )
						{
							entitydeletedself = false;
							for ( node2 = entitiesdeleted.first; node2 != nullptr; node2 = node2->next )
							{
								if ( entity == (Entity*)node2->element )
								{
									//printlog("DEBUG: Entity deleted self, sprite: %d", entity->sprite);
									entitydeletedself = true;
									break;
								}
							}
							if ( entitydeletedself == false )
							{
								entity->ranbehavior = true;
							}
							nextnode = map.worldUI->first;
							list_FreeAll(&entitiesdeleted);
						}
						else
						{
							entity->ranbehavior = true;
							nextnode = node->next;
						}
					}
				}
			}

			for ( node = map.entities->first; node != nullptr; node = nextnode )
			{
				nextnode = node->next;
				entity = (Entity*)node->element;
				if ( entity && !entity->ranbehavior )
				{
					if ( !gamePaused || (multiplayer && !client_disconnected[0]) )
					{
						++entity->ticks;
					}
					if ( entity->behavior != nullptr )
					{
						if ( gameloopFreezeEntities 
							&& entity->behavior != &actPlayer
							&& entity->behavior != &actPlayerLimb
							&& entity->behavior != &actHudWeapon
							&& entity->behavior != &actHudShield
							&& entity->behavior != &actHudAdditional
							&& entity->behavior != &actHudArrowModel
							&& entity->behavior != &actLeftHandMagic
							&& entity->behavior != &actRightHandMagic
							&& entity->behavior != &actFlame )
						{
							TimerExperiments::updateEntityInterpolationPosition(entity);
							continue;
						}
						int ox = -1;
						int oy = -1;
						auto t = std::chrono::high_resolution_clock::now();

						if ( !gamePaused || (multiplayer && !client_disconnected[0]) )
						{
							ox = static_cast<int>(entity->x) >> 4;
							oy = static_cast<int>(entity->y) >> 4;
							if ( !entity->myTileListNode )
							{
								TileEntityList.addEntity(*entity);
							}

							/*if ( entity->getUID() >= 0 && entity->behavior != &actFlame && !entity->flags[INVISIBLE]
								&& entity->behavior != &actDoor && entity->behavior != &actDoorFrame
								&& entity->behavior != &actGate && entity->behavior != &actTorch
								&& entity->behavior != &actSprite && !entity->flags[SPRITE]
								&& entity->skill[28] == 0 )
							{
								printlog("DEBUG: Starting Entity sprite: %d", entity->sprite);
							}*/
							(*entity->behavior)(entity);
						}
						if ( entitiesdeleted.first != nullptr )
						{
							entitydeletedself = false;
							for ( node2 = entitiesdeleted.first; node2 != nullptr; node2 = node2->next )
							{
								if ( entity == (Entity*)node2->element )
								{
									//printlog("DEBUG: Entity deleted self, sprite: %d", entity->sprite);
									entitydeletedself = true;
									break;
								}
							}
							if ( entitydeletedself == false )
							{
								if ( ox != -1 && oy != -1 )
								{
									if ( ox != static_cast<int>(entity->x) >> 4
										|| oy != static_cast<int>(entity->y) >> 4 )
									{
										// if entity moved into a new tile, update it's tile position in global tile list.
										TileEntityList.updateEntity(*entity);
									}
								}
								TimerExperiments::updateEntityInterpolationPosition(entity);

								entity->ranbehavior = true;
							}
							nextnode = map.entities->first;
							list_FreeAll(&entitiesdeleted);
						}
						else
						{
							if ( ox != -1 && oy != -1 )
							{
								if ( ox != static_cast<int>(entity->x) >> 4
									|| oy != static_cast<int>(entity->y) >> 4 )
								{
									// if entity moved into a new tile, update it's tile position in global tile list.
									TileEntityList.updateEntity(*entity);
								}
							}
							TimerExperiments::updateEntityInterpolationPosition(entity);

							entity->ranbehavior = true;
							nextnode = node->next;
							if ( debugMonsterTimer && entity->behavior == &actMonster )
							{
								auto t2 = std::chrono::high_resolution_clock::now();
								printlog("%d: %d %f", entity->sprite, entity->monsterState,
									1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t).count());
								accum += 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t).count();
							}

						}
					}
				}

				if ( loadnextlevel == true )
				{
					for ( node = map.entities->first; node != nullptr; node = node->next )
					{
						entity = (Entity*)node->element;
						entity->flags[NOUPDATE] = true;

						if ( (entity->behavior == &actThrown || entity->behavior == &actParticleSapCenter) && entity->sprite == 977 )
						{
							// boomerang particle, make sure to return on level change.
							Entity* parent = uidToEntity(entity->parent);
							if ( parent && parent->behavior == &actPlayer && stats[parent->skill[2]] )
							{
								Item* item = newItemFromEntity(entity);
								if ( !item )
								{
									continue;
								}
								item->ownerUid = parent->getUID();
								Item* pickedUp = itemPickup(parent->skill[2], item);
								Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								messagePlayerColor(parent->skill[2], MESSAGE_EQUIPMENT, color, language[3746], items[item->type].name_unidentified);
								if ( pickedUp )
								{
									if ( parent->skill[2] == 0 || (parent->skill[2] > 0 && splitscreen) )
									{
										// pickedUp is the new inventory stack for server, free the original items
										free(item);
										item = nullptr;
										if ( multiplayer != CLIENT && !stats[parent->skill[2]]->weapon )
										{
											useItem(pickedUp, parent->skill[2]);
										}

										auto& hotbar_t = players[parent->skill[2]]->hotbar;
										auto& hotbar = hotbar_t.slots();

										if ( hotbar_t.magicBoomerangHotbarSlot >= 0 )
										{
											hotbar[hotbar_t.magicBoomerangHotbarSlot].item = pickedUp->uid;
											for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
											{
												if ( i != hotbar_t.magicBoomerangHotbarSlot && hotbar[i].item == pickedUp->uid )
												{
													hotbar[i].item = 0;
												}
											}
										}
									}
									else
									{
										free(pickedUp); // item is the picked up items (item == pickedUp)
									}
								}
							}
						}
					}

					// hack to fix these things from breaking everything...
					for ( int i = 0; i < MAXPLAYERS; ++i )
					{
						players[i]->hud.arm = nullptr;
						players[i]->hud.weapon = nullptr;
						players[i]->hud.magicLeftHand = nullptr;
						players[i]->hud.magicRightHand = nullptr;
					}

					// stop all sounds
#ifdef USE_FMOD
					if ( sound_group )
					{
						sound_group->stop();
					}
					if ( soundAmbient_group )
					{
						soundAmbient_group->stop();
					}
					if ( soundEnvironment_group )
					{
						soundEnvironment_group->stop();
					}
#elif defined USE_OPENAL
					if ( sound_group )
					{
						OPENAL_ChannelGroup_Stop(sound_group);
					}
					if ( soundAmbient_group )
					{
						OPENAL_ChannelGroup_Stop(soundAmbient_group);
					}
					if ( soundEnvironment_group )
					{
						OPENAL_ChannelGroup_Stop(soundEnvironment_group);
					}
#endif
					// stop combat music
					// close chests
					for ( c = 0; c < MAXPLAYERS; ++c )
					{
						assailantTimer[c] = 0;
						if ( players[c]->isLocalPlayer() )
						{
							if ( openedChest[c] )
							{
								openedChest[c]->closeChest();
							}
						}
						else if ( c > 0 && !client_disconnected[c] )
						{
							if ( openedChest[c] )
							{
								openedChest[c]->closeChestServer();
							}
						}
					}



					// show loading message
					loading = true;
					drawClearBuffers();
					int w, h;
					getSizeOfText(ttf16, language[709], &w, &h);
					ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[709]);

					GO_SwapBuffers(screen);

					// copy followers list
					list_t tempFollowers[MAXPLAYERS];
					bool bCopyFollowers = true;
					if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
					{
						bCopyFollowers = false;
					}

					for ( c = 0; c < MAXPLAYERS; ++c )
					{
						tempFollowers[c].first = nullptr;
						tempFollowers[c].last = nullptr;

						node_t* node;
						for ( node = stats[c]->FOLLOWERS.first; bCopyFollowers && node != nullptr; node = node->next )
						{
							Entity* follower = nullptr;
							if ( (Uint32*)node->element )
							{
								follower = uidToEntity(*((Uint32*)node->element));
							}
							if ( follower )
							{
								Stat* followerStats = follower->getStats();
								if ( followerStats )
								{
									node_t* newNode = list_AddNodeLast(&tempFollowers[c]);
									newNode->element = followerStats->copyStats();
									newNode->deconstructor = &statDeconstructor;
									newNode->size = sizeof(followerStats);
								}
							}
						}

						list_FreeAll(&stats[c]->FOLLOWERS);
					}

					// unlock some steam achievements
					if ( !secretlevel )
					{
						switch ( currentlevel )
						{
							case 0:
								steamAchievement("BARONY_ACH_ENTER_THE_DUNGEON");
								break;
							default:
								break;
						}
					}

					// signal clients about level change
					mapseed = rand();
					lastEntityUIDs = entity_uids;
					if ( forceMapSeed > 0 )
					{
						mapseed = forceMapSeed;
						forceMapSeed = 0;
					}

					bool loadingTheSameFloorAsCurrent = false;
					if ( skipLevelsOnLoad > 0 )
					{
						currentlevel += skipLevelsOnLoad;
					}
					else
					{
						if ( skipLevelsOnLoad < 0 )
						{
							currentlevel += skipLevelsOnLoad;
							if ( skipLevelsOnLoad == -1 )
							{
								loadingTheSameFloorAsCurrent = true;
							}
						}
						++currentlevel;
					}
					skipLevelsOnLoad = 0;

					if ( !secretlevel )
					{
						switch ( currentlevel )
						{
							case 5:
								steamAchievement("BARONY_ACH_TWISTY_PASSAGES");
								// the observer will send out to clients.
								achievementObserver.updatePlayerAchievement(clientnum,
									AchievementObserver::Achievement::BARONY_ACH_COOP_ESCAPE_MINES, AchievementObserver::ACH_EVENT_NONE);
								break;
							case 10:
								steamAchievement("BARONY_ACH_JUNGLE_FEVER");
								break;
							case 15:
								steamAchievement("BARONY_ACH_SANDMAN");
								break;
							case 30:
								steamAchievement("BARONY_ACH_SPELUNKY");
								break;
							case 35:
								if ( ((completionTime / TICKS_PER_SECOND) / 60) <= 45 )
								{
									conductGameChallenges[CONDUCT_BLESSED_BOOTS_SPEED] = 1;
								}
								break;
							default:
								break;
						}
					}

					if ( multiplayer == SERVER )
					{
						for ( c = 1; c < MAXPLAYERS; ++c )
						{
							if ( client_disconnected[c] == true || players[c]->isLocalPlayer() )
							{
								continue;
							}
							if ( loadingSameLevelAsCurrent )
							{
								strcpy((char*)net_packet->data, "LVLR");
							}
							else
							{
								strcpy((char*)net_packet->data, "LVLC");
							}
							net_packet->data[4] = secretlevel;
							SDLNet_Write32(mapseed, &net_packet->data[5]);
							SDLNet_Write32(lastEntityUIDs, &net_packet->data[9]);
							net_packet->data[13] = currentlevel;

							if ( loadCustomNextMap.compare("") != 0 )
							{
								strcpy((char*)(&net_packet->data[14]), loadCustomNextMap.c_str());
								net_packet->data[14 + loadCustomNextMap.length()] = 0;
								net_packet->len = 14 + loadCustomNextMap.length() + 1;
							}
							else
							{
								net_packet->data[14] = 0;
								net_packet->len = 15;
							}
							net_packet->address.host = net_clients[c - 1].host;
							net_packet->address.port = net_clients[c - 1].port;
							sendPacketSafe(net_sock, -1, net_packet, c - 1);
						}
					}
					loadingSameLevelAsCurrent = false;
					darkmap = false;
					numplayers = 0;

					gameplayCustomManager.readFromFile();
					textSourceScript.scriptVariables.clear();

					int checkMapHash = -1;
					int result = physfsLoadMapFile(currentlevel, mapseed, false, &checkMapHash);
					if ( checkMapHash == 0 )
					{
						conductGameChallenges[CONDUCT_MODDED] = 1;
					}

					globalLightModifierActive = GLOBAL_LIGHT_MODIFIER_STOPPED;

					// clear follower menu entities.
					for ( int i = 0; i < MAXPLAYERS; ++i )
					{
						minimapPings[i].clear(); // clear minimap pings
						if ( players[i]->isLocalPlayer() )
						{
							FollowerMenu[i].closeFollowerMenuGUI(true);
						}
						enemyHPDamageBarHandler[i].HPBars.clear();
					}

					assignActions(&map);
					generatePathMaps();

					achievementObserver.updateData();

					if ( !strncmp(map.name, "Mages Guild", 11) )
					{
						for ( c = 0; c < MAXPLAYERS; ++c )
						{
							if ( players[c] && players[c]->entity )
							{
								players[c]->entity->modHP(999);
								players[c]->entity->modMP(999);
								if ( stats[c] && stats[c]->HUNGER < 1450 )
								{
									stats[c]->HUNGER = 1450;
									serverUpdateHunger(c);
								}
							}
						}
						messageLocalPlayers(MESSAGE_STATUS, language[2599]);

						// undo shopkeeper grudge
						swornenemies[SHOPKEEPER][HUMAN] = false;
						monsterally[SHOPKEEPER][HUMAN] = true;
						swornenemies[SHOPKEEPER][AUTOMATON] = false;
						monsterally[SHOPKEEPER][AUTOMATON] = true;
					}

					// (special) unlock temple achievement
					if ( secretlevel && currentlevel == 8 )
					{
						steamAchievement("BARONY_ACH_TRICKS_AND_TRAPS");
					}
					// flag setting we've reached the lich
					if ( !strncmp(map.name, "Boss", 4) || !strncmp(map.name, "Sanctum", 7) )
					{
						for ( int c = 0; c < MAXPLAYERS; ++c )
						{
							steamStatisticUpdateClient(c, STEAM_STAT_BACK_TO_BASICS, STEAM_STAT_INT, 1);
						}
					}

					Player::Minimap_t::mapDetails.clear();

					if ( !secretlevel )
					{
						messageLocalPlayers(MESSAGE_PROGRESSION, language[710], currentlevel);
					}
					else
					{
						messageLocalPlayers(MESSAGE_PROGRESSION, language[711], map.name);
					}
					if ( !secretlevel && result )
					{
						switch ( currentlevel )
						{
							case 2:
								messageLocalPlayers(MESSAGE_HINT, language[712]);
								Player::Minimap_t::mapDetails.push_back(std::make_pair("secret_exit_description", language[712]));
								break;
							case 3:
								messageLocalPlayers(MESSAGE_HINT, language[713]);
								Player::Minimap_t::mapDetails.push_back(std::make_pair("secret_exit_description", language[713]));
								break;
							case 7:
								messageLocalPlayers(MESSAGE_HINT, language[714]);
								Player::Minimap_t::mapDetails.push_back(std::make_pair("secret_exit_description", language[714]));
								break;
							case 8:
								messageLocalPlayers(MESSAGE_HINT, language[715]);
								Player::Minimap_t::mapDetails.push_back(std::make_pair("secret_exit_description", language[715]));
								break;
							case 11:
								messageLocalPlayers(MESSAGE_HINT, language[716]);
								Player::Minimap_t::mapDetails.push_back(std::make_pair("secret_exit_description", language[716]));
								break;
							case 13:
								messageLocalPlayers(MESSAGE_HINT, language[717]);
								Player::Minimap_t::mapDetails.push_back(std::make_pair("secret_exit_description", language[717]));
								break;
							case 16:
								messageLocalPlayers(MESSAGE_HINT, language[718]);
								Player::Minimap_t::mapDetails.push_back(std::make_pair("secret_exit_description", language[718]));
								break;
							case 18:
								messageLocalPlayers(MESSAGE_HINT, language[719]);
								Player::Minimap_t::mapDetails.push_back(std::make_pair("secret_exit_description", language[719]));
								break;
							default:
								break;
						}
					}
					if ( MFLAG_DISABLETELEPORT )
					{
						Player::Minimap_t::mapDetails.push_back(std::make_pair("map_flag_disable_teleport", language[2382]));
					}
					if ( MFLAG_DISABLEOPENING )
					{
						Player::Minimap_t::mapDetails.push_back(std::make_pair("map_flag_disable_opening", language[2382]));
					}
					if ( MFLAG_DISABLETELEPORT || MFLAG_DISABLEOPENING )
					{
						messageLocalPlayers(MESSAGE_HINT, language[2382]);
					}
					if ( MFLAG_DISABLELEVITATION )
					{
						messageLocalPlayers(MESSAGE_HINT, language[2383]);
						Player::Minimap_t::mapDetails.push_back(std::make_pair("map_flag_disable_levitation", language[2383]));
					}
					if ( MFLAG_DISABLEDIGGING )
					{
						messageLocalPlayers(MESSAGE_HINT, language[2450]);
						Player::Minimap_t::mapDetails.push_back(std::make_pair("map_flag_disable_digging", language[2450]));
					}
					if ( MFLAG_DISABLEHUNGER )
					{
						Player::Minimap_t::mapDetails.push_back(std::make_pair("map_flag_disable_hunger", ""));
					}
					loadnextlevel = false;
					loading = false;
					fadeout = false;
					fadealpha = 255;

					for (c = 0; c < MAXPLAYERS; c++)
					{
						if (players[c] && players[c]->entity && !client_disconnected[c])
						{
							if ( stats[c] && stats[c]->EFFECTS[EFF_POLYMORPH] && stats[c]->playerPolymorphStorage != NOTHING )
							{
								players[c]->entity->effectPolymorph = stats[c]->playerPolymorphStorage;
								serverUpdateEntitySkill(players[c]->entity, 50); // update visual polymorph effect for clients.
							}
							if ( stats[c] && stats[c]->EFFECTS[EFF_SHAPESHIFT] && stats[c]->playerShapeshiftStorage != NOTHING )
							{
								players[c]->entity->effectShapeshift = stats[c]->playerShapeshiftStorage;
								serverUpdateEntitySkill(players[c]->entity, 53); // update visual polymorph effect for clients.
							}
							if ( stats[c] && stats[c]->EFFECTS[EFF_VAMPIRICAURA] && stats[c]->EFFECTS_TIMERS[EFF_VAMPIRICAURA] == -2 )
							{
								players[c]->entity->playerVampireCurse = 1;
								serverUpdateEntitySkill(players[c]->entity, 51); // update curse progression
							}

							node_t* node;
							node_t* gyrobotNode = nullptr;
							Entity* gyrobotEntity = nullptr;
							std::vector<node_t*> allyRobotNodes;
							for ( node = tempFollowers[c].first; node != NULL; node = node->next )
							{
								Stat* tempStats = (Stat*)node->element;
								if ( tempStats && tempStats->type == GYROBOT )
								{
									gyrobotNode = node;
									break;
								}
							}
							for (node = tempFollowers[c].first; node != nullptr; node = node->next)
							{
								Stat* tempStats = (Stat*)node->element;
								if ( tempStats && (tempStats->type == DUMMYBOT
									|| tempStats->type == SENTRYBOT
									|| tempStats->type == SPELLBOT) )
								{
									// gyrobot will pick up these guys into it's inventory, otherwise leave them behind.
									if ( gyrobotNode )
									{
										allyRobotNodes.push_back(node);
									}
									continue;
								}
								Entity* monster = summonMonster(tempStats->type, players[c]->entity->x, players[c]->entity->y);
								if (monster)
								{
									if ( node == gyrobotNode )
									{
										gyrobotEntity = monster;
									}
									monster->skill[3] = 1; // to mark this monster partially initialized
									list_RemoveNode(monster->children.last);

									node_t* newNode = list_AddNodeLast(&monster->children);
									newNode->element = tempStats->copyStats();
									newNode->deconstructor = &statDeconstructor;
									newNode->size = sizeof(tempStats);

									Stat* monsterStats = (Stat*)newNode->element;
									monsterStats->leader_uid = players[c]->entity->getUID();
									messagePlayerMonsterEvent(c, 0xFFFFFFFF, *monsterStats, language[721], language[720], MSG_COMBAT);
									monster->flags[USERFLAG2] = true;
									serverUpdateEntityFlag(monster, USERFLAG2);
									/*if (!monsterally[HUMAN][monsterStats->type])
									{
									}*/
									monster->monsterAllyIndex = c;
									if ( multiplayer == SERVER )
									{
										serverUpdateEntitySkill(monster, 42); // update monsterAllyIndex for clients.
									}

									if ( multiplayer != CLIENT )
									{
										monster->monsterAllyClass = monsterStats->allyClass;
										monster->monsterAllyPickupItems = monsterStats->allyItemPickup;
										if ( stats[c]->playerSummonPERCHR != 0 && !strcmp(monsterStats->name, "skeleton knight") )
										{
											monster->monsterAllySummonRank = (stats[c]->playerSummonPERCHR & 0x0000FF00) >> 8;
										}
										else if ( stats[c]->playerSummon2PERCHR != 0 && !strcmp(monsterStats->name, "skeleton sentinel") )
										{
											monster->monsterAllySummonRank = (stats[c]->playerSummon2PERCHR & 0x0000FF00) >> 8;
										}
										serverUpdateEntitySkill(monster, 46); // update monsterAllyClass
										serverUpdateEntitySkill(monster, 44); // update monsterAllyPickupItems
										serverUpdateEntitySkill(monster, 50); // update monsterAllySummonRank
									}

									newNode = list_AddNodeLast(&stats[c]->FOLLOWERS);
									newNode->deconstructor = &defaultDeconstructor;
									Uint32* myuid = (Uint32*) malloc(sizeof(Uint32));
									newNode->element = myuid;
									*myuid = monster->getUID();

									if ( monsterStats->type == HUMAN && currentlevel == 25 && !strncmp(map.name, "Mages Guild", 11) )
									{
										steamAchievementClient(c, "BARONY_ACH_ESCORT");
									}

									if ( c > 0 && multiplayer == SERVER && !players[c]->isLocalPlayer() )
									{
										strcpy((char*)net_packet->data, "LEAD");
										SDLNet_Write32((Uint32)monster->getUID(), &net_packet->data[4]);
										strcpy((char*)(&net_packet->data[8]), monsterStats->name);
										net_packet->data[8 + strlen(monsterStats->name)] = 0;
										net_packet->address.host = net_clients[c - 1].host;
										net_packet->address.port = net_clients[c - 1].port;
										net_packet->len = 8 + strlen(monsterStats->name) + 1;
										sendPacketSafe(net_sock, -1, net_packet, c - 1);

										serverUpdateAllyStat(c, monster->getUID(), monsterStats->LVL, monsterStats->HP, monsterStats->MAXHP, monsterStats->type);
									}

									if ( !FollowerMenu[c].recentEntity && players[c]->isLocalPlayer() )
									{
										FollowerMenu[c].recentEntity = monster;
									}
								}
								else
								{
									messagePlayerMonsterEvent(c, 0xFFFFFFFF, *tempStats, language[723], language[722], MSG_COMBAT);
								}
							}
							if ( gyrobotEntity && !allyRobotNodes.empty() )
							{
								Stat* gyroStats = gyrobotEntity->getStats();
								for ( auto it = allyRobotNodes.begin(); gyroStats && it != allyRobotNodes.end(); ++it )
								{
									node_t* botNode = *it;
									if ( botNode )
									{
										Stat* tempStats = (Stat*)botNode->element;
										if ( tempStats )
										{
											ItemType type = WOODEN_SHIELD;
											if ( tempStats->type == SENTRYBOT )
											{
												type = TOOL_SENTRYBOT;
											}
											else if ( tempStats->type == SPELLBOT )
											{
												type = TOOL_SPELLBOT;
											}
											else if ( tempStats->type == DUMMYBOT )
											{
												type = TOOL_DUMMYBOT;
											}
											int appearance = monsterTinkeringConvertHPToAppearance(tempStats);
											if ( type != WOODEN_SHIELD )
											{
												Item* item = newItem(type, static_cast<Status>(tempStats->monsterTinkeringStatus),
													0, 1, appearance, true, &gyroStats->inventory);
											}
										}
									}
								}
							}
						}
						list_FreeAll(&tempFollowers[c]);
					}

					saveGame();
					break;
				}
			}
			if ( debugMonsterTimer )
			{
				printlog("accum: %f", accum);
			}
			for ( node = map.entities->first; node != nullptr; node = node->next )
			{
				entity = (Entity*)node->element;
				entity->ranbehavior = false;
			}
			for ( node = map.worldUI->first; node != nullptr; node = node->next )
			{
				entity = (Entity*)node->element;
				entity->ranbehavior = false;
			}
			DebugStats.eventsT4 = std::chrono::high_resolution_clock::now();
			if ( multiplayer == SERVER )
			{
				// periodically remind clients of the current level
				if ( ticks % (TICKS_PER_SECOND * 3) == 0 )
				{
					for ( c = 1; c < MAXPLAYERS; c++ )
					{
						if ( client_disconnected[c] == true || players[c]->isLocalPlayer() )
						{
							continue;
						}
						strcpy((char*)net_packet->data, "LVLC");
						net_packet->data[4] = secretlevel;
						SDLNet_Write32(mapseed, &net_packet->data[5]);
						SDLNet_Write32(lastEntityUIDs, &net_packet->data[9]);
						net_packet->data[13] = currentlevel;
						net_packet->address.host = net_clients[c - 1].host;
						net_packet->address.port = net_clients[c - 1].port;
						net_packet->len = 14;
						sendPacketSafe(net_sock, -1, net_packet, c - 1);
					}
				}

				bool updatePlayerHealth = false;
				if ( serverSchedulePlayerHealthUpdate != 0 && (ticks - serverSchedulePlayerHealthUpdate) >= TICKS_PER_SECOND * 0.5 )
				{
					// update if 0.5s have passed since request of health update.
					// the scheduled update needs to be reset to 0 to be set again.
					updatePlayerHealth = true;
				}
				else if ( (ticks % (TICKS_PER_SECOND * 3) == 0) 
					&& (serverLastPlayerHealthUpdate == 0 || ((ticks - serverLastPlayerHealthUpdate) >= 2 * TICKS_PER_SECOND)) 
				)
				{
					// regularly update every 3 seconds, only if the last update was more than 2 seconds ago.
					updatePlayerHealth = true;
				}

				if ( updatePlayerHealth )
				{
					// send update to all clients for global stats[NUMPLAYERS] struct
					serverLastPlayerHealthUpdate = ticks;
					serverSchedulePlayerHealthUpdate = 0;
					serverUpdatePlayerStats();
				}

				// send entity info to clients
				if ( ticks % (TICKS_PER_SECOND / 8) == 0 )
				{
					for ( node = map.entities->first; node != nullptr; node = node->next )
					{
						entity = (Entity*)node->element;
						for ( c = 1; c < MAXPLAYERS; ++c )
						{
							if ( !client_disconnected[c] )
							{
								if ( entity->flags[UPDATENEEDED] == true && entity->flags[NOUPDATE] == false )
								{
									// update entity for all clients
									if ( entity->getUID() % (TICKS_PER_SECOND * 4) == ticks % (TICKS_PER_SECOND * 4) )
									{
										sendEntityUDP(entity, c, true);
									}
									else
									{
										sendEntityUDP(entity, c, false);
									}
								}
							}
						}
					}
				}

				// handle keep alives
				for ( c = 1; c < MAXPLAYERS; c++ )
				{
					if ( client_disconnected[c] || players[c]->isLocalPlayer() )
					{
						continue;
					}
					if ( ticks % (TICKS_PER_SECOND * 1) == 0 )
					{
						// send a keep alive every second
						strcpy((char*)net_packet->data, "KPAL");
						net_packet->data[4] = clientnum;
						net_packet->address.host = net_clients[c - 1].host;
						net_packet->address.port = net_clients[c - 1].port;
						net_packet->len = 5;
						sendPacketSafe(net_sock, -1, net_packet, c - 1);
					}
					if ( losingConnection[c] && ticks - client_keepalive[c] == 1 )
					{
						// regained connection
						losingConnection[c] = false;
						messageLocalPlayers(MESSAGE_MISC, language[724], c, stats[c]->name);
					}
					else if ( !losingConnection[c] && ticks - client_keepalive[c] == TICKS_PER_SECOND * 30 - 1 )
					{
						// 30 second timer
						losingConnection[c] = true;
						messageLocalPlayers(MESSAGE_MISC, language[725], c, stats[c]->name);
					}
					else if ( !client_disconnected[c] && ticks - client_keepalive[c] >= TICKS_PER_SECOND * 45 - 1 )
					{
						// additional 15 seconds (kick time)
						messageLocalPlayers(MESSAGE_MISC, language[726], c, stats[c]->name);
						strcpy((char*)net_packet->data, "KICK");
						net_packet->address.host = net_clients[c - 1].host;
						net_packet->address.port = net_clients[c - 1].port;
						net_packet->len = 4;
						sendPacketSafe(net_sock, -1, net_packet, c - 1);
						client_disconnected[c] = true;
					}
				}
			}

			// update clients on assailant status
			if (multiplayer != SINGLE) {
				for ( c = 1; c < MAXPLAYERS; c++ )
				{
					if ( !client_disconnected[c] && !players[c]->isLocalPlayer() )
					{
						if ( oassailant[c] != assailant[c] )
						{
							oassailant[c] = assailant[c];
							strcpy((char*)net_packet->data, "MUSM");
							net_packet->address.host = net_clients[c - 1].host;
							net_packet->address.port = net_clients[c - 1].port;
							net_packet->data[4] = assailant[c];
							net_packet->len = 5;
							sendPacketSafe(net_sock, -1, net_packet, c - 1);
						}
					}
				}
			}
			combat = assailant[0];
			for ( j = 0; j < MAXPLAYERS; j++ )
			{
				client_selected[j] = NULL;
			}

			// world UI
			Player::WorldUI_t::handleTooltips();

			int backpack_sizey[MAXPLAYERS];

			for ( int player = 0; player < MAXPLAYERS; ++player )
			{
				if ( !players[player]->isLocalPlayer() )
				{
					continue;
				}
				backpack_sizey[player] = players[player]->inventoryUI.DEFAULT_INVENTORY_SIZEY;
				const int inventorySizeX = players[player]->inventoryUI.getSizeX();
				auto& playerInventory = players[player]->inventoryUI;

				if ( stats[player]->cloak && stats[player]->cloak->type == CLOAK_BACKPACK && stats[player]->cloak->status != BROKEN
					&& (shouldInvertEquipmentBeatitude(stats[player]) ? abs(stats[player]->cloak->beatitude) >= 0 : stats[player]->cloak->beatitude >= 0) )
				{
					backpack_sizey[player] = playerInventory.DEFAULT_INVENTORY_SIZEY + playerInventory.getPlayerBackpackBonusSizeY();
				}

				if ( backpack_sizey[player] == playerInventory.DEFAULT_INVENTORY_SIZEY + playerInventory.getPlayerBackpackBonusSizeY() )
				{
					playerInventory.setSizeY(playerInventory.DEFAULT_INVENTORY_SIZEY + playerInventory.getPlayerBackpackBonusSizeY());
				}
				else
				{
					playerInventory.setSizeY(players[player]->inventoryUI.DEFAULT_INVENTORY_SIZEY);
				}
			}

			DebugStats.eventsT5 = std::chrono::high_resolution_clock::now();

			for ( int player = 0; player < MAXPLAYERS; ++player )
			{
				if ( !players[player]->isLocalPlayer() )
				{
					continue;
				}

				int bloodCount = 0;
				for ( node = stats[player]->inventory.first; node != NULL; node = nextnode )
				{
					nextnode = node->next;
					Item* item = (Item*)node->element;
					if ( !item )
					{
						continue;
					}
					// unlock achievements for special collected items
					switch ( item->type )
					{
						case ARTIFACT_SWORD:
							steamAchievement("BARONY_ACH_KING_ARTHURS_BLADE");
							break;
						case ARTIFACT_MACE:
							steamAchievement("BARONY_ACH_SPUD_LORD");
							break;
						case ARTIFACT_AXE:
							steamAchievement("BARONY_ACH_THANKS_MR_SKELTAL");
							break;
						case ARTIFACT_SPEAR:
							steamAchievement("BARONY_ACH_SPEAR_OF_DESTINY");
							break;
						default:
							break;
					}

					if ( item->type == FOOD_BLOOD )
					{
						bloodCount += item->count;
						if ( bloodCount >= 20 )
						{
							steamAchievement("BARONY_ACH_BLOOD_VESSELS");
						}
					}

					if ( itemCategory(item) == WEAPON )
					{
						if ( item->beatitude >= 10 )
						{
							steamAchievement("BARONY_ACH_BLESSED");
						}
					}

					if ( item->status == BROKEN && itemCategory(item) != SPELL_CAT
						&& item->x == Player::PaperDoll_t::ITEM_PAPERDOLL_COORDINATE )
					{
						// item was equipped, but needs a new home in the inventory.
						if ( !players[player]->inventoryUI.moveItemToFreeInventorySlot(item) )
						{
							item->x = players[player]->inventoryUI.getSizeX(); // force unequip below
						}
					}

					// drop any inventory items you don't have room for
					if ( itemCategory(item) != SPELL_CAT 
						&& item->x != Player::PaperDoll_t::ITEM_PAPERDOLL_COORDINATE
						&& item->x != Player::PaperDoll_t::ITEM_RETURN_TO_INVENTORY_COORDINATE
						&& (item->x >= players[player]->inventoryUI.getSizeX() || item->y >= backpack_sizey[player]) )
					{
						messagePlayer(player, MESSAGE_INVENTORY, language[727], item->getName());
						bool droppedAll = false;
						while ( item && item->count > 1 )
						{
							droppedAll = dropItem(item, player);
							if ( droppedAll )
							{
								item = nullptr;
							}
						}
						if ( !droppedAll )
						{
							dropItem(item, player);
						}
					}
					else
					{
						if ( auto_appraise_new_items && players[player]->inventoryUI.appraisal.timer == 0 
							&& !(item->identified) )
						{
							int appraisal_time = players[player]->inventoryUI.appraisal.getAppraisalTime(item);
							if ( appraisal_time < auto_appraise_lowest_time[player] )
							{
								auto_appraise_target[player] = item;
								auto_appraise_lowest_time[player] = appraisal_time;
							}
						}
					}
				}
			}

			DebugStats.eventsT6 = std::chrono::high_resolution_clock::now();

			if ( kills[SHOPKEEPER] >= 3 )
			{
				steamAchievement("BARONY_ACH_PROFESSIONAL_BURGLAR");
			}
			if ( kills[HUMAN] >= 10 )
			{
				steamAchievement("BARONY_ACH_HOMICIDAL_MANIAC");
			}
		}
		else if ( multiplayer == CLIENT )
		{
			// keep alives
			if ( multiplayer == CLIENT ) //lol
			{
				if ( ticks % (TICKS_PER_SECOND * 1) == 0 )
				{
					// send a keep alive every second
					strcpy((char*)net_packet->data, "KPAL");
					net_packet->data[4] = clientnum;
					net_packet->address.host = net_server.host;
					net_packet->address.port = net_server.port;
					net_packet->len = 5;
					sendPacketSafe(net_sock, -1, net_packet, 0);
				}
				if ( losingConnection[0] && ticks - client_keepalive[0] == 1 )
				{
					// regained connection
					losingConnection[0] = false;
					messagePlayer(i, MESSAGE_MISC, language[728]);
				}
				else if ( !losingConnection[0] && ticks - client_keepalive[0] == TICKS_PER_SECOND * 30 - 1 )
				{
					// 30 second timer
					losingConnection[0] = true;
					messageLocalPlayers(MESSAGE_MISC, language[729]);
				}
				else if ( !client_disconnected[c] && ticks - client_keepalive[0] >= TICKS_PER_SECOND * 45 - 1 )
				{
					// additional 15 seconds (disconnect time)
					messageLocalPlayers(MESSAGE_MISC, language[730]);
                    MainMenu::disconnectedFromServer();
					client_disconnected[0] = true;
				}
			}

			// animate tiles
			if ( !gamePaused )
			{
				int x, y, z;
				for ( x = 0; x < map.width; x++ )
				{
					for ( y = 0; y < map.height; y++ )
					{
						for ( z = 0; z < MAPLAYERS; z++ )
						{
							int index = z + y * MAPLAYERS + x * MAPLAYERS * map.height;
							if ( animatedtiles[map.tiles[index]] )
							{
								if ( ticks % 10 == 0 )
								{
									map.tiles[index]--;
									if ( !animatedtiles[map.tiles[index]] )
									{
										do
										{
											map.tiles[index]++;
										}
										while ( animatedtiles[map.tiles[index]] );
										map.tiles[index]--;
									}
								}
								if ( z == 0 )
								{
									// water and lava noises
									if ( ticks % TICKS_PER_SECOND == (y + x * map.height) % TICKS_PER_SECOND && rand() % 3 == 0 )
									{
										if ( lavatiles[map.tiles[index]] )
										{
											// bubbling lava
											playSoundPosLocal( x * 16 + 8, y * 16 + 8, 155, 100 );
										}
										else if ( swimmingtiles[map.tiles[index]] )
										{
											// running water
											playSoundPosLocal( x * 16 + 8, y * 16 + 8, 135, 32 );
										}
									}

									// lava bubbles
									if ( lavatiles[map.tiles[index]] && !gameloopFreezeEntities )
									{
										if ( ticks % 40 == (y + x * map.height) % 40 && rand() % 3 == 0 )
										{
											int c, j = 1 + rand() % 2;
											for ( c = 0; c < j; c++ )
											{
												Entity* entity = newEntity(42, 1, map.entities, nullptr); //Gib entity.
												entity->behavior = &actGib;
												entity->x = x * 16 + rand() % 16;
												entity->y = y * 16 + rand() % 16;
												entity->z = 7.5;
												entity->flags[PASSABLE] = true;
												entity->flags[SPRITE] = true;
												entity->flags[NOUPDATE] = true;
												entity->flags[UPDATENEEDED] = false;
												entity->flags[UNCLICKABLE] = true;
												entity->sizex = 2;
												entity->sizey = 2;
												entity->fskill[3] = 0.01;
												double vel = (rand() % 10) / 20.f;
												entity->vel_x = vel * cos(entity->yaw);
												entity->vel_y = vel * sin(entity->yaw);
												entity->vel_z = -.15 - (rand() % 15) / 100.f;
												entity->yaw = (rand() % 360) * PI / 180.0;
												entity->pitch = (rand() % 360) * PI / 180.0;
												entity->roll = (rand() % 360) * PI / 180.0;
												if ( multiplayer != CLIENT )
												{
													entity_uids--;
												}
												entity->setUID(-3);
											}
										}
									}
								}
							}
						}
					}
				}
			}

			if ( ticks % TICKS_PER_SECOND == 0 )
			{
				updateGameplayStatisticsInMainLoop();
			}

			updatePlayerConductsInMainLoop();

			// ask for entity delete update
			if ( ticks % 4 == 0 && list_Size(map.entities) )
			{
				node_t* nodeToCheck = list_Node(map.entities, ticks % list_Size(map.entities));
				if ( nodeToCheck )
				{
					Entity* entity = (Entity*)nodeToCheck->element;
					if ( entity )
					{
						if ( !entity->flags[NOUPDATE] && entity->getUID() > 0 && entity->getUID() != -2 && entity->getUID() != -3 && entity->getUID() != -4 )
						{
							strcpy((char*)net_packet->data, "ENTE");
							net_packet->data[4] = clientnum;
							SDLNet_Write32(entity->getUID(), &net_packet->data[5]);
							net_packet->address.host = net_server.host;
							net_packet->address.port = net_server.port;
							net_packet->len = 9;
							sendPacket(net_sock, -1, net_packet, 0);
						}
					}
				}
			}

			// run world UI entities
			for ( node = map.worldUI->first; node != nullptr; node = nextnode )
			{
				nextnode = node->next;
				entity = (Entity*)node->element;
				if ( entity && !entity->ranbehavior )
				{
					if ( !gamePaused || (multiplayer && !client_disconnected[0]) )
					{
						++entity->ticks;
					}
					if ( entity->behavior != nullptr )
					{
						if ( !gamePaused || (multiplayer && !client_disconnected[0]) )
						{
							(*entity->behavior)(entity);
						}
						if ( entitiesdeleted.first != nullptr )
						{
							entitydeletedself = false;
							for ( node2 = entitiesdeleted.first; node2 != nullptr; node2 = node2->next )
							{
								if ( entity == (Entity*)node2->element )
								{
									//printlog("DEBUG: Entity deleted self, sprite: %d", entity->sprite);
									entitydeletedself = true;
									break;
								}
							}
							if ( entitydeletedself == false )
							{
								entity->ranbehavior = true;
							}
							nextnode = map.worldUI->first;
							list_FreeAll(&entitiesdeleted);
						}
						else
						{
							entity->ranbehavior = true;
							nextnode = node->next;
						}
					}
				}
			}

			// run entity actions
			for ( node = map.entities->first; node != nullptr; node = nextnode )
			{
				nextnode = node->next;
				entity = (Entity*)node->element;
				if ( entity && !entity->ranbehavior )
				{
					if ( !gamePaused || (multiplayer && !client_disconnected[0]) )
					{
						entity->ticks++;
					}
					if ( entity->behavior != nullptr )
					{
						if ( gameloopFreezeEntities
							&& entity->behavior != &actPlayer
							&& entity->behavior != &actPlayerLimb
							&& entity->behavior != &actHudWeapon
							&& entity->behavior != &actHudShield
							&& entity->behavior != &actHudAdditional
							&& entity->behavior != &actHudArrowModel
							&& entity->behavior != &actLeftHandMagic
							&& entity->behavior != &actRightHandMagic
							&& entity->behavior != &actFlame )
						{
							TimerExperiments::updateEntityInterpolationPosition(entity);
							continue;
						}
						if ( !gamePaused || (multiplayer && !client_disconnected[0]) )
						{
							(*entity->behavior)(entity);
							if ( entitiesdeleted.first != NULL )
							{
								entitydeletedself = false;
								for ( node2 = entitiesdeleted.first; node2 != NULL; node2 = node2->next )
								{
									if ( entity == (Entity*)node2->element )
									{
										entitydeletedself = true;
										break;
									}
								}
								if ( entitydeletedself == false )
								{
									entity->ranbehavior = true;
									TimerExperiments::updateEntityInterpolationPosition(entity);
								}
								nextnode = map.entities->first;
								list_FreeAll(&entitiesdeleted);
							}
							else
							{
								entity->ranbehavior = true;
								nextnode = node->next;
								if ( entity->flags[UPDATENEEDED] && !entity->flags[NOUPDATE] )
								{
									// adjust entity position
									if ( ticks - entity->lastupdate <= TICKS_PER_SECOND / 16 )
									{
										// interpolate to new position
										if ( entity->behavior != &actPlayerLimb || entity->skill[2] != clientnum )
										{
											double ox = 0, oy = 0, onewx = 0, onewy = 0;

											// move the bodyparts of these otherwise the limbs will get left behind in this adjustment.
											if ( entity->behavior == &actPlayer || entity->behavior == &actMonster )
											{
												ox = entity->x;
												oy = entity->y;
												onewx = entity->new_x;
												onewy = entity->new_y;
											}
											entity->x += (entity->new_x - entity->x) / 4;
											entity->y += (entity->new_y - entity->y) / 4;
											if ( entity->behavior != &actArrow )
											{
												// client handles z in actArrow.
												entity->z += (entity->new_z - entity->z) / 4;
											}

											// move the bodyparts of these otherwise the limbs will get left behind in this adjustment.
											if ( entity->behavior == &actPlayer || entity->behavior == &actMonster )
											{
												for ( Entity *bodypart : entity->bodyparts )
												{
													bodypart->x += entity->x - ox;
													bodypart->y += entity->y - oy;
													bodypart->new_x += entity->new_x - onewx;
													bodypart->new_y += entity->new_y - onewy;
												}
											}
										}
									}
									// dead reckoning
									if ( fabs(entity->vel_x) > 0.0001 || fabs(entity->vel_y) > 0.0001 )
									{
										double ox = 0, oy = 0, onewx = 0, onewy = 0;
										if ( entity->behavior == &actPlayer || entity->behavior == &actMonster )
										{
											ox = entity->x;
											oy = entity->y;
											onewx = entity->new_x;
											onewy = entity->new_y;
										}
										real_t dist = clipMove(&entity->x, &entity->y, entity->vel_x, entity->vel_y, entity);
										real_t new_dist = clipMove(&entity->new_x, &entity->new_y, entity->vel_x, entity->vel_y, entity);
										if ( entity->behavior == &actPlayer || entity->behavior == &actMonster )
										{
											for (Entity *bodypart : entity->bodyparts)
											{
												bodypart->x += entity->x - ox;
												bodypart->y += entity->y - oy;
												bodypart->new_x += entity->new_x - onewx;
												bodypart->new_y += entity->new_y - onewy;
											}
										}
									}
									if ( entity->behavior != &actArrow )
									{
										// client handles z in actArrow.
										entity->z += entity->vel_z;
										entity->new_z += entity->vel_z;
									}

									// rotate to new angles
									double dirYaw = entity->new_yaw - entity->yaw;
									while ( dirYaw >= PI )
									{
										dirYaw -= PI * 2;
									}
									while ( dirYaw < -PI )
									{
										dirYaw += PI * 2;
									}
									entity->yaw += dirYaw / 3;
									while ( entity->yaw < 0 )
									{
										entity->yaw += 2 * PI;
									}
									while ( entity->yaw >= 2 * PI )
									{
										entity->yaw -= 2 * PI;
									}
									double dirPitch = entity->new_pitch - entity->pitch;
									if ( entity->behavior != &actArrow )
									{
										// client handles pitch in actArrow.
										while ( dirPitch >= PI )
										{
											dirPitch -= PI * 2;
										}
										while ( dirPitch < -PI )
										{
											dirPitch += PI * 2;
										}
										entity->pitch += dirPitch / 3;
										while ( entity->pitch < 0 )
										{
											entity->pitch += 2 * PI;
										}
										while ( entity->pitch >= 2 * PI )
										{
											entity->pitch -= 2 * PI;
										}
									}
									double dirRoll = entity->new_roll - entity->roll;
									while ( dirRoll >= PI )
									{
										dirRoll -= PI * 2;
									}
									while ( dirRoll < -PI )
									{
										dirRoll += PI * 2;
									}
									entity->roll += dirRoll / 3;
									while ( entity->roll < 0 )
									{
										entity->roll += 2 * PI;
									}
									while ( entity->roll >= 2 * PI )
									{
										entity->roll -= 2 * PI;
									}

									if ( entity->behavior == &actPlayer && entity->skill[2] != clientnum )
									{
										node_t* tmpNode = nullptr;
										int bodypartNum = 0;
										for ( bodypartNum = 0, tmpNode = entity->children.first; tmpNode; tmpNode = tmpNode->next, bodypartNum++ )
										{
											if ( bodypartNum < 9 )
											{
												continue;
											}
											if ( bodypartNum > 10 )
											{
												break;
											}
											// update the players' head and mask as these will otherwise wait until actPlayer to update their rotation. stops clipping.
											if ( bodypartNum == 9 || bodypartNum == 10 )
											{
												Entity* limb = (Entity*)tmpNode->element;
												if ( limb )
												{
													limb->pitch = entity->pitch;
													limb->yaw = entity->yaw;
												}
											}
										}
									}
								}
								TimerExperiments::updateEntityInterpolationPosition(entity);
							}
						}
						else
						{
							TimerExperiments::updateEntityInterpolationPosition(entity);
						}
					}
				}
			}
			for ( node = map.entities->first; node != nullptr; node = node->next )
			{
				entity = (Entity*)node->element;
				entity->ranbehavior = false;
			}
			for ( node = map.worldUI->first; node != nullptr; node = node->next )
			{
				entity = (Entity*)node->element;
				entity->ranbehavior = false;
			}

			// world UI
			Player::WorldUI_t::handleTooltips();

			auto& playerInventory = players[clientnum]->inventoryUI;
			const int inventorySizeX = playerInventory.getSizeX();
			int backpack_sizey = playerInventory.DEFAULT_INVENTORY_SIZEY;
			if ( stats[clientnum]->cloak && stats[clientnum]->cloak->type == CLOAK_BACKPACK && stats[clientnum]->cloak->status != BROKEN
				&& (shouldInvertEquipmentBeatitude(stats[clientnum]) ? abs(stats[clientnum]->cloak->beatitude) >= 0 : stats[clientnum]->cloak->beatitude >= 0) )
			{
				backpack_sizey += playerInventory.getPlayerBackpackBonusSizeY();
			}

			if ( backpack_sizey == playerInventory.DEFAULT_INVENTORY_SIZEY + playerInventory.getPlayerBackpackBonusSizeY() )
			{
				playerInventory.setSizeY(playerInventory.DEFAULT_INVENTORY_SIZEY + playerInventory.getPlayerBackpackBonusSizeY());
			}
			else
			{
				playerInventory.setSizeY(playerInventory.DEFAULT_INVENTORY_SIZEY);
			}

			for ( node = stats[clientnum]->inventory.first; node != NULL; node = nextnode )
			{
				nextnode = node->next;
				Item* item = (Item*)node->element;
				if ( !item )
				{
					continue;
				}
				// unlock achievements for special collected items
				switch ( item->type )
				{
					case ARTIFACT_SWORD:
						steamAchievement("BARONY_ACH_KING_ARTHURS_BLADE");
						break;
					case ARTIFACT_MACE:
						steamAchievement("BARONY_ACH_SPUD_LORD");
						break;
					case ARTIFACT_AXE:
						steamAchievement("BARONY_ACH_THANKS_MR_SKELTAL");
						break;
					case ARTIFACT_SPEAR:
						steamAchievement("BARONY_ACH_SPEAR_OF_DESTINY");
						break;
					default:
						break;
				}

				if ( itemCategory(item) == WEAPON )
				{
					if ( item->beatitude >= 10 )
					{
						steamAchievement("BARONY_ACH_BLESSED");
					}
				}

				if ( item->type == FOOD_BLOOD && item->count >= 20 )
				{
					steamAchievement("BARONY_ACH_BLOOD_VESSELS");
				}

				if ( item->status == BROKEN && itemCategory(item) != SPELL_CAT
					&& item->x == Player::PaperDoll_t::ITEM_PAPERDOLL_COORDINATE )
				{
					// item was equipped, but needs a new home in the inventory.
					if ( !players[clientnum]->inventoryUI.moveItemToFreeInventorySlot(item) )
					{
						item->x = players[clientnum]->inventoryUI.getSizeX(); // force unequip below
					}
				}

				// drop any inventory items you don't have room for
				if ( itemCategory(item) != SPELL_CAT 
					&& item->x != Player::PaperDoll_t::ITEM_PAPERDOLL_COORDINATE
					&& item->x != Player::PaperDoll_t::ITEM_RETURN_TO_INVENTORY_COORDINATE
					&& (item->x >= players[clientnum]->inventoryUI.getSizeX() || item->y >= backpack_sizey) )
				{
					messagePlayer(clientnum, MESSAGE_INVENTORY, language[727], item->getName());
					bool droppedAll = false;
					while ( item && item->count > 1 )
					{
						droppedAll = dropItem(item, clientnum);
						if ( droppedAll )
						{
							item = nullptr;
						}
					}
					if ( !droppedAll )
					{
						dropItem(item, clientnum);
					}
				}
				else
				{
					if ( auto_appraise_new_items && players[clientnum]->inventoryUI.appraisal.timer == 0 && !(item->identified) )
					{
						int appraisal_time = players[clientnum]->inventoryUI.appraisal.getAppraisalTime(item);
						if (appraisal_time < auto_appraise_lowest_time[clientnum])
						{
							auto_appraise_target[clientnum] = item;
							auto_appraise_lowest_time[clientnum] = appraisal_time;
						}
					}
				}
			}

			if ( kills[SHOPKEEPER] >= 3 )
			{
				steamAchievement("BARONY_ACH_PROFESSIONAL_BURGLAR");
			}
			if ( kills[HUMAN] >= 10 )
			{
				steamAchievement("BARONY_ACH_HOMICIDAL_MANIAC");
			}
		}

		// Automatically identify items, shortest time required first
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( players[i]->isLocalPlayer() )
			{
				if ( auto_appraise_target[i] != NULL )
				{
					players[i]->inventoryUI.appraisal.appraiseItem(auto_appraise_target[i]);
				}
			}
		}
	}
}

/*-------------------------------------------------------------------------------

	handleButtons

	Draws buttons and processes clicks

-------------------------------------------------------------------------------*/

//int subx1, subx2, suby1, suby2;
//char subtext[1024];

void handleButtons(void)
{
	node_t* node;
	node_t* nextnode;
	button_t* button;
	int w = 0, h = 0;

	Sint32 mousex = inputs.getMouse(clientnum, Inputs::MouseInputs::X);
	Sint32 mousey = inputs.getMouse(clientnum, Inputs::MouseInputs::Y);
	Sint32 omousex = inputs.getMouse(clientnum, Inputs::MouseInputs::OX);
	Sint32 omousey = inputs.getMouse(clientnum, Inputs::MouseInputs::OY);

	// handle buttons
	for ( node = button_l.first; node != NULL; node = nextnode )
	{
		nextnode = node->next;
		if ( node->element == NULL )
		{
			continue;
		}
		button = (button_t*)node->element;
		if ( button == NULL )
		{
			continue;
		}
		if ( !subwindow && button->focused )
		{
			list_RemoveNode(button->node);
			continue;
		}
		//Hide "Random Character" button if not on first character creation step.
		if (!strcmp(button->label, language[733]))
		{
			if (charcreation_step > 1)
			{
				button->visible = 0;
			}
			else
			{
				button->visible = 1;
			}
		}
		//Hide "Random Name" button if not on character naming screen.
		if ( !strcmp(button->label, language[2498]) )
		{
			if ( charcreation_step != 4 )
			{
				button->visible = 0;
			}
			else
			{
				button->visible = 1;
			}
		}
		if ( button->visible == 0 )
		{
			continue;    // invisible buttons are not processed
		}
		getSizeOfText(ttf12, button->label, &w, &h);
		if ( subwindow && !button->focused )
		{
			// unfocused buttons do not work when a subwindow is active
			drawWindow(button->x, button->y, button->x + button->sizex, button->y + button->sizey);
			ttfPrintText(ttf12, button->x + (button->sizex - w) / 2 - 2, button->y + (button->sizey - h) / 2 + 3, button->label);
		}
		else
		{
			if ( keystatus[button->key] && button->key )
			{
				button->pressed = true;
				button->needclick = false;
			}
			if (button->joykey != -1 && inputs.bControllerRawInputPressed(clientnum, button->joykey) && rebindaction == -1 )
			{
				button->pressed = true;
				button->needclick = false;
			}
			if ( inputs.bMouseLeft(clientnum) )
			{
				if ( mousex >= button->x && mousex < button->x + button->sizex && omousex >= button->x && omousex < button->x + button->sizex )
				{
					if ( mousey >= button->y && mousey < button->y + button->sizey && omousey >= button->y && omousey < button->y + button->sizey )
					{
						node_t* node;
						for ( node = button_l.first; node != NULL; node = node->next )
						{
							if ( node->element == NULL )
							{
								continue;
							}
							button_t* button = (button_t*)node->element;
							button->pressed = false;
						}
						button->pressed = true;
					}
					else if ( !keystatus[button->key] )
					{
						button->pressed = false;
					}
				}
				else if ( !keystatus[button->key] )
				{
					button->pressed = false;
				}
				button->needclick = true;
			}
			if ( button->pressed )
			{
				drawDepressed(button->x, button->y, button->x + button->sizex, button->y + button->sizey);
				ttfPrintText(ttf12, button->x + (button->sizex - w) / 2 - 2, button->y + (button->sizey - h) / 2 + 3, button->label);
				if ( !inputs.bMouseLeft(clientnum) && !keystatus[button->key] )
				{
					if ( ( omousex >= button->x && omousex < button->x + button->sizex ) || !button->needclick )
					{
						if ( ( omousey >= button->y && omousey < button->y + button->sizey ) || !button->needclick )
						{
							keystatus[button->key] = false;
							inputs.controllerClearRawInput(clientnum, button->joykey);
							playSound(139, 64);
							if ( button->action != NULL )
							{
								(*button->action)(button); // run the button's assigned action
								if ( deleteallbuttons )
								{
									deleteallbuttons = false;
									button->pressed = false;
									break;
								}
							}
						}
					}
					button->pressed = false;
				}
			}
			else
			{
				drawWindow(button->x, button->y, button->x + button->sizex, button->y + button->sizey);
				ttfPrintText(ttf12, button->x + (button->sizex - w) / 2 - 2, button->y + (button->sizey - h) / 2 + 3, button->label);
			}
		}

		if ( button->outline )
		{
			//Draw golden border.
			//For such things as which settings tab the controller has presently selected.
			Uint32 color = SDL_MapRGBA(mainsurface->format, 255, 255, 0, 127);
			SDL_Rect pos;
			pos.x = button->x;
			pos.w = button->sizex;
			pos.y = button->y;
			pos.h = button->sizey;
			drawBox(&pos, color, 127);
			//Draw a 2 pixel thick box.
			pos.x = button->x + 1;
			pos.w = button->sizex - 2;
			pos.y = button->y + 1;
			pos.h = button->sizey - 2;
			drawBox(&pos, color, 127);
		}
	}
}

/*-------------------------------------------------------------------------------

	handleEvents

	Handles all SDL events; receives input, updates gamestate, etc.

-------------------------------------------------------------------------------*/

#ifdef NINTENDO
static real_t time_diff = (real_t)0;
#endif

void handleEvents(void)
{
	double d;
	int j;
	int runtimes = 0;

	// calculate app rate
	t = SDL_GetTicks();
	if (ot == 0.0) {
		ot = t;
	}
	real_t timesync = t - ot;
	ot = t;

	// do timer
	time_diff += timesync;
	constexpr real_t frame = (real_t)1000 / (real_t)TICKS_PER_SECOND;
	while (time_diff >= frame) {
		time_diff -= frame;
		timerCallback(0, NULL);
	}

	// calculate fps
	if ( timesync != 0 )
	{
		frameval[cycles % AVERAGEFRAMES] = 1.0 / timesync;
	}
	else
	{
		frameval[cycles % AVERAGEFRAMES] = 1.0;
	}
	d = frameval[0];
	for (j = 1; j < AVERAGEFRAMES; j++)
	{
		d += frameval[j];
	}
	fps = (d / AVERAGEFRAMES) * 1000;

	if (initialized) {
		inputs.updateAllMouse();
	}

	Input::lastInputOfAnyKind = "";
	for (auto& input : Input::inputs) {
		input.updateReleasedBindings();
		input.update();
		input.consumeBindingsSharedWithFaceHotbar();
	}

	while ( SDL_PollEvent(&event) )   // poll SDL events
	{
		// Global events
		switch ( event.type )
		{
			case SDL_QUIT: // if SDL receives the shutdown signal
				mainloop = 0;
				break;
			case SDL_KEYDOWN: // if a key is pressed...
				if ( command )
				{
					if ( event.key.keysym.sym == SDLK_UP )
					{
						if ( !chosen_command && command_history.last )   //If no command is chosen (user has not tried to go up through the commands yet...
						{
							//Assign the chosen command as the last thing the user typed.
							chosen_command = command_history.last;
							strcpy(command_str, ((string_t*)chosen_command->element)->data);
						}
						else if ( chosen_command )
						{
							//Scroll up through the list. Do nothing if already at the top.
							if ( chosen_command->prev )
							{
								chosen_command = chosen_command->prev;
								strcpy(command_str, ((string_t*)chosen_command->element)->data);
							}
						}
					}
					else if ( event.key.keysym.sym == SDLK_DOWN )
					{
						if ( chosen_command )   //If a command is chosen...
						{
							//Scroll down through the history, back to the latest command.
							if ( chosen_command->next )
							{
								//Move on to the newer command.
								chosen_command = chosen_command->next;
								strcpy(command_str, ((string_t*)chosen_command->element)->data);
							}
							else
							{
								//Already latest command. Clear the chosen command.
								chosen_command = NULL;
								strcpy(command_str, "");
							}
						}
					}
				}
				if ( SDL_IsTextInputActive() )
				{
					const size_t length = strlen(inputstr);

#ifdef APPLE
					if ( (event.key.keysym.sym == SDLK_DELETE || event.key.keysym.sym == SDLK_BACKSPACE) && length > 0 )
					{
						size_t utfOffset = 1;

						if ( length > 1 )
						{
							const uint32_t result = UTFD::ValidateUTF8Character(inputstr[length - 1]);

							if ( result > UTFD::UTF8_REJECT )
							{
								++utfOffset;
							}
						}

						inputstr[length - utfOffset] = '\0';
						cursorflash = ticks;
					}
#else

					if ( event.key.keysym.sym == SDLK_BACKSPACE && length > 0 )
					{
						size_t utfOffset = 1;

						if ( length > 1 )
						{
							const uint32_t result = UTFD::ValidateUTF8Character(inputstr[length - 1]);

							if ( result > UTFD::UTF8_REJECT )
							{
								++utfOffset;
							}
						}

						inputstr[length - utfOffset] = '\0';
						cursorflash = ticks;
					}
#endif
					else if ( event.key.keysym.sym == SDLK_c && SDL_GetModState()&KMOD_CTRL )
					{
						SDL_SetClipboardText(inputstr);
						cursorflash = ticks;
					}
					else if ( event.key.keysym.sym == SDLK_v && SDL_GetModState()&KMOD_CTRL )
					{
						strncpy(inputstr, SDL_GetClipboardText(), inputlen);
						cursorflash = ticks;
					}
				}
#ifdef PANDORA
				// Pandora Shoulder as Mouse Button handling
				if ( event.key.keysym.sym == SDLK_RCTRL ) { // L
					mousestatus[SDL_BUTTON_LEFT] = 1; // set this mouse button to 1
					lastkeypressed = 282 + SDL_BUTTON_LEFT;
				}
				else if ( event.key.keysym.sym == SDLK_RSHIFT ) { // R
					mousestatus[SDL_BUTTON_RIGHT] = 1; // set this mouse button to 1
					lastkeypressed = 282 + SDL_BUTTON_RIGHT;
				}
				else
#endif
				{
					lastkeypressed = event.key.keysym.scancode;
					keystatus[event.key.keysym.scancode] = 1; // set this key's index to 1
					Input::keys[event.key.keysym.scancode] = 1;
					Input::lastInputOfAnyKind = SDL_GetKeyName(SDL_GetKeyFromScancode(event.key.keysym.scancode));
				}
				break;
			case SDL_KEYUP: // if a key is unpressed...
#ifdef PANDORA
				if ( event.key.keysym.sym == SDLK_RCTRL ) { // L
					mousestatus[SDL_BUTTON_LEFT] = 0; // set this mouse button to 0
					lastkeypressed = 282 + SDL_BUTTON_LEFT;
				}
				else if ( event.key.keysym.sym == SDLK_RSHIFT ) { // R
					mousestatus[SDL_BUTTON_RIGHT] = 0; // set this mouse button to 0
					lastkeypressed = 282 + SDL_BUTTON_RIGHT;
				}
				else
#endif
				{
					keystatus[event.key.keysym.scancode] = 0; // set this key's index to 0
					Input::keys[event.key.keysym.scancode] = 0;
				}
				break;
			case SDL_TEXTINPUT:
				if ( (event.text.text[0] != 'c' && event.text.text[0] != 'C') || !(SDL_GetModState()&KMOD_CTRL) )
				{
					if ( (event.text.text[0] != 'v' && event.text.text[0] != 'V') || !(SDL_GetModState()&KMOD_CTRL) )
					{
						strncat(inputstr, event.text.text, std::max<size_t>(0, inputlen - strlen(inputstr)));
						cursorflash = ticks;
					}
				}
				break;
			case SDL_MOUSEBUTTONDOWN: // if a mouse button is pressed...
				mousestatus[event.button.button] = 1; // set this mouse button to 1
				Input::mouseButtons[event.button.button] = 1;
				Input::lastInputOfAnyKind = std::string("Mouse") + std::to_string(event.button.button);
				lastkeypressed = 282 + event.button.button;
				break;
			case SDL_MOUSEBUTTONUP: // if a mouse button is released...
				mousestatus[event.button.button] = 0; // set this mouse button to 0
				Input::mouseButtons[event.button.button] = 0;
				buttonclick = 0; // release any buttons that were being held down
				if (initialized)
				{
					for ( int i = 0; i < MAXPLAYERS; ++i )
					{
						if ( inputs.bPlayerUsingKeyboardControl(i) )
						{
							gui_clickdrag[i] = false;
						}
					}
				}
				break;
			case SDL_MOUSEWHEEL:
				if ( event.wheel.y > 0 )
				{
					mousestatus[SDL_BUTTON_WHEELUP] = 1;
					Input::mouseButtons[Input::MOUSE_WHEEL_UP] = 1;
					Input::lastInputOfAnyKind = "MouseWheelUp";
					lastkeypressed = 286;
				}
				else if ( event.wheel.y < 0 )
				{
					mousestatus[SDL_BUTTON_WHEELDOWN] = 1;
					Input::mouseButtons[Input::MOUSE_WHEEL_DOWN] = 1;
					Input::lastInputOfAnyKind = "MouseWheelDown";
					lastkeypressed = 287;
				}
				break;
			case SDL_MOUSEMOTION: // if the mouse is moved...
				if ( firstmouseevent == true )
				{
					firstmouseevent = false;
					break;
				}
				menuselect = 0;
				mousex = event.motion.x;
				mousey = event.motion.y;
#ifdef PANDORA
				if ( xres != 800 || yres != 480 ) {	// SEB Pandora
					mousex = (mousex*xres) / 800;
					mousey = (mousey*yres) / 480;
				}
#endif
				mousexrel += event.motion.xrel;
				mouseyrel += event.motion.yrel;

				if (initialized)
				{
					for ( int i = 0; i < MAXPLAYERS; ++i )
					{
						if ( inputs.bPlayerUsingKeyboardControl(i) )
						{
							if ( !inputs.getVirtualMouse(i)->draw_cursor && !inputs.getVirtualMouse(i)->lastMovementFromController )
							{
								inputs.getVirtualMouse(i)->draw_cursor = true;
							}
							if ( event.user.code == 0 ) 
							{
								// we use SDL_pushEvent() to push a event.user.code == 1 on a gamepad manipulating a mouse event
								// default 0 for normal mouse events
								inputs.getVirtualMouse(i)->lastMovementFromController = false;
							}
							break;
						}
					}
				}
				break;
			case SDL_CONTROLLERBUTTONDOWN: // if joystick button is pressed
			{
				//joystatus[event.cbutton.button] = 1; // set this button's index to 1
				lastkeypressed = 301 + event.cbutton.button;
				char buf[32] = "";
				switch (event.cbutton.button) {
				case SDL_CONTROLLER_BUTTON_A: snprintf(buf, sizeof(buf), "Pad%dButtonA", event.cbutton.which); break;
				case SDL_CONTROLLER_BUTTON_B: snprintf(buf, sizeof(buf), "Pad%dButtonB", event.cbutton.which); break;
				case SDL_CONTROLLER_BUTTON_X: snprintf(buf, sizeof(buf), "Pad%dButtonX", event.cbutton.which); break;
				case SDL_CONTROLLER_BUTTON_Y: snprintf(buf, sizeof(buf), "Pad%dButtonY", event.cbutton.which); break;
				case SDL_CONTROLLER_BUTTON_BACK: snprintf(buf, sizeof(buf), "Pad%dButtonBack", event.cbutton.which); break;
				case SDL_CONTROLLER_BUTTON_START: snprintf(buf, sizeof(buf), "Pad%dButtonStart", event.cbutton.which); break;
				case SDL_CONTROLLER_BUTTON_LEFTSTICK: snprintf(buf, sizeof(buf), "Pad%dButtonLeftStick", event.cbutton.which); break;
				case SDL_CONTROLLER_BUTTON_RIGHTSTICK: snprintf(buf, sizeof(buf), "Pad%dButtonRightStick", event.cbutton.which); break;
				case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: snprintf(buf, sizeof(buf), "Pad%dButtonLeftBumper", event.cbutton.which); break;
				case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: snprintf(buf, sizeof(buf), "Pad%dButtonRightBumper", event.cbutton.which); break;
				case SDL_CONTROLLER_BUTTON_DPAD_LEFT: snprintf(buf, sizeof(buf), "Pad%dDpadX-", event.cbutton.which); break;
				case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: snprintf(buf, sizeof(buf), "Pad%dDpadX+", event.cbutton.which); break;
				case SDL_CONTROLLER_BUTTON_DPAD_UP: snprintf(buf, sizeof(buf), "Pad%dDpadY-", event.cbutton.which); break;
				case SDL_CONTROLLER_BUTTON_DPAD_DOWN: snprintf(buf, sizeof(buf), "Pad%dDpadY+", event.cbutton.which); break;
				}
				Input::lastInputOfAnyKind = buf;
#ifndef NINTENDO
				if ( Input::waitingToBindControllerForPlayer >= 0
					&& event.cbutton.button == SDL_CONTROLLER_BUTTON_A )
				{
					const int id = event.cdevice.which;
					if ( SDL_IsGameController(id) )
					{
						for ( auto& controller : game_controllers )
						{
							if ( controller.isActive() && controller.getID() == id )
							{
							    int player = Input::waitingToBindControllerForPlayer;
								inputs.removeControllerWithDeviceID(id); // clear any other player using this
								inputs.setControllerID(player, id);
								inputs.getVirtualMouse(player)->draw_cursor = false;
								inputs.getVirtualMouse(player)->lastMovementFromController = true;
								printlog("(Device %d added to player %d", id, player);
								Input::waitingToBindControllerForPlayer = -1;
								for (int c = 0; c < 4; ++c) {
								    auto& input = Input::inputs[c];
									input.refresh();
								}
								auto& input = Input::inputs[player];
								input.consumeBinary("MenuConfirm");
								break;
							}
						}
					}
				}
#endif
				break;
			}
			case SDL_CONTROLLERAXISMOTION:
			{
				char buf[32] = "";
				float rebindingDeadzone = Input::getJoystickRebindingDeadzone() * 32768.f;
				switch (event.caxis.axis) {
				case SDL_CONTROLLER_AXIS_LEFTX: 
					if ( event.caxis.value < -rebindingDeadzone )
					{
						snprintf(buf, sizeof(buf), "Pad%dStickLeftX-", event.caxis.which);
					}
					else if ( event.caxis.value > rebindingDeadzone )
					{
						snprintf(buf, sizeof(buf), "Pad%dStickLeftX+", event.caxis.which);
					}
					break;
				case SDL_CONTROLLER_AXIS_LEFTY: 
					if ( event.caxis.value < -rebindingDeadzone )
					{
						snprintf(buf, sizeof(buf), "Pad%dStickLeftY-", event.caxis.which);
					}
					else if ( event.caxis.value > rebindingDeadzone )
					{
						snprintf(buf, sizeof(buf), "Pad%dStickLeftY+", event.caxis.which);
					}
					break;
				case SDL_CONTROLLER_AXIS_RIGHTX: 
					if ( event.caxis.value < -rebindingDeadzone )
					{
						snprintf(buf, sizeof(buf), "Pad%dStickRightX-", event.caxis.which);
					}
					else if ( event.caxis.value > rebindingDeadzone )
					{
						snprintf(buf, sizeof(buf), "Pad%dStickRightX+", event.caxis.which);
					}
					break;
				case SDL_CONTROLLER_AXIS_RIGHTY:
					if ( event.caxis.value < -rebindingDeadzone )
					{
						snprintf(buf, sizeof(buf), "Pad%dStickRightY-", event.caxis.which);
					}
					else if ( event.caxis.value > rebindingDeadzone )
					{
						snprintf(buf, sizeof(buf), "Pad%dStickRightY+", event.caxis.which);
					}
					break;
				case SDL_CONTROLLER_AXIS_TRIGGERLEFT: 
					if ( abs(event.caxis.value) > rebindingDeadzone )
					{
						snprintf(buf, sizeof(buf), "Pad%dLeftTrigger", event.caxis.which); 
					}
					break;
				case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: 
					if ( abs(event.caxis.value) > rebindingDeadzone )
					{
						snprintf(buf, sizeof(buf), "Pad%dRightTrigger", event.caxis.which); 
					}
					break;
				}
				if ( strcmp(buf, "") )
				{
					Input::lastInputOfAnyKind = buf;
				}
				break;
			}
			case SDL_CONTROLLERBUTTONUP:
			{
			    break;
			}
			case SDL_CONTROLLERDEVICEADDED:
			{
				const int id = event.cdevice.which;
				if ( !SDL_IsGameController(id) )
				{
					printlog("Info: device %d is not a game controller! Joysticks are not supported.\n", id);
					break;
				}

				bool deviceAlreadyAdded = false;
				for ( auto& controller : game_controllers )
				{
					if ( controller.isActive() && controller.getID() == id )
					{
						printlog("(Device %d added, but already in use as game controller.)\n", id);
						deviceAlreadyAdded = true;
						break;
					}
				}

				if ( deviceAlreadyAdded )
				{
					break;
				}

				// now find a free controller slot.
				for ( auto& controller : game_controllers )
				{
					if ( controller.isActive() )
					{
						continue;
					}

					if ( SDL_IsGameController(id) && controller.open(id) )
					{
						printlog("(Device %d successfully initialized as game controller.)\n", id);
						//inputs.addControllerIDToNextAvailableInput(id);
						Input::gameControllers[id] = controller.getControllerDevice();
						for (int c = 0; c < 4; ++c) {
							Input::inputs[c].refresh();
						}
					}
					else
					{
						printlog("Info: device %d is not a game controller! Joysticks are not supported.\n", id);
					}
					break;
				}
				break;
			}
			case SDL_CONTROLLERDEVICEREMOVED:
			{
				// device removed uses a 'joystick id', different to the device added event
				const int instanceID = event.cdevice.which;
				SDL_GameController* pad = SDL_GameControllerFromInstanceID(instanceID);
				if ( !pad )
				{
					printlog("(Unknown device removed as game controller, null controller returned.)\n");
				}
				for ( auto& controller : game_controllers )
				{
					if ( controller.isActive() && controller.getControllerDevice() == pad )
					{
						int id = controller.getID();
						inputs.removeControllerWithDeviceID(id);
						printlog("(Device %d removed as game controller, instance id: %d.)\n", id, instanceID);
						controller.close();
						Input::gameControllers.erase(id);
						for ( int c = 0; c < 4; ++c ) {
							Input::inputs[c].refresh();
						}
					}
				}
				break;
			}
			case SDL_JOYDEVICEADDED:
			{
				if ( SDL_IsGameController(event.jdevice.which) )
				{
					// this is supported by the SDL_GameController interface, no need to make a joystick for it
					break;
				}
				SDL_Joystick* joystick = SDL_JoystickOpen(event.jdevice.which);
				if (!joystick) {
					printlog("A joystick was plugged in, but no handle is available!");
				} else {
					Input::joysticks[event.jdevice.which] = joystick;
					printlog("Added joystick '%s' with device index (%d)", SDL_JoystickName(joystick), event.jdevice.which);
					printlog(" NumAxes: %d", SDL_JoystickNumAxes(joystick));
					printlog(" NumButtons: %d", SDL_JoystickNumButtons(joystick));
					printlog(" NumHats: %d", SDL_JoystickNumHats(joystick));
					for (int c = 0; c < 4; ++c) {
						Input::inputs[c].refresh();
					}
				}
				break;
			}
			case SDL_JOYBUTTONDOWN:
			{
				if ( Input::joysticks.find(event.jdevice.which) != Input::joysticks.end() )
				{
					char buf[32] = "";
					snprintf(buf, sizeof(buf), "Joy%dButton%d", event.jbutton.which, event.jbutton.button);
					Input::lastInputOfAnyKind = buf;
				}
				break;
			}
			case SDL_JOYAXISMOTION:
			{
				if ( Input::joysticks.find(event.jdevice.which) != Input::joysticks.end() )
				{
					char buf[32] = "";
					float rebindingDeadzone = Input::getJoystickRebindingDeadzone() * 32768.f;
					if ( event.jaxis.value < -rebindingDeadzone )
					{
						snprintf(buf, sizeof(buf), "Joy%dAxis-%d", event.jaxis.which, event.jaxis.axis);
					}
					else if ( event.jaxis.value > rebindingDeadzone )
					{
						snprintf(buf, sizeof(buf), "Joy%dAxis+%d", event.jaxis.which, event.jaxis.axis);
					}
					if ( strcmp(buf, "") )
					{
						Input::lastInputOfAnyKind = buf;
					}
				}
				break;
			}
			case SDL_JOYHATMOTION:
			{
				if ( Input::joysticks.find(event.jdevice.which) != Input::joysticks.end() )
				{
					char buf[32] = "";
					switch (event.jhat.value) {
					case SDL_HAT_LEFTUP: snprintf(buf, sizeof(buf), "Joy%dHat%dLeftUp", event.jhat.which, event.jhat.hat); break;
					case SDL_HAT_UP: snprintf(buf, sizeof(buf), "Joy%dHat%dUp", event.jhat.which, event.jhat.hat); break;
					case SDL_HAT_RIGHTUP: snprintf(buf, sizeof(buf), "Joy%dHat%dRightUp", event.jhat.which, event.jhat.hat); break;
					case SDL_HAT_RIGHT: snprintf(buf, sizeof(buf), "Joy%dHat%dRight", event.jhat.which, event.jhat.hat); break;
					case SDL_HAT_RIGHTDOWN: snprintf(buf, sizeof(buf), "Joy%dHat%dRightDown", event.jhat.which, event.jhat.hat); break;
					case SDL_HAT_DOWN: snprintf(buf, sizeof(buf), "Joy%dHat%dDown", event.jhat.which, event.jhat.hat); break;
					case SDL_HAT_LEFTDOWN: snprintf(buf, sizeof(buf), "Joy%dHat%dLeftDown", event.jhat.which, event.jhat.hat); break;
					case SDL_HAT_LEFT: snprintf(buf, sizeof(buf), "Joy%dHat%dLeft", event.jhat.which, event.jhat.hat); break;
					case SDL_HAT_CENTERED: snprintf(buf, sizeof(buf), "Joy%dHat%dCentered", event.jhat.which, event.jhat.hat); break;
					}
					Input::lastInputOfAnyKind = buf;
				}
				break;
			}
			case SDL_JOYDEVICEREMOVED:
			{
				if ( SDL_IsGameController(event.jdevice.which) )
				{
					// this is supported by the SDL_GameController interface, no need to make a joystick for it
					break;
				}
				SDL_Joystick* joystick = SDL_JoystickFromInstanceID(event.jdevice.which);
				if (joystick == nullptr) {
					printlog("A joystick was removed, but I don't know which one!");
				} else {
					int index = -1;
					for (auto& pair : Input::joysticks) {
						SDL_Joystick* curr = pair.second;
						if (joystick == curr) {
							SDL_JoystickClose(curr);
							index = pair.first;
							printlog("Removed joystick with device index (%d), instance id (%d)", index, event.jdevice.which);
							Input::joysticks.erase(index);
							for ( int c = 0; c < 4; ++c ) {
								Input::inputs[c].refresh();
							}
							break;
						}
					}
					if (index >= 0) {
						Input::joysticks.erase(index);
					}
				}
				break;
			}
			case SDL_USEREVENT: // if the game timer has elapsed
				if ( runtimes < 5 )
				{
					if ( runtimes == 0 )
					{
#ifdef SOUND
						sound_update(); //Update FMOD and whatnot.
#endif
					}
					if (initialized)
					{
						gameLogic();
					}
					mousexrel = 0;
					mouseyrel = 0;
					if (initialized)
					{
						inputs.updateAllRelMouse();
						for ( int i = 0; i < MAXPLAYERS; ++i )
						{
							if ( inputs.hasController(i) )
							{
								inputs.getController(i)->handleRumble();
								inputs.getController(i)->updateButtonsReleased();
							}
						}
					}
				}
				else
				{
					//printlog("overloaded timer! %d", runtimes);
				}
				++runtimes;
				break;
			case SDL_WINDOWEVENT:
				if ( event.window.event == SDL_WINDOWEVENT_FOCUS_LOST && mute_audio_on_focus_lost )
				{
				    setGlobalVolume(0.f, 0.f, 0.f, 0.f, 0.f);
				}
				else if ( event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED )
				{
				    setGlobalVolume(MainMenu::master_volume,
				        musvolume,
				        sfxvolume,
				        sfxAmbientVolume,
				        sfxEnvironmentVolume);
				}
				else if (event.window.event == SDL_WINDOWEVENT_RESIZED)
				{
					xres = event.window.data1;
					yres = event.window.data2;
					if (!changeVideoMode())
					{
						printlog("critical error! Attempting to abort safely...\n");
						mainloop = 0;
					}
				}
				break;
				/*case SDL_CONTROLLERAXISMOTION:
					printlog("Controller axis motion detected.\n");
					//if (event.caxis.which == 0) //TODO: Multi-controller support.
					//{
						printlog("Controller 0!\n");
						int x = 0, y = 0;
						if (event.caxis.axis == 0) //0 = x axis.
						{
							printlog("X-axis! Value: %d\n", event.caxis.value);
							if (event.caxis.value < -gamepad_deadzone)
							{
								printlog("Left!\n");
								x = -1; //Gamepad moved left.
							}
							else if (event.caxis.value > gamepad_deadzone)
							{
								printlog("Right!\n");
								x = 1; //Gamepad moved right.
							}
						}
						else if (event.caxis.axis  == 1)
						{
							if (event.caxis.value < -gamepad_deadzone)
							{
								printlog("Up!\n");
								y = -1; //Gamepad moved up.
							}
							else if (event.caxis.value > gamepad_deadzone)
							{
								printlog("Down!\n");
								y = 1; //Gamepad moved down.
							}
						}

						if (x || y)
						{
							printlog("Generating mouse motion!\n");
							SDL_Event e;

							e.type = SDL_MOUSEMOTION;
							e.motion.x += x;
							e.motion.y += y;
							e.motion.xrel = x;
							e.motion.yrel = y;
							SDL_PushEvent(&e);
						}
					//}
					break;*/
		}
	}
	if ( !mousestatus[SDL_BUTTON_LEFT] )
	{
		omousex = mousex;
		omousey = mousey;
	}
	if (!initialized) {
		return;
	}
	inputs.updateAllOMouse();
	for ( auto& controller : game_controllers )
	{
		controller.updateButtons();
		controller.updateAxis();
	}

}

/*-------------------------------------------------------------------------------

	timerCallback

	A callback function for the game timer which pushes an SDL event

-------------------------------------------------------------------------------*/

Uint32 timerCallback(Uint32 interval, void* param)
{
	SDL_Event event;
	SDL_UserEvent userevent;

	userevent.type = SDL_USEREVENT;
	userevent.code = 0;
	userevent.data1 = NULL;
	userevent.data2 = NULL;

	event.type = SDL_USEREVENT;
	event.user = userevent;

	int c;
	bool playeralive = false;
	for (c = 0; c < MAXPLAYERS; c++)
		if (players[c] && players[c]->entity && !client_disconnected[c])
		{
			playeralive = true;
		}

	if ((!gamePaused || multiplayer) && !loading && !intro && playeralive)
	{
		completionTime++;
	}
	ticks++;
	if (!loading)
	{
		SDL_PushEvent(&event);    // so the game doesn't overload itself while loading
	}
	return (interval);
}

/*-------------------------------------------------------------------------------

	startMessages

	prints several messages to the console for game start.

-------------------------------------------------------------------------------*/

void startMessages()
{
	// deprecated
}

/*-------------------------------------------------------------------------------

	pauseGame

	pauses or unpauses the game, depending on its current state

-------------------------------------------------------------------------------*/

void pauseGame(int mode, int ignoreplayer)
{
	int c;

	if ( intro )
	{
		return;
	}
	if ( mode == 1 && !gamePaused )
	{
		return;
	}
	if ( mode == 2 && gamePaused )
	{
		return;
	}
	if ( introstage == 9 
		|| introstage == 11 + MOVIE_MIDGAME_BAPHOMET_HUMAN_AUTOMATON
		|| introstage == 11 + MOVIE_MIDGAME_BAPHOMET_MONSTERS
		|| introstage == 11 + MOVIE_MIDGAME_HERX_MONSTERS )
	{
		return;
	}

	if ( (!gamePaused && mode != 1) || mode == 2 )
	{
	    MainMenu::soundToggleMenu();
		gamePaused = true;
		for (int c = 0; c < 4; ++c)
		{
		    auto& input = Input::inputs[c];
			if (input.binary("Pause Game") || (inputs.bPlayerUsingKeyboardControl(c) && keystatus[SDL_SCANCODE_ESCAPE] && !input.isDisabled())) {
			    MainMenu::pause_menu_owner = c;
			    break;
			}
		}
		if ( SDL_GetRelativeMouseMode() )
		{
			SDL_SetRelativeMouseMode(SDL_FALSE);
		}
		return; // doesn't disable the game in multiplayer anymore
		if ( multiplayer == SERVER )
		{
			for ( c = 1; c < MAXPLAYERS; c++ )
			{
				if ( client_disconnected[c] || ignoreplayer == c )
				{
					continue;
				}
				strcpy((char*)net_packet->data, "PAUS");
				net_packet->data[4] = clientnum;
				net_packet->address.host = net_clients[c - 1].host;
				net_packet->address.port = net_clients[c - 1].port;
				net_packet->len = 5;
				sendPacketSafe(net_sock, -1, net_packet, c - 1);
			}
		}
		else if ( multiplayer == CLIENT && ignoreplayer )
		{
			strcpy((char*)net_packet->data, "PAUS");
			net_packet->data[4] = clientnum;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 5;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
	}
	else if ( (gamePaused && mode != 2) || mode == 1 )
	{
		buttonCloseSubwindow(NULL);
		gamePaused = false;
		if ( !SDL_GetRelativeMouseMode() && capture_mouse )
		{
			SDL_SetRelativeMouseMode(EnableMouseCapture);
		}
		return; // doesn't disable the game in multiplayer anymore
		if ( multiplayer == SERVER )
		{
			for ( c = 1; c < MAXPLAYERS; c++ )
			{
				if ( client_disconnected[c] || ignoreplayer == c )
				{
					continue;
				}
				strcpy((char*)net_packet->data, "UNPS");
				net_packet->data[4] = clientnum;
				net_packet->address.host = net_clients[c - 1].host;
				net_packet->address.port = net_clients[c - 1].port;
				net_packet->len = 5;
				sendPacketSafe(net_sock, -1, net_packet, c - 1);
			}
		}
		else if ( multiplayer == CLIENT && ignoreplayer )
		{
			strcpy((char*)net_packet->data, "UNPS");
			net_packet->data[4] = clientnum;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 5;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
	}
}

/*-------------------------------------------------------------------------------

	frameRateLimit

	Returns true until the correct number of frames has passed from the
	beginning of the last cycle in the main loop.

-------------------------------------------------------------------------------*/

// records the SDL_GetTicks() value at the moment the mainloop restarted
Uint64 lastGameTickCount = 0;
float framerateAccumulatedTime = 0.f;
bool frameRateLimit( Uint32 maxFrameRate, bool resetAccumulator)
{
	if ( maxFrameRate == 0 )
	{
		return false;
	}
	float desiredFrameMilliseconds = 1.0f / maxFrameRate;
	Uint64 gameTickCount = SDL_GetPerformanceCounter();
	Uint64 ticksPerSecond = SDL_GetPerformanceFrequency();
	float millisecondsElapsed = (gameTickCount - lastGameTickCount) / static_cast<float>(ticksPerSecond);
	lastGameTickCount = gameTickCount;
	framerateAccumulatedTime += millisecondsElapsed;

	if ( framerateAccumulatedTime < desiredFrameMilliseconds )
	{
		// if enough time is left wait, otherwise just keep spinning so we don't go over the limit...
		return true;
	}
	else
	{
		if ( resetAccumulator )
		{
			framerateAccumulatedTime = 0.f;
		}
		return false;
	}
}

void ingameHud()
{
	for ( int player = 0; player < MAXPLAYERS; ++player )
	{
	    Input& input = Input::inputs[player];

	    // toggle minimap
		// player not needed to be alive
        if ( players[player]->shootmode && !command && input.consumeBinaryToggle("Toggle Minimap") && !gamePaused ) {
            openMinimap(player);
        }

		// inventory interface
		// player not needed to be alive
		if ( players[player]->isLocalPlayer() && !command && input.consumeBinaryToggle("Character Status") && !gamePaused )
		{
			if ( players[player]->shootmode )
			{
				players[player]->openStatusScreen(GUI_MODE_INVENTORY, INVENTORY_MODE_ITEM);
			}
			else
			{
				players[player]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_CLOSE_ALL);
			}
		}

		// spell list
		// player not needed to be alive
		if ( players[player]->isLocalPlayer() && !command && input.consumeBinaryToggle("Spell List") && !gamePaused )   //TODO: Move to function in interface or something?
		{
			if ( input.input("Spell List").isBindingUsingGamepad() )
			{
				// no action, gamepad doesn't use this binding
			}
			// no dropdowns/no selected item, if controller, has to be in inventory/hotbar + !shootmode
			else if ( !inputs.getUIInteraction(player)->selectedItem && !players[player]->GUI.isDropdownActive()
				&& !inputs.hasController(player) )
			{
				players[player]->gui_mode = GUI_MODE_INVENTORY;
				if ( players[player]->shootmode )
				{
					players[player]->openStatusScreen(GUI_MODE_INVENTORY, INVENTORY_MODE_ITEM);
				}
				players[player]->inventoryUI.cycleInventoryTab();
			}
		}

		// spellcasting
		// player needs to be alive
		if ( players[player]->isLocalPlayerAlive() && !gamePaused )
		{
            const bool shootmode = players[player]->shootmode;
			bool hasSpellbook = false;
			bool tryHotbarQuickCast = players[player]->hotbar.faceMenuQuickCast;
			bool tryInventoryQuickCast = players[player]->magic.doQuickCastSpell();
			if ( stats[player]->shield && itemCategory(stats[player]->shield) == SPELLBOOK )
			{
				hasSpellbook = true;
			}

			players[player]->hotbar.faceMenuQuickCast = false;
			bool allowCasting = false;
			if ( tryInventoryQuickCast )
			{
				allowCasting = true;
			}
			else if (!command && shootmode)
			{
			    if (tryHotbarQuickCast || input.binaryToggle("Cast Spell") || (hasSpellbook && input.binaryToggle("Block")) )
			    {
				    allowCasting = true;
				    if (tryHotbarQuickCast == false) {
				        if ((strcmp(input.binding("Cast Spell"), "Mouse3") == 0 || strcmp(input.binding("Block"), "Mouse3") == 0)
					        && players[player]->gui_mode >= GUI_MODE_INVENTORY
					        && (mouseInsidePlayerInventory(player) || mouseInsidePlayerHotbar(player)))
				        {
					        allowCasting = false;
				        }
				    }

				    if ( input.binaryToggle("Block") && hasSpellbook && players[player] && players[player]->entity )
				    {
					    if ( players[player]->entity->effectShapeshift != NOTHING )
					    {
						    if ( players[player]->entity->effectShapeshift == CREATURE_IMP )
						    {
							    // imp allowed to cast via spellbook.
						    }
						    else
						    {
							    allowCasting = false;
						    }
					    }

					    if ( input.binaryToggle("Block")
						    && strcmp(input.binding("Block"), "Mouse3") == 0
						    && inputs.getUIInteraction(player)->itemMenuOpen ) // bound to right click, has context menu open.
					    {
						    allowCasting = false;
					    }
					    else
					    {
						    if ( allowCasting && stats[player]->EFFECTS[EFF_BLIND] )
						    {
							    messagePlayer(player, MESSAGE_EQUIPMENT | MESSAGE_STATUS, language[3863]); // prevent casting of spell.
							    input.consumeBinaryToggle("Block");
							    allowCasting = false;
						    }
					    }
				    }
				}
			}

			if ( allowCasting )
			{
				if ( players[player] && players[player]->entity )
				{
					if ( conductGameChallenges[CONDUCT_BRAWLER] || achievementBrawlerMode )
					{
						if ( achievementBrawlerMode && conductGameChallenges[CONDUCT_BRAWLER] )
						{
							messagePlayer(player, MESSAGE_MISC, language[2999]); // prevent casting of spell.
						}
						else
						{
							if ( achievementBrawlerMode && players[player]->magic.selectedSpell() )
							{
								messagePlayer(player, MESSAGE_MISC, language[2998]); // notify no longer eligible for achievement but still cast.
							}
							if ( tryInventoryQuickCast )
							{
								castSpellInit(players[player]->entity->getUID(), players[player]->magic.quickCastSpell(), false);
							}
							else if ( hasSpellbook && input.consumeBinaryToggle("Block") )
							{
								castSpellInit(players[player]->entity->getUID(), getSpellFromID(getSpellIDFromSpellbook(stats[player]->shield->type)), true);
							}
							else
							{
								castSpellInit(players[player]->entity->getUID(), players[player]->magic.selectedSpell(), false);
							}
							if ( players[player]->magic.selectedSpell() )
							{
								conductGameChallenges[CONDUCT_BRAWLER] = 0;
							}
						}
					}
					else
					{
						if ( tryInventoryQuickCast )
						{
							castSpellInit(players[player]->entity->getUID(), players[player]->magic.quickCastSpell(), false);
						}
						else if ( hasSpellbook && input.consumeBinaryToggle("Block") )
						{
							castSpellInit(players[player]->entity->getUID(), getSpellFromID(getSpellIDFromSpellbook(stats[player]->shield->type)), true);
						}
						else
						{
							castSpellInit(players[player]->entity->getUID(), players[player]->magic.selectedSpell(), false);
						}
					}
				}
				input.consumeBinaryToggle("Cast Spell");
				input.consumeBinaryToggle("Block");
			}
		}
		players[player]->magic.resetQuickCastSpell();

		if ( !command && input.consumeBinaryToggle("Open Log") && !gamePaused )
		{
			// TODO perhaps this should open the new chat log window.
		}

		bool worldUIBlocksFollowerCycle = (
				players[player]->worldUI.isEnabled()
				&& players[player]->worldUI.bTooltipInView
				&& players[player]->worldUI.tooltipsInRange.size() > 1);

		if ( !command && input.consumeBinaryToggle("Cycle NPCs") && !gamePaused )
		{
			if ( !worldUIBlocksFollowerCycle && players[player]->shootmode )
			{
				//(players[player]->shootmode && !worldUIBlocksFollowerCycle) || FollowerMenu[player].followerMenuIsOpen())) ) -- todo needed?

				// can select next follower in inventory or shootmode
				FollowerMenu[player].selectNextFollower();
				players[player]->characterSheet.proficienciesPage = 1;
				if ( players[player]->shootmode && !players[player]->characterSheet.lock_right_sidebar )
				{
					// from now on, allies should be displayed all times
					//players[player]->openStatusScreen(GUI_MODE_INVENTORY, INVENTORY_MODE_ITEM);
				}
			}
		}
	}

	// commands - uses local clientnum only
	Input& input = Input::inputs[clientnum];

	if ( (input.binaryToggle("Chat") || input.binaryToggle("Console Command")) && !command && !gamePaused )
	{
		cursorflash = ticks;
		command = true;
		if ( !input.binaryToggle("Console Command") )
		{
			strcpy(command_str, "");
		}
		else
		{
			strcpy(command_str, "/");
		}
		inputstr = command_str;

		input.consumeBinaryToggle("Chat");
		input.consumeBinaryToggle("Console Command");
		keystatus[SDL_SCANCODE_RETURN] = 0;
		Input::keys[SDL_SCANCODE_RETURN] = 0;

		SDL_StartTextInput();

		// clear follower menu entities.
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( players[i]->isLocalPlayer() && inputs.bPlayerUsingKeyboardControl(i) )
			{
				FollowerMenu[i].closeFollowerMenuGUI();
			}
		}
	}
	else if ( command && !gamePaused )
	{
		int commandPlayer = clientnum;
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( inputs.bPlayerUsingKeyboardControl(i) )
			{
				commandPlayer = i;
				break;
			}
		}

		if ( !SDL_IsTextInputActive() )
		{
			SDL_StartTextInput();
			inputstr = command_str;
		}
		//strncpy(command_str,inputstr,127);
		inputlen = 127;
		if ( keystatus[SDL_SCANCODE_ESCAPE] )   // escape
		{
			keystatus[SDL_SCANCODE_ESCAPE] = 0;
			chosen_command = NULL;
			command = false;
		}
		if ( keystatus[SDL_SCANCODE_RETURN] )   // enter
		{
		    input.consumeBinaryToggle("Chat");
		    input.consumeBinaryToggle("Console Command");
		    keystatus[SDL_SCANCODE_RETURN] = 0;
		    Input::keys[SDL_SCANCODE_RETURN] = 0;
			command = false;

			strncpy(command_str, messageSanitizePercentSign(command_str, nullptr).c_str(), 127);

			if ( multiplayer != CLIENT )
			{
				if ( command_str[0] == '/' )
				{
					// backslash invokes command procedure
					messagePlayer(commandPlayer, MESSAGE_MISC, command_str);
					consoleCommand(command_str);
				}
				else
				{
					if ( strcmp(command_str, "") )
					{
						char chatstring[256];
						strcpy(chatstring, language[739]);
						strcat(chatstring, command_str);
						Uint32 color = SDL_MapRGBA(mainsurface->format, 0, 255, 255, 255);
						if (messagePlayerColor(commandPlayer, MESSAGE_CHAT, color, chatstring)) {
						    playSound(238, 64);
						}
						if ( multiplayer == SERVER )
						{
							// send message to all clients
							for ( int c = 1; c < MAXPLAYERS; c++ )
							{
								if ( client_disconnected[c] || players[c]->isLocalPlayer() )
								{
									continue;
								}
								strcpy((char*)net_packet->data, "MSGS");
								// strncpy() does not copy N bytes if a terminating null is encountered first
								// see http://www.cplusplus.com/reference/cstring/strncpy/
								// see https://en.cppreference.com/w/c/string/byte/strncpy
								// GCC throws a warning (intended) when the length argument to strncpy() in any
								// way depends on strlen(src) to discourage this (and related) construct(s).

								strncpy(chatstring, stats[0]->name, 10);
								chatstring[std::min<size_t>(strlen(stats[0]->name), 10)] = 0; //TODO: Why are size_t and int being compared?
								strcat(chatstring, ": ");
								strcat(chatstring, command_str);
								SDLNet_Write32(color, &net_packet->data[4]);
								strcpy((char*)(&net_packet->data[8]), chatstring);
								net_packet->address.host = net_clients[c - 1].host;
								net_packet->address.port = net_clients[c - 1].port;
								net_packet->len = 8 + strlen(chatstring) + 1;
								sendPacketSafe(net_sock, -1, net_packet, c - 1);
							}
						}
					}
					else
					{
						strcpy(command_str, "");
					}
				}
			}
			else
			{
				if ( command_str[0] == '/' )
				{
					// backslash invokes command procedure
					messagePlayer(commandPlayer, MESSAGE_MISC, command_str);
					consoleCommand(command_str);
				}
				else
				{
					if ( strcmp(command_str, "") )
					{
						char chatstring[256];
						strcpy(chatstring, language[739]);
						strcat(chatstring, command_str);
						Uint32 color = SDL_MapRGBA(mainsurface->format, 0, 255, 255, 255);
						if (messagePlayerColor(commandPlayer, MESSAGE_CHAT, color, chatstring)) {
						    playSound(238, 64);
						}

						// send message to server
						strcpy((char*)net_packet->data, "MSGS");
						net_packet->data[4] = commandPlayer;
						SDLNet_Write32(color, &net_packet->data[5]);
						strcpy((char*)(&net_packet->data[9]), command_str);
						net_packet->address.host = net_server.host;
						net_packet->address.port = net_server.port;
						net_packet->len = 9 + strlen(command_str) + 1;
						sendPacketSafe(net_sock, -1, net_packet, 0);
					}
					else
					{
						strcpy(command_str, "");
					}
				}
			}
			//In either case, save this in the command history.
			if ( strcmp(command_str, "") )
			{
				saveCommand(command_str);
			}
			chosen_command = NULL;
		}
	}
	else
	{
		if ( SDL_IsTextInputActive() )
		{
			SDL_StopTextInput();
		}
		if ( inputstr == command_str )
		{
			inputstr = nullptr;
		}
	}

	// other status
	for ( int player = 0; player < MAXPLAYERS; ++player )
	{
		if ( !players[player]->isLocalPlayer() )
		{
			continue;
		}
		if ( players[player]->shootmode == false )
		{
			if ( inputs.bPlayerUsingKeyboardControl(player) )
			{
				SDL_SetRelativeMouseMode(SDL_FALSE);
			}
		}
		else
		{
			//Do these get called every frame? Might be better to move this stuff into an if (went_back_into_shootmode) { ... } thing.
			//2-3 years later...yes, it is run every frame.
			GenericGUI[player].closeGUI();

			if ( players[player]->bookGUI.bBookOpen )
			{
				players[player]->bookGUI.closeBookGUI();
			}
			if ( players[player]->skillSheet.bSkillSheetOpen )
			{
				players[player]->skillSheet.closeSkillSheet();
			}

			gui_clickdrag[player] = false; //Just a catchall to make sure that any ongoing GUI dragging ends when the GUI is closed.

			if ( capture_mouse && !gamePaused )
			{
				if ( inputs.bPlayerUsingKeyboardControl(player) )
				{
					SDL_SetRelativeMouseMode(EnableMouseCapture);
				}
			}

		}
	}

	DebugStats.t7Inputs = std::chrono::high_resolution_clock::now();

	// Draw the static HUD elements
	if ( !nohud )
	{
		//auto tStartMinimapDraw = std::chrono::high_resolution_clock::now();
		/*auto tEndMinimapDraw = std::chrono::high_resolution_clock::now();
		double timeTaken = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(tEndMinimapDraw - tStartMinimapDraw).count();
		printlog("Minimap draw time: %.5f", timeTaken);*/
		for ( int player = 0; player < MAXPLAYERS; ++player )
		{
			if ( !players[player]->isLocalPlayer() )
			{
				continue;
			}
			//drawMinimap(player); // Draw the Minimap
			//drawStatus(player); // Draw the Status Bar (Hotbar, Hungry/Minotaur Icons, Tooltips, etc.)
		}
	}

	DebugStats.t8Status = std::chrono::high_resolution_clock::now();

	for ( int player = 0; player < MAXPLAYERS; ++player )
	{
		players[player]->messageZone.processChatbox();
		players[player]->hud.processHUD();
		players[player]->inventoryUI.updateSelectedItemAnimation();
		players[player]->inventoryUI.updateInventoryItemTooltip();
		players[player]->hotbar.processHotbar();
		players[player]->inventoryUI.processInventory();
		players[player]->GUI.dropdownMenu.process();
		players[player]->characterSheet.processCharacterSheet();
		players[player]->skillSheet.processSkillSheet();
		players[player]->inventoryUI.updateItemContextMenuClickFrame();
		players[player]->GUI.handleModuleNavigation(false);
		players[player]->inventoryUI.updateCursor();
		players[player]->hotbar.updateCursor();
		players[player]->hud.updateCursor();
		if ( !players[player]->isLocalPlayer() )
		{
			continue;
		}
		//drawSkillsSheet(player);
		if ( !gamePaused )
		{
			if ( !nohud )
			{
				drawStatusNew(player);
			}
			drawSustainedSpells(player);
			updateAppraisalItemBox(player);
		}

		// inventory and stats
		if ( players[player]->shootmode == false )
		{
			if ( players[player]->gui_mode == GUI_MODE_INVENTORY )
			{
				//updateCharacterSheet(player);
				GenericGUI[player].updateGUI();
				players[player]->bookGUI.updateBookGUI();
				//updateRightSidebar(); -- 06/12/20 we don't use this but it still somehow displays stuff :D

			}
			else if ( players[player]->gui_mode == GUI_MODE_MAGIC )
			{
				updateCharacterSheet(player);
				//updateMagicGUI();
			}
			else if ( players[player]->gui_mode == GUI_MODE_SHOP )
			{
				updateCharacterSheet(player);
				updateShopWindow(player);
			}
		}


		bool debugMouse = false;
		if ( debugMouse )
		{
			int x = players[player]->camera_x1() + 12;
			int y = players[player]->camera_y1() + 12;
			printTextFormatted(font8x8_bmp, x, y, "mx:  %4d | my:  %4d", mousex, mousey);
			printTextFormatted(font8x8_bmp, x, y + 12, "mox: %4d | moy: %4d", omousex, omousey);
			printTextFormatted(font8x8_bmp, x, y + 28, "vx:  %4d |  vy: %4d",
				inputs.getVirtualMouse(player)->x, inputs.getVirtualMouse(player)->y);
			printTextFormatted(font8x8_bmp, x, y + 40, "vox: %4d | voy: %4d",
				inputs.getVirtualMouse(player)->ox, inputs.getVirtualMouse(player)->oy);

			if ( inputs.hasController(player) )
			{
				printTextFormatted(font8x8_bmp, x, y + 60, "rawx: %4d | rawy: %4d",
					inputs.getController(player)->oldAxisRightX, inputs.getController(player)->oldAxisRightY);
				printTextFormatted(font8x8_bmp, x, y + 72, "flx: %4f | fly: %4f",
					inputs.getController(player)->oldFloatRightX, inputs.getController(player)->oldFloatRightY);
				printTextFormatted(font8x8_bmp, x, y + 84, "deadzonex: %3.1f%% | deadzoney: %3.1f%%",
					inputs.getController(player)->leftStickDeadzone * 100 / 32767.0,
					inputs.getController(player)->rightStickDeadzone * 100 / 32767.0);
			}
			if ( players[player]->entity )
			{
				printTextFormatted(font8x8_bmp, x, y + 100, "velx:  %4f | vely:  %4f",
					players[player]->entity->vel_x, players[player]->entity->vel_y);
			}
			if ( inputs.hasController(player) )
			{
				printTextFormatted(font8x8_bmp, x, y + 112, "leftx: %4f | lefty: %4f",
					inputs.getController(player)->getLeftXPercent(),
					inputs.getController(player)->getLeftYPercent());
			}
		}
	}

	DebugStats.t9GUI = std::chrono::high_resolution_clock::now();

	UIToastNotificationManager.drawNotifications(movie, true); // draw this before the cursors

	// pointer in inventory screen
	for ( int player = 0; player < MAXPLAYERS; ++player )
	{
		if ( !players[player]->isLocalPlayer() )
		{
			continue;
		}

		SDL_Rect pos;

		FollowerRadialMenu& followerMenu = FollowerMenu[player];

		Frame* frame = players[player]->inventoryUI.frame;
		Frame* draggingItemFrame = nullptr;
		Frame* oldDraggingItemFrame = nullptr;
		if ( frame )
		{
			if ( draggingItemFrame = frame->findFrame("dragging inventory item") )
			{
				draggingItemFrame->setDisabled(true);
			}
			// unused for now
			if ( bUseSelectedSlotCycleAnimation )
			{
				if ( oldDraggingItemFrame = frame->findFrame("dragging inventory item old") )
				{
					oldDraggingItemFrame->setDisabled(true);
					if ( !inputs.getUIInteraction(player)->selectedItem )
					{
						if ( auto img = oldDraggingItemFrame->findImage("item sprite img") )
						{
							img->path = "";
						}
					}
				}
			}
		}

		if ( players[player]->shootmode == false )
		{
			// dragging items, player not needed to be alive
			if ( inputs.getUIInteraction(player)->selectedItem )
			{
				Item*& selectedItem = inputs.getUIInteraction(player)->selectedItem;
				if ( frame )
				{
					if ( draggingItemFrame )
					{
						if ( oldDraggingItemFrame )
						{
							oldDraggingItemFrame->setDisabled(false);
						}
						updateSlotFrameFromItem(draggingItemFrame, selectedItem);

						Frame* selectedSlotCursor = players[player]->inventoryUI.selectedItemCursorFrame;
						if ( !inputs.getVirtualMouse(player)->draw_cursor )
						{
							SDL_Rect selectedItemCursorPos = selectedSlotCursor->getSize();
							SDL_Rect oldDraggingItemPos = selectedItemCursorPos;

							int cursorOffset = players[player]->inventoryUI.cursor.cursorToSlotOffset;
							real_t animX = players[player]->inventoryUI.selectedItemAnimate.animateX;
							real_t animY = players[player]->inventoryUI.selectedItemAnimate.animateY;
							real_t maxX = (selectedItemCursorPos.w - cursorOffset * 2) / 2;
							real_t maxY = (selectedItemCursorPos.h - cursorOffset * 2) / 2;

							if ( !bUseSelectedSlotCycleAnimation )
							{
								// linear diagonal motion here
								selectedItemCursorPos.x += cursorOffset +
									(animX) * (selectedItemCursorPos.w - cursorOffset * 2) / 2;
								selectedItemCursorPos.y += cursorOffset +
									(animY) * (selectedItemCursorPos.h - cursorOffset * 2) / 2;
								draggingItemFrame->setSize(selectedItemCursorPos);
							}
							else
							{
								if ( oldDraggingItemFrame )
								{
									// option to move items in a circular motion
									real_t magnitudeX = cos(animX * PI + 5 * PI / 4);
									real_t magnitudeY = sin(animY * PI + 5 * PI / 4);
									selectedItemCursorPos.x += cursorOffset + maxX / 2 + magnitudeX * maxX;
									selectedItemCursorPos.y += cursorOffset + maxY / 2 + magnitudeY * maxY;
									draggingItemFrame->setSize(selectedItemCursorPos);

									real_t magnitudeX2 = cos(animX * PI + 1 * PI / 4);
									real_t magnitudeY2 = sin(animY * PI + 1 * PI / 4);
									oldDraggingItemPos.x += cursorOffset + maxX / 2 + magnitudeX2 * maxX;
									oldDraggingItemPos.y += cursorOffset + maxY / 2 + magnitudeY2 * maxY;
									oldDraggingItemFrame->setSize(oldDraggingItemPos);
									if ( animX == 1.0 || animY == 1.0 )
									{
										oldDraggingItemFrame->setDisabled(true);
									}
								}
							}
						}
						else if ( inputs.getVirtualMouse(player)->draw_cursor )
						{
							pos.x = inputs.getMouse(player, Inputs::X) - 15;
							pos.y = inputs.getMouse(player, Inputs::Y) - 15;
							pos.x *= ((float)Frame::virtualScreenX / (float)xres);
							pos.y *= ((float)Frame::virtualScreenY / (float)yres);
							pos.x -= players[player]->camera_virtualx1();
							pos.y -= players[player]->camera_virtualy1();
							draggingItemFrame->setSize(SDL_Rect{ pos.x, pos.y, draggingItemFrame->getSize().w, draggingItemFrame->getSize().h });
						}
					}
#ifndef NDEBUG
					if ( enableDebugKeys )
					{
						// debug for controllers
						auto cursor = Image::get("images/system/cursor_hand.png");
						if ( keystatus[SDL_SCANCODE_J] )
						{
							cursor = Image::get("images/system/cursor.png");
						}

						pos.x = inputs.getVirtualMouse(player)->x - (cursor->getWidth() / 7) - cursor->getWidth() / 2;
						pos.y = inputs.getVirtualMouse(player)->y - (cursor->getHeight() / 7) - cursor->getHeight() / 2;
						pos.w = cursor->getWidth();
						pos.h = cursor->getHeight();
						cursor->drawColor(nullptr, pos, SDL_Rect{ 0, 0, xres, yres }, 0xFF0000FF);
					}
#endif // !NDEBUG
				}
				else
				{
					pos.x = inputs.getMouse(player, Inputs::X) - 15;
					pos.y = inputs.getMouse(player, Inputs::Y) - 15;
					pos.w = 32 * uiscale_inventory;
					pos.h = 32 * uiscale_inventory;

					drawImageScaled(itemSprite(selectedItem), NULL, &pos);
					if ( selectedItem->count > 1 )
					{
						ttfPrintTextFormatted(ttf8, pos.x + 24 * uiscale_inventory, pos.y + 24 * uiscale_inventory, "%d", selectedItem->count);
					}
					if ( itemCategory(selectedItem) != SPELL_CAT )
					{
						if ( itemIsEquipped(selectedItem, player) )
						{
							pos.y += 16;
							drawImage(equipped_bmp, NULL, &pos);
						}
						else if ( selectedItem->status == BROKEN )
						{
							pos.y += 16;
							drawImage(itembroken_bmp, NULL, &pos);
						}
					}
					else
					{
						spell_t* spell = getSpellFromItem(player, selectedItem);
						if ( players[player]->magic.selectedSpell() == spell &&
							(players[player]->magic.selected_spell_last_appearance == selectedItem->appearance
								|| players[player]->magic.selected_spell_last_appearance == -1) )
						{
							pos.y += 16;
							drawImage(equipped_bmp, NULL, &pos);
						}
					}
				}
			}
			else if ( players[player]->isLocalPlayer() && followerMenu.selectMoveTo &&
				(followerMenu.optionSelected == ALLY_CMD_MOVETO_SELECT
					|| followerMenu.optionSelected == ALLY_CMD_ATTACK_SELECT) )
			{
				auto cursor = Image::get("images/system/cursor_hand.png");
				pos.x = inputs.getMouse(player, Inputs::X) - cursor->getWidth() / 2;
				pos.y = inputs.getMouse(player, Inputs::Y) - cursor->getHeight() / 2;
				pos.w = cursor->getWidth();
				pos.h = cursor->getHeight();
				cursor->draw(nullptr, pos, SDL_Rect{0, 0, xres, yres});
				if ( followerMenu.optionSelected == ALLY_CMD_MOVETO_SELECT )
				{
					if ( followerMenu.followerToCommand
						&& (followerMenu.followerToCommand->getMonsterTypeFromSprite() == SENTRYBOT
							|| followerMenu.followerToCommand->getMonsterTypeFromSprite() == SPELLBOT)
						)
					{
						ttfPrintTextFormatted(ttf12, pos.x + 24, pos.y + 24, language[3650]);
					}
					else
					{
						ttfPrintTextFormatted(ttf12, pos.x + 24, pos.y + 24, language[3039]);
					}
				}
				else
				{
					if ( !strcmp(followerMenu.interactText, "") )
					{
						if ( followerMenu.followerToCommand )
						{
							int type = followerMenu.followerToCommand->getMonsterTypeFromSprite();
							if ( followerMenu.allowedInteractItems(type)
								|| followerMenu.allowedInteractFood(type)
								|| followerMenu.allowedInteractWorld(type)
								)
							{
								ttfPrintTextFormatted(ttf12, pos.x + 24, pos.y + 24, language[4041]); // "Interact with..."
							}
							else
							{
								ttfPrintTextFormatted(ttf12, pos.x + 24, pos.y + 24, language[4042]); // "Attack..."
							}
						}
						else
						{
							ttfPrintTextFormatted(ttf12, pos.x + 24, pos.y + 24, language[4041]); // "Interact with..."
						}
					}
					else
					{
						ttfPrintTextFormatted(ttf12, pos.x + 24, pos.y + 24, "%s", followerMenu.interactText);
					}
				}
			}
			else if ( inputs.getVirtualMouse(player)->draw_cursor )
			{
				auto cursor = Image::get("images/system/cursor_hand.png");
				real_t& mouseAnim = inputs.getVirtualMouse(player)->mouseAnimationPercent;
				if ( mousestatus[SDL_BUTTON_LEFT] )
				{
					mouseAnim = .5;
				}
				if ( mouseAnim > .25 )
				{
					cursor = Image::get("images/system/cursor_hand2.png");
				}
				if ( mouseAnim > 0.0 )
				{
					mouseAnim -= .05;
				}
				if ( keystatus[SDL_SCANCODE_J] )
				{
					cursor = Image::get("images/system/cursor.png");
				}
				pos.x = inputs.getMouse(player, Inputs::X) - (mouseAnim * cursor->getWidth() / 7) - cursor->getWidth() / 2;
				pos.y = inputs.getMouse(player, Inputs::Y) - (mouseAnim * cursor->getHeight() / 7) - cursor->getHeight() / 2;
				pos.w = cursor->getWidth();
				pos.h = cursor->getHeight();
				if ( inputs.getUIInteraction(player)->itemMenuOpen && inputs.getUIInteraction(player)->itemMenuFromHotbar )
				{
					// adjust cursor to match selection
					pos.y -= inputs.getUIInteraction(player)->itemMenuOffsetDetectionY;
				}
				cursor->draw(nullptr, pos, SDL_Rect{0, 0, xres, yres});
			}
			else
			{
#ifndef NDEBUG
				// debug for controllers
				if ( enableDebugKeys )
				{
					auto cursor = Image::get("images/system/cursor_hand.png");
					if ( keystatus[SDL_SCANCODE_J] )
					{
						cursor = Image::get("images/system/cursor.png");
					}
					pos.x = inputs.getVirtualMouse(player)->x - (cursor->getWidth() / 7) - cursor->getWidth() / 2;
					pos.y = inputs.getVirtualMouse(player)->y - (cursor->getHeight() / 7) - cursor->getHeight() / 2;
					pos.w = cursor->getWidth();
					pos.h = cursor->getHeight();
					cursor->drawColor(nullptr, pos, SDL_Rect{ 0, 0, xres, yres }, 0xFF0000FF);
				}
#endif
			}
		}
		players[player]->hud.updateWorldTooltipPrompts();
	}

	if ( ItemTooltips.autoReload )
	{
		if ( ticks % TICKS_PER_SECOND == 0 )
		{
			ItemTooltips.readTooltipsFromFile();
		}
	}
}

/*-------------------------------------------------------------------------------

	main

	Initializes game resources, harbors main game loop, and cleans up
	afterwards

-------------------------------------------------------------------------------*/

#include <stdio.h>
//#include <unistd.h>
#include <stdlib.h>
#ifdef APPLE
#include <mach-o/dyld.h>
#endif

int main(int argc, char** argv)
{
#ifdef WINDOWS
	SetUnhandledExceptionFilter(unhandled_handler);
#endif // WINDOWS
#ifdef NINTENDO
	nxInit();
	PHYSFS_init(nullptr);
#endif // NINTENDO

#ifdef LINUX
	struct sigaction sa;

	memset(&sa, 0, sizeof(struct sigaction));
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = segfault_sigaction;
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &sa, NULL);

	memset(&sa, 0, sizeof(struct sigaction));
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = stop_sigaction;
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGSTOP, &sa, NULL);

	memset(&sa, 0, sizeof(struct sigaction));
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = continue_sigaction;
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGCONT, &sa, NULL);

	(void)chdir(BASE_DATA_DIR); // fixes a lot of headaches...
#endif

#ifndef NINTENDO
	try
#endif
	{
#if defined(APPLE) || defined(BSD) || defined(HAIKU)
#ifdef APPLE
		uint32_t buffsize = 4096;
		char binarypath[buffsize];
		int result = _NSGetExecutablePath(binarypath, &buffsize);
#elif defined(BSD)
#if defined(__FreeBSD__)
		int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
		size_t buffsize = PATH_MAX;
		char binarypath[buffsize];
		int result = sysctl(mib, 4, binarypath, &buffsize, NULL, 0);
#elif defined(__NetBSD__)
		constexpr size_t buffsize = PATH_MAX;
		char binarypath[buffsize];
		int result = (readlink("/proc/curproc/exe", binarypath, buffsize) > 0 ? 0 : -1);
#endif
#elif defined(HAIKU)
		size_t buffsize = PATH_MAX;
		char binarypath[buffsize];
		int result = find_path(B_APP_IMAGE_SYMBOL, B_FIND_PATH_IMAGE_PATH, NULL, binarypath, sizeof(binarypath)); // B_OK is 0
#endif
		if (result == 0)   //It worked.
		{
			printlog("Binary path: %s\n", binarypath);
			char* last = strrchr(binarypath, '/');
			*last = '\0';
#ifdef APPLE
			char execpath[buffsize];
			strcpy(execpath, binarypath);
			//char* last = strrchr(execpath, '/');
			//strcat(execpath, '/');
			//strcat(execpath, "/../../../");
			printlog("Chrooting to directory: %s\n", execpath);
			chdir(execpath);
			///Users/ciprian/barony/barony-sdl2-take2/barony.app/Contents/MacOS/barony
			chdir("..");
			//chdir("..");
			chdir("Resources");
			//chdir("..");
			//chdir("..");
#endif
		}
		else
		{
			printlog("Failed to get binary path. Program may not work corectly!\n");
		}
#endif
		SDL_Rect pos, src;
		int c;
		//int tilesreceived=0;
		//Mix_Music **music, *intromusic, *splashmusic, *creditsmusic;
		node_t* node;
		Entity* entity;
		//SDL_Surface *sky_bmp;
		light_t* light;

		size_t datadirsz = std::min(sizeof(datadir) - 1, strlen(BASE_DATA_DIR));
		strncpy(datadir, BASE_DATA_DIR, datadirsz);
		datadir[datadirsz] = '\0';
#ifdef WINDOWS
		strcpy(outputdir, "./");
#else
 #ifndef NINTENDO
		char *basepath = getenv("HOME");
  #ifdef USE_EOS
   #ifdef STEAMWORKS
		//Steam + EOS
		snprintf(outputdir, sizeof(outputdir), "%s/.barony", basepath);
   #else
		//Just EOS.
		std::string firstDotdir(basepath);
		firstDotdir += "/.barony/";
		if (access(firstDotdir.c_str(), F_OK) == -1)
		{
			mkdir(firstDotdir.c_str(), 0777); //Since this mkdir is not equivalent to mkdir -p, have to create each part of the path manually.
		}
		snprintf(outputdir, sizeof(outputdir), "%s/epicgames", firstDotdir.c_str());
   #endif
  #else //USE_EOS
		//No EOS. Could be Steam though. Or could also not.
		snprintf(outputdir, sizeof(outputdir), "%s/.barony", basepath);
  #endif
		if (access(outputdir, F_OK) == -1)
		{
			mkdir(outputdir, 0777);
		}
 #else // !NINTENDO
		strcpy(outputdir, "save:");
 #endif // NINTENDO
#endif
		// read command line arguments
		if ( argc > 1 )
		{
			for (c = 1; c < argc; c++)
			{
				if ( argv[c] != NULL )
				{
					if ( !strcmp(argv[c], "-windowed") )
					{
						fullscreen = 0;
					}
					else if ( !strncmp(argv[c], "-size=", 6) )
					{
						strncpy(tempstr, argv[c] + 6, strcspn(argv[c] + 6, "x"));
						xres = std::max(320, atoi(tempstr));
						yres = std::max(200, atoi(argv[c] + 6 + strcspn(argv[c] + 6, "x") + 1));
					}
					else if ( !strncmp(argv[c], "-map=", 5) )
					{
						strcpy(maptoload, argv[c] + 5);
						loadingmap = true;
					}
					else if ( !strncmp(argv[c], "-gen=", 5) )
					{
						strcpy(maptoload, argv[c] + 5);
						loadingmap = true;
						genmap = true;
					}
					else if ( !strncmp(argv[c], "-config=", 8) )
					{
						strcpy(configtoload, argv[c] + 5);
						loadingconfig = true;
					}
					else if (!strncmp(argv[c], "-quickstart=", 12))
					{
						strcpy(classtoquickstart, argv[c] + 12);
					}
					else if (!strncmp(argv[c], "-datadir=", 9))
					{
						datadirsz = std::min(sizeof(datadir) - 1, strlen(argv[c] + 9));
						strncpy(datadir, argv[c] + 9, datadirsz);
						datadir[datadirsz] = '\0';
					}
					else if ( !strcmp(argv[c], "-nosound") )
					{
						no_sound = true;
					}
					else
					{
#ifdef USE_EOS
						EOS.CommandLineArgs.push_back(argv[c]);
#endif // USE_EOS
					}
				}
			}
		}
		printlog("Data path is %s", datadir);
		printlog("Output path is %s", outputdir);

#ifdef NINTENDO
//		strcpy(classtoquickstart, "warrior");
#endif

		// load default language file (english)
		if ( loadLanguage("en") )
		{
			printlog("Fatal error: failed to load default language file!\n");
			if (logfile)
			{
				fclose(logfile);
			}
			exit(1);
		}

		Input::defaultBindings();

		// load config file
		if ( loadingconfig )
		{
			loadConfig(configtoload);
		}
		else
		{
			loadDefaultConfig();
		}
		bool load_successful = MainMenu::settingsLoad();
		if ( load_successful ) {
			MainMenu::settingsApply();
		}
		else {
			MainMenu::settingsReset();
			MainMenu::settingsApply();
			skipintro = false;
		}

		// initialize map
		map.tiles = nullptr;
		map.vismap = nullptr;
		map.entities = (list_t*) malloc(sizeof(list_t));
		map.entities->first = nullptr;
		map.entities->last = nullptr;
		map.creatures = new list_t;
		map.creatures->first = nullptr;
		map.creatures->last = nullptr;
		map.worldUI = new list_t;
		map.worldUI->first = nullptr;
		map.worldUI->last = nullptr;

		// initialize engine
		if ( (c = initApp("Barony", fullscreen)) )
		{
			printlog("Critical error: %d\n", c);
#ifdef STEAMWORKS
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Uh oh",
									"Barony has encountered a critical error and cannot start.\n\n"
									"Please check the log.txt file in the game directory for additional info\n"
									"and verify Steam is running. Alternatively, contact us through our website\n"
									"at http://www.baronygame.com/ for support.",
				screen);
#elif defined USE_EOS
			if ( EOS.appRequiresRestart == EOS_EResult::EOS_Success )
			{
				// restarting app from launcher.
			}
			else
			{
				SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Uh oh",
					"Barony has encountered a critical error and cannot start.\n\n"
					"Please check the log.txt file in the game directory for additional info,\n"
					"and verify the game is launched through the Epic Games Store. \n"
					"Alternatively, contact us through our website at http://www.baronygame.com/ for support.",
					screen);
			}
#else
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Uh oh",
									"Barony has encountered a critical error and cannot start.\n\n"
									"Please check the log.txt file in the game directory for additional info,\n"
									"or contact us through our website at http://www.baronygame.com/ for support.",
									screen);
#endif
			deinitApp();
			exit(c);
		}

		// init message
		printlog("Barony version: %s\n", VERSION);
		time_t timething;
		char buffer[32];
		struct tm* tm_info;
		time(&timething);
		tm_info = localtime(&timething);
		strftime( buffer, 32, "%Y-%m-%d %H-%M-%S", tm_info );
		printlog("Launch time: %s\n", buffer);

		if ( (c = initGame()) )
		{
			printlog("Critical error in initGame: %d\n", c);
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Uh oh",
			                         "Barony has encountered a critical error and cannot start.\n\n"
			                         "Please check the log.txt file in the game directory for additional info,\n"
			                         "or contact us through our website at http://www.baronygame.com/ for support.",
			                         screen);
			deinitGame();
			deinitApp();
			exit(c);
		}
		initialized = true;

		// initialize player conducts
		setDefaultPlayerConducts();

		// seed random generators
		srand(time(NULL));
		fountainSeed.seed(rand());

		// play splash sound
#ifdef MUSIC
		playMusic(splashmusic, false, false, false);
#endif

		int old_sdl_ticks = 0;
		int indev_timer = 0;

		// main loop

		printlog("running main loop.\n");
		while (mainloop)
		{
			Frame::numFindFrameCalls = 0;
			// record the time at the start of this cycle
			lastGameTickCount = SDL_GetPerformanceCounter();
			DebugStats.t1StartLoop = std::chrono::high_resolution_clock::now();
			// game logic
			if ( !intro )
			{
				// handle network messages
				// only run up to % framerate interval (1 / (fps * networkTickrate))
				if ( networkTickrate == 0 )
				{
					networkTickrate = 2;
				}
				if ( multiplayer == CLIENT )
				{
					clientHandleMessages(fpsLimit * networkTickrate);
				}
				else if ( multiplayer == SERVER )
				{
					serverHandleMessages(fpsLimit * networkTickrate);
				}
			}
			DebugStats.t21PostHandleMessages = std::chrono::high_resolution_clock::now();
			handleEvents();
			DebugStats.t2PostEvents = std::chrono::high_resolution_clock::now();
			// handle steam callbacks
#ifdef STEAMWORKS
			if ( g_SteamLeaderboards )
			{
				g_SteamLeaderboards->ProcessLeaderboardUpload();
			}
			SteamAPI_RunCallbacks();
#endif
#ifdef USE_EOS
			EOS_Platform_Tick(EOS.PlatformHandle);
			EOS_Platform_Tick(EOS.ServerPlatformHandle);
			EOS.StatGlobalManager.updateQueuedStats();
			EOS.AccountManager.handleLogin();
			EOS.CrossplayAccountManager.handleLogin();
#endif // USE_EOS

			DebugStats.t3SteamCallbacks = std::chrono::high_resolution_clock::now();
			if ( intro )
			{
				globalLightModifierActive = GLOBAL_LIGHT_MODIFIER_STOPPED;
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					players[i]->shootmode = false; //Hack because somebody put a shootmode = true where it don't belong, which might and does break stuff.
				}
				if ( introstage == -1 )
				{
					// hack to fix these things from breaking everything...
					for ( int i = 0; i < MAXPLAYERS; ++i )
					{
						players[i]->hud.arm = nullptr;
						players[i]->hud.weapon = nullptr;
						players[i]->hud.magicLeftHand = nullptr;
						players[i]->hud.magicRightHand = nullptr;
					}

					// black background
					drawRect(NULL, 0, 255);

#ifdef USE_FMOD
					// fmod logo
					auto fmod_logo = Image::get("images/system/fmod-logo.png");
					int w = fmod_logo->getWidth() / 3;
					int h = fmod_logo->getHeight() / 3;
					fmod_logo->drawColor(nullptr,
					    SDL_Rect{xres - w - 16, yres - h - 16, w, h},
					    SDL_Rect{0, 0, xres, yres}, makeColor(150, 150, 150, 255));
#endif

					// team splash
					drawGear(xres / 2, yres / 2, gearsize, gearrot);
					drawLine(xres / 2 - 160, yres / 2 + 112, xres / 2 + 160, yres / 2 + 112, SDL_MapRGB(mainsurface->format, 255, 32, 0), std::min<Uint16>(logoalpha, 255));
					printTextFormattedAlpha(font16x16_bmp, (xres / 2) - strlen("Turning Wheel") * 9, yres / 2 + 128, std::min<Uint16>(std::max<Uint16>(0, logoalpha), 255), "Turning Wheel");
					if ( logoalpha >= 255 && !fadeout )
					{
						if ( !skipintro && !strcmp(classtoquickstart, "") )
						{
							MainMenu::beginFade(MainMenu::FadeDestination::IntroStoryScreen);
						}
						else
						{
							MainMenu::beginFade(MainMenu::FadeDestination::TitleScreen);
						}
					}

					bool skipButtonPressed = false;
					for ( int i = 0; i < MAXPLAYERS; ++i )
					{
						if ( Input::inputs[i].consumeBinaryToggle("MenuConfirm") )
						{
							skipButtonPressed = true;
						}
						if ( Input::inputs[i].consumeBinaryToggle("MenuCancel") )
						{
							skipButtonPressed = true;
						}
					}
					if ( Input::keys[SDL_SCANCODE_ESCAPE] )
					{
						Input::keys[SDL_SCANCODE_ESCAPE] = 0;
						skipButtonPressed = true;
					}

					if ( fadefinished || skipButtonPressed )
					{
						fadealpha = 255;
						int menuMapType = 0;
						switch ( rand() % 4 ) // STEAM VERSION INTRO
						{
							case 0:
							case 1:
								menuMapType = loadMainMenuMap(true, false);
								break;
							case 2:
							case 3:
								menuMapType = loadMainMenuMap(false, false);
								break;
							default:
								break;
						}
						numplayers = 0;
						multiplayer = 0;
						assignActions(&map);
						generatePathMaps();
						introstage = 1;
						if ( !skipintro && !strcmp(classtoquickstart, "") )
						{
							MainMenu::beginFade(MainMenu::FadeDestination::IntroStoryScreenNoMusicFade);
						}
						else
						{
							MainMenu::beginFade(MainMenu::FadeDestination::TitleScreen);
						}
					}
				}
				else if ( introstage == 0 )
				{
					// DEPRECATED
				}
				else
				{
					if (strcmp(classtoquickstart, ""))
					{
						for ( c = 0; c <= CLASS_MONK; c++ )
						{
							if ( !strcmp(classtoquickstart, playerClassLangEntry(c, 0)) )
							{
								client_classes[0] = c;
								break;
							}
						}

						// generate unique game key
						prng_seed_time();
						uniqueGameKey = prng_get_uint();
						if ( !uniqueGameKey )
						{
							uniqueGameKey++;
						}
						loading = true;

						// hack to fix these things from breaking everything...
						for ( int i = 0; i < MAXPLAYERS; ++i )
						{
							players[i]->hud.arm = nullptr;
							players[i]->hud.weapon = nullptr;
							players[i]->hud.magicLeftHand = nullptr;
							players[i]->hud.magicRightHand = nullptr;
						}

						// reset class loadout
						stats[0]->sex = static_cast<sex_t>(rand() % 2);
						stats[0]->appearance = rand() % NUMAPPEARANCES;
						stats[0]->clearStats();
						initClass(0);
						if ( stats[0]->playerRace != RACE_HUMAN )
						{
							stats[0]->appearance = 0;
						}

						strcpy(stats[0]->name, "Avatar");
						multiplayer = SINGLE;
						fadefinished = false;
						fadeout = false;
						numplayers = 0;

						//TODO: Replace all of this with centralized startGameRoutine().
						// setup game
						for ( int i = 0; i < MAXPLAYERS; ++i )
						{
							players[i]->shootmode = true;
						}
						// make some messages
						startMessages();

						//gameplayCustomManager.writeAllToDocument();
						gameplayCustomManager.readFromFile();

						// load dungeon
						mapseed = rand(); //Use prng if decide to make a quickstart for MP...
						lastEntityUIDs = entity_uids;
						for ( node = map.entities->first; node != nullptr; node = node->next )
						{
							entity = (Entity*)node->element;
							entity->flags[NOUPDATE] = true;
						}
						if ( loadingmap == false )
						{
							currentlevel = startfloor;
							int checkMapHash = -1;
							if ( startfloor )
							{
								physfsLoadMapFile(currentlevel, 0, true, &checkMapHash);
								conductGameChallenges[CONDUCT_CHEATS_ENABLED] = 1;
							}
							else
							{
								physfsLoadMapFile(0, 0, true, &checkMapHash);
							}
							if ( checkMapHash == 0 )
							{
								conductGameChallenges[CONDUCT_MODDED] = 1;
							}
						}
						else
						{
							if ( genmap == false )
							{
								std::string fullMapName = physfsFormatMapName(maptoload);
								int checkMapHash = -1;
								loadMap(fullMapName.c_str(), &map, map.entities, map.creatures, &checkMapHash);
								if ( checkMapHash == 0 )
								{
									conductGameChallenges[CONDUCT_MODDED] = 1;
								}
							}
							else
							{
								generateDungeon(maptoload, rand());
							}
						}
						assignActions(&map);
						generatePathMaps();

						achievementObserver.updateData();

						saveGame();

						enchantedFeatherScrollSeed.seed(uniqueGameKey);
						enchantedFeatherScrollsShuffled.clear();
						enchantedFeatherScrollsShuffled = enchantedFeatherScrollsFixedList;
						std::shuffle(enchantedFeatherScrollsShuffled.begin(), enchantedFeatherScrollsShuffled.end(), enchantedFeatherScrollSeed);
						//for ( auto it = enchantedFeatherScrollsShuffled.begin(); it != enchantedFeatherScrollsShuffled.end(); ++it )
						//{
						//	printlog("Sequence: %d", *it);
						//}

						// kick off the main loop!
						strcpy(classtoquickstart, "");
						intro = false;
						loading = false;
					}
					else
					{
						// draws the menu level "backdrop"
						drawClearBuffers();
						if ( movie == false )
						{
							menucam.winx = 0;
							menucam.winy = 0;
							menucam.winw = xres;
							menucam.winh = yres;
							light = lightSphere(menucam.x, menucam.y, 16, 64);
							occlusionCulling(map, menucam);
							glDrawWorld(&menucam, REALCOLORS);
							//drawFloors(&menucam);
							drawEntities3D(&menucam, REALCOLORS);
							list_RemoveNode(light->node);
						}

						if (newui)
						{
							MainMenu::doMainMenu(!intro);
						}
						else
						{
							handleMainMenu(intro);
							UIToastNotificationManager.drawNotifications(movie, true); // draw this before the cursor
						}

						doFrames();

#ifndef NINTENDO
						// draw mouse
						if ( !movie )
						{
							// only draw 1 cursor in the main menu
							if ( inputs.getVirtualMouse(clientnum)->draw_cursor )
							{
								auto cursor = Image::get("images/system/cursor_hand.png");
								pos.x = inputs.getMouse(clientnum, Inputs::X) - cursor->getWidth() / 2;
								pos.y = inputs.getMouse(clientnum, Inputs::Y) - cursor->getHeight() / 2;
								pos.w = cursor->getWidth();
								pos.h = cursor->getHeight();
								cursor->draw(nullptr, pos, SDL_Rect{0, 0, xres, yres});

								if (MainMenu::cursor_delete_mode)
								{
								    auto icon = Image::get("images/system/Broken.png");
								    pos.x = pos.x + pos.w;
								    pos.y = pos.y + pos.h;
								    pos.w = icon->getWidth() * 2;
								    pos.h = icon->getHeight() * 2;
								    icon->draw(nullptr, pos, SDL_Rect{0, 0, xres, yres});
								}
							}
						}
#endif
					}
				}
			}
			else
			{
				if ( multiplayer == CLIENT )
				{
					// make sure shop inventory is alloc'd
					for ( int i = 0; i < MAXPLAYERS; ++i )
					{
						if ( !shopInv[i] )
						{
							shopInv[i] = (list_t*) malloc(sizeof(list_t));
							shopInv[i]->first = NULL;
							shopInv[i]->last = NULL;
						}
					}
				}
#ifdef MUSIC
				handleLevelMusic();
#endif
				DebugStats.t4Music = std::chrono::high_resolution_clock::now();

#ifdef NINTENDO
				// activate console
				if ((inputs.bControllerInputPressed(clientnum, INJOY_PAUSE_MENU)) &&
					(inputs.bControllerInputPressed(clientnum, INJOY_GAME_DEFEND)) &&
					(inputs.bControllerInputPressed(clientnum, INJOY_GAME_ATTACK)))
				{
					inputs.bControllerInputPressed(clientnum, INJOY_PAUSE_MENU);
					inputs.bControllerInputPressed(clientnum, INJOY_GAME_DEFEND);
					inputs.bControllerInputPressed(clientnum, INJOY_GAME_ATTACK);

					auto result = nxKeyboard("Enter console command");
					if (result.success)
					{
						char temp[128];
						strncpy(temp, result.str.c_str(), 128);
						temp[127] = '\0';
						messagePlayer(clientnum, MESSAGE_MISC, temp);
						consoleCommand(temp);
					}
				}
#endif

				// toggling the game menu
				bool doPause = false;
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( !players[i]->isLocalPlayer() )
					{
						continue;
					}
					if ( (Input::inputs[i].consumeBinaryToggle("Pause Game") 
							|| (inputs.bPlayerUsingKeyboardControl(i) && keystatus[SDL_SCANCODE_ESCAPE] && !Input::inputs[i].isDisabled()))
						&& !command )
					{
						keystatus[SDL_SCANCODE_ESCAPE] = 0;
						if ( !players[i]->shootmode )
						{
							players[i]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_CLOSE_ALL);
							players[i]->gui_mode = GUI_MODE_INVENTORY;
							players[i]->characterSheet.attributespage = 0;
						}
						else
						{
							doPause = true;
						}
						break;
					}
				}
				if ( doPause )
				{
					if ( !gamePaused )
					{
						pauseGame(0, MAXPLAYERS);
					}
					else 
					{
						if ( MainMenu::main_menu_frame )
						{
							int dimmers = 0;
							for ( auto frame : MainMenu::main_menu_frame->getFrames() )
							{
								if ( !strcmp(frame->getName(), "dimmer") )
								{
									++dimmers;
								}
							}
							if ( dimmers == 0 )
							{
								pauseGame(0, MAXPLAYERS);
							}
						}
						else
						{
							pauseGame(0, MAXPLAYERS);
						}
					}
				}

				// main drawing
				drawClearBuffers();

				if ( TimerExperiments::bUseTimerInterpolation )
				{
					if ( !(!gamePaused || (multiplayer && !client_disconnected[0])) )
					{
						// game paused, dont't process any velocities for camera
						for ( int c = 0; c < MAXPLAYERS; ++c )
						{
							TimerExperiments::cameraCurrentState[c].resetMovement();
						}
					}
					TimerExperiments::updateClocks();
					//std::string timerOutput = TimerExperiments::render(TimerExperiments::cameraRenderState[0].yaw);
					//cameras[0].x = players[0]->entity->x / 16.0;//TimerExperiments::cameraRenderState.x.position;
					//cameras[0].y = players[0]->entity->y / 16.0;//TimerExperiments::cameraRenderState.y.position;
					//cameras[0].z = TimerExperiments::cameraRenderState.z.position;
					//printTextFormatted(font8x8_bmp, 8, 20, "%s", timerOutput.c_str());
					for ( node_t* node = map.entities->first; node; node = node->next )
					{
						entity = (Entity*)node->element;
						if ( entity->bUseRenderInterpolation )
						{
							entity->lerp_ox = entity->x;
							entity->lerp_oy = entity->y;
							entity->x = entity->lerpRenderState.x.position * 16.0;
							entity->y = entity->lerpRenderState.y.position * 16.0;
						}
					}
				}


				for (int c = 0; c < MAXPLAYERS; ++c) 
				{
					auto& camera = cameras[c];
					auto& cvars = cameravars[c];
					TimerExperiments::renderCameras(camera, c);
					camera.ang += cvars.shakex2;
					camera.vang += cvars.shakey2 / 200.0;

					for ( auto& HPBar : enemyHPDamageBarHandler[c].HPBars )
					{
						HPBar.second.updateWorldCoordinates(); // update enemy bar world coordinates before drawEntities3D called
					}
				}

				if ( true )
				{
					// drunkenness spinning
					double cosspin = cos(ticks % 360 * PI / 180.f) * 0.25;
					double sinspin = sin(ticks % 360 * PI / 180.f) * 0.25;

					int playercount = 0;
					for (int c = 0; c < MAXPLAYERS; ++c) 
					{
						if (!client_disconnected[c]) 
						{
							++playercount;
						}
					}

					if (playercount >= 1)
					{
						//int maximum = splitscreen ? MAXPLAYERS : 1;
						for (int c = 0; c < MAXPLAYERS; ++c)
						{
							if (client_disconnected[c]) 
							{
								continue;
							}
							if ( !splitscreen && c != clientnum )
							{
								continue;
							}
							auto& camera = players[c]->camera();
							if (shaking && players[c] && players[c]->entity && !gamePaused)
							{
								camera.ang += cosspin * drunkextend;
								camera.vang += sinspin * drunkextend;
							}

							if ( players[c] && players[c]->entity )
							{
								if ( usecamerasmoothing )
								{
									real_t oldYaw = players[c]->entity->yaw;
									//printText(font8x8_bmp, 20, 20, "using smooth camera");
									players[c]->movement.handlePlayerCameraBobbing(true);
									players[c]->movement.handlePlayerMovement(true);
									players[c]->movement.handlePlayerCameraUpdate(true);
									players[c]->movement.handlePlayerCameraPosition(true);
									//messagePlayer(0, "%3.2f | %3.2f", players[c]->entity->yaw, oldYaw);
								}
							}

							// do occlusion culling from the perspective of this camera
							occlusionCulling(map, camera);

							if ( players[c] && players[c]->entity )
							{
								if ( players[c]->entity->isBlind() )
								{
									if ( globalLightModifierActive == GLOBAL_LIGHT_MODIFIER_STOPPED 
										|| (globalLightModifierActive == GLOBAL_LIGHT_MODIFIER_DISSIPATING && globalLightModifier < 1.f) )
									{
										globalLightModifierActive = GLOBAL_LIGHT_MODIFIER_INUSE;
										globalLightModifier = 0.f;
										globalLightTelepathyModifier = 0.f;
										if ( stats[c]->mask && stats[c]->mask->type == TOOL_BLINDFOLD_TELEPATHY )
										{
											for ( node_t* mapNode = map.creatures->first; mapNode != nullptr; mapNode = mapNode->next )
											{
												Entity* mapCreature = (Entity*)mapNode->element;
												if ( mapCreature )
												{
													mapCreature->monsterEntityRenderAsTelepath = 1;
												}
											}
										}
									}

									int PERModifier = 0;
									if ( stats[c] && stats[c]->EFFECTS[EFF_BLIND]
										&& !stats[c]->EFFECTS[EFF_ASLEEP] && !stats[c]->EFFECTS[EFF_MESSY] )
									{
										// blind but not messy or asleep = allow PER to let you see the world a little.
										PERModifier = players[c]->entity->getPER() / 5;
										if ( PERModifier < 0 )
										{
											PERModifier = 0;
										}
									}

									real_t limit = PERModifier * 0.01;
									globalLightModifier = std::min(limit, globalLightModifier + 0.0005);

									int telepathyLimit = std::min(64, 48 + players[c]->entity->getPER());
									globalLightTelepathyModifier = std::min(telepathyLimit / 255.0, globalLightTelepathyModifier + (0.2 / 255.0));
								}
								else
								{
									if ( globalLightModifierActive == GLOBAL_LIGHT_MODIFIER_INUSE )
									{
										for ( node_t* mapNode = map.creatures->first; mapNode != nullptr; mapNode = mapNode->next )
										{
											Entity* mapCreature = (Entity*)mapNode->element;
											if ( mapCreature )
											{
												mapCreature->monsterEntityRenderAsTelepath = 0;
											}
										}
									}
									globalLightModifierActive = GLOBAL_LIGHT_MODIFIER_DISSIPATING;
									globalLightTelepathyModifier = 0.f;
									if ( globalLightModifier < 1.f )
									{
										globalLightModifier += 0.01;
									}
									else
									{
										globalLightModifier = 1.01;
										globalLightModifierActive = GLOBAL_LIGHT_MODIFIER_STOPPED;
									}
								}
								raycast(&camera, minimap); // update minimap
								glDrawWorld(&camera, REALCOLORS);

								if ( gameplayCustomManager.inUse() && gameplayCustomManager.minimapShareProgress && !splitscreen )
								{
									for ( int i = 0; i < MAXPLAYERS; ++i )
									{
										if ( i != clientnum && players[i] && players[i]->entity )
										{
											real_t x = camera.x;
											real_t y = camera.y;
											real_t ang = camera.ang;

											camera.x = players[i]->entity->x / 16.0;
											camera.y = players[i]->entity->y / 16.0;
											camera.ang = players[i]->entity->yaw;
											raycast(&camera, minimap);
											camera.x = x;
											camera.y = y;
											camera.ang = ang;
										}
									}
								}
							}
							else
							{
							    // player is dead, spectate
								glDrawWorld(&camera, REALCOLORS);
							}

							//drawFloors(&camera);
							drawEntities3D(&camera, REALCOLORS);

							if (shaking && players[c] && players[c]->entity && !gamePaused)
							{
								camera.ang -= cosspin * drunkextend;
								camera.vang -= sinspin * drunkextend;
							}

							auto& cvars = cameravars[c];
							camera.ang -= cvars.shakex2;
							camera.vang -= cvars.shakey2 / 200.0;
						}
					}
				}

				if ( TimerExperiments::bUseTimerInterpolation )
				{
					for ( node_t* node = map.entities->first; node; node = node->next )
					{
						entity = (Entity*)node->element;
						if ( entity->bUseRenderInterpolation )
						{
							entity->x = entity->lerp_ox;
							entity->y = entity->lerp_oy;
						}
					}
				}

				DebugStats.t5MainDraw = std::chrono::high_resolution_clock::now();

				for ( int player = 0; player < MAXPLAYERS; ++player )
				{
					if ( players[player]->isLocalPlayer() )
					{
						players[player]->messageZone.updateMessages();
					}
				}
				if ( !nohud )
				{
					for ( int player = 0; player < MAXPLAYERS; ++player )
					{
						if ( players[player]->isLocalPlayer() )
						{
							handleDamageIndicators(player);
							players[player]->messageZone.drawMessages();
						}
					}
				}

				DebugStats.t6Messages = std::chrono::high_resolution_clock::now();

				doFrames();

				ingameHud();

				if ( gamePaused )
				{
					// handle menu
					if (newui)
					{
						MainMenu::doMainMenu(!intro);
					}
					else
					{
						handleMainMenu(intro);
					}
				}
				else
				{
					MainMenu::destroyMainMenu();

					// draw subwindow
					if ( !movie )
					{
						if ( subwindow )
						{
							drawWindowFancy(subx1, suby1, subx2, suby2);
							if ( subtext && subtext[0] != '\0')
							{
								if ( strncmp(subtext, language[1133], 12) )
								{
									ttfPrintTextFormatted(ttf12, subx1 + 8, suby1 + 8, subtext);
								}
								else
								{
									ttfPrintTextFormatted(ttf16, subx1 + 8, suby1 + 8, subtext);
								}
							}
						}

						// process button actions
						handleButtons();
					}
				}

				if ( gamePaused ) // draw after main menu windows etc.
				{
					UIToastNotificationManager.drawNotifications(movie, true); // draw this before the cursor
				}

				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( !players[i]->isLocalPlayer() && !gamePaused )
					{
						continue;
					}

					if ( gamePaused )
					{
						if ( inputs.bPlayerUsingKeyboardControl(i) )
						{
							auto cursor = Image::get("images/system/cursor_hand.png");
							pos.x = inputs.getMouse(i, Inputs::X) - cursor->getWidth() / 2;
							pos.y = inputs.getMouse(i, Inputs::Y) - cursor->getHeight() / 2;
							pos.w = cursor->getWidth();
							pos.h = cursor->getHeight();
							cursor->draw(nullptr, pos, SDL_Rect{ 0, 0, xres, yres });
						}
						continue;
					}

					if ((subwindow && !players[i]->shootmode))
					{
						if (inputs.getVirtualMouse(i)->draw_cursor)
						{
							auto cursor = Image::get("images/system/cursor_hand.png");
							pos.x = inputs.getMouse(i, Inputs::X) - cursor->getWidth() / 2;
							pos.y = inputs.getMouse(i, Inputs::Y) - cursor->getHeight() / 2;
							pos.w = cursor->getWidth();
							pos.h = cursor->getHeight();
							cursor->draw(nullptr, pos, SDL_Rect{0, 0, xres, yres});
						}
					}

					// to make sure scroll wheel gets cleared, as it never un-sets itself
					Input::inputs[i].consumeBinaryToggle("Hotbar Scroll Left"); 
					Input::inputs[i].consumeBinaryToggle("Hotbar Scroll Right");
					Input::inputs[i].consumeBinaryToggle("MenuMouseWheelUpAlt");
					Input::inputs[i].consumeBinaryToggle("MenuMouseWheelDownAlt");
				}
			}

			// fade in/out effect
			if ( fadealpha > 0 )
			{
				drawRect(NULL, makeColor(0, 0, 0, 255), fadealpha);
			}

			// fps counter
			if ( showfps )
			{
			    printTextFormatted(font16x16_bmp, 8, 8, "fps = %3.1f", fps);
			}
			if ( enableDebugKeys )
			{
				printTextFormatted(font8x8_bmp, 8, 20, "gui mode: %d", players[0]->GUI.activeModule);
			}

			DebugStats.t10FrameLimiter = std::chrono::high_resolution_clock::now();
			if ( logCheckMainLoopTimers )
			{
				std::chrono::duration<double> time_span = 
					std::chrono::duration_cast<std::chrono::duration<double>>(DebugStats.t10FrameLimiter - DebugStats.t11End);
				double timer = time_span.count() * 1000;
				if ( timer > ((1000.f / (fps) * 1.4)) )
				{
					DebugStats.displayStats = true;
					DebugStats.storeStats();
					DebugStats.storeEventStats();
					messagePlayer(clientnum, MESSAGE_MISC, "Timers: %f total.", timer);
				}
				if ( DebugStats.displayStats )
				{
					printTextFormatted(font8x8_bmp, 8, 200 + 20, DebugStats.debugOutput);
					printTextFormatted(font8x8_bmp, 8, 200 + 100, DebugStats.debugEventOutput);
				}
			}

			DebugTimers.printAllTimepoints();
			DebugTimers.clearAllTimepoints();

			//printTextFormatted(font8x8_bmp, 8, 32, "findFrame() calls: %d / loop", Frame::numFindFrameCalls);

			UIToastNotificationManager.drawNotifications(movie, false);

			// update screen
			GO_SwapBuffers(screen);

			// screenshots
			if ( Input::inputs[clientnum].consumeBinaryToggle("Screenshot") )
			{
				takeScreenshot();
			}

			// frame rate limiter
			while ( frameRateLimit(fpsLimit, true) )
			{
				if ( !intro )
				{
					// handle network messages
					if ( multiplayer == CLIENT )
					{
						clientHandleMessages(fpsLimit);
					}
					else if ( multiplayer == SERVER )
					{
						serverHandleMessages(fpsLimit);
					}
				}
			}

			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
			    Input& input = Input::inputs[i];

				// selectedEntityGimpTimer will only allow the game to process a right click entity click 1-2 times
				// otherwise if we interacted with a menu the gimp timer does not increment. (it would have auto reset the status of IN_USE)
				if ( !input.binaryToggle("Use") )
				{
					players[i]->movement.selectedEntityGimpTimer = 0;
				}
				else
				{
					if ( players[i]->movement.selectedEntityGimpTimer >= 2 )
					{
					    input.consumeBinaryToggle("Use");
					}
				}

				players[i]->GUI.clearHoveringOverModuleButton();
			}

			DebugStats.t11End = std::chrono::high_resolution_clock::now();

			// increase the cycle count
			cycles++;
		}
		if ( !load_successful ) {
			skipintro = true;
		}
		saveConfig("default.cfg");
		MainMenu::settingsMount();
		(void)MainMenu::settingsSave();

		// deinit
		deinitGame();
		return deinitApp();
	}
#ifndef NINTENDO
	catch (const std::exception &exc)
	{
		// catch anything thrown within try block that derives from std::exception
		std::cerr << "UNHANDLED EXCEPTION CAUGHT: " << exc.what() << "\n";
		return 1;
	}
	catch (...)
	{
		//TODO:
		std::cerr << "UNKNOWN EXCEPTION CAUGHT!\n";
		return 1;
	}
#endif // NINTENDO

#ifdef NINTENDO
	nxTerm();
#endif // NINTENDO
}

void DebugStatsClass::storeStats()
{
	if ( !displayStats )
	{
		return;
	}
	storeOldTimePoints();
	double out1 = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t2Stored - t21Stored).count();
	double out2 = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t3Stored - t2Stored).count();
	double out3 = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t5Stored - t4Stored).count();
	double out4 = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t6Stored - t5Stored).count();
	double out5 = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t7Stored - t6Messages).count();
	double out6 = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t8Stored - t7Stored).count();
	double out7 = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t9Stored - t8Stored).count();
	double out8 = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t10Stored - t9Stored).count();
	double out9 = -1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t11Stored - t10Stored).count();
	double out10 = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t21Stored - t1Stored).count();
	snprintf(debugOutput, 1023,
		"Messages: %4.5fms\nEvents: %4.5fms\nSteamCallbacks: %4.5fms\nMainDraw: %4.5fms\nMessages: %4.5fms\nInputs: %4.5fms\nStatus: %4.5fms\nGUI: %4.5fms\nFrameLimiter: %4.5fms\nEnd: %4.5fms\n",
		out10, out1, out2, out3, out4, out5, out6, out7, out8, out9);
}

void DebugStatsClass::storeEventStats()
{
	double out1 = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(eventsT2stored - eventsT1stored).count();
	double out2 = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(eventsT3stored - eventsT2stored).count();
	double out3 = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(eventsT4stored - eventsT3stored).count();
	double out4 = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(eventsT5stored - eventsT4stored).count();
	double out5 = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(eventsT6stored - eventsT5stored).count();

	double messages1 = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(messagesT1stored - t1StartLoop).count();
	double messages2 = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t21Stored - messagesT1stored).count();

	snprintf(debugEventOutput, 1023,
		"Events1: %4.5fms\nEvents2: %4.5fms\nEvents3: %4.5fms\nEvents4: %4.5fms\nEvents5: %4.5fms\nMessagesT1: %4.5fms\nMessagesT2: %4.5fms\n",
		out1, out2, out3, out4, out5, messages1, messages2);
}
