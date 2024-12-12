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
#include "interface/consolecommand.hpp"
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
#ifdef USE_PLAYFAB
#include "playfab.hpp"
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
#include "ui/LoadingScreen.hpp"

#include "UnicodeDecoder.h"

#include <atomic>
#include <future>
#include <thread>

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

static void segfault_sigaction(int signal, siginfo_t* si, void* arg)
{
	SDL_SetRelativeMouseMode(SDL_FALSE); //Uncapture mouse.

	printf("Caught segfault at address %p\n", si->si_addr);
	printf("Signal %d (dumping stack):", signal);

	//Dump the stack.

    constexpr unsigned int STACK_SIZE = 32;
	void* array[STACK_SIZE];
	size_t size = backtrace(array, STACK_SIZE);
	backtrace_symbols_fd(array, size, STDERR_FILENO);

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

ConsoleVariable<bool> cvar_enableKeepAlives("/keepalive_enabled", true);
ConsoleVariable<bool> cvar_animate_tiles("/animate_tiles", true);
ConsoleVariable<bool> cvar_map_sequence_rng("/map_sequence_rng", true);

std::vector<std::string> randomPlayerNamesMale;
std::vector<std::string> randomPlayerNamesFemale;
std::vector<std::string> randomNPCNamesMale;
std::vector<std::string> randomNPCNamesFemale;
std::vector<std::string> physFSFilesInDirectory;
TileEntityListHandler TileEntityList;
// recommended for valgrind debugging:
// res of 480x270
// /nohud
// undefine SOUND, MUSIC (see sound.h)
int game = 1;
Uint32 uniqueGameKey = 0;
Uint32 uniqueLobbyKey = 0;
DebugStatsClass DebugStats;
Uint32 networkTickrate = 0;
bool gameloopFreezeEntities = false;
Uint32 serverSchedulePlayerHealthUpdate = 0;
Uint32 serverLastPlayerHealthUpdate = 0;
Frame* cursorFrame = nullptr;
bool arachnophobia_filter = false;
bool colorblind_lobby = false; // if true, colorblind settings enforced by lobby for shared assets (player colors)

Frame::result_t framesProcResult{
    false,
    0,
    nullptr,
    false
};

#ifdef NDEBUG
Uint32 messagesEnabled = 0xffffffff & ~MESSAGE_DEBUG; // all but debug enabled
#else
Uint32 messagesEnabled = 0xffffffff; // all enabled
#endif

real_t getFPSScale(real_t baseFPS)
{
#ifndef EDITOR
	static ConsoleVariable<bool> cvar_ui_fps_scale_fixed("/ui_fps_scale_fixed", false);
	if ( *cvar_ui_fps_scale_fixed )
	{
		return baseFPS / (std::max(1U, fpsLimit));
	}
	else
	{
		return baseFPS / (std::max(1U, (unsigned int)fps));
	}
#else
	return baseFPS / (std::max(1U, (unsigned int)fps));
#endif
}

//ConsoleVariable<bool> cvar_useTimerInterpolation("/timer_interpolation_enabled", true);
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
void preciseSleep(double seconds);

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
		|| entity->behavior == &actCircuit
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
		// wait for entity to position itself in the world by setting useful x/y vlues (monster limbs etc)
		if ( abs(entity->x) > 0.01 || abs(entity->y) > 0.01 ) 
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
		}
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
				diff = fmod(diff, 2 * PI);
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
			if ( bDebug && keystatus[SDLK_i] )
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
    real_t range = std::max(fabs(min), fabs(max));
    position = fmod(position, range);
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

	static ConsoleVariable<bool> cvar_frameLagCheck("/framelagcheck", false);
	if ( *cvar_frameLagCheck )
	{
		static Uint32 doneTick = 0;
		if ( doneTick != ticks && ticks % (2 * TICKS_PER_SECOND) == 0 )
		{
			doneTick = ticks;
			auto microseconds = std::chrono::microseconds((Uint64)(300000));
			preciseSleep(microseconds.count() / 1e6);
		}
	}

	static ConsoleVariable<int> cvar_frameTime("/frametimelimit", 32);
	static ConsoleVariable<bool> cvar_frameTimeAutoSet("/frametimeautolimit", true);
	int frameTimeLimit = (*cvar_frameTime);
	if ( *cvar_frameTimeAutoSet )
	{
		real_t decimal = 0.0;
		real_t ms = 1000 / fpsLimit;
		if ( fps > 0.0 )
		{
			ms = 1000 / fps;
		}
		frameTimeLimit = ms;
		if ( modf(ms, &decimal) > 0.01 )
		{
			frameTimeLimit += 1;
		}
	}

	static ConsoleVariable<float> cvar_cameraLerpFactor("/cameralerp", 30.0);
	lerpFactor = (*cvar_cameraLerpFactor);

	static ConsoleVariable<bool> cvar_lerpAutoAdjust("/autocameralerp", true);

	time_point newTime = Clock::now();
	auto frameTime = newTime - currentTime;
	if ( frameTime > std::chrono::milliseconds{ frameTimeLimit } )
	{
		frameTime = std::chrono::milliseconds{ frameTimeLimit };
	}

	currentTime = newTime;
	accumulator += frameTime;

	if ( (*cvar_lerpAutoAdjust) )
	{
		int frameTimeMillis = std::chrono::duration_cast<Clock::duration>(frameTime).count();
		real_t approxFPS = 1000.0 / frameTimeMillis;
		lerpFactor = 30.0;
		if ( approxFPS < 32.0 )
		{
			lerpFactor = std::max(1, (int)approxFPS - 2);
		}
	}

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
		cameraRenderState[i] = cameraCurrentState[i] * alpha + cameraPreviousState[i] * (1.0 - alpha);
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
		entity->lerpRenderState = entity->lerpCurrentState * alpha + entity->lerpPreviousState * (1.0 - alpha);
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

enum DemoMode {
    STOPPED,
    RECORDING,
    PLAYING
};
static DemoMode demo_mode = DemoMode::STOPPED;
static File* demo_file = nullptr;

static void demo_stop() {
    if (demo_mode == DemoMode::STOPPED) {
        messagePlayer(clientnum, MESSAGE_MISC, "Demo is not running");
        return;
    }
    switch (demo_mode) {
    case DemoMode::PLAYING:
        if (demo_file->eof()) {
            messagePlayer(clientnum, MESSAGE_MISC, "End of demo");
        } else {
            messagePlayer(clientnum, MESSAGE_MISC, "Stopped demo playback");
        }
        break;
    case DemoMode::RECORDING:
        messagePlayer(clientnum, MESSAGE_MISC, "Stopped demo recording");
        break;
    default:
        messagePlayer(clientnum, MESSAGE_MISC, "Stopped demo");
        break;
    }
    if (demo_file) {
        FileIO::close(demo_file);
        demo_file = nullptr;
    }
    demo_mode = DemoMode::STOPPED;

    TimerExperiments::bUseTimerInterpolation = true;
}

static void demo_record(const char* filename) {
    if (demo_mode != DemoMode::STOPPED) {
        messagePlayer(clientnum, MESSAGE_MISC, "Demo must be stopped first (/demo_stop)");
        return;
    }

    if (multiplayer != SINGLE || splitscreen) {
        messagePlayer(clientnum, MESSAGE_MISC, "Demos not permitted in multiplayer");
        return;
    }

    // create demo file for recording
    char path[PATH_MAX];
    completePath(path, filename, outputdir);
    demo_file = FileIO::open(path, "wb");
    if (!demo_file) {
        messagePlayer(clientnum, MESSAGE_MISC, "failed to open demo file '%s'", path);
        return;
    }

    TimerExperiments::bUseTimerInterpolation = false; // this causes mass desyncs

    // seed RNG
    local_rng.seedTime();
    local_rng.getSeed(&uniqueGameKey, sizeof(uniqueGameKey));
    net_rng.seedBytes(&uniqueGameKey, sizeof(uniqueGameKey));
    demo_file->write(&uniqueGameKey, sizeof(uniqueGameKey), 1);

    // write player stats
    demo_file->write(&stats[clientnum]->playerRace, sizeof(Stat::playerRace), 1);
    demo_file->write(&stats[clientnum]->sex, sizeof(Stat::sex), 1);
    demo_file->write(&stats[clientnum]->stat_appearance, sizeof(Stat::stat_appearance), 1);
    demo_file->write(&client_classes[clientnum], sizeof(client_classes[clientnum]), 1);

    // write player name
    Uint32 name_len = (Uint32)strlen(stats[clientnum]->name);
    demo_file->write(&name_len, sizeof(name_len), 1);
    demo_file->write(stats[clientnum]->name, sizeof(char), name_len);

    // reset player
    stats[clientnum]->clearStats();
	initClass(clientnum);

    // start game
    doNewGame(false);

    messagePlayer(clientnum, MESSAGE_MISC, "Recording demo to '%s'", path);
    demo_mode = DemoMode::RECORDING;
}

static void demo_play(const char* filename) {
    if (demo_mode != DemoMode::STOPPED) {
        messagePlayer(clientnum, MESSAGE_MISC, "Demo must be stopped first (/demo_stop)");
        return;
    }

    if (multiplayer != SINGLE || splitscreen) {
        messagePlayer(clientnum, MESSAGE_MISC, "Demos not permitted in multiplayer");
        return;
    }

    // open demo file for playing
    char path[PATH_MAX];
    completePath(path, filename, outputdir);
    demo_file = FileIO::open(path, "rb");
    if (!demo_file) {
        messagePlayer(clientnum, MESSAGE_MISC, "failed to open demo file '%s'", path);
        return;
    }

    TimerExperiments::bUseTimerInterpolation = false; // this causes mass desyncs

    // seed RNG
    demo_file->read(&uniqueGameKey, sizeof(uniqueGameKey), 1);
    local_rng.seedBytes(&uniqueGameKey, sizeof(uniqueGameKey));
    net_rng.seedBytes(&uniqueGameKey, sizeof(uniqueGameKey));

    // read player stats
    demo_file->read(&stats[clientnum]->playerRace, sizeof(Stat::playerRace), 1);
    demo_file->read(&stats[clientnum]->sex, sizeof(Stat::sex), 1);
    demo_file->read(&stats[clientnum]->stat_appearance, sizeof(Stat::stat_appearance), 1);
    demo_file->read(&client_classes[clientnum], sizeof(client_classes[clientnum]), 1);

    // read player name
    Uint32 name_len;
    demo_file->read(&name_len, sizeof(name_len), 1);
    demo_file->read(stats[clientnum]->name, sizeof(char), name_len);
    stats[clientnum]->name[name_len] = '\0';

    // reset player
    stats[clientnum]->clearStats();
	initClass(clientnum);

    // start game
    doNewGame(false);

    messagePlayer(clientnum, MESSAGE_MISC, "Playing demo in '%s'", path);
    demo_mode = DemoMode::PLAYING;
}

static ConsoleCommand ccmd_demo_stop("/demo_stop", "stop recording or playing a demo",
    [](int argc, const char* argv[]){
    demo_stop();
    });

static ConsoleCommand ccmd_demo_record("/demo_record", "record a demo to a file (default demo.dat)",
    [](int argc, const char* argv[]){
    if (argc < 2) {
	    demo_record("demo.dat");
    } else {
	    demo_record(argv[1]);
    }
    });

static ConsoleCommand ccmd_demo_play("/demo_play", "play a recorded demo(default demo.dat)",
    [](int argc, const char* argv[]){
    if (argc < 2) {
	    demo_play("demo.dat");
    } else {
	    demo_play(argv[1]);
    }
    });

/*-------------------------------------------------------------------------------

	gameLogic

	Updates the gamestate; moves actors, primarily

-------------------------------------------------------------------------------*/

ConsoleVariable<bool> framesEatMouse("/gui_eat_mouseclicks", true);
static ConsoleVariable<bool> cvar_lava_use_vismap("/lava_use_vismap", true);
#ifdef NINTENDO
static ConsoleVariable<bool> cvar_lava_bubbles_enabled("/lava_bubbles_enabled", false);
#else
static ConsoleVariable<bool> cvar_lava_bubbles_enabled("/lava_bubbles_enabled", true);
#endif

static real_t drunkextend[MAXPLAYERS] = { (real_t)0.0 };

void gameLogic(void)
{
	Uint32 x;
	node_t* node, *nextnode, *node2;
	Entity* entity;
	int c = 0;
	Uint32 i = 0, j;
	bool entitydeletedself;

#ifdef NINTENDO
	(void)nxUpdateCrashMessage();
#endif

    if (!gamePaused && !loading) {
        if (demo_file) {
            // demo recording
            if (demo_mode == DemoMode::RECORDING) {
                demo_file->write(&Input::inputs[clientnum].keys, sizeof(Input::keys), 1);
                demo_file->write(&Input::inputs[clientnum].mouseButtons, sizeof(Input::mouseButtons), 1);
                demo_file->write(&keystatus, sizeof(keystatus), 1);
                demo_file->write(&mousex, sizeof(mousex), 1);
                demo_file->write(&mousey, sizeof(mousey), 1);
                demo_file->write(&mousestatus, sizeof(mousestatus), 1);
                demo_file->write(&mousexrel, sizeof(mousexrel), 1);
                demo_file->write(&mouseyrel, sizeof(mouseyrel), 1);
            }

            // demo playback
            if (demo_mode == DemoMode::PLAYING) {
                demo_file->read(&Input::inputs[clientnum].keys, sizeof(Input::keys), 1);
                demo_file->read(&Input::inputs[clientnum].mouseButtons, sizeof(Input::mouseButtons), 1);
                demo_file->read(&keystatus, sizeof(keystatus), 1);
                demo_file->read(&mousex, sizeof(mousex), 1);
                demo_file->read(&mousey, sizeof(mousey), 1);
                demo_file->read(&mousestatus, sizeof(mousestatus), 1);
                demo_file->read(&mousexrel, sizeof(mousexrel), 1);
                demo_file->read(&mouseyrel, sizeof(mouseyrel), 1);
                if (demo_file->eof()) {
                    demo_stop();
                }
            }
        }
    }

	for (auto& input : Input::inputs) {
		input.update();
		input.consumeBindingsSharedWithFaceHotbar();
	}

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
			static ConsoleVariable<int> cvar_shake_max("/shake_max", 15);
			camera_shakex = std::min(camera_shakex, *cvar_shake_max / 100.0);
			camera_shakey = std::min(camera_shakey, *cvar_shake_max);

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
	    for (int c = 0; c < MAXPLAYERS; ++c)
	    {
			players[c]->hud.followerDisplay.bCommandNPCDisabled = false;
			players[c]->hud.followerDisplay.bOpenFollowerMenuDisabled = false;
			players[c]->hud.bOpenCalloutsMenuDisabled = false;

	        if (c != clientnum && !splitscreen)
	        {
	            continue;
	        }
	        if ( stats[c]->EFFECTS[EFF_DRUNK] )
		    {
			    // goat/drunkards no spin!
			    if ( stats[c]->type == GOATMAN )
			    {
				    // return to normal.
				    if ( drunkextend[c] > 0 )
				    {
					    drunkextend[c] -= .005;
					    if ( drunkextend[c] < 0 )
					    {
						    drunkextend[c] = 0;
					    }
				    }
			    }
			    else
			    {
				    if ( drunkextend[c] < 0.5 )
				    {
					    drunkextend[c] += .005;
					    if ( drunkextend[c] > 0.5 )
					    {
						    drunkextend[c] = 0.5;
					    }
				    }
			    }
		    }
		    else
		    {
			    if ( stats[c]->EFFECTS[EFF_WITHDRAWAL] || stats[c]->EFFECTS[EFF_DISORIENTED] )
			    {
				    // special widthdrawal shakes
				    if ( drunkextend[c] < 0.2 )
				    {
					    drunkextend[c] += .005;
					    if ( drunkextend[c] > 0.2 )
					    {
						    drunkextend[c] = 0.2;
					    }
				    }
			    }
			    else
			    {
				    // return to normal.
				    if ( drunkextend[c] > 0 )
				    {
					    drunkextend[c] -= .005;
					    if ( drunkextend[c] < 0 )
					    {
						    drunkextend[c] = 0;
					    }
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
				    j = 1 + local_rng.rand() % 4;
				    for ( c = 0; c < j; ++c )
				    {
						if ( Entity* flame = spawnFlame(entity, SPRITE_FLAME) )
						{
							flame->x += local_rng.rand() % (entity->sizex * 2 + 1) - entity->sizex;
							flame->y += local_rng.rand() % (entity->sizey * 2 + 1) - entity->sizey;
							flame->z += local_rng.rand() % 5 - 2;
							if ( entity->behavior == &actBell )
							{
								flame->x += entity->focalx * cos(entity->yaw) + entity->focaly * cos(entity->yaw + PI / 2);
								flame->y += entity->focalx * sin(entity->yaw) + entity->focaly * sin(entity->yaw + PI / 2);
							}
						}
				    }
				}
			}
		}
	}

	// damage indicator timers
	handleDamageIndicatorTicks();

	if ( intro == true )
	{
        if (gearsize == 0) {
            // initialize
            gearsize = 40000 * (xres / 1280.f);
        }
        
		// rotate gear
		gearrot += 1;
		if ( gearrot >= 360 )
		{
			gearrot -= 360;
		}
		gearsize -= std::max<double>(2, gearsize / 20);
        const float smallest_size = 70 * (xres / 1280.f);
		if ( gearsize < smallest_size )
		{
			gearsize = smallest_size;
			logoalpha += 2;
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

						deleteent_t* deleteent = nullptr;
						if (net_packet && net_packet->data) {
							// send the delete entity command to the client
							strcpy((char*)net_packet->data, "ENTD");
							deleteent = (deleteent_t*)node->element;
							SDLNet_Write32(deleteent->uid, &net_packet->data[4]);
							net_packet->address.host = net_clients[i - 1].host;
							net_packet->address.port = net_clients[i - 1].port;
							net_packet->len = 8;
							sendPacket(net_sock, -1, net_packet, i - 1);

							// quit reminding clients after a certain number of attempts]
							if (deleteent) {
								deleteent->tries++;
								if (deleteent->tries >= MAXTRIES)
								{
									list_RemoveNode(node);
								}
							}
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
								if ( z == 0 )
								{
									// water and lava noises
									if ( ticks % (TICKS_PER_SECOND * 4) == (y + x * map.height) % (TICKS_PER_SECOND * 4) && local_rng.rand() % 3 == 0 )
									{
										int coord = x + y * 1000;
										if ( map.liquidSfxPlayedTiles.find(coord) == map.liquidSfxPlayedTiles.end() )
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
											map.liquidSfxPlayedTiles.insert(coord);
										}
									}

									// lava bubbles
									if ( lavatiles[map.tiles[index]] && !gameloopFreezeEntities )
									{
										if ( ticks % 40 == (y + x * map.height) % 40 && local_rng.rand() % 3 == 0 )
										{
											bool doLavaParticles = *cvar_lava_bubbles_enabled;
											if ( doLavaParticles )
											{
												if ( *cvar_lava_use_vismap && !intro )
												{
													if ( x >= 0 && x < map.width && y >= 0 && y < map.height )
													{
														bool anyVismap = false;
														for ( int i = 0; i < MAXPLAYERS; ++i )
														{
															if ( !client_disconnected[i] && players[i]->isLocalPlayer() && cameras[i].vismap[y + x * map.height] )
															{
																anyVismap = true;
																break;
															}
														}
														if ( !anyVismap )
														{
															doLavaParticles = false;
														}
													}
												}
												int c, j = 1 + local_rng.rand() % 2;
												for ( c = 0; c < j && doLavaParticles; ++c )
												{
													Entity* entity = newEntity(42, 1, map.entities, nullptr); //Gib entity.
													entity->behavior = &actGib;
													entity->x = x * 16 + local_rng.rand() % 16;
													entity->y = y * 16 + local_rng.rand() % 16;
													entity->z = 7.5;
                                                    entity->ditheringDisabled = true;
													entity->flags[PASSABLE] = true;
													entity->flags[SPRITE] = true;
													entity->flags[NOUPDATE] = true;
													entity->flags[UPDATENEEDED] = false;
													entity->flags[UNCLICKABLE] = true;
													entity->sizex = 2;
													entity->sizey = 2;
													entity->fskill[3] = 0.01;
													double vel = (local_rng.rand() % 10) / 20.f;
													entity->vel_x = vel * cos(entity->yaw);
													entity->vel_y = vel * sin(entity->yaw);
													entity->vel_z = -.15 - (local_rng.rand() % 15) / 100.f;
													entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
													entity->pitch = (local_rng.rand() % 360) * PI / 180.0;
													entity->roll = (local_rng.rand() % 360) * PI / 180.0;
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

					if ( stats[c]->helmet && stats[c]->helmet->type == HAT_WOLF_HOOD
						&& stats[c]->helmet->beatitude > 0 )
					{
						steamAchievementClient(c, "BARONY_ACH_PET_DA_DOG");
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
						&& ((stats[c]->mask && stats[c]->mask->type == MASK_EYEPATCH) || !stats[c]->mask)
						&& !stats[c]->helmet
						&& !stats[c]->gloves && !stats[c]->shoes
						&& !stats[c]->breastplate && !stats[c]->ring
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

					/*if ( achievementStatusRhythmOfTheKnight[c] )
					{
						steamAchievementClient(c, "BARONY_ACH_RHYTHM_OF_THE_KNIGHT");
					}*/
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
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				gameplayPreferences[i].process();
			}
			updatePlayerConductsInMainLoop();
			Compendium_t::Events_t::updateEventsInMainLoop(clientnum);
			achievementObserver.updatePlayerAchievement(clientnum, AchievementObserver::BARONY_ACH_DAPPER, AchievementObserver::DAPPER_EQUIPMENT_CHECK);

			//if( TICKS_PER_SECOND )
			//generatePathMaps();
			bool debugMonsterTimer = false && !gamePaused && keystatus[SDLK_g];
			if ( debugMonsterTimer )
			{
				printlog("loop start");
			}
			real_t accum = 0.0;
			std::map<int, real_t> entityAccum;
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
							&& entity->behavior != &actDeathGhost
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
							if ( debugMonsterTimer )
							{
								auto t2 = std::chrono::high_resolution_clock::now();
								//printlog("%d: %d %f", entity->sprite, entity->monsterState,
								//	1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t).count());
								accum += 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t).count();
								entityAccum[entity->sprite] += 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t).count();
								/*if ( entity->sprite == 1426 || entity->sprite == 1430 )
								{
									if ( 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t).count() > 1.0 )
									{
										printlog("gnome time: uid %d | time %.2f", entity->getUID(), 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t).count());
									}
								}*/
							}

						}
					}
				}

				if ( loadnextlevel == true )
				{
				    // when this flag is set, it's time to load the next level.
					loadnextlevel = false;

					int totalFloorGold = 0;
					int totalFloorItems = 0;
					int totalFloorItemValue[MAXPLAYERS];
					int totalFloorMonsters = 0;
					int totalFloorEnemies[MAXPLAYERS];
					Item tmpItem;
					for ( int i = 0; i < MAXPLAYERS; ++i )
					{
						totalFloorItemValue[i] = 0;
						totalFloorEnemies[i] = 0;
					}

					for ( node = map.entities->first; node != nullptr; node = node->next )
					{
						entity = (Entity*)node->element;
						entity->flags[NOUPDATE] = true;
						if ( entity->behavior == &actGoldBag )
						{
							totalFloorGold += entity->goldAmount;
						}
						else if ( entity->behavior == &actItem )
						{
							totalFloorItems++;
							tmpItem.type = (entity->skill[10] >= 0 && entity->skill[10] < NUMITEMS) ? (ItemType)entity->skill[10] : ItemType::GEM_ROCK;
							tmpItem.status = (int)entity->skill[11] < Status::BROKEN ?
								Status::BROKEN : ((int)entity->skill[11] > EXCELLENT ? EXCELLENT : (Status)entity->skill[11]);
							tmpItem.beatitude = std::min(std::max((Sint16)-100, (Sint16)entity->skill[12]), (Sint16)100);
							tmpItem.count = std::max((Sint16)entity->skill[13], (Sint16)1);
							tmpItem.appearance = entity->skill[14];
							tmpItem.identified = entity->skill[15];

							for ( int i = 0; i < MAXPLAYERS; ++i )
							{
								if ( !client_disconnected[i] )
								{
									totalFloorItemValue[i] += tmpItem.sellValue(i);
								}
							}
						}
						else if ( entity->behavior == &actMonster )
						{
							for ( int i = 0; i < MAXPLAYERS; ++i )
							{
								if ( !client_disconnected[i] )
								{
									if ( players[i]->entity )
									{
										if ( !entity->checkFriend(players[i]->entity) )
										{
											totalFloorEnemies[i]++;
										}
									}
								}
							}
							totalFloorMonsters++;
						}
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
								Uint32 color = makeColorRGB(0, 255, 0);
								messagePlayerColor(parent->skill[2], MESSAGE_EQUIPMENT, color, Language::get(3746), items[item->type].getUnidentifiedName());
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
													hotbar[i].resetLastItem();
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
						players[i]->ghost.reset();
						FollowerMenu[i].recentEntity = nullptr;
						FollowerMenu[i].followerToCommand = nullptr;
						FollowerMenu[i].entityToInteractWith = nullptr;
						CalloutMenu[i].closeCalloutMenuGUI();
						CalloutMenu[i].callouts.clear();
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
					if ( soundNotification_group )
					{
						soundNotification_group->stop();
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
								if ( (int)follower->monsterSpecialAttackUnequipSafeguard > 0 )
								{
									// force deinit of special attacks to not be invalid state on next level.
									//messagePlayer(0, MESSAGE_DEBUG, "Cleared monster special");
									follower->handleMonsterSpecialAttack(followerStats, nullptr, 0.0, true);
								}

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

					std::string prevmapname = map.name;

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

					// signal clients about level change
					if ( gameModeManager.currentSession.seededRun.seed == 0 && !*cvar_map_sequence_rng )
					{
						mapseed = local_rng.rand();
					}
					else
					{
						map_sequence_rng.seedBytes(&uniqueGameKey, sizeof(uniqueGameKey));
						int rng_cycles = std::max(0, currentlevel + (secretlevel ? 100 : 0));
						while ( rng_cycles > 0 )
						{
							map_sequence_rng.rand(); // dummy advance
							--rng_cycles;
						}
						mapseed = map_sequence_rng.rand();
					}
					lastEntityUIDs = entity_uids;
					if ( forceMapSeed > 0 )
					{
						mapseed = forceMapSeed;
						forceMapSeed = 0;
					}

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

					if ( multiplayer == SERVER && net_packet && net_packet->data )
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

					bool playerDied[MAXPLAYERS] = { false };
					for ( int i = 0; i < MAXPLAYERS; ++i )
					{
						if ( stats[i] && stats[i]->HP <= 0 )
						{
							playerDied[i] = true;
						}
						if ( i == 0 || !players[i]->isLocalPlayer() )
						{
							Compendium_t::Events_t::eventUpdateWorld(i, Compendium_t::CPDM_GOLD_LEFT_BEHIND, "merchants guild", totalFloorGold);
							Compendium_t::Events_t::eventUpdateWorld(i, Compendium_t::MONSTERS_LEFT_BEHIND, "the church", totalFloorEnemies[i]);
							Compendium_t::Events_t::eventUpdateWorld(i, Compendium_t::ITEMS_LEFT_BEHIND, "merchants guild", totalFloorItems);
							Compendium_t::Events_t::eventUpdateWorld(i, Compendium_t::ITEM_VALUE_LEFT_BEHIND, "merchants guild", totalFloorItemValue[i]);
						}
					}

                    // load map file
					loading = true;
	                createLevelLoadScreen(5);
	                std::atomic_bool loading_done {false};
	                auto loading_task = std::async(std::launch::async, [&loading_done](){
					    gameplayCustomManager.readFromFile();
					    textSourceScript.scriptVariables.clear();
	                    updateLoadingScreen(10);

					    int checkMapHash = -1;
					    int result = physfsLoadMapFile(currentlevel, mapseed, false, &checkMapHash);
					    if (!verifyMapHash(map.filename, checkMapHash))
					    {
						    conductGameChallenges[CONDUCT_MODDED] = 1;
						    Mods::disableSteamAchievements = true;
					    }
	                    updateLoadingScreen(50);

					    numplayers = 0;
					    assignActions(&map);
                        updateLoadingScreen(55);

					    generatePathMaps();
	                    updateLoadingScreen(99);

		                loading_done = true;
		                return result;
		            });
	                while (!loading_done)
	                {
		                doLoadingScreen();
		                std::this_thread::sleep_for(std::chrono::milliseconds(1));
	                }
	                destroyLoadingScreen();
                    clearChunks();
                    createChunks();
		            loading = false;
	                int result = loading_task.get();

                    for (int c = 0; c < MAXPLAYERS; ++c) {
                        auto& camera = players[c]->camera();
					    camera.globalLightModifierActive = GLOBAL_LIGHT_MODIFIER_STOPPED;
                        camera.luminance = defaultLuminance;
					}

					// clear follower menu entities.
					for ( int i = 0; i < MAXPLAYERS; ++i )
					{
						minimapPings[i].clear(); // clear minimap pings
						if ( players[i]->isLocalPlayer() )
						{
							FollowerMenu[i].closeFollowerMenuGUI(true);
							CalloutMenu[i].closeCalloutMenuGUI();
						}
						players[i]->hud.followerBars.clear();
					}
					EnemyHPDamageBarHandler::dumpCache();
					monsterAllyFormations.reset();
					particleTimerEmitterHitEntities.clear();
					monsterTrapIgnoreEntities.clear();
					minimapHighlights.clear();

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
						messageLocalPlayers(MESSAGE_STATUS, Language::get(2599));

						// undo shopkeeper grudge
						for ( c = 0; c < MAXPLAYERS; ++c )
						{
							ShopkeeperPlayerHostility.resetPlayerHostility(c, true);
						}
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

					if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
					{
						if ( gameModeManager.Tutorial.showFirstTutorialCompletedPrompt )
						{
							gameModeManager.Tutorial.createFirstTutorialCompletedPrompt();
						}
					}
					else if ( !secretlevel )
					{
						messageLocalPlayers(MESSAGE_PROGRESSION, Language::get(710), currentlevel);
					}
					else
					{
						messageLocalPlayers(MESSAGE_PROGRESSION, Language::get(711), map.name);
					}

					gameModeManager.Tutorial.showFirstTutorialCompletedPrompt = false;

					if ( !secretlevel && result )
					{
						switch ( currentlevel )
						{
							case 2:
								messageLocalPlayers(MESSAGE_HINT, Language::get(712));
								Player::Minimap_t::mapDetails.push_back(std::make_pair("secret_exit_description", Language::get(712)));
								break;
							case 3:
								messageLocalPlayers(MESSAGE_HINT, Language::get(713));
								Player::Minimap_t::mapDetails.push_back(std::make_pair("secret_exit_description", Language::get(713)));
								break;
							case 7:
								messageLocalPlayers(MESSAGE_HINT, Language::get(714));
								Player::Minimap_t::mapDetails.push_back(std::make_pair("secret_exit_description", Language::get(714)));
								break;
							case 8:
								messageLocalPlayers(MESSAGE_HINT, Language::get(715));
								Player::Minimap_t::mapDetails.push_back(std::make_pair("secret_exit_description", Language::get(715)));
								break;
							case 11:
								messageLocalPlayers(MESSAGE_HINT, Language::get(716));
								Player::Minimap_t::mapDetails.push_back(std::make_pair("secret_exit_description", Language::get(716)));
								break;
							case 13:
								messageLocalPlayers(MESSAGE_HINT, Language::get(717));
								Player::Minimap_t::mapDetails.push_back(std::make_pair("secret_exit_description", Language::get(717)));
								break;
							case 16:
								messageLocalPlayers(MESSAGE_HINT, Language::get(718));
								Player::Minimap_t::mapDetails.push_back(std::make_pair("secret_exit_description", Language::get(718)));
								break;
							case 18:
								messageLocalPlayers(MESSAGE_HINT, Language::get(719));
								Player::Minimap_t::mapDetails.push_back(std::make_pair("secret_exit_description", Language::get(719)));
								break;
							default:
								break;
						}
					}
					if ( MFLAG_DISABLETELEPORT )
					{
						Player::Minimap_t::mapDetails.push_back(std::make_pair("map_flag_disable_teleport", Language::get(2382)));
					}
					if ( MFLAG_DISABLEOPENING )
					{
						Player::Minimap_t::mapDetails.push_back(std::make_pair("map_flag_disable_opening", Language::get(2382)));
					}
					if ( MFLAG_DISABLETELEPORT || MFLAG_DISABLEOPENING )
					{
						messageLocalPlayers(MESSAGE_HINT, Language::get(2382));
					}
					if ( MFLAG_DISABLELEVITATION )
					{
						messageLocalPlayers(MESSAGE_HINT, Language::get(2383));
						Player::Minimap_t::mapDetails.push_back(std::make_pair("map_flag_disable_levitation", Language::get(2383)));
					}
					if ( MFLAG_DISABLEDIGGING )
					{
						messageLocalPlayers(MESSAGE_HINT, Language::get(2450));
						Player::Minimap_t::mapDetails.push_back(std::make_pair("map_flag_disable_digging", Language::get(2450)));
					}
					if ( MFLAG_DISABLEHUNGER )
					{
						Player::Minimap_t::mapDetails.push_back(std::make_pair("map_flag_disable_hunger", ""));
					}

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
									messagePlayerMonsterEvent(c, 0xFFFFFFFF, *monsterStats, Language::get(721), Language::get(720), MSG_COMBAT_BASIC);
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
										if ( stats[c]->playerSummonPERCHR != 0 && MonsterData_t::nameMatchesSpecialNPCName(*monsterStats, "skeleton knight") )
										{
											monster->monsterAllySummonRank = (stats[c]->playerSummonPERCHR & 0x0000FF00) >> 8;
										}
										else if ( stats[c]->playerSummon2PERCHR != 0 && MonsterData_t::nameMatchesSpecialNPCName(*monsterStats, "skeleton sentinel") )
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
										Compendium_t::Events_t::eventUpdateWorld(c, Compendium_t::CPDM_HUMANS_SAVED, "the church", 1);
									}

									if ( c > 0 && multiplayer == SERVER && !players[c]->isLocalPlayer() && net_packet && net_packet->data )
									{
										strcpy((char*)net_packet->data, "LEAD");
										SDLNet_Write32((Uint32)monster->getUID(), &net_packet->data[4]);
										std::string name = monsterStats->name;
										if ( name != "" && name == MonsterData_t::getSpecialNPCName(*monsterStats) )
										{
											name = monsterStats->getAttribute("special_npc");
											name.insert(0, "$");
										}
                                        SDLNet_Write32(monsterStats->type, &net_packet->data[8]);
										strcpy((char*)(&net_packet->data[12]), name.c_str());
										net_packet->data[12 + strlen(name.c_str())] = 0;
										net_packet->address.host = net_clients[c - 1].host;
										net_packet->address.port = net_clients[c - 1].port;
										net_packet->len = 12 + strlen(name.c_str()) + 1;
										sendPacketSafe(net_sock, -1, net_packet, c - 1);

										serverUpdateAllyStat(c, monster->getUID(), monsterStats->LVL, monsterStats->HP, monsterStats->MAXHP, monsterStats->type);
									}
                                    
                                    if (players[c]->isLocalPlayer() && monsterStats->name[0] && (!monsterNameIsGeneric(*monsterStats) || monsterStats->type == SLIME)) {
                                        Entity* nametag = newEntity(-1, 1, map.entities, nullptr);
                                        nametag->x = monster->x;
                                        nametag->y = monster->y;
                                        nametag->z = monster->z - 6;
                                        nametag->sizex = 1;
                                        nametag->sizey = 1;
                                        nametag->flags[NOUPDATE] = true;
                                        nametag->flags[PASSABLE] = true;
                                        nametag->flags[SPRITE] = true;
                                        nametag->flags[UNCLICKABLE] = true;
                                        nametag->flags[BRIGHT] = true;
                                        nametag->behavior = &actSpriteNametag;
                                        nametag->parent = monster->getUID();
                                        nametag->scalex = 0.2;
                                        nametag->scaley = 0.2;
                                        nametag->scalez = 0.2;
                                        nametag->skill[0] = c;
                                        nametag->skill[1] = playerColor(c, colorblind_lobby, true);
                                    }

									if ( !FollowerMenu[c].recentEntity && players[c]->isLocalPlayer() )
									{
										FollowerMenu[c].recentEntity = monster;
									}
								}
								else
								{
									messagePlayerMonsterEvent(c, 0xFFFFFFFF, *tempStats, Language::get(723), Language::get(722), MSG_COMBAT_BASIC);
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

					for ( c = 0; c < MAXPLAYERS; c++ )
					{
						Compendium_t::Events_t::onLevelChangeEvent(c, Compendium_t::Events_t::previousCurrentLevel, Compendium_t::Events_t::previousSecretlevel, prevmapname, playerDied[c]);
						players[c]->compendiumProgress.playerAliveTimeTotal = 0;
						players[c]->compendiumProgress.playerGameTimeTotal = 0;
					}

                    // save at end of level change
					if ( gameModeManager.allowsSaves() )
					{
						saveGame();
					}
					Compendium_t::Events_t::writeItemsSaveData();
					Compendium_t::writeUnlocksSaveData();
#ifdef LOCAL_ACHIEVEMENTS
					LocalAchievements_t::writeToFile();
#endif
					break;
				}
			}
			if ( debugMonsterTimer )
			{
				if ( accum > 5.0 )
				{
					printlog("accum: %f, tick: %d", accum, ticks);
					for ( auto& pair : entityAccum )
					{
						printlog("entity: %d, accum: %.2f", pair.first, pair.second);
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
						if (net_packet && net_packet->data) {
							strcpy((char*)net_packet->data, "LVLC");
							net_packet->data[4] = secretlevel;
							SDLNet_Write32(mapseed, &net_packet->data[5]);
							SDLNet_Write32(lastEntityUIDs, &net_packet->data[9]);
							net_packet->data[13] = currentlevel;
							net_packet->data[14] = 0;
							net_packet->address.host = net_clients[c - 1].host;
							net_packet->address.port = net_clients[c - 1].port;
							net_packet->len = 15;
							sendPacketSafe(net_sock, -1, net_packet, c - 1);
						}
					}
				}

				bool updatePlayerHealth = false;
				if ( serverSchedulePlayerHealthUpdate != 0 && (ticks - serverSchedulePlayerHealthUpdate) >= TICKS_PER_SECOND * 0.2 )
				{
					// update if x ticks have passed since request of health update.
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

				{
					const bool forceUpdate = (ticks % (TICKS_PER_SECOND * 15)) == 0; // re-send every x seconds, or when update flag is dirty.
					ShopkeeperPlayerHostility.serverSendClientUpdate(forceUpdate);
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
				if (*cvar_enableKeepAlives) {
					for ( c = 1; c < MAXPLAYERS; c++ )
					{
						if ( client_disconnected[c] || players[c]->isLocalPlayer() || !net_packet || !net_packet->data )
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
							messageLocalPlayers(MESSAGE_MISC, Language::get(724), c, stats[c]->name);
						}
						else if ( !losingConnection[c] && ticks - client_keepalive[c] == TICKS_PER_SECOND * TIMEOUT_WARNING_TIME - 1 )
						{
							// give warning
							losingConnection[c] = true;
							messageLocalPlayers(MESSAGE_MISC, Language::get(725), c, stats[c]->name, TIMEOUT_TIME - TIMEOUT_WARNING_TIME);
						}
						else if ( !client_disconnected[c] && ticks - client_keepalive[c] >= TICKS_PER_SECOND * TIMEOUT_TIME - 1 )
						{
							// kick client
							messageLocalPlayers(MESSAGE_MISC, Language::get(726), c, stats[c]->name);
							strcpy((char*)net_packet->data, "KICK");
							net_packet->address.host = net_clients[c - 1].host;
							net_packet->address.port = net_clients[c - 1].port;
							net_packet->len = 4;
							sendPacketSafe(net_sock, -1, net_packet, c - 1);
							client_disconnected[c] = true;
						}
					}
					PingNetworkStatus_t::update();
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
							if (net_packet && net_packet->data) {
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
				players[player]->magic.bHasUnreadNewSpell = false;
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

					if ( item->notifyIcon && itemCategory(item) == SPELL_CAT )
					{
						players[player]->magic.bHasUnreadNewSpell = true;
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

					if ( ticks % TICKS_PER_SECOND == 25 )
					{
						if ( item->identified )
						{
							if ( item->type == SPELL_ITEM )
							{
								if ( auto spell = getSpellFromItem(player, item, true) )
								{
									Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_RUNS_COLLECTED, item->type, 1, false, spell->ID);
								}
							}
							else
							{
								Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_RUNS_COLLECTED, item->type, 1);
								if ( items[item->type].item_slot != ItemEquippableSlot::NO_EQUIP )
								{
									Compendium_t::Events_t::eventUpdate(player, Compendium_t::CPDM_BLESSED_MAX, item->type, item->beatitude);
								}
							}
						}
					}

					if ( item->type == FOOD_BLOOD && stats[player]->playerRace == RACE_VAMPIRE && stats[player]->stat_appearance == 0 )
					{
						bloodCount += item->count;
						if ( bloodCount >= 20 )
						{
							steamAchievement("BARONY_ACH_BLOOD_VESSELS");
						}
					}

					if ( itemCategory(item) == WEAPON || itemCategory(item) == THROWN )
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
						messagePlayer(player, MESSAGE_INVENTORY, Language::get(727), item->getName());
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
							&& !(item->identified) && players[player]->inventoryUI.appraisal.appraisalPossible(item) )
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


			if ( gameModeManager.currentSession.challengeRun.isActive()
				&& (gameModeManager.currentSession.challengeRun.eventType == gameModeManager.currentSession.challengeRun.CHEVENT_KILLS_MONSTERS
					|| gameModeManager.currentSession.challengeRun.eventType == gameModeManager.currentSession.challengeRun.CHEVENT_KILLS_FURNITURE)
				&& gameModeManager.currentSession.challengeRun.numKills >= 0 )
			{
				/*if ( gameStatistics[STATISTICS_TOTAL_KILLS] >= gameModeManager.currentSession.challengeRun.numKills
					&& achievementObserver.playerAchievements[PLAYER_NUM].totalKillsTickUpdate )
				{
					my->setHP(0);
					my->setObituary(Language::get(6152));
					stats[PLAYER_NUM]->killer = KilledBy::FAILED_CHALLENGE;
				}*/
				if ( gameStatistics[STATISTICS_TOTAL_KILLS] >= gameModeManager.currentSession.challengeRun.numKills
					&& achievementObserver.playerAchievements[clientnum].totalKillsTickUpdate )
				{
					if ( !fadeout )
					{
						victory = 100;
						if ( gameModeManager.currentSession.challengeRun.eventType == gameModeManager.currentSession.challengeRun.CHEVENT_KILLS_FURNITURE )
						{
							victory = 101;
						}
						if ( multiplayer == SERVER )
						{
							for ( int c = 1; c < MAXPLAYERS; c++ )
							{
								if ( client_disconnected[c] == true || players[c]->isLocalPlayer() )
								{
									continue;
								}
								strcpy((char*)net_packet->data, "WING");
								net_packet->data[4] = victory;
								net_packet->data[5] = 0;
								net_packet->address.host = net_clients[c - 1].host;
								net_packet->address.port = net_clients[c - 1].port;
								net_packet->len = 6;
								sendPacketSafe(net_sock, -1, net_packet, c - 1);
							}
						}
						movie = true;
						pauseGame(2, 0);
						MainMenu::destroyMainMenu();
						MainMenu::createDummyMainMenu();
						beginFade(MainMenu::FadeDestination::Endgame);
					}
				}
			}
			achievementObserver.playerAchievements[clientnum].totalKillsTickUpdate = false;
		}
		else if ( multiplayer == CLIENT )
		{
			// keep alives
			if ( *cvar_enableKeepAlives && net_packet && net_packet->data )
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
					messagePlayer(i, MESSAGE_MISC, Language::get(728));
				}
				else if ( !losingConnection[0] && ticks - client_keepalive[0] == TICKS_PER_SECOND * TIMEOUT_WARNING_TIME - 1 )
				{
					// give warning
					losingConnection[0] = true;
					messageLocalPlayers(MESSAGE_MISC, Language::get(729), TIMEOUT_TIME - TIMEOUT_WARNING_TIME);
				}
				else if ( !client_disconnected[c] && ticks - client_keepalive[0] >= TICKS_PER_SECOND * TIMEOUT_TIME - 1 )
				{
					// timeout
					messageLocalPlayers(MESSAGE_MISC, Language::get(730));
					MainMenu::timedOut();
					client_disconnected[0] = true;
				}
				PingNetworkStatus_t::update();
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
								if ( z == 0 )
								{
									// water and lava noises
									if ( ticks % (TICKS_PER_SECOND * 4) == (y + x * map.height) % (TICKS_PER_SECOND * 4) && local_rng.rand() % 3 == 0 )
									{
										int coord = x + y * 1000;
										if ( map.liquidSfxPlayedTiles.find(coord) == map.liquidSfxPlayedTiles.end() )
										{
											if ( lavatiles[map.tiles[index]] )
											{
												// bubbling lava
												playSoundPosLocal(x * 16 + 8, y * 16 + 8, 155, 100);
											}
											else if ( swimmingtiles[map.tiles[index]] )
											{
												// running water
												playSoundPosLocal(x * 16 + 8, y * 16 + 8, 135, 32);
											}
											map.liquidSfxPlayedTiles.insert(coord);
										}
									}

									// lava bubbles
									if ( lavatiles[map.tiles[index]] && !gameloopFreezeEntities )
									{
										if ( ticks % 40 == (y + x * map.height) % 40 && local_rng.rand() % 3 == 0 )
										{
											bool doLavaParticles = *cvar_lava_bubbles_enabled;
											if (doLavaParticles)
											{
												if ( *cvar_lava_use_vismap && !intro )
												{
													if ( x >= 0 && x < map.width && y >= 0 && y < map.height )
													{
														bool anyVismap = false;
														for ( int i = 0; i < MAXPLAYERS; ++i )
														{
															if ( !client_disconnected[i] && players[i]->isLocalPlayer() && cameras[i].vismap[y + x * map.height] )
															{
																anyVismap = true;
																break;
															}
														}
														if ( !anyVismap )
														{
															doLavaParticles = false;
														}
													}
												}
												int c, j = 1 + local_rng.rand() % 2;
												for ( c = 0; c < j && doLavaParticles; c++ )
												{
													Entity* entity = newEntity(42, 1, map.entities, nullptr); //Gib entity.
													entity->behavior = &actGib;
													entity->x = x * 16 + local_rng.rand() % 16;
													entity->y = y * 16 + local_rng.rand() % 16;
													entity->z = 7.5;
                                                    entity->ditheringDisabled = true;
													entity->flags[PASSABLE] = true;
													entity->flags[SPRITE] = true;
													entity->flags[NOUPDATE] = true;
													entity->flags[UPDATENEEDED] = false;
													entity->flags[UNCLICKABLE] = true;
													entity->sizex = 2;
													entity->sizey = 2;
													entity->fskill[3] = 0.01;
													double vel = (local_rng.rand() % 10) / 20.f;
													entity->vel_x = vel * cos(entity->yaw);
													entity->vel_y = vel * sin(entity->yaw);
													entity->vel_z = -.15 - (local_rng.rand() % 15) / 100.f;
													entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
													entity->pitch = (local_rng.rand() % 360) * PI / 180.0;
													entity->roll = (local_rng.rand() % 360) * PI / 180.0;
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
			}

			if ( ticks % TICKS_PER_SECOND == 0 )
			{
				updateGameplayStatisticsInMainLoop();
			}


			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				gameplayPreferences[i].process();
			}
			updatePlayerConductsInMainLoop();
			Compendium_t::Events_t::updateEventsInMainLoop(clientnum);
			achievementObserver.updatePlayerAchievement(clientnum, AchievementObserver::BARONY_ACH_DAPPER, AchievementObserver::DAPPER_EQUIPMENT_CHECK);

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
							if (net_packet && net_packet->data) {
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
							&& entity->behavior != &actDeathGhost
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
										if ( (entity->behavior != &actPlayerLimb && entity->behavior != &actDeathGhostLimb)
											|| entity->skill[2] != clientnum )
										{
											double ox = 0, oy = 0, onewx = 0, onewy = 0;

											// move the bodyparts of these otherwise the limbs will get left behind in this adjustment.
											if ( entity->behavior == &actPlayer || entity->behavior == &actMonster
												|| entity->behavior == &actDeathGhost )
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
											if ( entity->behavior == &actPlayer || entity->behavior == &actMonster
												|| entity->behavior == &actDeathGhost )
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
										if ( entity->behavior == &actPlayer 
											|| entity->behavior == &actMonster
											|| entity->behavior == &actDeathGhost )
										{
											ox = entity->x;
											oy = entity->y;
											onewx = entity->new_x;
											onewy = entity->new_y;
										}
										real_t dist = clipMove(&entity->x, &entity->y, entity->vel_x, entity->vel_y, entity);
										real_t new_dist = clipMove(&entity->new_x, &entity->new_y, entity->vel_x, entity->vel_y, entity);
										if ( entity->behavior == &actPlayer || entity->behavior == &actMonster
											|| entity->behavior == &actDeathGhost )
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
									dirYaw = fmod(dirYaw, 2 * PI);
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
									dirPitch = fmod(dirPitch, 2 * PI);
									while ( dirPitch >= PI )
									{
										dirPitch -= PI * 2;
									}
									while ( dirPitch < -PI )
									{
										dirPitch += PI * 2;
									}
									if ( entity->behavior != &actArrow )
									{
										// client handles pitch in actArrow.
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
									dirRoll = fmod(dirRoll, 2 * PI);
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

			int bloodCount = 0;
			players[clientnum]->magic.bHasUnreadNewSpell = false;
			for ( node = stats[clientnum]->inventory.first; node != NULL; node = nextnode )
			{
				nextnode = node->next;
				Item* item = (Item*)node->element;
				if ( !item )
				{
					continue;
				}
				if ( item->notifyIcon && itemCategory(item) == SPELL_CAT )
				{
					players[clientnum]->magic.bHasUnreadNewSpell = true;
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

				if ( ticks % TICKS_PER_SECOND == 25 )
				{
					if ( item->identified )
					{
						if ( item->type == SPELL_ITEM )
						{
							if ( auto spell = getSpellFromItem(clientnum, item, true) )
							{
								Compendium_t::Events_t::eventUpdate(clientnum, Compendium_t::CPDM_RUNS_COLLECTED, item->type, 1, false, spell->ID);
							}
						}
						else
						{
							Compendium_t::Events_t::eventUpdate(clientnum, Compendium_t::CPDM_RUNS_COLLECTED, item->type, 1);
							if ( items[item->type].item_slot != ItemEquippableSlot::NO_EQUIP )
							{
								Compendium_t::Events_t::eventUpdate(clientnum, Compendium_t::CPDM_BLESSED_MAX, item->type, item->beatitude);
							}
						}
					}
				}

				if ( itemCategory(item) == WEAPON )
				{
					if ( item->beatitude >= 10 )
					{
						steamAchievement("BARONY_ACH_BLESSED");
					}
				}

				if ( item->type == FOOD_BLOOD && stats[clientnum]->playerRace == RACE_VAMPIRE && stats[clientnum]->stat_appearance == 0 )
				{
					bloodCount += item->count;
					if ( bloodCount >= 20 )
					{
						steamAchievement("BARONY_ACH_BLOOD_VESSELS");
					}
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
					messagePlayer(clientnum, MESSAGE_INVENTORY, Language::get(727), item->getName());
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
					if ( auto_appraise_new_items && players[clientnum]->inventoryUI.appraisal.timer == 0 
						&& !(item->identified) && players[clientnum]->inventoryUI.appraisal.appraisalPossible(item) )
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

    bool playeralive = false;
    for (int c = 0; c < MAXPLAYERS; ++c) {
        if (players[c] && players[c]->entity) {
            playeralive = true;
        }
    }

    // increment gameplay time
	if (!gamePaused && !intro && playeralive)
	{
		++completionTime;
		for ( int c = 0; c < MAXPLAYERS; ++c ) 
		{
			players[c]->compendiumProgress.playerAliveTimeTotal++;
		}
	}
	if ( !gamePaused && !intro )
	{
		for ( int c = 0; c < MAXPLAYERS; ++c )
		{
			players[c]->compendiumProgress.playerGameTimeTotal++;
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
		if (!strcmp(button->label, Language::get(733)))
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
		if ( !strcmp(button->label, Language::get(2498)) )
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
			Uint32 color = makeColor( 255, 255, 0, 127);
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

static void bindControllerToPlayer(int id, int player) {
    inputs.removeControllerWithDeviceID(id); // clear any other player using this
    inputs.setControllerID(player, id);
    inputs.getVirtualMouse(player)->draw_cursor = false;
    inputs.getVirtualMouse(player)->lastMovementFromController = true;
    printlog("(Device %d bound to player %d)", id, player);
    for (int c = 0; c < 4; ++c) {
        auto& input = Input::inputs[c];
	    input.refresh();
    }
    auto& input = Input::inputs[player];
    if (event.cbutton.button == SDL_CONTROLLER_BUTTON_A)
    {
        input.consumeBinary("MenuConfirm");
    }
    if (event.cbutton.button == SDL_CONTROLLER_BUTTON_B)
    {
        input.consumeBinary("MenuBack");
    }
    if (event.cbutton.button == SDL_CONTROLLER_BUTTON_START)
    {
        input.consumeBinary("MenuStart");
    }
}

bool handleEvents(void)
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

#ifdef DEBUG_EVENT_TIMERS
	auto time1 = std::chrono::high_resolution_clock::now();
	auto time2 = std::chrono::high_resolution_clock::now();
	real_t accum = 0.0;

	time2 = std::chrono::high_resolution_clock::now();
	accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
	if ( accum > 5 )
	{
		printlog("Large tick time: [0] %f", accum);
	}
	time1 = std::chrono::high_resolution_clock::now();
#endif

	// do timer
	int numframes = 0;
	time_diff += timesync;
	constexpr real_t frame = (real_t)1000 / (real_t)TICKS_PER_SECOND;
	while (time_diff >= frame) {
		time_diff -= frame;
		++numframes;
	}

	// drop excessive frames
	constexpr int max_frames = 5;
	if (numframes > max_frames) {
		//printlog("dropped %d frames", numframes - max_frames);
		numframes = max_frames;
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

#ifdef DEBUG_EVENT_TIMERS
	time2 = std::chrono::high_resolution_clock::now();
	accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
	if ( accum > 5 )
	{
		printlog("Large tick time: [1] %f", accum);
	}
	time1 = std::chrono::high_resolution_clock::now();
#endif

	Input::lastInputOfAnyKind = "";

#ifdef NINTENDO
	// update controllers
	nxControllersUpdate();

	// detect resolution changes
	if (nxHasResolutionChanged()) {
		int x, y;
		nxGetCurrentResolution(x, y);
		printlog("new display size: %d %d", x, y);

		if (!changeVideoMode(x, y)) {
			printlog("critical error! Attempting to abort safely...\n");
			mainloop = 0;
		}
		if (!intro) {
			MainMenu::setupSplitscreen();
		}
	}

	// detect app focus changes
	const bool asleep = nxAppOutOfFocus();
#ifdef USE_EOS
	EOS.SetSleepStatus(asleep);
#endif
	if (asleep) {
		if (!intro && !gamePaused) {
			if (!MainMenu::isMenuOpen() && !MainMenu::isCutsceneActive()) {
				pauseGame(2, 0);
			}
		}
	}
#endif

    // consume mouse buttons that were eaten by GUI
	if (!framesProcResult.usable && *framesEatMouse) {
	    for (int c = 0; c < MAXPLAYERS; ++c) {
	        if (!players[c]->shootmode) {
	            if (Input::inputs[c].consumeBinaryToggle("MenuLeftClick")) {
	                Input::inputs[c].consumeBindingsSharedWithBinding("MenuLeftClick");
	            }
	            if (Input::inputs[c].consumeBinaryToggle("MenuMiddleClick")) {
	                Input::inputs[c].consumeBindingsSharedWithBinding("MenuMiddleClick");
	            }
	            if (Input::inputs[c].consumeBinaryToggle("MenuRightClick")) {
	                Input::inputs[c].consumeBindingsSharedWithBinding("MenuRightClick");
	            }
	        }
	    }
	}

#ifdef DEBUG_EVENT_TIMERS
	time2 = std::chrono::high_resolution_clock::now();
	accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
	if ( accum > 5 )
	{
		printlog("Large tick time: [2] %f", accum);
	}
	time1 = std::chrono::high_resolution_clock::now();
#endif

	// update network state
#if defined(NINTENDO)
	if (initialized && !loading) {
		// update local wireless communication mode
		if (directConnect && multiplayer != SINGLE) {
			if (!nxHandleWireless()) {
				MainMenu::timedOut(); // handle wireless disconnect
			}
			if (multiplayer == SERVER && !intro) {
				if (ticks % TICKS_PER_SECOND == 0) {
					int numplayers = 0;
					for (int c = 0; c < MAXPLAYERS; ++c) {
						if (!client_disconnected[c]) {
							++numplayers;
						}
					}
					char address[64] = { '\0' };
					bool result = false;
					nxGetWirelessAddress(address, sizeof(address));
					if (address[0]) {
						result = nxUpdateLobby(address, MainMenu::getHostname(), svFlags, numplayers);
					}
					if (!result) {
						MainMenu::timedOut();
					}
				}
			}
		}

#ifdef USE_EOS
		// handle EOS timeouts and disconnects
		const bool connected = nxConnectedToNetwork();
		EOS.SetNetworkAvailable(connected);
		if (EOS.isInitialized() && EOS.CurrentUserInfo.isLoggedIn() && EOS.CurrentUserInfo.isValid()) {
			// I don't care if we're in the lobby browser, hosting a lobby, or playing a game.
			// Any state we are in where EOS is connected, we need to end the game immediately if
			// the network is lost. Or else Epic has a freakout.
			if (!connected) {
				MainMenu::timedOut();
			}
		}
#endif // USE_EOS
	}
#endif // NINTENDO

#ifdef DEBUG_EVENT_TIMERS
	time2 = std::chrono::high_resolution_clock::now();
	accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
	if ( accum > 5 )
	{
		printlog("Large tick time: [3] %f", accum);
	}
	time1 = std::chrono::high_resolution_clock::now();
#endif

	while ( SDL_PollEvent(&event) )   // poll SDL events
	{
#ifdef USE_IMGUI
		if ( ImGui_t::isInit )
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
#ifdef DEBUG_EVENT_TIMERS
			time2 = std::chrono::high_resolution_clock::now();
			accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
			if ( accum > 5 )
			{
				printlog("Large tick time: [4] %f", accum);
			}
			time1 = std::chrono::high_resolution_clock::now();
#endif
		}
#endif
		// Global events
		switch ( event.type )
		{
			case SDL_QUIT: // if SDL receives the shutdown signal
				mainloop = 0;
				break;
			case SDL_KEYDOWN: // if a key is pressed...
			    if (demo_mode != DemoMode::STOPPED) {
			        if (event.key.keysym.sym == SDLK_ESCAPE) {
			            demo_stop();
			        }
			        else if (demo_mode == DemoMode::PLAYING) {
			            break;
			        }
			    }
				if ( command )
				{
				    static int saved_command_index = 0;
				    static std::string saved_command_str;
				    if ( event.key.keysym.sym == SDLK_TAB )
				    {
				        if (saved_command_str.empty()) {
				            saved_command_str = command_str;
				        }
				        auto find = FindConsoleCommand(saved_command_str.c_str(), saved_command_index);
				        if (find) {
				            strcpy(command_str, find);
				            ++saved_command_index;
				        } else if (saved_command_index) {
				            saved_command_index = 0;
				            find = FindConsoleCommand(saved_command_str.c_str(), saved_command_index);
				            if (find) {
				                strcpy(command_str, find);
				                ++saved_command_index;
				            }
				        }
				    }
				    else
				    {
				        saved_command_index = 0;
				        saved_command_str.clear();
				    }
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
					    size_t destlen = strlen(inputstr);
					    size_t remaining = inputlen - destlen - 1;
					    size_t srclen = std::min(strlen(SDL_GetClipboardText()), remaining);
					    memcpy(inputstr + destlen, SDL_GetClipboardText(), srclen);
					    inputstr[destlen + srclen] = '\0';
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
					lastkeypressed = event.key.keysym.sym;
#ifdef APPLE
                    switch (lastkeypressed)
                    {
                        default: break;
                        case SDLK_NUMLOCKCLEAR: lastkeypressed = SDLK_KP_CLEAR; break;
                        case SDLK_PRINTSCREEN: lastkeypressed = SDLK_F13; break;
                        case SDLK_SCROLLLOCK: lastkeypressed = SDLK_F14; break;
                        case SDLK_PAUSE: lastkeypressed = SDLK_F15; break;
                    }
#endif
					keystatus[lastkeypressed] = true;
					Input::keys[lastkeypressed] = true;
					Input::lastInputOfAnyKind = SDL_GetKeyName(lastkeypressed);
				}
				break;
			case SDL_KEYUP: // if a key is unpressed...
			    if (demo_mode == DemoMode::PLAYING) {
			        break;
			    }
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
                    SDL_Keycode key = event.key.keysym.sym;
#ifdef APPLE
                    switch (key)
                    {
                        default: break;
                        case SDLK_NUMLOCKCLEAR: key = SDLK_KP_CLEAR; break;
                        case SDLK_PRINTSCREEN: key = SDLK_F13; break;
                        case SDLK_SCROLLLOCK: key = SDLK_F14; break;
                        case SDLK_PAUSE: key = SDLK_F15; break;
                    }
#endif
					keystatus[key] = false;
					Input::keys[key] = false;
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
			case SDL_FINGERDOWN: {
				if (demo_mode == DemoMode::PLAYING) {
					break;
				}
				fingerdown = true;
				const int x = event.tfinger.x * xres;
				const int y = event.tfinger.y * yres;
				inputs.setMouse(clientnum, Inputs::MouseInputs::X, x);
				inputs.setMouse(clientnum, Inputs::MouseInputs::Y, y);
				inputs.setMouse(clientnum, Inputs::MouseInputs::OX, x);
				inputs.setMouse(clientnum, Inputs::MouseInputs::OY, y);
				//fingerx = x;
				//fingery = y;
				//ofingerx = x;
				//ofingery = y;
				//mousexrel += event.tfinger.dx * xres;
				//mouseyrel += event.tfinger.dy * yres;
				break;
			}
			case SDL_FINGERUP: {
				if (demo_mode == DemoMode::PLAYING) {
					break;
				}
				const int x = event.tfinger.x * xres;
				const int y = event.tfinger.y * yres;
				inputs.setMouse(clientnum, Inputs::MouseInputs::X, x);
				inputs.setMouse(clientnum, Inputs::MouseInputs::Y, y);
				fingerdown = false;
				//fingerx = x;
				//fingery = y;
				//mousexrel += event.tfinger.dx * xres;
				//mouseyrel += event.tfinger.dy * yres;
				break;
			}
			case SDL_FINGERMOTION: {
				if (demo_mode == DemoMode::PLAYING) {
					break;
				}
				const int x = event.tfinger.x * xres;
				const int y = event.tfinger.y * yres;
				inputs.setMouse(clientnum, Inputs::MouseInputs::X, x);
				inputs.setMouse(clientnum, Inputs::MouseInputs::Y, y);
				//fingerx = x;
				//fingery = y;
				//mousexrel += event.tfinger.dx * xres;
				//mouseyrel += event.tfinger.dy * yres;
				break;
			}
#ifndef NINTENDO
			case SDL_MOUSEBUTTONDOWN: // if a mouse button is pressed...
				if (demo_mode == DemoMode::PLAYING) {
					break;
				}
#ifdef USE_IMGUI
				if ( ImGui_t::requestingMouse() )
				{
					break;
				}
#endif // USE_IMGUI
#ifdef APPLE
                if ((keystatus[SDLK_LCTRL] || keystatus[SDLK_RCTRL]) && event.button.button == SDL_BUTTON_LEFT) {
                    mousestatus[SDL_BUTTON_RIGHT] = 1;
                    Input::mouseButtons[SDL_BUTTON_RIGHT] = 1;
                    Input::lastInputOfAnyKind = "Mouse2";
                    lastkeypressed = 284;
                } else {
                    mousestatus[event.button.button] = 1; // set this mouse button to 1
                    Input::mouseButtons[event.button.button] = 1;
                    Input::lastInputOfAnyKind = std::string("Mouse") + std::to_string(event.button.button);
                    lastkeypressed = 282 + event.button.button;
                }
#else
                mousestatus[event.button.button] = 1; // set this mouse button to 1
                Input::mouseButtons[event.button.button] = 1;
                Input::lastInputOfAnyKind = std::string("Mouse") + std::to_string(event.button.button);
                lastkeypressed = 282 + event.button.button;
#endif
				break;
			case SDL_MOUSEBUTTONUP: // if a mouse button is released...
			    if (demo_mode == DemoMode::PLAYING) {
			        break;
			    }
				mousestatus[event.button.button] = 0; // set this mouse button to 0
				Input::mouseButtons[event.button.button] = 0;
#ifdef APPLE
                mousestatus[SDL_BUTTON_RIGHT] = 0;
                Input::mouseButtons[SDL_BUTTON_RIGHT] = 0;
#endif
				break;
			case SDL_MOUSEWHEEL:
			    if (demo_mode == DemoMode::PLAYING) {
			        break;
			    }
#ifdef USE_IMGUI
				if ( ImGui_t::requestingMouse() )
				{
					break;
				}
#endif // USE_IMGUI
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
			    if (demo_mode == DemoMode::PLAYING) {
			        break;
			    }
				if ( firstmouseevent == true )
				{
					firstmouseevent = false;
					break;
				}
				menuselect = 0;
                float factorX;
                float factorY;
                {
                    int w1, w2, h1, h2;
                    SDL_GL_GetDrawableSize(screen, &w1, &h1);
                    SDL_GetWindowSize(screen, &w2, &h2);
                    factorX = (float)w1 / w2;
                    factorY = (float)h1 / h2;
                }
				mousex = event.motion.x * factorX;
				mousey = event.motion.y * factorY;
				mousexrel += event.motion.xrel;
				mouseyrel += event.motion.yrel;

				if (initialized)
				{
					for ( int i = 0; i < MAXPLAYERS; ++i )
					{
						if ( gamePaused || intro )
						{
							inputs.getVirtualMouse(i)->lastMovementFromController = false;
						}
						if ( inputs.bPlayerUsingKeyboardControl(i) && (!inputs.hasController(i) || gamePaused) )
						{
							inputs.getVirtualMouse(i)->lastMovementFromController = false;
							if ( !players[i]->shootmode || !players[i]->entity || gamePaused )
							{
								inputs.getVirtualMouse(i)->draw_cursor = true;
							}
						}
					}
				}
				break;
			case SDL_CONTROLLERBUTTONDOWN: // if joystick button is pressed
			{
			    if (demo_mode == DemoMode::PLAYING) {
			        break;
			    }
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
			    if ( event.cbutton.button == SDL_CONTROLLER_BUTTON_A ||
			         event.cbutton.button == SDL_CONTROLLER_BUTTON_B ||
			         event.cbutton.button == SDL_CONTROLLER_BUTTON_START )
			    {
				    SDL_GameController* pad = SDL_GameControllerFromInstanceID(event.cbutton.which);
				    if ( !pad )
				    {
					    printlog("(Unknown pad pressed input (instance: %d), null controller returned.)\n", event.cbutton.which);
				    }
				    else
				    {
					    for ( auto& controller : game_controllers )
					    {
						    if ( controller.isActive() && controller.getControllerDevice() == pad )
						    {
			                    if ( Input::waitingToBindControllerForPlayer >= 0 )
			                    {
			                        // we are explicitly waiting to bind a controller to a specific player
			                        bindControllerToPlayer(controller.getID(), Input::waitingToBindControllerForPlayer);
								    Input::waitingToBindControllerForPlayer = -1;
							    }
							    else
							    {
							        // we are not strictly waiting to bind a controller to a player, but check if this one is already bound
							        bool alreadyBound = false;
							        for (int player = 0; player < MAXPLAYERS; ++player)
							        {
							            if (inputs.getControllerID(player) == controller.getID())
							            {
							                alreadyBound = true;
							                break;
							            }
							        }
							        if (!alreadyBound)
							        {
							            // controller is not already bound - bind it to the first player who does not have a controller
							            for (int player = 0; player < MAXPLAYERS; ++player)
							            {
							                if (multiplayer != SINGLE && player != 0)
							                {
							                    // only assign the controller to the first player in net lobbies
												// do this even for clients: player 1 controls route to other slots
												// in online multiplayer!
							                    continue;
							                }
											if (intro && multiplayer != CLIENT && MainMenu::isPlayerSlotLocked(player))
											{
												// don't assign the controller to this player if their slot is locked!
												continue;
											}
							                if (intro && multiplayer != CLIENT && MainMenu::isPlayerSignedIn(player))
							                {
							                    // in the lobby, don't assign controllers to players who are already signed in...
							                    continue;
							                }
							                if (inputs.hasController(player))
							                {
							                    // this player already has a controller
							                    continue;
							                }
							                if (!intro && inputs.getPlayerIDAllowedKeyboard() == player && (splitscreen || multiplayer != SINGLE))
							                {
							                    // this player is using a keyboard
							                    continue;
							                }
		                                    bindControllerToPlayer(controller.getID(), player);
		                                    alreadyBound = true;
						                    break;
							            }
							            if (!alreadyBound) {
							                // didn't find anybody to bind to. try again, but ignore the keyboard restriction
							                for (int player = 0; player < MAXPLAYERS; ++player)
							                {
												if (multiplayer != SINGLE && player != 0)
												{
													// only assign the controller to the first player in net lobbies
													// do this even for clients: player 1 controls route to other slots
													// in online multiplayer!
													continue;
												}
												if (intro && multiplayer != CLIENT && MainMenu::isPlayerSlotLocked(player))
												{
													// don't assign the controller to this player if their slot is locked!
													continue;
												}
							                    if (intro && multiplayer != CLIENT && MainMenu::isPlayerSignedIn(player))
							                    {
							                        // in the lobby, don't assign controllers to players who are already signed in...
							                        continue;
							                    }
							                    if (inputs.hasController(player))
							                    {
							                        // this player already has a controller
							                        continue;
							                    }
		                                        bindControllerToPlayer(controller.getID(), player);
		                                        alreadyBound = true;
						                        break;
							                }
							            }
							        }
							    }
							    break;
						    }
					    }
					}
				}
				break;
			}
			case SDL_CONTROLLERAXISMOTION:
			{
			    if (demo_mode == DemoMode::PLAYING) {
			        break;
			    }
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
			    if (demo_mode == DemoMode::PLAYING) {
			        break;
			    }
			    break;
			}
			case SDL_CONTROLLERDEVICEADDED:
			{
			    if (demo_mode == DemoMode::PLAYING) {
			        break;
			    }
				const int sdl_device_index = event.cdevice.which; // this is an index within SDL_Numjoysticks(), not to be referred to from now on.
				if ( !SDL_IsGameController(sdl_device_index) )
				{
					printlog("Info: device %d is not a game controller! Joysticks are not supported.\n", sdl_device_index);
					break;
				}

				bool deviceAlreadyAdded = false;
				SDL_GameController* newControllerAdded = SDL_GameControllerOpen(sdl_device_index);
				if ( newControllerAdded != nullptr )
				{
					for ( auto& controller : game_controllers )
					{
						if ( controller.isActive() )
						{
							if ( controller.getControllerDevice() == newControllerAdded )
							{
								// we already have this controller in our system.
								deviceAlreadyAdded = true;
								printlog("(Device %d added, but already in use as game controller.)\n", sdl_device_index);
								break;
							}
						}
					}
				}

				if ( newControllerAdded )
				{
					SDL_GameControllerClose(newControllerAdded); // we're going to re-open this below..
					newControllerAdded = nullptr;
				}

				if ( deviceAlreadyAdded )
				{
					break;
				}

				// now find a free controller slot.
                int id = -1;
				for ( int c = 0; c < game_controllers.size(); ++c )
				{
					auto& controller = game_controllers[c];
					if ( controller.isActive() )
					{
						continue;
					}

                    id = c;
					bool result = controller.open(sdl_device_index, id);
					assert(result); // this should always succeed because we test that the device index is valid above.
					printlog("Device %d successfully initialized as game controller in slot %d.\n", sdl_device_index, id);
					controller.initBindings();
					Input::gameControllers[id] = controller.getControllerDevice();
					for (int c = 0; c < 4; ++c) {
						Input::inputs[c].refresh();
					}
					break;
				}
				for ( auto& controller : game_controllers )
				{
					// haptic devices are enumerated differently than joysticks
					// reobtain haptic devices for each existing controller
					controller.reinitHaptic();
				}
#ifdef STEAMWORKS
                // on steam deck, player 1 always needs a controller.
                if (SteamUtils()->IsSteamRunningOnSteamDeck()) {
                    if (id >= 0 && !inputs.hasController(0)) {
                        bindControllerToPlayer(id, 0);
                    }
                }
#endif
				break;
			}
			case SDL_CONTROLLERDEVICEREMOVED:
			{
			    if (demo_mode == DemoMode::PLAYING) {
			        break;
			    }
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
						const int id = controller.getID();
						inputs.removeControllerWithDeviceID(id);
						printlog("Device %d removed as game controller (it was in slot %d).\n", instanceID, id);
						controller.close();
						Input::gameControllers.erase(id);
						for ( int c = 0; c < 4; ++c ) {
							Input::inputs[c].refresh();
						}
					}
				}
				for ( auto& controller : game_controllers )
				{
					// haptic devices are enumerated differently than joysticks
					// reobtain haptic devices for each existing controller
					controller.reinitHaptic();
				}
				break;
			}
			case SDL_JOYDEVICEADDED:
			{
			    if (demo_mode == DemoMode::PLAYING) {
			        break;
			    }
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
			    if (demo_mode == DemoMode::PLAYING) {
			        break;
			    }
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
			    if (demo_mode == DemoMode::PLAYING) {
			        break;
			    }
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
			    if (demo_mode == DemoMode::PLAYING) {
			        break;
			    }
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
			    if (demo_mode == DemoMode::PLAYING) {
			        break;
			    }
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
#endif
			case SDL_WINDOWEVENT:
				if ( event.window.event == SDL_WINDOWEVENT_FOCUS_LOST && mute_audio_on_focus_lost )
				{
				    setGlobalVolume(0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
				}
				else if ( event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED )
				{
				    setGlobalVolume(MainMenu::master_volume,
				        musvolume,
				        sfxvolume,
				        sfxAmbientVolume,
				        sfxEnvironmentVolume,
						sfxNotificationVolume);
				}
				else if (event.window.event == SDL_WINDOWEVENT_RESIZED)
				{
#if defined(NINTENDO)
					if (!changeVideoMode(event.window.data1, event.window.data2))
					{
						printlog("critical error! Attempting to abort safely...\n");
						mainloop = 0;
					}
					if (!intro) {
						MainMenu::setupSplitscreen();
					}
#else
                    float factorX, factorY;
                    {
                        int w1, w2, h1, h2;
                        SDL_GL_GetDrawableSize(screen, &w1, &h1);
                        SDL_GetWindowSize(screen, &w2, &h2);
                        factorX = (float)w1 / w2;
                        factorY = (float)h1 / h2;
                    }
                    const int x = event.window.data1 * factorX;
                    const int y = event.window.data2 * factorY;
					if (!resizeWindow(x, y))
					{
						printlog("critical error! Attempting to abort safely...\n");
						mainloop = 0;
					}
#endif
				}
				break;
		}

#ifdef DEBUG_EVENT_TIMERS
		time2 = std::chrono::high_resolution_clock::now();
		accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
		if ( accum > 5 )
		{
			printlog("Large tick time: [5] event: %d %f", event.type, accum);
		}
		time1 = std::chrono::high_resolution_clock::now();
#endif
	}

#ifdef DEBUG_EVENT_TIMERS
	time2 = std::chrono::high_resolution_clock::now();
	accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
	if ( accum > 5 )
	{
		printlog("Large tick time: [6] %f", accum);
	}
	time1 = std::chrono::high_resolution_clock::now();
#endif

	if (numframes)
	{
#ifdef SOUND
		// update listener position, music fades, etc
		if (multiplayer != SINGLE || intro) {
			sound_update(0, clientnum, 1);
		} else {
			int numplayers = 0;
			for (int c = 0; c < MAXPLAYERS; ++c) {
				if (!client_disconnected[c]) {
					++numplayers;
				}
			}
			for (int player = 0, c = 0; c < MAXPLAYERS; ++c) {
				if (!client_disconnected[c]) {
					sound_update(player, c, numplayers);
					++player;
				}
			}
		}
#endif
	}

#ifdef DEBUG_EVENT_TIMERS
	time2 = std::chrono::high_resolution_clock::now();
	accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
	if ( accum > 5 )
	{
		printlog("Large tick time: [7] %f", accum);
	}
	time1 = std::chrono::high_resolution_clock::now();
#endif

	for (int runtimes = 0; runtimes < numframes; ++runtimes)
	{
		if (!loading && initialized)
		{
			gameLogic();
			++ticks;
		}
		else
		{
			++loadingticks;
		}
	}

#ifdef DEBUG_EVENT_TIMERS
	time2 = std::chrono::high_resolution_clock::now();
	accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
	if ( accum > 5 )
	{
		printlog("Large tick time: [8] %f", accum);
	}
	time1 = std::chrono::high_resolution_clock::now();
#endif

	if (initialized)
	{
		if ( !mousestatus[SDL_BUTTON_LEFT] )
		{
			omousex = mousex;
			omousey = mousey;
		}
		inputs.updateAllOMouse();
		for ( auto& controller : game_controllers )
		{
			controller.updateButtons();
			controller.updateAxis();
		}
	}

#ifdef DEBUG_EVENT_TIMERS
	time2 = std::chrono::high_resolution_clock::now();
	accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
	if ( accum > 5 )
	{
		printlog("Large tick time: [9] %f", accum);
	}
	time1 = std::chrono::high_resolution_clock::now();
#endif

	return numframes > 0;
}

/*-------------------------------------------------------------------------------

	pauseGame

	pauses or unpauses the game, depending on its current state

-------------------------------------------------------------------------------*/

void pauseGame(int mode /* 0 == toggle, 1 == force unpause, 2 == force pause */, int ignoreplayer /* ignored */)
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
	if ( MainMenu::isCutsceneActive() )
	{
		return;
	}

	if ( (!gamePaused && mode != 1) || mode == 2 )
	{
	    playSound(500, 96);
		gamePaused = true;
		bool noOneUsingKeyboard = true;
		for (int c = 0; c < 4; ++c)
		{
		    if (inputs.bPlayerUsingKeyboardControl(c) && MainMenu::isPlayerSignedIn(c) && players[c]->isLocalPlayer()) {
		        noOneUsingKeyboard = false;
		    }
		    auto& input = Input::inputs[c];
			if (input.binary("Pause Game") || (inputs.bPlayerUsingKeyboardControl(c) && keystatus[SDLK_ESCAPE] && !input.isDisabled())) {
			    MainMenu::pause_menu_owner = c;
			    break;
			}
		}
		if (noOneUsingKeyboard && keystatus[SDLK_ESCAPE]) {
		    MainMenu::pause_menu_owner = clientnum;
		}
		if ( SDL_GetRelativeMouseMode() )
		{
			SDL_SetRelativeMouseMode(SDL_FALSE);
		}
		if (keystatus[SDLK_ESCAPE]) {
			keystatus[SDLK_ESCAPE] = 0;
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
				if (net_packet && net_packet->data) {
					strcpy((char*)net_packet->data, "PAUS");
					net_packet->data[4] = clientnum;
					net_packet->address.host = net_clients[c - 1].host;
					net_packet->address.port = net_clients[c - 1].port;
					net_packet->len = 5;
					sendPacketSafe(net_sock, -1, net_packet, c - 1);
				}
			}
		}
		else if ( multiplayer == CLIENT && ignoreplayer )
		{
			if (net_packet && net_packet->data) {
				strcpy((char*)net_packet->data, "PAUS");
				net_packet->data[4] = clientnum;
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 5;
				sendPacketSafe(net_sock, -1, net_packet, 0);
			}
		}
	}
	else if ( (gamePaused && mode != 2) || mode == 1 )
	{
		buttonCloseSubwindow(NULL);
		gamePaused = false;
		if ( !SDL_GetRelativeMouseMode() && capture_mouse )
		{
            // fix for macOS: put mouse back in window before recapturing mouse
            if (EnableMouseCapture) {
                int mouse_x, mouse_y;
                SDL_GetGlobalMouseState(&mouse_x, &mouse_y);
                int x, y, w, h;
                SDL_GetWindowPosition(screen, &x, &y);
                SDL_GetWindowSize(screen, &w, &h);
                if (mouse_x < x || mouse_x >= x + w ||
                    mouse_y < y || mouse_y >= y + h) {
                    SDL_WarpMouseInWindow(screen, w/2, h/2);
                }
            }
			SDL_SetRelativeMouseMode(EnableMouseCapture);
		}
		if (keystatus[SDLK_ESCAPE]) {
			keystatus[SDLK_ESCAPE] = 0;
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
				if (net_packet && net_packet->data) {
					strcpy((char*)net_packet->data, "UNPS");
					net_packet->data[4] = clientnum;
					net_packet->address.host = net_clients[c - 1].host;
					net_packet->address.port = net_clients[c - 1].port;
					net_packet->len = 5;
					sendPacketSafe(net_sock, -1, net_packet, c - 1);
				}
			}
		}
		else if ( multiplayer == CLIENT && ignoreplayer )
		{
			if (net_packet && net_packet->data) {
				strcpy((char*)net_packet->data, "UNPS");
				net_packet->data[4] = clientnum;
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 5;
				sendPacketSafe(net_sock, -1, net_packet, 0);
			}
		}
	}
}

/*-------------------------------------------------------------------------------

	frameRateLimit

	Returns true until the correct number of frames has passed from the
	beginning of the last cycle in the main loop.

-------------------------------------------------------------------------------*/

// records the SDL_GetTicks() value at the moment the mainloop restarted
static Uint64 lastGameTickCount = 0;
static Uint64 framerateAccumulatedTicks = 0;

// credit "computerBear" for preciseSleep/preciseSleepWindows https://blat-blatnik.github.io/computerBear/making-accurate-sleep-function/
void preciseSleep(double seconds) 
{
	static double estimate = 5e-3;
	static double mean = 5e-3;
	static double m2 = 0;
	static int64_t count = 1;

	while ( seconds > estimate ) {
		auto start = std::chrono::high_resolution_clock::now();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		auto end = std::chrono::high_resolution_clock::now();

		double observed = (end - start).count() / 1e9;
		seconds -= observed;

		++count;
		double delta = observed - mean;
		mean += delta / count;
		m2 += delta * (observed - mean);
		double stddev = sqrt(m2 / (count - 1));
		estimate = mean + stddev;
	}

	// spin lock
	auto start = std::chrono::high_resolution_clock::now();
	while ( (std::chrono::high_resolution_clock::now() - start).count() / 1e9 < seconds );
}

void preciseSleepWindows(double seconds)
{
#ifdef WINDOWS
	static HANDLE timer = CreateWaitableTimer(NULL, FALSE, NULL);
	static double estimate = 5e-3;
	static double mean = 5e-3;
	static double m2 = 0;
	static int64_t count = 1;

	while ( seconds - estimate > 1e-7 ) {
		double toWait = seconds - estimate;
		LARGE_INTEGER due;
		due.QuadPart = -int64_t(toWait * 1e7);
		auto start = std::chrono::high_resolution_clock::now();
		SetWaitableTimerEx(timer, &due, 0, NULL, NULL, NULL, 0);
		WaitForSingleObject(timer, INFINITE);
		auto end = std::chrono::high_resolution_clock::now();

		double observed = (end - start).count() / 1e9;
		seconds -= observed;

		++count;
		double error = observed - toWait;
		double delta = error - mean;
		mean += delta / count;
		m2 += delta * (error - mean);
		double stddev = sqrt(m2 / (count - 1));
		estimate = mean + stddev;
	}

	// spin lock
	auto start = std::chrono::high_resolution_clock::now();
	while ( (std::chrono::high_resolution_clock::now() - start).count() / 1e9 < seconds );
#endif // WINDOWS
}

bool frameRateLimit( Uint32 maxFrameRate, bool resetAccumulator, bool sleep )
{
	if ( maxFrameRate == 0 )
	{
		return false;
	}
	float desiredFrameSeconds = 1.0f / maxFrameRate;
	Uint64 gameTickCount = SDL_GetPerformanceCounter();
	Uint64 ticksPerSecond = SDL_GetPerformanceFrequency();
	Uint64 ticksElapsed = gameTickCount - lastGameTickCount;
	lastGameTickCount = gameTickCount;
	framerateAccumulatedTicks += ticksElapsed;

	const float accumulatedSeconds = framerateAccumulatedTicks / (float)ticksPerSecond;
	const float diff = desiredFrameSeconds - accumulatedSeconds;

    static ConsoleVariable<bool> allowSleep("/timer_sleep_enabled", true,
        "allow main thread to sleep between ticks (saves power)");
	if ( diff >= 0.f )
	{
	    // we have not passed a full frame, so we must delay.
        if ( *allowSleep && sleep )
        {
            // sleep a fraction of the remaining time.
            // This saves power if you're running on battery.
#if 1
			auto microseconds = std::chrono::microseconds((Uint64)(diff * 1000000));
			preciseSleep(microseconds.count() / 1e6);
#else
            static ConsoleVariable<float> sleepLimit("/timer_sleep_limit", 0.001f);
            static ConsoleVariable<float> sleepFactor("/timer_sleep_factor", 0.97f);
            if ( diff >= *sleepLimit )
            {
				std::this_thread::sleep_for(std::chrono::microseconds((Uint64)(diff * 1000000 * (*sleepFactor))));
            }
#endif
        }
		return true;
	}
	else
	{
		if ( resetAccumulator )
		{
			framerateAccumulatedTicks = 0;
		}
		return false;
	}
}

void ingameHud()
{
	for ( int player = 0; player < MAXPLAYERS; ++player )
	{
		if ( !players[player]->isLocalPlayer() )
		{
			continue;
		}
		if ( players[player]->isLocalPlayerAlive() )
		{
			players[player]->bControlEnabled = true;
		}
		else if ( players[player]->ghost.isActive() )
		{
			players[player]->bControlEnabled = true;
		}
#ifdef USE_IMGUI
		if ( ImGui_t::isInit )
		{
			if ( ImGui_t::disablePlayerControl && player == clientnum )
			{
				players[player]->bControlEnabled = false;
			}
		}
#endif
		bool& bControlEnabled = players[player]->bControlEnabled;

	    Input& input = Input::inputs[player];

        // various UI keybinds
        if ( !gamePaused && bControlEnabled && !players[player]->usingCommand() )
        {
	        // toggle minimap
		    // player not needed to be alive
            // map window bind
			if ( players[player]->hotbar.faceMenuButtonHeld == Player::Hotbar_t::GROUP_NONE )
			{
				if ( players[player]->bUseCompactGUIHeight() && players[player]->bUseCompactGUIWidth() )
				{
					players[player]->minimap.bExpandPromptEnabled = players[player]->shootmode || players[player]->gui_mode == GUI_MODE_NONE;
					if ( players[player]->worldUI.isEnabled()
						&& players[player]->worldUI.bTooltipInView
						&& players[player]->worldUI.tooltipsInRange.size() > 1 )
					{
						std::string expandBinding = input.binding("Toggle Minimap");
						std::string cycleNextBinding = input.binding("Interact Tooltip Next");
						std::string cyclePrevBinding = input.binding("Interact Tooltip Prev");
						if ( expandBinding == cycleNextBinding
							|| expandBinding == cyclePrevBinding )
						{
							players[player]->minimap.bExpandPromptEnabled = false;
						}
					}
					if ( input.consumeBinaryToggle("Toggle Minimap") && players[player]->minimap.bExpandPromptEnabled )
					{
						openMapWindow(player);
					}
				}
				else if ( players[player]->shootmode && players[player]->minimap.bExpandPromptEnabled
					&& input.consumeBinaryToggle("Toggle Minimap") )
				{
					openMinimap(player);
				}
			}

			if ( input.consumeBinaryToggle("Open Map") )
            {
                openMapWindow(player);
            }

            // log window bind
            if ( input.consumeBinaryToggle("Open Log") )
            {
                openLogWindow(player);
            }
        }

		// inventory interface
		// player not needed to be alive
		if ( players[player]->isLocalPlayer() && !players[player]->usingCommand() && input.consumeBinaryToggle("Character Status")
			&& !gamePaused
			&& bControlEnabled )
		{
			if ( players[player]->shootmode )
			{
				players[player]->openStatusScreen(GUI_MODE_INVENTORY, INVENTORY_MODE_ITEM);
				//Player::soundStatusOpen();
			}
			else
			{
				players[player]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_CLOSE_ALL);
				//Player::soundStatusClose();
			}
		}

		// if useItemDropdownOnGamepad, then 'b' will close inventory, with a 'couple' checks..
		if ( players[player]->isLocalPlayer() 
			&& !players[player]->shootmode
			&& (players[player]->inventoryUI.useItemDropdownOnGamepad != Player::Inventory_t::GAMEPAD_DROPDOWN_DISABLE)
			&& !inputs.getVirtualMouse(player)->draw_cursor
			&& !players[player]->usingCommand() && input.binaryToggle("MenuCancel")
			&& !players[player]->GUI.isDropdownActive()
			&& players[player]->GUI.bModuleAccessibleWithMouse(players[player]->GUI.activeModule)
			&& !inputs.getUIInteraction(player)->selectedItem
			&& !gamePaused
			&& bControlEnabled
			&& players[player]->gui_mode == GUI_MODE_INVENTORY
			&& players[player]->inventory_mode == INVENTORY_MODE_ITEM
			&& !players[player]->inventoryUI.chestGUI.bOpen
			&& !players[player]->minimap.mapWindow
			&& !players[player]->messageZone.logWindow
			&& !players[player]->shopGUI.bOpen
			&& !GenericGUI[player].isGUIOpen() )
		{
			if ( (players[player]->inventoryUI.isInteractable && players[player]->GUI.activeModule == Player::GUI_t::MODULE_INVENTORY)
				|| players[player]->GUI.activeModule == Player::GUI_t::MODULE_HOTBAR
				|| players[player]->GUI.activeModule == Player::GUI_t::MODULE_CHARACTERSHEET )
			{
				players[player]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_CLOSE_ALL);
				input.consumeBinaryToggle("MenuCancel");
				input.consumeBindingsSharedWithBinding("MenuCancel");
                if (players[player]->hotbar.useHotbarFaceMenu)
                {
                    input.consumeBinaryToggle("Hotbar Left");
                    input.consumeBinaryToggle("Hotbar Up / Select");
                    input.consumeBinaryToggle("Hotbar Right");
                }
				Player::soundCancel();
			}
		}

		// spell list
		// player not needed to be alive
		if ( players[player]->isLocalPlayer() && !players[player]->usingCommand() && input.consumeBinaryToggle("Spell List")
			&& !gamePaused
			&& bControlEnabled )   //TODO: Move to function in interface or something?
		{
			if ( input.input("Spell List").isBindingUsingGamepad() )
			{
				// no action, gamepad doesn't use this binding
			}
			// no dropdowns/no selected item, if controller, has to be in inventory/hotbar + !shootmode
			else if ( !inputs.getUIInteraction(player)->selectedItem && !players[player]->GUI.isDropdownActive()
				&& !GenericGUI[player].isGUIOpen()
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
		if ( players[player]->isLocalPlayerAlive() 
			&& !gamePaused )
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
			else if ( !players[player]->usingCommand() && bControlEnabled
				&& ((shootmode && inputs.hasController(player)) || (!inputs.hasController(player) && inputs.bPlayerUsingKeyboardControl(player))) )
			{
				bool hotbarFaceMenuOpen = players[player]->hotbar.faceMenuButtonHeld != Player::Hotbar_t::GROUP_NONE;
				bool castMemorizedSpell = input.binaryToggle("Cast Spell");
				bool castSpellbook = (hasSpellbook && input.binaryToggle("Defend"));

			    if (tryHotbarQuickCast || castMemorizedSpell || castSpellbook )
			    {
				    allowCasting = true;
				    if ( tryHotbarQuickCast == false )
					{
						if ( hotbarFaceMenuOpen )
						{
							allowCasting = false;
						}
						if ( !shootmode ) // check we dont conflict with system bindings
						{
							if ( players[player]->messageZone.logWindow || players[player]->minimap.mapWindow 
								|| FollowerMenu[player].followerMenuIsOpen() || CalloutMenu[player].calloutMenuIsOpen() )
							{
								allowCasting = false;
							}
							else
							{
								if ( castMemorizedSpell )
								{
									if ( input.bindingIsSharedWithKeyboardSystemBinding("Cast Spell") )
									{
										allowCasting = false;
									}
								}
								if ( castSpellbook )
								{
									if ( input.bindingIsSharedWithKeyboardSystemBinding("Defend") )
									{
										allowCasting = false;
										input.consumeBinaryToggle("Defend");
									}
								}
							}
						}
						else
						{
							if ( FollowerMenu[player].followerMenuIsOpen() || CalloutMenu[player].calloutMenuIsOpen() )
							{
								if ( castMemorizedSpell )
								{
									if ( input.bindingIsSharedWithKeyboardSystemBinding("Cast Spell") )
									{
										allowCasting = false;
									}
								}
								if ( castSpellbook )
								{
									allowCasting = false;
									input.consumeBinaryToggle("Defend");
								}
							}
						}
				    }
					else
					{
						if ( !players[player]->hotbar.faceMenuQuickCastEnabled && !tryInventoryQuickCast )
						{
							allowCasting = false;
						}
					}

				    if ( allowCasting && castSpellbook && players[player] && players[player]->entity )
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

						if ( allowCasting && players[player]->entity->isBlind() )
						{
							messagePlayer(player, MESSAGE_EQUIPMENT | MESSAGE_STATUS, Language::get(3863)); // prevent casting of spell.
							input.consumeBinaryToggle("Defend");
							allowCasting = false;
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
							messagePlayer(player, MESSAGE_MISC, Language::get(2999)); // prevent casting of spell.
						}
						else
						{
							if ( achievementBrawlerMode && players[player]->magic.selectedSpell() )
							{
								messagePlayer(player, MESSAGE_MISC, Language::get(2998)); // notify no longer eligible for achievement but still cast.
							}
							if ( tryInventoryQuickCast )
							{
								castSpellInit(players[player]->entity->getUID(), players[player]->magic.quickCastSpell(), false);
							}
							else if ( hasSpellbook && input.consumeBinaryToggle("Defend") )
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
						else if ( hasSpellbook && input.consumeBinaryToggle("Defend") )
						{
							castSpellInit(players[player]->entity->getUID(), getSpellFromID(getSpellIDFromSpellbook(stats[player]->shield->type)), true);
						}
						else
						{
							castSpellInit(players[player]->entity->getUID(), players[player]->magic.selectedSpell(), false);
						}
					}
				}
				input.consumeBinaryToggle("Defend");
			}
			input.consumeBinaryToggle("Cast Spell");
		}
		players[player]->magic.resetQuickCastSpell();

		bool worldUIBlocksFollowerCycle = (
				players[player]->worldUI.isEnabled()
				&& players[player]->worldUI.bTooltipInView
				&& players[player]->worldUI.tooltipsInRange.size() > 1);
		if ( worldUIBlocksFollowerCycle )
		{
			std::string cycleNPCbinding = input.binding("Cycle NPCs");
			if ( cycleNPCbinding != input.binding("Interact Tooltip Next")
				&& cycleNPCbinding != input.binding("Interact Tooltip Prev") )
			{
				worldUIBlocksFollowerCycle = false;
			}
		}
		players[player]->hud.followerDisplay.bCycleNextDisabled = (worldUIBlocksFollowerCycle && players[player]->shootmode);
		bool allowCycle = true;
		if ( CalloutMenu[player].calloutMenuIsOpen() && !players[player]->shootmode )
		{
			players[player]->hud.followerDisplay.bCycleNextDisabled = true;
			allowCycle = false;
		}
		else if ( FollowerMenu[player].followerMenuIsOpen() )
		{
			std::string cycleNPCbinding = input.binding("Cycle NPCs");
			if ( cycleNPCbinding == input.binding("MenuCancel") )
			{
				allowCycle = false;
				if ( !players[player]->shootmode )
				{
					players[player]->hud.followerDisplay.bCycleNextDisabled = true;
				}
			}
		}

		if ( !players[player]->usingCommand() && input.consumeBinaryToggle("Cycle NPCs")
			&& !gamePaused
			&& bControlEnabled )
		{
			if ( (!worldUIBlocksFollowerCycle && players[player]->shootmode) || FollowerMenu[player].followerMenuIsOpen() )
			{
				if ( allowCycle )
				{
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
			if ( players[player]->signGUI.bSignOpen )
			{
				players[player]->signGUI.closeSignGUI();
			}
			if ( players[player]->skillSheet.bSkillSheetOpen )
			{
				players[player]->skillSheet.closeSkillSheet();
			}
			players[player]->hud.closeStatusFxWindow();

			if ( capture_mouse && !gamePaused )
			{
				if ( inputs.bPlayerUsingKeyboardControl(player) )
				{
                    // fix for macOS: put mouse back in window before recapturing mouse
                    if (EnableMouseCapture) {
                        int mouse_x, mouse_y;
                        SDL_GetGlobalMouseState(&mouse_x, &mouse_y);
                        int x, y, w, h;
                        SDL_GetWindowPosition(screen, &x, &y);
                        SDL_GetWindowSize(screen, &w, &h);
                        if (mouse_x < x || mouse_x >= x + w ||
                            mouse_y < y || mouse_y >= y + h) {
                            SDL_WarpMouseInWindow(screen, w/2, h/2);
                        }
                    }
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
		if ( !players[player]->isLocalPlayer() )
		{
			continue;
		}
		players[player]->hud.processHUD();
		players[player]->messageZone.processChatbox();
		updateSkillUpFrame(player);
		updateLevelUpFrame(player);
		players[player]->inventoryUI.updateSelectedItemAnimation();
		players[player]->inventoryUI.updateInventoryItemTooltip();
		players[player]->inventoryUI.updateInventoryMiscTooltip();
		players[player]->hotbar.processHotbar();
		players[player]->inventoryUI.processInventory();
		GenericGUI[player].tinkerGUI.updateTinkerMenu();
		GenericGUI[player].alchemyGUI.updateAlchemyMenu();
		GenericGUI[player].assistShrineGUI.updateAssistShrine();
		GenericGUI[player].featherGUI.updateFeatherMenu();
		GenericGUI[player].itemfxGUI.updateItemEffectMenu();
		players[player]->GUI.dropdownMenu.process();
		players[player]->characterSheet.processCharacterSheet();
		players[player]->hud.updateStatusEffectFocusedWindow();
		players[player]->messageZone.processLogFrame();
		players[player]->minimap.processMapFrame();
		players[player]->skillSheet.processSkillSheet();
		players[player]->signGUI.updateSignGUI();
		players[player]->hud.updateStatusEffectTooltip(); // to create a tooltip in this order to draw over previous elements
		CalloutRadialMenu::drawCallouts(player);
		players[player]->inventoryUI.updateItemContextMenuClickFrame();
		players[player]->GUI.handleModuleNavigation(false);
		players[player]->inventoryUI.updateCursor();
		players[player]->hotbar.updateCursor();
		players[player]->hud.updateCursor();
		players[player]->hud.updateMinotaurWarning();
		//drawSkillsSheet(player);
		if ( !gamePaused )
		{
			if ( !nohud )
			{
				drawStatusNew(player);
			}
			//drawSustainedSpells(player);
		}

		// inventory and stats
		if ( players[player]->shootmode == false )
		{
			if ( players[player]->gui_mode == GUI_MODE_INVENTORY
				|| players[player]->gui_mode == GUI_MODE_SHOP )
			{
				//updateCharacterSheet(player);
				GenericGUI[player].updateGUI();
				players[player]->bookGUI.updateBookGUI();
				//updateRightSidebar(); -- 06/12/20 we don't use this but it still somehow displays stuff :D

			}
			//else if ( players[player]->gui_mode == GUI_MODE_MAGIC )
			//{
			//	updateCharacterSheet(player);
			//	//updateMagicGUI();
			//}
			//else if ( players[player]->gui_mode == GUI_MODE_SHOP
			//	|| )
			//{
			//	//updateCharacterSheet(player);
			//	//updateShopWindow(player);
			//}
		}

		static ConsoleVariable<bool> cvar_debugmouse("/debugmouse", false);
		if ( *cvar_debugmouse )
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
			if ( players[player]->entity )
			{
				printTextFormatted(font8x8_bmp, x, y + 124, "%.5f | %.5f", players[player]->entity->fskill[6], players[player]->entity->fskill[7]);
			}
		}
	}

	DebugStats.t9GUI = std::chrono::high_resolution_clock::now();

	UIToastNotificationManager.drawNotifications(MainMenu::isCutsceneActive(), true); // draw this before the cursors
    static ConsoleVariable<bool> cvar_debugVMouse("/debug_virtual_mouse", false);

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
                    
					if ( *cvar_debugVMouse )
					{
						// debug for controllers
                        const float factorX = (float)xres / Frame::virtualScreenX;
                        const float factorY = (float)yres / Frame::virtualScreenY;
						auto cursor = Image::get("*#images/system/cursor_hand.png");
						if ( enableDebugKeys && keystatus[SDLK_j] )
						{
							cursor = Image::get("*#images/ui/Crosshairs/cursor_xB.png");
						}

                        const int w = cursor->getWidth() * factorX;
                        const int h = cursor->getHeight() * factorY;
						pos.x = inputs.getVirtualMouse(player)->x - (w / 7) - w / 2;
						pos.y = inputs.getVirtualMouse(player)->y - (h / 7) - h / 2;
						pos.x += 4;
						pos.y += 4;
						pos.w = w;
						pos.h = h;
						cursor->drawColor(nullptr, pos, SDL_Rect{ 0, 0, xres, yres }, 0xFF0000FF);
					}
				}
			}
			else if ( players[player]->isLocalPlayer() && followerMenu.selectMoveTo &&
				(followerMenu.optionSelected == ALLY_CMD_MOVETO_SELECT
					|| followerMenu.optionSelected == ALLY_CMD_ATTACK_SELECT) )
			{
				// note this currently does not get hit, moveto_select etc is shootmode
				/*auto cursor = Image::get("images/system/cursor_hand.png");
				pos.x = inputs.getMouse(player, Inputs::X) - cursor->getWidth() / 2;
				pos.y = inputs.getMouse(player, Inputs::Y) - cursor->getHeight() / 2;
				pos.x += 4;
				pos.y += 4;
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
						ttfPrintTextFormatted(ttf12, pos.x + 24, pos.y + 24, Language::get(3650));
					}
					else
					{
						ttfPrintTextFormatted(ttf12, pos.x + 24, pos.y + 24, Language::get(3039));
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
								ttfPrintTextFormatted(ttf12, pos.x + 24, pos.y + 24, Language::get(4041)); // "Interact with..."
							}
							else
							{
								ttfPrintTextFormatted(ttf12, pos.x + 24, pos.y + 24, Language::get(4042)); // "Attack..."
							}
						}
						else
						{
							ttfPrintTextFormatted(ttf12, pos.x + 24, pos.y + 24, Language::get(4041)); // "Interact with..."
						}
					}
					else
					{
						ttfPrintTextFormatted(ttf12, pos.x + 24, pos.y + 24, "%s", followerMenu.interactText);
					}
				}*/
			}
			else if ( inputs.getVirtualMouse(player)->draw_cursor )
			{
#ifndef NINTENDO
                const float factorX = (float)xres / Frame::virtualScreenX;
                const float factorY = (float)yres / Frame::virtualScreenY;
				auto cursor = Image::get("*#images/system/cursor_hand.png");
				real_t& mouseAnim = inputs.getVirtualMouse(player)->mouseAnimationPercent;
				if ( Input::inputs[player].binary("MenuLeftClick") )
				{
					mouseAnim = .5;
				}
				if ( mouseAnim > .25 )
				{
					cursor = Image::get("*#images/system/cursor_hand2.png");
				}
				if ( mouseAnim > 0.0 )
				{
					mouseAnim -= .05 * getFPSScale(144.0);
				}
				if ( enableDebugKeys && keystatus[SDLK_j] )
				{
					cursor = Image::get("*#images/ui/Crosshairs/cursor_xB.png");
				}
                const int w = cursor->getWidth() * factorX;
                const int h = cursor->getHeight() * factorY;
                pos.x = inputs.getMouse(player, Inputs::X) - (mouseAnim * w / 7) - w / 2;
                pos.y = inputs.getMouse(player, Inputs::Y) - (mouseAnim * h / 7) - h / 2;
                pos.x += 4;
                pos.y += 4;
                pos.w = w;
                pos.h = h;
				if ( inputs.getUIInteraction(player)->itemMenuOpen && inputs.getUIInteraction(player)->itemMenuFromHotbar )
				{
					// adjust cursor to match selection
					pos.y -= inputs.getUIInteraction(player)->itemMenuOffsetDetectionY;
				}
				cursor->draw(nullptr, pos, SDL_Rect{0, 0, xres, yres});
#endif
			}
			else
			{
#ifndef NDEBUG
				// debug for controllers
				if ( *cvar_debugVMouse )
				{
                    const float factorX = (float)xres / Frame::virtualScreenX;
                    const float factorY = (float)yres / Frame::virtualScreenY;
					auto cursor = Image::get("*#images/system/cursor_hand.png");
					if ( enableDebugKeys && keystatus[SDLK_j] )
					{
						cursor = Image::get("*#images/ui/Crosshairs/cursor_xB.png");
					}
                    const int w = cursor->getWidth() * factorX;
                    const int h = cursor->getHeight() * factorY;
                    pos.x = inputs.getVirtualMouse(player)->x - (w / 7) - w / 2;
                    pos.y = inputs.getVirtualMouse(player)->y - (h / 7) - h / 2;
                    pos.x += 4;
                    pos.y += 4;
                    pos.w = w;
                    pos.h = h;
					cursor->drawColor(nullptr, pos, SDL_Rect{ 0, 0, xres, yres }, 0xFF0000FF);
				}
#endif
			}
		}
		else
		{
			real_t& mouseAnim = inputs.getVirtualMouse(player)->mouseAnimationPercent;
			mouseAnim = 0.0;
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

void drawAllPlayerCameras() {
	DebugStats.drawWorldT1 = std::chrono::high_resolution_clock::now();
	int playercount = 0;
	for (int c = 0; c < MAXPLAYERS; ++c)
	{
		if (!client_disconnected[c] && players[c]->isLocalPlayer())
		{
			++playercount;
		}
	}
	Uint32 oldFov = ::fov;
	if ( playercount == 2 )
	{
		if ( *MainMenu::vertical_splitscreen )
		{
			::fov += 15;
		}
		else
		{
			/*if ( ::fov >= 15 )
			{
				::fov -= 15;
			}
			else
			{
				::fov = 0;
			}*/
		}
	}

	if (playercount >= 1)
	{
        // drunkenness spinning
	    double cosspin = cos(ticks % 360 * PI / 180.f) * 0.25;
	    double sinspin = sin(ticks % 360 * PI / 180.f) * 0.25;
        
        // setup a graphics frame
        beginGraphics();

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
		    auto& globalLightModifier = players[c]->camera().globalLightModifier;
		    auto& globalLightModifierEntities = players[c]->camera().globalLightModifierEntities;
		    auto& globalLightModifierActive = players[c]->camera().globalLightModifierActive;
			if (shaking && players[c] && players[c]->entity && !gamePaused)
			{
				camera.ang += cosspin * drunkextend[c];
				camera.vang += sinspin * drunkextend[c];
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

					players[c]->ghost.handleGhostCameraBobbing(true);
					players[c]->ghost.handleGhostMovement(true);
					players[c]->ghost.handleGhostCameraUpdate(true);
					//messagePlayer(0, "%3.2f | %3.2f", players[c]->entity->yaw, oldYaw);
				}
			}
   
            // activate ghost fog (if necessary)
            if (players[c]->ghost.isActive()) {
                *cvar_hdrBrightness = {0.9f, 0.9f, 1.2f, 1.0f};
                *cvar_fogColor = {0.7f, 0.7f, 1.1f, 0.25f};
                *cvar_fogDistance = 350.f;
            }

			// do occlusion culling from the perspective of this camera
			DebugStats.drawWorldT2 = std::chrono::high_resolution_clock::now();
			occlusionCulling(map, camera);
			glBeginCamera(&camera, true, map);

			// shared minimap progress
			if ( !splitscreen/*gameplayCustomManager.inUse() && gameplayCustomManager.minimapShareProgress && !splitscreen*/ )
			{
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( i != clientnum && players[i] && Player::getPlayerInteractEntity(i) )
					{
						if ( !players[i]->entity || (players[i]->entity && !players[i]->entity->isBlind()) )
						{
							if ( players[i]->entity && players[i]->entity->ticks < TICKS_PER_SECOND * 1 )
							{
								continue; // don't share for first x ticks due to level change warping
							}

							real_t x = camera.x;
							real_t y = camera.y;
							real_t ang = camera.ang;

							camera.x = Player::getPlayerInteractEntity(i)->x / 16.0;
							camera.y = Player::getPlayerInteractEntity(i)->y / 16.0;
							camera.ang = Player::getPlayerInteractEntity(i)->yaw;
							raycast(camera, minimap, false); // update minimap from other players' perspectives, player or ghost
							camera.x = x;
							camera.y = y;
							camera.ang = ang;
						}
					}
				}
			}

			if ( players[c] && players[c]->entity )
			{
				if ( players[c]->entity->isBlind() )
				{
					if ( globalLightModifierActive == GLOBAL_LIGHT_MODIFIER_STOPPED
						|| (globalLightModifierActive == GLOBAL_LIGHT_MODIFIER_DISSIPATING && globalLightModifier < 1.f) )
					{
						globalLightModifierActive = GLOBAL_LIGHT_MODIFIER_INUSE;
						globalLightModifier = 0.f;
						globalLightModifierEntities = 0.f;
						if ( stats[c]->mask && stats[c]->mask->type == TOOL_BLINDFOLD_TELEPATHY )
						{
							for ( node_t* mapNode = map.creatures->first; mapNode != nullptr; mapNode = mapNode->next )
							{
								Entity* mapCreature = (Entity*)mapNode->element;
								if ( mapCreature && !intro )
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
					globalLightModifierEntities = std::min(telepathyLimit / 255.0, globalLightModifierEntities + (0.2 / 255.0));
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
					globalLightModifierEntities = 0.f;
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
				DebugStats.drawWorldT3 = std::chrono::high_resolution_clock::now();
				if ( !players[c]->entity->isBlind() )
				{
				    raycast(camera, minimap, true); // update minimap
				}
				DebugStats.drawWorldT4 = std::chrono::high_resolution_clock::now();
				glDrawWorld(&camera, REALCOLORS);
			}
			else
			{
				// undo blindness effects
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
				globalLightModifierEntities = 0.f;
				if ( globalLightModifier < 1.f )
				{
					globalLightModifier += 0.01;
				}
				else
				{
					globalLightModifier = 1.01;
					globalLightModifierActive = GLOBAL_LIGHT_MODIFIER_STOPPED;
				}

				if ( players[c] && players[c]->ghost.isActive() )
				{
					raycast(camera, minimap, false); // update minimap for ghost
				}

			    // player is dead, spectate
				glDrawWorld(&camera, REALCOLORS);
			}

			DebugStats.drawWorldT5 = std::chrono::high_resolution_clock::now();
			drawEntities3D(&camera, REALCOLORS);
			glEndCamera(&camera, true, map);
            
            // undo ghost fog
            if (players[c]->ghost.isActive()) {
                *cvar_hdrBrightness = {1.0f, 1.0f, 1.0f, 1.0f};
                *cvar_fogColor = {0.0f, 0.0f, 0.0f, 1.0f};
                *cvar_fogDistance = 0.0f;
            }

			if (shaking && players[c] && players[c]->entity && !gamePaused)
			{
				camera.ang -= cosspin * drunkextend[c];
				camera.vang -= sinspin * drunkextend[c];
			}

			auto& cvars = cameravars[c];
			camera.ang -= cvars.shakex2;
			camera.vang -= cvars.shakey2 / 200.0;
		}
	}
	DebugStats.drawWorldT6 = std::chrono::high_resolution_clock::now();
	::fov = oldFov;
}

/*-------------------------------------------------------------------------------

	main

	Initializes game resources, harbors main game loop, and cleans up
	afterwards

-------------------------------------------------------------------------------*/

static void doConsoleCommands() {
	Input& input = Input::inputs[clientnum]; // commands - uses local clientnum only
	const bool controlEnabled = players[clientnum]->bControlEnabled && !movie;

#if defined(NINTENDO) && defined(NINTENDO_DEBUG)
	// activate console
	if (input.binaryToggle("ConsoleCommand1") &&
		input.binaryToggle("ConsoleCommand2") &&
		input.binaryToggle("ConsoleCommand3"))
	{
		input.consumeBinary("ConsoleCommand1");
		input.consumeBinary("ConsoleCommand2");
		input.consumeBinary("ConsoleCommand3");
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
#else
	// check for input to start/stop a command (enter / return keystroke, or chat binding)
	bool confirm = false;
	if (controlEnabled) {
		if (input.getPlayerControlType() != Input::playerControlType_t::PLAYER_CONTROLLED_BY_KEYBOARD) {
            if (command || !intro ) {
                if (keystatus[SDLK_RETURN]) {
                    keystatus[SDLK_RETURN] = 0;
                    confirm = true;
                }
                if (Input::keys[SDLK_RETURN]) {
                    Input::keys[SDLK_RETURN] = 0;
                    confirm = true;
                }
            }
		}
		if (input.consumeBinaryToggle("Chat")) {
			input.consumeBindingsSharedWithBinding("Chat");
            if (command || !intro ) {
                confirm = true;
            }
		}
		if ( confirm && !command )
		{
			if ( MainMenu::main_menu_frame )
			{
				if ( auto compendium = MainMenu::main_menu_frame->findFrame("compendium") )
				{
					confirm = false; // stop menuStart triggering console commands
				}
			}
		}

		if (input.consumeBinaryToggle("Console Command")) {
			input.consumeBindingsSharedWithBinding("Console Command");
			if (!command) {
				confirm = true;
			}
		}

	}

	if (confirm && !command) // begin a command
	{
		// start typing a command
		if (input.binary("Console Command")) {
			strcpy(command_str, "/");
		}
		else {
			strcpy(command_str, "");
		}
		inputstr = command_str;
		inputlen = 127;
		command = true;
		cursorflash = ticks;
		if (!SDL_IsTextInputActive()) {
			SDL_StartTextInput();
		}

		// clear follower menu entities.
		for (int i = 0; i < MAXPLAYERS; ++i) {
			if (players[i]->isLocalPlayer() && inputs.bPlayerUsingKeyboardControl(i)) {
				FollowerMenu[i].closeFollowerMenuGUI();
				CalloutMenu[i].closeCalloutMenuGUI();
			}
		}
	}
	else if (command) // finish a command
	{
		// check for cancel keystroke (always ESC)
		if (keystatus[SDLK_ESCAPE])
		{
			keystatus[SDLK_ESCAPE] = 0;
			chosen_command = nullptr;
			command = false;
		}

		// check for forced cancel
		if (!controlEnabled)
		{
			if (SDL_IsTextInputActive()) {
				SDL_StopTextInput();
			}
			inputstr = nullptr;
			inputlen = 0;
			chosen_command = nullptr;
			command = false;
		}

		// set player issuing command
		int commandPlayer = clientnum;
		if (multiplayer == SINGLE) {
			for (int i = 0; i < MAXPLAYERS; ++i) {
				if (inputs.bPlayerUsingKeyboardControl(i)) {
					commandPlayer = i;
					break;
				}
			}
		}

		// set inputstr
		if (!SDL_IsTextInputActive()) {
			SDL_StartTextInput();
		}
		inputstr = command_str;
		inputlen = 127;

		// issue command
		if (confirm)
		{
			// no longer accepting input
			if (SDL_IsTextInputActive()) {
				SDL_StopTextInput();
			}
			inputstr = nullptr;
			inputlen = 0;
			command = false;

			// sanitize strings (remove format codes)
			strncpy(command_str, messageSanitizePercentSign(command_str, nullptr).c_str(), 127);

			// process string
			if (command_str[0] == '/') // slash invokes a command procedure
			{
				messagePlayer(commandPlayer, MESSAGE_MISC, command_str);
				consoleCommand(command_str);
			}
			else if (!intro) // can't send messages in multiplayer
			{
				if (multiplayer == CLIENT) // send message as a client
				{
					if (strcmp(command_str, ""))
					{
						char chatstring[256];
						strcpy(chatstring, Language::get(739));
						strcat(chatstring, command_str);
						Uint32 color = playerColor(commandPlayer, colorblind_lobby, false);
						if (messagePlayerColor(commandPlayer, MESSAGE_CHAT, color, chatstring)) {
							playSound(Message::CHAT_MESSAGE_SFX, 64);
						}

						// send message to server
						if (net_packet && net_packet->data) {
							strcpy((char*)net_packet->data, "MSGS");
							net_packet->data[4] = commandPlayer;
							SDLNet_Write32(color, &net_packet->data[5]);
							strcpy((char*)(&net_packet->data[9]), command_str);
							net_packet->address.host = net_server.host;
							net_packet->address.port = net_server.port;
							net_packet->len = 9 + strlen(command_str) + 1;
							sendPacketSafe(net_sock, -1, net_packet, 0);
						}
					}
					else
					{
						strcpy(command_str, "");
					}
				}
				else // servers (or singleplayer) broadcast typed messages
				{
					if (strcmp(command_str, ""))
					{
						char chatstring[256];
						strcpy(chatstring, Language::get(739));
						strcat(chatstring, command_str);
						Uint32 color = playerColor(commandPlayer, colorblind_lobby, false);
						if (messagePlayerColor(commandPlayer, MESSAGE_CHAT, color, chatstring)) {
							playSound(Message::CHAT_MESSAGE_SFX, 64);
						}
						if (multiplayer == SERVER)
						{
							// send message to all clients
							for (int c = 1; c < MAXPLAYERS; c++)
							{
								if (client_disconnected[c] || players[c]->isLocalPlayer())
								{
									continue;
								}
								if (net_packet && net_packet->data) {
									strcpy((char*)net_packet->data, "MSGS");
									// strncpy() does not copy N bytes if a terminating null is encountered first
									// see http://www.cplusplus.com/reference/cstring/strncpy/
									// see https://en.cppreference.com/w/c/string/byte/strncpy
									// GCC throws a warning (intended) when the length argument to strncpy() in any
									// way depends on strlen(src) to discourage this (and related) construct(s).

									strncpy(chatstring, stats[0]->name, 22);
									chatstring[std::min<size_t>(strlen(stats[0]->name), 22)] = 0; //TODO: Why are size_t and int being compared?
									strcat(chatstring, ": ");
									strcat(chatstring, command_str);
									SDLNet_Write32(color, &net_packet->data[4]);
									SDLNet_Write32((Uint32)MESSAGE_CHAT, &net_packet->data[8]);
									strcpy((char*)(&net_packet->data[12]), chatstring);
									net_packet->address.host = net_clients[c - 1].host;
									net_packet->address.port = net_clients[c - 1].port;
									net_packet->len = 12 + strlen(chatstring) + 1;
									sendPacketSafe(net_sock, -1, net_packet, c - 1);
								}
							}
						}
					}
					else
					{
						strcpy(command_str, "");
					}
				}
			}

			// save this in the command history
			if (command_str[0]) {
				saveCommand(command_str);
			}
			chosen_command = NULL;
		}
	}
	else
	{
		// make sure not to create text input events
		if (inputstr == command_str) {
			inputstr = nullptr;
		}
		if (!inputstr && SDL_IsTextInputActive()) {
			SDL_StopTextInput();
		}
	}
#endif // NINTENDO
}

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
#ifdef STEAMWORKS
			    cmd_line += argv[c];
			    cmd_line += " ";
#endif
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
        
        // init sdl
        Uint32 init_flags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;
        init_flags |= SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC;
        if (SDL_Init(init_flags) == -1)
        {
            printlog("failed to initialize SDL: %s\n", SDL_GetError());
            return 1;
        }

        // load game config
		Input::defaultBindings();
        MainMenu::settingsReset();
        MainMenu::settingsApply();
		bool load_successful = MainMenu::settingsLoad();
		if ( load_successful ) {
			MainMenu::settingsApply();
		}
		else {
			skipintro = false;
		}

		// initialize map
		map.tiles = nullptr;
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
									"at https://www.baronygame.com/ for support.",
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
					"Alternatively, contact us through our website at https://www.baronygame.com/ for support.",
					screen);
			}
#else
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Uh oh",
									"Barony has encountered a critical error and cannot start.\n\n"
									"Please check the log.txt file in the game directory for additional info,\n"
									"or contact us through our website at https://www.baronygame.com/ for support.",
									screen);
#endif
			deinitApp();
			exit(c);
		}

		MainMenu::randomizeUsername();

		// init message
		printlog("Barony version: %s\n", VERSION);
		char buffer[32];
        getTimeAndDateFormatted(getTime(), buffer, sizeof(buffer));
		printlog("Launch time: %s\n", buffer);

		if ( (c = initGame()) )
		{
			printlog("Critical error in initGame: %d\n", c);
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Uh oh",
			                         "Barony has encountered a critical error and cannot start.\n\n"
			                         "Please check the log.txt file in the game directory for additional info,\n"
			                         "or contact us through our website at https://www.baronygame.com/ for support.",
			                         screen);
			deinitGame();
			deinitApp();
			exit(c);
		}
		initialized = true;

		// initialize player conducts
		setDefaultPlayerConducts();

#ifdef NINTENDO
		if (!nxIsHandheldMode()) {
			nxAssignControllers(1, 1, true, false, true, false, nullptr);
		}
		for (int c = 0; c < 4; ++c) {
			game_controllers[c].open(0, c); // first parameter is not used by Nintendo.
			bindControllerToPlayer(c, c);
		}
		//inputs.setPlayerIDAllowedKeyboard(-1);
#endif

		// play splash sound
#ifdef MUSIC
		playMusic(splashmusic, false, false, false);
#endif

		int indev_timer = 0;

		// main loop

		printlog("running main loop.\n");
		while (mainloop)
		{
			Frame::numFindFrameCalls = 0;
			// record the time at the start of this cycle
			lastGameTickCount = SDL_GetPerformanceCounter();
			DebugStats.t1StartLoop = std::chrono::high_resolution_clock::now();

#ifdef USE_IMGUI
			if ( ImGui_t::queueInit )
			{
				ImGui_t::queueInit = false;
				if ( !ImGui_t::isInit )
				{
					ImGui_t::init();
				}
			}
			if ( ImGui_t::queueDeinit )
			{
				ImGui_t::queueDeinit = false;
				if ( ImGui_t::isInit )
				{
					ImGui_t::deinit();
				}
			}
#endif

			doConsoleCommands();

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
			bool ranframes = handleEvents();
			DebugStats.t2PostEvents = std::chrono::high_resolution_clock::now();
#ifdef DEBUG_EVENT_TIMERS
			real_t accum = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(DebugStats.t2PostEvents - DebugStats.t21PostHandleMessages).count();
			if ( accum > 5.0 )
			{
				printlog("Long tick time: %f", accum);
			}
#endif
			// handle steam callbacks
#ifdef STEAMWORKS
			if ( g_SteamLeaderboards )
			{
				g_SteamLeaderboards->ProcessLeaderboardUpload();
			}
			SteamAPI_RunCallbacks();
#endif
#ifdef USE_PLAYFAB
			PlayFab::PlayFabClientAPI::Update();
			playfabUser.update();
#endif
#ifdef USE_EOS
			if (EOS.PlatformHandle) {
				EOS_Platform_Tick(EOS.PlatformHandle);
			}
			if (EOS.ServerPlatformHandle) {
				EOS_Platform_Tick(EOS.ServerPlatformHandle);
			}
			EOS.StatGlobalManager.updateQueuedStats();
			EOS.AccountManager.handleLogin();
			EOS.CrossplayAccountManager.handleLogin();
#endif // USE_EOS

			DebugStats.t3SteamCallbacks = std::chrono::high_resolution_clock::now();

#ifdef USE_IMGUI
			ImGui_t::update();
#endif

			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				if ( nohud || intro || !players[i]->isLocalPlayer() || !MainMenu::isPlayerSignedIn(i) )
				{
					if (gameUIFrame[i])
					{
						gameUIFrame[i]->setDisabled(true);
					}
					StatusEffectQueue[i].resetQueue();
				}
				else
				{
					if (gameUIFrame[i])
					{
						gameUIFrame[i]->setDisabled(false);
					}
				}
			}

			if ( intro )
			{
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
						players[i]->ghost.reset();
						FollowerMenu[i].recentEntity = nullptr;
						FollowerMenu[i].followerToCommand = nullptr;
						FollowerMenu[i].entityToInteractWith = nullptr;
						CalloutMenu[i].closeCalloutMenuGUI();
						CalloutMenu[i].callouts.clear();
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
                    const float factor = xres / 1280.f;
					drawGear(xres / 2, yres / 2, gearsize, gearrot);
					drawLine(xres / 2 - 160 * factor, yres / 2 + 112 * factor, xres / 2 + 160 * factor, yres / 2 + 112 * factor, makeColorRGB(255, 32, 0), std::min<Uint16>(logoalpha, 255));
                    auto text = Text::get("Turning Wheel", "fonts/pixel_maz.ttf#32#2", makeColorRGB(255, 255, 255), makeColorRGB(0, 0, 0));
                    const SDL_Rect r{
                        (int)(xres - text->getWidth() * factor) / 2,
                        (int)(yres / 2 + 128 * factor),
                        (int)(text->getWidth() * factor),
                        (int)(text->getHeight() * factor)};
                    text->drawColor(SDL_Rect{0,0,0,0}, r, SDL_Rect{0, 0, xres, yres}, makeColor(255, 255, 255, std::min(logoalpha, (Uint16)255)));
					if ( logoalpha >= 255 && !fadeout )
					{
						fadeout = true;
						fadefinished = false;
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
					if ( Input::keys[SDLK_ESCAPE] )
					{
						Input::keys[SDLK_ESCAPE] = 0;
						skipButtonPressed = true;
					}

					if ( fadefinished || skipButtonPressed )
					{
						fadealpha = 255;
						int menuMapType = 0;
						switch ( local_rng.rand() % 4 ) // STEAM VERSION INTRO
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
					if (classtoquickstart[0] != '\0')
					{
						for ( c = 0; c <= CLASS_MONK; c++ ) {
							if ( !strcmp(classtoquickstart, playerClassLangEntry(c, 0)) ) {
								client_classes[0] = c;
								break;
							}
						}
						strcpy(classtoquickstart, "");

						// set class loadout
						strcpy(stats[0]->name, "Avatar");
						stats[0]->sex = static_cast<sex_t>(local_rng.rand() % 2);
						stats[0]->stat_appearance = local_rng.rand() % NUMAPPEARANCES;
						stats[0]->clearStats();
						initClass(0);
						if ( stats[0]->playerRace != RACE_HUMAN )
						{
							stats[0]->stat_appearance = 0;
						}

						// generate unique game key
						local_rng.seedTime();
						local_rng.getSeed(&uniqueGameKey, sizeof(uniqueGameKey));
						uniqueLobbyKey = local_rng.getU32();
						net_rng.seedBytes(&uniqueGameKey, sizeof(uniqueGameKey));
						doNewGame(false);
					}
					else
					{
						// draws the menu level "backdrop"
						drawClearBuffers();
						if ( !MainMenu::isCutsceneActive() && fadealpha < 255 )
						{
							menucam.winx = 0;
							menucam.winy = 0;
							menucam.winw = xres;
							menucam.winh = yres;
							light = addLight(menucam.x, menucam.y, "mainmenu");
							occlusionCulling(map, menucam);
                            beginGraphics();
							glBeginCamera(&menucam, true, map);
							glDrawWorld(&menucam, REALCOLORS);
							drawEntities3D(&menucam, REALCOLORS);
							glEndCamera(&menucam, true, map);
							list_RemoveNode(light->node);
						}

						MainMenu::doMainMenu(!intro);
						UIToastNotificationManager.drawNotifications(MainMenu::isCutsceneActive(), true); // draw this before the cursor
                        framesProcResult = doFrames();

#ifdef USE_IMGUI
						ImGui_t::render();
#endif
#ifndef NINTENDO
						Compendium_t::updateTooltip();

						// draw mouse
						// only draw 1 cursor in the main menu
						if ( inputs.getVirtualMouse(inputs.getPlayerIDAllowedKeyboard())->draw_cursor )
						{
                            const float factorX = (float)xres / Frame::virtualScreenX;
                            const float factorY = (float)yres / Frame::virtualScreenY;
							auto cursor = Image::get("*#images/system/cursor_hand.png");
                            const int w = cursor->getWidth() * factorX;
                            const int h = cursor->getHeight() * factorY;

							if ( inputs.getPlayerIDAllowedKeyboard() >= 0 )
							{
								real_t& mouseAnim = inputs.getVirtualMouse(inputs.getPlayerIDAllowedKeyboard())->mouseAnimationPercent;
								if ( Input::inputs[inputs.getPlayerIDAllowedKeyboard()].binary("MenuLeftClick") )
								{
									mouseAnim = .5;
								}
								if ( mouseAnim > .25 )
								{
									cursor = Image::get("*#images/system/cursor_hand2.png");
								}
								if ( mouseAnim > 0.0 )
								{
									mouseAnim -= .05 * getFPSScale(144.0);
								}

								pos.x = inputs.getMouse(inputs.getPlayerIDAllowedKeyboard(), Inputs::X) - (mouseAnim * w / 7) - w / 2;
								pos.y = inputs.getMouse(inputs.getPlayerIDAllowedKeyboard(), Inputs::Y) - (mouseAnim * h / 7) - h / 2;
								pos.x += 4;
								pos.y += 4;
								pos.w = w;
								pos.h = h;
								cursor->draw(nullptr, pos, SDL_Rect{ 0, 0, xres, yres });

								if ( MainMenu::cursor_delete_mode )
								{
									auto icon = Image::get("*#images/system/Broken.png");
									pos.x = pos.x + pos.w;
									pos.y = pos.y + pos.h;
									pos.w = icon->getWidth() * 2;
									pos.h = icon->getHeight() * 2;
									icon->draw(nullptr, pos, SDL_Rect{ 0, 0, xres, yres });
								}
							}
							else
							{
								pos.x = inputs.getMouse(inputs.getPlayerIDAllowedKeyboard(), Inputs::X) - w / 2;
								pos.y = inputs.getMouse(inputs.getPlayerIDAllowedKeyboard(), Inputs::Y) - h / 2;
								pos.x += 4;
								pos.y += 4;
								pos.w = w;
								pos.h = h;
								cursor->draw(nullptr, pos, SDL_Rect{0, 0, xres, yres});

								if (MainMenu::cursor_delete_mode)
								{
									auto icon = Image::get("*#images/system/Broken.png");
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

				// toggling the game menu
				bool doPause = false;
				bool doCompendium = false;
				if ( !fadeout )
				{
				    bool noOneUsingKeyboard = true;
					for ( int i = 0; i < MAXPLAYERS; ++i )
					{
					    if (inputs.bPlayerUsingKeyboardControl(i) && MainMenu::isPlayerSignedIn(i) && players[i]->isLocalPlayer()) {
					        noOneUsingKeyboard = false;
					        break;
					    }
					}
					for ( int i = 0; i < MAXPLAYERS; ++i )
					{
						if ( !players[i]->isLocalPlayer() )
						{
							continue;
						}
						const bool escapePressed =
							inputs.bPlayerUsingKeyboardControl(i) &&
							keystatus[SDLK_ESCAPE] &&
							!Input::inputs[i].isDisabled();
						if ( (Input::inputs[i].consumeBinaryToggle("Pause Game") || escapePressed) && !command )
						{
							if ( !players[i]->shootmode )
							{
								players[i]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_CLOSE_ALL);
								players[i]->characterSheet.attributespage = 0;
								if (escapePressed) {
									keystatus[SDLK_ESCAPE] = false;
								}
							}
							else
							{
								doPause = true;
							}
							break;
						}
						const bool compendiumOpenToggle =
							!intro
							&& inputs.bPlayerUsingKeyboardControl(i)
							&& !Input::inputs[i].isDisabled()
							&& !command;
						if ( compendiumOpenToggle )
						{
							if ( Input::inputs[i].consumeBinaryToggle("Compendium") )
							{
								doCompendium = true;
							}
						}
					}
					if (noOneUsingKeyboard && keystatus[SDLK_ESCAPE]) {
					    doPause = true;
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
							else
							{
								keystatus[SDLK_ESCAPE] = 0;
							}
						}
						else
						{
							pauseGame(0, MAXPLAYERS);
						}
					}
				}
				else if ( doCompendium )
				{
					bool isOpen = false;
					if ( MainMenu::main_menu_frame )
					{
						if ( auto compendium = MainMenu::main_menu_frame->findFrame("compendium") )
						{
							isOpen = true;
						}
					}
					if ( !isOpen )
					{
						if ( !gamePaused )
						{
							pauseGame(0, MAXPLAYERS);
						}
						if ( !gamePaused )
						{
							doCompendium = false;
						}
					}
					else if ( isOpen )
					{
						if ( gamePaused )
						{
							pauseGame(0, MAXPLAYERS);
							doCompendium = false;
							if ( !gamePaused )
							{
								MainMenu::openCompendium(); // will close
							}
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

				if ( !MainMenu::isCutsceneActive() && fadealpha < 255 )
				{
					drawAllPlayerCameras();
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


				for ( int player = 0; player < MAXPLAYERS; ++player )
				{
					players[player]->messageZone.updateMessages();
					CalloutMenu[player].update();
				}
				if ( !nohud )
				{
					DamageIndicatorHandler.update();
					for ( int player = 0; player < MAXPLAYERS; ++player )
					{
						if ( players[player]->isLocalPlayer() )
						{
							players[player]->messageZone.drawMessages();
						}
					}
				}

				DebugStats.t5MainDraw = std::chrono::high_resolution_clock::now();
				framesProcResult = doFrames();
				DebugStats.t6Messages = std::chrono::high_resolution_clock::now();
				ingameHud();
				Compendium_t::updateTooltip();

                static ConsoleVariable<bool> showConsumeMouseInputs("/debug_consume_mouse", false);
                if (!framesProcResult.usable) {
				    if (*showConsumeMouseInputs) {
				        printText(font8x8_bmp, 16, 16, "eating mouse input");
				    }
				} else {
				    if (*showConsumeMouseInputs) {
				        printText(font8x8_bmp, 16, 16, "NOT eating mouse input");
				    }
				}

				if ( gamePaused )
				{
					MainMenu::doMainMenu(!intro);
				}
				else
				{
					MainMenu::destroyMainMenu();

					// draw subwindow
					if ( subwindow )
					{
						drawWindowFancy(subx1, suby1, subx2, suby2);
						if ( subtext[0] != '\0')
						{
							if ( strncmp(subtext, Language::get(1133), 12) )
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

				if ( doCompendium )
				{
					if ( gamePaused )
					{
						MainMenu::openCompendium();
					}
				}

				if ( gamePaused ) // draw after main menu windows etc.
				{
					UIToastNotificationManager.drawNotifications(MainMenu::isCutsceneActive(), true); // draw this before the cursor
				}

#ifdef USE_IMGUI
				ImGui_t::render();
#endif

#ifndef NINTENDO
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( gamePaused || players[i]->GUI.isGameoverActive() )
					{
						if ( inputs.bPlayerUsingKeyboardControl(i) && inputs.getVirtualMouse(i)->draw_cursor )
						{
                            const float factorX = (float)xres / Frame::virtualScreenX;
                            const float factorY = (float)yres / Frame::virtualScreenY;
							auto cursor = Image::get("*#images/system/cursor_hand.png");
                            const int w = cursor->getWidth() * factorX;
                            const int h = cursor->getHeight() * factorY;

							real_t& mouseAnim = inputs.getVirtualMouse(i)->mouseAnimationPercent;
							if ( Input::inputs[i].binary("MenuLeftClick") )
							{
								mouseAnim = .5;
							}
							if ( mouseAnim > .25 )
							{
								cursor = Image::get("*#images/system/cursor_hand2.png");
							}
							if ( mouseAnim > 0.0 )
							{
								mouseAnim -= .05 * getFPSScale(144.0);
							}

                            pos.x = inputs.getMouse(i, Inputs::X) - (mouseAnim * w / 7) - w / 2;
                            pos.y = inputs.getMouse(i, Inputs::Y) - (mouseAnim * h / 7) - h / 2;
                            pos.x += 4;
                            pos.y += 4;
                            pos.w = w;
                            pos.h = h;
							cursor->draw(nullptr, pos, SDL_Rect{ 0, 0, xres, yres });
						}
						continue;
					}
				}
#endif
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
				printTextFormatted(font8x8_bmp, 8, 20, "gui module: %d\ngui mode: %d", players[0]->GUI.activeModule, players[0]->gui_mode);
				static ConsoleVariable<bool> cvar_map_debug("/mapdebug", false);
				if ( *cvar_map_debug )
				{
					int ix = (int)cameras[clientnum].x;
					int iy = (int)cameras[clientnum].y;
					if ( ix >= 0 && ix < map.width && iy >= 0 && iy < map.height )
					{
						printTextFormatted(font8x8_bmp, 8, 44, "pos: x: %d y: %d pathmapZone: %d", 
							ix, iy, pathMapGrounded[iy + ix * map.height]);
					}
				}
				static ConsoleVariable<bool> cvar_light_debug("/lightdebug", false);
				if ( *cvar_light_debug )
				{
					if ( players[clientnum]->entity )
					{
						int light = players[clientnum]->entity->entityLight();
						int tiles = light / 16;
						int lightAfterReductions = std::max(TOUCHRANGE, 
							players[clientnum]->entity->entityLightAfterReductions(*stats[clientnum], players[clientnum]->entity));
						int tilesAfterReductions = lightAfterReductions / 16;
						printTextFormatted(font8x8_bmp, 8, 44, "base light: %3d, tiles: %2d | modified light: %3d, tiles: %2d",
							light, tiles, lightAfterReductions, tilesAfterReductions);
					}
				}
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

			static ConsoleVariable<bool> cvar_frame_search_count("/framesearchcount", false);
			if ( *cvar_frame_search_count )
			{
				printTextFormatted(font8x8_bmp, 300, 32, "findFrame() calls: %d / loop", Frame::numFindFrameCalls);
			}

			UIToastNotificationManager.drawNotifications(MainMenu::isCutsceneActive(), false);

#ifdef USE_THEORA_VIDEO
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				VideoManager[i].update();
			}
#endif

			// update screen
			GO_SwapBuffers(screen);

			// screenshots
			if (Input::inputs[clientnum].consumeBinaryToggle("Screenshot") ||
			    (inputs.hasController(clientnum) && Input::inputs[clientnum].consumeBinaryToggle("GamepadScreenshot")))
			{
                if (!hdrEnabled) {
                    main_framebuffer.unbindForWriting();
                }
				takeScreenshot();
                if (!hdrEnabled) {
                    main_framebuffer.bindForWriting();
                }
			}

			// frame rate limiter
			while ( frameRateLimit(fpsLimit, true, true) )
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

			if (ranframes)
			{
				Input::mouseButtons[Input::MOUSE_WHEEL_UP] = false;
				Input::mouseButtons[Input::MOUSE_WHEEL_DOWN] = false;
				mousexrel = 0;
				mouseyrel = 0;
				if (initialized)
				{
					inputs.updateAllRelMouse();
					for ( int i = 0; i < MAXPLAYERS; ++i )
					{
						if ( inputs.hasController(i) )
						{
							if (inputs.getController(i))
							{
#ifndef NINTENDO
								(void)inputs.getController(i)->handleRumble();
#endif
								inputs.getController(i)->updateButtonsReleased();
							}
						}
					}
				}
			}
			Text::dumpCacheInMainLoop();
			achievementObserver.getCurrentPlayerUids();
			DebugStats.t11End = std::chrono::high_resolution_clock::now();

			// increase the cycle count
			cycles++;
		}
		if ( !load_successful ) {
			skipintro = true;
		}
		saveConfig("default.cfg");
		MainMenu::settingsMount(false);
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
